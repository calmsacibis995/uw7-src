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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <aas/aas.h>
#include <aas/aas_proto.h>
#include <aas/aas_util.h>
#include <db.h>
#include "aasd.h"
#include "pathnames.h"
#include "atype.h"
#include "addr_db.h"

extern AddressFamily family_inet, family_unix;

AddressFamily *families[] = {
	&family_inet,
	&family_unix
};

#define NFAMILIES	(sizeof(families) / sizeof(families[0]))

/*
 * The following hold path names for the configuration file, the
 * directory in which the pool database files reside, and the directory
 * into which checkpoints are written.  The database directory and
 * checkpoint directory are specified separately so that they can be
 * placed on different file systems for added robustness.
 */

char *config_file;	/* config file */
char *db_dir = NULL;	/* database directory */
char *cp_dir;		/* checkpoint directory */

int cp_intvl;		/* seconds between checkpoints */
int cp_num;		/* number of checkpoints to keep */
int db_max;		/* trans file size that triggers compression */

Password *passwords = NULL;	/* list of valid passwords */

/*
 * config_ok is set based on the result of the configuration
 * operation.  If the configuration fails, requests (other than RECONFIG)
 * are denied.
 */

int config_ok;

/*
 * debug is the debugging level, as follows:
 * 0: none
 * 1: some
 * 2: more
 * 3: same as 2 but runs in foreground with debugging messages
 *    written to stderr instead of syslog.
 */

int debug;

static void
usage(void)
{
	report(LOG_ERR, "Usage: aasd [-c config-file] [-D debug-level]");
	exit(1);
}

static void
sigterm(int sig)
{
	report(LOG_INFO, "Address Allocation Server terminated.");
	(void) unlink(_PATH_PID_FILE);
	exit(0);
}

void
main(int argc, char *argv[])
{
	Endpoint *endpoints, **eplink, *ep;
	int c, i, addr_len;
	char *afname, *addrstr, *p;
	AddressFamily *af;
	struct sockaddr *addrp;
	Password *pwd;
	int pid;
	FILE *pid_fp;
	extern void checkpoint(void);

	openlog("aasd", LOG_PID|LOG_CONS|LOG_NOWAIT, LOG_DAEMON);

	endpoints = NULL;
	eplink = &endpoints;

	config_file = _PATH_CONFIG_FILE;

	debug = 0;

	/*
	 * Process options.
	 */
	
	opterr = 0;

	while ((c = getopt(argc, argv, "c:D:")) != -1) {
		switch (c) {
		case 'c':
			if (optarg[0] != '/') {
				report(LOG_ERR, "Configuration file must be an absolute path.");
				exit(1);
			}
			config_file = optarg;
			break;
		case 'D':
			debug = atoi(optarg);
			break;
		default:
			usage();
		}
	}

	/*
	 * The original version of this thing took a command line option
	 * to specify the endpoints to listen on.  To make things simpler,
	 * now we just listen at the default address for each supported
	 * family.
	 */

	for (i = 0; i < NFAMILIES; i++) {
		af = families[i];
		if (!(*af->str2addr)("", &addrp, &addr_len)) {
			report(LOG_ERR,
				"Unable to construct default %s address",
				af->name);
			continue;
		}
		if (!(ep = str_alloc(Endpoint))
		    || !(ep->addr =
		    (struct sockaddr *) malloc(addr_len))) {
			malloc_error("main");
			continue;
		}
		ep->af = af;
		memcpy(ep->addr, addrp, addr_len);
		ep->addr_len = addr_len;

		/*
		 * Put it on the list.
		 */
		
		ep->next = NULL;
		*eplink = ep;
		eplink = &ep->next;
	}

	/*
	 * Go into background, unless we're in debug level 3.
	 */
	
	if (debug < 3) {
		if ((pid = fork()) == -1) {
			report(LOG_ERR, "fork failed: %m");
			exit(1);
		}
		else if (pid != 0) {
			exit(0);
		}
		setsid();
	}
	else {
		closelog();
	}

	/*
	 * Ignore SIGCLD so the checkpoint processes don't become
	 * zombies.  Catch SIGTERM to write an exiting message.
	 */
	
	(void) sigset(SIGCLD, SIG_IGN);
	(void) sigset(SIGTERM, sigterm);
	
	report(LOG_INFO, "Address Allocation Server starting.");

	/*
	 * Record the process ID
	 */

	(void) unlink(_PATH_PID_FILE);
	if (pid_fp = fopen(_PATH_PID_FILE, "w")) {
		fprintf(pid_fp, "%d\n", getpid());
		fclose(pid_fp);
	}
	else {
		report(LOG_WARNING, "Unable to create pid file %s",
			_PATH_PID_FILE);
	}

	/*
	 * Read configuration.  If something fails, we go into a state
	 * where we don't honor any requests except reconfiguration.
	 */
	
	if (config()) {
		config_ok = 1;
	}
	else {
		report(LOG_ERR, "An error occurred during initial configuration; exiting.");
		exit(1);
	}

	/*
	 * Do it.  receive will complain if there are no valid endpoints
	 * (so we don't bother to check here).  If a checkpoint interval
	 * was specified, pass checkpoint function & interval.
	 */
	
	if (cp_intvl > 0) {
		/*
		 * Do a checkpoint now so we have a backup of the
		 * current state.
		 */
		checkpoint();
		receive(endpoints, checkpoint, cp_intvl);
	}
	else {
		receive(endpoints, NULL, 0);
	}
}
