#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:FGraph.c	1.18"
#endif

/******************************file*header********************************
 *
 * Description:
 *	This file contains the source code for the flattened graph
 *	widget.  This widget uses an arbitrary layout to place the
 *	sub-objects within it's container.
 */

						/* #includes go here	*/
#include <limits.h>
#include "FGraphP.h"
#include "ScrollUtil.h"

/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */
#define GET_INFO(w, i)	GetItemInfo(w, i, (Cardinal *)NULL)

	/* Adjust ExmFLAT_[X|Y]_UOM values depend on the class field,
	 * check_uoms (was no_class_field).
	 *
	 * Can't perform this check automatically because dtm does
	 * too many XtSetValues(items_touched == True) and it's possible
	 * that actual_width and actual_height will be set to `1' temporary
	 * during their layout calculation...
	 */
#define CHECK_UOMS(W)	( ((ExmFlatGraphWidgetClass)XtClass(W))->\
						graph_class.check_uoms )

/**************************forward*declarations***************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 */

					/* private procedures		*/

static ExmFlatGraphInfo * GetItemInfo(Widget, Cardinal, Cardinal*);
static void	Layout(Widget, WideDimension *, WideDimension *);
static void	CalcSize(Widget, WideDimension *, WideDimension *);
static void	PutItemInView(Widget, ExmFlatGraphInfoList);
					/* scrolled window related...	*/
static void	ScrollbarChangedCB(Widget, XtPointer, XtPointer);

					/* class procedures		*/

static void	ClassInitialize(void);
static void	ChangeManaged(Widget, ExmFlatItem *, Cardinal);
static void	Destroy(Widget);
static Cardinal	GetIndex(Widget, WidePosition, WidePosition, Boolean);
static void	GetItemGeometry(Widget, Cardinal, WidePosition *,
				WidePosition *, Dimension *, Dimension *);
static XtGeometryResult GeometryHandler(Widget, ExmFlatItem,
					ExmFlatItemGeometry *,
					ExmFlatItemGeometry *);
static void	Initialize(Widget, Widget, ArgList, Cardinal *);
static Boolean	ItemSetValues(Widget, ExmFlatItem, ExmFlatItem,
			      ExmFlatItem, ArgList, Cardinal *);
static void	Redisplay(Widget, XEvent *, Region);
static void	RefreshItem(Widget, ExmFlatItem, Boolean);
static Cardinal	TraverseItems(Widget, Cardinal, ExmTraverseDirType);

					/* action procedures		*/
/* There are no action procedures	*/

/***********************widget*translations*actions***********************
 *
 * Define Translations and Actions
 */

/****************************widget*resources*****************************
 *
 * Define Resource list associated with the Widget Instance
 */

#undef  OFFSET
#define OFFSET(f)	XtOffsetOf(ExmFlatRec, flat.f)

static XtResource
resources[] = {
	    /* Scrolling related - 0 cols and/or 0 rows mean that the width
	     * and/or height value(s) will be computed from its parent.
	     * For dtm, setting both rows/cols mean the folder is opened
	     * the very first time, otherwise the geometry info was saved
	     * in the .dtinfo file. */
	{ XmNgridColumns, XmCGridColumns, XmRUnsignedChar,
	  sizeof(unsigned char), OFFSET(cols), XmRImmediate, (XtPointer)0 },

	{ XmNgridRows, XmCGridRows, XmRUnsignedChar, sizeof(unsigned char),
	  OFFSET(rows), XmRImmediate, (XtPointer)0 },

	{ XmNgridHeight, XmCGridHeight, XmRVerticalDimension,
	  sizeof(Dimension), OFFSET(row_height), XmRImmediate, (XtPointer)1 },

	{ XmNgridWidth, XmCGridWidth, XmRHorizontalDimension,
	  sizeof(Dimension), OFFSET(col_width), XmRImmediate, (XtPointer)1 },
};

/***************************widget*class*record***************************
 *
 * Define Class Record structure to be initialized at Compile time
 */

ExmFlatGraphClassRec
exmFlatGraphClassRec = {
    {
	(WidgetClass)&exmFlatClassRec,		/* superclass		*/
	"ExmFlatGraph",				/* class_name		*/
	sizeof(ExmFlatGraphRec),		/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	(Cardinal)0,				/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	XtExposeCompressMultiple,		/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	NULL,					/* resize		*/
	Redisplay,				/* expose		*/
	NULL,					/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	NULL,					/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	NULL,					/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
	XmInheritBorderHighlight,		/* border_highlight   */
	XmInheritBorderUnhighlight,		/* border_unhighlight */
	XtInheritTranslations,		 	/* translations       */
	NULL,					/* arm_and_activate   */
	NULL,					/* syn resources      */
	0,					/* num syn_resources  */
	NULL,					/* extension          */
    },	/* End of XmPrimitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	XtOffsetOf(ExmFlatGraphRec, default_item),/* default_offset	*/
	sizeof(ExmFlatGraphItemRec),		/* rec_size		*/
	NULL,					/* item_resources	*/
	0,					/* num_item_resources	*/
	(Cardinal)0,				/* mask			*/
	NULL,					/* quarked_items	*/

	/* See ClassInitialize for procedures */

    }, /* End of ExmFlat Class Part Initialization */
    {
	False					/* check_uoms	*/
    } /* End of ExmFlatGraph Class Part Initialization */
};

/*************************public*class*definition*************************
 *
 * Public Widget Class Definition of the Widget Class Record
 */

WidgetClass ExmflatGraphWidgetClass = (WidgetClass) &exmFlatGraphClassRec;

/****************************procedure*header*****************************
 *
 * Procedures
 */

/****************************procedure*header*****************************
 * CalcSize-
 */
static void
CalcSize(Widget w, WideDimension * ret_width, WideDimension * ret_height)
{
    ExmFlatGraphInfoList info;
    int			 i;
    WideDimension	 cwidth=0, cheight=0;
    WidePosition	 x, y;

    for (info = GPART(w).info, i = FPART(w).num_items; i; --i, ++info)
	if ((info->flags & ExmB_FG_MANAGED) &&
	    (info->flags & ExmB_FG_MAPPED)) {
	    x = info->x + info->width;
	    y = info->y + info->height;

	    if (x > cwidth)
		cwidth = x;
	    if (y > cheight)
		cheight = y;
	}

    *ret_width  = cwidth;
    *ret_height = cheight;

}					/* End of CalcSize */

/****************************procedure*header*****************************
 * ScrollbarChangedCB - callbacks for both horizontal and vertical scrollbars.
 */
static void
ScrollbarChangedCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget		       wanted = (Widget)client_data;
    XmScrollBarCallbackStruct *cd = (XmScrollBarCallbackStruct *)call_data;

    ExmSWinHandleValueChange(wanted,
	w,				/* H/V XmScrollBarWidget */
	FPART(wanted).scroll_gc,
	cd->value,
					/* slider units in x/y direction */
	ExmFLAT_X_UOM(wanted), ExmFLAT_Y_UOM(wanted),
					/* current offset values */
	&FPART(wanted).x_offset, &FPART(wanted).y_offset);

}					/* end of ScrollbarChangedCB */

/****************************procedure*header*****************************
 * ChangeManaged - this routine is called whenever one or more flat items
 * change their managed state.  It's also called when the items get
 * touched or when a relayout hint was suggested.
 */
static void
ChangeManaged(Widget w, ExmFlatItem * items, Cardinal num_changed)
{
    WideDimension	width;
    WideDimension	height;

    /* If num_changed == 0, then this routine is being called due to a
     * touching of the item list or because of a relayout hint.  In this
     * case, just call a layout routine.  Else, update the information
     * array.
     */
    if (!num_changed)
    {
	Layout(w, &width, &height);

    } else
    {
	ExmFlatItem *		item;
	ExmFlatGraphInfo *	info;

	for (item = items; item < items + num_changed; item++)
	{
	    info = GET_INFO(w, FITEM(*item).item_index);

	    /* update these data because this routine can be called from
	     * ExmFlatSetValues
	     */
	    info->x		= FITEM(*item).x;
	    info->y		= FITEM(*item).y;
	    info->width		= FITEM(*item).width;
	    info->height	= FITEM(*item).height;

	    if (FITEM(*item).managed)
		info->flags |= ExmB_FG_MANAGED;
	    else
		info->flags &= ~ExmB_FG_MANAGED;

	    ExmFlatRefreshExpandedItem(w, *item, True);
	}

	CalcSize(w, &width, &height);
    }

    FPART(w).actual_width = width;
    FPART(w).actual_height = height;

    if (InSWin(w))			/* inside a scrolled window */
    {
	Dimension	req_wd = 0;
	Dimension	req_hi = 0;
	Boolean		offset_changed;

	if (FPART(w).relayout_hint & ExmFLAT_INIT)
	{
		req_wd = FPART(w).cols * FPART(w).col_width;;
		req_hi = FPART(w).rows * FPART(w).row_height;

			/* This can happen if num_items == 0 */
		if (width == 0)
			FPART(w).actual_width = width = req_wd;
		if (height == 0)
			FPART(w).actual_height = height = req_hi;
	}

	if (CHECK_UOMS(w)) {

		/* check special value `1' because it's possible that
		 * Layout() will return this value... */

		if (FPART(w).actual_width && FPART(w).actual_width != 1 &&
		    FPART(w).actual_width <= ExmFLAT_X_UOM(w)) {

			while (FPART(w).actual_width <= ExmFLAT_X_UOM(w)) {
				ExmFLAT_X_UOM(w) = ExmFLAT_X_UOM(w) / 2;
			}
		}

		if (FPART(w).actual_height && FPART(w).actual_height != 1 &&
		    FPART(w).actual_height <= ExmFLAT_Y_UOM(w)) {

			while (FPART(w).actual_height <= ExmFLAT_Y_UOM(w)) {
				ExmFLAT_Y_UOM(w) = ExmFLAT_Y_UOM(w) / 2;
			}
		}
	}

	offset_changed = ExmSWinCalcViewSize(w, FPART(w).hsb, FPART(w).vsb,

			    /* slider units in x/y direction */
			    ExmFLAT_X_UOM(w), ExmFLAT_Y_UOM(w),

			    /* Preferred width/height if non-zero */
			    req_wd, req_hi,

			    FPART(w).min_x, FPART(w).min_y,

			    /* current offset values */
			    &FPART(w).x_offset, &FPART(w).y_offset,

			    /* actual window size */
			    &width,	&height);

		/* We will have to force an expose if x_offset and/or
		 * y_offset are/is changed if (!num_changed && offset_changed).
		 * Should we check whether width/height are changed?
		 *
		 * Note that item_list is touched or will be re-layout'd
		 * if num_changed == 0. */
	if (num_changed && offset_changed) {
		XClearArea(
			XtDisplay(w), XtWindow(w), 0, 0, width, height, True);
	}
    }

    w->core.width = width;
    w->core.height = height;
}					/* END OF ChangeManaged() */

/****************************procedure*header*****************************
 * ClassInitialize -
 */
static void
ClassInitialize(void)
{
    ExmFlatInheritAll(ExmflatGraphWidgetClass);

#undef F
#define F	exmFlatGraphClassRec.flat_class
    F.change_managed	= ChangeManaged;
    F.item_set_values	= ItemSetValues;
    F.get_item_geometry	= GetItemGeometry;
    F.get_index		= GetIndex;
    F.geometry_handler	= GeometryHandler;
    F.initialize	= Initialize;
    F.refresh_item	= RefreshItem;
    F.traverse_items	= TraverseItems;
#undef F

}					/* END OF ClassInitialize() */

/****************************procedure*header*****************************
 * Destroy - frees memory allocated by the instance part
 */
static void
Destroy(Widget w)
{
    XtFree((XtPointer) GPART(w).info);

} /* end of Destroy() */

/****************************procedure*header*****************************
 * GeometryHandler - this procedure is called whenever an item wants to
 * change size.  It is called by ExmFlatMakeGeometryRequest.
 */
/* ARGSUSED */
static XtGeometryResult
GeometryHandler(Widget w, ExmFlatItem item,
		ExmFlatItemGeometry * request,
		ExmFlatItemGeometry * reply)
{
    XtGeometryResult	result = XtGeometryNo;
    Boolean		offset_changed = False;

    if (request->request_mode & XtCWQueryOnly)
    {
	result = XtGeometryYes;

    } else if (request->request_mode & (CWX|CWY|CWWidth|CWHeight))
    {
	WideDimension		width, height;
	Cardinal		i;
	ExmFlatGraphInfo	tmp;
	ExmFlatGraphInfo *	info=GetItemInfo(w, FITEM(item).item_index,&i);

	result = XtGeometryYes;

#define CHK_N_SET(i,r,b,f)	if (r->request_mode & b) i->f = r->f

	CHK_N_SET(info, request, CWX, x);
	CHK_N_SET(info, request, CWY, y);
	CHK_N_SET(info, request, CWWidth, width);
	CHK_N_SET(info, request, CWHeight, height);

#undef  CHK_N_SET

	    /* Put the item on the end of the list since we'll assume a moved
	     * item always goes on the top of the other items.
	     */
	tmp = *info;
	for ( ; i < (FPART(w).num_items - 1); ++i, ++info)
	{
	    *info = *(info + 1);
	}
	*info = tmp;

	CalcSize(w, &width, &height);

	FPART(w).actual_width = width;
	FPART(w).actual_height = height;

	if (InSWin(w))		/* inside a scrolled window */
	{
	    if (CHECK_UOMS(w)) {

		    if (FPART(w).actual_width && FPART(w).actual_width != 1 &&
			FPART(w).actual_width <= ExmFLAT_X_UOM(w)) {

			while (FPART(w).actual_width <= ExmFLAT_X_UOM(w)) {
				ExmFLAT_X_UOM(w) = ExmFLAT_X_UOM(w) / 2;
			}
		    }

		    if (FPART(w).actual_height && FPART(w).actual_height != 1 &&
			FPART(w).actual_height <= ExmFLAT_Y_UOM(w)) {

			while (FPART(w).actual_height <= ExmFLAT_Y_UOM(w)) {
				ExmFLAT_Y_UOM(w) = ExmFLAT_Y_UOM(w) / 2;
			}
		    }
	    }

	    FPART(w).in_geom_hdr = True;
	    offset_changed = ExmSWinCalcViewSize(w, FPART(w).hsb, FPART(w).vsb,

				/* slider units in x/y direction */
				ExmFLAT_X_UOM(w), ExmFLAT_Y_UOM(w),

				/* Preferred width/height are zero */
				(Dimension)0, (Dimension)0,

				FPART(w).min_x, FPART(w).min_y,

				/* current offset values */
				&FPART(w).x_offset, &FPART(w).y_offset,

				/* actual window size */
				&width, &height);
	    FPART(w).in_geom_hdr = False;
	}

	if ((width != w->core.width) || (height != w->core.height))
	{
#define MODE(r) ((r)->request_mode)
	    XtWidgetGeometry	p_request;
	    XtWidgetGeometry	p_reply;

	    MODE(&p_request) = 0;

	    if (width != w->core.width)
	    {
		MODE(&p_request) |= CWWidth;
		p_request.width   = width;
	    }

	    if (height != w->core.height)
	    {
		MODE(&p_request) |= CWHeight;
		p_request.height  = height;
	    }

	    if (XtMakeGeometryRequest(w, &p_request,
				      &p_reply) == XtGeometryAlmost )
	    {
		p_request = p_reply;
		(void)XtMakeGeometryRequest(w, &p_request, &p_reply);
	    }
	} else if (offset_changed)
	{
			/* We will have to force an expose if x_offset and/or
			 * y_offset are/is changed, but window size didn't
			 * change at all... */
		XClearArea(
			XtDisplay(w), XtWindow(w), 0, 0, width, height, True);
		result = XtGeometryDone;
	}
    }
    return(result);
}					/* END OF GeometryHandler() */

/****************************procedure*header*****************************
 * GetIndex - returns the index of the sub-object containing x & y
 */
static Cardinal
GetIndex(Widget w,			/* ExmFlat widget type	*/
	 register WidePosition x,	/* X source location	*/
	 register WidePosition y,	/* Y source location	*/
	 Boolean ignore_sensitivity)
{
    ExmFlatGraphInfoList	info;

    if (!FPART(w).num_items)
	return(ExmNO_ITEM);

    /* adjust (x, y) because we are dealing with virtual coords */
    x += FPART(w).x_offset;
    y += FPART(w).y_offset;

    /* Scan the list backwards since items on the end of the list are higher
     * in stacking order.
     */
    for (info = GPART(w).info + FPART(w).num_items - 1;
	 info >= GPART(w).info; info--)
    {
	if (info->x <= x && x < (info->x + (WidePosition)info->width)	&&
	    info->y <= y && y < (info->y + (WidePosition)info->height)	&&
	    (info->flags & ExmB_FG_MANAGED)				&&
	    (info->flags & ExmB_FG_MAPPED)				&&
	    (ignore_sensitivity || (info->flags & ExmB_FG_SENSITIVE)))
	{
	    return(info->item_index);
	}
    }

    return(ExmNO_ITEM);
}					/* END OF GetIndex() */

/****************************procedure*header*****************************
 * GetItemGeometry - initializes the ExmFlatDrawInfo structure.
 */
static void
GetItemGeometry(Widget w, Cardinal item_index,
		WidePosition * x, WidePosition * y,
		Dimension * width, Dimension * height)
{
    if (item_index >= FPART(w).num_items)
    {
	OlVaDisplayWarningMsg((XtDisplay(w),
			      OleNbadItemIndex, OleTflatState,
			      OleCOlToolkitWarning,
			      OleMbadItemIndex_flatState, XtName(w),
			      OlWidgetToClassName(w),
			      "GetItemGeometry", (unsigned)item_index));

	*x	= (WidePosition) 0;
	*y	= (WidePosition) 0;
	*width	= (Dimension) 0;
	*height	= (Dimension) 0;
    }
    else
    {
	ExmFlatGraphInfo *	info = GET_INFO(w, item_index);

	*x	= info->x;
	*y	= info->y;
	*width	= info->width;
	*height	= info->height;
    }
}					/* END OF GetItemGeometry() */

/*****************************procedure*header****************************
 * GetItemInfo -
 */
static ExmFlatGraphInfo *
GetItemInfo(Widget w,
	    register Cardinal item_index, register Cardinal * ret_index)
{
    register Cardinal			i;
    register ExmFlatGraphInfoList	info = GPART(w).info;

    for (i = 0; i < FPART(w).num_items; ++i, ++info)
    {
	if (item_index == info->item_index)
	{
	    if (ret_index)
	    {
		*ret_index = i;
	    }
	    return(info);
	}
    }
    return((ExmFlatGraphInfo *)NULL);
}					/* END OF GetItemInfo() */

/****************************procedure*header*****************************
 * Initialize - initializes the widget instance values
 */
/* ARGSUSED */
static void
Initialize(Widget request,	/* What we want		*/
	   Widget new,		/* What we get, so far..*/
	   ArgList args, Cardinal * num_args)
{
    GPART(new).info = (ExmFlatGraphInfoList)NULL;

    if (InSWin(new))
	ExmSWinSetupScrollbars(new, FPART(new).hsb, FPART(new).vsb,
			       ScrollbarChangedCB);

}					/* END OF Initialize() */

/****************************procedure*header*****************************
 * ItemSetValues -
 */
/* ARGSUSED */
static Boolean
ItemSetValues(Widget w,
	      ExmFlatItem current,	/* What we have		*/
	      ExmFlatItem request,	/* What we want		*/
	      ExmFlatItem new,		/* What we get, so far..*/
	      ArgList args, Cardinal * num_args)
{
    Boolean	redisplay = False;
	
#undef DIFFERENT
#define DIFFERENT(f)	(FITEM(new).f != FITEM(current).f)

    if (DIFFERENT(sensitive))
    {
	ExmFlatGraphInfo * info = GET_INFO(w, FITEM(new).item_index);

	redisplay = True;

	if (FITEM(new).sensitive)
	    info->flags |= ExmB_FG_SENSITIVE;
	else
	    info->flags &= ~ExmB_FG_SENSITIVE;
    }

    if (DIFFERENT(mapped_when_managed))
    {
	ExmFlatGraphInfo * info = GET_INFO(w, FITEM(new).item_index);

	redisplay = True;

	if (FITEM(new).mapped_when_managed)
	    info->flags |= ExmB_FG_MAPPED;
	else
	    info->flags &= ~ExmB_FG_MAPPED;
    }

    return(redisplay);
}					/* END OF ItemSetValues() */

/****************************procedure*header*****************************
 * Layout - figures out the container's size from the items.
 */
static void
Layout(Widget w, WideDimension * ret_w, WideDimension * ret_h)
{
    Boolean			sync_it;
    Cardinal			i;
    ExmFlatGraphInfoList	info;
    WideDimension		max_w;
    WideDimension		max_h;
    int				tmp;
    ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, item);

    /* Free the old information and set the widget's dimensions to its
     * minimum dimensions
     */
    XtFree((XtPointer) GPART(w).info);

    GPART(w).info = (ExmFlatGraphInfoList)NULL;

    if (FPART(w).num_items == (Cardinal)0)
    {
	ExmFLAT_FREE_ITEM(item);
	*ret_w = *ret_h = 0;
	return;
    }

    GPART(w).info = (ExmFlatGraphInfoList)
	XtMalloc(FPART(w).num_items * sizeof(ExmFlatGraphInfo));

    /* Initialize the information array	*/
    max_w = max_h = 0;
    sync_it = False;
    for (info = GPART(w).info, i=0; i < FPART(w).num_items; ++i, ++info)
    {
	ExmFlatExpandItem(w, i, item);

	if (FITEM(item).x == (WidePosition)ExmIGNORE)
	{
	    sync_it		= True;
	    FITEM(item).x	= 0;
	}

	if (FITEM(item).y == (WidePosition)ExmIGNORE)
	{
	    sync_it		= True;
	    FITEM(item).y	= 0;
	}

	/* If the height or width is not defined or it's zero, call the item
	 * dimensions routine since the XmNwidth and/or XmNheight resources
	 * may not have been specified in the application's itemFields list.
	 * Once we do this, we can then check the values of the dimensions.
	 */
	if ((FITEM(item).width == (Dimension)ExmIGNORE	||
	     FITEM(item).width == 0			||
	     FITEM(item).height == (Dimension)ExmIGNORE	||
	     FITEM(item).height == 0)			&&
	    FCLASS(w).item_dimensions)
	{
	    (*FCLASS(w).item_dimensions)(w, item, &FITEM(item).width,
					       &FITEM(item).height);
	}

	if (FITEM(item).width == (Dimension)ExmIGNORE ||
	    FITEM(item).width == 0)
	{
	    sync_it		= True;
	    FITEM(item).width	= 1;

	    OlVaDisplayWarningMsg((XtDisplay(w),
				  OleNinvalidDimension, OleTitemSize,
				  OleCOlToolkitWarning,
				  OleMinvalidDimension_itemSize,
				  XtName(w), OlWidgetToClassName(w),
				  FITEM(item).item_index, "width"));
	}

	if (FITEM(item).height == (Dimension)ExmIGNORE ||
	    FITEM(item).height == 0)
	{
	    sync_it		= True;
	    FITEM(item).height	= 1;

	    OlVaDisplayWarningMsg((XtDisplay(w),
				  OleNinvalidDimension, OleTitemSize,
				  OleCOlToolkitWarning,
				  OleMinvalidDimension_itemSize,
				  XtName(w), OlWidgetToClassName(w),
				  FITEM(item).item_index, "height"));
	}

	info->item_index	= i;
	info->x			= FITEM(item).x;
	info->y			= FITEM(item).y;
	info->width		= FITEM(item).width;
	info->height		= FITEM(item).height;
	info->flags		= 0;

#undef  SET_BIT
#define SET_BIT(i,in,f,b)	if (FITEM(i).f) in->flags |= b

	SET_BIT(item, info, managed, ExmB_FG_MANAGED);
	SET_BIT(item, info, mapped_when_managed, ExmB_FG_MAPPED);
	SET_BIT(item, info, sensitive, ExmB_FG_SENSITIVE);

	if ((info->flags & ExmB_FG_MANAGED) && (info->flags & ExmB_FG_MAPPED))
	{
	    if ((tmp = info->x + info->width) > 0 && max_w < tmp)
		max_w = tmp;
	    if ((tmp = info->y + info->height) > 0 && max_h < tmp)
		max_h = tmp;
	}

	if (sync_it)
	{
	    sync_it = False;
	    ExmFlatSyncItem(w, item);
	}
    }

    *ret_w = max_w;
    *ret_h = max_h;

    ExmFLAT_FREE_ITEM(item);
}					/* END OF Layout() */

/****************************procedure*header*****************************
 * PutItemInView-
 */
static void
PutItemInView(Widget w, ExmFlatGraphInfoList info)
{
#define X_CHANGED	(1L << 0)
#define Y_CHANGED	(1L << 1)
#define UPPER_X		(FPART(w).x_offset)
#define UPPER_Y		(FPART(w).y_offset)
#define LOWER_X		(UPPER_X + (WidePosition)CPART(w).width)
#define LOWER_Y		(UPPER_Y + (WidePosition)CPART(w).height)
#define ITEM_UX		(info->x)
#define ITEM_UY		(info->y)
#define ITEM_LX		(ITEM_UX + (WidePosition)info->width)
#define ITEM_LY		(ITEM_UY + (WidePosition)info->height)
#define IN_RANGE(L,M,H)	((L) < (M) && (M) < (H))

	Arg		args[1];
	Boolean		u_in, l_in, round_up;
	WidePosition	x_offset, y_offset;
	unsigned long	offset_changed = 0;

	int		tmp_val, max_val, slider_size, value;

	if (!InSWin(w))
		return;

#undef DEBUG
#ifdef DEBUG
 printf("item_index: %d\n", info->item_index);
#endif
    if (XtIsManaged(FPART(w).vsb)) { /* Determine new y_offset */

	u_in = IN_RANGE(UPPER_Y, ITEM_UY, LOWER_Y);
	l_in = IN_RANGE(UPPER_Y, ITEM_LY, LOWER_Y);

	if (u_in && l_in) {		/* in view */

		y_offset = FPART(w).y_offset;
		round_up = False;		/* no-op */
	} else if (info->height >= CPART(w).height) { /* taller than view hi */

		y_offset = ITEM_UY;
		round_up = True;
	} else if (!u_in && !l_in) {	/* completely out of view */

		if (ITEM_LY <= UPPER_Y) { 	       /* above view */

			y_offset = ITEM_UY;
			round_up = True;
		} else {			       /* below view */

			y_offset = FPART(w).y_offset + ITEM_UY -
						LOWER_Y + info->height;
			round_up = False;
		}
	} else if (u_in) {		/* upper corner in view but not lower */

		if ((tmp_val = LOWER_Y - ITEM_UY - info->height) < 0)
			tmp_val = -tmp_val;
		y_offset = FPART(w).y_offset + tmp_val;
		round_up = False;
	} else {			/* lower corner in view but not upper */
		y_offset = ITEM_UY;
		round_up = True;
	}
#ifdef DEBUG
 printf("\tVSB: u_in=%d, l_in=%d, round_up: %d, curr: %d, old: %d\n",
	u_in, l_in, round_up, y_offset, FPART(w).y_offset);
#endif

	if ((tmp_val = y_offset % (int)ExmFLAT_Y_UOM(w))) {
		if (!round_up)
			y_offset += ((int)ExmFLAT_Y_UOM(w) - tmp_val);
		else
			y_offset -= tmp_val;
	}

	if (y_offset != FPART(w).y_offset) {
#ifdef DEBUG
 printf("\tgot new y_offset -> %d vs %d\n", y_offset, FPART(w).y_offset);
#endif

		offset_changed |= Y_CHANGED;

		max_val = (int)FPART(w).actual_height / (int)ExmFLAT_Y_UOM(w);
		if ((int)FPART(w).actual_height % (int)ExmFLAT_Y_UOM(w))
			max_val++;

		slider_size	= (int)CPART(w).height / (int)ExmFLAT_Y_UOM(w);
		value		= (int)y_offset / (int)ExmFLAT_Y_UOM(w);

		if (value > (tmp_val = max_val - slider_size)) {
			value = tmp_val;
			y_offset = tmp_val * ExmFLAT_Y_UOM(w);
		}

		FPART(w).y_offset = y_offset;
		XtSetArg(args[0], XmNvalue, value);
		XtSetValues(FPART(w).vsb, args, 1);
	}
     }


    if (XtIsManaged(FPART(w).hsb)) { /* Determine new x_offset */

	u_in = IN_RANGE(UPPER_X, ITEM_UX, LOWER_X);
	l_in = IN_RANGE(UPPER_X, ITEM_LX, LOWER_X);

	if (u_in && l_in) {		/* in view */

		x_offset = FPART(w).x_offset;
		round_up = False;		/* no-op */
	} else if (info->width >= CPART(w).width) { /* wider than view width */

		x_offset = ITEM_UX;
		round_up = True;
	} else if (!u_in && !l_in) {	/* completely out of view */

		if (ITEM_LX <= UPPER_X) { 	     /* left view */

			x_offset = ITEM_UX;
			round_up = True;
		} else {			     /* right view */

			x_offset = FPART(w).x_offset + ITEM_UX -
						LOWER_X + info->width;
			round_up = False;
		}
	} else if (u_in) {		/* left corner in view but not right */

		if ((tmp_val = LOWER_X - ITEM_UX - info->width) < 0)
			tmp_val = -tmp_val;
		x_offset = FPART(w).x_offset + tmp_val;
		round_up = False;
	} else {			/* right corner in view but not left */

		x_offset = ITEM_UX;
		round_up = True;
	}
#ifdef DEBUG
 printf("\tHSB: u_in=%d, l_in=%d, round_up: %d, curr=%d, old=%d\n",
	u_in, l_in, round_up, x_offset, FPART(w).x_offset);
#endif

	if ((tmp_val = x_offset % (int)ExmFLAT_X_UOM(w))) {
		if (!round_up)
			x_offset += ((int)ExmFLAT_X_UOM(w) - tmp_val);
		else
			x_offset -= tmp_val;
	}

	if (x_offset != FPART(w).x_offset) {
#ifdef DEBUG
 printf("\tgot new x_offset -> %d vs %d\n", x_offset, FPART(w).x_offset);
#endif

		offset_changed |= X_CHANGED;

		max_val = (int)FPART(w).actual_width / (int)ExmFLAT_X_UOM(w);
		if ((int)FPART(w).actual_width % (int)ExmFLAT_X_UOM(w))
			max_val++;

		slider_size	= (int)CPART(w).width / (int)ExmFLAT_X_UOM(w);
		value		= (int)x_offset / (int)ExmFLAT_X_UOM(w);

		if (value > (tmp_val = max_val - slider_size)) {

			value = tmp_val;
			x_offset = tmp_val * ExmFLAT_X_UOM(w);
		}

		FPART(w).x_offset = x_offset;
		XtSetArg(args[0], XmNvalue, value);
		XtSetValues(FPART(w).hsb, args, 1);
	}
     }

		/* Force an expose if offset was changed */
	if (offset_changed)
		XClearArea(XtDisplay(w), XtWindow(w), 0, 0,
					CPART(w).width, CPART(w).height, True);

#undef X_CHANGED
#undef Y_CHANGED
#undef UPPER_X
#undef UPPER_Y
#undef LOWER_X
#undef LOWER_Y
#undef ITEM_UX
#undef ITEM_UY
#undef ITEM_LX
#undef ITEM_LY
#undef IN_RANGE
}

/****************************procedure*header*****************************
 * Redisplay - refreshes the container
 */
/* ARGSUSED2 */
static void
Redisplay(Widget w,			/* exposed widget	*/
	  XEvent * xevent,		/* the exposure		*/
	  Region region)		/* compressed exposures	*/
{
    ExmFlatGraphInfoList	info;
    ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, item);
    XEvent			this_event;

    if (!FCLASS(w).draw_item)
    {
	ExmFLAT_FREE_ITEM(item);
	return;
    }

    if (xevent->type == GraphicsExpose) {

	this_event.xexpose.x		= xevent->xgraphicsexpose.x;
	this_event.xexpose.y		= xevent->xgraphicsexpose.y;
	this_event.xexpose.width	= xevent->xgraphicsexpose.width;
	this_event.xexpose.height	= xevent->xgraphicsexpose.height;

	xevent = &this_event;
    }

    /* adjust (X, Y) because we are dealing with virtual coords but not di.x
     * and di.y, this should be done in draw_item procedure
     */
#define	X(e)	(WidePosition)(e->xexpose.x + FPART(w).x_offset)
#define	Y(e)	(WidePosition)(e->xexpose.y + FPART(w).y_offset)
#define	W(e)	(WidePosition)(e->xexpose.width)
#define	H(e)	(WidePosition)(e->xexpose.height)
#define EXPOSED(r,e)	\
    (((r->x >= X(e) &&  (r->x < (X(e) + W(e))))			||\
      (r->x <= X(e) && ((r->x + (WidePosition)r->width) > X(e))))	&&\
	 ((r->y >= Y(e) &&  (r->y < (Y(e) + H(e))))			||\
	  (r->y <= Y(e) && ((r->y + (WidePosition)r->height) > Y(e)))))

    for (info = GPART(w).info;
	 info < GPART(w).info + FPART(w).num_items; info++)
    {
	if (EXPOSED(info, xevent))
	{
	    ExmFlatExpandItem(w, info->item_index, item);

	    ExmFlatDrawExpandedItem(w, item);
	}
    }
    ExmFLAT_FREE_ITEM(item);

} /* END OF Redisplay() */

/****************************procedure*header*****************************
 * RefreshItem -
 */
static void
RefreshItem(Widget w, ExmFlatItem item, Boolean clear_area)
{
    WidePosition	x;
    WidePosition	y;
    Dimension		width;
    Dimension		height;
    XExposeEvent	exposure;

    if (!XtClass(w)->core_class.expose)
	return;

    ExmFlatGetItemGeometry(w, FITEM(item).item_index, &x, &y,
			   &width, &height);

    /* Adjust (x, y) because we are dealing with virtual coords */
    x -= FPART(w).x_offset;
    y -= FPART(w).y_offset;

    if (clear_area)
	(void)XClearArea(XtDisplay(w), XtWindow(w),
			 (int)x, (int)y,
			 (unsigned int)width, (unsigned int)height, (Bool)0);

    exposure.display	= XtDisplay(w);
    exposure.type	= Expose;
    exposure.serial	= (unsigned long) 0L;
    exposure.send_event	= (Bool) False;
    exposure.window	= XtWindow(w);
    exposure.x		= (int) x;
    exposure.y		= (int) y;
    exposure.width	= (int) width;
    exposure.height	= (int) height;
    exposure.count	= 0;

    XtDispatchEvent((XEvent *)&exposure);

}					/* END OF RefreshItem() */

/*************************************************************************
 * TraverseItems
 */
static Cardinal
TraverseItems(Widget w	,		/* ExmFlatWidget id */
	      Cardinal start_fi,	/* start focus item */
	      ExmTraverseDirType dir)	/* Direction to move */
{
    /* The purpose of these two macros are that we want to use the center of
     * each item to figure out what should be the new focus item. Using just
     * (x,y) is not good, because (x,y) is the upper left hand corner. The
     * assumption here is that an item is symmetrical in geometry.
     */
#define CALC_X(P)	((P)->x + (P)->width / 2) 
#define CALC_Y(P)	((P)->y + (P)->height / 2) 
#define MULTI		(1 << 0)
#define HORIZ		(1 << 1)
#define VERT		(1 << 2)
#define POSITIVE	(1 << 3)

#define ROWCOL_CLOSER	(1 << 0)
#define ANGLE		(1 << 1)
#define DIAG_CLOSER	(1 << 2)
#define QUADRANT	(1 << 3)
#define SAME_ROWCOL	(1 << 4)
    ExmFlatGraphInfoList	info;
    ExmFlatGraphInfoList	last_info;
    ExmFlatGraphInfoList	o_info;	/* info of starting item */
    WidePosition ox, oy;	/* position of starting item */
    int		dx, dy, dd;	/* distance from (ox, oy) to item */
    int		lx, ly, ld;	/* distance from the last candidate */
    int		da, la;		/* angle deviated from the intended direction */
    int		dq, lq = -1;	/* qualification flags */
    int		action = 0;
    Cardinal	i;


    switch(dir) {
    case ExmK_MULTILEFT:
	action = MULTI;
	/* FALLTHROUGH */
    case ExmK_MOVELEFT:
	action |= HORIZ;
	break;
    case ExmK_MULTIUP:
	action = MULTI;
	/* FALLTHROUGH */
    case ExmK_MOVEUP:
	action |= VERT;
	break;
    case ExmK_MULTIDOWN:
	action = MULTI;
	/* FALLTHROUGH */
    case ExmK_MOVEDOWN:
	action |= VERT | POSITIVE;
	break;
    case ExmK_MULTIRIGHT:
	action = MULTI;
	/* FALLTHROUGH */
    default:
	/* FALLTHROUGH */
    case ExmK_MOVERIGHT:
	action |= HORIZ | POSITIVE;
	break;
    }

    da = INT_MAX;
    if (action & MULTI)
	lx = ly = dd = 0;
    else {
	lx = ly = dd = INT_MAX;
    }

    if (start_fi != ExmNO_ITEM)
	i = start_fi;
    else
	i = 0;

    /*
     * Entries in info list is not in the same order as the item list.
     * So, get the index into the item list.
     */
    o_info = last_info = info = GetItemInfo(w, i, &i);
    ox = CALC_X(info);
    oy = CALC_Y(info);

    while (1) {
	/* bump the ptr */
	if (++i >= FPART(w).num_items) {
	    i = 0;
	    info = GPART(w).info;
	}
	else
	    info++;

	/* loop back to the starting entry */
	if (info->item_index == start_fi) {
	    break;
	}

	if ( ! (info->flags & ExmB_FG_MANAGED &&
		info->flags & ExmB_FG_MAPPED &&
		info->flags & ExmB_FG_SENSITIVE) )
	{
	    continue;
	}

	dx = CALC_X(info) - ox;
	dy = CALC_Y(info) - oy;
	dq = 0;

	/* make sure it is in the correct relative direction */
	if (action & HORIZ) {
	    if (action & POSITIVE) {
		if (dx <= 0)
		    continue;
	    }
	    else {
		if (dx >= 0)
		    continue;
	    }

	    /* make dx & dy absolute */
	    if (dx < 0) dx = -dx;
	    if (dy < 0) dy = -dy;

	    /* calculate diaganol distance and angle */
	    dd = dx * dx + dy * dy;
	    da = dy * 1000 / dx;

	    if (dx < (int)(o_info->width) / 2)
		continue;
	    /*
	      if (dx < (dy / 2))
	      continue;
	      */

	    if (dx >= dy)
		dq = QUADRANT;


	    if (dy < (int)(o_info->height) / 2)
		dq |= SAME_ROWCOL;
	    else if (da < la)
		dq |= ANGLE;

	    if (action & MULTI) {
		if (dd > ld) dq |= DIAG_CLOSER;
		if (dx > lx) dq |= ROWCOL_CLOSER;
	    }
	    else {
		if (dd < ld) dq |= DIAG_CLOSER;
		if (dx < lx) dq |= ROWCOL_CLOSER;
	    }
	}
	else {
	    if (action & POSITIVE) {
		if (dy <= 0)
		    continue;
	    }
	    else {
		if (dy >= 0)
		    continue;
	    }
	
	    /* make dx & dy absolute */
	    if (dx < 0) dx = -dx;
	    if (dy < 0) dy = -dy;

	    /* calculate diaganol distance and angle */
	    dd = dx * dx + dy * dy;
	    da = dx * 1000 / dy;

	    if (dy < (int)(o_info->height) / 2)
		continue;
	    /*
	      if (dy < (dx / 2))
	      continue;
	      */

	    if (dy >= dx)
		dq = QUADRANT;


	    if (dx < (int)(o_info->width) / 2)
		dq |= SAME_ROWCOL;
	    else if (da < la)
		dq |= ANGLE;

	    if (action & MULTI) {
		if (dd > ld) dq |= DIAG_CLOSER;
		if (dy > ly) dq |= ROWCOL_CLOSER;
	    }
	    else {
		if (dd < ld) dq |= DIAG_CLOSER;
		if (dy < ly) dq |= ROWCOL_CLOSER;
	    }
	}

	if (dq >= (lq | ((dq & ROWCOL_CLOSER) ? 0 : ROWCOL_CLOSER) |
		   ((dq & (DIAG_CLOSER | SAME_ROWCOL)) ? 0 : DIAG_CLOSER) |
		   ((dq & (ANGLE | SAME_ROWCOL)) ? 0 : ANGLE))) {
	    /* make it pass all the tests */
	    last_info = info;
	    lx = dx;
	    ly = dy;
	    ld = dd;
	    la = da;
	    lq = dq & ~(DIAG_CLOSER | ROWCOL_CLOSER | ANGLE);
	}
    }

    if (last_info->item_index != start_fi)
	(void)ExmFlatCallAcceptFocus(w, last_info->item_index);

    PutItemInView(w, last_info);
    return(last_info->item_index);
#undef CALC_X
#undef CALC_Y
#undef MULTI
#undef HORIZ
#undef VERT
#undef POSITIVE
#undef ROWCOL_CLOSER
#undef ANGLE
#undef DIAG_CLOSER
#undef QUADRANT
#undef SAME_ROWCOL
}				/* End of TraverseItems */

/****************************procedure*header*****************************
 *
 * Public Procedures
 */

/****************************procedure*header*****************************
 * ExmFlatRaiseExpandedItem-
 */
void	
ExmFlatRaiseExpandedItem(Widget w,	/* ExmFlatWidget id */
			 ExmFlatItem item,/* index of item to be raised */
			 Boolean expose)
{
    Cardinal		i;
    ExmFlatGraphInfo	tmp;
    ExmFlatGraphInfo *	info = GetItemInfo(w, FITEM(item).item_index, &i);

	/* This can happen at the creation time
	 * when exclusive == True && none_set == False!
	 * AnalyzeItems() tries to pick up an item, and indirectly
	 * triggered this routine. `info' is not created
	 * because ChangeManaged() is not called yet.
	 * In this case, we just return and let ChangeManaged()
	 * handles it, yuck!
	 */
    if (info == NULL)
	return;

    tmp = *info;
    for ( ; i < (FPART(w).num_items - 1); ++i, ++info)
    {
	*info = *(info + 1);
    }
    *info = tmp;


    if (expose)
	ExmFlatRefreshExpandedItem(w, item, True);

}				/* End of ExmFlatRaiseExpandedItem */

/****************************procedure*header*****************************
 * ExmFlatRaiseItem-
 */
void	
ExmFlatRaiseItem(Widget w,		/* ExmFlatWidget id */
		Cardinal item_index,	/* index of item to be raised */
		Boolean expose)
{
    ExmFLAT_ALLOC_ITEM(w, ExmFlatItem, item);

    ExmFlatExpandItem(w, item_index, item);
    ExmFlatRaiseExpandedItem(w, item, expose);

    ExmFLAT_FREE_ITEM(item);
}				/* End of ExmFlatRaiseItem */
