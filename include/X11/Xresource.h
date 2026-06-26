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
typedef struct _XrmHashBucketRec *XrmHashBucket;
typedef XrmHashBucket *XrmHashTable;
typedef XrmHashTable XrmSearchList[];
typedef XrmQuark XrmRepresentation;
#define XrmStringToRepresentation(string) XrmStringToQuark(string)
#define XrmRepresentationToString(type) XrmQuarkToString(type)
typedef struct {
    unsigned int size;
    XPointer addr;
} XrmValue, *XrmValuePtr;
#endif

#define XrmStringsEqual(a1, a2) (strcmp((a1), (a2)) == 0)

typedef enum {
    XrmBindTightly,
    XrmBindLoosely
} XrmBinding, *XrmBindingList;

typedef XrmQuark XrmName;
typedef XrmQuarkList XrmNameList;
#define XrmNameToString(name) XrmQuarkToString(name)
#define XrmStringToName(string) XrmStringToQuark(string)
#define XrmStringToNameList(str, name) XrmStringToQuarkList((str), (name))

typedef XrmQuark XrmClass;
typedef XrmQuarkList XrmClassList;
#define XrmClassToString(c_class) XrmQuarkToString(c_class)
#define XrmStringToClass(c_class) XrmStringToQuark(c_class)
#define XrmStringToClassList(str, c_class) \
    XrmStringToQuarkList((str), (c_class))

typedef enum {
    XrmoptionNoArg,
    XrmoptionIsArg,
    XrmoptionStickyArg,
    XrmoptionSepArg,
    XrmoptionResArg,
    XrmoptionSkipArg,
    XrmoptionSkipLine,
    XrmoptionSkipNArgs
} XrmOptionKind;

typedef struct {
    char *option;
    char *specifier;
    XrmOptionKind argKind;
    XPointer value;
} XrmOptionDescRec, *XrmOptionDescList;

#define XrmEnumAllLevels 0
#define XrmEnumOneLevel 1

extern char *Xpermalloc(
    unsigned int size);
extern void XrmStringToQuarkList(
    _Xconst char *string,
    XrmQuarkList quarks_return);
extern void XrmStringToBindingQuarkList(
    _Xconst char *string,
    XrmBindingList bindings_return,
    XrmQuarkList quarks_return);
extern XrmDatabase XrmGetStringDatabase(
    _Xconst char *data);
extern XrmDatabase XrmGetFileDatabase(
    _Xconst char *filename);
extern Status XrmCombineFileDatabase(
    _Xconst char *filename,
    XrmDatabase *target_database,
    Bool override);
extern void XrmPutFileDatabase(
    XrmDatabase database,
    _Xconst char *filename);
extern const char *XrmLocaleOfDatabase(
    XrmDatabase database);
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
extern void XrmQPutResource(
    XrmDatabase *database,
    XrmBindingList bindings,
    XrmQuarkList quarks,
    XrmRepresentation type,
    XrmValue *value);
extern void XrmPutStringResource(
    XrmDatabase *database,
    _Xconst char *specifier,
    _Xconst char *value);
extern void XrmQPutStringResource(
    XrmDatabase *database,
    XrmBindingList bindings,
    XrmQuarkList quarks,
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
extern Bool XrmQGetResource(
    XrmDatabase database,
    XrmNameList quark_name,
    XrmClassList quark_class,
    XrmRepresentation *quark_type_return,
    XrmValue *value_return);
extern Bool XrmQGetSearchList(
    XrmDatabase database,
    XrmNameList names,
    XrmClassList classes,
    XrmSearchList list_return,
    int list_length);
extern Bool XrmQGetSearchResource(
    XrmSearchList list,
    XrmName name,
    XrmClass class,
    XrmRepresentation *type_return,
    XrmValue *value_return);
extern Bool XrmEnumerateDatabase(
    XrmDatabase database,
    XrmNameList name_prefix,
    XrmClassList class_prefix,
    int mode,
    Bool (*proc)(XrmDatabase *, XrmBindingList, XrmQuarkList,
	    XrmRepresentation *, XrmValue *, XPointer),
    XPointer closure);
extern void XrmDestroyDatabase(
    XrmDatabase database);
extern void XrmMergeDatabases(
    XrmDatabase source_database,
    XrmDatabase *target_database);
extern void XrmCombineDatabase(
    XrmDatabase source_database,
    XrmDatabase *target_database,
    Bool override);
extern void XrmParseCommand(
    XrmDatabase *database,
    XrmOptionDescList table,
    int table_count,
    _Xconst char *name,
    int *argc_in_out,
    char **argv_in_out);

#ifdef __cplusplus
}
#endif

#endif /* _X11_XRESOURCE_H_ */
