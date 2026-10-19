// @ts-check

const { expect, test, waitForX11Runtime } = require("../support/x11");

test("WM_STATE persists through map iconify restore withdraw and property operations", async ({ page }) => {
  await waitForX11Runtime(page);
  const result = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const id = m._X11WasmTestGeometryWindow();
    const op = (action = 0, value = 0) => m._X11WasmTestGeometryWmState(id, action, value);
    op(12);
    const initial = [op(), op(7)];
    op(1);
    const iconic = [op(), op(10), op(8)];
    op(3);
    const restored = [op(), op(10), op(8)];
    op(11);
    const unmapped = op();
    op(2);
    const withdrawn = op();
    const fetched = op(6);
    const deleted = [op(), op(7), op(8)];
    op(9, 3);
    const manual = [op(), op(10), op(8)];
    op(5);
    const removed = [op(), op(7), op(8)];
    return {initial, iconic, restored, unmapped, withdrawn, fetched, deleted, manual, removed};
  });
  expect(result).toEqual({initial: [1, 1], iconic: [3, 0, 1],
    restored: [1, 2, 1], unmapped: 1, withdrawn: 0, fetched: 0,
    deleted: [-1, 0, 2], manual: [3, 0, 1], removed: [-1, 0, 1]});
});

test("WM_STATE initial hints and invalid iconify screen do not loop or change state", async ({ page }) => {
  await waitForX11Runtime(page);
  const result = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const id = m._X11WasmTestGeometryWindow();
    const op = (action = 0, value = 0) => m._X11WasmTestGeometryWmState(id, action, value);
    op(12);
    op(11);
    op(4, 3);
    op(3);
    const initial = [op(), op(8)];
    const invalid = op(1, 99);
    return {initial, invalid, after: op(), notifications: op(8)};
  });
  expect(result).toEqual({initial: [3, 1], invalid: 0, after: 3, notifications: 0});
});
