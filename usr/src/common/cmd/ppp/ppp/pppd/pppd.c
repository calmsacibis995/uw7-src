#ident	"@(#)pppd.c	1.6"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <termios.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <dlfcn.h>
#include <sys/fcntl.h>
#include <string.h>
#include "psm.h"
#include "pathnames.h"

char *program_name;
FILE *log_fp = stdout;
int user_debug = 0;	/* Set if administrator requires generalised debug */
int pppd_debug = 0; 	/* Hold other debug flags (in psm.h) */

/*
 * This is the entry point for the PPP Daemon.
 *
 * Parse command line arguments, open a logfile if requested
 * and then fork to become a deamon process (we also create
 * a psuedo tty that becomes our controlling tty).
 */
main(int argc, char *argv[])
{
	int c, pid, pty, i = 0;
	extern char *optarg;
	extern int optind;
	int errflg = 0;
	void *ppp_handle;
	int (*ppp_main)();
	char *logfile = DEFAULT_LOG;
	char *ptsnm;
	int ptm, pts, ret, fg = 0;

        (void)setlocale(LC_ALL,"");     
        (void)setcat("uxppp");
        (void)setlabel("UX:pppd");     

	program_name = strrchr(argv[0],'/');
	program_name = program_name ? program_name + 1 : argv[0];

	/*
	 * Process command line options
	 */
	while ((c = getopt(argc, argv, "dfl:")) != EOF) {
		switch (c) {
		case 'l':
			logfile = optarg;
			break;
		case 'd':
			user_debug++;
			break;
		case 'f':
			fg++;
			break;
		case '?':
			errflg++;
			break;
		}
	}

	if (errflg) {
		fprintf(stderr, "usage: pppd [-d] [-l <log file>]\n");
		exit(1);
	}

	/* Don't create anything users can access ! */
	umask(0077);

	/* Open the logfile */

	if (logfile) {
		log_fp = fopen(logfile, "a+");
		if (!log_fp) {
			fprintf(stderr,
				"Unable to open log file %s for writing\n",
				logfile);
			exit(1);
		}
	}

	/* Check environment variables */
	if (getenv(PPPD_DEBUG)) {
		pppd_debug |= DEBUG_PPPD;
		/* pppd_debug implies user_debug */
		user_debug++;
	}
	if (getenv(ANVL_TESTS))
		pppd_debug |= DEBUG_ANVL;
	
	if (fg) {
		/* Run in the foregroung .. load the config */

		pid = fork();
		switch (pid) {
		case -1:
			ppplog(MSG_ERROR, 0,
			       "main: fork() failed, %d\n", errno);
			exit(1);
		case 0:
			nap(500);
			do {
				ret = system(PPPTALK_INIT);
				ppplog(MSG_DEBUG, 0,
				       "System %s returns 0x%x\n",
				       PPPTALK_INIT, ret);
				if (ret) {
					i++;
					sleep(2 * i);
				}
			} while (ret && i < 5);

			if (ret) 
				kill(getppid(), SIGTERM);

			exit(ret);
		default:
			break;
		}

	} else {
		/* Switch to a daemon process */

		pid = fork();
		if (pid > 0) {
			signal(SIGPIPE, SIG_IGN);
			nap(500);
			do {
				ret = system(PPPTALK_INIT);
				ppplog(MSG_DEBUG, 0,
				       "System %s returns 0x%x\n",
				       PPPTALK_INIT, ret);
				if (ret) {
					i++;
					sleep(2 * i);
				}
			} while (ret && i < 5);
			if (ret) 
				kill(pid, SIGTERM);

			exit(ret);
		} else if (pid < 0) {
			ppplog(MSG_ERROR, 0,
			       "main: fork() failed, %d\n", errno);
			exit(1);
		}

		/* We are the child process ... */

		ptm = open(DEV_PTM, O_RDWR);
		if (ptm < 0) {
			ppplog(MSG_ERROR, 0,
			       "main: Failed to open %s\n", DEV_PTM);
			exit(1);
		}

		if (grantpt(ptm) < 0) {
			ppplog(MSG_ERROR, 0,
			       "main: Failed to grantpt\n");
			exit(1);
		}

		if (unlockpt(ptm) < 0) {
			ppplog(MSG_ERROR, 0,
			       "main: Failed to unlockpt\n");
			exit(1);
		}

		ptsnm = ptsname(ptm);
		if (!ptsname) {	
			ppplog(MSG_ERROR, 0, "main: ptsname failed\n");
			exit(1);
		}

		setsid();

		close(0);
		close(1);
		close(2);

		pts = open(ptsnm, O_RDWR);
		if (pts < 0) {
			ppplog(MSG_ERROR, 0,
			       "main: Failed to open %s\n", ptsnm);
			exit(1);
		}

		dup(pts);
		dup(pts);
	}

	/* Change to be a daemon process */
	
	ppp_handle = dlopen(PPPRT, RTLD_LAZY);
	if (!ppp_handle) {
		ppplog(MSG_ERROR, 0,
		       "main: Error loading libppprt.so : dlerror says '%s'\n",
		       dlerror());
		exit(1);
	}

	/* Find the entry point */

	ppp_main = (int (*)())dlsym(ppp_handle, "pppstart");
	if (!ppp_main) {
		ppplog(MSG_ERROR, 0,
		       "main: Symbol pppstart not found in libppprt\n");
		exit(1);
	}

	(*ppp_main)();
}
