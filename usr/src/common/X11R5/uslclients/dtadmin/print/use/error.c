#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/error.c	1.9"
#endif

#include <stdlib.h>
#include <unistd.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/Modal.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>

#include "error.h"
#include "properties.h"

static void	ErrorSelectCB (Widget, XtPointer, XtPointer);
static void	ErrorPopdownCB (Widget, XtPointer, XtPointer);

/* Lower Control Area buttons */
static String	LcaFields [] = {
    XtNlabel, XtNdefault,
};

static struct {
    XtArgVal	lbl;
    XtArgVal	dflt;
} LcaItems [1];

/* Error Notification
 *
 * Display a notice box with an error message.  The only button is a
 * "continue" button.
 */
void
Error (Widget widget, char *errorMsg)
{
    ErrorConfirm (widget, errorMsg, 0, 0);
} /* End of Error () */

/* ErrorPopdownCB
 *
 * Destroy Error notice on popdown
 */
static void
ErrorPopdownCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtDestroyWidget (widget);
} /* End of ErrorPopdownCB () */

/* ErrorConfirm
 *
 * Similar to Error, except allows a callback function to be called when
 * the notice pops down.
 */
void
ErrorConfirm (Widget widget, char *errorMsg, XtCallbackProc callback,
	      XtPointer closure)
{
    Widget		notice;
    static Boolean	first = True;
    static char		*titleMsg;

    if (first)
    {
	first = False;
	titleMsg = GetStr (TXT_errorTitle);
	LcaItems [0].lbl = (XtArgVal) GetStr (TXT_continue);
	LcaItems [0].dflt = (XtArgVal) True;
    }

    if ((int) closure != Job_Submitted) 
	    notice = XtVaCreatePopupShell ("Message", modalShellWidgetClass, 
		widget,
		XtNtitle,		(XtArgVal) titleMsg,
		0);
    else
	    notice = XtVaCreatePopupShell ("Message", modalShellWidgetClass, 
		widget,
		XtNtitle,		(XtArgVal) titleMsg,
		XtNnoticeType,		(XtArgVal) OL_INFORMATION,
		0);

    /* Add the error message text */
    XtVaCreateManagedWidget ("errorTxt", staticTextWidgetClass, notice,
		XtNstring,		(XtArgVal) errorMsg,
		XtNalignment,		(XtArgVal) OL_CENTER,
    		XtNfont,		(XtArgVal) _OlGetDefaultFont (widget,
							OlDefaultNoticeFont),
		0);

    /* Add the continue button to the bottom */
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

    if (callback)
	XtAddCallback (notice, XtNpopdownCallback, callback, closure);

    XtPopup (notice, XtGrabExclusive);
}	/* End of ErrorConfirm () */

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

/* GetStr
 *
 * Get an internationalized string.  String id's contain both the filename:id
 * and default string, separated by the FS_CHR character.
 */
char *
GetStr (char *idstr)
{
    char	*sep;
    char	*str;

    sep = strchr (idstr, FS_CHR);
    *sep = 0;
    str = gettxt (idstr, sep + 1);
    *sep = FS_CHR;

    return (str);
}	/* End of GetStr () */
