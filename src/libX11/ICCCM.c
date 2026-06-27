#include "XlibInt.h"

#define X11_WASM_CUT_BUFFER_COUNT 8
#define X11_WASM_OLD_SIZE_HINTS_ELEMENTS 15UL
#define X11_WASM_SIZE_HINTS_ELEMENTS ((unsigned long)(sizeof(XSizeHints) / 4U))
#define X11_WASM_OLD_SIZE_HINTS_SUPPLIED \
	(USPosition | USSize | PAllHints)
#define X11_WASM_SIZE_HINTS_SUPPLIED \
	(X11_WASM_OLD_SIZE_HINTS_SUPPLIED | PBaseSize | PWinGravity)

typedef struct BrowserPropertySnapshot {
    Atom type;
    int format;
    unsigned long nitems;
    int dataLength;
    unsigned char *data;
} BrowserPropertySnapshot;

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
	    XA_WINDOW, 32, PropModeReplace,
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

EMSCRIPTEN_KEEPALIVE unsigned int
X11WasmTestWindowManagerStateTransition(int state)
{
    Display *display;
    Window window = None;
    XWindowAttributes attributes;
    XEvent event;
    Status status;
    unsigned int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    window = XCreateWindow(display, browserDisplay.rootWindow, 14, 18, 64,
	    42, 0, CopyFromParent, InputOutput, NULL, 0, NULL);
    if (window == None) {
	goto cleanup;
    }
    if ((XSelectInput(display, window, StructureNotifyMask) != Success)
	    || (XMapWindow(display, window) != Success)) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if (!BrowserPopMatchingEvent(window, StructureNotifyMask, &event)
	    || (event.type != MapNotify)
	    || (event.xmap.window != window)
	    || (event.xmap.event != window)
	    || !XGetWindowAttributes(display, window, &attributes)
	    || (attributes.map_state != IsViewable)
	    || BrowserPopMatchingEvent(window, StructureNotifyMask, &event)) {
	goto cleanup;
    }

    if (state == IconicState) {
	status = XIconifyWindow(display, window, DefaultScreen(display));
    } else if (state == WithdrawnState) {
	status = XWithdrawWindow(display, window, DefaultScreen(display));
    } else {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if ((status != True)
	    || !BrowserPopMatchingEvent(window, StructureNotifyMask, &event)
	    || (event.type != UnmapNotify)
	    || (event.xunmap.window != window)
	    || (event.xunmap.event != window)
	    || (event.xunmap.from_configure != False)
	    || !XGetWindowAttributes(display, window, &attributes)
	    || (attributes.map_state != IsUnmapped)
	    || BrowserPopMatchingEvent(window, StructureNotifyMask, &event)) {
	goto cleanup;
    }
    result = (unsigned int)BrowserJsXId(window);
    window = None;

cleanup:
    BrowserClearEventQueue();
    if (window != None) {
	(void)XDestroyWindow(display, window);
    }
    BrowserClearEventQueue();
    return result;
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
X11WasmTestRotateWindowNameProperties(unsigned int window)
{
    Display *display;
    Window xid = (Window)window;
    Atom properties[2];
    XTextProperty title;
    XTextProperty iconName;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    memset(&title, 0, sizeof(title));
    memset(&iconName, 0, sizeof(iconName));
    if (BrowserWindowResource(xid) == NULL) {
	return 0;
    }
    properties[0] = XA_WM_NAME;
    properties[1] = XA_WM_ICON_NAME;

    if ((XStoreName(display, xid, "Rotated Title") != Success)
	    || (XSetIconName(display, xid, "Rotated Icon") != Success)
	    || (XRotateWindowProperties(display, xid, properties, 2, 1)
		    != Success)
	    || !XGetWMName(display, xid, &title)
	    || !XGetWMIconName(display, xid, &iconName)
	    || (title.value == NULL)
	    || (iconName.value == NULL)
	    || (strcmp((const char *)title.value, "Rotated Icon") != 0)
	    || (strcmp((const char *)iconName.value, "Rotated Title") != 0)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (title.value != NULL) {
	XFree(title.value);
    }
    if (iconName.value != NULL) {
	XFree(iconName.value);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGetWindowNamePropertiesDeletes(unsigned int window)
{
    static const char titleValue[] = "Deleted Title";
    static const char iconNameValue[] = "Deleted Icon";
    Display *display;
    Window xid = (Window)window;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *titleData = NULL;
    unsigned char *iconNameData = NULL;
    char *fetchedTitle = NULL;
    char *fetchedIconName = NULL;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    if (BrowserWindowResource(xid) == NULL) {
	return 0;
    }

    if ((XStoreName(display, xid, titleValue) != Success)
	    || (XSetIconName(display, xid, iconNameValue) != Success)) {
	goto cleanup;
    }
    if ((XGetWindowProperty(display, xid, XA_WM_NAME, 0,
		    (long)(LONG_MAX / 4L), True, AnyPropertyType, &actualType,
		    &actualFormat, &nitems, &bytesAfter, &titleData) != Success)
	    || (actualType != XA_STRING)
	    || (actualFormat != 8)
	    || (nitems != strlen(titleValue))
	    || (bytesAfter != 0)
	    || (titleData == NULL)
	    || (memcmp(titleData, titleValue, strlen(titleValue)) != 0)
	    || (titleData[nitems] != '\0')) {
	goto cleanup;
    }
    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, XA_WM_ICON_NAME, 0,
		    (long)(LONG_MAX / 4L), True, AnyPropertyType, &actualType,
		    &actualFormat, &nitems, &bytesAfter, &iconNameData) != Success)
	    || (actualType != XA_STRING)
	    || (actualFormat != 8)
	    || (nitems != strlen(iconNameValue))
	    || (bytesAfter != 0)
	    || (iconNameData == NULL)
	    || (memcmp(iconNameData, iconNameValue, strlen(iconNameValue)) != 0)
	    || (iconNameData[nitems] != '\0')) {
	goto cleanup;
    }
    if (XFetchName(display, xid, &fetchedTitle)
	    || (fetchedTitle != NULL)
	    || XGetIconName(display, xid, &fetchedIconName)
	    || (fetchedIconName != NULL)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (titleData != NULL) {
	XFree(titleData);
    }
    if (iconNameData != NULL) {
	XFree(iconNameData);
    }
    if (fetchedTitle != NULL) {
	XFree(fetchedTitle);
    }
    if (fetchedIconName != NULL) {
	XFree(fetchedIconName);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGetWindowMetadataPropertiesDeletes(unsigned int window)
{
    static char *commandArgv[] = {
	(char *)"x11wasm-test", (char *)"--delete", NULL
    };
    static const unsigned char expectedCommand[] =
	    "x11wasm-test\0--delete";
    static const unsigned char expectedClass[] =
	    "delete-class\0X11WasmDeleteClass";
    static const char clientMachineValue[] = "delete-client";
    Display *display;
    Window xid = (Window)window;
    XClassHint classHint;
    XClassHint fetchedClassHint;
    XTextProperty clientMachine;
    XTextProperty fetchedClientMachine;
    Window fetchedTransientFor = None;
    char **fetchedCommandArgv = NULL;
    int fetchedCommandArgc = -1;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *commandData = NULL;
    unsigned char *classData = NULL;
    unsigned char *clientMachineData = NULL;
    unsigned char *transientForData = NULL;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    if (BrowserWindowResource(xid) == NULL) {
	return 0;
    }

    classHint.res_name = (char *)"delete-class";
    classHint.res_class = (char *)"X11WasmDeleteClass";
    memset(&fetchedClassHint, 0, sizeof(fetchedClassHint));
    memset(&clientMachine, 0, sizeof(clientMachine));
    memset(&fetchedClientMachine, 0, sizeof(fetchedClientMachine));
    clientMachine.value = (unsigned char *)clientMachineValue;
    clientMachine.encoding = XA_STRING;
    clientMachine.format = 8;
    clientMachine.nitems = strlen(clientMachineValue);

    if ((XSetCommand(display, xid, commandArgv, 2) != Success)
	    || (XSetClassHint(display, xid, &classHint) != Success)
	    || (XSetTransientForHint(display, xid, browserDisplay.rootWindow)
		    != Success)) {
	goto cleanup;
    }
    XSetWMClientMachine(display, xid, &clientMachine);

    if ((XGetWindowProperty(display, xid, XA_WM_COMMAND, 0,
		    (long)(LONG_MAX / 4L), True, XA_STRING, &actualType,
		    &actualFormat, &nitems, &bytesAfter,
		    &commandData) != Success)
	    || (actualType != XA_STRING)
	    || (actualFormat != 8)
	    || (nitems != sizeof(expectedCommand))
	    || (bytesAfter != 0)
	    || (commandData == NULL)
	    || (memcmp(commandData, expectedCommand,
		    sizeof(expectedCommand)) != 0)) {
	goto cleanup;
    }

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, XA_WM_CLASS, 0,
		    (long)(LONG_MAX / 4L), True, XA_STRING, &actualType,
		    &actualFormat, &nitems, &bytesAfter, &classData)
		    != Success)
	    || (actualType != XA_STRING)
	    || (actualFormat != 8)
	    || (nitems != sizeof(expectedClass))
	    || (bytesAfter != 0)
	    || (classData == NULL)
	    || (memcmp(classData, expectedClass, sizeof(expectedClass))
		    != 0)) {
	goto cleanup;
    }

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, XA_WM_CLIENT_MACHINE, 0,
		    (long)(LONG_MAX / 4L), True, XA_STRING, &actualType,
		    &actualFormat, &nitems, &bytesAfter,
		    &clientMachineData) != Success)
	    || (actualType != XA_STRING)
	    || (actualFormat != 8)
	    || (nitems != strlen(clientMachineValue))
	    || (bytesAfter != 0)
	    || (clientMachineData == NULL)
	    || (memcmp(clientMachineData, clientMachineValue,
		    strlen(clientMachineValue)) != 0)) {
	goto cleanup;
    }

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, XA_WM_TRANSIENT_FOR, 0, 1, True,
		    XA_WINDOW, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &transientForData) != Success)
	    || (actualType != XA_WINDOW)
	    || (actualFormat != 32)
	    || (nitems != 1)
	    || (bytesAfter != 0)
	    || (transientForData == NULL)
	    || ((Window)BrowserPropertyAtomAt(transientForData, 0)
		    != browserDisplay.rootWindow)) {
	goto cleanup;
    }

    if (XGetCommand(display, xid, &fetchedCommandArgv, &fetchedCommandArgc)
	    || (fetchedCommandArgv != NULL)
	    || (fetchedCommandArgc != 0)
	    || XGetClassHint(display, xid, &fetchedClassHint)
	    || (fetchedClassHint.res_name != NULL)
	    || (fetchedClassHint.res_class != NULL)
	    || XGetWMClientMachine(display, xid, &fetchedClientMachine)
	    || (fetchedClientMachine.value != NULL)
	    || XGetTransientForHint(display, xid, &fetchedTransientFor)
	    || (fetchedTransientFor != None)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (commandData != NULL) {
	XFree(commandData);
    }
    if (classData != NULL) {
	XFree(classData);
    }
    if (clientMachineData != NULL) {
	XFree(clientMachineData);
    }
    if (transientForData != NULL) {
	XFree(transientForData);
    }
    if (fetchedCommandArgv != NULL) {
	XFreeStringList(fetchedCommandArgv);
    }
    if (fetchedClassHint.res_name != NULL) {
	XFree(fetchedClassHint.res_name);
    }
    if (fetchedClassHint.res_class != NULL) {
	XFree(fetchedClassHint.res_class);
    }
    if (fetchedClientMachine.value != NULL) {
	XFree(fetchedClientMachine.value);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGetWindowWMHintsDeletes(
    unsigned int window,
    unsigned int iconWindow,
    unsigned int groupWindow)
{
    Display *display;
    Window xid = (Window)window;
    XWMHints hints;
    XWMHints fetchedHints;
    XWMHints *remainingHints = NULL;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    if ((BrowserWindowResource(xid) == NULL)
	    || (BrowserWindowResource((Window)iconWindow) == NULL)
	    || (BrowserWindowResource((Window)groupWindow) == NULL)) {
	return 0;
    }

    memset(&hints, 0, sizeof(hints));
    hints.flags = InputHint | StateHint | IconWindowHint | IconPositionHint
	    | WindowGroupHint;
    hints.input = False;
    hints.initial_state = IconicState;
    hints.icon_window = (Window)iconWindow;
    hints.icon_x = 23;
    hints.icon_y = 31;
    hints.window_group = (Window)groupWindow;

    if ((XSetWMHints(display, xid, &hints) != Success)
	    || (XGetWindowProperty(display, xid, XA_WM_HINTS, 0,
		    (long)(LONG_MAX / 4L), True, XA_WM_HINTS, &actualType,
		    &actualFormat, &nitems, &bytesAfter, &data) != Success)
	    || (actualType != XA_WM_HINTS)
	    || (actualFormat != 32)
	    || (nitems != (sizeof(XWMHints) / 4U))
	    || (bytesAfter != 0)
	    || (data == NULL)) {
	goto cleanup;
    }
    memset(&fetchedHints, 0, sizeof(fetchedHints));
    memcpy(&fetchedHints, data, sizeof(fetchedHints));
    if ((fetchedHints.flags != hints.flags)
	    || (fetchedHints.input != hints.input)
	    || (fetchedHints.initial_state != hints.initial_state)
	    || (fetchedHints.icon_window != hints.icon_window)
	    || (fetchedHints.icon_x != hints.icon_x)
	    || (fetchedHints.icon_y != hints.icon_y)
	    || (fetchedHints.window_group != hints.window_group)) {
	goto cleanup;
    }
    remainingHints = XGetWMHints(display, xid);
    if (remainingHints != NULL) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (data != NULL) {
	XFree(data);
    }
    if (remainingHints != NULL) {
	XFree(remainingHints);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGetWindowNormalHintsDeletes(unsigned int window)
{
    Display *display;
    Window xid = (Window)window;
    XSizeHints hints;
    XSizeHints fetchedHints;
    XSizeHints remainingHints;
    long supplied = 0;
    long remainingSupplied = -1;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    if (BrowserWindowResource(xid) == NULL) {
	return 0;
    }

    memset(&hints, 0, sizeof(hints));
    hints.flags = PPosition | PSize | PMinSize | PMaxSize | PResizeInc
	    | PAspect | PBaseSize | PWinGravity;
    hints.x = 19;
    hints.y = 23;
    hints.width = 180;
    hints.height = 120;
    hints.min_width = 90;
    hints.min_height = 60;
    hints.max_width = 360;
    hints.max_height = 240;
    hints.width_inc = 6;
    hints.height_inc = 4;
    hints.min_aspect.x = 3;
    hints.min_aspect.y = 2;
    hints.max_aspect.x = 3;
    hints.max_aspect.y = 2;
    hints.base_width = 30;
    hints.base_height = 20;
    hints.win_gravity = SouthEastGravity;

    XSetWMNormalHints(display, xid, &hints);
    if ((XGetWindowProperty(display, xid, XA_WM_NORMAL_HINTS, 0,
		    (long)(LONG_MAX / 4L), True, XA_WM_SIZE_HINTS,
		    &actualType, &actualFormat, &nitems, &bytesAfter,
		    &data) != Success)
	    || (actualType != XA_WM_SIZE_HINTS)
	    || (actualFormat != 32)
	    || (nitems != X11_WASM_SIZE_HINTS_ELEMENTS)
	    || (bytesAfter != 0)
	    || !BrowserDecodeSizeHintsProperty(data, nitems, &fetchedHints,
		    &supplied)) {
	goto cleanup;
    }
    if ((supplied != X11_WASM_SIZE_HINTS_SUPPLIED)
	    || (fetchedHints.flags != hints.flags)
	    || (fetchedHints.x != hints.x)
	    || (fetchedHints.y != hints.y)
	    || (fetchedHints.width != hints.width)
	    || (fetchedHints.height != hints.height)
	    || (fetchedHints.min_width != hints.min_width)
	    || (fetchedHints.min_height != hints.min_height)
	    || (fetchedHints.max_width != hints.max_width)
	    || (fetchedHints.max_height != hints.max_height)
	    || (fetchedHints.width_inc != hints.width_inc)
	    || (fetchedHints.height_inc != hints.height_inc)
	    || (fetchedHints.min_aspect.x != hints.min_aspect.x)
	    || (fetchedHints.min_aspect.y != hints.min_aspect.y)
	    || (fetchedHints.max_aspect.x != hints.max_aspect.x)
	    || (fetchedHints.max_aspect.y != hints.max_aspect.y)
	    || (fetchedHints.base_width != hints.base_width)
	    || (fetchedHints.base_height != hints.base_height)
	    || (fetchedHints.win_gravity != hints.win_gravity)) {
	goto cleanup;
    }

    memset(&remainingHints, 0xff, sizeof(remainingHints));
    if (XGetWMNormalHints(display, xid, &remainingHints, &remainingSupplied)
	    || (remainingSupplied != 0)
	    || (remainingHints.flags != 0)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (data != NULL) {
	XFree(data);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGetWindowCollectionPropertiesDeletes(
    unsigned int window,
    unsigned int colormapWindow)
{
    Display *display;
    Window xid = (Window)window;
    Window colorXid = (Window)colormapWindow;
    Atom protocols[2];
    Atom standardColormapProperty;
    XIconSize iconSizes[2];
    XIconSize *remainingIconSizes = NULL;
    int remainingIconSizeCount = -1;
    Window colormapWindows[2];
    Window *remainingColormapWindows = NULL;
    int remainingColormapWindowCount = -1;
    Atom *remainingProtocols = NULL;
    int remainingProtocolCount = -1;
    XStandardColormap colormaps[2];
    XStandardColormap *remainingColormaps = NULL;
    int remainingColormapCount = -1;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *iconSizeData = NULL;
    unsigned char *protocolData = NULL;
    unsigned char *colormapWindowData = NULL;
    unsigned char *standardColormapData = NULL;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    if ((BrowserWindowResource(xid) == NULL)
	    || (BrowserWindowResource(colorXid) == NULL)) {
	return 0;
    }

    memset(iconSizes, 0, sizeof(iconSizes));
    iconSizes[0].min_width = 12;
    iconSizes[0].min_height = 16;
    iconSizes[0].max_width = 48;
    iconSizes[0].max_height = 64;
    iconSizes[0].width_inc = 6;
    iconSizes[0].height_inc = 8;
    iconSizes[1].min_width = 24;
    iconSizes[1].min_height = 24;
    iconSizes[1].max_width = 96;
    iconSizes[1].max_height = 96;
    iconSizes[1].width_inc = 12;
    iconSizes[1].height_inc = 12;

    protocols[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    protocols[1] = XInternAtom(display, "WM_TAKE_FOCUS", False);
    standardColormapProperty = XInternAtom(display,
	    "_X11_WASM_TEST_RGB_MAP_DELETE", False);
    colormapWindows[0] = xid;
    colormapWindows[1] = colorXid;

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
    colormaps[1].base_pixel = 32;
    colormaps[1].red_max = 31;
    colormaps[1].green_max = 63;
    colormaps[1].blue_max = 31;

    if ((protocols[0] == None) || (protocols[1] == None)
	    || (standardColormapProperty == None)
	    || (XSetIconSizes(display, xid, iconSizes, 2) != Success)
	    || !XSetWMProtocols(display, xid, protocols, 2)
	    || !XSetWMColormapWindows(display, xid, colormapWindows, 2)) {
	goto cleanup;
    }
    XSetRGBColormaps(display, xid, colormaps, 2, standardColormapProperty);

    if ((XGetWindowProperty(display, xid, XA_WM_ICON_SIZE, 0,
		    (long)(LONG_MAX / 4L), True, XA_WM_ICON_SIZE,
		    &actualType, &actualFormat, &nitems, &bytesAfter,
		    &iconSizeData) != Success)
	    || (actualType != XA_WM_ICON_SIZE)
	    || (actualFormat != 32)
	    || (nitems != (sizeof(iconSizes) / 4UL))
	    || (bytesAfter != 0)
	    || (iconSizeData == NULL)
	    || (memcmp(iconSizeData, iconSizes, sizeof(iconSizes)) != 0)) {
	goto cleanup;
    }

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, BrowserWmProtocolsAtom(), 0,
		    (long)(LONG_MAX / 4L), True, XA_ATOM, &actualType,
		    &actualFormat, &nitems, &bytesAfter,
		    &protocolData) != Success)
	    || (actualType != XA_ATOM)
	    || (actualFormat != 32)
	    || (nitems != 2)
	    || (bytesAfter != 0)
	    || (protocolData == NULL)
	    || (BrowserPropertyAtomAt(protocolData, 0) != protocols[0])
	    || (BrowserPropertyAtomAt(protocolData, 1) != protocols[1])) {
	goto cleanup;
    }

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, BrowserWmColormapWindowsAtom(), 0,
		    (long)(LONG_MAX / 4L), True, XA_WINDOW, &actualType,
		    &actualFormat, &nitems, &bytesAfter,
		    &colormapWindowData) != Success)
	    || (actualType != XA_WINDOW)
	    || (actualFormat != 32)
	    || (nitems != 2)
	    || (bytesAfter != 0)
	    || (colormapWindowData == NULL)
	    || ((Window)BrowserPropertyAtomAt(colormapWindowData, 0) != xid)
	    || ((Window)BrowserPropertyAtomAt(colormapWindowData, 1)
		    != colorXid)) {
	goto cleanup;
    }

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, standardColormapProperty, 0,
		    (long)(LONG_MAX / 4L), True, XA_RGB_COLOR_MAP,
		    &actualType, &actualFormat, &nitems, &bytesAfter,
		    &standardColormapData) != Success)
	    || (actualType != XA_RGB_COLOR_MAP)
	    || (actualFormat != 32)
	    || (nitems != (sizeof(colormaps) / 4UL))
	    || (bytesAfter != 0)
	    || (standardColormapData == NULL)
	    || (memcmp(standardColormapData, colormaps, sizeof(colormaps))
		    != 0)) {
	goto cleanup;
    }

    if (XGetIconSizes(display, xid, &remainingIconSizes,
		    &remainingIconSizeCount)
	    || (remainingIconSizes != NULL)
	    || (remainingIconSizeCount != 0)
	    || XGetWMProtocols(display, xid, &remainingProtocols,
		    &remainingProtocolCount)
	    || (remainingProtocols != NULL)
	    || (remainingProtocolCount != 0)
	    || XGetWMColormapWindows(display, xid, &remainingColormapWindows,
		    &remainingColormapWindowCount)
	    || (remainingColormapWindows != NULL)
	    || (remainingColormapWindowCount != 0)
	    || XGetRGBColormaps(display, xid, &remainingColormaps,
		    &remainingColormapCount, standardColormapProperty)
	    || (remainingColormaps != NULL)
	    || (remainingColormapCount != 0)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (iconSizeData != NULL) {
	XFree(iconSizeData);
    }
    if (protocolData != NULL) {
	XFree(protocolData);
    }
    if (colormapWindowData != NULL) {
	XFree(colormapWindowData);
    }
    if (standardColormapData != NULL) {
	XFree(standardColormapData);
    }
    if (remainingIconSizes != NULL) {
	XFree(remainingIconSizes);
    }
    if (remainingProtocols != NULL) {
	XFree(remainingProtocols);
    }
    if (remainingColormapWindows != NULL) {
	XFree(remainingColormapWindows);
    }
    if (remainingColormaps != NULL) {
	XFree(remainingColormaps);
    }
    return result;
}

static int
BrowserTestWindowPropertyIsMissing(
    Display *display,
    Window window,
    Atom property)
{
    Atom actualType = None;
    int actualFormat = -1;
    unsigned long nitems = 1;
    unsigned long bytesAfter = 1;
    unsigned char *data = NULL;
    int status;

    status = XGetWindowProperty(display, window, property, 0, 0, False,
	    AnyPropertyType, &actualType, &actualFormat, &nitems,
	    &bytesAfter, &data);
    if (data != NULL) {
	XFree(data);
    }
    return (status == Success) && (actualType == None)
	    && (actualFormat == 0) && (nitems == 0)
	    && (bytesAfter == 0);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGetWindowEwmhPropertiesDeletes(unsigned int window)
{
    static const unsigned int iconData[] = {
	2, 2,
	0xffff0000U, 0xff00ff00U,
	0xff0000ffU, 0xffffffffU
    };
    Display *display;
    Window xid = (Window)window;
    Atom windowTypeAtom;
    Atom windowTypes[2];
    Atom netWmStateAtom;
    Atom states[4];
    Atom opacityAtom;
    Atom iconAtom;
    unsigned int opacity = 0x80000000U;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *typeData = NULL;
    unsigned char *stateData = NULL;
    unsigned char *opacityData = NULL;
    unsigned char *iconPropertyData = NULL;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    if (BrowserWindowResource(xid) == NULL) {
	return 0;
    }

    windowTypeAtom = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    windowTypes[0] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG",
	    False);
    windowTypes[1] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY",
	    False);
    netWmStateAtom = BrowserNetWmStateAtom();
    states[0] = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    states[1] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    states[2] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    states[3] = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    opacityAtom = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
    iconAtom = XInternAtom(display, "_NET_WM_ICON", False);
    if ((windowTypeAtom == None) || (windowTypes[0] == None)
	    || (windowTypes[1] == None) || (netWmStateAtom == None)
	    || (states[0] == None) || (states[1] == None)
	    || (states[2] == None) || (states[3] == None)
	    || (opacityAtom == None) || (iconAtom == None)) {
	goto cleanup;
    }

    if ((XChangeProperty(display, xid, windowTypeAtom, XA_ATOM, 32,
		    PropModeReplace, (const unsigned char *)windowTypes,
		    2) != Success)
	    || (XChangeProperty(display, xid, netWmStateAtom, XA_ATOM, 32,
		    PropModeReplace, (const unsigned char *)states,
		    4) != Success)
	    || (XChangeProperty(display, xid, opacityAtom, XA_CARDINAL, 32,
		    PropModeReplace, (const unsigned char *)&opacity,
		    1) != Success)
	    || (XChangeProperty(display, xid, iconAtom, XA_CARDINAL, 32,
		    PropModeReplace, (const unsigned char *)iconData,
		    (int)(sizeof(iconData) / sizeof(iconData[0])))
		    != Success)) {
	goto cleanup;
    }

    if ((XGetWindowProperty(display, xid, windowTypeAtom, 0,
		    (long)(LONG_MAX / 4L), True, XA_ATOM, &actualType,
		    &actualFormat, &nitems, &bytesAfter,
		    &typeData) != Success)
	    || (actualType != XA_ATOM)
	    || (actualFormat != 32)
	    || (nitems != 2)
	    || (bytesAfter != 0)
	    || (typeData == NULL)
	    || (BrowserPropertyAtomAt(typeData, 0) != windowTypes[0])
	    || (BrowserPropertyAtomAt(typeData, 1) != windowTypes[1])) {
	goto cleanup;
    }

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, netWmStateAtom, 0,
		    (long)(LONG_MAX / 4L), True, XA_ATOM, &actualType,
		    &actualFormat, &nitems, &bytesAfter,
		    &stateData) != Success)
	    || (actualType != XA_ATOM)
	    || (actualFormat != 32)
	    || (nitems != 4)
	    || (bytesAfter != 0)
	    || (stateData == NULL)
	    || (BrowserPropertyAtomAt(stateData, 0) != states[0])
	    || (BrowserPropertyAtomAt(stateData, 1) != states[1])
	    || (BrowserPropertyAtomAt(stateData, 2) != states[2])
	    || (BrowserPropertyAtomAt(stateData, 3) != states[3])) {
	goto cleanup;
    }

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, opacityAtom, 0, 1, True,
		    XA_CARDINAL, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &opacityData) != Success)
	    || (actualType != XA_CARDINAL)
	    || (actualFormat != 32)
	    || (nitems != 1)
	    || (bytesAfter != 0)
	    || (opacityData == NULL)
	    || (BrowserPropertyAtomAt(opacityData, 0) != (Atom)opacity)) {
	goto cleanup;
    }

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if ((XGetWindowProperty(display, xid, iconAtom, 0,
		    (long)(LONG_MAX / 4L), True, XA_CARDINAL, &actualType,
		    &actualFormat, &nitems, &bytesAfter,
		    &iconPropertyData) != Success)
	    || (actualType != XA_CARDINAL)
	    || (actualFormat != 32)
	    || (nitems != (sizeof(iconData) / sizeof(iconData[0])))
	    || (bytesAfter != 0)
	    || (iconPropertyData == NULL)
	    || (memcmp(iconPropertyData, iconData, sizeof(iconData)) != 0)) {
	goto cleanup;
    }

    if (!BrowserTestWindowPropertyIsMissing(display, xid, windowTypeAtom)
	    || !BrowserTestWindowPropertyIsMissing(display, xid,
		    netWmStateAtom)
	    || !BrowserTestWindowPropertyIsMissing(display, xid,
		    opacityAtom)
	    || !BrowserTestWindowPropertyIsMissing(display, xid, iconAtom)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (typeData != NULL) {
	XFree(typeData);
    }
    if (stateData != NULL) {
	XFree(stateData);
    }
    if (opacityData != NULL) {
	XFree(opacityData);
    }
    if (iconPropertyData != NULL) {
	XFree(iconPropertyData);
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetWindowNetWmNameProperties(unsigned int window, int mode)
{
    static const char netTitle[] = "Net " "\xe2\x98\x83" " Title";
    static const char netIconName[] = "Net " "\xc3\xa9" " Icon";
    static const char invalidTitle[] = "Invalid Net Title";
    static const char invalidIconName[] = "Invalid Net Icon";
    Atom netWmNameAtom;
    Atom netWmIconNameAtom;
    Atom utf8StringAtom;
    Window xid = (Window)window;

    BrowserInitDisplay();
    if (BrowserWindowResource(xid) == NULL) {
	return 0;
    }
    netWmNameAtom = XInternAtom(&browserDisplay.display, "_NET_WM_NAME",
	    False);
    netWmIconNameAtom = XInternAtom(&browserDisplay.display,
	    "_NET_WM_ICON_NAME", False);
    utf8StringAtom = BrowserUtf8StringAtom();

    switch (mode) {
    case 0:
	return (XDeleteProperty(&browserDisplay.display, xid, netWmNameAtom)
		== Success)
		&& (XDeleteProperty(&browserDisplay.display, xid,
			netWmIconNameAtom) == Success)
		&& (XDeleteProperty(&browserDisplay.display, xid,
			XA_WM_NAME) == Success)
		&& (XDeleteProperty(&browserDisplay.display, xid,
			XA_WM_ICON_NAME) == Success);
    case 1:
	return (XStoreName(&browserDisplay.display, xid, "Fallback Title")
		== Success)
		&& (XSetIconName(&browserDisplay.display, xid,
			"Fallback Icon") == Success);
    case 2:
	return (XChangeProperty(&browserDisplay.display, xid, netWmNameAtom,
		    utf8StringAtom, 8, PropModeReplace,
		    (const unsigned char *)netTitle, (int)strlen(netTitle))
		== Success)
		&& (XChangeProperty(&browserDisplay.display, xid,
			netWmIconNameAtom, utf8StringAtom, 8, PropModeReplace,
			(const unsigned char *)netIconName,
			(int)strlen(netIconName)) == Success);
    case 3:
	return (XDeleteProperty(&browserDisplay.display, xid, netWmNameAtom)
		== Success)
		&& (XDeleteProperty(&browserDisplay.display, xid,
			netWmIconNameAtom) == Success);
    case 4:
	return (XChangeProperty(&browserDisplay.display, xid, netWmNameAtom,
		    XA_STRING, 8, PropModeReplace,
		    (const unsigned char *)invalidTitle,
		    (int)strlen(invalidTitle)) == Success)
		&& (XChangeProperty(&browserDisplay.display, xid,
			netWmIconNameAtom, XA_STRING, 8, PropModeReplace,
			(const unsigned char *)invalidIconName,
			(int)strlen(invalidIconName)) == Success);
    default:
	return 0;
    }
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetWindowTypeProperties(unsigned int window, int mode)
{
    static const char invalidTypes[] = "not-atoms";
    Atom windowTypeAtom;
    Atom atoms[3];
    Window xid = (Window)window;

    BrowserInitDisplay();
    if (BrowserWindowResource(xid) == NULL) {
	return 0;
    }
    windowTypeAtom = XInternAtom(&browserDisplay.display,
	    "_NET_WM_WINDOW_TYPE", False);

    switch (mode) {
    case 0:
	return XDeleteProperty(&browserDisplay.display, xid, windowTypeAtom)
		== Success;
    case 1:
	atoms[0] = XInternAtom(&browserDisplay.display,
		"_NET_WM_WINDOW_TYPE_DIALOG", False);
	atoms[1] = XInternAtom(&browserDisplay.display,
		"_NET_WM_WINDOW_TYPE_UTILITY", False);
	atoms[2] = XInternAtom(&browserDisplay.display,
		"_NET_WM_WINDOW_TYPE_NORMAL", False);
	return XChangeProperty(&browserDisplay.display, xid, windowTypeAtom,
		XA_ATOM, 32, PropModeReplace, (const unsigned char *)atoms, 3)
		== Success;
    case 2:
	return XChangeProperty(&browserDisplay.display, xid, windowTypeAtom,
		XA_STRING, 8, PropModeReplace,
		(const unsigned char *)invalidTypes,
		(int)strlen(invalidTypes)) == Success;
    default:
	return 0;
    }
}

static Atom
BrowserTestNetWmStateAtom(int state)
{
    const char *name = NULL;

    switch (state) {
    case 1:
	name = "_NET_WM_STATE_ABOVE";
	break;
    case 2:
	name = "_NET_WM_STATE_MAXIMIZED_VERT";
	break;
    case 3:
	name = "_NET_WM_STATE_MAXIMIZED_HORZ";
	break;
    case 4:
	name = "_NET_WM_STATE_FULLSCREEN";
	break;
    default:
	return None;
    }
    return XInternAtom(&browserDisplay.display, name, False);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSendNetWmStateMessage(
    unsigned int window,
    int action,
    int firstState,
    int secondState)
{
    XEvent event;
    Atom firstAtom;
    Atom secondAtom;
    Window xid = (Window)window;

    BrowserInitDisplay();
    if ((action < 0) || (action > 2) || (BrowserWindowResource(xid) == NULL)) {
	return 0;
    }
    firstAtom = BrowserTestNetWmStateAtom(firstState);
    secondAtom = BrowserTestNetWmStateAtom(secondState);
    if ((firstAtom == None) && (secondAtom == None)) {
	return 0;
    }

    memset(&event, 0, sizeof(event));
    event.xclient.type = ClientMessage;
    event.xclient.display = &browserDisplay.display;
    event.xclient.window = xid;
    event.xclient.message_type = BrowserNetWmStateAtom();
    event.xclient.format = 32;
    event.xclient.data.l[0] = action;
    event.xclient.data.l[1] = (long)firstAtom;
    event.xclient.data.l[2] = (long)secondAtom;
    return XSendEvent(&browserDisplay.display, xid, False, NoEventMask,
	    &event) == True;
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

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetWindowMetadataTargetApi(
    unsigned int window,
    unsigned int target,
    int isRootToplevel,
    int useDefaultDisplay)
{
    BrowserInitDisplay();
    return X11WasmSetWindowMetadataTarget(
	    useDefaultDisplay ? NULL : &browserDisplay.display, (Window)window,
	    (Window)target, isRootToplevel);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestDestroyWindowMetadataTargetSource(
    unsigned int window,
    unsigned int target)
{
    Window xid = (Window)window;
    Window targetXid = (Window)target;

    BrowserInitDisplay();
    if ((BrowserWindowResource(xid) == NULL)
	    || (BrowserWindowResource(targetXid) == NULL)) {
	return 0;
    }
    if (!X11WasmTestSetWindowMetadataTargetProperty(window, target, 0)
	    || !X11WasmTestSetWindowMetadataProperties(window, 1)
	    || (XDestroyWindow(&browserDisplay.display, xid) != Success)
	    || (BrowserWindowResource(xid) != NULL)
	    || (BrowserWindowResource(targetXid) == NULL)) {
	return 0;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestDestroyWindowMetadataFallbackSource(
    unsigned int target)
{
    Window targetXid = (Window)target;
    Window source = None;

    BrowserInitDisplay();
    if (BrowserWindowResource(targetXid) == NULL) {
	return 0;
    }
    source = XCreateSimpleWindow(&browserDisplay.display, targetXid, 0, 0,
	    64, 48, 0, 0, 0);
    if ((source == None) || (BrowserWindowResource(source) == NULL)) {
	return 0;
    }
    if ((XMapWindow(&browserDisplay.display, source) != Success)
	    || !X11WasmTestSetWindowMetadataProperties((unsigned int)source, 1)
	    || (XDestroyWindow(&browserDisplay.display, source) != Success)
	    || (BrowserWindowResource(source) != NULL)
	    || (BrowserWindowResource(targetXid) == NULL)) {
	if (BrowserWindowResource(source) != NULL) {
	    (void)XDestroyWindow(&browserDisplay.display, source);
	}
	return 0;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestGetWindowMetadataTargetDeletes(
    unsigned int window,
    unsigned int target)
{
    Display *display;
    Window xid = (Window)window;
    Window targetXid = (Window)target;
    unsigned int targetValue = target;
    Atom targetAtom;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *targetData = NULL;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    if ((BrowserWindowResource(xid) == NULL)
	    || (BrowserWindowResource(targetXid) == NULL)) {
	return 0;
    }
    targetAtom = XInternAtom(display, "_X11_WASM_METADATA_TARGET", False);
    if (targetAtom == None) {
	goto cleanup;
    }
    if ((XChangeProperty(display, xid, targetAtom, XA_WINDOW, 32,
		    PropModeReplace, (const unsigned char *)&targetValue,
		    1) != Success)
	    || !X11WasmTestSetWindowMetadataProperties(window, 1)
	    || (XGetWindowProperty(display, xid, targetAtom, 0, 1, True,
		    XA_WINDOW, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &targetData) != Success)
	    || (actualType != XA_WINDOW)
	    || (actualFormat != 32)
	    || (nitems != 1)
	    || (bytesAfter != 0)
	    || (targetData == NULL)
	    || ((Window)BrowserPropertyAtomAt(targetData, 0) != targetXid)
	    || !BrowserTestWindowPropertyIsMissing(display, xid, targetAtom)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (targetData != NULL) {
	XFree(targetData);
    }
    return result;
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
BrowserDecodeSizeHintsProperty(
    const unsigned char *data,
    unsigned long nitems,
    XSizeHints *hintsReturn,
    long *suppliedReturn)
{
    unsigned long copyItems;
    unsigned long copyBytes;
    long supplied = X11_WASM_OLD_SIZE_HINTS_SUPPLIED;

    if (hintsReturn != NULL) {
	memset(hintsReturn, 0, sizeof(*hintsReturn));
    }
    if (suppliedReturn != NULL) {
	*suppliedReturn = 0;
    }
    if ((data == NULL) || (hintsReturn == NULL)
	    || (nitems < X11_WASM_OLD_SIZE_HINTS_ELEMENTS)) {
	return 0;
    }

    copyItems = (nitems < X11_WASM_SIZE_HINTS_ELEMENTS)
	    ? nitems : X11_WASM_SIZE_HINTS_ELEMENTS;
    copyBytes = copyItems * 4UL;
    memcpy(hintsReturn, data, (size_t)copyBytes);
    if (nitems >= X11_WASM_SIZE_HINTS_ELEMENTS) {
	supplied = X11_WASM_SIZE_HINTS_SUPPLIED;
    }
    hintsReturn->flags &= supplied;
    if (suppliedReturn != NULL) {
	*suppliedReturn = supplied;
    }
    return 1;
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
	    || !BrowserDecodeSizeHintsProperty(data, nitems, hintsReturn,
		    suppliedReturn)) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
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

static int
BrowserValidatePropertyList(
    Display **displayPtr,
    const void *items,
    int count,
    unsigned char requestCode)
{
    if (count < 0) {
	return BrowserValueOrError(displayPtr, count, 0, requestCode);
    }
    if ((count > 0) && (items == NULL)) {
	return BrowserPropertyValueOrError(displayPtr, (XID)count,
		requestCode);
    }
    return 1;
}

static int
BrowserPropertyListElementCountOrError(
    Display **displayPtr,
    int count,
    size_t itemBytes,
    unsigned char requestCode,
    int *nelementsPtr)
{
    int wordsPerItem = (int)(itemBytes / 4U);

    if (nelementsPtr != NULL) {
	*nelementsPtr = 0;
    }
    if ((itemBytes == 0U) || ((itemBytes % 4U) != 0U)
	    || (wordsPerItem <= 0)) {
	return BrowserPropertyValueOrError(displayPtr, (XID)itemBytes,
		requestCode);
    }
    if (count > (INT_MAX / wordsPerItem)) {
	Display *display = (displayPtr != NULL) ? *displayPtr : NULL;

	if (display == NULL) {
	    display = X11WasmGetDisplay(NULL);
	    if (displayPtr != NULL) {
		*displayPtr = display;
	    }
	}
	(void)BrowserReportXError(display, BadLength, (XID)count,
		requestCode, 0);
	return 0;
    }
    if (nelementsPtr != NULL) {
	*nelementsPtr = count * wordsPerItem;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE int
XSetIconSizes(
    Display *display,
    Window window,
    XIconSize *sizeList,
    int count)
{
    int nelements;

    if (!BrowserValidatePropertyList(&display, sizeList, count,
	    X11_WASM_XREQ_CHANGE_PROPERTY)) {
	return Success;
    }
    if (count == 0) {
	(void)XDeleteProperty(display, window, XA_WM_ICON_SIZE);
	return Success;
    }
    if (!BrowserPropertyListElementCountOrError(&display, count,
	    sizeof(XIconSize), X11_WASM_XREQ_CHANGE_PROPERTY, &nelements)) {
	return Success;
    }
    return XChangeProperty(display, window, XA_WM_ICON_SIZE, XA_WM_ICON_SIZE,
	    32, PropModeReplace, (const unsigned char *)sizeList,
	    nelements);
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
    int nelements;

    if (!BrowserValidatePropertyList(&display, stdcmaps, count,
	    X11_WASM_XREQ_CHANGE_PROPERTY)) {
	return;
    }
    if (count == 0) {
	(void)XDeleteProperty(display, window, property);
	return;
    }
    if (!BrowserPropertyListElementCountOrError(&display, count,
	    sizeof(XStandardColormap), X11_WASM_XREQ_CHANGE_PROPERTY,
	    &nelements)) {
	return;
    }
    (void)XChangeProperty(display, window, property, XA_RGB_COLOR_MAP, 32,
	    PropModeReplace, (const unsigned char *)stdcmaps,
	    nelements);
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
	    || ((nitems > 0) && (data == NULL))) {
	if (data != NULL) {
	    XFree(data);
	}
	return False;
    }
    if (nitems > 0) {
	copyPtr = (Window *)ckalloc((size_t)nitems * sizeof(Window));
	for (i = 0; i < nitems; i++) {
	    copyPtr[i] = (Window)BrowserPropertyAtomAt(data, i);
	}
    }
    if (data != NULL) {
	XFree(data);
    }
    if (windows != NULL) {
	*windows = copyPtr;
    } else {
	if (copyPtr != NULL) {
	    XFree(copyPtr);
	}
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

    if (!BrowserValidatePropertyList(&display, windows, count,
	    X11_WASM_XREQ_CHANGE_PROPERTY)) {
	return False;
    }
    (void)XChangeProperty(display, window, property, XA_WINDOW, 32,
	    PropModeReplace, (const unsigned char *)windows, count);
    return True;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestSetWMColormapWindows(
    unsigned int window,
    unsigned int colormapWindow,
    int enabled)
{
    Window windows[2];

    BrowserInitDisplay();
    if (!enabled) {
	return XSetWMColormapWindows(&browserDisplay.display, (Window)window,
		NULL, 0);
    }
    windows[0] = (Window)window;
    windows[1] = (Window)colormapWindow;
    return XSetWMColormapWindows(&browserDisplay.display, (Window)window,
	    windows, 2);
}

Status
XSetWMProtocols(
    Display *display,
    Window window,
    Atom *protocols,
    int count)
{
    if (!BrowserValidatePropertyList(&display, protocols, count,
	    X11_WASM_XREQ_CHANGE_PROPERTY)) {
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

static Atom
BrowserCutBufferAtom(
    int buffer)
{
    return (buffer >= 0 && buffer < X11_WASM_CUT_BUFFER_COUNT)
	    ? (Atom)(XA_CUT_BUFFER0 + buffer) : None;
}

static int
BrowserCutBufferIndexIsValid(
    int buffer)
{
    return BrowserCutBufferAtom(buffer) != None;
}

static void
BrowserFreePropertySnapshots(
    BrowserPropertySnapshot *snapshots,
    int count)
{
    int i;

    if (snapshots == NULL) {
	return;
    }
    for (i = 0; i < count; i++) {
	if (snapshots[i].data != NULL) {
	    ckfree(snapshots[i].data);
	}
    }
    ckfree(snapshots);
}

static void
BrowserSnapshotProperty(
    X11WasmProperty *propertyPtr,
    BrowserPropertySnapshot *snapshotPtr)
{
    int dataLength = propertyPtr->dataLength;

    memset(snapshotPtr, 0, sizeof(BrowserPropertySnapshot));
    snapshotPtr->type = propertyPtr->type;
    snapshotPtr->format = propertyPtr->format;
    snapshotPtr->nitems = propertyPtr->nitems;
    snapshotPtr->dataLength = dataLength;
    snapshotPtr->data = (unsigned char *)ckalloc((size_t)dataLength + 1U);
    if (dataLength > 0) {
	memcpy(snapshotPtr->data, propertyPtr->data, (size_t)dataLength);
    }
    snapshotPtr->data[dataLength] = '\0';
}

static void
BrowserAssignPropertySnapshot(
    X11WasmProperty *propertyPtr,
    BrowserPropertySnapshot *snapshotPtr)
{
    if (propertyPtr->data != NULL) {
	ckfree(propertyPtr->data);
    }
    propertyPtr->type = snapshotPtr->type;
    propertyPtr->format = snapshotPtr->format;
    propertyPtr->nitems = snapshotPtr->nitems;
    propertyPtr->dataLength = snapshotPtr->dataLength;
    propertyPtr->data = snapshotPtr->data;
    snapshotPtr->data = NULL;
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
    int propertyBytes;
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
    propertyBytes = BrowserPropertyByteLength(format, nelements);
    if (propertyBytes < 0) {
	if (display == NULL) {
	    display = X11WasmGetDisplay(NULL);
	}
	(void)BrowserReportXError(display, BadLength, (XID)nelements,
		X11_WASM_XREQ_CHANGE_PROPERTY, 0);
	return Success;
    }
    propertyPtr = BrowserFindProperty(resourcePtr, property);
    if (!BrowserPropertyChangeOrError(&display, propertyPtr, property, type,
	    format, mode, X11_WASM_XREQ_CHANGE_PROPERTY)) {
	return Success;
    }
    if ((propertyBytes > 0) && (data == NULL)) {
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
    if (!BrowserValidateBool(&display, deleteProperty,
	    X11_WASM_XREQ_GET_PROPERTY)) {
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
	if (propReturn && (*propReturn != NULL)) {
	    ckfree(*propReturn);
	    *propReturn = NULL;
	}
	(void)BrowserPropertyValueOrError(&display, (XID)offset,
		X11_WASM_XREQ_GET_PROPERTY);
	return BadValue;
    }

    elementBytes = BrowserPropertyElementBytes(propertyPtr->format);
    startBytes = (int)(offset * 4L);
    if (startBytes < propertyPtr->dataLength) {
	long requestedBytes = (length > (LONG_MAX / 4L))
		? LONG_MAX : (length * 4L);
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
XRotateWindowProperties(
    Display *display,
    Window window,
    Atom *properties,
    int numProperties,
    int npositions)
{
    X11WasmResource *resourcePtr;
    BrowserPropertySnapshot *snapshots;
    int i;
    int j;
    int shift;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_ROTATE_PROPERTIES);
    if (resourcePtr == NULL) {
	return Success;
    }
    if (numProperties < 0) {
	(void)BrowserPropertyValueOrError(&display, (XID)numProperties,
		X11_WASM_XREQ_ROTATE_PROPERTIES);
	return Success;
    }
    if (numProperties == 0) {
	return Success;
    }
    if (properties == NULL) {
	(void)BrowserPropertyValueOrError(&display, 0,
		X11_WASM_XREQ_ROTATE_PROPERTIES);
	return Success;
    }
    for (i = 0; i < numProperties; i++) {
	if (!BrowserPropertyAtomOrError(&display, properties[i],
		X11_WASM_XREQ_ROTATE_PROPERTIES)) {
	    return Success;
	}
	for (j = 0; j < i; j++) {
	    if (properties[j] == properties[i]) {
		(void)BrowserBadMatchOrError(&display, (XID)properties[i],
			X11_WASM_XREQ_ROTATE_PROPERTIES);
		return Success;
	    }
	}
	if (BrowserFindProperty(resourcePtr, properties[i]) == NULL) {
	    (void)BrowserBadMatchOrError(&display, (XID)properties[i],
		    X11_WASM_XREQ_ROTATE_PROPERTIES);
	    return Success;
	}
    }

    shift = npositions % numProperties;
    if (shift < 0) {
	shift += numProperties;
    }
    if (shift == 0) {
	return Success;
    }

    snapshots = (BrowserPropertySnapshot *)ckalloc(
	    (size_t)numProperties * sizeof(BrowserPropertySnapshot));
    memset(snapshots, 0,
	    (size_t)numProperties * sizeof(BrowserPropertySnapshot));
    for (i = 0; i < numProperties; i++) {
	BrowserSnapshotProperty(BrowserFindProperty(resourcePtr,
		properties[i]), snapshots + i);
    }
    for (i = 0; i < numProperties; i++) {
	int target = (i + shift) % numProperties;
	X11WasmProperty *targetPtr = BrowserFindProperty(resourcePtr,
		properties[target]);

	BrowserAssignPropertySnapshot(targetPtr, snapshots + i);
    }
    BrowserFreePropertySnapshots(snapshots, numProperties);

    for (i = 0; i < numProperties; i++) {
	X11WasmProperty *propertyPtr = BrowserFindProperty(resourcePtr,
		properties[i]);

	BrowserNotifyProperty(resourcePtr, properties[i], PropertyNewValue);
	BrowserApplyWindowMetadataProperty(window, properties[i], propertyPtr);
	BrowserSyncClipboardProperty(properties[i], propertyPtr);
    }
    return Success;
}

int
XStoreBuffer(
    Display *display,
    _Xconst char *bytes,
    int nbytes,
    int buffer)
{
    Atom property;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    property = BrowserCutBufferAtom(buffer);
    if (property == None) {
	return Success;
    }
    return XChangeProperty(display,
	    RootWindow(display, DefaultScreen(display)), property, XA_STRING, 8,
	    PropModeReplace, (_Xconst unsigned char *)bytes, nbytes);
}

int
XStoreBytes(
    Display *display,
    _Xconst char *bytes,
    int nbytes)
{
    return XStoreBuffer(display, bytes, nbytes, 0);
}

char *
XFetchBuffer(
    Display *display,
    int *nbytesReturn,
    int buffer)
{
    Atom property = BrowserCutBufferAtom(buffer);
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    int result;

    if (nbytesReturn != NULL) {
	*nbytesReturn = 0;
    }
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (!BrowserCutBufferIndexIsValid(buffer)) {
	return NULL;
    }
    result = XGetWindowProperty(display,
	    RootWindow(display, DefaultScreen(display)), property, 0, LONG_MAX,
	    False, XA_STRING, &actualType, &actualFormat, &nitems,
	    &bytesAfter, &data);
    if ((result != Success) || (actualType != XA_STRING)
	    || (actualFormat != 8) || (nitems == 0) || (data == NULL)) {
	if (data != NULL) {
	    XFree(data);
	}
	return NULL;
    }
    if (nbytesReturn != NULL) {
	*nbytesReturn = (nitems > (unsigned long)INT_MAX)
		? INT_MAX : (int)nitems;
    }
    return (char *)data;
}

char *
XFetchBytes(
    Display *display,
    int *nbytesReturn)
{
    return XFetchBuffer(display, nbytesReturn, 0);
}

int
XRotateBuffers(
    Display *display,
    int rotate)
{
    Atom properties[X11_WASM_CUT_BUFFER_COUNT];
    int i;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    for (i = 0; i < X11_WASM_CUT_BUFFER_COUNT; i++) {
	properties[i] = BrowserCutBufferAtom(i);
    }
    return XRotateWindowProperties(display,
	    RootWindow(display, DefaultScreen(display)), properties,
	    X11_WASM_CUT_BUFFER_COUNT, rotate);
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

static int
BrowserTestPropertyAtomListContains(
    const unsigned char *data,
    unsigned long count,
    Atom atom)
{
    unsigned long i;

    for (i = 0; i < count; i++) {
	if (BrowserPropertyAtomAt(data, i) == atom) {
	    return 1;
	}
    }
    return 0;
}

static int
BrowserTestExternalClipboardTargetList(void)
{
    Atom actualType = None;
    Atom property;
    XEvent event;
    unsigned char *data = NULL;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    int ok = 0;

    property = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_TARGET_LIST", False);
    BrowserClearEventQueue();
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    property);
    (void)XConvertSelection(&browserDisplay.display,
	    BrowserClipboardSelectionAtom(), BrowserTargetsAtom(), property,
	    browserDisplay.rootWindow, CurrentTime);
    if (!BrowserPopEvent(&event, NULL)
	    || (event.type != SelectionNotify)
	    || (event.xselection.selection != BrowserClipboardSelectionAtom())
	    || (event.xselection.target != BrowserTargetsAtom())
	    || (event.xselection.property != property)) {
	goto cleanup;
    }
    if (XGetWindowProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    property, 0, 4096, False, XA_ATOM, &actualType, &actualFormat,
	    &nitems, &bytesAfter, &data) != Success) {
	goto cleanup;
    }
    if ((actualType != XA_ATOM) || (actualFormat != 32)
	    || (bytesAfter != 0) || (nitems != 8)
	    || !BrowserTestPropertyAtomListContains(data, nitems,
		    XA_STRING)
	    || !BrowserTestPropertyAtomListContains(data, nitems,
		    BrowserUtf8StringAtom())
	    || !BrowserTestPropertyAtomListContains(data, nitems,
		    BrowserTextAtom())
	    || !BrowserTestPropertyAtomListContains(data, nitems,
		    BrowserCompoundTextAtom())
	    || !BrowserTestPropertyAtomListContains(data, nitems,
		    BrowserLengthAtom())
	    || !BrowserTestPropertyAtomListContains(data, nitems,
		    BrowserTargetsAtom())
	    || !BrowserTestPropertyAtomListContains(data, nitems,
		    BrowserTimestampAtom())
	    || !BrowserTestPropertyAtomListContains(data, nitems,
		    BrowserMultipleAtom())) {
	goto cleanup;
    }
    ok = 1;

cleanup:
    if (data != NULL) {
	XFree(data);
    }
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    property);
    return ok;
}

static int
BrowserTestExternalClipboardLength(
    unsigned int expectedLength)
{
    Atom actualType = None;
    Atom property;
    XEvent event;
    unsigned char *data = NULL;
    unsigned int actualLength = 0;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    int ok = 0;

    property = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_LENGTH", False);
    BrowserClearEventQueue();
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    property);
    (void)XConvertSelection(&browserDisplay.display,
	    BrowserClipboardSelectionAtom(), BrowserLengthAtom(), property,
	    browserDisplay.rootWindow, CurrentTime);
    if (!BrowserPopEvent(&event, NULL)
	    || (event.type != SelectionNotify)
	    || (event.xselection.selection != BrowserClipboardSelectionAtom())
	    || (event.xselection.target != BrowserLengthAtom())
	    || (event.xselection.property != property)) {
	goto cleanup;
    }
    if (XGetWindowProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    property, 0, 1, False, AnyPropertyType, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data) != Success) {
	goto cleanup;
    }
    if ((actualType != XA_INTEGER) || (actualFormat != 32)
	    || (nitems != 1) || (bytesAfter != 0) || (data == NULL)) {
	goto cleanup;
    }
    memcpy(&actualLength, data, sizeof(actualLength));
    if (actualLength != expectedLength) {
	goto cleanup;
    }
    ok = 1;

cleanup:
    if (data != NULL) {
	XFree(data);
    }
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    property);
    return ok;
}

static int
BrowserTestExternalClipboardMultiple(
    const unsigned char *expectedString,
    unsigned long expectedStringCount,
    const unsigned char *expectedUtf8,
    unsigned long expectedUtf8Count,
    unsigned int expectedLength)
{
    Atom actualType = None;
    Atom multipleProperty;
    Atom stringProperty;
    Atom utf8Property;
    Atom timestampProperty;
    Atom lengthProperty;
    Atom unsupportedProperty;
    Atom unsupportedTarget;
    XEvent event;
    unsigned int pairs[12];
    unsigned char *data = NULL;
    int actualFormat = 0;
    unsigned int actualLength = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    int ok = 0;

    multipleProperty = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_MULTIPLE", False);
    stringProperty = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_MULTIPLE_STRING", False);
    utf8Property = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_MULTIPLE_UTF8", False);
    timestampProperty = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_MULTIPLE_TIMESTAMP", False);
    lengthProperty = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_MULTIPLE_LENGTH", False);
    unsupportedProperty = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_MULTIPLE_UNSUPPORTED_PROPERTY", False);
    unsupportedTarget = XInternAtom(&browserDisplay.display,
	    "_X11_WASM_TEST_CLIPBOARD_MULTIPLE_UNSUPPORTED_TARGET", False);

    pairs[0] = (unsigned int)XA_STRING;
    pairs[1] = (unsigned int)stringProperty;
    pairs[2] = (unsigned int)BrowserUtf8StringAtom();
    pairs[3] = (unsigned int)utf8Property;
    pairs[4] = (unsigned int)BrowserTimestampAtom();
    pairs[5] = (unsigned int)timestampProperty;
    pairs[6] = (unsigned int)unsupportedTarget;
    pairs[7] = (unsigned int)unsupportedProperty;
    pairs[8] = (unsigned int)BrowserTextAtom();
    pairs[9] = (unsigned int)None;
    pairs[10] = (unsigned int)BrowserLengthAtom();
    pairs[11] = (unsigned int)lengthProperty;

    BrowserClearEventQueue();
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    multipleProperty);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    stringProperty);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    utf8Property);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    timestampProperty);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    lengthProperty);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    unsupportedProperty);

    (void)XChangeProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    multipleProperty, BrowserAtomPairAtom(), 32, PropModeReplace,
	    (const unsigned char *)pairs, (int)(sizeof(pairs) / sizeof(pairs[0])));
    (void)XConvertSelection(&browserDisplay.display,
	    BrowserClipboardSelectionAtom(), BrowserMultipleAtom(),
	    multipleProperty, browserDisplay.rootWindow, CurrentTime);
    if (!BrowserPopEvent(&event, NULL)
	    || (event.type != SelectionNotify)
	    || (event.xselection.selection != BrowserClipboardSelectionAtom())
	    || (event.xselection.target != BrowserMultipleAtom())
	    || (event.xselection.property != multipleProperty)) {
	goto cleanup;
    }

    if (XGetWindowProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    multipleProperty, 0, 16, False, BrowserAtomPairAtom(),
	    &actualType, &actualFormat, &nitems, &bytesAfter, &data)
	    != Success) {
	goto cleanup;
    }
    if ((actualType != BrowserAtomPairAtom()) || (actualFormat != 32)
	    || (nitems != 12) || (bytesAfter != 0)
	    || (BrowserPropertyAtomAt(data, 0) != XA_STRING)
	    || (BrowserPropertyAtomAt(data, 1) != stringProperty)
	    || (BrowserPropertyAtomAt(data, 2) != BrowserUtf8StringAtom())
	    || (BrowserPropertyAtomAt(data, 3) != utf8Property)
	    || (BrowserPropertyAtomAt(data, 4) != BrowserTimestampAtom())
	    || (BrowserPropertyAtomAt(data, 5) != timestampProperty)
	    || (BrowserPropertyAtomAt(data, 6) != unsupportedTarget)
	    || (BrowserPropertyAtomAt(data, 7) != None)
	    || (BrowserPropertyAtomAt(data, 8) != BrowserTextAtom())
	    || (BrowserPropertyAtomAt(data, 9) != None)
	    || (BrowserPropertyAtomAt(data, 10) != BrowserLengthAtom())
	    || (BrowserPropertyAtomAt(data, 11) != lengthProperty)) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if (XGetWindowProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    stringProperty, 0, 4096, False, AnyPropertyType, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data) != Success) {
	goto cleanup;
    }
    if ((actualType != XA_STRING) || (actualFormat != 8)
	    || (nitems != expectedStringCount) || (bytesAfter != 0)
	    || ((expectedStringCount > 0)
		    && (memcmp(data, expectedString,
			    (size_t)expectedStringCount) != 0))) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if (XGetWindowProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    utf8Property, 0, 4096, False, AnyPropertyType, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data) != Success) {
	goto cleanup;
    }
    if ((actualType != BrowserUtf8StringAtom()) || (actualFormat != 8)
	    || (nitems != expectedUtf8Count) || (bytesAfter != 0)
	    || ((expectedUtf8Count > 0)
		    && (memcmp(data, expectedUtf8,
			    (size_t)expectedUtf8Count) != 0))) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if (XGetWindowProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    timestampProperty, 0, 1, False, AnyPropertyType, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data) != Success) {
	goto cleanup;
    }
    if ((actualType != XA_INTEGER) || (actualFormat != 32)
	    || (nitems != 1) || (bytesAfter != 0)) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    actualType = None;
    actualFormat = 0;
    nitems = 0;
    bytesAfter = 0;
    if (XGetWindowProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    lengthProperty, 0, 1, False, AnyPropertyType, &actualType,
	    &actualFormat, &nitems, &bytesAfter, &data) != Success) {
	goto cleanup;
    }
    if ((actualType != XA_INTEGER) || (actualFormat != 32)
	    || (nitems != 1) || (bytesAfter != 0) || (data == NULL)) {
	goto cleanup;
    }
    memcpy(&actualLength, data, sizeof(actualLength));
    if (actualLength != expectedLength) {
	goto cleanup;
    }
    ok = 1;

cleanup:
    if (data != NULL) {
	XFree(data);
    }
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    multipleProperty);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    stringProperty);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    utf8Property);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    timestampProperty);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    lengthProperty);
    (void)XDeleteProperty(&browserDisplay.display, browserDisplay.rootWindow,
	    unsupportedProperty);
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
    if (!BrowserTestExternalClipboardTargetList()) {
	result = 17;
	goto cleanup;
    }
    if (!BrowserTestExternalClipboardLength(
	    (unsigned int)sizeof(latin1Utf8Bytes))) {
	result = 18;
	goto cleanup;
    }
    if (!BrowserTestExternalClipboardMultiple(latin1String,
	    (unsigned long)sizeof(latin1String), latin1Utf8Bytes,
	    (unsigned long)sizeof(latin1Utf8Bytes),
	    (unsigned int)sizeof(latin1Utf8Bytes))) {
	result = 16;
	goto cleanup;
    }

    BrowserSetExternalClipboardText(unicodeUtf8, -1);
    if (!BrowserTestExternalClipboardLength((unsigned int)strlen(unicodeUtf8))) {
	result = 19;
	goto cleanup;
    }
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
X11WasmTestPropertyMutationApis(void)
{
    static const unsigned char replaceText[] = {'B'};
    static const unsigned char appendText[] = {'C', 'D'};
    static const unsigned char prependText[] = {'A'};
    static const unsigned char partialText[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L'
    };
    Display *display;
    Window window = None;
    Atom property;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    property = XInternAtom(display, "_X11_WASM_PROPERTY_MUTATION_TEST", False);
    window = XCreateSimpleWindow(display, browserDisplay.rootWindow, 0, 0,
	    4, 4, 0, 0, 0);
    if ((property == None) || (window == None)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    if ((XChangeProperty(display, window, property, XA_STRING, 8,
		    PropModeReplace, replaceText, 1) != Success)
	    || (XChangeProperty(display, window, property, XA_STRING, 8,
		    PropModeAppend, appendText, 2) != Success)
	    || (XChangeProperty(display, window, property, XA_STRING, 8,
		    PropModePrepend, prependText, 1) != Success)) {
	goto cleanup;
    }
    if ((XGetWindowProperty(display, window, property, 0, 1, False,
		    AnyPropertyType, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &data) != Success)
	    || (actualType != XA_STRING) || (actualFormat != 8)
	    || (nitems != 4) || (bytesAfter != 0) || (data == NULL)
	    || (memcmp(data, "ABCD", 4) != 0) || (data[4] != '\0')) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    if (XChangeProperty(display, window, property, XA_STRING, 8,
	    PropModeReplace, partialText, (int)sizeof(partialText)) != Success) {
	goto cleanup;
    }
    if ((XGetWindowProperty(display, window, property, 0, 0, True,
		    XA_STRING, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &data) != Success)
	    || (actualType != XA_STRING) || (actualFormat != 8)
	    || (nitems != 0) || (bytesAfter != sizeof(partialText))
	    || (data == NULL) || (data[0] != '\0')) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;
    if ((XGetWindowProperty(display, window, property, 0, 1, True,
		    XA_ATOM, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &data) != Success)
	    || (actualType != XA_STRING) || (actualFormat != 8)
	    || (nitems != 0) || (bytesAfter != sizeof(partialText))
	    || (data == NULL) || (data[0] != '\0')) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;
    if ((XGetWindowProperty(display, window, property, 1, 1, True,
		    XA_STRING, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &data) != Success)
	    || (actualType != XA_STRING) || (actualFormat != 8)
	    || (nitems != 4) || (bytesAfter != 4) || (data == NULL)
	    || (memcmp(data, "EFGH", 4) != 0) || (data[4] != '\0')) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    if ((XGetWindowProperty(display, window, property, 0, 3, False,
		    XA_STRING, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &data) != Success)
	    || (actualType != XA_STRING) || (actualFormat != 8)
	    || (nitems != sizeof(partialText)) || (bytesAfter != 0)
	    || (data == NULL)
	    || (memcmp(data, partialText, sizeof(partialText)) != 0)
	    || (data[sizeof(partialText)] != '\0')) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    if ((XGetWindowProperty(display, window, property, 2, 1, True,
		    XA_STRING, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &data) != Success)
	    || (actualType != XA_STRING) || (actualFormat != 8)
	    || (nitems != 4) || (bytesAfter != 0) || (data == NULL)
	    || (memcmp(data, "IJKL", 4) != 0) || (data[4] != '\0')) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    if ((XGetWindowProperty(display, window, property, 0, 1, False,
		    AnyPropertyType, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &data) != Success)
	    || (actualType != None) || (actualFormat != 0) || (nitems != 0)
	    || (bytesAfter != 0) || (data == NULL) || (data[0] != '\0')) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (data != NULL) {
	XFree(data);
    }
    BrowserClearEventQueue();
    if (window != None) {
	(void)XDestroyWindow(display, window);
    }
    BrowserClearEventQueue();
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestPropertyNotifyEvents(void)
{
    static const unsigned char value[] = {'p', 'r', 'o', 'p'};
    static const unsigned char rotateValues[] = {'0', '1', '2'};
    static const unsigned char rotatedValues[] = {'2', '0', '1'};
    Display *display;
    Window window = None;
    Atom property;
    Atom rotateProperties[3];
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    XEvent event;
    int i;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    property = XInternAtom(display, "_X11_WASM_PROPERTY_NOTIFY_TEST", False);
    rotateProperties[0] = XInternAtom(display,
	    "_X11_WASM_PROPERTY_NOTIFY_ROTATE_A", False);
    rotateProperties[1] = XInternAtom(display,
	    "_X11_WASM_PROPERTY_NOTIFY_ROTATE_B", False);
    rotateProperties[2] = XInternAtom(display,
	    "_X11_WASM_PROPERTY_NOTIFY_ROTATE_C", False);
    window = XCreateSimpleWindow(display, browserDisplay.rootWindow, 0, 0,
	    4, 4, 0, 0, 0);
    if ((property == None) || (rotateProperties[0] == None)
	    || (rotateProperties[1] == None) || (rotateProperties[2] == None)
	    || (window == None)) {
	goto cleanup;
    }
    BrowserClearEventQueue();
    if (XSelectInput(display, window, PropertyChangeMask) != Success) {
	goto cleanup;
    }

    if ((XChangeProperty(display, window, property, XA_STRING, 8,
		    PropModeReplace, value, (int)sizeof(value)) != Success)
	    || (XEventsQueued(display, QueuedAlready) != 1)
	    || (XNextEvent(display, &event) != Success)
	    || (event.type != PropertyNotify)
	    || (event.xproperty.window != window)
	    || (event.xproperty.atom != property)
	    || (event.xproperty.state != PropertyNewValue)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }

    if ((XGetWindowProperty(display, window, property, 0, 1, True,
		    AnyPropertyType, &actualType, &actualFormat, &nitems,
		    &bytesAfter, &data) != Success)
	    || (actualType != XA_STRING) || (actualFormat != 8)
	    || (nitems != sizeof(value)) || (bytesAfter != 0)
	    || (data == NULL) || (memcmp(data, value, sizeof(value)) != 0)
	    || (XEventsQueued(display, QueuedAlready) != 1)
	    || (XNextEvent(display, &event) != Success)
	    || (event.type != PropertyNotify)
	    || (event.xproperty.window != window)
	    || (event.xproperty.atom != property)
	    || (event.xproperty.state != PropertyDelete)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    if ((XChangeProperty(display, window, property, XA_STRING, 8,
		    PropModeReplace, value, (int)sizeof(value)) != Success)
	    || (XEventsQueued(display, QueuedAlready) != 1)
	    || (XNextEvent(display, &event) != Success)
	    || (event.type != PropertyNotify)
	    || (event.xproperty.window != window)
	    || (event.xproperty.atom != property)
	    || (event.xproperty.state != PropertyNewValue)
	    || (XDeleteProperty(display, window, property) != Success)
	    || (XEventsQueued(display, QueuedAlready) != 1)
	    || (XNextEvent(display, &event) != Success)
	    || (event.type != PropertyNotify)
	    || (event.xproperty.window != window)
	    || (event.xproperty.atom != property)
	    || (event.xproperty.state != PropertyDelete)
	    || (XDeleteProperty(display, window, property) != Success)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }

    for (i = 0; i < 3; i++) {
	if (XChangeProperty(display, window, rotateProperties[i], XA_STRING,
		8, PropModeReplace, rotateValues + i, 1) != Success) {
	    goto cleanup;
	}
    }
    BrowserClearEventQueue();
    if ((XRotateWindowProperties(display, window, rotateProperties, 3, 1)
		    != Success)
	    || (XEventsQueued(display, QueuedAlready) != 3)) {
	goto cleanup;
    }
    for (i = 0; i < 3; i++) {
	if ((XNextEvent(display, &event) != Success)
		|| (event.type != PropertyNotify)
		|| (event.xproperty.window != window)
		|| (event.xproperty.atom != rotateProperties[i])
		|| (event.xproperty.state != PropertyNewValue)) {
	    goto cleanup;
	}
    }
    if (XEventsQueued(display, QueuedAlready) != 0) {
	goto cleanup;
    }
    for (i = 0; i < 3; i++) {
	if ((XGetWindowProperty(display, window, rotateProperties[i], 0, 1,
			False, AnyPropertyType, &actualType, &actualFormat,
			&nitems, &bytesAfter, &data) != Success)
		|| (actualType != XA_STRING) || (actualFormat != 8)
		|| (nitems != 1) || (bytesAfter != 0)
		|| (data == NULL) || (data[0] != rotatedValues[i])
		|| (data[1] != '\0')) {
	    goto cleanup;
	}
	XFree(data);
	data = NULL;
    }
    if ((XRotateWindowProperties(display, window, rotateProperties, 3, 3)
		    != Success)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (data != NULL) {
	XFree(data);
    }
    BrowserClearEventQueue();
    if (window != None) {
	(void)XDestroyWindow(display, window);
    }
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

EMSCRIPTEN_KEEPALIVE int
X11WasmTestLocalSelectionConversion(void)
{
    static const unsigned char selectionData[] = {'o', 'k'};
    Display *display;
    Window owner = None;
    Window requestor = None;
    Atom selection;
    Atom target;
    Atom property;
    Atom actualType = None;
    int actualFormat = 0;
    unsigned long nitems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = NULL;
    Time requestTime;
    XEvent event;
    int result = 0;

    BrowserInitDisplay();
    display = &browserDisplay.display;
    selection = XInternAtom(display, "_X11_WASM_LOCAL_SELECTION", False);
    target = XInternAtom(display, "_X11_WASM_LOCAL_SELECTION_TARGET", False);
    property = XInternAtom(display, "_X11_WASM_LOCAL_SELECTION_PROPERTY",
	    False);
    owner = XCreateSimpleWindow(display, browserDisplay.rootWindow, 1, 1,
	    4, 4, 0, 0, 0);
    requestor = XCreateSimpleWindow(display, browserDisplay.rootWindow, 8, 1,
	    4, 4, 0, 0, 0);
    if ((selection == None) || (target == None) || (property == None)
	    || (owner == None) || (requestor == None)) {
	goto cleanup;
    }
    BrowserClearEventQueue();

    if ((XSetSelectionOwner(display, selection, owner, CurrentTime)
		    != Success)
	    || (XGetSelectionOwner(display, selection) != owner)) {
	goto cleanup;
    }
    requestTime = eventTime + 1;
    if (XConvertSelection(display, selection, target, property, requestor,
	    requestTime) != Success) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if ((XNextEvent(display, &event) != Success)
	    || (event.type != SelectionRequest)
	    || (event.xselectionrequest.owner != owner)
	    || (event.xselectionrequest.requestor != requestor)
	    || (event.xselectionrequest.selection != selection)
	    || (event.xselectionrequest.target != target)
	    || (event.xselectionrequest.property != property)
	    || (event.xselectionrequest.time != requestTime)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    if (XChangeProperty(display, requestor, property, target, 8,
	    PropModeReplace, selectionData, (int)sizeof(selectionData))
	    != Success) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    event.xselection.type = SelectionNotify;
    event.xselection.display = display;
    event.xselection.requestor = requestor;
    event.xselection.selection = selection;
    event.xselection.target = target;
    event.xselection.property = property;
    event.xselection.time = requestTime;
    if ((XSendEvent(display, requestor, False, NoEventMask, &event)
		    != True)
	    || (XNextEvent(display, &event) != Success)
	    || (event.type != SelectionNotify)
	    || (event.xselection.send_event != True)
	    || (event.xselection.requestor != requestor)
	    || (event.xselection.selection != selection)
	    || (event.xselection.target != target)
	    || (event.xselection.property != property)
	    || (event.xselection.time != requestTime)
	    || (XGetWindowProperty(display, requestor, property, 0,
		    (long)sizeof(selectionData), False, target, &actualType,
		    &actualFormat, &nitems, &bytesAfter, &data) != Success)
	    || (actualType != target) || (actualFormat != 8)
	    || (nitems != sizeof(selectionData)) || (bytesAfter != 0)
	    || (data == NULL)
	    || (memcmp(data, selectionData, sizeof(selectionData)) != 0)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    BrowserClearEventQueue();
    requestTime++;
    if (XConvertSelection(display, selection, target, None, requestor,
	    requestTime) != Success) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    if ((XNextEvent(display, &event) != Success)
	    || (event.type != SelectionRequest)
	    || (event.xselectionrequest.owner != owner)
	    || (event.xselectionrequest.requestor != requestor)
	    || (event.xselectionrequest.selection != selection)
	    || (event.xselectionrequest.target != target)
	    || (event.xselectionrequest.property != None)
	    || (event.xselectionrequest.time != requestTime)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    if (XChangeProperty(display, requestor, target, target, 8,
	    PropModeReplace, selectionData, (int)sizeof(selectionData))
	    != Success) {
	goto cleanup;
    }
    memset(&event, 0, sizeof(event));
    event.xselection.type = SelectionNotify;
    event.xselection.display = display;
    event.xselection.requestor = requestor;
    event.xselection.selection = selection;
    event.xselection.target = target;
    event.xselection.property = target;
    event.xselection.time = requestTime;
    if ((XSendEvent(display, requestor, False, NoEventMask, &event)
		    != True)
	    || (XNextEvent(display, &event) != Success)
	    || (event.type != SelectionNotify)
	    || (event.xselection.send_event != True)
	    || (event.xselection.requestor != requestor)
	    || (event.xselection.selection != selection)
	    || (event.xselection.target != target)
	    || (event.xselection.property != target)
	    || (event.xselection.time != requestTime)
	    || (XGetWindowProperty(display, requestor, target, 0,
		    (long)sizeof(selectionData), False, target, &actualType,
		    &actualFormat, &nitems, &bytesAfter, &data) != Success)
	    || (actualType != target) || (actualFormat != 8)
	    || (nitems != sizeof(selectionData)) || (bytesAfter != 0)
	    || (data == NULL)
	    || (memcmp(data, selectionData, sizeof(selectionData)) != 0)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    XFree(data);
    data = NULL;

    BrowserClearEventQueue();
    if ((XSetSelectionOwner(display, selection, owner, CurrentTime)
		    != Success)
	    || (XGetSelectionOwner(display, selection) != owner)) {
	goto cleanup;
    }
    requestTime = eventTime + 1;
    if (XConvertSelection(display, selection, target, property, requestor,
	    requestTime) != Success) {
	goto cleanup;
    }
    if (XDestroyWindow(display, owner) != Success) {
	goto cleanup;
    }
    owner = None;
    memset(&event, 0, sizeof(event));
    if ((XNextEvent(display, &event) != Success)
	    || (event.type != SelectionNotify)
	    || (event.xselection.requestor != requestor)
	    || (event.xselection.selection != selection)
	    || (event.xselection.target != target)
	    || (event.xselection.property != None)
	    || (event.xselection.time != requestTime)
	    || (XGetSelectionOwner(display, selection) != None)
	    || (XEventsQueued(display, QueuedAlready) != 0)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (data != NULL) {
	XFree(data);
    }
    BrowserClearEventQueue();
    if (owner != None) {
	(void)XDestroyWindow(display, owner);
    }
    if (requestor != None) {
	(void)XDestroyWindow(display, requestor);
    }
    BrowserClearEventQueue();
    return result;
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
	if (BrowserValidateWindowPixmapAttribute(&display, resourcePtr->depth,
		pixmap,
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
	if (BrowserValidateWindowPixmapAttribute(&display, resourcePtr->depth,
		pixmap,
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
    if (mask & (CWBackPixmap | CWBackPixel | CWBorderPixmap | CWBorderPixel
	    | CWCursor)) {
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

    changes.stack_mode = Above;
    return XConfigureWindow(display, window, CWStackMode, &changes);
}
int XLowerWindow(Display *display, Window window)
{
    XWindowChanges changes;

    changes.stack_mode = Below;
    return XConfigureWindow(display, window, CWStackMode, &changes);
}

int
XCirculateSubwindows(
    Display *display,
    Window window,
    int direction)
{
    X11WasmResource *parentPtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CIRCULATE_WINDOW);
    X11WasmResource *resourcePtr;
    int place = PlaceOnTop;

    if (parentPtr == NULL) {
	return Success;
    }
    if (!BrowserValueOrError(&display, direction,
	    (direction == RaiseLowest) || (direction == LowerHighest),
	    X11_WASM_XREQ_CIRCULATE_WINDOW)) {
	return Success;
    }
    resourcePtr = BrowserCirculateSubwindow(parentPtr, direction, &place);
    if (resourcePtr != NULL) {
	(void)BrowserQueueCirculateNotify(resourcePtr, place);
    }
    return Success;
}

int
XCirculateSubwindowsUp(
    Display *display,
    Window window)
{
    return XCirculateSubwindows(display, window, RaiseLowest);
}

int
XCirculateSubwindowsDown(
    Display *display,
    Window window)
{
    return XCirculateSubwindows(display, window, LowerHighest);
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
    X11WasmResource *parentPtr;
    Window oldParent;
    int wasMapped;

    if (resourcePtr == NULL) {
	return Success;
    }
    parentPtr = BrowserWindowResourceOrError(&display, parent,
	    X11_WASM_XREQ_REPARENT_WINDOW);
    if (parentPtr == NULL) {
	return Success;
    }
    if (((Window)resourcePtr->id == browserDisplay.rootWindow)
	    || BrowserWindowInTree(parent, window)) {
	(void)BrowserBadMatchOrError(&display, (XID)window,
		X11_WASM_XREQ_REPARENT_WINDOW);
	return Success;
    }
    if ((parentPtr->c_class == InputOnly)
	    && (resourcePtr->c_class != InputOnly)) {
	(void)BrowserBadMatchOrError(&display, (XID)parent,
		X11_WASM_XREQ_REPARENT_WINDOW);
	return Success;
    }
    oldParent = resourcePtr->parent;
    wasMapped = resourcePtr->mapped;
    if (wasMapped) {
	resourcePtr->mapped = 0;
	(void)X11WasmJsUnmapWindow(BrowserJsXId(window));
	(void)BrowserQueueUnmapNotify(resourcePtr, False);
    }
    BrowserUnlinkChild(resourcePtr);
    resourcePtr->parent = parent;
    resourcePtr->x = x;
    resourcePtr->y = y;
    BrowserLinkChildAtTop(resourcePtr, parent);
    (void)X11WasmJsReparentWindow(BrowserJsXId(window),
	    BrowserJsXId(parent), x, y);
    if (BrowserWindowInheritsStyle(resourcePtr)) {
	BrowserSyncWindowStyle(resourcePtr);
	BrowserSyncInheritedChildStyles(resourcePtr);
    }
    BrowserSyncTransientStacking();
    (void)BrowserQueueReparentNotify(resourcePtr, oldParent, parent, x, y);
    if (wasMapped) {
	resourcePtr->mapped = 1;
	(void)X11WasmJsMapWindow(BrowserJsXId(window));
	(void)BrowserQueueMapNotify(resourcePtr);
	BrowserNotifyVisibilityTree(resourcePtr, VisibilityUnobscured);
	if (resourcePtr->c_class != InputOnly) {
	    (void)BrowserQueueExpose(resourcePtr, 0, 0, resourcePtr->width,
		    resourcePtr->height);
	}
    }
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

static int
BrowserClipClearArea(
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int windowWidth,
    unsigned int windowHeight,
    int *xReturn,
    int *yReturn,
    unsigned int *widthReturn,
    unsigned int *heightReturn)
{
    long long left = (long long)x;
    long long top = (long long)y;
    long long right = (width == 0U) ? (long long)windowWidth
	    : left + (long long)width;
    long long bottom = (height == 0U) ? (long long)windowHeight
	    : top + (long long)height;

    if (left < 0) {
	left = 0;
    }
    if (top < 0) {
	top = 0;
    }
    if (right > (long long)windowWidth) {
	right = (long long)windowWidth;
    }
    if (bottom > (long long)windowHeight) {
	bottom = (long long)windowHeight;
    }
    if ((right <= left) || (bottom <= top)) {
	return 0;
    }
    if (xReturn != NULL) {
	*xReturn = (int)left;
    }
    if (yReturn != NULL) {
	*yReturn = (int)top;
    }
    if (widthReturn != NULL) {
	*widthReturn = (unsigned int)(right - left);
    }
    if (heightReturn != NULL) {
	*heightReturn = (unsigned int)(bottom - top);
    }
    return 1;
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
    int clearX;
    int clearY;
    unsigned int clearWidth;
    unsigned int clearHeight;
    int hasBackgroundPixel;
    Pixmap backgroundPixmap;

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserValidateBool(&display, exposures,
	    X11_WASM_XREQ_CLEAR_AREA)) {
	return Success;
    }
    if (!BrowserValidateCard16(&display, (long)width, 1,
	    X11_WASM_XREQ_CLEAR_AREA)
	    || !BrowserValidateCard16(&display, (long)height, 1,
		    X11_WASM_XREQ_CLEAR_AREA)) {
	return Success;
    }
    if (resourcePtr->c_class == InputOnly) {
	(void)BrowserBadMatchOrError(&display, (XID)window,
		X11_WASM_XREQ_CLEAR_AREA);
	return Success;
    }
    if (!BrowserClipClearArea(x, y, width, height, resourcePtr->width,
	    resourcePtr->height, &clearX, &clearY, &clearWidth,
	    &clearHeight)) {
	return Success;
    }
    BrowserResolveWindowBackgroundStyle(resourcePtr, &hasBackgroundPixel, NULL,
	    &backgroundPixmap);
    if (!hasBackgroundPixel && (backgroundPixmap == None)) {
	if (exposures) {
	    (void)BrowserQueueExpose(resourcePtr, clearX, clearY, clearWidth,
		    clearHeight);
	}
	return Success;
    }
    (void)X11WasmJsClearArea(BrowserJsXId(window), clearX, clearY, clearWidth,
	    clearHeight);
    if (exposures) {
	(void)BrowserQueueExpose(resourcePtr, clearX, clearY, clearWidth,
		clearHeight);
    } else {
	BrowserMarkResourceDamage(resourcePtr, clearX, clearY, clearWidth,
		clearHeight);
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

GContext
XGContextFromGC(
    GC gc)
{
    return BrowserGCContext(gc);
}

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
    if (!BrowserValidateCountedList(&display, pixels, npixels,
	    X11_WASM_XREQ_FREE_COLORS)) {
	return Success;
    }
    BrowserForgetColors(colormap, pixels, npixels, planes);
    return Success;
}
