#ident	"@(#)action.C	1.2"
/*----------------------------------------------------------------------------
 *
 */
#include "action.h"

/*----------------------------------------------------------------------------
 *	Constructors/Destructors
 */
Action::Action (XtCallbackProc callback, XtPointer data, XmString helpStr)
{
	d_callback = callback;
	d_data = data;

	d_helpStr = 0;
	if (helpStr) {
		d_helpStr = XmStringCopy (helpStr);
	}
}

Action::~Action ()
{
	if (d_helpStr) {
		XmStringFree (d_helpStr);
	}
}

/*----------------------------------------------------------------------------
 *	Public data access methods.
 */
void
Action::callback (XtCallbackProc callback)
{
	d_callback = callback;
}

XtCallbackProc
Action::callback ()
{
	return (d_callback);
}

void
Action::data (XtPointer data)
{
	d_data = data;
}

XtPointer
Action::data ()
{
	return (d_data);
}

void
Action::helpString (XmString helpStr)
{
	if (d_helpStr) {
		XmStringFree (d_helpStr);
		d_helpStr = 0;
	}
	if (helpStr) {
		d_helpStr = XmStringCopy (helpStr);
	}
}

XmString
Action::helpString ()
{
	return (d_helpStr);
}

