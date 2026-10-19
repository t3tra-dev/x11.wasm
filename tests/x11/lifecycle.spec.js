// @ts-check

const { test, expect, X11_WASM_DOM_SOURCES } = require("../support/x11");

async function createBackend(page) {
  await page.setContent('<div id="lifecycle-root" style="width:400px;height:300px"></div>');
  for (const source of X11_WASM_DOM_SOURCES) {
    await page.addScriptTag({ path: source });
  }
  await page.evaluate(() => {
    const host = /** @type {any} */ (window);
    const backend = host.X11WasmDOM.createBackend({
      root: "#lifecycle-root",
      module: {},
    });
    host.lifecycleBackend = backend;
    host.lifecycleEvents = [];
    backend.init(400, 300, 1);
    backend.createWindow(2, 1, 10, 10, 180, 120, 0, 24, 1);
    backend.createWindow(3, 2, 5, 5, 80, 40, 0, 24, 1);
    backend.createWindow(4, 1, 210, 10, 80, 40, 0, 24, 1);
    for (const id of [2, 3, 4]) backend.mapWindow(id);
    backend.flush();
    backend.queueCompositionEvent = (phase, text, target) =>
      host.lifecycleEvents.push([phase, text, target]);
    backend.queueTextEvent = (type, text, state, time, target) =>
      host.lifecycleEvents.push([type, text, target]);
    backend.focusInput(true, 3);
    backend.setInputContext(true, 3);
  });
}

for (const action of ["destroy", "unmap"]) {
  test(`composition drops late input after ancestor ${action}`, async ({ page }) => {
    await createBackend(page);
    const result = await page.evaluate((action) => {
      const host = /** @type {any} */ (window);
      const backend = host.lifecycleBackend;
      const input = backend.inputElement;
      backend.startComposition("draft");
      if (action === "destroy") backend.destroyResource(2);
      else backend.unmapWindow(2);
      const retired = {
        composing: backend.composing,
        target: backend.compositionWindow,
        context: backend.inputContextWindow,
        focus: backend.focusedWindow,
      };
      backend.focusInput(true, 4);
      backend.setInputContext(true, 4);
      backend.updateComposition("late");
      input.value = "late";
      backend.endComposition("late");
      input.value = "late";
      input.dispatchEvent(new InputEvent("input", { inputType: "insertFromComposition", data: "late" }));
      const events = [...host.lifecycleEvents];
      backend.startComposition("fresh");
      backend.endComposition("fresh");
      return { retired, events, fresh: host.lifecycleEvents.slice(events.length) };
    }, action);
    expect(result.retired).toEqual({ composing: false, target: 0, context: 0, focus: 0 });
    expect(result.events).toEqual([[1, "draft", 3]]);
    expect(result.fresh).toEqual([
      [1, "fresh", 4], [3, "fresh", 4],
      ...[..."fresh"].map((char) => [2, char, 4]),
    ]);
  });
}

test("composition stays pinned across focus and input-context changes", async ({ page }) => {
  await createBackend(page);
  const events = await page.evaluate(() => {
    const host = /** @type {any} */ (window);
    const backend = host.lifecycleBackend;
    backend.startComposition("draft");
    backend.focusInput(true, 4);
    backend.setInputContext(true, 4);
    backend.updateComposition("text");
    backend.endComposition("text");
    return host.lifecycleEvents;
  });
  expect(events).toEqual([
    [1, "draft", 3], [2, "text", 3], [3, "text", 3],
    ...[..."text"].map((char) => [2, char, 3]),
  ]);
});

