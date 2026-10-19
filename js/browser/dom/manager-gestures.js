// @ts-check
/* Native-authoritative move/resize gestures for the optional DOM manager. */
(function attachX11WasmManagerGestures(global) {
  "use strict";
  const { X11WasmDOMBackend, CW_X, CW_Y, CW_WIDTH, CW_HEIGHT, P_MIN_SIZE,
    P_MAX_SIZE } = global["X11WasmDOMPrivate"];
  const DECORATION = "data-x11-wasm-decoration";

  Object.assign(X11WasmDOMBackend.prototype, /** @type {any} */ ({
    /** @param {number} id @param {number} x @param {number} y
     * @param {number} width @param {number} height @param {number} mask */
    configureWindowManager(id, x, y, width, height, mask) {
      const configure = this.module?.["_X11WasmManagerConfigureWindow"];
      if (this.windowManagerActionsBlocked() || typeof configure !== "function") return 0;
      const previous = this.managerConfiguring;
      this.managerConfiguring = id;
      try {
        const result = configure(id, x, y, width, height, mask);
        if (result) this.flush();
        return result;
      } finally {
        this.managerConfiguring = previous;
      }
    },

    /** @param {X11WasmDOMResource} metadata @returns {{ x: boolean, y: boolean }} */
    windowManagerResizable(metadata) {
      const hints = metadata.sizeHints;
      const fixed = hints && (hints.flags & P_MIN_SIZE) && (hints.flags & P_MAX_SIZE);
      return { x: !(fixed && hints.minWidth === hints.maxWidth),
        y: !(fixed && hints.minHeight === hints.maxHeight) };
    },

    /** @param {X11WasmDOMResource} metadata @param {number} width @param {number} height
     * @param {number=} widthLimit @param {number=} heightLimit
     * @returns {{ width: number, height: number } | null} */
    constrainWindowManagerSize(metadata, width, height, widthLimit = 65535, heightLimit = 65535) {
      if (widthLimit < 1 || heightLimit < 1) return null;
      const hints = metadata.sizeHints;
      const flags = hints?.flags || 0;
      const maxWidth = Math.min(65535, Math.floor(widthLimit),
        hints && (flags & P_MAX_SIZE) && hints.maxWidth > 0 ? hints.maxWidth : 65535);
      const maxHeight = Math.min(65535, Math.floor(heightLimit),
        hints && (flags & P_MAX_SIZE) && hints.maxHeight > 0 ? hints.maxHeight : 65535);
      const size = this.constrainSizeByHints({ ...hints, flags: flags | P_MAX_SIZE,
        maxWidth, maxHeight }, width, height);
      return size.width > maxWidth || size.height > maxHeight ? null : size;
    },

    /** @param {PointerEvent} event @param {Element} decoration */
    beginWindowManagerGesture(event, decoration) {
      const action = decoration.getAttribute(DECORATION) || "";
      const edge = action === "move" ? "move" : action.replace(/^resize-/, "");
      if (this.managerGesture || this.windowManagerActionsBlocked() ||
          event.button !== 0 || !event.isPrimary ||
          !["move", "n", "ne", "e", "se", "s", "sw", "w", "nw"].includes(edge) ||
          typeof this.module?.["_X11WasmManagerConfigureWindow"] !== "function") return;
      const id = Number(decoration.closest("[data-x11-wasm-frame]")?.getAttribute("data-x11-wasm-frame"));
      const resource = this.resources.get(id);
      const frame = this.managerFrames.get(id);
      const metadata = frame && this.resources.get(frame.identity.metadataId);
      if (!resource || !frame || !metadata || !this.isResourceDisplayable(resource) ||
          metadata.zoomed || metadata.fullscreen || resource.zoomed || resource.fullscreen) return;
      const axes = this.windowManagerResizable(metadata);
      if (edge !== "move" && !((/[ew]/.test(edge) && axes.x) || (/[ns]/.test(edge) && axes.y))) return;
      this.activateWindowManagerFrame(id, event.timeStamp);
      const position = this.windowManagerPointerPosition(event);
      const capture = /** @type {HTMLElement} */ (decoration);
      this.managerGesture = { id, pointerId: event.pointerId, capture, edge,
        startX: position.x, startY: position.y,
        x: resource.x, y: resource.y, width: resource.width, height: resource.height };
      try {
        capture.setPointerCapture(event.pointerId);
      } catch (error) {
        // Synthetic pointers cannot capture; window listeners still isolate input.
      }
    },

    /** @param {PointerEvent} event @returns {{ x: number, y: number }} */
    windowManagerPointerPosition(event) {
      const element = this.rootElement;
      const rect = element.getBoundingClientRect();
      const root = this.resources.get(this.rootWindow);
      const scaleX = rect.width / (element.offsetWidth || root.width) || 1;
      const scaleY = rect.height / (element.offsetHeight || root.height) || 1;
      return {
        x: ((event.clientX - rect.left) / scaleX - element.clientLeft + element.scrollLeft) *
          root.width / (element.clientWidth || root.width),
        y: ((event.clientY - rect.top) / scaleY - element.clientTop + element.scrollTop) *
          root.height / (element.clientHeight || root.height),
      };
    },

    /** @param {PointerEvent} event */
    moveWindowManagerGesture(event) {
      const gesture = this.managerGesture;
      if (!gesture || event.pointerId !== gesture.pointerId) return;
      const frame = this.managerFrames.get(gesture.id);
      const resource = this.resources.get(gesture.id);
      const metadata = frame && this.resources.get(frame.identity.metadataId);
      if (!metadata || !resource || this.windowManagerActionsBlocked() ||
          metadata.zoomed || metadata.fullscreen || resource.zoomed || resource.fullscreen ||
          (event.pointerType === "mouse" && event.buttons === 0)) {
        this.cancelWindowManagerGesture();
        return;
      }
      const position = this.windowManagerPointerPosition(event);
      const dx = Math.round(position.x - gesture.startX);
      const dy = Math.round(position.y - gesture.startY);
      let { x, y, width, height } = gesture;
      let mask = CW_X | CW_Y;
      if (gesture.edge === "move") {
        x += dx;
        y += dy;
      } else {
        const axes = this.windowManagerResizable(metadata);
        const west = axes.x && gesture.edge.includes("w");
        const east = axes.x && gesture.edge.includes("e");
        const north = axes.y && gesture.edge.includes("n");
        const south = axes.y && gesture.edge.includes("s");
        if (west || east) width = Math.max(1, width + (west ? -dx : dx));
        if (north || south) height = Math.max(1, height + (north ? -dy : dy));
        const constrained = this.constrainWindowManagerSize(metadata, width, height);
        if (!constrained) { this.cancelWindowManagerGesture(); return; }
        width = constrained.width;
        height = constrained.height;
        if (west) x += gesture.width - width;
        if (north) y += gesture.height - height;
        mask = (west ? CW_X : 0) | (north ? CW_Y : 0) |
          (width !== gesture.width ? CW_WIDTH : 0) | (height !== gesture.height ? CW_HEIGHT : 0);
      }
      if (!mask || (resource.x === x && resource.y === y &&
          resource.width === width && resource.height === height)) return;
      const candidate = [x, y, width, height, mask].join(",");
      if (gesture.lastCandidate === candidate) return;
      gesture.lastCandidate = candidate;
      if (!this.configureWindowManager(gesture.id, x, y, width, height, mask)) {
        this.cancelWindowManagerGesture();
      }
    },

    /** @param {number} id */
    cancelWindowManagerResource(id) {
      const gesture = this.managerGesture;
      const resource = this.resources.get(id);
      if (resource && resource.kind !== "window") return;
      if (gesture && (gesture.id === id || (resource &&
          this.stackingResourceFor(resource).id === gesture.id))) this.cancelWindowManagerGesture();
    },

    cancelWindowManagerGesture() {
      const gesture = this.managerGesture;
      this.managerGesture = null;
      if (!gesture) return;
      try {
        if (gesture.capture.hasPointerCapture(gesture.pointerId)) {
          gesture.capture.releasePointerCapture(gesture.pointerId);
        }
      } catch (error) {
        // Capture may already be released by removal or browser cancellation.
      }
    },
  }));
})(typeof globalThis !== "undefined" ? globalThis : window);
