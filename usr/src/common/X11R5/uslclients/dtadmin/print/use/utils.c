#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/utils.c	1.15.1.1"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <X11/Shell.h>

#include <Xol/Caption.h>
#include <Xol/MenuShell.h>
#include <Xol/ControlAre.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/AbbrevButt.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>

#include <Desktop.h>

#include "properties.h"
#include "error.h"

enum {
    Inches_Button, Centimeters_Button, Units_Button,
};

char	*AppName;
char	*AppTitle;

static void	DestroyText (Widget, XtPointer, XtPointer);
static void	ButtonSelectCB (Widget, XtPointer, XtPointer);
extern void	AbbrevSelectCB (Widget, XtPointer, XtPointer);
extern void	LocaleSelectCB (Widget, XtPointer, XtPointer);
static void	DestroyCheckCB (Widget, XtPointer, XtPointer);
static void     ListSelectCB (Widget , XtPointer , XtPointer );
 

static String	ButtonFields [] = {
    XtNuserData, XtNlabel,
};

ButtonItem UnitItems [] = {
    { (XtArgVal) 'i', (XtArgVal) TXT_in, },		/* in. */
    { (XtArgVal) 'c', (XtArgVal) TXT_cm, },		/* cm */
    { (XtArgVal) ' ', (XtArgVal) TXT_chars, },		/* chars */
};

/* FooterMsg
 *
 * Display a message in the footer.
 */
void
FooterMsg (Widget widget, char *msg)
{
    XtVaSetValues (widget,
		XtNstring,		(XtArgVal) msg,
		0);
}	/* End of FooterMsg () */

/* DisplayHelp
 *
 * Send a message to dtm to display a help window.  If help is NULL, then
 * ask dtm to display the help desk.
 */
void
DisplayHelp (Widget widget, HelpText *help)
{
    DtRequest			*req;
    static DtDisplayHelpRequest	displayHelpReq;
    Display			*display = XtDisplay (widget);
    Window			win = XtWindow (XtParent (XtParent (widget)));

    req = (DtRequest *) &displayHelpReq;
    displayHelpReq.rqtype = DT_DISPLAY_HELP;
    displayHelpReq.serial = 0;
    displayHelpReq.version = 1;
    displayHelpReq.client = win;
    displayHelpReq.nodename = NULL;

    if (help)
    {
	displayHelpReq.source_type =
	    help->section ? DT_SECTION_HELP : DT_TOC_HELP;
	displayHelpReq.app_name = AppName;
	displayHelpReq.app_title = AppTitle;
	displayHelpReq.title = help->title;
	displayHelpReq.help_dir = NULL;
	displayHelpReq.file_name = help->file;
	displayHelpReq.sect_tag = help->section;
    }
    else
	displayHelpReq.source_type = DT_OPEN_HELPDESK;

    (void)DtEnqueueRequest(XtScreen (widget), _HELP_QUEUE (display),
			   _HELP_QUEUE (display), win, req);
}	/* End of DisplayHelp () */

/* ButtonSelectCB
 *
 * Update the currently select button within an exclusives.  client_data is
 * structure for individual button.
 */
static void
ButtonSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    BtnChoice		*button = (BtnChoice *) client_data;
    OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;

    button->setIndx = pFlatData->item_index;
}	/* End of ButtonSelectCB () */

/* MakeButtons
 *
 * Make a button control with a caption.
 */
void
MakeButtons (Widget parent, char *lbl, ButtonItem *items, Cardinal numItems,
	    BtnChoice *data)
{
    Widget	caption;

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);

    data->btn = XtVaCreateManagedWidget ("button",
		flatButtonsWidgetClass, caption,
		XtNbuttonType,		(XtArgVal) OL_RECT_BTN,
		XtNselectProc,		(XtArgVal) ButtonSelectCB,
		XtNclientData,		(XtArgVal) data,
		XtNexclusives,		(XtArgVal) True,
		XtNitemFields,		(XtArgVal) ButtonFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ButtonFields),
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) numItems,
		0);
}	/* End of MakeButtons () */

/* CheckSelectCB
 *
 * Update the checkbox that the user clicked on. client_data is
 * structure for individual button.
 */
static void
CheckSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    BtnChoice		*button = (BtnChoice *) client_data;

    button->setIndx = !button->setIndx;
}	/* End of CheckSelectCB () */

/* MakeCheck
 *
 * Make a checkbox control with a caption.
 */
void
MakeCheck (Widget parent, char *lbl, BtnChoice *data)
{
    Widget	caption;
    ButtonItem	*item;

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);

    item = (ButtonItem *) XtCalloc (1, sizeof (ButtonItem));
    data->btn = XtVaCreateManagedWidget ("button",
		flatButtonsWidgetClass, caption,
		XtNbuttonType,		(XtArgVal) OL_CHECKBOX,
		XtNselectProc,		(XtArgVal) CheckSelectCB,
		XtNunselectProc,	(XtArgVal) CheckSelectCB,
		XtNclientData,		(XtArgVal) data,
		XtNitemFields,		(XtArgVal) ButtonFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ButtonFields),
		XtNitems,		(XtArgVal) item,
		XtNnumItems,		(XtArgVal) 1,
		0);

    XtAddCallback (data->btn, XtNdestroyCallback, DestroyCheckCB,
		   (XtPointer) item);
}	/* End of MakeCheck () */

/* ResetCheck
 *
 * Reset the current value in the menu and preview area.
 */
void
ResetCheck (BtnChoice *chk, Boolean set)
{
    chk->setIndx = (Cardinal) set;
    OlVaFlatSetValues (chk->btn, 0,
		XtNset,			(XtArgVal) set,
		0);
}	/* End if ResetCheck () */

/* ApplyCheck
 *
 * Save the current value set in the menu and update the preview area.
 */
void
ApplyCheck (BtnChoice *chk, Boolean *pBool)
{
    *pBool = chk->setIndx;
}	/* End if ApplyCheck () */

/* DestroyCheckCB
 *
 * Destroy dynamically allocated data associated with a check control.
 * client_data is a pointer to the check item list.
 */
static void
DestroyCheckCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtFree (client_data);
}	/* End of DestroyCheckCB () */

/* MakeText
 *
 * Make a text field with a caption.
 */
void
MakeText (Widget parent, char *lbl, TxtChoice *data, int length)
{
    Widget	caption;

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);

    data->txt = XtVaCreateManagedWidget ("text",
		textFieldWidgetClass, caption,
		XtNcharsVisible,		(XtArgVal) length,
		0);
    data->setText = (char *) 0;

    XtAddCallback (data->txt, XtNdestroyCallback, DestroyText, data);
}	/* End of MakeText () */

/* MakeStaticText
 *
 * Make a static text field with a caption.
 */
void
MakeStaticText (Widget parent, char *lbl, StaticTxt *data, int length)
{
    Widget	caption;

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);

    *data = XtVaCreateManagedWidget ("staticText",
		staticTextWidgetClass, caption,
		XtNcharsVisible,		(XtArgVal) length,
		0);
}	/* End of MakeStaticText () */

/* DestroyText
 *
 * Clean up after text control.
 */
static void
DestroyText (Widget widget, XtPointer client_data, XtPointer call_data)
{
    TxtChoice	*data = (TxtChoice *) client_data;

    if (data->setText)
	XtFree (data->setText);
}	/* End of DestroyText () */

/* ApplyText
 *
 * Apply changes to a text control.
 */
void
ApplyText (TxtChoice *control, char **pValue)
{
    if (*pValue)
	XtFree (*pValue);

    *pValue = control->setText;
    control->setText = (char *) 0;
}	/* End of ApplyText () */

/* GetText
 *
 * Get the value of a text control.  The returned string should NOT be
 * freed by the caller, and should be regarded as temporary.
 */
char *
GetText (TxtChoice *txtCtrl)
{
    if (txtCtrl->setText)
	XtFree (txtCtrl->setText);

    XtVaGetValues (txtCtrl->txt,
		XtNstring,	(XtArgVal) &txtCtrl->setText,
		0);

    return (txtCtrl->setText);
}	/* End of GetText () */

Widget
MakeList(Widget parent, char *lbl, PropPg *page,
                ButtonItem *items, int numItems, int length)
{
    Widget      caption;
    Widget      control;
    Widget      scroll;
    Widget      preview;
    ListChoice *list = &page->localeCtrl;

    

    list->buttons.setIndx = 0;
    list->items = items;

    list->caption = XtVaCreateManagedWidget ("caption",
                captionWidgetClass, parent,
                XtNlabel,       (XtArgVal) lbl,
                0);
    if(items){
    control = XtVaCreateWidget ("ctrlArea",
                controlAreaWidgetClass, list->caption,
                XtNshadowThickness,     (XtArgVal) 0,
		XtNlayoutType,          (XtArgVal) OL_FIXEDCOLS,
                0);

    scroll =  XtVaCreateManagedWidget("scrolledWindow",
                scrolledWindowWidgetClass, control,
                0);

    list->buttons.btn = XtVaCreateManagedWidget ("localeList", flatListWidgetClass,
                scroll,
                XtNviewHeight,          (XtArgVal) 2,
                XtNexclusives,          (XtArgVal) True,
                XtNnoneSet,             (XtArgVal) False,
		XtNitemFields,		(XtArgVal) ButtonFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ButtonFields),
		XtNitems,		(XtArgVal) list->items,
		XtNnumItems,		(XtArgVal) numItems,
                XtNselectProc,          (XtArgVal) ListSelectCB,
                XtNclientData,          (XtArgVal) page,
                XtNformat,              (XtArgVal) "%14s",
                0);

         preview   = XtVaCreateManagedWidget ("caption",
                captionWidgetClass, control,
                XtNlabel,       (XtArgVal) GetStr (TXT_selection),
                0);
         list->preview = XtVaCreateManagedWidget ("preview",
                staticTextWidgetClass, preview,
		XtNstring,              (XtArgVal) items [0].lbl,
                XtNwidth,               (XtArgVal) OlMMToPixel (OL_HORIZONTAL,
                                                                 length),
                0);
         XtManageChild(control);
    }
    else
        XtSetSensitive (list->caption, False);
    return(scroll);
}

static void
ListSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{

    PropPg 	    *page = (PropPg *)client_data;
    ListChoice	    *list = &page->localeCtrl;
    OlFlatCallData      *pFlatData = (OlFlatCallData *) call_data;

    list->buttons.setIndx = pFlatData->item_index;

    XtVaSetValues (list->preview,
        XtNstring,      (XtArgVal) list->items [pFlatData->item_index].lbl,
        0);

    if(!pFlatData->item_index)
        XtSetSensitive (page->charSetCtrl.caption, True);
    else
        XtSetSensitive (page->charSetCtrl.caption, False);

 }

/* ApplyList
 *
 * Save the current value if the selected item in the list.
 */
void
ApplyList(ListChoice *list, Cardinal *pIndx)
{
    *pIndx = list->buttons.setIndx;
}	/* End of ApplyList () */

/* ResetList
 *
 * Reset the item in the list
 */
void
ResetList(ListChoice *list, Cardinal indx)
{
    if (list->items)
    {
        list->buttons.setIndx = indx;
        OlVaFlatSetValues (list->buttons.btn, indx,
                XtNset,                 (XtArgVal) True,
                0);
	XtVaSetValues (list->buttons.btn,
                XtNviewItemIndex,       (XtArgVal) indx,
                0);

        XtVaSetValues (list->preview,
                XtNstring,              (XtArgVal) list->items [indx].lbl,
                0);
    }
}       /* End if ResetList() */


/* MakeAbbrevMenu
 *
 * An abbreviated menu control consists of an abbreviated menubutton and a
 * static text preview area.  The two are in a control area with a caption.
 * If the menu has no items, it is insensitive.
 */
void
MakeAbbrevMenu (Widget parent, char *lbl, AbbrevChoice *abbrev,
		ButtonItem *items, int numItems, int length)
{
    Widget	caption;
    Widget	control;
    Widget	popup;

    abbrev->buttons.setIndx = 0;
    abbrev->items = items;

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);
    abbrev->caption = caption;

    control = XtVaCreateManagedWidget ("ctrlArea",
		controlAreaWidgetClass, caption,
		XtNshadowThickness,	(XtArgVal) 0,
		0);

    popup = XtVaCreatePopupShell ("ucaMenuShell",
		popupMenuShellWidgetClass, parent,
		0);

    abbrev->menuBtn = XtVaCreateManagedWidget ("abbrevMenu",
		abbreviatedButtonWidgetClass, control,
		XtNpopupWidget,		(XtArgVal) popup,
		XtNsensitive,		(XtArgVal) numItems > 0 ? True : False,
		0);

    abbrev->buttons.btn = XtVaCreateManagedWidget ("menuButtons",
		flatButtonsWidgetClass, popup,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNitemFields,		(XtArgVal) ButtonFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ButtonFields),
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) numItems,
		XtNbuttonType,		(XtArgVal) OL_RECT_BTN,
		XtNselectProc,		(XtArgVal) AbbrevSelectCB,
		XtNclientData,		(XtArgVal) abbrev,
		XtNexclusives,		(XtArgVal) True,
		0);
    

    if (items)
    {
	abbrev->preview = XtVaCreateManagedWidget ("preview",
		staticTextWidgetClass, control,
		XtNwidth,		(XtArgVal) OlMMToPixel (OL_HORIZONTAL,
								 length),
		0);

	XtVaSetValues (abbrev->menuBtn,
		XtNpreviewWidget,	(XtArgVal) abbrev->preview,
		0);
    }
    else
	XtSetSensitive (caption, False);
}	/* End of MakeAbbrevMenu () */

/* ApplyAbbrevMenu
 *
 * Save the current value set in the menu and update the preview area.
 */
void
ApplyAbbrevMenu (AbbrevChoice *abbrev, Cardinal *pIndx)
{
    *pIndx = abbrev->buttons.setIndx;
}	/* End if ApplyAbbrevMenu () */

/* ResetAbbrevMenu
 *
 * Reset the current value in the menu and preview area.
 */
void
ResetAbbrevMenu (AbbrevChoice *abbrev, Cardinal indx)
{
    if (abbrev->items)
    {
	abbrev->buttons.setIndx = indx;
	OlVaFlatSetValues (abbrev->buttons.btn, indx,
		XtNset,			(XtArgVal) True,
		0);

	XtVaSetValues (abbrev->preview,
		XtNstring,		(XtArgVal) abbrev->items [indx].lbl,
		0);
    }
}	/* End if ResetAbbrevMenu () */

/* AbbrevSelectCB
 *
 * Update the currently select button within a menu.  Update the preview
 * area as well.  client_data is a pointer to the abbrev choice structure.
 */
extern void
AbbrevSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    AbbrevChoice	*abbrev = (AbbrevChoice *) client_data;
    OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;

    ButtonSelectCB (abbrev->buttons.btn, (XtPointer) &abbrev->buttons,
		    call_data);
    XtVaSetValues (abbrev->preview,
	XtNstring,	(XtArgVal) abbrev->items [pFlatData->item_index].lbl,
	0);
}	/* End of AbbrevSelectCB () */
extern void
LocaleSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg *page = (PropPg *)client_data;
    AbbrevChoice        *abbrev = &page->localeCtrl;
    OlFlatCallData      *pFlatData = (OlFlatCallData *) call_data;

    ButtonSelectCB (abbrev->buttons.btn, (XtPointer) &abbrev->buttons,
                    call_data);
    
    XtVaSetValues (abbrev->preview,
        XtNstring,      (XtArgVal) abbrev->items [pFlatData->item_index].lbl,
        0);

    if(!pFlatData->item_index)
	XtSetSensitive (page->charSetCtrl.caption, True);
    else
	XtSetSensitive (page->charSetCtrl.caption, False);
}       /* End of AbbrevSelectCB () */


/* MakeScaled
 *
 * A scaled number control is a text field that contains a positive real
 * number and an exclusives that scales the number in inches, centimeters,
 * and characters.
 */
void
MakeScaled (Widget parent, char *lbl, ScaledChoice *scaled)
{
    Widget		caption;
    Widget		control;
    static Boolean	first = True;

    if (first)
    {
	first = False;

	SetButtonLbls (UnitItems, XtNumber (UnitItems));
    }

    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, parent,
		XtNlabel,	(XtArgVal) lbl,
		0);

    control = XtVaCreateManagedWidget ("ctrlArea",
		controlAreaWidgetClass, caption,
		XtNshadowThickness,	(XtArgVal) 0,
		0);

    scaled->value.txt = XtVaCreateManagedWidget ("text",
		textFieldWidgetClass, control,
		XtNwidth,	(XtArgVal) OlMMToPixel (OL_HORIZONTAL, 20),
		0);
    scaled->value.setText = (char *) 0;

    XtAddCallback (scaled->value.txt, XtNdestroyCallback, DestroyText,
		   &scaled->value);

    scaled->units.btn = XtVaCreateManagedWidget ("button",
		flatButtonsWidgetClass, control,
		XtNbuttonType,		(XtArgVal) OL_RECT_BTN,
		XtNselectProc,		(XtArgVal) ButtonSelectCB,
		XtNclientData,		(XtArgVal) &scaled->units,
		XtNexclusives,		(XtArgVal) True,
		XtNitemFields,		(XtArgVal) ButtonFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (ButtonFields),
		XtNitems,		(XtArgVal) UnitItems,
		XtNnumItems,		(XtArgVal) XtNumber (UnitItems),
		0);
}	/* End of MakeScaled () */

/* ApplyScaled
 *
 * Save the current values set in the scaled number.
 */
void
ApplyScaled (ScaledChoice *scaled, float *pValue, char *pUnits)
{
    if (scaled->value.setText == NULL || strlen (scaled->value.setText) == 0)
    {
	*pValue = 0.0;
	*pUnits = 0;
    }
    else
    {
	sscanf (scaled->value.setText, "%f", pValue);
	*pUnits = (char) UnitItems [scaled->units.setIndx].userData;
    }
}	/* End if ApplyScaled () */

/* ResetScaled
 *
 * Reset the current value.
 */
void
ResetScaled (ScaledChoice *scaled, float value, char units)
{
    Cardinal	indx;
    char	buf [16];

    if (units)
    {
	sprintf (buf, "%.2f", value);
	XtVaSetValues (scaled->value.txt,
		XtNstring,		(XtArgVal) buf,
		0);
    }
    else
	XtVaSetValues (scaled->value.txt,
		XtNstring,		(XtArgVal) 0,
		0);

    switch (units) {
    case 'i':
	indx = Inches_Button;
	break;
    case 'c':
	indx = Centimeters_Button;
	break;
    case ' ':
    default:
	indx = Units_Button;
	break;
    }
    scaled->units.setIndx = indx;
    OlVaFlatSetValues (scaled->units.btn, indx,
		XtNset,			(XtArgVal) True,
		0);
}	/* End of ResetScaled () */

/* CheckScaled
 *
 * Check that the scaled number is a positive real number.
 */
Boolean
CheckScaled (ScaledChoice *scaled)
{
    if (scaled->value.setText)
	XtFree (scaled->value.setText);

    XtVaGetValues (scaled->value.txt,
		XtNstring,		(XtArgVal) &scaled->value.setText,
		0);

    if (strlen (scaled->value.setText) > (unsigned) 0)
    {
	char	*ptr;
	float	val;
	char	buf [32];

	/* it seems kind of silly to convert the string to a double and then
	 * back into a string, but it is a convenient way to remove any white
	 * space from the original string.
	 */
	val = (float) strtod (scaled->value.setText, &ptr);
	if (val <= 0.0 || *ptr)
	    return (False);
	XtFree (scaled->value.setText);
	sprintf (buf, "%.2f", val);
	scaled->value.setText = strdup (buf);
    }
    else
    {
	XtFree (scaled->value.setText);
	scaled->value.setText = (char *) 0;
    }

    return (True);
}	/* End of CheckScaled () */

/* GetScaled
 *
 * Convert a string into a scaled value.
 */
void
GetScaled (char *str, SCALED *scaledNum)
{
    char	*ptr;

    scaledNum->val = (float) strtod (str, &ptr);
    switch (*ptr) {
    case 'i':
    case 'c':
    case ' ':
	scaledNum->sc = *ptr;
	break;
    case 0:
	scaledNum->sc = ' ';
	break;
    default:
	scaledNum->sc = 0;
	break;
    }
}	/* End of GetScaled () */

/* SetLabels
 *
 * Set menu item labels and mnemonics.
 */
void
SetLabels (MenuItem *items, int cnt)
{
    char	*mnem;

    for ( ; --cnt>=0; items++)
    {
	items->lbl = (XtArgVal) GetStr ((char *) items->lbl);
	mnem = GetStr ((char *) items->mnem);
	items->mnem = (XtArgVal) mnem [0];
    }
}	/* End of SetLabels */

/* SetButtonLbls
 *
 * Set button item labels.
 */
void
SetButtonLbls (ButtonItem *items, int cnt)
{
    for ( ; --cnt>=0; items++)
	items->lbl = (XtArgVal) GetStr ((char *) items->lbl);
}	/* End of SetButtonLbls */

/* SetHelpLabels
 *
 * Set strings for help text.
 */
void
SetHelpLabels (HelpText *help)
{
    help->title = GetStr (help->title);
    if (help->section)
	help->section = GetStr (help->section);
}	/* End of SetHelpLabels */
