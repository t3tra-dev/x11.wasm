# x11.wasm

Browser-oriented X11/Xlib compatibility layer for WASM applications.

## Display model

The browser backend intentionally exposes a small X11 display model:

- one display with one screen (`DefaultScreen == 0`, `ScreenCount == 1`);
- one 24-bit `TrueColor` default visual (`visualid == 1`, RGB masks
  `0xff0000`, `0x00ff00`, and `0x0000ff`, `bits_per_rgb == 8`);
- one window depth (`24`) plus pixmap formats for depth 1 bitmap pixmaps and
  depth 24 pixmaps stored as 32 bpp canvas data;
- colormaps are valid only for the default visual.

Unsupported screens, depths, visual classes, and non-default visuals are
reported as missing by visual/depth queries and rejected by window or colormap
creation with `BadMatch`. Adding multiple screens, other depths, PseudoColor,
or alternate visuals should update the runtime harness expectations at the same
time.

## Optional window manager

Decoration controls bundle Lucide icons locally. See `THIRD_PARTY_NOTICES.md`
and `LICENSES/Lucide.txt` for the pinned source and redistribution terms.

Decorations are disabled by default. Set `Module.x11WasmWindowManager = true`
before initializing the runtime, or pass `windowManager: true` to
`X11WasmDOM.createBackend({ root, module, windowManager: true })` when creating
a backend directly. The native manager bridge must be included in the build.

The manager decorates normal, dialog, and utility root-level InputOutput
windows. Native identity resolves Tk wrapper/client metadata and excludes
widget children, InputOnly, override-redirect, and menu/popup windows. The
first recognized EWMH window type takes precedence over fallback types.
Closing requests `WM_DELETE_WINDOW`; the application may refuse.
`backend.setWindowManagerEnabled(false)` removes decorations and restores the
original DOM presentation without changing the logical X11 window tree.
Call `backend.setWindowManagerEnabled(true)` to enable them again.

The root-local window taskbar lists mapped and minimized windows. Select a
window there to restore it or explicitly bring an offscreen titlebar back
into the root. Enabling decorations does not move the client, including
windows initially at `y=0` or with explicit position hints.

Move, eight-direction resize, activation, and state controls use the native
manager bridge. Resize and maximization honor normal size hints. Maximize
reserves 28 pixels for the titlebar and 32 for the taskbar; it is separate from
browser fullscreen. Minimize preserves maximization, and Restore returns to
the saved normal geometry. Explicit program geometry while maximized updates
that restore target; root-size reflow does not. Native refusal leaves state
unchanged. Gesture capture is released on cancellation, blur, unmap, destroy,
or disabling decorations.

## Chrome test logs

On POSIX systems, crash reporting or updater descendants may retain Chrome's
stderr pipe and delay Playwright shutdown. Enable per-run regular-file logs
for the same installed Chrome without changing timeouts or exit status:

```sh
PLAYWRIGHT_CHANNEL=chrome PLAYWRIGHT_CHROME_STDIO_DIR=build/internal/chrome-stdio npm run test:browser -- --workers=1
```

The printed `run-*/chrome-<pid>.log` files are retained under that directory.
The wrapper preserves the browser PID, arguments, and CDP pipes. For a custom
Chrome installation, set `PLAYWRIGHT_CHROME_EXECUTABLE` to its absolute path.
Leaving `PLAYWRIGHT_CHROME_STDIO_DIR` unset preserves the normal launch path.

## License

Original project code is licensed under the [MIT License](LICENSE).
X11-derived public headers and generated tables retain their upstream terms.
See [Third-Party Notices](THIRD_PARTY_NOTICES.md) and [LICENSES](LICENSES/).

Library and JavaScript build targets collect the license texts next to the
archive and JS build directory; the browser target also collects them in
`build/browser/`. Include these notices when redistributing build outputs.
To collect only the notices, run `ninja -f build.ninja x11wasm_licenses`.
