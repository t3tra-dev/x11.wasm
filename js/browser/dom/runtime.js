// @ts-check
/*
 * runtime.js --
 *
 *	Bell, screen saver, host access, request, grab, clipboard, and selection methods.
 */

(function attachX11WasmDOMBackendRuntime(global) {
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
     * @param {number} percent
     * @param {number} pitch
     * @param {number} duration
     * @returns {number}
     */
    bell(percent, pitch, duration) {
      const rawPercent = Number.isFinite(percent) ? Math.trunc(percent) : 0;
      const volumePercent = Math.max(0, Math.min(100, rawPercent));
      const rawPitch = Number.isFinite(pitch) ? Math.trunc(pitch) : 400;
      const rawDuration = Number.isFinite(duration)
        ? Math.trunc(duration)
        : 100;
      const volume = Math.max(0, Math.min(1, volumePercent / 100));
      const durationMs = Math.max(0, rawDuration);
      const frequency = Math.max(0, rawPitch);
      const target = this.rootElement || this.document;
      const view = this.document && this.document.defaultView;
      const navigatorObject =
        (view && view.navigator) ||
        (typeof global.navigator !== "undefined" ? global.navigator : null);

      this.bellCount += 1;
      this.lastBellPercent = volumePercent;
      this.lastBellDuration = durationMs;
      this.lastBellFrequency = frequency;
      this.lastBellVolume = volume;
      this.bellAudioStatus = "unavailable";
      this.bellAudioError = "";
      this.bellVibrationStatus = "unavailable";
      this.bellVibrationError = "";

      if (volume <= 0 || durationMs <= 0 || frequency <= 0) {
        this.bellAudioStatus = "suppressed";
      } else if (view) {
        const audioContextConstructor =
          view.AudioContext || /** @type {any} */ (view).webkitAudioContext;

        if (typeof audioContextConstructor === "function") {
          try {
            const context =
              this.bellAudioContext || new audioContextConstructor();
            this.bellAudioContext = context;

            if (
              context.state === "suspended" &&
              typeof context.resume === "function"
            ) {
              const resumeResult = context.resume();
              if (resumeResult && typeof resumeResult.catch === "function") {
                resumeResult.catch((error) => {
                  this.bellAudioStatus = "resume-failed";
                  this.bellAudioError =
                    error instanceof Error ? error.message : String(error);
                });
              }
            }

            const oscillator = context.createOscillator();
            const gain = context.createGain();
            oscillator.type = "sine";
            oscillator.frequency.value = frequency;
            gain.gain.value = volume;
            oscillator.connect(gain);
            gain.connect(context.destination);

            const cleanup = () => {
              try {
                oscillator.disconnect();
                gain.disconnect();
              } catch (error) {
                // Audio nodes may already be disconnected by the browser.
              }
            };
            if (typeof oscillator.addEventListener === "function") {
              oscillator.addEventListener("ended", cleanup, { once: true });
            } else {
              oscillator.onended = cleanup;
            }

            const now =
              typeof context.currentTime === "number"
                ? context.currentTime
                : 0;
            oscillator.start(now);
            oscillator.stop(now + durationMs / 1000);
            this.bellAudioStatus = "played";
          } catch (error) {
            this.bellAudioStatus = "failed";
            this.bellAudioError =
              error instanceof Error ? error.message : String(error);
          }
        }
      }

      if (
        navigatorObject &&
        typeof navigatorObject.vibrate === "function" &&
        durationMs > 0 &&
        volume > 0
      ) {
        try {
          this.bellVibrationStatus =
            navigatorObject.vibrate(durationMs) === false
              ? "rejected"
              : "vibrated";
        } catch (error) {
          this.bellVibrationStatus = "failed";
          this.bellVibrationError =
            error instanceof Error ? error.message : String(error);
        }
      }

      if (target && view && typeof view.CustomEvent === "function") {
        target.dispatchEvent(
          new view.CustomEvent("x11wasmbell", {
            bubbles: true,
            detail: {
              count: this.bellCount,
              percent: this.lastBellPercent,
              duration: this.lastBellDuration,
              frequency: this.lastBellFrequency,
              volume: this.lastBellVolume,
              audioStatus: this.bellAudioStatus,
              vibrationStatus: this.bellVibrationStatus,
            },
          }),
        );
      }
      return 0;
    },

    /**
     * @param {number} mode
     * @returns {number}
     */
    forceScreenSaver(mode) {
      const normalizedMode = Math.trunc(mode);
      this.lastScreenSaverMode = normalizedMode;
      if (normalizedMode === 0) {
        this.screenSaverResetCount += 1;
        this.recordUserActivity();
      }
      this.updateScreenWakeLock();
      this.applyRootScreenSaverState();

      const target = this.rootElement || this.document;
      const view = this.document && this.document.defaultView;
      if (target && view && typeof view.CustomEvent === "function") {
        target.dispatchEvent(
          new view.CustomEvent("x11wasmscreensaver", {
            bubbles: true,
            detail: {
              mode: normalizedMode,
              resetCount: this.screenSaverResetCount,
              lastUserActivityTime: this.lastUserActivityTime,
            },
          }),
        );
      }
      return 0;
    },

    recordUserActivity() {
      this.lastUserActivityTime = Date.now();
      this.applyRootScreenSaverState();
    },

    /**
     * @returns {number}
     */
    getUserIdleTime() {
      if (!this.lastUserActivityTime) {
        return 0;
      }
      return Math.max(0, Math.trunc(Date.now() - this.lastUserActivityTime));
    },

    /**
     * @param {number} timeout
     * @param {number} interval
     * @param {number} preferBlanking
     * @param {number} allowExposures
     * @returns {number}
     */
    setScreenSaverSettings(
      timeout,
      interval,
      preferBlanking,
      allowExposures,
    ) {
      this.screenSaverTimeout = Math.trunc(timeout);
      this.screenSaverInterval = Math.trunc(interval);
      this.screenSaverPreferBlanking = Math.trunc(preferBlanking);
      this.screenSaverAllowExposures = Math.trunc(allowExposures);
      this.updateScreenWakeLock();
      this.applyRootScreenSaverState();
      return 0;
    },

    /**
     * @param {number} suspended
     * @returns {number}
     */
    setScreenSaverSuspended(suspended) {
      this.screenSaverSuspended = Math.trunc(suspended) !== 0;
      if (!this.screenSaverSuspended && this.lastScreenSaverMode !== 1) {
        this.recordUserActivity();
      } else {
        this.applyRootScreenSaverState();
      }
      this.updateScreenWakeLock();
      return 0;
    },

    applyRootScreenSaverState() {
      const root = this.rootElement;
      if (!root) {
        return;
      }
      root.setAttribute(
        "data-x11-wasm-screensaver-mode",
        String(this.lastScreenSaverMode),
      );
      root.setAttribute(
        "data-x11-wasm-screensaver-reset-count",
        String(this.screenSaverResetCount),
      );
      root.setAttribute(
        "data-x11-wasm-last-user-activity-time",
        String(this.lastUserActivityTime),
      );
      root.setAttribute(
        "data-x11-wasm-screensaver-timeout",
        String(this.screenSaverTimeout),
      );
      root.setAttribute(
        "data-x11-wasm-screensaver-interval",
        String(this.screenSaverInterval),
      );
      root.setAttribute(
        "data-x11-wasm-screensaver-prefer-blanking",
        String(this.screenSaverPreferBlanking),
      );
      root.setAttribute(
        "data-x11-wasm-screensaver-allow-exposures",
        String(this.screenSaverAllowExposures),
      );
      root.setAttribute(
        "data-x11-wasm-screensaver-suspended",
        this.screenSaverSuspended ? "1" : "0",
      );
      root.setAttribute(
        "data-x11-wasm-screen-wake-lock-status",
        this.screenWakeLockStatus,
      );
      root.setAttribute(
        "data-x11-wasm-screen-wake-lock-active",
        this.screenWakeLock ? "1" : "0",
      );
      root.setAttribute(
        "data-x11-wasm-screen-wake-lock-type",
        this.screenWakeLockType,
      );
      if (this.screenWakeLockError) {
        root.setAttribute(
          "data-x11-wasm-screen-wake-lock-error",
          this.screenWakeLockError,
        );
      } else {
        root.removeAttribute("data-x11-wasm-screen-wake-lock-error");
      }
    },

    screenSaverWantsWakeLock() {
      return (
        this.lastScreenSaverMode !== 1 &&
        (this.screenSaverSuspended || this.screenSaverPreferBlanking === 0)
      );
    },

    updateScreenWakeLock() {
      if (this.screenSaverWantsWakeLock()) {
        this.requestScreenWakeLock();
      } else {
        this.releaseScreenWakeLock(
          this.screenWakeLock ? "released" : "not-requested",
        );
      }
    },

    requestScreenWakeLock() {
      const view = this.document && this.document.defaultView;
      const navigatorObject =
        (view && view.navigator) ||
        (typeof global.navigator !== "undefined" ? global.navigator : null);
      const wakeLock = navigatorObject && navigatorObject.wakeLock;

      if (this.screenWakeLock) {
        this.screenWakeLockStatus = "active";
        this.screenWakeLockError = "";
        this.screenWakeLockType = "screen";
        this.applyRootScreenSaverState();
        return;
      }
      if (this.screenWakeLockStatus === "requesting") {
        return;
      }
      if (!wakeLock || typeof wakeLock.request !== "function") {
        this.screenWakeLockStatus = "unavailable";
        this.screenWakeLockError = "";
        this.screenWakeLockType = "";
        this.applyRootScreenSaverState();
        return;
      }

      this.screenWakeLockStatus = "requesting";
      this.screenWakeLockError = "";
      this.screenWakeLockType = "screen";
      this.applyRootScreenSaverState();

      try {
        const requestId = ++this.screenWakeLockRequestId;
        const request = wakeLock.request("screen");
        if (!request || typeof request.then !== "function") {
          this.screenWakeLockStatus = "failed";
          this.screenWakeLockError = "wakeLock.request did not return a promise";
          this.screenWakeLockType = "";
          this.applyRootScreenSaverState();
          return;
        }
        request
          .then((sentinel) => {
            if (
              requestId !== this.screenWakeLockRequestId ||
              !this.screenSaverWantsWakeLock()
            ) {
              this.releaseWakeLockSentinel(sentinel);
              return;
            }
            this.screenWakeLock = sentinel || null;
            this.screenWakeLockStatus = this.screenWakeLock ? "active" : "failed";
            this.screenWakeLockError = this.screenWakeLock
              ? ""
              : "wakeLock.request resolved without a sentinel";
            this.screenWakeLockType = this.screenWakeLock ? "screen" : "";
            if (
              this.screenWakeLock &&
              typeof this.screenWakeLock.addEventListener === "function"
            ) {
              this.screenWakeLock.addEventListener(
                "release",
                () => {
                  if (this.screenWakeLock === sentinel) {
                    this.screenWakeLock = null;
                    this.screenWakeLockStatus = "released";
                    this.screenWakeLockError = "";
                    this.applyRootScreenSaverState();
                  }
                },
                { once: true },
              );
            }
            this.applyRootScreenSaverState();
          })
          .catch((error) => {
            if (requestId !== this.screenWakeLockRequestId) {
              return;
            }
            this.screenWakeLock = null;
            this.screenWakeLockStatus = "failed";
            this.screenWakeLockError = this.errorMessage(error);
            this.screenWakeLockType = "";
            this.applyRootScreenSaverState();
          });
      } catch (error) {
        this.screenWakeLock = null;
        this.screenWakeLockStatus = "failed";
        this.screenWakeLockError = this.errorMessage(error);
        this.screenWakeLockType = "";
        this.applyRootScreenSaverState();
      }
    },

releaseScreenWakeLock(status) {
      const sentinel = this.screenWakeLock;
      this.screenWakeLockRequestId += 1;
      this.screenWakeLock = null;
      this.screenWakeLockStatus = status;
      this.screenWakeLockError = "";
      this.screenWakeLockType = "";
      this.applyRootScreenSaverState();
      this.releaseWakeLockSentinel(sentinel);
    },

releaseWakeLockSentinel(sentinel) {
      if (!sentinel || typeof sentinel.release !== "function") {
        return;
      }
      try {
        if (sentinel.released) {
          return;
        }
        const releaseResult = sentinel.release();
        if (releaseResult && typeof releaseResult.catch === "function") {
          releaseResult.catch((error) => {
            this.screenWakeLockStatus = "release-failed";
            this.screenWakeLockError = this.errorMessage(error);
            this.applyRootScreenSaverState();
          });
        }
      } catch (error) {
        this.screenWakeLockStatus = "release-failed";
        this.screenWakeLockError = this.errorMessage(error);
        this.applyRootScreenSaverState();
      }
    },

applyRootHostAccessState() {
      const root = this.rootElement;
      if (!root) {
        return;
      }
      root.setAttribute(
        "data-x11-wasm-host-access-enabled",
        this.hostAccessEnabled ? "1" : "0",
      );
      root.setAttribute(
        "data-x11-wasm-host-access-count",
        String(this.hostAccessHosts.length),
      );
      if (this.hostAccessList) {
        root.setAttribute("data-x11-wasm-host-access-list", this.hostAccessList);
      } else {
        root.removeAttribute("data-x11-wasm-host-access-list");
      }
    },

    /**
     * @param {string} hosts
     * @returns {X11WasmDOMHostAccessEntry[]}
     */
    parseHostAccessList(hosts) {
      if (!hosts) {
        return [];
      }
      return hosts
        .split(";")
        .filter(Boolean)
        .map((entry) => {
          const [family, length, address] = entry.split(":");
          return {
            family: Number.parseInt(family || "0", 10) || 0,
            length: Number.parseInt(length || "0", 10) || 0,
            address: address || "",
          };
        });
    },

    /**
     * @param {boolean} enabled
     * @param {string} hosts
     * @param {number} count
     * @returns {number}
     */
    setHostAccessControl(enabled, hosts, count) {
      const hostList = typeof hosts === "string" ? hosts : "";
      const parsedHosts = this.parseHostAccessList(hostList);
      this.hostAccessEnabled = !!enabled;
      this.hostAccessList = hostList;
      this.hostAccessHosts = parsedHosts.slice(0, positiveInt(count, 0));
      this.applyRootHostAccessState();
      return 0;
    },

applyRootRequestState() {
      const root = this.rootElement;
      if (!root) {
        return;
      }
      root.setAttribute("data-x11-wasm-request-serial", String(this.requestSerial));
      root.setAttribute("data-x11-wasm-flush-serial", String(this.flushSerial));
      root.setAttribute("data-x11-wasm-sync-serial", String(this.syncSerial));
    },

    /**
     * @param {number} requestSerial
     * @param {number} flushSerial
     * @param {number} syncSerial
     * @returns {number}
     */
    setRequestState(requestSerial, flushSerial, syncSerial) {
      this.requestSerial = unsignedInt(requestSerial, 0);
      this.flushSerial = unsignedInt(flushSerial, 0);
      this.syncSerial = unsignedInt(syncSerial, 0);
      this.applyRootRequestState();
      return 0;
    },

applyRootSynchronizeState() {
      const root = this.rootElement;
      if (!root) {
        return;
      }
      root.setAttribute(
        "data-x11-wasm-synchronize-active",
        this.synchronizeEnabled ? "1" : "0",
      );
    },

    /**
     * @param {boolean} enabled
     * @returns {number}
     */
    setSynchronizeState(enabled) {
      this.synchronizeEnabled = !!enabled;
      this.applyRootSynchronizeState();
      if (this.synchronizeEnabled && this.dirtyResources.size > 0) {
        this.flush();
      }
      return 0;
    },

applyRootServerGrabState() {
      const root = this.rootElement;
      if (!root) {
        return;
      }
      root.setAttribute(
        "data-x11-wasm-server-grab-active",
        this.serverGrabDepth > 0 ? "1" : "0",
      );
      root.setAttribute(
        "data-x11-wasm-server-grab-depth",
        String(this.serverGrabDepth),
      );
    },

    /**
     * @param {number} depth
     * @returns {number}
     */
    setServerGrabState(depth) {
      this.serverGrabDepth = positiveInt(depth, 0);
      this.applyRootServerGrabState();
      return 0;
    },

    /**
     * @param {string} text
     * @returns {number}
     */
    setClipboardText(text) {
      this.clipboardText = text;
      this.clipboardWriteStatus = "stored";
      this.clipboardWriteError = "";

      const clipboard = global.navigator && global.navigator.clipboard;
      if (!clipboard || typeof clipboard.writeText !== "function") {
        return 0;
      }

      this.clipboardWriteStatus = "pending";
      let writeResult;
      try {
        writeResult = clipboard.writeText(text);
      } catch (error) {
        this.clipboardWriteStatus = "failed";
        this.clipboardWriteError = this.errorMessage(error);
        return 0;
      }

      Promise.resolve(writeResult)
        .then(() => {
          if (this.clipboardText === text) {
            this.clipboardWriteStatus = "written";
            this.clipboardWriteError = "";
          }
        })
        .catch((error) => {
          if (this.clipboardText === text) {
            this.clipboardWriteStatus = "failed";
            this.clipboardWriteError = this.errorMessage(error);
          }
        });
      return 0;
    },

applyRootSelectionOwnerState() {
      const root = this.rootElement;
      if (!root) {
        return;
      }
      const owners = Array.from(this.selectionOwners.entries())
        .sort(([left], [right]) => left.localeCompare(right))
        .map(([selection, state]) => ({
          owner: state.owner,
          selection,
          time: state.time,
        }));
      root.setAttribute("data-x11-wasm-selection-owner-count", String(owners.length));
      root.setAttribute("data-x11-wasm-selection-owners", JSON.stringify(owners));
    },

    /**
     * @param {string} selection
     * @param {number} owner
     * @param {number} time
     * @returns {number}
     */
    setSelectionOwner(selection, owner, time) {
      const selectionName = String(selection || "");
      const ownerWindow = Math.max(0, Math.trunc(owner));
      const effectiveTime = Math.max(0, Math.trunc(time));
      if (!selectionName) {
        return 0;
      }
      if (ownerWindow) {
        this.selectionOwners.set(selectionName, {
          owner: ownerWindow,
          time: effectiveTime,
        });
      } else {
        this.selectionOwners.delete(selectionName);
      }
      this.applyRootSelectionOwnerState();

      const target = this.rootElement || this.document;
      const view = this.document && this.document.defaultView;
      if (target && view && typeof view.CustomEvent === "function") {
        target.dispatchEvent(
          new view.CustomEvent("x11wasmselectionowner", {
            bubbles: true,
            detail: {
              active: ownerWindow !== 0,
              owner: ownerWindow,
              selection: selectionName,
              time: effectiveTime,
            },
          }),
        );
      }
      return 0;
    },

    /**
     * @param {ClipboardEvent} event
     */
    handlePaste(event) {
      const data = event.clipboardData;
      let text;

      if (!data || typeof data.getData !== "function") {
        return;
      }
      text = data.getData("text/plain") || "";
      this.setExternalClipboardText(text);
      this.hostPasteShortcutActive = false;
      if (this.focusedResourceAcceptsInput()) {
        this.queuePasteShortcut(eventTime(event.timeStamp));
        event.preventDefault();
      }
    },

    /**
     * @returns {Promise<number>}
     */
    syncExternalClipboardText() {
      const clipboard = global.navigator && global.navigator.clipboard;

      if (!clipboard || typeof clipboard.readText !== "function") {
        this.clipboardReadStatus = "unavailable";
        this.clipboardReadError = "";
        return Promise.resolve(0);
      }

      this.clipboardReadStatus = "pending";
      this.clipboardReadError = "";
      const requestId = ++this.clipboardReadRequestId;
      let readResult;
      try {
        readResult = clipboard.readText();
      } catch (error) {
        if (requestId === this.clipboardReadRequestId) {
          this.clipboardReadStatus = "failed";
          this.clipboardReadError = this.errorMessage(error);
        }
        return Promise.resolve(0);
      }

      return Promise.resolve(readResult)
        .then((text) => {
          if (requestId === this.clipboardReadRequestId) {
            this.setExternalClipboardText(String(text));
            this.clipboardReadStatus = "read";
            this.clipboardReadError = "";
            this.clipboardReadCount += 1;
          }
          return 0;
        })
        .catch((error) => {
          if (requestId === this.clipboardReadRequestId) {
            this.clipboardReadStatus = "failed";
            this.clipboardReadError = this.errorMessage(error);
          }
          return 0;
        });
    },

    /**
     * @param {string} text
     * @returns {number}
     */
    setExternalClipboardText(text) {
      const normalizedText = String(text);
      const setExternal =
        this.module && this.module["_X11WasmSetExternalClipboardText"];

      this.externalClipboardText = normalizedText;
      this.externalClipboardAvailable = true;
      if (typeof setExternal !== "function") {
        return 0;
      }
      return this.withUtf8String(normalizedText, (pointer, byteLength) =>
        setExternal(pointer, byteLength),
      );
    }
  }));
})(typeof globalThis !== "undefined" ? globalThis : window);
