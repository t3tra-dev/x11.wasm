// @ts-check

const { test, expect, waitForX11Runtime } = require("../support/x11");

test("actual native focus fixture reads C focus, stacking and key queue", async ({ page }) => {
  await waitForX11Runtime(page);
  const id = await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    const id = module._X11WasmTestMapWindowWithInitialState(1);
    if (module._X11WasmTestManagerFocusChild(id) !== 1) throw new Error("native child focus failed");
    module.x11WasmBackend.flush();
    return id;
  });
  await /** @type {any} */ (page).keyboard.type("a");
  expect(await page.evaluate((id) => {
    const module = /** @type {any} */ (window).Module;
    return { focus: module._X11WasmTestManagerFocusWindow(),
      top: module._X11WasmTestManagerTopWindow(), key: module._X11WasmTestManagerChildKey(id) };
  }, id)).toEqual({ focus: id, top: id, key: 1 });
});

async function openActualManager(page) {
  await waitForX11Runtime(page);
  const missing = await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    const names = ["_X11WasmManagerWindowIdentity", "_X11WasmManagerActivateWindow",
      "_X11WasmManagerCloseWindow", "_X11WasmManagerConfigureWindow",
      "_X11WasmTestManagerWindow", "_X11WasmTestManagerPart", "_X11WasmTestManagerCloseEvent",
      "_X11WasmTestManagerFocusWindow", "_X11WasmTestManagerTopWindow",
      "_X11WasmTestManagerFocusChild", "_X11WasmTestManagerChildKey"];
    const missing = names.filter((name) => typeof module[name] !== "function");
    if (typeof module.x11WasmBackend?.setWindowManagerEnabled !== "function") missing.push("manager UI");
    return missing;
  });
  if (process.env.PLAYWRIGHT_REQUIRE_MANAGER === "1") expect(missing).toEqual([]);
  /** @type {any} */ (test).skip(missing.length > 0, `expected pending: ${missing.join(", ")}`);
  await page.evaluate(() => /** @type {any} */ (window).Module.x11WasmBackend.setWindowManagerEnabled(true));
}

async function openFocusedManager(page) {
  await openActualManager(page);
  const ids = await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    const frame = module._X11WasmTestManagerWindow(2);
    const child = module._X11WasmTestManagerPart(2);
    module._X11WasmManagerConfigureWindow(frame, 80, 90, 160, 100, 15);
    if (module._X11WasmTestManagerFocusChild(child) !== 1) throw new Error("native child focus failed");
    module.x11WasmBackend.flush();
    return { frame, child };
  });
  await expect(page.locator(".x11-wasm-input")).toBeFocused();
  return ids;
}

async function nativeFocus(page) {
  return page.evaluate(() => /** @type {any} */ (window).Module._X11WasmTestManagerFocusWindow());
}

test("actual native titlebar activation raises and focuses its frame", async ({ page }) => {
  await openActualManager(page);
  const ids = await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    const first = module._X11WasmTestManagerWindow(0);
    const second = module._X11WasmTestManagerWindow(0);
    module._X11WasmManagerConfigureWindow(first, 60, 90, 160, 100, 15);
    module._X11WasmManagerConfigureWindow(second, 280, 90, 160, 100, 15);
    module.x11WasmBackend.flush();
    return { first, second };
  });
  expect(ids.first).toBeGreaterThan(0);
  expect(ids.second).toBeGreaterThan(0);
  const first = page.locator(`[data-x11-wasm-frame="${ids.first}"]`);
  const second = page.locator(`[data-x11-wasm-frame="${ids.second}"]`);
  await page.locator(`[data-x11-wasm-frame="${ids.second}"] [data-x11-wasm-decoration="move"]`).click({ position: { x: 30, y: 10 } });
  await expect(second).toHaveAttribute("data-x11-wasm-active", "1");
  await page.locator(`[data-x11-wasm-frame="${ids.first}"] [data-x11-wasm-decoration="move"]`).click({ position: { x: 30, y: 10 } });
  await expect(first).toHaveAttribute("data-x11-wasm-active", "1");
  await expect(second).toHaveAttribute("data-x11-wasm-active", "0");
  expect(await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    const backend = module.x11WasmBackend;
    const frames = Array.from(backend.rootElement.children).filter((element) =>
      /** @type {Element} */ (element).hasAttribute("data-x11-wasm-frame"));
    return { focus: backend.focusedWindow, top: frames.at(-1)?.getAttribute("data-x11-wasm-frame"),
      nativeFocus: module._X11WasmTestManagerFocusWindow(),
      nativeTop: module._X11WasmTestManagerTopWindow(),
      activeInput: document.activeElement === backend.inputElement };
  })).toEqual({ focus: ids.first, top: String(ids.first), nativeFocus: ids.first,
    nativeTop: ids.first, activeInput: true });
});

test("actual native client activation preserves focused child and key delivery", async ({ page }) => {
  await openActualManager(page);
  const ids = await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    const frame = module._X11WasmTestManagerWindow(2);
    const child = module._X11WasmTestManagerPart(2);
    module._X11WasmManagerConfigureWindow(frame, 80, 90, 160, 100, 15);
    const focused = module._X11WasmTestManagerFocusChild(child);
    module.x11WasmBackend.flush();
    return { frame, child, focused };
  });
  expect(ids.focused).toBe(1);
  await page.locator(`[data-x11-wasm-xid="${ids.child}"]`).click({ position: { x: 10, y: 5 } });
  await /** @type {any} */ (page).keyboard.type("a");
  expect(await page.evaluate(({ child }) => {
    const module = /** @type {any} */ (window).Module;
    return { focus: module._X11WasmTestManagerFocusWindow(),
      top: module._X11WasmTestManagerTopWindow(),
      key: module._X11WasmTestManagerChildKey(child) };
  }, ids)).toEqual({ focus: ids.child, top: ids.frame, key: 1 });
});

for (const mode of [0, 1, 2]) {
  test(`actual native close queues one protocol without destroying mode ${mode}`, async ({ page }) => {
    await openActualManager(page);
    const ids = await page.evaluate((mode) => {
      const module = /** @type {any} */ (window).Module;
      const frame = module._X11WasmTestManagerWindow(mode);
      const metadata = module._X11WasmTestManagerPart(1);
      const owner = module._X11WasmManagerWindowIdentity(frame, 2);
      module._X11WasmManagerConfigureWindow(frame, 80, 90, 160, 100, 15);
      module.x11WasmBackend.flush();
      return { frame, metadata, owner };
    }, mode);
    expect(ids.owner).toBeGreaterThan(0);
    await page.locator(`[data-x11-wasm-frame="${ids.frame}"] [data-x11-wasm-decoration="close"]`).click();
    expect(await page.evaluate(({ frame, metadata, owner }) => {
      const module = /** @type {any} */ (window).Module;
      return {
        protocol: module._X11WasmTestManagerCloseEvent(owner),
        duplicate: module._X11WasmTestManagerCloseEvent(owner),
        frame: module._X11WasmManagerWindowIdentity(frame, 0),
        metadata: module._X11WasmManagerWindowIdentity(frame, 1),
        retained: module.x11WasmBackend.resources.has(metadata),
      };
    }, ids)).toEqual({ protocol: 1, duplicate: 0, frame: ids.frame, metadata: ids.metadata, retained: true });
  });
}

test("actual native trusted titlebar activation retains a focused child and raises its frame", async ({ page }) => {
  const { frame, child } = await openFocusedManager(page);
  await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    const other = module._X11WasmTestManagerWindow(0);
    module._X11WasmManagerConfigureWindow(other, 300, 90, 160, 100, 15);
    module.x11WasmBackend.flush();
    const activate = module._X11WasmManagerActivateWindow;
    module.managerActivationLog = [];
    module._X11WasmManagerActivateWindow = (id, time) => {
      const result = activate(id, time);
      module.managerActivationLog.push({ id, time, result });
      return result;
    };
    window.addEventListener("pointerdown", (event) => {
      module.managerPointer = { time: Math.floor(event.timeStamp), trusted: event.isTrusted };
    }, { capture: true, once: true });
  });
  expect(await page.evaluate(() => /** @type {any} */ (window).Module._X11WasmTestManagerTopWindow())).not.toBe(frame);
  await page.locator(`[data-x11-wasm-frame="${frame}"] [data-x11-wasm-decoration="move"]`).click({ position: { x: 30, y: 10 } });
  expect(await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    return { call: module.managerActivationLog, pointer: module.managerPointer,
      focus: module._X11WasmTestManagerFocusWindow(), top: module._X11WasmTestManagerTopWindow() };
  })).toEqual({ call: [{ id: frame, time: expect.any(Number), result: 1 }],
    pointer: { time: expect.any(Number), trusted: true }, focus: child, top: frame });
  const timing = await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    return { call: module.managerActivationLog[0].time, event: module.managerPointer.time };
  });
  expect(timing.call).toBeGreaterThan(0);
  expect(timing.call).toBe(timing.event);
  await /** @type {any} */ (page).keyboard.type("a");
  expect(await page.evaluate((child) => /** @type {any} */ (window).Module._X11WasmTestManagerChildKey(child), child)).toBe(1);
});

for (const source of ["input", "decoration"]) {
  for (const lookalike of [false, true]) {
    test(`actual native blur from ${source} to outside ${lookalike ? "lookalike" : "plain"} control is not suppressed`, async ({ page }) => {
      const { frame, child } = await openFocusedManager(page);
      if (source === "decoration") {
        await page.locator(`[data-x11-wasm-frame="${frame}"] [data-x11-wasm-decoration="close"]`).focus();
        expect(await nativeFocus(page)).toBe(child);
      }
      await page.evaluate(({ frame, lookalike }) => {
        const outside = document.createElement("div");
        const button = document.createElement("button");
        button.id = "outside-manager-focus";
        button.textContent = "Outside";
        if (lookalike) {
          outside.setAttribute("data-x11-wasm-frame", String(frame));
          button.setAttribute("data-x11-wasm-decoration", "close");
        }
        outside.append(button);
        document.body.append(outside);
      }, { frame, lookalike });
      await page.locator("#outside-manager-focus").focus();
      await expect(page.locator("#outside-manager-focus")).toBeFocused();
      await expect.poll(() => nativeFocus(page)).toBe(0);
    });
  }

  test(`actual browser tab blur from ${source} clears native focus and pointer capture`, async ({ page }) => {
    const { frame, child } = await openFocusedManager(page);
    if (source === "decoration") {
      await page.locator(`[data-x11-wasm-frame="${frame}"] [data-x11-wasm-decoration="close"]`).focus();
      expect(await nativeFocus(page)).toBe(child);
    }
    await page.evaluate(() => {
      const module = /** @type {any} */ (window).Module;
      module.managerWindowBlurs = [];
      window.addEventListener("blur", (event) => {
        if (event.target === window) module.managerWindowBlurs.push(event.isTrusted);
      }, true);
    });
    const other = await /** @type {any} */ (page).context().newPage();
    try {
      await other.goto("about:blank");
      await other.bringToFront();
      await expect.poll(() => page.evaluate(() => document.hasFocus())).toBe(false);
      await expect.poll(() => page.evaluate(() => /** @type {any} */ (window).Module.managerWindowBlurs)).toContain(true);
      await expect.poll(() => nativeFocus(page)).toBe(0);
      expect(await page.evaluate(() => {
        const backend = /** @type {any} */ (window).Module.x11WasmBackend;
        return { captured: backend.capturedPointerId, gesture: backend.managerGesture };
      })).toEqual({ captured: null, gesture: null });
    } finally {
      await other.close();
      await /** @type {any} */ (page).bringToFront();
    }
  });
}

test("actual native keyboard navigation across manager controls restores client input", async ({ page }) => {
  const { frame, child } = await openFocusedManager(page);
  const close = page.locator(`[data-x11-wasm-frame="${frame}"] [data-x11-wasm-decoration="close"]`);
  const maximize = page.locator(`[data-x11-wasm-frame="${frame}"] [data-x11-wasm-decoration="maximize"]`);
  const task = page.locator(`[data-x11-wasm-task="${frame}"]`);
  await close.focus();
  expect(await nativeFocus(page)).toBe(child);
  await /** @type {any} */ (page).keyboard.press("Shift+Tab");
  await expect(maximize).toBeFocused();
  expect(await nativeFocus(page)).toBe(child);
  await /** @type {any} */ (page).keyboard.press("Tab");
  await expect(close).toBeFocused();
  await /** @type {any} */ (page).keyboard.press("Tab");
  await expect(task).toBeFocused();
  expect(await nativeFocus(page)).toBe(child);
  await /** @type {any} */ (page).keyboard.press("Enter");
  await expect(page.locator(".x11-wasm-input")).toBeFocused();
  expect(await nativeFocus(page)).toBe(child);
  await /** @type {any} */ (page).keyboard.type("a");
  expect(await page.evaluate((child) => /** @type {any} */ (window).Module._X11WasmTestManagerChildKey(child), child)).toBe(1);
});
