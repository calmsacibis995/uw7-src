/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:fclassdb.c	1.34.1.1"

#include <libgen.h>
#include <X11/Intrinsic.h>
#include "Dtm.h"
#include "extern.h"


void
Dm__SetFileClass(op)
DmObjectPtr op;
{
	register DmFnameKeyPtr fnkp = DESKTOP_FNKP(Desktop);
	register char *p;
	char *path;
	char *name;
	char *rpath = NULL;
	char *real_path;
	char *real_name;
	char *class;
	int found = 0;

	class = DmGetObjProperty(op, OBJCLASS, NULL);
	path = strdup(dirname(DmMakePath(op->container->path, op->name)));
	name = basename(op->name);

	if (class) {
		for (; fnkp; fnkp = fnkp->next) {
			/*
			 * Skip new entries that haven't been applied yet.
			 * Note that deleted entries that haven't been applied are
			 * still being used.
			 */
			if (fnkp->attrs & (DtAttrs)(DM_B_NEW | DM_B_CLASSFILE |
					DM_B_OVERRIDDEN	| DM_B_INACTIVE_CLASS))
				continue;

			if (!strcmp(fnkp->name, class)) {
				/* found a match! */
				op->fcp = fnkp->fcp;
				found++;
			}
		}
	}

	if (!found) {
	if ((op->attrs & DM_B_SYMLINK)) {
		rpath = DmResolveSymLink(DmObjPath(op), &real_path,
				&real_name);
		if (rpath == NULL) {
			/* can't resolve symbolic link */
			return;
		}
	} else {
		/* check if dirname of object is a symbolic link */
		char *dup_path = STRDUP(DmObjPath(op));
		char *dir_name = dirname(dup_path);
		rpath = DmResolveSymLink(DmObjPath(op), &real_path,
				&real_name);
		FREE(dup_path);
	}

	for (; fnkp; fnkp = fnkp->next) {
		/*
		 * Skip new entries that haven't been applied yet.
		 * Note that deleted entries that haven't been applied are
		 * still being used.
		 */
		if (fnkp->attrs & (DM_B_NEW | DM_B_CLASSFILE | DM_B_OVERRIDDEN
			| DM_B_INACTIVE_CLASS | DM_B_MANUAL_CLASSING_ONLY))
			continue;

		if ((fnkp->attrs & DM_B_FILETYPE) &&(op->ftype != fnkp->ftype))
				continue;

		if (!(fnkp->attrs & DM_B_SYS) && fnkp->fstype &&
			(OBJ_FILEINFO(op)->fstype != fnkp->fstype))
				continue;

		if (fnkp->attrs & DM_B_FILEPATH) {
		    p = DtGetProperty(&(fnkp->fcp->plist), FILEPATH, NULL);
		    p = Dm__expand_sh(p, DmObjProp, (XtPointer)op);
			if (!DmMatchFilePath((p[0] == '/' &&
				p[1] == '/') ? p + 1 : p, path))
			{
				free(p);
				continue;
			}
			free(p);
		}

		if (fnkp->attrs & DM_B_LFILEPATH)
		{
			char *lfpath;
			char *free_this;

		    if ( !(op->attrs & DM_B_SYMLINK) && rpath == NULL )
			continue;

			if (!(lfpath = DmResolveLFILEPATH(fnkp, NULL)) ||
			  (!DmMatchFilePath((lfpath[0] == '/' &&
			  lfpath[1] == '/') ? lfpath + 1 : lfpath, real_path)))
			{
				continue;
			}
		}

		if (fnkp->attrs & DM_B_REGEXP) {
			char *p2;
			char *free_this;

			for (p2=fnkp->re; *p2; p2=p2+strlen(p2)+1) {
				free_this = DmToUpper(p2);
				if (gmatch(name, p2) || (free_this &&
				  gmatch(name, free_this)))
				{
					/* found it */
					if (free_this)
						free(free_this);
					break;
				}
				if (free_this)
					free(free_this);
			}

			if (*p2 == '\0')
				continue;
		}

		if (fnkp->attrs & DM_B_LREGEXP) {
		    char *p2;
		    char *free_this;

		    if ( !(op->attrs & DM_B_SYMLINK) )
			continue;

		    for (p2=fnkp->lre; *p2; p2=p2+strlen(p2)+1) {
			free_this = DmToUpper(p2);
			if (gmatch(real_name, p2) || (free_this &&
			  gmatch(real_name, free_this)))
			{
				/* found it */
				if (free_this)
					free(free_this);
				break;
			}
			if (free_this)
				free(free_this);
		    }

		    if (*p2 == '\0')
			continue;
		}

		/* found it! */
		op->fcp = fnkp->fcp;
		break;
	}
	} /* if (!found) */
	free(path);
	if (rpath)
		free(rpath);
}

void
DmFreeFileClass(fnkp)
DmFnameKeyPtr fnkp;
{
#ifdef NOT_USE
	XtFree(fnkp->name);
	XtFree(fnkp->re);

	if (fnkp->fcp) {
		DtFreePropertyList(&(fnkp->fcp->plist));
		if (fnkp->fcp->glyph) {
			DmReleasePixmap(DESKTOP_SCREEN(Desktop),
					fnkp->fcp->glyph);
		}
		free(fnkp->fcp);
	}

	free(fnkp);
#endif
}

/****************************procedure*header*****************************
    DmResolveLFILEPATH - resolves any symbolic links in LFILEPATH up to
	the first metacharacter (only '*' is supported) and stores the
	resolved path in fnkp.
*/
char *
DmResolveLFILEPATH(fnkp, p)
DmFnameKeyPtr fnkp;
char *p;
{
	char *p1;
	char *p2;
	char *p3;

	if (fnkp->lfpath)
		return(fnkp->lfpath);

	if (p == NULL)
		p = DtGetProperty(&(fnkp->fcp->plist), LFILEPATH, NULL);

	/* expand any environment variable */
	p2 = p1 = Dm__expand_sh(p, NULL, NULL);
	p3 = STRDUP(p1);

	/* if regular expression, just resolve up to the 1st metacharacter */
	if (strtok(p1, "*")) {
		if (realpath(p1, Dm__buffer)) {
			char buf[PATH_MAX];
			char *p4 = strchr(p3, '*');

			if (p4) {
				sprintf(buf, "%s/%s", Dm__buffer, p4);
				fnkp->lfpath = STRDUP(buf);
			} else
				fnkp->lfpath = STRDUP(Dm__buffer);
		} else {
			fnkp->lfpath = NULL;
		}
	} else {
		/* get realpath of LFILEPATH */
		if (realpath(p1, Dm__buffer)) {
			fnkp->lfpath = strdup(Dm__buffer);
		} else {
			fnkp->lfpath = NULL;
		}
	}

	/* p2 was malloc'ed in Dm__expand_sh() */
	FREE(p2);
	FREE(p3);
	return(fnkp->lfpath);

} /* end of DmResolveLFILEPATH */

/****************************procedure*header*****************************
    DmMatchFilePath - Does pattern matching of a given file path with a
	pattern.  Only works with '*' and '?' from the set or regular expressions. 
	Returns 1 on a match; 0 otherwise.
*/
Boolean
DmMatchFilePath(p, path)
char *p;	/* pattern */
char *path;	/* file path */
{
    char *s1;
    char *s2;
    char nextc;
    int match = 0;
    char *star = NULL;
    char *star_match_end = NULL;
    char pattern_buf[PATH_MAX];
    char path_buf[PATH_MAX];

    /* an empty pattern matches a NULL path */
    if (p == NULL && path == NULL)
	return(1);
    else if (p == NULL || path == NULL)
	return(0);
    
    strcpy(pattern_buf, p);
    if (!strtok(pattern_buf, "*?")) {
	return(strcmp(p, path) ? 0 : 1);
    }
    
    strcpy(pattern_buf, p);
    strcpy(path_buf, path);
    
    s1 = pattern_buf;
    s2 = path_buf;

loop:    
    /* compare each character in s1 and s2 one at a time */
    while (*s1) {
	if (*s2 == '\0') {
	    /* end of path, pattern contains more chars */
	    match = 0;
	    goto bye;
	} else if (*s1 == '?') {
	    ++s1;
	    ++s2;
	    continue;
	} else if (*s1 == '*'){
	    
	    star_match_end = s2;	/* last char 'swallowed' by '*' */
	    star = s1;
	    
	    /* find character after '*' */
	    nextc = *(s1+1);
	    if (nextc == '\0') {
		/* s1 ends in '*'. Look for next '/' in s2 or end of s2 
		 * '*' at end of pattern only matches up to a single
		 * path component
		 */
		s2 = strchr(s2, '/');
		if (!s2 || *(s2+1) == nextc)
		    match = 1;
		else
		    match = 0;
		goto bye;
	    } else {
		/* search s2 for char that follows '*' in s1 */
		while (*s2 && *s2 != nextc) {
		    /* begin matching nextc with first match found */
		    star_match_end = s2;
		    ++s2;
		}
		if (*s2 == '\0') {
		    match = 0;
		    goto bye;
		} else
		    --s2;
	    }
	} else {
	    if (*s1 != *s2) {
		if (star != NULL && star_match_end < s2){
		    /* Look for another match in s2 to the first character
		       following * in s1 */
		    s1 = star-1;	/* next time will be at star */
		    s2 = star_match_end;  /* next will be at star_match_end +1 */
		}	
		else{
		    match = 0;
		    goto bye;	
		}
	    }
	} 
	++s1;
	++s2;
    }
    if (*s2 == '\0')
	match = 1;
    else{
	if (star != NULL){
	    s1 = star;
	    s2 = star_match_end+1;
	    goto loop;
	}
	match = 0;
    }
    
 bye:
    return(match);
    
} /* end of DmMatchFilePath */

/****************************procedure*header*****************************
     DmToUpper - Converts string to upper case.
 */
char *
DmToUpper(char *str)
{
	int i = 0;
	char buf[256];
	char *s;

	if (str == NULL || *str == '\0')
		return(NULL);

	s = str;
	for (; *s; s++)
		buf[i++] = (char)toupper((int)((unsigned char)*s));
	buf[i] = '\0';
	return(strdup(buf));

} /* end of DmToUpper */
