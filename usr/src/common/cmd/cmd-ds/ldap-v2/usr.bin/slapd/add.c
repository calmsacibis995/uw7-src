/* @(#)add.c	1.5
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
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"

extern Backend	*select_backend();
extern char	*dn_normalize();

extern char		*default_referral;
extern time_t		currenttime;
extern pthread_mutex_t	currenttime_mutex;
extern int		global_lastmod;

static void	add_created_attrs();
extern char	strftime_fmt[];	/* format string for strftime, */
				/* declared in one place only */

/*
 * do_add (Connection  *conn, Operation   *op)
 *
 *	Perform the LDAP ADD operation.
 *
 *	This function gets the ADD request off the wire and passes
 *	the information as an Entry struct to the appropriate
 *	backend database.
 *
 */
void
do_add( conn, op )
    Connection	*conn;
    Operation	*op;
{
	BerElement	*ber = op->o_ber;
	char		*dn, *last;
	unsigned long	len, tag;
	Entry		*e;
	Backend		*be;
	char		mBuf[BUFSIZ];

	logDebug(LDAP_LOG_CLNT,"=>do_add\n",0,0,0);

	/*
	 * Parse the add request.  It looks like this:
	 *
	 *	AddRequest := [APPLICATION 14] SEQUENCE {
	 *		name	DistinguishedName,
	 *		attrs	SEQUENCE OF SEQUENCE {
	 *			type	AttributeType,
	 *			values	SET OF AttributeValue
	 *		}
	 *	}
	 */

	e = (Entry *) ch_calloc( 1, sizeof(Entry) );
	if ( e == NULL ) {
		logDebug(LDAP_LOG_CLNT,
		    "(do_add) Cannot allocate Entry struct\n",0,0,0);
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, NULL,
		     "server memory allocation error");

		logDebug(LDAP_LOG_CLNT,"<=do_add\n",0,0,0);
		return;
	}

	/* get the name */
	if ( ber_scanf( ber, "{a", &dn ) == LBER_ERROR ) {
		logDebug(LDAP_LOG_CLNT,
		    "(do_add) ber_scanf failed to get DN\n",0,0,0);
		send_ldap_result( conn, op, LDAP_PROTOCOL_ERROR, NULL,
		    "error decoding DN" );

		entry_free( e );

		logDebug(LDAP_LOG_CLNT,"<=do_add\n",0,0,0);
		return;
	}
	e->e_dn = dn;
	dn = dn_normalize( strdup( dn ) );
	if ( dn == NULL ) {
		logDebug(LDAP_LOG_CLNT,
		    "(do_add) strdup failed on dn\n",0,0,0);
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, NULL,
		    "server memory allocation error");

		entry_free( e );

		logDebug(LDAP_LOG_CLNT,"<=do_add\n",0,0,0);
		return;
	}

	logDebug(LDAP_LOG_CLNT,"(do_add) dn (%s)\n", dn, 0, 0 );

	/* get the attrs */
	e->e_attrs = NULL;
	for ( tag = ber_first_element( ber, &len, &last ); tag != LBER_DEFAULT;
	    tag = ber_next_element( ber, &len, last ) ) {
		char		*type;
		struct berval	**vals;

		type = NULL;
		vals = NULL;

		if ( ber_scanf( ber, "{a{V}}", &type, &vals ) == LBER_ERROR ) {
			logDebug(LDAP_LOG_CLNT,
			    "(do_add) ber_scanf failed to get attribute\n",0,0,0);
			sprintf(mBuf,"(dn %s) error decoding attributes",dn);
			send_ldap_result(conn,op,LDAP_PROTOCOL_ERROR,NULL,mBuf);

			if ( type != NULL ) free( type );
			if ( vals != NULL ) ber_bvecfree( vals );
			free( dn );
			entry_free( e );
			
			logDebug(LDAP_LOG_CLNT,"<=do_add\n",0,0,0);
			return;
		}

		if ( type == NULL ) {
			sprintf(mBuf,"(dn %s) error decoding attributes",dn);
			send_ldap_result(conn,op,LDAP_PROTOCOL_ERROR,NULL,mBuf);

			if ( vals == NULL ) ber_bvecfree( vals );
			free( dn );
			entry_free( e );

			logDebug(LDAP_LOG_CLNT,"<=do_add\n",0,0,0);
			return;
		}

		if ( vals == NULL ) {
			logDebug( LDAP_LOG_CLNT,
			    "(do_add) no values for attribute %s\n", type, 0, 0 );

			sprintf(mBuf,"(dn %s) No value for attribute %s",dn,type);
			send_ldap_result(conn,op,LDAP_PROTOCOL_ERROR,NULL,mBuf);

			free( type );
			free( dn );
			entry_free( e );

			logDebug(LDAP_LOG_CLNT,"<=do_add\n",0,0,0);
			return;
		}

		attr_merge( e, type, vals );
		free( type );
		ber_bvecfree( vals );
	}

	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */
	if ( (be = select_backend( dn )) == NULL ) {
		send_ldap_result( conn, op, LDAP_PARTIAL_RESULTS, NULL,
		    default_referral );

		free( dn );
		entry_free( e );
		logDebug(LDAP_LOG_CLNT,"<=do_add (no backend db)\n",
		    0,0,0);
		return;
	}

	/*
	 * do the add if 1 && (2 || 3)
	 * 1) there is an add function implemented in this backend;
	 * 2) this backend is master for what it holds;
	 * 3) it's a replica and the dn supplied is the updatedn.
	 */
	if ( be->be_add != NULL ) {
		/* do the update here */
		if ( be->be_updatedn == NULL || strcasecmp( be->be_updatedn,
		    op->o_dn ) == 0 ) {
			if ( (be->be_lastmod == ON || be->be_lastmod == 0 &&
			    global_lastmod == ON) && be->be_updatedn == NULL ) {
				add_created_attrs( op, e );
			}
		
			logDebug(LDAP_LOG_CLNT,
			    "(do_add) Call backend specific add\n",0,0,0);
			/* Call backend specific ADD here */
			if ( (*be->be_add)( be, conn, op, e ) == 0 ) {
				replog( be, LDAP_REQ_ADD, e->e_dn, e, 0 );
			}
		} else {
			entry_free( e );
			send_ldap_result( conn, op, LDAP_PARTIAL_RESULTS, NULL,
			    default_referral );
		}
	} else {
		entry_free( e );
		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM, NULL,
		    "Function not implemented" );
	}
	logDebug(LDAP_LOG_CLNT,"<=do_add\n",0,0,0);

}

static void
add_created_attrs( Operation *op, Entry *e )
{
	char		buf[20];
	struct berval	bv;
	struct berval	*bvals[2];
	Attribute	**a, **next;
	Attribute	*tmp;
	struct tm	*ltm;

	bvals[0] = &bv;
	bvals[1] = NULL;

	/* remove any attempts by the user to add these attrs */
	for ( a = &e->e_attrs; *a != NULL; a = next ) {
		if ( strcasecmp( (*a)->a_type, "createtimestamp" ) == 0
		    || strcasecmp( (*a)->a_type, "creatorsname" ) == 0 ) {
			tmp = *a;
			*a = (*a)->a_next;
			attr_free( tmp );
			next = a;
		} else {
			next = &(*a)->a_next;
		}
	}

	if ( op->o_dn == NULL || op->o_dn[0] == '\0' ) {
		bv.bv_val = "NULLDN";
		bv.bv_len = strlen( bv.bv_val );
	} else {
		bv.bv_val = op->o_dn;
		bv.bv_len = strlen( bv.bv_val );
	}
	attr_merge( e, "creatorsname", bvals );

	pthread_mutex_lock( &currenttime_mutex );
        ltm = localtime( &currenttime );
        strftime( buf, sizeof(buf), strftime_fmt, ltm );
	pthread_mutex_unlock( &currenttime_mutex );

	bv.bv_val = buf;
	bv.bv_len = strlen( bv.bv_val );
	attr_merge( e, "createtimestamp", bvals );
}
