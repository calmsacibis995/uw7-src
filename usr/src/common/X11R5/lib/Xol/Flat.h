#ifndef	NOIDENT
#ident	"@(#)flat:Flat.h	1.22"
#endif

#ifndef _OL_FLAT_H
#define _OL_FLAT_H

/*
 ************************************************************************	
 * Description:
 *	This is the flat container meta class's public header file.
 ************************************************************************	
 */

#include <Xol/Primitive.h>
#include <DnD/OlDnDVCX.h>

/*
 ************************************************************************	
 * Define class and instance pointers:
 *	- extern pointer to class data/procedures
 *	- typedef pointer to widget's class structure
 *	- typedef pointer to widget's instance structure
 ************************************************************************	
 */

extern WidgetClass		flatWidgetClass;
typedef struct _FlatClassRec *	FlatWidgetClass;
typedef struct _FlatRec *	FlatWidget;

			/* Convenience macro to check for subclass of Flat */
#define _OlIsFlat(w)	XtIsSubclass(w, flatWidgetClass)

/*
 ************************************************************************	
 * Declare structures used with flattened widgets
 ************************************************************************	
 */

#define OL_DEFAULT_ITEM	((Cardinal) (~0)-1)

	/* Define flat widget call data.  This includes item data which is
	   passed to all call backs.
	*/
typedef struct {
	Cardinal	item_index;	/* sub-object initiating callb.	*/
	XtPointer	items;		/* sub-object list		*/
	Cardinal	num_items;	/* number of sub-objects	*/
	String *	item_fields;	/* key of fields for list	*/
	Cardinal	num_item_fields;/* number of item fields	*/
	XtPointer	user_data;	/* widget's user_data value	*/
	XtPointer	item_user_data;	/* item's user_data value	*/
} OlFlatCallData;

	/* Define Drop data: this is supplied to drop call backs by flat
	   subclasses which support Drag and Drop.
	 */
typedef struct {
	OlFlatCallData		item_data;
	OlVirtualEvent		ve;		/* virtual event */
	OlDnDDestinationInfoPtr	dst_info;	/* destination window info */
	OlDnDDragDropInfoPtr	root_info;	/* drag-n-drop info */
	OlDnDDropStatus		drop_status;	/* drop status */
} OlFlatDropCallData;

	/* Define Drag Cursor data: this is supplied to drag cursor call
	   backs by flat subclasses which support Drag and Drop.
	 */
typedef struct {
	OlFlatCallData	item_data;
	OlVirtualEvent	ve;		/* virtual event */
	Cursor		yes_cursor;	/* YES cursor in DnD, will use	*/
					/* OlGetMoveCursor if not	*/
					/* specified (i.e., None).	*/
	Cursor		no_cursor;	/* NO cursor in DnD, will use	*/
					/* OlGetNoCursor if not		*/
					/* specified (i.e., None).	*/
	Position	x_hot;		/* hot spot of the YES cursor.	*/
	Position	y_hot;
	Boolean		static_cursor;	/* specify whether application	*/
					/* supplied YES and NO cursors	*/
					/* should be freed after Dnd	*/
					/* by widget. Default is True.	*/
} OlFlatDragCursorCallData;

	/* Define a structure for the complex flat help id */
typedef struct _OlFlatHelpId {
	Widget		widget;
	Cardinal	item_index;
} OlFlatHelpId;

/*
 ************************************************************************	
 * Declare external functions that applications can use.
 ************************************************************************	
 */

OLBeginFunctionPrototypeBlock

extern Boolean
OlFlatCallAcceptFocus OL_ARGS((
	Widget,		/* Flat widget id	*/
	Cardinal,	/* Item index		*/
	Time		/* time			*/
));

extern void
OlFlatChangeManagedItems OL_ARGS((
	Widget,		/* w;			container widget id	*/
	Cardinal *,	/* managed_items;	items to manage		*/
	Cardinal,	/* num_managed;		num items to manage	*/
	Cardinal *,	/* unmanaged_items;	items to unmanage	*/
	Cardinal	/* num_unmanaged;	num items to unmanage	*/
));

extern Cardinal
OlFlatGetFocusItem OL_ARGS((
	Widget		/* Flat widget id	*/
));

extern void
OlFlatGetItemGeometry OL_ARGS((
	Widget,		/* Flat widget id	*/
	Cardinal,	/* item index		*/
	Position *,	/* x return		*/
	Position *,	/* y return		*/
	Dimension *,	/* width return		*/
	Dimension *	/* height return	*/
));

extern Cardinal
OlFlatGetItemIndex OL_ARGS((
	Widget,		/* Flat widget id	*/
	Position,	/* x location		*/
	Position	/* y location		*/
));

extern void
OlFlatGetValues OL_ARGS((
	Widget,		/* widget;	- flat widget id	*/
	Cardinal,	/* item_index;	- item to query		*/
	ArgList,	/* args;	- querying args		*/
	Cardinal	/* num_args;	- number of args	*/
));

		/* OlFlatRefreshItem is an interface to
		 * OlFlatRefreshExpandItem				*/
extern void
OlFlatRefreshItem OL_ARGS((
	Widget,		/* w;		container widget id		*/
	Cardinal,	/* item_index;	Item to draw			*/
	Boolean		/* clear_area;	should the area be cleared?	*/
));

extern void
OlFlatSetValues OL_ARGS((
	Widget,		/* widget;	- flat widget id	*/
	Cardinal,	/* item_index;	- item to modify	*/
	ArgList,	/* args;	- modifying args	*/
	Cardinal	/* num_args;	- number of args	*/
));

extern void
OlVaFlatGetValues OL_ARGS((
	Widget,		/* widget;	- flat widget id	*/
	Cardinal,	/* item_index;	- item to query		*/
	...		/* NULL terminated name/value pairs	*/
));

extern void
OlVaFlatSetValues OL_ARGS((
	Widget,		/* widget;	- flat widget id	*/
	Cardinal,	/* item_index;	- item to modify	*/
	...		/* NULL terminated name/value pairs	*/
));
OLEndFunctionPrototypeBlock

#endif /* _OL_FLAT_H */
