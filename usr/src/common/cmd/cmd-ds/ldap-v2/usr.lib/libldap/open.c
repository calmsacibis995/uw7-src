/* @(#)open.c	1.5
 *
 *  Copyright (c) 1995 Regents of the University of Michigan.
 *  All rights reserved.
 *
 *  open.c
 */

#ifndef lint 
static char copyright[] = "@(#) Copyright (c) 1995 Regents of the University of Michigan.\nAll rights reserved.\n";
#endif

#include <stdio.h>
#include <string.h>

#ifdef MACOS
#include <stdlib.h>
#include "macos.h"
#endif /* MACOS */

#if defined( DOS ) || defined( _WIN32 )
#include "msdos.h"
#include <stdlib.h>
#endif /* DOS */

#if !defined(MACOS) && !defined(DOS) && !defined( _WIN32 )
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifndef VMS
#include <sys/param.h>
#endif
#include <netinet/in.h>
#endif
#include "lber.h"
#include "ldap.h"
#include "ldaplog.h"
#include "ldap-int.h"

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK	((unsigned long) 0x7f000001)
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  64
#endif


/*
 * ldap_open - initialize and connect to an ldap server.  A magic cookie to
 * be used for future communication is returned on success, NULL on failure.
 * "host" may be a space-separated list of hosts or IP addresses
 *
 * Example:
 *	LDAP	*ld;
 *	ld = ldap_open( hostname, port );
 */

LDAP *
ldap_open( char *host, int port )
{
	LDAP		*ld;
#ifdef LDAP_REFERRALS
	LDAPServer	*srv;
#endif /* LDAP_REFERRALS */

	logDebug(LDAP_LOG_CLNT, "=> ldap_open\n", 0, 0, 0 );

	if (( ld = ldap_init( host, port )) == NULL ) {
		logDebug(LDAP_LOG_CLNT,
		    "<= ldap_open (ldap_init failed)\n",0,0,0);
		return( NULL );
	}

#ifdef LDAP_REFERRALS
	if (( srv = (LDAPServer *)calloc( 1, sizeof( LDAPServer ))) ==
	    NULL || ( ld->ld_defhost != NULL && ( srv->lsrv_host =
	    strdup( ld->ld_defhost )) == NULL )) {
		ldap_ld_free( ld, 0 );

		logDebug(LDAP_LOG_CLNT,
		    "<= ldap_open (memory alloc failed)\n",0,0,0);

		return( NULL );
	}
	srv->lsrv_port = ld->ld_defport;

	if ((ld->ld_defconn = ldap_new_connection( ld, &srv, 1,1,0 ))==NULL) {
		if ( ld->ld_defhost != NULL ) free( srv->lsrv_host );
		free( (char *)srv );
		ldap_ld_free( ld, 0 );

		logDebug(LDAP_LOG_CLNT,
		    "<= ldap_open (ldap_new_connection failed)\n",0,0,0);

		return( NULL );
	}
	++ld->ld_defconn->lconn_refcnt;	/* so it never gets closed/freed */

#else /* LDAP_REFERRALS */
	if ( open_ldap_connection( ld, &ld->ld_sb, ld->ld_defhost,
	    ld->ld_defport, &ld->ld_host, 0 ) < 0 ) {
		ldap_ld_free( ld, 0 );
		return( NULL );
	}
#endif /* LDAP_REFERRALS */

	logDebug(LDAP_LOG_CLNT, "<= ldap_open (successful)\n",0,0,0);

	return( ld );
}


/*
 * ldap_init - initialize the LDAP library.  A magic cookie to be used for
 * future communication is returned on success, NULL on failure.
 * "defhost" may be a space-separated list of hosts or IP addresses
 *
 * Example:
 *	LDAP	*ld;
 *	ld = ldap_open( default_hostname, default_port );
 */
LDAP *
ldap_init( char *defhost, int defport )
{
	LDAP			*ld;

	logDebug( LDAP_LOG_CLNT, "=> ldap_init\n", 0, 0, 0 );

	if ( (ld = (LDAP *) calloc( 1, sizeof(LDAP) )) == NULL ) {
		logDebug( LDAP_LOG_CLNT, "<= ldap_init (fail)\n",0,0,0);
		return( NULL );
	}

#ifdef LDAP_REFERRALS
	if (( ld->ld_selectinfo = ldap_new_select_info()) == NULL ) {
		free( (char*)ld );
		logDebug( LDAP_LOG_CLNT, "<= ldap_init (fail)\n",0,0,0);
		return( NULL );
	}
	ld->ld_options = LDAP_FLAG_REFERRALS;
#endif /* LDAP_REFERRALS */

	if ( defhost != NULL &&
	    ( ld->ld_defhost = strdup( defhost )) == NULL ) {
#ifdef LDAP_REFERRALS
		ldap_free_select_info( ld->ld_selectinfo );
#endif /* LDAP_REFERRALS */
		free( (char*)ld );
		logDebug( LDAP_LOG_CLNT, "<= ldap_init (fail)\n",0,0,0);
		return( NULL );
	}

	ld->ld_defport = ( defport == 0 ) ? LDAP_PORT : defport;
	ld->ld_version = LDAP_VERSION;
	ld->ld_lberoptions = LBER_USE_DER;
	ld->ld_refhoplimit = LDAP_DEFAULT_REFHOPLIMIT;

#ifdef LDAP_REFERRALS
	ld->ld_options |= LDAP_FLAG_REFERRALS;
#endif /* LDAP_REFERRALS */

#if defined( STR_TRANSLATION ) && defined( LDAP_DEFAULT_CHARSET )
	ld->ld_lberoptions |= LBER_TRANSLATE_STRINGS;
#if LDAP_CHARSET_8859 == LDAP_DEFAULT_CHARSET
	ldap_set_string_translators( ld, ldap_8859_to_t61, ldap_t61_to_8859 );
#endif /* LDAP_CHARSET_8859 == LDAP_DEFAULT_CHARSET */
#endif /* STR_TRANSLATION && LDAP_DEFAULT_CHARSET */

	logDebug( LDAP_LOG_CLNT, "<= ldap_init\n",0,0,0);
	return( ld );
}


int
open_ldap_connection( LDAP *ld, Sockbuf *sb, char *host, int defport,
	char **krbinstancep, int async )
{
	int 			rc, port;
	char			*p, *q, *r;
	char			*curhost, hostname[ 2*MAXHOSTNAMELEN ];

	defport = htons( defport );

	if (( host == NULL ) || (strlen(host) == 0)) {
		rc = ldap_connect_to_host( sb, NULL, htonl( INADDR_LOOPBACK ),
		    defport, async );
	} else {
		for ( p = host; p != NULL && *p != '\0'; p = q ) {
			if (( q = strchr( p, ' ' )) != NULL ) {
				strncpy( hostname, p, q - p );
				hostname[ q - p ] = '\0';
				curhost = hostname;
				while ( *q == ' ' ) {
				    ++q;
				}
			} else {
				curhost = p;	/* avoid copy if possible */
				q = NULL;
			}

			if (( r = strchr( curhost, ':' )) != NULL ) {
			    if ( curhost != hostname ) {
				strcpy( hostname, curhost );	/* now copy */
				r = hostname + ( r - curhost );
				curhost = hostname;
			    }
			    *r++ = '\0';
			    port = htons( (short)atoi( r ));
			} else {
			    port = defport;   
			}

			if (( rc = ldap_connect_to_host( sb, curhost, 0L,
			    port, async )) != -1 ) {
				break;
			}
		}
	}

	if ( rc == -1 ) {
		return( rc );
	}

	if ( krbinstancep != NULL ) {
#ifdef KERBEROS
		if (( *krbinstancep = ldap_host_connected_to( sb )) != NULL &&
		    ( p = strchr( *krbinstancep, '.' )) != NULL ) {
			*p = '\0';
		}
#else /* KERBEROS */
		krbinstancep = NULL;
#endif /* KERBEROS */
	}

	return( 0 );
}
