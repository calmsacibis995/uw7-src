#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/openprt.c	1.18.3.2"
#endif

#include <stdlib.h>
#include <stdio.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Xol/ScrolledWi.h>
#include <Xol/Form.h>

#include <DtI.h>
#include <libDtI/FIconBox.h>

#include "properties.h"
#include "lpsys.h"
#include "error.h"

#define JOB_ALLOC_SIZE	10

#define ICONBOX_WIDTH	(5*BOGUS_WIDTH)
#define ICONBOX_HEIGHT	(3*BOGUS_HEIGHT)
#define BOGUS_WIDTH	70
#define BOGUS_HEIGHT	50

enum {
    Actions_Button, PrtReq_Button, Help_Button,
};

enum {
    Delete_Button, Prop_Button,
};


static void	PropertyCB (Widget, XtPointer, XtPointer);
static void	ProtocolCB (Widget, XtPointer, XtPointer);
static void	ExitCB (Widget, XtPointer, XtPointer);
extern void	HelpCB (Widget, XtPointer, XtPointer);
static void	CheckOpen (Widget, XtPointer, XtPointer);
static void	SetButtonState (Widget, Cardinal);
static void	SelectIcon (Widget widget, XtPointer client_data,
			    XtPointer call_data);
static void	LayoutIcons (IconItem *items, int itemCnt,
			     Dimension width, Dimension height);
static Widget	BuildIconBox (Widget parent, Printer *printer, int *itemCnt);
static Boolean	GetPrintQueue (Widget parent, Printer *printer,
			       Dimension width, Dimension Height,
			       IconItem **pItems, int *pItemCnt);
extern void	CheckQueue (XtPointer client_data, XtIntervalId *pId);

extern void	DeleteJobs (Widget, XtPointer, XtPointer);
extern Printer	*GetPrinter (char *name);
static void	MakeBaseWindow (Printer *printer);

static DmGlyphPtr	JobGlyph;
static int		OpenCnt;

extern ResourceRec AppResources;

/* Menu items and fields */

String	MenuFields [] = {
    XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc, XtNdefault,
    XtNuserData, XtNpopupMenu,
};

int	NumMenuFields = XtNumber (MenuFields);

static MenuItem MenuBarItems [] = {
    { (XtArgVal) TXT_actions, (XtArgVal) MNEM_actions, (XtArgVal) True,
	  (XtArgVal) 0, (XtArgVal) True, },		/* Actions */
    { (XtArgVal) TXT_prtreq, (XtArgVal) MNEM_prtreq, (XtArgVal) True,
	  (XtArgVal) 0, },				/* Print Req */
    { (XtArgVal) TXT_help, (XtArgVal) MNEM_help, (XtArgVal) True,
	  (XtArgVal) 0, },				/* Help */
};

/* Icon Box fields and types */
static String IconFields [] = {
    XtNlabel, XtNobjectData, XtNx, XtNy, XtNwidth, XtNheight, XtNset,
    XtNuserData, XtNmanaged,
};

static MenuItem PrtReqItems [] = {
    { (XtArgVal) TXT_delete, (XtArgVal) MNEM_delete, (XtArgVal) False,
	  (XtArgVal) DeleteJobs, },			/* Delete */
    { (XtArgVal) TXT_properties, (XtArgVal) MNEM_properties, (XtArgVal) False,
	  (XtArgVal) PropertyCB, },			/* Properties */
};

static MenuItem ActionItems [] = {
    { (XtArgVal) TXT_exit, (XtArgVal) MNEM_exit, (XtArgVal) True,
	  (XtArgVal) ExitCB, },				/* Exit */
};

static HelpText AppHelp = {
    TXT_appHelp, HELP_FILE, TXT_appHelpSect,
};

static HelpText TOCHelp = {
    TXT_tocHelp, HELP_FILE, 0,
};

static MenuItem HelpItems [] = {
    { (XtArgVal) TXT_application, (XtArgVal) MNEM_application, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) True,
	  (XtArgVal) &AppHelp, },			/* Application */
    { (XtArgVal) TXT_TOC, (XtArgVal) MNEM_TOC, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &TOCHelp, },			/* Table o' Contents */
    { (XtArgVal) TXT_helpDesk, (XtArgVal) MNEM_helpDesk, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) 0, },				/* Help Desk */
};



/* OpenPrinter
 *
 * Open the printer icon and show the current print queue.
 */
void
OpenPrinter (char *name)
{
    Printer		*printer;
    Widget		shell;
    static char		*noPrinterMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;

	noPrinterMsg = GetStr (TXT_noPrinter);
    }

    printer = GetPrinter (name);
    if (!printer)
	ErrorConfirm (TopLevel, noPrinterMsg, CheckOpen,
		      (XtPointer) No_Printer);
    else
    {
	/* If the print queue is already open, simply bring that window
	 * to the top.
	 */
	if (printer->iconbox)
	{
	    shell = XtParent (printer->iconbox);
	    while (!XtIsShell (shell))
		shell = XtParent (shell);

	    /* If the iconbox is posted, then the timeout value will be
	     * non-zero.  Map the shell widget in case the window is minimized.
	     */
	    if (printer->timeout) {
		XtMapWidget (shell);
		XRaiseWindow (XtDisplay (shell), XtWindow (shell));
	    } else
	    {
		XtMapWidget (shell);
		OpenCnt++;
	    }
	    CheckQueue (printer, (XtIntervalId *) 0);
	}
	else
	{
	    MakeBaseWindow (printer);
	    OpenCnt++;
	}
    }
}	/* End of OpenPrinter () */

/* MakeBaseWindow
 *
 * Create a new base window with menu bar and icon box for displaying the
 * printer queue.
 */
static void
MakeBaseWindow (Printer *printer)
{
    Widget		form;
    Widget		menuBar;
    Widget		base;
    MenuItem		*barItems;
    MenuItem		*prtreqItems;
    MenuItem		*actionItems;
    DmGlyphPtr		glyph;
    int			size;
    int			items;
    char		title [128];
    register		i;
    static Boolean	first = True;
    static char		*titleLbl;
    static Pixmap	icon;
    static Pixmap	mask;

    if (first)
    {
	first = False;

	titleLbl = GetStr (TXT_pgmTitle);

	SetLabels (MenuBarItems, XtNumber (MenuBarItems));
	SetLabels (PrtReqItems, XtNumber (PrtReqItems));
	SetLabels (ActionItems, XtNumber (ActionItems));
	SetLabels (HelpItems, XtNumber (HelpItems));
	SetHelpLabels (&AppHelp);
	SetHelpLabels (&TOCHelp);

	glyph = DmGetPixmap (XtScreen (TopLevel), AppResources.procIcon);
	if (glyph)
	{
	    icon = glyph->pix;
	    mask = glyph->mask;
	}
	else
	    icon = mask = (Pixmap) 0;
    }

    base = XtAppCreateShell (printer->name, printer->name,
			     topLevelShellWidgetClass, XtDisplay (TopLevel),
			     0, 0);

    OlAddCallback (base, XtNwmProtocol, ProtocolCB, (XtPointer) printer);

    form = XtVaCreateManagedWidget ("form", formWidgetClass, base,
		0);

    /* Because there can be multiple base windows, and the states of the
     * buttons within the windows can differ, we must make a copy of the
     * items.  Because help is common on all windows, it is not necessary
     * to copy the help items.
     */
    size = XtNumber (MenuBarItems) * sizeof (MenuItem);
    barItems = (MenuItem *) XtMalloc (size);
    memcpy (barItems, MenuBarItems, size);

    size = XtNumber (PrtReqItems) * sizeof (MenuItem);
    prtreqItems = (MenuItem *) XtMalloc (size);
    memcpy (prtreqItems, PrtReqItems, size);

    size = XtNumber (ActionItems) * sizeof (MenuItem);
    actionItems = (MenuItem *) XtMalloc (size);
    memcpy (actionItems, ActionItems, size);

    barItems [Actions_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("actionsMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    (void) XtVaCreateManagedWidget ("actionsMenu",
		flatButtonsWidgetClass,
		(Widget) barItems[Actions_Button].subMenu,
		XtNclientData,		(XtArgVal) printer,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) actionItems,
		XtNnumItems,		(XtArgVal) XtNumber (ActionItems),
		0);

    barItems [PrtReq_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("prtreqMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    printer->prtreqMenu = XtVaCreateManagedWidget ("prtreqMenu",
		flatButtonsWidgetClass,
		(Widget) barItems[PrtReq_Button].subMenu,
		XtNclientData,		(XtArgVal) printer,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) prtreqItems,
		XtNnumItems,		(XtArgVal) XtNumber (PrtReqItems),
		0);

    barItems [Help_Button].subMenu =
	(XtArgVal) XtVaCreatePopupShell ("helpMenuShell",
		popupMenuShellWidgetClass, form,
		0);

    (void) XtVaCreateManagedWidget ("helpMenu",
		flatButtonsWidgetClass,
		(Widget) barItems[Help_Button].subMenu,
		XtNclientData,		(XtArgVal) printer,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) HelpItems,
		XtNnumItems,		(XtArgVal) XtNumber (HelpItems),
		0);

    menuBar = XtVaCreateManagedWidget ("menuBar",
		flatButtonsWidgetClass, form,
		XtNvPad,		(XtArgVal) 6,
		XtNhPad,		(XtArgVal) 6,
		XtNmenubarBehavior,	(XtArgVal) True,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) barItems,
		XtNnumItems,		(XtArgVal) XtNumber (MenuBarItems),
		0);

    printer->iconbox = BuildIconBox (form, printer, &items);
    printer->timeout = XtAppAddTimeOut (AppContext, AppResources.timeout*1000,
					CheckQueue, printer);

    sprintf (title, titleLbl, printer->name);
    XtVaSetValues (base,
		XtNiconPixmap,		(XtArgVal) icon,
		XtNiconMask,		(XtArgVal) mask,
		XtNiconName,		(XtArgVal) printer->name,
		XtNtitle,		(XtArgVal) title,
		0);
    if (items)  {
    	OlVaFlatSetValues(printer->iconbox, 0, XtNset, True, 0);
    	SetButtonState (printer->prtreqMenu, 1);
    }
    XtRealizeWidget (base);
}	/* End of MakeBaseWindow () */

/* BuildIconBox
 *
 * Make Icon box and fill with currently known print jobs for a printer.
 */
static Widget
BuildIconBox (Widget parent, Printer *printer, int *itemCnt)
{
    IconItem		*items;
    Widget		scrolledWin;
    Widget		iconbox;
    extern void		DrawIcon ();

    items = (IconItem *) 0;
    *itemCnt = 0;
    (void) GetPrintQueue (parent, printer, ICONBOX_WIDTH, ICONBOX_HEIGHT,
			  &items, itemCnt);

    scrolledWin = XtVaCreateManagedWidget ("scrolledWin",
		scrolledWindowWidgetClass, parent,
		XtNyRefName,		(XtArgVal) "menuBar",
		XtNyAddHeight,		(XtArgVal) True,
		XtNxAttachRight,	(XtArgVal) True,
		XtNyAttachBottom,	(XtArgVal) True,
		XtNxResizable,		(XtArgVal) True,
		XtNyResizable,		(XtArgVal) True,
		XtNviewWidth,		(XtArgVal) ICONBOX_WIDTH,
		XtNviewHeight,		(XtArgVal) ICONBOX_HEIGHT,
		0);

    iconbox = XtVaCreateManagedWidget ("iconbox", flatIconBoxWidgetClass,
		scrolledWin,
		XtNdrawProc,		(XtArgVal) DrawIcon,
                XtNdblSelectProc,       (XtArgVal) PropertyCB,
		XtNpostSelectProc,	(XtArgVal) SelectIcon,
		XtNpostAdjustProc,	(XtArgVal) SelectIcon,
		XtNclientData,		(XtArgVal) printer,
		XtNmovableIcons,	(XtArgVal) False,
		XtNitemFields,		(XtArgVal) IconFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (IconFields),
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) *itemCnt,
		0);

    return (iconbox);
}	/* End of BuildIconBox () */

/* SelectIcon
 *
 * Set the select state of an icon in the icon box depending on whether the
 * the select or adjust button was pressed.  Update the state of the menu
 * buttons depending on the number of items selected.  client_data is a
 * pointer to the owning printer.  call_data is a pointer to a Flat Icon
 * box call data structure.
 */
static void
SelectIcon (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFIconBoxButtonCD	*iconData = (OlFIconBoxButtonCD *) call_data;
    Printer		*printer = (Printer *) client_data;
    IconItem		*item;
    IconItem		*selectedItem;
    register		i;
    int			numSelectedIcons;

    /* Count the number of selected items */
    numSelectedIcons = 0;
    item = (IconItem *) iconData->item_data.items;
    for (i=0; i<iconData->item_data.num_items; i++, item++)
    {
	if (item->managed && item->selected)
	{
	    selectedItem = item;
	    numSelectedIcons++;
	}
    }

    SetButtonState (printer->prtreqMenu, numSelectedIcons);

    /* If the property sheet for this printer is posted, update the
     * properties to reflect the new selection.
     */
    if (printer->page.poppedUp)
    {
	if (numSelectedIcons == 1)
	    PostProperties (widget, (Properties *) selectedItem->properties);
	else if (numSelectedIcons == 0)
	    printer->page.posted = (Properties *) 0;
    }
}	/* End of SelectIcon () */

/* SetButtonState
 *
 * Set the state of the delete and properties button in the prtreq menu.
 * The delete button is turned on when at least one icon is selected.  The
 * property button is on only when exactly one icon is selected.
 */
static void
SetButtonState (Widget menu, Cardinal numSelected)
{
    MenuItem	*items;

    XtVaGetValues (menu,
		XtNitems,		(XtArgVal) &items,
		0);

    if (numSelected > 0)
    {
	if (!items [Delete_Button].sensitive)
	    OlVaFlatSetValues (menu, Delete_Button,
			       XtNsensitive,	(XtArgVal) True,
			       0);
    }
    else
    {
	if (items [Delete_Button].sensitive)
	    OlVaFlatSetValues (menu, Delete_Button,
			       XtNsensitive,	(XtArgVal) False,
			       0);
    }

    if (numSelected == 1)
    {
	if (!items [Prop_Button].sensitive)
	    OlVaFlatSetValues (menu, Prop_Button,
			       XtNsensitive,	(XtArgVal) True,
			       0);
    }
    else
    {
	if (items [Prop_Button].sensitive)
	    OlVaFlatSetValues (menu, Prop_Button,
			       XtNsensitive,	(XtArgVal) False,
			       0);
    }
}	/* End of SetButtonState () */

/* GetPrintQueue
 *
 * Get the list of flat items for the active printer jobs.  If *pItemCnt
 * and *pItems are not zero, then update the printer queue.  Return True if
 * the queue changed from the values passed in.  Note that width and height
 * are not tracked; that is, if the width or height changes, but the queue
 * does not, then the function returns False and does not layout the icons
 * again.
 */
static Boolean
GetPrintQueue (Widget widget, Printer *printer,
	       Dimension width, Dimension height,
	       IconItem **pItems, int *pItemCnt)
{
    IconItem	*item;
    IconItem	*freeItem;
    PrintJob	*job;
    int		pos;
    Boolean	changed;
    register	i;

    if (!JobGlyph)
    {
	JobGlyph = DmGetPixmap (XtScreen (widget), AppResources.icon);
	if (!JobGlyph)
	    JobGlyph = DmGetPixmap (XtScreen (widget), NULL);
    }

    /* Mark all items in the old list as deleted from the queue.  Use a
     * negative position to indicate deleted.
     */
    for (item=*pItems, pos=0; pos<*pItemCnt; pos++, item++)
	if (item->managed)
	    ((Properties *) item->properties)->rank =
		(XtArgVal) -(((Properties *) item->properties)->rank);

    /* Get the jobs from lp.  Add it to the item list, checking that it is
     * not already there.  Mark it as active by its position in the queue.
     */
    pos = 1;
    changed = False;
    while (job = LpJobs (printer->name))
    {
	if (!(job->outcome & RS_CANCELLED))
	{
	    freeItem = (IconItem *) 0;
	    for (i=0, item=*pItems; i<*pItemCnt; i++, item++)
		if (item->managed)
		{
		    if (strcmp (((Properties *)item->properties)->id,
				job->id) == 0)
		    {
			if (-pos != ((Properties *) item->properties)->rank)
			    changed = True;
			((Properties *) item->properties)->rank = pos;
			break;
		    }
		}
		else
		    freeItem = item;

	    if (i >= *pItemCnt)
	    {
		/* Id not found in previous list--make a new entry. */
		changed = True;
		if (!freeItem)
		{
		    *pItemCnt += JOB_ALLOC_SIZE;
		    *pItems = (IconItem *) XtRealloc ((char *) *pItems,
					      *pItemCnt * sizeof(**pItems));
		    for (item=*pItems+*pItemCnt, i=JOB_ALLOC_SIZE; --i>=0; )
			(--item)->managed = (XtArgVal) False;
		    item = *pItems + pos - 1;
		}
		else
		    item = freeItem;

		item->lbl = (XtArgVal) strdup (job->user);
		item->selected = (XtArgVal) False;
		item->glyph = (XtArgVal) JobGlyph;
		item->properties = (XtArgVal) XtCalloc(1, sizeof (Properties));
		item->managed = (XtArgVal) True;
		((Properties *) item->properties)->rank = pos;
		((Properties *) item->properties)->printer = printer;
		((Properties *) item->properties)->page = &printer->page;
		InitProperties (widget, (Properties *) item->properties, job);
	    }
	    else
	    {
		/* Item was found in list.  Update the values from the job. */
		InitProperties (widget, (Properties *) item->properties, job);
	    }

	    pos++;
	}

	XtFree (job->id);
	XtFree (job->printer);
	XtFree (job->character_set);
	XtFree (job->form);
	XtFree (job->user);
    }

    /* Remove any old jobs that are no longer in the queue. */
    for (i=0, item=*pItems; i<*pItemCnt; i++, item++)
    {
	if (item->managed)
	{
	    if (((Properties *) item->properties)->rank < 0)
	    {
		/* The request is gone.  Clean up after it, checking if the
		 * property sheet is posted.
		 */
		if (printer->page.posted == (Properties *) item->properties)
		    printer->page.posted = (Properties *) 0;

		XtFree ((char *) item->lbl);
		FreeProperties ((Properties *) item->properties);
		item->managed = (XtArgVal) False;
		changed = True;
	    }
	}
    }

    if (changed)
	LayoutIcons (*pItems, *pItemCnt, width, height);
    return (changed);
}	/* End of GetPrintQueue () */

/* CheckQueue
 *
 * Update the print queue for a printer and schedule the next update.
 * client_data is a pointer to a Printer.
 */
void
CheckQueue (XtPointer client_data, XtIntervalId *pId)
{
    Printer	*printer = (Printer *) client_data;
    IconItem	*items;
    int		cnt;
    int		selectCnt = 0, shown = 0, i = 0;
    Dimension	width;
    Dimension	height;

    /* If pId is zero, then we were not called by the timeout, so remove
     * the timeout event, if any.
     */
    if (!pId && printer->timeout) {
	XtRemoveTimeOut (printer->timeout);
	printer->timeout = (XtIntervalId) 0;
    }

    XtVaGetValues (XtParent (printer->iconbox),
		XtNwidth,		(XtArgVal) &width,
		XtNheight,		(XtArgVal) &height,
		0);

    XtVaGetValues (printer->iconbox,
		XtNitems,		(XtArgVal) &items,
		XtNnumItems,		(XtArgVal) &cnt,
		0);

    if (GetPrintQueue (printer->iconbox, printer, width, height, &items, &cnt))
	XtVaSetValues (printer->iconbox,
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) cnt,
		XtNitemsTouched,	(XtArgVal) True,
		0);

    printer->timeout = XtAppAddTimeOut (AppContext, AppResources.timeout*1000,
					CheckQueue, printer);

    for (i = 0; i < cnt; i++, items++)
        if (items->managed == 1) {
	    shown = i ;
	    if (items->selected == 1)
	      selectCnt++;
	}

    if (selectCnt == 0 && shown != 0) {
        OlVaFlatSetValues(printer->iconbox, shown, XtNset, True, 0);
	selectCnt = 1;
    }
    SetButtonState(printer->prtreqMenu, selectCnt);
}	/* End of CheckQueue () */

/* LayoutIcons
 *
 * Layout the items in an icon box in an orderly fashion.  Put as many items
 * on a row as will fit.  Height is ignored.
 */
static void
LayoutIcons (IconItem *pItems, int itemCnt, Dimension width, Dimension height)
{
    IconItem	*item;
    int		cols;
    int		position;
    register	i;

    cols = width / BOGUS_WIDTH;
    if (cols < 1)
	cols = 1;
    for (item=pItems, i=itemCnt; --i>=0; item++)
    {
	if (item->managed)
	{
	    position = ((Properties *) item->properties)->rank - 1;
	    item->x = (XtArgVal) ((position % cols) * BOGUS_WIDTH);
	    item->y = (XtArgVal) ((position / cols) * BOGUS_HEIGHT);
	    item->width = (XtArgVal) BOGUS_WIDTH;
	    item->height = (XtArgVal) BOGUS_HEIGHT;
	}
    }
}	/* End of LayoutIcons () */

/* PropertyCB
 *
 * Display the property sheet for the printer.  client_data is a pointer to
 * the printer.
 */
static void
PropertyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Printer	*printer = (Printer *) client_data;
    IconItem	*item;
    Cardinal	cnt;
    register	i;

    XtVaGetValues (printer->iconbox,
		XtNitems,		(XtArgVal) &item,
		XtNnumItems,		(XtArgVal) &cnt,
	        0);

    for (i=cnt; --i>=0; item++)
    {
	if (item->managed && item->selected)
	    break;
    }

    PostProperties (widget, (Properties *) item->properties);
}	/* End of PropertyCB () */

/* ProtocolCB
 *
 * Handle window manager protocols.  In this case, the only one we care
 * about is DELETE_WINDOW.  Unmap the relevant window.  When the last
 * one is closed, die.  client_data is a pointer to the Printer.
 */
static void
ProtocolCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Printer		*printer = (Printer *) client_data;
    OlWMProtocolVerify	*cd = (OlWMProtocolVerify *) call_data;

    if (cd->msgtype != OL_WM_DELETE_WINDOW)
	return;

    ExitCB (widget, client_data, call_data);
}	/* End of ProtocolCB () */

/* ExitCB
 *
 * Similar to ProtocolCB, except called when the exit button is pressed in
 * the menu.  Unmap the relevant window.  When the last one is closed, die.
 * client_data is a pointer to the Printer.
 */
static void
ExitCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Printer		*printer = (Printer *) client_data;
    Widget		shell;

    XtRemoveTimeOut (printer->timeout);
    printer->timeout = (XtIntervalId) 0;

    shell = XtParent (printer->iconbox);
    while (!XtIsShell (shell))
	shell = XtParent (shell);

    XtUnmapWidget (shell);

    if (--OpenCnt <= 0)
	exit (0);
}	/* End of ExitCB () */

/* HelpCB
 *
 * Display help.  userData in the item is a pointer to the HelpText data.
 */
void
HelpCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
    MenuItem		*selected;

    selected = (MenuItem *) flatData->items + flatData->item_index;
    DisplayHelp (widget, (HelpText *) selected->userData);
}	/* End of HelpCB () */

/* CheckOpen
 *
 * Check if any print queue windows are open.  If not, quit.
 * client_data is the return code.
 */
static void
CheckOpen (Widget widget, XtPointer client_data, XtPointer call_data)
{
    if (OpenCnt <= 0)
	exit ((int) client_data);
}	/* End of ProtocolCB () */
