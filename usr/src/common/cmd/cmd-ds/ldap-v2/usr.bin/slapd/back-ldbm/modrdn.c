/* modrdn.c - ldbm backend modrdn routine */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"

extern Entry	*dn2entry();
extern char	*dn_parent();

int
ldbm_back_modrdn(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    char	*dn,
    char	*newrdn,
    int		deleteoldrdn
)
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	char		*matched;
	char		*pdn, *newdn, *p;
	char		sep[2];
	Entry		*e, *e2;

	matched = NULL;
	if ( (e = dn2entry( be, dn, &matched )) == NULL ) {
		send_ldap_result( conn, op, LDAP_NO_SUCH_OBJECT, matched, "" );
		if ( matched != NULL ) {
			free( matched );
		}
		return( -1 );
	}

	if ( (pdn = dn_parent( be, dn )) != NULL ) {
		/* parent + rdn + separator(s) + null */
		newdn = (char *) ch_malloc( strlen( pdn ) + strlen( newrdn )
		    + 3 );
		if ( newdn == NULL ) {
			send_ldap_result(conn,op,LDAP_NO_MEMORY,matched,"");
			if ( matched != NULL ) { free( matched ); }
			return -1; 
		}
		if ( dn_type( dn ) == DN_X500 ) {
			strcpy( newdn, newrdn );
			strcat( newdn, ", " );
			strcat( newdn, pdn );
		} else {
			strcpy( newdn, newrdn );
			p = strchr( newrdn, '\0' );
			p--;
			if ( *p != '.' && *p != '@' ) {
				if ( (p = strpbrk( dn, ".@" )) != NULL ) {
					sep[0] = *p;
					sep[1] = '\0';
					strcat( newdn, sep );
				}
			}
			strcat( newdn, pdn );
		}
	} else {
		newdn = strdup( newrdn );
	}
	(void) dn_normalize( newdn );

	matched = NULL;
	if ( (e2 = dn2entry( be, newdn, &matched )) != NULL ) {
		free( newdn );
		free( pdn );
		send_ldap_result( conn, op, LDAP_ALREADY_EXISTS, NULL, NULL );
		cache_return_entry( &li->li_cache, e2 );
		cache_return_entry( &li->li_cache, e );
		return( -1 );
	}
	if ( matched != NULL ) {
		free( matched );
	}

	/* check for abandon */
	pthread_mutex_lock( &op->o_abandonmutex );
	if ( op->o_abandon ) {
		pthread_mutex_unlock( &op->o_abandonmutex );
		free( newdn );
		free( pdn );
		cache_return_entry( &li->li_cache, e2 );
		cache_return_entry( &li->li_cache, e );
		return( -1 );
	}
	pthread_mutex_unlock( &op->o_abandonmutex );

	/* add new one */
	if ( dn2id_add( be, newdn, e->e_id ) != 0 ) {
		free( newdn );
		free( pdn );
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, NULL, NULL );
		cache_return_entry( &li->li_cache, e );
		return( -1 );
	}

	/* delete old one */
	if ( dn2id_delete( be, dn ) != 0 ) {
		free( newdn );
		free( pdn );
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, NULL, NULL );
		cache_return_entry( &li->li_cache, e );
		return( -1 );
	}

	(void) cache_delete_entry( &li->li_cache, e );
	free( e->e_dn );
	e->e_dn = newdn;

	/* XXX
	 * At some point here we need to update the attribute values in
	 * the entry itself that were effected by this RDN change
	 * (respecting the value of the deleteoldrdn parameter).
	 *
	 * Since the code to do this has not yet been written, treat this
	 * omission as a (documented) bug.
	 */

	/* id2entry index */
	if ( id2entry_add( be, e ) != 0 ) {
		entry_free( e );

		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, "", "" );
		return( -1 );
	}
	free( pdn );
	cache_return_entry( &li->li_cache, e );
	send_ldap_result( conn, op, LDAP_SUCCESS, NULL, NULL );

	return( 0 );
}
