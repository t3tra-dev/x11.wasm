// @ts-check

const { test, expect } = require("@playwright/test");
const fs = require("node:fs");

const X11_WASM_JS_LIBRARY_SOURCES = [
  "js/emscripten/library/bridge.js",
  "js/emscripten/library/window.js",
  "js/emscripten/library/draw.js",
  "js/emscripten/library/runtime.js",
  "js/emscripten/library/wm.js",
];
const X11_WASM_DOM_SOURCES = [
  "js/browser/dom/private.js",
  "js/browser/dom/types.js",
  "js/browser/dom/constants.js",
  "js/browser/dom/helpers.js",
  "js/browser/dom/backend.js",
  "js/browser/dom/window.js",
  "js/browser/dom/runtime.js",
  "js/browser/dom/draw.js",
  "js/browser/dom/wm.js",
  "js/browser/dom/input.js",
  "js/browser/dom/render.js",
  "js/browser/dom/export.js",
];

async function waitForX11Runtime(page) {
  await page.goto("/x11.html");
  await expect(page.locator("#x11-wasm-status")).toHaveText("running", {
    timeout: 15000,
  });
}

async function createMappedWindow(page, initialState = 1) {
  const windowId = await page.evaluate((state) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestMapWindowWithInitialState?: Function, x11WasmBackend?: { flush?: Function } } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestMapWindowWithInitialState) {
      throw new Error("missing X11 mapped-window test hook");
    }
    const id = module._X11WasmTestMapWindowWithInitialState(state);
    module.x11WasmBackend?.flush?.();
    return id;
  }, initialState);
  expect(windowId).toBeGreaterThan(0);
  return windowId;
}

async function createMappedWindows(page, count, initialState = 1) {
  const windowIds = [];
  for (let index = 0; index < count; index++) {
    windowIds.push(await createMappedWindow(page, initialState));
  }
  return windowIds;
}

module.exports = {
  createMappedWindow,
  createMappedWindows,
  expect,
  fs,
  test,
  waitForX11Runtime,
  X11_WASM_DOM_SOURCES,
  X11_WASM_JS_LIBRARY_SOURCES,
};
