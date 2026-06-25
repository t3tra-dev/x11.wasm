#include "XlibInt.h"

char *
BrowserDupString(
    const char *string)
{
    size_t length;
    char *copy;

    if (string == NULL) {
	string = "";
    }
    length = strlen(string);
    copy = (char *)ckalloc(length + 1);
    memcpy(copy, string, length + 1);
    return copy;
}

char *
BrowserDupCountedString(
    const unsigned char *string,
    int maxLength)
{
    int length = 0;
    char *copy;

    if ((string == NULL) || (maxLength <= 0)) {
	return BrowserDupString("");
    }
    while ((length < maxLength) && (string[length] != '\0')) {
	length++;
    }
    copy = (char *)ckalloc((size_t)length + 1U);
    if (length > 0) {
	memcpy(copy, string, (size_t)length);
    }
    copy[length] = '\0';
    return copy;
}

void
BrowserCopyString(
    char *buffer,
    int length,
    const char *string)
{
    size_t sourceLength;
    size_t copyLength;

    if ((buffer == NULL) || (length <= 0)) {
	return;
    }
    if (string == NULL) {
	string = "";
    }
    sourceLength = strlen(string);
    copyLength = (sourceLength < (size_t)(length - 1))
	    ? sourceLength : (size_t)(length - 1);
    if (copyLength > 0) {
	memcpy(buffer, string, copyLength);
    }
    buffer[copyLength] = '\0';
}

const X11WasmExtensionInfo *
BrowserFindExtension(
    const char *name)
{
    size_t i;

    if (name == NULL) {
	return NULL;
    }
    for (i = 0; i < browserExtensionCount;
	    i++) {
	if (strcmp(name, browserExtensions[i].name) == 0) {
	    return &browserExtensions[i];
	}
    }
    return NULL;
}

const char *
BrowserErrorText(
    int code)
{
    switch (code) {
    case Success: return "Success";
    case BadRequest: return "BadRequest";
    case BadValue: return "BadValue";
    case BadWindow: return "BadWindow";
    case BadPixmap: return "BadPixmap";
    case BadAtom: return "BadAtom";
    case BadCursor: return "BadCursor";
    case BadFont: return "BadFont";
    case BadMatch: return "BadMatch";
    case BadDrawable: return "BadDrawable";
    case BadAccess: return "BadAccess";
    case BadAlloc: return "BadAlloc";
    case BadColor: return "BadColor";
    case BadGC: return "BadGC";
    case BadIDChoice: return "BadIDChoice";
    case BadName: return "BadName";
    case BadLength: return "BadLength";
    case BadImplementation: return "BadImplementation";
    default:
	if ((code >= FirstExtensionError) && (code <= LastExtensionError)) {
	    return "Extension error";
	}
	return "Unknown error";
    }
}

int
BrowserErrorCodeForName(
    const char *name,
    int *codePtr)
{
    char *endPtr;
    long code;

    if ((name == NULL) || (codePtr == NULL)) {
	return 0;
    }
    if (strcmp(name, "Success") == 0) {
	*codePtr = Success;
	return 1;
    }
    if (strcmp(name, "BadRequest") == 0) {
	*codePtr = BadRequest;
	return 1;
    }
    if (strcmp(name, "BadValue") == 0) {
	*codePtr = BadValue;
	return 1;
    }
    if (strcmp(name, "BadWindow") == 0) {
	*codePtr = BadWindow;
	return 1;
    }
    if (strcmp(name, "BadPixmap") == 0) {
	*codePtr = BadPixmap;
	return 1;
    }
    if (strcmp(name, "BadAtom") == 0) {
	*codePtr = BadAtom;
	return 1;
    }
    if (strcmp(name, "BadCursor") == 0) {
	*codePtr = BadCursor;
	return 1;
    }
    if (strcmp(name, "BadFont") == 0) {
	*codePtr = BadFont;
	return 1;
    }
    if (strcmp(name, "BadMatch") == 0) {
	*codePtr = BadMatch;
	return 1;
    }
    if (strcmp(name, "BadDrawable") == 0) {
	*codePtr = BadDrawable;
	return 1;
    }
    if (strcmp(name, "BadAccess") == 0) {
	*codePtr = BadAccess;
	return 1;
    }
    if (strcmp(name, "BadAlloc") == 0) {
	*codePtr = BadAlloc;
	return 1;
    }
    if (strcmp(name, "BadColor") == 0) {
	*codePtr = BadColor;
	return 1;
    }
    if (strcmp(name, "BadGC") == 0) {
	*codePtr = BadGC;
	return 1;
    }
    if (strcmp(name, "BadIDChoice") == 0) {
	*codePtr = BadIDChoice;
	return 1;
    }
    if (strcmp(name, "BadName") == 0) {
	*codePtr = BadName;
	return 1;
    }
    if (strcmp(name, "BadLength") == 0) {
	*codePtr = BadLength;
	return 1;
    }
    if (strcmp(name, "BadImplementation") == 0) {
	*codePtr = BadImplementation;
	return 1;
    }
    if (!isdigit(UCHAR(name[0]))) {
	return 0;
    }
    code = strtol(name, &endPtr, 10);
    if ((endPtr == name) || (*endPtr != '\0') || (code < 0)
	    || (code > UCHAR_MAX)) {
	return 0;
    }
    *codePtr = (int)code;
    return 1;
}

int
BrowserDefaultErrorHandler(
    Display *display,
    XErrorEvent *eventPtr)
{
    (void)display;
    (void)eventPtr;
    return 0;
}

int
BrowserDefaultIOErrorHandler(
    Display *display)
{
    (void)display;
    return 0;
}

int
BrowserReportXError(
    Display *display,
    int errorCode,
    XID resourceId,
    unsigned char requestCode,
    unsigned char minorCode)
{
    XErrorEvent event;
    XErrorHandler handler;

    BrowserInitDisplay();
    if (display == NULL) {
	display = &browserDisplay.display;
    }
    handler = (errorHandler != NULL) ? errorHandler : BrowserDefaultErrorHandler;
    memset(&event, 0, sizeof(event));
    event.display = display;
    event.resourceid = resourceId;
    event.serial = ++LastKnownRequestProcessed(display);
    BrowserSyncRequestState(display);
    event.error_code = (unsigned char)errorCode;
    event.request_code = requestCode;
    event.minor_code = minorCode;
    return handler(display, &event);
}

int
BrowserTestXErrorHandler(
    Display *display,
    XErrorEvent *eventPtr)
{
    (void)display;
    browserTestXErrorCount++;
    browserTestLastErrorCode = eventPtr->error_code;
    browserTestLastRequestCode = eventPtr->request_code;
    browserTestLastResourceId = eventPtr->resourceid;
    browserTestLastSerial = eventPtr->serial;
    browserTestRequestCodeSum += eventPtr->request_code;
    if (eventPtr->request_code < 64) {
	browserTestRequestCodeMask |= 1ULL << eventPtr->request_code;
    }
    return 0;
}

int
BrowserTestXIOErrorHandler(
    Display *display)
{
    (void)display;
    browserTestXIOErrorCount++;
    return 0;
}

void
BrowserTestIMInstantiateCallback(
    Display *display,
    XPointer clientData,
    XPointer callData)
{
    browserTestIMInstantiateCount++;
    browserTestIMInstantiateDisplay = display;
    browserTestIMInstantiateClientData = clientData;
    browserTestIMInstantiateCallData = callData;
    (void)XUnregisterIMInstantiateCallback(display, NULL, NULL, NULL,
	    BrowserTestIMInstantiateCallback, clientData);
}

void
BrowserTestIMDestroyCallback(
    XIM inputMethod,
    XPointer clientData,
    XPointer callData)
{
    browserTestIMDestroyCount++;
    browserTestIMDestroyMethod = inputMethod;
    browserTestIMDestroyClientData = clientData;
    browserTestIMDestroyCallData = callData;
}

int
BrowserTestTextPropertyEquals(
    const XTextProperty *textProp,
    const char *value)
{
    size_t length = (value != NULL) ? strlen(value) : 0U;

    return (textProp != NULL)
	    && BrowserPropertyIsTextEncoding(textProp->encoding)
	    && (textProp->format == 8)
	    && (textProp->nitems == (unsigned long)length)
	    && (textProp->value != NULL)
	    && (memcmp(textProp->value, value, length) == 0)
	    && (textProp->value[length] == '\0');
}

int
BrowserTestAtomListContains(
    const Atom *atoms,
    int count,
    Atom atom)
{
    int i;

    for (i = 0; i < count; i++) {
	if (atoms[i] == atom) {
	    return 1;
	}
    }
    return 0;
}

X11WasmResource *
BrowserWindowResourceOrError(
    Display **displayPtr,
    Window window,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;
    X11WasmResource *resourcePtr;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    resourcePtr = X11WasmFindResource(window);
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)) {
	(void)BrowserReportXError(display, BadWindow, (XID)window,
		requestCode, 0);
	return NULL;
    }
    return resourcePtr;
}

X11WasmResource *
BrowserDrawableResourceOrError(
    Display **displayPtr,
    Drawable drawable,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;
    X11WasmResource *resourcePtr;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    resourcePtr = X11WasmFindResource(drawable);
    if ((resourcePtr == NULL)
	    || ((resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
		    && (resourcePtr->type != X11_WASM_RESOURCE_PIXMAP))
	    || ((resourcePtr->type == X11_WASM_RESOURCE_WINDOW)
		    && (resourcePtr->c_class == InputOnly))) {
	(void)BrowserReportXError(display, BadDrawable, (XID)drawable,
		requestCode, 0);
	return NULL;
    }
    return resourcePtr;
}

int
BrowserDrawableDepthsMatchOrError(
    Display **displayPtr,
    X11WasmResource *sourcePtr,
    X11WasmResource *targetPtr,
    Drawable target,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

    if ((sourcePtr == NULL) || (targetPtr == NULL)
	    || (sourcePtr->depth == targetPtr->depth)) {
	return 1;
    }
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    (void)BrowserReportXError(display, BadMatch, (XID)target, requestCode, 0);
    return 0;
}

X11WasmResource *
BrowserPixmapResourceOrError(
    Display **displayPtr,
    Pixmap pixmap,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;
    X11WasmResource *resourcePtr;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    resourcePtr = X11WasmFindResource(pixmap);
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_PIXMAP)) {
	(void)BrowserReportXError(display, BadPixmap, (XID)pixmap,
		requestCode, 0);
	return NULL;
    }
    return resourcePtr;
}

X11WasmResource *
BrowserFontResourceOrError(
    Display **displayPtr,
    Font font,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;
    X11WasmResource *resourcePtr;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    resourcePtr = X11WasmFindResource(font);
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_FONT)) {
	(void)BrowserReportXError(display, BadFont, (XID)font, requestCode, 0);
	return NULL;
    }
    return resourcePtr;
}

X11WasmResource *
BrowserCursorResourceOrError(
    Display **displayPtr,
    Cursor cursor,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;
    X11WasmResource *resourcePtr;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    resourcePtr = X11WasmFindResource((XID)cursor);
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_CURSOR)) {
	(void)BrowserReportXError(display, BadCursor, (XID)cursor,
		requestCode, 0);
	return NULL;
    }
    return resourcePtr;
}

int
BrowserColormapOrError(
    Display **displayPtr,
    Colormap colormap,
    unsigned char requestCode,
    int allowNone)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;
    X11WasmResource *resourcePtr;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    if (((colormap == None) && allowNone)
	    || (colormap == browserDisplay.defaultColormap)) {
	return 1;
    }
    resourcePtr = X11WasmFindResource((XID)colormap);
    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_COLORMAP)) {
	(void)BrowserReportXError(display, BadColor, (XID)colormap,
		requestCode, 0);
	return 0;
    }
    return 1;
}

X11WasmTrackedGC *
BrowserTrackedGC(
    GC gc)
{
    X11WasmTrackedGC *gcPtr;

    if (gc == NULL) {
	return NULL;
    }
    for (gcPtr = gcList; gcPtr != NULL; gcPtr = gcPtr->nextPtr) {
	if (gcPtr->gc == gc) {
	    return gcPtr;
	}
    }
    return NULL;
}

unsigned int
BrowserGCDepth(
    GC gc)
{
    X11WasmTrackedGC *gcPtr;

    BrowserInitDisplay();
    if (gc == &browserDisplay.defaultGC) {
	return (unsigned int)DefaultDepth(&browserDisplay.display,
		DefaultScreen(&browserDisplay.display));
    }
    gcPtr = BrowserTrackedGC(gc);
    return (gcPtr != NULL) ? gcPtr->depth : 0U;
}

void
BrowserRegisterGC(
    GC gc,
    unsigned int depth)
{
    X11WasmTrackedGC *gcPtr;

    if ((gc == NULL) || BrowserIsKnownGC(gc)) {
	return;
    }
    gcPtr = (X11WasmTrackedGC *)ckalloc(sizeof(X11WasmTrackedGC));
    gcPtr->gc = gc;
    gcPtr->depth = depth;
    gcPtr->nextPtr = gcList;
    gcList = gcPtr;
}

int
BrowserUnregisterGC(
    GC gc)
{
    X11WasmTrackedGC **linkPtr = &gcList;

    while (*linkPtr != NULL) {
	X11WasmTrackedGC *gcPtr = *linkPtr;

	if (gcPtr->gc == gc) {
	    *linkPtr = gcPtr->nextPtr;
	    ckfree(gcPtr);
	    return 1;
	}
	linkPtr = &gcPtr->nextPtr;
    }
    return 0;
}

int
BrowserIsKnownGC(
    GC gc)
{
    if (gc == NULL) {
	return 0;
    }
    BrowserInitDisplay();
    if (gc == &browserDisplay.defaultGC) {
	return 1;
    }
    return BrowserTrackedGC(gc) != NULL;
}

GC
BrowserGCOrError(
    Display **displayPtr,
    GC gc,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    if (!BrowserIsKnownGC(gc)) {
	(void)BrowserReportXError(display, BadGC, (XID)(uintptr_t)gc,
		requestCode, 0);
	return NULL;
    }
    return gc;
}

int
BrowserOptionalGCOrError(
    Display **displayPtr,
    GC gc,
    unsigned char requestCode)
{
    if (gc == NULL) {
	return 1;
    }
    return BrowserGCOrError(displayPtr, gc, requestCode) != NULL;
}

X11WasmResource *
BrowserFontableResourceOrError(
    Display **displayPtr,
    XID fontable,
    unsigned char requestCode,
    int *validPtr)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;
    X11WasmResource *resourcePtr;
    GC gc = (GC)(uintptr_t)fontable;

    if (validPtr != NULL) {
	*validPtr = 0;
    }
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    resourcePtr = X11WasmFindResource(fontable);
    if ((resourcePtr != NULL)
	    && (resourcePtr->type == X11_WASM_RESOURCE_FONT)) {
	if (validPtr != NULL) {
	    *validPtr = 1;
	}
	return resourcePtr;
    }
    if (BrowserIsKnownGC(gc)) {
	resourcePtr = (gc->font != None) ? X11WasmFindResource(gc->font)
		: NULL;
	if ((resourcePtr != NULL)
	    && (resourcePtr->type != X11_WASM_RESOURCE_FONT)) {
	resourcePtr = NULL;
    }
	if ((resourcePtr == NULL) && (gc->font != None)) {
	    (void)BrowserReportXError(display, BadFont, (XID)gc->font,
		    requestCode, 0);
	    return NULL;
	}
	if (validPtr != NULL) {
	    *validPtr = 1;
	}
	return resourcePtr;
    }
    (void)BrowserReportXError(display, BadFont, fontable, requestCode, 0);
    return NULL;
}

int
BrowserValidateGCPixmapDepth(
    Display **displayPtr,
    Pixmap pixmap,
    unsigned int expectedDepth,
    unsigned char requestCode,
    int allowNone)
{
    X11WasmResource *pixmapPtr;
    Display *display;

    if ((pixmap == None) && allowNone) {
	return 1;
    }
    pixmapPtr = BrowserPixmapResourceOrError(displayPtr, pixmap, requestCode);
    if (pixmapPtr == NULL) {
	return 0;
    }
    if ((expectedDepth != 0U) && (pixmapPtr->depth != expectedDepth)) {
	display = (displayPtr != NULL) ? *displayPtr : NULL;
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
BrowserValidateGCValueReferences(
    Display **displayPtr,
    unsigned long mask,
    XGCValues *values,
    unsigned int gcDepth,
    unsigned char requestCode)
{
    if (values == NULL) {
	return 1;
    }
    if ((mask & GCFont)
	    && (BrowserFontResourceOrError(displayPtr, values->font,
		    requestCode) == NULL)) {
	return 0;
    }
    if ((mask & GCTile)
	    && !BrowserValidateGCPixmapDepth(displayPtr, values->tile,
		    gcDepth, requestCode, 0)) {
	return 0;
    }
    if ((mask & GCStipple)
	    && !BrowserValidateGCPixmapDepth(displayPtr, values->stipple,
		    1U, requestCode, 0)) {
	return 0;
    }
    if ((mask & GCClipMask)
	    && !BrowserValidateGCPixmapDepth(displayPtr, values->clip_mask,
		    1U, requestCode, 1)) {
	return 0;
    }
    return 1;
}

int
BrowserValueOrError(
    Display **displayPtr,
    long value,
    int valid,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

    if (valid) {
	return 1;
    }
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    (void)BrowserReportXError(display, BadValue, (XID)value, requestCode, 0);
    return 0;
}

int
BrowserBadMatchOrError(
    Display **displayPtr,
    XID resource,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    (void)BrowserReportXError(display, BadMatch, resource, requestCode, 0);
    return 0;
}

int
BrowserValidateGCFunction(
    Display **displayPtr,
    int function,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, function,
	    (function >= GXclear) && (function <= GXset), requestCode);
}

int
BrowserValidateGCLineStyle(
    Display **displayPtr,
    int lineStyle,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, lineStyle,
	    (lineStyle >= LineSolid) && (lineStyle <= LineDoubleDash),
	    requestCode);
}

int
BrowserValidateGCCapStyle(
    Display **displayPtr,
    int capStyle,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, capStyle,
	    (capStyle >= CapNotLast) && (capStyle <= CapProjecting),
	    requestCode);
}

int
BrowserValidateGCJoinStyle(
    Display **displayPtr,
    int joinStyle,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, joinStyle,
	    (joinStyle >= JoinMiter) && (joinStyle <= JoinBevel),
	    requestCode);
}

int
BrowserValidateGCFillStyle(
    Display **displayPtr,
    int fillStyle,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, fillStyle,
	    (fillStyle >= FillSolid) && (fillStyle <= FillOpaqueStippled),
	    requestCode);
}

int
BrowserValidateGCFillRule(
    Display **displayPtr,
    int fillRule,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, fillRule,
	    (fillRule >= EvenOddRule) && (fillRule <= WindingRule),
	    requestCode);
}

int
BrowserValidateGCArcMode(
    Display **displayPtr,
    int arcMode,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, arcMode,
	    (arcMode >= ArcChord) && (arcMode <= ArcPieSlice), requestCode);
}

int
BrowserValidateGCSubwindowMode(
    Display **displayPtr,
    int subwindowMode,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, subwindowMode,
	    (subwindowMode >= ClipByChildren)
	    && (subwindowMode <= IncludeInferiors), requestCode);
}

int
BrowserValidateCoordMode(
    Display **displayPtr,
    int coordMode,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, coordMode,
	    (coordMode == CoordModeOrigin) || (coordMode == CoordModePrevious),
	    requestCode);
}

int
BrowserValidatePolygonShape(
    Display **displayPtr,
    int shape,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, shape,
	    (shape == Complex) || (shape == Nonconvex) || (shape == Convex),
	    requestCode);
}

int
BrowserValidateBool(
    Display **displayPtr,
    int value,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, value,
	    (value == False) || (value == True), requestCode);
}

int
BrowserValidateGCBool(
    Display **displayPtr,
    int value,
    unsigned char requestCode)
{
    return BrowserValidateBool(displayPtr, value, requestCode);
}

int
BrowserValidateScreenSaverPreferBlanking(
    Display **displayPtr,
    int value,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, value,
	    (value >= DontPreferBlanking) && (value <= DefaultBlanking),
	    requestCode);
}

int
BrowserValidateScreenSaverAllowExposures(
    Display **displayPtr,
    int value,
    unsigned char requestCode)
{
    return BrowserValueOrError(displayPtr, value,
	    (value >= DontAllowExposures) && (value <= DefaultExposures),
	    requestCode);
}

int
BrowserValidateCountedList(
    Display **displayPtr,
    const void *items,
    int count,
    unsigned char requestCode)
{
    if (count < 0) {
	return BrowserValueOrError(displayPtr, count, 0, requestCode);
    }
    if ((count > 0) && (items == NULL)) {
	return BrowserValueOrError(displayPtr, count, 0, requestCode);
    }
    return 1;
}

int
BrowserValidateGCDashList(
    Display **displayPtr,
    const char *dashList,
    int count,
    unsigned char requestCode)
{
    int i;

    if (count <= 0) {
	return BrowserValueOrError(displayPtr, count, 0, requestCode);
    }
    if (dashList == NULL) {
	return BrowserValueOrError(displayPtr, 0, 0, requestCode);
    }
    for (i = 0; i < count; i++) {
	if (dashList[i] == '\0') {
	    return BrowserValueOrError(displayPtr, 0, 0, requestCode);
	}
    }
    return 1;
}

int
BrowserValidateGCClipRectangles(
    Display **displayPtr,
    const XRectangle *rectangles,
    int count,
    int ordering,
    unsigned char requestCode)
{
    if ((ordering < Unsorted) || (ordering > YXBanded)) {
	return BrowserValueOrError(displayPtr, ordering, 0, requestCode);
    }
    if (count < 0) {
	return BrowserValueOrError(displayPtr, count, 0, requestCode);
    }
    if ((count > 0) && (rectangles == NULL)) {
	return BrowserValueOrError(displayPtr, count, 0, requestCode);
    }
    return 1;
}

int
BrowserValidateGCValueEnums(
    Display **displayPtr,
    unsigned long mask,
    XGCValues *values,
    unsigned char requestCode)
{
    if (values == NULL) {
	return 1;
    }
    if ((mask & GCFunction)
	    && !BrowserValidateGCFunction(displayPtr, values->function,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCLineWidth)
	    && !BrowserValidateCard16(displayPtr, values->line_width, 1,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCLineStyle)
	    && !BrowserValidateGCLineStyle(displayPtr, values->line_style,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCCapStyle)
	    && !BrowserValidateGCCapStyle(displayPtr, values->cap_style,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCJoinStyle)
	    && !BrowserValidateGCJoinStyle(displayPtr, values->join_style,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCFillStyle)
	    && !BrowserValidateGCFillStyle(displayPtr, values->fill_style,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCFillRule)
	    && !BrowserValidateGCFillRule(displayPtr, values->fill_rule,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCSubwindowMode)
	    && !BrowserValidateGCSubwindowMode(displayPtr,
		    values->subwindow_mode, requestCode)) {
	return 0;
    }
    if ((mask & GCGraphicsExposures)
	    && !BrowserValidateGCBool(displayPtr, values->graphics_exposures,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCDashOffset)
	    && !BrowserValidateCard16(displayPtr, values->dash_offset, 1,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCArcMode)
	    && !BrowserValidateGCArcMode(displayPtr, values->arc_mode,
		    requestCode)) {
	return 0;
    }
    if ((mask & GCDashList)
	    && !BrowserValidateGCDashList(displayPtr,
		    (const char *)&values->dashes, 1, requestCode)) {
	return 0;
    }
    return 1;
}

int
BrowserGCValueMaskOrError(
    Display **displayPtr,
    unsigned long mask,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

    if (BrowserGCChangeValueMaskIsValid(mask)) {
	return 1;
    }
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    (void)BrowserReportXError(display, BadValue, (XID)mask, requestCode, 0);
    return 0;
}

int
BrowserGCDepthsMatchOrError(
    Display **displayPtr,
    GC src,
    GC dest,
    unsigned char requestCode)
{
    Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

    if (BrowserGCDepth(src) == BrowserGCDepth(dest)) {
	return 1;
    }
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
	if (displayPtr != NULL) {
	    *displayPtr = display;
	}
    }
    (void)BrowserReportXError(display, BadMatch, (XID)(uintptr_t)dest,
	    requestCode, 0);
    return 0;
}

int
BrowserHexDigit(
    int ch)
{
    if ((ch >= '0') && (ch <= '9')) {
	return ch - '0';
    }
    if ((ch >= 'a') && (ch <= 'f')) {
	return ch - 'a' + 10;
    }
    if ((ch >= 'A') && (ch <= 'F')) {
	return ch - 'A' + 10;
    }
    return -1;
}

void
BrowserDStringAppendHexByte(
    X11WasmDString *dsPtr,
    unsigned char byte)
{
    static const char digits[] = "0123456789abcdef";
    char hex[2];

    hex[0] = digits[(byte >> 4) & 0xfU];
    hex[1] = digits[byte & 0xfU];
    X11WasmDStringAppend(dsPtr, hex, 2);
}

void
BrowserSyncHostAccessControl(void)
{
    X11WasmDString hosts;
    int i;

    X11WasmDStringInit(&hosts);
    for (i = 0; i < browserDisplay.hostCount; i++) {
	X11WasmHostEntry *hostPtr = &browserDisplay.hosts[i];
	char prefix[64];
	int j;

	if (i > 0) {
	    X11WasmDStringAppend(&hosts, ";", 1);
	}
	snprintf(prefix, sizeof(prefix), "%d:%d:", hostPtr->family,
		hostPtr->length);
	X11WasmDStringAppend(&hosts, prefix, -1);
	for (j = 0; (hostPtr->address != NULL) && (j < hostPtr->length); j++) {
	    BrowserDStringAppendHexByte(&hosts,
		    (unsigned char)hostPtr->address[j]);
	}
    }
    (void)X11WasmJsSetHostAccessControl(browserDisplay.hostAccessEnabled,
	    X11WasmDStringValue(&hosts), X11WasmDStringLength(&hosts),
	    browserDisplay.hostCount);
    X11WasmDStringFree(&hosts);
}

void
BrowserSyncServerGrabState(void)
{
    (void)X11WasmJsSetServerGrabState(browserDisplay.serverGrabDepth);
}

int
BrowserUtf8FromCodepoint(
    unsigned int codepoint,
    char *buffer,
    int bufferLength)
{
    if ((buffer == NULL) || (bufferLength <= 0)) {
	return 0;
    }
    if (codepoint <= 0x7fU) {
	if (bufferLength < 1) return 0;
	buffer[0] = (char)codepoint;
	return 1;
    }
    if (codepoint <= 0x7ffU) {
	if (bufferLength < 2) return 0;
	buffer[0] = (char)(0xc0U | (codepoint >> 6));
	buffer[1] = (char)(0x80U | (codepoint & 0x3fU));
	return 2;
    }
    if (codepoint <= 0xffffU) {
	if (bufferLength < 3) return 0;
	buffer[0] = (char)(0xe0U | (codepoint >> 12));
	buffer[1] = (char)(0x80U | ((codepoint >> 6) & 0x3fU));
	buffer[2] = (char)(0x80U | (codepoint & 0x3fU));
	return 3;
    }
    if (codepoint <= 0x10ffffU) {
	if (bufferLength < 4) return 0;
	buffer[0] = (char)(0xf0U | (codepoint >> 18));
	buffer[1] = (char)(0x80U | ((codepoint >> 12) & 0x3fU));
	buffer[2] = (char)(0x80U | ((codepoint >> 6) & 0x3fU));
	buffer[3] = (char)(0x80U | (codepoint & 0x3fU));
	return 4;
    }
    return 0;
}

int
BrowserUtf8CodepointLength(
    unsigned int codepoint)
{
    if (codepoint <= 0x7fU) {
	return 1;
    }
    if (codepoint <= 0x7ffU) {
	return 2;
    }
    if (codepoint <= 0xffffU) {
	return 3;
    }
    if (codepoint <= 0x10ffffU) {
	return 4;
    }
    return 0;
}

int
BrowserLatin1FromKeySym(
    KeySym keySym,
    unsigned char *bytePtr)
{
    unsigned char byte;

    if ((keySym >= 0x20) && (keySym <= 0xff)) {
	byte = (unsigned char)keySym;
    } else {
	switch (keySym) {
	case XK_BackSpace:
	    byte = '\b';
	    break;
	case XK_Tab:
	case XK_KP_Tab:
	case XK_ISO_Left_Tab:
	    byte = '\t';
	    break;
	case XK_Linefeed:
	    byte = '\n';
	    break;
	case XK_Clear:
	    byte = '\v';
	    break;
	case XK_Return:
	case XK_KP_Enter:
	    byte = '\r';
	    break;
	case XK_Escape:
	    byte = 0x1b;
	    break;
	case XK_Delete:
	case XK_KP_Delete:
	    byte = 0x7f;
	    break;
	case XK_KP_Space:
	    byte = ' ';
	    break;
	case XK_KP_Multiply:
	    byte = '*';
	    break;
	case XK_KP_Add:
	    byte = '+';
	    break;
	case XK_KP_Separator:
	    byte = ',';
	    break;
	case XK_KP_Subtract:
	    byte = '-';
	    break;
	case XK_KP_Decimal:
	    byte = '.';
	    break;
	case XK_KP_Divide:
	    byte = '/';
	    break;
	case XK_KP_0:
	case XK_KP_1:
	case XK_KP_2:
	case XK_KP_3:
	case XK_KP_4:
	case XK_KP_5:
	case XK_KP_6:
	case XK_KP_7:
	case XK_KP_8:
	case XK_KP_9:
	    byte = (unsigned char)('0' + (keySym - XK_KP_0));
	    break;
	case XK_KP_Equal:
	    byte = '=';
	    break;
	default:
	    return 0;
	}
    }
    if (bytePtr != NULL) {
	*bytePtr = byte;
    }
    return 1;
}

KeySym
BrowserKeySymFromCodepoint(
    unsigned int codepoint)
{
    if (codepoint <= 0xffU) {
	return (KeySym)codepoint;
    }
    if (codepoint <= 0x10ffffU) {
	return (KeySym)(X11_WASM_UNICODE_KEYSYM_BASE | codepoint);
    }
    return NoSymbol;
}

unsigned int BrowserKnownKeycodeForKeySym(KeySym keySym);
unsigned int BrowserAsciiKeycodeFromKeySym(KeySym keySym);

void
BrowserSetModifierKey(
    X11WasmDisplayState *state,
    int index,
    int slot,
    KeySym keySym)
{
    int maxKeyPerMod = state->modifierMapMaxKeyPerMod;
    KeyCode keycode;

    if ((index < 0) || (index >= 8) || (slot < 0)
	    || (slot >= maxKeyPerMod)) {
	return;
    }
    keycode = (KeyCode)BrowserKnownKeycodeForKeySym(keySym);
    if (keycode == 0) {
	keycode = (KeyCode)BrowserAsciiKeycodeFromKeySym(keySym);
    }
    if (keycode == 0) {
	keycode = (KeyCode)keySym;
    }
    state->modifierMap[index * maxKeyPerMod + slot] = keycode;
}

void
BrowserInitModifierMap(
    X11WasmDisplayState *state)
{
    state->modifierMapMaxKeyPerMod = 2;
    memset(state->modifierMap, 0, sizeof(state->modifierMap));
    BrowserSetModifierKey(state, ShiftMapIndex, 0, XK_Shift_L);
    BrowserSetModifierKey(state, ShiftMapIndex, 1, XK_Shift_R);
    BrowserSetModifierKey(state, LockMapIndex, 0, XK_Caps_Lock);
    BrowserSetModifierKey(state, ControlMapIndex, 0, XK_Control_L);
    BrowserSetModifierKey(state, ControlMapIndex, 1, XK_Control_R);
    BrowserSetModifierKey(state, Mod1MapIndex, 0, XK_Alt_L);
    BrowserSetModifierKey(state, Mod1MapIndex, 1, XK_Alt_R);
    BrowserSetModifierKey(state, Mod2MapIndex, 0, XK_Num_Lock);
    BrowserSetModifierKey(state, Mod4MapIndex, 0, XK_Meta_L);
    BrowserSetModifierKey(state, Mod4MapIndex, 1, XK_Meta_R);
    BrowserSetModifierKey(state, Mod5MapIndex, 0, XK_Mode_switch);
    state->modifierMapSerial++;
}

void
BrowserInitKeyboardMap(
    X11WasmDisplayState *state)
{
    memset(state->keycodeMap, 0, sizeof(state->keycodeMap));
    state->nextSyntheticKeycode = X11_WASM_SYNTHETIC_KEYCODE_START;
    state->keyboardMapSerial++;
    BrowserInitModifierMap(state);
}

int
BrowserIsUnicodeKeySym(
    KeySym keySym)
{
    return (((unsigned int)keySym & 0xff000000U)
	    == X11_WASM_UNICODE_KEYSYM_BASE);
}

void
BrowserRememberKeycode(
    unsigned int keycode,
    KeySym keySym)
{
    if ((keycode <= X11_WASM_MAX_KEYCODE) && (keySym != NoSymbol)) {
	if (browserDisplay.keycodeMap[keycode] == keySym) {
	    return;
	}
	browserDisplay.keycodeMap[keycode] = keySym;
	browserDisplay.keyboardMapSerial++;
    }
}

void
BrowserClearKeycodeRange(
    unsigned int firstKeycode,
    unsigned int count)
{
    unsigned int first;
    unsigned int last;
    unsigned int keycode;

    if (count == 0) {
	return;
    }
    first = (firstKeycode < X11_WASM_MIN_KEYCODE)
	    ? X11_WASM_MIN_KEYCODE : firstKeycode;
    if (first > X11_WASM_MAX_KEYCODE) {
	return;
    }
    last = firstKeycode + count - 1;
    if ((last < firstKeycode) || (last > X11_WASM_MAX_KEYCODE)) {
	last = X11_WASM_MAX_KEYCODE;
    }
    for (keycode = first; keycode <= last; keycode++) {
	browserDisplay.keycodeMap[keycode] = NoSymbol;
    }
    browserDisplay.keyboardMapSerial++;
}

unsigned int
BrowserKnownKeycodeForKeySym(
    KeySym keySym)
{
    switch (keySym) {
    case XK_Shift_L:
	return X11_WASM_KEYCODE_SHIFT;
    case XK_Shift_R:
	return X11_WASM_KEYCODE_SHIFT_RIGHT;
    case XK_Control_L:
	return X11_WASM_KEYCODE_CONTROL;
    case XK_Control_R:
	return X11_WASM_KEYCODE_CONTROL_RIGHT;
    case XK_Alt_L:
	return X11_WASM_KEYCODE_ALT;
    case XK_Alt_R:
	return X11_WASM_KEYCODE_ALT_RIGHT;
    case XK_Caps_Lock:
    case XK_Shift_Lock:
	return X11_WASM_KEYCODE_CAPS_LOCK;
    case XK_Meta_L:
	return X11_WASM_KEYCODE_META;
    case XK_Meta_R:
	return X11_WASM_KEYCODE_META_RIGHT;
    case XK_Num_Lock:
	return X11_WASM_KEYCODE_NUM_LOCK;
    case XK_Mode_switch:
	return X11_WASM_KEYCODE_MODE_SWITCH;
    default:
	return 0;
    }
}

KeySym
BrowserKnownKeySymForKeycode(
    unsigned int keycode)
{
    switch (keycode) {
    case X11_WASM_KEYCODE_SHIFT:
	return XK_Shift_L;
    case X11_WASM_KEYCODE_SHIFT_RIGHT:
	return XK_Shift_R;
    case X11_WASM_KEYCODE_CONTROL:
	return XK_Control_L;
    case X11_WASM_KEYCODE_CONTROL_RIGHT:
	return XK_Control_R;
    case X11_WASM_KEYCODE_ALT:
	return XK_Alt_L;
    case X11_WASM_KEYCODE_ALT_RIGHT:
	return XK_Alt_R;
    case X11_WASM_KEYCODE_CAPS_LOCK:
	return XK_Caps_Lock;
    case X11_WASM_KEYCODE_META:
	return XK_Meta_L;
    case X11_WASM_KEYCODE_META_RIGHT:
	return XK_Meta_R;
    case X11_WASM_KEYCODE_NUM_LOCK:
	return XK_Num_Lock;
    case X11_WASM_KEYCODE_MODE_SWITCH:
	return XK_Mode_switch;
    default:
	return NoSymbol;
    }
}

unsigned int
BrowserNextSyntheticKeycode(void)
{
    unsigned int keycode = browserDisplay.nextSyntheticKeycode;

    browserDisplay.nextSyntheticKeycode++;
    if (browserDisplay.nextSyntheticKeycode > X11_WASM_MAX_KEYCODE) {
	browserDisplay.nextSyntheticKeycode = X11_WASM_SYNTHETIC_KEYCODE_START;
    }
    return keycode;
}

KeySym
BrowserUnshiftedAsciiKeySym(
    KeySym keySym)
{
    if ((keySym >= XK_A) && (keySym <= XK_Z)) {
	return keySym + (XK_a - XK_A);
    }
    if (((keySym >= XK_a) && (keySym <= XK_z))
	    || ((keySym >= XK_0) && (keySym <= XK_9))
	    || (keySym == XK_space)) {
	return keySym;
    }
    switch (keySym) {
    case XK_exclam: return XK_1;
    case XK_at: return XK_2;
    case XK_numbersign: return XK_3;
    case XK_dollar: return XK_4;
    case XK_percent: return XK_5;
    case XK_asciicircum: return XK_6;
    case XK_ampersand: return XK_7;
    case XK_asterisk: return XK_8;
    case XK_parenleft: return XK_9;
    case XK_parenright: return XK_0;
    case XK_minus:
    case XK_underscore:
	return XK_minus;
    case XK_equal:
    case XK_plus:
	return XK_equal;
    case XK_bracketleft:
    case XK_braceleft:
	return XK_bracketleft;
    case XK_bracketright:
    case XK_braceright:
	return XK_bracketright;
    case XK_backslash:
    case XK_bar:
	return XK_backslash;
    case XK_semicolon:
    case XK_colon:
	return XK_semicolon;
    case XK_apostrophe:
    case XK_quotedbl:
	return XK_apostrophe;
    case XK_comma:
    case XK_less:
	return XK_comma;
    case XK_period:
    case XK_greater:
	return XK_period;
    case XK_slash:
    case XK_question:
	return XK_slash;
    case XK_grave:
    case XK_asciitilde:
	return XK_grave;
    default:
	return NoSymbol;
    }
}

KeySym
BrowserShiftedAsciiKeySym(
    KeySym baseKeySym)
{
    if ((baseKeySym >= XK_a) && (baseKeySym <= XK_z)) {
	return baseKeySym - (XK_a - XK_A);
    }
    switch (baseKeySym) {
    case XK_1: return XK_exclam;
    case XK_2: return XK_at;
    case XK_3: return XK_numbersign;
    case XK_4: return XK_dollar;
    case XK_5: return XK_percent;
    case XK_6: return XK_asciicircum;
    case XK_7: return XK_ampersand;
    case XK_8: return XK_asterisk;
    case XK_9: return XK_parenleft;
    case XK_0: return XK_parenright;
    case XK_minus: return XK_underscore;
    case XK_equal: return XK_plus;
    case XK_bracketleft: return XK_braceleft;
    case XK_bracketright: return XK_braceright;
    case XK_backslash: return XK_bar;
    case XK_semicolon: return XK_colon;
    case XK_apostrophe: return XK_quotedbl;
    case XK_comma: return XK_less;
    case XK_period: return XK_greater;
    case XK_slash: return XK_question;
    case XK_grave: return XK_asciitilde;
    case XK_space: return XK_space;
    default:
	return NoSymbol;
    }
}

void
XConvertCase(
    KeySym sym,
    KeySym *lowerReturn,
    KeySym *upperReturn)
{
    KeySym lower = sym;
    KeySym upper = sym;

    if ((sym >= XK_A) && (sym <= XK_Z)) {
	lower = sym + (XK_a - XK_A);
    } else if ((sym >= XK_a) && (sym <= XK_z)) {
	upper = sym - (XK_a - XK_A);
    } else if ((sym >= XK_Agrave) && (sym <= XK_Odiaeresis)) {
	lower = sym + (XK_agrave - XK_Agrave);
    } else if ((sym >= XK_agrave) && (sym <= XK_odiaeresis)) {
	upper = sym - (XK_agrave - XK_Agrave);
    } else if ((sym >= XK_Ooblique) && (sym <= XK_Thorn)) {
	lower = sym + (XK_oslash - XK_Ooblique);
    } else if ((sym >= XK_oslash) && (sym <= XK_thorn)) {
	upper = sym - (XK_oslash - XK_Ooblique);
    }

    if (lowerReturn != NULL) {
	*lowerReturn = lower;
    }
    if (upperReturn != NULL) {
	*upperReturn = upper;
    }
}

unsigned int
BrowserAsciiKeycodeFromKeySym(
    KeySym keySym)
{
    KeySym baseKeySym = BrowserUnshiftedAsciiKeySym(keySym);

    if (baseKeySym == NoSymbol) {
	return 0;
    }
    if ((baseKeySym >= XK_a) && (baseKeySym <= XK_z)) {
	return (unsigned int)(baseKeySym - (XK_a - XK_A));
    }
    switch (baseKeySym) {
    case XK_minus: return X11_WASM_KEYCODE_MINUS;
    case XK_equal: return X11_WASM_KEYCODE_EQUAL;
    case XK_bracketleft: return X11_WASM_KEYCODE_BRACKET_LEFT;
    case XK_bracketright: return X11_WASM_KEYCODE_BRACKET_RIGHT;
    case XK_backslash: return X11_WASM_KEYCODE_BACKSLASH;
    case XK_semicolon: return X11_WASM_KEYCODE_SEMICOLON;
    case XK_apostrophe: return X11_WASM_KEYCODE_APOSTROPHE;
    case XK_comma: return X11_WASM_KEYCODE_COMMA;
    case XK_period: return X11_WASM_KEYCODE_PERIOD;
    case XK_slash: return X11_WASM_KEYCODE_SLASH;
    case XK_grave: return X11_WASM_KEYCODE_GRAVE;
    default:
	break;
    }
    return (unsigned int)baseKeySym;
}

KeySym
BrowserBaseAsciiKeySymForKeycode(
    unsigned int keycode)
{
    if ((keycode >= XK_A) && (keycode <= XK_Z)) {
	return (KeySym)(keycode + (XK_a - XK_A));
    }
    if ((keycode >= XK_a) && (keycode <= XK_z)) {
	return (KeySym)keycode;
    }
    if (((keycode >= XK_0) && (keycode <= XK_9))
	    || (keycode == XK_space)) {
	return (KeySym)keycode;
    }
    switch (keycode) {
    case X11_WASM_KEYCODE_MINUS: return XK_minus;
    case X11_WASM_KEYCODE_EQUAL: return XK_equal;
    case X11_WASM_KEYCODE_BRACKET_LEFT: return XK_bracketleft;
    case X11_WASM_KEYCODE_BRACKET_RIGHT: return XK_bracketright;
    case X11_WASM_KEYCODE_BACKSLASH: return XK_backslash;
    case X11_WASM_KEYCODE_SEMICOLON: return XK_semicolon;
    case X11_WASM_KEYCODE_APOSTROPHE: return XK_apostrophe;
    case X11_WASM_KEYCODE_COMMA: return XK_comma;
    case X11_WASM_KEYCODE_PERIOD: return XK_period;
    case X11_WASM_KEYCODE_SLASH: return XK_slash;
    case X11_WASM_KEYCODE_GRAVE: return XK_grave;
    default:
	break;
    }
    return NoSymbol;
}

KeySym
BrowserKeySymForKeycodeLevel(
    unsigned int keycode,
    int index)
{
    KeySym baseKeySym = BrowserBaseAsciiKeySymForKeycode(keycode);
    KeySym knownKeySym;

    if ((index < 0) || (index >= X11_WASM_KEYSYMS_PER_KEYCODE)) {
	return NoSymbol;
    }
    if (baseKeySym != NoSymbol) {
	if (index & 1) {
	    KeySym shiftedKeySym = BrowserShiftedAsciiKeySym(baseKeySym);

	    if (shiftedKeySym != NoSymbol) {
		return shiftedKeySym;
	    }
	}
	return baseKeySym;
    }
    knownKeySym = BrowserKnownKeySymForKeycode(keycode);
    if (knownKeySym != NoSymbol) {
	return knownKeySym;
    }
    if ((keycode <= X11_WASM_MAX_KEYCODE)
	    && (browserDisplay.keycodeMap[keycode] != NoSymbol)) {
	return browserDisplay.keycodeMap[keycode];
    }
    if ((keycode >= X11_WASM_MIN_KEYCODE)
	    && (keycode <= X11_WASM_MAX_KEYCODE)) {
	return (KeySym)keycode;
    }
    return NoSymbol;
}

unsigned int
BrowserKeycodeFromKeySym(
    KeySym keySym)
{
    unsigned int keycode;

    if (keySym == NoSymbol) {
	return 0;
    }
    keycode = BrowserKnownKeycodeForKeySym(keySym);
    if (keycode != 0) {
	BrowserRememberKeycode(keycode, keySym);
	return keycode;
    }
    keycode = BrowserAsciiKeycodeFromKeySym(keySym);
    if (keycode != 0) {
	BrowserRememberKeycode(keycode, keySym);
	return keycode;
    }
    keycode = ((unsigned int)keySym & 0xffU);
    if (BrowserIsUnicodeKeySym(keySym)
	    || (keycode < X11_WASM_MIN_KEYCODE)
	    || (keycode > X11_WASM_MAX_KEYCODE)) {
	keycode = BrowserNextSyntheticKeycode();
    }
    BrowserRememberKeycode(keycode, keySym);
    return keycode;
}

unsigned int
BrowserNormalizeKeycode(
    unsigned int keycode,
    KeySym keySym)
{
    if ((keycode < X11_WASM_MIN_KEYCODE)
	    || (keycode > X11_WASM_MAX_KEYCODE)) {
	return BrowserKeycodeFromKeySym(keySym);
    }
    BrowserRememberKeycode(keycode, keySym);
    return keycode;
}

int
BrowserHasCasePrefix(
    const char *string,
    const char *prefix)
{
    while (*prefix != '\0') {
	if (tolower((unsigned char)*string)
		!= tolower((unsigned char)*prefix)) {
	    return 0;
	}
	string++;
	prefix++;
    }
    return 1;
}

int
BrowserParseHexComponent(
    const char *string,
    size_t digits,
    unsigned int *valuePtr)
{
    size_t i;
    unsigned int value = 0;
    unsigned int maxValue = 0;

    for (i = 0; i < digits; i++) {
	int digit = BrowserHexDigit(string[i]);

	if (digit < 0) {
	    return 0;
	}
	value = (value << 4) | (unsigned int)digit;
	maxValue = (maxValue << 4) | 0xfU;
    }
    *valuePtr = (unsigned int)(((unsigned long)value * 0xffffUL
	    + (maxValue / 2U)) / maxValue);
    return 1;
}

int
BrowserParseRgbHexSpec(
    const char *string,
    unsigned int *redPtr,
    unsigned int *greenPtr,
    unsigned int *bluePtr)
{
    unsigned int values[3];
    int component;

    for (component = 0; component < 3; component++) {
	const char *start = string;
	size_t digits = 0;

	while ((digits < 4) && (BrowserHexDigit(string[digits]) >= 0)) {
	    digits++;
	}
	if ((digits < 1)
		|| !BrowserParseHexComponent(start, digits,
			&values[component])) {
	    return 0;
	}
	string += digits;
	if (component < 2) {
	    if (*string != '/') {
		return 0;
	    }
	    string++;
	} else if (*string != '\0') {
	    return 0;
	}
    }
    *redPtr = values[0];
    *greenPtr = values[1];
    *bluePtr = values[2];
    return 1;
}

int
BrowserParseRgbiComponent(
    const char **cursorPtr,
    unsigned int *valuePtr)
{
    const char *cursor = *cursorPtr;
    double value = 0.0;
    double scale = 0.1;
    int sawDigit = 0;

    while (isdigit((unsigned char)*cursor)) {
	value = value * 10.0 + (double)(*cursor - '0');
	cursor++;
	sawDigit = 1;
    }
    if (*cursor == '.') {
	cursor++;
	while (isdigit((unsigned char)*cursor)) {
	    value += scale * (double)(*cursor - '0');
	    scale *= 0.1;
	    cursor++;
	    sawDigit = 1;
	}
    }
    if (!sawDigit || (value < 0.0) || (value > 1.0)) {
	return 0;
    }
    *valuePtr = (unsigned int)(value * 65535.0 + 0.5);
    *cursorPtr = cursor;
    return 1;
}

int
BrowserParseRgbiSpec(
    const char *string,
    unsigned int *redPtr,
    unsigned int *greenPtr,
    unsigned int *bluePtr)
{
    unsigned int values[3];
    int component;

    for (component = 0; component < 3; component++) {
	if (!BrowserParseRgbiComponent(&string, &values[component])) {
	    return 0;
	}
	if (component < 2) {
	    if (*string != '/') {
		return 0;
	    }
	    string++;
	} else if (*string != '\0') {
	    return 0;
	}
    }
    *redPtr = values[0];
    *greenPtr = values[1];
    *bluePtr = values[2];
    return 1;
}

const char *
BrowserCompositionEventName(
    int phase)
{
    switch (phase) {
    case X11_WASM_COMPOSITION_START:
	return "X11WasmCompositionStart";
    case X11_WASM_COMPOSITION_UPDATE:
	return "X11WasmCompositionUpdate";
    case X11_WASM_COMPOSITION_END:
	return "X11WasmCompositionEnd";
    default:
	return NULL;
    }
}

unsigned int
BrowserJsXId(
    XID xid)
{
    return (unsigned int)xid;
}

unsigned long
BrowserGCPixel(
    GC gc)
{
    if (gc == NULL) {
	return 0;
    }
    return gc->foreground;
}

unsigned int
BrowserGCLineWidth(
    GC gc)
{
    if ((gc == NULL) || (gc->line_width <= 0)) {
	return 1;
    }
    return (unsigned int)gc->line_width;
}

int
BrowserFontField(
    const char *name,
    int field,
    const char **startPtr,
    size_t *lengthPtr)
{
    const char *start;
    const char *end;
    int current;

    if ((name == NULL) || (name[0] != '-') || (field <= 0)) {
	return 0;
    }
    start = name + 1;
    current = 1;
    while (current < field) {
	start = strchr(start, '-');
	if (start == NULL) {
	    return 0;
	}
	start++;
	current++;
    }
    end = strchr(start, '-');
    if (end == NULL) {
	end = start + strlen(start);
    }
    if (startPtr != NULL) {
	*startPtr = start;
    }
    if (lengthPtr != NULL) {
	*lengthPtr = (size_t)(end - start);
    }
    return 1;
}

int
BrowserFontFieldInt(
    const char *name,
    int field)
{
    const char *start;
    size_t length;
    char buffer[32];
    char *endPtr;
    long value;

    if (!BrowserFontField(name, field, &start, &length)
	    || (length == 0) || (length >= sizeof(buffer))
	    || (start[0] == '*')) {
	return 0;
    }
    memcpy(buffer, start, length);
    buffer[length] = '\0';
    value = strtol(buffer, &endPtr, 10);
    if ((endPtr == buffer) || (*endPtr != '\0') || (value <= 0)
	    || (value > 512)) {
	return 0;
    }
    return (int)value;
}

char *
BrowserFontFamilyFromName(
    const char *name)
{
    const char *start;
    size_t length;
    char *family;
    size_t i;

    if (BrowserFontField(name, 2, &start, &length) && (length > 0)
	    && (start[0] != '*')) {
	family = (char *)ckalloc(length + 1);
	memcpy(family, start, length);
	family[length] = '\0';
    } else if ((name != NULL) && (name[0] != '\0') && (name[0] != '*')
	    && (strchr(name, '-') == NULL)) {
	family = BrowserDupString(name);
    } else {
	family = BrowserDupString("fixed");
    }
    for (i = 0; family[i] != '\0'; i++) {
	if ((family[i] == '*') || (family[i] == '?')
		|| (family[i] == '-') || isspace((unsigned char)family[i])) {
	    family[i] = '_';
	}
    }
    return family;
}

int
BrowserFontPatternMatches(
    const char *pattern,
    size_t patternLength,
    const char *string)
{
    while (patternLength > 0) {
	if (*pattern == '*') {
	    pattern++;
	    patternLength--;
	    if (patternLength == 0) {
		return 1;
	    }
	    while (*string != '\0') {
		if (BrowserFontPatternMatches(pattern, patternLength, string)) {
		    return 1;
		}
		string++;
	    }
	    return BrowserFontPatternMatches(pattern, patternLength, string);
	}
	if ((*string == '\0') || ((*pattern != '?')
		&& (tolower((unsigned char)*pattern)
		!= tolower((unsigned char)*string)))) {
	    return 0;
	}
	pattern++;
	patternLength--;
	string++;
    }
    return *string == '\0';
}

int
BrowserFontPatternIsWildcard(
    const char *pattern,
    size_t patternLength)
{
    size_t i;

    if (patternLength == 0) {
	return 1;
    }
    for (i = 0; i < patternLength; i++) {
	if (pattern[i] != '*') {
	    return 0;
	}
    }
    return 1;
}

int
BrowserFontPatternHasWildcards(
    const char *pattern,
    size_t patternLength)
{
    size_t i;

    for (i = 0; i < patternLength; i++) {
	if ((pattern[i] == '*') || (pattern[i] == '?')) {
	    return 1;
	}
    }
    return 0;
}

const char *const browserFontFallbackFamilies[] = {
    "fixed",
    "Helvetica",
    "Times",
    "Courier",
    "Arial",
    NULL
};

int
BrowserFontPixelSize(
    const char *name)
{
    int pixelSize = BrowserFontFieldInt(name, 7);
    int pointSize;

    if (pixelSize > 0) {
	return pixelSize;
    }
    pointSize = BrowserFontFieldInt(name, 8);
    if (pointSize > 0) {
	return (pointSize + 5) / 10;
    }
    return 14;
}

int
BrowserFontPointSize(
    const char *name,
    int pixelSize)
{
    int pointSize = BrowserFontFieldInt(name, 8);

    if (pointSize > 0) {
	return pointSize;
    }
    if (pixelSize <= 0) {
	pixelSize = 14;
    }
    return (pixelSize * 720 + 48) / 96;
}

char *
BrowserFontXLFD(
    const char *name,
    int scalable)
{
    char *family = BrowserFontFamilyFromName(name);
    int pixelSize = scalable ? 0 : BrowserFontPixelSize(name);
    char buffer[256];

    snprintf(buffer, sizeof(buffer),
	    "-browser-%.80s-medium-r-normal--%d-0-0-0-p-0-iso8859-1",
	    family, pixelSize);
    ckfree(family);
    return BrowserDupString(buffer);
}

int
BrowserFontCharWidth(
    const XFontStruct *fontPtr)
{
    if ((fontPtr != NULL) && (fontPtr->max_bounds.width > 0)) {
	return fontPtr->max_bounds.width;
    }
    return 8;
}

char *
BrowserFirstFontName(
    const char *nameList)
{
    const char *start = nameList;
    const char *end;
    char *name;
    size_t length;

    if (start == NULL) {
	return BrowserDupString("");
    }
    while ((*start != '\0') && (isspace((unsigned char)*start)
	    || (*start == ','))) {
	start++;
    }
    end = start;
    while ((*end != '\0') && (*end != ',')) {
	end++;
    }
    while ((end > start) && isspace((unsigned char)end[-1])) {
	end--;
    }
    length = (size_t)(end - start);
    name = (char *)ckalloc(length + 1);
    memcpy(name, start, length);
    name[length] = '\0';
    return name;
}

int
BrowserFontNameListCount(
    const char *nameList)
{
    const char *cursor = nameList;
    int count = 0;

    if (cursor == NULL) {
	return 0;
    }
    while (*cursor != '\0') {
	while ((*cursor != '\0') && (isspace((unsigned char)*cursor)
		|| (*cursor == ','))) {
	    cursor++;
	}
	if (*cursor == '\0') {
	    break;
	}
	count++;
	while ((*cursor != '\0') && (*cursor != ',')) {
	    cursor++;
	}
    }
    return count;
}

char *
BrowserNextFontName(
    const char **cursorPtr)
{
    const char *start;
    const char *end;
    char *name;
    size_t length;

    if ((cursorPtr == NULL) || (*cursorPtr == NULL)) {
	return NULL;
    }
    start = *cursorPtr;
    while ((*start != '\0') && (isspace((unsigned char)*start)
	    || (*start == ','))) {
	start++;
    }
    if (*start == '\0') {
	*cursorPtr = start;
	return NULL;
    }
    end = start;
    while ((*end != '\0') && (*end != ',')) {
	end++;
    }
    while ((end > start) && isspace((unsigned char)end[-1])) {
	end--;
    }
    length = (size_t)(end - start);
    name = (char *)ckalloc(length + 1);
    memcpy(name, start, length);
    name[length] = '\0';
    *cursorPtr = (*end == ',') ? end + 1 : end;
    return name;
}

void
BrowserFontMetricsForPixelSize(
    int pixelSize,
    int *charWidthPtr,
    int *ascentPtr,
    int *descentPtr)
{
    int charWidth;
    int ascent;
    int descent;

    if (pixelSize <= 0) {
	pixelSize = 14;
    }
    charWidth = (pixelSize * 3 + 2) / 5;
    ascent = (pixelSize * 4 + 2) / 5;
    descent = pixelSize - ascent;
    if (charWidth <= 0) {
	charWidth = 1;
    }
    if (ascent <= 0) {
	ascent = 1;
    }
    if (descent <= 0) {
	descent = 1;
    }
    if (charWidthPtr != NULL) {
	*charWidthPtr = charWidth;
    }
    if (ascentPtr != NULL) {
	*ascentPtr = ascent;
    }
    if (descentPtr != NULL) {
	*descentPtr = descent;
    }
}

void
BrowserFontMetricsFromName(
    const char *name,
    int *pixelSizePtr,
    int *charWidthPtr,
    int *ascentPtr,
    int *descentPtr)
{
    char *fontName = BrowserFontXLFD(name, 0);
    int pixelSize = BrowserFontPixelSize(fontName);

    BrowserFontMetricsForPixelSize(pixelSize, charWidthPtr, ascentPtr,
	    descentPtr);
    if (pixelSizePtr != NULL) {
	*pixelSizePtr = pixelSize;
    }
    ckfree(fontName);
}

int
BrowserFontUnderlinePosition(
    const XFontStruct *fontPtr)
{
    int descent = (fontPtr != NULL) ? fontPtr->descent : 0;
    int position = descent / 2;

    return (position > 0) ? position : 1;
}

int
BrowserFontUnderlineThickness(
    const XFontStruct *fontPtr)
{
    int width = BrowserFontCharWidth(fontPtr);
    int thickness = width / 3;

    return (thickness > 0) ? thickness : 1;
}

void
BrowserSetFontProperty(
    XFontStruct *fontPtr,
    int *indexPtr,
    Atom name,
    unsigned long value)
{
    fontPtr->properties[*indexPtr].name = name;
    fontPtr->properties[*indexPtr].card32 = value;
    (*indexPtr)++;
}

void
BrowserSetFontSetExtents(
    XFontSetExtents *extentsPtr,
    int charWidth,
    int ascent,
    int descent)
{
    unsigned int height = (unsigned int)(ascent + descent);

    memset(extentsPtr, 0, sizeof(*extentsPtr));
    extentsPtr->max_ink_extent.x = 0;
    extentsPtr->max_ink_extent.y = (short)-ascent;
    extentsPtr->max_ink_extent.width = (unsigned short)charWidth;
    extentsPtr->max_ink_extent.height = (unsigned short)height;
    extentsPtr->max_logical_extent.x = 0;
    extentsPtr->max_logical_extent.y = (short)-ascent;
    extentsPtr->max_logical_extent.width = (unsigned short)charWidth;
    extentsPtr->max_logical_extent.height = (unsigned short)height;
}

char *
BrowserUtf8FromLatin1String(
    const char *string,
    int count,
    int *lengthPtr)
{
    char *utf8;
    int out = 0;
    int i;

    if (lengthPtr != NULL) {
	*lengthPtr = 0;
    }
    if ((string == NULL) || (count <= 0)) {
	return NULL;
    }
    utf8 = (char *)ckalloc((unsigned int)(count * 2 + 1));
    for (i = 0; i < count; i++) {
	unsigned int codepoint = (unsigned char)string[i];
	int written = BrowserUtf8FromCodepoint(codepoint, utf8 + out,
		count * 2 - out);

	if (written <= 0) {
	    continue;
	}
	out += written;
    }
    utf8[out] = '\0';
    if (lengthPtr != NULL) {
	*lengthPtr = out;
    }
    return utf8;
}

char *
BrowserUtf8FromXChar2bString(
    const XChar2b *string,
    int count,
    int *lengthPtr)
{
    char *utf8;
    int out = 0;
    int i;

    if (lengthPtr != NULL) {
	*lengthPtr = 0;
    }
    if ((string == NULL) || (count <= 0)) {
	return NULL;
    }
    utf8 = (char *)ckalloc((unsigned int)(count * 4 + 1));
    for (i = 0; i < count; i++) {
	unsigned int codepoint = ((unsigned int)string[i].byte1 << 8)
		| (unsigned int)string[i].byte2;
	int written = BrowserUtf8FromCodepoint(codepoint, utf8 + out,
		count * 4 - out);

	if (written <= 0) {
	    continue;
	}
	out += written;
    }
    utf8[out] = '\0';
    if (lengthPtr != NULL) {
	*lengthPtr = out;
    }
    return utf8;
}

char *
BrowserUtf8FromWideStringCount(
    const wchar_t *string,
    int count,
    int *lengthPtr)
{
    char *utf8;
    int length = 0;
    int out = 0;
    int i;

    if (lengthPtr != NULL) {
	*lengthPtr = 0;
    }
    if (string == NULL) {
	return BrowserDupString("");
    }
    if (count >= 0) {
	length = count;
    } else {
	while (string[length] != 0) {
	    length++;
	}
    }
    if (length <= 0) {
	return BrowserDupString("");
    }
    utf8 = (char *)ckalloc((unsigned int)(length * 4 + 1));
    for (i = 0; i < length; i++) {
	unsigned int codepoint = (unsigned int)string[i];
	int written;

	if (codepoint > 0x10ffffU) {
	    codepoint = 0xfffdU;
	}
	written = BrowserUtf8FromCodepoint(codepoint, utf8 + out,
		length * 4 - out);
	if (written <= 0) {
	    written = BrowserUtf8FromCodepoint(0xfffdU, utf8 + out,
		    length * 4 - out);
	}
	out += written;
    }
    utf8[out] = '\0';
    if (lengthPtr != NULL) {
	*lengthPtr = out;
    }
    return utf8;
}

char *
BrowserUtf8FromWideString(
    const wchar_t *string)
{
    return BrowserUtf8FromWideStringCount(string, -1, NULL);
}

unsigned int
BrowserUtf8DecodeOne(
    const unsigned char **cursorPtr)
{
    const unsigned char *cursor = *cursorPtr;
    unsigned int b0 = cursor[0];

    if (b0 == '\0') {
	return 0;
    }
    if (b0 < 0x80U) {
	*cursorPtr = cursor + 1;
	return b0;
    }
    if (((b0 & 0xe0U) == 0xc0U) && (cursor[1] != '\0')
	    && ((cursor[1] & 0xc0U) == 0x80U)) {
	unsigned int codepoint = ((b0 & 0x1fU) << 6)
		| (cursor[1] & 0x3fU);

	*cursorPtr = cursor + 2;
	return (codepoint >= 0x80U) ? codepoint : 0xfffdU;
    }
    if (((b0 & 0xf0U) == 0xe0U) && (cursor[1] != '\0')
	    && (cursor[2] != '\0')
	    && ((cursor[1] & 0xc0U) == 0x80U)
	    && ((cursor[2] & 0xc0U) == 0x80U)) {
	unsigned int codepoint = ((b0 & 0x0fU) << 12)
		| ((cursor[1] & 0x3fU) << 6)
		| (cursor[2] & 0x3fU);

	*cursorPtr = cursor + 3;
	return (codepoint >= 0x800U) ? codepoint : 0xfffdU;
    }
    if (((b0 & 0xf8U) == 0xf0U) && (cursor[1] != '\0')
	    && (cursor[2] != '\0') && (cursor[3] != '\0')
	    && ((cursor[1] & 0xc0U) == 0x80U)
	    && ((cursor[2] & 0xc0U) == 0x80U)
	    && ((cursor[3] & 0xc0U) == 0x80U)) {
	unsigned int codepoint = ((b0 & 0x07U) << 18)
		| ((cursor[1] & 0x3fU) << 12)
		| ((cursor[2] & 0x3fU) << 6)
		| (cursor[3] & 0x3fU);

	*cursorPtr = cursor + 4;
	return ((codepoint >= 0x10000U) && (codepoint <= 0x10ffffU))
		? codepoint : 0xfffdU;
    }
    *cursorPtr = cursor + 1;
    return 0xfffdU;
}

wchar_t *
BrowserWideStringFromUtf8(
    const char *utf8)
{
    const unsigned char *cursor = (const unsigned char *)utf8;
    size_t capacity;
    wchar_t *wide;
    size_t out = 0;

    if (utf8 == NULL) {
	utf8 = "";
	cursor = (const unsigned char *)utf8;
    }
    capacity = strlen(utf8) + 1U;
    wide = (wchar_t *)ckalloc(sizeof(wchar_t) * capacity);
    while (*cursor != '\0') {
	unsigned int codepoint = BrowserUtf8DecodeOne(&cursor);

	if ((sizeof(wchar_t) < 4U) && (codepoint > 0xffffU)) {
	    codepoint = 0xfffdU;
	}
	wide[out++] = (wchar_t)codepoint;
    }
    wide[out] = 0;
    return wide;
}
