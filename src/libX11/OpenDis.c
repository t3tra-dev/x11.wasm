#include "XlibInt.h"

int
BrowserScreenMillimeters(
    int pixels)
{
    int millimeters = (pixels * 254 + 480) / 960;

    return (millimeters > 0) ? millimeters : 1;
}

int
BrowserResizeRoot(
    unsigned int width,
    unsigned int height,
    int queueEvents)
{
    X11WasmDisplayState *state = &browserDisplay;
    unsigned int oldWidth;
    unsigned int oldHeight;

    if (width == 0) {
	width = X11_WASM_DEFAULT_SCREEN_WIDTH;
    }
    if (height == 0) {
	height = X11_WASM_DEFAULT_SCREEN_HEIGHT;
    }
    if ((state->rootResource.width == width)
	    && (state->rootResource.height == height)
	    && (state->screen.width == (int)width)
	    && (state->screen.height == (int)height)) {
	return 0;
    }

    oldWidth = state->rootResource.width;
    oldHeight = state->rootResource.height;
    state->screen.width = (int)width;
    state->screen.height = (int)height;
    state->screen.mwidth = BrowserScreenMillimeters((int)width);
    state->screen.mheight = BrowserScreenMillimeters((int)height);
    state->rootResource.width = width;
    state->rootResource.height = height;

    if (queueEvents) {
	(void)BrowserQueueConfigureNotify(&state->rootResource, None);
	(void)BrowserQueueExpose(&state->rootResource, 0, 0, width, height);
    }
    BrowserApplyChildWinGravity(&state->rootResource, oldWidth, oldHeight);
    return 1;
}

void
BrowserInitDisplay(void)
{
    X11WasmDisplayState *state = &browserDisplay;
    int rootWidth, rootHeight;

    if (browserDisplayInitialized) {
	return;
    }
    memset(state, 0, sizeof(*state));

    state->rootWindow = X11_WASM_ROOT_WINDOW;
    state->focusWindow = state->rootWindow;
    state->focusRevertTo = RevertToParent;
    state->focusTime = CurrentTime;
    state->pointerWindow = state->rootWindow;
    state->defaultColormap = 2;
    state->nextXId = 100;
    state->screenSaverTimeout = 600;
    state->screenSaverInterval = 600;
    state->screenSaverPreferBlanking = DefaultBlanking;
    state->screenSaverAllowExposures = DefaultExposures;
    state->keyClickPercent = 0;
    state->bellPercent = 50;
    state->bellPitch = 400;
    state->bellDuration = 100;
    state->ledMask = 0;
    state->globalAutoRepeat = AutoRepeatModeOn;
    memset(state->autoRepeats, 0xff, sizeof(state->autoRepeats));
    state->pointerAccelNumerator = 1;
    state->pointerAccelDenominator = 1;
    state->pointerThreshold = 0;
    state->pointerMapCount = 5;
    state->pointerMap[0] = Button1;
    state->pointerMap[1] = Button2;
    state->pointerMap[2] = Button3;
    state->pointerMap[3] = Button4;
    state->pointerMap[4] = Button5;
    state->hostAccessEnabled = True;

    state->visual.visualid = 1;
    state->visual.c_class = TrueColor;
    state->visual.red_mask = 0xff0000;
    state->visual.green_mask = 0x00ff00;
    state->visual.blue_mask = 0x0000ff;
    state->visual.bits_per_rgb = 8;
    state->visual.map_entries = 256;
    state->defaultGCContext = X11_WASM_DEFAULT_GCONTEXT;

    state->depth.depth = 24;
    state->depth.nvisuals = 1;
    state->depth.visuals = &state->visual;

    state->pixmapFormats[0].depth = 1;
    state->pixmapFormats[0].bits_per_pixel = 1;
    state->pixmapFormats[0].scanline_pad = 32;
    state->pixmapFormats[1].depth = 24;
    state->pixmapFormats[1].bits_per_pixel = 32;
    state->pixmapFormats[1].scanline_pad = 32;

    state->screen.display = &state->display;
    state->screen.root = state->rootWindow;
    state->screen.width = X11_WASM_DEFAULT_SCREEN_WIDTH;
    state->screen.height = X11_WASM_DEFAULT_SCREEN_HEIGHT;
    state->screen.mwidth = BrowserScreenMillimeters(state->screen.width);
    state->screen.mheight = BrowserScreenMillimeters(state->screen.height);
    state->screen.ndepths = 1;
    state->screen.depths = &state->depth;
    state->screen.root_depth = 24;
    state->screen.root_visual = &state->visual;
    state->screen.cmap = state->defaultColormap;
    state->screen.white_pixel = 0xffffff;
    state->screen.black_pixel = 0x000000;
    state->screen.max_maps = 1;
    state->screen.min_maps = 1;
    state->screen.backing_store = NotUseful;
    state->screen.save_unders = False;

    state->display.fd = 0;
    state->display.proto_major_version = X_PROTOCOL;
    state->display.proto_minor_version = X_PROTOCOL_REVISION;
    state->display.vendor = (char *)"x11.wasm";
    state->display.resource_alloc = X11WasmAllocXId;
    state->display.byte_order = LSBFirst;
    state->display.bitmap_unit = 32;
    state->display.bitmap_pad = 32;
    state->display.bitmap_bit_order = LSBFirst;
    state->display.nformats = 2;
    state->display.pixmap_format = state->pixmapFormats;
    state->display.max_request_size = 65535U;
    state->display.motion_buffer = X11_WASM_MOTION_BUFFER_SIZE;
    state->display.release = 1;
    state->display.display_name = (char *)"browser";
    state->display.default_screen = 0;
    state->display.nscreens = 1;
    state->display.screens = &state->screen;
    state->display.min_keycode = 8;
    state->display.max_keycode = 255;
    BrowserInitKeyboardMap(state);

    BrowserInitGC(&state->display, &state->defaultGC);
    state->screen.default_gc = &state->defaultGC;

    state->rootResource.id = state->rootWindow;
    state->rootResource.type = X11_WASM_RESOURCE_WINDOW;
    state->rootResource.display = &state->display;
    state->rootResource.width = (unsigned int)state->screen.width;
    state->rootResource.height = (unsigned int)state->screen.height;
    state->rootResource.depth = (unsigned int)state->screen.root_depth;
    state->rootResource.c_class = InputOutput;
    state->rootResource.mapped = 1;
    state->rootResource.atts.colormap = state->defaultColormap;
    state->rootResource.atts.bit_gravity = NorthWestGravity;
    state->rootResource.atts.win_gravity = NorthWestGravity;
    state->rootResource.atts.backing_store = NotUseful;
    state->rootResource.atts.backing_planes = ~0UL;
    state->rootResource.atts.event_mask = StructureNotifyMask
	    | ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask
	    | ButtonReleaseMask | PointerMotionMask;
    state->resourceList = &state->rootResource;

    browserDisplayInitialized = 1;
    (void)X11WasmJsInit(state->screen.width, state->screen.height,
	    BrowserJsXId(state->rootWindow));
    rootWidth = X11WasmJsGetRootWidth();
    rootHeight = X11WasmJsGetRootHeight();
    (void)BrowserResizeRoot((unsigned int)rootWidth,
	    (unsigned int)rootHeight, 0);
}

void
BrowserUpdateQueueLength(void)
{
    if (browserDisplayInitialized) {
	browserDisplay.display.qlen = eventQueueCount;
    }
}

int
BrowserWindowCanReceiveEvents(
    Window window)
{
    if (window == None) {
	return 0;
    }
    return BrowserWindowResource(window) != NULL;
}

Window
BrowserResolveEventWindow(
    Window window)
{
    Window focusWindow;

    BrowserInitDisplay();
    if (BrowserWindowCanReceiveEvents(window)) {
	return window;
    }
    focusWindow = BrowserGetFocusWindow();
    if (BrowserWindowCanReceiveEvents(focusWindow)) {
	return focusWindow;
    }
    if (BrowserWindowCanReceiveEvents(browserDisplay.focusWindow)) {
	return browserDisplay.focusWindow;
    }
    return browserDisplay.rootWindow;
}

Window
BrowserGetFocusWindow(void)
{
    BrowserInitDisplay();
    return browserDisplay.focusWindow;
}

Window
BrowserGetGrabWindow(void)
{
    BrowserInitDisplay();
    if (browserDisplay.pointerGrabWindow != None) {
	return browserDisplay.pointerGrabWindow;
    }
    return browserDisplay.keyboardGrabWindow;
}

Window
BrowserResolveKeyEventWindow(
    Window window)
{
    Window focusWindow = BrowserGetFocusWindow();
    Window target = None;
    Window grabWindow;
    int grabOwnerEvents = 1;

    BrowserInitDisplay();
    if (BrowserWindowCanReceiveEvents(focusWindow)) {
	target = focusWindow;
    } else if (BrowserWindowCanReceiveEvents((Window)window)) {
	target = (Window)window;
    } else {
	target = BrowserResolveEventWindow(window);
    }

    grabWindow = browserDisplay.keyboardGrabWindow;
    if (BrowserWindowCanReceiveEvents(grabWindow)) {
	grabOwnerEvents = browserDisplay.keyboardGrabOwnerEvents;
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
	target = BrowserResolveEventWindow(window);
    }
    return target;
}

void
BrowserSetCommonEventFields(
    XEvent *eventPtr,
    int type,
    Window window,
    Time time)
{
    memset(eventPtr, 0, sizeof(XEvent));
    eventPtr->xany.type = type;
    eventPtr->xany.serial = eventSerial++;
    eventPtr->xany.send_event = False;
    eventPtr->xany.display = &browserDisplay.display;
    eventPtr->xany.window = window;
    if (time != CurrentTime) {
	eventTime = time;
    } else {
	eventTime++;
    }
}

int
BrowserQueueEventInternal(
    XEvent *eventPtr,
    int internal)
{
    int tail;

    BrowserInitDisplay();
    if (eventQueueCount >= X11_WASM_EVENT_QUEUE_LIMIT) {
	return 0;
    }
    tail = (eventQueueHead + eventQueueCount) % X11_WASM_EVENT_QUEUE_LIMIT;
    eventQueue[tail].event = *eventPtr;
    eventQueue[tail].internal = internal;
    eventQueueCount++;
    BrowserUpdateQueueLength();
    return 1;
}

int
BrowserQueueEvent(
    XEvent *eventPtr)
{
    return BrowserQueueEventInternal(eventPtr, 0);
}

int
BrowserQueueEventWhenIdle(
    const XEvent *eventPtr)
{
    if (eventPtr == NULL) {
	return 0;
    }
    return BrowserQueueEvent((XEvent *)eventPtr);
}

int
BrowserQueueMappingNotify(
    int request,
    int firstKeycode,
    int count)
{
    XEvent event;

    BrowserSetCommonEventFields(&event, MappingNotify, None, CurrentTime);
    event.xmapping.request = request;
    event.xmapping.first_keycode = firstKeycode;
    event.xmapping.count = count;
    return BrowserQueueEvent(&event);
}

int
BrowserQueueNoExposeEvent(
    Drawable drawable,
    unsigned char requestCode)
{
    XEvent event;

    BrowserSetCommonEventFields(&event, NoExpose, (Window)drawable,
	    CurrentTime);
    event.xnoexpose.drawable = drawable;
    event.xnoexpose.major_code = requestCode;
    event.xnoexpose.minor_code = 0;
    return BrowserQueueEvent(&event);
}

int
BrowserQueueGraphicsExposeEvent(
    Drawable drawable,
    const XRectangle *rectPtr,
    int count,
    unsigned char requestCode)
{
    XEvent event;

    if ((rectPtr == NULL) || (rectPtr->width == 0)
	    || (rectPtr->height == 0)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, GraphicsExpose, (Window)drawable,
	    CurrentTime);
    event.xgraphicsexpose.drawable = drawable;
    event.xgraphicsexpose.x = rectPtr->x;
    event.xgraphicsexpose.y = rectPtr->y;
    event.xgraphicsexpose.width = rectPtr->width;
    event.xgraphicsexpose.height = rectPtr->height;
    event.xgraphicsexpose.count = count;
    event.xgraphicsexpose.major_code = requestCode;
    event.xgraphicsexpose.minor_code = 0;
    return BrowserQueueEvent(&event);
}

unsigned short
BrowserClampUShort(
    unsigned int value)
{
    return (value > 0xffffU) ? 0xffffU : (unsigned short)value;
}

short
BrowserClampShort(
    int value)
{
    if (value > SHRT_MAX) {
	return SHRT_MAX;
    }
    if (value < SHRT_MIN) {
	return SHRT_MIN;
    }
    return (short)value;
}

void
BrowserClearRectangle(
    XRectangle *rectPtr)
{
    if (rectPtr != NULL) {
	memset(rectPtr, 0, sizeof(XRectangle));
    }
}

void
BrowserSetRectangleBounds(
    XRectangle *rectPtr,
    int left,
    int top,
    int right,
    int bottom)
{
    if (rectPtr == NULL) {
	return;
    }
    if ((right <= left) || (bottom <= top)) {
	BrowserClearRectangle(rectPtr);
	return;
    }
    rectPtr->x = BrowserClampShort(left);
    rectPtr->y = BrowserClampShort(top);
    rectPtr->width = BrowserClampUShort((unsigned int)(right - left));
    rectPtr->height = BrowserClampUShort((unsigned int)(bottom - top));
}

int
BrowserIntersectRectangles(
    const XRectangle *aPtr,
    const XRectangle *bPtr,
    XRectangle *destPtr)
{
    int left, top, right, bottom;

    if ((aPtr == NULL) || (bPtr == NULL)
	    || (aPtr->width == 0) || (aPtr->height == 0)
	    || (bPtr->width == 0) || (bPtr->height == 0)) {
	BrowserClearRectangle(destPtr);
	return 0;
    }
    left = (aPtr->x > bPtr->x) ? aPtr->x : bPtr->x;
    top = (aPtr->y > bPtr->y) ? aPtr->y : bPtr->y;
    right = (aPtr->x + (int)aPtr->width < bPtr->x + (int)bPtr->width)
	    ? aPtr->x + (int)aPtr->width : bPtr->x + (int)bPtr->width;
    bottom = (aPtr->y + (int)aPtr->height < bPtr->y + (int)bPtr->height)
	    ? aPtr->y + (int)aPtr->height : bPtr->y + (int)bPtr->height;
    if ((right <= left) || (bottom <= top)) {
	BrowserClearRectangle(destPtr);
	return 0;
    }
    BrowserSetRectangleBounds(destPtr, left, top, right, bottom);
    return 1;
}

void
BrowserUnionRectangle(
    XRectangle *destPtr,
    int *emptyPtr,
    const XRectangle *sourcePtr)
{
    int left, top, right, bottom;

    if ((destPtr == NULL) || (emptyPtr == NULL) || (sourcePtr == NULL)
	    || (sourcePtr->width == 0) || (sourcePtr->height == 0)) {
	return;
    }
    if (*emptyPtr) {
	*destPtr = *sourcePtr;
	*emptyPtr = 0;
	return;
    }
    left = (destPtr->x < sourcePtr->x) ? destPtr->x : sourcePtr->x;
    top = (destPtr->y < sourcePtr->y) ? destPtr->y : sourcePtr->y;
    right = (destPtr->x + (int)destPtr->width
	    > sourcePtr->x + (int)sourcePtr->width)
	    ? destPtr->x + (int)destPtr->width
	    : sourcePtr->x + (int)sourcePtr->width;
    bottom = (destPtr->y + (int)destPtr->height
	    > sourcePtr->y + (int)sourcePtr->height)
	    ? destPtr->y + (int)destPtr->height
	    : sourcePtr->y + (int)sourcePtr->height;
    BrowserSetRectangleBounds(destPtr, left, top, right, bottom);
}

int
BrowserRectangleIsEmpty(
    const XRectangle *rectPtr)
{
    return (rectPtr == NULL) || (rectPtr->width == 0)
	    || (rectPtr->height == 0);
}

void
BrowserAppendRectangle(
    XRectangle **rectsPtr,
    int *countPtr,
    const XRectangle *rectPtr)
{
    XRectangle *newRects;
    int count;

    if ((rectsPtr == NULL) || (countPtr == NULL)
	    || BrowserRectangleIsEmpty(rectPtr)) {
	return;
    }
    count = *countPtr;
    newRects = (XRectangle *)ckalloc(sizeof(XRectangle) * (size_t)(count + 1));
    if ((count > 0) && (*rectsPtr != NULL)) {
	memcpy(newRects, *rectsPtr, sizeof(XRectangle) * (size_t)count);
	ckfree(*rectsPtr);
    }
    newRects[count] = *rectPtr;
    *rectsPtr = newRects;
    *countPtr = count + 1;
}

XRectangle *
BrowserCopyRectangleList(
    const XRectangle *rects,
    int count,
    int *countPtr,
    XRectangle *extentsPtr)
{
    XRectangle *copy = NULL;
    int copyCount = 0;
    int empty = 1;
    int i;

    BrowserClearRectangle(extentsPtr);
    for (i = 0; (rects != NULL) && (i < count); i++) {
	if (BrowserRectangleIsEmpty(&rects[i])) {
	    continue;
	}
	BrowserAppendRectangle(&copy, &copyCount, &rects[i]);
	BrowserUnionRectangle(extentsPtr, &empty, &rects[i]);
    }
    if (countPtr != NULL) {
	*countPtr = copyCount;
    }
    if (copyCount == 0) {
	BrowserClearRectangle(extentsPtr);
    }
    return copy;
}
