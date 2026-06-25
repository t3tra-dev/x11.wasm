#include "XlibInt.h"

XClassHint *
XAllocClassHint(void)
{
    XClassHint *hintPtr = (XClassHint *)ckalloc(sizeof(XClassHint));
    memset(hintPtr, 0, sizeof(XClassHint));
    return hintPtr;
}

XSizeHints *
XAllocSizeHints(void)
{
    XSizeHints *hintPtr = (XSizeHints *)ckalloc(sizeof(XSizeHints));
    memset(hintPtr, 0, sizeof(XSizeHints));
    return hintPtr;
}

XWMHints *
XAllocWMHints(void)
{
    XWMHints *hintPtr = (XWMHints *)ckalloc(sizeof(XWMHints));
    memset(hintPtr, 0, sizeof(XWMHints));
    return hintPtr;
}

XIconSize *
XAllocIconSize(void)
{
    XIconSize *hintPtr = (XIconSize *)ckalloc(sizeof(XIconSize));

    memset(hintPtr, 0, sizeof(XIconSize));
    return hintPtr;
}

XStandardColormap *
XAllocStandardColormap(void)
{
    XStandardColormap *colormapPtr =
	    (XStandardColormap *)ckalloc(sizeof(XStandardColormap));

    memset(colormapPtr, 0, sizeof(XStandardColormap));
    return colormapPtr;
}

int
XSetClassHint(
    Display *display,
    Window window,
    XClassHint *hints)
{
    const char *resName = ((hints != NULL) && (hints->res_name != NULL))
	    ? hints->res_name : "";
    const char *resClass = ((hints != NULL) && (hints->res_class != NULL))
	    ? hints->res_class : "";
    size_t nameLength = strlen(resName);
    size_t classLength = strlen(resClass);
    size_t byteLength = nameLength + classLength + 2U;
    unsigned char *data = (unsigned char *)ckalloc(byteLength);
    int result;

    memcpy(data, resName, nameLength + 1U);
    memcpy(data + nameLength + 1U, resClass, classLength + 1U);
    result = XChangeProperty(display, window, XA_WM_CLASS, XA_STRING, 8,
	    PropModeReplace, data, (int)byteLength);
    ckfree(data);
    return result;
}

Status
XGetClassHint(
    Display *display,
    Window window,
    XClassHint *classHintsReturn)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    int status;
    int nameLength = 0;

    if (classHintsReturn == NULL) {
	return False;
    }
    classHintsReturn->res_name = NULL;
    classHintsReturn->res_class = NULL;
    status = XGetWindowProperty(display, window, XA_WM_CLASS, 0,
	    (long)(LONG_MAX / 4L), False, XA_STRING, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data);
    if ((status != Success) || (actualType != XA_STRING)
	    || (actualFormat != 8) || (nitems == 0) || (bytesAfter != 0)
	    || (data == NULL)) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    while ((nameLength < (int)nitems) && (data[nameLength] != '\0')) {
	nameLength++;
    }
    classHintsReturn->res_name = BrowserDupCountedString(data,
	    (int)nitems);
    classHintsReturn->res_class = BrowserDupCountedString(
	    (nameLength + 1 < (int)nitems) ? data + nameLength + 1 : NULL,
	    (nameLength + 1 < (int)nitems)
	    ? (int)nitems - nameLength - 1 : 0);
    XFree(data);
    return True;
}

int
XSetCommand(
    Display *display,
    Window window,
    char **argv,
    int argc)
{
    unsigned char *data;
    size_t byteLength = 0;
    size_t offset = 0;
    int i, result;

    if ((argv == NULL) || (argc <= 0)) {
	result = XChangeProperty(display, window, XA_WM_COMMAND, XA_STRING, 8,
		PropModeReplace, (const unsigned char *)"", 0);
	return result;
    }
    for (i = 0; i < argc; i++) {
	if (argv[i] != NULL) {
	    byteLength += strlen(argv[i]);
	}
	byteLength++;
    }
    data = (unsigned char *)ckalloc(byteLength);
    for (i = 0; i < argc; i++) {
	size_t length = (argv[i] != NULL) ? strlen(argv[i]) : 0U;

	if (length > 0) {
	    memcpy(data + offset, argv[i], length);
	}
	offset += length;
	data[offset++] = '\0';
    }
    result = XChangeProperty(display, window, XA_WM_COMMAND, XA_STRING, 8,
	    PropModeReplace, data, (int)byteLength);
    ckfree(data);
    return result;
}

Status
XGetCommand(
    Display *display,
    Window window,
    char ***argvReturn,
    int *argcReturn)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    char **argv = NULL;
    int argc = 0;
    unsigned long offset = 0;
    int status;

    if (argvReturn != NULL) {
	*argvReturn = NULL;
    }
    if (argcReturn != NULL) {
	*argcReturn = 0;
    }
    if ((argvReturn == NULL) || (argcReturn == NULL)) {
	return False;
    }
    status = XGetWindowProperty(display, window, XA_WM_COMMAND, 0,
	    (long)(LONG_MAX / 4L), False, XA_STRING, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data);
    if ((status != Success) || (actualType != XA_STRING)
	    || (actualFormat != 8) || (bytesAfter != 0)
	    || ((nitems > 0) && (data == NULL))) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    while (offset < nitems) {
	unsigned long length = 0;

	while ((offset + length < nitems)
		&& (data[offset + length] != '\0')) {
	    length++;
	}
	argc++;
	offset += length + 1U;
    }
    if (argc > 0) {
	int out = 0;

	argv = (char **)ckalloc(sizeof(char *) * ((size_t)argc + 1U));
	offset = 0;
	while (offset < nitems) {
	    unsigned long length = 0;

	    while ((offset + length < nitems)
		    && (data[offset + length] != '\0')) {
		length++;
	    }
	    argv[out++] = BrowserDupCountedString(data + offset,
		    (int)length);
	    offset += length + 1U;
	}
	argv[out] = NULL;
    }
    XFree(data);
    *argvReturn = argv;
    *argcReturn = argc;
    return True;
}

int
XSetIconName(
    Display *display,
    Window window,
    _Xconst char *name)
{
    const char *value = (name != NULL) ? name : "";
    int result = XChangeProperty(display, window, XA_WM_ICON_NAME, XA_STRING, 8,
	    PropModeReplace, (const unsigned char *)value, (int)strlen(value));

    return result;
}

Status
BrowserFetchTextPropertyString(
    Display *display,
    Window window,
    Atom property,
    char **stringReturn)
{
    XTextProperty textProp;
    char **list = NULL;
    int count = 0;
    Status status = 0;

    if (stringReturn != NULL) {
	*stringReturn = NULL;
    }
    if (stringReturn == NULL) {
	return 0;
    }
    memset(&textProp, 0, sizeof(textProp));
    if (!XGetTextProperty(display, window, &textProp, property)) {
	return 0;
    }
    if (BrowserPropertyIsTextEncoding(textProp.encoding)
	    && BrowserTextPropertyToStringList(&textProp, &list, &count)
	    && (count > 0) && (list != NULL) && (list[0] != NULL)) {
	*stringReturn = BrowserDupString(list[0]);
	status = 1;
    }
    if (list != NULL) {
	XFreeStringList(list);
    }
    if (textProp.value != NULL) {
	XFree(textProp.value);
    }
    return status;
}

Status
XFetchName(
    Display *display,
    Window window,
    char **windowNameReturn)
{
    return BrowserFetchTextPropertyString(display, window, XA_WM_NAME,
	    windowNameReturn);
}

Status
XGetIconName(
    Display *display,
    Window window,
    char **iconNameReturn)
{
    return BrowserFetchTextPropertyString(display, window, XA_WM_ICON_NAME,
	    iconNameReturn);
}

int
XStoreName(
    Display *display,
    Window window,
    _Xconst char *name)
{
    const char *value = (name != NULL) ? name : "";
    int result = XChangeProperty(display, window, XA_WM_NAME, XA_STRING, 8,
	    PropModeReplace, (const unsigned char *)value, (int)strlen(value));

    return result;
}

int
XSetTransientForHint(
    Display *display,
    Window window,
    Window propWindow)
{
    int result = XChangeProperty(display, window, XA_WM_TRANSIENT_FOR,
	    XA_WM_TRANSIENT_FOR, 32, PropModeReplace,
	    (const unsigned char *)&propWindow, 1);

    return result;
}

Status
XGetTransientForHint(
    Display *display,
    Window window,
    Window *propWindowReturn)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    int status;

    if (propWindowReturn != NULL) {
	*propWindowReturn = None;
    }
    if (propWindowReturn == NULL) {
	return False;
    }
    status = XGetWindowProperty(display, window, XA_WM_TRANSIENT_FOR, 0, 1,
	    False, AnyPropertyType, &actualType, &actualFormat, &nitems,
	    &bytesAfter, &data);
    if ((status != Success)
	    || ((actualType != XA_WM_TRANSIENT_FOR) && (actualType != XA_WINDOW))
	    || (actualFormat != 32) || (nitems < 1) || (bytesAfter != 0)
	    || (data == NULL)) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    *propWindowReturn = (Window)BrowserPropertyAtomAt(data, 0);
    XFree(data);
    return True;
}

int
XSetWMHints(
    Display *display,
    Window window,
    XWMHints *hints)
{
    int result;

    if (hints == NULL) {
	result = XDeleteProperty(display, window, XA_WM_HINTS);
	return result;
    }
    result = XChangeProperty(display, window, XA_WM_HINTS, XA_WM_HINTS, 32,
	    PropModeReplace, (const unsigned char *)hints,
	    (int)(sizeof(XWMHints) / 4U));
    return result;
}

XWMHints *
XGetWMHints(
    Display *display,
    Window window)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    XWMHints *hints;
    int status = XGetWindowProperty(display, window, XA_WM_HINTS, 0,
	    (long)(LONG_MAX / 4L), False, XA_WM_HINTS, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data);

    if ((status != Success) || (actualType != XA_WM_HINTS)
	    || (actualFormat != 32) || (bytesAfter != 0)
	    || (data == NULL)
	    || (nitems * 4UL < (unsigned long)sizeof(XWMHints))) {
	if (data != NULL) {
	    XFree(data);
	}
	return NULL;
    }
    hints = XAllocWMHints();
    memcpy(hints, data, sizeof(XWMHints));
    XFree(data);
    return hints;
}

EMSCRIPTEN_KEEPALIVE unsigned int
X11WasmTestMapWindowWithInitialState(int initialState)
{
    XWMHints hints;
    Window window;

    BrowserInitDisplay();
    window = XCreateWindow(&browserDisplay.display, browserDisplay.rootWindow,
	    9, 10, 48, 36, 0, CopyFromParent, InputOutput, NULL, 0, NULL);
    if (window == None) {
	return 0;
    }
    memset(&hints, 0, sizeof(hints));
    hints.flags = StateHint;
    hints.initial_state = initialState;
    if ((XSetWMHints(&browserDisplay.display, window, &hints) != Success)
	    || (XMapWindow(&browserDisplay.display, window) != Success)) {
	return 0;
    }
    return (unsigned int)BrowserJsXId(window);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetWindowMetadataProperties(unsigned int window, int enabled)
{
    static char *commandArgv[] = {
	(char *)"x11wasm-test", (char *)"--metadata", NULL
    };
    XClassHint classHint;
    XSizeHints sizeHints;
    XTextProperty clientMachine;
    XWMHints wmHints;
    Window xid = (Window)window;

    BrowserInitDisplay();
    if (!enabled) {
	(void)XDeleteProperty(&browserDisplay.display, xid, XA_WM_NAME);
	(void)XDeleteProperty(&browserDisplay.display, xid, XA_WM_ICON_NAME);
	(void)XDeleteProperty(&browserDisplay.display, xid, XA_WM_COMMAND);
	(void)XDeleteProperty(&browserDisplay.display, xid, XA_WM_CLASS);
	(void)XDeleteProperty(&browserDisplay.display, xid,
		XA_WM_CLIENT_MACHINE);
	(void)XDeleteProperty(&browserDisplay.display, xid,
		XA_WM_TRANSIENT_FOR);
	(void)XDeleteProperty(&browserDisplay.display, xid, XA_WM_HINTS);
	(void)XDeleteProperty(&browserDisplay.display, xid,
		XA_WM_NORMAL_HINTS);
	return 1;
    }

    classHint.res_name = (char *)"metadata";
    classHint.res_class = (char *)"X11WasmMetadata";
    memset(&sizeHints, 0, sizeof(sizeHints));
    sizeHints.flags = PPosition | PSize | PMinSize | PMaxSize | PResizeInc
	    | PAspect | PBaseSize | PWinGravity;
    sizeHints.x = 11;
    sizeHints.y = 13;
    sizeHints.width = 120;
    sizeHints.height = 80;
    sizeHints.min_width = 40;
    sizeHints.min_height = 30;
    sizeHints.max_width = 240;
    sizeHints.max_height = 160;
    sizeHints.width_inc = 7;
    sizeHints.height_inc = 5;
    sizeHints.min_aspect.x = 2;
    sizeHints.min_aspect.y = 3;
    sizeHints.max_aspect.x = 4;
    sizeHints.max_aspect.y = 5;
    sizeHints.base_width = 20;
    sizeHints.base_height = 10;
    sizeHints.win_gravity = SouthEastGravity;

    memset(&wmHints, 0, sizeof(wmHints));
    wmHints.flags = InputHint | StateHint | IconPositionHint;
    wmHints.input = False;
    wmHints.initial_state = IconicState;
    wmHints.icon_x = 17;
    wmHints.icon_y = 29;

    memset(&clientMachine, 0, sizeof(clientMachine));
    clientMachine.value = (unsigned char *)"x11wasm-client";
    clientMachine.encoding = XA_STRING;
    clientMachine.format = 8;
    clientMachine.nitems = strlen((const char *)clientMachine.value);

    if ((XStoreName(&browserDisplay.display, xid, "X11Wasm Metadata Title")
	    != Success)
	    || (XSetIconName(&browserDisplay.display, xid,
		    "X11Wasm Metadata Icon") != Success)
	    || (XSetClassHint(&browserDisplay.display, xid, &classHint)
		    != Success)
	    || (XSetCommand(&browserDisplay.display, xid, commandArgv, 2)
		    != Success)
	    || (XSetTransientForHint(&browserDisplay.display, xid,
		    browserDisplay.rootWindow) != Success)
	    || (XSetWMHints(&browserDisplay.display, xid, &wmHints)
		    != Success)) {
	return 0;
    }
    XSetWMNormalHints(&browserDisplay.display, xid, &sizeHints);
    XSetWMClientMachine(&browserDisplay.display, xid, &clientMachine);
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetWindowMetadataTargetProperty(
    unsigned int window,
    unsigned int target,
    int isRootToplevel)
{
    Atom targetAtom;
    Atom rootToplevelAtom;
    Window targetWindow = (Window)target;
    unsigned int targetValue = target;
    unsigned int rootToplevelValue = isRootToplevel ? 1U : 0U;

    BrowserInitDisplay();
    if (BrowserWindowResource((Window)window) == NULL) {
	return 0;
    }
    if ((targetWindow != None) && (BrowserWindowResource(targetWindow) == NULL)) {
	return 0;
    }
    targetAtom = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_METADATA_TARGET", False);
    rootToplevelAtom = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_METADATA_ROOT_TOPLEVEL", False);
    if ((XChangeProperty(&browserDisplay.display, (Window)window,
	    rootToplevelAtom, XA_CARDINAL, 32, PropModeReplace,
	    (const unsigned char *)&rootToplevelValue, 1) != Success)
	    || (XChangeProperty(&browserDisplay.display, (Window)window, targetAtom,
		    XA_WINDOW, 32, PropModeReplace,
		    (const unsigned char *)&targetValue, 1) != Success)) {
	return 0;
    }
    return 1;
}

void
XSetWMSizeHints(
    Display *display,
    Window window,
    XSizeHints *hints,
    Atom property)
{
    if (hints == NULL) {
	(void)XDeleteProperty(display, window, property);
	return;
    }
    (void)XChangeProperty(display, window, property, XA_WM_SIZE_HINTS, 32,
	    PropModeReplace,
	    (const unsigned char *)hints, (int)(sizeof(XSizeHints) / 4U));
}

int
XSetSizeHints(
    Display *display,
    Window window,
    XSizeHints *hints,
    Atom property)
{
    XSetWMSizeHints(display, window, hints, property);
    return Success;
}

int
XSetNormalHints(
    Display *display,
    Window window,
    XSizeHints *hints)
{
    return XSetSizeHints(display, window, hints, XA_WM_NORMAL_HINTS);
}

int
XSetZoomHints(
    Display *display,
    Window window,
    XSizeHints *hints)
{
    return XSetSizeHints(display, window, hints, XA_WM_ZOOM_HINTS);
}

void
XSetWMNormalHints(
    Display *display,
    Window window,
    XSizeHints *hints)
{
    XSetWMSizeHints(display, window, hints, XA_WM_NORMAL_HINTS);
}

Status
XGetWMSizeHints(
    Display *display,
    Window window,
    XSizeHints *hintsReturn,
    long *suppliedReturn,
    Atom property)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    int status;

    if (hintsReturn != NULL) {
	memset(hintsReturn, 0, sizeof(*hintsReturn));
    }
    if (suppliedReturn != NULL) {
	*suppliedReturn = 0;
    }
    if (hintsReturn == NULL) {
	return False;
    }
    status = XGetWindowProperty(display, window, property, 0,
	    (long)(LONG_MAX / 4L), False, XA_WM_SIZE_HINTS, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data);
    if ((status != Success) || (actualType != XA_WM_SIZE_HINTS)
	    || (actualFormat != 32) || (bytesAfter != 0)
	    || (data == NULL)
	    || (nitems * 4UL < (unsigned long)sizeof(XSizeHints))) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    memcpy(hintsReturn, data, sizeof(XSizeHints));
    if (suppliedReturn != NULL) {
	*suppliedReturn = hintsReturn->flags;
    }
    XFree(data);
    return True;
}

Status
XGetSizeHints(
    Display *display,
    Window window,
    XSizeHints *hintsReturn,
    Atom property)
{
    long supplied;

    return XGetWMSizeHints(display, window, hintsReturn, &supplied,
	    property);
}

Status
XGetNormalHints(
    Display *display,
    Window window,
    XSizeHints *hintsReturn)
{
    return XGetSizeHints(display, window, hintsReturn, XA_WM_NORMAL_HINTS);
}

Status
XGetZoomHints(
    Display *display,
    Window window,
    XSizeHints *hintsReturn)
{
    return XGetSizeHints(display, window, hintsReturn, XA_WM_ZOOM_HINTS);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetZoomHints(unsigned int window, int enabled)
{
    XSizeHints hints;

    BrowserInitDisplay();
    if (!enabled) {
	return XSetZoomHints(&browserDisplay.display, (Window)window, NULL);
    }

    memset(&hints, 0, sizeof(hints));
    hints.flags = PPosition | PSize | PMinSize | PMaxSize | PResizeInc
	    | PAspect | PBaseSize | PWinGravity;
    hints.x = 11;
    hints.y = 13;
    hints.width = 320;
    hints.height = 180;
    hints.min_width = 160;
    hints.min_height = 90;
    hints.max_width = 640;
    hints.max_height = 360;
    hints.width_inc = 8;
    hints.height_inc = 6;
    hints.min_aspect.x = 16;
    hints.min_aspect.y = 9;
    hints.max_aspect.x = 4;
    hints.max_aspect.y = 3;
    hints.base_width = 80;
    hints.base_height = 45;
    hints.win_gravity = SouthEastGravity;
    return XSetZoomHints(&browserDisplay.display, (Window)window, &hints);
}

EMSCRIPTEN_KEEPALIVE int
XSetIconSizes(
    Display *display,
    Window window,
    XIconSize *sizeList,
    int count)
{
    if ((sizeList == NULL) || (count <= 0)) {
	(void)XDeleteProperty(display, window, XA_WM_ICON_SIZE);
	return Success;
    }
    return XChangeProperty(display, window, XA_WM_ICON_SIZE, XA_WM_ICON_SIZE,
	    32, PropModeReplace, (const unsigned char *)sizeList,
	    count * (int)(sizeof(XIconSize) / 4U));
}

Status
XGetIconSizes(
    Display *display,
    Window window,
    XIconSize **sizeListReturn,
    int *countReturn)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    XIconSize *copyPtr;
    int count;
    int status;

    if (sizeListReturn != NULL) {
	*sizeListReturn = NULL;
    }
    if (countReturn != NULL) {
	*countReturn = 0;
    }
    status = XGetWindowProperty(display, window, XA_WM_ICON_SIZE, 0,
	    (long)(LONG_MAX / 4L), False, XA_WM_ICON_SIZE, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data);
    if ((status != Success) || (actualType != XA_WM_ICON_SIZE)
	    || (actualFormat != 32) || (bytesAfter != 0)
	    || (data == NULL) || (nitems == 0)
	    || ((nitems * 4UL) % sizeof(XIconSize)) != 0) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    count = (int)((nitems * 4UL) / sizeof(XIconSize));
    copyPtr = (XIconSize *)ckalloc((size_t)count * sizeof(XIconSize));
    memcpy(copyPtr, data, (size_t)count * sizeof(XIconSize));
    XFree(data);
    if (sizeListReturn != NULL) {
	*sizeListReturn = copyPtr;
    } else {
	XFree(copyPtr);
    }
    if (countReturn != NULL) {
	*countReturn = count;
    }
    return True;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetWindowIconSizes(
    unsigned int window,
    int enabled)
{
    XIconSize iconSizes[2];

    BrowserInitDisplay();
    if (!enabled) {
	return XSetIconSizes(&browserDisplay.display, (Window)window, NULL, 0);
    }
    iconSizes[0].min_width = 16;
    iconSizes[0].min_height = 16;
    iconSizes[0].max_width = 64;
    iconSizes[0].max_height = 64;
    iconSizes[0].width_inc = 8;
    iconSizes[0].height_inc = 8;
    iconSizes[1].min_width = 32;
    iconSizes[1].min_height = 32;
    iconSizes[1].max_width = 128;
    iconSizes[1].max_height = 128;
    iconSizes[1].width_inc = 16;
    iconSizes[1].height_inc = 16;
    return XSetIconSizes(&browserDisplay.display, (Window)window,
	    iconSizes, 2);
}

EMSCRIPTEN_KEEPALIVE unsigned int
X11WasmTestSetRootIconSizeAndWindowIcon(void)
{
    Atom iconAtom;
    Window window;
    XIconSize iconSize;
    unsigned int *data;
    int count = 2 + (16 * 16) + 2 + (32 * 32);
    int index = 0;
    int i;

    BrowserInitDisplay();
    iconAtom = XInternAtom(&browserDisplay.display, "_NET_WM_ICON", False);
    window = XCreateWindow(&browserDisplay.display, browserDisplay.rootWindow,
	    7, 11, 48, 36, 0, CopyFromParent, InputOutput, NULL, 0, NULL);
    if ((iconAtom == None) || (window == None)) {
	return 0;
    }

    memset(&iconSize, 0, sizeof(iconSize));
    iconSize.min_width = 16;
    iconSize.min_height = 16;
    iconSize.max_width = 16;
    iconSize.max_height = 16;
    iconSize.width_inc = 1;
    iconSize.height_inc = 1;
    if (XSetIconSizes(&browserDisplay.display, browserDisplay.rootWindow,
	    &iconSize, 1) != Success) {
	return 0;
    }

    data = (unsigned int *)ckalloc((size_t)count * sizeof(unsigned int));
    data[index++] = 16;
    data[index++] = 16;
    for (i = 0; i < 16 * 16; i++) {
	data[index++] = 0xffff0000U;
    }
    data[index++] = 32;
    data[index++] = 32;
    for (i = 0; i < 32 * 32; i++) {
	data[index++] = 0xff00ff00U;
    }
    (void)XChangeProperty(&browserDisplay.display, window, iconAtom,
	    XA_CARDINAL, 32, PropModeReplace, (const unsigned char *)data,
	    count);
    ckfree(data);
    return (unsigned int)BrowserJsXId(window);
}

void
XSetRGBColormaps(
    Display *display,
    Window window,
    XStandardColormap *stdcmaps,
    int count,
    Atom property)
{
    if ((stdcmaps == NULL) || (count <= 0)) {
	(void)XDeleteProperty(display, window, property);
	return;
    }
    (void)XChangeProperty(display, window, property, XA_RGB_COLOR_MAP, 32,
	    PropModeReplace, (const unsigned char *)stdcmaps,
	    count * (int)(sizeof(XStandardColormap) / 4U));
}

Status
XGetRGBColormaps(
    Display *display,
    Window window,
    XStandardColormap **stdcmapsReturn,
    int *countReturn,
    Atom property)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    XStandardColormap *copyPtr;
    int count;
    int status;

    if (stdcmapsReturn != NULL) {
	*stdcmapsReturn = NULL;
    }
    if (countReturn != NULL) {
	*countReturn = 0;
    }
    status = XGetWindowProperty(display, window, property, 0,
	    (long)(LONG_MAX / 4L), False, XA_RGB_COLOR_MAP, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data);
    if ((status != Success) || (actualType != XA_RGB_COLOR_MAP)
	    || (actualFormat != 32) || (bytesAfter != 0)
	    || (data == NULL) || (nitems == 0)
	    || ((nitems * 4UL) % sizeof(XStandardColormap)) != 0) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    count = (int)((nitems * 4UL) / sizeof(XStandardColormap));
    copyPtr = (XStandardColormap *)ckalloc((size_t)count
	    * sizeof(XStandardColormap));
    memcpy(copyPtr, data, (size_t)count * sizeof(XStandardColormap));
    XFree(data);
    if (stdcmapsReturn != NULL) {
	*stdcmapsReturn = copyPtr;
    } else {
	XFree(copyPtr);
    }
    if (countReturn != NULL) {
	*countReturn = count;
    }
    return True;
}

void
XSetStandardColormap(
    Display *display,
    Window window,
    XStandardColormap *colormap,
    Atom property)
{
    XSetRGBColormaps(display, window, colormap, (colormap != NULL) ? 1 : 0,
	    property);
}

Status
XGetStandardColormap(
    Display *display,
    Window window,
    XStandardColormap *colormapReturn,
    Atom property)
{
    XStandardColormap *stdcmaps = NULL;
    int count = 0;

    if (colormapReturn == NULL) {
	return False;
    }
    memset(colormapReturn, 0, sizeof(*colormapReturn));
    if (!XGetRGBColormaps(display, window, &stdcmaps, &count, property)
	    || (stdcmaps == NULL) || (count <= 0)) {
	return False;
    }
    *colormapReturn = stdcmaps[0];
    XFree(stdcmaps);
    return True;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetStandardColormaps(unsigned int window, int enabled)
{
    Atom property;
    XStandardColormap colormaps[2];

    BrowserInitDisplay();
    property = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_RGB_MAP", False);
    if (!enabled) {
	XSetRGBColormaps(&browserDisplay.display, (Window)window, NULL, 0,
		property);
	return Success;
    }
    memset(colormaps, 0, sizeof(colormaps));
    colormaps[0].colormap = browserDisplay.defaultColormap;
    colormaps[0].red_max = 255;
    colormaps[0].red_mult = 65536;
    colormaps[0].green_max = 255;
    colormaps[0].green_mult = 256;
    colormaps[0].blue_max = 255;
    colormaps[0].blue_mult = 1;
    colormaps[0].base_pixel = 0;
    colormaps[0].visualid = browserDisplay.visual.visualid;
    colormaps[0].killid = ReleaseByFreeingColormap;
    colormaps[1] = colormaps[0];
    colormaps[1].base_pixel = 16;
    colormaps[1].red_max = 31;
    colormaps[1].green_max = 63;
    colormaps[1].blue_max = 31;
    XSetRGBColormaps(&browserDisplay.display, (Window)window, colormaps, 2,
	    property);
    return Success;
}

Status
XGetWMNormalHints(
    Display *display,
    Window window,
    XSizeHints *hintsReturn,
    long *suppliedReturn)
{
    return XGetWMSizeHints(display, window, hintsReturn, suppliedReturn,
	    XA_WM_NORMAL_HINTS);
}

void
XSetWMProperties(
    Display *display,
    Window window,
    XTextProperty *windowName,
    XTextProperty *iconName,
    char **argv,
    int argc,
    XSizeHints *normalHints,
    XWMHints *wmHints,
    XClassHint *classHints)
{
    if (windowName != NULL) {
	XSetWMName(display, window, windowName);
    }
    if (iconName != NULL) {
	XSetWMIconName(display, window, iconName);
    }
    if (argv != NULL) {
	(void)XSetCommand(display, window, argv, argc);
    }
    if (normalHints != NULL) {
	XSetWMNormalHints(display, window, normalHints);
    }
    if (wmHints != NULL) {
	(void)XSetWMHints(display, window, wmHints);
    }
    if (classHints != NULL) {
	(void)XSetClassHint(display, window, classHints);
    }
}

void
BrowserSetWMPropertiesFromStrings(
    Display *display,
    Window window,
    const char *windowName,
    const char *iconName,
    char **argv,
    int argc,
    XSizeHints *normalHints,
    XWMHints *wmHints,
    XClassHint *classHints,
    XICCEncodingStyle style)
{
    XTextProperty windowNameProp;
    XTextProperty iconNameProp;
    XTextProperty *windowNamePtr = NULL;
    XTextProperty *iconNamePtr = NULL;
    char *windowNameList[1];
    char *iconNameList[1];

    memset(&windowNameProp, 0, sizeof(windowNameProp));
    memset(&iconNameProp, 0, sizeof(iconNameProp));
    if (windowName != NULL) {
	windowNameList[0] = (char *)windowName;
	if (BrowserTextListToTextProperty(display, windowNameList, 1, style,
		&windowNameProp) == Success) {
	    windowNamePtr = &windowNameProp;
	}
    }
    if (iconName != NULL) {
	iconNameList[0] = (char *)iconName;
	if (BrowserTextListToTextProperty(display, iconNameList, 1, style,
		&iconNameProp) == Success) {
	    iconNamePtr = &iconNameProp;
	}
    }
    XSetWMProperties(display, window, windowNamePtr, iconNamePtr, argv, argc,
	    normalHints, wmHints, classHints);
    if (windowNameProp.value != NULL) {
	XFree(windowNameProp.value);
    }
    if (iconNameProp.value != NULL) {
	XFree(iconNameProp.value);
    }
}

int
XSetStandardProperties(
    Display *display,
    Window window,
    _Xconst char *windowName,
    _Xconst char *iconName,
    Pixmap iconPixmap,
    char **argv,
    int argc,
    XSizeHints *hints)
{
    XWMHints wmHints;
    XWMHints *wmHintsPtr = NULL;

    memset(&wmHints, 0, sizeof(wmHints));
    if (iconPixmap != None) {
	wmHints.flags = IconPixmapHint;
	wmHints.icon_pixmap = iconPixmap;
	wmHintsPtr = &wmHints;
    }
    BrowserSetWMPropertiesFromStrings(display, window, windowName, iconName,
	    argv, argc, hints, wmHintsPtr, NULL, XStringStyle);
    return Success;
}

void
XmbSetWMProperties(
    Display *display,
    Window window,
    _Xconst char *windowName,
    _Xconst char *iconName,
    char **argv,
    int argc,
    XSizeHints *normalHints,
    XWMHints *wmHints,
    XClassHint *classHints)
{
    BrowserSetWMPropertiesFromStrings(display, window, windowName, iconName,
	    argv, argc, normalHints, wmHints, classHints, XStdICCTextStyle);
}

void
Xutf8SetWMProperties(
    Display *display,
    Window window,
    _Xconst char *windowName,
    _Xconst char *iconName,
    char **argv,
    int argc,
    XSizeHints *normalHints,
    XWMHints *wmHints,
    XClassHint *classHints)
{
    BrowserSetWMPropertiesFromStrings(display, window, windowName, iconName,
	    argv, argc, normalHints, wmHints, classHints, XUTF8StringStyle);
}
Status
XGetWMColormapWindows(
    Display *display,
    Window window,
    Window **windows,
    int *count)
{
    Atom property = BrowserWmColormapWindowsAtom();
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    Window *copyPtr = NULL;
    int status;
    unsigned long i;

    if (windows != NULL) {
	*windows = NULL;
    }
    if (count != NULL) {
	*count = 0;
    }
    status = XGetWindowProperty(display, window, property, 0,
	    (long)(LONG_MAX / 4L), False, XA_WINDOW, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data);
    if ((status != Success) || (actualType != XA_WINDOW)
	    || (actualFormat != 32) || (bytesAfter != 0)
	    || (nitems == 0) || (data == NULL)) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    copyPtr = (Window *)ckalloc((size_t)nitems * sizeof(Window));
    for (i = 0; i < nitems; i++) {
	copyPtr[i] = (Window)BrowserPropertyAtomAt(data, i);
    }
    XFree(data);
    if (windows != NULL) {
	*windows = copyPtr;
    } else {
	XFree(copyPtr);
    }
    if (count != NULL) {
	*count = (int)nitems;
    }
    return True;
}

Status
XSetWMColormapWindows(
    Display *display,
    Window window,
    Window *windows,
    int count)
{
    Atom property = BrowserWmColormapWindowsAtom();

    if ((count <= 0) || (windows == NULL)) {
	(void)XDeleteProperty(display, window, property);
	return True;
    }
    (void)XChangeProperty(display, window, property, XA_WINDOW, 32,
	    PropModeReplace, (const unsigned char *)windows, count);
    return True;
}

Status
XSetWMProtocols(
    Display *display,
    Window window,
    Atom *protocols,
    int count)
{
    if (count < 0) {
	if (display == NULL) {
	    display = X11WasmGetDisplay(NULL);
	}
	(void)BrowserValueOrError(&display, count, 0,
		X11_WASM_XREQ_CHANGE_PROPERTY);
	return False;
    }
    if (count == 0) {
	(void)XDeleteProperty(display, window, BrowserWmProtocolsAtom());
	return True;
    }
    if (protocols == NULL) {
	if (display == NULL) {
	    display = X11WasmGetDisplay(NULL);
	}
	(void)BrowserPropertyValueOrError(&display, (XID)count,
		X11_WASM_XREQ_CHANGE_PROPERTY);
	return False;
    }
    (void)XChangeProperty(display, window, BrowserWmProtocolsAtom(), XA_ATOM,
	    32, PropModeReplace, (const unsigned char *)protocols, count);
    return True;
}

Status
XGetWMProtocols(
    Display *display,
    Window window,
    Atom **protocolsReturn,
    int *countReturn)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    Atom *copyPtr = NULL;
    int status;
    unsigned long i;

    if (protocolsReturn != NULL) {
	*protocolsReturn = NULL;
    }
    if (countReturn != NULL) {
	*countReturn = 0;
    }
    if ((protocolsReturn == NULL) || (countReturn == NULL)) {
	return False;
    }
    status = XGetWindowProperty(display, window, BrowserWmProtocolsAtom(), 0,
	    (long)(LONG_MAX / 4L), False, XA_ATOM, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data);
    if ((status != Success) || (actualType != XA_ATOM)
	    || (actualFormat != 32) || (bytesAfter != 0)
	    || ((nitems > 0) && (data == NULL))) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    if (nitems > 0) {
	copyPtr = (Atom *)ckalloc((size_t)nitems * sizeof(Atom));
	for (i = 0; i < nitems; i++) {
	    copyPtr[i] = BrowserPropertyAtomAt(data, i);
	}
    }
    if (data != NULL) {
	XFree(data);
    }
    *protocolsReturn = copyPtr;
    *countReturn = (int)nitems;
    return True;
}

void
XSetWMClientMachine(
    Display *display,
    Window window,
    XTextProperty *textProp)
{
    if ((textProp == NULL) || (textProp->value == NULL)) {
	(void)XDeleteProperty(display, window, XA_WM_CLIENT_MACHINE);
	return;
    }
    (void)XChangeProperty(display, window, XA_WM_CLIENT_MACHINE,
	    textProp->encoding, textProp->format, PropModeReplace,
	    textProp->value, (int)textProp->nitems);
}

Status
XGetTextProperty(
    Display *display,
    Window window,
    XTextProperty *textPropReturn,
    Atom property)
{
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    int status;

    if (textPropReturn == NULL) {
	return False;
    }
    memset(textPropReturn, 0, sizeof(*textPropReturn));
    status = XGetWindowProperty(display, window, property, 0,
	    (long)(LONG_MAX / 4L), False, AnyPropertyType, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data);
    if ((status != Success) || (actualType == None) || (actualFormat == 0)
	    || (bytesAfter != 0)) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    textPropReturn->value = data;
    textPropReturn->encoding = actualType;
    textPropReturn->format = actualFormat;
    textPropReturn->nitems = nitems;
    return True;
}

Status
XGetWMName(
    Display *display,
    Window window,
    XTextProperty *textPropReturn)
{
    return XGetTextProperty(display, window, textPropReturn, XA_WM_NAME);
}

Status
XGetWMIconName(
    Display *display,
    Window window,
    XTextProperty *textPropReturn)
{
    return XGetTextProperty(display, window, textPropReturn,
	    XA_WM_ICON_NAME);
}

Status
XGetWMClientMachine(
    Display *display,
    Window window,
    XTextProperty *textPropReturn)
{
    return XGetTextProperty(display, window, textPropReturn,
	    XA_WM_CLIENT_MACHINE);
}

void
XSetTextProperty(
    Display *display,
    Window window,
    XTextProperty *textProp,
    Atom property)
{
    if ((textProp == NULL) || (textProp->value == NULL)) {
	(void)XDeleteProperty(display, window, property);
	return;
    }
    (void)XChangeProperty(display, window, property, textProp->encoding,
	    textProp->format, PropModeReplace, textProp->value,
	    (int)textProp->nitems);
}

void
XSetWMName(
    Display *display,
    Window window,
    XTextProperty *textProp)
{
    XSetTextProperty(display, window, textProp, XA_WM_NAME);
}

void
XSetWMIconName(
    Display *display,
    Window window,
    XTextProperty *textProp)
{
    XSetTextProperty(display, window, textProp, XA_WM_ICON_NAME);
}

Status
XStringListToTextProperty(
    char **list,
    int count,
    XTextProperty *textPropReturn)
{
    return (BrowserRawTextListToTextProperty(list, count, XA_STRING,
	    textPropReturn) == Success) ? True : False;
}

int
XChangeProperty(
    Display *display,
    Window window,
    Atom property,
    Atom type,
    int format,
    int mode,
    _Xconst unsigned char *data,
    int nelements)
{
    X11WasmResource *resourcePtr;
    X11WasmProperty *propertyPtr;
    int changed;

    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_CHANGE_PROPERTY);
    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserPropertyAtomOrError(&display, property,
	    X11_WASM_XREQ_CHANGE_PROPERTY)
	    || !BrowserPropertyAtomOrError(&display, type,
		    X11_WASM_XREQ_CHANGE_PROPERTY)) {
	return Success;
    }
    if (nelements < 0) {
	(void)BrowserPropertyValueOrError(&display, (XID)nelements,
		X11_WASM_XREQ_CHANGE_PROPERTY);
	return Success;
    }
    propertyPtr = BrowserFindProperty(resourcePtr, property);
    if (!BrowserPropertyChangeOrError(&display, propertyPtr, property, type,
	    format, mode, X11_WASM_XREQ_CHANGE_PROPERTY)) {
	return Success;
    }
    if ((BrowserPropertyByteLength(format, nelements) > 0) && (data == NULL)) {
	(void)BrowserPropertyValueOrError(&display, (XID)nelements,
		X11_WASM_XREQ_CHANGE_PROPERTY);
	return Success;
    }
    changed = BrowserSetProperty(resourcePtr, property, type, format, mode,
	    data, nelements);
    if (changed) {
	propertyPtr = BrowserFindProperty(resourcePtr, property);
	BrowserApplyWindowMetadataProperty(window, property, propertyPtr);
	BrowserSyncClipboardProperty(property, propertyPtr);
    }
    return Success;
}

int XDeleteProperty(Display *display, Window window, Atom property)
{
    X11WasmResource *resourcePtr;

    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_DELETE_PROPERTY);
    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserPropertyAtomOrError(&display, property,
	    X11_WASM_XREQ_DELETE_PROPERTY)) {
	return Success;
    }
    if (BrowserDeleteProperty(resourcePtr, property)) {
	BrowserNotifyProperty(resourcePtr, property, PropertyDelete);
	BrowserApplyWindowMetadataProperty(window, property, NULL);
    }
    return Success;
}

Atom *
XListProperties(
    Display *display,
    Window window,
    int *numPropertiesReturn)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_LIST_PROPERTIES);
    X11WasmProperty *propertyPtr;
    Atom *atoms;
    int count = 0;
    int index = 0;

    if (numPropertiesReturn != NULL) {
	*numPropertiesReturn = 0;
    }
    if (resourcePtr == NULL) {
	return NULL;
    }
    for (propertyPtr = resourcePtr->propertyList; propertyPtr != NULL;
	    propertyPtr = propertyPtr->nextPtr) {
	count++;
    }
    if (count == 0) {
	return NULL;
    }
    atoms = (Atom *)ckalloc((size_t)count * sizeof(Atom));
    for (propertyPtr = resourcePtr->propertyList; propertyPtr != NULL;
	    propertyPtr = propertyPtr->nextPtr) {
	atoms[index++] = propertyPtr->atom;
    }
    if (numPropertiesReturn != NULL) {
	*numPropertiesReturn = count;
    }
    return atoms;
}

int
XGetWindowProperty(
    Display *display,
    Window window,
    Atom property,
    long offset,
    long length,
    Bool deleteProperty,
    Atom reqType,
    Atom *actualTypeReturn,
    int *actualFormatReturn,
    unsigned long *nitemsReturn,
    unsigned long *bytesAfterReturn,
    unsigned char **propReturn)
{
    X11WasmResource *resourcePtr;
    X11WasmProperty *propertyPtr;
    unsigned char *data;
    int elementBytes;
    int copyBytes = 0;
    int startBytes = 0;
    unsigned long bytesAfter = 0;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (actualTypeReturn) *actualTypeReturn = None;
    if (actualFormatReturn) *actualFormatReturn = 0;
    if (nitemsReturn) *nitemsReturn = 0;
    if (bytesAfterReturn) *bytesAfterReturn = 0;
    if (propReturn) {
	*propReturn = NULL;
    }

    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_GET_PROPERTY);
    if (resourcePtr == NULL) {
	return BadWindow;
    }
    if (!BrowserPropertyAtomOrError(&display, property,
	    X11_WASM_XREQ_GET_PROPERTY)
	    || ((reqType != AnyPropertyType)
		    && !BrowserPropertyAtomOrError(&display, reqType,
			    X11_WASM_XREQ_GET_PROPERTY))) {
	return BadAtom;
    }
    if (offset < 0) {
	(void)BrowserPropertyValueOrError(&display, (XID)offset,
		X11_WASM_XREQ_GET_PROPERTY);
	return BadValue;
    }
    if (length < 0) {
	(void)BrowserPropertyValueOrError(&display, (XID)length,
		X11_WASM_XREQ_GET_PROPERTY);
	return BadValue;
    }
    propertyPtr = BrowserFindProperty(resourcePtr, property);
    if (propReturn) {
	*propReturn = (unsigned char *)ckalloc(1);
	(*propReturn)[0] = '\0';
    }

    if (propertyPtr == NULL) {
	return Success;
    }

    if (actualTypeReturn) *actualTypeReturn = propertyPtr->type;
    if (actualFormatReturn) *actualFormatReturn = propertyPtr->format;
    if ((reqType != AnyPropertyType) && (reqType != propertyPtr->type)) {
	if (bytesAfterReturn) {
	    *bytesAfterReturn = (unsigned long)propertyPtr->dataLength;
	}
	return Success;
    }
    if (offset > (long)(propertyPtr->dataLength / 4)) {
	return Success;
    }

    elementBytes = BrowserPropertyElementBytes(propertyPtr->format);
    startBytes = (int)(offset * 4L);
    if (startBytes < propertyPtr->dataLength) {
	long requestedBytes = length * 4L;
	long availableBytes = propertyPtr->dataLength - startBytes;

	if (requestedBytes <= 0) {
	    copyBytes = 0;
	} else if (requestedBytes < availableBytes) {
	    copyBytes = (int)requestedBytes;
	} else {
	    copyBytes = (int)availableBytes;
	}
	if (elementBytes > 0) {
	    copyBytes -= copyBytes % elementBytes;
	}
    }
    bytesAfter = (unsigned long)(propertyPtr->dataLength - startBytes
	    - copyBytes);
    if (bytesAfterReturn) {
	*bytesAfterReturn = bytesAfter;
    }
    if (nitemsReturn && (elementBytes > 0)) {
	*nitemsReturn = (unsigned long)(copyBytes / elementBytes);
    }
    if (propReturn) {
	ckfree(*propReturn);
	data = (unsigned char *)ckalloc((size_t)copyBytes + 1U);
	if (copyBytes > 0) {
	    memcpy(data, propertyPtr->data + startBytes, (size_t)copyBytes);
	}
	data[copyBytes] = '\0';
	*propReturn = data;
    }
    if (deleteProperty && (bytesAfter == 0UL)) {
	if (BrowserDeleteProperty(resourcePtr, property)) {
	    BrowserNotifyProperty(resourcePtr, property, PropertyDelete);
	    BrowserApplyWindowMetadataProperty(window, property, NULL);
	}
    }
    return Success;
}

int
XConvertSelection(
    Display *display,
    Atom selection,
    Atom target,
    Atom property,
    Window requestor,
    Time time)
{
    Window owner;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (!BrowserPropertyAtomOrError(&display, selection,
	    X11_WASM_XREQ_CONVERT_SELECTION)
	    || !BrowserPropertyAtomOrError(&display, target,
		    X11_WASM_XREQ_CONVERT_SELECTION)
	    || !BrowserOptionalPropertyAtomOrError(&display, property,
		    X11_WASM_XREQ_CONVERT_SELECTION)) {
	return Success;
    }
    if (BrowserWindowResourceOrError(&display, requestor,
	    X11_WASM_XREQ_CONVERT_SELECTION) == NULL) {
	return Success;
    }
    owner = BrowserSelectionOwnerWindow(selection);
    if (owner == None) {
	if (BrowserConvertExternalClipboardSelection(requestor, selection,
		target, property, time)) {
	    return Success;
	}
	(void)BrowserQueueSelectionNotify(requestor, selection, target, None,
		time);
	return Success;
    }
    if (!BrowserQueueSelectionRequest(owner, requestor, selection, target,
	    property, time)) {
	(void)BrowserQueueSelectionNotify(requestor, selection, target, None,
		time);
    }
    return Success;
}

Window
XGetSelectionOwner(
    Display *display,
    Atom selection)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (!BrowserPropertyAtomOrError(&display, selection,
	    X11_WASM_XREQ_GET_SELECTION_OWNER)) {
	return None;
    }
    return BrowserSelectionOwnerWindow(selection);
}

int
XSetSelectionOwner(
    Display *display,
    Atom selection,
    Window owner,
    Time time)
{
    X11WasmSelectionOwner *ownerPtr;
    Window oldOwner;
    unsigned long serial;
    Time effectiveTime;

    BrowserInitDisplay();
    if (display == NULL) {
	display = &browserDisplay.display;
    }
    if (!BrowserPropertyAtomOrError(&display, selection,
	    X11_WASM_XREQ_SET_SELECTION_OWNER)) {
	return Success;
    }
    if ((owner != None) && (BrowserWindowResourceOrError(&display, owner,
	    X11_WASM_XREQ_SET_SELECTION_OWNER) == NULL)) {
	return Success;
    }
    serial = ++LastKnownRequestProcessed(display);
    BrowserSyncRequestState(display);
    effectiveTime = BrowserSelectionEventTime(time);
    ownerPtr = BrowserFindSelectionOwner(selection);
    oldOwner = (ownerPtr != NULL) ? ownerPtr->owner : None;
    if ((effectiveTime > eventTime)
	    || ((ownerPtr != NULL) && (effectiveTime < ownerPtr->time))) {
	return Success;
    }

    if (owner == None) {
	if (oldOwner != None) {
	    (void)BrowserQueueSelectionClear(oldOwner, selection, effectiveTime,
		    serial);
	}
	BrowserDeleteSelectionOwner(selection);
	BrowserSyncSelectionOwner(selection, None, effectiveTime);
	return Success;
    }
    ownerPtr = BrowserCreateSelectionOwner(selection);
    if (ownerPtr == NULL) {
	return Success;
    }
    ownerPtr->owner = owner;
    ownerPtr->time = effectiveTime;

    if ((oldOwner != None) && (oldOwner != owner)) {
	(void)BrowserQueueSelectionClear(oldOwner, selection, effectiveTime,
		serial);
    }
    BrowserSyncSelectionOwner(selection, owner, effectiveTime);
    return Success;
}

int
BrowserTestExternalClipboardTarget(
    Atom target,
    Atom expectedType,
    const unsigned char *expectedBytes,
    unsigned long expectedByteCount,
    const char *expectedUtf8)
{
    Atom actualType = None;
    Atom property;
    XEvent event;
    XTextProperty textProp;
    unsigned char *data = NULL;
    char **textList = NULL;
    int textCount = -1;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    int ok = 0;

    property = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_TARGET", False);
    BrowserClearEventQueue();
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    property);
    (void)XConvertSelection(&browserDisplay.display,
	    BrowserClipboardSelectionAtom(), target, property,
	    browserDisplay.rootWindow, CurrentTime);
    if (!BrowserPopEvent(&event, NULL)
	    || (event.type != SelectionNotify)
	    || (event.xselection.selection != BrowserClipboardSelectionAtom())
	    || (event.xselection.target != target)
	    || (event.xselection.property != property)) {
	goto cleanup;
    }
    if (XGetWindowProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    property, 0, 4096, False, AnyPropertyType, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data) != Success) {
	goto cleanup;
    }
    if ((actualType != expectedType) || (actualFormat != 8)
	    || (bytesAfter != 0)) {
	goto cleanup;
    }
    if (expectedBytes != NULL) {
	if ((nitems != expectedByteCount)
		|| ((expectedByteCount > 0)
			&& (memcmp(data, expectedBytes,
				(size_t)expectedByteCount) != 0))) {
	    goto cleanup;
	}
    }
    if (expectedUtf8 != NULL) {
	memset(&textProp, 0, sizeof(textProp));
	textProp.value = data;
	textProp.encoding = actualType;
	textProp.format = actualFormat;
	textProp.nitems = nitems;
	if ((Xutf8TextPropertyToTextList(&browserDisplay.display, &textProp,
		&textList, &textCount) != Success)
		|| (textCount != 1) || (textList == NULL)
		|| (textList[0] == NULL)
		|| (strcmp(textList[0], expectedUtf8) != 0)) {
	    goto cleanup;
	}
    }
    ok = 1;

cleanup:
    if (textList != NULL) {
	XFreeStringList(textList);
    }
    if (data != NULL) {
	XFree(data);
    }
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    property);
    return ok;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestExternalClipboardTargets(void)
{
    static const char latin1Utf8[] = "caf\303\251";
    static const unsigned char latin1String[] = {'c', 'a', 'f', 0xe9};
    static const unsigned char latin1Utf8Bytes[] = {
	'c', 'a', 'f', 0xc3, 0xa9
    };
    static const char unicodeUtf8[] = "kana \343\201\202";
    int result = 1;

    BrowserInitDisplay();
    (void)XSetSelectionOwner(&browserDisplay.display,
	    BrowserClipboardSelectionAtom(), None, CurrentTime);
    BrowserSetExternalClipboardText(latin1Utf8, -1);
    if (!BrowserTestExternalClipboardTarget(XA_STRING, XA_STRING,
	    latin1String, (unsigned long)sizeof(latin1String), latin1Utf8)) {
	result = 10;
	goto cleanup;
    }
    if (!BrowserTestExternalClipboardTarget(BrowserUtf8StringAtom(),
	    BrowserUtf8StringAtom(), latin1Utf8Bytes,
	    (unsigned long)sizeof(latin1Utf8Bytes), latin1Utf8)) {
	result = 11;
	goto cleanup;
    }
    if (!BrowserTestExternalClipboardTarget(BrowserTextAtom(), XA_STRING,
	    latin1String, (unsigned long)sizeof(latin1String), latin1Utf8)) {
	result = 12;
	goto cleanup;
    }
    if (!BrowserTestExternalClipboardTarget(BrowserCompoundTextAtom(),
	    BrowserCompoundTextAtom(), NULL, 0, latin1Utf8)) {
	result = 13;
	goto cleanup;
    }

    BrowserSetExternalClipboardText(unicodeUtf8, -1);
    if (!BrowserTestExternalClipboardTarget(BrowserTextAtom(),
	    BrowserCompoundTextAtom(), NULL, 0, unicodeUtf8)) {
	result = 14;
	goto cleanup;
    }
    if (!BrowserTestExternalClipboardTarget(XA_STRING,
	    BrowserUtf8StringAtom(), (const unsigned char *)unicodeUtf8,
	    (unsigned long)strlen(unicodeUtf8), unicodeUtf8)) {
	result = 15;
	goto cleanup;
    }

cleanup:
    BrowserClearExternalClipboardText();
    BrowserClearEventQueue();
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetClipboardText(int clear)
{
    static const char clipboardText[] = "x11.wasm clipboard api";

    return X11WasmSetClipboardText(clear ? "" : clipboardText, clear ? 0 : -1);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSyncClipboardSelectionProperty(void)
{
    static const char clipboardText[] = "x11.wasm selection clipboard";
    Atom clipboardAtom;

    BrowserInitDisplay();
    clipboardAtom = BrowserClipboardSelectionAtom();
    if (XSetSelectionOwner(&browserDisplay.display, clipboardAtom,
	    browserDisplay.rootWindow, CurrentTime) != Success) {
	return 0;
    }
    if (XChangeProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    clipboardAtom, BrowserUtf8StringAtom(), 8, PropModeReplace,
	    (const unsigned char *)clipboardText,
	    (int)strlen(clipboardText)) != Success) {
	return 0;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetPrimarySelectionOwner(unsigned int window, int enabled)
{
    BrowserInitDisplay();
    return XSetSelectionOwner(&browserDisplay.display, XA_PRIMARY,
	    enabled ? (Window)window : None, CurrentTime);
}

int XSetWindowBackground(Display *display, Window window, unsigned long pixel)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);

    if (resourcePtr != NULL) {
	if (resourcePtr->c_class == InputOnly) {
	    (void)BrowserBadMatchOrError(&display, (XID)window,
		    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);
	    return Success;
	}
	resourcePtr->atts.background_pixmap = None;
	resourcePtr->atts.background_pixel = pixel;
	resourcePtr->hasBackgroundPixel = 1;
	BrowserSyncWindowStyle(resourcePtr);
	BrowserSyncInheritedChildStyles(resourcePtr);
    }
    return Success;
}
int XSetWindowBackgroundPixmap(Display *display, Window window, Pixmap pixmap)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);

    if (resourcePtr != NULL) {
	if (resourcePtr->c_class == InputOnly) {
	    (void)BrowserBadMatchOrError(&display, (XID)window,
		    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);
	    return Success;
	}
	if (BrowserValidateWindowPixmapAttribute(&display,
		    resourcePtr->depth, pixmap,
		    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES, 1)) {
	    resourcePtr->atts.background_pixmap = pixmap;
	    resourcePtr->hasBackgroundPixel = 0;
	    BrowserSyncWindowStyle(resourcePtr);
	    BrowserSyncInheritedChildStyles(resourcePtr);
	}
    }
    return Success;
}
int XSetWindowBorder(Display *display, Window window, unsigned long pixel)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);

    if (resourcePtr != NULL) {
	if (resourcePtr->c_class == InputOnly) {
	    (void)BrowserBadMatchOrError(&display, (XID)window,
		    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);
	    return Success;
	}
	resourcePtr->atts.border_pixmap = None;
	resourcePtr->atts.border_pixel = pixel;
	resourcePtr->hasBorderPixel = 1;
	BrowserSyncWindowStyle(resourcePtr);
	BrowserSyncInheritedChildStyles(resourcePtr);
    }
    return Success;
}
int XSetWindowBorderPixmap(Display *display, Window window, Pixmap pixmap)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);

    if (resourcePtr != NULL) {
	if (resourcePtr->c_class == InputOnly) {
	    (void)BrowserBadMatchOrError(&display, (XID)window,
		    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);
	    return Success;
	}
	if (BrowserValidateWindowPixmapAttribute(&display,
		    resourcePtr->depth, pixmap,
		    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES, 0)) {
	    resourcePtr->atts.border_pixmap = pixmap;
	    resourcePtr->hasBorderPixel = 0;
	    BrowserSyncWindowStyle(resourcePtr);
	    BrowserSyncInheritedChildStyles(resourcePtr);
	}
    }
    return Success;
}
int XSetWindowBorderWidth(Display *display, Window window, unsigned int width)
{
    XWindowChanges changes;

    changes.border_width = (int)width;
    return XConfigureWindow(display, window, CWBorderWidth, &changes);
}
int XSetWindowColormap(Display *display, Window window, Colormap colormap)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);

    if (resourcePtr != NULL) {
	if (resourcePtr->c_class == InputOnly) {
	    (void)BrowserBadMatchOrError(&display, (XID)window,
		    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);
	    return Success;
	}
	if (BrowserColormapOrError(&display, colormap,
		    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES, 1)) {
	    Colormap oldColormap = resourcePtr->atts.colormap;

	    resourcePtr->atts.colormap = colormap;
	    BrowserSyncWindowColormap(resourcePtr);
	    BrowserNotifyWindowColormap(resourcePtr, oldColormap);
	}
    }
    return Success;
}
int XChangeWindowAttributes(Display *display, Window window, unsigned long mask, XSetWindowAttributes *atts)
{
    X11WasmResource *r = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);
    Colormap oldColormap = None;

    if (r == NULL) {
	return Success;
    }
    if (!BrowserValidateInputOnlyChangeWindowAttributes(&display, r, mask,
	    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES)) {
	return Success;
    }
    if (!BrowserValidateWindowAttributeValues(&display, r->depth, mask, atts,
	    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES)) {
	return Success;
    }
    if (mask & CWColormap) {
	oldColormap = r->atts.colormap;
    }
    BrowserApplyWindowAttributes(r, mask, atts);
    if (mask & (CWEventMask | CWDontPropagate)) {
	BrowserSyncWindowEventMask(r);
    }
    if (mask & CWColormap) {
	BrowserSyncWindowColormap(r);
	BrowserNotifyWindowColormap(r, oldColormap);
    }
    if (mask & (CWBitGravity | CWWinGravity | CWBackingStore
	    | CWBackingPlanes | CWBackingPixel | CWOverrideRedirect
	    | CWSaveUnder)) {
	BrowserSyncWindowAttributes(r);
    }
    BrowserSyncWindowStyle(r);
    if (mask & (CWBackPixmap | CWBackPixel | CWBorderPixmap | CWBorderPixel)) {
	BrowserSyncInheritedChildStyles(r);
    }
    return Success;
}
int XSelectInput(Display *display, Window window, long mask)
{
    X11WasmResource *r = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES);

    if (r != NULL) {
	if (!BrowserValidateWindowEventMask(&display, mask,
		(unsigned long)X11_WASM_WINDOW_EVENT_MASK,
		X11_WASM_XREQ_CHANGE_WINDOW_ATTRIBUTES)) {
	    return Success;
	}
	r->atts.event_mask = mask;
	BrowserSyncWindowEventMask(r);
    }
    return Success;
}

int XRaiseWindow(Display *display, Window window)
{
    XWindowChanges changes;
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CONFIGURE_WINDOW);

    if (resourcePtr == NULL) {
	return Success;
    }
    changes.stack_mode = Above;
    BrowserRestackWindow(resourcePtr, CWStackMode, &changes);
    return Success;
}
int XLowerWindow(Display *display, Window window)
{
    XWindowChanges changes;
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CONFIGURE_WINDOW);

    if (resourcePtr == NULL) {
	return Success;
    }
    changes.stack_mode = Below;
    BrowserRestackWindow(resourcePtr, CWStackMode, &changes);
    return Success;
}
int
XReparentWindow(
    Display *display,
    Window window,
    Window parent,
    int x,
    int y)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_REPARENT_WINDOW);
    XEvent event;

    if (resourcePtr == NULL) {
	return Success;
    }
    if (BrowserWindowResourceOrError(&display, parent,
	    X11_WASM_XREQ_REPARENT_WINDOW) == NULL) {
	return Success;
    }
    BrowserUnlinkChild(resourcePtr);
    resourcePtr->parent = parent;
    resourcePtr->x = x;
    resourcePtr->y = y;
    BrowserLinkChildAtTop(resourcePtr, parent);
    (void)X11WasmJsReparentWindow(BrowserJsXId(window),
	    BrowserJsXId(parent), x, y);
    BrowserSyncTransientStacking();
    BrowserSetCommonEventFields(&event, ReparentNotify, window,
	    CurrentTime);
    event.xreparent.event = window;
    event.xreparent.window = window;
    event.xreparent.parent = parent;
    event.xreparent.x = x;
    event.xreparent.y = y;
    event.xreparent.override_redirect =
	    resourcePtr->atts.override_redirect;
    (void)BrowserQueueEvent(&event);
    return Success;
}
Status
XIconifyWindow(
    Display *display,
    Window window,
    int screen)
{
    Status status;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserScreenOrError(display, screen, X11_WASM_XREQ_UNMAP_WINDOW)) {
	return False;
    }
    status = (XUnmapWindow(display, window) == Success) ? True : False;
    if (status) {
	BrowserSetWindowWmState(window, IconicState);
    }
    return status;
}

Status
XWithdrawWindow(
    Display *display,
    Window window,
    int screen)
{
    Status status;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserScreenOrError(display, screen, X11_WASM_XREQ_UNMAP_WINDOW)) {
	return False;
    }
    status = (XUnmapWindow(display, window) == Success) ? True : False;
    if (status) {
	BrowserSetWindowWmState(window, WithdrawnState);
    }
    return status;
}
int
XClearArea(
    Display *display,
    Window window,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    Bool exposures)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CLEAR_AREA);
    unsigned int clearWidth;
    unsigned int clearHeight;

    if (resourcePtr == NULL) {
	return Success;
    }
    if (resourcePtr->c_class == InputOnly) {
	(void)BrowserBadMatchOrError(&display, (XID)window,
		X11_WASM_XREQ_CLEAR_AREA);
	return Success;
    }
    clearWidth = BrowserClearAreaExtent(x, width, resourcePtr->width);
    clearHeight = BrowserClearAreaExtent(y, height, resourcePtr->height);
    if ((clearWidth == 0U) || (clearHeight == 0U)) {
	return Success;
    }
    (void)X11WasmJsClearArea(BrowserJsXId(window), x, y, clearWidth,
	    clearHeight);
    if (exposures) {
	(void)BrowserQueueExpose(resourcePtr, x, y, clearWidth, clearHeight);
    } else {
	BrowserMarkResourceDamage(resourcePtr, x, y, clearWidth, clearHeight);
    }
    return Success;
}

int
XClearWindow(
    Display *display,
    Window window)
{
    return XClearArea(display, window, 0, 0, 0, 0, False);
}

XHostAddress *
XListHosts(
    Display *display,
    int *nhosts,
    Bool *state)
{
    XHostAddress *hosts;
    char *addressPtr;
    size_t addressBytes = 0;
    size_t headerBytes;
    int i;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (nhosts != NULL) {
	*nhosts = browserDisplay.hostCount;
    }
    if (state != NULL) {
	*state = browserDisplay.hostAccessEnabled ? True : False;
    }
    (void)XNoOp(display);
    if (browserDisplay.hostCount == 0) {
	return NULL;
    }
    for (i = 0; i < browserDisplay.hostCount; i++) {
	addressBytes += (size_t)browserDisplay.hosts[i].length;
    }
    headerBytes = sizeof(XHostAddress) * (size_t)browserDisplay.hostCount;
    hosts = (XHostAddress *)ckalloc(headerBytes + addressBytes);
    addressPtr = ((char *)hosts) + headerBytes;
    for (i = 0; i < browserDisplay.hostCount; i++) {
	hosts[i].family = browserDisplay.hosts[i].family;
	hosts[i].length = browserDisplay.hosts[i].length;
	if (hosts[i].length > 0) {
	    hosts[i].address = addressPtr;
	    memcpy(addressPtr, browserDisplay.hosts[i].address,
		    (size_t)hosts[i].length);
	    addressPtr += hosts[i].length;
	} else {
	    hosts[i].address = NULL;
	}
    }
    return hosts;
}

int
XAddHosts(
    Display *display,
    XHostAddress *hosts,
    int nhosts)
{
    int i;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (!BrowserValueOrError(&display, nhosts, nhosts >= 0,
	    X11_WASM_XREQ_CHANGE_HOSTS)
	    || ((nhosts > 0) && !BrowserValueOrError(&display, 0,
		    hosts != NULL, X11_WASM_XREQ_CHANGE_HOSTS))) {
	return Success;
    }
    for (i = 0; i < nhosts; i++) {
	if (!BrowserValidateHostAddress(&display, &hosts[i],
		X11_WASM_XREQ_CHANGE_HOSTS)) {
	    return Success;
	}
    }
    if (display == &browserDisplay.display) {
	for (i = 0; i < nhosts; i++) {
	    BrowserRememberHostAddress(&hosts[i]);
	}
    }
    return XNoOp(display);
}

int
XAddHost(
    Display *display,
    XHostAddress *host)
{
    return XAddHosts(display, host, 1);
}

int
XRemoveHosts(
    Display *display,
    XHostAddress *hosts,
    int nhosts)
{
    int i;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (!BrowserValueOrError(&display, nhosts, nhosts >= 0,
	    X11_WASM_XREQ_CHANGE_HOSTS)
	    || ((nhosts > 0) && !BrowserValueOrError(&display, 0,
		    hosts != NULL, X11_WASM_XREQ_CHANGE_HOSTS))) {
	return Success;
    }
    for (i = 0; i < nhosts; i++) {
	if (!BrowserValidateHostAddress(&display, &hosts[i],
		X11_WASM_XREQ_CHANGE_HOSTS)) {
	    return Success;
	}
    }
    if (display == &browserDisplay.display) {
	for (i = 0; i < nhosts; i++) {
	    BrowserForgetHostAddress(&hosts[i]);
	}
    }
    return XNoOp(display);
}

int
XRemoveHost(
    Display *display,
    XHostAddress *host)
{
    return XRemoveHosts(display, host, 1);
}

int
XSetAccessControl(
    Display *display,
    int mode)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserInitDisplay();
    if (!BrowserValueOrError(&display, mode,
	    (mode == EnableAccess) || (mode == DisableAccess),
	    X11_WASM_XREQ_SET_ACCESS_CONTROL)) {
	return Success;
    }
    if (display == &browserDisplay.display) {
	browserDisplay.hostAccessEnabled = (mode == EnableAccess);
	BrowserSyncHostAccessControl();
    }
    return XNoOp(display);
}

int
XEnableAccessControl(
    Display *display)
{
    return XSetAccessControl(display, EnableAccess);
}

int
XDisableAccessControl(
    Display *display)
{
    return XSetAccessControl(display, DisableAccess);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetHostAccessControl(int configured)
{
    unsigned char hostAddressOne[4] = {192, 0, 2, 33};
    unsigned char hostAddressTwo[16] = {
	0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42
    };
    XHostAddress hosts[2];
    int result;

    BrowserInitDisplay();
    BrowserResetHostAccessControl();
    if (!configured) {
	return Success;
    }

    memset(hosts, 0, sizeof(hosts));
    hosts[0].family = FamilyInternet;
    hosts[0].length = (int)sizeof(hostAddressOne);
    hosts[0].address = (char *)hostAddressOne;
    hosts[1].family = FamilyInternet6;
    hosts[1].length = (int)sizeof(hostAddressTwo);
    hosts[1].address = (char *)hostAddressTwo;

    result = XDisableAccessControl(&browserDisplay.display);
    if (result != Success) {
	return result;
    }
    return XAddHosts(&browserDisplay.display, hosts, 2);
}

GContext XGContextFromGC(GC gc)
{ return (GContext)(uintptr_t)gc; }
int
XFreeColors(
    Display *display,
    Colormap colormap,
    unsigned long *pixels,
    int npixels,
    unsigned long planes)
{
    (void)planes;
    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_FREE_COLORS, 0)) {
	return Success;
    }
    BrowserForgetColors(colormap, pixels, npixels);
    return Success;
}
