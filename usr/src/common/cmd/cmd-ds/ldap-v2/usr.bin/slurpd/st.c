/* @(#)st.c	1.4
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
 * st.c - routines for managing the status structure, and for reading and
 * writing status information to disk.
 */

#define MSG_STWRITE \
    1,187,"Cannot open status file \"%s\": %s\n"
#define MSG_STREAD01 \
    1,188,"Cannot create status file \"%s\"\n"
#define MSG_STREAD02 \
    1,189,"Warning: saved state for %s:%s, not a known replica\n"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "slurp.h"
#include "globals.h"
#include "ldaplog.h"

#ifndef SYSERRLIST_IN_STDIO
extern char *sys_errlist[];
#endif /* SYSERRLIST_IN_STDIO */

/*
 * Add information about replica host specified by Ri to list
 * of hosts.
 */
static Stel *
St_add(
    St	*st,
    Ri	*ri
)
{
    int	ind;

    if ( st == NULL || ri == NULL ) {
	return NULL;
    }

    /* Serialize access to the St struct */
    pthread_mutex_lock( &(st->st_mutex ));

    st->st_nreplicas++;
    ind = st->st_nreplicas - 1;

    st->st_data = ( Stel ** ) ch_realloc( st->st_data, 
	    ( st->st_nreplicas * sizeof( Stel* )));
    if ( st->st_data == NULL ) {
	pthread_mutex_unlock( &(st->st_mutex ));
	return NULL;
    }

/* old code 
    st->st_data[ ind ]  = ( Stel * ) ch_malloc( st->st_data, sizeof( Stel ));
*/

/* Bug fix - start */
    st->st_data[ ind ]  = ( Stel * ) ch_malloc( sizeof( Stel ));
    
/*Bug fix - end */

    if ( st->st_data[ ind ] == NULL ) {
	pthread_mutex_unlock( &(st->st_mutex ));
	return NULL;
    }

    st->st_data[ ind ]->hostname = strdup( ri->ri_hostname );
    st->st_data[ ind ]->port = ri->ri_port;
    memset( st->st_data[ ind ]->last, 0, sizeof( st->st_data[ ind ]->last )); 
    st->st_data[ ind ]->seq = 0;

    pthread_mutex_unlock( &(st->st_mutex ));
    return st->st_data[ ind ];
}



/*
 * Write the contents of an St to disk.
 */
static int
St_write (
    St	*st
)
{
    int		rc;
    Stel	*stel;
    int		i;

    if ( st == NULL ) {
	return -1;
    }
    pthread_mutex_lock( &(st->st_mutex ));
    if ( st->st_fp == NULL ) {
	/* Open file */
	if (( rc = acquire_lock( sglob->slurpd_status_file, &(st->st_fp),
		&(st->st_lfp))) < 0 ) {
	    if ( !st->st_err_logged ) {
		logError(get_ldap_message(MSG_STWRITE,
		    sglob->slurpd_status_file, sys_errlist[ errno ]));

		st->st_err_logged = 1;
		pthread_mutex_unlock( &(st->st_mutex ));
		return -1;
	    }
	} else {
	    st->st_err_logged = 0;
	}
    }

    /* Write data to the file */
    truncate( sglob->slurpd_status_file, 0L );
    fseek( st->st_fp, 0L, 0 );
    for ( i = 0; i < st->st_nreplicas; i++ ) {
	stel = st->st_data[ i ];
	fprintf( st->st_fp, "%s:%d:%s:%d\n", stel->hostname, stel->port,
		stel->last, stel->seq );
    }
    fflush( st->st_fp );

    pthread_mutex_unlock( &(st->st_mutex ));

    return 0;
}
    



/*
 * Update the entry for a given host.
 */
static int
St_update(
    St		*st,
    Stel	*stel,
    Re		*re
)
{
    if ( stel == NULL || re == NULL ) {
	return -1;
    }

    pthread_mutex_lock( &(st->st_mutex ));
    strcpy( stel->last, re->re_timestamp );
    stel->seq = re->re_seq;
    pthread_mutex_unlock( &(st->st_mutex ));
    return 0;
}




/*
 * Read status information from disk file.
 */
static int
St_read(
    St	*st
)
{
    FILE	*fp;
    FILE	*lfp;
    char	buf[ 255 ];
    int		i;
    int		rc;
    char	*hostname, *port, *timestamp, *seq, *p, *t;
    int		found;

    if ( st == NULL ) {
	return -1;
    }
    pthread_mutex_lock( &(st->st_mutex ));
    if ( access( sglob->slurpd_status_file, F_OK ) < 0 ) {
	/*
	 * File doesn't exist, so create it and return.
	 */
	if (( fp = fopen( sglob->slurpd_status_file, "w" )) == NULL ) {
	    logError(get_ldap_message(MSG_STREAD01,
		sglob->slurpd_status_file));

	    pthread_mutex_unlock( &(st->st_mutex ));
	    return -1;
	}
	(void) fclose( fp );
	pthread_mutex_unlock( &(st->st_mutex ));
	return 0;
    }
    if (( rc = acquire_lock( sglob->slurpd_status_file, &fp, &lfp)) < 0 ) {
	return 0;
    }
    while ( fgets( buf, sizeof( buf ), fp ) != NULL ) {
	p = buf;
	hostname = p;
	if (( t = strchr( p, ':' )) == NULL ) {
	    goto bad;
	}
	*t++ = '\0';
	p = t;
	port = p;
	if (( t = strchr( p, ':' )) == NULL ) {
	    goto bad;
	}
	*t++ = '\0';
	p = t;
	timestamp = p;
	if (( t = strchr( p, ':' )) == NULL ) {
	    goto bad;
	}
	*t++ = '\0';
	seq = t;
	if (( t = strchr( seq, '\n' )) != NULL ) {
	    *t = '\0';
	}

	found = 0;
	for ( i = 0; i < sglob->st->st_nreplicas; i++ ) {
	    if ( !strcmp( hostname, sglob->st->st_data[ i ]->hostname ) &&
		    atoi( port ) == sglob->st->st_data[ i ]->port ) {
		found = 1;
		strcpy( sglob->st->st_data[ i ]->last, timestamp );
		sglob->st->st_data[ i ]->seq = atoi( seq );
		break;
	    }
	}
	if ( found ) {
	    char tbuf[ 255 ];
	    sprintf( tbuf, "%s:%s (timestamp %s.%s)", hostname, port,
		    timestamp, seq );
	} else {
		logInfo(get_ldap_message(MSG_STREAD02,hostname,port));

	}
    }
    (void) relinquish_lock( sglob->slurpd_status_file, fp, lfp);
    pthread_mutex_unlock( &(st->st_mutex ));
    return 0;

bad:
    (void) relinquish_lock( sglob->slurpd_status_file, fp, lfp);
    pthread_mutex_unlock( &(st->st_mutex ));
    return -1;
}
    



/*
 * Lock an St struct.
 */
static int
St_lock(
    St *st
)
{
    return( pthread_mutex_lock( &st->st_mutex ));
}




/*
 * Lock an St struct.
 */
static int
St_unlock(
    St *st
)
{
    return( pthread_mutex_unlock( &st->st_mutex ));
}




/*
 * Allocate and initialize an St struct.
 */
int
St_init(
    St **st
)
{
    if ( st == NULL ) {
	return -1;
    }

    (*st) = (St *) malloc( sizeof( St ));
    if ( *st == NULL ) {
	return -1;
    }

    pthread_mutex_init( &((*st)->st_mutex), pthread_mutexattr_default );
    (*st)->st_data = NULL;
    (*st)->st_fp = NULL;
    (*st)->st_lfp = NULL;
    (*st)->st_nreplicas = 0;
    (*st)->st_err_logged = 0;
    (*st)->st_update = St_update;
    (*st)->st_add = St_add;
    (*st)->st_write = St_write;
    (*st)->st_read = St_read;
    (*st)->st_lock = St_lock;
    (*st)->st_unlock = St_unlock;
    return 0;
}

