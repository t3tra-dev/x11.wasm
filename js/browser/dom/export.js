// @ts-check
/*
 * export.js --
 *
 *	Public attachment point for the x11.wasm DOM/canvas backend.
 */

(function attachX11WasmDOMBackendExport(global) {
  "use strict";

  const privateApi =
    global["X11WasmDOMPrivate"] || (global["X11WasmDOMPrivate"] = {});
  const { X11WasmDOMBackend } = privateApi;

  if (!X11WasmDOMBackend) {
    throw new Error("X11WasmDOMBackend core must be loaded first");
  }

  function createBackend(options) {
    return new X11WasmDOMBackend(options);
  }

  global.X11WasmDOM = {
    createBackend,
    X11WasmDOMBackend,
  };
})(typeof globalThis !== "undefined" ? globalThis : window);
