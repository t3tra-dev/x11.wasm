// @ts-check
/*
 * runtime.js --
 *
 *	Runtime state, input, grab, and screen js-library calls for x11.wasm.
 */

(function attachruntimejs() {
  "use strict";

  // @ts-ignore Emscripten defines LibraryManager while processing js-library files.
  const X11WasmLibraryManager = LibraryManager;
  // @ts-ignore Emscripten defines mergeInto while processing js-library files.
  const X11WasmMergeInto = mergeInto;

  X11WasmMergeInto(X11WasmLibraryManager.library, {
  X11WasmJsFlush__deps: ["$X11WasmBridge"],
  X11WasmJsFlush: function () {
    return X11WasmBridge.call("flush", []) | 0;
  },

  X11WasmJsSetRequestState__deps: ["$X11WasmBridge"],
  X11WasmJsSetRequestState: function (
    requestSerial,
    flushSerial,
    syncSerial,
  ) {
    return (
      X11WasmBridge.call("setRequestState", [
        requestSerial,
        flushSerial,
        syncSerial,
      ]) | 0
    );
  },

  X11WasmJsSetSynchronizeState__deps: ["$X11WasmBridge"],
  X11WasmJsSetSynchronizeState: function (enabled) {
    return X11WasmBridge.call("setSynchronizeState", [!!enabled]) | 0;
  },

  X11WasmJsBell__deps: ["$X11WasmBridge"],
  X11WasmJsBell: function (percent, pitch, duration) {
    return X11WasmBridge.call("bell", [percent, pitch, duration]) | 0;
  },

  X11WasmJsForceScreenSaver__deps: ["$X11WasmBridge"],
  X11WasmJsForceScreenSaver: function (mode) {
    return X11WasmBridge.call("forceScreenSaver", [mode]) | 0;
  },

  X11WasmJsGetUserIdleTime__deps: ["$X11WasmBridge"],
  X11WasmJsGetUserIdleTime: function () {
    return X11WasmBridge.call("getUserIdleTime", []) | 0;
  },

  X11WasmJsSetScreenSaverSettings__deps: ["$X11WasmBridge"],
  X11WasmJsSetScreenSaverSettings: function (
    timeout,
    interval,
    preferBlanking,
    allowExposures,
  ) {
    return (
      X11WasmBridge.call("setScreenSaverSettings", [
        timeout,
        interval,
        preferBlanking,
        allowExposures,
      ]) | 0
    );
  },

  X11WasmJsSetScreenSaverSuspended__deps: ["$X11WasmBridge"],
  X11WasmJsSetScreenSaverSuspended: function (suspended) {
    return X11WasmBridge.call("setScreenSaverSuspended", [suspended]) | 0;
  },

  X11WasmJsSetHostAccessControl__deps: ["$X11WasmBridge"],
  X11WasmJsSetHostAccessControl: function (
    enabled,
    hostsUtf8,
    hostsUtf8Length,
    count,
  ) {
    var hosts = "";
    if (hostsUtf8 && hostsUtf8Length > 0) {
      // @ts-ignore UTF8ToString is provided by Emscripten generated code.
      hosts = UTF8ToString(hostsUtf8, hostsUtf8Length);
    }
    return (
      X11WasmBridge.call("setHostAccessControl", [
        !!enabled,
        hosts,
        count,
      ]) | 0
    );
  },

  X11WasmJsSetServerGrabState__deps: ["$X11WasmBridge"],
  X11WasmJsSetServerGrabState: function (depth) {
    return X11WasmBridge.call("setServerGrabState", [depth]) | 0;
  },

  X11WasmJsSetInputFocus__deps: ["$X11WasmBridge"],
  X11WasmJsSetInputFocus: function (focused, window) {
    return X11WasmBridge.call("focusInput", [!!focused, window]) | 0;
  },

  X11WasmJsSetInputContext__deps: ["$X11WasmBridge"],
  X11WasmJsSetInputContext: function (
    focused,
    window,
    hasSpot,
    spotRootX,
    spotRootY,
  ) {
    return (
      X11WasmBridge.call("setInputContext", [
        !!focused,
        window,
        !!hasSpot,
        spotRootX,
        spotRootY,
      ]) | 0
    );
  },

  X11WasmJsSetPointerGrabCursor__deps: ["$X11WasmBridge"],
  X11WasmJsSetPointerGrabCursor: function (enabled, cursor) {
    return (
      X11WasmBridge.call("setPointerGrabCursor", [!!enabled, cursor]) | 0
    );
  },

  X11WasmJsSetPointerGrabState__deps: ["$X11WasmBridge"],
  X11WasmJsSetPointerGrabState: function (
    enabled,
    window,
    ownerEvents,
    eventMask,
    confineWindow,
    cursor,
    pointerMode,
    keyboardMode,
  ) {
    return (
      X11WasmBridge.call("setPointerGrabState", [
        !!enabled,
        window,
        !!ownerEvents,
        eventMask,
        confineWindow,
        cursor,
        pointerMode,
        keyboardMode,
      ]) | 0
    );
  },

  X11WasmJsSetKeyboardGrabState__deps: ["$X11WasmBridge"],
  X11WasmJsSetKeyboardGrabState: function (
    enabled,
    window,
    ownerEvents,
    pointerMode,
    keyboardMode,
  ) {
    return (
      X11WasmBridge.call("setKeyboardGrabState", [
        !!enabled,
        window,
        !!ownerEvents,
        pointerMode,
        keyboardMode,
      ]) | 0
    );
  },

  X11WasmJsSetPointerState__deps: ["$X11WasmBridge"],
  X11WasmJsSetPointerState: function (
    rootX,
    rootY,
    window,
    state,
    captureWindow,
  ) {
    return (
      X11WasmBridge.call("setPointerState", [
        rootX,
        rootY,
        window,
        state,
        captureWindow,
      ]) | 0
    );
  },

  X11WasmJsWarpPointer__deps: ["$X11WasmBridge"],
  X11WasmJsWarpPointer: function (rootX, rootY, targetWindow, time) {
    return (
      X11WasmBridge.call("warpPointer", [rootX, rootY, targetWindow, time]) |
      0
    );
  },
  });
})();
