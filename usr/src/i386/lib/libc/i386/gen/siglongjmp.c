#ident	"@(#)libc-i386:gen/siglongjmp.c	1.3"

#ifndef GEMINI_ON_OSR5
#ifdef __STDC__
	#pragma weak siglongjmp = _siglongjmp
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/ucontext.h>

#include <setjmp.h>

void 
siglongjmp(env,val)
sigjmp_buf env;
int val;
{
	register ucontext_t *ucp = (ucontext_t *)env;
	if (val)
		ucp->uc_mcontext.gregs[ EAX ] = val;
	asm("cld");
	setcontext(ucp);
}
#endif
