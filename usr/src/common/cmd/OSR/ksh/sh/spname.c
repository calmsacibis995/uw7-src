#ident	"@(#)OSRcmds:ksh/sh/spname.c	1.1"
#pragma comment(exestr, "@(#) spname.c 25.1 92/12/11 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1984-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/* Begin SCO_BASE */

/*
 *	spname	Filename spelling correction
 *
 * Based on Rob Pike's orginal version (well before the published
 * version in "The UNIX Programming Environment").
 *
 *  MODIFICATION HISTORY
 *
 *	28 March 1984	sco!blf		M000
 *		- In XENIX 3.0 (System III), "" no longer means "."
 *		  and is, in fact, an illegal filename.  Hence, make
 *		  sure we always open ".", not "".
 *	01 Nov 1990	scol!dipakg	L001
 *		- Allow for long filenames
 *		- Use library routines for reading directories
 *	19 Feb 1991	scol!anthonys	L002
 *		- Removed a bug that caused the spelling checker to
 *		  give up on long filenames.
 */

#include <sys/types.h>
#include <sys/param.h>				/* L001 */
#include <dirent.h>				/* L001 */
#include "../../include/osr.h"

static int SPdist(char *, char *);		/* L001 */
static int SPeq(char *, char *);		/* L001 */

/*
 * char *spname(name)
 *	char name[];
 *
 * returns pointer to correctly spelled name,
 * or 0 if no reasonable name is found;
 * uses a static buffer to store correct name,
 * so copy it if you want to call the routine again.
 */
char *
spname(name)
	register char *name;
{
	register char *p, *q, *new;
	register d, nd;
	DIR *dir;				/* L001 */
	static char newname[PATHSIZE];		/* L001 */
	static char guess[_SCO_NAMELEN+1];	/* L001 */
	static char best[_SCO_NAMELEN+1];	/* L001 */
	struct dirent *nbuf;			/* L001 */

	new = newname;

	for(;;){
		while(*name == '/')
			*new++ = *name++;
		*new = '\0';
		if(*name == '\0')
			return(newname);
		p = guess;
		while(*name!='/' && *name!='\0'){
			if(p != guess + sizeof(guess) - 1)	/* L002 */
				*p++ = *name;
			name++;
		}
		*p = '\0';
		if (*newname == '\0') {			/* M000 begin... */
			if ((dir = opendir(".")) == NULL) /* L001 */
				return ((char *)0);
		}
		else if((dir=opendir(newname)) == NULL)	/* ...end M000 L001 */
			return((char *)0);
		d = 3;
		while( (nbuf=readdir(dir)) != NULL)	/* L001 begin */
			if(nbuf->d_ino){
				nd=SPdist(nbuf->d_name, guess);
				if(nd<=d && nd!=3) {	/* <= to avoid "." */
					p = best;
					q = nbuf->d_name;
					do; while(*p++ = *q++);
					d = nd;
					if(d == 0)
						break;
				}
			}
		(void) closedir(dir);			/* L001 end */
		if(d == 3)
			return(0);
		p = best;
		do; while(*new++ = *p++);
		--new;
	}
}
/*
 * very rough spelling metric
 * 0 if the strings are identical
 * 1 if two chars are interchanged
 * 2 if one char wrong, added or deleted
 * 3 otherwise
 */
static int
SPdist(s, t)
	register char *s, *t;
{
	while(*s++ == *t)
		if(*t++ == '\0')
			return(0);
	if(*--s){
		if(*t){
			if(s[1] && t[1] && *s==t[1] && *t==s[1] && SPeq(s+2,t+2))
				return(1);
			if(SPeq(s+1, t+1))
				return(2);
		}
		if(SPeq(s+1, t))
			return(2);
	}
	if(*t && SPeq(s, t+1))
		return(2);
	return(3);
}

static int
SPeq(s, t)
	register char *s, *t;
{
	while(*s++ == *t)
		if(*t++ == '\0')
			return(1);
	return(0);
}

/* End SCO_BASE */
