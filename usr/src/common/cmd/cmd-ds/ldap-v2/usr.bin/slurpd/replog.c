/* @(#)replog.c	1.4
 *
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
 * replog.c - routines which read and write replication log files.
 */


#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "portable.h"
#include "slurp.h"
#include "globals.h"
#include "ldaplog.h"

#define MSG_COPYREP1 \
    1,174,"Cannot copy the replication log, directory %s is not writeable\n"
#define MSG_COPYREP2 \
    1,175,"Can't lock replication log \"%s\" for read (error %s)\n"
#define MSG_COPYREP3 \
    1,176,"Can't lock replication log \"%s\" for write (error %s)\n"
#define MSG_COPYREP4 \
    1,177,"Error occured closing replication log \"%s\"\n"


/*
 * Externs
 */
#ifdef NEEDPROTOS
extern FILE *lock_fopen( char *, char *, FILE ** );
extern char *ch_malloc( unsigned long );
#else /* NEEDPROTOS */
extern FILE *lock_fopen();
extern char *ch_malloc();
#endif /* NEEDPROTOS */

/*
 * Forward declarations
 */
#ifdef NEEDPROTOS
int file_nonempty( char * );
#else /* NEEDPROTOS */
int file_nonempty();
#endif /* NEEDPROTOS */


#ifndef SYSERRLIST_IN_STDIO
extern char *sys_errlist[];
#endif

/*
 * Forward declarations
 */
static int duplicate_replog( char *, char * );




/*
 * Copy the replication log.  Returns 0 on success, 1 if a temporary
 * error occurs, and -1 if a fatal error occurs.
 */
int
copy_replog(
    char	*src,
    char	*dst
)
{
    int		rc = 0;
    FILE	*rfp;	/* replog fp */
    FILE	*lfp;	/* replog lockfile fp */
    FILE	*dfp;	/* duplicate replog fp */
    FILE	*dlfp;	/* duplicate replog lockfile fp */
    static char	buf[ MAXPATHLEN ];
    static char	rbuf[ 1024 ];
    char	*p;

    /*
     * Make sure the destination directory is writable.  If not, exit
     * with a fatal error.
     */
    strcpy( buf, src );
    if (( p = strrchr( buf, '/' )) == NULL ) {
	strcpy( buf, "." );
    } else {
	*p = '\0';
    }
    if ( access( buf, W_OK ) < 0 ) {
	logError(get_ldap_message(MSG_COPYREP1,buf));
	return( -1 );
    }
    strcpy( buf, dst );
    if (( p = strrchr( buf, '/' )) == NULL ) {
	strcpy( buf, "." );
    } else {
	*p = '\0';
    }
    if ( access( buf, W_OK ) < 0 ) {
	logError(get_ldap_message(MSG_COPYREP1,buf));
	return( -1 );
    }

    /* lock src */
    rfp = lock_fopen( src, "r", &lfp );
    if ( rfp == NULL ) {
	logInfo(get_ldap_message(MSG_COPYREP2,src,sys_errlist[errno]));
	return( 1 );
    }

    /* lock dst */
    dfp = lock_fopen( dst, "a", &dlfp );
    if ( dfp == NULL ) {
	logInfo(get_ldap_message(MSG_COPYREP3,src,sys_errlist[ errno ]));
	lock_fclose( rfp );
	return( 1 );
    }

    /*
     * Make our own private copy of the replication log.
     */
    while (( p = fgets( rbuf, sizeof( buf ), rfp )) != NULL ) {
	fputs( rbuf, dfp );
    }
    /* Only truncate the source file if we're not in one-shot mode */
    if ( !sglob->one_shot_mode ) {
	/* truncate replication log */
	truncate( src, (off_t) 0 );
    }

    if ( lock_fclose( rfp, lfp ) == EOF ) {
	logInfo(get_ldap_message(MSG_COPYREP4, src));
    }
    if ( lock_fclose( dfp, dlfp ) == EOF ) {
	logInfo(get_ldap_message(MSG_COPYREP4, src));
    }
    return( rc );
}




/*
 * Return 1 if the given file exists and has a nonzero size,
 * 0 if it is empty or nonexistent.
 */
int
file_nonempty(
    char	*filename
)
{
    static struct stat 	stbuf;

    if ( stat( filename, &stbuf ) < 0 ) {
	return( 0 );
    } else {
	return( stbuf.st_size > (off_t ) 0 );
    }
}
