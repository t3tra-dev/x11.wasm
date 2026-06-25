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

test("X11WasmSetClipboardText updates browser clipboard bridge", async ({
  page,
}) => {
  await page.addInitScript(() => {
    const clipboard = {
      writes: /** @type {string[]} */ ([]),
      writeText(text) {
        this.writes.push(String(text));
        return Promise.resolve();
      },
    };

    Object.defineProperty(navigator, "clipboard", {
      configurable: true,
      value: clipboard,
    });
  });
  await waitForX11Runtime(page);

  const result = await page.evaluate(async () => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetClipboardText?: Function, x11WasmBackend?: { clipboardText?: string, clipboardWriteStatus?: string } } }} */ (
        window
      ).Module;
    const nativeClipboard =
      /** @type {Navigator & { clipboard?: { writes?: string[] } }} */ (
        navigator
      ).clipboard;
    if (!module?._X11WasmTestSetClipboardText || !module.x11WasmBackend) {
      throw new Error("missing clipboard API test hooks");
    }

    const setResult = module._X11WasmTestSetClipboardText(0);
    await Promise.resolve();
    const clearResult = module._X11WasmTestSetClipboardText(1);
    await Promise.resolve();

    return {
      clearResult,
      setResult,
      status: module.x11WasmBackend.clipboardWriteStatus,
      stored: module.x11WasmBackend.clipboardText,
      writes: nativeClipboard?.writes || [],
    };
  });

  expect(result).toEqual({
    clearResult: 1,
    setResult: 1,
    status: "written",
    stored: "",
    writes: ["x11.wasm clipboard api", ""],
  });
});

test("CLIPBOARD text property updates browser clipboard bridge", async ({
  page,
}) => {
  await page.addInitScript(() => {
    const clipboard = {
      writes: /** @type {string[]} */ ([]),
      writeText(text) {
        this.writes.push(String(text));
        return Promise.resolve();
      },
    };

    Object.defineProperty(navigator, "clipboard", {
      configurable: true,
      value: clipboard,
    });
  });
  await waitForX11Runtime(page);

  const result = await page.evaluate(async () => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSyncClipboardSelectionProperty?: Function, x11WasmBackend?: { clipboardText?: string, clipboardWriteStatus?: string } } }} */ (
        window
      ).Module;
    const nativeClipboard =
      /** @type {Navigator & { clipboard?: { writes?: string[] } }} */ (
        navigator
      ).clipboard;
    if (
      !module?._X11WasmTestSyncClipboardSelectionProperty ||
      !module.x11WasmBackend
    ) {
      throw new Error("missing clipboard selection sync test hooks");
    }

    const syncResult = module._X11WasmTestSyncClipboardSelectionProperty();
    await Promise.resolve();

    return {
      status: module.x11WasmBackend.clipboardWriteStatus,
      stored: module.x11WasmBackend.clipboardText,
      syncResult,
      writes: nativeClipboard?.writes || [],
    };
  });

  expect(result).toEqual({
    status: "written",
    stored: "x11.wasm selection clipboard",
    syncResult: 1,
    writes: ["x11.wasm selection clipboard"],
  });
});

test("XSetSelectionOwner updates browser selection metadata", async ({
  page,
}) => {
  await page.addInitScript(() => {
    window.addEventListener("x11wasmselectionowner", (event) => {
      const detail =
        event instanceof CustomEvent && event.detail ? event.detail : {};
      const target =
        /** @type {Window & { __selectionOwnerEvents?: unknown[] }} */ (
          window
        );
      target.__selectionOwnerEvents = target.__selectionOwnerEvents || [];
      target.__selectionOwnerEvents.push(detail);
    });
  });

  await waitForX11Runtime(page);
  const selectionId = await createMappedWindow(page);

  const metadata = await page.evaluate((windowId) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetPrimarySelectionOwner?: Function, x11WasmBackend?: X11WasmBackendSnapshot }, __selectionOwnerEvents?: unknown[] }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    if (
      !module?._X11WasmTestSetPrimarySelectionOwner ||
      !backend?.selectionOwners ||
      !root
    ) {
      throw new Error("missing selection owner metadata test hooks");
    }

    const parseOwners = () =>
      JSON.parse(root.getAttribute("data-x11-wasm-selection-owners") || "[]");

    const setResult = module._X11WasmTestSetPrimarySelectionOwner(
      windowId,
      1,
    );
    const afterSetOwner = backend.selectionOwners.get("PRIMARY");
    if (!afterSetOwner) {
      throw new Error("missing PRIMARY selection owner after set");
    }
    const afterSet = {
      count: root.getAttribute("data-x11-wasm-selection-owner-count"),
      owner: afterSetOwner,
      owners: parseOwners(),
    };
    const clearResult = module._X11WasmTestSetPrimarySelectionOwner(
      windowId,
      0,
    );
    const afterClear = {
      count: root.getAttribute("data-x11-wasm-selection-owner-count"),
      owner: backend.selectionOwners.get("PRIMARY") || null,
      owners: parseOwners(),
    };
    return {
      afterClear,
      afterSet,
      clearResult,
      events:
        /** @type {Window & { __selectionOwnerEvents?: unknown[] }} */ (window)
          .__selectionOwnerEvents || [],
      setResult,
    };
  }, selectionId);

  expect(metadata.setResult).toBe(0);
  expect(metadata.clearResult).toBe(0);
  expect(metadata.afterSet.count).toBe("1");
  expect(metadata.afterSet.owner).toEqual({
    owner: selectionId,
    time: expect.any(Number),
  });
  expect(metadata.afterSet.owners).toEqual([
    {
      owner: selectionId,
      selection: "PRIMARY",
      time: metadata.afterSet.owner.time,
    },
  ]);
  expect(metadata.afterClear).toEqual({
    count: "0",
    owner: null,
    owners: [],
  });
  expect(metadata.events).toEqual([
    {
      active: true,
      owner: selectionId,
      selection: "PRIMARY",
      time: metadata.afterSet.owner.time,
    },
    {
      active: false,
      owner: 0,
      selection: "PRIMARY",
      time: metadata.afterSet.owner.time,
    },
  ]);
});
