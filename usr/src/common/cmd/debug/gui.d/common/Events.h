#ifndef _EVENTS_H
#define _EVENTS_H
#ident	"@(#)debugger:gui.d/common/Events.h	1.26"

#include "Component.h"
#include "Dialogs.h"
#include "Panes.h"
#include "Reason.h"

class Base_window;
class Pane_descriptor;
class Menu_bar_table;
class Message;
class ProcObj;
class Status_pane;
class Event;
class Table_calldata;
class Buffer;
class Sensitivity;
class Text_line;

// The dialogs do not directly delete any screen components (Text_line, Radio_list, etc)
// The complete component tree is deleted recursively when the Dialog_shell is
// deleted in the Dialog destructor

class Stop_dialog : public Process_dialog
{
	Text_line	*expr;
	Text_line	*commands;
	Text_line	*count;
	int		eventId;
	int		doChange;

			// state variables for cancel operation
	char		*save_expr;
	char		*save_cmd;
	char		*save_count;
public:
			Stop_dialog(Base_window *);
			~Stop_dialog() { delete save_expr; delete save_cmd;
					 delete save_count; }

			// access functions
	void		setEventId(int id)	{ eventId = id;}
	void		setChange(int s = 0);

			// initialization routines
	void		fillContents(Event *);
	void		set_expression(const char *);

			// callbacks
	void		apply(Component	*, int);
	void		reset(Component *, int);
};

class Stop_on_function_dialog : public Object_dialog
{
	Selection_list	*functions;
	Caption		*caption;
	int		f_selection;
	Text_line	*filter;

	void		object_changed(const char *obj);
	void		do_it();
public:
			Stop_on_function_dialog(Base_window *);
			~Stop_on_function_dialog() { delete save_proc; }

	void		set_obj(Boolean reset);

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		default_cb(Component *, int);
};

class Signal_dialog : public Process_dialog
{
	Selection_list	*siglist;
	Text_line	*commands;
	Radio_list	*ordering;

			// state variables for reset operation
	Order		order;
	int		eventId;
	int		doChange;
	char		*save_cmd;
	int		*selections;
	int		nselections;

	int		do_it(int, int *);
public:
			Signal_dialog(Base_window *);
			~Signal_dialog() { delete save_cmd; delete selections; };

			// access functions
	void		setEventId(int id)	{ eventId = id;}
	void		setChange(int s = 0);

			// initialization function
	void		fillContents(Event *);

			// callbacks
	void		set_order(Component *, Order);
	void		apply(Component *, int);
	void		reset(Component *, int);
	void		default_cb(Component *, int);
};

class Syscall_dialog : public Process_dialog
{
	Selection_list	*syslist;
	Text_line	*commands;
	Radio_list	*ordering;
	Text_line	*count;
	Toggle_button	*entry_exit;
	Order		order;
	int		eventId;
	int		doChange;

			// state information saved for reset
	char		*save_cmd;
	char		*save_count;
	int		entry_exit_state;	// which choices are set
	int		*selections;
	int		nselections;

	int		do_it(int, int *);
public:
			Syscall_dialog(Base_window *);
			~Syscall_dialog() { delete selections; delete save_cmd;
					    delete save_count; }

			// access routines
	void		setEventId(int id)	{ eventId = id;}
	void		setChange(int s = 0);

			// initialization function
	void		fillContents(Event *);

			// callbacks
	void		set_order(Component *, Order);
	void		apply(Component *, int);
	void		reset(Component *, int);
	void		default_cb(Component *, int);
};

class Onstop_dialog : public Process_dialog
{
	Text_area	*commands;

			// state variables for reset operation
	char		*save_cmds;
	int		eventId;
	int		doChange;
public:
			Onstop_dialog(Base_window *);
			~Onstop_dialog() { delete save_cmds; };

			// access functions
	void		setEventId(int id)	{ eventId = id;}
	void		setChange(int s = 0);

			// initialization function
	void		fillContents(Event *);

	void		apply(Component *, int);
	void		reset(Component *, int);
};

#if EXCEPTION_HANDLING
class Exception_dialog : public Process_dialog
{
	enum TC_state { tc_none, tc_throw, tc_catch, tc_both };

	TC_state	throw_catch_state;	// which choices are set;
	Toggle_button	*throw_catch;
	Text_line	*type;
	Text_line	*commands;

			// state variables for reset operation
	int		eventId;
	int		doChange;
	char		*save_cmd;
	char		*save_type;

	void		reset_toggles();
	TC_state	get_state(const char *&);
public:
			Exception_dialog(Base_window *);
			~Exception_dialog() { delete save_cmd; delete save_type; };

			// access functions
	void		setEventId(int id)	{ eventId = id;}
	void		setChange(int s = 0);

			// initialization function
	void		fillContents(Event *);

			// callbacks
	void		apply(Component *, int);
	void		reset(Component *, int);
};

class Ignore_exceptions_dialog : public Process_dialog
{
	Selection_list	*ehlist;
	Radio_list	*which_list;
	int		list_type;

	void		common_apply(Boolean ignore);
	void		do_it(int, int *, Boolean ignore);
	void		set_list(int);	// initialization
public:
			Ignore_exceptions_dialog(Base_window *);
			~Ignore_exceptions_dialog() {};

			// callbacks
	void		ignore_cb(Component *, int);
	void		stop_cb(Component *, int);
	void		cancel(Component *, int);
	void		default_cb(Component *, int);
	void		set_list_type(Component *, int which);

			// functions inherited from Dialog_box or Process_dialog
	void		set_obj(Boolean reset);
	void		cmd_complete();
};
#endif

class Cancel_dialog : public Process_dialog
{
	Selection_list	*siglist;
	Order		order;
	Radio_list	*ordering;

			// state variables for reset operation
	int		nselections;
	int		*selections;

	void		set_list(Order);	// reset the display when anything changes
	void		do_it(int, int *);

public:
			Cancel_dialog(Base_window *);
			~Cancel_dialog() { delete selections; };

			// callbacks
	void		apply(Component *, int);
	void		cancel(Component *, int);
	void		set_order(Component *, Order);
	void		default_cb(Component *, int);

			// inherited from Process_dialog
	void		set_obj(Boolean reset);

			// functions inherited from Dialog_box 
	void		cmd_complete();
};

class Kill_dialog : public Process_dialog
{
	Selection_list	*siglist;
	Order		order;
	Radio_list	*ordering;

			// state variables for reset operation
	char		*selection;

	void		do_it(int);
public:
			Kill_dialog(Base_window *);
			~Kill_dialog() { delete selection; };

			// callbacks
	void		apply(Component *, int);
	void		cancel(Component *, int);
	void		set_order(Component *, Order);
	void		default_cb(Component *, int);
#ifdef DEBUG_THREADS
	void		thread_kill_cb(Component *, int);
#endif
};

class Setup_signals_dialog : public Process_dialog
{
	Selection_list	*siglist;
	Order		order;
	Radio_list	*ordering;
	int		list_type;
	Caption		*hcaption;
	Radio_list	*which_list;
	int		first_time;

	void		set_list(Order, int which);
	// reset the display when anything changes
	void		do_it(int, int *, Boolean ignore);
	void		common_apply(Boolean ignore);
			// This function returns the number of signals.
			// Pointers to the strings are in the first vector.
			// The strings themselves are in the second vector.
	int		get_signals_with_status(ProcObj *, Order,
				Vector *, Vector *);
public:
			Setup_signals_dialog(Base_window *);
			~Setup_signals_dialog() {};

			// callbacks
	void		catch_sigs(Component *, void *);
	void		ignore_sigs(Component *, void *);
	void		cancel(Component *, int);
	void		set_order(Component *, Order);
	void		set_list_type(Component *, int which);
	void		default_cb(Component *, int);

			// functions inherited from Dialog_box
	void		set_obj(Boolean reset);
	void		cmd_complete();
};

class Event_pane : public Pane
{
	Table			*events;
	Table			*onstop;
	int			max_events;
	int			max_onstop;
						// flag for sensitivity
	int		ableEventSel;		// true if one or more undisabled
						//  event is selected
	int		disableEventSel;

	void		count_selections(Table *);
	void		get_selected_ids(Buffer *);
public:
			Event_pane(Window_set *, Base_window *, Box *,
				Pane_descriptor *);
			~Event_pane();

			// display functions
	void		update_cb(void *server, Reason_code, void *, ProcObj *);
	void		update_state_cb(void *, Reason_code, void *, ProcObj *);
	void		disableEventCb(Component *, void *);
	void		enableEventCb(Component *, void *);
	void		deleteEventCb(Component *, void *);
	void		changeEventCb(Component *, void *);

			// update the display
	void		add_event(Event *);
	void		delete_event(Event *);
	void		change_event(Event *);

	void		selectEventCb(Table *, Table_calldata *);
	void		col_selectEventCb(Table *, Table_calldata *);
	void		deselectEventCb(Table *, int);

	int		contains(int eId);	//!0 if contains the event eId
	Table		*getEventPane(){ return events;}
	Table		*getOnstopPane(){ return onstop;}

			// functions overriding virtuals in Pane base class
	void		popup();
	void		popdown();
	Selection_type	selection_type();
	void		deselect();
	int		check_sensitivity(Sensitivity *);
};

#endif	// _EVENTS_H
