// @ts-check

const {
  createMappedWindow,
  createMappedWindows,
  expect,
  fs,
  test,
  waitForX11Runtime,
  X11_WASM_DOM_SOURCES,
  X11_WASM_JS_LIBRARY_SOURCES,
} = require("../support/x11");

test("XConfigureWindow honors conditional stacking modes", async ({ page }) => {
  await waitForX11Runtime(page);

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestConfigureStackingApis?: () => number } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestConfigureStackingApis) {
      throw new Error("missing XConfigureWindow stacking test export");
    }
    return module._X11WasmTestConfigureStackingApis();
  });

  expect(result).toBe(1);
});

test("structure notifications split child and parent masks", async ({ page }) => {
  await waitForX11Runtime(page);

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSubstructureNotifyApis?: () => number } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestSubstructureNotifyApis) {
      throw new Error("missing substructure notify test export");
    }
    return module._X11WasmTestSubstructureNotifyApis();
  });

  expect(result).toBe(1);
});

test("XMapWindow exposes newly viewable mapped inferiors", async ({ page }) => {
  await waitForX11Runtime(page);

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestMapViewableExposeApis?: () => number } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestMapViewableExposeApis) {
      throw new Error("missing map viewable expose test export");
    }
    return module._X11WasmTestMapViewableExposeApis();
  });

  expect(result).toBe(1);
});

test("XMapWindow redirects map requests to selecting parents", async ({
  page,
}) => {
  await waitForX11Runtime(page);

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestMapRequestRedirectApis?: () => number } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestMapRequestRedirectApis) {
      throw new Error("missing map request redirect test export");
    }
    return module._X11WasmTestMapRequestRedirectApis();
  });

  expect(result).toBe(1);
});

test("XSetIconSizes updates browser WM_ICON_SIZE metadata", async ({ page }) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const iconSizes = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetWindowIconSizes?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (!module?._X11WasmTestSetWindowIconSizes || !backend) {
      throw new Error("missing XSetIconSizes test hooks");
    }

    const setResult = module._X11WasmTestSetWindowIconSizes(windowId, 1);
    const resource = backend.resources.get(windowId);
    const element = resource?.element;
    if (!(element instanceof HTMLElement) || !resource) {
      throw new Error("missing icon size test element");
    }
    const afterSet = {
      attrCount: element.getAttribute("data-x11-wasm-icon-size-count"),
      attrSizes: element.getAttribute("data-x11-wasm-icon-sizes"),
      resourceCount: resource.iconSizeCount,
      resourceSizes: resource.iconSizes,
    };
    const clearResult = module._X11WasmTestSetWindowIconSizes(windowId, 0);
    const afterClear = {
      attrCount: element.getAttribute("data-x11-wasm-icon-size-count"),
      attrSizes: element.getAttribute("data-x11-wasm-icon-sizes"),
      resourceCount: resource.iconSizeCount,
      resourceSizes: resource.iconSizes,
    };
    return { afterClear, afterSet, clearResult, setResult };
  }, windowId);

  expect(iconSizes).toEqual({
    setResult: 0,
    clearResult: 0,
    afterSet: {
      attrCount: "2",
      attrSizes: "16x16-64x64+8+8 32x32-128x128+16+16",
      resourceCount: 2,
      resourceSizes: "16x16-64x64+8+8 32x32-128x128+16+16",
    },
    afterClear: {
      attrCount: null,
      attrSizes: null,
      resourceCount: 0,
      resourceSizes: "",
    },
  });
});

test("XSetWMColormapWindows updates browser WM_COLORMAP_WINDOWS metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const [ownerId, secondaryId] = await createMappedWindows(page, 2);

  const metadata = await page.evaluate(
    ({ ownerId, secondaryId }) => {
      const module =
        /** @type {Window & { Module?: { _X11WasmTestSetWMColormapWindows?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
          window
        ).Module;
      const backend = module?.x11WasmBackend;
      const resource = backend?.resources.get(ownerId);
      const element = resource?.element;
      if (
        !module?._X11WasmTestSetWMColormapWindows ||
        !resource ||
        !(element instanceof HTMLElement)
      ) {
        throw new Error("missing WM_COLORMAP_WINDOWS test hooks");
      }

      const setResult = module._X11WasmTestSetWMColormapWindows(
        ownerId,
        secondaryId,
        1,
      );
      const afterSet = {
        attrCount: element.getAttribute(
          "data-x11-wasm-colormap-window-count",
        ),
        attrWindows: element.getAttribute("data-x11-wasm-colormap-windows"),
        resourceWindows: [...(resource.colormapWindows ?? [])],
      };
      const clearResult = module._X11WasmTestSetWMColormapWindows(
        ownerId,
        secondaryId,
        0,
      );
      const afterClear = {
        attrCount: element.getAttribute(
          "data-x11-wasm-colormap-window-count",
        ),
        attrWindows: element.getAttribute("data-x11-wasm-colormap-windows"),
        resourceWindows: [...(resource.colormapWindows ?? [])],
      };
      return { afterClear, afterSet, clearResult, setResult };
    },
    { ownerId, secondaryId },
  );

  expect(metadata).toEqual({
    setResult: 1,
    clearResult: 1,
    afterSet: {
      attrCount: "2",
      attrWindows: `${ownerId} ${secondaryId}`,
      resourceWindows: [String(ownerId), String(secondaryId)],
    },
    afterClear: {
      attrCount: null,
      attrWindows: null,
      resourceWindows: [],
    },
  });
});

test("_NET_WM_ICON honors root WM_ICON_SIZE when choosing favicon", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const icon = await page.evaluate(async () => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetRootIconSizeAndWindowIcon?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (!module?._X11WasmTestSetRootIconSizeAndWindowIcon || !backend) {
      throw new Error("missing root icon size test hook");
    }

    const windowId = module._X11WasmTestSetRootIconSizeAndWindowIcon();
    const resource = backend.resources.get(windowId);
    const element = resource?.element;
    if (!(element instanceof HTMLElement) || !resource) {
      throw new Error("missing root icon size test element");
    }
    const href = element.getAttribute("data-x11-wasm-icon-photo-href") || "";
    const image = new Image();
    image.src = href;
    await new Promise((resolve, reject) => {
      image.onload = resolve;
      image.onerror = reject;
    });
    const canvas = document.createElement("canvas");
    canvas.width = image.naturalWidth;
    canvas.height = image.naturalHeight;
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing root icon size canvas context");
    }
    context.drawImage(image, 0, 0);
    const center = Array.from(
      context.getImageData(
        Math.floor(image.naturalWidth / 2),
        Math.floor(image.naturalHeight / 2),
        1,
        1,
      ).data,
    );
    const documentIcon = document.querySelector("link[data-x11-wasm-icon]");
    return {
      count: element.getAttribute("data-x11-wasm-icon-photo-count"),
      documentHref: documentIcon?.getAttribute("href") || "",
      href,
      naturalHeight: image.naturalHeight,
      naturalWidth: image.naturalWidth,
      sizes: element.getAttribute("data-x11-wasm-icon-photo-sizes"),
      windowId,
      center,
    };
  });

  expect(icon.windowId).toBeGreaterThan(0);
  expect(icon.count).toBe("2");
  expect(icon.sizes).toBe("16x16 32x32");
  expect(icon.href).toContain("data:image/png");
  expect(icon.documentHref).toBe(icon.href);
  expect(icon.naturalWidth).toBe(16);
  expect(icon.naturalHeight).toBe(16);
  expect(icon.center).toEqual([255, 0, 0, 255]);
});

test("ICCCM setters update browser metadata through window properties", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetWindowMetadataProperties?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (!module?._X11WasmTestSetWindowMetadataProperties || !backend) {
      throw new Error("missing ICCCM metadata test hooks");
    }

    const snapshot = () => {
      const resource = backend.resources.get(windowId);
      const element = resource?.element;
      if (!(element instanceof HTMLElement) || !resource) {
        throw new Error("missing ICCCM metadata test element");
      }
      return {
        attrs: {
          title: element.getAttribute("data-x11-wasm-title"),
          iconName: element.getAttribute("data-x11-wasm-icon-name"),
          command: element.getAttribute("data-x11-wasm-command"),
          className: element.getAttribute("data-x11-wasm-class-name"),
          classValue: element.getAttribute("data-x11-wasm-class"),
          clientMachine: element.getAttribute("data-x11-wasm-client-machine"),
          transientFor: element.getAttribute("data-x11-wasm-transient-for"),
          wmHintsFlags: element.getAttribute("data-x11-wasm-wm-hints-flags"),
          inputEligible: element.getAttribute("data-x11-wasm-input-eligible"),
          initialState: element.getAttribute(
            "data-x11-wasm-initial-state-name",
          ),
          iconX: element.getAttribute("data-x11-wasm-icon-x"),
          iconY: element.getAttribute("data-x11-wasm-icon-y"),
          sizeFlags: element.getAttribute("data-x11-wasm-size-hints-flags"),
          sizeX: element.getAttribute("data-x11-wasm-hint-x"),
          sizeY: element.getAttribute("data-x11-wasm-hint-y"),
          sizeWidth: element.getAttribute("data-x11-wasm-hint-width"),
          sizeHeight: element.getAttribute("data-x11-wasm-hint-height"),
          minWidth: element.getAttribute("data-x11-wasm-min-width"),
          minHeight: element.getAttribute("data-x11-wasm-min-height"),
          maxWidth: element.getAttribute("data-x11-wasm-max-width"),
          maxHeight: element.getAttribute("data-x11-wasm-max-height"),
          widthInc: element.getAttribute("data-x11-wasm-width-inc"),
          heightInc: element.getAttribute("data-x11-wasm-height-inc"),
          baseWidth: element.getAttribute("data-x11-wasm-base-width"),
          baseHeight: element.getAttribute("data-x11-wasm-base-height"),
          winGravity: element.getAttribute("data-x11-wasm-win-gravity"),
        },
        resource: {
          title: resource.title,
          iconName: resource.iconName,
          command: resource.command,
          className: resource.wmClassName,
          classValue: resource.wmClass,
          clientMachine: resource.clientMachine,
          transientFor: resource.transientFor,
          wmHints: resource.wmHints
            ? JSON.parse(JSON.stringify(resource.wmHints))
            : null,
          sizeHints: resource.sizeHints
            ? JSON.parse(JSON.stringify(resource.sizeHints))
            : null,
        },
      };
    };

    const setResult = module._X11WasmTestSetWindowMetadataProperties(
      windowId,
      1,
    );
    backend.flush?.();
    const afterSet = snapshot();
    const clearResult = module._X11WasmTestSetWindowMetadataProperties(
      windowId,
      0,
    );
    backend.flush?.();
    const afterClear = snapshot();
    return {
      afterClear,
      afterSet,
      clearResult,
      rootWindow: backend.rootWindow,
      setResult,
    };
  }, windowId);

  expect(metadata.setResult).toBe(1);
  expect(metadata.clearResult).toBe(1);
  expect(metadata.afterSet.attrs).toEqual({
    title: "X11Wasm Metadata Title",
    iconName: "X11Wasm Metadata Icon",
    command: "x11wasm-test --metadata",
    className: "metadata",
    classValue: "X11WasmMetadata",
    clientMachine: "x11wasm-client",
    transientFor: String(metadata.rootWindow),
    wmHintsFlags: "19",
    inputEligible: "0",
    initialState: "iconic",
    iconX: "17",
    iconY: "29",
    sizeFlags: "1020",
    sizeX: "11",
    sizeY: "13",
    sizeWidth: "120",
    sizeHeight: "80",
    minWidth: "40",
    minHeight: "30",
    maxWidth: "240",
    maxHeight: "160",
    widthInc: "7",
    heightInc: "5",
    baseWidth: "20",
    baseHeight: "10",
    winGravity: "9",
  });
  expect(metadata.afterSet.resource).toMatchObject({
    title: "X11Wasm Metadata Title",
    iconName: "X11Wasm Metadata Icon",
    command: "x11wasm-test --metadata",
    className: "metadata",
    classValue: "X11WasmMetadata",
    clientMachine: "x11wasm-client",
    transientFor: metadata.rootWindow,
    wmHints: {
      flags: 19,
      input: 0,
      initialState: 3,
      iconX: 17,
      iconY: 29,
    },
    sizeHints: {
      flags: 1020,
      x: 11,
      y: 13,
      width: 120,
      height: 80,
      minWidth: 40,
      minHeight: 30,
      maxWidth: 240,
      maxHeight: 160,
      widthInc: 7,
      heightInc: 5,
      baseWidth: 20,
      baseHeight: 10,
      winGravity: 9,
    },
  });
  expect(metadata.afterClear.attrs).toMatchObject({
    title: "",
    iconName: "",
    command: null,
    className: null,
    classValue: null,
    clientMachine: null,
    transientFor: null,
    wmHintsFlags: null,
    inputEligible: null,
    initialState: null,
    iconX: null,
    iconY: null,
    sizeFlags: null,
    minWidth: null,
    maxWidth: null,
  });
  expect(metadata.afterClear.resource).toMatchObject({
    title: "",
    iconName: "",
    command: "",
    className: "",
    classValue: "",
    clientMachine: "",
    transientFor: 0,
    wmHints: null,
    sizeHints: null,
  });
});

test("EWMH UTF-8 name properties override ICCCM title metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetWindowNetWmNameProperties?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (!module?._X11WasmTestSetWindowNetWmNameProperties || !backend) {
      throw new Error("missing EWMH name metadata test hooks");
    }

    const resource = backend.resources.get(windowId);
    const element = resource?.element;
    if (!(element instanceof HTMLElement) || !resource) {
      throw new Error("missing EWMH name metadata test element");
    }

    const snapshot = () => ({
      attrIconName: element.getAttribute("data-x11-wasm-icon-name"),
      attrTitle: element.getAttribute("data-x11-wasm-title"),
      documentTitle: document.title,
      resourceIconName: resource.iconName,
      resourceTitle: resource.title,
    });

    const setFallbackResult =
      module._X11WasmTestSetWindowNetWmNameProperties(windowId, 1);
    backend.flush?.();
    const afterFallback = snapshot();
    const setNetResult =
      module._X11WasmTestSetWindowNetWmNameProperties(windowId, 2);
    backend.flush?.();
    const afterNet = snapshot();
    const setInvalidNetResult =
      module._X11WasmTestSetWindowNetWmNameProperties(windowId, 4);
    backend.flush?.();
    const afterInvalidNet = snapshot();
    const deleteNetResult =
      module._X11WasmTestSetWindowNetWmNameProperties(windowId, 3);
    backend.flush?.();
    const afterDeleteNet = snapshot();
    const clearResult =
      module._X11WasmTestSetWindowNetWmNameProperties(windowId, 0);
    backend.flush?.();
    const afterClear = snapshot();

    return {
      afterClear,
      afterDeleteNet,
      afterFallback,
      afterInvalidNet,
      afterNet,
      clearResult,
      deleteNetResult,
      setFallbackResult,
      setInvalidNetResult,
      setNetResult,
    };
  }, windowId);

  expect(metadata.setFallbackResult).toBe(1);
  expect(metadata.setNetResult).toBe(1);
  expect(metadata.setInvalidNetResult).toBe(1);
  expect(metadata.deleteNetResult).toBe(1);
  expect(metadata.clearResult).toBe(1);
  expect(metadata.afterFallback).toEqual({
    attrIconName: "Fallback Icon",
    attrTitle: "Fallback Title",
    documentTitle: "Fallback Title",
    resourceIconName: "Fallback Icon",
    resourceTitle: "Fallback Title",
  });
  expect(metadata.afterNet).toEqual({
    attrIconName: "Net \u00e9 Icon",
    attrTitle: "Net \u2603 Title",
    documentTitle: "Net \u2603 Title",
    resourceIconName: "Net \u00e9 Icon",
    resourceTitle: "Net \u2603 Title",
  });
  expect(metadata.afterInvalidNet).toEqual(metadata.afterFallback);
  expect(metadata.afterDeleteNet).toEqual(metadata.afterFallback);
  expect(metadata.afterClear).toEqual({
    attrIconName: "",
    attrTitle: "",
    documentTitle: "",
    resourceIconName: "",
    resourceTitle: "",
  });
});

test("XRotateWindowProperties reapplies browser name metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestRotateWindowNameProperties?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const resource = backend?.resources.get(windowId);
    const element = resource?.element;
    if (
      !module?._X11WasmTestRotateWindowNameProperties ||
      !backend ||
      !resource ||
      !(element instanceof HTMLElement)
    ) {
      throw new Error("missing window name rotation metadata test hooks");
    }

    const rotateResult =
      module._X11WasmTestRotateWindowNameProperties(windowId);
    backend.flush?.();
    return {
      attrIconName: element.getAttribute("data-x11-wasm-icon-name"),
      attrTitle: element.getAttribute("data-x11-wasm-title"),
      documentTitle: document.title,
      resourceIconName: resource.iconName,
      resourceTitle: resource.title,
      rotateResult,
    };
  }, windowId);

  expect(metadata).toEqual({
    attrIconName: "Rotated Title",
    attrTitle: "Rotated Icon",
    documentTitle: "Rotated Icon",
    resourceIconName: "Rotated Title",
    resourceTitle: "Rotated Icon",
    rotateResult: 1,
  });
});

test("XGetWindowProperty delete clears browser name metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestGetWindowNamePropertiesDeletes?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const resource = backend?.resources.get(windowId);
    const element = resource?.element;
    if (
      !module?._X11WasmTestGetWindowNamePropertiesDeletes ||
      !backend ||
      !resource ||
      !(element instanceof HTMLElement)
    ) {
      throw new Error("missing window name delete metadata test hooks");
    }

    const deleteResult =
      module._X11WasmTestGetWindowNamePropertiesDeletes(windowId);
    backend.flush?.();
    return {
      attrIconName: element.getAttribute("data-x11-wasm-icon-name"),
      attrTitle: element.getAttribute("data-x11-wasm-title"),
      documentTitle: document.title,
      resourceIconName: resource.iconName,
      resourceTitle: resource.title,
      deleteResult,
    };
  }, windowId);

  expect(metadata).toEqual({
    attrIconName: "",
    attrTitle: "",
    documentTitle: "",
    resourceIconName: "",
    resourceTitle: "",
    deleteResult: 1,
  });
});

test("XGetWindowProperty delete clears browser ICCCM metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestGetWindowMetadataPropertiesDeletes?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const resource = backend?.resources.get(windowId);
    const element = resource?.element;
    if (
      !module?._X11WasmTestGetWindowMetadataPropertiesDeletes ||
      !backend ||
      !resource ||
      !(element instanceof HTMLElement)
    ) {
      throw new Error("missing ICCCM metadata delete test hooks");
    }

    const deleteResult =
      module._X11WasmTestGetWindowMetadataPropertiesDeletes(windowId);
    backend.flush?.();
    return {
      className: element.getAttribute("data-x11-wasm-class-name"),
      classValue: element.getAttribute("data-x11-wasm-class"),
      clientMachine: element.getAttribute("data-x11-wasm-client-machine"),
      command: element.getAttribute("data-x11-wasm-command"),
      deleteResult,
      resourceClassName: resource.wmClassName,
      resourceClassValue: resource.wmClass,
      resourceClientMachine: resource.clientMachine,
      resourceCommand: resource.command,
      resourceTransientFor: resource.transientFor,
      transientFor: element.getAttribute("data-x11-wasm-transient-for"),
    };
  }, windowId);

  expect(metadata).toEqual({
    className: null,
    classValue: null,
    clientMachine: null,
    command: null,
    deleteResult: 1,
    resourceClassName: "",
    resourceClassValue: "",
    resourceClientMachine: "",
    resourceCommand: "",
    resourceTransientFor: 0,
    transientFor: null,
  });
});

test("XGetWindowProperty delete clears browser WM_HINTS metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const [ownerId, iconId, groupId] = await createMappedWindows(page, 3);

  const metadata = await page.evaluate(
    ({ ownerId, iconId, groupId }) => {
      const module =
        /** @type {Window & { Module?: { _X11WasmTestGetWindowWMHintsDeletes?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
          window
        ).Module;
      const backend = module?.x11WasmBackend;
      const owner = backend?.resources.get(ownerId);
      const icon = backend?.resources.get(iconId);
      const group = backend?.resources.get(groupId);
      const ownerElement = owner?.element;
      const iconElement = icon?.element;
      const groupElement = group?.element;
      if (
        !module?._X11WasmTestGetWindowWMHintsDeletes ||
        !backend ||
        !owner ||
        !icon ||
        !group ||
        !(ownerElement instanceof HTMLElement) ||
        !(iconElement instanceof HTMLElement) ||
        !(groupElement instanceof HTMLElement)
      ) {
        throw new Error("missing WM_HINTS delete metadata test hooks");
      }

      const deleteResult = module._X11WasmTestGetWindowWMHintsDeletes(
        ownerId,
        iconId,
        groupId,
      );
      backend.flush?.();
      return {
        deleteResult,
        iconOwner: iconElement.getAttribute("data-x11-wasm-icon-owner"),
        ownerFlags: ownerElement.getAttribute("data-x11-wasm-wm-hints-flags"),
        ownerGroup: ownerElement.getAttribute("data-x11-wasm-window-group"),
        ownerIconWindow: ownerElement.getAttribute(
          "data-x11-wasm-icon-window",
        ),
        ownerInitialState: ownerElement.getAttribute(
          "data-x11-wasm-initial-state-name",
        ),
        ownerInputEligible: ownerElement.getAttribute(
          "data-x11-wasm-input-eligible",
        ),
        ownerWmHints: owner.wmHints,
        groupMembers: groupElement.getAttribute(
          "data-x11-wasm-window-group-members",
        ),
      };
    },
    { ownerId, iconId, groupId },
  );

  expect(metadata).toEqual({
    deleteResult: 1,
    iconOwner: null,
    ownerFlags: null,
    ownerGroup: null,
    ownerIconWindow: null,
    ownerInitialState: null,
    ownerInputEligible: null,
    ownerWmHints: null,
    groupMembers: null,
  });
});

test("XGetWindowProperty delete clears browser size hints metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestGetWindowNormalHintsDeletes?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const resource = backend?.resources.get(windowId);
    const element = resource?.element;
    if (
      !module?._X11WasmTestGetWindowNormalHintsDeletes ||
      !backend ||
      !resource ||
      !(element instanceof HTMLElement)
    ) {
      throw new Error("missing size hints delete metadata test hooks");
    }

    const deleteResult =
      module._X11WasmTestGetWindowNormalHintsDeletes(windowId);
    backend.flush?.();
    const computed = getComputedStyle(element);
    return {
      deleteResult,
      flags: element.getAttribute("data-x11-wasm-size-hints-flags"),
      minWidth: element.getAttribute("data-x11-wasm-min-width"),
      maxWidth: element.getAttribute("data-x11-wasm-max-width"),
      widthInc: element.getAttribute("data-x11-wasm-width-inc"),
      heightInc: element.getAttribute("data-x11-wasm-height-inc"),
      baseWidth: element.getAttribute("data-x11-wasm-base-width"),
      baseHeight: element.getAttribute("data-x11-wasm-base-height"),
      resourceSizeHints: resource.sizeHints,
      styleAspectRatio: element.style.aspectRatio,
      styleMaxWidth: computed.maxWidth,
      styleMinWidth: computed.minWidth,
    };
  }, windowId);

  expect(metadata).toEqual({
    deleteResult: 1,
    flags: null,
    minWidth: null,
    maxWidth: null,
    widthInc: null,
    heightInc: null,
    baseWidth: null,
    baseHeight: null,
    resourceSizeHints: null,
    styleAspectRatio: "",
    styleMaxWidth: "none",
    styleMinWidth: "0px",
  });
});

test("XGetWindowProperty delete clears browser collection metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const [windowId, colormapWindowId] = await createMappedWindows(page, 2);

  const metadata = await page.evaluate(
    ({ colormapWindowId, windowId }) => {
      const module =
        /** @type {Window & { Module?: { _X11WasmTestGetWindowCollectionPropertiesDeletes?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
          window
        ).Module;
      const backend = module?.x11WasmBackend;
      const resource = backend?.resources.get(windowId);
      const element = resource?.element;
      if (
        !module?._X11WasmTestGetWindowCollectionPropertiesDeletes ||
        !backend ||
        !resource ||
        !(element instanceof HTMLElement)
      ) {
        throw new Error("missing collection delete metadata test hooks");
      }

      const deleteResult =
        module._X11WasmTestGetWindowCollectionPropertiesDeletes(
          windowId,
          colormapWindowId,
        );
      backend.flush?.();
      return {
        colormapWindowCount: element.getAttribute(
          "data-x11-wasm-colormap-window-count",
        ),
        colormapWindows: element.getAttribute(
          "data-x11-wasm-colormap-windows",
        ),
        deleteResult,
        iconSizeCount: element.getAttribute("data-x11-wasm-icon-size-count"),
        iconSizes: element.getAttribute("data-x11-wasm-icon-sizes"),
        protocols: element.getAttribute("data-x11-wasm-protocols"),
        resourceColormapWindows: [...(resource.colormapWindows ?? [])],
        resourceIconSizeCount: resource.iconSizeCount,
        resourceIconSizes: resource.iconSizes,
        resourceProtocols: [...(resource.protocols ?? [])],
        resourceStandardColormaps: JSON.parse(
          JSON.stringify(resource.standardColormaps ?? {}),
        ),
        standardColormapCount: element.getAttribute(
          "data-x11-wasm-standard-colormap-count",
        ),
        standardColormaps: element.getAttribute(
          "data-x11-wasm-standard-colormaps",
        ),
      };
    },
    { colormapWindowId, windowId },
  );

  expect(metadata).toEqual({
    colormapWindowCount: null,
    colormapWindows: null,
    deleteResult: 1,
    iconSizeCount: null,
    iconSizes: null,
    protocols: null,
    resourceColormapWindows: [],
    resourceIconSizeCount: 0,
    resourceIconSizes: "",
    resourceProtocols: [],
    resourceStandardColormaps: {},
    standardColormapCount: null,
    standardColormaps: null,
  });
});

test("XGetWindowProperty delete clears browser EWMH metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestGetWindowEwmhPropertiesDeletes?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const resource = backend?.resources.get(windowId);
    const element = resource?.element;
    if (
      !module?._X11WasmTestGetWindowEwmhPropertiesDeletes ||
      !backend ||
      !resource ||
      !(element instanceof HTMLElement)
    ) {
      throw new Error("missing EWMH delete metadata test hooks");
    }

    const deleteResult =
      module._X11WasmTestGetWindowEwmhPropertiesDeletes(windowId);
    backend.flush?.();
    const documentIcon = document.querySelector("link[data-x11-wasm-icon]");
    return {
      alpha: element.getAttribute("data-x11-wasm-alpha"),
      deleteResult,
      documentIconHref: documentIcon?.getAttribute("href") ?? null,
      fullscreen: element.getAttribute("data-x11-wasm-fullscreen"),
      iconPhotoCount: element.getAttribute("data-x11-wasm-icon-photo-count"),
      iconPhotoHeight: element.getAttribute("data-x11-wasm-icon-photo-height"),
      iconPhotoHref: element.getAttribute("data-x11-wasm-icon-photo-href"),
      iconPhotoSizes: element.getAttribute("data-x11-wasm-icon-photo-sizes"),
      iconPhotoWidth: element.getAttribute("data-x11-wasm-icon-photo-width"),
      opacity: element.style.opacity,
      resourceAlpha: resource.alpha,
      resourceFullscreen: !!resource.fullscreen,
      resourceIconPhotoCount: resource.iconPhotoCount,
      resourceIconPhotoHeight: resource.iconPhotoHeight,
      resourceIconPhotoHref: resource.iconPhotoHref,
      resourceIconPhotoSizes: resource.iconPhotoSizes,
      resourceIconPhotoWidth: resource.iconPhotoWidth,
      resourceTopmost: !!resource.topmost,
      resourceWindowTypes: [...(resource.windowTypes ?? [])],
      resourceZoomed: !!resource.zoomed,
      topmost: element.getAttribute("data-x11-wasm-topmost"),
      windowTypes: element.getAttribute("data-x11-wasm-window-types"),
      zoomed: element.getAttribute("data-x11-wasm-zoomed"),
    };
  }, windowId);

  expect(metadata).toEqual({
    alpha: "1",
    deleteResult: 1,
    documentIconHref: null,
    fullscreen: null,
    iconPhotoCount: null,
    iconPhotoHeight: null,
    iconPhotoHref: null,
    iconPhotoSizes: null,
    iconPhotoWidth: null,
    opacity: "",
    resourceAlpha: 1,
    resourceFullscreen: false,
    resourceIconPhotoCount: 0,
    resourceIconPhotoHeight: 0,
    resourceIconPhotoHref: "",
    resourceIconPhotoSizes: "",
    resourceIconPhotoWidth: 0,
    resourceTopmost: false,
    resourceWindowTypes: [],
    resourceZoomed: false,
    topmost: null,
    windowTypes: null,
    zoomed: null,
  });
});

test("_NET_WM_WINDOW_TYPE updates browser window type metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetWindowTypeProperties?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const resource = backend?.resources.get(windowId);
    const element = resource?.element;
    if (
      !module?._X11WasmTestSetWindowTypeProperties ||
      !backend ||
      !resource ||
      !(element instanceof HTMLElement)
    ) {
      throw new Error("missing window type metadata test hooks");
    }

    const snapshot = () => ({
      attr: element.getAttribute("data-x11-wasm-window-types"),
      resourceTypes: [...(resource.windowTypes || [])],
    });
    const setResult = module._X11WasmTestSetWindowTypeProperties(windowId, 1);
    backend.flush?.();
    const afterSet = snapshot();
    const invalidResult = module._X11WasmTestSetWindowTypeProperties(
      windowId,
      2,
    );
    backend.flush?.();
    const afterInvalid = snapshot();
    const resetResult = module._X11WasmTestSetWindowTypeProperties(windowId, 1);
    backend.flush?.();
    const afterReset = snapshot();
    const clearResult = module._X11WasmTestSetWindowTypeProperties(windowId, 0);
    backend.flush?.();
    const afterClear = snapshot();

    return {
      afterClear,
      afterInvalid,
      afterReset,
      afterSet,
      clearResult,
      invalidResult,
      resetResult,
      setResult,
    };
  }, windowId);

  const expectedTypes = [
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_UTILITY",
    "_NET_WM_WINDOW_TYPE_NORMAL",
  ];
  const expectedAttr = expectedTypes.join(" ");
  expect(metadata).toEqual({
    setResult: 1,
    invalidResult: 1,
    resetResult: 1,
    clearResult: 1,
    afterSet: {
      attr: expectedAttr,
      resourceTypes: expectedTypes,
    },
    afterInvalid: {
      attr: null,
      resourceTypes: [],
    },
    afterReset: {
      attr: expectedAttr,
      resourceTypes: expectedTypes,
    },
    afterClear: {
      attr: null,
      resourceTypes: [],
    },
  });
});

test("_NET_WM_STATE client messages update browser window state metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSendNetWmStateMessage?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const resource = backend?.resources.get(windowId);
    const element = resource?.element;
    if (
      !module?._X11WasmTestSendNetWmStateMessage ||
      !backend ||
      !resource ||
      !(element instanceof HTMLElement)
    ) {
      throw new Error("missing EWMH state metadata test hooks");
    }

    const states = {
      above: 1,
      maximizedVert: 2,
      maximizedHorz: 3,
      fullscreen: 4,
    };
    const actions = {
      remove: 0,
      add: 1,
      toggle: 2,
    };
    const snapshot = () => ({
      attrFullscreen: element.getAttribute("data-x11-wasm-fullscreen"),
      attrTopmost: element.getAttribute("data-x11-wasm-topmost"),
      attrZoomed: element.getAttribute("data-x11-wasm-zoomed"),
      fullscreen: !!resource.fullscreen,
      topmost: !!resource.topmost,
      zoomed: !!resource.zoomed,
    });

    const initial = snapshot();
    const addZoomResult = module._X11WasmTestSendNetWmStateMessage(
      windowId,
      actions.add,
      states.maximizedVert,
      states.maximizedHorz,
    );
    backend.flush?.();
    const afterAddZoom = snapshot();
    const addTopFullscreenResult = module._X11WasmTestSendNetWmStateMessage(
      windowId,
      actions.add,
      states.above,
      states.fullscreen,
    );
    backend.flush?.();
    const afterAddTopFullscreen = snapshot();
    const toggleTopResult = module._X11WasmTestSendNetWmStateMessage(
      windowId,
      actions.toggle,
      states.above,
      0,
    );
    backend.flush?.();
    const afterToggleTop = snapshot();
    const removeZoomResult = module._X11WasmTestSendNetWmStateMessage(
      windowId,
      actions.remove,
      states.maximizedVert,
      states.maximizedHorz,
    );
    backend.flush?.();
    const afterRemoveZoom = snapshot();
    const removeFullscreenResult = module._X11WasmTestSendNetWmStateMessage(
      windowId,
      actions.remove,
      states.fullscreen,
      0,
    );
    backend.flush?.();
    const afterRemoveFullscreen = snapshot();

    return {
      addTopFullscreenResult,
      addZoomResult,
      afterAddTopFullscreen,
      afterAddZoom,
      afterRemoveFullscreen,
      afterRemoveZoom,
      afterToggleTop,
      initial,
      removeFullscreenResult,
      removeZoomResult,
      toggleTopResult,
    };
  }, windowId);

  expect(metadata.addZoomResult).toBe(1);
  expect(metadata.addTopFullscreenResult).toBe(1);
  expect(metadata.toggleTopResult).toBe(1);
  expect(metadata.removeZoomResult).toBe(1);
  expect(metadata.removeFullscreenResult).toBe(1);
  expect(metadata.initial).toEqual({
    attrFullscreen: null,
    attrTopmost: null,
    attrZoomed: null,
    fullscreen: false,
    topmost: false,
    zoomed: false,
  });
  expect(metadata.afterAddZoom).toEqual({
    attrFullscreen: null,
    attrTopmost: null,
    attrZoomed: "1",
    fullscreen: false,
    topmost: false,
    zoomed: true,
  });
  expect(metadata.afterAddTopFullscreen).toEqual({
    attrFullscreen: "1",
    attrTopmost: "1",
    attrZoomed: "1",
    fullscreen: true,
    topmost: true,
    zoomed: true,
  });
  expect(metadata.afterToggleTop).toEqual({
    attrFullscreen: "1",
    attrTopmost: null,
    attrZoomed: "1",
    fullscreen: true,
    topmost: false,
    zoomed: true,
  });
  expect(metadata.afterRemoveZoom).toEqual({
    attrFullscreen: "1",
    attrTopmost: null,
    attrZoomed: null,
    fullscreen: true,
    topmost: false,
    zoomed: false,
  });
  expect(metadata.afterRemoveFullscreen).toEqual({
    attrFullscreen: null,
    attrTopmost: null,
    attrZoomed: null,
    fullscreen: false,
    topmost: false,
    zoomed: false,
  });
});

test("metadata target properties forward ICCCM metadata to another window", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const [sourceId, targetId] = await createMappedWindows(page, 2);

  const metadata = await page.evaluate(({ sourceId, targetId }) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetWindowMetadataProperties?: Function, _X11WasmTestSetWindowMetadataTargetProperty?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (
      !module?._X11WasmTestSetWindowMetadataProperties ||
      !module._X11WasmTestSetWindowMetadataTargetProperty ||
      !backend
    ) {
      throw new Error("missing metadata routing test hooks");
    }

    const targetResult = module._X11WasmTestSetWindowMetadataTargetProperty(
      sourceId,
      targetId,
      1,
    );
    const metadataResult = module._X11WasmTestSetWindowMetadataProperties(
      sourceId,
      1,
    );
    backend.flush?.();
    const rootDocumentTitle = document.title;
    document.title = "non-root metadata title sentinel";
    const nonRootTargetResult = module._X11WasmTestSetWindowMetadataTargetProperty(
      sourceId,
      targetId,
      0,
    );
    backend.flush?.();
    const nonRootDocumentTitle = document.title;

    const source = backend.resources.get(sourceId);
    const target = backend.resources.get(targetId);
    const sourceElement = source?.element;
    const targetElement = target?.element;
    if (
      !(sourceElement instanceof HTMLElement) ||
      !(targetElement instanceof HTMLElement) ||
      !source ||
      !target
    ) {
      throw new Error("missing metadata routing test elements");
    }
    return {
      documentTitle: rootDocumentTitle,
      metadataResult,
      nonRootDocumentTitle,
      nonRootTargetResult,
      source: {
        command: source.command,
        title: source.title,
        titleAttr: sourceElement.getAttribute("data-x11-wasm-title"),
      },
      target: {
        command: target.command,
        commandAttr: targetElement.getAttribute("data-x11-wasm-command"),
        title: target.title,
        titleAttr: targetElement.getAttribute("data-x11-wasm-title"),
      },
      targetResult,
    };
  }, { sourceId, targetId });

  expect(metadata).toEqual({
    documentTitle: "X11Wasm Metadata Title",
    metadataResult: 1,
    nonRootDocumentTitle: "",
    nonRootTargetResult: 1,
    source: {
      command: "",
      title: "",
      titleAttr: null,
    },
    target: {
      command: "x11wasm-test --metadata",
      commandAttr: "x11wasm-test --metadata",
      title: "X11Wasm Metadata Title",
      titleAttr: "X11Wasm Metadata Title",
    },
    targetResult: 1,
  });
});

test("metadata target delete clears old forwarded ICCCM metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const [sourceId, targetId] = await createMappedWindows(page, 2);

  const metadata = await page.evaluate(({ sourceId, targetId }) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestGetWindowMetadataTargetDeletes?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (
      !module?._X11WasmTestGetWindowMetadataTargetDeletes ||
      !backend
    ) {
      throw new Error("missing metadata target delete test hooks");
    }

    const deleteResult = module._X11WasmTestGetWindowMetadataTargetDeletes(
      sourceId,
      targetId,
    );
    backend.flush?.();
    const snapshot = (id) => {
      const resource = backend.resources.get(id);
      const element = resource?.element;
      if (!(element instanceof HTMLElement) || !resource) {
        throw new Error("missing metadata target delete test element");
      }
      return {
        attrs: {
          title: element.getAttribute("data-x11-wasm-title"),
          iconName: element.getAttribute("data-x11-wasm-icon-name"),
          command: element.getAttribute("data-x11-wasm-command"),
          className: element.getAttribute("data-x11-wasm-class-name"),
          classValue: element.getAttribute("data-x11-wasm-class"),
          clientMachine: element.getAttribute("data-x11-wasm-client-machine"),
          transientFor: element.getAttribute("data-x11-wasm-transient-for"),
          wmHintsFlags: element.getAttribute("data-x11-wasm-wm-hints-flags"),
          sizeFlags: element.getAttribute("data-x11-wasm-size-hints-flags"),
        },
        resource: {
          title: resource.title,
          iconName: resource.iconName,
          command: resource.command,
          className: resource.wmClassName,
          classValue: resource.wmClass,
          clientMachine: resource.clientMachine,
          transientFor: resource.transientFor,
          wmHintsFlags: resource.wmHints?.flags ?? null,
          sizeFlags: resource.sizeHints?.flags ?? null,
        },
      };
    };

    return {
      deleteResult,
      documentTitle: document.title,
      rootWindow: backend.rootWindow,
      source: snapshot(sourceId),
      target: snapshot(targetId),
    };
  }, { sourceId, targetId });

  expect(metadata).toEqual({
    deleteResult: 1,
    documentTitle: "X11Wasm Metadata Title",
    rootWindow: expect.any(Number),
    source: {
      attrs: {
        title: "X11Wasm Metadata Title",
        iconName: "X11Wasm Metadata Icon",
        command: "x11wasm-test --metadata",
        className: "metadata",
        classValue: "X11WasmMetadata",
        clientMachine: "x11wasm-client",
        transientFor: String(metadata.rootWindow),
        wmHintsFlags: "19",
        sizeFlags: "1020",
      },
      resource: {
        title: "X11Wasm Metadata Title",
        iconName: "X11Wasm Metadata Icon",
        command: "x11wasm-test --metadata",
        className: "metadata",
        classValue: "X11WasmMetadata",
        clientMachine: "x11wasm-client",
        transientFor: metadata.rootWindow,
        wmHintsFlags: 19,
        sizeFlags: 1020,
      },
    },
    target: {
      attrs: {
        title: null,
        iconName: null,
        command: null,
        className: null,
        classValue: null,
        clientMachine: null,
        transientFor: null,
        wmHintsFlags: "2",
        sizeFlags: null,
      },
      resource: {
        title: "",
        iconName: "",
        command: "",
        className: "",
        classValue: "",
        clientMachine: "",
        transientFor: 0,
        wmHintsFlags: 2,
        sizeFlags: null,
      },
    },
  });
});

test("metadata target source destroy clears old forwarded ICCCM metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const [sourceId, targetId] = await createMappedWindows(page, 2);

  const metadata = await page.evaluate(({ sourceId, targetId }) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestDestroyWindowMetadataTargetSource?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (
      !module?._X11WasmTestDestroyWindowMetadataTargetSource ||
      !backend
    ) {
      throw new Error("missing metadata target destroy test hooks");
    }

    const destroyResult = module._X11WasmTestDestroyWindowMetadataTargetSource(
      sourceId,
      targetId,
    );
    backend.flush?.();
    const target = backend.resources.get(targetId);
    const targetElement = target?.element;
    if (!(targetElement instanceof HTMLElement) || !target) {
      throw new Error("missing metadata target destroy test element");
    }
    return {
      destroyResult,
      sourcePresent: backend.resources.has(sourceId),
      target: {
        command: target.command,
        commandAttr: targetElement.getAttribute("data-x11-wasm-command"),
        iconName: target.iconName,
        iconNameAttr: targetElement.getAttribute("data-x11-wasm-icon-name"),
        title: target.title,
        titleAttr: targetElement.getAttribute("data-x11-wasm-title"),
      },
    };
  }, { sourceId, targetId });

  expect(metadata).toEqual({
    destroyResult: 1,
    sourcePresent: false,
    target: {
      command: "",
      commandAttr: null,
      iconName: "",
      iconNameAttr: null,
      title: "",
      titleAttr: null,
    },
  });
});

test("metadata fallback source destroy clears old forwarded ICCCM metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const targetId = await createMappedWindow(page);

  const metadata = await page.evaluate((targetId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestDestroyWindowMetadataFallbackSource?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (
      !module?._X11WasmTestDestroyWindowMetadataFallbackSource ||
      !backend
    ) {
      throw new Error("missing metadata fallback destroy test hooks");
    }

    const destroyResult =
      module._X11WasmTestDestroyWindowMetadataFallbackSource(targetId);
    backend.flush?.();
    const target = backend.resources.get(targetId);
    const targetElement = target?.element;
    if (!(targetElement instanceof HTMLElement) || !target) {
      throw new Error("missing metadata fallback destroy test element");
    }
    return {
      destroyResult,
      target: {
        command: target.command,
        commandAttr: targetElement.getAttribute("data-x11-wasm-command"),
        iconName: target.iconName,
        iconNameAttr: targetElement.getAttribute("data-x11-wasm-icon-name"),
        title: target.title,
        titleAttr: targetElement.getAttribute("data-x11-wasm-title"),
      },
    };
  }, targetId);

  expect(metadata).toEqual({
    destroyResult: 1,
    target: {
      command: "",
      commandAttr: null,
      iconName: "",
      iconNameAttr: null,
      title: "",
      titleAttr: null,
    },
  });
});

test("metadata target API reapplies existing ICCCM metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const [sourceId, targetId] = await createMappedWindows(page, 2);

  const metadata = await page.evaluate(({ sourceId, targetId }) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetWindowMetadataProperties?: Function, _X11WasmTestSetWindowMetadataTargetApi?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (
      !module?._X11WasmTestSetWindowMetadataProperties ||
      !module._X11WasmTestSetWindowMetadataTargetApi ||
      !backend
    ) {
      throw new Error("missing metadata target API test hooks");
    }

    const metadataResult = module._X11WasmTestSetWindowMetadataProperties(
      sourceId,
      1,
    );
    backend.flush?.();
    document.title = "metadata target API sentinel";
    const targetResult = module._X11WasmTestSetWindowMetadataTargetApi(
      sourceId,
      targetId,
      1,
      1,
    );
    backend.flush?.();

    const target = backend.resources.get(targetId);
    const targetElement = target?.element;
    if (!(targetElement instanceof HTMLElement) || !target) {
      throw new Error("missing metadata target API test element");
    }
    return {
      documentTitle: document.title,
      metadataResult,
      target: {
        className: target.wmClassName,
        classNameAttr: targetElement.getAttribute(
          "data-x11-wasm-class-name",
        ),
        command: target.command,
        commandAttr: targetElement.getAttribute("data-x11-wasm-command"),
        title: target.title,
        titleAttr: targetElement.getAttribute("data-x11-wasm-title"),
      },
      targetResult,
    };
  }, { sourceId, targetId });

  expect(metadata).toEqual({
    documentTitle: "X11Wasm Metadata Title",
    metadataResult: 1,
    target: {
      className: "metadata",
      classNameAttr: "metadata",
      command: "x11wasm-test --metadata",
      commandAttr: "x11wasm-test --metadata",
      title: "X11Wasm Metadata Title",
      titleAttr: "X11Wasm Metadata Title",
    },
    targetResult: 1,
  });
});

test("browser backend reflects WM_HINTS relationships in DOM state", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const hints = await page.evaluate(() => {
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { createWindow?: Function, setWindowWMHints?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (!backend?.createWindow || !backend.setWindowWMHints) {
      throw new Error("missing X11 wasm backend WM_HINTS methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const ownerId = 0x710201;
    const iconId = 0x710202;
    const groupId = 0x710203;
    const inputHint = 1 << 0;
    const stateHint = 1 << 1;
    const iconWindowHint = 1 << 3;
    const windowGroupHint = 1 << 6;
    const iconicState = 3;

    backend.createWindow(ownerId, rootWindow, 0, 0, 80, 60, 24, 0);
    backend.createWindow(iconId, rootWindow, 0, 0, 24, 24, 24, 0);
    backend.createWindow(groupId, rootWindow, 0, 0, 10, 10, 24, 0);
    backend.setWindowWMHints(
      ownerId,
      inputHint | stateHint | iconWindowHint | windowGroupHint,
      0,
      iconicState,
      0,
      iconId,
      0,
      0,
      0,
      groupId,
    );

    const owner = backend.resources.get(ownerId);
    const icon = backend.resources.get(iconId);
    const group = backend.resources.get(groupId);
    if (
      !(owner?.element instanceof HTMLElement) ||
      !(icon?.element instanceof HTMLElement) ||
      !(group?.element instanceof HTMLElement)
    ) {
      throw new Error("missing WM_HINTS test elements");
    }
    const beforeClear = {
      iconOwner: icon.element.getAttribute("data-x11-wasm-icon-owner"),
      initialStateName: owner.element.getAttribute(
        "data-x11-wasm-initial-state-name",
      ),
      inputEligible: owner.element.getAttribute("data-x11-wasm-input-eligible"),
      ownerIconWindow: owner.element.getAttribute("data-x11-wasm-icon-window"),
      ownerWindowGroup: owner.element.getAttribute("data-x11-wasm-window-group"),
      groupMembers: group.element.getAttribute(
        "data-x11-wasm-window-group-members",
      ),
      resourceIconWindow: owner.wmHints?.iconWindow,
      resourceWindowGroup: owner.wmHints?.windowGroup,
    };

    backend.setWindowWMHints(ownerId, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    const afterClear = {
      iconOwner: icon.element.getAttribute("data-x11-wasm-icon-owner"),
      ownerFlags: owner.element.getAttribute("data-x11-wasm-wm-hints-flags"),
      groupMembers: group.element.getAttribute(
        "data-x11-wasm-window-group-members",
      ),
      resourceHints: owner.wmHints,
    };

    return { afterClear, beforeClear };
  });

  expect(hints).toEqual({
    beforeClear: {
      iconOwner: String(0x710201),
      initialStateName: "iconic",
      inputEligible: "0",
      ownerIconWindow: String(0x710202),
      ownerWindowGroup: String(0x710203),
      groupMembers: String(0x710201),
      resourceIconWindow: 0x710202,
      resourceWindowGroup: 0x710203,
    },
    afterClear: {
      iconOwner: null,
      ownerFlags: null,
      groupMembers: null,
      resourceHints: null,
    },
  });
});

test("browser backend honors WM_HINTS initialState on first map", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const states = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestMapWindowWithInitialState?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, setWindowWMHints?: Function, flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (
      !module?._X11WasmTestMapWindowWithInitialState ||
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.setWindowWMHints ||
      !backend.flush
    ) {
      throw new Error("missing WM_HINTS initial state test hooks");
    }
    const rootWindow = backend.rootWindow || 1;
    const ownerId = 0x710204;
    const iconId = 0x710207;
    const stateHint = 1 << 1;
    const iconWindowHint = 1 << 3;
    const iconPositionHint = 1 << 4;
    const iconicState = 3;

    backend.createWindow(ownerId, rootWindow, 40, 50, 80, 60, 24, 0);
    backend.createWindow(iconId, rootWindow, 5, 6, 24, 20, 24, 0);
    backend.setWindowWMHints(
      ownerId,
      stateHint | iconWindowHint | iconPositionHint,
      0,
      iconicState,
      0,
      iconId,
      17,
      29,
      0,
      0,
    );
    backend.mapWindow(ownerId);
    backend.flush();

    const owner = backend.resources.get(ownerId);
    const icon = backend.resources.get(iconId);
    const cWindowId =
      module._X11WasmTestMapWindowWithInitialState(iconicState);
    backend.flush();
    const cWindow = backend.resources.get(cWindowId);
    if (
      !(owner?.element instanceof HTMLElement) ||
      !(icon?.element instanceof HTMLElement) ||
      !(cWindow?.element instanceof HTMLElement)
    ) {
      throw new Error("missing WM_HINTS initial state elements");
    }

    return {
      cPath: {
        display: cWindow.element.style.display,
        mapped: cWindow.mapped,
        state: cWindow.element.getAttribute("data-x11-wasm-wm-state"),
        viewable: cWindow.element.getAttribute("data-x11-wasm-viewable"),
        windowId: cWindowId,
      },
      jsPath: {
        iconDisplay: icon.element.style.display,
        iconLeft: icon.element.style.left,
        iconProxyActive: icon.element.getAttribute(
          "data-x11-wasm-icon-proxy-active",
        ),
        iconProxyOwner: icon.element.getAttribute("data-x11-wasm-icon-proxy-owner"),
        iconTop: icon.element.style.top,
        iconViewable: icon.element.getAttribute("data-x11-wasm-viewable"),
        ownerDisplay: owner.element.style.display,
        ownerMapped: owner.mapped,
        ownerState: owner.element.getAttribute("data-x11-wasm-wm-state"),
        ownerViewable: owner.element.getAttribute("data-x11-wasm-viewable"),
      },
    };
  });

  expect(states.jsPath).toEqual({
    iconDisplay: "",
    iconLeft: "17px",
    iconProxyActive: "1",
    iconProxyOwner: String(0x710204),
    iconTop: "29px",
    iconViewable: "1",
    ownerDisplay: "none",
    ownerMapped: true,
    ownerState: "iconic",
    ownerViewable: "0",
  });
  expect(states.cPath).toEqual({
    display: "none",
    mapped: true,
    state: "iconic",
    viewable: "0",
    windowId: expect.any(Number),
  });
  expect(states.cPath.windowId).toBeGreaterThan(0);
});

test("XIconifyWindow and XWithdrawWindow unmap windows and update WM_STATE", async ({
  page,
}) => {
  await waitForX11Runtime(page);

  const states = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestWindowManagerStateTransition?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (!module?._X11WasmTestWindowManagerStateTransition || !backend) {
      throw new Error("missing WM state transition test hooks");
    }
    const snapshot = (windowId) => {
      backend.flush?.();
      const resource = backend.resources.get(windowId);
      const element = resource?.element;
      if (!(element instanceof HTMLElement) || !resource) {
        throw new Error("missing WM state transition test element");
      }
      return {
        ariaHidden: element.getAttribute("aria-hidden"),
        display: element.style.display,
        mapped: resource.mapped,
        state: element.getAttribute("data-x11-wasm-wm-state"),
        viewable: element.getAttribute("data-x11-wasm-viewable"),
        windowId,
      };
    };

    const iconicId =
      module._X11WasmTestWindowManagerStateTransition(3) >>> 0;
    const withdrawnId =
      module._X11WasmTestWindowManagerStateTransition(0) >>> 0;
    if (iconicId === 0 || withdrawnId === 0) {
      throw new Error("WM state transition C hook failed");
    }

    return {
      iconic: snapshot(iconicId),
      withdrawn: snapshot(withdrawnId),
    };
  });

  expect(states.iconic).toEqual({
    ariaHidden: "true",
    display: "none",
    mapped: false,
    state: "iconic",
    viewable: "0",
    windowId: expect.any(Number),
  });
  expect(states.withdrawn).toEqual({
    ariaHidden: "true",
    display: "none",
    mapped: false,
    state: "withdrawn",
    viewable: "0",
    windowId: expect.any(Number),
  });
  expect(states.iconic.windowId).toBeGreaterThan(0);
  expect(states.withdrawn.windowId).toBeGreaterThan(0);
  expect(states.iconic.windowId).not.toBe(states.withdrawn.windowId);
});

test("browser backend displays WM_HINTS icon window while owner is iconic", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const states = await page.evaluate(() => {
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, unmapWindow?: Function, setWindowWMHints?: Function, setWindowWmState?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.unmapWindow ||
      !backend.setWindowWMHints ||
      !backend.setWindowWmState ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend WM_HINTS icon methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const ownerId = 0x710205;
    const iconId = 0x710206;
    const normalState = 1;
    const iconicState = 3;
    const iconWindowHint = 1 << 3;
    const iconPositionHint = 1 << 4;

    backend.createWindow(ownerId, rootWindow, 40, 50, 80, 60, 24, 0);
    backend.createWindow(iconId, rootWindow, 5, 6, 24, 20, 24, 0);
    backend.mapWindow(ownerId);
    backend.flush();
    backend.setWindowWMHints(
      ownerId,
      iconWindowHint | iconPositionHint,
      0,
      0,
      0,
      iconId,
      17,
      29,
      0,
      0,
    );

    const owner = backend.resources.get(ownerId);
    const icon = backend.resources.get(iconId);
    if (
      !(owner?.element instanceof HTMLElement) ||
      !(icon?.element instanceof HTMLElement)
    ) {
      throw new Error("missing WM_HINTS icon window elements");
    }
    const ownerElement = owner.element;
    const iconElement = icon.element;
    const snapshot = () => ({
      iconDisplay: iconElement.style.display,
      iconLeft: iconElement.style.left,
      iconMapped: icon.mapped,
      iconOwner: iconElement.getAttribute("data-x11-wasm-icon-owner"),
      iconProxyActive: iconElement.getAttribute(
        "data-x11-wasm-icon-proxy-active",
      ),
      iconProxyOwner: iconElement.getAttribute("data-x11-wasm-icon-proxy-owner"),
      iconProxyX: iconElement.getAttribute("data-x11-wasm-icon-proxy-x"),
      iconProxyY: iconElement.getAttribute("data-x11-wasm-icon-proxy-y"),
      iconTop: iconElement.style.top,
      iconViewable: iconElement.getAttribute("data-x11-wasm-viewable"),
      ownerDisplay: ownerElement.style.display,
      ownerState: ownerElement.getAttribute("data-x11-wasm-wm-state"),
      ownerViewable: ownerElement.getAttribute("data-x11-wasm-viewable"),
    });

    backend.setWindowWmState(ownerId, iconicState);
    const iconic = snapshot();
    backend.setWindowWmState(ownerId, normalState);
    const normal = snapshot();
    backend.mapWindow(iconId);
    backend.flush();
    const mappedIcon = snapshot();
    backend.unmapWindow(iconId);
    backend.flush();
    backend.setWindowWmState(ownerId, iconicState);
    backend.setWindowWMHints(ownerId, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    const cleared = snapshot();

    return { cleared, iconic, mappedIcon, normal };
  });

  expect(states.iconic).toEqual({
    iconDisplay: "",
    iconLeft: "17px",
    iconMapped: false,
    iconOwner: String(0x710205),
    iconProxyActive: "1",
    iconProxyOwner: String(0x710205),
    iconProxyX: "17",
    iconProxyY: "29",
    iconTop: "29px",
    iconViewable: "1",
    ownerDisplay: "none",
    ownerState: "iconic",
    ownerViewable: "0",
  });
  expect(states.normal).toEqual({
    iconDisplay: "none",
    iconLeft: "5px",
    iconMapped: false,
    iconOwner: String(0x710205),
    iconProxyActive: null,
    iconProxyOwner: null,
    iconProxyX: null,
    iconProxyY: null,
    iconTop: "6px",
    iconViewable: "0",
    ownerDisplay: "",
    ownerState: "normal",
    ownerViewable: "1",
  });
  expect(states.mappedIcon).toEqual({
    iconDisplay: "",
    iconLeft: "5px",
    iconMapped: true,
    iconOwner: String(0x710205),
    iconProxyActive: null,
    iconProxyOwner: null,
    iconProxyX: null,
    iconProxyY: null,
    iconTop: "6px",
    iconViewable: "1",
    ownerDisplay: "",
    ownerState: "normal",
    ownerViewable: "1",
  });
  expect(states.cleared).toEqual({
    iconDisplay: "none",
    iconLeft: "5px",
    iconMapped: false,
    iconOwner: null,
    iconProxyActive: null,
    iconProxyOwner: null,
    iconProxyX: null,
    iconProxyY: null,
    iconTop: "6px",
    iconViewable: "0",
    ownerDisplay: "none",
    ownerState: "iconic",
    ownerViewable: "0",
  });
});

test("browser backend honors WM_INPUT_HINT for pointer input focus", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const ids = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, setWindowWMHints?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.setWindowWMHints ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend WM input hint methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const noInputWindow = 0x710401;
    const inputWindow = 0x710402;
    const inputHint = 1;
    backend.createWindow(noInputWindow, rootWindow, 0, 0, 24, 24, 24, 0);
    backend.createWindow(inputWindow, rootWindow, 32, 0, 24, 24, 24, 0);
    backend.setWindowWMHints(
      noInputWindow,
      inputHint,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
    );
    backend.setWindowWMHints(
      inputWindow,
      inputHint,
      1,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
    );
    backend.mapWindow(noInputWindow);
    backend.mapWindow(inputWindow);
    backend.flush();
    return { noInputWindow, inputWindow };
  });

  const noInput = page.locator(`[data-x11-wasm-xid="${ids.noInputWindow}"]`);
  const input = page.locator(`[data-x11-wasm-xid="${ids.inputWindow}"]`);
  await expect(noInput).toHaveAttribute("data-x11-wasm-input-eligible", "0");
  await expect(input).toHaveAttribute("data-x11-wasm-input-eligible", "1");

  await page.evaluate(() => {
    const inputElement = document.querySelector(".x11-wasm-input");
    if (inputElement instanceof HTMLElement) {
      inputElement.blur();
    }
  });
  await noInput.click({ position: { x: 8, y: 8 } });
  expect(
    await page.evaluate(() =>
      document.activeElement?.classList.contains("x11-wasm-input"),
    ),
  ).toBe(false);

  await input.click({ position: { x: 8, y: 8 } });
  expect(
    await page.evaluate(() =>
      document.activeElement?.classList.contains("x11-wasm-input"),
    ),
  ).toBe(true);
});

test("browser backend refreshes icon bitmap when source pixmap changes", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const icon = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createPixmap?: Function, createWindow?: Function, mapWindow?: Function, drawRectangle?: Function, setWindowWMHints?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createPixmap ||
      !backend.createWindow ||
      !backend.mapWindow ||
      !backend.drawRectangle ||
      !backend.setWindowWMHints ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend icon bitmap methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const pixmapId = 0x710301;
    const windowId = 0x710302;
    const iconPixmapHint = 1 << 2;
    backend.createPixmap(pixmapId, rootWindow, 4, 4, 24);
    backend.drawRectangle(pixmapId, 0xff0000, 0, 0, 4, 4, true);
    backend.createWindow(windowId, rootWindow, 0, 0, 16, 12, 24, 0);
    backend.mapWindow(windowId);
    backend.setWindowWMHints(
      windowId,
      iconPixmapHint,
      0,
      0,
      pixmapId,
      0,
      0,
      0,
      0,
      0,
    );
    backend.flush();

    const resource = backend.resources.get(windowId);
    const element = resource?.element;
    if (!(element instanceof HTMLElement)) {
      throw new Error("missing icon bitmap refresh element");
    }
    const readIcon = () => {
      const documentIcon = document.querySelector("link[data-x11-wasm-icon]");
      return {
        href: element.getAttribute("data-x11-wasm-icon-bitmap-href") || "",
        documentHref: documentIcon?.getAttribute("href") || "",
      };
    };
    const initial = readIcon();
    backend.drawRectangle(pixmapId, 0x00ff00, 0, 0, 4, 4, true);
    backend.flush();
    const updated = readIcon();
    return { initial, updated };
  });

  expect(icon.initial.href).toContain("data:image/png");
  expect(icon.initial.documentHref).toBe(icon.initial.href);
  expect(icon.updated.href).toContain("data:image/png");
  expect(icon.updated.documentHref).toBe(icon.updated.href);
  expect(icon.updated.href).not.toBe(icon.initial.href);
});

test("XSetZoomHints updates browser zoom hint metadata", async ({ page }) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((id) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetZoomHints?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const resource = backend?.resources.get(id);
    const element = resource?.element;
    const root = backend?.rootWindow
      ? backend.resources.get(backend.rootWindow)
      : null;
    const stateBackend =
      /** @type {X11WasmBackendSnapshot & { setWindowState?: Function }} */ (
        backend
      );
    if (
      !module?._X11WasmTestSetZoomHints ||
      !resource ||
      !element ||
      !root ||
      typeof stateBackend?.setWindowState !== "function"
    ) {
      throw new Error("missing zoom hint test hooks");
    }

    const attrs = () => ({
      flags: element.getAttribute("data-x11-wasm-zoom-hints-flags"),
      x: element.getAttribute("data-x11-wasm-zoom-hint-x"),
      y: element.getAttribute("data-x11-wasm-zoom-hint-y"),
      width: element.getAttribute("data-x11-wasm-zoom-hint-width"),
      height: element.getAttribute("data-x11-wasm-zoom-hint-height"),
      minWidth: element.getAttribute("data-x11-wasm-zoom-min-width"),
      minHeight: element.getAttribute("data-x11-wasm-zoom-min-height"),
      maxWidth: element.getAttribute("data-x11-wasm-zoom-max-width"),
      maxHeight: element.getAttribute("data-x11-wasm-zoom-max-height"),
      widthInc: element.getAttribute("data-x11-wasm-zoom-width-inc"),
      heightInc: element.getAttribute("data-x11-wasm-zoom-height-inc"),
      minAspectX: element.getAttribute("data-x11-wasm-zoom-min-aspect-x"),
      minAspectY: element.getAttribute("data-x11-wasm-zoom-min-aspect-y"),
      maxAspectX: element.getAttribute("data-x11-wasm-zoom-max-aspect-x"),
      maxAspectY: element.getAttribute("data-x11-wasm-zoom-max-aspect-y"),
      baseWidth: element.getAttribute("data-x11-wasm-zoom-base-width"),
      baseHeight: element.getAttribute("data-x11-wasm-zoom-base-height"),
      winGravity: element.getAttribute("data-x11-wasm-zoom-win-gravity"),
    });
    const normalState = () => ({
      flags: element.getAttribute("data-x11-wasm-size-hints-flags"),
      minWidth: element.getAttribute("data-x11-wasm-min-width"),
      maxWidth: element.getAttribute("data-x11-wasm-max-width"),
      styleMinWidth: getComputedStyle(element).minWidth,
      styleMaxWidth: getComputedStyle(element).maxWidth,
    });
    const plainHints = () =>
      resource.zoomHints ? JSON.parse(JSON.stringify(resource.zoomHints)) : null;
    const geometry = () => ({
      fullscreen: resource.fullscreen,
      height: element.style.height,
      left: element.style.left,
      rootHeight: root.height,
      rootWidth: root.width,
      top: element.style.top,
      width: element.style.width,
      zoomed: resource.zoomed,
    });

    const beforeSet = normalState();
    stateBackend.setWindowState(id, 0, 1, 0);
    const beforeZoomHintGeometry = geometry();
    const setResult = module._X11WasmTestSetZoomHints(id, 1);
    const afterSet = {
      attrs: attrs(),
      geometry: geometry(),
      normalState: normalState(),
      resourceHints: plainHints(),
    };
    const clearResult = module._X11WasmTestSetZoomHints(id, 0);
    const afterClear = {
      attrs: attrs(),
      geometry: geometry(),
      normalState: normalState(),
      resourceHints: plainHints(),
    };
    return {
      afterClear,
      afterSet,
      beforeSet,
      beforeZoomHintGeometry,
      clearResult,
      setResult,
    };
  }, windowId);

  const expectedHints = {
    baseHeight: 45,
    baseWidth: 80,
    flags: 1020,
    height: 180,
    heightInc: 6,
    maxAspectX: 4,
    maxAspectY: 3,
    maxHeight: 360,
    maxWidth: 640,
    minAspectX: 16,
    minAspectY: 9,
    minHeight: 90,
    minWidth: 160,
    width: 320,
    widthInc: 8,
    winGravity: 9,
    x: 11,
    y: 13,
  };
  const expectedAttrs = {
    baseHeight: "45",
    baseWidth: "80",
    flags: "1020",
    height: "180",
    heightInc: "6",
    maxAspectX: "4",
    maxAspectY: "3",
    maxHeight: "360",
    maxWidth: "640",
    minAspectX: "16",
    minAspectY: "9",
    minHeight: "90",
    minWidth: "160",
    width: "320",
    widthInc: "8",
    winGravity: "9",
    x: "11",
    y: "13",
  };
  const clearedAttrs = Object.fromEntries(
    Object.keys(expectedAttrs).map((name) => [name, null]),
  );

  expect(metadata.setResult).toBe(0);
  expect(metadata.clearResult).toBe(0);
  expect(metadata.afterSet.attrs).toEqual(expectedAttrs);
  expect(metadata.afterSet.geometry).toMatchObject({
    fullscreen: false,
    height: "177px",
    left: "11px",
    top: "13px",
    width: "312px",
    zoomed: true,
  });
  expect(metadata.afterSet.resourceHints).toEqual(expectedHints);
  expect(metadata.afterSet.normalState).toEqual(metadata.beforeSet);
  expect(metadata.afterClear.attrs).toEqual(clearedAttrs);
  expect(metadata.afterClear.geometry).toMatchObject({
    fullscreen: false,
    zoomed: true,
  });
  expect(metadata.afterClear.geometry).toEqual(
    metadata.beforeZoomHintGeometry,
  );
  expect(metadata.afterClear.resourceHints).toBeNull();
  expect(metadata.afterClear.normalState).toEqual(metadata.beforeSet);
});

test("browser backend clears window size hints metadata and CSS", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const hints = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, setWindowAttributes?: Function, setWindowSizeHints?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.setWindowAttributes ||
      !backend.setWindowSizeHints ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend size hints methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x710101;
    const pMinSize = 1 << 4;
    const pMaxSize = 1 << 5;
    const pResizeInc = 1 << 6;
    const pAspect = 1 << 7;
    backend.createWindow(windowId, rootWindow, 0, 0, 80, 40, 24, 0);
    backend.mapWindow(windowId);
    backend.setWindowAttributes(windowId, 0, 0, 1, 1, 0, 0xffffffff, 0);
    backend.setWindowSizeHints(
      windowId,
      pMinSize | pMaxSize | pResizeInc | pAspect,
      0,
      0,
      0,
      0,
      100,
      50,
      200,
      120,
      4,
      5,
      16,
      9,
      16,
      9,
      0,
      0,
      9,
    );
    backend.setWindowSizeHints(
      windowId,
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
    backend.flush();

    const resource = backend.resources.get(windowId);
    const element = resource?.element;
    if (!(element instanceof HTMLElement)) {
      throw new Error("missing size hints clear test element");
    }
    const computed = getComputedStyle(element);
    return {
      flags: element.getAttribute("data-x11-wasm-size-hints-flags"),
      minWidth: element.getAttribute("data-x11-wasm-min-width"),
      maxWidth: element.getAttribute("data-x11-wasm-max-width"),
      widthInc: element.getAttribute("data-x11-wasm-width-inc"),
      winGravity: element.getAttribute("data-x11-wasm-win-gravity"),
      styleMinWidth: computed.minWidth,
      styleMaxWidth: computed.maxWidth,
      styleAspectRatio: element.style.aspectRatio,
    };
  });

  expect(hints.flags).toBeNull();
  expect(hints.minWidth).toBeNull();
  expect(hints.maxWidth).toBeNull();
  expect(hints.widthInc).toBeNull();
  expect(hints.winGravity).toBe("1");
  expect(hints.styleMinWidth).toBe("0px");
  expect(hints.styleMaxWidth).toBe("none");
  expect(hints.styleAspectRatio).toBe("");
});

test("browser backend applies resize increment size hints to configured windows", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const geometry = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, configureWindow?: Function, mapWindow?: Function, setWindowSizeHints?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.configureWindow ||
      !backend.mapWindow ||
      !backend.setWindowSizeHints ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend size hint geometry methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x710102;
    const pMinSize = 1 << 4;
    const pMaxSize = 1 << 5;
    const pResizeInc = 1 << 6;
    const pBaseSize = 1 << 8;
    backend.createWindow(windowId, rootWindow, 0, 0, 80, 40, 24, 0);
    backend.mapWindow(windowId);
    backend.setWindowSizeHints(
      windowId,
      pMinSize | pMaxSize | pResizeInc | pBaseSize,
      0,
      0,
      0,
      0,
      100,
      50,
      220,
      160,
      16,
      10,
      0,
      0,
      0,
      0,
      10,
      5,
      1,
    );
    backend.configureWindow(windowId, 0, 0, 137, 88, 0, 0);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const element = resource?.element;
    if (!(element instanceof HTMLElement) || !resource) {
      throw new Error("missing size hint geometry test element");
    }
    const rect = element.getBoundingClientRect();
    return {
      baseWidth: element.getAttribute("data-x11-wasm-base-width"),
      height: resource.height,
      heightInc: element.getAttribute("data-x11-wasm-height-inc"),
      pendingHeight: resource.pendingHeight,
      pendingWidth: resource.pendingWidth,
      rectHeight: Math.round(rect.height),
      rectWidth: Math.round(rect.width),
      styleHeight: element.style.height,
      styleWidth: element.style.width,
      width: resource.width,
      widthInc: element.getAttribute("data-x11-wasm-width-inc"),
    };
  });

  expect(geometry).toEqual({
    baseWidth: "10",
    height: 85,
    heightInc: "10",
    pendingHeight: 85,
    pendingWidth: 122,
    rectHeight: 85,
    rectWidth: 122,
    styleHeight: "85px",
    styleWidth: "122px",
    width: 122,
    widthInc: "16",
  });
});

test("browser backend constrains configured windows to aspect size hints", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const geometry = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, configureWindow?: Function, mapWindow?: Function, setWindowSizeHints?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.configureWindow ||
      !backend.mapWindow ||
      !backend.setWindowSizeHints ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend aspect hint methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x710103;
    const pAspect = 1 << 7;
    backend.createWindow(windowId, rootWindow, 0, 0, 80, 80, 24, 0);
    backend.mapWindow(windowId);
    backend.setWindowSizeHints(
      windowId,
      pAspect,
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
      4,
      3,
      16,
      9,
      0,
      0,
      1,
    );
    backend.configureWindow(windowId, 0, 0, 120, 300, 0, 0);
    backend.flush();
    const tallResource = backend.resources.get(windowId);
    const element = tallResource?.element;
    if (!(element instanceof HTMLElement) || !tallResource) {
      throw new Error("missing aspect hint test element");
    }
    const tallRect = element.getBoundingClientRect();
    const tallSnapshot = {
      height: tallResource.height,
      pendingHeight: tallResource.pendingHeight,
      pendingWidth: tallResource.pendingWidth,
      rectHeight: Math.round(tallRect.height),
      rectWidth: Math.round(tallRect.width),
      width: tallResource.width,
    };

    backend.configureWindow(windowId, 0, 0, 400, 100, 0, 0);
    backend.flush();
    const wideResource = backend.resources.get(windowId);
    if (!wideResource) {
      throw new Error("missing aspect hint test resource after resize");
    }
    const wideRect = element.getBoundingClientRect();
    const wideSnapshot = {
      height: wideResource.height,
      pendingHeight: wideResource.pendingHeight,
      pendingWidth: wideResource.pendingWidth,
      rectHeight: Math.round(wideRect.height),
      rectWidth: Math.round(wideRect.width),
      width: wideResource.width,
    };

    return {
      maxAspectX: element.getAttribute("data-x11-wasm-max-aspect-x"),
      maxAspectY: element.getAttribute("data-x11-wasm-max-aspect-y"),
      minAspectX: element.getAttribute("data-x11-wasm-min-aspect-x"),
      minAspectY: element.getAttribute("data-x11-wasm-min-aspect-y"),
      tallHeight: tallSnapshot.height,
      tallPendingHeight: tallSnapshot.pendingHeight,
      tallPendingWidth: tallSnapshot.pendingWidth,
      tallRectHeight: tallSnapshot.rectHeight,
      tallRectWidth: tallSnapshot.rectWidth,
      tallWidth: tallSnapshot.width,
      wideHeight: wideSnapshot.height,
      widePendingHeight: wideSnapshot.pendingHeight,
      widePendingWidth: wideSnapshot.pendingWidth,
      wideRectHeight: wideSnapshot.rectHeight,
      wideRectWidth: wideSnapshot.rectWidth,
      wideWidth: wideSnapshot.width,
    };
  });

  expect(geometry).toEqual({
    maxAspectX: "16",
    maxAspectY: "9",
    minAspectX: "4",
    minAspectY: "3",
    tallHeight: 300,
    tallPendingHeight: 300,
    tallPendingWidth: 400,
    tallRectHeight: 300,
    tallRectWidth: 400,
    tallWidth: 400,
    wideHeight: 225,
    widePendingHeight: 225,
    widePendingWidth: 400,
    wideRectHeight: 225,
    wideRectWidth: 400,
    wideWidth: 400,
  });
});

test("browser backend applies configure window value masks", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const snapshots = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, configureWindow?: Function, mapWindow?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.configureWindow ||
      !backend.mapWindow ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend configure methods");
    }

    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x710103;
    const cwX = 1 << 0;
    const cwY = 1 << 1;
    const cwWidth = 1 << 2;
    const cwHeight = 1 << 3;
    const cwBorderWidth = 1 << 4;

    backend.createWindow(windowId, rootWindow, 0, 0, 11, 13, 24, 0);
    backend.mapWindow(windowId);

    const snapshot = () => {
      backend.flush();
      const resource = backend.resources.get(windowId);
      const element = resource?.element;
      if (!(element instanceof HTMLElement) || !resource) {
        throw new Error("missing configure mask test element");
      }
      return {
        borderWidth: resource.borderWidth,
        height: resource.height,
        left: element.style.left,
        pendingBorderWidth: resource.pendingBorderWidth,
        pendingHeight: resource.pendingHeight,
        pendingWidth: resource.pendingWidth,
        pendingX: resource.pendingX,
        pendingY: resource.pendingY,
        styleBorderWidth: element.style.borderWidth,
        styleHeight: element.style.height,
        styleWidth: element.style.width,
        top: element.style.top,
        width: resource.width,
        x: resource.x,
        y: resource.y,
      };
    };

    const initial = snapshot();
    backend.configureWindow(windowId, 7, 9, 99, 88, 5, cwX | cwY);
    const positionOnly = snapshot();
    backend.configureWindow(windowId, 1, 2, 17, 99, 7, cwWidth);
    const widthOnly = snapshot();
    backend.configureWindow(
      windowId,
      3,
      4,
      200,
      21,
      6,
      cwHeight | cwBorderWidth,
    );
    const heightAndBorder = snapshot();

    return { heightAndBorder, initial, positionOnly, widthOnly };
  });

  expect(snapshots.initial).toEqual(
    expect.objectContaining({
      borderWidth: 0,
      height: 13,
      left: "0px",
      top: "0px",
      width: 11,
      x: 0,
      y: 0,
    }),
  );
  expect(snapshots.positionOnly).toEqual(
    expect.objectContaining({
      borderWidth: 0,
      height: 13,
      left: "7px",
      pendingHeight: 13,
      pendingWidth: 11,
      top: "9px",
      width: 11,
      x: 7,
      y: 9,
    }),
  );
  expect(snapshots.widthOnly).toEqual(
    expect.objectContaining({
      borderWidth: 0,
      height: 13,
      left: "7px",
      pendingWidth: 17,
      styleWidth: "17px",
      top: "9px",
      width: 17,
      x: 7,
      y: 9,
    }),
  );
  expect(snapshots.heightAndBorder).toEqual(
    expect.objectContaining({
      borderWidth: 6,
      height: 21,
      left: "7px",
      pendingBorderWidth: 6,
      pendingHeight: 21,
      styleBorderWidth: "6px",
      styleHeight: "21px",
      styleWidth: "17px",
      top: "9px",
      width: 17,
      x: 7,
      y: 9,
    }),
  );
});

test("browser backend overrides window cursors during pointer grab", async ({
  page,
}) => {
  await waitForX11Runtime(page);

  const cursors = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, setWindowStyle?: Function, setPointerGrabCursor?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.setWindowStyle ||
      !backend.setPointerGrabCursor ||
      !backend.flush
    ) {
      throw new Error("missing pointer grab cursor backend methods");
    }
    const targetId = 0x710801;
    backend.createWindow(targetId, backend.rootWindow || 1, 0, 0, 72, 48, 24, 0);
    backend.mapWindow(targetId);
    backend.setWindowStyle(targetId, false, 0, 0, false, 0, 0, true, 152);
    const root = document.querySelector("#x11-wasm-root");
    const target = document.querySelector(`[data-x11-wasm-xid="${targetId}"]`);
    if (!(root instanceof HTMLElement) || !(target instanceof HTMLElement)) {
      throw new Error("missing pointer grab cursor elements");
    }
    backend.flush();
    const before = getComputedStyle(target).cursor;
    backend.setPointerGrabCursor(true, 150);
    const grabbed = getComputedStyle(target).cursor;
    const classDuringGrab = root.classList.contains("x11-wasm-pointer-grab");
    backend.setPointerGrabCursor(false, 0);
    const after = getComputedStyle(target).cursor;
    const classAfterGrab = root.classList.contains("x11-wasm-pointer-grab");
    return { before, grabbed, after, classDuringGrab, classAfterGrab };
  });

  expect(cursors).toEqual({
    before: "text",
    grabbed: "wait",
    after: "text",
    classDuringGrab: true,
    classAfterGrab: false,
  });
});

test("browser backend applies WM_STATE visibility without remapping", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const states = await page.evaluate(() => {
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { createWindow?: Function, mapWindow?: Function, setWindowWmState?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.setWindowWmState
    ) {
      throw new Error("missing X11 wasm backend WM_STATE methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x710204;
    const withdrawnState = 0;
    const normalState = 1;
    const iconicState = 3;

    backend.createWindow(windowId, rootWindow, 12, 18, 96, 72, 24, 0);
    backend.mapWindow(windowId);
    backend.flush();
    const resource = backend.resources.get(windowId);
    const element = resource?.element;
    if (!(element instanceof HTMLElement) || !resource) {
      throw new Error("missing WM_STATE test element");
    }
    const snapshot = () => ({
      ariaHidden: element.getAttribute("aria-hidden"),
      display: element.style.display,
      mapped: resource.mapped,
      state: element.getAttribute("data-x11-wasm-wm-state"),
      viewable: element.getAttribute("data-x11-wasm-viewable"),
    });

    const mapped = snapshot();
    backend.setWindowWmState(windowId, iconicState);
    const iconic = snapshot();
    backend.setWindowWmState(windowId, withdrawnState);
    const withdrawn = snapshot();
    backend.setWindowWmState(windowId, normalState);
    const normal = snapshot();

    return { iconic, mapped, normal, withdrawn };
  });

  expect(states).toEqual({
    mapped: {
      ariaHidden: null,
      display: "",
      mapped: true,
      state: null,
      viewable: "1",
    },
    iconic: {
      ariaHidden: "true",
      display: "none",
      mapped: true,
      state: "iconic",
      viewable: "0",
    },
    withdrawn: {
      ariaHidden: "true",
      display: "none",
      mapped: true,
      state: "withdrawn",
      viewable: "0",
    },
    normal: {
      ariaHidden: null,
      display: "",
      mapped: true,
      state: "normal",
      viewable: "1",
    },
  });
});

test("XSetRGBColormaps updates browser standard colormap metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate((id) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetStandardColormaps?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const resource = backend?.resources.get(id);
    const element = resource?.element;
    if (
      !module?._X11WasmTestSetStandardColormaps ||
      !resource?.standardColormaps ||
      !element
    ) {
      throw new Error("missing standard colormap test hooks");
    }

	    const setResult = module._X11WasmTestSetStandardColormaps(id, 1);
	    const afterSet = {
	      attrCount: element.getAttribute("data-x11-wasm-standard-colormap-count"),
	      attrMaps: JSON.parse(
	        element.getAttribute("data-x11-wasm-standard-colormaps") || "{}",
	      ),
	      resourceMaps: JSON.parse(JSON.stringify(resource.standardColormaps)),
	    };
	    const clearResult = module._X11WasmTestSetStandardColormaps(id, 0);
	    const afterClear = {
	      attrCount: element.getAttribute("data-x11-wasm-standard-colormap-count"),
	      attrMaps: element.getAttribute("data-x11-wasm-standard-colormaps"),
	      resourceMaps: JSON.parse(JSON.stringify(resource.standardColormaps)),
	    };
    return { afterClear, afterSet, clearResult, setResult };
  }, windowId);

  const expectedMaps = {
    _X11_WASM_TEST_RGB_MAP: [
      {
        basePixel: 0,
        blueMax: 255,
        blueMult: 1,
        colormap: 2,
        greenMax: 255,
        greenMult: 256,
        killid: 1,
        redMax: 255,
        redMult: 65536,
        visualid: 1,
      },
      {
        basePixel: 16,
        blueMax: 31,
        blueMult: 1,
        colormap: 2,
        greenMax: 63,
        greenMult: 256,
        killid: 1,
        redMax: 31,
        redMult: 65536,
        visualid: 1,
      },
    ],
  };

  expect(metadata).toEqual({
    setResult: 0,
    clearResult: 0,
    afterSet: {
      attrCount: "1",
      attrMaps: expectedMaps,
      resourceMaps: expectedMaps,
    },
    afterClear: {
      attrCount: null,
      attrMaps: null,
      resourceMaps: {},
    },
  });
});
