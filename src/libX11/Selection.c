#include "XlibInt.h"

void
BrowserSyncSelectionOwner(
    Atom selection,
    Window owner,
    Time time)
{
    char *selectionName = XGetAtomName(&x11wasm_browserDisplay.display, selection);

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

    for (ownerPtr = x11wasm_selectionOwnerList; ownerPtr != NULL;
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
    ownerPtr->nextPtr = x11wasm_selectionOwnerList;
    x11wasm_selectionOwnerList = ownerPtr;
    return ownerPtr;
}

void
BrowserDeleteSelectionOwner(
    Atom selection)
{
    X11WasmSelectionOwner **linkPtr = &x11wasm_selectionOwnerList;

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
    X11WasmSelectionOwner **linkPtr = &x11wasm_selectionOwnerList;

    while (*linkPtr != NULL) {
	X11WasmSelectionOwner *ownerPtr = *linkPtr;

	if (ownerPtr->owner == window) {
	    BrowserFailQueuedSelectionRequestsForOwner(window);
	    BrowserSyncSelectionOwner(ownerPtr->selection, None, x11wasm_eventTime);
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
	BrowserSyncSelectionOwner(selection, None, x11wasm_eventTime);
	BrowserDeleteSelectionOwner(selection);
	return None;
    }
    return ownerPtr->owner;
}

Time
BrowserSelectionEventTime(
    Time time)
{
    return (time == CurrentTime) ? x11wasm_eventTime : time;
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
    if (x11wasm_externalClipboardText == NULL) {
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

    list[0] = x11wasm_externalClipboardText;
    result = BrowserTextListToTextProperty(&x11wasm_browserDisplay.display, list, 1,
	    style, textPropPtr);
    if ((result == XConverterNotFound) && (target == XA_STRING)) {
	result = BrowserTextListToTextProperty(&x11wasm_browserDisplay.display, list, 1,
		XUTF8StringStyle, textPropPtr);
    }
    return result;
}

static int
BrowserStoreExternalClipboardTarget(
    Window requestor,
    Atom target,
    Atom property,
    Time time)
{
    unsigned int targets[8];
    Time timestamp = x11wasm_externalClipboardTime;

    if ((property == None) || (BrowserWindowResource(requestor) == NULL)) {
	return 0;
    }
    if (target == BrowserTargetsAtom()) {
	targets[0] = (unsigned int)XA_STRING;
	targets[1] = (unsigned int)BrowserUtf8StringAtom();
	targets[2] = (unsigned int)BrowserTextAtom();
	targets[3] = (unsigned int)BrowserCompoundTextAtom();
	targets[4] = (unsigned int)BrowserTargetsAtom();
	targets[5] = (unsigned int)BrowserTimestampAtom();
	targets[6] = (unsigned int)BrowserMultipleAtom();
	targets[7] = (unsigned int)BrowserLengthAtom();
	(void)XChangeProperty(&x11wasm_browserDisplay.display, requestor,
		property, XA_ATOM, 32, PropModeReplace,
		(const unsigned char *)targets,
		(int)(sizeof(targets) / sizeof(targets[0])));
	return 1;
    }
    if (target == BrowserTimestampAtom()) {
	unsigned int timestampValue;

	if (timestamp == CurrentTime) {
	    timestamp = BrowserSelectionEventTime(time);
	}
	timestampValue = (unsigned int)timestamp;
	(void)XChangeProperty(&x11wasm_browserDisplay.display, requestor,
		property, XA_INTEGER, 32, PropModeReplace,
		(const unsigned char *)&timestampValue, 1);
	return 1;
    }
    if (target == BrowserLengthAtom()) {
	unsigned int lengthValue = 0;

	if (x11wasm_externalClipboardText != NULL) {
	    lengthValue = (unsigned int)strlen(x11wasm_externalClipboardText);
	}
	(void)XChangeProperty(&x11wasm_browserDisplay.display, requestor,
		property, XA_INTEGER, 32, PropModeReplace,
		(const unsigned char *)&lengthValue, 1);
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
	(void)XChangeProperty(&x11wasm_browserDisplay.display, requestor,
		property, textProp.encoding, textProp.format,
		PropModeReplace, (const unsigned char *)textProp.value,
		(int)textProp.nitems);
	if (textProp.value != NULL) {
	    XFree(textProp.value);
	}
    }
    return 1;
}

static int
BrowserConvertExternalClipboardMultiple(
    Window requestor,
    Atom property,
    Time time)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    unsigned int *pairs;
    unsigned long i;
    int ok = 0;

    if ((property == None) || (BrowserWindowResource(requestor) == NULL)) {
	return 0;
    }
    if (XGetWindowProperty(&x11wasm_browserDisplay.display, requestor, property, 0,
	    (long)(LONG_MAX / 4L), False, BrowserAtomPairAtom(), &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data) != Success) {
	goto cleanup;
    }
    if ((actualType != BrowserAtomPairAtom()) || (actualFormat != 32)
	    || (bytesAfter != 0) || (data == NULL) || ((nitems % 2UL) != 0)
	    || (nitems > (unsigned long)INT_MAX)) {
	goto cleanup;
    }
    pairs = (unsigned int *)data;
    for (i = 0; i < nitems; i += 2UL) {
	Atom target = (Atom)pairs[i];
	Atom targetProperty = (Atom)pairs[i + 1UL];

	if ((target == BrowserMultipleAtom()) || (targetProperty == None)
		|| !BrowserStoreExternalClipboardTarget(requestor, target,
			targetProperty, time)) {
	    pairs[i + 1UL] = (unsigned int)None;
	}
    }
    (void)XChangeProperty(&x11wasm_browserDisplay.display, requestor, property,
	    BrowserAtomPairAtom(), 32, PropModeReplace, data, (int)nitems);
    ok = 1;

cleanup:
    if (data != NULL) {
	XFree(data);
    }
    return ok;
}

int
BrowserConvertExternalClipboardSelection(
    Window requestor,
    Atom selection,
    Atom target,
    Atom property,
    Time time)
{
    Atom effectiveProperty = property;

    if ((selection != BrowserClipboardSelectionAtom())
	    || !x11wasm_externalClipboardAvailable) {
	return 0;
    }
    if (target == BrowserMultipleAtom()) {
	if (!BrowserConvertExternalClipboardMultiple(requestor, property,
		time)) {
	    return 0;
	}
	(void)BrowserQueueSelectionNotify(requestor, selection, target,
		property, time);
	return 1;
    }
    if (effectiveProperty == None) {
	effectiveProperty = target;
    }
    if (!BrowserStoreExternalClipboardTarget(requestor, target,
	    effectiveProperty, time)) {
	return 0;
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
    for (offset = 0; offset < x11wasm_eventQueueCount; offset++) {
	int index = (x11wasm_eventQueueHead + offset) % x11wasm_eventQueueCapacity;
	XSelectionRequestEvent request;
	XEvent notify;

	if ((x11wasm_eventQueue[index].event.type != SelectionRequest)
		|| (x11wasm_eventQueue[index].event.xselectionrequest.owner != owner)) {
	    continue;
	}
	request = x11wasm_eventQueue[index].event.xselectionrequest;
	BrowserSetCommonEventFields(&notify, SelectionNotify, request.requestor,
		request.time);
	notify.xselection.requestor = request.requestor;
	notify.xselection.selection = request.selection;
	notify.xselection.target = request.target;
	notify.xselection.property = None;
	notify.xselection.time = BrowserSelectionEventTime(request.time);
	x11wasm_eventQueue[index].event = notify;
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
    return BrowserTakeQueuedEvent(&x11wasm_browserDisplay.display, NULL, NULL, 1,
	    eventPtr, internalPtr);
}

void
BrowserClearEventQueue(void)
{
    BrowserClearCompositionPayloads();
    x11wasm_eventQueueHead = 0;
    x11wasm_eventQueueCount = 0;
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
	return (eventPtr->xdestroywindow.event == eventPtr->xdestroywindow.window)
		? StructureNotifyMask : SubstructureNotifyMask;
    case UnmapNotify:
	return (eventPtr->xunmap.event == eventPtr->xunmap.window)
		? StructureNotifyMask : SubstructureNotifyMask;
    case MapNotify:
	return (eventPtr->xmap.event == eventPtr->xmap.window)
		? StructureNotifyMask : SubstructureNotifyMask;
    case ReparentNotify:
	return (eventPtr->xreparent.event == eventPtr->xreparent.window)
		? StructureNotifyMask : SubstructureNotifyMask;
    case ConfigureNotify:
	return (eventPtr->xconfigure.event == eventPtr->xconfigure.window)
		? StructureNotifyMask : SubstructureNotifyMask;
    case GravityNotify:
	return (eventPtr->xgravity.event == eventPtr->xgravity.window)
		? StructureNotifyMask : SubstructureNotifyMask;
    case CirculateNotify:
	return (eventPtr->xcirculate.event == eventPtr->xcirculate.window)
		? StructureNotifyMask : SubstructureNotifyMask;
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

    if (!BrowserWindowCanReceiveEvents(x11wasm_browserDisplay.pointerGrabWindow)) {
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
    if (eventMask == NoEventMask) {
	return 1;
    }
    if (x11wasm_browserDisplay.pointerGrabOwnerEvents
	    && (BrowserSelectedEventTarget(BrowserEventWindow(eventPtr),
		    eventMask, 1) != None)) {
	return 1;
    }
    return (x11wasm_browserDisplay.pointerGrabEventMask & eventMask) != 0;
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
	Window pointerWindow = x11wasm_browserDisplay.pointerWindow;

	if (!BrowserWindowCanReceiveEvents(pointerWindow)) {
	    pointerWindow = BrowserHitTestRoot(x11wasm_browserDisplay.pointerX,
		    x11wasm_browserDisplay.pointerY, NULL, NULL);
	}
	window = BrowserWindowCanReceiveEvents(pointerWindow)
		? pointerWindow : x11wasm_browserDisplay.rootWindow;
    } else if (window == InputFocus) {
	Window focusWindow = x11wasm_browserDisplay.focusWindow;

	window = BrowserWindowCanReceiveEvents(focusWindow)
		? focusWindow : x11wasm_browserDisplay.rootWindow;
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
	if (resourcePtr->id == x11wasm_browserDisplay.rootWindow) {
	    break;
	}
    }
    return None;
}

Window
BrowserSelectedEventTarget(
    Window window,
    long eventMask,
    int propagate)
{
    X11WasmResource *resourcePtr;

    BrowserInitDisplay();
    resourcePtr = BrowserWindowResource(window);
    if (eventMask == NoEventMask) {
	return (resourcePtr != NULL) ? window : None;
    }
    if (!propagate) {
	return BrowserResourceWantsEvent(resourcePtr, eventMask) ? window : None;
    }
    for (; resourcePtr != NULL; resourcePtr = BrowserWindowResource(
	    resourcePtr->parent)) {
	if (BrowserResourceWantsEvent(resourcePtr, eventMask)) {
	    return (Window)resourcePtr->id;
	}
	if (resourcePtr->atts.do_not_propagate_mask & eventMask) {
	    return None;
	}
	if (resourcePtr->id == x11wasm_browserDisplay.rootWindow) {
	    break;
	}
    }
    return None;
}

int
BrowserWindowMaskQueuedEvent(
    Display *display,
    XEvent *eventPtr,
    XPointer arg)
{
    const long *matchPtr = (const long *)arg;
    Window window = (Window)matchPtr[0];
    long mask = matchPtr[1];

    (void)display;
    return BrowserEventMatchesWindowMask(eventPtr, window, mask);
}

int
BrowserTakeQueuedEvent(
    Display *display,
    X11WasmQueuedEventPredicate predicate,
    XPointer arg,
    int remove,
    XEvent *eventPtr,
    int *internalPtr)
{
    int offset;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    for (offset = 0; offset < x11wasm_eventQueueCount; offset++) {
	int index = (x11wasm_eventQueueHead + offset) % x11wasm_eventQueueCapacity;

	if ((predicate != NULL)
		&& !predicate(display, &x11wasm_eventQueue[index].event, arg)) {
	    continue;
	}
	if (eventPtr != NULL) {
	    *eventPtr = x11wasm_eventQueue[index].event;
	}
	if (internalPtr != NULL) {
	    *internalPtr = x11wasm_eventQueue[index].internal;
	}
	if (remove) {
	    if (offset == 0) {
		x11wasm_eventQueueHead = (x11wasm_eventQueueHead + 1) % x11wasm_eventQueueCapacity;
	    } else {
		while (offset < x11wasm_eventQueueCount - 1) {
		    int dest = (x11wasm_eventQueueHead + offset) % x11wasm_eventQueueCapacity;
		    int src = (x11wasm_eventQueueHead + offset + 1) % x11wasm_eventQueueCapacity;

		    x11wasm_eventQueue[dest] = x11wasm_eventQueue[src];
		    offset++;
		}
	    }
	    x11wasm_eventQueueCount--;
	}
	BrowserUpdateQueueLength();
	return 1;
    }
    if (eventPtr != NULL) {
	memset(eventPtr, 0, sizeof(XEvent));
    }
    if (internalPtr != NULL) {
	*internalPtr = 0;
    }
    BrowserUpdateQueueLength();
    return 0;
}

int
BrowserPopMatchingEvent(
    Window window,
    long mask,
    XEvent *eventPtr)
{
    long match[2];

    match[0] = (long)window;
    match[1] = mask;
    return BrowserTakeQueuedEvent(&x11wasm_browserDisplay.display,
	    BrowserWindowMaskQueuedEvent, (XPointer)match, 1, eventPtr, NULL);
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
	    resourcePtr != NULL && resourcePtr->id != x11wasm_browserDisplay.rootWindow;
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

	if (currentPtr->parent == x11wasm_browserDisplay.rootWindow) {
	    break;
	}
	parentPtr = (currentPtr->parentPtr != NULL)
		? currentPtr->parentPtr : BrowserWindowResource(currentPtr->parent);
	if ((parentPtr == NULL) || (parentPtr->type != X11_WASM_RESOURCE_WINDOW)
		|| (parentPtr->id == x11wasm_browserDisplay.rootWindow)) {
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

    for (resourcePtr = x11wasm_browserDisplay.resourceList; resourcePtr != NULL;
	    resourcePtr = resourcePtr->nextPtr) {
	resourceCount++;
    }
    for (pass = 0; pass < resourceCount; pass++) {
	int changed = 0;

	for (resourcePtr = x11wasm_browserDisplay.resourceList; resourcePtr != NULL;
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
	    || (resourcePtr->id == x11wasm_browserDisplay.rootWindow)) {
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

X11WasmResource *
BrowserFindCirculateSubwindow(
    X11WasmResource *parentPtr,
    int direction,
    int *placePtr)
{
    X11WasmResource *resourcePtr;

    if ((parentPtr == NULL)
	    || (parentPtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return NULL;
    }
    switch (direction) {
    case RaiseLowest:
	for (resourcePtr = parentPtr->firstChildPtr; resourcePtr != NULL;
		resourcePtr = resourcePtr->nextSiblingPtr) {
	    if (BrowserWindowIsObscuredBySibling(resourcePtr, NULL)) {
		if (placePtr != NULL) {
		    *placePtr = PlaceOnTop;
		}
		return resourcePtr;
	    }
	}
	break;
    case LowerHighest:
	for (resourcePtr = parentPtr->lastChildPtr; resourcePtr != NULL;
		resourcePtr = resourcePtr->prevSiblingPtr) {
	    if (BrowserWindowObscuresSibling(resourcePtr, NULL)) {
		if (placePtr != NULL) {
		    *placePtr = PlaceOnBottom;
		}
		return resourcePtr;
	    }
	}
	break;
    }
    return NULL;
}

X11WasmResource *
BrowserCirculateSubwindow(
    X11WasmResource *parentPtr,
    int direction,
    int *placePtr)
{
    X11WasmResource *resourcePtr;
    int place = PlaceOnTop;

    resourcePtr = BrowserFindCirculateSubwindow(parentPtr, direction, &place);
    if (resourcePtr == NULL) {
	return NULL;
    }
    if (place == PlaceOnTop) {
	BrowserLinkChildAtTop(resourcePtr, (Window)parentPtr->id);
    } else {
	BrowserLinkChildAtBottom(resourcePtr, (Window)parentPtr->id);
    }
    (void)X11WasmJsRestackWindow(BrowserJsXId(resourcePtr->id),
	    BrowserJsXId(resourcePtr->prevSiblingPtr != NULL
		    ? resourcePtr->prevSiblingPtr->id : None));
    BrowserSyncTransientStacking();
    if (placePtr != NULL) {
	*placePtr = place;
    }
    return resourcePtr;
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
	if (resourcePtr->id == x11wasm_browserDisplay.rootWindow) {
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
    resourcePtr = &x11wasm_browserDisplay.rootResource;
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
	    || (x11wasm_browserDisplay.pointerGrabConfineWindow == None)) {
	return;
    }
    resourcePtr = BrowserWindowResource(x11wasm_browserDisplay.pointerGrabConfineWindow);
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

X11WasmResource *
BrowserWindowParentResource(
    X11WasmResource *resourcePtr)
{
    if ((resourcePtr == NULL) || (resourcePtr->id == x11wasm_browserDisplay.rootWindow)) {
	return NULL;
    }
    if (resourcePtr->parentPtr != NULL) {
	return resourcePtr->parentPtr;
    }
    return BrowserWindowResource(resourcePtr->parent);
}

int
BrowserWindowTreeDepth(
    X11WasmResource *resourcePtr)
{
    int depth = 0;

    while ((resourcePtr != NULL)
	    && (resourcePtr->id != x11wasm_browserDisplay.rootWindow)) {
	resourcePtr = BrowserWindowParentResource(resourcePtr);
	depth++;
    }
    return depth;
}

X11WasmResource *
BrowserWindowAncestorResource(
    X11WasmResource *resourcePtr,
    int levels)
{
    while ((resourcePtr != NULL) && (levels > 0)) {
	resourcePtr = BrowserWindowParentResource(resourcePtr);
	levels--;
    }
    return resourcePtr;
}

X11WasmResource *
BrowserWindowCommonAncestorResource(
    X11WasmResource *sourcePtr,
    X11WasmResource *destPtr,
    int *upLevelsPtr,
    int *downLevelsPtr)
{
    X11WasmResource *sourceAncestorPtr = sourcePtr;
    X11WasmResource *destAncestorPtr = destPtr;
    int sourceDepth = BrowserWindowTreeDepth(sourcePtr);
    int destDepth = BrowserWindowTreeDepth(destPtr);
    int upLevels = 0;
    int downLevels = 0;

    while (sourceDepth > destDepth) {
	sourceAncestorPtr = BrowserWindowParentResource(sourceAncestorPtr);
	sourceDepth--;
	upLevels++;
    }
    while (destDepth > sourceDepth) {
	destAncestorPtr = BrowserWindowParentResource(destAncestorPtr);
	destDepth--;
	downLevels++;
    }
    while ((sourceAncestorPtr != NULL) && (destAncestorPtr != NULL)
	    && (sourceAncestorPtr != destAncestorPtr)) {
	sourceAncestorPtr = BrowserWindowParentResource(sourceAncestorPtr);
	destAncestorPtr = BrowserWindowParentResource(destAncestorPtr);
	upLevels++;
	downLevels++;
    }
    if (sourceAncestorPtr != destAncestorPtr) {
	sourceAncestorPtr = NULL;
    }
    if (upLevelsPtr != NULL) {
	*upLevelsPtr = upLevels;
    }
    if (downLevelsPtr != NULL) {
	*downLevelsPtr = downLevels;
    }
    return sourceAncestorPtr;
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
	    resourcePtr != NULL && resourcePtr->id != x11wasm_browserDisplay.rootWindow;
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
    int hasGrab;

    (void)window;
    BrowserInitDisplay();
    rawTarget = BrowserHitTestRoot(rootX, rootY, NULL, NULL);
    grabWindow = x11wasm_browserDisplay.pointerGrabWindow;
    hasGrab = BrowserWindowCanReceiveEvents(grabWindow);

    target = rawTarget;
    if (hasGrab && !x11wasm_browserDisplay.pointerGrabOwnerEvents) {
	target = grabWindow;
    } else if (!hasGrab
	    && BrowserWindowCanReceiveEvents(
		    x11wasm_browserDisplay.pointerCaptureWindow)) {
	target = x11wasm_browserDisplay.pointerCaptureWindow;
    }
    if (!BrowserWindowCanReceiveEvents(target)) {
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
    x11wasm_browserDisplay.pointerWindow = window;
    x11wasm_browserDisplay.pointerX = rootX;
    x11wasm_browserDisplay.pointerY = rootY;
    x11wasm_browserDisplay.pointerState = state;
    BrowserSyncPointerState();
}

static void
BrowserUpdateKeymapState(
    unsigned int keycode,
    int pressed)
{
    unsigned int index;
    unsigned char mask;

    if (keycode > X11_WASM_MAX_KEYCODE) {
	return;
    }
    index = keycode >> 3;
    mask = (unsigned char)(1U << (keycode & 7U));
    if (pressed) {
	x11wasm_browserDisplay.keymap[index] =
		(char)((unsigned char)x11wasm_browserDisplay.keymap[index] | mask);
    } else {
	x11wasm_browserDisplay.keymap[index] =
		(char)((unsigned char)x11wasm_browserDisplay.keymap[index] & ~mask);
    }
}

static Window
BrowserResolvePinnedKeyEventWindow(
    Window window)
{
    Window target;
    Window grabWindow;
    int grabOwnerEvents = 1;

    if (!BrowserWindowCanReceiveEvents(window)) {
	return BrowserResolveKeyEventWindow(window);
    }
    target = window;

    grabWindow = x11wasm_browserDisplay.keyboardGrabWindow;
    if (BrowserWindowCanReceiveEvents(grabWindow)) {
	grabOwnerEvents = x11wasm_browserDisplay.keyboardGrabOwnerEvents;
    } else {
	grabWindow = BrowserGetGrabWindow();
	grabOwnerEvents = 1;
    }
    if (BrowserWindowCanReceiveEvents(grabWindow)) {
	if (!grabOwnerEvents || !BrowserWindowInTree(target, grabWindow)) {
	    target = grabWindow;
	}
    }
    if (!BrowserWindowCanReceiveEvents(target)) {
	target = BrowserResolveKeyEventWindow(window);
    }
    return target;
}

static int
BrowserQueueKeyEventFieldsToTarget(
    int type,
    unsigned int window,
    unsigned int keycode,
    unsigned int keySym,
    unsigned int state,
    int x,
    int y,
    unsigned int time,
    int imeControl,
    int forceWindow)
{
    XEvent event;
    Window target;
    Window deliveryTarget;
    int rootX = x;
    int rootY = y;
    unsigned int resolvedKeycode;
    KeySym resolvedKeySym = (KeySym)keySym;
    int queued;
    int forceTarget;

    BrowserInitDisplay();
    if ((type != KeyPress) && (type != KeyRelease)) {
	return 0;
    }
    BrowserResetScreenSaverForInput();
    resolvedKeycode = BrowserNormalizeKeycode(keycode, resolvedKeySym);
    target = forceWindow ? BrowserResolvePinnedKeyEventWindow((Window)window)
	    : BrowserResolveKeyEventWindow((Window)window);
    if (type == KeyPress) {
	(void)BrowserActivatePassiveKeyGrab(target, resolvedKeycode, state,
		(Time)time);
	target = forceWindow ? BrowserResolvePinnedKeyEventWindow((Window)window)
		: BrowserResolveKeyEventWindow((Window)window);
    }
    BrowserWindowCoordinatesFromRoot(target, rootX, rootY, &x, &y);

    BrowserSetCommonEventFields(&event, type, target, (Time)time);
    event.xkey.root = x11wasm_browserDisplay.rootWindow;
    event.xkey.subwindow = None;
    event.xkey.time = x11wasm_eventTime;
    event.xkey.x = x;
    event.xkey.y = y;
    event.xkey.x_root = rootX;
    event.xkey.y_root = rootY;
    event.xkey.state = state;
    if (imeControl) {
	event.xkey.state |= X11_WASM_IME_CONTROL_STATE;
    }
    if (forceWindow) {
	event.xkey.state |= X11_WASM_PINNED_KEY_EVENT_STATE;
    }
    event.xkey.keycode = resolvedKeycode;
    event.xkey.same_screen = True;
    BrowserUpdateKeymapState(resolvedKeycode, type == KeyPress);
    forceTarget = BrowserWindowCanReceiveEvents(x11wasm_browserDisplay.keyboardGrabWindow)
	    && !x11wasm_browserDisplay.keyboardGrabOwnerEvents;
    deliveryTarget = forceTarget ? target
	    : BrowserSelectedEventTarget(target, BrowserEventMask(&event), 1);
    if (deliveryTarget == None) {
	if (type == KeyRelease) {
	    BrowserReleasePassiveKeyboardGrabIfIdle();
	}
	return 0;
    }
    if (deliveryTarget != target) {
	target = deliveryTarget;
	BrowserWindowCoordinatesFromRoot(target, rootX, rootY, &x, &y);
	event.xany.window = target;
	event.xkey.x = x;
	event.xkey.y = y;
    }
    queued = BrowserQueueEvent(&event);
    if (type == KeyRelease) {
	BrowserReleasePassiveKeyboardGrabIfIdle();
    }
    return queued;
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
    return BrowserQueueKeyEventFieldsToTarget(type, window, keycode, keySym,
	    state, x, y, time, imeControl, 0);
}

int
BrowserQueueKeyEventFieldsAtWindow(
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
    return BrowserQueueKeyEventFieldsToTarget(type, window, keycode, keySym,
	    state, x, y, time, imeControl, 1);
}
