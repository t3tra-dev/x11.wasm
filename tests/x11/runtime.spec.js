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

test("browser harness reaches a terminal startup state", async ({ page }) => {
  /** @type {string[]} */
  const consoleMessages = [];
  /** @type {string[]} */
  const pageErrors = [];

  page.on("console", (message) => {
    consoleMessages.push(`${message.type()}: ${message.text()}`);
  });
  page.on("pageerror", (error) => {
    pageErrors.push(error.stack || error.message);
  });

  await page.goto("/x11.html");
  const status = page.locator("#x11-wasm-status");
  try {
    await expect(status).not.toHaveText("loading", { timeout: 15000 });
  } catch (error) {
    throw new Error(
      [
        error instanceof Error ? error.message : String(error),
        "Console messages:",
        ...consoleMessages,
        "Page errors:",
        ...pageErrors,
      ].join("\n"),
    );
  }

  const statusText = (await status.textContent()) || "";
  expect(pageErrors, consoleMessages.join("\n")).toEqual([]);
  expect(statusText, consoleMessages.join("\n")).toBe("running");
  await expect(page.locator("#x11-wasm-root")).toHaveClass(/x11-wasm-root/);
  await expect(page.locator("#x11-wasm-root > canvas")).toHaveCount(1);
  const backendState = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (!backend) {
      throw new Error("missing X11 wasm backend");
    }
    backend.flush();
    const resources = Array.from(backend.resources.values());
    const mappedWindows = resources.filter(
      (resource) => resource.kind === "window" && resource.mapped,
    );
    return {
      dirtyCount: backend.dirtyResources.size,
      flushScheduled: backend.flushScheduled,
      mappedWindowCount: mappedWindows.length,
      pendingMismatchCount: resources.filter(
        (resource) =>
          resource.x !== resource.pendingX ||
          resource.y !== resource.pendingY ||
          resource.width !== resource.pendingWidth ||
          resource.height !== resource.pendingHeight ||
          resource.mapped !== resource.pendingMapped,
      ).length,
      geometryMismatchCount: mappedWindows.filter(
        (resource) =>
          !resource.element ||
          resource.element.style.display === "none" ||
          !resource.canvas ||
          resource.canvas.width !== resource.width ||
          resource.canvas.height !== resource.height,
      ).length,
    };
  });
  expect(backendState.mappedWindowCount).toBe(0);
  expect(backendState.pendingMismatchCount).toBe(0);
  expect(backendState.geometryMismatchCount).toBe(0);
  expect(backendState.dirtyCount).toBe(0);
  expect(backendState.flushScheduled).toBe(false);
});

test("browser wheel events use the exported x11.wasm event constant", async ({
  page,
}) => {
  await waitForX11Runtime(page);

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestWheelEventConstants?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestWheelEventConstants) {
      throw new Error("missing wheel event constant test hook");
    }
    return module._X11WasmTestWheelEventConstants();
  });

  expect(result).toBe(1);
});

test("X11 keyboard mapping APIs resolve ASCII key events", async ({ page }) => {
  await waitForX11Runtime(page);

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestKeyboardAsciiMapping?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestKeyboardAsciiMapping) {
      throw new Error("missing X11 keyboard mapping test export");
    }
    return module._X11WasmTestKeyboardAsciiMapping();
  });

  expect(result).toBe(1);
});

test("X event queue and display keycode APIs preserve ordering", async ({
  page,
}) => {
  await waitForX11Runtime(page);

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestEventQueueAndKeycodeApis?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestEventQueueAndKeycodeApis) {
      throw new Error("missing X event queue and keycode test export");
    }
    return module._X11WasmTestEventQueueAndKeycodeApis();
  });

  expect(result).toBe(1);
});

test("XIC spot location anchors the browser input context", async ({ page }) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const callSpotHook = async (phase) =>
    page.evaluate(
      ({ windowId, phase }) => {
        const module =
          /** @type {Window & { Module?: { _X11WasmTestXICSpotLocationSync?: Function } }} */ (
            window
          ).Module;
        if (!module?._X11WasmTestXICSpotLocationSync) {
          throw new Error("missing XIC spot location test hook");
        }
        return module._X11WasmTestXICSpotLocationSync(windowId, phase);
      },
      { windowId, phase },
    );

  const readInputContext = async () =>
    page.evaluate(() => {
      const input = document.querySelector(".x11-wasm-input");
      const module =
        /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
          window
        ).Module;
      if (!(input instanceof HTMLTextAreaElement)) {
        throw new Error("missing browser input textarea");
      }
      return {
        focused: input.getAttribute("data-x11-wasm-ic-focused"),
        window: input.getAttribute("data-x11-wasm-ic-window"),
        hasSpot: input.getAttribute("data-x11-wasm-ic-has-spot"),
        spotX: input.getAttribute("data-x11-wasm-ic-spot-x"),
        spotY: input.getAttribute("data-x11-wasm-ic-spot-y"),
        styleLeft: input.style.left,
        styleTop: input.style.top,
        backendFocused: module?.x11WasmBackend?.inputContextFocused,
        backendWindow: module?.x11WasmBackend?.inputContextWindow,
        backendHasSpot: module?.x11WasmBackend?.inputContextHasSpot,
        backendSpotX: module?.x11WasmBackend?.inputContextSpotX,
        backendSpotY: module?.x11WasmBackend?.inputContextSpotY,
      };
    });

  try {
    expect(await callSpotHook(1)).toBe(1);
    const phaseOne = await readInputContext();
    expect(phaseOne.focused).toBe("1");
    expect(phaseOne.window).toBe(String(windowId));
    expect(phaseOne.hasSpot).toBe("1");
    expect(phaseOne.backendFocused).toBe(true);
    expect(phaseOne.backendWindow).toBe(windowId);
    expect(phaseOne.backendHasSpot).toBe(true);
    expect(phaseOne.backendSpotX).toBe(Number(phaseOne.spotX));
    expect(phaseOne.backendSpotY).toBe(Number(phaseOne.spotY));
    expect(phaseOne.styleLeft).toBe(`${phaseOne.spotX}px`);
    expect(phaseOne.styleTop).toBe(`${phaseOne.spotY}px`);

    expect(await callSpotHook(2)).toBe(1);
    const phaseTwo = await readInputContext();
    expect(phaseTwo.focused).toBe("1");
    expect(phaseTwo.window).toBe(String(windowId));
    expect(phaseTwo.hasSpot).toBe("1");
    expect(phaseTwo.backendFocused).toBe(true);
    expect(phaseTwo.backendWindow).toBe(windowId);
    expect(phaseTwo.backendHasSpot).toBe(true);
    expect(phaseTwo.backendSpotX).toBe(Number(phaseTwo.spotX));
    expect(phaseTwo.backendSpotY).toBe(Number(phaseTwo.spotY));
    expect(phaseTwo.styleLeft).toBe(`${phaseTwo.spotX}px`);
    expect(phaseTwo.styleTop).toBe(`${phaseTwo.spotY}px`);
    expect(Number(phaseTwo.spotX) - Number(phaseOne.spotX)).toBe(12);
    expect(Number(phaseTwo.spotY) - Number(phaseOne.spotY)).toBe(12);
  } finally {
    await callSpotHook(0);
  }

  expect(await readInputContext()).toEqual({
    focused: null,
    window: null,
    hasSpot: null,
    spotX: null,
    spotY: null,
    styleLeft: "0px",
    styleTop: "0px",
    backendFocused: false,
    backendWindow: 0,
    backendHasSpot: false,
    backendSpotX: 0,
    backendSpotY: 0,
  });
});

test("browser backend represents InputOnly windows without drawable canvas", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const snapshot = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (!backend?.createWindow || !backend.mapWindow || !backend.flush) {
      throw new Error("missing X11 wasm backend window methods");
    }
    const id = 0x730101;
    const rootWindow = backend.rootWindow || 1;
    backend.createWindow(id, rootWindow, 3, 4, 20, 12, 0, 5, 2);
    backend.mapWindow(id);
    backend.flush();
    const resource = backend.resources.get(id);
    const element = resource?.element;
    return {
      kind: resource?.kind,
      windowClass: resource?.windowClass,
      hasCanvas: !!resource?.canvas,
      childCanvasCount: element?.querySelectorAll("canvas").length ?? -1,
      className: element?.className || "",
      eventMask: element?.getAttribute("data-x11-wasm-event-mask") || "",
      classAttr: element?.getAttribute("data-x11-wasm-window-class") || "",
      display: element instanceof HTMLElement ? element.style.display : "",
    };
  });

  expect(snapshot).toEqual({
    kind: "window",
    windowClass: 2,
    hasCanvas: false,
    childCanvasCount: 0,
    className: "x11-wasm-window x11-wasm-input-only",
    eventMask: "5",
    classAttr: "2",
    display: "",
  });
});

test("XEventsQueued QueuedAfterFlush flushes browser updates", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const state = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const module =
      /** @type {Window & { Module?: { _XEventsQueued?: (display: number, mode: number) => number, x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, drawRectangle?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const eventsQueued = module?._XEventsQueued;
    if (
      !backend?.createWindow ||
      !backend.drawRectangle ||
      !backend.flush ||
      !eventsQueued
    ) {
      throw new Error("missing XEventsQueued browser test hooks");
    }
    backend.flush();
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x710102;
    backend.createWindow(windowId, rootWindow, 3, 4, 24, 18, 24, 0);
    backend.drawRectangle(windowId, 0xff0000, 0, 0, 12, 8, 1);

    const dirtyAfterCreate = backend.dirtyResources.size;
    const queuedAlready = eventsQueued(0, 0);
    const dirtyAfterAlready = backend.dirtyResources.size;
    const queuedAfterFlush = eventsQueued(0, 2);

    return {
      dirtyAfterCreate,
      dirtyAfterAlready,
      dirtyAfterFlush: backend.dirtyResources.size,
      flushScheduled: backend.flushScheduled,
      queuedAlready,
      queuedAfterFlush,
    };
  });

  expect(state.dirtyAfterCreate).toBeGreaterThan(0);
  expect(state.dirtyAfterAlready).toBe(state.dirtyAfterCreate);
  expect(state.dirtyAfterFlush).toBe(0);
  expect(state.flushScheduled).toBe(false);
  expect(state.queuedAlready).toBeGreaterThanOrEqual(0);
  expect(state.queuedAfterFlush).toBeGreaterThanOrEqual(0);
});

test("X request completion updates browser request metadata", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const state = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestCompleteRequests?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    if (!module?._X11WasmTestCompleteRequests || !backend || !root) {
      throw new Error("missing request metadata test hooks");
    }
    const read = () => ({
      flushSerial: backend.flushSerial,
      requestSerial: backend.requestSerial,
      rootFlushSerial: root.getAttribute("data-x11-wasm-flush-serial"),
      rootRequestSerial: root.getAttribute("data-x11-wasm-request-serial"),
      rootSyncSerial: root.getAttribute("data-x11-wasm-sync-serial"),
      syncSerial: backend.syncSerial,
    });

    const before = read();
    const result = module._X11WasmTestCompleteRequests(2, 1, 1, 0);
    const after = read();
    return { after, before, result };
  });

  expect(state.result).toBe(0);
  expect(state.after.requestSerial - state.before.requestSerial).toBe(4);
  expect(state.after.flushSerial - state.before.flushSerial).toBe(2);
  expect(state.after.syncSerial - state.before.syncSerial).toBe(1);
  expect(state.after.rootRequestSerial).toBe(
    String(state.after.requestSerial),
  );
  expect(state.after.rootFlushSerial).toBe(String(state.after.flushSerial));
  expect(state.after.rootSyncSerial).toBe(String(state.after.syncSerial));
});

test("XSynchronize updates browser synchronize metadata", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const state = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetSynchronizeState?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    if (!module?._X11WasmTestSetSynchronizeState || !backend || !root) {
      throw new Error("missing synchronize metadata test hooks");
    }
    const read = () => ({
      dirtyCount: backend.dirtyResources.size,
      flushScheduled: backend.flushScheduled,
      flushSerial: backend.flushSerial,
      requestSerial: backend.requestSerial,
      rootSynchronizeActive: root.getAttribute("data-x11-wasm-synchronize-active"),
      syncSerial: backend.syncSerial,
      synchronizeEnabled: backend.synchronizeEnabled,
    });

    const resetResult = module._X11WasmTestSetSynchronizeState(0);
    const before = read();
    backend.createWindow?.(
      0x740501,
      backend.rootWindow || 1,
      3,
      4,
      12,
      8,
      24,
      0,
    );
    backend.mapWindow?.(0x740501);
    const pendingWithoutSync = read();
    backend.flush();
    const onResult = module._X11WasmTestSetSynchronizeState(1);
    const afterOn = read();
    backend.createWindow?.(
      0x740502,
      backend.rootWindow || 1,
      6,
      7,
      10,
      6,
      24,
      0,
    );
    backend.mapWindow?.(0x740502);
    const flushedWithSync = read();
    const offResult = module._X11WasmTestSetSynchronizeState(0);
    const afterOff = read();
    return {
      afterOff,
      afterOn,
      before,
      flushedWithSync,
      offResult,
      onResult,
      pendingWithoutSync,
      resetResult,
    };
  });

  expect(state.resetResult).toBe(0);
  expect(state.onResult).toBe(0);
  expect(state.offResult).toBe(0);
  expect(state.before.synchronizeEnabled).toBe(false);
  expect(state.before.rootSynchronizeActive).toBe("0");
  expect(state.pendingWithoutSync.dirtyCount).toBeGreaterThan(0);
  expect(state.pendingWithoutSync.flushScheduled).toBe(true);
  expect(state.afterOn.synchronizeEnabled).toBe(true);
  expect(state.afterOn.rootSynchronizeActive).toBe("1");
  expect(state.afterOn.dirtyCount).toBe(0);
  expect(state.afterOn.flushScheduled).toBe(false);
  expect(state.afterOn.requestSerial - state.before.requestSerial).toBe(1);
  expect(state.afterOn.flushSerial - state.before.flushSerial).toBe(1);
  expect(state.afterOn.syncSerial - state.before.syncSerial).toBe(1);
  expect(state.flushedWithSync.dirtyCount).toBe(0);
  expect(state.flushedWithSync.flushScheduled).toBe(false);
  expect(state.afterOff.synchronizeEnabled).toBe(false);
  expect(state.afterOff.rootSynchronizeActive).toBe("0");
  expect(state.afterOff.requestSerial - state.afterOn.requestSerial).toBe(1);
  expect(state.afterOff.flushSerial).toBe(state.afterOn.flushSerial);
  expect(state.afterOff.syncSerial).toBe(state.afterOn.syncSerial);
});

test("Xlib extension and error text compatibility APIs are stable", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestXlibApis?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestXlibApis) {
      throw new Error("missing Xlib test hooks");
    }
    return module._X11WasmTestXlibApis();
  });

  expect(result).toBe(1);
});

test("Xlib colormap compatibility APIs are stable", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestColormapApis?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestColormapApis) {
      throw new Error("missing Xlib colormap test hooks");
    }
    return module._X11WasmTestColormapApis();
  });

  expect(result).toBe(1);
});

test("X resource manager defaults are stable", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestResourceManagerApis?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestResourceManagerApis) {
      throw new Error("missing X resource manager test hooks");
    }
    return module._X11WasmTestResourceManagerApis();
  });

  expect(result).toBe(1);
});

test("XSendEvent resolves InputFocus and PointerWindow targets", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const [focusWindowId, pointerWindowId] = await createMappedWindows(page, 2);

  const result = await page.evaluate(
    ({ focusWindowId, pointerWindowId }) => {
      const module =
        /** @type {Window & { Module?: { _X11WasmTestXSendEventTargets?: Function } }} */ (
          window
        ).Module;
      if (!module?._X11WasmTestXSendEventTargets) {
        throw new Error("missing XSendEvent target test hook");
      }
      return module._X11WasmTestXSendEventTargets(
        focusWindowId,
        pointerWindowId,
      );
    },
    { focusWindowId, pointerWindowId },
  );

  expect(result).toBe(1);
});

test("external clipboard selection returns target-specific text properties", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestExternalClipboardTargets?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestExternalClipboardTargets) {
      throw new Error("missing external clipboard target test hook");
    }
    return module._X11WasmTestExternalClipboardTargets();
  });

  expect(result).toBe(1);
});

test("XOpenDisplay rejects non-browser display names", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestOpenDisplayNames?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestOpenDisplayNames) {
      throw new Error("missing XOpenDisplay test hook");
    }
    return module._X11WasmTestOpenDisplayNames();
  });

  expect(result).toBe(1);
});

test("X error handler receives invalid resource errors", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestXErrorHandler?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestXErrorHandler) {
      throw new Error("missing X error handler test hook");
    }
    return module._X11WasmTestXErrorHandler();
  });

  expect(result).toBe(1);
});

test("XIC lookup honors focused input context", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestXICLookupFocus?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestXICLookupFocus) {
      throw new Error("missing XIC lookup test hook");
    }
    return module._X11WasmTestXICLookupFocus();
  });

  expect(result).toBe(1);
});
