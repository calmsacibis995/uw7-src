#ifndef NOIDENT
#ident	"@(#)dtadmin:print/use/cancel.c	1.11"
#endif

#include <stdio.h>
#include <string.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/Modal.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>

#include <lp.h>
#include <msgs.h>

#include "properties.h"
#include "lpsys.h"
#include "error.h"

extern void	CheckQueue (XtPointer client_data, XtIntervalId *pId);

static void	CancelJobs (Widget, XtPointer, XtPointer);
static void	CleanUp (Widget, XtPointer, XtPointer);

static Widget		DeleteNotice;
static Widget		DeleteText;
static Widget		DeleteBtns;
static IconItem		**DeleteList;
static Cardinal		DeleteCnt;

/* Lower Control Area buttons */
static MenuItem DeleteItems [] = {
    { (XtArgVal) TXT_delete, (XtArgVal) MNEM_delete, (XtArgVal) True,
	  (XtArgVal) CancelJobs, (XtArgVal) True, },	/* Delete */
    { (XtArgVal) TXT_cancel, (XtArgVal) MNEM_cancel, (XtArgVal) True,
	  (XtArgVal) CleanUp, },			/* Cancel */
};

/* DeleteJobs
 *
 * Post a notice to confirm if the user really wants to Delete the
 * selected jobs from the print queue.  client_data is a pointer to the
 * printer.
 */
void
DeleteJobs (Widget widget, XtPointer client_data, XtPointer call_data)
{
    Printer		*printer = (Printer *) client_data;
    IconItem		*item;
    Cardinal		cnt;
    int			msgLen;
    int			bufLen;
    char		*msg;
    register		i;
    static char		*confirmMsg;
    static char		*deleteTitle;
    static Boolean	first = True;

    /* Set Labels and create a notice asking if the user really wants
     * to do this.
     */
    if (first)
    {
	first = False;
	SetLabels (DeleteItems, XtNumber (DeleteItems));
	confirmMsg = GetStr (TXT_reallyDelete);
	deleteTitle = GetStr (TXT_deleteTitle);

	DeleteNotice = XtVaCreatePopupShell ("deleteNotice",
		modalShellWidgetClass, widget,
		XtNnoticeType,		(XtArgVal) OL_WARNING,
		XtNtitle,		(XtArgVal) deleteTitle,
		0);

	/* Add the error message widget */
	DeleteText = XtVaCreateManagedWidget ("delTxt", staticTextWidgetClass,
		DeleteNotice,
		XtNalignment,		(XtArgVal) OL_CENTER,
    		XtNfont,		(XtArgVal) _OlGetDefaultFont (widget,
							OlDefaultNoticeFont),
		0);

	/* Add delete/cancel buttons to the bottom */
	DeleteBtns = XtVaCreateManagedWidget ("lcaButton",
		flatButtonsWidgetClass, DeleteNotice,
		XtNitemFields,		(XtArgVal) MenuFields,
		XtNnumItemFields,	(XtArgVal) NumMenuFields,
		XtNitems,		(XtArgVal) DeleteItems,
		XtNnumItems,		(XtArgVal) XtNumber (DeleteItems),
		0);
    }

    /* Get the select printers */
    XtVaGetValues (printer->iconbox,
		XtNitems,		(XtArgVal) &item,
		XtNnumItems,		(XtArgVal) &cnt,
	        0);

    msgLen = strlen (confirmMsg);
    bufLen = msgLen + cnt*20;
    msg = (char *) XtMalloc (bufLen);
    strcpy (msg, confirmMsg);

    DeleteList = (IconItem **) XtMalloc (cnt * sizeof (*DeleteList));
    DeleteCnt = 0;
    for (i=cnt; --i>=0; item++)
    {
	if (item->managed && item->selected)
	{
	    DeleteList [DeleteCnt++] = item;
	    msgLen += strlen (((Properties *) item->properties)->id) + 1;
	    if (msgLen >= bufLen)
	    {
		bufLen = msgLen + 100;
		msg = XtRealloc (msg, bufLen);
	    }
	    strcat (msg, ((Properties *) item->properties)->id);
	    strcat (msg, "\n");
	}
    }

    XtVaSetValues (DeleteText,
		XtNstring,		(XtArgVal) msg,
		0);

    XtPopup (DeleteNotice, XtGrabExclusive);

    XtFree (msg);
}	/* End of DeleteJobs () */

/* CleanUp
 *
 * Free resources associated with the delete notice and popdown window.
 */
static void
CleanUp (Widget widget, XtPointer client_data, XtPointer call_data)
{
    XtFree ((char *) DeleteList);
    XtPopdown (XtParent (widget));
}	/* End of CleanUp () */

/* CancelJobs
 *
 * Cancel the jobs in the delete list.  Display a message if a job can not
 * be deleted.
 */
static void
CancelJobs (Widget widget, XtPointer client_data, XtPointer call_data)
{
    int			status;
    Printer		*printer;
    char		**permList;
    char		**doneList;
    int			permCnt;
    int			doneCnt;
    int			msgLen;
    Boolean		fail;
    register		i;
    static char		*cancelMsg;
    static char		*permMsg;
    static char		*doneMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	cancelMsg = GetStr (TXT_badCancel);
	permMsg = GetStr (TXT_noPerm);
	doneMsg = GetStr (TXT_jobDone);
    }

    /* The fact that we got here at all says that DeleteList has at least
     * one element in it.
     */
    printer = ((Properties *) DeleteList [0]->properties)->printer;

    msgLen = 0;
    fail = False;
    permCnt = doneCnt = 0;
    for (i=0; i<DeleteCnt; i++)
    {
	char	*id = ((Properties *) DeleteList [i]->properties)->id;

	status = LpCancel (id);
	switch (status) {
	case MNOPERM:
	    if (permCnt == 0)
	    {
		permList = (char **) XtMalloc (DeleteCnt * sizeof (char *));
		msgLen += strlen (permMsg);
	    }
	    permList [permCnt++] = (id);
	    msgLen += strlen (id) + 1;
	    break;

	case MUNKNOWN:
	case M2LATE:
	    if (doneCnt == 0)
	    {
		doneList = (char **) XtMalloc (DeleteCnt * sizeof (char *));
		msgLen += strlen (doneMsg);
	    }
	    doneList [doneCnt++] = id;
	    msgLen += strlen (id) + 1;
	    /* Fall Through!!! */

	case MOK:
	    /* The job was deleted.  Note that we do not free the
	     * properties here; they will be cleaned up when the
	     * queue is checked later on.
	     */
	    break;

	default:
	    fail = True;
	    msgLen += strlen (cancelMsg);
	    break;
	}
    }

    (void) LpCancel (0);	/* close the connection to the spooler */

    /* If jobs couldn't be deleted, post a notice */
    if (permCnt > 0 || doneCnt > 0 || fail)
    {
	char	*msg;

	msg = XtMalloc (msgLen + 3);	/* Allow for NULL plus 2 newlines */
	msg [0] = 0;
	if (fail)
	    strcpy (msg, cancelMsg);

	if (permCnt > 0)
	{
	    strcat (msg, permMsg);
	    for (i=0; i<permCnt; i++)
	    {
		strcat (msg, permList [i]);
		strcat (msg, "\n");
	    }

	    if (doneCnt > 0)
		strcat (msg, "\n\n");

	    XtFree ((char *) permList);
	}

	if (doneCnt > 0)
	{
	    strcat (msg, doneMsg);
	    for (i=0; i<doneCnt; i++)
	    {
		strcat (msg, doneList [i]);
		strcat (msg, "\n");
	    }

	    XtFree ((char *) doneList);
	}

	Error (TopLevel, msg);
	XtFree (msg);
    }

    CleanUp (widget, 0, 0);

    /* Remove the deleted printers from the icon box */
    CheckQueue (printer, (XtIntervalId *) 0);
}	/* End of CancelJobs () */
