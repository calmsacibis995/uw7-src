#ifndef	_RADIOP_H
#define	_RADIOP_H
#ident	"@(#)debugger:libmotif/common/RadioP.h	1.1"

// toolkit specific members of Radio class,
// included by ../../gui.d/common/Radio.h

// Radio list is implemented as a FlatButton widget

// toolkit specific members:
//	buttons		list of ToggleButtons
//	nbuttons	total number of buttons
//	current		button currently pushed

#define RADIO_TOOLKIT_SPECIFICS		\
private:				\
	Widget		*buttons; 	\
        int             nbuttons;       \
        int             current;        \
					\
public:					\
	int		get_nbuttons()	{ return nbuttons; }	\
	void		set_current(int i)	{ current = i; }

#endif	// _RADIOP_H
