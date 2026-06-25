/*
 * Runtime.c --
 *
 *	Runtime helpers for the x11.wasm browser X11 port.
 */

#include "XlibInt.h"

static void
X11WasmDStringEnsure(
    X11WasmDString *dsPtr,
    int extra)
{
    int needed;
    char *newString;

    if (dsPtr == NULL) {
	return;
    }
    needed = dsPtr->length + extra + 1;
    if (needed <= dsPtr->spaceAvl) {
	return;
    }
    while (dsPtr->spaceAvl < needed) {
	dsPtr->spaceAvl *= 2;
    }
    newString = (char *)ckalloc((size_t)dsPtr->spaceAvl);
    memcpy(newString, dsPtr->string, (size_t)dsPtr->length);
    if (dsPtr->string != dsPtr->staticSpace) {
	ckfree(dsPtr->string);
    }
    dsPtr->string = newString;
    dsPtr->string[dsPtr->length] = '\0';
}

void
X11WasmDStringInit(
    X11WasmDString *dsPtr)
{
    if (dsPtr == NULL) {
	return;
    }
    dsPtr->string = dsPtr->staticSpace;
    dsPtr->length = 0;
    dsPtr->spaceAvl = X11_WASM_DSTRING_STATIC_SIZE;
    dsPtr->staticSpace[0] = '\0';
}

char *
X11WasmDStringAppend(
    X11WasmDString *dsPtr,
    const char *bytes,
    int length)
{
    if (dsPtr == NULL) {
	return NULL;
    }
    if (bytes == NULL) {
	bytes = "";
    }
    if (length < 0) {
	length = (int)strlen(bytes);
    }
    X11WasmDStringEnsure(dsPtr, length);
    memcpy(dsPtr->string + dsPtr->length, bytes, (size_t)length);
    dsPtr->length += length;
    dsPtr->string[dsPtr->length] = '\0';
    return dsPtr->string;
}

char *
X11WasmDStringAppendElement(
    X11WasmDString *dsPtr,
    const char *element)
{
    const char *p;
    int needsBraces = 0;

    if (dsPtr == NULL) {
	return NULL;
    }
    if (element == NULL) {
	element = "";
    }
    for (p = element; *p != '\0'; p++) {
	if (isspace((unsigned char)*p) || (*p == '{') || (*p == '}')
		|| (*p == '\\')) {
	    needsBraces = 1;
	    break;
	}
    }
    if (dsPtr->length > 0) {
	(void)X11WasmDStringAppend(dsPtr, " ", 1);
    }
    if (!needsBraces) {
	return X11WasmDStringAppend(dsPtr, element, -1);
    }
    (void)X11WasmDStringAppend(dsPtr, "{", 1);
    for (p = element; *p != '\0'; p++) {
	if ((*p == '{') || (*p == '}') || (*p == '\\')) {
	    (void)X11WasmDStringAppend(dsPtr, "\\", 1);
	}
	(void)X11WasmDStringAppend(dsPtr, p, 1);
    }
    return X11WasmDStringAppend(dsPtr, "}", 1);
}

void
X11WasmDStringFree(
    X11WasmDString *dsPtr)
{
    if (dsPtr == NULL) {
	return;
    }
    if ((dsPtr->string != NULL) && (dsPtr->string != dsPtr->staticSpace)) {
	ckfree(dsPtr->string);
    }
    X11WasmDStringInit(dsPtr);
}

X11WasmEncoding
X11WasmGetEncoding(
    const char *name)
{
    (void)name;
    return (X11WasmEncoding)1;
}

void
X11WasmFreeEncoding(
    X11WasmEncoding encoding)
{
    (void)encoding;
}

char *
X11WasmExternalToUtfDString(
    X11WasmEncoding encoding,
    const char *src,
    int srcLen,
    X11WasmDString *dsPtr)
{
    (void)encoding;
    X11WasmDStringInit(dsPtr);
    return X11WasmDStringAppend(dsPtr, src, srcLen);
}

char *
X11WasmUtfToExternalDString(
    X11WasmEncoding encoding,
    const char *src,
    int srcLen,
    X11WasmDString *dsPtr)
{
    (void)encoding;
    X11WasmDStringInit(dsPtr);
    return X11WasmDStringAppend(dsPtr, src, srcLen);
}

int
X11WasmUtfToCodepoint(
    const char *src,
    X11WasmCodepoint *chPtr)
{
    unsigned char c;

    if ((src == NULL) || (src[0] == '\0')) {
	if (chPtr != NULL) {
	    *chPtr = 0;
	}
	return 0;
    }
    c = (unsigned char)src[0];
    if (c < 0x80) {
	if (chPtr != NULL) {
	    *chPtr = c;
	}
	return 1;
    }
    if ((c & 0xe0) == 0xc0) {
	if (chPtr != NULL) {
	    *chPtr = (X11WasmCodepoint)(((c & 0x1f) << 6)
		    | ((unsigned char)src[1] & 0x3f));
	}
	return 2;
    }
    if ((c & 0xf0) == 0xe0) {
	if (chPtr != NULL) {
	    *chPtr = (X11WasmCodepoint)(((c & 0x0f) << 12)
		    | (((unsigned char)src[1] & 0x3f) << 6)
		    | ((unsigned char)src[2] & 0x3f));
	}
	return 3;
    }
    if (chPtr != NULL) {
	*chPtr = (X11WasmCodepoint)'?';
    }
    return 1;
}
