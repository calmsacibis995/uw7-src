#ident	"@(#)debugger:libint/common/Mformat.C	1.7"

#include	"UIutil.h"
#include	"Msgtab.h"
#include	"Message.h"

#include	<stdio.h>
#include	<string.h>

// Message::format unbundles the message data and sprintfs the message
// using the format string from the Message Table.  This is in a separate
// file from the rest of the Message member functions so that it is not
// automatically pulled into the debugger executable, which doesn't
// need this capability (or at least not the full blown version with
// all the Signatures)

char *
Message::format()
{
	static char		*buf = 0;
	static size_t		max_len = 0;
	static const char	*fname = "Message::format";

	char		*s1 = 0;
	char		*s2 = 0;
	char		*s3 = 0;
	char		*s4 = 0;
	char		*s5 = 0;
	char		*s6 = 0;
	char		*s7 = 0;
	char		*s8 = 0;
#ifdef SDE_SUPPORT
	char		*s10 = 0;
	char		*s11 = 0;
	Word		i9;
#endif

	Word i1, i2, i3, i4, i5;
	const char		*fmt;
	size_t			len;
	int			i = 0;

#ifndef NOCHECKS
	if ((msg_state != MSTATE_received)
		&& (msg_state != MSTATE_ready_to_send)
		&& (msg_state != MSTATE_sent))
		interface_error(fname, __LINE__);
#endif // NOCHECKS

	fmt = Mtable.format(msg_id);
	if (fmt)
	{
		len = strlen(fmt) + msg_length; // length of final string
		if (len >= max_len)
		{
			max_len = len + 64;	// alloc extra to avoid doing it
						// again soon (maybe)
			delete buf;
			buf = new char[max_len+1];
		}
	}
	else if (!buf)
	{
		// SIG_none with null content gets a buffer
		// also so a "" can be returned, i.e. we
		// make sure that format() never returns NULL
		max_len = 64;
		buf = new char[max_len+1];
	}

	switch(Mtable.signature(msg_id))
	{
		case SIG_last:
		case SIG_invalid:
		default:
			interface_error(fname, __LINE__);
			break;

		case SIG_none:
			if (fmt)
				strcpy(buf, fmt);
			else
				buf[0] = '\0';
			break;

		// Mformat.h includes the rest of the machine-generated cases
		#include "Mformat.h"
	}

	if (i > max_len)
		interface_error(fname, __LINE__, 1);
	return buf;
}
