/*
 * Runtime.h --
 *
 *	Small runtime helpers used by the x11.wasm browser X11 port.
 */

#ifndef _X11WASM_RUNTIME_H
#define _X11WASM_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#include <X11/Xlib.h>
#ifndef X11WASM_XAFTERFUNCTION_DEFINED
#define X11WASM_XAFTERFUNCTION_DEFINED
typedef int (*XAfterFunction)(Display *display);
#endif
#include <X11/Xutil.h>

#include <X11Wasm/Events.h>

#ifndef MODULE_SCOPE
#define MODULE_SCOPE extern
#endif

#ifndef X11_WASM_INTEGER_SPACE
#define X11_WASM_INTEGER_SPACE 24
#endif

typedef struct X11WasmEncoding_ *X11WasmEncoding;
typedef unsigned int X11WasmCodepoint;

#define X11_WASM_DSTRING_STATIC_SIZE 200
typedef struct X11WasmDString {
    char *string;
    int length;
    int spaceAvl;
    char staticSpace[X11_WASM_DSTRING_STATIC_SIZE];
} X11WasmDString;

#define X11WasmDStringLength(dsPtr) ((dsPtr)->length)
#define X11WasmDStringValue(dsPtr) ((dsPtr)->string)

#ifndef ckalloc
void *ckalloc(size_t size);
#endif
#ifndef ckfree
void ckfree(void *ptr);
#endif
void X11WasmDStringInit(X11WasmDString *dsPtr);
char *X11WasmDStringAppend(X11WasmDString *dsPtr, const char *bytes,
	int length);
char *X11WasmDStringAppendElement(X11WasmDString *dsPtr,
	const char *element);
void X11WasmDStringFree(X11WasmDString *dsPtr);
X11WasmEncoding X11WasmGetEncoding(const char *name);
void X11WasmFreeEncoding(X11WasmEncoding encoding);
char *X11WasmExternalToUtfDString(X11WasmEncoding encoding, const char *src,
	int srcLen, X11WasmDString *dsPtr);
char *X11WasmUtfToExternalDString(X11WasmEncoding encoding, const char *src,
	int srcLen, X11WasmDString *dsPtr);
int X11WasmUtfToCodepoint(const char *src, X11WasmCodepoint *chPtr);
void X11WasmSetPixmapColormap(Pixmap pixmap, Colormap colormap);
Pixmap X11WasmCreateNativeBitmap(Display *display, const void *source);
Pixmap X11WasmGetNativeAppBitmap(Display *display, const char *name,
	int *width, int *height);

#endif /* _X11WASM_RUNTIME_H */
