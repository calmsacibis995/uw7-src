#ident	"@(#)sccs:cmd/help.c	1.7"
#include	<stdio.h>
#include	<sys/types.h>
#include	<macros.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<sys/euc.h>
#include	<limits.h>
#include	<strings.h>

#ifndef	DEFAULT_HELPDIR
#define	DEFAULT_HELPDIR	"/usr/ccs/lib/help/"
#endif

extern	int	errno;

main(argc,argv)
int argc;
char *argv[];
{
	char	Ohelpcmd[PATH_MAX+1];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxepu");
	(void)setlabel("UX:help");

	(void)strcpy(Ohelpcmd, DEFAULT_HELPDIR);
	(void)strcat(Ohelpcmd, "lib/help2");

	execv(Ohelpcmd,argv);
	pfmt(stderr,MM_ERROR,
		":100:Could not exec: %s.  Errno=%d\n",Ohelpcmd,errno);
	exit(1);
}
