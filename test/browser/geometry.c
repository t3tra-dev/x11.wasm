#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <emscripten.h>
#include <string.h>

EMSCRIPTEN_KEEPALIVE unsigned int
X11WasmTestGeometryWindow(void)
{
    Display *display = XOpenDisplay(NULL);
    Window window = XCreateSimpleWindow(display, DefaultRootWindow(display),
	    19, 23, 137, 88, 0, 0, 0);

    XSelectInput(display, window, StructureNotifyMask);
    XMapWindow(display, window);
    return (unsigned int)window;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGeometryValue(unsigned int window, int field)
{
    Display *display = XOpenDisplay(NULL);
    Window root;
    int x, y;
    unsigned int width, height, border, depth;

    if (!XGetGeometry(display, window, &root, &x, &y, &width, &height,
	    &border, &depth)) {
	return -1;
    }
    switch (field) {
    case 0: return x;
    case 1: return y;
    case 2: return (int)width;
    case 3: return (int)height;
    default: return (int)border;
    }
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGeometryConfigure(unsigned int window, unsigned int mask,
    int x, int y, int width, int height, int border)
{
    XWindowChanges changes;

    memset(&changes, 0, sizeof(changes));
    changes.x = x;
    changes.y = y;
    changes.width = width;
    changes.height = height;
    changes.border_width = border;
    return XConfigureWindow(XOpenDisplay(NULL), window, mask, &changes);
}

EMSCRIPTEN_KEEPALIVE void
X11WasmTestGeometryHints(unsigned int window, int mode)
{
    XSizeHints hints;

    memset(&hints, 0, sizeof(hints));
    if (mode == 1) {
	hints.flags = PMinSize | PMaxSize | PResizeInc | PBaseSize;
	hints.min_width = 100;
	hints.min_height = 50;
	hints.max_width = 220;
	hints.max_height = 160;
	hints.width_inc = 16;
	hints.height_inc = 10;
	hints.base_width = 10;
	hints.base_height = 5;
    } else if (mode == 2) {
	hints.flags = PAspect;
	hints.min_aspect.x = 4;
	hints.min_aspect.y = 3;
	hints.max_aspect.x = 16;
	hints.max_aspect.y = 9;
    } else if (mode == 3) {
	hints.flags = PMinSize | PMaxSize;
	hints.min_width = 200;
	hints.min_height = 150;
	hints.max_width = 220;
	hints.max_height = 160;
    }
    XSetWMNormalHints(XOpenDisplay(NULL), window, &hints);
}

EMSCRIPTEN_KEEPALIVE unsigned int
X11WasmTestGeometryHit(int x, int y)
{
    Display *display = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(display);
    Window child = None;
    int translatedX, translatedY;

    XTranslateCoordinates(display, root, root, x, y,
	    &translatedX, &translatedY, &child);
    return (unsigned int)child;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGeometryWmState(unsigned int window, int action, int value)
{
    Display *display = XOpenDisplay(NULL);
    Atom property = XInternAtom(display, "WM_STATE", False);
    Atom type;
    int format, result = -1;
    unsigned long count, after;
    unsigned char *data = NULL;
    XEvent event;

    switch (action) {
    case 1: return XIconifyWindow(display, window, value);
    case 2: return XWithdrawWindow(display, window, value);
    case 3: return XMapWindow(display, window);
    case 4: {
	XWMHints hints;
	memset(&hints, 0, sizeof(hints));
	hints.flags = StateHint;
	hints.initial_state = value;
	return XSetWMHints(display, window, &hints);
    }
    case 5: return XDeleteProperty(display, window, property);
    case 7: {
	int i, length;
	Atom *properties = XListProperties(display, window, &length);
	result = 0;
	for (i = 0; i < length; i++) {
	    if (properties[i] == property) result++;
	}
	XFree(properties);
	return result;
    }
    case 8:
	result = 0;
	while (XCheckTypedWindowEvent(display, window, PropertyNotify, &event)) {
	    if (event.xproperty.atom == property) result++;
	}
	return result;
    case 9: {
	long state[2] = {value, None};
	return XChangeProperty(display, window, property, property, 32,
		PropModeReplace, (unsigned char *)state, 2);
    }
    case 10: {
	XWindowAttributes attributes;
	return XGetWindowAttributes(display, window, &attributes)
		? attributes.map_state : -1;
    }
    case 11: return XUnmapWindow(display, window);
    case 12: XSelectInput(display, window, StructureNotifyMask | PropertyChangeMask);
	return 1;
    default: break;
    }
    if ((XGetWindowProperty(display, window, property, 0, 2,
	    action == 6, property, &type, &format, &count, &after, &data)
	    == Success) && (type == property) && (format == 32)
	    && (count == 2) && (after == 0) && (data != NULL)) {
	result = (int)((long *)data)[0];
    }
    XFree(data);
    return result;
}
