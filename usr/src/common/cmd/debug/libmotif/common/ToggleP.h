#ifndef	_TOGGLEP_H
#define	_TOGGLEP_H
#ident	"@(#)debugger:libmotif/common/ToggleP.h	1.1"

// toolkit specific members of Toggle_button class
// included by ../../gui.d/common/Toggle.h

#define	TOGGLE_BUTTON_TOOLKIT_SPECIFICS		\
private:					\
	Widget			*toggles;	\
        const Toggle_data       *buttons;	\
        int                     nbuttons;	\
						\
public:						\
	const Toggle_data	*get_buttons()	{ return buttons; }	\
	int			get_nbuttons()	{ return nbuttons; }

#endif	// _TOGGLEP_H
