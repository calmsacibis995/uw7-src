/* @(#)id2children.c	1.3
 * 
 * id2children.c - routines to deal with the id2children index
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"

/* Messages */
#define MSG_NOCREATEID2CHILD \
    1,89,"Could not open database file %s%s%s\n"

struct dbcache	*ldbm_cache_open();
extern Datum	ldbm_cache_fetch();
IDList		*idl_fetch();

int
id2children_add(
    Backend	*be,
    Entry	*p,
    Entry	*e
)
{
	struct ldbminfo *li = (struct ldbminfo *) be->be_private;
	struct dbcache	*db;
	Datum		key, data;
	int		len, rc;
	IDList		*idl;
	char		buf[20];

	logDebug( LDAP_LOG_LDBMBE, 
	    "=> id2children_add( %d, %d )\n", p ? p->e_id : 0, e->e_id, 0 );

	if ( (db = ldbm_cache_open( be, "id2children", LDBM_SUFFIX,
	    LDBM_WRCREAT )) == NULL ) {

		logError( get_ldap_message(MSG_NOCREATEID2CHILD,
                    li->li_directory,"id2children",LDBM_SUFFIX));

		return( -1 );
	}

	sprintf( buf, "%c%d", EQ_PREFIX, p ? p->e_id : 0 );
	key.dptr = buf;
	key.dsize = strlen( buf ) + 1;

	if ( idl_insert_key( be, db, key, e->e_id ) != 0 ) {

		logDebug( LDAP_LOG_LDBMBE,
		    "<= id2children_add (idl_insert failed)\n",
		    0, 0, 0 );

		ldbm_cache_close( be, db );
		return( -1 );
	}

	ldbm_cache_close( be, db );

	logDebug( LDAP_LOG_LDBMBE, "<= id2children_add\n",0,0,0);
	return( 0 );
}

int
has_children(
    Backend	*be,
    Entry	*p
)
{
	struct ldbminfo *li = (struct ldbminfo *) be->be_private;
	struct dbcache	*db;
	Datum		key;
	int		rc;
	IDList		*idl;
	char		buf[20];

	logDebug( LDAP_LOG_LDBMBE,
	    "=> has_children( %d )\n",p->e_id,0,0);

	if ( (db = ldbm_cache_open( be, "id2children", LDBM_SUFFIX,
	    LDBM_WRCREAT )) == NULL ) {

		logError( get_ldap_message(MSG_NOCREATEID2CHILD,
		    li->li_directory,"id2children",LDBM_SUFFIX));

		return( 0 );
	}

	sprintf( buf, "%c%d", EQ_PREFIX, p->e_id );
	key.dptr = buf;
	key.dsize = strlen( buf ) + 1;

	idl = idl_fetch( be, db, key );

	ldbm_cache_close( be, db );
	rc = idl ? 1 : 0;
	idl_free( idl );

	logDebug( LDAP_LOG_LDBMBE, "<= has_children (%d)\n", rc, 0, 0 );
	return( rc );
}
