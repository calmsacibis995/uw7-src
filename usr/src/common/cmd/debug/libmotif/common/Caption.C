#ident	"@(#)debugger:libmotif/common/Caption.C	1.3"

#include "UI.h"
#include "Component.h"
#include "Caption.h"
#include "gui_label.h"
#include "Label.h"
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <stdio.h>

// Captions have a label associated with a child component, which can be
// positioned one of three ways:
//	CAP_TOP_CENTER
//	CAP_TOP_LEFT
//	CAP_LEFT (always assume centered wrt height of component)

Caption::Caption(Component *p, LabelId lid, Caption_position pos, Help_id h)
	: Component(p, 0, 0, h)
{
	
	Arg	args[7];
	int	n = 0;
		
	label = labeltab.get_label(lid);
	if (!label || !*label)
		// at least one blank or we can't expand label later
		label = " ";
	child = 0;
	position = pos;

	widget = XtVaCreateWidget("caption", 
		xmFormWidgetClass, parent->get_widget(), 
		XmNuserData, this,
		XmNshadowThickness, 0,
		0);

	XmString label_string = XmStringCreateLocalized((String)label);

	XtSetArg(args[n], XmNlabelString, label_string); n++;
	XtSetArg(args[n], XmNuserData, this); n++;
	XtSetArg(args[n], XmNrecomputeSize, TRUE); n++;
	if (position == CAP_TOP_CENTER)
	{
		XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++;
		XtSetArg(args[n], XmNresizable, TRUE); n++;
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
		XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	}
	else
		XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;

	caption = XtCreateManagedWidget("caption_label",
		xmLabelWidgetClass, widget, args, n);

	XmStringFree(label_string);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Caption::~Caption()
{
	delete child;
}

void
Caption::add_component(Component *c, Boolean resizable)
{
	Arg	args[6];
	int	n = 0;

	if (child)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	child = c;

	Widget child_widget = child->get_widget();

	if (position == CAP_TOP_CENTER || position == CAP_TOP_LEFT)
	{
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
		XtSetArg(args[n], XmNtopWidget, caption); n++;
		XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
		if (resizable)
		{
			XtSetArg(args[n], XmNresizable, TRUE); n++;
			XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
		}
	}
	else
	{
		// position == CAP_LEFT
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
		XtSetArg(args[n], XmNleftWidget, caption); n++;
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
		XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
		if (resizable)
		{
			XtSetArg(args[n], XmNresizable, TRUE); n++;
			XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
		}
		// align caption
		align_y(child_widget);
	}
	XtSetValues(child_widget, args, n);
	// assumes all descendants created and managed
	XtManageChild(widget);
}

void
Caption::remove_child()
{
	XtDestroyWidget(child->get_widget());
	child = 0;
}

// align the caption along the y-axis wrt height of 'w'
// NOTE: this doesn't work if the child is an unrealized manager (e.g. form)
//       whose height is unknown until realization time
void
Caption::align_y(Widget w)
{
	int cap_ht = 0, child_ht = 0;
	int fract_base = 0;

	XtVaGetValues(widget, XmNfractionBase, &fract_base, 0);
	XtVaGetValues(w, XmNheight, &child_ht, 0);
	XtVaGetValues(caption, XmNheight, &cap_ht, 0);
#if DEBUG > 1
fprintf(stderr,"Caption: capht %d, childht %d\n",
		cap_ht, child_ht);
#endif
	if (child_ht > cap_ht)
		XtVaSetValues(caption, 
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, ((child_ht-cap_ht)/2)*fract_base/child_ht,
			0);
}

// change the caption's text
void
Caption::set_label(const char *s)
{
	XmString label_string = XmStringCreateLocalized((String)s);
	XtVaSetValues(caption, XmNlabelString, label_string, 0);
	XmStringFree(label_string);
}

void
Caption::set_sensitive(Boolean s)
{
	XtVaSetValues(caption, XmNsensitive, s, 0);
}
