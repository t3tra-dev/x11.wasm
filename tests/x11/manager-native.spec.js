// @ts-check

const { expect, test, waitForX11Runtime } = require("../support/x11");

for (const mode of [0, 1, 2]) {
  test(`manager resolves native frame metadata and close source ${mode}`, async ({ page }) => {
    await waitForX11Runtime(page);
    const result = await page.evaluate((mode) => {
      const module = /** @type {any} */ (window).Module;
      const frame = module._X11WasmTestManagerWindow(mode);
      const metadata = module._X11WasmTestManagerPart(1);
      const widget = module._X11WasmTestManagerPart(2);
      const identity = (id) => [0, 1, 2].map((field) =>
        module._X11WasmManagerWindowIdentity(id, field));
      return {
        frame, metadata,
        fromFrame: identity(frame),
        fromMetadata: identity(metadata),
        fromWidget: identity(widget),
        close: module._X11WasmManagerCloseWindow(metadata, 123),
        delivered: module._X11WasmTestManagerCloseEvent(frame),
        alive: module._X11WasmTestGeometryValue(frame, 2),
      };
    }, mode);
    expect(result.fromFrame).toEqual([result.frame, result.metadata, result.frame]);
    expect(result.fromMetadata).toEqual(result.fromFrame);
    expect(result.fromWidget).toEqual([0, 0, 0]);
    expect(result.close).toBe(1);
    expect(result.delivered).toBe(1);
    expect(result.alive).toBe(137);
  });
}

test("manager rejects invalid unmanaged and unsupported close targets", async ({ page }) => {
  await waitForX11Runtime(page);
  const results = await page.evaluate(() => {
    const module = /** @type {any} */ (window).Module;
    const ids = [0, module._X11WasmTestManagerWindow(3),
      module._X11WasmTestManagerWindow(4)];
    const rejected = ids.map((id) => [
      module._X11WasmManagerWindowIdentity(id, 0),
      module._X11WasmManagerCloseWindow(id, 123),
    ]);
    const unsupported = module._X11WasmTestManagerWindow(5);
    return { rejected,
      unsupported: module._X11WasmManagerCloseWindow(unsupported, 123),
      badField: module._X11WasmManagerWindowIdentity(unsupported, 99) };
  });
  expect(results).toEqual({ rejected: [[0, 0], [0, 0], [0, 0]],
    unsupported: 0, badField: 0 });
});

for (const [types, eligible] of [[1, false], [16, true], [2, true],
  [3, true], [4, true], [5, false], [18, false]]) {
  test(`manager eligibility uses first recognized native window type ${types}`, async ({ page }) => {
    await waitForX11Runtime(page);
    const result = await page.evaluate(({types}) => {
      const m = /** @type {any} */ (window).Module;
      const id = m._X11WasmTestManagerWindow(1);
      m._X11WasmTestManagerOptions(id, 0, types);
      return [m._X11WasmManagerWindowIdentity(id,0), id];
    }, {types});
    expect(result[0]).toBe(eligible ? result[1] : 0);
  });
}

test("manager configure validates hints without resnapping or changing client axes", async ({ page }) => {
  await waitForX11Runtime(page);
  const result = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const frame = m._X11WasmTestManagerWindow(1);
    const client = m._X11WasmTestManagerPart(1);
    m._X11WasmTestGeometryHints(frame,1);
    const configure = (w,h,x=31,y=37,mask=15) => m._X11WasmManagerConfigureWindow(client,x,y,w,h,mask);
    const rejected = [configure(137,88),configure(0,55),configure(106,55,99999),configure(106,55,31,37,64)];
    const valid = configure(106,55);
    const frameGeometry = [0,1,2,3].map(f=>m._X11WasmTestGeometryValue(frame,f));
    const clientAxes = [0,1].map(f=>m._X11WasmTestGeometryValue(client,f));
    const exact = m._X11WasmConfigureWindow(frame,31,37,137,88,15);
    const moved = configure(0,0,41,0,1);
    return {rejected,valid,frameGeometry,clientAxes,exact,moved};
  });
  expect(result).toEqual({rejected:[0,0,0,0],valid:1,
    frameGeometry:[31,37,106,55],clientAxes:[0,0],exact:1,moved:1});
});

for (const mode of [1,2]) {
  test(`manager state notifications preserve geometry and zoom across iconify ${mode}`, async ({ page }) => {
    await waitForX11Runtime(page);
    const result = await page.evaluate((mode) => {
      const m = /** @type {any} */ (window).Module;
      const id = m._X11WasmTestManagerWindow(mode);
      const client = m._X11WasmTestManagerPart(1);
      const snapshot = () => {
        m.x11WasmBackend.flush();
        return {state:m._X11WasmTestGeometryWmState(id,0,0),
          mapped:m._X11WasmTestGeometryWmState(id,10,0),
          max:[2,3].map(f=>m._X11WasmTestWindowHasNetWmState(id,f)),
          geometry:[0,1,2,3].map(f=>m._X11WasmTestGeometryValue(id,f)),
          zoom:m.x11WasmBackend.resources.get(client).zoomed};
      };
      const values = [];
      for (const state of [2,1,2,0]) {
        values.push([m._X11WasmManagerSetWindowState(client,state,0),snapshot()]);
      }
      return values;
    },mode);
    for (let i=0;i<4;i++) expect(result[i]).toEqual([1,{state:i===1?3:1,
      mapped:i===1?0:2,max:i===3?[0,0]:[1,1],geometry:[19,23,137,88],zoom:i!==3}]);
  });
}

test("manager activate preserves child focus and obeys input hints and WM_TAKE_FOCUS", async ({ page }) => {
  await waitForX11Runtime(page);
  const result = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const frame = m._X11WasmTestManagerWindow(1);
    const client = m._X11WasmTestManagerPart(1);
    const child = m._X11WasmTestManagerPart(2);
    m._X11WasmTestManagerOptions(child,8,0);
    m._X11WasmTestManagerOptions(child,3,0);
    const other = m._X11WasmTestManagerWindow(0);
    const activated = m._X11WasmManagerActivateWindow(client,0);
    const focusedChild = m._X11WasmTestManagerOptions(0,6,0);
    const raised = m._X11WasmTestManagerOptions(0,7,0);
    m._X11WasmTestManagerOptions(other,3,0);
    m._X11WasmTestManagerOptions(frame,1,0);
    m._X11WasmTestManagerOptions(frame,2,1);
    const passive = m._X11WasmManagerActivateWindow(frame,0);
    return {frame,child,other,activated,focusedChild,raised,passive,
      focusAfter:m._X11WasmTestManagerOptions(0,6,0),
      protocol:m._X11WasmTestManagerOptions(frame,5,0)};
  });
  expect(result.activated).toBe(1);
  expect(result.focusedChild).toBe(result.child);
  expect(result.raised).toBe(result.frame);
  expect(result.passive).toBe(1);
  expect(result.focusAfter).toBe(result.other);
  expect(result.protocol).toBe(1);
});

test("manager activation accepts newly observed browser time and rejects stale focus requests", async ({ page }) => {
  await waitForX11Runtime(page);
  await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const first = m._X11WasmTestManagerWindow(0);
    const second = m._X11WasmTestManagerWindow(0);
    m._X11WasmTestManagerOptions(first,3,0);
    const button = document.createElement("button");
    button.id = "native-manager-activation";
    button.textContent = "Activate native manager window";
    button.onpointerdown = event => {
      event.preventDefault();
      const time = Math.floor(event.timeStamp);
      m.managerActivation = {first,second,time,trusted:event.isTrusted,
        current:m._X11WasmManagerActivateWindow(second,time)};
    };
    document.body.prepend(button);
  });
  await page.waitForFunction(() => performance.now() >
    /** @type {any} */ (window).Module._X11WasmTestManagerOptions(0,10,1)+1);
  await page.locator("#native-manager-activation").click();
  const result = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const result = m.managerActivation;
    const snapshot = () => [m._X11WasmTestManagerOptions(0,10,0)>>>0,
      m._X11WasmTestManagerOptions(0,10,1)>>>0,
      m._X11WasmTestManagerOptions(0,6,0),m._X11WasmTestManagerOptions(0,7,0)];
    const before = snapshot();
    const invalid = m._X11WasmManagerActivateWindow(0,result.time);
    const stale = m._X11WasmManagerActivateWindow(result.first,result.time-1);
    const future = m._X11WasmManagerActivateWindow(result.first,Math.floor(performance.now())+100000);
    const after = snapshot();
    m._X11WasmTestManagerOptions(result.second,4,3);
    const grabbedBefore = snapshot();
    const grabbed = m._X11WasmManagerActivateWindow(result.first,Math.floor(performance.now()));
    const grabbedAfter = snapshot();
    m._X11WasmTestManagerOptions(result.second,4,0);
    return {...result,before,invalid,stale,future,after,grabbedBefore,grabbed,grabbedAfter};
  });
  expect(result.trusted).toBe(true);
  expect(result.current).toBe(1);
  expect(result.before[2]).toBe(result.second);
  expect(result.before[1]).toBe(result.time);
  expect(result.invalid).toBe(0);
  expect(result.stale).toBe(0);
  expect(result.future).toBe(0);
  expect(result.after).toEqual(result.before);
  expect(result.grabbed).toBe(0);
  expect(result.grabbedAfter).toEqual(result.grabbedBefore);
});

test("manager browser time wraps without losing native focus time or client future validation", async ({ page }) => {
  await waitForX11Runtime(page);
  const result = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const first = m._X11WasmTestManagerWindow(0);
    const second = m._X11WasmTestManagerWindow(0);
    m._X11WasmQueueFocusEvent(9,first,0xfffffff0);
    m._X11WasmTestManagerOptions(first,3,0);
    const previous = Object.getOwnPropertyDescriptor(performance,"now");
    Object.defineProperty(performance,"now",{configurable:true,value:()=>4294967310});
    try {
      const accepted = m._X11WasmManagerActivateWindow(second,5);
      const snapshot = () => [m._X11WasmTestManagerOptions(0,10,0)>>>0,
        m._X11WasmTestManagerOptions(0,10,1)>>>0,
        m._X11WasmTestManagerOptions(0,6,0),m._X11WasmTestManagerOptions(0,7,0)];
      const before = snapshot();
      const stale = m._X11WasmManagerActivateWindow(first,0xfffffffe);
      const future = m._X11WasmManagerActivateWindow(first,20);
      m._X11WasmTestManagerOptions(first,11,before[0]+1);
      const after = snapshot();
      const current = m._X11WasmManagerActivateWindow(first,0);
      return {first,second,accepted,before,stale,future,after,current,
        focus:m._X11WasmTestManagerOptions(0,6,0)};
    } finally {
      if (previous) Object.defineProperty(performance,"now",previous);
      else delete /** @type {any} */ (performance).now;
    }
  });
  expect(result.accepted).toBe(1);
  expect(result.before[0]).toBeGreaterThanOrEqual(5);
  expect(result.before[1]).toBe(5);
  expect(result.before[2]).toBe(result.second);
  expect(result.stale).toBe(0);
  expect(result.future).toBe(0);
  expect(result.after).toEqual(result.before);
  expect(result.current).toBe(1);
  expect(result.focus).toBe(result.first);
});

test("native grabs refuse all manager actions without changing geometry or state", async ({ page }) => {
  await waitForX11Runtime(page);
  const result = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const id = m._X11WasmTestManagerWindow(0);
    const rejected = [];
    for (const grab of [1,2,3]) {
      m._X11WasmTestManagerOptions(id,4,grab);
      rejected.push([m._X11WasmManagerConfigureWindow(id,31,37,200,100,15),
        m._X11WasmManagerSetWindowState(id,1,0),m._X11WasmManagerActivateWindow(id,0),
        m._X11WasmManagerCloseWindow(id,0)]);
      m._X11WasmTestManagerOptions(id,4,0);
    }
    return {rejected,geometry:[0,1,2,3].map(f=>m._X11WasmTestGeometryValue(id,f)),
      mapped:m._X11WasmTestGeometryWmState(id,10,0),closed:m._X11WasmTestManagerCloseEvent(id)};
  });
  expect(result).toEqual({rejected:[[0,0,0,0],[0,0,0,0],[0,0,0,0]],geometry:[19,23,137,88],mapped:2,closed:0});
});

test("manager validates explicit base aspect and increment hints without substituting minimum size", async ({ page }) => {
  await waitForX11Runtime(page);
  const result = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const id = m._X11WasmTestManagerWindow(0);
    const configure = (w,h) => m._X11WasmManagerConfigureWindow(id,19,23,w,h,12);
    m._X11WasmTestManagerOptions(id,9,0);
    const explicit = [configure(42,29), configure(42,35)];
    m._X11WasmTestManagerOptions(id,9,1);
    const minimum = [configure(40,30), configure(40,24)];
    m._X11WasmTestManagerOptions(id,9,2);
    const baseOnly = [configure(10,5), configure(2,5), configure(42,29)];
    return {explicit,minimum,baseOnly};
  });
  expect(result).toEqual({explicit:[1,0],minimum:[1,0],baseOnly:[1,0,1]});
});

test("state actions preserve above and fullscreen atoms with a synthetic browser API refusal", async ({ page }) => {
  await page.addInitScript(() => {
    HTMLElement.prototype.requestFullscreen = () => Promise.reject(new Error("synthetic refusal"));
  });
  await waitForX11Runtime(page);
  const result = await page.evaluate(() => {
    const m = /** @type {any} */ (window).Module;
    const id = m._X11WasmTestManagerWindow(1);
    m._X11WasmManagerConfigureWindow(id,19,23,137,88,15);
    m._X11WasmTestSendNetWmStateMessage(id,1,1,4);
    const result = [];
    for (const state of [2,1,2,0]) {
      m._X11WasmManagerSetWindowState(id,state,0);
      result.push([1,2,3,4].map(f=>m._X11WasmTestWindowHasNetWmState(id,f)));
    }
    return result;
  });
  expect(result).toEqual([[1,1,1,1],[1,1,1,1],[1,1,1,1],[1,0,0,1]]);
});
