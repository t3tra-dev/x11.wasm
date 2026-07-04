#include "XlibInt.h"

EMSCRIPTEN_KEEPALIVE int
X11WasmResizeRoot(
    unsigned int width,
    unsigned int height)
{
    BrowserInitDisplay();
    return BrowserResizeRoot(width, height, 1);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueKeyEventFields(
    int type,
    unsigned int window,
    unsigned int keycode,
    unsigned int keySym,
    unsigned int state,
    int x,
    int y,
    unsigned int time)
{
    return BrowserQueueKeyEventFields(type, window, keycode, keySym, state,
	    x, y, time, 0);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueIMKeyEventFields(
    int type,
    unsigned int window,
    unsigned int keycode,
    unsigned int state,
    int x,
    int y,
    unsigned int time)
{
    return BrowserQueueKeyEventFields(type, window, keycode, NoSymbol, state,
	    x, y, time, 1);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueKeyEvent(
    int type,
    unsigned int window,
    unsigned int keySym,
    unsigned int state,
    unsigned int time)
{
    BrowserInitDisplay();
    return X11WasmQueueKeyEventFields(type, window,
	    BrowserKeycodeFromKeySym((KeySym)keySym), keySym, state,
	    browserDisplay.pointerX, browserDisplay.pointerY, time);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueTextEventFields(
    int type,
    unsigned int window,
    unsigned int codepoint,
    unsigned int state,
    unsigned int time)
{
    KeySym keySym = BrowserKeySymFromCodepoint(codepoint);

    BrowserInitDisplay();
    if (((type != KeyPress) && (type != KeyRelease))
	    || (keySym == NoSymbol)) {
	return 0;
    }
    return X11WasmQueueKeyEventFields(type, window,
	    BrowserKeycodeFromKeySym(keySym), (unsigned int)keySym, state,
	    browserDisplay.pointerX, browserDisplay.pointerY, time);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueTextEventFieldsAtWindow(
    int type,
    unsigned int window,
    unsigned int codepoint,
    unsigned int state,
    unsigned int time)
{
    KeySym keySym = BrowserKeySymFromCodepoint(codepoint);

    BrowserInitDisplay();
    if (((type != KeyPress) && (type != KeyRelease))
	    || (keySym == NoSymbol)) {
	return 0;
    }
    return BrowserQueueKeyEventFieldsAtWindow(type, window,
	    BrowserKeycodeFromKeySym(keySym), (unsigned int)keySym, state,
	    browserDisplay.pointerX, browserDisplay.pointerY, time, 0);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueTextEvent(
    unsigned int window,
    unsigned int codepoint,
    unsigned int state,
    unsigned int time)
{
    return X11WasmQueueTextEventFields(KeyPress, window, codepoint, state,
	    time);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueCompositionEvent(
    int phase,
    unsigned int window,
    const char *utf8,
    int utf8Length)
{
    const char *eventName = BrowserCompositionEventName(phase);
    Window target;
    XEvent event;
    unsigned long payloadId = 0;

    if (eventName == NULL) {
	return 0;
    }
    if (utf8 == NULL) {
	utf8 = "";
	utf8Length = 0;
    } else if (utf8Length < 0) {
	utf8Length = (int)strlen(utf8);
    }
    if (((Window)window != browserDisplay.rootWindow)
	    && BrowserWindowCanReceiveEvents((Window)window)) {
	target = (Window)window;
    } else {
	target = BrowserResolveKeyEventWindow((Window)window);
    }
    if (!BrowserWindowCanReceiveEvents(target)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, ClientMessage, target, CurrentTime);
    event.xclient.message_type = XInternAtom(&browserDisplay.display,
	    eventName, False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = (long)phase;
    event.xclient.data.l[1] = (long)utf8Length;
    payloadId = BrowserStoreCompositionPayload(utf8, utf8Length);
    event.xclient.data.l[2] = (long)payloadId;
    if (!BrowserQueueEvent(&event)) {
	BrowserForgetCompositionPayload(payloadId);
	return 0;
    }
    BrowserUpdateFocusedICPreedit(target, phase, utf8, utf8Length);
    return 1;
}

unsigned long
BrowserStoreCompositionPayload(
    const char *utf8,
    int utf8Length)
{
    X11WasmCompositionPayload *payloadPtr;

    if ((utf8 == NULL) || (utf8Length <= 0)) {
	return 0;
    }
    payloadPtr = (X11WasmCompositionPayload *)ckalloc(
	    sizeof(X11WasmCompositionPayload));
    payloadPtr->utf8 = (char *)ckalloc((size_t)utf8Length + 1);
    memcpy(payloadPtr->utf8, utf8, (size_t)utf8Length);
    payloadPtr->utf8[utf8Length] = '\0';
    payloadPtr->utf8Length = utf8Length;
    payloadPtr->id = nextCompositionPayloadId++;
    if (nextCompositionPayloadId == 0) {
	nextCompositionPayloadId = 1;
    }
    payloadPtr->nextPtr = compositionPayloadList;
    compositionPayloadList = payloadPtr;
    return payloadPtr->id;
}

void
BrowserForgetCompositionPayload(
    unsigned long payloadId)
{
    X11WasmCompositionPayload **linkPtr = &compositionPayloadList;

    if (payloadId == 0) {
	return;
    }
    while (*linkPtr != NULL) {
	X11WasmCompositionPayload *payloadPtr = *linkPtr;

	if (payloadPtr->id == payloadId) {
	    *linkPtr = payloadPtr->nextPtr;
	    ckfree(payloadPtr->utf8);
	    ckfree(payloadPtr);
	    return;
	}
	linkPtr = &payloadPtr->nextPtr;
    }
}

void
BrowserClearCompositionPayloads(void)
{
    while (compositionPayloadList != NULL) {
	X11WasmCompositionPayload *payloadPtr = compositionPayloadList;

	compositionPayloadList = payloadPtr->nextPtr;
	ckfree(payloadPtr->utf8);
	ckfree(payloadPtr);
    }
}

char *
X11WasmTakeCompositionText(
    XEvent *eventPtr,
    int *utf8LengthReturn)
{
    X11WasmCompositionPayload **linkPtr = &compositionPayloadList;
    unsigned long payloadId;

    if (utf8LengthReturn != NULL) {
	*utf8LengthReturn = 0;
    }
    if ((eventPtr == NULL) || (eventPtr->type != ClientMessage)) {
	return NULL;
    }
    payloadId = (unsigned long)eventPtr->xclient.data.l[2];
    if (payloadId == 0) {
	return NULL;
    }
    while (*linkPtr != NULL) {
	X11WasmCompositionPayload *payloadPtr = *linkPtr;

	if (payloadPtr->id == payloadId) {
	    char *utf8 = payloadPtr->utf8;

	    *linkPtr = payloadPtr->nextPtr;
	    if (utf8LengthReturn != NULL) {
		*utf8LengthReturn = payloadPtr->utf8Length;
	    }
	    ckfree(payloadPtr);
	    eventPtr->xclient.data.l[2] = 0;
	    return utf8;
	}
	linkPtr = &payloadPtr->nextPtr;
    }
    eventPtr->xclient.data.l[2] = 0;
    return NULL;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmSetExternalClipboardText(
    const char *utf8,
    int utf8Length)
{
    BrowserSetExternalClipboardText(utf8, utf8Length);
    return 1;
}

int
X11WasmSetClipboardText(
    const char *utf8,
    int utf8Length)
{
    if (utf8 == NULL) {
	utf8 = "";
	utf8Length = 0;
    } else if (utf8Length < 0) {
	utf8Length = (int)strlen(utf8);
    }
    (void)X11WasmJsSetClipboardText(utf8, utf8Length);
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueWindowProtocol(
    unsigned int window,
    const char *protocolUtf8,
    int protocolUtf8Length,
    unsigned int time)
{
    X11WasmResource *resourcePtr;
    char *protocolName;
    Atom protocolAtom;
    XEvent event;

    BrowserInitDisplay();
    resourcePtr = X11WasmFindResource((Window)window);
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (protocolUtf8 == NULL)) {
	return 0;
    }
    if (protocolUtf8Length < 0) {
	protocolUtf8Length = (int)strlen(protocolUtf8);
    }
    protocolName = BrowserDupCountedString((const unsigned char *)protocolUtf8,
	    protocolUtf8Length);
    if (protocolName[0] == '\0') {
	ckfree(protocolName);
	return 0;
    }
    protocolAtom = XInternAtom(&browserDisplay.display, protocolName, True);
    ckfree(protocolName);
    if (!BrowserWindowSupportsProtocol(resourcePtr, protocolAtom)) {
	return 0;
    }

    BrowserSetCommonEventFields(&event, ClientMessage, (Window)window,
	    (Time)time);
    event.xclient.send_event = True;
    event.xclient.message_type = BrowserWmProtocolsAtom();
    event.xclient.format = 32;
    event.xclient.data.l[0] = (long)protocolAtom;
    event.xclient.data.l[1] = (long)((time != CurrentTime) ? time : eventTime);
    return BrowserQueueEvent(&event);
}

static void
BrowserQueueImplicitPointerCrossingEvents(
    int type,
    unsigned int state,
    int rootX,
    int rootY,
    unsigned int time);

static unsigned int
BrowserMappedPointerButton(
    unsigned int button)
{
    if ((button >= Button1)
	    && (button <= (unsigned int)browserDisplay.pointerMapCount)) {
	return browserDisplay.pointerMap[button - Button1];
    }
    return button;
}

static unsigned int
BrowserMappedPointerState(
    unsigned int state)
{
    unsigned int mappedState = state
	    & ~(Button1Mask | Button2Mask | Button3Mask | Button4Mask
		    | Button5Mask);
    unsigned int button;

    for (button = Button1; button <= Button5; button++) {
	if (state & BrowserPointerButtonMask(button)) {
	    mappedState |= BrowserPointerButtonMask(
		    BrowserMappedPointerButton(button));
	}
    }
    return mappedState;
}

static unsigned int
BrowserPointerPostStateForButtonEvent(
    int type,
    unsigned int button,
    unsigned int state)
{
    unsigned int mask = BrowserPointerButtonMask(button);

    if (type == ButtonPress) {
	return state | mask;
    }
    if (type == ButtonRelease) {
	return state & ~mask;
    }
    return state;
}

static void
BrowserUpdatePointerStateForSuppressedEvent(
    int type,
    unsigned int window,
    unsigned int state,
    int x,
    int y,
    unsigned int time,
    int resetScreenSaver)
{
    Window target;
    int rootX = x;
    int rootY = y;

    if (resetScreenSaver) {
	BrowserResetScreenSaverForInput();
    }
    BrowserClampPointerToGrabConfine(&rootX, &rootY);
    x = rootX;
    y = rootY;
    BrowserQueueImplicitPointerCrossingEvents(type, state, rootX, rootY,
	    time);
    target = BrowserResolvePointerEventWindow((Window)window, &x, &y);
    BrowserUpdatePointerState(target, rootX, rootY, state);
}

static int
BrowserQueuePointerEventFieldsWithState(
    int type,
    unsigned int window,
    unsigned int button,
    unsigned int state,
    unsigned int pointerState,
    int x,
    int y,
    unsigned int time,
    int resetScreenSaver)
{
    XEvent event;
    Window target;
    Window deliveryTarget;
    Window captureTarget;
    int rootX = x;
    int rootY = y;
    int queued;
    int hasPointerGrab;
    long eventMask;

    if ((type != ButtonPress) && (type != ButtonRelease)
	    && (type != MotionNotify)) {
	return 0;
    }
    if (resetScreenSaver) {
	BrowserResetScreenSaverForInput();
    }
    BrowserClampPointerToGrabConfine(&rootX, &rootY);
    x = rootX;
    y = rootY;
    BrowserQueueImplicitPointerCrossingEvents(type, state, rootX, rootY, time);
    if (type == ButtonPress) {
	Window passiveWindow = BrowserHitTestRoot(rootX, rootY, NULL, NULL);

	(void)BrowserActivatePassiveButtonGrab(passiveWindow, button, state,
		(Time)time);
    }
    target = BrowserResolvePointerEventWindow((Window)window, &x, &y);
    captureTarget = target;
    BrowserUpdatePointerState(target, rootX, rootY, pointerState);

    BrowserSetCommonEventFields(&event, type, target, (Time)time);
    if (type == MotionNotify) {
	event.xmotion.root = browserDisplay.rootWindow;
	event.xmotion.subwindow = None;
	event.xmotion.time = eventTime;
	event.xmotion.x = x;
	event.xmotion.y = y;
	event.xmotion.x_root = rootX;
	event.xmotion.y_root = rootY;
	event.xmotion.state = state;
	event.xmotion.is_hint = NotifyNormal;
	event.xmotion.same_screen = True;
	BrowserRecordPointerMotion(rootX, rootY, event.xmotion.time);
    } else {
	event.xbutton.root = browserDisplay.rootWindow;
	event.xbutton.subwindow = None;
	event.xbutton.time = eventTime;
	event.xbutton.x = x;
	event.xbutton.y = y;
	event.xbutton.x_root = rootX;
	event.xbutton.y_root = rootY;
	event.xbutton.state = state;
	event.xbutton.button = button;
	event.xbutton.same_screen = True;
    }
    if (type == ButtonRelease) {
	browserDisplay.pointerCaptureWindow = None;
	BrowserSyncPointerState();
    }
    if (!BrowserPointerGrabAllowsEvent(&event)) {
	if (type == ButtonRelease) {
	    BrowserReleasePassivePointerGrabIfIdle(
		    pointerState);
	}
	return 0;
    }
    hasPointerGrab =
	    BrowserWindowCanReceiveEvents(browserDisplay.pointerGrabWindow);
    eventMask = BrowserEventMask(&event);
    deliveryTarget = (hasPointerGrab && !browserDisplay.pointerGrabOwnerEvents)
	    ? target : BrowserSelectedEventTarget(target, eventMask, 1);
    if ((deliveryTarget == None) && hasPointerGrab
	    && browserDisplay.pointerGrabOwnerEvents
	    && ((browserDisplay.pointerGrabEventMask & eventMask) != 0)) {
	deliveryTarget = browserDisplay.pointerGrabWindow;
    }
    if (deliveryTarget == None) {
	if (type == ButtonRelease) {
	    BrowserReleasePassivePointerGrabIfIdle(
		    pointerState);
	}
	return 0;
    }
    if (deliveryTarget != target) {
	target = deliveryTarget;
	BrowserWindowCoordinatesFromRoot(target, rootX, rootY, &x, &y);
	event.xany.window = target;
	if (type == MotionNotify) {
	    event.xmotion.x = x;
	    event.xmotion.y = y;
	} else {
	    event.xbutton.x = x;
	    event.xbutton.y = y;
	}
    }
    if (type == ButtonPress) {
	browserDisplay.pointerCaptureWindow = captureTarget;
	BrowserSyncPointerState();
    }
    queued = BrowserQueueEvent(&event);
    if (type == ButtonRelease) {
	BrowserReleasePassivePointerGrabIfIdle(pointerState);
    }
    return queued;
}

int
BrowserQueuePointerEventFields(
    int type,
    unsigned int window,
    unsigned int button,
    unsigned int state,
    int x,
    int y,
    unsigned int time,
    int resetScreenSaver)
{
    return BrowserQueuePointerEventFieldsWithState(type, window, button, state,
	    state, x, y, time, resetScreenSaver);
}

static void
BrowserQueueImplicitPointerCrossingEvents(
    int type,
    unsigned int state,
    int rootX,
    int rootY,
    unsigned int time)
{
    Window target;

    if ((type != ButtonPress) && (type != ButtonRelease)
	    && (type != MotionNotify)) {
	return;
    }
    if (BrowserWindowCanReceiveEvents(browserDisplay.pointerCaptureWindow)
	    && (!BrowserWindowCanReceiveEvents(browserDisplay.pointerGrabWindow)
		    || !browserDisplay.pointerGrabOwnerEvents)) {
	return;
    }
    target = BrowserHitTestRoot(rootX, rootY, NULL, NULL);
    if (target == browserDisplay.pointerWindow) {
	return;
    }
    (void)BrowserQueuePointerCrossingEvents(state, rootX, rootY, time);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueuePointerEvent(
    int type,
    unsigned int window,
    unsigned int button,
    unsigned int state,
    int x,
    int y,
    unsigned int time)
{
    unsigned int mappedButton;
    unsigned int mappedState;
    unsigned int pointerState;

    if ((type != ButtonPress) && (type != ButtonRelease)
	    && (type != MotionNotify)) {
	return 0;
    }
    BrowserInitDisplay();
    mappedState = BrowserMappedPointerState(state);
    if (type == MotionNotify) {
	return BrowserQueuePointerEventFieldsWithState(type, window, button,
		mappedState, mappedState, x, y, time, 1);
    }
    mappedButton = BrowserMappedPointerButton(button);
    pointerState = BrowserMappedPointerState(
	    BrowserPointerPostStateForButtonEvent(type, button, state));
    if (mappedButton == 0) {
	BrowserUpdatePointerStateForSuppressedEvent(type, window, pointerState,
		x, y, time, 1);
	return 0;
    }
    return BrowserQueuePointerEventFieldsWithState(type, window, mappedButton,
	    mappedState, pointerState, x, y, time, 1);
}

static int
BrowserQueueCrossingEventForTarget(
    int type,
    Window target,
    unsigned int state,
    int rootX,
    int rootY,
    Time time,
    int detail,
    int subwindowRootX,
    int subwindowRootY)
{
    XEvent event;
    int x = rootX;
    int y = rootY;
    X11WasmResource *targetPtr;
    X11WasmResource *subwindowPtr = NULL;
    int subwindowX = subwindowRootX;
    int subwindowY = subwindowRootY;

    if ((type != EnterNotify) && (type != LeaveNotify)) {
	return 0;
    }
    if (!BrowserWindowCanReceiveEvents(target)) {
	return 0;
    }
    targetPtr = BrowserWindowResource(target);
    if (targetPtr == NULL) {
	return 0;
    }
    BrowserWindowCoordinatesFromRoot(target, rootX, rootY, &x, &y);
    BrowserWindowCoordinatesFromRoot(target, subwindowRootX, subwindowRootY,
	    &subwindowX, &subwindowY);
    subwindowPtr = BrowserFindTopVisibleChildAt(targetPtr, subwindowX,
	    subwindowY);
    BrowserSetCommonEventFields(&event, type, target, time);
    event.xcrossing.root = browserDisplay.rootWindow;
    event.xcrossing.subwindow = (subwindowPtr != NULL)
	    ? (Window)subwindowPtr->id : None;
    event.xcrossing.time = eventTime;
    event.xcrossing.x = x;
    event.xcrossing.y = y;
    event.xcrossing.x_root = rootX;
    event.xcrossing.y_root = rootY;
    event.xcrossing.mode = NotifyNormal;
    event.xcrossing.detail = detail;
    event.xcrossing.same_screen = True;
    event.xcrossing.focus = BrowserWindowInTree(target, BrowserGetFocusWindow());
    event.xcrossing.state = state;
    if (!BrowserPointerGrabAllowsEvent(&event)) {
	return 0;
    }
    if (BrowserSelectedEventTarget(target, BrowserEventMask(&event), 0)
	    == None) {
	return 0;
    }
    return BrowserQueueEvent(&event);
}

static int
BrowserQueueCrossingForResource(
    X11WasmResource *resourcePtr,
    int type,
    int detail,
    unsigned int state,
    int oldRootX,
    int oldRootY,
    int rootX,
    int rootY,
    Time time)
{
    int subwindowRootX = (type == LeaveNotify) ? oldRootX : rootX;
    int subwindowRootY = (type == LeaveNotify) ? oldRootY : rootY;

    if (resourcePtr == NULL) {
	return 0;
    }
    return BrowserQueueCrossingEventForTarget(type, (Window)resourcePtr->id,
	    state, rootX, rootY, time, detail, subwindowRootX,
	    subwindowRootY);
}

static int
BrowserQueuePointerCrossingEventPath(
    X11WasmResource *sourcePtr,
    X11WasmResource *destPtr,
    unsigned int state,
    int oldRootX,
    int oldRootY,
    int rootX,
    int rootY,
    Time time)
{
    int upLevels = 0;
    int downLevels = 0;
    int queued = 0;
    int i;

    if ((sourcePtr == NULL) || (destPtr == NULL) || (sourcePtr == destPtr)) {
	return 0;
    }
    (void)BrowserWindowCommonAncestorResource(sourcePtr, destPtr, &upLevels,
	    &downLevels);
    if (downLevels == 0) {
	X11WasmResource *resourcePtr;

	queued += BrowserQueueCrossingForResource(sourcePtr, LeaveNotify,
		NotifyAncestor, state, oldRootX, oldRootY, rootX, rootY, time);
	for (resourcePtr = BrowserWindowParentResource(sourcePtr),
		i = upLevels - 1;
		(resourcePtr != NULL) && (i > 0);
		resourcePtr = BrowserWindowParentResource(resourcePtr), i--) {
	    queued += BrowserQueueCrossingForResource(resourcePtr, LeaveNotify,
		    NotifyVirtual, state, oldRootX, oldRootY, rootX, rootY,
		    time);
	}
	queued += BrowserQueueCrossingForResource(destPtr, EnterNotify,
		NotifyInferior, state, oldRootX, oldRootY, rootX, rootY, time);
    } else if (upLevels == 0) {
	queued += BrowserQueueCrossingForResource(sourcePtr, LeaveNotify,
		NotifyInferior, state, oldRootX, oldRootY, rootX, rootY, time);
	for (i = downLevels - 1; i > 0; i--) {
	    X11WasmResource *resourcePtr =
		    BrowserWindowAncestorResource(destPtr, i);

	    queued += BrowserQueueCrossingForResource(resourcePtr, EnterNotify,
		    NotifyVirtual, state, oldRootX, oldRootY, rootX, rootY,
		    time);
	}
	queued += BrowserQueueCrossingForResource(destPtr, EnterNotify,
		NotifyAncestor, state, oldRootX, oldRootY, rootX, rootY, time);
    } else {
	X11WasmResource *resourcePtr;

	queued += BrowserQueueCrossingForResource(sourcePtr, LeaveNotify,
		NotifyNonlinear, state, oldRootX, oldRootY, rootX, rootY, time);
	for (resourcePtr = BrowserWindowParentResource(sourcePtr),
		i = upLevels - 1;
		(resourcePtr != NULL) && (i > 0);
		resourcePtr = BrowserWindowParentResource(resourcePtr), i--) {
	    queued += BrowserQueueCrossingForResource(resourcePtr, LeaveNotify,
		    NotifyNonlinearVirtual, state, oldRootX, oldRootY, rootX,
		    rootY, time);
	}
	for (i = downLevels - 1; i > 0; i--) {
	    resourcePtr = BrowserWindowAncestorResource(destPtr, i);
	    queued += BrowserQueueCrossingForResource(resourcePtr, EnterNotify,
		    NotifyNonlinearVirtual, state, oldRootX, oldRootY, rootX,
		    rootY, time);
	}
	queued += BrowserQueueCrossingForResource(destPtr, EnterNotify,
		NotifyNonlinear, state, oldRootX, oldRootY, rootX, rootY,
		time);
    }
    return queued;
}

static int
BrowserCrossingEndpointDetail(
    int type,
    Window oldTarget,
    Window nextTarget)
{
    int nextIsInferior = BrowserWindowInTree(nextTarget, oldTarget);
    int oldIsInferior = BrowserWindowInTree(oldTarget, nextTarget);

    if (type == LeaveNotify) {
	if (nextIsInferior) {
	    return NotifyInferior;
	}
	if (oldIsInferior) {
	    return NotifyAncestor;
	}
	return NotifyNonlinear;
    }
    if (nextIsInferior) {
	return NotifyAncestor;
    }
    if (oldIsInferior) {
	return NotifyInferior;
    }
    return NotifyNonlinear;
}

int
BrowserQueuePointerCrossingEvents(
    unsigned int state,
    int rootX,
    int rootY,
    unsigned int time)
{
    Window oldTarget;
    Window nextTarget;
    int queued = 0;
    int oldRootX;
    int oldRootY;

    BrowserInitDisplay();
    BrowserClampPointerToGrabConfine(&rootX, &rootY);
    oldTarget = browserDisplay.pointerWindow;
    oldRootX = browserDisplay.pointerX;
    oldRootY = browserDisplay.pointerY;
    nextTarget = BrowserHitTestRoot(rootX, rootY, NULL, NULL);
    if (nextTarget == oldTarget) {
	BrowserUpdatePointerState(nextTarget, rootX, rootY, state);
	return 0;
    }
    queued += BrowserQueuePointerCrossingEventPath(
	    BrowserWindowResource(oldTarget), BrowserWindowResource(nextTarget),
	    state, oldRootX, oldRootY, rootX, rootY, (Time)time);
    BrowserUpdatePointerState(nextTarget, rootX, rootY, state);
    return queued;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueCrossingEvent(
    int type,
    unsigned int window,
    unsigned int state,
    int x,
    int y,
    unsigned int time)
{
    Window target;
    Window nextTarget;
    Window oldTarget;
    int rootX = x;
    int rootY = y;
    int oldRootX;
    int oldRootY;

    if ((type != EnterNotify) && (type != LeaveNotify)) {
	return 0;
    }
    BrowserInitDisplay();
    state = BrowserMappedPointerState(state);
    (void)window;
    BrowserClampPointerToGrabConfine(&rootX, &rootY);
    oldTarget = browserDisplay.pointerWindow;
    nextTarget = BrowserHitTestRoot(rootX, rootY, NULL, NULL);
    if (nextTarget == oldTarget) {
	BrowserUpdatePointerState(nextTarget, rootX, rootY, state);
	return 0;
    }
    oldRootX = browserDisplay.pointerX;
    oldRootY = browserDisplay.pointerY;
    if (type == LeaveNotify) {
	if (!BrowserWindowCanReceiveEvents(browserDisplay.pointerWindow)) {
	    BrowserUpdatePointerState(nextTarget, rootX, rootY, state);
	    return 0;
	}
	target = browserDisplay.pointerWindow;
    } else {
	if (!BrowserWindowCanReceiveEvents(nextTarget)) {
	    BrowserUpdatePointerState(nextTarget, rootX, rootY, state);
	    return 0;
	}
	target = nextTarget;
    }
    BrowserUpdatePointerState(nextTarget, rootX, rootY, state);
    return BrowserQueueCrossingEventForTarget(type, target, state, rootX,
	    rootY, (Time)time,
	    BrowserCrossingEndpointDetail(type, oldTarget, nextTarget),
	    (type == LeaveNotify) ? oldRootX : rootX,
	    (type == LeaveNotify) ? oldRootY : rootY);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueFocusEvent(
    int type,
    unsigned int window,
    unsigned int time)
{
    XEvent event;
    Window target;
    Window eventTarget;
    Window oldFocusWindow;

    if ((type != FocusIn) && (type != FocusOut)) {
	return 0;
    }
    if ((Window)window == browserDisplay.rootWindow) {
	if ((type == FocusOut)
		&& BrowserWindowCanReceiveEvents(browserDisplay.focusWindow)) {
	    target = browserDisplay.focusWindow;
	} else if (BrowserWindowCanReceiveEvents(browserDisplay.pointerWindow)) {
	    target = browserDisplay.pointerWindow;
	} else {
	    target = BrowserResolveEventWindow((Window)window);
	}
    } else {
	target = BrowserWindowCanReceiveEvents((Window)window)
		? (Window)window : BrowserResolveEventWindow((Window)window);
	if (!BrowserWindowCanReceiveEvents((Window)window)
		&& BrowserWindowCanReceiveEvents(browserDisplay.pointerWindow)) {
	    target = browserDisplay.pointerWindow;
	}
    }
    eventTarget = target;
    oldFocusWindow = BrowserGetFocusWindow();
    BrowserRecordFocusChangeTime((Time)time);
    if (type == FocusIn) {
	browserDisplay.focusWindow = target;
    } else {
	if ((browserDisplay.focusWindow == target)
		|| (browserDisplay.focusWindow == eventTarget)) {
	    browserDisplay.focusWindow = None;
	}
    }

    BrowserSetCommonEventFields(&event, type, eventTarget, (Time)time);
    event.xfocus.send_event = True;
    event.xfocus.serial = ++LastKnownRequestProcessed(&browserDisplay.display);
    BrowserSyncRequestState(&browserDisplay.display);
    event.xfocus.mode = NotifyNormal;
    event.xfocus.detail = BrowserFocusEndpointDetail(type, oldFocusWindow,
	    (type == FocusOut) ? None : eventTarget);
    if (BrowserSelectedEventTarget(eventTarget, BrowserEventMask(&event), 0)
	    == None) {
	return 0;
    }
    return BrowserQueueEvent(&event);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmQueueWheelEvent(
    unsigned int window,
    int delta,
    unsigned int state,
    int x,
    int y,
    unsigned int time)
{
    XEvent event;
    Window target;
    Window deliveryTarget;
    int rootX = x;
    int rootY = y;

    if (delta == 0) {
	return 0;
    }
    BrowserInitDisplay();
    state = BrowserMappedPointerState(state);
    BrowserClampPointerToGrabConfine(&rootX, &rootY);
    x = rootX;
    y = rootY;
    target = BrowserResolvePointerEventWindow((Window)window, &x, &y);
    BrowserUpdatePointerState(target, rootX, rootY, state);

    BrowserSetCommonEventFields(&event, X11_WASM_BROWSER_WHEEL_EVENT, target,
	    (Time)time);
    event.xbutton.root = browserDisplay.rootWindow;
    event.xbutton.subwindow = None;
    event.xbutton.time = eventTime;
    event.xbutton.x = x;
    event.xbutton.y = y;
    event.xbutton.x_root = rootX;
    event.xbutton.y_root = rootY;
    event.xbutton.state = state;
    event.xbutton.button = (unsigned int)delta;
    event.xbutton.same_screen = True;
    deliveryTarget = BrowserSelectedEventTarget(target,
	    BrowserEventMask(&event), 1);
    if (deliveryTarget == None) {
	return 0;
    }
    if (deliveryTarget != target) {
	target = deliveryTarget;
	BrowserWindowCoordinatesFromRoot(target, rootX, rootY, &x, &y);
	event.xany.window = target;
	event.xbutton.x = x;
	event.xbutton.y = y;
    }
    return BrowserQueueEventInternal(&event, 1);
}

int
BrowserEventMatchesFocusedIC(
    XEvent *eventPtr,
    Window window)
{
    Window target;
    Window focusWindow;
    long eventMask;

    if ((focusedInputContext == NULL) || !focusedInputContext->focused
	    || (eventPtr == NULL)) {
	return 0;
    }
    eventMask = BrowserEventMask(eventPtr);
    if ((eventMask == NoEventMask)
	    || ((focusedInputContext->filterEvents & eventMask) == 0)) {
	return 0;
    }
    target = (window != None) ? window : BrowserEventWindow(eventPtr);
    focusWindow = (focusedInputContext->focusWindow != None)
	    ? focusedInputContext->focusWindow
	    : focusedInputContext->clientWindow;
    if ((target == None) || (focusWindow == None)) {
	return 0;
    }
    return target == focusWindow;
}

int
BrowserFilterEventForIM(
    XEvent *eventPtr,
    Window window)
{
    if ((eventPtr == NULL) || eventPtr->xany.send_event) {
	return 0;
    }
    if (((eventPtr->type == KeyPress) || (eventPtr->type == KeyRelease))
	    && (eventPtr->xkey.state & X11_WASM_IME_CONTROL_STATE)
	    && BrowserEventMatchesFocusedIC(eventPtr, window)) {
	return 1;
    }
    return 0;
}

int
X11WasmServiceEvents(void)
{
    BrowserInitDisplay();
    BrowserUpdateQueueLength();
    return eventQueueCount;
}

int
BrowserDisplayNameIsAccepted(
    const char *displayName)
{
    if ((displayName == NULL) || (displayName[0] == '\0')) {
	return 1;
    }
    return (strcmp(displayName, "browser") == 0)
	    || (strcmp(displayName, ":0") == 0)
	    || (strcmp(displayName, ":0.0") == 0)
	    || (strcmp(displayName, "browser:0") == 0)
	    || (strcmp(displayName, "browser:0.0") == 0);
}

Display *
X11WasmGetDisplay(
    const char *displayName)
{
    if (!BrowserDisplayNameIsAccepted(displayName)) {
	return NULL;
    }
    BrowserInitDisplay();
    return &browserDisplay.display;
}

XID
X11WasmAllocXId(
    Display *display)
{
    (void)display;
    BrowserInitDisplay();
    return browserDisplay.nextXId++;
}

X11WasmResource *
X11WasmFindResource(
    XID id)
{
    X11WasmResource *resourcePtr;

    BrowserInitDisplay();
    for (resourcePtr = browserDisplay.resourceList; resourcePtr != NULL;
	    resourcePtr = resourcePtr->nextPtr) {
	if (resourcePtr->id == id) {
	    return resourcePtr;
	}
    }
    return NULL;
}

X11WasmResource *
X11WasmCreateResource(
    Display *display,
    X11WasmResourceType type)
{
    X11WasmResource *resourcePtr;

    BrowserInitDisplay();
    if (display == NULL) {
	display = &browserDisplay.display;
    }
    resourcePtr = (X11WasmResource *)ckalloc(sizeof(X11WasmResource));
    memset(resourcePtr, 0, sizeof(X11WasmResource));
    resourcePtr->id = X11WasmAllocXId(display);
    resourcePtr->type = type;
    resourcePtr->display = display;
    resourcePtr->clientSerial = browserDisplay.currentClientSerial;
    resourcePtr->width = 1;
    resourcePtr->height = 1;
    resourcePtr->visibilityState = X11_WASM_VISIBILITY_NOT_VIEWABLE;
    resourcePtr->depth = 24;
    resourcePtr->c_class = InputOutput;
    resourcePtr->atts.colormap = browserDisplay.defaultColormap;
    resourcePtr->atts.bit_gravity = NorthWestGravity;
    resourcePtr->atts.win_gravity = NorthWestGravity;
    resourcePtr->atts.backing_store = NotUseful;
    resourcePtr->atts.backing_planes = ~0UL;
    resourcePtr->nextPtr = browserDisplay.resourceList;
    browserDisplay.resourceList = resourcePtr;
    return resourcePtr;
}

void
X11WasmDeleteResource(
    XID id)
{
    X11WasmResource **linkPtr;
    X11WasmResource *aliasPtr;
    Cursor pointerGrabCursorToRelease = None;
    Cursor windowCursorToRelease = None;

    BrowserInitDisplay();
    for (aliasPtr = browserDisplay.resourceList; aliasPtr != NULL;
	    aliasPtr = aliasPtr->nextPtr) {
	if (aliasPtr->metadataTarget == (Window)id) {
	    aliasPtr->metadataTarget = None;
	    aliasPtr->metadataRootToplevelSet = 0;
	    aliasPtr->metadataIsRootToplevel = 0;
	}
    }
    linkPtr = &browserDisplay.resourceList;
    while (*linkPtr != NULL) {
	X11WasmResource *resourcePtr = *linkPtr;
	int pointerStateChanged = 0;

	if (resourcePtr->id == id) {
	    if (resourcePtr == &browserDisplay.rootResource) {
		return;
	    }
	    while (resourcePtr->firstChildPtr != NULL) {
		X11WasmResource *childPtr = resourcePtr->firstChildPtr;

		BrowserUnlinkChild(childPtr);
		childPtr->parent = None;
	    }
	    BrowserUnlinkChild(resourcePtr);
	    if (browserDisplay.focusWindow == (Window)id) {
		browserDisplay.focusWindow = None;
	    }
	    if (browserDisplay.pointerWindow == (Window)id) {
		browserDisplay.pointerWindow = browserDisplay.rootWindow;
		pointerStateChanged = 1;
	    }
	    if (browserDisplay.pointerCaptureWindow == (Window)id) {
		browserDisplay.pointerCaptureWindow = None;
		pointerStateChanged = 1;
	    }
		    if (pointerStateChanged) {
			BrowserSyncPointerState();
		    }
		    if (browserDisplay.pointerGrabWindow == (Window)id) {
			browserDisplay.pointerGrabWindow = None;
			browserDisplay.pointerGrabConfineWindow = None;
			browserDisplay.pointerGrabOwnerEvents = 0;
			browserDisplay.pointerGrabEventMask = 0;
			pointerGrabCursorToRelease = browserDisplay.pointerGrabCursor;
			browserDisplay.pointerGrabCursor = None;
			browserDisplay.pointerGrabPointerMode = GrabModeAsync;
			browserDisplay.pointerGrabKeyboardMode = GrabModeAsync;
			browserDisplay.pointerGrabPassive = 0;
			BrowserSyncPointerGrabCursor();
			BrowserSyncPointerGrabState();
		    } else if (browserDisplay.pointerGrabConfineWindow == (Window)id) {
			browserDisplay.pointerGrabConfineWindow = None;
			BrowserSyncPointerGrabState();
		    }
		    if (browserDisplay.keyboardGrabWindow == (Window)id) {
			browserDisplay.keyboardGrabWindow = None;
			browserDisplay.keyboardGrabOwnerEvents = 0;
			browserDisplay.keyboardGrabPointerMode = GrabModeAsync;
			browserDisplay.keyboardGrabKeyboardMode = GrabModeAsync;
			browserDisplay.keyboardGrabPassive = 0;
			BrowserSyncKeyboardGrabState();
		    }
		    BrowserDeletePassiveGrabsForWindow((Window)id);
		    if (resourcePtr->type == X11_WASM_RESOURCE_WINDOW) {
			windowCursorToRelease = resourcePtr->atts.cursor;
			resourcePtr->atts.cursor = None;
		    }
	    BrowserDeleteSelectionOwnersForWindow((Window)id);
	    BrowserFreePixmapBacking(resourcePtr);
	    if (resourcePtr->bitmapData != NULL) {
		ckfree(resourcePtr->bitmapData);
	    }
	    if (resourcePtr->type == X11_WASM_RESOURCE_CURSOR) {
		BrowserFreeCursorSnapshot(resourcePtr);
	    }
	    if (resourcePtr->type == X11_WASM_RESOURCE_FONT) {
		resourcePtr->fontStruct = NULL;
	    }
	    if (resourcePtr->fontFamily != NULL) {
		ckfree(resourcePtr->fontFamily);
	    }
	    BrowserFreePropertyList(resourcePtr);
	    BrowserFreeColorList(resourcePtr);
	    *linkPtr = resourcePtr->nextPtr;
	    ckfree(resourcePtr);
	    BrowserReleaseCursor(pointerGrabCursorToRelease);
	    BrowserReleaseCursor(windowCursorToRelease);
	    return;
	}
	linkPtr = &resourcePtr->nextPtr;
    }
}

static void
BrowserCloseDownWindowResource(
    X11WasmResource *resourcePtr);

void
BrowserStartNextClient(void)
{
    do {
	browserDisplay.currentClientSerial = browserDisplay.nextClientSerial++;
    } while (browserDisplay.currentClientSerial == 0);
}

static void
BrowserRetainSaveSetWindow(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *oldParentPtr = NULL;
    BrowserVisibilitySnapshot oldVisibilitySnapshot;
    Window oldParent;
    int wasMapped;

    if ((resourcePtr == NULL) || (resourcePtr == &browserDisplay.rootResource)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    while (resourcePtr->firstChildPtr != NULL) {
	X11WasmResource *childPtr = resourcePtr->firstChildPtr;

	if (childPtr->inSaveSet) {
	    BrowserRetainSaveSetWindow(childPtr);
	} else {
	    BrowserCloseDownWindowResource(childPtr);
	}
    }

    oldParent = resourcePtr->parent;
    wasMapped = resourcePtr->mapped;
    if (wasMapped) {
	oldParentPtr = BrowserWindowResource(oldParent);
	BrowserCaptureVisibilitySnapshot(oldParentPtr, &oldVisibilitySnapshot);
	resourcePtr->mapped = 0;
	(void)X11WasmJsUnmapWindow(BrowserJsXId(resourcePtr->id));
	(void)BrowserQueueUnmapNotify(resourcePtr, False);
	BrowserRefreshVisibilityTree(oldParentPtr);
	BrowserQueueNewlyVisibleExposes(oldParentPtr, &oldVisibilitySnapshot,
		(Window)resourcePtr->id);
	BrowserFreeVisibilitySnapshot(&oldVisibilitySnapshot);
    }
    if (oldParent != browserDisplay.rootWindow) {
	BrowserUnlinkChild(resourcePtr);
	resourcePtr->parent = browserDisplay.rootWindow;
	resourcePtr->x = 0;
	resourcePtr->y = 0;
	BrowserLinkChildAtTop(resourcePtr, browserDisplay.rootWindow);
	(void)X11WasmJsReparentWindow(BrowserJsXId(resourcePtr->id),
		BrowserJsXId(browserDisplay.rootWindow), 0, 0);
	if (BrowserWindowInheritsStyle(resourcePtr)) {
	    BrowserSyncWindowStyle(resourcePtr);
	    BrowserSyncInheritedChildStyles(resourcePtr);
	}
	BrowserSyncTransientStacking();
	(void)BrowserQueueReparentNotify(resourcePtr, oldParent,
		browserDisplay.rootWindow, 0, 0);
    }
    resourcePtr->retained = 1;
    resourcePtr->temporary = 0;
    resourcePtr->inSaveSet = 0;
    resourcePtr->clientSerial = 0;
    if (!resourcePtr->mapped) {
	resourcePtr->mapped = 1;
	(void)X11WasmJsMapWindow(BrowserJsXId(resourcePtr->id));
	(void)BrowserQueueMapNotify(resourcePtr);
	BrowserRefreshVisibilityTree(BrowserWindowResource(resourcePtr->parent));
	BrowserQueueViewableExposeTree(resourcePtr);
    }
}

static void
BrowserCloseDownWindowResource(
    X11WasmResource *resourcePtr)
{
    if ((resourcePtr == NULL) || (resourcePtr == &browserDisplay.rootResource)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    if (resourcePtr->inSaveSet) {
	BrowserRetainSaveSetWindow(resourcePtr);
	return;
    }
    while (resourcePtr->firstChildPtr != NULL) {
	X11WasmResource *childPtr = resourcePtr->firstChildPtr;

	if (childPtr->inSaveSet) {
	    BrowserRetainSaveSetWindow(childPtr);
	} else {
	    BrowserCloseDownWindowResource(childPtr);
	}
    }
    (void)BrowserDestroyWindowResource(resourcePtr);
}

static void
BrowserDeleteCloseDownResource(
    X11WasmResource *resourcePtr)
{
    XID id;

    if ((resourcePtr == NULL) || (resourcePtr == &browserDisplay.rootResource)) {
	return;
    }
    id = resourcePtr->id;
    if (resourcePtr->type == X11_WASM_RESOURCE_WINDOW) {
	BrowserCloseDownWindowResource(resourcePtr);
	return;
    }
    if ((resourcePtr->type == X11_WASM_RESOURCE_PIXMAP)
	    || (resourcePtr->type == X11_WASM_RESOURCE_CURSOR)) {
	(void)X11WasmJsDestroyResource(BrowserJsXId(id));
    }
    X11WasmDeleteResource(id);
}

void
BrowserDeleteTemporaryResources(void)
{
    X11WasmResource *resourcePtr;
    X11WasmTrackedGC *gcPtr;

    for (;;) {
	for (gcPtr = gcList; gcPtr != NULL; gcPtr = gcPtr->nextPtr) {
	    if (gcPtr->retained && gcPtr->temporary) {
		break;
	    }
	}
	if (gcPtr != NULL) {
	    (void)BrowserDeleteRegisteredGC(gcPtr->gc);
	    continue;
	}
	for (resourcePtr = browserDisplay.resourceList; resourcePtr != NULL;
		resourcePtr = resourcePtr->nextPtr) {
	    if ((resourcePtr != &browserDisplay.rootResource)
		    && resourcePtr->retained && resourcePtr->temporary) {
		break;
	    }
	}
	if (resourcePtr == NULL) {
	    return;
	}
	if (resourcePtr->inSaveSet
		&& (resourcePtr->type == X11_WASM_RESOURCE_WINDOW)) {
	    BrowserRetainSaveSetWindow(resourcePtr);
	} else {
	    BrowserDeleteCloseDownResource(resourcePtr);
	}
    }
}

void
BrowserDeleteClientResources(
    unsigned long clientSerial)
{
    X11WasmResource *resourcePtr;
    X11WasmTrackedGC *gcPtr;

    if (clientSerial == 0) {
	return;
    }
    for (;;) {
	for (gcPtr = gcList; gcPtr != NULL; gcPtr = gcPtr->nextPtr) {
	    if (gcPtr->clientSerial == clientSerial) {
		break;
	    }
	}
	if (gcPtr != NULL) {
	    (void)BrowserDeleteRegisteredGC(gcPtr->gc);
	    continue;
	}
	for (resourcePtr = browserDisplay.resourceList; resourcePtr != NULL;
		resourcePtr = resourcePtr->nextPtr) {
	    if ((resourcePtr != &browserDisplay.rootResource)
		    && (resourcePtr->clientSerial == clientSerial)) {
		break;
	    }
	}
	if (resourcePtr == NULL) {
	    return;
	}
	if (resourcePtr->inSaveSet
		&& (resourcePtr->type == X11_WASM_RESOURCE_WINDOW)) {
	    BrowserRetainSaveSetWindow(resourcePtr);
	} else {
	    BrowserDeleteCloseDownResource(resourcePtr);
	}
    }
}

void
BrowserApplyCloseDownMode(void)
{
    X11WasmResource *resourcePtr;
    X11WasmTrackedGC *gcPtr;

    if (browserDisplay.closeDownMode == DestroyAll) {
	BrowserDeleteClientResources(browserDisplay.currentClientSerial);
	return;
    }

    for (resourcePtr = browserDisplay.resourceList; resourcePtr != NULL;
	    resourcePtr = resourcePtr->nextPtr) {
	if ((resourcePtr != &browserDisplay.rootResource)
		&& !resourcePtr->retained
		&& (resourcePtr->clientSerial == browserDisplay.currentClientSerial)) {
	    resourcePtr->retained = 1;
	    resourcePtr->temporary =
		    browserDisplay.closeDownMode == RetainTemporary;
	}
    }
    for (gcPtr = gcList; gcPtr != NULL; gcPtr = gcPtr->nextPtr) {
	if (!gcPtr->retained
		&& (gcPtr->clientSerial == browserDisplay.currentClientSerial)) {
	    gcPtr->retained = 1;
	    gcPtr->temporary = browserDisplay.closeDownMode == RetainTemporary;
	}
    }
}

void
X11WasmSetPixmapColormap(
    Pixmap pixmap,
    Colormap colormap)
{
    X11WasmResource *pixmapPtr;

    BrowserInitDisplay();
    pixmapPtr = X11WasmFindResource((XID)pixmap);
    if ((pixmapPtr == NULL)
	    || (pixmapPtr->type != X11_WASM_RESOURCE_PIXMAP)) {
	return;
    }
    if ((colormap != None) && (colormap != browserDisplay.defaultColormap)) {
	X11WasmResource *colormapPtr = X11WasmFindResource((XID)colormap);

	if ((colormapPtr == NULL)
		|| (colormapPtr->type != X11_WASM_RESOURCE_COLORMAP)) {
	    return;
	}
    }
    pixmapPtr->atts.colormap = (colormap != None)
	    ? colormap : browserDisplay.defaultColormap;
}
