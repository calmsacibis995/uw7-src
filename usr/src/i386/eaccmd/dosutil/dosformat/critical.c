/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/critical.c	1.1.1.2"
#ident  "$Header$"

/* #define		DEBUG		1	/* */

#include	<signal.h>

#include	<stdio.h>

void
critical(on)
int	on;
{
	(void) signal(SIGHUP, on ? SIG_IGN : SIG_DFL);
	(void) signal(SIGINT, on ? SIG_IGN : SIG_DFL);
	(void) signal(SIGQUIT, on ? SIG_IGN : SIG_DFL);

#ifdef DEBUG
	(void) fprintf(stderr, "critical(): DEBUG - Critical code is %s\n", on ? "ON" : "OFF");
#endif
}
