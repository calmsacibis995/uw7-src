#ident	"@(#)debugger:libmotif/common/Xm/ListP.h	1.1"
#pragma ident	"@(#)m1.2libs:Xm/ListP.h	1.5"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmListP_h
#define _XmListP_h

#include <Xm/List.h>
#include <Xm/PrimitiveP.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  List struct passed to Convert proc for drag and drop */
typedef struct _XmListDragConvertStruct
{
   Widget w;
   XmString * strings;
   int num_strings;
} XmListDragConvertStruct;

/*  List class structure  */

typedef struct _XmListClassPart
{
   XtPointer extension;   /* Pointer to extension record */
} XmListClassPart;


/*  Full class record declaration for List class  */

typedef struct _XmListClassRec
{
   CoreClassPart        core_class;
   XmPrimitiveClassPart primitive_class;
   XmListClassPart     list_class;
} XmListClassRec;

externalref XmListClassRec xmListClassRec;

/****************
 *
 * Internal form of the list elements.
 *
 ****************/

#ifdef NOVELL

typedef struct {
	Pixmap		pixmap;	/* pixmap, dft = None */
	Pixmap		mask;	/* mask, dft = None */
	int		depth;	/* depth of pixmap and mask, no dft */
	Dimension	width;	/* width/height of pixmap/mask, no dft */
	Dimension	height;
	Dimension	h_pad;	/* padding between glyph and label, dft = 0 */
	Dimension	v_pad;	/* vertical padding, dft = 0 */
	XmGlyphPosition	glyph_pos; /* placement of the glyph, dft = LEFT */
	Boolean		static_data;	/* True - app will free pixmap, mask,
					 *	when the widget is destroyed.
					 * False - widget should do freeing.
					 * dft = False */
} GlyphData;

#endif /* NOVELL */
 
typedef	struct {
	_XmString	name;
	Dimension	height;
	Dimension	width;
	Dimension	CumHeight;
	Boolean		selected;
	Boolean		last_selected;
	Boolean		LastTimeDrawn;
	unsigned short	NumLines;
	int		length;
#ifdef NOVELL
	GlyphData *	glyph_data;
#endif /* NOVELL */
} Element, *ElementPtr;

/*  The List instance record  */

typedef struct _XmListPart
{
	Dimension	spacing;
	short           ItemSpacing;
	Dimension       margin_width;
	Dimension    	margin_height;
	XmFontList 	font;
	XmString	*items;
	int		itemCount;
	XmString	*selectedItems;
        int             *selectedIndices;
	int		selectedItemCount;
	int 		visibleItemCount;
	int 		LastSetVizCount;
	unsigned char	SelectionPolicy;
	unsigned char	ScrollBarDisplayPolicy;
	unsigned char	SizePolicy;
        XmStringDirection StrDir;

        Boolean		AutoSelect;
        Boolean		DidSelection;
        Boolean		FromSetSB;
        Boolean		FromSetNewSize;
        Boolean		AddMode;
	unsigned char	LeaveDir;
	unsigned char	HighlightThickness;
	int 		ClickInterval;
        XtIntervalId	DragID;
	XtCallbackList 	SingleCallback;
	XtCallbackList 	MultipleCallback;
	XtCallbackList 	ExtendCallback;
	XtCallbackList 	BrowseCallback;
	XtCallbackList 	DefaultCallback;


	GC		NormalGC;	
	GC		InverseGC;
	GC		HighlightGC;
        Pixmap          DashTile;
	ElementPtr	*InternalList;
	int		LastItem;
	int		FontHeight;
	int		top_position;
	char		Event;
	int		LastHLItem;
	int		StartItem;
	int		OldStartItem;
	int		EndItem;
	int		OldEndItem;
	Position	BaseX;
	Position	BaseY;
	Boolean		MouseMoved;
	Boolean		AppendInProgress;
	Boolean		Traversing;
	Boolean		KbdSelection;
	short		DownCount;
	Time		DownTime;
	int		CurrentKbdItem;
	unsigned char	SelectionType;
	GC		InsensitiveGC;

	int vmin;		  /*  slider minimum coordiate position     */
	int vmax;		  /*  slider maximum coordiate position     */
	int vOrigin;		  /*  slider edge location                  */
	int vExtent;		  /*  slider size                           */

	int hmin;		  /*  Same as above for horizontal bar.     */
	int hmax;
	int hOrigin;
	int hExtent;

	Dimension	MaxWidth;
	Dimension	CharWidth;
	Position	XOrigin;
	
	XmScrollBarWidget   	hScrollBar;
	XmScrollBarWidget   	vScrollBar;
	XmScrolledWindowWidget  Mom;
	Dimension	MaxItemHeight;

#ifdef NOVELL /* will need to move out after the prototype */

	XtCallbackList	item_init_cb;	/* XmNitemInitCallback, dft = NULL */
	Dimension *	max_col_width;	/* private, see MaxWidth */
	Dimension	col_spacing;	/* XmNlistColumnSpacing, dft = 0 */
	short 		cols;		/* XmNnumColumns, dft = 1 */
	short 		static_rows;	/* XmNstaticRowCount, dft = 0 */
	short		col_info;	/* private, see WhichItem,BrowseScroll*/

#endif /* NOVELL */
	
} XmListPart;


/*  Full instance record declaration  */

typedef struct _XmListRec
{
   CorePart	   core;
   XmPrimitivePart primitive;
   XmListPart	   list;
} XmListRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmListP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
