// @ts-check

const { expect, test, waitForX11Runtime } = require("../support/x11");

for (const obsoleteResult of ["resolve", "reject"]) {
  test(`mock same-text clipboard write ignores obsolete ${obsoleteResult}`, async ({ page }) => {
    await waitForX11Runtime(page);
    const result = await page.evaluate(async (obsoleteResult) => {
      const module = /** @type {Window & { Module?: {
       * _X11WasmTestSetClipboardText?: (clear: number) => number,
       * x11WasmBackend?: { clipboardWriteStatus?: string, clipboardWriteError?: string }
       * } }} */ (window).Module;
      const backend = module?.x11WasmBackend;
      const setText = module?._X11WasmTestSetClipboardText;
      if (!backend || !setText) {
        throw new Error("missing clipboard lifecycle hooks");
      }
      /** @type {Array<{ resolve: () => void, reject: (reason: unknown) => void }>} */
      const pending = [];
      Object.defineProperty(navigator, "clipboard", {
        configurable: true,
        value: { writeText: () => new Promise((resolve, reject) => pending.push({ resolve: () => resolve(undefined), reject })) },
      });
      setText(0);
      setText(0);
      if (obsoleteResult === "reject") {
        pending[1].resolve();
      } else {
        pending[1].reject(new DOMException("current denial", "NotAllowedError"));
      }
      await new Promise((resolve) => setTimeout(resolve, 0));
      const current = { status: backend.clipboardWriteStatus, error: backend.clipboardWriteError };
      if (obsoleteResult === "reject") {
        pending[0].reject(new DOMException("obsolete denial", "NotAllowedError"));
      } else {
        pending[0].resolve();
      }
      await new Promise((resolve) => setTimeout(resolve, 0));
      return { writes: pending.length, current,
        final: { status: backend.clipboardWriteStatus, error: backend.clipboardWriteError } };
    }, obsoleteResult);
    expect(result.writes).toBe(2);
    expect(result.current).toEqual(obsoleteResult === "reject"
      ? { status: "written", error: "" }
      : { status: "failed", error: "current denial" });
    expect(result.final).toEqual(result.current);
  });
}
