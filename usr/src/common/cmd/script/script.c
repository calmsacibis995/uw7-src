#ident	"@(#)script:script.c	1.1.7.2"
#ident  "$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * script
 */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stropts.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/secsys.h>
#include <priv.h>
#include <sys/mac.h>

#ifndef	FALSE
#define	FALSE	0
#endif	/* FALSE */
#ifndef	TRUE
#define	TRUE	(!FALSE)
#endif	/* TRUE */

int	Priv_Flag	=TRUE,	/* True when process is privileged */
	Admin_Flag	=FALSE;	/* True when we we are the administrator */

/* Am_I_Admin will determine if the code will run privileged
* in areas other than those used that manipulate pseudo ttys.
*
* If the effective user id "the administrator" or secsys(ES_PRVID, 0),
* the daemon must run with privileges in the working set turned on.
* If the effective user id is NOT secsys(ES_PRVID, 0),
* the daemon must run without privileges.
*
* This information is stored in Admin_Flag, and does not change since
* there are no set[ear]uid(2) calls in this code.
*
* The (_loc_privid == getuid()) code provides COMPATIBILITY with
* UID based privilege mechanism.
*/

void
Am_I_Admin()
{
	int	_loc_olderrno;

	uid_t	_loc_id_priv;

	_loc_olderrno	= errno;
	if (((_loc_id_priv = secsys(ES_PRVID, 0)) >= 0)
		&& (geteuid() == _loc_id_priv))
			Admin_Flag = TRUE;
		else	Admin_Flag = FALSE;
	errno = _loc_olderrno;
}


/* CLR_WORKPRIVS_NON_ADMIN should be used after
* a section of code that manipulates pseudo-ttys to
* re-compute the privileges appropriate
* for that user.  It is 
*
* This information is stored in Admin_Flag, and does not change since
* there are no set[ear]uid(2) calls in this code.  Cache the fact that
* privileges are set or cleared in Priv_Flag.
*/

void
CLR_WORKPRIVS_NON_ADMIN()
{
	int	_loc_priv_cmd = CLRPRV,
		_loc_olderrno;

	uid_t	_loc_id_priv;

	/* if we are the administrator AND we are not privileged, SET them */
	if (Admin_Flag && !Priv_Flag) {
		_loc_priv_cmd = SETPRV;
	}

	/* if the above SETPRV happened OR
	 * we are not the administrator and we are privileged
	 *	(therefore we must clear privileges),
	 * THEN do the procprivl and adjust Priv_Flag
	 */
	if ((SETPRV == _loc_priv_cmd) ||
	    (Priv_Flag && !Admin_Flag)) {
		_loc_olderrno = errno;
		procprivl(_loc_priv_cmd, pm_work(P_ALLPRIVS), 0);
		Priv_Flag = (SETPRV == _loc_priv_cmd);
		errno = _loc_olderrno;
	}
}

/* ENABLE_WORK_PRIVS is a macro placed directly before
* code that requires access to network devices or otherwise
* restricted resources.  In particular, this must be used in
* services that require privilege to open reserved ports
* and then must turn off the privilege before performing
* a user request in this same process - or the exec is
* not soon enough in the code path.
* This prevents a network user from exploiting fixed
* privileges for the particular file the user will request.
* Since the privileges are manipulated in the Working set but
* are still in the MAX set, these privileges are not "lost"
* and can be enabled/disabled as particular code segments need.
* Used with CLR_WORKPRIVS_NON_ADMIN.
*/

void
ENABLE_WORK_PRIVS()
{
	int	_loc_olderrno;

	if (!Priv_Flag) {
		_loc_olderrno = errno;
		procprivl(SETPRV, pm_work(P_ALLPRIVS), 0);
		Priv_Flag = TRUE;
		errno = _loc_olderrno;
	}
}

/* CLR_MAXPRIVS_FOR_EXEC is a macro placed directly before
* a service potentially exec's a shell stricly for a user
* request.  This prevents a user from gaining inheritable
* privileges for the particular file the user will request.
*
* If this kernel is running with an ID based privilege
* mechanism, check if  that  ID is equal to ``_loc_privid''.
* If so, set  the ``_loc_clrprivs'' flag to  0 so privileges
* are NOT cleared for a process with the ``privileged''
* ID before ``exec''ing the shell.
*
* If it is not ID based, then clear ALL process privileges
*
* This information is stored in Admin_Flag, and does not change since
* there are no set[ear]uid(2) calls in this code.
*
* IMPORTANT: Since this may clear all of the process privileges,
* if the exec() call following this fails, this process will resume
* without privileges, which may leave you dead in the water.
*
* NOTE: This macro is should not be used when the daemon does intend
* for the exec'd binary to have privileges.  This macro need not be
* always used before an exec when the absolute pathname ALWAYS
* is known not to inherit privileges (/usr/bin/sh, for example).
*/

void
CLR_MAXPRIVS_FOR_EXEC()
{
	int	_loc_privid,
		_loc_olderrno;
	uid_t	_loc_id_priv;

	_loc_olderrno = errno;
	if (!Admin_Flag) {
		procprivl(CLRPRV, pm_max(P_ALLPRIVS), 0);
	}
	errno = _loc_olderrno;
}

int 	grantpt();
int 	unlockpt();
char	*ptsname();

char	*getenv();
char	*ctime();
char	*shell;
FILE	*fscript;
int	master;			/* file descriptor for master pseudo-tty */
int	slave;			/* file descriptor for slave pseudo-tty */
int	child;
int	subchild;
char	*fname = "typescript";
void	sigwinch();
void	finish();

struct	termios b;
struct	winsize size;
int	lb;
int	l;
char	*mptname = "/dev/ptmx";	/* master pseudo-tty device */

int	aflg;

main(argc, argv)
	int argc;
	char *argv[];
{
	uid_t ruidt;
	gid_t gidt;

	/* set global privileged administartor flag,
	 * clear privileges on non-administrators
	 */
	Am_I_Admin();
	CLR_WORKPRIVS_NON_ADMIN();

	shell = getenv("SHELL");
	if (shell == 0)
		shell = "/bin/sh";
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {

		case 'a':
			aflg++;
			break;

		default:
			fprintf(stderr,
			    "usage: script [ -a ] [ typescript ]\n");
			exit(1);
		}
		argc--, argv++;
	}
	if (argc > 0)
		fname = argv[0];
	ruidt = getuid();
	gidt = getgid();
	if ((fscript = fopen(fname, aflg ? "a" : "w")) == NULL) {
		perror(fname);
		fail();
	}
	chown(fname, ruidt, gidt);
	getmaster();
	printf("Script started, file is %s\n", fname);
	fixtty();

	(void) signal(SIGCHLD, finish);
	child = fork();
	if (child < 0) {
		perror("fork");
		fail();
	}
	if (child == 0) {
		subchild = child = fork();
		if (child < 0) {
			perror("fork");
			fail();
		}
		if (child)
			dooutput();
		else
			doshell();
	}
	doinput();
}

doinput()
{
	char ibuf[BUFSIZ];
	int cc;

	(void) fclose(fscript);
	signal(SIGWINCH, sigwinch);

	{
		/* Need to wait till the device is public,
		 * another fork()/exec() is taking care of it
		 */
		struct devstat atrib;
		int delay = 15, sleeptime = 2;

		/* Need privilege to get status */
		ENABLE_WORK_PRIVS();
		while ((delay > 0) &&
		       (0==fdevstat(master, DEV_GET, &atrib)) &&
		       (DEV_PRIVATE == atrib.dev_state) ) {
			delay--;
			sleep(sleeptime);
		}
		/* Never need privilege */
		CLR_MAXPRIVS_FOR_EXEC();
	}

	while ((cc = read(0, ibuf, BUFSIZ)) > 0)
		(void) write(master, ibuf, cc);
	done();
}

void
sigwinch()
{
	struct winsize ws;

	if (ioctl(0, TIOCGWINSZ, &ws) == 0)
		(void) ioctl(master, TIOCSWINSZ, &ws);
}

#include <wait.h>
#include <sys/wait.h>

void
finish()
{
	int status;

	if (child==waitpid(child, &status, WNOHANG))
		done();
}

dooutput()
{
	time_t tvec;
	char obuf[BUFSIZ];
	int cc;

	(void) close(0);
	tvec = time((time_t *)0);
	fprintf(fscript, "Script started on %s", ctime(&tvec));

	{
		/* Need to wait till the device is public,
		 * another fork()/exec() is taking care of it
		 */
		struct devstat atrib;
		int delay = 15, sleeptime = 2;

		/* Need privilege to get status */
		ENABLE_WORK_PRIVS();
		while ((delay > 0) &&
		       (0==fdevstat(master, DEV_GET, &atrib)) &&
		       (DEV_PRIVATE == atrib.dev_state) ) {
			delay--;
			sleep(sleeptime);
		}
		/* Never need privilege */
		CLR_MAXPRIVS_FOR_EXEC();
	}

	for (;;) {
		cc = read(master, obuf, sizeof (obuf));
		if (cc <= 0)
			break;
		(void) write(1, obuf, cc);
		(void) fwrite(obuf, 1, cc, fscript);
	}
	done();
}

doshell()
{

	setpgrp();	/* relinquish control terminal */
	getslave();
	(void) close(master);
	(void) fclose(fscript);
	(void) dup2(slave, 0);
	(void) dup2(slave, 1);
	(void) dup2(slave, 2);
	(void) close(slave);
	/* clear privileges on non-administrators */
	CLR_MAXPRIVS_FOR_EXEC();
	execl(shell, shell, NULL);
	perror(shell);
	fail();
}

fixtty()
{
	struct termios sbuf;

	sbuf = b;
	sbuf.c_iflag &= ~(INLCR|IGNCR|ICRNL|IUCLC|IXON);
	sbuf.c_oflag &= ~OPOST;
	sbuf.c_lflag &= ~(ICANON|ISIG|ECHO);
	sbuf.c_cc[VMIN] = 1;
	sbuf.c_cc[VTIME] = 0;
	(void) ioctl(0, TCSETSW, (char *)&sbuf);
}

fail()
{

	(void) kill(0, SIGTERM);
	done();
}

done()
{
	time_t tvec;

	if (subchild) {
		tvec = time((time_t *)0);
		fprintf(fscript,"\nscript done on %s", ctime(&tvec));
		(void) fclose(fscript);
		(void) close(master);
	} else {
		(void) ioctl(0, TCSETSW, (char *)&b);
		printf("Script done, file is %s\n", fname);
	}
	exit(0);
}

getmaster()
{
	struct stat stb;

	/* enable privs to access device */
	ENABLE_WORK_PRIVS();
	if ( (master=open(mptname, O_RDWR)) >= 0 ) {	/* a pseudo-tty is free */
		/* clear privileges on non-administrators */
		CLR_WORKPRIVS_NON_ADMIN();
		(void) ioctl(0, TCGETS, (char *)&b);
		(void) ioctl(0, TIOCGWINSZ, (char *)&size);
		return;
	} else {					/* out of pseudo-tty's */
		/* clear privileges on non-administrators */
		CLR_WORKPRIVS_NON_ADMIN();
		perror(mptname);
		fprintf(stderr, "Out of pseudo-tty's\n");
		fail();
	}
}

getslave()
{
	char *slavename = "/dev/pts/?";	/* name of slave pseudo-tty */

	/* enable privs to access device */
	ENABLE_WORK_PRIVS();
	grantpt(master);	  		/* change slave permissions */
	/* clear privileges on non-administrators */
	CLR_WORKPRIVS_NON_ADMIN();
	if ((0!=unlockpt(master)) ||			/* unlock slave */
	    (NULL==(slavename = ptsname(master))) ||	/* get name of slave */
	    (0>(slave = open(slavename, O_RDWR)))) {	/* open slave */
		perror(slavename);
		fail();
	}
	ioctl(slave, I_PUSH, "ptem");		/* push pt hw emulation module */
	ioctl(slave, I_PUSH, "ldterm");		/* push line discipline */

	(void) ioctl(slave, TCSETSF, (char *)&b);
	(void) ioctl(slave, TIOCSWINSZ, (char *)&size);
}
