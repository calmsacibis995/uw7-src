#ident	"@(#)mp.cmds:common/cmd/priocntl/fppriocntl.c	1.7"
/***************************************************************************
 * Command: FPpriocntl
 *
 * Inheritable Privileges: P_OWNER,P_RTIME,P_MACREAD,P_MACWRITE
 *       Fixed Privileges: None
 *
 * Notes: This file contains the class specific code implementing
 * 	  the fixed-priority priocntl sub-command.
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/procset.h>
#include	<sys/priocntl.h>
#include	<sys/fppriocntl.h>
#include	<sys/param.h>
#include	<sys/hrtcntl.h>
#include	<limits.h>
#include	<sys/dl.h>
#include	<errno.h>
#include	<priv.h>
#include	<sys/secsys.h>
#include	"priocntl.h"
#include	<pfmt.h>
#include	<locale.h>

#define FP_TQINF_STRING	"FP_TQINF"

static void	print_fpinfo(), print_fpprocs(), set_fpprocs(), exec_fpcmd();

static char usage[] =
	":1039:Usage:	priocntl -l\n\tpriocntl -d [-i idtype] [idlist]\n\tpriocntl -s -c FP [-p fppri] [-t tqntm [-r res]] [-i idtype] [idlist]\n\tpriocntl -e -c FP [-p fppri] [-t tqntm [-r res]] command [argument(s)]\n";

static char
	badclid[] = ":1040:Cannot get FP class ID, errno = %d\n",
	badquantum[] = ":1041:Invalid time quantum; time quantum must be positive\n",
	badrange[] = ":1042:Specified priority %d out of configured range\n",
	badres[] = ":1043:Invalid resolution; resolution must be between 1 and 1,000,000,000\n",
	badreset[] = ":1044:Cannot reset fixed priority parameters, errno = %d\n";
	
static char	cmdpath[256];

/*
 * Procedure: main - process options & call appropriate subroutines.
 */
main(argc, argv)
int	argc;
char	**argv;
{
	extern char	*optarg;
	extern int	optind;

	int		c;
	int		lflag, dflag, sflag, pflag, tflag, rflag, eflag, iflag;
	short		fppri;
	long		tqntm;
	long		res;
	char		*idtypnm;
	idtype_t	idtype;
	int		idargc;

	setlocale(LC_ALL, "");
	setcat("uxcore.abi");
	setlabel("UX:priocntl");
	
	strcpy(cmdpath, argv[0]);
	lflag = dflag = sflag = pflag = tflag = rflag = eflag = iflag = 0;
	while ((c = getopt(argc, argv, "ldsp:t:r:ec:i:")) != -1) {
		switch(c) {

		case 'l':
			lflag++;
			break;

		case 'd':
			dflag++;
			break;

		case 's':
			sflag++;
			break;

		case 'p':
			pflag++;
			fppri = (short)atoi(optarg);
			break;

		case 't':
			tflag++;
			tqntm = atol(optarg);
			break;

		case 'r':
			rflag++;
			res = atol(optarg);
			break;

		case 'e':
			eflag++;
			break;

		case 'c':
			if( strcmp(optarg,"RT") != 0 )	/* COMPATABILITY*/
				if (strcmp(optarg, "FP") != 0) {
				pfmt(stderr, MM_ERROR, ":1045:%s executed for %s class, %s is actually sub-command for FP class\n", cmdpath, optarg, cmdpath);
				exit(1);
			}
			break;

		case 'i':
			iflag++;
			idtypnm = optarg;
			break;

		case '?':
			pfmt(stderr, MM_ERROR, usage);
			exit(1);

		}
	}

	if (lflag) {
		if (dflag || sflag || pflag || tflag || rflag || eflag || iflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		print_fpinfo();
		exit(0);

	} else if (dflag) {
		if (lflag || sflag || pflag || tflag || rflag || eflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		print_fpprocs();
		exit(0);

	} else if (sflag) {
		if (lflag || dflag || eflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		if (iflag) {
			if (str2idtyp(idtypnm, &idtype) == -1) {
				pfmt(stderr, MM_ERROR, ":1024:Bad idtype %s\n",idtypnm);
				exit(1);
			}
		} else
			idtype = P_PID;

		if (pflag == 0)
			fppri = FP_NOCHANGE;

		if (tflag == 0)
			tqntm = FP_NOCHANGE;
		else if (tqntm < 1) {
			pfmt(stderr, MM_ERROR, badquantum);
			exit(1);
		}
		if (rflag == 0)
			res = 1000;

		if (optind < argc)
			idargc = argc - optind;
		else
			idargc = 0;

		set_fpprocs(idtype, idargc, &argv[optind], fppri, tqntm, res);
		exit(0);

	} else if (eflag) {
		if (lflag || dflag || sflag || iflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		if (pflag == 0)
			fppri = FP_NOCHANGE;

		if (tflag == 0)
			tqntm = FP_NOCHANGE;
		else if (tqntm < 1) {
			pfmt(stderr, MM_ERROR, badquantum);
			exit(1);
		}
		if (rflag == 0)
			res = 1000;

		exec_fpcmd(&argv[optind], fppri, tqntm, res);

	} else {
		pfmt(stderr, MM_ERROR, usage);
		exit(1);
	}
	/* NOTREACHED */
}


/*
 * Procedure:     print_fpinfo
 *
 * Notes: Print our class name and the maximum configured fixed-priority
 *
 * Restrictions: priocntl(2): <none>
 */
static void
print_fpinfo()
{
	pcinfo_t	pcinfo;

	strcpy(pcinfo.pc_clname, "FP");

	pfmt(stdout, MM_NOSTD, ":1046:FP (Fixed Priority)\n");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, ":1047:\tCannot get maximum configured FP priority\n");
		exit(1);
	}
	pfmt(stdout, MM_NOSTD, ":1048:\tMaximum Configured FP Priority: %d\n", 
		((fpinfo_t *)pcinfo.pc_clinfo)->fp_maxpri);
}



/*
 * Procedure:     print_fpprocs
 * 
 * Notes: Read a list of pids from stdin and print the fixed-priority
 *	  priority and time quantum (in millisecond resolution) for
 *	  each of the corresponding processes.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
print_fpprocs()
{
	pid_t		pidlist[NPIDS];
	int		numread;
	int		i;
	id_t		fpcid;
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	ulong		tqsecs;
	long		tqnsecs;

	numread = fread(pidlist, sizeof(pid_t), NPIDS, stdin);

	pfmt(stdout, MM_NOSTD, ":1049:FIXED PRIORITY PROCESSES:\n    PID    FPPRI       TQNTM\n");

	strcpy(pcinfo.pc_clname, "FP");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid, errno);
		exit(1);
	}
	fpcid = pcinfo.pc_cid;

	if (numread <= 0) {
		pfmt(stderr, MM_ERROR, ":1050:No pids on input\n");
		exit(1);
	}
	pcparms.pc_cid = PC_CLNULL;
	for (i = 0; i < numread; i++) {
		printf("%7ld", pidlist[i]);
		if (priocntl(P_PID, pidlist[i], PC_GETPARMS, &pcparms) == -1) {
			pfmt(stdout, MM_WARNING, ":1051:\tCannot get fixed priority parameters\n");
			continue;
		}

		if (pcparms.pc_cid == fpcid) {
			printf("   %5d", ((fpparms_t *)pcparms.pc_clparms)->fp_pri);
			tqsecs = ((fpparms_t *)pcparms.pc_clparms)->fp_tqsecs;
			tqnsecs = ((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs;
			if (tqsecs > LONG_MAX / 1000 - 1)
			pfmt(stdout, MM_WARNING, ":1052:    Time quantum too large to express in millisecond resolution.\n");
			else {
				if (tqnsecs == FP_TQINF)
					printf(" %11s\n", FP_TQINF_STRING);
				else
					printf(" %11ld\n", tqsecs * 1000 + tqnsecs / 1000000);
			}
		} else {

			/*
			 * Process from some class other than fixed priority.
			 * It has probably changed class while priocntl
			 * command was executing (otherwise we wouldn't
			 * have been passed its pid).  Print the little
			 * we know about it.
			 */
			pcinfo.pc_cid = pcparms.pc_cid;
			if (priocntl(0, 0, PC_GETCLINFO, &pcinfo) != -1)
			pfmt(stdout, MM_WARNING, ":1053:%ld\tChanged to class %s while priocntl command executing\n", pidlist[i], pcinfo.pc_clname);

		}
	}
}



/*
 * Procedure:     set_fpprocs
 *
 * Notes: Set all processes in the set specified by idtype/idargv to
 *	  fixed priority (if they aren't already fixed priority) and set their
 *	  fixed-priority and quantum to those specified by fppri
 *	  and tqntm/res.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
set_fpprocs(idtype, idargc, idargv, fppri, tqntm, res)
idtype_t	idtype;
int		idargc;
char		**idargv;
short		fppri;
long		tqntm;
long		res;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxfppri;
	hrtime_t	hrtime;
	char		idtypnm[12];
	int		i;
	id_t		fpcid;


	/*
	 * Get the fixed priority class ID and max configured FP priority.
	 */
	strcpy(pcinfo.pc_clname, "FP");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid, errno);
		exit(1);
	}
	maxfppri = ((fpinfo_t *)pcinfo.pc_clinfo)->fp_maxpri;

	/*
	 * Validate the fppri and res arguments.
	 */
	if ((fppri > maxfppri || fppri < 0) && fppri != FP_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badrange, fppri);
		exit(1);
	}
	if (res > NANOSEC || res < 1) {
		pfmt(stderr, MM_ERROR, badres);
		exit(1);
	}
	pcparms.pc_cid = pcinfo.pc_cid;
	((fpparms_t *)pcparms.pc_clparms)->fp_pri = fppri;
	if (tqntm == FP_NOCHANGE) {
		((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs = FP_NOCHANGE;
	} else {
		hrtime.hrt_secs = 0;
		hrtime.hrt_rem = tqntm;
		hrtime.hrt_res = res;
		if (nanores(&hrtime) == -1) {
			pfmt(stderr, MM_ERROR, ":1054:Cannot convert resolution.\n");
			exit(1);
		}
		((fpparms_t *)pcparms.pc_clparms)->fp_tqsecs = hrtime.hrt_secs;
		((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs = hrtime.hrt_rem;
	}

	if (idtype == P_ALL) {
		if (priocntl(P_ALL, 0, PC_SETPARMS, &pcparms) == -1) {
			if (errno == EPERM)
				pfmt(stderr, MM_ERROR, ":1055:Permissions error encountered on one or more processes.\n");
			else {
				pfmt(stderr, MM_ERROR, badreset, errno);
				exit(1);
			}
		}
	} else if (idargc == 0) {
		if (priocntl(idtype, P_MYID, PC_SETPARMS, &pcparms) == -1) {
			if (errno == EPERM) {
				(void)idtyp2str(idtype, idtypnm);
				pfmt(stderr, MM_ERROR, ":1056:Permissions error encountered on current %s.\n", idtypnm);
			} else {
				pfmt(stderr, MM_ERROR, badreset, errno);
				exit(1);
			}
		}
	} else {
		(void)idtyp2str(idtype, idtypnm);
		for (i = 0; i < idargc; i++) {
			if ( idtype == P_CID ) {
				fpcid = clname2cid(idargv[i]);
				if (priocntl(idtype, fpcid, PC_SETPARMS, &pcparms) == -1) {
					if (errno == EPERM)
						pfmt(stderr, MM_ERROR, ":1057:Permissions error encountered on %s %s.\n", idtypnm, idargv[i]);
					else {
						pfmt(stderr, MM_ERROR, badreset, errno);
						exit(1);
					}
				}
			} else if (priocntl(idtype, (id_t)atol(idargv[i]),
			    PC_SETPARMS, &pcparms) == -1) {
				if (errno == EPERM)
					pfmt(stderr, MM_ERROR, ":1057:Permissions error encountered on %s %s.\n", idtypnm, idargv[i]);
				else {
					pfmt(stderr, MM_ERROR, badreset, errno);
					exit(1);
				}
			}
		}
	}
}


/*
 * Procedure:     exec_fpcmd
 *
 * Notes: Execute the command pointed to by cmdargv as a fixed-priority
 *	  process with fixed  priority fppri and quantum tqntm/res.
 *
 * Restrictions:  priocntl(2): <none>
 *                execvp(2): P_MACREAD
 */
static void
exec_fpcmd(cmdargv, fppri, tqntm, res)
char	**cmdargv;
short	fppri;
long	tqntm;
long	res;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxfppri;
	hrtime_t	hrtime;
	uid_t		priv_id;	/* hold privileged ID if any */
	
	/*
	 * Get the fixed priority class ID and max configured FP priority.
	 */
	strcpy(pcinfo.pc_clname, "FP");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid, errno);
		exit(1);
	}
	maxfppri = ((fpinfo_t *)pcinfo.pc_clinfo)->fp_maxpri;

	if ((fppri > maxfppri || fppri < 0) && fppri != FP_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badrange, fppri);
		exit(1);
	}
	if (res > NANOSEC || res < 1) {
		pfmt(stderr, MM_ERROR, badres);
		exit(1);
	}
	pcparms.pc_cid = pcinfo.pc_cid;
	((fpparms_t *)pcparms.pc_clparms)->fp_pri = fppri;
	if (tqntm == FP_NOCHANGE) {
		((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs = FP_NOCHANGE;
	} else {
		hrtime.hrt_secs = 0;
		hrtime.hrt_rem = tqntm;
		hrtime.hrt_res = res;
		if (nanores(&hrtime) == -1) {
			pfmt(stderr, MM_ERROR, ":1054:Cannot convert resolution.\n");
			exit(1);
		}
		((fpparms_t *)pcparms.pc_clparms)->fp_tqsecs=hrtime.hrt_secs;
		((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs=hrtime.hrt_rem;
	}
	
	if (priocntl(P_PID, P_MYID, PC_SETPARMS, &pcparms) == -1) {
		pfmt(stderr, MM_ERROR, badreset, errno);
		exit(1);
	}

	/*
	 * Check if the current system is privileged ID based or 
	 * not by calling secsys().
	 *
	 * If this is NOT a privileged ID based system i.e (this is lpm), then
	 * clear maximum privileges before executing specified
	 * command.  If the specified command needs privilege, it
	 * should use tfadmin(1M).
	 *
	 * If this IS a privileged ID based system, then
	 * check if the current UID is equal to the privileged ID.
	 * If current UID is not equal to the privileged ID, then 
	 * also clear all privileges.
	 */
	
	{
		int privid;

		privid = secsys(ES_PRVID, 0);

		if (privid < 0 ||  privid != getuid()) 
			procprivl(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);
	}

	(void) execvp(cmdargv[0], cmdargv);
	pfmt(stderr, MM_ERROR, ":1058:Cannot execute %s, errno = %d\n", cmdargv[0], errno);
	exit(1);
}

/*
**	Convert interval expressed in htp->hrt_res to nanosecond.
**
**	Calculate: (interval * NANOSEC) / htp->hrt_res  rounding up
**
**	Note:	All args are assumed to be positive.  If
**	the last divide results in something bigger than
**	a long, then -1 is returned instead.
*/

nanores(htp)
register hrtime_t *htp;
{
	register long  interval;
	dl_t		dint;
	dl_t		dto_res;
	dl_t		drem;
	dl_t		dfrom_res;
	dl_t		prod;
	dl_t		quot;
	register long	numerator;
	register long	result;
	long		temp;

	if (htp->hrt_res <= 0 || htp->hrt_rem < 0)
		return(-1);

	if (htp->hrt_rem >= htp->hrt_res) {
		htp->hrt_secs += htp->hrt_rem / htp->hrt_res;
		htp->hrt_rem = htp->hrt_rem % htp->hrt_res;
	}

	interval = htp->hrt_rem;
	if (interval == 0) {
		htp->hrt_res = NANOSEC;
		return(0);
	}

	/*	Try to do the calculations in single precision first
	**	(for speed).  If they overflow, use double precision.
	**	What we want to compute is:
	**
	**		(interval * NANOSEC) / hrt->hrt_res
	*/

	numerator = interval * NANOSEC;

	if (numerator / NANOSEC  ==  interval) {
			
		/*	The above multiply didn't give overflow since
		**	the division got back the original number.  Go
		**	ahead and compute the result.
		*/
	
		result = numerator / htp->hrt_res;
	
		/*
		**	For roundup, we increase the result by 1 if:
		**
		**		result * htp->hrt_res != numerator
		**
		**	because this tells us we truncated when calculating
		**	result above.
		**
		**	We also check for overflow when incrementing result
		**	although this is extremely rare.
		*/
	
		if (result * htp->hrt_res != numerator) {
			temp = result + 1;
			if (temp - 1 == result)
				result++;
			else
				return(-1);
		}
		htp->hrt_res = NANOSEC;
		htp->hrt_rem = result;
		return(0);
	}
	
	/*	We would get overflow doing the calculation is
	**	single precision so do it the slow but careful way.
	**
	**	Compute the interval times the resolution we are
	**	going to.
	*/

	dint.dl_hop	= 0;
	dint.dl_lop	= interval;
	dto_res.dl_hop	= 0;
	dto_res.dl_lop	= NANOSEC;
	prod		= lmul(dint, dto_res);

	/*	for roundup we use:
	**
	**		((interval * NANOSEC) + htp->hrt_res - 1) / htp->hrt_res
	**
	** 	This is a different but equivalent way of rounding.
	*/

	drem.dl_hop = 0;
	drem.dl_lop = htp->hrt_res - 1;
	prod	    = ladd(prod, drem);

	dfrom_res.dl_hop = 0;
	dfrom_res.dl_lop = htp->hrt_res;
	quot		 = ldivide(prod, dfrom_res);

	/*	If the quotient won't fit in a long, then we have
	**	overflow.  Otherwise, return the result.
	*/

	if (quot.dl_hop != 0) {
		return(-1);
	} else {
		htp->hrt_res = NANOSEC;
		htp->hrt_rem = quot.dl_lop;
		return(0);
	}
}
