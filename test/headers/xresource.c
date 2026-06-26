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
    void (*putResource)(XrmDatabase *, _Xconst char *, _Xconst char *,
	    XrmValue *) = XrmPutResource;
    void (*putStringResource)(XrmDatabase *, _Xconst char *,
	    _Xconst char *) = XrmPutStringResource;
    void (*putLineResource)(XrmDatabase *, _Xconst char *) =
	    XrmPutLineResource;
    Bool (*getResource)(XrmDatabase, _Xconst char *, _Xconst char *,
	    char **, XrmValue *) = XrmGetResource;
    void (*destroyDatabase)(XrmDatabase) = XrmDestroyDatabase;
    void (*mergeDatabases)(XrmDatabase, XrmDatabase *) = XrmMergeDatabases;
    void (*combineDatabase)(XrmDatabase, XrmDatabase *, Bool) =
	    XrmCombineDatabase;
    XrmDatabase database = NULL;
    XrmValue value;
    char *resourceType = NULL;

    value.size = 0;
    value.addr = NULL;
    return (getStringDatabase != 0) && (getResource != 0)
	    && (getDatabase != 0) && (setDatabase != 0)
	    && (putResource != 0) && (putStringResource != 0)
	    && (putLineResource != 0)
	    && (destroyDatabase != 0) && (database == NULL)
	    && (mergeDatabases != 0) && (combineDatabase != 0)
	    && (resourceType == NULL) && (value.addr == NULL);
}
