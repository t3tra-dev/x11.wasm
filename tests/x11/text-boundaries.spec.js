// @ts-check

const { test, expect, waitForX11Runtime } = require("../support/x11");

test("generic font families preserve their CSS metrics", async ({ page }) => {
  await waitForX11Runtime(page);
  const metrics = await page.evaluate(() => {
    const backend = /** @type {any} */ (window).Module.x11WasmBackend;
    const context = document.createElement("canvas").getContext("2d");
    if (!context) {
      throw new Error("missing font measurement context");
    }
    context.fontKerning = "none";
    return ["monospace", "serif", "sans-serif", "system-ui"].map((family) => {
      context.font = `32px ${family}`;
      const text = "iW0123456789".repeat(100);
      const expected = Array.from(text).reduce(
        (sum, char) => sum + Math.ceil(context.measureText(char).width),
        0,
      );
      return {
        family,
        expected,
        actual: backend.measureText(32, family, text),
      };
    });
  });
  for (const metric of metrics) {
    expect(metric.actual, metric.family).toBe(metric.expected);
  }
});
