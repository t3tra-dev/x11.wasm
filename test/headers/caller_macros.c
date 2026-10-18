#define EXTERN 42
#include <X11/Xlib.h>

#if EXTERN != 42
#error Xlib must preserve caller macros
#endif
