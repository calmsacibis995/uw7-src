#ident	"@(#)menubar.h	1.2"
/*----------------------------------------------------------------------------
 *
 */
#ifndef MENUBAR_H
#define MENUBAR_H

#include <Xm/Xm.h>

/*----------------------------------------------------------------------------
 *	Menubar is a wrapper object for a menubar Widget.
 */
class MenubarMenu;

class Menubar {
public:								// Constructors/Destructors
								Menubar (Widget			parent,
										 const char*	name = "menubar",
										 Arg*			args = 0,
										 int			count = 0);
								~Menubar ();

private:							// Private Data
	Widget						d_widget;

public:								// Public Interface Methods
	void						helpMenu (MenubarMenu* menu);

public:								// Public Data Access Methods
	Widget						widget ();
};

#endif	// MENUBAR_H
