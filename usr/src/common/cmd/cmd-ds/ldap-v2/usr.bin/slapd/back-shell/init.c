/* init.c - initialize shell backend */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "slap.h"
#include "shell.h"

/* Messages */
#define MSGNOMEM \
    1,107,"Could not allocate memory to shell backend database structure\n"


shell_back_init(
    Backend	*be
)
{
	struct shellinfo	*si;

	si = (struct shellinfo *) ch_calloc( 1, sizeof(struct shellinfo) );
	if ( si == NULL ) {
		logError( get_ldap_message( MSGNOMEM ));
		exit ( LDAP_NO_MEMORY );
	}

	be->be_private = si;
}
