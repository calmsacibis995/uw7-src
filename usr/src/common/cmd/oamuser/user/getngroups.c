#ident  "@(#)getngroups.c	1.3"
#ident  "$Header$"

#include	<stdio.h>

extern void exit();
extern int get_ngm();

/* Print out the value of NGROUPS_MAX in the kernel */

main()
{
	(void) fprintf( stdout, "%d\n", get_ngm() );
	exit( 0 );
	/*NOTREACHED*/
}
