/* @(#)search.c	1.4
 *
 * search.c - ldbm backend search function
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"

extern time_t		currenttime;
extern pthread_mutex_t	currenttime_mutex;

extern ID		dn2id();
extern IDList		*idl_alloc();
extern Entry		*id2entry();
extern Entry		*dn2entry();
extern Attribute	*attr_find();
extern IDList		*filter_candidates();
extern char		*ch_realloc();
extern char		*dn_parent();

static IDList	*base_candidates();
static IDList	*onelevel_candidates();
static IDList	*subtree_candidates();

#define GRABSIZE	BUFSIZ

#define MAKE_SPACE( n ) { \
	if ( rcur + n > rbuf + rmaxsize ) { \
		int	offset = rcur - rbuf; \
		rbuf =  ch_realloc( rbuf, rmaxsize + GRABSIZE ); \
		if ( rbuf == NULL ) { return( -1 ); } \
		rmaxsize += GRABSIZE; \
		rcur = rbuf + offset; \
	} \
}

int
ldbm_back_search(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    char	*base,
    int		scope,
    int		deref,
    int		slimit,
    int		tlimit,
    Filter	*filter,
    char	*filterstr,
    char	**attrs,
    int		attrsonly
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	int		err;
	time_t		stoptime;
	IDList		*candidates;
	ID		id;
	Entry		*e;
	Attribute	*ref;
	char		*matched = NULL;
	int		rmaxsize, nrefs;
	char		*rbuf, *rcur, *r;
	int		nentries = 0;

	if ( tlimit == 0 && be_isroot( be, op->o_dn ) ) {
		tlimit = -1;	/* allow root to set no limit */
	} else {
		tlimit = (tlimit > be->be_timelimit || tlimit < 1) ?
		    be->be_timelimit : tlimit;
		stoptime = op->o_time + tlimit;
	}
	if ( slimit == 0 && be_isroot( be, op->o_dn ) ) {
		slimit = -1;	/* allow root to set no limit */
	} else {
		slimit = (slimit > be->be_sizelimit || slimit < 1) ?
		    be->be_sizelimit : slimit;
	}

	switch ( scope ) {
	case LDAP_SCOPE_BASE:
		candidates = base_candidates( be, conn, op, base, filter,
		    attrs, attrsonly, &matched, &err );
		break;

	case LDAP_SCOPE_ONELEVEL:
		candidates = onelevel_candidates( be, conn, op, base, filter,
		    attrs, attrsonly, &matched, &err );
		break;

	case LDAP_SCOPE_SUBTREE:
		candidates = subtree_candidates( be, conn, op, base, filter,
		    attrs, attrsonly, &matched, NULL, &err, 1 );
		break;

	default:
		send_ldap_result( conn, op, LDAP_PROTOCOL_ERROR, "",
		    "Bad scope" );
		return( -1 );
	}

	/* null candidates means we could not find the base object */
	if ( candidates == NULL ) {
		send_ldap_result( conn, op, err, matched, "" );
		if ( matched != NULL ) {
			free( matched );
		}
		return( -1 );
	}

	rmaxsize = 0;
	nrefs = 0;
	rbuf = rcur = NULL;
	MAKE_SPACE( sizeof("Referral:") + 1 );
	strcpy( rbuf, "Referral:" );
	rcur = strchr( rbuf, '\0' );
	for ( id = idl_firstid( candidates ); id != NOID;
	    id = idl_nextid( candidates, id ) ) {
		/* check for abandon */
		pthread_mutex_lock( &op->o_abandonmutex );
		if ( op->o_abandon ) {
			pthread_mutex_unlock( &op->o_abandonmutex );
			idl_free( candidates );
			free( rbuf );
			return( 0 );
		}
		pthread_mutex_unlock( &op->o_abandonmutex );

		/* check time limit */
		pthread_mutex_lock( &currenttime_mutex );
		time( &currenttime );
		if ( tlimit != -1 && currenttime > stoptime ) {
			pthread_mutex_unlock( &currenttime_mutex );
			send_ldap_search_result( conn, op,
			    LDAP_TIMELIMIT_EXCEEDED, NULL, nrefs > 0 ? rbuf :
			    NULL, nentries );
			idl_free( candidates );
			free( rbuf );
			return( 0 );
		}
		pthread_mutex_unlock( &currenttime_mutex );

		/* get the entry */
		if ( (e = id2entry( be, id )) == NULL ) {

			logDebug( LDAP_LOG_LDBMBE,
			    "(ldbm_back_search) candidate %d not found\n",
			    id, 0, 0 );

			continue;
		}

		/*
		 * if it's a referral, add it to the list of referrals. only do
		 * this for subtree searches, and don't check the filter 
		 * explicitly here since it's only a candidate anyway.
		 */
		if ( e->e_dn != NULL && strncasecmp( e->e_dn, "ref=", 4 )
		    == 0 && (ref = attr_find( e->e_attrs, "ref" )) != NULL &&
		    scope == LDAP_SCOPE_SUBTREE )
		{
			int	i, len;

			if ( ref->a_vals == NULL ) {

				logDebug( LDAP_LOG_LDBMBE,
			 	  "(ldbm_back_search) null referral in (%s)\n",
				  e->e_dn,0,0);

			} else {
				for ( i = 0; ref->a_vals[i] != NULL; i++ ) {
					/* referral + newline + null */
					MAKE_SPACE( ref->a_vals[i]->bv_len + 2 );
					*rcur++ = '\n';
					strncpy( rcur, ref->a_vals[i]->bv_val,
					  ref->a_vals[i]->bv_len );
					rcur = rcur + ref->a_vals[i]->bv_len;
					*rcur = '\0';
					nrefs++;
				}
			}

		/* otherwise it's an entry - see if it matches the filter */
		} else {
			/* if it matches the filter and scope, send it */
			if ( test_filter( be, conn, op, e, filter ) == 0 ) {
				int		scopeok;
				char	*dn;

				/* check scope */
				scopeok = 1;
				if ( scope == LDAP_SCOPE_ONELEVEL ) {
					if ( (dn = dn_parent( be, e->e_dn )) != NULL ) {
						(void) dn_normalize( dn );
						scopeok = (dn == base) ? 1 : (! strcasecmp( dn, base ));
					} else {
						scopeok = (base == NULL || *base == '\0');
					}
					free( dn );
				} else if ( scope == LDAP_SCOPE_SUBTREE ) {
					dn = strdup( e->e_dn );
					(void) dn_normalize( dn );
					scopeok = dn_issuffix( dn, base );
					free( dn );
				}

				if ( scopeok ) {
					/* check size limit */
					if ( --slimit == -1 ) {
						cache_return_entry( &li->li_cache, e );
						send_ldap_search_result( conn, op,
							LDAP_SIZELIMIT_EXCEEDED, NULL,
							nrefs > 0 ? rbuf : NULL, nentries );
						idl_free( candidates );
						free( rbuf );
						return( 0 );
					}

					switch ( send_search_entry( be, conn, op, e,
						attrs, attrsonly ) ) {
					case 0:		/* entry sent ok */
						nentries++;
						break;
					case 1:		/* entry not sent */
						break;
					case -1:	/* connection closed */
						cache_return_entry( &li->li_cache, e );
						idl_free( candidates );
						free( rbuf );
						return( 0 );
					}
				}
			}
		}

		cache_return_entry( &li->li_cache, e );

		pthread_yield();
	}
	idl_free( candidates );
	if ( nrefs > 0 ) {
		send_ldap_search_result( conn, op, LDAP_PARTIAL_RESULTS, NULL,
		    rbuf, nentries );
	} else {
		send_ldap_search_result( conn, op, LDAP_SUCCESS, NULL, NULL,
		    nentries );
	}
	free( rbuf );

	return( 0 );
}

static IDList *
base_candidates(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    char	*base,
    Filter	*filter,
    char	**attrs,
    int		attrsonly,
    char	**matched,
    int		*err
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	int		rc;
	ID		id;
	IDList		*idl;
	Entry		*e;

	*err = LDAP_SUCCESS;
	if ( (e = dn2entry( be, base, matched )) == NULL ) {
		*err = LDAP_NO_SUCH_OBJECT;
		return( NULL );
	}

	idl = idl_alloc( 1 );
	if ( idl_insert( &idl, e->e_id, 1 ) == -1 ) {
		*err = LDAP_NO_MEMORY;
		return( NULL );
	}

	cache_return_entry( &li->li_cache, e );

	return( idl );
}

static IDList *
onelevel_candidates(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    char	*base,
    Filter	*filter,
    char	**attrs,
    int		attrsonly,
    char	**matched,
    int		*err
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	Entry		*e;
	Filter		*f;
	char		buf[20];
	IDList		*candidates;

	*err = LDAP_SUCCESS;
	e = NULL;
	/* get the base object */
	if ( base != NULL && *base != '\0' && (e = dn2entry( be, base,
	    matched )) == NULL ) {
		*err = LDAP_NO_SUCH_OBJECT;
		return( NULL );
	}

	/*
	 * modify the filter to be something like this:
	 *
	 *	parent=baseobject & originalfilter
	 */

	f = (Filter *) ch_malloc( sizeof(Filter) );
	if ( f == NULL ) {
		*err = LDAP_NO_MEMORY;
		return( NULL );
	}
	f->f_next = NULL;
	f->f_choice = LDAP_FILTER_AND;
	f->f_and = (Filter *) ch_malloc( sizeof(Filter) );
	if ( f->f_and == NULL ) {
		*err = LDAP_NO_MEMORY;
		return( NULL );
	}

	f->f_and->f_choice = LDAP_FILTER_EQUALITY;
	f->f_and->f_ava.ava_type = strdup( "id2children" );
	sprintf( buf, "%d", e != NULL ? e->e_id : 0 );
	f->f_and->f_ava.ava_value.bv_val = strdup( buf );
	f->f_and->f_ava.ava_value.bv_len = strlen( buf );
	f->f_and->f_next = filter;

	/* from here, it's just like subtree_candidates */
	candidates = subtree_candidates( be, conn, op, base, f, attrs,
	    attrsonly, matched, e, err, 0 );

	/* free up just the filter stuff we allocated above */
	f->f_and->f_next = NULL;
	filter_free( f );

	return( candidates );
}

static IDList *
subtree_candidates(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    char	*base,
    Filter	*filter,
    char	**attrs,
    int		attrsonly,
    char	**matched,
    Entry	*e,
    int		*err,
    int		lookupbase
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	Filter		*f;
	IDList		*candidates;

	/*
	 * get the base object - unless we already have it (from one-level).
	 * also, unless this is a one-level search or a subtree search
	 * starting at the very top of our subtree, we need to modify the
	 * filter to be something like this:
	 *
	 *	dn=*baseobjectdn & (originalfilter | ref=*)
	 *
	 * the "objectclass=referral" part is used to select referrals to return
	 */

	*err = LDAP_SUCCESS;
	f = NULL;
	if ( lookupbase ) {
		if ( base != NULL && *base != '\0' && (e = dn2entry( be, base,
		    matched )) == NULL ) {
			*err = LDAP_NO_SUCH_OBJECT;
			return( NULL );
	 	}

		f = (Filter *) ch_malloc( sizeof(Filter) );
		if ( f == NULL ) {
			*err = LDAP_NO_MEMORY;
			return( NULL );
		}
		f->f_next = NULL;
		f->f_choice = LDAP_FILTER_OR;
		f->f_or = (Filter *) ch_malloc( sizeof(Filter) );
		if ( f->f_or == NULL ) {
			*err = LDAP_NO_MEMORY;
			return( NULL );
		}
		f->f_or->f_choice = LDAP_FILTER_EQUALITY;
		f->f_or->f_avtype = strdup( "objectclass" );
		/* Patch to use normalized uppercase */
		f->f_or->f_avvalue.bv_val = strdup( "REFERRAL" );
		f->f_or->f_avvalue.bv_len = strlen( "REFERRAL" );
		f->f_or->f_next = filter;
		filter = f;

		if ( ! be_issuffix( be, base ) ) {
			f = (Filter *) ch_malloc( sizeof(Filter) );
			if ( f == NULL ) {
				*err = LDAP_NO_MEMORY;
				return( NULL );
			}
			f->f_next = NULL;
			f->f_choice = LDAP_FILTER_AND;
			f->f_and = (Filter *) ch_malloc( sizeof(Filter) );
			if ( f->f_and == NULL ) {
				*err = LDAP_NO_MEMORY;
				return( NULL );
			}
			f->f_and->f_choice = LDAP_FILTER_SUBSTRINGS;
			f->f_and->f_sub_type = strdup( "dn" );
			f->f_and->f_sub_initial = NULL;
			f->f_and->f_sub_any = NULL;
			f->f_and->f_sub_final = strdup( base );
			value_normalize( f->f_and->f_sub_final, SYNTAX_CIS );
			f->f_and->f_next = filter;
			filter = f;
		}
	}

	candidates = filter_candidates( be, filter );

	/* free up just the parts we allocated above */
	if ( f != NULL ) {
		f->f_and->f_next = NULL;
		filter_free( f );
	}

	if ( e != NULL ) {
		cache_return_entry( &li->li_cache, e );
	}

	return( candidates );
}
