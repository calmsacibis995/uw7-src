#ifndef	NOIDENT
#ident	"@(#)scrollinglist:ListPaneP.h	1.31"
#endif
/*
 ListPaneP.h (C hdr file)
	Acc: 596865505 Tue Nov 29 22:58:25 1988
	Mod: 596865505 Tue Nov 29 22:58:25 1988
	Sta: 596865505 Tue Nov 29 22:58:25 1988
	Owner: 4777
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/* 
 * ListPaneP.h - Private definitions for ListPane widget
 */

#ifndef _ListPaneP_h
#define _ListPaneP_h

#include <Xol/PrimitiveP.h>		/* include superclasses's header */
#include <Xol/ListPane.h>		/* include public header file */

#include <Xol/ScrollingL.h>		/* container's header file */
#include <Xol/Olg.h>			/* drawing pkg header */
#include <Xol/array.h>			/* for list-offset array */

/***********************************************************************
	Declarations, #defines, etc.
 */

/* dynamic resource bit masks */
#define OL_B_LISTPANE_BG			(1 << 0)
#define OL_B_LISTPANE_FONTCOLOR			(1 << 1)

typedef struct _OlHeadRec {
    int		offset;			/* offset of head in internal list */
    IntArray	offsets;		/* offsets of items & item count */
} * _OlHead;

/***********************************************************************
 *
 *	Class structure
 *
 */

/* New fields for the ListPane widget class record */
typedef struct {
    char no_class_fields;		/* Makes compiler happy */
} ListPaneClassPart;

/* Full class record declaration */
typedef struct _ListPaneClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	ListPaneClassPart	list_pane_class;
} ListPaneClassRec;

/* Class record variable */
externalref ListPaneClassRec listPaneClassRec;

/***********************************************************************
 *
 *	Instance (widget) structure
 *
 */

/* New fields for the ListPane widget record */
typedef struct { 
    /* "PUBLIC" (resource) members */
    OlListToken (*applAddItem) OL_ARGS((Widget, OlListToken,                                                            OlListToken, OlListItem));
    void	(*applDeleteItem) OL_ARGS((Widget, OlListToken));
    void	(*applEditClose) OL_ARGS((Widget));
    void	(*applEditOpen) OL_ARGS((Widget, Boolean, OlListToken));
    void	(*applTouchItem) OL_ARGS((Widget, OlListToken));
    void	(*applUpdateView) OL_ARGS((Widget, Boolean));
    void	(*applViewItem)  OL_ARGS((Widget, OlListToken));
    Boolean	recompute_width;	/* resize or live with geometry */
    Boolean	selectable;
    Cardinal	view_height;		/* # of items in view */

    /* PRIVATE members */
    Cardinal		actualViewHeight; /* (actual) # of items in view */
    OlgAttrs *		attr_focus;	/* drawing attrs for focus_item */
    OlgAttrs *		attr_normal;	/* drawing attrs */
    unsigned char	dyn_flags;	/* dynamic resources dirty bits */
    Boolean		editing;	/* item being editted (?) */
    int			focus_item;	/* list offset of item with "focus" */
    GC			gc_inverted;
    GC			gc_normal;
    struct _OlHeadRec	head;		/* head of list */
    int			initial_motion;	/* direction of initial motion */
    Cardinal		items_selected;	/* number of items selected */
    Dimension		max_height;	/* height of tallest item (in pixels) */
    Dimension		max_width;	/* width of widest item (in pixels) */
    Boolean		own_clipboard;	/* CLIPBOARD ownership */
    Boolean		own_selection;	/* selection ownership */
    int			prev_index;	/* for motion: previous index */
    unsigned long	repeat_rate;	/* for auto scolling */
    int			scroll;		/* pointer w/i pane for auto scroll */
    int			search_item;	/* item last searched from keyboard */
    int			start_index;	/* for motion: start index */
    int			top_item;	/* list offset of top item in view */
    Widget		text_field;	/* editable text field */
    XtIntervalId	timer_id;	/* timer for auto-scroll */
    Boolean		update_view;	/* view is locked/unlocked */

    /* item spacing: these are pixel dimensions derived from device-
     * independent dimensions.  they are fixed so are calculated once.
     */
    /* margin between top (bottom) of * pane and top (bottom) item: */
    Dimension vert_margin;
    Dimension horiz_margin;		/* horiz margin between pane & item */

    /* total vertical padding.  therefore: pad + label height = item height */
    Dimension vert_pad;

    /* total horizontal padding.  therefore: pad + label width = item width */
    Dimension horiz_pad;
} ListPanePart;

/* Full instance record declaration */
typedef struct _ListPaneRec {
	CorePart	core;
	PrimitivePart	primitive;
	ListPanePart	list_pane;
} ListPaneRec;

#endif /* _ListPaneP_h */
