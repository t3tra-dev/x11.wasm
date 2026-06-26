#include "XlibInt.h"
#include <X11/Xresource.h>

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

static XrmQuark
BrowserQuarkFromRange(
    const char *start,
    const char *end)
{
    size_t length = (size_t)(end - start);
    char *copy = (char *)ckalloc(length + 1U);
    XrmQuark quark;

    if (length > 0U) {
	memcpy(copy, start, length);
    }
    copy[length] = '\0';
    quark = XrmStringToQuark(copy);
    ckfree(copy);
    return quark;
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

void
XrmStringToQuarkList(
    _Xconst char *string,
    XrmQuarkList quarksReturn)
{
    const char *segmentStart;
    const char *p;

    if (quarksReturn == NULL) {
	return;
    }
    if (string == NULL) {
	*quarksReturn = NULLQUARK;
	return;
    }
    segmentStart = string;
    for (p = string; *p != '\0'; p++) {
	if ((*p == '.') || (*p == '*')) {
	    if (p > segmentStart) {
		*quarksReturn++ = BrowserQuarkFromRange(segmentStart, p);
	    }
	    segmentStart = p + 1;
	}
    }
    *quarksReturn++ = BrowserQuarkFromRange(segmentStart, p);
    *quarksReturn = NULLQUARK;
}

void
XrmStringToBindingQuarkList(
    _Xconst char *string,
    XrmBindingList bindingsReturn,
    XrmQuarkList quarksReturn)
{
    const char *segmentStart;
    const char *p;
    XrmBinding binding = XrmBindTightly;

    if (quarksReturn == NULL) {
	return;
    }
    if (string == NULL) {
	*quarksReturn = NULLQUARK;
	return;
    }
    segmentStart = string;
    for (p = string; *p != '\0'; p++) {
	if ((*p == '.') || (*p == '*')) {
	    if (p > segmentStart) {
		if (bindingsReturn != NULL) {
		    *bindingsReturn++ = binding;
		}
		*quarksReturn++ = BrowserQuarkFromRange(segmentStart, p);
		binding = XrmBindTightly;
	    }
	    segmentStart = p + 1;
	    if (*p == '*') {
		binding = XrmBindLoosely;
	    }
	}
    }
    if (bindingsReturn != NULL) {
	*bindingsReturn = binding;
    }
    *quarksReturn++ = BrowserQuarkFromRange(segmentStart, p);
    *quarksReturn = NULLQUARK;
}
