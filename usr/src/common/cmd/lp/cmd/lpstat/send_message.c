/*		copyright	"%c%" 	*/

#ident	"@(#)send_message.c	1.2"
#ident	"$Header$"

#include "stdio.h"

#if	defined(__STDC__)
#include "stdarg.h"
#else
#include "varargs.h"
#endif

#include "msgs.h"

#define	WHO_AM_I	I_AM_LPSTAT
#include "oam.h"

#include "lpstat.h"


/**
 ** send_message() - HANDLE MESSAGE SENDING TO SPOOLER
 **/

/*VARARGS1*/
void
#if	defined(__STDC__)
send_message (
	int			type,
	...
)
#else
send_message (type, va_alist)
	int			type;
	va_dcl
#endif
{
	va_list			ap;

	int			n;

	char			msgbuf[MSGMAX];


	if (!scheduler_active)
		return;

#if	defined(__STDC__)
	va_start (ap, type);
#else
	va_start (ap);
#endif

	(void)_putmessage (msgbuf, type, ap);

	va_end (ap);

	if (msend(msgbuf) == -1) {
		LP_ERRMSG (ERROR, E_LP_MSEND);
		done(1);
	}

	return;
}
