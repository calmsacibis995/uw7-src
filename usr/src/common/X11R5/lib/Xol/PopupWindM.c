#ident	"@(#)popupwindo:PopupWindM.c	1.13"

/****************************************************************************
 *
 *	PopupWindM.c:
 *		Motif-mode GUI-specific code for PopupWindowShell
 *
 *
 ****************************************************************************/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/PopupMenu.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <PopupWindP.h>

static void MenuSelect OL_ARGS((Widget, XtPointer, XtPointer));

/****************************************************************************
 *
 *	_OlmPWAddButtons:
 *		Add (Flat) Buttons to the lower control area of the
 * 		PopupWindowShell for the callback lists which are
 *		non-NULL. In addition, a popup MenuShell is created
 *		with the same buttons so the buttons can be activated
 *		with the mouse from within the upper and lower control
 *		areas (This is currently broken, because the child
 *		widgets of the  controlareas consume the ButtonPress
 *		events
 *
 ****************************************************************************/

static char * fields[] = {
	XtNlabel, XtNmnemonic, XtNmanaged
#if	!defined(ITEM_MANAGED_WORKS)
	,XtNuserData
#endif
};

typedef struct { XtArgVal label, mnemonic, managed
#if	!defined(ITEM_MANAGED_WORKS)
	,user_data;
#endif
} Item;

void
_OlmPWAddButtons OLARGLIST((w))
	OLGRA(PopupWindowShellWidget, w)
{
    PopupWindowShellPart *popupwindow = &(w->popupwindow);
    Widget fb;
    Arg args[10];
    Item *items;
    Cardinal num_items, n = 0;
    
    if (popupwindow->apply)
	n += 2;
    if (popupwindow->setDefaults)
	n++;
    if (popupwindow->reset)
	n++;
    if (popupwindow->resetFactory)
	n++;
    if (popupwindow->cancel)
	n++;

    if (n > 0){
	    popupwindow->menu = XtCreatePopupShell(
				"olPopupWindowMenu",
				popupMenuShellWidgetClass, 
				(Widget)w, NULL, 0);
	       /* Add an event handler to pop up the menu when
		* the menu button is pressed in the upper controlarea
		*/
	    OlAddDefaultPopupMenuEH((Widget) popupwindow->upperControlArea, 
				    popupwindow->menu);
	    OlAddDefaultPopupMenuEH((Widget) popupwindow->lowerControlArea, 
				    popupwindow->menu);
	    OlAddDefaultPopupMenuEH(XtParent((Widget)popupwindow->upperControlArea),
				    popupwindow->menu);

	    XtSetArg(args[0], XtNitemFields,    fields);
	    XtSetArg(args[1], XtNnumItemFields, XtNumber(fields));
	    XtSetArg(args[2], XtNselectProc,    MenuSelect);
	    XtSetArg(args[3], XtNclientData,    w);
	    
	    fb = XtCreateManagedWidget(
		      "olPopupWindowMenu",
		      flatButtonsWidgetClass,
		      popupwindow->lowerControlArea,
		      args, 4);

	    /*
	     * The ControlArea widget added an XtNpostSelect callback
	     * (per our instructions; see Initialize), which invokes
	     * _OlPWBringDownPopup. This callback is needed for buttons
	     * created directly by the client, but not by the buttons
	     * we create since we call _OlPWBringDownPopup "manually"
	     * and only when appropriate.
	     */
	    XtRemoveAllCallbacks (fb, XtNpostSelect);

	    XtVaGetValues (
		fb,
		XtNitems,    (XtArgVal)&items,
		XtNnumItems, (XtArgVal)&num_items,
		(String)0
	    );

	    items[OK_ITEM].managed = popupwindow->apply != 0;
	    items[APPLY_ITEM].managed = popupwindow->apply != 0;
	    items[SET_DEFAULT_ITEM].managed = popupwindow->setDefaults != 0;
	    items[RESET_ITEM].managed = popupwindow->reset != 0;
	    items[RESET_TO_FACTORY_ITEM].managed = popupwindow->resetFactory != 0;
	    items[CANCEL_ITEM].managed = popupwindow->cancel != 0;

#if	defined(ITEM_MANAGED_WORKS)
	    XtVaSetValues (fb, XtNitemsTouched, (XtArgVal)True, NULL);
#else
	{   Cardinal j;
	    for (j = n = 0; j < num_items; j++)
		if (items[j].managed) {
		    items[n] = items[j];
		    items[n].user_data = (XtArgVal)j;
		    n++;
		}
	    num_items = n;
	    XtVaSetValues (
		fb,
		XtNitems,    (XtArgVal)items,
		XtNnumItems, (XtArgVal)num_items,
		(String)0
	    );
	}
#endif

	    XtSetArg (args[4], XtNitems, items);
	    XtSetArg (args[5], XtNnumItems, num_items);
	    fb = XtCreateManagedWidget(
			 "pane",
			 flatButtonsWidgetClass,
			 popupwindow->menu,
			 args, 6);

	}
} /* end of _OlmPWAddButtons */

/*
 * MenuSelect
 */

static void
MenuSelect OLARGLIST((w, client_data, call_data))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)
	OLGRA( XtPointer,	call_data)
{
	OlFlatCallData * p = (OlFlatCallData *)call_data;
#if	defined(ITEM_MANAGED_WORKS)
	Cardinal which = p->item_index;
#else
	Item * q = (Item *)p-> items;
	Item * r = &q[p-> item_index];
	Cardinal which = (int)(r->user_data);
#endif
	Widget parent = (Widget)client_data;

	switch (which)
		{
		case OK_ITEM:
			XtCallCallbacks(parent, XtNapply, NULL);
			_OlPWBringDownPopup(parent, True);
			break;
		case APPLY_ITEM:
			XtCallCallbacks(parent, XtNapply, NULL);
			break;
		case SET_DEFAULT_ITEM:
			XtCallCallbacks(parent, XtNsetDefaults, NULL);
			break;
		case RESET_ITEM:
			XtCallCallbacks(parent, XtNreset, NULL);
			break;
		case RESET_TO_FACTORY_ITEM:
			XtCallCallbacks(parent, XtNresetFactory, NULL);
			break;
		case CANCEL_ITEM:
			XtCallCallbacks(parent, XtNcancel, NULL);
			_OlPWBringDownPopup(parent, True);
			break;
		}
} /* end of MenuSelect */
