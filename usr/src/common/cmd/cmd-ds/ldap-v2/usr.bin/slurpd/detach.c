/* @(#)detach.c	1.4
 *
 * Copyright (c) 1990, 1994 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include <stdio.h>
#include <sys/types.h>
#ifdef SVR4
#include <sys/stat.h>
#endif /* svr4 */
#include <fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "portable.h"
#include "ldaplog.h"

#ifdef USE_SYSCONF
#include <unistd.h>
#endif /* USE_SYSCONF */

extern void slurpdExit(int errcode);



detach()
{
	int		i, sd, nbits;

#ifdef USE_SYSCONF
	nbits = sysconf( _SC_OPEN_MAX );
#else /* USE_SYSCONF */
	nbits = getdtablesize();
#endif /* USE_SYSCONF */

	if ( ldapdebug_level == 0 ) {

		for ( i = 0; i < 5; i++ ) {
#if defined( sunos5 ) && defined( THREAD_SUNOS5_LWP )
			switch ( fork1() ) {
#else
			switch ( fork() ) {
#endif
			case -1:
				sleep( 5 );
				continue;

			case 0:
				break;

			default:
				_exit( 0 );
			}
			break;
		}

/*
		for ( i = 3; i < nbits; i++ )
			close( i );
*/

		(void) chdir( "/" );

		if ( (sd = open( "/dev/null", O_RDWR )) == -1 ) {
			perror( "/dev/null" );
			slurpdExit( 1 );
		}
		if ( isatty( 0 ) )
			(void) dup2( sd, 0 );
		if ( isatty( 1 ) )
			(void) dup2( sd, 1 );
		if ( isatty(2) )
			(void) dup2( sd, 2 );
		close( sd );

#ifdef USE_SETSID
		setsid();
#else /* USE_SETSID */
		if ( (sd = open( "/dev/tty", O_RDWR )) != -1 ) {
			(void) ioctl( sd, TIOCNOTTY, NULL );
			(void) close( sd );
		}
#endif /* USE_SETSID */
	} 

	(void) SIGNAL( SIGPIPE, SIG_IGN );
	logProcessDetached(1);
}
