/*		copyright	"%c%" 	*/


#ident	"@(#)putprinter.c	1.3"
#ident  "$Header$"

/*******************************************************************************
 *
 * FILENAME:    putprinter.c
 *
 * DESCRIPTION: Create or Modify a printer by writing the printer structure
 *              to disk files.
 *
 * SCCS:	putprinter.c 1.3  7/3/97 at 11:33:16
 *
 * CHANGE HISTORY:
 *
 * 03-07-97  Paul Cunningham        ul97-04808
 *           Change function putprinter() so that it no longer links to the
 *           standard interface models in /etc/lp/model, but always copies
 *           the interface file into /etc/lp/interface instead. Added the
 *           compile option USE_SYMLINK to remove the link code (so that it
 *           can be put back in very easily).
 *
 *******************************************************************************
 */

#include "sys/types.h"
#include "sys/stat.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"

#include "lp.h"
#include "printers.h"

extern struct {
	char			*v;
	short			len,
				okremote;
}			prtrheadings[];

#if	defined(__STDC__)

static void		print_sdn (FILE *, char *, SCALED);
static void		print_l (FILE *, char *, char **);
static void		print_str (FILE *, char *, char *);

#else

static void		print_sdn(),
			print_l(),
			print_str();

#endif

unsigned long		ignprinter	= 0;

/**
 ** putprinter() - WRITE PRINTER STRUCTURE TO DISK FILES
 **/

int
#if	defined(__STDC__)
putprinter (
	char *			name,
	PRINTER *		prbufp
)
#else
putprinter (name, prbufp)
	char			*name;
	PRINTER			*prbufp;
#endif
{
	register char *		path;
	register char *		stty;
	register char *		speed;

	FILE *			fp;
	FILE *			fpin;

	int			fld;

	char			buf[BUFSIZ];

	struct stat		statbuf1,
				statbuf2;
	level_t			lid;
	int			n;


	badprinter = 0;

	if (!name || !*name) {
		errno = EINVAL;
		return (-1);
	}

	if (STREQU(NAME_ALL, name)) {
		errno = EINVAL;
		return (-1);
	}

	/*
	 * First go through the structure and see if we have
	 * anything strange.
	 */
	if (!okprinter(name, prbufp, 1)) {
		errno = EINVAL;
		return (-1);
	}

	/* CONSTCOND */
	if (!Lp_A_Printers || !Lp_A_Interfaces) {
		getadminpaths (LPUSER);
		/* CONSTCOND */
		if (!Lp_A_Printers || !Lp_A_Interfaces)
			return (0);
	}

	/*
	 * Create the parent directory for this printer
	 * if it doesn't yet exist.
	 */
	if (!(path = getprinterfile(name, (char *)0)))
		return (-1);
	if (Stat(path, &statbuf1) == 0) {
		if (!(statbuf1.st_mode & S_IFDIR)) {
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
	 * Create the copy of the interface program, unless
	 * that would be silly or not desired.
	 * Conversely, make sure the interface program doesn't
	 * exist for a remote printer.
	 */
	if (prbufp->remote) {
		if (!(path = makepath(Lp_A_Interfaces, name, (char *)0)))
			return (-1);
		(void)rmfile (path);
		Free (path);
	}
	if (prbufp->interface && (ignprinter & BAD_INTERFACE) == 0) {
		if (Stat(prbufp->interface, &statbuf1) == -1)
			return (-1);
		if (!(path = makepath(Lp_A_Interfaces, name, (char *)0)))
			return (-1);
		if (
			Stat(path, &statbuf2) == -1
		     || statbuf1.st_dev != statbuf2.st_dev
		     || statbuf1.st_ino != statbuf2.st_ino)
		{
			register int		n, link_files;
			
			Unlink(path); 			/* abs s20 */

#ifdef USE_SYMLINK
			/* if the interface is a standard one, then
			 * try to link the files instead of copying.  abs s19.2
			 */
			link_files = (strncmp(Lp_Model, prbufp->interface,
					      strlen(Lp_Model)) == 0) ? 1 : 0;
			if (link_files &&
			    ((n = Symlink(prbufp->interface, path)) == 0) ) {
			       (void)lchown_lppath(path);
			}
			else
			if (!link_files || errno == ENOSYS)
			{
			    /* non-std interface or 
			     * filesystem doesn't support symbolic links
			     * so copy the file instead.
			     */
#else
			    /* ul97-04808
			     * Always copy the interface file
			     */
#endif
			    if (!(fpin = open_lpfile(prbufp->interface, "r",0)))
			    {
				Free (path);
				return (-1);
			    }
			    if (!(fp = open_lpfile(path, "w", MODE_EXEC))) {
				Free (path);
				close_lpfile (fpin);
				return (-1);
			    }
			    while ((n = fread(buf, 1, BUFSIZ, fpin)) > 0)
				(void)fwrite (buf, 1, n, fp);
			    close_lpfile (fp);
			    close_lpfile (fpin);
			    lid = PR_SYS_PUBLIC;
			    while ((n=lvlfile (path, MAC_SET, &lid)) < 0
				   && errno == EINTR)
				continue;
			    
			    if (n < 0 && errno != ENOSYS)
			    {
				Free (path);
				return	-1;
			    }
#ifdef USE_SYMLINK
			}
			else 
			{	/* symlink failed for other than ENOSYS */
			    Free (path);
			    return -1;
			}
#endif
		}
		Free (path);
	}

	/*
	 * If this printer is dialed up, remove any baud rates
	 * from the stty option list and move the last one to
	 * the ".speed" member if the ".speed" member isn't already
	 * set. Conversely, if this printer is directly connected,
	 * move any value from the ".speed" member to the stty list. 
	 */

	stty = (prbufp->stty? Strdup(prbufp->stty) : 0);
	if (prbufp->speed)
		speed = Strdup(prbufp->speed);
	else
		speed = 0;

	if (prbufp->dial_info && stty) {
		register char		*newstty,
					*p,
					*q;

		register int		len;

		if (!(q = newstty = Malloc(strlen(stty) + 1))) {
			Free (stty);
			errno = ENOMEM;
			return (-1);
		}
		newstty[0] = 0;	/* start with empty copy */

		for (
			p = strtok(stty, " ");
			p;
			p = strtok((char *)0, " ")
		) {
			len = strlen(p);
			if (strspn(p, "0123456789") == len) {
				/*
				 * If "prbufp->speed" isn't set, then
				 * use the speed we just found. Don't
				 * check "speed", because if more than
				 * one speed was given in the list, we
				 * want the last one.
				 */
				if (!prbufp->speed) {
					if (speed)
						Free (speed);
					speed = Strdup(p);
				}

			} else {
				/*
				 * Not a speed, so copy it to the
				 * new stty string.
				 */
				if (q != newstty)
					*q++ = ' ';
				(void)strcpy (q, p);
				q += len;
			}
		}

		Free (stty);
		stty = newstty;

	} else if (!prbufp->dial_info && speed) {
		register char		*newstty;

		newstty = Malloc(strlen(stty) + 1 + strlen(speed) + 1);
		if (!newstty) {
			if (stty)
				Free (stty);
			errno = ENOMEM;
			return (-1);
		}

		if (stty) {
			(void)strcpy (newstty, stty);
			(void)strcat (newstty, " ");
			(void)strcat (newstty, speed);
			Free (stty);
		} else
			(void)strcpy (newstty, speed);
		Free (speed);
		speed = 0;

		stty = newstty;

	}

	/*
	 * Open the configuration file and write out the printer
	 * configuration.
	 */

	if (!(path = getprinterfile(name, CONFIGFILE))) {
		if (stty)
			Free (stty);
		if (speed)
			Free (speed);
		return (-1);
	}
	if (!(fp = open_lpfile(path, "w", MODE_READ))) {
		Free (path);
		if (stty)
			Free (stty);
		if (speed)
			Free (speed);
		return (-1);
	}
	/*
	**  'path' is free'd below after the close_lpfile().
	*/
	for (fld = 0; fld < PR_MAX; fld++) {
		if (prbufp->remote && !prtrheadings[fld].okremote)
			continue;

		switch (fld) {

#define HEAD	prtrheadings[fld].v

		case PR_BAN:
			(void)fprintf (
				fp,
				"%s %s",
				HEAD,
				prbufp->banner & BAN_OFF? NAME_OFF : NAME_ON
			);
			if (prbufp->banner & BAN_ALWAYS)
				(void)fprintf (fp, ":%s", NAME_ALWAYS);
			(void)fprintf (fp, "\n");
			break;

		case PR_CPI:
			print_sdn (fp, HEAD, prbufp->cpi);
			break;

		case PR_CS:
			if (!emptylist(prbufp->char_sets))
				print_l (fp, HEAD, prbufp->char_sets);
			break;

		case PR_ITYPES:
			/*
			 * Put out the header even if the list is empty,
			 * to distinguish no input types from the default.
			 */
			print_l (fp, HEAD, prbufp->input_types);
			break;

		case PR_DEV:
			print_str (fp, HEAD, prbufp->device);
			break;

		case PR_DIAL:
			print_str (fp, HEAD, prbufp->dial_info);
			break;

		case PR_RECOV:
			print_str (fp, HEAD, prbufp->fault_rec);
			break;

		case PR_INTFC:
			print_str (fp, HEAD, prbufp->interface);
			break;

		case PR_LPI:
			print_sdn (fp, HEAD, prbufp->lpi);
			break;

		case PR_LEN:
			print_sdn (fp, HEAD, prbufp->plen);
			break;

		case PR_LOGIN:
			if (prbufp->login & LOG_IN)
				(void)fprintf (fp, "%s\n", HEAD);
			break;

		case PR_PTYPE:
		{
			char			**printer_types;

			/*
			 * For backward compatibility for those who
			 * use only "->printer_type", we have to play
			 * some games here.
			 */
			if (prbufp->printer_type && !prbufp->printer_types)
				printer_types = getlist(
					prbufp->printer_type,
					LP_WS,
					LP_SEP
				);
			else
				printer_types = prbufp->printer_types;

			if (!printer_types || !*printer_types)
				print_str (fp, HEAD, NAME_UNKNOWN);
			else
				print_l (fp, HEAD, printer_types);

			if (printer_types != prbufp->printer_types)
				freelist (printer_types);
			break;
		}

		case PR_REMOTE:
			print_str (fp, HEAD, prbufp->remote);
			break;

		case PR_SPEED:
			print_str (fp, HEAD, speed);
			break;

		case PR_STTY:
			print_str (fp, HEAD, stty);
			break;

		case PR_WIDTH:
			print_sdn (fp, HEAD, prbufp->pwid);
			break;

#if	defined(CAN_DO_MODULES)
		case PR_MODULES:
			/*
			 * Put out the header even if the list is empty,
			 * to distinguish no modules from the default.
			 */
			print_l (fp, HEAD, prbufp->modules);
			break;
#endif
		/*
		**  If 'remote' is not defined then range is not
		**  applicable.  In fact hilevel and lolevel will
		**  only contain the defaults.
		*/
		case PR_RANGE:
			if (! prbufp->remote)
				break;
			(void)	fprintf (fp, "%s %ld,%ld\n", HEAD,
			(long) prbufp->hilevel, (long) prbufp->lolevel);
			break;
		case PR_USER:
			print_str (fp, HEAD, prbufp->user);
			break;

		case PR_FF:
			(void)fprintf (
				fp,
				"%s %s",
				HEAD,
				prbufp->nw_flags & FF_OFF? NAME_OFF : NAME_ON
			);
			if (prbufp->nw_flags & BAN_ALWAYS)
				(void)fprintf (fp, ":%s", NAME_ALWAYS);
			(void)fprintf (fp, "\n");
			break;
		}

	}
	if (stty)
		Free (stty);
	if (speed)
		Free (speed);
	if (ferror(fp)) {
		close_lpfile (fp);
		Free (path);
		return (-1);
	}
	close_lpfile (fp);
	lid = PR_SYS_PUBLIC;
	while ((n=lvlfile (path, MAC_SET, &lid)) < 0 && errno == EINTR)
		continue;

	Free (path);
	if (n < 0 && errno != ENOSYS)
		return	(-1);
	
	/*
	 * If we have a description of the printer,
	 * write it out to a separate file.
	 */
	if (prbufp->description) {

		if (!(path = getprinterfile(name, COMMENTFILE)))
			return (-1);

		if (dumpstring(path, prbufp->description) == -1) {
			Free (path);
			return (-1);
		}
		lid = PR_SYS_PUBLIC;
		while ((n=lvlfile (path, MAC_SET, &lid)) < 0 && errno == EINTR)
			continue;

		Free (path);
		if (n < 0 && errno != ENOSYS)
			return	(-1);
	
	}

	/*
	 * Now write out the alert condition.
	 */
	if (
		prbufp->fault_alert.shcmd
	     && putalert(Lp_A_Printers, name, &(prbufp->fault_alert)) == -1
	)
		return (-1);

	return (0);
}

/**
 ** print_sdn() - PRINT SCALED DECIMAL NUMBER WITH HEADER
 ** print_l() - PRINT (char **) LIST WITH HEADER
 ** print_str() - PRINT STRING WITH HEADER
 **/

static void
#if	defined(__STDC__)
print_sdn (
	FILE *			fp,
	char *			head,
	SCALED			sdn
)
#else
print_sdn (fp, head, sdn)
	FILE			*fp;
	char			*head;
	SCALED			sdn;
#endif
{
	if (sdn.val <= 0)
		return;

	(void)fprintf (fp, "%s ", head);
	printsdn (fp, sdn);

	return;
}

static void
#if	defined(__STDC__)
print_l (
	FILE *			fp,
	char *			head,
	char **			list
)
#else
print_l (fp, head, list)
	FILE			*fp;
	char			*head,
				**list;
#endif
{
	(void)fprintf (fp, "%s ", head);
	printlist_setup (0, 0, LP_SEP, 0);
	printlist (fp, list);
	printlist_unsetup ();

	return;
}

static void
#if	defined(__STDC__)
print_str (
	FILE *			fp,
	char *			head,
	char *			str
)
#else
print_str (fp, head, str)
	FILE			*fp;
	char			*head,
				*str;
#endif
{
	if (!str || !*str)
		return;

	(void)fprintf (fp, "%s %s\n", head, str);

	return;
}
