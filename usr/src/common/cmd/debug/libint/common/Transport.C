#ident	"@(#)debugger:libint/common/Transport.C	1.7"

#include	"Message.h"
#include	"Transport.h"
#include	"UIutil.h"
#include	<unistd.h>
#include	<errno.h>

// initialize the transport layer
// readf and writef are the functions used to get and put the messages,
// called readf and writef because they currently read from and write
// to pipes - different functions could be used for a different
// transport mechanism, like shared memory.  Note that on the debug
// side, it uses debug_read, instead of simple read(2), because it
// must be able to handle interrupts while getting messages
// handler is set from the UI side, since the UI must be able to
// respond to a DE query at any time, even while waiting for a response
// to a query of its own.  On the debug side handler is zero.

Transport::Transport(int in, int out,
	int (*readf)(int, void *, unsigned int),
	int (*writef)(int, const void *, unsigned int),
	void (*exitf)(int),
	void (*handler)(Message *))
{
	which_side = handler ? ts_gui : ts_debug;
	last_sent = 0;
	last_received = 0;
	input = in;
	output = out;
	in_query = 0;
	readfunc = readf;
	writefunc = writef;
	exitfunc = exitf;
	qhandler = handler;
}

void
Transport::send_message(Message *m, Transport_type tt, DBcontext db_ctxt,
	UIcontext ui_ctxt)
{
#ifndef NOCHECKS
	static const char	*fname = "Transport::send_message";

	if (!m || ((m->msg_state != MSTATE_ready_to_send)
		&& (m->msg_state != MSTATE_sent)))
		interface_error(fname, __LINE__, 1);
	if (which_side == ts_gui)
	{
		if ((tt < TT_UI_user_cmd) || (tt > TT_UI_response))
			interface_error(fname, __LINE__, 1);
	}
	else
	{
		if ((tt < TT_DE_notify) || (tt > TT_DE_query))
			interface_error(fname, __LINE__, 1);
	}
#endif // NOCHECKS

	if (tt == TT_UI_query || tt == TT_DE_query)
	{
#ifndef NOCHECKS
		if (in_query)
			interface_error(fname, __LINE__, 1);
#endif // NOCHECKS
		in_query = 1;
	}
	m->transport_type = tt;
	m->dbcontext = db_ctxt;
	m->uicontext = ui_ctxt;
	m->sequence_num = ++last_sent;

	// sizeof(Message) - sizeof(void *) avoids writing the pointer
	// to the data, which would be meaningless in the other process
	if ((*writefunc)(output, m, sizeof(Message) - sizeof(void *))
		!= (sizeof(Message) - sizeof(void *)))
	{
		if (errno == EPIPE)	// other side exited without sending quit message
			(*exitfunc)(0);
		else
			interface_error(fname, __LINE__, 1);
	}

	if (m->msg_length)
		if ((*writefunc)(output, m->msg_data, m->msg_length)
			!= m->msg_length)
		{
			if (errno == EPIPE)
				(*exitfunc)(0);
			else
				interface_error(fname, __LINE__, 1);
		}

	m->msg_state = MSTATE_sent;
}

// read from transport pipe. note:
// 1. need to iterate because reading from a pipe might return
//    with less than what was requested
// 2. the read function can be interrupted, e.g. by a signal
// 3. the read function returns 0 if the other end of the pipe
//    went away
static int
transport_read(int fd, char *ptr, int len, 
			int (*readf)(int, void *, unsigned int))
{
	int	bytes_read;
	int	bytes_needed = len;

	bytes_read = 0;
	while (bytes_read < bytes_needed)
	{
		int	i;

		do
		{
			i = (*readf)(fd, ptr, len);
		}
		while (i < 0 && (errno == EAGAIN || errno == EINTR));

		if (i <= 0)
			// EOF or some other error condition
			return i;
		len -= i;
		ptr += i;
		bytes_read += i;
	}
	return bytes_read;
}

void
Transport::read_message(Message *m)
{
	static const char	*fname = "Transport::read_message";

#ifndef NOCHECKS
	if (!m)
		interface_error(fname, __LINE__, 1);
#endif // NOCHECKS

	int bytes_read = transport_read(input, (char *)m, sizeof(Message) - sizeof(void *), readfunc);
	if (bytes_read == 0)	// other side exited without sending quit message
		(*exitfunc)(0);

	else if (bytes_read != (sizeof(Message) - sizeof(void *)))
		interface_error(fname, __LINE__, 1);
	if (m->msg_length)
	{
		m->msg_data = new char[m->msg_length];

		bytes_read = transport_read(input, (char *)m->msg_data, m->msg_length,
			readfunc);
		if (bytes_read < m->msg_length)
			interface_error(fname, __LINE__, 1);
	}
	else
		m->msg_data = 0;

#ifndef NOCHECKS
	if (which_side == ts_debug)
	{
		if ((m->transport_type < TT_UI_user_cmd)
			|| (m->transport_type > TT_UI_response))
			interface_error(fname, __LINE__, 1);
	}
	else
	{
		if ((m->transport_type < TT_DE_notify)
			|| (m->transport_type > TT_DE_query))
			interface_error(fname, __LINE__, 1);
	}
	if (m->sequence_num != ++last_received)
		interface_error(fname, __LINE__, 1);
#endif // NOCHECKS

	m->msg_state = MSTATE_received;
}

// get the next available message of any type
// check the queue first - some messages, like user commands, are
// skipped while handling queries or waiting for a sync response

int
Transport::get_next_message(Message *m,
			int (*readf)(int, void *, unsigned int))
{
	Message	*m2;

#ifndef NOCHECKS
	if (!m)
		interface_error("Transport::get_next_message", __LINE__, 1);
#endif // NOCHECKS

	readfunc = readf;
	in_query = 0;
	if (inq.isempty())
	{
		read_message(m);
		if (m->transport_type == TT_DE_query && qhandler)
		{
			(*qhandler)(m);
			return 0;
		}
	}
	else
	{
		m2 = (Message *)inq.first();
		inq.remove(m2);
		*m = *m2;
		m2->clear();
		freeq.add(m2);

#ifndef NOCHECKS
		// should never have gotten queued
		if (m->transport_type == TT_UI_query
			|| m->transport_type == TT_DE_query
			|| m->transport_type == TT_UI_response
			|| m->transport_type == TT_DE_response)
			interface_error("Transport::get_next_message",__LINE__, 1);
#endif // NOCHECKS
	}
	return 1;
}

// The debugger or the ui has sent a query and is waiting for a response
// Any other messages that come in are queued to be handled later
// Don't bother searching through the list of messages already waiting,
// since queries and responses are handled as they are read and should
// never get queued

void
Transport::get_response(Message *m,
			int (*readf)(int, void *, unsigned int))
{
	Message	*m2;

#ifndef NOCHECKS
	if (!m || !in_query)
		interface_error("Transport::get_next_message", __LINE__, 1);
#endif // NOCHECKS

	readfunc = readf;
	for (;;)
	{
		if (freeq.isempty())
			m2 = new Message;
		else
		{
			m2 = (Message *)freeq.first();
			freeq.remove(m2);
		}
		read_message(m2);

		if ((m2->transport_type == TT_DE_response)
			|| (m2->transport_type == TT_UI_response))
			break;

		if (qhandler && (m2->transport_type == TT_DE_query))
		{
			(*qhandler)(m2);
			m2->clear();
			freeq.add(m2);
			continue;
		}
		else
			inq.add(m2);
	}

	*m = *m2;
	m2->clear();
	freeq.add(m2);
}

// get the next message - off the queue or reading from the pipe if
// the queue is empty - that is anything other than a user command

void
Transport::get_nonuser_message(Message *m,
			int (*readf)(int, void *, unsigned int))
{
	Message	*m2;

#ifndef NOCHECKS
	if (!m)
		interface_error("Transport::get_nonuser_message", __LINE__, 1);
#endif // NOCHECKS

	readfunc = readf;
	in_query = 0;
	for (m2 = (Message *)inq.first(); m2; m2 = (Message *)inq.next())
	{
		if (m2->transport_type != TT_UI_user_cmd)
		{
#ifndef NOCHECKS
			// should never have gotten queued
			if (m2->transport_type == TT_UI_query
				|| m2->transport_type == TT_DE_query
				|| m2->transport_type == TT_UI_response
				|| m2->transport_type == TT_DE_response)
				interface_error("Transport::get_nonuser_message", __LINE__, 1);
#endif // NOCHECKS

			inq.remove(m2);
			*m = *m2;
			m2->clear();
			freeq.add(m2);
			return;
		}
	}

	for (;;)
	{
		if (freeq.isempty())
			m2 = new Message;
		else
		{
			m2 = (Message *)freeq.first();
			freeq.remove(m2);
		}
		read_message(m2);

		if (m2->transport_type != TT_UI_user_cmd)
			break;
		inq.add(m2);
	}

	*m = *m2;
	m2->clear();
	freeq.add(m2);
}
