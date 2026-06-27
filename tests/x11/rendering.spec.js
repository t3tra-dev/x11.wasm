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

test("XClearArea queues Expose events only when requested", async ({ page }) => {
  await waitForX11Runtime(page);

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestClearAreaApis?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestClearAreaApis) {
      throw new Error("missing XClearArea test export");
    }
    return module._X11WasmTestClearAreaApis();
  });

  expect(result).toBe(1);
});

test("browser backend copyPlane honors the requested source plane", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createPixmap?: Function, createWindow?: Function, mapWindow?: Function, drawRectangle?: Function, copyPlane?: Function, setDrawStyle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createPixmap ||
      !backend.createWindow ||
      !backend.mapWindow ||
      !backend.drawRectangle ||
      !backend.copyPlane ||
      !backend.setDrawStyle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend copyPlane methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const source = 0x720601;
    const target = 0x720602;
    /** @param {number} functionId */
    const setFunction = (functionId) =>
      backend.setDrawStyle(
        functionId,
        1,
        0,
        1,
        0,
        0,
        4,
        [],
        0,
        0,
        0,
        0,
        0,
        0xffffff,
        14,
        "sans-serif",
      );
    backend.createPixmap(source, rootWindow, 2, 1, 24);
    backend.createWindow(target, rootWindow, 0, 0, 2, 4, 24, 0);
    backend.mapWindow(target);
    backend.drawRectangle(source, 0xff0000, 0, 0, 1, 1, true);
    backend.drawRectangle(source, 0x00ff00, 1, 0, 1, 1, true);
    backend.copyPlane(
      source,
      target,
      0,
      0,
      2,
      1,
      0,
      0,
      0x00ffff,
      0x0000ff,
      0xff0000,
      false,
    );
    backend.copyPlane(
      source,
      target,
      0,
      0,
      2,
      1,
      0,
      1,
      0x00ffff,
      0x0000ff,
      0x00ff00,
      false,
    );
    backend.drawRectangle(target, 0x0000ff, 0, 2, 2, 2, true);
    setFunction(6);
    backend.copyPlane(
      source,
      target,
      0,
      0,
      2,
      1,
      0,
      2,
      0xff0000,
      0x00ff00,
      0xff0000,
      false,
    );
    setFunction(5);
    backend.copyPlane(
      source,
      target,
      0,
      0,
      2,
      1,
      0,
      3,
      0xff0000,
      0x00ff00,
      0xff0000,
      false,
    );
    backend.flush();

    const resource = backend.resources.get(target);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing copyPlane target canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing copyPlane target context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      redPlaneRedSource: pixel(0, 0),
      redPlaneGreenSource: pixel(1, 0),
      greenPlaneRedSource: pixel(0, 1),
      greenPlaneGreenSource: pixel(1, 1),
      xorForeground: pixel(0, 2),
      xorBackground: pixel(1, 2),
      noopForeground: pixel(0, 3),
      noopBackground: pixel(1, 3),
    };
  });

  for (const sample of [
    samples.redPlaneRedSource,
    samples.greenPlaneGreenSource,
  ]) {
    expect(sample[0]).toBeLessThan(80);
    expect(sample[1]).toBeGreaterThan(180);
    expect(sample[2]).toBeGreaterThan(180);
    expect(sample[3]).toBe(255);
  }
  for (const sample of [
    samples.redPlaneGreenSource,
    samples.greenPlaneRedSource,
  ]) {
    expect(sample[0]).toBeLessThan(80);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeGreaterThan(180);
    expect(sample[3]).toBe(255);
  }
  expect(samples.xorForeground).toEqual([255, 0, 255, 255]);
  expect(samples.xorBackground).toEqual([0, 255, 255, 255]);
  expect(samples.noopForeground).toEqual([0, 0, 255, 255]);
  expect(samples.noopBackground).toEqual([0, 0, 255, 255]);
});

test("browser backend clips copyPlane to drawable bounds", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createPixmap?: Function, createWindow?: Function, mapWindow?: Function, drawRectangle?: Function, copyPlane?: Function, setDrawStyle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createPixmap ||
      !backend.createWindow ||
      !backend.mapWindow ||
      !backend.drawRectangle ||
      !backend.copyPlane ||
      !backend.setDrawStyle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend copyPlane clipping methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const source = 0x720607;
    const target = 0x720608;
    backend.createPixmap(source, rootWindow, 2, 1, 24);
    backend.createWindow(target, rootWindow, 0, 0, 4, 2, 24, 0);
    backend.mapWindow(target);
    backend.drawRectangle(source, 0xff0000, 0, 0, 1, 1, true);
    backend.drawRectangle(source, 0x00ff00, 1, 0, 1, 1, true);
    backend.drawRectangle(target, 0x0000ff, 0, 0, 4, 2, true);
    backend.setDrawStyle(
      3,
      1,
      0,
      1,
      0,
      0,
      4,
      [],
      0,
      0,
      0,
      0,
      0,
      0xffffff,
      14,
      "sans-serif",
    );
    backend.copyPlane(
      source,
      target,
      -1,
      0,
      3,
      1,
      0,
      0,
      0x00ffff,
      0xffff00,
      0xff0000,
      false,
    );
    backend.copyPlane(
      source,
      target,
      0,
      0,
      3,
      1,
      -1,
      1,
      0x00ffff,
      0xffff00,
      0xff0000,
      false,
    );
    backend.flush();

    const resource = backend.resources.get(target);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing copyPlane clipping target canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing copyPlane clipping target context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      sourceLeftOutside: pixel(0, 0),
      redPlane: pixel(1, 0),
      greenNoRedPlane: pixel(2, 0),
      visibleAfterNegativeDest: pixel(0, 1),
      untouchedAfterNegativeDest: pixel(1, 1),
    };
  });

  expect(samples.sourceLeftOutside).toEqual([0, 0, 255, 255]);
  expect(samples.redPlane).toEqual([0, 255, 255, 255]);
  expect(samples.greenNoRedPlane).toEqual([255, 255, 0, 255]);
  expect(samples.visibleAfterNegativeDest).toEqual([255, 255, 0, 255]);
  expect(samples.untouchedAfterNegativeDest).toEqual([0, 0, 255, 255]);
});

test("browser backend applies clip rect to canvas drawing", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, setClipRect?: Function, drawRectangle?: Function, clearWindow?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (!backend?.setClipRect || !backend.drawRectangle || !backend.clearWindow) {
      throw new Error("missing X11 wasm backend clip methods");
    }
    const rootWindow = backend.rootWindow || 1;
    backend.clearWindow(rootWindow);
    backend.setClipRect(true, 20, 20, 30, 30);
    backend.drawRectangle(rootWindow, 0xff0000, 0, 0, 80, 80, true);
    backend.setClipRect(false, 0, 0, 0, 0);
    backend.flush();

    const canvas = document.querySelector("#x11-wasm-root > canvas");
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing root canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing root canvas context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      inside: pixel(25, 25),
      outside: pixel(10, 10),
    };
  });

  expect(samples.inside[0]).toBeGreaterThan(180);
  expect(samples.inside[1]).toBeLessThan(80);
  expect(samples.inside[2]).toBeLessThan(80);
  expect(samples.inside[3]).toBe(255);
  expect(samples.outside[0]).toBeLessThan(80);
  expect(samples.outside[3]).toBe(0);
});

test("Xlib GC clip rectangles and empty regions constrain drawing", async ({
  page,
}) => {
  await waitForX11Runtime(page);

  const samples = await page.evaluate(async () => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestGCClipRendering?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (!module?._X11WasmTestGCClipRendering || !backend) {
      throw new Error("missing Xlib GC clip rendering test hooks");
    }
    const windowId = module._X11WasmTestGCClipRendering() >>> 0;
    backend.flush?.();
    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing Xlib GC clip rendering canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing Xlib GC clip rendering context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      firstClip: pixel(12, 12),
      gap: pixel(34, 12),
      secondClip: pixel(50, 12),
      regionClip: pixel(34, 32),
      regionMiss: pixel(44, 32),
      untouchedBase: pixel(70, 12),
      windowId,
    };
  });

  for (const sample of [samples.firstClip, samples.secondClip]) {
    expect(sample[0]).toBeGreaterThan(180);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeLessThan(80);
    expect(sample[3]).toBe(255);
  }
  for (const sample of [samples.gap, samples.untouchedBase]) {
    expect(sample[0]).toBeLessThan(80);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeGreaterThan(180);
    expect(sample[3]).toBe(255);
  }
  expect(samples.regionClip[0]).toBeLessThan(80);
  expect(samples.regionClip[1]).toBeGreaterThan(180);
  expect(samples.regionClip[2]).toBeLessThan(80);
  expect(samples.regionClip[3]).toBe(255);
  expect(samples.regionMiss[0]).toBeLessThan(80);
  expect(samples.regionMiss[1]).toBeLessThan(80);
  expect(samples.regionMiss[2]).toBeGreaterThan(180);
  expect(samples.regionMiss[3]).toBe(255);
  expect(samples.windowId).toBeGreaterThan(0);
});

test("Xlib GC subwindow_mode clips visible child windows", async ({ page }) => {
  await waitForX11Runtime(page);

  const samples = await page.evaluate(async () => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSubwindowModeRendering?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (!module?._X11WasmTestSubwindowModeRendering || !backend) {
      throw new Error("missing Xlib GC subwindow_mode rendering test hooks");
    }
    const windowId = module._X11WasmTestSubwindowModeRendering() >>> 0;
    backend.flush?.();
    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing Xlib GC subwindow_mode rendering canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing Xlib GC subwindow_mode rendering context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      clippedParent: pixel(4, 4),
      clippedInferior: pixel(20, 10),
      includedInferior: pixel(28, 10),
      includedParent: pixel(40, 4),
      windowId,
    };
  });

  expect(samples.clippedParent).toEqual([255, 0, 0, 255]);
  expect(samples.clippedInferior).toEqual([0, 0, 255, 255]);
  expect(samples.includedInferior).toEqual([0, 255, 0, 255]);
  expect(samples.includedParent).toEqual([0, 255, 0, 255]);
  expect(samples.windowId).toBeGreaterThan(0);
});

test("Xlib XPutImage keeps pixmap backing constrained by GC clip", async ({
  page,
}) => {
  await waitForX11Runtime(page);

  const result = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestPutImageClipBacking?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestPutImageClipBacking) {
      throw new Error("missing XPutImage clip backing test export");
    }
    return module._X11WasmTestPutImageClipBacking();
  });

  expect(result).toBe(1);
});

test("Xlib XCopyArea snapshots overlapping self copies", async ({ page }) => {
  await waitForX11Runtime(page);

  const samples = await page.evaluate(async () => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestCopyAreaSelfOverlapRendering?: Function, x11WasmBackend?: X11WasmBackendSnapshot & { flush?: Function } } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    if (!module?._X11WasmTestCopyAreaSelfOverlapRendering || !backend) {
      throw new Error("missing Xlib XCopyArea self-overlap test hooks");
    }
    const windowId = module._X11WasmTestCopyAreaSelfOverlapRendering() >>> 0;
    backend.flush?.();
    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing XCopyArea self-overlap canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing XCopyArea self-overlap context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      rightCopySource: pixel(0, 1),
      rightCopyFirst: pixel(1, 1),
      rightCopySecond: pixel(2, 1),
      rightCopyFifth: pixel(5, 1),
      leftCopyFirst: pixel(0, 4),
      leftCopySecond: pixel(1, 4),
      leftCopyFifth: pixel(4, 4),
      leftCopyTail: pixel(5, 4),
      windowId,
    };
  });

  expect(samples.rightCopySource).toEqual([255, 0, 0, 255]);
  expect(samples.rightCopyFirst).toEqual([255, 0, 0, 255]);
  expect(samples.rightCopySecond).toEqual([0, 255, 0, 255]);
  expect(samples.rightCopyFifth).toEqual([0, 255, 255, 255]);
  expect(samples.leftCopyFirst).toEqual([0, 255, 0, 255]);
  expect(samples.leftCopySecond).toEqual([0, 0, 255, 255]);
  expect(samples.leftCopyFifth).toEqual([255, 0, 255, 255]);
  expect(samples.leftCopyTail).toEqual([255, 0, 255, 255]);
  expect(samples.windowId).toBeGreaterThan(0);
});

test("browser backend applies GC raster operations", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, drawRectangle?: Function, copyArea?: Function, setDrawStyle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.drawRectangle ||
      !backend.copyArea ||
      !backend.setDrawStyle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend raster op methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x720301;
    const sourceId = 0x720302;
    const transparentSourceId = 0x720303;
    const truthTargetId = 0x720304;
    const truthSourceId = 0x720305;
    /** @param {number} functionId */
    const setFunction = (functionId) =>
      backend.setDrawStyle(
        functionId,
        1,
        0,
        1,
        0,
        0,
        4,
        [],
        0,
        0,
        0,
        0,
        0,
        0xffffff,
        14,
        "sans-serif",
      );

    backend.createWindow(windowId, rootWindow, 0, 0, 5, 3, 24, 0);
    backend.mapWindow(windowId);
    backend.drawRectangle(windowId, 0x0000ff, 0, 0, 5, 3, true);
    setFunction(0);
    backend.drawRectangle(windowId, 0xff0000, 1, 1, 2, 1, true);
    setFunction(5);
    backend.drawRectangle(windowId, 0xff0000, 0, 0, 1, 1, true);
    setFunction(15);
    backend.drawRectangle(windowId, 0x000000, 2, 0, 1, 1, true);
    setFunction(10);
    backend.drawRectangle(windowId, 0x000000, 3, 0, 1, 1, true);
    setFunction(7);
    backend.drawRectangle(windowId, 0xff0000, 4, 0, 1, 1, true);
    setFunction(1);
    backend.drawRectangle(windowId, 0xff0000, 0, 2, 1, 1, true);
    backend.createWindow(sourceId, rootWindow, 0, 0, 1, 1, 24, 0);
    setFunction(3);
    backend.drawRectangle(sourceId, 0xff0000, 0, 0, 1, 1, true);
    setFunction(6);
    backend.copyArea(sourceId, windowId, 0, 0, 1, 1, 2, 2);
    backend.createWindow(transparentSourceId, rootWindow, 0, 0, 1, 1, 24, 0);
    setFunction(3);
    backend.copyArea(transparentSourceId, windowId, 0, 0, 1, 1, 3, 2);

    const rasterSourcePixel = 0xcc6633;
    const rasterDestinationPixel = 0x3366aa;
    backend.createWindow(truthTargetId, rootWindow, 0, 0, 16, 2, 24, 0);
    backend.mapWindow(truthTargetId);
    backend.createWindow(truthSourceId, rootWindow, 0, 0, 16, 1, 24, 0);
    setFunction(3);
    backend.drawRectangle(
      truthTargetId,
      rasterDestinationPixel,
      0,
      0,
      16,
      2,
      true,
    );
    backend.drawRectangle(truthSourceId, rasterSourcePixel, 0, 0, 16, 1, true);
    for (let functionId = 0; functionId < 16; functionId++) {
      setFunction(functionId);
      backend.drawRectangle(
        truthTargetId,
        rasterSourcePixel,
        functionId,
        0,
        1,
        1,
        true,
      );
      backend.copyArea(
        truthSourceId,
        truthTargetId,
        functionId,
        0,
        1,
        1,
        functionId,
        1,
      );
    }
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing raster op test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing raster op test context");
    }
    const truthResource = backend.resources.get(truthTargetId);
    const truthCanvas = truthResource?.canvas;
    if (!(truthCanvas instanceof HTMLCanvasElement)) {
      throw new Error("missing raster op truth table canvas");
    }
    const truthContext = truthCanvas.getContext("2d");
    if (!truthContext) {
      throw new Error("missing raster op truth table context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    const truthPixel = (x, y) =>
      Array.from(truthContext.getImageData(x, y, 1, 1).data);
    return {
      noopTarget: pixel(0, 0),
      cleared: pixel(1, 1),
      setTarget: pixel(2, 0),
      invertTarget: pixel(3, 0),
      orTarget: pixel(4, 0),
      andTarget: pixel(0, 2),
      copyXorTarget: pixel(2, 2),
      transparentCopyTarget: pixel(3, 2),
      untouched: pixel(4, 2),
      truthFill: Array.from({ length: 16 }, (_, functionId) =>
        truthPixel(functionId, 0),
      ),
      truthCopy: Array.from({ length: 16 }, (_, functionId) =>
        truthPixel(functionId, 1),
      ),
    };
  });

  for (const sample of [samples.noopTarget, samples.untouched]) {
    expect(sample[0]).toBeLessThan(80);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeGreaterThan(180);
    expect(sample[3]).toBe(255);
  }
  expect(samples.cleared[3]).toBe(0);
  expect(samples.setTarget).toEqual([255, 255, 255, 255]);
  expect(samples.invertTarget).toEqual([255, 255, 0, 255]);
  expect(samples.orTarget).toEqual([255, 0, 255, 255]);
  expect(samples.andTarget).toEqual([0, 0, 0, 255]);
  expect(samples.copyXorTarget).toEqual([255, 0, 255, 255]);
  expect(samples.transparentCopyTarget[3]).toBe(0);

  const rasterByte = (functionId, source, destination) => {
    const src = source & 0xff;
    const dst = destination & 0xff;
    switch (functionId) {
      case 0:
        return 0;
      case 1:
        return src & dst;
      case 2:
        return src & ~dst & 0xff;
      case 3:
        return src;
      case 4:
        return ~src & dst & 0xff;
      case 5:
        return dst;
      case 6:
        return src ^ dst;
      case 7:
        return src | dst;
      case 8:
        return ~(src | dst) & 0xff;
      case 9:
        return ~(src ^ dst) & 0xff;
      case 10:
        return ~dst & 0xff;
      case 11:
        return (src | ~dst) & 0xff;
      case 12:
        return ~src & 0xff;
      case 13:
        return (~src | dst) & 0xff;
      case 14:
        return ~(src & dst) & 0xff;
      case 15:
        return 0xff;
      default:
        return src;
    }
  };
  const source = [0xcc, 0x66, 0x33];
  const destination = [0x33, 0x66, 0xaa];
  const expectedTruthTable = Array.from({ length: 16 }, (_, functionId) => [
    rasterByte(functionId, source[0], destination[0]),
    rasterByte(functionId, source[1], destination[1]),
    rasterByte(functionId, source[2], destination[2]),
    functionId === 0 ? 0 : 255,
  ]);
  expect(samples.truthFill).toEqual(expectedTruthTable);
  expect(samples.truthCopy).toEqual(expectedTruthTable);
});

test("browser backend applies GC raster operations to vector drawing", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, drawRectangle?: Function, drawLine?: Function, fillPolygon?: Function, drawText?: Function, setDrawStyle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.drawRectangle ||
      !backend.drawLine ||
      !backend.fillPolygon ||
      !backend.drawText ||
      !backend.setDrawStyle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend vector raster methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x720306;
    /** @param {number} functionId @param {number=} fontPixelSize */
    const setFunction = (functionId, fontPixelSize = 14) =>
      backend.setDrawStyle(
        functionId,
        1,
        0,
        1,
        0,
        0,
        4,
        [],
        0,
        0,
        0,
        0,
        0,
        0xffffff,
        fontPixelSize,
        "sans-serif",
      );

    backend.createWindow(windowId, rootWindow, 0, 0, 36, 20, 24, 0);
    backend.mapWindow(windowId);
    setFunction(3);
    backend.drawRectangle(windowId, 0x0000ff, 0, 0, 13, 20, true);
    backend.drawRectangle(windowId, 0xffffff, 14, 0, 22, 20, true);
    setFunction(10);
    backend.drawLine(windowId, 0x000000, 0, 0, 8, 0);
    setFunction(12);
    backend.drawLine(windowId, 0xff0000, 0, 2, 8, 2);
    setFunction(7);
    backend.fillPolygon(windowId, 0xff0000, [0, 8, 8, 8, 8, 16, 0, 16], 0);
    setFunction(10, 12);
    backend.drawText(windowId, 0x000000, 15, 13, "A");
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing vector raster test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing vector raster test context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    const textData = context.getImageData(14, 2, 18, 14).data;
    let darkTextPixels = 0;
    for (let offset = 0; offset < textData.length; offset += 4) {
      if (
        textData[offset] < 80 &&
        textData[offset + 1] < 80 &&
        textData[offset + 2] < 80 &&
        textData[offset + 3] === 255
      ) {
        darkTextPixels++;
      }
    }
      return {
        invertedLine: pixel(4, 0),
        copyInvertedLine: pixel(4, 2),
        orPolygon: pixel(4, 12),
        darkTextPixels,
      };
  });

  expect(samples.invertedLine).toEqual([255, 255, 0, 255]);
  expect(samples.copyInvertedLine).toEqual([0, 255, 255, 255]);
  expect(samples.orPolygon).toEqual([255, 0, 255, 255]);
  expect(samples.darkTextPixels).toBeGreaterThan(8);
});

test("browser backend applies GC raster operations to putImage", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, drawRectangle?: Function, putImage?: Function, setDrawStyle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.drawRectangle ||
      !backend.putImage ||
      !backend.setDrawStyle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend putImage raster methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x720304;
    const bytes = new Uint8Array([0x00, 0x00, 0xff, 0xff]);
    /** @param {number} functionId */
    const setFunction = (functionId) =>
      backend.setDrawStyle(
        functionId,
        1,
        0,
        1,
        0,
        0,
        4,
        [],
        0,
        0,
        0,
        0,
        0,
        0xffffff,
        14,
        "sans-serif",
      );

    backend.createWindow(windowId, rootWindow, 0, 0, 3, 1, 24, 0);
    backend.mapWindow(windowId);
    backend.drawRectangle(windowId, 0x0000ff, 0, 0, 3, 1, true);
    setFunction(5);
    backend.putImage(windowId, bytes, 0, 0, 0, 0, 1, 1, 4, 32);
    setFunction(6);
    backend.putImage(windowId, bytes, 0, 0, 1, 0, 1, 1, 4, 32);
    setFunction(0);
    backend.putImage(windowId, bytes, 0, 0, 2, 0, 1, 1, 4, 32);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing putImage raster test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing putImage raster test context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      noopTarget: pixel(0, 0),
      xorTarget: pixel(1, 0),
      clearTarget: pixel(2, 0),
    };
  });

  expect(samples.noopTarget).toEqual([0, 0, 255, 255]);
  expect(samples.xorTarget).toEqual([255, 0, 255, 255]);
  expect(samples.clearTarget[3]).toBe(0);
});

test("browser backend applies GC plane masks to rasterized drawing", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, drawRectangle?: Function, setDrawStyle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.drawRectangle ||
      !backend.setDrawStyle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend plane mask methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x72030a;
    /** @param {number} planeMask */
    const setPlaneMask = (planeMask) =>
      backend.setDrawStyle(
        3,
        1,
        0,
        1,
        0,
        0,
        4,
        [],
        0,
        0,
        0,
        0,
        0,
        0xffffff,
        14,
        "sans-serif",
        planeMask,
      );

    backend.createWindow(windowId, rootWindow, 0, 0, 2, 1, 24, 0);
    backend.mapWindow(windowId);
    backend.drawRectangle(windowId, 0x112233, 0, 0, 2, 1, true);
    setPlaneMask(0x00ff00);
    backend.drawRectangle(windowId, 0xaabbcc, 0, 0, 1, 1, true);
    setPlaneMask(0);
    backend.drawRectangle(windowId, 0xffffff, 1, 0, 1, 1, true);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing plane mask test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing plane mask test context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      greenPlaneOnly: pixel(0, 0),
      zeroMask: pixel(1, 0),
    };
  });

  expect(samples.greenPlaneOnly).toEqual([0x11, 0xbb, 0x33, 255]);
  expect(samples.zeroMask).toEqual([0x11, 0x22, 0x33, 255]);
});

test("browser backend clips putImage to drawable bounds", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, drawRectangle?: Function, putImage?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.drawRectangle ||
      !backend.putImage ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend putImage clipping methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x720309;
    const threePixels = new Uint8Array([
      0x00,
      0x00,
      0xff,
      0xff,
      0x00,
      0xff,
      0x00,
      0xff,
      0x00,
      0xff,
      0xff,
      0xff,
    ]);
    const twoPixels = threePixels.slice(0, 8);

    backend.createWindow(windowId, rootWindow, 0, 0, 4, 3, 24, 0);
    backend.mapWindow(windowId);
    backend.drawRectangle(windowId, 0x0000ff, 0, 0, 4, 3, true);
    backend.putImage(windowId, twoPixels, -1, 0, 0, 0, 3, 1, 8, 32);
    backend.putImage(windowId, threePixels, 0, 0, -1, 1, 3, 1, 12, 32);
    backend.putImage(windowId, twoPixels, 0, 0, 0, 2, 3, 1, 8, 32);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing putImage clipping target canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing putImage clipping target context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      sourceLeftOutside: pixel(0, 0),
      sourceLeftRed: pixel(1, 0),
      sourceLeftGreen: pixel(2, 0),
      negativeDestGreen: pixel(0, 1),
      negativeDestYellow: pixel(1, 1),
      rightOverrunRed: pixel(0, 2),
      rightOverrunGreen: pixel(1, 2),
      rightOverrunUntouched: pixel(2, 2),
    };
  });

  expect(samples.sourceLeftOutside).toEqual([0, 0, 255, 255]);
  expect(samples.sourceLeftRed).toEqual([255, 0, 0, 255]);
  expect(samples.sourceLeftGreen).toEqual([0, 255, 0, 255]);
  expect(samples.negativeDestGreen).toEqual([0, 255, 0, 255]);
  expect(samples.negativeDestYellow).toEqual([255, 255, 0, 255]);
  expect(samples.rightOverrunRed).toEqual([255, 0, 0, 255]);
  expect(samples.rightOverrunGreen).toEqual([0, 255, 0, 255]);
  expect(samples.rightOverrunUntouched).toEqual([0, 0, 255, 255]);
});

test("browser backend packs and unpacks 16bpp RGB565 images", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, putImage?: Function, getImage?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.putImage ||
      !backend.getImage ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend RGB565 image methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x720305;
    const bytes = new Uint8Array([
      0x00,
      0xf8,
      0xe0,
      0x07,
      0x1f,
      0x00,
    ]);
    const roundTrip = new Uint8Array(6);

    backend.createWindow(windowId, rootWindow, 0, 0, 3, 1, 24, 0);
    backend.mapWindow(windowId);
    backend.putImage(windowId, bytes, 0, 0, 0, 0, 3, 1, 6, 16);
    backend.getImage(windowId, 0, 0, 3, 1, roundTrip, 6, 16);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing RGB565 test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing RGB565 test context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      red: pixel(0, 0),
      green: pixel(1, 0),
      blue: pixel(2, 0),
      roundTrip: Array.from(roundTrip),
    };
  });

  expect(samples.red).toEqual([255, 0, 0, 255]);
  expect(samples.green).toEqual([0, 255, 0, 255]);
  expect(samples.blue).toEqual([0, 0, 255, 255]);
  expect(samples.roundTrip).toEqual([0x00, 0xf8, 0xe0, 0x07, 0x1f, 0x00]);
});

test("browser backend draws LineDoubleDash gaps with background pixel", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, drawLine?: Function, setDrawStyle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.drawLine ||
      !backend.setDrawStyle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend double dash methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x720401;
    backend.createWindow(windowId, rootWindow, 0, 0, 14, 3, 24, 0);
    backend.mapWindow(windowId);
    backend.setDrawStyle(
      3,
      1,
      2,
      1,
      0,
      0,
      4,
      [4, 4],
      0,
      0,
      0,
      0,
      0,
      0x00ff00,
      14,
      "sans-serif",
    );
    backend.drawLine(windowId, 0xff0000, 0, 1, 12, 1);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing double dash test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing double dash test context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      firstDash: pixel(1, 1),
      firstGap: pixel(5, 1),
      secondDash: pixel(9, 1),
    };
  });

  for (const sample of [samples.firstDash, samples.secondDash]) {
    expect(sample[0]).toBeGreaterThan(180);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeLessThan(80);
    expect(sample[3]).toBe(255);
  }
  expect(samples.firstGap[0]).toBeLessThan(80);
  expect(samples.firstGap[1]).toBeGreaterThan(180);
  expect(samples.firstGap[2]).toBeLessThan(80);
  expect(samples.firstGap[3]).toBe(255);
});

test("browser backend applies multiple clip rectangles to image writes", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, setClipRects?: Function, putImage?: Function, clearWindow?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.setClipRects ||
      !backend.putImage ||
      !backend.clearWindow
    ) {
      throw new Error("missing X11 wasm backend multi-clip methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const width = 80;
    const height = 40;
    const bytes = new Uint8Array(width * height * 4);
    for (let offset = 0; offset < bytes.length; offset += 4) {
      bytes[offset] = 0;
      bytes[offset + 1] = 0;
      bytes[offset + 2] = 255;
      bytes[offset + 3] = 255;
    }

    backend.clearWindow(rootWindow);
    backend.setClipRects(true, [
      { x: 10, y: 10, width: 16, height: 18 },
      { x: 46, y: 10, width: 16, height: 18 },
    ]);
    backend.putImage(rootWindow, bytes, 0, 0, 0, 0, width, height, width * 4, 32);
    backend.setClipRects(false, []);
    backend.flush();

    const canvas = document.querySelector("#x11-wasm-root > canvas");
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing root canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing root canvas context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      first: pixel(12, 12),
      gap: pixel(34, 12),
      second: pixel(50, 12),
      outside: pixel(12, 32),
    };
  });

  for (const sample of [samples.first, samples.second]) {
    expect(sample[0]).toBeGreaterThan(180);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeLessThan(80);
    expect(sample[3]).toBe(255);
  }
  for (const sample of [samples.gap, samples.outside]) {
    expect(sample[0]).toBeLessThan(80);
    expect(sample[3]).toBe(0);
  }
});

test("browser backend clears windows with background pixmap", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createPixmap?: Function, createWindow?: Function, mapWindow?: Function, setWindowStyle?: Function, clearWindow?: Function, drawRectangle?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createPixmap ||
      !backend.createWindow ||
      !backend.mapWindow ||
      !backend.setWindowStyle ||
      !backend.clearWindow ||
      !backend.drawRectangle
    ) {
      throw new Error("missing X11 wasm backend background pixmap methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const pixmapId = 0x720001;
    const windowId = 0x720002;
    backend.createPixmap(pixmapId, rootWindow, 2, 1, 24);
    backend.drawRectangle(pixmapId, 0xff0000, 0, 0, 1, 1, true);
    backend.drawRectangle(pixmapId, 0x0000ff, 1, 0, 1, 1, true);
    backend.createWindow(windowId, rootWindow, 0, 0, 6, 2, 24, 0);
    backend.mapWindow(windowId);
    backend.setWindowStyle(windowId, false, 0, pixmapId, false, 0, 0, false, 0);
    backend.clearWindow(windowId);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing background pixmap test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing background pixmap test context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      first: pixel(0, 0),
      second: pixel(1, 0),
      third: pixel(2, 0),
    };
  });

  expect(samples.first[0]).toBeGreaterThan(180);
  expect(samples.first[1]).toBeLessThan(80);
  expect(samples.first[2]).toBeLessThan(80);
  expect(samples.first[3]).toBe(255);
  expect(samples.second[0]).toBeLessThan(80);
  expect(samples.second[1]).toBeLessThan(80);
  expect(samples.second[2]).toBeGreaterThan(180);
  expect(samples.second[3]).toBe(255);
  expect(samples.third[0]).toBeGreaterThan(180);
  expect(samples.third[1]).toBeLessThan(80);
  expect(samples.third[2]).toBeLessThan(80);
  expect(samples.third[3]).toBe(255);
});

test("browser backend clears a window subarea with background color", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, setWindowStyle?: Function, clearWindowArea?: Function, drawRectangle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.setWindowStyle ||
      !backend.clearWindowArea ||
      !backend.drawRectangle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend clear area methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x720501;
    backend.createWindow(windowId, rootWindow, 0, 0, 5, 3, 24, 0);
    backend.mapWindow(windowId);
    backend.drawRectangle(windowId, 0x0000ff, 0, 0, 5, 3, true);
    backend.setWindowStyle(windowId, true, 0xff0000, 0, false, 0, 0, false, 0);
    backend.clearWindowArea(windowId, 1, 1, 2, 1);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing clear area test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing clear area test context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      untouched: pixel(0, 1),
      cleared: pixel(1, 1),
      clearedRight: pixel(2, 1),
      outside: pixel(3, 1),
    };
  });

  for (const sample of [samples.untouched, samples.outside]) {
    expect(sample[0]).toBeLessThan(80);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeGreaterThan(180);
    expect(sample[3]).toBe(255);
  }
  for (const sample of [samples.cleared, samples.clearedRight]) {
    expect(sample[0]).toBeGreaterThan(180);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeLessThan(80);
    expect(sample[3]).toBe(255);
  }
});

test("browser backend preserves window contents without a background", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, setWindowStyle?: Function, clearWindowArea?: Function, drawRectangle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.setWindowStyle ||
      !backend.clearWindowArea ||
      !backend.drawRectangle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend clear background methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x720601;
    backend.createWindow(windowId, rootWindow, 0, 0, 5, 3, 24, 0);
    backend.mapWindow(windowId);
    backend.drawRectangle(windowId, 0x0000ff, 0, 0, 5, 3, true);
    backend.flush();
    backend.setWindowStyle(windowId, false, 0, 0, false, 0, 0, false, 0);
    backend.clearWindowArea(windowId, 1, 1, 2, 1);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing clear background test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing clear background test context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      cleared: pixel(1, 1),
      clearedRight: pixel(2, 1),
      outside: pixel(3, 1),
    };
  });

  for (const sample of [samples.cleared, samples.clearedRight, samples.outside]) {
    expect(sample[0]).toBeLessThan(80);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeGreaterThan(180);
    expect(sample[3]).toBe(255);
  }
});

test("browser backend preserves window bits according to bit gravity", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const samples = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createWindow?: Function, mapWindow?: Function, drawRectangle?: Function, setWindowAttributes?: Function, configureWindow?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createWindow ||
      !backend.mapWindow ||
      !backend.drawRectangle ||
      !backend.setWindowAttributes ||
      !backend.configureWindow ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend bit gravity methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const windowId = 0x720201;
    const southEastGravity = 9;
    backend.createWindow(windowId, rootWindow, 0, 0, 4, 4, 24, 0);
    backend.mapWindow(windowId);
    backend.drawRectangle(windowId, 0xff0000, 0, 0, 2, 2, true);
    backend.setWindowAttributes(
      windowId,
      0,
      0,
      southEastGravity,
      1,
      0,
      0xffffffff,
      0,
    );
    backend.configureWindow(windowId, 0, 0, 6, 6, 0, 0);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const canvas = resource?.canvas;
    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error("missing bit gravity test canvas");
    }
    const context = canvas.getContext("2d");
    if (!context) {
      throw new Error("missing bit gravity test context");
    }
    const pixel = (x, y) => Array.from(context.getImageData(x, y, 1, 1).data);
    return {
      origin: pixel(0, 0),
      shiftedTopLeft: pixel(2, 2),
      shiftedBottomRight: pixel(3, 3),
      outside: pixel(4, 4),
    };
  });

  expect(samples.origin[3]).toBe(0);
  for (const sample of [samples.shiftedTopLeft, samples.shiftedBottomRight]) {
    expect(sample[0]).toBeGreaterThan(180);
    expect(sample[1]).toBeLessThan(80);
    expect(sample[2]).toBeLessThan(80);
    expect(sample[3]).toBe(255);
  }
  expect(samples.outside[3]).toBe(0);
});

test("browser backend applies border pixmap to DOM style", async ({ page }) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const style = await page.evaluate(async () => {
    await new Promise((resolve) =>
      requestAnimationFrame(() => requestAnimationFrame(resolve)),
    );
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot & { rootWindow?: number, createPixmap?: Function, createWindow?: Function, configureWindow?: Function, mapWindow?: Function, setWindowStyle?: Function, drawRectangle?: Function, flush?: Function } } }} */ (
        window
      ).Module?.x11WasmBackend;
    if (
      !backend?.createPixmap ||
      !backend.createWindow ||
      !backend.configureWindow ||
      !backend.mapWindow ||
      !backend.setWindowStyle ||
      !backend.drawRectangle ||
      !backend.flush
    ) {
      throw new Error("missing X11 wasm backend border pixmap methods");
    }
    const rootWindow = backend.rootWindow || 1;
    const pixmapId = 0x720101;
    const windowId = 0x720102;
    backend.createPixmap(pixmapId, rootWindow, 2, 2, 24);
    backend.drawRectangle(pixmapId, 0xff0000, 0, 0, 1, 2, true);
    backend.drawRectangle(pixmapId, 0x0000ff, 1, 0, 1, 2, true);
    backend.createWindow(windowId, rootWindow, 0, 0, 16, 12, 24, 0);
    backend.configureWindow(windowId, 0, 0, 16, 12, 4, 0);
    backend.mapWindow(windowId);
    backend.setWindowStyle(windowId, false, 0, 0, false, 0, pixmapId, false, 0);
    backend.flush();

    const resource = backend.resources.get(windowId);
    const element = resource?.element;
    if (!(element instanceof HTMLElement)) {
      throw new Error("missing border pixmap test element");
    }
    const initialSource = element.style.borderImageSource;
    backend.drawRectangle(pixmapId, 0x00ff00, 0, 0, 2, 2, true);
    backend.flush();
    const computed = getComputedStyle(element);
    return {
      borderWidth: computed.borderTopWidth,
      initialSource,
      borderImageSource: element.style.borderImageSource,
      borderImageSlice: computed.borderImageSlice,
      borderImageRepeat: computed.borderImageRepeat,
    };
  });

  expect(style.borderWidth).toBe("4px");
  expect(style.initialSource).toContain("data:image/png");
  expect(style.borderImageSource).toContain("data:image/png");
  expect(style.borderImageSource).not.toBe(style.initialSource);
  expect(style.borderImageSlice).toBe("1");
  expect(style.borderImageRepeat).toContain("repeat");
});
