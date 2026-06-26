#include "XlibInt.h"

static int
BrowserReadGeometryUnsignedInteger(
    const char **cursorPtr,
    int *valuePtr)
{
    const char *cursor = *cursorPtr;
    long value = 0;
    int sawDigit = 0;

    while ((*cursor >= '0') && (*cursor <= '9')) {
	if (value <= (LONG_MAX - 9L) / 10L) {
	    value = (value * 10L) + (long)(*cursor - '0');
	}
	cursor++;
	sawDigit = 1;
    }
    if (!sawDigit) {
	return 0;
    }
    if (value > INT_MAX) {
	value = INT_MAX;
    }

    *cursorPtr = cursor;
    *valuePtr = (int)value;
    return 1;
}

int
XParseGeometry(
    _Xconst char *string,
    int *xReturn,
    int *yReturn,
    unsigned int *widthReturn,
    unsigned int *heightReturn)
{
    const char *cursor;
    int value = 0;
    int mask = NoValue;
    int x = 0;
    int y = 0;
    unsigned int width = 0;
    unsigned int height = 0;

    if ((string == NULL) || (string[0] == '\0')) {
	return NoValue;
    }
    cursor = string;
    if (*cursor == '=') {
	cursor++;
    }

    if ((*cursor != '+') && (*cursor != '-') && (*cursor != 'x')
	    && (*cursor != 'X')) {
	if (!BrowserReadGeometryUnsignedInteger(&cursor, &value)) {
	    return NoValue;
	}
	width = (unsigned int)value;
	mask |= WidthValue;
    }

    if ((*cursor == 'x') || (*cursor == 'X')) {
	cursor++;
	if (!BrowserReadGeometryUnsignedInteger(&cursor, &value)) {
	    return NoValue;
	}
	height = (unsigned int)value;
	mask |= HeightValue;
    }

    if ((*cursor == '+') || (*cursor == '-')) {
	if (*cursor == '-') {
	    cursor++;
	    if (!BrowserReadGeometryUnsignedInteger(&cursor, &value)) {
		return NoValue;
	    }
	    x = -value;
	    mask |= XNegative;
	} else {
	    cursor++;
	    if (!BrowserReadGeometryUnsignedInteger(&cursor, &value)) {
		return NoValue;
	    }
	    x = value;
	}
	mask |= XValue;

	if ((*cursor == '+') || (*cursor == '-')) {
	    if (*cursor == '-') {
		cursor++;
		if (!BrowserReadGeometryUnsignedInteger(&cursor, &value)) {
		    return NoValue;
		}
		y = -value;
		mask |= YNegative;
	    } else {
		cursor++;
		if (!BrowserReadGeometryUnsignedInteger(&cursor, &value)) {
		    return NoValue;
		}
		y = value;
	    }
	    mask |= YValue;
	}
    }

    if (*cursor != '\0') {
	return NoValue;
    }
    if ((mask & XValue) && (xReturn != NULL)) {
	*xReturn = x;
    }
    if ((mask & YValue) && (yReturn != NULL)) {
	*yReturn = y;
    }
    if ((mask & WidthValue) && (widthReturn != NULL)) {
	*widthReturn = width;
    }
    if ((mask & HeightValue) && (heightReturn != NULL)) {
	*heightReturn = height;
    }
    return mask;
}

static int
BrowserGeometryMaskToGravity(
    int mask)
{
    switch (mask & (XNegative | YNegative)) {
    case XNegative:
	return NorthEastGravity;
    case YNegative:
	return SouthWestGravity;
    case XNegative | YNegative:
	return SouthEastGravity;
    default:
	return NorthWestGravity;
    }
}

static int
BrowserScreenWidth(
    Display *display,
    int screen)
{
    if ((screen < 0) || (screen >= ScreenCount(display))) {
	screen = DefaultScreen(display);
    }
    return DisplayWidth(display, screen);
}

static int
BrowserScreenHeight(
    Display *display,
    int screen)
{
    if ((screen < 0) || (screen >= ScreenCount(display))) {
	screen = DefaultScreen(display);
    }
    return DisplayHeight(display, screen);
}

int
XWMGeometry(
    Display *display,
    int screen,
    _Xconst char *userGeometry,
    _Xconst char *defaultGeometry,
    unsigned int borderWidth,
    XSizeHints *hints,
    int *xReturn,
    int *yReturn,
    int *widthReturn,
    int *heightReturn,
    int *gravityReturn)
{
    XSizeHints emptyHints;
    int userX = 0;
    int userY = 0;
    int defaultX = 0;
    int defaultY = 0;
    unsigned int userWidth = 0;
    unsigned int userHeight = 0;
    unsigned int defaultWidth = 0;
    unsigned int defaultHeight = 0;
    int userMask;
    int defaultMask;
    int resultMask;
    int baseWidth;
    int baseHeight;
    int minWidth;
    int minHeight;
    int widthInc;
    int heightInc;
    int width;
    int height;
    int x;
    int y;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (hints == NULL) {
	memset(&emptyHints, 0, sizeof(emptyHints));
	hints = &emptyHints;
    }

    baseWidth = (hints->flags & PBaseSize) ? hints->base_width
	    : ((hints->flags & PMinSize) ? hints->min_width : 0);
    baseHeight = (hints->flags & PBaseSize) ? hints->base_height
	    : ((hints->flags & PMinSize) ? hints->min_height : 0);
    minWidth = (hints->flags & PMinSize) ? hints->min_width : baseWidth;
    minHeight = (hints->flags & PMinSize) ? hints->min_height : baseHeight;
    widthInc = (hints->flags & PResizeInc) ? hints->width_inc : 1;
    heightInc = (hints->flags & PResizeInc) ? hints->height_inc : 1;
    if (widthInc <= 0) {
	widthInc = 1;
    }
    if (heightInc <= 0) {
	heightInc = 1;
    }

    resultMask = userMask = XParseGeometry(userGeometry, &userX, &userY,
	    &userWidth, &userHeight);
    defaultMask = XParseGeometry(defaultGeometry, &defaultX, &defaultY,
	    &defaultWidth, &defaultHeight);

    width = (int)(((userMask & WidthValue) ? userWidth
	    : ((defaultMask & WidthValue) ? defaultWidth : 1U))
	    * (unsigned int)widthInc) + baseWidth;
    height = (int)(((userMask & HeightValue) ? userHeight
	    : ((defaultMask & HeightValue) ? defaultHeight : 1U))
	    * (unsigned int)heightInc) + baseHeight;
    if (width < minWidth) {
	width = minWidth;
    }
    if (height < minHeight) {
	height = minHeight;
    }
    if (hints->flags & PMaxSize) {
	if (width > hints->max_width) {
	    width = hints->max_width;
	}
	if (height > hints->max_height) {
	    height = hints->max_height;
	}
    }

    if (userMask & XValue) {
	x = (userMask & XNegative)
		? BrowserScreenWidth(display, screen) + userX - width
		    - (int)(2U * borderWidth)
		: userX;
    } else if (defaultMask & XValue) {
	if (defaultMask & XNegative) {
	    x = BrowserScreenWidth(display, screen) + defaultX - width
		    - (int)(2U * borderWidth);
	    resultMask |= XNegative;
	} else {
	    x = defaultX;
	}
    } else {
	x = 0;
    }

    if (userMask & YValue) {
	y = (userMask & YNegative)
		? BrowserScreenHeight(display, screen) + userY - height
		    - (int)(2U * borderWidth)
		: userY;
    } else if (defaultMask & YValue) {
	if (defaultMask & YNegative) {
	    y = BrowserScreenHeight(display, screen) + defaultY - height
		    - (int)(2U * borderWidth);
	    resultMask |= YNegative;
	} else {
	    y = defaultY;
	}
    } else {
	y = 0;
    }

    if (xReturn != NULL) {
	*xReturn = x;
    }
    if (yReturn != NULL) {
	*yReturn = y;
    }
    if (widthReturn != NULL) {
	*widthReturn = width;
    }
    if (heightReturn != NULL) {
	*heightReturn = height;
    }
    if (gravityReturn != NULL) {
	*gravityReturn = BrowserGeometryMaskToGravity(resultMask);
    }
    return resultMask;
}
