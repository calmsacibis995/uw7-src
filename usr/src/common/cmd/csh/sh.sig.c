/*		copyright	"%c%" 	*/

#ident	"@(#)csh:common/cmd/csh/sh.sig.c	1.2.2.3"
#ident  "$Header$"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * C shell - old jobs library sigrelse meant unblock mask
 *	     AND reinstall handler, so we simulate it here.
 */
#include <signal.h>

#define	mask(s)	(1 << ((s)-1))

static	void (*actions[NSIG])();
static	int achanged[NSIG];

/*
 * Perform action and save handler state.
 */
sigset(s, a)
	int s;
	void (*a)();
{

	actions[s] = a;
	achanged[s] = 0;
	return ((int)signal(s, a));
}

/*
 * Release any masking of signal and
 * reinstall handler in case someone's
 * done a sigignore.
 */
sigrelse(s)
	int s;
{

	if (achanged[s]) {
		signal(s, actions[s]);
		achanged[s] = 0;
	}
	sigsetmask(sigblock(0) &~ mask(s));
}

/*
 * Ignore signal but maintain state so sigrelse
 * will restore handler.  We avoid the overhead
 * of doing a signal for each sigrelse call by
 * marking the signal's action as changed.
 */
sigignore(s)
	int s;
{

	if (actions[s] != SIG_IGN)
		achanged[s] = 1;
	signal(s, SIG_IGN);
}
