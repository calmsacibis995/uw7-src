#ident "@(#)main.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "dhcpd.h"
#include "proto.h"
#include "pathnames.h"

/*
 * The following are set by command-line options.
 */

char *config_file;		/* configuration file name */
int standalone;			/* mode: 0 -> inetd, 1 -> standalone */
int debug;			/* debugging level */
int bootp_fwd;			/* if 1, forward BOOTP packets */
struct sockaddr_in bootpd_addr;	/* address at which bootpd is listening */

void
done(int exit_code)
{
	(void) unlink(_PATH_PID_FILE);
	if (debug) {
		report(LOG_INFO, "Exiting (%d)", exit_code);
	}
	exit(exit_code);
}

static void
killed(int sig)
{
	report(LOG_INFO, "Terminated by signal %d.", sig);
	done(1);
}

void
main(int argc, char *argv[])
{
	FILE *pidfp;
	int o;
	static struct sockaddr_in bpdaddr;
	struct servent *sp;
	int val, timeout, pid;

	/*
	 * If fd 0 is a transport endpoint, we were started by inetd;
	 * otherwise, we are running standalone.
	 */
	
	if (is_endpt(0)) {
		standalone = 0;
	}
	else {
		standalone = 1;
	}

	/*
	 * Process command line options.
	 */
	
	debug = 0;
	config_file = _PATH_CONFIG_FILE;
	timeout = 15;
	bootp_fwd = 0;
	while ((o = getopt(argc, argv, "t:D:b:")) != -1) {
		switch (o) {

		case 't':
			if (sscanf(optarg, "%d", &val) != 1 || val < 0) {
				report(LOG_ERR, "Invalid timeout value %s.\n",
					optarg);
				break;
			}
			timeout = val;
			break;
		
		case 'D':
			if (sscanf(optarg, "%d", &val) != 1 || val < 0) {
				report(LOG_ERR, "Invalid debug value %s.\n",
					optarg);
				break;
			}
			debug = val;
			break;
		
		case 'b':
			memset(&bootpd_addr, 0, sizeof(struct sockaddr_in));
			bootpd_addr.sin_family = AF_INET;
			bootpd_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			if (sp = getservbyname(optarg, "udp")) {
				bootpd_addr.sin_port = sp->s_port;
			}
			else if (sscanf(optarg, "%d", &val) == 1
			    && val > 0 && val <= 65535) {
				bootpd_addr.sin_port = htons((u_short) val);
			}
			else {
				report(LOG_ERR, "Invalid bootpd port %s.\n",
					optarg);
				break;
			}
			bootp_fwd = 1;
			break;
		
		default:
			report(LOG_ERR, "Usage: dhcpd [-D level] [-t timeout] [-b bootp-port] [configfile]\n");
			done(1);
		}
	}

	if (optind < argc) {
		config_file = argv[optind];
	}

	if (standalone) {
		timeout = 0;
	}

	if (standalone && debug < 3) {
		if ((pid = fork()) == -1) {
			perror("dhcpd: fork failed");
			done(1);
		}
		else if (pid) {
			exit(0);
		}
		setsid();
	}

	if (!standalone || debug < 3) {
		openlog("dhcpd", LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_DAEMON);
	}

	if (debug) {
		report(LOG_INFO, "Starting.");
	}

	/*
	 * Record our process ID.
	 */

	if (pidfp = fopen(_PATH_PID_FILE, "w")) {
		fprintf(pidfp, "%d\n", getpid());
		fclose(pidfp);
	}
	else {
		report(LOG_WARNING, "Unable to create PID file %s: %m",
			_PATH_PID_FILE);
	}

	/*
	 * Ignore SIGPIPE, which we might get if we lose the connection
	 * to the address server.
	 */
	
	sigset(SIGPIPE, SIG_IGN);

	sigset(SIGTERM, killed);

	/*
	 * Initialize the database and read the configuration file.
	 */

	if (!init_database()) {
		done(1);
	}
	configure();

	proto(timeout);
}
