// @ts-check

const { expect, fs, test, X11_WASM_DOM_SOURCES } = require("../support/x11");

async function loadManagerBackend(page) {
  await page.setContent('<div id="x11-wasm-root" style="width:640px;height:480px"></div>');
  for (const path of X11_WASM_DOM_SOURCES) {
    await page.addScriptTag({ content: fs.readFileSync(path, "utf8") });
  }
  await page.addScriptTag({ content: fs.readFileSync("js/browser/dom/manager.js", "utf8") });
}

async function createManagedBackend(page) {
  await loadManagerBackend(page);
  await page.evaluate(() => {
    const state = /** @type {any} */ (window);
    const calls = [];
    const identities = { 2: [2, 3, 3], 9: [9, 9, 9], 10: [10, 10, 10],
      11: [11, 11, 11], 12: [12, 12, 12], 13: [13, 13, 13] };
    const module = {
      _X11WasmManagerWindowIdentity: (id, field) => identities[id]?.[field] || 0,
      _X11WasmManagerCloseWindow: (id, time) => { calls.push(["close", id, time]); return 1; },
    };
    const backend = state.X11WasmDOM.createBackend({ windowManager: true, module });
    state.managerTest = { backend, module, calls, identities };
    backend.init(640, 480, 1);
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.createWindow(3, 2, 0, 0, 200, 120, 24, 0);
    backend.setWindowTitle(3, "Client title");
    backend.setWindowProtocols(3, "WM_DELETE_WINDOW");
    backend.mapWindow(2);
    backend.mapWindow(3);
    backend.flush();
  });
}

test("window manager is disabled unless explicitly enabled", async ({ page }) => {
  await loadManagerBackend(page);
  const result = await page.evaluate(() => {
    const api = /** @type {any} */ (window).X11WasmDOM;
    const backend = api.createBackend();
    backend.init(640, 480, 1);
    backend.createWindow(2, 1, 40, 60, 200, 120, 24, 0);
    backend.mapWindow(2);
    backend.flush();
    return {
      decorated: document.querySelectorAll(".x11-wasm-frame").length,
      canvas: backend.resources.get(2).element.firstElementChild.tagName,
      parent: backend.resources.get(2).element.parentElement.id,
    };
  });
  expect(result).toEqual({ decorated: 0, canvas: "CANVAS", parent: "x11-wasm-root" });
});

test("window manager option requires an explicit boolean", async ({ page }) => {
  await loadManagerBackend(page);
  const result = await page.evaluate(() => {
    const api = /** @type {any} */ (window).X11WasmDOM;
    return [undefined, false, true, "true", {}].map((windowManager) =>
      api.createBackend({ windowManager }).windowManagerEnabled,
    );
  });
  expect(result).toEqual([false, false, true, false, false]);
});

test("native identity decorates a wrapper once without shifting client coordinates", async ({ page }) => {
  await createManagedBackend(page);
  const result = await page.evaluate(() => {
    const { backend } = /** @type {any} */ (window).managerTest;
    const wrapper = backend.resources.get(2);
    const client = backend.resources.get(3);
    const root = backend.rootElement.getBoundingClientRect();
    const rect = wrapper.element.getBoundingClientRect();
    return {
      frames: backend.managerFrames.size,
      coordinates: [rect.left - root.left, rect.top - root.top],
      nativeCoordinates: [wrapper.x, wrapper.y],
      canvasDirect: wrapper.element.firstElementChild === wrapper.canvas,
      childDirect: client.element.parentElement === wrapper.element,
      logicalParent: client.parent,
      frameHasXid: backend.displayElement(wrapper).hasAttribute("data-x11-wasm-xid"),
    };
  });
  expect(result).toEqual({ frames: 1, coordinates: [40, 60], nativeCoordinates: [40, 60],
    canvasDirect: true, childDirect: true, logicalParent: 2, frameHasXid: false });
  await expect(page.locator(".x11-wasm-window-title")).toHaveText("Client title");
});

test("only normal root InputOutput non-Widget windows are eligible", async ({ page }) => {
  await createManagedBackend(page);
  const result = await page.evaluate(() => {
    const { backend } = /** @type {any} */ (window).managerTest;
    for (const id of [9, 10, 11, 12, 13]) {
      backend.createWindow(id, 1, 250, 60, 100, 100, 24, 0, id === 9 ? 2 : 1);
      backend.mapWindow(id);
    }
    backend.resources.get(10).overrideRedirect = true;
    backend.setWindowClassHint(11, "widget", "Widget");
    backend.setWindowTypes(12, "_NET_WM_WINDOW_TYPE_MENU");
    backend.flush();
    backend.syncWindowManager();
    return Array.from(backend.managerFrames.keys());
  });
  expect(result).toEqual([2, 13]);
});

test("dynamic titles are text and close can be refused without destroying the window", async ({ page }) => {
  await createManagedBackend(page);
  await page.evaluate(() => {
    const { backend } = /** @type {any} */ (window).managerTest;
    backend.setWindowTitle(3, '<img src=x onerror="throw 1">');
  });
  await expect(page.locator(".x11-wasm-window-title")).toHaveText('<img src=x onerror="throw 1">');
  await expect(page.locator(".x11-wasm-titlebar img")).toHaveCount(0);
  await page.locator(".x11-wasm-close").click();
  const result = await page.evaluate(() => {
    const { backend, calls } = /** @type {any} */ (window).managerTest;
    backend.setWindowProtocols(3, "");
    backend.syncWindowManager();
    backend.managerFrames.get(2).close.click();
    return { calls: calls.map((call) => call.slice(0, 2)), exists: backend.resources.has(2),
      disabled: backend.managerFrames.get(2).close.disabled };
  });
  expect(result).toEqual({ calls: [["close", 2]], exists: true, disabled: true });
});

test("decoration input is isolated and close supports keyboard activation", async ({ page }) => {
  await createManagedBackend(page);
  const result = await page.evaluate(() => {
    const { backend, calls } = /** @type {any} */ (window).managerTest;
    let leaked = 0;
    for (const type of ["pointerdown", "pointerup", "pointermove", "pointerover", "pointerout",
      "wheel", "keydown", "keyup", "click"]) {
      backend.rootElement.addEventListener(type, () => leaked++);
    }
    const frame = backend.managerFrames.get(2);
    for (const type of ["pointerdown", "pointerup", "pointermove", "pointerover", "pointerout"]) {
      frame.titlebar.dispatchEvent(new PointerEvent(type, { bubbles: true, cancelable: true, pointerId: 1 }));
    }
    frame.titlebar.dispatchEvent(new WheelEvent("wheel", { bubbles: true, cancelable: true }));
    frame.close.dispatchEvent(new KeyboardEvent("keydown", { key: "Enter", bubbles: true, cancelable: true }));
    frame.close.dispatchEvent(new KeyboardEvent("keyup", { key: "Enter", bubbles: true, cancelable: true }));
    return { leaked, closeCalls: calls.length };
  });
  expect(result).toEqual({ leaked: 0, closeCalls: 1 });
});

test("repeated enable and disable restores DOM and disconnects manager listeners", async ({ page }) => {
  await createManagedBackend(page);
  const result = await page.evaluate(() => {
    const { backend, calls } = /** @type {any} */ (window).managerTest;
    let retained;
    for (let index = 0; index < 4; index++) {
      retained = backend.managerFrames.get(2).close;
      backend.setWindowManagerEnabled(false);
      retained.click();
      backend.setWindowManagerEnabled(true);
    }
    backend.managerFrames.get(2).close.click();
    backend.setWindowManagerEnabled(false);
    const resource = backend.resources.get(2);
    return { calls: calls.length, frames: document.querySelectorAll(".x11-wasm-frame").length,
      parent: resource.element.parentElement.id, geometry: [resource.element.style.left, resource.element.style.top],
      listeners: backend.managerListeners, observer: backend.managerObserver };
  });
  expect(result).toEqual({ calls: 1, frames: 0, parent: "x11-wasm-root", geometry: ["40px", "60px"],
    listeners: null, observer: null });
});

test("frame display elements preserve restacking, reparenting and destroy", async ({ page }) => {
  await createManagedBackend(page);
  const result = await page.evaluate(() => {
    const { backend } = /** @type {any} */ (window).managerTest;
    backend.createWindow(13, 1, 300, 60, 100, 100, 24, 0);
    backend.mapWindow(13);
    backend.flush();
    backend.restackWindow(2, 13);
    const frame = backend.displayElement(backend.resources.get(2));
    const previous = frame.previousElementSibling.getAttribute("data-x11-wasm-frame");
    backend.reparentWindow(2, 13, 10, 20);
    backend.flush();
    const reparented = backend.resources.get(2).element.parentElement === backend.resources.get(13).element;
    backend.destroyResource(13);
    return { previous, reparented, orphanFrames: document.querySelectorAll(".x11-wasm-frame").length };
  });
  expect(result).toEqual({ previous: "13", reparented: true, orphanFrames: 0 });
});

test("manager does not fabricate identity when the native bridge is absent", async ({ page }) => {
  await loadManagerBackend(page);
  const count = await page.evaluate(() => {
    const backend = /** @type {any} */ (window).X11WasmDOM.createBackend({ windowManager: true });
    backend.init(640, 480, 1);
    backend.createWindow(2, 1, 40, 60, 100, 100, 24, 0);
    backend.mapWindow(2);
    backend.flush();
    return backend.managerFrames.size;
  });
  expect(count).toBe(0);
});

test("Module opt-in is forwarded by the standard bridge", async ({ page }) => {
  await loadManagerBackend(page);
  await page.evaluate(() => {
    const state = /** @type {any} */ (window);
    state.LibraryManager = { library: {} };
    state.mergeInto = (target, source) => Object.assign(target, source);
  });
  await page.addScriptTag({ content: fs.readFileSync("js/emscripten/library/bridge.js", "utf8") });
  const result = await page.evaluate(() => {
    const state = /** @type {any} */ (window);
    const bridge = state.LibraryManager.library.$X11WasmBridge;
    state.X11WasmBridge = bridge;
    return [undefined, false, true, "true"].map((flag) => {
      state.Module = { x11WasmWindowManager: flag };
      bridge.backend = null;
      return bridge.getBackend().windowManagerEnabled;
    });
  });
  expect(result).toEqual([false, false, true, false]);
});
