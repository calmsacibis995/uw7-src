/*
 * Copyright (c) 1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 * lock.c - routines to open and apply an advisory lock to a file
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/socket.h>
#include "portable.h"
#ifdef USE_LOCKF
#include <unistd.h>
#endif
#include "../slapd/slap.h"
#include "i18n.h"



FILE *
lock_fopen(
    char	*fname,
    char	*type,
    FILE	**lfp
)
{
	FILE	*fp;
	char	buf[MAXPATHLEN];

	/* open the lock file */
	strcpy( buf, fname );
	strcat( buf, ".lock" );
	if ( (*lfp = fopen( buf, "w" )) == NULL ) {
		return( NULL );
	}

	/* acquire the lock */
#ifdef USE_LOCKF
	while ( lockf( fileno( *lfp ), F_LOCK, 0 ) != 0 ) {
#else
	while ( flock( fileno( *lfp ), LOCK_EX ) != 0 ) {
#endif
		;	/* NULL */
	}

	/* open the log file */
	if ( (fp = fopen( fname, type )) == NULL ) {
#ifdef USE_LOCKF
		lockf( fileno( *lfp ), F_ULOCK, 0 );
#else
		flock( fileno( *lfp ), LOCK_UN );
#endif
		return( NULL );
	}

	return( fp );
}



int
lock_fclose(
    FILE	*fp,
    FILE	*lfp
)
{
	/* unlock */
#ifdef USE_LOCKF
	lockf( fileno( lfp ), F_ULOCK, 0 );
#else
	flock( fileno( lfp ), LOCK_UN );
#endif
	fclose( lfp );

	return( fclose( fp ) );
}



/*
 * Apply an advisory lock on a file.  Just calls lock_fopen()
 */
int
acquire_lock(
    char	*file,
    FILE	**rfp,
    FILE	**lfp
)
{
    if (( *rfp = lock_fopen( file, "r+", lfp )) == NULL ) {
	return( -1 );
    }
    return( 0 );
}



/*
 * Relinquish a lock on a file.  Calls lock_fclose() and also removes the
 * lock file.
 */
int
relinquish_lock(
    char	*file,
    FILE	*rfp,
    FILE	*lfp
)
{
    if ( lock_fclose( rfp, lfp ) == EOF ) {
	return( -1 );
    }
    return( 0 );
}
