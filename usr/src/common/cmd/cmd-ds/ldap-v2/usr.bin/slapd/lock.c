/* @(#)lock.c	1.3
 * 
 * lock.c - routines to open and apply an advisory lock to a file
 *
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include "portable.h"
#ifdef USE_LOCKF
#include <unistd.h>
#endif
#include <sys/file.h>
#include <sys/param.h>
#include <sys/socket.h>
#include "slap.h"

/* Messages */

#define MSG_NOOPEN1 \
    1,67,"Could not open lock file \"%s\"\n"


FILE *
lock_fopen( char *fname, char *type, FILE **lfp )
{
	FILE	*fp;
	char	buf[MAXPATHLEN];

	/* open the lock file */
	strcpy( buf, fname );
	strcat( buf, ".lock" );
	if ( (*lfp = fopen( buf, "w" )) == NULL ) {

		logError(get_ldap_message(MSG_NOOPEN1,buf));

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

		logError(get_ldap_message(MSG_NOOPEN1,fname));

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
lock_fclose( FILE *fp, FILE *lfp )
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
