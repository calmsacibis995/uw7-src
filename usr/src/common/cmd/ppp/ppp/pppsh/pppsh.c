#ident	"@(#)pppsh.c	1.4"

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <termios.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/uio.h>
#include <string.h>
#include <cs.h>

#include "ppp_type.h"
#include "ulr.h"

char *program_name;
int dbg = 0;

/*
 * This function provides an external interface to the PPP Daemon.
 * It allows an application to tell the daemon about incoming 
 * connections
 */
ppp_incoming(char *uid, int fd)
{
	int so;
	struct ulr_incoming msg;
	struct msghdr msgh;
	struct iovec iov;
	int ret;
	int cfd = fd;

	if (dbg)
		syslog(LOG_INFO, "ppp_incoming: uid %s fd %d\n", uid, fd);

	errno = 0;

	so = ppp_sockinit();
	if (so < 0) {
		syslog(LOG_ERR, "ppp_sockinit .. fails %d\n", errno);
		return -1;
	}

	msgh.msg_name = NULL;
	msgh.msg_namelen = 0;
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	iov.iov_base = (char *)&msg;
	iov.iov_len = sizeof(struct ulr_incoming);

	/* Send the info */

	msg.prim = ULR_INCOMING;

	if (uid && strcmp(uid, "root") != 0)
		strncpy(msg.uid, uid, MAXUIDLEN);
	else
		*msg.uid = 0;

	/* Get the ics blob */

	msg.ics[0] = 0;

	ics_getident(msg.ics);

	msgh.msg_accrights = (caddr_t)&cfd;
	msgh.msg_accrightslen = sizeof(cfd);

	ret = sendmsg(so, &msgh, 0);
	if (ret < 0) {
		syslog(LOG_ERR, "sendmsg, errno %d\n", errno);
		return -1;
	}

	msgh.msg_accrights = (caddr_t)NULL;
	msgh.msg_accrightslen = 0;

	ret = recvmsg(so, &msgh, 0);
	if (ret < 0) {
		syslog(LOG_ERR, "recv, errno %d\n", errno);
		return -1;
	}

	if (msg.error != 0) {
		if (dbg)
			syslog(LOG_INFO,
			       "PPP Daemon rejected call. Error %d\n",
			       msg.error);
		return -1;
	}

	/* Send the filedescriptor */

	msg.prim = ULR_END;
	ret = sendmsg(so, &msgh, 0);
	if (ret < 0) {
		syslog(LOG_ERR, "sendmsg2, errno %d\n", errno);
		return -1;
	}

	close(so);
	return 0;
}

/*
 * The ppp shell program - called to notify the PPP Daemon of an incoming
 * call
 *
 * When executed with the -a flag this indicates that auto ppp detection
 * was used, and no loginname is available.
 */
main(int argc, char *argv[])
{
	struct passwd *p_ent;
	char *loginname = NULL;
	int ret;
	int c;
	
	program_name = strrchr(argv[0],'/');
	program_name = program_name ? program_name + 1 : argv[0];

	openlog(program_name, LOG_PID|LOG_CONS|LOG_NDELAY, LOG_DAEMON);

        while ((c=getopt(argc,argv,"d")) != -1) {
		switch (c) {
		case 'd':
			dbg++;
			break;
		default:
			syslog(LOG_ERR, "usage: %s [-d]\n", program_name);
			exit(1);
		}
        }

	if (dbg)
		syslog(LOG_INFO, "pppsh entered."); 
	/*
	 * If the shell was executed as a result of a login
	 * get the login name
	 */
	p_ent = getpwuid(getuid());
	loginname = p_ent->pw_name;

	if (dbg && loginname == NULL)
		syslog(LOG_INFO, "can't get login name - assume autodetected");

	/* Tell pppd we have a call */

	if (ppp_incoming(loginname, 0) < 0) {
		syslog(LOG_WARNING, "ppp_incoming failed - exit(1)");
		exit(1);
	}

	if (dbg)
		syslog(LOG_INFO, "call handed off to pppd - exit(0)");

	exit(0);
}

