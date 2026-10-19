// @ts-check

const fs = require("node:fs");
const path = require("node:path");

/** @param {string} value */
function shellQuote(value) {
  return "'" + value.replace(/'/g, "'\\''") + "'";
}

/**
 * Keep Chrome descendants from holding Playwright's stdout/stderr pipes open.
 * exec retains the browser PID, arguments, and CDP descriptors 3/4.
 * @param {string} executable
 * @param {string} logRoot
 */
function createChromeStdioWrapper(executable, logRoot) {
  if (!path.isAbsolute(executable)) {
    throw new Error("Chrome executable must be an absolute path");
  }
  fs.accessSync(executable, fs.constants.X_OK);
  fs.mkdirSync(logRoot, { recursive: true, mode: 0o700 });
  const directory = fs.mkdtempSync(path.join(logRoot, "run-"));
  const executablePath = path.join(directory, "chrome-wrapper.sh");
  fs.writeFileSync(executablePath, "#!/bin/sh\n" +
    `exec ${shellQuote(executable)} "$@" >> ${shellQuote(directory)}/chrome-$$.log 2>&1\n`,
  { mode: 0o700, flag: "wx" });
  return { executablePath, directory };
}

/**
 * @param {string | undefined} channel
 * @param {string | undefined} logRoot
 * @param {string | undefined} executable
 */
function chromeStdioLaunchOptions(channel, logRoot, executable) {
  if (!logRoot) return undefined;
  if (channel !== "chrome") {
    throw new Error("PLAYWRIGHT_CHROME_STDIO_DIR requires PLAYWRIGHT_CHANNEL=chrome");
  }
  if (process.platform === "win32") {
    throw new Error("Chrome stdio isolation requires a POSIX shell");
  }
  const chrome = executable || (process.platform === "darwin"
    ? "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
    : "/opt/google/chrome/chrome");
  const wrapper = createChromeStdioWrapper(chrome, logRoot);
  console.error(`[chrome-stdio] logs: ${wrapper.directory}/chrome-<pid>.log`);
  return { executablePath: wrapper.executablePath };
}

module.exports = { createChromeStdioWrapper, chromeStdioLaunchOptions };
