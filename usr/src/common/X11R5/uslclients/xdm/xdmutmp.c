#ident	"@(#)xdm:xdmutmp.c	1.4"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/***************************************************************

		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#include	<unistd.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<string.h>
#include	<memory.h>
#include	<utmpx.h>
#include	<priv.h>
#include	<pfmt.h>
#include	"sac.h"

extern	char	Scratch[];
extern	void	log();
extern	time_t	time();

char	*lastname ();

/*
 * Procedure:	  account
 *
 * Restrictions:
                 makeut: none
*/

/*
 * account - create a utmp record for service
 *
 */

int
account(line)
char	*line;
{
	struct utmpx utmp;			/* prototype utmp entry */
	register struct utmpx *up = &utmp;	/* and a pointer to it */
	extern	struct	utmpx *makeutx();

	Debug("account\n");
	(void) memset(up, '\0', sizeof(utmp));
	(void)strncpy(up->ut_user, "XDM", 3);
	(void)strncpy(up->ut_line, lastname(line), sizeof(up->ut_line));
	up->ut_pid = getpid ();
	Debug ("ut_pid = %d\n", up->ut_pid);
	up->ut_type = LOGIN_PROCESS;
	up->ut_id[0] = 't';
	up->ut_id[1] = 'm';
	up->ut_id[2] = SC_WILDC;
	up->ut_id[3] = SC_WILDC;
	up->ut_exit.e_termination = 0;
	up->ut_exit.e_exit = 0;
	(void)time(&up->ut_tv.tv_sec);
	if (makeutx(up) == NULL) {
		Debug ("makeutx for pid %d failed\n", up->ut_pid);
		return(-1);
	}
	Debug ("makeutx for pid %d \n", up->ut_pid);
	return(0);
}

/*
 * Procedure:     cleanut                                                        *
 * Restrictions:
                 modut: none
                 getutent: none
*/

int
cleanut(pid,status)
pid_t	pid;
int	status;
{
	register struct utmpx *up;
	extern	struct utmpx *modutx();

	Debug("cleanut pid=%d\n",pid);
	setutxent();
	while (up = getutxent()) {
		if (up->ut_pid != (o_pid_t)pid)
			continue;
		up->ut_type = DEAD_PROCESS;
		up->ut_exit.e_termination = (status & 0xff);
		up->ut_exit.e_exit = ((status >> 8) & 0xff);
		(void)time(&up->ut_tv.tv_sec);
		if (modutx(up) == NULL) {
			Debug ("cleanut: Modify utmpx failed\n");
			return 1;
		}
		break;
	}
	endutxent();
	return 0;
}

/*
 *	lastname	- If the path name starts with "/dev/",
 *			  return the rest of the string.
 *			- Otherwise, return the last token of the path name
 */
char	*
lastname(name)
char	*name;
{
	char	*sp, *p;
	sp = name;
	if (strncmp(sp, "/dev/", 5) == 0)
		sp += 5;
	else
		while ((p = (char *)strchr(sp,'/')) != (char *)NULL) {
			sp = ++p;
		}
	if (name) Debug("lastname name=%s\n",name);
	return(sp);
}
