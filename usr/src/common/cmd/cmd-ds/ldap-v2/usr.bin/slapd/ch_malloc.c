/* @(#)ch_malloc.c	1.3
 *
 * ch_malloc.c - malloc routines that test returns from malloc and friends
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "slap.h"
#include "ldaplog.h"

/* Messages */

#define MSG_MALLOCFAIL1  \
    1,21,"Out of memory, malloc() of %d bytes failed\n"
#define MSG_REALLOCFAIL1 \
    1,22,"Out of memory, realloc() of %d bytes failed\n"
#define MSG_CALLOCFAILED \
    1,23,"Out of memory, calloc() of %d bytes failed\n" 


char *
ch_malloc(
    unsigned long	size
)
{
	char	*new;

	if ( (new = (char *) malloc( size )) == NULL ) {

		logError( get_ldap_message( MSG_MALLOCFAIL1, size) );
	}
	return( new );
}

char *
ch_realloc(
    char		*block,
    unsigned long	size
)
{
	char	*new;

	if ( block == NULL ) {
		return( ch_malloc( size ) );
	}

	if ( (new = (char *) realloc( block, size )) == NULL ) {
		logError( get_ldap_message( MSG_REALLOCFAIL1, size));
	}
	return( new );
}

char *
ch_calloc(
    unsigned long	nelem,
    unsigned long	size
)
{
	char	*new;

	if ( (new = (char *) calloc( nelem, size )) == NULL ) {
		logError( get_ldap_message( MSG_CALLOCFAILED, nelem*size));
	}
	return( new );
}
