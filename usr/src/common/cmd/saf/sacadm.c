/*		copyright	"%c%" 	*/

# ident	"@(#)sacadm.c	1.6"
#ident  "$Header$"

/***************************************************************************
 * Command: sacadm
 * Inheritable Privileges: P_SETFLEVEL,P_COMPAT,P_FILESYS,P_DACREAD,P_DACWRITE
 *	 Fixed Privileges: None
 *
 * Notes: In order to use this command the user has to be at 
 *        SYS_PRIVATE level.
 *
 ***************************************************************************/

# include <stdio.h>
# include <fcntl.h>
# include <errno.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <signal.h>
# include <unistd.h>
# include <sac.h>
# include <stropts.h>
# include <priv.h>
# include <mac.h>
# include <sys/secsys.h>
# include <locale.h>
# include <pfmt.h>
# include "misc.h"
# include "structs.h"
# include "adm.h"
# include "extern.h"
# include "msg.h"
# include "msgs.h"

/*
 * functions
 */

char	*pflags();
char	*getfield();
void	add_pm();
void	cleandirs();
void	rem_pm();
void	start_pm();
void	kill_pm();
void	enable_pm();
void	disable_pm();
void	list_pms();
void	read_db();
void	sendcmd();
void	checkresp();
void	single_print();
void	catch();
void	usage();

# define START		0x1	/* -s seen */
# define KILL		0x2	/* -k seen */
# define ENABLE		0x4	/* -e seen */
# define DISABLE	0x8	/* -d seen */
# define PLIST		0x10	/* -l seen */
# define LIST		0x20	/* -L seen */
# define DBREAD		0x40	/* -x seen */
# define CONFIG		0x80	/* -G seen */
# define PCONFIG	0x100	/* -g seen */
# define ADD		0x200	/* -a or other required options seen */
# define REMOVE		0x400	/* -r seen */

/*
 * common error messages
 */


int	Saferrno;		/* internal `errno' for exit */

char    *msg_label = "UX:sacadm";

/*
 * Procedure: main - scan args for sacadm and call appropriate handling code
 *
 * Restrictions: check_version: <none>
 */

main(argc, argv)
int argc;
char *argv[];
{
	int c;			/* option letter */
	int ret;		/* scratch return code */
	int flag = 0;		/* flag to record requested operations */
	int errflg = 0;		/* error indicator */
	int version = -1;	/* argument to -v */
	int count = 0;		/* argument to -n */
	int badcnt = 0;		/* count of bad args to -f */
	int sawaflag = 0;	/* true if actually saw -a */
	int conflag = 0;	/* true if output should be in condensed form */
	long flags = 0;		/* arguments to -f */
	FILE *fp;		/* scratch file pointer */
	char *pmtag = NULL;	/* argument to -p */
	char *type = NULL;	/* argument to -t */
	char *script = NULL;	/* argument to -z */
	char *command = NULL;	/* argument to -c */
	char *comment = NULL;	/* argument to -y */
	char badargs[SIZE];	/* place to hold bad args to -f */
	char buf[SIZE];		/* scratch buffer */
	char *p;	                /* scratch pointer */

    (void)setlocale(LC_ALL,"");
    (void)setcat("uxsaf");
    (void)setlabel(msg_label);

	if (argc == 1)
		usage(argv[0]);
	while ((c = getopt(argc, argv, "ac:def:GgkLln:p:rst:v:xy:z:")) != -1) {
		switch (c) {
		case 'a':
			flag |= ADD;
			sawaflag = 1;
			break;
		case 'c':
			flag |= ADD;
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				exit (1);
			}
			command = optarg;
			if (*command != '/') {
				Saferrno = E_BADARGS;
				pfmt (stderr, MM_ERROR, MSG112);
				quit();
			}
			break;
		case 'd':
			flag |= DISABLE;
			break;
		case 'e':
			flag |= ENABLE;
			break;
		case 'f':
			flag |= ADD;
			while (*optarg) {
				switch (*optarg++) {
				case 'd':
					flags |= D_FLAG;
					break;
				case 'x':
					flags |= X_FLAG;
					break;
				default:
					badargs[badcnt++] = *(optarg - 1);
					break;
				}
			}
			/* null terminate just in case anything is there */
			badargs[badcnt] = '\0';
			break;
		case 'G':
			flag |= CONFIG;
			break;
		case 'g':
			flag |= PCONFIG;
			break;
		case 'k':
			flag |= KILL;
			break;
		case 'L':
			flag |= LIST;
			break;
		case 'l':
			flag |= PLIST;
			break;
		case 'n':
			flag |= ADD;
			count = atoi(optarg);
			if (count < 0) {
				Saferrno = E_BADARGS;
				pfmt (stderr, MM_ERROR, MSG113);
				quit();
			}
			break;
		case 'p':
			pmtag = optarg;
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				exit (1);
			}
			if (strlen(pmtag) > PMTAGSIZE) {
				pmtag[PMTAGSIZE] = '\0';
				pfmt (stderr, MM_ERROR, MSG57,pmtag);
				quit();
			}
			for (p = pmtag; *p; p++) {
				if (!isalnum(*p)) {
					Saferrno = E_BADARGS;
					pfmt (stderr, MM_ERROR, MSG80);
					quit();
				}
			}
			break;
		case 'r':
			flag |= REMOVE;
			break;
		case 's':
			flag |= START;
			break;
		case 't':
			type = optarg;
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				exit (1);
			}
			if (strlen(type) > PMTYPESIZE) {
				type[PMTYPESIZE] = '\0';
				pfmt (stderr, MM_ERROR, MSG59, type);
				quit();
			}
			for (p = type; *p; p++) {
				if (!isalnum(*p)) {
					Saferrno = E_BADARGS;
					pfmt (stderr, MM_ERROR, MSG83);
					quit();
				}
			}
			break;
		case 'v':
			flag |= ADD;
			version = (int) strtol (optarg, &p, 10);
			if (p == optarg) {
			        Saferrno = E_BADARGS;
					pfmt (stderr, MM_ERROR, MSG84);
					quit();
			} else if (version < 0) {
				Saferrno = E_BADARGS;
				pfmt (stderr, MM_ERROR, MSG85);
				quit();
			}
			break;
		case 'x':
			flag |= DBREAD;
			break;
		case 'y':
			flag |= ADD;
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				exit (1);
			}
			comment = optarg;
			break;
		case 'z':
			if (strchr(optarg, '\n')) {
				Saferrno = E_BADARGS;
				fprintf (stderr, "%s",gettxt(MSGID186, MSG186));
				exit (1);
			}
			script = optarg;
			break;
		case '?':
			errflg++;
		}
	}
	if (errflg || (optind < argc))
		usage(argv[0]);

	if (badcnt) {
		/* bad flags were given to -f */
		Saferrno = E_BADARGS;
		pfmt (stderr, MM_ERROR, MSG86, badargs);
		quit();
	}

	ret = check_version(VERSION, SACTAB);
	if (ret == 1) {
		Saferrno = E_SAFERR;
		pfmt (stderr, MM_ERROR, MSG87);
		quit();
	}
	else if (ret == 2) {
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG88, SACTAB);
		quit();
	}
	else if (ret == 3) {
		Saferrno = E_SAFERR;
		pfmt (stderr, MM_ERROR, MSG89, SACTAB);
		quit();
	}
	switch (flag) {
	case ADD:
		if (!sawaflag || !pmtag || !type || !command || (version < 0))
			usage(argv[0]);
		add_pm(pmtag, type, command, version, flags, count, script, comment);
		break;
	case REMOVE:
		if (!pmtag || type || script)
			usage(argv[0]);
		rem_pm(pmtag);
		break;
	case START:
		if (!pmtag || type || script)
			usage(argv[0]);
		start_pm(pmtag);
		break;
	case KILL:
		if (!pmtag || type || script)
			usage(argv[0]);
		kill_pm(pmtag);
		break;
	case ENABLE:
		if (!pmtag || type || script)
			usage(argv[0]);
		enable_pm(pmtag);
		break;
	case DISABLE:
		if (!pmtag || type || script)
			usage(argv[0]);
		disable_pm(pmtag);
		break;
	case LIST:
		conflag = 1;
		/* fall through */
	case PLIST:
		if ((pmtag && type) || script)
			usage(argv[0]);
		list_pms(pmtag, type, conflag);
		break;
	case DBREAD:
		if (type || script)
			usage(argv[0]);
		read_db(pmtag);
		break;
	case CONFIG:
		if (type || pmtag)
			usage(argv[0]);
		(void) do_config(script, "_sysconfig");
		break;
	case PCONFIG:
		if (!pmtag || type)
			usage(argv[0]);
		if ((fp = open_sactab("r")) == NULL) {
			Saferrno = E_SYSERR;
			pfmt (stderr, MM_ERROR, MSG90);
			quit();
		}
		if (!find_pm(fp, pmtag)) {
			Saferrno = E_NOEXIST;
			pfmt (stderr, MM_ERROR, MSG91, pmtag);
			quit();
		}
		(void) fclose(fp);
		(void) sprintf(buf, "%s/_config", pmtag);
		(void) do_config(script, buf);
		break;
	default:
		/* we only get here if more than one flag bit was set */
		usage(argv[0]);
		/* NOTREACHED */
	}
	quit();
	/* NOTREACHED */
}


/*
 * Procedure: usage - print out a usage message
 *
 * Args:	cmdname - the name command was invoked with
 */

void
usage(cmdname)
char *cmdname;
{
	
	pfmt (stderr, MM_ERROR, MSG114, cmdname);
	pfmt (stderr, MM_ERROR, MSG115);
	pfmt (stderr, MM_ACTION, MSG116, cmdname);
	pfmt (stderr, MM_ACTION, MSG117, cmdname);
	pfmt (stderr, MM_ACTION, MSG118, cmdname);
	pfmt (stderr, MM_ACTION, MSG119, cmdname);
	pfmt (stderr, MM_ACTION, MSG120, cmdname);
	pfmt (stderr, MM_ACTION, MSG121, cmdname);
	pfmt (stderr, MM_ACTION, MSG122, cmdname);
	pfmt (stderr, MM_ACTION, MSG123, cmdname);
	pfmt (stderr, MM_ACTION, MSG124, cmdname);
	pfmt (stderr, MM_ACTION, MSG125, cmdname);
	Saferrno = E_BADARGS;
	quit();
}


/*
 * Procedure: add_pm - add a port monitor entry
 *
 * Args:	tag - port monitor's tag
 *		type - port monitor's type
 *		command - command string to invoke port monitor
 *		version - version number of port monitor's pmtab
 *		flags - port monitor flags
 *		count - restart count
 *		script - port monitor's configuration script
 *		comment - comment describing port monitor
 *
 * Restrictions: stat(2): <none>	stat(2) for "command": <none>
 *               execl(2): <none>	mkdir(2): <none>
 *               mknod(2): <none>	chown(2): <none>
 *               creat(2): <none>	unlink(2): <none>
 *               lvlin:  <none>      	lvlfile(2): <none>
 *		 access(2): <none> 
 */

void
add_pm(tag, type, command, version, flags, count, script, comment)
char *tag;
char *type;
char *command;
int version;
long flags;
int count;
char *script;
char *comment;
{
	FILE *fp;		/* file pointer for _sactab */
	int fd;			/* scratch file descriptor */
	struct stat statbuf;	/* file status info */
	char buf[SIZE];		/* scratch buffer */
	char fname[SIZE];	/* scratch buffer for building names */
	register int i;		/* scratch variable */
	level_t level_pri;	/* SYS_PRIVATE level identifier */	
	pid_t pid;		/* fork() pid */
	int ret;		/* scratch variable */
	char home_dir[SIZE];	/* /etc/saf/<tag> pathname */
	char althome_dir[SIZE];	/* /var/saf/<tag> pathname */
	char pid_file[SIZE];	/* /etc/saf/<tag>/_pid pathname */
	char pmpipe_file[SIZE];	/* /etc/saf/<tag>/_pmpipe pathname */
	char pmtab_file[SIZE];	/* /etc/saf/<tag>/_pmtab pathname */
	short set_level = 1;    /* there exists valid level lids for lvlfile() */

	if ((fp = open_sactab("r")) == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG37);
		quit();
	}

	if (find_pm(fp, tag)) {
		Saferrno = E_DUP;
		pfmt (stderr, MM_ERROR, MSG126,tag);
		quit();
	}
	(void) fclose(fp);
	
	/* set variables for new <tag> directories & files */
	(void) sprintf(home_dir, "%s/%s", HOME, tag);	
	(void) sprintf(althome_dir, "%s/%s", ALTHOME, tag);
	(void) sprintf(pid_file, "%s/%s/_pid", HOME, tag);
	(void) sprintf(pmpipe_file, "%s/%s/_pmpipe", HOME, tag);	
	(void) sprintf(pmtab_file, "%s/%s/_pmtab", HOME, tag);	

/*
 * create the directories for <tag> under /etc/saf and /var/saf if needed 
 * and put in initial files _pid, _pmpipe, _pmtab, & _config
 */
	for (i = 0; i < 2; i++) {
		/* i == 0 do /etc/saf, i == 1 do /var/saf */
		(void) sprintf(fname, "%s/%s", (i == 0 ) ? HOME : ALTHOME, tag);
		if (access(fname, F_OK) == 0) {
			/* something is there, find out what it is */
			if (stat(fname, &statbuf) < 0) {
				Saferrno = E_SYSERR;
				pfmt (stderr, MM_ERROR, MSG1);
				quit();
			}
			if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
				Saferrno = E_SYSERR;
				pfmt (stderr, MM_ERROR, MSG127,fname);
				quit();
			}

			/* note: this removes the directory too */
			if ((pid = fork()) < 0) {
				Saferrno = E_SYSERR;
				pfmt (stderr, MM_ERROR, MSG128);
				quit();
			}
			else if ( pid == 0 ) { 		/* child */
				(void) execl("/usr/bin/rm", "rm", "-f", "-r", fname, 0);
				Saferrno = E_SYSERR;
				pfmt (stderr, MM_ERROR, MSG129, fname);
				quit();
			}
			else 
				(void) wait(&ret);	/* parent */
		}

/*
 * create the directory - later will change ownership and level 
 */
		ret = mkdir(fname, 0755);		
		if (ret < 0) {
			Saferrno = E_SYSERR;
			cleandirs(tag);
			pfmt (stderr, MM_ERROR, MSG130, fname);
			quit();
		}
	}

/*
 * put in the config script, if specified
 */

	if (script) {
		(void) sprintf(fname, "%s/_config", tag);
		if (do_config(script, fname)) {
			cleandirs(tag);
			/* do_config put out any messages */
			quit();
		}
	}

/*
 * create the communications pipe, but first make sure that the
 * permissions we specify are what we get
 */

	(void) umask(0);
	if (mknod(pmpipe_file, S_IFIFO | 0600, 0) < 0) {
		Saferrno = E_SYSERR;
		cleandirs(tag);
		pfmt (stderr, MM_ERROR, MSG131);
		quit();
	}
	
/*
 * create the _pid file
 */

	if ((fd = creat(pid_file, 0644)) < 0) {
		Saferrno = E_SYSERR;
		cleandirs(tag);
		pfmt (stderr, MM_ERROR, MSG132);
		quit();
	}
	(void) close(fd);
	
/*
 * create the _pmtab file
 */

	if ((fd = creat(pmtab_file, 0644)) < 0) {
		Saferrno = E_SYSERR;
		cleandirs(tag);
		pfmt (stderr, MM_ERROR, MSG133);
		quit();
	}
	(void) sprintf(buf, "%s%d\n", VSTR, version);
	if (write(fd, buf, (unsigned) strlen(buf)) != strlen(buf)) {
		(void) close(fd);
		(void) unlink(pmtab_file);
		Saferrno = E_SYSERR;
		cleandirs(tag);
		pfmt (stderr, MM_ERROR, MSG134);
		quit();
	}
	(void) close(fd);

/*
 * isolate the command name, but remember it since strtok() trashes it
 */

	(void) strcpy(buf, command);
	(void) strtok(command, " \t");

/*
 * check out the command - let addition succeed if it doesn't exist (assume
 * it will be added later); fail anything else.  Do not allow using a
 * command which is in an incorrect level.
 */
	if (access(command, 0) == 0) {
		ret = stat(command, &statbuf);
		if (ret < 0) {
			Saferrno = E_SYSERR;
			pfmt (stderr,MM_ERROR,MSG1,command);
			cleandirs(tag);
			quit();
		}
		if (!(statbuf.st_mode & 0111)) {
			Saferrno = E_BADARGS;
			pfmt (stderr,MM_ERROR,MSG135,command);
			cleandirs(tag);
			quit();
		}
		if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
			Saferrno = E_BADARGS;
			pfmt (stderr,MM_ERROR,MSG136,command);
			cleandirs(tag);
			quit();
		}
	}
	else {
		pfmt (stdout,MM_WARNING,MSG137,command);
	}

/*
 * After creating directories & files, 
 *	1. set the correct ownerships on the directories & files
 *	2. set the correct levels on the directories & files
 */
	/* find level identifier of SYS_PRIVATE */
	if ( lvlin("SYS_PRIVATE", &level_pri) != 0 ) {
		if (mac_check()) {
			Saferrno = E_SYSERR;
			pfmt (stderr, MM_ERROR, MSG138);
			quit();
		} else {
			set_level = 0;
		}
	}
	
	if (set_level) {
		if ( lvlfile(pid_file, MAC_SET, &level_pri) != 0 ||
		     lvlfile(pmpipe_file, MAC_SET, &level_pri) != 0 ||
		     lvlfile(pmtab_file, MAC_SET, &level_pri) != 0 ||
		     lvlfile(home_dir, MAC_SET, &level_pri) != 0 || 
		     lvlfile(althome_dir, MAC_SET, &level_pri) != 0 ) {
			Saferrno = E_NOPRIV;
			cleandirs(tag);
			pfmt (stderr, MM_ERROR, MSG139,tag);
			quit();
		}
	}

	if ( chown(pid_file, (uid_t) SAF_OWNER, (gid_t) SAF_GROUP) != 0 ||
	     chown(pmpipe_file, (uid_t) SAF_OWNER, (gid_t) SAF_GROUP) != 0 ||
	     chown(pmtab_file, (uid_t) SAF_OWNER, (gid_t) SAF_GROUP) != 0 ||
	     chown(home_dir, (uid_t) SAF_OWNER, (gid_t) SAF_GROUP) != 0 ||
	     chown(althome_dir, (uid_t) SAF_OWNER, (gid_t) SAF_GROUP) != 0 ) {
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG140,tag);
		quit();
	}
	
/*
 * add the service to _sactab
 */
	if ((fp = open_sactab("a")) == NULL) {
		Saferrno = E_SYSERR;
		cleandirs(tag);
		pfmt (stderr, MM_ERROR, MSG90);
		quit();
	}
	(void) fprintf(fp, "%s:%s:%s:%d:%s\t#%s\n", tag, type,
		(flags ? pflags(flags, FALSE) : ""), count, buf,
		(comment ? comment : ""));
	(void) fclose(fp);


/*
 * tell the SAC to read _sactab if its there (i.e. single user)
 */

	if (sac_home())
		read_db(NULL);
	return;
}


/*
 * Procedure: cleandirs - remove anything that might have been created
 *
 * Notes: cleandirs usually called if an addition of a port monitor
 *	  fails.  Saferrno is set elsewhere; this is strictly an attempt
 *	  to clean up what mess we've left, so don't check to see if the
 *	  cleanup worked.
 *
 * Args:	tag - tag of port monitor whose trees should be removed
 *
 * Restrictions: execl(2): <none>	rmdir(2): <none>
 */

void
cleandirs(tag)
char *tag;
{
	char buf[SIZE];		/* scratch buffer */
	pid_t pid;		/* pid of fork() */
	int status;		/* child status */
	
	/* first remove /etc/saf/<tag>, note: this removes the directory too */
	(void) sprintf(buf, "%s/%s", HOME, tag);	

	if ((pid = fork()) == 0) {	/* child */
		(void) execl("/usr/bin/rm", "rm", "-f", "-r", buf, 0);
	}
	else if ( pid > 0 )
		(void) wait(&status);	/* parent */

	/* now remove /var/saf/<tag> */
	(void) sprintf(buf, "%s/%s", ALTHOME, tag);
	(void) rmdir(buf);
}


/*
 * Procedure: rem_pm - remove a port monitor
 *
 * Args:	tag - tag of port monitor to be removed
 *
 * Restrictions:  unlink(2): <none>
 */

void
rem_pm(tag)
char *tag;
{
	FILE *fp;		/* file pointer for _sactab */
	FILE *tfp;		/* file pointer for temp file */
	int line;		/* line number entry is on */
	char *tname;		/* temp file name */
	char buf[SIZE];		/* scratch buffer */

	if ((fp = open_sactab("r")) == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG90);
		quit();
	}
	
	if ((line = find_pm(fp, tag)) == 0) {
		Saferrno = E_NOEXIST;
		pfmt (stderr, MM_ERROR, MSG91, tag);
		quit();
	}
	tname = make_tempname("_sactab");
	tfp = open_temp(tname);
	if (line != 1) {
		if (copy_file(fp, tfp, 1, line - 1)) {
			(void) unlink(tname);
			Saferrno = E_SYSERR;
			pfmt (stderr, MM_ERROR, MSG99);
			quit();
		}
	}
	if (copy_file(fp, tfp, line + 1, -1)) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG99);
		quit();
	}
	(void) fclose(fp);
	if (fclose(tfp) == EOF) {
		(void) unlink(tname);
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG100);
		quit();
	}

	/* note - replace only returns if successful */
	replace(SACTAB, tname);
	
/*
 * tell the SAC to read _sactab if its there (i.e. single user)
 */

	if (sac_home())
		read_db(NULL);
	return;
}


/*
 * Procedure: start_pm - start a particular port monitor
 *
 * Args:	tag - tag of port monitor to be started
 */

void
start_pm(tag)
char *tag;
{
	struct admcmd cmd;			/* command structure */
	register struct admcmd *ap = &cmd;	/* and a pointer to it */

	ap->ac_mtype = AC_START;
	(void) strcpy(ap->ac_tag, tag);
	ap->ac_pid = getpid();
	sendcmd(ap, NULL, tag);
	return;
}


/*
 * Procedure: kill_pm - stop a particular port monitor
 *
 * Args:	tag - tag of port monitor to be stopped
 */

void
kill_pm(tag)
char *tag;
{
	struct admcmd cmd;			/* command structure */
	register struct admcmd *ap = &cmd;	/* and a pointer to it */

	ap->ac_mtype = AC_KILL;
	(void) strcpy(ap->ac_tag, tag);
	ap->ac_pid = getpid();
	sendcmd(ap, NULL, tag);
	return;
}


/*
 * Procedure: enable_pm - enable a particular port monitor
 *
 * Args:	tag - tag of port monitor to be enabled
 */

void
enable_pm(tag)
char *tag;
{
	struct admcmd cmd;			/* command structure */
	register struct admcmd *ap = &cmd;	/* and a pointer to it */

	ap->ac_mtype = AC_ENABLE;
	(void) strcpy(ap->ac_tag, tag);
	ap->ac_pid = getpid();
	sendcmd(ap, NULL, tag);
	return;
}


/*
 * Procedure: disable_pm - disable a particular port monitor
 *
 * Args:	tag - tag of port monitor to be disabled
 */

void
disable_pm(tag)
char *tag;
{
	struct admcmd cmd;			/* command structure */
	register struct admcmd *ap = &cmd;	/* and a pointer to it */

	ap->ac_mtype = AC_DISABLE;
	(void) strcpy(ap->ac_tag, tag);
	ap->ac_pid = getpid();
	sendcmd(ap, NULL, tag);
	return;
}


/*
 * Procedure: read_db - tell SAC or a port monitor to read its administrative file.
 *
 * Args:	tag - tag of port monitor that should read its
 *		      administrative file.  If NULL, it means SAC should.
 */

void
read_db(tag)
char *tag;
{
	struct admcmd cmd;			/* command structure */
	register struct admcmd *ap = &cmd;	/* and a pointer to it */

	ap->ac_mtype = (tag) ? AC_PMREAD : AC_SACREAD;
	if (tag)
		(void) strcpy(ap->ac_tag, tag);
	ap->ac_pid = getpid();
	sendcmd(ap, NULL, tag);
	return;
}


/*
 * Procedure: list_pms - request information about port monitors from
 *			 SAC and output requested info
 *
 * Args:	pmtag - tag of port monitor to be listed (may be null)
 *		pmtype - type of port monitors to be listed (may be null)
 *		oflag - true if output should be easily parseable
 */

void
list_pms(pmtag, pmtype, oflag)
char *pmtag;
char *pmtype;
int oflag;
{
	struct admcmd acmd;			/* command structure */
	register struct admcmd *ap = &acmd;	/* and a pointer to it */
	int nprint = 0;				/* count # of PMs printed */
	char *p;				/* scratch pointer */
	char *tag;				/* returned tag */
	char *type;				/* returned type */
	char *flags;				/* returned flags */
	char *rsmax;				/* returned restart count */
	char *state;				/* returned state */
	char *cmd;				/* returned command string */
	char *comment;				/* returned comment string */

/*
 * if sac isn't there (single user), provide info direct from _sactab
 * note: when this routine returns, the process exits, so there is no
 * need to free up any memory
 */

	p = NULL;
	if (sac_home()) {
		ap->ac_mtype = AC_STATUS;
		ap->ac_tag[0] = '\0';
		ap->ac_pid = getpid();
		sendcmd(ap, &p, NULL);
	}
	else {
		single_print(&p);
	}

/*
 * SAC sends back info in condensed form, we have to separate it out
 * fields come in ':' separated, records are separated by newlines
 */

	while (p && *p) {
		tag = getfield(&p, ':');	/* PM tag */
		type = getfield(&p, ':');	/* PM type */
		flags = getfield(&p, ':');	/* flags */
		rsmax = getfield(&p, ':');	/* restart count */
		state = pstate((unchar) atoi(getfield(&p, ':')));	/* state in nice output format */
		cmd = getfield(&p, ':');	/* command */
		comment = getfield(&p, '\n');	/* comment */


/*
 * print out if no selectors specified, else check to see if
 * a selector matched
 */

		if ((!pmtag && !pmtype) || (pmtag && !strcmp(pmtag, tag)) || (pmtype && !strcmp(pmtype, type))) {
			if (oflag) {
				(void) printf("%s:%s:%s:%s:%s:%s#%s\n", tag, type, pflags(atol(flags), FALSE),
						rsmax, state, cmd, comment);
			}
			else {
				if (nprint == 0) {
					(void) printf("PMTAG          PMTYPE         FLGS RCNT STATUS     COMMAND\n");
				}
				(void) printf("%-14s %-14s %-4s %-4s %-10s %s #%s\n", tag, type, pflags(atol(flags), TRUE),
						rsmax, state, cmd, comment);
			}
			nprint++;
		}
	}
	/*
	 * if we didn't find any valid ones, indicate an error (note: 1 and
	 * only 1 of the if statements should be true)
	 */
	if (nprint == 0) {
		if (pmtype)
			pfmt (stderr, MM_ERROR, MSG91, pmtype);
		else if (pmtag)
			pfmt (stderr, MM_ERROR, MSG91, pmtag);
		else if (!pmtag && !pmtype)
			pfmt (stderr, MM_ERROR, MSG141, pmtag);
		Saferrno = E_NOEXIST;
	}
	return;
}


/*
 * Procedure: getfield - retrieve and return a field from the sac
 *			 "status" string (input argument is modified to
 *			 point to next field as a side-effect) 
 *
 * Args:	p - address of remaining portion of string
 *		sepchar - field terminator character
 */

char *
getfield(p, sepchar)
char **p;
char sepchar;
{
	char *savep;	/* for saving argument */

	savep = *p;
	*p = strchr(*p, sepchar);
	if (*p == NULL) {
		Saferrno = E_SAFERR;
		pfmt (stderr, MM_ERROR, MSG142);
		return(NULL);
	}
	**p = '\0';
	(*p)++;
	return(savep);
}


/*
 * Procedure: single_print - print out _sactab if sac not at home
 *
 * Notes: this should be used only in single user mode
 *
 * Args:	p - address of pointer where formatted data should be
 *		    placed (space allocated here)
 */

void
single_print(p)
char **p;
{
	FILE *fp;				/* file pointer for _sactab */
	struct stat statbuf;			/* file status info */
	register char *tp1;			/* scratch pointer */
	register char *tp2;			/* scratch pointer */
	struct sactab stab;			/* place to hold parsed info */
	register struct sactab *sp = &stab;	/* and a pointer to it */
	char buf[SIZE];				/* scratch buffer */

	if ((fp = open_sactab("r")) == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG91);
		quit();
	}
	
	if (fstat(fileno(fp), &statbuf) < 0) {
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG91);
		quit();
	}

/*
 * allocate space to build return string, twice file size should be more
 * than enough (and make sure it's zero'ed out)
 */

	tp1 = calloc(2 * statbuf.st_size, sizeof(char));
	if (tp1 == NULL) {
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG143);
		quit();
	}

/*
 * read the file and build the string
 */

	while (fgets(buf, SIZE, fp)) {
		tp2 = trim(buf);
		if (*tp2 == '\0')
			continue;
		parse(tp2, &stab);
		(void) sprintf(buf, "%s:%s:%d:%d:%d:%s:%s\n", sp->sc_tag, sp->sc_type,
			sp->sc_flags, sp->sc_rsmax, SSTATE, sp->sc_cmd, sp->sc_comment);
		(void) strcat(tp1, buf);
		free(sp->sc_cmd);
		free(sp->sc_comment);
	}
	if (!feof(fp)) {
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG108);
		quit();
	}
	(void) fclose(fp);

/*
 * point at the just-built string
 */

	*p = tp1;
	return;
}


/*
 * Procedure: sendcmd - send a command to the SAC
 *
 * Args:	ap - pointer to command to send
 *		info - pointer to return information from the SAC
 *		tag - tag of port monitor to which the command applies (may
 *		      be NULL)
 *
 * Notes: The opening of the _cmdpipe for writing requires P_MACWRITE
 * 
 * Restrictions: open(2): <none>	lockf: <none>
 */

void
sendcmd(ap, info, tag)
struct admcmd *ap;
char **info;
char *tag;
{
	int fd;		/* file descriptor associated with command pipe */

	/* open up command pipe to SAC for read & write */
	fd = open(CMDPIPE, O_RDWR);
	if (fd < 0) {
		Saferrno = E_SYSERR;

		fprintf (stderr, "%s",gettxt(MSGID185, MSG185)); 
		quit();
	}

/*
 * lock pipe to insure serial access, lock will disappear if process dies
 */

	if (lockf(fd, F_LOCK, 0) < 0) {
		Saferrno = E_SYSERR;
		pfmt (stderr, MM_ERROR, MSG144);
		quit();
	}

	/* ged rid of any garbage that may be hanging around */
	(void)ioctl(fd, I_FLUSH, FLUSHRW);

	if (write(fd, ap, sizeof(struct admcmd)) < 0) {
		Saferrno = E_SYSERR;
		fprintf (stderr, "%s",gettxt(MSGID185, MSG185));
		quit();
	}
	checkresp(fd, info, tag);

/*
 * unlock the command pipe - not really necessary since we're about to close
 */

	(void) lockf(fd, F_ULOCK, 0);
	(void) close(fd);
	return;
}


/*
 * Procedure: checkresp - check the SAC's response to our command
 *
 * Args:	fd - file descriptor of command pipe
 *		info - pointer to return and info send along by SAC
 *		tag - tag of port monitor that the command had been
 *		      for, only used for error reporting
 */

void
checkresp(fd, info, tag)
int fd;
char **info;
char *tag;
{
	struct admack ack;			/* acknowledgment struct */
	register struct admack *ak = &ack;	/* and a pointer to it */
	pid_t pid;				/* my pid */
	struct sigaction sigact;		/* signal handler setup */

/*
 * make sure this ack is meant for me, put an alarm around the read
 * so we don't hang out forever.
 */

	pid = getpid();
	sigact.sa_flags = 0;
	sigact.sa_handler = catch;
	(void) sigemptyset(&sigact.sa_mask);
	(void) sigaddset(&sigact.sa_mask, SIGALRM);
	(void) sigaction(SIGALRM, &sigact, NULL);
	(void) alarm(10);
	do {
		if (read(fd, ak, sizeof(ack)) != sizeof(ack)) {
			Saferrno = E_SYSERR;
			fprintf (stderr, "%s",gettxt(MSGID185, MSG185));
			quit();
		}
	} while (pid != ak->ak_pid);
	(void) alarm(0);

/*
 * check out what happened
 */

	switch (ak->ak_resp) {
	case AK_ACK:
		/* everything was A-OK */
		if (info && ak->ak_size) {
			char *bp;
			int got;

			/* there is return info and a place to put it */
			if ((*info = malloc((unsigned) (ak->ak_size + 1))) == NULL) {
				Saferrno = E_SYSERR;
				pfmt (stderr, MM_ERROR, MSG143);
				quit();
			}

			bp = *info;
			do {
				if ((got = read(fd, bp, (unsigned) ak->ak_size)) <= 0) {
					Saferrno = E_SYSERR;
					fprintf (stderr, "%s",gettxt(MSGID185, MSG185));
		/*fprintf (stderr, "%s",gettxt("uxsaf:185", "Can not contact SAC")); */
					quit();
				}
				else
				{
					bp += got;
					ak->ak_size -= got;
				}
			} while (ak->ak_size > 0);
			/* make sure "string" is null-terminated */
			*bp = '\0';
		}
		return;
	/* something went wrong - see what */
	case AK_PMRUN:
		Saferrno = E_PMRUN;
		pfmt (stderr, MM_ERROR, MSG145, tag);
		break;
	case AK_PMNOTRUN:
		Saferrno = E_PMNOTRUN;
		pfmt (stderr, MM_ERROR, MSG64, tag);
		break;
	case AK_NOPM:
		Saferrno = E_NOEXIST;
		pfmt (stderr, MM_ERROR, MSG91, tag);
		break;
	case AK_UNKNOWN:
		Saferrno = E_SAFERR;
		pfmt (stderr, MM_ERROR, MSG146, tag);
		break;
	case AK_NOCONTACT:
		Saferrno = E_SAFERR;
		pfmt (stderr, MM_ERROR, MSG147, tag);
		break;
	case AK_PMLOCK:
		Saferrno = E_SAFERR;
		pfmt (stderr, MM_ERROR, MSG148, tag);
		break;
	case AK_RECOVER:
		Saferrno = E_RECOVER;
		pfmt (stderr, MM_ERROR, MSG149, tag);
		break;
	case AK_REQFAIL:
		Saferrno = E_SAFERR;
		pfmt (stderr, MM_ERROR, MSG150);
		break;
	default:
		Saferrno = E_SAFERR;
		pfmt (stderr, MM_ERROR, MSG151);
		break;
	}
}


/*
 * Procedure: catch - catcher for SIGALRM, don't need to do anything
 */

void
catch()
{
}


/*
 * Procedure: pflags - put port monitor flags into intelligible form for output
 *
 * Args:	flags - binary representation of flags
 *		dflag - true if a "-" should be returned if no flags
 */

char *
pflags(flags, dflag)
long flags;
int dflag;
{
	register int i;			/* scratch counter */
	static char buf[SIZE];		/* formatted flags */

	if (flags == 0) {
		if (dflag)
			return("-");
		else
			return("");
	}
	i = 0;
	if (flags & D_FLAG) {
		buf[i++] = 'd';
		flags &= ~D_FLAG;
	}
	if (flags & X_FLAG) {
		buf[i++] = 'x';
		flags &= ~X_FLAG;
	}
	if (flags) {
		pfmt (stderr, MM_ERROR, MSG152);
		exit(1);
	}
	buf[i] = '\0';
	return(buf);
}


/*
 * Procedure: sac_home - returns true is sac has a lock on its logfile,
 *				 false otherwise (useful to avoid errors
 *				       for administrative actions in
 *				       single user mode) 
 *
 * Restrictions: open(2): <none>	lockf: <none>
 */

sac_home()
{
	int fd;		/* fd to sac logfile */

	fd = open(LOGFILE, O_RDONLY);
	if (fd < 0) {
		pfmt (stdout, MM_WARNING, MSG152);
		return(FALSE);
	}
	if (lockf(fd, F_TEST, 0) < 0) {
		/* everything is ok */
		(void) close(fd);
		return(TRUE);
	}
	else {
		/* no one home */
		(void) close(fd);
		return(FALSE);
	}
}

