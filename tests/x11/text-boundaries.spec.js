// @ts-check

const { test, expect, waitForX11Runtime } = require("../support/x11");
// The local Playwright type shim does not include suite options.
const scaleTest = /** @type {typeof test & {
  describe: (name: string, body: () => void) => void,
  use: (options: { deviceScaleFactor: number }) => void
}} */ (test);

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

for (const deviceScaleFactor of [1, 1.25, 2]) {
  scaleTest.describe(`text at device scale ${deviceScaleFactor}`, () => {
    scaleTest.use({ deviceScaleFactor });

    test("grapheme advances survive CSS zoom and long runs", async ({ page }) => {
      await waitForX11Runtime(page);
      const results = await page.evaluate(() => {
        const backend = /** @type {any} */ (window).Module.x11WasmBackend;
        const id = 0x740611;
        backend.createWindow(id, backend.rootWindow, 0, 0, 640, 100, 24, 0);
        backend.mapWindow(id);
        const context = backend.getContext(id);
        const segments = [
          "A", "e\u0301", "\ud83d\ude00",
          "\ud83d\udc69\u200d\ud83d\udcbb", "\ud83c\uddef\ud83c\uddf5", "x",
        ];
        const text = segments.join("").repeat(80);
        const results = [];
        for (const family of ["Helvetica", "Times", "monospace"]) {
          for (const zoom of [0.8, 1, 1.25, 2]) {
            document.documentElement.style.zoom = String(zoom);
            backend.setDrawStyle(
              3, 1, 0, 1, 0, 0, 4, [], 0, 0, 0, 0, 0, 0xffffff,
              24, family,
            );
            context.clearRect(0, 0, 640, 100);
            const calls = [];
            const fillText = context.fillText;
            context.fillText = function (segment, x, y) {
              calls.push({ segment, x });
              return fillText.call(this, segment, x, y);
            };
            try {
              backend.drawText(id, 0x000000, 8, 50, text);
            } finally {
              context.fillText = fillText;
            }
            const advances = segments.map((segment) =>
              backend.measureText(24, family, segment),
            );
            let expectedX = 8;
            const matches = calls.length === segments.length * 80 &&
              calls.every((call, index) => {
                const segmentIndex = index % segments.length;
                const matches = call.segment === segments[segmentIndex] &&
                  call.x === expectedX;
                expectedX += advances[segmentIndex];
                return matches;
              });
            results.push({
              family, zoom, matches,
              width: backend.measureText(24, family, text),
              expectedWidth: advances.reduce((sum, width) => sum + width, 0) * 80,
              ink: context.getImageData(0, 0, 640, 100).data.some(
                (value, index) => index % 4 === 3 && value > 0,
              ),
            });
          }
        }
        document.documentElement.style.zoom = "1";
        return { scale: devicePixelRatio, results };
      });
      expect(results.scale).toBe(deviceScaleFactor);
      for (const result of results.results) {
        expect(result.matches, `${result.family}, zoom ${result.zoom}`).toBe(true);
        expect(result.width).toBe(result.expectedWidth);
        expect(result.ink).toBe(true);
      }
    });
  });
}

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
