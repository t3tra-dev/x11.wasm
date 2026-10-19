// @ts-check

const { test, expect, waitForX11Runtime, X11_WASM_DOM_SOURCES } = require("../support/x11");

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

for (const action of ["pointercancel", "lostpointercapture", "blur"]) {
  test(`DOM capture retires on ${action}`, async ({ page }) => {
    await createBackend(page);
    const mouse = /** @type {any} */ (page).mouse;
    const point = await page.evaluate(() => {
      const rect = /** @type {any} */ (window).lifecycleBackend.resources.get(3).element.getBoundingClientRect();
      return { x: rect.left + 10, y: rect.top + 10 };
    });
    await mouse.move(point.x, point.y);
    await mouse.down();
    await mouse.move(point.x + 1, point.y);
    try {
      const pointerId = await page.evaluate((action) => {
        const backend = /** @type {any} */ (window).lifecycleBackend;
        const pointerId = backend.capturedPointerId;
        if (action === "blur") window.dispatchEvent(new Event("blur"));
        else if (action === "lostpointercapture") backend.rootElement.releasePointerCapture(pointerId);
        else backend.rootElement.dispatchEvent(new PointerEvent("pointercancel", { pointerId }));
        return pointerId;
      }, action);
      await mouse.move(point.x + 2, point.y);
      const state = await page.evaluate((pointerId) => {
        const backend = /** @type {any} */ (window).lifecycleBackend;
        return {
          captured: backend.capturedPointerId,
          owner: backend.capturedPointerWindow,
          nativeCapture: backend.rootElement.hasPointerCapture(pointerId),
        };
      }, pointerId);
      expect(state).toEqual({ captured: null, owner: 0, nativeCapture: false });
      await mouse.up();
      await mouse.down();
      const resumed = await page.evaluate(() => {
        const backend = /** @type {any} */ (window).lifecycleBackend;
        return {
          owner: backend.capturedPointerWindow,
          captured: backend.rootElement.hasPointerCapture(backend.capturedPointerId),
        };
      });
      expect(resumed).toEqual({ owner: 3, captured: true });
    } finally {
      await mouse.up();
    }
  });
}

for (const action of ["pointercancel", "lostpointercapture"]) {
  test(`unrelated ${action} does not retire the active pointer`, async ({ page }) => {
    await createBackend(page);
    const result = await page.evaluate((action) => {
      const backend = /** @type {any} */ (window).lifecycleBackend;
      backend.capturedPointerId = 42;
      backend.capturedPointerWindow = 3;
      backend.rootElement.dispatchEvent(new PointerEvent(action, { pointerId: 41 }));
      return { pointerId: backend.capturedPointerId, owner: backend.capturedPointerWindow };
    }, action);
    expect(result).toEqual({ pointerId: 42, owner: 3 });
  });
}

for (const explicitGrab of [false, true]) {
  test(`pointer cancellation ${explicitGrab ? "preserves explicit" : "releases passive"} grabs`, async ({ page }) => {
    await waitForX11Runtime(page);
    const result = await page.evaluate((explicitGrab) =>
      /** @type {any} */ (window).Module._X11WasmTestPointerCancellation(explicitGrab ? 1 : 0), explicitGrab);
    expect(result).toBe(1);
  });
}

for (const action of ["destroy", "unmap"]) {
  test(`DOM pointer capture releases on ancestor ${action}`, async ({ page }) => {
    await createBackend(page);
    const mouse = /** @type {any} */ (page).mouse;
    const point = await page.evaluate(() => {
      const rect = /** @type {any} */ (window).lifecycleBackend.resources.get(3).element.getBoundingClientRect();
      return { x: rect.left + 10, y: rect.top + 10 };
    });
    await mouse.move(point.x, point.y);
    await mouse.down();
    try {
      const result = await page.evaluate((action) => {
        const backend = /** @type {any} */ (window).lifecycleBackend;
        const pointerId = backend.capturedPointerId;
        const root = backend.rootElement;
        const before = root.hasPointerCapture(pointerId);
        if (action === "destroy") backend.destroyResource(2);
        else backend.unmapWindow(2);
        return {
          before,
          after: root.hasPointerCapture(pointerId),
          captured: backend.capturedPointerId,
        };
      }, action);
      expect(result).toEqual({ before: true, after: false, captured: null });
    } finally {
      await mouse.up();
    }
  });

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
      const lateInput = new InputEvent("input", { data: "late" });
      // Chrome normalizes legacy inputType values in constructed InputEvents.
      Object.defineProperty(lateInput, "inputType", { value: "insertFromComposition" });
      input.dispatchEvent(lateInput);
      const events = [...host.lifecycleEvents];
      input.value = "next";
      input.dispatchEvent(new InputEvent("input", { inputType: "insertText", data: "next" }));
      backend.handleKeyDown(new KeyboardEvent("keydown", { key: "x" }));
      backend.updateComposition("late-again");
      backend.endComposition("late-again");
      input.value = "late-again";
      input.dispatchEvent(new InputEvent("input", { inputType: "insertCompositionText", data: "late-again" }));
      const normal = host.lifecycleEvents.slice(events.length);
      backend.startComposition("fresh");
      backend.endComposition("fresh");
      return { retired, events, normal, fresh: host.lifecycleEvents.slice(events.length + normal.length) };
    }, action);
    expect(result.retired).toEqual({ composing: false, target: 0, context: 0, focus: 0 });
    expect(result.events).toEqual([[1, "draft", 3]]);
    expect(result.normal).toEqual([..."next"].map((char) => [2, char, 0]));
    expect(result.fresh).toEqual([
      [1, "fresh", 4], [3, "fresh", 4],
      ...[..."fresh"].map((char) => [2, char, 4]),
    ]);
  });
}

for (const action of ["destroy", "unmap"]) {
  for (const inputType of ["insertText", "insertReplacementText", "insertFromPaste", "insertFromDrop", "", "generic"]) {
    test(`cancelled composition accepts keyless ${inputType || "untyped input"} after ${action}`, async ({ page }) => {
      await createBackend(page);
      const result = await page.evaluate(({ action, inputType }) => {
        const host = /** @type {any} */ (window);
        const backend = host.lifecycleBackend;
        const input = backend.inputElement;
        let keydowns = 0;
        input.addEventListener("keydown", () => keydowns++);
        input.dispatchEvent(new CompositionEvent("compositionstart", { data: "draft" }));
        if (action === "destroy") backend.destroyResource(2);
        else backend.unmapWindow(2);
        backend.focusInput(true, 4);
        backend.setInputContext(true, 4);
        host.lifecycleEvents.length = 0;
        for (const text of ["first", "second"]) {
          input.value = text;
          input.dispatchEvent(inputType === "generic" ? new Event("input") :
            new InputEvent("input", { inputType, data: text }));
        }
        for (const inputType of ["insertCompositionText", "insertFromComposition"]) {
          input.value = "late";
          const event = new InputEvent("input", { inputType, data: "late" });
          if (event.inputType !== inputType) {
            Object.defineProperty(event, "inputType", { value: inputType });
          }
          input.dispatchEvent(event);
        }
        input.dispatchEvent(new CompositionEvent("compositionupdate", { data: "late" }));
        input.dispatchEvent(new CompositionEvent("compositionend", { data: "late" }));
        return { events: host.lifecycleEvents, value: input.value, keydowns, active: document.activeElement === input };
      }, { action, inputType });
      expect(result).toEqual({
        events: [..."firstsecond"].map((char) => [2, char, 0]),
        value: "", keydowns: 0, active: true,
      });
    });
  }

  test(`cancelled composition accepts browser text insertion after ${action}`, async ({ page }) => {
    await createBackend(page);
    await page.evaluate((action) => {
      const host = /** @type {any} */ (window);
      const backend = host.lifecycleBackend;
      backend.inputElement.dispatchEvent(new CompositionEvent("compositionstart", { data: "draft" }));
      if (action === "destroy") backend.destroyResource(2);
      else backend.unmapWindow(2);
      backend.focusInput(true, 4);
      backend.setInputContext(true, 4);
      host.lifecycleEvents.length = 0;
      host.lifecycleKeydowns = 0;
      backend.inputElement.addEventListener("keydown", () => host.lifecycleKeydowns++);
    }, action);
    await /** @type {any} */ (page).keyboard.insertText("fresh");
    expect(await page.evaluate(() => {
      const host = /** @type {any} */ (window);
      return { events: host.lifecycleEvents, keydowns: host.lifecycleKeydowns, value: host.lifecycleBackend.inputElement.value };
    })).toEqual({ events: [..."fresh"].map((char) => [2, char, 0]), keydowns: 0, value: "" });
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

for (const [scenario, name] of ["ancestor unmap", "grab window unmap", "confine window unmap", "ancestor destroy"].entries()) {
  test(`native input lifetime follows ${name}`, async ({ page }) => {
    await waitForX11Runtime(page);
    const result = await page.evaluate((scenario) => {
      const module = /** @type {any} */ (window).Module;
      return module._X11WasmTestUnviewableInputLifetime(scenario);
    }, scenario);
    expect(result).toBe(1);
  });
}

for (const [action, name] of ["focused IC destroy", "window unmap", "window destroy", "inactive IC destroy"].entries()) {
  test(`native composition queue and DOM ownership follow ${name}`, async ({ page }) => {
    await waitForX11Runtime(page);
    const result = await page.evaluate((action) =>
      /** @type {any} */ (window).Module._X11WasmTestRetiredICInput(action), action);
    expect(result).toBe(1);
  });
}

