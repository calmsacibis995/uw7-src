#ident	"@(#)toolbarItem.h	1.2"
/*----------------------------------------------------------------------------
 *
 */
#ifndef TOOLBARITEM_H
#define TOOLBARITEM_H

#include <Xm/Xm.h>

typedef void					(*HelpCallback) (XtPointer	data,
												 XmString	helpStr);

/*----------------------------------------------------------------------------
 *	An object that maintains an interface to a pushbutton widget placed on
 *	a Toolbar.
 */
class Toolbar;
class Action;

class ToolbarItem {
public:								// Constructors/Destructors
								ToolbarItem (Toolbar*		parent,
											 Action*		action,
											 const char*	name,
											 const XmString	labelString);
								ToolbarItem (Toolbar*		parent,
											 Action*		action,
											 const char*	name,
											 const char*	pixmapName);
								~ToolbarItem ();

private:							// Private Data
	Widget						d_widget;
	Display*					d_display;
	Screen*						d_screen;
	Toolbar*					d_parent;
	Action*						d_action;

	Dimension					d_width;
	Dimension					d_height;

	XmString					d_helpString;			// Override default

	XtPointer					d_helpData;
	HelpCallback				d_helpCallback;

private:							// Constructor Helper Methods
	void						initialize (Toolbar*	parent,
											Action*		action,
											const char*	name);

private:							// Private Methods
	void						createPixmap (const char* pixmapName);
	static void					helpCallback (Widget,
											  XtPointer	data,
											  XEvent*	event,
											  Boolean*);
	void						help (int type);

public:								// Public Interface Methods
	void						helpCallback (HelpCallback	helpCallback,
											  XtPointer		data);

	void						label (const XmString label);
	void						pixmap (const char* pixmapName);
	void						helpString (const XmString str);

	void						position (int x, int y);

	void						manage ();
	void						unmanage ();

	void						activate ();
	void						deactivate ();

public:								// Data Access Methods
	Widget						widget ();

	Dimension					width ();
	Dimension					height ();
};

#endif	// TOOLBARITEM_H
