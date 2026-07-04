// @ts-check
/*
 * backend.js --
 *
 *	Core constructor for the x11.wasm DOM/canvas backend. Prototype
 *	methods live in the adjacent responsibility-specific shards.
 */

(function attachX11WasmDOMBackendCore(global) {
  "use strict";

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
  } = privateApi;



  class X11WasmDOMBackend {
/**
     * @param {X11WasmDOMBackendOptions=} options
     */
    constructor(options = {}) {
      /** @private @type {Document | null} */
      this.document = options.document || global.document || null;
      /** @private @type {any} */
      this.module =
        options.module || (typeof Module !== "undefined" ? Module : null);
      /** @private @type {HTMLElement | null} */
      this.rootElement = null;
      /** @private @type {HTMLTextAreaElement | null} */
      this.inputElement = null;
      /** @private */
      this.focusedWindow = 0;
      this.inputContextFocused = false;
      this.inputContextWindow = 0;
      this.inputContextHasSpot = false;
      this.inputContextSpotX = 0;
      this.inputContextSpotY = 0;
      /** @private @type {HTMLElement | null} */
      this.focusedElement = null;
      /** @private @type {Map<number, X11WasmDOMResource>} */
      this.resources = new Map();
      /** @private @type {Map<number, string>} */
      this.cursors = new Map();
      /** @private @type {string | null} */
      this.pointerGrabCursor = null;
      /** @private @type {Set<X11WasmDOMResource>} */
      this.dirtyResources = new Set();
      /** @private @type {string | HTMLElement | undefined} */
      this.rootOption = options.root;
      /** @private */
      this.rootWindow = 0;
      /** @private @type {ResizeObserver | null} */
      this.rootResizeObserver = null;
      /** @private @type {(() => void) | null} */
      this.rootResizeHandler = null;
      /** @private */
      this.lastPointerRootX = 0;
      /** @private */
      this.lastPointerRootY = 0;
      this.pointerWindow = 0;
      this.pointerState = 0;
      this.pointerCaptureWindow = 0;
      this.pointerWarpCount = 0;
      this.lastWarpPointer = null;
      /** @private */
      this.wheelRemainderX = 0;
      /** @private */
      this.wheelRemainderY = 0;
      /** @private */
      this.composing = false;
      /** @private */
      this.compositionWindow = 0;
      /** @private */
      this.compositionText = "";
      /** @private */
      this.listenersInstalled = false;
      /** @private @type {number | null} */
      this.capturedPointerId = null;
      /** @private @type {Element | null} */
      this.capturedPointerElement = null;
      /** @private */
      this.hostPasteShortcutActive = false;
      /** @private */
      this.suppressInputFocusEvents = 0;
      /** @private */
      this.flushScheduled = false;
      /** @private */
      this.damageSerial = 1;
      this.requestSerial = 0;
      this.flushSerial = 0;
      this.syncSerial = 0;
      this.synchronizeEnabled = false;
      this.bellCount = 0;
      this.lastBellPercent = 0;
      this.lastBellDuration = 0;
      this.lastBellFrequency = 0;
      this.lastBellVolume = 0;
      this.bellAudioStatus = "idle";
      this.bellAudioError = "";
      this.bellVibrationStatus = "idle";
      this.bellVibrationError = "";
      /** @private @type {AudioContext | null} */
      this.bellAudioContext = null;
      this.screenSaverResetCount = 0;
      this.lastScreenSaverMode = 0;
      this.lastUserActivityTime = 0;
      this.screenSaverTimeout = 600;
      this.screenSaverInterval = 600;
      this.screenSaverPreferBlanking = 2;
      this.screenSaverAllowExposures = 2;
      this.screenSaverSuspended = false;
      this.screenWakeLockStatus = "idle";
      this.screenWakeLockError = "";
      this.screenWakeLockType = "";
      /** @private @type {any} */
      this.screenWakeLock = null;
      /** @private */
      this.screenWakeLockRequestId = 0;
      this.fullscreenRequestStatus = "idle";
      this.fullscreenRequestError = "";
      this.fullscreenRequestWindow = 0;
      this.fullscreenRequestCount = 0;
      this.fullscreenExitCount = 0;
      this.fullscreenChangeCount = 0;
      this.fullscreenErrorCount = 0;
      /** @private @type {Element | null} */
      this.fullscreenElement = null;
      /** @private @type {((event: Event) => void) | null} */
      this.fullscreenChangeHandler = null;
      /** @private @type {((event: Event) => void) | null} */
      this.fullscreenErrorHandler = null;
      this.hostAccessEnabled = true;
      this.hostAccessList = "";
      /** @type {X11WasmDOMHostAccessEntry[]} */
      this.hostAccessHosts = [];
      this.serverGrabDepth = 0;
      this.pointerGrabWindow = 0;
      this.pointerGrabOwnerEvents = false;
      this.pointerGrabEventMask = 0;
      this.pointerGrabConfineWindow = 0;
      this.pointerGrabCursorShape = 0;
      this.pointerGrabPointerMode = 1;
      this.pointerGrabKeyboardMode = 1;
      this.keyboardGrabWindow = 0;
      this.keyboardGrabOwnerEvents = false;
      this.keyboardGrabPointerMode = 1;
      this.keyboardGrabKeyboardMode = 1;
      this.clipboardText = "";
      this.clipboardWriteStatus = "idle";
      this.clipboardWriteError = "";
      this.clipboardReadStatus = "idle";
      this.clipboardReadError = "";
      this.clipboardReadCount = 0;
      /** @private */
      this.clipboardReadRequestId = 0;
      this.externalClipboardText = "";
      this.externalClipboardAvailable = false;
      this.selectionOwners = new Map();
      /** @private @type {X11WasmDOMClipRect[] | null} */
      this.currentClip = null;
      /** @private @type {X11WasmDOMClipMask | null} */
      this.currentClipMask = null;
      /** @private @type {X11WasmDOMDrawStyle} */
      this.currentDrawStyle = {
        functionId: 3,
        lineWidth: 1,
        lineStyle: LINE_SOLID,
        capStyle: 1,
        joinStyle: 0,
        dashOffset: 0,
        dashLength: 4,
        dashPattern: [],
        fillStyle: FILL_SOLID,
        stipple: 0,
        tile: 0,
        tsOriginX: 0,
        tsOriginY: 0,
        backgroundPixel: 0xffffff,
        fontPixelSize: 14,
        fontFamily: "sans-serif",
        planeMask: 0xffffffff,
      };
    }
  }

  privateApi.X11WasmDOMBackend = X11WasmDOMBackend;
})(typeof globalThis !== "undefined" ? globalThis : window);
