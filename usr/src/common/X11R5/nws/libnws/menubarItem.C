#ident	"@(#)menubarItem.C	1.2"
/*----------------------------------------------------------------------------
 *
 */
#include <iostream.h>

#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>

#include "menubarItem.h"
#include "menubarMenu.h"
#include "action.h"

/*----------------------------------------------------------------------------
 *	Constructors/Destructors
 */
MenubarItem::MenubarItem (MenubarMenu*	parent,
						  Action*		action,
						  const char*	name,
						  Arg*			args,
						  int			count)
{
	d_widget = 0;

	if (parent) {
		if (d_widget = XmCreatePushButton (parent->widget (),
										   (char*)name,
										   args,
										   count)) {
			manage ();

			if (action && action->callback ()) {
				XtAddCallback (d_widget,
							   XmNactivateCallback,
							   action->callback (),
							   action->data ());
			}
		}
	}
}

MenubarItem::MenubarItem (MenubarMenu*	parent,
						  Action*		arm,
						  Arg*			args,
						  int			count)
{
	d_widget = 0;

	if (parent) {
		if (d_widget = XmCreateToggleButton (parent->widget (),
											 0,
											 args,
											 count)) {
			manage ();

			if (arm && arm->callback ()) {
				XtAddCallback (d_widget,
							   XmNvalueChangedCallback,
							   arm->callback (),
							   arm->data ());
			}
		}
	}
}

MenubarItem::MenubarItem (MenubarMenu*	parent,
						  const char*	name,
						  Arg*			args,
						  int			count)
{
	d_widget = 0;

	if (parent) {
		if (d_widget = XmCreateSeparator (parent->widget (),
										  (char*)name,
										  args,
										  count)) {
			manage ();
		}
	}
}

MenubarItem::~MenubarItem ()
{
	if (d_widget) {
		XtDestroyWidget (d_widget);
	}
}

/*----------------------------------------------------------------------------
 *	Public Interface Methods
 */
void
MenubarItem::manage ()
{
	if (d_widget) {
		XtManageChild (d_widget);
	}
}

void
MenubarItem::unmanage ()
{
	if (d_widget) {
		XtUnmanageChild (d_widget);
	}
}

void
MenubarItem::activate ()
{
	if (d_widget) {
		XtVaSetValues (d_widget, XmNsensitive, True, 0);
	}
}

void
MenubarItem::deactivate ()
{
	if (d_widget) {
		XtVaSetValues (d_widget, XmNsensitive, False, 0);
	}
}

/*----------------------------------------------------------------------------
 *	Public Data Access Methods
 */
Widget
MenubarItem::widget ()
{
	return (d_widget);
}

