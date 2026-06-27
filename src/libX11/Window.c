#include "XlibInt.h"

X11WasmGCClip *
BrowserFindGCClip(
    GC gc,
    int create)
{
    X11WasmGCClip *clipPtr;

    if (gc == NULL) {
	return NULL;
    }
    for (clipPtr = gcClipList; clipPtr != NULL; clipPtr = clipPtr->nextPtr) {
	if (clipPtr->gc == gc) {
	    return clipPtr;
	}
    }
    if (!create) {
	return NULL;
    }
    clipPtr = (X11WasmGCClip *)ckalloc(sizeof(X11WasmGCClip));
    memset(clipPtr, 0, sizeof(X11WasmGCClip));
    clipPtr->gc = gc;
    clipPtr->nextPtr = gcClipList;
    gcClipList = clipPtr;
    return clipPtr;
}

void
BrowserDeleteGCClip(
    GC gc)
{
    X11WasmGCClip **linkPtr = &gcClipList;

    while (*linkPtr != NULL) {
	X11WasmGCClip *clipPtr = *linkPtr;

	if (clipPtr->gc == gc) {
	    *linkPtr = clipPtr->nextPtr;
	    if (clipPtr->rects != NULL) {
		ckfree(clipPtr->rects);
	    }
	    ckfree(clipPtr);
	    return;
	}
	linkPtr = &clipPtr->nextPtr;
    }
}

X11WasmGCDashes *
BrowserFindGCDashes(
    GC gc,
    int create)
{
    X11WasmGCDashes *dashPtr;

    if (gc == NULL) {
	return NULL;
    }
    for (dashPtr = gcDashList; dashPtr != NULL; dashPtr = dashPtr->nextPtr) {
	if (dashPtr->gc == gc) {
	    return dashPtr;
	}
    }
    if (!create) {
	return NULL;
    }
    dashPtr = (X11WasmGCDashes *)ckalloc(sizeof(X11WasmGCDashes));
    memset(dashPtr, 0, sizeof(X11WasmGCDashes));
    dashPtr->gc = gc;
    dashPtr->nextPtr = gcDashList;
    gcDashList = dashPtr;
    return dashPtr;
}

void
BrowserDeleteGCDashes(
    GC gc)
{
    X11WasmGCDashes **linkPtr = &gcDashList;

    while (*linkPtr != NULL) {
	X11WasmGCDashes *dashPtr = *linkPtr;

	if (dashPtr->gc == gc) {
	    *linkPtr = dashPtr->nextPtr;
	    if (dashPtr->pattern != NULL) {
		ckfree(dashPtr->pattern);
	    }
	    ckfree(dashPtr);
	    return;
	}
	linkPtr = &dashPtr->nextPtr;
    }
}

void
BrowserSetGCDashes(
    GC gc,
    const char *dashList,
    int count)
{
    X11WasmGCDashes *dashPtr;
    unsigned char *pattern = NULL;
    int i;

    if (gc == NULL) {
	return;
    }
    if ((dashList == NULL) || (count <= 0)) {
	BrowserDeleteGCDashes(gc);
	return;
    }
    pattern = (unsigned char *)ckalloc((size_t)count);
    for (i = 0; i < count; i++) {
	pattern[i] = (unsigned char)dashList[i];
	if (pattern[i] == 0) {
	    pattern[i] = 1;
	}
    }
    dashPtr = BrowserFindGCDashes(gc, 1);
    if (dashPtr != NULL) {
	if (dashPtr->pattern != NULL) {
	    ckfree(dashPtr->pattern);
	}
	dashPtr->pattern = pattern;
	dashPtr->count = count;
    } else {
	ckfree(pattern);
    }
}

int
BrowserGetGCDashes(
    GC gc,
    const unsigned char **patternPtr,
    int *countPtr)
{
    X11WasmGCDashes *dashPtr = BrowserFindGCDashes(gc, 0);

    if ((dashPtr == NULL) || (dashPtr->count <= 0)
	    || (dashPtr->pattern == NULL)) {
	if (patternPtr != NULL) {
	    *patternPtr = NULL;
	}
	if (countPtr != NULL) {
	    *countPtr = 0;
	}
	return 0;
    }
    if (patternPtr != NULL) {
	*patternPtr = dashPtr->pattern;
    }
    if (countPtr != NULL) {
	*countPtr = dashPtr->count;
    }
    return 1;
}

void
BrowserSetGCClip(
    GC gc,
    int enabled,
    const XRectangle *rectPtr)
{
    BrowserSetGCClipRects(gc, enabled, rectPtr, (rectPtr != NULL) ? 1 : 0);
}

void
BrowserSetGCClipRects(
    GC gc,
    int enabled,
    const XRectangle *rects,
    int count)
{
    X11WasmGCClip *clipPtr;
    XRectangle extents;
    int rectCount = 0;
    XRectangle *copy = NULL;

    if (gc == NULL) {
	return;
    }
    if (!enabled) {
	clipPtr = BrowserFindGCClip(gc, 0);
	if (clipPtr != NULL) {
	    clipPtr->enabled = 0;
	    BrowserClearRectangle(&clipPtr->rect);
	    clipPtr->rectCount = 0;
	    if (clipPtr->rects != NULL) {
		ckfree(clipPtr->rects);
		clipPtr->rects = NULL;
	    }
	}
	return;
    }
    copy = BrowserCopyRectangleList(rects, count, &rectCount, &extents);
    clipPtr = BrowserFindGCClip(gc, 1);
    if (clipPtr != NULL) {
	clipPtr->enabled = 1;
	clipPtr->rect = extents;
	if (clipPtr->rects != NULL) {
	    ckfree(clipPtr->rects);
	}
	clipPtr->rects = copy;
	clipPtr->rectCount = rectCount;
    } else if (copy != NULL) {
	ckfree(copy);
    }
}

static int
BrowserDrawableHasVisibleInferiors(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *childPtr;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (resourcePtr->c_class == InputOnly)) {
	return 0;
    }
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	if ((childPtr->type == X11_WASM_RESOURCE_WINDOW)
		&& (childPtr->c_class != InputOnly)
		&& BrowserResourceIsVisible(childPtr)) {
	    return 1;
	}
    }
    return 0;
}

static int
BrowserDrawableClipsInferiors(
    GC gc,
    X11WasmResource *resourcePtr)
{
    if ((gc != NULL) && !BrowserIsKnownGC(gc)) {
	return 0;
    }
    if ((gc != NULL) && (gc->subwindow_mode != ClipByChildren)) {
	return 0;
    }
    return BrowserDrawableHasVisibleInferiors(resourcePtr);
}

static void
BrowserAppendOffsetGCClipRects(
    GC gc,
    X11WasmGCClip *clipPtr,
    XRectangle **rectsPtr,
    int *countPtr)
{
    int i;
    int xOrigin = (gc != NULL) ? gc->clip_x_origin : 0;
    int yOrigin = (gc != NULL) ? gc->clip_y_origin : 0;

    if ((clipPtr == NULL) || !clipPtr->enabled) {
	return;
    }
    for (i = 0; i < clipPtr->rectCount; i++) {
	XRectangle rect;
	const XRectangle *sourcePtr = &clipPtr->rects[i];

	BrowserSetRectangleBounds(&rect,
		(int)sourcePtr->x + xOrigin,
		(int)sourcePtr->y + yOrigin,
		(int)sourcePtr->x + xOrigin + (int)sourcePtr->width,
		(int)sourcePtr->y + yOrigin + (int)sourcePtr->height);
	BrowserAppendRectangle(rectsPtr, countPtr, &rect);
    }
}

static void
BrowserSubtractInferiorClipRects(
    X11WasmResource *resourcePtr,
    XRectangle **rectsPtr,
    int *countPtr)
{
    X11WasmResource *childPtr;

    if ((resourcePtr == NULL) || (rectsPtr == NULL) || (countPtr == NULL)) {
	return;
    }
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	XRectangle childRect;
	XRectangle *remainingRects = NULL;
	int remainingCount = 0;
	int i;

	if ((childPtr->type != X11_WASM_RESOURCE_WINDOW)
		|| (childPtr->c_class == InputOnly)
		|| !BrowserResourceIsVisible(childPtr)) {
	    continue;
	}
	BrowserSetRectangleBounds(&childRect, childPtr->x, childPtr->y,
		childPtr->x + (int)childPtr->width,
		childPtr->y + (int)childPtr->height);
	if (BrowserRectangleIsEmpty(&childRect)) {
	    continue;
	}
	for (i = 0; i < *countPtr; i++) {
	    BrowserSubtractRectangle(&(*rectsPtr)[i], &childRect,
		    &remainingRects, &remainingCount);
	}
	if (*rectsPtr != NULL) {
	    ckfree(*rectsPtr);
	}
	*rectsPtr = remainingRects;
	*countPtr = remainingCount;
	if (remainingCount == 0) {
	    return;
	}
    }
}

void
BrowserApplyGCClip(
    GC gc,
    Drawable drawable)
{
    X11WasmGCClip *clipPtr = BrowserFindGCClip(gc, 0);
    X11WasmResource *resourcePtr = X11WasmFindResource(drawable);

    if (BrowserDrawableClipsInferiors(gc, resourcePtr)) {
	XRectangle *clipRects = NULL;
	int clipRectCount = 0;

	if ((clipPtr != NULL) && clipPtr->enabled) {
	    BrowserAppendOffsetGCClipRects(gc, clipPtr, &clipRects,
		    &clipRectCount);
	} else {
	    XRectangle drawableRect;

	    BrowserSetRectangleBounds(&drawableRect, 0, 0,
		    (int)resourcePtr->width, (int)resourcePtr->height);
	    BrowserAppendRectangle(&clipRects, &clipRectCount, &drawableRect);
	}
	BrowserSubtractInferiorClipRects(resourcePtr, &clipRects,
		&clipRectCount);
	(void)X11WasmJsSetClipRects(1, clipRects, clipRectCount, 0, 0);
	if (clipRects != NULL) {
	    ckfree(clipRects);
	}
    } else if ((clipPtr != NULL) && clipPtr->enabled) {
	(void)X11WasmJsSetClipRects(1, clipPtr->rects, clipPtr->rectCount,
		(gc != NULL) ? gc->clip_x_origin : 0,
		(gc != NULL) ? gc->clip_y_origin : 0);
    } else {
	(void)X11WasmJsSetClipRect(0, 0, 0, 0, 0);
    }
    if ((gc != NULL) && (gc->clip_mask != None)) {
	(void)X11WasmJsSetClipMask(1, BrowserJsXId(gc->clip_mask),
		gc->clip_x_origin, gc->clip_y_origin);
    } else {
	(void)X11WasmJsSetClipMask(0, 0, 0, 0);
    }
}

void
BrowserApplyGCStyle(
    GC gc)
{
    const unsigned char *dashPattern = NULL;
    int dashCount = 0;
    X11WasmResource *fontPtr = NULL;
    int fontPixelSize = 14;
    const char *fontFamily = "sans-serif";

    if (gc == NULL) {
	(void)X11WasmJsSetDrawStyle(GXcopy, 1, LineSolid, CapButt,
		JoinMiter, 0, 4, NULL, 0, FillSolid, None, None, 0, 0,
		0xffffff, fontPixelSize, fontFamily, AllPlanes);
	return;
    }
    fontPtr = X11WasmFindResource(gc->font);
    if ((fontPtr != NULL) && (fontPtr->type == X11_WASM_RESOURCE_FONT)) {
	if (fontPtr->fontPixelSize > 0) {
	    fontPixelSize = fontPtr->fontPixelSize;
	}
	if (fontPtr->fontFamily != NULL) {
	    fontFamily = fontPtr->fontFamily;
	}
    }
    (void)BrowserGetGCDashes(gc, &dashPattern, &dashCount);
    (void)X11WasmJsSetDrawStyle(gc->function, BrowserGCLineWidth(gc),
	    gc->line_style, gc->cap_style, gc->join_style,
	    gc->dash_offset, (unsigned int)((unsigned char)gc->dashes),
	    dashPattern, dashCount, gc->fill_style, BrowserJsXId(gc->stipple),
	    BrowserJsXId(gc->tile), gc->ts_x_origin, gc->ts_y_origin,
	    gc->background, fontPixelSize, fontFamily, gc->plane_mask);
}

int
BrowserNormalizeCopyArea(
    X11WasmResource *sourcePtr,
    X11WasmResource *targetPtr,
    int *srcXPtr,
    int *srcYPtr,
    unsigned int *widthPtr,
    unsigned int *heightPtr,
    int *destXPtr,
    int *destYPtr)
{
    long long srcX, srcY, destX, destY, width, height;
    long long sourceWidth, sourceHeight, targetWidth, targetHeight;
    long long amount;

    if ((sourcePtr == NULL) || (targetPtr == NULL) || (srcXPtr == NULL)
	    || (srcYPtr == NULL) || (widthPtr == NULL)
	    || (heightPtr == NULL) || (destXPtr == NULL)
	    || (destYPtr == NULL)) {
	return 0;
    }
    srcX = *srcXPtr;
    srcY = *srcYPtr;
    destX = *destXPtr;
    destY = *destYPtr;
    width = *widthPtr;
    height = *heightPtr;
    sourceWidth = sourcePtr->width;
    sourceHeight = sourcePtr->height;
    targetWidth = targetPtr->width;
    targetHeight = targetPtr->height;
    if ((width <= 0) || (height <= 0) || (sourceWidth <= 0)
	    || (sourceHeight <= 0) || (targetWidth <= 0)
	    || (targetHeight <= 0)) {
	return 0;
    }

    if (destX < 0) {
	amount = -destX;
	if (amount >= width) {
	    return 0;
	}
	srcX += amount;
	width -= amount;
	destX = 0;
    }
    if (destY < 0) {
	amount = -destY;
	if (amount >= height) {
	    return 0;
	}
	srcY += amount;
	height -= amount;
	destY = 0;
    }
    if (srcX < 0) {
	amount = -srcX;
	if (amount >= width) {
	    return 0;
	}
	destX += amount;
	width -= amount;
	srcX = 0;
    }
    if (srcY < 0) {
	amount = -srcY;
	if (amount >= height) {
	    return 0;
	}
	destY += amount;
	height -= amount;
	srcY = 0;
    }
    if ((srcX >= sourceWidth) || (srcY >= sourceHeight)
	    || (destX >= targetWidth) || (destY >= targetHeight)) {
	return 0;
    }
    if (width > sourceWidth - srcX) {
	width = sourceWidth - srcX;
    }
    if (height > sourceHeight - srcY) {
	height = sourceHeight - srcY;
    }
    if (width > targetWidth - destX) {
	width = targetWidth - destX;
    }
    if (height > targetHeight - destY) {
	height = targetHeight - destY;
    }
    if ((width <= 0) || (height <= 0) || (srcX > INT_MAX)
	    || (srcY > INT_MAX) || (destX > INT_MAX) || (destY > INT_MAX)
	    || (width > UINT_MAX) || (height > UINT_MAX)) {
	return 0;
    }

    *srcXPtr = (int)srcX;
    *srcYPtr = (int)srcY;
    *destXPtr = (int)destX;
    *destYPtr = (int)destY;
    *widthPtr = (unsigned int)width;
    *heightPtr = (unsigned int)height;
    return 1;
}

int
BrowserGCClipContains(
    GC gc,
    int x,
    int y)
{
    X11WasmGCClip *clipPtr = BrowserFindGCClip(gc, 0);
    int i;

    if ((clipPtr == NULL) || !clipPtr->enabled) {
	return 1;
    }
    for (i = 0; i < clipPtr->rectCount; i++) {
	long left = (long)clipPtr->rects[i].x
		+ ((gc != NULL) ? gc->clip_x_origin : 0);
	long top = (long)clipPtr->rects[i].y
		+ ((gc != NULL) ? gc->clip_y_origin : 0);
	long right = left + clipPtr->rects[i].width;
	long bottom = top + clipPtr->rects[i].height;

	if (((long)x >= left) && ((long)y >= top)
		&& ((long)x < right) && ((long)y < bottom)) {
	    return 1;
	}
    }
    return 0;
}

int
BrowserGCClipMaskContains(
    GC gc,
    int x,
    int y)
{
    X11WasmResource *maskPtr;
    int maskX, maskY;

    if ((gc == NULL) || (gc->clip_mask == None)) {
	return 1;
    }
    maskPtr = X11WasmFindResource(gc->clip_mask);
    if ((maskPtr == NULL) || (maskPtr->type != X11_WASM_RESOURCE_PIXMAP)
	    || (maskPtr->width == 0)
	    || (maskPtr->height == 0)) {
	return 0;
    }
    maskX = x - gc->clip_x_origin;
    maskY = y - gc->clip_y_origin;
    if ((maskX < 0) || (maskY < 0)
	    || ((unsigned int)maskX >= maskPtr->width)
	    || ((unsigned int)maskY >= maskPtr->height)) {
	return 0;
    }
    if (maskPtr->bitmapData != NULL) {
	return BrowserBitmapGetPixel(maskPtr, (unsigned int)maskX,
		(unsigned int)maskY);
    }
    if (maskPtr->backingData != NULL) {
	XImage backingImage;

	if (BrowserInitPixmapBackingImage(maskPtr, &backingImage)) {
	    return (BrowserImageGetPixel(&backingImage, maskX, maskY) & 1UL)
		    != 0;
	}
    }
    return 0;
}

int
BrowserCopyTargetContains(
    GC gc,
    int x,
    int y)
{
    return BrowserGCClipContains(gc, x, y)
	    && BrowserGCClipMaskContains(gc, x, y);
}

void
BrowserStorePixmapCopyImage(
    X11WasmResource *targetPtr,
    GC gc,
    XImage *sourceImage,
    int destX,
    int destY,
    unsigned int width,
    unsigned int height,
    int copyPlane,
    unsigned long foreground,
    unsigned long background,
    unsigned long plane,
    int transparentZero)
{
    XImage backingImage;
    unsigned int ix, iy;
    int function = (gc != NULL) ? gc->function : GXcopy;
    unsigned long planeMask = (gc != NULL) ? gc->plane_mask : AllPlanes;

    if ((targetPtr == NULL) || (targetPtr->type != X11_WASM_RESOURCE_PIXMAP)
	    || (sourceImage == NULL)
	    || !BrowserEnsurePixmapBacking(targetPtr)
	    || !BrowserInitPixmapBackingImage(targetPtr, &backingImage)) {
	return;
    }
    if (targetPtr->depth == 1) {
	(void)BrowserEnsureBitmapStorage(targetPtr);
    }
    for (iy = 0; iy < height; iy++) {
	for (ix = 0; ix < width; ix++) {
	    int targetX = destX + (int)ix;
	    int targetY = destY + (int)iy;
	    unsigned long sourcePixel;
	    unsigned long source;
	    unsigned long dest;
	    unsigned long result;

	    if ((targetX < 0) || (targetY < 0)
		    || ((unsigned int)targetX >= targetPtr->width)
		    || ((unsigned int)targetY >= targetPtr->height)
		    || !BrowserCopyTargetContains(gc, targetX, targetY)) {
		continue;
	    }
	    sourcePixel = BrowserImageGetPixel(sourceImage, (int)ix, (int)iy);
	    if (copyPlane) {
		int bit = (sourcePixel & plane) != 0;

		if (!bit && transparentZero) {
		    continue;
		}
		source = bit ? foreground : background;
	    } else {
		source = sourcePixel;
	    }
	    dest = BrowserImageGetPixel(&backingImage, targetX, targetY);
	    result = BrowserApplyPlaneMaskedRasterFunction(function, source,
		    dest, targetPtr->depth, planeMask);
	    (void)BrowserImagePutPixel(&backingImage, targetX, targetY,
		    result);
	    if ((targetPtr->depth == 1) && (targetPtr->bitmapData != NULL)) {
		BrowserBitmapPutPixel(targetPtr, (unsigned int)targetX,
			(unsigned int)targetY, (result & 1UL) != 0);
	    }
	}
    }
}

void
BrowserStorePixmapCopy(
    Display *display,
    Drawable source,
    X11WasmResource *sourcePtr,
    X11WasmResource *targetPtr,
    GC gc,
    int srcX,
    int srcY,
    unsigned int width,
    unsigned int height,
    int destX,
    int destY,
    int copyPlane,
    unsigned long plane,
    int transparentZero)
{
    XImage *sourceImage;
    unsigned long foreground = (gc != NULL) ? gc->foreground : 0;
    unsigned long background = (gc != NULL) ? gc->background : 0xffffffUL;

    if ((targetPtr == NULL) || (targetPtr->type != X11_WASM_RESOURCE_PIXMAP)
	    || (sourcePtr == NULL)) {
	return;
    }
    if (!copyPlane && (sourcePtr->depth != targetPtr->depth)) {
	return;
    }
    if ((sourcePtr->type == X11_WASM_RESOURCE_WINDOW)
	    && !BrowserResourceIsVisible(sourcePtr)) {
	return;
    }
    if (!BrowserNormalizeCopyArea(sourcePtr, targetPtr, &srcX, &srcY,
	    &width, &height, &destX, &destY)) {
	return;
    }
    sourceImage = XGetImage(display, source, srcX, srcY, width, height,
	    AllPlanes, ZPixmap);
    if (sourceImage == NULL) {
	return;
    }
    BrowserStorePixmapCopyImage(targetPtr, gc, sourceImage, destX, destY,
	    width, height, copyPlane, foreground, background, plane,
	    transparentZero);
    (void)XDestroyImage(sourceImage);
}

int
BrowserResourceWantsEvent(
    X11WasmResource *resourcePtr,
    long mask)
{
    return (resourcePtr != NULL) && ((resourcePtr->atts.event_mask & mask) != 0);
}

int
BrowserValidateWindowPixmapAttribute(
    Display **displayPtr,
    unsigned int windowDepth,
    Pixmap pixmap,
    unsigned char requestCode,
    int isBackground)
{
    X11WasmResource *pixmapPtr;

    if (isBackground) {
	if ((pixmap == None) || (pixmap == (Pixmap)ParentRelative)) {
	    return 1;
	}
    } else if (pixmap == (Pixmap)CopyFromParent) {
	return 1;
    }
    pixmapPtr = BrowserPixmapResourceOrError(displayPtr, pixmap, requestCode);
    if (pixmapPtr == NULL) {
	return 0;
    }
    if (pixmapPtr->depth != windowDepth) {
	Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

	if (display == NULL) {
	    display = X11WasmGetDisplay(NULL);
	    if (displayPtr != NULL) {
		*displayPtr = display;
	    }
	}
	(void)BrowserReportXError(display, BadMatch, (XID)pixmap,
		requestCode, 0);
	return 0;
    }
    return 1;
}

int
BrowserWindowClassOrError(
    Display **displayPtr,
    unsigned int clazz,
    unsigned int parentClass,
    unsigned char requestCode,
    unsigned int *classPtr)
{
    unsigned int windowClass = (clazz == CopyFromParent) ? parentClass : clazz;

    if ((windowClass != InputOutput) && (windowClass != InputOnly)) {
	return BrowserValueOrError(displayPtr, (long)clazz, 0, requestCode);
    }
    if (classPtr != NULL) {
	*classPtr = windowClass;
    }
    return 1;
}

int
BrowserValidateInputOnlyCreateWindow(
    Display **displayPtr,
    Window parent,
    int depth,
    Visual *visual,
    unsigned int borderWidth,
    unsigned long valueMask,
    unsigned char requestCode)
{
    unsigned long invalidMask =
	    valueMask & ~((unsigned long)X11_WASM_INPUT_ONLY_WINDOW_ATTR_MASK);

    (void)visual;
    if ((depth != CopyFromParent) || (borderWidth != 0U)
	    || (invalidMask != 0UL)) {
	return BrowserBadMatchOrError(displayPtr, (XID)parent, requestCode);
    }
    return 1;
}

int
BrowserValidateInputOutputCreateWindow(
    Display **displayPtr,
    X11WasmResource *parentPtr,
    int depth,
    Visual *visual,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;
    Visual *defaultVisual;
    int defaultDepth;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    if ((parentPtr == NULL) || (parentPtr->c_class != InputOutput)) {
	return BrowserBadMatchOrError(displayPtr,
		(parentPtr != NULL) ? (XID)parentPtr->id : None, requestCode);
    }
    defaultVisual = DefaultVisual(display, DefaultScreen(display));
    defaultDepth = DefaultDepth(display, DefaultScreen(display));
    if ((depth != CopyFromParent) && (depth != defaultDepth)) {
	return BrowserBadMatchOrError(displayPtr, (XID)parentPtr->id,
		requestCode);
    }
    if ((visual != (Visual *)CopyFromParent)
	    && ((visual == NULL) || (defaultVisual == NULL)
	    || (visual->visualid != defaultVisual->visualid))) {
	return BrowserBadMatchOrError(displayPtr, (XID)parentPtr->id,
		requestCode);
    }
    return 1;
}

int
BrowserValidateInputOnlyChangeWindowAttributes(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    unsigned long valueMask,
    unsigned char requestCode)
{
    unsigned long invalidMask =
	    valueMask & ~((unsigned long)X11_WASM_INPUT_ONLY_WINDOW_ATTR_MASK);

    if ((resourcePtr != NULL) && (resourcePtr->c_class == InputOnly)
	    && (invalidMask != 0UL)) {
	return BrowserBadMatchOrError(displayPtr, (XID)resourcePtr->id,
		requestCode);
    }
    return 1;
}

int
BrowserValidateWindowEventMask(
    Display **displayPtr,
    long eventMask,
    unsigned long validMask,
    unsigned char requestCode)
{
    unsigned long invalidMask = ((unsigned long)eventMask) & ~validMask;

    return BrowserValueOrError(displayPtr, eventMask, invalidMask == 0UL,
	    requestCode);
}

int
BrowserValidateWindowAttributeValues(
    Display **displayPtr,
    unsigned int windowDepth,
    unsigned long mask,
    XSetWindowAttributes *atts,
    unsigned char requestCode)
{
    unsigned long invalidMask = mask & ~((unsigned long)X11_WASM_WINDOW_ATTR_MASK);

    if (invalidMask != 0UL) {
	return BrowserValueOrError(displayPtr, (long)invalidMask, 0,
		requestCode);
    }
    if (atts == NULL) {
	return 1;
    }
    if ((mask & CWBackPixmap)
	    && !BrowserValidateWindowPixmapAttribute(displayPtr, windowDepth,
		    atts->background_pixmap, requestCode, 1)) {
	return 0;
    }
    if ((mask & CWBorderPixmap)
	    && !BrowserValidateWindowPixmapAttribute(displayPtr, windowDepth,
		    atts->border_pixmap, requestCode, 0)) {
	return 0;
    }
    if ((mask & CWBitGravity)
	    && !BrowserValueOrError(displayPtr, atts->bit_gravity,
		    (atts->bit_gravity >= ForgetGravity)
		    && (atts->bit_gravity <= StaticGravity), requestCode)) {
	return 0;
    }
    if ((mask & CWWinGravity)
	    && !BrowserValueOrError(displayPtr, atts->win_gravity,
		    (atts->win_gravity >= UnmapGravity)
		    && (atts->win_gravity <= StaticGravity), requestCode)) {
	return 0;
    }
    if ((mask & CWBackingStore)
	    && !BrowserValueOrError(displayPtr, atts->backing_store,
		    (atts->backing_store >= NotUseful)
		    && (atts->backing_store <= Always), requestCode)) {
	return 0;
    }
    if ((mask & CWOverrideRedirect)
	    && !BrowserValueOrError(displayPtr, atts->override_redirect,
		    (atts->override_redirect == False)
		    || (atts->override_redirect == True), requestCode)) {
	return 0;
    }
    if ((mask & CWSaveUnder)
	    && !BrowserValueOrError(displayPtr, atts->save_under,
		    (atts->save_under == False) || (atts->save_under == True),
		    requestCode)) {
	return 0;
    }
    if ((mask & CWEventMask)
	    && !BrowserValidateWindowEventMask(displayPtr, atts->event_mask,
		    (unsigned long)X11_WASM_WINDOW_EVENT_MASK, requestCode)) {
	return 0;
    }
    if ((mask & CWDontPropagate)
	    && !BrowserValidateWindowEventMask(displayPtr,
		    atts->do_not_propagate_mask,
		    (unsigned long)X11_WASM_DO_NOT_PROPAGATE_EVENT_MASK,
		    requestCode)) {
	return 0;
    }
    if ((mask & CWColormap)
	    && !BrowserColormapOrError(displayPtr, atts->colormap,
		    requestCode, 1)) {
	return 0;
    }
    if ((mask & CWCursor) && (atts->cursor != None)
	    && (BrowserCursorResourceOrError(displayPtr, atts->cursor,
		    requestCode) == NULL)) {
	return 0;
    }
    return 1;
}

int
BrowserValidateCard16(
    Display **displayPtr,
    long value,
    int allowZero,
    unsigned char requestCode)
{
    int lowerBoundOk = allowZero ? (value >= 0) : (value > 0);

    return BrowserValueOrError(displayPtr, value,
	    lowerBoundOk && (value <= USHRT_MAX), requestCode);
}

int
BrowserValidateConfigureWindowValues(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    unsigned int valueMask,
    XWindowChanges *changes,
    unsigned char requestCode)
{
    const unsigned int validMask = CWX | CWY | CWWidth | CWHeight
	    | CWBorderWidth | CWSibling | CWStackMode;
    unsigned int invalidMask = valueMask & ~validMask;

    if (invalidMask != 0U) {
	return BrowserValueOrError(displayPtr, (long)invalidMask, 0,
		requestCode);
    }
    if (changes == NULL) {
	return 1;
    }
    if ((valueMask & CWWidth)
	    && !BrowserValidateCard16(displayPtr, changes->width, 0,
		    requestCode)) {
	return 0;
    }
    if ((valueMask & CWHeight)
	    && !BrowserValidateCard16(displayPtr, changes->height, 0,
		    requestCode)) {
	return 0;
    }
    if ((valueMask & CWBorderWidth)
	    && !BrowserValidateCard16(displayPtr, changes->border_width, 1,
		    requestCode)) {
	return 0;
    }
    if ((resourcePtr != NULL) && (resourcePtr->c_class == InputOnly)
	    && (valueMask & CWBorderWidth) && (changes->border_width != 0)) {
	return BrowserBadMatchOrError(displayPtr, (XID)resourcePtr->id,
		requestCode);
    }
    if ((valueMask & CWStackMode)
	    && !BrowserValueOrError(displayPtr, changes->stack_mode,
		    (changes->stack_mode >= Above)
		    && (changes->stack_mode <= Opposite), requestCode)) {
	return 0;
    }
    if (valueMask & CWSibling) {
	X11WasmResource *siblingPtr = BrowserWindowResourceOrError(displayPtr,
		changes->sibling, requestCode);

	if (siblingPtr == NULL) {
	    return 0;
	}
	if ((resourcePtr != NULL) && ((siblingPtr == resourcePtr)
		|| (siblingPtr->parentPtr != resourcePtr->parentPtr))) {
	    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

	    if (display == NULL) {
		display = X11WasmGetDisplay(NULL);
		if (displayPtr != NULL) {
		    *displayPtr = display;
		}
	    }
	    (void)BrowserReportXError(display, BadMatch,
		    (XID)changes->sibling, requestCode, 0);
	    return 0;
	}
    }
    return 1;
}

void
BrowserApplyWindowAttributes(
    X11WasmResource *resourcePtr,
    unsigned long mask,
    XSetWindowAttributes *atts)
{
    if ((resourcePtr == NULL) || (atts == NULL)) {
	return;
    }
    if (mask & CWBackPixmap) {
	resourcePtr->atts.background_pixmap = atts->background_pixmap;
	resourcePtr->hasBackgroundPixel = 0;
    }
    if (mask & CWBackPixel) {
	resourcePtr->atts.background_pixel = atts->background_pixel;
	resourcePtr->hasBackgroundPixel = 1;
    }
    if (mask & CWBorderPixmap) {
	resourcePtr->atts.border_pixmap = atts->border_pixmap;
	resourcePtr->hasBorderPixel = 0;
    }
    if (mask & CWBorderPixel) {
	resourcePtr->atts.border_pixel = atts->border_pixel;
	resourcePtr->hasBorderPixel = 1;
    }
    if (mask & CWBitGravity) {
	resourcePtr->atts.bit_gravity = atts->bit_gravity;
    }
    if (mask & CWWinGravity) {
	resourcePtr->atts.win_gravity = atts->win_gravity;
    }
    if (mask & CWBackingStore) {
	resourcePtr->atts.backing_store = atts->backing_store;
    }
    if (mask & CWBackingPlanes) {
	resourcePtr->atts.backing_planes = atts->backing_planes;
    }
    if (mask & CWBackingPixel) {
	resourcePtr->atts.backing_pixel = atts->backing_pixel;
    }
    if (mask & CWOverrideRedirect) {
	resourcePtr->atts.override_redirect = atts->override_redirect;
    }
    if (mask & CWSaveUnder) {
	resourcePtr->atts.save_under = atts->save_under;
    }
    if (mask & CWEventMask) {
	resourcePtr->atts.event_mask = atts->event_mask;
    }
    if (mask & CWDontPropagate) {
	resourcePtr->atts.do_not_propagate_mask = atts->do_not_propagate_mask;
    }
    if (mask & CWColormap) {
	resourcePtr->atts.colormap = atts->colormap;
    }
    if (mask & CWCursor) {
	BrowserSetWindowCursor(resourcePtr, atts->cursor);
    }
}

unsigned int
BrowserStylePixmap(
    Pixmap pixmap)
{
    X11WasmResource *pixmapPtr = X11WasmFindResource((XID)pixmap);

    if ((pixmapPtr == NULL)
	    || (pixmapPtr->type != X11_WASM_RESOURCE_PIXMAP)) {
	return 0;
    }
    return BrowserJsXId(pixmapPtr->id);
}

void
BrowserResolveWindowBackgroundStyle(
    X11WasmResource *resourcePtr,
    int *hasPixelPtr,
    unsigned long *pixelPtr,
    Pixmap *pixmapPtr)
{
    if (hasPixelPtr != NULL) {
	*hasPixelPtr = 0;
    }
    if (pixelPtr != NULL) {
	*pixelPtr = 0;
    }
    if (pixmapPtr != NULL) {
	*pixmapPtr = None;
    }
    while ((resourcePtr != NULL)
	    && (resourcePtr->type == X11_WASM_RESOURCE_WINDOW)) {
	if (resourcePtr->hasBackgroundPixel) {
	    if (hasPixelPtr != NULL) {
		*hasPixelPtr = 1;
	    }
	    if (pixelPtr != NULL) {
		*pixelPtr = resourcePtr->atts.background_pixel;
	    }
	    return;
	}
	if (resourcePtr->atts.background_pixmap != (Pixmap)ParentRelative) {
	    if (pixmapPtr != NULL) {
		*pixmapPtr = resourcePtr->atts.background_pixmap;
	    }
	    return;
	}
	resourcePtr = resourcePtr->parentPtr;
    }
}

void
BrowserResolveWindowBorderStyle(
    X11WasmResource *resourcePtr,
    int *hasPixelPtr,
    unsigned long *pixelPtr,
    Pixmap *pixmapPtr)
{
    if (hasPixelPtr != NULL) {
	*hasPixelPtr = 0;
    }
    if (pixelPtr != NULL) {
	*pixelPtr = 0;
    }
    if (pixmapPtr != NULL) {
	*pixmapPtr = None;
    }
    while ((resourcePtr != NULL)
	    && (resourcePtr->type == X11_WASM_RESOURCE_WINDOW)) {
	if (resourcePtr->hasBorderPixel) {
	    if (hasPixelPtr != NULL) {
		*hasPixelPtr = 1;
	    }
	    if (pixelPtr != NULL) {
		*pixelPtr = resourcePtr->atts.border_pixel;
	    }
	    return;
	}
	if (resourcePtr->atts.border_pixmap != (Pixmap)CopyFromParent) {
	    if (pixmapPtr != NULL) {
		*pixmapPtr = resourcePtr->atts.border_pixmap;
	    }
	    return;
	}
	resourcePtr = resourcePtr->parentPtr;
    }
}

int
BrowserWindowInheritsStyle(
    X11WasmResource *resourcePtr)
{
    return (resourcePtr != NULL)
	    && (resourcePtr->type == X11_WASM_RESOURCE_WINDOW)
	    && ((!resourcePtr->hasBackgroundPixel
		    && (resourcePtr->atts.background_pixmap
			    == (Pixmap)ParentRelative))
		    || (!resourcePtr->hasBorderPixel
			    && (resourcePtr->atts.border_pixmap
				    == (Pixmap)CopyFromParent))
		    || (resourcePtr->atts.cursor == None));
}

void
BrowserSyncInheritedChildStyles(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *childPtr;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	if (BrowserWindowInheritsStyle(childPtr)) {
	    BrowserSyncWindowStyle(childPtr);
	    BrowserSyncInheritedChildStyles(childPtr);
	}
    }
}

unsigned int
BrowserCursorShape(
    Cursor cursor)
{
    X11WasmResource *cursorPtr;

    if (cursor == None) {
	return 0;
    }
    cursorPtr = X11WasmFindResource((XID)cursor);
    if ((cursorPtr == NULL) || (cursorPtr->type != X11_WASM_RESOURCE_CURSOR)) {
	return 0;
    }
    if (cursorPtr->hasCursorImage) {
	return BrowserJsXId(cursorPtr->id);
    }
    return cursorPtr->cursorShape;
}

Cursor
BrowserResolveWindowCursorStyle(
    X11WasmResource *resourcePtr)
{
    while ((resourcePtr != NULL)
	    && (resourcePtr->type == X11_WASM_RESOURCE_WINDOW)) {
	if (resourcePtr->atts.cursor != None) {
	    return resourcePtr->atts.cursor;
	}
	resourcePtr = resourcePtr->parentPtr;
    }
    return None;
}

void
BrowserRetainCursor(
    Cursor cursor)
{
    X11WasmResource *cursorPtr;

    if (cursor == None) {
	return;
    }
    cursorPtr = X11WasmFindResource((XID)cursor);
    if ((cursorPtr == NULL) || (cursorPtr->type != X11_WASM_RESOURCE_CURSOR)) {
	return;
    }
    cursorPtr->referenceCount++;
}

void
BrowserReleaseCursor(
    Cursor cursor)
{
    X11WasmResource *cursorPtr;

    if (cursor == None) {
	return;
    }
    cursorPtr = X11WasmFindResource((XID)cursor);
    if ((cursorPtr == NULL) || (cursorPtr->type != X11_WASM_RESOURCE_CURSOR)) {
	return;
    }
    if (cursorPtr->referenceCount > 0) {
	cursorPtr->referenceCount--;
    }
    if (cursorPtr->clientReleased && (cursorPtr->referenceCount == 0)) {
	(void)X11WasmJsDestroyResource(BrowserJsXId(cursor));
	X11WasmDeleteResource((XID)cursor);
    }
}

void
BrowserSetPointerGrabCursor(
    Cursor cursor)
{
    Cursor oldCursor = browserDisplay.pointerGrabCursor;

    if (oldCursor == cursor) {
	return;
    }
    BrowserRetainCursor(cursor);
    browserDisplay.pointerGrabCursor = cursor;
    BrowserReleaseCursor(oldCursor);
}

void
BrowserSetWindowCursor(
    X11WasmResource *resourcePtr,
    Cursor cursor)
{
    Cursor oldCursor;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    oldCursor = resourcePtr->atts.cursor;
    if (oldCursor == cursor) {
	return;
    }
    BrowserRetainCursor(cursor);
    resourcePtr->atts.cursor = cursor;
    BrowserReleaseCursor(oldCursor);
}

void
BrowserSyncWindowStyle(
    X11WasmResource *resourcePtr)
{
    int hasCursor;
    int hasBackgroundPixel;
    int hasBorderPixel;
    unsigned long backgroundPixel;
    unsigned long borderPixel;
    Pixmap backgroundPixmap;
    Pixmap borderPixmap;
    Cursor cursor;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    cursor = BrowserResolveWindowCursorStyle(resourcePtr);
    hasCursor = cursor != None;
    BrowserResolveWindowBackgroundStyle(resourcePtr, &hasBackgroundPixel,
	    &backgroundPixel, &backgroundPixmap);
    BrowserResolveWindowBorderStyle(resourcePtr, &hasBorderPixel,
	    &borderPixel, &borderPixmap);
    (void)X11WasmJsSetWindowStyle(BrowserJsXId(resourcePtr->id),
	    hasBackgroundPixel, backgroundPixel,
	    BrowserStylePixmap(backgroundPixmap),
	    hasBorderPixel, borderPixel, BrowserStylePixmap(borderPixmap),
	    hasCursor, BrowserCursorShape(cursor));
}

void
BrowserSyncPointerGrabCursor(void)
{
    Cursor cursor = browserDisplay.pointerGrabCursor;
    unsigned int cursorShape = BrowserCursorShape(cursor);

    (void)X11WasmJsSetPointerGrabCursor(
	    (cursor != None) && (cursorShape != 0), cursorShape);
}

void
BrowserSyncPointerGrabState(void)
{
    Window grabWindow = browserDisplay.pointerGrabWindow;
    Window confineWindow = browserDisplay.pointerGrabConfineWindow;
    Cursor cursor = browserDisplay.pointerGrabCursor;
    unsigned int cursorShape = BrowserCursorShape(cursor);
    int enabled = grabWindow != None;

    (void)X11WasmJsSetPointerGrabState(enabled,
	    enabled ? BrowserJsXId(grabWindow) : 0,
	    enabled ? browserDisplay.pointerGrabOwnerEvents : 0,
	    enabled ? browserDisplay.pointerGrabEventMask : 0,
	    (enabled && (confineWindow != None)) ? BrowserJsXId(confineWindow) : 0,
	    enabled ? cursorShape : 0,
	    enabled ? browserDisplay.pointerGrabPointerMode : GrabModeAsync,
	    enabled ? browserDisplay.pointerGrabKeyboardMode : GrabModeAsync);
}

void
BrowserSyncCursorUsers(
    Cursor cursor)
{
    X11WasmResource *resourcePtr;

    if (cursor == None) {
	return;
    }
    for (resourcePtr = browserDisplay.resourceList; resourcePtr != NULL;
	    resourcePtr = resourcePtr->nextPtr) {
	if ((resourcePtr->type == X11_WASM_RESOURCE_WINDOW)
		&& (resourcePtr->atts.cursor == cursor)) {
	    BrowserSyncWindowStyle(resourcePtr);
	    BrowserSyncInheritedChildStyles(resourcePtr);
	}
    }
    if (browserDisplay.pointerGrabCursor == cursor) {
	BrowserSyncPointerGrabCursor();
	BrowserSyncPointerGrabState();
    }
}

void
BrowserSyncPointerState(void)
{
    (void)X11WasmJsSetPointerState(browserDisplay.pointerX,
	    browserDisplay.pointerY,
	    BrowserJsXId(browserDisplay.pointerWindow),
	    browserDisplay.pointerState,
	    (browserDisplay.pointerCaptureWindow != None)
		? BrowserJsXId(browserDisplay.pointerCaptureWindow) : 0);
}

void
BrowserSyncKeyboardGrabState(void)
{
    Window grabWindow = browserDisplay.keyboardGrabWindow;
    int enabled = grabWindow != None;

    (void)X11WasmJsSetKeyboardGrabState(enabled,
	    enabled ? BrowserJsXId(grabWindow) : 0,
	    enabled ? browserDisplay.keyboardGrabOwnerEvents : 0,
	    enabled ? browserDisplay.keyboardGrabPointerMode : GrabModeAsync,
	    enabled ? browserDisplay.keyboardGrabKeyboardMode : GrabModeAsync);
}

void
BrowserSyncWindowEventMask(
    X11WasmResource *resourcePtr)
{
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    (void)X11WasmJsSetWindowEventMask(BrowserJsXId(resourcePtr->id),
	    (unsigned long)resourcePtr->atts.event_mask,
	    (unsigned long)resourcePtr->atts.do_not_propagate_mask);
}

void
BrowserSyncWindowColormap(
    X11WasmResource *resourcePtr)
{
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    (void)X11WasmJsSetWindowColormap(BrowserJsXId(resourcePtr->id),
	    BrowserJsXId(resourcePtr->atts.colormap));
}

void
BrowserSyncWindowAttributes(
    X11WasmResource *resourcePtr)
{
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    (void)X11WasmJsSetWindowAttributes(BrowserJsXId(resourcePtr->id),
	    resourcePtr->atts.override_redirect,
	    resourcePtr->atts.save_under,
	    resourcePtr->atts.bit_gravity,
	    resourcePtr->atts.win_gravity,
	    resourcePtr->atts.backing_store,
	    resourcePtr->atts.backing_planes,
	    resourcePtr->atts.backing_pixel);
}

void
BrowserMarkResourceDamage(
    X11WasmResource *resourcePtr,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    int x1, y1, x2, y2;
    int oldX1, oldY1, oldX2, oldY2;

    if ((resourcePtr == NULL) || (width == 0) || (height == 0)) {
	return;
    }
    if ((resourcePtr->type == X11_WASM_RESOURCE_WINDOW)
	    && (resourcePtr->c_class == InputOnly)) {
	return;
    }
    if (x < 0) {
	unsigned int clipped = (unsigned int)-x;
	if (clipped >= width) {
	    return;
	}
	width -= clipped;
	x = 0;
    }
    if (y < 0) {
	unsigned int clipped = (unsigned int)-y;
	if (clipped >= height) {
	    return;
	}
	height -= clipped;
	y = 0;
    }
    if (((unsigned int)x >= resourcePtr->width)
	    || ((unsigned int)y >= resourcePtr->height)) {
	return;
    }
    if (width > resourcePtr->width - (unsigned int)x) {
	width = resourcePtr->width - (unsigned int)x;
    }
    if (height > resourcePtr->height - (unsigned int)y) {
	height = resourcePtr->height - (unsigned int)y;
    }
    if ((width == 0) || (height == 0)) {
	return;
    }

    x1 = x;
    y1 = y;
    x2 = x + (int)width;
    y2 = y + (int)height;
    if (resourcePtr->damaged) {
	oldX1 = resourcePtr->damage.x;
	oldY1 = resourcePtr->damage.y;
	oldX2 = oldX1 + resourcePtr->damage.width;
	oldY2 = oldY1 + resourcePtr->damage.height;
	if (oldX1 < x1) x1 = oldX1;
	if (oldY1 < y1) y1 = oldY1;
	if (oldX2 > x2) x2 = oldX2;
	if (oldY2 > y2) y2 = oldY2;
    }

    resourcePtr->damaged = 1;
    resourcePtr->damage.x = BrowserClampShort(x1);
    resourcePtr->damage.y = BrowserClampShort(y1);
    resourcePtr->damage.width = BrowserClampUShort((unsigned int)(x2 - x1));
    resourcePtr->damage.height = BrowserClampUShort((unsigned int)(y2 - y1));
    resourcePtr->damageSerial = damageSerial++;
}

int
BrowserQueueExpose(
    X11WasmResource *resourcePtr,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (resourcePtr->c_class == InputOnly)
	    || !BrowserResourceIsVisible(resourcePtr)
	    || !BrowserResourceWantsEvent(resourcePtr, ExposureMask)) {
	return 0;
    }
    BrowserMarkResourceDamage(resourcePtr, x, y, width, height);
    BrowserSetCommonEventFields(&event, Expose, (Window)resourcePtr->id,
	    CurrentTime);
    event.xexpose.window = (Window)resourcePtr->id;
    event.xexpose.x = x;
    event.xexpose.y = y;
    event.xexpose.width = (int)width;
    event.xexpose.height = (int)height;
    event.xexpose.count = 0;
    return BrowserQueueEvent(&event);
}

void
BrowserQueueViewableExposeTree(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *childPtr;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || !BrowserResourceIsVisible(resourcePtr)) {
	return;
    }
    (void)BrowserQueueExpose(resourcePtr, 0, 0, resourcePtr->width,
	    resourcePtr->height);
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	if (childPtr->mapped) {
	    BrowserQueueViewableExposeTree(childPtr);
	}
    }
}

int
BrowserQueueCreateNotify(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *parentPtr;
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if ((parentPtr == NULL)
	    || !BrowserResourceWantsEvent(parentPtr, SubstructureNotifyMask)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, CreateNotify,
	    (Window)parentPtr->id, CurrentTime);
    event.xcreatewindow.parent = (Window)parentPtr->id;
    event.xcreatewindow.window = (Window)resourcePtr->id;
    event.xcreatewindow.x = resourcePtr->x;
    event.xcreatewindow.y = resourcePtr->y;
    event.xcreatewindow.width = (int)resourcePtr->width;
    event.xcreatewindow.height = (int)resourcePtr->height;
    event.xcreatewindow.border_width = (int)resourcePtr->borderWidth;
    event.xcreatewindow.override_redirect =
	    resourcePtr->atts.override_redirect;
    return BrowserQueueEvent(&event);
}

int
BrowserQueueMapNotify(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *parentPtr;
    XEvent event;
    int queued = 0;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    if (BrowserResourceWantsEvent(resourcePtr, StructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, MapNotify, (Window)resourcePtr->id,
		CurrentTime);
	event.xmap.event = (Window)resourcePtr->id;
	event.xmap.window = (Window)resourcePtr->id;
	event.xmap.override_redirect = resourcePtr->atts.override_redirect;
	queued += BrowserQueueEvent(&event);
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if (BrowserResourceWantsEvent(parentPtr, SubstructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, MapNotify, (Window)parentPtr->id,
		CurrentTime);
	event.xmap.event = (Window)parentPtr->id;
	event.xmap.window = (Window)resourcePtr->id;
	event.xmap.override_redirect = resourcePtr->atts.override_redirect;
	queued += BrowserQueueEvent(&event);
    }
    return queued;
}

int
BrowserQueueMapRequest(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *parentPtr;
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if (!BrowserResourceWantsEvent(parentPtr, SubstructureRedirectMask)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, MapRequest, (Window)parentPtr->id,
	    CurrentTime);
    event.xmaprequest.parent = (Window)parentPtr->id;
    event.xmaprequest.window = (Window)resourcePtr->id;
    return BrowserQueueEvent(&event);
}

int
BrowserQueueUnmapNotify(
    X11WasmResource *resourcePtr,
    Bool fromConfigure)
{
    X11WasmResource *parentPtr;
    XEvent event;
    int queued = 0;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    if (BrowserResourceWantsEvent(resourcePtr, StructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, UnmapNotify,
		(Window)resourcePtr->id, CurrentTime);
	event.xunmap.event = (Window)resourcePtr->id;
	event.xunmap.window = (Window)resourcePtr->id;
	event.xunmap.from_configure = fromConfigure;
	queued += BrowserQueueEvent(&event);
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if (BrowserResourceWantsEvent(parentPtr, SubstructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, UnmapNotify,
		(Window)parentPtr->id, CurrentTime);
	event.xunmap.event = (Window)parentPtr->id;
	event.xunmap.window = (Window)resourcePtr->id;
	event.xunmap.from_configure = fromConfigure;
	queued += BrowserQueueEvent(&event);
    }
    return queued;
}

int
BrowserQueueReparentNotify(
    X11WasmResource *resourcePtr,
    Window oldParent,
    Window newParent,
    int x,
    int y)
{
    X11WasmResource *oldParentPtr;
    X11WasmResource *newParentPtr;
    XEvent event;
    int queued = 0;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    if (BrowserResourceWantsEvent(resourcePtr, StructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, ReparentNotify,
		(Window)resourcePtr->id, CurrentTime);
	event.xreparent.event = (Window)resourcePtr->id;
	event.xreparent.window = (Window)resourcePtr->id;
	event.xreparent.parent = newParent;
	event.xreparent.x = x;
	event.xreparent.y = y;
	event.xreparent.override_redirect =
		resourcePtr->atts.override_redirect;
	queued += BrowserQueueEvent(&event);
    }
    oldParentPtr = BrowserWindowResource(oldParent);
    if (BrowserResourceWantsEvent(oldParentPtr, SubstructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, ReparentNotify, oldParent,
		CurrentTime);
	event.xreparent.event = oldParent;
	event.xreparent.window = (Window)resourcePtr->id;
	event.xreparent.parent = newParent;
	event.xreparent.x = x;
	event.xreparent.y = y;
	event.xreparent.override_redirect =
		resourcePtr->atts.override_redirect;
	queued += BrowserQueueEvent(&event);
    }
    newParentPtr = BrowserWindowResource(newParent);
    if ((newParent != oldParent)
	    && BrowserResourceWantsEvent(newParentPtr, SubstructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, ReparentNotify, newParent,
		CurrentTime);
	event.xreparent.event = newParent;
	event.xreparent.window = (Window)resourcePtr->id;
	event.xreparent.parent = newParent;
	event.xreparent.x = x;
	event.xreparent.y = y;
	event.xreparent.override_redirect =
		resourcePtr->atts.override_redirect;
	queued += BrowserQueueEvent(&event);
    }
    return queued;
}

int
BrowserQueueDestroyNotify(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *parentPtr;
    XEvent event;
    int queued = 0;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    if (BrowserResourceWantsEvent(resourcePtr, StructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, DestroyNotify,
		(Window)resourcePtr->id, CurrentTime);
	event.xdestroywindow.event = (Window)resourcePtr->id;
	event.xdestroywindow.window = (Window)resourcePtr->id;
	queued += BrowserQueueEvent(&event);
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if (BrowserResourceWantsEvent(parentPtr, SubstructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, DestroyNotify,
		(Window)parentPtr->id, CurrentTime);
	event.xdestroywindow.event = (Window)parentPtr->id;
	event.xdestroywindow.window = (Window)resourcePtr->id;
	queued += BrowserQueueEvent(&event);
    }
    return queued;
}

int
BrowserQueueConfigureNotify(
    X11WasmResource *resourcePtr,
    Window above)
{
    X11WasmResource *parentPtr;
    XEvent event;
    int queued = 0;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    if (BrowserResourceWantsEvent(resourcePtr, StructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, ConfigureNotify,
		(Window)resourcePtr->id, CurrentTime);
	event.xconfigure.event = (Window)resourcePtr->id;
	event.xconfigure.window = (Window)resourcePtr->id;
	event.xconfigure.x = resourcePtr->x;
	event.xconfigure.y = resourcePtr->y;
	event.xconfigure.width = (int)resourcePtr->width;
	event.xconfigure.height = (int)resourcePtr->height;
	event.xconfigure.border_width = (int)resourcePtr->borderWidth;
	event.xconfigure.above = above;
	event.xconfigure.override_redirect = resourcePtr->atts.override_redirect;
	queued += BrowserQueueEvent(&event);
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if (BrowserResourceWantsEvent(parentPtr, SubstructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, ConfigureNotify,
		(Window)parentPtr->id, CurrentTime);
	event.xconfigure.event = (Window)parentPtr->id;
	event.xconfigure.window = (Window)resourcePtr->id;
	event.xconfigure.x = resourcePtr->x;
	event.xconfigure.y = resourcePtr->y;
	event.xconfigure.width = (int)resourcePtr->width;
	event.xconfigure.height = (int)resourcePtr->height;
	event.xconfigure.border_width = (int)resourcePtr->borderWidth;
	event.xconfigure.above = above;
	event.xconfigure.override_redirect = resourcePtr->atts.override_redirect;
	queued += BrowserQueueEvent(&event);
    }
    return queued;
}

int
BrowserQueueConfigureRequest(
    X11WasmResource *resourcePtr,
    unsigned int valueMask,
    XWindowChanges *changes)
{
    X11WasmResource *parentPtr;
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if (!BrowserResourceWantsEvent(parentPtr, SubstructureRedirectMask)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, ConfigureRequest,
	    (Window)parentPtr->id, CurrentTime);
    event.xconfigurerequest.parent = (Window)parentPtr->id;
    event.xconfigurerequest.window = (Window)resourcePtr->id;
    event.xconfigurerequest.x = (valueMask & CWX)
	    ? changes->x : resourcePtr->x;
    event.xconfigurerequest.y = (valueMask & CWY)
	    ? changes->y : resourcePtr->y;
    event.xconfigurerequest.width = (valueMask & CWWidth)
	    ? changes->width : (int)resourcePtr->width;
    event.xconfigurerequest.height = (valueMask & CWHeight)
	    ? changes->height : (int)resourcePtr->height;
    event.xconfigurerequest.border_width = (valueMask & CWBorderWidth)
	    ? changes->border_width : (int)resourcePtr->borderWidth;
    event.xconfigurerequest.above = (valueMask & CWSibling)
	    ? changes->sibling : None;
    event.xconfigurerequest.detail = (valueMask & CWStackMode)
	    ? changes->stack_mode : Above;
    event.xconfigurerequest.value_mask = valueMask;
    return BrowserQueueEvent(&event);
}

int
BrowserQueueResizeRequest(
    X11WasmResource *resourcePtr,
    unsigned int valueMask,
    XWindowChanges *changes)
{
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (changes == NULL)
	    || !BrowserResourceWantsEvent(resourcePtr, ResizeRedirectMask)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, ResizeRequest,
	    (Window)resourcePtr->id, CurrentTime);
    event.xresizerequest.window = (Window)resourcePtr->id;
    event.xresizerequest.width = (valueMask & CWWidth)
	    ? changes->width : (int)resourcePtr->width;
    event.xresizerequest.height = (valueMask & CWHeight)
	    ? changes->height : (int)resourcePtr->height;
    return BrowserQueueEvent(&event);
}

int
BrowserQueueCirculateNotify(
    X11WasmResource *resourcePtr,
    int place)
{
    X11WasmResource *parentPtr;
    XEvent event;
    int queued = 0;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    if (BrowserResourceWantsEvent(resourcePtr, StructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, CirculateNotify,
		(Window)resourcePtr->id, CurrentTime);
	event.xcirculate.event = (Window)resourcePtr->id;
	event.xcirculate.window = (Window)resourcePtr->id;
	event.xcirculate.place = place;
	queued += BrowserQueueEvent(&event);
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if (BrowserResourceWantsEvent(parentPtr, SubstructureNotifyMask)) {
	BrowserSetCommonEventFields(&event, CirculateNotify,
		(Window)parentPtr->id, CurrentTime);
	event.xcirculate.event = (Window)parentPtr->id;
	event.xcirculate.window = (Window)resourcePtr->id;
	event.xcirculate.place = place;
	queued += BrowserQueueEvent(&event);
    }
    return queued;
}

int
BrowserQueueCirculateRequest(
    X11WasmResource *resourcePtr,
    int place)
{
    X11WasmResource *parentPtr;
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return 0;
    }
    parentPtr = BrowserWindowResource(resourcePtr->parent);
    if (!BrowserResourceWantsEvent(parentPtr, SubstructureRedirectMask)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, CirculateRequest,
	    (Window)parentPtr->id, CurrentTime);
    event.xcirculaterequest.parent = (Window)parentPtr->id;
    event.xcirculaterequest.window = (Window)resourcePtr->id;
    event.xcirculaterequest.place = place;
    return BrowserQueueEvent(&event);
}

int
BrowserQueueGravityNotify(
    X11WasmResource *resourcePtr)
{
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || !BrowserResourceWantsEvent(resourcePtr, StructureNotifyMask)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, GravityNotify,
	    (Window)resourcePtr->id, CurrentTime);
    event.xgravity.event = (Window)resourcePtr->id;
    event.xgravity.window = (Window)resourcePtr->id;
    event.xgravity.x = resourcePtr->x;
    event.xgravity.y = resourcePtr->y;
    return BrowserQueueEvent(&event);
}

#define BROWSER_VISIBILITY_RECT_LIMIT 256
#define BROWSER_VISIBILITY_RECTS_OVERFLOW -1
#define BROWSER_VISIBILITY_RECTS_EMPTY 0
#define BROWSER_VISIBILITY_RECTS_READY 1

static int
BrowserVisibilityRectIsEmpty(
    const BrowserVisibilityRect *rectPtr)
{
    return (rectPtr == NULL) || (rectPtr->x1 >= rectPtr->x2)
	    || (rectPtr->y1 >= rectPtr->y2);
}

static int
BrowserVisibilityRectEquals(
    const BrowserVisibilityRect *firstPtr,
    const BrowserVisibilityRect *secondPtr)
{
    return (firstPtr != NULL) && (secondPtr != NULL)
	    && (firstPtr->x1 == secondPtr->x1)
	    && (firstPtr->y1 == secondPtr->y1)
	    && (firstPtr->x2 == secondPtr->x2)
	    && (firstPtr->y2 == secondPtr->y2);
}

static int
BrowserVisibilityRectIntersect(
    BrowserVisibilityRect *rectPtr,
    const BrowserVisibilityRect *clipPtr)
{
    if ((rectPtr == NULL) || (clipPtr == NULL)) {
	return 0;
    }
    if (rectPtr->x1 < clipPtr->x1) rectPtr->x1 = clipPtr->x1;
    if (rectPtr->y1 < clipPtr->y1) rectPtr->y1 = clipPtr->y1;
    if (rectPtr->x2 > clipPtr->x2) rectPtr->x2 = clipPtr->x2;
    if (rectPtr->y2 > clipPtr->y2) rectPtr->y2 = clipPtr->y2;
    return !BrowserVisibilityRectIsEmpty(rectPtr);
}

static int
BrowserVisibilityRectsOverlap(
    const BrowserVisibilityRect *firstPtr,
    const BrowserVisibilityRect *secondPtr)
{
    return (firstPtr != NULL) && (secondPtr != NULL)
	    && (firstPtr->x1 < secondPtr->x2)
	    && (firstPtr->x2 > secondPtr->x1)
	    && (firstPtr->y1 < secondPtr->y2)
	    && (firstPtr->y2 > secondPtr->y1);
}

static int
BrowserVisibilityResourceRect(
    X11WasmResource *resourcePtr,
    BrowserVisibilityRect *rectPtr)
{
    int rootX = 0;
    int rootY = 0;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (rectPtr == NULL)
	    || (resourcePtr->width == 0)
	    || (resourcePtr->height == 0)) {
	return 0;
    }
    BrowserWindowRootOffset((Window)resourcePtr->id, &rootX, &rootY);
    rectPtr->x1 = rootX;
    rectPtr->y1 = rootY;
    rectPtr->x2 = rootX + (int)resourcePtr->width;
    rectPtr->y2 = rootY + (int)resourcePtr->height;
    return !BrowserVisibilityRectIsEmpty(rectPtr);
}

static int
BrowserVisibilityVisibleRect(
    X11WasmResource *resourcePtr,
    BrowserVisibilityRect *rectPtr)
{
    X11WasmResource *ancestorPtr;
    BrowserVisibilityRect clip;

    if (!BrowserVisibilityResourceRect(resourcePtr, rectPtr)) {
	return 0;
    }
    for (ancestorPtr = BrowserWindowParentResource(resourcePtr);
	    ancestorPtr != NULL;
	    ancestorPtr = BrowserWindowParentResource(ancestorPtr)) {
	if (!BrowserVisibilityResourceRect(ancestorPtr, &clip)
		|| !BrowserVisibilityRectIntersect(rectPtr, &clip)) {
	    return 0;
	}
    }
    return 1;
}

static int
BrowserVisibilityAppendRect(
    BrowserVisibilityRect *rects,
    int *countPtr,
    const BrowserVisibilityRect *rectPtr)
{
    if (BrowserVisibilityRectIsEmpty(rectPtr)) {
	return 1;
    }
    if (*countPtr >= BROWSER_VISIBILITY_RECT_LIMIT) {
	return 0;
    }
    rects[(*countPtr)++] = *rectPtr;
    return 1;
}

static int
BrowserVisibilitySubtractRect(
    BrowserVisibilityRect *rects,
    int *countPtr,
    const BrowserVisibilityRect *coverPtr)
{
    BrowserVisibilityRect result[BROWSER_VISIBILITY_RECT_LIMIT];
    int resultCount = 0;
    int i;

    for (i = 0; i < *countPtr; i++) {
	BrowserVisibilityRect rect = rects[i];
	BrowserVisibilityRect overlap = rect;
	BrowserVisibilityRect piece;

	if (!BrowserVisibilityRectIntersect(&overlap, coverPtr)) {
	    if (!BrowserVisibilityAppendRect(result, &resultCount, &rect)) {
		return 0;
	    }
	    continue;
	}

	piece = rect;
	piece.x2 = overlap.x1;
	if (!BrowserVisibilityAppendRect(result, &resultCount, &piece)) {
	    return 0;
	}
	piece = rect;
	piece.x1 = overlap.x2;
	if (!BrowserVisibilityAppendRect(result, &resultCount, &piece)) {
	    return 0;
	}
	piece = rect;
	piece.x1 = overlap.x1;
	piece.x2 = overlap.x2;
	piece.y2 = overlap.y1;
	if (!BrowserVisibilityAppendRect(result, &resultCount, &piece)) {
	    return 0;
	}
	piece = rect;
	piece.x1 = overlap.x1;
	piece.x2 = overlap.x2;
	piece.y1 = overlap.y2;
	if (!BrowserVisibilityAppendRect(result, &resultCount, &piece)) {
	    return 0;
	}
    }
    memcpy(rects, result, sizeof(BrowserVisibilityRect) * (size_t)resultCount);
    *countPtr = resultCount;
    return 1;
}

static int
BrowserWindowVisibleRects(
    X11WasmResource *resourcePtr,
    BrowserVisibilityRect *rects,
    int *countPtr)
{
    BrowserVisibilityRect visibleRect;
    X11WasmResource *nodePtr;

    if (countPtr != NULL) {
	*countPtr = 0;
    }
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (resourcePtr->c_class == InputOnly)
	    || !BrowserResourceIsVisible(resourcePtr)
	    || (rects == NULL)
	    || (countPtr == NULL)
	    || !BrowserVisibilityVisibleRect(resourcePtr, &visibleRect)) {
	return BROWSER_VISIBILITY_RECTS_EMPTY;
    }
    rects[0] = visibleRect;
    *countPtr = 1;

    for (nodePtr = resourcePtr; nodePtr != NULL
	    && nodePtr->id != browserDisplay.rootWindow;
	    nodePtr = BrowserWindowParentResource(nodePtr)) {
	X11WasmResource *parentPtr = BrowserWindowParentResource(nodePtr);
	X11WasmResource *siblingPtr;

	if (parentPtr == NULL) {
	    break;
	}
	for (siblingPtr = nodePtr->nextSiblingPtr; siblingPtr != NULL;
		siblingPtr = siblingPtr->nextSiblingPtr) {
	    BrowserVisibilityRect coverRect;

	    if ((siblingPtr->type != X11_WASM_RESOURCE_WINDOW)
		    || (siblingPtr->c_class == InputOnly)
		    || !BrowserResourceIsVisible(siblingPtr)
		    || !BrowserVisibilityVisibleRect(siblingPtr, &coverRect)
		    || !BrowserVisibilityRectsOverlap(&visibleRect,
			    &coverRect)) {
		continue;
	    }
	    if (!BrowserVisibilitySubtractRect(rects, countPtr, &coverRect)) {
		return BROWSER_VISIBILITY_RECTS_OVERFLOW;
	    }
	    if (*countPtr == 0) {
		return BROWSER_VISIBILITY_RECTS_READY;
	    }
	}
    }
    return BROWSER_VISIBILITY_RECTS_READY;
}

static int
BrowserWindowExposureRects(
    X11WasmResource *resourcePtr,
    BrowserVisibilityRect *rects,
    int *countPtr)
{
    X11WasmResource *childPtr;
    int status;

    status = BrowserWindowVisibleRects(resourcePtr, rects, countPtr);
    if (status <= BROWSER_VISIBILITY_RECTS_EMPTY) {
	return status;
    }
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	BrowserVisibilityRect childRect;

	if ((childPtr->type != X11_WASM_RESOURCE_WINDOW)
		|| (childPtr->c_class == InputOnly)
		|| !BrowserResourceIsVisible(childPtr)
		|| !BrowserVisibilityVisibleRect(childPtr, &childRect)) {
	    continue;
	}
	if (!BrowserVisibilitySubtractRect(rects, countPtr, &childRect)) {
	    return BROWSER_VISIBILITY_RECTS_OVERFLOW;
	}
	if (*countPtr == 0) {
	    return BROWSER_VISIBILITY_RECTS_READY;
	}
    }
    return BROWSER_VISIBILITY_RECTS_READY;
}

static int
BrowserWindowVisibilityState(
    X11WasmResource *resourcePtr)
{
    BrowserVisibilityRect fullRect;
    BrowserVisibilityRect uncovered[BROWSER_VISIBILITY_RECT_LIMIT];
    int visibleStatus;
    int uncoveredCount = 0;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (resourcePtr->c_class == InputOnly)
	    || !BrowserResourceIsVisible(resourcePtr)) {
	return X11_WASM_VISIBILITY_NOT_VIEWABLE;
    }
    if (!BrowserVisibilityResourceRect(resourcePtr, &fullRect)) {
	return VisibilityFullyObscured;
    }
    visibleStatus = BrowserWindowVisibleRects(resourcePtr, uncovered,
	    &uncoveredCount);
    if (visibleStatus == BROWSER_VISIBILITY_RECTS_OVERFLOW) {
	return VisibilityPartiallyObscured;
    }
    if ((visibleStatus == BROWSER_VISIBILITY_RECTS_EMPTY)
	    || (uncoveredCount == 0)) {
	return VisibilityFullyObscured;
    }
    if ((uncoveredCount == 1)
	    && BrowserVisibilityRectEquals(&fullRect, &uncovered[0])) {
	return VisibilityUnobscured;
    }
    return VisibilityPartiallyObscured;
}

int
BrowserQueueVisibilityNotify(
    X11WasmResource *resourcePtr,
    int state)
{
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (resourcePtr->c_class == InputOnly)
	    || !BrowserResourceWantsEvent(resourcePtr, VisibilityChangeMask)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, VisibilityNotify,
	    (Window)resourcePtr->id, CurrentTime);
    event.xvisibility.window = (Window)resourcePtr->id;
    event.xvisibility.state = state;
    return BrowserQueueEvent(&event);
}

static BrowserVisibilitySnapshotEntry *
BrowserFindVisibilitySnapshotEntry(
    BrowserVisibilitySnapshot *snapshotPtr,
    Window window)
{
    BrowserVisibilitySnapshotEntry *entryPtr;

    if (snapshotPtr == NULL) {
	return NULL;
    }
    for (entryPtr = snapshotPtr->firstEntryPtr; entryPtr != NULL;
	    entryPtr = entryPtr->nextPtr) {
	if (entryPtr->window == window) {
	    return entryPtr;
	}
    }
    return NULL;
}

static void
BrowserCaptureVisibilitySnapshotTree(
    X11WasmResource *resourcePtr,
    BrowserVisibilitySnapshot *snapshotPtr)
{
    X11WasmResource *childPtr;
    BrowserVisibilityRect rects[BROWSER_VISIBILITY_RECT_LIMIT];
    int rectCount = 0;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (snapshotPtr == NULL)) {
	return;
    }
    if ((resourcePtr->c_class != InputOnly)
	    && BrowserResourceWantsEvent(resourcePtr, ExposureMask)
	    && (BrowserWindowExposureRects(resourcePtr, rects, &rectCount)
		    == BROWSER_VISIBILITY_RECTS_READY)
	    && (rectCount > 0)) {
	BrowserVisibilitySnapshotEntry *entryPtr =
		(BrowserVisibilitySnapshotEntry *)ckalloc(sizeof(*entryPtr));

	entryPtr->window = (Window)resourcePtr->id;
	entryPtr->rectCount = rectCount;
	entryPtr->rects = (BrowserVisibilityRect *)ckalloc(
		sizeof(BrowserVisibilityRect) * (size_t)rectCount);
	memcpy(entryPtr->rects, rects,
		sizeof(BrowserVisibilityRect) * (size_t)rectCount);
	entryPtr->nextPtr = snapshotPtr->firstEntryPtr;
	snapshotPtr->firstEntryPtr = entryPtr;
    }
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
	    BrowserCaptureVisibilitySnapshotTree(childPtr, snapshotPtr);
	}
    }
}

void
BrowserCaptureVisibilitySnapshot(
    X11WasmResource *resourcePtr,
    BrowserVisibilitySnapshot *snapshotPtr)
{
    if (snapshotPtr == NULL) {
	return;
    }
    snapshotPtr->firstEntryPtr = NULL;
    BrowserCaptureVisibilitySnapshotTree(resourcePtr, snapshotPtr);
}

void
BrowserFreeVisibilitySnapshot(
    BrowserVisibilitySnapshot *snapshotPtr)
{
    BrowserVisibilitySnapshotEntry *entryPtr;

    if (snapshotPtr == NULL) {
	return;
    }
    entryPtr = snapshotPtr->firstEntryPtr;
    while (entryPtr != NULL) {
	BrowserVisibilitySnapshotEntry *nextPtr = entryPtr->nextPtr;

	if (entryPtr->rects != NULL) {
	    ckfree(entryPtr->rects);
	}
	ckfree(entryPtr);
	entryPtr = nextPtr;
    }
    snapshotPtr->firstEntryPtr = NULL;
}

static void
BrowserQueueNewlyVisibleExposeForWindow(
    X11WasmResource *resourcePtr,
    BrowserVisibilitySnapshot *snapshotPtr)
{
    BrowserVisibilitySnapshotEntry *oldEntryPtr;
    BrowserVisibilityRect newRects[BROWSER_VISIBILITY_RECT_LIMIT];
    int newRectCount = 0;
    int rootX = 0;
    int rootY = 0;
    int i;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (resourcePtr->c_class == InputOnly)
	    || !BrowserResourceWantsEvent(resourcePtr, ExposureMask)
	    || (BrowserWindowExposureRects(resourcePtr, newRects,
		    &newRectCount) != BROWSER_VISIBILITY_RECTS_READY)
	    || (newRectCount == 0)) {
	return;
    }
    oldEntryPtr = BrowserFindVisibilitySnapshotEntry(snapshotPtr,
	    (Window)resourcePtr->id);
    for (i = 0; i < newRectCount; i++) {
	BrowserVisibilityRect exposed[BROWSER_VISIBILITY_RECT_LIMIT];
	int exposedCount = 1;
	int j;

	exposed[0] = newRects[i];
	if (oldEntryPtr != NULL) {
	    for (j = 0; j < oldEntryPtr->rectCount; j++) {
		if (!BrowserVisibilitySubtractRect(exposed, &exposedCount,
			&oldEntryPtr->rects[j])
			|| (exposedCount == 0)) {
		    break;
		}
	    }
	}
	if (exposedCount == 0) {
	    continue;
	}
	BrowserWindowRootOffset((Window)resourcePtr->id, &rootX, &rootY);
	for (j = 0; j < exposedCount; j++) {
	    (void)BrowserQueueExpose(resourcePtr, exposed[j].x1 - rootX,
		    exposed[j].y1 - rootY,
		    (unsigned int)(exposed[j].x2 - exposed[j].x1),
		    (unsigned int)(exposed[j].y2 - exposed[j].y1));
	}
    }
}

void
BrowserQueueNewlyVisibleExposes(
    X11WasmResource *resourcePtr,
    BrowserVisibilitySnapshot *snapshotPtr,
    Window excludeSubtree)
{
    X11WasmResource *childPtr;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    if ((excludeSubtree == None)
	    || !BrowserWindowInTree((Window)resourcePtr->id, excludeSubtree)) {
	BrowserQueueNewlyVisibleExposeForWindow(resourcePtr, snapshotPtr);
    } else {
	return;
    }
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
	    BrowserQueueNewlyVisibleExposes(childPtr, snapshotPtr,
		    excludeSubtree);
	}
    }
}

void
BrowserRefreshVisibilityTree(
    X11WasmResource *resourcePtr)
{
    X11WasmResource *childPtr;
    int state;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    state = BrowserWindowVisibilityState(resourcePtr);
    if (state == X11_WASM_VISIBILITY_NOT_VIEWABLE) {
	resourcePtr->visibilityState = X11_WASM_VISIBILITY_NOT_VIEWABLE;
    } else if (resourcePtr->visibilityState != state) {
	(void)BrowserQueueVisibilityNotify(resourcePtr, state);
	resourcePtr->visibilityState = state;
    }
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
	    BrowserRefreshVisibilityTree(childPtr);
	}
    }
}

int
BrowserQueueColormapNotify(
    X11WasmResource *resourcePtr,
    Colormap colormap,
    Bool isNew,
    int state)
{
    XEvent event;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || !BrowserResourceWantsEvent(resourcePtr, ColormapChangeMask)) {
	return 0;
    }
    BrowserSetCommonEventFields(&event, ColormapNotify,
	    (Window)resourcePtr->id, CurrentTime);
    event.xcolormap.window = (Window)resourcePtr->id;
    event.xcolormap.colormap = colormap;
    event.xcolormap.new = isNew;
    event.xcolormap.state = state;
    return BrowserQueueEvent(&event);
}

void
BrowserNotifyWindowColormap(
    X11WasmResource *resourcePtr,
    Colormap oldColormap)
{
    Colormap colormap;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	return;
    }
    colormap = resourcePtr->atts.colormap;
    if (colormap == oldColormap) {
	return;
    }
    (void)BrowserQueueColormapNotify(resourcePtr, colormap, True,
	    (colormap != None) ? ColormapInstalled : ColormapUninstalled);
}

int
BrowserWinGravityHorizontalOffset(
    int gravity,
    int deltaWidth)
{
    switch (gravity) {
    case NorthGravity:
    case CenterGravity:
    case SouthGravity:
	return deltaWidth / 2;
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
	return deltaWidth;
    default:
	return 0;
    }
}

int
BrowserWinGravityVerticalOffset(
    int gravity,
    int deltaHeight)
{
    switch (gravity) {
    case WestGravity:
    case CenterGravity:
    case EastGravity:
	return deltaHeight / 2;
    case SouthWestGravity:
    case SouthGravity:
    case SouthEastGravity:
	return deltaHeight;
    default:
	return 0;
    }
}

void
BrowserApplyChildWinGravity(
    X11WasmResource *parentPtr,
    unsigned int oldWidth,
    unsigned int oldHeight)
{
    X11WasmResource *childPtr;
    int deltaWidth;
    int deltaHeight;

    if ((parentPtr == NULL)
	    || (parentPtr->type != X11_WASM_RESOURCE_WINDOW)
	    || ((parentPtr->width == oldWidth)
		&& (parentPtr->height == oldHeight))) {
	return;
    }
    deltaWidth = (int)parentPtr->width - (int)oldWidth;
    deltaHeight = (int)parentPtr->height - (int)oldHeight;
    for (childPtr = parentPtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	int xOffset;
	int yOffset;

	if (childPtr->type != X11_WASM_RESOURCE_WINDOW) {
	    continue;
	}
	if (childPtr->atts.win_gravity == UnmapGravity) {
	    (void)BrowserUnmapWindowResource(childPtr, True);
	    continue;
	}
	xOffset = BrowserWinGravityHorizontalOffset(childPtr->atts.win_gravity,
		deltaWidth);
	yOffset = BrowserWinGravityVerticalOffset(childPtr->atts.win_gravity,
		deltaHeight);
	if ((xOffset == 0) && (yOffset == 0)) {
	    continue;
	}
	childPtr->x += xOffset;
	childPtr->y += yOffset;
	(void)X11WasmJsConfigureWindow(BrowserJsXId(childPtr->id),
		childPtr->x, childPtr->y, childPtr->width, childPtr->height,
		childPtr->borderWidth, CWX | CWY);
	(void)BrowserQueueGravityNotify(childPtr);
    }
}
