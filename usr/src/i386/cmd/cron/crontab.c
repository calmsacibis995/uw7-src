/*		copyright	"%c%" 	*/

/* 	Portions Copyright(c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident	"@(#)cron:i386/cmd/cron/crontab.c	1.6.3.1"
/***************************************************************************
 * Command: crontab
 * Inheritable Privileges: P_DACWRITE,P_DACREAD,P_OWNER,P_SETUID
 *       Fixed Privileges: None
 * Notes:
 *	1. crontab runs as group "cron" only to access /etc/cron.d and
 *	   /var/spool/cron, and to write a message to the NPIPE file.
 *	   At all other times, crontab runs with the invoking user's group.
 *	2. with enhanced security installed, crontabs is expected to
 *	   be a multi-level directory.
 *
 *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <locale.h>
#include <pwd.h>
#include <priv.h>
#include <mac.h>
#include <pfmt.h>
#include <string.h>
#include "cron.h"

#define TMPFILE		"_cron"		/* prefix for tmp file */
#define CRMODE		0600	/* mode for creating crontabs */
static const char
	BADCREATE[] =	":635:Cannot create crontab file in\n\tthe crontab directory.: %s\n",
	BADOPEN[] =	":636:Cannot open crontab file: %s\n",
	BADSHELL[] =	":9:Because your login shell is not /usr/bin/sh, you cannot use %s\n",
	WARNSHELL[] =	":1084:Commands will be executed using %s\n",
	BADUSAGE[] =	":637:Usage: crontab [file] | [ [  -e | -l | -r ] [user] ]\n",
	INVALIDUSER[] =	":14:You are not a valid user (no entry in /etc/passwd)\n",
	NOTALLOWED[] =	":17:You are not authorized to use %s.  Sorry.\n",
	EOLN[] =	":78:Unexpected end of line.\n",
	UNEXPECT[] =	":121:Unexpected character found in line.\n",
	OUTOFBOUND[] =	":122:Number out of bounds.\n",
	ERRSFND[] =	":123:Errors detected in input, no crontab file generated.\n",
	BADREAD[] =	":124:Error reading your crontab file\n",
	LPMERR[] =	":587:Process terminated to enforce least privilege\n",
	NOLVLPROC[] =	":638:Cannot perform lvlproc() call.\n",
	NOCHOWN[] =	":639:Cannot change owner as owner of file.\n",
	NOCHDIR[] =	":342:Cannot change directory to \"%s\" directory: %s\n",
	BADWRITETMP[] =	":125:Write error on temporary file: %s\n",
	BADOPENTMP[] = 	":126:Cannot open temporary file: %s\n",
	BADSTATTMP[] = 	":127:Cannot access temporary file: %s\n",
	CWDERR[] = 	":640:Cannot determine current directory.\n",
	NOREMOVE[] =	":641:Cannot remove crontab file: %s.\n",
	INVINPUT[] =	":642:Invalid input to crontab was saved in %s\n";


extern int opterr, optind, per_errno;
extern char *optarg, *xmalloc();
int err,cursor;
void catch();
char *cf,*tnam,line[CTLINESIZE];
char edtemp[5+13+1];
uid_t	ruid, chown_uid;
gid_t	rgid, egid;

static char posix_var[] = "POSIX2";
static int posix;

/*
 * Procedure:     main
 *
 * Restrictions:
                 getcwd: None
                 chown(2): None
                 setgid(2): None
                 setlocale: None
                 pfmt: None
                 getpwnam: None
                 chdir(2): None
                 unlink(2): None
                 sendmsg: None
                 fopen: None
                 strerror: None
                 mktemp: None
                 stat(2): None
                 access(2): None
*/

main(argc,argv)
char **argv;
{
	int c, rflag, lflag, eflag, errflg;
	char login[UNAMESIZE],*getuser(),*strcat(),*strcpy();
	char *pp = NULL, *cwd, *getcwd();
	FILE *fp, *tmpfp;
	struct stat stbuf;
	time_t omodtime;
	char *editor, *getenv();
	char buf[BUFSIZ];
	struct passwd *pw;
	level_t user_lid;
	pid_t pid;
	int stat_loc;
	/* Enhanced Application Compatibility */
	int uflag;
	char *username;
	char *sh;

	rgid = getgid();
	egid = getegid();


	if(setegid(rgid) == -1)
		crabort(LPMERR);

	if (lvlproc(MAC_GET, &user_lid) == -1) {
		if (errno != ENOPKG)
			crabort(NOLVLPROC);
	} else {
		/*
		 * Set MLD virtual mode unconditionally.
		 * The mldmode() call will fail for regular users.
		 * However, they should already be in virtual mode.
		 */
		(void)mldmode(MLD_VIRT);
	}

	rflag = 0;
	lflag = 0;
	eflag = 0;
	uflag = 0;	/* Enhanced Application Compatibility */
	errflg = 0;
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:crontab");

	if (getenv(posix_var) != 0)	{
		posix = 1;
	} else	{
		posix = 0;
	}

	while ((c=getopt(argc, argv, "elru:")) != EOF)
		switch (c) {
			case 'e':
				if (lflag || rflag)
					errflg++;
				else
					eflag++;
				break;
			case 'l':
				if (eflag || rflag)
					errflg++;
				else
					lflag++;
				break;
			case 'r':
				if (eflag || lflag)
					errflg++;
				else
					rflag++;
				break;
		/* Enhanced Application Compatibility */
			case 'u':
				/* argc--; */
				username = optarg;
				uflag++;
				break;
			case '?':
				errflg++;
				break;
		}
	argc -= optind;
	argv += optind;
	if (errflg || argc > 1 ||
		/* Enhanced Application Compatibility */
		(uflag && !(rflag || lflag || eflag)) ||
		(uflag && (username == (char *)NULL))) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, BADUSAGE);
		exit(1);
	}
	ruid = getuid();

	if ((eflag || lflag || rflag) && uflag)		/* Enhanced Application Compatibility */
		pp =  username; 			/* Enhanced Application Compatibility */
	else if ((eflag || lflag || rflag) && argc == 1)
		pp = *argv++;
	if (pp) {
		if ((pw = getpwnam(pp)) == NULL)
			crabort(INVALIDUSER);
		chown_uid = (uid_t)pw->pw_uid;
	} else {
		pp = getuser(ruid, &sh);
		if(pp == NULL) {
			if (per_errno==2)
				crabort(BADSHELL, "cron");
			else
				crabort(INVALIDUSER); 
		}
		chown_uid = ruid;
	}
		
	strcpy(login,pp);

	/* /etc/cron.d and /var/spool/cron are accessible by
	 * group "cron" only.
	 */
	(void)setegid(egid);
	if (!allowed(login,CRONALLOW,CRONDENY)) crabort(NOTALLOWED, "crontab");
	if ((cwd = getcwd(NULL, MAXPATHLEN)) == NULL)
		crabort(CWDERR);
	if (chdir(CRONDIR) == -1) crabort(NOCHDIR, "crontab");
	if (setegid(rgid) == -1) crabort(LPMERR);
	cf = login;
	
	if (rflag) {
		/*
		 * If we do not own or have privilege to 
		 * remove crontab file,  abort.
		 */
		if(unlink(cf) < 0)
			 crabort(NOREMOVE, strerror(errno));
		sendmsg(DELETE, login, login, CRON, egid);
		exit(0); 
	}
	if (lflag) {
		if((fp = fopen(cf,"r")) == NULL)
			crabort(BADOPEN, strerror(errno));
		while(fgets(line,CTLINESIZE,fp) != NULL)
			fputs(line,stdout);
		fclose(fp);
		exit(0);
	}
	if (eflag) {
		if (argc == 1)
			if (setuid(pw->pw_uid) < 0)
				crabort(BADOPEN, strerror(errno));
		if((fp = fopen(cf,"r")) == NULL) {
			if(errno != ENOENT)
				 crabort(BADOPEN, strerror(errno));
		}
		(void)strcpy(edtemp, "/tmp/crontabXXXXXX");
		(void)mktemp(edtemp);
		/* 
		 * Fork off a child with user's permissions, 
		 * to edit the crontab file
		 */
		pid = fork();
		if (pid == (pid_t)-1)
			crabort(":43:fork() failed: %s\n", strerror(errno));
		if (pid == 0) {		/* child process */
			if (setgid(rgid) == -1)
				crabort(LPMERR);
			if (chdir(cwd) == -1 && chdir("/tmp") == -1)
				crabort(NOCHDIR, cwd);
			if((tmpfp = fopen(edtemp,"w")) == NULL)
				crabort(":128:Cannot create temporary file: %s\n",
					strerror(errno));
			if(fp != NULL) {
				/*
				 * Copy user's crontab file to temporary file.
				 */
				while(fgets(line,CTLINESIZE,fp) != NULL) {
					fputs(line,tmpfp);
					if(ferror(tmpfp)) {
						fclose(fp);
						fclose(tmpfp);
						crabort(BADWRITETMP, strerror(errno));
					}
				}
				if (ferror(fp)) {
					fclose(fp);
					fclose(tmpfp);
					crabort(BADREAD);
				}
				fclose(fp);
			}
			if(fclose(tmpfp) == EOF)
				crabort(BADWRITETMP, strerror(errno));
			if(stat(edtemp, &stbuf) < 0)
				crabort(BADSTATTMP, strerror(errno));
			omodtime = stbuf.st_mtime;
			if (posix) {
				editor = getenv("EDITOR");
				if (editor == NULL)
					editor = "vi";
			} else {
				editor = getenv("VISUAL");
				if (editor == NULL)
					editor = getenv("EDITOR");
				if (editor == NULL)
					editor = "ed";
			}
			(void)sprintf(buf, "%s %s", editor, edtemp);
			(void)printf("%s\n", buf);
			/* there is no MACREAD so don't worry about trojan horse */
			(void) sleep(1);
			if (system(buf) == 0) {
				/* sanity checks */
				if((tmpfp = fopen(edtemp, "r")) == NULL)
					crabort(BADOPENTMP, strerror(errno));
				if(fstat(fileno(tmpfp), &stbuf) < 0)
					crabort(BADSTATTMP, strerror(errno));
				if(stbuf.st_size == 0)
					crabort(":129:Temporary file empty\n");
				if(omodtime == stbuf.st_mtime) {
					(void)unlink(edtemp);
					pfmt(stderr, MM_NOSTD,
					    ":130:The crontab file was not changed.\n");
					exit(1);
				}
				exit(0);
			} else {
				/*
				 * Couldn't run editor.
				 */
				(void)unlink(edtemp);
				exit(1);
			}
		}
		wait(&stat_loc);
		if ((stat_loc & 0xFF00) != 0)
			exit(1);
		if ((tmpfp = fopen(edtemp,"r")) == NULL) 
			crabort(BADOPENTMP, strerror(errno));
		if (copycron(tmpfp) == 0)
			(void)unlink(edtemp);
		else {
			pfmt(stderr, MM_ERROR, ERRSFND); 
			pfmt(stderr, MM_INFO, INVINPUT, edtemp);
			exit(1);
		}
	} else {
		if (argc==0) {
			if(copycron(stdin) != 0) {
				pfmt(stderr, MM_ERROR, ERRSFND);
				exit(1);
			}
		}
		else {
			/* Check if crontab input file is full pathname */
			if (argv[0][0] == '/')
				(void)sprintf(buf, "%s", argv[0]);
			else
				(void)sprintf(buf, "%s/%s", cwd, argv[0]);
			if (access(buf, 04) || (fp=fopen(buf, "r"))==NULL)
				crabort(BADOPEN, strerror(errno));
			if(copycron(fp) != 0) {
				pfmt(stderr, MM_ERROR, ERRSFND);
				exit(1);
			}
		}
	}
	sendmsg(ADD, login, login, CRON, egid);
	if(per_errno == 2)
		pfmt(stderr, MM_WARNING, WARNSHELL, sh);
	exit(0);
}


/*
 * Procedure:     copycron
 *
 * Restrictions:
                 creat(2): None
                 strerror: None
                 unlink(2): None
                 rename(2): None
                 chown(2): None
                 pfmt: None
*/
int
copycron(fp)
FILE *fp;
{
	FILE *tfp,*fdopen();
	char pid[6],*strcat(),*strcpy();
	int t;

	sprintf(pid,"%-5d",getpid());
	/* currently in CRONDIR directory */
	tnam=xmalloc(strlen(TMPFILE)+7);
	strcat(strcpy(tnam,TMPFILE),pid);
	/* catch SIGINT, SIGHUP, SIGQUIT signals */
	if (signal(SIGINT,catch) == SIG_IGN) signal(SIGINT,SIG_IGN);
	if (signal(SIGHUP,catch) == SIG_IGN) signal(SIGHUP,SIG_IGN);
	if (signal(SIGQUIT,catch) == SIG_IGN) signal(SIGQUIT,SIG_IGN);
	if (signal(SIGTERM,catch) == SIG_IGN) signal(SIGTERM,SIG_IGN);
	if ((t=creat(tnam,CRMODE))==-1) crabort(BADCREATE, strerror(errno));
	if ((tfp=fdopen(t,"w"))==NULL) {
		unlink(tnam);
		crabort(BADCREATE, strerror(errno)); 
	}
	err=0;	/* if errors found, err set to 1 */
	while (fgets(line,CTLINESIZE,fp) != NULL) {
		cursor=0;
		while(line[cursor] == ' ' || line[cursor] == '\t')
			cursor++;
		if(line[cursor] == '#')
			goto cont;
		if (next_field(0,59)) continue;
		if (next_field(0,23)) continue;
		if (next_field(1,31)) continue;
		if (next_field(1,12)) continue;
		if (next_field(0,06)) continue;
		if (line[++cursor] == '\0') {
			cerror(EOLN);
			continue; 
		}
cont:
		if (fputs(line,tfp) == EOF) {
			unlink(tnam);
			crabort(BADCREATE, strerror(errno)); 
		}
	}
	fclose(fp);
	fclose(tfp);
	if (!err) {
		/* make file tfp the new crontab */
		unlink(cf);
		if (rename(tnam,cf)==-1) {
			unlink(tnam);
			crabort(BADCREATE, strerror(errno)); 
		} 
		/* change owner of file if necessary */
		if (ruid != chown_uid) {
			/*
			 * chown requires privilege if kernel is
			 * configured with rstchown set.
			 */
			if (chown(cf, chown_uid, egid) == -1) crabort(NOCHOWN);
		}
		return(0);
	} else {
		/* Return a non-zero code if error is detected input file
	   	   for case where -e option was used so that the tmp file
		   is save in /tmp for editing if desired */
		return(1);
	}
}


next_field(lower,upper)
int lower,upper;
{
	int num,num2;

	while ((line[cursor]==' ') || (line[cursor]=='\t')) cursor++;
	if (line[cursor] == '\0') {
		cerror(EOLN);
		return(1); 
	}
	if (line[cursor] == '*') {
		cursor++;
		if ((line[cursor]!=' ') && (line[cursor]!='\t')) {
			cerror(UNEXPECT);
			return(1); 
		}
		return(0); 
	}
	while (TRUE) {
		if (!isdigit(line[cursor])) {
			cerror(UNEXPECT);
			return(1); 
		}
		num = 0;
		do { 
			num = num*10 + (line[cursor]-'0'); 
		}			while (isdigit(line[++cursor]));
		if ((num<lower) || (num>upper)) {
			cerror(OUTOFBOUND);
			return(1); 
		}
		if (line[cursor]=='-') {
			if (!isdigit(line[++cursor])) {
				cerror(UNEXPECT);
				return(1); 
			}
			num2 = 0;
			do { 
				num2 = num2*10 + (line[cursor]-'0'); 
			}				while (isdigit(line[++cursor]));
			if ((num2<lower) || (num2>upper)) {
				cerror(OUTOFBOUND);
				return(1); 
			}
		}
		if ((line[cursor]==' ') || (line[cursor]=='\t')) break;
		if (line[cursor]=='\0') {
			cerror(EOLN);
			return(1); 
		}
		if (line[cursor++]!=',') {
			cerror(UNEXPECT);
			return(1); 
		}
	}
	return(0);
}


/*
 * Procedure:     cerror
 *
 * Restrictions:
                 pfmt: None
*/
cerror(msg)
char *msg;
{
	pfmt(stderr, MM_ERROR, ":131:%s: error on previous line;", line);
	pfmt(stderr, MM_NOSTD, msg);
	err=1;
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
	unlink(tnam);
	exit(1);
}


/*
 * Procedure:     crabort
 *
 * Restrictions:
                 unlink(2): None
                 pfmt: None
*/
crabort(msg, a1, a2, a3)
char *msg, *a1, *a2, *a3;
{
	int sverrno;

	if (strcmp(edtemp, "") != 0) {
		sverrno = errno;
		(void)unlink(edtemp);
		errno = sverrno;
	}
	if (tnam != NULL) {
		sverrno = errno;
		(void)unlink(tnam);
		errno = sverrno;
	}
	pfmt(stderr, MM_ERROR, msg, a1, a2, a3);
	exit(1);
}
