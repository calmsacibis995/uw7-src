#ifndef	_DIALOGS_H
#define _DIALOGS_H
#ident	"@(#)debugger:gui.d/common/Dialogs.h	1.21"

#include "Component.h"
#include "Dialog_sh.h"
#include "Sender.h"
#include "Base_win.h"
#include "UI.h"
#include "Reason.h"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define DIALOG_BOX	Dialog_box
#define PROCESS_DIALOG	Process_dialog
#define OBJECT_DIALOG	Object_dialog
#else
#define DIALOG_BOX
#define PROCESS_DIALOG
#define OBJECT_DIALOG
#endif

class	Window_set;
class	Process;
#ifdef DEBUG_THREADS
class	ProcObj;
#endif

// All popup windows are derived from Dialog_box, either directly or
// indirectly through Process_dialog.  de_message in the base class
// handles all error messages, but if the derived class expects other types of
// messages, it should provide its own version of de_message

class Dialog_box : public Command_sender
{
public:
	Dialog_shell	*dialog;
	Base_window	*window;

			Dialog_box(Base_window *bw) : 
				COMMAND_SENDER(bw->get_window_set())
					{ dialog = 0; window = bw; }
			~Dialog_box()	{ delete dialog; }

		void	display();
		void	dismiss();
		void	show_error(Message *m, Boolean stay_up);
		void	state_change_cb(void *, int rc, void *, ProcObj *proc);
		Dialog_shell	*get_shell() { return dialog; }

	virtual void	de_message(Message *);
	virtual void	cmd_complete();
	Base_window	*get_window();
};

// Process_dialog is the base class for all popup windows that depend on
// either the current process or the process selection in the Ps_pane
// (all the Event and Control menu dialogs, and some others, are derived
// from Process_dialog).  Process_dialog maintains a list of the selected
// processes to ensure that the dialog doesn't get into a bad state if
// a selected process dies while the dialog is popped up - but the derived
// class must still check that the processes pointer is non-null before
// using it.
class Process_dialog : public Dialog_box
{
protected:
	unsigned char	level;		// program(s), process(es), or thread(s)
	Boolean		track_current;	// true if no selection in Ps_pane
	short		total;		// number of selections
	ProcObj		**pobjs;
	Simple_text	*obj_list;
	Caption		*obj_caption;
	Boolean		single_process;	// use first process of list only

			// component_init is called once in the derived class, when
			// the dialog is first created, to create the header widgets.
			// It should be called right after the box component is
			// created
	void		component_init(Packed_box *);
	void		component_init(Expansion_box *);

public:
			// sp indicates whether only one process is used
			Process_dialog(Base_window *, Boolean sp = false);
			~Process_dialog() { delete pobjs; }

			// callbacks
	void		update_list_cb(void *, Reason_code, void *, ProcObj *proc);
	void		update_current_cb(void *, Reason_code, void *, ProcObj *proc);
	void		dismiss_cb(Component *, void *);

			// set_plist is called every time the dialog is popped up
	void		set_plist(Base_window *, unsigned char level);

			// set_obj is called when the current process changes,
			// if track_current is true.  reset is false when the
			// object list is first set; true if the object changes
			// while the dialog is already popped up.  A derived
			// class should define set_obj if the information
			// displayed in the window should be updated when the
			// current object changes
	virtual void	set_obj(Boolean reset);

			// update_obj is called whenever any object in the
			// list changes state.  A derived class should define
			// update_obj if the information displayed in the
			// window must be updated
	virtual void	update_obj(ProcObj *, Reason_code = RC_change_state);
};

// Object_dialog is the base class for popup windows that contain list of objects
// (like Stop on Function).  Those popups have several common operations, like
// saving the selected object for cancel, and updating some other piece of
// information in the window when a different object is selected
class Object_dialog : public Process_dialog
{
protected:
	Selection_list	*objects;
	short		current_object;
	short		saved_object;
	char		*save_proc;

public:
			Object_dialog(Base_window *);
			~Object_dialog() { delete save_proc; }

			// create the graphical components
	Component	*make_obj_list(Component *parent);

			// callbacks
	void		select_cb(Selection_list *, int);

			// the object list is updated whenever the process changes
	void		set_obj(Boolean reset);

			// a derived class should define object_changed to update
			// the information it displays
	virtual void	object_changed(const char *object);

	void		cancel_change();
};

class Create_dialog : public Dialog_box
{
	Text_line	*cmd_line;
	Text_line	*start_loc;
	Toggle_button	*toggles;
	char		*save_cmd;
	char		*save_location;
	Boolean		follow_state;
	Boolean		io_state;
	Boolean		new_window_state;
	Boolean		kill_state;
	Window_set	*save_ws;
public:
			Create_dialog(Base_window *);
			~Create_dialog() { delete save_cmd; delete save_location; }

			// overriding Dialog_box's
	void		de_message(Message *);

			// display functions and callbacks
	void		do_create(Component *, void *);
	void		drop_cb(Component *, void *);
	void		cancel(Component *, void *);

			// update the display
	void		set_create_args(const char *s);
};

class Grab_process_dialog : public Dialog_box
{
	Text_line	*object_file;
	Toggle_button	*toggles;
	Selection_list	*ps_list;
	char		*save_obj;
	Boolean		follow_state;
	Boolean		new_window_state;
	Window_set	*save_ws;

	void		do_it(Window_set *);	// called by apply & drop_cb to do the grab
	Boolean		do_grab(int, int *);
public:

			Grab_process_dialog(Base_window *);
			~Grab_process_dialog() { delete save_obj; }

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		drop_cb(Selection_list *, Component *dropped_on);

	void		default_cb(Component *, int);
	void		setup();		// initialize process list
};

class Grab_core_dialog : public Dialog_box
{
	Text_line	*object_file;
	Text_line	*core_file;
	Toggle_button	*new_set;
	char		*save_core;
	char		*save_obj;
	Boolean		save_toggle;
	Window_set	*save_ws;
public:

			Grab_core_dialog(Base_window *);
			~Grab_core_dialog() { delete save_core; delete save_obj; }

			// display functions and callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		drop_cb(Component *, void *);
};

class Set_language_dialog : public Process_dialog
{
	Simple_text	*current_lang;
	Radio_list	*lang_choices;
public:
			Set_language_dialog(Base_window *);
			~Set_language_dialog() {}

	void		apply(Component *, void *);
	void		reset(Component *, void *);

			// overriding Process_dialog's
	void		update_obj(ProcObj *, Reason_code = RC_change_state);
	void		set_obj(Boolean);
};

class Cd_dialog : public Dialog_box
{
	Simple_text	*current_directory;
	Text_line	*new_directory;
	char		*save_text;

public:
			Cd_dialog(Base_window *);
			~Cd_dialog()	{ delete save_text; }

			// button callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

			// update functions
	void		update_directory(const char *);
};

class Move_dialog : public  Process_dialog
{
	Radio_list	*choices;
	Selection_list	*ws_sel_list;

	int		get_ws_list(Vector *);
	void		free_ws_list(char **, int);
	void		do_it(int);
	void		move_objs(Window_set *);
public:
			Move_dialog(Base_window *);
			~Move_dialog() { }

	void		setup();

	void            apply(Component *, void *);
	void		reset(Component *, void *);
	void		default_cb(Component *, int);
	void		choices_cb(Component *, void *);
};

#endif // _DIALOGS_H
