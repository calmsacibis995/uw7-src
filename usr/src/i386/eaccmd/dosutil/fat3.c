/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/fat3.c	1.1.1.2"
#ident  "$Header$"

/*
 *	@(#) fat3.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
#include	<stdio.h>
#include	"dosutil.h"

extern char *fat;		/* buffer for FAT; malloc()ed in main() */
extern int bigfat;		/* TRUE if 16 bit FAT; FALSE if 12 bit FAT */



/*	clustalloc()  --  find a free cluster and allocate it.  If successful,
 *		return the cluster number, else return NOTFOUND.
 *		where : cluster number from which to start searching
 *
 *		NOTE: Only the in-memory FAT is updated !!
 */

unsigned clustalloc(where)
unsigned where;
{
	char *ent, *lim;
	unsigned clustno;

	clustno = NOTFOUND;
	ent     = fat;

	if (bigfat){					/* 16 bit FAT */

		lim = fat + (long) (frmp->f_clusters * 2L);

		for (ent += (long) (where * 2L); ent <= lim; ent += 2)
			if (word(ent) == 0){
				clustno = (ent - fat) / 2;
				break;
			}
	}
	else{						/* 12 bit FAT */

		lim = fat + (long) (frmp->f_clusters * 3 / 2L);

		if (where % 2){
			where = min(where-1, FIRSTCLUST);
		}
		for (ent += (long) (where * 3 / 2L); ent < lim; ent += 3){
			if ( !(longword(ent) & 0xfffL) ){
				clustno = (ent - fat) * 2 / 3;
				break;
			}
			if ( !(longword(ent) & 0xfff000L) ){
				clustno = ((ent - fat) * 2 / 3) + 1;
				break;
			}
		}
		if (( !(frmp->f_clusters % 2) ) && 
		    ( !(longword(ent) & 0xfffL) ))
				clustno = (ent - fat) * 2 / 3;
	}
	if (clustno != NOTFOUND){

#ifdef DEBUG
		fprintf(stderr,
			"DEBUG allocating cluster %u;\tFAT %.2x%.2x%.2x\n",
			clustno,*ent & 0xff, *(ent+1) & 0xff, *(ent+2) & 0xff);
#endif
		marklast(clustno);
	}
	return(clustno);
}


/*
 *	marklast(clustno)  --  marks a cluster as the last cluster of a file.
 */

marklast(clustno)
unsigned clustno;
{
	chain(clustno,bigfat ? 0xffff : 0xfff);
}
