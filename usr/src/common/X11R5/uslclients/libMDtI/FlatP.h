#ifndef EXM_FLATP_H
#define EXM_FLATP_H

#ifndef	NOIDENT
#pragma	ident	"@(#)libMDtI:FlatP.h	1.14"
#endif

/************************************************************************
 * Description:
 *	This is the flat container's private header file.
 */

#include <Xm/PrimitiveP.h>	/* include superclass header */
#include <Xm/DragCP.h>
#include "Flat.h"		/* include public header */

/************************************************************************
 * Define structures and names/types needed to support flat widgets
 */
#ifndef MOOLIT
#define OlVaDisplayWarningMsg(x) printf("File=%s, Line=%d\n", __FILE__,__LINE__)
#define OlVaDisplayErrorMsg(x)	 printf("File=%s, Line=%d\n", __FILE__,__LINE__)
#endif

#define XmRFlatItemFields	(_XmConst char *)"FlatItemFields"
#define XmRFlatItems		(_XmConst char *)"FlatItems"

#define ExmIGNORE		( ~0 )
	/* Hint for change_managed() and will be in `relayout_hint',
	 * indicating that, the caller of change_managed() is initialize() */
#define ExmFLAT_INIT		( 1L << 1 )

	/* possible bit value(s) for the `mask' field in ExmFlatClassPart */
		/* Flat.c:ItemSetValues() should return False when 
		 * XmNset is different, the actions include RAISE, and
		 * the ExmFLAT_HANDLE_RAISE bit is set in a subclass.
		 * The subclass is responsible for either returning
		 * True in its ItemSetValues() or calling
		 * ExmFlatRaiseExpandedItem(). This will remove one
		 * redundant drawing... */
#define ExmFLAT_HANDLE_RAISE	( 1 << 0 )

	/* Define slider units in x/y directions, use pixel based for now */
#define ExmFLAT_X_UOM(w)	FPART(w).col_width
#define ExmFLAT_Y_UOM(w)	FPART(w).row_height

#define CPART(w)	( (w)->core )
#define PPART(w)	( ((XmPrimitiveWidget)(w))->primitive )
#define FPART(w)	( ((ExmFlatWidget)(w))->flat )
#define FCLASS(w)	( ((ExmFlatWidgetClass)XtClass(w))->flat_class )
#define FITEM(i)	( (i)->flat )

#define InSWin(w)	( FPART(w).vsb ) /* in SWin? (after initialization) */

	/* Define a structure that's used to extract information from the
	 * application's list and store indexes of required
	 * resources.
	 */
typedef struct {
    Cardinal *	xlist;			/* resource extractor list	*/
    Cardinal	num_xlist;		/* number of xlist elements	*/
} ExmFlatResourceInfo;

	/* Create a structure used for making geometry requests.	*/

typedef struct {
    /* CWX, CWY, CWWidth, CWHeight, CWSibling, CWStackMode:	*/
    XtGeometryMask	request_mode;
    WidePosition	x;
    WidePosition	y;
    Dimension		width;
    Dimension		height;
    Cardinal		sibling;
    int			stack_mode;
} ExmFlatItemGeometry;

			/* Define a structure used to Draw Sub-objects	*/
typedef struct {
    Drawable		drawable;	/* place to draw */
    Screen *		screen;		/* drawable's screen */
    GC			gc;		/* drawing GC */
					/* "short" fields next: */
    WidePosition	x;		/* item's X coordinate */
    WidePosition	y;		/* item's Y coordinate */
    Dimension		width;		/* item's width */
    Dimension		height;		/* item's height */
					/* "byte" fields next: */
    Boolean		item_has_focus;	/* item has focus or not */
} ExmFlatDrawInfo;

/* Define directions for keyboard traversal.  These are passed to subclass'
 * TraverseItems to indicate the direction to traverse.
 *
 * NOTE: the characters are also used in the definition of the flat
 * translations.
 */
typedef enum {
    ExmK_MOVEUP		= 'u',
    ExmK_MULTIUP	= 'U',
    ExmK_MOVEDOWN	= 'd',
    ExmK_MULTIDOWN	= 'D',
    ExmK_MOVERIGHT	= 'r',
    ExmK_MULTIRIGHT	= 'R',
    ExmK_MOVELEFT	= 'l',
    ExmK_MULTILEFT	= 'L',
} ExmTraverseDirType;

/************************************************************************
 * Define Expanded Sub-object Instance Structure
 */

typedef struct {
    Cardinal	item_index;		/* the index of this item	*/
    XtPointer	user_data;		/* application data hook	*/
    _XmString	label;			/* textual label		*/
    WidePosition x;			/* item's X coordinate		*/
    WidePosition y;			/* item's Y coordinate		*/
					/* "short" fields go next:	*/
    Dimension	width;			/* item's width			*/
    Dimension	height;			/* item's height		*/
					/* "byte" fields go next:	*/
    Boolean	managed;		/* is this item managed ?	*/
    Boolean	mapped_when_managed;	/* mapped when managed ?	*/
    Boolean	sensitive;		/* is this item sensitive ?	*/
    Boolean	traversal_on;		/* is this item traversable ?	*/
    Boolean	selected;		/* is this item selected ?	*/
} ExmFlatItemPart;

			/* Item's Full Instance record declaration	*/
typedef struct {
	ExmFlatItemPart	flat;
} ExmFlatItemRec, *ExmFlatItem;

/************************************************************************
 * Define Widget Instance Structure
 */
			/* Define new fields for the instance part	*/

typedef struct _ExmFlatPart {
    XtPointer		items;		/* sub-object list		*/
    String *		item_fields;	/* array of fields in item list	*/
    Cardinal		num_item_fields;/* Number of fields per item	*/
    Cardinal		num_items;	/* number of sub-object items	*/
    ExmFlatResourceInfo resource_info;	/* resource information	*/
    Cardinal		focus_item;	/* item with focus		*/
    Cardinal		last_focus_item;/* last item to have focus	*/
    Cardinal		last_select;	/* last selected item index	*/
    Cardinal		select_count;	/* # of selected items		*/
    XtCallbackProc	select_proc;	/* single select callback	*/
    XtCallbackProc	dbl_select_proc;/* double select callback	*/
    XtCallbackProc	unselect_proc;	/* adjust button callback	*/
    XtPointer		client_data;	/* client data			*/
    XmFontList	 	font;		/* (for all items)		*/
    GC			normal_gc;	/* GC to draw normal item	*/
    GC			scroll_gc;	/* GC for scrollbar related	*/
    GC			select_gc;	/* GC to draw selected item	*/
    Pixmap		insens_pixmap;	/* Stipple for insensitive item */
					/* drag-and-drop related:	*/
    Atom *		targets;	/* XmN[export|import]Targets	*/
    Cardinal		num_targets;
    XmConvertSelectionRec convert_proc; /* required for dragging	*/
    XtCallbackProc	cursor_proc;	/* drag cursor routine		*/
    XtCallbackProc	dnd_done_proc;	/* DragDropFinishCallback	*/
    XtCallbackProc	drop_proc;	/* initiator's drop_proc	*/
					/* scrolled window related	*/
    Widget		vsb;		/* vertical scrollbar of swin	*/
    Widget		hsb;		/* horizontal scrollbar of swin	*/
    WidePosition	x_offset;	/* offset w.r.t real window	*/
    WidePosition	y_offset;
	/* Note that define min_x and min_y here, so that we can
	 * make use of negative part of WidePosition later... */
    WidePosition	min_x;		/* minimum value in x dir 	*/
    WidePosition	min_y;		/* minimum value in y dir 	*/
    WideDimension	actual_width;	/* actual width/height of the window */
    WideDimension	actual_height;	/* core.width/height has virtual size*/

					/* "short" fields next:		*/
    /* All subclasses seem to need rows and cols, row_height and col_width,
     * so include the storage for them here in the superclass.
     */
    Dimension		row_height;	/* also used for y slider unit	*/
    Dimension		col_width;	/* also used for x slider unit	*/
					/* "byte" fields next:		*/
    unsigned char	rows;		/* in row_height units		*/
    unsigned char	cols;		/* in col_width units		*/
    Boolean		items_touched;	/* application touched items	*/
    Boolean		relayout_hint;	/* force a relayout		*/
    Boolean		exclusives;	/* exclusives 			*/
    Boolean		none_set;	/* allow no items selected	*/
    Boolean		in_geom_hdr;	/* avoid mutilple calls to view */
					/* size calculation when True	*/
    Boolean		want_graphics_expose; /* want GraphicsExpose or not */
    unsigned char	drag_ops;	/* XmNdragOperations		*/
} ExmFlatPart;

			/* Full instance record declaration:
			 * 1. declare Widget "Part" Fields and then
			 * 2. declare Full flat Item Record
			 */
typedef struct _ExmFlatRec {
    CorePart		core;
    XmPrimitivePart	primitive;
    ExmFlatPart		flat;
    ExmFlatItemRec	default_item;
} ExmFlatRec;

	/* Define new function pointers for the flat widget class.	*/
typedef void	(*ExmFlatAnalyzeItemsProc)(
	Widget,		/* w;		container widget id		*/
	ArgList,	/* args;	args that created items		*/
	Cardinal *	/* num_args;	number of args			*/
);

typedef void	(*ExmFlatChangeManagedProc)(
	Widget,		/* w;		container widget id		*/
	ExmFlatItem *,	/* items;	items which changed state	*/
	Cardinal	/* num_changed;	num items which changed state	*/
);

	/* This definition should be in Flat.h because
	 * of FIconBox.c:XmNdrawProc, but doing this will force us
	 * to reveal ExmFlatItem and ExmFlatDrawInfo!
	 *
	 * SO the user of FIconBox.c will have to include FlatP.h for now, yuck!
	 */
typedef void	(*ExmFlatDrawItemProc)(
	Widget,		/* w;		container widget id		*/
	ExmFlatItem,	/* item;	expanded item			*/
	ExmFlatDrawInfo * /* draw_info;	Information used in drawing	*/
);

typedef XtGeometryResult (*ExmFlatGeometryHandlerFunc)(
	Widget,			/* w;		Flat widget type	*/
	ExmFlatItem,		/* item;	item making request	*/
	ExmFlatItemGeometry *,	/* request;	the request		*/
	ExmFlatItemGeometry *	/* reply;	the reply		*/
);

typedef void (*ExmFlatGetItemGeometryProc)(
	Widget,		/* w;		Flat widget type		*/
	Cardinal,	/* item_index;	Item to get drawing info. for	*/
	WidePosition *,	/* x_ret;	returned item's x location	*/
	WidePosition *,	/* y_ret;	returned item's y location	*/
	Dimension *,	/* width_ret;	returned item's width		*/
	Dimension *	/* height_ret;	returned item's height		*/
);

typedef Cardinal (*ExmFlatGetIndexFunc)(
	Widget,		/* w;			Flat widget type	*/
	WidePosition,	/* x;			X source location	*/
	WidePosition,	/* y;			Y source location	*/
	Boolean		/* ignore_sensitivity;	don't look at sensitivity*/
);

typedef Boolean	(*ExmFlatItemAcceptFocusFunc)(
	Widget,		/* w		container widget id		*/
	ExmFlatItem	/* item;	item to accept focus		*/
);

typedef void	(*ExmFlatItemDestroyProc)(
	Widget,		/* w;		Flat widget container id	*/
	ExmFlatItem	/* item;	expanded item			*/
);
	
typedef void	(*ExmFlatItemDimensionsProc)(
	Widget,		/* w;		Flat widget container id	*/
	ExmFlatItem,	/* item;	expanded item			*/
	Dimension *,	/* width;	returned width			*/
	Dimension *	/* height;	returned height			*/
);

typedef void	(*ExmFlatItemGetValuesProc)(
	Widget,		/* widget;	flat widget container		*/
	ExmFlatItem,	/* item;	expanded item			*/
	ArgList,	/* args;	item Args			*/
	Cardinal *	/* num_args;	num item Args			*/
);

typedef void	(*ExmFlatItemHighlightProc)(
	Widget,		/* w;		flat widget id			*/
	ExmFlatItem,	/* item;	item being affected		*/
	int		/* type;	highlight type			*/
);

typedef void	(*ExmFlatItemInitializeProc)(
	Widget,		/* w;		Flat widget container id	*/
	ExmFlatItem,	/* request;	expanded requested item		*/
	ExmFlatItem,	/* new;		expanded new item		*/
	ArgList,	/* args;	args that created items		*/
	Cardinal *	/* num_args;	number of args			*/
);

typedef Boolean	(*ExmFlatItemSetValuesFunc)(
	Widget,		/* w;		flat widget container		*/
	ExmFlatItem,	/* current;	item application has		*/
	ExmFlatItem,	/* request;	item application wants		*/
	ExmFlatItem,	/* new;		item application gets		*/
	ArgList,	/* args;	item Args			*/
	Cardinal *	/* num_args;	num item Args			*/
);

typedef void	(*ExmFlatItemSetValuesAlmostProc)(
	Widget,			/* w;		flat widget container	*/
	ExmFlatItem,		/* current;	item application has	*/
	ExmFlatItem,		/* new;		item application wants	*/
	ExmFlatItemGeometry *,	/* request;	the request		*/
	ExmFlatItemGeometry *	/* reply;	the compromise		*/
);

typedef void	(*ExmFlatRefreshItemProc)(
	Widget,		/* w;		Flat widget container id	*/
	ExmFlatItem,	/* item;	to be redisplayed		*/
	Boolean		/* clear_first;	should the item be cleared ?	*/
);

typedef Cardinal (*ExmFlatTraverseItemsFunc)(
	Widget,			/* w;		flat widget container	*/
	Cardinal,		/* item_index;	item index to start at	*/
	ExmTraverseDirType	/* direction;	direction to move	*/
);

			/* Define inheritance procedures for the class	*/

#define ExmInheritFlatDrawItem		((ExmFlatDrawItemProc)_XtInherit)
#define ExmInheritFlatGetItemGeometry	((ExmFlatGetItemGeometryProc)_XtInherit)
#define ExmInheritFlatGetIndex		((ExmFlatGetIndexFunc)_XtInherit)
#define ExmInheritFlatGeometryHandler	((ExmFlatGeometryHandlerFunc)_XtInherit)
#define ExmInheritFlatChangeManaged	((ExmFlatChangeManagedProc)_XtInherit)
#define ExmInheritFlatItemAcceptFocus	((ExmFlatItemAcceptFocusFunc)_XtInherit)
#define ExmInheritFlatItemHighlight	((ExmFlatItemHighlightProc)_XtInherit)
#define ExmInheritFlatItemDimensions	((ExmFlatItemDimensionsProc)_XtInherit)
#define ExmInheritFlatTraverseItems	((ExmFlatTraverseItemsFunc)_XtInherit)
#define ExmInheritFlatItemSetValuesAlmost \
				((ExmFlatItemSetValuesAlmostProc)_XtInherit)
#define ExmInheritFlatRefreshItem	((ExmFlatRefreshItemProc)_XtInherit)


/************************************************************************
 * Declare external routines and macros used by the flat widgets.
 * External routines exist for chained class procedures only.
 * Macros exist for non-chained class procedures, they provide a
 * way of checking the number of parameters when using a pointers
 * to functions.
 */
#define ExmFLATCLASS(w)	(((ExmFlatWidgetClass)XtClass(w))->flat_class)
	/* A convenient macro to allocate a temporary expanded item if the
	 * stack space is too small.  This routine must be used to insure a
	 * flatItem in a routine has enough memory to support future
	 * subclasses.  This macro is a declaration macro and therefore
	 * should be placed among the declarations in a function body.  Since
	 * this macro may allocate memory, OL_FLAT_FREE_ITEM should be used
	 * whenever exiting the scope of the OL_FLAT_ALLOC_ITEM() macro's
	 * usage.
	 *
	 * Example use:
	 *	{
	 *		int	foo;
	 * 		ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, my_item);
	 *		int	foobar;
	 *			....code....
	 *		ExmFLAT_FREE_ITEM(my_item);
	 *	}
	 */
#ifdef __STDC__
#define ExmTOKCONCAT(a,b)	a ## b
#else
#define ExmTOKCONCAT(a,b)	a/**/b
#endif

#define ExmFLAT_ALLOC_ITEM(w, type, i)					\
	auto char	ExmTOKCONCAT(i,OnStack)[256];			\
	auto type	i = (type)					\
	(FCLASS(w).rec_size > sizeof(ExmTOKCONCAT(i,OnStack)) ?	\
	XtMalloc(FCLASS(w).rec_size) : ExmTOKCONCAT(i,OnStack))

#define ExmFLAT_FREE_ITEM(i)					\
	if ((XtPointer)i != (XtPointer)ExmTOKCONCAT(i,OnStack)) 	\
		XtFree((XtPointer)i)

			/* Define macros to access class procedures
			 * and fields.  Some of these macros replace
			 * calls to public routines.
			 */

#define ExmFlatDefaultItem(w)						\
			((ExmFlatItem)((char *)w + FCLASS(w).default_offset))
#define ExmFlatGetItemGeometry(w,i,x,y,wi,h)				\
			(*FCLASS(w).get_item_geometry)(w,i,x,y,wi,h)
#define ExmFlatGetIndex(w,x,y,f)	(*FCLASS(w).get_index)(w,x,y,f)
#define ExmFlatItemDimensions(w,ei,iw,ih)				\
			(*FCLASS(w).item_dimensions)(w,ei,iw,ih)


			/* Declare extern routines			*/

ExmBeginFunctionPrototypeBlock

extern void
ExmFlatDrawExpandedItem(Widget,
		        ExmFlatItem
);

extern void
ExmFlatExpandItem(
	Widget,		/* w;		flat widget container		*/
	Cardinal,	/* item_index;	item to be expanded		*/
	ExmFlatItem	/* item;	memory to expand into		*/
);

extern void
ExmFlatInheritAll(
	WidgetClass	/* widget class wanting to inherit values	*/
);

extern XtGeometryResult
ExmFlatMakeGeometryRequest(
	Widget,			/* w;		the flat widget id	*/
	ExmFlatItem,		/* item;	item making the request	*/
	ExmFlatItemGeometry*,	/* request;	the request		*/
	ExmFlatItemGeometry*	/* reply;	the reply		*/
);

extern void
ExmFlatRefreshExpandedItem(
	Widget,		/* w;		container widget id		*/
	ExmFlatItem,	/* item;	Item to draw			*/
	Boolean		/* clear_area;	should the area be cleared?	*/
);

extern void
ExmFlatSetFocus(
	Widget,		/* w;		container widget id		*/
	Cardinal	/* item_index;  item will take the focus	*/
);

extern void
ExmFlatSyncItem(
	Widget,		/* w;		flat widget container           */
	ExmFlatItem	/* item;	sync with this item's fields	*/
);

/************************************************************************
 * Define some routines that are external, but should not be used by
 * widget programmers
 */

extern void
ExmFlatAddConverters(void);

extern void
ExmFlatStateDestroy(
	Widget
);

extern void
ExmFlatStateInitialize(
	Widget,		/* request;	request flat widget		*/
	Widget,		/* new;		new flat widget			*/
	ArgList,	/* args;	args that created items		*/
	Cardinal *	/* num_args;	number of args			*/
);

extern Boolean
ExmFlatStateSetValues(
	Widget,		/* current;	current flat widget or NULL	*/
	Widget,		/* request;	request flat widget		*/
	Widget,		/* new;		new flat widget			*/
	ArgList,	/* args;	args that created items		*/
	Cardinal *	/* num_args;	number of args			*/
);

ExmEndFunctionPrototypeBlock

/************************************************************************
 * Define Widget Class Part and Class Rec
 */

				/* Define new fields for the class part	*/

typedef struct _ExmFlatClassPart {

		/* List class fields that cannot be inherited		*/

    XtPointer			extension;	/* future extensions	*/
    Cardinal			default_offset;	/* full default item	*/
    Cardinal			rec_size;	/* Full item record size*/
    XtResourceList		item_resources;	/* sub-object resources	*/
    Cardinal			num_item_resources;/* number of item rsc*/
    Cardinal			mask;		/* reserve 16/32 bits for */
						/* various purposes, valid */
						/* bit: ExmFLAT_HANDLE_RAISE */
    XrmQuarkList		quarked_items;	/* quarked names	*/

	/* All non-chained procedures beyond this point can be explicitly
	 * inherited from a subclass's ClassInitialize procedure by calling
	 * ExmFlatInheritAll(wc)
	 */

	/*
	 * Container (i.e., layout) related procedures
	 */
    XtInitProc			initialize;	/* widget initialize	*/
    XtSetValuesFunc		set_values;	/* widget set_values	*/
    ExmFlatGeometryHandlerFunc	geometry_handler;/* GeometryHandler	*/
    ExmFlatChangeManagedProc	change_managed;	/* changed Managed	*/
    ExmFlatGetItemGeometryProc	get_item_geometry;/* return item's geom.*/
    ExmFlatGetIndexFunc		get_index;	/* gets an item's index	*/
    ExmFlatTraverseItemsFunc	traverse_items;	/* traverses items	*/
    ExmFlatRefreshItemProc	refresh_item;	/* refreshes an item	*/

	/*
	 * Item-specific procedures
	 */
    ExmFlatItemInitializeProc	default_initialize;/* def. item init	*/
    ExmFlatItemSetValuesFunc	default_set_values;/* def. item set val	*/
    ExmFlatAnalyzeItemsProc	analyze_items;	/* check all items	*/
    ExmFlatDrawItemProc		draw_item;	/* subclass's draw proc.*/
    ExmFlatItemAcceptFocusFunc	item_accept_focus; /* sets focus to items*/
    ExmFlatItemDimensionsProc	item_dimensions;/* item's width/height	*/
    ExmFlatItemGetValuesProc	item_get_values;/* queries an item	*/
    ExmFlatItemHighlightProc	item_highlight;	/* highlights an item	*/
    ExmFlatItemInitializeProc	item_initialize;/* checking a new item	*/
    ExmFlatItemSetValuesFunc	item_set_values;/* updates an item	*/
    ExmFlatItemSetValuesAlmostProc item_set_values_almost;/* geometry	*/
} ExmFlatClassPart;

				/* Full class record declaration 	*/

typedef struct _ExmFlatClassRec {
    CoreClassPart		core_class;
    XmPrimitiveClassPart	primitive_class;
    ExmFlatClassPart		flat_class;
} ExmFlatClassRec;

				/* External class record declaration	*/

extern ExmFlatClassRec		exmFlatClassRec;

#endif /* EXM_FLATP_H */
