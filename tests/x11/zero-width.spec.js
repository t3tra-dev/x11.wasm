// @ts-check

const { expect, test, X11_WASM_JS_LIBRARY_SOURCES } = require("../support/x11");

async function waitForZeroWidthRuntime(page) {
  await page.goto("/zero-width.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running");
}

const texts = ["", "e", "e\u0301", "\u0301", "\u200b", "\u200d", "\u00ad"];
const names = [
  "empty",
  "nonzero",
  "e + combining",
  "combining",
  "U+200B",
  "U+200D",
  "soft hyphen",
];

for (const [sample, text] of texts.entries()) {
  test(`X11 C APIs preserve measured advance: ${names[sample]}`, async ({ page }) => {
    await waitForZeroWidthRuntime(page);
    const result = await page.evaluate(({ sample, text }) => {
      const module = /** @type {any} */ (window).Module;
      return {
        measured: module.x11WasmBackend.measureText(14, "monospace", text),
        baseWidth: module.x11WasmBackend.measureText(14, "monospace", "e"),
        widths: Array.from({ length: 9 }, (_, api) =>
          module._X11WasmTestZeroWidth(sample, api),
        ),
      };
    }, { sample, text });
    expect(result.measured).toBe(
      sample === 1 || sample === 2 ? result.baseWidth : 0,
    );
    expect(result.widths).toEqual(Array(9).fill(result.measured));
    expect(result.baseWidth).toBeGreaterThan(0);
  });
}

test("Xutf8TextPerCharExtents preserves the combining mark's zero width", async ({ page }) => {
  await waitForZeroWidthRuntime(page);
  const width = await page.evaluate(() =>
    /** @type {any} */ (window).Module._X11WasmTestZeroWidth(2, 9),
  );
  expect(width).toBe(0);
});

test("X11 bridge distinguishes missing backend and method from measured zero", async ({ page }) => {
  await page.setContent(
    [
      "<!doctype html><script>",
      "var LibraryManager = { library: {} };",
      "function mergeInto(target, source) { Object.assign(target, source); }",
      "</script>",
    ].join(""),
  );
  for (const source of X11_WASM_JS_LIBRARY_SOURCES) {
    await page.addScriptTag({ path: source });
  }
  const result = await page.evaluate(() => {
    const target = /** @type {any} */ (window);
    const library = target.LibraryManager.library;
    const bridge = library.$X11WasmBridge;
    target.X11WasmBridge = bridge;
    target.UTF8ToString = () => "e";
    const measure = () => library.X11WasmJsMeasureText(14, 0, 1, 1);
    const missingBackend = measure();
    const genericMissingBackend = bridge.call("init", []);
    target.Module = { x11WasmBackend: {} };
    const missingMethod = measure();
    const genericMissingMethod = bridge.call("drawText", []);
    bridge.backend.measureText = () => undefined;
    const undefinedWidth = measure();
    const genericUndefined = bridge.call("measureText", []);
    bridge.backend.measureText = () => 0;
    const zeroWidth = measure();
    bridge.backend.measureText = () => 17;
    const positiveWidth = measure();
    bridge.backend.measureText = () => 2147483647;
    const maxWidth = measure();
    return {
      missingBackend,
      missingMethod,
      undefinedWidth,
      zeroWidth,
      positiveWidth,
      maxWidth,
      genericMissingBackend,
      genericMissingMethod,
      genericUndefined,
      missingBackendCount: bridge.diagnostics.missingBackendCount,
      missingMethodCount: bridge.diagnostics.missingMethodCount,
      missingMethods: bridge.diagnostics.missingMethods,
      lastMissingMethod: bridge.diagnostics.lastMissingMethod,
      lastMissingReason: bridge.diagnostics.lastMissingReason,
    };
  });
  expect(result).toEqual({
    missingBackend: -1,
    missingMethod: -1,
    undefinedWidth: -1,
    zeroWidth: 0,
    positiveWidth: 17,
    maxWidth: 2147483647,
    genericMissingBackend: 0,
    genericMissingMethod: 0,
    genericUndefined: 0,
    missingBackendCount: 2,
    missingMethodCount: 2,
    missingMethods: { measureText: 1, drawText: 1 },
    lastMissingMethod: "drawText",
    lastMissingReason: "method",
  });
});

for (const failure of [
  "missing backend",
  "missing method",
  "undefined",
  "null",
  "NaN",
  "infinity",
  "negative",
  "overflow",
  "wraparound",
  "fraction",
  "string",
  "exception",
  "backend exception",
  "no context",
]) {
  test(`X11 C APIs retain fallback for unavailable measurement: ${failure}`, async ({ page }) => {
    await waitForZeroWidthRuntime(page);
    const result = await page.evaluate((mode) => {
      const module = /** @type {any} */ (window).Module;
      const bridge = /** @type {any} */ (window).X11WasmBridge;
      const backend = module.x11WasmBackend;
      const saved = backend.measureText;
      const getBackend = bridge.getBackend;
      const getContext = HTMLCanvasElement.prototype.getContext;
      const values = {
        undefined: undefined,
        null: null,
        NaN: NaN,
        infinity: Infinity,
        negative: -1,
        overflow: 2147483648,
        wraparound: 4294967296,
        fraction: 0.5,
        string: "0",
      };
      try {
        if (mode === "missing backend") {
          bridge.getBackend = () => null;
        } else if (mode === "backend exception") {
          bridge.getBackend = () => {
            throw new Error("backend unavailable");
          };
        } else if (mode === "missing method") {
          backend.measureText = undefined;
        } else if (mode === "exception") {
          backend.measureText = () => {
            throw new Error("measurement unavailable");
          };
        } else if (mode === "no context") {
          HTMLCanvasElement.prototype.getContext = () => null;
        } else {
          backend.measureText = () => values[mode];
        }
        return {
          measured: module._X11WasmTestZeroWidth(4, 0),
          widths: Array.from({ length: 8 }, (_, api) =>
            module._X11WasmTestZeroWidth(4, api + 1),
          ),
          empty: Array.from({ length: 8 }, (_, api) =>
            module._X11WasmTestZeroWidth(0, api + 1),
          ),
          fallback: module._X11WasmTestZeroWidthFallback(4),
          domEmpty:
            mode === "no context"
              ? backend.measureText(14, "monospace", "")
              : 0,
        };
      } finally {
        backend.measureText = saved;
        bridge.getBackend = getBackend;
        HTMLCanvasElement.prototype.getContext = getContext;
      }
    }, failure);
    expect(result.measured).toBe(-1);
    expect(result.widths).toEqual(Array(8).fill(result.fallback));
    expect(result.fallback).toBeGreaterThan(0);
    expect(result.empty).toEqual(Array(8).fill(0));
    expect(result.domEmpty).toBe(0);
  });
}
