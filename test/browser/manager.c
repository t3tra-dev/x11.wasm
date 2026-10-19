#include <X11/XlibWasm.h>
#include <X11/Xatom.h>
#include "../../src/libX11/XlibInt.h"
#include <emscripten.h>
#include <string.h>

static Window managerParts[3];

EMSCRIPTEN_KEEPALIVE unsigned int
X11WasmTestManagerWindow(int mode)
{
    Display *display = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(display);
    Window frame, metadata, widget;
    Atom protocol = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWindowAttributes attributes;

    memset(&attributes, 0, sizeof(attributes));
    attributes.override_redirect = (mode == 3);
    frame = XCreateWindow(display, root, 19, 23, 137, 88, 0,
	    CopyFromParent, (mode == 4) ? InputOnly : InputOutput, NULL,
	    CWOverrideRedirect, &attributes);
    metadata = frame;
    if ((mode == 1) || (mode == 2)) {
	metadata = XCreateSimpleWindow(display, frame, 0, 0, 137, 88, 0, 0, 0);
	X11WasmSetWindowMetadataTarget(display, metadata, metadata, 0);
	if (mode == 1) {
	    X11WasmSetWindowMetadataTarget(display, frame, metadata, 0);
	}
	XMapWindow(display, metadata);
    }
    widget = XCreateSimpleWindow(display, metadata, 2, 3, 20, 15, 0, 0, 0);
    if (mode != 5) {
	XSetWMProtocols(display, frame, &protocol, 1);
    }
    XSelectInput(display, frame, StructureNotifyMask | PropertyChangeMask | FocusChangeMask);
    XSelectInput(display, metadata, StructureNotifyMask | PropertyChangeMask | FocusChangeMask);
    XMapWindow(display, frame);
    managerParts[0] = frame;
    managerParts[1] = metadata;
    managerParts[2] = widget;
    return (unsigned int)frame;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestManagerOptions(unsigned int window, int option, int value)
{
    Display *display = XOpenDisplay(NULL);
    if (option == 0) {
	static const char *types[] = {"_NET_WM_WINDOW_TYPE_NORMAL",
	    "_NET_WM_WINDOW_TYPE_POPUP_MENU", "_CUSTOM_TYPE", "_NET_WM_WINDOW_TYPE_DIALOG",
	    "_NET_WM_WINDOW_TYPE_UTILITY", "_NET_WM_WINDOW_TYPE_TOOLTIP"};
	Atom atoms[2];
	int first = value & 15, second = (value >> 4) & 15;
	if (first > 5 || second > 5) return 0;
	atoms[0] = XInternAtom(display, types[first], False);
	atoms[1] = XInternAtom(display, types[second], False);
	return XChangeProperty(display, window,
		XInternAtom(display, "_NET_WM_WINDOW_TYPE", False), XA_ATOM, 32,
		PropModeReplace, (unsigned char *)atoms, 2);
    }
    if (option == 1) {
	XWMHints hints;
	memset(&hints, 0, sizeof(hints));
	hints.flags = InputHint;
	hints.input = value;
	return XSetWMHints(display, window, &hints);
    }
    if (option == 2) {
	Atom protocols[2] = {XInternAtom(display,"WM_DELETE_WINDOW",False),
	    XInternAtom(display,"WM_TAKE_FOCUS",False)};
	return XSetWMProtocols(display, window, protocols, value ? 2 : 1);
    }
    if (option == 3) return XSetInputFocus(display, window, RevertToParent, CurrentTime);
    if (option == 4) {
	if (value == 1) return XGrabPointer(display, window, False,
		PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	if (value == 2) return XGrabKeyboard(display, window, False,
		GrabModeAsync, GrabModeAsync, CurrentTime);
	if (value == 3) return XGrabServer(display);
	XUngrabPointer(display, CurrentTime);
	XUngrabKeyboard(display, CurrentTime);
	XUngrabServer(display);
	return 0;
    }
    if (option == 5) {
	XEvent event;
	return XCheckTypedWindowEvent(display, window, ClientMessage, &event)
	    && event.xclient.message_type == XInternAtom(display,"WM_PROTOCOLS",False)
	    && event.xclient.data.l[0] == (long)XInternAtom(display,"WM_TAKE_FOCUS",False);
    }
    if (option == 6) {
	Window focus;
	int revert;
	XGetInputFocus(display, &focus, &revert);
	return (int)focus;
    }
    if (option == 7) {
	Window root, parent, *children;
	unsigned int count;
	if (!XQueryTree(display, DefaultRootWindow(display), &root, &parent, &children, &count)) return 0;
	value = count ? (int)children[count-1] : 0;
	XFree(children);
	return value;
    }
    if (option == 8) return XMapWindow(display, window);
    if (option == 9) {
	XSizeHints hints;
	memset(&hints, 0, sizeof(hints));
	hints.flags = PResizeInc | PAspect;
	hints.width_inc = 8;
	hints.height_inc = 6;
	hints.min_aspect.x = hints.max_aspect.x = 4;
	hints.min_aspect.y = hints.max_aspect.y = 3;
	if (value != 1) {
	    hints.flags |= PBaseSize;
	    hints.base_width = 10;
	    hints.base_height = 5;
	}
	if (value != 2) {
	    hints.flags |= PMinSize;
	    hints.min_width = 26;
	    hints.min_height = 17;
	    if (value == 1) {
		hints.min_width = 32;
		hints.min_height = 18;
	    }
	}
	XSetWMNormalHints(display, window, &hints);
	return 1;
    }
    if (option == 10) return (int)(value ? x11wasm_browserDisplay.focusTime : x11wasm_eventTime);
    if (option == 11) return XSetInputFocus(display, window, RevertToParent, (Time)(uint32_t)value);
    return 0;
}

EMSCRIPTEN_KEEPALIVE unsigned int
X11WasmTestManagerPart(int field)
{
    return ((field >= 0) && (field < 3)) ? (unsigned int)managerParts[field] : 0;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestManagerCloseEvent(unsigned int expected)
{
    Display *display = XOpenDisplay(NULL);
    XEvent event;

    return XCheckTypedWindowEvent(display, expected, ClientMessage, &event)
	    && (event.xclient.window == expected)
	    && (event.xclient.message_type == XInternAtom(display, "WM_PROTOCOLS", False))
	    && (event.xclient.data.l[0] ==
		    (long)XInternAtom(display, "WM_DELETE_WINDOW", False));
}
