// @ts-check

const { expect, test, waitForX11Runtime } = require("../support/x11");

async function openManagedWindow(page) {
  await waitForX11Runtime(page);
  const ids = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const b = m.x11WasmBackend;
    b.rootElement.style.width = "640px";
    b.rootElement.style.height = "480px";
    // A bare X11 client has no Tk ConfigureNotify handler to resize children.
    // Exercise retry state on a real single-resource native toplevel instead.
    const frame = m._X11WasmTestGeometryWindow();
    const client = frame;
    m._X11WasmTestGeometryConfigure(frame, 15, 70, 100, 240, 160, 0);
    b.setWindowManagerEnabled(true);
    b.flush();
    const button = document.createElement("button");
    button.id = "fullscreen-geometry-enter";
    button.textContent = "Enter native fullscreen";
    button.onclick = () => {
      m._X11WasmTestSendNetWmStateMessage(client, 1, 4, 0);
      b.flush();
    };
    document.body.prepend(button);
    return { frame, client };
  });
  await expect.poll(() => page.evaluate(() => {
    const b = /** @type {any} */ (window).Module.x11WasmBackend;
    return [b.getRootWidth(), b.getRootHeight()];
  })).toEqual([640, 480]);
  return ids;
}

async function state(page, ids) {
  return page.evaluate((ids) => {
    const m = /** @type {any} */ (window).Module;
    const b = m.x11WasmBackend;
    b.flush();
    const r = b.resources.get(ids.frame);
    const f = b.managerFrames.get(ids.frame);
    const resources = [...new Set([ids.frame, ids.client])].map((id) => {
      const r = b.resources.get(id);
      const rect = r.element.getBoundingClientRect();
      return { native: [0, 1, 2, 3].map((field) => m._X11WasmTestGeometryValue(id, field)),
        committed: [r.x, r.y, r.width, r.height],
        backing: [r.canvas.width, r.canvas.height], domSize: [rect.width, rect.height] };
    });
    return { resources, saved: r.managerFullscreenGeometry || null,
      markers: f ? [f.lastGeometry, f.lastZoomed, f.lastBounds, f.lastFullscreen] : null,
      fullscreen: document.fullscreenElement?.getAttribute("data-x11-wasm-xid") || null,
      root: [b.getRootWidth(), b.getRootHeight()], viewport: [innerWidth, innerHeight],
      refused: /** @type {any} */ (window).__fullscreenConfigureRefusal?.calls || [] };
  }, ids);
}

async function waitForGeometry(page, ids, expected) {
  await expect.poll(async () => (await state(page, ids)).resources[0].native).toEqual(expected);
  const value = await state(page, ids);
  for (const [index, r] of value.resources.entries()) {
    expect(r.native).toEqual(index ? [0, 0, ...expected.slice(2)] : expected);
    expect(r.committed).toEqual(r.native);
    expect(r.backing).toEqual(expected.slice(2));
    expect(r.domSize).toEqual(expected.slice(2));
  }
  return value;
}

async function enter(page) {
  await page.locator("#fullscreen-geometry-enter").click();
  await page.waitForFunction(() => !!document.fullscreenElement);
}

async function refuseConfigure(page, frame) {
  await page.evaluate((frame) => {
    const target = /** @type {any} */ (window);
    const m = target.Module;
    const original = m._X11WasmConfigureWindow;
    const refusal = { original, calls: /** @type {number[][]} */ ([]), released: false };
    target.__fullscreenConfigureRefusal = refusal;
    m._X11WasmConfigureWindow = (...args) => {
      if (args[0] === frame && !refusal.released) {
        refusal.calls.push(args);
        return 0;
      }
      return original(...args);
    };
  }, frame);
}

async function releaseConfigure(page) {
  await page.evaluate(() => {
    const target = /** @type {any} */ (window);
    const refusal = target.__fullscreenConfigureRefusal;
    if (refusal) {
      refusal.released = true;
      target.Module._X11WasmConfigureWindow = refusal.original;
    }
    target.Module.x11WasmBackend.syncWindowManager();
    target.Module.x11WasmBackend.flush();
  });
}

for (const phase of ["enter", "resize", "exit"]) {
  test(`managed native fullscreen retries a refused ${phase} configure without losing restore state`, async ({ page }, testInfo) => {
    const ids = await openManagedWindow(page);
    const normal = await state(page, ids);
    const observations = { normal };
    try {
      if (phase !== "enter") {
        await enter(page);
        const viewport = await page.evaluate(() => [innerWidth, innerHeight]);
        await waitForGeometry(page, ids, [0, 0, ...viewport]);
      }
      const before = await state(page, ids);
      await refuseConfigure(page, ids.frame);
      if (phase === "enter") await enter(page);
      if (phase === "resize") await page.setViewportSize({ width: 1100, height: 760 });
      if (phase === "exit") await page.evaluate(() => document.exitFullscreen());
      await page.waitForFunction(() =>
        /** @type {any} */ (window).__fullscreenConfigureRefusal.calls.length > 0);
      const refused = await state(page, ids);
      Object.assign(observations, { before, refused });
      expect(refused.resources[0].native).toEqual(before.resources[0].native);
      expect(refused.resources[0].backing).toEqual(before.resources[0].backing);
      expect(refused.markers).toEqual(before.markers);
      expect(refused.saved).toMatchObject({ x: 70, y: 100, width: 240, height: 160 });
      await releaseConfigure(page);
      const expected = phase === "exit" ? normal.resources[0].native
        : [0, 0, ...await page.evaluate(() => [innerWidth, innerHeight])];
      const recovered = await waitForGeometry(page, ids, expected);
      Object.assign(observations, { recovered });
      if (phase !== "exit") await page.evaluate(() => document.exitFullscreen());
      const restored = await waitForGeometry(page, ids, normal.resources[0].native);
      expect(restored.saved).toBe(null);
      Object.assign(observations, { restored });
    } finally {
      await releaseConfigure(page);
      if (await page.evaluate(() => !!document.fullscreenElement)) {
        await page.evaluate(() => document.exitFullscreen());
      }
      await testInfo.attach("configure-retry-observations", {
        body: JSON.stringify(observations, null, 2), contentType: "application/json",
      });
    }
  });
}

test("managed fullscreen restores original geometry after disabling the WM", async ({ page }) => {
  const ids = await openManagedWindow(page);
  const normal = await state(page, ids);
  try {
    await enter(page);
    const viewport = await page.evaluate(() => [innerWidth, innerHeight]);
    await waitForGeometry(page, ids, [0, 0, ...viewport]);
    await page.evaluate(() => {
      const b = /** @type {any} */ (window).Module.x11WasmBackend;
      b.setWindowManagerEnabled(false);
      b.flush();
    });
    // Removing real decorations can end native fullscreen by reparenting its
    // element. Either exit route must restore the original native geometry.
    if (await page.evaluate(() => !!document.fullscreenElement)) {
      await page.evaluate(() => document.exitFullscreen());
    }
    const restored = await waitForGeometry(page, ids, normal.resources[0].native);
    expect(restored.saved).toBe(null);
    await page.evaluate(() => {
      const b = /** @type {any} */ (window).Module.x11WasmBackend;
      b.setWindowManagerEnabled(true);
      b.flush();
    });
    await waitForGeometry(page, ids, normal.resources[0].native);
  } finally {
    if (await page.evaluate(() => !!document.fullscreenElement)) {
      await page.evaluate(() => document.exitFullscreen());
    }
  }
});
