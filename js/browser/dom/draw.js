// @ts-check
/*
 * draw.js --
 *
 *	Clip, draw style, rasterization, text, image, and copy methods.
 */

(function attachX11WasmDOMBackendDraw(global) {
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

  /**
   * @param {X11WasmDOMDrawStyle} style
   * @returns {boolean}
   */
  function drawStyleUsesFullPlaneMask(style) {
    return (unsignedInt(style.planeMask, 0xffffffff) & 0xffffff) === 0xffffff;
  }

  /**
   * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
   * @param {number} fontPixelSize
   * @param {string} fontFamily
   */
  function prepareTextContext(context, fontPixelSize, fontFamily) {
    context.font = `${positiveInt(fontPixelSize, 14)}px ${fontFamilyToCss(
      fontFamily,
    )}`;
    context.textBaseline = "alphabetic";
    if ("fontKerning" in context) {
      context.fontKerning = "none";
    }
    if ("letterSpacing" in context) {
      context.letterSpacing = "0px";
    }
    if ("wordSpacing" in context) {
      context.wordSpacing = "0px";
    }
  }

  /**
   * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
   * @param {string} text
   * @returns {number}
   */
  function textAdvanceWidth(context, text) {
    return Math.max(0, Math.ceil(context.measureText(text).width));
  }

  Object.assign(X11WasmDOMBackend.prototype, /** @type {any} */ ({
    /**
     * @param {boolean} enabled
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @returns {number}
     */
    setClipRect(enabled, x, y, width, height) {
      if (!enabled) {
        this.currentClip = null;
      } else {
        this.currentClip = [
          {
            x: Math.floor(x),
            y: Math.floor(y),
            width: Math.max(0, Math.floor(width)),
            height: Math.max(0, Math.floor(height)),
          },
        ];
      }
      return 0;
    },

    /**
     * @param {boolean} enabled
     * @param {X11WasmDOMClipRect[]} rects
     * @returns {number}
     */
    setClipRects(enabled, rects) {
      if (!enabled) {
        this.currentClip = null;
      } else {
        this.currentClip = rects
          .map((rect) => ({
            x: Math.floor(rect.x),
            y: Math.floor(rect.y),
            width: Math.max(0, Math.floor(rect.width)),
            height: Math.max(0, Math.floor(rect.height)),
          }))
          .filter((rect) => rect.width > 0 && rect.height > 0);
      }
      return 0;
    },

    /**
     * @param {boolean} enabled
     * @param {number} pixmap
     * @param {number} xOrigin
     * @param {number} yOrigin
     * @returns {number}
     */
    setClipMask(enabled, pixmap, xOrigin, yOrigin) {
      if (!enabled || pixmap === 0) {
        this.currentClipMask = null;
      } else {
        this.currentClipMask = {
          pixmap: Math.trunc(pixmap),
          xOrigin: Math.floor(xOrigin),
          yOrigin: Math.floor(yOrigin),
        };
      }
      return 0;
    },

    /**
     * @param {number} functionId
     * @param {number} lineWidth
     * @param {number} lineStyle
     * @param {number} capStyle
     * @param {number} joinStyle
     * @param {number} dashOffset
     * @param {number} dashLength
     * @param {number[]} dashPattern
     * @param {number} fillStyle
     * @param {number} stipple
     * @param {number} tile
     * @param {number} tsOriginX
     * @param {number} tsOriginY
     * @param {number} backgroundPixel
     * @param {number} fontPixelSize
     * @param {string} fontFamily
     * @param {number=} planeMask
     * @returns {number}
     */
    setDrawStyle(
      functionId,
      lineWidth,
      lineStyle,
      capStyle,
      joinStyle,
      dashOffset,
      dashLength,
      dashPattern,
      fillStyle,
      stipple,
      tile,
      tsOriginX,
      tsOriginY,
      backgroundPixel,
      fontPixelSize,
      fontFamily,
      planeMask = 0xffffffff,
    ) {
      this.currentDrawStyle = {
        functionId,
        lineWidth: positiveInt(lineWidth, 1),
        lineStyle,
        capStyle,
        joinStyle,
        dashOffset,
        dashLength: positiveInt(dashLength, 4),
        dashPattern: (Array.isArray(dashPattern) ? dashPattern : [])
          .map((length) => positiveInt(length, 1))
          .filter((length) => length > 0),
        fillStyle,
        stipple,
        tile,
        tsOriginX,
        tsOriginY,
        backgroundPixel,
        fontPixelSize: positiveInt(fontPixelSize, 14),
        fontFamily:
          typeof fontFamily === "string" && fontFamily.length > 0
            ? fontFamily
            : "sans-serif",
        planeMask: unsignedInt(planeMask, 0xffffffff),
      };
      return 0;
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {number} pixel
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @returns {boolean | null}
     */
    rasterFillSolidRectangle(context, pixel, x, y, width, height) {
      const style = this.currentDrawStyle;
      if (
        style.fillStyle !== FILL_SOLID ||
        (style.functionId === GX_COPY && drawStyleUsesFullPlaneMask(style))
      ) {
        return null;
      }
      if (style.functionId === GX_NOOP) {
        return false;
      }
      const canvas = context.canvas;
      let destX = Math.floor(x);
      let destY = Math.floor(y);
      let destWidth = Math.max(0, Math.floor(width));
      let destHeight = Math.max(0, Math.floor(height));
      if (destWidth <= 0 || destHeight <= 0) {
        return false;
      }

      const clip = this.currentClip;
      if (clip) {
        const bounds = this.clipBounds(clip);
        if (!bounds) {
          return false;
        }
        const left = Math.max(destX, bounds.x);
        const top = Math.max(destY, bounds.y);
        const right = Math.min(destX + destWidth, bounds.x + bounds.width);
        const bottom = Math.min(destY + destHeight, bounds.y + bounds.height);
        if (right <= left || bottom <= top) {
          return false;
        }
        destX = left;
        destY = top;
        destWidth = right - left;
        destHeight = bottom - top;
      }

      const left = Math.max(0, destX);
      const top = Math.max(0, destY);
      const right = Math.min(destX + destWidth, canvas.width);
      const bottom = Math.min(destY + destHeight, canvas.height);
      if (right <= left || bottom <= top) {
        return false;
      }
      destX = left;
      destY = top;
      destWidth = right - left;
      destHeight = bottom - top;

      const clipMask = this.clipMaskSnapshot(this.currentClipMask);
      if (this.currentClipMask && (!clipMask || !clipMask.hasBits)) {
        return false;
      }

      const imageData = context.getImageData(destX, destY, destWidth, destHeight);
      const [red, green, blue] = pixelToRgb(pixel);
      let offset = 0;
      for (let row = 0; row < destHeight; row++) {
        for (let col = 0; col < destWidth; col++) {
          const targetX = destX + col;
          const targetY = destY + row;
          if (
            this.clipContains(clip, targetX, targetY) &&
            this.clipMaskContains(clipMask, targetX, targetY)
          ) {
            applyRasterFunctionPixel(
              imageData.data,
              offset,
              style.functionId,
              red,
              green,
              blue,
              255,
              style.planeMask,
            );
          }
          offset += 4;
        }
      }
      context.putImageData(imageData, destX, destY);
      return true;
    },

    /**
     * @returns {number[]}
     */
    currentLineDashPattern() {
      const style = this.currentDrawStyle;
      return style.lineStyle === LINE_SOLID
        ? []
        : style.dashPattern.length > 0
          ? style.dashPattern
          : [
              positiveInt(style.dashLength, 4),
              positiveInt(style.dashLength, 4),
            ];
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     */
    prepareCurrentDrawContext(context) {
      const style = this.currentDrawStyle;

      context.lineWidth = positiveInt(style.lineWidth, 1);
      context.lineCap = lineCapForStyle(style.capStyle);
      context.lineJoin = lineJoinForStyle(style.joinStyle);
      context.lineDashOffset = style.dashOffset;
      context.setLineDash(this.currentLineDashPattern());
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} context
     * @param {{ x: number, y: number, width: number, height: number }} bounds
     * @param {(context: CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D) => void} draw
     * @param {((context: CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D) => void)=} drawBackground
     * @returns {boolean | null}
     */
    rasterDrawShape(context, bounds, draw, drawBackground) {
      const style = this.currentDrawStyle;
      if (style.functionId === GX_COPY && drawStyleUsesFullPlaneMask(style)) {
        return null;
      }
      if (style.functionId === GX_NOOP) {
        return false;
      }

      const canvas = context.canvas;
      let destX = Math.floor(bounds.x);
      let destY = Math.floor(bounds.y);
      let destWidth = Math.max(0, Math.ceil(bounds.width));
      let destHeight = Math.max(0, Math.ceil(bounds.height));
      if (destWidth <= 0 || destHeight <= 0) {
        return false;
      }

      const clip = this.currentClip;
      if (clip) {
        const clipBounds = this.clipBounds(clip);
        if (!clipBounds) {
          return false;
        }
        const left = Math.max(destX, clipBounds.x);
        const top = Math.max(destY, clipBounds.y);
        const right = Math.min(
          destX + destWidth,
          clipBounds.x + clipBounds.width,
        );
        const bottom = Math.min(
          destY + destHeight,
          clipBounds.y + clipBounds.height,
        );
        if (right <= left || bottom <= top) {
          return false;
        }
        destX = left;
        destY = top;
        destWidth = right - left;
        destHeight = bottom - top;
      }

      const left = Math.max(0, destX);
      const top = Math.max(0, destY);
      const right = Math.min(destX + destWidth, canvas.width);
      const bottom = Math.min(destY + destHeight, canvas.height);
      if (right <= left || bottom <= top) {
        return false;
      }
      destX = left;
      destY = top;
      destWidth = right - left;
      destHeight = bottom - top;

      const clipMask = this.clipMaskSnapshot(this.currentClipMask);
      if (this.currentClipMask && (!clipMask || !clipMask.hasBits)) {
        return false;
      }

      const sourceCanvas = this.createCanvas(destWidth, destHeight);
      const sourceContext = sourceCanvas.getContext("2d");
      if (!sourceContext) {
        return false;
      }
      sourceContext.save();
      sourceContext.translate(-destX, -destY);
      this.prepareCurrentDrawContext(sourceContext);
      if (style.lineStyle === LINE_DOUBLE_DASH && drawBackground) {
        sourceContext.strokeStyle = pixelToCss(style.backgroundPixel);
        sourceContext.setLineDash([]);
        sourceContext.lineDashOffset = 0;
        drawBackground(sourceContext);
        sourceContext.lineDashOffset = style.dashOffset;
        sourceContext.setLineDash(this.currentLineDashPattern());
      }
      draw(sourceContext);
      sourceContext.restore();

      const sourceData = sourceContext.getImageData(0, 0, destWidth, destHeight);
      const destData = context.getImageData(destX, destY, destWidth, destHeight);
      let offset = 0;
      for (let row = 0; row < destHeight; row++) {
        for (let col = 0; col < destWidth; col++) {
          const targetX = destX + col;
          const targetY = destY + row;
          const alpha = sourceData.data[offset + 3];
          if (
            alpha !== 0 &&
            this.clipContains(clip, targetX, targetY) &&
            this.clipMaskContains(clipMask, targetX, targetY)
          ) {
            applyRasterFunctionPixel(
              destData.data,
              offset,
              style.functionId,
              sourceData.data[offset],
              sourceData.data[offset + 1],
              sourceData.data[offset + 2],
              alpha,
              style.planeMask,
            );
          }
          offset += 4;
        }
      }
      context.putImageData(destData, destX, destY);
      return true;
    },

    /**
     * @param {HTMLCanvasElement | OffscreenCanvas} sourceCanvas
     * @param {HTMLCanvasElement | OffscreenCanvas} targetCanvas
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} width
     * @param {number} height
     * @param {number} destX
     * @param {number} destY
     * @param {X11WasmDOMClipRect[] | null} clip
     * @returns {{ readX: number, readY: number, writeX: number, writeY: number, width: number, height: number } | null}
     */
    normalizeImageTransfer(
      sourceCanvas,
      targetCanvas,
      srcX,
      srcY,
      width,
      height,
      destX,
      destY,
      clip,
    ) {
      let readX = Math.floor(srcX);
      let readY = Math.floor(srcY);
      let writeX = Math.floor(destX);
      let writeY = Math.floor(destY);
      let copyWidth = Math.max(0, Math.floor(width));
      let copyHeight = Math.max(0, Math.floor(height));
      if (copyWidth <= 0 || copyHeight <= 0) {
        return null;
      }

      /** @param {number} amount */
      const cropLeft = (amount) => {
        readX += amount;
        writeX += amount;
        copyWidth -= amount;
      };
      /** @param {number} amount */
      const cropTop = (amount) => {
        readY += amount;
        writeY += amount;
        copyHeight -= amount;
      };

      if (clip) {
        const bounds = this.clipBounds(clip);
        if (!bounds) {
          return null;
        }
        const left = Math.max(writeX, bounds.x);
        const top = Math.max(writeY, bounds.y);
        const right = Math.min(writeX + copyWidth, bounds.x + bounds.width);
        const bottom = Math.min(writeY + copyHeight, bounds.y + bounds.height);
        if (right <= left || bottom <= top) {
          return null;
        }
        cropLeft(left - writeX);
        cropTop(top - writeY);
        copyWidth = right - left;
        copyHeight = bottom - top;
      }

      if (writeX < 0) cropLeft(-writeX);
      if (writeY < 0) cropTop(-writeY);
      if (readX < 0) cropLeft(-readX);
      if (readY < 0) cropTop(-readY);
      copyWidth = Math.min(
        copyWidth,
        sourceCanvas.width - readX,
        targetCanvas.width - writeX,
      );
      copyHeight = Math.min(
        copyHeight,
        sourceCanvas.height - readY,
        targetCanvas.height - writeY,
      );
      if (copyWidth <= 0 || copyHeight <= 0) {
        return null;
      }
      return {
        readX,
        readY,
        writeX,
        writeY,
        width: copyWidth,
        height: copyHeight,
      };
    },

    /**
     * @param {HTMLCanvasElement | OffscreenCanvas} targetCanvas
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} destX
     * @param {number} destY
     * @param {number} width
     * @param {number} height
     * @param {X11WasmDOMClipRect[] | null} clip
     * @returns {{ readX: number, readY: number, writeX: number, writeY: number, width: number, height: number } | null}
     */
    normalizeImageWrite(
      targetCanvas,
      srcX,
      srcY,
      destX,
      destY,
      width,
      height,
      clip,
    ) {
      let readX = Math.floor(srcX);
      let readY = Math.floor(srcY);
      let writeX = Math.floor(destX);
      let writeY = Math.floor(destY);
      let copyWidth = Math.max(0, Math.floor(width));
      let copyHeight = Math.max(0, Math.floor(height));
      if (copyWidth <= 0 || copyHeight <= 0) {
        return null;
      }

      /** @param {number} amount */
      const cropLeft = (amount) => {
        readX += amount;
        writeX += amount;
        copyWidth -= amount;
      };
      /** @param {number} amount */
      const cropTop = (amount) => {
        readY += amount;
        writeY += amount;
        copyHeight -= amount;
      };

      if (clip) {
        const bounds = this.clipBounds(clip);
        if (!bounds) {
          return null;
        }
        const left = Math.max(writeX, bounds.x);
        const top = Math.max(writeY, bounds.y);
        const right = Math.min(writeX + copyWidth, bounds.x + bounds.width);
        const bottom = Math.min(writeY + copyHeight, bounds.y + bounds.height);
        if (right <= left || bottom <= top) {
          return null;
        }
        cropLeft(left - writeX);
        cropTop(top - writeY);
        copyWidth = right - left;
        copyHeight = bottom - top;
      }

      if (writeX < 0) cropLeft(-writeX);
      if (writeY < 0) cropTop(-writeY);
      copyWidth = Math.min(copyWidth, targetCanvas.width - writeX);
      copyHeight = Math.min(copyHeight, targetCanvas.height - writeY);
      if (copyWidth <= 0 || copyHeight <= 0) {
        return null;
      }
      return {
        readX,
        readY,
        writeX,
        writeY,
        width: copyWidth,
        height: copyHeight,
      };
    },

    /**
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} sourceContext
     * @param {CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D} targetContext
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} width
     * @param {number} height
     * @param {number} destX
     * @param {number} destY
     * @returns {boolean}
     */
    rasterCopyArea(
      sourceContext,
      targetContext,
      srcX,
      srcY,
      width,
      height,
      destX,
      destY,
    ) {
      const style = this.currentDrawStyle;
      if (style.functionId === GX_NOOP) {
        return false;
      }
      const sourceCanvas = sourceContext.canvas;
      const targetCanvas = targetContext.canvas;
      const clip = this.currentClip;
      const region = this.normalizeImageTransfer(
        sourceCanvas,
        targetCanvas,
        srcX,
        srcY,
        width,
        height,
        destX,
        destY,
        clip,
      );
      if (!region) {
        return false;
      }
      const {
        readX,
        readY,
        writeX,
        writeY,
        width: copyWidth,
        height: copyHeight,
      } = region;

      const clipMask = this.clipMaskSnapshot(this.currentClipMask);
      if (this.currentClipMask && (!clipMask || !clipMask.hasBits)) {
        return false;
      }

      const sourceData = sourceContext.getImageData(
        readX,
        readY,
        copyWidth,
        copyHeight,
      );
      const targetData = targetContext.getImageData(
        writeX,
        writeY,
        copyWidth,
        copyHeight,
      );
      let offset = 0;
      for (let row = 0; row < copyHeight; row++) {
        for (let col = 0; col < copyWidth; col++) {
          const targetX = writeX + col;
          const targetY = writeY + row;
          if (
            this.clipContains(clip, targetX, targetY) &&
            this.clipMaskContains(clipMask, targetX, targetY)
          ) {
            const sourceAlpha = sourceData.data[offset + 3];
            const sourceRed = sourceAlpha === 0 ? 0 : sourceData.data[offset];
            const sourceGreen =
              sourceAlpha === 0 ? 0 : sourceData.data[offset + 1];
            const sourceBlue =
              sourceAlpha === 0 ? 0 : sourceData.data[offset + 2];
            applyRasterFunctionPixel(
              targetData.data,
              offset,
              style.functionId,
              sourceRed,
              sourceGreen,
              sourceBlue,
              sourceAlpha,
              style.planeMask,
            );
          }
          offset += 4;
        }
      }
      targetContext.putImageData(targetData, writeX, writeY);
      return true;
    },

    /**
     * @param {number} source
     * @param {number} target
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} width
     * @param {number} height
     * @param {number} destX
     * @param {number} destY
     * @returns {number}
     */
    copyArea(source, target, srcX, srcY, width, height, destX, destY) {
      const sourceResource = this.resources.get(source);
      const sourceContext = sourceResource && sourceResource.context;
      const targetContext = this.getContext(target);
      const targetResource = this.resources.get(target);
      if (
        !sourceResource ||
        !sourceResource.canvas ||
        !sourceContext ||
        !targetContext
      ) {
        return 0;
      }
      if (sourceResource.geometryDirty) {
        this.flushResourceGeometry(sourceResource);
      }
      if (
        this.rasterCopyArea(
          sourceContext,
          targetContext,
          srcX,
          srcY,
          width,
          height,
          destX,
          destY,
        ) &&
        targetResource
      ) {
        this.markDirty(targetResource, destX, destY, width, height);
      }
      return 0;
    },

    /**
     * @param {number} source
     * @param {number} target
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} width
     * @param {number} height
     * @param {number} destX
     * @param {number} destY
     * @param {number} foreground
     * @param {number} background
     * @param {number} plane
     * @param {boolean} transparentZero
     * @returns {number}
     */
    copyPlane(
      source,
      target,
      srcX,
      srcY,
      width,
      height,
      destX,
      destY,
      foreground,
      background,
      plane,
      transparentZero,
    ) {
      const sourceResource = this.resources.get(source);
      const sourceContext = sourceResource && sourceResource.context;
      const targetContext = this.getContext(target);
      const targetResource = this.resources.get(target);
      const style = this.currentDrawStyle;

      if (!sourceResource || !sourceContext || !targetContext) {
        return 0;
      }
      if (style.functionId === GX_NOOP) {
        return 0;
      }
      if (sourceResource.geometryDirty) {
        this.flushResourceGeometry(sourceResource);
      }
      const clip = this.currentClip;
      const sourceCanvas = sourceContext.canvas;
      const targetCanvas = targetContext.canvas;
      const region = this.normalizeImageTransfer(
        sourceCanvas,
        targetCanvas,
        srcX,
        srcY,
        width,
        height,
        destX,
        destY,
        clip,
      );
      if (!region) {
        return 0;
      }
      const {
        readX,
        readY,
        writeX,
        writeY,
        width: copyWidth,
        height: copyHeight,
      } = region;

      const clipMask = this.clipMaskSnapshot(this.currentClipMask);
      if (this.currentClipMask && (!clipMask || !clipMask.hasBits)) {
        return 0;
      }

      const sourceData = sourceContext.getImageData(
        readX,
        readY,
        copyWidth,
        copyHeight,
      );
      const imageData =
        style.functionId !== GX_COPY || transparentZero || clip || clipMask
          ? targetContext.getImageData(writeX, writeY, copyWidth, copyHeight)
          : targetContext.createImageData(copyWidth, copyHeight);
      const foregroundRgb = pixelToRgb(foreground);
      const backgroundRgb = pixelToRgb(background);
      const planeMask = Math.max(0, Math.trunc(plane));
      let offset = 0;

      for (let row = 0; row < copyHeight; row++) {
        for (let col = 0; col < copyWidth; col++) {
          const targetX = writeX + col;
          const targetY = writeY + row;
          const allowed =
            this.clipContains(clip, targetX, targetY) &&
            this.clipMaskContains(clipMask, targetX, targetY);
          const sourcePixel =
            (sourceData.data[offset] << 16) |
            (sourceData.data[offset + 1] << 8) |
            sourceData.data[offset + 2];
          const bit =
            sourceData.data[offset + 3] !== 0 &&
            (sourcePixel & planeMask) !== 0;
          const rgb = bit ? foregroundRgb : backgroundRgb;
          if (allowed && (bit || !transparentZero)) {
            applyRasterFunctionPixel(
              imageData.data,
              offset,
              style.functionId,
              rgb[0],
              rgb[1],
              rgb[2],
              255,
              style.planeMask,
            );
          }
          offset += 4;
        }
      }
      targetContext.putImageData(imageData, writeX, writeY);
      if (targetResource) {
        this.markDirty(targetResource, writeX, writeY, copyWidth, copyHeight);
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} pixel
     * @param {number} x1
     * @param {number} y1
     * @param {number} x2
     * @param {number} y2
     * @returns {number}
     */
    drawLine(id, pixel, x1, y1, x2, y2) {
      const context = this.getContext(id);
      const resource = this.resources.get(id);
      if (!context) {
        return 0;
      }
      const strokePath = (drawContext = context) => {
        drawContext.beginPath();
        drawContext.moveTo(x1 + 0.5, y1 + 0.5);
        drawContext.lineTo(x2 + 0.5, y2 + 0.5);
        drawContext.stroke();
      };
      const padding =
        Math.ceil(positiveInt(this.currentDrawStyle.lineWidth, 1) / 2) + 1;
      const bounds = {
        x: Math.min(x1, x2) - padding,
        y: Math.min(y1, y2) - padding,
        width: Math.abs(x2 - x1) + 1 + padding * 2,
        height: Math.abs(y2 - y1) + 1 + padding * 2,
      };
      let changed = this.rasterDrawShape(
        context,
        bounds,
        (drawContext) => {
          drawContext.strokeStyle = this.paintStyleForPixel(drawContext, pixel);
          strokePath(drawContext);
        },
        strokePath,
      );
      if (changed === null) {
        const drawFallbackStroke = () => {
          context.beginPath();
          context.moveTo(x1 + 0.5, y1 + 0.5);
          context.lineTo(x2 + 0.5, y2 + 0.5);
          context.stroke();
        };
        this.withCurrentClip(context, () => {
          this.withCurrentDrawStyle(
            context,
            () => {
              context.strokeStyle = this.paintStyleForPixel(context, pixel);
              drawFallbackStroke();
            },
            drawFallbackStroke,
          );
        });
        changed = true;
      }
      if (changed !== false && resource) {
        this.markDirty(
          resource,
          bounds.x,
          bounds.y,
          bounds.width,
          bounds.height,
        );
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} pixel
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @param {boolean} fill
     * @returns {number}
     */
    drawRectangle(id, pixel, x, y, width, height, fill) {
      const context = this.getContext(id);
      const resource = this.resources.get(id);
      if (!context) {
        return 0;
      }
      if (fill) {
        let changed = this.rasterFillSolidRectangle(
          context,
          pixel,
          x,
          y,
          width,
          height,
        );
        if (changed === null) {
          const bounds = { x, y, width, height };
          const fillRect = (drawContext = context) => {
            drawContext.fillStyle = this.paintStyleForPixel(drawContext, pixel);
            drawContext.fillRect(x, y, width, height);
          };
          changed = this.rasterDrawShape(context, bounds, fillRect);
          if (changed === null) {
            this.withCurrentClip(context, () => {
              this.withCurrentDrawStyle(context, () => {
                fillRect();
              });
            });
            changed = true;
          }
        }
        if (changed !== false && resource) {
          this.markDirty(resource, x, y, width, height);
        }
      } else {
        const strokePath = (drawContext = context) => {
          drawContext.strokeRect(x + 0.5, y + 0.5, width, height);
        };
        const padding =
          Math.ceil(positiveInt(this.currentDrawStyle.lineWidth, 1) / 2) + 1;
        const bounds = {
          x: x - padding,
          y: y - padding,
          width: width + padding * 2 + 1,
          height: height + padding * 2 + 1,
        };
        let changed = this.rasterDrawShape(
          context,
          bounds,
          (drawContext) => {
            drawContext.strokeStyle = this.paintStyleForPixel(
              drawContext,
              pixel,
            );
            strokePath(drawContext);
          },
          strokePath,
        );
        if (changed === null) {
          this.withCurrentClip(context, () => {
            this.withCurrentDrawStyle(
              context,
              () => {
                context.strokeStyle = this.paintStyleForPixel(context, pixel);
                strokePath();
              },
              strokePath,
            );
          });
          changed = true;
        }
        if (changed !== false && resource) {
          this.markDirty(
            resource,
            bounds.x,
            bounds.y,
            bounds.width,
            bounds.height,
          );
        }
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} pixel
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @param {number} angle1
     * @param {number} angle2
     * @param {boolean} fill
     * @param {boolean} pieSlice
     * @param {number} lineWidth
     * @returns {number}
     */
    drawArc(
      id,
      pixel,
      x,
      y,
      width,
      height,
      angle1,
      angle2,
      fill,
      pieSlice,
      lineWidth,
    ) {
      const context = this.getContext(id);
      const resource = this.resources.get(id);
      if (!context || width <= 0 || height <= 0 || angle2 === 0) {
        return 0;
      }

      const centerX = x + width / 2;
      const centerY = y + height / 2;
      const radiusX = width / 2;
      const radiusY = height / 2;
      const startAngle = (-(angle1 / 64) * Math.PI) / 180;
      const sweep = (angle2 / 64) * Math.PI / 180;
      const full = Math.abs(sweep) >= Math.PI * 2;
      const endAngle = full ? startAngle + Math.PI * 2 : startAngle - sweep;
      const counterclockwise = !full && sweep > 0;
      const strokeWidth = positiveInt(lineWidth, 1);
      const drawArcPath = (drawContext = context) => {
        drawContext.beginPath();
        if (fill && pieSlice && !full) {
          drawContext.moveTo(centerX, centerY);
        }
        drawContext.ellipse(
          centerX,
          centerY,
          radiusX,
          radiusY,
          0,
          full ? 0 : startAngle,
          full ? Math.PI * 2 : endAngle,
          counterclockwise,
        );
      };
      const padding = fill ? 0 : Math.ceil(strokeWidth / 2) + 1;
      const bounds = {
        x: x - padding,
        y: y - padding,
        width: width + padding * 2,
        height: height + padding * 2,
      };

      /** @type {boolean | null} */
      let changed = false;
      if (fill) {
        changed = this.rasterDrawShape(
          context,
          bounds,
          (drawContext) => {
            drawArcPath(drawContext);
            drawContext.fillStyle = this.paintStyleForPixel(drawContext, pixel);
            if (!full) {
              drawContext.closePath();
            }
            drawContext.fill();
          },
        );
        if (changed === null) {
          this.withCurrentClip(context, () => {
            this.withCurrentDrawStyle(context, () => {
              drawArcPath();
              context.fillStyle = this.paintStyleForPixel(context, pixel);
              if (!full) {
                context.closePath();
              }
              context.fill();
            });
          });
          changed = true;
        }
      } else {
        const strokePath = (drawContext = context) => {
          drawArcPath(drawContext);
          drawContext.lineWidth = strokeWidth;
          drawContext.stroke();
        };
        changed = this.rasterDrawShape(
          context,
          bounds,
          (drawContext) => {
            drawContext.strokeStyle = this.paintStyleForPixel(
              drawContext,
              pixel,
            );
            strokePath(drawContext);
          },
          strokePath,
        );
        if (changed === null) {
          this.withCurrentClip(context, () => {
            this.withCurrentDrawStyle(
              context,
              () => {
                context.strokeStyle = this.paintStyleForPixel(context, pixel);
                strokePath();
              },
              strokePath,
            );
          });
          changed = true;
        }
      }

      if (changed !== false && resource) {
        this.markDirty(
          resource,
          bounds.x,
          bounds.y,
          bounds.width,
          bounds.height,
        );
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} pixel
     * @param {number[]} points
     * @param {number} fillRule
     * @returns {number}
     */
    fillPolygon(id, pixel, points, fillRule) {
      const context = this.getContext(id);
      const resource = this.resources.get(id);
      if (!context || points.length < 6) {
        return 0;
      }

      let left = points[0];
      let right = points[0];
      let top = points[1];
      let bottom = points[1];
      for (let index = 2; index < points.length; index += 2) {
        const pointX = points[index];
        const pointY = points[index + 1];
        left = Math.min(left, pointX);
        right = Math.max(right, pointX);
        top = Math.min(top, pointY);
        bottom = Math.max(bottom, pointY);
      }

      const fillPath = (drawContext = context) => {
        drawContext.fillStyle = this.paintStyleForPixel(drawContext, pixel);
        drawContext.beginPath();
        drawContext.moveTo(points[0], points[1]);
        for (let index = 2; index < points.length; index += 2) {
          drawContext.lineTo(points[index], points[index + 1]);
        }
        drawContext.closePath();
        drawContext.fill(fillRule === 0 ? "evenodd" : "nonzero");
      };
      const bounds = {
        x: left,
        y: top,
        width: right - left + 1,
        height: bottom - top + 1,
      };
      let changed = this.rasterDrawShape(context, bounds, fillPath);

      if (changed === null) {
        this.withCurrentClip(context, () => {
          this.withCurrentDrawStyle(context, () => {
            fillPath();
          });
        });
        changed = true;
      }

      if (changed !== false && resource) {
        this.markDirty(resource, bounds.x, bounds.y, bounds.width, bounds.height);
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} pixel
     * @param {number} x
     * @param {number} y
     * @param {string} text
     * @returns {number}
     */
    drawText(id, pixel, x, y, text) {
      const context = this.getContext(id);
      const resource = this.resources.get(id);
      if (!context || text.length === 0) {
        return 0;
      }
      const fontPixelSize = this.currentDrawStyle.fontPixelSize;
      const fontFamily = this.currentDrawStyle.fontFamily;
      const drawTextRun = (drawContext = context) => {
        drawContext.fillStyle = this.paintStyleForPixel(drawContext, pixel);
        prepareTextContext(drawContext, fontPixelSize, fontFamily);
        drawContext.fillText(text, x, y);
      };
      prepareTextContext(context, fontPixelSize, fontFamily);
      const metrics = context.measureText(text);
      const left = Math.floor(x - (metrics.actualBoundingBoxLeft || 0) - 1);
      const top = Math.floor(y - (metrics.actualBoundingBoxAscent || 11) - 1);
      const right = Math.ceil(
        x +
          Math.max(
            metrics.width,
            metrics.actualBoundingBoxRight || 0,
            text.length * 8,
          ) +
          1,
      );
      const bottom = Math.ceil(y + (metrics.actualBoundingBoxDescent || 3) + 1);
      const bounds = {
        x: left,
        y: top,
        width: Math.max(1, right - left),
        height: Math.max(1, bottom - top),
      };
      let changed = this.rasterDrawShape(context, bounds, drawTextRun);
      if (changed === null) {
        this.withCurrentClip(context, () => {
          this.withCurrentDrawStyle(context, () => {
            drawTextRun();
          });
        });
        changed = true;
      }
      if (changed !== false && resource) {
        this.markDirty(resource, bounds.x, bounds.y, bounds.width, bounds.height);
      }
      return 0;
    },

    /**
     * @param {number} fontPixelSize
     * @param {string} fontFamily
     * @param {string} text
     * @returns {number}
     */
    measureText(fontPixelSize, fontFamily, text) {
      const rootCanvas = this.rootElement?.querySelector(":scope > canvas");
      const canvas =
        rootCanvas instanceof HTMLCanvasElement
          ? rootCanvas
          : this.createCanvas(1, 1);
      const context = canvas.getContext("2d");
      if (!context || text.length === 0) {
        return 0;
      }
      prepareTextContext(context, fontPixelSize, fontFamily);
      return textAdvanceWidth(context, text);
    },

    /**
     * @param {number} id
     * @param {Uint8Array | null} bytes
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} destX
     * @param {number} destY
     * @param {number} width
     * @param {number} height
     * @param {number} bytesPerLine
     * @param {number} bitsPerPixel
     * @returns {number}
     */
    putImage(
      id,
      bytes,
      srcX,
      srcY,
      destX,
      destY,
      width,
      height,
      bytesPerLine,
      bitsPerPixel,
    ) {
      const context = this.getContext(id);
      const resource = this.resources.get(id);
      const style = this.currentDrawStyle;
      if (!context || !bytes || width <= 0 || height <= 0) {
        return 0;
      }
      if (style.functionId === GX_NOOP) {
        return 0;
      }
      const clip = this.currentClip;
      const region = this.normalizeImageWrite(
        context.canvas,
        srcX,
        srcY,
        destX,
        destY,
        width,
        height,
        clip,
      );
      if (!region) {
        return 0;
      }
      srcX = region.readX;
      srcY = region.readY;
      destX = region.writeX;
      destY = region.writeY;
      width = region.width;
      height = region.height;
      const clipMask = this.clipMaskSnapshot(this.currentClipMask);
      if (this.currentClipMask && (!clipMask || !clipMask.hasBits)) {
        return 0;
      }
      const imageData = context.getImageData(destX, destY, width, height);
      const bytesPerPixel = Math.max(1, Math.floor(bitsPerPixel / 8));
      let out = 0;
      for (let row = 0; row < height; row++) {
        const rowStart =
          bitsPerPixel < 8
            ? (srcY + row) * bytesPerLine * 8 + srcX
            : (srcY + row) * bytesPerLine + srcX * bytesPerPixel;
        for (let col = 0; col < width; col++) {
          const source =
            bitsPerPixel < 8 ? rowStart + col : rowStart + col * bytesPerPixel;
          const sourceByte = bitsPerPixel < 8 ? source >> 3 : source;
          const targetX = destX + col;
          const targetY = destY + row;
          const sourceByteCount =
            bitsPerPixel >= 8 ? bytesPerPixel : 1;
          if (
            !this.clipContains(clip, targetX, targetY) ||
            !this.clipMaskContains(clipMask, targetX, targetY)
          ) {
            out += 4;
            continue;
          }
          if (
            source < 0 ||
            sourceByte < 0 ||
            sourceByte + sourceByteCount > bytes.length
          ) {
            out += 4;
            continue;
          }
          let red = 0;
          let green = 0;
          let blue = 0;
          if (bitsPerPixel >= 24) {
            red = bytes[source + 2] || 0;
            green = bytes[source + 1] || 0;
            blue = bytes[source] || 0;
          } else if (bitsPerPixel >= 16) {
            [red, green, blue] = rgb565ToRgb(
              (bytes[source] || 0) | ((bytes[source + 1] || 0) << 8),
            );
          } else if (bitsPerPixel >= 8) {
            const gray = bytes[source] || 0;
            red = gray;
            green = gray;
            blue = gray;
          } else {
            const byte = bytes[sourceByte] || 0;
            const bit = (byte >> (source & 7)) & 1;
            const value = bit ? 255 : 0;
            red = value;
            green = value;
            blue = value;
          }
          applyRasterFunctionPixel(
            imageData.data,
            out,
            style.functionId,
            red,
            green,
            blue,
            255,
            style.planeMask,
          );
          out += 4;
        }
      }
      context.putImageData(imageData, destX, destY);
      if (resource) {
        this.markDirty(resource, destX, destY, width, height);
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} srcX
     * @param {number} srcY
     * @param {number} width
     * @param {number} height
     * @param {Uint8Array | null} bytes
     * @param {number} bytesPerLine
     * @param {number} bitsPerPixel
     * @returns {number}
     */
    getImage(id, srcX, srcY, width, height, bytes, bytesPerLine, bitsPerPixel) {
      const context = this.getContext(id);
      const resource = this.resources.get(id);
      if (!context || !resource || !bytes || width <= 0 || height <= 0) {
        return 0;
      }
      bytes.fill(0);

      const left = Math.max(0, srcX);
      const top = Math.max(0, srcY);
      const right = Math.min(srcX + width, resource.width);
      const bottom = Math.min(srcY + height, resource.height);
      if (right <= left || bottom <= top) {
        return 0;
      }

      const readWidth = right - left;
      const readHeight = bottom - top;
      const imageData = context.getImageData(left, top, readWidth, readHeight);
      const bytesPerPixel = Math.max(1, Math.floor(bitsPerPixel / 8));
      let source = 0;

      for (let row = 0; row < readHeight; row++) {
        const outY = top - srcY + row;
        const rowStart =
          bitsPerPixel < 8
            ? outY * bytesPerLine * 8 + (left - srcX)
            : outY * bytesPerLine + (left - srcX) * bytesPerPixel;
        for (let col = 0; col < readWidth; col++) {
          const red = imageData.data[source++];
          const green = imageData.data[source++];
          const blue = imageData.data[source++];
          const alpha = imageData.data[source++];
          const out =
            bitsPerPixel < 8
              ? rowStart + col
              : rowStart + col * bytesPerPixel;
          const outByte = bitsPerPixel < 8 ? out >> 3 : out;

          if (out < 0 || outByte < 0 || outByte >= bytes.length) {
            continue;
          }
          if (bitsPerPixel >= 24) {
            bytes[out] = blue;
            if (out + 1 < bytes.length) {
              bytes[out + 1] = green;
            }
            if (out + 2 < bytes.length) {
              bytes[out + 2] = red;
            }
            if (bitsPerPixel >= 32 && out + 3 < bytes.length) {
              bytes[out + 3] = alpha;
            }
          } else if (bitsPerPixel >= 16) {
            const pixel = rgbToRgb565(red, green, blue);
            bytes[out] = pixel & 0xff;
            if (out + 1 < bytes.length) {
              bytes[out + 1] = (pixel >> 8) & 0xff;
            }
          } else if (bitsPerPixel >= 8) {
            bytes[out] = Math.round((red + green + blue) / 3);
          } else if (alpha !== 0 && (red !== 0 || green !== 0 || blue !== 0)) {
            bytes[outByte] |= 1 << (out & 7);
          }
        }
      }
      return 0;
    }
  }));
})(typeof globalThis !== "undefined" ? globalThis : window);
