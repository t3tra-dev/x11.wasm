// @ts-check

const { expect, fs, test, X11_WASM_DOM_SOURCES } = require("../support/x11");

async function createManagerContract(page) {
  await page.setContent('<div id="x11-wasm-root" style="width:640px;height:480px"></div>');
  for (const path of X11_WASM_DOM_SOURCES) {
    await page.addScriptTag({ content: fs.readFileSync(path, "utf8") });
  }
  await page.evaluate(() => {
    const state = /** @type {any} */ (window);
    const calls = [];
    const module = {
      _X11WasmManagerWindowIdentity: (id) => id >= 2 && id <= 20 ? id : 0,
      _X11WasmManagerConfigureWindow: (...args) => { calls.push(args); return 1; },
    };
    const backend = state.X11WasmDOM.createBackend({ windowManager: true, module });
    backend.init(640, 480, 1);
    state.managerContract = { backend, module, calls };
  });
}

test("enabling decorations preserves exact program geometry including the root edge", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend, calls } = /** @type {any} */ (window).managerContract;
    backend.createWindow(2, 1, 12, 0, 180, 90, 24, 0);
    backend.mapWindow(2);
    backend.flush();
    backend.setWindowManagerEnabled(false);
    backend.setWindowManagerEnabled(true);
    const resource = backend.resources.get(2);
    const rect = resource.element.getBoundingClientRect();
    const root = backend.rootElement.getBoundingClientRect();
    return { calls, native: [resource.x, resource.y],
      client: [rect.left - root.left, rect.top - root.top] };
  });
  expect(result).toEqual({ calls: [], native: [12, 0], client: [12, 0] });
});

test("native identity and window types determine eligibility instead of class names", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend } = /** @type {any} */ (window).managerContract;
    const types = ["", "NORMAL", "DIALOG", "UTILITY", "MENU", "POPUP_MENU", "TOOLTIP", "DND"];
    types.forEach((type, index) => {
      const id = index + 2;
      backend.createWindow(id, 1, 30, 70, 100, 90, 24, 0);
      backend.setWindowClassHint(id, "widget", "Widget");
      if (type) backend.setWindowTypes(id, `_NET_WM_WINDOW_TYPE_${type}`);
      backend.mapWindow(id);
    });
    backend.flush();
    return Array.from(backend.managerFrames.keys());
  });
  expect(result).toEqual([2, 3, 4, 5]);
});

test("the first recognized EWMH type wins before fallback NORMAL", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend } = /** @type {any} */ (window).managerContract;
    const types = ["POPUP_MENU NORMAL", "NORMAL POPUP_MENU", "CUSTOM NORMAL",
      "CUSTOM TOOLTIP NORMAL", "CUSTOM", ""];
    types.forEach((list, index) => {
      const id = index + 2;
      backend.createWindow(id, 1, 30, 70, 100, 90, 24, 0);
      backend.setWindowTypes(id, list.split(" ").filter(Boolean)
        .map((type) => `_NET_WM_WINDOW_TYPE_${type}`).join(" "));
      backend.mapWindow(id);
    });
    backend.flush();
    return Array.from(backend.managerFrames.keys());
  });
  expect(result).toEqual([3, 4, 6, 7]);
});

test("titlebar uses bundled icons and keeps long titles inside narrow frames", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend } = /** @type {any} */ (window).managerContract;
    backend.createWindow(2, 1, 30, 70, 200, 90, 24, 0);
    backend.setWindowTitle(2, "A very long title ".repeat(40));
    backend.setWindowProtocols(2, "WM_DELETE_WINDOW");
    backend.resources.get(2).iconPhotoHref = "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7";
    backend.mapWindow(2);
    backend.flush();
    const frame = backend.managerFrames.get(2);
    const hasIcon = !frame.icon.hidden && frame.icon.getAttribute("src").startsWith("data:");
    const samples = [200, 100, 30, 12].map((width) => {
      backend.configureWindow(2, 30, 70, width, 90, 0, 4);
      backend.flush();
      const bar = frame.titlebar.getBoundingClientRect();
      const button = frame.close.getBoundingClientRect();
      const title = frame.title.getBoundingClientRect();
      return button.left >= bar.left && button.right <= bar.right + 0.1 &&
        title.right <= button.left + 0.1;
    });
    return { hasIcon, samples, paths: frame.close.querySelectorAll("svg path").length,
      label: frame.close.getAttribute("aria-label") };
  });
  expect(result).toEqual({ hasIcon: true, samples: [true, true, true, true], paths: 2, label: "Close" });
});

test("all eight resize edges preserve their opposite client anchor", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend, calls } = /** @type {any} */ (window).managerContract;
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.mapWindow(2);
    backend.flush();
    const edges = ["n", "ne", "e", "se", "s", "sw", "w", "nw"];
    for (const edge of edges) {
      const handle = backend.managerFrames.get(2).element.querySelector(`[data-x11-wasm-decoration="resize-${edge}"]`);
      if (!handle) return { missing: edge };
      handle.dispatchEvent(new PointerEvent("pointerdown", { bubbles: true, pointerId: 9,
        button: 0, isPrimary: true, clientX: 100, clientY: 100 }));
      handle.dispatchEvent(new PointerEvent("pointermove", { bubbles: true, pointerId: 9,
        buttons: 1, clientX: 120, clientY: 110 }));
      handle.dispatchEvent(new PointerEvent("pointerup", { bubbles: true, pointerId: 9 }));
    }
    return calls;
  });
  expect(result).toEqual([
    [2, 40, 70, 200, 110, 10], [2, 40, 70, 220, 110, 14],
    [2, 40, 60, 220, 120, 4], [2, 40, 60, 220, 130, 12],
    [2, 40, 60, 200, 130, 8], [2, 60, 60, 180, 130, 13],
    [2, 60, 60, 180, 120, 5], [2, 60, 70, 180, 110, 15],
  ]);
});

test("resize uses metadata min max base increments and fixed axes", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend, module, calls } = /** @type {any} */ (window).managerContract;
    module._X11WasmManagerWindowIdentity = (id, field) => id === 2 || id === 3 ? (field ? 3 : 2) : 0;
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.createWindow(3, 2, 0, 0, 200, 120, 24, 0);
    const metadata = backend.resources.get(3);
    metadata.sizeHints = { flags: 16 | 32 | 64 | 256, minWidth: 100, minHeight: 120,
      maxWidth: 240, maxHeight: 120, baseWidth: 0, baseHeight: 0, widthInc: 20, heightInc: 1 };
    backend.mapWindow(2);
    backend.mapWindow(3);
    backend.flush();
    const frame = backend.managerFrames.get(2);
    const west = frame.element.querySelector('[data-x11-wasm-decoration="resize-w"]');
    west.dispatchEvent(new PointerEvent("pointerdown", { bubbles: true, pointerId: 9,
      button: 0, isPrimary: true, clientX: 100, clientY: 100 }));
    west.dispatchEvent(new PointerEvent("pointermove", { bubbles: true, pointerId: 9,
      buttons: 1, clientX: 137, clientY: 150 }));
    west.dispatchEvent(new PointerEvent("pointerup", { bubbles: true, pointerId: 9 }));
    return { calls, hidden: frame.element.querySelector('[data-x11-wasm-decoration="resize-n"]').hidden,
      native: [backend.resources.get(2).x, backend.resources.get(2).width] };
  });
  expect(result).toEqual({ calls: [[2, 80, 60, 160, 120, 5]], hidden: true, native: [40, 200] });
});

test("move coordinates follow root CSS scale and page scroll", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend, calls } = /** @type {any} */ (window).managerContract;
    document.body.style.height = "2000px";
    backend.rootElement.style.transformOrigin = "top left";
    backend.rootElement.style.transform = "scale(0.75, 0.5)";
    backend.rootElement.style.marginTop = "600px";
    window.scrollTo(0, 400);
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.mapWindow(2);
    backend.flush();
    const title = backend.managerFrames.get(2).titlebar;
    const rect = title.getBoundingClientRect();
    title.dispatchEvent(new PointerEvent("pointerdown", { bubbles: true, pointerId: 9,
      button: 0, isPrimary: true, clientX: rect.left + 10, clientY: rect.top + 10 }));
    title.dispatchEvent(new PointerEvent("pointermove", { bubbles: true, pointerId: 9,
      buttons: 1, clientX: rect.left + 40, clientY: rect.top + 30 }));
    title.dispatchEvent(new PointerEvent("pointerup", { bubbles: true, pointerId: 9 }));
    return calls;
  });
  expect(result).toEqual([[2, 80, 100, 200, 120, 3]]);
});

test("client activation requests native focus and stacking without optimistic changes", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend, module, calls } = /** @type {any} */ (window).managerContract;
    module._X11WasmManagerActivateWindow = (id, time) => { calls.push([id, time]); return 1; };
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.createWindow(30, 2, 0, 0, 40, 20, 24, 0);
    backend.mapWindow(2);
    backend.flush();
    let delivered = 0;
    backend.rootElement.addEventListener("pointerdown", () => delivered++);
    const client = backend.resources.get(30).element;
    const event = new PointerEvent("pointerdown", { bubbles: true, pointerId: 9, button: 0, isPrimary: true });
    client.dispatchEvent(event);
    backend.pointerGrabWindow = 2;
    client.dispatchEvent(event);
    return { calls: calls.map((call) => call[0]), delivered, focus: backend.focusedWindow,
      active: backend.managerFrames.get(2).element.getAttribute("data-x11-wasm-active") };
  });
  expect(result).toEqual({ calls: [2], delivered: 2, focus: 0, active: "0" });
});

test("state buttons never fabricate native state and keep maximized restore geometry", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend, module, calls } = /** @type {any} */ (window).managerContract;
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.mapWindow(2);
    backend.flush();
    const frame = backend.managerFrames.get(2);
    const minimize = frame.element.querySelector('[data-x11-wasm-decoration="minimize"]');
    const maximize = frame.element.querySelector('[data-x11-wasm-decoration="maximize"]');
    if (!minimize || !maximize) return { missing: "state buttons" };
    module._X11WasmManagerSetWindowState = (id, state) => { calls.push(["state", id, state]); return 0; };
    backend.syncWindowManager();
    minimize.click();
    const refused = !backend.resources.get(2).zoomed && backend.resources.get(2).mapped;
    module._X11WasmManagerConfigureWindow = (id, x, y, w, h, mask) => {
      calls.push(["geometry", id, x, y, w, h, mask]);
      backend.configureWindow(id, x, y, w, h, 0, mask);
      backend.flush();
      return 1;
    };
    module._X11WasmManagerSetWindowState = (id, state) => {
      calls.push(["state", id, state]);
      const resource = backend.resources.get(id);
      if (state !== 1) resource.zoomed = state === 2;
      resource.wmState = state === 1 ? "iconic" : "normal";
      resource.mapped = state !== 1;
      backend.syncWindowManager();
      return 1;
    };
    maximize.click();
    const max = backend.resources.get(2);
    const maximized = [max.x, max.y, max.width, max.height];
    minimize.click();
    const task = document.querySelector('[data-x11-wasm-task="2"]');
    if (!task) throw new Error("missing taskbar restore button");
    task.dispatchEvent(new MouseEvent("click", { bubbles: true }));
    const restoredZoomed = max.zoomed && max.mapped;
    maximize.click();
    return { refused, maximized, restoredZoomed,
      geometry: [max.x, max.y, max.width, max.height],
      states: calls.filter((call) => call[0] === "state").map((call) => call[2]) };
  });
  expect(result).toEqual({ refused: true, maximized: [0, 28, 640, 420], restoredZoomed: true,
    geometry: [40, 60, 200, 120], states: [1, 2, 1, 2, 0] });
});

test("taskbar explicitly recovers y zero negative and position-hinted windows after root shrink", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend, module, calls } = /** @type {any} */ (window).managerContract;
    module._X11WasmManagerConfigureWindow = (id, x, y, w, h, mask) => {
      calls.push([id, x, y]); backend.configureWindow(id, x, y, w, h, 0, mask); backend.flush(); return 1;
    };
    module._X11WasmManagerActivateWindow = () => 1;
    const before = [];
    for (const [index, y] of [0, -80, 380, 0].entries()) {
      const id = index + 2;
      backend.createWindow(id, 1, 440, y, 200, 120, 24, 0);
      if (index) backend.resources.get(id).sizeHints = { flags: index === 1 ? 1 : 4, x: 440, y };
      backend.mapWindow(id);
      backend.flush();
      before.push([backend.resources.get(id).x, backend.resources.get(id).y]);
    }
    backend.setWindowManagerEnabled(false);
    backend.setWindowManagerEnabled(true);
    const implicit = calls.length;
    backend.rootElement.style.width = "320px";
    backend.rootElement.style.height = "240px";
    backend.syncRootElementSize(false);
    for (const id of [2, 3, 4, 5]) backend.managerTasks.get(id).click();
    const after = [2, 3, 4, 5].map((id) => {
      const frame = backend.managerFrames.get(id);
      const rect = frame.titlebar.getBoundingClientRect();
      const root = backend.rootElement.getBoundingClientRect();
      return rect.left >= root.left && rect.right <= root.right && rect.top >= root.top && rect.bottom <= root.bottom - 32;
    });
    return { before, implicit, after, calls };
  });
  expect(result).toEqual({ before: [[440, 0], [440, -80], [440, 380], [440, 0]], implicit: 0,
    after: [true, true, true, true], calls: [[2, 120, 28], [3, 120, 28], [4, 120, 208], [5, 120, 28]] });
});

test("state refusal rolls geometry back and disable removes taskbar ownership and gestures", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend, module, calls } = /** @type {any} */ (window).managerContract;
    module._X11WasmManagerConfigureWindow = (id, x, y, w, h, mask) => {
      calls.push([x, y, w, h]); backend.configureWindow(id, x, y, w, h, 0, mask); backend.flush(); return 1;
    };
    module._X11WasmManagerSetWindowState = () => 0;
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.mapWindow(2); backend.flush();
    backend.managerFrames.get(2).maximize.click();
    const resource = backend.resources.get(2);
    const geometry = [resource.x, resource.y, resource.width, resource.height];
    backend.setWindowManagerEnabled(false);
    return { calls, geometry, owned: resource.managerGeometryOwned,
      taskbar: backend.managerTaskbar, tasks: backend.managerTasks.size, gesture: backend.managerGesture };
  });
  expect(result).toEqual({ calls: [[0, 28, 640, 420], [40, 60, 200, 120]], geometry: [40, 60, 200, 120],
    owned: false, taskbar: null, tasks: 0, gesture: null });
});

test("maximized root reflow preserves normal geometry and explicit geometry updates its restore target", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(async () => {
    const { backend, module } = /** @type {any} */ (window).managerContract;
    module._X11WasmManagerConfigureWindow = (id, x, y, w, h, mask) => {
      backend.configureWindow(id, x, y, w, h, 0, mask); backend.flush(); return 1;
    };
    module._X11WasmManagerSetWindowState = (id, state) => {
      const resource = backend.resources.get(id);
      resource.zoomed = state === 2;
      resource.wmState = "normal";
      backend.syncWindowManager(); return 1;
    };
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.mapWindow(2); backend.flush();
    backend.managerFrames.get(2).maximize.click();
    backend.rootElement.style.width = "400px";
    backend.rootElement.style.height = "300px";
    backend.syncRootElementSize(false);
    backend.syncWindowManager();
    await new Promise((resolve) => setTimeout(resolve, 0));
    const resource = backend.resources.get(2);
    const reflow = [resource.x, resource.y, resource.width, resource.height];
    const saved = backend.managerRestoreGeometries.get(2);
    backend.configureWindow(2, 20, 50, 220, 140, 0, 15);
    backend.flush();
    const explicit = [resource.x, resource.y, resource.width, resource.height];
    backend.managerFrames.get(2).maximize.click();
    return { reflow, saved, explicit, restored: [resource.x, resource.y, resource.width, resource.height] };
  });
  expect(result).toEqual({ reflow: [0, 28, 400, 240], saved: { x: 40, y: 60, width: 200, height: 120 },
    explicit: [20, 50, 220, 140], restored: [20, 50, 220, 140] });
});

test("aspect and increment constraints produce a jointly valid resize candidate", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend, calls } = /** @type {any} */ (window).managerContract;
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.resources.get(2).sizeHints = { flags: 16 | 32 | 64 | 128 | 256,
      minWidth: 80, minHeight: 60, maxWidth: 400, maxHeight: 300,
      baseWidth: 0, baseHeight: 0, widthInc: 20, heightInc: 10,
      minAspectX: 4, minAspectY: 3, maxAspectX: 4, maxAspectY: 3 };
    backend.mapWindow(2); backend.flush();
    const handle = backend.managerFrames.get(2).element.querySelector('[data-x11-wasm-decoration="resize-se"]');
    handle.dispatchEvent(new PointerEvent("pointerdown", { bubbles: true, pointerId: 9,
      button: 0, isPrimary: true, clientX: 100, clientY: 100 }));
    handle.dispatchEvent(new PointerEvent("pointermove", { bubbles: true, pointerId: 9,
      buttons: 1, clientX: 120, clientY: 110 }));
    handle.dispatchEvent(new PointerEvent("pointerup", { bubbles: true, pointerId: 9 }));
    return calls;
  });
  expect(result).toEqual([[2, 40, 60, 200, 150, 8]]);
});

test("ICCCM aspect subtracts explicit base only and base supplies a missing minimum", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend } = /** @type {any} */ (window).managerContract;
    const hints = { flags: 64 | 128 | 256, minWidth: 20, minHeight: 10,
      baseWidth: 20, baseHeight: 10, widthInc: 10, heightInc: 5,
      minAspectX: 4, minAspectY: 3, maxAspectX: 4, maxAspectY: 3 };
    return [backend.constrainSizeByHints(hints, 100, 70),
      backend.constrainSizeByHints({ ...hints, flags: 16 | 64 | 128 }, 100, 70),
      backend.constrainSizeByHints({ ...hints, flags: 256 }, 1, 1),
      backend.constrainSizeByHints({ ...hints, flags: 16 | 64 | 128 | 256,
        minWidth: 80, minHeight: 50, widthInc: 20, heightInc: 10 }, 220, 130)];
  });
  expect(result).toEqual([{ width: 100, height: 70 }, { width: 100, height: 75 },
    { width: 20, height: 10 }, { width: 220, height: 160 }]);
});

test("shared hint correction stays on feasible base increment and aspect lattices", async ({ page }) => {
  await createManagerContract(page);
  const result = await page.evaluate(() => {
    const { backend } = /** @type {any} */ (window).managerContract;
    const cases = [
      { flags: 16 | 32 | 64 | 128, minWidth: 16, minHeight: 18, baseWidth: 99, baseHeight: 99, widthInc: 8, heightInc: 6 },
      { flags: 32 | 64 | 128 | 256, baseWidth: 24, baseHeight: 17, widthInc: 7, heightInc: 11 },
      { flags: 16 | 32 | 64 | 128 | 256, minWidth: 80, minHeight: 50, baseWidth: 20, baseHeight: 10, widthInc: 20, heightInc: 10 },
    ];
    let samples = 0;
    const invalid = [];
    for (const entry of cases) {
      const hints = { ...entry, maxWidth: 400, maxHeight: 300,
        minAspectX: 4, minAspectY: 3, maxAspectX: 4, maxAspectY: 3 };
      const explicitBase = !!(hints.flags & 256);
      const minWidth = (hints.flags & 16 ? hints.minWidth : hints.baseWidth) || 1;
      const minHeight = (hints.flags & 16 ? hints.minHeight : hints.baseHeight) || 1;
      const baseWidth = explicitBase ? hints.baseWidth : minWidth;
      const baseHeight = explicitBase ? hints.baseHeight : minHeight;
      for (let width = 30; width <= 390; width += 37) {
        for (let height = 30; height <= 290; height += 41) {
          const size = backend.constrainSizeByHints(hints, width, height);
          samples++;
          if (size.width < minWidth || size.height < minHeight || size.width > 400 || size.height > 300 ||
              (size.width - baseWidth) % hints.widthInc || (size.height - baseHeight) % hints.heightInc ||
              (size.width - (explicitBase ? hints.baseWidth : 0)) * 3 !==
                (size.height - (explicitBase ? hints.baseHeight : 0)) * 4) invalid.push(size);
        }
      }
    }
    return { samples, invalid };
  });
  expect(result).toEqual({ samples: 210, invalid: [] });
});
