/*		copyright	"%c%" 	*/

/* 	Portions Copyright(c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)cron:common/cmd/cron/at.c	1.12.8.8"

/***************************************************************************
 * Command: at
 * Inheritable Privileges: P_MACREAD,P_DACREAD,P_MACWRITE,P_DACWRITE
 *       Fixed Privileges: None
 *
 *
 * Files used:
 *	drwxrwx--- root cron  SYS_PUBLIC /etc/cron.d
 *	-rw-rw-r-- root cron  SYS_PUBLIC /etc/cron.d/.proto
 *	prw-rw---- root cron  SYS_PUBLIC /etc/cron.d/NPIPE
 *	-rw-rw-r-- root cron  SYS_PUBLIC /etc/cron.d/at.allow
 *	-rw-rw-r-- root cron  SYS_PUBLIC /etc/cron.d/at.deny
 *
 *	drwxrwx--- root cron  SYS_PUBLIC /var/spool/cron
 *	drwxrwxrwt root cron  SYS_PUBLIC /var/spool/cron/atjobs
 *	-rw-------  usr  grp             /var/spool/cron/atjobs/<at file>
 *
 * Notes:
 *	1. at runs as group "cron" only to access /etc/cron.d and
 *	   /var/spool/cron, and to write a message to the NPIPE file.
 *	   At all other times, at runs with the invoking user's group.
 *	2. with enhanced security installed, atjobs is expected to
 *	   be a multi-level directory.
 *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <locale.h>
#include <limits.h>
#include <priv.h>
#include <mac.h>
#include <pfmt.h>
#include <string.h>
#include "cron.h"

extern	int	getdate_err;

#define TMPFILE		"_at"	/* prefix for temporary files	*/
#define ATMODE		04600	/* Mode for creating files in ATDIR.
Setuid bit on so that if an owner of a file gives that file 
away to someone else, the setuid bit will no longer be set.  
If this happens, atrun will not execute the file	*/
#define BUFSIZE		512	/* for copying files */
#define LINESIZE	130	/* for listing jobs */
#define	MAXTRYS		100	/* max trys to create at job file */

static const char
	BADDATE[] = 	":4:Bad date specification\n",
	BADFIRST[] =	":5:Bad first argument\n",
	BADHOURS[] =	":6:Hours field is too large\n",
	BADMD[] =	":7:Bad month and day specification\n",
	BADMINUTES[] =	":8:Minutes field is too large\n",
	BADSHELL[] =	":9:Because your login shell is not /usr/bin/sh, you cannot use %s\n",
	WARNSHELL[] =	":1084:Commands will be executed using %s\n",
	CANTCD[] =	":11:Cannot change directory to the \"%s\" directory: %s\n",
	CANTCHMOD[] =	":584:Cannot change the mode of your job\n",
	CWDERR[] =	":585:Cannot determine current directory\n",
	CWDLONG[] =	":586:Current directory pathname too long\n",
	LPMERR[] =	":587:Process terminated to enforce least privilege\n",
	NOLVLPROC[] =	":588:lvlproc() failed\n",
	STATNOPEN[] =	":589:Attributes changed between stat() and open()\n",
	CANTCREATE[] =	":13:Cannot create a job for you\n",
	INVALIDUSER[] =	":14:You are not a valid user (no entry in /etc/passwd)\n",
	NONUMBER[] =	":15:Proper syntax is:\n\tat -ln\nwhere n is a number\n",
	NORNUMBER[] =	":748:Proper syntax is:\n\tat -r job [job ...]\nwhere job is a job number\n",
	NODNUMBER[] =	":749:Proper syntax is:\n\tat -d job [job ...]\nwhere job is a job number\n",
	NOOPENDIR[] =	":16:Cannot open the at directory: %s\n",
	NOTALLOWED[] =	":17:You are not authorized to use %s.  Sorry.\n",
	NOTHING[] =	":18:Nothing specified\n",
	PAST[] =	":19:It is past that time\n",
	NOTOWNER[] =	":20:You do not own %s\n",
 	BADSTAT[] = 	":690:Job number %s does not exist\n",
	BADUSAGE[] = 	":1:Incorrect usage\n",
	INVALIDJOB[] = 	":34:Invalid job name %s\n",
	BADSTATAT[] =	":32:Cannot access spooling directory for at: %s\n",
	USEREQ[] = 	":590:user = %s\t%s\t%s",
	NOMEMLVL[] =	":591:Not enough memory to print level for job %s\n",
	BADTRANSLATE[] =":592:Cannot translate level to text format for job %s\n";

static const char FORMAT[] = "%a %b %e %H:%M:%S %Y";
static const char FORMATID[] = ":22";

#define CMDCLASS	"UX:"	/* Command classification */

static char posix_var[] = "POSIX2";
static int posix;

/*
	this data is used for parsing time
*/
#define	dysize(y) \
	(((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0) ? 366 : 365)
int	gmtflag = 0;
int	dflag = 0;
extern	time_t	timezone;
extern	char	*argp;
char	login[UNAMESIZE + 1];
char	argpbuf[BUFSIZ];
char	pname[80];
char	pname1[80];
char	argbuf[80];
time_t	when, now, gtime();
struct	tm	*tp, at, rt, *localtime();
int	mday[12] =
{
	31,38,31,
	30,31,30,
	31,31,30,
	31,30,31,
};
int	mtab[12] =
{
	0,   31,  59,
	90,  120, 151,
	181, 212, 243,
	273, 304, 334,
};
int     dmsize[12] = {
	31,28,31,30,31,30,31,31,30,31,30,31};

struct	tm	*ct;
char	timebuf[80];
/*
 * Error in getdate(3G)
 */
static char 	*errlist[] = {
/* 	0	*/ 	"",
/*	1	*/	":23:getdate: The DATEMSK environment variable is not set\n",
/*	2	*/	":24:getdate: Cannot open the template file\n",
/*	3	*/	":25:getdate: Cannot access the template file\n",
/*      4	*/	":26:getdate: The template file is not a regular file\n",
/*	5	*/	":27:getdate: An error is encountered while reading the template\n",
/*	6 	*/	":28:getdate: Out of memory\n",
/*	7	*/	":29:getdate: There is no line in the template that matches the input\n",
/*	8	*/	":30:getdate: Invalid input specification\n"
};

short	jobtype = ATEVENT;		/* set to 1 if batch job */
char	*tfname;
extern char *malloc();
extern char *xmalloc();
extern int   per_errno;
extern void  exit();
time_t	num();
int	mailflag;
extern char *getcwd();
char	cwdname[PATH_MAX+2];
gid_t	rgid, egid;
char	*getfname();
level_t	effdirlid();
void	list_dir();
int lvl=0;	/* 1 if -[zZ] supported */
static time_t gtime_posix(const char *);



/*
 * Procedure:     main
 *
 * Restrictions:
                 fopen: None
                 fgets: None
                 getdate: None
                 setlocale: None
                 stat(2): None
                 access(2): None
                 chdir(2): None
                 unlink(2): None
                 pfmt: None
                 strerror: None
                 opendir: None
                 ascftime: None
                 gettxt: None
                 localtime: None
                 getpwuid: none
                 printf: None
                 lvlin: None
                 lvlout: None
                 getcwd: None
                 mktime: None
                 sprintf: None
                 open(2): None
                 chmod(2): None
                 fflush: None
                 rename(2): None
                 cftime: None

 * Notes:
 * print level information
 *	default: -1: don't print level
 *	LVL_ALIAS: print alias name
 *	LVL_FULL: print fully qualified level name
 */
int	lvlformat = -1;
#define	SHOWLVL()	(lvlformat != -1)

main(argc,argv)
char **argv;
{
	DIR *dir;
	struct dirent *dentry;
	struct passwd *pw;
	struct stat buf, st1, st2;
	uid_t user;
	int i,fd;
	void catch();
	char *ptr,*job,pid[6];
	char *pp;
	char *mkjobname();
	char *jobfile = NULL;	      /* file containing job to be run */
	FILE *inputfile;
	time_t t;
	int  try=0;
	char	*file;
	char	*getenv();
	int zflag = 0;
	int Zflag = 0;
	int qflag=0;
	int dflag=0;
	int rflag=0;
	int lflag=0;
	char *timestr=NULL;
	int a;
	char *sh;

	int retval;
	char *fname;			/* at file name */
	int mld_real;			/* traversal in real mode */
	level_t user_lid;		/* user's MAC level */
	level_t job_lid;		/* job's MAC level */
	char label[MAXLABEL+1];		/* Space for the catalogue label */
	char *nptr;
	char pathnm[BUFSIZ];

	rgid = getgid();		/* remember group ids */
	egid = getegid();

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)strcpy(label, CMDCLASS);

	if (getenv(posix_var) != 0){
		posix = 1;
	} else  {
                posix = 0;
        }

	if (lvlin("SYS_PRIVATE", &job_lid) == 0)
		lvl=1;

	if ((nptr = strrchr(argv[0], '/')) != 0)
		nptr++;
	else
		nptr = argv[0];
	(void)strncat(label, nptr, MAXLABEL - sizeof CMDCLASS - 1);
	(void)setlabel(label);

	while ((a = getopt(argc, argv, "zZmf:q:t:ldr")) != -1) 
		switch (a) {		
			case 'z':	lvlformat = LVL_ALIAS;
					zflag++;
					break;
			case 'Z':	lvlformat = LVL_FULL;
					Zflag++;
					break;
			case 'm':	mailflag++;
					break;
			case 'f':	jobfile=optarg;
					break;
			case 'q':	if (strcmp(optarg,"a") == 0)
						jobtype=ATEVENT;
					else if (strcmp(optarg,"b") == 0)
						jobtype=BATCHEVENT;
					else
						atabort(":772:Invalid queue specified\n");
					qflag = *optarg;
					break;
			case 'l':	lflag++;
					break;
			case 'd':	dflag++;
					break;
			case 'r':	rflag++;
					break;
			case 't':	timestr=optarg;
					break;
			default:	usage();
		}

	/* Check combination of args */
	if (lflag) {

		/* 
		 * -l jobid ... 
		 * -l [-q queue]
		 */
		if (dflag || rflag || jobfile || mailflag || zflag || Zflag)
			usage();

		if (qflag && optind <argc)
			usage();
	} else if (zflag) {

		/*
		 * -z [jobid ...]
		 */
		
		if (!lvl) {
			pfmt(stderr, MM_ERROR,"uxlibc:1:Illegal option -- %c\n",
 					'z');
			pfmt(stderr,MM_ERROR,":593:System service not installed\n");
			usage();
		}

		if (dflag || rflag || jobfile || mailflag || lflag || Zflag)
			usage();

	} else if (Zflag) {
		/*
		 * -Z [jobid ...]
		 */
		
		if (!lvl) {
			pfmt(stderr, MM_ERROR, "uxlibc:1:Illegal option -- %c\n"					, 'Z');
			pfmt(stderr, MM_ERROR, ":593:System service not installed\n");
			usage();
		}

		if (dflag || rflag || jobfile || mailflag || lflag || zflag)
			usage();

	} else if (dflag) {
		/*
		 * -d jobid ...
		 */
		if (rflag || qflag || jobfile || mailflag )
			usage();
		if (optind >= argc)
			atabort(NODNUMBER);
	} else if (rflag) {
		/*
		 * -r jobid ...
		 */
		if (qflag || jobfile || mailflag )
			usage();
		if (optind >= argc)
			atabort(NORNUMBER);
	} else if (!timestr && optind >= argc) {
			/* no timespec */
			usage();
	}


	/* stat ATDIR and us st_flags to determine if it is an MLD */
	(void) stat(ATDIR, &buf);
	if (lvlproc(MAC_GET, &user_lid) == -1) {
		if (errno == ENOPKG) {
			mld_real = 0;
			user_lid = 0;	/* consistency */
		} else
			atabort(NOLVLPROC);
	} else if (S_ISMLD & buf.st_flags)
		mld_real = 1;
	else
		mld_real = 0;

	/*
	 * Release group cron after tstmld() had a chance at
	 * ATDIR.
	 */
	if (setegid(rgid) == -1)
		atabort(LPMERR);

	pp = getuser((user=getuid()), &sh);
	if(pp == NULL) {
		if(per_errno == 2)
			atabort(BADSHELL, "at");
		else
			atabort(INVALIDUSER);
	}
	strcpy(login,pp);
	/* /etc/cron.d can only be accessed by group "cron" */
	(void)setegid(egid);
	if (!allowed(login,ATALLOW,ATDENY)) atabort(NOTALLOWED, "at");
	if (setegid(rgid) == -1) atabort(LPMERR);

	if (rflag) {
		/* remove jobs that are specified */
		/* /var/spool/cron can only be accessed by group "cron" */
		(void)setegid(egid);
		if (chdir(ATDIR)==-1) atabort(CANTCD, "at", strerror(errno));
		if (setegid(rgid) == -1) atabort(LPMERR);
		for (i=optind; i<argc; i++) {
			fname = getfname(argv[i], mld_real);
			retval = stat(fname,&buf);
			if (retval == -1) {
				pfmt(stderr, MM_ERROR, BADSTAT, argv[i], strerror(errno));
				exit (1);
			}
			else {
				retval = unlink(fname);
				if (retval == -1) {
					pfmt(stderr, MM_ERROR, NOTOWNER, argv[i]);
					exit (1);
				}
				else {
					sendmsg(DELETE,login,argv[i],AT,egid);
				}
			}
		}
		exit(0); 
	}

	if (dflag) {
		/* display jobs that are specified */
		/* /var/spool/cron can only be accessed by group "cron" */
		(void)setegid(egid);
		if (chdir(ATDIR)==-1) atabort(CANTCD, "at", strerror(errno));
		if (setegid(rgid) == -1) atabort(LPMERR);
		for (i = optind; i < argc; i++) { /* every argument */
			ptr = argv[i];
			if (((t = num(&ptr)) == 0) || (*ptr != '.')) {
				pfmt(stderr, MM_ERROR, INVALIDJOB, argv[i]);
				exit (1);
			}
			else { /* access file */
				fname = getfname(argv[i], mld_real);
				retval = stat(fname,&buf);


				if (retval == -1) {
					pfmt(stderr, MM_ERROR, BADSTAT, argv[i], strerror(errno));
					exit (1);
				}
				else { /* check for DAC read access */
					retval = access(fname, 04);
					if (retval == -1) {
						pfmt(stderr, MM_ERROR, NOTOWNER, argv[i]);
						exit (1);
					}
					else
						at_display(fname, argv[i]);
				} /* end-else check for DAC read access */
			} /* end-else access file */
		} /* end-for every argument */

		exit(0);
	}

	if ( lflag || SHOWLVL()) {
	   /* list jobs for user */
	   /* /var/spool/cron can only be accessed by group "cron" */
	   (void)setegid(egid);
	   if (chdir(ATDIR) == -1) atabort(CANTCD, "at", strerror(errno));
	   if (setegid(rgid) == -1) atabort(LPMERR);
	   if (optind == argc) {
	   	/* list all jobs for a user */
	      if (mld_real) { /* real mode traversal */
	         if (stat(".", &st1) != 0 || stat("..", &st2) != 0)
	   	    atabort(BADSTATAT, strerror(errno));
	   	 if ((dir=opendir(".")) == NULL) atabort(NOOPENDIR, strerror(errno));
	   	 for (;;) {
	   	     if ((dentry = readdir(dir)) == NULL)
	   	   	break;
	   	     /* skip dot and dotdot entries */
	   	     if (dentry->d_ino==st1.st_ino || dentry->d_ino==st2.st_ino)
	   	   	continue;
	   	     /* make sure directory is readable */
	   	     if (stat(dentry->d_name, &buf) != 0)
	   	   	continue;
	   	     list_dir(dentry->d_name, user, qflag);
	   	 }
	   	 (void)closedir(dir);
	      } else /* virtual mode traversal */
	         list_dir(".", user,qflag);
	   } else { /* list particular jobs for user */
   	      for (i = optind; i < argc; i++) { /* every argument */
	    	  ptr = argv[i];
	   	  if (((t = num(&ptr)) == 0) || (*ptr != '.')) {
	   	     pfmt(stderr, MM_ERROR, INVALIDJOB, argv[i]);
		     exit (1);
		  }
	   	  else { /* access file */
	   	     fname = getfname(argv[i], mld_real);
	   	     retval = stat(fname,&buf);
	   
	   	     if (retval == -1) {
	   		pfmt(stderr, MM_ERROR, BADSTAT, argv[i], strerror(errno));
			exit (1);
		     }
	   	     else { /* check for DAC read access */
	   		retval = access(fname, 04);
	   		if (retval == -1) {
	   		   pfmt(stderr, MM_ERROR, NOTOWNER, argv[i]);
			   exit (1);
			}
	   		else { /* print job */
	   		   ascftime(timebuf, gettxt(FORMATID, FORMAT), localtime(&t));	
	   	
	   	
	   		   if ((user != buf.st_uid) && ((pw = getpwuid(buf.st_uid)) != NULL))
	   		      pfmt(stdout, MM_NOSTD, USEREQ, pw->pw_name, argv[i], timebuf);
	   		   else
	   		      printf("%s\t%s", argv[i], timebuf);
	   		
	   		   if (SHOWLVL()) {
				  /*
	   			  * Pre-allocate buffer to store level name.  This should
	   			  * save a lvlout call in most cases.  Double size
	   			  * dynamically if level won't fit in buffer.
	   			  */
	   			 static char	lvl_buf[BUFSIZE];
	   			 static char	*lvl_name = lvl_buf;
	   			 static int	lvl_namesz = BUFSIZE;
	   			 int		err = 0;
	   	
	   		      	job_lid = buf.st_level;
	   			 while (lvlout(&job_lid, lvl_name, lvl_namesz, lvlformat)==-1) {
	   			    if ((lvlformat == LVL_FULL) && (errno == ENOSPC)) {
	   				char *tmp_name;
	   				char *malloc();
	   				if ((tmp_name = malloc(lvl_namesz*2))
	   				==  (char *)NULL) {
	   			 	   pfmt(stderr, MM_ERROR, NOMEMLVL, argv[i]);
	   				   err++;
	   				   break;
	   				}
	   				lvl_namesz *= 2;
	   				if (lvl_name != lvl_buf)
	   				   free(lvl_name);
	   				lvl_name = tmp_name;
	   			    } else {
	   			 	pfmt(stderr, MM_ERROR, BADTRANSLATE, argv[i]);
	   				err++;
	   				break;
	   			    }
	   			 }
	   	
	   			 if (err == 0)
	   		  	    printf("\t%s", lvl_name);
	   		   } /* end-if SHOWLVL() */
	   		printf("\n");
	   		} /* end-else print job */
   		     } /* end-else check for DAC read access */
	   	  } /* end-else access file */
	      } /* end-for every argument */
	   } /* end-else list particular jobs for users */
	   exit(0);
	}

	/*
	 * Set MLD virtual mode unconditionally.
	 * The mldmode() call will fail for regular users.
	 * However, they should already be in virtual mode.
	 */
	(void)mldmode(MLD_VIRT);
	/* /var/spool/cron can only be accessed by group "cron" */
	(void)setegid(egid);
	if (getcwd(cwdname, PATH_MAX+1) == (char *)NULL) {
		if (errno && errno != ERANGE)
			atabort(CWDERR);
		else
			atabort(CWDLONG);
	}
	if (chdir(ATDIR)==-1) atabort(CANTCD, "at", strerror(errno));
	if (setegid(rgid) == -1) atabort(LPMERR);

	/* figure out what time to run the job */

	time(&now);

	if (timestr) {
		if ((when=gtime_posix(timestr)) == (time_t) -1) {
			pfmt(stderr,MM_ERROR,
				":1054:Incorrect time specification\n");
			usage();
		}
	} else if(posix || jobtype != BATCHEVENT) {	/* at job */
		argp = argpbuf;
		i = optind;
		while(i < argc &&
		      argp + strlen(argv[i]) < &argpbuf[sizeof(argpbuf) - 2]) {
			argp=strlist(argp,argv[i]," ", 0);
			i++;
		}
		argp = argpbuf;
		if ((file = getenv("DATEMSK")) == 0 || file[0] == '\0')
		{
			tp = localtime(&now);
			mday[1] = 28 + leap(tp->tm_year);
			yyparse();
			atime(&at, &rt);
			when = gtime(&at);
			if(!gmtflag) {
				when += timezone;
				if(localtime(&when)->tm_isdst)
					when -= 60 * 60;
			}
		}
		else
		{	/*   DATEMSK is set  */
			if ((ct = getdate(argpbuf)) == NULL)		
				atabort(errlist[getdate_err]);
			else
				when = mktime(ct);
		}
	} else		/* batch job */
		when = now;

	if(when < now)	/* time has already past */
		atabort(":36:Too late\n");

	sprintf(pid,"%-5d",getpid());
	/* already in at spool directory */
	tfname=xmalloc(strlen(TMPFILE)+7);
	strcat(strcpy(tfname,TMPFILE),pid);
	/* catch SIGINT, HUP, and QUIT signals */
	if (signal(SIGINT, catch) == SIG_IGN) signal(SIGINT,SIG_IGN);
	if (signal(SIGHUP, catch) == SIG_IGN) signal(SIGHUP,SIG_IGN);
	if (signal(SIGQUIT,catch) == SIG_IGN) signal(SIGQUIT,SIG_IGN);
	if (signal(SIGTERM,catch) == SIG_IGN) signal(SIGTERM,SIG_IGN);
	if((fd = open(tfname,O_CREAT|O_EXCL|O_WRONLY,ATMODE)) < 0)
		atabort(CANTCREATE);
	close(1);
	dup(fd);
	close(fd);
	sprintf(pname,"%s",PROTO);
	sprintf(pname1,"%s.%c",PROTO,'a'+jobtype);

	/*
	 * Open the input file with the user's permissions.
	 */
	if (jobfile != NULL) {

		/* Check if jobfile is full pathname and set pathnm accordingly */
		if (jobfile[0] == '/')
			(void)sprintf(pathnm, "%s", jobfile);
		else
			(void)sprintf(pathnm, "%s/%s", cwdname, jobfile);

		if ((inputfile = fopen(pathnm, "r")) == NULL) {
			unlink(tfname);
			pfmt(stderr, MM_ERROR, ":37:%s: %s\n", jobfile, strerror(errno));
			exit(1);
		}
	} else 
		inputfile = stdin;



	copy(jobfile, inputfile);
	/*
	 * The setuid bit is turned off once a write occurs on the UFS/SFS
	 * file system type.  Therefore, the file descriptor for tfname
	 * is closed, and an explicit chmod() is performed.
	 */
	(void)fflush(stdout);
	(void)close(1);
	if (chmod(tfname, ATMODE) == -1) {
		unlink(tfname);
		atabort(CANTCHMOD);
	}

	/*
	 * mkjobname() appends the user's lid to the name for
	 * uniqueness.  It turns out that when removing an at job
	 * this lid can be used to get to the right effective
	 * directory.  See getfname().
	 * No privilege is required to rename() since the user
	 * owns tfname and can write to the directory.
	 */
	while (rename(tfname,job=mkjobname(&when, user_lid))==-1) {
		sleep(1);
		if(++try > MAXTRYS / 10){
			atabort(CANTCREATE);
		}
	}
	unlink(tfname);
	sendmsg(ADD,login,job,AT, egid);
	if(per_errno == 2)
		pfmt(stderr, MM_WARNING, WARNSHELL, sh);
	cftime(timebuf, gettxt(FORMATID, FORMAT), &when);
	if (posix)
		pfmt(stderr,MM_NOSTD,":1055:job %s at %s\n",job,timebuf);
	else
		pfmt(stderr, MM_INFO, ":38:Job %s at %s\n",job,timebuf);
	if(!posix && when-now < MINUTE) pfmt(stderr, MM_WARNING,
	    ":39:This job may not be executed at the proper time.\n");
	exit(0);
}

/*
 * Procedure:     at_display
 *
 * Restrictions:
                 fopen: None
                 pfmt: None
                 fgets: None
                 fputs: None
                 fclose: None

*/

at_display(fname, jobname)
char *fname;
char *jobname;
{
	FILE	*fp;
	char	line[BUFSIZ];

	fp = fopen(fname, "r");
	if (fp == (FILE *)NULL) {
		pfmt(stderr, MM_ERROR, STATNOPEN);
		return;
	}
	pfmt(stdout, MM_NOSTD, ":594:### BEGIN        %s        BEGIN ###\n", jobname);
	while (fgets(line, BUFSIZ, fp) != NULL)
		fputs(line, stdout);
	(void)fclose(fp);
	pfmt(stdout, MM_NOSTD, ":595:### END        %s        END ###\n", jobname);
}


find(elem,table,tabsize)
char *elem,**table;
int tabsize;
{
	int i;

	for (i=0; i<(int)strlen(elem); i++)
		elem[i] = tolower(elem[i]);
	for (i=0; i<tabsize; i++)
		if (strcmp(elem,table[i])==0) return(i);
		else if (strncmp(elem,table[i],3)==0) return(i);
	return(-1);
}

/*
 * Procedure:     mkjobname
 *
 * Restrictions:
                 sprintf: None
                 stat(2): None
*/

/****************/
char
*mkjobname(t, lid)
/****************/
time_t *t;
level_t lid;
{
	int i;
	char *name;
	struct  stat buf;
	name=xmalloc(200);
	for (i=0;i < MAXTRYS;i++) {
		/*
		 * The LID is appended instead of prepended
		 * because at and cron make use of the time t
		 * and type.
		 */
		sprintf(name,"%ld.%c.%ld",*t,'a'+jobtype,lid);
		if (stat(name,&buf)) 
			return(name);
		*t += 1;
	}
	atabort(":40:Queue full\n");
}


/*
 * Procedure:     catch
 *
 * Restrictions:
 *               unlink(2): None
*/

void
catch()
{
	unlink(tfname);
	exit(1);
}


/*
 * Procedure:     atabort
 *
 * Restrictions:
                 pfmt: None
*/

atabort(msg, a1, a2, a3)
char *msg, *a1, *a2, *a3;
{
	pfmt(stderr, MM_ERROR, msg, a1, a2, a3);
	exit(1);
}

yywrap()
{
	return 1;
}

yyerror()
{
	atabort(BADDATE);
}

/*
 * add time structures logically
 */
atime(a, b)
register
struct tm *a, *b;
{
	if ((a->tm_sec += b->tm_sec) >= 60) {
		b->tm_min += a->tm_sec / 60;
		a->tm_sec %= 60;
	}
	if ((a->tm_min += b->tm_min) >= 60) {
		b->tm_hour += a->tm_min / 60;
		a->tm_min %= 60;
	}
	if ((a->tm_hour += b->tm_hour) >= 24) {
		b->tm_mday += a->tm_hour / 24;
		a->tm_hour %= 24;
	}
	a->tm_year += b->tm_year;
	if ((a->tm_mon += b->tm_mon) >= 12) {
		a->tm_year += a->tm_mon / 12;
		a->tm_mon %= 12;
	}
	a->tm_mday += b->tm_mday;
	while (a->tm_mday > mday[a->tm_mon]) {
		a->tm_mday -= mday[a->tm_mon++];
		if (a->tm_mon > 11) {
			a->tm_mon = 0;
			mday[1] = 28 + leap(++a->tm_year);
		}
	}
}

leap(year)
{
	return year % 4 == 0;
}

/*
 * return time from time structure
 */
time_t
gtime(tptr)
register
struct	tm *tptr;
{
	register i;
	long	tv;
	extern int dmsize[];

	tv = 0;
	for (i = 1970; i < tptr->tm_year+1900; i++)
		tv += dysize(i);
	if (dysize(tptr->tm_year) == 366 && tptr->tm_mon >= 2)
		++tv;
	for (i = 0; i < tptr->tm_mon; ++i)
		tv += dmsize[i];
	tv += tptr->tm_mday - 1;
	tv = 24 * tv + tptr->tm_hour;
	tv = 60 * tv + tptr->tm_min;
	tv = 60 * tv + tptr->tm_sec;
	return tv;
}

/*
 * Procedure:     copy
 *
 * Restrictions:
                 printf: None
                  fopen: None
                putchar: None
                  fgets: None
                   puts: None
                 fclose: None

 * Notes:
 * make job file from proto + stdin
*/

copy(jobfile, inputfile)
char *jobfile;
FILE *inputfile;
{
	register c;
	register FILE *pfp;
	char	line[BUFSIZ];
	register char **ep;
	mode_t um;
	char *val;
	extern char **environ;
	printf(": %s job\n",jobtype ? "batch" : "at");
	printf(": jobname: %.127s\n",(jobfile==NULL) ? "stdin" : jobfile);
	printf(": notify by mail: %s\n", (mailflag) ? "yes" : "no");
	for (ep=environ; *ep; ep++) {
		if ( strchr(*ep,'\'')!=NULL )
			continue;
		if ((val=strchr(*ep,'='))==NULL)
			continue;
		*val++ = '\0';
		if (posix || strcmp(*ep, "TIMEOUT") != 0)
			printf("export %s; %s='%s'\n",*ep,*ep,val);
		*--val = '=';
	}
	/* always cd to current directory first */
	printf("cd %s\n", cwdname);
	/* /etc/cron.d is accessible to group cron only */
	(void)setegid(egid);
	if((pfp = fopen(pname1,"r")) == NULL && (pfp=fopen(pname,"r"))==NULL)
		atabort(":41:No prototype\n");
	if (setegid(rgid) == -1)
		atabort(LPMERR);
	um = umask(0);
	while ((c = getc(pfp)) != EOF) {
		if (c != '$')
			putchar(c);
		else switch (c = getc(pfp)) {
		case EOF:
			goto out;
		/*
		 * This is here for compatibility only.
		 * Even if this cd line is not in the proto file,
		 * we'll still cd to the current working directory.
		 */
		case 'd':
			printf("%s", cwdname);
			break;
		case 'l':
			printf("%ld",ulimit(1,-1L));
			break;
		case 'm':
			printf("%o", um);
			break;
		case '<':
			while (fgets(line, BUFSIZ, inputfile) != NULL)
				fputs(line,stdout);
			break;
		case 't':
			printf(":%lu", when);
			break;
		default:
			putchar(c);
		}
	}
out:
	fclose(pfp);
}



/*
 * Procedure:     usage
 *
 * Restrictions:
                 pfmt: None
 * Notes:
 *
 * Print usage info and exit.
 */

usage()
{
	pfmt(stderr, MM_ERROR, BADUSAGE);
	pfmt(stderr, MM_ACTION,":1056:Usage:\n\tat [-m] [-f filename] [-q queuename] time [date] [+ increment]\n");
	pfmt(stderr, MM_NOSTD, ":1057:\tat [-m] [-f filename] [-q queuename] -t [[CC]YY]MMDDhhmm[.SS]\n");
	pfmt(stderr, MM_NOSTD, ":597:\tat -d job [job ...]\n");
	pfmt(stderr, MM_NOSTD, ":1058:\tat -l [-q queuename]\n");
	pfmt(stderr, MM_NOSTD, ":598:\tat -l [job ...]\n");
	pfmt(stderr, MM_NOSTD, ":599:\tat -r job [job ...]\n");
	if (lvl) {
		pfmt(stderr, MM_NOSTD, ":600:\tat -z [job ...]\n");
		pfmt(stderr, MM_NOSTD, ":601:\tat -Z [job ...]\n");
	}
	exit(1);
}

/*
 * Procedure:     getfname
 *
 * Restrictions:
                 opendir: None
                 stat(2): None
 * Notes:
 *
 * getfname() returns the pathname for the at job specified.
 * If in MLD real mode, the at job is prepended by the effective
 * directory in which it resides, otherwise the job itself is returned.
 * If the at job is not found in any effective directory, the at
 * job itself is returned.
 */
char *
getfname(name, mld_real)
	char	*name;
	int	mld_real;
{
	static char	fname[BUFSIZE];
	char		*retname;
	DIR		*dir;
	struct dirent	*dentry;
	struct stat	buf;
	int		namelen;

	if (!mld_real)
		return(name);
	
	/* name length plus one for "/" plus one for '\0' */
	namelen = strlen(name) + 2;
	if ((dir=opendir(".")) == NULL) atabort(NOOPENDIR, strerror(errno));
	for (;;) {
		if ((dentry = readdir(dir)) == NULL) {
			retname = name;
			break;
		}
		/* skip dot and dotdot entries */
		if (strcmp(dentry->d_name, ".") == 0
		||  strcmp(dentry->d_name, "..") == 0)
			continue;
		/* make sure directory is readable */
		if (stat(dentry->d_name, &buf) != 0)
			continue;
		/*
		 * if path does not fit in buffer, something is wrong;
		 * just return the at job itself.
		 */
		if (namelen + (int)strlen(dentry->d_name) > BUFSIZE) {
			retname = name;
			break;
		}
		/* make up pathname */
		(void)strcat(strcat(strcpy(fname, dentry->d_name), "/"), name);
		/* make sure file exists */
		if (stat(fname, &buf) == 0) {
			retname = fname;
			break;
		}
	}
	(void)closedir(dir);
	return(retname);
}

/*
 * Procedure:     effdirlid
 *
 * Restrictions:
                 sscanf: none
 *
 * effdirlid() returns the LID of the effective directory in which the
 * at job resides.  Since mkjobname() appends the LID to the end of the
 * job name, it is returned as the LID.
 */
level_t
effdirlid(name)
	char	*name;
{
	char	*ptr;
	level_t	lid;

	ptr = strrchr(name, '.');
	sscanf(ptr, "%x", &lid);
	return(lid);
}

/*
 * Procedure:     list_dir
 *
 * Restrictions:
                 chdir(2): None
                 stat(2): None
                 strerror: None
                 opendir: None
                 unlink(2): None
                 access(2): None
                 ascftime: None
                 gettxt: None
                 localtime: None
                 getpwuid: none
                 pfmt: None
                 printf: None
                 lvlout: None
 *
 * Notes:
 * list_dir() lists the at jobs in the specified effective directory.
 * If the specified directory is ".", there is no need to change
 * directory.  If the user is not an administrator, only its at jobs
 * are listed.
 *
 * if queue is not 0, only jobs in the queue are listed.
 */
void
list_dir(dname, user,queue)
	char	*dname;
	uid_t	user;
	int	queue;
{
	DIR	*dir;
	struct dirent *dentry;
	struct passwd *pw;
	struct stat buf, st1, st2;
	char	*ptr;
	time_t	t;
	int	retval;
	int	cd = 0;
	level_t	job_lid;

	/* go to effective directory */
	if (strcmp(dname, ".") != 0) {
		if (chdir(dname) == -1)
			return;
		cd = 1;
	}

	if (stat(".", &st1) != 0 || stat("..", &st2) != 0)
		atabort(BADSTATAT, strerror(errno));
	if ((dir = opendir(".")) == NULL) atabort(NOOPENDIR, strerror(errno));
	for (;;) {
		if ((dentry = readdir(dir)) == NULL)
			break;
		if (dentry->d_ino==st1.st_ino || dentry->d_ino==st2.st_ino)
			continue;
		if (stat(dentry->d_name, &buf) == -1) {
			/* get rid off file if possible */
			(void)unlink(dentry->d_name);
			continue; 
		}
		retval = access(dentry->d_name, 04);
		if (retval == -1)
			continue;

		if (queue && queue != *(strchr(dentry->d_name, '.') + 1))
			continue;

		ptr = dentry->d_name;
		if (((t=num(&ptr))==0) || (*ptr!='.'))
			continue;

		ascftime(timebuf, gettxt(FORMATID, FORMAT), localtime(&t));	

		/* don't read from possibly spoofed password file */

		if ((user!=buf.st_uid) && ((pw=getpwuid(buf.st_uid))!=NULL))
			pfmt(stdout, MM_NOSTD, USEREQ, pw->pw_name, dentry->d_name,
				timebuf);
		else
			printf("%s\t%s",dentry->d_name, timebuf);

		if (SHOWLVL()) {
			/*
			 * Pre-allocate buffer to store level name.  This should
			 * save a lvlout call in most cases.  Double size
			 * dynamically if level won't fit in buffer.
			 */
			static char	llvl_buf[BUFSIZE];
			static char	*llvl_name = llvl_buf;
			static int	llvl_namesz = BUFSIZE;
			int		err = 0;

			job_lid = buf.st_level;
			while (lvlout(&job_lid, llvl_name, llvl_namesz, lvlformat)==-1) {
				if ((lvlformat == LVL_FULL) && (errno == ENOSPC)) {
					char *tmp_name;
					char *malloc();
					if ((tmp_name = malloc(llvl_namesz*2))
					==  (char *)NULL) {
						pfmt(stderr, MM_ERROR, NOMEMLVL,
							dentry->d_name);
						err++;
						break;
					}
					llvl_namesz *= 2;
					if (llvl_name != llvl_buf)
						free(llvl_name);
					llvl_name = tmp_name;
				} else {
					pfmt(stderr, MM_ERROR, BADTRANSLATE,
						dentry->d_name);
					err++;
					break;
				}
			}

			if (err == 0)
				printf("\t%s", llvl_name);
		} /* end-if SHOWLVL() */

		printf("\n");
	}
	(void)closedir(dir);

	if (cd && chdir("..") == -1)
		atabort(CANTCD, "at", strerror(errno));
}


/*
 * from touch(1m)
 * SCCS ID "touch:touch.c	1.11.3.1"
 */



static 
gpair(const char **cbp)
{
	register char *cp;
	register char *bufp;
	char buf[3];

	cp = (char *) *cbp;
	bufp = buf;
	if (isdigit(*cp)) {
		*bufp++ = *cp++;
	} else {
		return (-1);
	}
	if (isdigit(*cp)) {
		*bufp++ = *cp++;
	} else {
		return (-1);
	}
	*bufp = '\0';
	*cbp = cp;
	return ((int)strtol(buf, (char **)NULL, 10));
}


static time_t
gtime_posix(const char *cbp)
{
	register int	i;
	struct tm	time_str;
	time_t		nt;
	int		time_len;

	time_len = (int)strlen(cbp);
	if (strchr(cbp, (int)'.') != (char *)NULL) {
		time_len -= 3;
	}

	switch (time_len) {
	case 12:	/* CCYYMMDDhhmm[.SS] */
		i = gpair(&cbp);
		if ((i < 19) || (i > 20)) 	{
			return (time_t) -1;
		} else {
			time_str.tm_year = (i - 19) * 100 + gpair(&cbp);
		}
		break;
	case 10:	/* YYMMDDhhmm[.SS] */
		i =  gpair(&cbp);
		if ((i >= 0) && (i <= 68)) {
			time_str.tm_year = 100 + i;
		} else if ((i >= 69) && (i <= 99)) {
			time_str.tm_year = i;
		} else {
			return (time_t) -1;
		}
		break;
	case 8:		/* MMDDhhmm[.SS] */
		(void)time(&nt);
		time_str.tm_year = localtime(&nt)->tm_year;
		break;
	default:
		return (time_t) -1;
	}

	time_str.tm_mon = gpair(&cbp) - 1;
	if ((time_str.tm_mon < 0) || (time_str.tm_mon > 11)) {
		return (time_t) -1;
	}

	time_str.tm_mday = gpair(&cbp);
	if ((time_str.tm_mday < 1) || (time_str.tm_mday > 31)) {
		return (time_t) -1;
	}

	time_str.tm_hour = gpair(&cbp);
	if ((time_str.tm_hour < 0) || (time_str.tm_hour > 23)) {
		return (time_t) -1;
	}

	time_str.tm_min = gpair(&cbp);
	if ((time_str.tm_min < 0) || (time_str.tm_min > 59)) {
		return (time_t) -1;
	}

	if (*cbp == '.') {
		cbp++;
		time_str.tm_sec = gpair(&cbp);
		if ((time_str.tm_sec < 0) || (time_str.tm_sec > 61)) {
			return (time_t) -1;
		}
	} else if (*cbp == '\0') {
		time_str.tm_sec = 0;
	} else {
		return (time_t) -1;
	}

	time_str.tm_isdst = -1;

	return mktime(&time_str);
}
