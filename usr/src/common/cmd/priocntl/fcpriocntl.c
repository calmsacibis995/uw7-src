/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mp.cmds:common/cmd/priocntl/fcpriocntl.c	1.1"
#ident  "$Header$"

/***************************************************************************
 * Command: FCpriocntl
 *
 * Inheritable Privileges: P_OWNER,P_FCHAR,P_MACREAD,P_MACWRITE
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
#include	<sys/fcpriocntl.h>
#include	<errno.h>
#include	<priv.h>
#include	<sys/secsys.h>
#include	"priocntl.h"
#include	<pfmt.h>
#include	<locale.h>

#define BASENMSZ	16

static void	print_fcinfo(), print_fcprocs(), set_fcprocs(), exec_fccmd();

static char usage[] =
	":1059:Usage:	priocntl -l\n\tpriocntl -d [-d idtype] [idlist]\n\tpriocntl -s [-c FC] [-m fcuprilim] [-p fcupri] [-i idtype] [idlist]\n\tpriocntl -e [-c FC] [-m fcuprilim] [-p fcupri] command [argument(s)]\n";

static char
	badclid[] = ":1060:Cannot get FC class ID, errno = %d\n",
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
	short		fcuprilim;
	short		fcupri;
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
			fcuprilim = (short)atoi(optarg);
			break;

		case 'p':
			pflag++;
			fcupri = (short)atoi(optarg);
			break;

		case 'e':
			eflag++;
			break;

		case 'c':
			if (strcmp(optarg, "FC") != 0) {
				pfmt(stderr, MM_ERROR, ":1064:%s executed for %s class, %s is actually sub-command for FC class\n", cmdpath, optarg, cmdpath);
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
		print_fcinfo();
		exit(0);

	} else if (dflag) {
		if (lflag || sflag || mflag || pflag || eflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		print_fcprocs();
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
			fcuprilim = FC_NOCHANGE;

		if (pflag == 0)
			fcupri = FC_NOCHANGE;

		if (optind < argc)
			idargc = argc - optind;
		else
			idargc = 0;

		set_fcprocs(idtype, idargc, &argv[optind], fcuprilim, fcupri);
		exit(0);

	} else if (eflag) {
		if (lflag || dflag || sflag || iflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		if (mflag == 0)
			fcuprilim = FC_NOCHANGE;

		if (pflag == 0)
			fcupri = FC_NOCHANGE;

		exec_fccmd(&argv[optind], fcuprilim, fcupri);

	} else {
		pfmt(stderr, MM_ERROR, usage);
		exit(1);
	}
	/* NOTREACHED */
}



/*
 * Procedure:     print_fcinfo
 *
 * Notes: Print our class name and the configured user priority range.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
print_fcinfo()
{
	pcinfo_t	pcinfo;

	strcpy(pcinfo.pc_clname, "FC");

	pfmt(stdout, MM_NOSTD, ":1065:FC (Time Sharing)\n");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, ":1066:\tCannot get configured FC user priority range\n");
		exit(1);
	}
	pfmt(stdout, MM_NOSTD, ":1067:\tConfigured FC User Priority Range: -%d through %d\n",
	    ((fcinfo_t *)pcinfo.pc_clinfo)->fc_maxupri,
	    ((fcinfo_t *)pcinfo.pc_clinfo)->fc_maxupri);
}


/*
 * Procedure:     print_fcprocs
 *
 * Notes: Read a list of pids from stdin and print the user priority
 *	  and user priority limit for each of the corresponding
 *	  processes.
 *
 * Restrictions: priocntl(2): <none>
 */
static void
print_fcprocs()
{
	pid_t		pidlist[NPIDS];
	int		numread;
	int		i;
	id_t		fccid;
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;

	numread = fread(pidlist, sizeof(pid_t), NPIDS, stdin);

	pfmt(stdout, MM_NOSTD,
":1068:FIXED CLASS PROCESSES:\n PID FCUPRILIM FCUPRI  TL    CPUP    UMDP \n");

	strcpy(pcinfo.pc_clname, "FC");

	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid);
		exit(1);
	}
	fccid = pcinfo.pc_cid;

	if (numread <= 0) {
		pfmt(stderr, MM_ERROR, ":1050:No pids on input\n");
		exit(1);
	}
	pcparms.pc_cid = PC_CLNULL;
	
	for (i = 0; i < numread; i++) {
		printf("%5ld", pidlist[i]);
		((fcparms_t *)pcparms.pc_clparms)->fc_uprilim = 0;
		((fcparms_t *)pcparms.pc_clparms)->fc_upri = 0;
		((fcparms_t *)pcparms.pc_clparms)->fc_timeleft = 0;
		((fcparms_t *)pcparms.pc_clparms)->fc_cpupri = 0;
		((fcparms_t *)pcparms.pc_clparms)->fc_umdpri = 0;

		if (priocntl(P_PID, pidlist[i], PC_GETPARMS, &pcparms) == -1) {
			pfmt(stdout, MM_WARNING, ":1069:\tCannot get FC user priority\n");
			continue;
		}

		if (pcparms.pc_cid == fccid) {
			printf(" %5d  %5d  %5d  %5d   %5d\n",
			    ((fcparms_t *)pcparms.pc_clparms)->fc_uprilim,
			    ((fcparms_t *)pcparms.pc_clparms)->fc_upri,
			    ((fcparms_t *)pcparms.pc_clparms)->fc_timeleft, 
			    ((fcparms_t *)pcparms.pc_clparms)->fc_cpupri,
			    ((fcparms_t *)pcparms.pc_clparms)->fc_umdpri);
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
 * Procedure:	set_fcprocs
 *
 * Notes: Set all processes in the set specified by idtype/idargv to
 *	  time-sharing (if they aren't already time-sharing) and set
 *	  their user priority limit and user priority to those
 *	  specified by fcuprilim and fcupri. 
 *
 * Restrictions: priocntl(2): <none>
 */
static void
set_fcprocs(idtype, idargc, idargv, fcuprilim, fcupri)
idtype_t	idtype;
int		idargc;
char		**idargv;
short		fcuprilim;
short		fcupri;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxupri;
	char		idtypnm[12];
	int		i;
	id_t		fccid;

	/*
	 * If both user priority and limit have been defaulted then they
	 * need to be changed to 0 for later priocntl system calls.
	 */
	if (fcuprilim == FC_NOCHANGE && fcupri == FC_NOCHANGE)
		fcuprilim = fcupri = 0;

	/*
	 * Get the time sharing class ID and max configured user priority.
	 */
	strcpy(pcinfo.pc_clname, "FC");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid, errno);
		exit(1);
	}
	maxupri = ((fcinfo_t *)pcinfo.pc_clinfo)->fc_maxupri;

	/*
	 * Validate the fcuprilim and fcupri arguments.
	 */
	if ((fcuprilim > maxupri || fcuprilim < -maxupri) &&
	    fcuprilim != FC_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badlimit,  fcuprilim);
		exit(1);
	}
	if ((fcupri > maxupri || fcupri < -maxupri) && fcupri != FC_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badrange,  fcupri);
		exit(1);
	}
	pcparms.pc_cid = pcinfo.pc_cid;
	((fcparms_t *)pcparms.pc_clparms)->fc_uprilim = fcuprilim;
	((fcparms_t *)pcparms.pc_clparms)->fc_upri = fcupri;

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
				fccid = clname2cid(idargv[i]);
				if (priocntl(idtype, fccid, PC_SETPARMS, &pcparms) == -1) {
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
 * Procedure:     exec_fccmd
 *
 * Notes: Execute the command pointed to by cmdargv as a time-sharing
 *	  process with the user priority limit given by fcuprilim and
 *	  user priority fcupri.
 *
 * Restrictions: priocntl(2): <none>
 *               execvp(2): P_MACREAD
 */
static void
exec_fccmd(cmdargv, fcuprilim, fcupri)
char	**cmdargv;
short	fcuprilim;
short	fcupri;
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	short		maxupri;
	uid_t		priv_id;	/* hold privileged ID if any */
	
	/*
	 * If both user priority and limit have been defaulted then they
	 * need to be changed to 0 for later priocntl system calls.
	 */
	if (fcuprilim == FC_NOCHANGE && fcupri == FC_NOCHANGE)
		fcuprilim = fcupri = 0;

	/*
	 * Get the time sharing class ID and max configured user priority.
	 */
	strcpy(pcinfo.pc_clname, "FC");
	if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
		pfmt(stderr, MM_ERROR, badclid,  errno);
		exit(1);
	}
	maxupri = ((fcinfo_t *)pcinfo.pc_clinfo)->fc_maxupri;

	if ((fcuprilim > maxupri || fcuprilim < -maxupri) && 
	    fcuprilim != FC_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badlimit,  fcuprilim);
		exit(1);
	}
	if ((fcupri > maxupri || fcupri < -maxupri) && fcupri != FC_NOCHANGE) {
		pfmt(stderr, MM_ERROR, badrange,  fcupri);
		exit(1);
	}
	pcparms.pc_cid = pcinfo.pc_cid;
	((fcparms_t *)pcparms.pc_clparms)->fc_uprilim = fcuprilim;
	((fcparms_t *)pcparms.pc_clparms)->fc_upri = fcupri;
	
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
