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
