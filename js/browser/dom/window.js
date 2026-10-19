// @ts-check
/*
 * window.js --
 *
 *	Root, resource, cursor, pointer, and window lifecycle methods.
 */

(function attachX11WasmDOMBackendWindow(global) {
  "use strict";

  const privateApi =
    global["X11WasmDOMPrivate"] || (global["X11WasmDOMPrivate"] = {});
  const {
    X11WasmDOMBackend,
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

  if (!X11WasmDOMBackend) {
    throw new Error("X11WasmDOMBackend core must be loaded first");
  }

  Object.assign(X11WasmDOMBackend.prototype, /** @type {any} */ ({
    /**
     * @param {number} width
     * @param {number} height
     * @param {number} rootXid
     * @returns {number}
     */
    init(width, height, rootXid) {
      const document = this.document;
      if (!document) {
        return 0;
      }
      ensureStyle(document);
      const root = this.resolveRootElement();
      const fallbackWidth = positiveInt(width, 1024);
      const fallbackHeight = positiveInt(height, 768);
      root.classList.add("x11-wasm-root");
      root.setAttribute(RESOURCE_ATTR, String(rootXid));
      root.tabIndex = root.tabIndex < 0 ? 0 : root.tabIndex;
      const rootSize = this.measureRootElement(
        root,
        fallbackWidth,
        fallbackHeight,
      );
      const rootWidth = rootSize.width;
      const rootHeight = rootSize.height;

      /** @type {HTMLCanvasElement | null} */
      let canvas = root.querySelector(":scope > canvas");
      if (!canvas) {
        canvas = document.createElement("canvas");
        root.insertBefore(canvas, root.firstChild);
      }
      if (!canvas) {
        return 0;
      }
      canvas.width = rootWidth;
      canvas.height = rootHeight;

      this.rootElement = root;
      this.resources.set(rootXid, {
        id: rootXid,
        kind: "root",
        parent: 0,
        x: 0,
        y: 0,
        width: rootWidth,
        height: rootHeight,
        borderWidth: 0,
        pendingX: 0,
        pendingY: 0,
        pendingWidth: rootWidth,
        pendingHeight: rootHeight,
        pendingBorderWidth: 0,
        depth: 24,
        windowClass: 1,
        eventMask: 0,
        doNotPropagateMask: 0,
        colormap: 0,
        overrideRedirect: false,
        saveUnder: false,
        bitGravity: 1,
        winGravity: 1,
        backingStore: 0,
        backingPlanes: 0xffffffff,
        backingPixel: 0,
        mapped: true,
        pendingMapped: true,
        geometryDirty: false,
        dirty: false,
        damage: null,
        damageSerial: 0,
        element: root,
        canvas,
        context: canvas.getContext("2d"),
        backgroundColor: null,
        backgroundPixmap: 0,
        borderColor: null,
        borderPixmap: 0,
        cursor: null,
        title: "",
        iconName: "",
        transientFor: 0,
        command: "",
        clientMachine: "",
        wmClassName: "",
        wmClass: "",
        alpha: 1,
        topmost: false,
        zoomed: false,
        fullscreen: false,
        wmState: "normal",
        wmHints: null,
        iconPhotoCount: 0,
        iconPhotoSizes: "",
        iconPhotoHref: "",
        iconPhotoWidth: 0,
        iconPhotoHeight: 0,
        iconBitmapHref: "",
        iconBitmapWidth: 0,
        iconBitmapHeight: 0,
        iconSizeCount: 0,
        iconSizes: "",
        protocols: [],
        windowTypes: [],
        closeRequestCount: 0,
        lastCloseRequestTime: 0,
        closeRequestStatus: "idle",
        closeEventListenerInstalled: false,
	        colormapWindows: [],
	        standardColormaps: Object.create(null),
	        sizeHints: null,
	        zoomHints: null,
	      });
      this.rootWindow = rootXid;
      this.pointerWindow = rootXid;
      this.ensureInputElement();
      this.installEventListeners(root);
      this.installRootResizeObserver(root);
      this.applyRootRequestState();
      this.applyRootSynchronizeState();
      this.applyRootPointerState();
      this.applyRootGrabState();
      this.applyRootScreenSaverState();
      this.applyRootHostAccessState();
      this.applyRootServerGrabState();
      this.applyRootSelectionOwnerState();
      this.installWindowCloseListener(this.resources.get(rootXid));
      this.installFullscreenEventListeners();
      this.syncWindowManager?.();
      return 0;
    },

    /**
     * @returns {number}
     */
    getRootWidth() {
      const resource = this.resources.get(this.rootWindow);
      return resource ? resource.width : 0;
    },

    /**
     * @returns {number}
     */
    getRootHeight() {
      const resource = this.resources.get(this.rootWindow);
      return resource ? resource.height : 0;
    },

    /**
     * @param {HTMLElement} root
     * @param {number} fallbackWidth
     * @param {number} fallbackHeight
     * @returns {{ width: number, height: number }}
     */
    measureRootElement(root, fallbackWidth, fallbackHeight) {
      let rect = root.getBoundingClientRect();
      let width = Math.floor(root.clientWidth || rect.width);
      let height = Math.floor(root.clientHeight || rect.height);

      if (width <= 0) {
        root.style.width = `${positiveInt(fallbackWidth, 1024)}px`;
      }
      if (height <= 0) {
        root.style.height = `${positiveInt(fallbackHeight, 768)}px`;
      }
      if (width <= 0 || height <= 0) {
        rect = root.getBoundingClientRect();
        width = Math.floor(root.clientWidth || rect.width);
        height = Math.floor(root.clientHeight || rect.height);
      }
      return {
        width: positiveInt(width, fallbackWidth),
        height: positiveInt(height, fallbackHeight),
      };
    },

    /**
     * @param {HTMLElement} root
     */
    installRootResizeObserver(root) {
      if (this.rootResizeObserver) {
        this.rootResizeObserver.disconnect();
        this.rootResizeObserver = null;
      }
      if (
        this.rootResizeHandler &&
        typeof global.removeEventListener === "function"
      ) {
        global.removeEventListener("resize", this.rootResizeHandler);
      }
      const sync = () => {
        this.syncRootElementSize(true);
      };
      if (typeof global.ResizeObserver === "function") {
        this.rootResizeObserver = new global.ResizeObserver(sync);
        this.rootResizeObserver.observe(root);
      }
      if (typeof global.addEventListener === "function") {
        this.rootResizeHandler = sync;
        global.addEventListener("resize", sync);
      } else {
        this.rootResizeHandler = null;
      }
    },

    /**
     * @param {boolean} notifyNative
     * @returns {number}
     */
    syncRootElementSize(notifyNative) {
      const root = this.rootElement;
      const resource = this.resources.get(this.rootWindow);
      if (!root || !resource) {
        return 0;
      }
      const size = this.measureRootElement(root, resource.width, resource.height);
      if (size.width === resource.width && size.height === resource.height) {
        return 0;
      }
      resource.width = size.width;
      resource.height = size.height;
      resource.pendingWidth = size.width;
      resource.pendingHeight = size.height;
      resource.geometryDirty = false;
      if (resource.canvas) {
        if (resource.canvas.width !== size.width) {
          resource.canvas.width = size.width;
        }
        if (resource.canvas.height !== size.height) {
          resource.canvas.height = size.height;
        }
      }
      this.markDirty(resource, 0, 0, size.width, size.height);
      if (notifyNative) {
        const resize = this.module && this.module["_X11WasmResizeRoot"];
        if (typeof resize === "function") {
          resize(size.width, size.height);
        }
      }
      this.refreshWindowStateGeometry();
      return 1;
    },

    /**
     * @returns {HTMLElement}
     */
    resolveRootElement() {
      const document = this.document;
      if (!document) {
        throw new Error("X11WasmDOMBackend requires a document");
      }
      if (this.rootOption instanceof global.HTMLElement) {
        return this.rootOption;
      }
      if (typeof this.rootOption === "string") {
        const found = document.querySelector(this.rootOption);
        if (found instanceof global.HTMLElement) {
          return found;
        }
      }
      let root = document.getElementById("x11-wasm-root");
      if (!(root instanceof global.HTMLElement)) {
        root = document.createElement("div");
        root.id = "x11-wasm-root";
        document.body.appendChild(root);
      }
      return root;
    },

    /**
     * @param {number} id
     * @param {number} parent
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @param {number} depth
     * @param {number} eventMask
     * @param {number=} windowClass
     * @returns {number}
     */
    createWindow(
      id,
      parent,
      x,
      y,
      width,
      height,
      depth,
      eventMask,
      windowClass = 1,
    ) {
      const document = this.document;
      if (!document || !this.rootElement) {
        return 0;
      }
      const parentResource =
        this.resources.get(parent) || this.resources.values().next().value;
      const parentElement =
        parentResource && parentResource.element
          ? parentResource.element
          : this.rootElement;
      const element = document.createElement("div");
      const isInputOnly = windowClass === 2;
      const canvas = isInputOnly ? null : document.createElement("canvas");
      const resource = this.createResourceRecord(
        id,
        "window",
        parent,
        x,
        y,
        width,
        height,
        depth,
        eventMask,
        element,
        canvas,
        windowClass,
      );

      element.className = "x11-wasm-window";
      if (isInputOnly) {
        element.classList.add("x11-wasm-input-only");
      }
      element.setAttribute(RESOURCE_ATTR, String(id));
      element.setAttribute("data-x11-wasm-window-class", String(windowClass));
      this.applyWindowEventMask(resource);
      element.style.display = "none";
      if (canvas) {
        element.appendChild(canvas);
      }
      parentElement.appendChild(element);
      this.applyGeometry(resource);
      this.resources.set(id, resource);
      this.applyWindowCloseRequestState(resource);
      this.installWindowCloseListener(resource);
      this.refreshWindowWMHintRelations();
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} drawable
     * @param {number} width
     * @param {number} height
     * @param {number} depth
     * @returns {number}
     */
    createPixmap(id, drawable, width, height, depth) {
      const canvas = this.createCanvas(
        positiveInt(width, 1),
        positiveInt(height, 1),
      );
      const resource = this.createResourceRecord(
        id,
        "pixmap",
        drawable,
        0,
        0,
        width,
        height,
        depth,
        0,
        null,
        canvas,
      );
      this.resources.set(id, resource);
      return 0;
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applyWindowEventMask(resource) {
      const element = resource.element;
      if (!element) {
        return;
      }
      if (resource.eventMask) {
        element.setAttribute("data-x11-wasm-event-mask", String(resource.eventMask));
      } else {
        element.removeAttribute("data-x11-wasm-event-mask");
      }
      if (resource.doNotPropagateMask) {
        element.setAttribute(
          "data-x11-wasm-do-not-propagate-mask",
          String(resource.doNotPropagateMask),
        );
      } else {
        element.removeAttribute("data-x11-wasm-do-not-propagate-mask");
      }
    },

    /**
     * @param {number} id
     * @returns {number}
     */
    destroyResource(id) {
      this.cancelWindowManagerResource?.(id);
      this.managerRestoreGeometries.delete(id);
      this.removeWindowManagerFrame?.(id);
      const resource = this.resources.get(id);
      const windowId = unsignedInt(id, 0);
      const iconWindow = this.ownerIconWindow(resource);
      this.cursors.delete(id);
      if (
        this.focusedWindow === windowId ||
        (resource && resource.element === this.focusedElement)
      ) {
        this.setFocusedWindow(0);
      }
      if (!resource || resource.kind === "root") {
        return 0;
      }
      this.dirtyResources.delete(resource);
      if (resource.element && resource.element.parentNode) {
        resource.element.parentNode.removeChild(resource.element);
      }
      this.resources.delete(id);
      this.refreshWindowWMHintRelations();
      this.syncIconWindowPresentation(iconWindow);
      if (resource.iconPhotoHref || resource.iconBitmapHref) {
        this.syncDocumentIconFromResources();
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {Uint8Array} rgba
     * @param {number} width
     * @param {number} height
     * @param {number} hotX
     * @param {number} hotY
     * @returns {number}
     */
    createCursor(id, rgba, width, height, hotX, hotY) {
      if (!this.document) {
        return 0;
      }
      const canvas = this.document.createElement("canvas");
      canvas.width = positiveInt(width, 1);
      canvas.height = positiveInt(height, 1);
      const context = canvas.getContext("2d");
      if (!context) {
        return 0;
      }
      const imageData = context.createImageData(canvas.width, canvas.height);
      imageData.data.set(rgba.subarray(0, imageData.data.length));
      context.putImageData(imageData, 0, 0);
      const url = canvas.toDataURL("image/png");
      if (!url) {
        return 0;
      }
      this.cursors.set(
        id,
        `url("${url}") ${Math.trunc(hotX)} ${Math.trunc(hotY)}, auto`,
      );
      return 0;
    },

    /**
     * @param {number} cursor
     * @returns {string}
     */
    cursorStyle(cursor) {
      return this.cursors.get(cursor) || cursorShapeToCss(cursor);
    },

applyPointerGrabCursor() {
      const root = this.rootElement;
      if (!root) {
        return;
      }
      if (this.pointerGrabCursor) {
        root.classList.add("x11-wasm-pointer-grab");
        root.style.setProperty(
          "--x11-wasm-pointer-grab-cursor",
          this.pointerGrabCursor,
        );
      } else {
        root.classList.remove("x11-wasm-pointer-grab");
        root.style.removeProperty("--x11-wasm-pointer-grab-cursor");
      }
      if (this.inputElement) {
        this.inputElement.style.cursor = this.pointerGrabCursor || "";
      }
    },

    /**
     * @param {boolean} enabled
     * @param {number} cursor
     * @returns {number}
     */
    setPointerGrabCursor(enabled, cursor) {
      this.pointerGrabCursor = enabled ? this.cursorStyle(cursor) : null;
      this.applyPointerGrabCursor();
      return 0;
    },

applyRootGrabState() {
      const root = this.rootElement;
      if (!root) {
        return;
      }
      root.setAttribute(
        "data-x11-wasm-pointer-grab-active",
        this.pointerGrabWindow ? "1" : "0",
      );
      root.setAttribute(
        "data-x11-wasm-keyboard-grab-active",
        this.keyboardGrabWindow ? "1" : "0",
      );

      if (this.pointerGrabWindow) {
        root.setAttribute(
          "data-x11-wasm-pointer-grab-window",
          String(this.pointerGrabWindow),
        );
        root.setAttribute(
          "data-x11-wasm-pointer-grab-owner-events",
          this.pointerGrabOwnerEvents ? "1" : "0",
        );
        root.setAttribute(
          "data-x11-wasm-pointer-grab-event-mask",
          String(this.pointerGrabEventMask),
        );
        root.setAttribute(
          "data-x11-wasm-pointer-grab-confine-window",
          String(this.pointerGrabConfineWindow),
        );
        root.setAttribute(
          "data-x11-wasm-pointer-grab-cursor",
          String(this.pointerGrabCursorShape),
        );
        root.setAttribute(
          "data-x11-wasm-pointer-grab-pointer-mode",
          String(this.pointerGrabPointerMode),
        );
        root.setAttribute(
          "data-x11-wasm-pointer-grab-keyboard-mode",
          String(this.pointerGrabKeyboardMode),
        );
      } else {
        root.removeAttribute("data-x11-wasm-pointer-grab-window");
        root.removeAttribute("data-x11-wasm-pointer-grab-owner-events");
        root.removeAttribute("data-x11-wasm-pointer-grab-event-mask");
        root.removeAttribute("data-x11-wasm-pointer-grab-confine-window");
        root.removeAttribute("data-x11-wasm-pointer-grab-cursor");
        root.removeAttribute("data-x11-wasm-pointer-grab-pointer-mode");
        root.removeAttribute("data-x11-wasm-pointer-grab-keyboard-mode");
      }

      if (this.keyboardGrabWindow) {
        root.setAttribute(
          "data-x11-wasm-keyboard-grab-window",
          String(this.keyboardGrabWindow),
        );
        root.setAttribute(
          "data-x11-wasm-keyboard-grab-owner-events",
          this.keyboardGrabOwnerEvents ? "1" : "0",
        );
        root.setAttribute(
          "data-x11-wasm-keyboard-grab-pointer-mode",
          String(this.keyboardGrabPointerMode),
        );
        root.setAttribute(
          "data-x11-wasm-keyboard-grab-keyboard-mode",
          String(this.keyboardGrabKeyboardMode),
        );
      } else {
        root.removeAttribute("data-x11-wasm-keyboard-grab-window");
        root.removeAttribute("data-x11-wasm-keyboard-grab-owner-events");
        root.removeAttribute("data-x11-wasm-keyboard-grab-pointer-mode");
        root.removeAttribute("data-x11-wasm-keyboard-grab-keyboard-mode");
      }
    },

    /**
     * @param {boolean} enabled
     * @param {number} windowId
     * @param {boolean} ownerEvents
     * @param {number} eventMask
     * @param {number} confineWindow
     * @param {number} cursor
     * @param {number} pointerMode
     * @param {number} keyboardMode
     * @returns {number}
     */
    setPointerGrabState(
      enabled,
      windowId,
      ownerEvents,
      eventMask,
      confineWindow,
      cursor,
      pointerMode,
      keyboardMode,
    ) {
      this.pointerGrabWindow = enabled ? Math.trunc(windowId) : 0;
      this.pointerGrabOwnerEvents = enabled ? !!ownerEvents : false;
      this.pointerGrabEventMask = enabled ? Math.trunc(eventMask) : 0;
      this.pointerGrabConfineWindow = enabled ? Math.trunc(confineWindow) : 0;
      this.pointerGrabCursorShape = enabled ? Math.trunc(cursor) : 0;
      this.pointerGrabPointerMode = enabled ? Math.trunc(pointerMode) : 1;
      this.pointerGrabKeyboardMode = enabled ? Math.trunc(keyboardMode) : 1;
      this.applyRootGrabState();
      return 0;
    },

    /**
     * @param {boolean} enabled
     * @param {number} windowId
     * @param {boolean} ownerEvents
     * @param {number} pointerMode
     * @param {number} keyboardMode
     * @returns {number}
     */
    setKeyboardGrabState(
      enabled,
      windowId,
      ownerEvents,
      pointerMode,
      keyboardMode,
    ) {
      this.keyboardGrabWindow = enabled ? Math.trunc(windowId) : 0;
      this.keyboardGrabOwnerEvents = enabled ? !!ownerEvents : false;
      this.keyboardGrabPointerMode = enabled ? Math.trunc(pointerMode) : 1;
      this.keyboardGrabKeyboardMode = enabled ? Math.trunc(keyboardMode) : 1;
      this.applyRootGrabState();
      return 0;
    },

    /**
     * @param {number} rootX
     * @param {number} rootY
     * @param {number} windowId
     * @param {number} state
     * @param {number} captureWindow
     * @returns {number}
     */
    setPointerState(rootX, rootY, windowId, state, captureWindow) {
      this.lastPointerRootX = Number.isFinite(rootX) ? Math.trunc(rootX) : 0;
      this.lastPointerRootY = Number.isFinite(rootY) ? Math.trunc(rootY) : 0;
      this.pointerWindow = unsignedInt(windowId, 0);
      this.pointerState = unsignedInt(state, 0);
      this.pointerCaptureWindow = unsignedInt(captureWindow, 0);
      this.applyRootPointerState();
      return 0;
    },

    /**
     * @param {number} rootX
     * @param {number} rootY
     * @param {number} targetWindow
     * @param {number} time
     * @returns {number}
     */
    warpPointer(rootX, rootY, targetWindow, time) {
      const x = Number.isFinite(rootX) ? Math.trunc(rootX) : 0;
      const y = Number.isFinite(rootY) ? Math.trunc(rootY) : 0;
      const windowId = unsignedInt(targetWindow, 0);
      const eventTime = unsignedInt(time, 0);
      const target = this.rootElement || this.document;
      const view = this.document && this.document.defaultView;

      this.lastPointerRootX = x;
      this.lastPointerRootY = y;
      this.pointerWindow = windowId;
      this.pointerWarpCount += 1;
      this.lastWarpPointer = {
        count: this.pointerWarpCount,
        rootX: x,
        rootY: y,
        targetWindow: windowId,
        time: eventTime,
      };
      this.applyRootPointerState();

      if (target && view && typeof view.CustomEvent === "function") {
        target.dispatchEvent(
          new view.CustomEvent("x11wasmpointerwarp", {
            bubbles: true,
            detail: this.lastWarpPointer,
          }),
        );
      }
      return 0;
    },

applyRootPointerState() {
      const root = this.rootElement;
      if (!root) {
        return;
      }
      root.setAttribute("data-x11-wasm-pointer-x", String(this.lastPointerRootX));
      root.setAttribute("data-x11-wasm-pointer-y", String(this.lastPointerRootY));
      root.setAttribute("data-x11-wasm-pointer-window", String(this.pointerWindow));
      root.setAttribute("data-x11-wasm-pointer-state", String(this.pointerState));
      root.setAttribute(
        "data-x11-wasm-pointer-capture-window",
        String(this.pointerCaptureWindow),
      );
      root.setAttribute(
        "data-x11-wasm-pointer-warp-count",
        String(this.pointerWarpCount),
      );
      if (this.lastWarpPointer) {
        root.setAttribute(
          "data-x11-wasm-pointer-warp-window",
          String(this.lastWarpPointer.targetWindow),
        );
        root.setAttribute(
          "data-x11-wasm-pointer-warp-time",
          String(this.lastWarpPointer.time),
        );
      } else {
        root.removeAttribute("data-x11-wasm-pointer-warp-window");
        root.removeAttribute("data-x11-wasm-pointer-warp-time");
      }
    },

    /**
     * @param {number} id
     * @returns {number}
     */
    mapWindow(id) {
      const resource = this.resources.get(id);
      if (resource && resource.element) {
        this.applyInitialWmState(resource);
        if (!resource.pendingMapped) {
          const context = this.getContext(id);
          if (context) {
            // Preserve existing drawing while materializing the visible background.
            context.save();
            context.globalCompositeOperation = "destination-over";
            this.clearResourceArea(context, resource, 0, 0, resource.width, resource.height);
            context.restore();
          }
        }
        resource.pendingMapped = true;
        this.markGeometryDirty(resource);
        this.markDirty(
          resource,
          0,
          0,
          resource.pendingWidth,
          resource.pendingHeight,
        );
      }
      return 0;
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applyInitialWmState(resource) {
      if (resource.wmState !== "") {
        return;
      }
      const hints = resource.wmHints;
      if (!hints || !(hints.flags & WM_STATE_HINT)) {
        return;
      }
      const initialState = wmStateName(hints.initialState);
      if (initialState) {
        this.setResourceWmState(resource, initialState);
      }
    },

    /**
     * @param {number} id
     * @returns {number}
     */
    unmapWindow(id) {
      this.cancelWindowManagerResource?.(id);
      const resource = this.resources.get(id);
      if (resource && resource.element) {
        resource.pendingMapped = false;
        this.markGeometryDirty(resource);
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} eventMask
     * @param {number} doNotPropagateMask
     * @returns {number}
     */
    setWindowEventMask(id, eventMask, doNotPropagateMask) {
      const resource = this.resources.get(id);
      if (resource) {
        resource.eventMask = Math.max(0, Math.trunc(eventMask));
        resource.doNotPropagateMask = Math.max(
          0,
          Math.trunc(doNotPropagateMask),
        );
        this.applyWindowEventMask(resource);
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} colormap
     * @returns {number}
     */
    setWindowColormap(id, colormap) {
      const resource = this.resources.get(id);
      if (resource) {
        resource.colormap = Math.max(0, Math.trunc(colormap));
        if (resource.element) {
          if (resource.colormap) {
            resource.element.setAttribute(
              "data-x11-wasm-colormap",
              String(resource.colormap),
            );
          } else {
            resource.element.removeAttribute("data-x11-wasm-colormap");
          }
        }
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} overrideRedirect
     * @param {number} saveUnder
     * @param {number} bitGravity
     * @param {number} winGravity
     * @param {number} backingStore
     * @param {number} backingPlanes
     * @param {number} backingPixel
     * @returns {number}
     */
    setWindowAttributes(
      id,
      overrideRedirect,
      saveUnder,
      bitGravity,
      winGravity,
      backingStore,
      backingPlanes,
      backingPixel,
    ) {
      const resource = this.resources.get(id);
      if (resource) {
        resource.overrideRedirect = !!overrideRedirect;
        resource.saveUnder = !!saveUnder;
        resource.bitGravity = Math.trunc(bitGravity);
        resource.winGravity = Math.trunc(winGravity);
        resource.backingStore = Math.trunc(backingStore);
        resource.backingPlanes = Math.max(0, Math.trunc(backingPlanes));
        resource.backingPixel = Math.max(0, Math.trunc(backingPixel));
        this.applyWindowAttributes(resource);
      }
      return 0;
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applyWindowAttributes(resource) {
      const element = resource.element;
      if (!element) {
        return;
      }
      element.setAttribute(
        "data-x11-wasm-override-redirect",
        resource.overrideRedirect ? "1" : "0",
      );
      element.setAttribute("data-x11-wasm-save-under", resource.saveUnder ? "1" : "0");
      element.setAttribute("data-x11-wasm-bit-gravity", String(resource.bitGravity));
      element.setAttribute("data-x11-wasm-win-gravity", String(resource.winGravity));
      element.setAttribute(
        "data-x11-wasm-backing-store",
        String(resource.backingStore),
      );
      element.setAttribute(
        "data-x11-wasm-backing-planes",
        String(resource.backingPlanes),
      );
      element.setAttribute(
        "data-x11-wasm-backing-pixel",
        String(resource.backingPixel),
      );
    },

    /**
     * @param {number} id
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @param {number} borderWidth
     * @param {number} valueMask
     * @returns {number}
     */
    configureWindow(id, x, y, width, height, borderWidth, valueMask) {
      const resource = this.resources.get(id);
      if (!resource) {
        return 0;
      }
      const mask = valueMask || (
        CW_X | CW_Y | CW_WIDTH | CW_HEIGHT | CW_BORDER_WIDTH
      );
      const requestedWidth = positiveInt(
        mask & CW_WIDTH ? width : resource.pendingWidth,
        resource.pendingWidth,
      );
      const requestedHeight = positiveInt(
        mask & CW_HEIGHT ? height : resource.pendingHeight,
        resource.pendingHeight,
      );
      const constrainedSize = this.constrainWindowSize(
        resource,
        requestedWidth,
        requestedHeight,
      );
      const nextWidth =
        mask & CW_WIDTH ? constrainedSize.width : resource.pendingWidth;
      const nextHeight =
        mask & CW_HEIGHT ? constrainedSize.height : resource.pendingHeight;
      const sizeChanged =
        nextWidth !== resource.pendingWidth ||
        nextHeight !== resource.pendingHeight;

      if (mask & CW_X) {
        resource.pendingX = x;
      }
      if (mask & CW_Y) {
        resource.pendingY = y;
      }
      resource.pendingWidth = nextWidth;
      resource.pendingHeight = nextHeight;
      if (mask & CW_BORDER_WIDTH) {
        resource.pendingBorderWidth = Math.max(0, Math.floor(borderWidth));
      }
      this.markGeometryDirty(resource);
      if (sizeChanged) {
        this.markDirty(resource, 0, 0, nextWidth, nextHeight);
      }
      return 0;
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {number} width
     * @param {number} height
     * @returns {{ width: number, height: number }}
     */
    constrainWindowSize(resource, width, height) {
      const hints = resource.kind === "window" ? resource.sizeHints : null;
      if (!hints) {
        return { width, height };
      }
      return this.constrainSizeByHints(hints, width, height);
    },

    /**
     * @param {X11WasmDOMSizeHints} hints
     * @param {number} width
     * @param {number} height
     * @returns {{ width: number, height: number }}
     */
    constrainSizeByHints(hints, width, height) {
      let widthBase = 0;
      let heightBase = 0;
      if (hints.flags & P_BASE_SIZE) {
        widthBase = hints.baseWidth;
        heightBase = hints.baseHeight;
      } else if (hints.flags & P_MIN_SIZE) {
        widthBase = hints.minWidth;
        heightBase = hints.minHeight;
      }

      const minWidth = hints.flags & P_MIN_SIZE ? hints.minWidth
        : hints.flags & P_BASE_SIZE ? hints.baseWidth : 0;
      const minHeight = hints.flags & P_MIN_SIZE ? hints.minHeight
        : hints.flags & P_BASE_SIZE ? hints.baseHeight : 0;
      const maxWidth = hints.flags & P_MAX_SIZE ? hints.maxWidth : 0;
      const maxHeight = hints.flags & P_MAX_SIZE ? hints.maxHeight : 0;
      const widthInc = hints.flags & P_RESIZE_INC ? hints.widthInc : 0;
      const heightInc = hints.flags & P_RESIZE_INC ? hints.heightInc : 0;
      const constrainedWidth = this.constrainWindowSizeAxis(
        width,
        minWidth,
        maxWidth,
        widthInc,
        widthBase,
      );
      const constrainedHeight = this.constrainWindowSizeAxis(
        height,
        minHeight,
        maxHeight,
        heightInc,
        heightBase,
      );

      return this.constrainWindowAspectRatio(
        hints,
        constrainedWidth,
        constrainedHeight,
        minWidth,
        minHeight,
        maxWidth,
        maxHeight,
        widthInc,
        heightInc,
        widthBase,
        heightBase,
      );
    },

    /**
     * @param {number} size
     * @param {number} minSize
     * @param {number} maxSize
     * @param {number} increment
     * @param {number} baseSize
     * @returns {number}
     */
    constrainWindowSizeAxis(size, minSize, maxSize, increment, baseSize) {
      const min =
        Number.isFinite(minSize) && minSize > 0 ? Math.floor(minSize) : 1;
      const hasMax = Number.isFinite(maxSize) && maxSize > 0;
      const max = hasMax ? Math.max(min, Math.floor(maxSize)) : 0;
      let next = Math.max(min, positiveInt(size, min));
      if (hasMax) {
        next = Math.min(max, next);
      }

      const inc =
        Number.isFinite(increment) && increment > 0
          ? Math.floor(increment)
          : 0;
      if (inc <= 1) {
        return next;
      }

      const base =
        Number.isFinite(baseSize) && baseSize > 0 ? Math.floor(baseSize) : 0;
      let snapped =
        next <= base
          ? base
          : base + Math.floor((next - base) / inc) * inc;
      if (snapped < min) {
        snapped = base + Math.ceil((min - base) / inc) * inc;
      }
      if (hasMax && snapped > max) {
        snapped = base + Math.floor((max - base) / inc) * inc;
      }
      snapped = Math.max(min, snapped);
      return hasMax ? Math.min(max, snapped) : snapped;
    },

    /**
     * @param {X11WasmDOMSizeHints} hints
     * @param {number} width
     * @param {number} height
     * @param {number} minWidth
     * @param {number} minHeight
     * @param {number} maxWidth
     * @param {number} maxHeight
     * @param {number} widthInc
     * @param {number} heightInc
     * @param {number} widthBase
     * @param {number} heightBase
     * @returns {{ width: number, height: number }}
     */
    constrainWindowAspectRatio(
      hints,
      width,
      height,
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      widthInc,
      heightInc,
      widthBase,
      heightBase,
    ) {
      if (!(hints.flags & P_ASPECT) || hints.minAspectX <= 0 || hints.minAspectY <= 0 ||
          hints.maxAspectX <= 0 || hints.maxAspectY <= 0) return { width, height };

      let lower = [hints.minAspectX, hints.minAspectY];
      let upper = [hints.maxAspectX, hints.maxAspectY];
      if (lower[0] * upper[1] > upper[0] * lower[1]) [lower, upper] = [upper, lower];
      const baseX = hints.flags & P_BASE_SIZE ? Math.max(0, hints.baseWidth) : 0;
      const baseY = hints.flags & P_BASE_SIZE ? Math.max(0, hints.baseHeight) : 0;
      const withinAspect = (w, h) => w >= baseX && h >= baseY &&
        (w - baseX) * lower[1] >= (h - baseY) * lower[0] &&
        (w - baseX) * upper[1] <= (h - baseY) * upper[0];
      if (withinAspect(width, height)) return { width, height };
      const snapWidth = (value) => this.constrainWindowSizeAxis(value, minWidth,
        maxWidth, widthInc, widthBase);
      const snapHeight = (value) => this.constrainWindowSizeAxis(value, minHeight,
        maxHeight, heightInc, heightBase);
      const candidates = (width - baseX) * lower[1] < (height - baseY) * lower[0]
        ? [[snapWidth(Math.ceil((height - baseY) * lower[0] / lower[1]) + baseX), height],
          [width, snapHeight(Math.floor((width - baseX) * lower[1] / lower[0]) + baseY)]]
        : [[width, snapHeight(Math.ceil((width - baseX) * upper[1] / upper[0]) + baseY)],
          [snapWidth(Math.floor((height - baseY) * upper[0] / upper[1]) + baseX), height]];
      for (const [w, h] of candidates) {
        if (withinAspect(w, h)) return { width: w, height: h };
      }

      // Aspect correction must also remain on both resize-increment lattices.
      // Search the smaller bounded X11 dimension when the fast candidates fail.
      const range = (min, max, origin, increment) => {
        const low = Math.max(1, min || 0);
        const high = Math.min(65535, max > 0 ? max : 65535);
        const base = Math.max(0, origin || 0);
        const step = increment > 0 ? Math.max(1, Math.floor(increment)) : 1;
        return { base, step, first: Math.max(0, Math.ceil((low - base) / step)),
          last: Math.floor((high - base) / step) };
      };
      const x = range(minWidth, maxWidth, widthBase, widthInc);
      const y = range(minHeight, maxHeight, heightBase, heightInc);
      const nearest = (axis, target, low, high) => {
        const first = Math.max(axis.first, Math.ceil((low - axis.base) / axis.step));
        const last = Math.min(axis.last, Math.floor((high - axis.base) / axis.step));
        return first > last ? null : axis.base + Math.max(first,
          Math.min(last, Math.round((target - axis.base) / axis.step))) * axis.step;
      };
      let best = null;
      let distance = Infinity;
      const consider = (w, h) => {
        if (w === null || h === null || !withinAspect(w, h)) return;
        const score = (w - width) ** 2 + (h - height) ** 2;
        if (score < distance) { best = { width: w, height: h }; distance = score; }
      };
      if (x.last - x.first <= y.last - y.first) {
        for (let index = x.first; index <= x.last; index++) {
          const w = x.base + index * x.step;
          const h = nearest(y, height,
            Math.ceil((w - baseX) * upper[1] / upper[0]) + baseY,
            Math.floor((w - baseX) * lower[1] / lower[0]) + baseY);
          consider(w, h);
        }
      } else {
        for (let index = y.first; index <= y.last; index++) {
          const h = y.base + index * y.step;
          const w = nearest(x, width,
            Math.ceil((h - baseY) * lower[0] / lower[1]) + baseX,
            Math.floor((h - baseY) * upper[0] / upper[1]) + baseX);
          consider(w, h);
        }
      }
      return best || { width, height };
    },

    /**
     * @param {number} id
     * @param {number} previousSibling
     * @returns {number}
     */
    restackWindow(id, previousSibling) {
      const resource = this.resources.get(id);
      const element = resource && (this.displayElement?.(resource) || resource.element);
      if (!resource || !element) {
        return 0;
      }
      const parentResource = this.resources.get(resource.parent);
      const parentElement = parentResource?.element || this.rootElement;
      if (!parentElement) {
        return 0;
      }

      const previousElement = this.siblingWindowElement(
        parentElement,
        previousSibling,
      );
      if (previousElement) {
        parentElement.insertBefore(element, previousElement.nextSibling);
      } else {
        const firstWindow = this.firstChildWindowElement(parentElement);
        parentElement.insertBefore(element, firstWindow);
      }
      this.restackRelatedTransientGroups(resource);
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} parent
     * @param {number} x
     * @param {number} y
     * @returns {number}
     */
    reparentWindow(id, parent, x, y) {
      this.cancelWindowManagerResource?.(id);
      this.removeWindowManagerFrame?.(id);
      const resource = this.resources.get(id);
      const parentResource = this.resources.get(parent);
      const parentElement =
        parentResource && parentResource.element
          ? parentResource.element
          : this.rootElement;

      if (!resource || !resource.element || !parentElement) {
        return 0;
      }
      resource.parent = parent;
      resource.pendingX = x;
      resource.pendingY = y;
      resource.x = x;
      resource.y = y;
      if (resource.element.parentNode !== parentElement) {
        parentElement.appendChild(resource.element);
      }
      this.markGeometryDirty(resource);
      this.restackRelatedTransientGroups(resource);
      return 0;
    },

    /**
     * @param {number} id
     * @returns {number}
     */
    clearWindow(id) {
      const resource = this.resources.get(id);
      if (resource) {
        this.clearWindowArea(id, 0, 0, resource.width, resource.height);
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @returns {number}
     */
    clearWindowArea(id, x, y, width, height) {
      const context = this.getContext(id);
      const resource = this.resources.get(id);
      if (context && resource) {
        this.clearResourceArea(context, resource, x, y, width, height);
      }
      return 0;
    }
  }));
})(typeof globalThis !== "undefined" ? globalThis : window);
