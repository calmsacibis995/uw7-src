/* @(#)compare.c	1.3
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
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"

extern Backend	*select_backend();

extern char	*default_referral;

void
do_compare(
    Connection	*conn,
    Operation	*op
)
{
	char	*dn;
	Ava	ava;
	int	rc;
	Backend	*be;

	logDebug( LDAP_LOG_CLNT, "=>do_compare\n", 0, 0, 0 );

	/*
	 * Parse the compare request.  It looks like this:
	 *
	 *	CompareRequest := [APPLICATION 14] SEQUENCE {
	 *		entry	DistinguishedName,
	 *		ava	SEQUENCE {
	 *			type	AttributeType,
	 *			value	AttributeValue
	 *		}
	 *	}
	 */

	if ( ber_scanf( op->o_ber, "{a{ao}}", &dn, &ava.ava_type,
	    &ava.ava_value ) == LBER_ERROR ) {

		logDebug( LDAP_LOG_CLNT, "(do_compare) ber_scanf failed\n",
		    0, 0, 0 );

		send_ldap_result( conn, op, LDAP_PROTOCOL_ERROR, NULL, "" );
		logDebug( LDAP_LOG_CLNT, "<=do_compare\n", 0, 0, 0 );
		return;
	}
	value_normalize( ava.ava_value.bv_val, attr_syntax( ava.ava_type ) );
	dn_normalize( dn );

	logDebug( LDAP_LOG_CLNT, "(do_compare) dn (%s) attr (%s) value (%s)\n",
	    dn, ava.ava_type, ava.ava_value.bv_val );

	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */
	if ( (be = select_backend( dn )) == NULL ) {
		free( dn );
		ava_free( &ava, 0 );

		send_ldap_result( conn, op, LDAP_PARTIAL_RESULTS, NULL,
		    default_referral );

		logDebug( LDAP_LOG_CLNT, "<=do_compare\n", 0, 0, 0 );

		return;
	}

	if ( be->be_compare != NULL ) {
		(*be->be_compare)( be, conn, op, dn, &ava );
	} else {
		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM, NULL,
		    "Function not implemented" );
	}

	free( dn );
	ava_free( &ava, 0 );
	logDebug( LDAP_LOG_CLNT, "<=do_compare\n", 0, 0, 0 );
}
