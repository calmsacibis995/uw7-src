/* @(#)bind.c	1.4
 *
 * bind.c - decode an ldap bind operation and pass it to a backend db
 *
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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "ldaplog.h"

#define MSG_PROTOCOLERR \
     1,1,"Protocol error, cannot get bind() information - network read failed\n"

extern Backend	*select_backend();

extern char	*default_referral;

void
do_bind(
    Connection	*conn,
    Operation	*op
)
{
	BerElement	*ber = op->o_ber;
	int		version, method, len, rc;
	char		*dn;
	struct berval	cred;
	Backend		*be;

	logDebug( LDAP_LOG_CLNT, "=>do_bind\n", 0, 0, 0 );

	/*
	 * Parse the bind request.  It looks like this:
	 *
	 *	BindRequest ::= SEQUENCE {
	 *		version		INTEGER,		 -- version
	 *		name		DistinguishedName,	 -- dn
	 *		authentication	CHOICE {
	 *			simple		[0] OCTET STRING -- passwd
	 *			krbv42ldap	[1] OCTET STRING
	 *			krbv42dsa	[1] OCTET STRING
	 *		}
	 *	}
	 */

#ifdef COMPAT30
	/*
	 * in version 3.0 there is an extra SEQUENCE tag after the
	 * BindRequest SEQUENCE tag.
	 */

	{
	BerElement	tber;
	unsigned long	tlen, ttag;

	tber = *op->o_ber;
	ttag = ber_skip_tag( &tber, &tlen );
	if ( ber_peek_tag( &tber, &tlen ) == LBER_SEQUENCE ) {

		logDebug( LDAP_LOG_CLNT,
		    "(do_bind) version 3.0 detected\n",0,0,0);

		conn->c_version = 30;
		rc = ber_scanf(ber, "{{iato}}", &version, &dn, &method, &cred);
	} else {
		rc = ber_scanf( ber, "{iato}", &version, &dn, &method, &cred );
	}
	}
#else
	rc = ber_scanf( ber, "{iato}", &version, &dn, &method, &cred );
#endif
	if ( rc == LBER_ERROR ) {
		logDebug( LDAP_LOG_CLNT,
		     "(do_bind) ber_scanf failed\n", 0, 0, 0 );
		send_ldap_result( conn, op, LDAP_PROTOCOL_ERROR, NULL,
		    "error decoding DN" );
		logDebug( LDAP_LOG_CLNT, "<=do_bind\n", 0, 0, 0 );
		return;
	}
#ifdef COMPAT30
	if ( conn->c_version == 30 ) {
		switch ( method ) {
		case LDAP_AUTH_SIMPLE_30:
			method = LDAP_AUTH_SIMPLE;
			break;
#ifdef KERBEROS
		case LDAP_AUTH_KRBV41_30:
			method = LDAP_AUTH_KRBV41;
			break;
		case LDAP_AUTH_KRBV42_30:
			method = LDAP_AUTH_KRBV42;
			break;
#endif
		}
	}
#endif /* compat30 */

	dn_normalize( dn );

	if ( version != LDAP_VERSION2 ) {
		if ( dn != NULL ) {
			free( dn );
		}
		if ( cred.bv_val != NULL ) {
			free( cred.bv_val );
		}

		logDebug( LDAP_LOG_CLNT,
		    "(do_bind) unknown version %d\n", version, 0, 0 );

		send_ldap_result( conn, op, LDAP_PROTOCOL_ERROR, NULL,
		    "version not supported" );
		logDebug( LDAP_LOG_CLNT, "<=do_bind\n", 0, 0, 0 );
		return;
	}

	logDebug( LDAP_LOG_CLNT,
	     "(do_bind) version (%d) dn (%s) method (%d)\n",
	     version, dn, method );

	/* accept null binds */
	if ( dn == NULL || *dn == '\0' ) {
		if ( dn != NULL ) {
			free( dn );
		}
		if ( cred.bv_val != NULL ) {
			free( cred.bv_val );
		}

		send_ldap_result( conn, op, LDAP_SUCCESS, NULL, NULL );

		logDebug(LDAP_LOG_CLNT,
                    "(do_bind) Client bound to server anonymously\n",
		    0,0,0);

		logDebug( LDAP_LOG_CLNT, "<=do_bind\n", 0, 0, 0 );
		return;
	}

	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */

	if ( (be = select_backend( dn )) == NULL ) {
		free( dn );
		if ( cred.bv_val != NULL ) {
			free( cred.bv_val );
		}
		if ( cred.bv_len == 0 ) {
			send_ldap_result( conn, op, LDAP_SUCCESS, NULL, NULL );
		} else {
			send_ldap_result( conn, op, LDAP_PARTIAL_RESULTS, NULL,
			    default_referral );
		}
		logDebug( LDAP_LOG_CLNT, "<=do_bind\n", 0, 0, 0 );
		return;
	}

	if ( be->be_bind != NULL ) {
		if ( (*be->be_bind)( be, conn, op, dn, method, &cred ) == 0 ) {
			pthread_mutex_lock( &conn->c_dnmutex );
			if ( conn->c_dn != NULL ) {
				free( conn->c_dn );
			}
			conn->c_dn = strdup( dn );
			pthread_mutex_unlock( &conn->c_dnmutex );

			/* send this here to avoid a race condition */
			send_ldap_result( conn, op, LDAP_SUCCESS, NULL, NULL );
			logDebug( LDAP_LOG_CLNT, "(do_bind) Success\n", 0, 0, 0 );
		} else {
			logDebug( LDAP_LOG_CLNT, "(do_bind) Failed\n", 0, 0, 0 );
		}
	} else {
		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM, NULL,
		    "Function not implemented" );
	}

	free( dn );
	if ( cred.bv_val != NULL ) {
		free( cred.bv_val );
	}
	logDebug( LDAP_LOG_CLNT, "<=do_bind\n", 0, 0, 0 );
}
