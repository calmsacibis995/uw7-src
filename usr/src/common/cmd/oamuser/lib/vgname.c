#ident	"@(#)vgname.c	1.2"
#ident  "$Header$"

#include	<sys/types.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<grp.h>
#include	<userdefs.h>
#include	<users.h>

#define nisname(n) (*n == '+' || *n == '-')

extern struct group *nis_getgrnam();

extern unsigned int strlen();

/*
 * validate string given as group name.
 */
int
valid_gname( group, gptr )
char *group;
struct group **gptr;
{
	register struct group *t_gptr;
	register char *ptr = group;

	if( !group || !*group || (int) strlen(nisname(group)?(group+1):group) >= MAXGLEN )
		return( INVALID );

	for( ; *ptr != NULL; ptr++ ) 
		if( !isprint(*ptr) || (*ptr == ':') )
			return( INVALID );

	if( t_gptr = nis_getgrnam( group ) ) {
		if( gptr ) *gptr = t_gptr;
		return( NOTUNIQUE );
	}

	return( UNIQUE );
}
