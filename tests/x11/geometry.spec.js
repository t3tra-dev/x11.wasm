// @ts-check

const { expect, test, waitForX11Runtime } = require("../support/x11");

/** @param {import('@playwright/test').Page} page */
async function createGeometryWindow(page) {
  await waitForX11Runtime(page);
  return page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    const id = module._X11WasmTestGeometryWindow();
    module.x11WasmBackend.flush();
    return id;
  });
}

/**
 * @param {import('@playwright/test').Page} page
 * @param {number} id
 */
async function geometrySnapshot(page, id) {
  return page.evaluate((id) => {
    const module = /** @type {any} */ (window).Module;
    const backend = module.x11WasmBackend;
    backend.flush();
    const resource = backend.resources.get(id);
    const element = resource.element;
    const rect = element.getBoundingClientRect();
    const parentRect = element.parentElement.getBoundingClientRect();
    return {
      native: [0, 1, 2, 3].map((field) =>
        module._X11WasmTestGeometryValue(id, field)),
      pending: [resource.pendingX, resource.pendingY,
        resource.pendingWidth, resource.pendingHeight],
      committed: [resource.x, resource.y, resource.width, resource.height],
      css: [parseFloat(element.style.left), parseFloat(element.style.top),
        rect.width, rect.height],
      canvas: [resource.canvas.width, resource.canvas.height],
      hit: module._X11WasmTestGeometryHit(
        Math.round(rect.right - parentRect.left - element.parentElement.clientLeft - 1),
        Math.round(rect.bottom - parentRect.top - element.parentElement.clientTop - 1)),
    };
  }, id);
}

for (const mode of [1, 2, 3]) {
  test(`native configure remains authoritative with size hints ${mode}`, async ({ page }) => {
    const id = await createGeometryWindow(page);
    await page.evaluate(({ id, mode }) => {
      const module = /** @type {any} */ (window).Module;
      module._X11WasmTestGeometryHints(id, mode);
      module._X11WasmTestGeometryConfigure(id, 15, 31, 37,
        mode === 2 ? 120 : 137, mode === 2 ? 300 : 88, 0);
    }, { id, mode });
    const snapshot = await geometrySnapshot(page, id);
    expect(snapshot.native).toEqual([31, 37,
      mode === 2 ? 120 : 137, mode === 2 ? 300 : 88]);
    expect(snapshot.pending).toEqual(snapshot.native);
    expect(snapshot.committed).toEqual(snapshot.native);
    expect(snapshot.css).toEqual(snapshot.native);
    expect(snapshot.canvas).toEqual(snapshot.native.slice(2));
    expect(snapshot.hit).toBe(id);
  });
}

test("browser configure validates masks and preserves omitted axes", async ({ page }) => {
  const id = await createGeometryWindow(page);
  const results = await page.evaluate((id) => {
    const module = /** @type {any} */ (window).Module;
    return [
      module._X11WasmConfigureWindow(0, 0, 0, 100, 100, 15),
      module._X11WasmConfigureWindow(id, 0, 0, 0, 100, 4),
      module._X11WasmConfigureWindow(id, 0, 0, 100, 100, 64),
      module._X11WasmConfigureWindow(id, 0, 41, 0, 0, 2),
    ];
  }, id);
  expect(results).toEqual([0, 0, 0, 1]);
  const snapshot = await geometrySnapshot(page, id);
  expect(snapshot.native).toEqual([19, 41, 137, 88]);
  expect(snapshot.css).toEqual(snapshot.native);
});

test("zoom tracks root resize through native configure", async ({ page }) => {
  const id = await createGeometryWindow(page);
  await page.evaluate((id) => {
    const module = /** @type {any} */ (window).Module;
    module._X11WasmTestSendNetWmStateMessage(id, 1, 2, 3);
    module.x11WasmBackend.rootElement.style.width = "600px";
  }, id);
  await page.waitForFunction((id) =>
    /** @type {any} */ (window).Module._X11WasmTestGeometryValue(id, 2) === 600, id);
  const snapshot = await geometrySnapshot(page, id);
  expect(snapshot.css).toEqual(snapshot.native);
  expect(snapshot.canvas).toEqual(snapshot.native.slice(2));
});

test("changing normal hints alone does not resize a native window", async ({ page }) => {
  const id = await createGeometryWindow(page);
  await page.evaluate((id) => {
    /** @type {any} */ (window).Module._X11WasmTestGeometryHints(id, 3);
  }, id);
  const snapshot = await geometrySnapshot(page, id);
  expect(snapshot.native).toEqual([19, 23, 137, 88]);
  expect(snapshot.css).toEqual(snapshot.native);
  expect(snapshot.canvas).toEqual(snapshot.native.slice(2));
});

for (const state of ["zoomed", "fullscreen"]) {
  test(`${state} display geometry agrees with native state and hit testing`, async ({ page }) => {
    const id = await createGeometryWindow(page);
    await page.evaluate(({ id, state }) => {
      const module = /** @type {any} */ (window).Module;
      module._X11WasmTestSendNetWmStateMessage(id, 1,
        state === "fullscreen" ? 4 : 2,
        state === "fullscreen" ? 0 : 3);
    }, { id, state });
    // The browser applies its fullscreen stylesheet before fullscreenchange.
    await expect.poll(async () => {
      const snapshot = await geometrySnapshot(page, id);
      return snapshot.native.every((value, index) => value === snapshot.css[index]);
    }).toBe(true);
    const snapshot = await geometrySnapshot(page, id);
    expect(snapshot.native).toEqual(snapshot.css);
    expect(snapshot.pending).toEqual(snapshot.native);
    expect(snapshot.canvas).toEqual(snapshot.native.slice(2));
    expect(snapshot.hit).toBe(id);
    await page.evaluate(({ id, state }) => {
      /** @type {any} */ (window).Module._X11WasmTestSendNetWmStateMessage(
        id, 0, state === "fullscreen" ? 4 : 2, state === "fullscreen" ? 0 : 3);
    }, { id, state });
    const restored = await geometrySnapshot(page, id);
    expect(restored.native).toEqual([19, 23, 137, 88]);
    expect(restored.pending).toEqual(restored.native);
    expect(restored.css).toEqual(restored.native);
    expect(restored.canvas).toEqual(restored.native.slice(2));
  });
}
