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

test("text origin is independent of the page direction", async ({ page }) => {
  await waitForX11Runtime(page);
  const samples = await page.evaluate(() => {
    const backend = /** @type {any} */ (window).Module.x11WasmBackend;
    const id = 0x740610;
    backend.createWindow(id, backend.rootWindow, 0, 0, 480, 100, 24, 0);
    backend.mapWindow(id);
    const context = backend.getContext(id);
    const samples = [];
    for (const functionId of [3, 6]) {
      backend.setDrawStyle(
        functionId, 1, 0, 1, 0, 0, 4, [], 0, 0, 0, 0, 0, 0xffffff,
        32, "Helvetica",
      );
      const images = [];
      for (const direction of ["ltr", "rtl"]) {
        document.documentElement.dir = direction;
        context.clearRect(0, 0, 480, 100);
        backend.drawText(id, 0x000000, 50, 60, "Wi e\u0301 \ud83c\uddef\ud83c\uddf5!");
        images.push(context.getImageData(0, 0, 480, 100).data);
      }
      samples.push({
        functionId,
        ink: images[0].filter((value, index) => index % 4 === 3 && value > 0).length,
        differences: images[0].filter((value, index) => value !== images[1][index]).length,
      });
    }
    document.documentElement.dir = "ltr";
    return samples;
  });
  for (const sample of samples) {
    expect(sample.ink).toBeGreaterThan(100);
    expect(sample.differences, `raster function ${sample.functionId}`).toBe(0);
  }
});
