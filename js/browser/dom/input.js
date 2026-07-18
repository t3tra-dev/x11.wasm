// @ts-check
/*
 * input.js --
 *
 *	Focus, DOM input, pointer, wheel, keyboard, IME, and native event queue methods.
 */

(function attachX11WasmDOMBackendInput(global) {
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
     * @param {number} window
     */
    setFocusedWindow(window) {
      const windowId = unsignedInt(window, 0);
      if (this.focusedElement) {
        this.focusedElement.removeAttribute("data-x11-wasm-focused");
      }
      this.focusedWindow = windowId;
      this.focusedElement = null;
      if (windowId !== 0) {
        const resource = this.resources.get(windowId);
        if (resource && resource.element) {
          resource.element.setAttribute("data-x11-wasm-focused", "1");
          this.focusedElement = resource.element;
        }
      }
      if (this.inputElement) {
        if (windowId !== 0) {
          this.inputElement.setAttribute(
            "data-x11-wasm-focus-window",
            String(windowId),
          );
        } else {
          this.inputElement.removeAttribute("data-x11-wasm-focus-window");
        }
      }
    },

    /**
     * @param {X11WasmDOMResource | null | undefined} resource
     * @returns {boolean}
     */
    resourceAcceptsInput(resource) {
      if (!resource || (resource.kind !== "window" && resource.kind !== "root")) {
        return false;
      }
      const hints = resource.wmHints;
      return !hints || !(hints.flags & WM_INPUT_HINT) || hints.input !== 0;
    },

    /**
     * @returns {boolean}
     */
    focusedResourceAcceptsInput() {
      const windowId = this.focusedWindow || this.rootWindow;
      return this.resourceAcceptsInput(this.resources.get(windowId));
    },

    /**
     * @param {Event} event
     * @returns {X11WasmDOMResource | null}
     */
    inputResourceForEvent(event) {
      const target = event.target;
      if (!(target instanceof global.Element) || !this.rootElement) {
        return this.resources.get(this.rootWindow) || null;
      }
      const element = target.closest(`[${RESOURCE_ATTR}]`);
      if (
        !(element instanceof global.HTMLElement) ||
        !this.rootElement.contains(element)
      ) {
        return this.resources.get(this.rootWindow) || null;
      }
      const id = Number.parseInt(element.getAttribute(RESOURCE_ATTR) || "0", 10);
      return (Number.isFinite(id) && this.resources.get(id)) || null;
    },

    /**
     * @param {boolean} focused
     * @param {number=} window
     * @returns {number}
     */
    focusInput(focused, window = 0) {
      const input = this.ensureInputElement();
      this.setFocusedWindow(focused ? window : 0);
      if (!input) {
        return 0;
      }
      this.suppressInputFocusEvents += 1;
      try {
        if (focused) {
          input.focus({ preventScroll: true });
        } else {
          input.blur();
        }
      } finally {
        this.suppressInputFocusEvents -= 1;
      }
      return 0;
    },

    /**
     * @param {{ suppressFocusEvent?: boolean }=} options
     * @returns {boolean}
     */
    restoreFocusedInput(options = {}) {
      const input = this.inputElement;
      const document = this.document;
      const suppressFocusEvent = options.suppressFocusEvent === true;

      if (!input || !this.focusedResourceAcceptsInput()) {
        return false;
      }
      if (
        document &&
        typeof document.hasFocus === "function" &&
        !document.hasFocus()
      ) {
        return false;
      }
      if (document && document.activeElement === input) {
        return false;
      }
      if (suppressFocusEvent) {
        this.suppressInputFocusEvents += 1;
      }
      try {
        input.focus({ preventScroll: true });
      } catch (error) {
        this.inputFocusRestoreError =
          error instanceof Error ? error.message : String(error || "");
        return false;
      } finally {
        if (suppressFocusEvent) {
          this.suppressInputFocusEvents -= 1;
        }
      }
      return !document || document.activeElement === input;
    },

    /**
     * @param {boolean} focused
     * @param {number=} window
     * @param {boolean=} hasSpot
     * @param {number=} spotRootX
     * @param {number=} spotRootY
     * @returns {number}
     */
    setInputContext(
      focused,
      window = 0,
      hasSpot = false,
      spotRootX = 0,
      spotRootY = 0,
    ) {
      const input = this.ensureInputElement();
      this.inputContextFocused = !!focused;
      this.inputContextWindow = this.inputContextFocused
        ? unsignedInt(window, 0)
        : 0;
      this.inputContextHasSpot = this.inputContextFocused && !!hasSpot;
      this.inputContextSpotX = Math.trunc(spotRootX) || 0;
      this.inputContextSpotY = Math.trunc(spotRootY) || 0;
      if (!input) {
        return 0;
      }
      if (!this.inputContextFocused) {
        input.removeAttribute("data-x11-wasm-ic-focused");
        input.removeAttribute("data-x11-wasm-ic-window");
        input.removeAttribute("data-x11-wasm-ic-has-spot");
        input.removeAttribute("data-x11-wasm-ic-spot-x");
        input.removeAttribute("data-x11-wasm-ic-spot-y");
        input.style.left = "0px";
        input.style.top = "0px";
        return 0;
      }
      input.setAttribute("data-x11-wasm-ic-focused", "1");
      input.setAttribute(
        "data-x11-wasm-ic-window",
        String(this.inputContextWindow),
      );
      input.setAttribute(
        "data-x11-wasm-ic-has-spot",
        this.inputContextHasSpot ? "1" : "0",
      );
      if (this.inputContextHasSpot) {
        input.setAttribute(
          "data-x11-wasm-ic-spot-x",
          String(this.inputContextSpotX),
        );
        input.setAttribute(
          "data-x11-wasm-ic-spot-y",
          String(this.inputContextSpotY),
        );
        input.style.left = `${this.inputContextSpotX}px`;
        input.style.top = `${this.inputContextSpotY}px`;
      } else {
        input.removeAttribute("data-x11-wasm-ic-spot-x");
        input.removeAttribute("data-x11-wasm-ic-spot-y");
        input.style.left = "0px";
        input.style.top = "0px";
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {boolean} hasBackground
     * @param {number} backgroundPixel
     * @param {number} backgroundPixmap
     * @param {boolean} hasBorder
     * @param {number} borderPixel
     * @param {number} borderPixmap
     * @param {boolean} hasCursor
     * @param {number} cursor
     * @returns {number}
     */
    setWindowStyle(
      id,
      hasBackground,
      backgroundPixel,
      backgroundPixmap,
      hasBorder,
      borderPixel,
      borderPixmap,
      hasCursor,
      cursor,
    ) {
      const resource = this.resources.get(id);
      if (!resource) {
        return 0;
      }
      resource.backgroundColor = hasBackground
        ? pixelToCss(backgroundPixel)
        : null;
      resource.backgroundPixmap = hasBackground ? 0 : Math.trunc(backgroundPixmap);
      resource.borderColor = hasBorder ? pixelToCss(borderPixel) : null;
      resource.borderPixmap = hasBorder ? 0 : Math.trunc(borderPixmap);
      resource.cursor = hasCursor ? this.cursorStyle(cursor) : null;
      this.applyStyle(resource);
      return 0;
    },

    /**
     * @returns {HTMLTextAreaElement | null}
     */
    ensureInputElement() {
      const document = this.document;
      const root = this.rootElement;
      if (!document || !root) {
        return null;
      }
      if (this.inputElement) {
        return this.inputElement;
      }
      const input = document.createElement("textarea");
      input.className = "x11-wasm-input";
      input.setAttribute("aria-hidden", "true");
      input.autocomplete = "off";
      input.autocapitalize = "off";
      input.spellcheck = false;
      input.rows = 1;
      input.cols = 1;
      input.wrap = "off";
      input.value = "";
      root.appendChild(input);
      this.inputElement = input;
      return input;
    },

    /**
     * @param {HTMLElement} root
     */
    installEventListeners(root) {
      const input = this.ensureInputElement();
      if (!input || this.listenersInstalled) {
        return;
      }
      this.listenersInstalled = true;

      root.addEventListener("contextmenu", (event) => {
        event.preventDefault();
      });
      root.addEventListener("pointerdown", (event) => {
        this.recordUserActivity();
        const inputWasFocused = document.activeElement === input;
        this.queuePointerEvent(BUTTON_PRESS, event);
        this.capturePointer(root, event);
        if (this.resourceAcceptsInput(this.inputResourceForEvent(event))) {
          input.focus();
        }
        if (inputWasFocused) {
          this.queueFocusEvent(FOCUS_IN, eventTime(event.timeStamp));
        }
        event.preventDefault();
        this.serviceUserActivationEvents();
      });
      root.addEventListener("pointerup", (event) => {
        this.recordUserActivity();
        this.queuePointerEvent(BUTTON_RELEASE, event);
        this.releasePointer(root, event);
        event.preventDefault();
        this.serviceUserActivationEvents();
      });
      root.addEventListener("pointermove", (event) => {
        this.recordUserActivity();
        this.queuePointerEvent(MOTION_NOTIFY, event);
      });
      root.addEventListener("pointerover", (event) => {
        this.queueCrossingEvent(ENTER_NOTIFY, event);
      });
      root.addEventListener("pointerout", (event) => {
        this.queueCrossingEvent(LEAVE_NOTIFY, event);
      });
      root.addEventListener(
        "wheel",
        (event) => {
          this.recordUserActivity();
          this.queueWheelEvent(event);
          event.preventDefault();
        },
        { passive: false },
      );
      root.addEventListener("lostpointercapture", () => {
        this.capturedPointerId = null;
        this.capturedPointerElement = null;
      });
      root.addEventListener("focus", () => {
        this.restoreFocusedInput();
      });

      input.addEventListener("focus", (event) => {
        if (this.suppressInputFocusEvents > 0) {
          return;
        }
        this.queueFocusEvent(
          FOCUS_IN,
          eventTime(event.timeStamp),
          this.focusedWindow || this.rootWindow,
        );
      });
      input.addEventListener("blur", (event) => {
        if (this.suppressInputFocusEvents > 0) {
          return;
        }
        if (this.suppressTransientInputBlurEvents > 0) {
          this.transientInputBlurred = true;
          return;
        }
        this.queueFocusEvent(
          FOCUS_OUT,
          eventTime(event.timeStamp),
          this.focusedWindow || this.rootWindow,
        );
      });
      input.addEventListener("keydown", (event) => {
        this.recordUserActivity();
        this.handleKeyDown(event);
      });
      input.addEventListener("keypress", (event) => {
        this.recordUserActivity();
        this.handleKeyPress(event);
      });
      input.addEventListener("keyup", (event) => {
        this.recordUserActivity();
        this.handleKeyUp(event);
      });
      input.addEventListener("compositionstart", (event) => {
        this.recordUserActivity();
        this.startComposition(event.data || "");
      });
      input.addEventListener("compositionupdate", (event) => {
        this.recordUserActivity();
        this.updateComposition(event.data || "");
      });
      input.addEventListener("compositionend", (event) => {
        this.recordUserActivity();
        this.endComposition(event.data || "");
      });
      input.addEventListener("beforeinput", (event) => {
        this.recordUserActivity();
        this.handleBeforeInput(event);
      });
      input.addEventListener("paste", (event) => {
        this.handlePaste(event);
      });
      input.addEventListener("input", (event) => {
        if (event instanceof InputEvent && event.isComposing) {
          if (event.inputType === "insertCompositionText") {
            this.updateComposition(event.data || input.value);
          }
          return;
        }
        if (!this.composing) {
          this.flushInputText();
        }
      });
    },

    serviceUserActivationEvents() {
      const service =
        this.module && this.module.x11WasmServiceUserActivationEvents;

      if (typeof service !== "function" || this.servicingUserActivationEvents) {
        return;
      }
      this.servicingUserActivationEvents = true;
      this.suppressTransientInputBlurEvents =
        (this.suppressTransientInputBlurEvents || 0) + 1;
      const finishUserActivationService = () => {
        const transientInputBlurred = this.transientInputBlurred === true;
        this.transientInputBlurred = false;
        this.suppressTransientInputBlurEvents = Math.max(
          0,
          (this.suppressTransientInputBlurEvents || 0) - 1,
        );
        if (
          this.restoreFocusedInput({
            suppressFocusEvent: transientInputBlurred,
          }) &&
          !transientInputBlurred
        ) {
          service();
        }
        this.servicingUserActivationEvents = false;
      };
      try {
        const result = service();
        if (result && typeof result.then === "function") {
          result.then(finishUserActivationService).catch((error) => {
            this.userActivationServiceError =
              error instanceof Error ? error.message : String(error || "");
            finishUserActivationService();
          });
        } else {
          finishUserActivationService();
        }
      } catch (error) {
        this.userActivationServiceError =
          error instanceof Error ? error.message : String(error || "");
        finishUserActivationService();
      }
    },

    /**
     * @param {HTMLElement} root
     * @param {PointerEvent} event
     */
    capturePointer(root, event) {
      const captureTarget = root;

      if (typeof captureTarget.setPointerCapture === "function") {
        try {
          captureTarget.setPointerCapture(event.pointerId);
        } catch (error) {
          /* Pointer capture is opportunistic; event queueing still works. */
        }
      }
      this.capturedPointerId = event.pointerId;
      this.capturedPointerElement = captureTarget;
    },

    /**
     * @param {HTMLElement} root
     * @param {PointerEvent} event
     */
    releasePointer(root, event) {
      if (this.capturedPointerId !== event.pointerId) {
        return;
      }
      const captureTarget = this.capturedPointerElement || root;

      if (typeof captureTarget.releasePointerCapture === "function") {
        try {
          captureTarget.releasePointerCapture(event.pointerId);
        } catch (error) {
          /* The browser may have already released capture. */
        }
      }
      this.capturedPointerId = null;
      this.capturedPointerElement = null;
    },

    /**
     * @param {MouseEvent | PointerEvent | WheelEvent} event
     * @returns {{x: number, y: number}}
     */
    eventRootCoordinates(event) {
      const element = this.rootElement;
      if (!element) {
        return { x: 0, y: 0 };
      }
      const rect = element.getBoundingClientRect();
      const coordinates = {
        x: Math.floor(event.clientX - rect.left),
        y: Math.floor(event.clientY - rect.top),
      };
      this.lastPointerRootX = coordinates.x;
      this.lastPointerRootY = coordinates.y;
      this.applyRootPointerState();
      return coordinates;
    },

    /**
     * @param {number} type
     * @param {PointerEvent} event
     */
    queueCrossingEvent(type, event) {
      const { x, y } = this.eventRootCoordinates(event);
      const queue = this.module && this.module["_X11WasmQueueCrossingEvent"];
      if (typeof queue === "function") {
        queue(
          type,
          this.rootWindow,
          pointerButtonState(event),
          x,
          y,
          eventTime(event.timeStamp),
        );
      }
    },

    /**
     * @param {number} type
     * @param {number} time
     * @param {number=} window
     */
    queueFocusEvent(type, time, window = 0) {
      const queue = this.module && this.module["_X11WasmQueueFocusEvent"];
      if (typeof queue === "function") {
        queue(type, unsignedInt(window, this.rootWindow), time);
      }
    },

    /**
     * @param {WheelEvent} event
     */
    queueWheelEvent(event) {
      const { x, y } = this.eventRootCoordinates(event);
      const queue = this.module && this.module["_X11WasmQueueWheelEvent"];
      const wheel = this.wheelDeltaForEvent(event);
      if (typeof queue === "function" && wheel.delta !== 0) {
        queue(
          this.rootWindow,
          wheel.delta,
          wheel.state,
          x,
          y,
          eventTime(event.timeStamp),
        );
      }
    },

    /**
     * @param {WheelEvent} event
     * @returns {{ delta: number, state: number }}
     */
    wheelDeltaForEvent(event) {
      const horizontal = Math.abs(event.deltaX) > Math.abs(event.deltaY);
      const rawDelta = horizontal ? event.deltaX : event.deltaY;
      const state = modifierState(event) | (horizontal ? SHIFT_MASK : 0);

      if (rawDelta === 0) {
        return { delta: 0, state };
      }
      if (horizontal) {
        this.wheelRemainderX += rawDelta * this.wheelDeltaScale(event);
      } else {
        this.wheelRemainderY += rawDelta * this.wheelDeltaScale(event);
      }

      const remainder = horizontal
        ? this.wheelRemainderX
        : this.wheelRemainderY;
      const ticks =
        remainder < 0
          ? Math.ceil(remainder / WHEEL_PIXEL_THRESHOLD)
          : Math.floor(remainder / WHEEL_PIXEL_THRESHOLD);
      if (ticks === 0) {
        return { delta: 0, state };
      }
      if (horizontal) {
        this.wheelRemainderX -= ticks * WHEEL_PIXEL_THRESHOLD;
      } else {
        this.wheelRemainderY -= ticks * WHEEL_PIXEL_THRESHOLD;
      }
      return { delta: -ticks * 120, state };
    },

    /**
     * @param {WheelEvent} event
     * @returns {number}
     */
    wheelDeltaScale(event) {
      if (event.deltaMode === 1) {
        return WHEEL_LINE_SCALE;
      }
      if (event.deltaMode === 2) {
        return WHEEL_PAGE_SCALE;
      }
      return 1;
    },

    /**
     * @param {number} type
     * @param {PointerEvent} event
     */
    queuePointerEvent(type, event) {
      const queue = this.module && this.module["_X11WasmQueuePointerEvent"];
      if (typeof queue === "function") {
        for (const pointerEvent of this.pointerEventsForQueue(type, event)) {
          const { x, y } = this.eventRootCoordinates(pointerEvent);
          queue(
            type,
            this.rootWindow,
            xButtonFromPointerEvent(pointerEvent),
            pointerEventButtonState(type, pointerEvent),
            x,
            y,
            eventTime(pointerEvent.timeStamp),
          );
        }
      }
    },

    /**
     * @param {number} type
     * @param {PointerEvent} event
     * @returns {PointerEvent[]}
     */
    pointerEventsForQueue(type, event) {
      if (
        type !== MOTION_NOTIFY ||
        typeof event.getCoalescedEvents !== "function"
      ) {
        return [event];
      }

      const coalescedEvents = event.getCoalescedEvents();
      if (coalescedEvents.length === 0) {
        return [event];
      }

      const lastEvent = coalescedEvents[coalescedEvents.length - 1];
      if (lastEvent && this.sameQueuedPointerEvent(lastEvent, event)) {
        return coalescedEvents;
      }
      return [...coalescedEvents, event];
    },

    /**
     * @param {PointerEvent} a
     * @param {PointerEvent} b
     * @returns {boolean}
     */
    sameQueuedPointerEvent(a, b) {
      return (
        a.clientX === b.clientX &&
        a.clientY === b.clientY &&
        a.button === b.button &&
        a.buttons === b.buttons &&
        a.shiftKey === b.shiftKey &&
        a.ctrlKey === b.ctrlKey &&
        a.altKey === b.altKey &&
        a.metaKey === b.metaKey
      );
    },

    /**
     * @param {KeyboardEvent} event
     */
    handleKeyDown(event) {
      if (this.isImeKeyEvent(event)) {
        this.queueImeKeyEvent(KEY_PRESS, event);
        return;
      }
      if (this.isHostPasteShortcut(event)) {
        this.hostPasteShortcutActive = true;
        return;
      }
      if (this.plainTextFromKeyboardEvent(event) !== null) {
        return;
      }
      const keySym = keySymForKeyboardEvent(event);
      if (keySym === 0) {
        return;
      }
      this.queueKeyEvent(
        KEY_PRESS,
        keySym,
        keyCodeForKeyboardEvent(event, keySym),
        modifierState(event),
        eventTime(event.timeStamp),
      );
      if (event.ctrlKey || event.metaKey || event.altKey) {
        this.serviceUserActivationEvents();
      }
      event.preventDefault();
    },

    /**
     * @param {KeyboardEvent} event
     */
    handleKeyPress(event) {
      if (this.isImeKeyEvent(event)) {
        return;
      }
      if (this.isHostPasteShortcut(event)) {
        return;
      }
      if (this.plainTextFromKeyboardEvent(event) !== null) {
        return;
      }
      if (keySymForKeyboardEvent(event) !== 0) {
        event.preventDefault();
      }
    },

    /**
     * @param {KeyboardEvent} event
     */
    handleKeyUp(event) {
      if (this.isImeKeyEvent(event)) {
        this.queueImeKeyEvent(KEY_RELEASE, event);
        return;
      }
      if (this.hostPasteShortcutActive && this.isPasteShortcutKey(event)) {
        this.hostPasteShortcutActive = false;
        return;
      }
      const text = this.plainTextFromKeyboardEvent(event);
      if (text !== null) {
        this.queueTextEvent(
          KEY_RELEASE,
          text,
          modifierState(event),
          eventTime(event.timeStamp),
        );
        event.preventDefault();
        return;
      }
      const keySym = keySymForKeyboardEvent(event);
      if (keySym === 0) {
        return;
      }
      this.queueKeyEvent(
        KEY_RELEASE,
        keySym,
        keyCodeForKeyboardEvent(event, keySym),
        modifierState(event),
        eventTime(event.timeStamp),
      );
      if (event.ctrlKey || event.metaKey || event.altKey) {
        this.serviceUserActivationEvents();
      }
      event.preventDefault();
    },

    /**
     * @returns {boolean}
     */
    isMacHostPlatform() {
      const navigator = global.navigator || {};
      const userAgentData = navigator.userAgentData;
      const platform =
        userAgentData && typeof userAgentData.platform === "string"
          ? userAgentData.platform
          : typeof navigator.platform === "string"
            ? navigator.platform
            : "";
      return /Mac|iPhone|iPad|iPod/i.test(platform);
    },

    /**
     * @param {KeyboardEvent} event
     * @returns {string}
     */
    shortcutKey(event) {
      return typeof event.key === "string" ? event.key.toLowerCase() : "";
    },

    /**
     * @param {KeyboardEvent} event
     * @returns {boolean}
     */
    isHostShortcutModifier(event) {
      if (event.altKey) {
        return false;
      }
      if (this.isMacHostPlatform()) {
        return event.metaKey && !event.ctrlKey;
      }
      return event.ctrlKey && !event.metaKey;
    },

    /**
     * @param {KeyboardEvent} event
     * @returns {boolean}
     */
    isPasteShortcutKey(event) {
      return this.shortcutKey(event) === "v";
    },

    /**
     * @param {KeyboardEvent} event
     * @returns {boolean}
     */
    isHostPasteShortcut(event) {
      return (
        this.isHostShortcutModifier(event) &&
        !event.shiftKey &&
        this.isPasteShortcutKey(event)
      );
    },

    /**
     * @param {KeyboardEvent} event
     * @returns {boolean}
     */
    isImeKeyEvent(event) {
      return (
        event.isComposing ||
        this.composing ||
        event.key === "Process" ||
        event.keyCode === 229
      );
    },

    /**
     * @param {KeyboardEvent} event
     * @returns {string | null}
     */
    plainTextFromKeyboardEvent(event) {
      if (event.ctrlKey || event.metaKey || event.altKey) {
        return null;
      }
      return singleCodepointText(event.key);
    },

    /**
     * @param {InputEvent} event
     */
    handleBeforeInput(event) {
      if (event.isComposing || event.inputType === "insertCompositionText") {
        this.updateComposition(event.data || "");
      }
    },

    /**
     * @param {string} text
     */
    startComposition(text) {
      this.composing = true;
      this.compositionWindow = this.compositionTargetWindow();
      this.compositionText = text;
      this.queueCompositionEvent(COMPOSITION_START, text, this.compositionWindow);
    },

    /**
     * @param {string} text
     */
    updateComposition(text) {
      if (!this.composing) {
        this.startComposition(text);
        return;
      }
      if (text === this.compositionText) {
        return;
      }
      this.compositionText = text;
      this.queueCompositionEvent(COMPOSITION_UPDATE, text, this.compositionWindow);
    },

    /**
     * @param {string} text
     */
    endComposition(text) {
      const input = this.inputElement;
      const finalText = text || (input ? input.value : "");
      const targetWindow = this.compositionWindow || this.compositionTargetWindow();

      this.composing = false;
      this.compositionWindow = 0;
      this.compositionText = "";
      this.queueCompositionEvent(COMPOSITION_END, finalText, targetWindow);
      this.flushInputText(finalText || undefined, targetWindow);
    },

    /**
     * @returns {number}
     */
    compositionTargetWindow() {
      return this.inputContextWindow || this.focusedWindow || this.rootWindow;
    },

    /**
     * @param {string=} textOverride
     * @param {number=} window
     */
    flushInputText(textOverride, window = 0) {
      const input = this.inputElement;
      if (!input) {
        return;
      }
      const text =
        typeof textOverride === "string" ? textOverride : input.value;
      if (text.length === 0) {
        return;
      }
      input.value = "";
      for (const char of text) {
        this.queueTextEvent(KEY_PRESS, char, 0, 0, window);
      }
    },

    /**
     * @param {number} type
     * @param {string} text
     * @param {number} state
     * @param {number} time
     * @param {number=} window
     */
    queueTextEvent(type, text, state, time, window = 0) {
      const codepoint = text.codePointAt(0);
      const pinnedWindow = unsignedInt(window, 0);
      const targetWindow = pinnedWindow || this.rootWindow;
      const queueFieldsAtWindow =
        this.module && this.module["_X11WasmQueueTextEventFieldsAtWindow"];
      const queueFields =
        this.module && this.module["_X11WasmQueueTextEventFields"];
      const queue = this.module && this.module["_X11WasmQueueTextEvent"];

      if (typeof codepoint !== "number") {
        return;
      }
      if (pinnedWindow !== 0 && typeof queueFieldsAtWindow === "function") {
        queueFieldsAtWindow(type, targetWindow, codepoint, state, time);
      } else if (typeof queueFields === "function") {
        queueFields(type, targetWindow, codepoint, state, time);
      } else if (type === KEY_PRESS && typeof queue === "function") {
        queue(targetWindow, codepoint, state, time);
      }
    },

    /**
     * @param {string} text
     * @param {(pointer: number, byteLength: number) => number} callback
     * @returns {number}
     */
    withUtf8String(text, callback) {
      const module = this.module;
      const malloc = module && module["_malloc"];
      const free = module && module["_free"];

      if (typeof malloc !== "function") {
        return 0;
      }
      const bytes = new TextEncoder().encode(text);
      const pointer = malloc(bytes.length + 1);
      if (!pointer) {
        return 0;
      }
      HEAPU8.set(bytes, pointer);
      HEAPU8[pointer + bytes.length] = 0;
      try {
        return callback(pointer, bytes.length);
      } finally {
        if (typeof free === "function") {
          free(pointer);
        }
      }
    },

    /**
     * @param {number} phase
     * @param {string} text
     * @param {number=} window
     */
    queueCompositionEvent(phase, text, window = 0) {
      const queue =
        this.module && this.module["_X11WasmQueueCompositionEvent"];
      const targetWindow = unsignedInt(window, 0) || this.rootWindow;

      if (typeof queue !== "function") {
        return;
      }
      this.withUtf8String(text, (pointer, byteLength) =>
        queue(phase, targetWindow, pointer, byteLength),
      );
    },

    /**
     * @param {number} time
     */
    queuePasteShortcut(time) {
      const keySym = codepointToKeysym("v".codePointAt(0) || 0);
      const keyCode = 86;

      this.queueKeyEvent(KEY_PRESS, keySym, keyCode, CONTROL_MASK, time);
      this.queueKeyEvent(KEY_RELEASE, keySym, keyCode, CONTROL_MASK, time);
    },

    /**
     * @param {number} type
     * @param {number} keySym
     * @param {number} keyCode
     * @param {number} state
     * @param {number} time
     */
    queueKeyEvent(type, keySym, keyCode, state, time) {
      const queueFields =
        this.module && this.module["_X11WasmQueueKeyEventFields"];
      if (typeof queueFields === "function") {
        queueFields(
          type,
          this.rootWindow,
          keyCode,
          keySym,
          state,
          this.lastPointerRootX,
          this.lastPointerRootY,
          time,
        );
        return;
      }
      const queue = this.module && this.module["_X11WasmQueueKeyEvent"];
      if (typeof queue === "function") {
        queue(type, this.rootWindow, keySym, state, time);
      }
    },

    /**
     * @param {number} type
     * @param {KeyboardEvent} event
     */
    queueImeKeyEvent(type, event) {
      const queue =
        this.module && this.module["_X11WasmQueueIMKeyEventFields"];
      if (typeof queue !== "function") {
        return;
      }
      queue(
        type,
        this.rootWindow,
        event.keyCode || event.which || 229,
        modifierState(event),
        this.lastPointerRootX,
        this.lastPointerRootY,
        eventTime(event.timeStamp),
      );
    },

    /**
     * @param {number} id
     * @param {X11WasmDOMResourceKind} kind
     * @param {number} parent
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @param {number} depth
     * @param {number} eventMask
     * @param {HTMLElement | null} element
     * @param {HTMLCanvasElement | OffscreenCanvas | null} canvas
     * @returns {X11WasmDOMResource}
     */
    createResourceRecord(
      id,
      kind,
      parent,
      x,
      y,
      width,
      height,
      depth,
      eventMask,
      element,
      canvas,
      windowClass = 0,
    ) {
      const resourceWidth = positiveInt(width, 1);
      const resourceHeight = positiveInt(height, 1);
      if (canvas) {
        canvas.width = resourceWidth;
        canvas.height = resourceHeight;
      }
      return {
        id,
        kind,
        parent,
        x,
        y,
        width: resourceWidth,
        height: resourceHeight,
        borderWidth: 0,
        pendingX: x,
        pendingY: y,
        pendingWidth: resourceWidth,
        pendingHeight: resourceHeight,
        pendingBorderWidth: 0,
        depth,
        windowClass,
        eventMask,
        doNotPropagateMask: 0,
        colormap: 0,
        overrideRedirect: false,
        saveUnder: false,
        bitGravity: 1,
        winGravity: 1,
        backingStore: 0,
        backingPlanes: 0xffffffff,
        backingPixel: 0,
        mapped: false,
        pendingMapped: false,
        geometryDirty: false,
        dirty: false,
        damage: null,
        damageSerial: 0,
        element,
        canvas,
        context: canvas ? canvas.getContext("2d") : null,
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
        wmState: "",
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
	      };
    }
  }));
})(typeof globalThis !== "undefined" ? globalThis : window);
