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
    unsigned char b0, b1, b2, b3;
    X11WasmCodepoint ch = 0xfffdU;
    int length = 1;

    if ((src == NULL) || (src[0] == '\0')) {
	if (chPtr != NULL) {
	    *chPtr = 0;
	}
	return 0;
    }
    b0 = UCHAR(src[0]);
    if (b0 < 0x80U) {
	ch = b0;
    } else if ((b0 >= 0xc2U) && (b0 <= 0xdfU)) {
	b1 = UCHAR(src[1]);
	if ((b1 & 0xc0U) == 0x80U) {
	    ch = (X11WasmCodepoint)(((b0 & 0x1fU) << 6)
		    | (b1 & 0x3fU));
	    length = 2;
	}
    } else if ((b0 >= 0xe0U) && (b0 <= 0xefU)) {
	b1 = UCHAR(src[1]);
	if ((b1 & 0xc0U) == 0x80U) {
	    b2 = UCHAR(src[2]);
	} else {
	    b2 = 0;
	}
	if ((b2 & 0xc0U) == 0x80U) {
	    ch = (X11WasmCodepoint)(((b0 & 0x0fU) << 12)
		    | ((b1 & 0x3fU) << 6)
		    | (b2 & 0x3fU));
	    if ((ch >= 0x800U) && ((ch < 0xd800U) || (ch > 0xdfffU))) {
		length = 3;
	    } else {
		ch = 0xfffdU;
	    }
	}
    } else if ((b0 >= 0xf0U) && (b0 <= 0xf4U)) {
	b1 = UCHAR(src[1]);
	if ((b1 & 0xc0U) == 0x80U) {
	    b2 = UCHAR(src[2]);
	} else {
	    b2 = 0;
	}
	if ((b2 & 0xc0U) == 0x80U) {
	    b3 = UCHAR(src[3]);
	} else {
	    b3 = 0;
	}
	if ((b3 & 0xc0U) == 0x80U) {
	    ch = (X11WasmCodepoint)(((b0 & 0x07U) << 18)
		    | ((b1 & 0x3fU) << 12)
		    | ((b2 & 0x3fU) << 6)
		    | (b3 & 0x3fU));
	    if ((ch >= 0x10000U) && (ch <= 0x10ffffU)) {
		length = 4;
	    } else {
		ch = 0xfffdU;
	    }
	}
    }
    if (chPtr != NULL) {
	*chPtr = ch;
    }
    return length;
}
