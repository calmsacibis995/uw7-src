#ident	"@(#)menubarMenu.h	1.2"
/*----------------------------------------------------------------------------
 *
 */
#ifndef MENUBARMENU_H
#define MENUBARMENU_H

#include <Xm/Xm.h>

/*----------------------------------------------------------------------------
 *	MenubarMenu is a container object for MenubarItems.
 */
class Menubar;
class MenubarItem;

class MenubarMenu {
public:								// Constructors/Destructors
								MenubarMenu (Menubar*		parent,
											 const char*	name = "menu",
											 Arg*			args = 0,
											 int			count = 0);
								MenubarMenu (MenubarMenu*	parent,
											 const char*	name = "menu",
											 Arg*			args = 0,
											 int			count = 0);
								~MenubarMenu ();

private:							// Private Data
	Widget						d_widget;
	Widget						d_button;

public:								// Public Interface Methods
	void						manage ();
	void						unmanage ();

	void						activate ();
	void						deactivate ();

public:								// Public Data Access Methods
	Widget						widget ();
	Widget						button ();
};

#endif	// MENUBARMENU_H
