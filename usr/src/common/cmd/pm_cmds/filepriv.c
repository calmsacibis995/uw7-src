/*		copyright	"%c%" 	*/

#ident	"@(#)filepriv.c	1.3"

/*
 * Command: filepriv
 *
 * Inheritable Privileges:	None
 * Inheritable Autorizations:	None
 *
 *       Fixed Privileges:	None
 *       Fixed Authorizations:	None
 *
 * Notes:	Set, remove, or print privileges associated with files.
 *
 */

#include	<libcmd.h>

#define	PRVDIR	"/etc/security/tcb"

#define	SYNTAX	":1:Incorrect usage\n"
#define	BADOPTS	":695:Incompatible options specified\n"
#define	ARGREQ	"uxlibc:2:Option requires an argument -- %c\n"
#define	UNPRIV	":697:Undefined process privilege \"%s\"\n"
#define	NOSUPP	":701:\"%s\" set not supported by this privilege mechanism\n"
#define	USAGE	":702:Usage: filepriv [-f priv[,...]] [-i priv[,...]] file ...\n"
#define	BADPRV	":703:Cannot use \"%s\" as both fixed and inheritable privilege\n"

extern	int	link(),
		chown(),
		access(),
		unlink(),
		lvlfile(),
		lckprvf(),
		privnum(),
		filepriv(),
		procpriv(),
		ulckprvf(),
		getrlimit(),
		get_procmax(),
		set_index(),
		dofiles();

extern	char	*strrchr(),
		*privname(),
		*realpath();

extern	long	getcksum();

extern	setdef_t	*init_sdefs();

extern	unsigned long	umask();

static	void	usage(),
		setvec();

struct	pdf	*getpfent();

static	int	update(),
		dflag = 0,
		fflag = 0,
		iflag = 0,
		nsets = 0,
		nproc = 0,
		addpriv(),
		fcount = 0,
		gen_cksum = GEN_CKSUM,	/* default is to always generate a cksum */
		update_pfile(),
		setflg[PRVMAXSETS] = { 0 },
		washere[PRVMAXSETS] = { 0 };

static	char	*label,
		*getval(),
		procmask[NPRIVS],
		*privargp[PRVMAXSETS],
		prvmask[PRVMAXSETS][NPRIVS],
		privcase[PRVMAXSETS][ABUFSIZ];

static	priv_t	pvec[NPRIVS],
		*fbufp = &pvec[0];

static	setdef_t	*sdefs = (setdef_t *)0;

static	ulong	objtyp = PS_FILE_OTYPE;

main(argc, argv)
	int    argc;
	char **argv;
{
	extern	int	optind;
	extern	char	*optarg;
	char	*cp,
		*cmdnm,
		*setname,
		*prvs4file,
		*lprefix = "UX:";
	register int	psize = 0, i,
			opterr = 0, j,
			c, status = SUCCESS;
	register unsigned	nsize = 0;

	(void) setlocale(LC_ALL, "");
	setcat("uxcore");
	/*
	 * get the "simple" name of the command for use
	 * in diagnostic messages.
	*/
	cmdnm = cp = argv[0];
	if ((cp = strrchr(cp, '/')) != NULL) {
		cmdnm = ++cp;
	}
	label = (char *)malloc(strlen(cmdnm) + strlen(lprefix) + 1);
	(void) sprintf(label, "%s%s", lprefix, cmdnm);
	(void) setlabel(label);

	gen_cksum = get_gen_cksum();

	/* do initialization of required data structures */

	for (i = 0; i < NPRIVS; ++i)
		pvec[i] = (priv_t)0;

	for (i = 0; i < PRVMAXSETS; ++i) {
		privargp[i] = &privcase[i][0];
		for (j = 0; j < NPRIVS; ++j) {
			prvmask[i][j] = 0;
		}
	}

	sdefs = init_sdefs(&nsets);

	nproc = get_procmax(sdefs, nsets, procmask);

	/*
	 * Read in the argument list using the approved method
	 * of "getopt".  Only three arguments allowed (four,
	 * if you count NO arguments at all).
	 */
	while((c = getopt(argc, argv, "df:i:")) != EOF ) {
		switch(c) {
		case 'd':		/* delete keyletter */
			++dflag;
			continue;
		case 'f':		/* fixed privileges keyletter */
			++fflag;
			setname = "fixed";
			++setflg[c];
			/*
			 * Parse the argument list and set up for use
			 * later on.
			 */
			setvec(c, setname, optarg, fbufp, &fcount);
			if (sdefs && set_index(sdefs, nsets, setname, objtyp) < 0) {
				if (setflg[c] == 1) {
					(void) pfmt(stdout, MM_WARNING, NOSUPP, setname);
				}
			}
			continue;
		case 'i':		/* inheritable privileges keyletter */
			++iflag;
			setname = "inher";
			++setflg[c];
			/*
			 * Parse the argument list and set up for use
			 * later on.
			 */
			setvec(c, setname, optarg, fbufp, &fcount);
			if (sdefs && set_index(sdefs, nsets, setname, objtyp) < 0) {
				if (setflg[c] == 1) {
					(void) pfmt(stdout, MM_WARNING, NOSUPP, setname);
				}
			}
			continue;
		case '?':		/* Oops!!  Bad keyletter */
			opterr++;
			continue;
		}
		break;
	}
	if (opterr) {
		usage(BADSYN, MM_ACTION);	/* this routine exits! */
	}
	/*
	 * Strip off everything but the remaining files (if any)
	 * from the original "argv" vector and adjust "argc".
	 */
	argv = &argv[optind];
	argc -= optind;

	if (!argc) {			/* error, or no files specified */
		(void) pfmt(stderr, MM_ERROR, SYNTAX);
		usage(BADSYN, MM_ACTION);	/* this routine exits! */
	}
	/*
	 * The "-d" keyletter was specified with either the "-f"
	 * or "-i" keyletter.
	 */
	if ((fflag || iflag) && dflag) {
		(void) pfmt(stderr, MM_ERROR, BADOPTS);
		usage(BADSYN, MM_ACTION);
	}
	/*
	 * Start processing.  Do this case if either "-f" or
	 * "-i" keyletter (or both) was specified.
	 */
	if (fflag || iflag) {
		/* set up "prvs4file" ptr for use later */

		for (i = 0; i < PRVMAXSETS; ++i) {
			if ((psize = strlen(privargp[i]))) {
				nsize += psize;
			}
		}
		prvs4file = (char *)malloc(nsize + 1);
		prvs4file[0] = '\0';
		for (i = 0; i < PRVMAXSETS; ++i) {
			if (strlen(privargp[i])) {
				(void) strcat(prvs4file, privargp[i]);
			}
		}
		if (nproc) {
			status = dofiles(SET, argc, argv, fbufp, fcount, prvs4file, sdefs, nsets, gen_cksum);
		}
	}
	/*
	 * Do this case if the "-d" keyletter was specified.
	 */
	else if (dflag) {
		status = dofiles(RMV, argc, argv, fbufp, fcount, prvs4file, sdefs, nsets, gen_cksum);
	}
	/*
	 * This case is for printing the file privileges (no options).
	 */
	else {
		status = dofiles(PRT, argc, argv, fbufp, NPRIVS, prvs4file, sdefs, nsets, gen_cksum);
	}

	exit(status);
	/* NOTREACHED */
}


/*
 * Procedure:	usage
 *
 * Notes:	This routine prints out the usage message and then exits
 *		with the value passed as an argument.
 */
void
usage(ecode, action)
	int	ecode,
		action;
{
	int	i, spaces = 16;		/* 16 is the minimum number of spaces */
	char	*space = " ",		/* for header on 2nd  line of message */
		buffer[BUFSIZ],
		*p = &buffer[0];

	spaces += ((action == MM_ACTION) ? 1 : 0) + (int) strlen(label);

	/*
	 * USAGE is the first line of the usage message (including the
	 * newline).  Prepare for the second line by padding with spaces,
	 * so it lines up nicely under the first line.
	 */

	sprintf(p, "%s%*s", USAGE, spaces, "");

	/*
	 * Add on the remainder of the message after aligning it correctly.
	 */
	(void) strcat(p, gettxt(":705", "filepriv -d file ...\n"));

	(void) pfmt(stderr, action, p);

	exit(ecode);
}

/*
 * Procedure:	setvec
 *
 * Notes:	This routine scans the "argp" argument.  This should be a comma
 *		separated list of privileges.   It sets up the buffer passed to
 *		"filepriv".   It also adjusts the count for the buffer which is
 *		the number of elements in the array.
 */
void
setvec(type, setname, argp, fprivp, fcnt)
	char	type;
	char	*setname;
	char	*argp;
	priv_t	fprivp[];
	int	*fcnt;
{
	char	tmp[ABUFSIZ],
		name[BUFSIZ],
		pbuf[BUFSIZ],
		tmask[NPRIVS];

	int	i, j;
	char	*pbp = &pbuf[0];
	register int	n = 0, comma = 0,
			had_name = 0, didsome = 0;

	tmp[0] = '\0';

	j = *fcnt;

	/*
	 * Initialize the tmask variable to all 0's.
	 */
	for (i = 0; i < NPRIVS; ++i)
		tmask[i] = 0;

	/*
	 * Scan the argument list.
	 */
	while (*argp) {
		while (*argp == ',')
			++argp;			/* skip over this character */
		name[0] = '\0';			/* clear out "name" */
		if (*argp) {
			/*
			 * get the name of the privilege and place in "name"
			*/
			argp = getval(argp, name);
			/*
			 * Check to see if this is a "known" privilege.
			 */
			if ((n = privnum(name)) < 0){
				/*
			 	 * It wasn't a "known" privilege, so print
			 	 * a message.
				 */
				(void) pfmt(stderr, MM_ERROR, UNPRIV, name);
				exit(1);
			}
			++had_name;
			if (n == P_ALLPRIVS) {
				if (washere[type]++)
					continue;
				/*
				 * This is the pseudo-privilege "allprivs"
				 * so set up prvmask[type] appropriately.
				 */
				for (i = 0; i < NPRIVS; ++i) {
					if (addpriv(i, type) == FAILURE) {
						/*
						 * "addpriv" failed because this
						 * priv existed in more than one						 * vector.
						 */
						(void) pfmt(stderr, MM_ERROR, BADPRV,
								privname(pbp, i));
						exit(1);
					}
					tmask[i] = 1;
				}
				if (nproc == NPRIVS) {	/* process had all privileges */
					(void) strcat(tmp, "allprivs");
					didsome = 1;
					break;		/* exit from "while" */
				}
				else {
					/*
					 * Only set what's in "procmask" and
					 * clean up "prvmask[type]".
					 */
					for (i = 0; i < NPRIVS; ++i) {
						if (procmask[i]) {
							if (comma) {
								comma = 0;
								(void) strcat(tmp, ",");
							}
							(void) strcat(tmp, privname(pbp, i));
							tmask[i] = comma = 1;
							didsome = 1;
						}
						else {
							tmask[i] = prvmask[type][i] = 0;
						}
					}
				}
			}
			else {
				if (prvmask[type][n]) {	/* name already set */
					continue;
				}
				if (addpriv(n, type) == FAILURE) {
					/*
					 * "addpriv" failed because this priv
					 * existed in more than one vector.
					 */
					(void) pfmt(stderr, MM_ERROR, BADPRV, name);
					exit(1);
				}
				if (procmask[n]) {	/* priv is in process max */
					if (comma) {
						comma = 0;
						(void) strcat(tmp, ",");
					}
					(void) strcat(tmp, name);
					tmask[n] = comma = 1;
					didsome = 1;
				}
				else {		/* priv was'nt in process max */
					tmask[n] = prvmask[type][n] = 0;
				}
			}
		}
	}	/* end of "while" loop */

	/*
	 * Now do the actual setting of the buffers for the call to filepriv.
	 */
	if (didsome) {
		for (i = 0; i < NPRIVS; ++i) {
			if (tmask[i]) {
				fprivp[j++] = pm_prid(type, i);
			}
		}

		/* adjust the count for "filepriv" */
	
		*fcnt = j;
	
		if (setflg[type] > 1) {
			(void) strcat(privargp[type], ",");
		}
		else {
			(void) strcat(privargp[type], "%");
			(void) strcat(privargp[type], setname);
			(void) strcat(privargp[type], ",");
		}

		(void) strcat(privargp[type], tmp);

	}	/* end of "didsome" */
	else if (!had_name) {
		(void) pfmt(stderr, MM_ERROR, ARGREQ, type);
		usage(BADSYN, MM_ACTION);
	}
}


/*
 * Procedure:	addpriv
 *
 * Notes:	This routine checks all privilege "type"s (except the "type"
 *		passed) to determine if the privilege is already in another
 *		privilege set.  If it is, it's an ERROR.
 *
 *		Otherwise, just set a bit in the named "type" set and return.
 */
static	int
addpriv(pos, type)
	int	pos;
	char	type;
{
	register int	i;

	for (i = 0; i < PRVMAXSETS; ++i) {
		if (i == type)
			continue;
		if (prvmask[i][pos])	/* priv exists in more than one set */
			return FAILURE;
	}

	prvmask[type][pos] = 1;

	return SUCCESS;
}

/*
 * Procedure:	getval
 *
 * Notes:	This function places into a specified destination characters
 *		which are delimited by either a "," or 0.  It obtains the
 *		characters from a line of characters.
 */
static	char	*
getval(sourcep, destp)
	register char	*sourcep;
	register char	*destp;
{
	while (*sourcep != ',' && *sourcep != '\0')
		*destp++ = *sourcep++;
	*destp = 0;
	return sourcep;
}
