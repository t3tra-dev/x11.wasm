#define NeedWidePrototypes 0
#include <X11/Xlib.h>

/* Match the declaration supplied by Xorg's XKBlib.h. */
extern KeySym XkbKeycodeToKeysym(Display *, KeyCode, int, int);
