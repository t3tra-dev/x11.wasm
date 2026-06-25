// @ts-check
/*
 * private.js --
 *
 *	Shared private namespace for the x11.wasm DOM backend shards.
 */

(function attachX11WasmDOMPrivate(global) {
  "use strict";

  global["X11WasmDOMPrivate"] = global["X11WasmDOMPrivate"] || {};
})(typeof globalThis !== "undefined" ? globalThis : window);
