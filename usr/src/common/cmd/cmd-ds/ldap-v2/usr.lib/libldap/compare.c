/* @(#)compare.c	1.5
 *  Copyright (c) 1990 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  compare.c
 */

#ifndef lint 
static char copyright[] = "@(#) Copyright (c) 1990 Regents of the University of Michigan.\nAll rights reserved.\n";
#endif

#include <stdio.h>
#include <string.h>

#ifdef MACOS
#include "macos.h"
#endif /* MACOS */

#if !defined( MACOS ) && !defined( DOS )
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include "lber.h"
#include "ldap.h"
#include "ldap-int.h"
#include "ldaplog.h"

/*
 * ldap_compare - perform an ldap (and X.500) compare operation.  The dn
 * of the entry to compare to and the attribute and value to compare (in
 * attr and value) are supplied.  The msgid of the response is returned.
 *
 * Example:
 *	ldap_compare( ld, "c=us@cn=bob", "userPassword", "secret" )
 */
int
ldap_compare( LDAP *ld, char *dn, char *attr, char *value )
{
	BerElement	*ber;

	/* The compare request looks like this:
	 *	CompareRequest ::= SEQUENCE {
	 *		entry	DistinguishedName,
	 *		ava	SEQUENCE {
	 *			type	AttributeType,
	 *			value	AttributeValue
	 *		}
	 *	}
	 * and must be wrapped in an LDAPMessage.
	 */

	/* create a message to send */
	if ( (ber = ldap_alloc_ber_with_options( ld )) == NULLBER ) {
		return( -1 );
	}

	if ( ber_printf( ber, "{it{s{ss}}}", ++ld->ld_msgid, LDAP_REQ_COMPARE,
	    dn, attr, value ) == -1 ) {
		ld->ld_errno = LDAP_ENCODING_ERROR;
		ber_free( ber, 1 );
		return( -1 );
	}

#ifndef NO_CACHE
	if ( ld->ld_cache != NULL ) {
		if ( ldap_check_cache( ld, LDAP_REQ_COMPARE, ber ) == 0 ) {
			ber_free( ber, 1 );
			ld->ld_errno = LDAP_SUCCESS;
			return( ld->ld_msgid );
		}
		ldap_add_request_to_cache( ld, LDAP_REQ_COMPARE, ber );
	}
#endif /* NO_CACHE */

	/* send the message */
	return ( ldap_send_initial_request( ld, LDAP_REQ_COMPARE, dn, ber ));
}

int
ldap_compare_s( LDAP *ld, char *dn, char *attr, char *value )
{
	int		msgid;
	LDAPMessage	*res;

	if ( (msgid = ldap_compare( ld, dn, attr, value )) == -1 )
		return( ld->ld_errno );

	if ( ldap_result( ld, msgid, 1, (struct timeval *) NULL, &res ) == -1 )
		return( ld->ld_errno );

	return( ldap_result2error( ld, res, 1 ) );
}
