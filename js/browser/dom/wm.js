// @ts-check
/*
 * wm.js --
 *
 *	Window manager metadata, icon, protocol, state, and size hint methods.
 */

(function attachX11WasmDOMBackendWM(global) {
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
     * @param {number} id
     * @param {string} title
     * @param {boolean} updatesDocument
     * @returns {number}
     */
    setWindowTitle(id, title, updatesDocument = false) {
      const resource = this.resources.get(id);
      if (resource) {
        resource.title = title;
        if (resource.element) {
          resource.element.setAttribute("data-x11-wasm-title", title);
        }
      }
      if (this.document && updatesDocument) {
        this.document.title = title;
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {string} iconName
     * @returns {number}
     */
    setWindowIconName(id, iconName) {
      const resource = this.resources.get(id);
      if (resource) {
        resource.iconName = iconName;
        if (resource.element) {
          resource.element.setAttribute("data-x11-wasm-icon-name", iconName);
        }
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {boolean} updatesDocument
     * @returns {number}
     */
    clearWindowMetadata(id, updatesDocument = false) {
      const resource = this.resources.get(id);
      if (!resource) {
        return 0;
      }

      resource.title = "";
      resource.iconName = "";
      resource.alpha = 1;
      if (resource.element) {
        resource.element.removeAttribute("data-x11-wasm-title");
        resource.element.removeAttribute("data-x11-wasm-icon-name");
        resource.element.removeAttribute("data-x11-wasm-alpha");
        resource.element.style.opacity = "";
      }
      if (this.document && updatesDocument) {
        this.document.title = "";
      }
      this.setWindowTransientFor(id, 0);
      this.setWindowCommand(id, "");
      this.setWindowClientMachine(id, "");
      this.setWindowClassHint(id, "", "");
      this.setWindowSizeHints(
        id,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
      );
      this.setWindowZoomHints(
        id,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
      );
      this.setWindowWMHints(id, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      this.setWindowIconSizes(id, "", 0);
      this.setWindowIconPhoto(id, 0, "", null, 0, 0);
      this.setWindowProtocols(id, "");
      this.setWindowTypes(id, "");
      this.setWindowColormapWindows(id, "");
      this.setWindowState(id, false, false, false);
      resource.standardColormaps = Object.create(null);
      this.applyStandardColormapAttributes(resource);
      this.syncDocumentIconFromResources();
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} transientFor
     * @returns {number}
     */
    setWindowTransientFor(id, transientFor) {
      const resource = this.resources.get(id);
      if (resource) {
        const oldTransientFor = resource.transientFor;
        resource.transientFor = transientFor;
        if (resource.element) {
          if (transientFor) {
            resource.element.setAttribute(
              "data-x11-wasm-transient-for",
              String(transientFor),
            );
          } else {
            resource.element.removeAttribute("data-x11-wasm-transient-for");
          }
        }
        this.restackRelatedTransientGroups(resource);
        const oldParent = this.resources.get(oldTransientFor);
        if (oldParent) {
          this.restackTransientGroup(oldParent);
        }
        this.applyGeometry(resource);
        this.applyTransientDescendantGeometry(resource);
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {string} command
     * @returns {number}
     */
    setWindowCommand(id, command) {
      const resource = this.resources.get(id);
      if (resource) {
        resource.command = command;
        if (resource.element) {
          if (command) {
            resource.element.setAttribute("data-x11-wasm-command", command);
          } else {
            resource.element.removeAttribute("data-x11-wasm-command");
          }
        }
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {string} clientMachine
     * @returns {number}
     */
    setWindowClientMachine(id, clientMachine) {
      const resource = this.resources.get(id);
      if (resource) {
        resource.clientMachine = clientMachine;
        if (resource.element) {
          if (clientMachine) {
            resource.element.setAttribute(
              "data-x11-wasm-client-machine",
              clientMachine,
            );
          } else {
            resource.element.removeAttribute("data-x11-wasm-client-machine");
          }
        }
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {string} className
     * @param {string} classValue
     * @returns {number}
     */
    setWindowClassHint(id, className, classValue) {
      const resource = this.resources.get(id);
      if (resource) {
        resource.wmClassName = className;
        resource.wmClass = classValue;
        if (resource.element) {
          if (className) {
            resource.element.setAttribute("data-x11-wasm-class-name", className);
          } else {
            resource.element.removeAttribute("data-x11-wasm-class-name");
          }
          if (classValue) {
            resource.element.setAttribute("data-x11-wasm-class", classValue);
          } else {
            resource.element.removeAttribute("data-x11-wasm-class");
          }
        }
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} alpha
     * @returns {number}
     */
    setWindowAlpha(id, alpha) {
      const resource = this.resources.get(id);
      if (resource) {
        const clampedAlpha = Math.max(0, Math.min(1, alpha));
        resource.alpha = clampedAlpha;
        if (resource.element) {
          resource.element.style.opacity =
            clampedAlpha < 1 ? String(clampedAlpha) : "";
          resource.element.setAttribute("data-x11-wasm-alpha", String(clampedAlpha));
        }
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} flags
     * @param {number} input
     * @param {number} initialState
     * @param {number} iconPixmap
     * @param {number} iconWindow
     * @param {number} iconX
     * @param {number} iconY
     * @param {number} iconMask
     * @param {number} windowGroup
     * @returns {number}
     */
    setWindowWMHints(
      id,
      flags,
      input,
      initialState,
      iconPixmap,
      iconWindow,
      iconX,
      iconY,
      iconMask,
      windowGroup,
    ) {
      const resource = this.resources.get(id);
      if (resource) {
        const previousIconWindow = this.ownerIconWindow(resource);
        resource.wmHints =
          flags === 0
            ? null
            : {
                flags,
                input,
                initialState,
                iconPixmap,
                iconWindow,
                iconX,
                iconY,
                iconMask,
                windowGroup,
              };
        this.applyWindowWMHints(resource);
        this.refreshWindowWMHintRelations();
        this.syncIconWindowPresentation(previousIconWindow);
        this.syncIconWindowPresentation(this.ownerIconWindow(resource));
        this.setWindowIconBitmap(
          resource,
          flags & WM_ICON_PIXMAP_HINT ? iconPixmap : 0,
          flags & WM_ICON_MASK_HINT ? iconMask : 0,
        );
        this.syncDocumentIcon(resource);
      }
      return 0;
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applyWindowWMHints(resource) {
      const element = resource.element;
      if (!element) {
        return;
      }
      const hints = resource.wmHints;
      const hintAttributes = [
        "data-x11-wasm-wm-hints-flags",
        "data-x11-wasm-input-hint",
        "data-x11-wasm-input-eligible",
        "data-x11-wasm-initial-state",
        "data-x11-wasm-initial-state-name",
        "data-x11-wasm-icon-pixmap",
        "data-x11-wasm-icon-window",
        "data-x11-wasm-icon-x",
        "data-x11-wasm-icon-y",
        "data-x11-wasm-icon-mask",
        "data-x11-wasm-window-group",
      ];

      if (!hints) {
        for (const attribute of hintAttributes) {
          element.removeAttribute(attribute);
        }
        return;
      }

      const flags = hints.flags;
      const setHint = (attribute, enabled, value) => {
        if (enabled && value !== "") {
          element.setAttribute(attribute, String(value));
        } else {
          element.removeAttribute(attribute);
        }
      };

      element.setAttribute("data-x11-wasm-wm-hints-flags", String(flags));
      setHint("data-x11-wasm-input-hint", flags & WM_INPUT_HINT, hints.input);
      setHint(
        "data-x11-wasm-input-eligible",
        flags & WM_INPUT_HINT,
        hints.input ? 1 : 0,
      );
      setHint(
        "data-x11-wasm-initial-state",
        flags & WM_STATE_HINT,
        hints.initialState,
      );
      setHint(
        "data-x11-wasm-initial-state-name",
        flags & WM_STATE_HINT,
        wmStateName(hints.initialState),
      );
      setHint(
        "data-x11-wasm-icon-pixmap",
        flags & WM_ICON_PIXMAP_HINT,
        hints.iconPixmap,
      );
      setHint(
        "data-x11-wasm-icon-window",
        flags & WM_ICON_WINDOW_HINT,
        hints.iconWindow,
      );
      setHint("data-x11-wasm-icon-x", flags & WM_ICON_POSITION_HINT, hints.iconX);
      setHint("data-x11-wasm-icon-y", flags & WM_ICON_POSITION_HINT, hints.iconY);
      setHint("data-x11-wasm-icon-mask", flags & WM_ICON_MASK_HINT, hints.iconMask);
      setHint(
        "data-x11-wasm-window-group",
        flags & WM_WINDOW_GROUP_HINT,
        hints.windowGroup,
      );
    },

refreshWindowWMHintRelations() {
      /** @type {Map<number, number[]>} */
      const iconOwners = new Map();
      /** @type {Map<number, number[]>} */
      const groupMembers = new Map();

      for (const resource of this.resources.values()) {
        resource.element?.removeAttribute("data-x11-wasm-icon-owner");
        resource.element?.removeAttribute("data-x11-wasm-window-group-members");
      }
      for (const resource of this.resources.values()) {
        const hints = resource.wmHints;
        if (!hints) {
          continue;
        }
        if ((hints.flags & WM_ICON_WINDOW_HINT) && hints.iconWindow) {
          const owners = iconOwners.get(hints.iconWindow) || [];
          owners.push(resource.id);
          iconOwners.set(hints.iconWindow, owners);
        }
        if ((hints.flags & WM_WINDOW_GROUP_HINT) && hints.windowGroup) {
          const members = groupMembers.get(hints.windowGroup) || [];
          members.push(resource.id);
          groupMembers.set(hints.windowGroup, members);
        }
      }
      for (const [iconWindow, owners] of iconOwners.entries()) {
        const resource = this.resources.get(iconWindow);
        resource?.element?.setAttribute(
          "data-x11-wasm-icon-owner",
          owners.join(" "),
        );
      }
      for (const [windowGroup, members] of groupMembers.entries()) {
        const resource = this.resources.get(windowGroup);
        resource?.element?.setAttribute(
          "data-x11-wasm-window-group-members",
          members.join(" "),
        );
      }
    },

    /**
     * @param {X11WasmDOMResource | null | undefined} resource
     * @returns {number}
     */
    ownerIconWindow(resource) {
      const hints = resource?.wmHints;
      if (!hints || !(hints.flags & WM_ICON_WINDOW_HINT)) {
        return 0;
      }
      return unsignedInt(hints.iconWindow, 0);
    },

    /**
     * @param {number} iconWindow
     */
    syncIconWindowPresentation(iconWindow) {
      if (!iconWindow) {
        return;
      }
      const icon = this.resources.get(iconWindow);
      if (icon && icon.kind === "window") {
        this.applyGeometry(icon);
      }
    },

    /**
     * @param {X11WasmDOMResource} owner
     */
    syncIconWindowPresentationForOwner(owner) {
      this.syncIconWindowPresentation(this.ownerIconWindow(owner));
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @returns {X11WasmDOMResource | null}
     */
    activeIconProxyOwner(resource) {
      if (resource.kind !== "window") {
        return null;
      }
      for (const owner of this.resources.values()) {
        if (owner === resource || owner.kind !== "window") {
          continue;
        }
        const hints = owner.wmHints;
        if (
          !hints ||
          !(hints.flags & WM_ICON_WINDOW_HINT) ||
          hints.iconWindow !== resource.id
        ) {
          continue;
        }
        if (owner.wmState === "iconic") {
          return owner;
        }
      }
      return null;
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {number} iconPixmap
     * @param {number} iconMask
     */
    setWindowIconBitmap(resource, iconPixmap, iconMask) {
      const icon = iconPixmap
        ? this.iconPixmapDataUrl(iconPixmap, iconMask)
        : null;
      resource.iconBitmapHref = icon ? icon.href : "";
      resource.iconBitmapWidth = icon ? icon.width : 0;
      resource.iconBitmapHeight = icon ? icon.height : 0;

      const element = resource.element;
      if (!element) {
        return;
      }
      if (icon) {
        element.setAttribute("data-x11-wasm-icon-bitmap-href", icon.href);
        element.setAttribute("data-x11-wasm-icon-bitmap-width", String(icon.width));
        element.setAttribute("data-x11-wasm-icon-bitmap-height", String(icon.height));
      } else {
        element.removeAttribute("data-x11-wasm-icon-bitmap-href");
        element.removeAttribute("data-x11-wasm-icon-bitmap-width");
        element.removeAttribute("data-x11-wasm-icon-bitmap-height");
      }
    },

    /**
     * @param {number} iconPixmap
     * @param {number} iconMask
     * @returns {X11WasmDOMIconBitmap | null}
     */
    iconPixmapDataUrl(iconPixmap, iconMask) {
      const pixmap = this.resources.get(iconPixmap);
      if (
        !this.document ||
        !pixmap ||
        pixmap.kind !== "pixmap" ||
        !pixmap.canvas ||
        !pixmap.context ||
        pixmap.width <= 0 ||
        pixmap.height <= 0
      ) {
        return null;
      }
      if (pixmap.geometryDirty) {
        this.flushResourceGeometry(pixmap);
      }

      const width = pixmap.width;
      const height = pixmap.height;
      if (!iconMask) {
        const href = this.canvasDataUrl(pixmap.canvas, width, height);
        return href ? { href, width, height } : null;
      }

      const canvas = this.document.createElement("canvas");
      canvas.width = width;
      canvas.height = height;
      const context = canvas.getContext("2d");
      if (!context) {
        return null;
      }
      context.drawImage(pixmap.canvas, 0, 0);

      const mask = this.resources.get(iconMask);
      const maskContext = mask && mask.context;
      if (
        !mask ||
        mask.kind !== "pixmap" ||
        !maskContext ||
        mask.width <= 0 ||
        mask.height <= 0
      ) {
        const href = canvas.toDataURL("image/png") || "";
        return href ? { href, width, height } : null;
      }
      if (mask.geometryDirty) {
        this.flushResourceGeometry(mask);
      }

      const imageData = context.getImageData(0, 0, width, height);
      const maskData = maskContext.getImageData(0, 0, mask.width, mask.height)
        .data;
      for (let row = 0; row < height; row++) {
        for (let col = 0; col < width; col++) {
          const targetOffset = (row * width + col) * 4;
          const maskOffset = (row * mask.width + col) * 4;
          const masked =
            row < mask.height &&
            col < mask.width &&
            rgbaPixelHasBit(maskData, maskOffset);
          if (!masked) {
            imageData.data[targetOffset + 3] = 0;
          }
        }
      }
      context.putImageData(imageData, 0, 0);

      const href = canvas.toDataURL("image/png") || "";
      return href ? { href, width, height } : null;
    },

    /**
     * @param {number} id
     * @param {string} sizes
     * @param {number} count
     * @returns {number}
     */
    setWindowIconSizes(id, sizes, count) {
      const resource = this.resources.get(id);
      if (resource) {
        const iconSizeCount = Math.max(0, Math.trunc(count));
        resource.iconSizeCount = iconSizeCount;
        resource.iconSizes = iconSizeCount > 0 ? sizes : "";
        if (resource.element) {
          if (iconSizeCount > 0) {
            resource.element.setAttribute(
              "data-x11-wasm-icon-size-count",
              String(iconSizeCount),
            );
            resource.element.setAttribute("data-x11-wasm-icon-sizes", sizes);
          } else {
            resource.element.removeAttribute("data-x11-wasm-icon-size-count");
            resource.element.removeAttribute("data-x11-wasm-icon-sizes");
          }
        }
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} imageCount
     * @param {string} sizes
     * @param {Uint8Array | null} rgba
     * @param {number} width
     * @param {number} height
     * @returns {number}
     */
    setWindowIconPhoto(id, imageCount, sizes, rgba, width, height) {
      const resource = this.resources.get(id);
      if (resource) {
        const href =
          imageCount > 0 && rgba
            ? this.iconPhotoDataUrl(rgba, width, height)
            : "";
        resource.iconPhotoCount = imageCount;
        resource.iconPhotoSizes = sizes;
        resource.iconPhotoHref = href;
        resource.iconPhotoWidth = href ? positiveInt(width, 0) : 0;
        resource.iconPhotoHeight = href ? positiveInt(height, 0) : 0;
        if (resource.element) {
          if (imageCount > 0) {
            resource.element.setAttribute(
              "data-x11-wasm-icon-photo-count",
              String(imageCount),
            );
            resource.element.setAttribute("data-x11-wasm-icon-photo-sizes", sizes);
            if (href) {
              resource.element.setAttribute("data-x11-wasm-icon-photo-href", href);
              resource.element.setAttribute(
                "data-x11-wasm-icon-photo-width",
                String(resource.iconPhotoWidth),
              );
              resource.element.setAttribute(
                "data-x11-wasm-icon-photo-height",
                String(resource.iconPhotoHeight),
              );
            } else {
              resource.element.removeAttribute("data-x11-wasm-icon-photo-href");
              resource.element.removeAttribute("data-x11-wasm-icon-photo-width");
              resource.element.removeAttribute("data-x11-wasm-icon-photo-height");
            }
          } else {
            resource.element.removeAttribute("data-x11-wasm-icon-photo-count");
            resource.element.removeAttribute("data-x11-wasm-icon-photo-sizes");
            resource.element.removeAttribute("data-x11-wasm-icon-photo-href");
            resource.element.removeAttribute("data-x11-wasm-icon-photo-width");
            resource.element.removeAttribute("data-x11-wasm-icon-photo-height");
          }
        }
        this.syncDocumentIcon(resource);
      }
      return 0;
    },

    /**
     * @param {Uint8Array} rgba
     * @param {number} width
     * @param {number} height
     * @returns {string}
     */
    iconPhotoDataUrl(rgba, width, height) {
      if (!this.document) {
        return "";
      }
      const imageWidth = positiveInt(width, 0);
      const imageHeight = positiveInt(height, 0);
      if (imageWidth <= 0 || imageHeight <= 0) {
        return "";
      }
      const canvas = this.document.createElement("canvas");
      canvas.width = imageWidth;
      canvas.height = imageHeight;
      const context = canvas.getContext("2d");
      if (!context) {
        return "";
      }
      const imageData = context.createImageData(imageWidth, imageHeight);
      imageData.data.set(rgba.subarray(0, imageData.data.length));
      context.putImageData(imageData, 0, 0);
      return canvas.toDataURL("image/png") || "";
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    syncDocumentIcon(resource) {
      this.updateDocumentIcon(resource.iconPhotoHref || resource.iconBitmapHref);
    },

syncDocumentIconFromResources() {
      let fallbackHref = "";
      for (const resource of this.resources.values()) {
        if (resource.kind !== "window" && resource.kind !== "root") {
          continue;
        }
        if (resource.iconPhotoHref) {
          this.updateDocumentIcon(resource.iconPhotoHref);
          return;
        }
        if (!fallbackHref && resource.iconBitmapHref) {
          fallbackHref = resource.iconBitmapHref;
        }
      }
      this.updateDocumentIcon(fallbackHref);
    },

    /**
     * @param {string} href
     */
    updateDocumentIcon(href) {
      const document = this.document;
      if (!document) {
        return;
      }
      let link = document.querySelector("link[data-x11-wasm-icon]");
      if (!href) {
        link?.remove();
        return;
      }
      if (!(link instanceof HTMLLinkElement)) {
        link = document.createElement("link");
        link.setAttribute("data-x11-wasm-icon", "1");
        document.head.appendChild(link);
      }
      link.setAttribute("rel", "icon");
      link.setAttribute("type", "image/png");
      link.setAttribute("href", href);
    },

    /**
     * @param {number} id
     * @param {string} protocols
     * @returns {number}
     */
    setWindowProtocols(id, protocols) {
      const resource = this.resources.get(id);
      if (resource) {
        const names = protocols ? protocols.split(/\s+/).filter(Boolean) : [];
        resource.protocols = names;
        if (resource.element) {
          if (names.length > 0) {
            resource.element.setAttribute("data-x11-wasm-protocols", names.join(" "));
          } else {
            resource.element.removeAttribute("data-x11-wasm-protocols");
          }
        }
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {string} types
     * @returns {number}
     */
    setWindowTypes(id, types) {
      const resource = this.resources.get(id);
      if (resource) {
        const names = types ? types.split(/\s+/).filter(Boolean) : [];
        resource.windowTypes = names;
        if (resource.element) {
          if (names.length > 0) {
            resource.element.setAttribute(
              "data-x11-wasm-window-types",
              names.join(" "),
            );
          } else {
            resource.element.removeAttribute("data-x11-wasm-window-types");
          }
        }
      }
      return 0;
    },

    /**
     * @param {X11WasmDOMResource | undefined} resource
     */
    installWindowCloseListener(resource) {
      if (!resource || !resource.element || resource.closeEventListenerInstalled) {
        return;
      }
      resource.closeEventListenerInstalled = true;
      resource.element.addEventListener("x11wasmwindowclose", (event) => {
        const detail =
          "detail" in event && event.detail && typeof event.detail === "object"
            ? /** @type {{ time?: unknown }} */ (event.detail)
            : null;
        const rawTime =
          detail && detail.time !== undefined
            ? Number(detail.time)
            : event.timeStamp;
        if (this.requestWindowClose(resource.id, eventTime(rawTime))) {
          event.preventDefault();
        }
      });
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applyWindowCloseRequestState(resource) {
      const element = resource.element;
      if (!element) {
        return;
      }
      element.setAttribute(
        "data-x11-wasm-close-request-count",
        String(resource.closeRequestCount),
      );
      element.setAttribute(
        "data-x11-wasm-close-request-time",
        String(resource.lastCloseRequestTime),
      );
      element.setAttribute(
        "data-x11-wasm-close-request-status",
        resource.closeRequestStatus,
      );
    },

    /**
     * @param {number} id
     * @param {string} protocol
     * @param {number=} time
     * @returns {number}
     */
    dispatchWindowProtocol(id, protocol, time = 0) {
      const resource = this.resources.get(id);
      const queue = this.module && this.module["_X11WasmQueueWindowProtocol"];

      if (
        !resource ||
        !protocol ||
        !resource.protocols.includes(protocol) ||
        typeof queue !== "function"
      ) {
        return 0;
      }
      return this.withUtf8String(protocol, (pointer, byteLength) =>
        queue(id, pointer, byteLength, positiveInt(time, 0)),
      );
    },

    /**
     * @param {number} id
     * @param {number=} time
     * @returns {number}
     */
    requestWindowClose(id, time = 0) {
      const resource = this.resources.get(id);
      if (!resource) {
        return 0;
      }
      const requestTime = positiveInt(time, 0);
      const result = this.dispatchWindowProtocol(
        id,
        "WM_DELETE_WINDOW",
        requestTime,
      );
      resource.closeRequestCount += 1;
      resource.lastCloseRequestTime = requestTime;
      resource.closeRequestStatus = result ? "dispatched" : "ignored";
      this.applyWindowCloseRequestState(resource);
      return result;
    },

    /**
     * @param {number} id
     * @param {string} colormapWindows
     * @returns {number}
     */
    setWindowColormapWindows(id, colormapWindows) {
      const resource = this.resources.get(id);
      if (resource) {
        const windowIds = colormapWindows
          ? colormapWindows.split(/\s+/).filter(Boolean)
          : [];
        resource.colormapWindows = windowIds;
        if (resource.element) {
          if (windowIds.length > 0) {
            resource.element.setAttribute(
              "data-x11-wasm-colormap-windows",
              windowIds.join(" "),
            );
            resource.element.setAttribute(
              "data-x11-wasm-colormap-window-count",
              String(windowIds.length),
            );
          } else {
            resource.element.removeAttribute("data-x11-wasm-colormap-windows");
            resource.element.removeAttribute(
              "data-x11-wasm-colormap-window-count",
            );
          }
        }
      }
      return 0;
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applyStandardColormapAttributes(resource) {
      const element = resource.element;
      if (!element) {
        return;
      }
      const names = Object.keys(resource.standardColormaps).sort();
      if (names.length === 0) {
        element.removeAttribute("data-x11-wasm-standard-colormap-count");
        element.removeAttribute("data-x11-wasm-standard-colormaps");
        return;
      }

      const snapshot = Object.create(null);
      for (const name of names) {
        snapshot[name] = resource.standardColormaps[name];
      }
      element.setAttribute(
        "data-x11-wasm-standard-colormap-count",
        String(names.length),
      );
      element.setAttribute(
        "data-x11-wasm-standard-colormaps",
        JSON.stringify(snapshot),
      );
    },

    /**
     * @param {number} id
     * @param {string} property
     * @param {string} maps
     * @param {number} count
     * @returns {number}
     */
    setWindowStandardColormaps(id, property, maps, count) {
      const resource = this.resources.get(id);
      const propertyName = String(property || "");
      if (!resource || !propertyName) {
        return 0;
      }

      const mapCount = Math.max(0, Math.trunc(count));
      if (mapCount > 0 && maps) {
        try {
          const parsed = JSON.parse(maps);
          resource.standardColormaps[propertyName] = Array.isArray(parsed)
            ? parsed.slice(0, mapCount)
            : [];
        } catch {
          resource.standardColormaps[propertyName] = [];
        }
      } else {
        delete resource.standardColormaps[propertyName];
      }
      if (resource.standardColormaps[propertyName]?.length === 0) {
        delete resource.standardColormaps[propertyName];
      }
      this.applyStandardColormapAttributes(resource);
      return 0;
    },

    installFullscreenEventListeners() {
      const document = this.document;
      if (!document || this.fullscreenChangeHandler) {
        return;
      }
      this.fullscreenChangeHandler = (event) => {
        this.handleFullscreenChange(event);
      };
      this.fullscreenErrorHandler = (event) => {
        this.handleFullscreenError(event);
      };
      document.addEventListener("fullscreenchange", this.fullscreenChangeHandler);
      document.addEventListener("fullscreenerror", this.fullscreenErrorHandler);
    },

    /**
     * @param {Element | null} element
     * @returns {X11WasmDOMResource | null}
     */
    fullscreenResourceForElement(element) {
      if (!element) {
        return null;
      }
      for (const resource of this.resources.values()) {
        if (resource.element === element) {
          return resource;
        }
      }
      return null;
    },

    /**
     * @param {X11WasmDOMResource} resource
     */
    applyWindowStateAttributes(resource) {
      const element = resource.element;
      if (!element) {
        return;
      }
      const setBooleanAttribute = (attribute, value) => {
        if (value) {
          element.setAttribute(attribute, "1");
        } else {
          element.removeAttribute(attribute);
        }
      };
      setBooleanAttribute("data-x11-wasm-topmost", resource.topmost);
      setBooleanAttribute("data-x11-wasm-zoomed", resource.zoomed);
      setBooleanAttribute("data-x11-wasm-fullscreen", resource.fullscreen);
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {boolean} fullscreen
     * @param {number=} time
     * @returns {number}
     */
    queueFullscreenState(resource, fullscreen, time = 0) {
      const queue = this.module && this.module["_X11WasmQueueFullscreenState"];
      if (typeof queue !== "function") {
        this.fullscreenRequestStatus = "queue-missing";
        return 0;
      }
      return queue(resource.id, fullscreen ? 1 : 0, positiveInt(time, 0)) ? 1 : 0;
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {boolean} previousFullscreen
     */
    syncWindowFullscreen(resource, previousFullscreen) {
      const document = this.document;
      const element = resource.element;
      if (!document || !element) {
        return;
      }
      const activeElement = document.fullscreenElement || null;
      if (resource.fullscreen) {
        this.fullscreenRequestWindow = resource.id;
        if (activeElement === element) {
          this.fullscreenElement = element;
          this.fullscreenRequestStatus = "active";
          this.fullscreenRequestError = "";
          return;
        }
        if (
          previousFullscreen &&
          this.fullscreenRequestWindow === resource.id &&
          this.fullscreenRequestStatus !== "exited"
        ) {
          return;
        }
        if (typeof element.requestFullscreen !== "function") {
          this.fullscreenRequestStatus = "unsupported";
          this.fullscreenRequestError = "Element.requestFullscreen unavailable";
          return;
        }
        this.fullscreenRequestCount += 1;
        this.fullscreenRequestStatus = "requested";
        this.fullscreenRequestError = "";
        try {
          Promise.resolve(element.requestFullscreen()).catch((error) => {
            this.fullscreenRequestStatus = "error";
            this.fullscreenRequestError =
              error instanceof Error ? error.message : String(error);
          });
        } catch (error) {
          this.fullscreenRequestStatus = "error";
          this.fullscreenRequestError =
            error instanceof Error ? error.message : String(error);
        }
        return;
      }

      if (activeElement === element && typeof document.exitFullscreen === "function") {
        this.fullscreenExitCount += 1;
        this.fullscreenRequestStatus = "exit-requested";
        this.fullscreenRequestError = "";
        try {
          Promise.resolve(document.exitFullscreen()).catch((error) => {
            this.fullscreenRequestStatus = "error";
            this.fullscreenRequestError =
              error instanceof Error ? error.message : String(error);
          });
        } catch (error) {
          this.fullscreenRequestStatus = "error";
          this.fullscreenRequestError =
            error instanceof Error ? error.message : String(error);
        }
      } else if (this.fullscreenRequestWindow === resource.id) {
        this.fullscreenRequestWindow = 0;
        this.fullscreenElement = activeElement;
        this.fullscreenRequestStatus = "exited";
      }
    },

    /**
     * @param {Event} event
     */
    handleFullscreenChange(event) {
      const document = this.document;
      if (!document) {
        return;
      }
      const activeElement = document.fullscreenElement || null;
      const activeResource = this.fullscreenResourceForElement(activeElement);
      const previousWindow = this.fullscreenRequestWindow;
      this.fullscreenChangeCount += 1;
      this.fullscreenElement = activeElement;
      if (activeResource) {
        this.fullscreenRequestWindow = activeResource.id;
        this.fullscreenRequestStatus = "active";
        this.fullscreenRequestError = "";
        if (!activeResource.fullscreen) {
          this.queueFullscreenState(
            activeResource,
            true,
            eventTime(event.timeStamp),
          );
        }
        return;
      }
      if (previousWindow) {
        const resource = this.resources.get(previousWindow);
        if (resource?.fullscreen) {
          this.queueFullscreenState(resource, false, eventTime(event.timeStamp));
        }
      }
      this.fullscreenRequestWindow = 0;
      this.fullscreenRequestStatus = "exited";
    },

    /**
     * @param {Event} event
     */
    handleFullscreenError(event) {
      this.fullscreenErrorCount += 1;
      this.fullscreenRequestStatus = "error";
      this.fullscreenRequestError = event.type || "fullscreenerror";
    },

    /**
     * @param {number} id
     * @param {boolean} topmost
     * @param {boolean} zoomed
     * @param {boolean} fullscreen
     * @returns {number}
     */
    setWindowState(id, topmost, zoomed, fullscreen) {
      const resource = this.resources.get(id);
      if (resource) {
        const previousFullscreen = !!resource.fullscreen;
        resource.topmost = !!topmost;
        resource.zoomed = !!zoomed;
        resource.fullscreen = !!fullscreen;
        this.applyWindowStateAttributes(resource);
        this.applyGeometry(resource);
        this.applyTransientDescendantGeometry(resource);
        this.syncWindowFullscreen(resource, previousFullscreen);
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} state
     * @returns {number}
     */
    setWindowWmState(id, state) {
      const resource = this.resources.get(id);
      if (resource) {
        this.setResourceWmState(resource, wmStateName(state));
        this.applyGeometry(resource);
        this.syncIconWindowPresentationForOwner(resource);
      }
      return 0;
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {X11WasmDOMWmState} state
     */
    setResourceWmState(resource, state) {
      resource.wmState = state;
      if (!resource.element) {
        return;
      }
      if (resource.wmState) {
        resource.element.setAttribute("data-x11-wasm-wm-state", resource.wmState);
      } else {
        resource.element.removeAttribute("data-x11-wasm-wm-state");
      }
    },

    /**
     * @param {number} id
     * @param {number} flags
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @param {number} minWidth
     * @param {number} minHeight
     * @param {number} maxWidth
     * @param {number} maxHeight
     * @param {number} widthInc
     * @param {number} heightInc
     * @param {number} minAspectX
     * @param {number} minAspectY
     * @param {number} maxAspectX
     * @param {number} maxAspectY
     * @param {number} baseWidth
     * @param {number} baseHeight
     * @param {number} winGravity
     * @returns {number}
     */
    setWindowSizeHints(
      id,
      flags,
      x,
      y,
      width,
      height,
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      widthInc,
      heightInc,
      minAspectX,
      minAspectY,
      maxAspectX,
      maxAspectY,
      baseWidth,
      baseHeight,
      winGravity,
    ) {
      const resource = this.resources.get(id);
      if (resource) {
        if (flags === 0) {
          resource.sizeHints = null;
          this.clearWindowSizeHintAttributes(
            resource,
            NORMAL_SIZE_HINT_ATTRIBUTES,
          );
          this.applyWindowAttributes(resource);
          this.applySizeHints(resource);
          return 0;
        }
        resource.sizeHints = this.makeWindowSizeHints(
          flags,
          x,
          y,
          width,
          height,
          minWidth,
          minHeight,
          maxWidth,
          maxHeight,
          widthInc,
          heightInc,
          minAspectX,
          minAspectY,
          maxAspectX,
          maxAspectY,
          baseWidth,
          baseHeight,
          winGravity,
        );
        this.applyWindowSizeHintAttributes(
          resource,
          resource.sizeHints,
          NORMAL_SIZE_HINT_ATTRIBUTES,
        );
        this.applySizeHints(resource);
      }
      return 0;
    },

    /**
     * @param {number} id
     * @param {number} flags
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @param {number} minWidth
     * @param {number} minHeight
     * @param {number} maxWidth
     * @param {number} maxHeight
     * @param {number} widthInc
     * @param {number} heightInc
     * @param {number} minAspectX
     * @param {number} minAspectY
     * @param {number} maxAspectX
     * @param {number} maxAspectY
     * @param {number} baseWidth
     * @param {number} baseHeight
     * @param {number} winGravity
     * @returns {number}
     */
    setWindowZoomHints(
      id,
      flags,
      x,
      y,
      width,
      height,
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      widthInc,
      heightInc,
      minAspectX,
      minAspectY,
      maxAspectX,
      maxAspectY,
      baseWidth,
      baseHeight,
      winGravity,
    ) {
      const resource = this.resources.get(id);
      if (resource) {
        if (flags === 0) {
          resource.zoomHints = null;
          this.clearWindowSizeHintAttributes(
            resource,
            ZOOM_SIZE_HINT_ATTRIBUTES,
          );
          this.applyGeometry(resource);
          this.applyTransientDescendantGeometry(resource);
          return 0;
        }
        resource.zoomHints = this.makeWindowSizeHints(
          flags,
          x,
          y,
          width,
          height,
          minWidth,
          minHeight,
          maxWidth,
          maxHeight,
          widthInc,
          heightInc,
          minAspectX,
          minAspectY,
          maxAspectX,
          maxAspectY,
          baseWidth,
          baseHeight,
          winGravity,
        );
        this.applyWindowSizeHintAttributes(
          resource,
          resource.zoomHints,
          ZOOM_SIZE_HINT_ATTRIBUTES,
        );
        this.applyGeometry(resource);
        this.applyTransientDescendantGeometry(resource);
      }
      return 0;
    },

    /**
     * @param {number} flags
     * @param {number} x
     * @param {number} y
     * @param {number} width
     * @param {number} height
     * @param {number} minWidth
     * @param {number} minHeight
     * @param {number} maxWidth
     * @param {number} maxHeight
     * @param {number} widthInc
     * @param {number} heightInc
     * @param {number} minAspectX
     * @param {number} minAspectY
     * @param {number} maxAspectX
     * @param {number} maxAspectY
     * @param {number} baseWidth
     * @param {number} baseHeight
     * @param {number} winGravity
     * @returns {X11WasmDOMSizeHints}
     */
    makeWindowSizeHints(
      flags,
      x,
      y,
      width,
      height,
      minWidth,
      minHeight,
      maxWidth,
      maxHeight,
      widthInc,
      heightInc,
      minAspectX,
      minAspectY,
      maxAspectX,
      maxAspectY,
      baseWidth,
      baseHeight,
      winGravity,
    ) {
      return {
        flags,
        x,
        y,
        width,
        height,
        minWidth,
        minHeight,
        maxWidth,
        maxHeight,
        widthInc,
        heightInc,
        minAspectX,
        minAspectY,
        maxAspectX,
        maxAspectY,
        baseWidth,
        baseHeight,
        winGravity,
      };
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {X11WasmDOMSizeHints | null} hints
     * @param {Record<string, string>} attributes
     */
    applyWindowSizeHintAttributes(resource, hints, attributes) {
      const element = resource.element;
      if (!element) {
        return;
      }
      if (!hints) {
        this.clearWindowSizeHintAttributes(resource, attributes);
        return;
      }
      const flags = hints.flags;
      const setHint = (attribute, enabled, value) => {
        if (enabled) {
          element.setAttribute(attribute, String(value));
        } else {
          element.removeAttribute(attribute);
        }
      };
      element.setAttribute(attributes.flags, String(flags));
      setHint(attributes.x, flags & P_POSITION, hints.x);
      setHint(attributes.y, flags & P_POSITION, hints.y);
      setHint(attributes.width, flags & P_SIZE, hints.width);
      setHint(attributes.height, flags & P_SIZE, hints.height);
      setHint(attributes.minWidth, flags & P_MIN_SIZE, hints.minWidth);
      setHint(attributes.minHeight, flags & P_MIN_SIZE, hints.minHeight);
      setHint(attributes.maxWidth, flags & P_MAX_SIZE, hints.maxWidth);
      setHint(attributes.maxHeight, flags & P_MAX_SIZE, hints.maxHeight);
      setHint(attributes.widthInc, flags & P_RESIZE_INC, hints.widthInc);
      setHint(attributes.heightInc, flags & P_RESIZE_INC, hints.heightInc);
      setHint(attributes.minAspectX, flags & P_ASPECT, hints.minAspectX);
      setHint(attributes.minAspectY, flags & P_ASPECT, hints.minAspectY);
      setHint(attributes.maxAspectX, flags & P_ASPECT, hints.maxAspectX);
      setHint(attributes.maxAspectY, flags & P_ASPECT, hints.maxAspectY);
      setHint(attributes.baseWidth, flags & P_BASE_SIZE, hints.baseWidth);
      setHint(attributes.baseHeight, flags & P_BASE_SIZE, hints.baseHeight);
      setHint(attributes.winGravity, flags & P_WIN_GRAVITY, hints.winGravity);
    },

    /**
     * @param {X11WasmDOMResource} resource
     * @param {Record<string, string>} attributes
     */
    clearWindowSizeHintAttributes(resource, attributes) {
      const element = resource.element;
      if (!element) {
        return;
      }
      for (const attribute of Object.values(attributes)) {
        element.removeAttribute(attribute);
      }
    }
  }));
})(typeof globalThis !== "undefined" ? globalThis : window);
