#ident	"@(#)pcintf:bridge/unmapname.c	1.1.1.4"
#include        "sccs.h"
SCCSID(@(#)unmapname.c	6.5	LCC);	/* Modified: 13:20:42 1/28/92 */

/*****************************************************************************

	Copyright (c) 1984, 1991 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <fcntl.h>
#include <string.h>

#include "pci_types.h"

#ifdef	NO_MAP_UPPERCASE
#include <stat.h>
#endif	/* NO_MAP_UPPERCASE */

#include "dossvr.h"
#include "xdir.h"
#include "log.h"

#define umDbg(x)

LOCAL struct dirent	*find_mapped	PROTO((char *, char *));
LOCAL ino_t		str2INum	PROTO((char *));
LOCAL char		*xstrtok	PROTO((char **, char *));
#ifdef	NO_MAP_UPPERCASE
LOCAL char *	find_upper();
#endif	/* NO_MAP_UPPERCASE */

#ifdef	NO_MAP_UPPERCASE
extern int unix_uppercase();	/* (translate.c) */
#endif	/* NO_MAP_UPPERCASE */

/*
   introChars: Used in introducing the altered part of a mapped file name
*/
extern char     *introChars;
extern char     *iNumChars;
#ifdef	NO_MAP_UPPERCASE
extern int	upper_case_fn_ok;
#endif	/* NO_MAP_UPPERCASE */

int unmap_fail = 0;		/* Fail unmapname if file doesn't exist */


#define INUM_RADIX      (36L)           /* Default # of i-number chars */

#define has_special(s)  (strpbrk(s, introChars))
#define isupper(c)      (((c) >= 'A') && ((c) <= 'Z'))
#define islower(c)      (((c) >= 'a') && ((c) <= 'z'))
#define isdigit(c)      (((c) >= '0') && ((c) <= '9'))
#define nil(x)          ((x) == 0)

/*
 * unmapfilename() -	Parses an input string containing pathnames returned 
 * 			by the Bridge into real UNIX pathnames.  Note the
 *			Bridge creates special file and directory names to 
 *			insure uniqueness on MS-DOS which alllows a file
 *			and extension eight and three characters respectively.
 *			If the string is OK it returns TRUE otherwise FALSE.
 */

int
unmapfilename(pathname,name)
char *pathname;		/* ptr to current directory string */
char *name;		/* Points to pathname/file to be mapped */
{
    register char *comp;    /* ptr to component of pathname */
    register char *newcomp; /* ptr to newly validated component */
    int ok = TRUE;          /* TRUE if mapping valid; FALSE if dup */
    int rel = FALSE;        /* TRUE if pathname is relative */

    char *parseptr;         /* ptr to parse buffer */
    char tmpname[MAX_FN_TOTAL]; /* Holds name temporarily */
    char dir[MAX_FN_TOTAL];     /* Directory to search in */
    char parsebuf[MAX_FN_TOTAL];/* Temp buffer for string parsing */
    struct dirent *dirp;    /* ptr to matched dir entry */
#ifdef	NO_MAP_UPPERCASE
    char compbuffer[14];	/* Temp buffer */
#endif	/* NO_MAP_UPPERCASE */

#ifdef	NO_MAP_UPPERCASE
    if (!upper_case_fn_ok && !has_special(name))
#else	/* not NO_MAP_UPPERCASE */
    if (!has_special(name))
#endif	/* not NO_MAP_UPPERCASE */
	return(TRUE);

    strcpy(tmpname,name);

    if (*name != '/')
    {
	rel++;
	strcpy(parsebuf, "/");
	strcat(parsebuf, name);
	strcpy(dir, pathname);
	strcat(dir, "/");
	*name=0;
    } else
    {
	strcpy(parsebuf,name);
	strcpy(name,"/");	/* xstrtok will drop first / */
	dir[0] = 0;		/* for mapdebugging */
    }

    parseptr = parsebuf;

    /* the following loop seems odd because of the semantics of
    *  xstrtok; it parses off tokens,returning a ptr to the
    *  start of the first token from its current string ptr,
    *  after zapping a null into the position where the first of
    *  its token separator chars was; if called with a null string
    *  ptr,it continues from where it last left off;
    */

    while(comp=xstrtok(&parseptr,"/"))
    {
#ifdef	NO_MAP_UPPERCASE
	newcomp = 0;
#else	/* not NO_MAP_UPPERCASE */
	newcomp = comp;
#endif	/* not NO_MAP_UPPERCASE */

	if (has_special(comp))
	{
	    if (dirp = find_mapped(rel?dir:name,comp))
	    {
		newcomp = dirp->d_name;
	    } else if (unmap_fail)
		return(FALSE);
	}
#ifdef	NO_MAP_UPPERCASE
	if (newcomp == 0) {
		if (upper_case_fn_ok) {
			if ((newcomp = find_upper(rel?dir:name,comp,compbuffer))
				== NULL)
			{
				newcomp = comp;
			}
		} else {
			newcomp = comp;
		}
	}
#endif	/* NO_MAP_UPPERCASE */
	strcat(name, newcomp);
	strcat(name, "/");
	if (rel) {
	    strcat(dir, newcomp);
	    strcat(dir, "/");
	}
    }
    /* Strip off trailing "/" */
    if (*name)
	name[strlen(name)-1] = '\0';

    return(TRUE);
}

#ifdef	NO_MAP_UPPERCASE
/* ---------------------------------------------------------------------------
   find_upper -
	Find an uppercase version of the file.
*/
LOCAL
char *
find_upper(dir,file,tempbuf)
char *dir;	/* Directory in which to search. */
char *file;	/* Filename to match. */
char *tempbuf;	/* This can be used to hold a dos sized filename (13 chars). */
{
#ifdef	MIXED_CASE_OK
	DIR *dirptr;                    /* handle on the dir being searched */
	struct dirent *dirent;          /* handle on cur direntry */
#endif	/* MIXED_CASE_OK */
	char fullname[MAX_FN_COMP + 1];
	struct stat statbuf;
	register char *cp, *outp;

	umDbg(("find_upper:%s %s\n", dir, file));

#ifdef	MIXED_CASE_OK
	if (upper_case_fn_ok == 2) {
		/* Mixed case filenames are ok.  Must search the
		** directory for a match.
		*/

		if ((dirptr = opendir(dir)) == NULL) {
			return(NULL);
		}

		/* Step thru the directory seeing if the filename matches. */
		while ((dirent = readdir(dirptr)))
		{
			strcpy(fullname,dirent->d_name);
			mapfilename(dir,fullname);
			umDbg(("%s match %s\n", dirent->d_name, fullname));
	
			if (match(fullname,file,IGNORECASE))
			{
				umDbg(("MATCH\n"));
				/* this is the one we really want */
				break;
			}
		}
	
		strcpy(tempbuf, dirent->d_name);
		closedir(dirptr);
		return(tempbuf);
	}
#endif	/* MIXED_CASE_OK */

	/* Uppercase the filename. */
	(void) unix_uppercase(file, tempbuf, 13);

	strcpy(fullname, dir);
	outp = fullname + strlen(fullname);
	if (*(outp -1) != '/')
		*outp++ = '/';
	strcpy(outp, tempbuf);
	umDbg(("try:%s\n", fullname));
	if (stat(fullname, &statbuf) < 0) {
		umDbg(("stat failed\n", errno));
		return(NULL);
	}
	return(tempbuf);
}
#endif	/* NO_MAP_UPPERCASE */

/* ---------------------------------------------------------------------------
   find_mapped search a directory for a mapped name
*/
LOCAL
struct dirent *
find_mapped(dir,file)
char *dir;	/* directory in which to search */
char *file;	/* mapped name to be found */
{

    DIR *dirptr;                    /* handle on the dir being searched */
    struct dirent *dirent;          /* handle on cur direntry */
    ino_t iNum;                     /* inode number of file wanted */
    register char *mapBegin;        /* First char of mapped part of name */
    char mappedname[MAX_FN_COMP + 1];

    if (!(mapBegin = strpbrk(file,introChars)))
    {
	/* This was supposed to be a mapped name,but just in case
	*  someone goofed
	*/

	return(NULL);
    }

    if ((dirptr = opendir(dir)) == NULL)
    {
	return(NULL);
    }
    iNum = str2INum(mapBegin);
    /* try to find the given entry */
    while ((dirent = readdir(dirptr)))
    {
	if (match(file, dirent->d_name, UNMAPPED))
	{
	    /* this one wasn't really a mapped name */
	    break;
	}
	if (dirent->d_ino == iNum)
	{
	    /* got a match,make sure it's the real one */
	    strcpy(mappedname,dirent->d_name);
	    mapfilename(dir,mappedname);

	    if (match(mappedname,file,IGNORECASE))
	    {
		/* this is the one we really want */
		break;
	    }
	}
    }

    closedir(dirptr);
    return(dirent);
}

/* ---------------------------------------------------------------------------
   str2INum: Convert a string to an inode number
*/
LOCAL
ino_t
str2INum(iNumStr)
char *iNumStr;               /* Inode number string to translate */
{
    int             iNumDigit;          /* Numeric value of iNum char */
    unsigned long   iNum = 0;           /* The computed inode number */
    unsigned long   introVal;           /* Value of the introducer char */
    char            *introPos;          /* Points into introChars */

    /* Compute contribution to inode number from introducer character */
    if (nil(introPos = strpbrk(introChars, iNumStr)))
    {
	return (ino_t) 0;
    }
    introVal = (introPos == introChars) ? 0 : 0x80000000;

    /* Get iNum chars' contributions */
    while (*++iNumStr != '\0' && *iNumStr != '.')
    {
	if (isupper(*iNumStr))
	    iNumDigit = *iNumStr - 'A';
	else if (islower(*iNumStr))
	    iNumDigit = *iNumStr - 'a';
	else if (isdigit(*iNumStr))
	    iNumDigit = *iNumStr - '0' + 26;
	else
	{
	    return (ino_t) 0;
	}
	iNum *= INUM_RADIX;
	iNum += iNumDigit;
    }

    return (ino_t) (introVal + iNum);
}


/*----------------------------------------------------------------------------
 * xstrtok: modified version of strtok from SYSV; differs in that instead of
 * using a static to remember where it left off, one passes it the address of
 * a variable which contains a pointer to the string to be scanned; this
 * variable is updated to point to the next place to start from 
 * uses strpbrk and strspn to break string into tokens on
 * sequentially subsequent calls.  returns NULL when no
 * non-separator characters remain.
 */
LOCAL
char *
xstrtok(stradr, sepset)
char	**stradr, *sepset;
{
    register char *p = *stradr;
    register char   *q, *r;


    if(p == 0)                  /* return if no tokens remaining */
	return(NULL);

    q = p + strspn(p, sepset);  /* skip leading separators */

    if(*q == '\0')              /* return if no tokens remaining */
	return(NULL);

    if((r = strpbrk(q, sepset)) == NULL)    /* move past token */
    {
	*stradr = 0;            /* indicate this is last token */
    }
    else
    {
	*r = '\0';
	*stradr = ++r;
    }
    return(q);
}
