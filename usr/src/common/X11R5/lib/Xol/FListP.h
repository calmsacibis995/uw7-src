#ifndef	NOIDENT
#ident	"@(#)flat:FListP.h	1.12"
#endif

#ifndef _OL_FLISTP_H
#define _OL_FLISTP_H

/************************************************************************
    Description:
	This is the flat list container's private header file.
*/

#include <Xol/FRowColumP.h>	/* superclasses' header */
#include <Xol/FList.h>		/* public header */

/************************************************************************
    Define Widget Class Part and Class Rec
*/
				/* Define new fields for the class part	*/

typedef struct {
    char no_class_fields;	/* Makes the compiler happy */
} FlatListClassPart;

				/* Full class record declaration 	*/

typedef struct _FlatListClassRec {
    CoreClassPart		core_class;
    PrimitiveClassPart		primitive_class;
    FlatClassPart		flat_class;
    FlatRowColumnClassPart	row_column_class;
    FlatListClassPart		list_class;
} FlatListClassRec;

				/* External class record declaration	*/

extern FlatListClassRec	flatListClassRec;

/************************************************************************
    Define Widget Instance Structure
*/
			/* Define Expanded sub-object instance	*/
typedef struct {
    XtPointer		client_data;	/* for callbacks */
    XtPointer *		field_data;	/* vector of item field data */
    XtCallbackProc	cursor_proc;	/* drag cursor call back */
    XtCallbackProc	dbl_select_proc;/* execute for multi-click */
    XtCallbackProc	drop_proc;	/* drop for drag 'N drop */
    XtCallbackProc	select_proc;	/* select callback */
    XtCallbackProc	unselect_proc;	/* unselect callback */
    Boolean		selected;	/* item is selected or not */
} FlatListItemPart;

			/* Item's Full Instance record declaration */
typedef struct {
    FlatItemPart		flat;
    FlatRowColumnItemPart	row_column;
    FlatListItemPart		list;
} FlatListItemRec, *FlatListItem;

			/* Define new fields for the instance part.
			   '@' in the comment indicates public resource.
			*/
typedef struct {
					/* pointers & ints: */
    String		format;		/* @ format string (ie. "%s") */
    Cardinal		view_height;	/* @ height of view in 'slots' */
    XtCallbackList	visibility;	/* as items enter/leave view */
    XtCallbackList	limit_exceeded; /* as items limit exceeded */
    Widget		vSBar;		/* vert SBar from SWin */
    struct _Field *	fields;		/* array of Field records */
    Cardinal		num_fields;	/* number of fields */
    Position *		y_offsets;	/* 'virtual' y offsets for each item */
    Cardinal		num_slots;	/* regular sized item slots */
    Cardinal		top_slot;	/* 'slot' index of top item */
    Cardinal		selected_item;	/* for excl: index of selected item */
    Cardinal		start_indx;	/* for motion: starting index */
    Cardinal		prev_indx;	/* for motion: previous index */
    unsigned long	repeat_rate;	/* scrollbar repeat rate */
					/* shorts: */
    Dimension		total_padding;	/* Max of total padding for items */
    Dimension		min_field_width;/* to calibrate horiz scrolling */
    Dimension		min_height;	/* height of shortest item */
    Dimension		total_height;	/* effective height */
					/* bytes: */
    char		prev_motion;
    Boolean		exclusive_settings; /* @ [non]exclusives behavior */
    Boolean		none_set;	/* @ for excl: none-set allowable */
    Boolean		need_vsb;
    Boolean		begin_scroll;	/* auto-scroll after initial delay */
    Boolean		maintain_view;	/* @ try to maintain the same view */
					/* when a list is being touched. */
    Boolean		need_hsb;	/* True, if hsb is there */
    Boolean		use_preferred;
    Boolean		bottom_is_partial;
    Cardinal		preferred;
} FlatListPart;

				/* Full instance record declaration	*/
typedef struct _FlatListRec {
    CorePart		core;
    PrimitivePart	primitive;
    FlatPart		flat;
    FlatRowColumnPart	row_column;
    FlatListPart	list;

    FlatListItemRec	default_item;	/* embedded full item record */
} FlatListRec;

#endif /* _OL_FLISTP_H */
