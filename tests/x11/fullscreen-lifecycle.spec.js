// @ts-check

const { createMappedWindow, expect, test, waitForX11Runtime } = require("../support/x11");

test("mock fullscreen rejection permits retry of the same ON state", async ({ page }) => {
  await waitForX11Runtime(page);
  const id = await createMappedWindow(page);
  const result = await page.evaluate(async (id) => {
    const backend = window.Module?.x11WasmBackend;
    const element = backend?.resources.get(id)?.element;
    if (!backend || !element) {
      throw new Error("missing fullscreen lifecycle hooks");
    }
    let calls = 0;
    let active = null;
    Object.defineProperty(document, "fullscreenElement", { configurable: true, get: () => active });
    element.requestFullscreen = () => {
      calls++;
      if (calls === 1) {
        return Promise.reject(new DOMException("denied", "NotAllowedError"));
      }
      active = element;
      document.dispatchEvent(new Event("fullscreenchange"));
      return Promise.resolve();
    };
    backend.setWindowState(id, 0, 0, 1);
    await new Promise((resolve) => setTimeout(resolve, 0));
    const rejected = backend.fullscreenRequestStatus;
    backend.setWindowState(id, 0, 0, 1);
    await new Promise((resolve) => setTimeout(resolve, 0));
    return { calls, rejected, status: backend.fullscreenRequestStatus };
  }, id);
  expect(result).toEqual({ calls: 2, rejected: "error", status: "active" });
});

test("actual Chrome fullscreen enters and exits repeatedly with trusted activation", async ({ page }) => {
  await waitForX11Runtime(page);
  const id = await createMappedWindow(page);
  await page.evaluate((id) => {
    const module = window.Module;
    const backend = module?.x11WasmBackend;
    const element = backend?.resources.get(id)?.element;
    const send = module?.["_X11WasmTestSendNetWmStateMessage"];
    if (!backend || !element || typeof send !== "function") {
      throw new Error("missing native fullscreen lifecycle hooks");
    }
    const enter = document.createElement("button");
    enter.id = "native-enter";
    enter.textContent = "Enter";
    enter.addEventListener("click", () => {
      send(id, 1, 4, 0);
      backend.flush();
    });
    document.body.prepend(enter);
    const exit = document.createElement("button");
    exit.id = "native-exit";
    exit.textContent = "Exit";
    exit.addEventListener("pointerdown", (event) => event.stopPropagation());
    exit.addEventListener("pointerup", (event) => event.stopPropagation());
    exit.addEventListener("click", () => {
      send(id, 0, 4, 0);
      backend.flush();
    });
    element.append(exit);
  }, id);
  for (let attempt = 0; attempt < 2; attempt++) {
    await page.locator("#native-enter").click();
    await page.waitForFunction(() => !!document.fullscreenElement);
    await page.locator("#native-exit").click();
    await page.waitForFunction(() => !document.fullscreenElement);
  }
});
