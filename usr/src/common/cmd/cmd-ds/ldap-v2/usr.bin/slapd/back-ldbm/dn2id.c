/* @(#)dn2id.c	1.3
 * 
 * dn2id.c - routines to deal with the dn2id index
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"

/* Messages */
#define MSG_NOCREATEDN2ID \
    1,88,"Could not open database file %s%s%s\n"


extern struct dbcache	*ldbm_cache_open();
extern Entry		*cache_find_entry_dn();
extern Entry		*id2entry();
extern char		*dn_parent();
extern Datum		ldbm_cache_fetch();

int
dn2id_add(
    Backend	*be,
    char	*dn,
    ID		id
)
{
	struct ldbminfo *li = (struct ldbminfo *) be->be_private;
	int		rc;
	struct dbcache	*db;
	Datum		key, data;

	logDebug( LDAP_LOG_LDBMBE, "=> dn2id_ad\n",0,0,0);
	logDebug( LDAP_LOG_LDBMBE, 
	    "(dn2id_add) dn(%s), id(%ld)\n",dn,id,0 );

	if ( (db = ldbm_cache_open( be, "dn2id", LDBM_SUFFIX, LDBM_WRCREAT ))
	    == NULL ) {

		logError( get_ldap_message(MSG_NOCREATEDN2ID,
		    li->li_directory,"dn2id",LDBM_SUFFIX));

		logDebug( LDAP_LOG_LDBMBE, "<= dn2id_add\n", 0, 0, 0 );
		return( -1 );
	}

	dn = strdup( dn );
	dn_normalize_case( dn );

	key.dptr = dn;
	key.dsize = strlen( dn ) + 1;
	data.dptr = (char *) &id;
	data.dsize = sizeof(ID);

	rc = ldbm_cache_store( db, key, data, LDBM_INSERT );

	free( dn );
	ldbm_cache_close( be, db );

	logDebug( LDAP_LOG_LDBMBE, "<= dn2id_add res(%d)\n", rc, 0, 0 );
	return( rc );
}

ID
dn2id(
    Backend	*be,
    char	*dn
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	struct dbcache	*db;
	Entry		*e;
	ID		id;
	Datum		key, data;

	logDebug( LDAP_LOG_LDBMBE, "=> dn2id, dn(%s)\n",dn,0,0);

	dn = strdup( dn );
	dn_normalize_case( dn );

	/* first check the cache */
	if ( (e = cache_find_entry_dn( &li->li_cache, dn )) != NULL ) {
		id = e->e_id;
		free( dn );

		logDebug( LDAP_LOG_LDBMBE,
		    "<= dn2id %d (in cache)\n", e->e_id,0,0);

		cache_return_entry( &li->li_cache, e );

		return( id );
	}

	if ( (db = ldbm_cache_open( be, "dn2id", LDBM_SUFFIX, LDBM_WRCREAT ))
	    == NULL ) {
		free( dn );

		logError( get_ldap_message(MSG_NOCREATEDN2ID,
		    li->li_directory,"dn2id",LDBM_SUFFIX));

		logDebug( LDAP_LOG_LDBMBE,"<= dn2id NOID\n",0,0,0);

		return( NOID );
	}

	key.dptr = dn;
	key.dsize = strlen( dn ) + 1;

	data = ldbm_cache_fetch( db, key );

	ldbm_cache_close( be, db );
	free( dn );

	if ( data.dptr == NULL ) {
		logDebug( LDAP_LOG_LDBMBE,"<= dn2id NOID\n",0,0,0);
		return( NOID );
	}

	(void) memcpy( (char *) &id, data.dptr, sizeof(ID) );

	ldbm_datum_free( db->dbc_db, data );

	logDebug( LDAP_LOG_LDBMBE, "<= dn2id %d\n", id, 0, 0 );
	return( id );
}

int
dn2id_delete(
    Backend	*be,
    char	*dn
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	struct dbcache	*db;
	Datum		key;
	int		rc;

	logDebug(LDAP_LOG_LDBMBE,"=> dn2id_delete, dn(%s)\n",dn,0,0);

	if ( (db = ldbm_cache_open( be, "dn2id", LDBM_SUFFIX, LDBM_WRCREAT ))
	    == NULL ) {

		logError( get_ldap_message(MSG_NOCREATEDN2ID,
		    li->li_directory,"dn2id",LDBM_SUFFIX));

		logDebug(LDAP_LOG_LDBMBE,"<= dn2id_delete\n",0,0,0);

		return( -1 );
	}

	dn_normalize_case( dn );
	key.dptr = dn;
	key.dsize = strlen( dn ) + 1;

	rc = ldbm_cache_delete( db, key );

	ldbm_cache_close( be, db );

	logDebug( LDAP_LOG_LDBMBE, "<= dn2id_delete %d\n",rc,0,0);
	return( rc );
}

/*
 * dn2entry - look up dn in the cache/indexes and return the corresponding
 * entry.
 */

Entry *
dn2entry(
    Backend	*be,
    char	*dn,
    char	**matched
)
{
	struct ldbminfo *li = (struct ldbminfo *) be->be_private;
	ID		id;
	Entry		*e;
	char		*pdn;

	if ( (id = dn2id( be, dn )) != NOID && (e = id2entry( be, id ))
	    != NULL ) {
		return( e );
	}
	*matched = NULL;

	/* stop when we get to the suffix */
	if ( be_issuffix( be, dn ) ) {
		return( NULL );
	}

	/* entry does not exist - see how much of the dn does exist */
	if ( (pdn = dn_parent( be, dn )) != NULL ) {
		if ( (e = dn2entry( be, pdn, matched )) != NULL ) {
			*matched = pdn;
			cache_return_entry( &li->li_cache, e );
		} else {
			free( pdn );
		}
	}

	return( NULL );
}
