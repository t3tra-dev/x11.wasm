/*
 * XlibInt.c --
 *
 *	Shared state and common helpers for the x11.wasm libX11 compatibility
 *	layer.
 */

#include "XlibInt.h"

X11WasmDisplayState browserDisplay;
int browserDisplayInitialized;
X11WasmAtomName *atomList;
X11WasmSelectionOwner *selectionOwnerList;
X11WasmGCClip *gcClipList;
X11WasmGCDashes *gcDashList;
X11WasmTrackedGC *gcList;
X11WasmIMInstantiateCallback *imInstantiateCallbackList;
XIC focusedInputContext;
char *localeModifiers;
char *externalClipboardText;
int externalClipboardAvailable;
Time externalClipboardTime;
Atom nextAtom = XA_LAST_PREDEFINED;
XErrorHandler errorHandler;
XIOErrorHandler ioErrorHandler;
X11WasmQueuedEvent eventQueue[X11_WASM_EVENT_QUEUE_LIMIT];
int eventQueueHead;
int eventQueueCount;
unsigned long eventSerial = 1;
unsigned long damageSerial = 1;
Time eventTime = 1;
int browserTestXErrorCount;
int browserTestXIOErrorCount;
int browserTestIMInstantiateCount;
Display *browserTestIMInstantiateDisplay;
XPointer browserTestIMInstantiateClientData;
XPointer browserTestIMInstantiateCallData;
int browserTestIMDestroyCount;
XIM browserTestIMDestroyMethod;
XPointer browserTestIMDestroyClientData;
XPointer browserTestIMDestroyCallData;
unsigned char browserTestLastErrorCode;
unsigned char browserTestLastRequestCode;
XID browserTestLastResourceId;
unsigned long browserTestLastSerial;
unsigned int browserTestRequestCodeSum;
unsigned long long browserTestRequestCodeMask;

void BrowserSyncHostAccessControl(void);
void BrowserSyncServerGrabState(void);
void BrowserSyncSynchronizeState(Display *display);

void
BrowserClearHostAccessList(void)
{
    int i;

    for (i = 0; i < browserDisplay.hostCount; i++) {
	if (browserDisplay.hosts[i].address != NULL) {
	    ckfree(browserDisplay.hosts[i].address);
	}
    }
    if (browserDisplay.hosts != NULL) {
	ckfree(browserDisplay.hosts);
    }
    browserDisplay.hosts = NULL;
    browserDisplay.hostCount = 0;
}

void
BrowserResetHostAccessControl(void)
{
    BrowserClearHostAccessList();
    browserDisplay.hostAccessEnabled = True;
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

    for (i = 0; i < browserDisplay.hostCount; i++) {
	if (BrowserHostAddressMatches(&browserDisplay.hosts[i], hostPtr)) {
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
	    * (size_t)(browserDisplay.hostCount + 1));
    if (browserDisplay.hostCount > 0) {
	memcpy(newHosts, browserDisplay.hosts, sizeof(X11WasmHostEntry)
		* (size_t)browserDisplay.hostCount);
	ckfree(browserDisplay.hosts);
    }
    browserDisplay.hosts = newHosts;
    entryPtr = &browserDisplay.hosts[browserDisplay.hostCount];
    entryPtr->family = hostPtr->family;
    entryPtr->length = hostPtr->length;
    entryPtr->address = NULL;
    if (hostPtr->length > 0) {
	entryPtr->address = (char *)ckalloc((size_t)hostPtr->length);
	memcpy(entryPtr->address, hostPtr->address, (size_t)hostPtr->length);
    }
    browserDisplay.hostCount++;
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
    if (browserDisplay.hosts[index].address != NULL) {
	ckfree(browserDisplay.hosts[index].address);
    }
    browserDisplay.hostCount--;
    if (index < browserDisplay.hostCount) {
	memmove(&browserDisplay.hosts[index], &browserDisplay.hosts[index + 1],
		sizeof(X11WasmHostEntry)
		* (size_t)(browserDisplay.hostCount - index));
    }
    if (browserDisplay.hostCount == 0) {
	ckfree(browserDisplay.hosts);
	browserDisplay.hosts = NULL;
    }
    BrowserSyncHostAccessControl();
}

const X11WasmKeySymName browserKeySymNames[] = {
#include "tables/KeySyms.inc"
};
const size_t browserKeySymNameCount = sizeof(browserKeySymNames) / sizeof(browserKeySymNames[0]);

const X11WasmExtensionInfo browserExtensions[] = {
    {
	X11_WASM_EXTENSION_NAME,
	X11_WASM_EXTENSION_MAJOR_OPCODE,
	0,
	0
    }
};
const size_t browserExtensionCount = sizeof(browserExtensions) / sizeof(browserExtensions[0]);

const X11WasmNamedColor browserNamedColors[] = {
#include "tables/XColors.inc"
};
const size_t browserNamedColorCount = sizeof(browserNamedColors) / sizeof(browserNamedColors[0]);
