/* @(#)reject.c	1.4
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
 * reject.c - routines to write replication records to reject files.
 * An Re struct is writted to a reject file if it cannot be propagated
 * to a replica LDAP server.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "slurp.h"
#include "globals.h"
#include "ldaplog.h"

#define MSG_WRREJ1 \
    1,169,"Cannot create rejection file \"%s\": %s\n"
#define MSG_WRREJ2 \
    1,170,"Cannot open reject file \"%s\"\n"
#define MSG_WRREJ3 \
    1,171,"Cannot write to reject file \"%s\"\n"
#define MSG_WRREJ4 \
    1,172,"ldap operation to %s:%d failed, data written to \"%s\"\n"

#ifndef SYSERRLIST_IN_STDIO
extern char *sys_errlist[];
#endif /* SYSERRLIST_IN_STDIO */


/*
 * Write a replication record to a reject file.  The reject file has the
 * same name as the replica's private copy of the file but with ".rej"
 * appended (e.g. "/usr/tmp/<hostname>:<port>.rej")
 *
 * If errmsg is non-NULL, use that as the error message in the reject
 * file.  Otherwise, use ldap_err2string( lderr ).
 */
void
write_reject(
    Ri		*ri,
    Re		*re,
    int		lderr,
    char	*errmsg
)
{
    char	rejfile[ MAXPATHLEN ];
    FILE	*rfp, *lfp;
    int		rc;

    pthread_mutex_lock( &sglob->rej_mutex );
    sprintf( rejfile, "%s/%s:%d.rej", sglob->slurpd_rdir,
	    ri->ri_hostname, ri->ri_port );

    if ( access( rejfile, F_OK ) < 0 ) {
	/* Doesn't exist - try to create */
	int rjfd;
	if (( rjfd = open( rejfile, O_RDWR | O_APPEND | O_CREAT,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP )) < 0 ) {

		logError(get_ldap_message(MSG_WRREJ1,
			rejfile, sys_errlist[ errno ]));

	    pthread_mutex_unlock( &sglob->rej_mutex );
	    return;
	} else {
	    close( rjfd );
	}
    }
    if (( rc = acquire_lock( rejfile, &rfp, &lfp )) < 0 ) {
	logError(get_ldap_message(MSG_WRREJ2,rejfile));
    } else {
	fseek( rfp, 0, 2 );
	if ( errmsg != NULL ) {
	    fprintf( rfp, "%s: %s\n", ERROR_STR, errmsg );
	} else {
	    fprintf( rfp, "%s: %s\n", ERROR_STR, ldap_err2string( lderr ));
	}
	if ((rc = re->re_write( ri, re, rfp )) < 0 ) {
	    logError(get_ldap_message(MSG_WRREJ3,rejfile));
	}
	(void) relinquish_lock( rejfile, rfp, lfp );
	logError(get_ldap_message(MSG_WRREJ4,
	    ri->ri_hostname, ri->ri_port,rejfile));
    }
    pthread_mutex_unlock( &sglob->rej_mutex );
    return;
}

