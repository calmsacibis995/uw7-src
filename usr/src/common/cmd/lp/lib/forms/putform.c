/*		copyright	"%c%" 	*/

#ident	"@(#)putform.c	1.2"
#ident	"$Header$"

#include "sys/types.h"
#include "sys/stat.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "stdlib.h"

#include "lp.h"
#include "form.h"
#include "printers.h"

/**
 ** putform() - WRITE FORM STRUCTURE TO DISK FILES
 **/

int
#if	defined(__STDC__)
putform (
	char *			name,
	FORM *			formp,
	FALERT *		alertp,
	FILE **			p_align_fp
)
#else
putform (name, formp, alertp, p_align_fp)
	char *			name;
	FORM *			formp;
	FALERT *		alertp;
	FILE **			p_align_fp;
#endif
{
	register char *		path;

	FILE *			fp;

	struct stat		statbuf;

	level_t			lid;

	int			j;

	if (!name || !*name) {
		errno = EINVAL;
		return (-1);
	}

	if (STREQU(NAME_ALL, name)) {
		errno = EINVAL;
		return (-1);
	}

	/*
	 * Create the parent directory for this form
	 * if it doesn't yet exist.
	 */
	if (!(path = getformfile(name, (char *)0)))
		return (-1);
	if (Stat(path, &statbuf) == 0) {
		if (!(statbuf.st_mode & S_IFDIR)) {
			Free (path);
			errno = ENOTDIR;
			return (-1);
		}
	} else if (errno != ENOENT || mkdir_lpdir(path, MODE_DIR) == -1) {
		Free (path);
		return (-1);
	}
	lid = PR_SYS_PUBLIC;
	while (lvlfile (path, MAC_SET, &lid) < 0 &&
	       errno == EINTR)
	    continue;
	Free (path);

	/*
	 * Open the configuration file and write out the form
	 * configuration (?)
	 */
	if (formp) {
		if (!(path = getformfile(name, DESCRIBE)))
			return (-1);
		if (!(fp = open_lpfile(path, "w", MODE_READ))) {
			Free (path);
			return (-1);
		}

		if (wrform(name, formp, fp, 0, (int *)0) == -1) {
			close_lpfile (fp);
			lid = PR_SYS_PUBLIC;
			while (lvlfile (path, MAC_SET, &lid) < 0 &&
			       errno == EINTR)
			    continue;
			Free (path);
			return -1;
		}
		close_lpfile (fp);
		lid = PR_SYS_PUBLIC;
                while ((j=lvlfile (path, MAC_SET, &lid)) < 0 && errno == EINTR)
                    continue;
		Free (path);
		if (j < 0 && errno != ENOSYS)
		    return -1;

	}

	/*
	 * Write out the alert condition (?)
	 */
	if (alertp) {
		if (
			alertp->shcmd
		     && putalert(Lp_A_Forms, name, alertp) == -1
		)
			return (-1);
	}

	/*
	 * Write out the alignment pattern (?)
	 */
	if (p_align_fp && *p_align_fp) {

		int			size	= 0,
					n;

		char			buf[BUFSIZ];


		if (!(path = getformfile(name, ALIGN_PTRN)))
			return (-1);
		if (!(fp = open_lpfile(path, "w", MODE_READ))) {
			Free (path);
			return (-1);
		}

		while ((n = fread(buf, 1, BUFSIZ, *p_align_fp)) != 0) {
			size += n;
			(void)fwrite (buf, 1, n, fp);
		}
		close_lpfile (fp);

		if (!size)
			Unlink(path);
		else {
		    lid = PR_SYS_PUBLIC;
		    while ((j=lvlfile (path, MAC_SET, &lid)) < 0 &&
			   errno == EINTR)
			continue;
		    
		    if (j < 0 && errno != ENOSYS) {
			Free (path);
			return -1;
		    }
		}
		Free(path);
	}

	return (0);
}
