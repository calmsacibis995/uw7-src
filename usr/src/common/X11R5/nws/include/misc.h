#ident	"@(#)misc.h	1.2"
/*****************************************************************************
 * Miscellaneous object = Miscellaneous methods. Generic usage by applications.
 *****************************************************************************/
#ifndef MISCELLANEOUS_H
#define MISCELLANEOUS_H

#include <Xm/Xm.h>

class Dialog;

class Miscellaneous {
    
  private:  

						/* Static methods for public invokation */
  public:
	enum Space_flag { prepend, append };

						/* Set and unset watch or any type of cursor */
	static Cursor 		SetCursor (Widget shell, unsigned int cursortype); 
	static void			ResetCursor (Widget shell, Cursor cursor); 
	static WidgetList	ParseWidgetTree(Widget parent, int &total);
	static void			TurnOffSashTraversal (Widget panedwindow); 

						/* Generic message dialog routine and callback */
	static Dialog		*SetupDialog(Widget parent, char *message, char *title,
                                     int dialogType);
	static void			CancelDialogCB (Widget , XtPointer clientdata, 
                                        XtPointer calldata); 

						/* Variable string methods */
	static int			FindMaxLength(char * ...);
	static int			FindMaxWidth(XmFontList, char * ...);
	static void			MakeStringsEqual(XmFontList, XmString *, int *width, 											Boolean AppendOrPrependSpace,
										char * ...);

						/* Fixed string methods */
	static int			FindMaxLength(const char **labels, int total);
	static int			FindMaxWidth(XmFontList,const char **labels, int total);
	static void			MakeStringsEqual(XmFontList, XmString *, int total, 
										const char **labelfields,
										Boolean AppendOrPrependSpace);

						/* Desktop related functions */
	static char 		*GetStr (char *msgId); 
	static Boolean		IsUserPrivileged(Display *display);
	static Boolean		IsApplicationRunning(Widget topLevel, Display *display,
	                                         char *appName);
	static void			DisplayDesktopMessage(Display *display, char *message);
};

#endif /* MISCELLANEOUS_H */

