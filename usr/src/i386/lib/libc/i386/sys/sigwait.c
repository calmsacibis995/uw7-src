#ident	"@(#)libc-i386:sys/sigwait.c	1.1"

#ifdef __STDC__
	#pragma weak sigwait = _sigwait
#endif

#include "synonyms.h"
#include <sys/types.h>
#include <signal.h>
#include <siginfo.h>
#include <time.h>

int
sigwait(sigset_t *set)
{
	return __sigwait(set, (siginfo_t *)0, (struct timespec *)0);
}
