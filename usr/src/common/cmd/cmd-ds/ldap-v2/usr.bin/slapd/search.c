#ident	"@(#)search.c	1.6"

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
#include "ldapconfig.h"

extern int	get_filter();
extern Backend	*select_backend();

extern char	*default_referral;

extern char *slapd_config_dn;
extern char *slapd_monitor_dn;

void
do_search( conn, op )
    Connection	*conn;	/* where to send results 		       */
    Operation	*op;	/* info about the op to which we're responding */
{
	int		i, err;
	int		scope, deref, attrsonly;
	int		sizelimit, timelimit;
	char		*base, *fstr;
	Filter		*filter;
	char		**attrs;
	Backend		*be;

	logDebug( LDAP_LOG_CLNT, "=> do_search\n", 0, 0, 0 );

	/*
	 * Parse the search request.  It looks like this:
	 *
	 *	SearchRequest := [APPLICATION 3] SEQUENCE {
	 *		baseObject	DistinguishedName,
	 *		scope		ENUMERATED {
	 *			baseObject	(0),
	 *			singleLevel	(1),
	 *			wholeSubtree	(2)
	 *		},
	 *		derefAliases	ENUMERATED {
	 *			neverDerefaliases	(0),
	 *			derefInSearching	(1),
	 *			derefFindingBaseObj	(2),
	 *			alwaysDerefAliases	(3)
	 *		},
	 *		sizelimit	INTEGER (0 .. 65535),
	 *		timelimit	INTEGER (0 .. 65535),
	 *		attrsOnly	BOOLEAN,
	 *		filter		Filter,
	 *		attributes	SEQUENCE OF AttributeType
	 *	}
	 */

	/* baseObject, scope, derefAliases, sizelimit, timelimit, attrsOnly */
	if ( ber_scanf( op->o_ber, "{aiiiib", &base, &scope, &deref, &sizelimit,
	    &timelimit, &attrsonly ) == LBER_ERROR ) {
		send_ldap_result( conn, op, LDAP_PROTOCOL_ERROR, NULL, "" );
		logDebug( LDAP_LOG_CLNT, "<= do_search\n", 0, 0, 0 );
		return;
	}
	if ( scope != LDAP_SCOPE_BASE && scope != LDAP_SCOPE_ONELEVEL
	    && scope != LDAP_SCOPE_SUBTREE ) {
		free( base );
		send_ldap_result( conn, op, LDAP_PROTOCOL_ERROR, NULL,
		    "Unknown search scope" );
		logDebug( LDAP_LOG_CLNT, "<= do_search\n", 0, 0, 0 );
		return;
	}
	(void) dn_normalize( base );

	logDebug( LDAP_LOG_CLNT, 
	    "(do_search) base(%s) scope(%d) deref(%d)\n",
	    base, scope, deref );

	logDebug( LDAP_LOG_CLNT,
	    "(do_search) sizelimit(%d) timelimit(%d) attrsonly(%d)\n",
	    sizelimit, timelimit, attrsonly);

	/* filter - returns a "normalized" version */
	filter = NULL;
	fstr = NULL;
	if ( (err = get_filter( conn, op->o_ber, &filter, &fstr )) != 0 ) {
		if ( fstr != NULL ) {
			free( fstr );
		}
		free( base );
		send_ldap_result( conn, op, err, NULL, "Bad search filter" );
		logDebug( LDAP_LOG_CLNT, "<= do_search\n", 0, 0, 0 );
		return;
	}
	logDebug( LDAP_LOG_CLNT, "(do_search) search filter(%s)\n",fstr,0,0);

	/* attributes */
	attrs = NULL;
	if ( ber_scanf( op->o_ber, "{v}}", &attrs ) == LBER_ERROR ) {
		free( base );
		free( fstr );
		send_ldap_result( conn, op, LDAP_PROTOCOL_ERROR, NULL, "" );
		logDebug( LDAP_LOG_CLNT, "<= do_search\n", 0, 0, 0 );
		return;
	}
	logDebug( LDAP_LOG_CLNT, "(do_search) attrs:", 0, 0, 0 );
	if ( attrs != NULL ) {
		for ( i = 0; attrs[i] != NULL; i++ ) {
			attr_normalize( attrs[i] );
			logDebug( LDAP_LOG_CLNT, " %s", attrs[i], 0, 0 );
		}
	}
	logDebug( LDAP_LOG_CLNT, "\n", 0, 0, 0 );

	/*
	 * See if it's a request for one of the special monitoring DNs
	 */
	if ( scope == LDAP_SCOPE_BASE ) {
		if ((slapd_monitor_dn != NULL)  
		&&  ( strcasecmp( base, slapd_monitor_dn ) == 0 )) {
			free( base );
			free( fstr );
			monitor_info( conn, op );
			logDebug( LDAP_LOG_CLNT, "<= do_search\n", 0, 0, 0 );
			return;
		}
		if ((slapd_config_dn != NULL)  
		&&  (strcasecmp( base, slapd_config_dn ) == 0 )) {
			free( base );
			free( fstr );
			config_info( conn, op );
			logDebug( LDAP_LOG_CLNT, "<= do_search\n", 0, 0, 0 );
			return;
		}
	}

	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */
	if ( (be = select_backend( base )) == NULL ) {
		send_ldap_result( conn, op, LDAP_PARTIAL_RESULTS, NULL,
		    default_referral );

		free( base );
		free( fstr );
		filter_free( filter );
		if ( attrs != NULL ) {
			charray_free( attrs );
		}
		logDebug( LDAP_LOG_CLNT, "<= do_search\n", 0, 0, 0 );
		return;
	}

	/* actually do the search and send the result(s) */
	if ( be->be_search != NULL ) {
		(*be->be_search)( be, conn, op, base, scope, deref, sizelimit,
		    timelimit, filter, fstr, attrs, attrsonly );
	} else {
		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM, NULL,
		    "Function not implemented" );
	}

	free( base );
	free( fstr );
	filter_free( filter );
	if ( attrs != NULL ) {
		charray_free( attrs );
	}
	logDebug( LDAP_LOG_CLNT, "<= do_search\n", 0, 0, 0 );
}
