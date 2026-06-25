#include "XlibInt.h"

int XLookupString(XKeyEvent *event, char *buffer, int bytes, KeySym *keySym,
	XComposeStatus *status)
{
    int index = ((event != NULL) && (event->state & ShiftMask)) ? 1 : 0;
    KeySym sym = XLookupKeysym(event, index);
    unsigned int codepoint;
    unsigned char latin1;
    int length;

    (void)status;
    if (keySym != NULL) *keySym = sym;
    if (((unsigned int)sym & 0xff000000U) == X11_WASM_UNICODE_KEYSYM_BASE) {
	codepoint = ((unsigned int)sym & 0x00ffffffU);
	length = BrowserUtf8FromCodepoint(codepoint, buffer, bytes);
	return length;
    }
    if (BrowserLatin1FromKeySym(sym, &latin1)) {
	if ((buffer == NULL) || (bytes <= 0)) {
	    return 0;
	}
	buffer[0] = (char)latin1;
	return 1;
    }
    return 0;
}

int
XmbLookupString(
    XIC inputContext,
    XKeyPressedEvent *event,
    char *buffer,
    int bytes,
    KeySym *keySym,
    Status *status)
{
    if (!BrowserICAllowsLookup(inputContext, event)) {
	if (keySym != NULL) {
	    *keySym = NoSymbol;
	}
	if (status != NULL) {
	    *status = XLookupNone;
	}
	return 0;
    }
    return BrowserLookupUtf8String(event, buffer, bytes, keySym, status);
}

int
Xutf8LookupString(
    XIC inputContext,
    XKeyPressedEvent *event,
    char *buffer,
    int bytes,
    KeySym *keySym,
    Status *status)
{
    return XmbLookupString(inputContext, event, buffer, bytes, keySym, status);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestXICLookupFocus(void)
{
    char buffer[8];
    char *savedLocaleModifiers = NULL;
    int result = 0;
    int lookupLength;
    KeySym lookupSym;
    Status lookupStatus;
    XIC inputContext = NULL;
    XIM inputMethod = NULL;
    XKeyPressedEvent lookupEvent;

    BrowserInitDisplay();
    savedLocaleModifiers = (localeModifiers != NULL)
	    ? BrowserDupString(localeModifiers) : NULL;
    (void)XSetLocaleModifiers("@im=x11wasm");
    inputMethod = XOpenIM(&browserDisplay.display, NULL, NULL, NULL);
    if (inputMethod == NULL) {
	goto cleanup;
    }
    inputContext = XCreateIC(inputMethod,
	    XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	    XNClientWindow, browserDisplay.rootWindow,
	    XNFocusWindow, browserDisplay.rootWindow,
	    NULL);
    if (inputContext == NULL) {
	goto cleanup;
    }
    memset(&lookupEvent, 0, sizeof(lookupEvent));
    lookupEvent.type = KeyPress;
    lookupEvent.display = &browserDisplay.display;
    lookupEvent.window = browserDisplay.rootWindow;
    lookupEvent.keycode = BrowserKeycodeFromKeySym(XK_a);

    lookupSym = XK_a;
    lookupStatus = XLookupBoth;
    lookupLength = Xutf8LookupString(inputContext, &lookupEvent, buffer,
	    (int)sizeof(buffer), &lookupSym, &lookupStatus);
    if ((lookupLength != 0) || (lookupStatus != XLookupNone)
	    || (lookupSym != NoSymbol)) {
	goto cleanup;
    }

    XSetICFocus(inputContext);
    memset(buffer, 0, sizeof(buffer));
    lookupSym = NoSymbol;
    lookupStatus = 0;
    lookupLength = Xutf8LookupString(inputContext, &lookupEvent, buffer,
	    (int)sizeof(buffer), &lookupSym, &lookupStatus);
    if ((lookupLength != 1) || (lookupStatus != XLookupBoth)
	    || (lookupSym != XK_a) || (buffer[0] != 'a')) {
	goto cleanup;
    }

    lookupEvent.window = (Window)0x7fffffebUL;
    lookupSym = XK_a;
    lookupStatus = XLookupBoth;
    lookupLength = Xutf8LookupString(inputContext, &lookupEvent, buffer,
	    (int)sizeof(buffer), &lookupSym, &lookupStatus);
    if ((lookupLength != 0) || (lookupStatus != XLookupNone)
	    || (lookupSym != NoSymbol)) {
	goto cleanup;
    }

    lookupEvent.window = browserDisplay.rootWindow;
    XUnsetICFocus(inputContext);
    lookupSym = XK_a;
    lookupStatus = XLookupBoth;
    lookupLength = XmbLookupString(inputContext, &lookupEvent, buffer,
	    (int)sizeof(buffer), &lookupSym, &lookupStatus);
    if ((lookupLength != 0) || (lookupStatus != XLookupNone)
	    || (lookupSym != NoSymbol)) {
	goto cleanup;
    }
    result = 1;

cleanup:
    if (inputContext != NULL) {
	XDestroyIC(inputContext);
    }
    if (inputMethod != NULL) {
	(void)XCloseIM(inputMethod);
    }
    if (localeModifiers != NULL) {
	ckfree(localeModifiers);
    }
    localeModifiers = savedLocaleModifiers;
    return result;
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestXICSpotLocationSync(
    unsigned int windowId,
    int phase)
{
    static XIM inputMethod = NULL;
    static XIC inputContext = NULL;
    static char *savedLocaleModifiers = NULL;
    static int savedLocaleModifiersValid = 0;
    Window window = (Window)windowId;
    XPoint spot;
    XVaNestedList preeditAttributes = NULL;
    int result = 0;

    BrowserInitDisplay();
    if ((phase < 0) || (phase > 2)) {
	return 0;
    }
    if ((phase == 0) || (phase == 1)) {
	if (inputContext != NULL) {
	    XDestroyIC(inputContext);
	    inputContext = NULL;
	}
	if (inputMethod != NULL) {
	    (void)XCloseIM(inputMethod);
	    inputMethod = NULL;
	}
	if (savedLocaleModifiersValid) {
	    if (localeModifiers != NULL) {
		ckfree(localeModifiers);
	    }
	    localeModifiers = savedLocaleModifiers;
	    savedLocaleModifiers = NULL;
	    savedLocaleModifiersValid = 0;
	}
	if (phase == 0) {
	    return 1;
	}
    }
    if (BrowserWindowResource(window) == NULL) {
	return 0;
    }
    if (inputContext == NULL) {
	savedLocaleModifiers = (localeModifiers != NULL)
		? BrowserDupString(localeModifiers) : NULL;
	savedLocaleModifiersValid = 1;
	(void)XSetLocaleModifiers("@im=x11wasm");
	inputMethod = XOpenIM(&browserDisplay.display, NULL, NULL, NULL);
	if (inputMethod == NULL) {
	    goto cleanup;
	}
	inputContext = XCreateIC(inputMethod,
		XNInputStyle, XIMPreeditPosition | XIMStatusNothing,
		XNClientWindow, window,
		XNFocusWindow, window,
		NULL);
	if (inputContext == NULL) {
	    goto cleanup;
	}
    }
    spot.x = (phase == 2) ? 23 : 11;
    spot.y = (phase == 2) ? 29 : 17;
    preeditAttributes = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
    if (XSetICValues(inputContext,
	    XNClientWindow, window,
	    XNFocusWindow, window,
	    XNPreeditAttributes, preeditAttributes,
	    NULL) != NULL) {
	goto cleanup;
    }
    XFree(preeditAttributes);
    preeditAttributes = NULL;
    XSetICFocus(inputContext);
    result = 1;

cleanup:
    if (preeditAttributes != NULL) {
	XFree(preeditAttributes);
    }
    if (!result) {
	if (inputContext != NULL) {
	    XDestroyIC(inputContext);
	    inputContext = NULL;
	}
	if (inputMethod != NULL) {
	    (void)XCloseIM(inputMethod);
	    inputMethod = NULL;
	}
	if (savedLocaleModifiersValid) {
	    if (localeModifiers != NULL) {
		ckfree(localeModifiers);
	    }
	    localeModifiers = savedLocaleModifiers;
	    savedLocaleModifiers = NULL;
	    savedLocaleModifiersValid = 0;
	}
    }
    return result;
}

X11WasmNestedList *
BrowserNestedListFromXVa(
    XVaNestedList list)
{
    X11WasmNestedList *listPtr = (X11WasmNestedList *)list;

    if ((listPtr == NULL)
	    || (listPtr->marker != X11_WASM_NESTED_LIST_MARKER)) {
	return NULL;
    }
    return listPtr;
}

char *
BrowserApplyICAttributes(
    X11WasmICAttributeSet *attributesPtr,
    XVaNestedList list,
    char *name)
{
    X11WasmNestedList *listPtr = BrowserNestedListFromXVa(list);

    if (list == NULL) {
	return NULL;
    }
    if ((attributesPtr == NULL) || (listPtr == NULL)) {
	return name;
    }
    if (listPtr->values.hasArea) {
	attributesPtr->area = listPtr->values.area;
	attributesPtr->hasArea = 1;
    }
    if (listPtr->values.hasAreaNeeded) {
	attributesPtr->areaNeeded = listPtr->values.areaNeeded;
	attributesPtr->hasAreaNeeded = 1;
    }
    if (listPtr->values.hasSpotLocation) {
	attributesPtr->spotLocation = listPtr->values.spotLocation;
	attributesPtr->hasSpotLocation = 1;
    }
    if (listPtr->values.hasForeground) {
	attributesPtr->foreground = listPtr->values.foreground;
	attributesPtr->hasForeground = 1;
    }
    if (listPtr->values.hasBackground) {
	attributesPtr->background = listPtr->values.background;
	attributesPtr->hasBackground = 1;
    }
    if (listPtr->values.hasFontSet) {
	attributesPtr->fontSet = listPtr->values.fontSet;
	attributesPtr->hasFontSet = 1;
    }
    if (listPtr->values.hasLineSpace) {
	attributesPtr->lineSpace = listPtr->values.lineSpace;
	attributesPtr->hasLineSpace = 1;
    }
    if (listPtr->values.hasCursor) {
	attributesPtr->cursor = listPtr->values.cursor;
	attributesPtr->hasCursor = 1;
    }
    return NULL;
}

char *
BrowserReadICAttributes(
    X11WasmICAttributeSet *attributesPtr,
    XVaNestedList list,
    char *name)
{
    X11WasmNestedList *listPtr = BrowserNestedListFromXVa(list);

    if (list == NULL) {
	return NULL;
    }
    if ((attributesPtr == NULL) || (listPtr == NULL)) {
	return name;
    }
    if ((listPtr->values.hasArea) && (listPtr->areaReturnPtr != NULL)) {
	if (attributesPtr->hasArea) {
	    *listPtr->areaReturnPtr = attributesPtr->area;
	} else {
	    memset(listPtr->areaReturnPtr, 0, sizeof(XRectangle));
	}
    }
    if ((listPtr->values.hasAreaNeeded)
	    && (listPtr->areaNeededReturnPtr != NULL)) {
	if (attributesPtr->hasAreaNeeded) {
	    *listPtr->areaNeededReturnPtr = attributesPtr->areaNeeded;
	} else {
	    memset(listPtr->areaNeededReturnPtr, 0, sizeof(XRectangle));
	}
    }
    if ((listPtr->values.hasSpotLocation)
	    && (listPtr->spotLocationReturnPtr != NULL)) {
	if (attributesPtr->hasSpotLocation) {
	    *listPtr->spotLocationReturnPtr = attributesPtr->spotLocation;
	} else {
	    memset(listPtr->spotLocationReturnPtr, 0, sizeof(XPoint));
	}
    }
    if ((listPtr->values.hasForeground)
	    && (listPtr->foregroundReturnPtr != NULL)) {
	*listPtr->foregroundReturnPtr = attributesPtr->hasForeground
		? attributesPtr->foreground : 0UL;
    }
    if ((listPtr->values.hasBackground)
	    && (listPtr->backgroundReturnPtr != NULL)) {
	*listPtr->backgroundReturnPtr = attributesPtr->hasBackground
		? attributesPtr->background : 0UL;
    }
    if ((listPtr->values.hasFontSet) && (listPtr->fontSetReturnPtr != NULL)) {
	*listPtr->fontSetReturnPtr = attributesPtr->hasFontSet
		? attributesPtr->fontSet : NULL;
    }
    if ((listPtr->values.hasLineSpace)
	    && (listPtr->lineSpaceReturnPtr != NULL)) {
	*listPtr->lineSpaceReturnPtr = attributesPtr->hasLineSpace
		? attributesPtr->lineSpace : 0;
    }
    if ((listPtr->values.hasCursor) && (listPtr->cursorReturnPtr != NULL)) {
	*listPtr->cursorReturnPtr = attributesPtr->hasCursor
		? attributesPtr->cursor : None;
    }
    return NULL;
}

int
BrowserLocaleModifiersDisableIM(void)
{
    return (localeModifiers != NULL)
	    && (strstr(localeModifiers, "@im=none") != NULL);
}

int
BrowserIMInstantiateCallbackExists(
    Display *display,
    XIDProc callback,
    XPointer clientData)
{
    X11WasmIMInstantiateCallback *callbackPtr;

    for (callbackPtr = imInstantiateCallbackList; callbackPtr != NULL;
	    callbackPtr = callbackPtr->nextPtr) {
	if ((callbackPtr->display == display)
		&& (callbackPtr->callback == callback)
		&& (callbackPtr->clientData == clientData)) {
	    return 1;
	}
    }
    return 0;
}

void
BrowserForgetIMInstantiateCallbacks(
    Display *display)
{
    X11WasmIMInstantiateCallback **linkPtr = &imInstantiateCallbackList;

    while (*linkPtr != NULL) {
	X11WasmIMInstantiateCallback *callbackPtr = *linkPtr;

	if (callbackPtr->display == display) {
	    *linkPtr = callbackPtr->nextPtr;
	    ckfree(callbackPtr);
	} else {
	    linkPtr = &callbackPtr->nextPtr;
	}
    }
}

void
BrowserNotifyIMInstantiateCallbacks(void)
{
    X11WasmIMInstantiateCallback *callbackPtr;
    X11WasmIMInstantiateCallback *snapshot = NULL;
    int count = 0;
    int i = 0;

    if (BrowserLocaleModifiersDisableIM()) {
	return;
    }
    for (callbackPtr = imInstantiateCallbackList; callbackPtr != NULL;
	    callbackPtr = callbackPtr->nextPtr) {
	count++;
    }
    if (count == 0) {
	return;
    }
    snapshot = (X11WasmIMInstantiateCallback *)ckalloc(
	    sizeof(X11WasmIMInstantiateCallback) * (size_t)count);
    for (callbackPtr = imInstantiateCallbackList; callbackPtr != NULL;
	    callbackPtr = callbackPtr->nextPtr) {
	snapshot[i++] = *callbackPtr;
    }
    for (i = 0; i < count; i++) {
	if (BrowserIMInstantiateCallbackExists(snapshot[i].display,
		snapshot[i].callback, snapshot[i].clientData)) {
	    snapshot[i].callback(snapshot[i].display, snapshot[i].clientData,
		    NULL);
	}
    }
    ckfree(snapshot);
}

XIM
XOpenIM(
    Display *display,
    XPointer rdb,
    char *resourceName,
    char *resourceClass)
{
    XIM inputMethod;

    (void)rdb;
    (void)resourceName;
    (void)resourceClass;
    if (display == NULL) {
	return NULL;
    }
    if (BrowserLocaleModifiersDisableIM()) {
	return NULL;
    }
    inputMethod = (XIM)ckalloc(sizeof(*inputMethod));
    memset(inputMethod, 0, sizeof(*inputMethod));
    inputMethod->display = display;
    inputMethod->styleCount = 2;
    inputMethod->styles[0] = XIMPreeditPosition | XIMStatusNothing;
    inputMethod->styles[1] = XIMPreeditNothing | XIMStatusNothing;
    return inputMethod;
}

Status
XCloseIM(
    XIM inputMethod)
{
    if (inputMethod != NULL) {
	if (inputMethod->hasDestroyCallback
		&& (inputMethod->destroyCallback.callback != NULL)) {
	    inputMethod->destroyCallback.callback(inputMethod,
		    inputMethod->destroyCallback.client_data, NULL);
	}
	ckfree(inputMethod);
    }
    return True;
}

char *
XSetLocaleModifiers(
    _Xconst char *modifierList)
{
    if (modifierList != NULL) {
	int wasDisabled = BrowserLocaleModifiersDisableIM();
	char *copy = BrowserDupString(modifierList);

	if (localeModifiers != NULL) {
	    ckfree(localeModifiers);
	}
	localeModifiers = copy;
	if (wasDisabled && !BrowserLocaleModifiersDisableIM()) {
	    BrowserNotifyIMInstantiateCallbacks();
	}
    }
    return (localeModifiers != NULL) ? localeModifiers : (char *)"";
}

Bool
XRegisterIMInstantiateCallback(
    Display *display,
    struct _XrmHashBucketRec *rdb,
    char *resourceName,
    char *resourceClass,
    XIDProc callback,
    XPointer clientData)
{
    X11WasmIMInstantiateCallback *callbackPtr;

    (void)rdb;
    (void)resourceName;
    (void)resourceClass;
    if ((display == NULL) || (callback == NULL)) {
	return False;
    }
    for (callbackPtr = imInstantiateCallbackList; callbackPtr != NULL;
	    callbackPtr = callbackPtr->nextPtr) {
	if ((callbackPtr->display == display)
		&& (callbackPtr->callback == callback)
		&& (callbackPtr->clientData == clientData)) {
	    return True;
	}
    }
    callbackPtr = (X11WasmIMInstantiateCallback *)ckalloc(
	    sizeof(X11WasmIMInstantiateCallback));
    memset(callbackPtr, 0, sizeof(X11WasmIMInstantiateCallback));
    callbackPtr->display = display;
    callbackPtr->callback = callback;
    callbackPtr->clientData = clientData;
    callbackPtr->nextPtr = imInstantiateCallbackList;
    imInstantiateCallbackList = callbackPtr;
    return True;
}

Bool
XUnregisterIMInstantiateCallback(
    Display *display,
    struct _XrmHashBucketRec *rdb,
    char *resourceName,
    char *resourceClass,
    XIDProc callback,
    XPointer clientData)
{
    X11WasmIMInstantiateCallback **linkPtr = &imInstantiateCallbackList;

    (void)rdb;
    (void)resourceName;
    (void)resourceClass;
    while (*linkPtr != NULL) {
	X11WasmIMInstantiateCallback *callbackPtr = *linkPtr;

	if ((callbackPtr->display == display)
		&& (callbackPtr->callback == callback)
		&& (callbackPtr->clientData == clientData)) {
	    *linkPtr = callbackPtr->nextPtr;
	    ckfree(callbackPtr);
	    return True;
	}
	linkPtr = &callbackPtr->nextPtr;
    }
    return False;
}

XIC
XCreateIC(
    XIM inputMethod,
    ...)
{
    XIC inputContext;
    va_list args;
    char *name;

    if (inputMethod == NULL) {
	return NULL;
    }
    inputContext = (XIC)ckalloc(sizeof(*inputContext));
    memset(inputContext, 0, sizeof(*inputContext));
    inputContext->inputMethod = inputMethod;
    inputContext->inputStyle = inputMethod->styles[0];
    inputContext->filterEvents = KeyPressMask | KeyReleaseMask
	    | FocusChangeMask;

    va_start(args, inputMethod);
    while ((name = va_arg(args, char *)) != NULL) {
	if (strcmp(name, XNInputStyle) == 0) {
	    inputContext->inputStyle = va_arg(args, XIMStyle);
	} else if (strcmp(name, XNClientWindow) == 0) {
	    inputContext->clientWindow = va_arg(args, Window);
	} else if (strcmp(name, XNFocusWindow) == 0) {
	    inputContext->focusWindow = va_arg(args, Window);
	} else if (strcmp(name, XNPreeditAttributes) == 0) {
	    (void)BrowserApplyICAttributes(&inputContext->preedit,
		    va_arg(args, XVaNestedList), name);
	} else if (strcmp(name, XNStatusAttributes) == 0) {
	    (void)BrowserApplyICAttributes(&inputContext->status,
		    va_arg(args, XVaNestedList), name);
	} else {
	    (void)va_arg(args, XPointer);
	}
    }
    va_end(args);

    if (!BrowserIMSupportsStyle(inputMethod, inputContext->inputStyle)) {
	ckfree(inputContext);
	return NULL;
    }
    return inputContext;
}

void
XDestroyIC(
    XIC inputContext)
{
    if (inputContext != NULL) {
	if (focusedInputContext == inputContext) {
	    BrowserClearInputContext();
	    focusedInputContext = NULL;
	}
	ckfree(inputContext);
    }
}

char *
XGetICValues(
    XIC inputContext,
    ...)
{
    va_list args;
    char *name;
    char *failedName = NULL;

    if (inputContext == NULL) {
	return (char *)"inputContext";
    }
    va_start(args, inputContext);
    while ((name = va_arg(args, char *)) != NULL) {
	XPointer valuePtr = va_arg(args, XPointer);

	if (strcmp(name, XNFilterEvents) == 0) {
	    if (valuePtr != NULL) {
		*((long *)valuePtr) = inputContext->filterEvents;
	    }
	} else if (strcmp(name, XNInputStyle) == 0) {
	    if (valuePtr != NULL) {
		*((XIMStyle *)valuePtr) = inputContext->inputStyle;
	    }
	} else if (strcmp(name, XNClientWindow) == 0) {
	    if (valuePtr != NULL) {
		*((Window *)valuePtr) = inputContext->clientWindow;
	    }
	} else if (strcmp(name, XNFocusWindow) == 0) {
	    if (valuePtr != NULL) {
		*((Window *)valuePtr) = inputContext->focusWindow;
	    }
	} else if (strcmp(name, XNPreeditAttributes) == 0) {
	    char *failed = BrowserReadICAttributes(&inputContext->preedit,
		    (XVaNestedList)valuePtr, name);

	    if ((failed != NULL) && (failedName == NULL)) {
		failedName = failed;
	    }
	} else if (strcmp(name, XNStatusAttributes) == 0) {
	    char *failed = BrowserReadICAttributes(&inputContext->status,
		    (XVaNestedList)valuePtr, name);

	    if ((failed != NULL) && (failedName == NULL)) {
		failedName = failed;
	    }
	} else if (failedName == NULL) {
	    failedName = name;
	}
    }
    va_end(args);
    return failedName;
}

char *
XSetICValues(
    XIC inputContext,
    ...)
{
    va_list args;
    char *name;
    char *failedName = NULL;

    if (inputContext == NULL) {
	return (char *)"inputContext";
    }
    va_start(args, inputContext);
    while ((name = va_arg(args, char *)) != NULL) {
	if (strcmp(name, XNInputStyle) == 0) {
	    XIMStyle style = va_arg(args, XIMStyle);

	    if (BrowserIMSupportsStyle(inputContext->inputMethod, style)) {
		inputContext->inputStyle = style;
	    } else if (failedName == NULL) {
		failedName = name;
	    }
	} else if (strcmp(name, XNClientWindow) == 0) {
	    inputContext->clientWindow = va_arg(args, Window);
	} else if (strcmp(name, XNFocusWindow) == 0) {
	    inputContext->focusWindow = va_arg(args, Window);
	} else if (strcmp(name, XNPreeditAttributes) == 0) {
	    char *failed = BrowserApplyICAttributes(&inputContext->preedit,
		    va_arg(args, XVaNestedList), name);

	    if ((failed != NULL) && (failedName == NULL)) {
		failedName = failed;
	    }
	} else if (strcmp(name, XNStatusAttributes) == 0) {
	    char *failed = BrowserApplyICAttributes(&inputContext->status,
		    va_arg(args, XVaNestedList), name);

	    if ((failed != NULL) && (failedName == NULL)) {
		failedName = failed;
	    }
	} else {
	    (void)va_arg(args, XPointer);
	    if (failedName == NULL) {
		failedName = name;
	    }
	}
    }
    va_end(args);
    BrowserSyncInputContext(inputContext);
    return failedName;
}

void
XSetICFocus(
    XIC inputContext)
{
    if (inputContext != NULL) {
	inputContext->focused = 1;
	focusedInputContext = inputContext;
	BrowserSyncInputContext(inputContext);
    }
}

void
XUnsetICFocus(
    XIC inputContext)
{
    if (inputContext != NULL) {
	inputContext->focused = 0;
	if (focusedInputContext == inputContext) {
	    BrowserClearInputContext();
	    focusedInputContext = NULL;
	}
    }
}

char *
XGetIMValues(
    XIM inputMethod,
    ...)
{
    va_list args;
    char *name;
    char *failedName = NULL;

    if (inputMethod == NULL) {
	return (char *)"inputMethod";
    }
    va_start(args, inputMethod);
    while ((name = va_arg(args, char *)) != NULL) {
	XPointer valuePtr = va_arg(args, XPointer);

	if (strcmp(name, XNQueryInputStyle) == 0) {
	    if (valuePtr != NULL) {
		size_t bytes = sizeof(XIMStyles)
			+ sizeof(XIMStyle) * inputMethod->styleCount;
		XIMStyles *styles = (XIMStyles *)ckalloc(bytes);

		styles->count_styles = inputMethod->styleCount;
		styles->supported_styles = (XIMStyle *)(styles + 1);
		memcpy(styles->supported_styles, inputMethod->styles,
			sizeof(XIMStyle) * inputMethod->styleCount);
		*((XIMStyles **)valuePtr) = styles;
	    }
	} else if (strcmp(name, XNDestroyCallback) == 0) {
	    if (valuePtr != NULL) {
		XIMCallback *callbackPtr = (XIMCallback *)valuePtr;

		if (inputMethod->hasDestroyCallback) {
		    *callbackPtr = inputMethod->destroyCallback;
		} else {
		    memset(callbackPtr, 0, sizeof(*callbackPtr));
		}
	    }
	} else if (failedName == NULL) {
	    failedName = name;
	}
    }
    va_end(args);
    return failedName;
}

char *
XSetIMValues(
    XIM inputMethod,
    ...)
{
    va_list args;
    char *name;
    char *failedName = NULL;

    if (inputMethod == NULL) {
	return (char *)"inputMethod";
    }
    va_start(args, inputMethod);
    while ((name = va_arg(args, char *)) != NULL) {
	if (strcmp(name, XNDestroyCallback) == 0) {
	    XIMCallback *callbackPtr = va_arg(args, XIMCallback *);

	    if (callbackPtr != NULL) {
		inputMethod->destroyCallback = *callbackPtr;
		inputMethod->hasDestroyCallback = 1;
	    } else {
		memset(&inputMethod->destroyCallback, 0,
			sizeof(inputMethod->destroyCallback));
		inputMethod->hasDestroyCallback = 0;
	    }
	} else {
	    (void)va_arg(args, XPointer);
	    if (failedName == NULL) {
		failedName = name;
	    }
	}
    }
    va_end(args);
    return failedName;
}

XVaNestedList
XVaCreateNestedList(
    int unused,
    ...)
{
    X11WasmNestedList *listPtr;
    va_list args;
    char *name;

    (void)unused;
    listPtr = (X11WasmNestedList *)ckalloc(sizeof(X11WasmNestedList));
    memset(listPtr, 0, sizeof(X11WasmNestedList));
    listPtr->marker = X11_WASM_NESTED_LIST_MARKER;
    va_start(args, unused);
    while ((name = va_arg(args, char *)) != NULL) {
	if (strcmp(name, XNArea) == 0) {
	    XRectangle *areaPtr = va_arg(args, XRectangle *);

	    if (areaPtr != NULL) {
		listPtr->values.area = *areaPtr;
		listPtr->areaReturnPtr = areaPtr;
		listPtr->values.hasArea = 1;
	    }
	} else if (strcmp(name, XNAreaNeeded) == 0) {
	    XRectangle *areaPtr = va_arg(args, XRectangle *);

	    if (areaPtr != NULL) {
		listPtr->values.areaNeeded = *areaPtr;
		listPtr->areaNeededReturnPtr = areaPtr;
		listPtr->values.hasAreaNeeded = 1;
	    }
	} else if (strcmp(name, XNSpotLocation) == 0) {
	    XPoint *spotPtr = va_arg(args, XPoint *);

	    if (spotPtr != NULL) {
		listPtr->values.spotLocation = *spotPtr;
		listPtr->spotLocationReturnPtr = spotPtr;
		listPtr->values.hasSpotLocation = 1;
	    }
	} else if (strcmp(name, XNForeground) == 0) {
	    unsigned long foreground = va_arg(args, unsigned long);

	    listPtr->values.foreground = foreground;
	    listPtr->foregroundReturnPtr = (unsigned long *)(uintptr_t)foreground;
	    listPtr->values.hasForeground = 1;
	} else if (strcmp(name, XNBackground) == 0) {
	    unsigned long background = va_arg(args, unsigned long);

	    listPtr->values.background = background;
	    listPtr->backgroundReturnPtr = (unsigned long *)(uintptr_t)background;
	    listPtr->values.hasBackground = 1;
	} else if (strcmp(name, XNFontSet) == 0) {
	    XPointer value = va_arg(args, XPointer);

	    if (value != NULL) {
		listPtr->values.fontSet = (XFontSet)value;
		listPtr->fontSetReturnPtr = (XFontSet *)value;
		listPtr->values.hasFontSet = 1;
	    }
	} else if (strcmp(name, XNLineSpace) == 0) {
	    int lineSpace = va_arg(args, int);

	    listPtr->values.lineSpace = lineSpace;
	    listPtr->lineSpaceReturnPtr =
		    (int *)(uintptr_t)(unsigned int)lineSpace;
	    listPtr->values.hasLineSpace = 1;
	} else if (strcmp(name, XNCursor) == 0) {
	    Cursor cursor = va_arg(args, Cursor);

	    listPtr->values.cursor = cursor;
	    listPtr->cursorReturnPtr = (Cursor *)(uintptr_t)cursor;
	    listPtr->values.hasCursor = 1;
	} else {
	    (void)va_arg(args, XPointer);
	}
    }
    va_end(args);
    return (XVaNestedList)listPtr;
}

XFontSet
XCreateFontSet(
    Display *display,
    _Xconst char *baseFontNameList,
    char ***missingCharsetListReturn,
    int *missingCharsetCountReturn,
    char **defStringReturn)
{
    XFontSet fontSet;
    char *firstFontName;
    const char *fontCursor;
    const char *localeName;
    int charWidth;
    int ascent;
    int descent;
    int fontCount;
    int i;

    if (display == NULL) {
	return NULL;
    }
    if (missingCharsetListReturn != NULL) {
	*missingCharsetListReturn = NULL;
    }
    if (missingCharsetCountReturn != NULL) {
	*missingCharsetCountReturn = 0;
    }
    if (defStringReturn != NULL) {
	*defStringReturn = (char *)"";
    }
    fontSet = (XFontSet)ckalloc(sizeof(*fontSet));
    memset(fontSet, 0, sizeof(*fontSet));
    fontSet->display = display;
    fontSet->baseFontNameList = BrowserDupString(
	    (baseFontNameList != NULL) ? baseFontNameList : "");
    localeName = setlocale(LC_CTYPE, NULL);
    fontSet->localeName = BrowserDupString(
	    (localeName != NULL) ? localeName : "C");
    firstFontName = BrowserFirstFontName(baseFontNameList);
    BrowserFontMetricsFromName(firstFontName, NULL, &charWidth, &ascent,
	    &descent);
    BrowserSetFontSetExtents(&fontSet->extents, charWidth, ascent, descent);
    fontCount = BrowserFontNameListCount(baseFontNameList);
    if (fontCount <= 0) {
	fontCount = 1;
    }
    fontSet->fontCount = fontCount;
    fontSet->fontStructList = (XFontStruct **)ckalloc(sizeof(XFontStruct *)
	    * (size_t)(fontCount + 1));
    fontSet->fontNameList = (char **)ckalloc(sizeof(char *)
	    * (size_t)(fontCount + 1));
    memset(fontSet->fontStructList, 0, sizeof(XFontStruct *)
	    * (size_t)(fontCount + 1));
    memset(fontSet->fontNameList, 0, sizeof(char *) * (size_t)(fontCount + 1));
    fontCursor = baseFontNameList;
    for (i = 0; i < fontCount; i++) {
	char *fontName = BrowserNextFontName(&fontCursor);

	if (fontName == NULL) {
	    fontName = BrowserDupString(firstFontName);
	}
	fontSet->fontNameList[i] = BrowserFontXLFD(fontName, 0);
	fontSet->fontStructList[i] = XLoadQueryFont(display, fontName);
	if (i == 0) {
	    fontSet->fontName = fontSet->fontNameList[i];
	    fontSet->fontStruct = fontSet->fontStructList[i];
	}
	ckfree(fontName);
    }
    ckfree(firstFontName);
    return fontSet;
}

void
XFreeFontSet(
    Display *display,
    XFontSet fontSet)
{
    (void)display;
    if (fontSet != NULL) {
	int i;

	for (i = 0; i < fontSet->fontCount; i++) {
	    if ((fontSet->fontStructList != NULL)
		    && (fontSet->fontStructList[i] != NULL)) {
		XFreeFont(display, fontSet->fontStructList[i]);
	    }
	    if ((fontSet->fontNameList != NULL)
		    && (fontSet->fontNameList[i] != NULL)) {
		ckfree(fontSet->fontNameList[i]);
	    }
	}
	if (fontSet->fontStructList != NULL) {
	    ckfree(fontSet->fontStructList);
	}
	if (fontSet->fontNameList != NULL) {
	    ckfree(fontSet->fontNameList);
	}
	if (fontSet->localeName != NULL) {
	    ckfree(fontSet->localeName);
	}
	if (fontSet->baseFontNameList != NULL) {
	    ckfree(fontSet->baseFontNameList);
	}
	ckfree(fontSet);
    }
}

XFontSetExtents *
XExtentsOfFontSet(
    XFontSet fontSet)
{
    return (fontSet != NULL) ? &fontSet->extents : NULL;
}

char *
XBaseFontNameListOfFontSet(
    XFontSet fontSet)
{
    return (fontSet != NULL) ? fontSet->baseFontNameList : NULL;
}

char *
XLocaleOfFontSet(
    XFontSet fontSet)
{
    return (fontSet != NULL) ? fontSet->localeName : NULL;
}

Bool
XContextDependentDrawing(
    XFontSet fontSet)
{
    (void)fontSet;
    return False;
}

Bool
XDirectionalDependentDrawing(
    XFontSet fontSet)
{
    (void)fontSet;
    return False;
}

Bool
XContextualDrawing(
    XFontSet fontSet)
{
    (void)fontSet;
    return False;
}

int
XFontsOfFontSet(
    XFontSet fontSet,
    XFontStruct ***fontStructListReturn,
    char ***fontNameListReturn)
{
    if (fontStructListReturn != NULL) {
	*fontStructListReturn = NULL;
    }
    if (fontNameListReturn != NULL) {
	*fontNameListReturn = NULL;
    }
    if ((fontSet == NULL) || (fontSet->fontCount <= 0)
	    || (fontSet->fontStructList == NULL)
	    || (fontSet->fontNameList == NULL)) {
	return 0;
    }
    if (fontStructListReturn != NULL) {
	*fontStructListReturn = fontSet->fontStructList;
    }
    if (fontNameListReturn != NULL) {
	*fontNameListReturn = fontSet->fontNameList;
    }
    return fontSet->fontCount;
}

int
BrowserTextByteCount(
    const char *text,
    int byteCount)
{
    if (text == NULL) {
	return 0;
    }
    return (byteCount >= 0) ? byteCount : (int)strlen(text);
}

int
BrowserUtf8SequenceLength(
    const char *text,
    int available)
{
    const unsigned char *bytes = (const unsigned char *)text;

    if ((text == NULL) || (available <= 0)) {
	return 0;
    }
    if (bytes[0] < 0x80U) {
	return 1;
    }
    if (((bytes[0] & 0xe0U) == 0xc0U) && (available >= 2)
	    && ((bytes[1] & 0xc0U) == 0x80U)) {
	return 2;
    }
    if (((bytes[0] & 0xf0U) == 0xe0U) && (available >= 3)
	    && ((bytes[1] & 0xc0U) == 0x80U)
	    && ((bytes[2] & 0xc0U) == 0x80U)) {
	return 3;
    }
    if (((bytes[0] & 0xf8U) == 0xf0U) && (available >= 4)
	    && ((bytes[1] & 0xc0U) == 0x80U)
	    && ((bytes[2] & 0xc0U) == 0x80U)
	    && ((bytes[3] & 0xc0U) == 0x80U)) {
	return 4;
    }
    return 1;
}

int
BrowserUtf8CharacterCount(
    const char *utf8,
    int length)
{
    int count = 0;
    int offset = 0;

    while (offset < length) {
	int charLength = BrowserUtf8SequenceLength(utf8 + offset,
		length - offset);

	if (charLength <= 0) {
	    break;
	}
	offset += charLength;
	count++;
    }
    return count;
}

void
BrowserFontSetMetrics(
    XFontSet fontSet,
    int *ascentPtr,
    int *descentPtr)
{
    int ascent = 11;
    int descent = 3;

    if ((fontSet != NULL) && (fontSet->fontStruct != NULL)) {
	ascent = fontSet->fontStruct->ascent;
	descent = fontSet->fontStruct->descent;
    } else if (fontSet != NULL) {
	XRectangle *logicalPtr = &fontSet->extents.max_logical_extent;

	if (logicalPtr->height > 0) {
	    ascent = -logicalPtr->y;
	    descent = (int)logicalPtr->height - ascent;
	}
    }
    if (ascentPtr != NULL) {
	*ascentPtr = (ascent > 0) ? ascent : 11;
    }
    if (descentPtr != NULL) {
	*descentPtr = (descent > 0) ? descent : 3;
    }
}

int
BrowserFontSetTextEscapement(
    XFontSet fontSet,
    const char *utf8,
    int byteCount)
{
    int length = BrowserTextByteCount(utf8, byteCount);

    if (length <= 0) {
	return 0;
    }
    return BrowserFontTextWidth(
	    (fontSet != NULL) ? fontSet->fontStruct : NULL, utf8, length);
}

int Xutf8TextEscapement(XFontSet fontSet, _Xconst char *text, int bytesText);
int Xutf8TextExtents(XFontSet fontSet, _Xconst char *text, int bytesText,
	XRectangle *overallInkReturn, XRectangle *overallLogicalReturn);
Status Xutf8TextPerCharExtents(XFontSet fontSet, _Xconst char *text,
	int bytesText, XRectangle *inkExtentsBuffer,
	XRectangle *logicalExtentsBuffer, int bufferSize, int *numChars,
	XRectangle *overallInkReturn, XRectangle *overallLogicalReturn);
void Xutf8DrawString(Display *display, Drawable drawable, XFontSet fontSet,
	GC gc, int x, int y, _Xconst char *text, int bytesText);
void Xutf8DrawImageString(Display *display, Drawable drawable,
	XFontSet fontSet, GC gc, int x, int y, _Xconst char *text,
	int bytesText);

int
BrowserFontSetTextExtents(
    XFontSet fontSet,
    const char *utf8,
    int byteCount,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn)
{
    int length = BrowserTextByteCount(utf8, byteCount);
    int width = BrowserFontSetTextEscapement(fontSet, utf8, length);
    int ascent;
    int descent;
    unsigned short rectWidth;
    unsigned short rectHeight;

    BrowserFontSetMetrics(fontSet, &ascent, &descent);
    rectWidth = BrowserClampUShort((unsigned int)width);
    rectHeight = BrowserClampUShort((unsigned int)(ascent + descent));
    if (overallInkReturn != NULL) {
	overallInkReturn->x = 0;
	overallInkReturn->y = BrowserClampShort(-ascent);
	overallInkReturn->width = rectWidth;
	overallInkReturn->height = rectHeight;
    }
    if (overallLogicalReturn != NULL) {
	overallLogicalReturn->x = 0;
	overallLogicalReturn->y = BrowserClampShort(-ascent);
	overallLogicalReturn->width = rectWidth;
	overallLogicalReturn->height = rectHeight;
    }
    return width;
}

Status
BrowserFontSetTextPerCharExtents(
    XFontSet fontSet,
    const char *utf8,
    int byteCount,
    XRectangle *inkExtentsBuffer,
    XRectangle *logicalExtentsBuffer,
    int bufferSize,
    int *numChars,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn)
{
    int length = BrowserTextByteCount(utf8, byteCount);
    int totalChars = BrowserUtf8CharacterCount(utf8, length);
    int ascent;
    int descent;
    int offset = 0;
    int x = 0;
    int i;

    if (numChars != NULL) {
	*numChars = totalChars;
    }
    (void)BrowserFontSetTextExtents(fontSet, utf8, length,
	    overallInkReturn, overallLogicalReturn);
    BrowserFontSetMetrics(fontSet, &ascent, &descent);
    for (i = 0; (i < totalChars) && (i < bufferSize); i++) {
	int charLength = BrowserUtf8SequenceLength(utf8 + offset,
		length - offset);
	int width = BrowserFontSetTextEscapement(fontSet, utf8 + offset,
		charLength);
	XRectangle rect;

	rect.x = BrowserClampShort(x);
	rect.y = BrowserClampShort(-ascent);
	rect.width = BrowserClampUShort((unsigned int)width);
	rect.height = BrowserClampUShort((unsigned int)(ascent + descent));
	if (inkExtentsBuffer != NULL) {
	    inkExtentsBuffer[i] = rect;
	}
	if (logicalExtentsBuffer != NULL) {
	    logicalExtentsBuffer[i] = rect;
	}
	x += width;
	offset += charLength;
    }
    return (bufferSize >= totalChars) ? True : False;
}

int
XmbTextEscapement(
    XFontSet fontSet,
    _Xconst char *text,
    int bytesText)
{
    return Xutf8TextEscapement(fontSet, text, bytesText);
}

int
XwcTextEscapement(
    XFontSet fontSet,
    _Xconst wchar_t *text,
    int numWchars)
{
    char *utf8;
    int utf8Length;
    int width;

    utf8 = BrowserUtf8FromWideStringCount(text, numWchars, &utf8Length);
    width = BrowserFontSetTextEscapement(fontSet, utf8, utf8Length);
    ckfree(utf8);
    return width;
}

int
Xutf8TextEscapement(
    XFontSet fontSet,
    _Xconst char *text,
    int bytesText)
{
    return BrowserFontSetTextEscapement(fontSet, text, bytesText);
}

int
XmbTextExtents(
    XFontSet fontSet,
    _Xconst char *text,
    int bytesText,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn)
{
    return Xutf8TextExtents(fontSet, text, bytesText, overallInkReturn,
	    overallLogicalReturn);
}

int
XwcTextExtents(
    XFontSet fontSet,
    _Xconst wchar_t *text,
    int numWchars,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn)
{
    char *utf8;
    int utf8Length;
    int width;

    utf8 = BrowserUtf8FromWideStringCount(text, numWchars, &utf8Length);
    width = BrowserFontSetTextExtents(fontSet, utf8, utf8Length,
	    overallInkReturn, overallLogicalReturn);
    ckfree(utf8);
    return width;
}

int
Xutf8TextExtents(
    XFontSet fontSet,
    _Xconst char *text,
    int bytesText,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn)
{
    return BrowserFontSetTextExtents(fontSet, text, bytesText,
	    overallInkReturn, overallLogicalReturn);
}

Status
XmbTextPerCharExtents(
    XFontSet fontSet,
    _Xconst char *text,
    int bytesText,
    XRectangle *inkExtentsBuffer,
    XRectangle *logicalExtentsBuffer,
    int bufferSize,
    int *numChars,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn)
{
    return Xutf8TextPerCharExtents(fontSet, text, bytesText,
	    inkExtentsBuffer, logicalExtentsBuffer, bufferSize, numChars,
	    overallInkReturn, overallLogicalReturn);
}

Status
XwcTextPerCharExtents(
    XFontSet fontSet,
    _Xconst wchar_t *text,
    int numWchars,
    XRectangle *inkExtentsBuffer,
    XRectangle *logicalExtentsBuffer,
    int bufferSize,
    int *numChars,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn)
{
    char *utf8;
    int utf8Length;
    Status status;

    utf8 = BrowserUtf8FromWideStringCount(text, numWchars, &utf8Length);
    status = BrowserFontSetTextPerCharExtents(fontSet, utf8, utf8Length,
	    inkExtentsBuffer, logicalExtentsBuffer, bufferSize, numChars,
	    overallInkReturn, overallLogicalReturn);
    ckfree(utf8);
    return status;
}

Status
Xutf8TextPerCharExtents(
    XFontSet fontSet,
    _Xconst char *text,
    int bytesText,
    XRectangle *inkExtentsBuffer,
    XRectangle *logicalExtentsBuffer,
    int bufferSize,
    int *numChars,
    XRectangle *overallInkReturn,
    XRectangle *overallLogicalReturn)
{
    return BrowserFontSetTextPerCharExtents(fontSet, text, bytesText,
	    inkExtentsBuffer, logicalExtentsBuffer, bufferSize, numChars,
	    overallInkReturn, overallLogicalReturn);
}

void
BrowserDrawUtf8ImageText(
    Display *display,
    Drawable drawable,
    XFontSet fontSet,
    GC gc,
    int x,
    int y,
    const char *utf8,
    int length)
{
    X11WasmResource *resourcePtr;
    int width;
    int ascent;
    int descent;

    if ((utf8 == NULL) || (length <= 0)) {
	return;
    }
    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_POLY_TEXT_8);
    if (resourcePtr == NULL) {
	return;
    }
    if (!BrowserOptionalGCOrError(&display, gc, X11_WASM_XREQ_POLY_TEXT_8)) {
	return;
    }
    width = BrowserFontSetTextEscapement(fontSet, utf8, length);
    BrowserFontSetMetrics(fontSet, &ascent, &descent);
    BrowserApplyGCStyle(gc);
    BrowserApplyGCClip(gc);
    (void)X11WasmJsDrawRectangle(BrowserJsXId(drawable),
	    (gc != NULL) ? gc->background : 0, x, y - ascent,
	    (unsigned int)width, (unsigned int)(ascent + descent), 1);
    (void)X11WasmJsDrawText(BrowserJsXId(drawable), BrowserGCPixel(gc), x,
	    y, utf8, length);
    BrowserMarkResourceDamage(resourcePtr, x, y - ascent,
	    (unsigned int)(width + 2), (unsigned int)(ascent + descent));
}

void
XmbDrawString(
    Display *display,
    Drawable drawable,
    XFontSet fontSet,
    GC gc,
    int x,
    int y,
    _Xconst char *text,
    int bytesText)
{
    Xutf8DrawString(display, drawable, fontSet, gc, x, y, text, bytesText);
}

void
XwcDrawString(
    Display *display,
    Drawable drawable,
    XFontSet fontSet,
    GC gc,
    int x,
    int y,
    _Xconst wchar_t *text,
    int numWchars)
{
    char *utf8;
    int utf8Length;

    utf8 = BrowserUtf8FromWideStringCount(text, numWchars, &utf8Length);
    Xutf8DrawString(display, drawable, fontSet, gc, x, y, utf8, utf8Length);
    ckfree(utf8);
}

void
Xutf8DrawString(
    Display *display,
    Drawable drawable,
    XFontSet fontSet,
    GC gc,
    int x,
    int y,
    _Xconst char *text,
    int bytesText)
{
    int length = BrowserTextByteCount(text, bytesText);

    (void)fontSet;
    (void)BrowserDrawUtf8Text(display, drawable, gc, x, y, text, length,
	    X11_WASM_XREQ_POLY_TEXT_8);
}

void
XmbDrawImageString(
    Display *display,
    Drawable drawable,
    XFontSet fontSet,
    GC gc,
    int x,
    int y,
    _Xconst char *text,
    int bytesText)
{
    Xutf8DrawImageString(display, drawable, fontSet, gc, x, y, text,
	    bytesText);
}

void
XwcDrawImageString(
    Display *display,
    Drawable drawable,
    XFontSet fontSet,
    GC gc,
    int x,
    int y,
    _Xconst wchar_t *text,
    int numWchars)
{
    char *utf8;
    int utf8Length;

    utf8 = BrowserUtf8FromWideStringCount(text, numWchars, &utf8Length);
    Xutf8DrawImageString(display, drawable, fontSet, gc, x, y, utf8,
	    utf8Length);
    ckfree(utf8);
}

void
Xutf8DrawImageString(
    Display *display,
    Drawable drawable,
    XFontSet fontSet,
    GC gc,
    int x,
    int y,
    _Xconst char *text,
    int bytesText)
{
    int length = BrowserTextByteCount(text, bytesText);

    BrowserDrawUtf8ImageText(display, drawable, fontSet, gc, x, y, text,
	    length);
}

void
XmbDrawText(
    Display *display,
    Drawable drawable,
    GC gc,
    int x,
    int y,
    XmbTextItem *items,
    int itemCount)
{
    int i;

    if ((items == NULL) || (itemCount <= 0)) {
	return;
    }
    for (i = 0; i < itemCount; i++) {
	int length = BrowserTextByteCount(items[i].chars, items[i].nchars);
	XFontSet fontSet = (items[i].font_set != NULL)
		? items[i].font_set : NULL;

	Xutf8DrawString(display, drawable, fontSet, gc, x, y,
		items[i].chars, length);
	x += BrowserFontSetTextEscapement(fontSet, items[i].chars, length)
		+ items[i].delta;
    }
}

void
XwcDrawText(
    Display *display,
    Drawable drawable,
    GC gc,
    int x,
    int y,
    XwcTextItem *items,
    int itemCount)
{
    int i;

    if ((items == NULL) || (itemCount <= 0)) {
	return;
    }
    for (i = 0; i < itemCount; i++) {
	XFontSet fontSet = (items[i].font_set != NULL)
		? items[i].font_set : NULL;

	XwcDrawString(display, drawable, fontSet, gc, x, y, items[i].chars,
		items[i].nchars);
	x += XwcTextEscapement(fontSet, items[i].chars, items[i].nchars)
		+ items[i].delta;
    }
}

void
Xutf8DrawText(
    Display *display,
    Drawable drawable,
    GC gc,
    int x,
    int y,
    XmbTextItem *items,
    int itemCount)
{
    XmbDrawText(display, drawable, gc, x, y, items, itemCount);
}

void
XFreeStringList(
    char **list)
{
    (void)XFreeFontNames(list);
}

void
XwcFreeStringList(
    wchar_t **list)
{
    int i;

    if (list == NULL) {
	return;
    }
    for (i = 0; list[i] != NULL; i++) {
	ckfree(list[i]);
    }
    ckfree(list);
}

Status
BrowserTextPropertyToStringList(
    const XTextProperty *textProp,
    char ***listReturn,
    int *countReturn)
{
    char **list;
    unsigned long start = 0;
    unsigned long i;
    int count = 1;
    int out = 0;

    if (listReturn != NULL) {
	*listReturn = NULL;
    }
    if (countReturn != NULL) {
	*countReturn = 0;
    }
    if ((textProp == NULL) || (textProp->format != 8)
	    || ((textProp->nitems > 0) && (textProp->value == NULL))) {
	return False;
    }
    if (textProp->nitems == 0) {
	return True;
    }
    for (i = 0; i < textProp->nitems; i++) {
	if (textProp->value[i] == '\0') {
	    count++;
	}
    }
    list = (char **)ckalloc(sizeof(char *) * ((size_t)count + 1U));
    for (i = 0; i <= textProp->nitems; i++) {
	if ((i == textProp->nitems) || (textProp->value[i] == '\0')) {
	    unsigned long length = i - start;

	    list[out] = (char *)ckalloc((size_t)length + 1U);
	    if (length > 0) {
		memcpy(list[out], textProp->value + start, (size_t)length);
	    }
	    list[out][length] = '\0';
	    out++;
	    start = i + 1U;
	}
    }
    list[out] = NULL;
    if (listReturn != NULL) {
	*listReturn = list;
    } else {
	XFreeStringList(list);
    }
    if (countReturn != NULL) {
	*countReturn = count;
    }
    return True;
}

const char *
BrowserTextPropertyEncodingName(
    Atom encoding)
{
    if (encoding == XA_STRING) {
	return "iso8859-1";
    }
    if (encoding == BrowserUtf8StringAtom()) {
	return "utf-8";
    }
    if (encoding == BrowserCompoundTextAtom()) {
	return "iso2022";
    }
    return NULL;
}

char *
BrowserUtf8StringFromLatin1(
    const char *source)
{
    const unsigned char *p;
    char *result;
    char *out;

    if (source == NULL) {
	source = "";
    }
    result = (char *)ckalloc((strlen(source) * 2U) + 1U);
    out = result;
    for (p = (const unsigned char *)source; *p != '\0'; p++) {
	if (*p < 0x80) {
	    *out++ = (char)*p;
	} else {
	    *out++ = (char)(0xc0 | (*p >> 6));
	    *out++ = (char)(0x80 | (*p & 0x3f));
	}
    }
    *out = '\0';
    return result;
}

Status
BrowserTextPropertyToUtf8StringList(
    const XTextProperty *textProp,
    char ***listReturn,
    int *countReturn)
{
    const char *encodingName;
    X11WasmEncoding encoding;
    char **externalList = NULL;
    char **utf8List = NULL;
    int externalCount = 0;
    int i;

    if (listReturn != NULL) {
	*listReturn = NULL;
    }
    if (countReturn != NULL) {
	*countReturn = 0;
    }
    if ((textProp == NULL) || (textProp->format != 8)
	    || ((textProp->nitems > 0) && (textProp->value == NULL))) {
	return False;
    }
    if (textProp->encoding == BrowserUtf8StringAtom()) {
	return BrowserTextPropertyToStringList(textProp, listReturn,
		countReturn);
    }
    encodingName = BrowserTextPropertyEncodingName(textProp->encoding);
    if (encodingName == NULL) {
	return False;
    }
    if (textProp->nitems == 0) {
	return True;
    }
    if (textProp->encoding == XA_STRING) {
	if (!BrowserTextPropertyToStringList(textProp, &externalList,
		&externalCount)) {
	    return False;
	}
	utf8List = (char **)ckalloc(sizeof(char *) * ((size_t)externalCount
		+ 1U));
	for (i = 0; i < externalCount; i++) {
	    utf8List[i] = BrowserUtf8StringFromLatin1(externalList[i]);
	}
	utf8List[externalCount] = NULL;
	XFreeStringList(externalList);
	if (listReturn != NULL) {
	    *listReturn = utf8List;
	} else {
	    XFreeStringList(utf8List);
	}
	if (countReturn != NULL) {
	    *countReturn = externalCount;
	}
	return True;
    }
    encoding = X11WasmGetEncoding(encodingName);
    if (encoding == NULL) {
	return False;
    }
    if (!BrowserTextPropertyToStringList(textProp, &externalList,
	    &externalCount)) {
	X11WasmFreeEncoding(encoding);
	return False;
    }
    utf8List = (char **)ckalloc(sizeof(char *) * ((size_t)externalCount
	    + 1U));
    for (i = 0; i < externalCount; i++) {
	X11WasmDString utf8;
	int length;

	X11WasmExternalToUtfDString(encoding,
		(externalList[i] != NULL) ? externalList[i] : "", -1, &utf8);
	length = X11WasmDStringLength(&utf8);
	utf8List[i] = (char *)ckalloc((size_t)length + 1U);
	if (length > 0) {
	    memcpy(utf8List[i], X11WasmDStringValue(&utf8), (size_t)length);
	}
	utf8List[i][length] = '\0';
	X11WasmDStringFree(&utf8);
    }
    utf8List[externalCount] = NULL;
    X11WasmFreeEncoding(encoding);
    XFreeStringList(externalList);
    if (listReturn != NULL) {
	*listReturn = utf8List;
    } else {
	XFreeStringList(utf8List);
    }
    if (countReturn != NULL) {
	*countReturn = externalCount;
    }
    return True;
}

Status
XTextPropertyToStringList(
    XTextProperty *textProp,
    char ***listReturn,
    int *countReturn)
{
    if ((textProp == NULL) || (textProp->encoding != XA_STRING)) {
	if (listReturn != NULL) {
	    *listReturn = NULL;
	}
	if (countReturn != NULL) {
	    *countReturn = 0;
	}
	return False;
    }
    return BrowserTextPropertyToStringList(textProp, listReturn, countReturn);
}

int
XmbTextPropertyToTextList(
    Display *display,
    const XTextProperty *textProp,
    char ***listReturn,
    int *countReturn)
{
    (void)display;
    if ((textProp == NULL) || !BrowserPropertyIsTextEncoding(textProp->encoding)
	    || !BrowserTextPropertyToUtf8StringList(textProp, listReturn,
		    countReturn)) {
	return XConverterNotFound;
    }
    return Success;
}

int
Xutf8TextPropertyToTextList(
    Display *display,
    const XTextProperty *textProp,
    char ***listReturn,
    int *countReturn)
{
    return XmbTextPropertyToTextList(display, textProp, listReturn,
	    countReturn);
}

int
XwcTextPropertyToTextList(
    Display *display,
    const XTextProperty *textProp,
    wchar_t ***listReturn,
    int *countReturn)
{
    char **utf8List = NULL;
    wchar_t **wideList;
    int utf8Count = 0;
    int result;
    int i;

    if (listReturn != NULL) {
	*listReturn = NULL;
    }
    if (countReturn != NULL) {
	*countReturn = 0;
    }
    result = Xutf8TextPropertyToTextList(display, textProp, &utf8List,
	    &utf8Count);
    if (result != Success) {
	return result;
    }
    wideList = (wchar_t **)ckalloc(sizeof(wchar_t *)
	    * ((size_t)utf8Count + 1U));
    for (i = 0; i < utf8Count; i++) {
	wideList[i] = BrowserWideStringFromUtf8(utf8List[i]);
    }
    wideList[utf8Count] = NULL;
    XFreeStringList(utf8List);
    if (listReturn != NULL) {
	*listReturn = wideList;
    } else {
	XwcFreeStringList(wideList);
    }
    if (countReturn != NULL) {
	*countReturn = utf8Count;
    }
    return Success;
}

int BrowserUtf8ListFitsLatin1(char **list, int count);

Atom
BrowserTextPropertyEncodingForStyle(
    XICCEncodingStyle style,
    char **list,
    int count)
{
    switch (style) {
    case XStringStyle:
    case XTextStyle:
	return XA_STRING;
    case XStdICCTextStyle:
	return BrowserUtf8ListFitsLatin1(list, count) ? XA_STRING
		: BrowserCompoundTextAtom();
    case XCompoundTextStyle:
	return BrowserCompoundTextAtom();
    case XUTF8StringStyle:
	return BrowserUtf8StringAtom();
    default:
	return None;
    }
}

int
BrowserRawTextListToTextProperty(
    char **list,
    int count,
    Atom encoding,
    XTextProperty *textPropReturn)
{
    size_t byteLength = 0;
    size_t offset = 0;
    int i;

    if (textPropReturn == NULL) {
	return XNoMemory;
    }
    memset(textPropReturn, 0, sizeof(*textPropReturn));
    if (encoding == None) {
	return XConverterNotFound;
    }
    if ((list != NULL) && (count > 0)) {
	for (i = 0; i < count; i++) {
	    if (list[i] != NULL) {
		byteLength += strlen(list[i]);
	    }
	}
	byteLength += (size_t)count - 1U;
    }
    textPropReturn->value = (unsigned char *)ckalloc(byteLength + 1U);
    if ((list != NULL) && (count > 0)) {
	for (i = 0; i < count; i++) {
	    size_t length = (list[i] != NULL) ? strlen(list[i]) : 0U;

	    if (length > 0) {
		memcpy(textPropReturn->value + offset, list[i], length);
	    }
	    offset += length;
	    if (i < count - 1) {
		textPropReturn->value[offset++] = '\0';
	    }
	}
    }
    textPropReturn->value[byteLength] = '\0';
    textPropReturn->encoding = encoding;
    textPropReturn->format = 8;
    textPropReturn->nitems = (unsigned long)byteLength;
    return Success;
}

int
BrowserUtf8StringFitsLatin1(
    const char *string)
{
    const char *p;

    if (string == NULL) {
	return 1;
    }
    for (p = string; *p != '\0'; ) {
	X11WasmCodepoint ch;
	int length = X11WasmUtfToCodepoint(p, &ch);

	if ((length <= 0) || (ch > 0xff)) {
	    return 0;
	}
	p += length;
    }
    return 1;
}

int
BrowserUtf8ListFitsLatin1(
    char **list,
    int count)
{
    int i;

    if ((list == NULL) || (count <= 0)) {
	return 1;
    }
    for (i = 0; i < count; i++) {
	if (!BrowserUtf8StringFitsLatin1(list[i])) {
	    return 0;
	}
    }
    return 1;
}

void
BrowserFreeTemporaryStringList(
    char **list,
    int count)
{
    int i;

    if (list == NULL) {
	return;
    }
    for (i = 0; i < count; i++) {
	if (list[i] != NULL) {
	    ckfree(list[i]);
	}
    }
    ckfree(list);
}

char *
BrowserLatin1StringFromUtf8(
    const char *source)
{
    const char *p;
    char *result;
    char *out;

    if (source == NULL) {
	source = "";
    }
    result = (char *)ckalloc(strlen(source) + 1U);
    out = result;
    for (p = source; *p != '\0'; ) {
	X11WasmCodepoint ch;
	int length = X11WasmUtfToCodepoint(p, &ch);

	if ((length <= 0) || (ch > 0xff)) {
	    ckfree(result);
	    return NULL;
	}
	*out++ = (char)(unsigned char)ch;
	p += length;
    }
    *out = '\0';
    return result;
}

int
BrowserConvertUtf8ListToEncoding(
    char **list,
    int count,
    Atom encoding,
    char ***convertedListPtr)
{
    const char *encodingName;
    X11WasmEncoding browserEncoding;
    char **convertedList = NULL;
    int i;

    *convertedListPtr = NULL;
    if ((list == NULL) || (count <= 0)) {
	return Success;
    }
    if ((encoding == XA_STRING) && !BrowserUtf8ListFitsLatin1(list, count)) {
	return XConverterNotFound;
    }
    if (encoding == XA_STRING) {
	convertedList = (char **)ckalloc(sizeof(char *) * (size_t)count);
	memset(convertedList, 0, sizeof(char *) * (size_t)count);
	for (i = 0; i < count; i++) {
	    convertedList[i] = BrowserLatin1StringFromUtf8(list[i]);
	    if (convertedList[i] == NULL) {
		BrowserFreeTemporaryStringList(convertedList, count);
		return XConverterNotFound;
	    }
	}
	*convertedListPtr = convertedList;
	return Success;
    }
    encodingName = BrowserTextPropertyEncodingName(encoding);
    if (encodingName == NULL) {
	return XConverterNotFound;
    }
    browserEncoding = X11WasmGetEncoding(encodingName);
    if (browserEncoding == NULL) {
	return XConverterNotFound;
    }
    convertedList = (char **)ckalloc(sizeof(char *) * (size_t)count);
    memset(convertedList, 0, sizeof(char *) * (size_t)count);
    for (i = 0; i < count; i++) {
	X11WasmDString external;
	const char *source = (list[i] != NULL) ? list[i] : "";
	int length;

	X11WasmUtfToExternalDString(browserEncoding, source, -1, &external);
	length = X11WasmDStringLength(&external);
	convertedList[i] = (char *)ckalloc((size_t)length + 1U);
	if (length > 0) {
	    memcpy(convertedList[i], X11WasmDStringValue(&external),
		    (size_t)length);
	}
	convertedList[i][length] = '\0';
	X11WasmDStringFree(&external);
    }
    X11WasmFreeEncoding(browserEncoding);
    *convertedListPtr = convertedList;
    return Success;
}

int
BrowserTextListToTextProperty(
    Display *display,
    char **list,
    int count,
    XICCEncodingStyle style,
    XTextProperty *textPropReturn)
{
    Atom encoding;
    char **convertedList = NULL;
    int result;

    (void)display;
    if (textPropReturn == NULL) {
	return XNoMemory;
    }
    memset(textPropReturn, 0, sizeof(*textPropReturn));
    encoding = BrowserTextPropertyEncodingForStyle(style, list, count);
    if (encoding == None) {
	return XConverterNotFound;
    }
    result = BrowserConvertUtf8ListToEncoding(list, count, encoding,
	    &convertedList);
    if (result != Success) {
	return result;
    }
    result = BrowserRawTextListToTextProperty(convertedList, count, encoding,
	    textPropReturn);
    BrowserFreeTemporaryStringList(convertedList, count);
    return result;
}

int
XmbTextListToTextProperty(
    Display *display,
    char **list,
    int count,
    XICCEncodingStyle style,
    XTextProperty *textPropReturn)
{
    return BrowserTextListToTextProperty(display, list, count, style,
	    textPropReturn);
}

int
XwcTextListToTextProperty(
    Display *display,
    wchar_t **list,
    int count,
    XICCEncodingStyle style,
    XTextProperty *textPropReturn)
{
    char **utf8List = NULL;
    int result;
    int i;

    if ((list != NULL) && (count > 0)) {
	utf8List = (char **)ckalloc(sizeof(char *) * (size_t)count);
	for (i = 0; i < count; i++) {
	    utf8List[i] = BrowserUtf8FromWideString(list[i]);
	}
    }
    result = BrowserTextListToTextProperty(display, utf8List, count, style,
	    textPropReturn);
    if (utf8List != NULL) {
	for (i = 0; i < count; i++) {
	    ckfree(utf8List[i]);
	}
	ckfree(utf8List);
    }
    return result;
}

int
Xutf8TextListToTextProperty(
    Display *display,
    char **list,
    int count,
    XICCEncodingStyle style,
    XTextProperty *textPropReturn)
{
    return BrowserTextListToTextProperty(display, list, count, style,
	    textPropReturn);
}

static XModifierKeymap *
BrowserAllocModifierMap(
    int maxKeyPerMod)
{
    XModifierKeymap *mapPtr;
    size_t mapSize;

    if (maxKeyPerMod < 0) {
	return NULL;
    }
    mapPtr = (XModifierKeymap *)ckalloc(sizeof(XModifierKeymap));
    mapPtr->max_keypermod = maxKeyPerMod;
    if (maxKeyPerMod == 0) {
	mapPtr->modifiermap = NULL;
	return mapPtr;
    }
    mapSize = sizeof(KeyCode) * X11_WASM_MODIFIER_COUNT
	    * (size_t)maxKeyPerMod;
    mapPtr->modifiermap = (KeyCode *)ckalloc(mapSize);
    memset(mapPtr->modifiermap, 0, mapSize);
    return mapPtr;
}

static XModifierKeymap *
BrowserResizeModifierMap(
    XModifierKeymap *mapPtr,
    int maxKeyPerMod)
{
    XModifierKeymap *newMapPtr;
    int oldMaxKeyPerMod;
    int modifier;
    int copyCount;

    if ((mapPtr == NULL) || (maxKeyPerMod < 0)) {
	return mapPtr;
    }
    if (mapPtr->max_keypermod == maxKeyPerMod) {
	return mapPtr;
    }
    newMapPtr = BrowserAllocModifierMap(maxKeyPerMod);
    if (newMapPtr == NULL) {
	return mapPtr;
    }
    oldMaxKeyPerMod = mapPtr->max_keypermod;
    copyCount = (oldMaxKeyPerMod < maxKeyPerMod)
	    ? oldMaxKeyPerMod : maxKeyPerMod;
    if ((mapPtr->modifiermap != NULL) && (newMapPtr->modifiermap != NULL)
	    && (copyCount > 0)) {
	for (modifier = 0; modifier < X11_WASM_MODIFIER_COUNT; modifier++) {
	    memcpy(newMapPtr->modifiermap + modifier * maxKeyPerMod,
		    mapPtr->modifiermap + modifier * oldMaxKeyPerMod,
		    sizeof(KeyCode) * (size_t)copyCount);
	}
    }
    XFreeModifiermap(mapPtr);
    return newMapPtr;
}

XModifierKeymap *
XNewModifiermap(
    int maxKeysPerMod)
{
    return BrowserAllocModifierMap(maxKeysPerMod);
}

XModifierKeymap *
XInsertModifiermapEntry(
    XModifierKeymap *mapPtr,
    KeyCode keycode,
    int modifier)
{
    KeyCode *row;
    int i;
    int maxKeyPerMod;

    if ((mapPtr == NULL) || (keycode == 0)
	    || (modifier < 0) || (modifier >= X11_WASM_MODIFIER_COUNT)) {
	return mapPtr;
    }
    maxKeyPerMod = mapPtr->max_keypermod;
    row = (maxKeyPerMod > 0) ? mapPtr->modifiermap
	    + modifier * maxKeyPerMod : NULL;
    for (i = 0; i < maxKeyPerMod; i++) {
	if (row[i] == keycode) {
	    return mapPtr;
	}
    }
    for (i = 0; i < maxKeyPerMod; i++) {
	if (row[i] == 0) {
	    row[i] = keycode;
	    return mapPtr;
	}
    }
    mapPtr = BrowserResizeModifierMap(mapPtr, maxKeyPerMod + 1);
    if ((mapPtr != NULL) && (mapPtr->modifiermap != NULL)
	    && (mapPtr->max_keypermod > maxKeyPerMod)) {
	mapPtr->modifiermap[modifier * mapPtr->max_keypermod
		+ maxKeyPerMod] = keycode;
    }
    return mapPtr;
}

XModifierKeymap *
XDeleteModifiermapEntry(
    XModifierKeymap *mapPtr,
    KeyCode keycode,
    int modifier)
{
    KeyCode *row;
    int i, out;
    int maxKeyPerMod;

    if ((mapPtr == NULL) || (mapPtr->modifiermap == NULL)
	    || (modifier < 0) || (modifier >= X11_WASM_MODIFIER_COUNT)) {
	return mapPtr;
    }
    maxKeyPerMod = mapPtr->max_keypermod;
    row = mapPtr->modifiermap + modifier * maxKeyPerMod;
    out = 0;
    for (i = 0; i < maxKeyPerMod; i++) {
	if (row[i] != keycode) {
	    row[out++] = row[i];
	}
    }
    while (out < maxKeyPerMod) {
	row[out++] = 0;
    }
    return mapPtr;
}

XModifierKeymap *
XGetModifierMapping(
    Display *display)
{
    XModifierKeymap *mapPtr;
    X11WasmDisplayState *state = &browserDisplay;
    int maxKeyPerMod;
    size_t mapSize;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    (void)display;

    maxKeyPerMod = state->modifierMapMaxKeyPerMod;
    mapPtr = BrowserAllocModifierMap(maxKeyPerMod);
    if (mapPtr == NULL) {
	return NULL;
    }
    mapSize = sizeof(KeyCode) * X11_WASM_MODIFIER_COUNT
	    * (size_t)maxKeyPerMod;
    if (mapSize > 0) {
	memcpy(mapPtr->modifiermap, state->modifierMap, mapSize);
    }
    return mapPtr;
}

int
XFreeModifiermap(
    XModifierKeymap *mapPtr)
{
    if (mapPtr != NULL) {
	if (mapPtr->modifiermap != NULL) {
	    ckfree(mapPtr->modifiermap);
	}
	ckfree(mapPtr);
    }
    return Success;
}

static int
BrowserModifierRowHasKey(
    const KeyCode *map,
    int maxKeyPerMod,
    KeyCode keycode)
{
    int slot;

    for (slot = 0; slot < maxKeyPerMod; slot++) {
	if (map[slot] == keycode) {
	    return 1;
	}
    }
    return 0;
}

static int
BrowserModifierRowsDiffer(
    const KeyCode *oldRow,
    int oldMaxKeyPerMod,
    const KeyCode *newRow,
    int newMaxKeyPerMod)
{
    int slot;

    for (slot = 0; slot < oldMaxKeyPerMod; slot++) {
	KeyCode keycode = oldRow[slot];

	if ((keycode != 0) && !BrowserModifierRowHasKey(newRow,
		newMaxKeyPerMod, keycode)) {
	    return 1;
	}
    }
    for (slot = 0; slot < newMaxKeyPerMod; slot++) {
	KeyCode keycode = newRow[slot];

	if ((keycode != 0) && !BrowserModifierRowHasKey(oldRow,
		oldMaxKeyPerMod, keycode)) {
	    return 1;
	}
    }
    return 0;
}

static int
BrowserKeycodeIsDown(
    KeyCode keycode)
{
    unsigned int index = ((unsigned int)keycode) >> 3;
    unsigned int mask = 1U << (((unsigned int)keycode) & 7U);

    if (index >= sizeof(browserDisplay.keymap)) {
	return 0;
    }
    return (((unsigned char)browserDisplay.keymap[index] & mask) != 0);
}

static int
BrowserModifierRowHasDownKey(
    const KeyCode *row,
    int maxKeyPerMod)
{
    int slot;

    for (slot = 0; slot < maxKeyPerMod; slot++) {
	KeyCode keycode = row[slot];

	if ((keycode != 0) && BrowserKeycodeIsDown(keycode)) {
	    return 1;
	}
    }
    return 0;
}

static int
BrowserModifierMappingIsBusy(
    const XModifierKeymap *mapPtr)
{
    int modifier;
    int oldMaxKeyPerMod = browserDisplay.modifierMapMaxKeyPerMod;
    int newMaxKeyPerMod = mapPtr->max_keypermod;

    for (modifier = 0; modifier < X11_WASM_MODIFIER_COUNT; modifier++) {
	const KeyCode *oldRow = (oldMaxKeyPerMod > 0)
		? browserDisplay.modifierMap + modifier * oldMaxKeyPerMod
		: NULL;
	const KeyCode *newRow = (newMaxKeyPerMod > 0)
		? mapPtr->modifiermap + modifier * newMaxKeyPerMod : NULL;

	if (!BrowserModifierRowsDiffer(oldRow, oldMaxKeyPerMod, newRow,
		newMaxKeyPerMod)) {
	    continue;
	}
	if (BrowserModifierRowHasDownKey(oldRow, oldMaxKeyPerMod)
		|| BrowserModifierRowHasDownKey(newRow, newMaxKeyPerMod)) {
	    return 1;
	}
    }
    return 0;
}

int
XSetModifierMapping(
    Display *display,
    XModifierKeymap *mapPtr)
{
    int maxKeyPerMod;
    int modifier;
    int slot;
    size_t mapSize;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if ((display != &browserDisplay.display) || (mapPtr == NULL)
	    || (mapPtr->max_keypermod < 0)) {
	(void)XNoOp(display);
	return MappingFailed;
    }
    maxKeyPerMod = mapPtr->max_keypermod;
    if (maxKeyPerMod > X11_WASM_MODIFIER_MAP_MAX_KEYPERMOD) {
	(void)BrowserReportXError(display, BadLength, 0,
		X11_WASM_XREQ_SET_MODIFIER_MAPPING, 0);
	return MappingFailed;
    }
    if ((maxKeyPerMod > 0) && (mapPtr->modifiermap == NULL)) {
	(void)BrowserValueOrError(&display, 0, 0,
		X11_WASM_XREQ_SET_MODIFIER_MAPPING);
	return MappingFailed;
    }
    for (modifier = 0; modifier < X11_WASM_MODIFIER_COUNT; modifier++) {
	for (slot = 0; slot < maxKeyPerMod; slot++) {
	    KeyCode keycode = mapPtr->modifiermap[modifier * maxKeyPerMod
		    + slot];
	    int compareModifier;
	    int compareSlot;

	    if (keycode == 0) {
		continue;
	    }
	    if (!BrowserValueOrError(&display, keycode,
		    (keycode >= display->min_keycode)
		    && (keycode <= display->max_keycode),
		    X11_WASM_XREQ_SET_MODIFIER_MAPPING)) {
		return MappingFailed;
	    }
	    for (compareModifier = modifier;
		    compareModifier < X11_WASM_MODIFIER_COUNT;
		    compareModifier++) {
		int startSlot = (compareModifier == modifier) ? slot + 1 : 0;

		for (compareSlot = startSlot; compareSlot < maxKeyPerMod;
			compareSlot++) {
		    if (mapPtr->modifiermap[compareModifier * maxKeyPerMod
			    + compareSlot] == keycode) {
			(void)XNoOp(display);
			return MappingFailed;
		    }
		}
	    }
	}
    }
    if (BrowserModifierMappingIsBusy(mapPtr)) {
	(void)XNoOp(display);
	return MappingBusy;
    }
    browserDisplay.modifierMapMaxKeyPerMod = maxKeyPerMod;
    memset(browserDisplay.modifierMap, 0, sizeof(browserDisplay.modifierMap));
    mapSize = sizeof(KeyCode) * X11_WASM_MODIFIER_COUNT
	    * (size_t)maxKeyPerMod;
    if (mapSize > 0) {
	memcpy(browserDisplay.modifierMap, mapPtr->modifiermap, mapSize);
    }
    browserDisplay.modifierMapSerial++;
    (void)BrowserQueueMappingNotify(MappingModifier, 0, 0);
    return XNoOp(display);
}

int XDisplayKeycodes(Display *display, int *minReturn, int *maxReturn)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (minReturn != NULL) {
	*minReturn = display->min_keycode;
    }
    if (maxReturn != NULL) {
	*maxReturn = display->max_keycode;
    }
    return Success;
}
int XRefreshKeyboardMapping(XMappingEvent *event)
{
    BrowserInitDisplay();
    if (event == NULL) {
	return Success;
    }
    if (event->request == MappingKeyboard) {
	BrowserClearKeycodeRange((unsigned int)event->first_keycode,
		(unsigned int)event->count);
    } else if (event->request == MappingModifier) {
	BrowserInitModifierMap(&browserDisplay);
    }
    return Success;
}
