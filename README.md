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

## License

Original project code is licensed under the [MIT License](LICENSE).
X11-derived public headers and generated tables retain their upstream terms.
See [Third-Party Notices](THIRD_PARTY_NOTICES.md) and [LICENSES](LICENSES/).

Library and JavaScript build targets collect the license texts next to the
archive and JS build directory; the browser target also collects them in
`build/browser/`. Include these notices when redistributing build outputs.
To collect only the notices, run `ninja -f build.ninja x11wasm_licenses`.
