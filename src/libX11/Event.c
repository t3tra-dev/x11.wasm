#include "XlibInt.h"

typedef struct BrowserTypedEventMatch {
    Window window;
    int type;
    int matchWindow;
} BrowserTypedEventMatch;

typedef struct BrowserUserEventPredicate {
    Bool (*predicate)(Display *display, XEvent *eventPtr, XPointer arg);
    XPointer arg;
} BrowserUserEventPredicate;

struct X11WasmPassiveButtonGrab {
    Window window;
    unsigned int button;
    unsigned int modifiers;
    int ownerEvents;
    unsigned int eventMask;
    int pointerMode;
    int keyboardMode;
    Window confineTo;
    Cursor cursor;
    X11WasmPassiveButtonGrab *nextPtr;
};

struct X11WasmPassiveKeyGrab {
    Window window;
    int keycode;
    unsigned int modifiers;
    int ownerEvents;
    int pointerMode;
    int keyboardMode;
    X11WasmPassiveKeyGrab *nextPtr;
};

static int
BrowserKeyboardPercentIsValid(
    int value)
{
    return (value == -1) || ((value >= 0) && (value <= 100));
}

static int
BrowserKeyboardAutoRepeatModeIsValid(
    int mode)
{
    return (mode == AutoRepeatModeOff) || (mode == AutoRepeatModeOn)
	    || (mode == AutoRepeatModeDefault);
}

static unsigned long
BrowserScreenSaverIdle(void)
{
    int idle = X11WasmJsGetUserIdleTime();

    return (idle > 0) ? (unsigned long)idle : 0UL;
}

static int
BrowserScreenSaverEventMaskIsValid(
    unsigned long eventMask)
{
    const unsigned long allowedMask = ScreenSaverNotifyMask
	    | ScreenSaverCycleMask;

    return (eventMask & ~allowedMask) == 0UL;
}

static int
BrowserQueueScreenSaverNotify(
    int state,
    int forced)
{
    XEvent event;
    XScreenSaverNotifyEvent *notifyPtr;

    if ((browserDisplay.screenSaverEventMask & ScreenSaverNotifyMask) == 0) {
	return 0;
    }
    BrowserSetCommonEventFields(&event,
	    X11_WASM_SCREENSAVER_EVENT_BASE + ScreenSaverNotify,
	    browserDisplay.rootWindow, CurrentTime);
    notifyPtr = (XScreenSaverNotifyEvent *)&event;
    notifyPtr->root = browserDisplay.rootWindow;
    notifyPtr->state = state;
    notifyPtr->kind = ScreenSaverInternal;
    notifyPtr->forced = forced ? True : False;
    notifyPtr->time = eventTime;
    return BrowserQueueEvent(&event);
}

static void
BrowserForceScreenSaverMode(
    int mode)
{
    unsigned long idle = BrowserScreenSaverIdle();
    int wasActive = browserDisplay.screenSaverActive;

    if (mode == ScreenSaverActive) {
	if (!browserDisplay.screenSaverActive) {
	    browserDisplay.screenSaverActivatedIdle = idle;
	}
	browserDisplay.screenSaverActive = 1;
	if (!wasActive) {
	    (void)BrowserQueueScreenSaverNotify(ScreenSaverOn, True);
	}
    } else {
	browserDisplay.screenSaverActive = 0;
	browserDisplay.screenSaverActivatedIdle = 0UL;
	browserDisplay.screenSaverTimerResetIdle = idle;
	if (wasActive) {
	    (void)BrowserQueueScreenSaverNotify(ScreenSaverOff, True);
	}
    }
    (void)X11WasmJsForceScreenSaver(mode);
}

void
BrowserResetScreenSaverForInput(void)
{
    int wasActive = browserDisplay.screenSaverActive;

    browserDisplay.screenSaverActive = 0;
    browserDisplay.screenSaverActivatedIdle = 0UL;
    browserDisplay.screenSaverTimerResetIdle = BrowserScreenSaverIdle();
    if (wasActive) {
	(void)BrowserQueueScreenSaverNotify(ScreenSaverOff, False);
	(void)X11WasmJsForceScreenSaver(ScreenSaverReset);
    }
}

static void
BrowserSetKeyBit(
    char bits[32],
    unsigned int keycode,
    int enabled)
{
    unsigned int index;
    unsigned char mask;

    if (keycode > X11_WASM_MAX_KEYCODE) {
	return;
    }
    index = keycode >> 3;
    mask = (unsigned char)(1U << (keycode & 7U));
    if (enabled) {
	bits[index] = (char)((unsigned char)bits[index] | mask);
    } else {
	bits[index] = (char)((unsigned char)bits[index] & ~mask);
    }
}

static unsigned int
BrowserPassiveGrabModifierMask(void)
{
    return ShiftMask | LockMask | ControlMask | Mod1Mask | Mod2Mask
	    | Mod3Mask | Mod4Mask | Mod5Mask;
}

static int
BrowserPassiveGrabModifiersMatch(
    unsigned int grabModifiers,
    unsigned int state)
{
    if (grabModifiers == AnyModifier) {
	return 1;
    }
    return (state & BrowserPassiveGrabModifierMask()) == grabModifiers;
}

static int
BrowserPassiveGrabWindowMatches(
    Window eventWindow,
    Window grabWindow)
{
    return (eventWindow == grabWindow)
	    || BrowserWindowInTree(eventWindow, grabWindow);
}

static void
BrowserFreePassiveButtonGrab(
    X11WasmPassiveButtonGrab *grabPtr)
{
    if (grabPtr == NULL) {
	return;
    }
    BrowserReleaseCursor(grabPtr->cursor);
    ckfree(grabPtr);
}

static void
BrowserFreePassiveKeyGrab(
    X11WasmPassiveKeyGrab *grabPtr)
{
    if (grabPtr != NULL) {
	ckfree(grabPtr);
    }
}

static void
BrowserDeactivatePointerGrab(void)
{
    browserDisplay.pointerGrabWindow = None;
    browserDisplay.pointerGrabConfineWindow = None;
    browserDisplay.pointerGrabOwnerEvents = 0;
    browserDisplay.pointerGrabEventMask = 0;
    BrowserSetPointerGrabCursor(None);
    browserDisplay.pointerGrabPointerMode = GrabModeAsync;
    browserDisplay.pointerGrabKeyboardMode = GrabModeAsync;
    browserDisplay.pointerGrabPassive = 0;
    BrowserSyncPointerGrabCursor();
    BrowserSyncPointerGrabState();
}

static void
BrowserDeactivateKeyboardGrab(void)
{
    browserDisplay.keyboardGrabWindow = None;
    browserDisplay.keyboardGrabOwnerEvents = 0;
    browserDisplay.keyboardGrabPointerMode = GrabModeAsync;
    browserDisplay.keyboardGrabKeyboardMode = GrabModeAsync;
    browserDisplay.keyboardGrabPassive = 0;
    BrowserSyncKeyboardGrabState();
}

static X11WasmPassiveButtonGrab *
BrowserFindPassiveButtonGrab(
    Window eventWindow,
    unsigned int button,
    unsigned int state)
{
    X11WasmPassiveButtonGrab *grabPtr;

    if (!BrowserWindowCanReceiveEvents(eventWindow)) {
	return NULL;
    }
    for (grabPtr = browserDisplay.passiveButtonGrabs; grabPtr != NULL;
	    grabPtr = grabPtr->nextPtr) {
	if (!BrowserWindowCanReceiveEvents(grabPtr->window)
		|| ((grabPtr->button != AnyButton)
		&& (grabPtr->button != button))
		|| !BrowserPassiveGrabModifiersMatch(grabPtr->modifiers, state)
		|| !BrowserPassiveGrabWindowMatches(eventWindow,
			grabPtr->window)) {
	    continue;
	}
	return grabPtr;
    }
    return NULL;
}

static X11WasmPassiveKeyGrab *
BrowserFindPassiveKeyGrab(
    Window eventWindow,
    unsigned int keycode,
    unsigned int state)
{
    X11WasmPassiveKeyGrab *grabPtr;

    if (!BrowserWindowCanReceiveEvents(eventWindow)) {
	return NULL;
    }
    for (grabPtr = browserDisplay.passiveKeyGrabs; grabPtr != NULL;
	    grabPtr = grabPtr->nextPtr) {
	if (!BrowserWindowCanReceiveEvents(grabPtr->window)
		|| ((grabPtr->keycode != AnyKey)
		&& ((unsigned int)grabPtr->keycode != keycode))
		|| !BrowserPassiveGrabModifiersMatch(grabPtr->modifiers, state)
		|| !BrowserPassiveGrabWindowMatches(eventWindow,
			grabPtr->window)) {
	    continue;
	}
	return grabPtr;
    }
    return NULL;
}

static int
BrowserTypedQueuedEvent(
    Display *display,
    XEvent *eventPtr,
    XPointer arg)
{
    BrowserTypedEventMatch *matchPtr = (BrowserTypedEventMatch *)arg;

    (void)display;
    if ((eventPtr == NULL) || (matchPtr == NULL)) {
	return 0;
    }
    if (eventPtr->type != matchPtr->type) {
	return 0;
    }
    if (matchPtr->matchWindow
	    && (BrowserEventWindow(eventPtr) != matchPtr->window)) {
	return 0;
    }
    return 1;
}

static int
BrowserUserQueuedEvent(
    Display *display,
    XEvent *eventPtr,
    XPointer arg)
{
    BrowserUserEventPredicate *predicatePtr =
	    (BrowserUserEventPredicate *)arg;

    if ((predicatePtr == NULL) || (predicatePtr->predicate == NULL)) {
	return 0;
    }
    return predicatePtr->predicate(display, eventPtr, predicatePtr->arg)
	    ? 1 : 0;
}

static int
BrowserTakeQueuedEventAfterPump(
    Display *display,
    X11WasmQueuedEventPredicate predicate,
    XPointer arg,
    int remove,
    XEvent *event)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (BrowserTakeQueuedEvent(display, predicate, arg, remove, event,
	    NULL)) {
	return 1;
    }
    (void)XFlush(display);
    return BrowserTakeQueuedEvent(display, predicate, arg, remove, event,
	    NULL);
}

EMSCRIPTEN_KEEPALIVE int
XEventsQueued(
    Display *display,
    int mode)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (mode == QueuedAfterFlush) {
	(void)XFlush(display);
    }
    BrowserUpdateQueueLength();
    return eventQueueCount;
}

int
XPending(
    Display *display)
{
    return XEventsQueued(display, QueuedAfterFlush);
}

int
BrowserPopEventAfterPump(
    Display *display,
    XEvent *event)
{
    if (BrowserPopEvent(event, NULL)) {
	return 1;
    }
    (void)XFlush(display);
    return BrowserPopEvent(event, NULL);
}

int
BrowserPopMatchingEventAfterPump(
    Display *display,
    Window window,
    long mask,
    XEvent *event)
{
    if (BrowserPopMatchingEvent(window, mask, event)) {
	return 1;
    }
    (void)XFlush(display);
    return BrowserPopMatchingEvent(window, mask, event);
}

int
XPeekEvent(
    Display *display,
    XEvent *event)
{
    (void)BrowserTakeQueuedEventAfterPump(display, NULL, NULL, 0, event);
    return Success;
}

int
XNextEvent(
    Display *display,
    XEvent *event)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    (void)BrowserPopEventAfterPump(display, event);
    return Success;
}

int
XMaskEvent(
    Display *display,
    long mask,
    XEvent *event)
{
    long match[2];

    match[0] = (long)None;
    match[1] = mask;
    (void)BrowserTakeQueuedEventAfterPump(display, BrowserWindowMaskQueuedEvent,
	    (XPointer)match, 1, event);
    return Success;
}

Bool
XCheckMaskEvent(
    Display *display,
    long mask,
    XEvent *event)
{
    long match[2];

    match[0] = (long)None;
    match[1] = mask;
    return BrowserTakeQueuedEventAfterPump(display,
	    BrowserWindowMaskQueuedEvent, (XPointer)match, 1, event)
	    ? True : False;
}

Bool
XCheckWindowEvent(
    Display *display,
    Window window,
    long mask,
    XEvent *event)
{
    long match[2];

    match[0] = (long)window;
    match[1] = mask;
    return BrowserTakeQueuedEventAfterPump(display,
	    BrowserWindowMaskQueuedEvent, (XPointer)match, 1, event)
	    ? True : False;
}

Bool
XCheckTypedEvent(
    Display *display,
    int eventType,
    XEvent *event)
{
    BrowserTypedEventMatch match;

    match.window = None;
    match.type = eventType;
    match.matchWindow = 0;
    return BrowserTakeQueuedEventAfterPump(display, BrowserTypedQueuedEvent,
	    (XPointer)&match, 1, event) ? True : False;
}

Bool
XCheckTypedWindowEvent(
    Display *display,
    Window window,
    int eventType,
    XEvent *event)
{
    BrowserTypedEventMatch match;

    match.window = window;
    match.type = eventType;
    match.matchWindow = 1;
    return BrowserTakeQueuedEventAfterPump(display, BrowserTypedQueuedEvent,
	    (XPointer)&match, 1, event) ? True : False;
}

int
XIfEvent(
    Display *display,
    XEvent *event,
    Bool (*predicate)(Display *display, XEvent *eventPtr, XPointer arg),
    XPointer arg)
{
    BrowserUserEventPredicate userPredicate;

    userPredicate.predicate = predicate;
    userPredicate.arg = arg;
    (void)BrowserTakeQueuedEventAfterPump(display, BrowserUserQueuedEvent,
	    (XPointer)&userPredicate, 1, event);
    return Success;
}

Bool
XCheckIfEvent(
    Display *display,
    XEvent *event,
    Bool (*predicate)(Display *display, XEvent *eventPtr, XPointer arg),
    XPointer arg)
{
    BrowserUserEventPredicate userPredicate;

    userPredicate.predicate = predicate;
    userPredicate.arg = arg;
    return BrowserTakeQueuedEventAfterPump(display, BrowserUserQueuedEvent,
	    (XPointer)&userPredicate, 1, event) ? True : False;
}

int
XPeekIfEvent(
    Display *display,
    XEvent *event,
    Bool (*predicate)(Display *display, XEvent *eventPtr, XPointer arg),
    XPointer arg)
{
    BrowserUserEventPredicate userPredicate;

    userPredicate.predicate = predicate;
    userPredicate.arg = arg;
    (void)BrowserTakeQueuedEventAfterPump(display, BrowserUserQueuedEvent,
	    (XPointer)&userPredicate, 0, event);
    return Success;
}

int
XPutBackEvent(
    Display *display,
    XEvent *event)
{
    (void)display;
    BrowserInitDisplay();
    if ((event == NULL) || (eventQueueCount >= X11_WASM_EVENT_QUEUE_LIMIT)) {
	return Success;
    }
    eventQueueHead = (eventQueueHead + X11_WASM_EVENT_QUEUE_LIMIT - 1)
	    % X11_WASM_EVENT_QUEUE_LIMIT;
    eventQueue[eventQueueHead].event = *event;
    eventQueue[eventQueueHead].internal = 0;
    eventQueueCount++;
    BrowserUpdateQueueLength();
    return Success;
}

int
XWindowEvent(
    Display *display,
    Window window,
    long mask,
    XEvent *event)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    (void)BrowserPopMatchingEventAfterPump(display, window, mask, event);
    return Success;
}

Status
XSendEvent(
    Display *display,
    Window window,
    Bool propagate,
    long mask,
    XEvent *event)
{
    XEvent copy;
    Window target;
    X11WasmResource *resourcePtr;

    if (event == NULL) {
	return False;
    }
    if (((event->type < KeyPress) || (event->type >= LASTEvent))
	    && (event->type != X11_WASM_BROWSER_WHEEL_EVENT)) {
	if (display == NULL) {
	    display = X11WasmGetDisplay(NULL);
	}
	(void)BrowserReportXError(display, BadValue, (XID)event->type,
		X11_WASM_XREQ_SEND_EVENT, 0);
	return False;
    }
    if (!BrowserValidateBool(&display, propagate,
	    X11_WASM_XREQ_SEND_EVENT)) {
	return False;
    }
    if ((window != PointerWindow) && (window != InputFocus)) {
	resourcePtr = BrowserWindowResourceOrError(&display, window,
		X11_WASM_XREQ_SEND_EVENT);
	if (resourcePtr == NULL) {
	    return False;
	}
	target = BrowserSendEventTarget((Window)resourcePtr->id, propagate,
		mask, event);
    } else {
	if (display == NULL) {
	    display = X11WasmGetDisplay(NULL);
	}
	target = BrowserSendEventTarget(window, propagate, mask, event);
    }
    if (target == None) {
	return False;
    }
    if ((event->type == ClientMessage)
	    && (event->xclient.message_type == BrowserNetWmStateAtom())) {
	BrowserApplyNetWmStateClientMessage(&event->xclient);
    }
    copy = *event;
    copy.xany.window = target;
    copy.xany.display = display;
    copy.xany.send_event = True;
    return BrowserQueueEvent(&copy) ? True : False;
}
Bool
XFilterEvent(
    XEvent *event,
    Window window)
{
    return BrowserFilterEventForIM(event, window) ? True : False;
}

Bool
XQueryPointer(
    Display *display,
    Window window,
    Window *rootReturn,
    Window *childReturn,
    int *rootXReturn,
    int *rootYReturn,
    int *winXReturn,
    int *winYReturn,
    unsigned int *maskReturn)
{
    X11WasmResource *resourcePtr;
    X11WasmResource *childPtr = NULL;
    int winX = browserDisplay.pointerX;
    int winY = browserDisplay.pointerY;

    if (rootReturn) *rootReturn = None;
    if (childReturn) *childReturn = None;
    if (rootXReturn) *rootXReturn = 0;
    if (rootYReturn) *rootYReturn = 0;
    if (winXReturn) *winXReturn = 0;
    if (winYReturn) *winYReturn = 0;
    if (maskReturn) *maskReturn = 0;

    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_QUERY_POINTER);
    if (resourcePtr == NULL) {
	return False;
    }
    BrowserWindowCoordinatesFromRoot(window, browserDisplay.pointerX,
	    browserDisplay.pointerY, &winX, &winY);
    childPtr = BrowserFindTopVisibleChildAt(resourcePtr, winX, winY);
    if (rootReturn) *rootReturn = RootWindow(display, DefaultScreen(display));
    if (childReturn) {
	*childReturn = (childPtr != NULL) ? (Window)childPtr->id : None;
    }
    if (rootXReturn) *rootXReturn = browserDisplay.pointerX;
    if (rootYReturn) *rootYReturn = browserDisplay.pointerY;
    if (winXReturn) *winXReturn = winX;
    if (winYReturn) *winYReturn = winY;
    if (maskReturn) *maskReturn = browserDisplay.pointerState;
    return True;
}

int
XGetInputFocus(
    Display *display,
    Window *focusReturn,
    int *revertReturn)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (focusReturn) {
	*focusReturn = (browserDisplay.focusWindow != None)
		? browserDisplay.focusWindow
		: RootWindow(display, DefaultScreen(display));
    }
    if (revertReturn) *revertReturn = browserDisplay.focusRevertTo;
    return Success;
}

int
BrowserQueueRawFocusEvent(
    Window window,
    int type,
    Time time)
{
    XEvent event;

    if (!BrowserWindowCanReceiveEvents(window)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, type, window, time);
    event.xfocus.serial = ++LastKnownRequestProcessed(&browserDisplay.display);
    BrowserSyncRequestState(&browserDisplay.display);
    event.xfocus.mode = NotifyNormal;
    event.xfocus.detail = NotifyNonlinear;
    return BrowserQueueEvent(&event);
}

int
BrowserGrabModeIsValid(
    int mode)
{
    return (mode == GrabModeSync) || (mode == GrabModeAsync);
}

int
BrowserPointerGrabEventMaskIsValid(
    unsigned int eventMask)
{
    const unsigned int allowedMask = ButtonPressMask | ButtonReleaseMask
	    | EnterWindowMask | LeaveWindowMask | PointerMotionMask
	    | PointerMotionHintMask | Button1MotionMask | Button2MotionMask
	    | Button3MotionMask | Button4MotionMask | Button5MotionMask
	    | ButtonMotionMask | KeymapStateMask;

    return (eventMask & ~allowedMask) == 0;
}

static int
BrowserPassiveGrabButtonIsValid(
    unsigned int button)
{
    return button <= 255U;
}

static int
BrowserPassiveGrabKeycodeIsValid(
    Display *display,
    int keycode)
{
    return (keycode == AnyKey)
	    || ((display != NULL) && (keycode >= display->min_keycode)
	    && (keycode <= display->max_keycode));
}

static int
BrowserPassiveGrabModifierMaskIsValid(
    unsigned int modifiers)
{
    const unsigned int modifierMask = ShiftMask | LockMask | ControlMask
	    | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask;

    return (modifiers == AnyModifier)
	    || ((modifiers & ~(modifierMask)) == 0);
}

static int
BrowserAllowEventsModeIsValid(
    int mode)
{
    return (mode == AsyncPointer) || (mode == SyncPointer)
	    || (mode == ReplayPointer) || (mode == AsyncKeyboard)
	    || (mode == SyncKeyboard) || (mode == ReplayKeyboard)
	    || (mode == AsyncBoth) || (mode == SyncBoth);
}

static int
BrowserAllowEventsAffectsPointer(
    int mode)
{
    return (mode == AsyncPointer) || (mode == SyncPointer)
	    || (mode == ReplayPointer) || (mode == AsyncBoth)
	    || (mode == SyncBoth);
}

static int
BrowserAllowEventsAffectsKeyboard(
    int mode)
{
    return (mode == AsyncKeyboard) || (mode == SyncKeyboard)
	    || (mode == ReplayKeyboard) || (mode == AsyncBoth)
	    || (mode == SyncBoth);
}

static int
BrowserAllowEventsTimeIsValid(
    int mode,
    Time time)
{
    return (!BrowserAllowEventsAffectsPointer(mode)
	    || BrowserRequestTimeIsValid(time, browserDisplay.pointerGrabTime))
	    && (!BrowserAllowEventsAffectsKeyboard(mode)
	    || BrowserRequestTimeIsValid(time, browserDisplay.keyboardGrabTime));
}

static X11WasmPassiveButtonGrab *
BrowserFindExactPassiveButtonGrab(
    Window window,
    unsigned int button,
    unsigned int modifiers)
{
    X11WasmPassiveButtonGrab *grabPtr;

    for (grabPtr = browserDisplay.passiveButtonGrabs; grabPtr != NULL;
	    grabPtr = grabPtr->nextPtr) {
	if ((grabPtr->window == window) && (grabPtr->button == button)
		&& (grabPtr->modifiers == modifiers)) {
	    return grabPtr;
	}
    }
    return NULL;
}

static X11WasmPassiveKeyGrab *
BrowserFindExactPassiveKeyGrab(
    Window window,
    int keycode,
    unsigned int modifiers)
{
    X11WasmPassiveKeyGrab *grabPtr;

    for (grabPtr = browserDisplay.passiveKeyGrabs; grabPtr != NULL;
	    grabPtr = grabPtr->nextPtr) {
	if ((grabPtr->window == window) && (grabPtr->keycode == keycode)
		&& (grabPtr->modifiers == modifiers)) {
	    return grabPtr;
	}
    }
    return NULL;
}

static void
BrowserInstallExactPassiveButtonGrab(
    Window window,
    unsigned int button,
    unsigned int modifiers,
    Bool ownerEvents,
    unsigned int eventMask,
    int pointerMode,
    int keyboardMode,
    Window confineTo,
    Cursor cursor)
{
    X11WasmPassiveButtonGrab *grabPtr =
	    BrowserFindExactPassiveButtonGrab(window, button, modifiers);

    if (grabPtr == NULL) {
	grabPtr = (X11WasmPassiveButtonGrab *)ckalloc(sizeof(*grabPtr));
	memset(grabPtr, 0, sizeof(*grabPtr));
	grabPtr->window = window;
	grabPtr->button = button;
	grabPtr->modifiers = modifiers;
	grabPtr->nextPtr = browserDisplay.passiveButtonGrabs;
	browserDisplay.passiveButtonGrabs = grabPtr;
    }
    grabPtr->ownerEvents = ownerEvents ? 1 : 0;
    grabPtr->eventMask = eventMask;
    grabPtr->pointerMode = pointerMode;
    grabPtr->keyboardMode = keyboardMode;
    grabPtr->confineTo = confineTo;
    if (grabPtr->cursor != cursor) {
	Cursor oldCursor = grabPtr->cursor;

	BrowserRetainCursor(cursor);
	grabPtr->cursor = cursor;
	BrowserReleaseCursor(oldCursor);
    }
}

static void
BrowserInstallPassiveButtonGrab(
    Window window,
    unsigned int button,
    unsigned int modifiers,
    Bool ownerEvents,
    unsigned int eventMask,
    int pointerMode,
    int keyboardMode,
    Window confineTo,
    Cursor cursor)
{
    unsigned int buttonValue;
    unsigned int firstButton = (button == AnyButton) ? 1U : button;
    unsigned int lastButton = (button == AnyButton) ? 255U : button;
    unsigned int modifierMask = BrowserPassiveGrabModifierMask();

    for (buttonValue = firstButton; buttonValue <= lastButton;
	    buttonValue++) {
	if (modifiers == AnyModifier) {
	    unsigned int modifierValue;

	    for (modifierValue = 0; modifierValue <= modifierMask;
		    modifierValue++) {
		if ((modifierValue & ~modifierMask) != 0) {
		    continue;
		}
		BrowserInstallExactPassiveButtonGrab(window, buttonValue,
			modifierValue, ownerEvents, eventMask, pointerMode,
			keyboardMode, confineTo, cursor);
	    }
	} else {
	    BrowserInstallExactPassiveButtonGrab(window, buttonValue,
		    modifiers, ownerEvents, eventMask, pointerMode,
		    keyboardMode, confineTo, cursor);
	}
    }
}

static void
BrowserInstallExactPassiveKeyGrab(
    Window window,
    int keycode,
    unsigned int modifiers,
    Bool ownerEvents,
    int pointerMode,
    int keyboardMode)
{
    X11WasmPassiveKeyGrab *grabPtr =
	    BrowserFindExactPassiveKeyGrab(window, keycode, modifiers);

    if (grabPtr == NULL) {
	grabPtr = (X11WasmPassiveKeyGrab *)ckalloc(sizeof(*grabPtr));
	memset(grabPtr, 0, sizeof(*grabPtr));
	grabPtr->window = window;
	grabPtr->keycode = keycode;
	grabPtr->modifiers = modifiers;
	grabPtr->nextPtr = browserDisplay.passiveKeyGrabs;
	browserDisplay.passiveKeyGrabs = grabPtr;
    }
    grabPtr->ownerEvents = ownerEvents ? 1 : 0;
    grabPtr->pointerMode = pointerMode;
    grabPtr->keyboardMode = keyboardMode;
}

static void
BrowserInstallPassiveKeyGrab(
    Display *display,
    Window window,
    int keycode,
    unsigned int modifiers,
    Bool ownerEvents,
    int pointerMode,
    int keyboardMode)
{
    int keycodeValue;
    int firstKeycode = (keycode == AnyKey) ? display->min_keycode : keycode;
    int lastKeycode = (keycode == AnyKey) ? display->max_keycode : keycode;
    unsigned int modifierMask = BrowserPassiveGrabModifierMask();

    for (keycodeValue = firstKeycode; keycodeValue <= lastKeycode;
	    keycodeValue++) {
	if (modifiers == AnyModifier) {
	    unsigned int modifierValue;

	    for (modifierValue = 0; modifierValue <= modifierMask;
		    modifierValue++) {
		if ((modifierValue & ~modifierMask) != 0) {
		    continue;
		}
		BrowserInstallExactPassiveKeyGrab(window, keycodeValue,
			modifierValue, ownerEvents, pointerMode, keyboardMode);
	    }
	} else {
	    BrowserInstallExactPassiveKeyGrab(window, keycodeValue,
		    modifiers, ownerEvents, pointerMode, keyboardMode);
	}
    }
}

static int
BrowserPassiveButtonGrabMatchesUngrab(
    const X11WasmPassiveButtonGrab *grabPtr,
    Window window,
    unsigned int button,
    unsigned int modifiers)
{
    return (grabPtr != NULL) && (grabPtr->window == window)
	    && ((button == AnyButton) || (grabPtr->button == button)
		    || (grabPtr->button == AnyButton))
	    && ((modifiers == AnyModifier) || (grabPtr->modifiers == modifiers)
		    || (grabPtr->modifiers == AnyModifier));
}

static int
BrowserPassiveKeyGrabMatchesUngrab(
    const X11WasmPassiveKeyGrab *grabPtr,
    Window window,
    int keycode,
    unsigned int modifiers)
{
    return (grabPtr != NULL) && (grabPtr->window == window)
	    && ((keycode == AnyKey) || (grabPtr->keycode == keycode)
		    || (grabPtr->keycode == AnyKey))
	    && ((modifiers == AnyModifier) || (grabPtr->modifiers == modifiers)
		    || (grabPtr->modifiers == AnyModifier));
}

static void
BrowserRemovePassiveButtonGrabs(
    Window window,
    unsigned int button,
    unsigned int modifiers)
{
    X11WasmPassiveButtonGrab **linkPtr = &browserDisplay.passiveButtonGrabs;

    while (*linkPtr != NULL) {
	X11WasmPassiveButtonGrab *grabPtr = *linkPtr;

	if (BrowserPassiveButtonGrabMatchesUngrab(grabPtr, window, button,
		modifiers)) {
	    *linkPtr = grabPtr->nextPtr;
	    BrowserFreePassiveButtonGrab(grabPtr);
	    continue;
	}
	linkPtr = &grabPtr->nextPtr;
    }
}

static void
BrowserRemovePassiveKeyGrabs(
    Window window,
    int keycode,
    unsigned int modifiers)
{
    X11WasmPassiveKeyGrab **linkPtr = &browserDisplay.passiveKeyGrabs;

    while (*linkPtr != NULL) {
	X11WasmPassiveKeyGrab *grabPtr = *linkPtr;

	if (BrowserPassiveKeyGrabMatchesUngrab(grabPtr, window, keycode,
		modifiers)) {
	    *linkPtr = grabPtr->nextPtr;
	    BrowserFreePassiveKeyGrab(grabPtr);
	    continue;
	}
	linkPtr = &grabPtr->nextPtr;
    }
}

int
BrowserRequestTimeIsValid(
    Time time,
    Time lastChangeTime)
{
    if (time == CurrentTime) {
	return 1;
    }
    if ((lastChangeTime != CurrentTime) && (time < lastChangeTime)) {
	return 0;
    }
    if (time > eventTime) {
	return 0;
    }
    return 1;
}

Time
BrowserRequestEffectiveTime(
    Time time)
{
    return (time == CurrentTime) ? eventTime : time;
}

void
BrowserRecordFocusChangeTime(
    Time time)
{
    if (BrowserRequestTimeIsValid(time, browserDisplay.focusTime)) {
	browserDisplay.focusTime = BrowserRequestEffectiveTime(time);
    }
}

int
BrowserActivatePassiveButtonGrab(
    Window eventWindow,
    unsigned int button,
    unsigned int state,
    Time time)
{
    X11WasmPassiveButtonGrab *grabPtr;

    BrowserInitDisplay();
    if (BrowserWindowCanReceiveEvents(browserDisplay.pointerGrabWindow)) {
	return 0;
    }
    grabPtr = BrowserFindPassiveButtonGrab(eventWindow, button, state);
    if (grabPtr == NULL) {
	return 0;
    }
    browserDisplay.pointerGrabWindow = grabPtr->window;
    browserDisplay.pointerGrabOwnerEvents = grabPtr->ownerEvents;
    browserDisplay.pointerGrabEventMask = grabPtr->eventMask;
    browserDisplay.pointerGrabConfineWindow = grabPtr->confineTo;
    BrowserSetPointerGrabCursor(grabPtr->cursor);
    browserDisplay.pointerGrabPointerMode = grabPtr->pointerMode;
    browserDisplay.pointerGrabKeyboardMode = grabPtr->keyboardMode;
    browserDisplay.pointerGrabTime = BrowserRequestEffectiveTime(time);
    browserDisplay.pointerGrabPassive = 1;
    BrowserSyncPointerGrabCursor();
    BrowserSyncPointerGrabState();
    return 1;
}

int
BrowserActivatePassiveKeyGrab(
    Window eventWindow,
    unsigned int keycode,
    unsigned int state,
    Time time)
{
    X11WasmPassiveKeyGrab *grabPtr;

    BrowserInitDisplay();
    if (BrowserWindowCanReceiveEvents(browserDisplay.keyboardGrabWindow)) {
	return 0;
    }
    grabPtr = BrowserFindPassiveKeyGrab(eventWindow, keycode, state);
    if (grabPtr == NULL) {
	return 0;
    }
    browserDisplay.keyboardGrabWindow = grabPtr->window;
    browserDisplay.keyboardGrabOwnerEvents = grabPtr->ownerEvents;
    browserDisplay.keyboardGrabPointerMode = grabPtr->pointerMode;
    browserDisplay.keyboardGrabKeyboardMode = grabPtr->keyboardMode;
    browserDisplay.keyboardGrabTime = BrowserRequestEffectiveTime(time);
    browserDisplay.keyboardGrabPassive = 1;
    BrowserSyncKeyboardGrabState();
    return 1;
}

void
BrowserReleasePassivePointerGrabIfIdle(
    unsigned int state)
{
    const unsigned int buttonMask = Button1Mask | Button2Mask | Button3Mask
	    | Button4Mask | Button5Mask;

    if (browserDisplay.pointerGrabPassive && ((state & buttonMask) == 0)) {
	BrowserDeactivatePointerGrab();
    }
}

void
BrowserReleasePassiveKeyboardGrabIfIdle(void)
{
    int i;

    if (!browserDisplay.keyboardGrabPassive) {
	return;
    }
    for (i = 0; i < 32; i++) {
	if (browserDisplay.keymap[i] != 0) {
	    return;
	}
    }
    BrowserDeactivateKeyboardGrab();
}

void
BrowserDeletePassiveGrabsForWindow(
    Window window)
{
    X11WasmPassiveButtonGrab **buttonLinkPtr;
    X11WasmPassiveKeyGrab **keyLinkPtr;

    buttonLinkPtr = &browserDisplay.passiveButtonGrabs;
    while (*buttonLinkPtr != NULL) {
	X11WasmPassiveButtonGrab *grabPtr = *buttonLinkPtr;

	if ((grabPtr->window == window) || (grabPtr->confineTo == window)) {
	    *buttonLinkPtr = grabPtr->nextPtr;
	    BrowserFreePassiveButtonGrab(grabPtr);
	    continue;
	}
	buttonLinkPtr = &grabPtr->nextPtr;
    }

    keyLinkPtr = &browserDisplay.passiveKeyGrabs;
    while (*keyLinkPtr != NULL) {
	X11WasmPassiveKeyGrab *grabPtr = *keyLinkPtr;

	if (grabPtr->window == window) {
	    *keyLinkPtr = grabPtr->nextPtr;
	    BrowserFreePassiveKeyGrab(grabPtr);
	    continue;
	}
	keyLinkPtr = &grabPtr->nextPtr;
    }
}

void
BrowserClearPassiveGrabs(void)
{
    while (browserDisplay.passiveButtonGrabs != NULL) {
	X11WasmPassiveButtonGrab *grabPtr = browserDisplay.passiveButtonGrabs;

	browserDisplay.passiveButtonGrabs = grabPtr->nextPtr;
	BrowserFreePassiveButtonGrab(grabPtr);
    }
    while (browserDisplay.passiveKeyGrabs != NULL) {
	X11WasmPassiveKeyGrab *grabPtr = browserDisplay.passiveKeyGrabs;

	browserDisplay.passiveKeyGrabs = grabPtr->nextPtr;
	BrowserFreePassiveKeyGrab(grabPtr);
    }
}

int
XSetInputFocus(
    Display *display,
    Window focus,
    int revertTo,
    Time time)
{
    Window oldFocus;
    Window newFocus;
    X11WasmResource *resourcePtr = NULL;
    Time effectiveTime;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (!BrowserValueOrError(&display, revertTo,
	    (revertTo == RevertToNone) || (revertTo == RevertToPointerRoot)
	    || (revertTo == RevertToParent),
	    X11_WASM_XREQ_SET_INPUT_FOCUS)) {
	return Success;
    }
    oldFocus = browserDisplay.focusWindow;
    if ((focus == None) || (focus == PointerRoot)) {
	newFocus = None;
    } else {
	resourcePtr = BrowserWindowResourceOrError(&display, focus,
		X11_WASM_XREQ_SET_INPUT_FOCUS);
	if (resourcePtr == NULL) {
	    return Success;
	}
	if (!BrowserResourceIsVisible(resourcePtr)) {
	    (void)BrowserBadMatchOrError(&display, (XID)focus,
		    X11_WASM_XREQ_SET_INPUT_FOCUS);
	    return Success;
	}
	newFocus = focus;
    }
    if (!BrowserRequestTimeIsValid(time, browserDisplay.focusTime)) {
	return Success;
    }
    effectiveTime = BrowserRequestEffectiveTime(time);
    browserDisplay.focusRevertTo = revertTo;
    browserDisplay.focusWindow = newFocus;
    BrowserRecordFocusChangeTime(effectiveTime);
    (void)X11WasmJsSetInputFocus(newFocus != None, (unsigned int)newFocus);
    if (oldFocus != newFocus) {
	(void)BrowserQueueRawFocusEvent(oldFocus, FocusOut, effectiveTime);
	(void)BrowserQueueRawFocusEvent(newFocus, FocusIn, effectiveTime);
    }
    (void)display;
    return Success;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestXSendEventTargets(
    unsigned int focusWindowId,
    unsigned int pointerWindowId)
{
    Display *display = &browserDisplay.display;
    Window focusWindow = (Window)focusWindowId;
    Window pointerWindow = (Window)pointerWindowId;
    Window rootWindow;
    Window returnedFocus;
    XEvent event;
    XEvent received;
    int returnedRevert;
    int rootX = 0;
    int rootY = 0;

    BrowserInitDisplay();
    rootWindow = RootWindow(display, DefaultScreen(display));
    if ((rootWindow == None) || (rootWindow == PointerWindow)
	    || (rootWindow == InputFocus) || (rootWindow == PointerRoot)) {
	return 0;
    }
    if (!BrowserWindowCanReceiveEvents(focusWindow)
	    || !BrowserWindowCanReceiveEvents(pointerWindow)) {
	return 0;
    }
    if ((XSelectInput(display, focusWindow,
		    KeyPressMask | FocusChangeMask) != Success)
	    || (XSelectInput(display, pointerWindow,
		    ButtonPressMask | PointerMotionMask) != Success)
	    || (XSetInputFocus(display, focusWindow, RevertToParent,
		    CurrentTime) != Success)) {
	return 0;
    }
    while (BrowserPopMatchingEvent(focusWindow, KeyPressMask, &received)) {
	/* Drain stale matching events before checking the sent event. */
    }
    while (BrowserPopMatchingEvent(pointerWindow, ButtonPressMask,
	    &received)) {
	/* Drain stale matching events before checking the sent event. */
    }
    while (BrowserPopMatchingEvent(rootWindow, KeyPressMask, &received)) {
	/* Drain stale matching events before checking the sent event. */
    }

    memset(&event, 0, sizeof(event));
    event.type = KeyPress;
    event.xkey.display = display;
    event.xkey.window = InputFocus;
    event.xkey.root = browserDisplay.rootWindow;
    event.xkey.keycode = XKeysymToKeycode(display, XK_a);
    if ((XSendEvent(display, InputFocus, False, KeyPressMask, &event)
		    != True)
	    || !BrowserPopMatchingEvent(focusWindow, KeyPressMask, &received)
	    || (received.type != KeyPress)
	    || (received.xany.window != focusWindow)
	    || (received.xany.send_event != True)) {
	return 0;
    }

    memset(&event, 0, sizeof(event));
    event.type = KeyPress;
    event.xkey.display = display;
    event.xkey.window = rootWindow;
    event.xkey.root = rootWindow;
    event.xkey.keycode = XKeysymToKeycode(display, XK_b);
    if ((XSendEvent(display, rootWindow, False, KeyPressMask, &event)
		    != True)
	    || !BrowserPopMatchingEvent(rootWindow, KeyPressMask, &received)
	    || (received.type != KeyPress)
	    || (received.xany.window != rootWindow)
	    || (received.xany.send_event != True)) {
	return 0;
    }

    if ((XSetInputFocus(display, rootWindow, RevertToParent, CurrentTime)
		    != Success)
	    || (browserDisplay.focusWindow != rootWindow)) {
	return 0;
    }
    (void)XGetInputFocus(display, &returnedFocus, &returnedRevert);
    if ((returnedFocus != rootWindow) || (returnedRevert != RevertToParent)
	    || (XSetInputFocus(display, focusWindow, RevertToParent,
		    CurrentTime) != Success)) {
	return 0;
    }

    BrowserWindowRootOffset(pointerWindow, &rootX, &rootY);
    BrowserUpdatePointerState(pointerWindow, rootX + 1, rootY + 1, 0);
    memset(&event, 0, sizeof(event));
    event.type = ButtonPress;
    event.xbutton.display = display;
    event.xbutton.window = PointerWindow;
    event.xbutton.root = browserDisplay.rootWindow;
    event.xbutton.button = Button1;
    event.xbutton.x_root = rootX + 1;
    event.xbutton.y_root = rootY + 1;
    if ((XSendEvent(display, PointerWindow, False, ButtonPressMask, &event)
		    != True)
	    || !BrowserPopMatchingEvent(pointerWindow, ButtonPressMask,
		    &received)
	    || (received.type != ButtonPress)
	    || (received.xany.window != pointerWindow)
	    || (received.xany.send_event != True)) {
	return 0;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
XGrabKeyboard(
    Display *display,
    Window window,
    Bool ownerEvents,
    int pointerMode,
    int keyboardMode,
    Time time)
{
    X11WasmResource *resourcePtr;
    Window resolvedWindow;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (!BrowserGrabModeIsValid(pointerMode)
	    || !BrowserGrabModeIsValid(keyboardMode)) {
	(void)BrowserReportXError(display, BadValue,
		(XID)(!BrowserGrabModeIsValid(pointerMode)
			? pointerMode : keyboardMode),
		X11_WASM_XREQ_GRAB_KEYBOARD, 0);
	return BadValue;
    }
    if (!BrowserValidateBool(&display, ownerEvents,
	    X11_WASM_XREQ_GRAB_KEYBOARD)) {
	return BadValue;
    }
    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_GRAB_KEYBOARD);
    if (resourcePtr == NULL) {
	return GrabNotViewable;
    }
    if (!BrowserRequestTimeIsValid(time, browserDisplay.keyboardGrabTime)) {
	return GrabInvalidTime;
    }
    if (!BrowserResourceIsVisible(resourcePtr)) {
	return GrabNotViewable;
    }
    resolvedWindow = BrowserResolveEventWindow(window);
    if ((browserDisplay.keyboardGrabWindow != None)
	    && BrowserWindowCanReceiveEvents(browserDisplay.keyboardGrabWindow)
	    && (browserDisplay.keyboardGrabWindow != resolvedWindow)) {
	return AlreadyGrabbed;
    }
    browserDisplay.keyboardGrabWindow = resolvedWindow;
    browserDisplay.keyboardGrabOwnerEvents = ownerEvents ? 1 : 0;
    browserDisplay.keyboardGrabPointerMode = pointerMode;
    browserDisplay.keyboardGrabKeyboardMode = keyboardMode;
    browserDisplay.keyboardGrabTime = BrowserRequestEffectiveTime(time);
    browserDisplay.keyboardGrabPassive = 0;
    BrowserSyncKeyboardGrabState();
    return GrabSuccess;
}
EMSCRIPTEN_KEEPALIVE int
XGrabPointer(
    Display *display,
    Window grabWindow,
    Bool ownerEvents,
    unsigned int eventMask,
    int pointerMode,
    int keyboardMode,
    Window confineTo,
    Cursor cursor,
    Time time)
{
    X11WasmResource *grabPtr;
    X11WasmResource *confinePtr = NULL;
    Window resolvedGrabWindow;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (!BrowserGrabModeIsValid(pointerMode)
	    || !BrowserGrabModeIsValid(keyboardMode)) {
	(void)BrowserReportXError(display, BadValue,
		(XID)(!BrowserGrabModeIsValid(pointerMode)
			? pointerMode : keyboardMode),
		X11_WASM_XREQ_GRAB_POINTER, 0);
	return BadValue;
    }
    if (!BrowserValidateBool(&display, ownerEvents,
	    X11_WASM_XREQ_GRAB_POINTER)) {
	return BadValue;
    }
    if (!BrowserPointerGrabEventMaskIsValid(eventMask)) {
	(void)BrowserReportXError(display, BadValue, (XID)eventMask,
		X11_WASM_XREQ_GRAB_POINTER, 0);
	return BadValue;
    }
    grabPtr = BrowserWindowResourceOrError(&display, grabWindow,
	    X11_WASM_XREQ_GRAB_POINTER);
    if (grabPtr == NULL) {
	return GrabNotViewable;
    }
    if (confineTo != None) {
	confinePtr = BrowserWindowResourceOrError(&display, confineTo,
		X11_WASM_XREQ_GRAB_POINTER);
	if (confinePtr == NULL) {
	    return GrabNotViewable;
	}
    }
    if ((cursor != None) && (BrowserCursorResourceOrError(&display, cursor,
	    X11_WASM_XREQ_GRAB_POINTER) == NULL)) {
	return BadCursor;
    }
    if (!BrowserRequestTimeIsValid(time, browserDisplay.pointerGrabTime)) {
	return GrabInvalidTime;
    }
    if (!BrowserResourceIsVisible(grabPtr)
	    || ((confinePtr != NULL) && !BrowserResourceIsVisible(confinePtr))) {
	return GrabNotViewable;
    }
    resolvedGrabWindow = BrowserResolveEventWindow(grabWindow);
    if ((browserDisplay.pointerGrabWindow != None)
	    && BrowserWindowCanReceiveEvents(browserDisplay.pointerGrabWindow)
	    && (browserDisplay.pointerGrabWindow != resolvedGrabWindow)) {
	return AlreadyGrabbed;
    }
    browserDisplay.pointerGrabWindow = resolvedGrabWindow;
    browserDisplay.pointerGrabOwnerEvents = ownerEvents ? 1 : 0;
    browserDisplay.pointerGrabEventMask = eventMask;
    browserDisplay.pointerGrabConfineWindow = confineTo;
    BrowserSetPointerGrabCursor(cursor);
    browserDisplay.pointerGrabPointerMode = pointerMode;
    browserDisplay.pointerGrabKeyboardMode = keyboardMode;
    browserDisplay.pointerGrabTime = BrowserRequestEffectiveTime(time);
    browserDisplay.pointerGrabPassive = 0;
    BrowserClampPointerToGrabConfine(&browserDisplay.pointerX,
	    &browserDisplay.pointerY);
    browserDisplay.pointerWindow = BrowserHitTestRoot(browserDisplay.pointerX,
	    browserDisplay.pointerY, NULL, NULL);
    BrowserSyncPointerState();
    BrowserSyncPointerGrabCursor();
    BrowserSyncPointerGrabState();
    return GrabSuccess;
}

EMSCRIPTEN_KEEPALIVE int
XAllowEvents(
    Display *display,
    int eventMode,
    Time time)
{
    int pointerChanged = 0;
    int keyboardChanged = 0;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, eventMode,
	    BrowserAllowEventsModeIsValid(eventMode),
	    X11_WASM_XREQ_ALLOW_EVENTS)) {
	return Success;
    }
    if (!BrowserAllowEventsTimeIsValid(eventMode, time)) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	if ((eventMode == AsyncPointer) || (eventMode == ReplayPointer)
		|| (eventMode == AsyncBoth)) {
	    if (browserDisplay.pointerGrabWindow != None) {
		browserDisplay.pointerGrabPointerMode = GrabModeAsync;
		pointerChanged = 1;
	    }
	    if (browserDisplay.keyboardGrabWindow != None) {
		browserDisplay.keyboardGrabPointerMode = GrabModeAsync;
		keyboardChanged = 1;
	    }
	}
	if ((eventMode == AsyncKeyboard) || (eventMode == ReplayKeyboard)
		|| (eventMode == AsyncBoth)) {
	    if (browserDisplay.pointerGrabWindow != None) {
		browserDisplay.pointerGrabKeyboardMode = GrabModeAsync;
		pointerChanged = 1;
	    }
	    if (browserDisplay.keyboardGrabWindow != None) {
		browserDisplay.keyboardGrabKeyboardMode = GrabModeAsync;
		keyboardChanged = 1;
	    }
	}
	if (pointerChanged) {
	    BrowserSyncPointerGrabState();
	}
	if (keyboardChanged) {
	    BrowserSyncKeyboardGrabState();
	}
    }
    return XNoOp(display);
}

EMSCRIPTEN_KEEPALIVE int
XChangeActivePointerGrab(
    Display *display,
    unsigned int eventMask,
    Cursor cursor,
    Time time)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, (long)eventMask,
	    BrowserPointerGrabEventMaskIsValid(eventMask),
	    X11_WASM_XREQ_CHANGE_ACTIVE_POINTER_GRAB)) {
	return Success;
    }
    if ((cursor != None) && (BrowserCursorResourceOrError(&display, cursor,
	    X11_WASM_XREQ_CHANGE_ACTIVE_POINTER_GRAB) == NULL)) {
	return Success;
    }
    if (!BrowserRequestTimeIsValid(time, browserDisplay.pointerGrabTime)) {
	return Success;
    }
    if ((display == &browserDisplay.display)
	    && (browserDisplay.pointerGrabWindow != None)) {
	browserDisplay.pointerGrabEventMask = eventMask;
	BrowserSetPointerGrabCursor(cursor);
	BrowserSyncPointerGrabCursor();
	BrowserSyncPointerGrabState();
    }
    return XNoOp(display);
}

EMSCRIPTEN_KEEPALIVE int
XGrabButton(
    Display *display,
    unsigned int button,
    unsigned int modifiers,
    Window grabWindow,
    Bool ownerEvents,
    unsigned int eventMask,
    int pointerMode,
    int keyboardMode,
    Window confineTo,
    Cursor cursor)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, (long)button,
	    BrowserPassiveGrabButtonIsValid(button),
	    X11_WASM_XREQ_GRAB_BUTTON)
	    || !BrowserValueOrError(&display, (long)modifiers,
		    BrowserPassiveGrabModifierMaskIsValid(modifiers),
		    X11_WASM_XREQ_GRAB_BUTTON)
	    || !BrowserValidateBool(&display, ownerEvents,
		    X11_WASM_XREQ_GRAB_BUTTON)
	    || !BrowserValueOrError(&display, (long)eventMask,
		    BrowserPointerGrabEventMaskIsValid(eventMask),
		    X11_WASM_XREQ_GRAB_BUTTON)
	    || !BrowserValueOrError(&display, pointerMode,
		    BrowserGrabModeIsValid(pointerMode),
		    X11_WASM_XREQ_GRAB_BUTTON)
	    || !BrowserValueOrError(&display, keyboardMode,
		    BrowserGrabModeIsValid(keyboardMode),
		    X11_WASM_XREQ_GRAB_BUTTON)) {
	return Success;
    }
    if (BrowserWindowResourceOrError(&display, grabWindow,
	    X11_WASM_XREQ_GRAB_BUTTON) == NULL) {
	return Success;
    }
    if ((confineTo != None) && (BrowserWindowResourceOrError(&display,
	    confineTo, X11_WASM_XREQ_GRAB_BUTTON) == NULL)) {
	return Success;
    }
    if ((cursor != None) && (BrowserCursorResourceOrError(&display, cursor,
	    X11_WASM_XREQ_GRAB_BUTTON) == NULL)) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	BrowserInstallPassiveButtonGrab(grabWindow, button, modifiers,
		ownerEvents, eventMask, pointerMode, keyboardMode, confineTo,
		cursor);
    }
    return XNoOp(display);
}

EMSCRIPTEN_KEEPALIVE int
XGrabKey(
    Display *display,
    int keycode,
    unsigned int modifiers,
    Window grabWindow,
    Bool ownerEvents,
    int pointerMode,
    int keyboardMode)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, keycode,
	    BrowserPassiveGrabKeycodeIsValid(display, keycode),
	    X11_WASM_XREQ_GRAB_KEY)
	    || !BrowserValueOrError(&display, (long)modifiers,
		    BrowserPassiveGrabModifierMaskIsValid(modifiers),
		    X11_WASM_XREQ_GRAB_KEY)
	    || !BrowserValidateBool(&display, ownerEvents,
		    X11_WASM_XREQ_GRAB_KEY)
	    || !BrowserValueOrError(&display, pointerMode,
		    BrowserGrabModeIsValid(pointerMode),
		    X11_WASM_XREQ_GRAB_KEY)
	    || !BrowserValueOrError(&display, keyboardMode,
		    BrowserGrabModeIsValid(keyboardMode),
		    X11_WASM_XREQ_GRAB_KEY)) {
	return Success;
    }
    if (BrowserWindowResourceOrError(&display, grabWindow,
	    X11_WASM_XREQ_GRAB_KEY) == NULL) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	BrowserInstallPassiveKeyGrab(display, grabWindow, keycode, modifiers,
		ownerEvents, pointerMode, keyboardMode);
    }
    return XNoOp(display);
}

EMSCRIPTEN_KEEPALIVE int
XUngrabButton(
    Display *display,
    unsigned int button,
    unsigned int modifiers,
    Window grabWindow)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, (long)button,
	    BrowserPassiveGrabButtonIsValid(button),
	    X11_WASM_XREQ_UNGRAB_BUTTON)
	    || !BrowserValueOrError(&display, (long)modifiers,
		    BrowserPassiveGrabModifierMaskIsValid(modifiers),
		    X11_WASM_XREQ_UNGRAB_BUTTON)) {
	return Success;
    }
    if (BrowserWindowResourceOrError(&display, grabWindow,
	    X11_WASM_XREQ_UNGRAB_BUTTON) == NULL) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	BrowserRemovePassiveButtonGrabs(grabWindow, button, modifiers);
    }
    return XNoOp(display);
}

EMSCRIPTEN_KEEPALIVE int
XUngrabKey(
    Display *display,
    int keycode,
    unsigned int modifiers,
    Window grabWindow)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, keycode,
	    BrowserPassiveGrabKeycodeIsValid(display, keycode),
	    X11_WASM_XREQ_UNGRAB_KEY)
	    || !BrowserValueOrError(&display, (long)modifiers,
		    BrowserPassiveGrabModifierMaskIsValid(modifiers),
		    X11_WASM_XREQ_UNGRAB_KEY)) {
	return Success;
    }
    if (BrowserWindowResourceOrError(&display, grabWindow,
	    X11_WASM_XREQ_UNGRAB_KEY) == NULL) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	BrowserRemovePassiveKeyGrabs(grabWindow, keycode, modifiers);
    }
    return XNoOp(display);
}

EMSCRIPTEN_KEEPALIVE int
XUngrabKeyboard(
    Display *display,
    Time time)
{
    Time effectiveTime;

    (void)display;
    if (!BrowserRequestTimeIsValid(time, browserDisplay.keyboardGrabTime)) {
	return Success;
    }
    if (browserDisplay.keyboardGrabWindow == None) {
	return Success;
    }
    effectiveTime = BrowserRequestEffectiveTime(time);
    BrowserDeactivateKeyboardGrab();
    browserDisplay.keyboardGrabTime = effectiveTime;
    return Success;
}
EMSCRIPTEN_KEEPALIVE int
XUngrabPointer(
    Display *display,
    Time time)
{
    Time effectiveTime;

    (void)display;
    if (!BrowserRequestTimeIsValid(time, browserDisplay.pointerGrabTime)) {
	return Success;
    }
    if (browserDisplay.pointerGrabWindow == None) {
	return Success;
    }
    effectiveTime = BrowserRequestEffectiveTime(time);
    BrowserDeactivatePointerGrab();
    browserDisplay.pointerGrabTime = effectiveTime;
    return Success;
}
int
XWarpPointer(
    Display *display,
    Window src,
    Window dest,
    int srcX,
    int srcY,
    unsigned int srcW,
    unsigned int srcH,
    int destX,
    int destY)
{
    int rootX;
    int rootY;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    (void)display;

    if (!BrowserValidateCard16(&display, (long)srcW, 1,
	    X11_WASM_XREQ_WARP_POINTER)
	    || !BrowserValidateCard16(&display, (long)srcH, 1,
		    X11_WASM_XREQ_WARP_POINTER)) {
	return Success;
    }

    if (src != None) {
	int pointerX = 0;
	int pointerY = 0;
	long long srcRight;
	long long srcBottom;
	X11WasmResource *sourcePtr = BrowserWindowResourceOrError(&display,
		src, X11_WASM_XREQ_WARP_POINTER);

	if (sourcePtr == NULL) {
	    return Success;
	}
	BrowserWindowCoordinatesFromRoot(src, browserDisplay.pointerX,
		browserDisplay.pointerY, &pointerX, &pointerY);
	if ((pointerX < 0) || (pointerY < 0)
		|| (pointerX >= (int)sourcePtr->width)
		|| (pointerY >= (int)sourcePtr->height)) {
	    return Success;
	}
	srcRight = (srcW == 0) ? (long long)sourcePtr->width
		: (long long)srcX + (long long)srcW;
	srcBottom = (srcH == 0) ? (long long)sourcePtr->height
		: (long long)srcY + (long long)srcH;
	if (((long long)pointerX < (long long)srcX)
		|| ((long long)pointerX >= srcRight)) {
	    return Success;
	}
	if (((long long)pointerY < (long long)srcY)
		|| ((long long)pointerY >= srcBottom)) {
	    return Success;
	}
    }

    if (dest != None) {
	if (BrowserWindowResourceOrError(&display, dest,
		X11_WASM_XREQ_WARP_POINTER) == NULL) {
	    return Success;
	}
	BrowserWindowRootOffset(dest, &rootX, &rootY);
	rootX += destX;
	rootY += destY;
    } else {
	rootX = browserDisplay.pointerX + destX;
	rootY = browserDisplay.pointerY + destY;
    }

    (void)BrowserQueuePointerEventFields(MotionNotify,
	    browserDisplay.rootWindow, 0, browserDisplay.pointerState, rootX,
	    rootY, CurrentTime, 0);
    (void)X11WasmJsWarpPointer(browserDisplay.pointerX,
	    browserDisplay.pointerY, (unsigned int)browserDisplay.pointerWindow,
	    (unsigned int)eventTime);
    return Success;
}

int
XBell(
    Display *display,
    int percent)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, percent,
	    (percent >= -100) && (percent <= 100), X11_WASM_XREQ_BELL)) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	return X11WasmJsBell(percent);
    }
    return Success;
}

int
XChangeKeyboardControl(
    Display *display,
    unsigned long valueMask,
    XKeyboardControl *values)
{
    unsigned long validMask = KBKeyClickPercent | KBBellPercent | KBBellPitch
	    | KBBellDuration | KBLed | KBLedMode | KBKey
	    | KBAutoRepeatMode;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, (long)(valueMask & ~validMask),
	    (valueMask & ~validMask) == 0UL,
	    X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL)) {
	return Success;
    }
    if ((valueMask != 0UL) && (values == NULL)) {
	(void)BrowserValueOrError(&display, 0, 0,
		X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL);
	return Success;
    }
    if (valueMask == 0UL) {
	return XNoOp(display);
    }
    if ((valueMask & KBKeyClickPercent)
	    && !BrowserValueOrError(&display, values->key_click_percent,
		    BrowserKeyboardPercentIsValid(values->key_click_percent),
		    X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL)) {
	return Success;
    }
    if ((valueMask & KBBellPercent)
	    && !BrowserValueOrError(&display, values->bell_percent,
		    BrowserKeyboardPercentIsValid(values->bell_percent),
		    X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL)) {
	return Success;
    }
    if ((valueMask & KBBellPitch)
	    && !BrowserValueOrError(&display, values->bell_pitch,
		    values->bell_pitch >= -1,
		    X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL)) {
	return Success;
    }
    if ((valueMask & KBBellDuration)
	    && !BrowserValueOrError(&display, values->bell_duration,
		    values->bell_duration >= -1,
		    X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL)) {
	return Success;
    }
    if ((valueMask & (KBLed | KBLedMode))
	    && !BrowserValueOrError(&display, values->led,
		    (values->led >= 1) && (values->led <= 32),
		    X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL)) {
	return Success;
    }
    if ((valueMask & KBLedMode)
	    && !BrowserValueOrError(&display, values->led_mode,
		    (values->led_mode == LedModeOff)
		    || (values->led_mode == LedModeOn),
		    X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL)) {
	return Success;
    }
    if ((valueMask & KBKey)
	    && !BrowserValueOrError(&display, values->key,
		    (values->key >= display->min_keycode)
		    && (values->key <= display->max_keycode),
		    X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL)) {
	return Success;
    }
    if ((valueMask & KBAutoRepeatMode)
	    && !BrowserValueOrError(&display, values->auto_repeat_mode,
		    BrowserKeyboardAutoRepeatModeIsValid(
			    values->auto_repeat_mode),
		    X11_WASM_XREQ_CHANGE_KEYBOARD_CONTROL)) {
	return Success;
    }

    if (display == &browserDisplay.display) {
	if (valueMask & KBKeyClickPercent) {
	    browserDisplay.keyClickPercent =
		    (values->key_click_percent == -1) ? 0
		    : values->key_click_percent;
	}
	if (valueMask & KBBellPercent) {
	    browserDisplay.bellPercent =
		    (values->bell_percent == -1) ? 50 : values->bell_percent;
	}
	if (valueMask & KBBellPitch) {
	    browserDisplay.bellPitch =
		    (values->bell_pitch == -1) ? 400
		    : (unsigned int)values->bell_pitch;
	}
	if (valueMask & KBBellDuration) {
	    browserDisplay.bellDuration =
		    (values->bell_duration == -1) ? 100
		    : (unsigned int)values->bell_duration;
	}
	if (valueMask & KBLedMode) {
	    unsigned long ledMask = 1UL << (values->led - 1);

	    if (values->led_mode == LedModeOn) {
		browserDisplay.ledMask |= ledMask;
	    } else {
		browserDisplay.ledMask &= ~ledMask;
	    }
	}
	if (valueMask & KBAutoRepeatMode) {
	    int repeatOn = values->auto_repeat_mode != AutoRepeatModeOff;

	    if (valueMask & KBKey) {
		BrowserSetKeyBit(browserDisplay.autoRepeats,
			(unsigned int)values->key, repeatOn);
	    } else {
		browserDisplay.globalAutoRepeat = repeatOn
			? AutoRepeatModeOn : AutoRepeatModeOff;
	    }
	}
    }
    return XNoOp(display);
}

int
XAutoRepeatOff(
    Display *display)
{
    XKeyboardControl values;

    memset(&values, 0, sizeof(values));
    values.auto_repeat_mode = AutoRepeatModeOff;
    return XChangeKeyboardControl(display, KBAutoRepeatMode, &values);
}

int
XAutoRepeatOn(
    Display *display)
{
    XKeyboardControl values;

    memset(&values, 0, sizeof(values));
    values.auto_repeat_mode = AutoRepeatModeOn;
    return XChangeKeyboardControl(display, KBAutoRepeatMode, &values);
}

int
XGetKeyboardControl(
    Display *display,
    XKeyboardState *valuesReturn)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if ((display == &browserDisplay.display) && (valuesReturn != NULL)) {
	valuesReturn->key_click_percent = browserDisplay.keyClickPercent;
	valuesReturn->bell_percent = browserDisplay.bellPercent;
	valuesReturn->bell_pitch = browserDisplay.bellPitch;
	valuesReturn->bell_duration = browserDisplay.bellDuration;
	valuesReturn->led_mask = browserDisplay.ledMask;
	valuesReturn->global_auto_repeat = browserDisplay.globalAutoRepeat;
	memcpy(valuesReturn->auto_repeats, browserDisplay.autoRepeats,
		sizeof(valuesReturn->auto_repeats));
    } else if (valuesReturn != NULL) {
	memset(valuesReturn, 0, sizeof(*valuesReturn));
    }
    return XNoOp(display);
}

int
XQueryKeymap(
    Display *display,
    char keysReturn[32])
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if ((display == &browserDisplay.display) && (keysReturn != NULL)) {
	memcpy(keysReturn, browserDisplay.keymap, sizeof(browserDisplay.keymap));
    } else if (keysReturn != NULL) {
	memset(keysReturn, 0, 32);
    }
    return XNoOp(display);
}

int
XChangePointerControl(
    Display *display,
    Bool doAccel,
    Bool doThreshold,
    int accelNumerator,
    int accelDenominator,
    int threshold)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValidateBool(&display, doAccel,
	    X11_WASM_XREQ_CHANGE_POINTER_CONTROL)
	    || !BrowserValidateBool(&display, doThreshold,
		    X11_WASM_XREQ_CHANGE_POINTER_CONTROL)) {
	return Success;
    }
    if (doAccel
	    && (!BrowserValueOrError(&display, accelNumerator,
		    accelNumerator >= -1,
		    X11_WASM_XREQ_CHANGE_POINTER_CONTROL)
	    || !BrowserValueOrError(&display, accelDenominator,
		    (accelDenominator >= -1) && (accelDenominator != 0),
		    X11_WASM_XREQ_CHANGE_POINTER_CONTROL))) {
	return Success;
    }
    if (doThreshold
	    && !BrowserValueOrError(&display, threshold, threshold >= -1,
		    X11_WASM_XREQ_CHANGE_POINTER_CONTROL)) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	if (doAccel) {
	    if ((accelNumerator == -1) || (accelDenominator == -1)) {
		browserDisplay.pointerAccelNumerator = 1;
		browserDisplay.pointerAccelDenominator = 1;
	    } else {
		browserDisplay.pointerAccelNumerator = accelNumerator;
		browserDisplay.pointerAccelDenominator = accelDenominator;
	    }
	}
	if (doThreshold) {
	    browserDisplay.pointerThreshold =
		    (threshold == -1) ? 0 : threshold;
	}
    }
    return XNoOp(display);
}

int
XGetPointerControl(
    Display *display,
    int *accelNumeratorReturn,
    int *accelDenominatorReturn,
    int *thresholdReturn)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (display == &browserDisplay.display) {
	if (accelNumeratorReturn != NULL) {
	    *accelNumeratorReturn = browserDisplay.pointerAccelNumerator;
	}
	if (accelDenominatorReturn != NULL) {
	    *accelDenominatorReturn = browserDisplay.pointerAccelDenominator;
	}
	if (thresholdReturn != NULL) {
	    *thresholdReturn = browserDisplay.pointerThreshold;
	}
    } else {
	if (accelNumeratorReturn != NULL) {
	    *accelNumeratorReturn = 0;
	}
	if (accelDenominatorReturn != NULL) {
	    *accelDenominatorReturn = 0;
	}
	if (thresholdReturn != NULL) {
	    *thresholdReturn = 0;
	}
    }
    return XNoOp(display);
}

int
XGetPointerMapping(
    Display *display,
    unsigned char *mapReturn,
    int nmap)
{
    int count;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    count = (display == &browserDisplay.display)
	    ? browserDisplay.pointerMapCount : 0;
    if ((display == &browserDisplay.display) && (mapReturn != NULL)
	    && (nmap > 0)) {
	int copyCount = (nmap < count) ? nmap : count;

	memcpy(mapReturn, browserDisplay.pointerMap, (size_t)copyCount);
    }
    (void)XNoOp(display);
    return count;
}

unsigned int
BrowserPointerButtonMask(
    unsigned int button)
{
    switch (button) {
    case Button1:
	return Button1Mask;
    case Button2:
	return Button2Mask;
    case Button3:
	return Button3Mask;
    case Button4:
	return Button4Mask;
    case Button5:
	return Button5Mask;
    default:
	return 0;
    }
}

static int
BrowserPointerMappingIsBusy(
    const unsigned char *map,
    int nmap)
{
    int i;

    for (i = 0; i < nmap; i++) {
	unsigned int oldMask;
	unsigned int newMask;

	if (map[i] == browserDisplay.pointerMap[i]) {
	    continue;
	}
	oldMask = BrowserPointerButtonMask(browserDisplay.pointerMap[i]);
	newMask = BrowserPointerButtonMask(map[i]);
	if (browserDisplay.pointerState & (oldMask | newMask)) {
	    return 1;
	}
    }
    return 0;
}

int
XSetPointerMapping(
    Display *display,
    _Xconst unsigned char *map,
    int nmap)
{
    int i;
    int j;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if ((display != &browserDisplay.display)
	    || ((nmap > 0) && (map == NULL))) {
	(void)XNoOp(display);
	return MappingFailed;
    }
    if (!BrowserValueOrError(&display, nmap,
	    nmap == browserDisplay.pointerMapCount,
	    X11_WASM_XREQ_SET_POINTER_MAPPING)) {
	(void)XNoOp(display);
	return MappingFailed;
    }
    for (i = 0; i < nmap; i++) {
	if (map[i] == 0) {
	    continue;
	}
	for (j = i + 1; j < nmap; j++) {
	    if (map[i] == map[j]) {
		(void)BrowserValueOrError(&display, (long)map[i], 0,
			X11_WASM_XREQ_SET_POINTER_MAPPING);
		(void)XNoOp(display);
		return MappingFailed;
	    }
	}
    }
    if (BrowserPointerMappingIsBusy(map, nmap)) {
	(void)XNoOp(display);
	return MappingBusy;
    }
    memcpy(browserDisplay.pointerMap, map, (size_t)nmap);
    (void)BrowserQueueMappingNotify(MappingPointer, 0, 0);
    (void)XNoOp(display);
    return MappingSuccess;
}

void
BrowserRecordPointerMotion(
    int rootX,
    int rootY,
    Time time)
{
    int index;

    BrowserInitDisplay();
    if (time == CurrentTime) {
	time = eventTime;
    }
    if (browserDisplay.motionHistoryCount < X11_WASM_MOTION_BUFFER_SIZE) {
	index = (browserDisplay.motionHistoryStart
		+ browserDisplay.motionHistoryCount)
		% X11_WASM_MOTION_BUFFER_SIZE;
	browserDisplay.motionHistoryCount++;
    } else {
	index = browserDisplay.motionHistoryStart;
	browserDisplay.motionHistoryStart =
		(browserDisplay.motionHistoryStart + 1)
		% X11_WASM_MOTION_BUFFER_SIZE;
    }
    browserDisplay.motionHistory[index].time = time;
    browserDisplay.motionHistory[index].rootX = rootX;
    browserDisplay.motionHistory[index].rootY = rootY;
}

static int
BrowserMotionEventInRange(
    Time time,
    Time start,
    Time stop)
{
    return (time >= start) && (time <= stop);
}

XTimeCoord *
XGetMotionEvents(
    Display *display,
    Window window,
    Time start,
    Time stop,
    int *neventsReturn)
{
    XTimeCoord *events;
    int count = 0;
    int i;
    int out = 0;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (neventsReturn != NULL) {
	*neventsReturn = 0;
    }
    if (BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_GET_MOTION_EVENTS) == NULL) {
	return NULL;
    }
    if ((display != &browserDisplay.display)
	    || (browserDisplay.motionHistoryCount == 0)
	    || (start > stop)) {
	(void)XNoOp(display);
	return NULL;
    }
    for (i = 0; i < browserDisplay.motionHistoryCount; i++) {
	int index = (browserDisplay.motionHistoryStart + i)
		% X11_WASM_MOTION_BUFFER_SIZE;
	X11WasmMotionRecord *recordPtr =
		&browserDisplay.motionHistory[index];

	if (BrowserMotionEventInRange(recordPtr->time, start, stop)) {
	    count++;
	}
    }
    if (count == 0) {
	(void)XNoOp(display);
	return NULL;
    }
    events = (XTimeCoord *)ckalloc(sizeof(XTimeCoord) * (size_t)count);
    for (i = 0; i < browserDisplay.motionHistoryCount; i++) {
	int index = (browserDisplay.motionHistoryStart + i)
		% X11_WASM_MOTION_BUFFER_SIZE;
	X11WasmMotionRecord *recordPtr =
		&browserDisplay.motionHistory[index];
	int x;
	int y;

	if (!BrowserMotionEventInRange(recordPtr->time, start, stop)) {
	    continue;
	}
	BrowserWindowCoordinatesFromRoot(window, recordPtr->rootX,
		recordPtr->rootY, &x, &y);
	events[out].time = recordPtr->time;
	events[out].x = BrowserClampShort(x);
	events[out].y = BrowserClampShort(y);
	out++;
    }
    if (neventsReturn != NULL) {
	*neventsReturn = count;
    }
    (void)XNoOp(display);
    return events;
}

int
BrowserFlushToJs(
    Display *display)
{
    (void)display;
    if (display == &browserDisplay.display) {
	browserDisplay.flushSerial++;
	BrowserSyncRequestState(display);
    }
    return X11WasmJsFlush();
}

void
BrowserSyncRequestState(
    Display *display)
{
    if (display == &browserDisplay.display) {
	(void)X11WasmJsSetRequestState(
		(unsigned int)LastKnownRequestProcessed(display),
		(unsigned int)browserDisplay.flushSerial,
		(unsigned int)browserDisplay.syncSerial);
    }
}

void
BrowserSyncSynchronizeState(
    Display *display)
{
    if (display == &browserDisplay.display) {
	(void)X11WasmJsSetSynchronizeState(browserDisplay.synchronize != 0);
    }
}

int
BrowserSynchronizedAfterFunction(
    Display *display)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (display == &browserDisplay.display) {
	browserDisplay.syncSerial++;
    }
    return BrowserFlushToJs(display);
}

int
BrowserInvokeAfterFunction(
    Display *display)
{
    XAfterFunction afterFunction;
    int result;

    if (display != &browserDisplay.display) {
	return Success;
    }
    afterFunction = browserDisplay.afterFunction;
    if ((afterFunction == NULL) || browserDisplay.runningAfterFunction) {
	return Success;
    }
    browserDisplay.runningAfterFunction = 1;
    result = afterFunction(display);
    browserDisplay.runningAfterFunction = 0;
    return result;
}

int
BrowserCompleteRequest(
    Display *display,
    int flush)
{
    int result = Success;
    int afterResult;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    LastKnownRequestProcessed(display)++;
    if (flush) {
	result = BrowserFlushToJs(display);
    } else {
	BrowserSyncRequestState(display);
    }
    afterResult = BrowserInvokeAfterFunction(display);
    return (result != Success) ? result : afterResult;
}

int XFlush(Display *display)
{
    return BrowserCompleteRequest(display, 1);
}
int XSync(Display *display, Bool discard)
{
    int result;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    LastKnownRequestProcessed(display)++;
    if (display == &browserDisplay.display) {
	browserDisplay.syncSerial++;
    }
    result = BrowserFlushToJs(display);
    if (discard) {
	BrowserClearEventQueue();
    }
    return result;
}
int XNoOp(Display *display)
{
    return BrowserCompleteRequest(display, 0);
}
int XGrabServer(Display *display)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (display == &browserDisplay.display) {
	browserDisplay.serverGrabDepth++;
	BrowserSyncServerGrabState();
    }
    return XNoOp(display);
}
int XUngrabServer(Display *display)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if ((display == &browserDisplay.display)
	    && (browserDisplay.serverGrabDepth > 0)) {
	browserDisplay.serverGrabDepth--;
    }
    if (display == &browserDisplay.display) {
	BrowserSyncServerGrabState();
    }
    return XFlush(display);
}

static int
BrowserChangeSaveSet(
    Display *display,
    Window window,
    int mode)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, mode,
	    (mode == SetModeInsert) || (mode == SetModeDelete),
	    X11_WASM_XREQ_CHANGE_SAVE_SET)) {
	return Success;
    }
    if (BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_CHANGE_SAVE_SET) == NULL) {
	return Success;
    }
    return XNoOp(display);
}

int
XChangeSaveSet(
    Display *display,
    Window window,
    int mode)
{
    return BrowserChangeSaveSet(display, window, mode);
}

int
XAddToSaveSet(
    Display *display,
    Window window)
{
    return BrowserChangeSaveSet(display, window, SetModeInsert);
}

int
XRemoveFromSaveSet(
    Display *display,
    Window window)
{
    return BrowserChangeSaveSet(display, window, SetModeDelete);
}

int
XSetCloseDownMode(
    Display *display,
    int mode)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserValueOrError(&display, mode,
	    (mode == DestroyAll) || (mode == RetainPermanent)
	    || (mode == RetainTemporary),
	    X11_WASM_XREQ_SET_CLOSE_DOWN_MODE)) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	browserDisplay.closeDownMode = mode;
    }
    return XNoOp(display);
}

int
XKillClient(
    Display *display,
    XID resource)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if ((resource != AllTemporary)
	    && (X11WasmFindResource(resource) == NULL)) {
	(void)BrowserValueOrError(&display, (long)resource, 0,
		X11_WASM_XREQ_KILL_CLIENT);
	return Success;
    }
    if ((display == &browserDisplay.display) && (resource == AllTemporary)) {
	BrowserDeleteTemporaryResources();
    }
    return XNoOp(display);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestTemporaryResourceKill(void)
{
    Display *display = &browserDisplay.display;
    int screen;
    Window destroyWindow = None;
    Window permanentWindow = None;
    Window temporaryWindow = None;
    Pixmap temporaryPixmap = None;
    Colormap temporaryColormap = None;
    Font temporaryFont = None;
    X11WasmResource *resourcePtr;
    int result = 0;

    BrowserInitDisplay();
    screen = DefaultScreen(display);
    (void)XSetCloseDownMode(display, DestroyAll);

    destroyWindow = XCreateWindow(display, browserDisplay.rootWindow, 0, 0,
	    11, 11, 0, CopyFromParent, InputOutput,
	    DefaultVisual(display, screen), 0, NULL);
    if ((destroyWindow == None)
	    || (XCloseDisplay(display) != Success)
	    || (X11WasmFindResource((XID)destroyWindow) != NULL)) {
	goto cleanup;
    }

    permanentWindow = XCreateWindow(display, browserDisplay.rootWindow, 0, 0,
	    11, 11, 0, CopyFromParent, InputOutput,
	    DefaultVisual(display, screen), 0, NULL);
    if ((permanentWindow == None)
	    || (XSetCloseDownMode(display, RetainPermanent) != Success)
	    || (XCloseDisplay(display) != Success)) {
	goto cleanup;
    }
    resourcePtr = X11WasmFindResource((XID)permanentWindow);
    if ((resourcePtr == NULL) || !resourcePtr->retained
	    || resourcePtr->temporary
	    || (XKillClient(display, AllTemporary) != Success)
	    || (X11WasmFindResource((XID)permanentWindow) == NULL)) {
	goto cleanup;
    }

    if (XSetCloseDownMode(display, RetainTemporary) != Success) {
	goto cleanup;
    }
    temporaryWindow = XCreateWindow(display, browserDisplay.rootWindow, 1, 1,
	    12, 12, 0, CopyFromParent, InputOutput,
	    DefaultVisual(display, screen), 0, NULL);
    temporaryPixmap = XCreatePixmap(display, browserDisplay.rootWindow, 2, 2,
	    DefaultDepth(display, screen));
    temporaryColormap = XCreateColormap(display, browserDisplay.rootWindow,
	    DefaultVisual(display, screen), AllocNone);
    temporaryFont = XLoadFont(display, "fixed");
    if ((temporaryWindow == None) || (temporaryPixmap == None)
	    || (temporaryColormap == None) || (temporaryFont == None)) {
	goto cleanup;
    }
    if (XCloseDisplay(display) != Success) {
	goto cleanup;
    }
    resourcePtr = X11WasmFindResource((XID)temporaryWindow);
    if ((resourcePtr == NULL) || !resourcePtr->retained
	    || !resourcePtr->temporary) {
	goto cleanup;
    }
    resourcePtr = X11WasmFindResource((XID)temporaryPixmap);
    if ((resourcePtr == NULL) || !resourcePtr->retained
	    || !resourcePtr->temporary) {
	goto cleanup;
    }
    resourcePtr = X11WasmFindResource((XID)temporaryColormap);
    if ((resourcePtr == NULL) || !resourcePtr->retained
	    || !resourcePtr->temporary) {
	goto cleanup;
    }
    resourcePtr = X11WasmFindResource((XID)temporaryFont);
    if ((resourcePtr == NULL) || !resourcePtr->retained
	    || !resourcePtr->temporary) {
	goto cleanup;
    }
    if (XKillClient(display, AllTemporary) != Success) {
	goto cleanup;
    }
    if ((X11WasmFindResource((XID)temporaryWindow) != NULL)
	    || (X11WasmFindResource((XID)temporaryPixmap) != NULL)
	    || (X11WasmFindResource((XID)temporaryColormap) != NULL)
	    || (X11WasmFindResource((XID)temporaryFont) != NULL)
	    || (X11WasmFindResource((XID)permanentWindow) == NULL)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    (void)XSetCloseDownMode(display, DestroyAll);
    if (X11WasmFindResource((XID)destroyWindow) != NULL) {
	(void)XDestroyWindow(display, destroyWindow);
    }
    if (X11WasmFindResource((XID)temporaryFont) != NULL) {
	(void)XUnloadFont(display, temporaryFont);
    }
    if (X11WasmFindResource((XID)temporaryColormap) != NULL) {
	(void)XFreeColormap(display, temporaryColormap);
    }
    if (X11WasmFindResource((XID)temporaryPixmap) != NULL) {
	(void)XFreePixmap(display, temporaryPixmap);
    }
    if (X11WasmFindResource((XID)temporaryWindow) != NULL) {
	(void)XDestroyWindow(display, temporaryWindow);
    }
    if (X11WasmFindResource((XID)permanentWindow) != NULL) {
	(void)XDestroyWindow(display, permanentWindow);
    }
    return result;
}

void
XSetAuthorization(
    char *name,
    int nameLength,
    char *data,
    int dataLength)
{
    (void)name;
    (void)nameLength;
    (void)data;
    (void)dataLength;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestCompleteRequests(
    int noOpCount,
    int flushCount,
    int syncCount,
    int discard)
{
    int i;
    int result;

    BrowserInitDisplay();
    if ((noOpCount < 0) || (flushCount < 0) || (syncCount < 0)
	    || (noOpCount > 16) || (flushCount > 16) || (syncCount > 16)) {
	return BadValue;
    }
    for (i = 0; i < noOpCount; i++) {
	result = XNoOp(&browserDisplay.display);
	if (result != Success) {
	    return result;
	}
    }
    for (i = 0; i < flushCount; i++) {
	result = XFlush(&browserDisplay.display);
	if (result != Success) {
	    return result;
	}
    }
    for (i = 0; i < syncCount; i++) {
	result = XSync(&browserDisplay.display, discard ? True : False);
	if (result != Success) {
	    return result;
	}
    }
    return Success;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetServerGrabDepth(int depth)
{
    BrowserInitDisplay();
    if ((depth < 0) || (depth > 16)) {
	return BadValue;
    }
    while (browserDisplay.serverGrabDepth > depth) {
	(void)XUngrabServer(&browserDisplay.display);
    }
    while (browserDisplay.serverGrabDepth < depth) {
	(void)XGrabServer(&browserDisplay.display);
    }
    return Success;
}

int XForceScreenSaver(Display *display, int mode)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if ((mode != ScreenSaverReset) && (mode != ScreenSaverActive)) {
	(void)BrowserValueOrError(&display, mode, 0,
		X11_WASM_XREQ_FORCE_SCREEN_SAVER);
	return Success;
    }
    if (display == &browserDisplay.display) {
	BrowserForceScreenSaverMode(mode);
    }
    return XNoOp(display);
}
int XActivateScreenSaver(Display *display)
{ return XForceScreenSaver(display, ScreenSaverActive); }
int XResetScreenSaver(Display *display)
{ return XForceScreenSaver(display, ScreenSaverReset); }

int
XSetScreenSaver(
    Display *display,
    int timeout,
    int interval,
    int preferBlanking,
    int allowExposures)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (!BrowserValueOrError(&display, timeout, timeout >= -1,
	    X11_WASM_XREQ_SET_SCREEN_SAVER)
	    || !BrowserValueOrError(&display, interval, interval >= -1,
		    X11_WASM_XREQ_SET_SCREEN_SAVER)
	    || !BrowserValidateScreenSaverPreferBlanking(&display,
		    preferBlanking, X11_WASM_XREQ_SET_SCREEN_SAVER)
	    || !BrowserValidateScreenSaverAllowExposures(&display,
		    allowExposures, X11_WASM_XREQ_SET_SCREEN_SAVER)) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	timeout = (timeout == -1) ? X11_WASM_DEFAULT_SCREEN_SAVER_TIMEOUT
		: timeout;
	interval = (interval == -1) ? X11_WASM_DEFAULT_SCREEN_SAVER_INTERVAL
		: interval;
	browserDisplay.screenSaverTimeout = timeout;
	browserDisplay.screenSaverInterval = interval;
	browserDisplay.screenSaverPreferBlanking = preferBlanking;
	browserDisplay.screenSaverAllowExposures = allowExposures;
	(void)X11WasmJsSetScreenSaverSettings(timeout, interval,
		preferBlanking, allowExposures);
    }
    return XNoOp(display);
}

int
XGetScreenSaver(
    Display *display,
    int *timeoutReturn,
    int *intervalReturn,
    int *preferBlankingReturn,
    int *allowExposuresReturn)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (timeoutReturn != NULL) {
	*timeoutReturn = browserDisplay.screenSaverTimeout;
    }
    if (intervalReturn != NULL) {
	*intervalReturn = browserDisplay.screenSaverInterval;
    }
    if (preferBlankingReturn != NULL) {
	*preferBlankingReturn = browserDisplay.screenSaverPreferBlanking;
    }
    if (allowExposuresReturn != NULL) {
	*allowExposuresReturn = browserDisplay.screenSaverAllowExposures;
    }
    return XNoOp(display);
}

Bool
XScreenSaverQueryExtension(
    Display *display,
    int *eventBaseReturn,
    int *errorBaseReturn)
{
    return XQueryExtension(display, ScreenSaverName, NULL, eventBaseReturn,
	    errorBaseReturn);
}

Status
XScreenSaverQueryVersion(
    Display *display,
    int *majorVersionReturn,
    int *minorVersionReturn)
{
    (void)display;
    if (majorVersionReturn != NULL) {
	*majorVersionReturn = ScreenSaverMajorVersion;
    }
    if (minorVersionReturn != NULL) {
	*minorVersionReturn = ScreenSaverMinorVersion;
    }
    return True;
}

XScreenSaverInfo *
XScreenSaverAllocInfo(void)
{
    XScreenSaverInfo *infoPtr;

    infoPtr = (XScreenSaverInfo *)ckalloc(sizeof(XScreenSaverInfo));
    memset(infoPtr, 0, sizeof(XScreenSaverInfo));
    return infoPtr;
}

Status
XScreenSaverQueryInfo(
    Display *display,
    Drawable drawable,
    XScreenSaverInfo *infoPtr)
{
    X11WasmResource *resourcePtr;
    unsigned long idle;
    unsigned long elapsed;
    unsigned long timeout;

    if (infoPtr == NULL) {
	return False;
    }
    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_GET_SCREEN_SAVER);
    if (resourcePtr == NULL) {
	return False;
    }
    idle = BrowserScreenSaverIdle();
    timeout = (unsigned long)browserDisplay.screenSaverTimeout * 1000UL;
    memset(infoPtr, 0, sizeof(*infoPtr));
    infoPtr->window = (Window)resourcePtr->id;
    infoPtr->kind = ScreenSaverInternal;
    if (browserDisplay.screenSaverActive) {
	infoPtr->state = ScreenSaverOn;
	infoPtr->til_or_since = (idle >= browserDisplay.screenSaverActivatedIdle)
		? (idle - browserDisplay.screenSaverActivatedIdle) : 0UL;
    } else if (browserDisplay.screenSaverTimeout == 0) {
	infoPtr->state = ScreenSaverDisabled;
	infoPtr->til_or_since = 0UL;
    } else {
	infoPtr->state = ScreenSaverOff;
	elapsed = (idle >= browserDisplay.screenSaverTimerResetIdle)
		? (idle - browserDisplay.screenSaverTimerResetIdle) : 0UL;
	infoPtr->til_or_since = (timeout > elapsed) ? (timeout - elapsed) : 0UL;
    }
    infoPtr->idle = idle;
    infoPtr->eventMask = browserDisplay.screenSaverEventMask;
    return True;
}

void
XScreenSaverSelectInput(
    Display *display,
    Drawable drawable,
    unsigned long eventMask)
{
    X11WasmResource *resourcePtr;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    resourcePtr = X11WasmFindResource(drawable);
    if ((resourcePtr == NULL)
	    || ((resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
		    && (resourcePtr->type != X11_WASM_RESOURCE_PIXMAP))
	    || ((resourcePtr->type == X11_WASM_RESOURCE_WINDOW)
		    && (resourcePtr->c_class == InputOnly))) {
	(void)BrowserReportXError(display, BadDrawable, (XID)drawable,
		X11_WASM_SCREENSAVER_MAJOR_OPCODE,
		X11_WASM_SCREENSAVER_SELECT_INPUT_MINOR_OPCODE);
	return;
    }
    if (!BrowserScreenSaverEventMaskIsValid(eventMask)) {
	(void)BrowserReportXError(display, BadValue, (XID)eventMask,
		X11_WASM_SCREENSAVER_MAJOR_OPCODE,
		X11_WASM_SCREENSAVER_SELECT_INPUT_MINOR_OPCODE);
	return;
    }
    if (display == &browserDisplay.display) {
	browserDisplay.screenSaverEventMask = eventMask;
    }
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetScreenSaverSettings(int enabled)
{
    BrowserInitDisplay();
    if (!enabled) {
	return XSetScreenSaver(&browserDisplay.display, -1, -1,
		DefaultBlanking, DefaultExposures);
    }
    return XSetScreenSaver(&browserDisplay.display, 120, 30,
	    PreferBlanking, AllowExposures);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetScreenSaverWakeLock(int enabled)
{
    BrowserInitDisplay();
    if (!enabled) {
	return XSetScreenSaver(&browserDisplay.display, -1, -1,
		DefaultBlanking, DefaultExposures);
    }
    return XSetScreenSaver(&browserDisplay.display, 120, 30,
	    DontPreferBlanking, AllowExposures);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestForceScreenSaverState(void)
{
    Display *display = &browserDisplay.display;
    XEvent event;
    XScreenSaverInfo info;
    XScreenSaverNotifyEvent *notifyPtr;
    XErrorHandler oldHandler;
    int eventBase = -1;
    int errorBase = -1;
    int eventType;
    unsigned int keycode;
    int result = 0;

    BrowserInitDisplay();
    (void)XResetScreenSaver(display);
    XScreenSaverSelectInput(display, browserDisplay.rootWindow, 0);
    BrowserClearEventQueue();
    if ((XScreenSaverQueryExtension(display, &eventBase, &errorBase) != True)
	    || (eventBase != X11_WASM_SCREENSAVER_EVENT_BASE)
	    || (errorBase != 0)) {
	goto cleanup;
    }
    eventType = eventBase + ScreenSaverNotify;
    if ((XSetScreenSaver(display, 120, 30, PreferBlanking, AllowExposures)
		    != Success)
	    || (XScreenSaverQueryInfo(display, browserDisplay.rootWindow,
		    &info) != True)
	    || (info.window != browserDisplay.rootWindow)
	    || (info.state != ScreenSaverOff)
	    || (info.kind != ScreenSaverInternal)
	    || (info.eventMask != 0UL)
	    || (info.til_or_since > 120000UL)) {
	goto cleanup;
    }

    XScreenSaverSelectInput(display, browserDisplay.rootWindow,
	    ScreenSaverNotifyMask);
    if ((XScreenSaverQueryInfo(display, browserDisplay.rootWindow,
		    &info) != True)
	    || (info.eventMask != ScreenSaverNotifyMask)) {
	goto cleanup;
    }
    oldHandler = XSetErrorHandler(BrowserTestXErrorHandler);
    browserTestLastErrorCode = Success;
    browserTestLastRequestCode = 0;
    browserTestLastResourceId = None;
    browserTestLastSerial = 0;
    XScreenSaverSelectInput(display, browserDisplay.rootWindow,
	    ScreenSaverNotifyMask | (1UL << 7));
    (void)XSetErrorHandler(oldHandler);
    if ((browserTestLastErrorCode != BadValue)
	    || (browserTestLastRequestCode != X11_WASM_SCREENSAVER_MAJOR_OPCODE)
	    || (browserTestLastResourceId != (XID)(ScreenSaverNotifyMask
		    | (1UL << 7)))) {
	goto cleanup;
    }
    if ((XScreenSaverQueryInfo(display, browserDisplay.rootWindow,
		    &info) != True)
	    || (info.eventMask != ScreenSaverNotifyMask)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    if ((XActivateScreenSaver(display) != Success)
	    || (XCheckTypedEvent(display, eventType, &event) != True)) {
	goto cleanup;
    }
    notifyPtr = (XScreenSaverNotifyEvent *)&event;
    if ((notifyPtr->window != browserDisplay.rootWindow)
	    || (notifyPtr->root != browserDisplay.rootWindow)
	    || (notifyPtr->state != ScreenSaverOn)
	    || (notifyPtr->kind != ScreenSaverInternal)
	    || (notifyPtr->forced != True)) {
	goto cleanup;
    }
    if ((XResetScreenSaver(display) != Success)
	    || (XCheckTypedEvent(display, eventType, &event) != True)) {
	goto cleanup;
    }
    notifyPtr = (XScreenSaverNotifyEvent *)&event;
    if ((notifyPtr->state != ScreenSaverOff)
	    || (notifyPtr->kind != ScreenSaverInternal)
	    || (notifyPtr->forced != True)) {
	goto cleanup;
    }
    XScreenSaverSelectInput(display, browserDisplay.rootWindow,
	    ScreenSaverNotifyMask | ScreenSaverCycleMask);
    if ((XScreenSaverQueryInfo(display, browserDisplay.rootWindow,
		    &info) != True)
	    || (info.eventMask != (ScreenSaverNotifyMask
		    | ScreenSaverCycleMask))) {
	goto cleanup;
    }

    if ((XActivateScreenSaver(display) != Success)
	    || (XScreenSaverQueryInfo(display, browserDisplay.rootWindow,
		    &info) != True)
	    || (info.state != ScreenSaverOn)
	    || (info.kind != ScreenSaverInternal)
	    || (info.til_or_since > info.idle)) {
	goto cleanup;
    }

    if ((XSetScreenSaver(display, 0, 30, PreferBlanking, AllowExposures)
		    != Success)
	    || (XScreenSaverQueryInfo(display, browserDisplay.rootWindow,
		    &info) != True)
	    || (info.state != ScreenSaverOn)) {
	goto cleanup;
    }

    if ((XResetScreenSaver(display) != Success)
	    || (XScreenSaverQueryInfo(display, browserDisplay.rootWindow,
		    &info) != True)
	    || (info.state != ScreenSaverDisabled)
	    || (info.til_or_since != 0UL)) {
	goto cleanup;
    }

    if ((XSetScreenSaver(display, 120, 30, PreferBlanking, AllowExposures)
		    != Success)
	    || (XActivateScreenSaver(display) != Success)) {
	goto cleanup;
    }
    if ((XWarpPointer(display, None, browserDisplay.rootWindow, 0, 0, 0, 0,
		    2, 3) != Success)
	    || (XScreenSaverQueryInfo(display, browserDisplay.rootWindow,
		    &info) != True)
	    || (info.state != ScreenSaverOn)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    (void)X11WasmQueuePointerEvent(MotionNotify, browserDisplay.rootWindow,
	    0, 0, 4, 5, CurrentTime);
    if ((XScreenSaverQueryInfo(display, browserDisplay.rootWindow, &info)
		    != True)
	    || (info.state != ScreenSaverOff)
	    || (XCheckTypedEvent(display, eventType, &event) != True)) {
	goto cleanup;
    }
    notifyPtr = (XScreenSaverNotifyEvent *)&event;
    if ((notifyPtr->state != ScreenSaverOff)
	    || (notifyPtr->kind != ScreenSaverInternal)
	    || (notifyPtr->forced != False)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    keycode = (unsigned int)display->min_keycode;
    if ((XActivateScreenSaver(display) != Success) || (keycode == 0)) {
	goto cleanup;
    }
    (void)BrowserQueueKeyEventFields(KeyPress, browserDisplay.rootWindow,
	    keycode, NoSymbol, 0, 2, 3, CurrentTime, 0);
    if ((XScreenSaverQueryInfo(display, browserDisplay.rootWindow, &info)
		    != True)
	    || (info.state != ScreenSaverOff)) {
	goto cleanup;
    }

    result = 1;

cleanup:
    (void)XResetScreenSaver(display);
    XScreenSaverSelectInput(display, browserDisplay.rootWindow, 0);
    (void)XSetScreenSaver(display, -1, -1, DefaultBlanking, DefaultExposures);
    BrowserClearEventQueue();
    return result;
}

XAfterFunction
XSetAfterFunction(
    Display *display,
    XAfterFunction procedure)
{
    XAfterFunction oldAfterFunction = NULL;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (display == &browserDisplay.display) {
	oldAfterFunction = browserDisplay.afterFunction;
	browserDisplay.afterFunction = procedure;
	browserDisplay.synchronize =
		(procedure == BrowserSynchronizedAfterFunction) ? 1 : 0;
	BrowserSyncSynchronizeState(display);
    }
    return oldAfterFunction;
}

XAfterFunction
XSynchronize(
    Display *display,
    Bool onoff)
{
    XAfterFunction oldAfterFunction;

    oldAfterFunction = XSetAfterFunction(display,
	    onoff ? BrowserSynchronizedAfterFunction : NULL);
    (void)XNoOp(display);
    return oldAfterFunction;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetSynchronizeState(int enabled)
{
    BrowserInitDisplay();
    (void)XSynchronize(&browserDisplay.display, enabled ? True : False);
    return Success;
}

XErrorHandler
XSetErrorHandler(
    XErrorHandler handler)
{
    XErrorHandler oldHandler = (errorHandler != NULL) ? errorHandler
	    : BrowserDefaultErrorHandler;
    errorHandler = (handler != NULL) ? handler : BrowserDefaultErrorHandler;
    return oldHandler;
}

XIOErrorHandler
XSetIOErrorHandler(
    XIOErrorHandler handler)
{
    XIOErrorHandler oldHandler = (ioErrorHandler != NULL) ? ioErrorHandler
	    : BrowserDefaultIOErrorHandler;
    ioErrorHandler = (handler != NULL) ? handler : BrowserDefaultIOErrorHandler;
    return oldHandler;
}

EMSCRIPTEN_KEEPALIVE int
XGetErrorText(
    Display *display,
    int code,
    char *buffer,
    int length)
{
    (void)display;
    BrowserCopyString(buffer, length, BrowserErrorText(code));
    return Success;
}

EMSCRIPTEN_KEEPALIVE int
XGetErrorDatabaseText(
    Display *display,
    const char *name,
    const char *message,
    const char *defaultString,
    char *buffer,
    int length)
{
    int code;

    (void)display;
    if ((name != NULL) && (strcmp(name, "XProtoError") == 0)
	    && BrowserErrorCodeForName(message, &code)) {
	BrowserCopyString(buffer, length, BrowserErrorText(code));
    } else {
	BrowserCopyString(buffer, length, defaultString);
    }
    return Success;
}

EMSCRIPTEN_KEEPALIVE Bool
XQueryExtension(
    Display *display,
    const char *name,
    int *majorOpcodeReturn,
    int *firstEventReturn,
    int *firstErrorReturn)
{
    const X11WasmExtensionInfo *extensionPtr = BrowserFindExtension(name);

    (void)display;
    if (majorOpcodeReturn != NULL) {
	*majorOpcodeReturn = (extensionPtr != NULL)
		? extensionPtr->majorOpcode : 0;
    }
    if (firstEventReturn != NULL) {
	*firstEventReturn = (extensionPtr != NULL)
		? extensionPtr->firstEvent : 0;
    }
    if (firstErrorReturn != NULL) {
	*firstErrorReturn = (extensionPtr != NULL)
		? extensionPtr->firstError : 0;
    }
    return (extensionPtr != NULL) ? True : False;
}

EMSCRIPTEN_KEEPALIVE char **
XListExtensions(
    Display *display,
    int *nextensionsReturn)
{
    char **list;
    size_t count = browserExtensionCount;
    size_t i;

    (void)display;
    list = (char **)ckalloc(sizeof(char *) * (count + 1U));
    for (i = 0; i < count; i++) {
	list[i] = BrowserDupString(browserExtensions[i].name);
    }
    list[count] = NULL;
    if (nextensionsReturn != NULL) {
	*nextensionsReturn = (int)count;
    }
    return list;
}

void
XFreeExtensionList(
    char **list)
{
    int i;

    if (list != NULL) {
	for (i = 0; list[i] != NULL; i++) {
	    ckfree(list[i]);
	}
	ckfree(list);
    }
}
