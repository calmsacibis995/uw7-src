/* @(#)close.c	1.3
 *
 * close.c - close ldbm backend
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "back-ldbm.h"

ldbm_back_close( Backend *be )
{
	logDebug( LDAP_LOG_LDBMBE, 
	    "(ldbm_back_close) ldbm backend syncing\n", 0, 0, 0 );

	ldbm_cache_flush_all( be );

	logDebug( LDAP_LOG_LDBMBE,
	    "(ldbm_back_close) ldbm backend done syncing\n", 0, 0, 0 );
}
