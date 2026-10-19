// @ts-check

const { test, expect, X11_WASM_DOM_SOURCES, waitForX11Runtime, createMappedWindow } = require("../support/x11");
const { dispatchComposition, screenshotDifference } = require("../support/preedit");

async function createBackend(page, managed = false, scale = 1) {
  await page.setContent(`<style>body{margin:0;background:white}</style><div id="preedit-root" style="position:relative;margin:30px;width:400px;height:260px;transform:scale(${scale});transform-origin:top left"></div>`);
  for (const source of X11_WASM_DOM_SOURCES) await page.addScriptTag({ path: source });
  await page.evaluate((managed) => {
    const host = /** @type {any} */ (window);
    const backend = host.X11WasmDOM.createBackend({
      root: "#preedit-root", windowManager: managed,
      module: { _X11WasmManagerWindowIdentity: (id, field) => id === 2 ? [2, 3, 3][field] : 0 },
    });
    host.preeditBackend = backend;
    host.preeditEvents = [];
    backend.init(400, 260, 1);
    backend.createWindow(2, 1, 30, 50, 180, 100, 0, 24, 1);
    backend.createWindow(3, 2, 0, 0, 180, 100, 0, 24, 1);
    backend.createWindow(4, 1, 250, 50, 100, 100, 0, 24, 1);
    for (const id of [2, 3, 4]) backend.mapWindow(id);
    backend.flush();
    backend.queueCompositionEvent = (phase, text, target) => host.preeditEvents.push([phase, text, target]);
    backend.queueTextEvent = (type, text, state, time, target) => host.preeditEvents.push([type, text, target]);
    backend.focusInput(true, 3);
    backend.setInputContext(true, 3, true, 60, 90);
  }, managed);
}

test("preedit is painted at the IC spot without intercepting input", async ({ page }) => {
  await createBackend(page);
  const preedit = page.locator(".x11-wasm-preedit");
  const before = await /** @type {any} */ (page).screenshot();
  await dispatchComposition(page, "compositionstart", "変換中");
  await expect(preedit).toBeVisible();
  await expect(preedit).toHaveText("変換中");
  const after = await /** @type {any} */ (page).screenshot();
  expect((await screenshotDifference(page, before, after)).count).toBeGreaterThan(20);
  const state = await preedit.evaluate((element) => {
    const backend = /** @type {any} */ (window).preeditBackend;
    const rect = element.getBoundingClientRect();
    const client = backend.resources.get(3).element.getBoundingClientRect();
    return { x: rect.left, top: rect.top, bottom: rect.bottom,
      spot: { x: client.left + 60 - 30, y: client.top + 90 - 50 },
      fontSize: parseFloat(getComputedStyle(element).fontSize),
      pointer: getComputedStyle(element).pointerEvents,
      focused: document.activeElement === backend.inputElement,
      hit: element.contains(document.elementFromPoint(rect.left + 1, rect.top + 1)),
      input: [backend.inputElement.offsetWidth, backend.inputElement.offsetHeight] };
  });
  expect(state.x).toBeCloseTo(state.spot.x, 0);
  expect(state.top).toBeLessThan(state.spot.y);
  expect(state.bottom).toBeGreaterThanOrEqual(state.spot.y);
  expect(state.bottom - state.spot.y).toBeLessThan(state.fontSize / 2);
  expect(state.pointer).toBe("none");
  expect(state.focused).toBe(true);
  expect(state.hit).toBe(false);
  expect(state.input).toEqual([1, 1]);
});

test("preedit hides on an IC switch without retargeting the composition commit", async ({ page }) => {
  await createBackend(page);
  const preedit = page.locator(".x11-wasm-preedit");
  await dispatchComposition(page, "compositionstart", "元");
  await expect(preedit).toBeVisible();
  await page.evaluate(() => {
    const backend = /** @type {any} */ (window).preeditBackend;
    backend.focusInput(true, 4);
    backend.setInputContext(true, 4, true, 280, 130);
  });
  await expect(preedit).toBeHidden();
  await dispatchComposition(page, "compositionupdate", "元の対象");
  await expect(preedit).toBeHidden();
  await dispatchComposition(page, "compositionend", "確定");
  await expect(preedit).toBeHidden();
  expect(await page.evaluate(() => /** @type {any} */ (window).preeditEvents)).toEqual([
    [1, "元", 3], [2, "元の対象", 3], [3, "確定", 3], [2, "確", 3], [2, "定", 3],
  ]);
});

for (const action of ["cancel", "unmap", "destroy"]) {
  test(`preedit disappears after ${action} and ignores delayed composition`, async ({ page }) => {
    await createBackend(page);
    const preedit = page.locator(".x11-wasm-preedit");
    await dispatchComposition(page, "compositionstart", "未確定");
    await expect(preedit).toBeVisible();
    await page.evaluate((action) => {
      const backend = /** @type {any} */ (window).preeditBackend;
      if (action === "cancel") backend.cancelComposition(3);
      else if (action === "unmap") backend.unmapWindow(2);
      else backend.destroyResource(2);
    }, action);
    await expect(preedit).toBeHidden();
    await dispatchComposition(page, "compositionupdate", "遅延");
    await dispatchComposition(page, "compositionend", "遅延");
    await expect(preedit).toBeHidden();
    expect(await page.evaluate(() => /** @type {any} */ (window).preeditEvents)).toEqual([[1, "未確定", 3]]);
  });
}

test("preedit is hidden while the browser window is blurred", async ({ page }) => {
  await createBackend(page);
  const preedit = page.locator(".x11-wasm-preedit");
  await dispatchComposition(page, "compositionstart", "未確定");
  await expect(preedit).toBeVisible();
  await page.evaluate(() => window.dispatchEvent(new Event("blur")));
  await expect(preedit).toBeHidden();
});

for (const [phase, name] of [[3, "utf8"], [4, "mb"], [5, "wc"], [6, "inactive same-window"]]) {
  test(`native ${name} IC reset ${phase === 6 ? "preserves" : "clears"} preedit`, async ({ page }) => {
    await waitForX11Runtime(page);
    const windowId = await createMappedWindow(page);
    const callHook = (phase) => page.evaluate(({ windowId, phase }) => {
      const module = /** @type {any} */ (window).Module;
      if (typeof module._X11WasmTestXICSpotLocationSync !== "function") {
        throw new Error("missing native IC reset test hook");
      }
      return module._X11WasmTestXICSpotLocationSync(windowId, phase);
    }, { windowId, phase });
    try {
      expect(await callHook(1)).toBe(1);
      await dispatchComposition(page, "compositionstart", "変換");
      const preedit = page.locator(".x11-wasm-preedit");
      await expect(preedit).toBeVisible();
      expect(await callHook(phase)).toBe(1);
      if (phase === 6) {
        await expect(preedit).toBeVisible();
        await expect(preedit).toHaveText("変換");
      } else {
        await expect(preedit).toBeHidden();
        expect(await page.evaluate(() => /** @type {any} */ (window).Module.x11WasmBackend.composing)).toBe(false);
      }
      await dispatchComposition(page, "compositionupdate", "続き");
      if (phase === 6) await expect(preedit).toHaveText("続き");
      else await expect(preedit).toBeHidden();
      await dispatchComposition(page, "compositionend", "続き");
      await expect(preedit).toBeHidden();
    } finally {
      expect(await callHook(0)).toBe(1);
    }
  });
}

for (const scale of [1, 1.25]) {
  test(`managed preedit respects the client spot and clipping at CSS scale ${scale}`, async ({ page }) => {
    await createBackend(page, true, scale);
    await expect(page.locator(".x11-wasm-frame")).toHaveCount(1);
    const before = await /** @type {any} */ (page).screenshot();
    await page.evaluate(() => {
      const backend = /** @type {any} */ (window).preeditBackend;
      backend.setInputContext(true, 3, true, 195, 90);
    });
    await dispatchComposition(page, "compositionstart", "長い変換候補はクライアント領域に収まる");
    const preedit = page.locator(".x11-wasm-preedit");
    await expect(preedit).toBeVisible();
    const rects = await preedit.evaluate((element, scale) => {
      const backend = /** @type {any} */ (window).preeditBackend;
      const text = element.getBoundingClientRect();
      const client = backend.resources.get(3).element.getBoundingClientRect();
      return { x: text.left, top: text.top, bottom: text.bottom,
        spot: { x: client.left + (195 - 30) * scale, y: client.top + (90 - 50) * scale },
        fontSize: parseFloat(getComputedStyle(element).fontSize) * scale,
        client: { left: client.left, top: client.top, right: client.right, bottom: client.bottom } };
    }, scale);
    expect(rects.x).toBeCloseTo(rects.spot.x, 0);
    expect(rects.top).toBeLessThan(rects.spot.y);
    expect(rects.bottom).toBeGreaterThanOrEqual(rects.spot.y);
    expect(rects.bottom - rects.spot.y).toBeLessThan(rects.fontSize / 2);
    const after = await /** @type {any} */ (page).screenshot();
    const changed = await screenshotDifference(page, before, after);
    expect(changed.count).toBeGreaterThan(10);
    expect(changed.left).toBeGreaterThanOrEqual(Math.floor(rects.client.left));
    expect(changed.top).toBeGreaterThanOrEqual(Math.floor(rects.client.top));
    expect(changed.right).toBeLessThan(Math.ceil(rects.client.right));
    expect(changed.bottom).toBeLessThan(Math.ceil(rects.client.bottom));
  });
}
