/*		copyright	"%c%" 	*/

#ident	"@(#)uucp.c	1.3"
#ident "$Header$"
/* test*/

#include "uucp.h"
#include <wait.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>

/*
 * uucp
 * user id 
 * make a copy in spool directory
 */
int Copy = 0;
static int _Transfer = 0;
static int Total;
char Muser[512];
char Nuser[32];
int jflag = 0;
int rflag = 0;
char Optns[10];
char Uopts[BUFSIZ];
char *gflag = NULL;
char Sgrade[NAMESIZE];
int Mail = 0;
int Notify = 0;
static char posix_var[]="POSIX2";
static int posix2 = 0;

void cleanup(), ruux(), usage(), copy();
int guinfo(), vergrd(), gwd(), ckexpf(), uidstat(), uidxcp(),
	gtcfile();
void commitall(), wfabort(), mailst(), gename(), svcfile();

char	Sfile[MAXFULLNAME];

main(argc, argv, envp)
char *argv[];
char	**envp;
{
	char *jid();
	int	ret;
	char	*fopt, *s;
	char	sys1[MAXFULLNAME], sys2[MAXFULLNAME];
	char	fwd1[MAXFULLNAME], fwd2[MAXFULLNAME];
	char	file1[MAXFULLNAME], file2[MAXFULLNAME];
	static char emsg1[] = "No administrator defined service grades available on this machine.";
	static char emsg2[] = "UUCP service grades range from [A-Z][a-z] only.";
	extern int	split();

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxbnu.abi");
	(void)setlabel("UX:uucp");

	if (getenv(posix_var) != NULL)
		posix2 = 1;

	/* this fails in some versions, but it doesn't hurt */
	Uid = getuid();
	Euid = geteuid();
	Gid = getgid();
	Egid = getegid();

	/* choose LOGFILE */
	(void) strcpy(Logfile, LOGUUCP);

	Env = envp;
	fopt = NULL;
	(void) strcpy(Progname, "uucp");
	Pchar = 'U';
	*Uopts = NULLCHAR;
	gflag = NULL;
	*Sgrade = NULLCHAR;

	if (eaccess(GRADES, F_OK) != -1) {
		Grade = 'A';
		Sgrades = TRUE;
		sprintf(Sgrade, "%s", "default");
	}

	/*
	 * find name of local system
	 */
	uucpname(Myname);
	Optns[0] = '-';
	Optns[1] = 'd';
	Optns[2] = 'c';
	Optns[3] = Nuser[0] = Sfile[0] = NULLCHAR;

	/*
	 * find id of user who spawned command to 
	 * determine
	 */
	(void) guinfo(Uid, User);

	/*
	 * Set mail address if being called from uux
	 */
	if ((s = getenv("UU_USER")) != NULL)
		strncpy(Muser, s, sizeof(Muser));
	else
		strncpy(Muser, User, sizeof(Muser));

	/*
	 * create/append command log
	 */
	commandlog(argc,argv);

	while ((ret = getopt(argc, argv, "Ccdfg:jmn:rs:x:w")) != EOF) {
		switch (ret) {

		/*
		 * make a copy of the file in the spool
		 * directory.
		 */
		case 'C':
			Copy = 1;
			Optns[2] = 'C';
			break;

		/*
		 * not used (default)
		 */
		case 'c':
			break;

		/*
		 * not used (default)
		 */
		case 'd':
			break;
		case 'f':
			Optns[1] = 'f';
			break;

		/*
		 * set service grade
		 */
		case 'g':
			gflag = optarg;
			if (!Sgrades) {
				if (strlen(optarg) < (size_t)2 && isalnum(*optarg)) 
					Grade = *optarg;
				else {
					(void) fprintf(stderr, "%s\n%s\n",
						gettxt(":1",emsg1), gettxt(":2",emsg2));
					exit(4);
				}
			}
			else {
				(void) strncpy(Sgrade, optarg, NAMESIZE-1);
				Sgrade[NAMESIZE-1] = NULLCHAR;
				if ((ret = vergrd(Sgrade)) != SUCCESS)
					exit(ret); 
			}
			break;

		case 'j':	/* job id */
			jflag = 1;
			break;

		/*
		 * send notification to local user
		 */
		case 'm':
			Mail = 1;
			(void) strcat(Optns, "m");
			/*
			 * Only add -m flag to uucp command executed by
			 * uux if POSIX2 set - could cause interoperability
			 * problems.
			 */
			if (posix2)
				(void) sprintf(Uopts + strlen(Uopts), " -m ");
			break;

		/*
		 * send notification to user on remote
		 * if no user specified do not send notification
		 */
		case 'n':
			Notify = 1;
			(void) strcat(Optns, "n");
			(void) sprintf(Nuser, "%.8s", optarg);
			(void) sprintf(Uopts+strlen(Uopts), "-n%s ", Nuser);
			break;

		/*
		 * create JCL files but do not start uucico
		 */
		case 'r':
			rflag++;
			break;

		/*
		 * return status file
		 */
		case 's':
			fopt = optarg;
			/* "m" needed for compatability */
			(void) strcat(Optns, "mo");
			break;

		/*
		 * create new files with incremented suffixes
		 */
		case 'w':
			(void) strcat(Optns, "F");
			break;

		/*
		 * turn on debugging
		 */
		case 'x':
			Debug = atoi(optarg);
			if (Debug <= 0)
				Debug = 1;
#ifdef SMALL
			(void) pfmt(stderr, MM_WARNING,
			":3:uucp built with SMALL flag defined -- no debug info available\n");
#endif /* SMALL */
			break;

		default:
			usage();
			break;
		}
	}
	DEBUG(4, "\n\n** %s **\n", "START");
	if (gwd(Wrkdir) == FAIL) {
		(void) pfmt(stderr,MM_ERROR,":7:Can not determine current directory\n");
		exit(54);
	}
	if (fopt) {
		if (*fopt != '/')
			(void) sprintf(Sfile, "%s/%s", Wrkdir, fopt);
		else
			(void) sprintf(Sfile, "%s", fopt);

	}
	else
		strcpy (Sfile, "dummy");

	/*
	 * work in WORKSPACE directory
	 */
	ret = CHDIR(WORKSPACE);
	if (ret != 0) {
		(void) pfmt(stderr, MM_ERROR, ":4:<%s>: no work directory - get help\n", WORKSPACE);
		exit(7);
	}

	if (Nuser[0] == NULLCHAR)
		(void) strcpy(Nuser, User);
	(void) strcpy(Loginuser, User);
	DEBUG(4, "UID %ld, ", (long) Uid);
	DEBUG(4, "User %s\n", User);
	if (argc - optind < 2) {
		usage();
	}

	/*
	 * set up "to" system and file names
	 */

	(void) split(argv[argc - 1], sys2, fwd2, file2);
	if (*sys2 != NULLCHAR) {
		if (versys(sys2) != 0) {
			(void) pfmt(stderr, MM_ERROR, ":5:<%s>: bad system name\n", sys2);
			exit(8);
		}
	}
	else
		(void) strcpy(sys2, Myname);

	(void) strncpy(Rmtname, sys2, MAXBASENAME);
	Rmtname[MAXBASENAME] = NULLCHAR;

	DEBUG(9, "sys2: %s, ", sys2);
	DEBUG(9, "fwd2: %s, ", fwd2);
	DEBUG(9, "file2: %s\n", file2);

	/* Load Permissions file. */
	(void) mchFind(sys2);

	/*
	 * if there are more than 2 argsc, file2 is a directory
	 */
	if (argc - optind > 2)
		(void) strcat(file2, "/");

	/*
	 * do each from argument
	 */

	Total = argc - 1 - optind;

	for ( ; optind < argc - 1; optind++) {

	    (void) split(argv[optind], sys1, fwd1, file1);

	    if (*sys1 != NULLCHAR) {
		if (versys(sys1) != 0) {
			(void) pfmt(stderr, MM_ERROR, ":5:<%s>: bad system name\n", sys1);
			cleanup(8);
		}
	    }

	    /*  source files can have at most one ! */
	    if (*fwd1 != NULLCHAR) {
		/* syntax error */
		(void) pfmt(stderr, MM_ERROR, ":6:<%s>: cannot have more than one ! in source file\n", argv[optind]);
	        cleanup(11);
	    }

	    /*
	     * check for required remote expansion of file names -- generate
	     *	and execute a uux command
	     * e.g.
	     *		uucp   owl!~/dan/*   ~/dan/
	     *
	     * NOTE: The source file part must be full path name.
	     *  If ~ it will be expanded locally - it assumes the remote
	     *  names are the same.
	     */

	    if (*sys1 != NULLCHAR)
		if ((strchr(file1, '*') != NULL
		      || strchr(file1, '?') != NULL
		      || strchr(file1, '[') != NULL)) {
		        /* do a uux command */
		        if (ckexpf(file1) == FAIL) {
			    cleanup(12);
			}
		        ruux(sys1, sys1, file1, sys2, fwd2, file2);
		        continue;
		}

	    /*
	     * check for forwarding -- generate and execute a uux command
	     * e.g.
	     *		uucp uucp.c raven!owl!~/dan/
	     */

	    if (*fwd2 != NULLCHAR) {
	        ruux(sys2, sys1, file1, "", fwd2, file2);
	        continue;
	    }

	    /*
	     * check for both source and destination on other systems --
	     *  generate and execute a uux command
	     */

	    if (*sys1 != NULLCHAR )
		if ( (!EQUALS(Myname, sys1))
	    	  && (!EQUALS(sys2, Myname)) ) {
		    ruux(sys2, sys1, file1, "", fwd2, file2);
	            continue;
	        }


	    if (*sys1 == NULLCHAR)
		(void) strcpy(sys1, Myname);
	    else {
		(void) strncpy(Rmtname, sys1, MAXBASENAME);
		Rmtname[MAXBASENAME] = NULLCHAR;
	    }

	    /*
	     * Check for both source and destination local and
	     * -j -r flags specified -- generate and execute
	     * a uux command.
	     */
	    if (EQUALS(Myname, sys1) &&
		EQUALS(Myname, sys2) &&
		(jflag || rflag)) {
		    if (ckexpf(file1) == FAIL) {
			    cleanup(12);
		    }
		    ruux(sys1, sys1, file1, sys2, fwd2, file2);
		    continue;
	    }

	    DEBUG(4, "sys1 - %s, ", sys1);
	    DEBUG(4, "file1 - %s, ", file1);
	    DEBUG(4, "Rmtname - %s\n", Rmtname);
	    copy(sys1, file1, sys2, file2); 
	}

	/* move the work files to their proper places */
	commitall(jflag);

	/*
	 * do not spawn daemon if -r option specified and
	   if there are no files for transfer
	 */
	if ((!rflag) && (_Transfer > 0)) {
		long	limit;
		char	msg[100];
		limit = ulimit(1, (long) 0);
		if (limit < MINULIMIT)  {
			(void) sprintf(msg,
			    "ULIMIT (%ld) < MINULIMIT (%ld)", limit, MINULIMIT);
			logent(msg, "Low-ULIMIT");
		}
		else
			xuucico(Rmtname);
	}

	if (Total > _Transfer)
		cleanup(22);
	else
		cleanup(0);
	/* NOT REACHED */
}

/*
 * cleanup lock files before exiting
 */
void
cleanup(code)
register int	code;
{

	rmlock(CNULL);

	if (code == 0)
		exit(0);

	wfabort();	/* this may be extreme -- abort all work */

	if (code == 22)
		(void) pfmt(stderr, MM_ERROR,
		":8:failed with <%d> file(s) sent and <%d> error(s)\n",
		 _Transfer, Total - _Transfer);

	exit(code);
}
/*
 * generate copy files for s1!f1 -> s2!f2
 *	Note: only one remote machine, other situations
 *	have been taken care of in main.
 */

void
copy(s1, f1, s2, f2)
char *s1, *f1, *s2, *f2;
{
	FILE *cfp;
	static FILE *syscfile();
	struct stat stbuf, stbuf1, stdir;
	int type, statret;
	char dfile[NAMESIZE];
	char cfile[NAMESIZE];
	char file1[MAXFULLNAME], file2[MAXFULLNAME];
	char msg[BUFSIZ];

	type = 0;
	(void) strcpy(file1, f1);
	(void) strcpy(file2, f2);
	if (!EQUALS(s1, Myname))
		type = 1;
	if (!EQUALS(s2, Myname))
		type = 2;

	switch (type) {
	case 0:

		/*
		 * all work here. local copy
		 */
		DEBUG(4, "all work here %d\n", type);

		/*
		 * check access control permissions
		 */
		if (ckexpf(file1))
			 return;
		if (ckexpf(file2))
			 cleanup(12);
		if (uidstat(file1, &stbuf) != 0) {
			(void) pfmt(stderr, MM_ERROR,
			    ":9:<%s>: cannot get file status\n", 
				file1);
			return;
		}
		statret = uidstat(file2, &stbuf1);
		if (statret == 0
		  && stbuf.st_ino == stbuf1.st_ino
		  && stbuf.st_dev == stbuf1.st_dev) {
			(void) pfmt(stderr, MM_ERROR,
			    ":10:<%s> <%s>: same file; cannot copy\n", file1, file2);
			return;
		}

		if (chkperm(file1, file2, strchr(Optns, 'd')) ) {
			(void) pfmt(stderr, MM_ERROR, ":11:<%s>,<%s>: permission denied\n",file1,file2);
			cleanup(15);
		}
		if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
			(void) pfmt(stderr, MM_ERROR, ":12:<%s>: source file cannot be a directory\n", file1);
			return;
		}
		/* see if I can read this file as real uid, gid */
		if ( access(file1, R_OK) ) {
			(void) pfmt(stderr, MM_ERROR,
			    ":13:<%s>: cannot read file\n", file1);
			return;
		}

		/* if the target file exists, see if I can	*/
		/* write it as real uid, gid			*/

		if (statret == 0 && (stbuf1.st_mode & (S_IWUSR|S_IWGRP)) == 0) {
			(void) pfmt(stderr, MM_ERROR,
			    ":15:<%s>: cannot write file with mode <%lo>\n",
			    file2, (long) stbuf1.st_mode);
			return;
		}

		/*
		 * copy file locally
		 */
		DEBUG(2, "local copy:  uidxcp(%s, ", file1);
		DEBUG(2, "%s\n", file2);

		/* Check if the parent directory is private to uucp */
		if (uucp_private(file2, &stdir)) {
			(void) pfmt(stderr, MM_ERROR, ":14:<%s>: permission denied\n",file2);
			return;
		}

		/* do copy as Uid, Gid; but the file will be owned by uucp */
		if (uidxcp(file1, file2) == FAIL) {
			(void) pfmt(stderr, MM_ERROR,
			    ":16:<%s>: cannot copy file, errno=<%d>\n", file2, errno);
			return;
		}
		/*
		 * doing ((mode & 0777) | 0666) so that file mode will be
		 * 666, but setuid bit will NOT be preserved. if do only
		 * (mode | 0666), then a local uucp of a setuid file will 
		 * create a file owned by uucp with the setuid bit on
		 * (lesson 1: how to give away your Systems file . . .).
		 */
		chmod(file2, (int) (((stbuf.st_mode & LEGALMODE) | PUB_FILEMODE)) & stdir.st_mode);
		chown(file2, stdir.st_uid, stdir.st_gid);
		/*
		 * if user specified -m, notify "local" user
		 */
		 if ( Mail ) {
		 	sprintf(msg,
		 	"REQUEST: %s!%s --> %s!%s (%s)\n(SYSTEM %s) copy succeeded\n",
		 	s1, file1, s2, file2, Muser, s2 );
		 	mailst(Muser, msg, "", "");
		}
		/*
		 * if user specified -n, notify "remote" user
		 */
		if ( Notify ) {
			sprintf(msg, "%s from %s!%s arrived\n",
				file2, s1, Muser );
			mailst(Nuser, msg, "", "");
		}
		break;
	case 1:

		/*
		 * receive file
		 */
		DEBUG(4, "receive file - %d\n", type);

		/*
		 * expand source and destination file names
		 * and check access permissions
		 */
		if (file1[0] != '~')
			if (ckexpf(file1))
				 return;
		if (ckexpf(file2))
			 cleanup(12);


		gename(DATAPRE, s2, Grade, dfile);

		/*
		 * insert JCL card in file
		 */
		cfp = syscfile(cfile, s1);
		(void) fprintf(cfp, 
	       	"R %s %s %s %s %s %o %s %s\n", file1, file2,
			User, Optns,
			*Sfile ? Sfile : "dummy",
			0777, Nuser, dfile);
		(void) fclose(cfp);
		(void) sprintf(msg, "%s!%s --> %s!%s", Rmtname, file1,
		    Myname, file2);
		logent(msg, "QUEUED");
		break;
	case 2:

		/*
		 * send file
		 */
		if (ckexpf(file1))
			 return;
		/* XQTDIR hook enables 3rd party uux requests (cough) */
		if (file2[0] != '~' && !EQUALS(Wrkdir, XQTDIR))
			if (ckexpf(file2))
				 cleanup(12);
		DEBUG(4, "send file - %d\n", type);

		if (uidstat(file1, &stbuf) != 0) {
			(void) pfmt(stderr, MM_ERROR,
			    ":17:<%s>: cannot get file status\n", file1);
			return;
		}
		if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
			(void) pfmt(stderr, MM_ERROR,
			    ":18:<%s>: source file cannot be a directory\n", file1);
			return;
		}
		/* see if I can read this file as read uid, gid */
		if ( access(file1, R_OK) ) {
			(void) pfmt(stderr, MM_ERROR,
			    ":19:<%s>: cannot read file\n", file1);
			return;
		}

		/*
		 * make a copy of file in spool directory
		 */

		gename(DATAPRE, s2, Grade, dfile);

		if (Copy || !READANY(file1) ) {

			if (uidxcp(file1, dfile)) {
			(void) pfmt(stderr, MM_ERROR,
			    ":22:<%s>: cannot copy file, errno=<%d>\n", file1, errno);
			    return;
			}

			(void) chmod(dfile, DFILEMODE);
		}

		cfp = syscfile(cfile, s2);
		(void) fprintf(cfp, "S  %s %s %s %s %s %lo %s %s\n",
		    file1, file2, User, Optns, dfile,
		    (long) stbuf.st_mode & LEGALMODE, Nuser, Sfile);
		(void) fclose(cfp);
		(void) sprintf(msg, "%s!%s --> %s!%s", Myname, file1,
		    Rmtname, file2);
		logent(msg, "QUEUED");
		break;
	}
	_Transfer++;
}


/*
 *	syscfile(file, sys)
 *	char	*file, *sys;
 *
 *	get the cfile for system sys (creat if need be)
 *	return stream pointer
 *
 *	returns
 *		stream pointer to open cfile
 *		
 */

static FILE	*
syscfile(file, sys)
char	*file, *sys;
{
	FILE	*cfp;

	if (gtcfile(file, sys) == FAIL) {
		gename(CMDPRE, sys, Grade, file);
		ASSERT(access(file, F_OK) != 0, Fl_EXISTS, file, errno);
		cfp = fdopen(creat(file, CFILEMODE), "w");
		(void)chmod(file, CFILEMODE);
		svcfile(file, sys, Sgrade);
	} else
		cfp = fopen(file, "a");
	ASSERT(cfp != NULL, Ct_OPEN, file, errno);
	return(cfp);
}


/*
 * generate and execute a uux command
 */

void
ruux(rmt, sys1, file1, sys2, fwd2, file2)
char *rmt, *sys1, *file1, *sys2, *fwd2, *file2;
{
    char cmd[BUFSIZ];
    char xcmd[BUFSIZ];
    char * xarg[8];
    int narg = 0;
    int i;
    pid_t chpid;

    xarg[narg++] = UUX;
    xarg[narg++] = "-C";
    if (gflag) {
	xarg[narg++] = "-g";
	xarg[narg++] = gflag;
    }
    if (jflag)
	xarg[narg++] = "-j";
    if (rflag)
	xarg[narg++] = "-r";

    (void) sprintf(cmd, "%s!uucp -C", rmt);

    if (*Uopts != NULLCHAR)
	(void) sprintf(cmd+strlen(cmd), " (%s) ", Uopts);

    if (*sys1 == NULLCHAR || EQUALS(sys1, Myname)) {
        if (ckexpf(file1))
  	    return;
	(void) sprintf(cmd+strlen(cmd), " %s!%s ", sys1, file1);
    }
    else
	if (!EQUALS(rmt, sys1))
	    (void) sprintf(cmd+strlen(cmd), " (%s!%s) ", sys1, file1);
	else
	    (void) sprintf(cmd+strlen(cmd), " (%s) ", file1);

    if (*fwd2 != NULLCHAR) {
	if (*sys2 != NULLCHAR)
	    (void) sprintf(cmd+strlen(cmd),
		" (%s!%s!%s) ", sys2, fwd2, file2);
	else
	    (void) sprintf(cmd+strlen(cmd), " (%s!%s) ", fwd2, file2);
    }
    else {
	if (*sys2 == NULLCHAR || EQUALS(sys2, Myname))
	    if (ckexpf(file2))
		cleanup(12);
	(void) sprintf(cmd+strlen(cmd), " (%s!%s) ", sys2, file2);
    }

    xarg[narg++] = cmd;
    xarg[narg++] = (char *) 0;

    xcmd[0] = NULLCHAR;
    for (i=0; i < narg; i++) {
	if ( xarg[i] != CNULL ) {
		strcat(xcmd, xarg[i]);
		strcat(xcmd, " ");
	}
    }
    DEBUG(2, "cmd: %s\n", xcmd);
    logent(xcmd, "QUEUED");

    switch (chpid = fork()) {
    case 0:
        ASSERT(setuid(getuid()) == 0, "setuid", "failed", 99);
	execv(UUX, xarg);
	fprintf(stderr,"uucp: cannot execute <%s>\n",UUX);
	exit(9);
    case -1:
	fprintf(stderr,"uucp: fork failed, errno=<%d>\n",errno);
	return;
    default:
	for (;;) {
		int ret, status;

		if ((ret = wait(&status)) == -1) {
			if (errno != EINTR)
				break;
		} else {
			if (ret == chpid)
			{
				if (WIFEXITED(status)) {
					ret = WEXITSTATUS(status);
					DEBUG(5, "Child exited with exit code <%d>\n",ret);
					if (ret == 0)
						_Transfer++;
					return;
				}
				if (WIFSIGNALED(status)) {
					ret = WTERMSIG(status);
					DEBUG(5, "Child received signal (%d)\n", ret);
					return;
				}

			}
		}
	}
    }
}

void
usage()
{

	(void) pfmt(stderr, MM_ACTION,
	":20:Usage: uucp [-c|-C] [-d|-f] [-gGRADE] [-j] [-m] [-nUSER]\n");
	(void) pfmt(stderr, MM_NOSTD,
	":21:\t\t[-r] [-sFILE] [-w] [-xDEBUG_LEVEL] source-files destination-file\n");
	exit(10);
}
