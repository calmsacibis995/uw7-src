#ifndef	_UI_H
#define	_UI_H
#ident	"@(#)debugger:gui.d/common/UI.h	1.27"

// GUI headers
#include "gui_msg.h"
#include "Component.h"
#include "gui_label.h"

// Debug headers
#include "Language.h"
#include "Severity.h"

#include <stdarg.h>
#include <X11/Intrinsic.h>

class	Command_sender;
class	Message;
class	Vector;

// Order is used by the functions that return lists of signals or system calls
enum Order { Alpha, Numeric };

extern	int		in_script;
extern	int		has_assoc_cmd;
extern  int		exit_called;

// initialize message catalog
extern 	void		init_message_cat();

// get_signals and get_syslist set the global variables Nsignals and Nsyscalls,
// respectively, to tell how many entries are in the lists.
extern	int		Nsignals;
extern	int		Nsyscalls;
extern	const char	**get_signals(Order);
extern	const char	**get_syslist(Order);

// comparison function for qsort
extern	int	alpha_comp(const void *, const void *);

// Toolkit specific routines
extern	void	init_gui(const char *name, const char *widget_class,
			int *argc, char **argv);
extern  void	toolkit_main_loop();
extern	void	beep();

// interface to sprintf that ensures there is enough room
char *do_vsprintf(const char *fmt, va_list args);

extern	Language	cur_language;
extern	int		dis_mode;
extern	int		set_debug_dis_mode(Command_sender *, int);



// the following are called by the Dispatcher
// to keep certain dialogs up to date
extern	void		set_create_args(Message *);
extern	void		set_lang(Message *);
extern	void		set_frame_dir(Message *);
extern	void		set_dis_mode(Message *);

// handle messages (informational or errors) that aren't associated with
// a particular window, or that need confirmation
// The callback in the last version of display_msg is invoked as
//	 object->function((void *)0, int ok_or_cancel)

enum Alert_type {
	AT_error,
	AT_fatal,
	AT_warning,
	AT_question,
	AT_message,
	AT_urgent,
};

extern	void	display_msg(Message *);
extern	void	display_msg(Severity, Gui_msg_id ...);
extern	void	display_msg(Callback_ptr, void *obj, LabelId action,
			LabelId no_action, Alert_type, Gui_msg_id ...);
extern	void	display_msg(Callback_ptr, Command_sender *obj, LabelId action,
			LabelId no_action, Message *);
extern	void	query_handler(Message *);

// get the ps info for the Grab Process dialog
// two scratch vectors are needed, v1 the strings, v2 holds pointers to the strings
extern int	do_ps(Vector *v1, Vector *v2);

// returns TRUE if the message is one that should never be displayed to the user
extern Boolean	gui_only_message(Message *);

// frees an array of strings
extern void	free_strings(const char **list, int size);

// notify debugger we are ready
extern void	notify_debugger();

#ifdef DEBUG_THREADS
// creates process list from procobj list
extern int	get_obj_processes(ProcObj **olist, int ocnt, Vector *v);
#endif

#endif	// _UI_H
