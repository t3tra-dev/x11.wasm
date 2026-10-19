// @ts-check
/*
 * window.js --
 *
 *	Window and resource js-library calls for x11.wasm.
 */

(function attachwindowjs() {
  "use strict";

  // @ts-ignore Emscripten defines LibraryManager while processing js-library files.
  const X11WasmLibraryManager = LibraryManager;
  // @ts-ignore Emscripten defines mergeInto while processing js-library files.
  const X11WasmMergeInto = mergeInto;

  X11WasmMergeInto(X11WasmLibraryManager.library, {
  X11WasmJsInit__deps: ["$X11WasmBridge"],
  X11WasmJsInit: function (width, height, rootXid) {
    return X11WasmBridge.call("init", [width, height, rootXid]) | 0;
  },

  X11WasmJsGetRootWidth__deps: ["$X11WasmBridge"],
  X11WasmJsGetRootWidth: function () {
    return X11WasmBridge.call("getRootWidth", []) | 0;
  },

  X11WasmJsGetRootHeight__deps: ["$X11WasmBridge"],
  X11WasmJsGetRootHeight: function () {
    return X11WasmBridge.call("getRootHeight", []) | 0;
  },

  X11WasmJsCreateWindow__deps: ["$X11WasmBridge"],
  X11WasmJsCreateWindow: function (
    id,
    parent,
    x,
    y,
    width,
    height,
    depth,
    eventMask,
    clazz,
  ) {
    return (
      X11WasmBridge.call("createWindow", [
        id,
        parent,
        x,
        y,
        width,
        height,
        depth,
        eventMask,
        clazz,
        true,
      ]) | 0
    );
  },

  X11WasmJsCreatePixmap__deps: ["$X11WasmBridge"],
  X11WasmJsCreatePixmap: function (id, drawable, width, height, depth) {
    return (
      X11WasmBridge.call("createPixmap", [
        id,
        drawable,
        width,
        height,
        depth,
      ]) | 0
    );
  },

  X11WasmJsDestroyResource__deps: ["$X11WasmBridge"],
  X11WasmJsDestroyResource: function (id) {
    return X11WasmBridge.call("destroyResource", [id]) | 0;
  },

  X11WasmJsMapWindow__deps: ["$X11WasmBridge"],
  X11WasmJsMapWindow: function (id) {
    return X11WasmBridge.call("mapWindow", [id]) | 0;
  },

  X11WasmJsUnmapWindow__deps: ["$X11WasmBridge"],
  X11WasmJsUnmapWindow: function (id) {
    return X11WasmBridge.call("unmapWindow", [id]) | 0;
  },

  X11WasmJsSetWindowEventMask__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowEventMask: function (
    id,
    eventMask,
    doNotPropagateMask,
  ) {
    return (
      X11WasmBridge.call("setWindowEventMask", [
        id,
        eventMask,
        doNotPropagateMask,
      ]) | 0
    );
  },

  X11WasmJsSetWindowColormap__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowColormap: function (id, colormap) {
    return X11WasmBridge.call("setWindowColormap", [id, colormap]) | 0;
  },

  X11WasmJsSetWindowAttributes__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowAttributes: function (
    id,
    overrideRedirect,
    saveUnder,
    bitGravity,
    winGravity,
    backingStore,
    backingPlanes,
    backingPixel,
  ) {
    return (
      X11WasmBridge.call("setWindowAttributes", [
        id,
        overrideRedirect,
        saveUnder,
        bitGravity,
        winGravity,
        backingStore,
        backingPlanes,
        backingPixel,
      ]) | 0
    );
  },

  X11WasmJsConfigureWindow__deps: ["$X11WasmBridge"],
  X11WasmJsConfigureWindow: function (
    id,
    x,
    y,
    width,
    height,
    borderWidth,
    valueMask,
  ) {
    return (
      X11WasmBridge.call("configureWindow", [
        id,
        x,
        y,
        width,
        height,
        borderWidth,
        valueMask,
      ]) | 0
    );
  },

  X11WasmJsRestackWindow__deps: ["$X11WasmBridge"],
  X11WasmJsRestackWindow: function (id, previousSibling) {
    return X11WasmBridge.call("restackWindow", [id, previousSibling]) | 0;
  },

  X11WasmJsReparentWindow__deps: ["$X11WasmBridge"],
  X11WasmJsReparentWindow: function (id, parent, x, y) {
    return X11WasmBridge.call("reparentWindow", [id, parent, x, y]) | 0;
  },

  X11WasmJsClearWindow__deps: ["$X11WasmBridge"],
  X11WasmJsClearWindow: function (id) {
    return X11WasmBridge.call("clearWindow", [id]) | 0;
  },

  X11WasmJsClearArea__deps: ["$X11WasmBridge"],
  X11WasmJsClearArea: function (id, x, y, width, height) {
    return X11WasmBridge.call("clearWindowArea", [id, x, y, width, height]) | 0;
  },

  X11WasmJsCopyArea__deps: ["$X11WasmBridge"],
  X11WasmJsCopyArea: function (
    source,
    target,
    srcX,
    srcY,
    width,
    height,
    destX,
    destY,
  ) {
    return (
      X11WasmBridge.call("copyArea", [
        source,
        target,
        srcX,
        srcY,
        width,
        height,
        destX,
        destY,
      ]) | 0
    );
  },

  X11WasmJsCopyPlane__deps: ["$X11WasmBridge"],
  X11WasmJsCopyPlane: function (
    source,
    target,
    srcX,
    srcY,
    width,
    height,
    destX,
    destY,
    foreground,
    background,
    plane,
    transparentZero,
  ) {
    return (
      X11WasmBridge.call("copyPlane", [
        source,
        target,
        srcX,
        srcY,
        width,
        height,
        destX,
        destY,
        foreground,
        background,
        plane,
        !!transparentZero,
      ]) | 0
    );
  },
  });
})();
