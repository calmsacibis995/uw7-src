/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/fat4.c	1.1.1.2"
#ident  "$Header$"

/*
 *	@(#) fat4.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*	assess()  --  determine the number of free clusters available.  In
 *			the interest of speed, nextclust() is not used.
 */


#include	<stdio.h>
#include	"dosutil.h"

extern char *fat;		/* buffer for FAT; malloc()ed in main() */
extern int bigfat;		/* TRUE if 16 bit FAT; FALSE if 12 bit FAT */


unsigned assess()
{
	char *ent, *lim;
	unsigned freeclust;

	ent       = fat;
	freeclust = 0;

	if (bigfat){					/* 16 bit FAT */

		lim = fat + (long) (frmp->f_clusters * 2L);
		for (ent += FIRSTCLUST * 2; ent <= lim; ent += 2)
			if (word(ent) == 0)
				freeclust++;
	}
	else{						/* 12 bit FAT */

		lim = fat + (long) (frmp->f_clusters * 3 / 2L);

		for (ent += FIRSTCLUST * 3 / 2; ent < lim; ent += 3){
			if ( !(longword(ent) & 0xfffL) )
				freeclust++;
			if ( !(longword(ent) & 0xfff000L) )
				freeclust++;
		}
		if (( !(frmp->f_clusters % 2) ) && 
		    ( !(longword(ent) & 0xfffL) ))
				freeclust++;
	}
	return(freeclust);
}
