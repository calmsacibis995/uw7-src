#ident	"@(#)ksh93:src/lib/libast/string/fmtsignal.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * if sig>=0 then return signal text for signal sig
 * otherwise return signal name for signal -sig
 */

#include <ast.h>
#include <sig.h>

char*
fmtsignal(register int sig)
{
	static char	buf[20];

	if (sig >= 0)
	{
		if (sig <= sig_info.sigmax)
			return(sig_info.text[sig]);
		sfsprintf(buf, sizeof(buf), "Signal %d", sig);
	}
	else
	{
		sig = -sig;
		if (sig <= sig_info.sigmax)
			return(sig_info.name[sig]);
		sfsprintf(buf, sizeof(buf), "%d", sig);
	}
	return(buf);
}
