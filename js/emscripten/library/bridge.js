// @ts-check
/*
 * bridge.js --
 *
 *	Shared bridge and backend diagnostics for x11.wasm js-library calls.
 */

(function attachbridgejs() {
  "use strict";

  // @ts-ignore Emscripten defines LibraryManager while processing js-library files.
  const X11WasmLibraryManager = LibraryManager;
  // @ts-ignore Emscripten defines mergeInto while processing js-library files.
  const X11WasmMergeInto = mergeInto;

  X11WasmMergeInto(X11WasmLibraryManager.library, {
  $X11WasmBridge: {
    backend: null,
    diagnostics: {
      missingBackendCount: 0,
      missingMethodCount: 0,
      lastMissingMethod: "",
      lastMissingReason: "",
      missingMethods: {},
      requiredMethods: [],
      missingRequiredMethods: [],
      backendComplete: false,
      backendValidationCount: 0,
      warnedMissing: {},
    },
    requiredMethods: [
      "bell",
      "clearWindow",
      "clearWindowArea",
      "cancelComposition",
      "clearWindowMetadata",
      "configureWindow",
      "copyArea",
      "copyPlane",
      "createCursor",
      "createPixmap",
      "createWindow",
      "destroyResource",
      "drawArc",
      "drawLine",
      "drawRectangle",
      "drawText",
      "fillPolygon",
      "flush",
      "focusInput",
      "forceScreenSaver",
      "getUserIdleTime",
      "getImage",
      "getRootHeight",
      "getRootWidth",
      "init",
      "mapWindow",
      "measureText",
      "putImage",
      "reparentWindow",
      "restackWindow",
      "setClipMask",
      "setClipRect",
      "setClipRects",
      "setClipboardText",
      "setDrawStyle",
      "setHostAccessControl",
      "setInputContext",
      "setKeyboardGrabState",
      "setPointerGrabCursor",
      "setPointerGrabState",
      "setPointerState",
      "setRequestState",
      "setScreenSaverSettings",
      "setScreenSaverSuspended",
      "setSelectionOwner",
      "setServerGrabState",
      "setSynchronizeState",
      "setWindowAlpha",
      "setWindowAttributes",
      "setWindowClassHint",
      "setWindowClientMachine",
      "setWindowColormap",
      "setWindowColormapWindows",
      "setWindowCommand",
      "setWindowEventMask",
      "setWindowIconName",
      "setWindowIconPhoto",
      "setWindowIconSizes",
      "setWindowProtocols",
      "setWindowTypes",
      "setWindowSizeHints",
      "setWindowStandardColormaps",
      "setWindowState",
      "setWindowStyle",
      "setWindowTitle",
      "setWindowTransientFor",
      "setWindowWMHints",
      "setWindowWmState",
      "setWindowZoomHints",
      "textRunWidths",
      "unmapWindow",
      "warpPointer",
    ],

    getModule: function () {
      // @ts-ignore Module is provided by Emscripten generated code.
      var module = typeof Module !== "undefined" ? Module : null;
      if (
        module &&
        typeof module["x11WasmValidateBridge"] !== "function"
      ) {
        module["x11WasmValidateBridge"] = function () {
          return X11WasmBridge.validateBackend() | 0;
        };
      }
      return module;
    },

    updateRequiredMethodDiagnostics: function (backend) {
      var diagnostics = X11WasmBridge.getDiagnostics();
      var missing = [];
      var required = X11WasmBridge.requiredMethods;
      var i;

      for (i = 0; i < required.length; i++) {
        if (!backend || typeof backend[required[i]] !== "function") {
          missing.push(required[i]);
        }
      }
      diagnostics.requiredMethods = required.slice();
      diagnostics.missingRequiredMethods = missing;
      diagnostics.backendComplete = missing.length === 0;
      diagnostics.backendValidationCount += 1;
      X11WasmBridge.getDiagnostics();
      return diagnostics.backendComplete ? 1 : 0;
    },

    validateBackend: function () {
      return X11WasmBridge.updateRequiredMethodDiagnostics(
        X11WasmBridge.getBackend(),
      );
    },

    getBackend: function () {
      if (X11WasmBridge.backend) {
        return X11WasmBridge.backend;
      }
      var module = X11WasmBridge.getModule();
      if (module && module["x11WasmBackend"]) {
        X11WasmBridge.backend = module["x11WasmBackend"];
        X11WasmBridge.updateRequiredMethodDiagnostics(X11WasmBridge.backend);
        return X11WasmBridge.backend;
      }
      if (module && typeof module["x11WasmCreateBackend"] === "function") {
        X11WasmBridge.backend = module["x11WasmCreateBackend"](module);
        module["x11WasmBackend"] = X11WasmBridge.backend;
        X11WasmBridge.updateRequiredMethodDiagnostics(X11WasmBridge.backend);
        return X11WasmBridge.backend;
      }
      if (
        typeof globalThis !== "undefined" &&
        globalThis["X11WasmDOM"] &&
        typeof globalThis["X11WasmDOM"]["createBackend"] === "function"
      ) {
        X11WasmBridge.backend = globalThis["X11WasmDOM"]["createBackend"]({
          root:
            module && module["x11WasmRoot"]
              ? module["x11WasmRoot"]
              : undefined,
          module: module || undefined,
});
        if (module) {
          module["x11WasmBackend"] = X11WasmBridge.backend;
        }
      }
      if (X11WasmBridge.backend) {
        X11WasmBridge.updateRequiredMethodDiagnostics(X11WasmBridge.backend);
      }
      return X11WasmBridge.backend;
    },

    getDiagnostics: function () {
      var module = X11WasmBridge.getModule();
      if (module) {
        module["x11WasmBridgeDiagnostics"] = X11WasmBridge.diagnostics;
      }
      return X11WasmBridge.diagnostics;
    },

    recordMissing: function (method, reason) {
      var diagnostics = X11WasmBridge.getDiagnostics();
      var key = reason + ":" + method;

      diagnostics.lastMissingMethod = method;
      diagnostics.lastMissingReason = reason;
      if (reason === "backend") {
        diagnostics.missingBackendCount += 1;
      } else {
        diagnostics.missingMethodCount += 1;
        diagnostics.missingMethods[method] =
          (diagnostics.missingMethods[method] || 0) + 1;
      }
      if (
        !diagnostics.warnedMissing[key] &&
        typeof console !== "undefined" &&
        typeof console.warn === "function"
      ) {
        diagnostics.warnedMissing[key] = true;
        console.warn(
          "X11 wasm bridge missing " + reason + " for " + method,
        );
      }
      X11WasmBridge.getDiagnostics();
    },

    call: function (method, args, unavailableResult) {
      var unavailable = unavailableResult === undefined ? 0 : unavailableResult;
      var backend = X11WasmBridge.getBackend();
      if (!backend) {
        X11WasmBridge.recordMissing(method, "backend");
        return unavailable;
      }
      if (typeof backend[method] !== "function") {
        X11WasmBridge.recordMissing(method, "method");
        return unavailable;
      }
      var result = backend[method].apply(backend, args);
      return result === undefined ? unavailable : result;
    },
  },
  });
})();
