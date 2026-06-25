/*
 * XlibWasm.h --
 *
 *	Function declarations for the Xlib-shaped ABI used by browser clients.
 */

#ifndef _X11WASM_XLIBWASM_H
#define _X11WASM_XLIBWASM_H

#define X11WASM_XLIBWASM_INCLUDES_XUTIL
#include <X11/Xutil.h>
#undef X11WASM_XLIBWASM_INCLUDES_XUTIL

#include <X11Wasm/Events.h>

#ifndef EXTERN
#define EXTERN extern
#endif

#ifndef X11WASM_XAFTERFUNCTION_DEFINED
#define X11WASM_XAFTERFUNCTION_DEFINED
typedef int (*XAfterFunction)(Display *display);
#endif

#ifdef __cplusplus
extern "C" {
#endif

EXTERN int XSetDashes(Display *display, GC gc, int dash_offset,
	_Xconst char *dash_list, int n);
EXTERN XModifierKeymap *XGetModifierMapping(Display *d);
EXTERN XImage *XCreateImage(Display *d, Visual *v, unsigned int ui1,
	int i1, int i2, char *cp, unsigned int ui2, unsigned int ui3,
	int i3, int i4);
EXTERN XImage *XGetImage(Display *d, Drawable dr, int i1, int i2,
	unsigned int ui1, unsigned int ui2, unsigned long ul, int i3);
EXTERN char *XGetAtomName(Display *d, Atom a);
EXTERN char *XKeysymToString(KeySym k);
EXTERN Colormap XCreateColormap(Display *d, Window w, Visual *v, int i);
EXTERN Cursor XCreatePixmapCursor(Display *d, Pixmap p1, Pixmap p2,
	XColor *x1, XColor *x2, unsigned int ui1, unsigned int ui2);
EXTERN Cursor XCreateFontCursor(Display *d, unsigned int ui);
EXTERN Cursor XCreateGlyphCursor(Display *d, Font f1, Font f2,
	unsigned int ui1, unsigned int ui2, XColor _Xconst *x1,
	XColor _Xconst *x2);
EXTERN GContext XGContextFromGC(GC g);
EXTERN int XAddHost(Display *d, XHostAddress *host);
EXTERN int XAddHosts(Display *d, XHostAddress *hosts, int nhosts);
EXTERN int XDisableAccessControl(Display *d);
EXTERN int XEnableAccessControl(Display *d);
EXTERN XHostAddress *XListHosts(Display *d, int *i, Bool *b);
EXTERN int XRemoveHost(Display *d, XHostAddress *host);
EXTERN int XRemoveHosts(Display *d, XHostAddress *hosts, int nhosts);
EXTERN int XSetAccessControl(Display *d, int mode);
EXTERN char **XListExtensions(Display *d, int *i);
EXTERN KeySym XKeycodeToKeysym(Display *d, unsigned int k, int i);
EXTERN KeySym XkbKeycodeToKeysym(Display *d, unsigned int k, int g, int i);
EXTERN KeySym XStringToKeysym(_Xconst char *c);
EXTERN Window XRootWindow(Display *d, int i);
EXTERN int XConnectionNumber(Display *d);
EXTERN int XDefaultScreen(Display *d);
EXTERN Window XDefaultRootWindow(Display *d);
EXTERN Visual *XDefaultVisual(Display *d, int i);
EXTERN GC XDefaultGC(Display *d, int i);
EXTERN unsigned long XBlackPixel(Display *d, int i);
EXTERN unsigned long XWhitePixel(Display *d, int i);
EXTERN int XQLength(Display *d);
EXTERN int XDisplayWidth(Display *d, int i);
EXTERN int XDisplayHeight(Display *d, int i);
EXTERN int XDisplayWidthMM(Display *d, int i);
EXTERN int XDisplayHeightMM(Display *d, int i);
EXTERN int XDisplayPlanes(Display *d, int i);
EXTERN int XDisplayCells(Display *d, int i);
EXTERN int XScreenCount(Display *d);
EXTERN char *XServerVendor(Display *d);
EXTERN int XProtocolVersion(Display *d);
EXTERN int XProtocolRevision(Display *d);
EXTERN int XVendorRelease(Display *d);
EXTERN char *XDisplayString(Display *d);
EXTERN int XDefaultDepth(Display *d, int i);
EXTERN Colormap XDefaultColormap(Display *d, int i);
EXTERN int XBitmapUnit(Display *d);
EXTERN int XBitmapBitOrder(Display *d);
EXTERN int XBitmapPad(Display *d);
EXTERN int XImageByteOrder(Display *d);
EXTERN long XMaxRequestSize(Display *d);
EXTERN long XExtendedMaxRequestSize(Display *d);
EXTERN unsigned long XDisplayMotionBufferSize(Display *d);
EXTERN unsigned long XAllPlanes(void);
EXTERN unsigned long XNextRequest(Display *d);
EXTERN unsigned long XLastKnownRequestProcessed(Display *d);
EXTERN Screen *XScreenOfDisplay(Display *d, int i);
EXTERN Screen *XDefaultScreenOfDisplay(Display *d);
EXTERN Display *XDisplayOfScreen(Screen *s);
EXTERN Window XRootWindowOfScreen(Screen *s);
EXTERN unsigned long XBlackPixelOfScreen(Screen *s);
EXTERN unsigned long XWhitePixelOfScreen(Screen *s);
EXTERN Colormap XDefaultColormapOfScreen(Screen *s);
EXTERN int XDefaultDepthOfScreen(Screen *s);
EXTERN GC XDefaultGCOfScreen(Screen *s);
EXTERN Visual *XDefaultVisualOfScreen(Screen *s);
EXTERN int XWidthOfScreen(Screen *s);
EXTERN int XHeightOfScreen(Screen *s);
EXTERN int XWidthMMOfScreen(Screen *s);
EXTERN int XHeightMMOfScreen(Screen *s);
EXTERN int XPlanesOfScreen(Screen *s);
EXTERN int XCellsOfScreen(Screen *s);
EXTERN int XMinCmapsOfScreen(Screen *s);
EXTERN int XMaxCmapsOfScreen(Screen *s);
EXTERN Bool XDoesSaveUnders(Screen *s);
EXTERN int XDoesBackingStore(Screen *s);
EXTERN long XEventMaskOfScreen(Screen *s);
EXTERN int XScreenNumberOfScreen(Screen *s);
EXTERN Status XInitThreads(void);
EXTERN Status XFreeThreads(void);
EXTERN void XLockDisplay(Display *d);
EXTERN void XUnlockDisplay(Display *d);
EXTERN Bool XSupportsLocale(void);
EXTERN XErrorHandler XSetErrorHandler(XErrorHandler x);
EXTERN XIOErrorHandler XSetIOErrorHandler(XIOErrorHandler x);
EXTERN Status XIconifyWindow(Display *d, Window w, int i);
EXTERN Status XWithdrawWindow(Display *d, Window w, int i);
EXTERN Status XGetClassHint(Display *d, Window w,
	XClassHint *class_hints_return);
EXTERN Status XGetIconSizes(Display *d, Window w,
	XIconSize **size_list_return, int *count_return);
EXTERN XIconSize *XAllocIconSize(void);
EXTERN XStandardColormap *XAllocStandardColormap(void);
EXTERN Status XGetRGBColormaps(Display *d, Window w,
	XStandardColormap **stdcmap_return, int *count_return,
	Atom property);
EXTERN Status XGetStandardColormap(Display *d, Window w,
	XStandardColormap *colormap_return, Atom property);
EXTERN Status XGetNormalHints(Display *d, Window w,
	XSizeHints *hints_return);
EXTERN Status XGetSizeHints(Display *d, Window w,
	XSizeHints *hints_return, Atom property);
EXTERN Status XGetZoomHints(Display *d, Window w,
	XSizeHints *zhints_return);
EXTERN Status XGetTextProperty(Display *d, Window w,
	XTextProperty *text_prop_return, Atom property);
EXTERN Status XGetWMClientMachine(Display *d, Window w,
	XTextProperty *text_prop_return);
EXTERN Status XGetWMColormapWindows(Display *d, Window w, Window **wpp,
	int *ip);
EXTERN XWMHints *XGetWMHints(Display *d, Window w);
EXTERN Status XGetWMIconName(Display *d, Window w,
	XTextProperty *text_prop_return);
EXTERN Status XGetWMName(Display *d, Window w,
	XTextProperty *text_prop_return);
EXTERN Status XGetWMNormalHints(Display *d, Window w,
	XSizeHints *hints_return, long *supplied_return);
EXTERN Status XGetWMSizeHints(Display *d, Window w,
	XSizeHints *hints_return, long *supplied_return, Atom property);
EXTERN Status XAllocColor(Display *d, Colormap c, XColor *xp);
EXTERN Status XAllocNamedColor(Display *d, Colormap c, _Xconst char *name,
	XColor *screen_def_return, XColor *exact_def_return);
	EXTERN int XBell(Display *d, int i);
	EXTERN int XChangeProperty(Display *d, Window w, Atom a1, Atom a2,
		int i1, int i2, _Xconst unsigned char *c, int i3);
EXTERN int XChangeWindowAttributes(Display *d, Window w, unsigned long ul,
	XSetWindowAttributes *x);
EXTERN int XClearArea(Display *d, Window w, int x, int y,
	unsigned int width, unsigned int height, Bool exposures);
EXTERN int XClearWindow(Display *d, Window w);
EXTERN int XConfigureWindow(Display *d, Window w, unsigned int i,
	XWindowChanges *x);
EXTERN int XConvertSelection(Display *d, Atom selection, Atom target,
	Atom property, Window requestor, Time time);
EXTERN int XCopyArea(Display *d, Drawable dr1, Drawable dr2, GC g,
	int i1, int i2, unsigned int ui1, unsigned int ui2, int i3, int i4);
EXTERN int XCopyPlane(Display *d, Drawable dr1, Drawable dr2, GC g,
	int i1, int i2, unsigned int ui1, unsigned int ui2, int i3, int i4,
	unsigned long ul);
EXTERN int XCopyGC(Display *d, GC src, unsigned long valuemask, GC dest);
EXTERN Pixmap XCreateBitmapFromData(Display *display, Drawable d,
	_Xconst char *data, unsigned int width, unsigned int height);
EXTERN int XDefineCursor(Display *d, Window w, Cursor c);
EXTERN int XDeleteProperty(Display *d, Window w, Atom a);
EXTERN int XDestroySubwindows(Display *d, Window w);
EXTERN int XDestroyWindow(Display *d, Window w);
	EXTERN int XDrawArc(Display *d, Drawable dr, GC g, int i1, int i2,
		unsigned int ui1, unsigned int ui2, int i3, int i4);
	EXTERN int XDrawLines(Display *d, Drawable dr, GC g, XPoint *x, int i1,
		int i2);
	EXTERN int XDrawRectangle(Display *d, Drawable dr, GC g, int i1, int i2,
		unsigned int ui1, unsigned int ui2);
	EXTERN Status XFetchName(Display *d, Window w,
		char **window_name_return);
EXTERN Status XGetIconName(Display *d, Window w, char **icon_name_return);
	EXTERN int XFillArc(Display *d, Drawable dr, GC g, int i1, int i2,
		unsigned int ui1, unsigned int ui2, int i3, int i4);
EXTERN int XFillPolygon(Display *d, Drawable dr, GC g, XPoint *x, int i1,
	int i2, int i3);
EXTERN int XFillRectangles(Display *d, Drawable dr, GC g, XRectangle *x,
	int i);
EXTERN int XForceScreenSaver(Display *d, int i);
EXTERN int XFreeColormap(Display *d, Colormap c);
EXTERN int XFreeColors(Display *d, Colormap c, unsigned long *ulp, int i,
	unsigned long ul);
EXTERN int XFreeCursor(Display *d, Cursor c);
EXTERN int XFreeFont(Display *d, XFontStruct *font_struct);
	EXTERN int XFreeFontNames(char **list);
	EXTERN void XFreeExtensionList(char **list);
	EXTERN int XFreeModifiermap(XModifierKeymap *x);
	EXTERN int XFreePixmap(Display *d, Pixmap pixmap);
EXTERN int XGetErrorDatabaseText(Display *d, const char *name,
	const char *message, const char *default_string, char *buffer,
	int length);
EXTERN int XGetErrorText(Display *d, int code, char *buffer, int length);
EXTERN Status XGetGeometry(Display *d, Drawable dr, Window *w, int *i1,
	int *i2, unsigned int *ui1, unsigned int *ui2, unsigned int *ui3,
	unsigned int *ui4);
EXTERN int XGetInputFocus(Display *d, Window *w, int *i);
EXTERN KeySym *XGetKeyboardMapping(Display *d, KeyCode first_keycode,
	int keycode_count, int *keysyms_per_keycode_return);
EXTERN Window XGetSelectionOwner(Display *d, Atom a);
EXTERN int XGetWindowProperty(Display *d, Window w, Atom a1, long l1,
	long l2, Bool b, Atom a2, Atom *ap, int *ip,
	unsigned long *ulp1, unsigned long *ulp2, unsigned char **cpp);
EXTERN Status XGetWindowAttributes(Display *d, Window w,
	XWindowAttributes *x);
EXTERN Bool XGetFontProperty(XFontStruct *font_struct, Atom atom,
	unsigned long *value_return);
EXTERN int XGetGCValues(Display *d, GC gc, unsigned long valuemask,
	XGCValues *values_return);
EXTERN int XGrabKeyboard(Display *d, Window w, Bool b, int i1, int i2,
	Time t);
EXTERN int XGrabPointer(Display *d, Window w1, Bool b, unsigned int ui,
	int i1, int i2, Window w2, Cursor c, Time t);
EXTERN KeyCode XKeysymToKeycode(Display *d, KeySym k);
EXTERN Status XLookupColor(Display *d, Colormap c1, _Xconst char *c2,
	XColor *x1, XColor *x2);
EXTERN KeySym XLookupKeysym(XKeyEvent *key_event, int index);
	EXTERN int XLookupString(XKeyEvent *event_struct, char *buffer_return,
		int bytes_buffer, KeySym *keysym_return, XComposeStatus *status);
	EXTERN Atom *XListProperties(Display *d, Window w,
		int *num_prop_return);
	EXTERN int XMapRaised(Display *d, Window w);
	EXTERN int XMapSubwindows(Display *d, Window w);
	EXTERN int XMapWindow(Display *d, Window w);
EXTERN int XCirculateSubwindows(Display *d, Window w, int i);
EXTERN int XCirculateSubwindowsDown(Display *d, Window w);
EXTERN int XCirculateSubwindowsUp(Display *d, Window w);
EXTERN int XMoveResizeWindow(Display *d, Window w, int i1, int i2,
	unsigned int ui1, unsigned int ui2);
EXTERN int XMoveWindow(Display *d, Window w, int i1, int i2);
EXTERN Bool XCheckIfEvent(Display *d, XEvent *x,
	Bool (*predicate)(Display *display, XEvent *event, XPointer arg),
	XPointer arg);
EXTERN Bool XCheckMaskEvent(Display *d, long l, XEvent *x);
EXTERN Bool XCheckTypedEvent(Display *d, int i, XEvent *x);
EXTERN Bool XCheckTypedWindowEvent(Display *d, Window w, int i, XEvent *x);
EXTERN Bool XCheckWindowEvent(Display *d, Window w, long l, XEvent *x);
EXTERN int XIfEvent(Display *d, XEvent *x,
	Bool (*predicate)(Display *display, XEvent *event, XPointer arg),
	XPointer arg);
EXTERN int XMaskEvent(Display *d, long l, XEvent *x);
EXTERN int XNextEvent(Display *d, XEvent *x);
EXTERN int XPeekEvent(Display *d, XEvent *x);
EXTERN int XPeekIfEvent(Display *d, XEvent *x,
	Bool (*predicate)(Display *display, XEvent *event, XPointer arg),
	XPointer arg);
EXTERN int XPending(Display *d);
EXTERN int XPutBackEvent(Display *d, XEvent *x);
EXTERN int XQueryColors(Display *d, Colormap c, XColor *x, int i);
EXTERN int XQueryColor(Display *d, Colormap c, XColor *def_in_out);
EXTERN Bool XQueryExtension(Display *d, const char *name,
	int *major_opcode_return, int *first_event_return,
	int *first_error_return);
EXTERN Bool XQueryPointer(Display *d, Window w1, Window *w2, Window *w3,
	int *i1, int *i2, int *i3, int *i4, unsigned int *ui);
	EXTERN Status XQueryTree(Display *d, Window w1, Window *w2, Window *w3,
		Window **w4, unsigned int *ui);
	EXTERN XFontStruct *XQueryFont(Display *d, XID font_ID);
	EXTERN int XQueryTextExtents(Display *d, XID font_ID, _Xconst char *string,
		int nchars, int *direction_return, int *font_ascent_return,
		int *font_descent_return, XCharStruct *overall_return);
	EXTERN int XQueryTextExtents16(Display *d, XID font_ID,
		_Xconst XChar2b *string, int nchars, int *direction_return,
		int *font_ascent_return, int *font_descent_return,
		XCharStruct *overall_return);
	EXTERN int XRaiseWindow(Display *d, Window w);
EXTERN int XRefreshKeyboardMapping(XMappingEvent *x);
EXTERN Status XReconfigureWMWindow(Display *d, Window w, int screen,
	unsigned int value_mask, XWindowChanges *values);
EXTERN int XResetScreenSaver(Display *d);
EXTERN int XResizeWindow(Display *d, Window w, unsigned int ui1,
	unsigned int ui2);
EXTERN int XRestackWindows(Display *d, Window *w, int i);
EXTERN int XSelectInput(Display *d, Window w, long l);
EXTERN Status XSendEvent(Display *d, Window w, Bool b, long l, XEvent *x);
EXTERN int XGetScreenSaver(Display *d, int *timeout_return,
	int *interval_return, int *prefer_blanking_return,
	int *allow_exposures_return);
EXTERN int XSetCommand(Display *d, Window w, char **c, int i);
EXTERN Status XGetCommand(Display *d, Window w, char ***argv_return,
	int *argc_return);
EXTERN int XSetClassHint(Display *d, Window w, XClassHint *class_hints);
EXTERN int XSetIconName(Display *d, Window w, _Xconst char *c);
EXTERN int XSetIconSizes(Display *d, Window w, XIconSize *size_list,
	int count);
EXTERN int XSetInputFocus(Display *d, Window w, int i, Time t);
EXTERN int XSetNormalHints(Display *d, Window w, XSizeHints *hints);
EXTERN void XSetRGBColormaps(Display *d, Window w,
	XStandardColormap *stdcmaps, int count, Atom property);
EXTERN int XSetSelectionOwner(Display *d, Atom a, Window w, Time t);
EXTERN int XSetScreenSaver(Display *d, int timeout, int interval,
	int prefer_blanking, int allow_exposures);
EXTERN int XSetSizeHints(Display *d, Window w, XSizeHints *hints,
	Atom property);
EXTERN void XSetStandardColormap(Display *d, Window w,
	XStandardColormap *colormap, Atom property);
EXTERN int XSetStandardProperties(Display *d, Window w,
	_Xconst char *window_name, _Xconst char *icon_name, Pixmap icon_pixmap,
	char **argv, int argc, XSizeHints *hints);
EXTERN void XSetTextProperty(Display *d, Window w, XTextProperty *text_prop,
	Atom property);
EXTERN int XSetTransientForHint(Display *d, Window w, Window prop_window);
EXTERN Status XGetTransientForHint(Display *d, Window w,
	Window *prop_window_return);
EXTERN int XSetZoomHints(Display *d, Window w, XSizeHints *zhints);
EXTERN Status XSetWMColormapWindows(Display *d, Window w,
	Window *colormap_windows, int count);
EXTERN Status XSetWMProtocols(Display *d, Window w, Atom *protocols,
	int count);
EXTERN Status XGetWMProtocols(Display *d, Window w, Atom **protocols_return,
	int *count_return);
EXTERN int XSetWMHints(Display *d, Window w, XWMHints *wm_hints);
EXTERN void XSetWMIconName(Display *d, Window w, XTextProperty *text_prop);
EXTERN void XSetWMName(Display *d, Window w, XTextProperty *text_prop);
EXTERN void XSetWMNormalHints(Display *d, Window w, XSizeHints *hints);
EXTERN void XSetWMProperties(Display *d, Window w,
	XTextProperty *window_name, XTextProperty *icon_name, char **argv,
	int argc, XSizeHints *normal_hints, XWMHints *wm_hints,
	XClassHint *class_hints);
EXTERN void XSetWMSizeHints(Display *d, Window w, XSizeHints *hints,
	Atom property);
EXTERN int XSetWindowBackground(Display *d, Window w, unsigned long ul);
EXTERN int XSetWindowBackgroundPixmap(Display *d, Window w, Pixmap p);
EXTERN int XSetWindowBorder(Display *d, Window w, unsigned long ul);
EXTERN int XSetWindowBorderPixmap(Display *d, Window w, Pixmap p);
EXTERN int XSetWindowBorderWidth(Display *d, Window w, unsigned int ui);
EXTERN int XSetWindowColormap(Display *d, Window w, Colormap c);
EXTERN int XStoreName(Display *d, Window w, _Xconst char *name);
EXTERN Bool XTranslateCoordinates(Display *d, Window w1, Window w2,
	int i1, int i2, int *i3, int *i4, Window *w3);
EXTERN int XUnloadFont(Display *d, Font font);
EXTERN int XUngrabKeyboard(Display *d, Time t);
EXTERN int XUngrabPointer(Display *d, Time t);
EXTERN int XUnmapSubwindows(Display *d, Window w);
EXTERN int XUnmapWindow(Display *d, Window w);
EXTERN int XWindowEvent(Display *d, Window w, long l, XEvent *x);
EXTERN void XDestroyIC(XIC x);
EXTERN Bool XFilterEvent(XEvent *x, Window w);
EXTERN int XmbLookupString(XIC xi, XKeyPressedEvent *xk, char *c, int i,
	KeySym *k, Status *s);
EXTERN int Xutf8LookupString(XIC xi, XKeyPressedEvent *xk, char *c, int i,
	KeySym *k, Status *s);
EXTERN char *XGetICValues(XIC xi, ...);
EXTERN char *XSetICValues(XIC xi, ...);
EXTERN void XSetICFocus(XIC xi);
EXTERN void XUnsetICFocus(XIC xi);
EXTERN XIM XOpenIM(Display *display, XPointer rdb, char *resourceName,
	char *resourceClass);
EXTERN Status XCloseIM(XIM xim);
EXTERN char *XSetLocaleModifiers(_Xconst char *modifierList);
EXTERN Bool XRegisterIMInstantiateCallback(Display *display,
	struct _XrmHashBucketRec *rdb, char *resourceName, char *resourceClass,
	XIDProc callback, XPointer clientData);
EXTERN Bool XUnregisterIMInstantiateCallback(Display *display,
	struct _XrmHashBucketRec *rdb, char *resourceName, char *resourceClass,
	XIDProc callback, XPointer clientData);
EXTERN char *XGetIMValues(XIM xim, ...);
EXTERN char *XSetIMValues(XIM xim, ...);
EXTERN XVaNestedList XVaCreateNestedList(int unused, ...);
EXTERN XFontSet XCreateFontSet(Display *display, _Xconst char *baseFontNameList,
	char ***missingCharsetListReturn, int *missingCharsetCountReturn,
	char **defStringReturn);
EXTERN void XFreeFontSet(Display *display, XFontSet fontSet);
EXTERN XFontSetExtents *XExtentsOfFontSet(XFontSet fontSet);
EXTERN char *XBaseFontNameListOfFontSet(XFontSet fontSet);
EXTERN char *XLocaleOfFontSet(XFontSet fontSet);
EXTERN Bool XContextDependentDrawing(XFontSet fontSet);
EXTERN Bool XDirectionalDependentDrawing(XFontSet fontSet);
EXTERN Bool XContextualDrawing(XFontSet fontSet);
EXTERN int XFontsOfFontSet(XFontSet fontSet,
	XFontStruct ***fontStructListReturn, char ***fontNameListReturn);
EXTERN int XmbTextEscapement(XFontSet fontSet, _Xconst char *text,
	int bytes_text);
EXTERN int XwcTextEscapement(XFontSet fontSet, _Xconst wchar_t *text,
	int num_wchars);
EXTERN int Xutf8TextEscapement(XFontSet fontSet, _Xconst char *text,
	int bytes_text);
EXTERN int XmbTextExtents(XFontSet fontSet, _Xconst char *text,
	int bytes_text, XRectangle *overall_ink_return,
	XRectangle *overall_logical_return);
EXTERN int XwcTextExtents(XFontSet fontSet, _Xconst wchar_t *text,
	int num_wchars, XRectangle *overall_ink_return,
	XRectangle *overall_logical_return);
EXTERN int Xutf8TextExtents(XFontSet fontSet, _Xconst char *text,
	int bytes_text, XRectangle *overall_ink_return,
	XRectangle *overall_logical_return);
EXTERN Status XmbTextPerCharExtents(XFontSet fontSet, _Xconst char *text,
	int bytes_text, XRectangle *ink_extents_buffer,
	XRectangle *logical_extents_buffer, int buffer_size, int *num_chars,
	XRectangle *overall_ink_return, XRectangle *overall_logical_return);
EXTERN Status XwcTextPerCharExtents(XFontSet fontSet, _Xconst wchar_t *text,
	int num_wchars, XRectangle *ink_extents_buffer,
	XRectangle *logical_extents_buffer, int buffer_size, int *num_chars,
	XRectangle *overall_ink_return, XRectangle *overall_logical_return);
EXTERN Status Xutf8TextPerCharExtents(XFontSet fontSet, _Xconst char *text,
	int bytes_text, XRectangle *ink_extents_buffer,
	XRectangle *logical_extents_buffer, int buffer_size, int *num_chars,
	XRectangle *overall_ink_return, XRectangle *overall_logical_return);
EXTERN void XmbDrawText(Display *display, Drawable d, GC gc, int x, int y,
	XmbTextItem *text_items, int nitems);
EXTERN void XwcDrawText(Display *display, Drawable d, GC gc, int x, int y,
	XwcTextItem *text_items, int nitems);
EXTERN void Xutf8DrawText(Display *display, Drawable d, GC gc, int x, int y,
	XmbTextItem *text_items, int nitems);
EXTERN void XmbDrawString(Display *display, Drawable d, XFontSet fontSet,
	GC gc, int x, int y, _Xconst char *text, int bytes_text);
EXTERN void XwcDrawString(Display *display, Drawable d, XFontSet fontSet,
	GC gc, int x, int y, _Xconst wchar_t *text, int num_wchars);
EXTERN void Xutf8DrawString(Display *display, Drawable d, XFontSet fontSet,
	GC gc, int x, int y, _Xconst char *text, int bytes_text);
EXTERN void XmbDrawImageString(Display *display, Drawable d,
	XFontSet fontSet, GC gc, int x, int y, _Xconst char *text,
	int bytes_text);
EXTERN void XwcDrawImageString(Display *display, Drawable d,
	XFontSet fontSet, GC gc, int x, int y, _Xconst wchar_t *text,
	int num_wchars);
EXTERN void Xutf8DrawImageString(Display *display, Drawable d,
	XFontSet fontSet, GC gc, int x, int y, _Xconst char *text,
	int bytes_text);
EXTERN void XFreeStringList(char **list);
EXTERN void XwcFreeStringList(wchar_t **list);
EXTERN Status XTextPropertyToStringList(XTextProperty *text_prop,
	char ***list_return, int *count_return);
EXTERN int XmbTextListToTextProperty(Display *display, char **list,
	int count, XICCEncodingStyle style, XTextProperty *text_prop_return);
EXTERN int XmbTextPropertyToTextList(Display *display,
	const XTextProperty *text_prop, char ***list_return, int *count_return);
EXTERN int XwcTextListToTextProperty(Display *display, wchar_t **list,
	int count, XICCEncodingStyle style, XTextProperty *text_prop_return);
EXTERN int XwcTextPropertyToTextList(Display *display,
	const XTextProperty *text_prop, wchar_t ***list_return,
	int *count_return);
EXTERN void XmbSetWMProperties(Display *display, Window w,
	_Xconst char *window_name, _Xconst char *icon_name, char **argv,
	int argc, XSizeHints *normal_hints, XWMHints *wm_hints,
	XClassHint *class_hints);
EXTERN int Xutf8TextListToTextProperty(Display *display, char **list,
	int count, XICCEncodingStyle style, XTextProperty *text_prop_return);
EXTERN int Xutf8TextPropertyToTextList(Display *display,
	const XTextProperty *text_prop, char ***list_return, int *count_return);
EXTERN void Xutf8SetWMProperties(Display *display, Window w,
	_Xconst char *window_name, _Xconst char *icon_name, char **argv,
	int argc, XSizeHints *normal_hints, XWMHints *wm_hints,
	XClassHint *class_hints);
EXTERN int X11WasmSetClipboardText(_Xconst char *utf8, int utf8Length);
EXTERN int X11WasmSetWindowMetadataTarget(Display *display, Window window,
	Window target, int isRootToplevel);
EXTERN void X11WasmSetPixmapColormap(Pixmap pixmap, Colormap colormap);
EXTERN int XSetClipRectangles(Display *display, GC gc, int clip_x_origin,
	int clip_y_origin, XRectangle rectangles[], int n, int ordering);
EXTERN Status XParseColor(Display *display, Colormap map, _Xconst char *spec,
	XColor *colorPtr);
EXTERN GC XCreateGC(Display *display, Drawable d, unsigned long valuemask,
	XGCValues *values);
EXTERN int XFreeGC(Display *display, GC gc);
EXTERN void XFlushGC(Display *display, GC gc);
EXTERN Atom XInternAtom(Display *display, _Xconst char *atom_name,
	Bool only_if_exists);
EXTERN Status XInternAtoms(Display *display, char **names, int count,
	Bool only_if_exists, Atom *atoms_return);
EXTERN Status XGetAtomNames(Display *display, Atom *atoms, int count,
	char **names_return);
EXTERN int XSetBackground(Display *display, GC gc, unsigned long foreground);
EXTERN int XSetForeground(Display *display, GC gc, unsigned long foreground);
EXTERN int XSetClipMask(Display *display, GC gc, Pixmap pixmap);
EXTERN int XSetClipOrigin(Display *display, GC gc, int clip_x_origin,
	int clip_y_origin);
EXTERN int XSetTSOrigin(Display *display, GC gc, int ts_x_origin,
	int ts_y_origin);
EXTERN int XSetGraphicsExposures(Display *display, GC gc,
	Bool graphics_exposures);
EXTERN int XSetPlaneMask(Display *display, GC gc, unsigned long plane_mask);
EXTERN int XSetState(Display *display, GC gc, unsigned long foreground,
	unsigned long background, int function, unsigned long plane_mask);
EXTERN int XSetSubwindowMode(Display *display, GC gc, int subwindow_mode);
EXTERN int XChangeGC(Display *d, GC gc, unsigned long mask,
	XGCValues *values);
EXTERN int XSetFont(Display *display, GC gc, Font font);
EXTERN int XSetArcMode(Display *display, GC gc, int arc_mode);
EXTERN int XSetTile(Display *display, GC gc, Pixmap tile);
EXTERN int XSetStipple(Display *display, GC gc, Pixmap stipple);
EXTERN int XSetFillRule(Display *display, GC gc, int fill_rule);
EXTERN int XSetFillStyle(Display *display, GC gc, int fill_style);
EXTERN int XSetFunction(Display *display, GC gc, int function);
EXTERN int XSetLineAttributes(Display *display, GC gc,
	unsigned int line_width, int line_style, int cap_style, int join_style);
EXTERN int _XInitImageFuncPtrs(XImage *image);
EXTERN XIC XCreateIC(XIM xim, ...);
EXTERN XVisualInfo *XGetVisualInfo(Display *display, long vinfo_mask,
	XVisualInfo *vinfo_template, int *nitems_return);
EXTERN XPixmapFormatValues *XListPixmapFormats(Display *display,
	int *count_return);
EXTERN int *XListDepths(Display *display, int screen_number,
	int *count_return);
EXTERN Status XMatchVisualInfo(Display *display, int screen, int depth,
	int c_class, XVisualInfo *vinfo_return);
EXTERN void XSetWMClientMachine(Display *display, Window w,
	XTextProperty *text_prop);
EXTERN Status XStringListToTextProperty(char **list, int count,
	XTextProperty *text_prop_return);
EXTERN int XDrawLine(Display *d, Drawable dr, GC g, int x1, int y1,
	int x2, int y2);
EXTERN int XWarpPointer(Display *d, Window s, Window dw, int sx, int sy,
	unsigned int sw, unsigned int sh, int dx, int dy);
EXTERN int XFillRectangle(Display *display, Drawable d, GC gc, int x, int y,
	unsigned int width, unsigned int height);
EXTERN int XFlush(Display *display);
EXTERN int XGrabServer(Display *display);
EXTERN int XUngrabServer(Display *display);
EXTERN int XFree(void *data);
EXTERN int XNoOp(Display *display);
EXTERN XAfterFunction XSetAfterFunction(Display *display,
	XAfterFunction procedure);
EXTERN XAfterFunction XSynchronize(Display *display, Bool onoff);
EXTERN int XSync(Display *display, Bool discard);
EXTERN VisualID XVisualIDFromVisual(Visual *visual);
EXTERN Bool XPointInRegion(Region rgn, int x, int y);
EXTERN Region XPolygonRegion(XPoint *points, int n, int fill_rule);
EXTERN Bool XEqualRegion(Region rgn1, Region rgn2);
EXTERN int XOffsetRegion(Region rgn, int dx, int dy);
EXTERN int XShrinkRegion(Region rgn, int dx, int dy);
EXTERN int XUnionRegion(Region srca, Region srcb, Region dr_return);
EXTERN int XXorRegion(Region srca, Region srcb, Region dr_return);
EXTERN Window XCreateWindow(Display *display, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	int depth, unsigned int clazz, Visual *visual, unsigned long value_mask,
	XSetWindowAttributes *attributes);
EXTERN Window XCreateSimpleWindow(Display *display, Window parent, int x,
	int y, unsigned int width, unsigned int height,
	unsigned int border_width, unsigned long border,
	unsigned long background);
EXTERN Pixmap XCreatePixmap(Display *display, Drawable d, unsigned int width,
	unsigned int height, unsigned int depth);
EXTERN int XLowerWindow(Display *d, Window w);
EXTERN int XFillArcs(Display *d, Drawable dr, GC gc, XArc *a, int n);
EXTERN int XDrawArcs(Display *d, Drawable dr, GC gc, XArc *a, int n);
EXTERN int XDrawRectangles(Display *d, Drawable dr, GC gc, XRectangle *r,
	int n);
EXTERN int XDrawSegments(Display *d, Drawable dr, GC gc, XSegment *s, int n);
EXTERN int XDrawPoint(Display *d, Drawable dr, GC gc, int x, int y);
EXTERN int XDrawPoints(Display *d, Drawable dr, GC gc, XPoint *p, int n,
	int m);
EXTERN int XReparentWindow(Display *d, Window w, Window p, int x, int y);
EXTERN int XPutImage(Display *d, Drawable dr, GC gc, XImage *im, int sx,
	int sy, int dx, int dy, unsigned int w, unsigned int h);

EXTERN Display *XOpenDisplay(_Xconst char *display_name);
EXTERN char *XDisplayName(_Xconst char *display_name);
EXTERN char *XGetDefault(Display *display, _Xconst char *program,
	_Xconst char *option);
EXTERN char *XResourceManagerString(Display *display);
EXTERN const char *XDefaultString(void);
EXTERN void XrmInitialize(void);
EXTERN int XCloseDisplay(Display *display);
EXTERN int XEventsQueued(Display *display, int mode);
EXTERN int XDisplayKeycodes(Display *display, int *min_keycodes_return,
	int *max_keycodes_return);
EXTERN int XAutoRepeatOff(Display *display);
EXTERN int XAutoRepeatOn(Display *display);
EXTERN int XChangeKeyboardControl(Display *display, unsigned long value_mask,
	XKeyboardControl *values);
EXTERN int XGetKeyboardControl(Display *display,
	XKeyboardState *values_return);
EXTERN int XQueryKeymap(Display *display, char keys_return[32]);
EXTERN Font XLoadFont(Display *display, _Xconst char *name);
EXTERN XFontStruct *XLoadQueryFont(Display *display, _Xconst char *name);
EXTERN char **XListFonts(Display *display, _Xconst char *pattern,
	int maxnames, int *actual_count_return);
EXTERN int XTextWidth(XFontStruct *font_struct, _Xconst char *string,
	int count);
	EXTERN int XTextWidth16(XFontStruct *font_struct, _Xconst XChar2b *string,
		int count);
	EXTERN int XTextExtents(XFontStruct *font_struct, _Xconst char *string,
		int nchars, int *direction_return, int *font_ascent_return,
		int *font_descent_return, XCharStruct *overall_return);
	EXTERN int XTextExtents16(XFontStruct *font_struct,
		_Xconst XChar2b *string, int nchars, int *direction_return,
		int *font_ascent_return, int *font_descent_return,
		XCharStruct *overall_return);
	EXTERN int XDrawString(Display *display, Drawable d, GC gc, int x, int y,
		_Xconst char *string, int length);
	EXTERN int XDrawString16(Display *display, Drawable d, GC gc, int x, int y,
		_Xconst XChar2b *string, int length);
EXTERN int XDrawImageString(Display *display, Drawable d, GC gc, int x, int y,
	_Xconst char *string, int length);
EXTERN int XDrawImageString16(Display *display, Drawable d, GC gc, int x,
	int y, _Xconst XChar2b *string, int length);
	EXTERN int XUndefineCursor(Display *display, Window w);

#ifdef __cplusplus
}
#endif

#endif /* _X11WASM_XLIBWASM_H */
