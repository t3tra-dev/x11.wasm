// @ts-check
/*
 * draw.js --
 *
 *	Drawing and graphics-context js-library calls for x11.wasm.
 */

(function attachdrawjs() {
  "use strict";

  // @ts-ignore Emscripten defines LibraryManager while processing js-library files.
  const X11WasmLibraryManager = LibraryManager;
  // @ts-ignore Emscripten defines mergeInto while processing js-library files.
  const X11WasmMergeInto = mergeInto;

  X11WasmMergeInto(X11WasmLibraryManager.library, {
  X11WasmJsDrawLine__deps: ["$X11WasmBridge"],
  X11WasmJsDrawLine: function (id, pixel, x1, y1, x2, y2) {
    return X11WasmBridge.call("drawLine", [id, pixel, x1, y1, x2, y2]) | 0;
  },

  X11WasmJsDrawRectangle__deps: ["$X11WasmBridge"],
  X11WasmJsDrawRectangle: function (id, pixel, x, y, width, height, fill) {
    return (
      X11WasmBridge.call("drawRectangle", [
        id,
        pixel,
        x,
        y,
        width,
        height,
        !!fill,
      ]) | 0
    );
  },

  X11WasmJsDrawArc__deps: ["$X11WasmBridge"],
  X11WasmJsDrawArc: function (
    id,
    pixel,
    x,
    y,
    width,
    height,
    angle1,
    angle2,
    fill,
    pieSlice,
    lineWidth,
  ) {
    return (
      X11WasmBridge.call("drawArc", [
        id,
        pixel,
        x,
        y,
        width,
        height,
        angle1,
        angle2,
        !!fill,
        !!pieSlice,
        lineWidth,
      ]) | 0
    );
  },

  X11WasmJsFillPolygon__deps: ["$X11WasmBridge"],
  X11WasmJsFillPolygon: function (id, pixel, points, pointCount, fillRule) {
    /** @type {number[]} */
    var coordinates = [];
    if (points && pointCount > 0) {
      var offset = points >> 2;
      for (var i = 0; i < pointCount; i++) {
        coordinates.push(HEAP32[offset + i * 2], HEAP32[offset + i * 2 + 1]);
      }
    }
    return (
      X11WasmBridge.call("fillPolygon", [
        id,
        pixel,
        coordinates,
        fillRule,
      ]) | 0
    );
  },

  X11WasmJsDrawText__deps: ["$X11WasmBridge"],
  X11WasmJsDrawText: function (id, pixel, x, y, utf8, utf8Length) {
    var text = "";
    if (utf8 && utf8Length > 0) {
      // @ts-ignore UTF8ToString is provided by Emscripten generated code.
      text = UTF8ToString(utf8, utf8Length);
    }
    return X11WasmBridge.call("drawText", [id, pixel, x, y, text]) | 0;
  },

  X11WasmJsMeasureText__deps: ["$X11WasmBridge"],
  X11WasmJsMeasureText: function (
    fontPixelSize,
    fontFamilyUtf8,
    utf8,
    utf8Length,
  ) {
    var fontFamily = "sans-serif";
    var text = "";
    if (fontFamilyUtf8) {
      // @ts-ignore UTF8ToString is provided by Emscripten generated code.
      fontFamily = UTF8ToString(fontFamilyUtf8);
    }
    if (utf8 && utf8Length > 0) {
      // @ts-ignore UTF8ToString is provided by Emscripten generated code.
      text = UTF8ToString(utf8, utf8Length);
    }
    return (
      X11WasmBridge.call("measureText", [fontPixelSize, fontFamily, text]) | 0
    );
  },

  X11WasmJsPutImage__deps: ["$X11WasmBridge"],
  X11WasmJsPutImage: function (
    id,
    data,
    dataLength,
    srcX,
    srcY,
    destX,
    destY,
    width,
    height,
    bytesPerLine,
    bitsPerPixel,
  ) {
    var bytes = null;
    if (data && dataLength > 0) {
      // @ts-ignore HEAPU8 is provided by Emscripten generated code.
      bytes = HEAPU8.subarray(data, data + dataLength);
    }
    return (
      X11WasmBridge.call("putImage", [
        id,
        bytes,
        srcX,
        srcY,
        destX,
        destY,
        width,
        height,
        bytesPerLine,
        bitsPerPixel,
      ]) | 0
    );
  },

  X11WasmJsGetImage__deps: ["$X11WasmBridge"],
  X11WasmJsGetImage: function (
    id,
    srcX,
    srcY,
    width,
    height,
    data,
    dataLength,
    bytesPerLine,
    bitsPerPixel,
  ) {
    var bytes = null;
    if (data && dataLength > 0) {
      // @ts-ignore HEAPU8 is provided by Emscripten generated code.
      bytes = HEAPU8.subarray(data, data + dataLength);
    }
    return (
      X11WasmBridge.call("getImage", [
        id,
        srcX,
        srcY,
        width,
        height,
        bytes,
        bytesPerLine,
        bitsPerPixel,
      ]) | 0
    );
  },

  X11WasmJsSetClipRect__deps: ["$X11WasmBridge"],
  X11WasmJsSetClipRect: function (enabled, x, y, width, height) {
    return (
      X11WasmBridge.call("setClipRect", [
        !!enabled,
        x,
        y,
        width,
        height,
      ]) | 0
    );
  },

  X11WasmJsSetClipRects__deps: ["$X11WasmBridge"],
  X11WasmJsSetClipRects: function (
    enabled,
    rectangles,
    count,
    xOrigin,
    yOrigin,
  ) {
    var rects = [];
    var base = rectangles >> 1;
    var rectCount = Math.max(0, count | 0);
    for (var i = 0; rectangles && i < rectCount; i++) {
      var offset = base + i * 4;
      // XRectangle is short x/y plus unsigned short width/height.
      rects.push({
        x: HEAP16[offset] + xOrigin,
        y: HEAP16[offset + 1] + yOrigin,
        width: HEAPU16[offset + 2],
        height: HEAPU16[offset + 3],
      });
    }
    return X11WasmBridge.call("setClipRects", [!!enabled, rects]) | 0;
  },

  X11WasmJsSetClipMask__deps: ["$X11WasmBridge"],
  X11WasmJsSetClipMask: function (enabled, pixmap, xOrigin, yOrigin) {
    return (
      X11WasmBridge.call("setClipMask", [
        !!enabled,
        pixmap,
        xOrigin,
        yOrigin,
      ]) | 0
    );
  },

  X11WasmJsSetDrawStyle__deps: ["$X11WasmBridge"],
  X11WasmJsSetDrawStyle: function (
    functionId,
    lineWidth,
    lineStyle,
    capStyle,
    joinStyle,
    dashOffset,
    dashLength,
    dashPattern,
    dashCount,
    fillStyle,
    stipple,
    tile,
    tsOriginX,
    tsOriginY,
    backgroundPixel,
    fontPixelSize,
    fontFamily,
    planeMask,
  ) {
    var dashes = [];
    var count = Math.max(0, dashCount | 0);
    var family = fontFamily ? UTF8ToString(fontFamily) : "";
    for (var i = 0; dashPattern && i < count; i++) {
      dashes.push(HEAPU8[dashPattern + i]);
    }
    return (
      X11WasmBridge.call("setDrawStyle", [
        functionId,
        lineWidth,
        lineStyle,
        capStyle,
        joinStyle,
        dashOffset,
        dashLength,
        dashes,
        fillStyle,
        stipple,
        tile,
        tsOriginX,
        tsOriginY,
        backgroundPixel,
        fontPixelSize,
        family,
        planeMask,
      ]) | 0
    );
  },

  X11WasmJsSetWindowStyle__deps: ["$X11WasmBridge"],
  X11WasmJsSetWindowStyle: function (
    id,
    hasBackground,
    backgroundPixel,
    backgroundPixmap,
    hasBorder,
    borderPixel,
    borderPixmap,
    hasCursor,
    cursorShape,
  ) {
    return (
      X11WasmBridge.call("setWindowStyle", [
        id,
        !!hasBackground,
        backgroundPixel,
        backgroundPixmap,
        !!hasBorder,
        borderPixel,
        borderPixmap,
        !!hasCursor,
        cursorShape,
      ]) | 0
    );
  },

  X11WasmJsCreateCursor__deps: ["$X11WasmBridge"],
  X11WasmJsCreateCursor: function (
    id,
    rgba,
    dataLength,
    width,
    height,
    hotX,
    hotY,
  ) {
    var bytes = new Uint8Array(HEAPU8.subarray(rgba, rgba + dataLength));
    return (
      X11WasmBridge.call("createCursor", [
        id,
        bytes,
        width,
        height,
        hotX,
        hotY,
      ]) | 0
    );
  },
  });
})();
