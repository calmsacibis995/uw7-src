#ident	"@(#)prerrno.c	1.2"
#ident  "$Header$"

#include	<errno.h>

extern char *sys_errlist[];
extern int sys_nerr;
extern int sprintf();

char *
prerrno( L_errno )
int L_errno;
{
	static char buffer[ 30 ];
	if( L_errno < sys_nerr ) return( sys_errlist[ L_errno ] );
	(void) sprintf( buffer, "Unknown errno %d", L_errno );
	return( buffer );
}

