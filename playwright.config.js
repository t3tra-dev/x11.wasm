// @ts-check

const path = require("node:path");
const { chromeStdioLaunchOptions } = require("./tests/support/chrome-stdio");
const channel = process.env.PLAYWRIGHT_CHANNEL || undefined;
const stdioDir = process.env.PLAYWRIGHT_CHROME_STDIO_DIR;
const port = Number(process.env.PLAYWRIGHT_PORT || 8766);
if (!Number.isInteger(port) || port < 1 || port > 65535) {
    throw new Error("PLAYWRIGHT_PORT must be an integer between 1 and 65535");
}
const browserDir = path.resolve(__dirname,
    process.env.PLAYWRIGHT_BROWSER_DIR || "build/browser");
const quotedDir = "'" + browserDir.replace(/'/g, "'\\''") + "'";
const baseURL = `http://127.0.0.1:${port}`;

/** @type {import('@playwright/test').PlaywrightTestConfig} */
module.exports = {
    testDir: "tests/x11",
    timeout: 30000,
    webServer: {
	command: `python3 -m http.server ${port} --bind 127.0.0.1 -d ${quotedDir}`,
	cwd: __dirname,
	url: `${baseURL}/x11.html`,
	reuseExistingServer: process.env.PLAYWRIGHT_REUSE_SERVER === "1",
	timeout: 120000,
    },
    use: {
	baseURL,
	channel,
	launchOptions: chromeStdioLaunchOptions(channel,
	    stdioDir ? path.resolve(__dirname, stdioDir) : undefined,
	    process.env.PLAYWRIGHT_CHROME_EXECUTABLE),
	trace: "retain-on-failure",
	screenshot: "only-on-failure",
    },
};
