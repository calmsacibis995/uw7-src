/* @(#)bind.c	1.5
 *
 *  Copyright (c) 1990 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  bind.c
 */

#ifndef lint 
static char copyright[] = "@(#) Copyright (c) 1990 Regents of the University of Michigan.\nAll rights reserved.\n";
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "lber.h"
#include "ldap.h"
#include "ldaplog.h"

#ifdef LDAP_REFERRALS
void ldap_set_rebind( LDAP *ld,char *dn, char *passwd, int authmethod);
#endif

/*
 * ldap_bind - bind to the ldap server (and X.500).  The dn and password
 * of the entry to which to bind are supplied, along with the authentication
 * method to use.  The msgid of the bind request is returned on success,
 * -1 if there's trouble.  Note, the kerberos support assumes the user already
 * has a valid tgt for now.  ldap_result() should be called to find out the
 * outcome of the bind request.
 *
 * Example:
 *	ldap_bind( ld, "cn=manager, o=university of michigan, c=us", "secret",
 *	    LDAP_AUTH_SIMPLE )
 */

int
ldap_bind( LDAP *ld, char *dn, char *passwd, int authmethod )
{
	/*
	 * The bind request looks like this:
	 *	BindRequest ::= SEQUENCE {
	 *		version		INTEGER,
	 *		name		DistinguishedName,	 -- who
	 *		authentication	CHOICE {
	 *			simple		[0] OCTET STRING -- passwd
#ifdef KERBEROS
	 *			krbv42ldap	[1] OCTET STRING
	 *			krbv42dsa	[2] OCTET STRING
#endif
	 *		}
	 *	}
	 * all wrapped up in an LDAPMessage sequence.
	 */

#ifdef LDAP_REFERRALS
	ldap_set_rebind(ld,dn,passwd,authmethod);
#endif


	switch ( authmethod ) {
	case LDAP_AUTH_SIMPLE:
		return( ldap_simple_bind( ld, dn, passwd ) );

#ifdef KERBEROS
	case LDAP_AUTH_KRBV41:
		return( ldap_kerberos_bind1( ld, dn ) );

	case LDAP_AUTH_KRBV42:
		return( ldap_kerberos_bind2( ld, dn ) );
#endif

	default:
		ld->ld_errno = LDAP_AUTH_UNKNOWN;
		return( -1 );
	}
}

/*
 * ldap_bind_s - bind to the ldap server (and X.500).  The dn and password
 * of the entry to which to bind are supplied, along with the authentication
 * method to use.  This routine just calls whichever bind routine is
 * appropriate and returns the result of the bind (e.g. LDAP_SUCCESS or
 * some other error indication).  Note, the kerberos support assumes the
 * user already has a valid tgt for now.
 *
 * Examples:
 *	ldap_bind_s( ld, "cn=manager, o=university of michigan, c=us",
 *	    "secret", LDAP_AUTH_SIMPLE )
 *	ldap_bind_s( ld, "cn=manager, o=university of michigan, c=us",
 *	    NULL, LDAP_AUTH_KRBV4 )
 */
int
ldap_bind_s( LDAP *ld, char *dn, char *passwd, int authmethod )
{

#ifdef LDAP_REFERRALS
	/*
	 * Store information for binding so that if a referral
	 * is received we can attempt to bind to the new
	 * server using the same information
	 */
	ldap_set_rebind(ld,dn,passwd,authmethod);
#endif

	switch ( authmethod ) {
	case LDAP_AUTH_SIMPLE:
		return( ldap_simple_bind_s( ld, dn, passwd ) );

#ifdef KERBEROS
	case LDAP_AUTH_KRBV4:
		return( ldap_kerberos_bind_s( ld, dn ) );

	case LDAP_AUTH_KRBV41:
		return( ldap_kerberos_bind1_s( ld, dn ) );

	case LDAP_AUTH_KRBV42:
		return( ldap_kerberos_bind2_s( ld, dn ) );
#endif

	default:
		return( ld->ld_errno = LDAP_AUTH_UNKNOWN );
	}
}

#ifdef LDAP_REFERRALS
/*
 * Called when handling a referral to determine how the original bind
 * was made 
 */
ldap_simplerebind( LDAP *ld, char **dnp,char **passwdp, int *authmethodp, int freeit )
{
	char *ptr;

	if (!freeit) {
		ptr=(char *)malloc(strlen(ld->ld_passwd)+1);
		strcpy(ptr,ld->ld_passwd);
		*dnp=ld->ld_redn;
		*passwdp=ld->ld_passwd;
		*authmethodp=ld->ld_authmethod;
	} else  {
		/*
		 * nothing to free - gets released when LDAP structure
		 * is destroyed.
		 */
	}
	return(LDAP_SUCCESS);
}
#endif

#ifdef LDAP_REFERRALS
/*
 * Sets rebind information in case of a referral
 */
void
ldap_set_rebind( LDAP *ld, char *dn,char *passwd, int authmethod)
{
	char *ptr;

	if (ld->ld_passwd) {
		/* 
	  	 * Already done
		 */
		return;
	}

	ld->ld_passwd=(char *)malloc(strlen(passwd)+1);
	strcpy(ld->ld_passwd,passwd);
	ld->ld_redn=(char *)malloc(strlen(dn)+1);
	strcpy(ld->ld_redn,dn);
	ld->ld_authmethod=authmethod;
	ldap_set_rebind_proc( ld, ldap_simplerebind);
}
#endif


#ifdef LDAP_REFERRALS
void
ldap_set_rebind_proc( LDAP *ld, int (*rebindproc)( LDAP *ld, char **dnp,
	char **passwdp, int *authmethodp, int freeit ))
{
	ld->ld_rebindproc = rebindproc;
}
#endif /* LDAP_REFERRALS */
