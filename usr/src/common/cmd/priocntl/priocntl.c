/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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

#ident	"@(#)mp.cmds:common/cmd/priocntl/priocntl.c	1.6.1.1"
#ident  "$Header$"
/***************************************************************************
 * Command: priocntl
 *
 * Inheritable Privileges: P_OWNER,P_TSHAR,P_RTIME,P_MACREAD,P_MACWRITE
 *       Fixed Privileges: P_DACREAD
 *
 * Notes: P_DACREAD is given as a fixed privilege because it is needed
 * 	to open all processes in /proc in the current level.  P_MACREAD
 *	may be used to open processes of all levels.  P_OWNER, P_TSHAR,
 *	P_MACREAD, P_MACWRITE and P_RTIME are needed for priocntl(2).
 *
 * This file contains the code implementing the class independent part
 * of the priocntl command.  Most of the useful work for the priocntl
 * command is done by the class specific sub-commands, the code for
 * which is elsewhere.  The class independent part of the command is
 * responsible for executing the appropriate class specific sub-commands
 * and providing any necessary input to the sub-commands.
 * Code in this file should never assume any knowledge of any specific
 * scheduler class (other than the SYS class).
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<search.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<dirent.h>
#include	<fcntl.h>
#include	<limits.h>
#include	<sys/procset.h>
#include	<sys/priocntl.h>
#include	"priocntl.h"
#include	<sys/procfs.h>
#include	<macros.h>
#include	<errno.h>
#include	<priv.h>
#include	<pfmt.h>
#include	<locale.h>

#define	CLASSPATH	"/usr/lib/class"

typedef struct classpids {
	char	clp_clname[PC_CLNMSZ];
	pid_t	*clp_pidlist;
	int	clp_pidlistsz;
	int	clp_npids;
} classpids_t;

static char usage[] =
	":1015:Usage:\n\tpriocntl -l\n\tpriocntl -d [-i idtype] [idlist]\n\tpriocntl -s [-c class] [c.s.o.] [-i idtype] [idlist]\n\tpriocntl -e [-c class] [c.s.o.] command [argument(s)]\n\tpriocntl -r size [-i idtype] [idlist]\n\tpriocntl -t interval [-i idtype] [idlist]\n\tpriocntl -q init min max [-i idtype] [idlist]\n\tpriocntl -g [-i idtype] [idlist]\n";

static char
	badclnum[] = ":1016:Cannot get number of configured classes, errno = %d\n",
	badclname[] = ":1017:Cannot get class name (class ID = %d)\n",
	badexec[] = ":1018:Cannot execute %s specific subcommand\n",
	badmalloc[] = ":1019:Cannot allocate memory\n",
	badfind[] = ":1020:Process not found\n",
	badclproc[] = ":1021:Cannot get class of process, errno = %d\n",
	badclass[] = ":1022:Invalid or unconfigured class %s\n",
	badsetuid[] = ":1023:Cannot set effective UID back to real UID\n";
	
static char	basenm[BASENMSZ];

static char	*procdir = "/proc";

static void	print_classlist(), set_procs(), exec_cmd(), print_procs(),
		setage_procs(), getage_procs();
static void	ids2pids(), add_pid_tolist();
static boolean_t	idmatch();

/*
 * Procedure: main - process options & call appropriate subroutines.
 */
main(argc, argv)
int	argc;
char	**argv;
{
	extern char	*optarg;
	extern int	optind, opterr;

	int		c, n, i, err;
	int		lflag, dflag, sflag, eflag, cflag, iflag, csoptsflag,
			rflag, tflag, qflag, gflag;
	char		*clname;
	char		*idtypnm;
	idtype_t	idtype;
	int		idargc;
	char		**idargv;
	size_t		rss_size = 0;
	ageparms_t	age_args;
	priv_t		privs[NPRIVS * 2];
	clock_t		et_interval = 0;
	clock_t		init_qntm = 0, min_qntm = 0, max_qntm = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:priocntl");

	strcpy(basenm, basename(argv[0]));
	rflag = lflag = dflag = sflag = eflag = cflag = iflag = csoptsflag = 
	tflag = qflag = gflag = 0;
	opterr = 0;
	while ((c = getopt(argc, argv, "gldsec:i:r:t:q:")) != -1) {

		switch(c) {

		case 'l':
			lflag++;
			break;

		case 'r':
			if (sflag || cflag)
				break;
			rflag++;
			rss_size = strtoul(optarg, NULL, 0);
			break;

		case 'g':
			gflag++;
			break;

		case 't':
			if (sflag || cflag)
				break;
			tflag++;
			et_interval = strtol(optarg, NULL, 0);
			break;

		case 'q':
			qflag++;
			err = sscanf(optarg, "%d", &init_qntm);
			if (err != 1 || optind >= argc ) {
				pfmt(stderr, MM_ERROR, usage);
				exit(1);
			}	
			err = sscanf(argv[optind++], "%d", &min_qntm);
			if (err != 1 || optind >= argc) {
				pfmt(stderr, MM_ERROR, usage);
				exit(1);
			}
			err = sscanf(argv[optind++], "%d", &max_qntm);
			if (err != 1) {
				pfmt(stderr, MM_ERROR, usage);
				exit(1);
			}
			break;

		case 'd':
			dflag++;
			break;

		case 's':
			sflag++;
			break;

		case 'e':
			eflag++;
			break;

		case 'c':
			cflag++;
			if (strcmp(optarg,"RT") == 0 ){	/*COMPATABILITY*/
				clname = "FP";
			} else
				clname = optarg;
			break;

		case 'i':
			iflag++;
			idtypnm = optarg;
			break;

		case '?':
			/*
			 * getopt() returns ? if either "-i"
			 * or "-c" appears without an argument.
			 */
			if (strcmp(argv[optind - 1], "-c") == 0 ||
			    strcmp(argv[optind - 1], "-i") == 0) {
				pfmt(stderr, MM_ERROR, usage);
				exit(1);
			}

			/*
			 * We assume for now that any option that
			 * getopt() doesn't recognize (with the
			 * exception of c and i) is intended for a
			 * class specific subcommand.
			 */
			csoptsflag++;
			if (argv[optind][0] != '-' || isdigit(argv[optind][1])) {
				/*
				 * Class specific option takes an
				 * argument which we skip over for now.
				 */
				optind++;
			}
			break;
		}
	}

	if (lflag) {
		if (dflag || sflag || eflag || cflag || iflag || csoptsflag ||
			rflag || tflag || qflag || gflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		print_classlist();
		exit(0);

	} else if (dflag) {
		if (lflag || sflag || eflag || cflag || csoptsflag || rflag ||
			tflag || qflag || gflag) {
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

		if (optind < argc) {
			idargc = argc - optind;
			idargv = &argv[optind];
		} else
			idargc = 0;

		print_procs(idtype, idargc, idargv);
		exit(0);

	} else if (sflag) {
		if (lflag || dflag || eflag || rflag || tflag || qflag || 
			gflag) {
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

		if (cflag == 0)
			clname = NULL;

		if (optind < argc) {
			idargc = argc - optind;
			idargv = &argv[optind];
		} else
			idargc = 0;

		set_procs(clname, idtype, idargc, idargv, argv);

	} else if (gflag) {
		if (lflag || sflag || eflag || cflag || csoptsflag || rflag ||
			tflag || qflag || dflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		if (iflag) {
			if (str2idtyp(idtypnm, &idtype) == -1) {
				pfmt(stderr, MM_ERROR, ":1024:Bad idtype %s\n",
					idtypnm);
				exit(1);
			}
		} else
			idtype = P_PID;

		if (idtype == P_CID) {
			pfmt(stderr, MM_ERROR,
				":1024:Bad idtype %s\n",idtypnm);
			exit(1);
		}

		if (optind < argc) {
			idargc = argc - optind;
			idargv = &argv[optind];
		} else
			idargc = 0;

		getage_procs(idtype, idargc, idargv);
		exit(0);
	} else if (rflag || tflag || qflag) {
		if (lflag || dflag || eflag || cflag || csoptsflag || gflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		if (iflag) {
			if (str2idtyp(idtypnm, &idtype) == -1) {
				pfmt(stderr, MM_ERROR, 
					":1024:Bad idtype %s\n",idtypnm);
				exit(1);
			}
		} else
			idtype = P_PID;

		if (idtype == P_CID) {
			pfmt(stderr, MM_ERROR,
				":1024:Bad idtype %s\n",idtypnm);
			exit(1);
		}

		memset(&age_args, 0, sizeof(ageparms_t));

		if (rss_size == ULONG_MAX) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		} else
			age_args.maxrss = rss_size;

		age_args.et_age_interval = et_interval;
		age_args.init_agequantum = init_qntm;
		age_args.min_agequantum = min_qntm;
		age_args.max_agequantum = max_qntm;

		if (optind < argc) {
			idargc = argc - optind;
			idargv = &argv[optind];
		} else
			idargc = 0;

		setage_procs(idtype, idargc, idargv, &age_args);

	} else if (eflag) {
		if (lflag || dflag || sflag || iflag || rflag || tflag || 
			qflag || gflag) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		if (cflag == 0)
			clname = NULL;

		if (optind >= argc) {
			pfmt(stderr, MM_ERROR, usage);
			exit(1);
		}
		exec_cmd(clname, argv);

	} else {
		pfmt(stderr, MM_ERROR, usage);
		exit(1);
	}
	/* NOTREACHED */
}

/*
 * Procedure:     print_classlist
 *
 * Restrictions: 
 *  priocntl(2): <none>	
 *  execl(2): P_MACREAD
 *
 * Notes: Print the heading for the class list and execute the class
 * 	  specific sub-command with the -l option for each configured
 *	  class.  Clear P_MACREAD before exec'ing the sub-command to
 *	  prevent from exec'ing a command at the wrong level.
 */
static void
print_classlist()
{
	id_t		cid;
	int		nclass;
	pcinfo_t	pcinfo;
	static char	subcmdpath[128];
	int		status;
	pid_t		pid;

	if ((nclass = priocntl(0, 0, PC_GETCLINFO, NULL)) == -1) {
		pfmt(stderr, MM_ERROR, badclass, errno);
		exit(1);
	}
	pfmt(stdout, MM_NOSTD, ":1025:CONFIGURED CLASSES\n==================\n\n");
	pfmt(stdout, MM_NOSTD, ":1026:SYS (System Class)\n");
	for (cid = 1; cid < nclass; cid++) {
		printf("\n");
		fflush(stdout);
		pcinfo.pc_cid = cid;
		if (priocntl(0, 0, PC_GETCLINFO, &pcinfo) == -1) {
			pfmt(stderr, MM_ERROR, badclname, cid);
			exit(1);
		}
		sprintf(subcmdpath, "%s/%s/%s%s", CLASSPATH, pcinfo.pc_clname,
			pcinfo.pc_clname, basenm);
		if ((pid = fork()) == 0) {	/* child */
			/*
			 * clear P_MACREAD so that the correct level
			 * subcmd is exec'ed. Set effective UID back
			 * to real UID for the benefit of the privileged
			 * ID based system.
			 */
			if (setuid(getuid()) < 0) {
				pfmt(stderr, MM_ERROR, badsetuid);
				exit(1);
			}
			procprivl(CLRPRV, MACREAD_W, pm_max(P_DACREAD), 0);
			(void)execl(subcmdpath, subcmdpath, "-l", (char *)0);
			pfmt(stderr, MM_ERROR, badexec, pcinfo.pc_clname);
			exit(1);
		} else if (pid == (pid_t)-1) {
			pfmt(stderr, MM_ERROR, ":1027:Cannot fork\n");
			exit(1);
		} else {
			wait(&status);
		}
	}
}

static void
setage_procs(idtype, idargc, idargv, cmdargs)
idtype_t	idtype;
int		idargc;
char		**idargv;
void		*cmdargs;
{
	int		i, nids, error;
	id_t		id;
	id_t		idlist[NIDS];
	ageparms_t	ageparms;

	/*
	 * Build a list of ids eliminating any duplicates in idargv.
	 */
	nids = 0;
	if (idargc == 0 && idtype != P_ALL) {

		/*
		 * No ids supplied by user; use current id.
		 */
		if (getmyid(idtype, &idlist[0]) == -1) {
			pfmt(stderr, MM_ERROR, ":1028:Cannot get ID for current process, idtype = %d\n",idtype);
			exit(1);
		}
		nids = 1;
	} else {
		for (i = 0; i < idargc && nids < NIDS; i++) {
			id = (id_t)atol(idargv[i]);
			/*
			 * lsearch(3C) adds ids to the idlist,
			 * eliminating duplicates.
			 */
			(void)lsearch((char *)&id, (char *)idlist,
			    (unsigned int *)&nids, sizeof(id), idcompar);
		}
	}

	/* call priocntl on each of the id on the idlist */
	for (i = 0; i < nids; i++) {
		error = priocntl(idtype, idlist[i], PC_SETAGEPARMS, cmdargs);
		if (error == -1) {
			if (errno == ESRCH) {
				pfmt(stderr, MM_ERROR,
					":1020:process not found\n",
					idlist[i]);
				continue;
			}
			pfmt(stderr, MM_ERROR, 
				":1232:priocntl failed; errno = %d\n", errno);
			exit(1);
		}
	}
}

static void
getage_procs(idtype, idargc, idargv)
idtype_t	idtype;
int		idargc;
char		**idargv;
{
	int		i;
	id_t		id;
	id_t		idlist[NIDS];
	int		nids;
	int		maxpids, pidexists;
	int		pid;
	classpids_t	clpid;
	ageparms_t	age_args;

	/*
	 * Build a list of ids eliminating any duplicates in idargv.
	 */
	nids = 0;
	if (idargc == 0 && idtype != P_ALL) {

		/*
		 * No ids supplied by user; use current id.
		 */
		if (getmyid(idtype, &idlist[0]) == -1) {
			pfmt(stderr, MM_ERROR, 
				":1028:Cannot get ID for current process, idtype = %d\n",
				idtype);
			exit(1);
		}
		nids = 1;
	} else {
		for (i = 0; i < idargc && nids < NIDS; i++) {
			id = (id_t)atol(idargv[i]);
			/*
			 * lsearch(3C) adds ids to the idlist,
			 * eliminating duplicates.
			 */
			(void)lsearch((char *)&id, (char *)idlist,
			    (unsigned int *)&nids, sizeof(id), idcompar);
		}
	}


	/*
	 * Allocate memory for the pidlist.  We won't know for sure
	 * how many pids we'll need until we actually build the list
	 * but we try not to be too wasteful here.  We allocate enough
	 * min(NPIDS , nids * our best guess at the max number
	 * of pids a given id will yield).
	 */
	maxpids = idtyp2maxprocs(idtype);
	if (idtype == P_ALL)
		maxpids = min(maxpids, NPIDS);
	else
		maxpids = min(maxpids * nids, NPIDS);

	if ((clpid.clp_pidlist = 
		     (pid_t *)malloc(maxpids * sizeof(pid_t))) == NULL) {
			pfmt(stderr, MM_ERROR, badmalloc);
			exit(1);
	}

	clpid.clp_pidlistsz = maxpids;
	clpid.clp_npids = 0;

	/*
	 * Build the pidlist.
	 */
	ids2pids(idtype, idlist, nids, &clpid, -1);

	for (i = 0; i < clpid.clp_npids; i++) {
		if (priocntl(P_PID, clpid.clp_pidlist[i], PC_GETAGEPARMS,
				&age_args) == -1) {
			if (errno == ESRCH)
				memset(&age_args, 0, sizeof(ageparms_t));
			else {
				pfmt(stderr, MM_ERROR, 
					":1232:priocntl failed; errno = %d\n",
					errno);
				exit(1);
			}
		}
		if (!i)
			pfmt(stdout, MM_NOSTD,
			":1233:PID	MAXRSS	AGE_INTERVAL	AGE_QUANTUN\n");


		pfmt(stdout, MM_NOSTD, "::%d	%u	%ld		%ld %ld %ld\n", 
			clpid.clp_pidlist[i], age_args.maxrss,
			age_args.et_age_interval, age_args.init_agequantum,
			age_args.min_agequantum, age_args.max_agequantum);
	}
}

/*
 * Procedure:     print_procs
 *
 * Restrictions: priocntl(2): <none>	
 *               execl(2): P_MACREAD
 *               fcntl(2): None
 *               fwrite: None
 *               fclose: None
 *
 * Notes: For each class represented within the set of processes
 *	  specified by idtype/idargv, print_procs() executes the class
 *	  specific sub-command with the -d option.  We pipe to each
 *	  sub-command a list of pids in the set belonging to that class. 
 *
 */
static void
print_procs(idtype, idargc, idargv)
idtype_t	idtype;
int		idargc;
char		**idargv;
{
	int		i;
	id_t		id;
	id_t		idlist[NIDS];
	int		nids;
	classpids_t	*clpids;
	int		nclass;
	id_t		cid;
	pcinfo_t	pcinfo;
	int		maxpids, pidexists;
	FILE		*pipe_to_subcmd;
	int 		pipe_fildes[2];
	char		subcmd[128];
	int		pid;

	/*
	 * Build a list of ids eliminating any duplicates in idargv.
	 */
	nids = 0;
	if (idargc == 0 && idtype != P_ALL) {

		/*
		 * No ids supplied by user; use current id.
		 */
		if (getmyid(idtype, &idlist[0]) == -1) {
			pfmt(stderr, MM_ERROR, ":1028:Cannot get ID for current process, idtype = %d\n",idtype);
			exit(1);
		}
		nids = 1;
	} else {
		for (i = 0; i < idargc && nids < NIDS; i++) {
			if (idtype == P_CID) {
				if ((id = clname2cid(idargv[i])) == -1)
					pfmt(stderr, MM_ERROR, ":1029:Invalid or unconfigured class %s in idlist - ignored\n", idargv[i]);
			} else {
				id = (id_t)atol(idargv[i]);
			}

			/*
			 * lsearch(3C) adds ids to the idlist,
			 * eliminating duplicates.
			 */
			(void)lsearch((char *)&id, (char *)idlist,
			    (unsigned int *)&nids, sizeof(id), idcompar);
		}
	}

	if ((nclass = priocntl(0, 0, PC_GETCLINFO, NULL)) == -1) {
		pfmt(stderr, MM_ERROR, badclnum, errno);
		exit(1);
	}
	if ((clpids= (classpids_t *)malloc(sizeof(classpids_t)*nclass))	== NULL) {
		pfmt(stderr, MM_ERROR, badmalloc);
		exit(1);
	}
	for (cid = 1; cid < nclass; cid++) {
		pcinfo.pc_cid = cid;
		if (priocntl(0, 0, PC_GETCLINFO, &pcinfo) == -1) {
			pfmt(stderr, MM_ERROR, badclname, cid);
			exit(1);
		}
		strcpy(clpids[cid].clp_clname, pcinfo.pc_clname);

		/*
		 * Allocate memory for the pidlist.  We won't know for sure
		 * how many pids we'll need until we actually build the list
		 * but we try not to be too wasteful here.  We allocate enough
		 * min(NPIDS , nids * our best guess at the max number
		 * of pids a given id will yield).
		 */
		maxpids = idtyp2maxprocs(idtype);
		if (idtype == P_ALL)
			maxpids = min(maxpids, NPIDS);
		else
			maxpids = min(maxpids * nids, NPIDS);
		if ((clpids[cid].clp_pidlist = 
		     (pid_t *)malloc(maxpids * sizeof(pid_t))) == NULL) {
			pfmt(stderr, MM_ERROR, badmalloc);
			exit(1);
		}
		clpids[cid].clp_pidlistsz = maxpids;
		clpids[cid].clp_npids = 0;
	}

	/*
	 * Build the pidlist.
	 */
	ids2pids(idtype, idlist, nids, clpids, nclass);
	
	/*
	 * For each class, the class dependent subcommand will be
	 * executed.  A list of process ids will be passed to the
	 * subcommand through a pipe.
	 */
	pidexists = 0;
	for (cid = 1; cid < nclass; cid++) {
		if (clpids[cid].clp_npids == 0)
			continue;
		pidexists = 1;
		sprintf(subcmd, "%s/%s/%s%s", CLASSPATH, clpids[cid].clp_clname,
		    clpids[cid].clp_clname, basenm);
		if (pipe(pipe_fildes) < 0) {
			pfmt(stderr, MM_ERROR, ":1030:Cannot pipe\n");
			exit(1);
		}
		if ((pid = fork()) == 0) {	/* child */
			close(0);
			fcntl(pipe_fildes[0], F_DUPFD, 0);
			close(pipe_fildes[0]); 
			close(pipe_fildes[1]); /* child uses read side only */
			/*
			 * clear P_MACREAD so that the correct level
			 * subcmd is exec'ed. Set effective UID back
			 * to real UID for the benefit of the privileged
			 * ID based system.
			 */
			if (setuid(getuid()) < 0) {
				pfmt(stderr, MM_ERROR, badsetuid);
				exit(1);
			}
			procprivl(CLRPRV, MACREAD_W, pm_max(P_DACREAD), 0);
			(void)execl(subcmd, subcmd, "-d", (char *) 0);
			pfmt(stderr, MM_ERROR, badexec, clpids[cid].clp_clname);
		}
		else if (pid == -1) {
			pfmt(stderr, MM_ERROR, ":1027:Cannot fork\n");
			exit(1);
		}
		pipe_to_subcmd = fdopen(pipe_fildes[1], "w");
		fwrite(clpids[cid].clp_pidlist, sizeof(pid_t),
		       clpids[cid].clp_npids, pipe_to_subcmd);
		(void) fclose(pipe_to_subcmd);
		(void) wait(0);
	}
	if (pidexists == 0) {
		pfmt(stderr, MM_ERROR, ":1031:Process or processes not found.\n");
		exit(1);
	}
}


/*
 * Procedure:     set_procs
 *
 * Notes: Execute the appropriate class specific sub-command with the
 *	  arguments pointed to by subcmdargv.  If the user specified a
 *	  class we simply exec the sub-command for that class.  If no
 *	  class was specified we verify that the processes in the set
 *	  specified by idtype/idargv are all in the same class and then
 *	  execute the sub-command for that class.
 *
 * Restrictions: priocntl(2): <none>	
 *               opendir: <none>
 *		           open(2): <none>	
 *               execv(2): P_MACREAD
 *		 
 */
static void
set_procs(clname, idtype, idargc, idargv, subcmdargv)
char	       *clname;
idtype_t	idtype;
int		idargc;
char	      **idargv;
char	      **subcmdargv;
{
	char		idstr[12];
	char		myidstr[12];
	char		clnmbuf[PC_CLNMSZ];
	pcparms_t	pcparms;
	pcinfo_t	pcinfo;
	static pstatus_t pstatus;
	static psinfo_t	prinfo;
	static prcred_t	prcred;
	DIR	       *dirp;
	struct dirent  *dentp;
	static char	pname[100];
	int		procfd;
	int		saverr;
	static char	subcmdpath[128];
	boolean_t	procinset;
	id_t		id;

	if (clname == NULL && idtype == P_PID && idargc <= 1) {

		/*
		 * No class specified by user but only one process in
		 * specified set.  Get the class the easy way.
		 */
		pcparms.pc_cid = PC_CLNULL;
		id = (idargc == 0)? P_MYID : atoi(idargv[0]);
		if (priocntl(P_PID, id, PC_GETPARMS, &pcparms) == -1) {
			if (errno == ESRCH)
				pfmt(stderr, MM_ERROR, badfind);
			else
				pfmt(stderr, MM_ERROR, badclproc, errno);
			exit(1);
		}
		pcinfo.pc_cid = pcparms.pc_cid;
		if (priocntl(0, 0, PC_GETCLINFO, &pcinfo) == -1) {
			pfmt(stderr, MM_ERROR, badclname, pcinfo.pc_cid);
			exit(1);
		}
		clname = pcinfo.pc_clname;

	} else if (clname == NULL) {

		/*
		 * No class specified by user and potentially more than one
		 * process in specified set.  Verify that all procs in set
		 * are in the same class.
		 */
		if (idargc == 0 && idtype != P_ALL) {
			/* No ids supplied by user; use current id */
			if (getmyidstr(idtype, myidstr) == -1) {
				pfmt(stderr, MM_ERROR, ":1032:Cannot get ID string for current process, idtype = %d\n", idtype);
				exit(1);
			}
		}
		if ((dirp = opendir(procdir)) == NULL) {
			pfmt(stderr, MM_ERROR, ":1033:Cannot open PROC directory %s\n", procdir);
			exit(1);
		}
		while ((dentp = readdir(dirp)) != NULL) {
			if (dentp->d_name[0] == '.')	/* skip . and .. */
				continue;
	retry:
			/* open psinfo */
			sprintf(pname, "%s/%s/psinfo", procdir, dentp->d_name);
			if ((procfd = open(pname, O_RDONLY)) < 0)
				continue;

			/* read psinfo */
			if (read(procfd, &prinfo, sizeof(psinfo_t)) != sizeof(psinfo_t)) {
				saverr = errno;
				close(procfd);
				if (saverr == EAGAIN)
					goto retry;
				if (saverr != ENOENT)
					pfmt(stderr, MM_ERROR, ":1034:Cannot get process info for %s\n", pname);
				continue;
			}

			close(procfd);

			if (prinfo.pr_nlwp == 0)	/* zombie */
				continue;

			if (idtype == P_UID || idtype == P_GID) {

				/* open credentials */
				sprintf(pname, "%s/%s/cred", procdir, dentp->d_name);
				if ((procfd = open(pname, O_RDONLY)) < 0) {
					if (errno == EAGAIN)
						goto retry;
					if (errno != ENOENT)
						pfmt(stderr, MM_ERROR, ":1035:Cannot get process credentials for %s\n", pname);
					continue;
				}

				/* read credentials */
				if (read(procfd, &prcred, sizeof(prcred_t)) != sizeof(prcred_t)) {
					saverr = errno;
					close(procfd);
					if (saverr == EAGAIN)
						goto retry;
					if (saverr != ENOENT)
						pfmt(stderr, MM_ERROR, ":1035:Cannot get process credentials for %s\n", pname);
					continue;
				}

				close(procfd);
			}

			if (idtype == P_PGID) {

				/* open status */
				sprintf(pname, "%s/%s/status", procdir, dentp->d_name);
				if ((procfd = open(pname, O_RDONLY)) < 0) {
					if (errno == EAGAIN)
						goto retry;
					if (errno != ENOENT)
						pfmt(stderr, MM_ERROR, ":1039:Cannot get process status for %s\n", pname);
					continue;
				}

				/* read status */
				if (read(procfd, &pstatus, sizeof(pstatus_t)) != sizeof(pstatus_t)) {
					saverr = errno;
					close(procfd);
					if (saverr == EAGAIN)
						goto retry;
					if (saverr != ENOENT)
						pfmt(stderr, MM_ERROR, ":1039:Cannot get process status for %s\n", pname);
					continue;
				}

				close(procfd);
			}

			switch (idtype) {

			case P_PID:
				itoa((long) prinfo.pr_pid, idstr);
				procinset = idmatch(idstr, myidstr, idargc, idargv);
				break;

			case P_PPID:
				itoa((long) prinfo.pr_ppid, idstr);
				procinset = idmatch(idstr, myidstr, idargc, idargv);
				break;

			case P_PGID:
				itoa((long) pstatus.pr_pgid, idstr);
				procinset = idmatch(idstr, myidstr, idargc, idargv);
				break;

			case P_SID:
				itoa((long) prinfo.pr_sid, idstr);
				procinset = idmatch(idstr, myidstr, idargc, idargv);
				break;

			case P_CID:
				procinset = idmatch(prinfo.pr_lwp.pr_clname, myidstr, idargc, idargv);
				break;

			case P_UID:
				itoa((long) prcred.pr_euid, idstr);
				procinset = idmatch(idstr, myidstr, idargc, idargv);
				break;

			case P_GID:
				itoa((long) prcred.pr_egid, idstr);
				procinset = idmatch(idstr, myidstr, idargc, idargv);
				break;

			case P_ALL:
				procinset = B_TRUE;
				break;

			default:
				pfmt(stderr, MM_ERROR, ":1036:Bad idtype %d in set_procs()\n", idtype);
				exit(1);
			}

			if (procinset == B_TRUE) {
				if (clname == NULL) {
					/*
					 * First proc found in set.
					 */
					strcpy(clnmbuf, prinfo.pr_lwp.pr_clname);
					clname = clnmbuf;
				} else if (strcmp(clname, prinfo.pr_lwp.pr_clname) != 0) {
					pfmt(stderr, MM_ERROR, ":1037:Specified processes from different classes.\n");
					exit(1);
				}
			}
		}  /* end while */

		closedir(dirp);

		if (clname == NULL) {
			pfmt(stderr, MM_ERROR, badfind);
			exit(1);
		}
	} else {
		/*
		 * User specified class. Check it for validity.
		 */
		strcpy(pcinfo.pc_clname, clname);
		if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
			pfmt(stderr, MM_ERROR, badclass, clname);
			exit(1);
		}
	}

	sprintf(subcmdpath, "%s/%s/%s%s", CLASSPATH, clname, clname, basenm);
	subcmdargv[0] = subcmdpath;

	/*
	 * Clear P_MACREAD so that the correct level subcmd is exec'ed.
	 * Set effective UID back to real UID for the benefit of the
	 * privileged ID based system.
	 */
	if (setuid(getuid()) < 0) {
		pfmt(stderr, MM_ERROR, badsetuid);
		exit(1);
	}
	procprivl(CLRPRV, MACREAD_W, pm_max(P_DACREAD), 0);
	(void) execv(subcmdpath, subcmdargv);
	pfmt(stderr, MM_ERROR, badexec, clname);
	exit(1);
}


/*
 * Procedure:     exec_cmd
 *
 * Notes: Execute the appropriate class specific sub-command with the
 *	  arguments pointed to by subcmdargv.  If the user specified a
 *	  class we simply exec the sub-command for that class.  If no
 *	  class was specified we execute the sub-command for our own

 *	  current class. 
 *
 * Restrictions: 
 *  priocntl(2): <none>	
 *  execv(2): P_MACREAD
 */
static void
exec_cmd(clname, subcmdargv)
char	*clname;
char	**subcmdargv;
{
	pcparms_t	pcparms;
	pcinfo_t	pcinfo;
	char		subcmdpath[128];

	if (clname == NULL) {
		pcparms.pc_cid = PC_CLNULL;
		if (priocntl(P_PID, P_MYID, PC_GETPARMS, &pcparms) == -1) {
			pfmt(stderr, MM_ERROR, badclproc, errno);
			exit(1);
		}
		pcinfo.pc_cid = pcparms.pc_cid;
		if (priocntl(0, 0, PC_GETCLINFO, &pcinfo) == -1) {
			pfmt(stderr, MM_ERROR, badclname, pcinfo.pc_cid);
			exit(1);
		}
		clname = pcinfo.pc_clname;
	} else {

		/*
		 * User specified class. Check it for validity.
		 */
		strcpy(pcinfo.pc_clname, clname);
		if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
			pfmt(stderr, MM_ERROR, badclass, clname);
			exit(1);
		}
	}

	sprintf(subcmdpath, "%s/%s/%s%s", CLASSPATH, clname, clname, basenm);
	subcmdargv[0] = subcmdpath;

	/*
	 * clear P_MACREAD so that the correct level
	 * subcmd is exec'ed. Set effective UID back
	 * to real UID for the benefit of the privileged
	 * ID based system.
	 */
	if (setuid(getuid()) < 0) {
		pfmt(stderr, MM_ERROR, badsetuid);
		exit(1);
	}
	procprivl(CLRPRV, MACREAD_W, pm_max(P_DACREAD), 0);
	(void)execv(subcmdpath, subcmdargv);
	fprintf(stderr, clname);
	exit(1);
}

/*
 * Procedure:     ids2pids
 *
 * Notes: Fill in the classpids structures in the array pointed to by
 *	  clpids with pids for the processes in the set specified by
 *	  idtype/idlist. We read the /proc/<PID>/psinfo  file to get
 *	  the necessary process information.
 *
 * Restrictions: 
 *  opendir: <none>	
 *  open(2): <none>
 *  ioctl(2): <none>
 */

static void
ids2pids(idtype, idlist, nids, clpids, nclass)
idtype_t	idtype;
id_t	       *idlist;
int		nids;
classpids_t    *clpids;
int		nclass;
{
	static psinfo_t prinfo;
	static pstatus_t pstatus;
	static prcred_t prcred;
	DIR	       *dirp;
	struct dirent  *dentp;
	char		pname[100];
	int		procfd;
	int		saverr;
	int		i;
	char	       *clname;

	if ((dirp = opendir(procdir)) == NULL) {
		pfmt(stderr, MM_ERROR, ":1033:Cannot open PROC directory %s\n", procdir);
		exit(1);
	}

	while ((dentp = readdir(dirp)) != NULL) {
		if (dentp->d_name[0] == '.')		/* skip . and .. */
			continue;
	retry:
		/* open psinfo */
		sprintf(pname, "%s/%s/psinfo", procdir, dentp->d_name);
		if ((procfd = open(pname, O_RDONLY)) < 0)
			continue;

		/* read psinfo */
		if (read(procfd, &prinfo, sizeof(psinfo_t)) != sizeof(psinfo_t)) {
			saverr = errno;
			close(procfd);
			if (saverr == EAGAIN)
				goto retry;
			if (saverr != ENOENT)
				pfmt(stderr, MM_ERROR, ":1034:Cannot get process info for %s\n", pname);
			continue;
		}

		close(procfd);

		if (prinfo.pr_nlwp == 0)		/* zombie */
			continue;

		if (idtype == P_UID || idtype == P_GID) {

			/* open credentials */
			sprintf(pname, "%s/%s/cred", procdir, dentp->d_name);
			if ((procfd = open(pname, O_RDONLY)) < 0) {
				if (errno == EAGAIN)
					goto retry;
				if (errno != ENOENT)
					pfmt(stderr, MM_ERROR, ":1035:Cannot get process credentials for %s\n", pname);
				continue;
			}

			/* read credentials */
			if (read(procfd, &prcred, sizeof(prcred_t)) != sizeof(prcred_t)) {
				saverr = errno;
				close(procfd);
				if (saverr == EAGAIN)
					goto retry;
				if (saverr != ENOENT)
					pfmt(stderr, MM_ERROR, ":1035:Cannot get process credentials for %s\n", pname);
				continue;
			}

			close(procfd);
		}

		if (idtype == P_PGID) {

			/* open status */
			sprintf(pname, "%s/%s/status", procdir, dentp->d_name);
			if ((procfd = open(pname, O_RDONLY)) < 0) {
				if (errno == EAGAIN)
					goto retry;
				if (errno != ENOENT)
					pfmt(stderr, MM_ERROR, ":1039:Cannot get process status for %s\n", pname);
				continue;
			}

			/* read status */
			if (read(procfd, &pstatus, sizeof(pstatus_t)) != sizeof(pstatus_t)) {
				saverr = errno;
				close(procfd);
				if (saverr == EAGAIN)
					goto retry;
				if (saverr != ENOENT)
					pfmt(stderr, MM_ERROR, ":1039:Cannot get process status for %s\n", pname);
				continue;
			}

			close(procfd);
		}

		switch (idtype) {

		case P_PID:
			for (i = 0; i < nids; i++) {
				if (idlist[i] == (id_t) prinfo.pr_pid)
					add_pid_tolist(clpids, nclass, prinfo.pr_lwp.pr_clname, prinfo.pr_pid);
			}
			break;

		case P_PPID:
			for (i = 0; i < nids; i++) {
				if (idlist[i] == (id_t) prinfo.pr_ppid)
					add_pid_tolist(clpids, nclass, prinfo.pr_lwp.pr_clname, prinfo.pr_pid);
			}
			break;

		case P_PGID:
			for (i = 0; i < nids; i++) {
				if (idlist[i] == (id_t) pstatus.pr_pgid)
					add_pid_tolist(clpids, nclass, prinfo.pr_lwp.pr_clname, prinfo.pr_pid);
			}
			break;

		case P_SID:
			for (i = 0; i < nids; i++) {
				if (idlist[i] == (id_t) prinfo.pr_sid)
					add_pid_tolist(clpids, nclass, prinfo.pr_lwp.pr_clname, prinfo.pr_pid);
			}
			break;

		case P_CID:
			for (i = 0; i < nids; i++) {
				clname = clpids[idlist[i]].clp_clname;
				if (strcmp(clname, prinfo.pr_lwp.pr_clname) == 0)
					add_pid_tolist(clpids, nclass, prinfo.pr_lwp.pr_clname, prinfo.pr_pid);
			}
			break;

		case P_UID:
			for (i = 0; i < nids; i++) {
				if (idlist[i] == (id_t) prcred.pr_euid)
					add_pid_tolist(clpids, nclass, prinfo.pr_lwp.pr_clname, prinfo.pr_pid);
			}
			break;

		case P_GID:
			for (i = 0; i < nids; i++) {
				if (idlist[i] == (id_t) prcred.pr_egid)
					add_pid_tolist(clpids, nclass, prinfo.pr_lwp.pr_clname, prinfo.pr_pid);
			}
			break;

		case P_ALL:
			add_pid_tolist(clpids, nclass, prinfo.pr_lwp.pr_clname, prinfo.pr_pid);
			break;

		default:
			pfmt(stderr, MM_ERROR, ":1038:Bad idtype %d in ids2pids()\n", idtype);
			exit(1);
		}
	}

	closedir(dirp);
}


/*
 * Procedure:     add_pid_tolist
 *
 * Notes: Search the array pointed to by clpids for the classpids
 * 	  structure corresponding to clname and add pid to its
 * 	  pidlist.
 */
static void
add_pid_tolist(clpids, nclass, clname, pid)
classpids_t	*clpids;
int		nclass;
char		*clname;
pid_t		pid;
{
	classpids_t	*clp;

	if (nclass == -1) { 	/* getage_procs() */ 
		clpids->clp_pidlist[clpids->clp_npids] = pid;
		clpids->clp_npids++;
		return;
	}

	for (clp = clpids; clp != &clpids[nclass]; clp++) {
		if (strcmp(clp->clp_clname, clname) == 0) {
			(clp->clp_pidlist)[clp->clp_npids] = pid;
			clp->clp_npids++;
			return;
		}
	}
}


/*
 * Procedure:     idmatch
 *
 * Notes: Compare id strings for equality.  If idargv contains ids
 * 	  (idargc > 0) compare idstr to each id in idargv, otherwise
 * 	  just compare to curidstr.
 */
static boolean_t
idmatch(idstr, curidstr, idargc, idargv)
char	*idstr;
char	*curidstr;
int	idargc;
char	**idargv;
{
	int	i;

	if (idargc == 0) {
		if (strcmp(curidstr, idstr) == 0)
			return(B_TRUE);
	} else {
		for (i = 0; i < idargc; i++) {
			if (strcmp(idargv[i], idstr) == 0)
				return(B_TRUE);
		}
	}
	return(B_FALSE);
}
