/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mp.cmds:common/cmd/priocntl/tspriocntl.c	1.1"
#ident  "$Header$"

/***************************************************************************
 * Command: TSpriocntl
 *
 * Inheritable Privileges: P_OWNER,P_TSHAR,P_MACREAD,P_MACWRITE
 *       Fixed Privileges: None
 *
 * Notes: This file contains the class specific code implementing the
 * 	  time-sharing priocntl sub-command.
 ***************************************************************************/

#include	<stdio.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/procset.h>
#include	<sys/priocntl.h>
#include	<sys/tspriocntl.h>
#include	<errno.h>
#include	<priv.h>
#include	<sys/secsys.h>
#include	"priocntl.h"
#include	<pfmt.h>
#include	<locale.h>

#define BASENMSZ	16

static void	print_tsinfo(), print_tsprocs(), set_tsprocs(), exec_tscmd();

static char usage[] =
	":1059:Usage:	priocntl -l\n\tpriocntl -d [-d idtype] [idlist]\n\tpriocntl -s [-c TS] [-m tsuprilim] [-p tsupri] [-i idtype] [idlist]\n\tpriocntl -e [-c TS] [-m tsuprilim] [-p tsupri] command [argument(s)]\n";

static char
	badclid[] = ":1060:Cannot get TS class ID, errno = %d\n",
	badlimit[] = ":1061:Specified user priority limit %d out of configured range\n",
	badrange[] = ":1062:Specified user priority %d out of configured range\n",
	badreset[] = ":1063:Cannot reset time sharing parameters, errno = %d\n";

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
	int		lflag, dflag, sflag, mflag, pflag, eflag, iflag;
	short		tsuprilim;
	short		tsupri;
	char		*idtypnm;
	idtype_t	idtype;
	int		idargc;

	setlocale(LC_ALL, "");
	setcat("uxcore.abi");
	setlabel("UX:priocntl");
	
	strcpy(cmdpath, argv[0]);
	lflag = dflag = sflag = mflag = pflag = eflag = iflag = 0;
	while ((c = getopt(argc, argv, "ldsm:p:ec:i:")) != -1) {
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

		case 'm':
			mflag++;
			tsuprilim = (short)atoi(optarg);
			break;

		case 'p':
			pflag++;
			tsupri = (short)atoi(optarg);
			break;

		case 'e':
			eflag++;
			break;

		case 'c':
			if (strcmp(optarg, "TS") != 0) {
				pfmt(stderr, MM_ERROR, ":1064:%s executed for %s class, %s is actually sub-command for TS class\n", cmdpath, optarg, cmdpath);
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
		if (dflag || sflag || mflag || pflag || eflag || iflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		print_tsinfo();
		exit(0);

	} else if (dflag) {
		if (lflag || sflag || mflag || pflag || eflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		print_tsprocs();
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

		if (mflag == 0)
			tsuprilim = TS_NOCHANGE;

		if (pflag == 0)
			tsupri = TS_NOCHANGE;

		if (optind < argc)
			idargc = argc - optind;
		else
			idargc = 0;

		set_tsprocs(idtype, idargc, &argv[optind], tsuprilim, tsupri);
		exit(0);

	} else if (eflag) {
		if (lflag || dflag || sflag || iflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		if (mflag == 0)
			tsuprilim = TS_NOCHANGE;

		if (pflag == 0)
			tsupri = TS_NOCHANGE;

		exec_tscmd(&argv[optind], tsuprilim, tsupri);

	} else {
		pfmt(stderr, MM_ERROR, usage);
		exit(1);
	}
	/* NOTREACHED */
}



/*
 * Procedure:     print_tsinfo
 *
 * Notes: Print our class name and the configured user priority range.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
print_tsinfo()
{
	pcinfo_t	pcinfo;

	strcpy(pcinfo.pc_clname, "TS");

	pfmt(stdout, MM_NOSTD, ":1065:TS (Time Sharing)\n");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, ":1066:\tCannot get configured TS user priority range\n");
		exit(1);
	}
	pfmt(stdout, MM_NOSTD, ":1067:\tConfigured TS User Priority Range: -%d through %d\n",
	    ((tsinfo_t *)pcinfo.pc_clinfo)->ts_maxupri,
	    ((tsinfo_t *)pcinfo.pc_clinfo)->ts_maxupri);
}

/*
 * Procedure:     print_tsprocs
 *
 * Notes: Read a list of pids from stdin and print the user priority
 *	  and user priority limit for each of the corresponding
 *	  processes.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
print_tsprocs()
{
	pid_t		pidlist[NPIDS];
	int		numread;
	int		i;
	id_t		tscid;
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;

	numread = fread(pidlist, sizeof(pid_t), NPIDS, stdin);

	pfmt(stdout, MM_NOSTD,":1068:TIME SHARING PROCESSES:\n    PID    TSUPRILIM    TSUPRI\n");

	strcpy(pcinfo.pc_clname, "TS");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid);
		exit(1);
	}
	tscid = pcinfo.pc_cid;

	if (numread <= 0) {
		pfmt(stderr, MM_ERROR, ":1050:No pids on input\n");
		exit(1);
	}
	pcparms.pc_cid = PC_CLNULL;
	
	for (i = 0; i < numread; i++) {
		printf("%7ld", pidlist[i]);
		if (priocntl(P_PID, pidlist[i], PC_GETPARMS, &pcparms) == -1) {
			pfmt(stdout, MM_WARNING, ":1069:\tCannot get TS user priority\n");
			continue;
		}

		if (pcparms.pc_cid == tscid) {
			printf("    %5d       %5d\n",
			    ((tsparms_t *)pcparms.pc_clparms)->ts_uprilim,
			    ((tsparms_t *)pcparms.pc_clparms)->ts_upri);
		} else {

			/*
			 * Process from some class other than time sharing.
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
 * Procedure:	set_tsprocs
 *
 * Notes: Set all processes in the set specified by idtype/idargv to
 *	  time-sharing (if they aren't already time-sharing) and set
 *	  their user priority limit and user priority to those
 *	  specified by tsuprilim and tsupri. 
 *
 * Restrictions: priocntl(2): <none>
 */
static void
set_tsprocs(idtype, idargc, idargv, tsuprilim, tsupri)
idtype_t	idtype;
int		idargc;
char		**idargv;
short		tsuprilim;
short		tsupri;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxupri;
	char		idtypnm[12];
	int		i;
	id_t		tscid;

	/*
	 * If both user priority and limit have been defaulted then they
	 * need to be changed to 0 for later priocntl system calls.
	 */
	if (tsuprilim == TS_NOCHANGE && tsupri == TS_NOCHANGE)
		tsuprilim = tsupri = 0;

	/*
	 * Get the time sharing class ID and max configured user priority.
	 */
	strcpy(pcinfo.pc_clname, "TS");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid, errno);
		exit(1);
	}
	maxupri = ((tsinfo_t *)pcinfo.pc_clinfo)->ts_maxupri;

	/*
	 * Validate the tsuprilim and tsupri arguments.
	 */
	if ((tsuprilim > maxupri || tsuprilim < -maxupri) &&
	    tsuprilim != TS_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badlimit,  tsuprilim);
		exit(1);
	}
	if ((tsupri > maxupri || tsupri < -maxupri) && tsupri != TS_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badrange,  tsupri);
		exit(1);
	}
	pcparms.pc_cid = pcinfo.pc_cid;
	((tsparms_t *)pcparms.pc_clparms)->ts_uprilim = tsuprilim;
	((tsparms_t *)pcparms.pc_clparms)->ts_upri = tsupri;

	if (idtype == P_ALL) {
		if (priocntl(P_ALL, 0, PC_SETPARMS, &pcparms) == -1) {
			if (errno == EPERM)
				pfmt(stderr, MM_ERROR, ":1055:Permissions error encountered on one or more processes.\n");
			else {
				pfmt(stderr, MM_ERROR, badreset,  errno);
				exit(1);
			}
		}
	} else if (idargc == 0) {
		if (priocntl(idtype, P_MYID, PC_SETPARMS, &pcparms) == -1) {
			if (errno == EPERM) {
				(void)idtyp2str(idtype, idtypnm);
				pfmt(stderr, MM_ERROR, ":1056:Permissions error encountered on current %s.\n", idtypnm);
			} else {
				pfmt(stderr, MM_ERROR, badreset,  errno);
				exit(1);
			}
		}
	} else {
		(void)idtyp2str(idtype, idtypnm);
		for (i = 0; i < idargc; i++) {
			if ( idtype == P_CID ) {
				tscid = clname2cid(idargv[i]);
				if (priocntl(idtype, tscid, PC_SETPARMS, &pcparms) == -1) {
					if (errno == EPERM)
						pfmt(stderr, MM_ERROR, ":1057:Permissions error encountered on %s %s.\n", idtypnm, idargv[i]);
					else {
						pfmt(stderr, MM_ERROR, badreset,  errno);
						exit(1);
					}
				}
			} else if (priocntl(idtype, (id_t)atol(idargv[i]),
			    PC_SETPARMS, &pcparms) == -1) {
				if (errno == EPERM)
					pfmt(stderr, MM_ERROR, ":1057:Permissions error encountered on %s %s.\n", idtypnm, idargv[i]);
				else {
					pfmt(stderr, MM_ERROR, badreset,  errno);
					exit(1);
				}
			}
		}
	}
}


/*
 * Procedure:     exec_tscmd
 *
 * Notes: Execute the command pointed to by cmdargv as a time-sharing
 *	  process with the user priority limit given by tsuprilim and
 *	  user priority tsupri.
 *
 * Restrictions: priocntl(2): <none>
 *               execvp(2): P_MACREAD
 */
static void
exec_tscmd(cmdargv, tsuprilim, tsupri)
char	**cmdargv;
short	tsuprilim;
short	tsupri;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxupri;
	uid_t		priv_id;	/* hold privileged ID if any */
	
	/*
	 * If both user priority and limit have been defaulted then they
	 * need to be changed to 0 for later priocntl system calls.
	 */
	if (tsuprilim == TS_NOCHANGE && tsupri == TS_NOCHANGE)
		tsuprilim = tsupri = 0;

	/*
	 * Get the time sharing class ID and max configured user priority.
	 */
	strcpy(pcinfo.pc_clname, "TS");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid,  errno);
		exit(1);
	}
	maxupri = ((tsinfo_t *)pcinfo.pc_clinfo)->ts_maxupri;

	if ((tsuprilim > maxupri || tsuprilim < -maxupri) && 
	    tsuprilim != TS_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badlimit,  tsuprilim);
		exit(1);
	}
	if ((tsupri > maxupri || tsupri < -maxupri) && tsupri != TS_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badrange,  tsupri);
		exit(1);
	}
	pcparms.pc_cid = pcinfo.pc_cid;
	((tsparms_t *)pcparms.pc_clparms)->ts_uprilim = tsuprilim;
	((tsparms_t *)pcparms.pc_clparms)->ts_upri = tsupri;
	
	if (priocntl(P_PID, P_MYID, PC_SETPARMS, &pcparms) == -1) {
		pfmt(stderr, MM_ERROR, badreset,  errno);
		exit(1);
	}
	
	/*
	 * Check if the current system is privileged ID based or 
	 * not by calling secsys().
	 *
	 * If this is NOT a privileged ID based system, then
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

	(void)execvp(cmdargv[0], cmdargv);
	pfmt(stderr, MM_ERROR, ":1058:Cannot execute %s, errno = %d\n", 
		 cmdargv[0], errno);
	exit(1);
}
