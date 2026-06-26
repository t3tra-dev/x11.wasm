/*
 * scrnsaver.h --
 *
 *	Minimal MIT-SCREEN-SAVER extension declarations for x11.wasm.
 */

#ifndef _X11_EXTENSIONS_SCRNSAVER_H
#define _X11_EXTENSIONS_SCRNSAVER_H

#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ScreenSaverName "MIT-SCREEN-SAVER"
#define ScreenSaverMajorVersion 1
#define ScreenSaverMinorVersion 1

#define ScreenSaverNotifyMask (1L << 0)
#define ScreenSaverCycleMask (1L << 1)

#define ScreenSaverNotify 0
#define ScreenSaverCycle 1

#define ScreenSaverOff 0
#define ScreenSaverOn 1
#define ScreenSaverDisabled 2

#define ScreenSaverBlanked 0
#define ScreenSaverInternal 1
#define ScreenSaverExternal 2

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

#ifdef __cplusplus
}
#endif

#endif /* _X11_EXTENSIONS_SCRNSAVER_H */
