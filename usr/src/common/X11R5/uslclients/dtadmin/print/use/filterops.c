#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/filterops.c	1.22"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <locale.h>
#include <memory.h>
#include <sys/types.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <X11/Shell.h>

#include <Xol/ChangeBar.h>
#include <Xol/Caption.h>
#include <Xol/MenuShell.h>
#include <Xol/PopupWindo.h>
#include <Xol/ControlAre.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/ScrolledWi.h>

#include <lp.h>
#include <msgs.h>

#include "properties.h"
#include "error.h"

static void	ApplyFilterCB (Widget, XtPointer, XtPointer);
static void	SetFilterDfltsCB (Widget, XtPointer, XtPointer);
static void	ResetFilterCB (Widget, XtPointer, XtPointer);
static void	DestroyFilterPopup (Widget, XtPointer, XtPointer);
static void	DestroyFilterCtrls (Widget, XtPointer, XtPointer);
static void	DestroyCtrlArea (Widget, XtPointer, XtPointer);
extern void	CancelCB (Widget, XtPointer, XtPointer);
extern void	VerifyCB (Widget, XtPointer, XtPointer);
extern void	HelpCB (Widget, XtPointer, XtPointer);
extern void	LocaleSelectCB(Widget, XtPointer, XtPointer);

extern Filter	*FindFilter (char *, Printer *);
extern int	FindCharSet (Printer *, char *);
static Cardinal	FindLocale (Properties *properties, char *name);
extern void	InitFilterOpts (Properties *, char *, Boolean, FilterOpts *);

static void	CreateFilterProperties (Widget parent, Properties *properties);
static Widget 	CreateFilterOpts (Widget, Properties *);
static char	*CheckFilterOpts (Widget, Properties *);
static void	ApplyFilterOpts (Widget, Properties *);
static void	ResetFilterOpts (Widget, Properties *);
static char	**ParseFilterOpts (Properties *properties,
				   FilterOpts *filterOpts, int numOpts);
extern void	CopyFilterOpts (FilterOpts *, FilterOpts *);
extern void	FreeFilterOpts (FilterOpts *);

extern Boolean	SetDefaults (FilterOpts *, Printer *);

extern ButtonItem	*InputTypes;
extern unsigned		NumInputTypes;

static Widget	FilterFooter;

static HelpText FilterHelp = {
    TXT_filterHelp, HELP_FILE, TXT_nfilterHelpSect,
};

/* Lower Control Area buttons */
static MenuItem FilterItems [] = {
    { (XtArgVal) TXT_ok, (XtArgVal) MNEM_ok, (XtArgVal) True,
	  (XtArgVal) ApplyFilterCB, (XtArgVal) True, },	/* Apply */
    { (XtArgVal) TXT_setDflts, (XtArgVal) MNEM_setDflts, (XtArgVal) True,
	  (XtArgVal) SetFilterDfltsCB, },		/* Set Defaults */
    { (XtArgVal) TXT_reset, (XtArgVal) MNEM_reset, (XtArgVal) True,
	  (XtArgVal) ResetFilterCB, },			/* Reset */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &FilterHelp, },			/* Help */
};

/* FilterOptsCB
 *
 * Create and post a property sheet for setting the filter options.
 * client_data is a pointer to the PropPg of the owning job.
 */
void
FilterOptsCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg	*page = (PropPg *) client_data;
    Properties	*properties = page->posted;
    Widget	swin;
    Filter	*filter;
    char	*inputType;

    /* If the window is already posted, check if it's for the same request.
     * If so, bring the window to the top.  Otherwise, destroy the old
     * controls and bring up the new ones.
     */
    if (page->optionsPopup)
    {
	if (page->optionsOwner == properties)
	{
	    XRaiseWindow (XtDisplay (page->optionsPopup),
			  XtWindow (page->optionsPopup));
	    return;
	}
	else
	{
	    swin = XtParent (page->optionsCtrlArea);
	    XtDestroyWidget (page->optionsCtrlArea);
	    DestroyFilterCtrls (widget, client_data, (XtPointer) 0);
	}
    }

    /* The input type might have changed from what is in the request, so
     * get the current value.
     */
    if (page->inTypeCtrl.buttons.setIndx == NumInputTypes - 1)
	inputType = GetText (&page->otherTypeCtrl);
    else
	inputType =
	    (char *) InputTypes [page->inTypeCtrl.buttons.setIndx].userData;

    if (strcmp (inputType, properties->appliedFilter.inputType) != 0)
	InitFilterOpts (properties, inputType, True, &page->workFilter);
    else
    {
	if (!properties->appliedFilter.filter)
	    InitFilterOpts (properties, inputType, False,
			    &properties->appliedFilter);
	CopyFilterOpts (&properties->appliedFilter, &page->workFilter);
    }

    /* Read the options files for the filters, if not already done. */
    page->numOptions = 0;
    for (filter=page->workFilter.filter; filter; filter=filter->next)
    {
	if (!filter->data)
	    ReadFilterOpts (filter);
	page->numOptions += filter->cnt;
    }

    /* Parse the modes to extract options settings */
    page->filterOptions = ParseFilterOpts (properties, &page->workFilter,
					   page->numOptions);

    if (page->optionsPopup)
	page->optionsCtrlArea = CreateFilterOpts (swin, properties);
    else
	CreateFilterProperties (widget, properties);

    /* Make sure all widgets visually reflect the correct values. */
    ResetFilterOpts (page->optionsPopup, properties);

    page->optionsOwner = properties;
}	/* End of FilterOptsCB () */

/* CreateFilterProperties
 *
 * Create widgets for the filter options property sheet.
 */
static void
CreateFilterProperties (Widget parent, Properties *properties)
{
    Widget		lca;
    Widget		uca;
    Widget		footer;
    Widget		ucaMenuShell;
    Widget		ucaMenu;
    Widget		lcaMenu;
    Widget		swin;
    Widget		caption;
    Widget		locList;
    PropPg		*page;
    static Boolean	first = True;
    static char		*titleStr;
    static char		*charSetLbl;
    static char		*localeLbl;
    static char		*pgLenLbl;
    static char		*pgWidLbl;
    static char		*cpiLbl;
    static char		*lpiLbl;
    static char		*optionsLbl;
    int 		widthM;
    int 		widthC;

    /* Initialize all labels first time only */
    if (first)
    {
	first = False;

	titleStr = GetStr (TXT_filterTitle);
	localeLbl = GetStr (TXT_newlocale);
	charSetLbl = GetStr (TXT_charSet);
	pgLenLbl = GetStr (TXT_pgLen);
	pgWidLbl = GetStr (TXT_pgWid);
	cpiLbl = GetStr (TXT_cpi);
	lpiLbl = GetStr (TXT_lpi);
	optionsLbl = GetStr (TXT_options);
	SetLabels (FilterItems, XtNumber (FilterItems));
	SetHelpLabels (&FilterHelp);
    }

    page = properties->page;

    /* Create the popup.  Because the property sheet controls vary based on
     * the filter type, it is somewhat more convenient the destroy the sheet
     * when it is brought down and recreate it each time.
     */
    page->optionsPopup = XtVaCreatePopupShell ("properties",
		popupWindowShellWidgetClass,
		parent,
		XtNtitle,		(XtArgVal) titleStr,
		0);

    XtAddCallback (page->optionsPopup, XtNverify, VerifyCB, (XtPointer) 0);
    XtAddCallback (page->optionsPopup, XtNpopdownCallback, DestroyFilterPopup,
		   (XtPointer) page);
    XtAddCallback (page->optionsPopup, XtNdestroyCallback, DestroyFilterCtrls,
		   (XtPointer) page);

    XtVaGetValues (page->optionsPopup,
		XtNlowerControlArea,	(XtArgVal) &lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		XtNfooterPanel,		(XtArgVal) &footer,
		0);

    /* We want an "apply" and "reset" buttons in both the lower control
     * area and in a popup menu on the upper control area.
     */
    lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, lca,
		XtNclientData,		(XtArgVal) page,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) FilterItems,
		XtNnumItems,		(XtArgVal) XtNumber (FilterItems),
		0);
    XtVaGetValues (lcaMenu,
		XtNwidth,		(XtArgVal) &widthM,
		0);

    ucaMenuShell = XtVaCreatePopupShell ("ucaMenuShell",
		popupMenuShellWidgetClass, uca,
		0);

    ucaMenu = XtVaCreateManagedWidget ("ucaMenu",
		flatButtonsWidgetClass, ucaMenuShell,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNclientData,		(XtArgVal) page,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) FilterItems,
		XtNnumItems,		(XtArgVal) XtNumber (FilterItems),
		0);

    OlAddDefaultPopupMenuEH (uca, ucaMenuShell);

    /* Make a text area in the footer for error messages. */
    FilterFooter = XtVaCreateManagedWidget ("footer",
		staticTextWidgetClass, footer,
		0);

    /* Create the controls for adding/changing print jobs */
    MakeScaled (uca, pgLenLbl, &page->pgLenCtrl);
    MakeScaled (uca, pgWidLbl, &page->pgWidCtrl);
    MakeScaled (uca, cpiLbl, &page->cpiCtrl);
    MakeScaled (uca, lpiLbl, &page->lpiCtrl);

    locList = MakeList(uca, localeLbl, page,
                    properties->printer->localeItems,
                    properties->printer->numLocales,
                    35);

    MakeAbbrevMenu (uca, charSetLbl, &page->charSetCtrl,
		    properties->printer->charSetItems,
		    properties->printer->numCharSets,
		    35);

    if(!page->workFilter.locale)
         XtSetSensitive (page->charSetCtrl.caption, True);
    else
         XtSetSensitive (page->charSetCtrl.caption, False);

    /* Filter specific options appear in a scrolled window */
    caption = XtVaCreateManagedWidget ("caption",
		captionWidgetClass, uca,
		XtNlabel,	(XtArgVal) optionsLbl,
		0);

    XtVaGetValues (caption,
		XtNwidth,		(XtArgVal) &widthC,
		0);
    swin = XtVaCreateManagedWidget ("scrolledWin", scrolledWindowWidgetClass,
		caption,
		XtNwidth,		(XtArgVal) (widthM - widthC -60),
                XtNviewHeight,          (XtArgVal) 54,
		0);

    page->optionsCtrlArea = CreateFilterOpts (swin, properties);

    XtPopup (page->optionsPopup, XtGrabNone);
}	/* End of CreateFilterProperties () */

/* DestroyFilterPopup
 *
 * Destroy the popup window and all its widgets and associated data.
 * client_data is a pointer to the PropPg of the owning property sheet.
 */
static void
DestroyFilterPopup (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg	*page = (PropPg *) client_data;

    page->optionsPopup = 0;
    XtDestroyWidget (widget);
}	/* End of DestroyFilterPopup () */

/* DestroyFilterCtrls
 *
 * Destroy dynamically allocated data associated with the filter controls.
 * client_data is a pointer to the PropPg.
 */
static void
DestroyFilterCtrls (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg	*page = (PropPg *) client_data;
    char	**opt;
    Filter	*filter;
    register	i;

    /* Destroy option strings */
    opt = page->filterOptions;
    for (filter=page->workFilter.filter; filter; filter=filter->next)
    {
	for (i=0; i<filter->cnt; i++, opt++)
	{
	    switch ((*filter->data) [i].type) {
	    case 'b':
		break;
	    case 'c':
	    case 'd':
	    case 'u':
	    case 'f':
	    case 's':
		XtFree (*opt);
		break;
	    }
	}
    }
    XtFree ((char *) page->filterOptions);
    page->filterOptions = (char **) 0;

    FreeFilterOpts (&page->workFilter);
}	/* End of DestroyFilterCtrls () */


/* DestroyCtrlArea
 *
 * Destroy dynamically allocated data associated with the control area
 * that contains the filter controls. client_data is a pointer to the
 * controls to free.
 */
static void
DestroyCtrlArea (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtFree ((char *) client_data);
}	/* End of DestroyCtrlArea () */

/* CreateFilterOpts
 *
 * Create the filter specific controls for each filter in the pipeline.
 */
static Widget
CreateFilterOpts (Widget parent, Properties *properties)
{
    Widget	control;
    Filter	*filter;
    FilterCtrl	*ctrls;
    PropPg	*page;
    register	i;
    int 	width;

    control = XtVaCreateWidget ("ctrlArea",
		controlAreaWidgetClass, parent,
		XtNshadowThickness,	(XtArgVal) 0,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		0);

    page = properties->page;
    page->filterCtrls = ctrls = (FilterCtrl *)
	XtMalloc (page->numOptions * sizeof (FilterCtrl));

    XtAddCallback (control, XtNdestroyCallback, DestroyCtrlArea,
		   (XtPointer) ctrls);

    /* Make controls */
    for (filter=page->workFilter.filter; filter; filter=filter->next)
    {
	for (i=0; i<filter->cnt; i++, ctrls++)
	{
	    switch ((*filter->data) [i].type) {
	    case 'b':
		MakeCheck (control, (*filter->data)[i].lbl, &ctrls->chkCtrl);
		break;
	    case 'c':
		MakeText (control, (*filter->data)[i].lbl, &ctrls->txtCtrl, 2);
		break;
	    case 'd':
	    case 'u':
		MakeText (control, (*filter->data)[i].lbl, &ctrls->txtCtrl, 5);
		break;
	    case 'f':
	    case 's':
		MakeText (control, (*filter->data)[i].lbl, &ctrls->txtCtrl,
			  10);
		break;
	    }
	}
    }

    XtManageChild (control);
    return (control);
}	/* End of CreateFilterOpts () */

/* InitFilterOpts
 *
 * Initialize the filter options that relate to the request.  If dfltModes
 * is True, initialize with default values.  Otherwise, use the values
 * from the request in the properties structure.  In this case, the
 * interface options (pg length, etc) have already been set.
 */
void
InitFilterOpts (Properties *properties, char *inType, Boolean dfltModes,
		FilterOpts *filterOpts)
{
    Defaults	*dflts;

    filterOpts->filter = FindFilter (inType, properties->printer);
    if (!filterOpts->inputType)
	filterOpts->inputType = strdup (inType);

    if (dfltModes)
    {
	/* Get the defaults */
	for (dflts=properties->printer->dflts; dflts; dflts=dflts->next)
	{
	    if (strcmp (inType, dflts->inputType) == 0)
		break;
	}

	if (dflts)
	{
	    filterOpts->modes = strdup (dflts->modes);
	    filterOpts->opts = strdup (dflts->opts);
	    filterOpts->charSet = FindCharSet (properties->printer,
					       dflts->charSet);
	}
	else
	{
	    filterOpts->modes = strdup ("");
	    filterOpts->opts = strdup ("");
	    filterOpts->charSet = 0;
	}
    }
    else
    {
	/* Until we actually have to display the filter options controls, we
	 * don't need the individual values.  So all we have to do is save
	 * the modes string from the request.  The option string is handled
	 * in properties.c.
	 */
	filterOpts->modes = strdup (properties->request->modes);

	filterOpts->charSet =
	    FindCharSet (properties->printer, properties->request->charset);
    }
}	/* End if InitFilterOpts () */

/* CheckFilterOpts
 *
 * Check validity of new filter option values.  Return pointer to error
 * text if there is a problem, else NULL.
 */
static char *
CheckFilterOpts (Widget widget, Properties *properties)
{
    Filter		*filter;
    FilterCtrl		*ctrls;
    PropPg		*page;
    char		*text;
    char		*ptr;
    int			val;
    float		fval;
    register		i;
    static char		errorMsg [256];
    static char		*badPgLenMsg;
    static char		*badPgWidMsg;
    static char		*badCpiMsg;
    static char		*badLpiMsg;
    static char		*badCharMsg;
    static char		*badIntMsg;
    static char		*badUnsignedMsg;
    static char		*badFloatMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	badPgLenMsg = GetStr (TXT_badPgLen);
	badPgWidMsg = GetStr (TXT_badPgWid);
	badCpiMsg = GetStr (TXT_badCpi);
	badLpiMsg = GetStr (TXT_badLpi);
	badCharMsg = GetStr (TXT_badCharOpt);
	badIntMsg = GetStr (TXT_badIntOpt);
	badUnsignedMsg = GetStr (TXT_badUnsignedOpt);
	badFloatMsg = GetStr (TXT_badFloatOpt);
    }

    page = properties->page;

    if (!CheckScaled (&page->pgLenCtrl))
	return (badPgLenMsg);
    if (!CheckScaled (&page->pgWidCtrl))
	return (badPgWidMsg);
    if (!CheckScaled (&page->cpiCtrl))
	return (badCpiMsg);
    if (!CheckScaled (&page->lpiCtrl))
	return (badLpiMsg);

    ctrls = page->filterCtrls;
    for (filter=page->workFilter.filter; filter; filter=filter->next)
    {
	for (i=0; i<filter->cnt; i++, ctrls++)
	{
	    char	type;

	    type = (*filter->data) [i].type;
	    if (type != 'b')
	    {
		text = GetText (&ctrls->txtCtrl);
		switch (type) {
		case 'c':
		    if (strlen (text) > (unsigned) 1)
		    {
			sprintf (errorMsg, badCharMsg, (*filter->data)[i].lbl);
			return (errorMsg);
		    }
		    break;

		case 's':
		    break;

		case 'd':
		    val = (int) strtol (text, &ptr, 10);
		    if (*ptr)
		    {
			sprintf (errorMsg, badIntMsg, (*filter->data) [i].lbl);
			return (errorMsg);
		    }
		    break;

		case 'u':
		    val = (int) strtol (text, &ptr, 10);
		    if (val < 0 || *ptr)
		    {
			sprintf (errorMsg, badUnsignedMsg,
				 (*filter->data) [i].lbl);
			return (errorMsg);
		    }
		    break;

		case 'f':
		    fval = (float) strtod (text, &ptr);
		    if (*ptr)
		    {
			sprintf (errorMsg, badFloatMsg,(*filter->data)[i].lbl);
			return (errorMsg);
		    }
		    break;
		}
	    }
	}
    }

    return (NULL);
}	/* End of CheckFilterOpts () */

/* ApplyFilterOpts
 *
 * Update property sheet to reflect the changed values.
 */
static void
ApplyFilterOpts (Widget sheet, Properties *properties)
{
    char		**opts;
    FilterCtrl		*ctrls;
    Filter		*filter;
    PropPg		*page;
    register		i;
    char		options [1024];
    char		*endModes;
    extern ButtonItem	UnitItems [];

    page = properties->page;

    ApplyList(&page->localeCtrl, &page->workFilter.locale);
    ApplyAbbrevMenu (&page->charSetCtrl, &page->workFilter.charSet);
    if(!page->workFilter.locale)
         XtSetSensitive (page->charSetCtrl.caption, True);
    else
         XtSetSensitive (page->charSetCtrl.caption, False);
    ApplyScaled (&page->pgLenCtrl, &page->workFilter.pgLen.val,
		 &page->workFilter.pgLen.sc);
    ApplyScaled (&page->pgWidCtrl, &page->workFilter.pgWid.val,
		 &page->workFilter.pgWid.sc);
    ApplyScaled (&page->cpiCtrl, &page->workFilter.cpi.val,
		 &page->workFilter.cpi.sc);
    ApplyScaled (&page->lpiCtrl, &page->workFilter.lpi.val,
		 &page->workFilter.lpi.sc);

    options [0] = 0;
    endModes = options;

    if (page->pgLenCtrl.value.setText)
    {
	sprintf (endModes, "length=%s%c ", page->pgLenCtrl.value.setText,
		 (char) UnitItems [page->pgLenCtrl.units.setIndx].userData);
	endModes += strlen (endModes);
    }

    if (page->pgWidCtrl.value.setText)
    {
	sprintf (endModes, "width=%s%c ", page->pgWidCtrl.value.setText,
		 (char) UnitItems [page->pgWidCtrl.units.setIndx].userData);
	endModes += strlen (endModes);
    }

    if (page->cpiCtrl.value.setText)
    {
	sprintf (endModes, "cpi=%s%c ", page->cpiCtrl.value.setText,
		 (char) UnitItems [page->cpiCtrl.units.setIndx].userData);
	endModes += strlen (endModes);
    }

    if (page->lpiCtrl.value.setText)
    {
	sprintf (endModes, "lpi=%s%c ", page->lpiCtrl.value.setText,
		 (char) UnitItems [page->lpiCtrl.units.setIndx].userData);
	endModes += strlen (endModes);
    }

    sprintf (endModes, "locale=%s ",
		 page->localeCtrl.items[page->workFilter.locale].userData);
    endModes += strlen (endModes);

    XtFree (page->workFilter.opts);
    page->workFilter.opts = strdup (options);

    /* Process the modes */
    options [0] = 0;
    endModes = options;
    opts = page->filterOptions;
    ctrls=page->filterCtrls;
    for (filter=page->workFilter.filter; filter; filter=filter->next)
    {
	for (i=0; i<filter->cnt; i++, ctrls++, opts++)
	{
	    if (*(*filter->data)[i].lbl)
		strcat (endModes++, " ");

	    switch ((*filter->data) [i].type) {
	    case 'b':
		ApplyCheck (&ctrls->chkCtrl, (Boolean *) opts);
		if (*opts)
		    strcpy (endModes, (*filter->data)[i].pattern);
		break;

	    case 'c':
	    case 'd':
	    case 'u':
	    case 'f':
	    case 's':
		ApplyText (&ctrls->txtCtrl, opts);
		if (*opts && **opts)
		    sprintf (endModes, (*filter->data)[i].pattern, *opts);
		break;
	    }

	    endModes += strlen (endModes);
	}
    }

    XtFree (page->workFilter.modes);
    page->workFilter.modes = strdup (options);

    FreeFilterOpts (&properties->appliedFilter);
    CopyFilterOpts (&page->workFilter, &properties->appliedFilter);
}	/* End of ApplyFilterOpts () */

/* ResetFilterOpts
 *
 * Reset property sheet to its original values
 */
static void
ResetFilterOpts (Widget sheet, Properties *properties)
{
    char	**opts;
    FilterCtrl	*ctrls;
    Filter	*filter;
    PropPg	*page;
    register	i;

    page = properties->page;

    /*ResetAbbrevMenu (&page->localeCtrl, page->workFilter.locale);*/
    ResetList(&page->localeCtrl, page->workFilter.locale);
    ResetAbbrevMenu(&page->charSetCtrl, page->workFilter.charSet);
    if(!page->workFilter.locale)
         XtSetSensitive (page->charSetCtrl.caption, True);
    else
         XtSetSensitive (page->charSetCtrl.caption, False);
    ResetScaled (&page->pgLenCtrl, page->workFilter.pgLen.val,
		 page->workFilter.pgLen.sc);
    ResetScaled (&page->pgWidCtrl, page->workFilter.pgWid.val,
		 page->workFilter.pgWid.sc);
    ResetScaled (&page->cpiCtrl, page->workFilter.cpi.val,
		 page->workFilter.cpi.sc);
    ResetScaled (&page->lpiCtrl, page->workFilter.lpi.val,
		 page->workFilter.lpi.sc);

    opts = page->filterOptions;
    ctrls = page->filterCtrls;
    for (filter=page->workFilter.filter; filter; filter=filter->next)
    {
	for (i=0; i<filter->cnt; i++, ctrls++, opts++)
	{
	    if ((*filter->data) [i].type != 'b')
		XtVaSetValues (ctrls->txtCtrl.txt,
			       XtNstring,	(XtArgVal) *opts,
			       0);
	    else
		ResetCheck (&ctrls->chkCtrl, (Boolean) *opts);
	}
    }

}	/* End of ResetFilterOpts () */

/* ApplyFilterCB
 *
 * Property sheet apply callback.  Check sheet for validity, and if valid,
 * make the changes.  client_data is a pointer to the properties page data.
 */
static void
ApplyFilterCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg	*page = (PropPg *) client_data;
    Properties	*properties = page->posted;
    char	*err;

    /* Check all property sheets for validity.  Errors here are not
     * horribly serious, so display the message in the footer.
     */
    page->popdownOK = False;
    FooterMsg (FilterFooter, NULL);
    if (err = CheckFilterOpts (widget, properties))
    {
	FooterMsg (FilterFooter, err);
	return;
    }

    ApplyFilterOpts (widget, properties);
    page->popdownOK = True;
    BringDownPopup (page->optionsPopup);
}	/* End of ApplyFilterCB () */

/* SetFilterDfltsCB
 *
 * Apply the properties and save the values for future use.  client_data
 * refers to the property page.
 */
static void
SetFilterDfltsCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg		*page = (PropPg *) client_data;
    Properties		*properties = page->posted;
    static char		*dfltErrMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	dfltErrMsg = GetStr (TXT_dfltErr);
    }

    ApplyFilterCB (widget, client_data, call_data);
    if (page->popdownOK)
    {
	if (!SetDefaults (&properties->appliedFilter, properties->printer))
	    Error (widget, dfltErrMsg);
    }
}	/* End of SetFilterDfltsCB () */

/* ResetFilterCB
 *
 * Reset the list to the original values.  client_data refers to the
 * property page.
 */
static void
ResetFilterCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg	*page = (PropPg *) client_data;
    Properties	*properties = page->posted;

    FooterMsg (FilterFooter, NULL);
    if (!properties)
	return;
    ResetFilterOpts (widget, properties);
}	/* End of ResetFilterCB () */

/* CopyFilterOpts
 *
 * Make a copy of the filter options
 */
void
CopyFilterOpts (FilterOpts *pSrc, FilterOpts *pDst)
{
  *pDst = *pSrc;
  pDst->inputType = pDst->inputType ? strdup (pDst->inputType) : strdup("");
  pDst->opts = pDst->opts ? strdup (pDst->opts) : strdup("");
  pDst->modes = pDst->modes ? strdup (pDst->modes) : strdup("");
} /* End of CopyFilterOpts () */

/* FreeFilterOpts
 *
 * Destroy the filter options.
 */
void
FreeFilterOpts (FilterOpts *filterOpts)
{
    XtFree (filterOpts->inputType);
    filterOpts->inputType = (char *) 0;
    XtFree (filterOpts->opts);
    XtFree (filterOpts->modes);
    filterOpts->filter = (Filter *) 0;
}	/* End of FreeFilterOpts () */

/* FindCharSet
 *
 * Lookup the character set name in the list supported by the printer.
 */
int
FindCharSet (Printer *prt, char *set)
{
    ButtonItem	*item;
    register	i;

    if (!set || !*set)
	return (0);

    for (item=prt->charSetItems, i=0; i<prt->numCharSets; i++, item++)
	if (!strcmp (set, (char *) item->lbl))
	    return (i);

    return (0);
}	/* End of FindCharSet () */

/* ParseFilterOpts
 *
 * Parse the modes string to extract the currently set filter options.
 * Allocate space for the options and return a pointer to this space.
 * Also parse the interface options for page length, etc., and store the
 * numeric values in the filter options structure.
 */
static char **
ParseFilterOpts (Properties *properties, FilterOpts *filterOpts, int numOpts)
{
    char	**optionList;
    char	**pOpt;
    char	**modeList;
    char	**pMode;
    char	*modes;
    char	*option;
    char	options [256];
    Filter	*filter;
    register	i;
    Boolean	have_locale = False;

    pOpt = optionList = (char **) XtCalloc (numOpts, sizeof (char *));

    modes = strdup (filterOpts->modes);
    modeList = getlist(modes, LP_WS, LP_SEP);
    if (!modeList)
    {
	modeList = (char **) XtMalloc (sizeof (char *));
	*modeList = (char *) 0;
    }
    else
    {
	/* Remove any occurrence of locale in the mode list */
	for (pMode=modeList; *pMode; pMode++)
	{
	    if (strncmp (*pMode, "locale=", 7) == 0)
	    {
		XtFree (*pMode);
		*pMode = "";
	    }
	}
    }
    modes [0] = 0;

    /* For each possible option, search the mode list for something that
     * matches the option's pattern.  Remove matches from the mode list.
     * Be careful about options that take multiple values.  The last filter
     * option is assumed to be miscellaneous options, where we collect all
     * modes that don't match any pattern.
     */
    for (filter=filterOpts->filter; filter->next; filter=filter->next)
    {
	i = 0;
	while (i < filter->cnt)
	{
	    if ((*filter->data) [i].type != 'b')
	    {
		register	j, k;

		for (j=i+1; j<filter->cnt && !*(*filter->data)[j].lbl; j++)
		    ;	/* do nothing */

		j -= i;
		for (pMode=modeList; *pMode; pMode++)
		{
		    char		buf [16];
		    union {
			char		chr;
			int		integer;
			unsigned	uint;
			float		real;
			char		str [64];
		    } vals [5];

		    if (!**pMode)
			continue;	/* option already matched */

		    /* This is a little sloppy:  the pattern can have at most
		     * five substitutions.  (This is checked when the filter
		     * options are read in.)  If it has fewer than that, the
		     * additional args in the sscanf below will simply be
		     * ignored.
		     */
		    if (sscanf (*pMode, (*filter->data)[i].origPattern,
				vals, vals+1, vals+2, vals+3, vals+4) == j)
		    {
			/* We have a match--assign the values to the option
			 * strings.
			 */
			for (k=0; k<j; k++)
			{
			    switch ((*filter->data)[i+k].type) {
			    case 'c':
				pOpt [k] = XtMalloc (2);
				pOpt [k][0] = vals [k].chr;
				pOpt [k][1] = 0;
				break;

			    case 'd':
				sprintf (buf, "%d", vals [k].integer);
				pOpt [k] = strdup (buf);
				break;

			    case 'u':
				sprintf (buf, "%u", vals [k].uint);
				pOpt [k] = strdup (buf);
				break;

			    case 'f':
				sprintf (buf, "%f", vals [k].real);
				pOpt [k] = strdup (buf);
				break;

			    case 's':
				pOpt [k] = strdup (vals [k].str);
				break;

			    }
			}
			XtFree (*pMode);
			*pMode = "";
		    }
		}
		i += j;
		pOpt += j;
	    }
	    else
	    {
		for (pMode=modeList ; *pMode; pMode++)
		{
		    if (!**pMode)
			continue;	/* option already matched */

		    if (strcmp (*pMode, (*filter->data)[i].origPattern) == 0)
		    {
			*pOpt = (char *) 1;
			XtFree (*pMode);
			*pMode = "";
		    }
		}
		i++;
		pOpt++;
	    }
	}
    }

    /* Collect all unmatched modes into the miscellaneous options. */
    for (pMode=modeList; *pMode; pMode++)
    {
	if (**pMode)
	{
	    strcat (modes, *pMode);
	    XtFree (*pMode);
	}
    }
    *pOpt = modes;

    XtFree ((char *) modeList);

    /* Parse the interface options. */
    filterOpts->pgWid.val = 0.0;
    filterOpts->pgWid.sc = 0;
    filterOpts->pgLen.val = 0.0;
    filterOpts->pgLen.sc = 0;
    filterOpts->cpi.val = 0.0;
    filterOpts->cpi.sc = 0;
    filterOpts->lpi.val = 0.0;
    filterOpts->lpi.sc = 0;
    filterOpts->locale = 0;

    strcpy (options, filterOpts->opts);
    for (option=strtok(options, " "); option; option=strtok(NULL," "))
    {
	switch (Lookup (option)) {
	case Length:
	    GetScaled (option + 7, &filterOpts->pgLen);
	    break;

	case Width:
	    GetScaled (option + 6, &filterOpts->pgWid);
	    break;

	case Cpi:
	    GetScaled (option + 4, &filterOpts->cpi);
	    break;

	case Lpi:
	    GetScaled (option + 4, &filterOpts->lpi);
	    break;

	case Locale:
	    filterOpts->locale = FindLocale (properties, option + 7);
	    have_locale = True;
	    break;
	}
    }

    if (!filterOpts->locale && have_locale == False) {
	char *locale = NULL;

	if ((locale = setlocale (LC_CTYPE,""))){ 
	    filterOpts->locale = FindLocale (properties, locale);
        }
    }
    return (optionList);
}	/* End of ParseFilterOpts () */

/* FindLocale
 *
 * Search the list of known locales and return the index of the locale in
 * the menu.
 */

static Cardinal
FindLocale (Properties *properties, char *name)
{
    ButtonItem		*items;
    int			cnt;
    int			indx;

    items = properties->printer->localeItems;
    cnt = properties->printer->numLocales;

    for (indx=0; indx<cnt; indx++)
    {
	if (strcmp (name, (char *) items[indx].userData) == 0)
	    return ((Cardinal) indx);
    }

    /* If not found return C locale, which is always item 0. */
    return 0;
}	/* End of FindLocale () */
