#ifndef BUTTON_BAR_H
#define BUTTON_BAR_H

#ident	"@(#)debugger:gui.d/common/Button_bar.h	1.10"

#include "Component.h"
#include "Button_barP.h"
#include "config.h"

class Base_window;
class Button;
class Window_set;
class Command_sender;
struct Bar_info;

class Button_bar : public Component
{
	BUTTON_BAR_TOOLKIT_SPECIFICS

private:
	Base_window		*window;
	Window_set		*window_set;
	Bar_descriptor		*first_panel;
	Bar_descriptor		*current_panel;
	short			nbuttons;
	unsigned char		current_index;
	Boolean			multiple;
public:
				Button_bar(Component *parent,
					Base_window *w,
					Window_set *window_set,
					Bar_descriptor *,
					Orientation o = OR_horizontal,
					Command_sender *c = 0);
				~Button_bar();

				// access functions
	Base_window		*get_window() { return window; }
	Window_set		*get_window_set() { return window_set; }
	int			get_nbuttons() { return nbuttons; }
	Button			*get_buttons() { return current_panel ?
					current_panel->get_buttons(): 0; }

				// set sensitivity of an individual button
	void			set_sensitivity(int button, Boolean sense);
	int			get_current_index() {return current_index; }
	void			update_bar(Bar_info *);
	void			destroy();
};

#endif /* BUTTON_BAR_H*/
