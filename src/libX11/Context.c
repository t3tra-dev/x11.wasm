#include "XlibInt.h"

typedef struct X11WasmContextEntry {
    XID rid;
    XContext context;
    XPointer data;
    struct X11WasmContextEntry *nextPtr;
} X11WasmContextEntry;

static X11WasmContextEntry *browserContextList = NULL;

static X11WasmContextEntry **
BrowserContextEntryLink(
    XID rid,
    XContext context)
{
    X11WasmContextEntry **linkPtr = &browserContextList;

    while (*linkPtr != NULL) {
	X11WasmContextEntry *entryPtr = *linkPtr;

	if ((entryPtr->rid == rid) && (entryPtr->context == context)) {
	    return linkPtr;
	}
	linkPtr = &entryPtr->nextPtr;
    }
    return linkPtr;
}

int
XSaveContext(
    Display *display,
    XID rid,
    XContext context,
    _Xconst char *data)
{
    X11WasmContextEntry **linkPtr;
    X11WasmContextEntry *entryPtr;

    (void)display;
    linkPtr = BrowserContextEntryLink(rid, context);
    if (*linkPtr != NULL) {
	(*linkPtr)->data = (XPointer)data;
	return Success;
    }

    entryPtr = (X11WasmContextEntry *)ckalloc(sizeof(*entryPtr));
    entryPtr->rid = rid;
    entryPtr->context = context;
    entryPtr->data = (XPointer)data;
    entryPtr->nextPtr = browserContextList;
    browserContextList = entryPtr;
    return Success;
}

int
XFindContext(
    Display *display,
    XID rid,
    XContext context,
    XPointer *dataReturn)
{
    X11WasmContextEntry **linkPtr;

    (void)display;
    linkPtr = BrowserContextEntryLink(rid, context);
    if (*linkPtr == NULL) {
	return XCNOENT;
    }
    if (dataReturn != NULL) {
	*dataReturn = (*linkPtr)->data;
    }
    return Success;
}

int
XDeleteContext(
    Display *display,
    XID rid,
    XContext context)
{
    X11WasmContextEntry **linkPtr;
    X11WasmContextEntry *entryPtr;

    (void)display;
    linkPtr = BrowserContextEntryLink(rid, context);
    if (*linkPtr == NULL) {
	return XCNOENT;
    }
    entryPtr = *linkPtr;
    *linkPtr = entryPtr->nextPtr;
    ckfree(entryPtr);
    return Success;
}
