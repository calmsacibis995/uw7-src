/* @(#)abandon.c	1.4
 *
 * abandon.c - decode and handle an ldap abandon operation
 */

/*
 * Copyright (c) 1995 Regents of the University of Michigan.
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
#include <sys/socket.h>
#include "slap.h"
#include "ldaplog.h"

void
do_abandon(
    Connection	*conn,
    Operation	*op
)
{
	int		id;
	Backend		*be;
	Operation	*o;

	logDebug( LDAP_LOG_CLNT, "->do_abandon\n", 0, 0, 0 );

	/*
	 * Parse the abandon request.  It looks like this:
	 *
	 *	AbandonRequest := MessageID
	 */

	if ( ber_scanf( op->o_ber, "i", &id ) == LBER_ERROR ) {
		logDebug( LDAP_LOG_CLNT,"(do_abandon) ber_scanf failed\n", 0, 0 ,0 );
		return;
	}

	logDebug( LDAP_LOG_CLNT, "(do_abandon) MessageID=%d\n", id, 0 ,0 );

	/*
	 * find the operation being abandoned and set the o_abandon
	 * flag.  It's up to the backend to periodically check this
	 * flag and abort the operation at a convenient time.
	 */

	pthread_mutex_lock( &conn->c_opsmutex );
	for ( o = conn->c_ops; o != NULL; o = o->o_next ) {
		if ( o->o_msgid == id )
			break;
	}

	if ( o != NULL ) {
		pthread_mutex_lock( &o->o_abandonmutex );
		o->o_abandon = 1;
		pthread_mutex_unlock( &o->o_abandonmutex );
	} else {
		logDebug(LDAP_LOG_CLNT,
		    "(do_abandon) Cannot find operation to abandon\n",0,0,0);
	}
	pthread_mutex_unlock( &conn->c_opsmutex );
}
