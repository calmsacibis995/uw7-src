#ident	"@(#)application.C	1.2"
/*----------------------------------------------------------------------------
 *		Application class - Generic Application object for each GUI program.
 *--------------------------------------------------------------------------*/
#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>

#include "application.h"

/**************************************************************
 *	Application Constructors/Destructors
 **************************************************************/
Application::Application ()
{
#ifdef DEBUG
cout << "ctor for Application" <<  endl;
#endif
	XtSetLanguageProc (NULL, NULL, NULL);

	XtToolkitInitialize ();
	_appContext = XtCreateApplicationContext ();
}

/**************************************************************
 * Dtor for Application
 **************************************************************/
Application::~Application ()
{
#ifdef DEBUG
cout << "dtor for Application" <<  endl;
#endif
	delete [] _appname;
	delete [] _apptitle;
	XtDestroyApplicationContext (_appContext);
}

/******************************************************************************
 * Setup variables for generic use by any application.  This should always be 
 * called after CreateApplicationShell. It depends on _appShell being there. 
 ******************************************************************************/
void
Application::SetupVariables(char *pgmname, char *title, char *icon)
{
	Pixmap 	iconpix = NULL;

	/* Set the program title and program name here.  Also set screen variable 
	 * and the screen number here
 	 */
	_appname = new char [strlen(pgmname) + 1];
	strcpy (_appname, pgmname);

	_apptitle = new char [strlen(title) + 1];
	strcpy (_apptitle, title);

	_screen = XtScreen (_appShell);
	_scrnum = DefaultScreen (_display);

	XtVaSetValues (_appShell, 
			XmNtitle, 	_apptitle,
			XmNiconPixmap,  iconpix,	
			0);

	/* Create fontList from toplevel widget
	 */
	XtVaGetValues (_appShell, XmNlabelFontList, &_fontList, NULL);
}

/******************************************************************************
 * Realise the top level widget  
 *****************************************************************************/
void
Application::Realize()
{
	/* Realize the Application Shell
	 */
	XtRealizeWidget (_appShell);
}


/**************************************************************
 * Open the display here.  Return True or False.
 **************************************************************/
Boolean
Application::OpenDisplay (char *appname, char *classname, int argc, char** argv)
{
	_display = XtOpenDisplay (_appContext, NULL, appname, classname, 0, 0, 
                              &argc, argv);

	if (!_display) 
		return (FALSE);

	return TRUE;
}

/*******************************************************************************
 * Create the Application Shell. Standard applicationShellWidgetClass. Can
 * be overriden by application specific derived object to create whatever is
 * appropriate for the application itself.
 *****************************************************************************/
Boolean
Application::CreateShell (char *appname, char *classname, Boolean MappedWhenMgd)
{
	if (_display)
		_appShell = XtVaAppCreateShell(appname, classname,  
                                       applicationShellWidgetClass, _display, 
										XtNmappedWhenManaged, 	MappedWhenMgd,
                                       NULL);
	else
		return False;
	return True;
}
