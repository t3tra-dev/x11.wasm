// @ts-check

const { expect: baseExpect, test, waitForX11Runtime } = require("../support/x11");
const expect = /** @type {any} */ (baseExpect);

async function createNativeManagerWindow(page, enabled = true) {
  await waitForX11Runtime(page);
  const id = await page.evaluate((enabled) => {
    const module = /** @type {any} */ (window).Module;
    const backend = module.x11WasmBackend;
    for (const name of ["_X11WasmTestGeometryWindow", "_X11WasmTestGeometryValue",
      "_X11WasmTestGeometryWmState", "_X11WasmManagerSetWindowState",
      "_X11WasmManagerWindowIdentity"]) {
      if (typeof module[name] !== "function") throw new Error(`missing native manager hook ${name}`);
    }
    backend.rootElement.style.width = "640px";
    backend.rootElement.style.height = "480px";
    const id = module._X11WasmTestGeometryWindow();
    module._X11WasmTestGeometryConfigure(id, 15, 70, 100, 240, 160, 0);
    backend.setWindowManagerEnabled(enabled);
    backend.flush();
    return id;
  }, enabled);
  expect(id).toBeGreaterThan(0);
  if (enabled) await expect(frameFor(page, id)).toBeVisible();
  await expect.poll(() => page.evaluate(() => {
    const backend = /** @type {any} */ (window).Module.x11WasmBackend;
    const root = backend.resources.get(backend.rootWindow);
    return [root.width, root.height];
  })).toEqual([640, 480]);
  return id;
}

function frameFor(page, id) {
  return page.locator(`[data-x11-wasm-frame="${id}"]`);
}

function actionFor(page, id, action) {
  return frameFor(page, id).locator(`[data-x11-wasm-decoration="${action}"]`);
}

function taskFor(page, id) {
  return page.locator(`[data-x11-wasm-taskbar] [data-x11-wasm-task="${id}"][data-x11-wasm-decoration="restore"]`);
}

async function stateSnapshot(page, id) {
  return page.evaluate((id) => {
    const module = /** @type {any} */ (window).Module;
    const backend = module.x11WasmBackend;
    backend.flush();
    const resource = backend.resources.get(id);
    const root = backend.rootElement;
    const rootRect = root.getBoundingClientRect();
    const rect = resource.element.getBoundingClientRect();
    return {
      wmState: module._X11WasmTestGeometryWmState(id, 0, 0),
      mapState: module._X11WasmTestGeometryWmState(id, 10, 0),
      flags: [1, 2, 3, 4].map((flag) => module._X11WasmTestWindowHasNetWmState(id, flag)),
      native: [0, 1, 2, 3].map((field) => module._X11WasmTestGeometryValue(id, field)),
      committed: [resource.x, resource.y, resource.width, resource.height],
      css: [rect.left - rootRect.left - root.clientLeft,
        rect.top - rootRect.top - root.clientTop, rect.width, rect.height],
      canvas: [resource.canvas.width, resource.canvas.height],
    };
  }, id);
}

async function expectState(page, id, expected) {
  await expect.poll(() => stateSnapshot(page, id)).toMatchObject(expected);
  const snapshot = await stateSnapshot(page, id);
  expect(snapshot.committed).toEqual(snapshot.native);
  if (snapshot.mapState === 2) {
    expect(snapshot.css).toEqual(snapshot.native);
    expect(snapshot.canvas).toEqual(snapshot.native.slice(2));
  }
  return snapshot;
}

async function setManagerEnabled(page, enabled) {
  await page.evaluate((enabled) => {
    const backend = /** @type {any} */ (window).Module.x11WasmBackend;
    backend.setWindowManagerEnabled(enabled);
    backend.flush();
  }, enabled);
}

test("native WM_STATE and EWMH properties follow maximize minimize and taskbar restore", async ({ page }) => {
  const id = await createNativeManagerWindow(page);
  const normal = await expectState(page, id, { wmState: 1, mapState: 2, flags: [0, 0, 0, 0] });
  await actionFor(page, id, "maximize").click();
  await expectState(page, id, { wmState: 1, mapState: 2, flags: [0, 1, 1, 0], native: [0, 28, 640, 420] });
  await expect(actionFor(page, id, "maximize")).toHaveAttribute("aria-label", "Restore");
  await actionFor(page, id, "minimize").click();
  await expectState(page, id, { wmState: 3, mapState: 0, flags: [0, 1, 1, 0] });
  await expect(frameFor(page, id)).toBeHidden();
  await taskFor(page, id).click();
  await expectState(page, id, { wmState: 1, mapState: 2, flags: [0, 1, 1, 0] });
  await actionFor(page, id, "maximize").click();
  await expectState(page, id, { wmState: 1, mapState: 2, flags: [0, 0, 0, 0], native: normal.native });
});

for (const iconic of [false, true]) {
  test(`native restore geometry survives root resize and toggles while ${iconic ? "iconic" : "zoomed"}`, async ({ page }) => {
    const id = await createNativeManagerWindow(page);
    const normal = (await stateSnapshot(page, id)).native;
    await actionFor(page, id, "maximize").click();
    if (iconic) await actionFor(page, id, "minimize").click();
    await page.evaluate(() => {
      const element = /** @type {HTMLElement} */ (document.querySelector("#x11-wasm-root"));
      element.style.width = "600px";
      element.style.height = "450px";
    });
    await expectState(page, id, iconic
      ? { wmState: 3, mapState: 0, flags: [0, 1, 1, 0] }
      : { wmState: 1, mapState: 2, native: [0, 28, 600, 390] });
    await setManagerEnabled(page, false);
    await expect(frameFor(page, id)).toHaveCount(0);
    await page.evaluate(() => {
      const element = /** @type {HTMLElement} */ (document.querySelector("#x11-wasm-root"));
      element.style.width = "580px";
      element.style.height = "430px";
    });
    await setManagerEnabled(page, true);
    if (iconic) {
      await expectState(page, id, { wmState: 3, mapState: 0, flags: [0, 1, 1, 0] });
      await taskFor(page, id).click();
    }
    await expectState(page, id, { wmState: 1, mapState: 2, native: [0, 28, 580, 370] });
    await actionFor(page, id, "maximize").click();
    await expectState(page, id, { flags: [0, 0, 0, 0], native: normal });
  });
}

test("external native geometry while zoomed replaces the saved normal geometry", async ({ page }) => {
  const id = await createNativeManagerWindow(page);
  await actionFor(page, id, "maximize").click();
  const programmed = [95, 120, 210, 130];
  await page.evaluate(({ id, programmed }) => {
    const module = /** @type {any} */ (window).Module;
    module._X11WasmTestGeometryConfigure(id, 15, ...programmed, 0);
    module.x11WasmBackend.flush();
  }, { id, programmed });
  await expectState(page, id, { flags: [0, 1, 1, 0], native: programmed });
  await actionFor(page, id, "maximize").click();
  await expectState(page, id, { flags: [0, 0, 0, 0], native: programmed });
});

test("programmatic EWMH state roundtrips through UI restore without clearing above", async ({ page }) => {
  const id = await createNativeManagerWindow(page);
  const normal = (await stateSnapshot(page, id)).native;
  await page.evaluate((id) => {
    const module = /** @type {any} */ (window).Module;
    module._X11WasmTestSendNetWmStateMessage(id, 1, 1, 0);
    module._X11WasmTestSendNetWmStateMessage(id, 1, 2, 3);
    module.x11WasmBackend.flush();
  }, id);
  await expectState(page, id, { flags: [1, 1, 1, 0] });
  await expect(actionFor(page, id, "maximize")).toHaveAttribute("aria-label", "Restore");
  await actionFor(page, id, "maximize").click();
  await expectState(page, id, { flags: [1, 0, 0, 0], native: normal });
  await page.evaluate((id) => {
    /** @type {any} */ (window).Module._X11WasmTestGeometryWmState(id, 2, 0);
  }, id);
  await expectState(page, id, { wmState: 0, mapState: 0 });
  await expect(taskFor(page, id)).toHaveCount(0);
  await page.evaluate((id) => {
    /** @type {any} */ (window).Module._X11WasmTestGeometryWmState(id, 3, 0);
  }, id);
  await expectState(page, id, { wmState: 1, mapState: 2, flags: [1, 0, 0, 0] });
  await expect(taskFor(page, id)).toBeVisible();
});

test("native manager state requests preserve unrelated above and fullscreen atoms", async ({ page }) => {
  const id = await createNativeManagerWindow(page, false);
  await page.evaluate((id) => {
    const module = /** @type {any} */ (window).Module;
    module._X11WasmTestSendNetWmStateMessage(id, 1, 1, 4);
  }, id);
  for (const state of [2, 1, 2, 0]) {
    const snapshot = await page.evaluate(({ id, state }) => {
      const module = /** @type {any} */ (window).Module;
      const result = module._X11WasmManagerSetWindowState(id, state, 0);
      return { result, flags: [1, 2, 3, 4].map((flag) =>
        module._X11WasmTestWindowHasNetWmState(id, flag)),
      wmState: module._X11WasmTestGeometryWmState(id, 0, 0) };
    }, { id, state });
    expect(snapshot).toEqual({ result: 1,
      flags: state === 0 ? [1, 0, 0, 1] : [1, 1, 1, 1],
      wmState: state === 1 ? 3 : 1 });
  }
});

for (const [name, grab] of [["pointer", 1], ["keyboard", 2], ["server", 3]]) {
  test(`native ${name} grab rejects WM actions and release restores native and UI actions`, async ({ page }) => {
    await waitForX11Runtime(page);
    const id = await page.evaluate(() => {
      const module = /** @type {any} */ (window).Module;
      const id = module._X11WasmTestManagerWindow(0);
      module._X11WasmTestGeometryConfigure(id, 15, 70, 100, 240, 160, 0);
      module.x11WasmBackend.setWindowManagerEnabled(true);
      module.x11WasmBackend.flush();
      return id;
    });
    const before = await stateSnapshot(page, id);
    const rejected = await page.evaluate(({ id, grab }) => {
      const module = /** @type {any} */ (window).Module;
      module._X11WasmTestManagerOptions(id, 4, grab);
      const results = [module._X11WasmManagerConfigureWindow(id, 31, 70, 200, 100, 15),
        ...[0, 1, 2].map((state) => module._X11WasmManagerSetWindowState(id, state, 0)),
        module._X11WasmManagerActivateWindow(id, 0),
        module._X11WasmManagerCloseWindow(id, 0)];
      module.x11WasmBackend.flush();
      return { results, closed: module._X11WasmTestManagerCloseEvent(id) };
    }, { id, grab });
    expect(rejected).toEqual({ results: [0, 0, 0, 0, 0, 0], closed: 0 });
    expect(await stateSnapshot(page, id)).toEqual(before);
    for (const action of ["minimize", "maximize", "close"]) {
      await expect(actionFor(page, id, action)).toBeDisabled();
    }
    const accepted = await page.evaluate((id) => {
      const module = /** @type {any} */ (window).Module;
      module._X11WasmTestManagerOptions(id, 4, 0);
      const results = [module._X11WasmManagerConfigureWindow(id, 31, 70, 200, 100, 15),
        ...[2, 1, 0].map((state) => module._X11WasmManagerSetWindowState(id, state, 0)),
        module._X11WasmManagerActivateWindow(id, 0),
        module._X11WasmManagerCloseWindow(id, 0)];
      module.x11WasmBackend.flush();
      return { results, closed: module._X11WasmTestManagerCloseEvent(id) };
    }, id);
    expect(accepted).toEqual({ results: [1, 1, 1, 1, 1, 1], closed: 1 });
    const restored = await expectState(page, id, { wmState: 1, mapState: 2,
      flags: [0, 0, 0, 0], native: [31, 70, 200, 100] });
    await actionFor(page, id, "maximize").click();
    await expectState(page, id, { flags: [0, 1, 1, 0] });
    await actionFor(page, id, "minimize").click();
    await expectState(page, id, { wmState: 3, mapState: 0 });
    await taskFor(page, id).click();
    await expectState(page, id, { wmState: 1, mapState: 2, flags: [0, 1, 1, 0] });
    await actionFor(page, id, "maximize").click();
    await expectState(page, id, { flags: [0, 0, 0, 0], native: restored.native });
  });
}
