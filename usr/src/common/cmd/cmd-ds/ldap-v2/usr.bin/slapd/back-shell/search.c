/* search.c - shell backend search function */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "shell.h"

extern Entry	*str2entry();

void
shell_back_search(
    Backend	*be,
    Connection	*conn,
    Operation	*op,
    char	*base,
    int		scope,
    int		deref,
    int		size,
    int		time,
    Filter	*filter,
    char	*filterstr,
    char	**attrs,
    int		attrsonly
)
{
	struct shellinfo	*si = (struct shellinfo *) be->be_private;
	int			i, rc, bsize, len;
	int			err;
	char			*matched, *info;
	FILE			*rfp, *wfp;

	if ( si->si_search == NULL ) {
		send_ldap_result( conn, op, LDAP_UNWILLING_TO_PERFORM, NULL,
		    "search not implemented" );
		return;
	}

	if ( (op->o_private = forkandexec( si->si_search, &rfp, &wfp ))
	    == -1 ) {
		send_ldap_result( conn, op, LDAP_OPERATIONS_ERROR, NULL,
		    "could not fork/exec" );
		return;
	}

	/* write out the request to the search process */
	fprintf( wfp, "SEARCH\n" );
	fprintf( wfp, "msgid: %d\n", op->o_msgid );
	print_suffixes( wfp, be );
	fprintf( wfp, "base: %s\n", base );
	fprintf( wfp, "scope: %d\n", scope );
	fprintf( wfp, "deref: %d\n", deref );
	fprintf( wfp, "sizelimit: %d\n", size );
	fprintf( wfp, "timelimit: %d\n", time );
	fprintf( wfp, "filter: %s\n", filterstr );
	fprintf( wfp, "attrsonly: %d\n", attrsonly ? 1 : 0 );
	fprintf( wfp, "attrs:%s", attrs == NULL ? " all" : "" );
	for ( i = 0; attrs != NULL && attrs[i] != NULL; i++ ) {
		fprintf( wfp, " %s", attrs[i] );
	}
	fprintf( wfp, "\n" );
	fclose( wfp );

	/* read in the results and send them along */
	read_and_send_results( be, conn, op, rfp, attrs, attrsonly );

	fclose( rfp );
}
