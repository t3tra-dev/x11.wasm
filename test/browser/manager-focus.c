#include <X11/XlibWasm.h>
#include <X11/keysym.h>
#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE unsigned int
X11WasmTestManagerFocusWindow(void)
{
    Display *display = XOpenDisplay(NULL);
    Window window = None;
    int revertTo;

    XGetInputFocus(display, &window, &revertTo);
    return (unsigned int)window;
}

EMSCRIPTEN_KEEPALIVE unsigned int
X11WasmTestManagerTopWindow(void)
{
    Display *display = XOpenDisplay(NULL);
    Window root, parent, *children = NULL;
    unsigned int count = 0;
    Window top = None;

    if (XQueryTree(display, DefaultRootWindow(display), &root, &parent,
	    &children, &count) && count) {
	top = children[count - 1];
    }
    XFree(children);
    return (unsigned int)top;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestManagerFocusChild(unsigned int window)
{
    Display *display = XOpenDisplay(NULL);

    XSelectInput(display, (Window)window, KeyPressMask | ButtonPressMask);
    XMapWindow(display, (Window)window);
    XSetInputFocus(display, (Window)window, RevertToParent, CurrentTime);
    XFlush(display);
    return X11WasmTestManagerFocusWindow() == window;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestManagerChildKey(unsigned int window)
{
    Display *display = XOpenDisplay(NULL);
    XEvent event;

    return XCheckWindowEvent(display, (Window)window, KeyPressMask, &event)
	    && event.xkey.window == (Window)window
	    && XLookupKeysym(&event.xkey, 0) == XK_a;
}
