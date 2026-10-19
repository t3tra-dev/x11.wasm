// @ts-check
/*
 * helpers.js --
 *
 *	Shared helpers used by the x11.wasm DOM backend.
 */

(function attachX11WasmDOMHelpers(global) {
  "use strict";

  /** @typedef {"" | "normal" | "iconic" | "withdrawn"} X11WasmWmState */

  const privateApi =
    global["X11WasmDOMPrivate"] || (global["X11WasmDOMPrivate"] = {});
  const {
    RESOURCE_ATTR,
    STYLE_ID,
    KEY_PRESS,
    KEY_RELEASE,
    BUTTON_PRESS,
    BUTTON_RELEASE,
    MOTION_NOTIFY,
    ENTER_NOTIFY,
    LEAVE_NOTIFY,
    FOCUS_IN,
    FOCUS_OUT,
    WM_INPUT_HINT,
    WM_STATE_HINT,
    WM_ICON_PIXMAP_HINT,
    WM_ICON_WINDOW_HINT,
    WM_ICON_POSITION_HINT,
    WM_ICON_MASK_HINT,
    WM_WINDOW_GROUP_HINT,
    WM_WITHDRAWN_STATE,
    WM_NORMAL_STATE,
    WM_ICONIC_STATE,
    P_POSITION,
    P_SIZE,
    P_MIN_SIZE,
    P_MAX_SIZE,
    P_RESIZE_INC,
    P_ASPECT,
    P_BASE_SIZE,
    P_WIN_GRAVITY,
    NORMAL_SIZE_HINT_ATTRIBUTES,
    ZOOM_SIZE_HINT_ATTRIBUTES,
    CW_X,
    CW_Y,
    CW_WIDTH,
    CW_HEIGHT,
    CW_BORDER_WIDTH,
    FORGET_GRAVITY,
    NORTH_WEST_GRAVITY,
    NORTH_GRAVITY,
    NORTH_EAST_GRAVITY,
    WEST_GRAVITY,
    CENTER_GRAVITY,
    EAST_GRAVITY,
    SOUTH_WEST_GRAVITY,
    SOUTH_GRAVITY,
    SOUTH_EAST_GRAVITY,
    SHIFT_MASK,
    CONTROL_MASK,
    MOD1_MASK,
    MOD4_MASK,
    BUTTON1_MASK,
    BUTTON2_MASK,
    BUTTON3_MASK,
    BUTTON4_MASK,
    BUTTON5_MASK,
    UNICODE_KEYSYM_BASE,
    COMPOSITION_START,
    COMPOSITION_UPDATE,
    COMPOSITION_END,
    GX_CLEAR,
    GX_AND,
    GX_AND_REVERSE,
    GX_COPY,
    GX_AND_INVERTED,
    GX_NOOP,
    GX_XOR,
    GX_OR,
    GX_NOR,
    GX_EQUIV,
    GX_INVERT,
    GX_OR_REVERSE,
    GX_COPY_INVERTED,
    GX_OR_INVERTED,
    GX_NAND,
    GX_SET,
    LINE_SOLID,
    LINE_DOUBLE_DASH,
    FILL_SOLID,
    FILL_TILED,
    FILL_STIPPLED,
    FILL_OPAQUE_STIPPLED,
    CAP_ROUND,
    CAP_PROJECTING,
    JOIN_ROUND,
    JOIN_BEVEL,
    WHEEL_PIXEL_THRESHOLD,
    WHEEL_LINE_SCALE,
    WHEEL_PAGE_SCALE,
    DOM_KEY_LOCATION_RIGHT,
    KEY_SYMS,
    KEY_CODES,
    RIGHT_MODIFIER_KEY_SYMS,
    RIGHT_MODIFIER_KEY_CODES,
  } = privateApi;

  function positiveInt(value, fallback) {
    if (!Number.isFinite(value) || value <= 0) {
      return fallback;
    }
    return Math.max(1, Math.floor(value));
  }

  /**
   * @param {number} value
   * @param {number=} fallback
   * @returns {number}
   */
  function unsignedInt(value, fallback = 0) {
    if (!Number.isFinite(value)) {
      return fallback >>> 0;
    }
    return Math.trunc(value) >>> 0;
  }

  /**
   * @param {number} state
   * @returns {X11WasmWmState}
   */
  function wmStateName(state) {
    switch (state) {
      case WM_NORMAL_STATE:
        return "normal";
      case WM_ICONIC_STATE:
        return "iconic";
      case WM_WITHDRAWN_STATE:
        return "withdrawn";
      default:
        return "";
    }
  }

  /**
   * @param {number} pixel
   * @returns {string}
   */
  function pixelToCss(pixel) {
    const [red, green, blue] = pixelToRgb(pixel);
    return `rgb(${red} ${green} ${blue})`;
  }

  /**
   * @param {number} pixel
   * @returns {[number, number, number]}
   */
  function pixelToRgb(pixel) {
    const value = Number(pixel) >>> 0;
    const red = (value >> 16) & 0xff;
    const green = (value >> 8) & 0xff;
    const blue = value & 0xff;
    return [red, green, blue];
  }

  /**
   * @param {number} pixel
   * @returns {[number, number, number]}
   */
  function rgb565ToRgb(pixel) {
    const value = pixel & 0xffff;
    const red5 = (value >> 11) & 0x1f;
    const green6 = (value >> 5) & 0x3f;
    const blue5 = value & 0x1f;
    return [
      (red5 << 3) | (red5 >> 2),
      (green6 << 2) | (green6 >> 4),
      (blue5 << 3) | (blue5 >> 2),
    ];
  }

  /**
   * @param {number} red
   * @param {number} green
   * @param {number} blue
   * @returns {number}
   */
  function rgbToRgb565(red, green, blue) {
    const red5 = Math.round(((red & 0xff) * 31) / 255);
    const green6 = Math.round(((green & 0xff) * 63) / 255);
    const blue5 = Math.round(((blue & 0xff) * 31) / 255);
    return (red5 << 11) | (green6 << 5) | blue5;
  }

  /**
   * @param {Uint8ClampedArray} data
   * @param {number} offset
   * @returns {boolean}
   */
  function rgbaPixelHasBit(data, offset) {
    return (
      data[offset + 3] !== 0 &&
      (data[offset] !== 0 || data[offset + 1] !== 0 || data[offset + 2] !== 0)
    );
  }

  /**
   * @param {number} functionId
   * @param {number} source
   * @param {number} destination
   * @returns {number}
   */
  function rasterFunctionByte(functionId, source, destination) {
    const src = source & 0xff;
    const dst = destination & 0xff;

    switch (functionId) {
      case GX_CLEAR:
        return 0;
      case GX_AND:
        return src & dst;
      case GX_AND_REVERSE:
        return src & ~dst & 0xff;
      case GX_COPY:
        return src;
      case GX_AND_INVERTED:
        return ~src & dst & 0xff;
      case GX_NOOP:
        return dst;
      case GX_XOR:
        return src ^ dst;
      case GX_OR:
        return src | dst;
      case GX_NOR:
        return ~(src | dst) & 0xff;
      case GX_EQUIV:
        return ~(src ^ dst) & 0xff;
      case GX_INVERT:
        return ~dst & 0xff;
      case GX_OR_REVERSE:
        return (src | ~dst) & 0xff;
      case GX_COPY_INVERTED:
        return ~src & 0xff;
      case GX_OR_INVERTED:
        return (~src | dst) & 0xff;
      case GX_NAND:
        return ~(src & dst) & 0xff;
      case GX_SET:
        return 0xff;
      default:
        return src;
    }
  }

  /**
   * @param {number} functionId
   * @param {number} sourceAlpha
   * @param {number} destinationAlpha
   * @returns {number}
   */
  function rasterFunctionAlpha(functionId, sourceAlpha, destinationAlpha) {
    if (functionId === GX_CLEAR) {
      return 0;
    }
    if (functionId === GX_NOOP) {
      return destinationAlpha & 0xff;
    }
    if (functionId === GX_COPY) {
      return sourceAlpha & 0xff;
    }
    return 0xff;
  }

  /**
   * @param {Uint8ClampedArray} data
   * @param {number} offset
   * @param {number} functionId
   * @param {number} red
   * @param {number} green
   * @param {number} blue
   * @param {number} alpha
   * @param {number=} planeMask
   */
  function applyRasterFunctionPixel(
    data,
    offset,
    functionId,
    red,
    green,
    blue,
    alpha,
    planeMask = 0xffffffff,
  ) {
    const effectivePlaneMask = unsignedInt(planeMask, 0xffffffff) & 0xffffff;
    if (effectivePlaneMask === 0) {
      return;
    }
    const resultPixel =
      (rasterFunctionByte(functionId, red, data[offset]) << 16) |
      (rasterFunctionByte(functionId, green, data[offset + 1]) << 8) |
      rasterFunctionByte(functionId, blue, data[offset + 2]);
    const destPixel =
      (data[offset] << 16) | (data[offset + 1] << 8) | data[offset + 2];
    const maskedPixel =
      (resultPixel & effectivePlaneMask) |
      (destPixel & ~effectivePlaneMask & 0xffffff);
    const resultAlpha = rasterFunctionAlpha(functionId, alpha, data[offset + 3]);

    data[offset] = (maskedPixel >> 16) & 0xff;
    data[offset + 1] = (maskedPixel >> 8) & 0xff;
    data[offset + 2] = maskedPixel & 0xff;
    data[offset + 3] =
      effectivePlaneMask === 0xffffff
        ? resultAlpha
        : Math.max(data[offset + 3], resultAlpha);
  }

  /**
   * @param {string} family
   * @returns {string}
   */
  function fontFamilyToCss(family) {
    const value =
      typeof family === "string" && family.trim() ? family.trim() : "sans-serif";

    if (
      /^(serif|sans-serif|monospace|cursive|fantasy|system-ui|ui-serif|ui-sans-serif|ui-monospace|ui-rounded|math|fangsong)$/i.test(value)
    ) {
      return value;
    }
    return `"${value.replace(/["\\]/g, "\\$&")}", sans-serif`;
  }

  /**
   * @param {number} shape
   * @returns {string}
   */
  function cursorShapeToCss(shape) {
    switch (shape) {
      case 0:
      case 2:
      case 4:
      case 6:
      case 8:
      case 22:
      case 48:
      case 64:
      case 68:
      case 80:
      case 84:
      case 94:
      case 102:
      case 104:
      case 118:
      case 132:
      case 142:
      case 146:
        return "default";
      case 10:
      case 24:
      case 88:
        return "not-allowed";
      case 26:
      case 28:
      case 150:
        return "wait";
      case 20:
        return "progress";
      case 30:
      case 32:
      case 34:
      case 36:
      case 38:
      case 40:
      case 44:
      case 46:
      case 66:
      case 86:
      case 124:
      case 128:
      case 130:
        return "crosshair";
      case 42:
      case 52:
      case 120:
        return "move";
      case 50:
        return "copy";
      case 54:
      case 56:
      case 122:
        return "grab";
      case 58:
      case 60:
      case 62:
      case 74:
      case 82:
      case 100:
      case 126:
        return "pointer";
      case 70:
      case 72:
      case 96:
      case 98:
      case 108:
      case 110:
      case 112:
        return "ew-resize";
      case 16:
      case 18:
      case 138:
      case 140:
      case 106:
      case 114:
      case 116:
        return "ns-resize";
      case 12:
      case 76:
      case 136:
      case 148:
        return "nesw-resize";
      case 14:
      case 78:
      case 134:
      case 144:
        return "nwse-resize";
      case 90:
        return "cell";
      case 92:
        return "help";
      case 152:
        return "text";
      default:
        return "default";
    }
  }

  /**
   * @param {number} functionId
   * @returns {GlobalCompositeOperation}
   */
  function compositeOperationForFunction(functionId) {
    if (functionId === GX_CLEAR) {
      return "destination-out";
    }
    if (functionId === GX_XOR) {
      return "xor";
    }
    return "source-over";
  }

  /**
   * @param {number} functionId
   * @returns {boolean}
   */
  function drawFunctionChangesDestination(functionId) {
    return functionId !== GX_NOOP;
  }

  /**
   * @param {number} capStyle
   * @returns {CanvasLineCap}
   */
  function lineCapForStyle(capStyle) {
    if (capStyle === CAP_ROUND) {
      return "round";
    }
    if (capStyle === CAP_PROJECTING) {
      return "square";
    }
    return "butt";
  }

  /**
   * @param {number} joinStyle
   * @returns {CanvasLineJoin}
   */
  function lineJoinForStyle(joinStyle) {
    if (joinStyle === JOIN_ROUND) {
      return "round";
    }
    if (joinStyle === JOIN_BEVEL) {
      return "bevel";
    }
    return "miter";
  }

  /**
   * @param {number} codepoint
   * @returns {number}
   */
  function codepointToKeysym(codepoint) {
    if (codepoint <= 0xff) {
      return codepoint;
    }
    if (codepoint <= 0x10ffff) {
      return (UNICODE_KEYSYM_BASE | codepoint) >>> 0;
    }
    return 0;
  }

  /**
   * @param {KeyboardEvent} event
   * @returns {boolean}
   */
  function isRightModifierEvent(event) {
    return (
      event.location === DOM_KEY_LOCATION_RIGHT ||
      (typeof event.code === "string" && event.code.endsWith("Right"))
    );
  }

  /**
   * @param {KeyboardEvent | MouseEvent | PointerEvent} event
   * @returns {number}
   */
  function modifierState(event) {
    let state = 0;
    if (event.shiftKey) {
      state |= SHIFT_MASK;
    }
    if (event.ctrlKey) {
      state |= CONTROL_MASK;
    }
    if (event.altKey) {
      state |= MOD1_MASK;
    }
    if (event.metaKey) {
      state |= MOD4_MASK;
    }
    return state;
  }

  /**
   * @param {PointerEvent} event
   * @returns {number}
   */
  function pointerButtonStateFromButtons(event, buttons) {
    let state = modifierState(event);
    if (buttons & 1) {
      state |= BUTTON1_MASK;
    }
    if (buttons & 4) {
      state |= BUTTON2_MASK;
    }
    if (buttons & 2) {
      state |= BUTTON3_MASK;
    }
    if (buttons & 8) {
      state |= BUTTON4_MASK;
    }
    if (buttons & 16) {
      state |= BUTTON5_MASK;
    }
    return state;
  }

  /**
   * @param {PointerEvent} event
   * @returns {number}
   */
  function pointerButtonState(event) {
    return pointerButtonStateFromButtons(event, event.buttons);
  }

  /**
   * @param {number} button
   * @returns {number}
   */
  function domButtonMask(button) {
    if (button === 1) {
      return 4;
    }
    if (button === 2) {
      return 2;
    }
    if (button === 3) {
      return 8;
    }
    if (button === 4) {
      return 16;
    }
    return 1;
  }

  /**
   * @param {number} type
   * @param {PointerEvent} event
   * @returns {number}
   */
  function pointerEventButtonState(type, event) {
    let buttons = event.buttons;
    const buttonMask = domButtonMask(event.button);

    if (type === BUTTON_PRESS) {
      buttons &= ~buttonMask;
    } else if (type === BUTTON_RELEASE) {
      buttons |= buttonMask;
    }
    return pointerButtonStateFromButtons(event, buttons);
  }

  /**
   * @param {KeyboardEvent} event
   * @returns {number}
   */
  function keySymForKeyboardEvent(event) {
    if (isRightModifierEvent(event)) {
      const rightMapped = RIGHT_MODIFIER_KEY_SYMS[event.key];
      if (typeof rightMapped === "number") {
        return rightMapped;
      }
    }
    const mapped = KEY_SYMS[event.key];
    if (typeof mapped === "number") {
      return mapped;
    }
    const text = singleCodepointText(event.key);
    if (text) {
      const codepoint = text.codePointAt(0);
      return typeof codepoint === "number" ? codepointToKeysym(codepoint) : 0;
    }
    return 0;
  }

  /**
   * @param {string} text
   * @returns {string | null}
   */
  function singleCodepointText(text) {
    const chars = Array.from(text || "");
    return chars.length === 1 ? chars[0] : null;
  }

  /**
   * @param {KeyboardEvent} event
   * @param {number} keySym
   * @returns {number}
   */
  function keyCodeForKeyboardEvent(event, keySym) {
    if (isRightModifierEvent(event)) {
      const rightMapped = RIGHT_MODIFIER_KEY_CODES[event.key];
      if (typeof rightMapped === "number") {
        return rightMapped;
      }
    }
    const mappedKeyCode = KEY_CODES[event.key];
    if (typeof mappedKeyCode === "number") {
      return mappedKeyCode;
    }
    const legacyCode = event.keyCode || event.which;
    if (legacyCode > 0) {
      return legacyCode;
    }
    const fallback = keySym & 0xff;
    return fallback > 0 ? fallback : 0;
  }

  /**
   * @param {PointerEvent} event
   * @returns {number}
   */
  function xButtonFromPointerEvent(event) {
    if (event.button === 1) {
      return 2;
    }
    if (event.button === 2) {
      return 3;
    }
    if (event.button === 3) {
      return 4;
    }
    if (event.button === 4) {
      return 5;
    }
    return 1;
  }

  /**
   * @param {number} value
   * @returns {number}
   */
  function eventTime(value) {
    return Math.max(0, Math.floor(value));
  }

  /**
   * @param {Document} document
   */
  function ensureStyle(document) {
    if (document.getElementById(STYLE_ID)) {
      return;
    }
    const style = document.createElement("style");
    style.id = STYLE_ID;
    style.textContent = `
.x11-wasm-root {
  position: relative;
  overflow: hidden;
  isolation: isolate;
  touch-action: none;
  user-select: none;
  -webkit-user-select: none;
  -webkit-touch-callout: none;
}
.x11-wasm-window {
  position: absolute;
  overflow: hidden;
  box-sizing: content-box;
  transform-origin: 0 0;
}
.x11-wasm-frame {
  position: absolute;
  box-sizing: content-box;
  isolation: isolate;
  border-radius: 3px 3px 0 0;
  box-shadow: 0 0 0 1px #777, 0 3px 8px #0003;
}
.x11-wasm-titlebar {
  position: absolute;
  inset: 0 0 auto;
  height: 28px;
  display: flex;
  align-items: center;
  background: #e4e4e4;
  color: #303030;
  cursor: move;
  font: 13px sans-serif;
  letter-spacing: 0;
  touch-action: none;
  container-type: inline-size;
}
.x11-wasm-frame[data-x11-wasm-active="1"] > .x11-wasm-titlebar {
  background: #21654e;
  color: #fff;
}
.x11-wasm-window-title {
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  padding: 0 8px;
}
.x11-wasm-window-icon {
  flex: 0 0 16px;
  width: 16px;
  height: 16px;
  margin-left: 6px;
  object-fit: contain;
}
.x11-wasm-window-icon[hidden] { display: none; }
@container (max-width: 100px) {
  .x11-wasm-window-icon { display: none; }
  .x11-wasm-window-title { padding: 0; }
}
.x11-wasm-close {
  flex: 0 1 28px;
  min-width: 0;
  position: relative;
  width: 28px;
  height: 28px;
  padding: 0;
  border: 0;
  border-radius: 0;
  color: inherit;
  background: transparent;
  cursor: pointer;
}
.x11-wasm-close svg {
  display: block;
  width: 16px;
  max-width: 100%;
  height: 16px;
  margin: auto;
  pointer-events: none;
}
.x11-wasm-close:hover:enabled { background: #ac2430; color: white; }
.x11-wasm-close:disabled { opacity: .4; cursor: default; }
.x11-wasm-close:focus-visible { outline: 2px solid currentColor; outline-offset: -3px; }
.x11-wasm-resize { position: absolute; z-index: 2; touch-action: none; }
.x11-wasm-resize[hidden] { display: none; }
.x11-wasm-resize-n, .x11-wasm-resize-s { left: 6px; right: 6px; height: 6px; cursor: ns-resize; }
.x11-wasm-resize-n { top: -3px; }
.x11-wasm-resize-s { bottom: -3px; }
.x11-wasm-resize-e, .x11-wasm-resize-w { top: 6px; bottom: 6px; width: 6px; cursor: ew-resize; }
.x11-wasm-resize-e { right: -3px; }
.x11-wasm-resize-w { left: -3px; }
.x11-wasm-resize-ne, .x11-wasm-resize-nw, .x11-wasm-resize-se, .x11-wasm-resize-sw { width: 9px; height: 9px; }
.x11-wasm-resize-ne { right: -3px; top: -3px; cursor: nesw-resize; }
.x11-wasm-resize-sw { left: -3px; bottom: -3px; cursor: nesw-resize; }
.x11-wasm-resize-nw { left: -3px; top: -3px; cursor: nwse-resize; }
.x11-wasm-resize-se { right: -3px; bottom: -3px; cursor: nwse-resize; }
.x11-wasm-root.x11-wasm-pointer-grab,
.x11-wasm-root.x11-wasm-pointer-grab * {
  cursor: var(--x11-wasm-pointer-grab-cursor, auto) !important;
}
.x11-wasm-window > canvas,
.x11-wasm-root > canvas {
  position: absolute;
  inset: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
}
.x11-wasm-input {
  position: absolute;
  left: 0;
  top: 0;
  width: 1px;
  height: 1px;
  min-width: 1px;
  max-width: 1px;
  min-height: 1px;
  max-height: 1px;
  box-sizing: border-box;
  border: 0;
  padding: 0;
  margin: 0;
  overflow: hidden;
  opacity: 0;
  outline: none;
  font: 1px monospace;
  line-height: 1px;
  white-space: pre;
  color: transparent;
  background: transparent;
  caret-color: transparent;
  resize: none;
  contain: strict;
  user-select: none;
  -webkit-user-select: none;
  -webkit-touch-callout: none;
  pointer-events: none;
}
`;
    document.head.appendChild(style);
  }

  Object.assign(privateApi, {
    positiveInt,
    unsignedInt,
    wmStateName,
    pixelToCss,
    pixelToRgb,
    rgb565ToRgb,
    rgbToRgb565,
    rgbaPixelHasBit,
    rasterFunctionByte,
    rasterFunctionAlpha,
    applyRasterFunctionPixel,
    fontFamilyToCss,
    cursorShapeToCss,
    compositeOperationForFunction,
    drawFunctionChangesDestination,
    lineCapForStyle,
    lineJoinForStyle,
    codepointToKeysym,
    isRightModifierEvent,
    modifierState,
    pointerButtonStateFromButtons,
    pointerButtonState,
    domButtonMask,
    pointerEventButtonState,
    keySymForKeyboardEvent,
    singleCodepointText,
    keyCodeForKeyboardEvent,
    xButtonFromPointerEvent,
    eventTime,
    ensureStyle,
  });
})(typeof globalThis !== "undefined" ? globalThis : window);
