#ident	"@(#)action.h	1.2"
/*----------------------------------------------------------------------------
 *
 */
#ifndef ACTION_H
#define ACTION_H

#include <Xm/Xm.h>

/*----------------------------------------------------------------------------
 *	Action is a low level object that maintains a callback, data, and a
 *	mini-help string for an application command/action/function.  Action
 *	doesn't contain any widgets and can be used by multiple objects to
 *	access a single application command.
 */
class Action {
public:								// Constructors/Destructors
								Action (XtCallbackProc	callback,
										XtPointer		data = 0,
										XmString		helpStr = 0);
								~Action ();

private:							// Private Data
	XtPointer					d_data;					// Callback data
	XtCallbackProc				d_callback;				// Command callback
	XmString					d_helpStr;				// Mini-help string

public:								// Public Data Access Methods
	void						callback (XtCallbackProc callback);
	XtCallbackProc				callback ();
	void						data (XtPointer data);
	XtPointer					data ();
	void						helpString (XmString helpStr);
	XmString					helpString ();
};

#endif	// ACTION_H
