#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/sharetab.c	1.7"
#endif

/* this file also known as the following.  free(), malloc() and 
** strdup() were changed to FREE(), MALLOC() and STRDUP().
** Some routines may be deleted.
** Other routines made Global.

#ident	"@(#)nfs.cmds:nfs/share/sharetab.c	1.2.5.1"
#ident "$Header$"

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Intrinsic.h>	/* needed by nfs.h */
#include <DtI.h>		/* needed by nfs.h */
#include <Gizmo/Gizmos.h>	/* needed by nfs.h */
#include <Gizmo/BaseWGizmo.h>   /* needed by nfs.h */
#include "nfs.h"		/* defines NO_MEMORY_EXIT() */
#include "sharetab.h"

extern struct share *	sharedup();
extern void		sharefree();

/*
 * Get an entry from the share table.
 * There should be at least 4 fields:
 *
 * 	pathname  resource  fstype  options  [ description ]
 *
 * A fifth field (description) is optional.
 *
 * Returns:
 *	> 1  valid entry
 *	= 0  end of file
 *	< 0  error
 */
int
getshare(fd, shp)
	FILE *fd;
	struct share **shp;
{
	static char *line = NULL;
	static struct share *sh = NULL;
	register char *p;
	char *w = " \t";

	if (line == NULL) {
		line = (char *) MALLOC(BUFSIZ+1);
		if (line == NULL)
		    NO_MEMORY_EXIT();
	}
	if (sh == NULL) {
		sh = (struct share *) MALLOC(sizeof(*sh));
		if (sh == NULL)
		    NO_MEMORY_EXIT();
	}

	p = fgets(line, BUFSIZ, fd);
	if (p == NULL)
		return (0);
	line[strlen(line) - 1] = '\0';

	sh->sh_path = strtok(p, w);
	if (sh->sh_path == NULL)
		return (-1);
	sh->sh_res = strtok(NULL, w);
	if (sh->sh_res == NULL)
		return (-1);
	sh->sh_fstype = strtok(NULL, w);
	if (sh->sh_fstype == NULL)
		return (-1);
	sh->sh_opts = strtok(NULL, w);
	if (sh->sh_opts == NULL)
		return (-1);
	sh->sh_descr = strtok(NULL, "");
	if (sh->sh_descr == NULL)
		sh->sh_descr = "";

	*shp = sh;
	return (1);
}

/*
 * Append an entry to the sharetab file.
 */
int
putshare(fd, sh)
	FILE *fd;
	struct share *sh;
{
	int r;

	if (fseek(fd, 0L, 2) < 0)
		return (-1);

	r = fprintf(fd, "%s\t%s\t%s\t%s\t%s\n",
		sh->sh_path,
		sh->sh_res,
		sh->sh_fstype,
		sh->sh_opts,
		sh->sh_descr);
	return (r);
}

/*
 * The entry corresponding to path is removed from the
 * sharetab file.  The file is assumed to be locked.
 * Read the entries into a linked list of share structures
 * minus the entry to be removed.  Then truncate the sharetab
 * file and write almost all of it back to the file from the
 * linked list.
 * Note: The file is assumed to be locked.
 */
int
remshare(fd, path)
	FILE *fd;
	char *path;
{
	struct share *sh_tmp;
	struct shl {			/* the linked list */
		struct shl   *shl_next;
		struct share *shl_sh;
	};
	struct shl *shl_head = NULL;
	struct shl *shl, *prev, *next;
	int res, remcnt;

	rewind(fd);
	remcnt = 0;
	while ((res = getshare(fd, &sh_tmp)) > 0) {
		if ((strcmp(path, sh_tmp->sh_path) == 0 ||
		     strcmp(path, sh_tmp->sh_res)  == 0) &&
		    strcmp(sh_tmp->sh_fstype, "nfs") == 0) {
			remcnt++;
		} else {
			prev = shl;
			shl = (struct shl *) MALLOC(sizeof(*shl));
			if (shl == NULL) {
				res = -1;
				goto dealloc;
			}
			if (shl_head == NULL)
				shl_head = shl;
			else
				prev->shl_next = shl;
			shl->shl_next = NULL;
			shl->shl_sh = sharedup(sh_tmp);
			if (shl->shl_sh == NULL) {
				res = -1;
				goto dealloc;
			}
		}
	}
	if (res < 0)
		goto dealloc;
	if (remcnt == 0) {
		res = 1;	/* nothing removed */
		goto dealloc;
	}

	if (ftruncate(fileno(fd), 0) < 0) {
		res = -1;
		goto dealloc;
	}

	for (shl = shl_head ; shl ; shl = shl->shl_next)
		putshare(fd, shl->shl_sh);
	res = 1;

dealloc:
	for (shl = shl_head ; shl ; shl = next) {
		sharefree(shl->shl_sh);
		next = shl->shl_next;
		FREE((char *)shl);
	}
	return (res);
}

extern struct share *
sharedup(sh)
	struct share *sh;
{
	struct share *nsh;
	
	nsh = (struct share *) MALLOC(sizeof(*nsh));
	if (nsh == NULL)
		return (NULL);
	nsh->sh_path = STRDUP(sh->sh_path);
	if (nsh->sh_path == NULL)
		goto alloc_failed;
	nsh->sh_res = STRDUP(sh->sh_res);
	if (nsh->sh_res == NULL)
		goto alloc_failed;
	nsh->sh_fstype = STRDUP(sh->sh_fstype);
	if (nsh->sh_fstype == NULL)
		goto alloc_failed;
	nsh->sh_opts = STRDUP(sh->sh_opts);
	if (nsh->sh_opts == NULL)
		goto alloc_failed;
	nsh->sh_descr = STRDUP(sh->sh_descr);
	if (nsh->sh_descr == NULL)
		goto alloc_failed;
	return (nsh);

alloc_failed:
	sharefree(nsh);
	return (NULL);
}

extern void
sharefree(sh)
	struct share *sh;
{
	if (sh == NULL)
		return;
	if (sh->sh_path != NULL)
		FREE(sh->sh_path);
	if (sh->sh_res != NULL)
		FREE(sh->sh_res);
	if (sh->sh_fstype != NULL)
		FREE(sh->sh_fstype);
	if (sh->sh_opts != NULL)
		FREE(sh->sh_opts);
	if (sh->sh_descr != NULL)
		FREE(sh->sh_descr);
	FREE((char *)sh);
}

/*
 * Return the value after "=" for option "opt"
 * in option string "optlist".
 */
char *
getshareopt(optlist, opt)
	char *optlist, *opt;
{
	char *p, *pe;
	char *b;
	static char *bb;

	if (bb)
		FREE(bb);
	b = bb = STRDUP(optlist);
	if (b == NULL)
		return (NULL);

	while (p = strtok(b, ",")) {
		b = NULL;
		if (pe = strchr(p, '=')) {
			*pe = '\0';
			if (strcmp(opt, p) == 0)
				return (pe + 1);
		}
		if (strcmp(opt, p) == 0)
			return ("");
	}

	return (NULL);
}

