#ident	"@(#)debugger:libmotif/common/Stext.C	1.1"

#include "Component.h"
#include "Stext.h"

#include <Xm/Label.h>

Simple_text::Simple_text(Component *p, const char *s, Boolean resize, Help_id h)
	: COMPONENT(p, s, 0, h)
{
	XmString label_string = XmStringCreateLocalized((String)label);
	widget = XtVaCreateManagedWidget("simple text", xmLabelWidgetClass,
		parent->get_widget(),
		XmNlabelString, label_string,
		//XtNgravity, WestGravity,
		XmNrecomputeSize, resize,
		XmNuserData, this,
		0);
	XmStringFree(label_string);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Simple_text::~Simple_text()
{
}


// change the text displayed
void
Simple_text::set_text(const char *s)
{
	if (!s)
		s = " ";
	XmString string = XmStringCreateLocalized((String)s);
	XtVaSetValues(widget, XmNlabelString, string, 0);
	XmStringFree(string);
}
