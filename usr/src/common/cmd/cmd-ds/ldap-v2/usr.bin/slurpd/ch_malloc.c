/* @(#)ch_malloc.c	1.4
 *
 * Copyright (c) 1996 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 * ch_malloc.c - malloc() and friends, with check for NULL return.
 */

/*
 * Revision history:
 *
 * 5th March 1997       tonylo
 *	i18n
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../slapd/slap.h"
#include "ldaplog.h"

/* Messages */
#define MSG_MEMALLOC1   \
    1,141,"Out of memory, malloc() of %d bytes failed\n"
#define MSG_MEMREALLOC1 \
    1,142,"Out of memory, realloc() of %d bytes failed\n"
#define MSG_MEMALLOC2   \
    1,143,"Out of memory, calloc() of %d bytes failed\n"

/*
 * Just like malloc, except we check the returned value and exit
 * if anything goes wrong.
 */
char *
ch_malloc(
    unsigned long	size
)
{
	char	*new;
	if ( (new = (char *) malloc( size )) == NULL ) {
		logError(get_ldap_message(MSG_MEMALLOC1,size));
	}

	return( new );
}


/*
 * Just like realloc, except we check the returned value and exit
 * if anything goes wrong.
 */
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
		logError(get_ldap_message(MSG_MEMREALLOC1,size));
	}

	return( new );
}




/*
 * Just like calloc, except we check the returned value and exit
 * if anything goes wrong.
 */
char *
ch_calloc(
    unsigned long	nelem,
    unsigned long	size
)
{
	char	*new;

	if ( (new = (char *) calloc( nelem, size )) == NULL ) {
		logError(get_ldap_message(MSG_MEMALLOC2,nelem*size));
	}

	return( new );
}


/*
 * Just like free, except we check to see if p is null.
 */
void
ch_free(
    char *p
)
{
    if ( p != NULL ) {
	free( p );
    }
    return;
}
	
