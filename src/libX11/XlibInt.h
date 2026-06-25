/*
 * XlibInt.h --
 *
 *	Internal declarations shared by the x11.wasm libX11 compatibility
 *	layer translation units.
 */

#ifndef X11WASM_LIBX11_XLIBINT_H
#define X11WASM_LIBX11_XLIBINT_H

#include "X11Wasm/Browser.h"

#include <ctype.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/extensions/scrnsaver.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#ifndef c_class
#define c_class class
#endif
#ifndef UCHAR
#define UCHAR(c) ((unsigned char)(c))
#endif

#define X11_WASM_EVENT_QUEUE_LIMIT 256
#define X11_WASM_UNICODE_KEYSYM_BASE 0x01000000U
#define X11_WASM_COMPOSITION_START 1
#define X11_WASM_COMPOSITION_UPDATE 2
#define X11_WASM_COMPOSITION_END 3
#define X11_WASM_IME_CONTROL_STATE	((unsigned int)0x80000000U)
#define X11_WASM_MIN_KEYCODE 8
#define X11_WASM_MAX_KEYCODE 255
#define X11_WASM_SYNTHETIC_KEYCODE_START 128
#define X11_WASM_KEYSYMS_PER_KEYCODE 2
#define X11_WASM_DEFAULT_SCREEN_WIDTH 1024
#define X11_WASM_DEFAULT_SCREEN_HEIGHT 768
#define X11_WASM_KEYCODE_SHIFT 16
#define X11_WASM_KEYCODE_CONTROL 17
#define X11_WASM_KEYCODE_ALT 18
#define X11_WASM_KEYCODE_CAPS_LOCK 20
#define X11_WASM_KEYCODE_META 91
#define X11_WASM_KEYCODE_META_RIGHT 92
#define X11_WASM_KEYCODE_NUM_LOCK 144
#define X11_WASM_KEYCODE_SHIFT_RIGHT 161
#define X11_WASM_KEYCODE_CONTROL_RIGHT 163
#define X11_WASM_KEYCODE_ALT_RIGHT 165
#define X11_WASM_KEYCODE_SEMICOLON 186
#define X11_WASM_KEYCODE_EQUAL 187
#define X11_WASM_KEYCODE_COMMA 188
#define X11_WASM_KEYCODE_MINUS 189
#define X11_WASM_KEYCODE_PERIOD 190
#define X11_WASM_KEYCODE_SLASH 191
#define X11_WASM_KEYCODE_GRAVE 192
#define X11_WASM_KEYCODE_BRACKET_LEFT 219
#define X11_WASM_KEYCODE_BACKSLASH 220
#define X11_WASM_KEYCODE_BRACKET_RIGHT 221
#define X11_WASM_KEYCODE_APOSTROPHE 222
#define X11_WASM_KEYCODE_MODE_SWITCH 225
#define X11_WASM_NESTED_LIST_MARKER 0x544b4e4cU
#define X11_WASM_XREQ_CREATE_WINDOW 1
#define X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES 2
#define X11_WASM_XREQ_GET_WINDOW_ATTRIBUTES 3
#define X11_WASM_XREQ_DESTROY_WINDOW 4
#define X11_WASM_XREQ_DESTROY_SUBWINDOWS 5
#define X11_WASM_XREQ_REPARENT_WINDOW 7
#define X11_WASM_XREQ_MAP_WINDOW 8
#define X11_WASM_XREQ_MAP_SUBWINDOWS 9
#define X11_WASM_XREQ_UNMAP_WINDOW 10
#define X11_WASM_XREQ_UNMAP_SUBWINDOWS 11
#define X11_WASM_XREQ_CONFIGURE_WINDOW 12
#define X11_WASM_XREQ_CIRCULATE_WINDOW 13
#define X11_WASM_XREQ_GET_GEOMETRY 14
#define X11_WASM_XREQ_QUERY_TREE 15
#define X11_WASM_XREQ_GET_ATOM_NAME 17
#define X11_WASM_XREQ_CHANGE_PROPERTY 18
#define X11_WASM_XREQ_DELETE_PROPERTY 19
#define X11_WASM_XREQ_GET_PROPERTY 20
#define X11_WASM_XREQ_LIST_PROPERTIES 21
#define X11_WASM_XREQ_SET_SELECTION_OWNER 22
#define X11_WASM_XREQ_GET_SELECTION_OWNER 23
#define X11_WASM_XREQ_CONVERT_SELECTION 24
#define X11_WASM_XREQ_SEND_EVENT 25
#define X11_WASM_XREQ_GRAB_POINTER 26
#define X11_WASM_XREQ_UNGRAB_POINTER 27
#define X11_WASM_XREQ_GRAB_KEYBOARD 31
#define X11_WASM_XREQ_UNGRAB_KEYBOARD 32
#define X11_WASM_XREQ_QUERY_POINTER 38
#define X11_WASM_XREQ_TRANSLATE_COORDINATES 40
#define X11_WASM_XREQ_WARP_POINTER 41
#define X11_WASM_XREQ_SET_INPUT_FOCUS 42
#define X11_WASM_XREQ_CLOSE_FONT 46
#define X11_WASM_XREQ_QUERY_TEXT_EXTENTS 48
#define X11_WASM_XREQ_CREATE_PIXMAP 53
#define X11_WASM_XREQ_FREE_PIXMAP 54
#define X11_WASM_XREQ_CREATE_GC 55
#define X11_WASM_XREQ_CHANGE_GC 56
#define X11_WASM_XREQ_COPY_GC 57
#define X11_WASM_XREQ_SET_DASHES 58
#define X11_WASM_XREQ_SET_CLIP_RECTANGLES 59
#define X11_WASM_XREQ_FREE_GC 60
#define X11_WASM_XREQ_CLEAR_AREA 61
#define X11_WASM_XREQ_COPY_AREA 62
#define X11_WASM_XREQ_COPY_PLANE 63
#define X11_WASM_XREQ_POLY_POINT 64
#define X11_WASM_XREQ_POLY_LINE 65
#define X11_WASM_XREQ_POLY_SEGMENT 66
#define X11_WASM_XREQ_POLY_RECTANGLE 67
#define X11_WASM_XREQ_POLY_ARC 68
#define X11_WASM_XREQ_FILL_POLY 69
#define X11_WASM_XREQ_POLY_FILL_RECTANGLE 70
#define X11_WASM_XREQ_POLY_FILL_ARC 71
#define X11_WASM_XREQ_PUT_IMAGE 72
#define X11_WASM_XREQ_GET_IMAGE 73
#define X11_WASM_XREQ_POLY_TEXT_8 74
#define X11_WASM_XREQ_POLY_TEXT_16 75
#define X11_WASM_XREQ_CREATE_COLORMAP 78
#define X11_WASM_XREQ_FREE_COLORMAP 79
#define X11_WASM_XREQ_ALLOC_COLOR 84
#define X11_WASM_XREQ_ALLOC_NAMED_COLOR 85
#define X11_WASM_XREQ_FREE_COLORS 88
#define X11_WASM_XREQ_QUERY_COLORS 91
#define X11_WASM_XREQ_LOOKUP_COLOR 92
#define X11_WASM_XREQ_CREATE_CURSOR 93
#define X11_WASM_XREQ_CREATE_GLYPH_CURSOR 94
#define X11_WASM_XREQ_FREE_CURSOR 95
#define X11_WASM_XREQ_GET_KEYBOARD_MAPPING 101
#define X11_WASM_XREQ_BELL 104
#define X11_WASM_XREQ_SET_SCREEN_SAVER 107
#define X11_WASM_XREQ_GET_SCREEN_SAVER 108
#define X11_WASM_XREQ_CHANGE_HOSTS 109
#define X11_WASM_XREQ_LIST_HOSTS 110
#define X11_WASM_XREQ_SET_ACCESS_CONTROL 111
#define X11_WASM_XREQ_FORCE_SCREEN_SAVER 115
#define X11_WASM_EXTENSION_NAME "X11-WASM"
#define X11_WASM_EXTENSION_MAJOR_OPCODE 128
#define X11_WASM_SCREENSAVER_MAJOR_OPCODE 129
#define X11_WASM_INPUT_ONLY_WINDOW_ATTR_MASK \
	(CWWinGravity | CWEventMask | CWDontPropagate | CWOverrideRedirect \
	| CWCursor)
#define X11_WASM_WINDOW_ATTR_MASK \
	(CWBackPixmap | CWBackPixel | CWBorderPixmap | CWBorderPixel \
	| CWBitGravity | CWWinGravity | CWBackingStore | CWBackingPlanes \
	| CWBackingPixel | CWOverrideRedirect | CWSaveUnder | CWEventMask \
	| CWDontPropagate | CWColormap | CWCursor)
#define X11_WASM_WINDOW_EVENT_MASK \
	(KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask \
	| EnterWindowMask | LeaveWindowMask | PointerMotionMask \
	| PointerMotionHintMask | Button1MotionMask | Button2MotionMask \
	| Button3MotionMask | Button4MotionMask | Button5MotionMask \
	| ButtonMotionMask | KeymapStateMask | ExposureMask \
	| VisibilityChangeMask | StructureNotifyMask | ResizeRedirectMask \
	| SubstructureNotifyMask | SubstructureRedirectMask | FocusChangeMask \
	| PropertyChangeMask | ColormapChangeMask | OwnerGrabButtonMask)
#define X11_WASM_DO_NOT_PROPAGATE_EVENT_MASK \
	(KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask \
	| PointerMotionMask | Button1MotionMask | Button2MotionMask \
	| Button3MotionMask | Button4MotionMask | Button5MotionMask \
	| ButtonMotionMask)

typedef struct X11WasmQueuedEvent {
    XEvent event;
    int internal;
} X11WasmQueuedEvent;

typedef struct X11WasmAtomName {
    Atom atom;
    char *name;
    struct X11WasmAtomName *nextPtr;
} X11WasmAtomName;

typedef struct X11WasmKeySymName {
    const char *name;
    KeySym keySym;
} X11WasmKeySymName;

typedef struct X11WasmNamedColor {
    const char *name;
    unsigned short red;
    unsigned short green;
    unsigned short blue;
} X11WasmNamedColor;

typedef struct X11WasmExtensionInfo {
    const char *name;
    int majorOpcode;
    int firstEvent;
    int firstError;
} X11WasmExtensionInfo;

typedef struct BrowserPolygonIntersection {
    double x;
    int winding;
} BrowserPolygonIntersection;

struct X11WasmProperty {
    Atom atom;
    Atom type;
    int format;
    unsigned long nitems;
    unsigned char *data;
    int dataLength;
    X11WasmProperty *nextPtr;
};

struct X11WasmColorEntry {
    unsigned long pixel;
    unsigned short red;
    unsigned short green;
    unsigned short blue;
    unsigned char flags;
    X11WasmColorEntry *nextPtr;
};

typedef struct X11WasmSelectionOwner {
    Atom selection;
    Window owner;
    Time time;
    struct X11WasmSelectionOwner *nextPtr;
} X11WasmSelectionOwner;

typedef struct X11WasmGCClip {
    GC gc;
    int enabled;
    XRectangle rect;
    int rectCount;
    XRectangle *rects;
    struct X11WasmGCClip *nextPtr;
} X11WasmGCClip;

typedef struct X11WasmGCDashes {
    GC gc;
    int count;
    unsigned char *pattern;
    struct X11WasmGCDashes *nextPtr;
} X11WasmGCDashes;

typedef struct X11WasmTrackedGC {
    GC gc;
    unsigned int depth;
    struct X11WasmTrackedGC *nextPtr;
} X11WasmTrackedGC;

typedef struct X11WasmIMInstantiateCallback {
    Display *display;
    XIDProc callback;
    XPointer clientData;
    struct X11WasmIMInstantiateCallback *nextPtr;
} X11WasmIMInstantiateCallback;

typedef struct X11WasmICAttributeSet {
    int hasArea;
    XRectangle area;
    int hasAreaNeeded;
    XRectangle areaNeeded;
    int hasSpotLocation;
    XPoint spotLocation;
    int hasForeground;
    unsigned long foreground;
    int hasBackground;
    unsigned long background;
    int hasFontSet;
    XFontSet fontSet;
    int hasLineSpace;
    int lineSpace;
    int hasCursor;
    Cursor cursor;
} X11WasmICAttributeSet;

typedef struct X11WasmNestedList {
    unsigned int marker;
    X11WasmICAttributeSet values;
    XRectangle *areaReturnPtr;
    XRectangle *areaNeededReturnPtr;
    XPoint *spotLocationReturnPtr;
    unsigned long *foregroundReturnPtr;
    unsigned long *backgroundReturnPtr;
    XFontSet *fontSetReturnPtr;
    int *lineSpaceReturnPtr;
    Cursor *cursorReturnPtr;
} X11WasmNestedList;

struct _XIM {
    Display *display;
    unsigned short styleCount;
    XIMStyle styles[2];
    int hasDestroyCallback;
    XIMCallback destroyCallback;
};

struct _XIC {
    XIM inputMethod;
    Window clientWindow;
    Window focusWindow;
    XIMStyle inputStyle;
    long filterEvents;
    X11WasmICAttributeSet preedit;
    X11WasmICAttributeSet status;
    int focused;
};

struct _XOC {
    Display *display;
    char *baseFontNameList;
    char *localeName;
    char *fontName;
    XFontStruct *fontStruct;
    int fontCount;
    XFontStruct **fontStructList;
    char **fontNameList;
    XFontSetExtents extents;
};

extern X11WasmDisplayState browserDisplay;
extern int browserDisplayInitialized;
extern X11WasmAtomName *atomList;
extern X11WasmSelectionOwner *selectionOwnerList;
extern X11WasmGCClip *gcClipList;
extern X11WasmGCDashes *gcDashList;
extern X11WasmTrackedGC *gcList;
extern X11WasmIMInstantiateCallback *imInstantiateCallbackList;
extern XIC focusedInputContext;
extern char *localeModifiers;
extern char *externalClipboardText;
extern int externalClipboardAvailable;
extern Time externalClipboardTime;
extern Atom nextAtom;
extern XErrorHandler errorHandler;
extern XIOErrorHandler ioErrorHandler;
extern X11WasmQueuedEvent eventQueue[X11_WASM_EVENT_QUEUE_LIMIT];
extern int eventQueueHead;
extern int eventQueueCount;
extern unsigned long eventSerial;
extern unsigned long damageSerial;
extern Time eventTime;
extern int browserTestXErrorCount;
extern int browserTestXIOErrorCount;
extern int browserTestIMInstantiateCount;
extern Display *browserTestIMInstantiateDisplay;
extern XPointer browserTestIMInstantiateClientData;
extern XPointer browserTestIMInstantiateCallData;
extern int browserTestIMDestroyCount;
extern XIM browserTestIMDestroyMethod;
extern XPointer browserTestIMDestroyClientData;
extern XPointer browserTestIMDestroyCallData;
extern unsigned char browserTestLastErrorCode;
extern unsigned char browserTestLastRequestCode;
extern XID browserTestLastResourceId;
extern unsigned long browserTestLastSerial;
extern unsigned int browserTestRequestCodeSum;
extern unsigned long long browserTestRequestCodeMask;

extern const X11WasmKeySymName browserKeySymNames[];
extern const size_t browserKeySymNameCount;
extern const X11WasmExtensionInfo browserExtensions[];
extern const size_t browserExtensionCount;
extern const X11WasmNamedColor browserNamedColors[];
extern const size_t browserNamedColorCount;
extern const char *const browserFontFallbackFamilies[];

int X11WasmQueuePointerEvent(
    int type,
    unsigned int window,
    unsigned int button,
    unsigned int state,
    int x,
    int y,
    unsigned int time);
int X11WasmQueueWheelEvent(
    unsigned int window,
    int delta,
    unsigned int state,
    int x,
    int y,
    unsigned int time);
void BrowserAppendPolygonSpan(
    XRectangle **rectsPtr,
    int *countPtr,
    double left,
    double right,
    int y);
void BrowserAppendRectangle(
    XRectangle **rectsPtr,
    int *countPtr,
    const XRectangle *rectPtr);
void BrowserApplyChildWinGravity(
    X11WasmResource *parentPtr,
    unsigned int oldWidth,
    unsigned int oldHeight);
void BrowserApplyGCClip(
    GC gc);
void BrowserApplyGCStyle(
    GC gc);
void BrowserApplyGCValues(
    GC gc,
    unsigned long mask,
    XGCValues *values);
char *BrowserApplyICAttributes(
    X11WasmICAttributeSet *attributesPtr,
    XVaNestedList list,
    char *name);
void BrowserApplyNetWmStateAtom(
    Window window,
    X11WasmResource *resourcePtr,
    Atom netWmStateAtom,
    long action,
    Atom stateAtom);
void BrowserApplyNetWmStateClientMessage(
    XClientMessageEvent *eventPtr);
unsigned long BrowserApplyRasterFunction(
    int function,
    unsigned long source,
    unsigned long dest,
    unsigned int depth);
void BrowserApplyWindowAttributes(
    X11WasmResource *resourcePtr,
    unsigned long mask,
    XSetWindowAttributes *atts);
void BrowserApplyWindowColormapWindowsProperty(
    Window window,
    X11WasmProperty *propertyPtr);
void BrowserApplyWindowIconProperty(
    Window window,
    Atom property,
    Atom type,
    int format,
    _Xconst unsigned char *data,
    int nelements);
void BrowserApplyWindowMetadataProperty(
    Window window,
    Atom property,
    X11WasmProperty *propertyPtr);
void BrowserApplyWindowOpacityProperty(
    Window window,
    Atom property,
    Atom type,
    int format,
    _Xconst unsigned char *data,
    int nelements);
void BrowserApplyWindowProtocolsProperty(
    Window window,
    X11WasmProperty *propertyPtr);
void BrowserApplyWindowStandardColormapProperty(
    Window window,
    Atom property,
    X11WasmProperty *propertyPtr);
void BrowserApplyWindowStateProperty(
    Window window,
    X11WasmProperty *propertyPtr);
unsigned int BrowserAsciiKeycodeFromKeySym(
    KeySym keySym);
int BrowserAtomIsValid(
    Atom atom);
int BrowserBadMatchOrError(
    Display **displayPtr,
    XID resource,
    unsigned char requestCode);
KeySym BrowserBaseAsciiKeySymForKeycode(
    unsigned int keycode);
int BrowserBitmapBytesPerLine(
    unsigned int width);
int BrowserBitmapGetPixel(
    X11WasmResource *resourcePtr,
    unsigned int x,
    unsigned int y);
void BrowserBitmapPutPixel(
    X11WasmResource *resourcePtr,
    unsigned int x,
    unsigned int y,
    int bit);
int BrowserCeilDoubleToInt(
    double value);
void BrowserClampPointerToGrabConfine(
    int *rootXPtr,
    int *rootYPtr);
unsigned short BrowserClampUShort(
    unsigned int value);
unsigned int BrowserClearAreaExtent(
    int origin,
    unsigned int requested,
    unsigned int limit);
void BrowserClearEventQueue(void);
void BrowserClearExternalClipboardText(void);
void BrowserClearHostAccessList(void);
void BrowserClearInputContext(
    void);
void BrowserClearKeycodeRange(
    unsigned int firstKeycode,
    unsigned int count);
void BrowserClearRectangle(
    XRectangle *rectPtr);
X11WasmResource *BrowserCirculateSubwindow(
    X11WasmResource *parentPtr,
    int direction,
    int *placePtr);
Atom BrowserClipboardSelectionAtom(void);
int BrowserCollectVisualInfos(
    Display *display,
    long mask,
    const XVisualInfo *templatePtr,
    XVisualInfo *infoList);
int BrowserColormapOrError(
    Display **displayPtr,
    Colormap colormap,
    unsigned char requestCode,
    int allowNone);
X11WasmResource *BrowserColormapResource(
    Colormap colormap);
int BrowserCompleteRequest(
    Display *display,
    int flush);
const char *BrowserCompositionEventName(
    int phase);
Atom BrowserCompoundTextAtom(void);
int BrowserConvertExternalClipboardSelection(
    Window requestor,
    Atom selection,
    Atom target,
    Atom property,
    Time time);
int BrowserConvertUtf8ListToEncoding(
    char **list,
    int count,
    Atom encoding,
    char ***convertedListPtr);
XImage *BrowserConvertXYBitmapForPut(
    X11WasmResource *resourcePtr,
    GC gc,
    XImage *image,
    int srcX,
    int srcY,
    unsigned int width,
    unsigned int height);
int BrowserCopyDestinationRectangle(
    X11WasmResource *sourcePtr,
    X11WasmResource *targetPtr,
    int srcX,
    int srcY,
    unsigned int width,
    unsigned int height,
    int destX,
    int destY,
    XRectangle *rectPtr);
void BrowserCopyGCComponents(
    GC src,
    GC dest,
    unsigned long mask);
void BrowserCopyGCValueFields(
    GC src,
    XGCValues *dest,
    unsigned long mask);
XRectangle *BrowserCopyRectangleList(
    const XRectangle *rects,
    int count,
    int *countPtr,
    XRectangle *extentsPtr);
void BrowserCopyString(
    char *buffer,
    int length,
    const char *string);
int BrowserCopyTargetContains(
    GC gc,
    int x,
    int y);
int BrowserCopyTargetRectangle(
    X11WasmResource *targetPtr,
    unsigned int width,
    unsigned int height,
    int destX,
    int destY,
    XRectangle *rectPtr);
int BrowserCreateGlyphCursorImage(
    X11WasmResource *cursorPtr,
    unsigned int sourceChar,
    unsigned int maskChar,
    XColor _Xconst *foreground,
    XColor _Xconst *background);
int BrowserCreatePixmapCursorImage(
    X11WasmResource *cursorPtr,
    Pixmap source,
    Pixmap mask,
    XColor *foreground,
    XColor *background,
    unsigned int hotX,
    unsigned int hotY);
X11WasmProperty *BrowserCreateProperty(
    X11WasmResource *resourcePtr,
    Atom atom);
X11WasmSelectionOwner *BrowserCreateSelectionOwner(
    Atom selection);
unsigned char BrowserCursorColorByte(
    unsigned short value);
int BrowserCursorColorIsDefaultBlack(
    const XColor *colorPtr);
int BrowserCursorColorIsDefaultWhite(
    const XColor *colorPtr);
int BrowserCursorFontResource(
    X11WasmResource *resourcePtr);
X11WasmResource *BrowserCursorResourceOrError(
    Display **displayPtr,
    Cursor cursor,
    unsigned char requestCode);
unsigned int BrowserCursorShape(
    Cursor cursor);
void BrowserDStringAppendHexByte(
    X11WasmDString *dsPtr,
    unsigned char byte);
int BrowserDefaultErrorHandler(
    Display *display,
    XErrorEvent *eventPtr);
int BrowserDefaultIOErrorHandler(
    Display *display);
void BrowserDeleteGCClip(
    GC gc);
void BrowserDeleteGCDashes(
    GC gc);
int BrowserDeleteProperty(
    X11WasmResource *resourcePtr,
    Atom atom);
void BrowserDeleteSelectionOwner(
    Atom selection);
void BrowserDeleteSelectionOwnersForWindow(
    Window window);
unsigned long BrowserDepthPlaneMask(
    unsigned int depth);
int BrowserDestroyWindowResource(
    X11WasmResource *resourcePtr);
int BrowserDisplayNameIsAccepted(
    const char *displayName);
void BrowserDrawUtf8ImageText(
    Display *display,
    Drawable drawable,
    XFontSet fontSet,
    GC gc,
    int x,
    int y,
    const char *utf8,
    int length);
int BrowserDrawUtf8Text(
    Display *display,
    Drawable drawable,
    GC gc,
    int x,
    int y,
    const char *utf8,
    int length,
    unsigned char requestCode);
int BrowserDrawableDepthsMatchOrError(
    Display **displayPtr,
    X11WasmResource *sourcePtr,
    X11WasmResource *targetPtr,
    Drawable target,
    unsigned char requestCode);
X11WasmResource *BrowserDrawableResourceOrError(
    Display **displayPtr,
    Drawable drawable,
    unsigned char requestCode);
char *BrowserDupCountedString(
    const unsigned char *string,
    int maxLength);
char *BrowserDupString(
    const char *string);
int BrowserEnsureBitmapStorage(
    X11WasmResource *resourcePtr);
int BrowserEnsurePixmapBacking(
    X11WasmResource *resourcePtr);
int BrowserErrorCodeForName(
    const char *name,
    int *codePtr);
const char *BrowserErrorText(
    int code);
long BrowserEventMask(
    const XEvent *eventPtr);
int BrowserEventMatchesFocusedIC(
    XEvent *eventPtr,
    Window window);
int BrowserEventMatchesWindowMask(
    const XEvent *eventPtr,
    Window window,
    long mask);
Window BrowserEventWindow(
    const XEvent *eventPtr);
int BrowserExternalClipboardTextProperty(
    Atom target,
    XTextProperty *textPropPtr);
void BrowserFailQueuedSelectionRequestsForOwner(
    Window owner);
Status BrowserFetchTextPropertyString(
    Display *display,
    Window window,
    Atom property,
    char **stringReturn);
void BrowserFillVisualInfo(
    XVisualInfo *infoPtr,
    Visual *visualPtr,
    int screen,
    int depth);
int BrowserFilterEventForIM(
    XEvent *eventPtr,
    Window window);
const X11WasmExtensionInfo *BrowserFindExtension(
    const char *name);
X11WasmGCClip *BrowserFindGCClip(
    GC gc,
    int create);
X11WasmGCDashes *BrowserFindGCDashes(
    GC gc,
    int create);
int BrowserFindHostAddress(
    const XHostAddress *hostPtr);
X11WasmProperty *BrowserFindProperty(
    X11WasmResource *resourcePtr,
    Atom atom);
X11WasmSelectionOwner *BrowserFindSelectionOwner(
    Atom selection);
X11WasmResource *BrowserFindTopVisibleChildAt(
    X11WasmResource *parentPtr,
    int x,
    int y);
char *BrowserFirstFontName(
    const char *nameList);
int BrowserFlushToJs(
    Display *display);
int BrowserFontCharWidth(
    const XFontStruct *fontPtr);
char *BrowserFontFamilyFromName(
    const char *name);
int BrowserFontField(
    const char *name,
    int field,
    const char **startPtr,
    size_t *lengthPtr);
int BrowserFontFieldInt(
    const char *name,
    int field);
void BrowserFontMetricsForPixelSize(
    int pixelSize,
    int *charWidthPtr,
    int *ascentPtr,
    int *descentPtr);
void BrowserFontMetricsFromName(
    const char *name,
    int *pixelSizePtr,
    int *charWidthPtr,
    int *ascentPtr,
    int *descentPtr);
int BrowserFontNameListCount(
    const char *nameList);
int BrowserFontPatternHasWildcards(
    const char *pattern,
    size_t patternLength);
int BrowserFontPatternIsWildcard(
    const char *pattern,
    size_t patternLength);
int BrowserFontPatternMatches(
    const char *pattern,
    size_t patternLength,
    const char *string);
int BrowserFontPixelSize(
    const char *name);
int BrowserFontPointSize(
    const char *name,
    int pixelSize);
X11WasmResource *BrowserFontResourceOrError(
    Display **displayPtr,
    Font font,
    unsigned char requestCode);
int BrowserFontResourceTextWidth(
    X11WasmResource *resourcePtr,
    int fallbackCharWidth,
    const char *utf8,
    int length);
void BrowserFontSetMetrics(
    XFontSet fontSet,
    int *ascentPtr,
    int *descentPtr);
int BrowserFontSetTextEscapement(
    XFontSet fontSet,
    const char *utf8,
    int byteCount);
int BrowserFontSetTextExtents(
    XFontSet fontSet,
    const char *utf8,
    int byteCount,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn);
Status BrowserFontSetTextPerCharExtents(
    XFontSet fontSet,
    const char *utf8,
    int byteCount,
    XRectangle *inkExtentsBuffer,
    XRectangle *logicalExtentsBuffer,
    int bufferSize,
    int *numChars,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn);
int BrowserFontTextWidth(
    const XFontStruct *fontPtr,
    const char *utf8,
    int length);
int BrowserFontUnderlinePosition(
    const XFontStruct *fontPtr);
int BrowserFontUnderlineThickness(
    const XFontStruct *fontPtr);
char *BrowserFontXLFD(
    const char *name,
    int scalable);
X11WasmResource *BrowserFontableResourceOrError(
    Display **displayPtr,
    XID fontable,
    unsigned char requestCode,
    int *validPtr);
void BrowserForgetColors(
    Colormap colormap,
    unsigned long *pixels,
    int count);
void BrowserForgetHostAddress(
    const XHostAddress *hostPtr);
void BrowserForgetIMInstantiateCallbacks(
    Display *display);
void BrowserFreeColorList(
    X11WasmResource *resourcePtr);
void BrowserFreePixmapBacking(
    X11WasmResource *resourcePtr);
void BrowserFreeProperty(
    X11WasmProperty *propertyPtr);
void BrowserFreePropertyList(
    X11WasmResource *resourcePtr);
void BrowserFreeTemporaryStringList(
    char **list,
    int count);
int BrowserGCChangeValueMaskIsValid(
    unsigned long mask);
int BrowserGCClipContains(
    GC gc,
    int x,
    int y);
int BrowserGCClipMaskContains(
    GC gc,
    int x,
    int y);
unsigned int BrowserGCDepth(
    GC gc);
int BrowserGCDepthsMatchOrError(
    Display **displayPtr,
    GC src,
    GC dest,
    unsigned char requestCode);
int BrowserGCGetValueMaskIsValid(
    unsigned long mask);
unsigned int BrowserGCLineWidth(
    GC gc);
GC BrowserGCOrError(
    Display **displayPtr,
    GC gc,
    unsigned char requestCode);
unsigned long BrowserGCPixel(
    GC gc);
int BrowserGCValueMaskOrError(
    Display **displayPtr,
    unsigned long mask,
    unsigned char requestCode);
int BrowserGetGCDashes(
    GC gc,
    const unsigned char **patternPtr,
    int *countPtr);
int BrowserGetImageDrawableMatches(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    Drawable drawable,
    int x,
    int y,
    unsigned int width,
    unsigned int height);
Window BrowserGetFocusWindow(void);
Window BrowserGetGrabWindow(void);
int BrowserGrabModeIsValid(
    int mode);
int BrowserHasCasePrefix(
    const char *string,
    const char *prefix);
int BrowserHexDigit(
    int ch);
Window BrowserHitTestRoot(
    int rootX,
    int rootY,
    int *xPtr,
    int *yPtr);
int BrowserHostAddressMatches(
    const X11WasmHostEntry *entryPtr,
    const XHostAddress *hostPtr);
int BrowserHostFamilyIsValid(
    int family);
int BrowserICAllowsLookup(
    XIC inputContext,
    XKeyPressedEvent *event);
int BrowserIMInstantiateCallbackExists(
    Display *display,
    XIDProc callback,
    XPointer clientData);
int BrowserIMSupportsStyle(
    XIM inputMethod,
    XIMStyle style);
int BrowserIconDimensionMatches(
    unsigned int value,
    int minValue,
    int maxValue,
    int increment);
int BrowserIconMatchesAnySizeHint(
    unsigned int width,
    unsigned int height,
    const XIconSize *hints,
    int hintCount,
    unsigned long long *penaltyPtr);
int BrowserIconMatchesSizeHint(
    unsigned int width,
    unsigned int height,
    const XIconSize *hintPtr,
    unsigned long long *penaltyPtr);
unsigned int BrowserIconPropertyCardinal(
    const unsigned char *data,
    int index);
int BrowserImageAddPixel(
    XImage *image,
    long value);
void BrowserImageApplyPlaneMask(
    XImage *image,
    unsigned long planeMask);
int BrowserImageBitsPerPixel(
    unsigned int depth);
int BrowserImageBytesPerLine(
    unsigned int width,
    int bitsPerPixel,
    int bitmapPad);
int BrowserImageDataLength(
    XImage *image);
int BrowserImageDestroy(
    XImage *image);
int BrowserImageFormatIsValid(
    int format);
unsigned long BrowserImageGetPixel(
    XImage *image,
    int x,
    int y);
int BrowserImagePutPixel(
    XImage *image,
    int x,
    int y,
    unsigned long pixel);
XImage *BrowserImageSubImage(
    XImage *image,
    int x,
    int y,
    unsigned int width,
    unsigned int height);
void BrowserInitDisplay(void);
void BrowserInitGC(
    Display *display,
    GC gc);
void BrowserInitImageFuncs(
    XImage *image);
void BrowserInitKeyboardMap(
    X11WasmDisplayState *state);
void BrowserInitModifierMap(
    X11WasmDisplayState *state);
int BrowserInitPixmapBackingImage(
    X11WasmResource *resourcePtr,
    XImage *imagePtr);
int BrowserInitialWmStateForMap(
    Display *display,
    Window window);
Window BrowserInputContextWindow(
    XIC inputContext);
int BrowserIntersectRectangles(
    const XRectangle *aPtr,
    const XRectangle *bPtr,
    XRectangle *destPtr);
int BrowserInvokeAfterFunction(
    Display *display);
int BrowserIsAlgorithmicUnicodeCodepoint(
    unsigned int codepoint);
int BrowserIsKnownGC(
    GC gc);
int BrowserIsSyntheticKeyEvent(
    XKeyPressedEvent *event);
int BrowserIsUnicodeKeySym(
    KeySym keySym);
unsigned int BrowserJsXId(
    XID xid);
KeySym BrowserKeySymForKeycodeLevel(
    unsigned int keycode,
    int index);
KeySym BrowserKeySymFromCodepoint(
    unsigned int codepoint);
unsigned int BrowserKeycodeFromKeySym(
    KeySym keySym);
KeySym BrowserKnownKeySymForKeycode(
    unsigned int keycode);
unsigned int BrowserKnownKeycodeForKeySym(
    KeySym keySym);
int BrowserLatin1FromKeySym(
    KeySym keySym,
    unsigned char *bytePtr);
void BrowserLinkChildAfter(
    X11WasmResource *resourcePtr,
    X11WasmResource *parentPtr,
    X11WasmResource *siblingPtr);
void BrowserLinkChildAtBottom(
    X11WasmResource *resourcePtr,
    Window parent);
void BrowserLinkChildAtTop(
    X11WasmResource *resourcePtr,
    Window parent);
void BrowserLinkChildBefore(
    X11WasmResource *resourcePtr,
    X11WasmResource *parentPtr,
    X11WasmResource *siblingPtr);
int BrowserLocaleModifiersDisableIM(void);
int BrowserLookupColorEntry(
    Colormap colormap,
    unsigned long pixel,
    XColor *colorPtr);
Status BrowserLookupStatus(
    int length,
    KeySym keySym);
int BrowserLookupUtf8String(
    XKeyPressedEvent *event,
    char *buffer,
    int bytes,
    KeySym *keySymReturn,
    Status *statusReturn);
void BrowserMarkResourceDamage(
    X11WasmResource *resourcePtr,
    int x,
    int y,
    unsigned int width,
    unsigned int height);
X11WasmNestedList *BrowserNestedListFromXVa(
    XVaNestedList list);
Atom BrowserNetWmIconAtom(void);
Atom BrowserNetWmStateAtom(void);
char *BrowserNextFontName(
    const char **cursorPtr);
unsigned int BrowserNextSyntheticKeycode(void);
int BrowserNormalizeCopyArea(
    X11WasmResource *sourcePtr,
    X11WasmResource *targetPtr,
    int *srcXPtr,
    int *srcYPtr,
    unsigned int *widthPtr,
    unsigned int *heightPtr,
    int *destXPtr,
    int *destYPtr);
unsigned int BrowserNormalizeKeycode(
    unsigned int keycode,
    KeySym keySym);
void BrowserNotifyIMInstantiateCallbacks(void);
void BrowserNotifyProperty(
    X11WasmResource *resourcePtr,
    Atom atom,
    int state);
void BrowserNotifyVisibilityTree(
    X11WasmResource *resourcePtr,
    int state);
void BrowserNotifyWindowColormap(
    X11WasmResource *resourcePtr,
    Colormap oldColormap);
int BrowserOptionalGCOrError(
    Display **displayPtr,
    GC gc,
    unsigned char requestCode);
int BrowserOptionalPropertyAtomOrError(
    Display **displayPtr,
    Atom atom,
    unsigned char requestCode);
int BrowserParseColor(
    const char *name,
    XColor *colorPtr);
int BrowserParseHexComponent(
    const char *string,
    size_t digits,
    unsigned int *valuePtr);
int BrowserParseKeySymHex(
    const char *string,
    size_t minDigits,
    size_t maxDigits,
    unsigned long *valuePtr);
int BrowserParseRgbHexSpec(
    const char *string,
    unsigned int *redPtr,
    unsigned int *greenPtr,
    unsigned int *bluePtr);
int BrowserParseRgbiComponent(
    const char **cursorPtr,
    unsigned int *valuePtr);
int BrowserParseRgbiSpec(
    const char *string,
    unsigned int *redPtr,
    unsigned int *greenPtr,
    unsigned int *bluePtr);
X11WasmResource *BrowserPixmapResourceOrError(
    Display **displayPtr,
    Pixmap pixmap,
    unsigned char requestCode);
int BrowserPointerGrabAllowsEvent(
    const XEvent *eventPtr);
int BrowserPointerGrabEventMaskIsValid(
    unsigned int eventMask);
int BrowserPopEvent(
    XEvent *eventPtr,
    int *internalPtr);
int BrowserPopEventAfterPump(
    Display *display,
    XEvent *event);
int BrowserPopMatchingEvent(
    Window window,
    long mask,
    XEvent *eventPtr);
int BrowserPopMatchingEventAfterPump(
    Display *display,
    Window window,
    long mask,
    XEvent *event);
Atom BrowserPredefinedAtom(
    const char *name);
const char *BrowserPredefinedAtomName(
    Atom atom);
Atom BrowserPropertyAtomAt(
    const unsigned char *data,
    unsigned long index);
int BrowserPropertyAtomOrError(
    Display **displayPtr,
    Atom atom,
    unsigned char requestCode);
int BrowserPropertyByteLength(
    int format,
    int nelements);
int BrowserPropertyChangeOrError(
    Display **displayPtr,
    X11WasmProperty *propertyPtr,
    Atom property,
    Atom type,
    int format,
    int mode,
    unsigned char requestCode);
int BrowserPropertyElementBytes(
    int format);
int BrowserPropertyIsTextEncoding(
    Atom type);
int BrowserPropertyValueOrError(
    Display **displayPtr,
    XID resourceId,
    unsigned char requestCode);
int BrowserPutImageDrawableMatches(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    Drawable drawable,
    XImage *image);
int BrowserQueueColormapNotify(
    X11WasmResource *resourcePtr,
    Colormap colormap,
    Bool isNew,
    int state);
int BrowserQueueConfigureNotify(
    X11WasmResource *resourcePtr,
    Window above);
int BrowserQueueCirculateNotify(
    X11WasmResource *resourcePtr,
    int place);
int BrowserQueueCopyExposureEvents(
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
    unsigned char requestCode);
int BrowserQueueCreateNotify(
    X11WasmResource *resourcePtr);
int BrowserQueueDestroyNotify(
    X11WasmResource *resourcePtr);
int BrowserQueueEvent(
    XEvent *eventPtr);
int BrowserQueueEventInternal(
    XEvent *eventPtr,
    int internal);
int BrowserQueueEventWhenIdle(
    const XEvent *eventPtr);
int BrowserQueueExpose(
    X11WasmResource *resourcePtr,
    int x,
    int y,
    unsigned int width,
    unsigned int height);
int BrowserQueueGraphicsExposeEvent(
    Drawable drawable,
    const XRectangle *rectPtr,
    int count,
    unsigned char requestCode);
int BrowserQueueGravityNotify(
    X11WasmResource *resourcePtr);
int BrowserQueueMapNotify(
    X11WasmResource *resourcePtr);
int BrowserQueueKeyEventFields(
    int type,
    unsigned int window,
    unsigned int keycode,
    unsigned int keySym,
    unsigned int state,
    int x,
    int y,
    unsigned int time,
    int imeControl);
int BrowserQueueNoExposeEvent(
    Drawable drawable,
    unsigned char requestCode);
int BrowserQueueReparentNotify(
    X11WasmResource *resourcePtr,
    Window oldParent,
    Window newParent,
    int x,
    int y);
int BrowserQueueUnmapNotify(
    X11WasmResource *resourcePtr,
    Bool fromConfigure);
int BrowserQueueRawFocusEvent(
    Window window,
    int type,
    Time time);
int BrowserQueueSelectionClear(
    Window owner,
    Atom selection,
    Time time,
    unsigned long serial);
int BrowserQueueSelectionNotify(
    Window requestor,
    Atom selection,
    Atom target,
    Atom property,
    Time time);
int BrowserQueueSelectionRequest(
    Window owner,
    Window requestor,
    Atom selection,
    Atom target,
    Atom property,
    Time time);
int BrowserQueueVisibilityNotify(
    X11WasmResource *resourcePtr,
    int state);
int BrowserRawTextListToTextProperty(
    char **list,
    int count,
    Atom encoding,
    XTextProperty *textPropReturn);
char *BrowserReadICAttributes(
    X11WasmICAttributeSet *attributesPtr,
    XVaNestedList list,
    char *name);
int BrowserReadPixmapBacking(
    X11WasmResource *resourcePtr,
    XImage *destImage,
    int srcX,
    int srcY,
    unsigned int width,
    unsigned int height);
void BrowserRecordFocusChangeTime(
    Time time);
int BrowserRectangleCoveredByList(
    const XRectangle *rectPtr,
    const XRectangle *rects,
    int count);
int BrowserRectangleIsEmpty(
    const XRectangle *rectPtr);
void BrowserRegionAppendRectangle(
    X11WasmRegion *regionPtr,
    const XRectangle *rectPtr);
void BrowserRegionClear(
    X11WasmRegion *regionPtr);
void BrowserRegionSetRectangles(
    X11WasmRegion *regionPtr,
    const XRectangle *rects,
    int count);
void BrowserRegisterGC(
    GC gc,
    unsigned int depth);
void BrowserRememberColor(
    Colormap colormap,
    const XColor *colorPtr);
void BrowserRememberHostAddress(
    const XHostAddress *hostPtr);
void BrowserRememberKeycode(
    unsigned int keycode,
    KeySym keySym);
int BrowserReportXError(
    Display *display,
    int errorCode,
    XID resourceId,
    unsigned char requestCode,
    unsigned char minorCode);
Time BrowserRequestEffectiveTime(
    Time time);
int BrowserRequestTimeIsValid(
    Time time,
    Time lastChangeTime);
void BrowserResetHostAccessControl(void);
int BrowserResizeRoot(
    unsigned int width,
    unsigned int height,
    int queueEvents);
Window BrowserResolveEventWindow(
    Window window);
Window BrowserResolveKeyEventWindow(
    Window window);
Window BrowserResolvePointerEventWindow(
    Window window,
    int *xPtr,
    int *yPtr);
void BrowserResolveWindowBackgroundStyle(
    X11WasmResource *resourcePtr,
    int *hasPixelPtr,
    unsigned long *pixelPtr,
    Pixmap *pixmapPtr);
void BrowserResolveWindowBorderStyle(
    X11WasmResource *resourcePtr,
    int *hasPixelPtr,
    unsigned long *pixelPtr,
    Pixmap *pixmapPtr);
int BrowserResourceIsVisible(
    X11WasmResource *resourcePtr);
int BrowserResourceSupportsProtocol(
    X11WasmResource *resourcePtr,
    Atom protocolAtom);
int BrowserResourceWantsEvent(
    X11WasmResource *resourcePtr,
    long mask);
void BrowserRestackWindow(
    X11WasmResource *resourcePtr,
    unsigned int valueMask,
    XWindowChanges *changes);
int BrowserScreenMillimeters(
    int pixels);
int BrowserScreenOrError(
    Display *display,
    int screen,
    int requestCode);
Time BrowserSelectionEventTime(
    Time time);
void BrowserSortPolygonIntersections(
    BrowserPolygonIntersection *intersections,
    int count);
Window BrowserSelectionOwnerWindow(
    Atom selection);
int BrowserSelectionTargetIsText(
    Atom target);
int BrowserSendEventMaskMatches(
    const XEvent *eventPtr,
    X11WasmResource *resourcePtr,
    long mask);
Window BrowserSendEventTarget(
    Window window,
    Bool propagate,
    long mask,
    const XEvent *eventPtr);
void BrowserSetCommonEventFields(
    XEvent *eventPtr,
    int type,
    Window window,
    Time time);
void BrowserSetExternalClipboardText(
    const char *utf8,
    int utf8Length);
void BrowserSetFontProperty(
    XFontStruct *fontPtr,
    int *indexPtr,
    Atom name,
    unsigned long value);
void BrowserSetFontSetExtents(
    XFontSetExtents *extentsPtr,
    int charWidth,
    int ascent,
    int descent);
void BrowserSetGCClip(
    GC gc,
    int enabled,
    const XRectangle *rectPtr);
void BrowserSetGCClipRects(
    GC gc,
    int enabled,
    const XRectangle *rects,
    int count);
void BrowserSetGCDashes(
    GC gc,
    const char *dashList,
    int count);
void BrowserSetImageMasks(
    XImage *imagePtr);
void BrowserSetModifierKey(
    X11WasmDisplayState *state,
    int index,
    int slot,
    KeySym keySym);
int BrowserSetProperty(
    X11WasmResource *resourcePtr,
    Atom atom,
    Atom type,
    int format,
    int mode,
    _Xconst unsigned char *data,
    int nelements);
void BrowserSetRectangleBounds(
    XRectangle *rectPtr,
    int left,
    int top,
    int right,
    int bottom);
void BrowserSetWMPropertiesFromStrings(
    Display *display,
    Window window,
    const char *windowName,
    const char *iconName,
    char **argv,
    int argc,
    XSizeHints *normalHints,
    XWMHints *wmHints,
    XClassHint *classHints,
    XICCEncodingStyle style);
void BrowserSetWindowAlpha(
    Window window,
    double alpha);
void BrowserSetWindowClassData(
    Window window,
    const unsigned char *data,
    int length);
void BrowserSetWindowClassHint(
    Window window,
    const char *resName,
    const char *resClass);
void BrowserSetWindowClientMachine(
    Window window,
    const char *clientMachine,
    int clientMachineLength);
void BrowserSetWindowColormapWindows(
    Window window,
    const unsigned char *data,
    unsigned long windowCount);
void BrowserSetWindowCommand(
    Window window,
    char **argv,
    int argc);
void BrowserSetWindowCommandData(
    Window window,
    const unsigned char *data,
    int length);
void BrowserSetWindowIconName(
    Window window,
    const char *iconName);
void BrowserSetWindowIconPhoto(
    Window window,
    int imageCount,
    const char *sizes,
    int sizesLength,
    const unsigned char *rgba,
    int rgbaLength,
    int width,
    int height);
void BrowserSetWindowIconSizes(
    Window window,
    const XIconSize *sizeList,
    int count);
void BrowserSetWindowProtocols(
    Window window,
    const unsigned char *data,
    unsigned long atomCount);
void BrowserSetWindowSizeHints(
    Window window,
    XSizeHints *hints);
void BrowserSetWindowStandardColormaps(
    Window window,
    Atom property,
    const unsigned char *data,
    int count);
void BrowserSetWindowState(
    Window window,
    int topmost,
    int zoomed,
    int fullscreen);
void BrowserSetWindowTitle(
    Window window,
    const char *title,
    int updatesDocument);
void BrowserSetWindowTransientFor(
    Window window,
    Window transientFor);
void BrowserSetWindowWMHints(
    Window window,
    XWMHints *hints);
void BrowserSetWindowWmState(
    Window window,
    int state);
void BrowserSetWindowZoomHints(
    Window window,
    XSizeHints *hints);
KeySym BrowserShiftedAsciiKeySym(
    KeySym baseKeySym);
int BrowserSiblingPrecedes(
    X11WasmResource *resourcePtr,
    X11WasmResource *siblingPtr);
X11WasmResource *BrowserStackingResource(
    X11WasmResource *resourcePtr);
unsigned int BrowserStandardColormapXId(
    XID xid);
void BrowserStoreBitmapImage(
    X11WasmResource *resourcePtr,
    XImage *image,
    int srcX,
    int srcY,
    int destX,
    int destY,
    unsigned int width,
    unsigned int height);
void BrowserStorePixmapCopy(
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
    int transparentZero);
void BrowserStorePixmapCopyImage(
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
    int transparentZero);
void BrowserStorePixmapImage(
    X11WasmResource *resourcePtr,
    GC gc,
    XImage *image,
    int srcX,
    int srcY,
    int destX,
    int destY,
    unsigned int width,
    unsigned int height);
unsigned int BrowserStylePixmap(
    Pixmap pixmap);
void BrowserSubtractRectangle(
    const XRectangle *sourcePtr,
    const XRectangle *subtractPtr,
    XRectangle **rectsPtr,
    int *countPtr);
void BrowserSyncHostAccessControl(void);
void BrowserSyncInheritedChildStyles(
    X11WasmResource *resourcePtr);
void BrowserSyncInputContext(
    XIC inputContext);
void BrowserSyncKeyboardGrabState(void);
void BrowserSyncPointerGrabCursor(void);
void BrowserSyncPointerGrabState(void);
void BrowserSyncPointerState(void);
void BrowserSyncRequestState(
    Display *display);
void BrowserSyncClipboardProperty(
    Atom property,
    const X11WasmProperty *propertyPtr);
void BrowserSyncSelectionOwner(
    Atom selection,
    Window owner,
    Time time);
void BrowserSyncServerGrabState(void);
void BrowserSyncSynchronizeState(
    Display *display);
void BrowserSyncTransientStacking(void);
void BrowserSyncWindowAttributes(
    X11WasmResource *resourcePtr);
void BrowserSyncWindowColormap(
    X11WasmResource *resourcePtr);
void BrowserSyncWindowEventMask(
    X11WasmResource *resourcePtr);
void BrowserSyncWindowStyle(
    X11WasmResource *resourcePtr);
int BrowserSynchronizedAfterFunction(
    Display *display);
Atom BrowserTargetsAtom(void);
int BrowserTestAtomListContains(
    const Atom *atoms,
    int count,
    Atom atom);
int BrowserTestExternalClipboardTarget(
    Atom target,
    Atom expectedType,
    const unsigned char *expectedBytes,
    unsigned long expectedByteCount,
    const char *expectedUtf8);
void BrowserTestIMDestroyCallback(
    XIM inputMethod,
    XPointer clientData,
    XPointer callData);
void BrowserTestIMInstantiateCallback(
    Display *display,
    XPointer clientData,
    XPointer callData);
int BrowserTestTextPropertyEquals(
    const XTextProperty *textProp,
    const char *value);
int BrowserTestXErrorHandler(
    Display *display,
    XErrorEvent *eventPtr);
int BrowserTestXIOErrorHandler(
    Display *display);
Atom BrowserTextAtom(void);
int BrowserTextByteCount(
    const char *text,
    int byteCount);
int BrowserTextListToTextProperty(
    Display *display,
    char **list,
    int count,
    XICCEncodingStyle style,
    XTextProperty *textPropReturn);
Atom BrowserTextPropertyEncodingForStyle(
    XICCEncodingStyle style,
    char **list,
    int count);
const char *BrowserTextPropertyEncodingName(
    Atom encoding);
Status BrowserTextPropertyToStringList(
    const XTextProperty *textProp,
    char ***listReturn,
    int *countReturn);
Status BrowserTextPropertyToUtf8StringList(
    const XTextProperty *textProp,
    char ***listReturn,
    int *countReturn);
Atom BrowserTimestampAtom(void);
X11WasmTrackedGC *BrowserTrackedGC(
    GC gc);
int BrowserTransientChainContains(
    X11WasmResource *resourcePtr,
    Window ancestor);
void BrowserTranslateWindowCoordinates(
    Window source,
    Window destination,
    int sourceX,
    int sourceY,
    int *destXPtr,
    int *destYPtr);
void BrowserUnionRectangle(
    XRectangle *destPtr,
    int *emptyPtr,
    const XRectangle *sourcePtr);
void BrowserUnlinkChild(
    X11WasmResource *resourcePtr);
int BrowserUnmapWindowResource(
    X11WasmResource *resourcePtr,
    Bool fromConfigure);
int BrowserUnregisterGC(
    GC gc);
KeySym BrowserUnshiftedAsciiKeySym(
    KeySym keySym);
void BrowserUpdatePointerState(
    Window window,
    int rootX,
    int rootY,
    unsigned int state);
void BrowserUpdateQueueLength(void);
int BrowserUtf8CharacterCount(
    const char *utf8,
    int length);
int BrowserUtf8CodepointLength(
    unsigned int codepoint);
unsigned int BrowserUtf8DecodeOne(
    const unsigned char **cursorPtr);
int BrowserUtf8FromCodepoint(
    unsigned int codepoint,
    char *buffer,
    int bufferLength);
char *BrowserUtf8FromLatin1String(
    const char *string,
    int count,
    int *lengthPtr);
char *BrowserUtf8FromWideString(
    const wchar_t *string);
char *BrowserUtf8FromWideStringCount(
    const wchar_t *string,
    int count,
    int *lengthPtr);
char *BrowserUtf8FromXChar2bString(
    const XChar2b *string,
    int count,
    int *lengthPtr);
int BrowserUtf8ListFitsLatin1(
    char **list,
    int count);
int BrowserUtf8SequenceLength(
    const char *text,
    int available);
Atom BrowserUtf8StringAtom(void);
int BrowserUtf8StringFitsLatin1(
    const char *string);
int BrowserValidateConfigureWindowValues(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    unsigned int valueMask,
    XWindowChanges *changes,
    unsigned char requestCode);
int BrowserValidateCoordMode(
    Display **displayPtr,
    int coordMode,
    unsigned char requestCode);
int BrowserValidateGCArcMode(
    Display **displayPtr,
    int arcMode,
    unsigned char requestCode);
int BrowserValidateGCBool(
    Display **displayPtr,
    int value,
    unsigned char requestCode);
int BrowserValidateGCCapStyle(
    Display **displayPtr,
    int capStyle,
    unsigned char requestCode);
int BrowserValidateGCClipRectangles(
    Display **displayPtr,
    const XRectangle *rectangles,
    int count,
    int ordering,
    unsigned char requestCode);
int BrowserValidateGCDashList(
    Display **displayPtr,
    const char *dashList,
    int count,
    unsigned char requestCode);
int BrowserValidateGCFillRule(
    Display **displayPtr,
    int fillRule,
    unsigned char requestCode);
int BrowserValidateGCFillStyle(
    Display **displayPtr,
    int fillStyle,
    unsigned char requestCode);
int BrowserValidateGCFunction(
    Display **displayPtr,
    int function,
    unsigned char requestCode);
int BrowserValidateGCJoinStyle(
    Display **displayPtr,
    int joinStyle,
    unsigned char requestCode);
int BrowserValidateGCLineStyle(
    Display **displayPtr,
    int lineStyle,
    unsigned char requestCode);
int BrowserValidateGCPixmapDepth(
    Display **displayPtr,
    Pixmap pixmap,
    unsigned int expectedDepth,
    unsigned char requestCode,
    int allowNone);
int BrowserValidateGCSubwindowMode(
    Display **displayPtr,
    int subwindowMode,
    unsigned char requestCode);
int BrowserValidateGCValueEnums(
    Display **displayPtr,
    unsigned long mask,
    XGCValues *values,
    unsigned char requestCode);
int BrowserValidateGCValueReferences(
    Display **displayPtr,
    unsigned long mask,
    XGCValues *values,
    unsigned int gcDepth,
    unsigned char requestCode);
int BrowserValidateHostAddress(
    Display **displayPtr,
    XHostAddress *hostPtr,
    unsigned char requestCode);
int BrowserValidateInputOnlyChangeWindowAttributes(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    unsigned long valueMask,
    unsigned char requestCode);
int BrowserValidateInputOnlyCreateWindow(
    Display **displayPtr,
    Window parent,
    int depth,
    Visual *visual,
    unsigned int borderWidth,
    unsigned long valueMask,
    unsigned char requestCode);
int BrowserValidateInputOutputCreateWindow(
    Display **displayPtr,
    X11WasmResource *parentPtr,
    int depth,
    Visual *visual,
    unsigned char requestCode);
int BrowserValidatePolygonShape(
    Display **displayPtr,
    int shape,
    unsigned char requestCode);
int BrowserValidateScreenSaverAllowExposures(
    Display **displayPtr,
    int value,
    unsigned char requestCode);
int BrowserValidateScreenSaverPreferBlanking(
    Display **displayPtr,
    int value,
    unsigned char requestCode);
int BrowserValidateWindowAttributeValues(
    Display **displayPtr,
    unsigned int windowDepth,
    unsigned long mask,
    XSetWindowAttributes *atts,
    unsigned char requestCode);
int BrowserValidateWindowEventMask(
    Display **displayPtr,
    long eventMask,
    unsigned long validMask,
    unsigned char requestCode);
int BrowserValidateWindowPixmapAttribute(
    Display **displayPtr,
    unsigned int windowDepth,
    Pixmap pixmap,
    unsigned char requestCode,
    int isBackground);
int BrowserValueOrError(
    Display **displayPtr,
    long value,
    int valid,
    unsigned char requestCode);
int BrowserVisualInfoMatches(
    const XVisualInfo *infoPtr,
    long mask,
    const XVisualInfo *templatePtr);
wchar_t *BrowserWideStringFromUtf8(
    const char *utf8);
int BrowserWinGravityHorizontalOffset(
    int gravity,
    int deltaWidth);
int BrowserWinGravityVerticalOffset(
    int gravity,
    int deltaHeight);
int BrowserWindowCanReceiveEvents(
    Window window);
int BrowserWindowClassOrError(
    Display **displayPtr,
    unsigned int clazz,
    unsigned int parentClass,
    unsigned char requestCode,
    unsigned int *classPtr);
void BrowserWindowCoordinatesFromRoot(
    Window window,
    int rootX,
    int rootY,
    int *xPtr,
    int *yPtr);
int BrowserWindowHasStateAtom(
    X11WasmProperty *propertyPtr,
    Atom stateAtom);
int BrowserWindowInTree(
    Window window,
    Window ancestor);
int BrowserWindowInheritsStyle(
    X11WasmResource *resourcePtr);
int BrowserWindowIsRootToplevel(
    Window window);
Window BrowserWindowMetadataTarget(
    Window window);
Atom BrowserWindowOpacityAtom(void);
X11WasmResource *BrowserWindowResource(
    Window window);
X11WasmResource *BrowserWindowResourceOrError(
    Display **displayPtr,
    Window window,
    unsigned char requestCode);
void BrowserWindowRootOffset(
    Window window,
    int *xPtr,
    int *yPtr);
int BrowserWindowSupportsProtocol(
    X11WasmResource *resourcePtr,
    Atom protocolAtom);
Window BrowserWindowTransientFor(
    X11WasmResource *resourcePtr);
Atom BrowserWmColormapWindowsAtom(void);
Atom BrowserWmProtocolsAtom(void);
int BrowserXCursorGlyphBit(
    const unsigned char *data,
    unsigned int offset,
    unsigned int x,
    unsigned int y);
unsigned int BrowserXCursorGlyphHotspot(
    const unsigned char *data,
    unsigned int offset);

#endif /* X11WASM_LIBX11_XLIBINT_H */
