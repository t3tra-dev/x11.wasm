// @ts-check
/*
 * wm.js --
 *
 *	Window-manager metadata js-library calls for x11.wasm.
 */

(function attachwmjs() {
  "use strict";

  // @ts-ignore Emscripten defines LibraryManager while processing js-library files.
  const X11WasmLibraryManager = LibraryManager;
  // @ts-ignore Emscripten defines mergeInto while processing js-library files.
  const X11WasmMergeInto = mergeInto;

  X11WasmMergeInto(X11WasmLibraryManager.library, {
  X11WasmJsSetWindowTitle__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowTitle: function(id, utf8, utf8Length, updatesDocument) {
        var title = "";
        if (utf8 && utf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          title = UTF8ToString(utf8, utf8Length);
        }
        return (
          X11WasmBridge.call("setWindowTitle", [id, title, !!updatesDocument]) | 0
        );
      },

  X11WasmJsSetWindowIconName__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowIconName: function(id, utf8, utf8Length) {
        var iconName = "";
        if (utf8 && utf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          iconName = UTF8ToString(utf8, utf8Length);
        }
        return X11WasmBridge.call("setWindowIconName", [id, iconName]) | 0;
      },

  X11WasmJsClearWindowMetadata__deps: ["$X11WasmBridge"],
  X11WasmJsClearWindowMetadata: function(id, updatesDocument) {
        return (
          X11WasmBridge.call("clearWindowMetadata", [
            id,
            !!updatesDocument,
          ]) | 0
        );
      },

  X11WasmJsSetWindowTransientFor__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowTransientFor: function(id, transientFor) {
        return (
          X11WasmBridge.call("setWindowTransientFor", [id, transientFor]) | 0
        );
      },

  X11WasmJsSetWindowCommand__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowCommand: function(id, utf8, utf8Length) {
        var command = "";
        if (utf8 && utf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          command = UTF8ToString(utf8, utf8Length);
        }
        return X11WasmBridge.call("setWindowCommand", [id, command]) | 0;
      },

  X11WasmJsSetWindowClientMachine__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowClientMachine: function(id, utf8, utf8Length) {
        var clientMachine = "";
        if (utf8 && utf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          clientMachine = UTF8ToString(utf8, utf8Length);
        }
        return (
          X11WasmBridge.call("setWindowClientMachine", [id, clientMachine]) | 0
        );
      },

  X11WasmJsSetWindowClassHint__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowClassHint: function(
        id,
        nameUtf8,
        nameUtf8Length,
        classUtf8,
        classUtf8Length,
      ) {
        var className = "";
        var classValue = "";
        if (nameUtf8 && nameUtf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          className = UTF8ToString(nameUtf8, nameUtf8Length);
        }
        if (classUtf8 && classUtf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          classValue = UTF8ToString(classUtf8, classUtf8Length);
        }
        return (
          X11WasmBridge.call("setWindowClassHint", [
            id,
            className,
            classValue,
          ]) | 0
        );
      },

  X11WasmJsSetWindowAlpha__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowAlpha: function(id, alpha) {
        return X11WasmBridge.call("setWindowAlpha", [id, alpha]) | 0;
      },

  X11WasmJsSetClipboardText__deps: ["$X11WasmBridge"],
  X11WasmJsSetClipboardText: function(utf8, utf8Length) {
        var text = "";
        if (utf8 && utf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          text = UTF8ToString(utf8, utf8Length);
        }
        return X11WasmBridge.call("setClipboardText", [text]) | 0;
      },

  X11WasmJsSetSelectionOwner__deps: ["$X11WasmBridge"],
  X11WasmJsSetSelectionOwner: function(
        selectionUtf8,
        selectionUtf8Length,
        owner,
        time,
      ) {
        var selection = "";
        if (selectionUtf8 && selectionUtf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          selection = UTF8ToString(selectionUtf8, selectionUtf8Length);
        }
        return (
          X11WasmBridge.call("setSelectionOwner", [selection, owner, time]) | 0
        );
      },

  X11WasmJsSetWindowWMHints__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowWMHints: function(
        id,
        flags,
        input,
        initialState,
        iconPixmap,
        iconWindow,
        iconX,
        iconY,
        iconMask,
        windowGroup,
      ) {
        return (
          X11WasmBridge.call("setWindowWMHints", [
            id,
            flags,
            input,
            initialState,
            iconPixmap,
            iconWindow,
            iconX,
            iconY,
            iconMask,
            windowGroup,
          ]) | 0
        );
      },

  X11WasmJsSetWindowIconSizes__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowIconSizes: function(id, sizesUtf8, sizesUtf8Length, count) {
        var sizes = "";
        if (sizesUtf8 && sizesUtf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          sizes = UTF8ToString(sizesUtf8, sizesUtf8Length);
        }
        return X11WasmBridge.call("setWindowIconSizes", [id, sizes, count]) | 0;
      },

  X11WasmJsSetWindowIconPhoto__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowIconPhoto: function(
        id,
        imageCount,
        sizesUtf8,
        sizesUtf8Length,
        rgba,
        rgbaLength,
        width,
        height,
      ) {
        var sizes = "";
        var bytes = null;
        if (sizesUtf8 && sizesUtf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          sizes = UTF8ToString(sizesUtf8, sizesUtf8Length);
        }
        if (rgba && rgbaLength > 0) {
          // @ts-ignore HEAPU8 is provided by Emscripten generated code.
          bytes = new Uint8Array(HEAPU8.subarray(rgba, rgba + rgbaLength));
        }
        return (
          X11WasmBridge.call("setWindowIconPhoto", [
            id,
            imageCount,
            sizes,
            bytes,
            width,
            height,
          ]) | 0
        );
      },

  X11WasmJsSetWindowProtocols__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowProtocols: function(id, utf8, utf8Length) {
        var protocols = "";
        if (utf8 && utf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          protocols = UTF8ToString(utf8, utf8Length);
        }
        return X11WasmBridge.call("setWindowProtocols", [id, protocols]) | 0;
      },

  X11WasmJsSetWindowTypes__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowTypes: function(id, utf8, utf8Length) {
        var types = "";
        if (utf8 && utf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          types = UTF8ToString(utf8, utf8Length);
        }
        return X11WasmBridge.call("setWindowTypes", [id, types]) | 0;
      },

  X11WasmJsSetWindowColormapWindows__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowColormapWindows: function(id, utf8, utf8Length) {
        var colormapWindows = "";
        if (utf8 && utf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          colormapWindows = UTF8ToString(utf8, utf8Length);
        }
        return (
          X11WasmBridge.call("setWindowColormapWindows", [
            id,
            colormapWindows,
          ]) | 0
        );
      },

  X11WasmJsSetWindowStandardColormaps__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowStandardColormaps: function(
        id,
        propertyUtf8,
        propertyUtf8Length,
        mapsUtf8,
        mapsUtf8Length,
        count,
      ) {
        var property = "";
        var maps = "";
        if (propertyUtf8 && propertyUtf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          property = UTF8ToString(propertyUtf8, propertyUtf8Length);
        }
        if (mapsUtf8 && mapsUtf8Length > 0) {
          // @ts-ignore UTF8ToString is provided by Emscripten generated code.
          maps = UTF8ToString(mapsUtf8, mapsUtf8Length);
        }
        return (
          X11WasmBridge.call("setWindowStandardColormaps", [
            id,
            property,
            maps,
            count,
          ]) | 0
        );
      },

  X11WasmJsSetWindowState__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowState: function(id, topmost, zoomed, fullscreen) {
        return (
          X11WasmBridge.call("setWindowState", [
            id,
            !!topmost,
            !!zoomed,
            !!fullscreen,
          ]) | 0
        );
      },

  X11WasmJsSetWindowWmState__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowWmState: function(id, state) {
        return X11WasmBridge.call("setWindowWmState", [id, state]) | 0;
      },

  X11WasmJsSetWindowSizeHints__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowSizeHints: function(
        id,
        flags,
        x,
        y,
        width,
        height,
        minWidth,
        minHeight,
        maxWidth,
        maxHeight,
        widthInc,
        heightInc,
        minAspectX,
        minAspectY,
        maxAspectX,
        maxAspectY,
        baseWidth,
        baseHeight,
        winGravity,
      ) {
        return (
          X11WasmBridge.call("setWindowSizeHints", [
            id,
            flags,
            x,
            y,
            width,
            height,
            minWidth,
            minHeight,
            maxWidth,
            maxHeight,
            widthInc,
            heightInc,
            minAspectX,
            minAspectY,
            maxAspectX,
            maxAspectY,
            baseWidth,
            baseHeight,
            winGravity,
          ]) | 0
        );
      },

  X11WasmJsSetWindowZoomHints__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowZoomHints: function(
        id,
        flags,
        x,
        y,
        width,
        height,
        minWidth,
        minHeight,
        maxWidth,
        maxHeight,
        widthInc,
        heightInc,
        minAspectX,
        minAspectY,
        maxAspectX,
        maxAspectY,
        baseWidth,
        baseHeight,
        winGravity,
      ) {
        return (
          X11WasmBridge.call("setWindowZoomHints", [
            id,
            flags,
            x,
            y,
            width,
            height,
            minWidth,
            minHeight,
            maxWidth,
            maxHeight,
            widthInc,
            heightInc,
            minAspectX,
            minAspectY,
            maxAspectX,
            maxAspectY,
            baseWidth,
            baseHeight,
            winGravity,
          ]) | 0
        );
      },
  });
})();
