#include "XlibInt.h"

typedef struct X11WasmQuarkEntry {
    XrmQuark quark;
    char *string;
    struct X11WasmQuarkEntry *nextPtr;
} X11WasmQuarkEntry;

static X11WasmQuarkEntry *browserQuarkList = NULL;
static XrmQuark browserNextStringQuark = 1;
static XrmQuark browserNextUniqueQuark = -1;

static X11WasmQuarkEntry *
BrowserFindQuarkByString(
    const char *string)
{
    X11WasmQuarkEntry *entryPtr;

    for (entryPtr = browserQuarkList; entryPtr != NULL;
	    entryPtr = entryPtr->nextPtr) {
	if (strcmp(entryPtr->string, string) == 0) {
	    return entryPtr;
	}
    }
    return NULL;
}

static X11WasmQuarkEntry *
BrowserFindQuarkById(
    XrmQuark quark)
{
    X11WasmQuarkEntry *entryPtr;

    for (entryPtr = browserQuarkList; entryPtr != NULL;
	    entryPtr = entryPtr->nextPtr) {
	if (entryPtr->quark == quark) {
	    return entryPtr;
	}
    }
    return NULL;
}

XrmQuark
XrmStringToQuark(
    _Xconst char *string)
{
    X11WasmQuarkEntry *entryPtr;

    if (string == NULL) {
	return NULLQUARK;
    }
    entryPtr = BrowserFindQuarkByString(string);
    if (entryPtr != NULL) {
	return entryPtr->quark;
    }

    entryPtr = (X11WasmQuarkEntry *)ckalloc(sizeof(*entryPtr));
    entryPtr->quark = browserNextStringQuark++;
    entryPtr->string = BrowserDupString(string);
    entryPtr->nextPtr = browserQuarkList;
    browserQuarkList = entryPtr;
    return entryPtr->quark;
}

XrmQuark
XrmPermStringToQuark(
    _Xconst char *string)
{
    return XrmStringToQuark(string);
}

XrmString
XrmQuarkToString(
    XrmQuark quark)
{
    X11WasmQuarkEntry *entryPtr;

    if (quark <= NULLQUARK) {
	return NULLSTRING;
    }
    entryPtr = BrowserFindQuarkById(quark);
    return (entryPtr != NULL) ? entryPtr->string : NULLSTRING;
}

XrmQuark
XrmUniqueQuark(void)
{
    return browserNextUniqueQuark--;
}
