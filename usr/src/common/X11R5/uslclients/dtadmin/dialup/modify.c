#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/modify.c	1.6"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <StaticText.h>
#include "uucp.h"
#include "error.h"

extern void DeviceNotifyUser();
extern void NotifyUser();
Widget getShell(Widget);


Widget
getShell(w)
Widget w;
{
 	Widget shell;
	shell = w;
	while (( XtIsShell(shell)) == False) {
		/* get the parent of the current widget */
		shell = XtParent(shell);
	}

return (shell);
}
/* ModifyNameCB
 *
 * Textfield modify callback.  As characters are types, check for white
 * space and non-printable characters.
 * call_data is a pointer to OlTextModifyCallData.
 * client_data is the footer area for message.
 */
void
ModifyNameCB (widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
    OlTextModifyCallDataPointer		tf_verify;
    char	*cp;
    register		i;
    static Boolean	first_time = True;
    static char		*format;

    if (first_time)
    {
	first_time = False;

	format = GGT (string_invalidChar);
    }

    ClearLeftFooter((Widget) client_data);
    tf_verify = (OlTextModifyCallDataPointer) call_data;
    for (cp=tf_verify->text, i=tf_verify->text_length; --i>=0; cp++)
    {
	if (!isgraph (*cp))
	{
	    char	msg [256];
	    sprintf (msg, format, *cp);
#ifdef DEBUG
		fprintf(stderr,"before OlCategorySetPage : category=%0x page=%0x\n",sf->category, sf->pages[0]);
#endif
		if (sf->category) OlCategorySetPage(sf->category, sf->pages[0]);
		/* called only from dtcall and dial so use device notify */
		DeviceNotifyUser(df->toplevel, msg);
	    	tf_verify->ok = False;
	    	return;
	}
    }
} /* ModifyNameCB () */

/* ModifyPhoneCB
 *
 * Phone Textfield modify callback.  As characters are types, validate
 * the characters and throw out all the invalid characters.  Valid
 * characters for the 'cu' command include the following characters:
 *	'0-9', '-', '=', '*' and '#'.
 *
 * call_data is a pointer to OlTextModifyCallData.
 * client_data is the footer area for message.
 */
void
ModifyPhoneCB (widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
    OlTextModifyCallDataPointer		tf_verify;
    char	*cp;
    register		i;
    static Boolean	first_time = True;
    static char		*invalidChar, *pauseChar;
	Widget w;

    if (first_time) {
		first_time = False;
		invalidChar = GGT (string_invalidChar);
		pauseChar = GGT (string_pauseChar);
    }

    ClearLeftFooter((Widget) client_data);
    tf_verify = (OlTextModifyCallDataPointer) call_data;
    if (strcspn(tf_verify->text, "0123456789=-*#")) {
		/* found an offend character */
		char	msg [256];
		tf_verify->ok = False;

	/* find out which one is the offend character */
	/* according to 'cu' manual page, the comma   */
	/* character is also not permitted, but it is */
	/* reasonable to tell the user that a '-'     */
	/* character can be used to replace the comma */

		for (cp=tf_verify->text, i=tf_verify->text_length; --i>=0; cp++) {
	    if (*cp == ',') {
				sprintf (msg, pauseChar);
#ifdef DEBUG
		fprintf(stderr,"before OlCategorySetPage : category=%0x page=%0x\n",sf->category, sf->pages[0]);
#endif
				if (sf->category) OlCategorySetPage(sf->category, sf->pages[0]);
				/* find out which shell is the parent of the phone # widget */
				w = getShell(widget);
				if (w == df->toplevel) {
					DeviceNotifyUser(df->toplevel, msg);
				} else {
					NotifyUser(w, msg);
				}
				return;
	    } else if (!isdigit(*cp) || *cp != '=' || *cp != '-'
			|| *cp != '*' || *cp != '#') {
				sprintf (msg, invalidChar, *cp);
#ifdef DEBUG
		fprintf(stderr,"before OlCategorySetPage : category=%0x page=%0x\n",sf->category, sf->pages[0]);
#endif
				if (sf->category) OlCategorySetPage(sf->category, sf->pages[0]);
				/* find out which shell is the parent of the phone # widget */
				if (w == sf->toplevel) {
					NotifyUser(w, msg);
				} else {
					DeviceNotifyUser(df->toplevel, msg);
				}
				return;
	    }
		}
    }
} /* ModifyPhoneCB() */
