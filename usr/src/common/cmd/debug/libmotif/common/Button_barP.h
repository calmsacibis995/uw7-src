#ifndef	_BUTTON_BARP_H
#define	_BUTTON_BARP_H
#ident	"@(#)debugger:libmotif/common/Button_barP.h	1.3"

// toolkit specific members of the Button_box class
// included by ../../gui.d/common/Button_box.h

class MneInfo;
class Button;

#define	BUTTON_BAR_TOOLKIT_SPECIFICS		\
public:					\
	void	display_next_panel();	\
private:				\
	MneInfo	*mne_info;		\
	Widget	*xbuttons;		\
	void	change_display(Bar_descriptor *nbar, Boolean);	\
	void	update_current(Bar_descriptor *update);		\
	void	set_button(Widget *, Button *, int);

#endif	// _BUTTON_BARP_H
