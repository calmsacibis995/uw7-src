#ident	"@(#)start.c	1.6"

#include <stdio.h>
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
#include <synch.h>
#include <thread.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/conf.h>

#include "pathnames.h"
#include "fsm.h"
#include "ppp_type.h"
#include "psm.h"


void *ulr_thread();
void *cd_thread();
int timeout(int, int (*)(), caddr_t, caddr_t);
void timer_init();
void *timer_thread(void *argp);
extern thread_t timer_tid;

/* Globals */
int next_bundle_index = 0;
sigset_t sig_mask;
int pcid_fd;	/* Stream to the pcid driver */
char host[MAXHOSTNAMELEN + 1];

int ucfg_garbage();

init()
{
	if (gethostname(host, MAXHOSTNAMELEN) < 0) {
		ppplog(MSG_WARN, 0, "Failed to obtain hostname.\n");
		host[0] = 0;
	}

	/* Don't create anything users can access ! */
	umask(0077);
}
/*
 * Start PPP
 */
pppstart()
{
	extern int pppd_debug, user_debug;

	/* Start proper */

	init();

	timer_init();

	ucfg_init();

	act_init();

	hist_init();

	/* Set signals as required */

	sigemptyset(&sig_mask);
	sigaddset(&sig_mask, SIGALRM);
	sigaddset(&sig_mask, SIGPIPE);

	sigaddset(&sig_mask, SIGHUP);
	sigaddset(&sig_mask, SIGTERM);
	sigaddset(&sig_mask, SIGSTOP);
	sigaddset(&sig_mask, SIGINT);

	thr_sigsetmask(SIG_BLOCK, &sig_mask, NULL);

	sigdelset(&sig_mask, SIGPIPE);

	/* Open pcid driver ... communication path to ppp */

	pcid_fd = open(DEV_PCID, O_RDWR);
	if (pcid_fd < 0) {
		printf("Error opening %s\n", DEV_PCID);
		perror("open");
		exit(1);
	}

	ppplog(MSG_INFO, 0, "PPP Started (Version %s).\n",
	       PPP_VERSION_STRING);

	if (user_debug) /* pppd -d */
		ppplog(MSG_INFO, 0, "Debug mode selected.\n");

	if (pppd_debug & DEBUG_ANVL) /* ANVL defined */
		ppplog(MSG_INFO, 0, "Anvl test mode selected.\n");

	if (pppd_debug & DEBUG_PPPD) /* PPPD_DEBUG defined */
		ppplog(MSG_INFO, 0, "Implementation debugging selected\n");

	/* Start the Timer thread */

	thr_create(NULL, 0, timer_thread, (void *)&sig_mask,
		   THR_BOUND | THR_DETACHED, &timer_tid);

	/* PPP Driver control thread */
	thr_create(NULL, 0, cd_thread, NULL, 
		   THR_DETACHED | THR_BOUND, NULL);

	/* User-level Request thread */
	thr_create(NULL, 0, ulr_thread, NULL,
		   THR_DETACHED | THR_BOUND, NULL);

	/* Config garbage collector */
	timeout(10 * HZ, ucfg_garbage, (caddr_t)0, (caddr_t)0);

	thr_exit(0);

	ppplog(MSG_INFO, 0, "PPP Exited\n");
}

pppstop()
{
	/* Tell various interested parties that we are stopping */
	act_unload();
	psm_unload();
}
