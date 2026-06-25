#include "XlibInt.h"

Display *
XOpenDisplay(
    _Xconst char *displayName)
{
    return X11WasmGetDisplay(displayName);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestOpenDisplayNames(void)
{
    if ((XOpenDisplay(NULL) != &browserDisplay.display)
	    || (XOpenDisplay("") != &browserDisplay.display)
	    || (XOpenDisplay("browser") != &browserDisplay.display)
	    || (XOpenDisplay(":0") != &browserDisplay.display)
	    || (XOpenDisplay(":0.0") != &browserDisplay.display)
	    || (XOpenDisplay("browser:0") != &browserDisplay.display)
	    || (XOpenDisplay("browser:0.0") != &browserDisplay.display)) {
	return 0;
    }
    if ((XOpenDisplay("definitely-invalid") != NULL)
	    || (XOpenDisplay(":1") != NULL)
	    || (XOpenDisplay("browser:0.1") != NULL)) {
	return 0;
    }
    return 1;
}

int
XCloseDisplay(
    Display *display)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserForgetIMInstantiateCallbacks(display);
    focusedInputContext = NULL;
    if (display == &browserDisplay.display) {
	BrowserResetHostAccessControl();
	browserDisplay.pointerGrabWindow = None;
	browserDisplay.pointerGrabConfineWindow = None;
	browserDisplay.pointerGrabCursor = None;
	browserDisplay.pointerGrabEventMask = 0;
	browserDisplay.pointerGrabOwnerEvents = 0;
	browserDisplay.pointerGrabPointerMode = GrabModeAsync;
	browserDisplay.pointerGrabKeyboardMode = GrabModeAsync;
	browserDisplay.keyboardGrabWindow = None;
	browserDisplay.keyboardGrabOwnerEvents = 0;
	browserDisplay.keyboardGrabPointerMode = GrabModeAsync;
	browserDisplay.keyboardGrabKeyboardMode = GrabModeAsync;
	browserDisplay.serverGrabDepth = 0;
	BrowserSyncServerGrabState();
	browserDisplay.synchronize = 0;
	browserDisplay.afterFunction = NULL;
	browserDisplay.runningAfterFunction = 0;
	BrowserSyncSynchronizeState(display);
	browserDisplay.closeSerial++;
	BrowserSyncPointerGrabCursor();
	BrowserSyncPointerGrabState();
	BrowserSyncKeyboardGrabState();
    }
    return XSync(display, False);
}

Window
XCreateWindow(
    Display *display,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int borderWidth,
    int depth,
    unsigned int clazz,
    Visual *visual,
    unsigned long valueMask,
    XSetWindowAttributes *attributes)
{
    X11WasmResource *parentPtr;
    X11WasmResource *resourcePtr;
    unsigned int windowDepth;
    unsigned int windowClass;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    parentPtr = BrowserWindowResourceOrError(&display, parent,
	    X11_WASM_XREQ_CREATE_WINDOW);
    if (parentPtr == NULL) {
	return None;
    }
    if (!BrowserValueOrError(&display, width, width > 0,
	    X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    if (!BrowserValueOrError(&display, height, height > 0,
	    X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    if (!BrowserWindowClassOrError(&display, clazz, parentPtr->c_class,
	    X11_WASM_XREQ_CREATE_WINDOW, &windowClass)) {
	return None;
    }
    if ((windowClass == InputOnly)
	    && !BrowserValidateInputOnlyCreateWindow(&display, parent, depth,
		    visual, borderWidth, valueMask,
		    X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    windowDepth = (unsigned int)((depth == CopyFromParent)
	    ? ((windowClass == InputOnly) ? 0U : parentPtr->depth)
	    : depth);
    if (!BrowserValidateWindowAttributeValues(&display, windowDepth,
	    valueMask, attributes, X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    resourcePtr = X11WasmCreateResource(display, X11_WASM_RESOURCE_WINDOW);
    resourcePtr->parent = parent;
    resourcePtr->x = x;
    resourcePtr->y = y;
    resourcePtr->width = width;
    resourcePtr->height = height;
    resourcePtr->borderWidth = borderWidth;
    resourcePtr->depth = windowDepth;
    resourcePtr->c_class = windowClass;
    BrowserApplyWindowAttributes(resourcePtr, valueMask, attributes);
    BrowserLinkChildAtTop(resourcePtr, parent);
    (void)X11WasmJsCreateWindow(BrowserJsXId(resourcePtr->id),
	    BrowserJsXId(parent), x, y, width, height, resourcePtr->depth,
	    resourcePtr->atts.event_mask, resourcePtr->c_class);
    BrowserSyncWindowEventMask(resourcePtr);
    BrowserSyncWindowColormap(resourcePtr);
    BrowserSyncWindowAttributes(resourcePtr);
    BrowserSyncWindowStyle(resourcePtr);
    (void)BrowserQueueCreateNotify(resourcePtr);
    return (Window)resourcePtr->id;
}

Window
XCreateSimpleWindow(
    Display *display,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int borderWidth,
    unsigned long border,
    unsigned long background)
{
    XSetWindowAttributes attributes;

    memset(&attributes, 0, sizeof(attributes));
    attributes.background_pixel = background;
    attributes.border_pixel = border;
    return XCreateWindow(display, parent, x, y, width, height, borderWidth,
	    CopyFromParent, InputOutput, (Visual *)CopyFromParent,
	    CWBackPixel | CWBorderPixel, &attributes);
}

int
XDestroyWindow(
    Display *display,
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_DESTROY_WINDOW);

    if (resourcePtr == NULL) {
	return Success;
    }
    (void)BrowserDestroyWindowResource(resourcePtr);
    return Success;
}

int
XDestroySubwindows(
    Display *display,
    Window window)
{
    X11WasmResource *parentPtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_DESTROY_SUBWINDOWS);

    if (parentPtr == NULL) {
	return Success;
    }
    while (parentPtr->firstChildPtr != NULL) {
	(void)BrowserDestroyWindowResource(parentPtr->firstChildPtr);
    }
    return Success;
}

int
BrowserDestroyWindowResource(
    X11WasmResource *resourcePtr)
{
    Window window;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (resourcePtr == &browserDisplay.rootResource)) {
	return Success;
    }
    while (resourcePtr->firstChildPtr != NULL) {
	(void)BrowserDestroyWindowResource(resourcePtr->firstChildPtr);
    }
    window = (Window)resourcePtr->id;
    (void)BrowserQueueDestroyNotify(resourcePtr);
    (void)X11WasmJsDestroyResource(BrowserJsXId(window));
    X11WasmDeleteResource(window);
    return Success;
}

int
XMapWindow(
    Display *display,
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_MAP_WINDOW);

    if (resourcePtr == NULL) {
	return Success;
    }
    if (resourcePtr->mapped) {
	return Success;
    }
    resourcePtr->mapped = 1;
    (void)X11WasmJsMapWindow(BrowserJsXId(window));
    if (BrowserWindowMetadataTarget(window) == window) {
	BrowserSetWindowWmState(window,
		BrowserInitialWmStateForMap(display, window));
    }
    (void)BrowserQueueMapNotify(resourcePtr);
    BrowserNotifyVisibilityTree(resourcePtr, VisibilityUnobscured);
    if (resourcePtr->c_class != InputOnly) {
	(void)BrowserQueueExpose(resourcePtr, 0, 0, resourcePtr->width,
		resourcePtr->height);
    }
    return Success;
}

int
XMapRaised(
    Display *display,
    Window window)
{
    int result = XMapWindow(display, window);

    if (result == Success) {
	(void)XRaiseWindow(display, window);
    }
    return result;
}

int
XMapSubwindows(
    Display *display,
    Window window)
{
    X11WasmResource *parentPtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_MAP_SUBWINDOWS);
    X11WasmResource *childPtr;

    if (parentPtr == NULL) {
	return Success;
    }
    for (childPtr = parentPtr->lastChildPtr; childPtr != NULL; ) {
	X11WasmResource *prevPtr = childPtr->prevSiblingPtr;

	if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
	    (void)XMapWindow(display, (Window)childPtr->id);
	}
	childPtr = prevPtr;
    }
    return Success;
}

int
BrowserUnmapWindowResource(
    X11WasmResource *resourcePtr,
    Bool fromConfigure)
{
    if (resourcePtr == NULL) {
	return Success;
    }
    if (!resourcePtr->mapped) {
	return Success;
    }
    resourcePtr->mapped = 0;
    (void)X11WasmJsUnmapWindow(BrowserJsXId(resourcePtr->id));
    (void)BrowserQueueUnmapNotify(resourcePtr, fromConfigure);
    return Success;
}

int
XUnmapWindow(
    Display *display,
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_UNMAP_WINDOW);

    return BrowserUnmapWindowResource(resourcePtr, False);
}

int
XUnmapSubwindows(
    Display *display,
    Window window)
{
    X11WasmResource *parentPtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_UNMAP_SUBWINDOWS);
    X11WasmResource *childPtr;

    if (parentPtr == NULL) {
	return Success;
    }
    for (childPtr = parentPtr->firstChildPtr; childPtr != NULL; ) {
	X11WasmResource *nextPtr = childPtr->nextSiblingPtr;

	if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
	    (void)BrowserUnmapWindowResource(childPtr, False);
	}
	childPtr = nextPtr;
    }
    return Success;
}

int
XMoveWindow(
    Display *display,
    Window window,
    int x,
    int y)
{
    XWindowChanges changes;
    changes.x = x;
    changes.y = y;
    return XConfigureWindow(display, window, CWX | CWY, &changes);
}

int
XResizeWindow(
    Display *display,
    Window window,
    unsigned int width,
    unsigned int height)
{
    XWindowChanges changes;
    changes.width = (int)width;
    changes.height = (int)height;
    return XConfigureWindow(display, window, CWWidth | CWHeight, &changes);
}

int
XMoveResizeWindow(
    Display *display,
    Window window,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    XWindowChanges changes;
    changes.x = x;
    changes.y = y;
    changes.width = (int)width;
    changes.height = (int)height;
    return XConfigureWindow(display, window, CWX | CWY | CWWidth | CWHeight,
	    &changes);
}

int
XConfigureWindow(
    Display *display,
    Window window,
    unsigned int valueMask,
    XWindowChanges *changes)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CONFIGURE_WINDOW);
    int sizeChanged;
    unsigned int oldWidth;
    unsigned int oldHeight;
    Window above = None;

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserValidateConfigureWindowValues(&display, resourcePtr,
	    valueMask, changes, X11_WASM_XREQ_CONFIGURE_WINDOW)) {
	return Success;
    }
    if (changes == NULL) {
	return Success;
    }
    oldWidth = resourcePtr->width;
    oldHeight = resourcePtr->height;
    sizeChanged = ((valueMask & CWWidth)
	    && (resourcePtr->width != (unsigned int)changes->width))
	    || ((valueMask & CWHeight)
	    && (resourcePtr->height != (unsigned int)changes->height));
    if (valueMask & CWX) resourcePtr->x = changes->x;
    if (valueMask & CWY) resourcePtr->y = changes->y;
    if (valueMask & CWWidth) resourcePtr->width = (unsigned int)changes->width;
    if (valueMask & CWHeight) resourcePtr->height = (unsigned int)changes->height;
    if (valueMask & CWBorderWidth) {
	resourcePtr->borderWidth = (unsigned int)changes->border_width;
    }
    if (valueMask & (CWSibling | CWStackMode)) {
	BrowserRestackWindow(resourcePtr, valueMask, changes);
	if (resourcePtr->prevSiblingPtr != NULL) {
	    above = (Window)resourcePtr->prevSiblingPtr->id;
	}
    }
    (void)X11WasmJsConfigureWindow(BrowserJsXId(window), resourcePtr->x,
	    resourcePtr->y, resourcePtr->width, resourcePtr->height,
	    resourcePtr->borderWidth, valueMask);
    (void)BrowserQueueConfigureNotify(resourcePtr, above);
    if (sizeChanged) {
	BrowserApplyChildWinGravity(resourcePtr, oldWidth, oldHeight);
    }
    if (sizeChanged && (resourcePtr->c_class != InputOnly)) {
	(void)BrowserQueueExpose(resourcePtr, 0, 0, resourcePtr->width,
		resourcePtr->height);
    }
    return Success;
}

int
XRestackWindows(
    Display *display,
    Window *windows,
    int nwindows)
{
    XWindowChanges changes;
    int i;

    if ((nwindows <= 0) || (windows == NULL)) {
	return Success;
    }
    if (BrowserWindowResourceOrError(&display, windows[0],
	    X11_WASM_XREQ_CONFIGURE_WINDOW) == NULL) {
	return Success;
    }
    memset(&changes, 0, sizeof(changes));
    changes.stack_mode = Below;
    for (i = 1; i < nwindows; i++) {
	changes.sibling = windows[i - 1];
	(void)XConfigureWindow(display, windows[i], CWSibling | CWStackMode,
		&changes);
    }
    return Success;
}

int
BrowserScreenOrError(
    Display *display,
    int screen,
    int requestCode)
{
    if ((screen < 0) || (screen >= ScreenCount(display))) {
	(void)BrowserReportXError(display, BadValue, (XID)screen,
		requestCode, 0);
	return 0;
    }
    return 1;
}

Status
XReconfigureWMWindow(
    Display *display,
    Window window,
    int screen,
    unsigned int valueMask,
    XWindowChanges *changes)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserScreenOrError(display, screen,
	    X11_WASM_XREQ_CONFIGURE_WINDOW)) {
	return False;
    }
    return (XConfigureWindow(display, window, valueMask, changes) == Success)
	    ? True : False;
}

int
XGetWindowAttributes(
    Display *display,
    Window window,
    XWindowAttributes *attributes)
{
    X11WasmResource *resourcePtr;

    if (attributes == NULL) {
	return False;
    }
    memset(attributes, 0, sizeof(XWindowAttributes));
    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_GET_WINDOW_ATTRIBUTES);
    if (resourcePtr == NULL) {
	return False;
    }
    attributes->x = resourcePtr->x;
    attributes->y = resourcePtr->y;
    attributes->width = (int)resourcePtr->width;
    attributes->height = (int)resourcePtr->height;
    attributes->border_width = (int)resourcePtr->borderWidth;
    attributes->depth = (int)resourcePtr->depth;
    attributes->visual = (resourcePtr->c_class == InputOnly)
	    ? NULL : DefaultVisual(display, DefaultScreen(display));
    attributes->root = RootWindow(display, DefaultScreen(display));
    attributes->c_class = resourcePtr->c_class;
    attributes->bit_gravity = resourcePtr->atts.bit_gravity;
    attributes->win_gravity = resourcePtr->atts.win_gravity;
    attributes->backing_store = resourcePtr->atts.backing_store;
    attributes->backing_planes = resourcePtr->atts.backing_planes;
    attributes->backing_pixel = resourcePtr->atts.backing_pixel;
    attributes->save_under = resourcePtr->atts.save_under;
    attributes->colormap = (resourcePtr->c_class == InputOnly) ? None
	    : ((resourcePtr->atts.colormap != None)
	    ? resourcePtr->atts.colormap
	    : DefaultColormap(display, DefaultScreen(display)));
    attributes->map_installed = True;
    attributes->map_state = BrowserResourceIsVisible(resourcePtr) ? IsViewable
	    : (resourcePtr->mapped ? IsUnviewable : IsUnmapped);
    attributes->all_event_masks = resourcePtr->atts.event_mask;
    attributes->your_event_mask = resourcePtr->atts.event_mask;
    attributes->do_not_propagate_mask =
	    resourcePtr->atts.do_not_propagate_mask;
    attributes->override_redirect = resourcePtr->atts.override_redirect;
    attributes->screen = DefaultScreenOfDisplay(display);
    return True;
}

Status
XGetGeometry(
    Display *display,
    Drawable drawable,
    Window *rootReturn,
    int *xReturn,
    int *yReturn,
    unsigned int *widthReturn,
    unsigned int *heightReturn,
    unsigned int *borderWidthReturn,
    unsigned int *depthReturn)
{
    X11WasmResource *resourcePtr;

    if (rootReturn != NULL) {
	*rootReturn = None;
    }
    if (xReturn != NULL) *xReturn = 0;
    if (yReturn != NULL) *yReturn = 0;
    if (widthReturn != NULL) *widthReturn = 0;
    if (heightReturn != NULL) *heightReturn = 0;
    if (borderWidthReturn != NULL) *borderWidthReturn = 0;
    if (depthReturn != NULL) *depthReturn = 0;

    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_GET_GEOMETRY);
    if (resourcePtr == NULL) {
	return False;
    }
    if (rootReturn != NULL) *rootReturn = RootWindow(display, DefaultScreen(display));
    if (xReturn != NULL) *xReturn = resourcePtr->x;
    if (yReturn != NULL) *yReturn = resourcePtr->y;
    if (widthReturn != NULL) *widthReturn = resourcePtr->width;
    if (heightReturn != NULL) *heightReturn = resourcePtr->height;
    if (borderWidthReturn != NULL) *borderWidthReturn = resourcePtr->borderWidth;
    if (depthReturn != NULL) *depthReturn = resourcePtr->depth;
    return True;
}

Bool
XTranslateCoordinates(
    Display *display,
    Window source,
    Window destination,
    int sourceX,
    int sourceY,
    int *destXReturn,
    int *destYReturn,
    Window *childReturn)
{
    X11WasmResource *resourcePtr;
    X11WasmResource *childPtr = NULL;
    int destX = sourceX;
    int destY = sourceY;

    if (destXReturn != NULL) *destXReturn = 0;
    if (destYReturn != NULL) *destYReturn = 0;
    if (childReturn != NULL) *childReturn = None;

    if (BrowserWindowResourceOrError(&display, source,
	    X11_WASM_XREQ_TRANSLATE_COORDINATES) == NULL) {
	return False;
    }
    resourcePtr = BrowserWindowResourceOrError(&display, destination,
	    X11_WASM_XREQ_TRANSLATE_COORDINATES);
    if (resourcePtr == NULL) {
	return False;
    }
    BrowserTranslateWindowCoordinates(source, destination, sourceX, sourceY,
	    &destX, &destY);
    childPtr = BrowserFindTopVisibleChildAt(resourcePtr, destX, destY);
    if (destXReturn != NULL) *destXReturn = destX;
    if (destYReturn != NULL) *destYReturn = destY;
    if (childReturn != NULL) {
	*childReturn = (childPtr != NULL) ? (Window)childPtr->id : None;
    }
    return True;
}

Status
XQueryTree(
    Display *display,
    Window window,
    Window *rootReturn,
    Window *parentReturn,
    Window **childrenReturn,
    unsigned int *nchildrenReturn)
{
    X11WasmResource *resourcePtr;
    X11WasmResource *childPtr;
    Window *children = NULL;
    unsigned int count = 0;
    unsigned int index = 0;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    resourcePtr = X11WasmFindResource(window);
    if (resourcePtr == NULL) {
	if (childrenReturn != NULL) *childrenReturn = NULL;
	if (nchildrenReturn != NULL) *nchildrenReturn = 0;
	(void)BrowserReportXError(display, BadWindow, (XID)window,
		X11_WASM_XREQ_QUERY_TREE, 0);
	return False;
    }
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
	    count++;
	}
    }
    if (count > 0) {
	children = (Window *)ckalloc(sizeof(Window) * count);
	for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
		childPtr = childPtr->nextSiblingPtr) {
	    if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
		children[index++] = (Window)childPtr->id;
	    }
	}
    }
    if (rootReturn != NULL) *rootReturn = RootWindow(display, DefaultScreen(display));
    if (parentReturn != NULL) {
	*parentReturn = (window == RootWindow(display, DefaultScreen(display)))
		? None : resourcePtr->parent;
    }
    if (childrenReturn != NULL) {
	*childrenReturn = children;
    } else if (children != NULL) {
	ckfree(children);
    }
    if (nchildrenReturn != NULL) *nchildrenReturn = count;
    return True;
}

Window
XRootWindow(
    Display *display,
    int screen)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    return RootWindow(display, screen);
}

Pixmap
XCreatePixmap(
    Display *display,
    Drawable drawable,
    unsigned int width,
    unsigned int height,
    unsigned int depth)
{
    X11WasmResource *drawablePtr;
    X11WasmResource *resourcePtr;

    drawablePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_CREATE_PIXMAP);
    if (drawablePtr == NULL) {
	return None;
    }
    if (!BrowserValueOrError(&display, width, width > 0,
	    X11_WASM_XREQ_CREATE_PIXMAP)) {
	return None;
    }
    if (!BrowserValueOrError(&display, height, height > 0,
	    X11_WASM_XREQ_CREATE_PIXMAP)) {
	return None;
    }
    if (!BrowserValueOrError(&display, depth,
	    (depth == 1U)
	    || (depth == (unsigned int)DefaultDepth(display,
		    DefaultScreen(display))), X11_WASM_XREQ_CREATE_PIXMAP)) {
	return None;
    }
    resourcePtr = X11WasmCreateResource(display, X11_WASM_RESOURCE_PIXMAP);
    resourcePtr->width = width;
    resourcePtr->height = height;
    resourcePtr->depth = depth;
    resourcePtr->atts.colormap = (drawablePtr->atts.colormap != None)
	    ? drawablePtr->atts.colormap : browserDisplay.defaultColormap;
    (void)X11WasmJsCreatePixmap(BrowserJsXId(resourcePtr->id),
	    BrowserJsXId(drawable), width, height, depth);
    return (Pixmap)resourcePtr->id;
}

int
XFreePixmap(
    Display *display,
    Pixmap pixmap)
{
    if (BrowserPixmapResourceOrError(&display, pixmap,
	    X11_WASM_XREQ_FREE_PIXMAP) == NULL) {
	return Success;
    }
    (void)X11WasmJsDestroyResource(BrowserJsXId(pixmap));
    X11WasmDeleteResource(pixmap);
    return Success;
}

Colormap
XCreateColormap(
    Display *display,
    Window window,
    Visual *visual,
    int alloc)
{
    X11WasmResource *resourcePtr;
    Visual *defaultVisual;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_CREATE_COLORMAP) == NULL) {
	return None;
    }
    defaultVisual = DefaultVisual(display, DefaultScreen(display));
    if (visual == NULL) {
	visual = defaultVisual;
    }
    if ((visual == NULL) || (visual->visualid != defaultVisual->visualid)) {
	(void)BrowserReportXError(display, BadMatch, (XID)window,
		X11_WASM_XREQ_CREATE_COLORMAP, 0);
	return None;
    }
    if ((alloc != AllocNone) && (alloc != AllocAll)) {
	(void)BrowserReportXError(display, BadValue, (XID)alloc,
		X11_WASM_XREQ_CREATE_COLORMAP, 0);
	return None;
    }
    resourcePtr = X11WasmCreateResource(display, X11_WASM_RESOURCE_COLORMAP);
    return (Colormap)resourcePtr->id;
}

int
XFreeColormap(
    Display *display,
    Colormap colormap)
{
    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_FREE_COLORMAP, 0)) {
	return Success;
    }
    if (colormap == browserDisplay.defaultColormap) {
	return Success;
    }
    X11WasmDeleteResource(colormap);
    return Success;
}

GC
XCreateGC(
    Display *display,
    Drawable drawable,
    unsigned long valueMask,
    XGCValues *values)
{
    X11WasmResource *resourcePtr;
    GC gc;

    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_CREATE_GC);
    if (resourcePtr == NULL) {
	return NULL;
    }
    if (!BrowserGCValueMaskOrError(&display, valueMask,
	    X11_WASM_XREQ_CREATE_GC)) {
	return NULL;
    }
    if (!BrowserValidateGCValueEnums(&display, valueMask, values,
	    X11_WASM_XREQ_CREATE_GC)) {
	return NULL;
    }
    if (!BrowserValidateGCValueReferences(&display, valueMask, values,
	    resourcePtr->depth, X11_WASM_XREQ_CREATE_GC)) {
	return NULL;
    }
    gc = (GC)ckalloc(sizeof(XGCValues));
    BrowserInitGC(display, gc);
    BrowserApplyGCValues(gc, valueMask, values);
    BrowserRegisterGC(gc, resourcePtr->depth);
    return gc;
}

int
XFreeGC(
    Display *display,
    GC gc)
{
    if (!BrowserUnregisterGC(gc)) {
	(void)BrowserReportXError(display, BadGC, (XID)(uintptr_t)gc,
		X11_WASM_XREQ_FREE_GC, 0);
	return Success;
    }
    BrowserDeleteGCClip(gc);
    BrowserDeleteGCDashes(gc);
    ckfree(gc);
    return Success;
}

int
XChangeGC(
    Display *display,
    GC gc,
    unsigned long mask,
    XGCValues *values)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (!BrowserGCValueMaskOrError(&display, mask,
	    X11_WASM_XREQ_CHANGE_GC)) {
	return Success;
    }
    if (!BrowserValidateGCValueEnums(&display, mask, values,
	    X11_WASM_XREQ_CHANGE_GC)) {
	return Success;
    }
    if (!BrowserValidateGCValueReferences(&display, mask, values,
	    BrowserGCDepth(gc), X11_WASM_XREQ_CHANGE_GC)) {
	return Success;
    }
    BrowserApplyGCValues(gc, mask, values);
    return Success;
}

int
XCopyGC(
    Display *display,
    GC src,
    unsigned long valuemask,
    GC dest)
{
    if (BrowserGCOrError(&display, src, X11_WASM_XREQ_COPY_GC) == NULL) {
	return Success;
    }
    if (BrowserGCOrError(&display, dest, X11_WASM_XREQ_COPY_GC) == NULL) {
	return Success;
    }
    if (!BrowserGCValueMaskOrError(&display, valuemask,
	    X11_WASM_XREQ_COPY_GC)) {
	return Success;
    }
    if (!BrowserGCDepthsMatchOrError(&display, src, dest,
	    X11_WASM_XREQ_COPY_GC)) {
	return Success;
    }
    BrowserCopyGCComponents(src, dest, valuemask);
    return Success;
}

int
XGetGCValues(
    Display *display,
    GC gc,
    unsigned long valuemask,
    XGCValues *valuesReturn)
{
    (void)display;
    if (!BrowserIsKnownGC(gc) || (valuesReturn == NULL)) {
	return False;
    }
    if (!BrowserGCGetValueMaskIsValid(valuemask)) {
	return False;
    }
    memset(valuesReturn, 0, sizeof(*valuesReturn));
    BrowserCopyGCValueFields(gc, valuesReturn, valuemask);
    return True;
}

int
XSetForeground(
    Display *display,
    GC gc,
    unsigned long foreground)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) != NULL) {
	gc->foreground = foreground;
    }
    return Success;
}

int
XSetBackground(
    Display *display,
    GC gc,
    unsigned long background)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) != NULL) {
	gc->background = background;
    }
    return Success;
}

int
XSetFont(
    Display *display,
    GC gc,
    Font font)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserFontResourceOrError(&display, font,
	    X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    gc->font = font;
    return Success;
}

int
XSetFunction(
    Display *display,
    GC gc,
    int function)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCFunction(&display, function,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->function = function;
    }
    return Success;
}

int
XSetFillStyle(
    Display *display,
    GC gc,
    int fillStyle)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCFillStyle(&display, fillStyle,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->fill_style = fillStyle;
    }
    return Success;
}

int
XSetFillRule(
    Display *display,
    GC gc,
    int fillRule)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCFillRule(&display, fillRule,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->fill_rule = fillRule;
    }
    return Success;
}

int
XSetArcMode(
    Display *display,
    GC gc,
    int arcMode)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCArcMode(&display, arcMode,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->arc_mode = arcMode;
    }
    return Success;
}

int
XSetTile(
    Display *display,
    GC gc,
    Pixmap tile)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (!BrowserValidateGCPixmapDepth(&display, tile, BrowserGCDepth(gc),
	    X11_WASM_XREQ_CHANGE_GC, 0)) {
	return Success;
    }
    gc->tile = tile;
    return Success;
}

int
XSetStipple(
    Display *display,
    GC gc,
    Pixmap stipple)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (!BrowserValidateGCPixmapDepth(&display, stipple, 1U,
	    X11_WASM_XREQ_CHANGE_GC, 0)) {
	return Success;
    }
    gc->stipple = stipple;
    return Success;
}

int XSetClipMask(Display *display, GC gc, Pixmap pixmap)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (!BrowserValidateGCPixmapDepth(&display, pixmap, 1U,
	    X11_WASM_XREQ_CHANGE_GC, 1)) {
	return Success;
    }
    gc->clip_mask = pixmap;
    BrowserSetGCClip(gc, 0, NULL);
    return Success;
}
int XSetClipOrigin(Display *display, GC gc, int x, int y)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) != NULL) {
	gc->clip_x_origin = x;
	gc->clip_y_origin = y;
    }
    return Success;
}
int XSetTSOrigin(Display *display, GC gc, int x, int y)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) != NULL) {
	gc->ts_x_origin = x;
	gc->ts_y_origin = y;
    }
    return Success;
}

int
XSetLineAttributes(
    Display *display,
    GC gc,
    unsigned int lineWidth,
    int lineStyle,
    int capStyle,
    int joinStyle)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCLineStyle(&display, lineStyle,
	    X11_WASM_XREQ_CHANGE_GC)
	    && BrowserValidateGCCapStyle(&display, capStyle,
		    X11_WASM_XREQ_CHANGE_GC)
	    && BrowserValidateGCJoinStyle(&display, joinStyle,
		    X11_WASM_XREQ_CHANGE_GC)) {
	gc->line_width = (int)lineWidth;
	gc->line_style = lineStyle;
	gc->cap_style = capStyle;
	gc->join_style = joinStyle;
    }
    return Success;
}

int
XSetDashes(
    Display *display,
    GC gc,
    int dashOffset,
    _Xconst char *dashList,
    int n)
{
    if ((BrowserGCOrError(&display, gc, X11_WASM_XREQ_SET_DASHES) != NULL)
	    && BrowserValidateGCDashList(&display, dashList, n,
		    X11_WASM_XREQ_SET_DASHES)) {
	gc->dash_offset = dashOffset;
	gc->dashes = dashList[0];
	BrowserSetGCDashes(gc, dashList, n);
    }
    return Success;
}

int
XSetClipRectangles(
    Display *display,
    GC gc,
    int clipXOrigin,
    int clipYOrigin,
    XRectangle rectangles[],
    int n,
    int ordering)
{
    if (BrowserGCOrError(&display, gc,
	    X11_WASM_XREQ_SET_CLIP_RECTANGLES) == NULL) {
	return Success;
    }
    if (!BrowserValidateGCClipRectangles(&display, rectangles, n, ordering,
	    X11_WASM_XREQ_SET_CLIP_RECTANGLES)) {
	return Success;
    }
    gc->clip_x_origin = clipXOrigin;
    gc->clip_y_origin = clipYOrigin;
    gc->clip_mask = None;
    BrowserSetGCClipRects(gc, 1, rectangles, n);
    return Success;
}

int
XSetRegion(
    Display *display,
    GC gc,
    Region region)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;

    if (BrowserGCOrError(&display, gc,
	    X11_WASM_XREQ_SET_CLIP_RECTANGLES) == NULL) {
	return Success;
    }
    gc->clip_x_origin = 0;
    gc->clip_y_origin = 0;
    gc->clip_mask = None;
    if ((regionPtr == NULL) || regionPtr->empty) {
	BrowserSetGCClipRects(gc, 1, NULL, 0);
	return Success;
    }
    BrowserSetGCClipRects(gc, 1, regionPtr->rects, regionPtr->rectCount);
    return Success;
}

Region
XCreateRegion(void)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)ckalloc(sizeof(X11WasmRegion));
    memset(regionPtr, 0, sizeof(X11WasmRegion));
    regionPtr->empty = 1;
    return (Region)regionPtr;
}

Region
XPolygonRegion(
    XPoint *points,
    int npoints,
    int fillRule)
{
    Region region = XCreateRegion();
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    BrowserPolygonIntersection *intersections;
    XRectangle *rects = NULL;
    int rectCount = 0;
    int minY, maxY;
    int i, y;

    if ((points == NULL) || (npoints < 3)
	    || (fillRule < EvenOddRule) || (fillRule > WindingRule)) {
	return region;
    }
    minY = maxY = points[0].y;
    for (i = 1; i < npoints; i++) {
	if (points[i].y < minY) {
	    minY = points[i].y;
	}
	if (points[i].y > maxY) {
	    maxY = points[i].y;
	}
    }
    if (minY == maxY) {
	return region;
    }
    intersections = (BrowserPolygonIntersection *)ckalloc(
	    sizeof(BrowserPolygonIntersection) * (size_t)npoints);
    for (y = minY; y < maxY; y++) {
	double scanY = (double)y + 0.5;
	int intersectionCount = 0;

	for (i = 0; i < npoints; i++) {
	    XPoint *aPtr = &points[i];
	    XPoint *bPtr = &points[(i + 1) % npoints];
	    int y1 = aPtr->y;
	    int y2 = bPtr->y;
	    int edgeTop, edgeBottom;
	    double t;

	    if (y1 == y2) {
		continue;
	    }
	    edgeTop = (y1 < y2) ? y1 : y2;
	    edgeBottom = (y1 > y2) ? y1 : y2;
	    if ((scanY < (double)edgeTop) || (scanY >= (double)edgeBottom)) {
		continue;
	    }
	    t = (scanY - (double)y1) / (double)(y2 - y1);
	    intersections[intersectionCount].x = (double)aPtr->x
		    + t * (double)(bPtr->x - aPtr->x);
	    intersections[intersectionCount].winding = (y2 > y1) ? 1 : -1;
	    intersectionCount++;
	}
	if (intersectionCount < 2) {
	    continue;
	}
	BrowserSortPolygonIntersections(intersections, intersectionCount);
	if (fillRule == EvenOddRule) {
	    for (i = 0; i + 1 < intersectionCount; i += 2) {
		BrowserAppendPolygonSpan(&rects, &rectCount,
			intersections[i].x, intersections[i + 1].x, y);
	    }
	} else {
	    int winding = 0;
	    double spanStart = 0.0;

	    for (i = 0; i < intersectionCount; i++) {
		int oldWinding = winding;

		winding += intersections[i].winding;
		if ((oldWinding == 0) && (winding != 0)) {
		    spanStart = intersections[i].x;
		} else if ((oldWinding != 0) && (winding == 0)) {
		    BrowserAppendPolygonSpan(&rects, &rectCount, spanStart,
			    intersections[i].x, y);
		}
	    }
	}
    }
    ckfree(intersections);
    BrowserRegionSetRectangles(regionPtr, rects, rectCount);
    if (rects != NULL) {
	ckfree(rects);
    }
    return region;
}

int
XDestroyRegion(
    Region region)
{
    if (region != NULL) {
	BrowserRegionClear((X11WasmRegion *)region);
	ckfree(region);
    }
    return Success;
}

Bool
XEmptyRegion(
    Region region)
{
    return (region == NULL) || ((X11WasmRegion *)region)->empty;
}

int
XClipBox(
    Region region,
    XRectangle *rectReturn)
{
    if (rectReturn != NULL) {
	if (region == NULL) {
	    memset(rectReturn, 0, sizeof(XRectangle));
	} else {
	    *rectReturn = ((X11WasmRegion *)region)->extents;
	}
    }
    return Success;
}

int
XRectInRegion(
    Region region,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    XRectangle rect;
    XRectangle intersection;
    int intersects = 0;
    int i;

    if ((regionPtr == NULL) || regionPtr->empty || (width == 0)
	    || (height == 0)) {
	return RectangleOut;
    }
    BrowserSetRectangleBounds(&rect, x, y, x + (int)width,
	    y + (int)height);
    if (!BrowserIntersectRectangles(&regionPtr->extents, &rect,
	    &intersection)) {
	return RectangleOut;
    }
    for (i = 0; i < regionPtr->rectCount; i++) {
	if (BrowserIntersectRectangles(&regionPtr->rects[i], &rect,
		&intersection)) {
	    intersects = 1;
	    break;
	}
    }
    if (!intersects) {
	return RectangleOut;
    }
    if (BrowserRectangleCoveredByList(&rect, regionPtr->rects,
	    regionPtr->rectCount)) {
	return RectangleIn;
    }
    return RectanglePart;
}

Bool
XPointInRegion(
    Region region,
    int x,
    int y)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    int i;

    if ((regionPtr == NULL) || regionPtr->empty) {
	return False;
    }
    if ((x < regionPtr->extents.x)
	    || (x >= regionPtr->extents.x + (int)regionPtr->extents.width)
	    || (y < regionPtr->extents.y)
	    || (y >= regionPtr->extents.y + (int)regionPtr->extents.height)) {
	return False;
    }
    for (i = 0; i < regionPtr->rectCount; i++) {
	const XRectangle *rectPtr = &regionPtr->rects[i];
	int right = rectPtr->x + (int)rectPtr->width;
	int bottom = rectPtr->y + (int)rectPtr->height;

	if ((x >= rectPtr->x) && (x < right)
		&& (y >= rectPtr->y) && (y < bottom)) {
	    return True;
	}
    }
    return False;
}

Bool
XEqualRegion(
    Region regionA,
    Region regionB)
{
    X11WasmRegion *aPtr = (X11WasmRegion *)regionA;
    X11WasmRegion *bPtr = (X11WasmRegion *)regionB;
    int i;

    if ((aPtr == NULL) || aPtr->empty) {
	return ((bPtr == NULL) || bPtr->empty) ? True : False;
    }
    if ((bPtr == NULL) || bPtr->empty) {
	return False;
    }
    if ((aPtr->extents.x != bPtr->extents.x)
	    || (aPtr->extents.y != bPtr->extents.y)
	    || (aPtr->extents.width != bPtr->extents.width)
	    || (aPtr->extents.height != bPtr->extents.height)) {
	return False;
    }
    for (i = 0; i < aPtr->rectCount; i++) {
	if (!BrowserRectangleCoveredByList(&aPtr->rects[i], bPtr->rects,
		bPtr->rectCount)) {
	    return False;
	}
    }
    for (i = 0; i < bPtr->rectCount; i++) {
	if (!BrowserRectangleCoveredByList(&bPtr->rects[i], aPtr->rects,
		aPtr->rectCount)) {
	    return False;
	}
    }
    return True;
}

int
XOffsetRegion(
    Region region,
    int dx,
    int dy)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    if ((regionPtr != NULL) && !regionPtr->empty) {
	int i;

	regionPtr->extents.x += (short)dx;
	regionPtr->extents.y += (short)dy;
	for (i = 0; i < regionPtr->rectCount; i++) {
	    regionPtr->rects[i].x += (short)dx;
	    regionPtr->rects[i].y += (short)dy;
	}
    }
    return Success;
}

int
XShrinkRegion(
    Region region,
    int dx,
    int dy)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    XRectangle *rects = NULL;
    int count = 0;
    int i;

    if ((regionPtr == NULL) || regionPtr->empty) {
	return Success;
    }
    for (i = 0; i < regionPtr->rectCount; i++) {
	XRectangle rect;
	const XRectangle *sourcePtr = &regionPtr->rects[i];
	int left = sourcePtr->x + dx;
	int top = sourcePtr->y + dy;
	int right = sourcePtr->x + (int)sourcePtr->width - dx;
	int bottom = sourcePtr->y + (int)sourcePtr->height - dy;

	BrowserSetRectangleBounds(&rect, left, top, right, bottom);
	BrowserAppendRectangle(&rects, &count, &rect);
    }
    BrowserRegionSetRectangles(regionPtr, rects, count);
    if (rects != NULL) {
	ckfree(rects);
    }
    return Success;
}

int
XUnionRectWithRegion(
    XRectangle *rectangle,
    Region source,
    Region dest)
{
    X11WasmRegion *sourcePtr = (X11WasmRegion *)source;
    X11WasmRegion *destPtr = (X11WasmRegion *)dest;

    if (destPtr == NULL) {
	return Success;
    }
    if (destPtr != sourcePtr) {
	if ((sourcePtr != NULL) && !sourcePtr->empty) {
	    BrowserRegionSetRectangles(destPtr, sourcePtr->rects,
		    sourcePtr->rectCount);
	} else {
	    BrowserRegionClear(destPtr);
	}
    }
    BrowserRegionAppendRectangle(destPtr, rectangle);
    return Success;
}

int
XUnionRegion(
    Region sourceA,
    Region sourceB,
    Region dest)
{
    X11WasmRegion *aPtr = (X11WasmRegion *)sourceA;
    X11WasmRegion *bPtr = (X11WasmRegion *)sourceB;
    X11WasmRegion *destPtr = (X11WasmRegion *)dest;
    XRectangle *rects = NULL;
    int count = 0;
    int i;

    if (destPtr == NULL) {
	return Success;
    }
    if ((aPtr != NULL) && !aPtr->empty) {
	for (i = 0; i < aPtr->rectCount; i++) {
	    BrowserAppendRectangle(&rects, &count, &aPtr->rects[i]);
	}
    }
    if ((bPtr != NULL) && !bPtr->empty) {
	for (i = 0; i < bPtr->rectCount; i++) {
	    BrowserAppendRectangle(&rects, &count, &bPtr->rects[i]);
	}
    }
    BrowserRegionSetRectangles(destPtr, rects, count);
    if (rects != NULL) {
	ckfree(rects);
    }
    return Success;
}

int
XIntersectRegion(
    Region sourceA,
    Region sourceB,
    Region dest)
{
    X11WasmRegion *aPtr = (X11WasmRegion *)sourceA;
    X11WasmRegion *bPtr = (X11WasmRegion *)sourceB;
    X11WasmRegion *destPtr = (X11WasmRegion *)dest;
    XRectangle *rects = NULL;
    int count = 0;
    int i, j;

    if (destPtr == NULL) {
	return Success;
    }
    if ((aPtr == NULL) || aPtr->empty || (bPtr == NULL) || bPtr->empty) {
	BrowserRegionClear(destPtr);
	return Success;
    }
    for (i = 0; i < aPtr->rectCount; i++) {
	for (j = 0; j < bPtr->rectCount; j++) {
	    XRectangle intersection;

	    if (BrowserIntersectRectangles(&aPtr->rects[i], &bPtr->rects[j],
		    &intersection)) {
		BrowserAppendRectangle(&rects, &count, &intersection);
	    }
	}
    }
    BrowserRegionSetRectangles(destPtr, rects, count);
    if (rects != NULL) {
	ckfree(rects);
    }
    return Success;
}

int
XSubtractRegion(
    Region sourceA,
    Region sourceB,
    Region dest)
{
    X11WasmRegion *aPtr = (X11WasmRegion *)sourceA;
    X11WasmRegion *bPtr = (X11WasmRegion *)sourceB;
    X11WasmRegion *destPtr = (X11WasmRegion *)dest;
    XRectangle *current = NULL;
    int currentCount = 0;
    int i, j;

    if (destPtr == NULL) {
	return Success;
    }
    if ((aPtr == NULL) || aPtr->empty) {
	BrowserRegionClear(destPtr);
	return Success;
    }
    current = BrowserCopyRectangleList(aPtr->rects, aPtr->rectCount,
	    &currentCount, NULL);
    for (i = 0; (bPtr != NULL) && !bPtr->empty && (i < bPtr->rectCount)
	    && (currentCount > 0); i++) {
	XRectangle *next = NULL;
	int nextCount = 0;

	for (j = 0; j < currentCount; j++) {
	    BrowserSubtractRectangle(&current[j], &bPtr->rects[i], &next,
		    &nextCount);
	}
	if (current != NULL) {
	    ckfree(current);
	}
	current = next;
	currentCount = nextCount;
    }
    BrowserRegionSetRectangles(destPtr, current, currentCount);
    if (current != NULL) {
	ckfree(current);
    }
    return Success;
}

int
XXorRegion(
    Region sourceA,
    Region sourceB,
    Region dest)
{
    Region onlyA;
    Region onlyB;

    if (dest == NULL) {
	return Success;
    }
    onlyA = XCreateRegion();
    onlyB = XCreateRegion();
    (void)XSubtractRegion(sourceA, sourceB, onlyA);
    (void)XSubtractRegion(sourceB, sourceA, onlyB);
    (void)XUnionRegion(onlyA, onlyB, dest);
    (void)XDestroyRegion(onlyA);
    (void)XDestroyRegion(onlyB);
    return Success;
}

XImage *
XCreateImage(
    Display *display,
    Visual *visual,
    unsigned int depth,
    int format,
    int offset,
    char *data,
    unsigned int width,
    unsigned int height,
    int bitmapPad,
    int bytesPerLine)
{
    XImage *imagePtr;
    int bitsPerPixel;

    (void)display;
    (void)visual;
    if (!BrowserImageFormatIsValid(format)) {
	return NULL;
    }
    imagePtr = (XImage *)ckalloc(sizeof(XImage));
    memset(imagePtr, 0, sizeof(XImage));
    bitsPerPixel = BrowserImageBitsPerPixel(depth);
    imagePtr->width = (int)width;
    imagePtr->height = (int)height;
    imagePtr->xoffset = offset;
    imagePtr->format = format;
    imagePtr->data = data;
    imagePtr->byte_order = LSBFirst;
    imagePtr->bitmap_unit = 32;
    imagePtr->bitmap_bit_order = LSBFirst;
    imagePtr->bitmap_pad = (bitmapPad != 0) ? bitmapPad : 32;
    imagePtr->depth = (int)depth;
    imagePtr->bits_per_pixel = bitsPerPixel;
    imagePtr->bytes_per_line = (bytesPerLine != 0) ? bytesPerLine
	    : BrowserImageBytesPerLine(width, bitsPerPixel,
		    imagePtr->bitmap_pad);
    if (imagePtr->bytes_per_line <= 0) {
	ckfree(imagePtr);
	return NULL;
    }
    BrowserSetImageMasks(imagePtr);
    BrowserInitImageFuncs(imagePtr);
    return imagePtr;
}

int
_XInitImageFuncPtrs(
    XImage *image)
{
    if (image != NULL) {
	BrowserInitImageFuncs(image);
    }
    return Success;
}

int
BrowserGetImageDrawableMatches(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    Drawable drawable,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    if ((resourcePtr->type == X11_WASM_RESOURCE_WINDOW)
	    && !BrowserResourceIsVisible(resourcePtr)) {
	return BrowserBadMatchOrError(displayPtr, (XID)drawable,
		X11_WASM_XREQ_GET_IMAGE);
    }
    if ((x < 0) || (y < 0)
	    || ((unsigned int)x > resourcePtr->width)
	    || ((unsigned int)y > resourcePtr->height)
	    || (width > resourcePtr->width - (unsigned int)x)
	    || (height > resourcePtr->height - (unsigned int)y)) {
	return BrowserBadMatchOrError(displayPtr, (XID)drawable,
		X11_WASM_XREQ_GET_IMAGE);
    }
    return 1;
}

int
BrowserPutImageDrawableMatches(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    Drawable drawable,
    XImage *image)
{
    if (image == NULL) {
	return 1;
    }
    if (image->format == XYBitmap) {
	if ((image->depth != 1) || (image->bits_per_pixel != 1)) {
	    return BrowserBadMatchOrError(displayPtr, (XID)drawable,
		    X11_WASM_XREQ_PUT_IMAGE);
	}
	return 1;
    }
    if ((image->format == ZPixmap) || (image->format == XYPixmap)) {
	if ((unsigned int)image->depth != resourcePtr->depth) {
	    return BrowserBadMatchOrError(displayPtr, (XID)drawable,
		    X11_WASM_XREQ_PUT_IMAGE);
	}
    }
    return 1;
}

XImage *
BrowserConvertXYBitmapForPut(
    X11WasmResource *resourcePtr,
    GC gc,
    XImage *image,
    int srcX,
    int srcY,
    unsigned int width,
    unsigned int height)
{
    XImage *convertedImage;
    int dataLength;
    unsigned long foreground;
    unsigned long background;
    unsigned int x, y;

    if ((image == NULL) || (image->format != XYBitmap)) {
	return image;
    }
    if (gc != NULL) {
	foreground = gc->foreground;
	background = gc->background;
    } else if ((resourcePtr != NULL) && (resourcePtr->depth == 1)) {
	foreground = 1UL;
	background = 0UL;
    } else {
	foreground = 0UL;
	background = 0xffffffUL;
    }
    convertedImage = XCreateImage(NULL, NULL, resourcePtr->depth, ZPixmap, 0,
	    NULL, width, height, 32, 0);
    if (convertedImage == NULL) {
	return NULL;
    }
    dataLength = convertedImage->bytes_per_line * convertedImage->height;
    if (dataLength <= 0) {
	(void)XDestroyImage(convertedImage);
	return NULL;
    }
    convertedImage->data = (char *)ckalloc((unsigned int)dataLength);
    memset(convertedImage->data, 0, (unsigned int)dataLength);
    convertedImage->obdata = (XPointer)convertedImage;
    for (y = 0; y < height; y++) {
	for (x = 0; x < width; x++) {
	    unsigned long pixel = BrowserImageGetPixel(image,
		    srcX + (int)x, srcY + (int)y) ? foreground : background;

	    (void)BrowserImagePutPixel(convertedImage, (int)x, (int)y, pixel);
	}
    }
    return convertedImage;
}

XImage *
XGetImage(
    Display *display,
    Drawable drawable,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned long planeMask,
    int format)
{
    X11WasmResource *resourcePtr;
    XImage *imagePtr;
    unsigned int depth;
    int dataLength;

    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_GET_IMAGE);
    if (resourcePtr == NULL) {
	return NULL;
    }
    if (!BrowserValueOrError(&display, format, BrowserImageFormatIsValid(format),
	    X11_WASM_XREQ_GET_IMAGE)) {
	return NULL;
    }
    if (!BrowserGetImageDrawableMatches(&display, resourcePtr, drawable,
	    x, y, width, height)) {
	return NULL;
    }
    (void)planeMask;
    depth = resourcePtr->depth;
    imagePtr = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)),
	    depth, format, 0, NULL, width, height, 32, 0);
    if (imagePtr != NULL) {
	dataLength = imagePtr->bytes_per_line * (int)height;
	imagePtr->data = (char *)ckalloc((unsigned int)dataLength);
	memset(imagePtr->data, 0, (unsigned int)dataLength);
	imagePtr->obdata = (XPointer)imagePtr;
	if (!BrowserReadPixmapBacking(resourcePtr, imagePtr, x, y, width,
		height)) {
	    (void)X11WasmJsGetImage(BrowserJsXId(drawable), x, y, width,
		    height, imagePtr->data, dataLength,
		    imagePtr->bytes_per_line, imagePtr->bits_per_pixel);
	}
	BrowserImageApplyPlaneMask(imagePtr, planeMask);
    }
    return imagePtr;
}

int
XPutImage(
    Display *display,
    Drawable drawable,
    GC gc,
    XImage *image,
    int srcX,
    int srcY,
    int destX,
    int destY,
    unsigned int width,
    unsigned int height)
{
    X11WasmResource *resourcePtr = BrowserDrawableResourceOrError(&display,
	    drawable, X11_WASM_XREQ_PUT_IMAGE);
    XImage *putImage = image;

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&display, gc, X11_WASM_XREQ_PUT_IMAGE)) {
	return Success;
    }
    if ((image != NULL) && !BrowserValueOrError(&display, image->format,
	    BrowserImageFormatIsValid(image->format),
	    X11_WASM_XREQ_PUT_IMAGE)) {
	return Success;
    }
    if (!BrowserPutImageDrawableMatches(&display, resourcePtr, drawable,
	    image)) {
	return Success;
    }
    BrowserApplyGCStyle(gc);
    BrowserApplyGCClip(gc);
    putImage = BrowserConvertXYBitmapForPut(resourcePtr, gc, image,
	    srcX, srcY, width, height);
    if (putImage == NULL) {
	return Success;
    }
    (void)X11WasmJsPutImage(BrowserJsXId(drawable),
	    (putImage != NULL) ? putImage->data : NULL,
	    BrowserImageDataLength(putImage), (putImage == image) ? srcX : 0,
	    (putImage == image) ? srcY : 0, destX, destY, width, height,
	    (putImage != NULL) ? putImage->bytes_per_line : 0,
	    (putImage != NULL) ? putImage->bits_per_pixel : 0);
    BrowserStorePixmapImage(resourcePtr, gc, putImage,
	    (putImage == image) ? srcX : 0, (putImage == image) ? srcY : 0,
	    destX, destY, width, height);
    BrowserStoreBitmapImage(resourcePtr, putImage, (putImage == image) ? srcX : 0,
	    (putImage == image) ? srcY : 0, destX, destY,
	    width, height);
    if ((putImage != NULL) && (putImage != image)) {
	(void)XDestroyImage(putImage);
    }
    BrowserMarkResourceDamage(resourcePtr, destX, destY, width, height);
    return Success;
}

Pixmap
XCreateBitmapFromData(
    Display *display,
    Drawable drawable,
    _Xconst char *data,
    unsigned int width,
    unsigned int height)
{
    Pixmap pixmap = XCreatePixmap(display, drawable, width, height, 1);
    XImage *imagePtr;

    if ((pixmap == None) || (data == NULL)) {
	return pixmap;
    }
    imagePtr = XCreateImage(display, DefaultVisual(display,
	    DefaultScreen(display)), 1, XYBitmap, 0, (char *)data, width,
	    height, 8, BrowserBitmapBytesPerLine(width));
    if (imagePtr == NULL) {
	return pixmap;
    }
    (void)XPutImage(display, pixmap, NULL, imagePtr, 0, 0, 0, 0, width,
	    height);
    (void)BrowserImageDestroy(imagePtr);
    return pixmap;
}
