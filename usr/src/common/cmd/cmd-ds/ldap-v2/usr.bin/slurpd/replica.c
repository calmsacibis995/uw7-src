/* @(#)replica.c	1.4
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
 * replica.c - code to start up replica threads.
 */


#include <stdio.h>

#include "slurp.h"
#include "globals.h"
#include "ldaplog.h"

/* Messages */
#define MSG_THREAD01 \
    1,173,"Could not create thread to distribute information to replica %s:%d\n"



/*
 * Just invoke the Ri's process() member function, and log the start and
 * finish.
 */
void
replicate(
    Ri	*ri
)
{
    int i;
    unsigned long seq;

    logDebug(LDAP_SLURPD,
	"(replicate) begin replication thread for %s:%d\n",
	ri->ri_hostname, ri->ri_port, 0 );

    ri->ri_process( ri );

    logDebug(LDAP_SLURPD,
	"(replicate) end replication thread for %s:%d\n",
	ri->ri_hostname, ri->ri_port, 0 );
    return;
}



/*
 * Start a detached thread for the given replica.
 */
int
start_replica_thread(
    Ri	*ri
)
{
    pthread_attr_t	attr;

    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

    if ( pthread_create( &(ri->ri_tid), attr, (void *) replicate,
	    (void *) ri ) != 0 ) {
	logError(get_ldap_message(MSG_THREAD01,
		ri->ri_hostname, ri->ri_port, 0 ));

	pthread_attr_destroy( &attr );
	return -1;
    }
    pthread_attr_destroy( &attr );
    return 0;
}
