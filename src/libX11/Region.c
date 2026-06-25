#include "XlibInt.h"

void
BrowserRegionClear(
    X11WasmRegion *regionPtr)
{
    if (regionPtr == NULL) {
	return;
    }
    if (regionPtr->rects != NULL) {
	ckfree(regionPtr->rects);
    }
    regionPtr->rects = NULL;
    regionPtr->rectCount = 0;
    regionPtr->empty = 1;
    BrowserClearRectangle(&regionPtr->extents);
}

void
BrowserRegionSetRectangles(
    X11WasmRegion *regionPtr,
    const XRectangle *rects,
    int count)
{
    XRectangle extents;
    int rectCount = 0;
    XRectangle *copy = BrowserCopyRectangleList(rects, count, &rectCount,
	    &extents);

    if (regionPtr == NULL) {
	if (copy != NULL) {
	    ckfree(copy);
	}
	return;
    }
    BrowserRegionClear(regionPtr);
    regionPtr->rects = copy;
    regionPtr->rectCount = rectCount;
    regionPtr->empty = (rectCount == 0);
    regionPtr->extents = extents;
}

void
BrowserRegionAppendRectangle(
    X11WasmRegion *regionPtr,
    const XRectangle *rectPtr)
{
    int wasEmpty;

    if ((regionPtr == NULL) || BrowserRectangleIsEmpty(rectPtr)) {
	return;
    }
    wasEmpty = regionPtr->empty;
    BrowserAppendRectangle(&regionPtr->rects, &regionPtr->rectCount, rectPtr);
    BrowserUnionRectangle(&regionPtr->extents, &wasEmpty, rectPtr);
    regionPtr->empty = 0;
}

int
BrowserCeilDoubleToInt(
    double value)
{
    int truncated = (int)value;

    return ((double)truncated < value) ? truncated + 1 : truncated;
}

void
BrowserSortPolygonIntersections(
    BrowserPolygonIntersection *intersections,
    int count)
{
    int i, j;

    for (i = 1; i < count; i++) {
	BrowserPolygonIntersection key = intersections[i];

	j = i - 1;
	while ((j >= 0) && (intersections[j].x > key.x)) {
	    intersections[j + 1] = intersections[j];
	    j--;
	}
	intersections[j + 1] = key;
    }
}

void
BrowserAppendPolygonSpan(
    XRectangle **rectsPtr,
    int *countPtr,
    double left,
    double right,
    int y)
{
    XRectangle rect;
    XRectangle *lastPtr;
    int leftPixel, rightPixel;
    int spanWidth;

    if (right <= left) {
	return;
    }
    leftPixel = BrowserCeilDoubleToInt(left - 0.5);
    rightPixel = BrowserCeilDoubleToInt(right - 0.5);
    if (rightPixel <= leftPixel) {
	return;
    }
    spanWidth = rightPixel - leftPixel;
    if ((rectsPtr != NULL) && (*rectsPtr != NULL) && (countPtr != NULL)
	    && (*countPtr > 0)) {
	lastPtr = &(*rectsPtr)[*countPtr - 1];
	if ((lastPtr->x == (short)leftPixel)
		&& (spanWidth <= 0xffff)
		&& (lastPtr->width == (unsigned short)spanWidth)
		&& ((int)lastPtr->y + (int)lastPtr->height == y)
		&& (lastPtr->height < 0xffffU)) {
	    lastPtr->height++;
	    return;
	}
    }
    BrowserSetRectangleBounds(&rect, leftPixel, y, rightPixel, y + 1);
    BrowserAppendRectangle(rectsPtr, countPtr, &rect);
}

void
BrowserSubtractRectangle(
    const XRectangle *sourcePtr,
    const XRectangle *subtractPtr,
    XRectangle **rectsPtr,
    int *countPtr)
{
    XRectangle intersection;
    int sourceRight, sourceBottom, interRight, interBottom;
    XRectangle piece;

    if (BrowserRectangleIsEmpty(sourcePtr)) {
	return;
    }
    if (!BrowserIntersectRectangles(sourcePtr, subtractPtr, &intersection)) {
	BrowserAppendRectangle(rectsPtr, countPtr, sourcePtr);
	return;
    }

    sourceRight = sourcePtr->x + (int)sourcePtr->width;
    sourceBottom = sourcePtr->y + (int)sourcePtr->height;
    interRight = intersection.x + (int)intersection.width;
    interBottom = intersection.y + (int)intersection.height;

    BrowserSetRectangleBounds(&piece, sourcePtr->x, sourcePtr->y,
	    sourceRight, intersection.y);
    BrowserAppendRectangle(rectsPtr, countPtr, &piece);

    BrowserSetRectangleBounds(&piece, sourcePtr->x, interBottom,
	    sourceRight, sourceBottom);
    BrowserAppendRectangle(rectsPtr, countPtr, &piece);

    BrowserSetRectangleBounds(&piece, sourcePtr->x, intersection.y,
	    intersection.x, interBottom);
    BrowserAppendRectangle(rectsPtr, countPtr, &piece);

    BrowserSetRectangleBounds(&piece, interRight, intersection.y,
	    sourceRight, interBottom);
    BrowserAppendRectangle(rectsPtr, countPtr, &piece);
}

int
BrowserCopyDestinationRectangle(
    X11WasmResource *sourcePtr,
    X11WasmResource *targetPtr,
    int srcX,
    int srcY,
    unsigned int width,
    unsigned int height,
    int destX,
    int destY,
    XRectangle *rectPtr)
{
    if (!BrowserNormalizeCopyArea(sourcePtr, targetPtr, &srcX, &srcY,
	    &width, &height, &destX, &destY)) {
	BrowserClearRectangle(rectPtr);
	return 0;
    }
    BrowserSetRectangleBounds(rectPtr, destX, destY, destX + (int)width,
	    destY + (int)height);
    return !BrowserRectangleIsEmpty(rectPtr);
}

int
BrowserCopyTargetRectangle(
    X11WasmResource *targetPtr,
    unsigned int width,
    unsigned int height,
    int destX,
    int destY,
    XRectangle *rectPtr)
{
    XRectangle targetBounds;
    XRectangle requestedRect;

    BrowserClearRectangle(rectPtr);
    if ((targetPtr == NULL) || (width == 0) || (height == 0)) {
	return 0;
    }
    BrowserSetRectangleBounds(&targetBounds, 0, 0, (int)targetPtr->width,
	    (int)targetPtr->height);
    BrowserSetRectangleBounds(&requestedRect, destX, destY,
	    destX + (int)width, destY + (int)height);
    return BrowserIntersectRectangles(&targetBounds, &requestedRect, rectPtr);
}

int
BrowserQueueCopyExposureEvents(
    X11WasmResource *sourcePtr,
    X11WasmResource *targetPtr,
    Drawable drawable,
    GC gc,
    int srcX,
    int srcY,
    unsigned int width,
    unsigned int height,
    int destX,
    int destY,
    unsigned char requestCode)
{
    XRectangle targetRect;
    XRectangle copiedRect;
    XRectangle copiedTargetRect;
    XRectangle *exposedRects = NULL;
    int exposedCount = 0;
    int i;
    int copied = 0;

    if ((gc == NULL) || !gc->graphics_exposures) {
	return 0;
    }
    if (!BrowserCopyTargetRectangle(targetPtr, width, height, destX, destY,
	    &targetRect)) {
	return BrowserQueueNoExposeEvent(drawable, requestCode);
    }
    if ((sourcePtr != NULL)
	    && ((sourcePtr->type != X11_WASM_RESOURCE_WINDOW)
		    || BrowserResourceIsVisible(sourcePtr))
	    && BrowserCopyDestinationRectangle(sourcePtr, targetPtr, srcX, srcY,
		    width, height, destX, destY, &copiedRect)) {
	copied = BrowserIntersectRectangles(&targetRect, &copiedRect,
		&copiedTargetRect);
    }
    if (copied) {
	BrowserSubtractRectangle(&targetRect, &copiedTargetRect, &exposedRects,
		&exposedCount);
    } else {
	BrowserAppendRectangle(&exposedRects, &exposedCount, &targetRect);
    }
    if (exposedCount == 0) {
	return BrowserQueueNoExposeEvent(drawable, requestCode);
    }
    for (i = 0; i < exposedCount; i++) {
	(void)BrowserQueueGraphicsExposeEvent(drawable, &exposedRects[i],
		exposedCount - i - 1, requestCode);
    }
    ckfree(exposedRects);
    return exposedCount;
}

int
BrowserRectangleCoveredByList(
    const XRectangle *rectPtr,
    const XRectangle *rects,
    int count)
{
    XRectangle *remaining = NULL;
    int remainingCount = 0;
    int i, j;

    if (BrowserRectangleIsEmpty(rectPtr)) {
	return 1;
    }
    BrowserAppendRectangle(&remaining, &remainingCount, rectPtr);
    for (i = 0; i < count && remainingCount > 0; i++) {
	XRectangle *next = NULL;
	int nextCount = 0;

	for (j = 0; j < remainingCount; j++) {
	    BrowserSubtractRectangle(&remaining[j], &rects[i], &next,
		    &nextCount);
	}
	if (remaining != NULL) {
	    ckfree(remaining);
	}
	remaining = next;
	remainingCount = nextCount;
    }
    if (remaining != NULL) {
	ckfree(remaining);
    }
    return remainingCount == 0;
}
