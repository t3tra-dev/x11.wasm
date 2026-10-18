/*
 * XlibInt.c --
 *
 *	Shared state and common helpers for the x11.wasm libX11 compatibility
 *	layer.
 */

#include "XlibInt.h"

X11WasmDisplayState x11wasm_browserDisplay;
int x11wasm_browserDisplayInitialized;
X11WasmAtomName *x11wasm_atomList;
X11WasmSelectionOwner *x11wasm_selectionOwnerList;
X11WasmGCClip *x11wasm_gcClipList;
X11WasmGCDashes *x11wasm_gcDashList;
X11WasmTrackedGC *x11wasm_gcList;
X11WasmKeyBinding *x11wasm_keyBindingList;
X11WasmIMInstantiateCallback *x11wasm_imInstantiateCallbackList;
XIC x11wasm_focusedInputContext;
char *x11wasm_localeModifiers;
char *x11wasm_externalClipboardText;
int x11wasm_externalClipboardAvailable;
Time x11wasm_externalClipboardTime;
Atom x11wasm_nextAtom = XA_LAST_PREDEFINED;
XErrorHandler x11wasm_errorHandler;
XIOErrorHandler x11wasm_ioErrorHandler;
X11WasmQueuedEvent *x11wasm_eventQueue;
X11WasmCompositionPayload *x11wasm_compositionPayloadList;
unsigned long x11wasm_nextCompositionPayloadId = 1;
int x11wasm_eventQueueHead;
int x11wasm_eventQueueCount;
int x11wasm_eventQueueCapacity;
unsigned long x11wasm_eventSerial = 1;
unsigned long x11wasm_damageSerial = 1;
Time x11wasm_eventTime = 1;
int x11wasm_browserTestXErrorCount;
int x11wasm_browserTestXIOErrorCount;
int x11wasm_browserTestIMInstantiateCount;
Display *x11wasm_browserTestIMInstantiateDisplay;
XPointer x11wasm_browserTestIMInstantiateClientData;
XPointer x11wasm_browserTestIMInstantiateCallData;
int x11wasm_browserTestIMDestroyCount;
XIM x11wasm_browserTestIMDestroyMethod;
XPointer x11wasm_browserTestIMDestroyClientData;
XPointer x11wasm_browserTestIMDestroyCallData;
unsigned char x11wasm_browserTestLastErrorCode;
unsigned char x11wasm_browserTestLastRequestCode;
XID x11wasm_browserTestLastResourceId;
unsigned long x11wasm_browserTestLastSerial;
unsigned int x11wasm_browserTestRequestCodeSum;
unsigned long long x11wasm_browserTestRequestCodeMask;

void BrowserSyncHostAccessControl(void);
void BrowserSyncServerGrabState(void);
void BrowserSyncSynchronizeState(Display *display);

void
BrowserClearHostAccessList(void)
{
    int i;

    for (i = 0; i < x11wasm_browserDisplay.hostCount; i++) {
	if (x11wasm_browserDisplay.hosts[i].address != NULL) {
	    ckfree(x11wasm_browserDisplay.hosts[i].address);
	}
    }
    if (x11wasm_browserDisplay.hosts != NULL) {
	ckfree(x11wasm_browserDisplay.hosts);
    }
    x11wasm_browserDisplay.hosts = NULL;
    x11wasm_browserDisplay.hostCount = 0;
}

void
BrowserResetHostAccessControl(void)
{
    BrowserClearHostAccessList();
    x11wasm_browserDisplay.hostAccessEnabled = True;
    BrowserSyncHostAccessControl();
}

int
BrowserHostFamilyIsValid(
    int family)
{
    return (family == FamilyInternet) || (family == FamilyInternet6)
	    || (family == FamilyDECnet) || (family == FamilyChaos)
	    || (family == FamilyServerInterpreted);
}

int
BrowserValidateHostAddress(
    Display **displayPtr,
    XHostAddress *hostPtr,
    unsigned char requestCode)
{
    if (hostPtr == NULL) {
	return BrowserValueOrError(displayPtr, 0, 0, requestCode);
    }
    if (!BrowserHostFamilyIsValid(hostPtr->family)) {
	return BrowserValueOrError(displayPtr, hostPtr->family, 0,
		requestCode);
    }
    if (hostPtr->length < 0) {
	return BrowserValueOrError(displayPtr, hostPtr->length, 0,
		requestCode);
    }
    if ((hostPtr->length > 0) && (hostPtr->address == NULL)) {
	return BrowserValueOrError(displayPtr, 0, 0, requestCode);
    }
    return 1;
}

int
BrowserHostAddressMatches(
    const X11WasmHostEntry *entryPtr,
    const XHostAddress *hostPtr)
{
    if ((entryPtr->family != hostPtr->family)
	    || (entryPtr->length != hostPtr->length)) {
	return 0;
    }
    if (entryPtr->length == 0) {
	return 1;
    }
    return memcmp(entryPtr->address, hostPtr->address,
	    (size_t)entryPtr->length) == 0;
}

int
BrowserFindHostAddress(
    const XHostAddress *hostPtr)
{
    int i;

    for (i = 0; i < x11wasm_browserDisplay.hostCount; i++) {
	if (BrowserHostAddressMatches(&x11wasm_browserDisplay.hosts[i], hostPtr)) {
	    return i;
	}
    }
    return -1;
}

void
BrowserRememberHostAddress(
    const XHostAddress *hostPtr)
{
    X11WasmHostEntry *newHosts;
    X11WasmHostEntry *entryPtr;

    if (BrowserFindHostAddress(hostPtr) >= 0) {
	return;
    }
    newHosts = (X11WasmHostEntry *)ckalloc(sizeof(X11WasmHostEntry)
	    * (size_t)(x11wasm_browserDisplay.hostCount + 1));
    if (x11wasm_browserDisplay.hostCount > 0) {
	memcpy(newHosts, x11wasm_browserDisplay.hosts, sizeof(X11WasmHostEntry)
		* (size_t)x11wasm_browserDisplay.hostCount);
	ckfree(x11wasm_browserDisplay.hosts);
    }
    x11wasm_browserDisplay.hosts = newHosts;
    entryPtr = &x11wasm_browserDisplay.hosts[x11wasm_browserDisplay.hostCount];
    entryPtr->family = hostPtr->family;
    entryPtr->length = hostPtr->length;
    entryPtr->address = NULL;
    if (hostPtr->length > 0) {
	entryPtr->address = (char *)ckalloc((size_t)hostPtr->length);
	memcpy(entryPtr->address, hostPtr->address, (size_t)hostPtr->length);
    }
    x11wasm_browserDisplay.hostCount++;
    BrowserSyncHostAccessControl();
}

void
BrowserForgetHostAddress(
    const XHostAddress *hostPtr)
{
    int index = BrowserFindHostAddress(hostPtr);

    if (index < 0) {
	return;
    }
    if (x11wasm_browserDisplay.hosts[index].address != NULL) {
	ckfree(x11wasm_browserDisplay.hosts[index].address);
    }
    x11wasm_browserDisplay.hostCount--;
    if (index < x11wasm_browserDisplay.hostCount) {
	memmove(&x11wasm_browserDisplay.hosts[index], &x11wasm_browserDisplay.hosts[index + 1],
		sizeof(X11WasmHostEntry)
		* (size_t)(x11wasm_browserDisplay.hostCount - index));
    }
    if (x11wasm_browserDisplay.hostCount == 0) {
	ckfree(x11wasm_browserDisplay.hosts);
	x11wasm_browserDisplay.hosts = NULL;
    }
    BrowserSyncHostAccessControl();
}

const X11WasmKeySymName x11wasm_browserKeySymNames[] = {
#include "tables/KeySyms.inc"
};
const size_t x11wasm_browserKeySymNameCount = sizeof(x11wasm_browserKeySymNames) / sizeof(x11wasm_browserKeySymNames[0]);

const X11WasmExtensionInfo x11wasm_browserExtensions[] = {
    {
	X11_WASM_EXTENSION_NAME,
	X11_WASM_EXTENSION_MAJOR_OPCODE,
	0,
	0
    },
    {
	X11_WASM_BIG_REQUESTS_EXTENSION_NAME,
	X11_WASM_BIG_REQUESTS_MAJOR_OPCODE,
	0,
	0
    },
    {
	ScreenSaverName,
	X11_WASM_SCREENSAVER_MAJOR_OPCODE,
	X11_WASM_SCREENSAVER_EVENT_BASE,
	0
    }
};
const size_t x11wasm_browserExtensionCount = sizeof(x11wasm_browserExtensions) / sizeof(x11wasm_browserExtensions[0]);

const X11WasmNamedColor x11wasm_browserNamedColors[] = {
#include "tables/XColors.inc"
};
const size_t x11wasm_browserNamedColorCount = sizeof(x11wasm_browserNamedColors) / sizeof(x11wasm_browserNamedColors[0]);
