#pragma ident	"@(#)libDtI:FIconBox.c	1.49"

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the source code for the flat iconBox
 *	widget.
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/Dynamic.h>
#include <Xol/Error.h>
#include <Xol/OlCursors.h>
#include "FIconBoxP.h"

#define ClassName FlatIconBox
#include <Xol/NameDefs.h>

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define CPART(w)		(((FlatIconBoxWidget)(w))->core)
#define PPART(w)		(((FlatIconBoxWidget)(w))->primitive)
#define FPART(w)		(((FlatIconBoxWidget)(w))->flat)
#define GPART(w)		(((FlatIconBoxWidget)(w))->graph)
#define IPART(w)		(((FlatIconBoxWidget)(w))->iconBox)
#define FITEM(i)		(((FlatIconBoxItem)(i))->flat)
#define GITEM(i)		(((FlatIconBoxItem)(i))->graph)
#define IITEM(i)		(((FlatIconBoxItem)(i))->iconBox)
#define BUTTON(vevent)		(ve->xevent->xbutton)
#define ISTRUE(B)		(((B) != False) ? 1 : 0)
#define ACTION_IDX(B2, B1, B0)  ((ISTRUE(B2) << 2) + (ISTRUE(B1) << 1) + \
				 ISTRUE(B0))
#define SWAP(A, B, TMP)		{ TMP=A; A=B; B=TMP; }

#define CHK_CNT		(1 << 0)
#define CHK_OK		(1 << 1)
#define RESET		(1 << 2)
#define INC		(1 << 3)
#define DEC		(1 << 4)
#define ONE		(1 << 5)
#define RAISE		(1 << 6)

static char set_actions[] = {
/* new value	noneset	exclusives */
/* N		N	N	   */	CHK_CNT | CHK_OK | DEC,
/* N		N	Y	   */	0,
/* N		Y	N	   */	CHK_OK  | DEC,
/* N		Y	Y	   */	CHK_OK  | RESET  | DEC,
/* Y		N	N	   */	CHK_OK  | INC | RAISE,
/* Y		N	Y	   */	CHK_OK  | RESET  | ONE | RAISE,
/* Y		Y	N	   */	CHK_OK  | INC | RAISE,
/* Y		Y	Y	   */	CHK_OK  | RESET  | ONE | RAISE,
};

/* return value of CreateCursor */
#define FREE_NO_CURSOR	(1 << 0)
#define FREE_YES_CURSOR	(1 << 1)

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

static Boolean	CallButtonCallbacks OL_ARGS((Widget, Cardinal,
					     XtCallbackProc, int, int,
					     Position, Position));
static void	HandleDrop OL_ARGS((Widget, OlVirtualEvent,
			   OlDnDDropStatus, OlDnDDestinationInfoPtr,
			   OlDnDDragDropInfoPtr, OlFlatDragCursorCallData *));
static void	MoveIcon OL_ARGS((Widget, Cardinal, Position, Position,
			   OlFlatDragCursorCallData *));
static void	_OlFlatLayoutIconBox OL_ARGS((Widget, Dimension, Dimension));
static XtPointer
		GetItemData OL_ARGS((Widget, OlFlatCallData *, Cardinal));
static void	TrackBoundingBox OL_ARGS((Widget, OlVirtualEvent));
static void	HandleAdjust OL_ARGS((Widget, Cardinal, OlVirtualName,
					 Position, Position));
static void	HandleSelect OL_ARGS((Widget, Cardinal, OlVirtualName,
					 Position, Position));


					/* class procedures		*/

static void	ClassInitialize OL_NO_ARGS();
static void	Realize OL_ARGS((Widget, Mask *, XSetWindowAttributes *));
static void	Destroy OL_ARGS((Widget));
static void	DrawItem OL_ARGS((Widget, FlatItem, OlFlatDrawInfo *));
static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));
static void	ItemDimensions OL_ARGS((Widget, FlatItem, Dimension *,
					Dimension *));
static Boolean  ItemSetValues OL_ARGS((Widget, FlatItem, FlatItem,
					FlatItem, ArgList, Cardinal *));
static Boolean	ItemActivate OL_ARGS((Widget, FlatItem, OlVirtualName,
					 XtPointer));
static Boolean	SetValues OL_ARGS((Widget, Widget, Widget, ArgList,
					Cardinal *));

					/* action procedures		*/

static void	ButtonHandler OL_ARGS((Widget, OlVirtualEvent));

					/* public procedures		*/

void	OlFlatIconBoxSnapGrid OL_ARGS((Widget, Dimension));

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

static OlEventHandlerRec event_procs[] = {
	{ ButtonPress,		ButtonHandler	},
}; 

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#undef  OFFSET
#define OFFSET(f)	XtOffsetOf(FlatIconBoxRec, iconBox.f)

static XtResource
resources[] = {
	{ XtNmovableIcons, XtCMovableIcons, XtRBoolean, sizeof(Boolean),
	  OFFSET(movable), XtRImmediate, (XtPointer)True },

	{ XtNtriggerMsgProc, XtCTriggerMsgProc, XtRPointer, sizeof(XtPointer),
	  OFFSET(trigger_proc), XtRImmediate, (XtPointer)NULL },

	{ XtNselectCount, XtCSelectCount, XtRInt, sizeof(int),
	  OFFSET(select_count), XtRImmediate, (XtPointer)0 },

	{ XtNlastSelectItem, XtCLastSelectItem, XtRInt, sizeof(int),
	  OFFSET(last_select), XtRImmediate, (XtPointer)OL_NO_ITEM },

	{ XtNdropProc, XtCCallbackProc, XtRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(drop_proc),
	  XtRCallbackProc, (XtPointer) NULL },

	{ XtNselectProc, XtCCallbackProc, XtRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(select1_proc),
	  XtRCallbackProc, (XtPointer) NULL},

	{ XtNpostSelectProc, XtCCallbackProc, XtRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(post_select1_proc),
	  XtRCallbackProc, (XtPointer) NULL},

	{ XtNdblSelectProc, XtCCallbackProc, XtRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(select2_proc),
	  XtRCallbackProc, (XtPointer) NULL },

	{ XtNadjustProc, XtCCallbackProc, XtRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(adjust_proc),
	  XtRCallbackProc, (XtPointer) NULL },

	{ XtNpostAdjustProc, XtCCallbackProc, XtRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(post_adjust_proc),
	  XtRCallbackProc, (XtPointer) NULL },

	{ XtNmenuProc, XtCCallbackProc, XtRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(menu_proc),
	  XtRCallbackProc, (XtPointer) NULL },

	{ XtNdrawProc, XtCCallbackProc, XtRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(draw_proc),
	  XtRCallbackProc, (XtPointer) NULL },

	{ XtNdragCursorProc, XtCCallbackProc, XtRCallbackProc,
	  sizeof(XtCallbackProc), OFFSET(cursor_proc),
	  XtRCallbackProc, (XtPointer) NULL },

	{ XtNexclusives, XtCExclusives,
	  XtRBoolean, sizeof(Boolean),
	  OFFSET(exclusives), XtRImmediate, (XtPointer)False },

	{ XtNnoneSet, XtCNoneSet,
	  XtRBoolean, sizeof(Boolean),
	  OFFSET(noneset), XtRImmediate, (XtPointer)True },

	{ XtNdropSiteID, XtCDropSiteID, XtRPointer,
	  sizeof(XtPointer), OFFSET(drop_site_id),
	  XtRImmediate, (XtPointer) NULL },

};
				/* Define Resources for sub-objects	*/

#undef  OFFSET
#define OFFSET(f)	XtOffsetOf(FlatIconBoxItemRec, iconBox.f)

static const Boolean def_false = False;

static XtResource
item_resources[] = {
	{ XtNset, XtCSet, XtRBoolean, sizeof(Boolean),
	  OFFSET(set), XtRBoolean, (XtPointer) &def_false },

	{ XtNbusy, XtCBusy, XtRBoolean, sizeof(Boolean),
	  OFFSET(busy), XtRBoolean, (XtPointer) &def_false },

	{ XtNobjectData, XtCObjectData, XtRPointer, sizeof(XtPointer),
	  OFFSET(object_data), XtRPointer, (XtPointer) NULL },

	{ XtNclientData, XtCClientData, XtRPointer, sizeof(XtPointer),
	  OFFSET(client_data), XtRPointer, (XtPointer) NULL },

};

	/* Specify resources that we want the flat class to manage
	 * internally if the application doesn't put them in their
	 * item fields list.						*/

static OlFlatReqRsc
required_resources[] = {
	{ XtNset	}
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatIconBoxClassRec
flatIconBoxClassRec = {
    {
	(WidgetClass)&flatGraphClassRec,	/* superclass		*/
	"FlatIconBox",				/* class_name		*/
	sizeof(FlatIconBoxRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	NULL,					/* initialize_hook	*/
	Realize,				/* realize		*/
	NULL,					/* actions		*/
	(Cardinal)0,				/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	FALSE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	NULL,					/* resize		*/
	XtInheritExpose,			/* expose		*/
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
        False,					/* focus_on_select	*/
	XtInheritHighlightHandler,		/* highlight_handler	*/
	XtInheritTraversalHandler,		/* traversl_handler	*/
	NULL,					/* register_focus	*/
	XtInheritActivateFunc,			/* activate		*/
	event_procs,				/* event_procs		*/
	XtNumber(event_procs),			/* num_event_procs	*/
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
	XtOffsetOf(FlatIconBoxRec, default_item),/* default_offset	*/
	sizeof(FlatIconBoxItemRec),		/* rec_size		*/
	item_resources,				/* item_resources	*/
	XtNumber(item_resources),		/* num_item_resources	*/
	required_resources,			/* required_resources	*/
	XtNumber(required_resources),		/* num_required_resources*/

		/*
		 * See ClassInitialize for procedures
		 */

    }, /* End of Flat Class Part Initialization */
    {
	NULL					/* no class fields	*/
    }, /* End of FlatGraph Class Part Initialization */
    {
	NULL					/* no_class_fields	*/
    } /* End of FlatIconBox Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatIconBoxWidgetClass = (WidgetClass) &flatIconBoxClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

static XtPointer
GetItemData OLARGLIST((w, item_data, item_index))
	OLARG( Widget,			w)
	OLARG( OlFlatCallData *,	item_data)
	OLGRA( Cardinal,		item_index)
{
	XtPointer	user_data;
	XtPointer	client_data;
	Arg		args[2];

	XtSetArg(args[0], XtNuserData, &user_data);
	XtSetArg(args[1], XtNclientData, &client_data);
	OlFlatGetValues(w, item_index, args, 2);

	item_data->item_index		= item_index;
	item_data->items		= FPART(w).items;
	item_data->num_items		= FPART(w).num_items;
	item_data->item_fields		= FPART(w).item_fields;
	item_data->num_item_fields	= FPART(w).num_item_fields;
	item_data->user_data		= PPART(w).user_data;
	item_data->item_user_data	= user_data;

	/* kai, what about client_data? */
	return(client_data);
}

static void
HandleMultiSelect OLARGLIST((widget, type, x, y, w, h))
	OLARG( Widget,		widget)
	OLARG( OlVirtualName,	type)
	OLARG( Position,	x)
	OLARG( Position,	y)
	OLARG( Dimension,	w)
	OLGRA( Dimension,	h)
{
	OlFlatGraphInfoList info;
	int call_select;
	int i;
	int idx;
	Arg args[1];
	Boolean selected;

	XtSetArg(args[0], XtNset, &selected);

	if (type == OL_SELECT)
		call_select = 1;
	else
		call_select = 0;
	for (idx=0; idx < FPART(widget).num_items; idx++) {
		/* find the entry in the cache */
		for (info=GPART(widget).info,i=FPART(widget).num_items; i;
			 i--,info++)
			if (idx == info->item_index)
				break;

		if (!i)
			/* couldn't find an entry in the cache? */
			continue;

		if ((info->x >= x) && (info->y >= y) &&
		    ((int)(info->x + info->width)  <= (int)(x + w)) &&
		    ((int)(info->y + info->height) <= (int)(y + h))) {
			if ((info->flags & OL_B_FG_MANAGED) &&
			    (info->flags & OL_B_FG_MAPPED) &&
			    (info->flags & OL_B_FG_SENSITIVE)) {
			    if (type == OLM_KDeselectAll) {
				OlFlatGetValues(widget,info->item_index,args,1);
				if (selected != False)
					HandleAdjust(widget,info->item_index,
							type,x,y);
			    }
			    else {
				if (call_select) {
					HandleSelect(widget,info->item_index,
							type,x,y);
					call_select--;
				}
				else
					HandleAdjust(widget,info->item_index,
							type,x,y);
			    }
			}
		}
	}
}

static void
DrawRectangle OLARGLIST((dpy, win, gc, x1, y1, x2, y2))
	OLARG( Display *,	dpy)
	OLARG( Window,		win)
	OLARG( GC,		gc)
	OLARG( int,		x1)
	OLARG( int,		y1)
	OLARG( int,		x2)
	OLGRA( int,		y2)
{
	int tmp;

	if (x2 < x1)
		SWAP(x1, x2, tmp)
	if (y2 < y1)
		SWAP(y1, y2, tmp)

	XDrawRectangle(dpy, win, gc, x1, y1, x2 - x1, y2 - y1);
}

static void
TrackBoundingBox OLARGLIST((w, ve))
	OLARG( Widget,		w)
	OLGRA( OlVirtualEvent,	ve)
{
	Display *dpy = XtDisplay(w);
	Window  win = XtWindow(w);
	Window junk_win;
	int junk;
	unsigned int mask;
	int x1, y1;			/* original point */
	int x2, y2;			/* new point */
	int ox, oy;			/* last point */
	int tmp;
	int erase = 0;
	XRectangle rect;
	GC gc = FPART(w).label_gc;
	XEvent ev;

	x1 = ve->xevent->xbutton.x;
	y1 = ve->xevent->xbutton.y;

	XSetFunction(dpy, gc, GXinvert);
	XSetLineAttributes(dpy, gc, 0, LineOnOffDash, CapButt, JoinMiter);

	while (XCheckWindowEvent(dpy, win, ButtonPressMask | ButtonReleaseMask,
		&ev) != True) {
		XQueryPointer(dpy, win, &junk_win, &junk_win, &junk, &junk,
				&x2, &y2, &mask);

		/* range check */
		if (x2 < 0)
			x2 = 0;
		else if (x2+1 > (int)(w->core.width))
			x2 = w->core.width - 1;
		if (y2 < 0)
			y2 = 0;
		else if (y2+1 > (int)(w->core.height))
			y2 = w->core.height - 1;

		if ((x2 == ox) && (y2 == oy))
			continue;

		DrawRectangle(dpy, win, gc, x1, y1, x2, y2);
		if (erase)
			DrawRectangle(dpy, win, gc, x1, y1, ox, oy);

		ox = x2;
		oy = y2;
		erase = 1;
	}
	
	if (erase)
		DrawRectangle(dpy, win, gc, x1, y1, ox, oy);
	XSetFunction(dpy, gc, GXcopy);
	XSetLineAttributes(dpy, gc, 0, LineSolid, CapButt, JoinMiter);

	XUngrabPointer(XtDisplay(w), CurrentTime);

	/*
	 * If erase is False, that means the bounding box was not even
	 * drawn on the screen, and (x2,y2) are not initialized. So,
	 * don't bother doing any work.
	 */
	if (erase) {
		/* make sure (x1, y1) is the upper left corner */
		if (x2 < x1)
			SWAP(x1, x2, tmp)
		if (y2 < y1)
			SWAP(y1, y2, tmp)

		HandleMultiSelect(w, ve->virtual_name,
				  (Position)x1, (Position)y1,
			  	  (Dimension)(x2+1 - x1), (Dimension)(y2+1 - y1));
	}
}

/*
 *************************************************************************
 * CreateCursor - creates cursor suitable for the drag operation
 ****************************procedure*header*****************************
 */
static int
CreateCursor OLARGLIST((w, ve, data))
	OLARG( Widget,				w)
	OLARG( OlVirtualEvent,			ve)
	OLGRA( OlFlatDragCursorCallData *,	data)
{
	XtCallbackProc	cursor_proc;
	int ret = 0;

	data->x_hot 		= 0;
	data->y_hot 		= 0;
	data->yes_cursor 	= None;
	data->no_cursor  	= None;
	data->static_cursor 	= True;

	if (cursor_proc = IPART(w).cursor_proc) {
		XtPointer client_data;

		client_data = GetItemData(w,&(data->item_data),ve->item_index);
		data->ve = ve;
		(*cursor_proc)(w, client_data, (XtPointer)data);
	}

	if (data->yes_cursor == None)
		data->yes_cursor = (ve->virtual_name == OL_SELECT ||
                         	    ve->virtual_name == OL_DRAG) ?
                                	OlGetMoveCursor(w) :
					OlGetDuplicateCursor(w);
	else
		if (data->static_cursor == False)
			ret |= FREE_YES_CURSOR;
 
	if (data->no_cursor == None)
		data->no_cursor = OlGetNoCursor(XtScreen(w));
	else
		if (data->static_cursor == False)
			ret |= FREE_NO_CURSOR;
 
	return(ret);
} /* END OF CreateCursor() */

/*
 *************************************************************************
 * CallButtonCallbacks - invokes various button callbacks
 ****************************procedure*header*****************************
 */
static Boolean
CallButtonCallbacks OLARGLIST((w,item_index,proc,button_type,num_presses,x,y))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLARG( XtCallbackProc,	proc)
	OLARG( int,		button_type)
	OLARG( int,		num_presses)
	OLARG( Position,	x)
	OLGRA( Position,	y)
{
	OlFIconBoxButtonCD call_data;
	XtPointer client_data;

	client_data = GetItemData(w, &(call_data.item_data), item_index);
	call_data.x	 = x;
	call_data.y	 = y;
	call_data.count	 = num_presses;
	call_data.reason = button_type;
	call_data.ok	 = True;

	(*proc)(w, client_data, (XtPointer)&call_data);
	return(call_data.ok);
} /* END OF CallButtonCallbacks() */

/*
 *************************************************************************
 * HandleDrop - routine called when an icon is dragged and then dropped on
 * something.
 ****************************procedure*header*****************************
 */
static void
HandleDrop OLARGLIST((w, ve, drop_status, dst_info, root_info, f_cursor))
	OLARG(Widget,			w)
	OLARG(OlVirtualEvent,		ve)
	OLARG(OlDnDDropStatus,		drop_status)
	OLARG(OlDnDDestinationInfoPtr,	dst_info)
	OLARG(OlDnDDragDropInfoPtr,	root_info)
	OLGRA(OlFlatDragCursorCallData *,	f_cursor)
{
#define DROP_WINDOW	dst_info->window
#define X		dst_info->x
#define Y		dst_info->y
	XtCallbackProc	drop_proc;

	if (drop_status == OlDnDDropCanceled)
		/* Don't bother the client with canceled operations */
		return;

	if ((DROP_WINDOW == XtWindow(w)) &&
	    ((ve->virtual_name == OL_SELECT) || (ve->virtual_name == OL_DRAG))){
		Cardinal idx = OlFlatGetItemIndex(w, X, Y);

		/* If dropped in the same window &
		 * either onto some blank space or onto itself,
		 * it is considered an icon move. 
		 * Thus no need to bother the application.
		 */
		if ((idx == OL_NO_ITEM) || (idx == ve->item_index)) {
			MoveIcon(w, ve->item_index, X, Y, f_cursor);
			return;
		}
	}

	if (drop_proc = IPART(w).drop_proc) {
 	/* SAMC_TMP_DND_CHGS: somehow you need to show which DnD should	*/
	/* be used w.r.t. drop_status...				*/
		OlFlatDropCallData	data;
		XtPointer		client_data;

		client_data = GetItemData(w, &(data.item_data), ve->item_index);
		data.ve			= ve;
		data.dst_info		= dst_info;
		data.drop_status	= drop_status;
		data.root_info		= root_info;
		(*drop_proc)(w, client_data, (XtPointer)&data);
	}
#undef DROP_WINDOW
#undef X
#undef Y
} /* END OF HandleDrop() */

/*
 *************************************************************************
 * MoveIcon - moves an icon within the icon box window.  This routine is
 * called as result of the user dragging the icon around.
 ****************************procedure*header*****************************
 */
static void
MoveIcon OLARGLIST((w, item_index, cx, cy, f_cursor))
	OLARG( Widget,				w)
	OLARG( Cardinal,			item_index)
	OLARG( Position,			cx) /* cursor's hot x, y*/
	OLARG( Position,			cy) /* location...	*/
	OLGRA( OlFlatDragCursorCallData *,	f_cursor)
{
	if (IPART(w).movable != FALSE) {
		Arg args[2];

		cx -= f_cursor->x_hot;
		cy -= f_cursor->y_hot;
		XtSetArg(args[0], XtNx, cx);
		XtSetArg(args[1], XtNy, cy);
		OlFlatSetValues(w, item_index, args, 2);
/*		_OlFlatLayoutIconBox(w, 0, 0); */
	}
} /* END OF MoveIcon() */

static void
HandleMenu OLARGLIST((w, item_index, type, x, y))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLARG( OlVirtualName,	type)
	OLARG( Position,	x)
	OLGRA( Position,	y)
{
	if (IPART(w).menu_proc)
		CallButtonCallbacks(w, item_index, IPART(w).menu_proc, type,
				    1, x, y);
} /* END OF HandleMenu() */

static void
UpdateLastSelectedItem OLARGLIST((w, except_idx))
	OLARG( Widget,		w)
	OLGRA( Cardinal,	except_idx)
{
    Arg args[2];
    Boolean set;
    Boolean managed;
    int i;

    IPART(w).last_select = (Cardinal)OL_NO_ITEM;

    /*
     * The next to the last item was turned off, so find
     * the only remaining item that is set.
     */
    XtSetArg(args[0], XtNset, &set);
    XtSetArg(args[1], XtNmanaged, &managed);
    for (i=0; i < FPART(w).num_items; i++) {
	OlFlatGetValues(w, i, args, 2);
	if ((managed != False) && (set != False) && (i != except_idx)) {
		IPART(w).last_select = i;
		break;
	}
    }
}

static void
HandleAdjust OLARGLIST((w, item_index, type, x, y))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLARG( OlVirtualName,	type)
	OLARG( Position,	x)
	OLGRA( Position,	y)
{
	Boolean ret;

	if (IPART(w).adjust_proc)
		ret = CallButtonCallbacks(w, item_index, IPART(w).adjust_proc,
				type, 1, x, y);
	else
		ret = True;

	if (ret !=False) {
		/* do the default action */
		Boolean selected;
		Arg args[1];

		XtSetArg(args[0], XtNset, &selected);
		OlFlatGetValues(w, item_index, args, 1);

		if (IPART(w).exclusives != False) {
			if (IPART(w).noneset == False)
				return;
			else {
				if (selected == False) {
					if (IPART(w).select_count != 0)
						return;
				}
				else {
					if (IPART(w).select_count != 1)
						return;
				}
			}
		}
		else {
			if ((selected != False) &&
			    (IPART(w).noneset == False) &&
			    (IPART(w).select_count == 1))
				return;
		}

		if (selected == True) {
			selected = False;
			IPART(w).last_select = OL_NO_ITEM;
		}
		else {
			selected = True;
			IPART(w).last_select = item_index;
		}

		XtSetArg(args[0], XtNset, selected);
		OlFlatSetValues(w, item_index, args, 1);

		if ((IPART(w).select_count == 1) && (selected == False))
		    UpdateLastSelectedItem(w, (Cardinal)OL_NO_ITEM);
	}

	if (IPART(w).post_adjust_proc)
		ret = CallButtonCallbacks(w, item_index,
				IPART(w).post_adjust_proc, type, 1, x, y);
} /* END OF HandleAdjust() */

static void
HandleSelect OLARGLIST((w, item_index, type, x, y))
	OLARG( Widget,		w)
	OLARG( Cardinal,	item_index)
	OLARG( OlVirtualName,	type)
	OLARG( Position,	x)
	OLGRA( Position,	y)
{
	Boolean ret;

	if (IPART(w).select1_proc)
		ret = CallButtonCallbacks(w, item_index,
				IPART(w).select1_proc, type, 1,
				x, y);
	else
		ret = True;

	if (ret != False) {
		/* do the default action */
		Arg false_args[1];
		Arg true_args[1];
		Arg query_args[1];
		Boolean	managed;
		int i;

		XtSetArg(false_args[0], XtNset, False);
		XtSetArg(true_args[0], XtNset, True);
		XtSetArg(query_args[0], XtNmanaged, &managed);

		if ((IPART(w).select_count == 0) ||
		    (IPART(w).select_count == 1)) {
			if (IPART(w).last_select != item_index) {
			    if ((IPART(w).select_count == 1) &&
				(IPART(w).last_select != (Cardinal)OL_NO_ITEM))
				    OlFlatSetValues(w, IPART(w).last_select,
						    false_args, 1);
			    OlFlatSetValues(w, item_index, true_args, 1);
			}
			IPART(w).select_count = 1;
			IPART(w).last_select  = item_index;
		}
		else {
			if (IPART(w).exclusives == False) {
			    for (i=0; i < FPART(w).num_items; i++)
				if (i == item_index)
				{
					Arg	arg[1];
					Boolean is_set;

	/* Find out whether `i' is set, if it's already
	 * set, call refresh_item() because of focus visual */

					XtSetArg(arg[0], XtNset, &is_set);
					OlFlatGetValues(w, i, arg, 1);
					if (!is_set)
					   OlFlatSetValues(w, i, true_args, 1);
					else
					   OlFlatRefreshItem(w, i, True);
				}
		    		else {
					OlFlatGetValues(w, i, query_args, 1);
					if (managed != False)
					OlFlatSetValues(w, i, false_args, 1);
		    		}
			    IPART(w).select_count = 1;
			    IPART(w).last_select  = item_index;
			}
			else {
			        IPART(w).select_count = 0;
				OlFlatSetValues(w, item_index, true_args, 1);
			}
		}
	}

	if (IPART(w).post_select1_proc)
		ret = CallButtonCallbacks(w, item_index,
				IPART(w).post_select1_proc, type, 1,
				x, y);
} /* END OF HandleSelect() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * ClassInitialize -
 ****************************procedure*header*****************************
 */
static void
ClassInitialize OL_NO_ARGS()
{
	OlFlatInheritAll(flatIconBoxWidgetClass);

#undef F
#define F	flatIconBoxClassRec.flat_class
	F.draw_item		= DrawItem;
	F.initialize		= Initialize;
	F.item_activate		= ItemActivate;
	F.set_values		= SetValues;
	F.item_dimensions	= ItemDimensions;
	F.item_set_values	= ItemSetValues;
#undef F

} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * Initialize
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	IPART(new).last_select = (Cardinal)OL_NO_ITEM;
	IPART(new).select_count = 0;
} /* END OF Initialize() */

/*
 *************************************************************************
 * Realize
 ****************************procedure*header*****************************
 */
static void
Realize OLARGLIST((w, value_mask, attributes))
	OLARG( Widget,			w)
	OLARG( Mask *,			value_mask)
	OLGRA( XSetWindowAttributes *,	attributes)
{
	XtCreateWindow(w, InputOutput, (Visual *)CopyFromParent,
		*value_mask, attributes);

	if ((IPART(w).trigger_proc) || (IPART(w).movable != False)) {
		OlDnDSiteRect rect;
		Arg args[1];
		XtPointer client_data = NULL;

		XtSetArg(args[0], XtNclientData, &client_data);
		OlFlatGetValues(w, 1, args, 1);

		rect.x 		= 0;
		rect.y		= 0;
		rect.width	= w->core.width;
		rect.height	= w->core.height;
		IPART(w).drop_site_id =
			 OlDnDRegisterWidgetDropSite(w,
				OlDnDSitePreviewNone,
				&rect, 1,
				(OlDnDTMNotifyProc)IPART(w).trigger_proc,
				NULL,
				True,
				client_data);
	}
	else
		IPART(w).drop_site_id = NULL;

	/*
	 * If exclusives and noneset is false, then set the first item.
	 */
	if ((IPART(w).exclusives != False) && (IPART(w).noneset == False) &&
	    (IPART(w).select_count == 0)) {
		Cardinal i;
		OlFlatGraphInfoList info;

		for (info=GPART(w).info,i=0; i < FPART(w).num_items; i++,info++)
			if (info->flags & OL_B_FG_MANAGED)
				break;

		if (i == FPART(w).num_items) {
			IPART(w).last_select  = OL_NO_ITEM;
			IPART(w).select_count = 0;
		}
		else {
			Arg args[1];

			XtSetArg(args[0], XtNset, True);
			OlFlatSetValues(w, i, args, 1);
		}
	}
}

/*
 *************************************************************************
 * Destroy
 ****************************procedure*header*****************************
 */
static void
Destroy OLARGLIST((w))
	OLGRA( Widget,	w)
{
} /* end of Destroy */

/*
 *************************************************************************
 * SetValues
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,		current)
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	if (FPART(new).items_touched == True)
		UpdateLastSelectedItem(new, (Cardinal)OL_NO_ITEM);

	return(False);
} /* END OF SetValues() */


/*
 *************************************************************************
 * DrawItem - this routine draws a single instance of a iconBox
 * sub-object.  In this routine, we use the Olg code to draw a RectBox
 * around our item.  We pass in a function pointer to the RectBox drawing
 * routine that fills in the interior of the item.
 ****************************procedure*header*****************************
 */
static void
DrawItem OLARGLIST((w, item, di))
	OLARG( Widget,		w)	/* container widget id	*/
	OLARG( FlatItem,	item)	/* expanded item	*/
	OLGRA( OlFlatDrawInfo *,di)	/* Drawing information	*/
{
	if ((FITEM(item).mapped_when_managed != False) &&
	    (FITEM(item).managed != False) &&
	    (IPART(w).draw_proc)) {
		OlFIconDrawRec draw_rec;
		XtPointer client_data;
		Arg args[1];

		/* set up drawing record for drawing function */
		draw_rec.x 		= FITEM(item).x;
		draw_rec.y 		= FITEM(item).y;
		draw_rec.item_index	= FITEM(item).item_index;
		draw_rec.op		= IITEM(item).object_data;
		draw_rec.select		= IITEM(item).set;
		draw_rec.busy		= IITEM(item).busy;
		draw_rec.width		= FITEM(item).width;
		draw_rec.height		= FITEM(item).height;
		draw_rec.label		= FITEM(item).label;
		draw_rec.font		= item->flat.font;
		draw_rec.font_list	= PPART(w).font_list;
		draw_rec.label_gc	= FPART(w).label_gc;
		draw_rec.fg_color	= item->flat.foreground;
		draw_rec.bg_color	= CPART(w).background_pixel;
		draw_rec.focus_color	= PPART(w).input_focus_color;
		draw_rec.focus		= (FPART(w).focus_item ==
						FITEM(item).item_index) ?
						True : False;

		XtSetArg(args[0], XtNclientData, &client_data);
		OlFlatGetValues(w, FITEM(item).item_index, args, 1);

		(IPART(w).draw_proc)(w, client_data, (XtPointer)&draw_rec);
	}
} /* END OF DrawItem() */

static void
DoDragOp(w, ve, mouse)
Widget w;
OlVirtualEvent ve;
Boolean mouse;		/* operation was initiated using a mouse */
{
	if (IPART(w).drop_site_id || IPART(w).drop_proc) {
		OlDnDDragDropInfo	root_info;
		OlDnDDestinationInfo	dst_info;
		OlDnDAnimateCursors	cursor;
		OlDnDDropStatus		drop_status;
		OlFlatDragCursorCallData f_cursor;
		int			free_cursor;
		Arg			args[1];

		/* turn on busy state */
		XtSetArg(args[0], XtNbusy, True);
		OlFlatSetValues(w, ve->item_index, args, 1);
	
		free_cursor = CreateCursor(w, ve, &f_cursor);
		cursor.yes_cursor = f_cursor.yes_cursor;
		cursor.no_cursor  = f_cursor.no_cursor;

		if (mouse != False)
			XUngrabPointer(XtDisplay(w), CurrentTime);

		drop_status = OlDnDTrackDragCursor(w, &cursor, &dst_info,
						 &root_info);

		/* restore busy state */
		XtSetArg(args[0], XtNbusy, False);
		OlFlatSetValues(w, ve->item_index, args, 1);

		/* free the cursor only if the app created it */
		if (free_cursor & FREE_YES_CURSOR)
			XFreeCursor(XtDisplay(w), cursor.yes_cursor);
		if (free_cursor & FREE_NO_CURSOR)
			XFreeCursor(XtDisplay(w), cursor.no_cursor);

		HandleDrop(w, ve, drop_status, &dst_info,&root_info, &f_cursor);
        }
        else {
		if (mouse != False)
			XUngrabPointer(XtDisplay(w), CurrentTime);
	}
}

/*
 *************************************************************************
 * ItemDimensions - this routine determines the size of a single sub-object
 ****************************procedure*header*****************************
 */
static void
ItemDimensions OLARGLIST((w, item, ret_width, ret_height))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLARG( register Dimension *,	ret_width)
	OLGRA( register Dimension *,	ret_height)
{
    Dimension width = 0;
    Dimension height = 0;
    Arg args[2];

    XtSetArg(args[0], XtNwidth, &width);
    XtSetArg(args[1], XtNheight, &height);
    OlFlatGetValues(w, FITEM(item).item_index, args, 2);

    *ret_width = (width == 0) ? 1 : width;
    *ret_height = (height == 0) ? 1 : height;

}					/* END OF ItemDimensions() */

/*
 *************************************************************************
 * ItemSetValues - this routine is called whenever the application does
 * an XtSetValues on the container, requesting that an item be updated.
 * If the item is to be refreshed, the routine returns True.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
ItemSetValues OLARGLIST((w, current, request, new, args, num_args))
	OLARG( Widget,	   w)		/* Flat widget container id	*/
	OLARG( FlatItem,   current)	/* expanded current item	*/
	OLARG( FlatItem,   request)	/* expanded requested item	*/
	OLARG( FlatItem,   new)		/* expanded new item		*/
	OLARG( ArgList,	   args)
	OLGRA( Cardinal *, num_args)
{
	Cardinal	item_index 	= new->flat.item_index;
	Boolean		redisplay 	= False;
	Boolean		call_refresh 	= False;

#define DIFFERENT(field)	(IITEM(new).field != IITEM(current).field)

	if (FITEM(new).managed != FITEM(current).managed) {
		if ((FITEM(new).managed == False) &&
		    (IPART(w).last_select == item_index) &&
		    (IPART(w).select_count <= 2)) {
			UpdateLastSelectedItem(w, item_index);
		}
	}

	if (DIFFERENT(set)) {
		int actions;

		redisplay = True;

		/*
		 * The actions to be performed here depends on 3 parameters,
		 * set, noneset, and the new exclusives setting. Because
		 * similar actions may be taken in various combination of
		 * the 3 parameters, the actions will be table driven.
		 */
		actions = set_actions[ACTION_IDX(IITEM(new).set,
						 IPART(w).noneset,
						 IPART(w).exclusives)];

		if (actions & CHK_CNT) {
			if (IPART(w).select_count <= 1)
				goto skip;
		}

		if (actions & RESET) {
			if ((IPART(w).last_select != OL_NO_ITEM) &&
			    (IPART(w).last_select != item_index)) {
				Arg args[1];

				XtSetArg(args[0], XtNset, False);
				OlFlatSetValues(w,IPART(w).last_select,args,1);
			}
		}

		if (actions & INC)
			IPART(w).select_count++;
		else if (actions & DEC)
			IPART(w).select_count--;
		else if (actions & ONE)
			IPART(w).select_count = 1;

		if (actions & CHK_OK) {
			if (IITEM(new).set != False)
				IPART(w).last_select = item_index;
			else
				IPART(w).last_select = OL_NO_ITEM;
		}

		if (actions & RAISE) {
			call_refresh = True;
		}
	}

skip:
	if (DIFFERENT(busy))
		redisplay = True;

	if (strcmp(FITEM(new).label, FITEM(current).label)) {
		unsigned int width;
		unsigned int height;
		int  x;
		int  y;

		width  =_OlMax(FITEM(new).width, FITEM(current).width);
		height =_OlMax(FITEM(new).height,FITEM(current).height);
		x = _OlMin(FITEM(new).x, FITEM(current).x);
		y = _OlMin(FITEM(new).y, FITEM(current).y);
		(void)XClearArea(XtDisplay(w), XtWindow(w), x, y,
				 width, height, 0);
	}

#undef DIFFERENT

	if (call_refresh)
	{
	/* If we are refreshing this item then re-set redisplay,
	 * so we don't see additional drawing */

		OlFlatRaiseExpandedItem(w, new, True);
		redisplay = False;
	}
	return(redisplay);
} /* END OF ItemSetValues() */

/* ARGSUSED */
static Boolean
ItemActivate OLARGLIST((w, item, type, data))
	OLARG( Widget,		w)
	OLARG( FlatItem,	item)
	OLARG( OlVirtualName,	type)
	OLGRA( XtPointer,	data)
{
	Boolean consumed = False;
	void (*func)(Widget, Cardinal, OlVirtualName, Position, Position)=NULL;

	if (item->flat.item_index != (Cardinal)OL_NO_ITEM) {
		Boolean busy;
		Arg args[1];

		/* Don't process any events for a busy item */
		XtSetArg(args[0], XtNbusy, &busy);
		OlFlatGetValues(w, item->flat.item_index, args, 1);

		if (busy != False) {
			return(True);
		}
	}

	switch(type) {
	case OL_SELECTKEY:
		func = HandleSelect;
		type = OL_SELECTKEY;
		break;
	case OL_MENUKEY:
		func = HandleMenu;
		type = OL_MENUKEY;
		break;
	case OL_ADJUSTKEY:
		func = HandleAdjust;
		type = OL_ADJUSTKEY;
		break;
	case OL_DRAG:
	case OL_DUPLICATEKEY:
	case OL_LINKKEY:
		{
			OlVirtualEventRec ve;

			memset((void *)&ve, 0, sizeof(OlVirtualEventRec));
			ve.virtual_name = type;
			ve.item_index   = item->flat.item_index;
			DoDragOp(w, &ve, False);
			consumed = True;
		}
	}

	switch(type) {
	case OLM_KDeselectAll:
		consumed = True;
		HandleMultiSelect(w, OLM_KDeselectAll,
				  (Position)-1, (Position)-1,
			  	  (Dimension)(w->core.width + 2),
				  (Dimension)(w->core.height + 2));
		break;
	case OLM_KSelectAll:
		/*
		 * Implement as a bounding box big enough to
		 * surround the entire widget.
		 */
		consumed = True;
		HandleMultiSelect(w, OL_SELECT,
				  (Position)-1, (Position)-1,
			  	  (Dimension)(w->core.width + 2),
				  (Dimension)(w->core.height + 2));
		break;
	}

	if (func) {
		consumed = True;
		(*func)(w, item->flat.item_index, type,
			item->flat.x + (item->flat.width / 2),
			item->flat.y + (item->flat.height / 2));
	}
	return(consumed);
} /* END OF ItemActivate() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * ButtonHandler - handles button presses and ButtonReleases
 ****************************procedure*header*****************************
 */
static void
ButtonHandler OLARGLIST((w, ve))
	OLARG( Widget,		w)
	OLGRA( OlVirtualEvent,	ve)
{
	Boolean ret;
	Display *dpy = XtDisplay(w);
	Cardinal ofi = OL_NO_ITEM;

	switch(ve->xevent->type) {
	case ButtonPress:
	case EnterNotify:
	case MotionNotify:
		if (ve->item_index != (Cardinal)OL_NO_ITEM) {
			Boolean	busy;
			Arg	args[1];

#define PWIDGET(w)	(((PrimitiveWidget)w)->primitive)
#define TIME		ve->xevent->xbutton.time

	/* `focus_on_select' class field is False, so we will have to
	 * handle focus ourselves */


			if (ve->xevent->type == ButtonPress &&
			    ve->virtual_name == OL_SELECT)
			{
	/* Optimize the situation if the widget already has focus,
	 * otherwise call item_accept_focus() */

				if (!PWIDGET(w).has_focus)
				{
	/* I don't have focus, so call item_accept_focus(), one extra
	 * drawing here but we can't optimize this situation because
	 * requiring OlSetInputFocus() call. */

					(void)OlFlatCallAcceptFocus(
						w, ve->item_index, TIME);
				}
				else if (FPART(w).focus_item != ve->item_index)
				{

	/* The container already has focus and I know calling DoDragOp() or
	 * HandleSelect() will force a re-draw on ve->item_index, so just
	 * setting focus_item and last_focus_item to ve->item_index and bypass
	 * item_accept_focus(), save one drawing. Meanwhile save the old
	 * focus item index in `ofi' for later use */

					ofi = FPART(w).focus_item;
					FPART(w).last_focus_item =
					FPART(w).focus_item = ve->item_index;
				}
			}

				/* Don't process any events for a busy item */
			XtSetArg(args[0], XtNbusy, &busy);
			OlFlatGetValues(w, ve->item_index, args, 1);

			if (busy != False) {
				ve->consumed = True;
	/* If the item is busy and ofi is not OL_NO_ITEM, then
	 * do recovering because we don't want to switch focus
	 * to the busy item... */

				if (ofi != OL_NO_ITEM)
				{
					FPART(w).last_focus_item =
					FPART(w).focus_item = ofi;
				}
				return;
			}
		}
		else if (ve->xevent->type == ButtonPress &&
			 ve->virtual_name == OL_SELECT)
		{

			if (!PWIDGET(w).has_focus)
			{
	/* I don't have focus, and clicked on an empty spot! */

				(void)XtCallAcceptFocus(w, &TIME);
			}
		}

		switch(ve->virtual_name) {
		case OL_DUPLICATE:
		case OL_LINK:
		case OL_SELECT:
		case OLM_BDrag:
			ve->consumed = True;
			switch(OlDetermineMouseAction(w, ve->xevent)) {
			case MOUSE_MOVE:
			    if (ve->virtual_name == OLM_BDrag)
				ve->virtual_name = OL_SELECT;
			    if (ve->item_index == (Cardinal)OL_NO_ITEM) {
				if (IPART(w).exclusives == False)
					TrackBoundingBox(w, ve);
				else
					XUngrabPointer(dpy, CurrentTime);
			    }
			    else {
				if (ofi != OL_NO_ITEM)
					OlFlatRefreshItem(w, ofi, True);
				DoDragOp(w, ve, True);
			    }
			    break;
			case MOUSE_CLICK:
				if (ve->virtual_name != OL_SELECT ||
				    ve->item_index == (Cardinal)OL_NO_ITEM)
					return;

	/* Check whether we need to refresh old focus item, ofi. */
				if (ofi != OL_NO_ITEM)
				{
	/* Well, we have to refresh the old focus item because we are
	 * not calling item_accept_focus(). */

					Boolean doit;

					if (!IPART(w).select_count)
					{
	/* We definitely need to refresh the focus item because
	 * the item is not currently set, so HandleSelect() won't
	 * need to re-draw this item */

						doit = True;
					}
					else
					{
	/* In this case, it's conditional, it the focus item is set
	 * then it will be un-set by HandleSelect(), otherwise,
	 * we will have to refresh it */

						Arg	arg[1];

						XtSetArg(
							arg[0], XtNset, &doit);
						OlFlatGetValues(w, ofi, arg, 1);
						doit = !doit;
					}
					if (doit)
					{
						OlFlatRefreshItem(w, ofi, True);
					}
				}
				HandleSelect(w, ve->item_index, OL_SELECT,
						ve->xevent->xbutton.x,
						ve->xevent->xbutton.y);
				break;
#undef PWIDGET
#undef TIME
			case MOUSE_MULTI_CLICK:
				/*
				 * Reset internal count so that the next click
				 * will not be mistaken for a multi-click.
				 */
				OlResetMouseAction(w);

				if ((ve->virtual_name != OL_SELECT) ||
				    (ve->item_index == (Cardinal)OL_NO_ITEM))
					return;

				if (IPART(w).select2_proc) {
					Arg args[1];

					/* busy icon */
					XtSetArg(args[0], XtNbusy, True);
					OlFlatSetValues(w, ve->item_index,
							args, 1);
					
					/*
					 * Flush the queue now, so that
					 * it gets displayed. This will
					 * improve the user feedback.
					 */
					OlUpdateDisplay(w);

					ret = CallButtonCallbacks(w,
						ve->item_index,
						IPART(w).select2_proc,
						OL_SELECT, 2,
						ve->xevent->xbutton.x,
						ve->xevent->xbutton.y);

					if (ret != False) {
						XtSetArg(args[0],XtNbusy,False);
						OlFlatSetValues(w,
						    ve->item_index, args, 1);
					}
				}
				break;
			}
			break;
		case OL_ADJUST:
			ve->consumed = True;
			if ((ve->xevent->type != ButtonPress))
				return;
			if (ve->item_index == (Cardinal)OL_NO_ITEM) {
				/* pointer on the background */

				if (IPART(w).exclusives == False) {
				    if (OlDetermineMouseAction(w, ve->xevent) ==
				 	MOUSE_MOVE)
					TrackBoundingBox(w, ve);
				    else
					XUngrabPointer(dpy, CurrentTime);
			    	}
			} /* if on background */
			else {
				/* pointer on an item */

				HandleAdjust(w, ve->item_index, OL_ADJUST,
				     		ve->xevent->xbutton.x,
				     		ve->xevent->xbutton.y);
			}
			break;
		case OL_MENU:
			if (ve->item_index == (Cardinal)OL_NO_ITEM)
				return;
			ve->consumed = True;

			if (ve->xevent->type == ButtonPress)
				HandleMenu(w, ve->item_index, OL_MENU,
					   ve->xevent->xbutton.x,
					   ve->xevent->xbutton.y);
			break;
		default:
			break;
		}
		break;
	}
} /* END OF ButtonHandler() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

