/*		copyright	"%c%" 	*/

#ifndef	NOIDENT
#ident	"@(#)flat:FRowColumn.c	1.13"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the source code for the flat row/column
 *	widget class.  The row/column class is a meta-class, i.e.,
 *	it's not intended for instantiation; rather, it serves as a
 *	managing class for its subclass's items.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/FRowColumP.h>

#define ClassName FlatRowColumn
#include <Xol/NameDefs.h>

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define LOCAL_SIZE	50

#define INIT_GEOM_FIELDS(w)					\
	RCPART(w).col_widths		= (Dimension *) NULL;	\
	RCPART(w).row_heights		= (Dimension *) NULL;	\
	RCPART(w).x_offset		= (Position) 0;		\
	RCPART(w).y_offset		= (Position) 0;		\
	RCPART(w).bounding_width	= (Dimension) 0;	\
	RCPART(w).bounding_height	= (Dimension) 0;	\
	RCPART(w).rows			= (Cardinal) 0;		\
	RCPART(w).cols			= (Cardinal) 0

					/* Define some handy macros	*/

#define FPART(w)		(((FlatWidget)(w))->flat)
#define RCPART(w)		(((FlatRowColumnWidget)(w))->row_column)
#define DRAW_ITEM(w,ei,i)	(*OL_FLATCLASS(w).draw_item)(w,ei,i)

				/* Define a structure for holding widths
				 * and heights				*/
typedef struct {
	Dimension	width;
	Dimension	height;
} W_and_H;

					/* Define constants for width
					 * and height calculations	*/

#define ALL_ITEMS	0		/* every item			*/
#define COLUMN_ITEMS	1		/* All items in a column	*/
#define ROW_ITEMS	2		/* All items in a row		*/

#define	WIDTH			0
#define	HEIGHT			1

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static Dimension CalcItemSize OL_ARGS((Widget, W_and_H *, int, int,
					Cardinal, Cardinal, Cardinal));
static void	CheckLayoutParameters OL_ARGS((Widget, Widget));
static Cardinal	GetFocusItems OL_ARGS((Widget, Cardinal, int, Cardinal *,
					Cardinal **, Cardinal *, int));
static void	Layout OL_ARGS((Widget));
static void	WarningMsg OL_ARGS((Widget, String, String, char *));

					/* class procedures		*/

static void	ClassInitialize OL_NO_ARGS();
static void	ChangeManaged OL_ARGS((Widget, FlatItem *, Cardinal));
static void	Destroy OL_ARGS((Widget));
static void	DrawItem OL_ARGS((Widget, FlatItem, OlFlatDrawInfo *));
static void	DrawLocationCursor OL_ARGS((Display *, Window, GC,
					Dimension, OlFlatDrawInfo *));
static void	GetGeometry OL_ARGS((Widget, Cardinal, Position*, Position*,
				Dimension *, Dimension *));
static Cardinal	GetIndex OL_ARGS((Widget, Position, Position, Boolean));
static XtGeometryResult GeometryHandler OL_ARGS((Widget, FlatItem,
			OlFlatItemGeometry *, OlFlatItemGeometry *));
static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static void	Redisplay OL_ARGS((Widget, XEvent *, Region));
static void	Resize OL_ARGS((Widget));
static Boolean	SetValues OL_ARGS((Widget, Widget, Widget,ArgList,Cardinal*));
static Cardinal	TraverseItems OL_ARGS((Widget,Cardinal,OlVirtualName,Time));

					/* action procedures		*/

/* There are no action procedures */

					/* public procedures		*/

/* There are no public procedures */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* There are no translations or actions */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

			/* Define the resources for the container.	*/

#define	OFFSET(f)	XtOffsetOf(FlatRowColumnRec, row_column.f)

static XtResource
resources[] = {
					/* Define layout resources	*/

	{ XtNgravity, XtCGravity, XtRGravity, sizeof(int),
	  OFFSET(gravity), XtRImmediate, (XtPointer) ((int)CenterGravity) },

	{ XtNhPad, XtCHPad, XtRDimension, sizeof(Dimension),
	  OFFSET(h_pad), XtRImmediate, (XtPointer) ((Dimension) 0)},

	{ XtNhSpace, XtCHSpace, XtRDimension, sizeof(Dimension),
	  OFFSET(h_space), XtRImmediate, (XtPointer) ((Dimension) 0)},

	{ XtNitemGravity, XtCItemGravity, XtRGravity, sizeof(int),
	  OFFSET(item_gravity), XtRImmediate,
	  (XtPointer) ((int) NorthWestGravity) },

	{ XtNitemMaxHeight, XtCItemMaxHeight, XtRDimension, sizeof(Dimension),
	  OFFSET(item_max_height), XtRImmediate,
	  (XtPointer) ((Dimension) OL_IGNORE) },

	{ XtNitemMaxWidth, XtCItemMaxWidth, XtRDimension, sizeof(Dimension),
	  OFFSET(item_max_width), XtRImmediate,
	  (XtPointer) ((Dimension) OL_IGNORE) },

	{ XtNitemMinHeight, XtCItemMinHeight, XtRDimension, sizeof(Dimension),
	  OFFSET(item_min_height), XtRImmediate,
	  (XtPointer) ((Dimension) OL_IGNORE) },

	{ XtNitemMinWidth, XtCItemMinWidth, XtRDimension, sizeof(Dimension),
	  OFFSET(item_min_width), XtRImmediate,
	  (XtPointer) ((Dimension) OL_IGNORE) },

	{ XtNlayoutHeight, XtCLayout, XtROlDefine, sizeof(OlDefine),
	  OFFSET(layout_height), XtRImmediate,
	  (XtPointer) ((OlDefine) OL_MINIMIZE) },

	{ XtNlayoutType, XtCLayoutType, XtROlDefine, sizeof(OlDefine),
	  OFFSET(layout_type), XtRImmediate,
	  (XtPointer) ((OlDefine) OL_FIXEDROWS) },

	{ XtNlayoutWidth, XtCLayout, XtROlDefine, sizeof(OlDefine),
	  OFFSET(layout_width), XtRImmediate,
	  (XtPointer) ((OlDefine) OL_MINIMIZE) },

	{ XtNmeasure, XtCMeasure, XtRInt, sizeof(int),
	  OFFSET(measure), XtRImmediate, (XtPointer) ((int)1) },

	{ XtNsameHeight, XtCSameHeight, XtROlDefine, sizeof(OlDefine),
	  OFFSET(same_height), XtRImmediate, (XtPointer) ((OlDefine) OL_ALL) },

	{ XtNsameWidth, XtCSameWidth, XtROlDefine, sizeof(OlDefine),
	  OFFSET(same_width), XtRImmediate,
	  (XtPointer) ((OlDefine) OL_COLUMNS) },

	{ XtNvPad, XtCVPad, XtRDimension, sizeof(Dimension),
	  OFFSET(v_pad), XtRImmediate, (XtPointer) ((Dimension) 0) },

	{ XtNvSpace, XtCVSpace, XtRDimension, sizeof(Dimension),
	  OFFSET(v_space), XtRImmediate, (XtPointer) ((Dimension) 4) }
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatRowColumnClassRec
flatRowColumnClassRec = {
    {
	(WidgetClass)&flatClassRec,		/* superclass		*/
	"FlatRowColumn",			/* class_name		*/
	sizeof(FlatRowColumnRec),		/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	0,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	Resize,					/* resize		*/
	Redisplay,				/* expose		*/
	NULL,					/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	XtInheritAcceptFocus,			/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	XtInheritTranslations,			/* tm_table		*/
	NULL,					/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
        True,					/* focus_on_select	*/
	XtInheritHighlightHandler,		/* highlight_handler	*/
	XtInheritTraversalHandler,		/* traversal_handler	*/
	NULL,					/* register_focus	*/
	XtInheritActivateFunc,			/* activate		*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	OlVersion,				/* version		*/
	NULL,					/* extension		*/
	{
		(_OlDynResourceList)NULL,	/* resources		*/
		(Cardinal)0			/* num_resources	*/
	},					/* dyn_data		*/
	XtInheritTransparentProc		/* transparent_proc	*/
    },	/* End of Primitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	False,					/* transparent_bg	*/
	XtOffsetOf(FlatRowColumnRec, default_item),/* default_offset	*/
	sizeof(FlatRowColumnItemRec),		/* rec_size		*/

		/*
		 * See ClassInitialize for procedures
		 */
    } /* End of Flat Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatRowColumnWidgetClass = (WidgetClass) &flatRowColumnClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * CheckLayoutParameters - this routine checks the validity of various
 * layout parameters.  If any of the parameters are invalid, a warning
 * is generated and the values are set to a valid value. 
 ****************************procedure*header*****************************
 */
static void
CheckLayoutParameters OLARGLIST((current, new))
	OLARG( Widget,	current)	/* Current widget id or NULL	*/
	OLGRA( Widget,	new)		/* New widget id		*/
{
	String	error_type = (current == (Widget)NULL ?
				(String)OleTinitialize : (String)OleTsetValues);

			/* Define a macro to speed things up (typing that is)
			 * and make sure that there are no spaces after
			 * the commas when this is used.		*/

#define CHANGED(f)	(current == (Widget)NULL || \
			(RCPART(current).f != RCPART(new).f))

#ifdef __STDC__
#define CLEANUP(field, r, value) \
	WarningMsg(new,error_type,#r,#value); RCPART(new).field = value;
#else
#define CLEANUP(field, r, value) \
	WarningMsg(new,error_type,"r","value"); RCPART(new).field = value;
#endif

	if (CHANGED(gravity))
	{
		switch (RCPART(new).gravity)
		{
		case EastGravity:			/* fall through	*/
		case WestGravity:			/* fall through	*/
		case CenterGravity:			/* fall through	*/
		case NorthGravity:			/* fall through	*/
		case NorthEastGravity:			/* fall through	*/
		case NorthWestGravity:			/* fall through	*/
		case SouthGravity:			/* fall through	*/
		case SouthEastGravity:			/* fall through	*/
		case SouthWestGravity:
			break;				/* Do Nothing	*/
		default:
			CLEANUP(gravity, XtNgravity, CenterGravity)
			break;
		}
	}

	if (CHANGED(item_gravity))
	{
		switch (RCPART(new).item_gravity)
		{
		case EastGravity:			/* fall through	*/
		case WestGravity:			/* fall through	*/
		case CenterGravity:			/* fall through	*/
		case NorthGravity:			/* fall through	*/
		case NorthEastGravity:			/* fall through	*/
		case NorthWestGravity:			/* fall through	*/
		case SouthGravity:			/* fall through	*/
		case SouthEastGravity:			/* fall through	*/
		case SouthWestGravity:
			break;				/* Do Nothing	*/
		default:
			CLEANUP(item_gravity,XtNitemGravity,NorthWestGravity)
			break;
		}
	}

	if (CHANGED(layout_height) &&
	    (RCPART(new).layout_height != (OlDefine)OL_IGNORE &&
	     RCPART(new).layout_height != (OlDefine)OL_MAXIMIZE &&
	     RCPART(new).layout_height != (OlDefine)OL_MINIMIZE))
	{
		CLEANUP(layout_height,XtNlayoutHeight,OL_MINIMIZE)
	}

	if (CHANGED(layout_type) &&
	    (RCPART(new).layout_type != (OlDefine)OL_FIXEDCOLS &&
	     RCPART(new).layout_type != (OlDefine)OL_FIXEDROWS))
	{
		CLEANUP(layout_type,XtNlayoutType,OL_FIXEDROWS)
	}

	if (CHANGED(layout_width) &&
	    (RCPART(new).layout_width != (OlDefine)OL_IGNORE &&
	     RCPART(new).layout_width != (OlDefine)OL_MAXIMIZE &&
	     RCPART(new).layout_width != (OlDefine)OL_MINIMIZE))
	{
		CLEANUP(layout_width,XtNlayoutWidth,OL_MINIMIZE)
	}

	if (CHANGED(measure) && RCPART(new).measure < 1)
	{
		CLEANUP(measure,XtNmeasure,1)
	}

	if (CHANGED(same_width) &&
	    (RCPART(new).same_width != (OlDefine)OL_NONE &&
	     RCPART(new).same_width != (OlDefine)OL_COLUMNS &&
	     RCPART(new).same_width != (OlDefine)OL_ALL))
	{
		CLEANUP(same_width,XtNsameWidth,OL_COLUMNS)
	}

	if (CHANGED(same_height) &&
	    (RCPART(new).same_height != (OlDefine)OL_NONE &&
	     RCPART(new).same_height != (OlDefine)OL_ROWS &&
	     RCPART(new).same_height != (OlDefine)OL_ALL))
	{
		CLEANUP(same_height,XtNsameHeight,OL_ALL)
	}

#undef CLEANUP
#undef CHANGED
} /* END OF CheckLayoutParameters() */

/*
 *************************************************************************
 * GetFocusItems - this routine initializes an array with the item
 * indecies of all items in the current row or column.  If the supplied
 * array is too small, a new one is allocated.
 * This routine also returns the focus_item's index relative to the
 * returned array.
 ****************************procedure*header*****************************
 */
static Cardinal
GetFocusItems OLARGLIST((w, focus_item,
			do_row, num_ret, array_ret, array, array_size))
	OLARG( Widget,		w)
	OLARG( Cardinal,	focus_item)	/* focus item index	*/
	OLARG( int,		do_row)
	OLARG( Cardinal *,	num_ret)
	OLARG( Cardinal **,	array_ret)
	OLARG( Cardinal *,	array)
	OLGRA( int,		array_size)
{
	Cardinal	r;			/* row of focus item	*/
	Cardinal	c;			/* column of focus item	*/
	Cardinal	i;
	Cardinal	start_index;
	Cardinal	item_index;
	Cardinal *	roci;			/* row or column items	*/
	Cardinal	adder;
	Cardinal	max_num;	/* max number of focus items	*/

				/* IF 'do_row' is true calculate the
				 * row that the focus_item is in
				 * and fill in the array; else
				 * calculate the column that the focus_item
				 * item is in and fill in the array.	*/

	if (RCPART(w).layout_type == OL_FIXEDCOLS)
	{
		r = focus_item/RCPART(w).cols;
		c = focus_item - (r * RCPART(w).cols);

		if (do_row) {
			adder		= 1;
			item_index	= r * RCPART(w).cols;
		} else {
			adder		= RCPART(w).cols;
			item_index	= c;
		}
	}
	else		/* OL_FIXEDROWS	*/
	{
		c = focus_item/RCPART(w).rows;
		r = focus_item - (c * RCPART(w).rows);

		if (do_row) {
			adder		= RCPART(w).rows;
			item_index	= r;
		} else {
			adder		= 1;
			item_index	= c * RCPART(w).rows;
		}
	}

	max_num = (do_row ? RCPART(w).cols : RCPART(w).rows);

	if (max_num > (Cardinal)array_size) {
		roci = (Cardinal *)XtMalloc(max_num * sizeof(Cardinal));
	} else {
		roci = array;
	}

	for (i=0, *num_ret=0;
	     i < max_num && item_index < FPART(w).num_items;
	     ++i)
	{
		if (item_index == focus_item) {
			start_index = i;
		}
		roci[i] = item_index;
		item_index += adder;
		++(*num_ret);		/* count true number of items	*/
	}

	*array_ret = roci;
	return(start_index);
} /* END OF GetFocusItems() */

/*
 *************************************************************************
 * WarningMsg - this routine generates a warning message from the
 * specified arguments.
 ****************************procedure*header*****************************
 */
static void
WarningMsg OLARGLIST((w, error_type, resource, value))
	OLARG( Widget,	w)		/* The culprit			*/
	OLARG( String,	error_type)	/* Procedure name used in msg	*/
	OLARG( String,	resource)	/* name of the resource		*/
	OLGRA( char *,	value)		/* valid value (text form)	*/
{
	String 	message;

	if (!strcmp((OLconst XtPointer)error_type,
				(OLconst XtPointer)OleTinitialize))
	{
		message = (String)OleMinvalidResource_initialize;
	}
	else
	{
		message = (String)OleMinvalidResource_setValues;
	}

	OlVaDisplayWarningMsg(XtDisplay(w),
		OleNinvalidResource, error_type, OleCOlToolkitWarning,
		message, XtName(w), OlWidgetToClassName(w), resource, value);
} /* END OF WarningMsg() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * CalcItemSize - this routine calculates the height of one or more items.
 * the type of calculation depends on the rule passed in.
 ****************************procedure*header*****************************
 */
static Dimension
CalcItemSize OLARGLIST((w, w_h, rule, flag, i, rows, cols))
	OLARG( Widget,		w)
	OLARG( W_and_H,		w_h[])	/* Array of widths & heights	*/
	OLARG( int,		rule)	/* Type of desired calculation	*/
	OLARG( int,		flag)	/* WIDTH or HEIGHT		*/
	OLARG( Cardinal,	i)	/* item number			*/
	OLARG( Cardinal,	rows)	/* number of rows in layout	*/
	OLGRA( Cardinal,	cols)	/* number of columns in layout	*/
{
	Dimension	max;		/* maximum width or height	*/

	switch(rule)
	{
	case ALL_ITEMS:
		{
			Dimension tmp;	/* temporary calculation holder	*/

			for(i=0, max=0; i < FPART(w).num_items; ++i)
			{
				tmp = (flag == WIDTH ?
						w_h[i].width : w_h[i].height);
				if (tmp > max)
					max = tmp;
			}
		}
		break;
	case COLUMN_ITEMS:

		if (flag == HEIGHT)
		{
		    OlVaDisplayErrorMsg((Display *) NULL,
					OleNfileFlat,
					OleTmsg2,
					OleCOlToolkitError,
					OleMfileFlat_msg2);
		}

			 /* if fixed columns, then row-major order	*/

		if (RCPART(w).layout_type == OL_FIXEDCOLS)
		{
			Cardinal r;			/* current row	*/

			for (max=0, r=0; r < rows && i < FPART(w).num_items;
			     ++r, i += cols)
			{
				if (w_h[i].width > max)
					max = w_h[i].width;
			}
		}
		else {			/* fixed rows is column-major	*/
			Cardinal r;			/* current row	*/

			for (max=0, r=0, i *= rows;
			     r < rows && i < FPART(w).num_items; ++r, ++i)
			{
				if (w_h[i].width > max)
					max = w_h[i].width;
			}
		}

		break;
	case ROW_ITEMS:

		if (flag == WIDTH)
		{
		    OlVaDisplayErrorMsg((Display *) NULL,
					OleNfileFlat,
					OleTmsg3,
					OleCOlToolkitError,
					OleMfileFlat_msg3);
		}

			/* This rule says find the maximum height
			 * for all items in a particular row.
			 * OL_FIXEDROWS implies column-major order;
			 * if fixed columns, then row-major order	*/

		if (RCPART(w).layout_type == OL_FIXEDCOLS)
		{
			Cardinal c;		/* current column	*/

			for (max=0, c=0, i *= cols;
			     c < cols && i < FPART(w).num_items; ++c, ++i)
			{
				if (w_h[i].height > max)
					max = w_h[i].height;
			}
		}
		else {			/* fixed rows is column-major	*/
			Cardinal c;		/* current column	*/

			for (max=0, c=0; c < cols && i < FPART(w).num_items;
			     ++c, i += rows)
			{
				if (w_h[i].height > max)
					max = w_h[i].height;
			}
		}
		break;
	default:
		OlVaDisplayErrorMsg((Display *) NULL,
				    OleNfileFlat,
				    OleTmsg4,
				    OleCOlToolkitError,
				    OleMfileFlat_msg4);
		break;
	}

	return(max);
} /* END OF CalcItemSize() */

/*
 *************************************************************************
 * ClassInitialize - this routine initializes the Flat widgets by
 * registering some converters
 ****************************procedure*header*****************************
 */
static void
ClassInitialize OL_NO_ARGS()
{
			/* Define a macro to aid in the registering of
			 * OlDefine types.				*/

#ifdef __STDC__
#define REGISTER_OLDEFINE(d)	\
	_OlAddOlDefineType((String)#d,(OlDefine)d)
#else
#define REGISTER_OLDEFINE(d)	\
	_OlAddOlDefineType((String)"d",(OlDefine)d)
#endif

	REGISTER_OLDEFINE(OL_MAXIMIZE);
	REGISTER_OLDEFINE(OL_MINIMIZE);
	REGISTER_OLDEFINE(OL_IGNORE);
	REGISTER_OLDEFINE(OL_FIXEDROWS);
	REGISTER_OLDEFINE(OL_FIXEDCOLS);
	REGISTER_OLDEFINE(OL_ALL);
	REGISTER_OLDEFINE(OL_NONE);
	REGISTER_OLDEFINE(OL_ROWS);
	REGISTER_OLDEFINE(OL_COLUMNS);
	REGISTER_OLDEFINE(OL_LEFT);
	REGISTER_OLDEFINE(OL_RIGHT);
	REGISTER_OLDEFINE(OL_CENTER);

#undef REGISTER_OLDEFINE

			/* Inherit all superclass procedures, but
			 * override a few as well as provide some chained
			 * procedures.  This scheme saves us from
			 * worrying about putting function pointers
			 * in the wrong class slot if they were statically
			 * declared.  It also allows us to inherit
			 * new functions simply be recompiling, i.e.,
			 * we don't have to stick XtInheritBlah into the
			 * class slot.					*/

	OlFlatInheritAll(flatRowColumnWidgetClass);

#undef F
#define F	flatRowColumnClassRec.flat_class
	F.change_managed	= ChangeManaged;
	F.geometry_handler	= GeometryHandler;
	F.get_item_geometry	= GetGeometry;
	F.get_index		= GetIndex;
	F.initialize		= Initialize;
	F.set_values		= SetValues;
	F.traverse_items	= TraverseItems;

	if (OlGetGui() == OL_MOTIF_GUI)
	{
		F.draw_item	= DrawItem;
	}
#undef F

} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * Destroy - this procedure frees memory allocated by the instance part
 ****************************procedure*header*****************************
 */
static void
Destroy OLARGLIST((w))
	OLGRA( Widget, 	w)
{
	XtFree((XtPointer) RCPART(w).col_widths);
	XtFree((XtPointer) RCPART(w).row_heights);
} /* END OF Destroy() */

/*
 *************************************************************************
 * GetGeometry - given the index of a sub-object, this routine returns
 * the rectangle associated with it.  It's noted that the rectangle is
 * the row/column rectangle containing the entire sub-object, but the
 * sub-object may or may not fill the whole rectangle.
 * The returned x and y positions are relative to the NorthWest corner
 * of the container.
 ****************************procedure*header*****************************
 */
static void
GetGeometry OLARGLIST((w, item_index, x_ret, y_ret, width_ret, height_ret))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLARG( Position *,	x_ret)
	OLARG( Position *,	y_ret)
	OLARG( Dimension *,	width_ret)
	OLGRA( Dimension *,	height_ret)
{
	Cardinal		row;		/* item's row		*/
	Cardinal		col;		/* items' column	*/
	Boolean			do_gravity = False;
	register Cardinal	i;		/* loop counter		*/

	if (item_index >= FPART(w).num_items)
	{
		OlVaDisplayWarningMsg(XtDisplay(w),
			OleNbadItemIndex, OleTflatState,
			OleCOlToolkitWarning,
			OleMbadItemIndex_flatState,
			XtName(w), OlWidgetToClassName(w),
				      "OlFlatGetGeometryProc",
			(unsigned)item_index);

		*x_ret		= (Position) 0;
		*y_ret		= (Position) 0;
		*width_ret	= (Dimension) 0;
		*height_ret	= (Dimension) 0;
		return;
	}

	if (RCPART(w).layout_type == OL_FIXEDCOLS)
	{
		row = item_index/RCPART(w).cols;
		col = item_index - (row * RCPART(w).cols);
	}
	else
	{
		col = item_index/RCPART(w).rows;
		row = item_index - (col * RCPART(w).rows);
	}

				/* Determine the overall y position and
				 * then do a gravity calculation if
				 * necessary				*/

	*y_ret = RCPART(w).y_offset + (Position) (row * RCPART(w).v_space) -
				(Position) (row * RCPART(w).overlap);

	if (RCPART(w).same_height == OL_ALL)
	{
		*height_ret	= RCPART(w).row_heights[0];
		*y_ret		+= *height_ret * row;
	}
	else
	{
		*height_ret = (RCPART(w).same_height == OL_ROWS ?
				RCPART(w).row_heights[row] :
			(do_gravity = True,
			 RCPART(w).row_heights[RCPART(w).rows + item_index]));

		for (i=0; i < row; ++i)
		{
			*y_ret += (Position) RCPART(w).row_heights[i];
		}
	}
				/* Determine the overall x position and
				 * then do a gravity calculation if
				 * necessary				*/

	*x_ret = RCPART(w).x_offset + (Position)(col * RCPART(w).h_space) -
			(Position)(col * RCPART(w).overlap);

	if (RCPART(w).same_width == OL_ALL)
	{
		*width_ret = RCPART(w).col_widths[0];
		*x_ret += *width_ret * col;
	}
	else
	{
		*width_ret = (RCPART(w).same_width == OL_COLUMNS ?
				RCPART(w).col_widths[col] :
			(do_gravity = True,
			 RCPART(w).col_widths[RCPART(w).cols + item_index]));

		for (i=0; i < col; ++i)
		{
			*x_ret += (Position) RCPART(w).col_widths[i];
		}
	}

				/* Do we have to position this item within
				 * it's row and column?			*/

	if (do_gravity == True && RCPART(w).item_gravity != NorthWestGravity)
	{
		_OlDoGravity(RCPART(w).item_gravity, RCPART(w).col_widths[col],
			RCPART(w).row_heights[row], *width_ret, *height_ret,
			x_ret, y_ret);
	}

} /* END OF GetGeometry() */

/*
 *************************************************************************
 * GetIndex - this routine locates a particular sub-object based
 * on a coordinate pair (x,y).  If the x,y pair falls within an item, the
 * item index is returned, else, a OL_NO_ITEM value is returned.
 * The values of X and Y are relative to the interior of the container
 * at hand.
 * When determining which item a coordinate pair falls into, the
 * following rules are observed:
 *	1. an x coordinate is in an item if it's wholely contained within
 *	   that item or is directly on the left edge.
 *	2. a y coordinate is in an item if it's wholely contained within
 *	   that item or is directly on the top edge.
 *
 * If the (x,y) pair is outside the container, OL_NO_ITEM is returned.
 * The application also can request that the sensitivity of the item be
 * ignored.  Normally, an item is not considered valid if it's
 * insensitive.  If the 'ignore_sensitivity' flag is set, the item's id
 * will be returned.
 ****************************procedure*header*****************************
 */
static Cardinal
GetIndex OLARGLIST((w, x, y, ignore_sensitivity))
	OLARG( Widget,			w)	/* Flat widget type	*/
	OLARG( register Position,	x)	/* X source location	*/
	OLARG( register Position,	y)	/* Y source location	*/
	OLGRA( Boolean,			ignore_sensitivity)
{
	Cardinal		item_index;
	Cardinal		row;		/* item's row		*/
	Cardinal		col;		/* items' column	*/
	Position		envelope;	/* horiz or vert location*/
	Position		adder;		/* constant value to add*/
	Position		worh;		/* width or height	*/
	Cardinal		row_mask;	/* row index mask	*/
	Cardinal		col_mask;	/* column index mask	*/

	if (x >= (Position)w->core.width || y >= (Position)w->core.height ||
	    x < (Position)0 || y < (Position)0 )
	{
		return((Cardinal)OL_NO_ITEM);
	}

	row_mask = (Cardinal) (RCPART(w).same_height == OL_ALL ? 0 : ~0);
	col_mask = (Cardinal) (RCPART(w).same_width == OL_ALL ? 0 : ~0);

						/* Find the item's row	*/

	adder		= (Position) RCPART(w).v_space -
					(Position) RCPART(w).overlap;
	envelope	= RCPART(w).y_offset - adder;

	for (row=0; row < RCPART(w).rows; ++row)
	{
		worh	= (Position) RCPART(w).row_heights[row & row_mask];

		if ((envelope += adder + worh) > y)
		{
				/* The y coordinate is within the envelope,
				 * but is it between the rows ?		*/

			if (y < (envelope - worh))
			{
				row = RCPART(w).rows;
			}
			break;
		}
	}

	if (row == RCPART(w).rows)
	{
		return((Cardinal)OL_NO_ITEM);
	}

					/* Find the item's column	*/

	adder		= (Position) RCPART(w).h_space -
					(Position) RCPART(w).overlap;
	envelope	= RCPART(w).x_offset - adder;

	for (col=0; col < RCPART(w).cols; ++col)
	{
		worh	= (Position) RCPART(w).col_widths[col & col_mask];

		if ((envelope += adder + worh) > x)
		{
				/* The x coordinate is within the envelope,
				 * but is it between the columns ?	*/

			if (x < (envelope - worh))
			{
				col = RCPART(w).cols;
			}
			break;
		}
	}

	if (col == RCPART(w).cols)
	{
		return((Cardinal)OL_NO_ITEM);
	}
		/* If we've reached this point, we've got the row and
		 * the column.  Now we have to get the index and then
		 * check to see if this item fills up it's cell.  If it
		 * doesn't, we've to to apply the item's gravity and
		 * check it again.					*/

	item_index = (RCPART(w).layout_type == OL_FIXEDCOLS ?
		row * RCPART(w).cols + col : col * RCPART(w).rows + row);

	if (item_index >= FPART(w).num_items)
	{
		return((Cardinal)OL_NO_ITEM);
	}

	if (RCPART(w).same_height != OL_ALL || RCPART(w).same_width != OL_ALL)
	{
		Dimension item_width	= RCPART(w).col_widths[
			(RCPART(w).same_width == OL_ALL ? 0 :
			 RCPART(w).same_width == OL_COLUMNS ? col :
						RCPART(w).cols + item_index)];

		Dimension item_height	= RCPART(w).row_heights[
			(RCPART(w).same_height == OL_ALL ? 0 :
			 RCPART(w).same_height == OL_ROWS ? row :
						RCPART(w).rows + item_index)];

		Position  item_x;
		Position  item_y;

		_OlDoGravity(RCPART(w).item_gravity,
			RCPART(w).col_widths[col & col_mask],
			RCPART(w).row_heights[row & row_mask],
			item_width, item_height,
			&item_x, &item_y);

		item_x += x;
		item_y += y;

		if (!(	x >= item_x && x < (item_x + (Position)item_width) &&
			y >= item_y && y < (item_y + (Position)item_height)))
		{
			item_index = (Cardinal)OL_NO_ITEM;
		}
	}

		/* Because the semantics of this routine requires we
		 * check the sensitivity,  managed and mapped state of
		 * the item, we're forced to expand the item.  Maybe in
		 * the future, this can be optimized by caching a array
		 * of bit flags for these three state values.  If a
		 * cached array scheme is used, we won't need to expand
		 * the item.
		 * NOTE: the current row/column semantics ignores the
		 * managed state of all items, so we don't bother
		 * checking it here.					*/

	if (item_index != (Cardinal) OL_NO_ITEM)
	{
		OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

		OlFlatExpandItem(w, item_index, item);

		if (item->flat.mapped_when_managed == False ||
		    (ignore_sensitivity == False &&
		     item->flat.sensitive == False))
		{
			item_index = (Cardinal) OL_NO_ITEM;
		}
		OL_FLAT_FREE_ITEM(item);
	}
	return(item_index);
} /* END OF GetIndex() */

/*
 *************************************************************************
 * Initialize - this procedure initializes the instance part of the widget
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)	/* What we want		*/
	OLARG( Widget,		new)		/* What we get, so far..*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
			/* Initialize non-application settable data	*/

	INIT_GEOM_FIELDS(new);

	RCPART(new).overlap	= (Dimension) 0;

	CheckLayoutParameters((Widget) NULL, new);

} /* END OF Initialize() */

/*
 *************************************************************************
 * Layout - this routine determines the logical layout of a flat
 * widget container based on the information given for it's sub-objects.
 * This procedure puts information into the FlatPart concerning:
 * 	- the rectangle needed to tightly fit around all sub-objects.
 *	- the description of the container's layout, including gravity.
 *
 * The layout is done in a two step process.  First the width and height
 * needed to tightly fit around the items is calculated.  These
 * dimensions are not affected by the container's actual size.  After
 * the tightly fitting box is calculated, the container's width and
 * height are calculated based on the XtNlayoutType resource (these
 * are the returned values).  Next the container's gravity is calculated
 * based on the widget's dimensions.
 *
 * This routine caches information about the layout in the fields
 * row_heights and col_widths.  These two arrays are symmetrical in
 * usage.  Therefore:
 *
 *	Value of XtNsameWidth	'col_widths' contents
 *
 *		OL_ALL		1 element in array containing the
 *				width of all columns
 *		OL_COLUMNS	'cols' elements in the array containing
 *				the width of each column
 *		OL_NONE		('cols' + 'number of managed items')
 *				elements, containing the width of
 *				each column in the first 'cols' locations
 *				and the width of each item in the
 *				remaining locations.
 *		
 *	Value of XtNsameHeight	'row_heights' contents
 *
 *		OL_ALL		1 element in array containing the
 *				height of all rows
 *		OL_ROWS		'rows' elements in the array containing
 *				the height of each row
 *		OL_NONE		('rows' + 'number of managed items')
 *				elements, containing the height of
 *				each row in the first 'rows' locations
 *				and the height of each item in the
 *				remaining locations.
 *		
 ****************************procedure*header*****************************
 */
static void
Layout OLARGLIST((w))
	OLGRA( Widget,		w)		/* Flat widget		*/
{
	Cardinal	i;		/* loop counter			*/
	Dimension	width;		/* local width calculation	*/
	Dimension	height;		/* local height calculation	*/
	Cardinal	rows;		/* number of rows		*/
	Cardinal	cols;		/* number of columns		*/
	W_and_H			local_array[20];
	register W_and_H *	w_h_ptr;
	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

				/* Free the old information and
				 * re-initialize the geometry structure	*/

	XtFree((XtPointer) RCPART(w).col_widths);
	XtFree((XtPointer) RCPART(w).row_heights);

	INIT_GEOM_FIELDS(w);

				/* If there are no items, return; else
				 * set the width and height array pointer*/

	if (FPART(w).num_items == (Cardinal) 0)
	{
		OL_FLAT_FREE_ITEM(item);
		return;
	}
	else if (FPART(w).num_items > XtNumber(local_array))
	{
		w_h_ptr = (W_and_H *) XtMalloc(FPART(w).num_items *
						sizeof(W_and_H));
	}
	else
	{
		w_h_ptr = local_array;
	}

			/* Before doing the layout, get the width
			 * and height of each item.
			 * Get the size calculation procedure from the
			 * class field.  We don't have to see if it is
			 * NULL since the ClassPartInitialize procedure
			 * did the check already.			*/

	for (i=0; i < FPART(w).num_items; ++i)
	{
		OlFlatExpandItem(w, i, item);
		OlFlatItemDimensions(w, item, &width, &height);

					/* Make sure width is valid	*/

		if (RCPART(w).item_min_width != (Dimension) OL_IGNORE &&
		    width < RCPART(w).item_min_width)
		{
			width = RCPART(w).item_min_width;
		}
		if (RCPART(w).item_max_width != (Dimension) OL_IGNORE && 
			   RCPART(w).item_max_width < width)
		{
			width = RCPART(w).item_max_width;
		}
					/* Make sure height is valid	*/

		if (RCPART(w).item_min_height != (Dimension) OL_IGNORE &&
		    height < RCPART(w).item_min_height)
		{
			height = RCPART(w).item_min_height;
		}
		if (RCPART(w).item_max_height != (Dimension) OL_IGNORE && 
			   RCPART(w).item_max_height < height)
		{
			height = RCPART(w).item_max_height;
		}

		w_h_ptr[i].width	= width;
		w_h_ptr[i].height	= height;
	}

	if (RCPART(w).layout_type == OL_FIXEDCOLS)
	{
		if (FPART(w).num_items <= (Cardinal) RCPART(w).measure)
		{
			cols = FPART(w).num_items;
			rows = (Cardinal) 1;
		}
		else
		{
			cols = (Cardinal) RCPART(w).measure;
			rows = FPART(w).num_items/cols +
				(FPART(w).num_items % cols == 0 ? 0 : 1);
		}
	}
	else
	{
		if (FPART(w).num_items <= (Cardinal) RCPART(w).measure)
		{
			rows = FPART(w).num_items;
			cols = (Cardinal) 1;
		}
		else
		{
			rows = (Cardinal) RCPART(w).measure;
			cols = FPART(w).num_items/rows +
				(FPART(w).num_items % rows == 0 ? 0 : 1);
		}
	}

	RCPART(w).rows = rows;		/* Cache the number of rows	*/
	RCPART(w).cols = cols;		/* Cache the number of columns	*/

	if (RCPART(w).same_width == OL_ALL)
	{
					/* Allocate a single slot	*/

		RCPART(w).col_widths = XtNew(Dimension);

		RCPART(w).col_widths[0] = CalcItemSize(w, w_h_ptr,
				ALL_ITEMS, WIDTH, 0, NULL, NULL);
		width = (Dimension) (cols * RCPART(w).col_widths[0]);
	}
	else
	{
		Cardinal slots;

		slots = cols + (RCPART(w).same_width == OL_COLUMNS ?
					0 : FPART(w).num_items);

		RCPART(w).col_widths = (Dimension *)
					XtMalloc(slots * sizeof(Dimension));

					/* Fill in the maximum width of
					 * each column.			*/

		for (i=0, width = 0; i < cols; ++i)
		{
			RCPART(w).col_widths[i] = CalcItemSize(w,
				w_h_ptr, COLUMN_ITEMS, WIDTH, i, rows, cols);
			width += RCPART(w).col_widths[i];
		}

				/* If 'same_width' equals OL_NONE,
				 * put the width of each item into the
				 * array.				*/

		if (RCPART(w).same_width == OL_NONE)
		{
			for(i=0; i < FPART(w).num_items; ++i)
			{
				RCPART(w).col_widths[i+cols] = w_h_ptr[i].width;
			}
		}
	} /* end of 'RCPART(w).same_width != OL_ALL' */

					/* Calculate the heights	*/

	if (RCPART(w).same_height == OL_ALL)
	{
					/* Allocate a single slot	*/

		RCPART(w).row_heights = XtNew(Dimension);

		RCPART(w).row_heights[0] = CalcItemSize(w, w_h_ptr,
				ALL_ITEMS, HEIGHT, 0, NULL, NULL);
		height = (Dimension) (rows * RCPART(w).row_heights[0]);
	}
	else
	{
		Cardinal slots;

		slots = rows + (RCPART(w).same_height == OL_ROWS ?
					0 : FPART(w).num_items);

		RCPART(w).row_heights = (Dimension *)
					XtMalloc(slots * sizeof(Dimension));

					/* Fill in the maximum height of
					 * each row.			*/

		for (i=0, height = 0; i < rows; ++i)
		{
			RCPART(w).row_heights[i] = CalcItemSize(w,
				w_h_ptr, ROW_ITEMS, HEIGHT, i, rows, cols);
			height += RCPART(w).row_heights[i];
		}

				/* If 'same_height' equals OL_NONE,
				 * put the height of each item into the
				 * array.				*/

		if (RCPART(w).same_height == OL_NONE)
		{
			for(i=0; i < FPART(w).num_items; ++i)
			{
				RCPART(w).row_heights[i+rows] =
							w_h_ptr[i].height;
			}
		}
	} /* end of 'RCPART(w).same_height != OL_ALL' */

	
	width	+= 2*(RCPART(w).h_pad) + (cols - 1)*(RCPART(w).h_space) -
					(cols - 1) * RCPART(w).overlap;
	height	+= 2*(RCPART(w).v_pad) + (rows - 1)*(RCPART(w).v_space) -
					(rows - 1) * RCPART(w).overlap;

				/* Set bounding width and height.  These
				 * values do not include the exterior
				 * padding.				*/

	RCPART(w).bounding_width  = width - (2 * RCPART(w).h_pad);
	RCPART(w).bounding_height = height - (2 * RCPART(w).v_pad);

				/* Now, check the layout rules to see
				 * what the container size must be	*/

	switch(RCPART(w).layout_width)
	{
	case OL_MINIMIZE:
		w->core.width = width;
		break;
	case OL_MAXIMIZE:
		w->core.width = (width > w->core.width ? width : w->core.width);
		break;
	}

	switch(RCPART(w).layout_height)
	{
	case OL_MINIMIZE:
		w->core.height = height;
		break;
	case OL_MAXIMIZE:
		w->core.height = (height > w->core.height ? height :
					w->core.height);
		break;
	}

			/* Now update the gravity fields.  Our resize
			 * procedure does just this.  Note: don't call
			 * the class resize procedure since a subclass
			 * may have specified a different resize
			 * procedure which may do something else.	*/

	Resize(w);

			/* If there is an item resize procedure, call
			 * it for all the items that have changed
			 * in size.					*/

	if (OL_FLATCLASS(w).item_resize)
	{
		Cardinal	row;
		Cardinal	col;

		for (i=0; i < FPART(w).num_items; ++i)
		{
			if (RCPART(w).same_width == OL_COLUMNS ||
			    RCPART(w).same_height == OL_ROWS)
			{
				if (RCPART(w).layout_type == OL_FIXEDCOLS)
				{
					row = i/RCPART(w).cols;
					col = i - (row * RCPART(w).cols);
				}
				else
				{
					col = i/RCPART(w).rows;
					row = i - (col * RCPART(w).rows);
				}
			}

			switch(RCPART(w).same_width) {
			case OL_ALL:
				width = RCPART(w).col_widths[0];
				break;
			case OL_COLUMNS:
				width = RCPART(w).col_widths[col];
				break;
			default:
				width =
				   RCPART(w).col_widths[RCPART(w).cols + i];
				   break;
			}

			switch(RCPART(w).same_height) {
			case OL_ALL:
				height = RCPART(w).row_heights[0];
				break;
			case OL_ROWS:
				height = RCPART(w).row_heights[row];
				break;
			default:
				height =
				   RCPART(w).row_heights[RCPART(w).rows + i];
				   break;
			}

			if (w_h_ptr[i].width != width ||
			    w_h_ptr[i].height != height)
			{
				OlFlatExpandItem(w, i, item);

				item->flat.width	= width;
				item->flat.height	= height;

				(*OL_FLATCLASS(w).item_resize)(w, item);
			}
		}
	}

				/* Free width and height array if
				 * necessary				*/

	if (w_h_ptr != local_array)
	{
		XtFree((char *) w_h_ptr);
	}

	OL_FLAT_FREE_ITEM(item);

} /* END OF Layout() */

/*
 *************************************************************************
 * ChangeManaged - this routine is called whenever one or more flat items
 * change their managed state.  It's also called when the items get
 * touched or when a relayout hint was suggested.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ChangeManaged OLARGLIST((w, items, num_changed))
	OLARG( Widget, 		w)
	OLARG( FlatItem *,	items)
	OLGRA( Cardinal,	num_changed)
{
	Layout(w);
} /* END OF ChangeManaged() */

/*
 *************************************************************************
 * GeometryHandler - this procedure is called whenever an item wants to
 * change size.  It is called by OlFlatMakeGeometryRequest.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static XtGeometryResult
GeometryHandler OLARGLIST((w, item, request, reply))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLARG( OlFlatItemGeometry *,	request)
	OLGRA( OlFlatItemGeometry *,	reply)
{
	XtGeometryResult	result = XtGeometryYes;

	*reply = *request;

					/* This is the geometry handler
					 * for the row/column layout	*/
#undef MODE
#define MODE(r) ((r)->request_mode)

			/* If changes to x & y were requested, deny them.
			 * In this case, however, check to see if more
			 * than just positional changes were requested.
			 * If so, they compromise; else the request is
			 * refused later in this routine.		*/

	if ((MODE(request) & (CWX|CWY)) &&
	    (MODE(request) != (CWX|CWY)))
	{
		result		= XtGeometryAlmost;
		MODE(reply)	&= ~(CWX|CWY);
	}

	if (MODE(request) & (CWWidth|CWHeight))
	{
		Dimension		old_width = w->core.width;
		Dimension		old_height = w->core.height;

			/* Let the ChangeManaged routine do the layout	*/

		(*OL_FLATCLASS(w).change_managed)(w, (FlatItem *)NULL,
							(Cardinal)0);

			/* If the widget's dimensions change, it's our
			 * responsibility to ask our parent to approve
			 * the changes.					*/

		if (old_width != w->core.width ||
		    old_height != w->core.height)
		{
			Boolean			resize = False;
			XtGeometryResult	p_result;
			XtWidgetGeometry	p_request;
			XtWidgetGeometry	p_reply;

			MODE(&p_request) = 0;

			if (old_width != w->core.width)
			{
				MODE(&p_request) |= CWWidth;
				p_request.width   = w->core.width;
				w->core.width     = old_width;
			}

			if (old_height != w->core.height)
			{
				MODE(&p_request) |= CWHeight;
				p_request.height  = w->core.height;
				w->core.height    = old_height;
			}

			if ((p_result = XtMakeGeometryRequest(w, &p_request,
					&p_reply)) == XtGeometryAlmost)
			{
				resize = True;
				p_request = p_reply;
				p_result = XtMakeGeometryRequest(w, &p_request,
							&p_reply);
			}

				/* If the result was a Yes, do nothing
				 * because we've already laid the items
				 * assuming the size is approved.  If the
				 * request is refused or a compromise was
				 * used, then we'll call the resize procedure
				 * to recover.				*/

			if ((p_result == XtGeometryNo || resize == True) &&
			    XtClass(w)->core_class.resize)
			{
				(*(XtClass(w)->core_class.resize))(w);
			}
		}

			/* Rather than being too intelligent here, just
			 * refresh the container all the time.
			 * (We could have checked to see if the new item's
			 * size did not affect any of the other items.
			 * In that case we'd only have to refresh the 
			 * item rather than the whole container...but
			 * it's late.  So take the easy way out.)	*/

		_OlClearWidget(w, True);

		result = XtGeometryDone;
	}
	else
	{
		result = XtGeometryNo;
	}

	return(result);
#undef MODE
} /* END OF GeometryHandler() */

static void
DrawLocationCursor OLARGLIST((dpy, window, gc, thickness, di))
	OLARG( Display	*,	dpy)
	OLARG( Window,		window)
	OLARG( GC,		gc)
	OLARG( Dimension,	thickness)
	OLGRA( OlFlatDrawInfo *,di)
{
	XRectangle rects[4];

	rects[0].x = di->x;
	rects[0].y = di->y;
	rects[0].width = thickness;
	rects[0].height = di->height;

	rects[1].x = di->x + thickness;
	rects[1].y = rects[0].y;
	rects[1].width = di->width - thickness;
	rects[1].height = thickness;

	rects[2].x = rects[1].x;
	rects[2].y = di->y + di->height - thickness;
	rects[2].width = rects[1].width;
	rects[2].height = rects[1].height;

	rects[3].x = di->x + di->width - thickness;
	rects[3].y = rects[1].y + thickness;
	rects[3].width = rects[0].width;
	rects[3].height = rects[0].height - thickness - thickness;

	XFillRectangles(dpy, window, gc, rects, 4);
} /* end of DrawLocationCursor */

/*
 *************************************************************************
 * DrawItem - this routine is called to draw a Motif style location
 *	cursor. This should be an item_highlight routine but subclass
 *	is calling OlFlatRefreshItem lots of time so...
 *
 * Note that, Subclass should call this method in Motif mode from
 *	it's draw_item().
 ****************************procedure*header*****************************
 */
/* ARGSUSED2 */
static void
DrawItem OLARGLIST((w, item, dii))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLGRA( OlFlatDrawInfo *,	dii)
{
#define METHOD			OL_FLATCLASS(w).item_location_cursor_dimensions
#define DRAW_LOC_CURSOR		( !METHOD || (*METHOD)(w, item, &di) == True )

	OlFlatDrawInfo			di;

	if ( (di = *dii, DRAW_LOC_CURSOR) )
	{
#undef METHOD
#undef DRAW_LOC_CURSOR

		Display *		dpy = XtDisplay(w);
		GC			gc;
		Pixel			fg;
		OlFlatScreenCache *	sc = OlFlatScreenManager(w,
						OL_DEFAULT_POINT_SIZE,
						OL_JUST_LOOKING);
		Window			window = XtWindow(w);
		XGCValues		values;

			/* This check is for Flat List, sigh... */
		if (FPART(w).focus_item == item->flat.item_index)
			fg = item->flat.input_focus_color;
		else
			fg = item->flat.background_pixel;

		gc = sc->scratch_gc;
		values.foreground = fg;
		XCopyGC(dpy, sc->default_gc, (unsigned long)~0, gc);
		XChangeGC(dpy, gc, GCForeground, &values);

#define THICKNESS	((PrimitiveWidget)w)->primitive.highlight_thickness

		DrawLocationCursor(dpy, window, gc, THICKNESS, &di);
#undef THICKNESS
	}
} /* end of DrawItem */

/*
 *************************************************************************
 * Redisplay - this routine is a generic redisplay routine.  It 
 * loops over all of the sub-objects and calls a draw routine.
 * This routine does not have to check the class procedures for NULL since
 * the ClassPartInitialize procedure has already checked.
 ****************************procedure*header*****************************
 */
/* ARGSUSED2 */
static void
Redisplay OLARGLIST((w, xevent, region))
	OLARG( Widget,		w)		/* exposed widget	*/
	OLARG( XEvent *,	xevent)		/* the exposure		*/
	OLGRA( Region,		region)		/* compressed exposures	*/
{
#undef RP
#define RP	RCPART(w)
	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);
	OlFlatDrawInfo	draw_info;
	register
	OlFlatDrawInfo *di = &draw_info;
	Cardinal	item_index;		/* item index to refresh*/
	Position	h_start;		/* Expose x position	*/
	Position	v_start;		/* Expose y position	*/
	Position	h_end;			/* Expose x + width pos.*/
	Position	v_end;			/* Expose y + height pos.*/
	Cardinal	row;			/* First row to draw at	*/
	Cardinal	col;			/* column loop counter	*/
	Cardinal	col_start;		/* first column to draw	*/
	Cardinal	col_stop;		/* stop drawing at this col.*/
	Position	x_start;		/* first column's x pos.*/
	Position	y_adder;		/* constant vertical incr.*/
	Position	x_adder;		/* constant horiz. incr.*/
	Boolean		do_gravity;
	Cardinal	row_mask;		/* Row index mask	*/
	Cardinal	col_mask;		/* Column index mask	*/

#define SUPERCLASS	\
	((FlatRowColumnClassRec *)flatRowColumnClassRec.core_class.superclass)

		/* Use superclass expose method to draw shadow_thickness... */
	(*SUPERCLASS->core_class.expose)(w, xevent, region);

#undef SUPERCLASS
	if (OL_FLATCLASS(w).draw_item == (OlFlatDrawItemProc)NULL)
	{
		OL_FLAT_FREE_ITEM(item);
		return;
	}

	di->drawable	= (Drawable) XtWindow(w);
	di->screen	= XtScreen(w);
	di->background	= w->core.background_pixel;

			/* Put the event into a more usable form	*/

	h_start	= (Position) xevent->xexpose.x;
	v_start	= (Position) xevent->xexpose.y;
	h_end	= (Position) xevent->xexpose.width + h_start;
	v_end	= (Position) xevent->xexpose.height + v_start;

				/* Initialize the constant incrementers	*/

	x_adder = (Position) RP.h_space - (Position) RP.overlap;
	y_adder = (Position) RP.v_space - (Position) RP.overlap;

	row_mask = (Cardinal) (RP.same_height == OL_ALL ? 0 : ~0);
	col_mask = (Cardinal) (RP.same_width == OL_ALL ? 0 : ~0);

			/* Calculate the columns to start refreshing at
			 * and the number of columns after that to
			 * refresh.					*/

	if (h_start <= RCPART(w).x_offset &&
	    (RP.x_offset + (Position)RP.bounding_width) <= h_end)
	{
		col_start	= (Cardinal) 0;
		x_start		= RP.x_offset;
		col_stop	= RP.cols;
	}
	else
	{
		for (col=0, di->x = RP.x_offset - x_adder,
		     col_start = RP.cols, col_stop = RP.cols;
		     col < RP.cols; ++col)
		{
			di->x += x_adder + (Position)
				RP.col_widths[col & col_mask];

			if (col_start == RP.cols)
			{
				if (di->x >= h_start )
				{
					col_start = col;
					x_start = di->x - (Position)
					   RP.col_widths[col & col_mask];
				}
			}
			else if (h_end <= di->x)
			{
				col_stop = col;
				if (di->x - (Position)
				   RP.col_widths[col & col_mask] <= h_end)
				{
					++col_stop;
				}
				break;
			}
		}

		if (col_start == RP.cols)
		{
			OL_FLAT_FREE_ITEM(item);
			return;		/* exposure is to the left or
					 * right of all columns		*/
		}
	}

	do_gravity = (RP.same_width != OL_ALL || RP.same_height != OL_ALL ?
			       True : False);

					/* Now refresh the items	*/

	for (row=0, di->y = RP.y_offset; row < RP.rows;
	     di->y += y_adder + (Position)RP.row_heights[row & row_mask],
	     ++row)
	{
					/* Increment to next row	*/

		di->height = RP.row_heights[row & row_mask];

		if (v_start > (Position)(di->y + (Position)di->height))
		{
			continue;		/* Skip this row	*/
		}
		else if (v_end < di->y)
		{
			break;			/* all rows are done	*/
		}


					/* Loop through the columns	*/

		for (col=col_start, di->x = x_start; col < col_stop; ++col)
		{

			item_index = (RP.layout_type == OL_FIXEDCOLS ?
					row * RP.cols + col :
					col * RP.rows + row);

			if (item_index >= FPART(w).num_items)
			{
				break;		/* go to next row	*/
			}

			di->width = (RP.same_width == OL_ALL ?
					RP.col_widths[0] :
				 RP.same_width == OL_COLUMNS ?
					RP.col_widths[col] :
					RP.col_widths[RP.cols + item_index]);

					/* Use last height if possible	*/

			if (RP.same_height == OL_NONE)
			{
				di->height = RP.row_heights[RP.rows +
								item_index];
			}

			OlFlatExpandItem(w, item_index, item);

			if (item->flat.managed == True &&
			    item->flat.mapped_when_managed == True)
			{
			    if (do_gravity == False)
			    {
				DRAW_ITEM(w, item, di);
			    }
			    else
			    {
				Position ix;	/* item's x location	*/
				Position iy;	/* item's y location	*/

				_OlDoGravity(RP.item_gravity,
					RP.col_widths[col & col_mask],
					RP.row_heights[row & row_mask],
					di->width, di->height, &ix, &iy);

				ix	+= di->x;
				iy	+= di->y;

				ix	^= di->x;	/* Swap the x's	*/
				di->x	^= ix;
				ix	^= di->x;

				iy	^= di->y;	/* Swap the y's	*/
				di->y	^= iy;
				iy	^= di->y;

				DRAW_ITEM(w, item, di);

						/* Restore x & y	*/

				di->x = ix;
				di->y = iy;
			    }
			}

			di->x += x_adder + (Position)
					RP.col_widths[col & col_mask];
		}
	}

	OL_FLAT_FREE_ITEM(item);
#undef RP
} /* END OF Redisplay() */

/*
 *************************************************************************
 * Resize - this procedure repositions the items within a container after
 * the container has undergone a resize by modifying the x and y offsets
 * of the bounding box that tightly envelopes all the items.
 * This calculation is done by taking the bounding box dimension and
 * adding the horizontal and vertical paddings to it.
 * After calling the gravity procedure, the returned offsets are adjusted
 * to compensate for the previously-added paddings.
 ****************************procedure*header*****************************
 */
static void
Resize OLARGLIST((w))
	OLGRA( Widget,	w)		/* flat widget container	*/
{
	_OlDoGravity(RCPART(w).gravity, w->core.width, w->core.height,
			(RCPART(w).bounding_width + (2 * RCPART(w).h_pad)),
			(RCPART(w).bounding_height + (2 * RCPART(w).v_pad)),
			&RCPART(w).x_offset, &RCPART(w).y_offset);

			/* Adjust the offset so that it is the upper left
			 * had corner of the first item.		*/

	RCPART(w).x_offset += RCPART(w).h_pad;
	RCPART(w).y_offset += RCPART(w).v_pad;
} /* END OF Resize() */

/*
 *************************************************************************
 * SetValues - this procedure monitors the changing of instance data.
 * Since the subclasses inherit the layout from this class, whenever this
 * class detects a change in any of its layout parameters, it sets a flag
 * in the instance part so that the subclasses merely have to check the
 * flag instead of checking all of their layout parameters.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,		current)	/* What we had		*/
	OLARG( Widget,		request)	/* What we want		*/
	OLARG( Widget,		new)		/* What we get, so far	*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	Boolean	redisplay = False;

		/* If one of the layout parameters has changed, check the
		 * validity of the parameters.  Then, if a parameter
		 * is still different, suggest a relayout		*/

#undef DIFFERENT
#define DIFFERENT(field)	(RCPART(new).field != RCPART(current).field)

#define PARAMETERS_HAVE_CHANGED \
	(DIFFERENT(gravity)		||\
	DIFFERENT(h_pad)		||\
	DIFFERENT(h_space)		||\
	DIFFERENT(item_gravity)		||\
	DIFFERENT(item_max_height)	||\
	DIFFERENT(item_max_width)	||\
	DIFFERENT(item_min_height)	||\
	DIFFERENT(item_min_width)	||\
	DIFFERENT(layout_height)	||\
	DIFFERENT(layout_type)		||\
	DIFFERENT(layout_width)		||\
	DIFFERENT(measure)		||\
	DIFFERENT(same_height)		||\
	DIFFERENT(same_width)		||\
	DIFFERENT(v_pad)		||\
	DIFFERENT(v_space))

	if (PARAMETERS_HAVE_CHANGED)
	{
		CheckLayoutParameters(current, new);

		/* Check the parameters again.  This allows us to avoid
		 * a relayout in the case when the new value was invalid
		 * and the widget set the value back to the current
		 * value.						*/

		if (PARAMETERS_HAVE_CHANGED)
		{
			FPART(new).relayout_hint = True;
		}
	}

	return(redisplay);

#undef DIFFERENT
#undef PARAMETERS_HAVE_CHANGED
} /* END OF SetValues() */

/*
 *************************************************************************
 * TraversalItems - this routine sets focus sub-objects.  When this
 * routine is called, the FlatWidget already has the input focus
 ****************************procedure*header*****************************
 */
static Cardinal
TraverseItems OLARGLIST((w, sfi, dir, time))
	OLARG( Widget,		w)	/* FlatWidget id		*/
	OLARG( Cardinal,	sfi)	/* start focus item		*/
	OLARG( OlVirtualName,	dir)	/* Direction to move		*/
	OLGRA( Time,		time)	/* Time of move (ignored)	*/
{
	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);
	Cardinal	start_pos;
	Cardinal	i;			/* current position	*/
	Cardinal	array[LOCAL_SIZE];
	Cardinal *	roci;		/* row or column items		*/
	Cardinal	num;		/* # of items to take focus	*/
	Cardinal	multi;
	Cardinal	new_focus_item;
	Time		timestamp = time;

	if (!OL_FLATCLASS(w).item_accept_focus)
	{
		return(FPART(w).focus_item);
	}

					/* Set the starting item	*/
	switch(dir) {
	case OL_MOVERIGHT:	/* FALLTHROUGH */
	case OL_MOVEDOWN:
		start_pos = GetFocusItems(w, sfi, (dir == OL_MOVERIGHT),
					&num, &roci, array, LOCAL_SIZE);
		i = (start_pos + 1) % num;
		break;
	case OL_MOVELEFT:	/* FALLTHROUGH */
	case OL_MOVEUP:
		start_pos = GetFocusItems(w, sfi, (dir == OL_MOVELEFT),
					&num, &roci, array, LOCAL_SIZE);
		i = (start_pos == 0 ? num : start_pos) - 1;
		break;
	case OL_MULTIRIGHT:
		dir = OL_MOVERIGHT;
		/* FALLTHROUGH */
	case OL_MULTIDOWN:
		if (dir == OL_MULTIDOWN) {
			dir = OL_MOVEDOWN;
		}
		multi = _OlGetMultiObjectCount(w);
		start_pos = GetFocusItems(w, sfi, (dir == OL_MOVERIGHT),
					&num, &roci, array, LOCAL_SIZE);
		i = ((start_pos + multi) >= num ? 0 : (start_pos + multi));
		break;
	case OL_MULTILEFT:
		dir = OL_MOVELEFT;
		/* FALLTHROUGH */
	case OL_MULTIUP:
		if (dir == OL_MULTIUP) {
			dir = OL_MOVEUP;
		}
		multi = _OlGetMultiObjectCount(w);
		start_pos = GetFocusItems(w, sfi, (dir == OL_MOVELEFT),
					&num, &roci, array, LOCAL_SIZE);
		i = (start_pos < multi ? (num - 1) : (start_pos - multi));
		break;
	default:
		OL_FLAT_FREE_ITEM(item);
		return (OL_NO_ITEM);
	}

	new_focus_item = (Cardinal)OL_NO_ITEM;	/* init new focus item */

	do {
		OlFlatExpandItem(w, roci[i], item);

		if ((*OL_FLATCLASS(w).item_accept_focus)(w, item, &timestamp)
			== True)
		{
			new_focus_item = item->flat.item_index;
			break;
		}

		switch(dir){
		case OL_MOVEUP:
		case OL_MOVELEFT:
			if (i == 0)
				i = num - 1;
			else
				--i;
			break;
		default:	/* OL_MOVERIGHT	&& OL_MOVEDOWN	*/
			if (i == (num-1))
				i = 0;
			else
				++i;
			break;
		}

	} while (i != start_pos);

					/* Free the array if necessary	*/
	if (roci != array)
	{
		XtFree((char *)roci);
	}

	OL_FLAT_FREE_ITEM(item);

	return(new_focus_item);
} /* END OF TraverseItems() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/* There are no action procedures */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public procedures */
