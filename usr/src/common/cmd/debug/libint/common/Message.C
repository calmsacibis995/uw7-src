#ident	"@(#)debugger:libint/common/Message.C	1.3"

#include	"UIutil.h"
#include	"Message.h"
#include	"Transport.h"
#include	"Msgtab.h"
#include	"Msgtypes.h"
#include	"Severity.h"

#include	<string.h>

Message&
Message::operator=(Message &m)
{
	transport_type = m.transport_type;
	sequence_num = m.sequence_num;
	dbcontext = m.dbcontext;
	uicontext = m.uicontext;
	msg_id = m.msg_id;
	msg_state = m.msg_state;
	msg_severity = m.msg_severity;
	msg_length = m.msg_length;
	msg_flags = m.msg_flags;
	if (msg_length)
	{
		msg_data = new char[msg_length];
		memcpy(msg_data, m.msg_data, msg_length);
	}
	else
		msg_data = 0;

	return *this;
}

Message::Message(const Message &m)
{
	transport_type = m.transport_type;
	sequence_num = m.sequence_num;
	dbcontext = m.dbcontext;
	uicontext = m.uicontext;
	msg_id = m.msg_id;
	msg_state = m.msg_state;
	msg_severity = m.msg_severity;
	msg_length = m.msg_length;
	msg_flags = m.msg_flags;
	if (msg_length)
	{
		msg_data = new char[msg_length];
		memcpy(msg_data, m.msg_data, msg_length);
	}
	else
		msg_data = 0;
}

void
Message::clear()
{
	delete msg_data;
	memset(this, 0, sizeof(Message));
}

#ifndef NOCHECKS
// versions of the access functions with error checking
// if the error checking is turned off, the access functions are inlined

DBcontext
Message::get_dbcontext()
{
	if ((msg_state != MSTATE_sent) && (msg_state != MSTATE_received))
		interface_error("Message::get_dbcontext", __LINE__);
	return dbcontext;
}

UIcontext
Message::get_uicontext()
{
	if ((msg_state != MSTATE_sent) && (msg_state != MSTATE_received))
		interface_error("Message::get_uicontext", __LINE__);
	return uicontext;
}

Severity
Message::get_severity()
{
	if ((msg_state != MSTATE_sent
		&& msg_state != MSTATE_received
		&& msg_state != MSTATE_ready_to_send)
		|| (Mtable.msg_class(msg_id) != MSGCL_error)
		|| (msg_severity < E_NONE)
		|| (msg_severity > E_FATAL))
		interface_error("Message::get_severity", __LINE__);
	return (Severity) msg_severity;
}

Msg_id
Message::get_msg_id()
{
	if ((msg_state != MSTATE_sent)
		&& (msg_state != MSTATE_ready_to_send)
		&& (msg_state != MSTATE_received))
		interface_error("Message::get_msg_id", __LINE__);
	return msg_id;
}

Transport_type
Message::get_transport_type()
{
	if ((msg_state != MSTATE_sent)
		&& (msg_state != MSTATE_received))
		interface_error("Message::get_transport_type", __LINE__);
	return transport_type;
}

size_t
Message::get_msg_length()
{
	if ((msg_state != MSTATE_sent)
		&& (msg_state != MSTATE_ready_to_send)
		&& (msg_state != MSTATE_received))
		interface_error("Message::get_msg_length", __LINE__);
	return msg_length;
}

#endif // NOCHECKS

#define	SET(n)		(1 << (n))
#define IS_SET(n,v)	((1 << (n)) & (v))

// Sigtable is used to make bundle table driven instead of having
// a switch with a case for each Signature.
// Each bit in argtypes represents one argument.  The only argument types
// supported now are strings (char *) and integral (Word).
// If the bit is set, the argument is a string, if not set, integral.

struct Sigtable
{
	int		nargs;
	unsigned long	argtypes;
};

static const Sigtable sigtable[] =
{
	{0},					// SIG_invalid
	{0},					// SIG_none
	#include "Sigtable.h"
	{0},	//SIG_last
};

void
Message::bundle(Msg_id mid, Severity sev, va_list ap)
{
	int		nargs;
	register int	argtypes;
	char		*ptr;
	int		i;

	Signature	signature = Mtable.signature(mid);
	va_list		ap2 = ap;

#ifndef NOCHECKS
	if ((msg_state < MSTATE_empty)
		|| (msg_state > MSTATE_received)
		|| (signature <= SIG_invalid) 
		|| (signature >= SIG_last)
		|| (Mtable.msg_class(mid) != MSGCL_error && sev > E_NONE))
		interface_error("Message::bundle", __LINE__);
#endif // NOCHECKS

	msg_id = mid;
	msg_length = 0;
	msg_state = MSTATE_ready_to_send;
	msg_severity = sev;
	if (signature == SIG_none)
		return;

	nargs = sigtable[signature].nargs;
	argtypes = (int)sigtable[signature].argtypes;

	// calculate the space needed for the message data
	for (i = 1; i <= nargs; ++i)
	{
		if (IS_SET(i, argtypes))
		{
			msg_length += strlen(va_arg(ap, char *)) + 1;
		}
		else
		{
			msg_length += sizeof(Word);
			(void) va_arg(ap, Word);
		}
	}
	
	delete msg_data;
	ptr = new char[msg_length];
	msg_data = ptr;
	
	// loop through the argument list, packing the data
	for (i = 1; i <= nargs; ++i)
	{
		if (IS_SET(i, argtypes))
		{
			strcpy(ptr, va_arg(ap2, char *));
			ptr += strlen(ptr) + 1;
		}
		else
		{
			memcpy(ptr, &va_arg(ap2, Word), sizeof(Word));
			ptr += sizeof(Word);
		}
	}
}

// retrieve one null-terminated string from the message data

void
Message::pick_string(char *&ptr, char *&buf)
{
	buf = ptr;
	ptr += strlen(ptr) + 1;
}

// retrieve an integral value from the message data
// note that this assumes the same byte ordering on both sides
// if the ui and debug were running on machines with different
// byte orders, this and the transport layer would have to deal
// with that problem

void
Message::pick_word(char *&ptr, Word &retval)
{
	union
	{
		Word	val;
		char	c[sizeof(Word)];
	} u;

	for (int i = 0; i < sizeof(Word); i++)
		u.c[i] = *ptr++;
	retval = u.val;
}
