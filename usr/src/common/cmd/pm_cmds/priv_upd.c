#ident	"@(#)priv_upd.c	1.2"

/***************************************************************************
 * Command:	priv_upd
 *
 * Notes:	priv_upd - update the "ctime" validity value and the "size"
 *			   value in the PDF.
 *
 *		Restores two elelments of the validity information in the
 *		Privilege Data File quickly and easily.  This is no guarantee
 *		that the remainder of the validity information is correct.
 *
 ***************************************************************************/

/* LINTLIBRARY */
#include <libcmd.h>

#define	OPDF	"/etc/security/tcb/oprivs"
#define	PRVTMP	"/etc/security/tcb/.temp_privs"

extern	int	access(),
		lvlfile();

extern	char	*strchr(),
		*strrchr();

static	void	updtcb();

struct	pdf	*getpfent();

static	int	upd_pfile();

static	char	*label;

main(argc, argv)
	int	argc;
	char	*argv[];
{
	char	*cp,
		*cmdnm,
		*lprefix = "UX:";

	(void) setlocale(LC_ALL, "");
	setcat("uxcore");
	/*
	 * get the simple name of the command for diagnostic messages.
	 */
	cmdnm = cp = argv[0];
	if ((cp = strrchr(cp, '/')) != NULL) {
		cmdnm = ++cp;
	}
	label = (char *)malloc(strlen(cmdnm) + strlen(lprefix) + 1);
	(void) sprintf(label, "%s%s", lprefix, cmdnm);

	(void) setlabel(label);
	/*
	 * first check command syntax for correctness.
	 * if more than just command name present, exit
	 * without a message to avoid any complications.
	 */
	if (argc > 1) {
		exit(1);
	}
	/*
	 * If the ``/etc/security/tcb/privs''  file is not
	 * accessible, exit with  the appropriate exit code. 
	 */
	if (access(PDF, 0) != 0) {
		if (errno == EPERM) {
			pfmt(stderr, MM_ERROR | MM_NOGET, strerror(errno));
		}
		exit(1);
	}
	/*
	 * read the privilege data file updating the ``ctime''
	 * and ``size'' values for each file that is accessible.
	 */
	updtcb();

	exit(0);
	/* NOTREACHED */
}


/*
 * Procedure:	updtcb
 *
 * Notes:	This routine reads the ``/etc/security/tcb/privs'' file
 *		and updates the ``ctime'' and ``size'' values for each
 *		file that is accessible (via stat()).
 */
static	void
updtcb()
{
	struct	stat	buf,
			statb;
	struct	pdf	*pdf;
	register int	i, end_of_file = 0;
	extern int	errno,
			badent;		/* this is set in getpfent() */
	char		newbuf[FP_BSIZ];
	FILE		*tpfp;

	/* ignore all the signals */

	for (i = 1; i < NSIG; i++)
		(void) sigset(i, SIG_IGN);

 	/* Clear the errno. It is set because SIGKILL can not be ignored. */

	errno = 0;

	/*
	 * set umask to 0464 so file will be created with correct mode.
	 */
	(void) umask(~(S_IRUSR|S_IRGRP|S_IWGRP|S_IROTH));
	/*
	 * do unconditional unlink of temporary privilege
	 * data file.  Shouldn't exist anyway!!
	 */
	(void) unlink(PRVTMP);
	/*
	 * see if the PDF file already exists.  If not, exit!
	 */
	if (stat(PDF, &buf) < 0) {
		exit(1);
	}
	/*
	 * open the temporary file for writing.
	 * Should have access.
	 */
	if ((tpfp = fopen(PRVTMP, "w")) == NULL) {
		exit(1);
	}
	(void) setvbuf(tpfp, (char *)newbuf, _IOLBF, sizeof(newbuf));

	/*
	 * Read privilege data file (/etc/security/tcb/privs).
	 */

	while (!end_of_file) {
		errno = 0;
		if ((pdf = getpfent()) != NULL) {	/* read entry */
			if ((stat(pdf->pf_filep, &statb) < 0)) {
				putpfent(pdf, tpfp);
				continue;
			}
			/*
			 * replace the file size and ctime.
			 */
			pdf->pf_size = statb.st_size;
			pdf->pf_validity = statb.st_ctime;

			putpfent(pdf, tpfp);
		}
		else {			/* call to getpfent() returned NULL. */
			if (!badent) {	/* badent NOT set, must be EOF */
				end_of_file = 1;
			} else if (badent) {		/* BADENT */
				badent = 0;
			}
		}
	}	/* end of while loop */

	(void) endpfent();		/* close the privilege data file */

	(void) fclose(tpfp);

	/*
	 * set the level of the temporary file to whatever
	 * the level is for either the existing PDF or the
	 * directory in which this file will eventually reside.
	 */
	(void) lvlfile(PRVTMP, MAC_SET, &buf.st_level);

	/*
	 * Do a chown() on the new privilege data file.
	 */
	(void) chown(PRVTMP, buf.st_uid, buf.st_gid);

	/*
	 * rename the current PDF to the old PDF;
	 * rename the temporary file to the new PDF.
	 */
	if (upd_pfile(PDF, OPDF, PRVTMP)) {
		pfmt(stderr, MM_ERROR, "Privlege Data File not updated\n");
		exit(1);
	}
	return;
}


/*
 * Procedure:	upd_pfile
 *
 * Notes:	This routine performs all the necessary checks when moving
 *		the current privilege data file to the old privilege data
 *		file and renaming the temporary privilege data file to the
 *		current privilege data file.
 */
static	int 
upd_pfile(pfilep, opfilep, tpfilep)
	char *pfilep;		/* privilege data file */
	char *opfilep;		/* old privilege data file */
	char *tpfilep;		/* temporary privilege data file */
{
	/*
	 * First check to see if there was an existing privilege
	 * data file.
	 */
	if (access(pfilep, 0) == 0) {
		/* if so, remove old privilege data file */
		if (access(opfilep, 0) == 0) {
			if (unlink(opfilep) < 0) {
				(void) unlink(tpfilep);
				return 1;
			}
		}
		/* rename privilege data file to old privilege data file */
		if (rename(pfilep, opfilep) < 0) {
			(void) unlink(tpfilep);
			return 1;
		}
	}
	if (access(tpfilep, 0) == 0) {
		/* rename temporary privilege data file to privilege data file */
		if (rename(tpfilep, pfilep) < 0) {
			(void) unlink(tpfilep);
			if (unlink(pfilep) < 0) {
				if (link(opfilep, pfilep) < 0) { 
					return 1;
				}
			}
			return 1;
		}
	}
	return 0;
}
