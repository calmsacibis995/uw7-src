#ident	"@(#)menubarItem.h	1.2"
/*----------------------------------------------------------------------------
 *
 */
#ifndef MENUBARITEM_H
#define MENUBARITEM_H

#include <Xm/Xm.h>

/*----------------------------------------------------------------------------
 *	MenubarItem is a wrapper object for a pushbutton Widget in a pulldown menu
 *	on a menubar Widget.  MenubarItems cannot be shared by multiple pulldown
 *	objects because it contains a Widget which uses the pulldown menu Widget
 *	as its parent.
 */
class MenubarMenu;
class Action;

class MenubarItem {
public:								// Constructors/Destructors
								MenubarItem (MenubarMenu*	parent,
											 Action*		action,
											 const char*	name,
											 Arg*			args = 0,
											 int			count = 0);
								MenubarItem (MenubarMenu*	parent,
											 Action*		arm,
											 Arg*			args = 0,
											 int			count = 0);
								MenubarItem (MenubarMenu*	parent,
											 const char*	name = "separator",
											 Arg*			args = 0,
											 int			count = 0);
								~MenubarItem ();

private:							// Private Data
	Widget						d_widget;

public:								// Public Interface Methods
	void						manage ();
	void						unmanage ();

	void						activate ();
	void						deactivate ();

public:								// Public Data Access Methods
	Widget						widget ();
};

#endif	// MENUBARITEM_H
