#ifndef	NOIDENT
#ident	"@(#)flat:FlatP.h	1.27"
#endif

#ifndef _OL_FLATP_H
#define _OL_FLATP_H

/*
 ************************************************************************	
 * Description:
 *	This is the flat container's private header file.
 ************************************************************************	
 */

#include <Xol/PrimitiveP.h>
#include <Xol/Flat.h>
#include <Xol/OlgP.h>

/*
 ************************************************************************	
 * Define structures and names/types needed to support flat widgets
 ************************************************************************	
 */

typedef void	(*OlFlatReqRscDestroyProc) OL_ARGS((
	Widget, 	/* w;		flattened widget id	*/
	Cardinal,	/* offset;	resource offset		*/
	String,		/* name;	require resource name	*/
	XtPointer	/* addr;	rsc to be destroyed	*/
));

typedef Boolean	(*OlFlatReqRscPredicateFunc) OL_ARGS((
	Widget, 	/* w;		flattened widget id	*/
	Cardinal,	/* offset;	resource offset		*/
	String		/* name		require resource name	*/
));

typedef struct {
	String				name;		/* Resource name*/
	OlFlatReqRscPredicateFunc	predicate;	/* Use it ?	*/
	OlFlatReqRscDestroyProc		destroy;	/* destroy proc	*/
	Cardinal			rsc_index;	/* private	*/
} OlFlatReqRsc, *OlFlatReqRscList;

			/* Define a structure that's used to extract
			 * information from the application's list and
			 * store indexes of required resources.		*/
typedef struct {
	Cardinal *	xlist;		/* resource extractor list	*/
	Cardinal	num_xlist;	/* number of xlist elements	*/
	Cardinal *	rlist;		/* required resource extractor list*/
	Cardinal	num_rlist;	/* number of rlist elements	*/
	char *		rdata;		/* place to hold data		*/
} OlFlatResourceInfo;

			/* Create a structure used for making geometry
			 * requests.					*/

typedef struct {
	XtGeometryMask	request_mode;	/* CWX, CWY, CWWidth, CWHeight,
					 * CWSibling, CWStackMode	*/
	Position	x;
	Position	y;
	Dimension	width;
	Dimension	height;
	Cardinal	sibling;
	int		stack_mode;
} OlFlatItemGeometry;

			/* Define a structure used to Draw Sub-objects	*/

typedef struct {
	Drawable	drawable;	/* place to draw		*/
	Screen *	screen;		/* drawable's screen		*/
	Pixel		background;	/* drawable's backgrnd		*/
	Pixmap		background_pixmap;/* drawable's backgrnd pixmap	*/
	Position	x;		/* item's X coordinate		*/
	Position	y;		/* item's Y coordinate		*/
	Dimension	width;		/* item's width			*/
	Dimension	height;		/* item's height		*/
} OlFlatDrawInfo;

				/* Define a structure to cache information
				 * on a per screen basis		*/

typedef struct _OlFlatScreenCache {
	Screen *		screen;		/* screen id		*/
	struct _OlFlatScreenCache * next;	/* next node in list	*/
	Cardinal		count;		/* reference count	*/
	Pixmap			stipple;	/* Stippled pixmap	*/
	GC			default_gc;	/* default GC		*/
	GC			scratch_gc;	/* scratch GC		*/
	OlgAttrs *		alt_attrs;	/* scratch attributes	*/
	Pixel			alt_bg;		/* scratch attrs bg	*/
	Pixel			alt_fg;		/* scratch attrs fg	*/
	Dimension		stroke_width;	/* brush stroke width	*/
	Dimension		abbrev_width;	/* points cvted to pixels*/
	Dimension		abbrev_height;	/* points cvted to pixels*/
	Dimension		menu_mark;	/* points cvted to pixels*/
	Dimension		oblong_radius;	/* points cvted to pixels*/
	Dimension		check_width;	/* points cvted to pixels*/
	Dimension		check_height;	/* points cvted to pixels*/
} OlFlatScreenCache, *OlFlatScreenCacheList;

#define OL_JUST_LOOKING	1		/* OlFlatScreenManager flag	*/
#define OL_ADD_REF	2		/* OlFlatScreenManager flag	*/
#define OL_DELETE_REF	3		/* OlFlatScreenManager flag	*/

/*
 ************************************************************************	
 * Define Expanded Sub-object Instance Structure
 ************************************************************************	
 */

typedef struct {
					/* Put public fields first	*/

	Cardinal	item_index;	/* the index of this item	*/
	Position	x;		/* item's X coordinate		*/
	Position	y;		/* item's Y coordinate		*/
	Dimension	width;		/* item's width			*/
	Dimension	height;		/* item's height		*/
	XtPointer	user_data;	/* application data hook	*/

					/* Put private fields next	*/

	Boolean		managed;	/* is this item managed ?	*/
	Boolean		mapped_when_managed; /* mapped when managed ?	*/
	Boolean		sensitive;	/* is this item sensitive ?	*/
	Boolean		traversal_on;	/* is this item traversable ?	*/
	Boolean		label_tile;	/* tile the label ?		*/
	OlDefine	label_justify;	/* justification of the labels	*/
	OlMnemonic	mnemonic;	/* mnemonic			*/
	Pixmap		background_pixmap; /* Background pixmap		*/
	Pixel		background_pixel;/* Background color		*/
	Pixel		foreground;	/* foreground			*/
	Pixel		font_color;	/* font color			*/
	Pixel		input_focus_color;
	String		accelerator;	/* accelerator binding		*/
	String		accelerator_text; /* accelerator display string	*/
	String		label;		/* textual label		*/
	XImage *	label_image;	/* Image label			*/
	XFontStruct *	font;		/* font struct			*/
} FlatItemPart;

			/* Item's Full Instance record declaration	*/

typedef struct {
	FlatItemPart	flat;
} FlatItemRec, *FlatItem;

/*
 ************************************************************************	
 * Define Widget Instance Structure
 ************************************************************************	
 */
			/* Define new fields for the instance part	*/

typedef struct _FlatPart {
	XtPointer	items;		/* sub-object list		*/
	String *	item_fields;	/* array of fields in item list	*/
	Cardinal	num_item_fields;/* Number of fields per item	*/
	Cardinal	num_items;	/* number of sub-object items	*/
	Cardinal	focus_item;	/* item with focus		*/
	Cardinal	last_focus_item;/* last item to have focus	*/
	OlFlatResourceInfo resource_info; /* resource information	*/
	Boolean		items_touched;	/* application touched items	*/
	Boolean		relayout_hint;	/* force a relayout		*/
	GC		label_gc;
	GC		inverse_gc;
	OlgAttrs *	pAttrs;
} FlatPart;

			/* Full instance record declaration:
			 * 1. declare Widget "Part" Fields and then
			 * 2. declare Full flat Item Record		*/

typedef struct _FlatRec {
    CorePart		core;
    PrimitivePart	primitive;
    FlatPart		flat;
    FlatItemRec		default_item;
} FlatRec;

			/* Define new function pointers for the flat
			 * widget class.				*/

typedef void	(*OlFlatAnalyzeItemsProc)OL_ARGS((
	Widget,		/* w;		container widget id		*/
	ArgList,	/* args;	args that created items		*/
	Cardinal *	/* num_args;	number of args			*/
));

typedef void	(*OlFlatChangeManagedProc) OL_ARGS((
	Widget,		/* w;		container widget id		*/
	FlatItem *,	/* items;	items which changed state	*/
	Cardinal	/* num_changed;	num items which changed state	*/
));

typedef void	(*OlFlatDrawItemProc) OL_ARGS((
	Widget,		/* w;		container widget id		*/
	FlatItem,	/* item;	expanded item			*/
	OlFlatDrawInfo * /* draw_info;	Information used in drawing	*/
));

typedef XtGeometryResult (*OlFlatGeometryHandlerFunc) OL_ARGS((
	Widget,			/* w;		Flat widget type	*/
	FlatItem,		/* item;	item making request	*/
	OlFlatItemGeometry *,	/* request;	the request		*/
	OlFlatItemGeometry *	/* reply;	the reply		*/
));

typedef void (*OlFlatGetItemGeometryProc) OL_ARGS((
	Widget,		/* w;		Flat widget type		*/
	Cardinal,	/* item_index;	Item to get drawing info. for	*/
	Position *,	/* x_ret;	returned item's x location	*/
	Position *,	/* y_ret;	returned item's y location	*/
	Dimension *,	/* width_ret;	returned item's width		*/
	Dimension *	/* height_ret;	returned item's height		*/
));

typedef Cardinal (*OlFlatGetIndexFunc) OL_ARGS((
	Widget,		/* w;			Flat widget type	*/
	Position,	/* x;			X source location	*/
	Position,	/* y;			Y source location	*/
	Boolean		/* ignore_sensitivity;	don't look at sensitivity*/
));

typedef Boolean	(*OlFlatItemAcceptFocusFunc) OL_ARGS((
	Widget,		/* w		container widget id		*/
	FlatItem,	/* item;	item to accept focus		*/
	Time *		/* time		time when request was made	*/
));

typedef Boolean	(*OlFlatItemActivateFunc) OL_ARGS((
	Widget,		/* w;		container widget id		*/
	FlatItem,	/* item;	item to activate		*/
	OlVirtualName,	/* type;	activation type			*/
	XtPointer	/* data;	activation data			*/
));

typedef void	(*OlFlatItemDestroyProc) OL_ARGS((
	Widget,		/* w;		Flat widget container id	*/
	FlatItem	/* item;	expanded item			*/
));
	
typedef void	(*OlFlatItemDimensionsProc) OL_ARGS((
	Widget,		/* w;		Flat widget container id	*/
	FlatItem,	/* item;	expanded item			*/
	Dimension *,	/* width;	returned width			*/
	Dimension *	/* height;	returned height			*/
));

typedef void	(*OlFlatItemGetValuesProc) OL_ARGS((
	Widget,		/* widget;	flat widget container		*/
	FlatItem,	/* item;	expanded item			*/
	ArgList,	/* args;	item Args			*/
	Cardinal *	/* num_args;	num item Args			*/
));

typedef void	(*OlFlatItemHighlightProc) OL_ARGS((
	Widget,		/* w;		flat widget id			*/
	FlatItem,	/* item;	item being affected		*/
	OlDefine	/* type;	highlight type			*/
));

typedef void	(*OlFlatItemInitializeProc) OL_ARGS((
	Widget,		/* w;		Flat widget container id	*/
	FlatItem,	/* request;	expanded requested item		*/
	FlatItem,	/* new;		expanded new item		*/
	ArgList,	/* args;	args that created items		*/
	Cardinal *	/* num_args;	number of args			*/
));

typedef Boolean	(*OlFlatItemLocationCursorDimensionsFunc) OL_ARGS((
	Widget,		/* w;		Flat widget container		*/
	FlatItem,	/* item;	expanded item			*/
	OlFlatDrawInfo * /* draw_info;	Information used in drawing	*/
));

typedef Boolean	(*OlFlatItemSetValuesFunc) OL_ARGS((
	Widget,		/* w;		flat widget container		*/
	FlatItem,	/* current;	item application has		*/
	FlatItem,	/* request;	item application wants		*/
	FlatItem,	/* new;		item application gets		*/
	ArgList,	/* args;	item Args			*/
	Cardinal *	/* num_args;	num item Args			*/
));

typedef void	(*OlFlatItemSetValuesAlmostProc) OL_ARGS((
	Widget,			/* w;		flat widget container	*/
	FlatItem,		/* current;	item application has	*/
	FlatItem,		/* new;		item application wants	*/
	OlFlatItemGeometry *,	/* request;	the request		*/
	OlFlatItemGeometry *	/* reply;	the compromise		*/
));

typedef void	(*OlFlatItemResizeProc) OL_ARGS((
	Widget,		/* w;		Flat widget container id	*/
	FlatItem	/* item;	Item that's been resized	*/
));

typedef void	(*OlFlatRefreshItemProc) OL_ARGS((
	Widget,		/* w;		Flat widget container id	*/
	FlatItem,	/* item;	to be redisplayed		*/
	Boolean		/* clear_first;	should the item be cleared ?	*/
));

typedef Cardinal	(*OlFlatTraverseItemsFunc) OL_ARGS((
	Widget,		/* w;		flat widget container		*/
	Cardinal,	/* item_index;	item index to start at		*/
	OlVirtualName,	/* direction;	direction to move		*/
	Time		/* time;	the time			*/
));

			/* Define inheritance procedures for the class	*/

#define XtInheritFlatDrawItem		((OlFlatDrawItemProc) _XtInherit)
#define XtInheritFlatGetItemGeometry	((OlFlatGetItemGeometryProc)_XtInherit)
#define XtInheritFlatGetIndex		((OlFlatGetIndexFunc) _XtInherit)
#define XtInheritFlatGeometryHandler	((OlFlatGeometryHandlerFunc) _XtInherit)
#define XtInheritFlatChangeManaged	((OlFlatChangeManagedProc) _XtInherit)
#define XtInheritFlatItemAcceptFocus	((OlFlatItemAcceptFocusFunc) _XtInherit)
#define XtInheritFlatItemActivate	((OlFlatItemActivateFunc) _XtInherit)
#define XtInheritFlatItemHighlight	((OlFlatItemHighlightProc) _XtInherit)
#define XtInheritFlatItemDimensions	((OlFlatItemDimensionsProc) _XtInherit)
#define XtInheritFlatTraverseItems	((OlFlatTraverseItemsFunc) _XtInherit)
#define XtInheritFlatItemSetValuesAlmost \
				((OlFlatItemSetValuesAlmostProc) _XtInherit)
#define XtInheritFlatItemResize		((OlFlatItemResizeProc) _XtInherit)
#define XtInheritFlatRefreshItem	((OlFlatRefreshItemProc) _XtInherit)
#define XtInheritFlatItemLocationCursorDimensions \
		((OlFlatItemLocationCursorDimensionsFunc) _XtInherit)


/*
 ************************************************************************	
 * Declare external routines and macros used by the flat widgets.
 * External routines exist for chained class procedures only.
 * Macros exist for non-chained class procedures, they provide a
 * way of checking the number of parameters when using a pointers
 * to functions.
 ************************************************************************	
 */

#define OL_FLATCLASS(w)	(((FlatWidgetClass)XtClass(w))->flat_class)

		/* A convenient macro to allocate a temporary
		 * expanded item if the stack space is too small.
		 * This routine must be used to insure a flatItem in a
		 * routine has enough memory to support future subclasses.
		 * This macro is a declaration macro and therefore should
		 * be placed among the declarations in a function body.
		 * Since this macro may allocate memory,
		 * OL_FLAT_FREE_ITEM should be used whenever exiting
		 * the scope of the OL_FLAT_ALLOC_ITEM() macro's usage.
		 *
		 * Example use:
		 *	{
		 *		int	foo;
		 * 		OL_FLAT_ALLOC_ITEM(w, FlatItem, my_item);
		 *		int	foobar;
		 *			....code....
		 *		OL_FLAT_FREE_ITEM(my_item);
		 *	}
		 */

#ifdef __STDC__
#define OL_TOKCONCAT(a,b)	a ## b
#else
#define OL_TOKCONCAT(a,b)	a/**/b
#endif

#define OL_FLAT_ALLOC_ITEM(w, type, i)					\
	auto char	OL_TOKCONCAT(i,OnStack)[256];			\
	auto type	i = (type)					\
	(OL_FLATCLASS(w).rec_size > sizeof(OL_TOKCONCAT(i,OnStack)) ?	\
	XtMalloc(OL_FLATCLASS(w).rec_size) : OL_TOKCONCAT(i,OnStack))

#define OL_FLAT_FREE_ITEM(i)					\
	if ((XtPointer)i != (XtPointer)OL_TOKCONCAT(i,OnStack)) 	\
		XtFree((XtPointer)i)

			/* Define macros to access class procedures
			 * and fields.  Some of these macros replace
			 * calls to public routines.
			 */

#define OlFlatDefaultItem(w) \
		((FlatItem)((char *)w + OL_FLATCLASS(w).default_offset))
#define OlFlatGetItemGeometry(w,i,x,y,wi,h) \
			(*OL_FLATCLASS(w).get_item_geometry)(w,i,x,y,wi,h)
#define OlFlatGetIndex(w,x,y,f)	(*OL_FLATCLASS(w).get_index)(w,x,y,f)
#define OlFlatItemDimensions(w,ei,iw,ih) \
				(*OL_FLATCLASS(w).item_dimensions)(w,ei,iw,ih)


			/* Declare extern routines
			 */

OLBeginFunctionPrototypeBlock

extern void
_OlDoGravity OL_ARGS((
	int,		/* gravity;	the desired gravity		*/
	Dimension,	/* c_width;	container's width		*/
	Dimension,	/* c_height;	container's height		*/
	Dimension,	/* width;	interior item width		*/
	Dimension,	/* height;	interior item height		*/
	Position *,	/* x;		returned x offset		*/
	Position *	/* y;		returned y offset		*/
));

extern Boolean
OlFlatItemActivate OL_ARGS((
	Widget,		/* w;		flat widge container		*/
	FlatItem,	/* item;	item to activate		*/
	OlVirtualName,	/* type;	type of activation		*/
	XtPointer	/* data;	arbitrary data			*/
));

extern void
OlFlatExpandItem OL_ARGS((
	Widget,		/* w;		flat widget container		*/
	Cardinal,	/* item_index;	item to be expanded		*/
	FlatItem	/* item;	memory to expand into		*/
));

extern void
OlFlatInheritAll OL_ARGS((
	WidgetClass	/* widget class wanting to inherit values	*/
));

extern XtGeometryResult
OlFlatMakeGeometryRequest OL_ARGS((
	Widget,			/* w;		the flat widget id	*/
	FlatItem,		/* item;	item making the request	*/
	OlFlatItemGeometry*,	/* request;	the request		*/
	OlFlatItemGeometry*	/* reply;	the reply		*/
));

extern void
OlFlatPreviewItem OL_ARGS((
	Widget,		/* w;		container widget id		*/
	Cardinal,	/* item_index;	Item to draw			*/
	Widget,		/* preview;	destination preview widget	*/
	Cardinal	/* preview_index; sub-object within preview widget*/
));

extern void
OlFlatRefreshExpandedItem OL_ARGS((
	Widget,		/* w;		container widget id		*/
	FlatItem,	/* item;	Item to draw			*/
	Boolean		/* clear_area;	should the area be cleared?	*/
));

extern OlFlatScreenCache *
OlFlatScreenManager OL_ARGS((
	Widget,		/* w;		any widget id			*/
	Cardinal,	/* point_size;	point size of information	*/
	OlDefine	/* flag;	OlFlatScreenManager flag	*/
));

extern void
OlFlatSyncItem OL_ARGS((
	Widget,		/* w;		flat widget container           */
	FlatItem	/* item;	sync with this item's fields	*/
));

extern void
OlFlatSetupLabelSize OL_ARGS((
	Widget,		/* w;		Widget making request		*/
	FlatItem,	/* item_rec;	expanded item			*/
	XtPointer *,	/* ppLbl;	pointer to label structure	*/
	void (**)()	/* ppSizeProc;	pointer to size procedure	*/
));

extern void
OlFlatSetupAttributes OL_ARGS((
	Widget,		/* w;		flat widget container		*/
	FlatItem,	/* item_rec;	expanded item			*/
	OlFlatDrawInfo *, /* di;	drawing information		*/
	OlgAttrs **,	/* ppAttrs;	returned pointer to attributes	*/
	XtPointer *,	/* ppLbl;	returned pointer to label	*/
	OlgLabelProc *	/* ppDrawProc;	returned ptr to lbl drawer	*/
));

/*
 ************************************************************************	
 * Define some routines that are external, but should not be used by
 * widget programmers
 ************************************************************************	
 */

extern void
_OlFlatAddConverters OL_NO_ARGS();

extern void
_OlFlatStateDestroy OL_ARGS((
	Widget
));

extern void
_OlFlatStateInitialize OL_ARGS((
	Widget,		/* request;	request flat widget		*/
	Widget,		/* new;		new flat widget			*/
	ArgList,	/* args;	args that created items		*/
	Cardinal *	/* num_args;	number of args			*/
));

extern Boolean
_OlFlatStateSetValues OL_ARGS((
	Widget,		/* current;	current flat widget or NULL	*/
	Widget,		/* request;	request flat widget		*/
	Widget,		/* new;		new flat widget			*/
	ArgList,	/* args;	args that created items		*/
	Cardinal *	/* num_args;	number of args			*/
));

OLEndFunctionPrototypeBlock

/*
 ************************************************************************	
 * Define Widget Class Part and Class Rec
 ************************************************************************	
 */

				/* Define new fields for the class part	*/

typedef struct _FlatClassPart {

		/* List class fields that cannot be inherited
		 */

    XtPointer			extension;	/* future extensions	*/
    Boolean			transparent_bg;	/* inherit parent's bg ?*/
    Cardinal			default_offset;	/* full default item	*/
    Cardinal			rec_size;	/* Full item record size*/
    XtResourceList		item_resources;	/* sub-object resources	*/
    Cardinal			num_item_resources;/* number of item rsc*/
    OlFlatReqRscList		required_resources; /* manditory item rscs*/
    Cardinal			num_required_resources;
    XrmQuarkList		quarked_items;	/* quarked names	*/

		/* All non-chained procedures beyond this point can be
		 * explicitly inherited from a subclass's ClassInitialize
		 * procedure by calling OlFlatInheritAll(wc)
		 */

	/*
	 * Container (i.e., layout) related procedures
	 */

    XtInitProc			initialize;	/* widget initialize	*/
    XtSetValuesFunc		set_values;	/* widget set_values	*/
    OlFlatGeometryHandlerFunc	geometry_handler;/* GeometryHandler	*/
    OlFlatChangeManagedProc	change_managed;	/* changed Managed	*/
    OlFlatGetItemGeometryProc	get_item_geometry;/* return item's geom.*/
    OlFlatGetIndexFunc		get_index;	/* gets an item's index	*/
    OlFlatTraverseItemsFunc	traverse_items;	/* traverses items	*/
    OlFlatRefreshItemProc	refresh_item;	/* refreshes an item	*/

	/*
	 * Item-specific procedures
	 */

    OlFlatItemInitializeProc	default_initialize;/* def. item init	*/
    OlFlatItemSetValuesFunc	default_set_values;/* def. item set val	*/
    OlFlatAnalyzeItemsProc	analyze_items;	/* check all items	*/
    OlFlatDrawItemProc		draw_item;	/* subclass's draw proc.*/
    OlFlatItemAcceptFocusFunc	item_accept_focus; /* sets focus to items*/
    OlFlatItemActivateFunc	item_activate;	/* activates an item	*/
    OlFlatItemDimensionsProc	item_dimensions;/* item's width/height	*/
    OlFlatItemGetValuesProc	item_get_values;/* queries an item	*/
    OlFlatItemHighlightProc	item_highlight;	/* highlights an item	*/
    OlFlatItemInitializeProc	item_initialize;/* checking a new item	*/
    OlFlatItemSetValuesFunc	item_set_values;/* updates an item	*/
    OlFlatItemSetValuesAlmostProc item_set_values_almost;/* geometry	*/
    OlFlatItemResizeProc	item_resize;
    OlFlatItemLocationCursorDimensionsFunc item_location_cursor_dimensions;
} FlatClassPart;

				/* Full class record declaration 	*/

typedef struct _FlatClassRec {
    CoreClassPart	core_class;
    PrimitiveClassPart	primitive_class;
    FlatClassPart	flat_class;
} FlatClassRec;

				/* External class record declaration	*/

extern FlatClassRec		flatClassRec;

#endif /* _OL_FLATP_H */
