/*		copyright	"%c%" 	*/

#ident	"@(#)err.c	1.2"
#ident  "$Header$"
#include <stdio.h>
#include <locale.h>
#include <pfmt.h>
#include "err.h"

void	exit();

/*
** Function: tfm_report()
**
** This function handles printing of errors in international format.
** The error message text is taken from the message buffer which is
** set up by the failing routine.  The severity is set by the failing
** routine.
*/

void
tfm_report(msgbuf)
struct	msg	*msgbuf;
{
	(void)pfmt(stderr,msgbuf->sev,msgbuf->text,msgbuf->args[0],
							msgbuf->args[1],
							msgbuf->args[2],
							msgbuf->args[3]);
	if(msgbuf->act == ERR_QUIT){
		exit(1);
	}
	msgbuf->sev = ERR_NONE;
	msgbuf->act = ERR_CONTINUE;
	msgbuf->text[0] = 0;
}

