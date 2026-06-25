// @ts-check

/** @type {import('@playwright/test').PlaywrightTestConfig} */
module.exports = {
    testDir: "tests/x11",
    timeout: 30000,
    webServer: {
	command: "python3 -m http.server 8766 -d build/browser",
	url: "http://127.0.0.1:8766/x11.html",
	reuseExistingServer: true,
	timeout: 120000,
    },
    use: {
	baseURL: "http://127.0.0.1:8766",
	trace: "retain-on-failure",
	screenshot: "only-on-failure",
    },
};
