#ident	"@(#)kern-i386:util/kdb/scodb/quit.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History:
 *
 *	L000	scol!jamesh	25 Nov 1993
 *		- re-enable the Watchdog timer when exiting
 *	L001	nadeem@sco.com	13dec94
 *	- support for user level scodb.  Upon quitting, restore the tty
 *	  setting and close the dump file.
 */


#include	"sys/reg.h"

#include	"dbg.h"
#include	"sent.h"

#ifndef USER_LEVEL						/* L001 */

/*
*	quit debugger command
*/
NOTSTATIC
c_quit(c, v)
	int c;
	char **v;
{
	long seg, off;
	extern int *REGP, MODE;

	if (c != 1) {
		seg = REGP[T_CS];
		if (!getaddrv(v + 1, &seg, &off)) {
			perr();
			return DB_ERROR;
		}
		REGP[T_CS] = seg;
		REGP[T_EIP] = off;
	}
	MODE = KERNEL;
	return DB_RETURN;
}

#else								/* L001v */

NOTSTATIC
c_quit(c, v)
int c;
char **v;
{
	extern int MODE;

	if (c == 1) {		/* real quit */
		MODE = KERNEL;
		return(DB_RETURN);
	} else {
		MODE = QUIT;
		retty();		/* restore tty settings */
		return(DB_RETURN);
	}
}

#endif								/* L001^ */
