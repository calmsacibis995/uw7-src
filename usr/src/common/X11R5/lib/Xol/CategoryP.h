#ifndef	NOIDENT
#ident	"@(#)category:CategoryP.h	2.2"
#endif

#if	!defined(_CATEGORYP_H)
#define _CATEGORYP_H

#include "Xol/ManagerP.h"
#include "Xol/Category.h"
#include "Xol/LayoutExtP.h"
#include "Xol/Olg.h"
#include "Xol/ChangeBar.h"

/*
 * Space (in points) between
 *
 *	- top of CATEGORY label, AbbrevMenuButton widget, page label
 *	- CATEGORY and AbbrevMenuButton widget
 *	- AbbrevMenuButton widget and page label
 *	- bottom of CATEGORY label, AbbrevMenuButton widget, page label
 *	- left edge and change bar
 */
#define CATEGORY_TOP_MARGIN		10
#define CATEGORY_SPACE1			10
#define CATEGORY_SPACE2			10
#define CATEGORY_BOTTOM_MARGIN		10
#define CATEGORY_CHANGE_BAR_SPACE	4

/*
 * Class record:
 */

typedef struct _CategoryClassPart {
	/*
	 * Public:
	 */
	XtPointer		extension;

	/*
	 * Private:
	 */
}			CategoryClassPart;

typedef struct _CategoryClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	CategoryClassPart	category_class;
}			CategoryClassRec;

extern CategoryClassRec categoryClassRec;

#define CATEGORY_C(WC) ((CategoryWidgetClass)(WC))->category_class

/*
 * Instance structure:
 */

typedef struct MenuItem {
	XtArgVal		label;
	XtArgVal		set;
	XtArgVal		user_data;
}			MenuItem;

typedef struct _CategoryPart {
	/*
	 * Public:
	 */
	OlLayoutResources	layout;
	struct {
		Dimension		width;
		Dimension		height;
	}			page;
	String			category_label;
	XFontStruct *		category_font;
	XFontStruct *		font;
	OlFontList *		font_list;
	Pixel			font_color;
	Boolean			show_footer;
	String			left_foot;
	String			right_foot;
	XtCallbackList		new_page;

	/*
	 * Private:
	 */
	Widget			page_choice;
	Widget			next_page;
	Widget			delayed_set_page_child;
	MenuItem *		page_list;
	Cardinal		first_page_child;
	Cardinal		page_list_size;
	Cardinal		current_page;
	GC			category_font_GC;
	GC			font_GC;
	ChangeBar *		cb;
	XtOrderProc		insert_position;
	unsigned char		flags;
	unsigned char		dynamics;
}			CategoryPart;

#define _CATEGORY_INTERNAL_CHILD		0x01

#define _CATEGORY_B_DYNAMIC_FONTCOLOR		0x01
#define _CATEGORY_B_DYNAMIC_FONT		0x02
#define _CATEGORY_B_DYNAMIC_CATEGORY_FONT	0x04

typedef struct _CategoryRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	ManagerPart		manager;
	CategoryPart		category;
}			CategoryRec;

#define CATEGORY_P(W) ((CategoryWidget)(W))->category

/*
 * Constraint record:
 */

typedef struct	_CategoryConstraintPart {
	/*
	 * Public:
	 */
	String			page_label;
	int			gravity;
	Boolean			_default;
	Boolean			changed;
	Boolean			query_child;

	/*
	 * Private:
	 */
#if	defined(AVAILABLE_WHEN_UNMANAGED)
	Boolean			available_when_unmanaged; /* obsolete */
#endif
	Boolean			window_gravity_set;
}			CategoryConstraintPart;

typedef struct _CategoryConstraintRec {
	CategoryConstraintPart	category;
}			CategoryConstraintRec;

#define CATEGORY_CP(W) ((CategoryConstraintRec *)(W)->core.constraints)->category

#endif
