/* @(#)detach.c	1.3
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
#include <unistd.h>
#include "portable.h"

#include "ldaplog.h"

/*
 * detach()
 *	detaches processes from ttys and stuff for daemonization
 */
detach()
{
	int		i, sd, nbits;
	extern int	ldapdebug_level;

	nbits = sysconf( _SC_OPEN_MAX );

	if ( ldapdebug_level == 0 ) {
		for ( i = 0; i < 5; i++ ) {
			switch ( fork() ) {
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

		(void) chdir( "/" );

		if ( (sd = open( "/dev/null", O_RDWR )) == -1 ) {
			perror( "/dev/null" );
			exit( 1 );
		}
		if ( isatty( 0 ) )
			(void) dup2( sd, 0 );
		if ( isatty( 1 ) )
			(void) dup2( sd, 1 );
		if ( isatty(2) )
			(void) dup2( sd, 2 );
		close( sd );

		setsid();
		/* Notify log library that process is detached from ttys 
		 * and messages should be sent to the system log
		 */
		logProcessDetached(1);
	} 

	(void) SIGNAL( SIGPIPE, SIG_IGN );
}
