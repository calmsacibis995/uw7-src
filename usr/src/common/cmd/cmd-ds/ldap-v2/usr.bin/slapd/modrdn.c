/* @(#)modrdn.c	1.3
 *
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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"

extern Backend	*select_backend();

extern char	*default_referral;

void
do_modrdn(
    Connection	*conn,
    Operation	*op
)
{
	char	*dn, *odn, *newrdn;
	int	deloldrdn;
	Backend	*be;

	logDebug( LDAP_LOG_CLNT, "=> do_modrdn\n", 0, 0, 0 );

	/*
	 * Parse the modrdn request.  It looks like this:
	 *
	 *	ModifyRDNRequest := SEQUENCE {
	 *		entry	DistinguishedName,
	 *		newrdn	RelativeDistinguishedName
	 *	}
	 */

	if ( ber_scanf( op->o_ber, "{aab}", &dn, &newrdn, &deloldrdn )
	    == LBER_ERROR ) {
		logDebug((LDAP_LOG_CLNT | LDAP_LOG_BER | LDAP_LOG_NETWORK),
		     "(do_modrdn) ber_scanf failed\n", 0, 0, 0 );

		send_ldap_result( conn, op, LDAP_PROTOCOL_ERROR, NULL, "" );
		logDebug( LDAP_LOG_CLNT, "<= do_modrdn\n", 0, 0, 0 );
		return;
	}
	odn = strdup( dn );
	dn_normalize( dn );

	logDebug( LDAP_LOG_CLNT,
	    "(do_modrdn) dn (%s) newrdn (%s) deloldrdn (%d)\n", dn, newrdn,
	    deloldrdn );


	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */

	if ( (be = select_backend( dn )) == NULL ) {
		free( dn );
		free( odn );
		free( newrdn );
		send_ldap_result( conn, op, LDAP_PARTIAL_RESULTS, NULL,
		    default_referral );
		logDebug( LDAP_LOG_CLNT, "<= do_modrdn\n", 0, 0, 0 );
		return;
	}

	/*
	 * do the add if 1 && (2 || 3)
	 * 1) there is an add function implemented in this backend;
	 * 2) this backend is master for what it holds;
	 * 3) it's a replica and the dn supplied is the updatedn.
	 */
	if ( be->be_modrdn != NULL ) {
		/* do the update here */
		if ( be->be_updatedn == NULL || strcasecmp( be->be_updatedn,
		    op->o_dn ) == 0 ) {
			if ( (*be->be_modrdn)( be, conn, op, dn, newrdn,
			    deloldrdn ) == 0 ) {
				replog( be, LDAP_REQ_MODRDN, odn, newrdn,
				    deloldrdn );
			}
		} else {
			send_ldap_result( conn, op, LDAP_PARTIAL_RESULTS, NULL,
			    default_referral );
		}
	} else {
		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM, NULL,
		    "Function not implemented" );
	}

	free( dn );
	free( odn );
	free( newrdn );
	logDebug( LDAP_LOG_CLNT, "<= do_modrdn\n", 0, 0, 0 );
}
