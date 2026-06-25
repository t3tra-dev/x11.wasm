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

test("JS bridge required methods match the DOM backend", async () => {
  const library = X11_WASM_JS_LIBRARY_SOURCES.map((source) =>
    fs.readFileSync(source, "utf8"),
  ).join("\n");
  const backend = X11_WASM_DOM_SOURCES.map((source) =>
    fs.readFileSync(source, "utf8"),
  ).join("\n");
  const callMethods = [
    ...new Set(
      [...library.matchAll(/X11WasmBridge\.call\("([^"]+)"/g)].map(
        (match) => match[1],
      ),
    ),
  ].sort();
  const requiredBlock =
    /requiredMethods:\s*\[([\s\S]*?)\],\n\s+getModule:/.exec(library);
  expect(requiredBlock, "missing requiredMethods block").toBeTruthy();
  const requiredMethods = [
    ...new Set(
      [...(requiredBlock?.[1] || "").matchAll(/"([^"]+)"/g)].map(
        (match) => match[1],
      ),
    ),
  ].sort();
  const backendMethods = [
    ...new Set(
      [...backend.matchAll(/^\s{4}([A-Za-z_$][\w$]*)\([^)]*\)\s*\{/gm)].map(
        (match) => match[1],
      ),
    ),
  ].sort();

  expect(requiredMethods).toEqual(callMethods);
  expect(
    callMethods.filter((method) => !backendMethods.includes(method)),
  ).toEqual([]);
});

test("JS bridge runtime validation reports a complete backend", async ({
  page,
}) => {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });

  const validation = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { x11WasmValidateBridge?: Function, x11WasmBridgeDiagnostics?: { backendComplete?: boolean, requiredMethods?: string[], missingRequiredMethods?: string[] } } }} */ (
        window
      ).Module;
    if (!module?.x11WasmValidateBridge) {
      throw new Error("missing X11 wasm bridge validation hook");
    }
    return {
      result: module.x11WasmValidateBridge(),
      diagnostics: module.x11WasmBridgeDiagnostics,
    };
  });

  expect(validation.result).toBe(1);
  expect(validation.diagnostics?.backendComplete).toBe(true);
  expect(validation.diagnostics?.missingRequiredMethods).toEqual([]);
  expect(validation.diagnostics?.requiredMethods?.length).toBeGreaterThan(0);
});

test("X11 wasm bridge reports missing backend calls", async ({ page }) => {
  /** @type {string[]} */
  const warnings = [];
  page.on("console", (message) => {
    if (message.type() === "warning") {
      warnings.push(message.text());
    }
  });

  await page.setContent(
    [
      "<!doctype html>",
      "<script>",
      "var LibraryManager = { library: {} };",
      "function mergeInto(target, source) { Object.assign(target, source); }",
      "</script>",
    ].join(""),
  );
  for (const source of X11_WASM_JS_LIBRARY_SOURCES) {
    await page.addScriptTag({ path: source });
  }

  const result = await page.evaluate(() => {
    const target =
      /** @type {Window & { LibraryManager?: { library?: Record<string, any> }, X11WasmBridge?: any, Module?: any }} */ (
        window
      );
    const bridge = target.LibraryManager?.library?.$X11WasmBridge;
    if (!bridge) {
      throw new Error("missing raw X11WasmBridge library object");
    }
    target.X11WasmBridge = bridge;

    const missingBackendReturn = bridge.call("init", [1, 2, 3]);
    target.Module = {
      x11WasmBackend: {
        init: (...args) => args.length + 39,
      },
    };
    const initReturn = bridge.call("init", [1, 2, 3]);
    const missingMethodReturn = bridge.call("drawLine", []);
    const repeatedMissingMethodReturn = bridge.call("drawLine", []);
    const diagnostics = bridge.diagnostics;

    return {
      diagnostics: {
        lastMissingMethod: diagnostics.lastMissingMethod,
        lastMissingReason: diagnostics.lastMissingReason,
        missingBackendCount: diagnostics.missingBackendCount,
        missingMethodCount: diagnostics.missingMethodCount,
        missingMethods: { ...diagnostics.missingMethods },
        warnedMissing: { ...diagnostics.warnedMissing },
      },
      initReturn,
      missingBackendReturn,
      missingMethodReturn,
      moduleDiagnosticsShared:
        target.Module.x11WasmBridgeDiagnostics === diagnostics,
      repeatedMissingMethodReturn,
    };
  });

  expect(result).toEqual({
    diagnostics: {
      lastMissingMethod: "drawLine",
      lastMissingReason: "method",
      missingBackendCount: 1,
      missingMethodCount: 2,
      missingMethods: { drawLine: 2 },
      warnedMissing: {
        "backend:init": true,
        "method:drawLine": true,
      },
    },
    initReturn: 42,
    missingBackendReturn: 0,
    missingMethodReturn: 0,
    moduleDiagnosticsShared: true,
    repeatedMissingMethodReturn: 0,
  });
  expect(warnings).toContain("X11 wasm bridge missing backend for init");
  expect(warnings).toContain("X11 wasm bridge missing method for drawLine");
  expect(
    warnings.filter(
      (message) => message === "X11 wasm bridge missing method for drawLine",
    ),
  ).toHaveLength(1);
});
