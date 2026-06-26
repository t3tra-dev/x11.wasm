/*
 * Xresource.h --
 *
 *	Minimal X resource manager declarations for x11.wasm.
 */

#ifndef _X11_XRESOURCE_H_
#define _X11_XRESOURCE_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef X11WASM_XRMDATABASE_DEFINED
#define X11WASM_XRMDATABASE_DEFINED
typedef struct _XrmHashBucketRec *XrmDatabase;
typedef XrmQuark XrmRepresentation;
typedef struct {
    unsigned int size;
    XPointer addr;
} XrmValue, *XrmValuePtr;
#endif

extern XrmDatabase XrmGetStringDatabase(
    _Xconst char *data);
extern XrmDatabase XrmGetDatabase(
    Display *display);
extern void XrmSetDatabase(
    Display *display,
    XrmDatabase database);
extern void XrmPutResource(
    XrmDatabase *database,
    _Xconst char *specifier,
    _Xconst char *type,
    XrmValue *value);
extern void XrmPutStringResource(
    XrmDatabase *database,
    _Xconst char *specifier,
    _Xconst char *value);
extern void XrmPutLineResource(
    XrmDatabase *database,
    _Xconst char *line);
extern Bool XrmGetResource(
    XrmDatabase database,
    _Xconst char *resource_name,
    _Xconst char *resource_class,
    char **resource_type_return,
    XrmValue *value_return);
extern void XrmDestroyDatabase(
    XrmDatabase database);
extern void XrmMergeDatabases(
    XrmDatabase source_database,
    XrmDatabase *target_database);
extern void XrmCombineDatabase(
    XrmDatabase source_database,
    XrmDatabase *target_database,
    Bool override);

#ifdef __cplusplus
}
#endif

#endif /* _X11_XRESOURCE_H_ */
