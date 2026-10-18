#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XlibWasm.h>
#include <emscripten.h>

/* These application symbols must not collide with Xlib's private state. */
int errorHandler;
int eventQueue;
int atomList;
int localeModifiers;
int browserDisplay;
int damageSerial;

static void
SetStatus(
    const char *text)
{
    EM_ASM({
	const status = document.getElementById("x11-wasm-status");
	if (status) {
	    status.textContent = UTF8ToString($0);
	}
    }, text);
}

static void
SetResult(
    int code,
    const char *text)
{
    EM_ASM({
	const result = document.getElementById("x11-wasm-result");
	if (result) {
	    result.dataset.code = String($0);
	    result.textContent = UTF8ToString($1);
	}
    }, code, text);
}

int
main(
    void)
{
    Display *display = XOpenDisplay(NULL);

    if (display == NULL) {
	SetStatus("failed");
	SetResult(1, "XOpenDisplay failed");
	return 1;
    }

    SetResult(0, ServerVendor(display));
    SetStatus("running");
    return 0;
}
