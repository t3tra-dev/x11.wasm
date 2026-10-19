// @ts-check
/* State controls, saved normal geometry, and an explicit offscreen recovery path. */
(function attachX11WasmManagerStates(global) {
  "use strict";
  const { X11WasmDOMBackend, CW_X, CW_Y, CW_WIDTH, CW_HEIGHT } = global["X11WasmDOMPrivate"];
  const TITLE_HEIGHT = 28;
  const TASKBAR_HEIGHT = 32;
  const GEOMETRY_MASK = CW_X | CW_Y | CW_WIDTH | CW_HEIGHT;

  Object.assign(X11WasmDOMBackend.prototype, /** @type {any} */ ({
    /** @param {number} id @param {number} state @param {number} time */
    setWindowManagerState(id, state, time) {
      const setState = this.module?.["_X11WasmManagerSetWindowState"];
      if (this.managerGesture || this.windowManagerActionsBlocked() || typeof setState !== "function") return 0;
      const result = setState(id, state, Math.max(0, Math.floor(time)));
      if (result) this.flush();
      return result;
    },

    /** @param {number} id @param {string} action @param {number} time */
    windowManagerDecorationAction(id, action, time) {
      let result = 0;
      if (action === "close") result = this.closeWindowManagerFrame(id, time);
      else if (action === "minimize") result = this.setWindowManagerState(id, 1, time);
      else if (action === "maximize") result = this.toggleWindowManagerMaximize(id, time);
      else if (action === "restore") result = this.restoreWindowManagerFrame(id, time);
      if (["close", "minimize", "maximize", "restore"].includes(action)) {
        this.restoreFocusedInput({ suppressFocusEvent: true });
      }
      return result;
    },

    /** @param {X11WasmDOMResource} resource @param {X11WasmDOMResource} metadata
     * @returns {{ x: number, y: number, width: number, height: number } | null} */
    windowManagerMaximizeGeometry(resource, metadata) {
      const root = this.resources.get(this.rootWindow);
      if (!root) return null;
      const width = root.width - resource.borderWidth * 2;
      const height = root.height - TITLE_HEIGHT - TASKBAR_HEIGHT - resource.borderWidth * 2;
      if (width < 1 || height < 1) return null;
      const size = metadata.sizeHints ? this.constrainSizeByHints(metadata.sizeHints, width, height) : { width, height };
      if (size.width > width || size.height > height) return null;
      return { x: 0, y: TITLE_HEIGHT, width: size.width, height: size.height };
    },

    /** @param {number} id @param {number} time */
    toggleWindowManagerMaximize(id, time) {
      const resource = this.resources.get(id);
      const frame = this.managerFrames.get(id);
      const metadata = frame && this.resources.get(frame.identity.metadataId);
      if (!resource || !metadata || metadata.fullscreen || resource.fullscreen ||
          this.managerGesture || this.windowManagerActionsBlocked() ||
          typeof this.module?.["_X11WasmManagerSetWindowState"] !== "function") return 0;
      const previous = { x: resource.x, y: resource.y, width: resource.width, height: resource.height };
      const zoomed = metadata.zoomed || resource.zoomed;
      const next = zoomed ? this.managerRestoreGeometries.get(id) || previous
        : this.windowManagerMaximizeGeometry(resource, metadata);
      if (!next) return 0;
      const changed = Object.keys(previous).some((key) => previous[key] !== next[key]);
      this.managerStateChanging = id;
      try {
        if (changed && !this.configureWindowManager(id, next.x, next.y, next.width, next.height, GEOMETRY_MASK)) return 0;
        const result = this.setWindowManagerState(id, zoomed ? 0 : 2, time);
        if (!result) {
          if (changed) this.configureWindowManager(id, previous.x, previous.y, previous.width, previous.height, GEOMETRY_MASK);
          return 0;
        }
        if (zoomed) this.managerRestoreGeometries.delete(id);
        else this.managerRestoreGeometries.set(id, previous);
        return result;
      } finally {
        this.managerStateChanging = 0;
      }
    },

    /** @param {X11WasmDOMResource} resource @param {X11WasmDOMManagerFrame} frame
     * @param {X11WasmDOMResource} metadata */
    syncWindowManagerStateGeometry(resource, frame, metadata) {
      const zoomed = metadata.zoomed || resource.zoomed;
      const root = this.resources.get(this.rootWindow);
      const bounds = `${root.width},${root.height}`;
      const previousZoomed = frame.lastZoomed;
      const fullscreen = metadata.fullscreen || resource.fullscreen;
      const resized = frame.lastBounds !== bounds || (frame.lastFullscreen && !fullscreen);
      const geometry = [resource.x, resource.y, resource.width, resource.height].join(",");
      if (zoomed && previousZoomed === true && frame.lastGeometry && frame.lastGeometry !== geometry &&
          this.managerConfiguring !== resource.id && this.managerStateChanging !== resource.id && !fullscreen) {
        this.managerRestoreGeometries.set(resource.id,
          { x: resource.x, y: resource.y, width: resource.width, height: resource.height });
      }
      frame.lastGeometry = geometry;
      if (metadata.wmState === "iconic") return;
      frame.lastZoomed = zoomed;
      frame.lastBounds = bounds;
      frame.lastFullscreen = fullscreen;
      if (this.managerStateChanging === resource.id || fullscreen) return;
      let next;
      if (zoomed && (previousZoomed !== true || resized)) {
        if (previousZoomed === false && !this.managerRestoreGeometries.has(resource.id)) {
          this.managerRestoreGeometries.set(resource.id,
            { x: resource.x, y: resource.y, width: resource.width, height: resource.height });
        }
        next = this.windowManagerMaximizeGeometry(resource, metadata);
      } else if (!zoomed && previousZoomed === true) {
        next = this.managerRestoreGeometries.get(resource.id);
      }
      if (!next) return;
      const changed = resource.x !== next.x || resource.y !== next.y ||
        resource.width !== next.width || resource.height !== next.height;
      if (!changed || this.configureWindowManager(resource.id, next.x, next.y, next.width, next.height, GEOMETRY_MASK)) {
        frame.lastGeometry = [resource.x, resource.y, resource.width, resource.height].join(",");
        if (!zoomed) this.managerRestoreGeometries.delete(resource.id);
      }
    },

    /** @param {number} id @param {number} time */
    restoreWindowManagerFrame(id, time) {
      const resource = this.resources.get(id);
      const frame = this.managerFrames.get(id);
      const metadata = frame && this.resources.get(frame.identity.metadataId);
      if (!resource || !metadata || this.managerGesture || this.windowManagerActionsBlocked()) return 0;
      if (metadata.wmState === "iconic" &&
          !this.setWindowManagerState(id, metadata.zoomed || resource.zoomed ? 2 : 0, time)) return 0;
      if (!(metadata.zoomed || resource.zoomed || metadata.fullscreen || resource.fullscreen)) {
        const root = this.resources.get(this.rootWindow);
        const outerWidth = resource.width + resource.borderWidth * 2;
        const x = outerWidth > root.width ? root.width - outerWidth
          : Math.max(0, Math.min(resource.x, root.width - outerWidth));
        const y = Math.max(TITLE_HEIGHT,
          Math.min(resource.y, Math.max(TITLE_HEIGHT, root.height - TASKBAR_HEIGHT)));
        if ((x !== resource.x || y !== resource.y) &&
            !this.configureWindowManager(id, x, y, resource.width, resource.height, CW_X | CW_Y)) return 0;
      }
      return this.activateWindowManagerFrame(id, time);
    },

    syncWindowManagerTaskbar() {
      if (!this.rootElement || !this.document) return;
      const active = new Set();
      for (const [id, frame] of this.managerFrames) {
        const resource = this.resources.get(id);
        const metadata = this.resources.get(frame.identity.metadataId);
        if (!resource || !metadata || metadata.wmState === "withdrawn" ||
            (!resource.mapped && metadata.wmState !== "iconic")) continue;
        active.add(id);
        if (!this.managerTaskbar) {
          this.managerTaskbar = this.document.createElement("div");
          this.managerTaskbar.className = "x11-wasm-taskbar";
          this.managerTaskbar.setAttribute("data-x11-wasm-taskbar", "");
          this.managerTaskbar.setAttribute("data-x11-wasm-decoration", "taskbar");
          this.managerTaskbar.setAttribute("role", "toolbar");
          this.managerTaskbar.setAttribute("aria-label", "Windows");
          this.rootElement.append(this.managerTaskbar);
        }
        let button = this.managerTasks.get(id);
        if (!button) {
          button = this.document.createElement("button");
          button.type = "button";
          button.className = "x11-wasm-task";
          button.setAttribute("data-x11-wasm-task", String(id));
          button.setAttribute("data-x11-wasm-decoration", "restore");
          this.managerTaskbar.append(button);
          this.managerTasks.set(id, button);
        }
        const title = metadata.title || "Untitled";
        if (button.textContent !== title) button.textContent = title;
        const iconic = metadata.wmState === "iconic";
        const label = `${iconic ? "Restore" : "Show"} ${title}`;
        if (button.title !== label) button.title = label;
        if (button.getAttribute("aria-label") !== label) button.setAttribute("aria-label", label);
        if (button.getAttribute("aria-pressed") !== String(!iconic &&
            frame.element.getAttribute("data-x11-wasm-active") === "1")) {
          button.setAttribute("aria-pressed", String(!iconic && frame.element.getAttribute("data-x11-wasm-active") === "1"));
        }
        button.disabled = this.windowManagerActionsBlocked() ||
          (iconic && typeof this.module?.["_X11WasmManagerSetWindowState"] !== "function");
      }
      for (const [id, button] of this.managerTasks) {
        if (!active.has(id)) { button.remove(); this.managerTasks.delete(id); }
      }
      if (!active.size) this.removeWindowManagerTaskbar();
    },

    removeWindowManagerTaskbar() {
      this.managerTaskbar?.remove();
      this.managerTaskbar = null;
      this.managerTasks.clear();
    },
  }));
})(typeof globalThis !== "undefined" ? globalThis : window);
