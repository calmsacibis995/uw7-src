/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)pax:hash.c	1.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif
/*
 * hash.c - hash table functions
 *
 * DESCRIPTION
 *
 *	These routines are provided to manipulate the hashed list
 *	of filenames in an achive we are updating.
 *
 * AUTHOR
 *
 *     Tom Jordahl - The Open Software Foundation
 *
 *
 */

/* Headers */
#include "config.h"
#include "pax.h"
#include "charmap.h"
#include "func.h"
#include "pax_msgids.h"




static	Hashentry	*hash_table[HTABLESIZE]; 

/* hash_name - enter a name into the table
 *
 * DESCRIPTION
 *
 *	hash_name places an entry into the hash table.  The name
 *	is hashed in to a 16K table of pointers by adding all the
 *	byte values in the pathname modulo 16K.  This gives us a
 *	moderatly size table without too much memory usage.
 *	
 *	Will update the mtime of an entry which already exists.
 *
 * PARAMETERS
 *
 *	char 	*name 	- Name to be hashed
 *	Stat	*sb	- stat buffer to get the mtime out of
 *
 */


void hash_name(char *name, Stat *sb)

{
    Hashentry	*entry;
    Hashentry	*hptr;
    char  	*p;
    uint	 total=0;


    p = name;
    while (*p != '\0') {
	total += *p; 
	p++;
    }

    total = total % HTABLESIZE;

    if ((entry = (Hashentry *)mem_get(sizeof(Hashentry))) == NULL) {
	fatal(gettxt(NOMEM, "Out of memory"));
    }

    if ((hptr = hash_table[total]) != NULL) {
	while (hptr->next != NULL) {
	    if (!strcmp(name, hptr->name)) {
		hptr->mtime =  sb->sb_mtime;
		free(entry);
		return;
	    }
	    hptr = hptr->next;
	}
	hptr->next = entry;
    } else {
	hash_table[total] = entry;
    }

    entry->name = mem_str(name);
    entry->mtime = sb->sb_mtime;
    entry->next = NULL;

}


/* hash_lookup - lookup the modification time of a file in hash table
 *
 * DESCRIPTION
 *
 *	Check the hash table for the given filename and returns the
 *	modification time stored in the table, -1 otherwise.
 *
 * PARAMETERS
 *
 *	char *name 	- name of file to lookup
 *
 * RETURNS
 *
 *	modification time found in the hash table.
 *	-1 if name isn't found.
 *
 */


time_t hash_lookup(char *name)

{
    char	*p;
    uint	 total=0;
    Hashentry	*hptr;

    p = name;
    while (*p != '\0') {
	total += *p; 
	p++;
    }

    total = total % HTABLESIZE;
   
    if ((hptr = hash_table[total]) == NULL) 
	return((time_t) -1);

    while (hptr != NULL) {
	if (!strcmp(name, hptr->name)) 
	    return(hptr->mtime);	/* found it */
	hptr = hptr->next;
    }

    return((time_t) -1);		/* not found */
}
