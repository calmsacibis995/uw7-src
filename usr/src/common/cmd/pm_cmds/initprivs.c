/*		copyright	"%c%" 	*/

#ident	"@(#)initprivs.c	1.3"
/***************************************************************************
 * Command:			initprivs
 *
 * Inheritable Privileges:	P_SETSPRIV,P_SETUPRIV
 *       Fixed Privileges:	P_OWNER,P_AUDIT,P_AUDITWR,P_COMPAT,P_DACREAD,
 *				P_DACWRITE,P_DEV,P_FILESYS,P_MACREAD,
 *				P_MACWRITE,P_MOUNT,P_MULTIDIR,P_SETPLEVEL,
 *				P_SETFLEVEL,P_SETUID,P_SYSOPS,P_DRIVER,
 *				P_RTIME,P_MACUPGRADE,P_FSYSRANGE,P_PLOCK,
 *				P_TSHAR
 *
 * Notes:	initprivs - set the system privilege information.
 *
 *		Initializes the system with privilege information.  It reads
 *		the information from ``/etc/security/tcb/privs''.  Invalid
 *		entries are ignored.
 *
 *		initprivs must have the appropriate privilege otherwise
 *		permission is denied.
 *
 ***************************************************************************/

#include <libcmd.h>

#define	NOPERM	":64:Permission denied\n"
#define	CANTCLR	":706:Cannot clear file privileges on %s\n"
#define	CANTSET	":707:File ``%s'' fails validation: entry ignored\n"
#define	BADENT	":708:1 entry ignored in ``%s''\n"
#define	BADENTS	":709:%d entries ignored in ``%s''\n"

#define	NPRVF	        2
#define	PROBLEM		3
#define	REBOOT	        5

static	char	*label,
		*getval(),
		gettype();

extern	int	access(),
		privnum(),
		filepriv(),
		set_index();

extern	char	*strchr(),
		*strrchr();

extern	long	getcksum();

extern	setdef_t	*init_sdefs();

static	void	cant_set();

static	int	verbose = 0;

struct	pdf	*getpfent();

static	const	char	*fname[NPRVF] = {
	"/sbin/init",
	"/sbin/initprivs",
};

static	int	setvec(),
		inittcb(),
		nsets = 0,
		pdf_error = 0,	/* incremented for bad entry in priv file */
		val_cksum = 1;	/* default is to always validate the cksum */

static	ulong	objtyp = PS_FILE_OTYPE;
static	setdef_t	*sdefs;

/*
 * Procedure:	main
 *
 * Restrictions:
 *		secsys(2):	none
 *		defopen():	none
 *		access(2):	none
*/
main(argc, argv)
	int	argc;
	char	*argv[];
{
	char	*cp,
		*ptr,
		*cmdnm,
		*lprefix = "UX:";
	long	flags = 0;
	FILE	*def_fp;
	register int	ret = 0;

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
	 * get the privilege mechanism "flag" information
	*/
	(void) secsys(ES_PRVINFO, (caddr_t)&flags);

	/*
	 * open the ``privcmds'' default file (if it exists)
	 * and determine if the cksum should be validated.
	*/
	if ((def_fp = defopen(PRIV_DEF)) != NULL) {
		if ((ptr = defread(def_fp, "VAL_CKSUM")) != NULL) {
			if (*ptr) {
				if (!strcmp(ptr, "No")) {
					/*
					 * DON'T use the cksum value.
					*/
					val_cksum = 0;
				}
			}
		}
		(void) defclose(def_fp);
	}
	if (flags & PM_ULVLINIT) {
		/*
		 * If the ``/etc/security/tcb/privs''  file is not
		 * accessible, exit with  the appropriate exit code. 
		 * The exit code is then checked by the "initprivs"
		 * function in "/sbin/init" and the proper action
		 * is taken.
		*/
		if (access(PDF, 0) != 0)
			exit((flags & PM_UIDBASE) ? PROBLEM : REBOOT);
		/*
		 * read the privilege data file setting up the
		 * kernel privilege table.  Then exit.
		*/

		ret = inittcb(flags);
	}
	exit(ret);
	/* NOTREACHED */
}


/*
 * Procedure:	inittcb
 *
 * Restrictions:
 *		getpfent():	none
 *		secsys(2):	none
 *		filepriv(2):	none
 *		stat(2):	none
 *
 * Notes:	This routine reads the ``/etc/security/tcb/privs'' file.
 *		Most validity checking is done at this level although
 *		the ``filepriv'' system call would fail for ALL valid-
 *		ation checks that are not correct.
*/
static	int
inittcb(prv_flags)
	long	prv_flags;
{
	struct	stat	statb;
	struct	pdf	*pdf;
	priv_t	pvec[NPRIVS],
		*fbufp = &pvec[0];
	int	fcnt = 0;
	char	type,
		sd[PRVNAMSIZ],
		*setname = &sd[0];
	register int	i,
			failed = 0,
			had_privs = 0,
			end_of_file = 0;
	extern int	errno,
			badent;		/* this is set in getpfent() */

	/* ignore all the signals */

	for (i = 1; i < NSIG; i++)
		(void) sigset(i, SIG_IGN);

 	/* Clear the errno. It is set because SIGKILL can not be ignored. */

	errno = 0;

	/* If "stdout" is a tty, set the "verbose" flag */

	if (isatty(fileno(stdout)))
		verbose = 1;

	sdefs = init_sdefs(&nsets);

	if (!(prv_flags & PM_UIDBASE)) {
		pvec[0] = 0;
		/*
		 * Try to clear the privileges on all files in the
		 * fname array.
		*/
		for (i = 0; i < NPRVF; ++i) {
			if (filepriv(fname[i], PUTPRV, pvec, 0) == FAILURE) {
				if (errno == EPERM) {
					if (verbose) {
						(void) pfmt(stderr, MM_ERROR, NOPERM);
					}
					exit(1);
				}
				if (verbose) {
					(void) pfmt(stdout, MM_WARNING, CANTCLR, fname[i]);
				}
				errno = 0;
			}
		}
	}
	/*
	 * read privilege data file (/etc/security/tcb/privs).
	 *
	 * NOTE:	This might be a good place to clear the P_MACREAD
	 *		privilege but that could cause ``initprivs'' to
	 *		fail during system initialization if the MAC feature
	 *		is installed but not initialized yet.
	*/

	while (!end_of_file) {
		/*
		 * The following lines re-initialize all variables for
		 * each entry read in.
		*/
		for (i = 0; i < NPRIVS; ++i)
			pvec[i] = (priv_t)0;

		errno = had_privs = fcnt = 0;

		if ((pdf = getpfent()) != NULL) {	/* read entry */

			if (filepriv(pdf->pf_filep, CNTPRV, fbufp, 0) != 0)
				continue;

			/*
			 * Check the privileges field.
			*/
			if (*pdf->pf_privs) {
				while(*pdf->pf_privs) {
					if (*pdf->pf_privs == '%')
						++pdf->pf_privs;
					pdf->pf_privs = getval(pdf->pf_privs,
							setname);
					if (sdefs && set_index(sdefs,
						nsets, setname, objtyp) >= 0) {
						type = gettype(sdefs, setname, nsets);
						if ((setvec(type, pdf->pf_privs,
							fbufp, &fcnt)) == FAILURE) {
								++failed;
								break;
						}
						++had_privs;
					}
					if ((pdf->pf_privs = strchr(pdf->pf_privs, '%')) == NULL) {
						break;
					}
				}
				if (failed) {
					failed = 0;
					cant_set(pdf->pf_filep);
					continue;	/* bad privileges */
				}
			}
			else {
				cant_set(pdf->pf_filep);
				continue;	/* NULL privilege entry */
			}
			if (!had_privs) {
				continue;	/* no privileges at all */
			}

			/*
			 * check the file size and ctime.
			*/
			if ((stat(pdf->pf_filep, &statb) < 0))
				continue;

			if (pdf->pf_size != statb.st_size ||
			    pdf->pf_validity != statb.st_ctime) {
				cant_set(pdf->pf_filep);
				continue;		/* validity incorrect */
			}
			/*
			 * If this system uses the cksum and it's in the
			 * PDF file, check against it to see if it matches.
			*/
			if (val_cksum && pdf->pf_cksum >= 0) {
				if (getcksum(pdf->pf_filep) != pdf->pf_cksum) {
					cant_set(pdf->pf_filep);
					continue;	/* cksums don't match */
				}
			}
			/*
			 * try to set the privileges in the kernel.
			*/
			if (filepriv(pdf->pf_filep, PUTPRV, fbufp,
				fcnt) == FAILURE) {
				cant_set(pdf->pf_filep);	/* FAILED! */
			}
		}
		else {			/* call to getpfent() returned NULL. */
			if (!badent)		/* badent NOT set, must be EOF */
				end_of_file = 1;
			else if (badent) {		/* BADENT */
				cant_set(pdf->pf_filep);
				badent = 0;
			}
		}
	}	/* end of while loop */

	(void) endpfent();		/* close the privilege data file */

	if (pdf_error >= 1) {		/* if ANY entry was bad, report it */
		if (pdf_error == 1) {
			if (verbose) {
				(void) pfmt(stdout, MM_WARNING, BADENT, PDF);
			}
		}
		else {
			if (verbose) {
				(void) pfmt(stdout, MM_WARNING, BADENTS, pdf_error, PDF);
			}
		}
	}
	return prv_flags;
}


/*
 * Procedure:	setvec
 *
 * Notes:	This routine parses the comma separated list of privileges in
 *		the entry and sets up the privilege buffer to be passed to
 *		the ``filepriv'' system call.
*/
static	int
setvec(type, argp, fprivp, fcnt)
	char	type;
	char	*argp;
	priv_t	fprivp[];
	int	*fcnt;
{
	char	name[PRVNAMSIZ];
	register int	n = 0, j;

	j = *fcnt;

	/*
	 * parse the list.  Stop when an error is hit, a ":", or a "%"
	 * is found.
	*/
	while (*argp) {
		if (*argp == ',')
			++argp;			/* bump pointer and continue */
		if (*argp == '%' || *argp == ':')
			break;			/* end of parse */
		name[0] = '\0';			/* clear out "name" */
		argp = getval(argp, name);	/* copy one priv into "name" */
		if ((n = privnum(name)) < 0) {
			return FAILURE;		/* undefined privilege name */
		}
		if (n == P_ALLPRIVS) {		/* psuedo-privilege ALLPRIVS */
			for (n = 0; n < NPRIVS; ++n)
				fprivp[j++] = pm_prid(type, n);
		}
		else {
			fprivp[j++] = pm_prid(type, n);
		}
		if (j > NPRIVS)
			return FAILURE;		/* too many privs in entry */
	}

	*fcnt = j;				/* adjust the count */

	return SUCCESS;
}


/*
 * Procedure:	gettype
 *
 * Notes:	This routine returns a one character value indicating
 *		the "type" for this privilege set.
*/
static	char
gettype(sd, setn, num)
	setdef_t	*sd;
	char		*setn;
	int		num;
{
	register int	i = 0;

	for (i = 0; i < num; ++i, ++sd) {
		if (!(strcmp(sd->sd_name, setn)))
			return(sd->sd_mask >> 24);
	}
	return 0;
}


/*
 * Procedure:	getval
 *
 * Notes:	This function places into a specified destination characters
 *		which are delimited by either a ",", a "%", or 0.  It obtains
 *		the characters from a line of characters.
*/
static	char	*
getval(sourcep, destp)
	register char	*sourcep;
	register char	*destp;
{
	while (*sourcep != ',' && *sourcep != '%' && *sourcep != '\0')
		*destp++ = *sourcep++;
	*destp = 0;
	return sourcep;
}


/*
 * Procedure:	cant_set
 *
 * Notes:	This routine prints the message CANTSET and increments pdf_error.
*/
void
cant_set(fnamep)
	char	*fnamep;
{
	if (verbose) {
		(void) pfmt(stdout, MM_WARNING, CANTSET, fnamep);
	}
	++pdf_error;
}
