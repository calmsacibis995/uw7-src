#ifndef	NOIDENT
#ident	"@(#)scrollinglist:ScrollingP.h	1.26"
#endif
/* 
 * ScrollingP.h - Private definitions for Scrolling List widget
 */

#ifndef _ScrollingP_h
#define _ScrollingP_h

#include <Xol/ScrollingL.h>	/* include my public header */
#include <Xol/FormP.h>		/* include superclass' header */

/***********************************************************************
 *
 *	Constants / Macros
 */

#define _OlListPane(w)	( ((CompositeWidget)(w))->composite.children[2] )
#define _OlListSBar(w)	( ((CompositeWidget)(w))->composite.children[0] )

/* dynamic resource bit masks */
#define OL_B_LIST_FG		(1 << 0)
#define OL_B_LIST_FONTCOLOR	(1 << 1)

/***********************************************************************
 *
 *	External Functions
 */


/***********************************************************************
 *
 *	Class structure
 */
/* New fields for the List widget class record */
typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} ListClassPart;

/* Full class record declaration */
typedef struct _ListClassRec {
    CoreClassPart	core_class;
    CompositeClassPart 	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
    FormClassPart	form_class;
    ListClassPart	list_class;
} ListClassRec;

/* Class record variable */
externalref ListClassRec listClassRec;
extern WidgetClass listWidgetClass;

/***********************************************************************
 *
 *	Instance (widget) structure
 */

/* New fields for the List widget record */
typedef struct { 
    XtCallbackList	userDeleteItems;
    XtCallbackList	userMakeCurrent;

    unsigned char	dyn_flags;
} ListPart;

/* Full instance record declaration */
typedef struct _ListRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    FormPart		form;
    ListPart		list;
} ListRec;

/* constraint record */
typedef struct {
    int	no_fields;
} ListConstraintPart;

typedef struct _ListConstraintRec {
    FormConstraintRec	form;
    ListConstraintPart	list;
} ListConstraintRec, *ListConstraint;

#endif /* _ScrollingP_h */
