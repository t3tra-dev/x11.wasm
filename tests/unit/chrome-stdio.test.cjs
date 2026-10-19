const assert = require("node:assert/strict");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const { spawn } = require("node:child_process");
const { once } = require("node:events");
const { test } = require("node:test");
const { createChromeStdioWrapper, chromeStdioLaunchOptions } = require("../support/chrome-stdio");

function stderrDescendant() {
  const fs = require("node:fs");
  const [release, done] = process.argv.slice(1);
  const timer = setInterval(() => {
    if (!fs.existsSync(release)) return;
    process.stderr.write("descendant done\n");
    fs.writeFileSync(done, "done");
    clearInterval(timer);
  }, 10);
}

function stderrParent(descendantSource) {
  const fs = require("node:fs");
  const { spawn } = require("node:child_process");
  const [release, done, marker] = process.argv.slice(1);
  const descendant = spawn(process.execPath, ["-e", descendantSource, release, done],
    { stdio: ["ignore", "ignore", 2] });
  descendant.unref();
  fs.writeSync(3, "fd3:" + marker);
  fs.writeSync(4, "fd4:" + marker);
  process.stdout.write("parent stdout\n");
  process.stderr.write("parent stderr\n");
}

const inheritedStderrChild = `(${stderrParent.toString()})(${JSON.stringify(`(${stderrDescendant.toString()})()`)});`;

async function waitForFile(file) {
  for (let attempt = 0; attempt < 200; attempt++) {
    if (fs.existsSync(file)) return;
    await new Promise(resolve => setTimeout(resolve, 10));
  }
  throw new Error(`controlled descendant did not finish: ${file}`);
}

function launch(executable, args) {
  const child = spawn(executable, args, { stdio: ["ignore", "pipe", "pipe", "pipe", "pipe"] });
  const output = ["", "", "", "", ""];
  for (const fd of [1, 2, 3, 4]) {
    child.stdio[fd].setEncoding("utf8");
    child.stdio[fd].on("data", value => { output[fd] += value; });
  }
  let closed = false;
  const exit = once(child, "exit");
  const close = once(child, "close").then(value => { closed = true; return value; });
  return { child, output, exit, close, isClosed: () => closed };
}

test("inherited stderr delays close after exit; wrapper closes before descendant release and retains CDP FDs", async () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "chrome's-stdio-"));
  const marker = "argument with spaces $; ' \u3042";
  try {
    for (const wrapped of [false, true]) {
      const directory = path.join(root, wrapped ? "wrapped" : "direct");
      fs.mkdirSync(directory);
      const release = path.join(directory, "release");
      const done = path.join(directory, "done");
      const wrapper = wrapped ? createChromeStdioWrapper(process.execPath, directory) : null;
      const run = launch(wrapper?.executablePath || process.execPath,
        ["-e", inheritedStderrChild, release, done, marker]);
      try {
        assert.deepEqual(await run.exit, [0, null]);
        if (wrapped) {
          const completed = await Promise.race([
            run.close.then(() => true),
            new Promise(resolve => setTimeout(() => resolve(false), 1000)),
          ]);
          assert.equal(completed, true, "close must not wait for inherited stderr");
          assert.equal(fs.existsSync(release), false);
          assert.equal(fs.existsSync(done), false);
          assert.equal(run.output[1], "");
          assert.equal(run.output[2], "");
        } else {
          await new Promise(setImmediate);
          assert.equal(run.isClosed(), false, "direct child still has a descendant stderr writer");
        }
      } finally {
        fs.writeFileSync(release, "release");
        try {
          await waitForFile(done);
        } catch (error) {
          error.message += `; stderr: ${run.output[2]}`;
          throw error;
        }
        await run.close;
      }
      assert.equal(run.output[3], `fd3:${marker}`);
      assert.equal(run.output[4], `fd4:${marker}`);
      if (wrapper) {
        const log = fs.readFileSync(path.join(wrapper.directory, `chrome-${run.child.pid}.log`), "utf8");
        assert.match(log, /parent stdout/);
        assert.match(log, /parent stderr/);
        assert.match(log, /descendant done/);
      } else {
        assert.match(run.output[2], /parent stderr/);
        assert.match(run.output[2], /descendant done/);
      }
    }
  } finally {
    fs.rmSync(root, { recursive: true, force: true });
  }
});

test("wrapper generation rejects relative and missing browser executables", () => {
  assert.throws(() => createChromeStdioWrapper("chrome", os.tmpdir()), /absolute path/);
  assert.throws(() => createChromeStdioWrapper("/nonexistent/chrome", os.tmpdir()), /ENOENT/);
});

test("stdio isolation is absent by default and requires explicit Chrome channel", () => {
  assert.equal(chromeStdioLaunchOptions(undefined, undefined), undefined);
  assert.equal(chromeStdioLaunchOptions("chrome", ""), undefined);
  assert.throws(() => chromeStdioLaunchOptions(undefined, os.tmpdir()), /PLAYWRIGHT_CHANNEL=chrome/);
  assert.throws(() => chromeStdioLaunchOptions("chromium", os.tmpdir()), /PLAYWRIGHT_CHANNEL=chrome/);
});

test("wrapper preserves native failure exit status", async () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "chrome-stdio-failure-"));
  try {
    const wrapper = createChromeStdioWrapper(process.execPath, root);
    const run = launch(wrapper.executablePath,
      ["-e", 'process.stderr.write("native failure\\n"); process.exitCode = 23;']);
    assert.deepEqual(await run.exit, [23, null]);
    assert.deepEqual(await run.close, [23, null]);
    assert.match(fs.readFileSync(path.join(wrapper.directory, `chrome-${run.child.pid}.log`), "utf8"), /native failure/);
  } finally {
    fs.rmSync(root, { recursive: true, force: true });
  }
});
