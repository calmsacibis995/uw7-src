#ident	"@(#)ksh93:src/lib/libast/misc/signal.c	1.1"
#pragma prototyped

/*
 * signal that disables syscall restart on interrupt
 * also clears signal mask
 */

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide signal
#else
#define signal		______signal
#endif

#include <ast.h>
#include <sig.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide signal
#else
#undef	signal
#endif

#if !_std_signal && (_lib_sigaction && defined(SA_NOCLDSTOP) || _lib_sigvec && defined(SV_INTERRUPT))

#if !defined(SA_NOCLDSTOP) || !defined(SA_INTERRUPT) && defined(SV_INTERRUPT)
#define SA_INTERRUPT	SV_INTERRUPT
#define sigaction	sigvec
#define sigemptyset(p)	(*(p)=0)
#define sa_flags	sv_flags
#define sa_handler	sv_handler
#define	sa_mask		sv_mask
#endif

Handler_t
signal(int sig, Handler_t fun)
{
	struct sigaction	na;
	struct sigaction	oa;

	memzero(&na, sizeof(na));
	na.sa_handler = fun;
#if defined(SA_INTERRUPT) || defined(SA_RESTART)
	switch (sig)
	{
#if defined(SIGIO) || defined(SIGTSTP) || defined(SIGTTIN) || defined(SIGTTOU)
#if defined(SIGIO)
	case SIGIO:
#endif
#if defined(SIGTSTP)
	case SIGTSTP:
#endif
#if defined(SIGTTIN)
	case SIGTTIN:
#endif
#if defined(SIGTTOU)
	case SIGTTOU:
#endif
#if defined(SA_RESTART)
		na.sa_flags = SA_RESTART;
#endif
		break;
#endif
	default:
#if defined(SA_INTERRUPT)
		na.sa_flags = SA_INTERRUPT;
#endif
		break;
	}
#endif
	return(sigaction(sig, &na, &oa) ? (Handler_t)0 : oa.sa_handler);
}

#else

NoN(signal)

#endif
