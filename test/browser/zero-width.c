#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11Wasm/Browser.h>
#include <assert.h>
#include <string.h>
#include <wchar.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

static Display *display;
static XFontStruct *font;
static XFontSet fontSet;

static const char *texts[] = {
    "", "e", "e\xcc\x81", "\xcc\x81", "\xe2\x80\x8b", "\xe2\x80\x8d", "\xc2\xad"
};
static const char *coreTexts[] = {
    "", "e", "e\\u0301", "\\u0301", "\\u200b", "\\u200d", "\xad"
};
static const wchar_t wideTexts[][3] = {
    {0}, {'e', 0}, {'e', 0x0301, 0}, {0x0301, 0},
    {0x200b, 0}, {0x200d, 0}, {0x00ad, 0}
};

EMSCRIPTEN_KEEPALIVE int
X11WasmTestZeroWidthFallback(
    int sample)
{
    return (int)strlen(texts[sample]) * font->max_bounds.width;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestZeroWidth(
    int sample,
    int api)
{
    const char *text = texts[sample];
    int length = (int)strlen(text);
    int count = (int)wcslen(wideTexts[sample]);
    XRectangle ink;
    XRectangle logical;
    XRectangle perChar[2];
    XChar2b text16[2];
    int numChars;
    int i;
    int width;

    switch (api) {
    case 0:
	return X11WasmJsMeasureText(14, "monospace", text, length);
    case 1:
	return XTextWidth(font, coreTexts[sample],
		(int)strlen(coreTexts[sample]));
    case 2:
	for (i = 0; i < count; i++) {
	    text16[i].byte1 = (unsigned char)(wideTexts[sample][i] >> 8);
	    text16[i].byte2 = (unsigned char)wideTexts[sample][i];
	}
	return XTextWidth16(font, text16, count);
    case 3:
	return Xutf8TextEscapement(fontSet, text, length);
    case 4:
    case 5:
    case 6:
	width = Xutf8TextExtents(fontSet, text, length, &ink, &logical);
	return (api == 4) ? width : ((api == 5) ? logical.width : ink.width);
    case 7:
	return XmbTextEscapement(fontSet, text, length);
    case 8:
	return XwcTextEscapement(fontSet, wideTexts[sample], count);
    case 9:
	if (!Xutf8TextPerCharExtents(fontSet, text, length, NULL, perChar,
		2, &numChars, &ink, &logical) || (numChars != 2)) {
	    return -2;
	}
	return perChar[1].width;
    }
    return -2;
}

int
main(void)
{
    display = XOpenDisplay(NULL);
    assert(display != NULL);
    font = XLoadQueryFont(display, "monospace");
    fontSet = XCreateFontSet(display, "monospace", NULL, NULL, NULL);
    assert((font != NULL) && (fontSet != NULL));
#ifdef __EMSCRIPTEN__
    EM_ASM({
	document.getElementById("x11-wasm-status").textContent = "running";
    });
#else
    int sample;
    int api;

    for (sample = 0; sample < 7; sample++) {
	assert(X11WasmTestZeroWidth(sample, 0) == -1);
	for (api = 1; api <= 8; api++) {
	    assert(X11WasmTestZeroWidth(sample, api)
		    == X11WasmTestZeroWidthFallback(sample));
	}
    }
    XFreeFontSet(display, fontSet);
    XFreeFont(display, font);
    XCloseDisplay(display);
#endif
    return 0;
}
