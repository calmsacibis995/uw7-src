#ident	"@(#)ksh93:src/lib/libast/misc/sigcrit.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * signal critical region support
 */

#include <ast.h>
#include <sig.h>

static int	signals[] =		/* held inside critical region	*/
{
	SIGINT,
#ifdef SIGQUIT
	SIGQUIT,
#endif
#ifdef SIGHUP
	SIGHUP,
#endif
};

#ifndef SIG_SETMASK
#undef	_lib_sigprocmask
#endif

#if !_lib_sigprocmask && !_lib_sigsetmask

static long	hold;			/* held signal mask		*/

/*
 * hold last signal for later delivery
 */

static void
interrupt(int sig)
{
	signal(sig, interrupt);
	hold |= sigmask(sig);
}

#endif

/*
 * critical signal region handler
 *
 * op>0		push region, return region level
 * op==0	pop region, return region level
 * op<0		return non-zero if any signals held in current region
 *
 * signals[] held until region popped
 */

int
sigcritical(int op)
{
	register int		i;
	static int		level;
#if _lib_sigprocmask
	static sigset_t		mask;
	sigset_t		nmask;
#else
#if _lib_sigsetmask
	static long		mask;
#else
	static Handler_t	handler[elementsof(signals)];
#endif
#endif

	if (op > 0)
	{
		if (!level++)
		{
#if _lib_sigprocmask
			sigemptyset(&nmask);
			for (i = 0; i < elementsof(signals); i++)
				sigaddset(&nmask, signals[i]);
			sigprocmask(SIG_BLOCK, &nmask, &mask);
#else
#if _lib_sigsetmask
			mask = 0;
			for (i = 0; i < elementsof(signals); i++)
				mask |= sigmask(signals[i]);
			mask = sigblock(mask);
#else
			hold = 0;
			for (i = 0; i < elementsof(signals); i++)
				if ((handler[i] = signal(signals[i], interrupt)) == SIG_IGN)
				{
					signal(signals[i], SIG_IGN);
					hold &= ~sigmask(signals[i]);
				}
#endif
#endif
		}
		return(level);
	}
	else if (!op)
	{
		/*
		 * a vfork() may have intervened so we
		 * allow apparent nesting mismatches
		 */

		if (--level <= 0)
		{
			level = 0;
#if _lib_sigprocmask
			sigprocmask(SIG_SETMASK, &mask, NiL);
#else
#if _lib_sigsetmask
			sigsetmask(mask);
#else
			for (i = 0; i < elementsof(signals); i++)
				signal(signals[i], handler[i]);
			if (hold)
			{
				for (i = 0; i < elementsof(signals); i++)
					if (hold & sigmask(signals[i]))
						kill(getpid(), signals[i]);
				pause();
			}
#endif
#endif
		}
		return(level);
	}
	else
	{
#if _lib_sigprocmask
		sigpending(&nmask);
		for (i = 0; i < elementsof(signals); i++)
			if (sigismember(&nmask, signals[i]))
				return(1);
		return(0);
#else
#if _lib_sigsetmask
		/* no way to get pending signals without installing handler */
		return(0);
#else
		return(hold != 0);
#endif
#endif
	}
}
