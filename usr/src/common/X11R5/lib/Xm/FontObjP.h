#pragma ident	"@(#)m1.2libs:Xm/FontObjP.h	1.2"
/*** FontObjP.h ***/

#ifndef _FontObjP_h
#define _FontObjP_h

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <Xm/FontObj.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _FamilyFontLists {
    XmFontList		sans_serif;
    XmFontList		serif;
    XmFontList		mono;
} FamilyFontLists;


typedef struct _FontObjectPart {
    XmFontList		sans_serif;
    XmFontList		serif;
    XmFontList		mono;
    XtCallbackList	dynamic_font_callback;
    /* Private data */
    WidgetList		shell_list;
    Cardinal		shell_list_size;
    Cardinal		shell_list_alloc_size;
} FontObjectPart;

typedef struct _FontObjectRec {
    CorePart 		core;
    FontObjectPart	font;
} FontObjectRec;

typedef struct _FontObjectClassPart {
    XtInitProc		shell_initialize;
    XtWidgetProc	shell_destroy;
    XtPointer		extension;
} FontObjectClassPart;

/* 
 * we make it a appShell subclass so it can have it's own instance
 * hierarchy
 */
typedef struct _FontObjectClassRec{
    CoreClassPart      		core_class;
    FontObjectClassPart		font_class;
} FontObjectClassRec;

extern FontObjectClassRec _xmFontObjectClassRec;

/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern void _XmFontObjectCreate() ;

#else

extern void _XmFontObjectCreate( 
                        Widget w,
                        ArgList al,
                        Cardinal *acPtr) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/

/*
 * Extension version and name:
 */
#define DynamicFontListClassExtensionVersion	1
#define XmNDynamicFontListClassExtension	"DynamicFontListClassExtension"

typedef struct _DynamicFontListClassExtensionRec {
        /*
         * Common:
         */
        XtPointer                       next_extension;
        XrmQuark                        record_type;
        long                            version;
        Cardinal                        record_size;
        /*
         * DynamicFontList Extension, public:
         */

        /*
         * DynamicFontList Extension, private:
         */
	XtInitProc			initialize;
	XtSetValuesFunc			set_values;
	XtWidgetProc			destroy;
        XContext			fonts_used;
} DynamicFontListClassExtensionRec, *DynamicFontListClassExtension;

        /* Define a common structure used for all class extensions      */
typedef struct {
    XtPointer   next_extension; /* pointer to next in list              */
    XrmQuark    record_type;    /* NULLQUARK                            */
    long        version;        /* version particular to extension record*/
    Cardinal    record_size;    /* sizeof() particular extension Record */
} XmClassExtensionRec, *XmClassExtension;

#if defined(__cplusplus) || defined(c_plusplus)
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _FontObjP_h */
