#ident	"@(#)dialogObj.h	1.2"
////////////////////////////////////////////////////////
// DialogObject.h:
/////////////////////////////////////////////////////////
#ifndef DIALOGOBJ_H
#define DIALOGOBJ_H

#include <Xm/Xm.h>
#include "linkObj.h"

class DialogObject  : public LinkObject {
    
public:
   						DialogObject();	/* CTOR for DialogObject */
   	virtual 			~DialogObject(); 	/* DTOR for DialogObject */

private:
	char				*_helpfile, *_helptitle, *_helpsection;

protected:									/* Inherited protected variables. */
	Widget				_popupDialog;
	virtual enum 		{OK, CANCEL, HELP, TOTALACTIONS};

protected: 									/* GUI inheritable functions */
											/* Callbacks */
	static void			okCB (Widget, XtPointer, XtPointer);
	static void			cancelCB (Widget, XtPointer, XtPointer);
	static void			helpCB (Widget, XtPointer, XtPointer);
											/* GUI Backend methods */
	virtual void		ok();
	virtual void		cancel();
	virtual void 		help();
	virtual void		manage();
											/* GUI frontend methods */
	virtual void		postDialog(Widget, char *);
	virtual void		DisableMwmFunctions();
	virtual void		registerHelpInfo(char *, char *, char *);

public:			/* Public member methods accessed from DialogManager Object */
	void				RaiseObject();
};
#endif
