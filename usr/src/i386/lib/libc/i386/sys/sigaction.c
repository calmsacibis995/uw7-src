#ident	"@(#)libc-i386:sys/sigaction.c	1.8"

#ifndef GEMINI_ON_OSR5

#ifdef __STDC__ 
	#pragma weak sigaction = _sigaction
#endif
#include "synonyms.h"
#include <signal.h>
#include <errno.h>
#include <siginfo.h>
#include <sys/user.h>
#include <ucontext.h>


void
_sigacthandler(int sig, siginfo_t *sip, ucontext_t *ucp, void (*handler)())
{
	(*handler)(sig, sip, ucp);
	setcontext(ucp);
	/*
	 * We have no context to return to.
	 */
	_exit(-1);
}

int
sigaction(int sig, const struct sigaction *nactp, struct sigaction *oactp)
{
	struct sigaction tact;

	if (nactp != 0) {
		tact = *nactp;			/* structure assignment */
		tact.sa_flags |= SA_NSIGACT;
		nactp = &tact;
	}

	return __sigaction(sig, nactp, oactp, _sigacthandler);
}
#endif
