#ifndef	NOIDENT
#ident	"@(#)dtadmin:dashboard/misc.c	1.4"
#endif

/* *************************************************************************
 *
 * Description: TIME ZONE
 *
 ******************************file*header******************************** */
#include <stdio.h>
#include <string.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/BaseWindow.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>
#include <Xol/Modal.h>
#include <libDtI/DtI.h>
#include "dashmsgs.h" 
#include "tz.h" 
#include <Gizmo/Gizmos.h>

/* ************************************************************************* 
 * Define global/static variables and #defines, and 
 * Declare externally referenced variables
 *
 *****************************file*variables*******************************/
void 		ErrorNotice (Widget, char *);
static void	ErrorSelectCB (Widget, XtPointer, XtPointer);
static void	ErrorPopdownCB (Widget, XtPointer, XtPointer);
void 		SetCountryLabels ();
void 		SetLabels ();
void 		tzhelpCB();
void 		VerifyCB();

/*******************************************************
	error fields
*******************************************************/
static String	LcaFields [] = {
    XtNlabel, XtNmnemonic, XtNdefault, 
};

static struct {
    XtArgVal	lbl;
    XtArgVal	mnem;
    XtArgVal	dflt;
} LcaItems [1];


/*********************************************************************************
	error routines are called over here
*********************************************************************************/
/* Error Notification
 *
 * Display a notice box with an error message.  The only button is a
 * "OK" button.
 */
void
ErrorNotice (Widget widget, char *errorMsg)
{
    Widget		notice;
    static Boolean	first = True;
	char 		*mnem;

    if (first)
    {
	first = False;
	LcaItems [0].lbl = (XtArgVal) GetGizmoText (TXT_OK);
	mnem =  GetGizmoText (MNEM_OK);
	LcaItems [0].mnem = (XtArgVal) mnem[0];
	LcaItems [0].dflt = (XtArgVal) True;
    }

    notice = XtVaCreatePopupShell ("Message", modalShellWidgetClass, widget,
				   0);

    /* Add the error message text */
    XtVaCreateManagedWidget ("errorTxt", staticTextWidgetClass, notice,
		XtNstring,		(XtArgVal) errorMsg,
		XtNalignment,		(XtArgVal) OL_CENTER,
    		XtNfont,		(XtArgVal) _OlGetDefaultFont (widget,
							OlDefaultNoticeFont),
		0);

    /* Add the OK button to the bottom */
    (void) XtVaCreateManagedWidget ("lcaButton",
		flatButtonsWidgetClass, notice,
		XtNclientData,		(XtArgVal) notice,
		XtNselectProc,		(XtArgVal) ErrorSelectCB,
		XtNitemFields,		(XtArgVal) LcaFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (LcaFields),
		XtNitems,		(XtArgVal) LcaItems,
		XtNnumItems,		(XtArgVal) XtNumber (LcaItems),
		0);

    XtAddCallback (notice, XtNpopdownCallback, ErrorPopdownCB,
		   (XtPointer) 0);

    XtPopup (notice, XtGrabExclusive);
} /* End of Error () */

/* ErrorSelectCB
 *
 * When a button is pressed in the lower control area, popdown the notice.
 * The notice is given an client_data.
 */
static void
ErrorSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtPopdown ((Widget) client_data);
} /* End of ErrorSelectCB () */

/* ErrorPopdownCB
 *
 * Destroy Error notice on popdown
 */
static void
ErrorPopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtDestroyWidget (widget);
} /* End of ErrorPopdownCB () */

/********************************************************************
 * SetCountryLabel
 * Set choice items 
********************************************************************/
void
SetCountryLabels (_choice_items *items, int cnt)
{
        char        *mnem;
    	for ( ;--cnt>=0; items++) {
       	 	items->label = (XtArgVal) GetGizmoText ((char *) items->label);
                if ( items->mnemonic != NULL ){
                	mnem = GetGizmoText ((char *) items->mnemonic);
                	items->mnemonic = (XtArgVal) mnem [0];
                }
	}
}       /* End of SetCountryLabels */

/********************************************************************
 * SetLabels
 * Set menu item labels and mnemonics.
********************************************************************/
void
SetLabels (MenuItem *items, int cnt)
{
    char        *mnem;

    for ( ;--cnt>=0; items++)
    {
        items->lbl = (XtArgVal) GetGizmoText ((char *) items->lbl);
        mnem = GetGizmoText ((char *) items->mnem);
        items->mnem = (XtArgVal) mnem [0];
    }
}       /* End of SetLabels */


 /***************************************************************************
 * Verify callback.  client_data is a pointer to the flag indicating if it's
 * ok to pop the window down.  Reset this flag back to false.
 ***************************************************************************/
void
VerifyCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Boolean     *pOk = (Boolean *) call_data;
    Boolean     *pFlag = (Boolean *) client_data;

    *pOk = *((Boolean *) client_data);
    *pFlag = False;
}       /* End of VerifyCB () */

/***********************************************************
		help routine
*************************************************************/
void
tzhelpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
        HelpInfo *help = (HelpInfo *) client_data;
        static String help_app_title;

        if (help_app_title == NULL)
                help_app_title = GetGizmoText(string_appName);

        help->app_title = help_app_title;
        help->title = string_appName;
        help->section = GetGizmoText(help->section);
        PostGizmoHelp(XtParent(XtParent(w)), help);
}

