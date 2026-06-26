#include "XlibInt.h"

static int
BrowserBitmapDataLength(
    unsigned int width,
    unsigned int height,
    size_t *lengthPtr)
{
    int bytesPerLine;
    unsigned long long length;

    if ((width == 0U) || (height == 0U)) {
	return BitmapFileInvalid;
    }
    bytesPerLine = BrowserBitmapBytesPerLine(width);
    length = (unsigned long long)bytesPerLine * (unsigned long long)height;
    if ((bytesPerLine <= 0) || (length > (unsigned long long)INT_MAX)) {
	return BitmapNoMemory;
    }
    *lengthPtr = (size_t)length;
    return BitmapSuccess;
}

static int
BrowserReadFileToString(
    const char *filename,
    char **contentsPtr)
{
    FILE *file;
    char *contents;
    size_t capacity = 4096U;
    size_t length = 0U;
    int ch;

    *contentsPtr = NULL;
    if (filename == NULL) {
	return BitmapOpenFailed;
    }
    file = fopen(filename, "rb");
    if (file == NULL) {
	return BitmapOpenFailed;
    }
    contents = (char *)ckalloc(capacity + 1U);
    while ((ch = fgetc(file)) != EOF) {
	if (length == capacity) {
	    char *expanded;

	    if (capacity > ((size_t)INT_MAX / 2U)) {
		ckfree(contents);
		(void)fclose(file);
		return BitmapNoMemory;
	    }
	    capacity *= 2U;
	    expanded = (char *)ckalloc(capacity + 1U);
	    memcpy(expanded, contents, length);
	    ckfree(contents);
	    contents = expanded;
	}
	contents[length++] = (char)ch;
    }
    if (ferror(file)) {
	ckfree(contents);
	(void)fclose(file);
	return BitmapOpenFailed;
    }
    (void)fclose(file);
    contents[length] = '\0';
    *contentsPtr = contents;
    return BitmapSuccess;
}

static int
BrowserBitmapNameHasSuffix(
    const char *name,
    size_t nameLength,
    const char *suffix)
{
    size_t suffixLength = strlen(suffix);

    return (nameLength >= suffixLength)
	    && (memcmp(name + nameLength - suffixLength, suffix,
		    suffixLength) == 0);
}

static void
BrowserParseBitmapDefineLine(
    const char *line,
    const char *end,
    unsigned int *widthPtr,
    unsigned int *heightPtr,
    int *xHotPtr,
    int *yHotPtr)
{
    const char *cursor = line;
    const char *nameStart;
    const char *nameEnd;
    char *valueEnd;
    long value;

    while ((cursor < end) && isspace(UCHAR(*cursor))) {
	cursor++;
    }
    if ((cursor >= end) || (*cursor != '#')) {
	return;
    }
    cursor++;
    while ((cursor < end) && isspace(UCHAR(*cursor))) {
	cursor++;
    }
    if (((size_t)(end - cursor) < 6U)
	    || (memcmp(cursor, "define", 6U) != 0)
	    || ((cursor + 6 < end) && !isspace(UCHAR(cursor[6])))) {
	return;
    }
    cursor += 6;
    while ((cursor < end) && isspace(UCHAR(*cursor))) {
	cursor++;
    }
    nameStart = cursor;
    while ((cursor < end) && !isspace(UCHAR(*cursor))) {
	cursor++;
    }
    nameEnd = cursor;
    while ((cursor < end) && isspace(UCHAR(*cursor))) {
	cursor++;
    }
    if ((nameStart == nameEnd) || (cursor >= end)) {
	return;
    }
    value = strtol(cursor, &valueEnd, 0);
    if (valueEnd == cursor) {
	return;
    }
    if (BrowserBitmapNameHasSuffix(nameStart, (size_t)(nameEnd - nameStart),
	    "_width")) {
	if ((value > 0) && ((unsigned long)value <= UINT_MAX)) {
	    *widthPtr = (unsigned int)value;
	}
    } else if (BrowserBitmapNameHasSuffix(nameStart,
	    (size_t)(nameEnd - nameStart), "_height")) {
	if ((value > 0) && ((unsigned long)value <= UINT_MAX)) {
	    *heightPtr = (unsigned int)value;
	}
    } else if (BrowserBitmapNameHasSuffix(nameStart,
	    (size_t)(nameEnd - nameStart), "_x_hot")) {
	*xHotPtr = (int)value;
    } else if (BrowserBitmapNameHasSuffix(nameStart,
	    (size_t)(nameEnd - nameStart), "_y_hot")) {
	*yHotPtr = (int)value;
    }
}

static void
BrowserParseBitmapDefines(
    const char *contents,
    unsigned int *widthPtr,
    unsigned int *heightPtr,
    int *xHotPtr,
    int *yHotPtr)
{
    const char *line = contents;

    *widthPtr = 0U;
    *heightPtr = 0U;
    *xHotPtr = -1;
    *yHotPtr = -1;
    while (*line != '\0') {
	const char *end = strchr(line, '\n');

	if (end == NULL) {
	    end = line + strlen(line);
	}
	BrowserParseBitmapDefineLine(line, end, widthPtr, heightPtr, xHotPtr,
		yHotPtr);
	line = (*end == '\n') ? end + 1 : end;
    }
}

static int
BrowserSegmentContainsWord(
    const char *start,
    const char *end,
    const char *word)
{
    size_t wordLength = strlen(word);
    const char *cursor = start;

    while ((cursor < end) && ((size_t)(end - cursor) >= wordLength)) {
	if ((memcmp(cursor, word, wordLength) == 0)
		&& ((cursor == start)
			|| (!isalnum(UCHAR(cursor[-1])) && (cursor[-1] != '_')))
		&& ((cursor + wordLength == end)
			|| (!isalnum(UCHAR(cursor[wordLength]))
				&& (cursor[wordLength] != '_')))) {
	    return 1;
	}
	cursor++;
    }
    return 0;
}

static const char *
BrowserFindBitmapArray(
    const char *contents,
    int *shortDataPtr)
{
    const char *cursor = contents;

    *shortDataPtr = 0;
    while ((cursor = strstr(cursor, "_bits")) != NULL) {
	const char *afterName = cursor + 5;
	const char *lineStart = cursor;
	const char *brace;

	while (isspace(UCHAR(*afterName))) {
	    afterName++;
	}
	if (*afterName != '[') {
	    cursor += 5;
	    continue;
	}
	while ((lineStart > contents) && (lineStart[-1] != '\n')) {
	    lineStart--;
	}
	brace = strchr(afterName, '{');
	if (brace == NULL) {
	    return NULL;
	}
	*shortDataPtr = BrowserSegmentContainsWord(lineStart, cursor, "short");
	return brace + 1;
    }
    return NULL;
}

static const char *
BrowserSkipBitmapSpace(
    const char *cursor)
{
    for (;;) {
	while (isspace(UCHAR(*cursor)) || (*cursor == ',')) {
	    cursor++;
	}
	if (*cursor == '\0') {
	    return cursor;
	}
	if ((cursor[0] == '/') && (cursor[1] == '*')) {
	    cursor += 2;
	    while ((cursor[0] != '\0')
		    && ((cursor[0] != '*') || (cursor[1] != '/'))) {
		cursor++;
	    }
	    if (cursor[0] != '\0') {
		cursor += 2;
	    }
	} else if ((cursor[0] == '/') && (cursor[1] == '/')) {
	    cursor += 2;
	    while ((*cursor != '\0') && (*cursor != '\n')) {
		cursor++;
	    }
	} else {
	    return cursor;
	}
    }
}

static int
BrowserParseBitmapArray(
    const char *array,
    int shortData,
    unsigned char *data,
    size_t dataLength)
{
    const char *cursor = array;
    size_t out = 0U;

    while (*cursor != '\0') {
	char *valueEnd;
	unsigned long value;

	cursor = BrowserSkipBitmapSpace(cursor);
	if (*cursor == '}') {
	    return out >= dataLength;
	}
	value = strtoul(cursor, &valueEnd, 0);
	if (valueEnd == cursor) {
	    cursor++;
	    continue;
	}
	if (out < dataLength) {
	    data[out++] = (unsigned char)(value & 0xffU);
	}
	if (shortData && (out < dataLength)) {
	    data[out++] = (unsigned char)((value >> 8) & 0xffU);
	}
	cursor = valueEnd;
    }
    return 0;
}

static int
BrowserParseBitmapFileData(
    const char *contents,
    unsigned int *widthReturn,
    unsigned int *heightReturn,
    unsigned char **dataReturn,
    int *xHotReturn,
    int *yHotReturn)
{
    unsigned int width;
    unsigned int height;
    int xHot;
    int yHot;
    int shortData;
    const char *array;
    unsigned char *data;
    size_t dataLength = 0U;
    int status;

    BrowserParseBitmapDefines(contents, &width, &height, &xHot, &yHot);
    status = BrowserBitmapDataLength(width, height, &dataLength);
    if (status != BitmapSuccess) {
	return status;
    }
    array = BrowserFindBitmapArray(contents, &shortData);
    if (array == NULL) {
	return BitmapFileInvalid;
    }
    data = (unsigned char *)ckalloc(dataLength);
    memset(data, 0, dataLength);
    if (!BrowserParseBitmapArray(array, shortData, data, dataLength)) {
	ckfree(data);
	return BitmapFileInvalid;
    }
    *widthReturn = width;
    *heightReturn = height;
    *dataReturn = data;
    if (xHotReturn != NULL) {
	*xHotReturn = xHot;
    }
    if (yHotReturn != NULL) {
	*yHotReturn = yHot;
    }
    return BitmapSuccess;
}

static char *
BrowserBitmapVariableName(
    const char *filename)
{
    const char *start;
    const char *end;
    const char *cursor;
    char *name;
    size_t length;
    size_t out = 0U;

    if ((filename == NULL) || (*filename == '\0')) {
	return BrowserDupString("image");
    }
    start = filename + strlen(filename);
    while ((start > filename) && (start[-1] != '/') && (start[-1] != '\\')) {
	start--;
    }
    end = start + strlen(start);
    for (cursor = end; cursor > start; cursor--) {
	if (cursor[-1] == '.') {
	    end = cursor - 1;
	    break;
	}
    }
    if (end <= start) {
	return BrowserDupString("image");
    }
    length = (size_t)(end - start);
    name = (char *)ckalloc(length + 2U);
    if (isdigit(UCHAR(*start))) {
	name[out++] = 'x';
    }
    for (cursor = start; cursor < end; cursor++) {
	name[out++] = (isalnum(UCHAR(*cursor)) || (*cursor == '_'))
		? *cursor : '_';
    }
    name[out] = '\0';
    return name;
}

int
XReadBitmapFileData(
    _Xconst char *filename,
    unsigned int *width_return,
    unsigned int *height_return,
    unsigned char **data_return,
    int *x_hot_return,
    int *y_hot_return)
{
    char *contents = NULL;
    int status;

    if (data_return != NULL) {
	*data_return = NULL;
    }
    if ((width_return == NULL) || (height_return == NULL)
	    || (data_return == NULL)) {
	return BitmapFileInvalid;
    }
    status = BrowserReadFileToString(filename, &contents);
    if (status != BitmapSuccess) {
	return status;
    }
    status = BrowserParseBitmapFileData(contents, width_return, height_return,
	    data_return, x_hot_return, y_hot_return);
    ckfree(contents);
    return status;
}

int
XReadBitmapFile(
    Display *display,
    Drawable drawable,
    _Xconst char *filename,
    unsigned int *width_return,
    unsigned int *height_return,
    Pixmap *bitmap_return,
    int *x_hot_return,
    int *y_hot_return)
{
    unsigned char *data = NULL;
    Pixmap bitmap;
    int status;

    if (bitmap_return != NULL) {
	*bitmap_return = None;
    }
    if ((width_return == NULL) || (height_return == NULL)
	    || (bitmap_return == NULL)) {
	return BitmapFileInvalid;
    }
    status = XReadBitmapFileData(filename, width_return, height_return, &data,
	    x_hot_return, y_hot_return);
    if (status != BitmapSuccess) {
	return status;
    }
    bitmap = XCreateBitmapFromData(display, drawable, (char *)data,
	    *width_return, *height_return);
    XFree(data);
    if (bitmap == None) {
	return BitmapNoMemory;
    }
    *bitmap_return = bitmap;
    return BitmapSuccess;
}

int
XWriteBitmapFile(
    Display *display,
    _Xconst char *filename,
    Pixmap bitmap,
    unsigned int width,
    unsigned int height,
    int x_hot,
    int y_hot)
{
    XImage *image;
    FILE *file;
    char *name;
    unsigned int x, y;
    int status = BitmapSuccess;

    if ((filename == NULL) || (width == 0U) || (height == 0U)) {
	return BitmapFileInvalid;
    }
    image = XGetImage(display, (Drawable)bitmap, 0, 0, width, height, 1UL,
	    XYPixmap);
    if (image == NULL) {
	return BitmapFileInvalid;
    }
    file = fopen(filename, "wb");
    if (file == NULL) {
	(void)XDestroyImage(image);
	return BitmapOpenFailed;
    }
    name = BrowserBitmapVariableName(filename);
    if ((fprintf(file, "#define %s_width %u\n", name, width) < 0)
	    || (fprintf(file, "#define %s_height %u\n", name, height) < 0)) {
	status = BitmapOpenFailed;
    }
    if ((status == BitmapSuccess) && (x_hot >= 0)
	    && (fprintf(file, "#define %s_x_hot %d\n", name, x_hot) < 0)) {
	status = BitmapOpenFailed;
    }
    if ((status == BitmapSuccess) && (y_hot >= 0)
	    && (fprintf(file, "#define %s_y_hot %d\n", name, y_hot) < 0)) {
	status = BitmapOpenFailed;
    }
    if ((status == BitmapSuccess)
	    && (fprintf(file, "static unsigned char %s_bits[] = {\n", name)
		    < 0)) {
	status = BitmapOpenFailed;
    }
    for (y = 0; (status == BitmapSuccess) && (y < height); y++) {
	unsigned int byteX;
	unsigned int bytesPerLine = (unsigned int)BrowserBitmapBytesPerLine(width);

	for (byteX = 0; byteX < bytesPerLine; byteX++) {
	    unsigned char value = 0U;
	    unsigned int bit;

	    for (bit = 0U; bit < 8U; bit++) {
		x = byteX * 8U + bit;
		if ((x < width) && ((BrowserImageGetPixel(image, (int)x,
			(int)y) & 1UL) != 0UL)) {
		    value |= (unsigned char)(1U << bit);
		}
	    }
	    if (fprintf(file, "%s0x%02x,",
		    (((y * bytesPerLine) + byteX) % 12U) == 0U ? "  " : " ",
		    value) < 0) {
		status = BitmapOpenFailed;
		break;
	    }
	    if ((((y * bytesPerLine) + byteX) % 12U) == 11U) {
		if (fprintf(file, "\n") < 0) {
		    status = BitmapOpenFailed;
		    break;
		}
	    }
	}
    }
    if ((status == BitmapSuccess) && (fprintf(file, "\n};\n") < 0)) {
	status = BitmapOpenFailed;
    }
    if (fclose(file) != 0) {
	status = BitmapOpenFailed;
    }
    XFree(name);
    (void)XDestroyImage(image);
    return status;
}
