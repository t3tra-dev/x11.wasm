/*
 * xresource.c --
 *
 *	Compile-only check that <X11/Xresource.h> exports the minimal Xrm ABI.
 */

#include <X11/Xresource.h>

int
X11WasmHeaderXresourcePrototypeTest(void)
{
    XrmDatabase (*getStringDatabase)(_Xconst char *) = XrmGetStringDatabase;
    XrmDatabase (*getDatabase)(Display *) = XrmGetDatabase;
    void (*setDatabase)(Display *, XrmDatabase) = XrmSetDatabase;
    Bool (*getResource)(XrmDatabase, _Xconst char *, _Xconst char *,
	    char **, XrmValue *) = XrmGetResource;
    void (*destroyDatabase)(XrmDatabase) = XrmDestroyDatabase;
    XrmDatabase database = NULL;
    XrmValue value;
    char *resourceType = NULL;

    value.size = 0;
    value.addr = NULL;
    return (getStringDatabase != 0) && (getResource != 0)
	    && (getDatabase != 0) && (setDatabase != 0)
	    && (destroyDatabase != 0) && (database == NULL)
	    && (resourceType == NULL) && (value.addr == NULL);
}
