/*	copyright	"%c%"	*/

#ident	"@(#)fs.cmds:common/cmd/fs.d/fsck.c	1.18.15.2"

/***************************************************************************
 * Command: fsck
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_DACREAD,P_DACWRITE,
 *			   P_DEV,P_COMPAT
 *       Fixed Privileges: None
 * Notes:  This is the generic fsck.  Its main function is to set up the
 *	   options needed to exec the file system dependent fscks.  The
 * 	   privileges that it has are passed to the file system
 *	   dependent fsck's.  When the system is booting up, /sbin/fsck
 *	   needs P_MACREAD to access files (because MAC is not
 *	   initialized) and P_DEV to write to the console which is in
 *	   private state. 
 ***************************************************************************/

#include	<stdio.h>
#include	<errno.h>
#include	<limits.h>
#include	<sys/types.h>
#include	<sys/vfstab.h>
#include	<priv.h>
#include	<mac.h>
#include	<fcntl.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>
#include	<signal.h>
#include	<sys/mman.h>
#include	<locale.h>
#include	<ctype.h>
#include	<pfmt.h>

#define FSCK_FILE	"/.fsck"
#define FSCK_ID		'J'
#define	ARGV_MAX	16
#define	FSTYPE_MAX	8
#define	VFS_PATH	"/usr/lib/fs"
#define	VFS_PATH2	"/etc/fs"

#define	CHECK(xx, yy)\
	if (xx == (yy)-1) {\
		pfmt(stderr, MM_ERROR, maxopterr);\
		usage();\
	}

#define	OPTION(flag)\
		options++; \
		nargv[nargc++] = flag; \
		CHECK(nargc, ARGV_MAX);\
		break
#define	OPTARG(flag)\
		nargv[nargc++] = flag; \
		CHECK(nargc, ARGV_MAX);\
		if (optarg) {\
			nargv[nargc++] = optarg;\
			CHECK(nargc, ARGV_MAX);\
		}\
		break
		
#define SERIAL_FAIL	1

extern char	*optarg;
extern int	optind;
extern char	*strrchr();

int	status;
int	nargc = 2;
int	options = 0;
char	*nargv[ARGV_MAX];
char	*myname, *fstype;
char	maxopterr[] = ":317:too many arguments\n";
char	vfstab[] = VFSTAB;
char	Fsck_file[80];
int	Sigval = 0;

/* Option flags. */
int 	Pflg =0, wflg = 0, bflg = 0, Lflg = 0, yflg=0, preen=0, Vflg = 0;
int 	semid, diskcnt = 0;
int	child_stopcnt = 0, jobdone_cnt = 0;

struct fslist {
	char fsckpass[2];
	char fstype[8];
	char	dev[20];
	pid_t	fspid;
	boolean_t	done;
	struct fslist *next;
	struct fslist *prev;
};
struct dlist {
	int	fscnt;
	struct fslist	*fsltp_head;
	struct fslist	*fsltp_tail;
	struct dlist *next;
	struct dlist *prev;
};

struct dlist	*Parallel_listp;
struct fslist	*Serial_hlistp;	/* Head of "failed" jobs to be serialized */
struct fslist	*Serial_tlistp;	/* Tail of "failed" jobs to be serialized */

pid_t execute(char *, char *, FILE *, int);

#define LOUTBUF	512	/* the buffer size for output for -L option */	

/*
 *  Count number of children that need input for -L option
 */
void
child_stop_cnt(sig)
int	sig;
{
	signal(SIGUSR1, (void(*)(int))child_stop_cnt);
 	child_stopcnt++;	
	Sigval = sig;
}

/*
 * Procedure:     main
 *
 * Restrictions: fopen: P_MACREAD only when process has valid lid
 * 		 getvfsany: P_MACREAD only when process has valid lid
 * 		 getvfsent: P_MACREAD only when process has valid lid
 * 		 fclose: P_MACREAD only when process has valid lid
 * 		 rewind: P_MACREAD only when process has valid lid
 *
 * Notes: P_MACREAD should be restricted to prevent /sbin/fsck from
 *	  opening files in the wrong levels (in this case 
 *	  /etc/vfstab).  However, /sbin/fsck is also executed 
 *	  during system start up before MAC is initialized and the 
 *	  process does not have a valid lid.  Thus, it needs P_MACREAD
 *	  to access files that have lids.  Also, the getvfsany() lib
 *	  routine does stat's on devices in the vfstab entry, so
 *	  P_MACREAD is restricted so that the correct level devices are
 *	  used.  Like fopen(), getvfsany() needs P_MACREAD when process
 *	  does not have a lid.
 */
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	cc, ret;
	int	questflg = 0, Fflg = 0, sanity = 0;
	FILE	*fd;
	struct vfstab	vget, vref;
	struct dlist	*disklist();
	level_t level;		/* level identifier of process */
	pid_t	st, pid=1;
	key_t	keyid;
	union	semun {
		int val;
		struct semid_ds	*buf;
		ushort	*array;
	} arg;
	void	sigterm();
	
	char	label[NAME_MAX];

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxfsck");
	myname = strrchr(argv[0], '/');
	myname = (myname != 0)? myname+1: argv[0];
	sprintf(label, "UX:%s", myname);
	(void)setlabel(label);

	while ((cc = getopt(argc, argv, "PLbw?BDfF:lmnNo:pqs;S;t:T:VxyY")) != -1) {
		switch (cc) {
		case '?':
			questflg++;
			if (questflg > 1)
				usage();
			nargv[nargc++] = "-?";
			CHECK(nargc, ARGV_MAX);
			break;
		case 'B':
			OPTION("-B");
		case 'b':
			bflg++;
			OPTION("-b");
		case 'D':
			OPTION("-D");
		case 'f':
			OPTION("-f");
		case 'F':
			Fflg++;
			/* check for more than one -F */
			if (Fflg > 1) {
				pfmt(stderr, MM_ERROR,
					":318:more than one fstype specified\n");
				usage();
			}
			fstype = optarg;
			if (strlen(fstype) > FSTYPE_MAX) {
				pfmt(stderr, MM_ERROR,
					":319:Fstype %s exceeds %d characters\n",
						fstype, FSTYPE_MAX);
				exit(1);
			}
			break;
		case 'L':
			Lflg++;
			OPTION("-L");
		case 'l':
			OPTION("-l");
		case 'm':
			sanity++;
			OPTION("-m");
		case 'n':
			OPTION("-n");
		case 'N':
			OPTION("-N");
		case 'o':
			OPTARG("-o");
		case 'p':
			preen++;
			OPTION("-p");
		case 'P':
			Pflg++;
			OPTION("-P");
		case 'q':
			OPTION("-q");
		case 's':
			OPTARG("-s");
		case 'S':
			OPTARG("-S");
		case 't':
			OPTARG("-t");
		case 'T':
			OPTARG("-T");
		case 'V':
			Vflg++;
			if (Vflg > 1)
				usage();
			break;
		case 'x':
			OPTION("-x");
		case 'y':
		case 'Y':
			yflg++;
			OPTION("-y");
		case 'w':
			wflg++;
			OPTION("-w");
		}
		optarg = NULL;
	}

	/* copy '--' to specific */
	if (strcmp(argv[optind-1], "--") == 0) {
		nargv[nargc++] = argv[optind-1];
		CHECK(nargc, ARGV_MAX);
	}

	if (questflg) {
		if (Fflg) {
			nargc = 2;
			nargv[nargc++] = "-?";
			nargv[nargc] = NULL;
			do_exec(fstype, nargv);
		}
		usage();
	}

	if ((sanity) && (options > 1)) {
		usage();
	}

	if ((Lflg || wflg) && !Pflg)
		usage();

	/*
	 * If the process has a level and its level is valid, then
	 * clear P_MACREAD for opening of the /etc/vfstab file and
	 * for the exec of the file system dependent command. 
	 */
	if ((lvlproc(MAC_GET, &level) == 0) && (level != 0))
		procprivl(CLRPRV, MACREAD_W, 0);
			
	for (cc = 1; cc < MAXSIG; cc++) {
		switch (cc) {
		case SIGCLD:
			signal(cc, SIG_DFL);
			break;
		case SIGUSR1:
			if (Lflg)
				signal(cc, (void(*)(int))child_stop_cnt);
			else
				signal(cc, sigterm);
			break;
		default:
			signal(cc, sigterm);
			break;
		}
	}

	if (optind == argc) {
		if (fstype == NULL) {
			if ((argc > 3) && (sanity)) {
				usage();
			}
		}
		fd = fopen(vfstab, "r");
		if (fd == NULL) {
			pfmt(stderr, MM_ERROR,
				":320:cannot open vfstab\n");
			exit(1);
		}
		if (Pflg) {		/* parallel fsck */
			/* create a list of fs to be checked */
			Parallel_listp = disklist(fd);
			/* create semaphore to control output */
			sprintf(Fsck_file,"%s.%d",FSCK_FILE, getpid());
			if (open(Fsck_file, O_RDONLY|O_CREAT, 0444) < 0) {
				pfmt(stderr, MM_ERROR, ":363:Cannot open a semaphore file\n");
				exit(1);
			}
			if ((keyid = ftok(Fsck_file, FSCK_ID)) < 0) {
				pfmt(stderr, MM_ERROR, ":364:Cannot get a key for semaphore\n");
				exit(1);
			}
			if ((semid = semget(keyid, 1, IPC_CREAT)) < 0) {
				pfmt(stderr, MM_ERROR, ":365:Cannot create semaphore\n");
				exit(1);
			}
			/* Initialize the semaphore */
			arg.val = 1;
			if (semctl(semid, 0, SETVAL, arg) < 0) {
				pfmt(stderr, MM_ERROR, ":366:Cannot initialize the semaphore\n");	
				rmsem();
				exit(1);
			}
		        /* execute all file systems in parallel */
			pfsexec(fd);
			rmsem();
		} else {
			while ((ret = getvfsent(fd, &vget)) == 0)
				if (numbers(vget.vfs_fsckpass) &&
			    	vget.vfs_fsckdev != NULL &&
			    	vget.vfs_fstype != NULL &&
			   	(fstype == NULL ||
			    	strcmp(fstype, vget.vfs_fstype) == 0))
			 		(void)execute(vget.vfs_fsckdev, vget.vfs_fstype, fd, 0);
			fclose(fd);
			if (ret > 0)
				vfserror(ret);
		}
	} else {
		if (fstype == NULL) {
			fd = fopen(vfstab, "r");
			if (fd == NULL) {
				pfmt(stderr, MM_ERROR,
					":320:cannot open vfstab\n");
				exit(1);
			}
		}
		while (optind < argc) {
			if (fstype == NULL) {
				if ((argc > 3) && (sanity)) {
				usage();
				}
				/* must check for both special && raw devices */
				vfsnull(&vref);
				vref.vfs_fsckdev = argv[optind];
				if ((ret = getvfsany(fd, &vget, &vref)) == -1) {
					rewind(fd);
					vref.vfs_fsckdev = NULL;
					vref.vfs_special = argv[optind];
					ret = getvfsany(fd, &vget, &vref);
				}
				rewind(fd);
				if (ret == 0)
					(void)execute(argv[optind], vget.vfs_fstype, fd, 0);
				else if (ret == -1) {
					pfmt(stderr, MM_ERROR,
						":321:FSType cannot be identified\n");
				} else
					vfserror(ret);
			} else		/* -F option is specified */
				(void)execute(argv[optind], fstype, (FILE *)NULL, 0);
			optind++;
		}
		if (fstype == NULL)
			fclose(fd);
	}
	exit(status);
	/* NOTREACHED */
}

/* 
 * Create a linked list of all physical disks in the system.
 * Each disk list is a linked list of all file systems from the same disk.
 */
struct dlist *
disklist(fd)
FILE	*fd;
{
	struct fslist *fslistp;
	struct	dlist	*dlistp, *hdlistp =NULL, *ndltp;
	struct vfstab	vget, vref;
	int ret, fdbuf;

	while ((ret = getvfsent(fd, &vget)) == 0) {
		if ((vget.vfs_fsckpass != NULL) &&
		strcmp(vget.vfs_fsckpass, "0") == 0)
			continue;
		if (hdlistp == NULL) {		/* the first disk */
			hdlistp = (struct dlist *)malloc(sizeof (struct dlist));
			if (hdlistp == NULL)
				break;
			hdlistp->next = hdlistp->prev = NULL;
			hdlistp->fsltp_head = hdlistp->fsltp_tail = NULL;
			diskcnt++;
		}
		fslistp = (struct fslist *)malloc(sizeof (struct fslist));
		if (fslistp == NULL)
			break;
		strncpy(fslistp->fsckpass,vget.vfs_fsckpass,strlen(vget.vfs_fsckpass));
		strncpy(fslistp->fstype,vget.vfs_fstype,strlen(vget.vfs_fstype));
		strncpy(fslistp->dev,vget.vfs_fsckdev,strlen(vget.vfs_fsckdev));
		fslistp->next = NULL;
		fslistp->fspid = 0;
		fslistp->done = B_FALSE;
		if (hdlistp->fsltp_head == NULL) {	/* 1st file system */
			hdlistp->fsltp_head = fslistp;
			hdlistp->fsltp_tail = fslistp;
			hdlistp->fscnt++;
			continue;
		} else {
			for (dlistp = hdlistp; dlistp != NULL; dlistp = dlistp->next) {
				if (dlistp->fsltp_head->dev != fslistp->dev) {
					if (dlistp->next == NULL) {
						ndltp = (struct dlist *)malloc(sizeof (struct dlist));
						if (ndltp == NULL)
							break;
						ndltp->next = NULL;
						ndltp->prev = dlistp;
						ndltp->fscnt = 1;
						ndltp->fsltp_head = fslistp;
						ndltp->fsltp_tail = fslistp;
						dlistp->next = ndltp;
						++diskcnt;
						break;
					} else
						continue;
				} else { 
					dlistp->fsltp_tail->next = fslistp;
					dlistp->fscnt++;
					fslistp->prev = dlistp->fsltp_tail;
					dlistp->fsltp_tail = fslistp;
					break;
				} 
			}
		}
	}
	return(hdlistp);
}

/*
 * One cycle of the parallel check. It checks all different physical disks
 * in the system at the same time. If a job is completed successfully,
 * it's then removed from the fslist. If failed and wflg is on, it's recorded
 * in the Serial_list. If Lflg is on return to the caller. 
 */
pid_t
dispatch_job(fsjob, fd, serialize)
struct fslist	*fsjob;
FILE *fd;
int	serialize;
{
	struct dlist	*dlistp;
	struct	fslist	*fsh;
	struct	fslist	*tfsh1;
	struct	fslist	*tfsh2;
	pid_t	npid;
	int	st;
	int	cc;
	void	sigterm();

	if (fsjob == NULL) {
		dlistp = Parallel_listp;
		while (dlistp) {
			fsh = dlistp->fsltp_head;
			fsh->fspid = execute(fsh->dev, fsh->fstype, fd, 1);
			dlistp = dlistp->next;
		}
	} else
		fsjob->fspid = execute(fsjob->dev, fsjob->fstype, fd, 1);

	if (Lflg)
		return;

	/* Wait for all children. If one completed successfully, then
	 * it's removed from the flistp and next file system of that disk
	 * is started. If wflg is specified and a job failed, it's recorded
	 * in the Serial_list to recheck.
	 */
	while ((npid = wait(&st)) != (pid_t)-1) {
		dlistp = Parallel_listp;
		while (dlistp && dlistp->fsltp_head->fspid != npid)
			dlistp = dlistp->next;

		if (dlistp == NULL) {
			pfmt(stderr, MM_ERROR, ":367:dlistp NULL when it should not be\n");
			fflush(stderr);
		}
			
		statcheck(st, npid);
		fsh = dlistp->fsltp_head;
		if (fsh->next != NULL) {
			dlistp->fsltp_head = tfsh1 = fsh->next;
			
			if (status && (serialize == SERIAL_FAIL)) {
				if (!Serial_tlistp) {
					Serial_hlistp = fsh;
					Serial_tlistp = fsh;
				} else {
					Serial_tlistp->next = fsh;
					Serial_tlistp = fsh;
				}
				fsh->next = NULL;
			} else
				free(fsh);
			tfsh1->fspid = dispatch_job(tfsh1, fd, serialize);
		} else {
			if (status && (serialize == SERIAL_FAIL)) {
				if (!Serial_tlistp) {
					Serial_hlistp = fsh;
					Serial_tlistp = fsh;
				} else {
					Serial_tlistp->next = fsh;
					Serial_tlistp = fsh;
				}
			} else
				free(fsh);

			if (dlistp->next != NULL)
				dlistp->next->prev = dlistp->prev;
			if (dlistp->prev != NULL)
				dlistp->prev->next = dlistp->next;
			if (Parallel_listp == dlistp)
				Parallel_listp = dlistp->next;
			free(dlistp);
		}
	}
}

/*
 * One dispatch_job is call for all physicall disks in the system for each
 * cycle. It then wait until all jobs are "done" then printout the output.
 * If a job is exited but not completed sucessfully, it will be picked
 * by the parent where it was left (in printout()). 
 * If a job is completed sucessfully, the job is removed from the fslist.
 * Once all the jobs of a dispatch_job() is done the next one will start 
 * until all file systems are checked.
 */
void
do_minusL(fd)
FILE	*fd;
{
	int	st;
	pid_t	pid = 0;
	struct dlist	*dlistp;
	struct fslist	*fslp;

	while (Parallel_listp) {
		(void)dispatch_job(NULL, fd, 0);
		/* parent waits for children */
		do {
			pid = wait(&st);
			if (pid == (pid_t)-1 && errno == EINTR) {
				if (Sigval == SIGUSR1 || Sigval == SIGCLD) {
					Sigval = 0;
					continue;
				}
				pfmt(stderr, MM_ERROR, ":323:bad wait\n");
				pfmt(stderr, MM_NOGET|MM_ERROR, "%s\n", strerror(errno));
				fflush(stderr);
			} else if (pid == (pid_t)-1 && errno == ECHILD) {
				rmsem();
				exit(__LINE__);
			} else if (pid != -1) {
				dlistp = Parallel_listp;
				while (dlistp) {
					fslp = dlistp->fsltp_head;
					if (fslp->fspid == pid) {
						fslp->done = B_TRUE;
						break;
					}
					dlistp = dlistp->next;
				}
				if (dlistp == NULL) {
					pfmt(stderr, MM_ERROR, ":368:Orphan process\n");	
					fflush(stderr);
				}
				jobdone_cnt++;
			}
		} while ((child_stopcnt + jobdone_cnt) != diskcnt);

		printout();
		child_stopcnt = 0;
		jobdone_cnt = 0;
		/* remove checked file systems */
		for (dlistp = Parallel_listp; dlistp; dlistp = dlistp->next) {
			fslp = dlistp->fsltp_head;
			if (fslp->next) {
				/* Another job for this disk */
				dlistp->fsltp_head = fslp->next;
				free(fslp);
			} else {
				/* No more jobs for this disk */
				if (dlistp == Parallel_listp)
					Parallel_listp = dlistp->next;
				if (dlistp->prev)
					dlistp->prev->next = dlistp->next;
				if (dlistp->next)
					dlistp->next->prev = dlistp->prev;
			}
		}
	}
}

/*
 * Time to run all the failed children sequentially.
 * Need to remove the "-w" option so that they run correctly.
 */
void
remove_minus_w()
{
	int	cnt;

	for (cnt = 2; cnt < nargc, strcmp(nargv[cnt], "-w"); cnt++)
		;

	if (cnt == nargc) {
		/* oops, no "-w" in nargv! */
		pfmt(stderr, MM_ERROR, ":369:No -w option. Environment is corrupted\n");
		exit(1);
	}

	while (cnt < nargc) {
		nargv[cnt] = nargv[cnt+1];
		cnt++;
	}

	nargc--;
}

/*
 * pfsexec will loop until all file systems are checked.
 * Each time it check one file system from the head of each disk list. 
 */
pfsexec(fd)
FILE *fd;
{
	struct fslist	*fslp, *tfslp;

	if (Lflg) {	/* output is ordered by file systems */
		do_minusL(fd);
	} else if (wflg && !yflg) {
		(void)dispatch_job(NULL, fd, SERIAL_FAIL);

		if (Serial_hlistp) {
			fslp = Serial_hlistp;
			Serial_hlistp = NULL;
			Serial_tlistp = NULL;
			while (fslp) {
				remove_minus_w();
				(void)execute(fslp->dev, fslp->fstype, fd, 0);
				tfslp = fslp;
				fslp = fslp->next;
				free(tfslp);
			}
		}
	} else
		(void)dispatch_job(NULL, fd, 0);
}

/* See if all numbers */
numbers(yp)
	char	*yp;
{
	if (yp == NULL)
		return	0;
	while ('0' <= *yp && *yp <= '9')
		yp++;
	if (*yp)
		return	0;
	return	1;
}

/*
 * Procedure:	execute
 *
 * Notes:  Does a fork and do_exec of file system dependent fscks.
 */
pid_t
execute(fsckdev, fstype, fd, parallel)
	char	*fsckdev, *fstype;
	FILE	*fd;
	int	parallel;
{
	int	st;
	pid_t	fk;
	char	tmpbuf[10];

	nargv[nargc] = fsckdev;

	if (Vflg) {
		prnt_cmd(stdout, fstype);
		return;
	}

	if ((fk = fork()) == (pid_t)-1) {
		pfmt(stderr, MM_ERROR,
			":322:cannot fork.  Try again later\n");
		pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		exit(1);
	}

	if (fk == 0) {
		/* close the fd in the child only */
		if (fd)
			fclose(fd);

		/* Try to exec the fstype dependent portion of the fsck. */
		do_exec(fstype, nargv);
	} else {
		if (parallel)
			return(fk);

		/* parent waits for child */
		if (wait(&st) == (pid_t)-1) {
			pfmt(stderr, MM_ERROR, ":323:bad wait\n");
			pfmt(stderr, MM_NOGET|MM_ERROR,
				"%s\n", strerror(errno));
			fflush(stderr);
			exit(1);
		}
		statcheck(st,fk);
	}

	return(0);
}

statcheck(st, fk)
int	st;
pid_t	fk;
{
	if ((st & 0xff) == 0x7f) {
		pfmt(stderr, MM_WARNING, ":324:the following command (process %d) was stopped by signal %d\n", fk, (st >> 8) & 0xff);
		fflush(stderr);
		prnt_cmd(stderr, fstype);
		status = ((st >> 8) & 0xff) | 0x80;
	} else if (st & 0xff) {
		if (st & 0x80)
			pfmt(stderr, MM_WARNING, ":325:the following command (process %d) was terminated by signal %d\n\t\t and dumped core\n",
			fk, st & 0x7f);
		else
			pfmt(stderr, MM_WARNING, ":326:the following command (process %d) was terminated by signal %d\n", fk, st & 0x7f);
		fflush(stderr);
		prnt_cmd(stderr, fstype);
		status = ((st & 0xff) | 0x80);
	} else if (st & 0xff00)
		status = (st >> 8) & 0xff;
}
/*
 * Procedure:     do_exec
 *
 * Restrictions:  execv(2): P_MACREAD only when process has a valid lid
 *
 * Notes: P_MACREAD should be restricted to prevent /sbin/fsck from
 *	  exec'ing files in the wrong levels.  However, /sbin/fsck is
 * 	  also executed during system start up before MAC is
 *	  initialized and thus the process needs P_MACREAD to access
 *	  any files. 
 */
do_exec(fstype, nargv)
	char	*fstype, *nargv[];
{
	char	full_path[PATH_MAX];
	char	*vfs_path = VFS_PATH;

	if (strlen(fstype) > FSTYPE_MAX) {
		pfmt(stderr, MM_ERROR,
			":319:Fstype %s exceeds %d characters\n",
				fstype, FSTYPE_MAX);
		fflush(stderr);
		exit(1);
	}

	/* build the full pathname of the fstype dependent command. */
	sprintf(full_path, "%s/%s/%s", vfs_path, fstype, myname);

	/* set the new argv[0] to the filename */
	nargv[1] = myname;

	/* Try to exec the fstype dependent portion of the fsck. */
	execv(full_path, &nargv[1]);
	if (errno == EACCES) {
		pfmt(stderr, MM_ERROR,
			":327:cannot execute %s - permission denied\n",
				full_path);
		fflush(stderr);
	}
	if (errno == ENOEXEC) {
		nargv[0] = "sh";
		nargv[1] = full_path;
		execv("/sbin/sh", &nargv[0]);
	}
	/* second path to try */
	vfs_path = VFS_PATH2;
	/* build the full pathname of the fstype dependent command. */
	sprintf(full_path, "%s/%s/%s", vfs_path, fstype, myname);

	/* set the new argv[0] to the filename */
	nargv[1] = myname;
	/* Try to exec the second fstype dependent portion of the fsck. */
	execv(full_path, &nargv[1]);
	if (errno == EACCES) {
		pfmt(stderr, MM_ERROR,
			":327:cannot execute %s - permission denied\n",
				full_path);
		fflush(stderr);
		exit(1);
	}
	if (errno == ENOEXEC) {
		nargv[0] = "sh";
		nargv[1] = full_path;
		execv("/sbin/sh", &nargv[0]);
	}
	pfmt(stderr, MM_ERROR,
		":328:operation not applicable to FSType %s\n",fstype);
	fflush(stderr);
	exit(1);
}

/* Print command for -V option */
prnt_cmd(fd, fstype)
	FILE	*fd;
	char	*fstype;
{
	char	**argp;

	fprintf(fd, "%s -F %s", myname, fstype);
	for (argp = &nargv[2]; *argp; argp++)
		fprintf(fd, " %s", *argp);
	fprintf(fd, "\n");
}

/* 
 * Prints error from reading /etc/vfstab 
 */
vfserror(flag)
	int	flag;
{
	switch (flag) {
	case VFS_TOOLONG:
		pfmt(stderr, MM_ERROR,
			":329:line in vfstab exceeds %d characters\n",
				VFS_LINE_MAX-2);
		break;
	case VFS_TOOFEW:
		pfmt(stderr, MM_ERROR,
			":330:line in vfstab has too few entries\n");
		break;
	case VFS_TOOMANY:
		pfmt(stderr, MM_ERROR,
			":331:line in vfstab has too many entries\n");
		break;
	}
	exit(1);
}

/*
 * This getopt() routine is slightly different from that of the libc
 * getopt routine.  It allows for a new type of option argument
 * as specified by ";".
 */
extern int strcmp();
extern char *strchr();

int opterr = 1, optind = 1;
char *optarg;

getopt(argc, argv, opts)
int	argc;
char	**argv, *opts;
{
	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1)
		if(optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(-1);
		else if(strcmp(argv[optind], "--") == 0) {
			optind++;
			return(-1);
		}
	c = argv[optind][sp];
	if(c == ':' || (cp=strchr(opts, c)) == 0) {
		if (opterr)
			pfmt(stderr, MM_ERROR,
				":167:illegal option -- %c\n", c);
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			if (opterr)
				pfmt(stderr, MM_ERROR,
					":168:option requires an argument -- %c\n", c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else if (*cp == ';') {
		if (argv[optind][++sp] != '\0')
			if (isoptarg(c, &argv[optind][sp])) {
				optarg = &argv[optind++][sp];
				sp = 1;
			} else
				optarg = NULL;
		else {
			sp = 1;
			if (++optind >= argc || !isoptarg(c, &argv[optind][0]))
				optarg = NULL;
			else
				optarg = argv[optind++];
		}
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}

/* Used by above getopt() routine */
isoptarg(cc, arg)
	int	cc;
	char	*arg;
{
	if (cc == 's' || cc == 'S') {
		while(*arg >= '0' && *arg <= '9')
			arg++;
		if(*arg++ != ':')
			return	0;
		while(*arg >= '0' && *arg <= '9')
			arg++;
		if (*arg)
			return	0;
		return	1;
	}
	return	0;
}

usage()
{
	pfmt(stderr, MM_ACTION, ":332:Usage:\n%s [-P] [-m] [-b] [-L] [-F FSType] [-V] [-m] [special ...]\n%s [-P] [-m] [-b] [-L] [-F FSType] [-V] [current_options] [-o specific_options] [special ...]\n", myname, myname);

	exit(1);
}

void
sigterm(sig)
int	sig;
{

	if (Pflg) 
		rmsem();
	exit(1);
}

rmsem()
{
	union	semun {
		int val;
		struct semid_ds	*buf;
		ushort	*array;
	} arg;

	arg.val = 1;
	if (semctl(semid, 0, IPC_RMID, arg) < 0)
		exit(1);
	unlink(Fsck_file);
}

/*
 * Print output of file systems in the ordered they are in /etc/vfstab.
 */
printout()
{
	struct	dlist	*dlistp;
	struct	dlist	*tdlistp;
	struct fslist	*fslp;
	struct fslist	*tfslp;
	int	st, cnt;
	char    buf[512];
        char    filename[20];   /* "max" length of name "/.fsck.L.<pid>" */
	pid_t	pid;
	int	fd;
	
	for (dlistp = Parallel_listp; dlistp; dlistp = dlistp->next) {
		fslp = dlistp->fsltp_head;
		sprintf(filename, "/.fsck.L.%d", fslp->fspid);
		if ((fd = open(filename, O_RDONLY,0444)) < 0 ) {
			rmsem();
			pfmt(stderr, MM_ERROR, ":284:Cannot open %s\n", filename);
			exit(1);
		}
		if ((cnt = read(fd, buf, LOUTBUF - 1)) < 0){
			rmsem();
			pfmt(stderr, MM_ERROR, ":370:Cannot read %s\n", filename);
			exit(1);
		}
		if (write(1, buf, cnt) < 0){
			rmsem();
			pfmt(stderr, MM_ERROR, ":371:Cannot write %s\n", filename);
			exit(1);
		}
		unlink(filename);
		if (fslp->done == B_TRUE)
			continue;

		/* Wait for a child had to stop for input */
		while ((pid = wait(&st)) != fslp->fspid) {
			if (pid == (pid_t)-1 && errno == EINTR) {
				if (Sigval == SIGUSR1) {
					Sigval = 0;
					continue;
				}
				pfmt(stderr, MM_ERROR, ":323:bad wait\n");
				pfmt(stderr, MM_NOGET|MM_ERROR, "%s\n", strerror(errno));
				fflush(stderr);
				continue;
			}
			tdlistp = Parallel_listp;
			while (tdlistp) {
				tfslp = tdlistp->fsltp_head;
				if (tfslp->fspid == pid) {
					tfslp->fspid = B_TRUE;
					break;
				}
				tdlistp = tdlistp->next;
			}
			if (tdlistp == NULL) {
				pfmt(stderr, MM_ERROR, ":368:Orphan process\n");
				fflush(stderr);
			}
		}
	}
}
