/*		copyright	"%c%" 	*/

#ident	"@(#)sleep:sleep.c	1.3.5.1"
/*
**	sleep -- suspend execution for an interval
**
**		sleep time
*/

#include	<stdio.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<unistd.h>

main(argc, argv)
char **argv;
{
	long  	n;
	char	*s;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:sleep");

	n = 0;
	if(argc >= 2 && (strcmp(argv[1], "--") == 0)) {
		argv++;
		argc--;
	}
	if(argc != 2) {
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":564:Usage: sleep time\n");
		exit(2);
	}
	n = strtol(argv[1],&s,10);
	/* n is a nonnegative decimal integer */
	if ((s != NULL && *s != '\0') || n < 0 ) {
		pfmt(stderr, MM_ERROR, ":565:Bad character in argument\n");
		exit(2);
	}
	if (errno == ERANGE) {
		pfmt(stderr, MM_ERROR, ":1191:Bad range\n") ;
		exit(2);
	}
	(void) sleep((unsigned)n);
	exit(0);
	/*NOTREACHED*/
}
