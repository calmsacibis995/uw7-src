#ident	"@(#)menubar.C	1.2"
/*----------------------------------------------------------------------------
 *
 */
#include <iostream.h>

#include <Xm/RowColumn.h>

#include "menubar.h"
#include "menubarMenu.h"

/*----------------------------------------------------------------------------
 *	Constructors/Destructors
 */
Menubar::Menubar (Widget parent, const char* name, Arg* args, int count)
{
	d_widget = 0;

	if (parent) {
		if (d_widget = XmCreateMenuBar (parent, (char*)name, args, count)) {
			XtManageChild (d_widget);
		}
	}
}

Menubar::~Menubar ()
{
	if (d_widget) {
		XtDestroyWidget (d_widget);
	}
}

/*----------------------------------------------------------------------------
 *	Public Interface Methods
 */
void
Menubar::helpMenu (MenubarMenu* menu)
{
	if (d_widget && menu) {
		XtVaSetValues (d_widget, XmNmenuHelpWidget, menu->button (), 0);
	}
}

/*----------------------------------------------------------------------------
 *	Public Data Access Methods
 */
Widget
Menubar::widget ()
{
	return (d_widget);
}

