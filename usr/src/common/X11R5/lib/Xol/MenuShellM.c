/*		copyright	"%c%" 	*/

#ifndef NOIDENT
#ident	"@(#)menu:MenuShellM.c	1.9"
#endif

/*
 * MenuShellM.c - This file contains the Motif-mode versions of
 *	the PopupMenuShell's look and feel functions.
 *
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/PopupMenuP.h>
#include <Xol/FButtonsP.h>
#include <Xol/VendorI.h>

static void StayUpCB OL_ARGS((Widget, XtPointer, XtPointer));

static void	WMMessageHandler();

static void SwapButtons OL_ARGS((Widget, int));

#define HAS_TITLE(nw)   (((PopupMenuShellWidget)(nw))->wm.title != NULL && strcmp((char *)((PopupMenuShellWidget)nw->wm.title), " "))
#define PAD	4
#define STAY_UP	0
#define DISMISS	1
#define NPART(nw,field) ((nw)-> popup_menu_shell.field)

static char * fields[] = { XtNlabel };
typedef struct { XtArgVal label; } Item;

/*
 * _OlmMSHandlePushpin - In Motif mode, the pushpin is replaced
 *	by a button whose label toggles between "Stay Up" and
 *	"Dismiss".  Rather than draw the button and have event
 *	handlers duplicating the button code, a flatButton widget
 *	is created with a callback that manages the "pinning".
 *
 */
/* ARGSUSED */
void
_OlmMSHandlePushpin OLARGLIST((nw, nPart, pin_state, size_or_draw))
	OLARG( PopupMenuShellWidget,	nw)
	OLARG( PopupMenuShellPart *,	nPart)
	OLARG( OlDefine, 		pin_state)
	OLGRA( char,			size_or_draw)
{
	if (size_or_draw == 's')  {
		nPart->pin.x = 0;
		nPart->pin.y = 0;
		nPart->pin.width = (pin_state != OL_NONE) ? 1 : 0;
		nPart->pin.height = 0;
	}

} /* end of _OlmMSHandlePushpin */

/*
 * _OlmMSHandleTitle
 *
 */
void
_OlmMSHandleTitle OLARGLIST((nw, nPart, size_or_draw))
	OLARG( PopupMenuShellWidget,	nw)
	OLARG( PopupMenuShellPart *,	nPart)
	OLGRA( char,			size_or_draw)
{
  OlgTextLbl labeldata;
  int shadow_thickness = nPart->shadow_thickness;
  XRectangle * title = &(nPart->title);

  if (HAS_TITLE(nw)) {
     labeldata.label         = nw-> wm.title;
     labeldata.normalGC      =
     labeldata.inverseGC     = nPart-> gc;
     labeldata.font          = nPart-> font;
     labeldata.accelerator   = NULL;
     labeldata.mnemonic      = '\0';
     labeldata.justification = TL_CENTER_JUSTIFY;
     labeldata.flags         = 0;
     labeldata.font_list     = nPart->font_list;

     if (size_or_draw == 'd') {
	/*  The Motif mode title is drawn centered in the space at the
	    top of the menu.  This code also draws the SHADOW_OUT border
	    around the title.  */
	int width = nw->core.width - shadow_thickness * 2;

        OlgDrawTextLabel(XtScreen(nw), XtWindow(nw), nPart-> attrs, 
           title->x, title->y, width, title->height, &labeldata);

	/*  The title.width and title.height account for the space needed
	    for the border shadow.  */
	if (nPart->shadow_thickness != 0)
		OlgDrawBorderShadow( XtScreen(nw), XtWindow(nw),
			nPart->attrs, OL_SHADOW_OUT,
			shadow_thickness,
			(Position)shadow_thickness,
			(Position)shadow_thickness,
			width,
			title->height);
     }
     else  {
	/*  The Motif-mode title is the size of the label plus the shadow
	    thickness space.  */
	Dimension width, height;

        OlgSizeTextLabel(XtScreen(nw), nPart-> attrs, &labeldata,
           &width, &height);
	title->width = width + (shadow_thickness * 2) + (PAD * 2);
	title->height = height + (shadow_thickness * 2) + (PAD * 2);
	title->x = title->y = shadow_thickness * 2;
     }
  }
} /* end of _OlmMSHandleTitle */

/**
 ** _OlmMSLayoutPreferred()
 **
 ** Compute the preferred size of the menu based on the title and the
 ** preferred size of the managed children.
 **/

extern void
_OlmMSLayoutPreferred OLARGLIST((_w, who_asking, request, width, height, xpos, ypos, child_space))
	OLARG(Widget,			_w)
	OLARG(Widget,			who_asking)
	OLARG(XtWidgetGeometry *,	request)
	OLARG(Dimension *,		width)
	OLARG(Dimension *,		height)
	OLARG(Position *,		xpos)
	OLARG(Position *,		ypos)
	OLGRA(Dimension *,		child_space)
{
	PopupMenuShellWidget	w = (PopupMenuShellWidget)_w;

	Cardinal		nchildren    = w->composite.num_children;
	Cardinal		n;

	Widget *		pchild	     = w->composite.children;
	Widget *		p;

	XtWidgetGeometry	prefer;

	Dimension		shadow_thickness = NPART(w, shadow_thickness);

	/*  Compute the optimum fit based on the size of the title,
	    margins, and width of children.
	*/

	/*  The minimum height starts with the shadow thickness. */
	*width = 0;
	*height = shadow_thickness;

	/*  When overried_redirect is False the window manager accounts for
	    the title and pushpin.  */
	if (HAS_TITLE(w) && w->shell.override_redirect)  {
		/*  Motif menu titles have no margins.  */
		*height += NPART(w, title.height);

		/*  Add on the width of the title.  */
		*width += NPART(w, title.width);
	}

	/*  Later, when the children are configured, ypos is the position
	    of the first child.  */
	*ypos = *height;

	/*  Add on to the height the shadow at the bottom */
	*height += shadow_thickness;

	/*  The width and height now have the prefered size based on the
	    size of the title.  Now we ask each managed child its prefered
	    size.  When keep track of the max width so that it can be
	    compared to the header's width.  The height is accumulated as
	    we ask each child.  */

	for (p = pchild, n = 0; n < nchildren; n++, p++)  {
		if (XtIsManaged(*p))  {
			if (who_asking == *p)  {
				/*  Allow the child's request.  */
				*height += request->height;
				*width = _OlMax((int)(*width), (int) request->width);
			}
			else  {
				/*  Don't care about the answer because prefer
				    will always get the preferred or current
				    geometry.  */
				XtQueryGeometry(*p, NULL, &prefer);

				/*  To avoid caching the heights or querying
				    twice, I assume that the core.height is
				    adequate and use it when the widget is
				    configrued as well.  */
				*height += (*p)->core.height;

				*width = _OlMax((int)(*width), (int) prefer.width);
			}
		}
	}

	/*  Add on the shadow thickness.  */
	*width += 2 * shadow_thickness;

	/*  Set the x position to 0 so that menu items span the width.  */
	*xpos = shadow_thickness;

	/*  There is no space between items in Motif mode. */
	*child_space = 0;

	return;
} /* _OlmMSLayout */

/* ARGSUSED */
static void
StayUpCB OLARGLIST((w, client_data, call_data))
	OLARG(Widget, w)
	OLARG(XtPointer, client_data)
	OLGRA(XtPointer, call_data)
{
        (void) OlActivateWidget(XtParent(w), OL_TOGGLEPUSHPIN,
			(XtPointer)NULL);
}  /* end of StayUpCB() */

/*ARGSUSED*/
static void
DestroyCB OLARGLIST((w, client_data, call_data))
	OLARG(Widget, w)
	OLARG(XtPointer, client_data)
	OLGRA(XtPointer, call_data)
{
				/* Remove the WMMessage Handler		*/
	XtRemoveEventHandler(XtParent(w), (EventMask)PropertyChangeMask, TRUE,
		     WMMessageHandler, (XtPointer)w);

}

void
_OlmMSCreateButton OLARGLIST((w))
	OLGRA(Widget, w)
{
        Arg arg[10];
	Cardinal n = 0;
	Widget button;
	OlVendorPartExtension	part = _OlGetVendorPartExtension(w);
	Item *items;

	/*
	 * With the help of the toolkit database, create a FlatButton
	 * with two items, the Stay-up and Dismiss buttons (in that
	 * order). Then, if necessary, swap the button labels so that
	 * the operative one is first. Finally, change the number of
	 * buttons so that only one shows up.
	 *
	 * These buttons implement a "pushpin" for Motif menus.
	 */

        XtSetArg(arg[0], XtNitemFields, fields);
        XtSetArg(arg[1], XtNnumItemFields, XtNumber(fields));
        XtSetArg(arg[2], XtNselectProc, StayUpCB);
        XtSetArg(arg[3], XtNuserData, STAY_UP);
        button = XtCreateManagedWidget("olMenuShellStayUp", flatButtonsWidgetClass,
			w, arg, 4);

	/*
	 * The above created a flat button with two items, Stay Up and
	 * Dismiss. Change the size of the list (but not its content)
	 * to one item. By default this will be the Stay Up button, but
	 * if the client/user has asked that the menu comes up pinned,
	 * then the WMMessageHandler will detect the inconsistency and
	 * fix the label.
	 */
	XtSetArg(arg[0], XtNnumItems, 1);
	XtSetValues(button, arg, 1);

	XtAddCallback(button, XtNdestroyCallback, DestroyCB, (XtPointer)0);

	XtAddEventHandler(w, (EventMask)PropertyChangeMask,
			True, WMMessageHandler, (XtPointer) button);

}  /* end of _OlmMSCreateButton() */

/* ARGSUSED */
static void
WMMessageHandler(widget, data, xevent, cont_to_dispatch)
        register Widget widget;
        XtPointer       data;
        XEvent *        xevent;
        Boolean *       cont_to_dispatch;
{
	Arg arg;
	XtPointer state;
	OlVendorPartExtension	part = 
			_OlGetVendorPartExtension(widget);
	Widget w = (Widget)data;


	/*
	 * This handler will be called whenever a property on the menu
	 * shell changes, or whenever a client message is sent to the
	 * menu shell. Any such event typically means the window manager
	 * or the toolkit has changed the state of the shell (e.g. changed
	 * the state of the pushpin, changed from override-redirect to
	 * transient, WM_DELETE_WINDOW, etc.) At any time, if the state
	 * of the pushpin differs from the label, swap the label to bring
	 * it in sync with the pushpin.
	 */

	XtSetArg(arg, XtNuserData, &state);
	XtGetValues(w, &arg, 1);

	if (part->pushpin == OL_IN && (int)state == STAY_UP)
		SwapButtons(w, (int)DISMISS);
	else if (part->pushpin == OL_OUT && (int)state == DISMISS)
		SwapButtons(w, (int)STAY_UP);
}  /* end of WMMessageHandler() */

static void
SwapButtons OLARGLIST((w, state))
	OLARG(Widget, 		w)
	OLGRA(int,		state)
{
	Arg			args[2];

	Item *			items;

	XtArgVal		tmp;


	XtSetArg (args[0], XtNitems, &items);
	XtGetValues (w, args, 1);

	tmp = items[0].label;
	items[0].label = items[1].label;
	items[1].label = tmp;

	XtSetArg (args[0], XtNitemsTouched, True);
	XtSetArg (args[1], XtNuserData, state);
	XtSetValues (w, args, 2);

	return;
} /* SwapButtons */
