#ident	"@(#)menubarMenu.C	1.2"
/*----------------------------------------------------------------------------
 *
 */
#include <iostream.h>

#include <Xm/CascadeB.h>
#include <Xm/RowColumn.h>

#include "menubarMenu.h"
#include "menubar.h"
#include "menubarItem.h"
#include "action.h"

/*----------------------------------------------------------------------------
 *
 */
MenubarMenu::MenubarMenu (Menubar*		parent,
						  const char*	name,
						  Arg*			args,
						  int			count)
{
	d_widget = 0;
	d_button = 0;

	if (parent) {
		if (d_widget = XmCreatePulldownMenu (parent->widget (),
											 (char*)name,
											 0,
											 0)) {

			if (d_button = XmCreateCascadeButton (parent->widget (),
												  "buttonWidget",
												  args,
												  count)) {
				XtVaSetValues (d_button, XmNsubMenuId, d_widget, 0);
				XtManageChild (d_button);
			}
		}
	}
}

MenubarMenu::MenubarMenu (MenubarMenu*	parent,
						  const char*	name,
						  Arg*			args,
						  int			count)
{
	d_widget = 0;
	d_button = 0;

	if (parent) {
		if (d_widget = XmCreatePulldownMenu (parent->widget (),
											 (char*)name,
											 0,
											 0)) {

			if (d_button = XmCreateCascadeButton (parent->widget (),
												  "buttonWidget",
												  args,
												  count)) {
				XtVaSetValues (d_button, XmNsubMenuId, d_widget, 0);
				XtManageChild (d_button);
			}
		}
	}
}

MenubarMenu::~MenubarMenu ()
{
	if (d_widget) {
		XtDestroyWidget (d_widget);
	}
	if (d_button) {
		XtDestroyWidget (d_button);
	}
}

/*----------------------------------------------------------------------------
 *	Public Interface Methods
 */
void
MenubarMenu::manage ()
{
	if (d_widget) {
		XtManageChild (d_widget);
	}
}

void
MenubarMenu::unmanage ()
{
	if (d_widget) {
		XtUnmanageChild (d_widget);
	}
}

void
MenubarMenu::activate ()
{
	if (d_widget) {
		XtVaSetValues (d_widget, XmNsensitive, True, 0);
	}
}

void
MenubarMenu::deactivate ()
{
	if (d_widget) {
		XtVaSetValues (d_widget, XmNsensitive, False, 0);
	}
}

/*----------------------------------------------------------------------------
 *	Public Data Access Methods
 */
Widget
MenubarMenu::widget ()
{
	return (d_widget);
}

Widget
MenubarMenu::button ()
{
	return (d_button);
}

