/* @(#)fork.c	1.3
 * 
 * fork.c - fork and exec a process, connecting stdin/out w/pipes
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"

/* Messages */
#define MSG_PIPEFAILED1 \
    1,102,"Could not create pipe to shell backend\n"
#define MSG_DUPFAILED1 \
    1,103,"Call to dup2 failed\n"
#define MSG_EVECVFAILED1 \
    1,104,"Could not execute shell backend script, execv failed\n"
#define MSG_FORKFAILED1 \
    1,105,"Could not execute shell backend script, fork failed\n"
#define MSG_FDOPENFAIL \
    1,106,"Could not execute shell backend script, could not open temporary file\n"


forkandexec(
    char	**args,
    FILE	**rfp,
    FILE	**wfp
)
{
	int	p2c[2], c2p[2];
	int	pid;

	if ( pipe( p2c ) != 0 || pipe( c2p ) != 0 ) {

		logError(get_ldap_message(MSG_PIPEFAILED1));
		return( -1 );
	}

	/*
	 * what we're trying to set up looks like this:
	 *	parent *wfp -> p2c[1] | p2c[0] -> stdin child
	 *	parent *rfp <- c2p[0] | c2p[1] <- stdout child
	 */

	switch ( (pid = fork()) ) {
	case 0:		/* child */
		close( p2c[1] );
		close( c2p[0] );
		if ( dup2( p2c[0], 0 ) == -1 || dup2( c2p[1], 1 ) == -1 ) {

			logError(get_ldap_message(MSG_DUPFAILED1));
			exit( -1 );
		}

		execv( args[0], args );

		logError(get_ldap_message(MSG_EVECVFAILED1));
		exit( -1 );

	case -1:	/* trouble */

		logError(get_ldap_message(MSG_FORKFAILED1));
		return( -1 );

	default:	/* parent */
		close( p2c[0] );
		close( c2p[1] );
		break;
	}

	if ( (*rfp = fdopen( c2p[0], "r" )) == NULL || (*wfp = fdopen( p2c[1],
	    "w" )) == NULL ) {

		logError(get_ldap_message(MSG_FDOPENFAIL));
		close( c2p[0] );
		close( p2c[1] );

		return( -1 );
	}

	return( pid );
}
