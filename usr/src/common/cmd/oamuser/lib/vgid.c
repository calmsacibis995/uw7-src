#ident	"@(#)vgid.c	1.2"
#ident  "$Header$"

#include	<sys/types.h>
#include	<stdio.h>
#include	<grp.h>
#include	<users.h>
#include	<sys/param.h>
#include	<userdefs.h>

/*
	MAXUID should be in param.h; if it isn't,
	try for UID_MAX in limits.h
*/
#ifndef	MAXUID
#include	<limits.h>
#define	MAXUID	UID_MAX
#endif

struct group *getgrgid();

/*  validate a GID */
int
valid_gid( gid, gptr )
gid_t gid;
struct group **gptr;
{
	register struct group *t_gptr;

	if( gid < 0 ) return( INVALID );

	if( gid > MAXUID ) return( TOOBIG );

	if( t_gptr = getgrgid( gid ) ) {
		if( gptr ) *gptr = t_gptr;
		return( NOTUNIQUE );
	}

	if( gid <= DEFGID ) {
		if( gptr ) *gptr = getgrgid( gid );
		return( RESERVED );
	}

	return( UNIQUE );
}
