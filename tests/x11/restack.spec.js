// @ts-check

const { expect, test, waitForX11Runtime } = require("../support/x11");

for (const managed of [false, true]) {
  test(`restack skips only actual DOM no-ops with WM ${managed ? "ON" : "OFF"}`, async ({ page }, testInfo) => {
    await waitForX11Runtime(page);
    const result = await page.evaluate((managed) => {
      const m = /** @type {any} */ (window).Module;
      const b = m.x11WasmBackend;
      b.setWindowManagerEnabled(managed);
      const create = () => {
        const frame = m._X11WasmTestManagerWindow(1);
        const client = m._X11WasmTestManagerPart(1);
        b.flush();
        return { frame, client, element: b.displayElement(b.resources.get(frame)) };
      };
      const a = create(), c = create(), d = create();
      const root = b.rootElement;
      b.restackWindow(a.frame, 0);
      b.restackWindow(c.frame, a.frame);
      b.restackWindow(d.frame, c.frame);
      const marker = document.createElement("span");
      marker.id = "restack-non-window-sibling";
      root.insertBefore(marker, c.element);
      const originalInsert = root.insertBefore;
      const originalRelated = b.restackRelatedTransientGroups;
      let insertions = 0, relatedCalls = 0;
      root.insertBefore = function (node, reference) {
        insertions++;
        return originalInsert.call(this, node, reference);
      };
      b.restackRelatedTransientGroups = function (resource) {
        relatedCalls++;
        return originalRelated.call(this, resource);
      };
      const observe = (id, previous) => {
        insertions = relatedCalls = 0;
        const element = b.displayElement(b.resources.get(id));
        const parent = element.parentNode, next = element.nextSibling;
        b.restackWindow(id, previous);
        return { insertions, relatedCalls, sameParent: parent === element.parentNode,
          sameNext: next === element.nextSibling };
      };
      try {
        const lowerNoop = observe(a.frame, 0);
        const raiseNoop = observe(d.frame, c.frame);
        const ownNextNoop = observe(d.frame, d.frame);
        // Although C is already the next logical window, a non-window sibling
        // separates it from A: moving C before that node is a genuine DOM move.
        const movePastMarker = observe(c.frame, a.frame);
        const markerOrder = a.element.nextSibling === c.element && c.element.nextSibling === marker;
        const lower = observe(d.frame, 0);
        const lowered = b.firstChildWindowElement(root) === d.element;
        const raise = observe(d.frame, c.frame);
        const raised = c.element.nextSibling === d.element;
        b.setWindowTransientFor(d.client, a.client);
        b.flush();
        originalInsert.call(root, d.element, a.element);
        const sync = observe(c.frame, c.frame);
        const transientAboveOwner = !!(a.element.compareDocumentPosition(d.element)
          & Node.DOCUMENT_POSITION_FOLLOWING);
        return { managed, ids: [a.frame, c.frame, d.frame], lowerNoop, raiseNoop, ownNextNoop,
          movePastMarker, markerOrder, lower, lowered, raise, raised, sync, transientAboveOwner,
          displayIdentity: [a, c, d].every(({ frame, client, element }) =>
            b.resources.get(client).element.parentElement === b.resources.get(frame).element &&
            (managed ? element.getAttribute("data-x11-wasm-frame") === String(frame)
              : element === b.resources.get(frame).element)) };
      } finally {
        root.insertBefore = originalInsert;
        b.restackRelatedTransientGroups = originalRelated;
      }
    }, managed);
    await testInfo.attach("restack-observations", {
      body: JSON.stringify(result, null, 2), contentType: "application/json",
    });
    for (const noop of [result.lowerNoop, result.raiseNoop, result.ownNextNoop]) {
      expect(noop).toEqual({ insertions: 0, relatedCalls: 1, sameParent: true, sameNext: true });
    }
    expect(result.movePastMarker.insertions).toBe(1);
    expect(result.markerOrder).toBe(true);
    expect(result.lower.insertions).toBe(1);
    expect(result.lowered).toBe(true);
    expect(result.raise.insertions).toBe(1);
    expect(result.raised).toBe(true);
    expect(result.sync.relatedCalls).toBe(1);
    expect(result.transientAboveOwner).toBe(true);
    expect(result.displayIdentity).toBe(true);
  });
}
