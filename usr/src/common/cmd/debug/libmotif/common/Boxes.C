#ident	"@(#)debugger:libmotif/common/Boxes.C	1.5"

#include "UI.h"
#include "Component.h"
#include "Boxes.h"
#include <Xm/Form.h>
#include <Xm/SeparatoG.h>
#include <Xm/PanedW.h>

#define	BOX_PADDING 2

// Boxes do not need to save the creator since they have no callbacks
Box::Box(Component *p, const char *s, Orientation o, Box_type t, Help_id h) 
	: Component(p, s, 0, h)
{
	orientation = o;
	type = t;
}

Box::~Box()
{
	Component *ptr;

	ptr = (Component *)children.first();
	for (; ptr; ptr = (Component *)children.next())
		delete ptr;
}

// all children created and managed - manage parent widget
void
Box::update_complete()
{
	XtManageChild(widget);
}

// Components in a packed box do not change sizes when the
// box is resized.
Packed_box::Packed_box(Component *p, const char *s, Orientation o, Help_id h)
	: Box(p, s, o, Box_packed, h)
{
	widget = XtVaCreateWidget(label, xmFormWidgetClass,
		parent->get_widget(),
		XmNverticalSpacing, BOX_PADDING,
		XmNhorizontalSpacing, BOX_PADDING,
		0);

	if (help_msg)
		register_help(widget, 0, help_msg);
}

void
Packed_box::add_component(Component *p, Boolean)
{
	Component	*sib = (Component *)children.last();
	Widget		sw = sib ? sib->get_widget() : 0;
	Widget		w = p->get_widget();

	add_to_form(sw, w);
	children.add(p);
}

// separator can only be added to beginning of component
void
Packed_box::add_component_with_separator(Component *p)
{
	Component	*sib = (Component *)children.last();
	Widget		sw = sib ? sib->get_widget() : 0;
	Widget		sepw;

	sepw = XtVaCreateManagedWidget("separator", xmSeparatorGadgetClass, 
		widget,
		XmNorientation, orientation == OR_vertical ? 
				XmHORIZONTAL : XmVERTICAL,
		XmNseparatorType, XmSINGLE_LINE,
		0);
	add_to_form(sw, sepw);
	add_to_form(sepw, p->get_widget());
	children.add(p);
}

void
Packed_box::add_to_form(Widget sw, Widget w)
{
	Arg		args[4];
	int		n = 0;

	if (orientation == OR_horizontal)
	{
		// position to the right of the previous widget
		if (sw)
		{
			XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
			XtSetArg(args[n], XmNleftWidget, sw); ++n;
		}
		else
		{
			XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
		}
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
		XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
	}
	else
	{
		// position below the previous widget
		if (sw)
		{
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
			XtSetArg(args[n], XmNtopWidget, sw); ++n;
		}
		else
		{
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
		}
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
		XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
	}
	XtSetValues(w, args, n);
}

// Expansion box is like Packed box except one (and only one)
// component resizes when the box resizes in the major direction
// (orientation).
// Current implementation uses a form whose children are laid out as:
//           ---------------------------------------------
//           |   -----   -----   -----   -----   -----   |
//           |<--| f |<--| f |<--| e |-->| f |-->| f |-->|
//           |   -----   -----   -----   -----   -----   |
//           ---------------------------------------------
// where f is a fixed-size component and e is the elastic one.

Expansion_box::Expansion_box(Component *p, const char *s, Orientation o,
	Help_id h) : Box(p, s, o, Box_expansion, h)
{
	widget = XtVaCreateWidget(label, xmFormWidgetClass,
		parent->get_widget(),
		XmNverticalSpacing, BOX_PADDING,
		XmNhorizontalSpacing, BOX_PADDING,
		0);
	elastic_added = FALSE;

	if (help_msg)
		register_help(widget, 0, help_msg);
}

void
Expansion_box::add_component(Component *p, Boolean is_elastic)
{
	Component 	*sib = (Component *)children.last();
	Widget		sw = sib ? sib->get_widget() : 0;
	Widget		w = p->get_widget();

	if (is_elastic && (elastic_added == TRUE))
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	add_to_form(sw, w, is_elastic);
	children.add(p);
	if (is_elastic)
		elastic_added = TRUE;
}

// insert component p after component lp
// we assume not inserting an elastic component;
// each existing widget (except elastic) is attached
// to either previous or next, but not both
void
Expansion_box::insert_component(Component *p, Component *lp)
{
	Component	*prev, *next;
	if (!lp)
	{
		prev = 0;
		next = (Component *)children.first();
	}
	else
	{
		prev = (Component *)children.first();
		while(prev != lp)
			prev = (Component *)children.next();
		next = (Component *)children.next();
	}

	Widget		pw = prev ? prev->get_widget() : 0;
	Widget		w = p->get_widget();
	Widget		nw = next ? next->get_widget() : 0;

	if (next)
		children.insert(p);
	else
		children.add(p);
	
	Arg		args[5];
	int		n = 0;

	if (orientation == OR_horizontal)
	{
		unsigned char	pright = XmATTACH_NONE;
		unsigned char	nleft = XmATTACH_NONE;
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
		XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
		if (!pw)
		{
			// insert at far left
			// pright and nleft will be XmATTACH_NONE
			XtVaSetValues(nw,
				XmNleftAttachment, XmATTACH_WIDGET,
				XmNleftWidget, w,
				0);
			XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);
			++n;
		}
		else
			XtVaGetValues(pw,
				XmNrightAttachment, &pright,
				0);
		if (!nw)
		{
			// insert at far right
			XtVaSetValues(pw,
				XmNrightAttachment, XmATTACH_WIDGET,
				XmNrightWidget, w,
				0);
			if (elastic_added)
			{
				XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);
				++n;
			}
		}
		else
			XtVaGetValues(nw,
				XmNleftAttachment, &nleft,
				0);
		if (pright == XmATTACH_WIDGET)
		{
			// insert to right of elastic but not
			// at far right
			XtVaSetValues(pw,
				XmNrightWidget, w,
				0);
			XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); ++n;
			XtSetArg(args[n], XmNrightWidget, nw); ++n;
		}
		else if (nleft == XmATTACH_WIDGET)
		{
			// insert to left of elastic but not
			// at far left
			XtVaSetValues(nw,
				XmNleftWidget, w,
				0);
			XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
			XtSetArg(args[n], XmNleftWidget, pw); ++n;
		}
	}
	else // vertical
	{
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
		XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
		unsigned char	pbottom = XmATTACH_NONE;
		unsigned char	ntop = XmATTACH_NONE;
		if (!pw)
		{
			// insert at top
			// pbottom and ntop will be XmATTACH_NONE
			XtVaSetValues(nw,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, w,
				0);
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);
			++n;
			XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE);
			++n;
		}
		else
			XtVaGetValues(pw,
				XmNbottomAttachment, &pbottom,
				0);
		if (!nw)
		{
			// insert at bottom
			// pbottom and ntop will be XmATTACH_NONE
			XtVaSetValues(pw,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, w,
				0);
			if (elastic_added)
			{
				XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); 
				++n;
			}
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_NONE);
			++n;
		}
		else
			XtVaGetValues(nw,
				XmNtopAttachment, &ntop,
				0);
		if (pbottom == XmATTACH_WIDGET)
		{
			// insert below elastic but not at bottom
			XtVaSetValues(pw,
				XmNbottomWidget, w,
				0);
			XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); ++n;
			XtSetArg(args[n], XmNbottomWidget, nw); ++n;
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_NONE); ++n;
		}
		else if (ntop == XmATTACH_WIDGET)
		{
			// insert above elastic but not at top
			XtVaSetValues(nw,
				XmNtopWidget, w,
				0);
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
			XtSetArg(args[n], XmNtopWidget, pw); ++n;
			XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE); ++n;
		}
	}
	XtSetValues(w, args, n);
}

void
Expansion_box::add_component_with_separator(Component *p, Boolean is_elastic)
{
	Component 	*sib = (Component *)children.last();
	Widget		sw = sib ? sib->get_widget() : 0;
	Widget		sepw;

	sepw = XtVaCreateManagedWidget("separator", xmSeparatorGadgetClass, 
		widget,
		XmNorientation, orientation == OR_vertical ? 
				XmHORIZONTAL : XmVERTICAL,
		XmNseparatorType, XmSINGLE_LINE,
		0);
	add_to_form(sw, sepw, FALSE);
	add_to_form(sepw, p->get_widget(), is_elastic);
	children.add(p);
	if (is_elastic)
		elastic_added = TRUE;
}

// remove component after p
// assume we are not removing the elastic component
// any component we remove was attached to either
// the previous widet or the next, but not both
void
Expansion_box::remove_component(Component *p)
{
	Component	*prev, *next, *cur;

	if (!p)
	{
		prev = 0;
		cur = (Component *)children.first();
	}
	else
	{
		prev = (Component *)children.first();
		while(prev != p)
		{
			prev = (Component *)children.next();
		}
		cur = (Component *)children.next();
	}
	next = (Component *)children.next();

	Widget		pw = prev ? prev->get_widget() : 0;
	Widget		w = cur->get_widget();
	Widget		nw = next ? next->get_widget() : 0;

	if (orientation == OR_horizontal)
	{
		unsigned char	wleft, wright;
		XtVaGetValues(w,
			XmNleftAttachment, &wleft,
			XmNrightAttachment, &wright,
			0);
		if (!pw)
			XtVaSetValues(nw,
				XmNleftAttachment, XmATTACH_FORM,
				0);
		else if (!nw && elastic_added)
			XtVaSetValues(pw,
				XmNrightAttachment, XmATTACH_FORM,
				0);
		if (wleft == XmATTACH_WIDGET)
			XtVaSetValues(nw,
				XmNleftAttachment, XmATTACH_WIDGET,
				XmNleftWidget, pw,
				0);
		if (wright == XmATTACH_WIDGET)
			XtVaSetValues(pw,
				XmNrightAttachment, XmATTACH_WIDGET,
				XmNrightWidget, nw,
				0);
	}
	else	// vertical
	{
		unsigned char	wtop, wbottom;
		XtVaGetValues(w,
			XmNtopAttachment, &wtop,
			XmNbottomAttachment, &wbottom,
			0);
		if (!pw)
			XtVaSetValues(nw,
				XmNtopAttachment, XmATTACH_FORM,
				0);
		else if (!nw && elastic_added)
			XtVaSetValues(pw,
				XmNbottomAttachment, XmATTACH_FORM,
				0);
		if (wtop == XmATTACH_WIDGET)
			XtVaSetValues(nw,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, pw,
				0);
		else if (wbottom == XmATTACH_WIDGET)
			XtVaSetValues(pw,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, nw,
				0);
	}
	children.remove(cur);
}

void
Expansion_box::add_to_form(Widget sw, Widget w, Boolean is_elastic)
{
	Arg		args[5];
	int		n = 0;

	if (orientation == OR_horizontal)
	{
		// position to the right of the previous widget
		if (sw)
		{
			if (elastic_added)
			{
				XtVaSetValues(sw, 
					XmNrightAttachment, XmATTACH_WIDGET, 
					XmNrightWidget, w, 
					0);
				XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
			}
			else
			{
				XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); ++n;
				XtSetArg(args[n], XmNleftWidget, sw); ++n;
				if (is_elastic)
				{
					XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
				}
			}
		}
		else
		{
			XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
			if (is_elastic)
			{
				XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
			}
		}
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
		XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
		XtSetValues(w, args, n);
	}
	else
	{
		// position below the previous widget
		if (sw)
		{
			if (elastic_added)
			{
				XtVaSetValues(sw, 
					XmNbottomAttachment, XmATTACH_WIDGET, 
					XmNbottomWidget, w, 
					0);
				XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
			}
			else
			{
				XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); ++n;
				XtSetArg(args[n], XmNtopWidget, sw); ++n;
				if (is_elastic)
				{
					XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
				}
			}
		}
		else
		{
			XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
			if (is_elastic)
			{
				XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
			}
		}
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
		XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
		XtSetValues(w, args, n);
	}
}

// Divided box contains tiled panes with sashes
Divided_box::Divided_box(Component *p, const char *s, Boolean sashes, Help_id h)
	: Box(p, s, OR_vertical, Box_divided, h)
{
	// make a panes widget with sashes
	widget = XtVaCreateWidget(label, xmPanedWindowWidgetClass,
		parent->get_widget(),
		0);
	if (!sashes)
		XtVaSetValues(widget, 
			XmNsashHeight, 1, 
			XmNsashWidth, 1, 
			0);

	if (help_msg)
		register_help(widget, 0, help_msg);
}

void
Divided_box::add_component(Component *p, Boolean)
{
	children.add(p);
}
