#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/slider.h	1.8"
#endif

#include "changebar.h"

#ifndef _WSM_SLIDER_H
#define _WSM_SLIDER_H

typedef struct Slider 
{
	Boolean		caption;
	char *		name;
	char *		string;
	void		(*f) ( struct Slider *, XtPointer, Boolean);
	XtPointer	closure;
	int		slider_value;
	int		slider_min;
	int		slider_max;
	String		min_label;
	String		max_label;
	int		granularity;
	int		ticks;
	Widget		w;
	Boolean		track_changes;
	Boolean		sensitive;
	char		string_slider_value[20];
	ChangeBar*	changebar;
	void		(*change) ();
}			Slider;

#define StringSliderValue(P) ( \
	sprintf((P)->string_slider_value, "%d", (P)->slider_value),	\
	(P)->string_slider_value					\
    )

#define SLIDER(name, value, min, max, minlabel, \
	       maxlabel, gran, ticks) \
       {True, name, NULL, NULL, NULL, value, min, max, \
	minlabel, maxlabel, gran, ticks, NULL, False, True}

extern Widget		CreateSlider (
	Widget			parent,
	Slider *		slider,
	Boolean			track_changes,
	void			(*change)()
);
extern void		SetSlider (
	Slider *		slider,
	int			value,
	int			change_state
);

#endif
