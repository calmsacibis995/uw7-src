/*
 * Copyright 1990, 1991, 1992 by the Massachusetts Institute of Technology and
 * UniSoft Group Limited.
 * 
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the names of MIT and UniSoft not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  MIT and UniSoft
 * make no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * $XConsortium: signals.c,v 1.2 92/07/01 11:59:11 rws Exp $
 */
#include <signal.h>
#include <errno.h>

extern int errno;

sigemptyset(set)
sigset_t *set;
{
	*set = 0;
	return 0;
}

sigfillset(set)
sigset_t *set;
{
	*set = 0xffffffff;
	return 0;
}

sigaddset(set, sig)
sigset_t *set;
int sig;
{
	if (sig < 1 || sig >= NSIG)
	{
		errno = EINVAL;
		return -1;
	}
	*set |= (1 << (sig-1));
	return 0;
}

sigdelset(set, sig)
sigset_t *set;
int sig;
{
	if (sig < 1 || sig >= NSIG)
	{
		errno = EINVAL;
		return -1;
	}
	*set &= ~(1 << (sig-1));
	return 0;
}

sigismember(set, sig)
sigset_t *set;
int sig;
{
	if (sig < 1 || sig >= NSIG)
	{
		errno = EINVAL;
		return -1;
	}
	return ((*set & (1 << (sig-1))) != 0);
}

sigaction(sig, act, oact)
int sig;
struct sigaction *act, *oact;
{
	int ret;

	/*
	 * This is what we do for now: zero sa_flags so that the sigaction
	 * structures can be passed to th BSD sigvec() as if they
	 * were sigvec structures.
	 */

#if 1
	/* Put in a trap so we know when assumption becomes wrong */
	if (act && act->sa_flags != 0) {
		printf("sa_flags != 0\n");
		exit(123);
	}
#endif

	if (act)
		act->sa_flags = 0;
	ret = sigvec(sig, act, oact);
	if (oact)
		oact->sa_flags = 0;

	return ret;
}

sigprocmask(how, set, oset)
int how;
sigset_t *set, *oset;
{
	sigset_t omask;

	if (set != 0)
		switch (how) {
		case SIG_BLOCK :
			omask = sigblock(*set);
			break;
		case SIG_UNBLOCK :
			omask = sigblock((sigset_t)0);
			omask = sigsetmask(omask & ~*set);
			break;
		case SIG_SETMASK :
			omask = sigsetmask(*set);
			break;
		default :
			errno = EINVAL;
			return -1;
		}
	else
		omask = sigblock((sigset_t)0);

	if (oset != 0)
		*oset = omask;
	
	return 0;
}

/* ARGSUSED */
sigpending(set)
sigset_t *set;
{
#ifdef ENOSYS
	errno = ENOSYS;
#else
	/*
	 * BSD4.2 doesn't have a suitable eqivalent to ENOSYS.
	 */
	errno = EINVAL;
#endif
	return -1;
}

sigsuspend(mask)
sigset_t *mask;
{
	(void) sigpause(*mask);
	return -1;
}
