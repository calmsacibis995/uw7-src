#ident	"@(#)ksh93:src/lib/libast/obsolete/strsignal.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use fmtsignal */

#include <ast.h>

char*
strsignal(int sig)
{
	return(fmtsignal(sig));
}
