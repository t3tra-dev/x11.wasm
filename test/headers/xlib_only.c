/*
 * xlib_only.c --
 *
 *	Compile-only check that <X11/Xlib.h> exports the browser Xlib ABI.
 */

#include <X11/Xlib.h>

int
X11WasmHeaderXlibOnlyPrototypeTest(void)
{
    Display *(*openDisplay)(_Xconst char *) = XOpenDisplay;
    int (*closeDisplay)(Display *) = XCloseDisplay;
    Window (*createWindow)(Display *, Window, int, int, unsigned int,
	    unsigned int, unsigned int, int, unsigned int, Visual *,
	    unsigned long, XSetWindowAttributes *) = XCreateWindow;
    int (*eventsQueued)(Display *, int) = XEventsQueued;
    int (*displayKeycodes)(Display *, int *, int *) = XDisplayKeycodes;
    int (*changeKeyboardControl)(Display *, unsigned long,
	    XKeyboardControl *) = XChangeKeyboardControl;
    int (*getKeyboardControl)(Display *, XKeyboardState *) =
	    XGetKeyboardControl;
    int (*autoRepeatOff)(Display *) = XAutoRepeatOff;
    int (*autoRepeatOn)(Display *) = XAutoRepeatOn;
    int (*queryKeymap)(Display *, char[32]) = XQueryKeymap;
    int (*changePointerControl)(Display *, Bool, Bool, int, int, int) =
	    XChangePointerControl;
    int (*getPointerControl)(Display *, int *, int *, int *) =
	    XGetPointerControl;
    int (*getPointerMapping)(Display *, unsigned char *, int) =
	    XGetPointerMapping;
    int (*setPointerMapping)(Display *, _Xconst unsigned char *, int) =
	    XSetPointerMapping;
    XTimeCoord *(*getMotionEvents)(Display *, Window, Time, Time, int *) =
	    XGetMotionEvents;
    int (*readBitmapFile)(Display *, Drawable, _Xconst char *,
	    unsigned int *, unsigned int *, Pixmap *, int *, int *) =
	    XReadBitmapFile;
    int (*readBitmapFileData)(_Xconst char *, unsigned int *, unsigned int *,
	    unsigned char **, int *, int *) = XReadBitmapFileData;
    int (*writeBitmapFile)(Display *, _Xconst char *, Pixmap, unsigned int,
	    unsigned int, int, int) = XWriteBitmapFile;
    Status (*initImage)(XImage *) = XInitImage;
    XFontStruct *(*loadQueryFont)(Display *, _Xconst char *) =
	    XLoadQueryFont;
    char **(*listFontsWithInfo)(Display *, _Xconst char *, int, int *,
	    XFontStruct **) = XListFontsWithInfo;
    int (*freeFontInfo)(char **, XFontStruct *, int) = XFreeFontInfo;
    char **(*getFontPath)(Display *, int *) = XGetFontPath;
    int (*setFontPath)(Display *, char **, int) = XSetFontPath;
    int (*freeFontPath)(char **) = XFreeFontPath;
    Display *(*displayOfIM)(XIM) = XDisplayOfIM;
    char *(*localeOfIM)(XIM) = XLocaleOfIM;
    XIM (*imOfIC)(XIC) = XIMOfIC;
    wchar_t *(*wcResetIC)(XIC) = XwcResetIC;
    char *(*mbResetIC)(XIC) = XmbResetIC;
    char *(*utf8ResetIC)(XIC) = Xutf8ResetIC;
    int (*wcLookupString)(XIC, XKeyPressedEvent *, wchar_t *, int, KeySym *,
	    Status *) = XwcLookupString;
    Region (*createRegion)(void) = XCreateRegion;
    int (*freeData)(void *) = XFree;

    return (openDisplay != 0) && (closeDisplay != 0)
	    && (createWindow != 0) && (eventsQueued != 0)
	    && (displayKeycodes != 0) && (changeKeyboardControl != 0)
	    && (getKeyboardControl != 0) && (autoRepeatOff != 0)
	    && (autoRepeatOn != 0) && (queryKeymap != 0)
	    && (changePointerControl != 0) && (getPointerControl != 0)
	    && (getPointerMapping != 0) && (setPointerMapping != 0)
	    && (getMotionEvents != 0) && (readBitmapFile != 0)
	    && (readBitmapFileData != 0) && (writeBitmapFile != 0)
	    && (initImage != 0) && (loadQueryFont != 0)
	    && (listFontsWithInfo != 0) && (freeFontInfo != 0)
	    && (getFontPath != 0) && (setFontPath != 0)
	    && (freeFontPath != 0)
	    && (displayOfIM != 0) && (localeOfIM != 0)
	    && (imOfIC != 0) && (wcResetIC != 0) && (mbResetIC != 0)
	    && (utf8ResetIC != 0) && (wcLookupString != 0)
	    && (createRegion != 0) && (freeData != 0);
}
