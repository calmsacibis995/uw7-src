#ident	"@(#)misc.C	1.2"
/*******************************************************************************
 * Miscellaneous:: 	Generic object containing static functions to be invoked
 * 					directly by an application.
 ******************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <iostream.h>
#include <unistd.h>

#include "misc.h"
#include "Dialog.h"

#include <Xm/SashP.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

static const char FS_CHR = '\001';


extern Window
DtSetAppId(
        Display *       dpy,
        Window          id_owner,
        char *          id
)
{
        Window  ret;
        Atom    id_atom;

        id_atom = XInternAtom(dpy, id, False);

        /* lock in a critical section so I can get a    */
        /* definite YES/NO...                           */
        XGrabServer(dpy);
        if ((ret = XGetSelectionOwner(dpy, id_atom)) == None) {
                XSetSelectionOwner(dpy, id_atom, id_owner, CurrentTime);
                ret = None;
        }
        XUngrabServer(dpy);
        XFlush(dpy);

        return(ret);
} /* end of DtSetAppId */

/*******************************************************************************
 *  FUNCTION:
 *	char	*getStr (char *msgId)
 *  DESCRIPTION:
 *	Get a localized string from a message file.  msgIds contain the filename
 *	(limited to 14 characters), a ':' character, a msgNumber which is an index
 *	of the string in the database, and the string itself. If strchr does not
 *  find an FS_CHAR, we assume the msgId string does not come from a message
 *  file, and we simply return the msgId string.
 *  RETURN:
 *	A pointer to just the string portion of the message file entry.
 ******************************************************************************/
char *Miscellaneous::GetStr (char *msgId)
{
    char	*sep;
    char	*str = msgId;

    sep = (char *)strchr (msgId, FS_CHR);
	if (sep) {
    	*sep = '\0';
    	str = (char *)gettxt (msgId, sep + 1);
    	*sep = FS_CHR;
	}
    return (str);
}	/*	End  GetStr () */

/*****************************************************************************
 *  FUNCTION:
 *	Cursor SetCursor (Widget, unsigned int)
 *  DESCRIPTION:
 *  Set the cursor to the type that is passed
 *  RETURN:
 *  The cursor that has been defined.	
 ******************************************************************************/
Cursor Miscellaneous::SetCursor(Widget shell, unsigned int cursortype)
{
	Cursor 		cursor;

	Display *display = XtDisplay (shell);
	cursor = XCreateFontCursor(XtDisplay (shell), cursortype);

	XDefineCursor(display, XtWindow(shell), cursor);
  	XSync (display, 0);
	return cursor;
}

/*****************************************************************************
 *  FUNCTION:
 *	void ReSetCursor (Widget, Cursor)
 *  DESCRIPTION:
 *  ReSet the cursor back to previous and free the cursor that is passed
 *  RETURN:
 *  nothing 
 *****************************************************************************/
void Miscellaneous::ResetCursor(Widget shell, Cursor cursor)
{
	Display *display = XtDisplay (shell);

	XUndefineCursor(display, XtWindow(shell));
  	XSync (display, 0);
	XFreeCursor (display, cursor);
}

/**************************************************************
 *  FUNCTION:
 *	void CancelDialogCB(Widget, Dialog *, XtPointer)
 *  DESCRIPTION:
 *  Generic cancel callback for error dialogs that have been
 *  popped up throughout the application. Delete the dialog object
 *  to get rid of the error popup.
 *  RETURN:
 *  nothing 
 **************************************************************/
void
Miscellaneous::CancelDialogCB (Widget, XtPointer clientdata, XtPointer)
{
	Dialog	*obj = (Dialog *) clientdata;

	delete obj;
}

/**************************************************************
 *  FUNCTION:
 *	Dialog *SetupDialog(Widget, char *, char *, int)
 *  DESCRIPTION:
 *  Common dialog setup routine for the whole application. 
 *  RETURN:
 *  Pointer to the Dialog object.  
 **************************************************************/
Dialog *
Miscellaneous::SetupDialog(Widget parent, char *message, char *title, 
                           int dialogType)
{
		Dialog		*popdialog;

		popdialog = new Dialog ("Popupdialog", dialogType);
		popdialog->postDialog(parent, title, message);
		popdialog->SetModality (XmDIALOG_FULL_APPLICATION_MODAL);
		return popdialog;
}

/*****************************************************************************
 *  FUNCTION:
 *	Boolean IsUserPrivileged (Display *display)
 *  DESCRIPTION:
 *  Validate owner privileges by using tfadmin
 *  Duplicate of dtamlib/owner.c bu with the leading undescore removed.
 *  I decided it is better to duplicate here rather than link in another
 *  static library for this one routine.
 *  RETURN:
 *	True on succesful exit status of tfadmin
 *	False on unsuccesful exit status of tfadmin
 *****************************************************************************/
Boolean
Miscellaneous::IsUserPrivileged (Display *display)
{
	String	execName, className;
	char    buf[256];

	XtGetApplicationNameAndClass (display, &execName, &className);

	sprintf(buf, "/sbin/tfadmin -t %s 2>/dev/null", execName);
	if (system (buf) == 0)
		return True;
	else
		return False;

}	/*  End  IsUserPrivileged () */

/*****************************************************************************
 *  FUNCTION:
 *	Boolean IsApplicationRunning(Widget w, Display *display, char *appName)
 *	Pass it the toplevel widget, display variable and app name
 *  DESCRIPTION:
 *  See if the program is already up and running. If it is set focus to 
 * 	existing app and exit.
 *  RETURN:
 *	True on successful = app is up. Then call exit () function in main pgm.
 *	False on unsuccessful = app is not yet up, so continue 
 *****************************************************************************/
Boolean
Miscellaneous::IsApplicationRunning (Widget topLevel, Display *display, 
                                     char *appName)
{
	Window			window;

	window  = DtSetAppId (display , XtWindow (topLevel), appName);
    if (window != None) { 
		XMapWindow (display, window);
        XRaiseWindow (display, window);
        XSetInputFocus (display, window, RevertToNone, CurrentTime);
        XFlush (display);
        return True;
    }
	return False;
}	/*  End  IsApplicationRunning () */

/*******************************************************************************
 *  FUNCTION:
 *  int FindMaxLength (char *...)
 *  DESCRIPTION:
 *  Find max length of several strings that were passed. Use variable arguments.
 *  RETURN:
 *  Length of the largest string.
 ******************************************************************************/
int
Miscellaneous::FindMaxLength(char *fmt ... )
{
	va_list		ap;
	char		*tmp;
	int			length, maxlen;

	length = maxlen = 0;
	va_start (ap, fmt);

	maxlen = length = strlen (fmt);
	while ((tmp = va_arg(ap, char *))) {
		length = strlen (tmp);
		if (length > maxlen)
			maxlen = length;
	}

	va_end(ap);
	return maxlen;	
}

/****************************************************************************** 
 *  FUNCTION:
 *  int FindMaxWidth (char *...)
 *  DESCRIPTION:
 *  Find max width of several strings that were passed. Use variable arguments.
 *  RETURN:
 *  Width of the largest string.
 ******************************************************************************/
int
Miscellaneous::FindMaxWidth( XmFontList fontList, char *fmt ...)
{
	Dimension 	width, maxWidth;
	va_list		ap;
	XmString 	xmstr;
	char		*tmp;

	maxWidth = width = 0;

	va_start (ap, fmt);

	/* Find the maximum width of all the label strings so that 
	 * we can align them properly, proportional fonts make the
	 * strlen calculations only partially correct.
	 */
	tmp = (char *)fmt;
	while (tmp != NULL) {
		xmstr = XmStringCreateLocalized( (char *)tmp);
		width = XmStringWidth(fontList, xmstr);
		maxWidth = (maxWidth < width) ? width : maxWidth;
		XmStringFree (xmstr);
		tmp = va_arg(ap, char *);
	}
	va_end(ap);

	return maxWidth;
}

/****************************************************************************** 
 *  FUNCTION:
 *  void MakeStringsEqual(XmFontList, XmString *, int *, Boolean, char *...)
 *  DESCRIPTION:
 *  Make all strings of equal width based on the max width that was calculated
 *  earlier.  Another method  with variable arguments.
 *  RETURN:
 *  nothing.
 ******************************************************************************/
void
Miscellaneous::MakeStringsEqual(XmFontList fontList, XmString *xmstr, 
								int *maxWidth, Boolean flag,char *fmt ...)
{
	va_list		ap;
	Dimension	width;
	int			i = 0;
	char		*tmp;

	va_start (ap, fmt);

	/* Create the label string taking into account proportional width fonts.
	 */
	tmp = (char *)fmt;
	while (tmp != NULL) {
		xmstr[i] = XmStringCreateLocalized ((String)tmp);
		width = XmStringWidth(fontList, xmstr[i]);
		while (width < *maxWidth) {
			XmString spaceStr = XmStringCreateLocalized((String)" ");
			xmstr[i] = XmStringConcat (
								flag == prepend ? spaceStr : xmstr[i], 	
								flag == prepend ? xmstr[i] : spaceStr
							);
			width = XmStringWidth(fontList, xmstr[i]);
		}
		tmp = va_arg(ap, char *);
	}

	va_end(ap);
}

/****************************************************************************** 
 *  FUNCTION:
 *  int FindMaxWidth(XmFontList, const char **, int) 
 *  DESCRIPTION:
 *  Find max width of several strings that were passed. Use variable arguments.
 *  Without variable arguments.
 *  RETURN:
 *  Width of largest string.
 ******************************************************************************/
int
Miscellaneous::FindMaxWidth(XmFontList fontList, const char **labels, int total)
{
	Dimension 	width, maxWidth;
	XmString 	xmstr;
	int			i;

	maxWidth = width = 0;

	/* Find the maximum width of all the label strings so that 
	 * we can align them properly, proportional fonts make the
	 * strlen calculations only partially correct.
	 */
	for (i = 0; i < total; i++) {
		xmstr = XmStringCreateLocalized((char *)labels[i]);
		width = XmStringWidth(fontList, xmstr);
		maxWidth = (maxWidth < width) ? width : maxWidth;
		XmStringFree (xmstr);		
	}

	return maxWidth;
}

/****************************************************************************** 
 *  FUNCTION:
 *  void MakeStringsEqual(XmFontList, XmString *, int, const char **, Boolean)
 *  DESCRIPTION:
 *  Make all strings of equal width based on the max width that was calculated
 *  earlier. 
 *  RETURN:
 *  nothing.
 ******************************************************************************/
void
Miscellaneous::MakeStringsEqual(XmFontList fontList, XmString *xmstr, int total,
								const char **fmt, Boolean flag)
{
	int			i;
	int			maxWidth = FindMaxWidth (fontList, fmt, total);
	Dimension	width;

#ifdef DEBUG
cout << "Max width " << maxWidth << endl;
#endif

	for (i = 0; i < total; i++) {

		/* Get the string and the width of the string 	
		 */
		xmstr[i] = XmStringCreateLocalized ((String)fmt[i]);
		width = XmStringWidth(fontList, xmstr[i]);

#ifdef DEBUG
cout << "before loop " << width << endl;
#endif

		/* While the width is < maxWidth keep concatenating space strings
		 * in front of the original string, if flag is prepend else if flag
		 * is append then append space at the end of original string.
		 */
		while (width < maxWidth) {
			XmString spaceStr = XmStringCreateLocalized((String)" ");
			XmString tmp = XmStringConcat (
								flag == prepend ? spaceStr : xmstr[i], 	
								flag == prepend ? xmstr[i] : spaceStr
							);
			width = XmStringWidth(fontList, tmp);
			if (width <= maxWidth)
				xmstr[i] = tmp;
			else
				XmStringFree(tmp);
			XmStringFree(spaceStr);

#ifdef DEBUG
cout << "in loop " << width << endl;
#endif
		}

#ifdef DEBUG
cout << "out of loop " << width << endl;
#endif
	}
}

/*******************************************************************************
 *  FUNCTION:
 *  int FindMaxLength(const char **, int) 
 *  DESCRIPTION:
 *  Find max length of several strings that were passed. Use variable arguments.
 *  Without variable arguments.
 *  RETURN:
 *  Length of largest string.
 ******************************************************************************/
int
Miscellaneous::FindMaxLength(const char **labels, int total)
{
	int			length, maxlen, i;

	length = maxlen = 0;

	maxlen = length = strlen (labels[0]);
	for (i = 0; i < total; i++) {
		length = strlen (labels[i]);
		if (length > maxlen)
			maxlen = length;
	}

	return maxlen;	
}

#define 	DTMSG	"/usr/X/desktop/rft/dtmsg -display "

/*******************************************************************************
 *  FUNCTION:
 *  void DisplayDesktopMessage(const char **, int) 
 *  DESCRIPTION:
 *  Display desktop message using the DTMSG command 
 *  RETURN:
 *  nothing.
 ******************************************************************************/
void
Miscellaneous::DisplayDesktopMessage(Display *display, char *msg)
{
	char			buf[BUFSIZ];

	sprintf (buf, "%s %s %s%s%s", DTMSG, DisplayString(display), "\"", msg, 
             "\"");
	system (buf);
}

/****************************************************************************** 
 *  FUNCTION:
 *  WidgetList ParseWidgetTree(Widget, int &) 
 *  DESCRIPTION:
 *  Recursive function to parse thru the entire widget hierarchy and returns the
 *  total number of widgets and returns the widget list filled with widgets. 
 *  RETURN:
 *  The widget tree.
 ******************************************************************************/
WidgetList
Miscellaneous::ParseWidgetTree(Widget w, int &total)
{
	Widget				*children = 0;
	int					i = 0, numOfChildren = 0;
	static WidgetList 	widgetlist = 0;		

	/* Get all the children of the passed in parent widget here
	 */
	XtVaGetValues (w, XmNchildren, &children, XmNnumChildren, &numOfChildren,
					NULL);

	/* If there are no children return; 
	 */
	if (!numOfChildren)
		return widgetlist;

	/* Get all children of each widget. Then for each individual child,
	 * search all its, respective children till the end of leaf, recursively.
	 */	
	for (i = 0; i < numOfChildren; i++) {

		Miscellaneous::ParseWidgetTree(children[i], total);	//recursively called

		/* If there was already a created list then new more space
		 * else create space for  a new list of one widget
		 */
		if (total) {
			Widget		*tmplist  = new Widget[total+1];

			for (int index = 0; index < total; index++) {
				tmplist[index] = widgetlist[index]; 
			}
			delete [] widgetlist;
			widgetlist = tmplist;
		}
		else {	/* If routine is re-entered several times, clean up space */
			if (widgetlist)
				delete [] widgetlist;
			widgetlist = 0;
			widgetlist = new Widget[total+1];
		}
		
		widgetlist[total] = children[i];
		total++;
	}

	return widgetlist;
}

/*******************************************************************************
 *  FUNCTION:
 *  void TurnOffSashTraversal(Widget) 
 *  DESCRIPTION:
 *  Turn of traversal for the sashes on all the children of the paned window .
 *  RETURN:
 *  nothing
 ******************************************************************************/
void
Miscellaneous::TurnOffSashTraversal (Widget pane)
{
	int			numOfChildren;
	Widget		*children;

	/* Get all sashes for the paned window and turn off the keyboard traversal
	 * for each one of them.
	 */
	XtVaGetValues(pane, XmNchildren, &children, XmNnumChildren, &numOfChildren,
				  NULL);

	if (!numOfChildren)
		return;

	while (numOfChildren -- > 0)  {
		if (XmIsSash (children[numOfChildren]))
			XtVaSetValues (children[numOfChildren],XmNtraversalOn,False,NULL);
	}
}
