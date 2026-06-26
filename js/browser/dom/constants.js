// @ts-check
/*
 * constants.js --
 *
 *	Constants used by the x11.wasm DOM backend.
 */

(function attachX11WasmDOMConstants(global) {
  "use strict";

  const privateApi =
    global["X11WasmDOMPrivate"] || (global["X11WasmDOMPrivate"] = {});

  const RESOURCE_ATTR = "data-x11-wasm-xid";
  const STYLE_ID = "x11-wasm-dom-style";
  const KEY_PRESS = 2;
  const KEY_RELEASE = 3;
  const BUTTON_PRESS = 4;
  const BUTTON_RELEASE = 5;
  const MOTION_NOTIFY = 6;
  const ENTER_NOTIFY = 7;
  const LEAVE_NOTIFY = 8;
  const FOCUS_IN = 9;
  const FOCUS_OUT = 10;
  const WM_INPUT_HINT = 1 << 0;
  const WM_STATE_HINT = 1 << 1;
  const WM_ICON_PIXMAP_HINT = 1 << 2;
  const WM_ICON_WINDOW_HINT = 1 << 3;
  const WM_ICON_POSITION_HINT = 1 << 4;
  const WM_ICON_MASK_HINT = 1 << 5;
  const WM_WINDOW_GROUP_HINT = 1 << 6;
  const WM_WITHDRAWN_STATE = 0;
  const WM_NORMAL_STATE = 1;
  const WM_ICONIC_STATE = 3;
  const P_POSITION = 1 << 2;
  const P_SIZE = 1 << 3;
  const P_MIN_SIZE = 1 << 4;
  const P_MAX_SIZE = 1 << 5;
  const P_RESIZE_INC = 1 << 6;
  const P_ASPECT = 1 << 7;
  const P_BASE_SIZE = 1 << 8;
  const P_WIN_GRAVITY = 1 << 9;
  const NORMAL_SIZE_HINT_ATTRIBUTES = {
    flags: "data-x11-wasm-size-hints-flags",
    x: "data-x11-wasm-hint-x",
    y: "data-x11-wasm-hint-y",
    width: "data-x11-wasm-hint-width",
    height: "data-x11-wasm-hint-height",
    minWidth: "data-x11-wasm-min-width",
    minHeight: "data-x11-wasm-min-height",
    maxWidth: "data-x11-wasm-max-width",
    maxHeight: "data-x11-wasm-max-height",
    widthInc: "data-x11-wasm-width-inc",
    heightInc: "data-x11-wasm-height-inc",
    minAspectX: "data-x11-wasm-min-aspect-x",
    minAspectY: "data-x11-wasm-min-aspect-y",
    maxAspectX: "data-x11-wasm-max-aspect-x",
    maxAspectY: "data-x11-wasm-max-aspect-y",
    baseWidth: "data-x11-wasm-base-width",
    baseHeight: "data-x11-wasm-base-height",
    winGravity: "data-x11-wasm-win-gravity",
  };
  const ZOOM_SIZE_HINT_ATTRIBUTES = {
    flags: "data-x11-wasm-zoom-hints-flags",
    x: "data-x11-wasm-zoom-hint-x",
    y: "data-x11-wasm-zoom-hint-y",
    width: "data-x11-wasm-zoom-hint-width",
    height: "data-x11-wasm-zoom-hint-height",
    minWidth: "data-x11-wasm-zoom-min-width",
    minHeight: "data-x11-wasm-zoom-min-height",
    maxWidth: "data-x11-wasm-zoom-max-width",
    maxHeight: "data-x11-wasm-zoom-max-height",
    widthInc: "data-x11-wasm-zoom-width-inc",
    heightInc: "data-x11-wasm-zoom-height-inc",
    minAspectX: "data-x11-wasm-zoom-min-aspect-x",
    minAspectY: "data-x11-wasm-zoom-min-aspect-y",
    maxAspectX: "data-x11-wasm-zoom-max-aspect-x",
    maxAspectY: "data-x11-wasm-zoom-max-aspect-y",
    baseWidth: "data-x11-wasm-zoom-base-width",
    baseHeight: "data-x11-wasm-zoom-base-height",
    winGravity: "data-x11-wasm-zoom-win-gravity",
  };
  const CW_X = 1 << 0;
  const CW_Y = 1 << 1;
  const CW_WIDTH = 1 << 2;
  const CW_HEIGHT = 1 << 3;
  const CW_BORDER_WIDTH = 1 << 4;
  const FORGET_GRAVITY = 0;
  const NORTH_WEST_GRAVITY = 1;
  const NORTH_GRAVITY = 2;
  const NORTH_EAST_GRAVITY = 3;
  const WEST_GRAVITY = 4;
  const CENTER_GRAVITY = 5;
  const EAST_GRAVITY = 6;
  const SOUTH_WEST_GRAVITY = 7;
  const SOUTH_GRAVITY = 8;
  const SOUTH_EAST_GRAVITY = 9;
  const SHIFT_MASK = 1 << 0;
  const CONTROL_MASK = 1 << 2;
  const MOD1_MASK = 1 << 3;
  const MOD4_MASK = 1 << 6;
  const BUTTON1_MASK = 1 << 8;
  const BUTTON2_MASK = 1 << 9;
  const BUTTON3_MASK = 1 << 10;
  const BUTTON4_MASK = 1 << 11;
  const BUTTON5_MASK = 1 << 12;
  const UNICODE_KEYSYM_BASE = 0x01000000;
  const COMPOSITION_START = 1;
  const COMPOSITION_UPDATE = 2;
  const COMPOSITION_END = 3;
  const GX_CLEAR = 0x0;
  const GX_AND = 0x1;
  const GX_AND_REVERSE = 0x2;
  const GX_COPY = 0x3;
  const GX_AND_INVERTED = 0x4;
  const GX_NOOP = 0x5;
  const GX_XOR = 0x6;
  const GX_OR = 0x7;
  const GX_NOR = 0x8;
  const GX_EQUIV = 0x9;
  const GX_INVERT = 0xa;
  const GX_OR_REVERSE = 0xb;
  const GX_COPY_INVERTED = 0xc;
  const GX_OR_INVERTED = 0xd;
  const GX_NAND = 0xe;
  const GX_SET = 0xf;
  const LINE_SOLID = 0;
  const LINE_DOUBLE_DASH = 2;
  const FILL_SOLID = 0;
  const FILL_TILED = 1;
  const FILL_STIPPLED = 2;
  const FILL_OPAQUE_STIPPLED = 3;
  const CAP_ROUND = 2;
  const CAP_PROJECTING = 3;
  const JOIN_ROUND = 1;
  const JOIN_BEVEL = 2;
  const WHEEL_PIXEL_THRESHOLD = 100;
  const WHEEL_LINE_SCALE = 40;
  const WHEEL_PAGE_SCALE = 800;
  const DOM_KEY_LOCATION_RIGHT = 2;
  /** @type {Record<string, number>} */
  const KEY_SYMS = {
    Alt: 0xffe9,
    Backspace: 0xff08,
    CapsLock: 0xffe5,
    Control: 0xffe3,
    Tab: 0xff09,
    Enter: 0xff0d,
    Escape: 0xff1b,
    Home: 0xff50,
    ArrowLeft: 0xff51,
    ArrowUp: 0xff52,
    ArrowRight: 0xff53,
    ArrowDown: 0xff54,
    PageUp: 0xff55,
    PageDown: 0xff56,
    End: 0xff57,
    Insert: 0xff63,
    Delete: 0xffff,
    Meta: 0xffe7,
    NumLock: 0xff7f,
    ScrollLock: 0xff14,
    Shift: 0xffe1,
  };

  /** @type {Record<string, number>} */
  const KEY_CODES = {
    Alt: 18,
    CapsLock: 20,
    Control: 17,
    Meta: 91,
    NumLock: 144,
    ScrollLock: 145,
    Shift: 16,
  };

  /** @type {Record<string, number>} */
  const RIGHT_MODIFIER_KEY_SYMS = {
    Alt: 0xffea,
    Control: 0xffe4,
    Meta: 0xffe8,
    Shift: 0xffe2,
  };

  /** @type {Record<string, number>} */
  const RIGHT_MODIFIER_KEY_CODES = {
    Alt: 165,
    Control: 163,
    Meta: 92,
    Shift: 161,
  };

  /**
   * @param {number} value
   * @param {number} fallback
   * @returns {number}
   */

  Object.assign(privateApi, {
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
  });
})(typeof globalThis !== "undefined" ? globalThis : window);
