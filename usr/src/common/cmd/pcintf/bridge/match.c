#ident	"@(#)pcintf:bridge/match.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)match.c	6.2	LCC);	/* Modified: 21:03:12 6/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <memory.h>
#include <string.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "const.h"
#include "dossvr.h"

#define	matchdbg(X)

LOCAL void	getname	PROTO((char *, char *, char *, int));
LOCAL int	amatch	PROTO((char *, char *, int));

/*			Utility Routines				*/


/*
 *	wildcard	-	scans a search pattern from the end and 
 *				true if the filename contains a wildcard.
 */

int
wildcard(ptr, len)
	register CONST char	*ptr;
	register int		len;
{
	for (ptr += --len; len >= 0 && *ptr != '/'; len--, ptr--)
		if (*ptr == '?' || *ptr == '*')
			return TRUE;
	return FALSE;
}



/*
 *	getpath		-	extracts the directory pathname and filename
 *				from a search pattern and stores names in
 *				the variables path and name.
 */

void
getpath(ptr, path, name)
register char *ptr;
char *path;
char *name;
{
    register int 
	i;

register int len = strlen(ptr);
/* Scan backwards for a backslash: copy search string up to but not including
 * backslash into 'path'.  Strcpy filename after backslash into name.
 * If search pattern implies current directory store "." in path.
 */
    for (i = len-2; (ptr[i] != '/') && (i > 0); i--)
	/*EMPTYLOOP*/
	;

    if (i > 0) {
	memcpy(path, ptr, i);	/* Copy pathname of directory into path */
	path[i] = '\0';		/* null terminate this sucker */
	strcpy(name, &ptr[i + 1]);/* Copy filename into name */
    } else {
	/* no slashes, or the only slash is the first char */
	if (ptr[0] == '/') {
	    strcpy(path, "/");      /* directory is ROOT */
	    strcpy(name, &ptr[1]);  /* filename */
	} else {
	    strcpy(path, CurDir);      /* Load current directory       */
	    strcpy(name, ptr);      /* Load filename into name      */
	}
    }
}


void
getname(pattern, string1, string2,mode)
char	*pattern,
	*string1,
	*string2;
int	mode;
{
    register char
	*ptr;

    register int
	length;


	matchdbg(("getname:pat %s,mode 0x%x\n",pattern,mode));
/* 
 *  Case 1: The pattern has an extension.
 */
    if (ptr = strrchr(pattern, '.')) {
	length = ptr - pattern;
	strncpy(string1, pattern, length);

    /* Case A: The filename is greater then DOS allows */
	if ((length > DOS_NAME) && (mode == MAPPED))
	    string1[DOS_NAME] = 0;

    /* Case B: The filename acceptable */
	else
	    string1[length] = 0;

    /* Case C: The extension is too long */
	strcpy(string2, ++ptr);
	if (((length = strlen(ptr)) > DOS_EXT) && (mode == MAPPED))
	    string2[DOS_EXT] = 0;

    /* Case D: The extension is acceptable */
	else
	    string2[length] = 0;
    }
/*
 * If the pattern has no extension just return name.
 */
    else {
	strcpy(string1, pattern);
	if (((length = strlen(pattern)) > DOS_NAME) && (mode == MAPPED))
	    string1[DOS_NAME] = 0;
	else
	    string1[length] = 0;
	strcpy(string2, "");
    }
	matchdbg(("getname:pattern %s,string1 %s,string2 %s,mode 0x%x\n",
		pattern,string1,string2,mode));
}


int
amatch(filename, pattern, mode)
char
	*filename,
	*pattern;
int
	mode;
{
register char
	*scanptr;
register int
	fn,
	pn;
int
	allWild;

	matchdbg(("amatch:file %s, pat %s, mode 0x%x\n",filename,
		pattern,mode));
    for (;;) {
	if (fn = *filename)
		filename++;
	switch (pn = *pattern++) {
	    case 0:
		return(fn == 0);

	    case '?':
		for (allWild = TRUE, scanptr = pattern; *scanptr; scanptr++)
		    if (*scanptr != '*' && *scanptr != '?') {
			allWild = FALSE;
			break;
		    }

		if (allWild && !fn)
		    return(TRUE);

		continue;

	    case '*':
		while (*pattern == '*')
		    pattern++;
		    
		if (!*pattern)
		    return(TRUE);
		
		for (filename--; *filename; filename++)
		    if (amatch(filename, pattern, mode))
			return(TRUE);
		return(FALSE);

	    default:
		if (fn == pn) continue;
		if (mode == MAPPED || mode == IGNORECASE) {
		    if (mode == IGNORECASE && ((fn|0x20) == pn)) continue;
		}
		return(FALSE);
	}
    }
}


int
match(filename, pattern, mode)
register char *filename;
register char *pattern;
int	mode;
{
    /* Note for the following dimensions that we might be doing an */
    /* extended-mode search, which allows arbitrary UNIX names. */
    static char
	file_name[MAX_FN_COMP],	/* Contains filename from dir filename */
	file_ext[MAX_FN_COMP],	/* Contains extension from dir filename */
	ptrn_name[MAX_FN_COMP],	/* Contains MS-DOS name portion of pattern */
	ptrn_ext[MAX_FN_COMP];	/* Contains MS-DOS ext portion of pattern */

	matchdbg(("match:file %s,pat %s,mode 0x%x\n",filename,
		pattern,mode));
/* MS-DOS expects "." and ".." returned on wildcard searches */
    if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0))
	return(strcmp(pattern, "*.*") == 0 || strcmp(pattern, "????????.???") == 0);

/* Apply pattern matching on file name and extension components individually */
    getname(filename, file_name, file_ext, mode);
    getname(pattern, ptrn_name, ptrn_ext, mode);
	matchdbg(
		("match:after getname file %s,fileext %s,pat %s,patext %s\n",
		file_name,file_ext,ptrn_name,ptrn_ext));

    return ((amatch(file_name, ptrn_name, mode)) && (amatch(file_ext, ptrn_ext, mode)));
}

#ifdef NOTNEEDED
/*
** Matchf: Returns 1 if pattern is equivalent to filename passed,
**		0 otherwise.
*/
int
matchf(fp, pp)
	register char *fp;	/* ptr to file name */
	register char *pp;      /* ptr to pattern */
{
	while (*pp) {
		if (*pp == QUESTION) {
			if (*fp != DOT && *fp != NULL)
				fp++;
		}
		else { /* regular character */
			if (*pp == DOT)
				if (*fp == NULL) {
					pp++;
					continue;
				}
			if (*fp != *pp)
				return(0);
			fp++;
		}
		pp++;
	}
	if (*fp != NULL) return(0);
	else	return(1);
}

/*
** dosrecomp: Compiles DOS regular expression by resolving *'s.
*/
void
dosrecomp(re, sp)
	register char *re;	/* regular expression */
	register char *sp;	/* location to hold compiled string */
{
	register int plen = 0;	/* length of prefix */
	register int slen = 0;	/* length of suffix */
	int  nodot = TRUE;

	/* handle prefix */
	while (*re && plen <= DOS_NAME)
		if (*re == STAR) { /* pad rest of word with ?'s */
			while (*++re)
				if (*re == DOT) {
					re++;
					nodot = FALSE;
					break;
				}
			strncpy(sp, WILDPREFIX, DOS_NAME - plen);
			sp += DOS_NAME - plen;
			break;
		}
		else if (*re == DOT) {
			re++;
			nodot = FALSE;
			break;
		}
		else {	/* regular character */
			*sp++ = *re++;
			plen++;
		}
	/* handle suffix */
	if (nodot) {
		*sp = NULL;
		return;
	}
	else
		*sp++ = DOT;
	if (plen > DOS_NAME)
		while (*re != DOT && *re != NULL)
			re++;		/* skip rest of prefix */
	while (*re && slen <= DOS_EXT) {
		if (*re != STAR) {
			*sp++ = *re++;
			slen++;
		}
		else {	/* pad rest of word with ?'s */
			while (slen++ < DOS_EXT)
				*sp++ = QUESTION;
			break;
		}
	}
	*sp = NULL;
	return;
}
#endif /* NOTNEEDED */
