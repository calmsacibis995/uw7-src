#ident  "@(#)findgid.c	1.3"
#ident  "$Header$"

#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>

extern gid_t findnextgid();
extern void exit();

/* return the next available gid */
main()
{
	gid_t gid = findnextgid();
	if( gid == -1 )
		exit( EX_FAILURE );
	(void) fprintf( stdout, "%ld\n", gid );
	exit( EX_SUCCESS );
	/*NOTREACHED*/
}
