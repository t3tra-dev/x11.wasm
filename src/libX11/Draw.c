#include "XlibInt.h"

#define BROWSER_COLOR_CHANNEL_BITS 8U
#define BROWSER_COLOR_RED_SHIFT 16U
#define BROWSER_COLOR_GREEN_SHIFT 8U
#define BROWSER_COLOR_BLUE_SHIFT 0U
#define BROWSER_MAX_COLOR_CELL_COMBINATIONS 4096U

static int
BrowserValidateDrawableDimensions(
    Display **displayPtr,
    unsigned int width,
    unsigned int height,
    unsigned char requestCode)
{
    return BrowserValidateCard16(displayPtr, (long)width, 1, requestCode)
	    && BrowserValidateCard16(displayPtr, (long)height, 1,
		    requestCode);
}

static int
BrowserValidateColorFlags(
    Display **displayPtr,
    int flags,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, flags,
	    (flags & ~(DoRed | DoGreen | DoBlue)) == 0, requestCode);
}

static void
BrowserColorFromPixel(
    unsigned long pixel,
    XColor *colorPtr)
{
    if (colorPtr == NULL) {
	return;
    }
    colorPtr->red = (unsigned short)(((pixel >> 16) & 0xff) * 257U);
    colorPtr->green = (unsigned short)(((pixel >> 8) & 0xff) * 257U);
    colorPtr->blue = (unsigned short)((pixel & 0xff) * 257U);
    colorPtr->flags = DoRed | DoGreen | DoBlue;
}

static void
BrowserStoreColorDefinition(
    Colormap colormap,
    const XColor *definitionPtr)
{
    XColor color;

    if (definitionPtr == NULL) {
	return;
    }
    color = *definitionPtr;
    if (BrowserLookupColorEntry(colormap, color.pixel, &color)) {
	if (definitionPtr->flags & DoRed) {
	    color.red = definitionPtr->red;
	}
	if (definitionPtr->flags & DoGreen) {
	    color.green = definitionPtr->green;
	}
	if (definitionPtr->flags & DoBlue) {
	    color.blue = definitionPtr->blue;
	}
	color.flags |= (unsigned char)(definitionPtr->flags
		& (DoRed | DoGreen | DoBlue));
    } else {
	if (!(definitionPtr->flags & DoRed)) {
	    color.red = (unsigned short)(((color.pixel >> 16) & 0xff) * 257U);
	}
	if (!(definitionPtr->flags & DoGreen)) {
	    color.green = (unsigned short)(((color.pixel >> 8) & 0xff) * 257U);
	}
	if (!(definitionPtr->flags & DoBlue)) {
	    color.blue = (unsigned short)((color.pixel & 0xff) * 257U);
	}
	color.flags = (unsigned char)(definitionPtr->flags
		& (DoRed | DoGreen | DoBlue));
	if (color.flags == 0) {
	    color.flags = DoRed | DoGreen | DoBlue;
	}
    }
    BrowserRememberColor(colormap, &color);
}

static unsigned long
BrowserColorCellPlaneBits(
    const unsigned long *planeMasks,
    unsigned int nplanes,
    size_t combination)
{
    unsigned long planeBits = 0;
    unsigned int i;

    for (i = 0; i < nplanes; i++) {
	if (combination & ((size_t)1 << i)) {
	    planeBits |= planeMasks[i];
	}
    }
    return planeBits;
}

static int
BrowserAppendColorPlaneMasks(
    unsigned long *planeMasks,
    unsigned int planeMaskCapacity,
    int planeCount,
    unsigned int shift,
    unsigned int *totalPlaneCountPtr,
    unsigned long *maskReturn)
{
    unsigned long mask = 0;
    unsigned int totalPlaneCount;
    int i;

    if ((planeCount < 0)
	    || (planeCount > (int)BROWSER_COLOR_CHANNEL_BITS)
	    || (shift + (unsigned int)planeCount
	    > (unsigned int)(sizeof(unsigned long) * CHAR_BIT))) {
	return 0;
    }
    totalPlaneCount = *totalPlaneCountPtr;
    if (totalPlaneCount + (unsigned int)planeCount > planeMaskCapacity) {
	return 0;
    }
    for (i = 0; i < planeCount; i++) {
	unsigned long plane = 1UL << (shift + (unsigned int)i);

	planeMasks[totalPlaneCount++] = plane;
	mask |= plane;
    }
    *totalPlaneCountPtr = totalPlaneCount;
    *maskReturn = mask;
    return 1;
}

static int
BrowserColorCellCombinationCount(
    unsigned int nplanes,
    size_t *combinationCountPtr)
{
    size_t combinationCount;

    if (nplanes >= (unsigned int)(sizeof(size_t) * CHAR_BIT)) {
	return 0;
    }
    combinationCount = ((size_t)1) << nplanes;
    if (combinationCount > (size_t)BROWSER_MAX_COLOR_CELL_COMBINATIONS) {
	return 0;
    }
    if (combinationCountPtr != NULL) {
	*combinationCountPtr = combinationCount;
    }
    return 1;
}

static int
BrowserColorCellBlockIsFree(
    Colormap colormap,
    unsigned long basePixel,
    const unsigned long *planeMasks,
    unsigned int nplanes,
    size_t combinationCount)
{
    size_t combination;

    for (combination = 0; combination < combinationCount; combination++) {
	unsigned long pixel = basePixel | BrowserColorCellPlaneBits(
		planeMasks, nplanes, combination);

	if (BrowserLookupColorEntry(colormap, pixel, NULL)) {
	    return 0;
	}
    }
    return 1;
}

static void
BrowserRememberColorCellBlock(
    Colormap colormap,
    unsigned long basePixel,
    const unsigned long *planeMasks,
    unsigned int nplanes,
    size_t combinationCount)
{
    XColor color;
    size_t combination;

    memset(&color, 0, sizeof(color));
    color.flags = DoRed | DoGreen | DoBlue;
    for (combination = 0; combination < combinationCount; combination++) {
	color.pixel = basePixel | BrowserColorCellPlaneBits(planeMasks,
		nplanes, combination);
	BrowserRememberColor(colormap, &color);
    }
}

static int
BrowserReserveColorCellBlock(
    Colormap colormap,
    const unsigned long *planeMasks,
    unsigned int nplanes,
    unsigned long planeMaskUnion,
    unsigned long *pixelReturn)
{
    size_t combinationCount;
    unsigned long basePixel;

    if (!BrowserColorCellCombinationCount(nplanes, &combinationCount)) {
	return 0;
    }
    for (basePixel = 0; ; basePixel++) {
	if ((basePixel & planeMaskUnion) != 0) {
	    continue;
	}
	if (BrowserColorCellBlockIsFree(colormap, basePixel, planeMasks,
		nplanes, combinationCount)) {
	    BrowserRememberColorCellBlock(colormap, basePixel, planeMasks,
		    nplanes, combinationCount);
	    *pixelReturn = basePixel;
	    return 1;
	}
    }
}

int XCopyArea(Display *d, Drawable s, Drawable t, GC g, int sx, int sy,
	unsigned int w, unsigned int h, int dx, int dy)
{
    X11WasmResource *sourcePtr = BrowserDrawableResourceOrError(&d, s,
	    X11_WASM_XREQ_COPY_AREA);
    X11WasmResource *targetPtr;

    if (sourcePtr == NULL) {
	return Success;
    }
    targetPtr = BrowserDrawableResourceOrError(&d, t,
	    X11_WASM_XREQ_COPY_AREA);
    if (targetPtr == NULL) {
	return Success;
    }
    if (!BrowserDrawableDepthsMatchOrError(&d, sourcePtr, targetPtr, t,
	    X11_WASM_XREQ_COPY_AREA)) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_COPY_AREA)) {
	return Success;
    }
    if (!BrowserValidateDrawableDimensions(&d, w, h,
	    X11_WASM_XREQ_COPY_AREA)) {
	return Success;
    }
    BrowserApplyGCStyle(g);
    BrowserApplyGCClip(g);
    BrowserStorePixmapCopy(d, s, sourcePtr, targetPtr, g, sx, sy, w, h, dx,
	    dy, 0, AllPlanes, 0);
    (void)X11WasmJsCopyArea(BrowserJsXId(s), BrowserJsXId(t), sx, sy, w, h,
	    dx, dy);
    BrowserMarkResourceDamage(targetPtr, dx, dy, w, h);
    (void)BrowserQueueCopyExposureEvents(sourcePtr, targetPtr, t, g, sx, sy,
	    w, h, dx, dy, X11_WASM_XREQ_COPY_AREA);
    return Success;
}
int XCopyPlane(Display *d, Drawable s, Drawable t, GC g, int sx, int sy,
	unsigned int w, unsigned int h, int dx, int dy, unsigned long p)
{
    X11WasmResource *sourcePtr = BrowserDrawableResourceOrError(&d, s,
	    X11_WASM_XREQ_COPY_PLANE);
    X11WasmResource *targetPtr;

    if (sourcePtr == NULL) {
	return Success;
    }
    targetPtr = BrowserDrawableResourceOrError(&d, t,
	    X11_WASM_XREQ_COPY_PLANE);
    if (targetPtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_COPY_PLANE)) {
	return Success;
    }
    if (!BrowserValidateDrawableDimensions(&d, w, h,
	    X11_WASM_XREQ_COPY_PLANE)) {
	return Success;
    }
    BrowserApplyGCStyle(g);
    BrowserApplyGCClip(g);
    BrowserStorePixmapCopy(d, s, sourcePtr, targetPtr, g, sx, sy, w, h, dx,
	    dy, 1, p, (g != NULL) && (g->clip_mask == s));
    (void)X11WasmJsCopyPlane(BrowserJsXId(s), BrowserJsXId(t), sx, sy,
	    w, h, dx, dy, (g != NULL) ? g->foreground : 0,
	    (g != NULL) ? g->background : 0xffffff, p,
	    (g != NULL) && (g->clip_mask == s));
    BrowserMarkResourceDamage(targetPtr, dx, dy, w, h);
    (void)BrowserQueueCopyExposureEvents(sourcePtr, targetPtr, t, g, sx, sy,
	    w, h, dx, dy, X11_WASM_XREQ_COPY_PLANE);
    return Success;
}
int XDrawArc(Display *d, Drawable dr, GC g, int x, int y, unsigned int w,
	unsigned int h, int a1, int a2)
{
    X11WasmResource *resourcePtr = BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_ARC);
    unsigned int lineWidth;
    int pad;

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_ARC)) {
	return Success;
    }
    if (!BrowserValidateDrawableDimensions(&d, w, h,
	    X11_WASM_XREQ_POLY_ARC)) {
	return Success;
    }
    lineWidth = BrowserGCLineWidth(g);
    pad = (int)((lineWidth + 1U) / 2U) + 1;
    BrowserApplyGCStyle(g);
    BrowserApplyGCClip(g);
    (void)X11WasmJsDrawArc(BrowserJsXId(dr), BrowserGCPixel(g), x, y, w, h,
	    a1, a2, 0, 0, lineWidth);
    BrowserMarkResourceDamage(resourcePtr, x - pad, y - pad,
	    w + (unsigned int)(pad * 2), h + (unsigned int)(pad * 2));
    return Success;
}
int XDrawArcs(Display *d, Drawable dr, GC g, XArc *a, int n)
{
    int i;

    if ((a == NULL) || (n <= 0)) {
	return Success;
    }
    if (BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_ARC) == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_ARC)) {
	return Success;
    }
    for (i = 0; i < n; i++) {
	(void)XDrawArc(d, dr, g, a[i].x, a[i].y, a[i].width, a[i].height,
		a[i].angle1, a[i].angle2);
    }
    return Success;
}
int XDrawLine(Display *d, Drawable dr, GC g, int x1, int y1, int x2, int y2)
{
    X11WasmResource *resourcePtr = BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_LINE);
    int x = (x1 < x2) ? x1 : x2;
    int y = (y1 < y2) ? y1 : y2;
    unsigned int width = (unsigned int)(((x1 > x2) ? x1 - x2 : x2 - x1) + 1);
    unsigned int height = (unsigned int)(((y1 > y2) ? y1 - y2 : y2 - y1) + 1);

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_LINE)) {
	return Success;
    }
    BrowserApplyGCStyle(g);
    BrowserApplyGCClip(g);
    (void)X11WasmJsDrawLine(BrowserJsXId(dr), BrowserGCPixel(g), x1, y1,
	    x2, y2);
    BrowserMarkResourceDamage(resourcePtr, x, y, width, height);
    return Success;
}
int XDrawLines(Display *d, Drawable dr, GC g, XPoint *p, int n, int m)
{
    int i;
    int x;
    int y;

    if ((p == NULL) || (n <= 1)) {
	return Success;
    }
    if (BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_LINE) == NULL) {
	return Success;
    }
    if (!BrowserValidateCoordMode(&d, m, X11_WASM_XREQ_POLY_LINE)) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_LINE)) {
	return Success;
    }
    x = p[0].x;
    y = p[0].y;
    for (i = 1; i < n; i++) {
	int nextX = p[i].x;
	int nextY = p[i].y;

	if (m == CoordModePrevious) {
	    nextX += x;
	    nextY += y;
	}
	(void)XDrawLine(d, dr, g, x, y, nextX, nextY);
	x = nextX;
	y = nextY;
    }
    return Success;
}
int XDrawPoint(Display *d, Drawable dr, GC g, int x, int y)
{
    X11WasmResource *resourcePtr = BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_POINT);

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_POINT)) {
	return Success;
    }
    BrowserApplyGCStyle(g);
    BrowserApplyGCClip(g);
    (void)X11WasmJsDrawRectangle(BrowserJsXId(dr), BrowserGCPixel(g), x, y,
	    1, 1, 1);
    BrowserMarkResourceDamage(resourcePtr, x, y, 1, 1);
    return Success;
}
int XDrawPoints(Display *d, Drawable dr, GC g, XPoint *p, int n, int m)
{
    int i;
    int x = 0;
    int y = 0;

    if ((p == NULL) || (n <= 0)) {
	return Success;
    }
    if (BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_POINT) == NULL) {
	return Success;
    }
    if (!BrowserValidateCoordMode(&d, m, X11_WASM_XREQ_POLY_POINT)) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_POINT)) {
	return Success;
    }
    for (i = 0; i < n; i++) {
	if ((m == CoordModePrevious) && (i > 0)) {
	    x += p[i].x;
	    y += p[i].y;
	} else {
	    x = p[i].x;
	    y = p[i].y;
	}
	(void)XDrawPoint(d, dr, g, x, y);
    }
    return Success;
}
int XDrawRectangle(Display *d, Drawable dr, GC g, int x, int y,
	unsigned int w, unsigned int h)
{
    X11WasmResource *resourcePtr = BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_RECTANGLE);

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_RECTANGLE)) {
	return Success;
    }
    if (!BrowserValidateDrawableDimensions(&d, w, h,
	    X11_WASM_XREQ_POLY_RECTANGLE)) {
	return Success;
    }
    BrowserApplyGCStyle(g);
    BrowserApplyGCClip(g);
    (void)X11WasmJsDrawRectangle(BrowserJsXId(dr), BrowserGCPixel(g), x, y,
	    w, h, 0);
    BrowserMarkResourceDamage(resourcePtr, x, y, w + 1, h + 1);
    return Success;
}
int XDrawRectangles(Display *d, Drawable dr, GC g, XRectangle *r, int n)
{
    int i;

    if ((r == NULL) || (n <= 0)) {
	return Success;
    }
    if (BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_RECTANGLE) == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_RECTANGLE)) {
	return Success;
    }
    for (i = 0; i < n; i++) {
	(void)XDrawRectangle(d, dr, g, r[i].x, r[i].y, r[i].width,
		r[i].height);
    }
    return Success;
}
int XDrawSegments(Display *d, Drawable dr, GC g, XSegment *s, int n)
{
    int i;

    if ((s == NULL) || (n <= 0)) {
	return Success;
    }
    if (BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_SEGMENT) == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_SEGMENT)) {
	return Success;
    }
    for (i = 0; i < n; i++) {
	(void)XDrawLine(d, dr, g, s[i].x1, s[i].y1, s[i].x2, s[i].y2);
    }
    return Success;
}
int XFillArc(Display *d, Drawable dr, GC g, int x, int y, unsigned int w,
	unsigned int h, int a1, int a2)
{
    X11WasmResource *resourcePtr = BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_FILL_ARC);
    int pieSlice;

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_FILL_ARC)) {
	return Success;
    }
    if (!BrowserValidateDrawableDimensions(&d, w, h,
	    X11_WASM_XREQ_POLY_FILL_ARC)) {
	return Success;
    }
    pieSlice = (g == NULL) || (g->arc_mode == ArcPieSlice);
    BrowserApplyGCStyle(g);
    BrowserApplyGCClip(g);
    (void)X11WasmJsDrawArc(BrowserJsXId(dr), BrowserGCPixel(g), x, y, w, h,
	    a1, a2, 1, pieSlice, BrowserGCLineWidth(g));
    BrowserMarkResourceDamage(resourcePtr, x, y, w, h);
    return Success;
}
int XFillArcs(Display *d, Drawable dr, GC g, XArc *a, int n)
{
    int i;

    if ((a == NULL) || (n <= 0)) {
	return Success;
    }
    if (BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_FILL_ARC) == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_POLY_FILL_ARC)) {
	return Success;
    }
    for (i = 0; i < n; i++) {
	(void)XFillArc(d, dr, g, a[i].x, a[i].y, a[i].width, a[i].height,
		a[i].angle1, a[i].angle2);
    }
    return Success;
}
int XFillPolygon(Display *d, Drawable dr, GC g, XPoint *p, int n, int shape,
	int mode)
{
    X11WasmResource *resourcePtr;
    int *coords;
    int i;
    int x = 0;
    int y = 0;
    int minX = 0, minY = 0, maxX = 0, maxY = 0;
    int fillRule;

    if ((p == NULL) || (n < 3)) {
	return Success;
    }
    resourcePtr = BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_FILL_POLY);
    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserValidatePolygonShape(&d, shape, X11_WASM_XREQ_FILL_POLY)
	    || !BrowserValidateCoordMode(&d, mode, X11_WASM_XREQ_FILL_POLY)) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g, X11_WASM_XREQ_FILL_POLY)) {
	return Success;
    }
    fillRule = (g != NULL) ? g->fill_rule : EvenOddRule;
    coords = (int *)ckalloc(sizeof(int) * (size_t)n * 2U);
    for (i = 0; i < n; i++) {
	if ((mode == CoordModePrevious) && (i > 0)) {
	    x += p[i].x;
	    y += p[i].y;
	} else {
	    x = p[i].x;
	    y = p[i].y;
	}
	coords[i * 2] = x;
	coords[i * 2 + 1] = y;
	if (i == 0) {
	    minX = maxX = x;
	    minY = maxY = y;
	} else {
	    if (x < minX) minX = x;
	    if (x > maxX) maxX = x;
	    if (y < minY) minY = y;
	    if (y > maxY) maxY = y;
	}
    }
    BrowserApplyGCStyle(g);
    BrowserApplyGCClip(g);
    (void)X11WasmJsFillPolygon(BrowserJsXId(dr), BrowserGCPixel(g),
	    coords, n, fillRule);
    BrowserMarkResourceDamage(resourcePtr, minX, minY,
	    (unsigned int)(maxX - minX + 1), (unsigned int)(maxY - minY + 1));
    ckfree(coords);
    return Success;
}
int XFillRectangle(Display *d, Drawable dr, GC g, int x, int y,
	unsigned int w, unsigned int h)
{
    X11WasmResource *resourcePtr = BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_FILL_RECTANGLE);

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g,
	    X11_WASM_XREQ_POLY_FILL_RECTANGLE)) {
	return Success;
    }
    if (!BrowserValidateDrawableDimensions(&d, w, h,
	    X11_WASM_XREQ_POLY_FILL_RECTANGLE)) {
	return Success;
    }
    BrowserApplyGCStyle(g);
    BrowserApplyGCClip(g);
    (void)X11WasmJsDrawRectangle(BrowserJsXId(dr), BrowserGCPixel(g), x, y,
	    w, h, 1);
    BrowserMarkResourceDamage(resourcePtr, x, y, w, h);
    return Success;
}
int XFillRectangles(Display *d, Drawable dr, GC g, XRectangle *r, int n)
{
    int i;

    if ((r == NULL) || (n <= 0)) {
	return Success;
    }
    if (BrowserDrawableResourceOrError(&d, dr,
	    X11_WASM_XREQ_POLY_FILL_RECTANGLE) == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&d, g,
	    X11_WASM_XREQ_POLY_FILL_RECTANGLE)) {
	return Success;
    }
    for (i = 0; i < n; i++) {
	(void)XFillRectangle(d, dr, g, r[i].x, r[i].y, r[i].width,
		r[i].height);
    }
    return Success;
}

Status
XAllocColor(
    Display *display,
    Colormap colormap,
    XColor *color)
{
    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_ALLOC_COLOR, 0)) {
	return False;
    }
    if (color != NULL) {
	color->flags = DoRed | DoGreen | DoBlue;
	color->pixel = ((unsigned long)(color->red >> 8) << 16)
		| ((unsigned long)(color->green >> 8) << 8)
		| (unsigned long)(color->blue >> 8);
	BrowserRememberColor(colormap, color);
    }
    return True;
}

Status
XAllocNamedColor(
    Display *display,
    Colormap colormap,
    _Xconst char *name,
    XColor *screenDefReturn,
    XColor *exactDefReturn)
{
    XColor exactColor;
    XColor screenColor;

    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_ALLOC_NAMED_COLOR, 0)) {
	return False;
    }
    if (!BrowserParseColor(name, &exactColor)) {
	return False;
    }
    screenColor = exactColor;
    if (!XAllocColor(display, colormap, &screenColor)) {
	return False;
    }
    if (screenDefReturn != NULL) *screenDefReturn = screenColor;
    if (exactDefReturn != NULL) *exactDefReturn = exactColor;
    return True;
}

Status
XAllocColorCells(
    Display *display,
    Colormap colormap,
    Bool contig,
    unsigned long *planeMasksReturn,
    unsigned int nplanes,
    unsigned long *pixelsReturn,
    unsigned int npixels)
{
    unsigned int i;
    unsigned long planeMaskUnion = 0;

    (void)contig;
    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_ALLOC_COLOR_CELLS, 0)) {
	return False;
    }
    if (!BrowserValueOrError(&display, (long)nplanes,
	    nplanes <= (unsigned int)INT_MAX,
	    X11_WASM_XREQ_ALLOC_COLOR_CELLS)
	    || !BrowserValueOrError(&display, (long)npixels,
		    npixels <= (unsigned int)INT_MAX,
		    X11_WASM_XREQ_ALLOC_COLOR_CELLS)
	    || !BrowserValidateCountedList(&display, planeMasksReturn,
		    (int)nplanes, X11_WASM_XREQ_ALLOC_COLOR_CELLS)
	    || !BrowserValidateCountedList(&display, pixelsReturn,
		    (int)npixels, X11_WASM_XREQ_ALLOC_COLOR_CELLS)) {
	return False;
    }
    if (!BrowserColorCellCombinationCount(nplanes, NULL)) {
	return False;
    }
    for (i = 0; i < nplanes; i++) {
	planeMasksReturn[i] = (i < (unsigned int)(sizeof(unsigned long)
		* CHAR_BIT)) ? (1UL << i) : 0UL;
	planeMaskUnion |= planeMasksReturn[i];
    }
    for (i = 0; i < npixels; i++) {
	if (!BrowserReserveColorCellBlock(colormap, planeMasksReturn, nplanes,
		planeMaskUnion, &pixelsReturn[i])) {
	    return False;
	}
    }
    return True;
}

Status
XAllocColorPlanes(
    Display *display,
    Colormap colormap,
    Bool contig,
    unsigned long *pixelsReturn,
    int ncolors,
    int nreds,
    int ngreens,
    int nblues,
    unsigned long *rmaskReturn,
    unsigned long *gmaskReturn,
    unsigned long *bmaskReturn)
{
    int i;
    unsigned long planeMasks[sizeof(unsigned long) * CHAR_BIT];
    unsigned int totalPlaneCount = 0;
    unsigned long redMask = 0;
    unsigned long greenMask = 0;
    unsigned long blueMask = 0;
    unsigned long planeMaskUnion;

    (void)contig;
    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_ALLOC_COLOR_PLANES, 0)) {
	return False;
    }
    if (!BrowserValidateCountedList(&display, pixelsReturn, ncolors,
	    X11_WASM_XREQ_ALLOC_COLOR_PLANES)
	    || !BrowserValueOrError(&display, nreds,
		    (nreds >= 0) && (nreds <= (int)BROWSER_COLOR_CHANNEL_BITS),
		    X11_WASM_XREQ_ALLOC_COLOR_PLANES)
	    || !BrowserValueOrError(&display, ngreens,
		    (ngreens >= 0)
		    && (ngreens <= (int)BROWSER_COLOR_CHANNEL_BITS),
		    X11_WASM_XREQ_ALLOC_COLOR_PLANES)
	    || !BrowserValueOrError(&display, nblues,
		    (nblues >= 0)
		    && (nblues <= (int)BROWSER_COLOR_CHANNEL_BITS),
		    X11_WASM_XREQ_ALLOC_COLOR_PLANES)
	    || !BrowserValueOrError(&display, 0,
		    (rmaskReturn != NULL) && (gmaskReturn != NULL)
		    && (bmaskReturn != NULL),
		    X11_WASM_XREQ_ALLOC_COLOR_PLANES)) {
	return False;
    }
    if (!BrowserAppendColorPlaneMasks(planeMasks,
	    (unsigned int)(sizeof(planeMasks) / sizeof(planeMasks[0])),
	    nreds, BROWSER_COLOR_RED_SHIFT, &totalPlaneCount, &redMask)
	    || !BrowserAppendColorPlaneMasks(planeMasks,
		    (unsigned int)(sizeof(planeMasks) / sizeof(planeMasks[0])),
		    ngreens, BROWSER_COLOR_GREEN_SHIFT, &totalPlaneCount,
		    &greenMask)
	    || !BrowserAppendColorPlaneMasks(planeMasks,
		    (unsigned int)(sizeof(planeMasks) / sizeof(planeMasks[0])),
		    nblues, BROWSER_COLOR_BLUE_SHIFT, &totalPlaneCount,
		    &blueMask)
	    || !BrowserColorCellCombinationCount(totalPlaneCount, NULL)) {
	return False;
    }
    *rmaskReturn = redMask;
    *gmaskReturn = greenMask;
    *bmaskReturn = blueMask;
    planeMaskUnion = redMask | greenMask | blueMask;
    for (i = 0; i < ncolors; i++) {
	if (!BrowserReserveColorCellBlock(colormap, planeMasks,
		totalPlaneCount, planeMaskUnion, &pixelsReturn[i])) {
	    return False;
	}
    }
    return True;
}

Status
XLookupColor(
    Display *display,
    Colormap colormap,
    _Xconst char *name,
    XColor *exactDefReturn,
    XColor *screenDefReturn)
{
    XColor color;

    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_LOOKUP_COLOR, 0)) {
	return False;
    }
    if (!BrowserParseColor(name, &color)) {
	return False;
    }
    if (exactDefReturn != NULL) {
	*exactDefReturn = color;
    }
    if (screenDefReturn != NULL) {
	*screenDefReturn = color;
    }
    return True;
}

Status
XParseColor(
    Display *display,
    Colormap colormap,
    _Xconst char *spec,
    XColor *color)
{
    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_LOOKUP_COLOR, 0)) {
	return False;
    }
    return BrowserParseColor(spec, color) ? True : False;
}

int
XQueryColor(
    Display *display,
    Colormap colormap,
    XColor *color)
{
    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_QUERY_COLORS, 0)) {
	return Success;
    }
    if (color != NULL) {
	if (!BrowserLookupColorEntry(colormap, color->pixel, color)) {
	    BrowserColorFromPixel(color->pixel, color);
	}
    }
    return Success;
}

int
XQueryColors(
    Display *display,
    Colormap colormap,
    XColor *defs,
    int ncolors)
{
    int i;

    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_QUERY_COLORS, 0)) {
	return Success;
    }
    if (!BrowserValidateCountedList(&display, defs, ncolors,
	    X11_WASM_XREQ_QUERY_COLORS)) {
	return Success;
    }
    for (i = 0; i < ncolors; i++) {
	XQueryColor(display, colormap, &defs[i]);
    }
    return Success;
}

int
XStoreColor(
    Display *display,
    Colormap colormap,
    XColor *color)
{
    return XStoreColors(display, colormap, color, (color != NULL) ? 1 : 0);
}

int
XStoreColors(
    Display *display,
    Colormap colormap,
    XColor *defs,
    int ncolors)
{
    int i;

    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_STORE_COLORS, 0)) {
	return Success;
    }
    if (!BrowserValidateCountedList(&display, defs, ncolors,
	    X11_WASM_XREQ_STORE_COLORS)) {
	return Success;
    }
    for (i = 0; i < ncolors; i++) {
	if (!BrowserValidateColorFlags(&display, defs[i].flags,
		X11_WASM_XREQ_STORE_COLORS)) {
	    return Success;
	}
    }
    for (i = 0; i < ncolors; i++) {
	BrowserStoreColorDefinition(colormap, &defs[i]);
    }
    return Success;
}

int
XStoreNamedColor(
    Display *display,
    Colormap colormap,
    _Xconst char *name,
    unsigned long pixel,
    int flags)
{
    XColor color;

    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_STORE_NAMED_COLOR, 0)) {
	return Success;
    }
    if (!BrowserValidateColorFlags(&display, flags,
	    X11_WASM_XREQ_STORE_NAMED_COLOR)) {
	return Success;
    }
    if (!BrowserParseColor(name, &color)) {
	(void)BrowserReportXError(display, BadName, (XID)colormap,
		X11_WASM_XREQ_STORE_NAMED_COLOR, 0);
	return Success;
    }
    color.pixel = pixel;
    color.flags = (unsigned char)(flags & (DoRed | DoGreen | DoBlue));
    BrowserStoreColorDefinition(colormap, &color);
    return Success;
}

void
BrowserFillVisualInfo(
    XVisualInfo *infoPtr,
    Visual *visualPtr,
    int screen,
    int depth)
{
    memset(infoPtr, 0, sizeof(*infoPtr));
    infoPtr->visual = visualPtr;
    infoPtr->visualid = (visualPtr != NULL) ? visualPtr->visualid : 0;
    infoPtr->screen = screen;
    infoPtr->depth = depth;
    if (visualPtr != NULL) {
	infoPtr->c_class = visualPtr->c_class;
	infoPtr->red_mask = visualPtr->red_mask;
	infoPtr->green_mask = visualPtr->green_mask;
	infoPtr->blue_mask = visualPtr->blue_mask;
	infoPtr->colormap_size = visualPtr->map_entries;
	infoPtr->bits_per_rgb = visualPtr->bits_per_rgb;
    }
}

int
BrowserVisualInfoMatches(
    const XVisualInfo *infoPtr,
    long mask,
    const XVisualInfo *templatePtr)
{
    if (mask == VisualNoMask) {
	return 1;
    }
    if (templatePtr == NULL) {
	return 0;
    }
    if (((mask & VisualIDMask) && (templatePtr->visualid != infoPtr->visualid))
	    || ((mask & VisualScreenMask)
	    && (templatePtr->screen != infoPtr->screen))
	    || ((mask & VisualDepthMask)
	    && (templatePtr->depth != infoPtr->depth))
	    || ((mask & VisualClassMask)
	    && (templatePtr->c_class != infoPtr->c_class))
	    || ((mask & VisualRedMaskMask)
	    && (templatePtr->red_mask != infoPtr->red_mask))
	    || ((mask & VisualGreenMaskMask)
	    && (templatePtr->green_mask != infoPtr->green_mask))
	    || ((mask & VisualBlueMaskMask)
	    && (templatePtr->blue_mask != infoPtr->blue_mask))
	    || ((mask & VisualColormapSizeMask)
	    && (templatePtr->colormap_size != infoPtr->colormap_size))
	    || ((mask & VisualBitsPerRGBMask)
	    && (templatePtr->bits_per_rgb != infoPtr->bits_per_rgb))) {
	return 0;
    }
    return 1;
}

int
BrowserCollectVisualInfos(
    Display *display,
    long mask,
    const XVisualInfo *templatePtr,
    XVisualInfo *infoList)
{
    XVisualInfo info;
    int count = 0;
    int screenCount = ScreenCount(display);
    int screenIndex;
    int depthIndex;
    int visualIndex;

    for (screenIndex = 0; screenIndex < screenCount; screenIndex++) {
	Screen *screenPtr = ScreenOfDisplay(display, screenIndex);

	if ((screenPtr == NULL) || (screenPtr->depths == NULL)) {
	    continue;
	}
	for (depthIndex = 0; depthIndex < screenPtr->ndepths; depthIndex++) {
	    Depth *depthPtr = &screenPtr->depths[depthIndex];

	    if (depthPtr->visuals == NULL) {
		continue;
	    }
	    for (visualIndex = 0; visualIndex < depthPtr->nvisuals;
		    visualIndex++) {
		BrowserFillVisualInfo(&info, &depthPtr->visuals[visualIndex],
			screenIndex, depthPtr->depth);
		if (BrowserVisualInfoMatches(&info, mask, templatePtr)) {
		    if (infoList != NULL) {
			infoList[count] = info;
		    }
		    count++;
		}
	    }
	}
    }
    return count;
}

XPixmapFormatValues *
XListPixmapFormats(
    Display *display,
    int *countReturn)
{
    XPixmapFormatValues *formatList;
    int count;
    int i;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (countReturn != NULL) {
	*countReturn = 0;
    }
    count = display->nformats;
    if ((count <= 0) || (display->pixmap_format == NULL)) {
	return NULL;
    }

    formatList = (XPixmapFormatValues *)ckalloc(
	    sizeof(XPixmapFormatValues) * (size_t)count);
    for (i = 0; i < count; i++) {
	formatList[i].depth = display->pixmap_format[i].depth;
	formatList[i].bits_per_pixel = display->pixmap_format[i].bits_per_pixel;
	formatList[i].scanline_pad = display->pixmap_format[i].scanline_pad;
    }
    if (countReturn != NULL) {
	*countReturn = count;
    }
    return formatList;
}

int *
XListDepths(
    Display *display,
    int screenNumber,
    int *countReturn)
{
    Screen *screenPtr;
    int *depthList;
    int count;
    int i;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (countReturn != NULL) {
	*countReturn = 0;
    }
    if ((screenNumber < 0) || (screenNumber >= ScreenCount(display))) {
	return NULL;
    }
    screenPtr = ScreenOfDisplay(display, screenNumber);
    if ((screenPtr == NULL) || (screenPtr->ndepths <= 0)
	    || (screenPtr->depths == NULL)) {
	return NULL;
    }

    count = screenPtr->ndepths;
    depthList = (int *)ckalloc(sizeof(int) * (size_t)count);
    for (i = 0; i < count; i++) {
	depthList[i] = screenPtr->depths[i].depth;
    }
    if (countReturn != NULL) {
	*countReturn = count;
    }
    return depthList;
}

XVisualInfo *
XGetVisualInfo(
    Display *display,
    long mask,
    XVisualInfo *templatePtr,
    int *nitemsReturn)
{
    XVisualInfo *infoPtr;
    int count = 0;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (nitemsReturn != NULL) {
	*nitemsReturn = 0;
    }

    if ((mask != VisualNoMask) && (templatePtr == NULL)) {
	return NULL;
    }
    count = BrowserCollectVisualInfos(display, mask, templatePtr, NULL);
    if (count == 0) {
	return NULL;
    }
    infoPtr = (XVisualInfo *)ckalloc(sizeof(XVisualInfo) * (size_t)count);
    count = BrowserCollectVisualInfos(display, mask, templatePtr, infoPtr);

    if (nitemsReturn != NULL) {
	*nitemsReturn = count;
    }
    return infoPtr;
}

Status
XMatchVisualInfo(
    Display *display,
    int screen,
    int depth,
    int cClass,
    XVisualInfo *vinfoReturn)
{
    XVisualInfo templateInfo;
    XVisualInfo *infoPtr;
    int count = 0;

    if (vinfoReturn == NULL) {
	return False;
    }
    memset(&templateInfo, 0, sizeof(templateInfo));
    templateInfo.screen = screen;
    templateInfo.depth = depth;
    templateInfo.c_class = cClass;
    infoPtr = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask
	    | VisualClassMask, &templateInfo, &count);
    if ((infoPtr == NULL) || (count <= 0)) {
	return False;
    }
    *vinfoReturn = infoPtr[0];
    XFree(infoPtr);
    return True;
}

VisualID
XVisualIDFromVisual(
    Visual *visual)
{
    return (visual != NULL) ? visual->visualid : 0;
}

Atom
XInternAtom(
    Display *display,
    _Xconst char *name,
    Bool onlyIfExists)
{
    X11WasmAtomName *atomPtr;
    Atom predefined;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (!BrowserValidateBool(&display, onlyIfExists,
	    X11_WASM_XREQ_INTERN_ATOM)) {
	return None;
    }
    if (name == NULL) {
	return None;
    }
    predefined = BrowserPredefinedAtom(name);
    if (predefined != None) {
	return predefined;
    }
    for (atomPtr = atomList; atomPtr != NULL; atomPtr = atomPtr->nextPtr) {
	if (strcmp(atomPtr->name, name) == 0) {
	    return atomPtr->atom;
	}
    }
    if (onlyIfExists) {
	return None;
    }
    atomPtr = (X11WasmAtomName *)ckalloc(sizeof(X11WasmAtomName));
    atomPtr->atom = ++nextAtom;
    atomPtr->name = BrowserDupString(name);
    atomPtr->nextPtr = atomList;
    atomList = atomPtr;
    return atomPtr->atom;
}

Status
XInternAtoms(
    Display *display,
    char **names,
    int count,
    Bool onlyIfExists,
    Atom *atomsReturn)
{
    int i;
    Status status = True;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (!BrowserValidateBool(&display, onlyIfExists,
	    X11_WASM_XREQ_INTERN_ATOM)) {
	return False;
    }
    if (!BrowserValueOrError(&display, count, count >= 0,
	    X11_WASM_XREQ_INTERN_ATOM)) {
	return False;
    }
    if (count == 0) {
	return True;
    }
    if ((names == NULL) || (atomsReturn == NULL)) {
	return False;
    }
    for (i = 0; i < count; i++) {
	atomsReturn[i] = None;
    }
    for (i = 0; i < count; i++) {
	if (names[i] == NULL) {
	    status = False;
	    continue;
	}
	atomsReturn[i] = XInternAtom(display, names[i], onlyIfExists);
	if ((atomsReturn[i] == None) && !onlyIfExists) {
	    status = False;
	}
    }
    return status;
}

char *
XGetAtomName(
    Display *display,
    Atom atom)
{
    X11WasmAtomName *atomPtr;
    const char *predefined;

    predefined = BrowserPredefinedAtomName(atom);
    if (predefined != NULL) {
	return BrowserDupString(predefined);
    }
    for (atomPtr = atomList; atomPtr != NULL; atomPtr = atomPtr->nextPtr) {
	if (atomPtr->atom == atom) {
	    return BrowserDupString(atomPtr->name);
	}
    }
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    (void)BrowserReportXError(display, BadAtom, (XID)atom,
	    X11_WASM_XREQ_GET_ATOM_NAME, 0);
    return NULL;
}

Status
XGetAtomNames(
    Display *display,
    Atom *atoms,
    int count,
    char **namesReturn)
{
    int i;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (!BrowserValueOrError(&display, count, count >= 0,
	    X11_WASM_XREQ_GET_ATOM_NAME)) {
	return False;
    }
    if (count == 0) {
	return True;
    }
    if ((atoms == NULL) || (namesReturn == NULL)) {
	return False;
    }
    for (i = 0; i < count; i++) {
	namesReturn[i] = NULL;
    }
    for (i = 0; i < count; i++) {
	namesReturn[i] = XGetAtomName(display, atoms[i]);
	if (namesReturn[i] == NULL) {
	    int j;

	    for (j = 0; j < i; j++) {
		if (namesReturn[j] != NULL) {
		    XFree(namesReturn[j]);
		    namesReturn[j] = NULL;
		}
	    }
	    return False;
	}
    }
    return True;
}

static char *
BrowserFontXLFDForFamily(
    const char *family,
    int pixelSize)
{
    char buffer[256];

    if ((family == NULL) || (family[0] == '\0')) {
	family = "fixed";
    }
    if (pixelSize <= 0) {
	pixelSize = 14;
    }
    snprintf(buffer, sizeof(buffer),
	    "-browser-%.80s-medium-r-normal--%d-0-0-0-p-0-iso8859-1",
	    family, pixelSize);
    return BrowserDupString(buffer);
}

static XFontStruct *
BrowserAllocFontStruct(
    Display *display,
    Font fid,
    const char *fontName,
    const char *familyName,
    int pixelSize,
    int pointSize)
{
    XFontStruct *fontPtr = (XFontStruct *)ckalloc(sizeof(XFontStruct));
    int charWidth;
    int ascent;
    int descent;
    int propIndex = 0;

    BrowserFontMetricsForPixelSize(pixelSize, &charWidth, &ascent, &descent);
    memset(fontPtr, 0, sizeof(XFontStruct));
    fontPtr->fid = fid;
    fontPtr->direction = FontLeftToRight;
    fontPtr->min_char_or_byte2 = 0;
    fontPtr->max_char_or_byte2 = 255;
    fontPtr->all_chars_exist = True;
    fontPtr->default_char = '?';
    fontPtr->min_bounds.width = (short)charWidth;
    fontPtr->max_bounds.width = (short)charWidth;
    fontPtr->min_bounds.rbearing = (short)charWidth;
    fontPtr->max_bounds.rbearing = (short)charWidth;
    fontPtr->min_bounds.ascent = (short)ascent;
    fontPtr->max_bounds.ascent = (short)ascent;
    fontPtr->min_bounds.descent = (short)descent;
    fontPtr->max_bounds.descent = (short)descent;
    fontPtr->ascent = ascent;
    fontPtr->descent = descent;
    fontPtr->n_properties = 10;
    fontPtr->properties = (XFontProp *)ckalloc(sizeof(XFontProp)
	    * (size_t)fontPtr->n_properties);
    BrowserSetFontProperty(fontPtr, &propIndex, XA_FONT,
	    XInternAtom(display, fontName, False));
    BrowserSetFontProperty(fontPtr, &propIndex, XA_FONT_NAME,
	    fontPtr->properties[0].card32);
    BrowserSetFontProperty(fontPtr, &propIndex, XA_FAMILY_NAME,
	    XInternAtom(display, familyName, False));
    BrowserSetFontProperty(fontPtr, &propIndex,
	    XInternAtom(display, "PIXEL_SIZE", False), (unsigned long)pixelSize);
    BrowserSetFontProperty(fontPtr, &propIndex, XA_POINT_SIZE,
	    (unsigned long)pointSize);
    BrowserSetFontProperty(fontPtr, &propIndex, XA_RESOLUTION, 96UL);
    BrowserSetFontProperty(fontPtr, &propIndex, XA_UNDERLINE_POSITION,
	    (unsigned long)BrowserFontUnderlinePosition(fontPtr));
    BrowserSetFontProperty(fontPtr, &propIndex, XA_UNDERLINE_THICKNESS,
	    (unsigned long)BrowserFontUnderlineThickness(fontPtr));
    BrowserSetFontProperty(fontPtr, &propIndex, XA_QUAD_WIDTH,
	    (unsigned long)charWidth);
    BrowserSetFontProperty(fontPtr, &propIndex, XA_CAP_HEIGHT,
	    (unsigned long)ascent);
    fontPtr->n_properties = propIndex;
    return fontPtr;
}

Font
XLoadFont(
    Display *display,
    _Xconst char *name)
{
    X11WasmResource *resourcePtr;

    resourcePtr = X11WasmCreateResource(display, X11_WASM_RESOURCE_FONT);
    resourcePtr->fontFamily = BrowserFontFamilyFromName(name);
    resourcePtr->fontPixelSize = BrowserFontPixelSize(name);
    resourcePtr->fontPointSize = BrowserFontPointSize(name,
	    resourcePtr->fontPixelSize);
    return (Font)resourcePtr->id;
}

XFontStruct *
XLoadQueryFont(
    Display *display,
    _Xconst char *name)
{
    Font font;
    char *fontName = BrowserFontXLFD(name, 0);
    char *familyName = BrowserFontFamilyFromName(fontName);
    int pixelSize = BrowserFontPixelSize(name);
    int pointSize = BrowserFontPointSize(name, pixelSize);
    XFontStruct *fontPtr;

    font = XLoadFont(display, name);
    fontPtr = BrowserAllocFontStruct(display, font, fontName, familyName,
	    pixelSize, pointSize);
    ckfree(familyName);
    ckfree(fontName);
    return fontPtr;
}

XFontStruct *
XQueryFont(
    Display *display,
    XID fontId)
{
    int validFontable = 0;
    X11WasmResource *resourcePtr = BrowserFontableResourceOrError(&display,
	    fontId, X11_WASM_XREQ_QUERY_FONT, &validFontable);
    GC gc = (GC)(uintptr_t)fontId;
    Font fid = None;
    const char *familyName = "fixed";
    int pixelSize = 14;
    int pointSize = 0;
    char *fontName;
    XFontStruct *fontPtr;

    if (!validFontable) {
	return NULL;
    }
    if (resourcePtr != NULL) {
	fid = (Font)resourcePtr->id;
	if (resourcePtr->fontFamily != NULL) {
	    familyName = resourcePtr->fontFamily;
	}
	if (resourcePtr->fontPixelSize > 0) {
	    pixelSize = resourcePtr->fontPixelSize;
	}
	if (resourcePtr->fontPointSize > 0) {
	    pointSize = resourcePtr->fontPointSize;
	}
    } else if (BrowserIsKnownGC(gc)) {
	fid = gc->font;
    }
    fontName = BrowserFontXLFDForFamily(familyName, pixelSize);
    if (pointSize <= 0) {
	pointSize = BrowserFontPointSize(fontName, pixelSize);
    }
    fontPtr = BrowserAllocFontStruct(display, fid, fontName, familyName,
	    pixelSize, pointSize);
    ckfree(fontName);
    return fontPtr;
}

int
XFreeFont(
    Display *display,
    XFontStruct *font)
{
    if (font != NULL) {
	X11WasmDeleteResource(font->fid);
	if (font->properties != NULL) {
	    ckfree(font->properties);
	}
	if (font->per_char != NULL) {
	    ckfree(font->per_char);
	}
	ckfree(font);
    }
    (void)display;
    return Success;
}

int
XUnloadFont(
    Display *display,
    Font font)
{
    if (BrowserFontResourceOrError(&display, font,
	    X11_WASM_XREQ_CLOSE_FONT) == NULL) {
	return Success;
    }
    X11WasmDeleteResource(font);
    return Success;
}

char **
XListFonts(
    Display *display,
    _Xconst char *pattern,
    int maxnames,
    int *actualCountReturn)
{
    char **names;
    const char *familyPattern = NULL;
    size_t familyPatternLength = 0;
    int familyPatternHasWildcards = 0;
    int familyPatternIsWildcard = 1;
    int fullXLFDPattern = 0;
    int fallbackCount = 0;
    int count = 0;
    int i;

    (void)display;
    if (actualCountReturn != NULL) {
	*actualCountReturn = 0;
    }
    if (maxnames <= 0) {
	return NULL;
    }

    if ((pattern == NULL) || (pattern[0] == '\0')) {
	pattern = "*";
    }
    fullXLFDPattern = (pattern[0] == '-');
    if (!fullXLFDPattern) {
	familyPattern = pattern;
	familyPatternLength = strlen(pattern);
	familyPatternHasWildcards = BrowserFontPatternHasWildcards(
		familyPattern, familyPatternLength);
	familyPatternIsWildcard = BrowserFontPatternIsWildcard(
		familyPattern, familyPatternLength);
    }

    while (browserFontFallbackFamilies[fallbackCount] != NULL) {
	fallbackCount++;
    }
    names = (char **)ckalloc(sizeof(char *) * (size_t)(maxnames + 1));

    if (fullXLFDPattern) {
	size_t patternLength = strlen(pattern);

	for (i = 0; (i < fallbackCount) && (count < maxnames); i++) {
	    char *candidate = BrowserFontXLFD(browserFontFallbackFamilies[i], 1);

	    if (BrowserFontPatternMatches(pattern, patternLength, candidate)) {
		names[count++] = candidate;
	    } else {
		ckfree(candidate);
	    }
	}
    } else if (familyPatternIsWildcard) {
	for (i = 0; (i < fallbackCount) && (count < maxnames); i++) {
	    names[count++] = BrowserFontXLFD(
		    browserFontFallbackFamilies[i], 1);
	}
    } else {
	for (i = 0; (i < fallbackCount) && (count < maxnames); i++) {
	    if (BrowserFontPatternMatches(familyPattern, familyPatternLength,
		    browserFontFallbackFamilies[i])) {
		names[count++] = BrowserFontXLFD(
			browserFontFallbackFamilies[i], 1);
	    }
	}
	if ((count == 0) && !familyPatternHasWildcards) {
	    names[count++] = BrowserFontXLFD(pattern, 1);
	}
    }

    names[count] = NULL;
    if (actualCountReturn != NULL) {
	*actualCountReturn = count;
    }
    if (count == 0) {
	ckfree(names);
	return NULL;
    }
    return names;
}

int
XFreeFontNames(
    char **list)
{
    int i;

    if (list != NULL) {
	for (i = 0; list[i] != NULL; i++) {
	    ckfree(list[i]);
	}
	ckfree(list);
    }
    return Success;
}

Bool
XGetFontProperty(
    XFontStruct *font,
    Atom atom,
    unsigned long *valueReturn)
{
    int i;

    if ((font == NULL) || (valueReturn == NULL)) {
	return False;
    }
    for (i = 0; i < font->n_properties; i++) {
	if (font->properties[i].name == atom) {
	    *valueReturn = font->properties[i].card32;
	    return True;
	}
    }
    if (atom == XA_UNDERLINE_POSITION) {
	*valueReturn = (unsigned long)BrowserFontUnderlinePosition(font);
	return True;
    }
    if (atom == XA_UNDERLINE_THICKNESS) {
	*valueReturn = (unsigned long)BrowserFontUnderlineThickness(font);
	return True;
    }
    if ((atom == XA_FONT) || (atom == XA_FONT_NAME)) {
	*valueReturn = font->fid;
	return True;
    }
    return False;
}

int
XTextWidth(
    XFontStruct *font,
    _Xconst char *string,
    int count)
{
    char *utf8;
    int utf8Length;
    int width;

    utf8 = BrowserUtf8FromLatin1String(string, count, &utf8Length);
    if (utf8 == NULL) {
	return 0;
    }
    width = BrowserFontTextWidth(font, utf8, utf8Length);
    ckfree(utf8);
    return width;
}

int
XTextWidth16(
    XFontStruct *font,
    _Xconst XChar2b *string,
    int count)
{
    char *utf8;
    int length;
    int width;

    utf8 = BrowserUtf8FromXChar2bString(string, count, &length);
    if (utf8 == NULL) {
	return 0;
    }
    width = BrowserFontTextWidth(font, utf8, length);
    ckfree(utf8);
    return width;
}

static void
BrowserSetTextExtents(
    int width,
    int ascent,
    int descent,
    int *directionReturn,
    int *fontAscentReturn,
    int *fontDescentReturn,
    XCharStruct *overallReturn)
{
    if (directionReturn != NULL) {
	*directionReturn = FontLeftToRight;
    }
    if (fontAscentReturn != NULL) {
	*fontAscentReturn = ascent;
    }
    if (fontDescentReturn != NULL) {
	*fontDescentReturn = descent;
    }
    if (overallReturn != NULL) {
	memset(overallReturn, 0, sizeof(*overallReturn));
	overallReturn->lbearing = 0;
	overallReturn->rbearing = BrowserClampShort(width);
	overallReturn->width = BrowserClampShort(width);
	overallReturn->ascent = BrowserClampShort(ascent);
	overallReturn->descent = BrowserClampShort(descent);
    }
}

int
XTextExtents(
    XFontStruct *font,
    _Xconst char *string,
    int nchars,
    int *directionReturn,
    int *fontAscentReturn,
    int *fontDescentReturn,
    XCharStruct *overallReturn)
{
    int width = XTextWidth(font, string, nchars);
    int ascent = (font != NULL) ? font->ascent : 11;
    int descent = (font != NULL) ? font->descent : 3;

    BrowserSetTextExtents(width, ascent, descent, directionReturn,
	    fontAscentReturn, fontDescentReturn, overallReturn);
    return Success;
}

int
XTextExtents16(
    XFontStruct *font,
    _Xconst XChar2b *string,
    int nchars,
    int *directionReturn,
    int *fontAscentReturn,
    int *fontDescentReturn,
    XCharStruct *overallReturn)
{
    int width = XTextWidth16(font, string, nchars);
    int ascent = (font != NULL) ? font->ascent : 11;
    int descent = (font != NULL) ? font->descent : 3;

    BrowserSetTextExtents(width, ascent, descent, directionReturn,
	    fontAscentReturn, fontDescentReturn, overallReturn);
    return Success;
}

int
XQueryTextExtents(
    Display *display,
    XID fontId,
    _Xconst char *string,
    int nchars,
    int *directionReturn,
    int *fontAscentReturn,
    int *fontDescentReturn,
    XCharStruct *overallReturn)
{
    int validFontable = 0;
    X11WasmResource *resourcePtr = BrowserFontableResourceOrError(&display,
	    fontId, X11_WASM_XREQ_QUERY_TEXT_EXTENTS, &validFontable);
    int pixelSize = 14;
    int charWidth = 8;
    int ascent = 11;
    int descent = 3;
    int width = 0;

    if (!validFontable) {
	return Success;
    }
    if ((resourcePtr != NULL) && (resourcePtr->fontPixelSize > 0)) {
	pixelSize = resourcePtr->fontPixelSize;
    }
    BrowserFontMetricsForPixelSize(pixelSize, &charWidth, &ascent, &descent);
    {
	char *utf8;
	int utf8Length;

	utf8 = BrowserUtf8FromLatin1String(string, nchars, &utf8Length);
	if (utf8 != NULL) {
	    width = BrowserFontResourceTextWidth(resourcePtr, charWidth, utf8,
		    utf8Length);
	    ckfree(utf8);
	}
    }
    BrowserSetTextExtents(width, ascent, descent, directionReturn,
	    fontAscentReturn, fontDescentReturn, overallReturn);
    return Success;
}

int
XQueryTextExtents16(
    Display *display,
    XID fontId,
    _Xconst XChar2b *string,
    int nchars,
    int *directionReturn,
    int *fontAscentReturn,
    int *fontDescentReturn,
    XCharStruct *overallReturn)
{
    int validFontable = 0;
    X11WasmResource *resourcePtr = BrowserFontableResourceOrError(&display,
	    fontId, X11_WASM_XREQ_QUERY_TEXT_EXTENTS, &validFontable);
    int pixelSize = 14;
    int charWidth = 8;
    int ascent = 11;
    int descent = 3;
    int width = 0;

    if (!validFontable) {
	return Success;
    }
    if ((resourcePtr != NULL) && (resourcePtr->fontPixelSize > 0)) {
	pixelSize = resourcePtr->fontPixelSize;
    }
    BrowserFontMetricsForPixelSize(pixelSize, &charWidth, &ascent, &descent);
    {
	char *utf8;
	int utf8Length;

	utf8 = BrowserUtf8FromXChar2bString(string, nchars, &utf8Length);
	if (utf8 != NULL) {
	    width = BrowserFontResourceTextWidth(resourcePtr, charWidth, utf8,
		    utf8Length);
	    ckfree(utf8);
	}
    }
    BrowserSetTextExtents(width, ascent, descent, directionReturn,
	    fontAscentReturn, fontDescentReturn, overallReturn);
    return Success;
}

int
BrowserDrawUtf8Text(
    Display *display,
    Drawable drawable,
    GC gc,
    int x,
    int y,
    const char *utf8,
    int length,
    unsigned char requestCode)
{
    X11WasmResource *resourcePtr;
    int width;

    if ((utf8 == NULL) || (length <= 0)) {
	return Success;
    }
    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    requestCode);
    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&display, gc, requestCode)) {
	return Success;
    }
    width = BrowserFontResourceTextWidth(
	    (gc != NULL) ? X11WasmFindResource(gc->font) : NULL, 8,
	    utf8, length);
    BrowserApplyGCStyle(gc);
    BrowserApplyGCClip(gc);
    (void)X11WasmJsDrawText(BrowserJsXId(drawable), BrowserGCPixel(gc), x,
	    y, utf8, length);
    BrowserMarkResourceDamage(resourcePtr, x, y - 12,
	    (unsigned int)(width + 2), 16);
    return Success;
}

int
BrowserDrawUtf8ImageString(
    Display *display,
    Drawable drawable,
    GC gc,
    int x,
    int y,
    const char *utf8,
    int length,
    unsigned char requestCode)
{
    X11WasmResource *resourcePtr;
    X11WasmResource *fontPtr = NULL;
    int charWidth = 8;
    int ascent = 12;
    int descent = 4;
    int width;

    if ((utf8 == NULL) || (length <= 0)) {
	return Success;
    }
    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    requestCode);
    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&display, gc, requestCode)) {
	return Success;
    }
    if (gc != NULL) {
	fontPtr = X11WasmFindResource(gc->font);
    }
    if ((fontPtr != NULL) && (fontPtr->fontPixelSize > 0)) {
	BrowserFontMetricsForPixelSize(fontPtr->fontPixelSize, &charWidth,
		&ascent, &descent);
    }
    width = BrowserFontResourceTextWidth(fontPtr, charWidth, utf8, length);
    BrowserApplyGCStyle(gc);
    BrowserApplyGCClip(gc);
    (void)X11WasmJsDrawRectangle(BrowserJsXId(drawable),
	    (gc != NULL) ? gc->background : 0, x, y - ascent,
	    (unsigned int)width, (unsigned int)(ascent + descent), 1);
    (void)X11WasmJsDrawText(BrowserJsXId(drawable), BrowserGCPixel(gc), x, y,
	    utf8, length);
    BrowserMarkResourceDamage(resourcePtr, x, y - ascent,
	    (unsigned int)(width + 2), (unsigned int)(ascent + descent));
    return Success;
}

int
XDrawString(
    Display *d,
    Drawable dr,
    GC gc,
    int x,
    int y,
    _Xconst char *string,
    int length)
{
    char *utf8;
    int utf8Length;
    int result;

    utf8 = BrowserUtf8FromLatin1String(string, length, &utf8Length);
    if (utf8 == NULL) {
	return Success;
    }
    result = BrowserDrawUtf8Text(d, dr, gc, x, y, utf8, utf8Length,
	    X11_WASM_XREQ_POLY_TEXT_8);
    ckfree(utf8);
    return result;
}

int
XDrawImageString(
    Display *d,
    Drawable dr,
    GC gc,
    int x,
    int y,
    _Xconst char *string,
    int length)
{
    char *utf8;
    int utf8Length;
    int result;

    utf8 = BrowserUtf8FromLatin1String(string, length, &utf8Length);
    if (utf8 == NULL) {
	return Success;
    }
    result = BrowserDrawUtf8ImageString(d, dr, gc, x, y, utf8, utf8Length,
	    X11_WASM_XREQ_IMAGE_TEXT_8);
    ckfree(utf8);
    return result;
}

int
XDrawString16(
    Display *d,
    Drawable dr,
    GC gc,
    int x,
    int y,
    _Xconst XChar2b *string,
    int length)
{
    char *utf8;
    int utf8Length;
    int result;

    utf8 = BrowserUtf8FromXChar2bString(string, length, &utf8Length);
    if (utf8 == NULL) {
	return Success;
    }
    result = BrowserDrawUtf8Text(d, dr, gc, x, y, utf8, utf8Length,
	    X11_WASM_XREQ_POLY_TEXT_16);
    ckfree(utf8);
    return result;
}

static int
BrowserDrawTextItemUtf8(
    Display *display,
    Drawable drawable,
    GC gc,
    int *xPtr,
    int y,
    int delta,
    Font activeFont,
    char *utf8,
    int utf8Length,
    unsigned char requestCode)
{
    Font savedFont = None;
    int result;

    if ((gc != NULL) && (activeFont != None)) {
	savedFont = gc->font;
	gc->font = activeFont;
    }
    *xPtr += delta;
    result = BrowserDrawUtf8Text(display, drawable, gc, *xPtr, y, utf8,
	    utf8Length, requestCode);
    *xPtr += BrowserFontResourceTextWidth(
	    (gc != NULL) ? X11WasmFindResource(gc->font) : NULL, 8,
	    utf8, utf8Length);
    if ((gc != NULL) && (activeFont != None)) {
	gc->font = savedFont;
    }
    return result;
}

int
XDrawText(
    Display *display,
    Drawable drawable,
    GC gc,
    int x,
    int y,
    XTextItem *items,
    int nitems)
{
    Font activeFont = None;
    int i;

    if ((items == NULL) || (nitems <= 0)) {
	return Success;
    }
    if (BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_POLY_TEXT_8) == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&display, gc, X11_WASM_XREQ_POLY_TEXT_8)) {
	return Success;
    }
    activeFont = (gc != NULL) ? gc->font : None;
    for (i = 0; i < nitems; i++) {
	char *utf8;
	int utf8Length;
	int result;

	if (items[i].font != None) {
	    if (BrowserFontResourceOrError(&display, items[i].font,
		    X11_WASM_XREQ_POLY_TEXT_8) == NULL) {
		return Success;
	    }
	    activeFont = items[i].font;
	}
	utf8 = BrowserUtf8FromLatin1String(items[i].chars, items[i].nchars,
		&utf8Length);
	if (utf8 == NULL) {
	    x += items[i].delta;
	    continue;
	}
	result = BrowserDrawTextItemUtf8(display, drawable, gc, &x, y,
		items[i].delta, activeFont, utf8, utf8Length,
		X11_WASM_XREQ_POLY_TEXT_8);
	ckfree(utf8);
	if (result != Success) {
	    return result;
	}
    }
    return Success;
}

int
XDrawText16(
    Display *display,
    Drawable drawable,
    GC gc,
    int x,
    int y,
    XTextItem16 *items,
    int nitems)
{
    Font activeFont = None;
    int i;

    if ((items == NULL) || (nitems <= 0)) {
	return Success;
    }
    if (BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_POLY_TEXT_16) == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&display, gc, X11_WASM_XREQ_POLY_TEXT_16)) {
	return Success;
    }
    activeFont = (gc != NULL) ? gc->font : None;
    for (i = 0; i < nitems; i++) {
	char *utf8;
	int utf8Length;
	int result;

	if (items[i].font != None) {
	    if (BrowserFontResourceOrError(&display, items[i].font,
		    X11_WASM_XREQ_POLY_TEXT_16) == NULL) {
		return Success;
	    }
	    activeFont = items[i].font;
	}
	utf8 = BrowserUtf8FromXChar2bString(items[i].chars, items[i].nchars,
		&utf8Length);
	if (utf8 == NULL) {
	    x += items[i].delta;
	    continue;
	}
	result = BrowserDrawTextItemUtf8(display, drawable, gc, &x, y,
		items[i].delta, activeFont, utf8, utf8Length,
		X11_WASM_XREQ_POLY_TEXT_16);
	ckfree(utf8);
	if (result != Success) {
	    return result;
	}
    }
    return Success;
}

int
XDrawImageString16(
    Display *d,
    Drawable dr,
    GC gc,
    int x,
    int y,
    _Xconst XChar2b *string,
    int length)
{
    char *utf8;
    int utf8Length;
    int result;

    utf8 = BrowserUtf8FromXChar2bString(string, length, &utf8Length);
    if (utf8 == NULL) {
	return Success;
    }
    result = BrowserDrawUtf8ImageString(d, dr, gc, x, y, utf8, utf8Length,
	    X11_WASM_XREQ_IMAGE_TEXT_16);
    ckfree(utf8);
    return result;
}

unsigned char
BrowserCursorColorByte(
    unsigned short value)
{
    return (unsigned char)(value >> 8);
}

int
BrowserCreatePixmapCursorImage(
    X11WasmResource *cursorPtr,
    Pixmap source,
    Pixmap mask,
    XColor *foreground,
    XColor *background,
    unsigned int hotX,
    unsigned int hotY)
{
    X11WasmResource *sourcePtr = X11WasmFindResource(source);
    X11WasmResource *maskPtr;
    XImage *imagePtr;
    unsigned char *rgba;
    unsigned long pixelCount;
    int dataLength;
    unsigned int width, height;
    unsigned int x, y;
    unsigned char fgRed, fgGreen, fgBlue;
    unsigned char bgRed, bgGreen, bgBlue;

    if ((cursorPtr == NULL) || (sourcePtr == NULL)
	    || (sourcePtr->type != X11_WASM_RESOURCE_PIXMAP)
	    || (sourcePtr->depth != 1) || (sourcePtr->width == 0)
	    || (sourcePtr->height == 0)) {
	return 0;
    }
    if (mask == None) {
	mask = source;
    }
    maskPtr = X11WasmFindResource(mask);
    if ((maskPtr == NULL) || (maskPtr->type != X11_WASM_RESOURCE_PIXMAP)
	    || (maskPtr->depth != 1) || (maskPtr->width != sourcePtr->width)
	    || (maskPtr->height != sourcePtr->height)) {
	return 0;
    }

    width = sourcePtr->width;
    height = sourcePtr->height;
    if (sourcePtr->bitmapData == NULL) {
	imagePtr = XGetImage(cursorPtr->display, source, 0, 0, width, height,
		1UL, XYPixmap);
	if (imagePtr != NULL) {
	    BrowserStoreBitmapImage(sourcePtr, imagePtr, 0, 0, 0, 0,
		    width, height);
	    (void)BrowserImageDestroy(imagePtr);
	}
    }
    if ((maskPtr->bitmapData == NULL) && (maskPtr != sourcePtr)) {
	imagePtr = XGetImage(cursorPtr->display, mask, 0, 0, width, height,
		1UL, XYPixmap);
	if (imagePtr != NULL) {
	    BrowserStoreBitmapImage(maskPtr, imagePtr, 0, 0, 0, 0,
		    width, height);
	    (void)BrowserImageDestroy(imagePtr);
	}
    }
    if ((sourcePtr->bitmapData == NULL) || (maskPtr->bitmapData == NULL)) {
	return 0;
    }

    pixelCount = (unsigned long)width * (unsigned long)height;
    if (pixelCount > ((unsigned long)0x7fffffff / 4UL)) {
	return 0;
    }
    dataLength = (int)(pixelCount * 4UL);

    rgba = (unsigned char *)ckalloc((unsigned int)dataLength);
    memset(rgba, 0, (unsigned int)dataLength);
    fgRed = (foreground != NULL) ? BrowserCursorColorByte(foreground->red) : 0;
    fgGreen = (foreground != NULL) ? BrowserCursorColorByte(foreground->green) : 0;
    fgBlue = (foreground != NULL) ? BrowserCursorColorByte(foreground->blue) : 0;
    bgRed = (background != NULL) ? BrowserCursorColorByte(background->red) : 255;
    bgGreen = (background != NULL) ? BrowserCursorColorByte(background->green) : 255;
    bgBlue = (background != NULL) ? BrowserCursorColorByte(background->blue) : 255;

    for (y = 0; y < height; y++) {
	for (x = 0; x < width; x++) {
	    unsigned char *pixelPtr = rgba + ((y * width + x) * 4U);

	    if (!BrowserBitmapGetPixel(maskPtr, x, y)) {
		continue;
	    }
	    if (BrowserBitmapGetPixel(sourcePtr, x, y)) {
		pixelPtr[0] = fgRed;
		pixelPtr[1] = fgGreen;
		pixelPtr[2] = fgBlue;
	    } else {
		pixelPtr[0] = bgRed;
		pixelPtr[1] = bgGreen;
		pixelPtr[2] = bgBlue;
	    }
	    pixelPtr[3] = 255;
	}
    }

    if (hotX >= width) {
	hotX = width - 1;
    }
    if (hotY >= height) {
	hotY = height - 1;
    }
    (void)X11WasmJsCreateCursor(BrowserJsXId(cursorPtr->id), rgba,
	    dataLength, width, height, hotX, hotY);
    cursorPtr->hasCursorImage = 1;
    ckfree(rgba);
    return 1;
}

int
BrowserCursorColorIsDefaultBlack(
    const XColor *colorPtr)
{
    return (colorPtr != NULL) && (colorPtr->red == 0)
	    && (colorPtr->green == 0) && (colorPtr->blue == 0);
}

int
BrowserCursorColorIsDefaultWhite(
    const XColor *colorPtr)
{
    return (colorPtr != NULL) && (colorPtr->red == 0xffff)
	    && (colorPtr->green == 0xffff) && (colorPtr->blue == 0xffff);
}

int
BrowserCursorFontResource(
    X11WasmResource *resourcePtr)
{
    return (resourcePtr != NULL)
	    && (resourcePtr->type == X11_WASM_RESOURCE_FONT)
	    && (resourcePtr->fontFamily != NULL)
	    && (strcmp(resourcePtr->fontFamily, "cursor") == 0);
}

int
BrowserGlyphCursorUsesDefaultColors(
    XColor _Xconst *foreground,
    XColor _Xconst *background)
{
    return ((foreground == NULL) || BrowserCursorColorIsDefaultBlack(foreground))
	    && ((background == NULL)
		    || BrowserCursorColorIsDefaultWhite(background));
}

int
BrowserSyntheticCursorGlyphBit(
    unsigned int glyph,
    unsigned int x,
    unsigned int y)
{
    unsigned int centerMin = 6U;
    unsigned int centerMax = 9U;

    switch (glyph) {
    case XC_watch:
	if (((y == 2U) || (y == 13U)) && (x >= 4U) && (x <= 11U)) {
	    return 1;
	}
	if (((x == 4U) || (x == 11U)) && (y >= 2U) && (y <= 13U)) {
	    return 1;
	}
	if (((x == y) || (x + y == 15U)) && (y >= 4U) && (y <= 11U)) {
	    return 1;
	}
	return (x >= centerMin) && (x <= centerMax) && (y >= centerMin)
		&& (y <= centerMax);
    case XC_xterm:
	return (x == 7U) || (x == 8U) || ((y == 1U || y == 14U)
		&& (x >= 4U) && (x <= 11U));
    case XC_left_ptr:
	return (x <= y / 2U) || ((x >= 4U) && (x <= 6U)
		&& (y >= 8U) && (y <= 13U));
    default:
	return (x == y) || (x + y == 15U) || ((x >= centerMin)
		&& (x <= centerMax) && (y >= centerMin) && (y <= centerMax));
    }
}

int
BrowserSyntheticCursorMaskBit(
    unsigned int sourceChar,
    unsigned int maskChar,
    unsigned int x,
    unsigned int y)
{
    int dx, dy;

    if (BrowserSyntheticCursorGlyphBit(sourceChar, x, y)) {
	return 1;
    }
    if (maskChar == sourceChar) {
	return 0;
    }
    for (dy = -1; dy <= 1; dy++) {
	for (dx = -1; dx <= 1; dx++) {
	    int nx = (int)x + dx;
	    int ny = (int)y + dy;

	    if ((nx < 0) || (ny < 0) || (nx >= 16) || (ny >= 16)) {
		continue;
	    }
	    if (BrowserSyntheticCursorGlyphBit(sourceChar, (unsigned int)nx,
		    (unsigned int)ny)) {
		return 1;
	    }
	}
    }
    return 0;
}

int
BrowserXCursorGlyphBit(
    const unsigned char *data,
    unsigned int offset,
    unsigned int x,
    unsigned int y)
{
    return (data[offset + (y * 2U) + (x / 8U)]
	    & (0x80U >> (x & 7U))) != 0;
}

unsigned int
BrowserXCursorGlyphHotspot(
    const unsigned char *data,
    unsigned int offset)
{
    return ((unsigned int)data[64U + offset] << 8)
	    | (unsigned int)data[65U + offset];
}

int
BrowserCreateGlyphCursorImage(
    X11WasmResource *cursorPtr,
    unsigned int sourceChar,
    unsigned int maskChar,
    XColor _Xconst *foreground,
    XColor _Xconst *background)
{
    unsigned char *rgba;
    int dataLength = 16 * 16 * 4;
    unsigned int x, y;
    unsigned char fgRed, fgGreen, fgBlue;
    unsigned char bgRed, bgGreen, bgBlue;

    if (cursorPtr == NULL) {
	return 0;
    }
    if (BrowserGlyphCursorUsesDefaultColors(foreground, background)) {
	return 0;
    }

    rgba = (unsigned char *)ckalloc((unsigned int)dataLength);
    memset(rgba, 0, (unsigned int)dataLength);
    fgRed = (foreground != NULL) ? BrowserCursorColorByte(foreground->red) : 0;
    fgGreen = (foreground != NULL) ? BrowserCursorColorByte(foreground->green) : 0;
    fgBlue = (foreground != NULL) ? BrowserCursorColorByte(foreground->blue) : 0;
    bgRed = (background != NULL) ? BrowserCursorColorByte(background->red) : 255;
    bgGreen = (background != NULL) ? BrowserCursorColorByte(background->green) : 255;
    bgBlue = (background != NULL) ? BrowserCursorColorByte(background->blue) : 255;

    for (y = 0; y < 16U; y++) {
	for (x = 0; x < 16U; x++) {
	    unsigned char *pixelPtr = rgba + ((y * 16U + x) * 4U);

	    if (!BrowserSyntheticCursorMaskBit(sourceChar, maskChar, x, y)) {
		continue;
	    }
	    if (BrowserSyntheticCursorGlyphBit(sourceChar, x, y)) {
		pixelPtr[0] = fgRed;
		pixelPtr[1] = fgGreen;
		pixelPtr[2] = fgBlue;
	    } else {
		pixelPtr[0] = bgRed;
		pixelPtr[1] = bgGreen;
		pixelPtr[2] = bgBlue;
	    }
	    pixelPtr[3] = 255;
	}
    }

    (void)X11WasmJsCreateCursor(BrowserJsXId(cursorPtr->id), rgba,
	    dataLength, 16U, 16U, 7U, 7U);
    cursorPtr->hasCursorImage = 1;
    ckfree(rgba);
    return 1;
}

Cursor
XCreatePixmapCursor(
    Display *display,
    Pixmap source,
    Pixmap mask,
    XColor *foreground,
    XColor *background,
    unsigned int x,
    unsigned int y)
{
    X11WasmResource *resourcePtr;
    X11WasmResource *sourcePtr;
    X11WasmResource *maskPtr = NULL;

    sourcePtr = BrowserPixmapResourceOrError(&display, source,
	    X11_WASM_XREQ_CREATE_CURSOR);
    if (sourcePtr == NULL) {
	return None;
    }
    if (mask != None) {
	maskPtr = BrowserPixmapResourceOrError(&display, mask,
		X11_WASM_XREQ_CREATE_CURSOR);
	if (maskPtr == NULL) {
	    return None;
	}
    }
    if ((sourcePtr->depth != 1) || (sourcePtr->width == 0)
	    || (sourcePtr->height == 0)
	    || ((maskPtr != NULL)
		    && ((maskPtr->depth != 1)
			    || (maskPtr->width != sourcePtr->width)
			    || (maskPtr->height != sourcePtr->height)))
	    || (x >= sourcePtr->width) || (y >= sourcePtr->height)) {
	(void)BrowserReportXError(display, BadMatch, (XID)source,
		X11_WASM_XREQ_CREATE_CURSOR, 0);
	return None;
    }
    resourcePtr = X11WasmCreateResource(display, X11_WASM_RESOURCE_CURSOR);
    resourcePtr->cursorShape = XC_X_cursor;
    (void)BrowserCreatePixmapCursorImage(resourcePtr, source, mask, foreground,
	    background, x, y);
    return (Cursor)resourcePtr->id;
}

Cursor
XCreateFontCursor(
    Display *display,
    unsigned int shape)
{
    XColor foreground;
    XColor background;
    Font cursorFont;
    Cursor cursor;

    memset(&foreground, 0, sizeof(foreground));
    memset(&background, 0, sizeof(background));
    background.red = 0xffff;
    background.green = 0xffff;
    background.blue = 0xffff;
    cursorFont = XLoadFont(display, "cursor");
    if (cursorFont == None) {
	return None;
    }
    cursor = XCreateGlyphCursor(display, cursorFont, cursorFont, shape,
	    shape + 1U, &foreground, &background);
    (void)XUnloadFont(display, cursorFont);
    return cursor;
}

Cursor
XCreateGlyphCursor(
    Display *display,
    Font sourceFont,
    Font maskFont,
    unsigned int sourceChar,
    unsigned int maskChar,
    XColor _Xconst *foreground,
    XColor _Xconst *background)
{
    X11WasmResource *resourcePtr;
    X11WasmResource *sourcePtr;
    X11WasmResource *maskPtr = NULL;

    sourcePtr = BrowserFontResourceOrError(&display, sourceFont,
	    X11_WASM_XREQ_CREATE_GLYPH_CURSOR);
    if (sourcePtr == NULL) {
	return None;
    }
    if (sourceChar >= XC_num_glyphs) {
	(void)BrowserReportXError(display, BadValue, (XID)sourceChar,
		X11_WASM_XREQ_CREATE_GLYPH_CURSOR, 0);
	return None;
    }
    if (maskFont != None) {
	maskPtr = BrowserFontResourceOrError(&display, maskFont,
		X11_WASM_XREQ_CREATE_GLYPH_CURSOR);
	if (maskPtr == NULL) {
	    return None;
	}
	if (maskChar >= XC_num_glyphs) {
	    (void)BrowserReportXError(display, BadValue, (XID)maskChar,
		    X11_WASM_XREQ_CREATE_GLYPH_CURSOR, 0);
	    return None;
	}
    }
    resourcePtr = X11WasmCreateResource(display, X11_WASM_RESOURCE_CURSOR);
    resourcePtr->cursorShape = sourceChar;
    if ((maskFont == None) || (maskFont == sourceFont)
	    || (BrowserCursorFontResource(sourcePtr)
		    && BrowserCursorFontResource(maskPtr))) {
	(void)BrowserCreateGlyphCursorImage(resourcePtr, sourceChar,
		(maskFont == None) ? sourceChar : maskChar, foreground,
		background);
    }
    return (Cursor)resourcePtr->id;
}

int XDefineCursor(Display *display, Window window, Cursor cursor)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);

    if (resourcePtr == NULL) {
	return Success;
    }
    if ((cursor != None) && (BrowserCursorResourceOrError(&display, cursor,
	    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES) == NULL)) {
	return Success;
    }
    BrowserSetWindowCursor(resourcePtr, cursor);
    BrowserSyncWindowStyle(resourcePtr);
    return Success;
}

int
XUndefineCursor(
    Display *display,
    Window window)
{
    return XDefineCursor(display, window, None);
}

int XFreeCursor(Display *display, Cursor cursor)
{
    X11WasmResource *resourcePtr;

    resourcePtr = BrowserCursorResourceOrError(&display, cursor,
	    X11_WASM_XREQ_FREE_CURSOR);
    if (resourcePtr == NULL) {
	return Success;
    }
    resourcePtr->clientReleased = 1;
    if (resourcePtr->referenceCount > 0) {
	return Success;
    }
    (void)X11WasmJsDestroyResource(BrowserJsXId(cursor));
    X11WasmDeleteResource(cursor);
    return Success;
}

int
BrowserIsAlgorithmicUnicodeCodepoint(
    unsigned int codepoint)
{
    if (codepoint > 0x10ffffU) {
	return 0;
    }
    if ((codepoint >= 0xd800U) && (codepoint <= 0xdfffU)) {
	return 0;
    }
    return (((codepoint >= 0x20U) && (codepoint <= 0x7eU))
	    || (codepoint >= 0xa0U));
}

int
BrowserParseKeySymHex(
    const char *string,
    size_t minDigits,
    size_t maxDigits,
    unsigned long *valuePtr)
{
    size_t digits = 0;
    unsigned long value = 0;

    while ((digits < maxDigits) && (BrowserHexDigit(string[digits]) >= 0)) {
	int digit = BrowserHexDigit(string[digits]);

	if (value > ((ULONG_MAX - (unsigned long)digit) / 16UL)) {
	    return 0;
	}
	value = (value << 4) | (unsigned long)digit;
	digits++;
    }
    if ((digits < minDigits) || (string[digits] != '\0')) {
	return 0;
    }
    *valuePtr = value;
    return 1;
}

KeySym
XStringToKeysym(
    _Xconst char *name)
{
    unsigned long value;
    size_t i;

    if ((name == NULL) || (name[0] == '\0')) {
	return NoSymbol;
    }
    if (name[1] == '\0') {
	return (KeySym)(unsigned char)name[0];
    }
    if ((name[0] == 'F') && (name[1] >= '1') && (name[1] <= '9')) {
	int number = 0;
	const char *p;

	for (p = name + 1; *p != '\0'; p++) {
	    if ((*p < '0') || (*p > '9')) {
		number = 0;
		break;
	    }
	    number = number * 10 + (*p - '0');
	}
	if ((number >= 1) && (number <= 35)) {
	    return XK_F1 + (KeySym)(number - 1);
	}
    }
    if ((strncmp(name, "KP_", 3) == 0) && (name[3] >= '0')
	    && (name[3] <= '9') && (name[4] == '\0')) {
	return XK_KP_0 + (KeySym)(name[3] - '0');
    }
    if ((name[0] == 'U')
	    && BrowserParseKeySymHex(name + 1, 4, 8, &value)
	    && BrowserIsAlgorithmicUnicodeCodepoint((unsigned int)value)) {
	return BrowserKeySymFromCodepoint((unsigned int)value);
    }
    if ((name[0] == '0') && ((name[1] == 'x') || (name[1] == 'X'))
	    && BrowserParseKeySymHex(name + 2, 1, 8, &value)) {
	return (KeySym)value;
    }
    for (i = 0; i < browserKeySymNameCount;
	    i++) {
	if (strcmp(name, browserKeySymNames[i].name) == 0) {
	    return browserKeySymNames[i].keySym;
	}
    }
    return NoSymbol;
}

char *
XKeysymToString(
    KeySym keySym)
{
    static char buffer[16];
    unsigned int codepoint;
    size_t i;

    if ((keySym >= XK_F1) && (keySym <= XK_F35)) {
	snprintf(buffer, sizeof(buffer), "F%lu",
		(unsigned long)(keySym - XK_F1 + 1));
	return buffer;
    }
    if ((keySym >= XK_KP_0) && (keySym <= XK_KP_9)) {
	snprintf(buffer, sizeof(buffer), "KP_%lu",
		(unsigned long)(keySym - XK_KP_0));
	return buffer;
    }
    for (i = 0; i < browserKeySymNameCount;
	    i++) {
	if (keySym == browserKeySymNames[i].keySym) {
	    return (char *)browserKeySymNames[i].name;
	}
    }
    if ((keySym >= 0x21) && (keySym <= 0x7e)) {
	buffer[0] = (char)keySym;
	buffer[1] = '\0';
	return buffer;
    }
    if (BrowserIsUnicodeKeySym(keySym)) {
	codepoint = ((unsigned int)keySym & 0x00ffffffU);
	if (BrowserIsAlgorithmicUnicodeCodepoint(codepoint)) {
	    if (codepoint <= 0xffffU) {
		snprintf(buffer, sizeof(buffer), "U%04X", codepoint);
	    } else {
		snprintf(buffer, sizeof(buffer), "U%08X", codepoint);
	    }
	    return buffer;
	}
    } else if (BrowserIsAlgorithmicUnicodeCodepoint((unsigned int)keySym)
	    && ((unsigned int)keySym >= 0xa0U)
	    && ((unsigned int)keySym <= 0xffU)) {
	snprintf(buffer, sizeof(buffer), "U%04X", (unsigned int)keySym);
	return buffer;
    }
    return NULL;
}

KeyCode XKeysymToKeycode(Display *display, KeySym keySym)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    (void)display;
    return (KeyCode)BrowserKeycodeFromKeySym(keySym);
}

KeySym XKeycodeToKeysym(Display *display, unsigned int keycode, int index)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    (void)display;
    return BrowserKeySymForKeycodeLevel(keycode, index);
}
KeySym XkbKeycodeToKeysym(Display *display, unsigned int keycode, int group, int level)
{
    if (group != 0) {
	return NoSymbol;
    }
    return XKeycodeToKeysym(display, keycode, level);
}
KeySym XLookupKeysym(XKeyEvent *event, int index)
{
    BrowserInitDisplay();
    return (event != NULL) ? BrowserKeySymForKeycodeLevel(event->keycode,
	    index) : NoSymbol;
}

int
XChangeKeyboardMapping(
    Display *display,
    int firstKeycode,
    int keysymsPerKeycode,
    KeySym *keysyms,
    int numCodes)
{
    int i;
    size_t total;
    int changed = 0;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if ((keysymsPerKeycode <= 0) || (numCodes <= 0)
	    || (firstKeycode < display->min_keycode)
	    || (numCodes > display->max_keycode - firstKeycode + 1)
	    || (keysyms == NULL)) {
	(void)BrowserReportXError(display, BadValue, (XID)firstKeycode,
		X11_WASM_XREQ_CHANGE_KEYBOARD_MAPPING, 0);
	return Success;
    }
    total = (size_t)keysymsPerKeycode * (size_t)numCodes;
    if ((keysymsPerKeycode > INT_MAX / numCodes)
	    || (total > (size_t)INT_MAX / sizeof(KeySym))) {
	(void)BrowserReportXError(display, BadAlloc, (XID)firstKeycode,
		X11_WASM_XREQ_CHANGE_KEYBOARD_MAPPING, 0);
	return Success;
    }
    for (i = 0; i < numCodes; i++) {
	changed += BrowserSetKeycodeMapping((unsigned int)firstKeycode
		+ (unsigned int)i, keysyms + i * keysymsPerKeycode,
		keysymsPerKeycode);
    }
    if (changed > 0) {
	browserDisplay.keyboardMapSerial++;
    }
    (void)BrowserQueueMappingNotify(MappingKeyboard, firstKeycode, numCodes);
    return XNoOp(display);
}

KeySym *
XGetKeyboardMapping(
    Display *display,
    KeyCode firstKeycode,
    int keycodeCount,
    int *keysymsPerKeycodeReturn)
{
    KeySym *mapping;
    int keysymsPerKeycode = X11_WASM_KEYSYMS_PER_KEYCODE;
    int i;
    size_t total;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (keysymsPerKeycodeReturn != NULL) {
	*keysymsPerKeycodeReturn = 0;
    }
    if ((keycodeCount <= 0)
	    || (firstKeycode < display->min_keycode)
	    || (keycodeCount
		    > display->max_keycode - (int)firstKeycode + 1)) {
	(void)BrowserReportXError(display, BadValue, (XID)firstKeycode,
		X11_WASM_XREQ_GET_KEYBOARD_MAPPING, 0);
	return NULL;
    }
    total = (size_t)keycodeCount * (size_t)keysymsPerKeycode;
    if (total > ((size_t)INT_MAX / sizeof(KeySym))) {
	(void)BrowserReportXError(display, BadAlloc, (XID)firstKeycode,
		X11_WASM_XREQ_GET_KEYBOARD_MAPPING, 0);
	return NULL;
    }
    mapping = (KeySym *)ckalloc(total * sizeof(KeySym));
    for (i = 0; i < keycodeCount; i++) {
	unsigned int keycode = (unsigned int)firstKeycode + (unsigned int)i;

	mapping[(i * keysymsPerKeycode) + 0] =
		BrowserKeySymForKeycodeLevel(keycode, 0);
	mapping[(i * keysymsPerKeycode) + 1] =
		BrowserKeySymForKeycodeLevel(keycode, 1);
    }
    if (keysymsPerKeycodeReturn != NULL) {
	*keysymsPerKeycodeReturn = keysymsPerKeycode;
    }
    return mapping;
}

Status
BrowserLookupStatus(
    int length,
    KeySym keySym)
{
    int hasChars = (length > 0);
    int hasKeySym = (keySym != NoSymbol);

    if (hasChars && hasKeySym) {
	return XLookupBoth;
    }
    if (hasChars) {
	return XLookupChars;
    }
    if (hasKeySym) {
	return XLookupKeySym;
    }
    return XLookupNone;
}

int
BrowserLookupUtf8String(
    XKeyPressedEvent *event,
    char *buffer,
    int bytes,
    KeySym *keySymReturn,
    Status *statusReturn)
{
    int index = ((event != NULL) && (event->state & ShiftMask)) ? 1 : 0;
    KeySym sym = XLookupKeysym(event, index);
    unsigned int codepoint = 0;
    unsigned char latin1;
    int required = 0;
    int length;

    if (keySymReturn != NULL) {
	*keySymReturn = sym;
    }
    if (BrowserIsUnicodeKeySym(sym)) {
	codepoint = ((unsigned int)sym & 0x00ffffffU);
	required = BrowserUtf8CodepointLength(codepoint);
    } else if (BrowserLatin1FromKeySym(sym, &latin1)) {
	codepoint = (unsigned int)latin1;
	required = BrowserUtf8CodepointLength(codepoint);
    }
    if (required <= 0) {
	if (statusReturn != NULL) {
	    *statusReturn = BrowserLookupStatus(0, sym);
	}
	return 0;
    }
    if ((buffer == NULL) || (bytes < required)) {
	if (statusReturn != NULL) {
	    *statusReturn = XBufferOverflow;
	}
	return required;
    }
    length = BrowserUtf8FromCodepoint(codepoint, buffer, bytes);
    if (statusReturn != NULL) {
	*statusReturn = BrowserLookupStatus(length, sym);
    }
    return length;
}

int
BrowserIsSyntheticKeyEvent(
    XKeyPressedEvent *event)
{
    if ((event == NULL)
	    || ((event->type != KeyPress) && (event->type != KeyRelease))) {
	return 0;
    }
    if ((event->serial == 0) || (event->root != None)) {
	return 0;
    }
    return (event->time == CurrentTime) || (event->x_root == -1)
	    || (event->y_root == -1);
}

int
BrowserICAllowsLookup(
    XIC inputContext,
    XKeyPressedEvent *event)
{
    XEvent eventCopy;
    Window focusWindow;
    long eventMask;

    if (inputContext == NULL) {
	return 1;
    }
    if ((event != NULL)
	    && (event->send_event || BrowserIsSyntheticKeyEvent(event))) {
	return 1;
    }
    if (event == NULL) {
	return 0;
    }
    memset(&eventCopy, 0, sizeof(eventCopy));
    eventCopy.xkey = *event;
    eventMask = BrowserEventMask(&eventCopy);
    if ((eventMask == NoEventMask)
	    || ((inputContext->filterEvents & eventMask) == 0)) {
	return 0;
    }
    focusWindow = (inputContext->focusWindow != None)
	    ? inputContext->focusWindow : inputContext->clientWindow;
    if ((event->window == None) || (focusWindow == None)) {
	return 0;
    }
    if (inputContext->focused && (focusedInputContext == inputContext)
	    && (event->window == focusWindow)) {
	return 1;
    }
    if ((event->type == KeyPress) || (event->type == KeyRelease)) {
	Window grabWindow = browserDisplay.keyboardGrabWindow;

	if (!BrowserWindowCanReceiveEvents(grabWindow)) {
	    grabWindow = BrowserGetGrabWindow();
	}
	if (BrowserWindowCanReceiveEvents(grabWindow)
		&& (event->window == grabWindow)) {
	    return (event->window == focusWindow)
		    || (inputContext->focused
			    && (focusedInputContext == inputContext));
	}
    }
    return 0;
}

int
BrowserIMSupportsStyle(
    XIM inputMethod,
    XIMStyle style)
{
    unsigned short i;

    if (inputMethod == NULL) {
	return 0;
    }
    for (i = 0; i < inputMethod->styleCount; i++) {
	if (inputMethod->styles[i] == style) {
	    return 1;
	}
    }
    return 0;
}

Window
BrowserInputContextWindow(
    XIC inputContext)
{
    if (inputContext == NULL) {
	return None;
    }
    return (inputContext->focusWindow != None)
	    ? inputContext->focusWindow : inputContext->clientWindow;
}

void
BrowserSyncInputContext(
    XIC inputContext)
{
    int hasSpot = 0;
    int spotRootX = 0;
    int spotRootY = 0;
    Window window;

    if ((inputContext == NULL) || !inputContext->focused
	    || (focusedInputContext != inputContext)) {
	return;
    }
    window = BrowserInputContextWindow(inputContext);
    if ((window == None) || (BrowserWindowResource(window) == NULL)) {
	(void)X11WasmJsSetInputContext(1, 0, 0, 0, 0);
	return;
    }
    hasSpot = inputContext->preedit.hasSpotLocation
	    && ((inputContext->inputStyle & XIMPreeditPosition) != 0);
    if (hasSpot) {
	BrowserWindowRootOffset(window, &spotRootX, &spotRootY);
	spotRootX += inputContext->preedit.spotLocation.x;
	spotRootY += inputContext->preedit.spotLocation.y;
    }
    (void)X11WasmJsSetInputContext(1, BrowserJsXId(window), hasSpot,
	    spotRootX, spotRootY);
}

void
BrowserClearInputContext(
    void)
{
    (void)X11WasmJsSetInputContext(0, 0, 0, 0, 0);
}
