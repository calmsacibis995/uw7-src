/* @(#)init.c	1.4
 *
 * init.c - initialize ldbm backend
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"

/* Messages */
#define MSG_MEMERR1 \
    1,93,"Cannot allocate memory whilst initialising backend\n"


ldbm_back_init(
    Backend	*be
)
{
	struct ldbminfo	*li;
	char		*argv[ 4 ];
	int		i;

	/* allocate backend-specific stuff */
	li = (struct ldbminfo *) ch_calloc( 1, sizeof(struct ldbminfo) );

	if ( li == NULL ) {
		logError( get_ldap_message( MSG_MEMERR1 ));
		exit( LDAP_NO_MEMORY );
	}

	/* arrange to read nextid later (on first request for it) */
	li->li_nextid = -1;

	/* default cache size */
	li->li_cache.c_maxsize = DEFAULT_CACHE_SIZE;

	/* default database cache size */
	li->li_dbcachesize = DEFAULT_DBCACHE_SIZE;

	/* default min maxids */
	li->li_min_maxids = DEFAULT_MIN_MAXIDS;

	/* default file creation mode */
	li->li_mode = DEFAULT_MODE;

	/* default database directory */
	li->li_directory = DEFAULT_DB_DIRECTORY;

	/* always index dn, id2children, objectclass (used in some searches) */
	argv[ 0 ] = "dn";
	argv[ 1 ] = "dn";
	argv[ 2 ] = NULL;
	attr_syntax_config( "ldbm dn initialization", 0, 2, argv );
	argv[ 0 ] = "dn";
	argv[ 1 ] = "sub";
	argv[ 2 ] = "eq";
	argv[ 3 ] = NULL;
	attr_index_config( li, "ldbm dn initialization", 0, 3, argv, 1 );
	argv[ 0 ] = "id2children";
	argv[ 1 ] = "eq";
	argv[ 2 ] = NULL;
	attr_index_config( li, "ldbm id2children initialization", 0, 2, argv,
	    1 );
	argv[ 0 ] = "objectclass";
	argv[ 1 ] = strdup( "pres,eq" );
	argv[ 2 ] = NULL;
	attr_index_config( li, "ldbm objectclass initialization", 0, 2, argv,
	    1 );
	free( argv[ 1 ] );

	/* initialize various mutex locks & condition variables */
	pthread_mutex_init( &li->li_cache.c_mutex, pthread_mutexattr_default );
	pthread_mutex_init( &li->li_nextid_mutex, pthread_mutexattr_default );
	pthread_mutex_init( &li->li_dbcache_mutex, pthread_mutexattr_default );
	pthread_cond_init( &li->li_dbcache_cv, pthread_condattr_default );
	for ( i = 0; i < MAXDBCACHE; i++ ) {
		pthread_mutex_init( &li->li_dbcache[i].dbc_mutex,
		    pthread_mutexattr_default );
		pthread_cond_init( &li->li_dbcache[i].dbc_cv,
		    pthread_condattr_default );
	}

	be->be_private = li;
}
