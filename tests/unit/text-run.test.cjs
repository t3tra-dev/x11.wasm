const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const { test } = require("node:test");

function loadLibrary(heap = new Uint8Array(16384)) {
  const library = {};
  const sandbox = {
    LibraryManager: { library }, mergeInto: Object.assign,
    HEAPU8: heap, HEAP32: new Int32Array(4096),
    X11WasmBridge: { call: () => null },
  };
  vm.createContext(sandbox);
  vm.runInContext(fs.readFileSync(path.join(__dirname,
    "../../js/emscripten/library/draw.js"), "utf8"), sandbox);
  sandbox.X11WasmDecodeTextRun = library.$X11WasmDecodeTextRun;
  return { library, sandbox };
}

test("bounded decoder preserves canonical UTF-8, Tcl CESU-8 and modified NUL", () => {
  const cases = [
    [[0xf0, 0x9f, 0x98, 0x80], "\ud83d\ude00", [0, 1, 1, 1, 2]],
    [[0xed, 0xa0, 0xbd, 0xed, 0xb8, 0x80], "\ud83d\ude00", [0, 0, 0, 1, 1, 1, 2]],
    [[0x41, 0xc0, 0x80, 0x42], "A\0B", [0, 1, 1, 2, 3]],
    [[0x41, 0, 0x42], "A\0B", [0, 1, 2, 3]],
    [[0xff, 0x80, 0xc1, 0xbf], "\u00ff\u0080\u00c1\u00bf", [0, 1, 2, 3, 4]],
  ];
  for (const [bytes, expected, offsets] of cases) {
    const heap = Uint8Array.from(bytes);
    const { library } = loadLibrary(heap);
    const result = library.$X11WasmDecodeTextRun(0, bytes.length);
    assert.equal(result.text, expected);
    assert.deepEqual(Array.from(result.offsets), offsets);
  }
});

test("decoder never reads beyond the supplied slice, including invalid tails", () => {
  const bytes = [0x41, 0xed, 0xa0, 0xbd, 0xed, 0xb8, 0x80,
    0xf0, 0x9f, 0x98, 0x80, 0xc0, 0x80, 0xff, 0xf4, 0xbf, 0xbf, 0xbf];
  for (let length = 0; length <= bytes.length; length++) {
    const heap = new Proxy(bytes.slice(0, length), {
      get(target, key) {
        if (/^\d+$/.test(String(key))) {
          assert.ok(Number(key) < length, `read ${String(key)} past ${length}`);
        }
        return target[key];
      },
    });
    const { library } = loadLibrary(heap);
    const result = library.$X11WasmDecodeTextRun(0, length);
    assert.equal(result.offsets.length, length + 1);
    assert.equal(result.offsets[0], 0);
    assert.equal(result.offsets[length], result.text.length);
    result.offsets.forEach((offset, index) => {
      assert.ok(offset >= 0 && offset <= result.text.length);
      if (index) assert.ok(offset >= result.offsets[index - 1]);
    });
    assert.equal(library.$X11WasmDecodeTextRun(0, length + 1), null);
  }
});

test("run-width bridge maps Tcl source bytes and rejects invalid advances", () => {
  const { library, sandbox } = loadLibrary();
  sandbox.HEAPU8.set([0x41, 0xed, 0xa0, 0xbd, 0xed, 0xb8, 0x80], 16);
  sandbox.X11WasmBridge.call = () => [0, 11, 11, 34];
  assert.equal(library.X11WasmJsTextRunWidths(32, 0, 16, 7, 64), 0);
  assert.deepEqual(Array.from(sandbox.HEAP32.slice(16, 24)),
    [0, 11, 11, 11, 11, 11, 11, 34]);
  for (const invalid of [undefined, null, 0, [], [1, 11, 11, 34], [0, 11, 10, 34],
    [0, 11, -1, 34], [0, 11, 11, 2 ** 31], [0, 11, 11, NaN], [0, 1.5, 2, 3]]) {
    sandbox.HEAP32.fill(-42);
    sandbox.X11WasmBridge.call = () => invalid;
    assert.equal(library.X11WasmJsTextRunWidths(32, 0, 16, 7, 64), -1);
    assert.ok(sandbox.HEAP32.every((value) => value === -42));
  }
  sandbox.X11WasmBridge.call = () => { throw new Error("backend unavailable"); };
  assert.equal(library.X11WasmJsTextRunWidths(32, 0, 16, 7, 64), -1);
  assert.ok(sandbox.HEAP32.every((value) => value === -42));
});

test("draw and legacy measure share decoding for long Tcl UTF runs", () => {
  const { library, sandbox } = loadLibrary();
  const bytes = [0xed, 0xa0, 0xbd, 0xed, 0xb8, 0x80, 0xc0, 0x80];
  sandbox.HEAPU8.set(Array.from({ length: 10 }, () => bytes).flat(), 16);
  const calls = [];
  sandbox.X11WasmBridge.call = (method, args) => {
    calls.push([method, args]);
    return 0;
  };
  library.X11WasmJsDrawText(1, 0, 0, 0, 16, 80);
  library.X11WasmJsMeasureText(32, 0, 16, 80);
  assert.equal(calls[0][1][4], "\ud83d\ude00\0".repeat(10));
  assert.equal(calls[1][1][2], calls[0][1][4]);
});
