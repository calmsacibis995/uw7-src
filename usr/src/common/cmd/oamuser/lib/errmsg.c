#ident	"@(#)errmsg.c	1.2"
#ident  "$Header$"

#include	<stdio.h>
#include	<varargs.h>
#include	<pfmt.h>

extern	char	*errmsgs[];
extern	int	lasterrmsg;
extern	char	*msg_label;

/*
	synopsis: errmsg( msgid, (arg1, ..., argN) )
*/

/*VARARGS*/
void
errmsg(va_alist)
va_dcl
{
	va_list	args;
	int	msgid;

	setlabel(msg_label);

	va_start(args);

	msgid = va_arg(args, int);

	if (msgid >= 0 && msgid < lasterrmsg) {
		(void) vpfmt(stderr, MM_ERROR, errmsgs[msgid], args);
	}

	va_end(args);
}
