#ifndef	NOIDENT
#ident	"@(#)stub:StubP.h	1.10"
#endif

#ifndef _StubP_h
#define _StubP_h

/*
 ************************************************************************
 *
 * Description:
 *		"Private" include file for the Stub Widget.
 *
 *****************************file*header********************************
 */

#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Stub.h>

/*
 ************************************************************************
 *
 * Define the Stub's Class Part and then the Class Record
 *
 ************************************************************************
 */

typedef struct _StubClass  {
    char no_class_fields;		/* Makes compiler happy */
} StubClassPart;

typedef struct _StubClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	StubClassPart		stub_class;
} StubClassRec;

		/* Declare the public hook to the Stub Class Record	*/
			
extern StubClassRec stubClassRec;

/*
 ************************************************************************
 *
 * Define the widget instance structure for the stub
 *
 ************************************************************************
 */

typedef struct {
    Window		window;			/* Initial window	*/
    Widget		reference_stub;		/* to inherit from	*/
    XtInitProc		initialize;		/* initialize instance	*/
    XtArgsProc		initialize_hook;	/* initialize subdata	*/
    XtRealizeProc	realize;		/* realize instance	*/
    XtWidgetProc	destroy;		/* destroy instance	*/
    XtWidgetProc	resize;			/* resize instance contents*/
    XtExposeProc	expose;			/* rediplay window	*/
    XtSetValuesFunc	set_values;		/* set instance resources*/
    XtArgsFunc		set_values_hook;	/* set subdata resources*/
    XtAlmostProc	set_values_almost;	/* set values almost geo.
						 * reply handler	*/
    XtArgsProc		get_values_hook;	/* get subdata resources*/
    XtGeometryHandler	query_geometry;		/* perferred geometry	*/

    XtAcceptFocusProc	accept_focus;		/* accept_focus 	*/
    OlActivateFunc	activate;		/* activate function    */
    OlHighlightProc	highlight;		/* highlight function   */
    OlRegisterFocusFunc	register_focus;		/* register_focus func. */
    OlTraversalFunc	traversal_handler;	/* traversal handler    */
} StubPart;

				/* Full Record Declaration		*/

typedef struct _StubRec {
	CorePart	core;
	PrimitivePart	primitive;
	StubPart	stub;
} StubRec;

#endif /* _StubP_h */
