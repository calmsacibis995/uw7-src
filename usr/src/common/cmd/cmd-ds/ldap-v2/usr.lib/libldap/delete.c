/* @(#)delete.c	1.5
 *  Copyright (c) 1990 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  delete.c
 */

#ifndef lint 
static char copyright[] = "@(#) Copyright (c) 1990 Regents of the University of Michigan.\nAll rights reserved.\n";
#endif

#include <stdio.h>
#include <string.h>

#ifdef MACOS
#include "macos.h"
#endif /* MACOS */

#if defined( DOS ) || defined( _WIN32 )
#include "msdos.h"
#endif /* DOS */

#if !defined( MACOS ) && !defined( DOS )
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include "lber.h"
#include "ldap.h"
#include "ldap-int.h"
#include "ldaplog.h"

/*
 * ldap_delete - initiate an ldap (and X.500) delete operation. Parameters:
 *
 *	ld		LDAP descriptor
 *	dn		DN of the object to delete
 *
 * Example:
 *	msgid = ldap_delete( ld, dn );
 */
int
ldap_delete( LDAP *ld, char *dn )
{
	BerElement	*ber;

	/*
	 * A delete request looks like this:
	 *	DelRequet ::= DistinguishedName,
	 */

	/* create a message to send */
	if ( (ber = ldap_alloc_ber_with_options( ld )) == NULLBER ) {
		return( -1 );
	}

	if ( ber_printf( ber, "{its}", ++ld->ld_msgid, LDAP_REQ_DELETE, dn )
	    == -1 ) {
		ld->ld_errno = LDAP_ENCODING_ERROR;
		ber_free( ber, 1 );
		return( -1 );
	}

	/* send the message */
	return ( ldap_send_initial_request( ld, LDAP_REQ_DELETE, dn, ber ));
}


int
ldap_delete_s( LDAP *ld, char *dn )
{
	int		msgid;
	LDAPMessage	*res;

	if ( (msgid = ldap_delete( ld, dn )) == -1 )
		return( ld->ld_errno );

	if ( ldap_result( ld, msgid, 1, (struct timeval *) NULL, &res ) == -1 )
		return( ld->ld_errno );

	return( ldap_result2error( ld, res, 1 ) );
}
