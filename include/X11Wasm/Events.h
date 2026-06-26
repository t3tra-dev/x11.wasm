/*
 * Events.h --
 *
 *	Browser event extensions exposed by the x11.wasm Xlib-shaped ABI.
 */

#ifndef _X11WASM_EVENTS_H
#define _X11WASM_EVENTS_H

#include <X11/Xlib.h>

#ifndef X11_WASM_BROWSER_WHEEL_EVENT
#define X11_WASM_BROWSER_WHEEL_EVENT (MappingNotify + 1)
#endif

#ifndef X11_WASM_BROWSER_WHEEL_MASK
#define X11_WASM_BROWSER_WHEEL_MASK (1L << 28)
#endif

#ifndef X11_WASM_PINNED_KEY_EVENT_STATE
#define X11_WASM_PINNED_KEY_EVENT_STATE ((unsigned int)0x40000000U)
#endif

#endif /* _X11WASM_EVENTS_H */
