#ident	"@(#)debugger:libmotif/common/Slider.C	1.1"

#include "UI.h"
#include "Component.h"
#include "Slider.h"
#include "Machine.h"
#include <Xm/Scale.h>

Slider::Slider(Component *p, const char *s, Orientation orient,
	int min, int max, int initial, int granularity,
	Help_id h) : COMPONENT(p, s, 0, h)
{
	widget = XtVaCreateManagedWidget((char *)label,
		xmScaleWidgetClass,
		parent->get_widget(),
		XmNminimum, min,
		XmNmaximum, max,
		XmNvalue, initial,
		XmNscaleMultiple, granularity,
		XmNorientation, (orient == OR_vertical) ? XmVERTICAL : XmHORIZONTAL,
		XmNshowValue, TRUE,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

int
Slider::get_value()
{
	int	val = 0;

	XtVaGetValues(widget, XmNvalue, &val, 0);
	return val;
}

void
Slider::set_value(int val)
{
	XtVaSetValues(widget, XmNvalue, val, 0);
}
