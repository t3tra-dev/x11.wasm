// @ts-check

async function dispatchComposition(page, type, data) {
  await page.locator(".x11-wasm-input").evaluate((input, { type, data }) => {
    input.dispatchEvent(new CompositionEvent(type, { bubbles: true, data }));
  }, { type, data });
}

async function screenshotDifference(page, before, after) {
  return page.evaluate(async ({ before, after }) => {
    async function pixels(source) {
      const image = new Image();
      image.src = `data:image/png;base64,${source}`;
      await image.decode();
      const canvas = document.createElement("canvas");
      canvas.width = image.naturalWidth;
      canvas.height = image.naturalHeight;
      const context = canvas.getContext("2d");
      if (!context) throw new Error("missing screenshot decoding context");
      context.drawImage(image, 0, 0);
      return context.getImageData(0, 0, canvas.width, canvas.height);
    }
    const first = await pixels(before);
    const second = await pixels(after);
    if (first.width !== second.width || first.height !== second.height) {
      throw new Error("screenshot dimensions changed");
    }
    let count = 0, left = second.width, top = second.height, right = -1, bottom = -1;
    for (let offset = 0; offset < first.data.length; offset += 4) {
      if (Math.max(Math.abs(first.data[offset] - second.data[offset]),
        Math.abs(first.data[offset + 1] - second.data[offset + 1]),
        Math.abs(first.data[offset + 2] - second.data[offset + 2])) < 16) continue;
      const x = (offset / 4) % second.width;
      const y = Math.floor(offset / 4 / second.width);
      count++;
      left = Math.min(left, x); right = Math.max(right, x);
      top = Math.min(top, y); bottom = Math.max(bottom, y);
    }
    return { count, left, top, right, bottom };
  }, { before: before.toString("base64"), after: after.toString("base64") });
}

module.exports = { dispatchComposition, screenshotDifference };
