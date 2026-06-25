#include "XlibInt.h"

static Bool
BrowserTestClientMessageDataPredicate(
    Display *display,
    XEvent *eventPtr,
    XPointer arg)
{
    long value = *((long *)arg);

    (void)display;
    return ((eventPtr != NULL) && (eventPtr->type == ClientMessage)
	    && (eventPtr->xclient.data.l[0] == value)) ? True : False;
}

static int browserTestAfterFunctionCount;

static int
BrowserTestAfterFunction(
    Display *display)
{
    if (display == &browserDisplay.display) {
	browserTestAfterFunctionCount++;
    }
    return Success;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestKeyboardAsciiMapping(void)
{
    Display *display;
    KeyCode keycode;
    KeySym lookupSym = NoSymbol;
    KeySym *mapping = NULL;
    XKeyEvent event;
    XIM inputMethod = NULL;
    XIC inputContext = NULL;
    XIC grabInputContext = NULL;
    Window focusWindow = None;
    Window grabWindow = None;
    char buffer[8];
    int keysymsPerKeycode = 0;
    int lookupLength;
    int keyboardGrabbed = 0;
    int result = 0;
    int screen;
    Status lookupStatus;

    /*
     * This broad compatibility hook returns section codes on failure so the
     * browser test can identify which cluster regressed.
     */
    BrowserInitDisplay();
    display = &browserDisplay.display;
    screen = DefaultScreen(display);
    keycode = XKeysymToKeycode(display, XK_a);
    if (keycode != 65) {
	goto cleanup;
    }
    mapping = XGetKeyboardMapping(display, keycode, 1, &keysymsPerKeycode);
    if ((mapping == NULL) || (keysymsPerKeycode < 2)
	    || (mapping[0] != XK_a) || (mapping[1] != XK_A)) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    event.type = KeyPress;
    event.display = display;
    event.window = browserDisplay.rootWindow;
    event.keycode = keycode;
    lookupLength = XLookupString(&event, buffer, (int)sizeof(buffer),
	    &lookupSym, NULL);
    if ((XKeycodeToKeysym(display, keycode, 0) != XK_a)
	    || (XKeycodeToKeysym(display, keycode, 1) != XK_A)
	    || (lookupLength != 1) || (buffer[0] != 'a')
	    || (lookupSym != XK_a)) {
	goto cleanup;
    }
    focusWindow = XCreateWindow(display, browserDisplay.rootWindow,
	    0, 0, 10, 10, 0, CopyFromParent, InputOutput,
	    DefaultVisual(display, screen), 0, NULL);
    grabWindow = XCreateWindow(display, browserDisplay.rootWindow,
	    12, 0, 10, 10, 0, CopyFromParent, InputOutput,
	    DefaultVisual(display, screen), 0, NULL);
    if ((focusWindow == None) || (grabWindow == None)
	    || (XMapWindow(display, focusWindow) != Success)
	    || (XMapWindow(display, grabWindow) != Success)) {
	goto cleanup;
    }
    inputMethod = XOpenIM(display, NULL, NULL, NULL);
    if (inputMethod == NULL) {
	goto cleanup;
    }
    inputContext = XCreateIC(inputMethod,
	    XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	    XNClientWindow, focusWindow,
	    XNFocusWindow, focusWindow,
	    NULL);
    if (inputContext == NULL) {
	goto cleanup;
    }
    grabInputContext = XCreateIC(inputMethod,
	    XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	    XNClientWindow, grabWindow,
	    XNFocusWindow, grabWindow,
	    NULL);
    if (grabInputContext == NULL) {
	goto cleanup;
    }
    XSetICFocus(inputContext);
    if (XGrabKeyboard(display, grabWindow, False, GrabModeAsync,
	    GrabModeAsync, CurrentTime) != GrabSuccess) {
	goto cleanup;
    }
    keyboardGrabbed = 1;
    memset(&event, 0, sizeof(event));
    event.type = KeyPress;
    event.display = display;
    event.window = grabWindow;
    event.root = browserDisplay.rootWindow;
    event.keycode = keycode;
    lookupSym = NoSymbol;
    lookupStatus = XLookupNone;
    lookupLength = Xutf8LookupString(inputContext, &event, buffer,
	    (int)sizeof(buffer), &lookupSym, &lookupStatus);
    if ((lookupLength != 1) || (buffer[0] != 'a') || (lookupSym != XK_a)
	    || ((lookupStatus != XLookupChars)
		    && (lookupStatus != XLookupBoth))) {
	goto cleanup;
    }
    lookupSym = NoSymbol;
    lookupStatus = XLookupNone;
    lookupLength = Xutf8LookupString(grabInputContext, &event, buffer,
	    (int)sizeof(buffer), &lookupSym, &lookupStatus);
    if ((lookupLength != 1) || (buffer[0] != 'a') || (lookupSym != XK_a)
	    || ((lookupStatus != XLookupChars)
		    && (lookupStatus != XLookupBoth))) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (keyboardGrabbed) {
	(void)XUngrabKeyboard(display, CurrentTime);
    }
    if (inputContext != NULL) {
	XDestroyIC(inputContext);
    }
    if (grabInputContext != NULL) {
	XDestroyIC(grabInputContext);
    }
    if (inputMethod != NULL) {
	(void)XCloseIM(inputMethod);
    }
    if (grabWindow != None) {
	(void)XDestroyWindow(display, grabWindow);
    }
    if (focusWindow != None) {
	(void)XDestroyWindow(display, focusWindow);
    }
    if (mapping != NULL) {
	XFree(mapping);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestWheelEventConstants(void)
{
    XEvent event;
    int internal = 0;

    BrowserInitDisplay();
    BrowserClearEventQueue();
    if ((X11_WASM_BROWSER_WHEEL_EVENT != (MappingNotify + 1))
	    || (MouseWheelEvent != X11_WASM_BROWSER_WHEEL_EVENT)
	    || (MouseWheelMask != X11_WASM_BROWSER_WHEEL_MASK)) {
	return 10;
    }
    if (!X11WasmQueueWheelEvent((unsigned int)browserDisplay.rootWindow, 120,
	    0, 12, 14, CurrentTime)) {
	return 11;
    }
    if (!BrowserPopEvent(&event, &internal)) {
	return 12;
    }
    BrowserClearEventQueue();
    if ((event.type != X11_WASM_BROWSER_WHEEL_EVENT) || !internal
	    || (event.xbutton.window != browserDisplay.rootWindow)
	    || (event.xbutton.button != 120U)
	    || (event.xbutton.x_root != 12)
	    || (event.xbutton.y_root != 14)) {
	return 13;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestEventQueueAndKeycodeApis(void)
{
    Display *display;
    XEvent firstEvent;
    XEvent secondEvent;
    XEvent exposeEvent;
    XEvent mapEvent;
    XEvent readEvent;
    XKeyboardControl keyboardControl;
    XKeyboardState keyboardState;
    XTimeCoord *motionEvents;
    char keymap[32];
    unsigned char pointerMap[5];
    unsigned char remappedPointerMap[5] = {
	Button2, Button1, Button3, Button4, Button5
    };
    unsigned char duplicatePointerMap[5] = {
	Button1, Button1, Button3, Button4, Button5
    };
    long predicateValue = 0;
    KeyCode testKeycode;
    unsigned int keyMask;
    unsigned int i;
    int accelNumerator = -1;
    int accelDenominator = -1;
    int motionEventCount = -1;
    Time firstMotionTime = 1000;
    Time secondMotionTime = 1010;
    Time thirdMotionTime = 1020;
    int pointerThreshold = -1;
    int minKeycode = -1;
    int maxKeycode = -1;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    BrowserClearEventQueue();
    browserDisplay.motionHistoryStart = 0;
    browserDisplay.motionHistoryCount = 0;

    if (XDisplayKeycodes(display, &minKeycode, &maxKeycode) != Success) {
	return 10;
    }
    if ((minKeycode != display->min_keycode)
	    || (maxKeycode != display->max_keycode)
	    || (minKeycode != 8) || (maxKeycode != 255)) {
	return 11;
    }
    if (XDisplayKeycodes(display, NULL, NULL) != Success) {
	return 12;
    }
    testKeycode = XKeysymToKeycode(display, XK_a);
    keyMask = 1U << (testKeycode & 7U);
    if (testKeycode == 0) {
	return 13;
    }

    memset(&keyboardState, 0, sizeof(keyboardState));
    if (XGetKeyboardControl(display, &keyboardState) != Success) {
	return 14;
    }
    if ((keyboardState.key_click_percent != 0)
	    || (keyboardState.bell_percent != 50)
	    || (keyboardState.bell_pitch != 400)
	    || (keyboardState.bell_duration != 100)
	    || (keyboardState.led_mask != 0)
	    || (keyboardState.global_auto_repeat != AutoRepeatModeOn)
	    || (((unsigned char)keyboardState.auto_repeats[testKeycode >> 3]
		    & keyMask) == 0)) {
	return 15;
    }

    memset(&keyboardControl, 0, sizeof(keyboardControl));
    keyboardControl.key_click_percent = 7;
    keyboardControl.bell_percent = 25;
    keyboardControl.bell_pitch = 880;
    keyboardControl.bell_duration = 120;
    keyboardControl.led = 3;
    keyboardControl.led_mode = LedModeOn;
    keyboardControl.auto_repeat_mode = AutoRepeatModeOff;
    if (XChangeKeyboardControl(display, KBKeyClickPercent | KBBellPercent
	    | KBBellPitch | KBBellDuration | KBLed | KBLedMode
	    | KBAutoRepeatMode, &keyboardControl) != Success) {
	return 16;
    }
    memset(&keyboardState, 0, sizeof(keyboardState));
    if ((XGetKeyboardControl(display, &keyboardState) != Success)
	    || (keyboardState.key_click_percent != 7)
	    || (keyboardState.bell_percent != 25)
	    || (keyboardState.bell_pitch != 880)
	    || (keyboardState.bell_duration != 120)
	    || (keyboardState.led_mask != (1UL << 2))
	    || (keyboardState.global_auto_repeat != AutoRepeatModeOff)) {
	return 17;
    }

    keyboardControl.key = testKeycode;
    keyboardControl.auto_repeat_mode = AutoRepeatModeOff;
    if (XChangeKeyboardControl(display, KBKey | KBAutoRepeatMode,
	    &keyboardControl) != Success) {
	return 18;
    }
    memset(&keyboardState, 0, sizeof(keyboardState));
    if ((XGetKeyboardControl(display, &keyboardState) != Success)
	    || (((unsigned char)keyboardState.auto_repeats[testKeycode >> 3]
		    & keyMask) != 0)) {
	return 19;
    }
    keyboardControl.auto_repeat_mode = AutoRepeatModeOn;
    if ((XChangeKeyboardControl(display, KBKey | KBAutoRepeatMode,
		    &keyboardControl) != Success)
	    || (XAutoRepeatOn(display) != Success)) {
	return 20;
    }
    memset(&keyboardState, 0, sizeof(keyboardState));
    if ((XGetKeyboardControl(display, &keyboardState) != Success)
	    || (((unsigned char)keyboardState.auto_repeats[testKeycode >> 3]
		    & keyMask) == 0)
	    || (keyboardState.global_auto_repeat != AutoRepeatModeOn)) {
	return 21;
    }

    memset(keymap, 0x7f, sizeof(keymap));
    if (XQueryKeymap(display, keymap) != Success) {
	return 22;
    }
    for (i = 0; i < sizeof(keymap); i++) {
	if (keymap[i] != 0) {
	    return 23;
	}
    }
    if (!BrowserQueueKeyEventFields(KeyPress, browserDisplay.rootWindow,
	    testKeycode, XK_a, 0, 1, 2, CurrentTime, 0)) {
	return 24;
    }
    memset(keymap, 0, sizeof(keymap));
    if ((XQueryKeymap(display, keymap) != Success)
	    || (((unsigned char)keymap[testKeycode >> 3] & keyMask) == 0)) {
	return 25;
    }
    if (!BrowserQueueKeyEventFields(KeyRelease, browserDisplay.rootWindow,
	    testKeycode, XK_a, 0, 1, 2, CurrentTime, 0)) {
	return 26;
    }
    memset(keymap, 0, sizeof(keymap));
    if ((XQueryKeymap(display, keymap) != Success)
	    || (((unsigned char)keymap[testKeycode >> 3] & keyMask) != 0)) {
	return 27;
    }
    BrowserClearEventQueue();

    memset(&keyboardControl, 0, sizeof(keyboardControl));
    keyboardControl.key_click_percent = 0;
    keyboardControl.bell_percent = 50;
    keyboardControl.bell_pitch = 400;
    keyboardControl.bell_duration = 100;
    keyboardControl.led = 3;
    keyboardControl.led_mode = LedModeOff;
    keyboardControl.auto_repeat_mode = AutoRepeatModeOn;
    if (XChangeKeyboardControl(display, KBKeyClickPercent | KBBellPercent
	    | KBBellPitch | KBBellDuration | KBLed | KBLedMode
	    | KBAutoRepeatMode, &keyboardControl) != Success) {
	return 28;
    }
    if ((XGetPointerControl(display, &accelNumerator, &accelDenominator,
		    &pointerThreshold) != Success)
	    || (accelNumerator != 1) || (accelDenominator != 1)
	    || (pointerThreshold != 0)) {
	return 90;
    }
    if (XChangePointerControl(display, True, True, 3, 2, 7) != Success) {
	return 91;
    }
    accelNumerator = accelDenominator = pointerThreshold = -1;
    if ((XGetPointerControl(display, &accelNumerator, &accelDenominator,
		    &pointerThreshold) != Success)
	    || (accelNumerator != 3) || (accelDenominator != 2)
	    || (pointerThreshold != 7)) {
	return 92;
    }
    if (XChangePointerControl(display, True, True, -1, -1, -1) != Success) {
	return 93;
    }
    accelNumerator = accelDenominator = pointerThreshold = -1;
    if ((XGetPointerControl(display, &accelNumerator, &accelDenominator,
		    &pointerThreshold) != Success)
	    || (accelNumerator != 1) || (accelDenominator != 1)
	    || (pointerThreshold != 0)) {
	return 94;
    }
    memset(pointerMap, 0, sizeof(pointerMap));
    if ((XGetPointerMapping(display, pointerMap, (int)sizeof(pointerMap))
		    != 5)
	    || (pointerMap[0] != Button1) || (pointerMap[1] != Button2)
	    || (pointerMap[2] != Button3) || (pointerMap[3] != Button4)
	    || (pointerMap[4] != Button5)) {
	return 95;
    }
    if (XSetPointerMapping(display, remappedPointerMap, 5)
	    != MappingSuccess) {
	return 96;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XNextEvent(display, &readEvent) != Success)
	    || (readEvent.type != MappingNotify)
	    || (readEvent.xmapping.request != MappingPointer)
	    || (readEvent.xmapping.first_keycode != 0)
	    || (readEvent.xmapping.count != 0)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 105;
    }
    memset(pointerMap, 0, sizeof(pointerMap));
    if ((XGetPointerMapping(display, pointerMap, (int)sizeof(pointerMap))
		    != 5)
	    || (memcmp(pointerMap, remappedPointerMap,
		    sizeof(pointerMap)) != 0)) {
	return 97;
    }
    if ((XSetPointerMapping(display, duplicatePointerMap, 5)
		    != MappingFailed)
	    || (XGetPointerMapping(display, pointerMap, (int)sizeof(pointerMap))
		    != 5)
	    || (memcmp(pointerMap, remappedPointerMap,
		    sizeof(pointerMap)) != 0)) {
	return 98;
    }
    if ((XSetPointerMapping(display, pointerMap, 4) != MappingFailed)
	    || (XSetPointerMapping(display, duplicatePointerMap, 0)
		    != MappingFailed)) {
	return 99;
    }
    pointerMap[0] = Button1;
    pointerMap[1] = Button2;
    pointerMap[2] = Button3;
    pointerMap[3] = Button4;
    pointerMap[4] = Button5;
    BrowserUpdatePointerState(browserDisplay.rootWindow, 5, 7, Button2Mask);
    if ((XSetPointerMapping(display, pointerMap, 5) != MappingBusy)
	    || (XEventsQueued(display, QueuedAlready) != 0)
	    || (XGetPointerMapping(display, pointerMap, (int)sizeof(pointerMap))
		    != 5)
	    || (memcmp(pointerMap, remappedPointerMap,
		    sizeof(pointerMap)) != 0)) {
	return 107;
    }
    BrowserUpdatePointerState(browserDisplay.rootWindow, 5, 7, 0);
    pointerMap[0] = Button1;
    pointerMap[1] = Button2;
    pointerMap[2] = Button3;
    pointerMap[3] = Button4;
    pointerMap[4] = Button5;
    if (XSetPointerMapping(display, pointerMap, 5) != MappingSuccess) {
	return 100;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XNextEvent(display, &readEvent) != Success)
	    || (readEvent.type != MappingNotify)
	    || (readEvent.xmapping.request != MappingPointer)
	    || (readEvent.xmapping.first_keycode != 0)
	    || (readEvent.xmapping.count != 0)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 106;
    }
    motionEvents = XGetMotionEvents(display, browserDisplay.rootWindow,
	    CurrentTime, CurrentTime, &motionEventCount);
    if ((motionEvents != NULL) || (motionEventCount != 0)) {
	return 101;
    }
    if (!X11WasmQueuePointerEvent(MotionNotify, browserDisplay.rootWindow, 0,
	    0, 11, 13, firstMotionTime)
	    || !X11WasmQueuePointerEvent(MotionNotify, browserDisplay.rootWindow,
		    0, 0, 21, 23, secondMotionTime)
	    || !X11WasmQueuePointerEvent(MotionNotify, browserDisplay.rootWindow,
		    0, 0, 31, 33, thirdMotionTime)) {
	return 102;
    }
    motionEventCount = -1;
    motionEvents = XGetMotionEvents(display, browserDisplay.rootWindow,
	    secondMotionTime, thirdMotionTime, &motionEventCount);
    if ((motionEvents == NULL) || (motionEventCount != 2)
	    || (motionEvents[0].time != secondMotionTime)
	    || (motionEvents[0].x != 21) || (motionEvents[0].y != 23)
	    || (motionEvents[1].time != thirdMotionTime)
	    || (motionEvents[1].x != 31) || (motionEvents[1].y != 33)) {
	if (motionEvents != NULL) {
	    XFree(motionEvents);
	}
	return 103;
    }
    XFree(motionEvents);
    motionEventCount = -1;
    motionEvents = XGetMotionEvents(display, browserDisplay.rootWindow,
	    firstMotionTime + 1, secondMotionTime - 1, &motionEventCount);
    if ((motionEvents != NULL) || (motionEventCount != 0)) {
	if (motionEvents != NULL) {
	    XFree(motionEvents);
	}
	return 104;
    }
    BrowserClearEventQueue();

    memset(&firstEvent, 0, sizeof(firstEvent));
    firstEvent.type = ClientMessage;
    firstEvent.xclient.display = display;
    firstEvent.xclient.window = browserDisplay.rootWindow;
    firstEvent.xclient.format = 32;
    firstEvent.xclient.data.l[0] = 1001;

    memset(&secondEvent, 0, sizeof(secondEvent));
    secondEvent.type = ClientMessage;
    secondEvent.xclient.display = display;
    secondEvent.xclient.window = browserDisplay.rootWindow;
    secondEvent.xclient.format = 32;
    secondEvent.xclient.data.l[0] = 1002;

    if (!BrowserQueueEvent(&firstEvent) || !BrowserQueueEvent(&secondEvent)) {
	return 20;
    }
    if ((XEventsQueued(display, QueuedAlready) != 2)
	    || (XPending(display) != 2)) {
	return 21;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XPeekEvent(display, &readEvent) != Success)
	    || (readEvent.type != ClientMessage)
	    || (readEvent.xclient.data.l[0] != 1001)
	    || (XEventsQueued(display, QueuedAlready) != 2)) {
	return 22;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XNextEvent(display, &readEvent) != Success)
	    || (readEvent.type != ClientMessage)
	    || (readEvent.xclient.data.l[0] != 1001)
	    || (XEventsQueued(display, QueuedAlready) != 1)) {
	return 23;
    }
    if ((XPutBackEvent(display, &readEvent) != Success)
	    || (XEventsQueued(display, QueuedAlready) != 2)) {
	return 24;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XNextEvent(display, &readEvent) != Success)
	    || (readEvent.type != ClientMessage)
	    || (readEvent.xclient.data.l[0] != 1001)) {
	return 25;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XNextEvent(display, &readEvent) != Success)
	    || (readEvent.type != ClientMessage)
	    || (readEvent.xclient.data.l[0] != 1002)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 26;
    }
    if ((XPutBackEvent(display, NULL) != Success)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 27;
    }

    if (!BrowserQueueEvent(&firstEvent) || !BrowserQueueEvent(&secondEvent)) {
	return 30;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XCheckTypedEvent(display, ClientMessage, &readEvent) != True)
	    || (readEvent.xclient.data.l[0] != 1001)
	    || (XEventsQueued(display, QueuedAlready) != 1)) {
	return 31;
    }
    predicateValue = 1002;
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XCheckIfEvent(display, &readEvent,
		    BrowserTestClientMessageDataPredicate,
		    (XPointer)&predicateValue) != True)
	    || (readEvent.xclient.data.l[0] != 1002)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 32;
    }
    predicateValue = 9999;
    memset(&readEvent, 0x7f, sizeof(readEvent));
    if ((XCheckIfEvent(display, &readEvent,
		    BrowserTestClientMessageDataPredicate,
		    (XPointer)&predicateValue) != False)
	    || (readEvent.type != 0)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 33;
    }

    firstEvent.xclient.window = browserDisplay.rootWindow;
    secondEvent.xclient.window = browserDisplay.rootWindow + 100;
    if (!BrowserQueueEvent(&firstEvent) || !BrowserQueueEvent(&secondEvent)) {
	return 40;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XCheckTypedWindowEvent(display, browserDisplay.rootWindow + 100,
		    ClientMessage, &readEvent) != True)
	    || (readEvent.xclient.data.l[0] != 1002)
	    || (XEventsQueued(display, QueuedAlready) != 1)) {
	return 41;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XCheckTypedWindowEvent(display, browserDisplay.rootWindow + 100,
		    ClientMessage, &readEvent) != False)
	    || (readEvent.type != 0)
	    || (XNextEvent(display, &readEvent) != Success)
	    || (readEvent.xclient.data.l[0] != 1001)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 42;
    }

    firstEvent.xclient.window = browserDisplay.rootWindow;
    secondEvent.xclient.window = browserDisplay.rootWindow;
    if (!BrowserQueueEvent(&firstEvent) || !BrowserQueueEvent(&secondEvent)) {
	return 50;
    }
    predicateValue = 1002;
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XPeekIfEvent(display, &readEvent,
		    BrowserTestClientMessageDataPredicate,
		    (XPointer)&predicateValue) != Success)
	    || (readEvent.xclient.data.l[0] != 1002)
	    || (XEventsQueued(display, QueuedAlready) != 2)) {
	return 51;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XIfEvent(display, &readEvent,
		    BrowserTestClientMessageDataPredicate,
		    (XPointer)&predicateValue) != Success)
	    || (readEvent.xclient.data.l[0] != 1002)
	    || (XEventsQueued(display, QueuedAlready) != 1)) {
	return 52;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XNextEvent(display, &readEvent) != Success)
	    || (readEvent.xclient.data.l[0] != 1001)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 53;
    }

    memset(&exposeEvent, 0, sizeof(exposeEvent));
    BrowserSetCommonEventFields(&exposeEvent, Expose,
	    browserDisplay.rootWindow, CurrentTime);
    exposeEvent.xexpose.window = browserDisplay.rootWindow;
    exposeEvent.xexpose.width = 5;
    exposeEvent.xexpose.height = 7;
    memset(&mapEvent, 0, sizeof(mapEvent));
    BrowserSetCommonEventFields(&mapEvent, MapNotify,
	    browserDisplay.rootWindow, CurrentTime);
    mapEvent.xmap.event = browserDisplay.rootWindow;
    mapEvent.xmap.window = browserDisplay.rootWindow;
    if (!BrowserQueueEvent(&firstEvent) || !BrowserQueueEvent(&exposeEvent)
	    || !BrowserQueueEvent(&mapEvent)) {
	return 60;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XCheckMaskEvent(display, ExposureMask, &readEvent) != True)
	    || (readEvent.type != Expose)
	    || (readEvent.xexpose.width != 5)
	    || (XEventsQueued(display, QueuedAlready) != 2)) {
	return 61;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XCheckWindowEvent(display, browserDisplay.rootWindow,
		    StructureNotifyMask, &readEvent) != True)
	    || (readEvent.type != MapNotify)
	    || (XEventsQueued(display, QueuedAlready) != 1)) {
	return 62;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XCheckWindowEvent(display, browserDisplay.rootWindow,
		    StructureNotifyMask, &readEvent) != False)
	    || (readEvent.type != 0)
	    || (XEventsQueued(display, QueuedAlready) != 1)) {
	return 63;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XNextEvent(display, &readEvent) != Success)
	    || (readEvent.type != ClientMessage)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 64;
    }
    if (!BrowserQueueEvent(&exposeEvent)) {
	return 65;
    }
    memset(&readEvent, 0, sizeof(readEvent));
    if ((XMaskEvent(display, ExposureMask, &readEvent) != Success)
	    || (readEvent.type != Expose)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	return 66;
    }
    BrowserClearEventQueue();
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestClearAreaApis(void)
{
    Display *display;
    XSetWindowAttributes attributes;
    XEvent event;
    Window window = None;
    int result = 0;
    int screen;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    screen = DefaultScreen(display);
    memset(&attributes, 0, sizeof(attributes));
    attributes.event_mask = ExposureMask;
    window = XCreateWindow(display, browserDisplay.rootWindow, 0, 0, 8, 6, 0,
	    CopyFromParent, InputOutput, DefaultVisual(display, screen),
	    CWEventMask, &attributes);
    if ((window == None) || (XMapWindow(display, window) != Success)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    if (XClearArea(display, window, 1, 2, 3, 2, True) != Success) {
	goto cleanup;
    }
    if (!BrowserPopEvent(&event, NULL)
	    || (event.type != Expose)
	    || (event.xexpose.window != window)
	    || (event.xexpose.x != 1)
	    || (event.xexpose.y != 2)
	    || (event.xexpose.width != 3)
	    || (event.xexpose.height != 2)
	    || (event.xexpose.count != 0)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    if (XClearArea(display, window, 2, 1, 4, 3, False) != Success) {
	goto cleanup;
    }
    if (XEventsQueued(display, QueuedAlready) != 0) {
	goto cleanup;
    }
    if (XClearWindow(display, window) != Success) {
	goto cleanup;
    }
    if (XEventsQueued(display, QueuedAlready) != 0) {
	goto cleanup;
    }
    if (XClearArea(display, window, 99, 0, 0, 0, True) != Success) {
	goto cleanup;
    }
    if (XEventsQueued(display, QueuedAlready) != 0) {
	goto cleanup;
    }
    result = 1;

cleanup:
    BrowserClearEventQueue();
    if (window != None) {
	(void)XDestroyWindow(display, window);
    }
    return result;
}

static int
BrowserTestQueryTreeOrder(
    Display *display,
    Window parent,
    const Window *expected,
    unsigned int expectedCount)
{
    Window rootReturn = None;
    Window parentReturn = None;
    Window *children = NULL;
    unsigned int childCount = 0;
    unsigned int i;
    int result = 0;

    if (!XQueryTree(display, parent, &rootReturn, &parentReturn, &children,
	    &childCount)) {
	goto cleanup;
    }
    if ((rootReturn != browserDisplay.rootWindow)
	    || (parentReturn != browserDisplay.rootWindow)
	    || (childCount != expectedCount)) {
	goto cleanup;
    }
    for (i = 0; i < expectedCount; i++) {
	if ((children == NULL) || (children[i] != expected[i])) {
	    goto cleanup;
	}
    }
    result = 1;

cleanup:
    if (children != NULL) {
	XFree(children);
    }
    return result;
}

static int
BrowserTestPopConfigureEvent(
    Window window,
    Window above)
{
    XEvent event;

    memset(&event, 0, sizeof(event));
    if (!BrowserPopEvent(&event, NULL)
	    || (event.type != ConfigureNotify)
	    || (event.xconfigure.event != window)
	    || (event.xconfigure.window != window)
	    || (event.xconfigure.above != above)) {
	return 0;
    }
    return 1;
}

static int
BrowserTestPopCirculateEvent(
    Window eventWindow,
    Window window,
    int place)
{
    XEvent event;

    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(eventWindow,
	    (eventWindow == window) ? StructureNotifyMask
	    : SubstructureNotifyMask, &event)
	    || (event.type != CirculateNotify)
	    || (event.xcirculate.event != eventWindow)
	    || (event.xcirculate.window != window)
	    || (event.xcirculate.place != place)) {
	return 0;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestConfigureStackingApis(void)
{
    Display *display;
    XSetWindowAttributes attributes;
    XWindowChanges changes;
    XWindowAttributes returnedAttributes;
    XEvent event;
    Window parent = None;
    Window bottom = None;
    Window middle = None;
    Window top = None;
    Window far = None;
    Window expected[4];
    Window restack[3];
    int result = 0;
    int screen;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    screen = DefaultScreen(display);
    memset(&attributes, 0, sizeof(attributes));
    attributes.event_mask = StructureNotifyMask;
    parent = XCreateWindow(display, browserDisplay.rootWindow, 0, 0, 100, 40, 0,
	    CopyFromParent, InputOutput, DefaultVisual(display, screen), 0, NULL);
    bottom = XCreateWindow(display, parent, 0, 0, 20, 20, 0, CopyFromParent,
	    InputOutput, DefaultVisual(display, screen), CWEventMask,
	    &attributes);
    middle = XCreateWindow(display, parent, 0, 0, 20, 20, 0, CopyFromParent,
	    InputOutput, DefaultVisual(display, screen), CWEventMask,
	    &attributes);
    top = XCreateWindow(display, parent, 0, 0, 20, 20, 0, CopyFromParent,
	    InputOutput, DefaultVisual(display, screen), CWEventMask,
	    &attributes);
    far = XCreateWindow(display, parent, 60, 0, 10, 10, 0, CopyFromParent,
	    InputOutput, DefaultVisual(display, screen), CWEventMask,
	    &attributes);
    if ((parent == None) || (bottom == None) || (middle == None)
	    || (top == None) || (far == None)
	    || (XMapWindow(display, parent) != Success)
	    || (XMapWindow(display, bottom) != Success)
	    || (XMapWindow(display, middle) != Success)
	    || (XMapWindow(display, top) != Success)
	    || (XMapWindow(display, far) != Success)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    expected[0] = bottom;
    expected[1] = middle;
    expected[2] = top;
    expected[3] = far;
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    memset(&changes, 0, sizeof(changes));
    changes.sibling = far;
    changes.stack_mode = TopIf;
    if (XConfigureWindow(display, bottom, CWSibling | CWStackMode, &changes)
	    != Success) {
	goto cleanup;
    }
    if (!BrowserTestPopConfigureEvent(bottom, None)) {
	goto cleanup;
    }
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    changes.sibling = middle;
    changes.stack_mode = TopIf;
    if (XConfigureWindow(display, bottom, CWSibling | CWStackMode, &changes)
	    != Success) {
	goto cleanup;
    }
    if (!BrowserTestPopConfigureEvent(bottom, middle)) {
	goto cleanup;
    }
    expected[0] = middle;
    expected[1] = bottom;
    expected[2] = top;
    expected[3] = far;
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    changes.sibling = middle;
    changes.stack_mode = BottomIf;
    if (XConfigureWindow(display, bottom, CWSibling | CWStackMode, &changes)
	    != Success) {
	goto cleanup;
    }
    if (!BrowserTestPopConfigureEvent(bottom, None)) {
	goto cleanup;
    }
    expected[0] = bottom;
    expected[1] = middle;
    expected[2] = top;
    expected[3] = far;
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    memset(&changes, 0, sizeof(changes));
    changes.stack_mode = Opposite;
    if (XConfigureWindow(display, far, CWStackMode, &changes) != Success) {
	goto cleanup;
    }
    if (!BrowserTestPopConfigureEvent(far, top)) {
	goto cleanup;
    }
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    if (XConfigureWindow(display, bottom, CWStackMode, &changes) != Success) {
	goto cleanup;
    }
    if (!BrowserTestPopConfigureEvent(bottom, far)) {
	goto cleanup;
    }
    expected[0] = middle;
    expected[1] = top;
    expected[2] = far;
    expected[3] = bottom;
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    if ((XLowerWindow(display, bottom) != Success)
	    || (XRaiseWindow(display, middle) != Success)) {
	goto cleanup;
    }
    if (!BrowserTestPopConfigureEvent(bottom, None)
	    || !BrowserTestPopConfigureEvent(middle, far)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    expected[0] = bottom;
    expected[1] = top;
    expected[2] = far;
    expected[3] = middle;
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    if (XSelectInput(display, parent, SubstructureNotifyMask) != Success) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    if (XCirculateSubwindowsUp(display, parent) != Success) {
	goto cleanup;
    }
    if (!BrowserTestPopCirculateEvent(bottom, bottom, PlaceOnTop)
	    || !BrowserTestPopCirculateEvent(parent, bottom, PlaceOnTop)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    expected[0] = top;
    expected[1] = far;
    expected[2] = middle;
    expected[3] = bottom;
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    if (XCirculateSubwindowsDown(display, parent) != Success) {
	goto cleanup;
    }
    if (!BrowserTestPopCirculateEvent(bottom, bottom, PlaceOnBottom)
	    || !BrowserTestPopCirculateEvent(parent, bottom, PlaceOnBottom)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    expected[0] = bottom;
    expected[1] = top;
    expected[2] = far;
    expected[3] = middle;
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    if (XCirculateSubwindows(display, parent, RaiseLowest) != Success) {
	goto cleanup;
    }
    if (!BrowserTestPopCirculateEvent(bottom, bottom, PlaceOnTop)
	    || !BrowserTestPopCirculateEvent(parent, bottom, PlaceOnTop)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    expected[0] = top;
    expected[1] = far;
    expected[2] = middle;
    expected[3] = bottom;
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    if (XSelectInput(display, parent, NoEventMask) != Success) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    restack[0] = middle;
    restack[1] = top;
    restack[2] = far;
    if (XRestackWindows(display, restack, 3) != Success) {
	goto cleanup;
    }
    if (!BrowserTestPopConfigureEvent(top, far)
	    || !BrowserTestPopConfigureEvent(far, None)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    expected[0] = far;
    expected[1] = top;
    expected[2] = middle;
    expected[3] = bottom;
    if (!BrowserTestQueryTreeOrder(display, parent, expected, 4)) {
	goto cleanup;
    }

    memset(&changes, 0, sizeof(changes));
    changes.x = 5;
    changes.y = 7;
    changes.width = 21;
    changes.height = 13;
    changes.border_width = 2;
    if (XConfigureWindow(display, top,
	    CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &changes)
	    != Success) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopEvent(&event, NULL)
	    || (event.type != ConfigureNotify)
	    || (event.xconfigure.event != top)
	    || (event.xconfigure.window != top)
	    || (event.xconfigure.x != 5)
	    || (event.xconfigure.y != 7)
	    || (event.xconfigure.width != 21)
	    || (event.xconfigure.height != 13)
	    || (event.xconfigure.border_width != 2)
	    || (event.xconfigure.above != None)
	    || (event.xconfigure.override_redirect != False)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    memset(&returnedAttributes, 0, sizeof(returnedAttributes));
    if (!XGetWindowAttributes(display, top, &returnedAttributes)
	    || (returnedAttributes.x != 5)
	    || (returnedAttributes.y != 7)
	    || (returnedAttributes.width != 21)
	    || (returnedAttributes.height != 13)
	    || (returnedAttributes.border_width != 2)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    BrowserClearEventQueue();
    if (parent != None) {
	(void)XDestroyWindow(display, parent);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSubstructureNotifyApis(void)
{
    Display *display;
    XSetWindowAttributes attributes;
    XWindowChanges changes;
    XEvent event;
    Window parent = None;
    Window child = None;
    Window destroyedChild = None;
    int result = 0;
    int screen;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    screen = DefaultScreen(display);

    parent = XCreateWindow(display, browserDisplay.rootWindow, 0, 0, 80, 60, 0,
	    CopyFromParent, InputOutput, DefaultVisual(display, screen), 0, NULL);
    if ((parent == None)
	    || (XSelectInput(display, parent, SubstructureNotifyMask)
		    != Success)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    child = XCreateWindow(display, parent, 1, 2, 12, 10, 0, CopyFromParent,
	    InputOutput, DefaultVisual(display, screen), 0, NULL);
    if (child == None) {
	goto cleanup;
    }
    if (!BrowserPopMatchingEvent(parent, SubstructureNotifyMask, &event)
	    || (event.type != CreateNotify)
	    || (event.xcreatewindow.parent != parent)
	    || (event.xcreatewindow.window != child)
	    || (event.xcreatewindow.x != 1)
	    || (event.xcreatewindow.y != 2)
	    || (event.xcreatewindow.width != 12)
	    || (event.xcreatewindow.height != 10)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }

    if (XSelectInput(display, child, StructureNotifyMask) != Success) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    if (XMapWindow(display, child) != Success) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(child, StructureNotifyMask, &event)
	    || (event.type != MapNotify)
	    || (event.xmap.event != child)
	    || (event.xmap.window != child)) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(parent, SubstructureNotifyMask, &event)
	    || (event.type != MapNotify)
	    || (event.xmap.event != parent)
	    || (event.xmap.window != child)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }

    memset(&changes, 0, sizeof(changes));
    changes.x = 3;
    changes.y = 4;
    changes.width = 20;
    changes.height = 16;
    if (XConfigureWindow(display, child, CWX | CWY | CWWidth | CWHeight,
	    &changes) != Success) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(child, StructureNotifyMask, &event)
	    || (event.type != ConfigureNotify)
	    || (event.xconfigure.event != child)
	    || (event.xconfigure.window != child)
	    || (event.xconfigure.x != 3)
	    || (event.xconfigure.y != 4)
	    || (event.xconfigure.width != 20)
	    || (event.xconfigure.height != 16)) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(parent, SubstructureNotifyMask, &event)
	    || (event.type != ConfigureNotify)
	    || (event.xconfigure.event != parent)
	    || (event.xconfigure.window != child)
	    || (event.xconfigure.x != 3)
	    || (event.xconfigure.y != 4)
	    || (event.xconfigure.width != 20)
	    || (event.xconfigure.height != 16)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }

    if (XUnmapWindow(display, child) != Success) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(child, StructureNotifyMask, &event)
	    || (event.type != UnmapNotify)
	    || (event.xunmap.event != child)
	    || (event.xunmap.window != child)
	    || (event.xunmap.from_configure != False)) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(parent, SubstructureNotifyMask, &event)
	    || (event.type != UnmapNotify)
	    || (event.xunmap.event != parent)
	    || (event.xunmap.window != child)
	    || (event.xunmap.from_configure != False)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }

    destroyedChild = child;
    if (XDestroyWindow(display, child) != Success) {
	goto cleanup;
    }
    child = None;
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(destroyedChild, StructureNotifyMask, &event)
	    || (event.type != DestroyNotify)
	    || (event.xdestroywindow.event != destroyedChild)
	    || (event.xdestroywindow.window != destroyedChild)) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(parent, SubstructureNotifyMask, &event)
	    || (event.type != DestroyNotify)
	    || (event.xdestroywindow.event != parent)
	    || (event.xdestroywindow.window != destroyedChild)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }

    child = XCreateWindow(display, parent, 7, 8, 9, 10, 0, CopyFromParent,
	    InputOutput, DefaultVisual(display, screen), 0, NULL);
    if ((child == None)
	    || (XSelectInput(display, child, StructureNotifyMask)
		    != Success)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    destroyedChild = child;
    if (XDestroySubwindows(display, parent) != Success) {
	goto cleanup;
    }
    child = None;
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(destroyedChild, StructureNotifyMask,
	    &event)
	    || (event.type != DestroyNotify)
	    || (event.xdestroywindow.event != destroyedChild)
	    || (event.xdestroywindow.window != destroyedChild)) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(parent, SubstructureNotifyMask, &event)
	    || (event.type != DestroyNotify)
	    || (event.xdestroywindow.event != parent)
	    || (event.xdestroywindow.window != destroyedChild)
	    || (XEventsQueued(display, QueuedAlready) != 0)
	    || !BrowserTestQueryTreeOrder(display, parent, NULL, 0)) {
	goto cleanup;
    }

    memset(&attributes, 0, sizeof(attributes));
    attributes.event_mask = NoEventMask;
    if (XChangeWindowAttributes(display, parent, CWEventMask, &attributes)
	    != Success) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    child = XCreateWindow(display, parent, 5, 6, 8, 7, 0, CopyFromParent,
	    InputOutput, DefaultVisual(display, screen), 0, NULL);
    if ((child == None)
	    || BrowserPopMatchingEvent(parent, SubstructureNotifyMask, &event)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }

    result = 1;

cleanup:
    BrowserClearEventQueue();
    if (child != None) {
	(void)XDestroyWindow(display, child);
    }
    if (parent != None) {
	(void)XDestroyWindow(display, parent);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestXlibApis(void)
{
    char buffer[64];
    int majorOpcode = -1;
    int firstEvent = -1;
    int firstError = -1;
    int extensionCount = -1;
    char **extensionList = NULL;
    int hostCount = -1;
    int screenSaverTimeout = -1;
    int screenSaverInterval = -1;
    int screenSaverPreferBlanking = -1;
    int screenSaverAllowExposures = -1;
    int screenSaverMajor = -1;
    int screenSaverMinor = -1;
    XScreenSaverInfo *screenSaverInfo = NULL;
    int screen;
    Screen *displayScreen = NULL;
    Screen *defaultDisplayScreen = NULL;
    unsigned int bestWidth = 0;
    unsigned int bestHeight = 0;
    unsigned long accessorSerial = 0;
    int result = 0;
    Bool hostAccessEnabled = False;
    XHostAddress hostAddresses[2];
    XHostAddress *hostList = NULL;
    XSetWindowAttributes attributes;
    Colormap firstColormap = None;
    Colormap secondColormap = None;
    Window window = None;
    Window notifyWindow = None;
    Window destroyedWindow = None;
    Window selectionInitialOwner = None;
    Window selectionAfterStaleTime = None;
    Window selectionAfterFutureTime = None;
    Window selectionAfterCurrentTime = None;
    Window focusInitialWindow = None;
    Window focusAfterStaleTime = None;
    Window focusAfterFutureTime = None;
    Window focusAfterCurrentTime = None;
    Window destroyParentWindow = None;
    Window destroyChildWindow = None;
    Window destroyGrandchildWindow = None;
    Window destroyedParentWindow = None;
    Window destroyedChildWindow = None;
    Window destroyedGrandchildWindow = None;
    Window simpleWindow = None;
    Window inputOnlyWindow = None;
    Window cursorWindow = None;
    Window invalidInputOnlyWindow = None;
    Window gravityParentWindow = None;
    Window gravitySouthEastWindow = None;
    Window gravityCenterWindow = None;
    Window gravityUnmapWindow = None;
    Window mapSubParentWindow = None;
    Window mapSubBottomWindow = None;
    Window mapSubMiddleWindow = None;
    Window mapSubTopWindow = None;
    Window reparentOldParentWindow = None;
    Window reparentNewParentWindow = None;
    Window reparentChildWindow = None;
    Window reparentRootReturn = None;
    Window reparentParentReturn = None;
    Window *reparentChildrenReturn = NULL;
    Window expectedSubwindowOrder[3];
    unsigned int reparentChildCount = 0;
    Pixmap pixmap = None;
    Pixmap copyExposurePixmap = None;
    Pixmap widePixmap = None;
    Pixmap textPixmap = None;
    XIM inputMethod = NULL;
    XIC inputContext = NULL;
    XVaNestedList preeditAttributes = NULL;
    XVaNestedList statusAttributes = NULL;
    XIMCallback destroyCallback;
    XIMCallback destroyCallbackReturn;
    X11WasmResource *pixmapPtr = NULL;
    X11WasmResource *simpleResourcePtr = NULL;
    XTextProperty listProp;
    XTextProperty titleProp;
    XTextProperty iconProp;
    XTextProperty machineProp;
    XTextProperty customProp;
    XTextProperty mbProp;
    XTextProperty utf8Prop;
    XTextProperty stdProp;
    XTextProperty wideProp;
    XTextProperty readProp;
    char *listValues[2];
    char *titleValues[1];
    char *iconValues[1];
    char *machineValues[1];
    char *customValues[1];
    char *mbValues[1];
    char *utf8Values[1];
    char *stdValues[1];
    wchar_t wideValue[7];
    wchar_t *wideValues[1];
    wchar_t fontWideText[3] = {'O', 'K', 0};
    wchar_t fontDrawWideText[2] = {'W', 0};
    wchar_t **wideList = NULL;
    int wideCount = -1;
    char **stringList = NULL;
    int stringCount = -1;
    char **listedFontNames = NULL;
    int listedFontCount = -1;
    XFontSet fontSet = NULL;
    char **missingCharsets = NULL;
    int missingCharsetCount = -1;
    char *defaultString = NULL;
    XFontSetExtents *fontSetExtents = NULL;
    XFontStruct **fontStructList = NULL;
    char **fontNameList = NULL;
    int fontSetFontCount = 0;
    int fontUtf8Width = 0;
    int fontWideWidth = 0;
    int fontLongWidth = 0;
    int fontCharCount = 0;
    unsigned short fontLongClampedWidth = 0;
    XRectangle fontInkExtent;
    XRectangle fontLogicalExtent;
    XRectangle fontOverallInkExtent;
    XRectangle fontOverallLogicalExtent;
    XRectangle fontPerCharInk[4];
    XRectangle fontPerCharLogical[4];
    XRectangle *fontLongPerCharLogical = NULL;
    Atom customTextAtom = None;
    Atom timestampSelection = None;
    XClassHint classHint;
    XClassHint classHintReturn;
    XSizeHints normalHints;
    XSizeHints normalHintsReturn;
    XSizeHints customHints;
    XSizeHints customHintsReturn;
    unsigned int oldSizeHints[15];
    XWMHints wmHints;
    XWMHints *wmHintsReturn = NULL;
    XWindowAttributes inputOnlyAttributesReturn;
    XWindowAttributes gravityAttributesReturn;
    XWindowAttributes subwindowAttributesReturn;
    XWindowAttributes simpleAttributesReturn;
    XWindowAttributes reparentAttributesReturn;
    Window colormapWindowList[2];
    Window *colormapWindowsReturn = NULL;
    int colormapWindowCount = -1;
    Window transientForReturn = None;
    Atom transientForActualType = None;
    int transientForActualFormat = 0;
    unsigned long transientForItemCount = 0;
    unsigned long transientForBytesAfter = 0;
    unsigned char *transientForData = NULL;
    Atom protocolAtoms[2];
    Atom *protocolAtomsReturn = NULL;
    int protocolAtomCount = -1;
    long suppliedHints = 0;
    long oldSizeHintsSupplied = USPosition | USSize | PAllHints;
    long allSizeHintsSupplied = oldSizeHintsSupplied | PBaseSize
	    | PWinGravity;
    long oldSizeHintsFlags = 0;
    Atom customHintsAtom = None;
    XIconSize iconSizes[2];
    XIconSize *iconSizesReturn = NULL;
    XIconSize *allocatedIconSize = NULL;
    XStandardColormap *allocatedStandardColormap = NULL;
    XStandardColormap standardColormaps[2];
    XStandardColormap standardColormapReturn;
    XStandardColormap *standardColormapsReturn = NULL;
    int iconSizeCount = -1;
    int standardColormapCount = -1;
    XPixmapFormatValues *pixmapFormats = NULL;
    int pixmapFormatCount = -1;
    int *depthList = NULL;
    int depthCount = -1;
    int focusInitialRevert = 0;
    int focusAfterStaleRevert = 0;
    int focusAfterFutureRevert = 0;
    int focusAfterCurrentRevert = 0;
    int inheritedBackgroundHasPixel = 0;
    int inheritedBorderHasPixel = 0;
    unsigned long inheritedBackgroundPixel = 0;
    unsigned long inheritedBorderPixel = 0;
    Pixmap inheritedBackgroundPixmap = None;
    Pixmap inheritedBorderPixmap = None;
    XVisualInfo visualTemplate;
    XVisualInfo *visualInfoList = NULL;
    int visualInfoCount = -1;
    XVisualInfo matchedVisualInfo;
    Atom *propertyAtoms = NULL;
    int propertyAtomCount = -1;
    Atom rotatePropertyAtoms[3];
    Atom rotateActualType = None;
    int rotateActualFormat = 0;
    unsigned long rotateNitems = 0;
    unsigned long rotateBytesAfter = 0;
    unsigned char *rotatePropertyData = NULL;
    char *fetchedCutBuffer = NULL;
    int cutBufferLength = -1;
    int cutBufferIndex;
    char cutBufferValues[8] = {'0', '1', '2', '3', '4', '5', '6', '7'};
    Atom standardColormapAtom = None;
    X11WasmProperty *netWmStateProperty = NULL;
    X11WasmResource *stateWindowPtr = NULL;
    Atom netWmStateAtom = None;
    Atom maxVertAtom = None;
    Atom maxHorzAtom = None;
    char *batchAtomNames[3];
    Atom batchAtoms[3];
    char *batchAtomNameReturns[3];
    Atom missingBatchAtom = None;
    int atomNameIndex;
    int netWmStateAddResult = False;
    int netWmStateRemoveResult = False;
    char *fetchedName = NULL;
    char *fetchedIconName = NULL;
    char *fontAtomName = NULL;
    char longText[40000];
    Font queryFont = None;
    Font textItemFont = None;
    Font cursorFont = None;
    Font cursorMaskFont = None;
    XFontStruct *queryFontStruct = NULL;
    XFontStruct *queryFontInfo = NULL;
    XFontStruct *queryGCFontInfo = NULL;
    GC inputOnlyGC = NULL;
    GC queryGC = NULL;
    GC textGC = NULL;
    Cursor testCursor = None;
    Cursor releasedCursor = None;
    XCharStruct textExtents;
    XTextItem textItems[3];
    XTextItem16 textItems16[2];
    XEvent exposureEvent;
    XColor parsedColor;
    XColor exactColor;
    XColor screenColor;
    XEvent structureEvent;
    XEvent visibilityEvent;
    XEvent colormapEvent;
    XEvent netWmStateEvent;
    int mapNotifyCount = 0;
    int unmapNotifyCount = 0;
    int subwindowEventCount = 0;
    unsigned long serialBeforeWait = 0;
    Time selectionOwnerTime = CurrentTime;
    Time focusChangeTime = CurrentTime;
    int textDirection = 0;
    int textAscent = 0;
    int textDescent = 0;
    XKeyPressedEvent lookupEvent;
    XMappingEvent mappingEvent;
    XEvent mappingNotifyEvent;
    XEvent filterEvent;
    KeySym lookupSym = NoSymbol;
    Status lookupStatus = 0;
    char lookupBuffer[8];
    int lookupLength = 0;
    long filterEventsReturn = 0;
    Window icClientWindowReturn = None;
    Window icFocusWindowReturn = None;
    XIMStyle icInputStyleReturn = 0;
    XPoint preeditSpot;
    XPoint preeditSpotReturn;
    XRectangle preeditArea;
    XRectangle preeditAreaReturn;
    XRectangle preeditAreaNeeded;
    XRectangle preeditAreaNeededReturn;
    XRectangle statusArea;
    XRectangle statusAreaReturn;
    XFontSet preeditFontSetReturn = NULL;
    unsigned long preeditForeground = 0x00112233UL;
    unsigned long preeditForegroundReturn = 0;
    unsigned long preeditBackground = 0x00445566UL;
    unsigned long preeditBackgroundReturn = 0;
    unsigned long statusBackground = 0x00010203UL;
    unsigned long statusBackgroundReturn = 0;
    int preeditLineSpace = 7;
    int preeditLineSpaceReturn = 0;
    Cursor preeditCursorReturn = None;
    KeyCode keyboardMappingKeycode = 0;
    KeyCode changedMappingKeycode = 230;
    KeyCode unicodeSnowmanKeycode = 0;
    KeySym *keyboardMapping = NULL;
    XImage *image16 = NULL;
    XImage *invalidFormatImage = NULL;
    XImage *invalidDepthImage = NULL;
    XImage *invalidXYBitmapDepthImage = NULL;
    XImage *invalidOffsetImage = NULL;
    XImage *invalidBitmapPadImage = NULL;
    XImage *shortStrideImage = NULL;
    XImage *bitmapImage = NULL;
    XImage *subImage16 = NULL;
    XImage *subBitmapImage = NULL;
    XImage *xyBitmapImage = NULL;
    XImage *xyBitmapReadback = NULL;
    XImage *xyBitmapSubImage = NULL;
    XImage *bitmapDataReadback = NULL;
    XImage *textReadback = NULL;
    Pixmap xyBitmapPixmap = None;
    Pixmap bitmapDataPixmap = None;
    GC xyBitmapGC = NULL;
    unsigned char image16Data[8];
    unsigned char bitmapImageData[4];
    unsigned char xyBitmapData[1];
    unsigned char xyBitmapSubImageData[16];
    int keysymsPerKeycode = -1;
    KeySym changedKeyboardMapping[2] = {XK_F13, XK_F14};
    KeySym unicodeSnowman =
	    (KeySym)(X11_WASM_UNICODE_KEYSYM_BASE | 0x2603U);
    KeySym lowerKeySym = NoSymbol;
    KeySym upperKeySym = NoSymbol;
    XModifierKeymap *modifierMap = NULL;
    int modifierIndex = 0;
    int modeSwitchFound = 0;
    int altModifierCount = 0;
    int metaModifierCount = 0;
    int shiftModifierCount = 0;
    unsigned long oldKeyboardMapSerial = 0;
    unsigned long oldModifierMapSerial = 0;
    int modifierBusyKeyPressed = 0;
    char *commandArgv[2];
    char **commandArgvReturn = NULL;
    int commandArgcReturn = -1;
    const char *contextDataOne = "context-one";
    const char *contextDataTwo = "context-two";
    XPointer contextDataReturn = NULL;
    XID contextRidOne = 0x01020304UL;
    XID contextRidTwo = 0x01020305UL;
    XContext contextOne = 101;
    XContext contextTwo = 102;
    XrmQuark quarkOne = NULLQUARK;
    XrmQuark quarkAgain = NULLQUARK;
    XrmQuark quarkTwo = NULLQUARK;
    XrmQuark uniqueQuarkOne = NULLQUARK;
    XrmQuark uniqueQuarkTwo = NULLQUARK;
    XContext stringContext = 0;
    XContext uniqueContext = 0;
    XSizeHints geometryHints;
    int geometryMask = 0;
    int geometryX = 0;
    int geometryY = 0;
    int geometryWidth = 0;
    int geometryHeight = 0;
    int geometryGravity = 0;
    unsigned int parsedWidth = 0;
    unsigned int parsedHeight = 0;
    XIOErrorHandler savedIOErrorHandler = ioErrorHandler;
    XAfterFunction oldAfterFunction = NULL;
    char *savedLocaleModifiers = NULL;
    char imAvailableToken = 0;
    char imCloseToken = 0;
    char imDelayedToken = 0;
    char imDestroyToken = 0;
    Region regionA = NULL;
    Region regionB = NULL;
    Region regionC = NULL;
    Region polygonRegion = NULL;
    XRectangle regionRect;
    XRectangle polygonExtents;
    XPoint polygonPoints[6];
    XGCValues copyGCValues;
    XGCValues stateGCValues;
    Atom pixelSizeAtom = None;
    unsigned long fontPropertyValue = 0;
    unsigned long oldFlushSerial = 0;
    unsigned long oldSyncSerial = 0;
    unsigned long oldCloseSerial = 0;
    const char latin1Text[4] = {'c', 'a', 'f', (char)0xe9};
    XChar2b latin1Text16[4] = {
	{0, 'c'}, {0, 'a'}, {0, 'f'}, {0, 0xe9}
    };
    int latin1TextWidth = 0;
    int latin1TextWidth16 = 0;
    XCharStruct latin1TextExtents;
    XCharStruct latin1TextExtents16;
    XCharStruct fontStructTextExtents;
    unsigned char hostAddressOne[4] = {127, 0, 0, 1};
    unsigned char hostAddressTwo[16] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
    };

    BrowserInitDisplay();
    BrowserResetHostAccessControl();
    result = 20;
    memset(&attributes, 0, sizeof(attributes));
    memset(hostAddresses, 0, sizeof(hostAddresses));
    memset(&listProp, 0, sizeof(listProp));
    memset(&titleProp, 0, sizeof(titleProp));
    memset(&iconProp, 0, sizeof(iconProp));
    memset(&machineProp, 0, sizeof(machineProp));
    memset(&customProp, 0, sizeof(customProp));
    memset(&mbProp, 0, sizeof(mbProp));
    memset(&utf8Prop, 0, sizeof(utf8Prop));
    memset(&stdProp, 0, sizeof(stdProp));
    memset(&wideProp, 0, sizeof(wideProp));
    memset(&readProp, 0, sizeof(readProp));
    memset(&classHint, 0, sizeof(classHint));
    memset(&classHintReturn, 0, sizeof(classHintReturn));
    memset(&normalHints, 0, sizeof(normalHints));
    memset(&normalHintsReturn, 0, sizeof(normalHintsReturn));
    memset(&customHints, 0, sizeof(customHints));
    memset(&customHintsReturn, 0, sizeof(customHintsReturn));
    memset(&wmHints, 0, sizeof(wmHints));
    memset(&destroyCallback, 0, sizeof(destroyCallback));
    memset(&destroyCallbackReturn, 0, sizeof(destroyCallbackReturn));
    memset(&inputOnlyAttributesReturn, 0, sizeof(inputOnlyAttributesReturn));
    memset(&gravityAttributesReturn, 0, sizeof(gravityAttributesReturn));
    memset(&subwindowAttributesReturn, 0, sizeof(subwindowAttributesReturn));
    memset(&textExtents, 0, sizeof(textExtents));
    memset(&latin1TextExtents, 0, sizeof(latin1TextExtents));
    memset(&fontInkExtent, 0, sizeof(fontInkExtent));
    memset(&fontLogicalExtent, 0, sizeof(fontLogicalExtent));
    memset(&fontOverallInkExtent, 0, sizeof(fontOverallInkExtent));
    memset(&fontOverallLogicalExtent, 0, sizeof(fontOverallLogicalExtent));
    memset(fontPerCharInk, 0, sizeof(fontPerCharInk));
    memset(fontPerCharLogical, 0, sizeof(fontPerCharLogical));
    memset(&exposureEvent, 0, sizeof(exposureEvent));
    memset(&parsedColor, 0, sizeof(parsedColor));
    memset(&exactColor, 0, sizeof(exactColor));
    memset(&screenColor, 0, sizeof(screenColor));
    memset(&structureEvent, 0, sizeof(structureEvent));
    memset(&visibilityEvent, 0, sizeof(visibilityEvent));
    memset(&colormapEvent, 0, sizeof(colormapEvent));
    memset(&netWmStateEvent, 0, sizeof(netWmStateEvent));
    memset(&lookupEvent, 0, sizeof(lookupEvent));
    memset(&mappingEvent, 0, sizeof(mappingEvent));
    memset(&filterEvent, 0, sizeof(filterEvent));
    memset(&matchedVisualInfo, 0, sizeof(matchedVisualInfo));
    memset(&preeditArea, 0, sizeof(preeditArea));
    memset(&preeditAreaReturn, 0, sizeof(preeditAreaReturn));
    memset(&preeditAreaNeeded, 0, sizeof(preeditAreaNeeded));
    memset(&preeditAreaNeededReturn, 0, sizeof(preeditAreaNeededReturn));
    memset(&statusArea, 0, sizeof(statusArea));
    memset(&statusAreaReturn, 0, sizeof(statusAreaReturn));
    memset(lookupBuffer, 0, sizeof(lookupBuffer));
    memset(image16Data, 0, sizeof(image16Data));
    memset(bitmapImageData, 0, sizeof(bitmapImageData));
    memset(xyBitmapData, 0, sizeof(xyBitmapData));
    memset(xyBitmapSubImageData, 0, sizeof(xyBitmapSubImageData));
    memset(iconSizes, 0, sizeof(iconSizes));
    memset(standardColormaps, 0, sizeof(standardColormaps));
    memset(batchAtoms, 0, sizeof(batchAtoms));
    memset(batchAtomNameReturns, 0, sizeof(batchAtomNameReturns));
    memset(&standardColormapReturn, 0, sizeof(standardColormapReturn));
    memset(&visualTemplate, 0, sizeof(visualTemplate));
    memset(&regionRect, 0, sizeof(regionRect));
    memset(&copyGCValues, 0, sizeof(copyGCValues));
    memset(&stateGCValues, 0, sizeof(stateGCValues));
    wideValue[0] = (wchar_t)'w';
    wideValue[1] = (wchar_t)'i';
    wideValue[2] = (wchar_t)'d';
    wideValue[3] = (wchar_t)'e';
    wideValue[4] = (wchar_t)' ';
    wideValue[5] = (wchar_t)0x2603U;
    wideValue[6] = 0;
    hostAddresses[0].family = FamilyInternet;
    hostAddresses[0].length = (int)sizeof(hostAddressOne);
    hostAddresses[0].address = (char *)hostAddressOne;
    hostAddresses[1].family = FamilyInternet6;
    hostAddresses[1].length = (int)sizeof(hostAddressTwo);
    hostAddresses[1].address = (char *)hostAddressTwo;
    XrmInitialize();
    if ((strcmp(XDisplayName(NULL), "browser") != 0)
	    || (strcmp(XDisplayName("custom:1"), "custom:1") != 0)
	    || (strcmp(XDefaultString(), "") != 0)
	    || (XResourceManagerString(&browserDisplay.display) != NULL)
	    || (XGetDefault(&browserDisplay.display, "wish", "geometry")
		    != NULL)) {
	return 0;
    }
    screen = DefaultScreen(&browserDisplay.display);
    displayScreen = ScreenOfDisplay(&browserDisplay.display, screen);
    defaultDisplayScreen = DefaultScreenOfDisplay(&browserDisplay.display);
    accessorSerial = LastKnownRequestProcessed(&browserDisplay.display);
    if ((displayScreen == NULL) || (defaultDisplayScreen != displayScreen)) {
	return 0;
    }
    if ((XConnectionNumber(&browserDisplay.display)
		    != ConnectionNumber(&browserDisplay.display))
	    || (XConnectionNumber(NULL)
		    != ConnectionNumber(&browserDisplay.display))
	    || (XDefaultScreen(&browserDisplay.display) != screen)
	    || (XDefaultScreen(NULL) != screen)
	    || (XDefaultRootWindow(&browserDisplay.display)
		    != DefaultRootWindow(&browserDisplay.display))
	    || (XDefaultRootWindow(NULL)
		    != DefaultRootWindow(&browserDisplay.display))
	    || (XRootWindow(&browserDisplay.display, screen)
		    != RootWindow(&browserDisplay.display, screen))
	    || (XDefaultVisual(&browserDisplay.display, screen)
		    != DefaultVisual(&browserDisplay.display, screen))
	    || (XDefaultGC(&browserDisplay.display, screen)
		    != DefaultGC(&browserDisplay.display, screen))
	    || (XBlackPixel(&browserDisplay.display, screen)
		    != BlackPixel(&browserDisplay.display, screen))
	    || (XWhitePixel(&browserDisplay.display, screen)
		    != WhitePixel(&browserDisplay.display, screen))
	    || (XQLength(&browserDisplay.display)
		    != QLength(&browserDisplay.display))
	    || (XDisplayWidth(&browserDisplay.display, screen)
		    != DisplayWidth(&browserDisplay.display, screen))
	    || (XDisplayHeight(&browserDisplay.display, screen)
		    != DisplayHeight(&browserDisplay.display, screen))
	    || (XDisplayWidthMM(&browserDisplay.display, screen)
		    != DisplayWidthMM(&browserDisplay.display, screen))
	    || (XDisplayHeightMM(&browserDisplay.display, screen)
		    != DisplayHeightMM(&browserDisplay.display, screen))
	    || (XDisplayPlanes(&browserDisplay.display, screen)
		    != DisplayPlanes(&browserDisplay.display, screen))
	    || (XDisplayCells(&browserDisplay.display, screen)
		    != DisplayCells(&browserDisplay.display, screen))
	    || (XScreenCount(&browserDisplay.display)
		    != ScreenCount(&browserDisplay.display))
	    || (XServerVendor(&browserDisplay.display)
		    != ServerVendor(&browserDisplay.display))
	    || (XProtocolVersion(&browserDisplay.display)
		    != ProtocolVersion(&browserDisplay.display))
	    || (XProtocolRevision(&browserDisplay.display)
		    != ProtocolRevision(&browserDisplay.display))
	    || (XVendorRelease(&browserDisplay.display)
		    != VendorRelease(&browserDisplay.display))
	    || (XDisplayString(&browserDisplay.display)
		    != DisplayString(&browserDisplay.display))
	    || (XDefaultDepth(&browserDisplay.display, screen)
		    != DefaultDepth(&browserDisplay.display, screen))
	    || (XDefaultColormap(&browserDisplay.display, screen)
		    != DefaultColormap(&browserDisplay.display, screen))
	    || (XBitmapUnit(&browserDisplay.display)
		    != BitmapUnit(&browserDisplay.display))
	    || (XBitmapBitOrder(&browserDisplay.display)
		    != BitmapBitOrder(&browserDisplay.display))
	    || (XBitmapPad(&browserDisplay.display)
		    != BitmapPad(&browserDisplay.display))
	    || (XImageByteOrder(&browserDisplay.display)
		    != ImageByteOrder(&browserDisplay.display))
	    || (browserDisplay.display.max_request_size != 65535U)
	    || (XMaxRequestSize(&browserDisplay.display) != 65535L)
	    || (XMaxRequestSize(NULL) != 65535L)
	    || (XExtendedMaxRequestSize(&browserDisplay.display) != 0L)
	    || (XDisplayMotionBufferSize(&browserDisplay.display)
		    != X11_WASM_MOTION_BUFFER_SIZE)
	    || (XAllPlanes() != AllPlanes)
	    || (XNextRequest(&browserDisplay.display)
		    != NextRequest(&browserDisplay.display))
	    || (XLastKnownRequestProcessed(&browserDisplay.display)
		    != accessorSerial)) {
	return 0;
    }
    if ((XScreenOfDisplay(&browserDisplay.display, screen) != displayScreen)
	    || (XScreenOfDisplay(NULL, screen) != displayScreen)
	    || (XDefaultScreenOfDisplay(&browserDisplay.display)
		    != defaultDisplayScreen)
	    || (XDefaultScreenOfDisplay(NULL) != defaultDisplayScreen)
	    || (XDisplayOfScreen(displayScreen)
		    != DisplayOfScreen(displayScreen))
	    || (XRootWindowOfScreen(displayScreen)
		    != RootWindowOfScreen(displayScreen))
	    || (XBlackPixelOfScreen(displayScreen)
		    != BlackPixelOfScreen(displayScreen))
	    || (XWhitePixelOfScreen(displayScreen)
		    != WhitePixelOfScreen(displayScreen))
	    || (XDefaultColormapOfScreen(displayScreen)
		    != DefaultColormapOfScreen(displayScreen))
	    || (XDefaultDepthOfScreen(displayScreen)
		    != DefaultDepthOfScreen(displayScreen))
	    || (XDefaultGCOfScreen(displayScreen)
		    != DefaultGCOfScreen(displayScreen))
	    || (XDefaultVisualOfScreen(displayScreen)
		    != DefaultVisualOfScreen(displayScreen))
	    || (XWidthOfScreen(displayScreen) != WidthOfScreen(displayScreen))
	    || (XHeightOfScreen(displayScreen) != HeightOfScreen(displayScreen))
	    || (XWidthMMOfScreen(displayScreen)
		    != WidthMMOfScreen(displayScreen))
	    || (XHeightMMOfScreen(displayScreen)
		    != HeightMMOfScreen(displayScreen))
	    || (XPlanesOfScreen(displayScreen)
		    != PlanesOfScreen(displayScreen))
	    || (XCellsOfScreen(displayScreen) != CellsOfScreen(displayScreen))
	    || (XMinCmapsOfScreen(displayScreen)
		    != MinCmapsOfScreen(displayScreen))
	    || (XMaxCmapsOfScreen(displayScreen)
		    != MaxCmapsOfScreen(displayScreen))
	    || (XDoesSaveUnders(displayScreen)
		    != DoesSaveUnders(displayScreen))
	    || (XDoesBackingStore(displayScreen)
		    != DoesBackingStore(displayScreen))
	    || (XEventMaskOfScreen(displayScreen)
		    != EventMaskOfScreen(displayScreen))
	    || (XScreenNumberOfScreen(displayScreen) != screen)) {
	return 0;
    }
    if ((XQueryBestCursor(&browserDisplay.display,
		    (Drawable)browserDisplay.rootWindow, 17, 19, &bestWidth,
		    &bestHeight) != True)
	    || (bestWidth != 17U) || (bestHeight != 19U)
	    || (XQueryBestSize(&browserDisplay.display, TileShape,
		    (Drawable)browserDisplay.rootWindow, 23, 29, &bestWidth,
		    &bestHeight) != True)
	    || (bestWidth != 23U) || (bestHeight != 29U)
	    || (XQueryBestStipple(&browserDisplay.display,
		    (Drawable)browserDisplay.rootWindow, 31, 37, &bestWidth,
		    &bestHeight) != True)
	    || (bestWidth != 31U) || (bestHeight != 37U)
	    || (XQueryBestTile(&browserDisplay.display,
		    (Drawable)browserDisplay.rootWindow, 41, 43, NULL,
		    &bestHeight) != True)
	    || (bestHeight != 43U)) {
	goto cleanup;
    }
    if ((XRootWindow(&browserDisplay.display, -1) != None)
	    || (XRootWindow(&browserDisplay.display,
		    XScreenCount(&browserDisplay.display)) != None)
	    || (XScreenOfDisplay(&browserDisplay.display, -1) != NULL)
	    || (XScreenOfDisplay(&browserDisplay.display,
		    XScreenCount(&browserDisplay.display)) != NULL)
	    || (XDefaultVisual(&browserDisplay.display,
		    XScreenCount(&browserDisplay.display)) != NULL)
	    || (XDefaultGC(&browserDisplay.display,
		    XScreenCount(&browserDisplay.display)) != NULL)
	    || (XDisplayWidth(&browserDisplay.display,
		    XScreenCount(&browserDisplay.display)) != 0)
	    || (XDefaultColormap(&browserDisplay.display,
		    XScreenCount(&browserDisplay.display)) != None)
	    || (XDisplayOfScreen(NULL) != NULL)
	    || (XRootWindowOfScreen(NULL) != None)
	    || (XDefaultVisualOfScreen(NULL) != NULL)
	    || (XDefaultGCOfScreen(NULL) != NULL)
	    || (XWidthOfScreen(NULL) != 0)
	    || (XCellsOfScreen(NULL) != 0)
	    || (XDoesSaveUnders(NULL) != False)
	    || (XDoesBackingStore(NULL) != NotUseful)
	    || (XEventMaskOfScreen(NULL) != 0L)
	    || (XScreenNumberOfScreen(NULL) != -1)) {
	return 0;
    }
    XFlushGC(&browserDisplay.display,
	    DefaultGC(&browserDisplay.display, screen));
    XFlushGC(NULL, NULL);
    XLockDisplay(&browserDisplay.display);
    XUnlockDisplay(&browserDisplay.display);
    XLockDisplay(NULL);
    XUnlockDisplay(NULL);
    if ((XInitThreads() != True) || (XFreeThreads() != True)
	    || (XSupportsLocale() != True)) {
	return 0;
    }
    if (LastKnownRequestProcessed(&browserDisplay.display) != accessorSerial) {
	return 0;
    }
    if ((XGetErrorText(&browserDisplay.display, BadWindow, buffer,
	    (int)sizeof(buffer)) != Success)
	    || (strcmp(buffer, "BadWindow") != 0)) {
	return 0;
    }
    if ((XGetErrorText(&browserDisplay.display, FirstExtensionError, buffer,
	    (int)sizeof(buffer)) != Success)
	    || (strcmp(buffer, "Extension error") != 0)) {
	return 0;
    }
    if ((XGetErrorDatabaseText(&browserDisplay.display, "XProtoError",
	    "missing", "fallback text", buffer, (int)sizeof(buffer))
	    != Success) || (strcmp(buffer, "fallback text") != 0)) {
	return 0;
    }
    if ((XGetErrorDatabaseText(&browserDisplay.display, "XProtoError",
	    "BadWindow", "fallback text", buffer, (int)sizeof(buffer))
	    != Success) || (strcmp(buffer, "BadWindow") != 0)) {
	return 0;
    }
    if ((XGetErrorDatabaseText(&browserDisplay.display, "XProtoError",
	    "3", "fallback text", buffer, (int)sizeof(buffer))
	    != Success) || (strcmp(buffer, "BadWindow") != 0)) {
	return 0;
    }
    if ((XGetErrorDatabaseText(&browserDisplay.display, "XProtoError",
	    "128", "fallback text", buffer, (int)sizeof(buffer))
	    != Success) || (strcmp(buffer, "Extension error") != 0)) {
	return 0;
    }
    if (XQueryExtension(&browserDisplay.display, "RENDER", &majorOpcode,
	    &firstEvent, &firstError) != False) {
	return 0;
    }
    if ((majorOpcode != 0) || (firstEvent != 0) || (firstError != 0)) {
	return 0;
    }
    majorOpcode = firstEvent = firstError = -1;
    if (XQueryExtension(&browserDisplay.display, X11_WASM_EXTENSION_NAME,
	    &majorOpcode, &firstEvent, &firstError) != True) {
	return 0;
    }
    if ((majorOpcode != X11_WASM_EXTENSION_MAJOR_OPCODE)
	    || (firstEvent != 0) || (firstError != 0)) {
	return 0;
    }
    majorOpcode = firstEvent = firstError = -1;
    if (XScreenSaverQueryExtension(&browserDisplay.display, &firstEvent,
	    &firstError) != True) {
	return 0;
    }
    if ((firstEvent != 0) || (firstError != 0)) {
	return 0;
    }
    if (XQueryExtension(&browserDisplay.display, ScreenSaverName,
	    &majorOpcode, &firstEvent, &firstError) != True) {
	return 0;
    }
    if ((majorOpcode != X11_WASM_SCREENSAVER_MAJOR_OPCODE)
	    || (firstEvent != 0) || (firstError != 0)) {
	return 0;
    }
    if ((XScreenSaverQueryVersion(&browserDisplay.display, &screenSaverMajor,
	    &screenSaverMinor) != True)
	    || (screenSaverMajor != ScreenSaverMajorVersion)
	    || (screenSaverMinor != ScreenSaverMinorVersion)) {
	return 0;
    }
    extensionList = XListExtensions(&browserDisplay.display, &extensionCount);
    if ((extensionList == NULL) || (extensionCount != 2)
	    || (extensionList[0] == NULL)
	    || (strcmp(extensionList[0], X11_WASM_EXTENSION_NAME) != 0)
	    || (extensionList[1] == NULL)
	    || (strcmp(extensionList[1], ScreenSaverName) != 0)
	    || (extensionList[2] != NULL)) {
	goto cleanup;
    }
    XFreeExtensionList(extensionList);
    extensionList = NULL;
    if ((XListHosts(&browserDisplay.display, &hostCount, &hostAccessEnabled)
	    != NULL) || (hostCount != 0) || (hostAccessEnabled != True)) {
	return 0;
    }
    if ((XDisableAccessControl(&browserDisplay.display) != Success)
	    || (XListHosts(&browserDisplay.display, &hostCount,
		    &hostAccessEnabled) != NULL)
	    || (hostCount != 0) || (hostAccessEnabled != False)) {
	goto cleanup;
    }
    if ((XEnableAccessControl(&browserDisplay.display) != Success)
	    || (XListHosts(&browserDisplay.display, &hostCount,
		    &hostAccessEnabled) != NULL)
	    || (hostCount != 0) || (hostAccessEnabled != True)) {
	goto cleanup;
    }
    if (XAddHost(&browserDisplay.display, &hostAddresses[0]) != Success) {
	goto cleanup;
    }
    hostList = XListHosts(&browserDisplay.display, &hostCount,
	    &hostAccessEnabled);
    if ((hostList == NULL) || (hostCount != 1) || (hostAccessEnabled != True)
	    || (hostList[0].family != FamilyInternet)
	    || (hostList[0].length != (int)sizeof(hostAddressOne))
	    || (memcmp(hostList[0].address, hostAddressOne,
		    sizeof(hostAddressOne)) != 0)) {
	goto cleanup;
    }
    XFree(hostList);
    hostList = NULL;
    if (XAddHost(&browserDisplay.display, &hostAddresses[0]) != Success) {
	goto cleanup;
    }
    hostList = XListHosts(&browserDisplay.display, &hostCount,
	    &hostAccessEnabled);
    if ((hostList == NULL) || (hostCount != 1)) {
	goto cleanup;
    }
    XFree(hostList);
    hostList = NULL;
    if (XAddHosts(&browserDisplay.display, hostAddresses, 2) != Success) {
	goto cleanup;
    }
    hostList = XListHosts(&browserDisplay.display, &hostCount,
	    &hostAccessEnabled);
    if ((hostList == NULL) || (hostCount != 2)
	    || (hostList[1].family != FamilyInternet6)
	    || (hostList[1].length != (int)sizeof(hostAddressTwo))
	    || (memcmp(hostList[1].address, hostAddressTwo,
		    sizeof(hostAddressTwo)) != 0)) {
	goto cleanup;
    }
    XFree(hostList);
    hostList = NULL;
    if (XRemoveHost(&browserDisplay.display, &hostAddresses[0]) != Success) {
	goto cleanup;
    }
    hostList = XListHosts(&browserDisplay.display, &hostCount,
	    &hostAccessEnabled);
    if ((hostList == NULL) || (hostCount != 1)
	    || (hostList[0].family != FamilyInternet6)) {
	goto cleanup;
    }
    XFree(hostList);
    hostList = NULL;
    if ((XRemoveHosts(&browserDisplay.display, hostAddresses, 2) != Success)
	    || (XListHosts(&browserDisplay.display, &hostCount,
		    &hostAccessEnabled) != NULL)
	    || (hostCount != 0) || (hostAccessEnabled != True)) {
	goto cleanup;
    }
    quarkOne = XrmStringToQuark("x11.wasm.context");
    quarkAgain = XrmStringToQuark("x11.wasm.context");
    quarkTwo = XrmPermStringToQuark("x11.wasm.other");
    uniqueQuarkOne = XrmUniqueQuark();
    uniqueQuarkTwo = XrmUniqueQuark();
    stringContext = XStringToContext("x11.wasm.context");
    uniqueContext = XUniqueContext();
    if ((XrmStringToQuark(NULL) != NULLQUARK)
	    || (XrmPermStringToQuark(NULL) != NULLQUARK)
	    || (XrmQuarkToString(NULLQUARK) != NULLSTRING)
	    || (quarkOne == NULLQUARK)
	    || (quarkOne != quarkAgain)
	    || (quarkTwo == NULLQUARK)
	    || (quarkTwo == quarkOne)
	    || (strcmp(XrmQuarkToString(quarkOne), "x11.wasm.context") != 0)
	    || (strcmp(XrmQuarkToString(quarkTwo), "x11.wasm.other") != 0)
	    || (stringContext != (XContext)quarkOne)
	    || (uniqueQuarkOne == NULLQUARK)
	    || (uniqueQuarkTwo == NULLQUARK)
	    || (uniqueQuarkOne == uniqueQuarkTwo)
	    || (uniqueContext == (XContext)uniqueQuarkOne)
	    || (uniqueContext == (XContext)uniqueQuarkTwo)
	    || (XrmQuarkToString(uniqueQuarkOne) != NULLSTRING)
	    || (XrmQuarkToString(99999) != NULLSTRING)) {
	goto cleanup;
    }
    if ((XFindContext(&browserDisplay.display, contextRidOne, contextOne,
	    &contextDataReturn) != XCNOENT)
	    || (XDeleteContext(&browserDisplay.display, contextRidOne,
		    contextOne) != XCNOENT)
	    || (XSaveContext(&browserDisplay.display, contextRidOne,
		    contextOne, contextDataOne) != Success)) {
	goto cleanup;
    }
    contextDataReturn = NULL;
    if ((XFindContext(&browserDisplay.display, contextRidOne, contextOne,
	    &contextDataReturn) != Success)
	    || (contextDataReturn != (XPointer)contextDataOne)
	    || (XFindContext(&browserDisplay.display, contextRidOne, contextTwo,
		    NULL) != XCNOENT)
	    || (XFindContext(&browserDisplay.display, contextRidTwo, contextOne,
		    NULL) != XCNOENT)) {
	goto cleanup;
    }
    if ((XSaveContext(&browserDisplay.display, contextRidOne, contextOne,
	    contextDataTwo) != Success)
	    || (XSaveContext(&browserDisplay.display, contextRidOne,
		    contextTwo, contextDataOne) != Success)
	    || (XSaveContext(&browserDisplay.display, contextRidTwo,
		    contextOne, contextDataOne) != Success)) {
	goto cleanup;
    }
    contextDataReturn = NULL;
    if ((XFindContext(&browserDisplay.display, contextRidOne, contextOne,
	    &contextDataReturn) != Success)
	    || (contextDataReturn != (XPointer)contextDataTwo)
	    || (XFindContext(&browserDisplay.display, contextRidOne, contextTwo,
		    NULL) != Success)
	    || (XFindContext(&browserDisplay.display, contextRidTwo, contextOne,
		    NULL) != Success)
	    || (XDeleteContext(&browserDisplay.display, contextRidOne,
		    contextOne) != Success)
	    || (XFindContext(&browserDisplay.display, contextRidOne, contextOne,
		    NULL) != XCNOENT)
	    || (XDeleteContext(&browserDisplay.display, contextRidOne,
		    contextOne) != XCNOENT)) {
	goto cleanup;
    }
    contextDataReturn = NULL;
    if ((XSaveContext(&browserDisplay.display, contextRidOne, contextOne,
	    contextDataOne) != Success)
	    || (XFindContext(&browserDisplay.display, contextRidOne, contextOne,
		    &contextDataReturn) != Success)
	    || (contextDataReturn != (XPointer)contextDataOne)
	    || (XDeleteContext(&browserDisplay.display, contextRidOne,
		    contextOne) != Success)
	    || (XFindContext(&browserDisplay.display, contextRidOne, contextOne,
		    NULL) != XCNOENT)) {
	goto cleanup;
    }
    geometryX = 1;
    geometryY = 2;
    parsedWidth = 3;
    parsedHeight = 4;
    geometryMask = XParseGeometry("=80x24+300-49", &geometryX, &geometryY,
	    &parsedWidth, &parsedHeight);
    if ((geometryMask != (XValue | YValue | WidthValue | HeightValue
		    | YNegative))
	    || (geometryX != 300) || (geometryY != -49)
	    || (parsedWidth != 80) || (parsedHeight != 24)) {
	goto cleanup;
    }
    geometryMask = XParseGeometry("-0-0", &geometryX, &geometryY,
	    &parsedWidth, &parsedHeight);
    if ((geometryMask != (XValue | YValue | XNegative | YNegative))
	    || (geometryX != 0) || (geometryY != 0)
	    || (parsedWidth != 80) || (parsedHeight != 24)) {
	goto cleanup;
    }
    geometryX = 7;
    geometryY = 8;
    parsedWidth = 9;
    parsedHeight = 10;
    geometryMask = XParseGeometry("bad", &geometryX, &geometryY,
	    &parsedWidth, &parsedHeight);
    if ((geometryMask != NoValue) || (geometryX != 7) || (geometryY != 8)
	    || (parsedWidth != 9) || (parsedHeight != 10)) {
	goto cleanup;
    }

    memset(&geometryHints, 0, sizeof(geometryHints));
    geometryMask = XWMGeometry(&browserDisplay.display,
	    DefaultScreen(&browserDisplay.display), "80x24+10-20", NULL, 2,
	    &geometryHints, &geometryX, &geometryY, &geometryWidth,
	    &geometryHeight, &geometryGravity);
    if ((geometryMask != (XValue | YValue | WidthValue | HeightValue
		    | YNegative))
	    || (geometryX != 10)
	    || (geometryY != (DisplayHeight(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)) - 20 - 24 - 4))
	    || (geometryWidth != 80) || (geometryHeight != 24)
	    || (geometryGravity != SouthWestGravity)) {
	goto cleanup;
    }

    memset(&geometryHints, 0, sizeof(geometryHints));
    geometryHints.flags = PBaseSize | PResizeInc | PMinSize | PMaxSize;
    geometryHints.base_width = 10;
    geometryHints.base_height = 20;
    geometryHints.width_inc = 5;
    geometryHints.height_inc = 10;
    geometryHints.min_width = 15;
    geometryHints.min_height = 25;
    geometryHints.max_width = 70;
    geometryHints.max_height = 90;
    geometryMask = XWMGeometry(&browserDisplay.display,
	    DefaultScreen(&browserDisplay.display), "4x3-10+12",
	    "9x9+1-2", 1, &geometryHints, &geometryX, &geometryY,
	    &geometryWidth, &geometryHeight, &geometryGravity);
    if ((geometryMask != (XValue | YValue | WidthValue | HeightValue
		    | XNegative))
	    || (geometryX != (DisplayWidth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)) - 10 - 30 - 2))
	    || (geometryY != 12) || (geometryWidth != 30)
	    || (geometryHeight != 50)
	    || (geometryGravity != NorthEastGravity)) {
	goto cleanup;
    }

    memset(&geometryHints, 0, sizeof(geometryHints));
    geometryHints.flags = PMinSize | PResizeInc;
    geometryHints.min_width = 20;
    geometryHints.min_height = 30;
    geometryHints.width_inc = 2;
    geometryHints.height_inc = 3;
    geometryMask = XWMGeometry(&browserDisplay.display,
	    DefaultScreen(&browserDisplay.display), NULL, "8x6-4-5", 3,
	    &geometryHints, &geometryX, &geometryY, &geometryWidth,
	    &geometryHeight, &geometryGravity);
    if ((geometryMask != (XNegative | YNegative))
	    || (geometryX != (DisplayWidth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)) - 4 - 36 - 6))
	    || (geometryY != (DisplayHeight(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)) - 5 - 48 - 6))
	    || (geometryWidth != 36) || (geometryHeight != 48)
	    || (geometryGravity != SouthEastGravity)) {
	goto cleanup;
    }

    memset(&geometryHints, 0, sizeof(geometryHints));
    geometryHints.flags = PResizeInc | PMaxSize;
    geometryHints.width_inc = 10;
    geometryHints.height_inc = 10;
    geometryHints.max_width = 50;
    geometryHints.max_height = 60;
    geometryMask = XWMGeometry(&browserDisplay.display,
	    DefaultScreen(&browserDisplay.display), "9x9", NULL, 0,
	    &geometryHints, &geometryX, &geometryY, &geometryWidth,
	    &geometryHeight, &geometryGravity);
    if ((geometryMask != (WidthValue | HeightValue))
	    || (geometryX != 0) || (geometryY != 0)
	    || (geometryWidth != 50) || (geometryHeight != 60)
	    || (geometryGravity != NorthWestGravity)) {
	goto cleanup;
    }
    keyboardMappingKeycode = XKeysymToKeycode(&browserDisplay.display, XK_a);
    keyboardMapping = XGetKeyboardMapping(&browserDisplay.display,
	    keyboardMappingKeycode, 1, &keysymsPerKeycode);
    if ((keyboardMapping == NULL) || (keysymsPerKeycode < 2)
	    || (keyboardMapping[0] != XK_a)
	    || (keyboardMapping[1] != XK_A)) {
	goto cleanup;
    }
    XFree(keyboardMapping);
    keyboardMapping = NULL;
    lookupEvent.keycode = keyboardMappingKeycode;
    if ((XKeycodeToKeysym(&browserDisplay.display, keyboardMappingKeycode, -1)
		!= NoSymbol)
	    || (XKeycodeToKeysym(&browserDisplay.display,
		    keyboardMappingKeycode, X11_WASM_KEYSYMS_PER_KEYCODE)
		!= NoSymbol)
	    || (XLookupKeysym(&lookupEvent, -1) != NoSymbol)
	    || (XLookupKeysym(&lookupEvent, X11_WASM_KEYSYMS_PER_KEYCODE)
		!= NoSymbol)
	    || (XkbKeycodeToKeysym(&browserDisplay.display,
		    keyboardMappingKeycode, 0, 0) != XK_a)
	    || (XkbKeycodeToKeysym(&browserDisplay.display,
		    keyboardMappingKeycode, 0, 1) != XK_A)
	    || (XkbKeycodeToKeysym(&browserDisplay.display,
		    keyboardMappingKeycode, 1, 0) != NoSymbol)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    oldKeyboardMapSerial = browserDisplay.keyboardMapSerial;
    if ((XChangeKeyboardMapping(&browserDisplay.display,
		    changedMappingKeycode, 2, changedKeyboardMapping, 1)
		    != Success)
	    || (browserDisplay.keyboardMapSerial <= oldKeyboardMapSerial)) {
	goto cleanup;
    }
    memset(&mappingNotifyEvent, 0, sizeof(mappingNotifyEvent));
    if ((XNextEvent(&browserDisplay.display, &mappingNotifyEvent) != Success)
	    || (mappingNotifyEvent.type != MappingNotify)
	    || (mappingNotifyEvent.xmapping.request != MappingKeyboard)
	    || (mappingNotifyEvent.xmapping.first_keycode
		    != changedMappingKeycode)
	    || (mappingNotifyEvent.xmapping.count != 1)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    keyboardMapping = XGetKeyboardMapping(&browserDisplay.display,
	    changedMappingKeycode, 1, &keysymsPerKeycode);
    lookupEvent.keycode = changedMappingKeycode;
    if ((keyboardMapping == NULL) || (keysymsPerKeycode < 2)
	    || (keyboardMapping[0] != XK_F13)
	    || (keyboardMapping[1] != XK_F14)
	    || (XKeycodeToKeysym(&browserDisplay.display,
		    changedMappingKeycode, 0) != XK_F13)
	    || (XKeycodeToKeysym(&browserDisplay.display,
		    changedMappingKeycode, 1) != XK_F14)
	    || (XLookupKeysym(&lookupEvent, 0) != XK_F13)
	    || (XLookupKeysym(&lookupEvent, 1) != XK_F14)
	    || (XKeysymToKeycode(&browserDisplay.display, XK_F13)
		    != changedMappingKeycode)) {
	goto cleanup;
    }
    XFree(keyboardMapping);
    keyboardMapping = NULL;
    unicodeSnowmanKeycode = XKeysymToKeycode(&browserDisplay.display,
	    unicodeSnowman);
    keyboardMapping = XGetKeyboardMapping(&browserDisplay.display,
	    unicodeSnowmanKeycode, 1, &keysymsPerKeycode);
    if ((unicodeSnowmanKeycode == 0) || (keyboardMapping == NULL)
	    || (keysymsPerKeycode < 1)
	    || (keyboardMapping[0] != unicodeSnowman)) {
	goto cleanup;
    }
    XFree(keyboardMapping);
    keyboardMapping = NULL;
    oldKeyboardMapSerial = browserDisplay.keyboardMapSerial;
    mappingEvent.request = MappingKeyboard;
    mappingEvent.first_keycode = unicodeSnowmanKeycode;
    mappingEvent.count = 1;
    if ((XRefreshKeyboardMapping(&mappingEvent) != Success)
	    || (browserDisplay.keyboardMapSerial <= oldKeyboardMapSerial)
	    || (XKeycodeToKeysym(&browserDisplay.display,
		    unicodeSnowmanKeycode, 0) == unicodeSnowman)) {
	goto cleanup;
    }
    unicodeSnowmanKeycode = XKeysymToKeycode(&browserDisplay.display,
	    unicodeSnowman);
    keyboardMapping = XGetKeyboardMapping(&browserDisplay.display,
	    unicodeSnowmanKeycode, 1, &keysymsPerKeycode);
    if ((unicodeSnowmanKeycode == 0) || (keyboardMapping == NULL)
	    || (keysymsPerKeycode < 1)
	    || (keyboardMapping[0] != unicodeSnowman)) {
	goto cleanup;
    }
    XFree(keyboardMapping);
    keyboardMapping = NULL;
    if ((XStringToKeysym("U2603") != unicodeSnowman)
	    || (XKeysymToString(unicodeSnowman) == NULL)
	    || (strcmp(XKeysymToString(unicodeSnowman), "U2603") != 0)
	    || (XStringToKeysym("U0001F600")
		!= (KeySym)(X11_WASM_UNICODE_KEYSYM_BASE | 0x1f600U))
	    || (strcmp(XKeysymToString((KeySym)(X11_WASM_UNICODE_KEYSYM_BASE
		    | 0x1f600U)), "U0001F600") != 0)
	    || (XStringToKeysym("U00E9") != (KeySym)0x00e9U)
	    || (XStringToKeysym("eacute") != (KeySym)0x00e9U)
	    || (strcmp(XKeysymToString((KeySym)0x00e9U), "eacute") != 0)
	    || (XStringToKeysym("adiaeresis") != (KeySym)0x00e4U)
	    || (strcmp(XKeysymToString((KeySym)0x00e4U), "adiaeresis") != 0)
	    || (XStringToKeysym("dead_acute") != (KeySym)0xfe51U)
	    || (strcmp(XKeysymToString((KeySym)0xfe51U), "dead_acute") != 0)
	    || (XStringToKeysym("XF86AudioRaiseVolume")
		    != (KeySym)0x1008ff13UL)
	    || (strcmp(XKeysymToString((KeySym)0x1008ff13UL),
		    "XF86AudioRaiseVolume") != 0)
	    || (XStringToKeysym("U0021") != XK_exclam)
	    || (strcmp(XKeysymToString(XK_exclam), "exclam") != 0)
	    || (XStringToKeysym("U0001") != NoSymbol)
	    || (XStringToKeysym("U007F") != NoSymbol)
	    || (XStringToKeysym("U00110000") != NoSymbol)
	    || (XStringToKeysym("0x1008ffff") != (KeySym)0x1008ffffUL)
	    || (XStringToKeysym("0x") != NoSymbol)
	    || (XStringToKeysym("0x100000000") != NoSymbol)) {
	goto cleanup;
    }
    XConvertCase(XK_A, &lowerKeySym, &upperKeySym);
    if ((lowerKeySym != XK_a) || (upperKeySym != XK_A)) {
	goto cleanup;
    }
    XConvertCase(XK_z, &lowerKeySym, &upperKeySym);
    if ((lowerKeySym != XK_z) || (upperKeySym != XK_Z)) {
	goto cleanup;
    }
    XConvertCase(XK_Eacute, &lowerKeySym, &upperKeySym);
    if ((lowerKeySym != XK_eacute) || (upperKeySym != XK_Eacute)) {
	goto cleanup;
    }
    XConvertCase(XK_oslash, &lowerKeySym, &upperKeySym);
    if ((lowerKeySym != XK_oslash) || (upperKeySym != XK_Oslash)) {
	goto cleanup;
    }
    XConvertCase(XK_THORN, &lowerKeySym, &upperKeySym);
    if ((lowerKeySym != XK_thorn) || (upperKeySym != XK_THORN)) {
	goto cleanup;
    }
    XConvertCase(XK_multiply, &lowerKeySym, &upperKeySym);
    if ((lowerKeySym != XK_multiply) || (upperKeySym != XK_multiply)) {
	goto cleanup;
    }
    upperKeySym = NoSymbol;
    XConvertCase(XK_B, NULL, &upperKeySym);
    if (upperKeySym != XK_B) {
	goto cleanup;
    }
    lowerKeySym = NoSymbol;
    XConvertCase(XK_b, &lowerKeySym, NULL);
    if (lowerKeySym != XK_b) {
	goto cleanup;
    }
    image16 = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 16, ZPixmap, 0,
	    (char *)image16Data, 3, 1, 32, 0);
    invalidFormatImage = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 24, 99, 0, NULL,
	    1, 1, 32, 0);
    invalidDepthImage = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 0, ZPixmap, 0, NULL,
	    1, 1, 32, 0);
    invalidXYBitmapDepthImage = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 8, XYBitmap, 0, NULL,
	    1, 1, 32, 0);
    invalidOffsetImage = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 1, XYBitmap, -1, NULL,
	    1, 1, 32, 0);
    invalidBitmapPadImage = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 24, ZPixmap, 0, NULL,
	    1, 1, 24, 0);
    shortStrideImage = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 16, ZPixmap, 0, NULL,
	    3, 1, 32, 2);
    bitmapImage = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 1, XYBitmap, 0,
	    (char *)bitmapImageData, 17, 1, 32, 0);
    if ((image16 == NULL) || (invalidFormatImage != NULL)
	    || (invalidDepthImage != NULL)
	    || (invalidXYBitmapDepthImage != NULL)
	    || (invalidOffsetImage != NULL)
	    || (invalidBitmapPadImage != NULL)
	    || (shortStrideImage != NULL)
	    || (bitmapImage == NULL)
	    || (image16->bits_per_pixel != 16)
	    || (image16->bytes_per_line != 8)
	    || (image16->red_mask != 0xf800)
	    || (image16->green_mask != 0x07e0)
	    || (image16->blue_mask != 0x001f)
	    || (bitmapImage->bits_per_pixel != 1)
	    || (bitmapImage->bytes_per_line != 4)
	    || (XPutPixel(image16, 0, 0, 0xf800UL) != Success)
	    || (XPutPixel(image16, 1, 0, 0x07e0UL) != Success)
	    || (XPutPixel(image16, 2, 0, 0x001fUL) != Success)
	    || (XPutPixel(bitmapImage, 16, 0, 1UL) != Success)
	    || (XGetPixel(image16, 0, 0) != 0xf800UL)
	    || (XGetPixel(image16, 1, 0) != 0x07e0UL)
	    || (XGetPixel(image16, 2, 0) != 0x001fUL)
	    || (XGetPixel(bitmapImage, 16, 0) != 1UL)
	    || (bitmapImageData[2] != 1U)) {
	goto cleanup;
    }
    subImage16 = XSubImage(image16, 1, 0, 2, 1);
    subBitmapImage = XSubImage(bitmapImage, 16, 0, 1, 1);
    if ((subImage16 == NULL) || (subBitmapImage == NULL)
	    || (subImage16->width != 2) || (subImage16->height != 1)
	    || (subImage16->bits_per_pixel != 16)
	    || (subBitmapImage->width != 1) || (subBitmapImage->height != 1)
	    || (subBitmapImage->bits_per_pixel != 1)
	    || (XGetPixel(subImage16, 0, 0) != 0x07e0UL)
	    || (XGetPixel(subImage16, 1, 0) != 0x001fUL)
	    || (XGetPixel(subBitmapImage, 0, 0) != 1UL)
	    || (XAddPixel(subImage16, 1) != Success)
	    || (XAddPixel(subBitmapImage, 1) != Success)
	    || (XGetPixel(subImage16, 0, 0) != 0x07e1UL)
	    || (XGetPixel(subImage16, 1, 0) != 0x0020UL)
	    || (XGetPixel(subBitmapImage, 0, 0) != 0UL)) {
	goto cleanup;
    }
    (void)XDestroyImage(subImage16);
    subImage16 = NULL;
    (void)XDestroyImage(subBitmapImage);
    subBitmapImage = NULL;
    memset(image16Data, 0, sizeof(image16Data));
    image16->byte_order = MSBFirst;
    image16->xoffset = 1;
    memset(bitmapImageData, 0, sizeof(bitmapImageData));
    bitmapImage->bitmap_bit_order = MSBFirst;
    bitmapImage->xoffset = 1;
    if ((XPutPixel(image16, 0, 0, 0x1234UL) != Success)
	    || (XGetPixel(image16, 0, 0) != 0x1234UL)
	    || (image16Data[2] != 0x12U)
	    || (image16Data[3] != 0x34U)
	    || (XPutPixel(bitmapImage, 0, 0, 1UL) != Success)
	    || (XGetPixel(bitmapImage, 0, 0) != 1UL)
	    || (bitmapImageData[0] != 0x40U)) {
	goto cleanup;
    }
    xyBitmapData[0] = 0x01U;
    xyBitmapPixmap = XCreatePixmap(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 2, 1,
	    (unsigned int)DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)));
    xyBitmapGC = (xyBitmapPixmap != None)
	    ? XCreateGC(&browserDisplay.display, xyBitmapPixmap, 0, NULL)
	    : NULL;
    xyBitmapImage = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 1, XYBitmap, 0,
	    (char *)xyBitmapData, 2, 1, 8, 1);
    if ((xyBitmapPixmap == None) || (xyBitmapGC == NULL)
	    || (xyBitmapImage == NULL)
	    || (XSetForeground(&browserDisplay.display, xyBitmapGC, 0xff0000UL)
		    != Success)
	    || (XSetBackground(&browserDisplay.display, xyBitmapGC, 0x0000ffUL)
		    != Success)
	    || (XPutImage(&browserDisplay.display, xyBitmapPixmap, xyBitmapGC,
		    xyBitmapImage, 0, 0, 0, 0, 2, 1) != Success)) {
	goto cleanup;
    }
    xyBitmapReadback = XGetImage(&browserDisplay.display, xyBitmapPixmap,
	    0, 0, 2, 1, AllPlanes, ZPixmap);
    if ((xyBitmapReadback == NULL)
	    || (XGetPixel(xyBitmapReadback, 0, 0) != 0xff0000UL)
	    || (XGetPixel(xyBitmapReadback, 1, 0) != 0x0000ffUL)) {
	goto cleanup;
    }
    if ((XSetForeground(&browserDisplay.display, xyBitmapGC, 0xaabbccUL)
	    != Success)
	    || (XSetBackground(&browserDisplay.display, xyBitmapGC, 0x445566UL)
		    != Success)
	    || (XSetPlaneMask(&browserDisplay.display, xyBitmapGC, 0x00ff00UL)
		    != Success)
	    || (XPutImage(&browserDisplay.display, xyBitmapPixmap, xyBitmapGC,
		    xyBitmapImage, 0, 0, 0, 0, 2, 1) != Success)) {
	goto cleanup;
    }
    (void)XDestroyImage(xyBitmapReadback);
    xyBitmapReadback = NULL;
    xyBitmapReadback = XGetImage(&browserDisplay.display, xyBitmapPixmap,
	    0, 0, 2, 1, AllPlanes, ZPixmap);
    if ((xyBitmapReadback == NULL)
	    || (XGetPixel(xyBitmapReadback, 0, 0) != 0xffbb00UL)
	    || (XGetPixel(xyBitmapReadback, 1, 0) != 0x0055ffUL)) {
	goto cleanup;
    }
    if ((XSetForeground(&browserDisplay.display, xyBitmapGC, 0xff0000UL)
	    != Success)
	    || (XSetBackground(&browserDisplay.display, xyBitmapGC, 0x0000ffUL)
		    != Success)
	    || (XSetPlaneMask(&browserDisplay.display, xyBitmapGC, AllPlanes)
		    != Success)
	    || (XPutImage(&browserDisplay.display, xyBitmapPixmap, xyBitmapGC,
		    xyBitmapImage, 0, 0, 0, 0, 2, 1) != Success)) {
	goto cleanup;
    }
    (void)XDestroyImage(xyBitmapReadback);
    xyBitmapReadback = NULL;
    xyBitmapReadback = XGetImage(&browserDisplay.display, xyBitmapPixmap,
	    0, 0, 2, 1, AllPlanes, ZPixmap);
    if ((xyBitmapReadback == NULL)
	    || (XGetPixel(xyBitmapReadback, 0, 0) != 0xff0000UL)
	    || (XGetPixel(xyBitmapReadback, 1, 0) != 0x0000ffUL)) {
	goto cleanup;
    }
    xyBitmapSubImage = XCreateImage(&browserDisplay.display,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)),
	    (unsigned int)DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), ZPixmap, 0,
	    (char *)xyBitmapSubImageData, 2, 1, 32, 0);
    if ((xyBitmapSubImage == NULL)
	    || (XGetSubImage(&browserDisplay.display,
		    (Drawable)xyBitmapPixmap, 0, 0, 2, 1, AllPlanes,
		    ZPixmap, xyBitmapSubImage, 0, 0) != xyBitmapSubImage)
	    || (XGetPixel(xyBitmapSubImage, 0, 0) != 0xff0000UL)
	    || (XGetPixel(xyBitmapSubImage, 1, 0) != 0x0000ffUL)) {
	goto cleanup;
    }
    bitmapDataPixmap = XCreatePixmapFromBitmapData(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, (char *)xyBitmapData, 2, 1,
	    0x0a0b0cUL, 0x010203UL,
	    (unsigned int)DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)));
    if (bitmapDataPixmap == None) {
	goto cleanup;
    }
    bitmapDataReadback = XGetImage(&browserDisplay.display, bitmapDataPixmap,
	    0, 0, 2, 1, AllPlanes, ZPixmap);
    if ((bitmapDataReadback == NULL)
	    || (XGetPixel(bitmapDataReadback, 0, 0) != 0x0a0b0cUL)
	    || (XGetPixel(bitmapDataReadback, 1, 0) != 0x010203UL)) {
	goto cleanup;
    }
    copyGCValues.graphics_exposures = False;
    if (XChangeGC(&browserDisplay.display, xyBitmapGC,
	    GCGraphicsExposures, &copyGCValues) != Success) {
	goto cleanup;
    }
    (void)XDestroyImage(xyBitmapReadback);
    xyBitmapReadback = NULL;
    if (XCopyArea(&browserDisplay.display, (Drawable)xyBitmapPixmap,
	    (Drawable)xyBitmapPixmap, xyBitmapGC, 0, 0, 1, 1, 1, 0)
	    != Success) {
	goto cleanup;
    }
    xyBitmapReadback = XGetImage(&browserDisplay.display, xyBitmapPixmap,
	    0, 0, 2, 1, AllPlanes, ZPixmap);
    if ((xyBitmapReadback == NULL)
	    || (XGetPixel(xyBitmapReadback, 0, 0) != 0xff0000UL)
	    || (XGetPixel(xyBitmapReadback, 1, 0) != 0xff0000UL)) {
	goto cleanup;
    }
    if ((XSetForeground(&browserDisplay.display, xyBitmapGC, 0x112233UL)
	    != Success)
	    || (XSetBackground(&browserDisplay.display, xyBitmapGC, 0x445566UL)
		    != Success)
	    || (XPutImage(&browserDisplay.display, xyBitmapPixmap, xyBitmapGC,
		    xyBitmapImage, 0, 0, 0, 0, 2, 1) != Success)
	    || (XSetPlaneMask(&browserDisplay.display, xyBitmapGC, 0x00ff00UL)
		    != Success)
	    || (XCopyArea(&browserDisplay.display, (Drawable)xyBitmapPixmap,
		    (Drawable)xyBitmapPixmap, xyBitmapGC, 0, 0, 1, 1, 1, 0)
		    != Success)) {
	goto cleanup;
    }
    (void)XDestroyImage(xyBitmapReadback);
    xyBitmapReadback = NULL;
    xyBitmapReadback = XGetImage(&browserDisplay.display, xyBitmapPixmap,
	    0, 0, 2, 1, AllPlanes, ZPixmap);
    if ((xyBitmapReadback == NULL)
	    || (XGetPixel(xyBitmapReadback, 0, 0) != 0x112233UL)
	    || (XGetPixel(xyBitmapReadback, 1, 0) != 0x442266UL)) {
	goto cleanup;
    }
    if ((XSetForeground(&browserDisplay.display, xyBitmapGC, 0xff0000UL)
	    != Success)
	    || (XSetBackground(&browserDisplay.display, xyBitmapGC, 0xff0000UL)
		    != Success)
	    || (XSetPlaneMask(&browserDisplay.display, xyBitmapGC, AllPlanes)
		    != Success)
	    || (XPutImage(&browserDisplay.display, xyBitmapPixmap, xyBitmapGC,
		    xyBitmapImage, 0, 0, 0, 0, 2, 1) != Success)) {
	goto cleanup;
    }
    (void)XDestroyImage(xyBitmapReadback);
    xyBitmapReadback = NULL;
    xyBitmapReadback = XGetImage(&browserDisplay.display, xyBitmapPixmap,
	    0, 0, 2, 1, AllPlanes, ZPixmap);
    if ((xyBitmapReadback == NULL)
	    || (XGetPixel(xyBitmapReadback, 0, 0) != 0xff0000UL)
	    || (XGetPixel(xyBitmapReadback, 1, 0) != 0xff0000UL)) {
	goto cleanup;
    }
    copyGCValues.foreground = 0x00ff00UL;
    copyGCValues.background = 0x0000ffUL;
    if ((XChangeGC(&browserDisplay.display, xyBitmapGC,
		    GCForeground | GCBackground, &copyGCValues) != Success)
	    || (XCopyPlane(&browserDisplay.display, (Drawable)xyBitmapPixmap,
		    (Drawable)xyBitmapPixmap, xyBitmapGC, 0, 0, 1, 1, 1, 0,
		    0x800000UL) != Success)) {
	goto cleanup;
    }
    (void)XDestroyImage(xyBitmapReadback);
    xyBitmapReadback = NULL;
    xyBitmapReadback = XGetImage(&browserDisplay.display, xyBitmapPixmap,
	    0, 0, 2, 1, AllPlanes, ZPixmap);
    if ((xyBitmapReadback == NULL)
	    || (XGetPixel(xyBitmapReadback, 0, 0) != 0xff0000UL)
	    || (XGetPixel(xyBitmapReadback, 1, 0) != 0x00ff00UL)) {
	goto cleanup;
    }
    if ((XSetState(&browserDisplay.display, xyBitmapGC, 0x010203UL,
		    0x040506UL, GXinvert, 0x00ffffUL) != Success)
	    || (XSetPlaneMask(&browserDisplay.display, xyBitmapGC,
		    0x0000ffUL) != Success)
	    || (XSetGraphicsExposures(&browserDisplay.display, xyBitmapGC,
		    False) != Success)
	    || (XSetSubwindowMode(&browserDisplay.display, xyBitmapGC,
		    IncludeInferiors) != Success)
	    || (XGetGCValues(&browserDisplay.display, xyBitmapGC,
		    GCForeground | GCBackground | GCFunction | GCPlaneMask
		    | GCGraphicsExposures | GCSubwindowMode,
		    &stateGCValues) != True)
	    || (stateGCValues.foreground != 0x010203UL)
	    || (stateGCValues.background != 0x040506UL)
	    || (stateGCValues.function != GXinvert)
	    || (stateGCValues.plane_mask != 0x0000ffUL)
	    || (stateGCValues.graphics_exposures != False)
	    || (stateGCValues.subwindow_mode != IncludeInferiors)) {
	goto cleanup;
    }
    regionA = XCreateRegion();
    regionB = XCreateRegion();
    regionC = XCreateRegion();
    if ((regionA == NULL) || (regionB == NULL) || (regionC == NULL)
	    || !XEmptyRegion(regionA) || !XEqualRegion(regionA, regionB)
	    || XPointInRegion(regionA, 2, 3)
	    || !XEqualRegion(NULL, regionC)) {
	goto cleanup;
    }
    BrowserSetRectangleBounds(&regionRect, (int)SHRT_MAX + 1,
	    (int)SHRT_MIN - 1, (int)SHRT_MAX + 4, (int)SHRT_MIN + 3);
    if ((regionRect.x != SHRT_MAX) || (regionRect.y != SHRT_MIN)
	    || (regionRect.width != 3) || (regionRect.height != 4)) {
	goto cleanup;
    }
    BrowserSetRectangleBounds(&regionRect, 2, 3, 12, 11);
    (void)XUnionRectWithRegion(&regionRect, regionA, regionA);
    if (XEmptyRegion(regionA)
	    || !XPointInRegion(regionA, 2, 3)
	    || !XPointInRegion(regionA, 11, 10)
	    || XPointInRegion(regionA, 12, 10)
	    || XPointInRegion(regionA, 11, 11)
	    || XEqualRegion(regionA, regionB)) {
	goto cleanup;
    }
    BrowserSetRectangleBounds(&regionRect, 2, 3, 7, 11);
    (void)XUnionRectWithRegion(&regionRect, regionB, regionB);
    BrowserSetRectangleBounds(&regionRect, 7, 3, 12, 11);
    (void)XUnionRectWithRegion(&regionRect, regionB, regionB);
    if (!XEqualRegion(regionA, regionB)) {
	goto cleanup;
    }
    (void)XOffsetRegion(regionB, 1, 0);
    if (XEqualRegion(regionA, regionB)
	    || XPointInRegion(regionB, 2, 3)
	    || !XPointInRegion(regionB, 12, 10)) {
	goto cleanup;
    }
    (void)XOffsetRegion(regionB, -1, 0);
    (void)XUnionRegion(regionA, NULL, regionC);
    if (!XEqualRegion(regionA, regionB)
	    || !XEqualRegion(regionA, regionC)) {
	goto cleanup;
    }
    (void)XSubtractRegion(regionC, regionC, regionC);
    BrowserSetRectangleBounds(&regionRect, 1, 2, 3, 4);
    (void)XUnionRectWithRegion(&regionRect, regionC, regionC);
    (void)XOffsetRegion(regionC, SHRT_MAX, SHRT_MAX);
    (void)XClipBox(regionC, &regionRect);
    if ((regionRect.x != SHRT_MAX) || (regionRect.y != SHRT_MAX)
	    || (regionRect.width != 2) || (regionRect.height != 2)) {
	goto cleanup;
    }
    (void)XSubtractRegion(regionC, regionC, regionC);
    BrowserSetRectangleBounds(&regionRect, -1, -2, 1, 1);
    (void)XUnionRectWithRegion(&regionRect, regionC, regionC);
    (void)XOffsetRegion(regionC, SHRT_MIN, SHRT_MIN);
    (void)XClipBox(regionC, &regionRect);
    if ((regionRect.x != SHRT_MIN) || (regionRect.y != SHRT_MIN)
	    || (regionRect.width != 2) || (regionRect.height != 3)) {
	goto cleanup;
    }
    (void)XSubtractRegion(regionC, regionC, regionC);
    (void)XXorRegion(regionA, regionB, regionC);
    if (!XEmptyRegion(regionC)) {
	goto cleanup;
    }
    (void)XSubtractRegion(regionB, regionB, regionB);
    BrowserSetRectangleBounds(&regionRect, 7, 3, 15, 11);
    (void)XUnionRectWithRegion(&regionRect, regionB, regionB);
    (void)XXorRegion(regionA, regionB, regionC);
    if (!XPointInRegion(regionC, 2, 3)
	    || XPointInRegion(regionC, 8, 4)
	    || !XPointInRegion(regionC, 14, 10)) {
	goto cleanup;
    }
    (void)XXorRegion(regionA, regionB, regionA);
    if (!XEqualRegion(regionA, regionC)) {
	goto cleanup;
    }
    (void)XSubtractRegion(regionB, regionB, regionB);
    (void)XXorRegion(regionA, regionB, regionC);
    if (!XEqualRegion(regionA, regionC)) {
	goto cleanup;
    }
    (void)XSubtractRegion(regionA, regionA, regionA);
    BrowserSetRectangleBounds(&regionRect, 10, 20, 20, 34);
    (void)XUnionRectWithRegion(&regionRect, regionA, regionA);
    (void)XShrinkRegion(regionA, 2, 3);
    if (!XPointInRegion(regionA, 12, 23)
	    || !XPointInRegion(regionA, 17, 30)
	    || XPointInRegion(regionA, 11, 23)
	    || XPointInRegion(regionA, 12, 22)
	    || XPointInRegion(regionA, 18, 30)
	    || XPointInRegion(regionA, 17, 31)) {
	goto cleanup;
    }
    (void)XShrinkRegion(regionA, -1, -2);
    if (!XPointInRegion(regionA, 11, 21)
	    || !XPointInRegion(regionA, 18, 32)
	    || XPointInRegion(regionA, 10, 21)
	    || XPointInRegion(regionA, 11, 20)
	    || XPointInRegion(regionA, 19, 32)
	    || XPointInRegion(regionA, 18, 33)) {
	goto cleanup;
    }
    (void)XShrinkRegion(regionA, 20, 0);
    if (!XEmptyRegion(regionA)) {
	goto cleanup;
    }
    polygonPoints[0].x = 2;
    polygonPoints[0].y = 3;
    polygonPoints[1].x = 12;
    polygonPoints[1].y = 3;
    polygonPoints[2].x = 12;
    polygonPoints[2].y = 11;
    polygonPoints[3].x = 2;
    polygonPoints[3].y = 11;
    polygonRegion = XPolygonRegion(polygonPoints, 4, EvenOddRule);
    memset(&polygonExtents, 0, sizeof(polygonExtents));
    (void)XClipBox(polygonRegion, &polygonExtents);
    if ((polygonRegion == NULL) || XEmptyRegion(polygonRegion)
	    || (polygonExtents.x != 2) || (polygonExtents.y != 3)
	    || (polygonExtents.width != 10)
	    || (polygonExtents.height != 8)
	    || !XPointInRegion(polygonRegion, 2, 3)
	    || !XPointInRegion(polygonRegion, 11, 10)
	    || XPointInRegion(polygonRegion, 12, 10)
	    || XPointInRegion(polygonRegion, 11, 11)) {
	goto cleanup;
    }
    XDestroyRegion(polygonRegion);
    polygonRegion = NULL;
    polygonPoints[0].x = 0;
    polygonPoints[0].y = 0;
    polygonPoints[1].x = 6;
    polygonPoints[1].y = 0;
    polygonPoints[2].x = 6;
    polygonPoints[2].y = 6;
    polygonPoints[3].x = 3;
    polygonPoints[3].y = 6;
    polygonPoints[4].x = 3;
    polygonPoints[4].y = 3;
    polygonPoints[5].x = 0;
    polygonPoints[5].y = 3;
    polygonRegion = XPolygonRegion(polygonPoints, 6, WindingRule);
    if ((polygonRegion == NULL) || XEmptyRegion(polygonRegion)
	    || !XPointInRegion(polygonRegion, 1, 1)
	    || !XPointInRegion(polygonRegion, 4, 4)
	    || XPointInRegion(polygonRegion, 1, 4)
	    || XPointInRegion(polygonRegion, 6, 5)) {
	goto cleanup;
    }
    XDestroyRegion(polygonRegion);
    polygonRegion = NULL;
    polygonRegion = XPolygonRegion(polygonPoints, 2, EvenOddRule);
    if ((polygonRegion == NULL) || !XEmptyRegion(polygonRegion)) {
	goto cleanup;
    }
    XDestroyRegion(polygonRegion);
    polygonRegion = NULL;
    modifierMap = XGetModifierMapping(&browserDisplay.display);
    if ((modifierMap == NULL) || (modifierMap->max_keypermod < 2)) {
	goto cleanup;
    }
    for (modifierIndex = 0;
	    modifierIndex < 8 * modifierMap->max_keypermod;
	    modifierIndex++) {
	KeyCode keycode = modifierMap->modifiermap[modifierIndex];
	int mapIndex = modifierIndex / modifierMap->max_keypermod;
	KeySym keySym;

	if (keycode == 0) {
	    continue;
	}
	keySym = XKeycodeToKeysym(&browserDisplay.display, keycode, 0);
	if ((keySym == XK_Mode_switch) && (mapIndex == Mod5MapIndex)) {
	    modeSwitchFound = 1;
	} else if (((keySym == XK_Alt_L) || (keySym == XK_Alt_R))
		&& (mapIndex == Mod1MapIndex)) {
	    altModifierCount++;
	} else if (((keySym == XK_Meta_L) || (keySym == XK_Meta_R))
		&& (mapIndex == Mod4MapIndex)) {
	    metaModifierCount++;
	} else if (((keySym == XK_Shift_L) || (keySym == XK_Shift_R))
		&& (mapIndex == ShiftMapIndex)) {
	    shiftModifierCount++;
	}
    }
    if (!modeSwitchFound || (altModifierCount < 2)
	    || (metaModifierCount < 2) || (shiftModifierCount < 2)) {
	goto cleanup;
    }
    XFreeModifiermap(modifierMap);
    modifierMap = XNewModifiermap(1);
    if ((modifierMap == NULL) || (modifierMap->max_keypermod != 1)
	    || (modifierMap->modifiermap == NULL)) {
	goto cleanup;
    }
    modifierMap = XInsertModifiermapEntry(modifierMap,
	    (KeyCode)X11_WASM_KEYCODE_SEMICOLON, Mod3MapIndex);
    modifierMap = XInsertModifiermapEntry(modifierMap,
	    (KeyCode)X11_WASM_KEYCODE_SEMICOLON, ShiftMapIndex);
    oldModifierMapSerial = browserDisplay.modifierMapSerial;
    if ((modifierMap == NULL)
	    || (XSetModifierMapping(&browserDisplay.display, modifierMap)
		    != MappingFailed)
	    || (browserDisplay.modifierMapSerial != oldModifierMapSerial)) {
	goto cleanup;
    }
    modifierMap = XDeleteModifiermapEntry(modifierMap,
	    (KeyCode)X11_WASM_KEYCODE_SEMICOLON, ShiftMapIndex);
    modifierMap = XInsertModifiermapEntry(modifierMap,
	    (KeyCode)X11_WASM_KEYCODE_EQUAL, Mod3MapIndex);
    modifierMap = XDeleteModifiermapEntry(modifierMap,
	    (KeyCode)X11_WASM_KEYCODE_SEMICOLON, Mod3MapIndex);
    oldModifierMapSerial = browserDisplay.modifierMapSerial;
    BrowserClearEventQueue();
    if ((modifierMap == NULL) || (modifierMap->max_keypermod != 2)
	    || (modifierMap->modifiermap[Mod3MapIndex
		    * modifierMap->max_keypermod] != X11_WASM_KEYCODE_EQUAL)
	    || (modifierMap->modifiermap[Mod3MapIndex
		    * modifierMap->max_keypermod + 1] != 0)
	    || (XSetModifierMapping(&browserDisplay.display, modifierMap)
		    != MappingSuccess)
	    || (browserDisplay.modifierMapSerial <= oldModifierMapSerial)) {
	goto cleanup;
    }
    memset(&mappingNotifyEvent, 0, sizeof(mappingNotifyEvent));
    if ((XNextEvent(&browserDisplay.display, &mappingNotifyEvent) != Success)
	    || (mappingNotifyEvent.type != MappingNotify)
	    || (mappingNotifyEvent.xmapping.request != MappingModifier)
	    || (mappingNotifyEvent.xmapping.first_keycode != 0)
	    || (mappingNotifyEvent.xmapping.count != 0)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    XFreeModifiermap(modifierMap);
    modifierMap = XGetModifierMapping(&browserDisplay.display);
    if ((modifierMap == NULL) || (modifierMap->max_keypermod != 2)
	    || (modifierMap->modifiermap[ShiftMapIndex
		    * modifierMap->max_keypermod] != 0)
	    || (modifierMap->modifiermap[Mod3MapIndex
		    * modifierMap->max_keypermod] != X11_WASM_KEYCODE_EQUAL)
	    || (modifierMap->modifiermap[Mod3MapIndex
		    * modifierMap->max_keypermod + 1] != 0)) {
	goto cleanup;
    }
    if (!BrowserQueueKeyEventFields(KeyPress, browserDisplay.rootWindow,
	    (KeyCode)X11_WASM_KEYCODE_EQUAL, XK_equal, 0, 0, 0,
	    CurrentTime, 0)) {
	goto cleanup;
    }
    modifierBusyKeyPressed = 1;
    BrowserClearEventQueue();
    oldModifierMapSerial = browserDisplay.modifierMapSerial;
    modifierMap->modifiermap[Mod3MapIndex * modifierMap->max_keypermod] =
	    X11_WASM_KEYCODE_SEMICOLON;
    if ((XSetModifierMapping(&browserDisplay.display, modifierMap)
		    != MappingBusy)
	    || (browserDisplay.modifierMapSerial != oldModifierMapSerial)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    if (!BrowserQueueKeyEventFields(KeyRelease, browserDisplay.rootWindow,
	    (KeyCode)X11_WASM_KEYCODE_EQUAL, XK_equal, 0, 0, 0,
	    CurrentTime, 0)) {
	goto cleanup;
    }
    modifierBusyKeyPressed = 0;
    BrowserClearEventQueue();
    XFreeModifiermap(modifierMap);
    modifierMap = XGetModifierMapping(&browserDisplay.display);
    if ((modifierMap == NULL) || (modifierMap->max_keypermod != 2)
	    || (modifierMap->modifiermap[Mod3MapIndex
		    * modifierMap->max_keypermod] != X11_WASM_KEYCODE_EQUAL)
	    || (modifierMap->modifiermap[Mod3MapIndex
		    * modifierMap->max_keypermod + 1] != 0)) {
	goto cleanup;
    }
    modifierMap->modifiermap[ShiftMapIndex * modifierMap->max_keypermod] = 0;
    XFreeModifiermap(modifierMap);
    modifierMap = NULL;
    oldModifierMapSerial = browserDisplay.modifierMapSerial;
    mappingEvent.request = MappingModifier;
    mappingEvent.first_keycode = 0;
    mappingEvent.count = 0;
    if ((XRefreshKeyboardMapping(&mappingEvent) != Success)
	    || (browserDisplay.modifierMapSerial <= oldModifierMapSerial)) {
	goto cleanup;
    }
    modifierMap = XGetModifierMapping(&browserDisplay.display);
    if ((modifierMap == NULL) || (modifierMap->max_keypermod < 2)
	    || (XKeycodeToKeysym(&browserDisplay.display,
		    modifierMap->modifiermap[ShiftMapIndex
		    * modifierMap->max_keypermod], 0) != XK_Shift_L)) {
	goto cleanup;
    }
    XFreeModifiermap(modifierMap);
    modifierMap = NULL;
    savedLocaleModifiers = (localeModifiers != NULL)
	    ? BrowserDupString(localeModifiers) : NULL;
    if ((strcmp(XSetLocaleModifiers("@im=x11wasm"), "@im=x11wasm") != 0)
	    || (strcmp(XSetLocaleModifiers(NULL), "@im=x11wasm") != 0)
	    || (strcmp(XSetLocaleModifiers(""), "") != 0)) {
	goto cleanup;
    }
    browserTestIMInstantiateCount = 0;
    browserTestIMInstantiateDisplay = NULL;
    browserTestIMInstantiateClientData = NULL;
    browserTestIMInstantiateCallData = NULL;
    if (!XRegisterIMInstantiateCallback(&browserDisplay.display, NULL, NULL,
	    NULL, BrowserTestIMInstantiateCallback,
	    (XPointer)&imAvailableToken)
	    || (browserTestIMInstantiateCount != 0)
	    || !XRegisterIMInstantiateCallback(&browserDisplay.display, NULL,
		    NULL, NULL, BrowserTestIMInstantiateCallback,
		    (XPointer)&imAvailableToken)
	    || !XUnregisterIMInstantiateCallback(&browserDisplay.display,
		    NULL, NULL, NULL, BrowserTestIMInstantiateCallback,
		    (XPointer)&imAvailableToken)
	    || XUnregisterIMInstantiateCallback(&browserDisplay.display,
		    NULL, NULL, NULL, BrowserTestIMInstantiateCallback,
		    (XPointer)&imAvailableToken)) {
	goto cleanup;
    }
    oldCloseSerial = browserDisplay.closeSerial;
    (void)XSynchronize(&browserDisplay.display, True);
    (void)XGrabServer(&browserDisplay.display);
    serialBeforeWait = LastKnownRequestProcessed(&browserDisplay.display);
    if (!XRegisterIMInstantiateCallback(&browserDisplay.display, NULL, NULL,
	    NULL, BrowserTestIMInstantiateCallback, (XPointer)&imCloseToken)
	    || (XCloseDisplay(&browserDisplay.display) != Success)
	    || (LastKnownRequestProcessed(&browserDisplay.display)
		    <= serialBeforeWait)
	    || (browserDisplay.closeSerial <= oldCloseSerial)
	    || (browserDisplay.serverGrabDepth != 0)
	    || (browserDisplay.synchronize != 0)
	    || (browserDisplay.afterFunction != NULL)
	    || XUnregisterIMInstantiateCallback(&browserDisplay.display,
		    NULL, NULL, NULL, BrowserTestIMInstantiateCallback,
		    (XPointer)&imCloseToken)) {
	goto cleanup;
    }
    (void)XSetLocaleModifiers("@im=none");
    if (XOpenIM(&browserDisplay.display, NULL, NULL, NULL) != NULL) {
	goto cleanup;
    }
    browserTestIMInstantiateCount = 0;
    browserTestIMInstantiateDisplay = NULL;
    browserTestIMInstantiateClientData = NULL;
    browserTestIMInstantiateCallData = NULL;
    if (!XRegisterIMInstantiateCallback(&browserDisplay.display, NULL, NULL,
	    NULL, BrowserTestIMInstantiateCallback,
	    (XPointer)&imDelayedToken)
	    || (browserTestIMInstantiateCount != 0)) {
	goto cleanup;
    }
    (void)XSetLocaleModifiers("@im=x11wasm");
    if ((browserTestIMInstantiateCount != 1)
	    || (browserTestIMInstantiateDisplay != &browserDisplay.display)
	    || (browserTestIMInstantiateClientData
		    != (XPointer)&imDelayedToken)
	    || (browserTestIMInstantiateCallData != NULL)
	    || XUnregisterIMInstantiateCallback(&browserDisplay.display,
		    NULL, NULL, NULL, BrowserTestIMInstantiateCallback,
		    (XPointer)&imDelayedToken)) {
	goto cleanup;
    }
    inputMethod = XOpenIM(&browserDisplay.display, NULL, NULL, NULL);
    if (inputMethod == NULL) {
	goto cleanup;
    }
    destroyCallback.client_data = (XPointer)&imDestroyToken;
    destroyCallback.callback = BrowserTestIMDestroyCallback;
    browserTestIMDestroyCount = 0;
    browserTestIMDestroyMethod = NULL;
    browserTestIMDestroyClientData = NULL;
    browserTestIMDestroyCallData = NULL;
    if ((XSetIMValues(inputMethod, XNDestroyCallback, &destroyCallback,
	    NULL) != NULL)
	    || (XGetIMValues(inputMethod, XNDestroyCallback,
		    &destroyCallbackReturn, NULL) != NULL)
	    || (destroyCallbackReturn.client_data != destroyCallback.client_data)
	    || (destroyCallbackReturn.callback != destroyCallback.callback)) {
	goto cleanup;
    }
    inputContext = XCreateIC(inputMethod,
	    XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	    XNClientWindow, browserDisplay.rootWindow,
	    XNFocusWindow, browserDisplay.rootWindow,
	    NULL);
    if ((inputContext == NULL)
	    || (XGetICValues(inputContext, XNFilterEvents, &filterEventsReturn,
		    XNClientWindow, &icClientWindowReturn,
		    XNFocusWindow, &icFocusWindowReturn, NULL) != NULL)
	    || ((filterEventsReturn & KeyPressMask) == 0)
	    || (icClientWindowReturn != browserDisplay.rootWindow)
	    || (icFocusWindowReturn != browserDisplay.rootWindow)) {
	goto cleanup;
    }
    filterEvent.xkey.type = KeyPress;
    filterEvent.xkey.display = &browserDisplay.display;
    filterEvent.xkey.window = browserDisplay.rootWindow;
    filterEvent.xkey.state = X11_WASM_IME_CONTROL_STATE;
    filterEvent.xkey.keycode = BrowserKeycodeFromKeySym(XK_a);
    if (XFilterEvent(&filterEvent, browserDisplay.rootWindow) != False) {
	goto cleanup;
    }
    XSetICFocus(inputContext);
    if ((XFilterEvent(&filterEvent, browserDisplay.rootWindow) != True)
	    || (XFilterEvent(&filterEvent, None) != True)
	    || (XFilterEvent(&filterEvent, (Window)0x7fffffebUL) != False)) {
	goto cleanup;
    }
    XUnsetICFocus(inputContext);
    if (XFilterEvent(&filterEvent, browserDisplay.rootWindow) != False) {
	goto cleanup;
    }
    XSetICFocus(inputContext);
    XDestroyIC(inputContext);
    inputContext = NULL;
    if ((focusedInputContext != NULL)
	    || (XFilterEvent(&filterEvent, browserDisplay.rootWindow)
		    != False)) {
	goto cleanup;
    }
    (void)XCloseIM(inputMethod);
    inputMethod = NULL;
    if ((browserTestIMDestroyCount != 1)
	    || (browserTestIMDestroyMethod == NULL)
	    || (browserTestIMDestroyClientData != (XPointer)&imDestroyToken)
	    || (browserTestIMDestroyCallData != NULL)) {
	goto cleanup;
    }
    browserTestXIOErrorCount = 0;
    if (XSetIOErrorHandler(BrowserTestXIOErrorHandler) == NULL) {
	goto cleanup;
    }
    if ((ioErrorHandler == NULL)
	    || (ioErrorHandler(&browserDisplay.display) != 0)
	    || (browserTestXIOErrorCount != 1)) {
	goto cleanup;
    }
    if (XSetIOErrorHandler(NULL) != BrowserTestXIOErrorHandler) {
	goto cleanup;
    }
    if ((ioErrorHandler == NULL)
	    || (ioErrorHandler == BrowserTestXIOErrorHandler)
	    || (ioErrorHandler(&browserDisplay.display) != 0)
	    || (browserTestXIOErrorCount != 1)) {
	goto cleanup;
    }

    screen = DefaultScreen(&browserDisplay.display);
    allocatedIconSize = XAllocIconSize();
    allocatedStandardColormap = XAllocStandardColormap();
    if ((allocatedIconSize == NULL)
	    || (allocatedIconSize->min_width != 0)
	    || (allocatedIconSize->min_height != 0)
	    || (allocatedIconSize->max_width != 0)
	    || (allocatedIconSize->max_height != 0)
	    || (allocatedIconSize->width_inc != 0)
	    || (allocatedIconSize->height_inc != 0)
	    || (allocatedStandardColormap == NULL)
	    || (allocatedStandardColormap->colormap != None)
	    || (allocatedStandardColormap->red_max != 0)
	    || (allocatedStandardColormap->red_mult != 0)
	    || (allocatedStandardColormap->green_max != 0)
	    || (allocatedStandardColormap->green_mult != 0)
	    || (allocatedStandardColormap->blue_max != 0)
	    || (allocatedStandardColormap->blue_mult != 0)
	    || (allocatedStandardColormap->base_pixel != 0)
	    || (allocatedStandardColormap->visualid != 0)
	    || (allocatedStandardColormap->killid != None)) {
	goto cleanup;
    }
    XFree(allocatedIconSize);
    allocatedIconSize = NULL;
    XFree(allocatedStandardColormap);
    allocatedStandardColormap = NULL;
    pixmapFormats = XListPixmapFormats(&browserDisplay.display,
	    &pixmapFormatCount);
    if ((pixmapFormats == NULL) || (pixmapFormatCount != 2)
	    || (pixmapFormats[0].depth != 1)
	    || (pixmapFormats[0].bits_per_pixel != 1)
	    || (pixmapFormats[0].scanline_pad != 32)
	    || (pixmapFormats[1].depth
		    != DefaultDepth(&browserDisplay.display, screen))
	    || (pixmapFormats[1].bits_per_pixel != 32)
	    || (pixmapFormats[1].scanline_pad != 32)) {
	goto cleanup;
    }
    XFree(pixmapFormats);
    pixmapFormats = NULL;
    depthList = XListDepths(&browserDisplay.display, screen, &depthCount);
    if ((depthList == NULL) || (depthCount != 1)
	    || (depthList[0] != DefaultDepth(&browserDisplay.display, screen))) {
	goto cleanup;
    }
    XFree(depthList);
    depthList = NULL;
    depthCount = -1;
    depthList = XListDepths(&browserDisplay.display, screen + 1, &depthCount);
    if ((depthList != NULL) || (depthCount != 0)) {
	goto cleanup;
    }
    visualInfoList = XGetVisualInfo(&browserDisplay.display, VisualNoMask,
	    NULL, &visualInfoCount);
    if ((visualInfoList == NULL) || (visualInfoCount != 1)
	    || (visualInfoList[0].visual
		    != DefaultVisual(&browserDisplay.display, screen))
	    || (visualInfoList[0].visualid
		    != XVisualIDFromVisual(DefaultVisual(&browserDisplay.display,
			    screen)))
	    || (visualInfoList[0].screen != screen)
	    || (visualInfoList[0].depth
		    != DefaultDepth(&browserDisplay.display, screen))) {
	goto cleanup;
    }
    XFree(visualInfoList);
    visualInfoList = NULL;
    visualInfoCount = -1;
    visualTemplate.visualid = XVisualIDFromVisual(
	    DefaultVisual(&browserDisplay.display, screen));
    visualInfoList = XGetVisualInfo(&browserDisplay.display, VisualIDMask,
	    &visualTemplate, &visualInfoCount);
    if ((visualInfoList == NULL) || (visualInfoCount != 1)
	    || (visualInfoList[0].visualid != visualTemplate.visualid)) {
	goto cleanup;
    }
    XFree(visualInfoList);
    visualInfoList = NULL;
    visualInfoCount = -1;
    visualTemplate.visualid++;
    visualInfoList = XGetVisualInfo(&browserDisplay.display, VisualIDMask,
	    &visualTemplate, &visualInfoCount);
    if ((visualInfoList != NULL) || (visualInfoCount != 0)) {
	goto cleanup;
    }
    visualInfoCount = -1;
    visualInfoList = XGetVisualInfo(&browserDisplay.display, VisualScreenMask,
	    NULL, &visualInfoCount);
    if ((visualInfoList != NULL) || (visualInfoCount != 0)) {
	goto cleanup;
    }
    if (!XMatchVisualInfo(&browserDisplay.display, screen,
	    DefaultDepth(&browserDisplay.display, screen),
	    DefaultVisual(&browserDisplay.display, screen)->c_class,
	    &matchedVisualInfo)
	    || (matchedVisualInfo.visual
		    != DefaultVisual(&browserDisplay.display, screen))
	    || (matchedVisualInfo.screen != screen)
	    || (matchedVisualInfo.depth
		    != DefaultDepth(&browserDisplay.display, screen))
	    || (matchedVisualInfo.c_class
		    != DefaultVisual(&browserDisplay.display, screen)->c_class)
	    || XMatchVisualInfo(&browserDisplay.display, screen,
		    DefaultDepth(&browserDisplay.display, screen) + 1,
		    DefaultVisual(&browserDisplay.display, screen)->c_class,
		    &matchedVisualInfo)) {
	goto cleanup;
    }
    result = 30;
    firstColormap = XCreateColormap(&browserDisplay.display,
	    browserDisplay.rootWindow,
	    DefaultVisual(&browserDisplay.display, screen), AllocNone);
    secondColormap = XCreateColormap(&browserDisplay.display,
	    browserDisplay.rootWindow,
	    DefaultVisual(&browserDisplay.display, screen), AllocNone);
    if ((firstColormap == None) || (secondColormap == None)) {
	goto cleanup;
    }
    simpleWindow = XCreateSimpleWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 2, 3, 17, 11, 2, 0x112233UL,
	    0x445566UL);
    simpleResourcePtr = X11WasmFindResource((XID)simpleWindow);
    if ((simpleWindow == None) || (simpleResourcePtr == NULL)
	    || (simpleResourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (simpleResourcePtr->c_class != InputOutput)
	    || !simpleResourcePtr->hasBackgroundPixel
	    || (simpleResourcePtr->atts.background_pixel != 0x445566UL)
	    || !simpleResourcePtr->hasBorderPixel
	    || (simpleResourcePtr->atts.border_pixel != 0x112233UL)
	    || !XGetWindowAttributes(&browserDisplay.display, simpleWindow,
		    &simpleAttributesReturn)
	    || (simpleAttributesReturn.x != 2)
	    || (simpleAttributesReturn.y != 3)
	    || (simpleAttributesReturn.width != 17)
	    || (simpleAttributesReturn.height != 11)
	    || (simpleAttributesReturn.border_width != 2)
	    || (simpleAttributesReturn.depth
		    != DefaultDepth(&browserDisplay.display, screen))
	    || (simpleAttributesReturn.c_class != InputOutput)) {
	goto cleanup;
    }
    attributes.colormap = firstColormap;
    window = XCreateWindow(&browserDisplay.display, browserDisplay.rootWindow,
	    0, 0, 1, 1, 0, CopyFromParent, InputOutput,
	    DefaultVisual(&browserDisplay.display, screen), CWColormap,
	    &attributes);
    if (window == None) {
	goto cleanup;
    }
    pixmap = XCreatePixmap(&browserDisplay.display, (Drawable)window, 1, 1,
	    (unsigned int)DefaultDepth(&browserDisplay.display, screen));
    copyExposurePixmap = XCreatePixmap(&browserDisplay.display,
	    (Drawable)window, 4, 4,
	    (unsigned int)DefaultDepth(&browserDisplay.display, screen));
    pixmapPtr = X11WasmFindResource((XID)pixmap);
    if ((copyExposurePixmap == None) || (pixmapPtr == NULL)
	    || (pixmapPtr->type != X11_WASM_RESOURCE_PIXMAP)
	    || (pixmapPtr->width != 1)
	    || (pixmapPtr->height != 1)
	    || (pixmapPtr->depth
		    != (unsigned int)DefaultDepth(&browserDisplay.display,
			    screen))
	    || (pixmapPtr->atts.colormap != firstColormap)) {
	goto cleanup;
    }
    X11WasmSetPixmapColormap(pixmap, secondColormap);
    if (pixmapPtr->atts.colormap != secondColormap) {
	goto cleanup;
    }
    widePixmap = XCreatePixmap(&browserDisplay.display, (Drawable)window,
	    40000, 2, (unsigned int)DefaultDepth(&browserDisplay.display,
		    screen));
    pixmapPtr = X11WasmFindResource((XID)widePixmap);
    if ((widePixmap == None) || (pixmapPtr == NULL)
	    || (pixmapPtr->type != X11_WASM_RESOURCE_PIXMAP)
	    || (pixmapPtr->width != 40000)
	    || (pixmapPtr->height != 2)
	    || (XFillRectangle(&browserDisplay.display, (Drawable)widePixmap,
		    xyBitmapGC, 35000, 1, 1, 1) != Success)
	    || !pixmapPtr->damaged
	    || (pixmapPtr->damage.x != SHRT_MAX)
	    || (pixmapPtr->damage.y != 1)
	    || (pixmapPtr->damage.width != 1)
	    || (pixmapPtr->damage.height != 1)) {
	goto cleanup;
    }
    X11WasmSetPixmapColormap((Pixmap)window, firstColormap);
    X11WasmSetPixmapColormap(pixmap, (Colormap)0x7fffffecUL);
    pixmapPtr = X11WasmFindResource((XID)pixmap);
    if ((pixmapPtr == NULL) || (pixmapPtr->atts.colormap != secondColormap)) {
	goto cleanup;
    }

    BrowserClearEventQueue();
    (void)XSelectInput(&browserDisplay.display, window, ColormapChangeMask);
    if (XSetWindowColormap(&browserDisplay.display, window, secondColormap)
	    != Success) {
	goto cleanup;
    }
    if (!BrowserPopMatchingEvent(window, ColormapChangeMask, &colormapEvent)
	    || (colormapEvent.type != ColormapNotify)
	    || (colormapEvent.xcolormap.window != window)
	    || (colormapEvent.xcolormap.colormap != secondColormap)
	    || (colormapEvent.xcolormap.new != True)
	    || (colormapEvent.xcolormap.state != ColormapInstalled)) {
	goto cleanup;
    }
    memset(&colormapEvent, 0, sizeof(colormapEvent));
    attributes.colormap = firstColormap;
    if (XChangeWindowAttributes(&browserDisplay.display, window, CWColormap,
	    &attributes) != Success) {
	goto cleanup;
    }
    if (!BrowserPopMatchingEvent(window, ColormapChangeMask, &colormapEvent)
	    || (colormapEvent.type != ColormapNotify)
	    || (colormapEvent.xcolormap.window != window)
	    || (colormapEvent.xcolormap.colormap != firstColormap)
	    || (colormapEvent.xcolormap.new != True)
	    || (colormapEvent.xcolormap.state != ColormapInstalled)) {
	goto cleanup;
    }
    memset(&colormapEvent, 0, sizeof(colormapEvent));
    if ((XSetWindowColormap(&browserDisplay.display, window, firstColormap)
	    != Success)
	    || BrowserPopMatchingEvent(window, ColormapChangeMask,
		    &colormapEvent)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    (void)XSelectInput(&browserDisplay.display, window,
	    VisibilityChangeMask);
    result = 40;
    if (XMapWindow(&browserDisplay.display, window) != Success) {
	goto cleanup;
    }
    if (!BrowserPopMatchingEvent(window, VisibilityChangeMask,
	    &visibilityEvent)
	    || (visibilityEvent.type != VisibilityNotify)
	    || (visibilityEvent.xvisibility.window != window)
	    || (visibilityEvent.xvisibility.state != VisibilityUnobscured)
	    || BrowserPopMatchingEvent(window, VisibilityChangeMask,
		    &visibilityEvent)) {
	goto cleanup;
    }
    if (XUnmapWindow(&browserDisplay.display, window) != Success) {
	goto cleanup;
    }

    BrowserClearEventQueue();
    (void)XSelectInput(&browserDisplay.display, window, StructureNotifyMask);
    if ((XMapWindow(&browserDisplay.display, window) != Success)
	    || (XMapWindow(&browserDisplay.display, window) != Success)) {
	goto cleanup;
    }
    while (BrowserPopMatchingEvent(window, StructureNotifyMask,
	    &structureEvent)) {
	if (structureEvent.type == MapNotify) {
	    mapNotifyCount++;
	}
	memset(&structureEvent, 0, sizeof(structureEvent));
    }
    if (mapNotifyCount != 1) {
	goto cleanup;
    }
    if ((XUnmapWindow(&browserDisplay.display, window) != Success)
	    || (XUnmapWindow(&browserDisplay.display, window) != Success)) {
	goto cleanup;
    }
    while (BrowserPopMatchingEvent(window, StructureNotifyMask,
	    &structureEvent)) {
	if (structureEvent.type == UnmapNotify) {
	    unmapNotifyCount++;
	}
	memset(&structureEvent, 0, sizeof(structureEvent));
    }
    if (unmapNotifyCount != 1) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    mapSubParentWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 8, 8, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen), 0,
	    NULL);
    mapSubBottomWindow = XCreateWindow(&browserDisplay.display,
	    mapSubParentWindow, 0, 0, 2, 2, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen), 0,
	    NULL);
    mapSubMiddleWindow = XCreateWindow(&browserDisplay.display,
	    mapSubParentWindow, 1, 1, 2, 2, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen), 0,
	    NULL);
    mapSubTopWindow = XCreateWindow(&browserDisplay.display,
	    mapSubParentWindow, 2, 2, 2, 2, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen), 0,
	    NULL);
    if ((mapSubParentWindow == None) || (mapSubBottomWindow == None)
	    || (mapSubMiddleWindow == None) || (mapSubTopWindow == None)
	    || (XMapWindow(&browserDisplay.display, mapSubParentWindow)
		    != Success)
	    || (XSelectInput(&browserDisplay.display, mapSubBottomWindow,
		    StructureNotifyMask) != Success)
	    || (XSelectInput(&browserDisplay.display, mapSubMiddleWindow,
		    StructureNotifyMask) != Success)
	    || (XSelectInput(&browserDisplay.display, mapSubTopWindow,
		    StructureNotifyMask) != Success)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    expectedSubwindowOrder[0] = mapSubTopWindow;
    expectedSubwindowOrder[1] = mapSubMiddleWindow;
    expectedSubwindowOrder[2] = mapSubBottomWindow;
    subwindowEventCount = 0;
    if (XMapSubwindows(&browserDisplay.display, mapSubParentWindow)
	    != Success) {
	goto cleanup;
    }
    while (BrowserPopMatchingEvent(None, StructureNotifyMask,
	    &structureEvent)) {
	if (structureEvent.type == MapNotify) {
	    if ((subwindowEventCount >= 3)
		    || (structureEvent.xmap.window
			    != expectedSubwindowOrder[subwindowEventCount])) {
		goto cleanup;
	    }
	    subwindowEventCount++;
	}
	memset(&structureEvent, 0, sizeof(structureEvent));
    }
    if ((subwindowEventCount != 3)
	    || !XGetWindowAttributes(&browserDisplay.display,
		    mapSubBottomWindow, &subwindowAttributesReturn)
	    || (subwindowAttributesReturn.map_state != IsViewable)
	    || !XGetWindowAttributes(&browserDisplay.display,
		    mapSubMiddleWindow, &subwindowAttributesReturn)
	    || (subwindowAttributesReturn.map_state != IsViewable)
	    || !XGetWindowAttributes(&browserDisplay.display,
		    mapSubTopWindow, &subwindowAttributesReturn)
	    || (subwindowAttributesReturn.map_state != IsViewable)) {
	goto cleanup;
    }

    BrowserClearEventQueue();
    expectedSubwindowOrder[0] = mapSubBottomWindow;
    expectedSubwindowOrder[1] = mapSubMiddleWindow;
    expectedSubwindowOrder[2] = mapSubTopWindow;
    subwindowEventCount = 0;
    if (XUnmapSubwindows(&browserDisplay.display, mapSubParentWindow)
	    != Success) {
	goto cleanup;
    }
    while (BrowserPopMatchingEvent(None, StructureNotifyMask,
	    &structureEvent)) {
	if (structureEvent.type == UnmapNotify) {
	    if ((subwindowEventCount >= 3)
		    || (structureEvent.xunmap.window
			    != expectedSubwindowOrder[subwindowEventCount])) {
		goto cleanup;
	    }
	    subwindowEventCount++;
	}
	memset(&structureEvent, 0, sizeof(structureEvent));
    }
    if ((subwindowEventCount != 3)
	    || !XGetWindowAttributes(&browserDisplay.display,
		    mapSubBottomWindow, &subwindowAttributesReturn)
	    || (subwindowAttributesReturn.map_state != IsUnmapped)
	    || !XGetWindowAttributes(&browserDisplay.display,
		    mapSubMiddleWindow, &subwindowAttributesReturn)
	    || (subwindowAttributesReturn.map_state != IsUnmapped)
	    || !XGetWindowAttributes(&browserDisplay.display,
		    mapSubTopWindow, &subwindowAttributesReturn)
	    || (subwindowAttributesReturn.map_state != IsUnmapped)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    reparentOldParentWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 20, 20, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen), 0,
	    NULL);
    reparentNewParentWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 30, 0, 20, 20, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen), 0,
	    NULL);
    reparentChildWindow = XCreateWindow(&browserDisplay.display,
	    reparentOldParentWindow, 1, 2, 5, 6, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen), 0,
	    NULL);
    if ((reparentOldParentWindow == None)
	    || (reparentNewParentWindow == None)
	    || (reparentChildWindow == None)
	    || (XSelectInput(&browserDisplay.display, reparentChildWindow,
		    StructureNotifyMask) != Success)
	    || (XSelectInput(&browserDisplay.display, reparentOldParentWindow,
		    SubstructureNotifyMask) != Success)
	    || (XSelectInput(&browserDisplay.display, reparentNewParentWindow,
		    SubstructureNotifyMask) != Success)
	    || (XMapWindow(&browserDisplay.display, reparentOldParentWindow)
		    != Success)
	    || (XMapWindow(&browserDisplay.display, reparentNewParentWindow)
		    != Success)
	    || (XMapWindow(&browserDisplay.display, reparentChildWindow)
		    != Success)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    if (XReparentWindow(&browserDisplay.display, reparentChildWindow,
	    reparentNewParentWindow, 7, 9) != Success) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XNextEvent(&browserDisplay.display, &structureEvent) != Success)
	    || (structureEvent.type != UnmapNotify)
	    || (structureEvent.xunmap.event != reparentChildWindow)
	    || (structureEvent.xunmap.window != reparentChildWindow)
	    || (structureEvent.xunmap.from_configure != False)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XNextEvent(&browserDisplay.display, &structureEvent) != Success)
	    || (structureEvent.type != UnmapNotify)
	    || (structureEvent.xunmap.event != reparentOldParentWindow)
	    || (structureEvent.xunmap.window != reparentChildWindow)
	    || (structureEvent.xunmap.from_configure != False)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XNextEvent(&browserDisplay.display, &structureEvent) != Success)
	    || (structureEvent.type != ReparentNotify)
	    || (structureEvent.xreparent.event != reparentChildWindow)
	    || (structureEvent.xreparent.window != reparentChildWindow)
	    || (structureEvent.xreparent.parent != reparentNewParentWindow)
	    || (structureEvent.xreparent.x != 7)
	    || (structureEvent.xreparent.y != 9)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XNextEvent(&browserDisplay.display, &structureEvent) != Success)
	    || (structureEvent.type != ReparentNotify)
	    || (structureEvent.xreparent.event != reparentOldParentWindow)
	    || (structureEvent.xreparent.window != reparentChildWindow)
	    || (structureEvent.xreparent.parent != reparentNewParentWindow)
	    || (structureEvent.xreparent.x != 7)
	    || (structureEvent.xreparent.y != 9)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XNextEvent(&browserDisplay.display, &structureEvent) != Success)
	    || (structureEvent.type != ReparentNotify)
	    || (structureEvent.xreparent.event != reparentNewParentWindow)
	    || (structureEvent.xreparent.window != reparentChildWindow)
	    || (structureEvent.xreparent.parent != reparentNewParentWindow)
	    || (structureEvent.xreparent.x != 7)
	    || (structureEvent.xreparent.y != 9)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XNextEvent(&browserDisplay.display, &structureEvent) != Success)
	    || (structureEvent.type != MapNotify)
	    || (structureEvent.xmap.event != reparentChildWindow)
	    || (structureEvent.xmap.window != reparentChildWindow)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XNextEvent(&browserDisplay.display, &structureEvent) != Success)
	    || (structureEvent.type != MapNotify)
	    || (structureEvent.xmap.event != reparentNewParentWindow)
	    || (structureEvent.xmap.window != reparentChildWindow)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    if (!XGetWindowAttributes(&browserDisplay.display, reparentChildWindow,
	    &reparentAttributesReturn)
	    || (reparentAttributesReturn.x != 7)
	    || (reparentAttributesReturn.y != 9)
	    || (reparentAttributesReturn.map_state != IsViewable)) {
	goto cleanup;
    }
    if (!XQueryTree(&browserDisplay.display, reparentOldParentWindow,
	    &reparentRootReturn, &reparentParentReturn,
	    &reparentChildrenReturn, &reparentChildCount)
	    || (reparentChildCount != 0)
	    || (reparentChildrenReturn != NULL)) {
	goto cleanup;
    }
    if (!XQueryTree(&browserDisplay.display, reparentNewParentWindow,
	    &reparentRootReturn, &reparentParentReturn,
	    &reparentChildrenReturn, &reparentChildCount)
	    || (reparentChildCount != 1)
	    || (reparentChildrenReturn == NULL)
	    || (reparentChildrenReturn[0] != reparentChildWindow)) {
	goto cleanup;
    }
    XFree(reparentChildrenReturn);
    reparentChildrenReturn = NULL;
    BrowserClearEventQueue();

    memset(&structureEvent, 0x7f, sizeof(structureEvent));
    serialBeforeWait = LastKnownRequestProcessed(&browserDisplay.display);
    if ((XNextEvent(&browserDisplay.display, &structureEvent) != Success)
	    || (structureEvent.type != 0)
	    || (LastKnownRequestProcessed(&browserDisplay.display)
		<= serialBeforeWait)) {
	goto cleanup;
    }

    BrowserSetCommonEventFields(&structureEvent, ClientMessage, window,
	    CurrentTime);
    structureEvent.xclient.window = window;
    if (!BrowserQueueEventWhenIdle(&structureEvent)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XNextEvent(&browserDisplay.display, &structureEvent) != Success)
	    || (structureEvent.type != ClientMessage)
	    || (structureEvent.xclient.window != window)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    BrowserSetCommonEventFields(&structureEvent, MapNotify, window,
	    CurrentTime);
    structureEvent.xmap.event = window;
    structureEvent.xmap.window = window;
    if (!BrowserQueueEventWhenIdle(&structureEvent)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XWindowEvent(&browserDisplay.display, window, StructureNotifyMask,
		    &structureEvent) != Success)
	    || (structureEvent.type != MapNotify)
	    || (structureEvent.xmap.window != window)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    BrowserSetCommonEventFields(&exposureEvent, NoExpose, (Window)pixmap,
	    CurrentTime);
    exposureEvent.xnoexpose.drawable = (Drawable)pixmap;
    exposureEvent.xnoexpose.major_code = X11_WASM_XREQ_COPY_AREA;
    exposureEvent.xnoexpose.minor_code = 0;
    BrowserSetCommonEventFields(&structureEvent, MapNotify, window,
	    CurrentTime);
    structureEvent.xmap.event = window;
    structureEvent.xmap.window = window;
    if (!BrowserQueueEvent(&exposureEvent)
	    || !BrowserQueueEvent(&structureEvent)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if ((XWindowEvent(&browserDisplay.display, window, StructureNotifyMask,
		    &structureEvent) != Success)
	    || (structureEvent.type != MapNotify)
	    || (structureEvent.xmap.window != window)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 1)) {
	goto cleanup;
    }
    memset(&exposureEvent, 0, sizeof(exposureEvent));
    if ((XNextEvent(&browserDisplay.display, &exposureEvent) != Success)
	    || (exposureEvent.type != NoExpose)
	    || (exposureEvent.xnoexpose.drawable != (Drawable)pixmap)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 0)) {
	goto cleanup;
    }

    BrowserSetCommonEventFields(&exposureEvent, NoExpose, (Window)pixmap,
	    CurrentTime);
    exposureEvent.xnoexpose.drawable = (Drawable)pixmap;
    exposureEvent.xnoexpose.major_code = X11_WASM_XREQ_COPY_PLANE;
    exposureEvent.xnoexpose.minor_code = 0;
    if (!BrowserQueueEvent(&exposureEvent)) {
	goto cleanup;
    }
    memset(&structureEvent, 0x7f, sizeof(structureEvent));
    serialBeforeWait = LastKnownRequestProcessed(&browserDisplay.display);
    if ((XWindowEvent(&browserDisplay.display, window, StructureNotifyMask,
		    &structureEvent) != Success)
	    || (structureEvent.type != 0)
	    || (LastKnownRequestProcessed(&browserDisplay.display)
		<= serialBeforeWait)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 1)) {
	goto cleanup;
    }
    memset(&exposureEvent, 0, sizeof(exposureEvent));
    if ((XNextEvent(&browserDisplay.display, &exposureEvent) != Success)
	    || (exposureEvent.type != NoExpose)
	    || (exposureEvent.xnoexpose.drawable != (Drawable)pixmap)
	    || (exposureEvent.xnoexpose.major_code
		!= X11_WASM_XREQ_COPY_PLANE)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    memset(&attributes, 0, sizeof(attributes));
    gravityParentWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 100, 100, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen), 0,
	    NULL);
    attributes.event_mask = StructureNotifyMask;
    attributes.win_gravity = SouthEastGravity;
    gravitySouthEastWindow = XCreateWindow(&browserDisplay.display,
	    gravityParentWindow, 10, 10, 10, 10, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen),
	    CWEventMask | CWWinGravity, &attributes);
    attributes.win_gravity = CenterGravity;
    gravityCenterWindow = XCreateWindow(&browserDisplay.display,
	    gravityParentWindow, 20, 20, 10, 10, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen),
	    CWEventMask | CWWinGravity, &attributes);
    attributes.win_gravity = UnmapGravity;
    gravityUnmapWindow = XCreateWindow(&browserDisplay.display,
	    gravityParentWindow, 30, 30, 10, 10, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen),
	    CWEventMask | CWWinGravity, &attributes);
    if ((gravityParentWindow == None) || (gravitySouthEastWindow == None)
	    || (gravityCenterWindow == None) || (gravityUnmapWindow == None)
	    || (XMapWindow(&browserDisplay.display, gravityParentWindow)
		!= Success)
	    || (XMapWindow(&browserDisplay.display, gravitySouthEastWindow)
		!= Success)
	    || (XMapWindow(&browserDisplay.display, gravityCenterWindow)
		!= Success)
	    || (XMapWindow(&browserDisplay.display, gravityUnmapWindow)
		!= Success)) {
	goto cleanup;
    }
    if ((XSetWindowBackgroundPixmap(&browserDisplay.display,
		    gravityParentWindow, pixmap) != Success)
	    || (XSetWindowBackgroundPixmap(&browserDisplay.display,
		    gravitySouthEastWindow, (Pixmap)ParentRelative)
		    != Success)) {
	goto cleanup;
    }
    BrowserResolveWindowBackgroundStyle(
	    X11WasmFindResource((XID)gravitySouthEastWindow),
	    &inheritedBackgroundHasPixel, &inheritedBackgroundPixel,
	    &inheritedBackgroundPixmap);
    if (inheritedBackgroundHasPixel || (inheritedBackgroundPixmap != pixmap)) {
	goto cleanup;
    }
    if (XSetWindowBackground(&browserDisplay.display, gravityParentWindow,
	    0x112233UL) != Success) {
	goto cleanup;
    }
    BrowserResolveWindowBackgroundStyle(
	    X11WasmFindResource((XID)gravitySouthEastWindow),
	    &inheritedBackgroundHasPixel, &inheritedBackgroundPixel,
	    &inheritedBackgroundPixmap);
    if (!inheritedBackgroundHasPixel
	    || (inheritedBackgroundPixel != 0x112233UL)
	    || (inheritedBackgroundPixmap != None)) {
	goto cleanup;
    }
    if ((XSetWindowBorderPixmap(&browserDisplay.display,
		    gravityParentWindow, pixmap) != Success)
	    || (XSetWindowBorderPixmap(&browserDisplay.display,
		    gravityCenterWindow, (Pixmap)CopyFromParent) != Success)) {
	goto cleanup;
    }
    BrowserResolveWindowBorderStyle(
	    X11WasmFindResource((XID)gravityCenterWindow),
	    &inheritedBorderHasPixel, &inheritedBorderPixel,
	    &inheritedBorderPixmap);
    if (inheritedBorderHasPixel || (inheritedBorderPixmap != pixmap)) {
	goto cleanup;
    }
    if (XSetWindowBorder(&browserDisplay.display, gravityParentWindow,
	    0x445566UL) != Success) {
	goto cleanup;
    }
    BrowserResolveWindowBorderStyle(
	    X11WasmFindResource((XID)gravityCenterWindow),
	    &inheritedBorderHasPixel, &inheritedBorderPixel,
	    &inheritedBorderPixmap);
    if (!inheritedBorderHasPixel || (inheritedBorderPixel != 0x445566UL)
	    || (inheritedBorderPixmap != None)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    if (XResizeWindow(&browserDisplay.display, gravityParentWindow, 130, 140)
	    != Success) {
	goto cleanup;
    }
    if (!XGetWindowAttributes(&browserDisplay.display, gravitySouthEastWindow,
	    &gravityAttributesReturn)
	    || (gravityAttributesReturn.x != 40)
	    || (gravityAttributesReturn.y != 50)
	    || !BrowserPopMatchingEvent(gravitySouthEastWindow,
		StructureNotifyMask, &structureEvent)
	    || (structureEvent.type != GravityNotify)
	    || (structureEvent.xgravity.x != 40)
	    || (structureEvent.xgravity.y != 50)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    memset(&gravityAttributesReturn, 0, sizeof(gravityAttributesReturn));
    if (!XGetWindowAttributes(&browserDisplay.display, gravityCenterWindow,
	    &gravityAttributesReturn)
	    || (gravityAttributesReturn.x != 35)
	    || (gravityAttributesReturn.y != 40)
	    || !BrowserPopMatchingEvent(gravityCenterWindow,
		StructureNotifyMask, &structureEvent)
	    || (structureEvent.type != GravityNotify)
	    || (structureEvent.xgravity.x != 35)
	    || (structureEvent.xgravity.y != 40)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    memset(&gravityAttributesReturn, 0, sizeof(gravityAttributesReturn));
    if (!XGetWindowAttributes(&browserDisplay.display, gravityUnmapWindow,
	    &gravityAttributesReturn)
	    || (gravityAttributesReturn.map_state != IsUnmapped)
	    || !BrowserPopMatchingEvent(gravityUnmapWindow,
		StructureNotifyMask, &structureEvent)
	    || (structureEvent.type != UnmapNotify)
	    || (structureEvent.xunmap.from_configure != True)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    if (XSetInputFocus(&browserDisplay.display, gravitySouthEastWindow,
	    RevertToParent, CurrentTime) != Success) {
	goto cleanup;
    }
    focusChangeTime = eventTime;
    (void)XGetInputFocus(&browserDisplay.display, &focusInitialWindow,
	    &focusInitialRevert);
    if ((focusChangeTime <= 1)
	    || (XSetInputFocus(&browserDisplay.display, gravityCenterWindow,
		    RevertToNone, focusChangeTime - 1) != Success)) {
	goto cleanup;
    }
    (void)XGetInputFocus(&browserDisplay.display, &focusAfterStaleTime,
	    &focusAfterStaleRevert);
    if (XSetInputFocus(&browserDisplay.display, gravityCenterWindow,
	    RevertToNone, eventTime + 1) != Success) {
	goto cleanup;
    }
    (void)XGetInputFocus(&browserDisplay.display, &focusAfterFutureTime,
	    &focusAfterFutureRevert);
    if (XSetInputFocus(&browserDisplay.display, gravityCenterWindow,
	    RevertToNone, CurrentTime) != Success) {
	goto cleanup;
    }
    (void)XGetInputFocus(&browserDisplay.display, &focusAfterCurrentTime,
	    &focusAfterCurrentRevert);
    if ((focusInitialWindow != gravitySouthEastWindow)
	    || (focusInitialRevert != RevertToParent)
	    || (focusAfterStaleTime != gravitySouthEastWindow)
	    || (focusAfterStaleRevert != RevertToParent)
	    || (focusAfterFutureTime != gravitySouthEastWindow)
	    || (focusAfterFutureRevert != RevertToParent)
	    || (focusAfterCurrentTime != gravityCenterWindow)
	    || (focusAfterCurrentRevert != RevertToNone)
	    || (XSetInputFocus(&browserDisplay.display,
		    browserDisplay.rootWindow, RevertToParent,
		    CurrentTime) != Success)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    memset(&attributes, 0, sizeof(attributes));
    attributes.event_mask = StructureNotifyMask;
    notifyWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 1, 1, 2, 2, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen),
	    CWEventMask, &attributes);
    if (notifyWindow == None) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    timestampSelection = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TIMESTAMP_SELECTION", False);
    if ((timestampSelection == None)
	    || (XSetSelectionOwner(&browserDisplay.display,
		    timestampSelection, window, CurrentTime) != Success)) {
	goto cleanup;
    }
    selectionOwnerTime = eventTime;
    selectionInitialOwner = XGetSelectionOwner(&browserDisplay.display,
	    timestampSelection);
    if ((selectionOwnerTime <= 1)
	    || (XSetSelectionOwner(&browserDisplay.display,
		    timestampSelection, notifyWindow, selectionOwnerTime - 1)
		!= Success)) {
	goto cleanup;
    }
    selectionAfterStaleTime = XGetSelectionOwner(&browserDisplay.display,
	    timestampSelection);
    if (XSetSelectionOwner(&browserDisplay.display, timestampSelection,
	    notifyWindow, eventTime + 1) != Success) {
	goto cleanup;
    }
    selectionAfterFutureTime = XGetSelectionOwner(&browserDisplay.display,
	    timestampSelection);
    if (XSetSelectionOwner(&browserDisplay.display, timestampSelection,
	    notifyWindow, CurrentTime) != Success) {
	goto cleanup;
    }
    selectionAfterCurrentTime = XGetSelectionOwner(&browserDisplay.display,
	    timestampSelection);
    if ((selectionInitialOwner != window)
	    || (selectionAfterStaleTime != window)
	    || (selectionAfterFutureTime != window)
	    || (selectionAfterCurrentTime != notifyWindow)
	    || (XSetSelectionOwner(&browserDisplay.display,
		    timestampSelection, None, CurrentTime) != Success)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    destroyedWindow = notifyWindow;
    if (XDestroyWindow(&browserDisplay.display, notifyWindow) != Success) {
	goto cleanup;
    }
    notifyWindow = None;
    if (!BrowserPopMatchingEvent(destroyedWindow, StructureNotifyMask,
	    &structureEvent)
	    || (structureEvent.type != DestroyNotify)
	    || (structureEvent.xdestroywindow.event != destroyedWindow)
	    || (structureEvent.xdestroywindow.window != destroyedWindow)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if (BrowserPopMatchingEvent(destroyedWindow, StructureNotifyMask,
	    &structureEvent)) {
	goto cleanup;
    }
    destroyedWindow = None;
    BrowserClearEventQueue();

    memset(&attributes, 0, sizeof(attributes));
    attributes.event_mask = StructureNotifyMask;
    destroyParentWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 1, 1, 8, 8, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen),
	    CWEventMask, &attributes);
    destroyChildWindow = XCreateWindow(&browserDisplay.display,
	    destroyParentWindow, 1, 1, 4, 4, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen),
	    CWEventMask, &attributes);
    destroyGrandchildWindow = XCreateWindow(&browserDisplay.display,
	    destroyChildWindow, 1, 1, 2, 2, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display, screen),
	    CWEventMask, &attributes);
    if ((destroyParentWindow == None) || (destroyChildWindow == None)
	    || (destroyGrandchildWindow == None)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    destroyedParentWindow = destroyParentWindow;
    destroyedChildWindow = destroyChildWindow;
    destroyedGrandchildWindow = destroyGrandchildWindow;
    if (XDestroyWindow(&browserDisplay.display, destroyParentWindow)
	    != Success) {
	goto cleanup;
    }
    destroyParentWindow = None;
    destroyChildWindow = None;
    destroyGrandchildWindow = None;
    if (!BrowserPopMatchingEvent(destroyedGrandchildWindow,
	    StructureNotifyMask, &structureEvent)
	    || (structureEvent.type != DestroyNotify)
	    || (structureEvent.xdestroywindow.window
		!= destroyedGrandchildWindow)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if (!BrowserPopMatchingEvent(destroyedChildWindow, StructureNotifyMask,
	    &structureEvent)
	    || (structureEvent.type != DestroyNotify)
	    || (structureEvent.xdestroywindow.window != destroyedChildWindow)) {
	goto cleanup;
    }
    memset(&structureEvent, 0, sizeof(structureEvent));
    if (!BrowserPopMatchingEvent(destroyedParentWindow, StructureNotifyMask,
	    &structureEvent)
	    || (structureEvent.type != DestroyNotify)
	    || (structureEvent.xdestroywindow.window != destroyedParentWindow)
	    || (X11WasmFindResource((XID)destroyedParentWindow) != NULL)
	    || (X11WasmFindResource((XID)destroyedChildWindow) != NULL)
	    || (X11WasmFindResource((XID)destroyedGrandchildWindow)
		!= NULL)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    memset(&attributes, 0, sizeof(attributes));
    attributes.event_mask = ButtonPressMask | KeyPressMask;
    attributes.do_not_propagate_mask = ButtonReleaseMask;
    attributes.override_redirect = True;
    inputOnlyWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 2, 3, 4, 5, 0, CopyFromParent,
	    InputOnly, (Visual *)CopyFromParent,
	    CWEventMask | CWDontPropagate | CWOverrideRedirect, &attributes);
    if ((inputOnlyWindow == None)
	    || !XGetWindowAttributes(&browserDisplay.display, inputOnlyWindow,
		    &inputOnlyAttributesReturn)
	    || (inputOnlyAttributesReturn.c_class != InputOnly)
	    || (inputOnlyAttributesReturn.depth != 0)
	    || (inputOnlyAttributesReturn.border_width != 0)
	    || (inputOnlyAttributesReturn.visual != NULL)
	    || (inputOnlyAttributesReturn.colormap != None)
	    || (inputOnlyAttributesReturn.your_event_mask
		    != (ButtonPressMask | KeyPressMask))
	    || (inputOnlyAttributesReturn.do_not_propagate_mask
		    != ButtonReleaseMask)
	    || (inputOnlyAttributesReturn.override_redirect != True)) {
	goto cleanup;
    }
    inputOnlyGC = XCreateGC(&browserDisplay.display,
	    (Drawable)inputOnlyWindow, 0, NULL);
    if (inputOnlyGC != NULL) {
	goto cleanup;
    }
    invalidInputOnlyWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 1, 1, 1, CopyFromParent,
	    InputOnly, (Visual *)CopyFromParent, 0, NULL);
    if (invalidInputOnlyWindow != None) {
	goto cleanup;
    }
    (void)XDestroyWindow(&browserDisplay.display, inputOnlyWindow);
    inputOnlyWindow = None;

    lookupEvent.type = KeyPress;
    lookupEvent.display = &browserDisplay.display;
    lookupEvent.window = window;
    lookupEvent.keycode = BrowserKeycodeFromKeySym(unicodeSnowman);
    lookupLength = XmbLookupString(NULL, &lookupEvent, lookupBuffer, 2,
	    &lookupSym, &lookupStatus);
    if ((lookupLength != 3) || (lookupStatus != XBufferOverflow)
	    || (lookupSym != unicodeSnowman)) {
	goto cleanup;
    }
    memset(lookupBuffer, 0, sizeof(lookupBuffer));
    lookupSym = NoSymbol;
    lookupStatus = 0;
    lookupLength = Xutf8LookupString(NULL, &lookupEvent, lookupBuffer,
	    (int)sizeof(lookupBuffer), &lookupSym, &lookupStatus);
    if ((lookupLength != 3) || (lookupStatus != XLookupBoth)
	    || (lookupSym != unicodeSnowman)
	    || (memcmp(lookupBuffer, "\xe2\x98\x83", 3) != 0)) {
	goto cleanup;
    }
    memset(lookupBuffer, 0, sizeof(lookupBuffer));
    lookupSym = NoSymbol;
    lookupStatus = 0;
    lookupEvent.keycode = BrowserKeycodeFromKeySym((KeySym)0x00e9U);
    lookupLength = Xutf8LookupString(NULL, &lookupEvent, lookupBuffer, 1,
	    &lookupSym, &lookupStatus);
    if ((lookupLength != 2) || (lookupStatus != XBufferOverflow)
	    || (lookupSym != (KeySym)0x00e9U)) {
	goto cleanup;
    }
    memset(lookupBuffer, 0, sizeof(lookupBuffer));
    lookupSym = NoSymbol;
    lookupStatus = 0;
    lookupLength = Xutf8LookupString(NULL, &lookupEvent, lookupBuffer,
	    (int)sizeof(lookupBuffer), &lookupSym, &lookupStatus);
    if ((lookupLength != 2) || (lookupStatus != XLookupBoth)
	    || (lookupSym != (KeySym)0x00e9U)
	    || (memcmp(lookupBuffer, "\xc3\xa9", 2) != 0)) {
	goto cleanup;
    }

    listValues[0] = (char *)"alpha";
    listValues[1] = (char *)"beta";
    if (!XStringListToTextProperty(listValues, 2, &listProp)
	    || !XTextPropertyToStringList(&listProp, &stringList,
		    &stringCount)
	    || (stringCount != 2) || (strcmp(stringList[0], "alpha") != 0)
	    || (strcmp(stringList[1], "beta") != 0)) {
	goto cleanup;
    }
    XFreeStringList(stringList);
    stringList = NULL;
    stringCount = -1;
    if ((XmbTextPropertyToTextList(&browserDisplay.display, &listProp,
	    &stringList, &stringCount) != Success)
	    || (stringCount != 2) || (strcmp(stringList[0], "alpha") != 0)
	    || (strcmp(stringList[1], "beta") != 0)) {
	goto cleanup;
    }
    XFreeStringList(stringList);
    stringList = NULL;
    stringCount = -1;
    if ((Xutf8TextPropertyToTextList(&browserDisplay.display, &listProp,
	    &stringList, &stringCount) != Success)
	    || (stringCount != 2) || (strcmp(stringList[0], "alpha") != 0)
	    || (strcmp(stringList[1], "beta") != 0)) {
	goto cleanup;
    }
    XFreeStringList(stringList);
    stringList = NULL;

    mbValues[0] = (char *)"compound";
    utf8Values[0] = (char *)"utf8";
    stdValues[0] = (char *)"caf\303\251";
    if ((XmbTextListToTextProperty(&browserDisplay.display, mbValues, 1,
	    XCompoundTextStyle, &mbProp) != Success)
	    || (mbProp.encoding != BrowserCompoundTextAtom())
	    || (XmbTextPropertyToTextList(&browserDisplay.display, &mbProp,
		    &stringList, &stringCount) != Success)
	    || (stringCount != 1) || (strcmp(stringList[0], "compound") != 0)) {
	goto cleanup;
    }
    XFreeStringList(stringList);
    stringList = NULL;
    stringCount = -1;
    if ((Xutf8TextListToTextProperty(&browserDisplay.display, stdValues, 1,
	    XStdICCTextStyle, &stdProp) != Success)
	    || (stdProp.encoding != XA_STRING)
	    || (stdProp.nitems != 4)
	    || (memcmp(stdProp.value, "caf\351", 4) != 0)
	    || (Xutf8TextPropertyToTextList(&browserDisplay.display, &stdProp,
		    &stringList, &stringCount) != Success)
	    || (stringCount != 1)
	    || (strcmp(stringList[0], "caf\303\251") != 0)) {
	goto cleanup;
    }
    XFreeStringList(stringList);
    stringList = NULL;
    stringCount = -1;
    XFree(stdProp.value);
    memset(&stdProp, 0, sizeof(stdProp));
    stdValues[0] = (char *)"\346\227\245\346\234\254";
    if ((Xutf8TextListToTextProperty(&browserDisplay.display, stdValues, 1,
	    XStdICCTextStyle, &stdProp) != Success)
	    || (stdProp.encoding != BrowserCompoundTextAtom())
	    || (Xutf8TextPropertyToTextList(&browserDisplay.display, &stdProp,
		    &stringList, &stringCount) != Success)
	    || (stringCount != 1)
	    || (strcmp(stringList[0], "\346\227\245\346\234\254") != 0)) {
	goto cleanup;
    }
    XFreeStringList(stringList);
    stringList = NULL;
    stringCount = -1;
    if ((Xutf8TextListToTextProperty(&browserDisplay.display, utf8Values, 1,
	    XUTF8StringStyle, &utf8Prop) != Success)
	    || (utf8Prop.encoding != BrowserUtf8StringAtom())
	    || (Xutf8TextPropertyToTextList(&browserDisplay.display, &utf8Prop,
		    &stringList, &stringCount) != Success)
	    || (stringCount != 1) || (strcmp(stringList[0], "utf8") != 0)) {
	goto cleanup;
    }
    XFreeStringList(stringList);
    stringList = NULL;

    wideValues[0] = wideValue;
    if ((XwcTextListToTextProperty(&browserDisplay.display, wideValues, 1,
	    XUTF8StringStyle, &wideProp) != Success)
	    || (wideProp.encoding != BrowserUtf8StringAtom())
	    || (XwcTextPropertyToTextList(&browserDisplay.display, &wideProp,
		    &wideList, &wideCount) != Success)
	    || (wideCount != 1) || (wcscmp(wideList[0], wideValue) != 0)) {
	goto cleanup;
    }
    XwcFreeStringList(wideList);
    wideList = NULL;
    wideCount = -1;

    listedFontNames = XListFonts(&browserDisplay.display,
	    "-*-Courier-*-*-*-*-*-*-*-*-*-*-*-*", 10, &listedFontCount);
    if ((listedFontNames == NULL) || (listedFontCount < 1)
	    || (strstr(listedFontNames[0], "Courier") == NULL)) {
	goto cleanup;
    }
    XFreeFontNames(listedFontNames);
    listedFontNames = NULL;
    listedFontCount = -1;
    listedFontNames = XListFonts(&browserDisplay.display,
	    "-*-NoSuchFamily-*-*-*-*-*-*-*-*-*-*-*-*", 10,
	    &listedFontCount);
    if ((listedFontNames != NULL) || (listedFontCount != 0)) {
	goto cleanup;
    }
    listedFontCount = -1;
    listedFontNames = XListFonts(&browserDisplay.display, "*", 1,
	    &listedFontCount);
    if ((listedFontNames == NULL) || (listedFontCount != 1)
	    || (listedFontNames[0] == NULL) || (listedFontNames[1] != NULL)) {
	goto cleanup;
    }
    XFreeFontNames(listedFontNames);
    listedFontNames = NULL;
    listedFontCount = -1;
    listedFontNames = XListFonts(&browserDisplay.display,
	    "CustomBrowserFamily", 10, &listedFontCount);
    if ((listedFontNames == NULL) || (listedFontCount != 1)
	    || (strstr(listedFontNames[0], "CustomBrowserFamily") == NULL)) {
	goto cleanup;
    }
    XFreeFontNames(listedFontNames);
    listedFontNames = NULL;

    fontSet = XCreateFontSet(&browserDisplay.display, "fixed, Courier",
	    &missingCharsets, &missingCharsetCount, &defaultString);
    fontSetExtents = XExtentsOfFontSet(fontSet);
    fontSetFontCount = XFontsOfFontSet(fontSet, &fontStructList,
	    &fontNameList);
    if ((fontSet == NULL) || (missingCharsets != NULL)
	    || (missingCharsetCount != 0) || (defaultString == NULL)
	    || (strcmp(defaultString, "") != 0)
	    || (fontSetExtents == NULL)
	    || (fontSetExtents->max_logical_extent.width == 0)
	    || (XBaseFontNameListOfFontSet(fontSet) == NULL)
	    || (strcmp(XBaseFontNameListOfFontSet(fontSet), "fixed, Courier")
		    != 0)
	    || (XLocaleOfFontSet(fontSet) == NULL)
	    || (XLocaleOfFontSet(fontSet)[0] == '\0')
	    || XContextDependentDrawing(fontSet)
	    || XDirectionalDependentDrawing(fontSet)
	    || XContextualDrawing(fontSet)
	    || (fontSetFontCount != 2)
	    || (fontStructList == NULL) || (fontStructList[0] == NULL)
	    || (fontStructList[1] == NULL)
	    || (fontNameList == NULL) || (fontNameList[0] == NULL)
	    || (fontNameList[1] == NULL)
	    || (fontNameList[0][0] == '\0')
	    || (strstr(fontNameList[1], "Courier") == NULL)
	    || (fontStructList[0]->max_bounds.width
		    != (short)fontSetExtents->max_logical_extent.width)
	    || (XTextWidth(fontStructList[0], "abc", 3) <= 0)
	    || (XTextWidth(fontStructList[1], "abc", 3) <= 0)) {
	goto cleanup;
    }
    fontUtf8Width = Xutf8TextEscapement(fontSet, "abc", 3);
    fontWideWidth = XwcTextEscapement(fontSet, fontWideText, 2);
    if ((fontUtf8Width <= 0) || (fontWideWidth <= 0)
	    || (XmbTextEscapement(fontSet, "abc", 3) != fontUtf8Width)
	    || (Xutf8TextExtents(fontSet, "abc", 3, &fontInkExtent,
		    &fontLogicalExtent) != fontUtf8Width)
	    || (fontLogicalExtent.width != (unsigned short)fontUtf8Width)
	    || (fontLogicalExtent.height == 0)
	    || (fontInkExtent.width != fontLogicalExtent.width)
	    || (XmbTextExtents(fontSet, "abc", 3, NULL,
		    &fontOverallLogicalExtent) != fontUtf8Width)
	    || (fontOverallLogicalExtent.width
		    != (unsigned short)fontUtf8Width)
	    || (XwcTextExtents(fontSet, fontWideText, 2, NULL,
		    &fontOverallLogicalExtent) != fontWideWidth)
	    || (fontOverallLogicalExtent.width
		    != (unsigned short)fontWideWidth)
	    || (Xutf8TextPerCharExtents(fontSet, "abc", 3, fontPerCharInk,
		    fontPerCharLogical, 2, &fontCharCount,
		    &fontOverallInkExtent, &fontOverallLogicalExtent) != False)
	    || (fontCharCount != 3)
	    || (fontOverallLogicalExtent.width
		    != (unsigned short)fontUtf8Width)
	    || (Xutf8TextPerCharExtents(fontSet, "abc", 3, fontPerCharInk,
		    fontPerCharLogical, 4, &fontCharCount,
		    &fontOverallInkExtent, &fontOverallLogicalExtent) != True)
	    || (fontCharCount != 3)
	    || (fontPerCharLogical[0].width == 0)
	    || (fontPerCharLogical[1].x
		    != (short)fontPerCharLogical[0].width)
	    || (fontPerCharInk[2].height != fontPerCharLogical[2].height)
	    || (XwcTextPerCharExtents(fontSet, fontWideText, 2,
		    fontPerCharInk, fontPerCharLogical, 4, &fontCharCount,
		    NULL, &fontOverallLogicalExtent) != True)
	    || (fontCharCount != 2)
	    || (fontOverallLogicalExtent.width
		    != (unsigned short)fontWideWidth)) {
	goto cleanup;
    }
    memset(longText, 'W', sizeof(longText));
    fontLongPerCharLogical = (XRectangle *)ckalloc(sizeof(XRectangle)
	    * sizeof(longText));
    memset(fontLongPerCharLogical, 0, sizeof(XRectangle) * sizeof(longText));
    fontLongWidth = Xutf8TextEscapement(fontSet, longText,
	    (int)sizeof(longText));
    fontLongClampedWidth = (fontLongWidth > (int)USHRT_MAX)
	    ? USHRT_MAX : (unsigned short)fontLongWidth;
    if ((fontLongWidth <= SHRT_MAX)
	    || (Xutf8TextExtents(fontSet, longText, (int)sizeof(longText),
		    &fontInkExtent, &fontLogicalExtent) != fontLongWidth)
	    || (fontLogicalExtent.width != fontLongClampedWidth)
	    || (Xutf8TextPerCharExtents(fontSet, longText,
		    (int)sizeof(longText), NULL, fontLongPerCharLogical,
		    (int)sizeof(longText), &fontCharCount,
		    &fontOverallInkExtent, &fontOverallLogicalExtent) != True)
	    || (fontCharCount != (int)sizeof(longText))
	    || (fontOverallLogicalExtent.width != fontLongClampedWidth)
	    || (fontLongPerCharLogical[sizeof(longText) - 1U].x
		    != SHRT_MAX)) {
	goto cleanup;
    }
    textPixmap = XCreatePixmap(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 96, 32,
	    (unsigned int)DefaultDepth(&browserDisplay.display, screen));
    textGC = (textPixmap != None)
	    ? XCreateGC(&browserDisplay.display, textPixmap, 0, NULL)
	    : NULL;
    if ((textPixmap == None) || (textGC == NULL)
	    || (XSetForeground(&browserDisplay.display, textGC, 0x00ff0000UL)
		    != Success)
	    || (XSetBackground(&browserDisplay.display, textGC, 0x00334455UL)
		    != Success)) {
	goto cleanup;
    }
    Xutf8DrawString(&browserDisplay.display, (Drawable)textPixmap, fontSet,
	    textGC, 1, 12, "utf8", 4);
    XmbDrawString(&browserDisplay.display, (Drawable)textPixmap, fontSet,
	    textGC, 1, 20, "mb", 2);
    XwcDrawString(&browserDisplay.display, (Drawable)textPixmap, fontSet,
	    textGC, 28, 20, fontDrawWideText, 1);
    {
	XmbTextItem utf8Items[2];
	XwcTextItem wideItems[1];

	memset(utf8Items, 0, sizeof(utf8Items));
	memset(wideItems, 0, sizeof(wideItems));
	utf8Items[0].chars = (char *)"A";
	utf8Items[0].nchars = 1;
	utf8Items[0].delta = 1;
	utf8Items[0].font_set = fontSet;
	utf8Items[1].chars = (char *)"B";
	utf8Items[1].nchars = 1;
	utf8Items[1].font_set = fontSet;
	Xutf8DrawText(&browserDisplay.display, (Drawable)textPixmap, textGC,
		1, 28, utf8Items, 2);
	XmbDrawText(&browserDisplay.display, (Drawable)textPixmap, textGC,
		32, 28, utf8Items, 2);
	wideItems[0].chars = fontDrawWideText;
	wideItems[0].nchars = 1;
	wideItems[0].font_set = fontSet;
	XwcDrawText(&browserDisplay.display, (Drawable)textPixmap, textGC,
		64, 28, wideItems, 1);
    }
    Xutf8DrawImageString(&browserDisplay.display, (Drawable)textPixmap,
	    fontSet, textGC, 4, 24, " ", 1);
    (void)XFlush(&browserDisplay.display);
    textReadback = XGetImage(&browserDisplay.display, textPixmap, 4,
	    24 + fontLogicalExtent.y, 1, 1, AllPlanes, ZPixmap);
    if ((textReadback == NULL)
	    || ((XGetPixel(textReadback, 0, 0) & 0x00ffffffUL)
		    != 0x00334455UL)) {
	goto cleanup;
    }
    (void)XDestroyImage(textReadback);
    textReadback = NULL;
    (void)XFreeGC(&browserDisplay.display, textGC);
    textGC = NULL;
    (void)XFreePixmap(&browserDisplay.display, textPixmap);
    textPixmap = None;

    inputMethod = XOpenIM(&browserDisplay.display, NULL, NULL, NULL);
    testCursor = XCreateFontCursor(&browserDisplay.display, XC_watch);
    if (testCursor == None) {
	goto cleanup;
    }
    preeditArea.x = 1;
    preeditArea.y = 2;
    preeditArea.width = 80;
    preeditArea.height = 20;
    preeditAreaNeeded.x = 3;
    preeditAreaNeeded.y = 4;
    preeditAreaNeeded.width = 90;
    preeditAreaNeeded.height = 30;
    statusArea.x = 5;
    statusArea.y = 6;
    statusArea.width = 70;
    statusArea.height = 16;
    preeditSpot.x = 9;
    preeditSpot.y = 13;
    preeditAttributes = XVaCreateNestedList(0,
	    XNArea, &preeditArea,
	    XNAreaNeeded, &preeditAreaNeeded,
	    XNSpotLocation, &preeditSpot,
	    XNForeground, preeditForeground,
	    XNBackground, preeditBackground,
	    XNFontSet, fontSet,
	    XNLineSpace, preeditLineSpace,
	    XNCursor, testCursor,
	    NULL);
    statusAttributes = XVaCreateNestedList(0,
	    XNArea, &statusArea,
	    XNBackground, statusBackground,
	    NULL);
    inputContext = XCreateIC(inputMethod,
	    XNInputStyle, XIMPreeditPosition | XIMStatusNothing,
	    XNClientWindow, browserDisplay.rootWindow,
	    XNFocusWindow, browserDisplay.rootWindow,
	    XNPreeditAttributes, preeditAttributes,
	    XNStatusAttributes, statusAttributes,
	    NULL);
    XFree(preeditAttributes);
    preeditAttributes = NULL;
    XFree(statusAttributes);
    statusAttributes = NULL;
    memset(&preeditSpotReturn, 0x7f, sizeof(preeditSpotReturn));
    memset(&preeditAreaReturn, 0x7f, sizeof(preeditAreaReturn));
    memset(&preeditAreaNeededReturn, 0x7f, sizeof(preeditAreaNeededReturn));
    memset(&statusAreaReturn, 0x7f, sizeof(statusAreaReturn));
    preeditFontSetReturn = NULL;
    preeditForegroundReturn = 0;
    preeditBackgroundReturn = 0;
    preeditLineSpaceReturn = 0;
    preeditCursorReturn = None;
    statusBackgroundReturn = 0;
    preeditAttributes = XVaCreateNestedList(0,
	    XNArea, &preeditAreaReturn,
	    XNAreaNeeded, &preeditAreaNeededReturn,
	    XNSpotLocation, &preeditSpotReturn,
	    XNForeground, &preeditForegroundReturn,
	    XNBackground, &preeditBackgroundReturn,
	    XNFontSet, &preeditFontSetReturn,
	    XNLineSpace, &preeditLineSpaceReturn,
	    XNCursor, &preeditCursorReturn,
	    NULL);
    statusAttributes = XVaCreateNestedList(0,
	    XNArea, &statusAreaReturn,
	    XNBackground, &statusBackgroundReturn,
	    NULL);
    if ((inputMethod == NULL) || (inputContext == NULL)
	    || (XGetICValues(inputContext, XNInputStyle, &icInputStyleReturn,
		    XNPreeditAttributes, preeditAttributes,
		    XNStatusAttributes, statusAttributes, NULL) != NULL)
	    || (icInputStyleReturn
		    != (XIMPreeditPosition | XIMStatusNothing))
	    || (preeditAreaReturn.x != 1) || (preeditAreaReturn.y != 2)
	    || (preeditAreaReturn.width != 80)
	    || (preeditAreaReturn.height != 20)
	    || (preeditAreaNeededReturn.x != 3)
	    || (preeditAreaNeededReturn.y != 4)
	    || (preeditAreaNeededReturn.width != 90)
	    || (preeditAreaNeededReturn.height != 30)
	    || (preeditSpotReturn.x != 9) || (preeditSpotReturn.y != 13)
	    || (preeditForegroundReturn != preeditForeground)
	    || (preeditBackgroundReturn != preeditBackground)
	    || (preeditFontSetReturn != fontSet)
	    || (preeditLineSpaceReturn != preeditLineSpace)
	    || (preeditCursorReturn != testCursor)
	    || (statusAreaReturn.x != 5) || (statusAreaReturn.y != 6)
	    || (statusAreaReturn.width != 70)
	    || (statusAreaReturn.height != 16)
	    || (statusBackgroundReturn != statusBackground)) {
	goto cleanup;
    }
    XFree(preeditAttributes);
    preeditAttributes = NULL;
    XFree(statusAttributes);
    statusAttributes = NULL;
    (void)XFreeCursor(&browserDisplay.display, testCursor);
    testCursor = None;

    preeditSpot.x = 21;
    preeditSpot.y = 34;
    preeditAttributes = XVaCreateNestedList(0, XNSpotLocation, &preeditSpot,
	    NULL);
    if (XSetICValues(inputContext, XNPreeditAttributes, preeditAttributes,
	    NULL) != NULL) {
	goto cleanup;
    }
    XFree(preeditAttributes);
    preeditAttributes = NULL;
    memset(&preeditSpotReturn, 0, sizeof(preeditSpotReturn));
    preeditFontSetReturn = NULL;
    preeditAttributes = XVaCreateNestedList(0,
	    XNSpotLocation, &preeditSpotReturn,
	    XNFontSet, &preeditFontSetReturn,
	    NULL);
    if ((XGetICValues(inputContext, XNPreeditAttributes, preeditAttributes,
		    NULL) != NULL)
	    || (preeditSpotReturn.x != 21) || (preeditSpotReturn.y != 34)
	    || (preeditFontSetReturn != fontSet)) {
	goto cleanup;
    }
    XFree(preeditAttributes);
    preeditAttributes = NULL;
    XDestroyIC(inputContext);
    inputContext = XCreateIC(inputMethod,
	    XNInputStyle, XIMPreeditPosition | XIMStatusNothing,
	    XNClientWindow, browserDisplay.rootWindow,
	    XNFocusWindow, browserDisplay.rootWindow,
	    NULL);
    preeditFontSetReturn = fontSet;
    preeditAttributes = XVaCreateNestedList(0, XNFontSet,
	    &preeditFontSetReturn, NULL);
    if ((inputContext == NULL)
	    || (XGetICValues(inputContext, XNPreeditAttributes,
		    preeditAttributes, NULL) != NULL)
	    || (preeditFontSetReturn != NULL)) {
	goto cleanup;
    }
    XFree(preeditAttributes);
    preeditAttributes = NULL;
    XDestroyIC(inputContext);
    inputContext = NULL;
    (void)XCloseIM(inputMethod);
    inputMethod = NULL;

    titleValues[0] = (char *)"Roundtrip Title";
    iconValues[0] = (char *)"Roundtrip Icon";
    machineValues[0] = (char *)"browser-host";
    customValues[0] = (char *)"custom text";
    commandArgv[0] = (char *)"x11wasm-test";
    commandArgv[1] = (char *)"x11-app";
    if (!XStringListToTextProperty(titleValues, 1, &titleProp)
	    || !XStringListToTextProperty(iconValues, 1, &iconProp)
	    || !XStringListToTextProperty(machineValues, 1, &machineProp)
	    || !XStringListToTextProperty(customValues, 1, &customProp)) {
	goto cleanup;
    }
    classHint.res_name = (char *)"x11wasm";
    classHint.res_class = (char *)"X11WasmTest";
    normalHints.flags = PMinSize | PMaxSize | PResizeInc | PBaseSize;
    normalHints.min_width = 12;
    normalHints.min_height = 13;
    normalHints.max_width = 240;
    normalHints.max_height = 241;
    normalHints.width_inc = 2;
    normalHints.height_inc = 3;
    normalHints.base_width = 10;
    normalHints.base_height = 11;
    wmHints.flags = InputHint | StateHint;
    wmHints.input = True;
    wmHints.initial_state = NormalState;
    XSetWMProperties(&browserDisplay.display, window, &titleProp, &iconProp,
	    commandArgv, 2, &normalHints, &wmHints, &classHint);
    if (!XGetWMName(&browserDisplay.display, window, &readProp)
	    || !BrowserTestTextPropertyEquals(&readProp, "Roundtrip Title")) {
	goto cleanup;
    }
    XFree(readProp.value);
    memset(&readProp, 0, sizeof(readProp));
    if (!XGetWMIconName(&browserDisplay.display, window, &readProp)
	    || !BrowserTestTextPropertyEquals(&readProp, "Roundtrip Icon")) {
	goto cleanup;
    }
    XFree(readProp.value);
    memset(&readProp, 0, sizeof(readProp));
    if (!XGetClassHint(&browserDisplay.display, window, &classHintReturn)
	    || (strcmp(classHintReturn.res_name, "x11wasm") != 0)
	    || (strcmp(classHintReturn.res_class, "X11WasmTest") != 0)) {
	goto cleanup;
    }
    XFree(classHintReturn.res_name);
    XFree(classHintReturn.res_class);
    memset(&classHintReturn, 0, sizeof(classHintReturn));
    if (!XGetCommand(&browserDisplay.display, window, &commandArgvReturn,
	    &commandArgcReturn)
	    || (commandArgcReturn != 2)
	    || (strcmp(commandArgvReturn[0], "x11wasm-test") != 0)
	    || (strcmp(commandArgvReturn[1], "x11-app") != 0)) {
	goto cleanup;
    }
    XFreeStringList(commandArgvReturn);
    commandArgvReturn = NULL;
    commandArgcReturn = -1;
    wmHintsReturn = XGetWMHints(&browserDisplay.display, window);
    if ((wmHintsReturn == NULL)
	    || (wmHintsReturn->flags != wmHints.flags)
	    || (wmHintsReturn->input != True)
	    || (wmHintsReturn->initial_state != NormalState)) {
	goto cleanup;
    }
    XFree(wmHintsReturn);
    wmHintsReturn = NULL;
    if (XSetWMHints(&browserDisplay.display, window, NULL) != Success) {
	goto cleanup;
    }
    wmHintsReturn = XGetWMHints(&browserDisplay.display, window);
    if (wmHintsReturn != NULL) {
	goto cleanup;
    }
    if (XSetWMHints(&browserDisplay.display, window, &wmHints) != Success) {
	goto cleanup;
    }
    wmHintsReturn = XGetWMHints(&browserDisplay.display, window);
    if ((wmHintsReturn == NULL)
	    || (wmHintsReturn->flags != wmHints.flags)
	    || (wmHintsReturn->input != True)
	    || (wmHintsReturn->initial_state != NormalState)) {
	goto cleanup;
    }
    XFree(wmHintsReturn);
    wmHintsReturn = NULL;
    if (!XGetWMNormalHints(&browserDisplay.display, window,
	    &normalHintsReturn, &suppliedHints)
	    || (suppliedHints != allSizeHintsSupplied)
	    || (normalHintsReturn.flags != normalHints.flags)
	    || (normalHintsReturn.min_width != 12)
	    || (normalHintsReturn.min_height != 13)
	    || (normalHintsReturn.max_width != 240)
	    || (normalHintsReturn.max_height != 241)
	    || (normalHintsReturn.width_inc != 2)
	    || (normalHintsReturn.height_inc != 3)
	    || (normalHintsReturn.base_width != 10)
	    || (normalHintsReturn.base_height != 11)) {
	goto cleanup;
    }
    memset(oldSizeHints, 0, sizeof(oldSizeHints));
    oldSizeHintsFlags = USPosition | USSize | PMinSize | PMaxSize
	    | PResizeInc | PAspect | PBaseSize | PWinGravity | (1L << 30);
    oldSizeHints[0] = (unsigned int)oldSizeHintsFlags;
    oldSizeHints[5] = 21;
    oldSizeHints[6] = 22;
    oldSizeHints[7] = 421;
    oldSizeHints[8] = 422;
    oldSizeHints[9] = 4;
    oldSizeHints[10] = 5;
    oldSizeHints[11] = 3;
    oldSizeHints[12] = 2;
    oldSizeHints[13] = 16;
    oldSizeHints[14] = 9;
    suppliedHints = -1;
    memset(&normalHintsReturn, 0xff, sizeof(normalHintsReturn));
    if ((XChangeProperty(&browserDisplay.display, window, XA_WM_NORMAL_HINTS,
	    XA_WM_SIZE_HINTS, 32, PropModeReplace,
	    (const unsigned char *)oldSizeHints, 15) != Success)
	    || !XGetWMSizeHints(&browserDisplay.display, window,
		    &normalHintsReturn, &suppliedHints, XA_WM_NORMAL_HINTS)
	    || (suppliedHints != oldSizeHintsSupplied)
	    || (normalHintsReturn.flags != (oldSizeHintsFlags
		    & oldSizeHintsSupplied))
	    || (normalHintsReturn.min_width != 21)
	    || (normalHintsReturn.min_height != 22)
	    || (normalHintsReturn.max_width != 421)
	    || (normalHintsReturn.max_height != 422)
	    || (normalHintsReturn.width_inc != 4)
	    || (normalHintsReturn.height_inc != 5)
	    || (normalHintsReturn.min_aspect.x != 3)
	    || (normalHintsReturn.min_aspect.y != 2)
	    || (normalHintsReturn.max_aspect.x != 16)
	    || (normalHintsReturn.max_aspect.y != 9)
	    || (normalHintsReturn.base_width != 0)
	    || (normalHintsReturn.base_height != 0)
	    || (normalHintsReturn.win_gravity != 0)) {
	goto cleanup;
    }
    XSetWMSizeHints(&browserDisplay.display, window, NULL,
	    XA_WM_NORMAL_HINTS);
    suppliedHints = -1;
    memset(&normalHintsReturn, 0, sizeof(normalHintsReturn));
    if (XGetWMNormalHints(&browserDisplay.display, window,
	    &normalHintsReturn, &suppliedHints) || (suppliedHints != 0)) {
	goto cleanup;
    }
    XSetWMNormalHints(&browserDisplay.display, window, &normalHints);
    memset(&normalHintsReturn, 0, sizeof(normalHintsReturn));
    suppliedHints = 0;
    if (!XGetWMNormalHints(&browserDisplay.display, window,
	    &normalHintsReturn, &suppliedHints)
	    || (suppliedHints != allSizeHintsSupplied)
	    || (normalHintsReturn.flags != normalHints.flags)
	    || (normalHintsReturn.min_width != 12)
	    || (normalHintsReturn.min_height != 13)
	    || (normalHintsReturn.max_width != 240)
	    || (normalHintsReturn.max_height != 241)
	    || (normalHintsReturn.width_inc != 2)
	    || (normalHintsReturn.height_inc != 3)
	    || (normalHintsReturn.base_width != 10)
	    || (normalHintsReturn.base_height != 11)) {
	goto cleanup;
    }
    XSetWMClientMachine(&browserDisplay.display, window, &machineProp);
    if (!XGetWMClientMachine(&browserDisplay.display, window, &readProp)
	    || !BrowserTestTextPropertyEquals(&readProp, "browser-host")) {
	goto cleanup;
    }
    XFree(readProp.value);
    memset(&readProp, 0, sizeof(readProp));
    customTextAtom = XInternAtom(&browserDisplay.display,
	    "X11_WASM_TEST_TEXT_PROPERTY", False);
    XSetTextProperty(&browserDisplay.display, window, &customProp,
	    customTextAtom);
    if (!XGetTextProperty(&browserDisplay.display, window, &readProp,
	    customTextAtom)
	    || !BrowserTestTextPropertyEquals(&readProp, "custom text")) {
	goto cleanup;
    }
    XFree(readProp.value);
    memset(&readProp, 0, sizeof(readProp));

    normalHints.min_width = 31;
    normalHints.min_height = 32;
    (void)XSetNormalHints(&browserDisplay.display, window, &normalHints);
    memset(&normalHintsReturn, 0, sizeof(normalHintsReturn));
    if (!XGetNormalHints(&browserDisplay.display, window, &normalHintsReturn)
	    || (normalHintsReturn.min_width != 31)
	    || (normalHintsReturn.min_height != 32)) {
	goto cleanup;
    }
    customHintsAtom = XInternAtom(&browserDisplay.display,
	    "X11_WASM_TEST_SIZE_HINTS", False);
    customHints.flags = PPosition | PSize;
    customHints.x = 5;
    customHints.y = 6;
    customHints.width = 70;
    customHints.height = 80;
    (void)XSetSizeHints(&browserDisplay.display, window, &customHints,
	    customHintsAtom);
    if (!XGetSizeHints(&browserDisplay.display, window, &customHintsReturn,
	    customHintsAtom)
	    || (customHintsReturn.flags != customHints.flags)
	    || (customHintsReturn.x != 5)
	    || (customHintsReturn.y != 6)
	    || (customHintsReturn.width != 70)
	    || (customHintsReturn.height != 80)) {
	goto cleanup;
    }
    memset(&customHintsReturn, 0, sizeof(customHintsReturn));
    if (XGetZoomHints(&browserDisplay.display, window, &customHintsReturn)) {
	goto cleanup;
    }
    memset(&customHints, 0, sizeof(customHints));
    customHints.flags = PMinSize | PMaxSize | PResizeInc;
    customHints.min_width = 41;
    customHints.min_height = 42;
    customHints.max_width = 410;
    customHints.max_height = 420;
    customHints.width_inc = 4;
    customHints.height_inc = 5;
    (void)XSetZoomHints(&browserDisplay.display, window, &customHints);
    memset(&customHintsReturn, 0, sizeof(customHintsReturn));
    if (!XGetZoomHints(&browserDisplay.display, window, &customHintsReturn)
	    || (customHintsReturn.flags != customHints.flags)
	    || (customHintsReturn.min_width != 41)
	    || (customHintsReturn.min_height != 42)
	    || (customHintsReturn.max_width != 410)
	    || (customHintsReturn.max_height != 420)
	    || (customHintsReturn.width_inc != 4)
	    || (customHintsReturn.height_inc != 5)) {
	goto cleanup;
    }
    iconSizes[0].min_width = 8;
    iconSizes[0].min_height = 8;
    iconSizes[0].max_width = 16;
    iconSizes[0].max_height = 16;
    iconSizes[0].width_inc = 1;
    iconSizes[0].height_inc = 1;
    iconSizes[1].min_width = 32;
    iconSizes[1].min_height = 32;
    iconSizes[1].max_width = 64;
    iconSizes[1].max_height = 64;
    iconSizes[1].width_inc = 2;
    iconSizes[1].height_inc = 2;
    if ((XSetIconSizes(&browserDisplay.display, window, iconSizes, 2)
	    != Success)
	    || !XGetIconSizes(&browserDisplay.display, window,
		    &iconSizesReturn, &iconSizeCount)
	    || (iconSizeCount != 2)
	    || (iconSizesReturn[0].min_width != 8)
	    || (iconSizesReturn[1].max_height != 64)
	    || (iconSizesReturn[1].width_inc != 2)) {
	goto cleanup;
    }
    XFree(iconSizesReturn);
    iconSizesReturn = NULL;
    result = 50;
    standardColormapAtom = XInternAtom(&browserDisplay.display,
	    "X11_WASM_TEST_RGB_COLORMAP", False);
    if (XGetRGBColormaps(&browserDisplay.display, window,
	    &standardColormapsReturn, &standardColormapCount,
	    standardColormapAtom)
	    || (standardColormapCount != 0)
	    || (standardColormapsReturn != NULL)) {
	goto cleanup;
    }
    standardColormaps[0].colormap = firstColormap;
    standardColormaps[0].red_max = 255;
    standardColormaps[0].red_mult = 1;
    standardColormaps[0].green_max = 127;
    standardColormaps[0].green_mult = 2;
    standardColormaps[0].blue_max = 63;
    standardColormaps[0].blue_mult = 4;
    standardColormaps[0].base_pixel = 5;
    standardColormaps[0].visualid =
	    XVisualIDFromVisual(DefaultVisual(&browserDisplay.display, screen));
    standardColormaps[0].killid = ReleaseByFreeingColormap;
    standardColormaps[1].colormap = secondColormap;
    standardColormaps[1].red_max = 31;
    standardColormaps[1].red_mult = 8;
    standardColormaps[1].green_max = 15;
    standardColormaps[1].green_mult = 16;
    standardColormaps[1].blue_max = 7;
    standardColormaps[1].blue_mult = 32;
    standardColormaps[1].base_pixel = 9;
    standardColormaps[1].visualid = standardColormaps[0].visualid;
    standardColormaps[1].killid = None;
    XSetRGBColormaps(&browserDisplay.display, window, standardColormaps, 2,
	    standardColormapAtom);
    if (!XGetRGBColormaps(&browserDisplay.display, window,
	    &standardColormapsReturn, &standardColormapCount,
	    standardColormapAtom)
	    || (standardColormapCount != 2)
	    || (standardColormapsReturn == NULL)
	    || (standardColormapsReturn[0].colormap != firstColormap)
	    || (standardColormapsReturn[0].red_max != 255)
	    || (standardColormapsReturn[0].green_mult != 2)
	    || (standardColormapsReturn[0].blue_mult != 4)
	    || (standardColormapsReturn[0].base_pixel != 5)
	    || (standardColormapsReturn[0].visualid
		    != standardColormaps[0].visualid)
	    || (standardColormapsReturn[0].killid
		    != ReleaseByFreeingColormap)
	    || (standardColormapsReturn[1].colormap != secondColormap)
	    || (standardColormapsReturn[1].red_mult != 8)
	    || (standardColormapsReturn[1].green_max != 15)
	    || (standardColormapsReturn[1].blue_max != 7)
	    || (standardColormapsReturn[1].base_pixel != 9)) {
	goto cleanup;
    }
    XFree(standardColormapsReturn);
    standardColormapsReturn = NULL;
    standardColormapCount = -1;
    memset(&standardColormapReturn, 0, sizeof(standardColormapReturn));
    if (!XGetStandardColormap(&browserDisplay.display, window,
	    &standardColormapReturn, standardColormapAtom)
	    || (standardColormapReturn.colormap != firstColormap)
	    || (standardColormapReturn.green_max != 127)
	    || (standardColormapReturn.blue_max != 63)
	    || (standardColormapReturn.killid != ReleaseByFreeingColormap)) {
	goto cleanup;
    }
    XSetStandardColormap(&browserDisplay.display, window,
	    &standardColormaps[1], XA_RGB_DEFAULT_MAP);
    memset(&standardColormapReturn, 0, sizeof(standardColormapReturn));
    if (!XGetStandardColormap(&browserDisplay.display, window,
	    &standardColormapReturn, XA_RGB_DEFAULT_MAP)
	    || (standardColormapReturn.colormap != secondColormap)
	    || (standardColormapReturn.red_max != 31)
	    || (standardColormapReturn.green_mult != 16)
	    || (standardColormapReturn.blue_mult != 32)
	    || (standardColormapReturn.killid != None)) {
	goto cleanup;
    }
    XSetRGBColormaps(&browserDisplay.display, window, NULL, 0,
	    standardColormapAtom);
    standardColormapCount = -1;
    if (XGetRGBColormaps(&browserDisplay.display, window,
	    &standardColormapsReturn, &standardColormapCount,
	    standardColormapAtom)
	    || (standardColormapCount != 0)
	    || (standardColormapsReturn != NULL)
	    || XGetStandardColormap(&browserDisplay.display, window,
		    &standardColormapReturn, standardColormapAtom)) {
	goto cleanup;
    }
    colormapWindowList[0] = window;
    colormapWindowList[1] = browserDisplay.rootWindow;
    if ((XSetWMColormapWindows(&browserDisplay.display, window,
	    colormapWindowList, 2) != True)
	    || !XGetWMColormapWindows(&browserDisplay.display, window,
		    &colormapWindowsReturn, &colormapWindowCount)
	    || (colormapWindowCount != 2)
	    || (colormapWindowsReturn == NULL)
	    || (colormapWindowsReturn[0] != colormapWindowList[0])
	    || (colormapWindowsReturn[1] != colormapWindowList[1])) {
	goto cleanup;
    }
    XFree(colormapWindowsReturn);
    colormapWindowsReturn = NULL;
    colormapWindowCount = -1;
    if ((XSetWMColormapWindows(&browserDisplay.display, window, NULL, 0)
	    != True)
	    || !XGetWMColormapWindows(&browserDisplay.display, window,
		    &colormapWindowsReturn, &colormapWindowCount)
	    || (colormapWindowCount != 0)
	    || (colormapWindowsReturn != NULL)) {
	goto cleanup;
    }

    protocolAtoms[0] = XInternAtom(&browserDisplay.display,
	    "X11_WASM_TEST_PROTOCOL_ONE", False);
    protocolAtoms[1] = XInternAtom(&browserDisplay.display,
	    "X11_WASM_TEST_PROTOCOL_TWO", False);
    if ((XSetWMProtocols(&browserDisplay.display, window, protocolAtoms, 2)
	    != True)
	    || !XGetWMProtocols(&browserDisplay.display, window,
		    &protocolAtomsReturn, &protocolAtomCount)
	    || (protocolAtomCount != 2)
	    || (protocolAtomsReturn == NULL)
	    || (protocolAtomsReturn[0] != protocolAtoms[0])
	    || (protocolAtomsReturn[1] != protocolAtoms[1])) {
	goto cleanup;
    }
    XFree(protocolAtomsReturn);
    protocolAtomsReturn = NULL;
    protocolAtomCount = -1;
    if ((XSetWMProtocols(&browserDisplay.display, window, NULL, 0)
	    != True)
	    || !XGetWMProtocols(&browserDisplay.display, window,
		    &protocolAtomsReturn, &protocolAtomCount)
	    || (protocolAtomCount != 0)
	    || (protocolAtomsReturn != NULL)) {
	goto cleanup;
    }

    if ((XSetTransientForHint(&browserDisplay.display, window,
	    browserDisplay.rootWindow) != Success)
	    || !XGetTransientForHint(&browserDisplay.display, window,
		    &transientForReturn)
	    || (transientForReturn != browserDisplay.rootWindow)) {
	goto cleanup;
    }
    if ((XGetWindowProperty(&browserDisplay.display, window,
		    XA_WM_TRANSIENT_FOR, 0, 1, False, XA_WINDOW,
		    &transientForActualType, &transientForActualFormat,
		    &transientForItemCount, &transientForBytesAfter,
		    &transientForData) != Success)
	    || (transientForActualType != XA_WINDOW)
	    || (transientForActualFormat != 32)
	    || (transientForItemCount != 1)
	    || (transientForBytesAfter != 0)
	    || (transientForData == NULL)
	    || ((Window)BrowserPropertyAtomAt(transientForData, 0)
		    != browserDisplay.rootWindow)) {
	goto cleanup;
    }
    XFree(transientForData);
    transientForData = NULL;
    (void)XDeleteProperty(&browserDisplay.display, window,
	    XA_WM_TRANSIENT_FOR);
    transientForReturn = None;
    if (XGetTransientForHint(&browserDisplay.display, window,
	    &transientForReturn) || (transientForReturn != None)) {
	goto cleanup;
    }

    (void)XSetStandardProperties(&browserDisplay.display, window,
	    "Standard Title", "Standard Icon", pixmap, commandArgv, 2,
	    &normalHints);
    if (!XGetWMName(&browserDisplay.display, window, &readProp)
	    || !BrowserTestTextPropertyEquals(&readProp, "Standard Title")) {
	goto cleanup;
    }
    XFree(readProp.value);
    memset(&readProp, 0, sizeof(readProp));
    if (!XGetWMIconName(&browserDisplay.display, window, &readProp)
	    || !BrowserTestTextPropertyEquals(&readProp, "Standard Icon")) {
	goto cleanup;
    }
    XFree(readProp.value);
    memset(&readProp, 0, sizeof(readProp));
    wmHintsReturn = XGetWMHints(&browserDisplay.display, window);
    if ((wmHintsReturn == NULL)
	    || ((wmHintsReturn->flags & IconPixmapHint) == 0)
	    || (wmHintsReturn->icon_pixmap != pixmap)) {
	goto cleanup;
    }
    XFree(wmHintsReturn);
    wmHintsReturn = NULL;

    XmbSetWMProperties(&browserDisplay.display, window, "MB Title",
	    "MB Icon", commandArgv, 2, &normalHints, &wmHints, &classHint);
    if (!XGetWMName(&browserDisplay.display, window, &readProp)
	    || !BrowserTestTextPropertyEquals(&readProp, "MB Title")) {
	goto cleanup;
    }
    XFree(readProp.value);
    memset(&readProp, 0, sizeof(readProp));
    Xutf8SetWMProperties(&browserDisplay.display, window, "UTF8 Title",
	    "UTF8 Icon", commandArgv, 2, &normalHints, &wmHints, &classHint);
    if (!XGetWMName(&browserDisplay.display, window, &readProp)
	    || (readProp.encoding != BrowserUtf8StringAtom())
	    || !BrowserTestTextPropertyEquals(&readProp, "UTF8 Title")) {
	goto cleanup;
    }
    XFree(readProp.value);
    memset(&readProp, 0, sizeof(readProp));

    propertyAtoms = XListProperties(&browserDisplay.display, window,
	    &propertyAtomCount);
    if ((propertyAtoms == NULL) || (propertyAtomCount < 4)
	    || !BrowserTestAtomListContains(propertyAtoms, propertyAtomCount,
		    XA_WM_NAME)
	    || !BrowserTestAtomListContains(propertyAtoms, propertyAtomCount,
		    XA_WM_ICON_NAME)
	    || !BrowserTestAtomListContains(propertyAtoms, propertyAtomCount,
		    XA_WM_NORMAL_HINTS)
	    || !BrowserTestAtomListContains(propertyAtoms, propertyAtomCount,
		    customTextAtom)) {
	goto cleanup;
    }
    XFree(propertyAtoms);
    propertyAtoms = NULL;

    rotatePropertyAtoms[0] = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_ROTATE_A", False);
    rotatePropertyAtoms[1] = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_ROTATE_B", False);
    rotatePropertyAtoms[2] = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_ROTATE_C", False);
    if ((rotatePropertyAtoms[0] == None) || (rotatePropertyAtoms[1] == None)
	    || (rotatePropertyAtoms[2] == None)
	    || (XChangeProperty(&browserDisplay.display, window,
		    rotatePropertyAtoms[0], XA_STRING, 8, PropModeReplace,
		    (const unsigned char *)"a", 1) != Success)
	    || (XChangeProperty(&browserDisplay.display, window,
		    rotatePropertyAtoms[1], XA_STRING, 8, PropModeReplace,
		    (const unsigned char *)"b", 1) != Success)
	    || (XChangeProperty(&browserDisplay.display, window,
		    rotatePropertyAtoms[2], XA_STRING, 8, PropModeReplace,
		    (const unsigned char *)"c", 1) != Success)
	    || (XRotateWindowProperties(&browserDisplay.display, window,
		    rotatePropertyAtoms, 3, 1) != Success)
	    || (XGetWindowProperty(&browserDisplay.display, window,
		    rotatePropertyAtoms[0], 0, 1, False, XA_STRING,
		    &rotateActualType, &rotateActualFormat, &rotateNitems,
		    &rotateBytesAfter, &rotatePropertyData) != Success)
	    || (rotateActualType != XA_STRING) || (rotateActualFormat != 8)
	    || (rotateNitems != 1) || (rotateBytesAfter != 0)
	    || (rotatePropertyData == NULL) || (rotatePropertyData[0] != 'c')) {
	goto cleanup;
    }
    XFree(rotatePropertyData);
    rotatePropertyData = NULL;
    if ((XGetWindowProperty(&browserDisplay.display, window,
		    rotatePropertyAtoms[1], 0, 1, False, XA_STRING,
		    &rotateActualType, &rotateActualFormat, &rotateNitems,
		    &rotateBytesAfter, &rotatePropertyData) != Success)
	    || (rotateActualType != XA_STRING) || (rotateActualFormat != 8)
	    || (rotateNitems != 1) || (rotateBytesAfter != 0)
	    || (rotatePropertyData == NULL) || (rotatePropertyData[0] != 'a')) {
	goto cleanup;
    }
    XFree(rotatePropertyData);
    rotatePropertyData = NULL;

    if (XStoreBytes(&browserDisplay.display, "alpha", 5) != Success) {
	goto cleanup;
    }
    fetchedCutBuffer = XFetchBytes(&browserDisplay.display,
	    &cutBufferLength);
    if ((fetchedCutBuffer == NULL) || (cutBufferLength != 5)
	    || (memcmp(fetchedCutBuffer, "alpha", 5) != 0)) {
	goto cleanup;
    }
    XFree(fetchedCutBuffer);
    fetchedCutBuffer = NULL;
    for (cutBufferIndex = 0; cutBufferIndex < 8; cutBufferIndex++) {
	if (XStoreBuffer(&browserDisplay.display,
		cutBufferValues + cutBufferIndex, 1, cutBufferIndex)
		!= Success) {
	    goto cleanup;
	}
    }
    if (XRotateBuffers(&browserDisplay.display, 2) != Success) {
	goto cleanup;
    }
    fetchedCutBuffer = XFetchBuffer(&browserDisplay.display,
	    &cutBufferLength, 0);
    if ((fetchedCutBuffer == NULL) || (cutBufferLength != 1)
	    || (fetchedCutBuffer[0] != '6')) {
	goto cleanup;
    }
    XFree(fetchedCutBuffer);
    fetchedCutBuffer = NULL;
    fetchedCutBuffer = XFetchBuffer(&browserDisplay.display,
	    &cutBufferLength, 2);
    if ((fetchedCutBuffer == NULL) || (cutBufferLength != 1)
	    || (fetchedCutBuffer[0] != '0')) {
	goto cleanup;
    }
    XFree(fetchedCutBuffer);
    fetchedCutBuffer = NULL;

    stateWindowPtr = X11WasmFindResource(window);
    netWmStateAtom = BrowserNetWmStateAtom();
    maxVertAtom = XInternAtom(&browserDisplay.display,
	    "_NET_WM_STATE_MAXIMIZED_VERT", False);
    maxHorzAtom = XInternAtom(&browserDisplay.display,
	    "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    netWmStateEvent.xclient.type = ClientMessage;
    netWmStateEvent.xclient.display = &browserDisplay.display;
    netWmStateEvent.xclient.window = window;
    netWmStateEvent.xclient.message_type = netWmStateAtom;
    netWmStateEvent.xclient.format = 32;
    netWmStateEvent.xclient.data.l[0] = 1;
    netWmStateEvent.xclient.data.l[1] = (long)maxVertAtom;
    netWmStateEvent.xclient.data.l[2] = (long)maxHorzAtom;
    netWmStateAddResult = XSendEvent(&browserDisplay.display, window, False,
	    NoEventMask, &netWmStateEvent);
    netWmStateProperty = BrowserFindProperty(stateWindowPtr, netWmStateAtom);
    if ((stateWindowPtr == NULL) || (netWmStateAddResult != True)
	    || !BrowserWindowHasStateAtom(netWmStateProperty, maxVertAtom)
	    || !BrowserWindowHasStateAtom(netWmStateProperty, maxHorzAtom)) {
	goto cleanup;
    }
    netWmStateEvent.xclient.data.l[0] = 0;
    netWmStateRemoveResult = XSendEvent(&browserDisplay.display, window,
	    False, NoEventMask, &netWmStateEvent);
    netWmStateProperty = BrowserFindProperty(stateWindowPtr, netWmStateAtom);
    if ((netWmStateRemoveResult != True)
	    || BrowserWindowHasStateAtom(netWmStateProperty, maxVertAtom)
	    || BrowserWindowHasStateAtom(netWmStateProperty, maxHorzAtom)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    if (!XFetchName(&browserDisplay.display, window, &fetchedName)
	    || (fetchedName == NULL)
	    || (strcmp(fetchedName, "UTF8 Title") != 0)) {
	goto cleanup;
    }
    XFree(fetchedName);
    fetchedName = NULL;
    if (!XGetIconName(&browserDisplay.display, window, &fetchedIconName)
	    || (fetchedIconName == NULL)
	    || (strcmp(fetchedIconName, "UTF8 Icon") != 0)) {
	goto cleanup;
    }
    XFree(fetchedIconName);
    fetchedIconName = NULL;

    fontAtomName = XGetAtomName(&browserDisplay.display, XA_FONT_NAME);
    if ((XInternAtom(&browserDisplay.display, "FONT_NAME", True)
	    != XA_FONT_NAME) || (fontAtomName == NULL)
	    || (strcmp(fontAtomName, "FONT_NAME") != 0)) {
	goto cleanup;
    }
    XFree(fontAtomName);
    fontAtomName = NULL;
    batchAtomNames[0] = "FONT_NAME";
    batchAtomNames[1] = "_X11_WASM_BATCH_ATOM";
    batchAtomNames[2] = "_X11_WASM_BATCH_ATOM_DOES_NOT_EXIST";
    if ((XInternAtoms(&browserDisplay.display, batchAtomNames, 2, False,
		    batchAtoms) != True)
	    || (batchAtoms[0] != XA_FONT_NAME)
	    || (batchAtoms[1] == None)
	    || (XInternAtom(&browserDisplay.display, batchAtomNames[1], True)
		    != batchAtoms[1])
	    || (XInternAtoms(&browserDisplay.display, &batchAtomNames[2], 1,
		    True, &missingBatchAtom) != True)
	    || (missingBatchAtom != None)
	    || (XGetAtomNames(&browserDisplay.display, batchAtoms, 2,
		    batchAtomNameReturns) != True)
	    || (batchAtomNameReturns[0] == NULL)
	    || (strcmp(batchAtomNameReturns[0], "FONT_NAME") != 0)
	    || (batchAtomNameReturns[1] == NULL)
	    || (strcmp(batchAtomNameReturns[1], batchAtomNames[1]) != 0)) {
	goto cleanup;
    }
    for (atomNameIndex = 0; atomNameIndex < 2; atomNameIndex++) {
	XFree(batchAtomNameReturns[atomNameIndex]);
	batchAtomNameReturns[atomNameIndex] = NULL;
    }
    pixelSizeAtom = XInternAtom(&browserDisplay.display, "PIXEL_SIZE", False);
    queryFontStruct = XLoadQueryFont(&browserDisplay.display,
	    "-browser-Helvetica-medium-r-normal--18-180-96-96-p-90-iso8859-1");
    if ((queryFontStruct == NULL)
	    || !XGetFontProperty(queryFontStruct, pixelSizeAtom,
		    &fontPropertyValue)
	    || (fontPropertyValue != 18UL)
	    || !XGetFontProperty(queryFontStruct, XA_POINT_SIZE,
		    &fontPropertyValue)
	    || (fontPropertyValue != 180UL)
	    || !XGetFontProperty(queryFontStruct, XA_RESOLUTION,
		    &fontPropertyValue)
	    || (fontPropertyValue != 96UL)
	    || !XGetFontProperty(queryFontStruct, XA_UNDERLINE_POSITION,
		    &fontPropertyValue)
	    || (fontPropertyValue != 2UL)
	    || !XGetFontProperty(queryFontStruct, XA_UNDERLINE_THICKNESS,
		    &fontPropertyValue)
	    || (fontPropertyValue != 3UL)
	    || !XGetFontProperty(queryFontStruct, XA_CAP_HEIGHT,
		    &fontPropertyValue)
	    || (fontPropertyValue != 14UL)
	    || XGetFontProperty(queryFontStruct,
		    XInternAtom(&browserDisplay.display,
			    "X11_WASM_UNKNOWN_FONT_PROPERTY", False),
		    &fontPropertyValue)) {
	goto cleanup;
    }
    if (!XGetFontProperty(queryFontStruct, XA_FAMILY_NAME,
	    &fontPropertyValue)) {
	goto cleanup;
    }
    fontAtomName = XGetAtomName(&browserDisplay.display,
	    (Atom)fontPropertyValue);
    if ((fontAtomName == NULL) || (strcmp(fontAtomName, "Helvetica") != 0)) {
	goto cleanup;
    }
    XFree(fontAtomName);
    fontAtomName = NULL;
    latin1TextWidth = XTextWidth(queryFontStruct, latin1Text, 4);
    latin1TextWidth16 = XTextWidth16(queryFontStruct, latin1Text16, 4);
    memset(&latin1TextExtents, 0, sizeof(latin1TextExtents));
    memset(&latin1TextExtents16, 0, sizeof(latin1TextExtents16));
    memset(&fontStructTextExtents, 0, sizeof(fontStructTextExtents));
    textDirection = 0;
    textAscent = 0;
    textDescent = 0;
    if ((latin1TextWidth <= 0)
	    || (latin1TextWidth != latin1TextWidth16)
	    || (XQueryTextExtents(&browserDisplay.display,
		    queryFontStruct->fid, latin1Text, 4, &textDirection,
		    &textAscent, &textDescent, &latin1TextExtents) != Success)
	    || (textDirection != FontLeftToRight)
	    || (latin1TextExtents.width != latin1TextWidth)
	    || (latin1TextExtents.rbearing != latin1TextWidth)
	    || (latin1TextExtents.ascent != textAscent)
	    || (latin1TextExtents.descent != textDescent)) {
	goto cleanup;
    }
    if ((XTextExtents(queryFontStruct, latin1Text, 4, &textDirection,
		    &textAscent, &textDescent, &fontStructTextExtents)
		    != Success)
	    || (textDirection != FontLeftToRight)
	    || (fontStructTextExtents.width != latin1TextWidth)
	    || (fontStructTextExtents.rbearing != latin1TextWidth)
	    || (fontStructTextExtents.ascent != textAscent)
	    || (fontStructTextExtents.descent != textDescent)
	    || (XTextExtents16(queryFontStruct, latin1Text16, 4,
		    &textDirection, &textAscent, &textDescent,
		    &latin1TextExtents16) != Success)
	    || (textDirection != FontLeftToRight)
	    || (latin1TextExtents16.width != latin1TextWidth16)
	    || (latin1TextExtents16.rbearing != latin1TextWidth16)
	    || (latin1TextExtents16.ascent != textAscent)
	    || (latin1TextExtents16.descent != textDescent)
	    || (XQueryTextExtents16(&browserDisplay.display,
		    queryFontStruct->fid, latin1Text16, 4, &textDirection,
		    &textAscent, &textDescent, &latin1TextExtents16)
		    != Success)
	    || (textDirection != FontLeftToRight)
	    || (latin1TextExtents16.width != latin1TextWidth16)
	    || (latin1TextExtents16.rbearing != latin1TextWidth16)
	    || (latin1TextExtents16.ascent != textAscent)
	    || (latin1TextExtents16.descent != textDescent)) {
	goto cleanup;
    }
    queryFontInfo = XQueryFont(&browserDisplay.display, queryFontStruct->fid);
    if ((queryFontInfo == NULL)
	    || (queryFontInfo->fid != queryFontStruct->fid)
	    || (queryFontInfo->ascent != queryFontStruct->ascent)
	    || (queryFontInfo->descent != queryFontStruct->descent)
	    || !XGetFontProperty(queryFontInfo, pixelSizeAtom,
		    &fontPropertyValue)
	    || (fontPropertyValue != 18UL)
	    || !XGetFontProperty(queryFontInfo, XA_POINT_SIZE,
		    &fontPropertyValue)
	    || (fontPropertyValue != 180UL)
	    || (XTextWidth(queryFontInfo, latin1Text, 4) != latin1TextWidth)) {
	goto cleanup;
    }
    memset(longText, 'W', sizeof(longText));
    memset(&textExtents, 0, sizeof(textExtents));
    if ((XQueryTextExtents(&browserDisplay.display, queryFontStruct->fid,
		    longText, (int)sizeof(longText), &textDirection,
		    &textAscent, &textDescent, &textExtents) != Success)
	    || (textExtents.width != SHRT_MAX)
	    || (textExtents.rbearing != SHRT_MAX)
	    || (textExtents.ascent != textAscent)
	    || (textExtents.descent != textDescent)) {
	goto cleanup;
    }
    XFreeFont(&browserDisplay.display, queryFontStruct);
    queryFontStruct = NULL;

    queryFont = XLoadFont(&browserDisplay.display, "Helvetica 18");
    if ((queryFont == None)
	    || (XQueryTextExtents(&browserDisplay.display, queryFont,
		    "extent", 6, &textDirection, &textAscent, &textDescent,
		    &textExtents) != Success)
	    || (textDirection != FontLeftToRight)
	    || (textAscent <= 0) || (textDescent < 0)
	    || (textExtents.width <= 0)
	    || (textExtents.rbearing != textExtents.width)
	    || (textExtents.ascent != textAscent)
	    || (textExtents.descent != textDescent)) {
	goto cleanup;
    }
    queryGC = XCreateGC(&browserDisplay.display, (Drawable)pixmap, 0, NULL);
    if ((queryGC == NULL)
	    || (XSetFont(&browserDisplay.display, queryGC, queryFont)
		    != Success)) {
	goto cleanup;
    }
    if (XDrawString(&browserDisplay.display, (Drawable)pixmap, queryGC, 0, 0,
	    latin1Text, 4) != Success) {
	goto cleanup;
    }
    if ((XDrawImageString(&browserDisplay.display, (Drawable)pixmap, queryGC,
		    0, textAscent, latin1Text, 4) != Success)
	    || (XDrawImageString16(&browserDisplay.display, (Drawable)pixmap,
		    queryGC, 0, textAscent * 2, latin1Text16, 4) != Success)) {
	goto cleanup;
    }
    textItemFont = XLoadFont(&browserDisplay.display, "Helvetica 12");
    if (textItemFont == None) {
	goto cleanup;
    }
    memset(textItems, 0, sizeof(textItems));
    textItems[0].chars = (char *)latin1Text;
    textItems[0].nchars = 2;
    textItems[0].delta = 0;
    textItems[0].font = None;
    textItems[1].chars = NULL;
    textItems[1].nchars = 0;
    textItems[1].delta = 3;
    textItems[1].font = textItemFont;
    textItems[2].chars = (char *)latin1Text + 2;
    textItems[2].nchars = 2;
    textItems[2].delta = 4;
    textItems[2].font = None;
    memset(textItems16, 0, sizeof(textItems16));
    textItems16[0].chars = latin1Text16;
    textItems16[0].nchars = 2;
    textItems16[0].delta = 0;
    textItems16[0].font = textItemFont;
    textItems16[1].chars = latin1Text16 + 2;
    textItems16[1].nchars = 2;
    textItems16[1].delta = 2;
    textItems16[1].font = None;
    if ((XDrawText(&browserDisplay.display, (Drawable)pixmap, queryGC,
		    0, textAscent * 3, textItems, 3) != Success)
	    || (queryGC->font != queryFont)
	    || (XDrawText16(&browserDisplay.display, (Drawable)pixmap, queryGC,
		    0, textAscent * 4, textItems16, 2) != Success)
	    || (queryGC->font != queryFont)) {
	goto cleanup;
    }
    (void)XUnloadFont(&browserDisplay.display, textItemFont);
    textItemFont = None;
    memset(&textExtents, 0, sizeof(textExtents));
    textDirection = 0;
    textAscent = 0;
    textDescent = 0;
    if ((XQueryTextExtents(&browserDisplay.display,
		    (XID)(uintptr_t)queryGC, "gc", 2, &textDirection,
		    &textAscent, &textDescent, &textExtents) != Success)
	    || (textDirection != FontLeftToRight)
	    || (textAscent <= 0) || (textDescent < 0)
	    || (textExtents.width <= 0)
	    || (textExtents.ascent != textAscent)
	    || (textExtents.descent != textDescent)) {
	goto cleanup;
    }
    queryGCFontInfo = XQueryFont(&browserDisplay.display,
	    (XID)(uintptr_t)queryGC);
    if ((queryGCFontInfo == NULL)
	    || (queryGCFontInfo->fid != queryFont)
	    || (queryGCFontInfo->ascent != textAscent)
	    || (queryGCFontInfo->descent != textDescent)
	    || (XTextWidth(queryGCFontInfo, "gc", 2) != textExtents.width)) {
	goto cleanup;
    }
    if (XCopyArea(&browserDisplay.display, (Drawable)pixmap,
	    (Drawable)pixmap, queryGC, 0, 0, 1, 1, 0, 0) != Success) {
	goto cleanup;
    }
    XWindowEvent(&browserDisplay.display, (Window)pixmap, ExposureMask,
	    &exposureEvent);
    if ((exposureEvent.type != NoExpose)
	    || (exposureEvent.xnoexpose.drawable != (Drawable)pixmap)
	    || (exposureEvent.xnoexpose.major_code
		    != X11_WASM_XREQ_COPY_AREA)) {
	goto cleanup;
    }
    memset(&exposureEvent, 0, sizeof(exposureEvent));
    BrowserClearEventQueue();
    if (XCopyArea(&browserDisplay.display, (Drawable)copyExposurePixmap,
	    (Drawable)copyExposurePixmap, queryGC, -1, 1, 3, 2, 0, 0)
	    != Success) {
	goto cleanup;
    }
    XWindowEvent(&browserDisplay.display, (Window)copyExposurePixmap,
	    ExposureMask, &exposureEvent);
    if ((exposureEvent.type != GraphicsExpose)
	    || (exposureEvent.xgraphicsexpose.drawable
		    != (Drawable)copyExposurePixmap)
	    || (exposureEvent.xgraphicsexpose.x != 0)
	    || (exposureEvent.xgraphicsexpose.y != 0)
	    || (exposureEvent.xgraphicsexpose.width != 1)
	    || (exposureEvent.xgraphicsexpose.height != 2)
	    || (exposureEvent.xgraphicsexpose.count != 0)
	    || (exposureEvent.xgraphicsexpose.major_code
		    != X11_WASM_XREQ_COPY_AREA)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    memset(&exposureEvent, 0, sizeof(exposureEvent));
    BrowserClearEventQueue();
    if (XCopyPlane(&browserDisplay.display, (Drawable)pixmap,
	    (Drawable)pixmap, queryGC, 0, 0, 1, 1, 0, 0, 1) != Success) {
	goto cleanup;
    }
    XWindowEvent(&browserDisplay.display, (Window)pixmap, ExposureMask,
	    &exposureEvent);
    if ((exposureEvent.type != NoExpose)
	    || (exposureEvent.xnoexpose.drawable != (Drawable)pixmap)
	    || (exposureEvent.xnoexpose.major_code
		    != X11_WASM_XREQ_COPY_PLANE)) {
	goto cleanup;
    }
    memset(&exposureEvent, 0, sizeof(exposureEvent));
    BrowserClearEventQueue();
    if (XCopyPlane(&browserDisplay.display, (Drawable)copyExposurePixmap,
	    (Drawable)copyExposurePixmap, queryGC, 0, -1, 2, 3, 0, 0, 1)
	    != Success) {
	goto cleanup;
    }
    XWindowEvent(&browserDisplay.display, (Window)copyExposurePixmap,
	    ExposureMask, &exposureEvent);
    if ((exposureEvent.type != GraphicsExpose)
	    || (exposureEvent.xgraphicsexpose.drawable
		    != (Drawable)copyExposurePixmap)
	    || (exposureEvent.xgraphicsexpose.x != 0)
	    || (exposureEvent.xgraphicsexpose.y != 0)
	    || (exposureEvent.xgraphicsexpose.width != 2)
	    || (exposureEvent.xgraphicsexpose.height != 1)
	    || (exposureEvent.xgraphicsexpose.count != 0)
	    || (exposureEvent.xgraphicsexpose.major_code
		    != X11_WASM_XREQ_COPY_PLANE)
	    || (XEventsQueued(&browserDisplay.display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    (void)XFreeGC(&browserDisplay.display, queryGC);
    queryGC = NULL;
    (void)XMapRaised(&browserDisplay.display, window);
    testCursor = XCreateGlyphCursor(&browserDisplay.display, queryFont, None,
	    XC_watch, XC_watch, NULL, NULL);
    if ((testCursor == None)
	    || (XDefineCursor(&browserDisplay.display, window, testCursor)
		    != Success)
	    || (XUndefineCursor(&browserDisplay.display, window) != Success)
	    || (X11WasmFindResource(window)->atts.cursor != None)) {
	goto cleanup;
    }
    (void)XFreeCursor(&browserDisplay.display, testCursor);
    testCursor = None;
    testCursor = XCreateFontCursor(&browserDisplay.display, XC_watch);
    if ((testCursor == None)
	    || (BrowserCursorShape(testCursor) != XC_watch)
	    || (XDefineCursor(&browserDisplay.display, window, testCursor)
		    != Success)
	    || (X11WasmFindResource(window)->atts.cursor != testCursor)) {
	goto cleanup;
    }
    if ((XFreeCursor(&browserDisplay.display, testCursor) != Success)
	    || (X11WasmFindResource(testCursor) == NULL)
	    || (BrowserCursorShape(testCursor) != XC_watch)
	    || (X11WasmFindResource(window)->atts.cursor != testCursor)) {
	goto cleanup;
    }
    if ((XUndefineCursor(&browserDisplay.display, window) != Success)
	    || (X11WasmFindResource(window)->atts.cursor != None)
	    || (X11WasmFindResource(testCursor) != NULL)) {
	goto cleanup;
    }
    testCursor = None;
    cursorWindow = XCreateSimpleWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 1, 1, 0, 0, 0);
    testCursor = XCreateFontCursor(&browserDisplay.display, XC_watch);
    if ((cursorWindow == None) || (testCursor == None)
	    || (XDefineCursor(&browserDisplay.display, cursorWindow,
		    testCursor) != Success)
	    || (X11WasmFindResource(cursorWindow)->atts.cursor != testCursor)) {
	goto cleanup;
    }
    releasedCursor = testCursor;
    if ((XFreeCursor(&browserDisplay.display, releasedCursor) != Success)
	    || (X11WasmFindResource(releasedCursor) == NULL)) {
	goto cleanup;
    }
    testCursor = None;
    destroyedWindow = cursorWindow;
    if (XDestroyWindow(&browserDisplay.display, cursorWindow) != Success) {
	goto cleanup;
    }
    cursorWindow = None;
    if ((X11WasmFindResource(destroyedWindow) != NULL)
	    || (X11WasmFindResource(releasedCursor) != NULL)) {
	goto cleanup;
    }
    destroyedWindow = None;
    releasedCursor = None;
    cursorFont = XLoadFont(&browserDisplay.display, "cursor");
    cursorMaskFont = XLoadFont(&browserDisplay.display, "cursor");
    parsedColor.red = 0xffff;
    parsedColor.green = 0;
    parsedColor.blue = 0;
    exactColor.red = 0;
    exactColor.green = 0;
    exactColor.blue = 0xffff;
    testCursor = XCreateGlyphCursor(&browserDisplay.display, cursorFont,
	    cursorMaskFont, XC_watch, XC_watch + 1U, &parsedColor,
	    &exactColor);
    if ((cursorFont == None) || (cursorMaskFont == None)
	    || (testCursor == None)
	    || (X11WasmFindResource(testCursor) == NULL)
	    || !X11WasmFindResource(testCursor)->hasCursorImage
	    || (BrowserCursorShape(testCursor) != BrowserJsXId(testCursor))) {
	goto cleanup;
    }
    (void)XFreeCursor(&browserDisplay.display, testCursor);
    testCursor = None;
    (void)XUnloadFont(&browserDisplay.display, cursorFont);
    cursorFont = None;
    (void)XUnloadFont(&browserDisplay.display, cursorMaskFont);
    cursorMaskFont = None;
    (void)XUnloadFont(&browserDisplay.display, queryFont);
    queryFont = None;

    result = 60;
    if (!XParseColor(&browserDisplay.display, firstColormap,
	    "rgb:1/22/333", &parsedColor)
	    || (parsedColor.red != 0x1111)
	    || (parsedColor.green != 0x2222)
	    || (parsedColor.blue != 0x3333)
	    || (parsedColor.pixel != 0x112233UL)) {
	goto cleanup;
    }
    if (!XAllocNamedColor(&browserDisplay.display, firstColormap,
	    "rgbi:0.5/1/0", &screenColor, &exactColor)
	    || (exactColor.red != 0x8000)
	    || (exactColor.green != 0xffff)
	    || (exactColor.blue != 0)
	    || (screenColor.pixel != 0x80ff00UL)) {
	goto cleanup;
    }
    if (!XParseColor(&browserDisplay.display, firstColormap,
	    "gray75", &parsedColor)
	    || (parsedColor.red != 0xbfbf)
	    || (parsedColor.green != 0xbfbf)
	    || (parsedColor.blue != 0xbfbf)
	    || (parsedColor.pixel != 0xbfbfbfUL)) {
	goto cleanup;
    }
    if (!XLookupColor(&browserDisplay.display, firstColormap,
	    "Dark Slate Gray", &exactColor, &screenColor)
	    || (exactColor.red != 0x2f2f)
	    || (exactColor.green != 0x4f4f)
	    || (exactColor.blue != 0x4f4f)
	    || (screenColor.pixel != 0x2f4f4fUL)) {
	goto cleanup;
    }
    if (!XLookupColor(&browserDisplay.display, firstColormap,
	    "alice blue", &exactColor, &screenColor)
	    || (exactColor.red != 0xf0f0)
	    || (exactColor.green != 0xf8f8)
	    || (exactColor.blue != 0xffff)
	    || (screenColor.pixel != 0xf0f8ffUL)) {
	goto cleanup;
    }
    if (!XLookupColor(&browserDisplay.display, firstColormap,
	    "DodgerBlue", &exactColor, &screenColor)
	    || (exactColor.red != 0x1e1e)
	    || (exactColor.green != 0x9090)
	    || (exactColor.blue != 0xffff)
	    || (screenColor.pixel != 0x1e90ffUL)) {
	goto cleanup;
    }
    if (!XLookupColor(&browserDisplay.display, firstColormap,
	    "green", &exactColor, &screenColor)
	    || (exactColor.red != 0x0000)
	    || (exactColor.green != 0x8080)
	    || (exactColor.blue != 0x0000)
	    || (screenColor.pixel != 0x008000UL)) {
	goto cleanup;
    }
    if (!XLookupColor(&browserDisplay.display, firstColormap,
	    "lime", &exactColor, &screenColor)
	    || (exactColor.red != 0x0000)
	    || (exactColor.green != 0xffff)
	    || (exactColor.blue != 0x0000)
	    || (screenColor.pixel != 0x00ff00UL)) {
	goto cleanup;
    }
    if ((XParseColor(&browserDisplay.display, firstColormap,
	    "rgb:12345/0/0", &parsedColor) != False)
	    || (XParseColor(&browserDisplay.display, firstColormap,
		    "rgbi:1.1/0/0", &parsedColor) != False)
	    || (XParseColor(&browserDisplay.display, firstColormap,
		    "gray101", &parsedColor) != False)) {
	goto cleanup;
    }

    if (XParseColor(&browserDisplay.display, (Colormap)0x7fffffebUL, "red",
	    NULL) != False) {
	goto cleanup;
    }
    result = 70;
    XSetAuthorization(NULL, 0, NULL, 0);
    if ((XChangeSaveSet(&browserDisplay.display, browserDisplay.rootWindow,
		    SetModeInsert) != Success)
	    || (XAddToSaveSet(&browserDisplay.display,
		    browserDisplay.rootWindow) != Success)
	    || (XRemoveFromSaveSet(&browserDisplay.display,
		    browserDisplay.rootWindow) != Success)
	    || (XSetCloseDownMode(&browserDisplay.display, DestroyAll)
		    != Success)
	    || (XSetCloseDownMode(&browserDisplay.display, RetainPermanent)
		    != Success)
	    || (XSetCloseDownMode(&browserDisplay.display, RetainTemporary)
		    != Success)
	    || (XKillClient(&browserDisplay.display, AllTemporary)
		    != Success)
	    || (XKillClient(&browserDisplay.display,
		    browserDisplay.rootWindow) != Success)
	    || (XActivateScreenSaver(&browserDisplay.display) != Success)
	    || (XResetScreenSaver(&browserDisplay.display) != Success)) {
	goto cleanup;
    }
    result = 71;
    if ((XGrabButton(&browserDisplay.display, Button1, AnyModifier,
		    browserDisplay.rootWindow, False, ButtonPressMask,
		    GrabModeAsync, GrabModeAsync, None, None) != Success)
	    || (XUngrabButton(&browserDisplay.display, AnyButton,
		    AnyModifier, browserDisplay.rootWindow) != Success)
	    || (XGrabKey(&browserDisplay.display, AnyKey, AnyModifier,
		    browserDisplay.rootWindow, False, GrabModeAsync,
		    GrabModeAsync) != Success)
	    || (XUngrabKey(&browserDisplay.display, AnyKey, AnyModifier,
		    browserDisplay.rootWindow) != Success)) {
	goto cleanup;
    }
    result = 72;
    if ((XGrabPointer(&browserDisplay.display, browserDisplay.rootWindow,
		    False, ButtonPressMask, GrabModeSync, GrabModeSync,
		    None, None, CurrentTime) != GrabSuccess)
	    || (XChangeActivePointerGrab(&browserDisplay.display,
		    PointerMotionMask, None, CurrentTime) != Success)
	    || (browserDisplay.pointerGrabEventMask != PointerMotionMask)
	    || (XAllowEvents(&browserDisplay.display, AsyncBoth, CurrentTime)
		    != Success)
	    || (browserDisplay.pointerGrabPointerMode != GrabModeAsync)
	    || (browserDisplay.pointerGrabKeyboardMode != GrabModeAsync)
	    || (XUngrabPointer(&browserDisplay.display, CurrentTime)
		    != Success)) {
	goto cleanup;
    }
    result = 73;
    if ((XGrabKeyboard(&browserDisplay.display, browserDisplay.rootWindow,
		    False, GrabModeSync, GrabModeSync, CurrentTime)
		    != GrabSuccess)
	    || (XAllowEvents(&browserDisplay.display, AsyncBoth, CurrentTime)
		    != Success)
	    || (browserDisplay.keyboardGrabPointerMode != GrabModeAsync)
	    || (browserDisplay.keyboardGrabKeyboardMode != GrabModeAsync)
	    || (XUngrabKeyboard(&browserDisplay.display, CurrentTime)
		    != Success)) {
	goto cleanup;
    }
    result = 74;

    if ((browserDisplay.serverGrabDepth != 0)
	    || (browserDisplay.synchronize != 0)
	    || (browserDisplay.afterFunction != NULL)) {
	goto cleanup;
    }
    if ((XGetScreenSaver(&browserDisplay.display, &screenSaverTimeout,
	    &screenSaverInterval, &screenSaverPreferBlanking,
	    &screenSaverAllowExposures) != Success)
	    || (screenSaverTimeout != 600)
	    || (screenSaverInterval != 600)
	    || (screenSaverPreferBlanking != DefaultBlanking)
	    || (screenSaverAllowExposures != DefaultExposures)) {
	goto cleanup;
    }
    (void)XSetScreenSaver(&browserDisplay.display, 120, 30,
	    PreferBlanking, AllowExposures);
    if ((XGetScreenSaver(&browserDisplay.display, &screenSaverTimeout,
	    &screenSaverInterval, &screenSaverPreferBlanking,
	    &screenSaverAllowExposures) != Success)
	    || (screenSaverTimeout != 120)
	    || (screenSaverInterval != 30)
	    || (screenSaverPreferBlanking != PreferBlanking)
	    || (screenSaverAllowExposures != AllowExposures)) {
	goto cleanup;
    }
    result = 75;
    screenSaverInfo = XScreenSaverAllocInfo();
    if (screenSaverInfo == NULL) {
	goto cleanup;
    }
    result = 751;
    if (XScreenSaverQueryInfo(&browserDisplay.display,
	    browserDisplay.rootWindow, screenSaverInfo) != True) {
	goto cleanup;
    }
    result = 752;
    if (screenSaverInfo->window != browserDisplay.rootWindow) {
	goto cleanup;
    }
    result = 753;
    if (screenSaverInfo->state != ScreenSaverOff) {
	goto cleanup;
    }
    result = 754;
    if (screenSaverInfo->kind != ScreenSaverInternal) {
	goto cleanup;
    }
    result = 755;
    if (screenSaverInfo->til_or_since > 120000UL) {
	goto cleanup;
    }
    result = 76;
    (void)XGrabServer(&browserDisplay.display);
    if (browserDisplay.serverGrabDepth != 1) {
	goto cleanup;
    }
    (void)XGrabServer(&browserDisplay.display);
    if (browserDisplay.serverGrabDepth != 2) {
	goto cleanup;
    }
    (void)XUngrabServer(&browserDisplay.display);
    if (browserDisplay.serverGrabDepth != 1) {
	goto cleanup;
    }
    (void)XUngrabServer(&browserDisplay.display);
    if (browserDisplay.serverGrabDepth != 0) {
	goto cleanup;
    }
    (void)XUngrabServer(&browserDisplay.display);
    if (browserDisplay.serverGrabDepth != 0) {
	goto cleanup;
    }
    result = 77;
    oldFlushSerial = browserDisplay.flushSerial;
    oldSyncSerial = browserDisplay.syncSerial;
    oldAfterFunction = XSynchronize(&browserDisplay.display, True);
    if ((oldAfterFunction != NULL)
	    || (browserDisplay.synchronize != 1)
	    || (browserDisplay.afterFunction != BrowserSynchronizedAfterFunction)
	    || (browserDisplay.flushSerial <= oldFlushSerial)
	    || (browserDisplay.syncSerial <= oldSyncSerial)) {
	goto cleanup;
    }
    oldFlushSerial = browserDisplay.flushSerial;
    oldSyncSerial = browserDisplay.syncSerial;
    oldAfterFunction = XSynchronize(&browserDisplay.display, True);
    if ((oldAfterFunction != BrowserSynchronizedAfterFunction)
	    || (browserDisplay.synchronize != 1)
	    || (browserDisplay.afterFunction != BrowserSynchronizedAfterFunction)
	    || (browserDisplay.flushSerial <= oldFlushSerial)
	    || (browserDisplay.syncSerial <= oldSyncSerial)) {
	goto cleanup;
    }
    oldAfterFunction = XSynchronize(&browserDisplay.display, False);
    if ((oldAfterFunction != BrowserSynchronizedAfterFunction)
	    || (browserDisplay.synchronize != 0)
	    || (browserDisplay.afterFunction != NULL)
	    || (XSynchronize(&browserDisplay.display, False) != NULL)) {
	goto cleanup;
    }
    browserTestAfterFunctionCount = 0;
    oldAfterFunction = XSetAfterFunction(&browserDisplay.display,
	    BrowserTestAfterFunction);
    if ((oldAfterFunction != NULL) || (browserDisplay.synchronize != 0)
	    || (browserDisplay.afterFunction != BrowserTestAfterFunction)
	    || (XNoOp(&browserDisplay.display) != Success)
	    || (browserTestAfterFunctionCount != 1)) {
	goto cleanup;
    }
    oldAfterFunction = XSetAfterFunction(&browserDisplay.display, NULL);
    if ((oldAfterFunction != BrowserTestAfterFunction)
	    || (browserDisplay.synchronize != 0)
	    || (browserDisplay.afterFunction != NULL)
	    || (browserTestAfterFunctionCount != 1)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (modifierBusyKeyPressed) {
	(void)BrowserQueueKeyEventFields(KeyRelease, browserDisplay.rootWindow,
		(KeyCode)X11_WASM_KEYCODE_EQUAL, XK_equal, 0, 0, 0,
		CurrentTime, 0);
	BrowserClearEventQueue();
    }
    (void)XDeleteContext(&browserDisplay.display, contextRidOne, contextOne);
    (void)XDeleteContext(&browserDisplay.display, contextRidOne, contextTwo);
    (void)XDeleteContext(&browserDisplay.display, contextRidTwo, contextOne);
    if (extensionList != NULL) {
	XFreeExtensionList(extensionList);
    }
    if (screenSaverInfo != NULL) {
	XFree(screenSaverInfo);
    }
    if (stringList != NULL) {
	XFreeStringList(stringList);
    }
    if (listedFontNames != NULL) {
	XFreeFontNames(listedFontNames);
    }
    if (hostList != NULL) {
	XFree(hostList);
    }
    if (wideList != NULL) {
	XwcFreeStringList(wideList);
    }
    if (missingCharsets != NULL) {
	XFreeStringList(missingCharsets);
    }
    if (fontSet != NULL) {
	XFreeFontSet(&browserDisplay.display, fontSet);
    }
    if (listProp.value != NULL) {
	XFree(listProp.value);
    }
    if (titleProp.value != NULL) {
	XFree(titleProp.value);
    }
    if (iconProp.value != NULL) {
	XFree(iconProp.value);
    }
    if (machineProp.value != NULL) {
	XFree(machineProp.value);
    }
    if (customProp.value != NULL) {
	XFree(customProp.value);
    }
    if (mbProp.value != NULL) {
	XFree(mbProp.value);
    }
    if (utf8Prop.value != NULL) {
	XFree(utf8Prop.value);
    }
    if (stdProp.value != NULL) {
	XFree(stdProp.value);
    }
    if (wideProp.value != NULL) {
	XFree(wideProp.value);
    }
    if (readProp.value != NULL) {
	XFree(readProp.value);
    }
    if (propertyAtoms != NULL) {
	XFree(propertyAtoms);
    }
    if (rotatePropertyData != NULL) {
	XFree(rotatePropertyData);
    }
    if (fetchedCutBuffer != NULL) {
	XFree(fetchedCutBuffer);
    }
    if (fetchedName != NULL) {
	XFree(fetchedName);
    }
    if (fetchedIconName != NULL) {
	XFree(fetchedIconName);
    }
    if (fontAtomName != NULL) {
	XFree(fontAtomName);
    }
    for (atomNameIndex = 0; atomNameIndex < 3; atomNameIndex++) {
	if (batchAtomNameReturns[atomNameIndex] != NULL) {
	    XFree(batchAtomNameReturns[atomNameIndex]);
	}
    }
    if (fontLongPerCharLogical != NULL) {
	ckfree(fontLongPerCharLogical);
    }
    if (queryFontStruct != NULL) {
	XFreeFont(&browserDisplay.display, queryFontStruct);
    }
    if (queryFontInfo != NULL) {
	XFreeFont(&browserDisplay.display, queryFontInfo);
    }
    if (queryGCFontInfo != NULL) {
	XFreeFont(&browserDisplay.display, queryGCFontInfo);
    }
    if (keyboardMapping != NULL) {
	XFree(keyboardMapping);
    }
    if (modifierMap != NULL) {
	XFreeModifiermap(modifierMap);
    }
    if (testCursor != None) {
	(void)XFreeCursor(&browserDisplay.display, testCursor);
    }
    if (inputOnlyGC != NULL) {
	(void)XFreeGC(&browserDisplay.display, inputOnlyGC);
    }
    if (queryGC != NULL) {
	(void)XFreeGC(&browserDisplay.display, queryGC);
    }
    if (queryFont != None) {
	(void)XUnloadFont(&browserDisplay.display, queryFont);
    }
    if (textItemFont != None) {
	(void)XUnloadFont(&browserDisplay.display, textItemFont);
    }
    if (cursorFont != None) {
	(void)XUnloadFont(&browserDisplay.display, cursorFont);
    }
    if (cursorMaskFont != None) {
	(void)XUnloadFont(&browserDisplay.display, cursorMaskFont);
    }
    if (classHintReturn.res_name != NULL) {
	XFree(classHintReturn.res_name);
    }
    if (classHintReturn.res_class != NULL) {
	XFree(classHintReturn.res_class);
    }
    if (commandArgvReturn != NULL) {
	XFreeStringList(commandArgvReturn);
    }
    if (wmHintsReturn != NULL) {
	XFree(wmHintsReturn);
    }
    if (iconSizesReturn != NULL) {
	XFree(iconSizesReturn);
    }
    if (allocatedIconSize != NULL) {
	XFree(allocatedIconSize);
    }
    if (allocatedStandardColormap != NULL) {
	XFree(allocatedStandardColormap);
    }
    if (standardColormapsReturn != NULL) {
	XFree(standardColormapsReturn);
    }
    if (pixmapFormats != NULL) {
	XFree(pixmapFormats);
    }
    if (depthList != NULL) {
	XFree(depthList);
    }
    if (visualInfoList != NULL) {
	XFree(visualInfoList);
    }
    if (colormapWindowsReturn != NULL) {
	XFree(colormapWindowsReturn);
    }
    if (protocolAtomsReturn != NULL) {
	XFree(protocolAtomsReturn);
    }
    if (transientForData != NULL) {
	XFree(transientForData);
    }
    if (reparentChildrenReturn != NULL) {
	XFree(reparentChildrenReturn);
    }
    if (image16 != NULL) {
	(void)XDestroyImage(image16);
    }
    if (invalidFormatImage != NULL) {
	(void)XDestroyImage(invalidFormatImage);
    }
    if (invalidDepthImage != NULL) {
	(void)XDestroyImage(invalidDepthImage);
    }
    if (invalidXYBitmapDepthImage != NULL) {
	(void)XDestroyImage(invalidXYBitmapDepthImage);
    }
    if (invalidOffsetImage != NULL) {
	(void)XDestroyImage(invalidOffsetImage);
    }
    if (invalidBitmapPadImage != NULL) {
	(void)XDestroyImage(invalidBitmapPadImage);
    }
    if (shortStrideImage != NULL) {
	(void)XDestroyImage(shortStrideImage);
    }
    if (bitmapImage != NULL) {
	(void)XDestroyImage(bitmapImage);
    }
    if (subImage16 != NULL) {
	(void)XDestroyImage(subImage16);
    }
    if (subBitmapImage != NULL) {
	(void)XDestroyImage(subBitmapImage);
    }
    if (xyBitmapReadback != NULL) {
	(void)XDestroyImage(xyBitmapReadback);
    }
    if (xyBitmapSubImage != NULL) {
	(void)XDestroyImage(xyBitmapSubImage);
    }
    if (bitmapDataReadback != NULL) {
	(void)XDestroyImage(bitmapDataReadback);
    }
    if (xyBitmapImage != NULL) {
	(void)XDestroyImage(xyBitmapImage);
    }
    if (textReadback != NULL) {
	(void)XDestroyImage(textReadback);
    }
    if (xyBitmapGC != NULL) {
	(void)XFreeGC(&browserDisplay.display, xyBitmapGC);
    }
    if (textGC != NULL) {
	(void)XFreeGC(&browserDisplay.display, textGC);
    }
    if (xyBitmapPixmap != None) {
	(void)XFreePixmap(&browserDisplay.display, xyBitmapPixmap);
    }
    if (bitmapDataPixmap != None) {
	(void)XFreePixmap(&browserDisplay.display, bitmapDataPixmap);
    }
    if (textPixmap != None) {
	(void)XFreePixmap(&browserDisplay.display, textPixmap);
    }
    if (regionA != NULL) {
	XDestroyRegion(regionA);
    }
    if (regionB != NULL) {
	XDestroyRegion(regionB);
    }
    if (regionC != NULL) {
	XDestroyRegion(regionC);
    }
    if (polygonRegion != NULL) {
	XDestroyRegion(polygonRegion);
    }
    if (preeditAttributes != NULL) {
	XFree(preeditAttributes);
    }
    if (statusAttributes != NULL) {
	XFree(statusAttributes);
    }
    if (inputContext != NULL) {
	XDestroyIC(inputContext);
    }
    if (inputMethod != NULL) {
	(void)XCloseIM(inputMethod);
    }
    ioErrorHandler = savedIOErrorHandler;
    if (localeModifiers != NULL) {
	ckfree(localeModifiers);
    }
    localeModifiers = savedLocaleModifiers;
    browserDisplay.serverGrabDepth = 0;
    BrowserSyncServerGrabState();
    browserDisplay.synchronize = 0;
    browserDisplay.afterFunction = NULL;
    browserDisplay.runningAfterFunction = 0;
    BrowserSyncSynchronizeState(&browserDisplay.display);
    browserDisplay.screenSaverTimeout = 600;
    browserDisplay.screenSaverInterval = 600;
    browserDisplay.screenSaverPreferBlanking = DefaultBlanking;
    browserDisplay.screenSaverAllowExposures = DefaultExposures;
    (void)X11WasmJsSetScreenSaverSettings(600, 600, DefaultBlanking,
	    DefaultExposures);
    BrowserResetHostAccessControl();
    if (copyExposurePixmap != None) {
	(void)XFreePixmap(&browserDisplay.display, copyExposurePixmap);
    }
    if (widePixmap != None) {
	(void)XFreePixmap(&browserDisplay.display, widePixmap);
    }
    if (pixmap != None) {
	(void)XFreePixmap(&browserDisplay.display, pixmap);
    }
    if (window != None) {
	(void)XDestroyWindow(&browserDisplay.display, window);
    }
    if (notifyWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, notifyWindow);
    }
    if (destroyGrandchildWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, destroyGrandchildWindow);
    }
    if (destroyChildWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, destroyChildWindow);
    }
    if (destroyParentWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, destroyParentWindow);
    }
    if (simpleWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, simpleWindow);
    }
    if (inputOnlyWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, inputOnlyWindow);
    }
    if (cursorWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, cursorWindow);
    }
    if (invalidInputOnlyWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, invalidInputOnlyWindow);
    }
    if (gravitySouthEastWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, gravitySouthEastWindow);
    }
    if (gravityCenterWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, gravityCenterWindow);
    }
    if (gravityUnmapWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, gravityUnmapWindow);
    }
    if (gravityParentWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, gravityParentWindow);
    }
    if (mapSubTopWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, mapSubTopWindow);
    }
    if (mapSubMiddleWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, mapSubMiddleWindow);
    }
    if (mapSubBottomWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, mapSubBottomWindow);
    }
    if (mapSubParentWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, mapSubParentWindow);
    }
    if (reparentChildWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, reparentChildWindow);
    }
    if (reparentNewParentWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, reparentNewParentWindow);
    }
    if (reparentOldParentWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, reparentOldParentWindow);
    }
    if (firstColormap != None) {
	(void)XFreeColormap(&browserDisplay.display, firstColormap);
    }
    if (secondColormap != None) {
	(void)XFreeColormap(&browserDisplay.display, secondColormap);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestColormapApis(void)
{
    Display *display;
    int screen;
    int result = 0;
    XSetWindowAttributes attributes;
    Window window = None;
    Colormap colormap = None;
    Colormap copiedColormap = None;
    Colormap *installedColormaps = NULL;
    int installedColormapCount = -1;
    XColor storedColor;
    XColor storedColors[2];
    unsigned long allocatedPixels[2];
    unsigned long extraAllocatedPixel;
    unsigned long allocatedPlaneMask;
    unsigned long planeFreePixel;
    unsigned long planeMasks[2];
    unsigned long colorPlanePixels[2];
    unsigned long colorPlaneMask;
    unsigned long colorPlaneCombination;
    unsigned long redMaskReturn = 0;
    unsigned long greenMaskReturn = 0;
    unsigned long blueMaskReturn = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    screen = DefaultScreen(display);
    memset(&attributes, 0, sizeof(attributes));
    memset(&storedColor, 0, sizeof(storedColor));
    memset(storedColors, 0, sizeof(storedColors));
    memset(allocatedPixels, 0, sizeof(allocatedPixels));
    extraAllocatedPixel = 0;
    allocatedPlaneMask = 0;
    memset(planeMasks, 0, sizeof(planeMasks));
    memset(colorPlanePixels, 0, sizeof(colorPlanePixels));
    colorPlaneMask = 0;
    colorPlaneCombination = 0;

    colormap = XCreateColormap(display, browserDisplay.rootWindow,
	    DefaultVisual(display, screen), AllocNone);
    if (colormap == None) {
	goto cleanup;
    }
    attributes.colormap = colormap;
    window = XCreateWindow(display, browserDisplay.rootWindow, 0, 0, 1, 1, 0,
	    CopyFromParent, InputOutput, DefaultVisual(display, screen),
	    CWColormap, &attributes);
    if (window == None) {
	goto cleanup;
    }

    storedColor.pixel = 0x010203UL;
    storedColor.red = 0x1111;
    storedColor.green = 0x2222;
    storedColor.blue = 0x3333;
    storedColor.flags = DoRed | DoGreen | DoBlue;
    if (XStoreColor(display, colormap, &storedColor) != Success) {
	goto cleanup;
    }
    storedColor.pixel = 0x010203UL;
    if ((XQueryColor(display, colormap, &storedColor) != Success)
	    || (storedColor.red != 0x1111)
	    || (storedColor.green != 0x2222)
	    || (storedColor.blue != 0x3333)
	    || (storedColor.flags != (DoRed | DoGreen | DoBlue))) {
	goto cleanup;
    }

    storedColors[0].pixel = 0x010204UL;
    storedColors[0].red = 0xaaaa;
    storedColors[0].green = 0xbbbb;
    storedColors[0].blue = 0xcccc;
    storedColors[0].flags = DoRed | DoGreen | DoBlue;
    storedColors[1].pixel = 0x010205UL;
    storedColors[1].red = 0xdddd;
    storedColors[1].green = 0xeeee;
    storedColors[1].blue = 0xffff;
    storedColors[1].flags = DoRed | DoGreen | DoBlue;
    if ((XStoreColors(display, colormap, storedColors, 2) != Success)
	    || (XQueryColors(display, colormap, storedColors, 2) != Success)
	    || (storedColors[0].red != 0xaaaa)
	    || (storedColors[0].green != 0xbbbb)
	    || (storedColors[0].blue != 0xcccc)
	    || (storedColors[1].red != 0xdddd)
	    || (storedColors[1].green != 0xeeee)
	    || (storedColors[1].blue != 0xffff)) {
	goto cleanup;
    }

    if (XStoreNamedColor(display, colormap, "red", 0x010206UL,
	    DoRed | DoGreen | DoBlue) != Success) {
	goto cleanup;
    }
    storedColor.pixel = 0x010206UL;
    if ((XQueryColor(display, colormap, &storedColor) != Success)
	    || (storedColor.red != 0xffff)
	    || (storedColor.green != 0)
	    || (storedColor.blue != 0)) {
	goto cleanup;
    }

    if (!XAllocColorCells(display, colormap, False, planeMasks, 2,
	    allocatedPixels, 2)
	    || (allocatedPixels[0] == allocatedPixels[1])
	    || (planeMasks[0] != 1UL)
	    || (planeMasks[1] != 2UL)) {
	goto cleanup;
    }
    allocatedPlaneMask = planeMasks[0] | planeMasks[1];
    if (((allocatedPixels[0] & allocatedPlaneMask) != 0)
	    || ((allocatedPixels[1] & allocatedPlaneMask) != 0)
	    || !XAllocColorCells(display, colormap, False, NULL, 0,
		    &extraAllocatedPixel, 1)
	    || (extraAllocatedPixel <= (allocatedPixels[1]
		    | allocatedPlaneMask))) {
	goto cleanup;
    }
    storedColor.pixel = allocatedPixels[0];
    if ((XQueryColor(display, colormap, &storedColor) != Success)
	    || (storedColor.red != 0)
	    || (storedColor.green != 0)
	    || (storedColor.blue != 0)) {
	goto cleanup;
    }

    if (!XAllocColorPlanes(display, colormap, False, colorPlanePixels, 2, 1,
	    1, 1, &redMaskReturn, &greenMaskReturn, &blueMaskReturn)
	    || (colorPlanePixels[0] == colorPlanePixels[1])
	    || (redMaskReturn != 0x00010000UL)
	    || (greenMaskReturn != 0x00000100UL)
	    || (blueMaskReturn != 0x00000001UL)) {
	goto cleanup;
    }
    colorPlaneMask = redMaskReturn | greenMaskReturn | blueMaskReturn;
    colorPlaneCombination = colorPlanePixels[0] | colorPlaneMask;
    storedColor.pixel = colorPlaneCombination;
    if (((colorPlanePixels[0] & colorPlaneMask) != 0)
	    || ((colorPlanePixels[1] & colorPlaneMask) != 0)
	    || (XQueryColor(display, colormap, &storedColor) != Success)
	    || (storedColor.red != 0)
	    || (storedColor.green != 0)
	    || (storedColor.blue != 0)) {
	goto cleanup;
    }

    storedColors[0].pixel = 0x010220UL;
    storedColors[0].red = 0x1111;
    storedColors[0].green = 0x2222;
    storedColors[0].blue = 0x3333;
    storedColors[0].flags = DoRed | DoGreen | DoBlue;
    storedColors[1].pixel = 0x010221UL;
    storedColors[1].red = 0x4444;
    storedColors[1].green = 0x5555;
    storedColors[1].blue = 0x6666;
    storedColors[1].flags = DoRed | DoGreen | DoBlue;
    planeFreePixel = 0x010220UL;
    storedColor.pixel = 0x010221UL;
    if ((XStoreColors(display, colormap, storedColors, 2) != Success)
	    || (XFreeColors(display, colormap, &planeFreePixel, 1, 0x1UL)
		    != Success)
	    || (XQueryColor(display, colormap, &storedColor) != Success)
	    || (storedColor.red != 0x0101)
	    || (storedColor.green != 0x0202)
	    || (storedColor.blue != 0x2121)) {
	goto cleanup;
    }

    copiedColormap = XCopyColormapAndFree(display, colormap);
    storedColor.pixel = 0x010206UL;
    if ((copiedColormap == None)
	    || (XQueryColor(display, copiedColormap, &storedColor) != Success)
	    || (storedColor.red != 0xffff)
	    || (storedColor.green != 0)
	    || (storedColor.blue != 0)) {
	goto cleanup;
    }

    if ((XInstallColormap(display, colormap) != Success)
	    || ((installedColormaps = XListInstalledColormaps(display, window,
		    &installedColormapCount)) == NULL)
	    || (installedColormapCount != 1)
	    || (installedColormaps[0] != colormap)) {
	goto cleanup;
    }
    XFree(installedColormaps);
    installedColormaps = NULL;
    installedColormapCount = -1;

    if ((XUninstallColormap(display, colormap) != Success)
	    || ((installedColormaps = XListInstalledColormaps(display, window,
		    &installedColormapCount)) == NULL)
	    || (installedColormapCount != 1)
	    || (installedColormaps[0] != browserDisplay.defaultColormap)) {
	goto cleanup;
    }

    result = 1;

cleanup:
    if (installedColormaps != NULL) {
	XFree(installedColormaps);
    }
    if (copiedColormap != None) {
	(void)XFreeColormap(display, copiedColormap);
    }
    if (window != None) {
	(void)XDestroyWindow(display, window);
    }
    if (colormap != None) {
	(void)XFreeColormap(display, colormap);
    }
    BrowserClearEventQueue();
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestResourceManagerApis(void)
{
    Display *display;
    const char resourceText[] =
	    "! ignored comment\n"
	    "*geometry: global-geometry\n"
	    "wish*geometry: wildcard-geometry\n"
	    "wish.geometry: exact-geometry\n"
	    "wish.background: black\n"
	    "*foreground: white\n"
	    "standalone: bare-value\n"
	    "wish.empty:\n";
    const char replacementText[] =
	    "wish.geometry: first\n"
	    "wish.geometry: second\n";
    const char screenResourceText[] = "screen.option: screen-value\n";
    Atom screenResourcesAtom;
    const char *resourceManagerString;
    char *screenResourceString = NULL;
    char *defaultValue;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    screenResourcesAtom = XInternAtom(display, "SCREEN_RESOURCES", False);
    (void)XDeleteProperty(display, browserDisplay.rootWindow,
	    XA_RESOURCE_MANAGER);
    (void)XDeleteProperty(display, browserDisplay.rootWindow,
	    screenResourcesAtom);
    if ((XResourceManagerString(display) != NULL)
	    || (XScreenResourceString(DefaultScreenOfDisplay(display)) != NULL)
	    || (XGetDefault(display, "wish", "geometry") != NULL)) {
	return 0;
    }

    if (XChangeProperty(display, browserDisplay.rootWindow,
	    XA_RESOURCE_MANAGER, XA_STRING, 8, PropModeReplace,
	    (const unsigned char *)resourceText,
	    (int)strlen(resourceText)) != Success) {
	return 0;
    }
    resourceManagerString = XResourceManagerString(display);
    if ((resourceManagerString == NULL)
	    || (strcmp(resourceManagerString, resourceText) != 0)) {
	return 0;
    }
    defaultValue = XGetDefault(display, "wish", "geometry");
    if ((defaultValue == NULL)
	    || (strcmp(defaultValue, "exact-geometry") != 0)) {
	return 0;
    }
    defaultValue = XGetDefault(display, "/usr/bin/wish", "geometry");
    if ((defaultValue == NULL)
	    || (strcmp(defaultValue, "exact-geometry") != 0)) {
	return 0;
    }
    defaultValue = XGetDefault(display, "wish", "background");
    if ((defaultValue == NULL) || (strcmp(defaultValue, "black") != 0)) {
	return 0;
    }
    defaultValue = XGetDefault(display, "other", "foreground");
    if ((defaultValue == NULL) || (strcmp(defaultValue, "white") != 0)) {
	return 0;
    }
    defaultValue = XGetDefault(display, "other", "standalone");
    if ((defaultValue == NULL) || (strcmp(defaultValue, "bare-value") != 0)) {
	return 0;
    }
    defaultValue = XGetDefault(display, "wish", "empty");
    if ((defaultValue == NULL) || (strcmp(defaultValue, "") != 0)) {
	return 0;
    }
    if ((XGetDefault(display, "other", "background") != NULL)
	    || (XGetDefault(display, NULL, NULL) != NULL)) {
	return 0;
    }
    if (XChangeProperty(display, browserDisplay.rootWindow,
	    screenResourcesAtom, XA_STRING, 8, PropModeReplace,
	    (const unsigned char *)screenResourceText,
	    (int)strlen(screenResourceText)) != Success) {
	return 0;
    }
    screenResourceString = XScreenResourceString(DefaultScreenOfDisplay(display));
    if ((screenResourceString == NULL)
	    || (strcmp(screenResourceString, screenResourceText) != 0)) {
	return 0;
    }
    XFree(screenResourceString);
    screenResourceString = NULL;

    if (XChangeProperty(display, browserDisplay.rootWindow,
	    XA_RESOURCE_MANAGER, XA_STRING, 8, PropModeReplace,
	    (const unsigned char *)replacementText,
	    (int)strlen(replacementText)) != Success) {
	return 0;
    }
    defaultValue = XGetDefault(display, "wish", "geometry");
    if ((defaultValue == NULL) || (strcmp(defaultValue, "second") != 0)) {
	return 0;
    }

    (void)XDeleteProperty(display, browserDisplay.rootWindow,
	    XA_RESOURCE_MANAGER);
    (void)XDeleteProperty(display, browserDisplay.rootWindow,
	    screenResourcesAtom);
    if ((XResourceManagerString(display) != NULL)
	    || (XScreenResourceString(DefaultScreenOfDisplay(display)) != NULL)
	    || (XGetDefault(display, "wish", "geometry") != NULL)) {
	return 0;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestXErrorHandler(void)
{
    XErrorHandler oldHandler;
    Window invalidWindow = (Window)0x7ffffffeUL;
    unsigned long initialSerial;
    int result;
    int getResult;
    int attributesResult;
    int geometryResult;
    int translateSourceResult;
    int translateDestinationResult;
    int warpPointerSizeResult;
    int queryPointerResult;
    int clearAreaExposuresResult;
    int clearAreaSizeResult;
    int invalidSendEventTypeResult;
    int sendEventPropagateResult;
    int sendEventResult;
    Atom invalidInternAtomResult;
    int selectionValidationResultSum;
    int freePixmapResult;
    int putImageResult;
    int copyAreaSourceResult;
    int copyAreaTargetResult;
    int copyAreaSizeResult;
    int copyPlaneSourceResult;
    int copyPlaneTargetResult;
    int copyPlaneSizeResult;
    int copyAreaDepthMismatchResult;
    int drawResultSum;
    int drawValueValidationResultSum;
    int gcResultSum;
    int getGCValuesInvalidResult;
    int freeTemporaryGCResult;
    int cursorResultSum;
    int gcReferenceResultSum;
    int invalidDrawGCResultSum;
    int invalidFreeColormapResult;
    int invalidAllocColorResult;
    int invalidAllocNamedColorResult;
    int invalidLookupColorResult;
    int invalidQueryColorResult;
    int invalidQueryColorsResultSum;
    int invalidFreeColorsResult;
    int invalidFreeColorsListResultSum;
    int invalidWindowAttributeResultSum;
    int sawSetRootParentRelativeBadMatch;
    int sawChangeRootParentRelativeBadMatch;
    int configureValidationResultSum;
    int reconfigureValidResult;
    int reconfigureInvalidScreenResult;
    int iconifyValidResult;
    int iconifyInvalidScreenResult;
    int withdrawValidResult;
    int withdrawInvalidScreenResult;
    int focusValidationResultSum;
    int grabKeyboardModeResult;
    int grabKeyboardOwnerEventsResult;
    int grabKeyboardWindowResult;
    int grabPointerModeResult;
    int grabPointerOwnerEventsResult;
    int grabPointerEventMaskResult;
    int grabPointerWindowResult;
    int grabPointerConfineResult;
    int grabPointerCursorResult;
    int passiveGrabValidationResultSum;
    int sawGrabButtonBadValue;
    int sawGrabButtonBadWindow;
    int sawGrabKeyBadValue;
    int sawUngrabKeyBadWindow;
    int sawAllowEventsBadValue;
    int sawChangeActivePointerGrabBadValue;
    int sawChangeActivePointerGrabBadCursor;
    int invalidPointerMapLengthResult;
    int duplicatePointerMapResult;
    int sawPointerMapLengthBadValue;
    int sawPointerMapDuplicateBadValue;
    XModifierKeymap *duplicateModifierMap = NULL;
    int duplicateModifierMapResult;
    int sawModifierMapDuplicateBadValue;
    int bellValidationResult;
    int propertyValidationResultSum;
    int invalidWMColormapCountResult;
    int invalidWMColormapListResult;
    int changePixmapPropertyResult;
    int getPixmapPropertyResult;
    int deletePixmapPropertyResult;
    int queryPixmapResult;
    int getInvalidAtomResult;
    int getNegativeOffsetResult;
    int getNegativeLengthResult;
    int getLargeOffsetResult;
    int getDeletePropertyResult;
    int rotateWindowValidationResultSum;
    int sawRotateWindowBadMatch;
    int sawRotateWindowDuplicateBadMatch;
    int storeBufferValidationResult;
    int sawStoreBufferBadValue;
    int invalidFetchBufferLength = -1;
    char *invalidFetchBuffer = NULL;
    int sawFetchBufferBadValue;
    int invalidAtomNamesResult;
    int sawGeometryBadDrawable;
    int sawWarpPointerBadValue;
    int warpPointerZeroWidthX;
    int warpPointerZeroWidthY;
    int warpPointerZeroHeightX;
    int warpPointerZeroHeightY;
    int sawClearAreaBadValue;
    int sawConfigureBadMatch;
    int sawReconfigureBadValue;
    int sawIconifyBadValue;
    int sawWithdrawBadValue;
    Window focusBeforeValidation = None;
    Window focusAfterValidation = None;
    int revertBeforeValidation = 0;
    int revertAfterValidation = 0;
    Window invalidCreateWindowResult;
    Pixmap createPixmapResult;
    Pixmap invalidCreatePixmapWidthResult;
    Pixmap invalidCreatePixmapHeightResult;
    Pixmap invalidCreatePixmapSizeResult;
    Pixmap invalidCreatePixmapDepthResult;
    Pixmap colorPixmap;
    Pixmap depthOnePixmap;
    Pixmap propertyPixmap;
    Colormap validColormap;
    Colormap invalidCreateColormapResult;
    Colormap invalidVisualColormapResult;
    GC createGCResult;
    GC createInvalidValueGCResult;
    GC createInvalidMaskGCResult;
    GC defaultGC;
    GC temporaryGC;
    GC referenceGC;
    GC depthOneGC;
    GC invalidGC;
    GContext defaultGContext;
    GContext nextTemporaryGContext;
    GContext temporaryGContext;
    Font validFont;
    Cursor validCursor;
    Cursor invalidPixmapCursorResult;
    Cursor badMatchCursorResult;
    Cursor invalidGlyphFontCursorResult;
    Cursor invalidGlyphValueCursorResult;
    XImage *getImageResult;
    XImage *invalidFormatGetImageResult;
    XImage *xyBitmapGetImageResult;
    XImage *oversizeGetImageResult;
    XImage *outOfBoundsGetImageResult;
    XImage *unviewableGetImageResult = NULL;
    XImage invalidFormatPutImage;
    XImage depthMismatchPutImage;
    int invalidFormatPutImageResult;
    int depthMismatchPutImageResult;
    unsigned char *propertyData = NULL;
    unsigned char *largeLengthPropertyData = NULL;
    Window invalidSelectionOwnerResult;
    Atom actualType = None;
    Atom largeLengthActualType = None;
    int actualFormat = -1;
    int largeLengthActualFormat = -1;
    unsigned long itemCount = 1;
    unsigned long largeLengthItemCount = 0;
    unsigned long bytesAfter = 1;
    unsigned long largeLengthBytesAfter = 1;
    char *invalidAtomName = NULL;
    char *invalidAtomNames[2];
    Window root = None;
    Window child = None;
    Window configureWindow = None;
    Window inputOnlyParentWindow = None;
    int x = -1;
    int y = -1;
    unsigned int width = 1;
    unsigned int height = 1;
    unsigned int borderWidth = 1;
    unsigned int depth = 1;
    const unsigned char value[] = "x";
    XWindowChanges changes;
    XSetWindowAttributes attributes;
    XWindowAttributes windowAttributes;
    XGCValues gcValues;
    XColor color;
    XColor exactColor;
    XColor screenColor;
    XIconSize oversizedIconSize;
    XStandardColormap oversizedStandardColormap;
    XRectangle clipRect;
    Visual mismatchedVisual;
    XEvent event;
    XPoint points[3];
    XSegment segment;
    XChar2b text16[1];
    XTextItem textItems[1];
    XTextItem16 textItems16[1];
    const char zeroDash[] = {'\0'};
    const unsigned char pointerMap[5] = {
	Button1, Button2, Button3, Button4, Button5
    };
    const unsigned char duplicatePointerMap[5] = {
	Button1, Button2, Button2, Button4, Button5
    };
    Atom validProperty;
    Atom validSelection;
    Atom invalidAtom = (Atom)0x7fffffedUL;
    Atom invalidAtomList[2];
    Atom rotateErrorAtoms[2];
    Atom atomPropertyValue = XA_WM_NAME;
    unsigned long colorPixel = 0;
    unsigned long invalidGCMask = 1UL << 30;
    unsigned long invalidWindowAttributeMask = 1UL << 30;
    unsigned int expectedRequestCodeSum;
    unsigned long long expectedRequestCodeMask;
    Window invalidCreateSizeResult;
    Window invalidCreateLargeWindowResult;
    Window invalidCreateSimpleWindowResult;
    Window invalidCreateDepthWindowResult;
    Window invalidCreateVisualWindowResult;
    Window invalidCreateParentClassWindowResult;
    Window invalidCreateAttributeMaskWindowResult;
    int largeLengthGetResult;
    int largeLengthGetOk = 0;

    BrowserInitDisplay();
    memset(&changes, 0, sizeof(changes));
    memset(&attributes, 0, sizeof(attributes));
    memset(&gcValues, 0, sizeof(gcValues));
    memset(&color, 0, sizeof(color));
    memset(&exactColor, 0, sizeof(exactColor));
    memset(&screenColor, 0, sizeof(screenColor));
    memset(&oversizedIconSize, 0, sizeof(oversizedIconSize));
    memset(&oversizedStandardColormap, 0, sizeof(oversizedStandardColormap));
    memset(&clipRect, 0, sizeof(clipRect));
    memset(&event, 0, sizeof(event));
    memset(&invalidFormatPutImage, 0, sizeof(invalidFormatPutImage));
    memset(&depthMismatchPutImage, 0, sizeof(depthMismatchPutImage));
    memset(points, 0, sizeof(points));
    memset(&segment, 0, sizeof(segment));
    memset(text16, 0, sizeof(text16));
    memset(invalidAtomNames, 0, sizeof(invalidAtomNames));
    invalidAtomList[0] = XA_WM_NAME;
    invalidAtomList[1] = invalidAtom;
    event.type = KeyPress;
    invalidFormatPutImage.format = 99;
    depthMismatchPutImage.format = ZPixmap;
    depthMismatchPutImage.depth = DefaultDepth(&browserDisplay.display,
	    DefaultScreen(&browserDisplay.display));
    points[1].x = 1;
    points[2].y = 1;
    segment.x2 = 1;
    text16[0].byte2 = 'x';
    gcValues.foreground = 1;
    clipRect.width = 1;
    clipRect.height = 1;
    defaultGC = DefaultGC(&browserDisplay.display,
	    DefaultScreen(&browserDisplay.display));
    defaultGContext = XGContextFromGC(defaultGC);
    invalidGC = (GC)(uintptr_t)invalidWindow;
    initialSerial = LastKnownRequestProcessed(&browserDisplay.display);
    browserTestXErrorCount = 0;
    browserTestLastErrorCode = Success;
    browserTestLastRequestCode = 0;
    browserTestLastResourceId = None;
    browserTestLastSerial = 0;
    browserTestRequestCodeSum = 0;
    browserTestRequestCodeMask = 0;
    sawConfigureBadMatch = 0;
    sawReconfigureBadValue = 0;
    sawIconifyBadValue = 0;
    sawWithdrawBadValue = 0;
    sawPointerMapLengthBadValue = 0;
    sawPointerMapDuplicateBadValue = 0;
    duplicateModifierMapResult = MappingFailed;
    sawModifierMapDuplicateBadValue = 0;
    sawStoreBufferBadValue = 0;
    sawSetRootParentRelativeBadMatch = 0;
    sawChangeRootParentRelativeBadMatch = 0;
    warpPointerZeroWidthX = -1;
    warpPointerZeroWidthY = -1;
    warpPointerZeroHeightX = -1;
    warpPointerZeroHeightY = -1;

    oldHandler = XSetErrorHandler(BrowserTestXErrorHandler);
    createPixmapResult = XCreatePixmap(&browserDisplay.display,
	    (Drawable)invalidWindow, 1, 1, 1);
    invalidCreatePixmapWidthResult = XCreatePixmap(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 0, 1,
	    (unsigned int)DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)));
    invalidCreatePixmapHeightResult = XCreatePixmap(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 1, 0,
	    (unsigned int)DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)));
    invalidCreatePixmapSizeResult = XCreatePixmap(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, (unsigned int)USHRT_MAX + 1U,
	    1, (unsigned int)DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)));
    invalidCreatePixmapDepthResult = XCreatePixmap(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 1, 1,
	    (unsigned int)DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)) + 1U);
    freePixmapResult = XFreePixmap(&browserDisplay.display,
	    (Pixmap)invalidWindow);
    createGCResult = XCreateGC(&browserDisplay.display,
	    (Drawable)invalidWindow, 0, NULL);
    getImageResult = XGetImage(&browserDisplay.display,
	    (Drawable)invalidWindow, 0, 0, 1, 1, AllPlanes, ZPixmap);
    invalidFormatGetImageResult = XGetImage(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 0, 0, 1, 1, AllPlanes, 99);
    xyBitmapGetImageResult = XGetImage(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 0, 0, 1, 1, AllPlanes,
	    XYBitmap);
    oversizeGetImageResult = XGetImage(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 0, 0,
	    (unsigned int)USHRT_MAX + 1U, 1, AllPlanes, ZPixmap);
    outOfBoundsGetImageResult = XGetImage(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, -1, 0, 1, 1, AllPlanes,
	    ZPixmap);
    putImageResult = XPutImage(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, NULL, 0, 0, 0, 0, 1, 1);
    invalidFormatPutImageResult = XPutImage(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, &invalidFormatPutImage,
	    0, 0, 0, 0, 1, 1);
    putImageResult += XPutImage(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, &depthMismatchPutImage,
	    0, 0, 0, 0, (unsigned int)USHRT_MAX + 1U, 1);
    copyAreaSourceResult = XCopyArea(&browserDisplay.display,
	    (Drawable)invalidWindow, (Drawable)browserDisplay.rootWindow,
	    NULL, 0, 0, 1, 1, 0, 0);
    copyAreaTargetResult = XCopyArea(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, (Drawable)invalidWindow,
	    NULL, 0, 0, 1, 1, 0, 0);
    copyAreaSizeResult = XCopyArea(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow,
	    (Drawable)browserDisplay.rootWindow, NULL, 0, 0,
	    (unsigned int)USHRT_MAX + 1U, 1, 0, 0);
    copyPlaneSourceResult = XCopyPlane(&browserDisplay.display,
	    (Drawable)invalidWindow, (Drawable)browserDisplay.rootWindow,
	    NULL, 0, 0, 1, 1, 0, 0, 1);
    copyPlaneTargetResult = XCopyPlane(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, (Drawable)invalidWindow,
	    NULL, 0, 0, 1, 1, 0, 0, 1);
    copyPlaneSizeResult = XCopyPlane(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow,
	    (Drawable)browserDisplay.rootWindow, NULL, 0, 0, 1,
	    (unsigned int)USHRT_MAX + 1U, 0, 0, 1);
    drawResultSum = XDrawPoint(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0);
    drawResultSum += XDrawLine(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, 1, 1);
    drawResultSum += XDrawSegments(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, &segment, 1);
    drawResultSum += XDrawRectangle(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, 1, 1);
    drawResultSum += XDrawArc(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, 1, 1, 0, 360 * 64);
    drawResultSum += XFillPolygon(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, points, 3, Convex,
	    CoordModeOrigin);
    drawResultSum += XFillRectangle(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, 1, 1);
    drawResultSum += XFillArc(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, 1, 1, 0, 360 * 64);
    drawResultSum += XDrawString(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, "x", 1);
    drawResultSum += XDrawString16(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, text16, 1);
    memset(textItems, 0, sizeof(textItems));
    textItems[0].chars = (char *)"x";
    textItems[0].nchars = 1;
    memset(textItems16, 0, sizeof(textItems16));
    textItems16[0].chars = text16;
    textItems16[0].nchars = 1;
    drawResultSum += XDrawText(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, textItems, 1);
    drawResultSum += XDrawText16(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, textItems16, 1);
    drawResultSum += XDrawImageString(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, "x", 1);
    drawResultSum += XDrawImageString16(&browserDisplay.display,
	    (Drawable)invalidWindow, NULL, 0, 0, text16, 1);
    drawValueValidationResultSum = XDrawLines(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, points, 2,
	    CoordModePrevious + 1);
    drawValueValidationResultSum += XDrawPoints(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, points, 1,
	    CoordModePrevious + 1);
    drawValueValidationResultSum += XFillPolygon(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, points, 3, Convex + 1,
	    CoordModeOrigin);
    drawValueValidationResultSum += XFillPolygon(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, points, 3, Convex,
	    CoordModePrevious + 1);
    drawValueValidationResultSum += XDrawRectangle(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, 0, 0,
	    (unsigned int)USHRT_MAX + 1U, 1);
    drawValueValidationResultSum += XDrawArc(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, 0, 0,
	    (unsigned int)USHRT_MAX + 1U, 1, 0, 360 * 64);
    drawValueValidationResultSum += XFillRectangle(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, 0, 0, 1,
	    (unsigned int)USHRT_MAX + 1U);
    drawValueValidationResultSum += XFillArc(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, NULL, 0, 0, 1,
	    (unsigned int)USHRT_MAX + 1U, 0, 360 * 64);
    nextTemporaryGContext = (GContext)browserDisplay.nextXId;
    temporaryGC = XCreateGC(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 0, NULL);
    temporaryGContext = XGContextFromGC(temporaryGC);
    freeTemporaryGCResult = (temporaryGC != NULL)
	    ? XFreeGC(&browserDisplay.display, temporaryGC) : !Success;
    gcResultSum = XChangeGC(&browserDisplay.display, NULL,
	    GCForeground, &gcValues);
    gcResultSum += XCopyGC(&browserDisplay.display, NULL, GCForeground,
	    defaultGC);
    gcResultSum += XCopyGC(&browserDisplay.display, defaultGC, GCForeground,
	    NULL);
    gcResultSum += XSetForeground(&browserDisplay.display, NULL, 1);
    gcResultSum += XSetDashes(&browserDisplay.display, NULL, 0, "\1", 1);
    gcResultSum += XSetClipRectangles(&browserDisplay.display, NULL, 0, 0,
	    &clipRect, 1, Unsorted);
    gcResultSum += XSetRegion(&browserDisplay.display, NULL, NULL);
    gcResultSum += XFreeGC(&browserDisplay.display, NULL);
    gcResultSum += XSetForeground(&browserDisplay.display, temporaryGC, 1);
    getGCValuesInvalidResult = XGetGCValues(&browserDisplay.display, NULL,
	    GCForeground, &gcValues);
    validFont = XLoadFont(&browserDisplay.display, "fixed");
    validCursor = XCreateGlyphCursor(&browserDisplay.display, validFont,
	    None, XC_left_ptr, 0, NULL, NULL);
    colorPixmap = XCreatePixmap(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 1, 1,
	    (unsigned int)DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)));
    invalidPixmapCursorResult = XCreatePixmapCursor(&browserDisplay.display,
	    (Pixmap)invalidWindow, None, NULL, NULL, 0, 0);
    badMatchCursorResult = XCreatePixmapCursor(&browserDisplay.display,
	    colorPixmap, None, NULL, NULL, 0, 0);
    invalidGlyphFontCursorResult = XCreateGlyphCursor(&browserDisplay.display,
	    (Font)invalidWindow, None, XC_left_ptr, 0, NULL, NULL);
    invalidGlyphValueCursorResult = XCreateGlyphCursor(&browserDisplay.display,
	    validFont, None, XC_num_glyphs, 0, NULL, NULL);
    cursorResultSum = XDefineCursor(&browserDisplay.display, invalidWindow,
	    validCursor);
    cursorResultSum += XDefineCursor(&browserDisplay.display,
	    browserDisplay.rootWindow, (Cursor)invalidWindow);
    cursorResultSum += XFreeCursor(&browserDisplay.display,
	    (Cursor)invalidWindow);
    cursorResultSum += XUnloadFont(&browserDisplay.display,
	    (Font)invalidWindow);
    depthOnePixmap = XCreatePixmap(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 1, 1, 1);
    validColormap = XCreateColormap(&browserDisplay.display,
	    browserDisplay.rootWindow, DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), AllocNone);
    gcValues.tile = (Pixmap)invalidWindow;
    createInvalidValueGCResult = XCreateGC(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, GCTile, &gcValues);
    createInvalidMaskGCResult = XCreateGC(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, invalidGCMask, &gcValues);
    referenceGC = XCreateGC(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 0, NULL);
    copyAreaDepthMismatchResult = XCopyArea(&browserDisplay.display,
	    (Drawable)depthOnePixmap, (Drawable)browserDisplay.rootWindow,
	    referenceGC, 0, 0, 1, 1, 0, 0);
    depthOneGC = XCreateGC(&browserDisplay.display,
	    (Drawable)depthOnePixmap, 0, NULL);
    depthMismatchPutImageResult = XPutImage(&browserDisplay.display,
	    depthOnePixmap, depthOneGC, &depthMismatchPutImage,
	    0, 0, 0, 0, 1, 1);
    gcValues.font = (Font)invalidWindow;
    gcReferenceResultSum = XChangeGC(&browserDisplay.display, referenceGC,
	    GCFont, &gcValues);
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    invalidGCMask, &gcValues);
    gcValues.font = validFont;
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    GCFont, &gcValues);
    gcValues.tile = (Pixmap)invalidWindow;
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    GCTile, &gcValues);
    gcValues.tile = depthOnePixmap;
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    GCTile, &gcValues);
    gcReferenceResultSum += XSetTile(&browserDisplay.display, referenceGC,
	    (Pixmap)invalidWindow);
    gcReferenceResultSum += XSetStipple(&browserDisplay.display,
	    referenceGC, colorPixmap);
    gcReferenceResultSum += XSetClipMask(&browserDisplay.display,
	    referenceGC, colorPixmap);
    gcReferenceResultSum += XSetFont(&browserDisplay.display, referenceGC,
	    (Font)invalidWindow);
    gcReferenceResultSum += XCopyGC(&browserDisplay.display, referenceGC,
	    invalidGCMask, referenceGC);
    gcReferenceResultSum += XCopyGC(&browserDisplay.display, referenceGC,
	    GCForeground, depthOneGC);
    gcValues.function = GXset + 1;
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    GCFunction, &gcValues);
    gcValues.line_width = (int)USHRT_MAX + 1;
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    GCLineWidth, &gcValues);
    gcValues.line_style = LineDoubleDash + 1;
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    GCLineStyle, &gcValues);
    gcValues.graphics_exposures = 2;
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    GCGraphicsExposures, &gcValues);
    gcValues.dash_offset = -1;
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    GCDashOffset, &gcValues);
    gcValues.dashes = 0;
    gcReferenceResultSum += XChangeGC(&browserDisplay.display, referenceGC,
	    GCDashList, &gcValues);
    gcReferenceResultSum += XSetFunction(&browserDisplay.display,
	    referenceGC, GXset + 1);
    gcReferenceResultSum += XSetState(&browserDisplay.display, referenceGC,
	    1, 0, GXset + 1, AllPlanes);
    gcReferenceResultSum += XSetGraphicsExposures(&browserDisplay.display,
	    referenceGC, 2);
    gcReferenceResultSum += XSetSubwindowMode(&browserDisplay.display,
	    referenceGC, IncludeInferiors + 1);
    gcReferenceResultSum += XSetFillStyle(&browserDisplay.display,
	    referenceGC, FillOpaqueStippled + 1);
    gcReferenceResultSum += XSetFillRule(&browserDisplay.display,
	    referenceGC, WindingRule + 1);
    gcReferenceResultSum += XSetArcMode(&browserDisplay.display,
	    referenceGC, ArcPieSlice + 1);
    gcReferenceResultSum += XSetLineAttributes(&browserDisplay.display,
	    referenceGC, (unsigned int)USHRT_MAX + 1U, LineSolid, CapButt,
	    JoinMiter);
    gcReferenceResultSum += XSetLineAttributes(&browserDisplay.display,
	    referenceGC, 1, LineDoubleDash + 1, CapButt, JoinMiter);
    gcReferenceResultSum += XSetDashes(&browserDisplay.display, referenceGC,
	    (int)USHRT_MAX + 1, "\1", 1);
    gcReferenceResultSum += XSetDashes(&browserDisplay.display, referenceGC,
	    0, NULL, 0);
    gcReferenceResultSum += XSetDashes(&browserDisplay.display, referenceGC,
	    0, zeroDash, 1);
    gcReferenceResultSum += XSetClipRectangles(&browserDisplay.display,
	    referenceGC, 0, 0, &clipRect, 1, YXBanded + 1);
    gcReferenceResultSum += XSetClipRectangles(&browserDisplay.display,
	    referenceGC, 0, 0, &clipRect, -1, Unsorted);
    gcReferenceResultSum += XSetClipRectangles(&browserDisplay.display,
	    referenceGC, 0, 0, NULL, 1, Unsorted);
    invalidDrawGCResultSum = XDrawPoint(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, invalidGC, 0, 0);
    invalidDrawGCResultSum += XCopyArea(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow,
	    (Drawable)browserDisplay.rootWindow, invalidGC, 0, 0, 1, 1, 0, 0);
    invalidDrawGCResultSum += XPutImage(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, invalidGC, NULL,
	    0, 0, 0, 0, 1, 1);
    invalidDrawGCResultSum += XDrawString(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, invalidGC, 0, 0, "x", 1);
    invalidDrawGCResultSum += XDrawImageString(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, invalidGC, 0, 0, "x", 1);
    invalidDrawGCResultSum += XDrawImageString16(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, invalidGC, 0, 0, text16, 1);
    invalidCreateWindowResult = XCreateWindow(&browserDisplay.display,
	    invalidWindow, 0, 0, 1, 1, 0, CopyFromParent, InputOutput,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 0, NULL);
    invalidCreateSizeResult = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 0, 1, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 0, NULL);
    invalidCreateLargeWindowResult = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, (unsigned int)USHRT_MAX + 1U, 1,
	    0, CopyFromParent, InputOutput,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 0, NULL);
    invalidCreateSimpleWindowResult = XCreateSimpleWindow(
	    &browserDisplay.display, browserDisplay.rootWindow, 0, 0, 1, 0,
	    0, 0, 0);
    mismatchedVisual = *DefaultVisual(&browserDisplay.display,
	    DefaultScreen(&browserDisplay.display));
    mismatchedVisual.visualid++;
    invalidCreateDepthWindowResult = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 1, 1, 0,
	    DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)) + 1,
	    InputOutput, DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 0, NULL);
    invalidCreateVisualWindowResult = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 1, 1, 0, CopyFromParent,
	    InputOutput, &mismatchedVisual, 0, NULL);
    inputOnlyParentWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 1, 1, 0, CopyFromParent,
	    InputOnly, (Visual *)CopyFromParent, 0, NULL);
    invalidCreateParentClassWindowResult = XCreateWindow(
	    &browserDisplay.display, inputOnlyParentWindow, 0, 0, 1, 1, 0,
	    CopyFromParent, InputOutput, DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 0, NULL);
    invalidCreateAttributeMaskWindowResult = XCreateWindow(
	    &browserDisplay.display, browserDisplay.rootWindow, 0, 0, 1, 1,
	    0, CopyFromParent, InputOutput,
	    DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)),
	    invalidWindowAttributeMask, NULL);
    invalidCreateColormapResult = XCreateColormap(&browserDisplay.display,
	    invalidWindow, DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), AllocNone);
    invalidVisualColormapResult = XCreateColormap(&browserDisplay.display,
	    browserDisplay.rootWindow, &mismatchedVisual, AllocNone);
    invalidFreeColormapResult = XFreeColormap(&browserDisplay.display,
	    (Colormap)invalidWindow);
    invalidAllocColorResult = XAllocColor(&browserDisplay.display,
	    (Colormap)invalidWindow, &color);
    invalidAllocNamedColorResult = XAllocNamedColor(&browserDisplay.display,
	    (Colormap)invalidWindow, "red", &screenColor, &exactColor);
    invalidLookupColorResult = XLookupColor(&browserDisplay.display,
	    (Colormap)invalidWindow, "red", &exactColor, &screenColor);
    invalidQueryColorResult = XQueryColor(&browserDisplay.display,
	    (Colormap)invalidWindow, &color);
    invalidQueryColorsResultSum = XQueryColors(&browserDisplay.display,
	    validColormap, NULL, -1);
    invalidQueryColorsResultSum += XQueryColors(&browserDisplay.display,
	    validColormap, NULL, 1);
    invalidFreeColorsResult = XFreeColors(&browserDisplay.display,
	    (Colormap)invalidWindow, &colorPixel, 1, 0);
    invalidFreeColorsListResultSum = XFreeColors(&browserDisplay.display,
	    validColormap, NULL, -1, 0);
    invalidFreeColorsListResultSum += XFreeColors(&browserDisplay.display,
	    validColormap, NULL, 1, 0);
    invalidWindowAttributeResultSum =
	    XSetWindowBackgroundPixmap(&browserDisplay.display,
		    browserDisplay.rootWindow, (Pixmap)invalidWindow);
    invalidWindowAttributeResultSum +=
	    XSetWindowBackgroundPixmap(&browserDisplay.display,
		    browserDisplay.rootWindow, depthOnePixmap);
    invalidWindowAttributeResultSum +=
	    XSetWindowBackgroundPixmap(&browserDisplay.display,
		    browserDisplay.rootWindow, (Pixmap)ParentRelative);
    sawSetRootParentRelativeBadMatch =
	    (browserTestLastErrorCode == BadMatch)
	    && (browserTestLastRequestCode
		    == X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES)
	    && (browserTestLastResourceId == browserDisplay.rootWindow);
    invalidWindowAttributeResultSum += XSetWindowColormap(
	    &browserDisplay.display, browserDisplay.rootWindow,
	    (Colormap)invalidWindow);
    attributes.background_pixmap = (Pixmap)ParentRelative;
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow, CWBackPixmap,
	    &attributes);
    sawChangeRootParentRelativeBadMatch =
	    (browserTestLastErrorCode == BadMatch)
	    && (browserTestLastRequestCode
		    == X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES)
	    && (browserTestLastResourceId == browserDisplay.rootWindow);
    attributes.cursor = (Cursor)invalidWindow;
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow, CWCursor,
	    &attributes);
    attributes.bit_gravity = StaticGravity + 1;
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow, CWBitGravity,
	    &attributes);
    attributes.win_gravity = StaticGravity + 1;
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow, CWWinGravity,
	    &attributes);
    attributes.backing_store = Always + 1;
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow, CWBackingStore,
	    &attributes);
    attributes.override_redirect = 2;
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow,
	    CWOverrideRedirect, &attributes);
    attributes.save_under = 2;
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow, CWSaveUnder,
	    &attributes);
    attributes.event_mask = OwnerGrabButtonMask << 1;
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow, CWEventMask,
	    &attributes);
    attributes.do_not_propagate_mask = ExposureMask;
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow,
	    CWDontPropagate, &attributes);
    invalidWindowAttributeResultSum += XChangeWindowAttributes(
	    &browserDisplay.display, browserDisplay.rootWindow,
	    invalidWindowAttributeMask, NULL);
    invalidWindowAttributeResultSum += XSelectInput(
	    &browserDisplay.display, browserDisplay.rootWindow,
	    OwnerGrabButtonMask << 1);
    configureWindow = XCreateWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 16, 12, 0, CopyFromParent,
	    InputOutput, DefaultVisual(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)), 0, NULL);
    configureValidationResultSum = (configureWindow != None)
	    ? Success : !Success;
    if (configureWindow != None) {
	changes.width = 0;
	configureValidationResultSum += XConfigureWindow(&browserDisplay.display,
		configureWindow, CWWidth, &changes);
	changes.width = (int)USHRT_MAX + 1;
	configureValidationResultSum += XConfigureWindow(&browserDisplay.display,
		configureWindow, CWWidth, &changes);
	changes.width = 16;
	changes.height = 0;
	configureValidationResultSum += XConfigureWindow(&browserDisplay.display,
		configureWindow, CWHeight, &changes);
	changes.height = 12;
	changes.border_width = -1;
	configureValidationResultSum += XConfigureWindow(&browserDisplay.display,
		configureWindow, CWBorderWidth, &changes);
	changes.border_width = (int)USHRT_MAX + 1;
	configureValidationResultSum += XConfigureWindow(&browserDisplay.display,
		configureWindow, CWBorderWidth, &changes);
	changes.border_width = 0;
	changes.stack_mode = Opposite + 1;
	configureValidationResultSum += XConfigureWindow(&browserDisplay.display,
		configureWindow, CWStackMode, &changes);
	configureValidationResultSum += XConfigureWindow(&browserDisplay.display,
		configureWindow, (unsigned int)(1U << 30), &changes);
	changes.sibling = invalidWindow;
	configureValidationResultSum += XConfigureWindow(&browserDisplay.display,
		configureWindow, CWSibling, &changes);
	changes.sibling = configureWindow;
	configureValidationResultSum += XConfigureWindow(&browserDisplay.display,
		configureWindow, CWSibling, &changes);
	sawConfigureBadMatch = (browserTestLastErrorCode == BadMatch)
		&& (browserTestLastRequestCode == X11_WASM_XREQ_CONFIGURE_WINDOW)
		&& (browserTestLastResourceId == (XID)configureWindow);
	changes.x = 3;
	changes.y = 5;
	reconfigureValidResult = XReconfigureWMWindow(&browserDisplay.display,
		configureWindow, DefaultScreen(&browserDisplay.display),
		CWX | CWY, &changes);
	reconfigureInvalidScreenResult = XReconfigureWMWindow(
		&browserDisplay.display, configureWindow,
		ScreenCount(&browserDisplay.display), CWX, &changes);
	sawReconfigureBadValue = (browserTestLastErrorCode == BadValue)
		&& (browserTestLastRequestCode == X11_WASM_XREQ_CONFIGURE_WINDOW)
		&& (browserTestLastResourceId
			== (XID)ScreenCount(&browserDisplay.display));
	iconifyValidResult = XIconifyWindow(&browserDisplay.display,
		configureWindow, DefaultScreen(&browserDisplay.display));
	iconifyInvalidScreenResult = XIconifyWindow(&browserDisplay.display,
		configureWindow, ScreenCount(&browserDisplay.display));
	sawIconifyBadValue = (browserTestLastErrorCode == BadValue)
		&& (browserTestLastRequestCode == X11_WASM_XREQ_UNMAP_WINDOW)
		&& (browserTestLastResourceId
			== (XID)ScreenCount(&browserDisplay.display));
	withdrawValidResult = XWithdrawWindow(&browserDisplay.display,
		configureWindow, DefaultScreen(&browserDisplay.display));
	withdrawInvalidScreenResult = XWithdrawWindow(&browserDisplay.display,
		configureWindow, ScreenCount(&browserDisplay.display));
	sawWithdrawBadValue = (browserTestLastErrorCode == BadValue)
		&& (browserTestLastRequestCode == X11_WASM_XREQ_UNMAP_WINDOW)
		&& (browserTestLastResourceId
			== (XID)ScreenCount(&browserDisplay.display));
	unviewableGetImageResult = XGetImage(&browserDisplay.display,
		(Drawable)configureWindow, 0, 0, 1, 1, AllPlanes,
		ZPixmap);
    } else {
	reconfigureValidResult = False;
	reconfigureInvalidScreenResult = True;
	iconifyValidResult = False;
	iconifyInvalidScreenResult = True;
	withdrawValidResult = False;
	withdrawInvalidScreenResult = True;
    }
    (void)XGetInputFocus(&browserDisplay.display, &focusBeforeValidation,
	    &revertBeforeValidation);
    focusValidationResultSum = XSetInputFocus(&browserDisplay.display,
	    invalidWindow, RevertToParent, CurrentTime);
    focusValidationResultSum += XSetInputFocus(&browserDisplay.display,
	    browserDisplay.rootWindow, RevertToParent + 1, CurrentTime);
    focusValidationResultSum += XSetInputFocus(&browserDisplay.display,
	    configureWindow, RevertToParent, CurrentTime);
    (void)XGetInputFocus(&browserDisplay.display, &focusAfterValidation,
	    &revertAfterValidation);
    grabKeyboardModeResult = XGrabKeyboard(&browserDisplay.display,
	    browserDisplay.rootWindow, False, GrabModeAsync + 1,
	    GrabModeAsync, CurrentTime);
    grabKeyboardOwnerEventsResult = XGrabKeyboard(&browserDisplay.display,
	    browserDisplay.rootWindow, 2, GrabModeAsync, GrabModeAsync,
	    CurrentTime);
    grabKeyboardWindowResult = XGrabKeyboard(&browserDisplay.display,
	    invalidWindow, False, GrabModeAsync, GrabModeAsync,
	    CurrentTime);
    grabPointerModeResult = XGrabPointer(&browserDisplay.display,
	    browserDisplay.rootWindow, False, 0, GrabModeAsync + 1,
	    GrabModeAsync, None, None, CurrentTime);
    grabPointerOwnerEventsResult = XGrabPointer(&browserDisplay.display,
	    browserDisplay.rootWindow, 2, 0, GrabModeAsync, GrabModeAsync,
	    None, None, CurrentTime);
    grabPointerEventMaskResult = XGrabPointer(&browserDisplay.display,
	    browserDisplay.rootWindow, False, ExposureMask, GrabModeAsync,
	    GrabModeAsync, None, None, CurrentTime);
    grabPointerWindowResult = XGrabPointer(&browserDisplay.display,
	    invalidWindow, False, 0, GrabModeAsync, GrabModeAsync,
	    None, None, CurrentTime);
    grabPointerConfineResult = XGrabPointer(&browserDisplay.display,
	    browserDisplay.rootWindow, False, 0, GrabModeAsync, GrabModeAsync,
	    invalidWindow, None, CurrentTime);
    grabPointerCursorResult = XGrabPointer(&browserDisplay.display,
	    browserDisplay.rootWindow, False, 0, GrabModeAsync, GrabModeAsync,
	    None, (Cursor)invalidWindow, CurrentTime);
    passiveGrabValidationResultSum = XGrabButton(&browserDisplay.display,
	    256U, 0, browserDisplay.rootWindow, False, 0, GrabModeAsync,
	    GrabModeAsync, None, None);
    sawGrabButtonBadValue = (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_GRAB_BUTTON)
	    && (browserTestLastResourceId == (XID)256U);
    passiveGrabValidationResultSum += XGrabButton(&browserDisplay.display,
	    Button1, 0, invalidWindow, False, 0, GrabModeAsync,
	    GrabModeAsync, None, None);
    sawGrabButtonBadWindow = (browserTestLastErrorCode == BadWindow)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_GRAB_BUTTON)
	    && (browserTestLastResourceId == (XID)invalidWindow);
    passiveGrabValidationResultSum += XGrabKey(&browserDisplay.display,
	    browserDisplay.display.min_keycode - 1, 0,
	    browserDisplay.rootWindow, False, GrabModeAsync, GrabModeAsync);
    sawGrabKeyBadValue = (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_GRAB_KEY)
	    && (browserTestLastResourceId
		    == (XID)(browserDisplay.display.min_keycode - 1));
    passiveGrabValidationResultSum += XUngrabKey(&browserDisplay.display,
	    AnyKey, 0, invalidWindow);
    sawUngrabKeyBadWindow = (browserTestLastErrorCode == BadWindow)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_UNGRAB_KEY)
	    && (browserTestLastResourceId == (XID)invalidWindow);
    passiveGrabValidationResultSum += XAllowEvents(&browserDisplay.display,
	    SyncBoth + 1, CurrentTime);
    sawAllowEventsBadValue = (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_ALLOW_EVENTS)
	    && (browserTestLastResourceId == (XID)(SyncBoth + 1));
    passiveGrabValidationResultSum += XChangeActivePointerGrab(
	    &browserDisplay.display, ExposureMask, None, CurrentTime);
    sawChangeActivePointerGrabBadValue =
	    (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode
		    == X11_WASM_XREQ_CHANGE_ACTIVE_POINTER_GRAB)
	    && (browserTestLastResourceId == (XID)ExposureMask);
    passiveGrabValidationResultSum += XChangeActivePointerGrab(
	    &browserDisplay.display, 0, (Cursor)invalidWindow, CurrentTime);
    sawChangeActivePointerGrabBadCursor =
	    (browserTestLastErrorCode == BadCursor)
	    && (browserTestLastRequestCode
		    == X11_WASM_XREQ_CHANGE_ACTIVE_POINTER_GRAB)
	    && (browserTestLastResourceId == (XID)invalidWindow);
    invalidPointerMapLengthResult = XSetPointerMapping(&browserDisplay.display,
	    pointerMap, 4);
    sawPointerMapLengthBadValue = (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode
		    == X11_WASM_XREQ_SET_POINTER_MAPPING)
	    && (browserTestLastResourceId == (XID)4);
    duplicatePointerMapResult = XSetPointerMapping(&browserDisplay.display,
	    duplicatePointerMap, 5);
    sawPointerMapDuplicateBadValue = (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode
		    == X11_WASM_XREQ_SET_POINTER_MAPPING)
	    && (browserTestLastResourceId == (XID)Button2);
    duplicateModifierMap = XNewModifiermap(1);
    if (duplicateModifierMap != NULL) {
	duplicateModifierMap->modifiermap[ShiftMapIndex] =
		X11_WASM_KEYCODE_SHIFT;
	duplicateModifierMap->modifiermap[ControlMapIndex] =
		X11_WASM_KEYCODE_SHIFT;
	duplicateModifierMapResult = XSetModifierMapping(
		&browserDisplay.display, duplicateModifierMap);
	sawModifierMapDuplicateBadValue =
		(browserTestLastErrorCode == BadValue)
		&& (browserTestLastRequestCode
			== X11_WASM_XREQ_SET_MODIFIER_MAPPING)
		&& (browserTestLastResourceId
			== (XID)X11_WASM_KEYCODE_SHIFT);
	XFreeModifiermap(duplicateModifierMap);
	duplicateModifierMap = NULL;
    }
    bellValidationResult = XBell(&browserDisplay.display, 101);
    (void)XSetScreenSaver(&browserDisplay.display, -1, 30,
	    PreferBlanking, AllowExposures);
    (void)XForceScreenSaver(&browserDisplay.display, ScreenSaverActive + 1);
    if (configureWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, configureWindow);
    }
    if (inputOnlyParentWindow != None) {
	(void)XDestroyWindow(&browserDisplay.display, inputOnlyParentWindow);
    }
    if (validCursor != None) {
	(void)XFreeCursor(&browserDisplay.display, validCursor);
    }
    if (referenceGC != NULL) {
	(void)XFreeGC(&browserDisplay.display, referenceGC);
    }
    if (depthOneGC != NULL) {
	(void)XFreeGC(&browserDisplay.display, depthOneGC);
    }
    if (colorPixmap != None) {
	(void)XFreePixmap(&browserDisplay.display, colorPixmap);
    }
    if (depthOnePixmap != None) {
	(void)XFreePixmap(&browserDisplay.display, depthOnePixmap);
    }
    if (validColormap != None) {
	(void)XFreeColormap(&browserDisplay.display, validColormap);
    }
    if (validFont != None) {
	(void)XUnloadFont(&browserDisplay.display, validFont);
    }
    attributesResult = XGetWindowAttributes(&browserDisplay.display,
	    invalidWindow, &windowAttributes);
    geometryResult = XGetGeometry(&browserDisplay.display,
	    (Drawable)invalidWindow, &root, &x, &y, &width, &height,
	    &borderWidth, &depth);
    sawGeometryBadDrawable = (browserTestLastErrorCode == BadDrawable)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_GET_GEOMETRY)
	    && (browserTestLastResourceId == (XID)invalidWindow);
    translateSourceResult = XTranslateCoordinates(&browserDisplay.display,
	    invalidWindow, browserDisplay.rootWindow, 1, 2, &x, &y, &child);
    translateDestinationResult = XTranslateCoordinates(&browserDisplay.display,
	    browserDisplay.rootWindow, invalidWindow, 1, 2, &x, &y, &child);
    (void)XWarpPointer(&browserDisplay.display, invalidWindow, None,
	    0, 0, 0, 0, 1, 2);
    (void)XWarpPointer(&browserDisplay.display, None, invalidWindow,
	    0, 0, 0, 0, 1, 2);
    warpPointerSizeResult = XWarpPointer(&browserDisplay.display, None, None,
	    0, 0, (unsigned int)USHRT_MAX + 1U, 1, 1, 2);
    sawWarpPointerBadValue = (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_WARP_POINTER);
    BrowserUpdatePointerState(browserDisplay.rootWindow, 4, 4, 0);
    (void)XWarpPointer(&browserDisplay.display, browserDisplay.rootWindow,
	    None, 10, 0, 0, 0, 100, 0);
    (void)XQueryPointer(&browserDisplay.display, browserDisplay.rootWindow,
	    &root, &child, &warpPointerZeroWidthX, &warpPointerZeroWidthY,
	    &x, &y, NULL);
    BrowserUpdatePointerState(browserDisplay.rootWindow, 4, 4, 0);
    (void)XWarpPointer(&browserDisplay.display, browserDisplay.rootWindow,
	    None, 0, 10, 0, 0, 0, 100);
    (void)XQueryPointer(&browserDisplay.display, browserDisplay.rootWindow,
	    &root, &child, &warpPointerZeroHeightX,
	    &warpPointerZeroHeightY, &x, &y, NULL);
    queryPointerResult = XQueryPointer(&browserDisplay.display,
	    invalidWindow, &root, &child, &x, &y, &x, &y, NULL);
    clearAreaExposuresResult = XClearArea(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0, 1, 1, 2);
    clearAreaSizeResult = XClearArea(&browserDisplay.display,
	    browserDisplay.rootWindow, 0, 0,
	    (unsigned int)USHRT_MAX + 1U, 1, False);
    sawClearAreaBadValue = (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_CLEAR_AREA);
    event.type = LASTEvent + 1;
    invalidSendEventTypeResult = XSendEvent(&browserDisplay.display,
	    browserDisplay.rootWindow, False, KeyPressMask, &event);
    event.type = KeyPress;
    sendEventPropagateResult = XSendEvent(&browserDisplay.display,
	    browserDisplay.rootWindow, 2, KeyPressMask, &event);
    sendEventResult = XSendEvent(&browserDisplay.display, invalidWindow,
	    False, KeyPressMask, &event);
    invalidInternAtomResult = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_INVALID_BOOL_ATOM", 2);
    validSelection = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_SELECTION", False);
    validProperty = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_PROPERTY", False);
    changePixmapPropertyResult = !Success;
    getPixmapPropertyResult = !BadWindow;
    deletePixmapPropertyResult = !Success;
    queryPixmapResult = !False;
    propertyPixmap = XCreatePixmap(&browserDisplay.display,
	    (Drawable)browserDisplay.rootWindow, 1, 1,
	    (unsigned int)DefaultDepth(&browserDisplay.display,
		    DefaultScreen(&browserDisplay.display)));
    if (propertyPixmap != None) {
	changePixmapPropertyResult = XChangeProperty(&browserDisplay.display,
		(Window)propertyPixmap, validProperty, XA_STRING, 8,
		PropModeReplace, value, 1);
	getPixmapPropertyResult = XGetWindowProperty(&browserDisplay.display,
		(Window)propertyPixmap, validProperty, 0, 1, False,
		AnyPropertyType, NULL, NULL, NULL, NULL, NULL);
	deletePixmapPropertyResult = XDeleteProperty(&browserDisplay.display,
		(Window)propertyPixmap, validProperty);
	queryPixmapResult = XQueryTree(&browserDisplay.display,
		(Window)propertyPixmap, NULL, NULL, NULL, NULL);
	(void)XFreePixmap(&browserDisplay.display, propertyPixmap);
    }
    invalidSelectionOwnerResult = XGetSelectionOwner(&browserDisplay.display,
	    invalidAtom);
    selectionValidationResultSum = XSetSelectionOwner(&browserDisplay.display,
	    invalidAtom, browserDisplay.rootWindow, CurrentTime);
    selectionValidationResultSum += XSetSelectionOwner(&browserDisplay.display,
	    validSelection, invalidWindow, CurrentTime);
    selectionValidationResultSum += XConvertSelection(&browserDisplay.display,
	    invalidAtom, XA_STRING, validProperty, browserDisplay.rootWindow,
	    CurrentTime);
    selectionValidationResultSum += XConvertSelection(&browserDisplay.display,
	    validSelection, invalidAtom, validProperty, browserDisplay.rootWindow,
	    CurrentTime);
    selectionValidationResultSum += XConvertSelection(&browserDisplay.display,
	    validSelection, XA_STRING, invalidAtom, browserDisplay.rootWindow,
	    CurrentTime);
    selectionValidationResultSum += XConvertSelection(&browserDisplay.display,
	    validSelection, XA_STRING, validProperty, invalidWindow,
	    CurrentTime);
    propertyValidationResultSum = XChangeProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, None, XA_STRING, 8, PropModeReplace,
	    value, 1);
    propertyValidationResultSum += XChangeProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, invalidAtom, 8,
	    PropModeReplace, value, 1);
    propertyValidationResultSum += XChangeProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, XA_STRING, 7,
	    PropModeReplace, value, 1);
    propertyValidationResultSum += XChangeProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, XA_STRING, 8, 99,
	    value, 1);
    propertyValidationResultSum += XChangeProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, XA_STRING, 32,
	    PropModeReplace, value, INT_MAX);
    propertyValidationResultSum += XChangeProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, XA_STRING, 8,
	    PropModeReplace, value, 1);
    propertyValidationResultSum += XChangeProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, XA_ATOM, 32,
	    PropModeAppend, (const unsigned char *)&atomPropertyValue, 1);
    largeLengthGetResult = XGetWindowProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, 0, LONG_MAX, False,
	    AnyPropertyType, &largeLengthActualType, &largeLengthActualFormat,
	    &largeLengthItemCount, &largeLengthBytesAfter,
	    &largeLengthPropertyData);
    largeLengthGetOk = (largeLengthGetResult == Success)
	    && (largeLengthActualType == XA_STRING)
	    && (largeLengthActualFormat == 8)
	    && (largeLengthItemCount == 1)
	    && (largeLengthBytesAfter == 0)
	    && (largeLengthPropertyData != NULL)
	    && (largeLengthPropertyData[0] == 'x');
    if (largeLengthPropertyData != NULL) {
	XFree(largeLengthPropertyData);
	largeLengthPropertyData = NULL;
    }
    getLargeOffsetResult = XGetWindowProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, 1, 1, False,
	    AnyPropertyType, NULL, NULL, NULL, NULL, NULL);
    rotateErrorAtoms[0] = validProperty;
    rotateErrorAtoms[1] = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_MISSING_ROTATE_PROPERTY", False);
    rotateWindowValidationResultSum = XRotateWindowProperties(
	    &browserDisplay.display, browserDisplay.rootWindow,
	    rotateErrorAtoms, 2, 1);
    sawRotateWindowBadMatch = (browserTestLastErrorCode == BadMatch)
	    && (browserTestLastRequestCode
		    == X11_WASM_XREQ_ROTATE_PROPERTIES)
	    && (browserTestLastResourceId == (XID)rotateErrorAtoms[1]);
    rotateErrorAtoms[1] = validProperty;
    rotateWindowValidationResultSum += XRotateWindowProperties(
	    &browserDisplay.display, browserDisplay.rootWindow,
	    rotateErrorAtoms, 2, 1);
    sawRotateWindowDuplicateBadMatch =
	    (browserTestLastErrorCode == BadMatch)
	    && (browserTestLastRequestCode
		    == X11_WASM_XREQ_ROTATE_PROPERTIES)
	    && (browserTestLastResourceId == (XID)validProperty);
    storeBufferValidationResult = XStoreBuffer(&browserDisplay.display,
	    "invalid", 7, 8);
    sawStoreBufferBadValue = (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_CHANGE_PROPERTY)
	    && (browserTestLastResourceId == (XID)8);
    invalidFetchBuffer = XFetchBuffer(&browserDisplay.display,
	    &invalidFetchBufferLength, 8);
    sawFetchBufferBadValue = (invalidFetchBuffer == NULL)
	    && (invalidFetchBufferLength == 0)
	    && (browserTestLastErrorCode == BadValue)
	    && (browserTestLastRequestCode == X11_WASM_XREQ_GET_PROPERTY)
	    && (browserTestLastResourceId == (XID)8);
    if (invalidFetchBuffer != NULL) {
	XFree(invalidFetchBuffer);
	invalidFetchBuffer = NULL;
    }
    propertyValidationResultSum += XChangeProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, BrowserWmProtocolsAtom(), XA_ATOM, 32,
	    PropModeReplace, (const unsigned char *)&invalidAtom, 1);
    propertyValidationResultSum += XDeleteProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, BrowserWmProtocolsAtom());
    propertyValidationResultSum += XDeleteProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty);
    propertyValidationResultSum += XDeleteProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, invalidAtom);
    propertyValidationResultSum += XSetIconSizes(&browserDisplay.display,
	    browserDisplay.rootWindow, NULL, -1);
    propertyValidationResultSum += XSetIconSizes(&browserDisplay.display,
	    browserDisplay.rootWindow, NULL, 1);
    propertyValidationResultSum += XSetIconSizes(&browserDisplay.display,
	    browserDisplay.rootWindow, &oversizedIconSize, INT_MAX);
    XSetRGBColormaps(&browserDisplay.display, browserDisplay.rootWindow,
	    NULL, -1, validProperty);
    XSetRGBColormaps(&browserDisplay.display, browserDisplay.rootWindow,
	    NULL, 1, validProperty);
    XSetRGBColormaps(&browserDisplay.display, browserDisplay.rootWindow,
	    &oversizedStandardColormap, INT_MAX, validProperty);
    invalidWMColormapCountResult = XSetWMColormapWindows(
	    &browserDisplay.display, browserDisplay.rootWindow, NULL, -1);
    invalidWMColormapListResult = XSetWMColormapWindows(
	    &browserDisplay.display, browserDisplay.rootWindow, NULL, 1);
    getInvalidAtomResult = XGetWindowProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, invalidAtom, 0, 1, False,
	    AnyPropertyType, NULL, NULL, NULL, NULL, NULL);
    getNegativeOffsetResult = XGetWindowProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, -1, 1, False,
	    AnyPropertyType, NULL, NULL, NULL, NULL, NULL);
    getNegativeLengthResult = XGetWindowProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, 0, -1, False,
	    AnyPropertyType, NULL, NULL, NULL, NULL, NULL);
    getDeletePropertyResult = XGetWindowProperty(&browserDisplay.display,
	    browserDisplay.rootWindow, validProperty, 0, 1, 2,
	    AnyPropertyType, NULL, NULL, NULL, NULL, NULL);
    invalidAtomName = XGetAtomName(&browserDisplay.display, invalidAtom);
    invalidAtomNamesResult = XGetAtomNames(&browserDisplay.display,
	    invalidAtomList, 2, invalidAtomNames);
    result = XQueryTree(&browserDisplay.display, invalidWindow, NULL, NULL,
	    NULL, NULL);
    (void)XChangeProperty(&browserDisplay.display, invalidWindow, XA_WM_NAME,
	    XA_STRING, 8, PropModeReplace, value, 1);
    getResult = XGetWindowProperty(&browserDisplay.display, invalidWindow,
	    XA_WM_NAME, 0, 1, False, AnyPropertyType, &actualType,
	    &actualFormat, &itemCount, &bytesAfter, &propertyData);
    (void)XDeleteProperty(&browserDisplay.display, invalidWindow, XA_WM_NAME);
    (void)XMapWindow(&browserDisplay.display, invalidWindow);
    (void)XUnmapWindow(&browserDisplay.display, invalidWindow);
    (void)XConfigureWindow(&browserDisplay.display, invalidWindow,
	    CWX | CWY, &changes);
    (void)XChangeWindowAttributes(&browserDisplay.display, invalidWindow,
	    CWEventMask, &attributes);
    (void)XSelectInput(&browserDisplay.display, invalidWindow,
	    ExposureMask);
    (void)XRaiseWindow(&browserDisplay.display, invalidWindow);
    (void)XLowerWindow(&browserDisplay.display, invalidWindow);
    (void)XReparentWindow(&browserDisplay.display,
	    browserDisplay.rootWindow, invalidWindow, 0, 0);
    (void)XReparentWindow(&browserDisplay.display, invalidWindow,
	    browserDisplay.rootWindow, 0, 0);
    (void)XClearArea(&browserDisplay.display, invalidWindow, 0, 0, 1, 1,
	    False);
    (void)XDestroyWindow(&browserDisplay.display, invalidWindow);
    (void)XSetErrorHandler(oldHandler);

    if (result != False) {
	return 0;
    }
    if ((attributesResult != False) || (geometryResult != False)
	    || (translateSourceResult != False)
	    || (translateDestinationResult != False)
	    || (warpPointerSizeResult != Success)
	    || (queryPointerResult != False)
	    || (clearAreaExposuresResult != Success)
	    || (clearAreaSizeResult != Success)
	    || (invalidSendEventTypeResult != False)
	    || (sendEventPropagateResult != False)
	    || (sendEventResult != False)
	    || (invalidInternAtomResult != None)
	    || (validSelection == None)
	    || (invalidSelectionOwnerResult != None)
	    || (selectionValidationResultSum != Success)
	    || (createPixmapResult != None)
	    || (invalidCreatePixmapWidthResult != None)
	    || (invalidCreatePixmapHeightResult != None)
	    || (invalidCreatePixmapSizeResult != None)
	    || (invalidCreatePixmapDepthResult != None)
	    || (freePixmapResult != Success)
	    || (createGCResult != NULL)
	    || (getImageResult != NULL)
	    || (invalidFormatGetImageResult != NULL)
	    || (xyBitmapGetImageResult != NULL)
	    || (oversizeGetImageResult != NULL)
	    || (outOfBoundsGetImageResult != NULL)
	    || (unviewableGetImageResult != NULL)
	    || (putImageResult != Success)
	    || (invalidFormatPutImageResult != Success)
	    || (depthMismatchPutImageResult != Success)
	    || (copyAreaSourceResult != Success)
	    || (copyAreaTargetResult != Success)
	    || (copyAreaSizeResult != Success)
	    || (copyPlaneSourceResult != Success)
	    || (copyPlaneTargetResult != Success)
	    || (copyPlaneSizeResult != Success)
	    || (drawResultSum != Success)
	    || (drawValueValidationResultSum != Success)
	    || (defaultGContext == None)
	    || (XGContextFromGC(NULL) != None)
	    || (XGContextFromGC(invalidGC) != None)
	    || (temporaryGC == NULL)
	    || (temporaryGContext != nextTemporaryGContext)
	    || (temporaryGContext == defaultGContext)
	    || (freeTemporaryGCResult != Success)
	    || (gcResultSum != Success)
	    || (getGCValuesInvalidResult != False)
	    || (validFont == None)
	    || (validCursor == None)
	    || (colorPixmap == None)
	    || (invalidPixmapCursorResult != None)
	    || (badMatchCursorResult != None)
	    || (invalidGlyphFontCursorResult != None)
	    || (invalidGlyphValueCursorResult != None)
	    || (cursorResultSum != Success)
	    || (depthOnePixmap == None)
	    || (validColormap == None)
	    || (createInvalidValueGCResult != NULL)
	    || (createInvalidMaskGCResult != NULL)
	    || (referenceGC == NULL)
	    || (depthOneGC == NULL)
	    || (copyAreaDepthMismatchResult != Success)
	    || (gcReferenceResultSum != Success)
	    || (invalidDrawGCResultSum != Success)
	    || (invalidCreateWindowResult != None)
	    || (invalidCreateSizeResult != None)
	    || (invalidCreateLargeWindowResult != None)
	    || (invalidCreateSimpleWindowResult != None)
	    || (invalidCreateDepthWindowResult != None)
	    || (invalidCreateVisualWindowResult != None)
	    || (inputOnlyParentWindow == None)
	    || (invalidCreateParentClassWindowResult != None)
	    || (invalidCreateAttributeMaskWindowResult != None)
	    || (invalidCreateColormapResult != None)
	    || (invalidVisualColormapResult != None)
	    || (invalidFreeColormapResult != Success)
	    || (invalidAllocColorResult != False)
	    || (invalidAllocNamedColorResult != False)
	    || (invalidLookupColorResult != False)
	    || (invalidQueryColorResult != Success)
	    || (invalidQueryColorsResultSum != Success)
	    || (invalidFreeColorsResult != Success)
	    || (invalidFreeColorsListResultSum != Success)
	    || (invalidWindowAttributeResultSum != Success)
	    || !sawSetRootParentRelativeBadMatch
	    || !sawChangeRootParentRelativeBadMatch
	    || (configureValidationResultSum != Success)
	    || (reconfigureValidResult != True)
	    || (reconfigureInvalidScreenResult != False)
	    || (iconifyValidResult != True)
	    || (iconifyInvalidScreenResult != False)
	    || (withdrawValidResult != True)
	    || (withdrawInvalidScreenResult != False)
	    || (focusValidationResultSum != Success)
	    || (focusAfterValidation != focusBeforeValidation)
	    || (revertAfterValidation != revertBeforeValidation)
	    || (grabKeyboardModeResult != BadValue)
	    || (grabKeyboardOwnerEventsResult != BadValue)
	    || (grabKeyboardWindowResult != GrabNotViewable)
	    || (grabPointerModeResult != BadValue)
	    || (grabPointerOwnerEventsResult != BadValue)
	    || (grabPointerEventMaskResult != BadValue)
	    || (grabPointerWindowResult != GrabNotViewable)
	    || (grabPointerConfineResult != GrabNotViewable)
	    || (grabPointerCursorResult != BadCursor)
	    || (passiveGrabValidationResultSum != Success)
	    || !sawGrabButtonBadValue
	    || !sawGrabButtonBadWindow
	    || !sawGrabKeyBadValue
	    || !sawUngrabKeyBadWindow
	    || !sawAllowEventsBadValue
	    || !sawChangeActivePointerGrabBadValue
	    || !sawChangeActivePointerGrabBadCursor
	    || (invalidPointerMapLengthResult != MappingFailed)
	    || (duplicatePointerMapResult != MappingFailed)
	    || !sawPointerMapLengthBadValue
	    || !sawPointerMapDuplicateBadValue
	    || (duplicateModifierMapResult != MappingFailed)
	    || !sawModifierMapDuplicateBadValue
	    || (bellValidationResult != Success)
	    || (validProperty == None)
	    || (propertyPixmap == None)
	    || (changePixmapPropertyResult != Success)
	    || (getPixmapPropertyResult != BadWindow)
	    || (deletePixmapPropertyResult != Success)
	    || (queryPixmapResult != False)
	    || (propertyValidationResultSum != Success)
	    || !largeLengthGetOk
	    || (invalidWMColormapCountResult != False)
	    || (invalidWMColormapListResult != False)
	    || (getInvalidAtomResult != BadAtom)
	    || (getNegativeOffsetResult != BadValue)
	    || (getNegativeLengthResult != BadValue)
	    || (getLargeOffsetResult != BadValue)
	    || (getDeletePropertyResult != BadValue)
	    || (rotateWindowValidationResultSum != Success)
	    || !sawRotateWindowBadMatch
	    || !sawRotateWindowDuplicateBadMatch
	    || (storeBufferValidationResult != Success)
	    || !sawStoreBufferBadValue
	    || !sawFetchBufferBadValue
	    || (invalidAtomName != NULL)
	    || (invalidAtomNamesResult != False)
	    || (invalidAtomNames[0] != NULL)
	    || (invalidAtomNames[1] != NULL)
	    || !sawGeometryBadDrawable
	    || !sawWarpPointerBadValue
	    || (warpPointerZeroWidthX != 4)
	    || (warpPointerZeroWidthY != 4)
	    || (warpPointerZeroHeightX != 4)
	    || (warpPointerZeroHeightY != 4)
	    || !sawClearAreaBadValue
	    || !sawConfigureBadMatch
	    || !sawReconfigureBadValue
	    || !sawIconifyBadValue
	    || !sawWithdrawBadValue) {
	return 0;
    }
    if ((getResult != BadWindow) || (propertyData != NULL)
	    || (actualType != None) || (actualFormat != 0)
	    || (itemCount != 0) || (bytesAfter != 0)) {
	return 0;
    }
    expectedRequestCodeSum =
	    X11_WASM_XREQ_CREATE_PIXMAP
	    + X11_WASM_XREQ_CREATE_PIXMAP
	    + X11_WASM_XREQ_CREATE_PIXMAP
	    + X11_WASM_XREQ_CREATE_PIXMAP
	    + X11_WASM_XREQ_CREATE_PIXMAP
	    + X11_WASM_XREQ_FREE_PIXMAP
	    + X11_WASM_XREQ_CREATE_GC
	    + X11_WASM_XREQ_GET_IMAGE
	    + X11_WASM_XREQ_GET_IMAGE
	    + X11_WASM_XREQ_GET_IMAGE
	    + X11_WASM_XREQ_GET_IMAGE
	    + X11_WASM_XREQ_GET_IMAGE
	    + X11_WASM_XREQ_PUT_IMAGE
	    + X11_WASM_XREQ_PUT_IMAGE
	    + X11_WASM_XREQ_PUT_IMAGE
	    + X11_WASM_XREQ_PUT_IMAGE
	    + X11_WASM_XREQ_COPY_AREA
	    + X11_WASM_XREQ_COPY_AREA
	    + X11_WASM_XREQ_COPY_AREA
	    + X11_WASM_XREQ_COPY_PLANE
	    + X11_WASM_XREQ_COPY_PLANE
	    + X11_WASM_XREQ_COPY_PLANE
	    + X11_WASM_XREQ_POLY_POINT
	    + X11_WASM_XREQ_POLY_LINE
	    + X11_WASM_XREQ_POLY_SEGMENT
	    + X11_WASM_XREQ_POLY_RECTANGLE
	    + X11_WASM_XREQ_POLY_ARC
	    + X11_WASM_XREQ_FILL_POLY
	    + X11_WASM_XREQ_POLY_FILL_RECTANGLE
	    + X11_WASM_XREQ_POLY_FILL_ARC
	    + X11_WASM_XREQ_POLY_TEXT_8
	    + X11_WASM_XREQ_POLY_TEXT_16
	    + X11_WASM_XREQ_POLY_TEXT_8
	    + X11_WASM_XREQ_POLY_TEXT_16
	    + X11_WASM_XREQ_IMAGE_TEXT_8
	    + X11_WASM_XREQ_IMAGE_TEXT_16
	    + X11_WASM_XREQ_POLY_LINE
	    + X11_WASM_XREQ_POLY_POINT
	    + X11_WASM_XREQ_FILL_POLY
	    + X11_WASM_XREQ_FILL_POLY
	    + X11_WASM_XREQ_POLY_RECTANGLE
	    + X11_WASM_XREQ_POLY_ARC
	    + X11_WASM_XREQ_POLY_FILL_RECTANGLE
	    + X11_WASM_XREQ_POLY_FILL_ARC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_COPY_GC
	    + X11_WASM_XREQ_COPY_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_SET_DASHES
	    + X11_WASM_XREQ_SET_CLIP_RECTANGLES
	    + X11_WASM_XREQ_SET_CLIP_RECTANGLES
	    + X11_WASM_XREQ_FREE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CREATE_CURSOR
	    + X11_WASM_XREQ_CREATE_CURSOR
	    + X11_WASM_XREQ_CREATE_GLYPH_CURSOR
	    + X11_WASM_XREQ_CREATE_GLYPH_CURSOR
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_FREE_CURSOR
	    + X11_WASM_XREQ_CLOSE_FONT
	    + X11_WASM_XREQ_CREATE_GC
	    + X11_WASM_XREQ_CREATE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_COPY_AREA
	    + X11_WASM_XREQ_COPY_GC
	    + X11_WASM_XREQ_COPY_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_CHANGE_GC
	    + X11_WASM_XREQ_SET_DASHES
	    + X11_WASM_XREQ_SET_DASHES
	    + X11_WASM_XREQ_SET_DASHES
	    + X11_WASM_XREQ_SET_CLIP_RECTANGLES
	    + X11_WASM_XREQ_SET_CLIP_RECTANGLES
	    + X11_WASM_XREQ_SET_CLIP_RECTANGLES
	    + X11_WASM_XREQ_POLY_POINT
	    + X11_WASM_XREQ_COPY_AREA
	    + X11_WASM_XREQ_PUT_IMAGE
	    + X11_WASM_XREQ_POLY_TEXT_8
	    + X11_WASM_XREQ_IMAGE_TEXT_8
	    + X11_WASM_XREQ_IMAGE_TEXT_16
	    + X11_WASM_XREQ_CREATE_WINDOW
	    + X11_WASM_XREQ_CREATE_WINDOW
	    + X11_WASM_XREQ_CREATE_WINDOW
	    + X11_WASM_XREQ_CREATE_WINDOW
	    + X11_WASM_XREQ_CREATE_WINDOW
	    + X11_WASM_XREQ_CREATE_WINDOW
	    + X11_WASM_XREQ_CREATE_WINDOW
	    + X11_WASM_XREQ_CREATE_WINDOW
	    + X11_WASM_XREQ_CREATE_COLORMAP
	    + X11_WASM_XREQ_CREATE_COLORMAP
	    + X11_WASM_XREQ_FREE_COLORMAP
	    + X11_WASM_XREQ_ALLOC_COLOR
	    + X11_WASM_XREQ_ALLOC_NAMED_COLOR
	    + X11_WASM_XREQ_LOOKUP_COLOR
	    + X11_WASM_XREQ_QUERY_COLORS
	    + X11_WASM_XREQ_QUERY_COLORS
	    + X11_WASM_XREQ_QUERY_COLORS
	    + X11_WASM_XREQ_FREE_COLORS
	    + X11_WASM_XREQ_FREE_COLORS
	    + X11_WASM_XREQ_FREE_COLORS
		    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
		    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
		    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
		    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
		    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
		    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
		    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
		    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
		    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_UNMAP_WINDOW
	    + X11_WASM_XREQ_UNMAP_WINDOW
	    + X11_WASM_XREQ_GET_IMAGE
	    + X11_WASM_XREQ_SET_INPUT_FOCUS
	    + X11_WASM_XREQ_SET_INPUT_FOCUS
	    + X11_WASM_XREQ_SET_INPUT_FOCUS
	    + X11_WASM_XREQ_GRAB_KEYBOARD
	    + X11_WASM_XREQ_GRAB_KEYBOARD
	    + X11_WASM_XREQ_GRAB_KEYBOARD
	    + X11_WASM_XREQ_GRAB_POINTER
	    + X11_WASM_XREQ_GRAB_POINTER
	    + X11_WASM_XREQ_GRAB_POINTER
	    + X11_WASM_XREQ_GRAB_POINTER
	    + X11_WASM_XREQ_GRAB_POINTER
	    + X11_WASM_XREQ_GRAB_POINTER
	    + X11_WASM_XREQ_GRAB_BUTTON
	    + X11_WASM_XREQ_GRAB_BUTTON
	    + X11_WASM_XREQ_GRAB_KEY
	    + X11_WASM_XREQ_UNGRAB_KEY
	    + X11_WASM_XREQ_ALLOW_EVENTS
	    + X11_WASM_XREQ_CHANGE_ACTIVE_POINTER_GRAB
	    + X11_WASM_XREQ_CHANGE_ACTIVE_POINTER_GRAB
	    + X11_WASM_XREQ_SET_POINTER_MAPPING
	    + X11_WASM_XREQ_SET_POINTER_MAPPING
	    + X11_WASM_XREQ_SET_MODIFIER_MAPPING
	    + X11_WASM_XREQ_BELL
	    + X11_WASM_XREQ_SET_SCREEN_SAVER
	    + X11_WASM_XREQ_FORCE_SCREEN_SAVER
	    + X11_WASM_XREQ_GET_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_GET_GEOMETRY
	    + X11_WASM_XREQ_TRANSLATE_COORDINATES
	    + X11_WASM_XREQ_TRANSLATE_COORDINATES
	    + X11_WASM_XREQ_WARP_POINTER
	    + X11_WASM_XREQ_WARP_POINTER
	    + X11_WASM_XREQ_WARP_POINTER
	    + X11_WASM_XREQ_QUERY_POINTER
	    + X11_WASM_XREQ_CLEAR_AREA
	    + X11_WASM_XREQ_CLEAR_AREA
	    + X11_WASM_XREQ_SEND_EVENT
	    + X11_WASM_XREQ_SEND_EVENT
	    + X11_WASM_XREQ_SEND_EVENT
	    + X11_WASM_XREQ_INTERN_ATOM
	    + X11_WASM_XREQ_GET_SELECTION_OWNER
	    + X11_WASM_XREQ_SET_SELECTION_OWNER
	    + X11_WASM_XREQ_SET_SELECTION_OWNER
	    + X11_WASM_XREQ_CONVERT_SELECTION
	    + X11_WASM_XREQ_CONVERT_SELECTION
	    + X11_WASM_XREQ_CONVERT_SELECTION
	    + X11_WASM_XREQ_CONVERT_SELECTION
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_GET_PROPERTY
	    + X11_WASM_XREQ_DELETE_PROPERTY
	    + X11_WASM_XREQ_QUERY_TREE
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_DELETE_PROPERTY
	    + X11_WASM_XREQ_GET_PROPERTY
	    + X11_WASM_XREQ_ROTATE_PROPERTIES
	    + X11_WASM_XREQ_ROTATE_PROPERTIES
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_GET_PROPERTY
	    + X11_WASM_XREQ_GET_PROPERTY
	    + X11_WASM_XREQ_GET_PROPERTY
	    + X11_WASM_XREQ_GET_PROPERTY
	    + X11_WASM_XREQ_GET_PROPERTY
	    + X11_WASM_XREQ_GET_ATOM_NAME
	    + X11_WASM_XREQ_GET_ATOM_NAME
	    + X11_WASM_XREQ_QUERY_TREE
	    + X11_WASM_XREQ_CHANGE_PROPERTY
	    + X11_WASM_XREQ_GET_PROPERTY
	    + X11_WASM_XREQ_DELETE_PROPERTY
	    + X11_WASM_XREQ_MAP_WINDOW
	    + X11_WASM_XREQ_UNMAP_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_CONFIGURE_WINDOW
	    + X11_WASM_XREQ_REPARENT_WINDOW
	    + X11_WASM_XREQ_REPARENT_WINDOW
	    + X11_WASM_XREQ_CLEAR_AREA
	    + X11_WASM_XREQ_DESTROY_WINDOW;
    expectedRequestCodeMask =
	    (1ULL << X11_WASM_XREQ_CREATE_WINDOW)
	    | (1ULL << X11_WASM_XREQ_CREATE_PIXMAP)
	    | (1ULL << X11_WASM_XREQ_FREE_PIXMAP)
	    | (1ULL << X11_WASM_XREQ_CREATE_GC)
	    | (1ULL << X11_WASM_XREQ_COPY_AREA)
	    | (1ULL << X11_WASM_XREQ_COPY_PLANE)
	    | (1ULL << X11_WASM_XREQ_CHANGE_GC)
	    | (1ULL << X11_WASM_XREQ_COPY_GC)
	    | (1ULL << X11_WASM_XREQ_SET_DASHES)
	    | (1ULL << X11_WASM_XREQ_SET_CLIP_RECTANGLES)
	    | (1ULL << X11_WASM_XREQ_FREE_GC)
	    | (1ULL << X11_WASM_XREQ_CLOSE_FONT)
	    | (1ULL << X11_WASM_XREQ_GET_WINDOW_ATTRIBUTES)
	    | (1ULL << X11_WASM_XREQ_GET_GEOMETRY)
	    | (1ULL << X11_WASM_XREQ_TRANSLATE_COORDINATES)
	    | (1ULL << X11_WASM_XREQ_WARP_POINTER)
	    | (1ULL << X11_WASM_XREQ_SET_INPUT_FOCUS)
	    | (1ULL << X11_WASM_XREQ_GRAB_POINTER)
	    | (1ULL << X11_WASM_XREQ_GRAB_KEYBOARD)
	    | (1ULL << X11_WASM_XREQ_GRAB_BUTTON)
	    | (1ULL << X11_WASM_XREQ_GRAB_KEY)
	    | (1ULL << X11_WASM_XREQ_UNGRAB_KEY)
	    | (1ULL << X11_WASM_XREQ_ALLOW_EVENTS)
	    | (1ULL << X11_WASM_XREQ_CHANGE_ACTIVE_POINTER_GRAB)
	    | (1ULL << X11_WASM_XREQ_QUERY_POINTER)
	    | (1ULL << X11_WASM_XREQ_SEND_EVENT)
	    | (1ULL << X11_WASM_XREQ_SET_SELECTION_OWNER)
	    | (1ULL << X11_WASM_XREQ_GET_SELECTION_OWNER)
	    | (1ULL << X11_WASM_XREQ_CONVERT_SELECTION)
	    | (1ULL << X11_WASM_XREQ_QUERY_TREE)
	    | (1ULL << X11_WASM_XREQ_INTERN_ATOM)
	    | (1ULL << X11_WASM_XREQ_GET_ATOM_NAME)
	    | (1ULL << X11_WASM_XREQ_CHANGE_PROPERTY)
	    | (1ULL << X11_WASM_XREQ_GET_PROPERTY)
	    | (1ULL << X11_WASM_XREQ_DELETE_PROPERTY)
	    | (1ULL << X11_WASM_XREQ_MAP_WINDOW)
	    | (1ULL << X11_WASM_XREQ_UNMAP_WINDOW)
	    | (1ULL << X11_WASM_XREQ_CONFIGURE_WINDOW)
	    | (1ULL << X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES)
	    | (1ULL << X11_WASM_XREQ_REPARENT_WINDOW)
	    | (1ULL << X11_WASM_XREQ_CLEAR_AREA)
	    | (1ULL << X11_WASM_XREQ_DESTROY_WINDOW);
    if ((browserTestXErrorCount != 240)
	    || (browserTestLastErrorCode != BadWindow)
	    || (browserTestLastRequestCode != X11_WASM_XREQ_DESTROY_WINDOW)
	    || (browserTestLastResourceId != (XID)invalidWindow)
	    || (browserTestRequestCodeSum != expectedRequestCodeSum)
	    || (browserTestRequestCodeMask != expectedRequestCodeMask)) {
	return 0;
    }
    return 1;
}

int
XFree(
    void *data)
{
    if (data != NULL) {
	ckfree(data);
    }
    return Success;
}
