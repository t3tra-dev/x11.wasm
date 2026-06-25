#include "XlibInt.h"

void
BrowserSyncSelectionOwner(
    Atom selection,
    Window owner,
    Time time)
{
    char *selectionName = XGetAtomName(&browserDisplay.display, selection);

    if (selectionName == NULL) {
	return;
    }
    (void)X11WasmJsSetSelectionOwner(selectionName,
	    (int)strlen(selectionName), owner != None ? BrowserJsXId(owner) : 0,
	    (unsigned int)BrowserSelectionEventTime(time));
    XFree(selectionName);
}

X11WasmSelectionOwner *
BrowserFindSelectionOwner(
    Atom selection)
{
    X11WasmSelectionOwner *ownerPtr;

    for (ownerPtr = selectionOwnerList; ownerPtr != NULL;
	    ownerPtr = ownerPtr->nextPtr) {
	if (ownerPtr->selection == selection) {
	    return ownerPtr;
	}
    }
    return NULL;
}

X11WasmSelectionOwner *
BrowserCreateSelectionOwner(
    Atom selection)
{
    X11WasmSelectionOwner *ownerPtr = BrowserFindSelectionOwner(selection);

    if (ownerPtr != NULL) {
	return ownerPtr;
    }
    ownerPtr = (X11WasmSelectionOwner *)ckalloc(
	    sizeof(X11WasmSelectionOwner));
    memset(ownerPtr, 0, sizeof(X11WasmSelectionOwner));
    ownerPtr->selection = selection;
    ownerPtr->nextPtr = selectionOwnerList;
    selectionOwnerList = ownerPtr;
    return ownerPtr;
}

void
BrowserDeleteSelectionOwner(
    Atom selection)
{
    X11WasmSelectionOwner **linkPtr = &selectionOwnerList;

    while (*linkPtr != NULL) {
	X11WasmSelectionOwner *ownerPtr = *linkPtr;

	if (ownerPtr->selection == selection) {
	    *linkPtr = ownerPtr->nextPtr;
	    ckfree(ownerPtr);
	    return;
	}
	linkPtr = &ownerPtr->nextPtr;
    }
}

void
BrowserDeleteSelectionOwnersForWindow(
    Window window)
{
    X11WasmSelectionOwner **linkPtr = &selectionOwnerList;

    while (*linkPtr != NULL) {
	X11WasmSelectionOwner *ownerPtr = *linkPtr;

	if (ownerPtr->owner == window) {
	    BrowserFailQueuedSelectionRequestsForOwner(window);
	    BrowserSyncSelectionOwner(ownerPtr->selection, None, eventTime);
	    *linkPtr = ownerPtr->nextPtr;
	    ckfree(ownerPtr);
	    continue;
	}
	linkPtr = &ownerPtr->nextPtr;
    }
}

Window
BrowserSelectionOwnerWindow(
    Atom selection)
{
    X11WasmSelectionOwner *ownerPtr = BrowserFindSelectionOwner(selection);

    if ((ownerPtr == NULL) || (ownerPtr->owner == None)) {
	return None;
    }
    if (BrowserWindowResource(ownerPtr->owner) == NULL) {
	BrowserSyncSelectionOwner(selection, None, eventTime);
	BrowserDeleteSelectionOwner(selection);
	return None;
    }
    return ownerPtr->owner;
}

Time
BrowserSelectionEventTime(
    Time time)
{
    return (time == CurrentTime) ? eventTime : time;
}

int
BrowserQueueSelectionClear(
    Window owner,
    Atom selection,
    Time time,
    unsigned long serial)
{
    XEvent event;

    if ((owner == None) || (BrowserWindowResource(owner) == NULL)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, SelectionClear, owner, time);
    event.xselectionclear.serial = serial;
    event.xselectionclear.window = owner;
    event.xselectionclear.selection = selection;
    event.xselectionclear.time = BrowserSelectionEventTime(time);
    return BrowserQueueEvent(&event);
}

int
BrowserQueueSelectionNotify(
    Window requestor,
    Atom selection,
    Atom target,
    Atom property,
    Time time)
{
    XEvent event;

    BrowserSetCommonEventFields(&event, SelectionNotify, requestor, time);
    event.xselection.requestor = requestor;
    event.xselection.selection = selection;
    event.xselection.target = target;
    event.xselection.property = property;
    event.xselection.time = BrowserSelectionEventTime(time);
    return BrowserQueueEvent(&event);
}

int
BrowserSelectionTargetIsText(
    Atom target)
{
    return (target == XA_STRING) || (target == BrowserUtf8StringAtom())
	    || (target == BrowserTextAtom())
	    || (target == BrowserCompoundTextAtom());
}

void
BrowserSyncClipboardProperty(
    Atom property,
    const X11WasmProperty *propertyPtr)
{
    XTextProperty textProp;
    char **list = NULL;
    int count = 0;
    const char *text = "";

    if ((property != BrowserClipboardSelectionAtom()) || (propertyPtr == NULL)
	    || (BrowserSelectionOwnerWindow(BrowserClipboardSelectionAtom())
		== None)
	    || (propertyPtr->format != 8)
	    || !BrowserPropertyIsTextEncoding(propertyPtr->type)) {
	return;
    }
    memset(&textProp, 0, sizeof(textProp));
    textProp.value = propertyPtr->data;
    textProp.encoding = propertyPtr->type;
    textProp.format = propertyPtr->format;
    textProp.nitems = propertyPtr->nitems;
    if (!BrowserTextPropertyToUtf8StringList(&textProp, &list, &count)) {
	return;
    }
    if ((count > 0) && (list != NULL) && (list[0] != NULL)) {
	text = list[0];
    }
    (void)X11WasmJsSetClipboardText(text, (int)strlen(text));
    if (list != NULL) {
	XFreeStringList(list);
    }
}

int
BrowserExternalClipboardTextProperty(
    Atom target,
    XTextProperty *textPropPtr)
{
    char *list[1];
    XICCEncodingStyle style;
    int result;

    if (textPropPtr == NULL) {
	return XNoMemory;
    }
    memset(textPropPtr, 0, sizeof(*textPropPtr));
    if (externalClipboardText == NULL) {
	return XConverterNotFound;
    }
    if (target == XA_STRING) {
	style = XStringStyle;
    } else if (target == BrowserUtf8StringAtom()) {
	style = XUTF8StringStyle;
    } else if (target == BrowserTextAtom()) {
	style = XStdICCTextStyle;
    } else if (target == BrowserCompoundTextAtom()) {
	style = XCompoundTextStyle;
    } else {
	return XConverterNotFound;
    }

    list[0] = externalClipboardText;
    result = BrowserTextListToTextProperty(&browserDisplay.display, list, 1,
	    style, textPropPtr);
    if ((result == XConverterNotFound) && (target == XA_STRING)) {
	result = BrowserTextListToTextProperty(&browserDisplay.display, list, 1,
		XUTF8StringStyle, textPropPtr);
    }
    return result;
}

int
BrowserConvertExternalClipboardSelection(
    Window requestor,
    Atom selection,
    Atom target,
    Atom property,
    Time time)
{
    unsigned int targets[6];
    Atom effectiveProperty = property;
    Time timestamp = externalClipboardTime;

    if ((selection != BrowserClipboardSelectionAtom())
	    || !externalClipboardAvailable) {
	return 0;
    }
    if (effectiveProperty == None) {
	effectiveProperty = target;
    }
    if ((effectiveProperty == None)
	    || (BrowserWindowResource(requestor) == NULL)) {
	return 0;
    }
    if (target == BrowserTargetsAtom()) {
	targets[0] = (unsigned int)XA_STRING;
	targets[1] = (unsigned int)BrowserUtf8StringAtom();
	targets[2] = (unsigned int)BrowserTextAtom();
	targets[3] = (unsigned int)BrowserCompoundTextAtom();
	targets[4] = (unsigned int)BrowserTargetsAtom();
	targets[5] = (unsigned int)BrowserTimestampAtom();
	(void)XChangeProperty(&browserDisplay.display, requestor,
		effectiveProperty, XA_ATOM, 32, PropModeReplace,
		(const unsigned char *)targets,
		(int)(sizeof(targets) / sizeof(targets[0])));
	(void)BrowserQueueSelectionNotify(requestor, selection, target,
		effectiveProperty, time);
	return 1;
    }
    if (target == BrowserTimestampAtom()) {
	unsigned int timestampValue;

	if (timestamp == CurrentTime) {
	    timestamp = BrowserSelectionEventTime(time);
	}
	timestampValue = (unsigned int)timestamp;
	(void)XChangeProperty(&browserDisplay.display, requestor,
		effectiveProperty, XA_INTEGER, 32, PropModeReplace,
		(const unsigned char *)&timestampValue, 1);
	(void)BrowserQueueSelectionNotify(requestor, selection, target,
		effectiveProperty, time);
	return 1;
    }
    if (!BrowserSelectionTargetIsText(target)) {
	return 0;
    }
    {
	XTextProperty textProp;
	int result;

	result = BrowserExternalClipboardTextProperty(target, &textProp);
	if ((result != Success) || (textProp.nitems > (unsigned long)INT_MAX)) {
	    if (textProp.value != NULL) {
		XFree(textProp.value);
	    }
	    return 0;
	}
	(void)XChangeProperty(&browserDisplay.display, requestor,
		effectiveProperty, textProp.encoding, textProp.format,
		PropModeReplace, (const unsigned char *)textProp.value,
		(int)textProp.nitems);
	if (textProp.value != NULL) {
	    XFree(textProp.value);
	}
    }
    (void)BrowserQueueSelectionNotify(requestor, selection, target,
	    effectiveProperty, time);
    return 1;
}

void
BrowserFailQueuedSelectionRequestsForOwner(
    Window owner)
{
    int offset;

    if (owner == None) {
	return;
    }
    for (offset = 0; offset < eventQueueCount; offset++) {
	int index = (eventQueueHead + offset) % X11_WASM_EVENT_QUEUE_LIMIT;
	XSelectionRequestEvent request;
	XEvent notify;

	if ((eventQueue[index].event.type != SelectionRequest)
		|| (eventQueue[index].event.xselectionrequest.owner != owner)) {
	    continue;
	}
	request = eventQueue[index].event.xselectionrequest;
	BrowserSetCommonEventFields(&notify, SelectionNotify, request.requestor,
		request.time);
	notify.xselection.requestor = request.requestor;
	notify.xselection.selection = request.selection;
	notify.xselection.target = request.target;
	notify.xselection.property = None;
	notify.xselection.time = BrowserSelectionEventTime(request.time);
	eventQueue[index].event = notify;
    }
    BrowserUpdateQueueLength();
}

int
BrowserQueueSelectionRequest(
    Window owner,
    Window requestor,
    Atom selection,
    Atom target,
    Atom property,
    Time time)
{
    XEvent event;

    if ((owner == None) || (BrowserWindowResource(owner) == NULL)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, SelectionRequest, owner, time);
    event.xselectionrequest.owner = owner;
    event.xselectionrequest.requestor = requestor;
    event.xselectionrequest.selection = selection;
    event.xselectionrequest.target = target;
    event.xselectionrequest.property = property;
    event.xselectionrequest.time = BrowserSelectionEventTime(time);
    return BrowserQueueEvent(&event);
}

int
BrowserPopEvent(
    XEvent *eventPtr,
    int *internalPtr)
{
    BrowserInitDisplay();
    if (eventQueueCount <= 0) {
	if (eventPtr != NULL) {
	    memset(eventPtr, 0, sizeof(XEvent));
	}
	if (internalPtr != NULL) {
	    *internalPtr = 0;
	}
	return 0;
    }
    if (eventPtr != NULL) {
	*eventPtr = eventQueue[eventQueueHead].event;
    }
    if (internalPtr != NULL) {
	*internalPtr = eventQueue[eventQueueHead].internal;
    }
    eventQueueHead = (eventQueueHead + 1) % X11_WASM_EVENT_QUEUE_LIMIT;
    eventQueueCount--;
    BrowserUpdateQueueLength();
    return 1;
}

void
BrowserClearEventQueue(void)
{
    eventQueueHead = 0;
    eventQueueCount = 0;
    BrowserUpdateQueueLength();
}

Window
BrowserEventWindow(
    const XEvent *eventPtr)
{
    if (eventPtr == NULL) {
	return None;
    }
    switch (eventPtr->type) {
    case CreateNotify:
	return eventPtr->xcreatewindow.parent;
    case DestroyNotify:
	return eventPtr->xdestroywindow.event;
    case UnmapNotify:
	return eventPtr->xunmap.event;
    case MapNotify:
	return eventPtr->xmap.event;
    case ReparentNotify:
	return eventPtr->xreparent.event;
    case ConfigureNotify:
	return eventPtr->xconfigure.event;
    case GravityNotify:
	return eventPtr->xgravity.event;
    case CirculateNotify:
	return eventPtr->xcirculate.event;
    case MapRequest:
	return eventPtr->xmaprequest.parent;
    case ConfigureRequest:
	return eventPtr->xconfigurerequest.parent;
    case CirculateRequest:
	return eventPtr->xcirculaterequest.parent;
    case GraphicsExpose:
	return (Window)eventPtr->xgraphicsexpose.drawable;
    case NoExpose:
	return (Window)eventPtr->xnoexpose.drawable;
    case SelectionRequest:
	return eventPtr->xselectionrequest.owner;
    case SelectionNotify:
	return eventPtr->xselection.requestor;
    default:
	return eventPtr->xany.window;
    }
}

long
BrowserEventMask(
    const XEvent *eventPtr)
{
    unsigned int state;

    if (eventPtr == NULL) {
	return NoEventMask;
    }
    switch (eventPtr->type) {
    case KeyPress:
	return KeyPressMask;
    case KeyRelease:
	return KeyReleaseMask;
    case ButtonPress:
	return ButtonPressMask;
    case ButtonRelease:
	return ButtonReleaseMask;
    case MotionNotify:
	state = eventPtr->xmotion.state;
	if (state & (Button1Mask | Button2Mask | Button3Mask | Button4Mask
		| Button5Mask)) {
	    long mask = PointerMotionMask | ButtonMotionMask;

	    if (state & Button1Mask) mask |= Button1MotionMask;
	    if (state & Button2Mask) mask |= Button2MotionMask;
	    if (state & Button3Mask) mask |= Button3MotionMask;
	    if (state & Button4Mask) mask |= Button4MotionMask;
	    if (state & Button5Mask) mask |= Button5MotionMask;
	    return mask;
	}
	return PointerMotionMask;
    case EnterNotify:
	return EnterWindowMask;
    case LeaveNotify:
	return LeaveWindowMask;
    case FocusIn:
    case FocusOut:
	return FocusChangeMask;
    case X11_WASM_BROWSER_WHEEL_EVENT:
	return X11_WASM_BROWSER_WHEEL_MASK;
    case KeymapNotify:
	return KeymapStateMask;
    case Expose:
    case GraphicsExpose:
    case NoExpose:
	return ExposureMask;
    case VisibilityNotify:
	return VisibilityChangeMask;
    case DestroyNotify:
    case UnmapNotify:
    case MapNotify:
    case ReparentNotify:
    case ConfigureNotify:
    case GravityNotify:
    case CirculateNotify:
	return StructureNotifyMask;
    case CreateNotify:
	return SubstructureNotifyMask;
    case ResizeRequest:
	return ResizeRedirectMask;
    case MapRequest:
    case ConfigureRequest:
    case CirculateRequest:
	return SubstructureRedirectMask;
    case PropertyNotify:
	return PropertyChangeMask;
    case ColormapNotify:
	return ColormapChangeMask;
    default:
	return NoEventMask;
    }
}

int
BrowserPointerGrabAllowsEvent(
    const XEvent *eventPtr)
{
    long eventMask;

    if (!BrowserWindowCanReceiveEvents(browserDisplay.pointerGrabWindow)) {
	return 1;
    }
    switch (eventPtr != NULL ? eventPtr->type : 0) {
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
    case EnterNotify:
    case LeaveNotify:
	break;
    default:
	return 1;
    }
    eventMask = BrowserEventMask(eventPtr);
    return (eventMask == NoEventMask)
	    || ((browserDisplay.pointerGrabEventMask & eventMask) != 0);
}

int
BrowserEventMatchesWindowMask(
    const XEvent *eventPtr,
    Window window,
    long mask)
{
    long eventMask = BrowserEventMask(eventPtr);

    if (eventMask == NoEventMask) {
	return 0;
    }
    if ((mask & eventMask) == 0) {
	return 0;
    }
    if ((window != None) && (BrowserEventWindow(eventPtr) != window)) {
	return 0;
    }
    return 1;
}

int
BrowserSendEventMaskMatches(
    const XEvent *eventPtr,
    X11WasmResource *resourcePtr,
    long mask)
{
    long eventMask = BrowserEventMask(eventPtr);

    if (resourcePtr == NULL) {
	return 0;
    }
    if ((mask == NoEventMask) || (eventMask == NoEventMask)) {
	return 1;
    }
    if ((mask & eventMask) == 0) {
	return 0;
    }
    return BrowserResourceWantsEvent(resourcePtr, mask & eventMask);
}

Window
BrowserSendEventTarget(
    Window window,
    Bool propagate,
    long mask,
    const XEvent *eventPtr)
{
    long eventMask = BrowserEventMask(eventPtr);
    X11WasmResource *resourcePtr;

    BrowserInitDisplay();
    if (window == PointerWindow) {
	Window pointerWindow = browserDisplay.pointerWindow;

	if (!BrowserWindowCanReceiveEvents(pointerWindow)) {
	    pointerWindow = BrowserHitTestRoot(browserDisplay.pointerX,
		    browserDisplay.pointerY, NULL, NULL);
	}
	window = BrowserWindowCanReceiveEvents(pointerWindow)
		? pointerWindow : browserDisplay.rootWindow;
    } else if (window == InputFocus) {
	Window focusWindow = browserDisplay.focusWindow;

	window = BrowserWindowCanReceiveEvents(focusWindow)
		? focusWindow : browserDisplay.rootWindow;
    } else {
	window = BrowserResolveEventWindow(window);
    }
    resourcePtr = BrowserWindowResource(window);
    if (!propagate || (mask == NoEventMask) || (eventMask == NoEventMask)) {
	return BrowserSendEventMaskMatches(eventPtr, resourcePtr, mask)
		? window : None;
    }
    for (; resourcePtr != NULL; resourcePtr = BrowserWindowResource(
	    resourcePtr->parent)) {
	if (BrowserSendEventMaskMatches(eventPtr, resourcePtr, mask)) {
	    return (Window)resourcePtr->id;
	}
	if (resourcePtr->atts.do_not_propagate_mask & eventMask) {
	    return None;
	}
	if (resourcePtr->id == browserDisplay.rootWindow) {
	    break;
	}
    }
    return None;
}

int
BrowserPopMatchingEvent(
    Window window,
    long mask,
    XEvent *eventPtr)
{
    int offset;

    BrowserInitDisplay();
    for (offset = 0; offset < eventQueueCount; offset++) {
	int index = (eventQueueHead + offset) % X11_WASM_EVENT_QUEUE_LIMIT;

	if (!BrowserEventMatchesWindowMask(&eventQueue[index].event, window,
		mask)) {
	    continue;
	}
	if (eventPtr != NULL) {
	    *eventPtr = eventQueue[index].event;
	}
	while (offset < eventQueueCount - 1) {
	    int dest = (eventQueueHead + offset) % X11_WASM_EVENT_QUEUE_LIMIT;
	    int src = (eventQueueHead + offset + 1)
		    % X11_WASM_EVENT_QUEUE_LIMIT;

	    eventQueue[dest] = eventQueue[src];
	    offset++;
	}
	eventQueueCount--;
	BrowserUpdateQueueLength();
	return 1;
    }
    if (eventPtr != NULL) {
	memset(eventPtr, 0, sizeof(XEvent));
    }
    BrowserUpdateQueueLength();
    return 0;
}

void
BrowserWindowRootOffset(
    Window window,
    int *xPtr,
    int *yPtr)
{
    int x = 0;
    int y = 0;
    X11WasmResource *resourcePtr;

    for (resourcePtr = X11WasmFindResource(window);
	    resourcePtr != NULL && resourcePtr->id != browserDisplay.rootWindow;
	    resourcePtr = X11WasmFindResource(resourcePtr->parent)) {
	x += resourcePtr->x;
	y += resourcePtr->y;
    }
    if (xPtr != NULL) {
	*xPtr = x;
    }
    if (yPtr != NULL) {
	*yPtr = y;
    }
}

X11WasmResource *
BrowserWindowResource(
    Window window)
{
    X11WasmResource *resourcePtr = X11WasmFindResource(window);

    if ((resourcePtr != NULL)
	    && (resourcePtr->type == X11_WASM_RESOURCE_WINDOW)) {
	return resourcePtr;
    }
    return NULL;
}

void
BrowserUnlinkChild(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *parentPtr;

    if (resourcePtr == NULL) {
	return;
    }
    parentPtr = resourcePtr->parentPtr;
    if (parentPtr != NULL) {
	if (parentPtr->firstChildPtr == resourcePtr) {
	    parentPtr->firstChildPtr = resourcePtr->nextSiblingPtr;
	}
	if (parentPtr->lastChildPtr == resourcePtr) {
	    parentPtr->lastChildPtr = resourcePtr->prevSiblingPtr;
	}
    }
    if (resourcePtr->prevSiblingPtr != NULL) {
	resourcePtr->prevSiblingPtr->nextSiblingPtr =
		resourcePtr->nextSiblingPtr;
    }
    if (resourcePtr->nextSiblingPtr != NULL) {
	resourcePtr->nextSiblingPtr->prevSiblingPtr =
		resourcePtr->prevSiblingPtr;
    }
    resourcePtr->parentPtr = NULL;
    resourcePtr->prevSiblingPtr = NULL;
    resourcePtr->nextSiblingPtr = NULL;
}

void
BrowserLinkChildBefore(
    X11WasmResource *resourcePtr,
    X11WasmResource *parentPtr,
    X11WasmResource *siblingPtr)
{
    if ((resourcePtr == NULL) || (parentPtr == NULL)
	    || (resourcePtr == parentPtr) || (resourcePtr == siblingPtr)) {
	return;
    }
    BrowserUnlinkChild(resourcePtr);
    resourcePtr->parentPtr = parentPtr;
    resourcePtr->nextSiblingPtr = siblingPtr;
    if (siblingPtr != NULL) {
	resourcePtr->prevSiblingPtr = siblingPtr->prevSiblingPtr;
	siblingPtr->prevSiblingPtr = resourcePtr;
    } else {
	resourcePtr->prevSiblingPtr = parentPtr->lastChildPtr;
	parentPtr->lastChildPtr = resourcePtr;
    }
    if (resourcePtr->prevSiblingPtr != NULL) {
	resourcePtr->prevSiblingPtr->nextSiblingPtr = resourcePtr;
    } else {
	parentPtr->firstChildPtr = resourcePtr;
    }
    if (resourcePtr->nextSiblingPtr == NULL) {
	parentPtr->lastChildPtr = resourcePtr;
    }
}

void
BrowserLinkChildAfter(
    X11WasmResource *resourcePtr,
    X11WasmResource *parentPtr,
    X11WasmResource *siblingPtr)
{
    if ((resourcePtr == NULL) || (parentPtr == NULL)
	    || (resourcePtr == parentPtr) || (resourcePtr == siblingPtr)) {
	return;
    }
    BrowserUnlinkChild(resourcePtr);
    resourcePtr->parentPtr = parentPtr;
    resourcePtr->prevSiblingPtr = siblingPtr;
    if (siblingPtr != NULL) {
	resourcePtr->nextSiblingPtr = siblingPtr->nextSiblingPtr;
	siblingPtr->nextSiblingPtr = resourcePtr;
    } else {
	resourcePtr->nextSiblingPtr = parentPtr->firstChildPtr;
	parentPtr->firstChildPtr = resourcePtr;
    }
    if (resourcePtr->nextSiblingPtr != NULL) {
	resourcePtr->nextSiblingPtr->prevSiblingPtr = resourcePtr;
    } else {
	parentPtr->lastChildPtr = resourcePtr;
    }
    if (resourcePtr->prevSiblingPtr == NULL) {
	parentPtr->firstChildPtr = resourcePtr;
    }
}

void
BrowserLinkChildAtTop(
    X11WasmResource *resourcePtr,
    Window parent)
{
    X11WasmResource *parentPtr = BrowserWindowResource(parent);

    BrowserUnlinkChild(resourcePtr);
    if (parentPtr != NULL) {
	BrowserLinkChildBefore(resourcePtr, parentPtr, NULL);
    }
}

void
BrowserLinkChildAtBottom(
    X11WasmResource *resourcePtr,
    Window parent)
{
    X11WasmResource *parentPtr = BrowserWindowResource(parent);

    BrowserUnlinkChild(resourcePtr);
    if (parentPtr != NULL) {
	BrowserLinkChildAfter(resourcePtr, parentPtr, NULL);
    }
}

Window
BrowserWindowTransientFor(
    X11WasmResource *resourcePtr)
{
    X11WasmProperty *propertyPtr;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return None;
    }
    propertyPtr = BrowserFindProperty(resourcePtr, XA_WM_TRANSIENT_FOR);
    if ((propertyPtr == NULL) || (propertyPtr->format != 32)
	    || (propertyPtr->nitems == 0) || (propertyPtr->data == NULL)) {
	return None;
    }
    return (Window)BrowserPropertyAtomAt(propertyPtr->data, 0);
}

X11WasmResource *
BrowserStackingResource(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *currentPtr = resourcePtr;
    int guard = 0;

    while ((currentPtr != NULL) && (guard++ < 1024)) {
	X11WasmResource *parentPtr;

	if (currentPtr->parent == browserDisplay.rootWindow) {
	    break;
	}
	parentPtr = (currentPtr->parentPtr != NULL)
		? currentPtr->parentPtr : BrowserWindowResource(currentPtr->parent);
	if ((parentPtr == NULL) || (parentPtr->type != X11_WASM_RESOURCE_WINDOW)
		|| (parentPtr->id == browserDisplay.rootWindow)) {
	    break;
	}
	currentPtr = parentPtr;
    }
    return currentPtr;
}

int
BrowserTransientChainContains(
    X11WasmResource *resourcePtr,
    Window ancestor)
{
    Window transientFor;
    int guard = 0;

    if ((resourcePtr == NULL) || (ancestor == None)) {
	return 0;
    }
    transientFor = BrowserWindowTransientFor(resourcePtr);
    while ((transientFor != None) && (guard++ < 1024)) {
	X11WasmResource *parentPtr;

	if (transientFor == ancestor) {
	    return 1;
	}
	parentPtr = BrowserWindowResource(transientFor);
	if (parentPtr == NULL) {
	    return 0;
	}
	transientFor = BrowserWindowTransientFor(parentPtr);
    }
    return 0;
}

int
BrowserSiblingPrecedes(
    X11WasmResource *resourcePtr,
    X11WasmResource *siblingPtr)
{
    X11WasmResource *scanPtr;

    for (scanPtr = resourcePtr; scanPtr != NULL;
	    scanPtr = scanPtr->nextSiblingPtr) {
	if (scanPtr == siblingPtr) {
	    return 1;
	}
    }
    return 0;
}

static int
BrowserWindowRectanglesOverlap(
    X11WasmResource *firstPtr,
    X11WasmResource *secondPtr)
{
    long firstRight, firstBottom;
    long secondRight, secondBottom;

    if ((firstPtr == NULL) || (secondPtr == NULL)
	    || (firstPtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (secondPtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (firstPtr->parentPtr != secondPtr->parentPtr)
	    || !BrowserResourceIsVisible(firstPtr)
	    || !BrowserResourceIsVisible(secondPtr)
	    || (firstPtr->width == 0) || (firstPtr->height == 0)
	    || (secondPtr->width == 0) || (secondPtr->height == 0)) {
	return 0;
    }
    firstRight = (long)firstPtr->x + (long)firstPtr->width;
    firstBottom = (long)firstPtr->y + (long)firstPtr->height;
    secondRight = (long)secondPtr->x + (long)secondPtr->width;
    secondBottom = (long)secondPtr->y + (long)secondPtr->height;
    return ((long)firstPtr->x < secondRight)
	    && (firstRight > (long)secondPtr->x)
	    && ((long)firstPtr->y < secondBottom)
	    && (firstBottom > (long)secondPtr->y);
}

static int
BrowserWindowIsObscuredBySibling(
    X11WasmResource *resourcePtr,
    X11WasmResource *siblingPtr)
{
    X11WasmResource *scanPtr;

    if ((resourcePtr == NULL) || !BrowserResourceIsVisible(resourcePtr)) {
	return 0;
    }
    if (siblingPtr != NULL) {
	return BrowserSiblingPrecedes(resourcePtr, siblingPtr)
		&& BrowserWindowRectanglesOverlap(resourcePtr, siblingPtr);
    }
    for (scanPtr = resourcePtr->nextSiblingPtr; scanPtr != NULL;
	    scanPtr = scanPtr->nextSiblingPtr) {
	if (BrowserWindowRectanglesOverlap(resourcePtr, scanPtr)) {
	    return 1;
	}
    }
    return 0;
}

static int
BrowserWindowObscuresSibling(
    X11WasmResource *resourcePtr,
    X11WasmResource *siblingPtr)
{
    X11WasmResource *scanPtr;

    if ((resourcePtr == NULL) || !BrowserResourceIsVisible(resourcePtr)) {
	return 0;
    }
    if (siblingPtr != NULL) {
	return BrowserSiblingPrecedes(siblingPtr, resourcePtr)
		&& BrowserWindowRectanglesOverlap(resourcePtr, siblingPtr);
    }
    for (scanPtr = resourcePtr->prevSiblingPtr; scanPtr != NULL;
	    scanPtr = scanPtr->prevSiblingPtr) {
	if (BrowserWindowRectanglesOverlap(resourcePtr, scanPtr)) {
	    return 1;
	}
    }
    return 0;
}

void
BrowserSyncTransientStacking(void)
{
    X11WasmResource *resourcePtr;
    int resourceCount = 0;
    int pass;

    for (resourcePtr = browserDisplay.resourceList; resourcePtr != NULL;
	    resourcePtr = resourcePtr->nextPtr) {
	resourceCount++;
    }
    for (pass = 0; pass < resourceCount; pass++) {
	int changed = 0;

	for (resourcePtr = browserDisplay.resourceList; resourcePtr != NULL;
		resourcePtr = resourcePtr->nextPtr) {
	    Window transientFor = BrowserWindowTransientFor(resourcePtr);
	    X11WasmResource *parentPtr;
	    X11WasmResource *stackPtr;
	    X11WasmResource *parentStackPtr;
	    X11WasmResource *commonParentPtr;

	    if (transientFor == None) {
		continue;
	    }
	    parentPtr = BrowserWindowResource(transientFor);
	    if ((parentPtr == NULL) || (parentPtr == resourcePtr)
		    || BrowserTransientChainContains(parentPtr,
			    (Window)resourcePtr->id)) {
		continue;
	    }
	    stackPtr = BrowserStackingResource(resourcePtr);
	    parentStackPtr = BrowserStackingResource(parentPtr);
	    if ((stackPtr == NULL) || (parentStackPtr == NULL)
		    || (stackPtr == parentStackPtr)
		    || (stackPtr->parentPtr == NULL)
		    || (stackPtr->parentPtr != parentStackPtr->parentPtr)) {
		continue;
	    }
	    if (!BrowserSiblingPrecedes(stackPtr, parentStackPtr)) {
		continue;
	    }
	    commonParentPtr = stackPtr->parentPtr;
	    BrowserLinkChildAfter(stackPtr, commonParentPtr, parentStackPtr);
	    (void)X11WasmJsRestackWindow(BrowserJsXId(stackPtr->id),
		    BrowserJsXId(stackPtr->prevSiblingPtr != NULL
			    ? stackPtr->prevSiblingPtr->id : None));
	    changed = 1;
	}
	if (!changed) {
	    break;
	}
    }
}

void
BrowserRestackWindow(
    X11WasmResource *resourcePtr,
    unsigned int valueMask,
    XWindowChanges *changes)
{
    X11WasmResource *siblingPtr = NULL;
    X11WasmResource *parentPtr;
    int mode = Above;
    int restack = 1;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (resourcePtr->id == browserDisplay.rootWindow)) {
	return;
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if (parentPtr == NULL) {
	return;
    }
    if ((valueMask & CWStackMode) && (changes != NULL)) {
	mode = changes->stack_mode;
    }
    if ((valueMask & CWSibling) && (changes != NULL)) {
	siblingPtr = BrowserWindowResource(changes->sibling);
	if ((siblingPtr == resourcePtr)
		|| (siblingPtr == NULL)
		|| (siblingPtr->parentPtr != parentPtr)) {
	    siblingPtr = NULL;
	}
    }

    switch (mode) {
    case Below:
	if (siblingPtr != NULL) {
	    BrowserLinkChildBefore(resourcePtr, parentPtr, siblingPtr);
	} else {
	    BrowserLinkChildAtBottom(resourcePtr, (Window)parentPtr->id);
	}
	break;
    case BottomIf:
	if (BrowserWindowObscuresSibling(resourcePtr, siblingPtr)) {
	    if (siblingPtr != NULL) {
		BrowserLinkChildBefore(resourcePtr, parentPtr, siblingPtr);
	    } else {
		BrowserLinkChildAtBottom(resourcePtr, (Window)parentPtr->id);
	    }
	} else {
	    restack = 0;
	}
	break;
    case Opposite:
	if (BrowserWindowIsObscuredBySibling(resourcePtr, siblingPtr)) {
	    if (siblingPtr != NULL) {
		BrowserLinkChildAfter(resourcePtr, parentPtr, siblingPtr);
	    } else {
		BrowserLinkChildAtTop(resourcePtr, (Window)parentPtr->id);
	    }
	} else if (BrowserWindowObscuresSibling(resourcePtr, siblingPtr)) {
	    if (siblingPtr != NULL) {
		BrowserLinkChildBefore(resourcePtr, parentPtr, siblingPtr);
	    } else {
		BrowserLinkChildAtBottom(resourcePtr, (Window)parentPtr->id);
	    }
	} else {
	    restack = 0;
	}
	break;
    case Above:
	if (siblingPtr != NULL) {
	    BrowserLinkChildAfter(resourcePtr, parentPtr, siblingPtr);
	} else {
	    BrowserLinkChildAtTop(resourcePtr, (Window)parentPtr->id);
	}
	break;
    case TopIf:
	if (BrowserWindowIsObscuredBySibling(resourcePtr, siblingPtr)) {
	    if (siblingPtr != NULL) {
		BrowserLinkChildAfter(resourcePtr, parentPtr, siblingPtr);
	    } else {
		BrowserLinkChildAtTop(resourcePtr, (Window)parentPtr->id);
	    }
	} else {
	    restack = 0;
	}
	break;
    default:
	if (siblingPtr != NULL) {
	    BrowserLinkChildAfter(resourcePtr, parentPtr, siblingPtr);
	} else {
	    BrowserLinkChildAtTop(resourcePtr, (Window)parentPtr->id);
	}
	break;
    }
    if (!restack) {
	return;
    }
    (void)X11WasmJsRestackWindow(BrowserJsXId(resourcePtr->id),
	    BrowserJsXId(resourcePtr->prevSiblingPtr != NULL
		    ? resourcePtr->prevSiblingPtr->id : None));
    BrowserSyncTransientStacking();
}

int
BrowserResourceIsVisible(
    X11WasmResource *resourcePtr)
{
    while (resourcePtr != NULL) {
	if ((resourcePtr->type == X11_WASM_RESOURCE_WINDOW)
		&& !resourcePtr->mapped) {
	    return 0;
	}
	if (resourcePtr->id == browserDisplay.rootWindow) {
	    return 1;
	}
	resourcePtr = (resourcePtr->parentPtr != NULL)
		? resourcePtr->parentPtr
		: X11WasmFindResource(resourcePtr->parent);
    }
    return 0;
}

X11WasmResource *
BrowserFindTopVisibleChildAt(
    X11WasmResource *parentPtr,
    int x,
    int y)
{
    X11WasmResource *childPtr;

    if (parentPtr == NULL) {
	return NULL;
    }
    for (childPtr = parentPtr->lastChildPtr; childPtr != NULL;
	    childPtr = childPtr->prevSiblingPtr) {
	if ((childPtr->type != X11_WASM_RESOURCE_WINDOW)
		|| !BrowserResourceIsVisible(childPtr)) {
	    continue;
	}
	if ((x >= childPtr->x) && (y >= childPtr->y)
		&& (x < (childPtr->x + (int)childPtr->width))
		&& (y < (childPtr->y + (int)childPtr->height))) {
	    return childPtr;
	}
    }
    return NULL;
}

Window
BrowserHitTestRoot(
    int rootX,
    int rootY,
    int *xPtr,
    int *yPtr)
{
    X11WasmResource *resourcePtr;
    X11WasmResource *childPtr;
    int x = rootX;
    int y = rootY;

    BrowserInitDisplay();
    resourcePtr = &browserDisplay.rootResource;
    while ((childPtr = BrowserFindTopVisibleChildAt(resourcePtr, x, y))
	    != NULL) {
	x -= childPtr->x;
	y -= childPtr->y;
	resourcePtr = childPtr;
    }
    if (xPtr != NULL) {
	*xPtr = x;
    }
    if (yPtr != NULL) {
	*yPtr = y;
    }
    return (Window)resourcePtr->id;
}

void
BrowserWindowCoordinatesFromRoot(
    Window window,
    int rootX,
    int rootY,
    int *xPtr,
    int *yPtr)
{
    int windowRootX = 0;
    int windowRootY = 0;

    BrowserWindowRootOffset(window, &windowRootX, &windowRootY);
    if (xPtr != NULL) {
	*xPtr = rootX - windowRootX;
    }
    if (yPtr != NULL) {
	*yPtr = rootY - windowRootY;
    }
}

void
BrowserClampPointerToGrabConfine(
    int *rootXPtr,
    int *rootYPtr)
{
    X11WasmResource *resourcePtr;
    int left = 0;
    int top = 0;
    long right;
    long bottom;
    long rootX;
    long rootY;

    if ((rootXPtr == NULL) || (rootYPtr == NULL)
	    || (browserDisplay.pointerGrabConfineWindow == None)) {
	return;
    }
    resourcePtr = BrowserWindowResource(browserDisplay.pointerGrabConfineWindow);
    if ((resourcePtr == NULL) || !BrowserResourceIsVisible(resourcePtr)) {
	return;
    }
    BrowserWindowRootOffset((Window)resourcePtr->id, &left, &top);
    right = (long)left + ((resourcePtr->width > 0)
	    ? (long)resourcePtr->width - 1L : 0L);
    bottom = (long)top + ((resourcePtr->height > 0)
	    ? (long)resourcePtr->height - 1L : 0L);
    rootX = *rootXPtr;
    rootY = *rootYPtr;
    if (rootX < left) {
	rootX = left;
    } else if (rootX > right) {
	rootX = right;
    }
    if (rootY < top) {
	rootY = top;
    } else if (rootY > bottom) {
	rootY = bottom;
    }
    *rootXPtr = (int)rootX;
    *rootYPtr = (int)rootY;
}

void
BrowserTranslateWindowCoordinates(
    Window source,
    Window destination,
    int sourceX,
    int sourceY,
    int *destXPtr,
    int *destYPtr)
{
    int sourceRootX = 0;
    int sourceRootY = 0;
    int destinationRootX = 0;
    int destinationRootY = 0;

    BrowserWindowRootOffset(source, &sourceRootX, &sourceRootY);
    BrowserWindowRootOffset(destination, &destinationRootX, &destinationRootY);
    if (destXPtr != NULL) {
	*destXPtr = sourceX + sourceRootX - destinationRootX;
    }
    if (destYPtr != NULL) {
	*destYPtr = sourceY + sourceRootY - destinationRootY;
    }
}

int
BrowserWindowInTree(
    Window window,
    Window ancestor)
{
    X11WasmResource *resourcePtr;

    if ((window == None) || (ancestor == None)) {
	return 0;
    }
    if (window == ancestor) {
	return 1;
    }
    for (resourcePtr = X11WasmFindResource(window);
	    resourcePtr != NULL && resourcePtr->id != browserDisplay.rootWindow;
	    resourcePtr = X11WasmFindResource(resourcePtr->parent)) {
	if (resourcePtr->parent == ancestor) {
	    return 1;
	}
    }
    return 0;
}

Window
BrowserResolvePointerEventWindow(
    Window window,
    int *xPtr,
    int *yPtr)
{
    int rootX = (xPtr != NULL) ? *xPtr : 0;
    int rootY = (yPtr != NULL) ? *yPtr : 0;
    Window target;
    Window rawTarget;
    Window grabWindow;
    int grabOwnerEvents = 1;

    (void)window;
    BrowserInitDisplay();
    rawTarget = BrowserHitTestRoot(rootX, rootY, NULL, NULL);

    target = rawTarget;
    if (BrowserWindowCanReceiveEvents(browserDisplay.pointerCaptureWindow)) {
	target = browserDisplay.pointerCaptureWindow;
    }
    grabWindow = browserDisplay.pointerGrabWindow;
    if (BrowserWindowCanReceiveEvents(grabWindow)) {
	grabOwnerEvents = browserDisplay.pointerGrabOwnerEvents;
    }

    if (BrowserWindowCanReceiveEvents(grabWindow)) {
	if (!grabOwnerEvents || !BrowserWindowInTree(rawTarget, grabWindow)) {
	    target = grabWindow;
	}
    } else if (!BrowserWindowCanReceiveEvents(target)) {
	target = BrowserResolveEventWindow(target);
    }
    if (!BrowserWindowCanReceiveEvents(target)) {
	target = BrowserResolveEventWindow(target);
    }

    BrowserWindowCoordinatesFromRoot(target, rootX, rootY, xPtr, yPtr);
    return target;
}

void
BrowserUpdatePointerState(
    Window window,
    int rootX,
    int rootY,
    unsigned int state)
{
    browserDisplay.pointerWindow = window;
    browserDisplay.pointerX = rootX;
    browserDisplay.pointerY = rootY;
    browserDisplay.pointerState = state;
    BrowserSyncPointerState();
}

int
BrowserQueueKeyEventFields(
    int type,
    unsigned int window,
    unsigned int keycode,
    unsigned int keySym,
    unsigned int state,
    int x,
    int y,
    unsigned int time,
    int imeControl)
{
    XEvent event;
    Window target;
    int rootX = x;
    int rootY = y;
    unsigned int resolvedKeycode;
    KeySym resolvedKeySym = (KeySym)keySym;

    BrowserInitDisplay();
    if ((type != KeyPress) && (type != KeyRelease)) {
	return 0;
    }
    resolvedKeycode = BrowserNormalizeKeycode(keycode, resolvedKeySym);
    target = BrowserResolveKeyEventWindow((Window)window);
    BrowserWindowCoordinatesFromRoot(target, rootX, rootY, &x, &y);

    BrowserSetCommonEventFields(&event, type, target, (Time)time);
    event.xkey.root = browserDisplay.rootWindow;
    event.xkey.subwindow = None;
    event.xkey.time = eventTime;
    event.xkey.x = x;
    event.xkey.y = y;
    event.xkey.x_root = rootX;
    event.xkey.y_root = rootY;
    event.xkey.state = state;
    if (imeControl) {
	event.xkey.state |= X11_WASM_IME_CONTROL_STATE;
    }
    event.xkey.keycode = resolvedKeycode;
    event.xkey.same_screen = True;
    return BrowserQueueEvent(&event);
}
