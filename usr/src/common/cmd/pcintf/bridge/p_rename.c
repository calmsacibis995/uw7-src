#ident	"@(#)pcintf:bridge/p_rename.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)p_rename.c	6.5	LCC);	/* Modified: 14:17:08 1/7/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>

#ifdef	RLOCK
#include <fcntl.h>
#include <rlock.h>
#endif	/* RLOCK */

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "const.h"
#include "dossvr.h"
#include "log.h"
#include "table.h"
#include "xdir.h"

#define sDbg(x)

#define	INNAME(c)	((c) != DOT && (c) != '\0')
#define	TMPDIR		"/tmp/"
#define	MV		"mv"

#ifndef	EOS
#define	EOS		0
#endif	/* EOS */

LOCAL void	form_target	PROTO((char *, char *, char *));
LOCAL int	do_rename	PROTO((char *, char **));


typedef struct filelist {
	char srcname[MAX_FN_COMP+1]; /* room for a simple filename (no path) */
	char dstname[MAX_FN_COMP+1];
	struct temp_slot *tslot;
	struct filelist *next;
} FILELIST;
FILELIST *flhead;

static char *args[] = { MV, NULL, NULL, NULL };	/* gets passed to exec_cmd() */


void pci_rename(source, target, request, mode, addr, pid)
	char *source, *target;
	int request;			/* distinguish between
					    old style/new style rename	*/
	int mode;
	struct output *addr;
	int		pid;		/* process ID of MSDOS process */
{
	register FILELIST *flp;
	register struct dirent *dp;	/* dir ptr from readdir() */
	register DIR *dirp;		/* dir ptr from opendir() */
	char s_pattern[MAX_FN_COMP+1];
	char d_pattern[MAX_FN_COMP+1];
	char s_path[MAX_FN_TOTAL+1];
	char d_path[MAX_FN_TOTAL+1];
	char s_name[MAX_FN_TOTAL+1];
	char d_name[MAX_FN_TOTAL+1];
	char tmpname[MAX_FN_TOTAL+1];
	char tmpcomp[MAX_FN_COMP+1];
	struct stat filestat;
	struct temp_slot *tslot;
	register FILELIST *p;
	int has_wildcard;
	int rc;
	int vdescriptor;

	addr->hdr.stat = NEW;
	flhead = NULL;

	sDbg(("rename(%s,%s,%d)\n",source,target,mode));
	cvt_fname_to_unix(UNMAPPED, (unsigned char *)source,
	    (unsigned char *)s_name);

	if (has_wildcard = wildcard(s_name, strlen(s_name))) {
	    if (mode == UNMAPPED || request == RENAME_NEW) {
		/* wildcards are disallowed in file handle renames */
		addr->hdr.res = PATH_NOT_FOUND;		/* sic */
		return;
	    }
	    cvt_fname_to_unix(UNMAPPED, (unsigned char *)target,
		(unsigned char *)d_name);
	    getpath(s_name, s_path, s_pattern);
	    getpath(d_name, d_path, d_pattern);
	    set_tables(U2U);
	    lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_LOWERCASE, 0,country);
	    lcs_translate_string(tmpname, MAX_FN_TOTAL, s_path);
	    strcpy(s_path, tmpname);
	    unmapfilename(CurDir, s_path);
	    lcs_translate_string(tmpname, MAX_FN_TOTAL, d_path);
	    strcpy(d_path, tmpname);
	    unmapfilename(CurDir, d_path);
	    lcs_translate_string(tmpname, MAX_FN_COMP, d_pattern);
	    strcpy(d_pattern, tmpname);
	} else {	/* No wildcards */
	    cvt_fname_to_unix(mode, (unsigned char *)source,
		(unsigned char *)s_name);
	    cvt_fname_to_unix(mode, (unsigned char *)target,
		(unsigned char *)d_name);
	    getpath(s_name, s_path, s_pattern);
	    getpath(d_name, d_path, d_pattern);
	}

	/* kludge to allow access to protected directories for tmps */
	tslot = NULL;
	while ((tslot = redirdir(source, (tslot != NULL) ? tslot->ts_next 
							 : NULL)) != NULL) {
	    flp = (FILELIST *)memory(sizeof(FILELIST));
	    strcpy(flp->srcname, tslot->ts_fname);
	    strcpy(tmpname, flp->srcname);
	    if (has_wildcard) {
		mapfilename(TMPDIR, tmpname);
		lcs_translate_string(tmpcomp, MAX_FN_COMP, tmpname);
		strcpy(tmpname, tmpcomp);
	    }
	    form_target(flp->srcname, d_pattern, flp->dstname);
	    flp->tslot = tslot;
	    flp->next = flhead;
	    flhead = flp;
	}

	if (access(d_path, 00) < 0) {
	    addr->hdr.res = PATH_NOT_FOUND;
	    goto free_list;
	}

	sDbg(("s_path=%s\n", s_path));
	if (has_wildcard) {
	    if ((dirp = opendir(s_path)) == NULL)
	    {
		if (flhead != NULL)
		    goto found_some;
		addr->hdr.res = ACCESS_DENIED;
		goto free_list;
	    }
	    sDbg(("s_pattern=%s\n", s_pattern));
	    while (dp = readdir(dirp)) {            /* find all valid files */
		/* check for filenames "." and ".." */
		if (dp->d_name[0] == '.' &&
		  ((dp->d_name[1] == '.' && dp->d_name[2] == '\0') ||
		    dp->d_name[1] == '\0'))
			continue;
		/* see if we've found a filename match */
		sDbg(("   match %s ?\n", dp->d_name));
		strcpy(tmpcomp, dp->d_name);
		mapfilename(s_path, tmpcomp);
		if (match(tmpcomp, s_pattern, MAPPED)) {
		    /* Construct pathname for stat() */
		    strcpy(tmpname, s_path);
		    strcat(tmpname, "/");
		    strcat(tmpname, dp->d_name);
		    sDbg(("pathname for stat=%s\n", tmpname));

		    if ((stat(tmpname, &filestat)) == 0) {
			/*   Check to see if the name matched on is
			 *   directory -- old style REN cannot be used to rename
			 *   directories under DOS -- non-wildcard directory
			 *   names are caught before this point
			 */
			sDbg(("  mode=%x\n", filestat.st_mode));
			if ((filestat.st_mode & S_IFMT) == S_IFDIR) {
			    sDbg((" ** match=%s, but is directory\n",
				dp->d_name));
			} else {	/* Not a directory */
			    sDbg((" ** match=%s\n", dp->d_name));
			    flp = (FILELIST *)memory(sizeof(FILELIST));
			    strcpy(flp->srcname, dp->d_name);
			    set_tables(U2U);
			    lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_LOWERCASE, 0,country);
			    lcs_translate_string(tmpname, MAX_FN_COMP, tmpcomp);
			    form_target(tmpname, d_pattern, flp->dstname);
			    flp->tslot = NULL;
			    flp->next = flhead;
			    flhead = flp;
			}
		    }
		}
	    }
	    closedir(dirp);
	} else {	/* Not wildcard */
	    sDbg(("pathname for stat=%s\n", s_name));

	    if ((stat(s_name, &filestat)) == 0) {
		sDbg(("  mode=%x\n", filestat.st_mode));
		if (((filestat.st_mode & S_IFMT) == S_IFDIR) &&
		    request != RENAME) {
		    sDbg((" ** match=%s, but is directory\n", s_pattern));
		} else {	/* Not a directory */
		    sDbg((" ** match=%s\n", s_pattern));
		    flp = (FILELIST *)memory(sizeof(FILELIST));
		    strcpy(flp->srcname, s_pattern);
		    form_target(flp->srcname, d_pattern, flp->dstname);
		    flp->tslot = NULL;
		    flp->next = flhead;
		    flhead = flp;
		}
	    }
	}

found_some:
	if (flhead == NULL) {    /* check for no files found */
	    sDbg(("No match\n"));
	    addr->hdr.res = FILE_NOT_FOUND;
	    return;
	}

	/* check for duplicate target name */
	for (flp = flhead; flp; flp = flp->next) {
	    for (p = flhead; p; p = p->next) {
		if (p == flp)
		    continue;
		if (strcmp(p->dstname, flp->dstname) == 0) {
		    addr->hdr.res = ACCESS_DENIED; /* dup target */
		    goto free_list;
		}
	    }
	}

      /* found the source filename(s), so form target(s) and do the moving */
	for (flp = flhead; flp; flp = flp->next) {
	  /* Convert to full path file names */
	    strcpy(d_name, fnQualify(flp->dstname, d_path));
	    strcpy(s_name, fnQualify(flp->srcname,
				    (flp->tslot != NULL)? TMPDIR : s_path));
	    sDbg(("rename %s to %s\n", s_name, d_name));
	    if (access(d_name, 0) == 0) {  /* fail if target exists */
		addr->hdr.res = ACCESS_DENIED;	/* target exists */
		goto free_list;
	    } else {
#ifdef RLOCK  /* record locking */
		if ((vdescriptor = open_file(s_name, O_RDONLY, SHR_RDWRT,
						pid, request)) < 0) {
		    addr->hdr.res = (unsigned char)-vdescriptor; /* err code */

		    goto free_list;
		}
#endif  /* RLOCK */
		args[1] = s_name;
		args[2] = d_name;
		rc = do_rename(MV, args);
#ifdef RLOCK
		close_file(vdescriptor,0);
#endif	/* RLOCK */
		if(rc) {	/* do the move */
		    /* update name/pathname in open file table */
		    changename(s_name, d_name);
		    if (flp->tslot != NULL)
			remove_redirdir(flp->tslot);
		} else {
		    addr->hdr.res = FILE_NOT_FOUND; /* MV failed */
		    goto free_list;
		}
	    }
	}
	addr->hdr.res = SUCCESS;
free_list:
	for (flp = flhead; flp; ) {
	    p = flp;
	    flp = flp->next;
	    free(p);
	}
	flhead = NULL;
	return;
}

/*
 * form_target:	Forms the target name based on the source name.
 *
 * input	s- source name, a real file name
 *		t- target pattern, may contain wildcards
 *
 * processing	creates a new file name by modifying the given
 *		source name to fit the given target pattern
 *
 * This function is a cleaned up version of the original and was lifted
 * out of PCI/SMB. It fixes the bug with renaming wild cards that was
 * in the original code.	[11/4/88 efh]
 */

void form_target(s, t, target)
register char	*s;			/* ptr to source name */
register char	*t;			/* ptr to target name */
char *target;				/* destination */
{
register char	*tp = target;		/* ptr to buffer */

	/* Process basename (part before the '.') first */
	for (;;) {
		if (*s == EOS)
			goto baseOut;
		else if (*s == DOT) {
			while (INNAME(*t)) { /* process rest of target prefix */
				if (*t == QUESTION || *t == STAR) {
					while (INNAME(*t))
						t++;
					break;
				}
				else
					*tp++ = *t++;
			}
			goto baseOut;
		} else {
			switch (*t) {
			case STAR:
				/* copy source chars */
				while (INNAME(*s))
					*tp++ = *s++;
				/* skip stuff after * in dest */
				while (INNAME(*t))
					t++;
				break;

			case QUESTION:	/* copy single source char */
				*tp++ = *s++;
				t++;
				break;

			case DOT:	/* skip rest of source prefix */
				while (INNAME(*s))
					s++;
				break;

			default:	/* regular character - copy over */
				*tp++ = *t++;
				s++;
				break;
			}
		}
	}

baseOut:
	/* handle suffix */
	if (*t != EOS) {
		/* Skip over dot */
		if (*s != EOS)
			s++;

		if (*t == DOT)
			*tp++ = *t++;

		while (*t) {
			switch (*t) {
			case STAR:
			case EOS:
				while (*s)
					*tp++ = *s++;
				*tp = EOS;
				return;

			case QUESTION:
				if (*s) *tp++ = *s;
				t++;
				break;

			default:	/* regular character */
				*tp++ = *t++;
				break;
			}

			if (*s != EOS)
				s++;
		}
	}

	*tp = EOS;
	return;
}

int
do_rename(mv, args)
char *mv, *args[];
{
    char *cp1, *cp2;
    int flag;
    
    log("renaming %s to %s\n",args[1],args[2]);
    cp1 = strrchr(args[1],'/');
    if (cp1) *cp1 = 0;
    cp2 = strrchr(args[2],'/');
    if (cp2) *cp2 = 0;
    flag = strcmp(args[1], args[2]);
    if (cp1) *cp1 = '/';
    if (cp2) *cp2 = '/';
    
    if (flag == 0) {
	log("attempting fast rename\n");
	if (link(args[1], args[2]) == 0) {
	    if (unlink(args[1]) == 0) {
		return(1);			/* successful */
	    }
	    else {
		log("unlink failed\n");
		unlink(args[2]);
	    }
	}
    }
    log("exec'ing mv\n");
    return (exec_cmd(mv, args));
}
