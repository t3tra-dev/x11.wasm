/*
 * Browser.h --
 *
 *	Internal declarations for the Browser/WASM X11 backend.
 */

#ifndef _X11WASM_BROWSER_H
#define _X11WASM_BROWSER_H

#include <X11Wasm/Runtime.h>

#define X11_WASM_MODIFIER_COUNT 8
#define X11_WASM_MODIFIER_MAP_MAX_KEYPERMOD 8
#define X11_WASM_MOTION_BUFFER_SIZE 64
#define X11_WASM_DEFAULT_GCONTEXT 3

typedef enum {
    X11_WASM_RESOURCE_WINDOW,
    X11_WASM_RESOURCE_PIXMAP,
    X11_WASM_RESOURCE_COLORMAP,
    X11_WASM_RESOURCE_CURSOR,
    X11_WASM_RESOURCE_FONT
} X11WasmResourceType;

typedef struct X11WasmProperty X11WasmProperty;
typedef struct X11WasmColorEntry X11WasmColorEntry;
typedef struct X11WasmPassiveButtonGrab X11WasmPassiveButtonGrab;
typedef struct X11WasmPassiveKeyGrab X11WasmPassiveKeyGrab;

typedef struct X11WasmResource {
    XID id;
    X11WasmResourceType type;
    Display *display;
    struct X11WasmResource *nextPtr;
    struct X11WasmResource *parentPtr;
    struct X11WasmResource *firstChildPtr;
    struct X11WasmResource *lastChildPtr;
    struct X11WasmResource *prevSiblingPtr;
    struct X11WasmResource *nextSiblingPtr;
    Window parent;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned int borderWidth;
    unsigned int depth;
    unsigned int class;
    unsigned char *backingData;
    int backingBytesPerLine;
    int backingBitsPerPixel;
    unsigned char *bitmapData;
    int bitmapBytesPerLine;
    int mapped;
    int damaged;
    XRectangle damage;
    unsigned long damageSerial;
    XSetWindowAttributes atts;
    int hasBackgroundPixel;
    int hasBorderPixel;
    unsigned int cursorShape;
    int hasCursorImage;
    unsigned int referenceCount;
    int clientReleased;
    char *fontFamily;
    int fontPixelSize;
    int fontPointSize;
    int colormapInstalled;
    X11WasmProperty *propertyList;
    X11WasmColorEntry *colorList;
    Window metadataTarget;
    int metadataRootToplevelSet;
    int metadataIsRootToplevel;
    int retained;
    int temporary;
} X11WasmResource;

typedef struct _XRegion {
    int empty;
    XRectangle extents;
    int rectCount;
    XRectangle *rects;
} X11WasmRegion;

typedef struct X11WasmHostEntry {
    int family;
    int length;
    char *address;
} X11WasmHostEntry;

typedef struct X11WasmMotionRecord {
    Time time;
    int rootX;
    int rootY;
} X11WasmMotionRecord;

typedef struct X11WasmDisplayState {
    Display display;
    Screen screen;
    Visual visual;
    Depth depth;
    ScreenFormat pixmapFormats[2];
    XGCValues defaultGC;
    GContext defaultGCContext;
    Window rootWindow;
    Window focusWindow;
    int focusRevertTo;
    Time focusTime;
    Window pointerWindow;
    Window pointerCaptureWindow;
    Window pointerGrabWindow;
    Window pointerGrabConfineWindow;
    Cursor pointerGrabCursor;
    unsigned int pointerGrabEventMask;
    int pointerGrabOwnerEvents;
    int pointerGrabPointerMode;
    int pointerGrabKeyboardMode;
    Time pointerGrabTime;
    int pointerGrabPassive;
    Window keyboardGrabWindow;
    int keyboardGrabOwnerEvents;
    int keyboardGrabPointerMode;
    int keyboardGrabKeyboardMode;
    Time keyboardGrabTime;
    int keyboardGrabPassive;
    X11WasmPassiveButtonGrab *passiveButtonGrabs;
    X11WasmPassiveKeyGrab *passiveKeyGrabs;
    int serverGrabDepth;
    int synchronize;
    XAfterFunction afterFunction;
    int runningAfterFunction;
    unsigned long flushSerial;
    unsigned long syncSerial;
    unsigned long closeSerial;
    int closeDownMode;
    int screenSaverTimeout;
    int screenSaverInterval;
    int screenSaverPreferBlanking;
    int screenSaverAllowExposures;
    int screenSaverActive;
    unsigned long screenSaverTimerResetIdle;
    unsigned long screenSaverActivatedIdle;
    int keyClickPercent;
    int bellPercent;
    unsigned int bellPitch;
    unsigned int bellDuration;
    unsigned long ledMask;
    int globalAutoRepeat;
    char autoRepeats[32];
    char keymap[32];
    KeySym keycodeMap[256][2];
    unsigned char keycodeMapExplicit[256];
    unsigned int nextSyntheticKeycode;
    unsigned long keyboardMapSerial;
    unsigned long modifierMapSerial;
    int modifierMapMaxKeyPerMod;
    KeyCode modifierMap[X11_WASM_MODIFIER_COUNT
	    * X11_WASM_MODIFIER_MAP_MAX_KEYPERMOD];
    int hostAccessEnabled;
    int hostCount;
    X11WasmHostEntry *hosts;
    int pointerX;
    int pointerY;
    unsigned int pointerState;
    int motionHistoryStart;
    int motionHistoryCount;
    X11WasmMotionRecord motionHistory[X11_WASM_MOTION_BUFFER_SIZE];
    int pointerAccelNumerator;
    int pointerAccelDenominator;
    int pointerThreshold;
    int pointerMapCount;
    unsigned char pointerMap[5];
    Colormap defaultColormap;
    XID nextXId;
    X11WasmResource rootResource;
    X11WasmResource *resourceList;
} X11WasmDisplayState;

MODULE_SCOPE Display *X11WasmGetDisplay(const char *displayName);
MODULE_SCOPE XID X11WasmAllocXId(Display *display);
MODULE_SCOPE X11WasmResource *X11WasmFindResource(XID id);
MODULE_SCOPE X11WasmResource *X11WasmCreateResource(
	Display *display, X11WasmResourceType type);
MODULE_SCOPE void X11WasmDeleteResource(XID id);
MODULE_SCOPE int X11WasmServiceEvents(void);

#ifdef __EMSCRIPTEN__
extern int X11WasmJsInit(int width, int height, unsigned int rootXid);
extern int X11WasmJsGetRootWidth(void);
extern int X11WasmJsGetRootHeight(void);
extern int X11WasmJsCreateWindow(unsigned int window, unsigned int parent,
	int x, int y, unsigned int width, unsigned int height,
	unsigned int depth, unsigned long eventMask, unsigned int clazz);
extern int X11WasmJsCreatePixmap(unsigned int pixmap, unsigned int drawable,
	unsigned int width, unsigned int height, unsigned int depth);
extern int X11WasmJsDestroyResource(unsigned int xid);
extern int X11WasmJsMapWindow(unsigned int window);
extern int X11WasmJsUnmapWindow(unsigned int window);
extern int X11WasmJsSetWindowEventMask(unsigned int window,
	unsigned long eventMask, unsigned long doNotPropagateMask);
extern int X11WasmJsSetWindowColormap(unsigned int window,
	unsigned int colormap);
extern int X11WasmJsSetWindowAttributes(unsigned int window,
	int overrideRedirect, int saveUnder, int bitGravity, int winGravity,
	int backingStore, unsigned long backingPlanes,
	unsigned long backingPixel);
extern int X11WasmJsConfigureWindow(unsigned int window, int x, int y,
	unsigned int width, unsigned int height, unsigned int borderWidth,
	unsigned int valueMask);
extern int X11WasmJsRestackWindow(unsigned int window,
	unsigned int previousSibling);
extern int X11WasmJsReparentWindow(unsigned int window, unsigned int parent,
	int x, int y);
extern int X11WasmJsClearWindow(unsigned int window);
extern int X11WasmJsClearArea(unsigned int window, int x, int y,
	unsigned int width, unsigned int height);
extern int X11WasmJsCopyArea(unsigned int source, unsigned int target,
	int srcX, int srcY, unsigned int width, unsigned int height,
	int destX, int destY);
extern int X11WasmJsCopyPlane(unsigned int source, unsigned int target,
	int srcX, int srcY, unsigned int width, unsigned int height,
	int destX, int destY, unsigned long foreground,
	unsigned long background, unsigned long plane, int transparentZero);
extern int X11WasmJsDrawLine(unsigned int drawable, unsigned long pixel,
	int x1, int y1, int x2, int y2);
extern int X11WasmJsDrawRectangle(unsigned int drawable, unsigned long pixel,
	int x, int y, unsigned int width, unsigned int height, int fill);
extern int X11WasmJsDrawArc(unsigned int drawable, unsigned long pixel,
	int x, int y, unsigned int width, unsigned int height, int angle1,
	int angle2, int fill, int pieSlice, unsigned int lineWidth);
extern int X11WasmJsFillPolygon(unsigned int drawable, unsigned long pixel,
	const int *points, int pointCount, int fillRule);
extern int X11WasmJsDrawText(unsigned int drawable, unsigned long pixel,
	int x, int y, const char *utf8, int utf8Length);
extern int X11WasmJsMeasureText(int fontPixelSize, const char *fontFamily,
	const char *utf8, int utf8Length);
extern int X11WasmJsPutImage(unsigned int drawable, const void *data,
	int dataLength, int srcX, int srcY, int destX, int destY,
	unsigned int width, unsigned int height, int bytesPerLine,
	int bitsPerPixel);
extern int X11WasmJsGetImage(unsigned int drawable, int srcX, int srcY,
	unsigned int width, unsigned int height, void *data, int dataLength,
	int bytesPerLine, int bitsPerPixel);
extern int X11WasmJsSetClipRect(int enabled, int x, int y,
	unsigned int width, unsigned int height);
extern int X11WasmJsSetClipRects(int enabled, const XRectangle *rectangles,
	int count, int xOrigin, int yOrigin);
extern int X11WasmJsSetClipMask(int enabled, unsigned int pixmap,
	int xOrigin, int yOrigin);
extern int X11WasmJsSetDrawStyle(int function, unsigned int lineWidth,
	int lineStyle, int capStyle, int joinStyle, int dashOffset,
	unsigned int dashLength, const unsigned char *dashPattern,
	int dashCount, int fillStyle, unsigned int stipple,
	unsigned int tile, int tsOriginX, int tsOriginY,
	unsigned long backgroundPixel, int fontPixelSize,
	const char *fontFamily, unsigned long planeMask);
extern int X11WasmJsSetWindowStyle(unsigned int window, int hasBackground,
	unsigned long backgroundPixel, unsigned int backgroundPixmap,
	int hasBorder, unsigned long borderPixel, unsigned int borderPixmap,
	int hasCursor, unsigned int cursorShape);
extern int X11WasmJsCreateCursor(unsigned int cursor, const void *rgba,
	int dataLength, unsigned int width, unsigned int height, unsigned int hotX,
	unsigned int hotY);
extern int X11WasmJsFlush(void);
extern int X11WasmJsSetRequestState(unsigned int requestSerial,
	unsigned int flushSerial, unsigned int syncSerial);
extern int X11WasmJsBell(int percent);
extern int X11WasmJsForceScreenSaver(int mode);
extern int X11WasmJsGetUserIdleTime(void);
extern int X11WasmJsSetScreenSaverSettings(int timeout, int interval,
	int preferBlanking, int allowExposures);
extern int X11WasmJsSetHostAccessControl(int enabled,
	const char *hostsUtf8, int hostsUtf8Length, int count);
extern int X11WasmJsSetServerGrabState(int depth);
extern int X11WasmJsSetInputFocus(int focused, unsigned int window);
extern int X11WasmJsSetInputContext(int focused, unsigned int window,
	int hasSpot, int spotRootX, int spotRootY);
extern int X11WasmJsSetPointerGrabCursor(int enabled, unsigned int cursor);
extern int X11WasmJsSetPointerGrabState(int enabled, unsigned int window,
	int ownerEvents, unsigned int eventMask, unsigned int confineWindow,
	unsigned int cursor, int pointerMode, int keyboardMode);
extern int X11WasmJsSetKeyboardGrabState(int enabled, unsigned int window,
	int ownerEvents, int pointerMode, int keyboardMode);
extern int X11WasmJsSetPointerState(int rootX, int rootY,
	unsigned int window, unsigned int state, unsigned int captureWindow);
extern int X11WasmJsWarpPointer(int rootX, int rootY,
	unsigned int targetWindow, unsigned int time);
extern int X11WasmJsSetSynchronizeState(int enabled);
extern int X11WasmJsSetWindowTitle(unsigned int window, const char *utf8,
	int utf8Length, int updatesDocument);
extern int X11WasmJsSetWindowIconName(unsigned int window, const char *utf8,
	int utf8Length);
extern int X11WasmJsSetWindowTransientFor(unsigned int window,
	unsigned int transientFor);
extern int X11WasmJsSetWindowCommand(unsigned int window, const char *utf8,
	int utf8Length);
extern int X11WasmJsSetWindowClientMachine(unsigned int window,
	const char *utf8, int utf8Length);
extern int X11WasmJsSetWindowClassHint(unsigned int window,
	const char *nameUtf8, int nameUtf8Length, const char *classUtf8,
	int classUtf8Length);
extern int X11WasmJsSetWindowAlpha(unsigned int window, double alpha);
extern int X11WasmJsSetClipboardText(const char *utf8, int utf8Length);
extern int X11WasmJsSetSelectionOwner(const char *selectionUtf8,
	int selectionUtf8Length, unsigned int owner, unsigned int time);
extern int X11WasmJsSetWindowWMHints(unsigned int window, int flags,
	int input, int initialState, unsigned int iconPixmap,
	unsigned int iconWindow, int iconX, int iconY, unsigned int iconMask,
	unsigned int windowGroup);
extern int X11WasmJsSetWindowIconSizes(unsigned int window,
	const char *sizesUtf8, int sizesUtf8Length, int count);
extern int X11WasmJsSetWindowIconPhoto(unsigned int window, int imageCount,
	const char *sizesUtf8, int sizesUtf8Length, const unsigned char *rgba,
	int rgbaLength, int width, int height);
extern int X11WasmJsSetWindowProtocols(unsigned int window,
	const char *utf8, int utf8Length);
extern int X11WasmJsSetWindowColormapWindows(unsigned int window,
	const char *utf8, int utf8Length);
extern int X11WasmJsSetWindowStandardColormaps(unsigned int window,
	const char *propertyUtf8, int propertyUtf8Length,
	const char *mapsUtf8, int mapsUtf8Length, int count);
extern int X11WasmJsSetWindowState(unsigned int window, int topmost,
	int zoomed, int fullscreen);
extern int X11WasmJsSetWindowWmState(unsigned int window, int state);
extern int X11WasmJsSetWindowSizeHints(unsigned int window, int flags,
	int x, int y, int width, int height, int minWidth, int minHeight,
	int maxWidth, int maxHeight, int widthInc, int heightInc,
	int minAspectX, int minAspectY, int maxAspectX, int maxAspectY,
	int baseWidth, int baseHeight, int winGravity);
extern int X11WasmJsSetWindowZoomHints(unsigned int window, int flags,
	int x, int y, int width, int height, int minWidth, int minHeight,
	int maxWidth, int maxHeight, int widthInc, int heightInc,
	int minAspectX, int minAspectY, int maxAspectX, int maxAspectY,
	int baseWidth, int baseHeight, int winGravity);
#else
static inline int
X11WasmJsInit(int width, int height, unsigned int rootXid)
{ (void)width; (void)height; (void)rootXid; return 0; }
static inline int
X11WasmJsGetRootWidth(void)
{ return 0; }
static inline int
X11WasmJsGetRootHeight(void)
{ return 0; }
static inline int
X11WasmJsCreateWindow(unsigned int window, unsigned int parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int depth,
	unsigned long eventMask, unsigned int clazz)
{ (void)window; (void)parent; (void)x; (void)y; (void)width; (void)height; (void)depth; (void)eventMask; (void)clazz; return 0; }
static inline int
X11WasmJsCreatePixmap(unsigned int pixmap, unsigned int drawable,
	unsigned int width, unsigned int height, unsigned int depth)
{ (void)pixmap; (void)drawable; (void)width; (void)height; (void)depth; return 0; }
static inline int
X11WasmJsDestroyResource(unsigned int xid)
{ (void)xid; return 0; }
static inline int
X11WasmJsMapWindow(unsigned int window)
{ (void)window; return 0; }
static inline int
X11WasmJsUnmapWindow(unsigned int window)
{ (void)window; return 0; }
static inline int
X11WasmJsSetWindowEventMask(unsigned int window, unsigned long eventMask,
	unsigned long doNotPropagateMask)
{ (void)window; (void)eventMask; (void)doNotPropagateMask; return 0; }
static inline int
X11WasmJsSetWindowColormap(unsigned int window, unsigned int colormap)
{ (void)window; (void)colormap; return 0; }
static inline int
X11WasmJsSetWindowAttributes(unsigned int window, int overrideRedirect,
	int saveUnder, int bitGravity, int winGravity, int backingStore,
	unsigned long backingPlanes, unsigned long backingPixel)
{
    (void)window;
    (void)overrideRedirect;
    (void)saveUnder;
    (void)bitGravity;
    (void)winGravity;
    (void)backingStore;
    (void)backingPlanes;
    (void)backingPixel;
    return 0;
}
static inline int
X11WasmJsConfigureWindow(unsigned int window, int x, int y,
	unsigned int width, unsigned int height, unsigned int borderWidth,
	unsigned int valueMask)
{ (void)window; (void)x; (void)y; (void)width; (void)height; (void)borderWidth; (void)valueMask; return 0; }
static inline int
X11WasmJsRestackWindow(unsigned int window, unsigned int previousSibling)
{ (void)window; (void)previousSibling; return 0; }
static inline int
X11WasmJsReparentWindow(unsigned int window, unsigned int parent, int x,
	int y)
{ (void)window; (void)parent; (void)x; (void)y; return 0; }
static inline int
X11WasmJsClearWindow(unsigned int window)
{ (void)window; return 0; }
static inline int
X11WasmJsClearArea(unsigned int window, int x, int y, unsigned int width,
	unsigned int height)
{ (void)window; (void)x; (void)y; (void)width; (void)height; return 0; }
static inline int
X11WasmJsCopyArea(unsigned int source, unsigned int target, int srcX,
	int srcY, unsigned int width, unsigned int height, int destX, int destY)
{ (void)source; (void)target; (void)srcX; (void)srcY; (void)width; (void)height; (void)destX; (void)destY; return 0; }
static inline int
X11WasmJsCopyPlane(unsigned int source, unsigned int target, int srcX,
	int srcY, unsigned int width, unsigned int height, int destX,
	int destY, unsigned long foreground, unsigned long background,
	unsigned long plane, int transparentZero)
{ (void)source; (void)target; (void)srcX; (void)srcY; (void)width; (void)height; (void)destX; (void)destY; (void)foreground; (void)background; (void)plane; (void)transparentZero; return 0; }
static inline int
X11WasmJsDrawLine(unsigned int drawable, unsigned long pixel, int x1,
	int y1, int x2, int y2)
{ (void)drawable; (void)pixel; (void)x1; (void)y1; (void)x2; (void)y2; return 0; }
static inline int
X11WasmJsDrawRectangle(unsigned int drawable, unsigned long pixel, int x,
	int y, unsigned int width, unsigned int height, int fill)
{ (void)drawable; (void)pixel; (void)x; (void)y; (void)width; (void)height; (void)fill; return 0; }
static inline int
X11WasmJsDrawArc(unsigned int drawable, unsigned long pixel, int x, int y,
	unsigned int width, unsigned int height, int angle1, int angle2,
	int fill, int pieSlice, unsigned int lineWidth)
{ (void)drawable; (void)pixel; (void)x; (void)y; (void)width; (void)height; (void)angle1; (void)angle2; (void)fill; (void)pieSlice; (void)lineWidth; return 0; }
static inline int
X11WasmJsFillPolygon(unsigned int drawable, unsigned long pixel,
	const int *points, int pointCount, int fillRule)
{ (void)drawable; (void)pixel; (void)points; (void)pointCount; (void)fillRule; return 0; }
static inline int
X11WasmJsDrawText(unsigned int drawable, unsigned long pixel, int x, int y,
	const char *utf8, int utf8Length)
{ (void)drawable; (void)pixel; (void)x; (void)y; (void)utf8; (void)utf8Length; return 0; }
static inline int
X11WasmJsPutImage(unsigned int drawable, const void *data, int dataLength,
	int srcX, int srcY, int destX, int destY, unsigned int width,
	unsigned int height, int bytesPerLine, int bitsPerPixel)
{ (void)drawable; (void)data; (void)dataLength; (void)srcX; (void)srcY; (void)destX; (void)destY; (void)width; (void)height; (void)bytesPerLine; (void)bitsPerPixel; return 0; }
static inline int
X11WasmJsGetImage(unsigned int drawable, int srcX, int srcY,
	unsigned int width, unsigned int height, void *data, int dataLength,
	int bytesPerLine, int bitsPerPixel)
{ (void)drawable; (void)srcX; (void)srcY; (void)width; (void)height; (void)data; (void)dataLength; (void)bytesPerLine; (void)bitsPerPixel; return 0; }
static inline int
X11WasmJsSetClipRect(int enabled, int x, int y, unsigned int width,
	unsigned int height)
{ (void)enabled; (void)x; (void)y; (void)width; (void)height; return 0; }
static inline int
X11WasmJsSetClipRects(int enabled, const XRectangle *rectangles, int count,
	int xOrigin, int yOrigin)
{ (void)enabled; (void)rectangles; (void)count; (void)xOrigin; (void)yOrigin; return 0; }
static inline int
X11WasmJsSetClipMask(int enabled, unsigned int pixmap, int xOrigin,
	int yOrigin)
{ (void)enabled; (void)pixmap; (void)xOrigin; (void)yOrigin; return 0; }
static inline int
X11WasmJsSetDrawStyle(int function, unsigned int lineWidth, int lineStyle,
	int capStyle, int joinStyle, int dashOffset, unsigned int dashLength,
	const unsigned char *dashPattern, int dashCount, int fillStyle,
	unsigned int stipple, unsigned int tile, int tsOriginX, int tsOriginY,
	unsigned long backgroundPixel, int fontPixelSize,
	const char *fontFamily, unsigned long planeMask)
{ (void)function; (void)lineWidth; (void)lineStyle; (void)capStyle; (void)joinStyle; (void)dashOffset; (void)dashLength; (void)dashPattern; (void)dashCount; (void)fillStyle; (void)stipple; (void)tile; (void)tsOriginX; (void)tsOriginY; (void)backgroundPixel; (void)fontPixelSize; (void)fontFamily; (void)planeMask; return 0; }
static inline int
X11WasmJsSetWindowStyle(unsigned int window, int hasBackground,
	unsigned long backgroundPixel, unsigned int backgroundPixmap,
	int hasBorder, unsigned long borderPixel, unsigned int borderPixmap,
	int hasCursor, unsigned int cursorShape)
{ (void)window; (void)hasBackground; (void)backgroundPixel; (void)backgroundPixmap; (void)hasBorder; (void)borderPixel; (void)borderPixmap; (void)hasCursor; (void)cursorShape; return 0; }
static inline int
X11WasmJsCreateCursor(unsigned int cursor, const void *rgba, int dataLength,
	unsigned int width, unsigned int height, unsigned int hotX,
	unsigned int hotY)
{
    (void)cursor;
    (void)rgba;
    (void)dataLength;
    (void)width;
    (void)height;
    (void)hotX;
    (void)hotY;
    return 0;
}
static inline int
X11WasmJsBell(int percent)
{ (void)percent; return 0; }
static inline int
X11WasmJsForceScreenSaver(int mode)
{ (void)mode; return 0; }
static inline int
X11WasmJsSetScreenSaverSettings(int timeout, int interval, int preferBlanking,
	int allowExposures)
{
    (void)timeout;
    (void)interval;
    (void)preferBlanking;
    (void)allowExposures;
    return 0;
}
static inline int
X11WasmJsSetHostAccessControl(int enabled, const char *hostsUtf8,
	int hostsUtf8Length, int count)
{
    (void)enabled;
    (void)hostsUtf8;
    (void)hostsUtf8Length;
    (void)count;
    return 0;
}
static inline int
X11WasmJsSetServerGrabState(int depth)
{
    (void)depth;
    return 0;
}
static inline int
X11WasmJsSetInputFocus(int focused, unsigned int window)
{ (void)focused; (void)window; return 0; }
static inline int
X11WasmJsSetInputContext(int focused, unsigned int window, int hasSpot,
	int spotRootX, int spotRootY)
{
    (void)focused;
    (void)window;
    (void)hasSpot;
    (void)spotRootX;
    (void)spotRootY;
    return 0;
}
static inline int
X11WasmJsSetPointerGrabCursor(int enabled, unsigned int cursor)
{ (void)enabled; (void)cursor; return 0; }
static inline int
X11WasmJsSetPointerGrabState(int enabled, unsigned int window,
	int ownerEvents, unsigned int eventMask, unsigned int confineWindow,
	unsigned int cursor, int pointerMode, int keyboardMode)
{
    (void)enabled;
    (void)window;
    (void)ownerEvents;
    (void)eventMask;
    (void)confineWindow;
    (void)cursor;
    (void)pointerMode;
    (void)keyboardMode;
    return 0;
}
static inline int
X11WasmJsSetKeyboardGrabState(int enabled, unsigned int window,
	int ownerEvents, int pointerMode, int keyboardMode)
{
    (void)enabled;
    (void)window;
    (void)ownerEvents;
    (void)pointerMode;
    (void)keyboardMode;
    return 0;
}
static inline int
X11WasmJsSetPointerState(int rootX, int rootY, unsigned int window,
	unsigned int state, unsigned int captureWindow)
{
    (void)rootX;
    (void)rootY;
    (void)window;
    (void)state;
    (void)captureWindow;
    return 0;
}
static inline int
X11WasmJsWarpPointer(int rootX, int rootY, unsigned int targetWindow,
	unsigned int time)
{
    (void)rootX;
    (void)rootY;
    (void)targetWindow;
    (void)time;
    return 0;
}
static inline int
X11WasmJsFlush(void)
{ return 0; }
static inline int
X11WasmJsSetRequestState(unsigned int requestSerial,
	unsigned int flushSerial, unsigned int syncSerial)
{
    (void)requestSerial;
    (void)flushSerial;
    (void)syncSerial;
    return 0;
}
static inline int
X11WasmJsSetSynchronizeState(int enabled)
{ (void)enabled; return 0; }
static inline int
X11WasmJsGetUserIdleTime(void)
{ return 0; }
static inline int
X11WasmJsMeasureText(int fontPixelSize, const char *fontFamily,
	const char *utf8, int utf8Length)
{
    (void)fontPixelSize;
    (void)fontFamily;
    (void)utf8;
    (void)utf8Length;
    return 0;
}
static inline int
X11WasmJsSetWindowTitle(unsigned int window, const char *utf8, int utf8Length,
	int updatesDocument)
{ (void)window; (void)utf8; (void)utf8Length; (void)updatesDocument; return 0; }
static inline int
X11WasmJsSetWindowIconName(unsigned int window, const char *utf8,
	int utf8Length)
{ (void)window; (void)utf8; (void)utf8Length; return 0; }
static inline int
X11WasmJsSetWindowTransientFor(unsigned int window, unsigned int transientFor)
{ (void)window; (void)transientFor; return 0; }
static inline int
X11WasmJsSetWindowCommand(unsigned int window, const char *utf8,
	int utf8Length)
{ (void)window; (void)utf8; (void)utf8Length; return 0; }
static inline int
X11WasmJsSetWindowClientMachine(unsigned int window, const char *utf8,
	int utf8Length)
{ (void)window; (void)utf8; (void)utf8Length; return 0; }
static inline int
X11WasmJsSetWindowClassHint(unsigned int window, const char *nameUtf8,
	int nameUtf8Length, const char *classUtf8, int classUtf8Length)
{
    (void)window;
    (void)nameUtf8;
    (void)nameUtf8Length;
    (void)classUtf8;
    (void)classUtf8Length;
    return 0;
}
static inline int
X11WasmJsSetWindowAlpha(unsigned int window, double alpha)
{ (void)window; (void)alpha; return 0; }
static inline int
X11WasmJsSetClipboardText(const char *utf8, int utf8Length)
{ (void)utf8; (void)utf8Length; return 0; }
static inline int
X11WasmJsSetSelectionOwner(const char *selectionUtf8,
	int selectionUtf8Length, unsigned int owner, unsigned int time)
{
    (void)selectionUtf8;
    (void)selectionUtf8Length;
    (void)owner;
    (void)time;
    return 0;
}
static inline int
X11WasmJsSetWindowWMHints(unsigned int window, int flags, int input,
	int initialState, unsigned int iconPixmap, unsigned int iconWindow,
	int iconX, int iconY, unsigned int iconMask, unsigned int windowGroup)
{
    (void)window;
    (void)flags;
    (void)input;
    (void)initialState;
    (void)iconPixmap;
    (void)iconWindow;
    (void)iconX;
    (void)iconY;
    (void)iconMask;
    (void)windowGroup;
    return 0;
}
static inline int
X11WasmJsSetWindowIconSizes(unsigned int window, const char *sizesUtf8,
	int sizesUtf8Length, int count)
{
    (void)window;
    (void)sizesUtf8;
    (void)sizesUtf8Length;
    (void)count;
    return 0;
}
static inline int
X11WasmJsSetWindowIconPhoto(unsigned int window, int imageCount,
	const char *sizesUtf8, int sizesUtf8Length, const unsigned char *rgba,
	int rgbaLength, int width, int height)
{
    (void)window;
    (void)imageCount;
    (void)sizesUtf8;
    (void)sizesUtf8Length;
    (void)rgba;
    (void)rgbaLength;
    (void)width;
    (void)height;
    return 0;
}
static inline int
X11WasmJsSetWindowProtocols(unsigned int window, const char *utf8,
	int utf8Length)
{ (void)window; (void)utf8; (void)utf8Length; return 0; }
static inline int
X11WasmJsSetWindowColormapWindows(unsigned int window, const char *utf8,
	int utf8Length)
{ (void)window; (void)utf8; (void)utf8Length; return 0; }
static inline int
X11WasmJsSetWindowStandardColormaps(unsigned int window,
	const char *propertyUtf8, int propertyUtf8Length,
	const char *mapsUtf8, int mapsUtf8Length, int count)
{
    (void)window;
    (void)propertyUtf8;
    (void)propertyUtf8Length;
    (void)mapsUtf8;
    (void)mapsUtf8Length;
    (void)count;
    return 0;
}
static inline int
X11WasmJsSetWindowState(unsigned int window, int topmost, int zoomed,
	int fullscreen)
{ (void)window; (void)topmost; (void)zoomed; (void)fullscreen; return 0; }
static inline int
X11WasmJsSetWindowWmState(unsigned int window, int state)
{ (void)window; (void)state; return 0; }
static inline int
X11WasmJsSetWindowSizeHints(unsigned int window, int flags, int x, int y,
	int width, int height, int minWidth, int minHeight, int maxWidth,
	int maxHeight, int widthInc, int heightInc, int minAspectX,
	int minAspectY, int maxAspectX, int maxAspectY, int baseWidth,
	int baseHeight, int winGravity)
{
    (void)window;
    (void)flags;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)minWidth;
    (void)minHeight;
    (void)maxWidth;
    (void)maxHeight;
    (void)widthInc;
    (void)heightInc;
    (void)minAspectX;
    (void)minAspectY;
    (void)maxAspectX;
    (void)maxAspectY;
    (void)baseWidth;
    (void)baseHeight;
    (void)winGravity;
    return 0;
}
static inline int
X11WasmJsSetWindowZoomHints(unsigned int window, int flags, int x, int y,
	int width, int height, int minWidth, int minHeight, int maxWidth,
	int maxHeight, int widthInc, int heightInc, int minAspectX,
	int minAspectY, int maxAspectX, int maxAspectY, int baseWidth,
	int baseHeight, int winGravity)
{
    (void)window;
    (void)flags;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)minWidth;
    (void)minHeight;
    (void)maxWidth;
    (void)maxHeight;
    (void)widthInc;
    (void)heightInc;
    (void)minAspectX;
    (void)minAspectY;
    (void)maxAspectX;
    (void)maxAspectY;
    (void)baseWidth;
    (void)baseHeight;
    (void)winGravity;
    return 0;
}
#endif

#endif /* _X11WASM_BROWSER_H */
