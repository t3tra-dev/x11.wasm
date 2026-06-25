// @ts-check

const {
  createMappedWindow,
  createMappedWindows,
  expect,
  fs,
  test,
  waitForX11Runtime,
  X11_WASM_DOM_SOURCES,
  X11_WASM_JS_LIBRARY_SOURCES,
} = require("../support/x11");

test("XSetScreenSaver updates browser root metadata", async ({ page }) => {
  await waitForX11Runtime(page);

  const settings = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetScreenSaverSettings?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    if (!module?._X11WasmTestSetScreenSaverSettings || !backend || !root) {
      throw new Error("missing screen saver settings test hooks");
    }

    const setResult = module._X11WasmTestSetScreenSaverSettings(1);
    const afterSet = {
      allowExposures: backend.screenSaverAllowExposures,
      interval: backend.screenSaverInterval,
      preferBlanking: backend.screenSaverPreferBlanking,
      rootAllowExposures: root.getAttribute(
        "data-x11-wasm-screensaver-allow-exposures",
      ),
      rootInterval: root.getAttribute("data-x11-wasm-screensaver-interval"),
      rootPreferBlanking: root.getAttribute(
        "data-x11-wasm-screensaver-prefer-blanking",
      ),
      rootTimeout: root.getAttribute("data-x11-wasm-screensaver-timeout"),
      timeout: backend.screenSaverTimeout,
    };
    const clearResult = module._X11WasmTestSetScreenSaverSettings(0);
    const afterClear = {
      allowExposures: backend.screenSaverAllowExposures,
      interval: backend.screenSaverInterval,
      preferBlanking: backend.screenSaverPreferBlanking,
      rootAllowExposures: root.getAttribute(
        "data-x11-wasm-screensaver-allow-exposures",
      ),
      rootInterval: root.getAttribute("data-x11-wasm-screensaver-interval"),
      rootPreferBlanking: root.getAttribute(
        "data-x11-wasm-screensaver-prefer-blanking",
      ),
      rootTimeout: root.getAttribute("data-x11-wasm-screensaver-timeout"),
      timeout: backend.screenSaverTimeout,
    };
    return { afterClear, afterSet, clearResult, setResult };
  });

  expect(settings).toEqual({
    setResult: 0,
    clearResult: 0,
    afterSet: {
      allowExposures: 1,
      interval: 30,
      preferBlanking: 1,
      rootAllowExposures: "1",
      rootInterval: "30",
      rootPreferBlanking: "1",
      rootTimeout: "120",
      timeout: 120,
    },
    afterClear: {
      allowExposures: 2,
      interval: 600,
      preferBlanking: 2,
      rootAllowExposures: "2",
      rootInterval: "600",
      rootPreferBlanking: "2",
      rootTimeout: "600",
      timeout: 600,
    },
  });
});

test("XSetScreenSaver uses browser wake lock when blanking is disabled", async ({
  page,
}) => {
  await page.addInitScript(() => {
    const target =
      /** @type {Window & { __wakeLockCalls?: unknown[], __wakeLockSentinel?: unknown }} */ (
        window
      );
    target.__wakeLockCalls = [];

    class FakeWakeLockSentinel extends EventTarget {
      /**
       * @param {string} type
       */
      constructor(type) {
        super();
        this.type = type;
        this.released = false;
      }

      release() {
        target.__wakeLockCalls?.push({ kind: "release", type: this.type });
        this.released = true;
        this.dispatchEvent(new Event("release"));
        return Promise.resolve();
      }
    }

    Object.defineProperty(navigator, "wakeLock", {
      configurable: true,
      value: {
        request: async (type) => {
          target.__wakeLockCalls?.push({ kind: "request", type });
          const sentinel = new FakeWakeLockSentinel(type);
          target.__wakeLockSentinel = sentinel;
          return sentinel;
        },
      },
    });
  });

  await waitForX11Runtime(page);

  const setResult = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetScreenSaverWakeLock?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestSetScreenSaverWakeLock) {
      throw new Error("missing screen wake lock test hook");
    }
    return module._X11WasmTestSetScreenSaverWakeLock(1);
  });
  expect(setResult).toBe(0);

  await page.waitForFunction(
    () => window.Module?.x11WasmBackend?.screenWakeLockStatus === "active",
  );

  const acquired = await page.evaluate(() => {
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot }, __wakeLockCalls?: unknown[] }} */ (
        window
      ).Module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    return {
      active: root?.getAttribute("data-x11-wasm-screen-wake-lock-active"),
      calls:
        /** @type {Window & { __wakeLockCalls?: unknown[] }} */ (window)
          .__wakeLockCalls || [],
      error: backend?.screenWakeLockError,
      rootError: root?.getAttribute("data-x11-wasm-screen-wake-lock-error"),
      rootStatus: root?.getAttribute("data-x11-wasm-screen-wake-lock-status"),
      rootType: root?.getAttribute("data-x11-wasm-screen-wake-lock-type"),
      status: backend?.screenWakeLockStatus,
      type: backend?.screenWakeLockType,
    };
  });

  expect(acquired).toEqual({
    active: "1",
    calls: [{ kind: "request", type: "screen" }],
    error: "",
    rootError: null,
    rootStatus: "active",
    rootType: "screen",
    status: "active",
    type: "screen",
  });

  const clearResult = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetScreenSaverWakeLock?: Function } }} */ (
        window
      ).Module;
    if (!module?._X11WasmTestSetScreenSaverWakeLock) {
      throw new Error("missing screen wake lock test hook");
    }
    return module._X11WasmTestSetScreenSaverWakeLock(0);
  });
  expect(clearResult).toBe(0);

  await page.waitForFunction(
    () => window.Module?.x11WasmBackend?.screenWakeLockStatus === "released",
  );

  const released = await page.evaluate(() => {
    const backend =
      /** @type {Window & { Module?: { x11WasmBackend?: X11WasmBackendSnapshot }, __wakeLockCalls?: unknown[] }} */ (
        window
      ).Module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    return {
      active: root?.getAttribute("data-x11-wasm-screen-wake-lock-active"),
      calls:
        /** @type {Window & { __wakeLockCalls?: unknown[] }} */ (window)
          .__wakeLockCalls || [],
      error: backend?.screenWakeLockError,
      rootError: root?.getAttribute("data-x11-wasm-screen-wake-lock-error"),
      rootStatus: root?.getAttribute("data-x11-wasm-screen-wake-lock-status"),
      rootType: root?.getAttribute("data-x11-wasm-screen-wake-lock-type"),
      status: backend?.screenWakeLockStatus,
      type: backend?.screenWakeLockType,
    };
  });

  expect(released).toEqual({
    active: "0",
    calls: [
      { kind: "request", type: "screen" },
      { kind: "release", type: "screen" },
    ],
    error: "",
    rootError: null,
    rootStatus: "released",
    rootType: "",
    status: "released",
    type: "",
  });
});

test("X host access APIs update browser root metadata", async ({ page }) => {
  await waitForX11Runtime(page);

  const hostAccess = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetHostAccessControl?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    if (!module?._X11WasmTestSetHostAccessControl || !backend || !root) {
      throw new Error("missing host access metadata test hooks");
    }

    const setResult = module._X11WasmTestSetHostAccessControl(1);
    const afterSet = {
      enabled: backend.hostAccessEnabled,
      hosts: backend.hostAccessHosts,
      list: backend.hostAccessList,
      rootCount: root.getAttribute("data-x11-wasm-host-access-count"),
      rootEnabled: root.getAttribute("data-x11-wasm-host-access-enabled"),
      rootList: root.getAttribute("data-x11-wasm-host-access-list"),
    };
    const clearResult = module._X11WasmTestSetHostAccessControl(0);
    const afterClear = {
      enabled: backend.hostAccessEnabled,
      hosts: backend.hostAccessHosts,
      list: backend.hostAccessList,
      rootCount: root.getAttribute("data-x11-wasm-host-access-count"),
      rootEnabled: root.getAttribute("data-x11-wasm-host-access-enabled"),
      rootList: root.getAttribute("data-x11-wasm-host-access-list"),
    };
    return { afterClear, afterSet, clearResult, setResult };
  });

  expect(hostAccess).toEqual({
    setResult: 0,
    clearResult: 0,
    afterSet: {
      enabled: false,
      hosts: [
        { family: 0, length: 4, address: "c0000221" },
        {
          family: 6,
          length: 16,
          address: "20010db8000000000000000000000042",
        },
      ],
      list: "0:4:c0000221;6:16:20010db8000000000000000000000042",
      rootCount: "2",
      rootEnabled: "0",
      rootList: "0:4:c0000221;6:16:20010db8000000000000000000000042",
    },
    afterClear: {
      enabled: true,
      hosts: [],
      list: "",
      rootCount: "0",
      rootEnabled: "1",
      rootList: null,
    },
  });
});

test("X pointer events update browser pointer state metadata", async ({
  page,
}) => {
  await waitForX11Runtime(page);
  const windowId = await createMappedWindow(page);

  const metadata = await page.evaluate(({ windowId }) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmQueuePointerEvent?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    if (!module?._X11WasmQueuePointerEvent || !backend || !root) {
      throw new Error("missing pointer state test hooks");
    }

    const buttonPress = 4;
    const buttonRelease = 5;
    const motionNotify = 6;
    const button1Mask = 1 << 8;
    const rootWindow = backend.rootWindow || 0;
    const read = () => ({
      backendCaptureWindow: backend.pointerCaptureWindow,
      backendPointerState: backend.pointerState,
      backendPointerWindow: backend.pointerWindow,
      rootCaptureWindow: root.getAttribute(
        "data-x11-wasm-pointer-capture-window",
      ),
      rootPointerState: root.getAttribute("data-x11-wasm-pointer-state"),
      rootPointerWindow: root.getAttribute("data-x11-wasm-pointer-window"),
      rootX: root.getAttribute("data-x11-wasm-pointer-x"),
      rootY: root.getAttribute("data-x11-wasm-pointer-y"),
    });

    const pressResult = module._X11WasmQueuePointerEvent(
      buttonPress,
      rootWindow,
      1,
      0,
      12,
      14,
      100,
    );
    const afterPress = read();
    const motionResult = module._X11WasmQueuePointerEvent(
      motionNotify,
      rootWindow,
      0,
      button1Mask,
      30,
      20,
      101,
    );
    const afterMotion = read();
    const releaseResult = module._X11WasmQueuePointerEvent(
      buttonRelease,
      rootWindow,
      1,
      0,
      30,
      20,
      102,
    );
    const afterRelease = read();
    return {
      afterMotion,
      afterPress,
      afterRelease,
      motionResult,
      pressResult,
      releaseResult,
      windowId,
    };
  }, { windowId });

  expect(metadata.pressResult).toBe(1);
  expect(metadata.motionResult).toBe(1);
  expect(metadata.releaseResult).toBe(1);
  expect(metadata.afterPress).toEqual({
    backendCaptureWindow: windowId,
    backendPointerState: 0,
    backendPointerWindow: windowId,
    rootCaptureWindow: String(windowId),
    rootPointerState: "0",
    rootPointerWindow: String(windowId),
    rootX: "12",
    rootY: "14",
  });
  expect(metadata.afterMotion).toEqual({
    backendCaptureWindow: windowId,
    backendPointerState: 256,
    backendPointerWindow: windowId,
    rootCaptureWindow: String(windowId),
    rootPointerState: "256",
    rootPointerWindow: String(windowId),
    rootX: "30",
    rootY: "20",
  });
  expect(metadata.afterRelease).toEqual({
    backendCaptureWindow: 0,
    backendPointerState: 0,
    backendPointerWindow: windowId,
    rootCaptureWindow: "0",
    rootPointerState: "0",
    rootPointerWindow: String(windowId),
    rootX: "30",
    rootY: "20",
  });
});

test("XGrabServer updates browser root metadata", async ({ page }) => {
  await waitForX11Runtime(page);

  const serverGrab = await page.evaluate(() => {
    const module =
      /** @type {Window & { Module?: { _X11WasmTestSetServerGrabDepth?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    if (!module?._X11WasmTestSetServerGrabDepth || !backend || !root) {
      throw new Error("missing server grab metadata test hooks");
    }

    const setResult = module._X11WasmTestSetServerGrabDepth(2);
    const afterSet = {
      depth: backend.serverGrabDepth,
      rootActive: root.getAttribute("data-x11-wasm-server-grab-active"),
      rootDepth: root.getAttribute("data-x11-wasm-server-grab-depth"),
    };
    const clearResult = module._X11WasmTestSetServerGrabDepth(0);
    const afterClear = {
      depth: backend.serverGrabDepth,
      rootActive: root.getAttribute("data-x11-wasm-server-grab-active"),
      rootDepth: root.getAttribute("data-x11-wasm-server-grab-depth"),
    };
    return { afterClear, afterSet, clearResult, setResult };
  });

  expect(serverGrab).toEqual({
    setResult: 0,
    clearResult: 0,
    afterSet: {
      depth: 2,
      rootActive: "1",
      rootDepth: "2",
    },
    afterClear: {
      depth: 0,
      rootActive: "0",
      rootDepth: "0",
    },
  });
});

test("X grab APIs validate explicit timestamps", async ({ page }) => {
  await waitForX11Runtime(page);
  const [grabId, otherId] = await createMappedWindows(page, 2);

  const grabResults = await page.evaluate(({ windowId, otherWindowId }) => {
    const module =
      /** @type {Window & { Module?: { _X11WasmQueuePointerEvent?: Function, _X11WasmQueueKeyEvent?: Function, _XGrabPointer?: Function, _XUngrabPointer?: Function, _XGrabKeyboard?: Function, _XUngrabKeyboard?: Function } }} */ (
        window
      ).Module;
    if (
      !module?._X11WasmQueuePointerEvent ||
      !module._X11WasmQueueKeyEvent ||
      !module._XGrabPointer ||
      !module._XUngrabPointer ||
      !module._XGrabKeyboard ||
      !module._XUngrabKeyboard
    ) {
      throw new Error("missing grab API test hooks");
    }

    const keyPress = 2;
    const motionNotify = 6;
    const grabModeSync = 0;
    const grabModeAsync = 1;
    const currentTime = 0;
    const pointerTime = 100;
    const pointerTimedGrabTime = 120;
    const pointerTimedUngrabTime = 130;
    const keyboardTime = 200;
    const keyboardTimedGrabTime = 220;
    const keyboardTimedUngrabTime = 230;
    const exposureMask = 1 << 15;
    const pointerMotionMask = 1 << 6;

    return {
      pointerSeed: module._X11WasmQueuePointerEvent(
        motionNotify,
        windowId,
        0,
        0,
        12,
        14,
        pointerTime,
      ),
      pointerInvalidEventMask: module._XGrabPointer(
        0,
        windowId,
        1,
        exposureMask,
        grabModeAsync,
        grabModeAsync,
        0,
        0,
        pointerTime,
      ),
      pointerAtCurrentTime: module._XGrabPointer(
        0,
        windowId,
        1,
        0,
        grabModeAsync,
        grabModeAsync,
        0,
        0,
        pointerTime,
      ),
      pointerFutureTime: module._XGrabPointer(
        0,
        windowId,
        1,
        0,
        grabModeAsync,
        grabModeAsync,
        0,
        0,
        pointerTime + 1,
      ),
      pointerStaleTime: module._XGrabPointer(
        0,
        windowId,
        1,
        0,
        grabModeAsync,
        grabModeAsync,
        0,
        0,
        pointerTime - 1,
      ),
      pointerCurrentTime: module._XGrabPointer(
        0,
        windowId,
        1,
        0,
        grabModeSync,
        grabModeSync,
        0,
        0,
        currentTime,
      ),
      pointerAlreadyGrabbed: module._XGrabPointer(
        0,
        otherWindowId,
        1,
        0,
        grabModeAsync,
        grabModeAsync,
        0,
        0,
        currentTime,
      ),
      pointerUngrab: module._XUngrabPointer(0, currentTime),
      pointerTimedSeed: module._X11WasmQueuePointerEvent(
        motionNotify,
        windowId,
        0,
        0,
        12,
        14,
        pointerTimedGrabTime,
      ),
      pointerTimedGrab: module._XGrabPointer(
        0,
        windowId,
        1,
        pointerMotionMask,
        grabModeAsync,
        grabModeAsync,
        0,
        0,
        pointerTimedGrabTime,
      ),
      pointerTimedUngrabSeed: module._X11WasmQueuePointerEvent(
        motionNotify,
        windowId,
        0,
        0,
        12,
        14,
        pointerTimedUngrabTime,
      ),
      pointerTimedUngrab: module._XUngrabPointer(
        0,
        pointerTimedUngrabTime,
      ),
      pointerRegrabStaleAfterUngrab: module._XGrabPointer(
        0,
        windowId,
        1,
        0,
        grabModeAsync,
        grabModeAsync,
        0,
        0,
        pointerTimedGrabTime + 5,
      ),
      pointerRegrabCurrentAfterUngrab: module._XGrabPointer(
        0,
        windowId,
        1,
        0,
        grabModeAsync,
        grabModeAsync,
        0,
        0,
        currentTime,
      ),
      pointerFinalUngrab: module._XUngrabPointer(0, currentTime),
      keyboardSeed: module._X11WasmQueueKeyEvent(
        keyPress,
        windowId,
        0x61,
        0,
        keyboardTime,
      ),
      keyboardAtCurrentTime: module._XGrabKeyboard(
        0,
        windowId,
        0,
        grabModeAsync,
        grabModeAsync,
        keyboardTime,
      ),
      keyboardFutureTime: module._XGrabKeyboard(
        0,
        windowId,
        0,
        grabModeAsync,
        grabModeAsync,
        keyboardTime + 1,
      ),
      keyboardStaleTime: module._XGrabKeyboard(
        0,
        windowId,
        0,
        grabModeAsync,
        grabModeAsync,
        keyboardTime - 1,
      ),
      keyboardCurrentTime: module._XGrabKeyboard(
        0,
        windowId,
        0,
        grabModeSync,
        grabModeSync,
        currentTime,
      ),
      keyboardAlreadyGrabbed: module._XGrabKeyboard(
        0,
        otherWindowId,
        0,
        grabModeAsync,
        grabModeAsync,
        currentTime,
      ),
      keyboardUngrab: module._XUngrabKeyboard(0, currentTime),
      keyboardTimedSeed: module._X11WasmQueueKeyEvent(
        keyPress,
        windowId,
        0x61,
        0,
        keyboardTimedGrabTime,
      ),
      keyboardTimedGrab: module._XGrabKeyboard(
        0,
        windowId,
        0,
        grabModeAsync,
        grabModeAsync,
        keyboardTimedGrabTime,
      ),
      keyboardTimedUngrabSeed: module._X11WasmQueueKeyEvent(
        keyPress,
        windowId,
        0x61,
        0,
        keyboardTimedUngrabTime,
      ),
      keyboardTimedUngrab: module._XUngrabKeyboard(
        0,
        keyboardTimedUngrabTime,
      ),
      keyboardRegrabStaleAfterUngrab: module._XGrabKeyboard(
        0,
        windowId,
        0,
        grabModeAsync,
        grabModeAsync,
        keyboardTimedGrabTime + 5,
      ),
      keyboardRegrabCurrentAfterUngrab: module._XGrabKeyboard(
        0,
        windowId,
        0,
        grabModeAsync,
        grabModeAsync,
        currentTime,
      ),
      keyboardFinalUngrab: module._XUngrabKeyboard(0, currentTime),
    };
  }, { windowId: grabId, otherWindowId: otherId });

  expect(grabResults).toEqual({
    pointerSeed: 1,
    pointerInvalidEventMask: 2,
    pointerAtCurrentTime: 0,
    pointerFutureTime: 2,
    pointerStaleTime: 2,
    pointerCurrentTime: 0,
    pointerAlreadyGrabbed: 1,
    pointerUngrab: 0,
    pointerTimedSeed: 1,
    pointerTimedGrab: 0,
    pointerTimedUngrabSeed: 1,
    pointerTimedUngrab: 0,
    pointerRegrabStaleAfterUngrab: 2,
    pointerRegrabCurrentAfterUngrab: 0,
    pointerFinalUngrab: 0,
    keyboardSeed: 1,
    keyboardAtCurrentTime: 0,
    keyboardFutureTime: 2,
    keyboardStaleTime: 2,
    keyboardCurrentTime: 0,
    keyboardAlreadyGrabbed: 1,
    keyboardUngrab: 0,
    keyboardTimedSeed: 1,
    keyboardTimedGrab: 0,
    keyboardTimedUngrabSeed: 1,
    keyboardTimedUngrab: 0,
    keyboardRegrabStaleAfterUngrab: 2,
    keyboardRegrabCurrentAfterUngrab: 0,
    keyboardFinalUngrab: 0,
  });
});

test("X grab APIs update browser root metadata", async ({ page }) => {
  await waitForX11Runtime(page);
  const [grabId, confineId] = await createMappedWindows(page, 2);

  const metadata = await page.evaluate(({ windowId, confineWindowId }) => {
    const module =
      /** @type {Window & { Module?: { _XGrabPointer?: Function, _XUngrabPointer?: Function, _XGrabKeyboard?: Function, _XUngrabKeyboard?: Function, x11WasmBackend?: X11WasmBackendSnapshot } }} */ (
        window
      ).Module;
    const backend = module?.x11WasmBackend;
    const root = document.getElementById("x11-wasm-root");
    if (
      !module?._XGrabPointer ||
      !module._XUngrabPointer ||
      !module._XGrabKeyboard ||
      !module._XUngrabKeyboard ||
      !backend ||
      !root
    ) {
      throw new Error("missing grab metadata test hooks");
    }

    const grabModeSync = 0;
    const grabModeAsync = 1;
    const buttonPressMask = 1 << 2;
    const pointerMotionMask = 1 << 6;
    const eventMask = buttonPressMask | pointerMotionMask;

    const pointerGrab = module._XGrabPointer(
      0,
      windowId,
      1,
      eventMask,
      grabModeSync,
      grabModeAsync,
      confineWindowId,
      0,
      0,
    );
    const afterPointerGrab = {
      active: root.getAttribute("data-x11-wasm-pointer-grab-active"),
      confineWindow: backend.pointerGrabConfineWindow,
      cursor: backend.pointerGrabCursorShape,
      eventMask: backend.pointerGrabEventMask,
      keyboardMode: backend.pointerGrabKeyboardMode,
      ownerEvents: backend.pointerGrabOwnerEvents,
      pointerMode: backend.pointerGrabPointerMode,
      rootConfineWindow: root.getAttribute(
        "data-x11-wasm-pointer-grab-confine-window",
      ),
      rootCursor: root.getAttribute("data-x11-wasm-pointer-grab-cursor"),
      rootEventMask: root.getAttribute("data-x11-wasm-pointer-grab-event-mask"),
      rootKeyboardMode: root.getAttribute(
        "data-x11-wasm-pointer-grab-keyboard-mode",
      ),
      rootOwnerEvents: root.getAttribute(
        "data-x11-wasm-pointer-grab-owner-events",
      ),
      rootPointerMode: root.getAttribute("data-x11-wasm-pointer-grab-pointer-mode"),
      rootWindow: root.getAttribute("data-x11-wasm-pointer-grab-window"),
      window: backend.pointerGrabWindow,
    };
    const pointerUngrab = module._XUngrabPointer(0, 0);
    const afterPointerUngrab = {
      active: root.getAttribute("data-x11-wasm-pointer-grab-active"),
      confineWindow: backend.pointerGrabConfineWindow,
      eventMask: backend.pointerGrabEventMask,
      rootWindow: root.getAttribute("data-x11-wasm-pointer-grab-window"),
      window: backend.pointerGrabWindow,
    };

    const keyboardGrab = module._XGrabKeyboard(
      0,
      windowId,
      0,
      grabModeSync,
      grabModeAsync,
      0,
    );
    const afterKeyboardGrab = {
      active: root.getAttribute("data-x11-wasm-keyboard-grab-active"),
      keyboardMode: backend.keyboardGrabKeyboardMode,
      ownerEvents: backend.keyboardGrabOwnerEvents,
      pointerMode: backend.keyboardGrabPointerMode,
      rootKeyboardMode: root.getAttribute(
        "data-x11-wasm-keyboard-grab-keyboard-mode",
      ),
      rootOwnerEvents: root.getAttribute(
        "data-x11-wasm-keyboard-grab-owner-events",
      ),
      rootPointerMode: root.getAttribute("data-x11-wasm-keyboard-grab-pointer-mode"),
      rootWindow: root.getAttribute("data-x11-wasm-keyboard-grab-window"),
      window: backend.keyboardGrabWindow,
    };
    const keyboardUngrab = module._XUngrabKeyboard(0, 0);
    const afterKeyboardUngrab = {
      active: root.getAttribute("data-x11-wasm-keyboard-grab-active"),
      rootWindow: root.getAttribute("data-x11-wasm-keyboard-grab-window"),
      window: backend.keyboardGrabWindow,
    };

    return {
      afterKeyboardGrab,
      afterKeyboardUngrab,
      afterPointerGrab,
      afterPointerUngrab,
      keyboardGrab,
      keyboardUngrab,
      pointerGrab,
      pointerUngrab,
    };
  }, { windowId: grabId, confineWindowId: confineId });

  expect(metadata).toEqual({
    pointerGrab: 0,
    pointerUngrab: 0,
    keyboardGrab: 0,
    keyboardUngrab: 0,
    afterPointerGrab: {
      active: "1",
      confineWindow: confineId,
      cursor: 0,
      eventMask: 68,
      keyboardMode: 1,
      ownerEvents: true,
      pointerMode: 0,
      rootConfineWindow: String(confineId),
      rootCursor: "0",
      rootEventMask: "68",
      rootKeyboardMode: "1",
      rootOwnerEvents: "1",
      rootPointerMode: "0",
      rootWindow: String(grabId),
      window: grabId,
    },
    afterPointerUngrab: {
      active: "0",
      confineWindow: 0,
      eventMask: 0,
      rootWindow: null,
      window: 0,
    },
    afterKeyboardGrab: {
      active: "1",
      keyboardMode: 1,
      ownerEvents: false,
      pointerMode: 0,
      rootKeyboardMode: "1",
      rootOwnerEvents: "0",
      rootPointerMode: "0",
      rootWindow: String(grabId),
      window: grabId,
    },
    afterKeyboardUngrab: {
      active: "0",
      rootWindow: null,
      window: 0,
    },
  });
});
