#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/properties.c	1.22.1.8"
#endif

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <locale.h>
#include <sys/types.h>
#include <time.h>

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

#include <lp.h>
#include <msgs.h>

#include "properties.h"
#include "error.h"

enum {
    Yes_Button, No_Button,
};

extern void	FreeProperties (Properties *properties);

static void	PopdownCB (Widget, XtPointer, XtPointer);
static void	ApplyCB (Widget, XtPointer, XtPointer);
static void	ResetCB (Widget, XtPointer, XtPointer);
extern void	CancelCB (Widget, XtPointer, XtPointer);
extern void	VerifyCB (Widget, XtPointer, XtPointer);
extern void	HelpCB (Widget, XtPointer, XtPointer);
extern void	FilterOptsCB (Widget, XtPointer, XtPointer);
static void	SpclAbbrevSelectCB (Widget, XtPointer, XtPointer);
extern void	InitFilterOpts (Properties *, char *, Boolean, FilterOpts *);
extern void	CopyFilterOpts (FilterOpts *, FilterOpts *);
static char	*ChangeRequest (Properties *);

static void	NewPage (Properties *);
static void	InitRequest (Properties *);

static void 	CreateProperties (Widget, Properties *);
static void 	UpdateProperties (Properties *);
static char	*CheckProperties (Widget, Properties *);
static void	ApplyProperties (Widget, Properties *);
static void	ResetProperties (Widget, Properties *);

static HelpText PropHelp = {
    TXT_propHelp, HELP_FILE, TXT_2propHelpSect,
};

/* Lower Control Area buttons */
static MenuItem CommandItems [] = {
    { (XtArgVal) TXT_ok, (XtArgVal) MNEM_ok, (XtArgVal) True,
	  (XtArgVal) ApplyCB, (XtArgVal) True, },	/* Apply */
    { (XtArgVal) TXT_reset, (XtArgVal) MNEM_reset, (XtArgVal) True,
	  (XtArgVal) ResetCB, },			/* Reset */
    { (XtArgVal) TXT_more, (XtArgVal) MNEM_other, (XtArgVal) True,
	  (XtArgVal) FilterOptsCB, },			/* More */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CancelCB, },			/* Cancel */
    { (XtArgVal) TXT_helpW, (XtArgVal) MNEM_helpW, (XtArgVal) True,
	  (XtArgVal) HelpCB, (XtArgVal) False,
	  (XtArgVal) &PropHelp, },			/* Help */
};

static ButtonItem YesNoItems [] = {
    { (XtArgVal) True, (XtArgVal) TXT_yes, },		/* Yes */
    { (XtArgVal) False, (XtArgVal) TXT_no, },		/* No */
};

static char *OptList [] = {
    "nobanner",
    "length",
    "width",
    "cpi",
    "lpi",
    "locale",
};

ButtonItem	*InputTypes;
unsigned	NumInputTypes;

/* PostProperties
 *
 * Post Property sheet for submitting jobs and changing existing jobs.
 * If the requested sheet is already posted, simply raise it to the top.
 * Otherwise, change to the new page.
 */
void
PostProperties (Widget widget, Properties *properties)
{
    /* If this is a new request, always create the property sheet, and
     * destroy it when it pops down.
     */
    if (!properties->id)
    {
	properties->page = (PropPg *) XtCalloc (1, sizeof (PropPg));
	properties->page->posted = properties;
	CreateProperties (widget, properties);
	XtAddCallback (properties->page->popup, XtNpopdownCallback,
		       Die, (XtPointer) Job_Canceled);
    }
    else
    {
	/* An existing request.  Allow only one property sheet per printer.
	 * The property sheet is not destroyed when finished.
	 */
	if (properties->page->popup)
	    if (properties == properties->page->posted)
		XRaiseWindow (XtDisplay (properties->page->popup),
			      XtWindow (properties->page->popup));
	    else
		NewPage (properties);
	else
	{
	    CreateProperties (widget, properties);
	    properties->page->posted = properties;
	    XtAddCallback (properties->page->popup, XtNpopdownCallback,
			   PopdownCB, (XtPointer) properties->page);
	}
    }

    XtPopup (properties->page->popup, XtGrabNone);

    OlSetInputFocus (properties->page->lca, RevertToNone, CurrentTime);

    properties->page->poppedUp = True;
}	/* End of PostProperties () */

/* NewPage
 *
 * Change the property sheet to reflect the values of the object.
 */
static void
NewPage (Properties *properties)
{
    PropPg	*page = properties->page;

    page->posted = properties;

    if (!properties->request)
	InitRequest (properties);

    ResetProperties (TopLevel, properties);
}	/* End of NewPage () */

/* PopdownCB
 *
 * Popdown callback.  Mark the property sheet as unposted.  client_data is
 * a pointer to the page.
 */
static void
PopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg	*page = (PropPg *) client_data;

    FooterMsg (page->footer, NULL);
    page->posted = (Properties *) 0;
    page->poppedUp = False;

    if (page->optionsPopup)
	XtPopdown (page->optionsPopup);
}	/* End of PopdownCB () */

/* CreateProperties
 *
 * Create widgets for the basic property sheet.
 */
static void
CreateProperties (Widget parent, Properties *properties)
{
    Widget		uca;
    Widget		ucaMenuShell;
    Widget		ucaMenu;
    Widget		lcaMenu;
    Widget		footer;
    PropPg		*page;
    char		title [256];
    static Boolean	first = True;
    static char		*copyLbl;
    static char		*mailLbl;
    static char		*bannerLbl;
    static char		*inputTypeLbl;
    static char		*otherInputTypeLbl;
    static char		*titleLbl;
    static char		*idLbl;
    static char		*userLbl;
    static char		*sizeLbl;
    static char		*dateLbl;
    static char		*stateLbl;
    static char		*titleStr;

    extern void		ReadInputTypes (ButtonItem **, unsigned *);

    /* Create popup and initialize all labels first time only */
    if (first)
    {
	first = False;

	copyLbl = GetStr (TXT_copies);
	mailLbl = GetStr (TXT_mail);
	bannerLbl = GetStr (TXT_banner);
	inputTypeLbl = GetStr (TXT_inputis);
	otherInputTypeLbl = GetStr (TXT_otherInputType);
	titleLbl = GetStr (TXT_title);
	idLbl = GetStr (TXT_id);
	userLbl = GetStr (TXT_user);
	sizeLbl = GetStr (TXT_size);
	dateLbl = GetStr (TXT_date);
	stateLbl = GetStr (TXT_state);
	titleStr = GetStr (TXT_winTitle);

	if (!properties->id)
	{
	    CommandItems [0].lbl = (XtArgVal) TXT_print;
	    CommandItems [0].mnem = (XtArgVal) MNEM_print;
	}
	SetLabels (CommandItems, XtNumber (CommandItems));
	SetButtonLbls (YesNoItems, XtNumber (YesNoItems));
	SetHelpLabels (&PropHelp);

	if (!InputTypes)
	    ReadInputTypes (&InputTypes, &NumInputTypes);
    }

    page = properties->page;

    if (!properties->request)
	InitRequest (properties);

    sprintf (title, titleStr, properties->printer->name);
    page->popup = XtVaCreatePopupShell ("properties",
		popupWindowShellWidgetClass,
		parent,
		XtNtitle,		(XtArgVal) title,
		0);

    XtAddCallback (page->popup, XtNverify, VerifyCB, (XtPointer) 0);

    XtVaGetValues (page->popup,
		XtNlowerControlArea,	(XtArgVal) &page->lca,
		XtNupperControlArea,	(XtArgVal) &uca,
		XtNfooterPanel,		(XtArgVal) &footer,
		0);

    /* We want an "apply" and "reset" buttons in both the lower control
     * area and in a popup menu on the upper control area.
     */
    lcaMenu = XtVaCreateManagedWidget ("lcaMenu",
		flatButtonsWidgetClass, page->lca,
		XtNclientData,		(XtArgVal) page,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
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
		XtNitems,		(XtArgVal) CommandItems,
		XtNnumItems,		(XtArgVal) XtNumber (CommandItems),
		0);

    OlAddDefaultPopupMenuEH (uca, ucaMenuShell);

    /* Make a text area in the footer for error messages. */
    page->footer = XtVaCreateManagedWidget ("footer",
		staticTextWidgetClass, footer,
		0);

    /* Create the controls for adding/changing print jobs */
    MakeText (uca, titleLbl, &page->titleCtrl, 35);
    MakeText (uca, copyLbl, &page->copyCtrl, 5);
    MakeButtons (uca, mailLbl, YesNoItems, XtNumber (YesNoItems),
		 &page->mailCtrl);
    MakeButtons (uca, bannerLbl, YesNoItems, XtNumber (YesNoItems),
		 &page->bannerCtrl);
    if (properties->printer->config->banner & BAN_ALWAYS)
	XtSetSensitive (page->bannerCtrl.btn, False);

    MakeAbbrevMenu (uca, inputTypeLbl, &page->inTypeCtrl,
		    InputTypes, NumInputTypes, 25);
    MakeText (uca, otherInputTypeLbl, &page->otherTypeCtrl, 8);
    XtSetMappedWhenManaged (XtParent (page->otherTypeCtrl.txt),
			    False);
    XtVaSetValues (page->inTypeCtrl.buttons.btn,
		XtNselectProc,		(XtArgVal) SpclAbbrevSelectCB,
		XtNclientData,		(XtArgVal) page,
		0);

    /* For existing jobs only, display the job id, size, etc. */
    if (properties->id)
    {
	MakeStaticText (uca, idLbl, &page->idCtrl, 20);
	MakeStaticText (uca, userLbl, &page->userCtrl, 8);
	MakeStaticText (uca, sizeLbl, &page->sizeCtrl, 9);
	MakeStaticText (uca, dateLbl, &page->dateCtrl, 11);
	MakeStaticText (uca, stateLbl, &page->stateCtrl, 40);
    }

    /* Make sure all widgets visually reflect the correct values */
    ResetProperties (page->popup, properties);
}	/* End of CreateProperties () */

/* InitProperties
 *
 * Initialize properties structure.  If job is 0, then the properties
 * structure is initialized with default values for adding new jobs.
 * If job is not 0, then the properties are extracted from the
 * configuration data.  This function does not initialize the lp request
 * structure for existing requests--that is deferred until the user
 * brings up the property sheet.  This avoids some unnecessary communication
 * with the spooler.
 */
void
InitProperties (Widget widget, Properties *properties, PrintJob *job)
{
    if (!job)
    {
	properties->id = (char *) 0;
	InitRequest (properties);
    }
    else
    {
	if (!properties->id)
	{
	    properties->id = strdup (job->id);
	    properties->request = (REQUEST *) 0;
	    properties->user = strdup (job->user);
	}

	properties->size = job->size;
	properties->date = job->date;
	properties->state = job->outcome;
    }
} /* End of InitProperties () */

/* InitRequest
 *
 * Initialize a request structure and the properties that relate to the
 * request.  If the request id is NULL, initialize the structure with
 * the default values.  Otherwise, ask lp to get the values.  If the
 * request fails, probably because the user doesn't have permission
 * or the job has already printed, quietly ignore the error and use
 * default values.
 */
static void
InitRequest (Properties *properties)
{
    REQUEST		*request;
    char		*option;
    char		buf [16];
    int			len;
    extern ResourceRec	AppResources;

    if (!properties->request)
	properties->request = (REQUEST *) XtCalloc (1, sizeof (REQUEST));

    if (properties->id)
    {
	request = LpInquire (properties->id);
	if (request)
	    *properties->request = *request;
    }
    else
	request = (REQUEST *) 0;

    if (!request)
    {
	request = properties->request;

	request->copies = 1;
	request->destination = strdup (properties->printer->name);
	request->file_list = 0;
	request->form = 0;
	request->actions = ACT_MAIL;
	request->alert = 0;
	request->options = strdup ("");
	request->priority = -1;
	request->pages = 0;
	request->charset = 0;
	request->title = strdup ("");
	request->modes = strdup ("");
	request->input_type = strdup (AppResources.contentType);
	request->user = getname ();
	request->outcome = 0;

	/* We only need to know the filters and filter options involved if
	 * we need to find the default values (as is the case here), or
	 * if we are actually going to bring up the property sheet to change
	 * them, in which case, finding the filters is deferred until this
	 * actually happens.
	 */
	InitFilterOpts (properties, request->input_type, True,
			&properties->originalFilter);

	properties->banner = (properties->printer->config->banner & BAN_OFF) ?
	                     No_Button : Yes_Button;
    }
    else
    {
	/* Divide the interface options into 3 parts--those that are
	 * addressed on the main property sheet, those on the filter
	 * options page, and those we don't know about.
	 */
	if (!request->options)
	    request->options = strdup ("");
	if (!request->modes)
	    request->modes = strdup ("");
	len = strlen (request->options) + 1;
	properties->originalFilter.opts = XtMalloc (len);
	properties->miscOpts = XtMalloc (len);
	*properties->originalFilter.opts = *properties->miscOpts = 0;
	properties->banner = Yes_Button;

	for (option=strtok(request->options," "); option;
	     option=strtok(NULL," "))
	{
	    switch (Lookup (option)) {
	    case NoBanner:
		properties->banner = No_Button;
		break;

	    case Length:
	    case Width:
	    case Cpi:
	    case Lpi:
	    case Locale:
		strcat (properties->originalFilter.opts, option);
		strcat (properties->originalFilter.opts, " ");
		break;

	    default:
		/* Simply save unknown options */
		strcat (properties->miscOpts, option);
		strcat (properties->miscOpts, " ");
		break;
	    }
	}

	properties->originalFilter.inputType = strdup (request->input_type);
    }

    /* Extract the properties values from the request */
    sprintf (buf, "%d", request->copies);
    properties->copies = strdup (buf);

    properties->title = strdup (request->title);
    properties->mail = request->actions & ACT_MAIL ? Yes_Button : No_Button;
    properties->inType = FindInputType (request->input_type);
    properties->otherType = strdup (request->input_type);
}	/* End if InitRequest () */

/* CheckProperties
 *
 * Check validity of new property sheet values.  Return pointer to error
 * text if there is a problem, else NULL.
 */
static char *
CheckProperties (Widget page, Properties *properties)
{
    short		copies;
    char		*text;
    static char		*noneSelectedMsg;
    static char		*badCopiesMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	noneSelectedMsg = GetStr (TXT_noneSelected);
	badCopiesMsg = GetStr (TXT_badCopies);
    }

    if (!properties->page->posted)
	return (noneSelectedMsg);

    /* Check the number of copies */
    text = GetText (&properties->page->copyCtrl);
    if (*text)
    {
	char	*ptr;

	copies = (short) strtol (text, &ptr, 10);
	if (copies <= 0 || *ptr)
	    return (badCopiesMsg);
    }

    /* Anything is valid for the title or other input type. */
    (void) GetText (&properties->page->titleCtrl);
    (void) GetText (&properties->page->otherTypeCtrl);

    return (NULL);
}	/* End of CheckProperties () */

/* ApplyProperties
 *
 * Update property sheet to reflect the changed values.
 */
static void
ApplyProperties (Widget page, Properties *properties)
{
    ApplyText (&properties->page->titleCtrl, &properties->title);
    ApplyText (&properties->page->copyCtrl, &properties->copies);
    properties->mail = properties->page->mailCtrl.setIndx;
    properties->banner = properties->page->bannerCtrl.setIndx;
    ApplyAbbrevMenu (&properties->page->inTypeCtrl, &properties->inType);
    ApplyText (&properties->page->otherTypeCtrl, &properties->otherType);

    FreeFilterOpts (&properties->originalFilter);
    CopyFilterOpts (&properties->appliedFilter, &properties->originalFilter);
}	/* End of ApplyProperties () */

/* ResetProperties
 *
 * Reset property sheet to its original values
 */
static void
ResetProperties (Widget widget, Properties *properties)
{
    PropPg		*page;
    static Boolean	first = True;
    static char		*heldLbl;
    static char		*filteringLbl;
    static char		*filteredLbl;
    static char		*printingLbl;
    static char		*printedLbl;
    static char		*changingLbl;
    static char		*canceledLbl;
    static char		*immediateLbl;
    static char		*failedLbl;
    static char		*notifyLbl;
    static char		*notifyingLbl;

    page = properties->page;

    if (first)
    {
	first = False;

	heldLbl = GetStr (TXT_held);
	filteringLbl = GetStr (TXT_filtering);
	filteredLbl = GetStr (TXT_filtered);
	printingLbl = GetStr (TXT_printing);
	printedLbl = GetStr (TXT_printed);
	changingLbl = GetStr (TXT_changing);
	canceledLbl = GetStr (TXT_canceled);
	immediateLbl = GetStr (TXT_immediate);
	failedLbl = GetStr (TXT_failed);
	notifyLbl = GetStr (TXT_notify);
	notifyingLbl = GetStr (TXT_notifying);
    }

    XtVaSetValues (page->titleCtrl.txt,
		XtNstring,		(XtArgVal) properties->title,
		0);
    XtVaSetValues (page->copyCtrl.txt,
		XtNstring,		(XtArgVal) properties->copies,
		0);
    page->mailCtrl.setIndx = properties->mail;
    OlVaFlatSetValues (page->mailCtrl.btn, properties->mail,
		XtNset,			(XtArgVal) True,
		0);
    page->bannerCtrl.setIndx = properties->banner;
    OlVaFlatSetValues (page->bannerCtrl.btn, properties->banner,
		XtNset,			(XtArgVal) True,
		0);
    ResetAbbrevMenu (&page->inTypeCtrl, properties->inType);
    XtVaSetValues (page->otherTypeCtrl.txt,
		XtNstring,		(XtArgVal) properties->otherType,
		0);
    XtSetMappedWhenManaged (XtParent (page->otherTypeCtrl.txt),
			    (properties->inType == NumInputTypes - 1));

    FreeFilterOpts (&properties->appliedFilter);
    CopyFilterOpts (&properties->originalFilter, &properties->appliedFilter);

    if (properties->id)
    {
	char	buf [256];

	XtVaSetValues (page->idCtrl,
		XtNstring,		(XtArgVal) properties->id,
		0);

	XtVaSetValues (page->userCtrl,
		XtNstring,		(XtArgVal) properties->user,
		0);

	sprintf (buf, "%d", properties->size);
	XtVaSetValues (page->sizeCtrl,
		XtNstring,		(XtArgVal) buf,
		0);

	cftime (buf, "%c", &properties->date);
	XtVaSetValues (page->dateCtrl,
		XtNstring,		(XtArgVal) buf,
		0);

	buf [0] = 0;
	if (properties->state & RS_HELD)
	    strcat (buf, heldLbl);
	if (properties->state & RS_FILTERING)
	    strcat (buf, filteringLbl);
	if (properties->state & RS_FILTERED)
	    strcat (buf, filteredLbl);
	if (properties->state & RS_PRINTING)
	    strcat (buf, printingLbl);
	if (properties->state & RS_PRINTED)
	    strcat (buf, printedLbl);
	if (properties->state & RS_CHANGING)
	    strcat (buf, changingLbl);
	if (properties->state & RS_CANCELLED)
	    strcat (buf, canceledLbl);
	if (properties->state & RS_IMMEDIATE)
	    strcat (buf, immediateLbl);
	if (properties->state & RS_FAILED)
	    strcat (buf, failedLbl);
	if (properties->state & RS_NOTIFY)
	    strcat (buf, notifyLbl);
	if (properties->state & RS_NOTIFYING)
	    strcat (buf, notifyingLbl);

	XtVaSetValues (page->stateCtrl,
		XtNstring,		(XtArgVal) buf,
		0);
    }

    if (page->optionsPopup)
    {
	page->optionsOwner = 0;
	FilterOptsCB (page->popup, (XtPointer) page, (XtPointer) 0);
    }
}	/* End of ResetProperties () */

/* UpdateProperties
 *
 * Populate a job request structure with the new attributes.  request is
 * a copy of properties->request, so we can't free string pointers within
 * the structure before we replace them with the updated versions.
 */
static void
UpdateProperties (Properties *properties)
{
    REQUEST	*request;
    char	*lcl;
    char	*endLcl;
    char	options [256];

    request = properties->request;
    options [0] = 0;

    request->copies = atoi (properties->page->copyCtrl.setText);
    if (request->copies <= 0)
	request->copies = 1;

    request->actions = (Boolean) YesNoItems
	[properties->page->mailCtrl.setIndx].userData ? ACT_MAIL : 0;

    XtFree (request->title);
    request->title = strdup (properties->page->titleCtrl.setText);

    /* If the input type is different from when the filter options were
     * last applied, then use the default values for the filter options;
     * otherwise, use the last applied values.
     */
    XtFree (request->input_type);
    if (properties->page->inTypeCtrl.buttons.setIndx == NumInputTypes - 1)
	request->input_type = strdup (properties->page->otherTypeCtrl.setText);
    else
	request->input_type =
	    strdup ((char *) InputTypes [properties->page->
					 inTypeCtrl.buttons.setIndx].userData);
    if (strcmp (request->input_type, properties->appliedFilter.inputType) != 0)
    {
	FreeFilterOpts (&properties->appliedFilter);
	InitFilterOpts (properties, request->input_type, True,
			&properties->appliedFilter);
    }

    XtFree (request->charset);
    if (properties->appliedFilter.charSet == 0)
	request->charset = 0;
    else
	request->charset = strdup ((char *) properties->printer->
	    charSetItems [properties->appliedFilter.charSet].lbl);

    XtFree (request->options);
    if (!(Boolean) YesNoItems [properties->page->bannerCtrl.setIndx].userData)
	strcat (options, "nobanner ");
    if (properties->appliedFilter.opts[0] != '0')
	strcat (options, properties->appliedFilter.opts);
    if (properties->miscOpts[0] != '0')
	strcat (options, properties->miscOpts);
    request->options = strdup (options);

    /* If local is specified, it must be included in both options and modes.
     * To avoid problems when locale is already listed in both places (what
     * if they aren't set to the same thing?), strip locale out of the modes
     * string and copy the value from the options string.  If the locale is
     * C, then it must not be copied into modes.
     */
    XtFree (request->modes);
    while (lcl = strstr (properties->appliedFilter.modes, "locale="))
    {
	/* Modes are space separated.  Remove locale from the string. */
	endLcl = strchr (lcl, ' ');
	if (endLcl)
	    strcpy (lcl, endLcl);
	else
	{
	    *lcl = 0;
	    break;
	}
    }

    if (lcl = strstr (properties->appliedFilter.opts, "locale="))
    {
	lcl += 7;
	if (strcmp (lcl, "C") && strncmp (lcl, "C ", 2) && *lcl && *lcl != ' ')
	{
	    int		len;

	    /* Not in C locale */
	    endLcl = strchr (lcl, ' ');
	    if (endLcl)
		len = endLcl - (lcl) ;
	    else
		len = strlen (lcl);

	    request->modes =
		XtMalloc (strlen (properties->appliedFilter.modes) + len + 9);
	    sprintf (request->modes, "%s locale=",
		     properties->appliedFilter.modes);
	    strncat (request->modes, lcl, len);
	    request->modes[strlen (properties->appliedFilter.modes) +
		len + 8] = 0;
	}
	else
	    request->modes = strdup (properties->appliedFilter.modes);
    }
    else{
        char *loc = NULL;

        if ((loc = setlocale (LC_CTYPE,""))){
		printf("%s LOCALE \n",loc);
             if (strcmp (loc, "C") && strncmp (loc, "C ", 2) && *loc && *loc !=
' ')
             {
              	request->modes =
                   XtMalloc (strlen (properties->appliedFilter.modes) +
                                strlen(loc) + 9);
		sprintf (request->modes, "%s locale=",
                     properties->appliedFilter.modes);
		strcat (request->modes, loc);
             }else
		request->modes = strdup (properties->appliedFilter.modes);


	}else
             request->modes = strdup (properties->appliedFilter.modes);
	}

}	/* End of UpdateProperties () */

/* ApplyCB
 *
 * Property sheet apply callback.  Check sheet for validity, and if valid,
 * make the changes.  client_data is a pointer to the properties page data.
 */
static void
ApplyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg	*page = (PropPg *) client_data;
    Properties	*properties = page->posted;
    char	*err;
    extern void	SubmitJob (Properties *);

    /* Check all property sheets for validity.  Errors here are not
     * horribly serious, so display the message in the footer.
     */
    FooterMsg (page->footer, NULL);
    if (err = CheckProperties (widget, properties))
    {
	FooterMsg (page->footer, err);
	return;
    }

    UpdateProperties (properties);

    /* Submit new jobs or update existing ones. */
    if (properties->id)
    {
	/* existing request */
	if (err = ChangeRequest (properties))
	{
	    Error (widget, err);
	    return;
	}

	ApplyProperties (widget, properties);
	BringDownPopup (page->popup);
    }
    else
    {
	/* New request.  Don't pop the window down, because there
	 * might be some error that the user can still fix.  Whether
	 * or not the request is successful, there is no need to apply
	 * the properties; if the request failed, the properties are
	 * wrong, and if successful, the process will be killed, anyway.
	 */
	XtVaSetValues (page->popup,
		XtNbusy,		(XtArgVal) True,
		0);
	SubmitJob (properties);
    }
}	/* End of ApplyCB () */

/* ResetCB
 *
 * Reset the list to the original values.  client_data refers to the
 * property page.
 */
static void
ResetCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg	*page = (PropPg *) client_data;
    Properties	*properties = page->posted;

    FooterMsg (page->footer, NULL);
    if (!properties)
	return;
    ResetProperties (widget, properties);
}	/* End of ResetCB () */

/* CancelCB
 *
 * Arrange for the property sheet to pop down.  client_data refers to the
 * property page.
 */
void
CancelCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    /* Get the shell widget for the sheet.  The shell is the first shell
     * up the ancestor chain.
     */
    while (!XtIsShell (widget))
	widget = XtParent (widget);

    XtPopdown (widget);
}	/* End of CancelCB () */

/* VerifyCB
 *
 * Verify callback.  Because the verify callback is not called
 * in motif mode, never set the flag to true; the individual button
 * callbacks will bring down the popup in needed.
 */
void
VerifyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Boolean	*pOk = (Boolean *) call_data;

    *pOk = False;
}	/* End of VerifyCB () */

/* BringDownPopup
 *
 * Check the state of the pushpin, and if it is not in, popdown the
 * property sheet.
 */
void
BringDownPopup (Widget popup)
{
    OlDefine	pinState;

    XtVaGetValues (popup,
		XtNpushpin,		(XtArgVal) &pinState,
		0);

    if (pinState != OL_IN)
	XtPopdown (popup);

}	/* End of BringDownPopup () */

/* ChangeRequest
 *
 * Call the lp utilities to change an existing print request.  Return
 * a pointer to the error text if there was an error.
 */
static char *
ChangeRequest (Properties *properties)
{
    int			status;
    char		*err;
    static char		*permMsg;
    static char		*filterMsg;
    static char		*unsettableMsg;
    static char		*doneMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	permMsg = GetStr (TXT_noChgPerm);
	filterMsg = GetStr (TXT_noFilter);
	unsettableMsg = GetStr (TXT_unsettable);
	doneMsg = GetStr (TXT_jobChgDone);
    }

    status = LpChangeRequest (properties->id, properties->request);

    switch (status) {
    case MOK:
	err = (char *) 0;
	break;

    case MUNKNOWN:
    case MBUSY:
    case M2LATE:
	err = doneMsg;
	break;

    case MNOFILTER:
	err = filterMsg;
	break;

    case MDENYDEST:
	err = unsettableMsg;
	break;

    case MNOPERM:
    default:
	err = permMsg;
	break;

    }

    return (err);
} /* End of ChangeRequest () */

/* FreeProperties
 *
 * Free all dynamically allocated storage associated with a request.
 */
void
FreeProperties (Properties *properties)
{
    REQUEST	*request = properties->request;

    XtFree (properties->id);
    XtFree (properties->copies);
    XtFree (properties->miscOpts);
    XtFree (properties->title);
    XtFree (properties->otherType);
    XtFree (properties->user);

    if (request)
    {
	XtFree (request->destination);
	XtFree (request->form);
	XtFree (request->alert);
	XtFree (request->options);
	XtFree (request->pages);
	XtFree (request->charset);
	XtFree (request->title);
	XtFree (request->modes);
	XtFree (request->input_type);
	XtFree (request->user);
	XtFree ((char *) request);
    }

    FreeFilterOpts (&properties->originalFilter);
    FreeFilterOpts (&properties->appliedFilter);

    XtFree ((char *) properties);
}	/* End of FreeProperties () */

/* Lookup
 *
 * Lookup a word in the list of known options.
 */
int
Lookup (char *word)
{
    int		n;
    register	i;

    /* If the word contains an =, only check the first part of the word. */
    n = strcspn (word, "=");

    for (i=0; i<XtNumber(OptList); i++)
	if (!strncmp (word, OptList [i], n))
	    if (!OptList [i][n])
		return (i);

    return (Unknown);
}	/* End of Lookup () */

/* FindInputType
 *
 * Lookup the input type name in the list of known types.  The last item in
 * the list is "other".
 */
static int
FindInputType (char *inputType)
{
    ButtonItem	*item;
    register    otherType;
    register	i;

    if (!inputType || !*inputType)
	inputType = "simple";

    otherType = NumInputTypes - 1;
    for (item=InputTypes, i=0; i<otherType; i++, item++)
	if (!strcmp (inputType, (char *) item->userData))
	    return (i);

    return (i);
}	/* End of FindInputType () */

/* SpclAbbrevSelectCB
 *
 * Similar to the select callback for abbreviated menu button controls, but
 * mapping the "other" text field when needed.  This function also forces
 * a reset of the filter specific options, if that property sheet is posted.
 */
static void
SpclAbbrevSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropPg		*page = (PropPg *) client_data;
    OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;
    ButtonItem		*selected;
    extern void		AbbrevSelectCB (Widget, XtPointer, XtPointer);

    AbbrevSelectCB (widget, (XtPointer) &page->inTypeCtrl, call_data);
    selected = (ButtonItem *) pFlatData->items + pFlatData->item_index;
    XtSetMappedWhenManaged (XtParent (page->otherTypeCtrl.txt),
			    !selected->userData);

    if (page->optionsPopup)
    {
	page->optionsOwner = 0;
	FilterOptsCB (page->popup, client_data, (XtPointer) 0);
    }
}	/* End of SpclAbbrevSelectCB () */
