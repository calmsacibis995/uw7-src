#ifndef	NOIDENT
#ident	"@(#)olhelp:HelpP.h	1.8"
#endif

#ifndef _Ol_HelpP_h_
#define _Ol_HelpP_h_
/*
 ************************************************************************
 *
 * Description:
 *		This is the "private" include file for the Help Widget
 *
 *****************************file*header********************************
 */

#include <Xol/RubberTilP.h>
#include <Xol/Help.h>

/*
 ***********************************************************************
 *
 * Widget Private Data
 *
 ***********************************************************************
 */

			/* New fields for the widget class record	*/

typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} HelpClassPart;

				/* Full class record declaration 	*/

typedef struct _HelpClassRec {
  	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	PanesClassPart		panes_class;
	RubberTileClassPart	rubber_tile_class;
	HelpClassPart		help_class;
} HelpClassRec;

extern HelpClassRec helpClassRec;

/*
 ***********************************************************************
 *
 * Instance (widget) structure 
 *
 ***********************************************************************
 */

				/* New fields for the widget record	*/

typedef struct {
	Widget		text_widget;	/* Text Widget Id		*/
	Widget		mag_widget;	/* Magnifier Widget Id		*/
	Boolean		allow_root_help;/* Permit RootWindow Help	*/
} HelpPart;

					/* Full Widget declaration	*/
typedef struct _HelpRec {
	CorePart 	core;
	CompositePart 	composite;
	ConstraintPart	constraint;
	ManagerPart	manager;
	PanesPart	panes;
	RubberTilePart	rubber_tile;
	HelpPart	help;
} HelpRec;

/*
 * Constraint record:
 */

typedef struct {
	int			no_fields;
} HelpConstraintPart;

typedef struct _HelpConstraintRec {
	PanesConstraintRec	panes;
	RubberTileConstraintRec	rubber_tile;
	HelpConstraintPart	help;
} HelpConstraintRec, *HelpConstraint;

#endif /* _Ol_HelpP_h_ */
