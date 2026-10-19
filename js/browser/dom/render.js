// @ts-check
/*
 * render.js --
 *
 *	Flush, damage, clipping, canvas, geometry, stacking, style, and low-level helper methods.
 */

(function attachX11WasmDOMBackendRender(global) {
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
     * @returns {number}
     */
    flush() {
      this.flushScheduled = false;
      for (const resource of Array.from(this.dirtyResources)) {
        this.flushResource(resource);
      }
      this.dirtyResources.clear();
      return 0;
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {X11WasmDOMResource} resource
     * @returns {CanvasPattern | null}
     */
    createBackgroundPattern(context, resource) {
      if (resource.backgroundPixmap === 0) {
        return null;
      }
      const pixmap = this.resources.get(resource.backgroundPixmap);
      if (
        !pixmap ||
        pixmap.kind !== "pixmap" ||
        !pixmap.canvas ||
        pixmap.width <= 0 ||
        pixmap.height <= 0
      ) {
        return null;
      }
      if (pixmap.geometryDirty) {
        this.flushResourceGeometry(pixmap);
      }
      return context.createPattern(pixmap.canvas, "repeat");
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {X11WasmDOMResource} resource
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @returns {boolean}
     */
    fillResourceBackground(context, resource, x, y, width, height) {
      const pattern = this.createBackgroundPattern(context, resource);
      if (pattern) {
        context.fillStyle = pattern;
      } else if (resource.backgroundColor) {
        context.fillStyle = resource.backgroundColor;
      } else {
        return false;
      }
      context.fillRect(x, y, width, height);
      return true;
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {X11WasmDOMResource} resource
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     */
    clearResourceArea(context, resource, x, y, width, height) {
      const clearX = Math.floor(x);
      const clearY = Math.floor(y);
      const clearWidth = Math.floor(width);
      const clearHeight = Math.floor(height);

      if (clearWidth <= 0 || clearHeight <= 0 ||
          !this.fillResourceBackground(context, resource,
            clearX, clearY, clearWidth, clearHeight)) {
        return;
      }
      this.markDirty(resource, clearX, clearY, clearWidth, clearHeight);
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    markGeometryDirty(resource) {
      resource.geometryDirty = true;
      this.dirtyResources.add(resource);
      this.scheduleFlush();
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     */
    markDirty(resource, x, y, width, height) {
      let damageX = Math.floor(x);
      let damageY = Math.floor(y);
      let damageWidth = Math.floor(width);
      let damageHeight = Math.floor(height);

      if (damageWidth <= 0 || damageHeight <= 0) {
        return;
      }
      if (damageX < 0) {
        damageWidth += damageX;
        damageX = 0;
      }
      if (damageY < 0) {
        damageHeight += damageY;
        damageY = 0;
      }
      if (
        damageX >= resource.pendingWidth ||
        damageY >= resource.pendingHeight
      ) {
        return;
      }
      damageWidth = Math.min(damageWidth, resource.pendingWidth - damageX);
      damageHeight = Math.min(damageHeight, resource.pendingHeight - damageY);
      if (damageWidth <= 0 || damageHeight <= 0) {
        return;
      }
      if (resource.damage) {
        const left = Math.min(resource.damage.x, damageX);
        const top = Math.min(resource.damage.y, damageY);
        const right = Math.max(
          resource.damage.x + resource.damage.width,
          damageX + damageWidth,
        );
        const bottom = Math.max(
          resource.damage.y + resource.damage.height,
          damageY + damageHeight,
        );
        resource.damage = {
          x: left,
          y: top,
          width: right - left,
          height: bottom - top,
        };
      } else {
        resource.damage = {
          x: damageX,
          y: damageY,
          width: damageWidth,
          height: damageHeight,
        };
      }
      resource.dirty = true;
      resource.damageSerial = this.damageSerial++;
      this.dirtyResources.add(resource);
      this.scheduleFlush();
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {() => void} draw
     */
    withCurrentClip(context, draw) {
      const clip = this.currentClip;
      const mask = this.currentClipMask;

      if (!clip && !mask) {
        draw();
        return;
      }
      context.save();
      if (clip) {
        context.beginPath();
        for (const rect of clip) {
          context.rect(rect.x, rect.y, rect.width, rect.height);
        }
        context.clip();
      }
      if (mask && !this.applyClipMask(context, mask)) {
        context.restore();
        return;
      }
      draw();
      context.restore();
    },

    /**
     * @param {X11WasmDOMClipRect[] | null} clip
     * @returns {X11WasmDOMClipRect | null}
     */
    clipBounds(clip) {
      if (!clip) {
        return null;
      }
      let left = Infinity;
      let top = Infinity;
      let right = -Infinity;
      let bottom = -Infinity;

      for (const rect of clip) {
        if (rect.width <= 0 || rect.height <= 0) {
          continue;
        }
        left = Math.min(left, rect.x);
        top = Math.min(top, rect.y);
        right = Math.max(right, rect.x + rect.width);
        bottom = Math.max(bottom, rect.y + rect.height);
      }
      if (right <= left || bottom <= top) {
        return null;
      }
      return {
        x: left,
        y: top,
        width: right - left,
        height: bottom - top,
      };
    },

    /**
     * @param {X11WasmDOMClipRect[] | null} clip
     * @param {number} x
     * @param {number} y
     * @returns {boolean}
     */
    clipContains(clip, x, y) {
      if (!clip) {
        return true;
      }
      for (const rect of clip) {
        if (
          x >= rect.x &&
          y >= rect.y &&
          x < rect.x + rect.width &&
          y < rect.y + rect.height
        ) {
          return true;
        }
      }
      return false;
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {X11WasmDOMClipMask} mask
     * @returns {boolean}
     */
    applyClipMask(context, mask) {
      const snapshot = this.clipMaskSnapshot(mask);
      if (!snapshot || !snapshot.hasBits) {
        return false;
      }
      const { width, height, xOrigin, yOrigin } = snapshot;

      context.beginPath();
      for (let row = 0; row < height; row++) {
        let runStart = -1;
        for (let col = 0; col <= width; col++) {
          const bit =
            col < width &&
            this.clipMaskContains(snapshot, xOrigin + col, yOrigin + row);

          if (bit && runStart < 0) {
            runStart = col;
          } else if (!bit && runStart >= 0) {
            context.rect(xOrigin + runStart, yOrigin + row, col - runStart, 1);
            runStart = -1;
          }
        }
      }
      context.clip();
      return true;
    },

    /**
     * @param {X11WasmDOMClipMask | null} mask
     * @returns {X11WasmDOMClipMaskSnapshot | null}
     */
    clipMaskSnapshot(mask) {
      if (!mask) {
        return null;
      }
      const resource = this.resources.get(mask.pixmap);
      const sourceContext = resource && resource.context;
      if (
        !resource ||
        !sourceContext ||
        resource.width <= 0 ||
        resource.height <= 0
      ) {
        return null;
      }
      if (resource.geometryDirty) {
        this.flushResourceGeometry(resource);
      }
      const width = resource.width;
      const height = resource.height;
      const data = sourceContext.getImageData(0, 0, width, height).data;
      let hasBits = false;

      for (let offset = 0; offset < data.length; offset += 4) {
        if (rgbaPixelHasBit(data, offset)) {
          hasBits = true;
          break;
        }
      }
      return {
        data,
        width,
        height,
        xOrigin: mask.xOrigin,
        yOrigin: mask.yOrigin,
        hasBits,
      };
    },

    /**
     * @param {X11WasmDOMClipMaskSnapshot | null} snapshot
     * @param {number} x
     * @param {number} y
     * @returns {boolean}
     */
    clipMaskContains(snapshot, x, y) {
      if (!snapshot) {
        return true;
      }
      const col = x - snapshot.xOrigin;
      const row = y - snapshot.yOrigin;
      if (
        col < 0 ||
        row < 0 ||
        col >= snapshot.width ||
        row >= snapshot.height
      ) {
        return false;
      }
      const offset = (row * snapshot.width + col) * 4;
      return rgbaPixelHasBit(snapshot.data, offset);
    },

    /**
     * @param {CanvasPattern} pattern
     */
    applyPatternOrigin(pattern) {
      const view = this.document && this.document.defaultView;
      if (
        !view ||
        typeof pattern.setTransform !== "function" ||
        typeof view.DOMMatrix !== "function"
      ) {
        return;
      }
      const matrix = new view.DOMMatrix();
      matrix.e = this.currentDrawStyle.tsOriginX;
      matrix.f = this.currentDrawStyle.tsOriginY;
      pattern.setTransform(matrix);
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {number} resourceId
     * @returns {CanvasPattern | null}
     */
    createTilePattern(context, resourceId) {
      const resource = this.resources.get(resourceId);
      if (!resource || !resource.canvas || resource.width <= 0 || resource.height <= 0) {
        return null;
      }
      if (resource.geometryDirty) {
        this.flushResourceGeometry(resource);
      }
      const pattern = context.createPattern(resource.canvas, "repeat");
      if (pattern) {
        this.applyPatternOrigin(pattern);
      }
      return pattern;
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {number} resourceId
     * @param {number} foregroundPixel
     * @param {number} backgroundPixel
     * @param {boolean} opaque
     * @returns {CanvasPattern | null}
     */
    createStipplePattern(
      context,
      resourceId,
      foregroundPixel,
      backgroundPixel,
      opaque,
    ) {
      const resource = this.resources.get(resourceId);
      const sourceContext = resource && resource.context;
      if (
        !this.document ||
        !resource ||
        !sourceContext ||
        resource.width <= 0 ||
        resource.height <= 0
      ) {
        return null;
      }
      if (resource.geometryDirty) {
        this.flushResourceGeometry(resource);
      }

      const patternCanvas = this.document.createElement("canvas");
      patternCanvas.width = resource.width;
      patternCanvas.height = resource.height;
      const patternContext = patternCanvas.getContext("2d");
      if (!patternContext) {
        return null;
      }

      const sourceData = sourceContext.getImageData(
        0,
        0,
        resource.width,
        resource.height,
      ).data;
      const imageData = patternContext.createImageData(
        resource.width,
        resource.height,
      );
      const foreground = pixelToRgb(foregroundPixel);
      const background = pixelToRgb(backgroundPixel);

      for (let offset = 0; offset < sourceData.length; offset += 4) {
        const bit = rgbaPixelHasBit(sourceData, offset);
        const rgb = bit ? foreground : background;
        if (bit || opaque) {
          imageData.data[offset] = rgb[0];
          imageData.data[offset + 1] = rgb[1];
          imageData.data[offset + 2] = rgb[2];
          imageData.data[offset + 3] = 255;
        }
      }
      patternContext.putImageData(imageData, 0, 0);

      const pattern = context.createPattern(patternCanvas, "repeat");
      if (pattern) {
        this.applyPatternOrigin(pattern);
      }
      return pattern;
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {number} pixel
     * @returns {string | CanvasPattern}
     */
    paintStyleForPixel(context, pixel) {
      const style = this.currentDrawStyle;

      if (style.fillStyle === FILL_TILED) {
        const pattern = this.createTilePattern(context, style.tile);
        if (pattern) {
          return pattern;
        }
      } else if (
        style.fillStyle === FILL_STIPPLED ||
        style.fillStyle === FILL_OPAQUE_STIPPLED
      ) {
        const pattern = this.createStipplePattern(
          context,
          style.stipple,
          pixel,
          style.backgroundPixel,
          style.fillStyle === FILL_OPAQUE_STIPPLED,
        );
        if (pattern) {
          return pattern;
        }
      }
      return pixelToCss(pixel);
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {() => void} draw
     * @param {(() => void)=} drawBackground
     */
    withCurrentDrawStyle(context, draw, drawBackground) {
      const style = this.currentDrawStyle;

      if (!drawFunctionChangesDestination(style.functionId)) {
        return;
      }
      context.save();
      context.globalCompositeOperation = compositeOperationForFunction(
        style.functionId,
      );
      this.prepareCurrentDrawContext(context);
      if (style.lineStyle === LINE_DOUBLE_DASH && drawBackground) {
        context.strokeStyle = pixelToCss(style.backgroundPixel);
        context.setLineDash([]);
        context.lineDashOffset = 0;
        drawBackground();
        context.lineDashOffset = style.dashOffset;
        context.setLineDash(this.currentLineDashPattern());
      }
      draw();
      context.restore();
    },

scheduleFlush() {
      if (this.synchronizeEnabled) {
        this.flush();
        return;
      }
      if (this.flushScheduled) {
        return;
      }
      this.flushScheduled = true;
      const window = this.document && this.document.defaultView;
      const callback = () => {
        this.flush();
      };

      if (window && typeof window.requestAnimationFrame === "function") {
        window.requestAnimationFrame(callback);
      } else if (typeof global.setTimeout === "function") {
        global.setTimeout(callback, 0);
      }
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    flushResource(resource) {
      if (resource.geometryDirty) {
        this.flushResourceGeometry(resource);
      }
      if (resource.kind === "pixmap") {
        this.syncPixmapDependents(resource);
      }
      resource.dirty = false;
      resource.damage = null;
    },

    /**
     * @param {X11WasmDOMResource} pixmap
     */
    syncPixmapDependents(pixmap) {
      for (const resource of this.resources.values()) {
        if (resource.kind !== "window" && resource.kind !== "root") {
          continue;
        }
        if (resource.borderPixmap === pixmap.id) {
          this.applyStyle(resource);
        }
        const hints = resource.wmHints;
        if (
          hints &&
          (((hints.flags & WM_ICON_PIXMAP_HINT) &&
            hints.iconPixmap === pixmap.id) ||
            ((hints.flags & WM_ICON_MASK_HINT) &&
              hints.iconMask === pixmap.id))
        ) {
          this.setWindowIconBitmap(
            resource,
            hints.flags & WM_ICON_PIXMAP_HINT ? hints.iconPixmap : 0,
            hints.flags & WM_ICON_MASK_HINT ? hints.iconMask : 0,
          );
          this.syncDocumentIcon(resource);
        }
      }
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    flushResourceGeometry(resource) {
      resource.x = resource.pendingX;
      resource.y = resource.pendingY;
      resource.width = resource.pendingWidth;
      resource.height = resource.pendingHeight;
      resource.borderWidth = resource.pendingBorderWidth;
      resource.mapped = resource.pendingMapped;
      resource.geometryDirty = false;
      this.applyGeometry(resource);
      this.syncIconWindowPresentationForOwner(resource);
    },

    /**
     * @param {number} width
     * @param {number} height
     * @returns {HTMLCanvasElement | OffscreenCanvas}
     */
    createCanvas(width, height) {
      if (typeof global.OffscreenCanvas === "function") {
        return new global.OffscreenCanvas(width, height);
      }
      if (!this.document) {
        throw new Error("X11WasmDOMBackend requires a document");
      }
      const canvas = this.document.createElement("canvas");
      canvas.width = width;
      canvas.height = height;
      return canvas;
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applyGeometry(resource) {
      if (!resource.element) {
        return;
      }
      if (resource.nativeGeometry && !resource.syncingNativeGeometry) {
        if (resource.geometryDirty) {
          this.flushResourceGeometry(resource);
          return;
        }
        this.syncNativeWindowStateGeometry(resource);
      }
      const iconProxyOwner = this.activeIconProxyOwner(resource);
      const geometry = this.windowDisplayGeometry(resource, iconProxyOwner);
      const displayable = this.isResourceDisplayable(resource, iconProxyOwner);
      resource.element.style.left = `${geometry.x}px`;
      resource.element.style.top = `${geometry.y}px`;
      resource.element.style.width = `${geometry.width}px`;
      resource.element.style.height = `${geometry.height}px`;
      resource.element.style.display = displayable ? "" : "none";
      resource.element.setAttribute(
        "data-x11-wasm-viewable",
        displayable ? "1" : "0",
      );
      if (displayable) {
        resource.element.removeAttribute("aria-hidden");
      } else {
        resource.element.setAttribute("aria-hidden", "true");
      }
      this.applyIconProxyState(resource, iconProxyOwner);
      resource.element.style.zIndex = this.isEffectivelyTopmost(resource)
        ? "1"
        : "";
      this.applyStyle(resource);
      if (resource.canvas) {
        this.resizeCanvasForBitGravity(resource);
      }
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {X11WasmDOMResource | null=} iconProxyOwner
     * @returns {boolean}
     */
    isResourceDisplayable(resource, iconProxyOwner = null) {
      if (iconProxyOwner) {
        return true;
      }
      if (!resource.mapped) {
        return false;
      }
      return resource.wmState !== "iconic" && resource.wmState !== "withdrawn";
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {X11WasmDOMResource | null=} iconProxyOwner
     * @returns {{ x: number, y: number, width: number, height: number }}
     */
    windowDisplayGeometry(resource, iconProxyOwner = null) {
      const hints = iconProxyOwner?.wmHints;
      if (hints) {
        return {
          x: hints.flags & WM_ICON_POSITION_HINT ? hints.iconX : resource.x,
          y: hints.flags & WM_ICON_POSITION_HINT ? hints.iconY : resource.y,
          width: resource.width,
          height: resource.height,
        };
      }
      if (resource.nativeGeometry || (!resource.zoomed && !resource.fullscreen)) {
        return {
          x: resource.x,
          y: resource.y,
          width: resource.width,
          height: resource.height,
        };
      }
      if (resource.fullscreen) {
        return this.windowStateFillGeometry(resource);
      }
      return this.windowZoomGeometry(resource);
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    syncNativeWindowStateGeometry(resource) {
      if (resource.managerGeometryOwned) return;
      const configure = this.module && this.module["_X11WasmConfigureWindow"];
      const expanded = resource.zoomed || resource.fullscreen;
      if (typeof configure !== "function" || (!expanded && !resource.normalGeometry)) {
        return;
      }
      const target = this.nativeWindowGeometryResource(resource);
      if (target !== resource && target.geometryDirty) {
        this.flushResourceGeometry(target);
      }
      if (expanded && !resource.normalGeometry) {
        resource.normalGeometry = {
          x: target.x,
          y: target.y,
          width: target.width,
          height: target.height,
        };
      }
      const geometry = expanded
        ? (resource.fullscreen
          ? this.windowStateFillGeometry(resource)
          : this.windowZoomGeometry(resource))
        : resource.normalGeometry;
      if (!geometry) {
        return;
      }
      if (geometry.x !== target.x || geometry.y !== target.y ||
          geometry.width !== target.width || geometry.height !== target.height) {
        // Re-enter the normal C configure path before changing CSS or canvas size.
        resource.syncingNativeGeometry = true;
        try {
          if (configure(target.id, geometry.x, geometry.y,
              geometry.width, geometry.height, 15)) {
            this.flushResourceGeometry(target);
          }
        } finally {
          resource.syncingNativeGeometry = false;
        }
      }
      if (!expanded) {
        resource.normalGeometry = undefined;
      }
    },

    /** @param {X11WasmDOMResource} resource @returns {X11WasmDOMResource} */
    nativeWindowGeometryResource(resource) {
      let target = resource;
      while (target.parent && target.parent !== this.rootWindow) {
        const parent = this.resources.get(target.parent);
        if (!parent || parent.kind !== "window" || !parent.nativeGeometry) break;
        target = parent;
      }
      return target;
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @returns {{ x: number, y: number, width: number, height: number }}
     */
    windowStateFillGeometry(resource) {
      const bounds = this.windowStateBounds(resource);
      const border = Math.max(0, Math.floor(resource.borderWidth));

      return {
        x: 0,
        y: 0,
        width: Math.max(1, bounds.width - border * 2),
        height: Math.max(1, bounds.height - border * 2),
      };
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @returns {{ x: number, y: number, width: number, height: number }}
     */
    windowZoomGeometry(resource) {
      const fill = this.windowStateFillGeometry(resource);
      const hints = resource.zoomHints;
      if (!hints) {
        return fill;
      }
      const hintLength = (enabled, value, fallback) =>
        enabled && Number.isFinite(value) && value > 0
          ? Math.floor(value)
          : fallback;
      const hintPosition = (enabled, value, fallback) =>
        enabled && Number.isFinite(value) ? Math.floor(value) : fallback;

      const x = hintPosition(hints.flags & P_POSITION, hints.x, fill.x);
      const y = hintPosition(hints.flags & P_POSITION, hints.y, fill.y);
      const constrainedSize = this.constrainSizeByHints(
        hints,
        hintLength(hints.flags & P_SIZE, hints.width, fill.width),
        hintLength(hints.flags & P_SIZE, hints.height, fill.height),
      );

      return {
        x,
        y,
        width: constrainedSize.width,
        height: constrainedSize.height,
      };
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {X11WasmDOMResource | null} iconProxyOwner
     */
    applyIconProxyState(resource, iconProxyOwner) {
      const element = resource.element;
      if (!element) {
        return;
      }
      if (iconProxyOwner) {
        const hints = iconProxyOwner.wmHints;
        element.setAttribute(
          "data-x11-wasm-icon-proxy-owner",
          String(iconProxyOwner.id),
        );
        element.setAttribute("data-x11-wasm-icon-proxy-active", "1");
        if (hints && hints.flags & WM_ICON_POSITION_HINT) {
          element.setAttribute("data-x11-wasm-icon-proxy-x", String(hints.iconX));
          element.setAttribute("data-x11-wasm-icon-proxy-y", String(hints.iconY));
        } else {
          element.removeAttribute("data-x11-wasm-icon-proxy-x");
          element.removeAttribute("data-x11-wasm-icon-proxy-y");
        }
      } else {
        element.removeAttribute("data-x11-wasm-icon-proxy-owner");
        element.removeAttribute("data-x11-wasm-icon-proxy-active");
        element.removeAttribute("data-x11-wasm-icon-proxy-x");
        element.removeAttribute("data-x11-wasm-icon-proxy-y");
      }
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @returns {{ width: number, height: number }}
     */
    windowStateBounds(resource) {
      const view = this.document && this.document.defaultView;
      const target = resource.nativeGeometry
        ? this.nativeWindowGeometryResource(resource) : resource;
      const active = this.document && this.document.fullscreenElement;
      if (resource.nativeGeometry && resource.fullscreen && view && active &&
          (active === resource.element || active === target.element)) {
        return {
          width: positiveInt(view.innerWidth, resource.width),
          height: positiveInt(view.innerHeight, resource.height),
        };
      }
      const rootResource = this.resources.get(this.rootWindow);
      const parentResource = resource.fullscreen
        ? rootResource
        : this.resources.get(target.parent) || rootResource;
      const boundsResource =
        parentResource && parentResource.kind !== "pixmap"
          ? parentResource
          : rootResource;

      return {
        width: boundsResource ? boundsResource.width : resource.width,
        height: boundsResource ? boundsResource.height : resource.height,
      };
    },

refreshWindowStateGeometry() {
      for (const resource of this.resources.values()) {
        if (
          resource.kind === "window" &&
          resource.element &&
          (resource.zoomed || resource.fullscreen)
        ) {
          this.applyGeometry(resource);
        }
      }
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    resizeCanvasForBitGravity(resource) {
      const canvas = resource.canvas;
      if (!canvas) {
        return;
      }
      const oldWidth = canvas.width;
      const oldHeight = canvas.height;
      const newWidth = positiveInt(resource.width, 1);
      const newHeight = positiveInt(resource.height, 1);
      if (oldWidth === newWidth && oldHeight === newHeight) {
        return;
      }
      const offset = this.bitGravityOffset(
        resource.bitGravity,
        oldWidth,
        oldHeight,
        newWidth,
        newHeight,
      );
      if (!offset.preserve || oldWidth <= 0 || oldHeight <= 0) {
        canvas.width = newWidth;
        canvas.height = newHeight;
        resource.context = canvas.getContext("2d");
        if (resource.context) {
          this.fillResourceBackground(resource.context, resource, 0, 0, newWidth, newHeight);
        }
        return;
      }

      const snapshot = this.createCanvas(oldWidth, oldHeight);
      const snapshotContext = snapshot.getContext("2d");
      if (!snapshotContext) {
        canvas.width = newWidth;
        canvas.height = newHeight;
        resource.context = canvas.getContext("2d");
        if (resource.context) {
          this.fillResourceBackground(resource.context, resource, 0, 0, newWidth, newHeight);
        }
        return;
      }
      snapshotContext.drawImage(canvas, 0, 0);
      canvas.width = newWidth;
      canvas.height = newHeight;
      const context = canvas.getContext("2d");
      resource.context = context;
      if (context) {
        this.fillResourceBackground(context, resource, 0, 0, newWidth, newHeight);
        context.drawImage(snapshot, offset.x, offset.y);
      }
    },

    /**
     * @param {number} gravity
     * @param {number} oldWidth
     * @param {number} oldHeight
     * @param {number} newWidth
     * @param {number} newHeight
     * @returns {{ x: number, y: number, preserve: boolean }}
     */
    bitGravityOffset(gravity, oldWidth, oldHeight, newWidth, newHeight) {
      const deltaX = newWidth - oldWidth;
      const deltaY = newHeight - oldHeight;
      const centerX = Math.trunc(deltaX / 2);
      const centerY = Math.trunc(deltaY / 2);
      switch (gravity) {
        case FORGET_GRAVITY:
          return { x: 0, y: 0, preserve: false };
        case NORTH_GRAVITY:
          return { x: centerX, y: 0, preserve: true };
        case NORTH_EAST_GRAVITY:
          return { x: deltaX, y: 0, preserve: true };
        case WEST_GRAVITY:
          return { x: 0, y: centerY, preserve: true };
        case CENTER_GRAVITY:
          return { x: centerX, y: centerY, preserve: true };
        case EAST_GRAVITY:
          return { x: deltaX, y: centerY, preserve: true };
        case SOUTH_WEST_GRAVITY:
          return { x: 0, y: deltaY, preserve: true };
        case SOUTH_GRAVITY:
          return { x: centerX, y: deltaY, preserve: true };
        case SOUTH_EAST_GRAVITY:
          return { x: deltaX, y: deltaY, preserve: true };
        case NORTH_WEST_GRAVITY:
        default:
          return { x: 0, y: 0, preserve: true };
      }
    },

    /**
     * @param {HTMLElement} parentElement
     * @param {number} id
     * @returns {HTMLElement | null}
     */
    siblingWindowElement(parentElement, id) {
      if (id === 0) {
        return null;
      }
      const resource = this.resources.get(id);
      const element = resource && resource.element;
      return element && element.parentElement === parentElement ? element : null;
    },

    /**
     * @param {HTMLElement} element
     * @returns {X11WasmDOMResource | null}
     */
    windowResourceForElement(element) {
      const idText = element.getAttribute(RESOURCE_ATTR);
      const id = idText ? Number.parseInt(idText, 10) : 0;
      return Number.isFinite(id) ? this.resources.get(id) || null : null;
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {number} ancestorId
     * @returns {boolean}
     */
    isTransientDescendantOf(resource, ancestorId) {
      const visited = new Set([resource.id]);
      let transientFor = resource.transientFor;

      while (transientFor) {
        if (transientFor === ancestorId) {
          return true;
        }
        if (visited.has(transientFor)) {
          return false;
        }
        visited.add(transientFor);
        const parent = this.resources.get(transientFor);
        if (!parent) {
          return false;
        }
        transientFor = parent.transientFor;
      }
      return false;
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @returns {boolean}
     */
    isEffectivelyTopmost(resource) {
      if (resource.topmost) {
        return true;
      }
      const visited = new Set([resource.id]);
      let transientFor = resource.transientFor;

      while (transientFor) {
        if (visited.has(transientFor)) {
          return false;
        }
        visited.add(transientFor);
        const parent = this.resources.get(transientFor);
        if (!parent) {
          return false;
        }
        if (parent.topmost) {
          return true;
        }
        transientFor = parent.transientFor;
      }
      return false;
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @returns {X11WasmDOMResource}
     */
    stackingResourceFor(resource) {
      const visited = new Set();
      let current = resource;

      while (!visited.has(current.id)) {
        visited.add(current.id);
        const parent = this.resources.get(current.parent);
        if (!parent || parent.kind === "root" || !parent.element) {
          break;
        }
        current = parent;
      }
      return current;
    },

syncTransientStacking() {
      const resources = Array.from(this.resources.values());
      const maxPasses = resources.length;

      for (let pass = 0; pass < maxPasses; pass++) {
        let changed = false;

        for (const resource of resources) {
          if (!resource.transientFor || !resource.element) {
            continue;
          }
          const parent = this.resources.get(resource.transientFor);
          if (
            !parent ||
            parent.id === resource.id ||
            !parent.element ||
            this.isTransientDescendantOf(parent, resource.id)
          ) {
            continue;
          }
          const stackResource = this.stackingResourceFor(resource);
          const parentStackResource = this.stackingResourceFor(parent);
          const element = stackResource.element;
          const parentElement = parentStackResource.element;
          const commonParent = parentElement?.parentElement;
          if (
            !element ||
            !parentElement ||
            !commonParent ||
            element === parentElement ||
            element.parentElement !== commonParent
          ) {
            continue;
          }
          if (element.compareDocumentPosition(parentElement)
              & global.Node.DOCUMENT_POSITION_FOLLOWING) {
            commonParent.insertBefore(element, parentElement.nextSibling);
            changed = true;
          }
        }
        if (!changed) {
          break;
        }
      }
    },

    /**
     * @param {X11WasmDOMResource} parentResource
     */
    restackTransientGroup(parentResource) {
      void parentResource;
      this.syncTransientStacking();
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    restackRelatedTransientGroups(resource) {
      void resource;
      this.syncTransientStacking();
    },

    /**
     * @param {X11WasmDOMResource} parentResource
     */
    applyTransientStackGeometry(parentResource) {
      const stackResource = this.stackingResourceFor(parentResource);

      for (const resource of this.resources.values()) {
        if (this.stackingResourceFor(resource) === stackResource) {
          this.applyGeometry(resource);
        }
      }
    },

    /**
     * @param {X11WasmDOMResource} parentResource
     */
    applyTransientDescendantGeometry(parentResource) {
      this.applyTransientStackGeometry(parentResource);
      for (const resource of this.resources.values()) {
        if (
          resource.element &&
          this.isTransientDescendantOf(resource, parentResource.id)
        ) {
          this.applyTransientStackGeometry(resource);
        }
      }
    },

    /**
     * @param {HTMLElement} parentElement
     * @returns {HTMLElement | null}
     */
    firstChildWindowElement(parentElement) {
      for (const child of parentElement.children) {
        if (
          child instanceof global.HTMLElement &&
          child.classList.contains("x11-wasm-window")
        ) {
          return child;
        }
      }
      return null;
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applyStyle(resource) {
      if (!resource.element) {
        return;
      }
      resource.element.style.backgroundColor = resource.backgroundColor || "";
      resource.element.style.cursor = resource.cursor || "";
      resource.element.style.borderStyle =
        resource.borderWidth > 0 ? "solid" : "none";
      resource.element.style.borderWidth = `${resource.borderWidth}px`;
      resource.element.style.borderColor =
        resource.borderColor || "transparent";
      const borderImage = this.borderImageSource(resource.borderPixmap);
      resource.element.style.borderImageSource = borderImage || "";
      resource.element.style.borderImageSlice = borderImage ? "1" : "";
      resource.element.style.borderImageRepeat = borderImage ? "repeat" : "";
      this.applySizeHints(resource);
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applySizeHints(resource) {
      const element = resource.element;
      const hints = resource.sizeHints;
      if (!element) {
        return;
      }
      if (!hints || resource.nativeGeometry) {
        element.style.minWidth = "";
        element.style.minHeight = "";
        element.style.maxWidth = "";
        element.style.maxHeight = "";
        element.style.aspectRatio = "";
        return;
      }
      const setLength = (property, enabled, value) => {
        element.style[property] =
          enabled && Number.isFinite(value) && value > 0
            ? `${Math.floor(value)}px`
            : "";
      };

      setLength("minWidth", hints.flags & P_MIN_SIZE, hints.minWidth);
      setLength("minHeight", hints.flags & P_MIN_SIZE, hints.minHeight);
      setLength("maxWidth", hints.flags & P_MAX_SIZE, hints.maxWidth);
      setLength("maxHeight", hints.flags & P_MAX_SIZE, hints.maxHeight);
      if (
        hints.flags & P_ASPECT &&
        hints.minAspectX > 0 &&
        hints.minAspectY > 0 &&
        hints.maxAspectX > 0 &&
        hints.maxAspectY > 0 &&
        hints.minAspectX * hints.maxAspectY ===
          hints.maxAspectX * hints.minAspectY
      ) {
        element.style.aspectRatio =
          `${Math.floor(hints.minAspectX)} / ${Math.floor(hints.minAspectY)}`;
      } else {
        element.style.aspectRatio = "";
      }
    },

    /**
     * @param {number} pixmapId
     * @returns {string}
     */
    borderImageSource(pixmapId) {
      if (pixmapId === 0) {
        return "";
      }
      const pixmap = this.resources.get(pixmapId);
      if (
        !pixmap ||
        pixmap.kind !== "pixmap" ||
        !pixmap.canvas ||
        pixmap.width <= 0 ||
        pixmap.height <= 0
      ) {
        return "";
      }
      if (pixmap.geometryDirty) {
        this.flushResourceGeometry(pixmap);
      }
      const dataUrl = this.canvasDataUrl(pixmap.canvas, pixmap.width, pixmap.height);
      return dataUrl ? `url("${dataUrl}")` : "";
    },

    /**
     * @param {HTMLCanvasElement | OffscreenCanvas} source
     * @param {number} width
     * @param {number} height
     * @returns {string}
     */
    canvasDataUrl(source, width, height) {
      if ("toDataURL" in source && typeof source.toDataURL === "function") {
        return source.toDataURL();
      }
      if (!this.document) {
        return "";
      }
      const canvas = this.document.createElement("canvas");
      canvas.width = width;
      canvas.height = height;
      const context = canvas.getContext("2d");
      if (!context) {
        return "";
      }
      context.drawImage(source, 0, 0);
      return canvas.toDataURL();
    },

    /**
     * @param {unknown} error
     * @returns {string}
     */
    errorMessage(error) {
      if (error && typeof error === "object" && "message" in error) {
        return String(error.message);
      }
      return String(error);
    },

    /**
     * @param {number} id
     * @returns {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D | null}
     */
    getContext(id) {
      const resource = this.resources.get(id);
      if (!resource) {
        return null;
      }
      if (resource.geometryDirty) {
        this.flushResourceGeometry(resource);
      }
      return resource.context;
    }
  }));
})(typeof globalThis !== "undefined" ? globalThis : window);
