#ifndef	NOIDENT
#ident	"@(#)control:ControlArP.h	1.18"
#endif

/**
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **/

#ifndef _ControlP_h
#define _ControlP_h

#include	<Xol/ManagerP.h>	/* include superclasses' header */
#include	<Xol/ControlAre.h>
#include	<Xol/ChangeBar.h>

/*  Internally used layout structure  */

typedef struct _OlCLayoutList
{
   Widget	widget;
   Dimension	border;
   Position	x, y;
   Dimension	width, height;
} OlCLayoutList;

typedef void	(*OlCtlDspLayoutList) OL_ARGS((
			Widget, OlCLayoutList **,
			int, Dimension *, Dimension *));

#define XtInheritDisplayLayoutList ((OlCtlDspLayoutList) _XtInherit)

/*  New fields for the Control Area widget class record  */

typedef struct
{
   OlCtlDspLayoutList	display_layout_list;	/* display a list of widgets */
} ControlClassPart;


/* Full class record declaration */

typedef struct _ControlClassRec
{
   CoreClassPart	core_class;
   CompositeClassPart	composite_class;
   ConstraintClassPart	constraint_class;
   ManagerClassPart	manager_class;
   ControlClassPart	control_class;
} ControlClassRec;

extern ControlClassRec controlClassRec;

/* New fields for the Control Panel widget record */

typedef struct
{
   Dimension		h_space, v_space;
   Dimension		h_pad, v_pad;
   int			measure;
   OlDefine		layouttype;
   OlDefine		same_size;
   Boolean		align_captions;
   Boolean		center;
   XtCallbackList	post_select;
   Boolean		allow_change_bars;
   Cardinal		ncolumns;
   Position *		col_x;
   ChangeBar *		cb;
} ControlPart;


/*
** Full instance record declaration
*/

typedef struct _ControlRec
{
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    ControlPart		control;
} ControlRec;

/*
 * Constraint record:
 */
typedef struct	_ControlAreaConstraintRec {
	/*
	 * Public:
	 */
	OlDefine		change_bar;

	/*
	 * Private:
	 */
	Position		col;
}			ControlAreaConstraintRec;

#endif
