#include "XlibInt.h"

int
BrowserFontResourceTextWidth(
    X11WasmResource *resourcePtr,
    int fallbackCharWidth,
    const char *utf8,
    int length)
{
    int pixelSize = 14;
    const char *family = "sans-serif";
    int fallback;
    int measured;

    if ((utf8 == NULL) || (length <= 0)) {
	return 0;
    }
    fallback = length * ((fallbackCharWidth > 0) ? fallbackCharWidth : 8);
    if ((resourcePtr != NULL) && (resourcePtr->type == X11_WASM_RESOURCE_FONT)) {
	if (resourcePtr->fontPixelSize > 0) {
	    pixelSize = resourcePtr->fontPixelSize;
	}
	if (resourcePtr->fontFamily != NULL) {
	    family = resourcePtr->fontFamily;
	}
    }
    measured = X11WasmJsMeasureText(pixelSize, family, utf8, length);
    return (measured > 0) ? measured : fallback;
}

int
BrowserFontTextWidth(
    const XFontStruct *fontPtr,
    const char *utf8,
    int length)
{
    X11WasmResource *resourcePtr = NULL;

    if (fontPtr != NULL) {
	resourcePtr = X11WasmFindResource(fontPtr->fid);
    }
    return BrowserFontResourceTextWidth(resourcePtr, BrowserFontCharWidth(fontPtr),
	    utf8, length);
}

int
BrowserImageDataLength(
    XImage *image)
{
    if ((image == NULL) || (image->data == NULL)
	    || (image->bytes_per_line <= 0) || (image->height <= 0)) {
	return 0;
    }
    return image->bytes_per_line * image->height;
}

int
BrowserImageBitsPerPixel(
    unsigned int depth)
{
    return (depth <= 1U) ? 1 : ((depth <= 8U) ? 8
	    : ((depth <= 16U) ? 16 : 32));
}

int
BrowserImageFormatIsValid(
    int format)
{
    return (format == XYBitmap) || (format == XYPixmap) || (format == ZPixmap);
}

int
BrowserImageBytesPerLine(
    unsigned int width,
    int bitsPerPixel,
    int bitmapPad)
{
    unsigned int pad = (bitmapPad > 0) ? (unsigned int)bitmapPad : 32U;
    unsigned long long bits;
    unsigned long long bytes;

    if (bitsPerPixel <= 0) {
	return 0;
    }
    if (pad < 8U) {
	pad = 8U;
    }
    bits = (unsigned long long)width * (unsigned int)bitsPerPixel;
    bits = ((bits + pad - 1ULL) / pad) * pad;
    bytes = bits / 8ULL;
    return (bytes <= (unsigned long long)INT_MAX) ? (int)bytes : 0;
}

unsigned long
BrowserDepthPlaneMask(
    unsigned int depth)
{
    if (depth >= (sizeof(unsigned long) * CHAR_BIT)) {
	return ~0UL;
    }
    return (1UL << depth) - 1UL;
}

unsigned long
BrowserApplyRasterFunction(
    int function,
    unsigned long source,
    unsigned long dest,
    unsigned int depth)
{
    unsigned long mask = BrowserDepthPlaneMask(depth);

    source &= mask;
    dest &= mask;
    switch (function) {
    case GXclear:		return 0UL;
    case GXand:			return source & dest;
    case GXandReverse:		return source & ~dest;
    case GXcopy:		return source;
    case GXandInverted:		return ~source & dest;
    case GXnoop:		return dest;
    case GXxor:			return source ^ dest;
    case GXor:			return source | dest;
    case GXnor:			return ~(source | dest) & mask;
    case GXequiv:		return ~(source ^ dest) & mask;
    case GXinvert:		return ~dest & mask;
    case GXorReverse:		return source | ~dest;
    case GXcopyInverted:	return ~source & mask;
    case GXorInverted:		return ~source | dest;
    case GXnand:		return ~(source & dest) & mask;
    case GXset:			return mask;
    default:			return source;
    }
}

void
BrowserSetImageMasks(
    XImage *imagePtr)
{
    if (imagePtr == NULL) {
	return;
    }
    if (imagePtr->bits_per_pixel == 16) {
	imagePtr->red_mask = 0xf800;
	imagePtr->green_mask = 0x07e0;
	imagePtr->blue_mask = 0x001f;
    } else if (imagePtr->bits_per_pixel > 16) {
	imagePtr->red_mask = 0xff0000;
	imagePtr->green_mask = 0x00ff00;
	imagePtr->blue_mask = 0x0000ff;
    } else {
	imagePtr->red_mask = 0;
	imagePtr->green_mask = 0;
	imagePtr->blue_mask = 0;
    }
}

int
BrowserParseColor(
    const char *name,
    XColor *colorPtr)
{
    unsigned int red = 0, green = 0, blue = 0;
    char normalized[64];
    size_t normalizedLength = 0;
    size_t i;

    if ((name == NULL) || (colorPtr == NULL)) {
	return 0;
    }

    if (name[0] == '#') {
	size_t length = strlen(name + 1);
	size_t componentDigits = length / 3;

	if (((length % 3) != 0) || (componentDigits < 1)
		|| (componentDigits > 4)) {
	    return 0;
	}
	if (!BrowserParseHexComponent(name + 1, componentDigits, &red)
		|| !BrowserParseHexComponent(name + 1 + componentDigits,
			componentDigits, &green)
		|| !BrowserParseHexComponent(name + 1 + 2 * componentDigits,
			componentDigits, &blue)) {
	    return 0;
	}
    } else if (BrowserHasCasePrefix(name, "rgb:")) {
	if (!BrowserParseRgbHexSpec(name + 4, &red, &green, &blue)) {
	    return 0;
	}
    } else if (BrowserHasCasePrefix(name, "rgbi:")) {
	if (!BrowserParseRgbiSpec(name + 5, &red, &green, &blue)) {
	    return 0;
	}
    } else {
	for (i = 0; name[i] != '\0'; i++) {
	    unsigned char ch = (unsigned char)name[i];

	    if ((ch == ' ') || (ch == '_') || (ch == '-')) {
		continue;
	    }
	    if (normalizedLength + 1 >= sizeof(normalized)) {
		return 0;
	    }
	    normalized[normalizedLength++] = (char)tolower(ch);
	}
	normalized[normalizedLength] = '\0';
	for (i = 0; i < browserNamedColorCount;
		i++) {
	    if (strcmp(normalized, browserNamedColors[i].name) == 0) {
		red = browserNamedColors[i].red;
		green = browserNamedColors[i].green;
		blue = browserNamedColors[i].blue;
		break;
	    }
	}
	if (i == browserNamedColorCount) {
	    const char *suffix = NULL;

	    if (strncmp(normalized, "gray", 4) == 0) {
		suffix = normalized + 4;
	    } else if (strncmp(normalized, "grey", 4) == 0) {
		suffix = normalized + 4;
	    }
	    if ((suffix != NULL) && (*suffix != '\0')) {
		int percent = 0;
		int byte;

		for (i = 0; suffix[i] != '\0'; i++) {
		    if (!isdigit((unsigned char)suffix[i])) {
			return 0;
		    }
		    percent = percent * 10 + (suffix[i] - '0');
		    if (percent > 100) {
			return 0;
		    }
		}
		byte = percent * 255 / 100;
		red = green = blue = (unsigned int)byte * 257U;
	    } else {
		return 0;
	    }
	}
    }

    colorPtr->red = (unsigned short)red;
    colorPtr->green = (unsigned short)green;
    colorPtr->blue = (unsigned short)blue;
    colorPtr->flags = DoRed | DoGreen | DoBlue;
    colorPtr->pixel = ((unsigned long)(red >> 8) << 16)
	    | ((unsigned long)(green >> 8) << 8) | (unsigned long)(blue >> 8);
    colorPtr->pad = 0;
    return 1;
}

Atom
BrowserPredefinedAtom(
    const char *name)
{
    if (strcmp(name, "PRIMARY") == 0) return XA_PRIMARY;
    if (strcmp(name, "SECONDARY") == 0) return XA_SECONDARY;
    if (strcmp(name, "ARC") == 0) return XA_ARC;
    if (strcmp(name, "ATOM") == 0) return XA_ATOM;
    if (strcmp(name, "BITMAP") == 0) return XA_BITMAP;
    if (strcmp(name, "CARDINAL") == 0) return XA_CARDINAL;
    if (strcmp(name, "COLORMAP") == 0) return XA_COLORMAP;
    if (strcmp(name, "CURSOR") == 0) return XA_CURSOR;
    if (strcmp(name, "CUT_BUFFER0") == 0) return XA_CUT_BUFFER0;
    if (strcmp(name, "CUT_BUFFER1") == 0) return XA_CUT_BUFFER1;
    if (strcmp(name, "CUT_BUFFER2") == 0) return XA_CUT_BUFFER2;
    if (strcmp(name, "CUT_BUFFER3") == 0) return XA_CUT_BUFFER3;
    if (strcmp(name, "CUT_BUFFER4") == 0) return XA_CUT_BUFFER4;
    if (strcmp(name, "CUT_BUFFER5") == 0) return XA_CUT_BUFFER5;
    if (strcmp(name, "CUT_BUFFER6") == 0) return XA_CUT_BUFFER6;
    if (strcmp(name, "CUT_BUFFER7") == 0) return XA_CUT_BUFFER7;
    if (strcmp(name, "DRAWABLE") == 0) return XA_DRAWABLE;
    if (strcmp(name, "FONT") == 0) return XA_FONT;
    if (strcmp(name, "INTEGER") == 0) return XA_INTEGER;
    if (strcmp(name, "PIXMAP") == 0) return XA_PIXMAP;
    if (strcmp(name, "POINT") == 0) return XA_POINT;
    if (strcmp(name, "RECTANGLE") == 0) return XA_RECTANGLE;
    if (strcmp(name, "RESOURCE_MANAGER") == 0) return XA_RESOURCE_MANAGER;
    if (strcmp(name, "RGB_COLOR_MAP") == 0) return XA_RGB_COLOR_MAP;
    if (strcmp(name, "RGB_BEST_MAP") == 0) return XA_RGB_BEST_MAP;
    if (strcmp(name, "RGB_BLUE_MAP") == 0) return XA_RGB_BLUE_MAP;
    if (strcmp(name, "RGB_DEFAULT_MAP") == 0) return XA_RGB_DEFAULT_MAP;
    if (strcmp(name, "RGB_GRAY_MAP") == 0) return XA_RGB_GRAY_MAP;
    if (strcmp(name, "RGB_GREEN_MAP") == 0) return XA_RGB_GREEN_MAP;
    if (strcmp(name, "RGB_RED_MAP") == 0) return XA_RGB_RED_MAP;
    if (strcmp(name, "STRING") == 0) return XA_STRING;
    if (strcmp(name, "VISUALID") == 0) return XA_VISUALID;
    if (strcmp(name, "WINDOW") == 0) return XA_WINDOW;
    if (strcmp(name, "WM_COMMAND") == 0) return XA_WM_COMMAND;
    if (strcmp(name, "WM_HINTS") == 0) return XA_WM_HINTS;
    if (strcmp(name, "WM_CLIENT_MACHINE") == 0) return XA_WM_CLIENT_MACHINE;
    if (strcmp(name, "WM_ICON_NAME") == 0) return XA_WM_ICON_NAME;
    if (strcmp(name, "WM_ICON_SIZE") == 0) return XA_WM_ICON_SIZE;
    if (strcmp(name, "WM_NAME") == 0) return XA_WM_NAME;
    if (strcmp(name, "WM_NORMAL_HINTS") == 0) return XA_WM_NORMAL_HINTS;
    if (strcmp(name, "WM_SIZE_HINTS") == 0) return XA_WM_SIZE_HINTS;
    if (strcmp(name, "WM_ZOOM_HINTS") == 0) return XA_WM_ZOOM_HINTS;
    if (strcmp(name, "MIN_SPACE") == 0) return XA_MIN_SPACE;
    if (strcmp(name, "NORM_SPACE") == 0) return XA_NORM_SPACE;
    if (strcmp(name, "MAX_SPACE") == 0) return XA_MAX_SPACE;
    if (strcmp(name, "END_SPACE") == 0) return XA_END_SPACE;
    if (strcmp(name, "SUPERSCRIPT_X") == 0) return XA_SUPERSCRIPT_X;
    if (strcmp(name, "SUPERSCRIPT_Y") == 0) return XA_SUPERSCRIPT_Y;
    if (strcmp(name, "SUBSCRIPT_X") == 0) return XA_SUBSCRIPT_X;
    if (strcmp(name, "SUBSCRIPT_Y") == 0) return XA_SUBSCRIPT_Y;
    if (strcmp(name, "UNDERLINE_POSITION") == 0) return XA_UNDERLINE_POSITION;
    if (strcmp(name, "UNDERLINE_THICKNESS") == 0) return XA_UNDERLINE_THICKNESS;
    if (strcmp(name, "STRIKEOUT_ASCENT") == 0) return XA_STRIKEOUT_ASCENT;
    if (strcmp(name, "STRIKEOUT_DESCENT") == 0) return XA_STRIKEOUT_DESCENT;
    if (strcmp(name, "ITALIC_ANGLE") == 0) return XA_ITALIC_ANGLE;
    if (strcmp(name, "X_HEIGHT") == 0) return XA_X_HEIGHT;
    if (strcmp(name, "QUAD_WIDTH") == 0) return XA_QUAD_WIDTH;
    if (strcmp(name, "WEIGHT") == 0) return XA_WEIGHT;
    if (strcmp(name, "POINT_SIZE") == 0) return XA_POINT_SIZE;
    if (strcmp(name, "RESOLUTION") == 0) return XA_RESOLUTION;
    if (strcmp(name, "COPYRIGHT") == 0) return XA_COPYRIGHT;
    if (strcmp(name, "NOTICE") == 0) return XA_NOTICE;
    if (strcmp(name, "FONT_NAME") == 0) return XA_FONT_NAME;
    if (strcmp(name, "FAMILY_NAME") == 0) return XA_FAMILY_NAME;
    if (strcmp(name, "FULL_NAME") == 0) return XA_FULL_NAME;
    if (strcmp(name, "CAP_HEIGHT") == 0) return XA_CAP_HEIGHT;
    if (strcmp(name, "WM_CLASS") == 0) return XA_WM_CLASS;
    if (strcmp(name, "WM_TRANSIENT_FOR") == 0) return XA_WM_TRANSIENT_FOR;
    return None;
}

const char *
BrowserPredefinedAtomName(
    Atom atom)
{
    switch (atom) {
    case XA_PRIMARY: return "PRIMARY";
    case XA_SECONDARY: return "SECONDARY";
    case XA_ARC: return "ARC";
    case XA_ATOM: return "ATOM";
    case XA_BITMAP: return "BITMAP";
    case XA_CARDINAL: return "CARDINAL";
    case XA_COLORMAP: return "COLORMAP";
    case XA_CURSOR: return "CURSOR";
    case XA_CUT_BUFFER0: return "CUT_BUFFER0";
    case XA_CUT_BUFFER1: return "CUT_BUFFER1";
    case XA_CUT_BUFFER2: return "CUT_BUFFER2";
    case XA_CUT_BUFFER3: return "CUT_BUFFER3";
    case XA_CUT_BUFFER4: return "CUT_BUFFER4";
    case XA_CUT_BUFFER5: return "CUT_BUFFER5";
    case XA_CUT_BUFFER6: return "CUT_BUFFER6";
    case XA_CUT_BUFFER7: return "CUT_BUFFER7";
    case XA_DRAWABLE: return "DRAWABLE";
    case XA_FONT: return "FONT";
    case XA_INTEGER: return "INTEGER";
    case XA_PIXMAP: return "PIXMAP";
    case XA_POINT: return "POINT";
    case XA_RECTANGLE: return "RECTANGLE";
    case XA_RESOURCE_MANAGER: return "RESOURCE_MANAGER";
    case XA_RGB_COLOR_MAP: return "RGB_COLOR_MAP";
    case XA_RGB_BEST_MAP: return "RGB_BEST_MAP";
    case XA_RGB_BLUE_MAP: return "RGB_BLUE_MAP";
    case XA_RGB_DEFAULT_MAP: return "RGB_DEFAULT_MAP";
    case XA_RGB_GRAY_MAP: return "RGB_GRAY_MAP";
    case XA_RGB_GREEN_MAP: return "RGB_GREEN_MAP";
    case XA_RGB_RED_MAP: return "RGB_RED_MAP";
    case XA_STRING: return "STRING";
    case XA_VISUALID: return "VISUALID";
    case XA_WINDOW: return "WINDOW";
    case XA_WM_COMMAND: return "WM_COMMAND";
    case XA_WM_HINTS: return "WM_HINTS";
    case XA_WM_CLIENT_MACHINE: return "WM_CLIENT_MACHINE";
    case XA_WM_ICON_NAME: return "WM_ICON_NAME";
    case XA_WM_ICON_SIZE: return "WM_ICON_SIZE";
    case XA_WM_NAME: return "WM_NAME";
    case XA_WM_NORMAL_HINTS: return "WM_NORMAL_HINTS";
    case XA_WM_SIZE_HINTS: return "WM_SIZE_HINTS";
    case XA_WM_ZOOM_HINTS: return "WM_ZOOM_HINTS";
    case XA_MIN_SPACE: return "MIN_SPACE";
    case XA_NORM_SPACE: return "NORM_SPACE";
    case XA_MAX_SPACE: return "MAX_SPACE";
    case XA_END_SPACE: return "END_SPACE";
    case XA_SUPERSCRIPT_X: return "SUPERSCRIPT_X";
    case XA_SUPERSCRIPT_Y: return "SUPERSCRIPT_Y";
    case XA_SUBSCRIPT_X: return "SUBSCRIPT_X";
    case XA_SUBSCRIPT_Y: return "SUBSCRIPT_Y";
    case XA_UNDERLINE_POSITION: return "UNDERLINE_POSITION";
    case XA_UNDERLINE_THICKNESS: return "UNDERLINE_THICKNESS";
    case XA_STRIKEOUT_ASCENT: return "STRIKEOUT_ASCENT";
    case XA_STRIKEOUT_DESCENT: return "STRIKEOUT_DESCENT";
    case XA_ITALIC_ANGLE: return "ITALIC_ANGLE";
    case XA_X_HEIGHT: return "X_HEIGHT";
    case XA_QUAD_WIDTH: return "QUAD_WIDTH";
    case XA_WEIGHT: return "WEIGHT";
    case XA_POINT_SIZE: return "POINT_SIZE";
    case XA_RESOLUTION: return "RESOLUTION";
    case XA_COPYRIGHT: return "COPYRIGHT";
    case XA_NOTICE: return "NOTICE";
    case XA_FONT_NAME: return "FONT_NAME";
    case XA_FAMILY_NAME: return "FAMILY_NAME";
    case XA_FULL_NAME: return "FULL_NAME";
    case XA_CAP_HEIGHT: return "CAP_HEIGHT";
    case XA_WM_CLASS: return "WM_CLASS";
    case XA_WM_TRANSIENT_FOR: return "WM_TRANSIENT_FOR";
    default: return NULL;
    }
}

void
BrowserInitImageFuncs(
    XImage *image)
{
    image->f.create_image = XCreateImage;
    image->f.destroy_image = BrowserImageDestroy;
    image->f.get_pixel = BrowserImageGetPixel;
    image->f.put_pixel = BrowserImagePutPixel;
    image->f.sub_image = BrowserImageSubImage;
    image->f.add_pixel = BrowserImageAddPixel;
}

int
BrowserImageDestroy(
    XImage *image)
{
    if (image == NULL) {
	return Success;
    }
    if ((image->obdata == (XPointer)image) && (image->data != NULL)) {
	ckfree(image->data);
    }
    ckfree(image);
    return Success;
}

unsigned long
BrowserImageGetPixel(
    XImage *image,
    int x,
    int y)
{
    int bytesPerPixel;
    int pixelX;
    unsigned char *pixelPtr;
    unsigned long pixel = 0;
    int i;

    if ((image == NULL) || (image->data == NULL)
	    || (x < 0) || (y < 0) || (x >= image->width)
	    || (y >= image->height)) {
	return 0;
    }
    pixelX = image->xoffset + x;
    if (pixelX < 0) {
	return 0;
    }
    if (image->bits_per_pixel == 1) {
	unsigned int bitOffset = (unsigned int)y
		* (unsigned int)image->bytes_per_line * 8U
		+ (unsigned int)pixelX;
	unsigned char byte = ((unsigned char *)image->data)[bitOffset >> 3];
	unsigned char mask = (image->bitmap_bit_order == MSBFirst)
		? (unsigned char)(0x80U >> (bitOffset & 7U))
		: (unsigned char)(1U << (bitOffset & 7U));

	return (byte & mask) ? 1U : 0U;
    }
    bytesPerPixel = image->bits_per_pixel / 8;
    if (bytesPerPixel <= 0) {
	return 0;
    }
    pixelPtr = (unsigned char *)image->data + y * image->bytes_per_line
	    + pixelX * bytesPerPixel;
    if (image->byte_order == MSBFirst) {
	for (i = 0; i < bytesPerPixel; i++) {
	    pixel = (pixel << 8) | pixelPtr[i];
	}
    } else {
	for (i = bytesPerPixel - 1; i >= 0; i--) {
	    pixel = (pixel << 8) | pixelPtr[i];
	}
    }
    return pixel;
}

int
BrowserImagePutPixel(
    XImage *image,
    int x,
    int y,
    unsigned long pixel)
{
    int bytesPerPixel;
    int pixelX;
    unsigned char *pixelPtr;
    int i;

    if ((image == NULL) || (image->data == NULL)
	    || (x < 0) || (y < 0) || (x >= image->width)
	    || (y >= image->height)) {
	return 0;
    }
    pixelX = image->xoffset + x;
    if (pixelX < 0) {
	return 0;
    }
    if (image->bits_per_pixel == 1) {
	unsigned int bitOffset = (unsigned int)y
		* (unsigned int)image->bytes_per_line * 8U
		+ (unsigned int)pixelX;
	unsigned char *bytePtr = (unsigned char *)image->data + (bitOffset >> 3);
	unsigned char mask = (image->bitmap_bit_order == MSBFirst)
		? (unsigned char)(0x80U >> (bitOffset & 7U))
		: (unsigned char)(1U << (bitOffset & 7U));

	if (pixel & 1UL) {
	    *bytePtr |= mask;
	} else {
	    *bytePtr &= (unsigned char)~mask;
	}
	return Success;
    }
    bytesPerPixel = image->bits_per_pixel / 8;
    if (bytesPerPixel <= 0) {
	return 0;
    }
    pixelPtr = (unsigned char *)image->data + y * image->bytes_per_line
	    + pixelX * bytesPerPixel;
    if (image->byte_order == MSBFirst) {
	for (i = 0; i < bytesPerPixel; i++) {
	    pixelPtr[i] = (unsigned char)((pixel
		    >> ((bytesPerPixel - i - 1) * 8)) & 0xff);
	}
    } else {
	for (i = 0; i < bytesPerPixel; i++) {
	    pixelPtr[i] = (unsigned char)((pixel >> (i * 8)) & 0xff);
	}
    }
    return Success;
}

void
BrowserImageApplyPlaneMask(
    XImage *image,
    unsigned long planeMask)
{
    int x, y;

    if ((image == NULL) || (image->data == NULL) || (planeMask == AllPlanes)) {
	return;
    }
    if (image->bits_per_pixel == 1) {
	if ((planeMask & 1UL) == 0) {
	    memset(image->data, 0,
		    (size_t)image->bytes_per_line * (size_t)image->height);
	}
	return;
    }
    for (y = 0; y < image->height; y++) {
	for (x = 0; x < image->width; x++) {
	    unsigned long pixel = BrowserImageGetPixel(image, x, y);

	    (void)BrowserImagePutPixel(image, x, y, pixel & planeMask);
	}
    }
}

int
BrowserBitmapBytesPerLine(
    unsigned int width)
{
    return (int)((width + 7U) / 8U);
}

void
BrowserFreePixmapBacking(
    X11WasmResource *resourcePtr)
{
    if ((resourcePtr != NULL) && (resourcePtr->backingData != NULL)) {
	ckfree(resourcePtr->backingData);
	resourcePtr->backingData = NULL;
	resourcePtr->backingBytesPerLine = 0;
	resourcePtr->backingBitsPerPixel = 0;
    }
}

int
BrowserEnsurePixmapBacking(
    X11WasmResource *resourcePtr)
{
    int bitsPerPixel;
    int bytesPerLine;
    unsigned long byteCount;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_PIXMAP)
	    || (resourcePtr->width == 0) || (resourcePtr->height == 0)) {
	return 0;
    }
    bitsPerPixel = BrowserImageBitsPerPixel(resourcePtr->depth);
    bytesPerLine = BrowserImageBytesPerLine(resourcePtr->width, bitsPerPixel,
	    32);
    byteCount = (unsigned long)bytesPerLine * (unsigned long)resourcePtr->height;
    if ((bytesPerLine <= 0) || (byteCount > (unsigned long)INT_MAX)) {
	return 0;
    }
    if ((resourcePtr->backingData != NULL)
	    && (resourcePtr->backingBytesPerLine == bytesPerLine)
	    && (resourcePtr->backingBitsPerPixel == bitsPerPixel)) {
	return 1;
    }
    BrowserFreePixmapBacking(resourcePtr);
    resourcePtr->backingData = (unsigned char *)ckalloc((unsigned int)byteCount);
    memset(resourcePtr->backingData, 0, (unsigned int)byteCount);
    resourcePtr->backingBytesPerLine = bytesPerLine;
    resourcePtr->backingBitsPerPixel = bitsPerPixel;
    return 1;
}

int
BrowserInitPixmapBackingImage(
    X11WasmResource *resourcePtr,
    XImage *imagePtr)
{
    if ((resourcePtr == NULL) || (imagePtr == NULL)
	    || (resourcePtr->backingData == NULL)
	    || (resourcePtr->backingBytesPerLine <= 0)
	    || (resourcePtr->backingBitsPerPixel <= 0)) {
	return 0;
    }
    memset(imagePtr, 0, sizeof(XImage));
    imagePtr->width = (int)resourcePtr->width;
    imagePtr->height = (int)resourcePtr->height;
    imagePtr->format = ZPixmap;
    imagePtr->data = (char *)resourcePtr->backingData;
    imagePtr->byte_order = LSBFirst;
    imagePtr->bitmap_unit = 32;
    imagePtr->bitmap_bit_order = LSBFirst;
    imagePtr->bitmap_pad = 32;
    imagePtr->depth = (int)resourcePtr->depth;
    imagePtr->bits_per_pixel = resourcePtr->backingBitsPerPixel;
    imagePtr->bytes_per_line = resourcePtr->backingBytesPerLine;
    BrowserSetImageMasks(imagePtr);
    BrowserInitImageFuncs(imagePtr);
    return 1;
}

int
BrowserReadPixmapBacking(
    X11WasmResource *resourcePtr,
    XImage *destImage,
    int srcX,
    int srcY,
    unsigned int width,
    unsigned int height)
{
    XImage backingImage;
    unsigned int ix, iy;

    if ((destImage == NULL) || (resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_PIXMAP)
	    || (resourcePtr->backingData == NULL)
	    || !BrowserInitPixmapBackingImage(resourcePtr, &backingImage)) {
	return 0;
    }
    for (iy = 0; iy < height; iy++) {
	for (ix = 0; ix < width; ix++) {
	    (void)BrowserImagePutPixel(destImage, (int)ix, (int)iy,
		    BrowserImageGetPixel(&backingImage, srcX + (int)ix,
			    srcY + (int)iy));
	}
    }
    return 1;
}

void
BrowserStorePixmapImage(
    X11WasmResource *resourcePtr,
    GC gc,
    XImage *image,
    int srcX,
    int srcY,
    int destX,
    int destY,
    unsigned int width,
    unsigned int height)
{
    XImage backingImage;
    unsigned int ix, iy;
    int function = (gc != NULL) ? gc->function : GXcopy;

    if ((image == NULL) || !BrowserEnsurePixmapBacking(resourcePtr)
	    || !BrowserInitPixmapBackingImage(resourcePtr, &backingImage)) {
	return;
    }
    for (iy = 0; iy < height; iy++) {
	for (ix = 0; ix < width; ix++) {
	    int targetX = destX + (int)ix;
	    int targetY = destY + (int)iy;
	    unsigned long source;
	    unsigned long dest;

	    if ((targetX < 0) || (targetY < 0)
		    || ((unsigned int)targetX >= resourcePtr->width)
		    || ((unsigned int)targetY >= resourcePtr->height)) {
		continue;
	    }
	    source = BrowserImageGetPixel(image, srcX + (int)ix,
		    srcY + (int)iy);
	    dest = BrowserImageGetPixel(&backingImage, targetX, targetY);
	    (void)BrowserImagePutPixel(&backingImage, targetX, targetY,
		    BrowserApplyRasterFunction(function, source, dest,
			    resourcePtr->depth));
	}
    }
}

int
BrowserEnsureBitmapStorage(
    X11WasmResource *resourcePtr)
{
    unsigned long byteCount;
    int bytesPerLine;

    if ((resourcePtr == NULL) || (resourcePtr->depth != 1)
	    || (resourcePtr->width == 0) || (resourcePtr->height == 0)) {
	return 0;
    }
    bytesPerLine = BrowserBitmapBytesPerLine(resourcePtr->width);
    byteCount = (unsigned long)bytesPerLine * (unsigned long)resourcePtr->height;
    if (byteCount > (unsigned long)0x7fffffff) {
	return 0;
    }
    if ((resourcePtr->bitmapData != NULL)
	    && (resourcePtr->bitmapBytesPerLine == bytesPerLine)) {
	return 1;
    }
    if (resourcePtr->bitmapData != NULL) {
	ckfree(resourcePtr->bitmapData);
    }
    resourcePtr->bitmapData = (unsigned char *)ckalloc((unsigned int)byteCount);
    memset(resourcePtr->bitmapData, 0, (unsigned int)byteCount);
    resourcePtr->bitmapBytesPerLine = bytesPerLine;
    return 1;
}

int
BrowserBitmapGetPixel(
    X11WasmResource *resourcePtr,
    unsigned int x,
    unsigned int y)
{
    unsigned int bitOffset;

    if ((resourcePtr == NULL) || (resourcePtr->bitmapData == NULL)
	    || (x >= resourcePtr->width) || (y >= resourcePtr->height)) {
	return 0;
    }
    bitOffset = y * (unsigned int)resourcePtr->bitmapBytesPerLine * 8U + x;
    return (resourcePtr->bitmapData[bitOffset >> 3] >> (bitOffset & 7U)) & 1U;
}

void
BrowserBitmapPutPixel(
    X11WasmResource *resourcePtr,
    unsigned int x,
    unsigned int y,
    int bit)
{
    unsigned int bitOffset;
    unsigned char *bytePtr;
    unsigned char mask;

    if ((resourcePtr == NULL) || (resourcePtr->bitmapData == NULL)
	    || (x >= resourcePtr->width) || (y >= resourcePtr->height)) {
	return;
    }
    bitOffset = y * (unsigned int)resourcePtr->bitmapBytesPerLine * 8U + x;
    bytePtr = resourcePtr->bitmapData + (bitOffset >> 3);
    mask = (unsigned char)(1U << (bitOffset & 7U));
    if (bit) {
	*bytePtr |= mask;
    } else {
	*bytePtr &= (unsigned char)~mask;
    }
}

void
BrowserStoreBitmapImage(
    X11WasmResource *resourcePtr,
    XImage *image,
    int srcX,
    int srcY,
    int destX,
    int destY,
    unsigned int width,
    unsigned int height)
{
    unsigned int ix, iy;

    if ((resourcePtr == NULL) || (image == NULL)
	    || (resourcePtr->depth != 1) || (image->bits_per_pixel != 1)
	    || !BrowserEnsureBitmapStorage(resourcePtr)) {
	return;
    }
    for (iy = 0; iy < height; iy++) {
	for (ix = 0; ix < width; ix++) {
	    int targetX = destX + (int)ix;
	    int targetY = destY + (int)iy;

	    if ((targetX < 0) || (targetY < 0)
		    || ((unsigned int)targetX >= resourcePtr->width)
		    || ((unsigned int)targetY >= resourcePtr->height)) {
		continue;
	    }
	    BrowserBitmapPutPixel(resourcePtr, (unsigned int)targetX,
		    (unsigned int)targetY,
		    (int)BrowserImageGetPixel(image, srcX + (int)ix,
			    srcY + (int)iy));
	}
    }
}

XImage *
BrowserImageSubImage(
    XImage *image,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    XImage *subImage;
    unsigned int ix, iy;

    subImage = XCreateImage(NULL, NULL, (unsigned int)image->depth,
	    image->format, 0, NULL, width, height, image->bitmap_pad, 0);
    if (subImage == NULL) {
	return NULL;
    }
    subImage->data = (char *)ckalloc(subImage->bytes_per_line * height);
    memset(subImage->data, 0, subImage->bytes_per_line * height);
    subImage->obdata = (XPointer)subImage;
    for (iy = 0; iy < height; iy++) {
	for (ix = 0; ix < width; ix++) {
	    BrowserImagePutPixel(subImage, (int)ix, (int)iy,
		    BrowserImageGetPixel(image, x + (int)ix, y + (int)iy));
	}
    }
    return subImage;
}

int
BrowserImageAddPixel(
    XImage *image,
    long value)
{
    int x, y;

    if ((image == NULL) || (image->data == NULL)) {
	return Success;
    }
    for (y = 0; y < image->height; y++) {
	for (x = 0; x < image->width; x++) {
	    unsigned long pixel = BrowserImageGetPixel(image, x, y);

	    (void)BrowserImagePutPixel(image, x, y,
		    pixel + (unsigned long)value);
	}
    }
    return Success;
}

void
BrowserInitGC(
    Display *display,
    GC gc)
{
    memset(gc, 0, sizeof(XGCValues));
    gc->function = GXcopy;
    gc->plane_mask = AllPlanes;
    gc->foreground = BlackPixel(display, DefaultScreen(display));
    gc->background = WhitePixel(display, DefaultScreen(display));
    gc->line_width = 1;
    gc->line_style = LineSolid;
    gc->fill_style = FillSolid;
    gc->fill_rule = EvenOddRule;
    gc->arc_mode = ArcPieSlice;
    gc->subwindow_mode = ClipByChildren;
    gc->graphics_exposures = True;
    gc->dashes = 4;
}

void
BrowserApplyGCValues(
    GC gc,
    unsigned long mask,
    XGCValues *values)
{
    if (values == NULL) {
	return;
    }
#define COPY_GC_FIELD(bit, field) \
    if (mask & (bit)) { gc->field = values->field; }
    COPY_GC_FIELD(GCFunction, function);
    COPY_GC_FIELD(GCPlaneMask, plane_mask);
    COPY_GC_FIELD(GCForeground, foreground);
    COPY_GC_FIELD(GCBackground, background);
    COPY_GC_FIELD(GCLineWidth, line_width);
    COPY_GC_FIELD(GCLineStyle, line_style);
    COPY_GC_FIELD(GCCapStyle, cap_style);
    COPY_GC_FIELD(GCJoinStyle, join_style);
    COPY_GC_FIELD(GCFillStyle, fill_style);
    COPY_GC_FIELD(GCFillRule, fill_rule);
    COPY_GC_FIELD(GCTile, tile);
    COPY_GC_FIELD(GCStipple, stipple);
    COPY_GC_FIELD(GCTileStipXOrigin, ts_x_origin);
    COPY_GC_FIELD(GCTileStipYOrigin, ts_y_origin);
    COPY_GC_FIELD(GCFont, font);
    COPY_GC_FIELD(GCSubwindowMode, subwindow_mode);
    COPY_GC_FIELD(GCGraphicsExposures, graphics_exposures);
    COPY_GC_FIELD(GCClipXOrigin, clip_x_origin);
    COPY_GC_FIELD(GCClipYOrigin, clip_y_origin);
    COPY_GC_FIELD(GCClipMask, clip_mask);
    COPY_GC_FIELD(GCDashOffset, dash_offset);
    COPY_GC_FIELD(GCDashList, dashes);
    COPY_GC_FIELD(GCArcMode, arc_mode);
#undef COPY_GC_FIELD
    if (mask & GCClipMask) {
	BrowserSetGCClip(gc, 0, NULL);
    }
    if (mask & GCDashList) {
	BrowserSetGCDashes(gc, (const char *)&gc->dashes, 1);
    }
}

int
BrowserGCGetValueMaskIsValid(
    unsigned long mask)
{
    unsigned long allowedMask = GCFunction|GCPlaneMask|GCForeground
	    |GCBackground|GCLineWidth|GCLineStyle|GCCapStyle|GCJoinStyle
	    |GCFillStyle|GCFillRule|GCTile|GCStipple|GCTileStipXOrigin
	    |GCTileStipYOrigin|GCFont|GCSubwindowMode|GCGraphicsExposures
	    |GCClipXOrigin|GCClipYOrigin|GCDashOffset|GCArcMode;

    return (mask & ~allowedMask) == 0;
}

int
BrowserGCChangeValueMaskIsValid(
    unsigned long mask)
{
    unsigned long allowedMask = GCFunction|GCPlaneMask|GCForeground
	    |GCBackground|GCLineWidth|GCLineStyle|GCCapStyle|GCJoinStyle
	    |GCFillStyle|GCFillRule|GCTile|GCStipple|GCTileStipXOrigin
	    |GCTileStipYOrigin|GCFont|GCSubwindowMode|GCGraphicsExposures
	    |GCClipXOrigin|GCClipYOrigin|GCClipMask|GCDashOffset
	    |GCDashList|GCArcMode;

    return (mask & ~allowedMask) == 0;
}

void
BrowserCopyGCValueFields(
    GC src,
    XGCValues *dest,
    unsigned long mask)
{
#define COPY_GC_COMPONENT(bit, field) \
    if (mask & (bit)) { dest->field = src->field; }
    COPY_GC_COMPONENT(GCFunction, function);
    COPY_GC_COMPONENT(GCPlaneMask, plane_mask);
    COPY_GC_COMPONENT(GCForeground, foreground);
    COPY_GC_COMPONENT(GCBackground, background);
    COPY_GC_COMPONENT(GCLineWidth, line_width);
    COPY_GC_COMPONENT(GCLineStyle, line_style);
    COPY_GC_COMPONENT(GCCapStyle, cap_style);
    COPY_GC_COMPONENT(GCJoinStyle, join_style);
    COPY_GC_COMPONENT(GCFillStyle, fill_style);
    COPY_GC_COMPONENT(GCFillRule, fill_rule);
    COPY_GC_COMPONENT(GCTile, tile);
    COPY_GC_COMPONENT(GCStipple, stipple);
    COPY_GC_COMPONENT(GCTileStipXOrigin, ts_x_origin);
    COPY_GC_COMPONENT(GCTileStipYOrigin, ts_y_origin);
    COPY_GC_COMPONENT(GCFont, font);
    COPY_GC_COMPONENT(GCSubwindowMode, subwindow_mode);
    COPY_GC_COMPONENT(GCGraphicsExposures, graphics_exposures);
    COPY_GC_COMPONENT(GCClipXOrigin, clip_x_origin);
    COPY_GC_COMPONENT(GCClipYOrigin, clip_y_origin);
    COPY_GC_COMPONENT(GCClipMask, clip_mask);
    COPY_GC_COMPONENT(GCDashOffset, dash_offset);
    COPY_GC_COMPONENT(GCDashList, dashes);
    COPY_GC_COMPONENT(GCArcMode, arc_mode);
#undef COPY_GC_COMPONENT
}

void
BrowserCopyGCComponents(
    GC src,
    GC dest,
    unsigned long mask)
{
    if ((src == NULL) || (dest == NULL)) {
	return;
    }
    BrowserCopyGCValueFields(src, dest, mask);

    if (mask & GCClipMask) {
	X11WasmGCClip *clipPtr = BrowserFindGCClip(src, 0);

	if ((clipPtr != NULL) && clipPtr->enabled) {
	    BrowserSetGCClipRects(dest, 1, clipPtr->rects, clipPtr->rectCount);
	} else {
	    BrowserSetGCClip(dest, 0, NULL);
	}
    }
    if (mask & GCDashList) {
	const unsigned char *dashPattern = NULL;
	int dashCount = 0;

	if (BrowserGetGCDashes(src, &dashPattern, &dashCount)) {
	    BrowserSetGCDashes(dest, (const char *)dashPattern, dashCount);
	} else {
	    BrowserSetGCDashes(dest, (const char *)&dest->dashes, 1);
	}
    }
}
