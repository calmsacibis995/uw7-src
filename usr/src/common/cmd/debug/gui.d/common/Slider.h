#ifndef	_SLIDER_H
#define	_SLIDER_H
#ident	"@(#)debugger:gui.d/common/Slider.h	1.1"

#include "Component.h"
#include "SliderP.h"

// A slider lets the user select a value from within a specified range

// Framework callbacks:

class Slider : public Component
{
	SLIDER_TOOLKIT_SPECIFICS

public:
		Slider(Component *parent, const char *name, Orientation,
			int min, int max, int initial, int granularity,
			Help_id help_msg = HELP_none);
		~Slider() {};

	int	get_value();
	void	set_value(int);
};

#endif	// _SLIDER_H
