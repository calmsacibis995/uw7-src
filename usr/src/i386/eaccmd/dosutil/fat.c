/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/fat.c	1.1.1.2"
#ident  "$Header$"

/*
 *	@(#) fat.c 22.1 89/11/14 
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



/*	goodclust(clustno)  --  returns TRUE if the cluster is part of an
 *		allocation chain, ie. the cluster holds part of a DOS file.
 */

goodclust(clustno)
unsigned clustno;
{
	if ((clustno < FIRSTCLUST)          ||
	    ( bigfat && (clustno > 0xfff8)) ||
	    (!bigfat && (clustno > 0x0ff8)))
			return(FALSE);
	return(TRUE);
}



/*	isbigfat()  --  returns TRUE if there are 16 bits per FAT entry;
 *			FALSE if 12 bits per FAT entry.
 */

isbigfat()
{
	int  retcode = FALSE;

	if ( frmp->f_clusters > 4086 )			/* 16 bit FAT */
		retcode = TRUE;

#ifdef DEBUG
	fprintf(stderr,"DEBUG %u clusters on disk; %d bit FAT\n",
			frmp->f_clusters, retcode ? 16 : 12);
#endif
	return(retcode);
}


/*	nextclust(thisclust)  -- interprets the in-memory FAT and returns
 *					the FAT entry for this cluster.
 */

unsigned nextclust(thisclust)
unsigned thisclust;
{
	char *ent;
	register unsigned answer;

	ent = fat;
	if (bigfat){				/* 16 bit FAT */
		ent   += (int) (thisclust * 2.0);
		answer = word(ent);
	}
	else{					/* 12 bit FAT */
		ent   += (int) (thisclust * 1.5);
		answer = word(ent);
		if (thisclust % 2)
			answer = (answer >> 4) & 0xfff;
		else
			answer = answer & 0xfff;
	}

#ifdef DEBUG
	fprintf(stderr,"DEBUG nextclust(%u) = %u;\tFAT %.2x%.2x\n",
			thisclust,answer,*ent & 0xff, *(ent + 1) & 0xff);
#endif
	return(answer);
}
