#ident	"@(#)dialogObj.C	1.2"
/******************************************************************************
 * Dialog Object = 	Generic DialogObject class that provides a template for	a
 *					custom dialog that can be used by to inherit a user object.
 *****************************************************************************/
#include <iostream.h>

#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#include <Xm/MwmUtil.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>

#include "dialogObj.h"
#include "misc.h"
#include "Dialog.h"
#include "ActionButtons.h"

/******************************************************************************
 * Constructor for the DialogObject class.  New the LinkManager from which the
 * DialogObject is inherited.
 *****************************************************************************/
DialogObject::DialogObject() : LinkObject ()
{
#ifdef DEBUG
cout << "ctor for DialogObject" << endl;
#endif
	/* Initialize the variables
	 */
	_popupDialog = NULL;
	_helpfile = _helptitle = _helpsection = NULL;
}

/******************************************************************************
 * Destructor for the DialogObject class.
 *****************************************************************************/
DialogObject::~DialogObject()
{
#ifdef DEBUG
cout << "dtor for DialogObject" << endl;
#endif
	if (_helptitle)
		delete _helptitle;
	if (_helpsection)
		delete _helpsection;
	if (_helpfile)
		delete _helpfile;
}

/******************************************************************************
 * Common post dialog routine to post dialog box. Creates control/action areas.
 * Uses the xmDialogShellWidgetClass, as a standard and creates a form over it.
 * However, the derived object can override this virtual function with its own.
 *****************************************************************************/
void
DialogObject::postDialog (Widget parent, char *title)
{
}

/******************************************************************************
 *  Manage the form and the prompt dialog separately.  
 *****************************************************************************/
void
DialogObject::manage()
{
	/* Manage the form and the popup dialog.
	 */
	XtManageChild(_popupDialog);
}

/******************************************************************************
 * Disable Mwm functions in the window menu of the login panel. We need to
 * connect callbacks for the CANCEL button to the close window manager function.
 *****************************************************************************/
void
DialogObject::DisableMwmFunctions()
{
	long			mwmFuncs;
	Atom			WM_DELETE_WINDOW;
	Widget			shell;

	/* Get the topmost shell widget. This is done, so in case the derived
	 * object has another shell of its own.
	 */
	for (shell = _popupDialog; !XtIsShell(shell); shell = XtParent(shell));

	XtVaGetValues(shell, XmNmwmFunctions, &mwmFuncs, NULL);
	mwmFuncs &= ~( MWM_FUNC_RESIZE | MWM_FUNC_ALL | MWM_FUNC_MAXIMIZE | 
				MWM_FUNC_MINIMIZE);
	XtVaSetValues(shell, XmNmwmFunctions, mwmFuncs, NULL);

	/* Don't want mwm to handle Close so tell it to do nothing. Install 
	 * WM_DELETE_WINDOW atom. Add callback for WM_DELETE_WINDOW protocol. 
	 */
    XtVaSetValues(shell, XmNdeleteResponse, XmDO_NOTHING, NULL);
    WM_DELETE_WINDOW = XmInternAtom(XtDisplay(shell),"WM_DELETE_WINDOW",False);
	XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, cancelCB, this);
}

/******************************************************************************
 * OK callback needs to call ok function of derived object.
 *****************************************************************************/
void
DialogObject::okCB (Widget w, XtPointer clientdata, XtPointer)
{
#ifdef DEBUG
    cerr << "DialogObject:okCallback ()" << endl;
#endif
    DialogObject *obj = (DialogObject *)  clientdata;
    	
	obj->ok ();
}

/******************************************************************************
 * Virtual ok method that needs to be called by the user method if the 
 * user wants to popoff the dialog
 *****************************************************************************/
void
DialogObject::ok()
{
	cancel();
}

/******************************************************************************
 * Static Cancel callback function.
 *****************************************************************************/
void
DialogObject::cancelCB (Widget w, XtPointer clientdata, XtPointer)
{
#ifdef DEBUG
    cerr << "DialogObject::cancelCallback (" << clientdata<< ")" << endl;
#endif
	DialogObject 	*obj = (DialogObject *) clientdata;

  	obj->cancel ();
}

/******************************************************************************
 * Cancel function sets the deleteObject flag to True so that the object can
 * be deleted when a new object is asked to be instantiated.  The popup dialog
 * is also destroyed. This pops away the dialog. However the memory for the
 * DialogObject still exists and will be deleted the next time around. 
 *****************************************************************************/
void
DialogObject::cancel()
{
#ifdef DEBUG
cout << "cancelling the dialog and setting the flag here " << endl;
#endif
	_deleteObject = True;
	XtDestroyWidget(_popupDialog);
	_popupDialog = NULL;
}

/******************************************************************************
 * Static function for  Help callback.
 *****************************************************************************/
void
DialogObject::helpCB (Widget w, XtPointer client_data, XtPointer call_data ) 
{
}

/******************************************************************************
 * Create the HELP class for the DialogObject panel here. This displays help
 * when the Help buttons is pressed.
 *****************************************************************************/
void
DialogObject::help ()
{
}

void
DialogObject::registerHelpInfo(char *file, char *title, char *section)
{
	_helpfile = new char [strlen (file) + 1];
	strcpy (_helpfile, file);
	_helptitle = new char [strlen (title) + 1];
	strcpy (_helptitle, title);
	_helpsection = new char [strlen (section) + 1];
	strcpy (_helpsection, section);
}

/******************************************************************************
 * Raise the dialog object, if it already exists.  This check is done in the
 * DialogManager object which retains a linked list of the DialogObjects.
 *****************************************************************************/
void
DialogObject::RaiseObject ()
{
    if (_popupDialog) 
        XRaiseWindow (XtDisplay (_popupDialog), XtWindow (_popupDialog));
}
