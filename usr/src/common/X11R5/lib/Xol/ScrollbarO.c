#ifndef NOIDENT
#ident	"@(#)scrollbar:ScrollbarO.c	1.13"
#endif

/* #includes go here    */
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>
#include <Xol/ScrollbarP.h>
#include <Xol/PopupMenu.h>
#include <Xol/FButtons.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *              1. Private Procedures
 *              2. Class   Procedures
 *              3. Action  Procedures
 *              4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

/* private procedures   */
static void MenuCallback OL_ARGS((Widget, XtPointer, int));
static void MenuSelect OL_ARGS((Widget, XtPointer, XtPointer));
static void RemoveHandler OL_ARGS((Widget, XtPointer, XtPointer));
static void SetMenuItemsSensitive OL_ARGS((ScrollbarWidget, Boolean));
static void ShowPageInd OL_ARGS((Widget, XtPointer, XEvent *, Boolean *));

extern Boolean _OloSBFindOp OL_ARGS((Widget, XEvent *, unsigned char *));
extern void _OloSBHighlightHandler OL_ARGS((Widget, OlDefine));
extern void _OloSBMakePageInd OL_ARGS((ScrollbarWidget));
extern Widget _OloSBCreateMenu OL_ARGS((Widget, OlDefine));
extern Boolean _OloSBMenu  OL_ARGS((Widget, XEvent *));
extern void _OloSBUpdatePageInd OL_ARGS((ScrollbarWidget, Boolean, Boolean));

void _OlSBMoveSlider
	OL_ARGS((ScrollbarWidget, OlScrollbarVerify *, Boolean, Boolean));
void _OlSBGetGCs OL_ARGS((ScrollbarWidget));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */
#define SBW             sw->scroll
#define HORIZ(W)        ((W)->scroll.orientation == OL_HORIZONTAL)
#define INRANGE(V,MIN,MAX) (((V) <= (MIN)) ? (MIN) : ((V) >= (MAX) ? (MAX) : (V)))
#define MIN(X,Y)        ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y)        ((X) > (Y) ? (X) : (Y))
#define VAL_TO_PIX(M1,M2,Q1)	(((unsigned long)(M2)/(Q1)*(M1))+(((M2)%(Q1))*(M1)/(Q1)))

#define THEBUTTON1	SBW.here_to_lt_btn
#define THEBUTTON2	SBW.lt_to_here_btn

/* Standard menu button types */
#define HERE_TO_BEGIN           1
#define PREVIOUS                2
#define BEGIN_TO_HERE           3

#define TO_BEGIN_ITEM    0
#define TO_HERE_ITEM     1
#define TO_PREVIOUS_ITEM 2

typedef struct _item
   {
   caddr_t label;
   caddr_t sensitive;
   } item;

/*
 *************************************************************************
 * Private Procedures
 ***************************private*procedures****************************
 */

/*
 * MenuSelect
 *
 */

static void
MenuSelect OLARGLIST((w, client_data, call_data))
	OLARG(Widget, w)
	OLARG(XtPointer, client_data)
	OLGRA(XtPointer, call_data)
{
        OlFlatCallData * p = (OlFlatCallData *)call_data;

        switch (p-> item_index)
        {
        case TO_BEGIN_ITEM:
                MenuCallback(w, client_data, HERE_TO_BEGIN);
                break;
        case TO_HERE_ITEM:
                MenuCallback(w, client_data, BEGIN_TO_HERE);
                break;
        case TO_PREVIOUS_ITEM:
                MenuCallback(w, client_data, PREVIOUS);
                break;
        }

} /* end of MenuSelect */

/*
 * _OloSBCreateMenu - returns a PopupMenuShell that can be used
 *	between many scrollbars.  An application could call this
 *	function for each orientation with a parent that is the
 *	toplevel shell and an
 */

Widget
_OloSBCreateMenu OLARGLIST((parent, orientation))
	OLARG(Widget, parent)
	OLGRA(OlDefine, orientation)
{
        Arg arg[3];
        Widget MenuShell;
	static char * fields[] = { XtNlabel, XtNsensitive };

        MenuShell =
           XtCreatePopupShell(
		orientation == OL_HORIZONTAL?
			"olScrollBarHMenu" : "olScrollBarVMenu",
		popupMenuShellWidgetClass, parent, NULL, 0);

        XtSetArg(arg[0], XtNitemFields, fields);
        XtSetArg(arg[1], XtNnumItemFields, XtNumber(fields));
        XtSetArg(arg[2], XtNselectProc, MenuSelect);
        XtCreateManagedWidget("pane",
           flatButtonsWidgetClass, MenuShell, arg, 3);

        return(MenuShell);

} /* end of _OloSBCreateMenu */

/* ARGSUSED */
static void
MenuCallback OLARGLIST((w, client_data, buttontype))
	OLARG(Widget, w)
	OLARG(XtPointer, client_data)
	OLGRA(int, buttontype)
{
       ScrollbarWidget sw = (ScrollbarWidget)client_data;
       OlScrollbarVerify olsb;

	/* save current position */
       if (buttontype == PREVIOUS) {
                olsb.new_location = INRANGE(SBW.previous,SBW.sliderMin,
                                        SBW.sliderMax - SBW.proportionLength);
		sw->scroll.previous = sw->scroll.sliderValue;
       }
       else {
                int val;
		int cableLength;

		if (HORIZ (sw))
		    cableLength = sw->core.width - sw->scroll.anchwidth * 2;
		else
		    cableLength = sw->core.height - sw->scroll.anchlen * 2;

                val = VAL_TO_PIX(SBW.XorY,SBW.proportionLength, cableLength);
		sw->scroll.previous = sw->scroll.sliderValue;
                if (buttontype == BEGIN_TO_HERE)
                        olsb.new_location = MAX(sw->scroll.sliderValue - val,
                                                sw->scroll.sliderMin);
                else
                        olsb.new_location = MIN(sw->scroll.sliderValue + val,
                                                SBW.sliderMax - SBW.proportionLength);
       }

	if (olsb.new_location != sw->scroll.sliderValue)
        	_OlSBMoveSlider(sw,&olsb,TRUE,FALSE);
} /* MenuCallback */


/*
 *************************************************************************
 * _OloSBUpdatePageInd - called from the _OlSBMoveSlider and highlight to
 * position and draw the scrollbar page indicator for Open Look.
 * A NoOp for Motif.
 ****************************procedure*header*****************************
 */
extern void
_OloSBUpdatePageInd OLARGLIST((sw, draw, move))
	OLARG(ScrollbarWidget, sw)
	OLARG(Boolean, draw)
	OLGRA(Boolean, move)
{
	char buff[16];
	int width, height, x, y;
	int textHeight;
	int w; /* width of page_ind window */
	int pad;

	/* page indicator is currently mapped */
	(void)sprintf(buff,"%d",sw->scroll.currentPage);
	width = XTextWidth(sw->primitive.font, buff, strlen(buff));
	pad = OlgGetHorizontalStroke (sw->scroll.pAttrs);
	w = width + pad * 12;
	textHeight = sw->primitive.font->max_bounds.ascent +
	    sw->primitive.font->max_bounds.descent;
	height = textHeight + OlgGetVerticalStroke (sw->scroll.pAttrs) * 4;

	if ((move)  || (SBW.page_ind->core.width != w)) {
		/* The next few lines assume page indicator only
		   works in vertical scrollbars */
		x = SBW.absx + SBW.offset + ((SBW.showPage == OL_LEFT) ? 
			-(w + pad*3 + 1) :
			(SBW.anchwidth + pad*3));
		y = SBW.absy + SBW.sliderPValue +
		    (int) (SBW.elevheight-height) / 2;

		XtConfigureWidget(sw->scroll.page_ind, x, y, w, height, 1);
	}
	if (draw) {
		XFillRectangle(XtDisplay(sw),
			     XtWindow(sw->scroll.page_ind),
			       OlgGetBg1GC (sw->scroll.pAttrs), 0, 0,
			       sw->scroll.page_ind->core.width,
			       sw->scroll.page_ind->core.height);

		XDrawString(XtDisplay(sw),
		            XtWindow(sw->scroll.page_ind),
			    sw->scroll.textGC,
			    (int)(SBW.page_ind->core.width - width)/2,
			    (int)(sw->scroll.page_ind->core.height -
				  textHeight) / 2 +
			          sw->primitive.font->max_bounds.ascent,
			    buff,strlen(buff));
	}
}  /* END OF _OloSBUpdatePageInd() */

/* ARGSUSED */
static void
ShowPageInd OLARGLIST((w, data, xevent, cont_to_dispatch))
	OLARG(Widget, w)
	OLARG(XtPointer, data)
	OLARG(XEvent *, xevent)
	OLGRA(Boolean *, cont_to_dispatch)
{
	ScrollbarWidget sw = (ScrollbarWidget)data;

	_OloSBUpdatePageInd(sw, True, False);
}

static void
SetMenuItemsSensitive OLARGLIST((sw, sensitive))
	OLARG(ScrollbarWidget, sw)
	OLGRA(Boolean, sensitive)
{
	Widget menu = SBW.popup;
	Widget buttons;
	WidgetList children;
	Cardinal num_children;
	Cardinal i;

	/*  From the menu, find the first FlatButtons child.  */
	XtVaGetValues(menu, XtNchildren, &children,
		XtNnumChildren, &num_children, (String) NULL);
	for (i = 0; i < num_children; i++)  {
		if (XtIsSubclass(children[i], flatWidgetClass))  {
			buttons = children[i];
			break;
		}
	}

	/*  If we found a FlatButtons child, then set the sensitivity of
	    the items.  */
	if (buttons)  {
		Arg args[1];

		XtSetArg(args[0], XtNsensitive, sensitive);
		OlFlatSetValues(buttons, 0, args, 1);
		OlFlatSetValues(buttons, 1, args, 1);
		/*  Here we set the client data to associate the scrollbar
		    with the menu.  This allows the same menu to be reused. */
		XtSetArg(args[0], XtNclientData, sw);
		XtSetValues(buttons, args, 1);
	}
}  /* end of SetMenuItemsSensitive() */


/* ARGSUSED */
static void
RemoveHandler OLARGLIST((w, client_data, call_data))
	OLARG(Widget, w)
	OLARG(XtPointer, client_data)
	OLGRA(XtPointer, call_data)
{
	XtRemoveEventHandler(w, ExposureMask, FALSE, ShowPageInd, call_data);
}

/*
 *************************************************************************
 * _OloSBMakePageInd - called from the Realize and SetValues to create the
 *  scrollbar page indicator for Open Look.  A NoOp for Motif.
 ****************************procedure*header*****************************
 */
extern void
_OloSBMakePageInd OLARGLIST((sw))
	OLGRA(ScrollbarWidget, sw)
{
	static Arg args[3];

	XtSetArg(args[0], XtNwidth, 1);
	XtSetArg(args[1], XtNheight, 1);
	XtSetArg(args[2], XtNbackground, sw->core.background_pixel);
	if ((sw->scroll.page_ind = XtCreatePopupShell("PageInd",
		overrideShellWidgetClass, (Widget)sw, (ArgList)args, 3)) == NULL) {
			OlVaDisplayWarningMsg(	XtDisplay((Widget)sw),
						OleNfileScrollbar,
						OleTmsg2,
						OleCOlToolkitWarning,
						OleMfileScrollbar_msg2,
						XtName((Widget)sw));
		sw->scroll.showPage = OL_NONE;
		return;
	}

	/* add event handler */
	XtAddEventHandler(SBW.page_ind,ExposureMask, FALSE,
			  ShowPageInd, (XtPointer) sw);
	XtAddCallback(SBW.page_ind, XtNdestroyCallback,
		      (XtCallbackProc)RemoveHandler, (XtPointer)sw);
}  /* END OF _OloMakePageInd() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * _OloSBHighlightHandler - changes the colors when this widget gains or
 *  loses focus.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
extern void
_OloSBHighlightHandler OLARGLIST((w, type))
	OLARG(Widget,	w)
	OLGRA(OlDefine,	type)
{
	_OlSBGetGCs ((ScrollbarWidget) w);
	(*(XtClass(w)->core_class.expose)) (w, NULL, NULL);
} /* END OF _OloSBHighlightHandler() */

/*
 *************************************************************************
 * _OloSBMenu - called from the SBButtonHandler and SBActivageWidget to
 *  popup the scrollbar's menu.  This is a NoOp in Motif.
 ****************************procedure*header*****************************
 */
extern Boolean
_OloSBMenu  OLARGLIST((w, event))
    OLARG(Widget, w)
    OLGRA(XEvent *, event)
{
    ScrollbarWidget sw = (ScrollbarWidget)w;

    if ((SBW.timerid == NULL) &&
	((SBW.opcode == NOOP) || !(SBW.opcode & ANCHOR))) {
	Position root_x, root_y, init_x, init_y;

        if (SBW.popup == NULL)  {
            SBW.popup = _OloSBCreateMenu(w, SBW.orientation);
        }
	
	if (event)  {
		/*  The menu is invoked from a button press.  */
                sw->scroll.XorY = HORIZ(sw)? event->xbutton.x:event->xbutton.y;
		SetMenuItemsSensitive(sw, True);
		root_x = event->xbutton.x_root;
		root_y = event->xbutton.y_root;
		init_x = event->xbutton.x;
		init_y = event->xbutton.y;
	}
	else {
		/* When the menu is invoked from the keyboard some items are
		   made inactive.  */
		SetMenuItemsSensitive(sw, False);

		init_x = SBW.offset + SBW.anchwidth;
		init_y = SBW.sliderPValue;
		if (HORIZ(sw)) {
			Position tmp = init_x;
			init_x = init_y;
			init_y = tmp;
		}
		XtTranslateCoords((Widget)sw, init_x, init_y, &root_x, &root_y);
	}

	OlPostPopupMenu((Widget)sw, sw->scroll.popup,
		((event) ? OL_MENU : OL_MENUKEY),
		(OlPopupMenuCallbackProc) NULL,
		root_x, root_y,
		init_x, init_y);

    	/*  Returning True means that the event was consumed.  */
    	return(True);
    }
    else 
	return(False);

}  /* end of _OloSBMenu() */

/*
 *************************************************************************
 * _OloSBFindOp - called from the SelectDown to determine the type of
 *  operation the user intends.
 ****************************procedure*header*****************************
 */
extern Boolean
_OloSBFindOp OLARGLIST((w, event, op_p))
    OLARG(Widget, w)
    OLARG(XEvent *, event)
    OLGRA(unsigned char *, op_p)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
	int point;
	int bottomAnchorPos, topAnchorPos, elevEndPos;
	unsigned char opcode = NOOP;
	int	partSize;

	/* Throw out events not directly over the scrollbar */
	if (HORIZ (sw))
	{
	    if (event->xbutton.y < sw->scroll.offset ||
		event->xbutton.y >= sw->scroll.offset +
		    (Position) sw->scroll.anchlen)
		return(False);
	    point = event->xbutton.x;
	    topAnchorPos = sw->scroll.anchwidth;
	    bottomAnchorPos = sw->core.width - sw->scroll.anchwidth;
	    elevEndPos = sw->scroll.elevwidth;
	    partSize = (int) (sw->scroll.elevwidth+1) /
		(int) (sw->scroll.type&EPARTMASK);
	}
	else
	{
	    if (event->xbutton.x < sw->scroll.offset ||
		event->xbutton.x >= sw->scroll.offset +
		    (Position) sw->scroll.anchwidth)
		return(False);
	    point = event->xbutton.y;
	    topAnchorPos = sw->scroll.anchlen;
	    bottomAnchorPos = sw->core.height - sw->scroll.anchlen;
	    elevEndPos = sw->scroll.elevheight;
	    partSize = (int) (sw->scroll.elevheight+1) /
		(int) (sw->scroll.type&EPARTMASK);
	}

	if (sw->scroll.type != SB_MINIMUM) {
       		if (point < topAnchorPos)
               		opcode = ANCHOR_TOP;
       		else if (point >= bottomAnchorPos)
               		opcode = ANCHOR_BOT;
		else
               		/* all subsequent checks are relative to elevator */
               		point -= sw->scroll.sliderPValue;
	}

	if (opcode == NOOP) {
               if (point < 0)
                       opcode = PAGE_DEC;
               else if (point < partSize)
                       opcode = GRAN_DEC;
               else if (point >= elevEndPos)
                       opcode = PAGE_INC;
	       else if (point < elevEndPos - partSize)
               	       opcode = DRAG_ELEV;
	       else
		       opcode = GRAN_INC;
       }

	/* if need to show page indicator, get the abs coord of the scrollbar
	   to be used by _OloSBUpdatePageInd().  */
	if ((sw->scroll.showPage != OL_NONE) && (opcode == DRAG_ELEV)) {
		sw->scroll.absx = event->xbutton.x_root - event->xbutton.x;
		sw->scroll.absy = event->xbutton.y_root - event->xbutton.y;
	}

       if (opcode == DRAG_ELEV) {
		if (sw->scroll.type == SB_REGULAR) {
			/* record pointer based pos. for dragging */
			sw->scroll.dragbase = SBW.sliderPValue - (HORIZ(sw) ?
				   event->xbutton.x : event->xbutton.y) -
				   topAnchorPos;
		}
	}

	*op_p = opcode;

	/*  Retrun true if event is to be consumed; otherwise false. */
	return(True);

}  /* end of _OloSBFindOp() */
