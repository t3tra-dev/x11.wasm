// @ts-check
/*
 * manager.js --
 *
 * Optional DOM decorations. Native resources remain the geometry and input
 * authority; frames have no XID and never enter the logical resource tree.
 */

(function attachX11WasmDOMBackendManager(global) {
  "use strict";

  const privateApi = global["X11WasmDOMPrivate"];
  const { X11WasmDOMBackend, RESOURCE_ATTR } = privateApi;
  const TITLE_HEIGHT = 28;
  const DECORATION = "data-x11-wasm-decoration";
  const MANAGED_TYPES = ["NORMAL", "DIALOG", "UTILITY"];
  const KNOWN_TYPES = [...MANAGED_TYPES, "DESKTOP", "DOCK", "TOOLBAR", "MENU",
    "SPLASH", "DROPDOWN_MENU", "POPUP_MENU", "TOOLTIP", "NOTIFICATION", "COMBO", "DND"];

  Object.assign(X11WasmDOMBackend.prototype, /** @type {any} */ ({
    /** @param {boolean} enabled */
    setWindowManagerEnabled(enabled) {
      this.windowManagerEnabled = enabled === true;
      if (!this.windowManagerEnabled) {
        this.cancelWindowManagerGesture();
        this.managerListeners?.abort();
        this.managerListeners = null;
        this.managerObserver?.disconnect();
        this.managerObserver = null;
        for (const id of this.managerFrames.keys()) {
          this.removeWindowManagerFrame(id);
        }
      } else {
        this.syncWindowManager();
      }
    },

    /** @param {X11WasmDOMResource} resource @returns {HTMLElement | null} */
    displayElement(resource) {
      return this.managerFrames.get(resource.id)?.element || resource.element;
    },

    /**
     * Native adapter boundary. A missing bridge must never invent identity.
     * @param {number} id
     * @returns {X11WasmDOMManagerIdentity | null}
     */
    windowManagerIdentity(id) {
      const query = this.module?.["_X11WasmManagerWindowIdentity"];
      if (typeof query !== "function") {
        return null;
      }
      const frameId = query(id, 0);
      const metadataId = query(id, 1);
      const protocolId = query(id, 2);
      return frameId && metadataId ? { frameId, metadataId, protocolId } : null;
    },

    /** @returns {boolean} */
    windowManagerActionsBlocked() {
      return !!(this.pointerGrabWindow || this.keyboardGrabWindow || this.serverGrabDepth);
    },

    syncWindowManager() {
      if (!this.windowManagerEnabled || !this.rootElement || this.managerSyncing) {
        return;
      }
      this.managerSyncing = true;
      try {
        this.installWindowManagerListeners();
        const eligible = new Set();
        for (const resource of this.resources.values()) {
          if (resource.kind !== "window" || resource.parent !== this.rootWindow ||
              resource.windowClass !== 1 || resource.overrideRedirect) {
            continue;
          }
          const identity = this.windowManagerIdentity(resource.id);
          const metadata = identity && this.resources.get(identity.metadataId);
          const type = metadata?.windowTypes.find((value) =>
            KNOWN_TYPES.some((known) => value === `_NET_WM_WINDOW_TYPE_${known}`));
          if (!identity || identity.frameId !== resource.id || !metadata ||
              metadata.overrideRedirect || metadata.windowClass !== 1 ||
              (type && !MANAGED_TYPES.some((known) => type === `_NET_WM_WINDOW_TYPE_${known}`))) {
            continue;
          }
          eligible.add(resource.id);
          const frame = this.managerFrames.get(resource.id) ||
            this.createWindowManagerFrame(resource, identity);
          if (frame) {
            this.setWindowManagerGeometryOwned(frame.identity, false);
            frame.identity = identity;
            this.setWindowManagerGeometryOwned(identity, true);
            this.syncWindowManagerFrame(resource, frame, metadata);
          }
        }
        for (const id of this.managerFrames.keys()) {
          if (!eligible.has(id)) {
            this.removeWindowManagerFrame(id);
          }
        }
        if (this.managerGesture && (this.windowManagerActionsBlocked() ||
            !this.managerFrames.has(this.managerGesture.id))) {
          this.cancelWindowManagerGesture();
        }
      } finally {
        this.managerSyncing = false;
      }
    },

    /** @param {X11WasmDOMManagerIdentity} identity @param {boolean} owned */
    setWindowManagerGeometryOwned(identity, owned) {
      for (const id of [identity.frameId, identity.metadataId]) {
        const resource = this.resources.get(id);
        if (resource) resource.managerGeometryOwned = owned;
      }
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {X11WasmDOMManagerIdentity} identity
     * @returns {X11WasmDOMManagerFrame | null}
     */
    createWindowManagerFrame(resource, identity) {
      const document = this.document;
      const client = resource.element;
      if (!document || !client || !client.parentElement) {
        return null;
      }
      const element = document.createElement("div");
      element.className = "x11-wasm-frame";
      element.setAttribute("data-x11-wasm-frame", String(resource.id));
      const titlebar = document.createElement("div");
      titlebar.className = "x11-wasm-titlebar";
      titlebar.setAttribute(DECORATION, "move");
      const title = document.createElement("span");
      title.className = "x11-wasm-window-title";
      const icon = document.createElement("img");
      icon.className = "x11-wasm-window-icon";
      icon.alt = "";
      icon.hidden = true;
      const close = document.createElement("button");
      close.className = "x11-wasm-close";
      close.type = "button";
      close.title = "Close";
      close.setAttribute("aria-label", "Close");
      close.setAttribute(DECORATION, "close");
      close.append(privateApi.managerIcon(document, "close"));
      titlebar.append(icon, title, close);
      client.parentElement.insertBefore(element, client);
      element.append(titlebar, client);
      const handles = ["n", "ne", "e", "se", "s", "sw", "w", "nw"].map((edge) => {
        const handle = document.createElement("div");
        handle.className = `x11-wasm-resize x11-wasm-resize-${edge}`;
        handle.setAttribute(DECORATION, `resize-${edge}`);
        handle.setAttribute("aria-hidden", "true");
        element.append(handle);
        return handle;
      });
      const frame = { identity, element, titlebar, title, icon, close, handles };
      this.managerFrames.set(resource.id, frame);
      return frame;
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {X11WasmDOMManagerFrame} frame
     * @param {X11WasmDOMResource} metadata
     */
    syncWindowManagerFrame(resource, frame, metadata) {
      const client = resource.element;
      if (!client) {
        return;
      }
      const geometry = this.windowDisplayGeometry(resource);
      const visible = this.isResourceDisplayable(resource) &&
        metadata.wmState !== "iconic" && metadata.wmState !== "withdrawn";
      const setStyle = (element, key, value) => {
        if (element.style[key] !== value) element.style[key] = value;
      };
      setStyle(frame.element, "left", `${geometry.x}px`);
      setStyle(frame.element, "top", `${geometry.y - TITLE_HEIGHT}px`);
      setStyle(frame.element, "width", `${geometry.width + resource.borderWidth * 2}px`);
      setStyle(frame.element, "height", `${geometry.height + resource.borderWidth * 2 + TITLE_HEIGHT}px`);
      setStyle(frame.element, "display", visible ? "" : "none");
      setStyle(frame.element, "zIndex", this.isEffectivelyTopmost(resource) ? "1" : "");
      setStyle(client, "left", "0px");
      setStyle(client, "top", `${TITLE_HEIGHT}px`);
      const title = metadata.title || "Untitled";
      if (frame.title.textContent !== title) frame.title.textContent = title;
      if (frame.title.title !== title) frame.title.title = title;
      const iconHref = metadata.iconPhotoHref || metadata.iconBitmapHref;
      frame.icon.hidden = !iconHref;
      if (iconHref && frame.icon.getAttribute("src") !== iconHref) frame.icon.src = iconHref;
      if (!iconHref) frame.icon.removeAttribute("src");
      frame.close.disabled = !frame.identity.protocolId ||
        !metadata.protocols.includes("WM_DELETE_WINDOW") || this.windowManagerActionsBlocked();
      const axes = this.windowManagerResizable(metadata);
      for (const handle of frame.handles) {
        const edge = handle.getAttribute(DECORATION) || "";
        handle.hidden = metadata.zoomed || metadata.fullscreen || resource.zoomed || resource.fullscreen ||
          this.windowManagerActionsBlocked() ||
          !((/[ew]/.test(edge.slice(7)) && axes.x) || (/[ns]/.test(edge.slice(7)) && axes.y));
      }
      const focused = this.resources.get(this.focusedWindow);
      const active = focused && this.stackingResourceFor(focused).id === resource.id;
      const activeText = active ? "1" : "0";
      if (frame.element.getAttribute("data-x11-wasm-active") !== activeText) {
        frame.element.setAttribute("data-x11-wasm-active", activeText);
      }
      if (!visible && this.managerGesture?.id === resource.id) {
        this.cancelWindowManagerGesture();
      }
    },

    /** @param {number} id */
    removeWindowManagerFrame(id) {
      const frame = this.managerFrames.get(id);
      if (!frame) return;
      if (this.managerGesture?.id === id) this.cancelWindowManagerGesture();
      this.setWindowManagerGeometryOwned(frame.identity, false);
      this.managerFrames.delete(id);
      const resource = this.resources.get(id);
      const client = resource?.element;
      if (client && client.parentElement === frame.element) {
        frame.element.replaceWith(client);
        const syncing = this.managerSyncing;
        this.managerSyncing = true;
        try {
          this.applyGeometry(resource);
        } finally {
          this.managerSyncing = syncing;
        }
      } else {
        frame.element.remove();
      }
    },

    installWindowManagerListeners() {
      if (this.managerListeners || !this.rootElement) return;
      const view = this.document?.defaultView;
      if (!view) return;
      const controller = new view.AbortController();
      this.managerListeners = controller;
      const options = { capture: true, signal: controller.signal };
      const consume = (event) => {
        if (event.cancelable && !(event.type === "keydown" && event.key === "Tab")) {
          event.preventDefault();
        }
        event.stopImmediatePropagation();
      };
      const decorationFor = (event) => event.target instanceof view.Element
        ? event.target.closest(`[${DECORATION}]`) : null;
      for (const type of ["pointerdown", "pointerup", "pointermove", "pointercancel",
        "lostpointercapture", "pointerover", "pointerout", "mousedown", "mouseup",
        "mousemove", "mouseover", "mouseout", "click", "dblclick", "contextmenu",
        "wheel", "keydown", "keyup", "keypress", "focusin", "focusout"]) {
        view.addEventListener(type, (event) => {
          const decoration = decorationFor(event);
          const gesture = this.managerGesture;
          const pointer = /** @type {PointerEvent} */ (event);
          const gestureEvent = gesture &&
            ["pointermove", "pointerup", "pointercancel", "lostpointercapture"].includes(type) &&
            pointer.pointerId === gesture.pointerId;
          const gesturePointer = gesture && type.startsWith("pointer");
          const gestureEscape = gesture && type === "keydown" &&
            /** @type {KeyboardEvent} */ (event).key === "Escape";
          if (!decoration && !gesturePointer && !gestureEscape) return;
          const frameElement = decoration?.closest("[data-x11-wasm-frame]");
          if (frameElement && !this.rootElement?.contains(frameElement)) return;
          consume(event);
          if (gestureEscape) {
            this.cancelWindowManagerGesture();
          } else if (type === "pointerdown" && decoration) {
            this.beginWindowManagerGesture(pointer, decoration);
          } else if (type === "pointermove" && gestureEvent) {
            this.moveWindowManagerGesture?.(pointer);
          } else if (gestureEvent && type !== "pointermove") {
            this.cancelWindowManagerGesture();
          } else if (type === "click" && decoration?.getAttribute(DECORATION) === "close") {
            this.closeWindowManagerFrame(Number(frameElement?.getAttribute("data-x11-wasm-frame")), event.timeStamp);
          } else if (type === "keydown" && decoration?.getAttribute(DECORATION) === "close") {
            const keyboard = /** @type {KeyboardEvent} */ (event);
            if (!keyboard.repeat && !keyboard.altKey && !keyboard.ctrlKey && !keyboard.metaKey &&
                (keyboard.key === "Enter" || keyboard.key === " ")) {
              this.closeWindowManagerFrame(Number(frameElement?.getAttribute("data-x11-wasm-frame")), event.timeStamp);
            }
          }
        }, { ...options, passive: false });
      }
      view.addEventListener("blur", () => this.cancelWindowManagerGesture(), options);
      this.managerObserver = new view.MutationObserver((records) => {
        if (records.some((record) => record.type === "childList" ||
            (record.target instanceof view.Element && record.target.hasAttribute(RESOURCE_ATTR)))) {
          this.syncWindowManager();
        }
      });
      this.managerObserver.observe(this.rootElement, {
        subtree: true,
        childList: true,
        attributes: true,
        attributeFilter: ["style", "data-x11-wasm-title", "data-x11-wasm-protocols",
          "data-x11-wasm-window-types", "data-x11-wasm-class", "data-x11-wasm-class-name",
          "data-x11-wasm-override-redirect", "data-x11-wasm-focused", "data-x11-wasm-wm-state",
          "data-x11-wasm-pointer-grab-active", "data-x11-wasm-keyboard-grab-active",
          "data-x11-wasm-server-grab-depth", "data-x11-wasm-icon-photo-href",
          "data-x11-wasm-icon-bitmap-href"],
      });
    },

    /** @param {number} id @param {number} time */
    closeWindowManagerFrame(id, time) {
      const frame = this.managerFrames.get(id);
      const close = this.module?.["_X11WasmManagerCloseWindow"];
      if (!frame || frame.close.disabled || this.windowManagerActionsBlocked() ||
          this.managerGesture || typeof close !== "function") return 0;
      return close(id, Math.max(0, Math.floor(time)));
    },

    /** @param {number} id @param {number} time */
    activateWindowManagerFrame(id, time) {
      const activate = this.module?.["_X11WasmManagerActivateWindow"];
      return !this.windowManagerActionsBlocked() && typeof activate === "function"
        ? activate(id, Math.max(0, Math.floor(time))) : 0;
    },

  }));
})(typeof globalThis !== "undefined" ? globalThis : window);
