/*		copyright	"%c%" 	*/

#ident	"@(#)uux.c	1.2"
#ident  "$Header$"

#include "uucp.h"
#include <cr1.h>
#include <pwd.h>
#include <wait.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>

pid_t child = 0;

static void onintr(int);
static int pass_key();

void cleanup();

/*
 *	uux (front end) to retrieve (effective) user's key for uuxcmd
 */

main(int argc, char **argv)
{
	char *getprm();
	char inargs[BUFSIZ];
	char *ap;
	char prm[BUFSIZ];
	char rest[BUFSIZ];
	char xsys[NAMEBUF];
	int i;
	int ret;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxbnu.abi");
	(void)setlabel("UX:uux");

	Uid = getuid();
	Euid = geteuid();
	Gid = getgid();
	Egid = getegid();

	/*
	 * determine local system name
	 */
	(void) strcpy(Progname, "uux");
	(void) signal(SIGILL, onintr);
	(void) signal(SIGTRAP, onintr);
	(void) signal(SIGIOT, onintr);
	(void) signal(SIGEMT, onintr);
	(void) signal(SIGFPE, onintr);
	(void) signal(SIGBUS, onintr);
	(void) signal(SIGSEGV, onintr);
	(void) signal(SIGSYS, onintr);
	(void) signal(SIGINT, onintr);
	(void) signal(SIGTERM, SIG_IGN);
	uucpname(Myname);

	/*
	 *"prcsess" arguments like uuxcmd would. We only really
	 * process -x (debug level) here, but pass all on to uuxcmd.
	 */

	for (i=1; i<argc  &&  *argv[i] == '-'; i++)
	    if (EQUALS(argv[i], "-"))
		argv[i] = "-p";

	while ((i = getopt(argc, argv, "a:bcCg:jnprs:x:z")) != EOF) {
		switch(i){

		case 'a':
		case 'b':
		case 'c':
		case 'C':
		case 'g':
		case 'j':
		case 'n':
		case 'p':
		case 'r':
		case 's':
		case 'z':
			break;
		case 'x':
			Debug = atoi(optarg);
			if (Debug <= 0)
				Debug = 1;
			break;

		default:
			usage();
			break;
		}
	}

	if( optind >= argc )
		usage();

	DEBUG(4, "\n** %s **\n", "START (uux)");

	strcpy(Progname, "uux");
	setservice(NULL);

	/*
	 * copy arguments into a buffer for later processing
	 */

	inargs[0] = '\0';
	for (; optind < argc; optind++) {
		DEBUG(4, "arg - %s:", argv[optind]);
		(void) strcat(inargs, " ");
		(void) strcat(inargs, argv[optind]);
	}

	DEBUG(4, "arg - %s\n", inargs);

	/*
	 * find remote system name
	 * remote name is first to know that 
	 * is not > or <
	 */
	ap = inargs;
	xsys[0] = '\0';
	while ((ap = getprm(ap, NULL, prm)) != NULL) {
		if (prm[0] == '>' || prm[0] == '<') {
			ap = getprm(ap, NULL, prm);
			continue;
		}

		/*
		 * split name into system name
		 * and command name
		 */
		(void) split(prm, xsys, CNULL, rest);
		break;
	}
	if (xsys[0] == '\0')
		(void) strcpy(xsys, Myname);
	strncpy(Rmtname, xsys, MAXBASENAME);
	Rmtname[MAXBASENAME] = '\0';
	DEBUG(4, "xsys %s\n", xsys);

	/*
	 * check to see if system name is valid
	 */
	if (versys(xsys) != 0) {
		/*
		 * bad system name
		 */
		pfmt(stderr, MM_ERROR, ":67:<%s>: bad system name\n", xsys);
		cleanup(8);
	}

	(void) mchFind(xsys);

	DEBUG(6, "User %s\n", User);

	ret = pass_key(xsys, argv);
	cleanup(ret);
}

/*
 * cleanup and unlink if error
 *	code	-> exit code
 * return:
 *	none
 */
void
cleanup(code)
register int code;
{

	DEBUG(1, "exit code %d\n", code);
	exit(code);
}

/*
 *	uux (front end) to get the "key" for the effective user.
 *	and pass it in to the real uux command.
 */


static int
pass_key(char *remote, char **argv)
{
	char buffer[BUFSIZ];
	struct passwd *entry;
	char *user_key;
	int ret, status, sig;
	size_t length;
	int fds[2];
	char key_master[128];

	keys(key_master);	/* get the name of the key mgmt scheme */

	(void)sprintf(buffer, "%s@%s", "uucp", remote);
			
	if ( (entry = getpwuid(geteuid()))
		&&  (user_key = getkey(key_master, entry->pw_name, buffer)) ) {
		sprintf(buffer, "%s %s", entry->pw_name, user_key);
		length = strlen(buffer) + 1;
		DEBUG(3, "passing %s's key to uuxcmd\n", entry->pw_name);
	} else {
		if (AuthReq()) {
			pfmt(stderr, MM_ERROR,
				":68:Cannot perform required authentication.\n");
			cleanup(31);
		}
		length = 0;
	}

	switch (child = fork()) {

	case 0:		/* child */

		if (pipe(fds) == -1) {
			pfmt(stderr, MM_ERROR, ":70:pipe() failed errno = %d.\n", errno);
			close(3);
		} else {
			if (write(fds[1], buffer, length) != length) {
			pfmt(stderr, MM_ERROR, ":71:write() to a pipe failed, errno=%d.\n", errno);
				exit(30);
			}
			close(fds[1]);
			if (fds[0] != 3) {
				close(3);
				if (dup(fds[0]) != 3)
					pfmt(stderr, MM_ERROR, ":72:could not set up pipe fd=3\n");
			}
		}
		execv(UUXCMD, argv);
		pfmt(stderr, MM_ERROR, ":73:cannot execute <%s>\n",UUXCMD);
		exit(9);
		break;

	case -1:	/* fork failed */

		pfmt(stderr, MM_ERROR, ":74:fork failed, errno=<%d>\n", errno);
		cleanup(20);
		break;

	default:	/* parent */

		while (ret = wait(&status)) {
			if (ret == -1 && errno != EINTR) {
				pfmt(stderr, MM_ERROR, ":75:wait failed, errno=%d\n", errno);
				ret = 33;
				break;
			}
			if (ret == child)
				if (WIFEXITED(status)) {
					ret = WEXITSTATUS(status);
					DEBUG(5, "Child exited (%d)\n", ret);
					break;
				}
				if (WIFSIGNALED(status)) {
					sig = WTERMSIG(status);
					pfmt(stderr, MM_ERROR, ":69:received signal <%d>\n", sig);
					ret = 29;
					break;
				}
		}

		DEBUG(5,"uux exiting from uuxcmd.\n","");
		break;
	}

	return(ret);
}

/*
 * catch signal then cleanup and exit
 */
static void
onintr(inter)
register int inter;
{
	/* the parent should send the interrupt to the child */
	/* the child will register its own interrupt handler */

	if ( child > 0 ) {
		pfmt(stderr, MM_ERROR, ":76:sending signal %d to %ld\n", inter, child);
		kill(child, inter);
	}
	signal(inter, onintr);
	return;
}

usage()
{

	(void) pfmt(stderr, MM_ACTION,
	":77:Usage: uux [-aNAME] [-b] [-c] [-C] [-j] [-gGRADE] [-n] [-p] \\\n");
	(void) pfmt(stderr, MM_NOSTD,
	":78:\t\t\t[-r] [-sFILE] [-xNUM] [-z] command-string\n");
	cleanup(32);
}
