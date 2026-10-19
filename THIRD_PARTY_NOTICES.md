# Third-Party Notices

Original x11.wasm implementation, browser JavaScript, build files, and tests
are licensed under the MIT License in `LICENSE`. The upstream materials below
retain their own copyright notices and license terms. The root MIT license
does not replace those terms. Paths in this document refer to the source
repository.

## X11 Public Headers

The public headers under `include/X11/` include X11 material also distributed
by Tcl/Tk in `xlib/X11/`:

<https://github.com/tcltk/tk/tree/d4e3ca45e8bb1c6c5c01f66a196ccb7610409a7c/xlib/X11>

Existing copyright and permission notices are preserved in the headers and
reproduced in `LICENSES/X11.txt`, identified by source file. These include
notices from The Open Group, Digital Equipment Corporation, Oracle, Apollo
Computer, and Hewlett-Packard. Some include restrictions on using the
copyright holders' names in advertising or promotion; they are not all
identical to the root MIT license.

The browser compatibility headers and extensions added by this project do not
remove or replace upstream notices in the files they extend.

## Generated Tables

`src/libX11/tables/KeySyms.inc` contains names and numeric values from the
bundled X11 keysym headers. All 2,297 entries correspond to definitions in
those headers. The applicable header notices are included in
`LICENSES/X11.txt`; conversion to a generated C table does not replace them.

`src/libX11/tables/XColors.inc` contains the normalized RGB table distributed
with Tk as `xlib/rgb.txt`:

<https://github.com/tcltk/tk/blob/d4e3ca45e8bb1c6c5c01f66a196ccb7610409a7c/xlib/rgb.txt>

All 665 normalized names and RGB values match that source. The Tk file records
its changes relative to the X.Org RGB table. Its introductory comments are
reproduced in `LICENSES/X11.txt`, together with the X.Org `rgb` source package's
`COPYING` notices from <https://www.x.org/releases/individual/app/rgb-1.1.0.tar.xz>.
Tk's distribution terms are reproduced verbatim in `LICENSES/Tk.txt`. The
upstream notices and Tk terms are retained for this generated table; it is not
represented as original MIT-licensed project data.

## Window Manager Icons

`js/browser/dom/manager-icons.js` bundles only the `x`, `minus`, `square`, and
`copy` SVG geometry from Lucide, pinned to commit
`02eb9161b57917ccaa262f1bf070507cddde3b08`:

<https://github.com/lucide-icons/lucide/tree/02eb9161b57917ccaa262f1bf070507cddde3b08/icons>

Lucide's ISC license and the Feather-derived icons' MIT notice are reproduced
verbatim in `LICENSES/Lucide.txt`. The icons are bundled locally; no CDN or
runtime download is required.

## Redistribution

Keep the existing notices in source distributions. When distributing the
archive, JavaScript bundles, or a linked WebAssembly application, include
`LICENSE`, this document, and the complete `LICENSES/` directory in the
accompanying license materials. The `x11wasm`, `x11wasm_lib`, and `x11wasm_js`
build targets collect these next to the archive and JS build directory;
`browser` also collects them in `build/browser/`.

`ninja -f build.ninja x11wasm_licenses` collects the notices without requiring
the Emscripten compiler.
