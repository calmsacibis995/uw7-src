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

extern int		nbackends;
extern Backend		*backends;
extern char		*default_referral;
extern char		*slapd_config_dn;

/*
 * no mutex protection in here - take our chances!
 */

void
config_info( Connection *conn, Operation *op )
{
	Entry		*e;
	char		buf[BUFSIZ];
	struct berval	val;
	struct berval	*vals[2];
	int		i, j;

	vals[0] = &val;
	vals[1] = NULL;

	e = (Entry *) ch_calloc( 1, sizeof(Entry) );
	if ( e == NULL ) { return; }

	e->e_attrs = NULL;
	e->e_dn = strdup( slapd_config_dn );

	for ( i = 0; i < nbackends; i++ ) {
		strcpy( buf, backends[i].be_type );
		for ( j = 0; backends[i].be_suffix[j] != NULL; j++ ) {
			strcat( buf, " : " );
			strcat( buf, backends[i].be_suffix[j] );
		}
		val.bv_val = buf;
		val.bv_len = strlen( buf );
		attr_merge( e, "database", vals );
	}

	if ( default_referral != NULL ) {
		strcpy( buf, default_referral );
		val.bv_val = buf;
		val.bv_len = strlen( buf );
		attr_merge( e, "database", vals );
	}

	send_search_entry( &backends[0], conn, op, e, NULL, 0 );
	send_ldap_search_result( conn, op, LDAP_SUCCESS, NULL, NULL, 1 );

	entry_free( e );
}
