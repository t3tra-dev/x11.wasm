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

    if (eventName == NULL) {
	return 0;
    }
    if (utf8 == NULL) {
	utf8 = "";
	utf8Length = 0;
    } else if (utf8Length < 0) {
	utf8Length = (int)strlen(utf8);
    }
    target = BrowserResolveKeyEventWindow((Window)window);
    if (!BrowserWindowCanReceiveEvents(target)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, ClientMessage, target, CurrentTime);
    event.xclient.message_type = XInternAtom(&browserDisplay.display,
	    eventName, False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = (long)phase;
    event.xclient.data.l[1] = (long)utf8Length;
    return BrowserQueueEvent(&event);
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
    XEvent event;
    Window target;
    int rootX = x;
    int rootY = y;
    int queued;

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
    if (type == ButtonPress) {
	Window passiveWindow = BrowserHitTestRoot(rootX, rootY, NULL, NULL);

	(void)BrowserActivatePassiveButtonGrab(passiveWindow, button, state,
		(Time)time);
    }
    target = BrowserResolvePointerEventWindow((Window)window, &x, &y);
    BrowserUpdatePointerState(target, rootX, rootY, state);

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
		    state & ~BrowserPointerButtonMask(button));
	}
	return 0;
    }
    if (type == ButtonPress) {
	browserDisplay.pointerCaptureWindow = target;
	BrowserSyncPointerState();
    }
    queued = BrowserQueueEvent(&event);
    if (type == ButtonRelease) {
	BrowserReleasePassivePointerGrabIfIdle(
		state & ~BrowserPointerButtonMask(button));
    }
    return queued;
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
    return BrowserQueuePointerEventFields(type, window, button, state, x, y,
	    time, 1);
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
    XEvent event;
    Window target;
    Window nextTarget;
    int rootX = x;
    int rootY = y;

    if ((type != EnterNotify) && (type != LeaveNotify)) {
	return 0;
    }
    (void)window;
    BrowserClampPointerToGrabConfine(&rootX, &rootY);
    x = rootX;
    y = rootY;
    nextTarget = BrowserHitTestRoot(rootX, rootY, NULL, NULL);
    if (nextTarget == browserDisplay.pointerWindow) {
	BrowserUpdatePointerState(nextTarget, rootX, rootY, state);
	return 0;
    }
    if (type == LeaveNotify) {
	if (!BrowserWindowCanReceiveEvents(browserDisplay.pointerWindow)) {
	    BrowserUpdatePointerState(nextTarget, rootX, rootY, state);
	    return 0;
	}
	target = browserDisplay.pointerWindow;
	BrowserWindowCoordinatesFromRoot(target, rootX, rootY, &x, &y);
    } else {
	if (!BrowserWindowCanReceiveEvents(nextTarget)) {
	    BrowserUpdatePointerState(nextTarget, rootX, rootY, state);
	    return 0;
	}
	target = nextTarget;
	BrowserWindowCoordinatesFromRoot(target, rootX, rootY, &x, &y);
    }
    BrowserUpdatePointerState(nextTarget, rootX, rootY, state);

    BrowserSetCommonEventFields(&event, type, target, (Time)time);
    event.xcrossing.root = browserDisplay.rootWindow;
    event.xcrossing.subwindow = None;
    event.xcrossing.time = eventTime;
    event.xcrossing.x = x;
    event.xcrossing.y = y;
    event.xcrossing.x_root = rootX;
    event.xcrossing.y_root = rootY;
    event.xcrossing.mode = NotifyNormal;
    event.xcrossing.detail = NotifyNonlinear;
    event.xcrossing.same_screen = True;
    event.xcrossing.focus = (browserDisplay.focusWindow != None);
    event.xcrossing.state = state;
    if (!BrowserPointerGrabAllowsEvent(&event)) {
	return 0;
    }
    return BrowserQueueEvent(&event);
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
    event.xfocus.serial = ++LastKnownRequestProcessed(&browserDisplay.display);
    BrowserSyncRequestState(&browserDisplay.display);
    event.xfocus.mode = NotifyNormal;
    event.xfocus.detail = NotifyNonlinear;
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
    int rootX = x;
    int rootY = y;

    if (delta == 0) {
	return 0;
    }
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
    resourcePtr->width = 1;
    resourcePtr->height = 1;
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
BrowserDeleteCloseDownResource(
    X11WasmResource *resourcePtr)
{
    XID id;

    if ((resourcePtr == NULL) || (resourcePtr == &browserDisplay.rootResource)) {
	return;
    }
    id = resourcePtr->id;
    if (resourcePtr->type == X11_WASM_RESOURCE_WINDOW) {
	(void)BrowserDestroyWindowResource(resourcePtr);
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

    for (;;) {
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
	BrowserDeleteCloseDownResource(resourcePtr);
    }
}

void
BrowserApplyCloseDownMode(void)
{
    X11WasmResource *resourcePtr;

    if (browserDisplay.closeDownMode == DestroyAll) {
	for (;;) {
	    for (resourcePtr = browserDisplay.resourceList;
		    resourcePtr != NULL; resourcePtr = resourcePtr->nextPtr) {
		if ((resourcePtr != &browserDisplay.rootResource)
			&& !resourcePtr->retained) {
		    break;
		}
	    }
	    if (resourcePtr == NULL) {
		return;
	    }
	    BrowserDeleteCloseDownResource(resourcePtr);
	}
    }

    for (resourcePtr = browserDisplay.resourceList; resourcePtr != NULL;
	    resourcePtr = resourcePtr->nextPtr) {
	if ((resourcePtr != &browserDisplay.rootResource)
		&& !resourcePtr->retained) {
	    resourcePtr->retained = 1;
	    resourcePtr->temporary =
		    browserDisplay.closeDownMode == RetainTemporary;
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
