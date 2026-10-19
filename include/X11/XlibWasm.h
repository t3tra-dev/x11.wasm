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
#include <X11/Xresource.h>

#include <X11Wasm/Events.h>

#ifndef X11WASM_XAFTERFUNCTION_DEFINED
#define X11WASM_XAFTERFUNCTION_DEFINED
typedef int (*XAfterFunction)(Display *display);
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Exact browser geometry synchronization; size hints belong to manager actions. */
extern int X11WasmConfigureWindow(unsigned int window, int x, int y,
	unsigned int width, unsigned int height, unsigned int valueMask);
extern unsigned int X11WasmManagerWindowIdentity(unsigned int window, int field);
extern int X11WasmManagerCloseWindow(unsigned int window, unsigned int time);
/* Identity fields: native root frame, metadata client, WM_DELETE_WINDOW owner. */
#define X11_WASM_MANAGER_FRAME 0
#define X11_WASM_MANAGER_METADATA 1
#define X11_WASM_MANAGER_PROTOCOL 2
/* Normal removes only maximized atoms; iconic preserves them; zoomed adds them. */
#define X11_WASM_MANAGER_NORMAL 0
#define X11_WASM_MANAGER_ICONIC 1
#define X11_WASM_MANAGER_ZOOMED 2
/* WM actions return zero on refusal, nonzero on acceptance. Configure rejects
 * invalid hints without snapping; SetWindowState never changes geometry. */
extern int X11WasmManagerConfigureWindow(unsigned int window, int x, int y,
	unsigned int width, unsigned int height, unsigned int valueMask);
extern int X11WasmManagerActivateWindow(unsigned int window, unsigned int time);
extern int X11WasmManagerSetWindowState(unsigned int window, int state,
	unsigned int time);

extern int XSetDashes(Display *display, GC gc, int dash_offset,
	_Xconst char *dash_list, int n);
extern XModifierKeymap *XDeleteModifiermapEntry(XModifierKeymap *x,
	KeyCode k, int i);
extern XModifierKeymap *XGetModifierMapping(Display *d);
extern XModifierKeymap *XInsertModifiermapEntry(XModifierKeymap *x,
	KeyCode k, int i);
extern XModifierKeymap *XNewModifiermap(int i);
extern XImage *XCreateImage(Display *d, Visual *v, unsigned int ui1,
	int i1, int i2, char *cp, unsigned int ui2, unsigned int ui3,
	int i3, int i4);
extern XImage *XGetImage(Display *d, Drawable dr, int i1, int i2,
	unsigned int ui1, unsigned int ui2, unsigned long ul, int i3);
extern XImage *XGetSubImage(Display *d, Drawable dr, int i1, int i2,
	unsigned int ui1, unsigned int ui2, unsigned long ul, int i3,
	XImage *x, int i4, int i5);
extern char *XGetAtomName(Display *d, Atom a);
extern char *XKeysymToString(KeySym k);
extern Colormap XCopyColormapAndFree(Display *d, Colormap c);
extern Colormap XCreateColormap(Display *d, Window w, Visual *v, int i);
extern Cursor XCreatePixmapCursor(Display *d, Pixmap p1, Pixmap p2,
	XColor *x1, XColor *x2, unsigned int ui1, unsigned int ui2);
extern Cursor XCreateFontCursor(Display *d, unsigned int ui);
extern Cursor XCreateGlyphCursor(Display *d, Font f1, Font f2,
	unsigned int ui1, unsigned int ui2, XColor _Xconst *x1,
	XColor _Xconst *x2);
extern GContext XGContextFromGC(GC g);
extern int XAddHost(Display *d, XHostAddress *host);
extern int XAddHosts(Display *d, XHostAddress *hosts, int nhosts);
extern int XAddToSaveSet(Display *d, Window w);
extern int XAllowEvents(Display *d, int i, Time t);
extern int XDisableAccessControl(Display *d);
extern int XEnableAccessControl(Display *d);
extern XHostAddress *XListHosts(Display *d, int *i, Bool *b);
extern int XRemoveHost(Display *d, XHostAddress *host);
extern int XRemoveHosts(Display *d, XHostAddress *hosts, int nhosts);
extern int XSetAccessControl(Display *d, int mode);
extern char **XListExtensions(Display *d, int *i);
extern Colormap *XListInstalledColormaps(Display *d, Window w, int *i);
extern KeySym XKeycodeToKeysym(Display *d, unsigned int k, int i);
extern KeySym XkbKeycodeToKeysym(Display *d,
#if NeedWidePrototypes
	unsigned int k,
#else
	KeyCode k,
#endif
	int g, int i);
extern KeySym XStringToKeysym(_Xconst char *c);
extern Window XRootWindow(Display *d, int i);
extern int XConnectionNumber(Display *d);
extern int XDefaultScreen(Display *d);
extern Window XDefaultRootWindow(Display *d);
extern Visual *XDefaultVisual(Display *d, int i);
extern GC XDefaultGC(Display *d, int i);
extern unsigned long XBlackPixel(Display *d, int i);
extern unsigned long XWhitePixel(Display *d, int i);
extern int XQLength(Display *d);
extern int XDisplayWidth(Display *d, int i);
extern int XDisplayHeight(Display *d, int i);
extern int XDisplayWidthMM(Display *d, int i);
extern int XDisplayHeightMM(Display *d, int i);
extern int XDisplayPlanes(Display *d, int i);
extern int XDisplayCells(Display *d, int i);
extern int XScreenCount(Display *d);
extern char *XServerVendor(Display *d);
extern int XProtocolVersion(Display *d);
extern int XProtocolRevision(Display *d);
extern int XVendorRelease(Display *d);
extern char *XDisplayString(Display *d);
extern int XDefaultDepth(Display *d, int i);
extern Colormap XDefaultColormap(Display *d, int i);
extern int XBitmapUnit(Display *d);
extern int XBitmapBitOrder(Display *d);
extern int XBitmapPad(Display *d);
extern int XImageByteOrder(Display *d);
extern long XMaxRequestSize(Display *d);
extern long XExtendedMaxRequestSize(Display *d);
extern unsigned long XDisplayMotionBufferSize(Display *d);
extern unsigned long XAllPlanes(void);
extern unsigned long XNextRequest(Display *d);
extern unsigned long XLastKnownRequestProcessed(Display *d);
extern Screen *XScreenOfDisplay(Display *d, int i);
extern Screen *XDefaultScreenOfDisplay(Display *d);
extern Display *XDisplayOfScreen(Screen *s);
extern Window XRootWindowOfScreen(Screen *s);
extern unsigned long XBlackPixelOfScreen(Screen *s);
extern unsigned long XWhitePixelOfScreen(Screen *s);
extern Colormap XDefaultColormapOfScreen(Screen *s);
extern int XDefaultDepthOfScreen(Screen *s);
extern GC XDefaultGCOfScreen(Screen *s);
extern Visual *XDefaultVisualOfScreen(Screen *s);
extern int XWidthOfScreen(Screen *s);
extern int XHeightOfScreen(Screen *s);
extern int XWidthMMOfScreen(Screen *s);
extern int XHeightMMOfScreen(Screen *s);
extern int XPlanesOfScreen(Screen *s);
extern int XCellsOfScreen(Screen *s);
extern int XMinCmapsOfScreen(Screen *s);
extern int XMaxCmapsOfScreen(Screen *s);
extern Bool XDoesSaveUnders(Screen *s);
extern int XDoesBackingStore(Screen *s);
extern long XEventMaskOfScreen(Screen *s);
extern int XScreenNumberOfScreen(Screen *s);
extern Status XInitThreads(void);
extern Status XFreeThreads(void);
extern void XLockDisplay(Display *d);
extern void XUnlockDisplay(Display *d);
extern Bool XSupportsLocale(void);
extern XErrorHandler XSetErrorHandler(XErrorHandler x);
extern XIOErrorHandler XSetIOErrorHandler(XIOErrorHandler x);
extern Status XIconifyWindow(Display *d, Window w, int i);
extern Status XWithdrawWindow(Display *d, Window w, int i);
extern Status XGetClassHint(Display *d, Window w,
	XClassHint *class_hints_return);
extern Status XGetIconSizes(Display *d, Window w,
	XIconSize **size_list_return, int *count_return);
extern XIconSize *XAllocIconSize(void);
extern XStandardColormap *XAllocStandardColormap(void);
extern Status XGetRGBColormaps(Display *d, Window w,
	XStandardColormap **stdcmap_return, int *count_return,
	Atom property);
extern Status XGetStandardColormap(Display *d, Window w,
	XStandardColormap *colormap_return, Atom property);
extern Status XGetNormalHints(Display *d, Window w,
	XSizeHints *hints_return);
extern Status XGetSizeHints(Display *d, Window w,
	XSizeHints *hints_return, Atom property);
extern Status XGetZoomHints(Display *d, Window w,
	XSizeHints *zhints_return);
extern Status XGetTextProperty(Display *d, Window w,
	XTextProperty *text_prop_return, Atom property);
extern Status XGetWMClientMachine(Display *d, Window w,
	XTextProperty *text_prop_return);
extern Status XGetWMColormapWindows(Display *d, Window w, Window **wpp,
	int *ip);
extern XWMHints *XGetWMHints(Display *d, Window w);
extern Status XGetWMIconName(Display *d, Window w,
	XTextProperty *text_prop_return);
extern Status XGetWMName(Display *d, Window w,
	XTextProperty *text_prop_return);
extern Status XGetWMNormalHints(Display *d, Window w,
	XSizeHints *hints_return, long *supplied_return);
extern Status XGetWMSizeHints(Display *d, Window w,
	XSizeHints *hints_return, long *supplied_return, Atom property);
extern Status XAllocColor(Display *d, Colormap c, XColor *xp);
extern Status XAllocColorCells(Display *d, Colormap c, Bool contig,
	unsigned long *plane_masks_return, unsigned int nplanes,
	unsigned long *pixels_return, unsigned int npixels);
extern Status XAllocNamedColor(Display *d, Colormap c, _Xconst char *name,
	XColor *screen_def_return, XColor *exact_def_return);
extern Status XAllocColorPlanes(Display *d, Colormap c, Bool contig,
	unsigned long *pixels_return, int ncolors, int nreds, int ngreens,
	int nblues, unsigned long *rmask_return,
	unsigned long *gmask_return, unsigned long *bmask_return);
	extern int XBell(Display *d, int i);
extern int XChangeActivePointerGrab(Display *d, unsigned int ui,
	Cursor c, Time t);
	extern int XChangeProperty(Display *d, Window w, Atom a1, Atom a2,
		int i1, int i2, _Xconst unsigned char *c, int i3);
extern int XChangeWindowAttributes(Display *d, Window w, unsigned long ul,
	XSetWindowAttributes *x);
extern int XClearArea(Display *d, Window w, int x, int y,
	unsigned int width, unsigned int height, Bool exposures);
extern int XClearWindow(Display *d, Window w);
extern int XConfigureWindow(Display *d, Window w, unsigned int i,
	XWindowChanges *x);
extern int XConvertSelection(Display *d, Atom selection, Atom target,
	Atom property, Window requestor, Time time);
extern int XCopyArea(Display *d, Drawable dr1, Drawable dr2, GC g,
	int i1, int i2, unsigned int ui1, unsigned int ui2, int i3, int i4);
extern int XCopyPlane(Display *d, Drawable dr1, Drawable dr2, GC g,
	int i1, int i2, unsigned int ui1, unsigned int ui2, int i3, int i4,
	unsigned long ul);
extern int XCopyGC(Display *d, GC src, unsigned long valuemask, GC dest);
extern int XChangeSaveSet(Display *d, Window w, int i);
extern Pixmap XCreateBitmapFromData(Display *display, Drawable d,
	_Xconst char *data, unsigned int width, unsigned int height);
extern Pixmap XCreatePixmapFromBitmapData(Display *display, Drawable d,
	char *data, unsigned int width, unsigned int height,
	unsigned long fg, unsigned long bg, unsigned int depth);
extern int XDefineCursor(Display *d, Window w, Cursor c);
extern int XDeleteProperty(Display *d, Window w, Atom a);
extern char *XFetchBuffer(Display *d, int *i, int i2);
extern char *XFetchBytes(Display *d, int *i);
extern int XDestroySubwindows(Display *d, Window w);
extern int XDestroyWindow(Display *d, Window w);
	extern int XDrawArc(Display *d, Drawable dr, GC g, int i1, int i2,
		unsigned int ui1, unsigned int ui2, int i3, int i4);
	extern int XDrawLines(Display *d, Drawable dr, GC g, XPoint *x, int i1,
		int i2);
	extern int XDrawRectangle(Display *d, Drawable dr, GC g, int i1, int i2,
		unsigned int ui1, unsigned int ui2);
	extern Status XFetchName(Display *d, Window w,
		char **window_name_return);
extern Status XGetIconName(Display *d, Window w, char **icon_name_return);
	extern int XFillArc(Display *d, Drawable dr, GC g, int i1, int i2,
		unsigned int ui1, unsigned int ui2, int i3, int i4);
extern int XFillPolygon(Display *d, Drawable dr, GC g, XPoint *x, int i1,
	int i2, int i3);
extern int XFillRectangles(Display *d, Drawable dr, GC g, XRectangle *x,
	int i);
extern int XForceScreenSaver(Display *d, int i);
extern int XFreeColormap(Display *d, Colormap c);
extern int XFreeColors(Display *d, Colormap c, unsigned long *ulp, int i,
	unsigned long ul);
extern int XFreeCursor(Display *d, Cursor c);
extern int XRecolorCursor(Display *d, Cursor c, XColor *x1, XColor *x2);
extern int XFreeFont(Display *d, XFontStruct *font_struct);
extern int XFreeFontInfo(char **names, XFontStruct *free_info,
	int actual_count);
extern int XFreeFontPath(char **list);
	extern int XFreeFontNames(char **list);
	extern void XFreeExtensionList(char **list);
	extern int XFreeModifiermap(XModifierKeymap *x);
	extern int XFreePixmap(Display *d, Pixmap pixmap);
extern int XGetErrorDatabaseText(Display *d, const char *name,
	const char *message, const char *default_string, char *buffer,
	int length);
extern int XGetErrorText(Display *d, int code, char *buffer, int length);
extern Status XGetGeometry(Display *d, Drawable dr, Window *w, int *i1,
	int *i2, unsigned int *ui1, unsigned int *ui2, unsigned int *ui3,
	unsigned int *ui4);
extern int XGetInputFocus(Display *d, Window *w, int *i);
extern int XChangeKeyboardMapping(Display *d, int first_keycode,
	int keysyms_per_keycode, KeySym *keysyms, int num_codes);
extern KeySym *XGetKeyboardMapping(Display *d, KeyCode first_keycode,
	int keycode_count, int *keysyms_per_keycode_return);
extern Window XGetSelectionOwner(Display *d, Atom a);
extern int XGetWindowProperty(Display *d, Window w, Atom a1, long l1,
	long l2, Bool b, Atom a2, Atom *ap, int *ip,
	unsigned long *ulp1, unsigned long *ulp2, unsigned char **cpp);
extern Status XGetWindowAttributes(Display *d, Window w,
	XWindowAttributes *x);
extern Bool XGetFontProperty(XFontStruct *font_struct, Atom atom,
	unsigned long *value_return);
extern int XGetGCValues(Display *d, GC gc, unsigned long valuemask,
	XGCValues *values_return);
extern int XGrabButton(Display *d, unsigned int ui1, unsigned int ui2,
	Window w1, Bool b, unsigned int ui3, int i1, int i2, Window w2,
	Cursor c);
extern int XGrabKey(Display *d, int i1, unsigned int ui, Window w, Bool b,
	int i2, int i3);
extern int XGrabKeyboard(Display *d, Window w, Bool b, int i1, int i2,
	Time t);
extern int XGrabPointer(Display *d, Window w1, Bool b, unsigned int ui,
	int i1, int i2, Window w2, Cursor c, Time t);
extern int XKillClient(Display *d, XID x);
extern KeyCode XKeysymToKeycode(Display *d, KeySym k);
extern Status XLookupColor(Display *d, Colormap c1, _Xconst char *c2,
	XColor *x1, XColor *x2);
extern KeySym XLookupKeysym(XKeyEvent *key_event, int index);
	extern int XLookupString(XKeyEvent *event_struct, char *buffer_return,
		int bytes_buffer, KeySym *keysym_return, XComposeStatus *status);
	extern Atom *XListProperties(Display *d, Window w,
		int *num_prop_return);
	extern int XMapRaised(Display *d, Window w);
	extern int XMapSubwindows(Display *d, Window w);
	extern int XMapWindow(Display *d, Window w);
extern int XInstallColormap(Display *d, Colormap c);
extern int XCirculateSubwindows(Display *d, Window w, int i);
extern int XCirculateSubwindowsDown(Display *d, Window w);
extern int XCirculateSubwindowsUp(Display *d, Window w);
extern int XMoveResizeWindow(Display *d, Window w, int i1, int i2,
	unsigned int ui1, unsigned int ui2);
extern int XMoveWindow(Display *d, Window w, int i1, int i2);
extern Bool XCheckIfEvent(Display *d, XEvent *x,
	Bool (*predicate)(Display *display, XEvent *event, XPointer arg),
	XPointer arg);
extern Bool XCheckMaskEvent(Display *d, long l, XEvent *x);
extern Bool XCheckTypedEvent(Display *d, int i, XEvent *x);
extern Bool XCheckTypedWindowEvent(Display *d, Window w, int i, XEvent *x);
extern Bool XCheckWindowEvent(Display *d, Window w, long l, XEvent *x);
extern int XIfEvent(Display *d, XEvent *x,
	Bool (*predicate)(Display *display, XEvent *event, XPointer arg),
	XPointer arg);
extern int XMaskEvent(Display *d, long l, XEvent *x);
extern int XNextEvent(Display *d, XEvent *x);
extern int XPeekEvent(Display *d, XEvent *x);
extern int XPeekIfEvent(Display *d, XEvent *x,
	Bool (*predicate)(Display *display, XEvent *event, XPointer arg),
	XPointer arg);
extern int XPending(Display *d);
extern int XPutBackEvent(Display *d, XEvent *x);
extern int XQueryColors(Display *d, Colormap c, XColor *x, int i);
extern int XQueryColor(Display *d, Colormap c, XColor *def_in_out);
extern Bool XQueryExtension(Display *d, const char *name,
	int *major_opcode_return, int *first_event_return,
	int *first_error_return);
extern Status XQueryBestCursor(Display *d, Drawable dr, unsigned int ui1,
	unsigned int ui2, unsigned int *ui3, unsigned int *ui4);
extern Status XQueryBestSize(Display *d, int i, Drawable dr,
	unsigned int ui1, unsigned int ui2, unsigned int *ui3,
	unsigned int *ui4);
extern Status XQueryBestStipple(Display *d, Drawable dr, unsigned int ui1,
	unsigned int ui2, unsigned int *ui3, unsigned int *ui4);
extern Status XQueryBestTile(Display *d, Drawable dr, unsigned int ui1,
	unsigned int ui2, unsigned int *ui3, unsigned int *ui4);
extern Bool XQueryPointer(Display *d, Window w1, Window *w2, Window *w3,
	int *i1, int *i2, int *i3, int *i4, unsigned int *ui);
	extern Status XQueryTree(Display *d, Window w1, Window *w2, Window *w3,
		Window **w4, unsigned int *ui);
	extern XFontStruct *XQueryFont(Display *d, XID font_ID);
	extern int XQueryTextExtents(Display *d, XID font_ID, _Xconst char *string,
		int nchars, int *direction_return, int *font_ascent_return,
		int *font_descent_return, XCharStruct *overall_return);
	extern int XQueryTextExtents16(Display *d, XID font_ID,
		_Xconst XChar2b *string, int nchars, int *direction_return,
		int *font_ascent_return, int *font_descent_return,
		XCharStruct *overall_return);
	extern int XRaiseWindow(Display *d, Window w);
extern int XRebindKeysym(Display *display, KeySym keysym, KeySym *list,
	int mod_count, _Xconst unsigned char *string, int bytes_string);
extern int XRefreshKeyboardMapping(XMappingEvent *x);
extern Status XReconfigureWMWindow(Display *d, Window w, int screen,
	unsigned int value_mask, XWindowChanges *values);
extern int XRemoveFromSaveSet(Display *d, Window w);
extern int XResetScreenSaver(Display *d);
extern int XResizeWindow(Display *d, Window w, unsigned int ui1,
	unsigned int ui2);
extern int XRestackWindows(Display *d, Window *w, int i);
extern int XRotateBuffers(Display *d, int i);
extern int XRotateWindowProperties(Display *d, Window w, Atom *a, int i1,
	int i2);
extern int XSelectInput(Display *d, Window w, long l);
extern Status XSendEvent(Display *d, Window w, Bool b, long l, XEvent *x);
extern int XGetScreenSaver(Display *d, int *timeout_return,
	int *interval_return, int *prefer_blanking_return,
	int *allow_exposures_return);
extern int XActivateScreenSaver(Display *d);
extern int XSetCommand(Display *d, Window w, char **c, int i);
extern int XSetCloseDownMode(Display *d, int i);
extern Status XGetCommand(Display *d, Window w, char ***argv_return,
	int *argc_return);
extern int XSetClassHint(Display *d, Window w, XClassHint *class_hints);
extern int XSetIconName(Display *d, Window w, _Xconst char *c);
extern int XSetIconSizes(Display *d, Window w, XIconSize *size_list,
	int count);
extern int XSetInputFocus(Display *d, Window w, int i, Time t);
extern int XSetModifierMapping(Display *d, XModifierKeymap *x);
extern int XSetNormalHints(Display *d, Window w, XSizeHints *hints);
extern void XSetRGBColormaps(Display *d, Window w,
	XStandardColormap *stdcmaps, int count, Atom property);
extern int XSetSelectionOwner(Display *d, Atom a, Window w, Time t);
extern int XSetScreenSaver(Display *d, int timeout, int interval,
	int prefer_blanking, int allow_exposures);
extern int XSetSizeHints(Display *d, Window w, XSizeHints *hints,
	Atom property);
extern void XSetStandardColormap(Display *d, Window w,
	XStandardColormap *colormap, Atom property);
extern int XSetStandardProperties(Display *d, Window w,
	_Xconst char *window_name, _Xconst char *icon_name, Pixmap icon_pixmap,
	char **argv, int argc, XSizeHints *hints);
extern void XSetTextProperty(Display *d, Window w, XTextProperty *text_prop,
	Atom property);
extern int XSetTransientForHint(Display *d, Window w, Window prop_window);
extern Status XGetTransientForHint(Display *d, Window w,
	Window *prop_window_return);
extern int XSetZoomHints(Display *d, Window w, XSizeHints *zhints);
extern Status XSetWMColormapWindows(Display *d, Window w,
	Window *colormap_windows, int count);
extern Status XSetWMProtocols(Display *d, Window w, Atom *protocols,
	int count);
extern Status XGetWMProtocols(Display *d, Window w, Atom **protocols_return,
	int *count_return);
extern int XSetWMHints(Display *d, Window w, XWMHints *wm_hints);
extern void XSetWMIconName(Display *d, Window w, XTextProperty *text_prop);
extern void XSetWMName(Display *d, Window w, XTextProperty *text_prop);
extern void XSetWMNormalHints(Display *d, Window w, XSizeHints *hints);
extern void XSetWMProperties(Display *d, Window w,
	XTextProperty *window_name, XTextProperty *icon_name, char **argv,
	int argc, XSizeHints *normal_hints, XWMHints *wm_hints,
	XClassHint *class_hints);
extern void XSetWMSizeHints(Display *d, Window w, XSizeHints *hints,
	Atom property);
extern int XSetWindowBackground(Display *d, Window w, unsigned long ul);
extern int XSetWindowBackgroundPixmap(Display *d, Window w, Pixmap p);
extern int XSetWindowBorder(Display *d, Window w, unsigned long ul);
extern int XSetWindowBorderPixmap(Display *d, Window w, Pixmap p);
extern int XSetWindowBorderWidth(Display *d, Window w, unsigned int ui);
extern int XSetWindowColormap(Display *d, Window w, Colormap c);
extern int XStoreColor(Display *d, Colormap c, XColor *xp);
extern int XStoreColors(Display *d, Colormap c, XColor *xp, int i);
extern int XStoreNamedColor(Display *d, Colormap c, _Xconst char *name,
	unsigned long pixel, int flags);
extern int XStoreName(Display *d, Window w, _Xconst char *name);
extern int XStoreBuffer(Display *d, _Xconst char *c, int i1, int i2);
extern int XStoreBytes(Display *d, _Xconst char *c, int i);
extern Bool XTranslateCoordinates(Display *d, Window w1, Window w2,
	int i1, int i2, int *i3, int *i4, Window *w3);
extern int XUnloadFont(Display *d, Font font);
extern int XUngrabButton(Display *d, unsigned int ui1, unsigned int ui2,
	Window w);
extern int XUngrabKey(Display *d, int i, unsigned int ui, Window w);
extern int XUngrabKeyboard(Display *d, Time t);
extern int XUngrabPointer(Display *d, Time t);
extern int XUninstallColormap(Display *d, Colormap c);
extern int XUnmapSubwindows(Display *d, Window w);
extern int XUnmapWindow(Display *d, Window w);
extern int XWindowEvent(Display *d, Window w, long l, XEvent *x);
extern void XDestroyIC(XIC x);
extern Bool XFilterEvent(XEvent *x, Window w);
extern int XmbLookupString(XIC xi, XKeyPressedEvent *xk, char *c, int i,
	KeySym *k, Status *s);
extern int Xutf8LookupString(XIC xi, XKeyPressedEvent *xk, char *c, int i,
	KeySym *k, Status *s);
extern char *XGetICValues(XIC xi, ...);
extern char *XSetICValues(XIC xi, ...);
extern void XSetICFocus(XIC xi);
extern void XUnsetICFocus(XIC xi);
extern XIM XOpenIM(Display *display, XPointer rdb, char *resourceName,
	char *resourceClass);
extern Status XCloseIM(XIM xim);
extern Display *XDisplayOfIM(XIM xim);
extern char *XLocaleOfIM(XIM xim);
extern char *XSetLocaleModifiers(_Xconst char *modifierList);
extern Bool XRegisterIMInstantiateCallback(Display *display,
	struct _XrmHashBucketRec *rdb, char *resourceName, char *resourceClass,
	XIDProc callback, XPointer clientData);
extern Bool XUnregisterIMInstantiateCallback(Display *display,
	struct _XrmHashBucketRec *rdb, char *resourceName, char *resourceClass,
	XIDProc callback, XPointer clientData);
extern char *XGetIMValues(XIM xim, ...);
extern char *XSetIMValues(XIM xim, ...);
extern XVaNestedList XVaCreateNestedList(int unused, ...);
extern XFontSet XCreateFontSet(Display *display, _Xconst char *baseFontNameList,
	char ***missingCharsetListReturn, int *missingCharsetCountReturn,
	char **defStringReturn);
extern void XFreeFontSet(Display *display, XFontSet fontSet);
extern XFontSetExtents *XExtentsOfFontSet(XFontSet fontSet);
extern char *XBaseFontNameListOfFontSet(XFontSet fontSet);
extern char *XLocaleOfFontSet(XFontSet fontSet);
extern Bool XContextDependentDrawing(XFontSet fontSet);
extern Bool XDirectionalDependentDrawing(XFontSet fontSet);
extern Bool XContextualDrawing(XFontSet fontSet);
extern int XFontsOfFontSet(XFontSet fontSet,
	XFontStruct ***fontStructListReturn, char ***fontNameListReturn);
extern int XmbTextEscapement(XFontSet fontSet, _Xconst char *text,
	int bytes_text);
extern int XwcTextEscapement(XFontSet fontSet, _Xconst wchar_t *text,
	int num_wchars);
extern int Xutf8TextEscapement(XFontSet fontSet, _Xconst char *text,
	int bytes_text);
extern int XmbTextExtents(XFontSet fontSet, _Xconst char *text,
	int bytes_text, XRectangle *overall_ink_return,
	XRectangle *overall_logical_return);
extern int XwcTextExtents(XFontSet fontSet, _Xconst wchar_t *text,
	int num_wchars, XRectangle *overall_ink_return,
	XRectangle *overall_logical_return);
extern int Xutf8TextExtents(XFontSet fontSet, _Xconst char *text,
	int bytes_text, XRectangle *overall_ink_return,
	XRectangle *overall_logical_return);
extern Status XmbTextPerCharExtents(XFontSet fontSet, _Xconst char *text,
	int bytes_text, XRectangle *ink_extents_buffer,
	XRectangle *logical_extents_buffer, int buffer_size, int *num_chars,
	XRectangle *overall_ink_return, XRectangle *overall_logical_return);
extern Status XwcTextPerCharExtents(XFontSet fontSet, _Xconst wchar_t *text,
	int num_wchars, XRectangle *ink_extents_buffer,
	XRectangle *logical_extents_buffer, int buffer_size, int *num_chars,
	XRectangle *overall_ink_return, XRectangle *overall_logical_return);
extern Status Xutf8TextPerCharExtents(XFontSet fontSet, _Xconst char *text,
	int bytes_text, XRectangle *ink_extents_buffer,
	XRectangle *logical_extents_buffer, int buffer_size, int *num_chars,
	XRectangle *overall_ink_return, XRectangle *overall_logical_return);
extern void XmbDrawText(Display *display, Drawable d, GC gc, int x, int y,
	XmbTextItem *text_items, int nitems);
extern void XwcDrawText(Display *display, Drawable d, GC gc, int x, int y,
	XwcTextItem *text_items, int nitems);
extern void Xutf8DrawText(Display *display, Drawable d, GC gc, int x, int y,
	XmbTextItem *text_items, int nitems);
extern void XmbDrawString(Display *display, Drawable d, XFontSet fontSet,
	GC gc, int x, int y, _Xconst char *text, int bytes_text);
extern void XwcDrawString(Display *display, Drawable d, XFontSet fontSet,
	GC gc, int x, int y, _Xconst wchar_t *text, int num_wchars);
extern void Xutf8DrawString(Display *display, Drawable d, XFontSet fontSet,
	GC gc, int x, int y, _Xconst char *text, int bytes_text);
extern void XmbDrawImageString(Display *display, Drawable d,
	XFontSet fontSet, GC gc, int x, int y, _Xconst char *text,
	int bytes_text);
extern void XwcDrawImageString(Display *display, Drawable d,
	XFontSet fontSet, GC gc, int x, int y, _Xconst wchar_t *text,
	int num_wchars);
extern void Xutf8DrawImageString(Display *display, Drawable d,
	XFontSet fontSet, GC gc, int x, int y, _Xconst char *text,
	int bytes_text);
extern void XFreeStringList(char **list);
extern void XwcFreeStringList(wchar_t **list);
extern Status XTextPropertyToStringList(XTextProperty *text_prop,
	char ***list_return, int *count_return);
extern int XmbTextListToTextProperty(Display *display, char **list,
	int count, XICCEncodingStyle style, XTextProperty *text_prop_return);
extern int XmbTextPropertyToTextList(Display *display,
	const XTextProperty *text_prop, char ***list_return, int *count_return);
extern int XwcTextListToTextProperty(Display *display, wchar_t **list,
	int count, XICCEncodingStyle style, XTextProperty *text_prop_return);
extern int XwcTextPropertyToTextList(Display *display,
	const XTextProperty *text_prop, wchar_t ***list_return,
	int *count_return);
extern void XmbSetWMProperties(Display *display, Window w,
	_Xconst char *window_name, _Xconst char *icon_name, char **argv,
	int argc, XSizeHints *normal_hints, XWMHints *wm_hints,
	XClassHint *class_hints);
extern int Xutf8TextListToTextProperty(Display *display, char **list,
	int count, XICCEncodingStyle style, XTextProperty *text_prop_return);
extern int Xutf8TextPropertyToTextList(Display *display,
	const XTextProperty *text_prop, char ***list_return, int *count_return);
extern void Xutf8SetWMProperties(Display *display, Window w,
	_Xconst char *window_name, _Xconst char *icon_name, char **argv,
	int argc, XSizeHints *normal_hints, XWMHints *wm_hints,
	XClassHint *class_hints);
extern int X11WasmSetClipboardText(_Xconst char *utf8, int utf8Length);
extern char *X11WasmTakeCompositionText(XEvent *eventPtr,
	int *utf8LengthReturn);
extern int X11WasmSetWindowMetadataTarget(Display *display, Window window,
	Window target, int isRootToplevel);
extern void X11WasmSetPixmapColormap(Pixmap pixmap, Colormap colormap);
extern int XSetClipRectangles(Display *display, GC gc, int clip_x_origin,
	int clip_y_origin, XRectangle rectangles[], int n, int ordering);
extern Status XParseColor(Display *display, Colormap map, _Xconst char *spec,
	XColor *colorPtr);
extern GC XCreateGC(Display *display, Drawable d, unsigned long valuemask,
	XGCValues *values);
extern int XFreeGC(Display *display, GC gc);
extern void XFlushGC(Display *display, GC gc);
extern Atom XInternAtom(Display *display, _Xconst char *atom_name,
	Bool only_if_exists);
extern Status XInternAtoms(Display *display, char **names, int count,
	Bool only_if_exists, Atom *atoms_return);
extern Status XGetAtomNames(Display *display, Atom *atoms, int count,
	char **names_return);
extern int XSetBackground(Display *display, GC gc, unsigned long foreground);
extern int XSetForeground(Display *display, GC gc, unsigned long foreground);
extern int XSetClipMask(Display *display, GC gc, Pixmap pixmap);
extern int XSetClipOrigin(Display *display, GC gc, int clip_x_origin,
	int clip_y_origin);
extern int XSetTSOrigin(Display *display, GC gc, int ts_x_origin,
	int ts_y_origin);
extern int XSetGraphicsExposures(Display *display, GC gc,
	Bool graphics_exposures);
extern int XSetPlaneMask(Display *display, GC gc, unsigned long plane_mask);
extern int XSetState(Display *display, GC gc, unsigned long foreground,
	unsigned long background, int function, unsigned long plane_mask);
extern int XSetSubwindowMode(Display *display, GC gc, int subwindow_mode);
extern int XChangeGC(Display *d, GC gc, unsigned long mask,
	XGCValues *values);
extern int XSetFont(Display *display, GC gc, Font font);
extern int XSetArcMode(Display *display, GC gc, int arc_mode);
extern int XSetTile(Display *display, GC gc, Pixmap tile);
extern int XSetStipple(Display *display, GC gc, Pixmap stipple);
extern int XSetFillRule(Display *display, GC gc, int fill_rule);
extern int XSetFillStyle(Display *display, GC gc, int fill_style);
extern int XSetFunction(Display *display, GC gc, int function);
extern int XSetLineAttributes(Display *display, GC gc,
	unsigned int line_width, int line_style, int cap_style, int join_style);
extern Status XInitImage(XImage *image);
extern int _XInitImageFuncPtrs(XImage *image);
extern XIC XCreateIC(XIM xim, ...);
extern XIM XIMOfIC(XIC xic);
extern wchar_t *XwcResetIC(XIC xic);
extern char *XmbResetIC(XIC xic);
extern char *Xutf8ResetIC(XIC xic);
extern int XwcLookupString(XIC xic, XKeyPressedEvent *event,
	wchar_t *buffer, int wchars, KeySym *keysym, Status *status);
extern XVisualInfo *XGetVisualInfo(Display *display, long vinfo_mask,
	XVisualInfo *vinfo_template, int *nitems_return);
extern XPixmapFormatValues *XListPixmapFormats(Display *display,
	int *count_return);
extern int *XListDepths(Display *display, int screen_number,
	int *count_return);
extern Status XMatchVisualInfo(Display *display, int screen, int depth,
	int c_class, XVisualInfo *vinfo_return);
extern void XSetWMClientMachine(Display *display, Window w,
	XTextProperty *text_prop);
extern Status XStringListToTextProperty(char **list, int count,
	XTextProperty *text_prop_return);
extern int XDrawLine(Display *d, Drawable dr, GC g, int x1, int y1,
	int x2, int y2);
extern int XWarpPointer(Display *d, Window s, Window dw, int sx, int sy,
	unsigned int sw, unsigned int sh, int dx, int dy);
extern int XFillRectangle(Display *display, Drawable d, GC gc, int x, int y,
	unsigned int width, unsigned int height);
extern int XFlush(Display *display);
extern int XGrabServer(Display *display);
extern int XUngrabServer(Display *display);
extern int XFree(void *data);
extern int XNoOp(Display *display);
extern XAfterFunction XSetAfterFunction(Display *display,
	XAfterFunction procedure);
extern XAfterFunction XSynchronize(Display *display, Bool onoff);
extern int XSync(Display *display, Bool discard);
extern void XSetAuthorization(char *name, int namelen, char *data,
	int datalen);
extern VisualID XVisualIDFromVisual(Visual *visual);
extern Bool XPointInRegion(Region rgn, int x, int y);
extern Region XPolygonRegion(XPoint *points, int n, int fill_rule);
extern Bool XEqualRegion(Region rgn1, Region rgn2);
extern int XOffsetRegion(Region rgn, int dx, int dy);
extern int XShrinkRegion(Region rgn, int dx, int dy);
extern int XUnionRegion(Region srca, Region srcb, Region dr_return);
extern int XXorRegion(Region srca, Region srcb, Region dr_return);
extern Window XCreateWindow(Display *display, Window parent, int x, int y,
	unsigned int width, unsigned int height, unsigned int border_width,
	int depth, unsigned int clazz, Visual *visual, unsigned long value_mask,
	XSetWindowAttributes *attributes);
extern Window XCreateSimpleWindow(Display *display, Window parent, int x,
	int y, unsigned int width, unsigned int height,
	unsigned int border_width, unsigned long border,
	unsigned long background);
extern Pixmap XCreatePixmap(Display *display, Drawable d, unsigned int width,
	unsigned int height, unsigned int depth);
extern int XLowerWindow(Display *d, Window w);
extern int XFillArcs(Display *d, Drawable dr, GC gc, XArc *a, int n);
extern int XDrawArcs(Display *d, Drawable dr, GC gc, XArc *a, int n);
extern int XDrawRectangles(Display *d, Drawable dr, GC gc, XRectangle *r,
	int n);
extern int XDrawSegments(Display *d, Drawable dr, GC gc, XSegment *s, int n);
extern int XDrawPoint(Display *d, Drawable dr, GC gc, int x, int y);
extern int XDrawPoints(Display *d, Drawable dr, GC gc, XPoint *p, int n,
	int m);
extern int XReparentWindow(Display *d, Window w, Window p, int x, int y);
extern int XPutImage(Display *d, Drawable dr, GC gc, XImage *im, int sx,
	int sy, int dx, int dy, unsigned int w, unsigned int h);

extern Display *XOpenDisplay(_Xconst char *display_name);
extern char *XDisplayName(_Xconst char *display_name);
extern char *XGetDefault(Display *display, _Xconst char *program,
	_Xconst char *option);
extern char *XResourceManagerString(Display *display);
extern char *XScreenResourceString(Screen *screen);
extern const char *XDefaultString(void);
extern void XrmInitialize(void);
extern int XCloseDisplay(Display *display);
extern int XEventsQueued(Display *display, int mode);
extern int XDisplayKeycodes(Display *display, int *min_keycodes_return,
	int *max_keycodes_return);
extern int XAutoRepeatOff(Display *display);
extern int XAutoRepeatOn(Display *display);
extern int XChangeKeyboardControl(Display *display, unsigned long value_mask,
	XKeyboardControl *values);
extern int XGetKeyboardControl(Display *display,
	XKeyboardState *values_return);
extern int XQueryKeymap(Display *display, char keys_return[32]);
extern int XChangePointerControl(Display *display, Bool do_accel,
	Bool do_threshold, int accel_numerator, int accel_denominator,
	int threshold);
extern int XGetPointerControl(Display *display,
	int *accel_numerator_return, int *accel_denominator_return,
	int *threshold_return);
extern int XGetPointerMapping(Display *display, unsigned char *map_return,
	int nmap);
extern int XSetPointerMapping(Display *display,
	_Xconst unsigned char *map, int nmap);
extern XTimeCoord *XGetMotionEvents(Display *display, Window w,
	Time start, Time stop, int *nevents_return);
extern Font XLoadFont(Display *display, _Xconst char *name);
extern XFontStruct *XLoadQueryFont(Display *display, _Xconst char *name);
extern char **XListFonts(Display *display, _Xconst char *pattern,
	int maxnames, int *actual_count_return);
extern char **XListFontsWithInfo(Display *display, _Xconst char *pattern,
	int maxnames, int *actual_count_return, XFontStruct **info_return);
extern char **XGetFontPath(Display *display, int *npaths_return);
extern int XSetFontPath(Display *display, char **directories, int ndirs);
extern int XTextWidth(XFontStruct *font_struct, _Xconst char *string,
	int count);
/* widths has numBytes + 1 entries, indexed by original UTF-8/Tcl UTF bytes.
 * Success is zero; an unavailable layout backend returns -1. */
extern int X11WasmTextRunWidths(XFontStruct *font, const char *source,
	int numBytes, int *widths);
extern int X11WasmDrawTextInContext(Display *display, Drawable drawable, GC gc,
	const char *source, int numBytes, int rangeStart, int rangeLength,
	int x, int y);
	extern int XTextWidth16(XFontStruct *font_struct, _Xconst XChar2b *string,
		int count);
	extern int XTextExtents(XFontStruct *font_struct, _Xconst char *string,
		int nchars, int *direction_return, int *font_ascent_return,
		int *font_descent_return, XCharStruct *overall_return);
	extern int XTextExtents16(XFontStruct *font_struct,
		_Xconst XChar2b *string, int nchars, int *direction_return,
		int *font_ascent_return, int *font_descent_return,
		XCharStruct *overall_return);
	extern int XDrawString(Display *display, Drawable d, GC gc, int x, int y,
		_Xconst char *string, int length);
	extern int XDrawString16(Display *display, Drawable d, GC gc, int x, int y,
		_Xconst XChar2b *string, int length);
	extern int XDrawText(Display *display, Drawable d, GC gc, int x, int y,
		XTextItem *items, int nitems);
	extern int XDrawText16(Display *display, Drawable d, GC gc, int x, int y,
		XTextItem16 *items, int nitems);
extern int XDrawImageString(Display *display, Drawable d, GC gc, int x, int y,
	_Xconst char *string, int length);
extern int XDrawImageString16(Display *display, Drawable d, GC gc, int x,
	int y, _Xconst XChar2b *string, int length);
	extern int XUndefineCursor(Display *display, Window w);

#ifdef __cplusplus
}
#endif

#endif /* _X11WASM_XLIBWASM_H */
