/* @(#)cache.c	1.3
 * 
 * cache.c - routines to maintain an in-core cache of entries
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"

static int	cache_delete_entry_internal();
#ifdef LDAP_DEBUG
static void	lru_print();
#endif

/*
 * the cache has three entry points (ways to find things):
 *
 * 	by entry	e.g., if you already have an entry from the cache
 *			and want to delete it. (really by entry ptr)
 *	by dn		e.g., when looking for the base object of a search
 *	by id		e.g., for search candidates
 *
 * these correspond to three different avl trees that are maintained.
 */

static int
cache_entry_cmp( Entry *e1, Entry *e2 )
{
	return( e1 < e2 ? -1 : (e1 > e2 ? 1 : 0) );
}

static int
cache_entrydn_cmp( Entry *e1, Entry *e2 )
{
	return( strcasecmp( e1->e_dn, e2->e_dn ) );
}

static int
cache_entryid_cmp( Entry *e1, Entry *e2 )
{
	return( e1->e_id < e2->e_id ? -1 : (e1->e_id > e2->e_id ? 1 : 0) );
}

void
cache_set_state( struct cache *cache, Entry *e, int state )
{
	/* set cache mutex */
	pthread_mutex_lock( &cache->c_mutex );

	e->e_state = state;

	/* free cache mutex */
	pthread_mutex_unlock( &cache->c_mutex );
}

void
cache_return_entry( struct cache *cache, Entry *e )
{
	/* set cache mutex */
	pthread_mutex_lock( &cache->c_mutex );

	if ( --e->e_refcnt == 0 && e->e_state == ENTRY_STATE_DELETED ) {
		entry_free( e );
	}

	/* free cache mutex */
	pthread_mutex_unlock( &cache->c_mutex );
}

#define LRU_DELETE( cache, e ) { \
	if ( e->e_lruprev != NULL ) { \
		e->e_lruprev->e_lrunext = e->e_lrunext; \
	} else { \
		cache->c_lruhead = e->e_lrunext; \
	} \
	if ( e->e_lrunext != NULL ) { \
		e->e_lrunext->e_lruprev = e->e_lruprev; \
	} else { \
		cache->c_lrutail = e->e_lruprev; \
	} \
}

#define LRU_ADD( cache, e ) { \
	e->e_lrunext = cache->c_lruhead; \
	if ( e->e_lrunext != NULL ) { \
		e->e_lrunext->e_lruprev = e; \
	} \
	cache->c_lruhead = e; \
	e->e_lruprev = NULL; \
	if ( cache->c_lrutail == NULL ) { \
		cache->c_lrutail = e; \
	} \
}

/*
 * cache_create_entry_lock - create an entry in the cache, and lock it.
 * returns:	0	entry has been created and locked
 *		1	entry already existed
 *		-1	something bad happened
 */
int
cache_add_entry_lock(
    struct cache	*cache,
    Entry		*e,
    int			state
)
{
	int	i, rc;
	Entry	*ee;

	/* set cache mutex */
	pthread_mutex_lock( &cache->c_mutex );

	if ( avl_insert( &cache->c_dntree, e, cache_entrydn_cmp, avl_dup_error )
	    != 0 ) {
		logDebug( LDAP_LOG_LDBMBE,
	    "(cache_add_entry_lock) entry %20s id %d already in dn cache\n",
		    e->e_dn, e->e_id, 0 );

		/* free cache mutex */
		pthread_mutex_unlock( &cache->c_mutex );
		return( 1 );
	}

	/* id tree */
	if ( avl_insert( &cache->c_idtree, e, cache_entryid_cmp, avl_dup_error )
	    != 0 ) {

		logDebug( LDAP_LOG_LDBMBE, 
	"(cache_add_entry_lock) entry %20s id %d already in id cache\n",
		    e->e_dn, e->e_id, 0 );

		/* delete from dn tree inserted above */
		if ( avl_delete( &cache->c_dntree, e, cache_entrydn_cmp )
		    == NULL ) {

		logDebug( LDAP_LOG_LDBMBE, 
	"(cache_add_entry_lock) can't delete from dn cache\n",0,0,0);

		}

		/* free cache mutex */
		pthread_mutex_unlock( &cache->c_mutex );
		return( -1 );
	}

	e->e_state = state;
	e->e_refcnt = 1;

	/* lru */
	LRU_ADD( cache, e );
	if ( ++cache->c_cursize > cache->c_maxsize ) {
		/*
		 * find the lru entry not currently in use and delete it.
		 * in case a lot of entries are in use, only look at the
		 * first 10 on the tail of the list.
		 */
		i = 0;
		while ( cache->c_lrutail != NULL && cache->c_lrutail->e_refcnt
		    != 0 && i < 10 ) {
			/* move this in-use entry to the front of the q */
			ee = cache->c_lrutail;
			LRU_DELETE( cache, ee );
			LRU_ADD( cache, ee );
			i++;
		}

		/*
		 * found at least one to delete - try to get back under
		 * the max cache size.
		 */
		while ( cache->c_lrutail != NULL && cache->c_lrutail->e_refcnt
                    == 0 && cache->c_cursize > cache->c_maxsize ) {
			e = cache->c_lrutail;

			/* delete from cache and lru q */
			rc = cache_delete_entry_internal( cache, e );

			entry_free( e );
		}
	}

	/* free cache mutex */
	pthread_mutex_unlock( &cache->c_mutex );
	return( 0 );
}

/*
 * cache_find_entry_dn - find an entry in the cache, given dn
 */

Entry *
cache_find_entry_dn(
    struct cache	*cache,
    char		*dn
)
{
	Entry		e, *ep;

	/* set cache mutex */
	pthread_mutex_lock( &cache->c_mutex );

	e.e_dn = dn;
	if ( (ep = (Entry *) avl_find( cache->c_dntree, &e, cache_entrydn_cmp ))
	    != NULL ) {
		/*
		 * entry is deleted or not fully created yet
		 */
		if ( ep->e_state == ENTRY_STATE_DELETED ||
		    ep->e_state == ENTRY_STATE_CREATING )
		{
			/* free cache mutex */
			pthread_mutex_unlock( &cache->c_mutex );
			return( NULL );
		}
		ep->e_refcnt++;

		/* lru */
		LRU_DELETE( cache, ep );
		LRU_ADD( cache, ep );
	}

	/* free cache mutex */
	pthread_mutex_unlock( &cache->c_mutex );

	return( ep );
}

/*
 * cache_find_entry_id - find an entry in the cache, given id
 */

Entry *
cache_find_entry_id(
    struct cache	*cache,
    ID			id
)
{
	Entry	e;
	Entry	*ep;

	/* set cache mutex */
	pthread_mutex_lock( &cache->c_mutex );

	e.e_id = id;
	if ( (ep = (Entry *) avl_find( cache->c_idtree, &e, cache_entryid_cmp ))
	    != NULL ) {
		/*
		 * entry is deleted or not fully created yet
		 */
		if ( ep->e_state == ENTRY_STATE_DELETED ||
		    ep->e_state == ENTRY_STATE_CREATING )
		{
			/* free cache mutex */
			pthread_mutex_unlock( &cache->c_mutex );
			return( NULL );
		}
		ep->e_refcnt++;

		/* lru */
		LRU_DELETE( cache, ep );
		LRU_ADD( cache, ep );
	}

	/* free cache mutex */
	pthread_mutex_unlock( &cache->c_mutex );

	return( ep );
}

/*
 * cache_delete_entry - delete the entry e from the cache.  the caller
 * should have obtained e (increasing its ref count) via a call to one
 * of the cache_find_* routines.  the caller should *not* call the
 * cache_return_entry() routine prior to calling cache_delete_entry().
 * it performs this function.
 *
 * returns:	0	e was deleted ok
 *		1	e was not in the cache
 *		-1	something bad happened
 */
int
cache_delete_entry(
    struct cache	*cache,
    Entry		*e
)
{
	int	rc;

	/* set cache mutex */
	pthread_mutex_lock( &cache->c_mutex );

	rc = cache_delete_entry_internal( cache, e );

	/* free cache mutex */
	pthread_mutex_unlock( &cache->c_mutex );
	return( rc );
}

static int
cache_delete_entry_internal(
    struct cache	*cache,
    Entry		*e
)
{
	/* dn tree */
	if ( avl_delete( &cache->c_dntree, e, cache_entrydn_cmp ) == NULL ) {
		return( -1 );
	}

	/* id tree */
	if ( avl_delete( &cache->c_idtree, e, cache_entryid_cmp ) == NULL ) {
		return( -1 );
	}

	/* lru */
	LRU_DELETE( cache, e );
	cache->c_cursize--;

	/*
	 * flag entry to be freed later by a call to cache_return_entry()
	 */
	e->e_state = ENTRY_STATE_DELETED;

	return( 0 );
}

#ifdef LDAP_DEBUG

static void
lru_print( struct cache *cache )
{
	Entry	*e;

	fprintf( stderr, "LRU queue (head to tail):\n" );
	for ( e = cache->c_lruhead; e != NULL; e = e->e_lrunext ) {
		fprintf( stderr, "\tdn %20s id %d refcnt %d\n", e->e_dn,
		    e->e_id, e->e_refcnt );
	}
	fprintf( stderr, "LRU queue (tail to head):\n" );
	for ( e = cache->c_lrutail; e != NULL; e = e->e_lruprev ) {
		fprintf( stderr, "\tdn %20s id %d refcnt %d\n", e->e_dn,
		    e->e_id, e->e_refcnt );
	}
}

#endif
