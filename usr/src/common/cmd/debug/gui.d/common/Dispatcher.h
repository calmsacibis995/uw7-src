#ifndef	_DISPATCHER_H
#define	_DISPATCHER_H
#ident	"@(#)debugger:gui.d/common/Dispatcher.h	1.11"

#include <stdarg.h>
#include "Message.h"
#include "UI.h"

class Component;
class Command_sender;
class Window_set;

// The Dispatcher handles all the message traffic between the gui
// and the debugger engine.

// Commands are sent to the debugger via send_msg; any object that
// calls send_msg must be derived from Command_sender.  The dispatcher
// sends the results of the command back to the Command_sender
// via its de_message and cmd_complete member functions.
// de_message may be called more than once per command.
// All incoming messages except query responses are handled by process_msg()

// Queries (commands asking for information the gui needs immediately)
// are handled by calls to query() followed by get_response().
// A response may consist of more than one message, so get_response()
// should be called in a loop until it returns 0.
 
class Dispatcher
{
	Message	current_msg;		// incoming message
	Message	out_msg;		// outgoing message
	Message quit_msg;		// quit message sent on abort
	Message sync_msg;		// sync_response message
	int	create_id;		// number of create commands issued
	Boolean	first_new_process;	// flags first process in a grab or create
	Boolean	process_killed;		// flag telling when to call Window_set::set_current
	Boolean	in_create;		// flag is true between MSG_createp ... MSG_cmd_complete
	Boolean io_flag;		// true if MSG_new_pty was seen

	Boolean notice_raised;		// true while associated command notice is up
	Boolean	update_needed;		// delay process updates to sync_request

	void	sync_response();
	void	update_new_processes();
public:
		Dispatcher();
		~Dispatcher()	{}

	void	process_msg();
	void	send_msg(Command_sender *, DBcontext, const char * ...);
	void	send_response(DBcontext, int response);
	void	query(Command_sender *, DBcontext, const char * ...);
	Message	*get_response();
	void	cleanup();

		// access functions
	int	get_create_id()	{ return create_id; }
	void	set_notice_raised(Boolean n) { notice_raised = n; }
};

extern	Dispatcher	dispatcher;

#endif	// _DISPATCHER_H
