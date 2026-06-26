#include "XlibInt.h"

static char *browserDefaultValue = NULL;

typedef struct BrowserXrmEntry {
    char *binding;
    char *value;
    char *type;
    unsigned int valueSize;
    struct BrowserXrmEntry *nextPtr;
} BrowserXrmEntry;

struct _XrmHashBucketRec {
    BrowserXrmEntry *firstEntryPtr;
    BrowserXrmEntry *lastEntryPtr;
};

typedef struct BrowserXrmComponent {
    const char *start;
    const char *end;
} BrowserXrmComponent;

typedef struct BrowserXrmBindingPart {
    const char *start;
    const char *end;
    char separator;
} BrowserXrmBindingPart;

Display *
XOpenDisplay(
    _Xconst char *displayName)
{
    return X11WasmGetDisplay(displayName);
}

char *
XDisplayName(
    _Xconst char *displayName)
{
    return (char *)((displayName != NULL && displayName[0] != '\0')
	    ? displayName : "browser");
}

static const char *
BrowserResourceManagerString(void)
{
    X11WasmProperty *propertyPtr;

    BrowserInitDisplay();
    propertyPtr = BrowserFindProperty(&browserDisplay.rootResource,
	    XA_RESOURCE_MANAGER);
    if ((propertyPtr == NULL) || (propertyPtr->type != XA_STRING)
	    || (propertyPtr->format != 8) || (propertyPtr->data == NULL)) {
	return NULL;
    }
    return (const char *)propertyPtr->data;
}

static const char *
BrowserDefaultProgramName(
    const char *program)
{
    const char *slash;
    const char *backslash;

    if (program == NULL) {
	return NULL;
    }
    slash = strrchr(program, '/');
    backslash = strrchr(program, '\\');
    if ((backslash != NULL) && ((slash == NULL) || (backslash > slash))) {
	slash = backslash;
    }
    return (slash != NULL) ? slash + 1 : program;
}

static const char *
BrowserSkipDefaultSpaces(
    const char *text,
    const char *end)
{
    while ((text < end) && isspace(UCHAR(*text))) {
	text++;
    }
    return text;
}

static const char *
BrowserTrimDefaultSpaces(
    const char *start,
    const char *end)
{
    while ((end > start) && isspace(UCHAR(end[-1]))) {
	end--;
    }
    return end;
}

static int
BrowserResourceBindingScore(
    const char *start,
    const char *end,
    const char *program,
    const char *option)
{
    size_t optionLength = strlen(option);
    size_t programLength = (program != NULL) ? strlen(program) : 0U;
    size_t bindingLength = (size_t)(end - start);
    const char *prefixEnd;
    const char *prefixStart;
    char separator;

    if ((optionLength == 0U) || (bindingLength < optionLength)
	    || (strncmp(end - optionLength, option, optionLength) != 0)) {
	return 0;
    }
    if (bindingLength == optionLength) {
	return 1;
    }
    separator = *(end - optionLength - 1U);
    if ((separator != '.') && (separator != '*')) {
	return 0;
    }
    prefixStart = start;
    prefixEnd = end - optionLength - 1;
    if ((prefixEnd == prefixStart)
	    || ((prefixEnd == prefixStart + 1) && (*prefixStart == '*'))) {
	return 2;
    }
    if ((programLength > 0U)
	    && ((size_t)(prefixEnd - prefixStart) == programLength)
	    && (strncmp(prefixStart, program, programLength) == 0)) {
	return (separator == '.') ? 4 : 3;
    }
    return 0;
}

static char *
BrowserRememberDefaultValue(
    const char *start,
    const char *end)
{
    size_t length = (size_t)(end - start);

    if (browserDefaultValue != NULL) {
	ckfree(browserDefaultValue);
	browserDefaultValue = NULL;
    }
    browserDefaultValue = (char *)ckalloc(length + 1U);
    if (length > 0U) {
	memcpy(browserDefaultValue, start, length);
    }
    browserDefaultValue[length] = '\0';
    return browserDefaultValue;
}

static char *
BrowserDupRange(
    const char *start,
    const char *end)
{
    size_t length = (size_t)(end - start);
    char *copy = (char *)ckalloc(length + 1U);

    if (length > 0U) {
	memcpy(copy, start, length);
    }
    copy[length] = '\0';
    return copy;
}

static XrmDatabase
BrowserXrmNewDatabase(void)
{
    XrmDatabase database = (XrmDatabase)ckalloc(sizeof(*database));

    database->firstEntryPtr = NULL;
    database->lastEntryPtr = NULL;
    return database;
}

static char *
BrowserDupValue(
    const XrmValue *valuePtr,
    unsigned int *sizePtr)
{
    unsigned int size = 0U;
    char *copy;

    if (valuePtr != NULL) {
	size = valuePtr->size;
    }
    copy = (char *)ckalloc((size_t)size + 1U);
    if ((size > 0U) && (valuePtr != NULL) && (valuePtr->addr != NULL)) {
	memcpy(copy, valuePtr->addr, (size_t)size);
    } else if (size > 0U) {
	memset(copy, 0, (size_t)size);
    }
    copy[size] = '\0';
    if (sizePtr != NULL) {
	*sizePtr = size;
    }
    return copy;
}

static int
BrowserXrmComponentCount(
    const char *text)
{
    int count = 0;
    const char *p;

    if (text == NULL) {
	return 0;
    }
    for (p = text; *p != '\0'; ) {
	const char *start;

	while ((*p == '.') || (*p == '*')) {
	    p++;
	}
	start = p;
	while ((*p != '\0') && (*p != '.') && (*p != '*')) {
	    p++;
	}
	if (p > start) {
	    count++;
	}
    }
    return count;
}

static BrowserXrmComponent *
BrowserXrmSplitComponents(
    const char *text,
    int *countPtr)
{
    BrowserXrmComponent *parts;
    int count;
    int index = 0;
    const char *p;

    count = BrowserXrmComponentCount(text);
    if (countPtr != NULL) {
	*countPtr = count;
    }
    if (count == 0) {
	return NULL;
    }
    parts = (BrowserXrmComponent *)ckalloc(sizeof(*parts) * (size_t)count);
    for (p = text; *p != '\0'; ) {
	const char *start;

	while ((*p == '.') || (*p == '*')) {
	    p++;
	}
	start = p;
	while ((*p != '\0') && (*p != '.') && (*p != '*')) {
	    p++;
	}
	if (p > start) {
	    parts[index].start = start;
	    parts[index].end = p;
	    index++;
	}
    }
    return parts;
}

static int
BrowserXrmBindingPartCount(
    const char *binding)
{
    int count = 0;
    const char *p;

    if (binding == NULL) {
	return 0;
    }
    for (p = binding; *p != '\0'; ) {
	if ((*p == '.') || (*p == '*')) {
	    p++;
	    continue;
	}
	while ((*p != '\0') && (*p != '.') && (*p != '*')) {
	    p++;
	}
	count++;
    }
    return count;
}

static BrowserXrmBindingPart *
BrowserXrmSplitBinding(
    const char *binding,
    int *countPtr,
    int *dotCountPtr,
    int *starCountPtr)
{
    BrowserXrmBindingPart *parts;
    int count;
    int index = 0;
    char separator = '\0';
    const char *p;

    if (dotCountPtr != NULL) {
	*dotCountPtr = 0;
    }
    if (starCountPtr != NULL) {
	*starCountPtr = 0;
    }
    count = BrowserXrmBindingPartCount(binding);
    if (countPtr != NULL) {
	*countPtr = count;
    }
    if (count == 0) {
	return NULL;
    }
    parts = (BrowserXrmBindingPart *)ckalloc(sizeof(*parts) * (size_t)count);
    for (p = binding; *p != '\0'; ) {
	const char *start;

	if ((*p == '.') || (*p == '*')) {
	    separator = *p;
	    if (separator == '.') {
		if (dotCountPtr != NULL) {
		    (*dotCountPtr)++;
		}
	    } else if (starCountPtr != NULL) {
		(*starCountPtr)++;
	    }
	    p++;
	    continue;
	}
	start = p;
	while ((*p != '\0') && (*p != '.') && (*p != '*')) {
	    p++;
	}
	parts[index].start = start;
	parts[index].end = p;
	parts[index].separator = separator;
	index++;
	separator = '\0';
    }
    return parts;
}

static int
BrowserXrmRangeEquals(
    const char *firstStart,
    const char *firstEnd,
    const char *secondStart,
    const char *secondEnd)
{
    size_t firstLength = (size_t)(firstEnd - firstStart);
    size_t secondLength = (size_t)(secondEnd - secondStart);

    return (firstLength == secondLength)
	    && (strncmp(firstStart, secondStart, firstLength) == 0);
}

static int
BrowserXrmPartMatchesComponent(
    const BrowserXrmBindingPart *partPtr,
    const BrowserXrmComponent *nameParts,
    int nameCount,
    const BrowserXrmComponent *classParts,
    int classCount,
    int componentIndex)
{
    if ((componentIndex < 0) || (componentIndex >= nameCount)) {
	return 0;
    }
    if (BrowserXrmRangeEquals(partPtr->start, partPtr->end,
	    nameParts[componentIndex].start, nameParts[componentIndex].end)) {
	return 1;
    }
    return (componentIndex < classCount)
	    && BrowserXrmRangeEquals(partPtr->start, partPtr->end,
		    classParts[componentIndex].start,
		    classParts[componentIndex].end);
}

static int
BrowserXrmBindingMatchesAt(
    const BrowserXrmBindingPart *parts,
    int partCount,
    int partIndex,
    const BrowserXrmComponent *nameParts,
    int nameCount,
    const BrowserXrmComponent *classParts,
    int classCount,
    int componentIndex)
{
    if (partIndex >= partCount) {
	return componentIndex == nameCount;
    }
    if (parts[partIndex].separator == '*') {
	int index;

	for (index = componentIndex; index < nameCount; index++) {
	    if (BrowserXrmPartMatchesComponent(&parts[partIndex], nameParts,
		    nameCount, classParts, classCount, index)
		    && BrowserXrmBindingMatchesAt(parts, partCount,
			    partIndex + 1, nameParts, nameCount, classParts,
			    classCount, index + 1)) {
		return 1;
	    }
	}
	return 0;
    }
    return BrowserXrmPartMatchesComponent(&parts[partIndex], nameParts,
	    nameCount, classParts, classCount, componentIndex)
	    && BrowserXrmBindingMatchesAt(parts, partCount, partIndex + 1,
		    nameParts, nameCount, classParts, classCount,
		    componentIndex + 1);
}

static int
BrowserXrmEntryScore(
    BrowserXrmEntry *entryPtr,
    const BrowserXrmComponent *nameParts,
    int nameCount,
    const BrowserXrmComponent *classParts,
    int classCount)
{
    BrowserXrmBindingPart *parts;
    int partCount = 0;
    int dotCount = 0;
    int starCount = 0;
    int score = 0;

    parts = BrowserXrmSplitBinding(entryPtr->binding, &partCount, &dotCount,
	    &starCount);
    if ((parts != NULL) && (nameCount > 0)
	    && BrowserXrmBindingMatchesAt(parts, partCount, 0, nameParts,
		    nameCount, classParts, classCount, 0)) {
	score = (partCount * 20) + (dotCount * 5) - starCount;
    }
    if (parts != NULL) {
	ckfree(parts);
    }
    return score;
}

static void
BrowserXrmFreeEntryFields(
    BrowserXrmEntry *entryPtr)
{
    if (entryPtr->binding != NULL) {
	ckfree(entryPtr->binding);
    }
    if (entryPtr->value != NULL) {
	ckfree(entryPtr->value);
    }
    if (entryPtr->type != NULL) {
	ckfree(entryPtr->type);
    }
}

static void
BrowserXrmDestroyEntry(
    BrowserXrmEntry *entryPtr)
{
    if (entryPtr == NULL) {
	return;
    }
    BrowserXrmFreeEntryFields(entryPtr);
    ckfree(entryPtr);
}

static BrowserXrmEntry *
BrowserXrmFindExactEntry(
    XrmDatabase database,
    const char *binding)
{
    BrowserXrmEntry *entryPtr;

    if ((database == NULL) || (binding == NULL)) {
	return NULL;
    }
    for (entryPtr = database->firstEntryPtr; entryPtr != NULL;
	    entryPtr = entryPtr->nextPtr) {
	if (strcmp(entryPtr->binding, binding) == 0) {
	    return entryPtr;
	}
    }
    return NULL;
}

static void
BrowserXrmAppendEntryObject(
    XrmDatabase database,
    BrowserXrmEntry *entryPtr)
{
    entryPtr->nextPtr = NULL;
    if (database->lastEntryPtr != NULL) {
	database->lastEntryPtr->nextPtr = entryPtr;
    } else {
	database->firstEntryPtr = entryPtr;
    }
    database->lastEntryPtr = entryPtr;
}

static void
BrowserXrmStoreEntry(
    XrmDatabase database,
    char *binding,
    char *type,
    char *value,
    unsigned int valueSize)
{
    BrowserXrmEntry *entryPtr;

    entryPtr = BrowserXrmFindExactEntry(database, binding);
    if (entryPtr != NULL) {
	ckfree(entryPtr->binding);
	if (entryPtr->type != NULL) {
	    ckfree(entryPtr->type);
	}
	if (entryPtr->value != NULL) {
	    ckfree(entryPtr->value);
	}
	entryPtr->binding = binding;
	entryPtr->type = type;
	entryPtr->value = value;
	entryPtr->valueSize = valueSize;
	return;
    }
    entryPtr = (BrowserXrmEntry *)ckalloc(sizeof(*entryPtr));
    entryPtr->binding = binding;
    entryPtr->value = value;
    entryPtr->type = type;
    entryPtr->valueSize = valueSize;
    BrowserXrmAppendEntryObject(database, entryPtr);
}

static void
BrowserXrmStoreStringEntry(
    XrmDatabase database,
    char *binding,
    char *value)
{
    BrowserXrmStoreEntry(database, binding, BrowserDupString("String"), value,
	    (unsigned int)strlen(value) + 1U);
}

static int
BrowserXrmParseLineIntoDatabase(
    XrmDatabase database,
    const char *lineStart,
    const char *lineEnd)
{
    const char *colon;
    const char *bindingStart;
    const char *bindingEnd;
    const char *valueStart;
    const char *valueEnd;

    bindingStart = BrowserSkipDefaultSpaces(lineStart, lineEnd);
    if ((bindingStart == lineEnd) || (*bindingStart == '!')) {
	return 0;
    }
    colon = memchr(bindingStart, ':', (size_t)(lineEnd - bindingStart));
    if (colon == NULL) {
	return 0;
    }
    bindingEnd = BrowserTrimDefaultSpaces(bindingStart, colon);
    if (bindingEnd == bindingStart) {
	return 0;
    }
    valueStart = BrowserSkipDefaultSpaces(colon + 1, lineEnd);
    valueEnd = BrowserTrimDefaultSpaces(valueStart, lineEnd);
    BrowserXrmStoreStringEntry(database,
	    BrowserDupRange(bindingStart, bindingEnd),
	    BrowserDupRange(valueStart, valueEnd));
    return 1;
}

char *
XGetDefault(
    Display *display,
    _Xconst char *program,
    _Xconst char *option)
{
    const char *resources = XResourceManagerString(display);
    const char *programName = BrowserDefaultProgramName(program);
    const char *lineStart;
    const char *bestStart = NULL;
    const char *bestEnd = NULL;
    int bestScore = 0;

    if ((resources == NULL) || (option == NULL) || (option[0] == '\0')) {
	return NULL;
    }
    for (lineStart = resources; *lineStart != '\0'; ) {
	const char *lineEnd = lineStart;
	const char *colon;
	const char *bindingStart;
	const char *bindingEnd;
	const char *valueStart;
	const char *valueEnd;
	int score;

	while ((*lineEnd != '\0') && (*lineEnd != '\n')) {
	    lineEnd++;
	}
	bindingStart = BrowserSkipDefaultSpaces(lineStart, lineEnd);
	if ((bindingStart == lineEnd) || (*bindingStart == '!')) {
	    lineStart = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
	    continue;
	}
	colon = memchr(bindingStart, ':', (size_t)(lineEnd - bindingStart));
	if (colon == NULL) {
	    lineStart = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
	    continue;
	}
	bindingEnd = BrowserTrimDefaultSpaces(bindingStart, colon);
	valueStart = BrowserSkipDefaultSpaces(colon + 1, lineEnd);
	valueEnd = BrowserTrimDefaultSpaces(valueStart, lineEnd);
	score = BrowserResourceBindingScore(bindingStart, bindingEnd,
		programName, option);
	if (score >= bestScore) {
	    bestScore = score;
	    bestStart = valueStart;
	    bestEnd = valueEnd;
	}
	lineStart = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
    }
    return (bestScore > 0) ? BrowserRememberDefaultValue(bestStart, bestEnd)
	    : NULL;
}

char *
XResourceManagerString(
    Display *display)
{
    (void)display;
    return (char *)BrowserResourceManagerString();
}

char *
XScreenResourceString(
    Screen *screen)
{
    Display *display;
    X11WasmResource *rootPtr;
    X11WasmProperty *propertyPtr;
    Atom screenResourcesAtom;
    char *copy;

    if (screen == NULL) {
	return NULL;
    }
    BrowserInitDisplay();
    display = XDisplayOfScreen(screen);
    screenResourcesAtom = XInternAtom(display, "SCREEN_RESOURCES", False);
    rootPtr = X11WasmFindResource((XID)XRootWindowOfScreen(screen));
    propertyPtr = BrowserFindProperty(rootPtr, screenResourcesAtom);
    if ((propertyPtr == NULL) || (propertyPtr->type != XA_STRING)
	    || (propertyPtr->format != 8) || (propertyPtr->data == NULL)) {
	return NULL;
    }
    copy = (char *)ckalloc((size_t)propertyPtr->dataLength + 1U);
    if (propertyPtr->dataLength > 0) {
	memcpy(copy, propertyPtr->data, (size_t)propertyPtr->dataLength);
    }
    copy[propertyPtr->dataLength] = '\0';
    return copy;
}

const char *
XDefaultString(void)
{
    return "";
}

static void
BrowserNotifyColormapInstallState(
    Colormap colormap,
    int state)
{
    X11WasmResource *resourcePtr;

    for (resourcePtr = browserDisplay.resourceList; resourcePtr != NULL;
	    resourcePtr = resourcePtr->nextPtr) {
	if ((resourcePtr->type == X11_WASM_RESOURCE_WINDOW)
		&& (resourcePtr->atts.colormap == colormap)) {
	    (void)BrowserQueueColormapNotify(resourcePtr, colormap, False,
		    state);
	}
    }
}

void
XrmInitialize(void)
{
}

XrmDatabase
XrmGetDatabase(
    Display *display)
{
    if (display == NULL) {
	return NULL;
    }
    return (XrmDatabase)display->db;
}

void
XrmSetDatabase(
    Display *display,
    XrmDatabase database)
{
    if (display != NULL) {
	display->db = (struct _XrmHashBucketRec *)database;
    }
}

void
XrmPutResource(
    XrmDatabase *databasePtr,
    _Xconst char *specifier,
    _Xconst char *type,
    XrmValue *value)
{
    char *valueCopy;
    unsigned int valueSize = 0U;

    if ((databasePtr == NULL) || (specifier == NULL)
	    || (specifier[0] == '\0')) {
	return;
    }
    if (*databasePtr == NULL) {
	*databasePtr = BrowserXrmNewDatabase();
    }
    valueCopy = BrowserDupValue(value, &valueSize);
    BrowserXrmStoreEntry(*databasePtr, BrowserDupString(specifier),
	    (type != NULL) ? BrowserDupString(type) : NULL,
	    valueCopy, valueSize);
}

void
XrmPutStringResource(
    XrmDatabase *databasePtr,
    _Xconst char *specifier,
    _Xconst char *value)
{
    XrmValue xrmValue;

    if (value == NULL) {
	value = "";
    }
    xrmValue.addr = (XPointer)value;
    xrmValue.size = (unsigned int)strlen(value) + 1U;
    XrmPutResource(databasePtr, specifier, "String", &xrmValue);
}

void
XrmPutLineResource(
    XrmDatabase *databasePtr,
    _Xconst char *line)
{
    const char *lineEnd;

    if ((databasePtr == NULL) || (line == NULL)) {
	return;
    }
    if (*databasePtr == NULL) {
	*databasePtr = BrowserXrmNewDatabase();
    }
    lineEnd = line;
    while ((*lineEnd != '\0') && (*lineEnd != '\n')) {
	lineEnd++;
    }
    (void)BrowserXrmParseLineIntoDatabase(*databasePtr, line, lineEnd);
}

XrmDatabase
XrmGetStringDatabase(
    _Xconst char *data)
{
    XrmDatabase database;
    const char *lineStart;

    if (data == NULL) {
	return NULL;
    }
    database = BrowserXrmNewDatabase();
    for (lineStart = data; *lineStart != '\0'; ) {
	const char *lineEnd = lineStart;

	while ((*lineEnd != '\0') && (*lineEnd != '\n')) {
	    lineEnd++;
	}
	(void)BrowserXrmParseLineIntoDatabase(database, lineStart, lineEnd);
	lineStart = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
    }
    return database;
}

Bool
XrmGetResource(
    XrmDatabase database,
    _Xconst char *resourceName,
    _Xconst char *resourceClass,
    char **resourceTypeReturn,
    XrmValue *valueReturn)
{
    BrowserXrmEntry *entryPtr;
    BrowserXrmEntry *bestEntryPtr = NULL;
    BrowserXrmComponent *nameParts;
    BrowserXrmComponent *classParts;
    int nameCount = 0;
    int classCount = 0;
    int bestScore = 0;

    if ((database == NULL) || (resourceName == NULL)) {
	return False;
    }
    nameParts = BrowserXrmSplitComponents(resourceName, &nameCount);
    classParts = BrowserXrmSplitComponents(resourceClass, &classCount);
    if (nameCount == 0) {
	goto cleanup;
    }
    for (entryPtr = database->firstEntryPtr; entryPtr != NULL;
	    entryPtr = entryPtr->nextPtr) {
	int score = BrowserXrmEntryScore(entryPtr, nameParts, nameCount,
		classParts, classCount);

	if (score >= bestScore) {
	    bestScore = score;
	    bestEntryPtr = (score > 0) ? entryPtr : bestEntryPtr;
	}
    }
    if (bestEntryPtr != NULL) {
	if (resourceTypeReturn != NULL) {
	    *resourceTypeReturn = bestEntryPtr->type;
	}
	if (valueReturn != NULL) {
	    valueReturn->size = bestEntryPtr->valueSize;
	    valueReturn->addr = bestEntryPtr->value;
	}
    }

cleanup:
    if (nameParts != NULL) {
	ckfree(nameParts);
    }
    if (classParts != NULL) {
	ckfree(classParts);
    }
    return (bestEntryPtr != NULL) ? True : False;
}

void
XrmDestroyDatabase(
    XrmDatabase database)
{
    BrowserXrmEntry *entryPtr;

    if (database == NULL) {
	return;
    }
    for (entryPtr = database->firstEntryPtr; entryPtr != NULL; ) {
	BrowserXrmEntry *nextPtr = entryPtr->nextPtr;

	BrowserXrmDestroyEntry(entryPtr);
	entryPtr = nextPtr;
    }
    ckfree(database);
}

void
XrmCombineDatabase(
    XrmDatabase sourceDatabase,
    XrmDatabase *targetDatabasePtr,
    Bool override)
{
    BrowserXrmEntry *entryPtr;

    if ((targetDatabasePtr == NULL) || (sourceDatabase == NULL)) {
	return;
    }
    if (*targetDatabasePtr == sourceDatabase) {
	return;
    }
    if (*targetDatabasePtr == NULL) {
	*targetDatabasePtr = sourceDatabase;
	return;
    }
    for (entryPtr = sourceDatabase->firstEntryPtr; entryPtr != NULL; ) {
	BrowserXrmEntry *nextPtr = entryPtr->nextPtr;
	BrowserXrmEntry *targetEntryPtr;

	entryPtr->nextPtr = NULL;
	targetEntryPtr = BrowserXrmFindExactEntry(*targetDatabasePtr,
		entryPtr->binding);
	if (targetEntryPtr == NULL) {
	    BrowserXrmAppendEntryObject(*targetDatabasePtr, entryPtr);
	} else if (override) {
	    BrowserXrmStoreEntry(*targetDatabasePtr, entryPtr->binding,
		    entryPtr->type, entryPtr->value, entryPtr->valueSize);
	    ckfree(entryPtr);
	} else {
	    BrowserXrmDestroyEntry(entryPtr);
	}
	entryPtr = nextPtr;
    }
    ckfree(sourceDatabase);
}

void
XrmMergeDatabases(
    XrmDatabase sourceDatabase,
    XrmDatabase *targetDatabasePtr)
{
    XrmCombineDatabase(sourceDatabase, targetDatabasePtr, True);
}

EMSCRIPTEN_KEEPALIVE int
X11WasmTestOpenDisplayNames(void)
{
    if ((XOpenDisplay(NULL) != &browserDisplay.display)
	    || (XOpenDisplay("") != &browserDisplay.display)
	    || (XOpenDisplay("browser") != &browserDisplay.display)
	    || (XOpenDisplay(":0") != &browserDisplay.display)
	    || (XOpenDisplay(":0.0") != &browserDisplay.display)
	    || (XOpenDisplay("browser:0") != &browserDisplay.display)
	    || (XOpenDisplay("browser:0.0") != &browserDisplay.display)) {
	return 0;
    }
    if ((XOpenDisplay("definitely-invalid") != NULL)
	    || (XOpenDisplay(":1") != NULL)
	    || (XOpenDisplay("browser:0.1") != NULL)) {
	return 0;
    }
    return 1;
}

int
XCloseDisplay(
    Display *display)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    BrowserForgetIMInstantiateCallbacks(display);
    focusedInputContext = NULL;
    if (display == &browserDisplay.display) {
	BrowserResetHostAccessControl();
	BrowserClearFontPath();
	BrowserClearPassiveGrabs();
	browserDisplay.pointerGrabWindow = None;
	browserDisplay.pointerGrabConfineWindow = None;
	BrowserSetPointerGrabCursor(None);
	browserDisplay.pointerGrabEventMask = 0;
	browserDisplay.pointerGrabOwnerEvents = 0;
	browserDisplay.pointerGrabPointerMode = GrabModeAsync;
	browserDisplay.pointerGrabKeyboardMode = GrabModeAsync;
	browserDisplay.pointerGrabPassive = 0;
	browserDisplay.motionHistoryStart = 0;
	browserDisplay.motionHistoryCount = 0;
	browserDisplay.keyboardGrabWindow = None;
	browserDisplay.keyboardGrabOwnerEvents = 0;
	browserDisplay.keyboardGrabPointerMode = GrabModeAsync;
	browserDisplay.keyboardGrabKeyboardMode = GrabModeAsync;
	browserDisplay.keyboardGrabPassive = 0;
	browserDisplay.serverGrabDepth = 0;
	BrowserSyncServerGrabState();
	browserDisplay.synchronize = 0;
	browserDisplay.threadsInitialized = 0;
	browserDisplay.displayLockDepth = 0;
	browserDisplay.afterFunction = NULL;
	browserDisplay.runningAfterFunction = 0;
	browserDisplay.display.db = NULL;
	BrowserSyncSynchronizeState(display);
	BrowserApplyCloseDownMode();
	browserDisplay.closeDownMode = DestroyAll;
	browserDisplay.closeSerial++;
	BrowserSyncPointerGrabCursor();
	BrowserSyncPointerGrabState();
	BrowserSyncKeyboardGrabState();
    }
    return XSync(display, False);
}

Window
XCreateWindow(
    Display *display,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int borderWidth,
    int depth,
    unsigned int clazz,
    Visual *visual,
    unsigned long valueMask,
    XSetWindowAttributes *attributes)
{
    X11WasmResource *parentPtr;
    X11WasmResource *resourcePtr;
    unsigned int windowDepth;
    unsigned int windowClass;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    parentPtr = BrowserWindowResourceOrError(&display, parent,
	    X11_WASM_XREQ_CREATE_WINDOW);
    if (parentPtr == NULL) {
	return None;
    }
    if (!BrowserValidateCard16(&display, (long)width, 0,
	    X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    if (!BrowserValidateCard16(&display, (long)height, 0,
	    X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    if (!BrowserValidateCard16(&display, (long)borderWidth, 1,
	    X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    if (!BrowserWindowClassOrError(&display, clazz, parentPtr->c_class,
	    X11_WASM_XREQ_CREATE_WINDOW, &windowClass)) {
	return None;
    }
    if ((windowClass == InputOnly)
	    && !BrowserValidateInputOnlyCreateWindow(&display, parent, depth,
		    visual, borderWidth, valueMask,
		    X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    if ((windowClass == InputOutput)
	    && !BrowserValidateInputOutputCreateWindow(&display, parentPtr,
		    depth, visual, X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    windowDepth = (unsigned int)((depth == CopyFromParent)
	    ? ((windowClass == InputOnly) ? 0U : parentPtr->depth)
	    : depth);
    if (!BrowserValidateWindowAttributeValues(&display, windowDepth,
	    valueMask, attributes, X11_WASM_XREQ_CREATE_WINDOW)) {
	return None;
    }
    resourcePtr = X11WasmCreateResource(display, X11_WASM_RESOURCE_WINDOW);
    resourcePtr->parent = parent;
    resourcePtr->x = x;
    resourcePtr->y = y;
    resourcePtr->width = width;
    resourcePtr->height = height;
    resourcePtr->borderWidth = borderWidth;
    resourcePtr->depth = windowDepth;
    resourcePtr->c_class = windowClass;
    BrowserApplyWindowAttributes(resourcePtr, valueMask, attributes);
    BrowserLinkChildAtTop(resourcePtr, parent);
    (void)X11WasmJsCreateWindow(BrowserJsXId(resourcePtr->id),
	    BrowserJsXId(parent), x, y, width, height, resourcePtr->depth,
	    resourcePtr->atts.event_mask, resourcePtr->c_class);
    BrowserSyncWindowEventMask(resourcePtr);
    BrowserSyncWindowColormap(resourcePtr);
    BrowserSyncWindowAttributes(resourcePtr);
    BrowserSyncWindowStyle(resourcePtr);
    (void)BrowserQueueCreateNotify(resourcePtr);
    return (Window)resourcePtr->id;
}

Window
XCreateSimpleWindow(
    Display *display,
    Window parent,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned int borderWidth,
    unsigned long border,
    unsigned long background)
{
    XSetWindowAttributes attributes;

    memset(&attributes, 0, sizeof(attributes));
    attributes.background_pixel = background;
    attributes.border_pixel = border;
    return XCreateWindow(display, parent, x, y, width, height, borderWidth,
	    CopyFromParent, InputOutput, (Visual *)CopyFromParent,
	    CWBackPixel | CWBorderPixel, &attributes);
}

int
XDestroyWindow(
    Display *display,
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_DESTROY_WINDOW);

    if (resourcePtr == NULL) {
	return Success;
    }
    (void)BrowserDestroyWindowResource(resourcePtr);
    return Success;
}

int
XDestroySubwindows(
    Display *display,
    Window window)
{
    X11WasmResource *parentPtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_DESTROY_SUBWINDOWS);

    if (parentPtr == NULL) {
	return Success;
    }
    while (parentPtr->firstChildPtr != NULL) {
	(void)BrowserDestroyWindowResource(parentPtr->firstChildPtr);
    }
    return Success;
}

int
BrowserDestroyWindowResource(
    X11WasmResource *resourcePtr)
{
    Window window;

    if ((resourcePtr == NULL)
	    || (resourcePtr->type != X11_WASM_RESOURCE_WINDOW)
	    || (resourcePtr == &browserDisplay.rootResource)) {
	return Success;
    }
    BrowserRevertInputFocusForUnviewableWindow(resourcePtr);
    while (resourcePtr->firstChildPtr != NULL) {
	(void)BrowserDestroyWindowResource(resourcePtr->firstChildPtr);
    }
    window = (Window)resourcePtr->id;
    (void)BrowserQueueDestroyNotify(resourcePtr);
    (void)X11WasmJsDestroyResource(BrowserJsXId(window));
    X11WasmDeleteResource(window);
    return Success;
}

int
XMapWindow(
    Display *display,
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_MAP_WINDOW);

    if (resourcePtr == NULL) {
	return Success;
    }
    if (resourcePtr->mapped) {
	return Success;
    }
    resourcePtr->mapped = 1;
    (void)X11WasmJsMapWindow(BrowserJsXId(window));
    if (BrowserWindowMetadataTarget(window) == window) {
	BrowserSetWindowWmState(window,
		BrowserInitialWmStateForMap(display, window));
    }
    (void)BrowserQueueMapNotify(resourcePtr);
    BrowserNotifyVisibilityTree(resourcePtr, VisibilityUnobscured);
    if (resourcePtr->c_class != InputOnly) {
	(void)BrowserQueueExpose(resourcePtr, 0, 0, resourcePtr->width,
		resourcePtr->height);
    }
    (void)BrowserQueuePointerCrossingEvents(browserDisplay.pointerState,
	    browserDisplay.pointerX, browserDisplay.pointerY, CurrentTime);
    return Success;
}

int
XMapRaised(
    Display *display,
    Window window)
{
    int result = XMapWindow(display, window);

    if (result == Success) {
	(void)XRaiseWindow(display, window);
    }
    return result;
}

int
XMapSubwindows(
    Display *display,
    Window window)
{
    X11WasmResource *parentPtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_MAP_SUBWINDOWS);
    X11WasmResource *childPtr;

    if (parentPtr == NULL) {
	return Success;
    }
    for (childPtr = parentPtr->lastChildPtr; childPtr != NULL; ) {
	X11WasmResource *prevPtr = childPtr->prevSiblingPtr;

	if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
	    (void)XMapWindow(display, (Window)childPtr->id);
	}
	childPtr = prevPtr;
    }
    return Success;
}

int
BrowserUnmapWindowResource(
    X11WasmResource *resourcePtr,
    Bool fromConfigure)
{
    if (resourcePtr == NULL) {
	return Success;
    }
    if (!resourcePtr->mapped) {
	return Success;
    }
    BrowserRevertInputFocusForUnviewableWindow(resourcePtr);
    resourcePtr->mapped = 0;
    (void)X11WasmJsUnmapWindow(BrowserJsXId(resourcePtr->id));
    (void)BrowserQueueUnmapNotify(resourcePtr, fromConfigure);
    (void)BrowserQueuePointerCrossingEvents(browserDisplay.pointerState,
	    browserDisplay.pointerX, browserDisplay.pointerY, CurrentTime);
    return Success;
}

int
XUnmapWindow(
    Display *display,
    Window window)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_UNMAP_WINDOW);

    return BrowserUnmapWindowResource(resourcePtr, False);
}

int
XUnmapSubwindows(
    Display *display,
    Window window)
{
    X11WasmResource *parentPtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_UNMAP_SUBWINDOWS);
    X11WasmResource *childPtr;

    if (parentPtr == NULL) {
	return Success;
    }
    for (childPtr = parentPtr->firstChildPtr; childPtr != NULL; ) {
	X11WasmResource *nextPtr = childPtr->nextSiblingPtr;

	if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
	    (void)BrowserUnmapWindowResource(childPtr, False);
	}
	childPtr = nextPtr;
    }
    return Success;
}

int
XMoveWindow(
    Display *display,
    Window window,
    int x,
    int y)
{
    XWindowChanges changes;
    changes.x = x;
    changes.y = y;
    return XConfigureWindow(display, window, CWX | CWY, &changes);
}

int
XResizeWindow(
    Display *display,
    Window window,
    unsigned int width,
    unsigned int height)
{
    XWindowChanges changes;
    changes.width = (int)width;
    changes.height = (int)height;
    return XConfigureWindow(display, window, CWWidth | CWHeight, &changes);
}

int
XMoveResizeWindow(
    Display *display,
    Window window,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    XWindowChanges changes;
    changes.x = x;
    changes.y = y;
    changes.width = (int)width;
    changes.height = (int)height;
    return XConfigureWindow(display, window, CWX | CWY | CWWidth | CWHeight,
	    &changes);
}

int
XConfigureWindow(
    Display *display,
    Window window,
    unsigned int valueMask,
    XWindowChanges *changes)
{
    X11WasmResource *resourcePtr = BrowserWindowResourceOrError(&display,
	    window, X11_WASM_XREQ_CONFIGURE_WINDOW);
    int sizeChanged;
    unsigned int oldWidth;
    unsigned int oldHeight;
    Window above = None;

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserValidateConfigureWindowValues(&display, resourcePtr,
	    valueMask, changes, X11_WASM_XREQ_CONFIGURE_WINDOW)) {
	return Success;
    }
    if (resourcePtr == &browserDisplay.rootResource) {
	return Success;
    }
    if (changes == NULL) {
	return Success;
    }
    oldWidth = resourcePtr->width;
    oldHeight = resourcePtr->height;
    sizeChanged = ((valueMask & CWWidth)
	    && (resourcePtr->width != (unsigned int)changes->width))
	    || ((valueMask & CWHeight)
	    && (resourcePtr->height != (unsigned int)changes->height));
    if (valueMask & CWX) resourcePtr->x = changes->x;
    if (valueMask & CWY) resourcePtr->y = changes->y;
    if (valueMask & CWWidth) resourcePtr->width = (unsigned int)changes->width;
    if (valueMask & CWHeight) resourcePtr->height = (unsigned int)changes->height;
    if (valueMask & CWBorderWidth) {
	resourcePtr->borderWidth = (unsigned int)changes->border_width;
    }
    if (valueMask & (CWSibling | CWStackMode)) {
	BrowserRestackWindow(resourcePtr, valueMask, changes);
	if (resourcePtr->prevSiblingPtr != NULL) {
	    above = (Window)resourcePtr->prevSiblingPtr->id;
	}
    }
    (void)X11WasmJsConfigureWindow(BrowserJsXId(window), resourcePtr->x,
	    resourcePtr->y, resourcePtr->width, resourcePtr->height,
	    resourcePtr->borderWidth, valueMask);
    (void)BrowserQueueConfigureNotify(resourcePtr, above);
    if (sizeChanged) {
	BrowserApplyChildWinGravity(resourcePtr, oldWidth, oldHeight);
    }
    if (sizeChanged && (resourcePtr->c_class != InputOnly)) {
	(void)BrowserQueueExpose(resourcePtr, 0, 0, resourcePtr->width,
		resourcePtr->height);
    }
    (void)BrowserQueuePointerCrossingEvents(browserDisplay.pointerState,
	    browserDisplay.pointerX, browserDisplay.pointerY, CurrentTime);
    return Success;
}

int
XRestackWindows(
    Display *display,
    Window *windows,
    int nwindows)
{
    XWindowChanges changes;
    int i;

    if ((nwindows <= 0) || (windows == NULL)) {
	return Success;
    }
    if (BrowserWindowResourceOrError(&display, windows[0],
	    X11_WASM_XREQ_CONFIGURE_WINDOW) == NULL) {
	return Success;
    }
    memset(&changes, 0, sizeof(changes));
    changes.stack_mode = Below;
    for (i = 1; i < nwindows; i++) {
	changes.sibling = windows[i - 1];
	(void)XConfigureWindow(display, windows[i], CWSibling | CWStackMode,
		&changes);
    }
    return Success;
}

int
BrowserScreenOrError(
    Display *display,
    int screen,
    int requestCode)
{
    if ((screen < 0) || (screen >= ScreenCount(display))) {
	(void)BrowserReportXError(display, BadValue, (XID)screen,
		requestCode, 0);
	return 0;
    }
    return 1;
}

Status
XReconfigureWMWindow(
    Display *display,
    Window window,
    int screen,
    unsigned int valueMask,
    XWindowChanges *changes)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    } else {
	BrowserInitDisplay();
    }
    if (!BrowserScreenOrError(display, screen,
	    X11_WASM_XREQ_CONFIGURE_WINDOW)) {
	return False;
    }
    return (XConfigureWindow(display, window, valueMask, changes) == Success)
	    ? True : False;
}

static int
BrowserWindowColormapInstalled(
    X11WasmResource *resourcePtr)
{
    Colormap colormap;
    X11WasmResource *colormapPtr;

    if ((resourcePtr == NULL) || (resourcePtr->c_class == InputOnly)) {
	return True;
    }
    colormap = (resourcePtr->atts.colormap != None)
	    ? resourcePtr->atts.colormap : browserDisplay.defaultColormap;
    colormapPtr = BrowserColormapResource(colormap);
    return (colormapPtr == NULL) || colormapPtr->colormapInstalled;
}

int
XGetWindowAttributes(
    Display *display,
    Window window,
    XWindowAttributes *attributes)
{
    X11WasmResource *resourcePtr;

    if (attributes == NULL) {
	return False;
    }
    memset(attributes, 0, sizeof(XWindowAttributes));
    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_GET_WINDOW_ATTRIBUTES);
    if (resourcePtr == NULL) {
	return False;
    }
    attributes->x = resourcePtr->x;
    attributes->y = resourcePtr->y;
    attributes->width = (int)resourcePtr->width;
    attributes->height = (int)resourcePtr->height;
    attributes->border_width = (int)resourcePtr->borderWidth;
    attributes->depth = (int)resourcePtr->depth;
    attributes->visual = (resourcePtr->c_class == InputOnly)
	    ? NULL : DefaultVisual(display, DefaultScreen(display));
    attributes->root = RootWindow(display, DefaultScreen(display));
    attributes->c_class = resourcePtr->c_class;
    attributes->bit_gravity = resourcePtr->atts.bit_gravity;
    attributes->win_gravity = resourcePtr->atts.win_gravity;
    attributes->backing_store = resourcePtr->atts.backing_store;
    attributes->backing_planes = resourcePtr->atts.backing_planes;
    attributes->backing_pixel = resourcePtr->atts.backing_pixel;
    attributes->save_under = resourcePtr->atts.save_under;
    attributes->colormap = (resourcePtr->c_class == InputOnly) ? None
	    : ((resourcePtr->atts.colormap != None)
	    ? resourcePtr->atts.colormap
	    : DefaultColormap(display, DefaultScreen(display)));
    attributes->map_installed = BrowserWindowColormapInstalled(resourcePtr);
    attributes->map_state = BrowserResourceIsVisible(resourcePtr) ? IsViewable
	    : (resourcePtr->mapped ? IsUnviewable : IsUnmapped);
    attributes->all_event_masks = resourcePtr->atts.event_mask;
    attributes->your_event_mask = resourcePtr->atts.event_mask;
    attributes->do_not_propagate_mask =
	    resourcePtr->atts.do_not_propagate_mask;
    attributes->override_redirect = resourcePtr->atts.override_redirect;
    attributes->screen = DefaultScreenOfDisplay(display);
    return True;
}

Status
XGetGeometry(
    Display *display,
    Drawable drawable,
    Window *rootReturn,
    int *xReturn,
    int *yReturn,
    unsigned int *widthReturn,
    unsigned int *heightReturn,
    unsigned int *borderWidthReturn,
    unsigned int *depthReturn)
{
    X11WasmResource *resourcePtr;

    if (rootReturn != NULL) {
	*rootReturn = None;
    }
    if (xReturn != NULL) *xReturn = 0;
    if (yReturn != NULL) *yReturn = 0;
    if (widthReturn != NULL) *widthReturn = 0;
    if (heightReturn != NULL) *heightReturn = 0;
    if (borderWidthReturn != NULL) *borderWidthReturn = 0;
    if (depthReturn != NULL) *depthReturn = 0;

    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_GET_GEOMETRY);
    if (resourcePtr == NULL) {
	return False;
    }
    if (rootReturn != NULL) *rootReturn = RootWindow(display, DefaultScreen(display));
    if (xReturn != NULL) *xReturn = resourcePtr->x;
    if (yReturn != NULL) *yReturn = resourcePtr->y;
    if (widthReturn != NULL) *widthReturn = resourcePtr->width;
    if (heightReturn != NULL) *heightReturn = resourcePtr->height;
    if (borderWidthReturn != NULL) *borderWidthReturn = resourcePtr->borderWidth;
    if (depthReturn != NULL) *depthReturn = resourcePtr->depth;
    return True;
}

Bool
XTranslateCoordinates(
    Display *display,
    Window source,
    Window destination,
    int sourceX,
    int sourceY,
    int *destXReturn,
    int *destYReturn,
    Window *childReturn)
{
    X11WasmResource *resourcePtr;
    X11WasmResource *childPtr = NULL;
    int destX = sourceX;
    int destY = sourceY;

    if (destXReturn != NULL) *destXReturn = 0;
    if (destYReturn != NULL) *destYReturn = 0;
    if (childReturn != NULL) *childReturn = None;

    if (BrowserWindowResourceOrError(&display, source,
	    X11_WASM_XREQ_TRANSLATE_COORDINATES) == NULL) {
	return False;
    }
    resourcePtr = BrowserWindowResourceOrError(&display, destination,
	    X11_WASM_XREQ_TRANSLATE_COORDINATES);
    if (resourcePtr == NULL) {
	return False;
    }
    BrowserTranslateWindowCoordinates(source, destination, sourceX, sourceY,
	    &destX, &destY);
    childPtr = BrowserFindTopVisibleChildAt(resourcePtr, destX, destY);
    if (destXReturn != NULL) *destXReturn = destX;
    if (destYReturn != NULL) *destYReturn = destY;
    if (childReturn != NULL) {
	*childReturn = (childPtr != NULL) ? (Window)childPtr->id : None;
    }
    return True;
}

Status
XQueryTree(
    Display *display,
    Window window,
    Window *rootReturn,
    Window *parentReturn,
    Window **childrenReturn,
    unsigned int *nchildrenReturn)
{
    X11WasmResource *resourcePtr;
    X11WasmResource *childPtr;
    Window *children = NULL;
    unsigned int count = 0;
    unsigned int index = 0;

    resourcePtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_QUERY_TREE);
    if (resourcePtr == NULL) {
	if (childrenReturn != NULL) *childrenReturn = NULL;
	if (nchildrenReturn != NULL) *nchildrenReturn = 0;
	return False;
    }
    for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
	    childPtr = childPtr->nextSiblingPtr) {
	if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
	    count++;
	}
    }
    if (count > 0) {
	children = (Window *)ckalloc(sizeof(Window) * count);
	for (childPtr = resourcePtr->firstChildPtr; childPtr != NULL;
		childPtr = childPtr->nextSiblingPtr) {
	    if (childPtr->type == X11_WASM_RESOURCE_WINDOW) {
		children[index++] = (Window)childPtr->id;
	    }
	}
    }
    if (rootReturn != NULL) *rootReturn = RootWindow(display, DefaultScreen(display));
    if (parentReturn != NULL) {
	*parentReturn = (window == RootWindow(display, DefaultScreen(display)))
		? None : resourcePtr->parent;
    }
    if (childrenReturn != NULL) {
	*childrenReturn = children;
    } else if (children != NULL) {
	ckfree(children);
    }
    if (nchildrenReturn != NULL) *nchildrenReturn = count;
    return True;
}

static Display *
BrowserAccessorDisplay(
    Display *display)
{
    return (display != NULL) ? display : X11WasmGetDisplay(NULL);
}

static Screen *
BrowserAccessorScreen(
    Display *display,
    int screen)
{
    display = BrowserAccessorDisplay(display);
    if ((display == NULL) || (screen < 0) || (screen >= ScreenCount(display))) {
	return NULL;
    }
    return ScreenOfDisplay(display, screen);
}

static Screen *
BrowserAccessorDefaultScreen(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    if (display == NULL) {
	return NULL;
    }
    return BrowserAccessorScreen(display, DefaultScreen(display));
}

Window
XRootWindow(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? RootWindowOfScreen(screenPtr) : None;
}

int
XConnectionNumber(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? ConnectionNumber(display) : -1;
}

int
XDefaultScreen(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? DefaultScreen(display) : 0;
}

Window
XDefaultRootWindow(
    Display *display)
{
    Screen *screenPtr = BrowserAccessorDefaultScreen(display);

    return (screenPtr != NULL) ? RootWindowOfScreen(screenPtr) : None;
}

Visual *
XDefaultVisual(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? DefaultVisualOfScreen(screenPtr) : NULL;
}

GC
XDefaultGC(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? DefaultGCOfScreen(screenPtr) : NULL;
}

unsigned long
XBlackPixel(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? BlackPixelOfScreen(screenPtr) : 0UL;
}

unsigned long
XWhitePixel(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? WhitePixelOfScreen(screenPtr) : 0UL;
}

int
XQLength(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? QLength(display) : 0;
}

int
XDisplayWidth(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? WidthOfScreen(screenPtr) : 0;
}

int
XDisplayHeight(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? HeightOfScreen(screenPtr) : 0;
}

int
XDisplayWidthMM(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? WidthMMOfScreen(screenPtr) : 0;
}

int
XDisplayHeightMM(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? HeightMMOfScreen(screenPtr) : 0;
}

int
XDisplayPlanes(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? PlanesOfScreen(screenPtr) : 0;
}

int
XDisplayCells(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return ((screenPtr != NULL) && (DefaultVisualOfScreen(screenPtr) != NULL))
	    ? CellsOfScreen(screenPtr) : 0;
}

int
XScreenCount(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? ScreenCount(display) : 0;
}

char *
XServerVendor(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? ServerVendor(display) : NULL;
}

int
XProtocolVersion(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? ProtocolVersion(display) : 0;
}

int
XProtocolRevision(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? ProtocolRevision(display) : 0;
}

int
XVendorRelease(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? VendorRelease(display) : 0;
}

char *
XDisplayString(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? DisplayString(display) : NULL;
}

int
XDefaultDepth(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? DefaultDepthOfScreen(screenPtr) : 0;
}

Colormap
XDefaultColormap(
    Display *display,
    int screen)
{
    Screen *screenPtr = BrowserAccessorScreen(display, screen);

    return (screenPtr != NULL) ? DefaultColormapOfScreen(screenPtr) : None;
}

int
XBitmapUnit(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? BitmapUnit(display) : 0;
}

int
XBitmapBitOrder(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? BitmapBitOrder(display) : 0;
}

int
XBitmapPad(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? BitmapPad(display) : 0;
}

int
XImageByteOrder(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? ImageByteOrder(display) : 0;
}

long
XMaxRequestSize(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? (long)display->max_request_size : 0L;
}

long
XExtendedMaxRequestSize(
    Display *display)
{
    (void)display;
    return 0L;
}

unsigned long
XDisplayMotionBufferSize(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? display->motion_buffer : 0UL;
}

unsigned long
XAllPlanes(void)
{
    return AllPlanes;
}

unsigned long
XNextRequest(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? NextRequest(display) : 0UL;
}

unsigned long
XLastKnownRequestProcessed(
    Display *display)
{
    display = BrowserAccessorDisplay(display);
    return (display != NULL) ? LastKnownRequestProcessed(display) : 0UL;
}

Screen *
XScreenOfDisplay(
    Display *display,
    int screen)
{
    return BrowserAccessorScreen(display, screen);
}

Screen *
XDefaultScreenOfDisplay(
    Display *display)
{
    return BrowserAccessorDefaultScreen(display);
}

Display *
XDisplayOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? DisplayOfScreen(screen) : NULL;
}

Window
XRootWindowOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? RootWindowOfScreen(screen) : None;
}

unsigned long
XBlackPixelOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? BlackPixelOfScreen(screen) : 0UL;
}

unsigned long
XWhitePixelOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? WhitePixelOfScreen(screen) : 0UL;
}

Colormap
XDefaultColormapOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? DefaultColormapOfScreen(screen) : None;
}

int
XDefaultDepthOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? DefaultDepthOfScreen(screen) : 0;
}

GC
XDefaultGCOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? DefaultGCOfScreen(screen) : NULL;
}

Visual *
XDefaultVisualOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? DefaultVisualOfScreen(screen) : NULL;
}

int
XWidthOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? WidthOfScreen(screen) : 0;
}

int
XHeightOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? HeightOfScreen(screen) : 0;
}

int
XWidthMMOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? WidthMMOfScreen(screen) : 0;
}

int
XHeightMMOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? HeightMMOfScreen(screen) : 0;
}

int
XPlanesOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? PlanesOfScreen(screen) : 0;
}

int
XCellsOfScreen(
    Screen *screen)
{
    return ((screen != NULL) && (DefaultVisualOfScreen(screen) != NULL))
	    ? CellsOfScreen(screen) : 0;
}

int
XMinCmapsOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? MinCmapsOfScreen(screen) : 0;
}

int
XMaxCmapsOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? MaxCmapsOfScreen(screen) : 0;
}

Bool
XDoesSaveUnders(
    Screen *screen)
{
    return (screen != NULL) ? DoesSaveUnders(screen) : False;
}

int
XDoesBackingStore(
    Screen *screen)
{
    return (screen != NULL) ? DoesBackingStore(screen) : NotUseful;
}

long
XEventMaskOfScreen(
    Screen *screen)
{
    return (screen != NULL) ? EventMaskOfScreen(screen) : 0L;
}

int
XScreenNumberOfScreen(
    Screen *screen)
{
    Display *display;
    int i;

    if (screen == NULL) {
	return -1;
    }
    display = DisplayOfScreen(screen);
    if (display == NULL) {
	return -1;
    }
    for (i = 0; i < ScreenCount(display); i++) {
	if (ScreenOfDisplay(display, i) == screen) {
	    return i;
	}
    }
    return -1;
}

Status
XInitThreads(void)
{
    BrowserInitDisplay();
    browserDisplay.threadsInitialized = 1;
    return True;
}

Status
XFreeThreads(void)
{
    BrowserInitDisplay();
    browserDisplay.threadsInitialized = 0;
    browserDisplay.displayLockDepth = 0;
    return True;
}

void
XLockDisplay(
    Display *display)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (display == &browserDisplay.display) {
	browserDisplay.displayLockDepth++;
    }
}

void
XUnlockDisplay(
    Display *display)
{
    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if ((display == &browserDisplay.display)
	    && (browserDisplay.displayLockDepth > 0)) {
	browserDisplay.displayLockDepth--;
    }
}

Bool
XSupportsLocale(void)
{
    return True;
}

Pixmap
XCreatePixmap(
    Display *display,
    Drawable drawable,
    unsigned int width,
    unsigned int height,
    unsigned int depth)
{
    X11WasmResource *drawablePtr;
    X11WasmResource *resourcePtr;

    drawablePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_CREATE_PIXMAP);
    if (drawablePtr == NULL) {
	return None;
    }
    if (!BrowserValidateCard16(&display, (long)width, 0,
	    X11_WASM_XREQ_CREATE_PIXMAP)) {
	return None;
    }
    if (!BrowserValidateCard16(&display, (long)height, 0,
	    X11_WASM_XREQ_CREATE_PIXMAP)) {
	return None;
    }
    if (!BrowserValueOrError(&display, depth,
	    (depth == 1U)
	    || (depth == (unsigned int)DefaultDepth(display,
		    DefaultScreen(display))), X11_WASM_XREQ_CREATE_PIXMAP)) {
	return None;
    }
    resourcePtr = X11WasmCreateResource(display, X11_WASM_RESOURCE_PIXMAP);
    resourcePtr->width = width;
    resourcePtr->height = height;
    resourcePtr->depth = depth;
    resourcePtr->atts.colormap = (drawablePtr->atts.colormap != None)
	    ? drawablePtr->atts.colormap : browserDisplay.defaultColormap;
    (void)X11WasmJsCreatePixmap(BrowserJsXId(resourcePtr->id),
	    BrowserJsXId(drawable), width, height, depth);
    return (Pixmap)resourcePtr->id;
}

int
XFreePixmap(
    Display *display,
    Pixmap pixmap)
{
    if (BrowserPixmapResourceOrError(&display, pixmap,
	    X11_WASM_XREQ_FREE_PIXMAP) == NULL) {
	return Success;
    }
    (void)X11WasmJsDestroyResource(BrowserJsXId(pixmap));
    X11WasmDeleteResource(pixmap);
    return Success;
}

Colormap
XCreateColormap(
    Display *display,
    Window window,
    Visual *visual,
    int alloc)
{
    X11WasmResource *resourcePtr;
    Visual *defaultVisual;

    if (display == NULL) {
	display = X11WasmGetDisplay(NULL);
    }
    if (BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_CREATE_COLORMAP) == NULL) {
	return None;
    }
    defaultVisual = DefaultVisual(display, DefaultScreen(display));
    if (visual == NULL) {
	visual = defaultVisual;
    }
    if ((visual == NULL) || (visual->visualid != defaultVisual->visualid)) {
	(void)BrowserReportXError(display, BadMatch, (XID)window,
		X11_WASM_XREQ_CREATE_COLORMAP, 0);
	return None;
    }
    if ((alloc != AllocNone) && (alloc != AllocAll)) {
	(void)BrowserReportXError(display, BadValue, (XID)alloc,
		X11_WASM_XREQ_CREATE_COLORMAP, 0);
	return None;
    }
    resourcePtr = X11WasmCreateResource(display, X11_WASM_RESOURCE_COLORMAP);
    return (Colormap)resourcePtr->id;
}

Colormap
XCopyColormapAndFree(
    Display *display,
    Colormap colormap)
{
    Colormap copy;
    X11WasmResource *sourcePtr;
    X11WasmResource *copyPtr;

    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_COPY_COLORMAP_AND_FREE, 0)) {
	return None;
    }
    sourcePtr = BrowserColormapResource(colormap);
    copy = XCreateColormap(display, browserDisplay.rootWindow,
	    DefaultVisual(display, DefaultScreen(display)), AllocNone);
    if (copy == None) {
	return None;
    }
    copyPtr = BrowserColormapResource(copy);
    if (copyPtr != NULL) {
	copyPtr->colormapInstalled = (sourcePtr != NULL)
		? sourcePtr->colormapInstalled : 1;
    }
    BrowserCopyColormapColors(colormap, copy);
    return copy;
}

int
XFreeColormap(
    Display *display,
    Colormap colormap)
{
    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_FREE_COLORMAP, 0)) {
	return Success;
    }
    if (colormap == browserDisplay.defaultColormap) {
	return Success;
    }
    X11WasmDeleteResource(colormap);
    return Success;
}

int
XInstallColormap(
    Display *display,
    Colormap colormap)
{
    X11WasmResource *resourcePtr;

    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_INSTALL_COLORMAP, 0)) {
	return Success;
    }
    resourcePtr = BrowserColormapResource(colormap);
    if (resourcePtr != NULL) {
	resourcePtr->colormapInstalled = 1;
    }
    BrowserNotifyColormapInstallState(colormap, ColormapInstalled);
    return Success;
}

int
XUninstallColormap(
    Display *display,
    Colormap colormap)
{
    X11WasmResource *resourcePtr;

    if (!BrowserColormapOrError(&display, colormap,
	    X11_WASM_XREQ_UNINSTALL_COLORMAP, 0)) {
	return Success;
    }
    resourcePtr = BrowserColormapResource(colormap);
    if (resourcePtr != NULL) {
	resourcePtr->colormapInstalled = 0;
    }
    BrowserNotifyColormapInstallState(colormap, ColormapUninstalled);
    return Success;
}

Colormap *
XListInstalledColormaps(
    Display *display,
    Window window,
    int *countReturn)
{
    X11WasmResource *windowPtr;
    Colormap colormap;
    Colormap *colormaps;

    windowPtr = BrowserWindowResourceOrError(&display, window,
	    X11_WASM_XREQ_LIST_INSTALLED_COLORMAPS);
    if (windowPtr == NULL) {
	if (countReturn != NULL) {
	    *countReturn = 0;
	}
	return NULL;
    }
    colormap = (windowPtr->atts.colormap != None)
	    ? windowPtr->atts.colormap : browserDisplay.defaultColormap;
    if (!BrowserWindowColormapInstalled(windowPtr)) {
	colormap = browserDisplay.defaultColormap;
    }
    colormaps = (Colormap *)ckalloc(sizeof(Colormap));
    colormaps[0] = colormap;
    if (countReturn != NULL) {
	*countReturn = 1;
    }
    return colormaps;
}

GC
XCreateGC(
    Display *display,
    Drawable drawable,
    unsigned long valueMask,
    XGCValues *values)
{
    X11WasmResource *resourcePtr;
    GC gc;
    GContext gcontext;

    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_CREATE_GC);
    if (resourcePtr == NULL) {
	return NULL;
    }
    if (!BrowserGCValueMaskOrError(&display, valueMask,
	    X11_WASM_XREQ_CREATE_GC)) {
	return NULL;
    }
    if (!BrowserValidateGCValueEnums(&display, valueMask, values,
	    X11_WASM_XREQ_CREATE_GC)) {
	return NULL;
    }
    if (!BrowserValidateGCValueReferences(&display, valueMask, values,
	    resourcePtr->depth, X11_WASM_XREQ_CREATE_GC)) {
	return NULL;
    }
    gc = (GC)ckalloc(sizeof(XGCValues));
    BrowserInitGC(display, gc);
    BrowserApplyGCValues(gc, valueMask, values);
    gcontext = (GContext)X11WasmAllocXId(display);
    BrowserRegisterGC(gc, gcontext, resourcePtr->depth);
    (void)XNoOp(display);
    return gc;
}

int
XFreeGC(
    Display *display,
    GC gc)
{
    if (!BrowserUnregisterGC(gc)) {
	(void)BrowserReportXError(display, BadGC, (XID)(uintptr_t)gc,
		X11_WASM_XREQ_FREE_GC, 0);
	return Success;
    }
    BrowserDeleteGCClip(gc);
    BrowserDeleteGCDashes(gc);
    ckfree(gc);
    return XNoOp(display);
}

void
XFlushGC(
    Display *display,
    GC gc)
{
    (void)display;
    (void)gc;
}

int
XChangeGC(
    Display *display,
    GC gc,
    unsigned long mask,
    XGCValues *values)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (!BrowserGCValueMaskOrError(&display, mask,
	    X11_WASM_XREQ_CHANGE_GC)) {
	return Success;
    }
    if (!BrowserValidateGCValueEnums(&display, mask, values,
	    X11_WASM_XREQ_CHANGE_GC)) {
	return Success;
    }
    if (!BrowserValidateGCValueReferences(&display, mask, values,
	    BrowserGCDepth(gc), X11_WASM_XREQ_CHANGE_GC)) {
	return Success;
    }
    BrowserApplyGCValues(gc, mask, values);
    return XNoOp(display);
}

int
XCopyGC(
    Display *display,
    GC src,
    unsigned long valuemask,
    GC dest)
{
    if (BrowserGCOrError(&display, src, X11_WASM_XREQ_COPY_GC) == NULL) {
	return Success;
    }
    if (BrowserGCOrError(&display, dest, X11_WASM_XREQ_COPY_GC) == NULL) {
	return Success;
    }
    if (!BrowserGCValueMaskOrError(&display, valuemask,
	    X11_WASM_XREQ_COPY_GC)) {
	return Success;
    }
    if (!BrowserGCDepthsMatchOrError(&display, src, dest,
	    X11_WASM_XREQ_COPY_GC)) {
	return Success;
    }
    BrowserCopyGCComponents(src, dest, valuemask);
    return XNoOp(display);
}

int
XGetGCValues(
    Display *display,
    GC gc,
    unsigned long valuemask,
    XGCValues *valuesReturn)
{
    (void)display;
    if (!BrowserIsKnownGC(gc) || (valuesReturn == NULL)) {
	return False;
    }
    if (!BrowserGCGetValueMaskIsValid(valuemask)) {
	return False;
    }
    memset(valuesReturn, 0, sizeof(*valuesReturn));
    BrowserCopyGCValueFields(gc, valuesReturn, valuemask);
    return True;
}

int
XSetForeground(
    Display *display,
    GC gc,
    unsigned long foreground)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) != NULL) {
	gc->foreground = foreground;
	return XNoOp(display);
    }
    return Success;
}

int
XSetBackground(
    Display *display,
    GC gc,
    unsigned long background)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) != NULL) {
	gc->background = background;
	return XNoOp(display);
    }
    return Success;
}

int
XSetPlaneMask(
    Display *display,
    GC gc,
    unsigned long planeMask)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) != NULL) {
	gc->plane_mask = planeMask;
	return XNoOp(display);
    }
    return Success;
}

int
XSetState(
    Display *display,
    GC gc,
    unsigned long foreground,
    unsigned long background,
    int function,
    unsigned long planeMask)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCFunction(&display, function,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->foreground = foreground;
	gc->background = background;
	gc->function = function;
	gc->plane_mask = planeMask;
	return XNoOp(display);
    }
    return Success;
}

int
XSetFont(
    Display *display,
    GC gc,
    Font font)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserFontResourceOrError(&display, font,
	    X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    gc->font = font;
    return XNoOp(display);
}

int
XSetFunction(
    Display *display,
    GC gc,
    int function)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCFunction(&display, function,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->function = function;
	return XNoOp(display);
    }
    return Success;
}

int
XSetGraphicsExposures(
    Display *display,
    GC gc,
    Bool graphicsExposures)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCBool(&display, graphicsExposures,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->graphics_exposures = graphicsExposures;
	return XNoOp(display);
    }
    return Success;
}

int
XSetSubwindowMode(
    Display *display,
    GC gc,
    int subwindowMode)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCSubwindowMode(&display, subwindowMode,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->subwindow_mode = subwindowMode;
	return XNoOp(display);
    }
    return Success;
}

int
XSetFillStyle(
    Display *display,
    GC gc,
    int fillStyle)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCFillStyle(&display, fillStyle,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->fill_style = fillStyle;
	return XNoOp(display);
    }
    return Success;
}

int
XSetFillRule(
    Display *display,
    GC gc,
    int fillRule)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCFillRule(&display, fillRule,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->fill_rule = fillRule;
	return XNoOp(display);
    }
    return Success;
}

int
XSetArcMode(
    Display *display,
    GC gc,
    int arcMode)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateGCArcMode(&display, arcMode,
	    X11_WASM_XREQ_CHANGE_GC)) {
	gc->arc_mode = arcMode;
	return XNoOp(display);
    }
    return Success;
}

int
XSetTile(
    Display *display,
    GC gc,
    Pixmap tile)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (!BrowserValidateGCPixmapDepth(&display, tile, BrowserGCDepth(gc),
	    X11_WASM_XREQ_CHANGE_GC, 0)) {
	return Success;
    }
    gc->tile = tile;
    return XNoOp(display);
}

int
XSetStipple(
    Display *display,
    GC gc,
    Pixmap stipple)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (!BrowserValidateGCPixmapDepth(&display, stipple, 1U,
	    X11_WASM_XREQ_CHANGE_GC, 0)) {
	return Success;
    }
    gc->stipple = stipple;
    return XNoOp(display);
}

int XSetClipMask(Display *display, GC gc, Pixmap pixmap)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (!BrowserValidateGCPixmapDepth(&display, pixmap, 1U,
	    X11_WASM_XREQ_CHANGE_GC, 1)) {
	return Success;
    }
    gc->clip_mask = pixmap;
    BrowserSetGCClip(gc, 0, NULL);
    return XNoOp(display);
}
int XSetClipOrigin(Display *display, GC gc, int x, int y)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) != NULL) {
	gc->clip_x_origin = x;
	gc->clip_y_origin = y;
	return XNoOp(display);
    }
    return Success;
}
int XSetTSOrigin(Display *display, GC gc, int x, int y)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) != NULL) {
	gc->ts_x_origin = x;
	gc->ts_y_origin = y;
	return XNoOp(display);
    }
    return Success;
}

int
XSetLineAttributes(
    Display *display,
    GC gc,
    unsigned int lineWidth,
    int lineStyle,
    int capStyle,
    int joinStyle)
{
    if (BrowserGCOrError(&display, gc, X11_WASM_XREQ_CHANGE_GC) == NULL) {
	return Success;
    }
    if (BrowserValidateCard16(&display, (long)lineWidth, 1,
	    X11_WASM_XREQ_CHANGE_GC)
	    && BrowserValidateGCLineStyle(&display, lineStyle,
		    X11_WASM_XREQ_CHANGE_GC)
	    && BrowserValidateGCCapStyle(&display, capStyle,
		    X11_WASM_XREQ_CHANGE_GC)
	    && BrowserValidateGCJoinStyle(&display, joinStyle,
		    X11_WASM_XREQ_CHANGE_GC)) {
	gc->line_width = (int)lineWidth;
	gc->line_style = lineStyle;
	gc->cap_style = capStyle;
	gc->join_style = joinStyle;
	return XNoOp(display);
    }
    return Success;
}

int
XSetDashes(
    Display *display,
    GC gc,
    int dashOffset,
    _Xconst char *dashList,
    int n)
{
    if ((BrowserGCOrError(&display, gc, X11_WASM_XREQ_SET_DASHES) != NULL)
	    && BrowserValidateCard16(&display, (long)dashOffset, 1,
		    X11_WASM_XREQ_SET_DASHES)
	    && BrowserValidateGCDashList(&display, dashList, n,
		    X11_WASM_XREQ_SET_DASHES)) {
	gc->dash_offset = dashOffset;
	gc->dashes = dashList[0];
	BrowserSetGCDashes(gc, dashList, n);
	return XNoOp(display);
    }
    return Success;
}

int
XSetClipRectangles(
    Display *display,
    GC gc,
    int clipXOrigin,
    int clipYOrigin,
    XRectangle rectangles[],
    int n,
    int ordering)
{
    if (BrowserGCOrError(&display, gc,
	    X11_WASM_XREQ_SET_CLIP_RECTANGLES) == NULL) {
	return Success;
    }
    if (!BrowserValidateGCClipRectangles(&display, rectangles, n, ordering,
	    X11_WASM_XREQ_SET_CLIP_RECTANGLES)) {
	return Success;
    }
    gc->clip_x_origin = clipXOrigin;
    gc->clip_y_origin = clipYOrigin;
    gc->clip_mask = None;
    BrowserSetGCClipRects(gc, 1, rectangles, n);
    return XNoOp(display);
}

int
XSetRegion(
    Display *display,
    GC gc,
    Region region)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;

    if (BrowserGCOrError(&display, gc,
	    X11_WASM_XREQ_SET_CLIP_RECTANGLES) == NULL) {
	return Success;
    }
    gc->clip_x_origin = 0;
    gc->clip_y_origin = 0;
    gc->clip_mask = None;
    if ((regionPtr == NULL) || regionPtr->empty) {
	BrowserSetGCClipRects(gc, 1, NULL, 0);
	return XNoOp(display);
    }
    BrowserSetGCClipRects(gc, 1, regionPtr->rects, regionPtr->rectCount);
    return XNoOp(display);
}

Region
XCreateRegion(void)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)ckalloc(sizeof(X11WasmRegion));
    memset(regionPtr, 0, sizeof(X11WasmRegion));
    regionPtr->empty = 1;
    return (Region)regionPtr;
}

Region
XPolygonRegion(
    XPoint *points,
    int npoints,
    int fillRule)
{
    Region region = XCreateRegion();
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    BrowserPolygonIntersection *intersections;
    XRectangle *rects = NULL;
    int rectCount = 0;
    int minY, maxY;
    int i, y;

    if ((points == NULL) || (npoints < 3)
	    || (fillRule < EvenOddRule) || (fillRule > WindingRule)) {
	return region;
    }
    minY = maxY = points[0].y;
    for (i = 1; i < npoints; i++) {
	if (points[i].y < minY) {
	    minY = points[i].y;
	}
	if (points[i].y > maxY) {
	    maxY = points[i].y;
	}
    }
    if (minY == maxY) {
	return region;
    }
    intersections = (BrowserPolygonIntersection *)ckalloc(
	    sizeof(BrowserPolygonIntersection) * (size_t)npoints);
    for (y = minY; y < maxY; y++) {
	double scanY = (double)y + 0.5;
	int intersectionCount = 0;

	for (i = 0; i < npoints; i++) {
	    XPoint *aPtr = &points[i];
	    XPoint *bPtr = &points[(i + 1) % npoints];
	    int y1 = aPtr->y;
	    int y2 = bPtr->y;
	    int edgeTop, edgeBottom;
	    double t;

	    if (y1 == y2) {
		continue;
	    }
	    edgeTop = (y1 < y2) ? y1 : y2;
	    edgeBottom = (y1 > y2) ? y1 : y2;
	    if ((scanY < (double)edgeTop) || (scanY >= (double)edgeBottom)) {
		continue;
	    }
	    t = (scanY - (double)y1) / (double)(y2 - y1);
	    intersections[intersectionCount].x = (double)aPtr->x
		    + t * (double)(bPtr->x - aPtr->x);
	    intersections[intersectionCount].winding = (y2 > y1) ? 1 : -1;
	    intersectionCount++;
	}
	if (intersectionCount < 2) {
	    continue;
	}
	BrowserSortPolygonIntersections(intersections, intersectionCount);
	if (fillRule == EvenOddRule) {
	    for (i = 0; i + 1 < intersectionCount; i += 2) {
		BrowserAppendPolygonSpan(&rects, &rectCount,
			intersections[i].x, intersections[i + 1].x, y);
	    }
	} else {
	    int winding = 0;
	    double spanStart = 0.0;

	    for (i = 0; i < intersectionCount; i++) {
		int oldWinding = winding;

		winding += intersections[i].winding;
		if ((oldWinding == 0) && (winding != 0)) {
		    spanStart = intersections[i].x;
		} else if ((oldWinding != 0) && (winding == 0)) {
		    BrowserAppendPolygonSpan(&rects, &rectCount, spanStart,
			    intersections[i].x, y);
		}
	    }
	}
    }
    ckfree(intersections);
    BrowserRegionSetRectangles(regionPtr, rects, rectCount);
    if (rects != NULL) {
	ckfree(rects);
    }
    return region;
}

int
XDestroyRegion(
    Region region)
{
    if (region != NULL) {
	BrowserRegionClear((X11WasmRegion *)region);
	ckfree(region);
    }
    return Success;
}

Bool
XEmptyRegion(
    Region region)
{
    return (region == NULL) || ((X11WasmRegion *)region)->empty;
}

int
XClipBox(
    Region region,
    XRectangle *rectReturn)
{
    if (rectReturn != NULL) {
	if (region == NULL) {
	    memset(rectReturn, 0, sizeof(XRectangle));
	} else {
	    *rectReturn = ((X11WasmRegion *)region)->extents;
	}
    }
    return Success;
}

int
XRectInRegion(
    Region region,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    XRectangle rect;
    XRectangle intersection;
    int intersects = 0;
    int i;

    if ((regionPtr == NULL) || regionPtr->empty || (width == 0)
	    || (height == 0)) {
	return RectangleOut;
    }
    BrowserSetRectangleBounds(&rect, x, y, x + (int)width,
	    y + (int)height);
    if (!BrowserIntersectRectangles(&regionPtr->extents, &rect,
	    &intersection)) {
	return RectangleOut;
    }
    for (i = 0; i < regionPtr->rectCount; i++) {
	if (BrowserIntersectRectangles(&regionPtr->rects[i], &rect,
		&intersection)) {
	    intersects = 1;
	    break;
	}
    }
    if (!intersects) {
	return RectangleOut;
    }
    if (BrowserRectangleCoveredByList(&rect, regionPtr->rects,
	    regionPtr->rectCount)) {
	return RectangleIn;
    }
    return RectanglePart;
}

Bool
XPointInRegion(
    Region region,
    int x,
    int y)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    int i;

    if ((regionPtr == NULL) || regionPtr->empty) {
	return False;
    }
    if ((x < regionPtr->extents.x)
	    || (x >= regionPtr->extents.x + (int)regionPtr->extents.width)
	    || (y < regionPtr->extents.y)
	    || (y >= regionPtr->extents.y + (int)regionPtr->extents.height)) {
	return False;
    }
    for (i = 0; i < regionPtr->rectCount; i++) {
	const XRectangle *rectPtr = &regionPtr->rects[i];
	int right = rectPtr->x + (int)rectPtr->width;
	int bottom = rectPtr->y + (int)rectPtr->height;

	if ((x >= rectPtr->x) && (x < right)
		&& (y >= rectPtr->y) && (y < bottom)) {
	    return True;
	}
    }
    return False;
}

Bool
XEqualRegion(
    Region regionA,
    Region regionB)
{
    X11WasmRegion *aPtr = (X11WasmRegion *)regionA;
    X11WasmRegion *bPtr = (X11WasmRegion *)regionB;
    int i;

    if ((aPtr == NULL) || aPtr->empty) {
	return ((bPtr == NULL) || bPtr->empty) ? True : False;
    }
    if ((bPtr == NULL) || bPtr->empty) {
	return False;
    }
    if ((aPtr->extents.x != bPtr->extents.x)
	    || (aPtr->extents.y != bPtr->extents.y)
	    || (aPtr->extents.width != bPtr->extents.width)
	    || (aPtr->extents.height != bPtr->extents.height)) {
	return False;
    }
    for (i = 0; i < aPtr->rectCount; i++) {
	if (!BrowserRectangleCoveredByList(&aPtr->rects[i], bPtr->rects,
		bPtr->rectCount)) {
	    return False;
	}
    }
    for (i = 0; i < bPtr->rectCount; i++) {
	if (!BrowserRectangleCoveredByList(&bPtr->rects[i], aPtr->rects,
		aPtr->rectCount)) {
	    return False;
	}
    }
    return True;
}

static short
BrowserRegionOffsetShort(
    short value,
    int delta)
{
    long result = (long)value + (long)delta;

    if (result > SHRT_MAX) {
	return SHRT_MAX;
    }
    if (result < SHRT_MIN) {
	return SHRT_MIN;
    }
    return (short)result;
}

int
XOffsetRegion(
    Region region,
    int dx,
    int dy)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    if ((regionPtr != NULL) && !regionPtr->empty) {
	int i;

	regionPtr->extents.x = BrowserRegionOffsetShort(
		regionPtr->extents.x, dx);
	regionPtr->extents.y = BrowserRegionOffsetShort(
		regionPtr->extents.y, dy);
	for (i = 0; i < regionPtr->rectCount; i++) {
	    regionPtr->rects[i].x = BrowserRegionOffsetShort(
		    regionPtr->rects[i].x, dx);
	    regionPtr->rects[i].y = BrowserRegionOffsetShort(
		    regionPtr->rects[i].y, dy);
	}
    }
    return Success;
}

int
XShrinkRegion(
    Region region,
    int dx,
    int dy)
{
    X11WasmRegion *regionPtr = (X11WasmRegion *)region;
    XRectangle *rects = NULL;
    int count = 0;
    int i;

    if ((regionPtr == NULL) || regionPtr->empty) {
	return Success;
    }
    for (i = 0; i < regionPtr->rectCount; i++) {
	XRectangle rect;
	const XRectangle *sourcePtr = &regionPtr->rects[i];
	int left = sourcePtr->x + dx;
	int top = sourcePtr->y + dy;
	int right = sourcePtr->x + (int)sourcePtr->width - dx;
	int bottom = sourcePtr->y + (int)sourcePtr->height - dy;

	BrowserSetRectangleBounds(&rect, left, top, right, bottom);
	BrowserAppendRectangle(&rects, &count, &rect);
    }
    BrowserRegionSetRectangles(regionPtr, rects, count);
    if (rects != NULL) {
	ckfree(rects);
    }
    return Success;
}

int
XUnionRectWithRegion(
    XRectangle *rectangle,
    Region source,
    Region dest)
{
    X11WasmRegion *sourcePtr = (X11WasmRegion *)source;
    X11WasmRegion *destPtr = (X11WasmRegion *)dest;

    if (destPtr == NULL) {
	return Success;
    }
    if (destPtr != sourcePtr) {
	if ((sourcePtr != NULL) && !sourcePtr->empty) {
	    BrowserRegionSetRectangles(destPtr, sourcePtr->rects,
		    sourcePtr->rectCount);
	} else {
	    BrowserRegionClear(destPtr);
	}
    }
    BrowserRegionAppendRectangle(destPtr, rectangle);
    return Success;
}

int
XUnionRegion(
    Region sourceA,
    Region sourceB,
    Region dest)
{
    X11WasmRegion *aPtr = (X11WasmRegion *)sourceA;
    X11WasmRegion *bPtr = (X11WasmRegion *)sourceB;
    X11WasmRegion *destPtr = (X11WasmRegion *)dest;
    XRectangle *rects = NULL;
    int count = 0;
    int i;

    if (destPtr == NULL) {
	return Success;
    }
    if ((aPtr != NULL) && !aPtr->empty) {
	for (i = 0; i < aPtr->rectCount; i++) {
	    BrowserAppendRectangle(&rects, &count, &aPtr->rects[i]);
	}
    }
    if ((bPtr != NULL) && !bPtr->empty) {
	for (i = 0; i < bPtr->rectCount; i++) {
	    BrowserAppendRectangle(&rects, &count, &bPtr->rects[i]);
	}
    }
    BrowserRegionSetRectangles(destPtr, rects, count);
    if (rects != NULL) {
	ckfree(rects);
    }
    return Success;
}

int
XIntersectRegion(
    Region sourceA,
    Region sourceB,
    Region dest)
{
    X11WasmRegion *aPtr = (X11WasmRegion *)sourceA;
    X11WasmRegion *bPtr = (X11WasmRegion *)sourceB;
    X11WasmRegion *destPtr = (X11WasmRegion *)dest;
    XRectangle *rects = NULL;
    int count = 0;
    int i, j;

    if (destPtr == NULL) {
	return Success;
    }
    if ((aPtr == NULL) || aPtr->empty || (bPtr == NULL) || bPtr->empty) {
	BrowserRegionClear(destPtr);
	return Success;
    }
    for (i = 0; i < aPtr->rectCount; i++) {
	for (j = 0; j < bPtr->rectCount; j++) {
	    XRectangle intersection;

	    if (BrowserIntersectRectangles(&aPtr->rects[i], &bPtr->rects[j],
		    &intersection)) {
		BrowserAppendRectangle(&rects, &count, &intersection);
	    }
	}
    }
    BrowserRegionSetRectangles(destPtr, rects, count);
    if (rects != NULL) {
	ckfree(rects);
    }
    return Success;
}

int
XSubtractRegion(
    Region sourceA,
    Region sourceB,
    Region dest)
{
    X11WasmRegion *aPtr = (X11WasmRegion *)sourceA;
    X11WasmRegion *bPtr = (X11WasmRegion *)sourceB;
    X11WasmRegion *destPtr = (X11WasmRegion *)dest;
    XRectangle *current = NULL;
    int currentCount = 0;
    int i, j;

    if (destPtr == NULL) {
	return Success;
    }
    if ((aPtr == NULL) || aPtr->empty) {
	BrowserRegionClear(destPtr);
	return Success;
    }
    current = BrowserCopyRectangleList(aPtr->rects, aPtr->rectCount,
	    &currentCount, NULL);
    for (i = 0; (bPtr != NULL) && !bPtr->empty && (i < bPtr->rectCount)
	    && (currentCount > 0); i++) {
	XRectangle *next = NULL;
	int nextCount = 0;

	for (j = 0; j < currentCount; j++) {
	    BrowserSubtractRectangle(&current[j], &bPtr->rects[i], &next,
		    &nextCount);
	}
	if (current != NULL) {
	    ckfree(current);
	}
	current = next;
	currentCount = nextCount;
    }
    BrowserRegionSetRectangles(destPtr, current, currentCount);
    if (current != NULL) {
	ckfree(current);
    }
    return Success;
}

int
XXorRegion(
    Region sourceA,
    Region sourceB,
    Region dest)
{
    Region onlyA;
    Region onlyB;

    if (dest == NULL) {
	return Success;
    }
    onlyA = XCreateRegion();
    onlyB = XCreateRegion();
    (void)XSubtractRegion(sourceA, sourceB, onlyA);
    (void)XSubtractRegion(sourceB, sourceA, onlyB);
    (void)XUnionRegion(onlyA, onlyB, dest);
    (void)XDestroyRegion(onlyA);
    (void)XDestroyRegion(onlyB);
    return Success;
}

XImage *
XCreateImage(
    Display *display,
    Visual *visual,
    unsigned int depth,
    int format,
    int offset,
    char *data,
    unsigned int width,
    unsigned int height,
    int bitmapPad,
    int bytesPerLine)
{
    XImage *imagePtr;
    int bitsPerPixel;
    int imageBitmapPad;
    int minBytesPerLine;
    unsigned long long xyBitsPerLine;

    (void)display;
    (void)visual;
    if ((depth == 0) || (depth > 32) || !BrowserImageFormatIsValid(format)
	    || ((format == XYBitmap) && (depth != 1)) || (offset < 0)) {
	return NULL;
    }
    imageBitmapPad = (bitmapPad != 0) ? bitmapPad : 32;
    if ((imageBitmapPad != 8) && (imageBitmapPad != 16)
	    && (imageBitmapPad != 32)) {
	return NULL;
    }
    bitsPerPixel = (format == ZPixmap) ? BrowserImageBitsPerPixel(depth) : 1;
    if ((format == XYBitmap) || (format == XYPixmap)) {
	xyBitsPerLine = (unsigned long long)width
		+ (unsigned long long)offset;
	if (xyBitsPerLine > (unsigned long long)INT_MAX) {
	    return NULL;
	}
	minBytesPerLine = BrowserImageBytesPerLine((unsigned int)xyBitsPerLine,
		1, imageBitmapPad);
    } else {
	minBytesPerLine = BrowserImageBytesPerLine(width, bitsPerPixel,
		imageBitmapPad);
    }
    if ((minBytesPerLine <= 0)
	    || ((bytesPerLine != 0) && (bytesPerLine < minBytesPerLine))) {
	return NULL;
    }
    imagePtr = (XImage *)ckalloc(sizeof(XImage));
    memset(imagePtr, 0, sizeof(XImage));
    imagePtr->width = (int)width;
    imagePtr->height = (int)height;
    imagePtr->xoffset = offset;
    imagePtr->format = format;
    imagePtr->data = data;
    imagePtr->byte_order = LSBFirst;
    imagePtr->bitmap_unit = 32;
    imagePtr->bitmap_bit_order = LSBFirst;
    imagePtr->bitmap_pad = imageBitmapPad;
    imagePtr->depth = (int)depth;
    imagePtr->bits_per_pixel = bitsPerPixel;
    imagePtr->bytes_per_line = (bytesPerLine != 0) ? bytesPerLine
	    : minBytesPerLine;
    BrowserSetImageMasks(imagePtr);
    BrowserInitImageFuncs(imagePtr);
    return imagePtr;
}

int
_XInitImageFuncPtrs(
    XImage *image)
{
    if (image != NULL) {
	BrowserInitImageFuncs(image);
    }
    return Success;
}

Status
XInitImage(
    XImage *image)
{
    int bitmapPad;
    int bitsPerPixel;
    int minBytesPerLine;
    unsigned long long xyBitsPerLine;

    if ((image == NULL) || (image->width <= 0) || (image->height <= 0)
	    || (image->depth <= 0) || (image->depth > 32)
	    || !BrowserImageFormatIsValid(image->format)
	    || ((image->format == XYBitmap) && (image->depth != 1))
	    || (image->xoffset < 0)
	    || ((image->byte_order != LSBFirst)
		    && (image->byte_order != MSBFirst))
	    || ((image->bitmap_bit_order != LSBFirst)
		    && (image->bitmap_bit_order != MSBFirst))) {
	return 0;
    }
    bitmapPad = (image->bitmap_pad != 0) ? image->bitmap_pad : 32;
    if ((bitmapPad != 8) && (bitmapPad != 16) && (bitmapPad != 32)) {
	return 0;
    }
    bitsPerPixel = (image->bits_per_pixel != 0) ? image->bits_per_pixel
	    : ((image->format == ZPixmap)
		    ? BrowserImageBitsPerPixel((unsigned int)image->depth) : 1);
    if ((bitsPerPixel <= 0) || (bitsPerPixel > 32)
	    || ((bitsPerPixel != 1) && ((bitsPerPixel % 8) != 0))
	    || ((image->format == ZPixmap) && (bitsPerPixel < image->depth))
	    || (((image->format == XYBitmap) || (image->format == XYPixmap))
		    && (bitsPerPixel != 1))) {
	return 0;
    }
    if ((image->format == XYBitmap) || (image->format == XYPixmap)) {
	xyBitsPerLine = (unsigned long long)image->width
		+ (unsigned long long)image->xoffset;
	if (xyBitsPerLine > (unsigned long long)INT_MAX) {
	    return 0;
	}
	minBytesPerLine = BrowserImageBytesPerLine((unsigned int)xyBitsPerLine,
		1, bitmapPad);
    } else {
	minBytesPerLine = BrowserImageBytesPerLine((unsigned int)image->width,
		bitsPerPixel, bitmapPad);
    }
    if ((minBytesPerLine <= 0)
	    || ((image->bytes_per_line != 0)
		    && (image->bytes_per_line < minBytesPerLine))) {
	return 0;
    }
    image->bitmap_pad = bitmapPad;
    if (image->bitmap_unit == 0) {
	image->bitmap_unit = 32;
    }
    image->bits_per_pixel = bitsPerPixel;
    if (image->bytes_per_line == 0) {
	image->bytes_per_line = minBytesPerLine;
    }
    BrowserSetImageMasks(image);
    BrowserInitImageFuncs(image);
    return 1;
}

int
BrowserGetImageDrawableMatches(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    Drawable drawable,
    int x,
    int y,
    unsigned int width,
    unsigned int height)
{
    if ((resourcePtr->type == X11_WASM_RESOURCE_WINDOW)
	    && !BrowserResourceIsVisible(resourcePtr)) {
	return BrowserBadMatchOrError(displayPtr, (XID)drawable,
		X11_WASM_XREQ_GET_IMAGE);
    }
    if ((x < 0) || (y < 0)
	    || ((unsigned int)x > resourcePtr->width)
	    || ((unsigned int)y > resourcePtr->height)
	    || (width > resourcePtr->width - (unsigned int)x)
	    || (height > resourcePtr->height - (unsigned int)y)) {
	return BrowserBadMatchOrError(displayPtr, (XID)drawable,
		X11_WASM_XREQ_GET_IMAGE);
    }
    return 1;
}

int
BrowserPutImageDrawableMatches(
    Display **displayPtr,
    X11WasmResource *resourcePtr,
    Drawable drawable,
    XImage *image)
{
    if (image == NULL) {
	return 1;
    }
    if (image->format == XYBitmap) {
	if ((image->depth != 1) || (image->bits_per_pixel != 1)) {
	    return BrowserBadMatchOrError(displayPtr, (XID)drawable,
		    X11_WASM_XREQ_PUT_IMAGE);
	}
	return 1;
    }
    if ((image->format == ZPixmap) || (image->format == XYPixmap)) {
	if ((unsigned int)image->depth != resourcePtr->depth) {
	    return BrowserBadMatchOrError(displayPtr, (XID)drawable,
		    X11_WASM_XREQ_PUT_IMAGE);
	}
    }
    return 1;
}

XImage *
BrowserConvertImageForPut(
    X11WasmResource *resourcePtr,
    GC gc,
    XImage *image,
    int srcX,
    int srcY,
    unsigned int width,
    unsigned int height)
{
    XImage *convertedImage;
    int dataLength;
    unsigned long foreground;
    unsigned long background;
    unsigned int x, y;

    if ((image == NULL)
	    || ((image->format != XYBitmap) && (image->format != XYPixmap))) {
	return image;
    }
    convertedImage = XCreateImage(NULL, NULL, resourcePtr->depth, ZPixmap, 0,
	    NULL, width, height, 32, 0);
    if (convertedImage == NULL) {
	return NULL;
    }
    dataLength = BrowserImageStorageLength(convertedImage);
    if (dataLength <= 0) {
	(void)XDestroyImage(convertedImage);
	return NULL;
    }
    convertedImage->data = (char *)ckalloc((unsigned int)dataLength);
    memset(convertedImage->data, 0, (unsigned int)dataLength);
    convertedImage->obdata = (XPointer)convertedImage;
    if (image->format == XYPixmap) {
	for (y = 0; y < height; y++) {
	    for (x = 0; x < width; x++) {
		(void)BrowserImagePutPixel(convertedImage, (int)x, (int)y,
			BrowserImageGetPixel(image, srcX + (int)x,
				srcY + (int)y));
	    }
	}
	return convertedImage;
    }
    if (gc != NULL) {
	foreground = gc->foreground;
	background = gc->background;
    } else if ((resourcePtr != NULL) && (resourcePtr->depth == 1)) {
	foreground = 1UL;
	background = 0UL;
    } else {
	foreground = 0UL;
	background = 0xffffffUL;
    }
    for (y = 0; y < height; y++) {
	for (x = 0; x < width; x++) {
	    unsigned long pixel = BrowserImageGetPixel(image,
		    srcX + (int)x, srcY + (int)y) ? foreground : background;

	    (void)BrowserImagePutPixel(convertedImage, (int)x, (int)y, pixel);
	}
    }
    return convertedImage;
}

XImage *
XGetImage(
    Display *display,
    Drawable drawable,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned long planeMask,
    int format)
{
    X11WasmResource *resourcePtr;
    XImage *imagePtr;
    unsigned int depth;
    int dataLength;

    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_GET_IMAGE);
    if (resourcePtr == NULL) {
	return NULL;
    }
    if (!BrowserValueOrError(&display, format,
	    (format == XYPixmap) || (format == ZPixmap),
	    X11_WASM_XREQ_GET_IMAGE)) {
	return NULL;
    }
    if (!BrowserValidateCard16(&display, (long)width, 1,
	    X11_WASM_XREQ_GET_IMAGE)
	    || !BrowserValidateCard16(&display, (long)height, 1,
		    X11_WASM_XREQ_GET_IMAGE)) {
	return NULL;
    }
    if (!BrowserGetImageDrawableMatches(&display, resourcePtr, drawable,
	    x, y, width, height)) {
	return NULL;
    }
    (void)planeMask;
    depth = resourcePtr->depth;
    imagePtr = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)),
	    depth, format, 0, NULL, width, height, 32, 0);
    if (imagePtr != NULL) {
	dataLength = BrowserImageStorageLength(imagePtr);
	if (dataLength <= 0) {
	    (void)XDestroyImage(imagePtr);
	    return NULL;
	}
	imagePtr->data = (char *)ckalloc((unsigned int)dataLength);
	memset(imagePtr->data, 0, (unsigned int)dataLength);
	imagePtr->obdata = (XPointer)imagePtr;
	if (!BrowserReadPixmapBacking(resourcePtr, imagePtr, x, y, width,
		height)) {
	    if (format == XYPixmap) {
		XImage *zImagePtr = XCreateImage(display,
			DefaultVisual(display, DefaultScreen(display)), depth,
			ZPixmap, 0, NULL, width, height, 32, 0);
		unsigned int ix, iy;
		int zDataLength;

		if (zImagePtr == NULL) {
		    (void)XDestroyImage(imagePtr);
		    return NULL;
		}
		zDataLength = BrowserImageStorageLength(zImagePtr);
		if (zDataLength <= 0) {
		    (void)XDestroyImage(zImagePtr);
		    (void)XDestroyImage(imagePtr);
		    return NULL;
		}
		zImagePtr->data = (char *)ckalloc((unsigned int)zDataLength);
		memset(zImagePtr->data, 0, (unsigned int)zDataLength);
		zImagePtr->obdata = (XPointer)zImagePtr;
		(void)X11WasmJsGetImage(BrowserJsXId(drawable), x, y, width,
			height, zImagePtr->data, zDataLength,
			zImagePtr->bytes_per_line, zImagePtr->bits_per_pixel);
		for (iy = 0; iy < height; iy++) {
		    for (ix = 0; ix < width; ix++) {
			(void)BrowserImagePutPixel(imagePtr, (int)ix, (int)iy,
				BrowserImageGetPixel(zImagePtr, (int)ix, (int)iy));
		    }
		}
		(void)XDestroyImage(zImagePtr);
	    } else {
		(void)X11WasmJsGetImage(BrowserJsXId(drawable), x, y, width,
			height, imagePtr->data, dataLength,
			imagePtr->bytes_per_line, imagePtr->bits_per_pixel);
	    }
	}
	BrowserImageApplyPlaneMask(imagePtr, planeMask);
    }
    return imagePtr;
}

XImage *
XGetSubImage(
    Display *display,
    Drawable drawable,
    int x,
    int y,
    unsigned int width,
    unsigned int height,
    unsigned long planeMask,
    int format,
    XImage *destImage,
    int destX,
    int destY)
{
    X11WasmResource *resourcePtr;
    XImage *sourceImage;
    unsigned int ix, iy;

    if (destImage == NULL) {
	return NULL;
    }
    resourcePtr = BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_GET_IMAGE);
    if (resourcePtr == NULL) {
	return NULL;
    }
    if ((unsigned int)destImage->depth != resourcePtr->depth) {
	(void)BrowserBadMatchOrError(&display, (XID)drawable,
		X11_WASM_XREQ_GET_IMAGE);
	return NULL;
    }
    sourceImage = XGetImage(display, drawable, x, y, width, height, planeMask,
	    format);
    if (sourceImage == NULL) {
	return NULL;
    }
    for (iy = 0; iy < height; iy++) {
	for (ix = 0; ix < width; ix++) {
	    (void)BrowserImagePutPixel(destImage, destX + (int)ix,
		    destY + (int)iy, BrowserImageGetPixel(sourceImage,
			    (int)ix, (int)iy));
	}
    }
    (void)XDestroyImage(sourceImage);
    return destImage;
}

int
XPutImage(
    Display *display,
    Drawable drawable,
    GC gc,
    XImage *image,
    int srcX,
    int srcY,
    int destX,
    int destY,
    unsigned int width,
    unsigned int height)
{
    X11WasmResource *resourcePtr = BrowserDrawableResourceOrError(&display,
	    drawable, X11_WASM_XREQ_PUT_IMAGE);
    XImage *putImage = image;

    if (resourcePtr == NULL) {
	return Success;
    }
    if (!BrowserOptionalGCOrError(&display, gc, X11_WASM_XREQ_PUT_IMAGE)) {
	return Success;
    }
    if (!BrowserValidateCard16(&display, (long)width, 1,
	    X11_WASM_XREQ_PUT_IMAGE)
	    || !BrowserValidateCard16(&display, (long)height, 1,
		    X11_WASM_XREQ_PUT_IMAGE)) {
	return Success;
    }
    if ((image != NULL) && !BrowserValueOrError(&display, image->format,
	    BrowserImageFormatIsValid(image->format),
	    X11_WASM_XREQ_PUT_IMAGE)) {
	return Success;
    }
    if (!BrowserPutImageDrawableMatches(&display, resourcePtr, drawable,
	    image)) {
	return Success;
    }
    BrowserApplyGCStyle(gc);
    BrowserApplyGCClip(gc);
    putImage = BrowserConvertImageForPut(resourcePtr, gc, image,
	    srcX, srcY, width, height);
    if (putImage == NULL) {
	return Success;
    }
    (void)X11WasmJsPutImage(BrowserJsXId(drawable),
	    (putImage != NULL) ? putImage->data : NULL,
	    BrowserImageDataLength(putImage), (putImage == image) ? srcX : 0,
	    (putImage == image) ? srcY : 0, destX, destY, width, height,
	    (putImage != NULL) ? putImage->bytes_per_line : 0,
	    (putImage != NULL) ? putImage->bits_per_pixel : 0);
    BrowserStorePixmapImage(resourcePtr, gc, putImage,
	    (putImage == image) ? srcX : 0, (putImage == image) ? srcY : 0,
	    destX, destY, width, height);
    if ((putImage != NULL) && (putImage != image)) {
	(void)XDestroyImage(putImage);
    }
    BrowserMarkResourceDamage(resourcePtr, destX, destY, width, height);
    return Success;
}

Pixmap
XCreateBitmapFromData(
    Display *display,
    Drawable drawable,
    _Xconst char *data,
    unsigned int width,
    unsigned int height)
{
    Pixmap pixmap = XCreatePixmap(display, drawable, width, height, 1);
    XImage *imagePtr;

    if ((pixmap == None) || (data == NULL)) {
	return pixmap;
    }
    imagePtr = XCreateImage(display, DefaultVisual(display,
	    DefaultScreen(display)), 1, XYBitmap, 0, (char *)data, width,
	    height, 8, BrowserBitmapBytesPerLine(width));
    if (imagePtr == NULL) {
	return pixmap;
    }
    (void)XPutImage(display, pixmap, NULL, imagePtr, 0, 0, 0, 0, width,
	    height);
    (void)BrowserImageDestroy(imagePtr);
    return pixmap;
}

Pixmap
XCreatePixmapFromBitmapData(
    Display *display,
    Drawable drawable,
    char *data,
    unsigned int width,
    unsigned int height,
    unsigned long foreground,
    unsigned long background,
    unsigned int depth)
{
    Pixmap pixmap = XCreatePixmap(display, drawable, width, height, depth);
    XGCValues gcValues;
    XImage *imagePtr;
    GC gc;

    if ((pixmap == None) || (data == NULL)) {
	return pixmap;
    }
    memset(&gcValues, 0, sizeof(gcValues));
    gcValues.foreground = foreground;
    gcValues.background = background;
    gc = XCreateGC(display, pixmap, GCForeground | GCBackground, &gcValues);
    if (gc == NULL) {
	return pixmap;
    }
    imagePtr = XCreateImage(display, DefaultVisual(display,
	    DefaultScreen(display)), 1, XYBitmap, 0, data, width, height, 8,
	    BrowserBitmapBytesPerLine(width));
    if (imagePtr != NULL) {
	(void)XPutImage(display, pixmap, gc, imagePtr, 0, 0, 0, 0, width,
		height);
	(void)BrowserImageDestroy(imagePtr);
    }
    (void)XFreeGC(display, gc);
    return pixmap;
}

static Status
BrowserQueryBestSize(
    Display *display,
    int shape,
    Drawable drawable,
    unsigned int width,
    unsigned int height,
    unsigned int *widthReturn,
    unsigned int *heightReturn)
{
    if (BrowserDrawableResourceOrError(&display, drawable,
	    X11_WASM_XREQ_QUERY_BEST_SIZE) == NULL) {
	return False;
    }
    if (!BrowserValueOrError(&display, shape,
	    (shape == CursorShape) || (shape == TileShape)
	    || (shape == StippleShape), X11_WASM_XREQ_QUERY_BEST_SIZE)) {
	return False;
    }
    if (!BrowserValidateCard16(&display, (long)width, 1,
	    X11_WASM_XREQ_QUERY_BEST_SIZE)
	    || !BrowserValidateCard16(&display, (long)height, 1,
		    X11_WASM_XREQ_QUERY_BEST_SIZE)) {
	return False;
    }
    if (widthReturn != NULL) {
	*widthReturn = width;
    }
    if (heightReturn != NULL) {
	*heightReturn = height;
    }
    return True;
}

Status
XQueryBestCursor(
    Display *display,
    Drawable drawable,
    unsigned int width,
    unsigned int height,
    unsigned int *widthReturn,
    unsigned int *heightReturn)
{
    return BrowserQueryBestSize(display, CursorShape, drawable, width, height,
	    widthReturn, heightReturn);
}

Status
XQueryBestSize(
    Display *display,
    int shape,
    Drawable drawable,
    unsigned int width,
    unsigned int height,
    unsigned int *widthReturn,
    unsigned int *heightReturn)
{
    return BrowserQueryBestSize(display, shape, drawable, width, height,
	    widthReturn, heightReturn);
}

Status
XQueryBestStipple(
    Display *display,
    Drawable drawable,
    unsigned int width,
    unsigned int height,
    unsigned int *widthReturn,
    unsigned int *heightReturn)
{
    return BrowserQueryBestSize(display, StippleShape, drawable, width, height,
	    widthReturn, heightReturn);
}

Status
XQueryBestTile(
    Display *display,
    Drawable drawable,
    unsigned int width,
    unsigned int height,
    unsigned int *widthReturn,
    unsigned int *heightReturn)
{
    return BrowserQueryBestSize(display, TileShape, drawable, width, height,
	    widthReturn, heightReturn);
}
