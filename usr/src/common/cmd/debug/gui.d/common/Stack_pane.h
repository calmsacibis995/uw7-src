#ifndef _STACK_PANE_H
#define _STACK_PANE_H
#ident	"@(#)debugger:gui.d/common/Stack_pane.h	1.10"

#include "Component.h"
#include "Windows.h"
#include "Panes.h"

enum Stack_cell { ST_current, ST_frame, ST_func, ST_params, ST_loc };

class Base_window;
class Message;
class ProcObj;
class Divided_box;
class Table_calldata;
struct Pane_descriptor;
class Sensitivity;

class Stack_pane : public Pane
{
	Table		*pane;
	int		top_frame;
	int		cur_frame;
	int		next_row;
	int		has_selection;
	char		*func;
	int		update_in_progress;
	Boolean		update_abort;
	Boolean		stack_count_down;
public:
			Stack_pane(Window_set *, Base_window *, Box *,
				Pane_descriptor *);
			~Stack_pane();

			// access functions
	Table		*get_table()	{ return pane; }
	int		get_frame(int frame, const char *&func, const char *&loc);
	void		reset();
	void		redisplay();

			// callbacks
	void		update_cb(void *server, Reason_code, void *, ProcObj *);
	void		update_state_cb(void *server, Reason_code, void *, ProcObj *);
	void		select_frame(Table *, Table_calldata *);
	void		deselect_frame(Table *, int);
	void		set_current();
	void		default_action(Table *, Table_calldata *);

			// functions inherited from Command_sender
	void		de_message(Message *);
	void		cmd_complete();

			// functions overriding virtuals in Pane base class
	void		deselect();
	Selection_type	selection_type();
	void		popup();
	void		popdown();
	int		check_sensitivity(Sensitivity *);
};

#endif	// _STACK_PANE_H
