#ifndef	_SENDER_H
#define	_SENDER_H
#ident	"@(#)debugger:gui.d/common/Sender.h	1.4"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define COMMAND_SENDER	Command_sender
#else
#define COMMAND_SENDER
#endif

class Base_window;
class Message;
class Window_set;
class Component;
class Notifier;

// Command_sender is the generic base class of nearly all framework objects.
// The class allows the object to send a command to the debugger,
// and to register callback functions with other framework objects or
// graphics components.
//
// When the debugger responds to a command sent from a Command_sender,
// the Dispatcher uses get_window_set and de_message to direct
// the message to the right place.  de_message may be called more
// than once per command.  The Dispatcher will call cmd_complete
// when it receives the final message (MSG_cmd_complete)

class Command_sender
{
protected:
	Window_set	*window_set;

public:
			Command_sender(Window_set *ws)	{ window_set = ws; }
			~Command_sender() {}

			// access functions
	Window_set	*get_window_set()	{ return window_set; }

			// handle the incoming message
	virtual void	de_message(Message *);
	virtual void	cmd_complete();
	virtual Base_window	*get_window();
};

// Framework callbacks have one of two forms:
//
// For component callbacks:
// 1st (implied) argument - pointer to the framework object that created the Component
// 2nd argument - pointer to the component
// 3rd argument - Component/callback specific - for most, simply zero
//
// For Notifier callbacks:
// 1st (implied) argument - pointer to the framework object
// 2nd argument - pointer to the Notifier object
// 3rd argument - reason for the notification
// 4th argument - data the framework object passed in when it registered with
// 			the notifier
// 5th argument - data from the notifier object
//
// NOTE: For the callbacks to work,
// 1) all objects that use callbacks must be derived from Command_sender,
// 2) if multiple inheritance is used, the object is must be derived from
//    Command_sender first - the code relies on the assumption that when
//    you cast the pointer to the object to (Command_sender *), that you
//    get the same pointer! (and vice versa)  If this assumption is not true,
//    lots of stuff will break
typedef void (Command_sender::*Callback_ptr)(Component *, void *);

typedef void (Command_sender::*Notify_func)(void *, int reason_code,
			void *client_data, void *call_data);

#endif	// _SENDER_H
