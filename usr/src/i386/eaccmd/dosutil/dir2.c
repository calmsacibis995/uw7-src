/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/dir2.c	1.1.1.3"
#ident  "$Header$"
/*	@(#) dir2.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*	Modification History
 *	M000	sco!rr	4 Apr 1988
 *		- toggle boolean _warn so warning message is not issued when
 *		  makename() is called from forment() (see dir.c M001).
 */

#include	<stdio.h>
#include	<time.h>
#include	"dosutil.h"

extern int Bps;

/*	dirfill()  --  make an entry in a DOS directory, adding a new
 *		cluster to the directory if necessary.  Returns FALSE
 *		if the entry cannot be made.
 *		dirclust : starting cluster of the parent directory
 *		entry    : the directory entry
 */

dirfill(dirclust,entry)
unsigned dirclust;
char *entry;
{
	char *bufend, *j;
	unsigned i, previous = 0;

	if (dirclust == ROOTDIR){
		for (i = 0; i < frmp->f_dirsect; i++){
			readsect(segp->s_dir + i,buffer);
			for (j = buffer; j < buffer + Bps; j += DIRBYTES){
#ifdef DEBUG
/*				fprintf(stderr,"DEBUG dirfill %.11s\n",j);
 */
#endif
				if ((*j == DIREND) || ((*j & 0xff) == WASUSED)){
					movchar(j,entry,DIRBYTES);
					writesect(segp->s_dir + i,buffer);
					return(TRUE);
				}
			}
		}
	}
	else{
		bufend   = buffer + (frmp->f_sectclust * Bps);
		while (goodclust(dirclust)){
			readclust(dirclust,buffer);
			for (j = buffer; j < bufend; j += DIRBYTES){
#ifdef DEBUG
/*				fprintf(stderr,"DEBUG dirfill %.11s\n",j);
 */
#endif
				if ((*j == DIREND) || ((*j & 0xff) == WASUSED)){
					movchar(j,entry,DIRBYTES);
					writeclust(dirclust,buffer);
					return(TRUE);
				}
			}
			previous = dirclust;
			dirclust = nextclust(dirclust);
		}
		if (previous == 0)
			fatal("ERROR DOS disk internals inconsistent !!",1);

		if ((i = clustalloc(FIRSTCLUST)) != NOTFOUND){
			zero(buffer,frmp->f_sectclust * Bps);
			movchar(buffer,entry,DIRBYTES);
			writeclust(i,buffer);
			chain(previous,i);
			return(TRUE);
		}
	}
	return(FALSE);
}


/*	forment()  --  format the buffer buff as a directory entry as given,
 *		returning a pointer to buff.
 *
 *	NOTES:  a) "." and ".." are not accepted as names.
 *		b) The extern variable *buffer is not touched.
 */

char *forment(buff,name,attrib,xtime,clustno,size)
char buff[], *name, attrib;
long xtime, size;
unsigned clustno;
{
	unsigned code;
	struct tm *localtime(), *ptr;
	extern int _warn;	/* M000 */

	zero(buff,DIRBYTES);
	_warn=0;	/* M000 */
	makename(name,buff);
	_warn=1;	/* M000 */
	buff[ATTRIB] = attrib;

	ptr   = localtime(&xtime);
	code  = (ptr->tm_sec   /  2) & 0x001f;
	code |= (ptr->tm_min  <<  5) & 0x07e0;
	code |= (ptr->tm_hour << 11) & 0xf800;
	inttochar(&buff[TIME],code);

	code  =  ptr->tm_mday              & 0x001f;
	code |= ((ptr->tm_mon  +  1) << 5) & 0x01e0;
	code |= ((ptr->tm_year - 80) << 9) & 0xfe00;
	inttochar(&buff[DATE],code);

	inttochar(&buff[CLUST],clustno);
	longtochar(&buff[SIZE],size);

	return(buff);
}
