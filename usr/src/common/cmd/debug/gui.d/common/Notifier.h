#ifndef	_NOTIFIER_H
#define	_NOTIFIER_H
#ident	"@(#)debugger:gui.d/common/Notifier.h	1.2"

// A Notifier is used by a data server object.
// The notifier keeps a list of client objects using the data,
// and notifies the clients when the data changes
// The client registers itself through notifier.add; Notify_func is a
// callback routine in the client
// clients are notified in the order they are registered

#include "List.h"
#include "Sender.h"

class Notifier
{
	void	*server;
	List	clients;
public:
		Notifier(void *s)	{ server = s; }
		~Notifier();

		// register a new client
	void	add(Command_sender *client, Notify_func, void *client_data);

		// unregister a client
	int	remove(Command_sender *client, Notify_func, void *client_data);

		// notify all clients
	void	notify(int reason_code, void *call_data);
};
#endif	// _NOTIFIER_H
