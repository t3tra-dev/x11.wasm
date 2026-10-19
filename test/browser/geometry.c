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
