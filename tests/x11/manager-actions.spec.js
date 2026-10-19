// @ts-check

const { expect, test, waitForX11Runtime } = require("../support/x11");

const screens = /** @type {Array<[string, number, number, number]>} */ (
  [["desktop", 1280, 800, 1], ["mobile", 390, 844, 2]]);
for (const [name, width, height, dpr] of screens) {
  test(`actual native manager component move resize states and focus: ${name}`, async ({ page }) => {
    const context = await /** @type {any} */ (page).context().browser().newContext({
      viewport: { width, height }, deviceScaleFactor: dpr,
      baseURL: `http://127.0.0.1:${process.env.PLAYWRIGHT_PORT || 8766}`,
    });
    const controlled = await context.newPage();
    try {
      await waitForX11Runtime(controlled);
      const ids = await controlled.evaluate(() => {
        const m = /** @type {any} */ (window).Module;
        const b = m.x11WasmBackend;
        b.rootElement.style.width = `${Math.min(640, innerWidth - 24)}px`;
        b.rootElement.style.height = "480px";
        b.syncRootElementSize(true);
        const frame = m._X11WasmTestManagerWindow(1);
        const metadata = m._X11WasmTestManagerPart(1);
        const child = m._X11WasmTestManagerPart(2);
        m._X11WasmConfigureWindow(frame, 19, 0, 137, 88, 15);
        m._X11WasmTestManagerOptions(child, 8, 0);
        m._X11WasmTestManagerOptions(child, 3, 0);
        b.setWindowManagerEnabled(true);
        b.flush();
        return { frame, metadata, child };
      });
      const geometry = () => controlled.evaluate((id) => {
        const m = /** @type {any} */ (window).Module;
        m.x11WasmBackend.flush();
        return [0, 1, 2, 3].map((field) => m._X11WasmTestGeometryValue(id, field));
      }, ids.frame);
      expect(await geometry()).toEqual([19, 0, 137, 88]);
      await controlled.locator(`[data-x11-wasm-task="${ids.frame}"]`).click();
      expect(await geometry()).toEqual([19, 28, 137, 88]);
      const selector = `[data-x11-wasm-frame="${ids.frame}"]`;
      const titlebar = await controlled.locator(`${selector} .x11-wasm-titlebar`).boundingBox();
      await controlled.mouse.move(titlebar.x + 18, titlebar.y + 14);
      await controlled.mouse.down();
      await controlled.mouse.move(titlebar.x + 48, titlebar.y + 34);
      await controlled.mouse.up();
      expect(await geometry()).toEqual([49, 48, 137, 88]);
      const edge = await controlled.locator(`${selector} [data-x11-wasm-decoration="resize-se"]`).boundingBox();
      await controlled.mouse.move(edge.x + 4, edge.y + 4);
      await controlled.mouse.down();
      await controlled.mouse.move(edge.x + 20, edge.y + 16);
      await controlled.mouse.up();
      expect(await geometry()).toEqual([49, 48, 153, 100]);
      const original = await geometry();
      for (let repeat = 0; repeat < 3; repeat++) {
        await controlled.locator(`${selector} [data-x11-wasm-decoration="maximize"]`).click();
        const rootWidth = Math.min(640, width - 24);
        expect(await geometry()).toEqual([0, 28, rootWidth, 420]);
        await controlled.locator(`${selector} [data-x11-wasm-decoration="minimize"]`).click();
        await controlled.locator(`[data-x11-wasm-task="${ids.frame}"]`).click();
        await controlled.locator(`${selector} [data-x11-wasm-decoration="maximize"]`).click();
        expect(await geometry()).toEqual(original);
      }
      const focus = await controlled.evaluate(({ frame, child }) => {
        const m = /** @type {any} */ (window).Module;
        const b = m.x11WasmBackend;
        m._X11WasmTestManagerOptions(child, 3, 0);
        b.focusInput(true, child);
        const close = b.managerFrames.get(frame).close;
        close.focus();
        close.dispatchEvent(new KeyboardEvent("keydown", { bubbles: true, cancelable: true, key: "Enter" }));
        return { native: m._X11WasmTestManagerOptions(0, 6, 0),
          input: document.activeElement === b.inputElement,
          protocol: m._X11WasmTestManagerCloseEvent(frame), dpr: devicePixelRatio };
      }, ids);
      expect(focus).toEqual({ native: ids.child, input: true, protocol: 1, dpr });
      await controlled.screenshot({ path: `build/internal/manager-native-${name}.png` });
      const result = await controlled.evaluate(({ frame, metadata }) => {
        const b = /** @type {any} */ (window).Module.x11WasmBackend;
        const ui = b.managerFrames.get(frame);
        const close = ui.close.getBoundingClientRect();
        const hit = ui.close.contains(document.elementFromPoint(close.left + close.width / 2, close.top + close.height / 2));
        b.setWindowManagerEnabled(false);
        return { hit, gesture: b.managerGesture,
          owned: [b.resources.get(frame).managerGeometryOwned, b.resources.get(metadata).managerGeometryOwned],
          taskbar: b.managerTaskbar };
      }, ids);
      expect(result).toEqual({ hit: true, gesture: null, owned: [false, false], taskbar: null });
    } finally {
      await context.close();
    }
  });
}
