/* @(#)unbind.c	1.3
 *
 * unbind.c - decode an ldap unbind operation and pass it to a backend db
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
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"

extern Backend	*select_backend();
extern void      be_unbind();

extern char		*default_referral;
extern pthread_mutex_t	new_conn_mutex;

void
do_unbind(
    Connection	*conn,
    Operation	*op
)
{
	logDebug(LDAP_LOG_CLNT, "=> do_unbind\n", 0, 0, 0 );

	/*
	 * Parse the unbind request.  It looks like this:
	 *
	 *	UnBindRequest ::= NULL
	 */

	/* pass the unbind to all backends */
	be_unbind( conn, op );
	
	/* close the connection to the client */
	close_connection( conn, op->o_connid, op->o_opid );
	logDebug( LDAP_LOG_CLNT, "<= do_unbind\n", 0, 0, 0 );
}
