#ident	"@(#)application.h	1.2"

/******************************************************************************
 *			Application class - header file definition.
 ******************************************************************************/
#ifndef APPLICATION_H
#define APPLICATION_H

#include <Xm/Xm.h>

class Application {
 
public:							/* Constructors/Destructors */
								Application ();
								~Application ();

protected:						/* Protected Data */
	Display				*_display;
	Screen				*_screen;
	XtAppContext		_appContext;
	Widget				_appShell;
	char				*_appname, *_apptitle;
	int					_scrnum;
	XmFontList			_fontList;

protected:						/* Protected methods. */

	virtual Boolean		OpenDisplay(char *, char *, int, char **);
	virtual Boolean		CreateShell(char *, char *, Boolean);
	virtual void		SetupVariables(char *, char *, char*);

public:							/* Public Interface Methods */

	XtAppContext		GetAppContext() const {return _appContext;}
	XmFontList			GetAppFontList() const { return _fontList; }
	int					GetScreenNumber() const {return _scrnum;}
	Widget				GetAppShell () const { return _appShell;}
	char				*GetAppName() const { return _appname; }
	char				*GetAppTitle() const { return _apptitle; }
	Screen				*GetScreen() const {return _screen;}
	Display 			*GetDisplay() const {return _display;}

	void				Realize();
};

#endif	/* APPLICATION_H */
