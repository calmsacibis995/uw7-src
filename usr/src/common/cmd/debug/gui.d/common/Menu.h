#ifndef	_MENU_H
#define	_MENU_H
#ident	"@(#)debugger:gui.d/common/Menu.h	1.25"

#include "Component.h"
#include "List.h"
#include "MenuP.h"
#include "Menu_barP.h"
#include "Buttons.h"

class Base_window;
class Menu_descriptor;
class Window_set;

class Menu : public Component
{
	MENU_TOOLKIT_SPECIFICS

private:
	const char		*name;
	Base_window		*window;
	Window_set		*window_set;
	List			*children;
	Button			*buttons;
	Button			*trigger; // for sub-menus
	unsigned short		nbuttons;
	Boolean			popped_up;

public:
				Menu(Component *parent,
					Boolean title, Widget pwidget,
					Base_window *window, Window_set *window_set,
					Menu_descriptor *,
					Boolean pull_down = TRUE,
					Help_id help_msg = HELP_none);
				~Menu();

				// access functions
	Base_window		*get_window()		{ return window; }
	Window_set		*get_window_set()	{ return window_set; }
	int			get_nbuttons()		{ return nbuttons; }
	Button			*get_buttons()		{ return buttons; }
	Button			*find_item(const char *);
	void			add_item(Button *);
	void			delete_item(const char *);
	void			delete_item(Button *, int);
	void			set_menu_label(int, const char *);

	Menu			*first_child()	{ return (children ?
					(Menu *)children->first() : 0);}
	Menu			*next_child()	{ return (children ?
					(Menu *)children->next() : 0);}

				// set the sensitivity of an individual button
	void			set_sensitive(int button, Boolean);
	const char		*get_name() { return name; }
	Button			*get_trigger() { return trigger; }
	Boolean			is_popped_up() { return popped_up; }
	void			set_popped_up() { popped_up = TRUE; }
	void			clear_popped_up() { popped_up = FALSE; }
};

class Menu_bar : public Component
{
	MENUBAR_TOOLKIT_SPECIFICS

private:
	int		nbuttons;
	Menu		**children;

public:
			Menu_bar(Component *parent, Base_window *window,
				Window_set *window_set, 
				Menu_descriptor *,
				Help_id help_msg = HELP_none);
			~Menu_bar();

			// access functions
	Menu		**get_menus()	{ return children; }
	int		get_nbuttons()	{ return nbuttons; }
	Menu		*find_item(CButtons);
};

extern Menu	*create_popup_menu(Component *, Base_window *,
			Menu_descriptor *);

#endif	// _MENU_H
