#include "XlibInt.h"

int
BrowserPropertyElementBytes(
    int format)
{
    switch (format) {
    case 8:
	return 1;
    case 16:
	return 2;
    case 32:
	return 4;
    default:
	return 0;
    }
}

int
BrowserAtomIsValid(
    Atom atom)
{
    X11WasmAtomName *atomPtr;

    if ((atom >= 1) && (atom <= XA_LAST_PREDEFINED)) {
	return 1;
    }
    for (atomPtr = x11wasm_atomList; atomPtr != NULL; atomPtr = atomPtr->nextPtr) {
	if (atomPtr->atom == atom) {
	    return 1;
	}
    }
    return 0;
}

int
BrowserPropertyAtomOrError(
    Display **displayPtr,
    Atom atom,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

    if (BrowserAtomIsValid(atom)) {
	return 1;
    }
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    (void)BrowserReportXError(display, BadAtom, (XID)atom, requestCode, 0);
    return 0;
}

int
BrowserOptionalPropertyAtomOrError(
    Display **displayPtr,
    Atom atom,
    unsigned char requestCode)
{
    return (atom == None) || BrowserPropertyAtomOrError(displayPtr, atom,
	    requestCode);
}

int
BrowserPropertyValueOrError(
    Display **displayPtr,
    XID resourceId,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    (void)BrowserReportXError(display, BadValue, resourceId, requestCode, 0);
    return 0;
}

int
BrowserPropertyChangeOrError(
    Display **displayPtr,
    X11WasmProperty *propertyPtr,
    Atom property,
    Atom type,
    int format,
    int mode,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

    if ((mode != PropModeAppend) && (mode != PropModePrepend)
	    && (mode != PropModeReplace)) {
	return BrowserPropertyValueOrError(displayPtr, (XID)mode,
		requestCode);
    }
    if (BrowserPropertyElementBytes(format) == 0) {
	return BrowserPropertyValueOrError(displayPtr, (XID)format,
		requestCode);
    }
    if ((propertyPtr == NULL) || (propertyPtr->data == NULL)
	    || (mode == PropModeReplace)
	    || ((propertyPtr->type == type) && (propertyPtr->format == format))) {
	return 1;
    }
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    (void)BrowserReportXError(display, BadMatch, (XID)property, requestCode,
	    0);
    return 0;
}

int
BrowserPropertyByteLength(
    int format,
    int nelements)
{
    int elementBytes = BrowserPropertyElementBytes(format);

    if ((elementBytes == 0) || (nelements <= 0)) {
	return 0;
    }
    if (nelements > (INT_MAX / elementBytes)) {
	return -1;
    }
    return elementBytes * nelements;
}

Atom
BrowserPropertyAtomAt(
    const unsigned char *data,
    unsigned long index)
{
    unsigned int atomValue = 0;

    if (data != NULL) {
	memcpy(&atomValue, data + (index * 4U), sizeof(atomValue));
    }
    return (Atom)atomValue;
}

X11WasmProperty *
BrowserFindProperty(
    X11WasmResource *resourcePtr,
    Atom atom)
{
    X11WasmProperty *propertyPtr;

    if (resourcePtr == NULL) {
	return NULL;
    }
    for (propertyPtr = resourcePtr->propertyList; propertyPtr != NULL;
	    propertyPtr = propertyPtr->nextPtr) {
	if (propertyPtr->atom == atom) {
	    return propertyPtr;
	}
    }
    return NULL;
}

X11WasmProperty *
BrowserCreateProperty(
    X11WasmResource *resourcePtr,
    Atom atom)
{
    X11WasmProperty *propertyPtr;

    if (resourcePtr == NULL) {
	return NULL;
    }
    propertyPtr = BrowserFindProperty(resourcePtr, atom);
    if (propertyPtr != NULL) {
	return propertyPtr;
    }
    propertyPtr = (X11WasmProperty *)ckalloc(sizeof(X11WasmProperty));
    memset(propertyPtr, 0, sizeof(X11WasmProperty));
    propertyPtr->atom = atom;
    propertyPtr->nextPtr = resourcePtr->propertyList;
    resourcePtr->propertyList = propertyPtr;
    return propertyPtr;
}

void
BrowserFreeProperty(
    X11WasmProperty *propertyPtr)
{
    if (propertyPtr == NULL) {
	return;
    }
    if (propertyPtr->data != NULL) {
	ckfree(propertyPtr->data);
    }
    ckfree(propertyPtr);
}

void
BrowserFreePropertyList(
    X11WasmResource *resourcePtr)
{
    X11WasmProperty *propertyPtr;

    if (resourcePtr == NULL) {
	return;
    }
    propertyPtr = resourcePtr->propertyList;
    while (propertyPtr != NULL) {
	X11WasmProperty *nextPtr = propertyPtr->nextPtr;

	BrowserFreeProperty(propertyPtr);
	propertyPtr = nextPtr;
    }
    resourcePtr->propertyList = NULL;
}

int
BrowserDeleteProperty(
    X11WasmResource *resourcePtr,
    Atom atom)
{
    X11WasmProperty **linkPtr;

    if (resourcePtr == NULL) {
	return 0;
    }
    linkPtr = &resourcePtr->propertyList;
    while (*linkPtr != NULL) {
	X11WasmProperty *propertyPtr = *linkPtr;

	if (propertyPtr->atom == atom) {
	    *linkPtr = propertyPtr->nextPtr;
	    BrowserFreeProperty(propertyPtr);
	    return 1;
	}
	linkPtr = &propertyPtr->nextPtr;
    }
    return 0;
}

void
BrowserNotifyProperty(
    X11WasmResource *resourcePtr,
    Atom atom,
    int state)
{
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || !BrowserResourceWantsEvent(resourcePtr, PropertyChangeMask)) {
	return;
    }
    BrowserSetCommonEventFields(&event, PropertyNotify,
	    (Window)resourcePtr->id, CurrentTime);
    event.xproperty.window = (Window)resourcePtr->id;
    event.xproperty.atom = atom;
    event.xproperty.time = x11wasm_eventTime;
    event.xproperty.state = state;
    (void)BrowserQueueEvent(&event);
}

int
BrowserSetProperty(
    X11WasmResource *resourcePtr,
    Atom atom,
    Atom type,
    int format,
    int mode,
    _Xconst unsigned char *data,
    int nelements)
{
    X11WasmProperty *propertyPtr;
    unsigned char *newData;
    int newLength = BrowserPropertyByteLength(format, nelements);
    int oldLength;

    if ((resourcePtr == NULL) || (atom == None)
	    || (BrowserPropertyElementBytes(format) == 0) || (nelements < 0)
	    || (newLength < 0)
	    || ((mode != PropModeReplace) && (mode != PropModePrepend)
		&& (mode != PropModeAppend))) {
	return 0;
    }
    if ((newLength > 0) && (data == NULL)) {
	return 0;
    }
    propertyPtr = BrowserCreateProperty(resourcePtr, atom);
    if (propertyPtr == NULL) {
	return 0;
    }
    if ((propertyPtr->data != NULL)
	    && ((mode == PropModeAppend) || (mode == PropModePrepend))
	    && ((propertyPtr->type != type) || (propertyPtr->format != format))) {
	mode = PropModeReplace;
    }

    oldLength = (mode == PropModeReplace) ? 0 : propertyPtr->dataLength;
    if (oldLength > (INT_MAX - newLength)) {
	return 0;
    }
    newData = (unsigned char *)ckalloc((size_t)oldLength + (size_t)newLength + 1U);
    if (mode == PropModePrepend) {
	if (newLength > 0) {
	    memcpy(newData, data, (size_t)newLength);
	}
	if (oldLength > 0) {
	    memcpy(newData + newLength, propertyPtr->data, (size_t)oldLength);
	}
    } else {
	if (oldLength > 0) {
	    memcpy(newData, propertyPtr->data, (size_t)oldLength);
	}
	if (newLength > 0) {
	    memcpy(newData + oldLength, data, (size_t)newLength);
	}
    }
    newData[oldLength + newLength] = '\0';
    if (propertyPtr->data != NULL) {
	ckfree(propertyPtr->data);
    }
    propertyPtr->type = type;
    propertyPtr->format = format;
    propertyPtr->data = newData;
    propertyPtr->dataLength = oldLength + newLength;
    propertyPtr->nitems = (unsigned long)(propertyPtr->dataLength
	    / BrowserPropertyElementBytes(format));
    BrowserNotifyProperty(resourcePtr, atom, PropertyNewValue);
    return 1;
}

X11WasmResource *
BrowserColormapResource(
    Colormap colormap)
{
    X11WasmResource *resourcePtr;

    if ((colormap == None) || (colormap == x11wasm_browserDisplay.defaultColormap)) {
	return NULL;
    }
    resourcePtr = X11WasmFindResource((XID)colormap);
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_COLORMAP)) {
	return NULL;
    }
    return resourcePtr;
}

void
BrowserRememberColor(
    Colormap colormap,
    const XColor *colorPtr)
{
    X11WasmResource *resourcePtr;
    X11WasmColorEntry *entryPtr;

    if (colorPtr == NULL) {
	return;
    }
    resourcePtr = BrowserColormapResource(colormap);
    if (resourcePtr == NULL) {
	return;
    }
    for (entryPtr = resourcePtr->colorList; entryPtr != NULL;
	    entryPtr = entryPtr->nextPtr) {
	if (entryPtr->pixel == colorPtr->pixel) {
	    break;
	}
    }
    if (entryPtr == NULL) {
	entryPtr = (X11WasmColorEntry *)ckalloc(sizeof(X11WasmColorEntry));
	memset(entryPtr, 0, sizeof(X11WasmColorEntry));
	entryPtr->pixel = colorPtr->pixel;
	entryPtr->nextPtr = resourcePtr->colorList;
	resourcePtr->colorList = entryPtr;
    }
    entryPtr->red = colorPtr->red;
    entryPtr->green = colorPtr->green;
    entryPtr->blue = colorPtr->blue;
    entryPtr->flags = colorPtr->flags;
}

unsigned long
BrowserReserveColorPixel(
    Colormap colormap)
{
    X11WasmResource *resourcePtr = BrowserColormapResource(colormap);
    unsigned long pixel;

    if (resourcePtr == NULL) {
	return 0;
    }
    for (pixel = 0; ; pixel++) {
	X11WasmColorEntry *entryPtr;
	int inUse = 0;

	for (entryPtr = resourcePtr->colorList; entryPtr != NULL;
		entryPtr = entryPtr->nextPtr) {
	    if (entryPtr->pixel == pixel) {
		inUse = 1;
		break;
	    }
	}
	if (!inUse) {
	    return pixel;
	}
    }
}

void
BrowserCopyColormapColors(
    Colormap source,
    Colormap target)
{
    X11WasmResource *sourcePtr = BrowserColormapResource(source);
    X11WasmColorEntry *entryPtr;

    if (sourcePtr == NULL) {
	return;
    }
    for (entryPtr = sourcePtr->colorList; entryPtr != NULL;
	    entryPtr = entryPtr->nextPtr) {
	XColor color;

	color.pixel = entryPtr->pixel;
	color.red = entryPtr->red;
	color.green = entryPtr->green;
	color.blue = entryPtr->blue;
	color.flags = entryPtr->flags;
	BrowserRememberColor(target, &color);
    }
}

void
BrowserForgetColors(
    Colormap colormap,
    unsigned long *pixels,
    int count,
    unsigned long planes)
{
    X11WasmResource *resourcePtr;
    int i;

    if ((pixels == NULL) || (count <= 0)) {
	return;
    }
    resourcePtr = BrowserColormapResource(colormap);
    if (resourcePtr == NULL) {
	return;
    }
    for (i = 0; i < count; i++) {
	X11WasmColorEntry **linkPtr = &resourcePtr->colorList;
	unsigned long basePixel = pixels[i] & ~planes;
	unsigned long requiredPlaneBits = pixels[i] & planes;

	while (*linkPtr != NULL) {
	    X11WasmColorEntry *entryPtr = *linkPtr;

	    if (((entryPtr->pixel & ~planes) == basePixel)
		    && ((entryPtr->pixel & requiredPlaneBits)
		    == requiredPlaneBits)) {
		*linkPtr = entryPtr->nextPtr;
		ckfree(entryPtr);
		continue;
	    }
	    linkPtr = &entryPtr->nextPtr;
	}
    }
}

int
BrowserLookupColorEntry(
    Colormap colormap,
    unsigned long pixel,
    XColor *colorPtr)
{
    X11WasmResource *resourcePtr;
    X11WasmColorEntry *entryPtr;

    resourcePtr = BrowserColormapResource(colormap);
    if (resourcePtr == NULL) {
	return 0;
    }
    for (entryPtr = resourcePtr->colorList; entryPtr != NULL;
	    entryPtr = entryPtr->nextPtr) {
	if (entryPtr->pixel == pixel) {
	    if (colorPtr != NULL) {
		colorPtr->pixel = entryPtr->pixel;
		colorPtr->red = entryPtr->red;
		colorPtr->green = entryPtr->green;
		colorPtr->blue = entryPtr->blue;
		colorPtr->flags = entryPtr->flags;
	    }
	    return 1;
	}
    }
    return 0;
}

void
BrowserFreeColorList(
    X11WasmResource *resourcePtr)
{
    X11WasmColorEntry *entryPtr;

    if (resourcePtr == NULL) {
	return;
    }
    entryPtr = resourcePtr->colorList;
    while (entryPtr != NULL) {
	X11WasmColorEntry *nextPtr = entryPtr->nextPtr;

	ckfree(entryPtr);
	entryPtr = nextPtr;
    }
    resourcePtr->colorList = NULL;
}

static X11WasmResource *
BrowserWindowToplevel(
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResource(window);
    X11WasmResource *topLevelPtr = resourcePtr;

    while ((resourcePtr != NULL) && (resourcePtr->parentPtr != NULL)
	    && ((Window)resourcePtr->parentPtr->id != x11wasm_browserDisplay.rootWindow)) {
	resourcePtr = resourcePtr->parentPtr;
	if (resourcePtr->type == X11_WASM_RESOURCE_WINDOW) {
	    topLevelPtr = resourcePtr;
	}
    }
    return topLevelPtr;
}

int
BrowserWindowIsRootToplevel(
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResource(window);
    X11WasmResource *targetPtr;
    Window target;

    if (resourcePtr == NULL) {
	return 0;
    }
    if (resourcePtr->metadataRootToplevelSet) {
	return resourcePtr->metadataIsRootToplevel ? 1 : 0;
    }

    target = BrowserWindowMetadataTarget(window);
    targetPtr = (target != window) ? BrowserWindowResource(target) : NULL;
    if ((targetPtr != NULL) && targetPtr->metadataRootToplevelSet) {
	return targetPtr->metadataIsRootToplevel ? 1 : 0;
    }
    return resourcePtr->parent == x11wasm_browserDisplay.rootWindow;
}

Window
BrowserWindowMetadataTarget(
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResource(window);
    X11WasmResource *topLevelPtr = BrowserWindowToplevel(window);

    if ((resourcePtr != NULL) && (resourcePtr->metadataTarget != None)) {
	return resourcePtr->metadataTarget;
    }
    return ((topLevelPtr != NULL) && ((Window)topLevelPtr->id != None))
	    ? (Window)topLevelPtr->id : window;
}

Window
BrowserWindowStateSource(
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResource(window);
    X11WasmResource *candidatePtr;
    Atom netWmStateAtom;
    Window fallback = window;

    if (resourcePtr == NULL) {
	return window;
    }
    netWmStateAtom = BrowserNetWmStateAtom();
    if (BrowserFindProperty(resourcePtr, netWmStateAtom) != NULL) {
	return window;
    }
    for (candidatePtr = x11wasm_browserDisplay.resourceList; candidatePtr != NULL;
	    candidatePtr = candidatePtr->nextPtr) {
	Window candidate;

	if ((candidatePtr->type != X11_WASM_RESOURCE_WINDOW)
		|| (candidatePtr == resourcePtr)) {
	    continue;
	}
	candidate = (Window)candidatePtr->id;
	if (BrowserWindowMetadataTarget(candidate) != window) {
	    continue;
	}
	if (candidatePtr->metadataTarget == window) {
	    fallback = candidate;
	}
	if (BrowserFindProperty(candidatePtr, netWmStateAtom) != NULL) {
	    return candidate;
	}
    }
    return fallback;
}

static Atom
BrowserWindowMetadataTargetAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "_X11_WASM_METADATA_TARGET",
	    False);
}

static Atom
BrowserWindowMetadataRootToplevelAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL),
	    "_X11_WASM_METADATA_ROOT_TOPLEVEL", False);
}

static void
BrowserReapplyWindowMetadataProperties(
    Window window,
    X11WasmResource *resourcePtr)
{
    X11WasmProperty *propertyPtr;
    Atom targetAtom = BrowserWindowMetadataTargetAtom();
    Atom rootToplevelAtom = BrowserWindowMetadataRootToplevelAtom();

    if (resourcePtr == NULL) {
	return;
    }
    for (propertyPtr = resourcePtr->propertyList; propertyPtr != NULL;
	    propertyPtr = propertyPtr->nextPtr) {
	if ((propertyPtr->atom == targetAtom)
		|| (propertyPtr->atom == rootToplevelAtom)) {
	    continue;
	}
	BrowserApplyWindowMetadataProperty(window, propertyPtr->atom,
		propertyPtr);
    }
}

static void
BrowserClearWindowMetadataTarget(
    Window target,
    int updatesDocument)
{
    if ((target == None) || (BrowserWindowResource(target) == NULL)) {
	return;
    }
    (void)X11WasmJsClearWindowMetadata(BrowserJsXId(target),
	    updatesDocument);
}

static void
BrowserRebuildWindowMetadataTarget(
    Window target,
    int updatesDocument)
{
    X11WasmResource *candidatePtr;

    if ((target == None) || (BrowserWindowResource(target) == NULL)) {
	return;
    }
    BrowserClearWindowMetadataTarget(target, updatesDocument);
    for (candidatePtr = x11wasm_browserDisplay.resourceList; candidatePtr != NULL;
	    candidatePtr = candidatePtr->nextPtr) {
	if ((candidatePtr->type != X11_WASM_RESOURCE_WINDOW)
		|| (BrowserWindowMetadataTarget((Window)candidatePtr->id)
		    != target)) {
	    continue;
	}
	BrowserReapplyWindowMetadataProperties((Window)candidatePtr->id,
		candidatePtr);
    }
}

void
BrowserForgetWindowMetadataSource(
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResource(window);
    Window target;
    int updatesDocument;

    if (resourcePtr == NULL) {
	return;
    }
    target = BrowserWindowMetadataTarget(window);
    if ((target == None) || (target == window)
	    || (BrowserWindowResource(target) == NULL)) {
	return;
    }
    updatesDocument = BrowserWindowIsRootToplevel(window);
    resourcePtr->metadataTarget = window;
    resourcePtr->metadataRootToplevelSet = 1;
    resourcePtr->metadataIsRootToplevel = 0;
    BrowserRebuildWindowMetadataTarget(target, updatesDocument);
}

static int
BrowserApplyWindowMetadataRoutingProperty(
    Window window,
    Atom property,
    X11WasmProperty *propertyPtr)
{
    X11WasmResource *resourcePtr;
    Atom targetAtom = BrowserWindowMetadataTargetAtom();
    Atom rootToplevelAtom = BrowserWindowMetadataRootToplevelAtom();
    Window oldTarget;
    Window newTarget;
    int oldUpdatesDocument;
    int newUpdatesDocument;

    if ((property != targetAtom) && (property != rootToplevelAtom)) {
	return 0;
    }
    resourcePtr = BrowserWindowResource(window);
    if (resourcePtr == NULL) {
	return 1;
    }
    oldTarget = BrowserWindowMetadataTarget(window);
    oldUpdatesDocument = BrowserWindowIsRootToplevel(window);
    if (property == targetAtom) {
	Window target = None;

	if ((propertyPtr != NULL) && (propertyPtr->format == 32)
		&& (propertyPtr->nitems > 0) && (propertyPtr->data != NULL)
		&& ((propertyPtr->type == XA_WINDOW)
			|| (propertyPtr->type == targetAtom))) {
	    target = (Window)BrowserPropertyAtomAt(propertyPtr->data, 0);
	    if ((target != None) && (BrowserWindowResource(target) == NULL)) {
		target = None;
	    }
	}
	resourcePtr->metadataTarget = target;
    } else {
	int isRootToplevel = 0;
	int rootToplevelSet = 0;

	if ((propertyPtr != NULL) && (propertyPtr->type == XA_CARDINAL)
		&& (propertyPtr->format == 32) && (propertyPtr->nitems > 0)
		&& (propertyPtr->data != NULL)) {
	    isRootToplevel = BrowserPropertyAtomAt(propertyPtr->data, 0)
		    ? 1 : 0;
	    rootToplevelSet = 1;
	}
	resourcePtr->metadataRootToplevelSet = rootToplevelSet;
	resourcePtr->metadataIsRootToplevel = isRootToplevel;
    }
    newTarget = BrowserWindowMetadataTarget(window);
    newUpdatesDocument = BrowserWindowIsRootToplevel(window);
    if ((oldTarget != None) && ((oldTarget != newTarget)
	    || (oldUpdatesDocument != newUpdatesDocument))) {
	BrowserRebuildWindowMetadataTarget(oldTarget, oldUpdatesDocument);
	if (newTarget != oldTarget) {
	    BrowserReapplyWindowMetadataProperties(window, resourcePtr);
	}
    } else {
	BrowserReapplyWindowMetadataProperties(window, resourcePtr);
    }
    return 1;
}

int
X11WasmSetWindowMetadataTarget(
    Display *display,
    Window window,
    Window target,
    int isRootToplevel)
{
    X11WasmResource *resourcePtr;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_CHANGE_PROPERTY);
    if (resourcePtr == NULL) {
	return 0;
    }
    if ((target != None) && (BrowserWindowResourceOrError(&display, target,
	    X11_WASM_XREQ_CHANGE_PROPERTY) == NULL)) {
	return 0;
    }
    resourcePtr->metadataTarget = target;
    resourcePtr->metadataRootToplevelSet = 1;
    resourcePtr->metadataIsRootToplevel = isRootToplevel ? 1 : 0;
    BrowserReapplyWindowMetadataProperties(window, resourcePtr);
    return 1;
}

void
BrowserSetWindowTitle(
    Window window,
    const char *title,
    int updatesDocument)
{
    Window titleWindow = BrowserWindowMetadataTarget(window);

    (void)X11WasmJsSetWindowTitle(BrowserJsXId(titleWindow),
	    (title != NULL) ? title : "", (title != NULL) ? (int)strlen(title) : 0,
	    updatesDocument);
}

void
BrowserSetWindowIconName(
    Window window,
    const char *iconName)
{
    Window iconWindow = BrowserWindowMetadataTarget(window);

    (void)X11WasmJsSetWindowIconName(BrowserJsXId(iconWindow),
	    (iconName != NULL) ? iconName : "",
	    (iconName != NULL) ? (int)strlen(iconName) : 0);
}

void
BrowserSetWindowTransientFor(
    Window window,
    Window transientFor)
{
    Window targetWindow = BrowserWindowMetadataTarget(window);
    Window transientWindow = (transientFor != None)
	    ? BrowserWindowMetadataTarget(transientFor) : None;

    (void)X11WasmJsSetWindowTransientFor(BrowserJsXId(targetWindow),
	    BrowserJsXId(transientWindow));
    BrowserSyncTransientStacking();
}

void
BrowserSetWindowCommand(
    Window window,
    char **argv,
    int argc)
{
    X11WasmDString command;
    Window commandWindow = BrowserWindowMetadataTarget(window);
    int i;

    X11WasmDStringInit(&command);
    for (i = 0; i < argc; i++) {
	X11WasmDStringAppendElement(&command, (argv[i] != NULL) ? argv[i] : "");
    }
    (void)X11WasmJsSetWindowCommand(BrowserJsXId(commandWindow),
	    X11WasmDStringValue(&command), X11WasmDStringLength(&command));
    X11WasmDStringFree(&command);
}

void
BrowserSetWindowClientMachine(
    Window window,
    const char *clientMachine,
    int clientMachineLength)
{
    Window clientWindow = BrowserWindowMetadataTarget(window);

    (void)X11WasmJsSetWindowClientMachine(BrowserJsXId(clientWindow),
	    (clientMachine != NULL) ? clientMachine : "",
	    (clientMachine != NULL) ? clientMachineLength : 0);
}

void
BrowserSetWindowClassHint(
    Window window,
    const char *resName,
    const char *resClass)
{
    Window classWindow = BrowserWindowMetadataTarget(window);

    (void)X11WasmJsSetWindowClassHint(BrowserJsXId(classWindow),
	    (resName != NULL) ? resName : "",
	    (resName != NULL) ? (int)strlen(resName) : 0,
	    (resClass != NULL) ? resClass : "",
	    (resClass != NULL) ? (int)strlen(resClass) : 0);
}

Atom
BrowserWindowOpacityAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "_NET_WM_WINDOW_OPACITY",
	    True);
}

Atom
BrowserUtf8StringAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "UTF8_STRING", False);
}

Atom
BrowserTextAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "TEXT", False);
}

Atom
BrowserCompoundTextAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "COMPOUND_TEXT", False);
}

Atom
BrowserMultipleAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "MULTIPLE", False);
}

Atom
BrowserAtomPairAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "ATOM_PAIR", False);
}

Atom
BrowserTargetsAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "TARGETS", False);
}

Atom
BrowserLengthAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "LENGTH", False);
}

Atom
BrowserTimestampAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "TIMESTAMP", False);
}

Atom
BrowserClipboardSelectionAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "CLIPBOARD", False);
}

Atom
BrowserNetWmIconAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "_NET_WM_ICON", True);
}

static Atom
BrowserNetWmNameAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "_NET_WM_NAME", True);
}

static Atom
BrowserNetWmIconNameAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "_NET_WM_ICON_NAME", True);
}

static Atom
BrowserNetWmWindowTypeAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "_NET_WM_WINDOW_TYPE", True);
}

Atom
BrowserNetWmStateAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "_NET_WM_STATE", False);
}

Atom
BrowserWmProtocolsAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "WM_PROTOCOLS", False);
}

Atom
BrowserWmColormapWindowsAtom(void)
{
    return XInternAtom(X11WasmGetDisplay(NULL), "WM_COLORMAP_WINDOWS",
	    False);
}

int
BrowserPropertyIsTextEncoding(
    Atom type)
{
    return (type == XA_STRING) || (type == BrowserUtf8StringAtom())
	    || (type == BrowserCompoundTextAtom());
}

void
BrowserSetWindowAlpha(
    Window window,
    double alpha)
{
    Window alphaWindow = BrowserWindowMetadataTarget(window);

    if (alpha < 0.0) {
	alpha = 0.0;
    } else if (alpha > 1.0) {
	alpha = 1.0;
    }
    (void)X11WasmJsSetWindowAlpha(BrowserJsXId(alphaWindow), alpha);
}

void
BrowserApplyWindowOpacityProperty(
    Window window,
    Atom property,
    Atom type,
    int format,
    _Xconst unsigned char *data,
    int nelements)
{
    Atom opacityAtom = BrowserWindowOpacityAtom();
    unsigned int opacity = 0xffffffffU;

    if ((opacityAtom == None) || (property != opacityAtom)) {
	return;
    }
    if ((type != XA_CARDINAL) || (format != 32) || (nelements <= 0)
	    || (data == NULL)) {
	BrowserSetWindowAlpha(window, 1.0);
	return;
    }
    memcpy(&opacity, data, sizeof(opacity));
    BrowserSetWindowAlpha(window, (double)opacity / 4294967295.0);
}

void
BrowserSetWindowSizeHints(
    Window window,
    XSizeHints *hints)
{
    Window hintsWindow = BrowserWindowMetadataTarget(window);

    if (hints == NULL) {
	(void)X11WasmJsSetWindowSizeHints(BrowserJsXId(hintsWindow),
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	return;
    }
    (void)X11WasmJsSetWindowSizeHints(BrowserJsXId(hintsWindow),
	    (int)hints->flags, hints->x, hints->y, hints->width, hints->height,
	    hints->min_width, hints->min_height, hints->max_width,
	    hints->max_height, hints->width_inc, hints->height_inc,
	    hints->min_aspect.x, hints->min_aspect.y, hints->max_aspect.x,
	    hints->max_aspect.y, hints->base_width, hints->base_height,
	    hints->win_gravity);
}

void
BrowserSetWindowZoomHints(
    Window window,
    XSizeHints *hints)
{
    Window hintsWindow = BrowserWindowMetadataTarget(window);

    if (hints == NULL) {
	(void)X11WasmJsSetWindowZoomHints(BrowserJsXId(hintsWindow),
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	return;
    }
    (void)X11WasmJsSetWindowZoomHints(BrowserJsXId(hintsWindow),
	    (int)hints->flags, hints->x, hints->y, hints->width, hints->height,
	    hints->min_width, hints->min_height, hints->max_width,
	    hints->max_height, hints->width_inc, hints->height_inc,
	    hints->min_aspect.x, hints->min_aspect.y, hints->max_aspect.x,
	    hints->max_aspect.y, hints->base_width, hints->base_height,
	    hints->win_gravity);
}

void
BrowserSetWindowWMHints(
    Window window,
    XWMHints *hints)
{
    Window hintsWindow = BrowserWindowMetadataTarget(window);
    Window iconWindow = None;
    Window windowGroup = None;

    if (hints == NULL) {
	(void)X11WasmJsSetWindowWMHints(BrowserJsXId(hintsWindow),
		0, 0, 0, 0, 0, 0, 0, 0, 0);
	return;
    }
    if (hints->icon_window != None) {
	iconWindow = BrowserWindowMetadataTarget(hints->icon_window);
    }
    if (hints->window_group != None) {
	windowGroup = BrowserWindowMetadataTarget(hints->window_group);
    }
    (void)X11WasmJsSetWindowWMHints(BrowserJsXId(hintsWindow),
	    (int)hints->flags, hints->input, hints->initial_state,
	    BrowserJsXId(hints->icon_pixmap), BrowserJsXId(iconWindow),
	    hints->icon_x, hints->icon_y, BrowserJsXId(hints->icon_mask),
	    BrowserJsXId(windowGroup));
}

void
BrowserSetWindowIconSizes(
    Window window,
    const XIconSize *sizeList,
    int count)
{
    X11WasmDString sizes;
    Window sizesWindow = BrowserWindowMetadataTarget(window);
    int i;

    if ((sizeList == NULL) || (count <= 0)) {
	(void)X11WasmJsSetWindowIconSizes(BrowserJsXId(sizesWindow),
		NULL, 0, 0);
	return;
    }
    X11WasmDStringInit(&sizes);
    for (i = 0; i < count; i++) {
	char buffer[128];

	snprintf(buffer, sizeof(buffer), "%dx%d-%dx%d+%d+%d",
		sizeList[i].min_width, sizeList[i].min_height,
		sizeList[i].max_width, sizeList[i].max_height,
		sizeList[i].width_inc, sizeList[i].height_inc);
	X11WasmDStringAppendElement(&sizes, buffer);
    }
    (void)X11WasmJsSetWindowIconSizes(BrowserJsXId(sizesWindow),
	    X11WasmDStringValue(&sizes), X11WasmDStringLength(&sizes), count);
    X11WasmDStringFree(&sizes);
}

void
BrowserSetWindowProtocols(
    Window window,
    const unsigned char *data,
    unsigned long atomCount)
{
    X11WasmDString protocols;
    Display *display = X11WasmGetDisplay(NULL);
    Window protocolsWindow = BrowserWindowMetadataTarget(window);
    unsigned long i;

    X11WasmDStringInit(&protocols);
    for (i = 0; (data != NULL) && (i < atomCount); i++) {
	Atom protocol = BrowserPropertyAtomAt(data, i);
	char *name;

	if (!BrowserAtomIsValid(protocol)) {
	    continue;
	}
	name = XGetAtomName(display, protocol);
	if ((name != NULL) && (name[0] != '\0')) {
	    X11WasmDStringAppendElement(&protocols, name);
	}
	if (name != NULL) {
	    XFree(name);
	}
    }
    (void)X11WasmJsSetWindowProtocols(BrowserJsXId(protocolsWindow),
	    X11WasmDStringValue(&protocols), X11WasmDStringLength(&protocols));
    X11WasmDStringFree(&protocols);
}

void
BrowserApplyWindowProtocolsProperty(
    Window window,
    X11WasmProperty *propertyPtr)
{
    if ((propertyPtr == NULL) || (propertyPtr->type != XA_ATOM)
	    || (propertyPtr->format != 32)) {
	BrowserSetWindowProtocols(window, NULL, 0);
	return;
    }
    BrowserSetWindowProtocols(window, propertyPtr->data,
	    propertyPtr->nitems);
}

static void
BrowserSetWindowTypes(
    Window window,
    const unsigned char *data,
    unsigned long atomCount)
{
    X11WasmDString types;
    Display *display = X11WasmGetDisplay(NULL);
    Window typesWindow = BrowserWindowMetadataTarget(window);
    unsigned long i;

    X11WasmDStringInit(&types);
    for (i = 0; (data != NULL) && (i < atomCount); i++) {
	Atom typeAtom = BrowserPropertyAtomAt(data, i);
	char *name;

	if (!BrowserAtomIsValid(typeAtom)) {
	    continue;
	}
	name = XGetAtomName(display, typeAtom);
	if ((name != NULL) && (name[0] != '\0')) {
	    X11WasmDStringAppendElement(&types, name);
	}
	if (name != NULL) {
	    XFree(name);
	}
    }
    (void)X11WasmJsSetWindowTypes(BrowserJsXId(typesWindow),
	    X11WasmDStringValue(&types), X11WasmDStringLength(&types));
    X11WasmDStringFree(&types);
}

static void
BrowserApplyWindowTypesProperty(
    Window window,
    X11WasmProperty *propertyPtr)
{
    if ((propertyPtr == NULL) || (propertyPtr->type != XA_ATOM)
	    || (propertyPtr->format != 32)) {
	BrowserSetWindowTypes(window, NULL, 0);
	return;
    }
    BrowserSetWindowTypes(window, propertyPtr->data, propertyPtr->nitems);
}

int
BrowserResourceSupportsProtocol(
    X11WasmResource *resourcePtr,
    Atom protocolAtom)
{
    X11WasmProperty *propertyPtr;
    unsigned long i;

    if ((resourcePtr == NULL) || (protocolAtom == None)) {
	return 0;
    }
    propertyPtr = BrowserFindProperty(resourcePtr, BrowserWmProtocolsAtom());
    if ((propertyPtr == NULL) || (propertyPtr->type != XA_ATOM)
	    || (propertyPtr->format != 32)) {
	return 0;
    }
    for (i = 0; i < propertyPtr->nitems; i++) {
	if (BrowserPropertyAtomAt(propertyPtr->data, i) == protocolAtom) {
	    return 1;
	}
    }
    return 0;
}

int
BrowserWindowSupportsProtocol(
    X11WasmResource *resourcePtr,
    Atom protocolAtom)
{
    X11WasmResource *candidatePtr;
    Window targetWindow;

    if ((resourcePtr == NULL) || (protocolAtom == None)) {
	return 0;
    }
    if (BrowserResourceSupportsProtocol(resourcePtr, protocolAtom)) {
	return 1;
    }

    targetWindow = BrowserWindowMetadataTarget((Window)resourcePtr->id);
    if (targetWindow != (Window)resourcePtr->id) {
	candidatePtr = X11WasmFindResource(targetWindow);
	if (BrowserResourceSupportsProtocol(candidatePtr, protocolAtom)) {
	    return 1;
	}
    }
    for (candidatePtr = x11wasm_browserDisplay.resourceList; candidatePtr != NULL;
	    candidatePtr = candidatePtr->nextPtr) {
	if ((candidatePtr == resourcePtr)
		|| (candidatePtr->type != X11_WASM_RESOURCE_WINDOW)
		|| (BrowserWindowMetadataTarget((Window)candidatePtr->id)
		    != targetWindow)) {
	    continue;
	}
	if (BrowserResourceSupportsProtocol(candidatePtr, protocolAtom)) {
	    return 1;
	}
    }
    return 0;
}

void
BrowserSetWindowColormapWindows(
    Window window,
    const unsigned char *data,
    unsigned long windowCount)
{
    X11WasmDString colormapWindows;
    Window targetWindow = BrowserWindowMetadataTarget(window);
    unsigned long i;

    X11WasmDStringInit(&colormapWindows);
    for (i = 0; (data != NULL) && (i < windowCount); i++) {
	Window colormapWindow = (Window)BrowserPropertyAtomAt(data, i);
	char buffer[X11_WASM_INTEGER_SPACE];

	snprintf(buffer, sizeof(buffer), "%u",
		(unsigned int)BrowserJsXId(colormapWindow));
	X11WasmDStringAppendElement(&colormapWindows, buffer);
    }
    (void)X11WasmJsSetWindowColormapWindows(BrowserJsXId(targetWindow),
	    X11WasmDStringValue(&colormapWindows),
	    X11WasmDStringLength(&colormapWindows));
    X11WasmDStringFree(&colormapWindows);
}

void
BrowserApplyWindowColormapWindowsProperty(
    Window window,
    X11WasmProperty *propertyPtr)
{
    if ((propertyPtr == NULL) || (propertyPtr->type != XA_WINDOW)
	    || (propertyPtr->format != 32)) {
	BrowserSetWindowColormapWindows(window, NULL, 0);
	return;
    }
    BrowserSetWindowColormapWindows(window, propertyPtr->data,
	    propertyPtr->nitems);
}

unsigned int
BrowserStandardColormapXId(
    XID xid)
{
    if ((xid == None) || (xid == ReleaseByFreeingColormap)) {
	return (unsigned int)xid;
    }
    return BrowserJsXId(xid);
}

void
BrowserSetWindowStandardColormaps(
    Window window,
    Atom property,
    const unsigned char *data,
    int count)
{
    Display *display = X11WasmGetDisplay(NULL);
    Window targetWindow = BrowserWindowMetadataTarget(window);
    X11WasmDString maps;
    char *propertyName;
    int i;

    propertyName = XGetAtomName(display, property);
    if (propertyName == NULL) {
	return;
    }
    if ((data == NULL) || (count <= 0)) {
	(void)X11WasmJsSetWindowStandardColormaps(
		BrowserJsXId(targetWindow), propertyName,
		(int)strlen(propertyName), NULL, 0, 0);
	XFree(propertyName);
	return;
    }

    X11WasmDStringInit(&maps);
    X11WasmDStringAppend(&maps, "[", 1);
    for (i = 0; i < count; i++) {
	XStandardColormap cmap;
	char buffer[512];

	memcpy(&cmap, data + ((size_t)i * sizeof(cmap)), sizeof(cmap));
	snprintf(buffer, sizeof(buffer),
		"%s{\"colormap\":%u,\"redMax\":%lu,\"redMult\":%lu,"
		"\"greenMax\":%lu,\"greenMult\":%lu,\"blueMax\":%lu,"
		"\"blueMult\":%lu,\"basePixel\":%lu,\"visualid\":%lu,"
		"\"killid\":%u}",
		(i > 0) ? "," : "",
		BrowserStandardColormapXId(cmap.colormap),
		cmap.red_max, cmap.red_mult, cmap.green_max, cmap.green_mult,
		cmap.blue_max, cmap.blue_mult, cmap.base_pixel,
		(unsigned long)cmap.visualid,
		BrowserStandardColormapXId(cmap.killid));
	X11WasmDStringAppend(&maps, buffer, -1);
    }
    X11WasmDStringAppend(&maps, "]", 1);
    (void)X11WasmJsSetWindowStandardColormaps(BrowserJsXId(targetWindow),
	    propertyName, (int)strlen(propertyName), X11WasmDStringValue(&maps),
	    X11WasmDStringLength(&maps), count);
    X11WasmDStringFree(&maps);
    XFree(propertyName);
}

void
BrowserApplyWindowStandardColormapProperty(
    Window window,
    Atom property,
    X11WasmProperty *propertyPtr)
{
    if ((propertyPtr == NULL) || (propertyPtr->type != XA_RGB_COLOR_MAP)
	    || (propertyPtr->format != 32)
	    || (propertyPtr->dataLength < (int)sizeof(XStandardColormap))
	    || ((propertyPtr->dataLength % (int)sizeof(XStandardColormap))
		!= 0)) {
	BrowserSetWindowStandardColormaps(window, property, NULL, 0);
	return;
    }
    BrowserSetWindowStandardColormaps(window, property, propertyPtr->data,
	    propertyPtr->dataLength / (int)sizeof(XStandardColormap));
}

void
BrowserSetWindowState(
    Window window,
    int topmost,
    int zoomed,
    int fullscreen)
{
    Window stateWindow = BrowserWindowMetadataTarget(window);

    (void)X11WasmJsSetWindowState(BrowserJsXId(stateWindow), topmost,
	    zoomed, fullscreen);
}

void
BrowserSetWindowWmState(
    Window window,
    int state)
{
    X11WasmResource *resourcePtr = BrowserWindowResource(window);
    Atom property;
    X11WasmProperty *propertyPtr;
    long values[2] = {state, None};

    if (resourcePtr == NULL) {
	return;
    }
    property = XInternAtom(resourcePtr->display, "WM_STATE", False);
    propertyPtr = BrowserFindProperty(resourcePtr, property);
    if ((propertyPtr != NULL) && (propertyPtr->type == property)
	    && (propertyPtr->format == 32) && (propertyPtr->nitems == 2)
	    && (BrowserPropertyAtomAt(propertyPtr->data, 0) == (Atom)state)
	    && (BrowserPropertyAtomAt(propertyPtr->data, 1) == None)) {
	return;
    }
    (void)XChangeProperty(resourcePtr->display, window, property, property,
	    32, PropModeReplace, (unsigned char *)values, 2);
}

int
BrowserInitialWmStateForMap(
    Display *display,
    Window window)
{
    XWMHints *hints = XGetWMHints(display, window);
    int state = NormalState;

    if (hints != NULL) {
	if (hints->flags & StateHint) {
	    switch (hints->initial_state) {
	    case NormalState:
	    case IconicState:
	    case WithdrawnState:
		state = hints->initial_state;
		break;
	    default:
		break;
	    }
	}
	XFree(hints);
    }
    return state;
}

void
BrowserApplyWindowStateProperty(
    Window window,
    X11WasmProperty *propertyPtr)
{
    Display *display = X11WasmGetDisplay(NULL);
    Atom aboveAtom = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    Atom maxVertAtom = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT",
	    False);
    Atom maxHorzAtom = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ",
	    False);
    Atom fullscreenAtom = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN",
	    False);
    int topmost = 0;
    int maxVert = 0;
    int maxHorz = 0;
    int fullscreen = 0;
    unsigned long i;

    if ((propertyPtr == NULL) || (propertyPtr->type != XA_ATOM)
	    || (propertyPtr->format != 32)) {
	BrowserSetWindowState(window, 0, 0, 0);
	return;
    }
    for (i = 0; i < propertyPtr->nitems; i++) {
	Atom atom = BrowserPropertyAtomAt(propertyPtr->data, i);

	if (atom == aboveAtom) {
	    topmost = 1;
	} else if (atom == maxVertAtom) {
	    maxVert = 1;
	} else if (atom == maxHorzAtom) {
	    maxHorz = 1;
	} else if (atom == fullscreenAtom) {
	    fullscreen = 1;
	}
    }
    BrowserSetWindowState(window, topmost, maxVert && maxHorz, fullscreen);
}

int
BrowserWindowHasStateAtom(
    X11WasmProperty *propertyPtr,
    Atom stateAtom)
{
    unsigned long i;

    if ((propertyPtr == NULL) || (propertyPtr->type != XA_ATOM)
	    || (propertyPtr->format != 32)) {
	return 0;
    }
    for (i = 0; i < propertyPtr->nitems; i++) {
	if (BrowserPropertyAtomAt(propertyPtr->data, i) == stateAtom) {
	    return 1;
	}
    }
    return 0;
}

void
BrowserApplyNetWmStateAtom(
    Window window,
    X11WasmResource *resourcePtr,
    Atom netWmStateAtom,
    long action,
    Atom stateAtom)
{
    X11WasmProperty *propertyPtr;
    int hasState;
    int shouldHaveState;

    if (stateAtom == None) {
	return;
    }
    propertyPtr = BrowserFindProperty(resourcePtr, netWmStateAtom);
    hasState = BrowserWindowHasStateAtom(propertyPtr, stateAtom);
    shouldHaveState = hasState;
    if (action == 0L) {
	shouldHaveState = 0;
    } else if (action == 1L) {
	shouldHaveState = 1;
    } else if (action == 2L) {
	shouldHaveState = !hasState;
    }
    if (shouldHaveState == hasState) {
	return;
    }
    if (shouldHaveState) {
	(void)XChangeProperty(&x11wasm_browserDisplay.display, window,
		netWmStateAtom, XA_ATOM, 32, PropModeAppend,
		(const unsigned char *)&stateAtom, 1);
    } else {
	unsigned long count = (propertyPtr != NULL) ? propertyPtr->nitems : 0;
	Atom *atoms = NULL;
	unsigned long i;
	int outCount = 0;

	if (count > 0) {
	    atoms = (Atom *)ckalloc((size_t)count * sizeof(Atom));
	}
	for (i = 0; i < count; i++) {
	    Atom atom = BrowserPropertyAtomAt(propertyPtr->data, i);

	    if (atom != stateAtom) {
		atoms[outCount++] = atom;
	    }
	}
	if (outCount > 0) {
	    (void)XChangeProperty(&x11wasm_browserDisplay.display, window,
		    netWmStateAtom, XA_ATOM, 32, PropModeReplace,
		    (const unsigned char *)atoms, outCount);
	} else {
	    (void)XDeleteProperty(&x11wasm_browserDisplay.display, window,
		    netWmStateAtom);
	}
	if (atoms != NULL) {
	    ckfree(atoms);
	}
    }
}

void
BrowserApplyNetWmStateClientMessage(
    XClientMessageEvent *eventPtr)
{
    X11WasmResource *resourcePtr;
    Atom netWmStateAtom = BrowserNetWmStateAtom();
    Atom stateAtoms[2];
    long action;
    int i;

    if ((eventPtr == NULL) || (eventPtr->type != ClientMessage)
	    || (eventPtr->message_type != netWmStateAtom)
	    || (eventPtr->format != 32) || (eventPtr->window == None)) {
	return;
    }
    resourcePtr = X11WasmFindResource(eventPtr->window);
    if (resourcePtr == NULL) {
	return;
    }
    action = eventPtr->data.l[0];
    stateAtoms[0] = (Atom)eventPtr->data.l[1];
    stateAtoms[1] = (Atom)eventPtr->data.l[2];
    for (i = 0; i < 2; i++) {
	if ((i > 0) && (stateAtoms[i] == stateAtoms[0])) {
	    continue;
	}
	BrowserApplyNetWmStateAtom(eventPtr->window, resourcePtr,
		netWmStateAtom, action, stateAtoms[i]);
    }
}

unsigned int
BrowserIconPropertyCardinal(
    const unsigned char *data,
    int index)
{
    unsigned int value = 0;

    memcpy(&value, data + ((size_t)index * 4U), sizeof(value));
    return value;
}

int
BrowserIconDimensionMatches(
    unsigned int value,
    int minValue,
    int maxValue,
    int increment)
{
    if ((minValue > 0) && (value < (unsigned int)minValue)) {
	return 0;
    }
    if ((maxValue > 0) && (value > (unsigned int)maxValue)) {
	return 0;
    }
    if ((increment > 1) && (minValue > 0)
	    && (((int)value - minValue) % increment != 0)) {
	return 0;
    }
    return 1;
}

int
BrowserIconMatchesSizeHint(
    unsigned int width,
    unsigned int height,
    const XIconSize *hintPtr,
    unsigned long long *penaltyPtr)
{
    unsigned int targetWidth;
    unsigned int targetHeight;
    unsigned long long widthDelta;
    unsigned long long heightDelta;

    if (hintPtr == NULL) {
	return 0;
    }
    if (!BrowserIconDimensionMatches(width, hintPtr->min_width,
	    hintPtr->max_width, hintPtr->width_inc)
	    || !BrowserIconDimensionMatches(height, hintPtr->min_height,
		    hintPtr->max_height, hintPtr->height_inc)) {
	return 0;
    }

    targetWidth = (hintPtr->max_width > 0)
	    ? (unsigned int)hintPtr->max_width : width;
    targetHeight = (hintPtr->max_height > 0)
	    ? (unsigned int)hintPtr->max_height : height;
    widthDelta = (width > targetWidth)
	    ? (unsigned long long)(width - targetWidth)
	    : (unsigned long long)(targetWidth - width);
    heightDelta = (height > targetHeight)
	    ? (unsigned long long)(height - targetHeight)
	    : (unsigned long long)(targetHeight - height);
    if (penaltyPtr != NULL) {
	*penaltyPtr = widthDelta + heightDelta;
    }
    return 1;
}

int
BrowserIconMatchesAnySizeHint(
    unsigned int width,
    unsigned int height,
    const XIconSize *hints,
    int hintCount,
    unsigned long long *penaltyPtr)
{
    unsigned long long bestPenalty = ULLONG_MAX;
    int i;
    int matched = 0;

    for (i = 0; i < hintCount; i++) {
	unsigned long long penalty = 0;

	if (!BrowserIconMatchesSizeHint(width, height, &hints[i], &penalty)) {
	    continue;
	}
	if (!matched || (penalty < bestPenalty)) {
	    bestPenalty = penalty;
	}
	matched = 1;
    }
    if (matched && (penaltyPtr != NULL)) {
	*penaltyPtr = bestPenalty;
    }
    return matched;
}

void
BrowserSetWindowIconPhoto(
    Window window,
    int imageCount,
    const char *sizes,
    int sizesLength,
    const unsigned char *rgba,
    int rgbaLength,
    int width,
    int height)
{
    Window iconWindow = BrowserWindowMetadataTarget(window);

    (void)X11WasmJsSetWindowIconPhoto(BrowserJsXId(iconWindow), imageCount,
	    (sizes != NULL) ? sizes : "", (sizes != NULL) ? sizesLength : 0,
	    rgba, rgbaLength, width, height);
}

void
BrowserApplyWindowIconProperty(
    Window window,
    Atom property,
    Atom type,
    int format,
    _Xconst unsigned char *data,
    int nelements)
{
    Atom iconAtom = BrowserNetWmIconAtom();
    X11WasmDString sizes;
    int imageCount = 0;
    int index = 0;
    int bestIndex = -1;
    int bestWidth = 0;
    int bestHeight = 0;
    unsigned long long bestArea = 0;
    int bestMatchesIconSize = 0;
    unsigned long long bestSizePenalty = ULLONG_MAX;
    unsigned char *rgba = NULL;
    int rgbaLength = 0;
    XIconSize *iconSizes = NULL;
    int iconSizeCount = 0;

    if ((iconAtom == None) || (property != iconAtom)) {
	return;
    }
    if ((type != XA_CARDINAL) || (format != 32) || (nelements <= 0)
	    || (data == NULL)) {
	BrowserSetWindowIconPhoto(window, 0, "", 0, NULL, 0, 0, 0);
	return;
    }

    X11WasmDStringInit(&sizes);
    (void)XGetIconSizes(X11WasmGetDisplay(NULL),
	    x11wasm_browserDisplay.rootWindow, &iconSizes, &iconSizeCount);
    while (index + 2 <= nelements) {
	unsigned int width = BrowserIconPropertyCardinal(data, index++);
	unsigned int height = BrowserIconPropertyCardinal(data, index++);
	unsigned long long pixelCount = (unsigned long long)width * height;
	unsigned long long sizePenalty = ULLONG_MAX;
	int matchesIconSize;
	char sizeBuffer[64];
	int pixelIndex = index;

	if ((width == 0) || (height == 0)
		|| (pixelCount > (unsigned long long)(nelements - index))) {
	    break;
	}
	if (imageCount > 0) {
	    X11WasmDStringAppend(&sizes, " ", 1);
	}
	snprintf(sizeBuffer, sizeof(sizeBuffer), "%ux%u", width, height);
	X11WasmDStringAppend(&sizes, sizeBuffer, -1);
	matchesIconSize = BrowserIconMatchesAnySizeHint(width, height,
		iconSizes, iconSizeCount, &sizePenalty);
	if ((matchesIconSize && (!bestMatchesIconSize
		|| (sizePenalty < bestSizePenalty)
		|| ((sizePenalty == bestSizePenalty)
			&& (pixelCount > bestArea))))
		|| (!matchesIconSize && !bestMatchesIconSize
			&& (pixelCount > bestArea))) {
	    bestIndex = pixelIndex;
	    bestWidth = (int)width;
	    bestHeight = (int)height;
	    bestArea = pixelCount;
	    bestMatchesIconSize = matchesIconSize;
	    bestSizePenalty = sizePenalty;
	}
	index += (int)pixelCount;
	imageCount++;
    }
    if ((bestIndex >= 0) && (bestArea <= (unsigned long long)INT_MAX / 4ULL)) {
	unsigned long long i;

	rgbaLength = (int)(bestArea * 4ULL);
	rgba = (unsigned char *)ckalloc((unsigned int)rgbaLength);
	for (i = 0; i < bestArea; i++) {
	    unsigned int argb = BrowserIconPropertyCardinal(data,
		    bestIndex + (int)i);
	    unsigned char *pixelPtr = rgba + (i * 4ULL);

	    pixelPtr[0] = (unsigned char)((argb >> 16) & 0xffU);
	    pixelPtr[1] = (unsigned char)((argb >> 8) & 0xffU);
	    pixelPtr[2] = (unsigned char)(argb & 0xffU);
	    pixelPtr[3] = (unsigned char)((argb >> 24) & 0xffU);
	}
    }
    BrowserSetWindowIconPhoto(window, imageCount, X11WasmDStringValue(&sizes),
	    X11WasmDStringLength(&sizes), rgba, rgbaLength, bestWidth, bestHeight);
    if (rgba != NULL) {
	ckfree(rgba);
    }
    if (iconSizes != NULL) {
	XFree(iconSizes);
    }
    X11WasmDStringFree(&sizes);
}

void
BrowserSetWindowCommandData(
    Window window,
    const unsigned char *data,
    int length)
{
    X11WasmDString command;
    Window commandWindow = BrowserWindowMetadataTarget(window);
    int offset = 0;

    X11WasmDStringInit(&command);
    while ((data != NULL) && (offset < length)) {
	const char *arg = (const char *)data + offset;
	int argLength = 0;

	while ((offset + argLength < length) && (data[offset + argLength] != '\0')) {
	    argLength++;
	}
	X11WasmDStringAppendElement(&command, arg);
	offset += argLength + 1;
    }
    (void)X11WasmJsSetWindowCommand(BrowserJsXId(commandWindow),
	    X11WasmDStringValue(&command), X11WasmDStringLength(&command));
    X11WasmDStringFree(&command);
}

void
BrowserSetWindowClassData(
    Window window,
    const unsigned char *data,
    int length)
{
    const char *resName = "";
    const char *resClass = "";
    int nameLength = 0;

    if ((data != NULL) && (length > 0)) {
	resName = (const char *)data;
	while ((nameLength < length) && (data[nameLength] != '\0')) {
	    nameLength++;
	}
	if (nameLength + 1 < length) {
	    resClass = (const char *)data + nameLength + 1;
	}
    }
    BrowserSetWindowClassHint(window, resName, resClass);
}

static const char *
BrowserWindowTextPropertyString(
    X11WasmProperty *propertyPtr,
    int requireUtf8)
{
    if ((propertyPtr == NULL) || (propertyPtr->format != 8)
	    || (propertyPtr->data == NULL)) {
	return NULL;
    }
    if (requireUtf8) {
	if (propertyPtr->type != BrowserUtf8StringAtom()) {
	    return NULL;
	}
    } else if (!BrowserPropertyIsTextEncoding(propertyPtr->type)) {
	return NULL;
    }
    return (const char *)propertyPtr->data;
}

static void
BrowserApplyWindowTitleMetadata(
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResource(window);
    const char *title = NULL;
    Atom netWmNameAtom = BrowserNetWmNameAtom();

    if (resourcePtr == NULL) {
	return;
    }
    if (netWmNameAtom != None) {
	title = BrowserWindowTextPropertyString(
		BrowserFindProperty(resourcePtr, netWmNameAtom), 1);
    }
    if (title == NULL) {
	title = BrowserWindowTextPropertyString(
		BrowserFindProperty(resourcePtr, XA_WM_NAME), 0);
    }
    BrowserSetWindowTitle(window, (title != NULL) ? title : "",
	    BrowserWindowIsRootToplevel(window));
}

static void
BrowserApplyWindowIconNameMetadata(
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResource(window);
    const char *iconName = NULL;
    Atom netWmIconNameAtom = BrowserNetWmIconNameAtom();

    if (resourcePtr == NULL) {
	return;
    }
    if (netWmIconNameAtom != None) {
	iconName = BrowserWindowTextPropertyString(
		BrowserFindProperty(resourcePtr, netWmIconNameAtom), 1);
    }
    if (iconName == NULL) {
	iconName = BrowserWindowTextPropertyString(
		BrowserFindProperty(resourcePtr, XA_WM_ICON_NAME), 0);
    }
    BrowserSetWindowIconName(window, (iconName != NULL) ? iconName : "");
}

void
BrowserApplyWindowMetadataProperty(
    Window window,
    Atom property,
    X11WasmProperty *propertyPtr)
{
    int type = (propertyPtr != NULL) ? propertyPtr->type : None;
    int format = (propertyPtr != NULL) ? propertyPtr->format : 0;
    int nitems = (propertyPtr != NULL) ? (int)propertyPtr->nitems : 0;
    const unsigned char *data = (propertyPtr != NULL) ? propertyPtr->data : NULL;

    if (BrowserApplyWindowMetadataRoutingProperty(window, property,
	    propertyPtr)) {
	return;
    }
    if ((property == XA_WM_NAME) || (property == BrowserNetWmNameAtom())) {
	BrowserApplyWindowTitleMetadata(window);
    } else if ((property == XA_WM_ICON_NAME)
	    || (property == BrowserNetWmIconNameAtom())) {
	BrowserApplyWindowIconNameMetadata(window);
    } else if (property == XA_WM_TRANSIENT_FOR) {
	Window transientFor = None;

	if ((format == 32) && (nitems > 0) && (data != NULL)) {
	    transientFor = (Window)BrowserPropertyAtomAt(data, 0);
	}
	BrowserSetWindowTransientFor(window, transientFor);
    } else if (property == XA_WM_COMMAND) {
	if ((type == XA_STRING) && (format == 8) && (data != NULL)) {
	    BrowserSetWindowCommandData(window, data, propertyPtr->dataLength);
	} else {
	    BrowserSetWindowCommandData(window, NULL, 0);
	}
    } else if (property == XA_WM_CLASS) {
	if ((type == XA_STRING) && (format == 8) && (data != NULL)) {
	    BrowserSetWindowClassData(window, data, propertyPtr->dataLength);
	} else {
	    BrowserSetWindowClassHint(window, "", "");
	}
    } else if (property == XA_WM_CLIENT_MACHINE) {
	if (BrowserPropertyIsTextEncoding((Atom)type) && (format == 8)
		&& (data != NULL)) {
	    BrowserSetWindowClientMachine(window, (const char *)data,
		    propertyPtr->dataLength);
	} else {
	    BrowserSetWindowClientMachine(window, NULL, 0);
	}
    } else if (property == XA_WM_NORMAL_HINTS) {
	XSizeHints hints;

	if ((type == XA_WM_SIZE_HINTS) && (format == 32)
		&& (propertyPtr != NULL)
		&& BrowserDecodeSizeHintsProperty(data, propertyPtr->nitems,
			&hints, NULL)) {
	    BrowserSetWindowSizeHints(window, &hints);
	} else {
	    BrowserSetWindowSizeHints(window, NULL);
	}
    } else if (property == XA_WM_ZOOM_HINTS) {
	XSizeHints hints;

	if ((type == XA_WM_SIZE_HINTS) && (format == 32)
		&& (propertyPtr != NULL)
		&& BrowserDecodeSizeHintsProperty(data, propertyPtr->nitems,
			&hints, NULL)) {
	    BrowserSetWindowZoomHints(window, &hints);
	} else {
	    BrowserSetWindowZoomHints(window, NULL);
	}
    } else if (property == XA_WM_HINTS) {
	XWMHints hints;

	if ((format == 32) && (propertyPtr != NULL)
		&& (propertyPtr->dataLength >= (int)sizeof(hints))) {
	    memcpy(&hints, data, sizeof(hints));
	    BrowserSetWindowWMHints(window, &hints);
	} else {
	    BrowserSetWindowWMHints(window, NULL);
	}
    } else if (property == XA_WM_ICON_SIZE) {
	if ((format == 32) && (propertyPtr != NULL)
		&& (propertyPtr->dataLength >= (int)sizeof(XIconSize))
		&& ((propertyPtr->dataLength % (int)sizeof(XIconSize)) == 0)) {
	    BrowserSetWindowIconSizes(window, (const XIconSize *)data,
		    propertyPtr->dataLength / (int)sizeof(XIconSize));
	} else {
	    BrowserSetWindowIconSizes(window, NULL, 0);
	}
    } else if (property == BrowserWmProtocolsAtom()) {
	BrowserApplyWindowProtocolsProperty(window, propertyPtr);
    } else if (property == BrowserNetWmWindowTypeAtom()) {
	BrowserApplyWindowTypesProperty(window, propertyPtr);
    } else if (property == BrowserWmColormapWindowsAtom()) {
	BrowserApplyWindowColormapWindowsProperty(window, propertyPtr);
    } else if (property == BrowserNetWmStateAtom()) {
	BrowserApplyWindowStateProperty(window, propertyPtr);
    } else if (property == XInternAtom(X11WasmGetDisplay(NULL), "WM_STATE", False)) {
	int state = WithdrawnState;

	if ((propertyPtr != NULL) && (type == (int)property) && (format == 32)
		&& (nitems == 2)) {
	    state = (int)BrowserPropertyAtomAt(data, 0);
	}
	(void)X11WasmJsSetWindowWmState(
		BrowserJsXId(BrowserWindowMetadataTarget(window)), state);
    } else if (property == BrowserWindowOpacityAtom()) {
	BrowserApplyWindowOpacityProperty(window, property, type, format, data,
		nitems);
    } else if (property == BrowserNetWmIconAtom()) {
	BrowserApplyWindowIconProperty(window, property, type, format, data,
		nitems);
    }
    BrowserApplyWindowStandardColormapProperty(window, property, propertyPtr);
}

void
BrowserClearExternalClipboardText(void)
{
    if (x11wasm_externalClipboardText != NULL) {
	ckfree(x11wasm_externalClipboardText);
	x11wasm_externalClipboardText = NULL;
    }
    x11wasm_externalClipboardAvailable = 0;
    x11wasm_externalClipboardTime = CurrentTime;
}

void
BrowserSetExternalClipboardText(
    const char *utf8,
    int utf8Length)
{
    char *copy;

    if (utf8 == NULL) {
	utf8 = "";
	utf8Length = 0;
    } else if (utf8Length < 0) {
	utf8Length = (int)strlen(utf8);
    }
    copy = (char *)ckalloc((size_t)utf8Length + 1U);
    if (utf8Length > 0) {
	memcpy(copy, utf8, (size_t)utf8Length);
    }
    copy[utf8Length] = '\0';
    BrowserClearExternalClipboardText();
    x11wasm_externalClipboardText = copy;
    x11wasm_externalClipboardAvailable = 1;
    x11wasm_externalClipboardTime = x11wasm_eventTime;
}
