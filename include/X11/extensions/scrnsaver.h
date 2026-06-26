/*
 * scrnsaver.h --
 *
 *	Minimal MIT-SCREEN-SAVER extension declarations for x11.wasm.
 */

#ifndef _X11_EXTENSIONS_SCRNSAVER_H
#define _X11_EXTENSIONS_SCRNSAVER_H

#include <X11/Xlib.h>
#include <X11/extensions/saver.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    Window root;
    int state;
    int kind;
    Bool forced;
    Time time;
} XScreenSaverNotifyEvent;

typedef struct {
    Window window;
    int state;
    int kind;
    unsigned long til_or_since;
    unsigned long idle;
    unsigned long eventMask;
} XScreenSaverInfo;

extern Bool XScreenSaverQueryExtension(
    Display *display,
    int *event_base_return,
    int *error_base_return);
extern Status XScreenSaverQueryVersion(
    Display *display,
    int *major_version_return,
    int *minor_version_return);
extern XScreenSaverInfo *XScreenSaverAllocInfo(void);
extern Status XScreenSaverQueryInfo(
    Display *display,
    Drawable drawable,
    XScreenSaverInfo *saver_info);
extern void XScreenSaverSelectInput(
    Display *display,
    Drawable drawable,
    unsigned long event_mask);
extern void XScreenSaverSuspend(
    Display *display,
    Bool suspend);
extern Status XScreenSaverRegister(
    Display *display,
    int screen,
    XID xid,
    Atom type);
extern Status XScreenSaverUnregister(
    Display *display,
    int screen);
extern Status XScreenSaverGetRegistered(
    Display *display,
    int screen,
    XID *xid,
    Atom *type);

#ifdef __cplusplus
}
#endif

#endif /* _X11_EXTENSIONS_SCRNSAVER_H */
