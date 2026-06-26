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
    Region (*createRegion)(void) = XCreateRegion;
    int (*freeData)(void *) = XFree;

    return (openDisplay != 0) && (closeDisplay != 0)
	    && (createWindow != 0) && (eventsQueued != 0)
	    && (getMotionEvents != 0) && (readBitmapFile != 0)
	    && (readBitmapFileData != 0) && (writeBitmapFile != 0)
	    && (initImage != 0) && (loadQueryFont != 0)
	    && (listFontsWithInfo != 0) && (freeFontInfo != 0)
	    && (getFontPath != 0) && (setFontPath != 0)
	    && (freeFontPath != 0)
	    && (createRegion != 0) && (freeData != 0);
}
