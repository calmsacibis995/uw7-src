/*		copyright	"%c%" 	*/

#ident	"@(#)startup.c	1.2"
#ident  "$Header$"

#include "stdio.h"

#include "lp.h"
#include "msgs.h"

#include "lpadmin.h"


/*
 * Procedure:     startup
 *
 * Restrictions:
 *               mopen: None
 * notes - OPEN CHANNEL TO SPOOLER
*/

void			startup ()
{
	trap_signals ();

	if (mopen() == -1)
		scheduler_active = 0;
	else
		scheduler_active = 1;

	return;
}
