#ifndef	PANES_H
#define	PANES_H
#ident	"@(#)debugger:gui.d/common/Panes.h	1.3"

#include "Sender.h"
#include "Component.h"

class Base_window;
class Menu;
class Vector;
class Sensitivity;

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define PANE	Pane
#else
#define PANE
#endif


#define PT_disassembler	0x1
#define PT_event	0x2
#define PT_process	0x4
#define PT_registers	0x8
#define PT_source	0x10
#define PT_second_src	0x20
#define PT_stack	0x40
#define PT_status	0x80
#define PT_symbols	0x100
#define PT_command	0x200

#define N_PANES		10

#define ALL_PANES	0xffff

class Pane : public Command_sender
{
protected:
	Base_window		*parent;
	Menu			*popup_menu;
	int			type;

public:
				Pane(Window_set *ws, Base_window *base, int t)
					: COMMAND_SENDER(ws)
					{ parent = base; type = t; 
						popup_menu = 0; }
	virtual			~Pane() {}

				// access functions
	int			get_type()		{ return type; }
	Base_window		*get_parent()		{ return parent; }
	Base_window		*get_window();

	virtual void		popup();
	virtual void		popdown();
	virtual char		*get_selection();
	virtual int		get_selections(Vector *);
	virtual void		copy_selection();
	virtual Selection_type	selection_type();
	virtual void		deselect();
	virtual	int		check_sensitivity(Sensitivity *);
};

extern int	find_pane_type(const char *);
extern const char	*pane_name(int);

#endif // PANES_H
