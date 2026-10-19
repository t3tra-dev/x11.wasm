const assert = require("node:assert/strict");
const fs = require("node:fs");
const os = require("node:os");
const { execFileSync } = require("node:child_process");
const path = require("node:path");
const { test } = require("node:test");

const configPath = path.resolve(__dirname, "../../playwright.config.js");

function readConfig(overrides = {}) {
  const env = { ...process.env };
  for (const name of ["PLAYWRIGHT_PORT", "PLAYWRIGHT_BROWSER_DIR",
    "PLAYWRIGHT_CHANNEL", "PLAYWRIGHT_REUSE_SERVER",
    "PLAYWRIGHT_CHROME_STDIO_DIR", "PLAYWRIGHT_CHROME_EXECUTABLE"]) {
    delete env[name];
  }
  return JSON.parse(execFileSync(process.execPath,
    ["-e", "process.stdout.write(JSON.stringify(require(process.argv[1])))", configPath],
    { env: { ...env, ...overrides }, encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] }));
}

test("browser server belongs to this checkout unless reuse is explicit", () => {
  const config = readConfig();
  assert.equal(config.webServer.reuseExistingServer, false);
  assert.equal(config.webServer.cwd, path.dirname(configPath));
  assert.equal(config.use.baseURL, "http://127.0.0.1:8766");
  assert.equal(readConfig({ PLAYWRIGHT_REUSE_SERVER: "1" }).webServer.reuseExistingServer, true);
  assert.equal(readConfig({ PLAYWRIGHT_REUSE_SERVER: "0" }).webServer.reuseExistingServer, false);
});

test("isolated runs can select their port, browser and read-only artifacts", () => {
  const directory = "/tmp/browser's build $data";
  const config = readConfig({ PLAYWRIGHT_PORT: "8811", PLAYWRIGHT_CHANNEL: "chrome",
    PLAYWRIGHT_BROWSER_DIR: directory });
  assert.equal(config.use.baseURL, "http://127.0.0.1:8811");
  assert.equal(config.use.channel, "chrome");
  assert.equal(config.webServer.url, "http://127.0.0.1:8811/x11.html");
  assert.equal(config.webServer.command,
    "python3 -m http.server 8811 --bind 127.0.0.1 -d '/tmp/browser'\\''s build $data'");
  assert.equal(config.workers, undefined);
  assert.equal(config.use.launchOptions, undefined);
});

test("Chrome stdio wrapper is an explicit per-run launch option", () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), "chrome-config-"));
  try {
    const logs = path.join(root, "browser's logs");
    const config = readConfig({ PLAYWRIGHT_CHANNEL: "chrome",
      PLAYWRIGHT_CHROME_STDIO_DIR: logs, PLAYWRIGHT_CHROME_EXECUTABLE: process.execPath });
    const wrapper = config.use.launchOptions.executablePath;
    assert.equal(config.use.channel, "chrome");
    assert.ok(wrapper.startsWith(logs + path.sep));
    assert.match(fs.readFileSync(wrapper, "utf8"), /"\$@"/);
    assert.deepEqual(Object.keys(config.use.launchOptions), ["executablePath"]);
    assert.equal(fs.statSync(wrapper).mode & 0o777, 0o700);
  } finally {
    fs.rmSync(root, { recursive: true, force: true });
  }
});

test("stdio isolation does not silently switch browser channels", () => {
  assert.throws(() => readConfig({ PLAYWRIGHT_CHROME_STDIO_DIR: os.tmpdir() }),
    /requires PLAYWRIGHT_CHANNEL=chrome/);
});

test("invalid ports fail before starting a server", () => {
  for (const port of ["0", "65536", "8811.5", "invalid", "8811; echo bad"]) {
    assert.throws(() => readConfig({ PLAYWRIGHT_PORT: port }),
      /PLAYWRIGHT_PORT must be an integer/);
  }
});
