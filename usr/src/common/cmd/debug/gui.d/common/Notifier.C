#ident	"@(#)debugger:gui.d/common/Notifier.C	1.2"

#include "Notifier.h"

struct Client
{
	Command_sender	*object;	// registered object
	Notify_func	func;		// member function of object
	void		*client_data;

			Client(Command_sender *o, Notify_func f, void *d)
				{ object = o; func = f; client_data = d; };
			~Client() {}
};

Notifier::~Notifier()
{
	Client	*p;

	// remove each item on the list, List links are deleted by List destructor
	for (p = (Client *)clients.first(); p; p = (Client *)clients.next())
		delete p;
}

// register a new client
void
Notifier::add(Command_sender *client, Notify_func func, void *client_data)
{
	Client	*new_client = new Client(client, func, client_data);
	clients.add(new_client);
}

// un-register a client
int
Notifier::remove(Command_sender *old_client, Notify_func old_func, void *old_cdata)
{
	Client	*item;

	for (item = (Client *)clients.first(); item;
		item = (Client *)clients.next())
	{
		if (item->object == old_client && item->func == old_func
			&& item->client_data == old_cdata)
			break;
	}

	if (item)
	{
		clients.remove(item);
		return 1;
	}
	else
		return 0;
}

// invoke all callbacks registered with this notifier
void
Notifier::notify(int reason_code, void *call_data)
{
	Client		*item;
	Notify_func	func;
	for (item = (Client *)clients.first(); item;
			item = (Client *)clients.next())
	{
		func = item->func;
		(item->object->*func)(server, reason_code,
				item->client_data, call_data);
	}
}
