#ifndef	NOIDENT
#ident	"@(#)flat:FGraph.c	1.11"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the source code for the flattened graph
 *	widget.  This widget uses an arbitrary layout to place the
 *	sub-objects within it's container.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <limits.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/FGraphP.h>
#include <Xol/ScrolledWi.h>

#define ClassName FlatGraph
#include <Xol/NameDefs.h>


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define FPART(w)	(((FlatWidget)(w))->flat)
#define GPART(w)	(((FlatGraphWidget)(w))->graph)
#define ITEM(i)		(((FlatGraphItem)(i))->flat)

#define GET_INFO(w, i)	GetItemInfo(w, i, (Cardinal *)NULL)

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

static OlFlatGraphInfo * GetItemInfo OL_ARGS((Widget, Cardinal, Cardinal*));
static void	Layout OL_ARGS((Widget));
static void	CalcSize OL_ARGS((Widget, Dimension *, Dimension *));
static void	ViewSizeChanged OL_ARGS((Widget, OlSWGeometries *));
static void	PutItemInView OL_ARGS((Widget, OlFlatGraphInfoList));
static void	CalcViewSize OL_ARGS((Widget, OlSWGeometries *, Dimension *, Dimension *));

					/* class procedures		*/

static void	ClassInitialize OL_NO_ARGS();
static void	ChangeManaged OL_ARGS((Widget, FlatItem *, Cardinal));
static void	Destroy OL_ARGS((Widget));
static Cardinal	GetIndex OL_ARGS((Widget, Position, Position, Boolean));
static void	GetItemGeometry OL_ARGS((Widget, Cardinal, Position *,
				Position *, Dimension *, Dimension *));
static XtGeometryResult GeometryHandler OL_ARGS((Widget, FlatItem,
			OlFlatItemGeometry *, OlFlatItemGeometry *));
static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static Boolean	ItemSetValues OL_ARGS((Widget, FlatItem, FlatItem,
					FlatItem, ArgList, Cardinal *));
static void	Redisplay OL_ARGS((Widget, XEvent *, Region));
static void	RefreshItem OL_ARGS((Widget, FlatItem, Boolean));
static Cardinal	TraverseItems OL_ARGS((Widget, Cardinal, OlVirtualName,
				       Time));

					/* action procedures		*/

/* There are no action procedures	*/

					/* public procedures		*/
extern void	OlFlatRaiseItem OL_ARGS((Widget, Cardinal, Boolean));
extern void	OlFlatRaiseExpandedItem OL_ARGS((Widget, FlatItem, Boolean));

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatGraphClassRec
flatGraphClassRec = {
    {
	(WidgetClass)&flatClassRec,		/* superclass		*/
	"FlatGraph",				/* class_name		*/
	sizeof(FlatGraphRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	(Cardinal)0,				/* num_actions		*/
	NULL,					/* resources		*/
	(Cardinal)0,				/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	False,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	NULL,					/* resize		*/
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
	XtInheritTraversalHandler,		/* traversl_handler	*/
	NULL,					/* register_focus	*/
	XtInheritActivateFunc,			/* activate		*/
	NULL,					/* event_procs		*/
	(Cardinal)0,				/* num_event_procs	*/
	OlVersion,				/* version		*/
	(XtPointer)NULL,			/* extension		*/
	{
		(_OlDynResourceList)NULL,	/* resources		*/
		(Cardinal)0			/* num_resources	*/
	},					/* dyn_data		*/
	XtInheritTransparentProc		/* transparent_proc	*/
    },	/* End of Primitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	True,					/* transparent_bg	*/
	XtOffsetOf(FlatGraphRec, default_item),	/* default_offset	*/
	sizeof(FlatGraphItemRec),		/* rec_size		*/

		/*
		 * See ClassInitialize for procedures
		 */

    }, /* End of Flat Class Part Initialization */
    {
	NULL					/* no_class_fields	*/
    } /* End of FlatGraph Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatGraphWidgetClass = (WidgetClass) &flatGraphClassRec;

/*
 *************************************************************************
 *
 * Procedures
 *
 ****************************procedure*header*****************************
 */

/*
 *************************************************************************
 * ClassInitialize -
 ****************************procedure*header*****************************
 */
static void
ClassInitialize OL_NO_ARGS()
{
	OlFlatInheritAll(flatGraphWidgetClass);

#undef F
#define F	flatGraphClassRec.flat_class
	F.change_managed	= ChangeManaged;
        F.item_set_values	= ItemSetValues;
	F.get_item_geometry	= GetItemGeometry;
	F.get_index		= GetIndex;
	F.geometry_handler	= GeometryHandler;
	F.initialize		= Initialize;
	F.refresh_item		= RefreshItem;
	F.traverse_items	= TraverseItems;
#undef F

} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * Destroy - frees memory allocated by the instance part
 ****************************procedure*header*****************************
 */
static void
Destroy OLARGLIST((w))
	OLGRA( Widget,	w)
{
	XtFree((XtPointer) GPART(w).info);
} /* END OF Destroy() */

/*
 *************************************************************************
 * GetItemInfo -
 ****************************procedure*header*****************************
 */
static OlFlatGraphInfo *
GetItemInfo OLARGLIST((w, item_index, ret_index))
	OLARG( Widget,			w)
	OLARG( register Cardinal,	item_index)
	OLGRA( register Cardinal *,	ret_index)
{
	register Cardinal		i;
	register OlFlatGraphInfoList	info = GPART(w).info;

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
	return((OlFlatGraphInfo *)NULL);
} /* END OF GetItemInfo() */

/*
 *************************************************************************
 * GetItemGeometry - initializes the OlFlatDrawInfo structure.
 ****************************procedure*header*****************************
 */
static void
GetItemGeometry OLARGLIST((w, item_index, x, y, width, height))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLARG( Position *,	x)
	OLARG( Position *,	y)
	OLARG( Dimension *,	width)
	OLGRA( Dimension *,	height)
{
	if (item_index >= FPART(w).num_items)
	{
		OlVaDisplayWarningMsg(XtDisplay(w),
			OleNbadItemIndex, OleTflatState,
			OleCOlToolkitWarning,
			OleMbadItemIndex_flatState, XtName(w),
			OlWidgetToClassName(w),
			"GetItemGeometry", (unsigned)item_index);

		*x	= (Position) 0;
		*y	= (Position) 0;
		*width	= (Dimension) 0;
		*height	= (Dimension) 0;
	}
	else
	{
		OlFlatGraphInfo *	info = GET_INFO(w, item_index);

		*x	= info->x;
		*y	= info->y;
		*width	= info->width;
		*height	= info->height;
	}
} /* END OF GetItemGeometry() */

/*
 *************************************************************************
 * GetIndex - returns the index of the sub-object containing x & y
 ****************************procedure*header*****************************
 */
static Cardinal
GetIndex OLARGLIST((w, x, y, ignore_sensitivity))
	OLARG( Widget,			w)	/* Flat widget type	*/
	OLARG( register Position,	x)	/* X source location	*/
	OLARG( register Position,	y)	/* Y source location	*/
	OLGRA( Boolean,			ignore_sensitivity)
{
	Cardinal i = FPART(w).num_items;

		/* Scan the list backwards since items on the end of
		 * the list are higher in stacking order.		*/

	if (i)
	{
		OlFlatGraphInfoList	info;

		for (info = GPART(w).info + i - 1; i--; --info)
		{
			if (info->x <= x				&&
			    x < (info->x + (Position)info->width)	&&
			    info->y <= y				&&
			    y < (info->y + (Position)info->height)		&&

			    (info->flags & OL_B_FG_MANAGED)		&&
			    (info->flags & OL_B_FG_MAPPED)		&&
			    (ignore_sensitivity == True ||
				(info->flags & OL_B_FG_SENSITIVE)))
			{
				return(info->item_index);
			}
		}
	}
	return((Cardinal)OL_NO_ITEM);
} /* END OF GetIndex() */

/*
 *************************************************************************
 * Initialize - initializes the widget instance values
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
	Widget parent = XtParent(new);

	GPART(new).info = (OlFlatGraphInfoList)NULL;

	/*
	 * Check if it is created inside a scrolled window. If so,
	 * need to do certain things extra later on.
	 */
	if (XtIsSubclass(parent, scrolledWindowWidgetClass) == True) {
		Arg args[1];
		OlSWGeometries geom;

		XtSetArg(args[0], XtNcomputeGeometries, ViewSizeChanged);
		XtSetValues(parent, args, 1);

		geom = GetOlSWGeometries((ScrolledWindowWidget)parent);
		GPART(new).vsbar = geom.vsb;
		GPART(new).hsbar = geom.hsb;
	}
	else {
		GPART(new).vsbar = NULL;
		GPART(new).hsbar = NULL;
	}
} /* END OF Initialize() */

/*
 *************************************************************************
 * ItemSetValues -
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ItemSetValues OLARGLIST((w, current, request, new, args, num_args))
	OLARG( Widget,		w)
	OLARG( FlatItem,	current)	/* What we have		*/
	OLARG( FlatItem,	request)	/* What we want		*/
	OLARG( FlatItem,	new)		/* What we get, so far..*/
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	Boolean	redisplay = False;
	
#undef DIFFERENT
#define DIFFERENT(f)	(new->flat.f != current->flat.f)

	if (DIFFERENT(sensitive))
	{
		OlFlatGraphInfo * info = GET_INFO(w, new->flat.item_index);

		redisplay = True;

		if (new->flat.sensitive == True)
			info->flags |= OL_B_FG_SENSITIVE;
		else
			info->flags &= ~OL_B_FG_SENSITIVE;
	}

	if (DIFFERENT(mapped_when_managed))
	{
		OlFlatGraphInfo * info = GET_INFO(w, new->flat.item_index);

		redisplay = True;

		if (new->flat.mapped_when_managed == True)
			info->flags |= OL_B_FG_MAPPED;
		else
			info->flags &= ~OL_B_FG_MAPPED;
	}

	return(redisplay);

} /* END OF ItemSetValues() */

static void
CalcViewSize OLARGLIST((w, geom, width, height))
	OLARG( Widget, 		 w)
	OLARG( OlSWGeometries *, geom)
	OLARG( Dimension *,	 width)
	OLGRA( Dimension *,	 height)
{
#define VWIDTH_BAR(G)	((G)->sw_view_width - (G)->vsb_width)
	Dimension view_width, view_height;
	Boolean need_vsb, need_hsb;

	view_width  = geom->sw_view_width;
	view_height = geom->sw_view_height;
	if (geom->force_vsb == True || (*height > view_height))
		need_vsb = True;
	else
		need_vsb = False;

	if (geom->force_hsb == True || (*width > view_width))
		need_hsb = True;
	else {
		/* may still need hsb because of vsb */
		if ((need_vsb == True) &&
		    ((int)*width > (int)VWIDTH_BAR(geom)))
			need_hsb = True;
		else
			need_hsb = False;
	}

	if (need_hsb)
		view_height -= geom->hsb_height;
	if (need_vsb)
		view_width  -= geom->vsb_width;

	GPART(w).view_width  = view_width;
	GPART(w).view_height = view_height;

	if (*width < view_width)
		*width = view_width;
	if (*height < view_height)
		*height = view_height;
#undef VWIDTH_BAR
}

/*
 *************************************************************************
 * Layout - figures out the container's size from the items.
 ****************************procedure*header*****************************
 */
static void
Layout OLARGLIST((w))
	OLGRA( Widget,	w)
{
	Boolean			sync_it;
	Cardinal		i;
	OlFlatGraphInfoList	info;
	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);
	Dimension		max_w;
	Dimension		max_h;
	Dimension		tmp;


			/* Free the old information and set the widget's
			 * dimensions to its minimum dimensions		*/

	XtFree((XtPointer) GPART(w).info);

	GPART(w).info	= (OlFlatGraphInfoList)NULL;

	if (FPART(w).num_items == (Cardinal)0)
	{
		OL_FLAT_FREE_ITEM(item);
		return;
	}

	GPART(w).info = (OlFlatGraphInfoList) XtMalloc(FPART(w).num_items *
						sizeof(OlFlatGraphInfo));

				/* Initialize the information array	*/

	for (info = GPART(w).info, i=0, max_w=0, max_h = 0, sync_it = False;
	     i < FPART(w).num_items;
	     ++i, ++info)
	{
		OlFlatExpandItem(w, i, item);

		if (ITEM(item).x == (Position)OL_IGNORE)
		{
			sync_it		= True;
			ITEM(item).x	= 0;
		}

		if (ITEM(item).y == (Position)OL_IGNORE)
		{
			sync_it		= True;
			ITEM(item).y	= 0;
		}

			/* If the height or width is not defined or
			 * it's zero, call the item dimensions routine
			 * since the XtNwidth and/or XtNheight resources
			 * may not have been specified in the 
			 * application's itemFields list.o
			 * Once we do this, we can then check the values
			 * of the dimensions.				*/

		if ((ITEM(item).width == (Dimension)OL_IGNORE	||
		     ITEM(item).width == (Dimension)0		||
		     ITEM(item).height == (Dimension)OL_IGNORE	||
		     ITEM(item).height == (Dimension)0)		&&
		     OL_FLATCLASS(w).item_dimensions)
		{
			(*OL_FLATCLASS(w).item_dimensions)(w, item,
				&item->flat.width, &item->flat.height);
		}

		if (ITEM(item).width == (Dimension)OL_IGNORE ||
		    ITEM(item).width == (Dimension)0)
		{
			sync_it			= True;
			ITEM(item).width	= 1;

			OlVaDisplayWarningMsg(XtDisplay(w),
				OleNinvalidDimension, OleTitemSize,
				OleCOlToolkitWarning,
				OleMinvalidDimension_itemSize,
				XtName(w), OlWidgetToClassName(w),
				ITEM(item).item_index, "width");
		}

		if (ITEM(item).height == (Dimension)OL_IGNORE ||
		    ITEM(item).height == (Dimension)0)
		{
			sync_it			= True;
			ITEM(item).height	= 1;

			OlVaDisplayWarningMsg(XtDisplay(w),
				OleNinvalidDimension, OleTitemSize,
				OleCOlToolkitWarning,
				OleMinvalidDimension_itemSize,
				XtName(w), OlWidgetToClassName(w),
				ITEM(item).item_index, "height");
		}

		info->item_index	= i;
		info->x			= ITEM(item).x;
		info->y			= ITEM(item).y;
		info->width		= ITEM(item).width;
		info->height		= ITEM(item).height;
		info->flags		= (unsigned char)0;

#undef  SET_BIT
#define SET_BIT(i,in,f,b)	if (ITEM(i).f == True) in->flags |= b

		SET_BIT(item, info, managed, OL_B_FG_MANAGED);
		SET_BIT(item, info, mapped_when_managed, OL_B_FG_MAPPED);
		SET_BIT(item, info, sensitive, OL_B_FG_SENSITIVE);

		if ((info->flags & OL_B_FG_MANAGED) &&
		    (info->flags & OL_B_FG_MAPPED)) {
			if (max_w <= (tmp = (Dimension)info->x + info->width))
				max_w = tmp;
			if (max_h <= (tmp = (Dimension)info->y + info->height))
				max_h = tmp;
		}

		if (sync_it == True)
		{
			sync_it = False;
			OlFlatSyncItem(w, item);
		}
	}

	if (GPART(w).vsbar) {
		/* inside a scrolled window */
		OlSWGeometries geom;

		geom = GetOlSWGeometries((ScrolledWindowWidget)
					 XtParent(GPART(w).vsbar));
		CalcViewSize(w, &geom, &max_w, &max_h);
	}

	w->core.width = max_w;
	w->core.height = max_h;

	OL_FLAT_FREE_ITEM(item);
} /* END OF Layout() */

static void
CalcSize OLARGLIST((w, ret_width, ret_height))
	OLARG( Widget,		w)
	OLARG( Dimension *,	ret_width)
	OLGRA( Dimension *,	ret_height)
{
	int i;
	OlFlatGraphInfoList info;
	int cwidth=0, cheight=0;
	int x, y;

	for (info = GPART(w).info, i = FPART(w).num_items; i; --i, ++info)
		if ((info->flags & OL_B_FG_MANAGED) &&
		    (info->flags & OL_B_FG_MAPPED)) {
			x = info->x + info->width;
			y = info->y + info->height;

			if (x > cwidth)
				cwidth = x;
			if (y > cheight)
				cheight = y;
		}

	*ret_width  = (Dimension)cwidth;
	*ret_height = (Dimension)cheight;
}

static void
ViewSizeChanged OLARGLIST((w, geom))
	OLARG( Widget,		w)
	OLGRA( OlSWGeometries *,geom)
{
#define VWIDTH(G)	(((G)->force_vsb == False) ? (G)->sw_view_width : \
						     VWIDTH_BAR(G))
#define VHEIGHT(G)	(((G)->force_hsb == False) ? (G)->sw_view_height : \
						     VHEIGHT_BAR(G))
#define VWIDTH_BAR(G)	((G)->sw_view_width - (G)->vsb_width)
#define VHEIGHT_BAR(G)	((G)->sw_view_height - (G)->hsb_height)
	if (FPART(w).num_items) {
		Dimension width, height;

		CalcSize(w, &width, &height);
		CalcViewSize(w, geom, &width, &height);

		geom->bbc_width  = geom->bbc_real_width  = width;
		geom->bbc_height = geom->bbc_real_height = height;
	}
	else {
		/* at least make the size the same as the view */
		geom->bbc_width  = geom->bbc_real_width  = VWIDTH(geom);
		geom->bbc_height = geom->bbc_real_height = VHEIGHT(geom);
	}
#undef VWIDTH
#undef VHEIGHT
#undef VWIDTH_BAR
#undef VHEIGHT_BAR
}

static void
PutItemInView OLARGLIST((w, info))
	OLARG( Widget,			w)
	OLGRA( OlFlatGraphInfoList,	info)
{
#define IX	(-(w->core.x))
#define IY	(-(w->core.y))
#define IW	(int)(IX + GPART(w).view_width)
#define IH	(int)(IY + GPART(w).view_height)
#define NX	(info->x)
#define NY	(info->y)
#define NW	(int)(info->x + info->width)
#define NH	(int)(info->y + info->height)
#define INRANGE(L,m,H)	((L) <= (m) && (m) <= (H))
	Arg args[1];
	Position dx = 0, dy = 0;

	if (GPART(w).vsbar) {
		if (info->height >= GPART(w).view_height) {
			/*
			 * The entire item will not fit into the view.
			 * If item is not even partially in view, then
			 * get the top edge of the item into view.
			 */
			if ((NY > IH) && (NH < IY)) {
				dy = NY - IY;
			}
			else
				/* Do nothing. */
				dy = 0;
		}
		else if (NH > IH) {
			/* below view */
			dy = IH - NH;
			if ((int)(IH + dy) > (int)(w->core.height))
				dy = -IH;
			else if ((int)(w->core.height - NH) <
				 (int)(info->height / 2))
				dy = IH - w->core.height;
		}
		else if ( NY < IY) {
			/* above view */
			dy = IY - NY;
			if ((w->core.y + dy) > 0)
				dy = -(w->core.y);
			else if ((int)(-(w->core.y + dy)) <
				 (int)(info->height / 2))
				dy = -(w->core.y);
		}

		if (dy) {
			/* Not in view, move it into view */
    			XtSetArg(args[0], XtNsliderValue, -(w->core.y + dy));
    			XtSetValues(GPART(w).vsbar, args, 1);
		}
	}

	if (GPART(w).hsbar) {
		if (info->width >= GPART(w).view_width) {
			/*
			 * The entire item will not fit into the view.
			 * If item is not even partially in view, then
			 * get the left edge of the item into view.
			 */
			if ((NX > IW) && (NW < IX)) {
				dx = NX - IX;
			}
			else
				/* Do nothing. */
				dx = 0;
		}
		else if (NW > IW) {
			/* to the right of view */
			dx = IW - NW;
			if ((int)(IW + dx) > (int)(w->core.width))
				dx = -IW;
			else if ((int)(w->core.width - NW) <
				 (int)(info->width / 2))
				dx = IW - w->core.width;
		}
		else if ( NX < IX) {
			/* to the left of view */
			dx = IX - NX;
			if ((w->core.x + dx) > 0)
				dx = -(w->core.x);
			else if ((int)(-(w->core.x + dx)) <
				 (int)(info->width / 2))
				dx = -(w->core.x);
		}

		if (dx) {
			/* Not in view, move it into view */
    			XtSetArg(args[0], XtNsliderValue, -(w->core.x + dx));
    			XtSetValues(GPART(w).hsbar, args, 1);
		}
	}

	if (dx || dy)
		XtMoveWidget(w, w->core.x + dx, w->core.y + dy);
#undef INRANGE
#undef IX
#undef IY
#undef IW
#undef IH
#undef NX
#undef NY
#undef NW
#undef NH
}

/*
 *************************************************************************
 * ChangeManaged - this routine is called whenever one or more flat items
 * change their managed state.  It's also called when the items get
 * touched or when a relayout hint was suggested.
 ****************************procedure*header*****************************
 */
static void
ChangeManaged OLARGLIST((w, items, num_changed))
	OLARG( Widget, 		w)
	OLARG( FlatItem *,	items)
	OLGRA( Cardinal,	num_changed)
{
			/* If num_changed == 0, then this routine is being
			 * called due to a touching of the item list or
			 * because of a relayout hint.  In this case,
			 * just call a layout routine.
			 * Else, update the information array.		*/

	if (!num_changed)
	{
		Layout(w);
	}
	else
	{
		Cardinal		i;
		OlFlatGraphInfo *	info;
		Dimension		width;
		Dimension		height;

		for (i = 0; i < num_changed; ++i)
		{

			info = GET_INFO(w, items[i]->flat.item_index);

				/* update these data because this	*/
				/* routine can be called from		*/
				/* OlFlatSetValues			*/
			info->x			= items[i]->flat.x;
			info->y			= items[i]->flat.y;
			info->width		= items[i]->flat.width;
			info->height		= items[i]->flat.height;

			if (items[i]->flat.managed == True)
			{
				info->flags |= OL_B_FG_MANAGED;
			}
			else
			{
				info->flags &= ~OL_B_FG_MANAGED;
			}

			OlFlatRefreshExpandedItem(w, items[i], True);
		}

		CalcSize(w, &width, &height);
		if (GPART(w).vsbar) {
			/* inside a scrolled window */
			OlSWGeometries geom;

			geom = GetOlSWGeometries((ScrolledWindowWidget)
					 	XtParent(GPART(w).vsbar));
			CalcViewSize(w, &geom, &width, &height);
		}
		w->core.width = width;
		w->core.height = height;
	}
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
#undef  CHK_N_SET
#define CHK_N_SET(i,r,b,f)	if (r->request_mode & b) i->f = r->f

	XtGeometryResult result = XtGeometryNo;

	if (request->request_mode & XtCWQueryOnly)
	{
		result = XtGeometryYes;
	}
	else if (request->request_mode & (CWX|CWY|CWWidth|CWHeight))
	{
		Cardinal		i;
		OlFlatGraphInfo		tmp;
		OlFlatGraphInfo *	info = GetItemInfo(w,
						item->flat.item_index, &i);
		Dimension		width, height;

		result = XtGeometryYes;

		CHK_N_SET(info, request, CWX, x);
		CHK_N_SET(info, request, CWY, y);
		CHK_N_SET(info, request, CWWidth, width);
		CHK_N_SET(info, request, CWHeight, height);

			/* Put the item on the end of the list since we'll
			 * assume a moved item always goes on the top
			 * of the other items.				*/

                tmp = *info;
		for ( ; i < (FPART(w).num_items - 1); ++i, ++info)
		{
			*info = *(info + 1);
		}
		*info = tmp;

		CalcSize(w, &width, &height);
		if (GPART(w).vsbar) {
			/* inside a scrolled window */
			OlSWGeometries geom;

			geom = GetOlSWGeometries((ScrolledWindowWidget)
					 	XtParent(GPART(w).vsbar));
			CalcViewSize(w, &geom, &width, &height);
		}

		if ((width != w->core.width) ||
		    (height != w->core.height)) {
#define MODE(r) ((r)->request_mode)
			Boolean			resize = False;
			XtGeometryResult	p_result;
			XtWidgetGeometry	p_request;
			XtWidgetGeometry	p_reply;

			MODE(&p_request) = 0;

			if (width != w->core.width) {
				MODE(&p_request) |= CWWidth;
				p_request.width   = width;
			}

			if (height != w->core.height) {
				MODE(&p_request) |= CWHeight;
				p_request.height  = height;
			}

			if ((p_result = XtMakeGeometryRequest(w, &p_request,
					&p_reply)) == XtGeometryAlmost)
			{
				resize = True;
				p_request = p_reply;
				p_result = XtMakeGeometryRequest(w, &p_request,
							&p_reply);
			}
		}
	}
	return(result);
} /* END OF GeometryHandler() */

/*
 *************************************************************************
 * Redisplay - refreshes the container
 ****************************procedure*header*****************************
 */
/* ARGSUSED2 */
static void
Redisplay OLARGLIST((w, xevent, region))
	OLARG( Widget,		w)		/* exposed widget	*/
	OLARG( XEvent *,	xevent)		/* the exposure		*/
	OLGRA( Region,		region)		/* compressed exposures	*/
{
	Cardinal		i;
	OlFlatGraphInfoList	info;
	OlFlatDrawInfo		di;
	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

	if (!OL_FLATCLASS(w).draw_item)
	{
		return;
	}

	di.drawable	= (Drawable) XtWindow(w);
	di.screen	= XtScreen(w);
	di.background	= w->core.background_pixel;

#define	X(e)	(Position)e->xexpose.x
#define	Y(e)	(Position)e->xexpose.y
#define	W(e)	(Position)e->xexpose.width
#define	H(e)	(Position)e->xexpose.height
#define EXPOSED(r,e)	\
	(((r->x >= X(e) &&  (r->x < (X(e) + W(e))))			||\
	  (r->x <= X(e) && ((r->x + (Position)r->width) > X(e))))	&&\
	 ((r->y >= Y(e) &&  (r->y < (Y(e) + H(e))))			||\
	  (r->y <= Y(e) && ((r->y + (Position)r->height) > Y(e)))))

	for (info = GPART(w).info, i = FPART(w).num_items; i; --i, ++info)
	{
		if (EXPOSED(info, xevent))
		{
			OlFlatExpandItem(w, info->item_index, item);

			if (item->flat.managed == True &&
			    item->flat.mapped_when_managed == True)
			{
				di.x		= info->x;
				di.y		= info->y;
				di.width	= info->width;
				di.height	= info->height;

				(*OL_FLATCLASS(w).draw_item)(w, item, &di);
			}
		}
	}
	OL_FLAT_FREE_ITEM(item);
} /* END OF Redisplay() */

/*
 *************************************************************************
 * RefreshItem -
 ****************************procedure*header*****************************
 */
static void
RefreshItem OLARGLIST((w, item, clear_area))
	OLARG( Widget,		w)
	OLARG( FlatItem,	item)
	OLGRA( Boolean,		clear_area)
{
	if (XtClass(w)->core_class.expose)
	{
		Position	x;
		Position	y;
		Dimension	width;
		Dimension	height;
		XExposeEvent	exposure;

		OlFlatGetItemGeometry(w, item->flat.item_index, &x, &y,
					&width, &height);

		if (clear_area == True)
		{
			(void)XClearArea(XtDisplay(w), XtWindow(w),
				(int)x, (int)y, (unsigned int)width,
				(unsigned int)height, (Bool)0);
		}


		exposure.display	= XtDisplay(w);
		exposure.type		= Expose;
		exposure.serial		= (unsigned long) 0;
		exposure.send_event	= (Bool) False;
		exposure.window		= (Window) XtWindow(w);
		exposure.x		= (int) x;
		exposure.y		= (int) y;
		exposure.width		= (int) width;
		exposure.height		= (int) height;
		exposure.count		= (int) 0;

		XtDispatchEvent((XEvent *)&exposure);
	}
} /* END OF RefreshItem() */

/*
 *************************************************************************
 * TraverseItems
 *************************************************************************
 */
static Cardinal
TraverseItems OLARGLIST((w, start_fi, dir, time))
    OLARG( Widget,		w)		/* FlatWidget id */
    OLARG( Cardinal,		start_fi)	/* start focus item */
    OLARG( OlVirtualName,	dir)		/* Direction to move */
    OLGRA( Time,		time)		/* Time of move (ignored) */
{
/*
 * The purpose of these two macros are that we want to use the center of each
 * item to figure out what should be the new focus item. Using just (x,y) is
 * not good, because (x,y) is the upper left hand corner. The assumption here
 * is that an item is symmetrical in geometry.
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
	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);
	OlFlatGraphInfoList	info;
	OlFlatGraphInfoList	last_info;
	OlFlatGraphInfoList	o_info; /* info of starting item */
	Position ox, oy;	/* position of starting item */
	int dx, dy, dd;		/* distance from (ox, oy) to item */
	int lx, ly, ld;		/* distance from the last candidate */
	int da, la;		/* angle deviated from the intended direction */
	int dq, lq = -1;	/* qualification flags */
	int action = 0;
	Cardinal i;

	switch(dir) {
	case OL_MULTILEFT:
		action = MULTI;
		/* falls through */
	case OL_MOVELEFT:
		action |= HORIZ;
		break;
	case OL_MULTIUP:
		action = MULTI;
		/* falls through */
	case OL_MOVEUP:
		action |= VERT;
		break;
	case OL_MULTIDOWN:
		action = MULTI;
		/* falls through */
	case OL_MOVEDOWN:
		action |= VERT | POSITIVE;
		break;
	case OL_MULTIRIGHT:
		action = MULTI;
		/* falls through */
	default:
		/* falls through */
	case OL_MOVERIGHT:
		action |= HORIZ | POSITIVE;
		break;
	}

	da = INT_MAX;
	if (action & MULTI)
		lx = ly = dd = 0;
	else {
		lx = ly = dd = INT_MAX;
	}

	if (start_fi != (Cardinal)OL_NO_ITEM)
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
		if (++i > FPART(w).num_items) {
			i = 0;
			info = GPART(w).info;
		}
		else
			info++;

		/* loop back to the starting entry */
		if (info->item_index == start_fi)
			break;

		if ((info->flags & OL_B_FG_MANAGED) &&
		    (info->flags & OL_B_FG_MAPPED) &&
		    (info->flags & OL_B_FG_SENSITIVE))
			; /* ok */
		else
			continue;

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

	if (last_info->item_index != start_fi) {
		Time	timestamp = time;

		OlFlatExpandItem(w, last_info->item_index, item);
		(*OL_FLATCLASS(w).item_accept_focus)(w, item, &timestamp);
		OL_FLAT_FREE_ITEM(item);
	}

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
}

/*
 * Public routines
 */

void	
OlFlatRaiseExpandedItem OLARGLIST((w, item, expose))
    OLARG( Widget,	w)	/* FlatWidget id */
    OLARG( FlatItem,	item)	/* index of item to be raised */
    OLGRA( Boolean,	expose)
{
	Cardinal i;
	OlFlatGraphInfo		tmp;
	OlFlatGraphInfo *	info = GetItemInfo(w, item->flat.item_index,
					 &i);

#ifdef NOT_USE
	/* save the info first */
	tmp = *info;

	/* shift up the rest of the list */
	OlMemMove(sizeof(OlFlatGraphInfo),
		  (char *)info, (char *)(info + 1),
		  FPART(w).num_items - (info - GPART(w).info) - 1);

	/* put the saved info at the end of the list */
	*(GPART(w).info + FPART(w).num_items - 1) = tmp;

#endif
                tmp = *info;
		for ( ; i < (FPART(w).num_items - 1); ++i, ++info)
		{
			*info = *(info + 1);
		}
		*info = tmp;


	if (expose)
		OlFlatRefreshExpandedItem(w, item, True);
}

void	
OlFlatRaiseItem OLARGLIST((w, item_index, expose))
    OLARG( Widget,	w)		/* FlatWidget id */
    OLARG( Cardinal,	item_index)	/* index of item to be raised */
    OLGRA( Boolean,	expose)
{
	OL_FLAT_ALLOC_ITEM(w, FlatItem, item);

	OlFlatExpandItem(w, item_index, item);
	OlFlatRaiseExpandedItem(w, item, expose);

	OL_FLAT_FREE_ITEM(item);
}
