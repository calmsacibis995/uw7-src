#ident	"@(#)ksh93:src/lib/libast/comp/sigunblock.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_sigunblock

NoN(sigunblock)

#else

#include <sig.h>

#ifndef SIG_UNBLOCK
#undef	_lib_sigprocmask
#endif

int
sigunblock(int s)
{
#if _lib_sigprocmask
	int		op;
	sigset_t	mask;

	sigemptyset(&mask);
	if (s)
	{
		sigaddset(&mask, s);
		op = SIG_UNBLOCK;
	}
	else op = SIG_SETMASK;
	return(sigprocmask(op, &mask, NiL));
#else
#if _lib_sigsetmask
	return(sigsetmask(s ? (sigsetmask(0L) & ~sigmask(s)) : 0L));
#else
	NoP(s);
	return(0);
#endif
#endif
}

#endif
