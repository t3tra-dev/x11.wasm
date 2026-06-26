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
    void (*qPutResource)(XrmDatabase *, XrmBindingList, XrmQuarkList,
	    XrmRepresentation, XrmValue *) = XrmQPutResource;
    void (*putStringResource)(XrmDatabase *, _Xconst char *,
	    _Xconst char *) = XrmPutStringResource;
    void (*qPutStringResource)(XrmDatabase *, XrmBindingList, XrmQuarkList,
	    _Xconst char *) = XrmQPutStringResource;
    void (*putLineResource)(XrmDatabase *, _Xconst char *) =
	    XrmPutLineResource;
    Bool (*getResource)(XrmDatabase, _Xconst char *, _Xconst char *,
	    char **, XrmValue *) = XrmGetResource;
    Bool (*qGetResource)(XrmDatabase, XrmNameList, XrmClassList,
	    XrmRepresentation *, XrmValue *) = XrmQGetResource;
    void (*destroyDatabase)(XrmDatabase) = XrmDestroyDatabase;
    void (*mergeDatabases)(XrmDatabase, XrmDatabase *) = XrmMergeDatabases;
    void (*combineDatabase)(XrmDatabase, XrmDatabase *, Bool) =
	    XrmCombineDatabase;
    void (*stringToQuarkList)(_Xconst char *, XrmQuarkList) =
	    XrmStringToQuarkList;
    void (*stringToBindingQuarkList)(_Xconst char *, XrmBindingList,
	    XrmQuarkList) = XrmStringToBindingQuarkList;
    XrmDatabase database = NULL;
    XrmBinding bindings[2];
    XrmQuark quarks[2];
    XrmName name = XrmStringToName("name");
    XrmClass class = XrmStringToClass("Class");
    XrmRepresentation representation = XrmStringToRepresentation("String");
    XrmValue value;
    char *resourceType = NULL;

    value.size = 0;
    value.addr = NULL;
    return (getStringDatabase != 0) && (getResource != 0)
	    && (getDatabase != 0) && (setDatabase != 0)
	    && (putResource != 0) && (putStringResource != 0)
	    && (putLineResource != 0)
	    && (qPutResource != 0) && (qPutStringResource != 0)
	    && (qGetResource != 0)
	    && (stringToQuarkList != 0) && (stringToBindingQuarkList != 0)
	    && (destroyDatabase != 0) && (database == NULL)
	    && (mergeDatabases != 0) && (combineDatabase != 0)
	    && (XrmNameToString(name) != NULLSTRING)
	    && (XrmClassToString(class) != NULLSTRING)
	    && (XrmRepresentationToString(representation) != NULLSTRING)
	    && (bindings[0] = XrmBindTightly) == XrmBindTightly
	    && (quarks[0] = NULLQUARK) == NULLQUARK
	    && (resourceType == NULL) && (value.addr == NULL);
}
