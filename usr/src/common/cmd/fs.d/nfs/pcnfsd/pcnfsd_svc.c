#ident	"@(#)pcnfsd_svc.c	1.2"
#ident	"$Header$"

/*
**=====================================================================
** Copyright (c) 1986-1993 by Sun Microsystems, Inc.
**=====================================================================
*/

/*
**=====================================================================
** Any and all changes made herein to the original code obtained from
** Sun Microsystems may not be supported by Sun Microsystems, Inc.
**=====================================================================
*/

/*
 * This file has been edited by hand.
 * It was generated using rpcgen.
 */
/* The following was added to conform with SVR4 */
#define PORTMAP
/* End inclusions */

#include "common.h"

#include <stdio.h>
#include <rpc/rpc.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <locale.h>
#include <pfmt.h>
#include <unistd.h>
#include <errno.h>

/* The following was added to conform with SVR4 */
#ifndef IPPROTO_IP
#include <netinet/in.h>
#endif
#include <sys/resource.h>
/* End inclusions */
#include <netdb.h>
#include "pcnfsd.h"

#ifdef DEBUG
#define RPC_SVC_FG
#endif
#define _RPCSVC_CLOSEDOWN 120
static _msgout();
#if 1
 void msg_out(msg) char *msg; {_msgout(msg);}
#endif
#if RPC_HDR
 extern void msg_out();
#endif

static void pcnfsdprog_1();
static void pcnfsdprog_2();
static void closedown();

static int _rpcpmstart;		/* Started by a port monitor ? */
static int _rpcfdtype;		/* Whether Stream or Datagram ? */
static int _rpcsvcdirty;	/* Still serving ? */

/*
** The following was added by hand:
*/
SVCXPRT *caller;
extern void config_from_file();
char            tempstr[256];

main(int argc, char *argv[])
{
	register SVCXPRT *transp;
	int sock;
	int proto;
	struct sockaddr_in saddr;
	size_t asize = sizeof(saddr);

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:pcnfsd");

	if (argc > 1) {
		pfmt(stderr, MM_ACTION, ":153:Usage: pcnfsd\n");
		exit(1);
	}

	if (getsockname(0, (struct sockaddr *)&saddr, &asize) == 0) {
		size_t ssize = sizeof(int);

		if (saddr.sin_family != AF_INET)
			exit(1);
		if (getsockopt(0, SOL_SOCKET, SO_TYPE,
				(char *)&_rpcfdtype, &ssize) == -1)
			exit(1);
		sock = 0;
		_rpcpmstart = 1;
		proto = 0;
		openlog("pcnfsd", LOG_PID, LOG_DAEMON);
	} else {
#ifndef RPC_SVC_FG
		int i, pid;

		pid = fork();
		if (pid < 0) {
			pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
			     "fork", strerror(errno));
			exit(1);
		}
		if (pid)
			exit(0);
		for (i = 0 ; i < 20; i++)
			(void) close(i);
#ifdef TIOCNOTTY
		i = open("/dev/console", 2);
		(void) dup2(i, 1);
		(void) dup2(i, 2);
		i = open("/dev/tty", 2);
		if (i >= 0) {
			(void) ioctl(i, TIOCNOTTY, (char *)NULL);
			(void) close(i);
		}
#else TIOCNOTTY
		(void)setsid();
#endif TIOCNOTTY
		openlog("pcnfsd", LOG_PID, LOG_DAEMON);
#endif RPC_SVC_FG
		sock = RPC_ANYSOCK;
		(void) pmap_unset(PCNFSDPROG, PCNFSDVERS);
		(void) pmap_unset(PCNFSDPROG, PCNFSDV2);
	}

	if ((_rpcfdtype == 0) || (_rpcfdtype == SOCK_DGRAM)) {
		transp = svcudp_create(sock);
		if (transp == NULL) {
			sprintf(tempstr,
				gettxt(":154", "cannot create %s service."),
				"udp");
			_msgout(tempstr);
			exit(1);
		}
		if (!_rpcpmstart)
			proto = IPPROTO_UDP;
		if (!svc_register(transp, PCNFSDPROG, PCNFSDVERS, pcnfsdprog_1, proto)) {
			sprintf(tempstr, 
				gettxt(":88", "cannot register (program %d, version %d) on %s"),
				PCNFSDPROG, PCNFSDVERS, "udp");
			_msgout(tempstr);
			exit(1);
		}
		if (!svc_register(transp, PCNFSDPROG, PCNFSDV2, pcnfsdprog_2, proto)) {
			sprintf(tempstr, 
				gettxt(":88", "cannot register (program %d, version %d) on %s"),
				PCNFSDPROG, PCNFSDV2, "udp");
			_msgout(tempstr);
			exit(1);
		}
	}

	if ((_rpcfdtype == 0) || (_rpcfdtype == SOCK_STREAM)) {
		transp = svctcp_create(sock, 0, 0);
		if (transp == NULL) {
			sprintf(tempstr, 
				gettxt(":154", "cannot create %s service."),
				"tcp");
			_msgout(tempstr);
			exit(1);
		}
		if (!_rpcpmstart)
			proto = IPPROTO_TCP;
		if (!svc_register(transp, PCNFSDPROG, PCNFSDVERS, pcnfsdprog_1, proto)) {
			sprintf(tempstr,
				gettxt(":88", "cannot register (program %d, version %d) on %s"),
				PCNFSDPROG, PCNFSDVERS, "tcp");
			_msgout(tempstr);
			exit(1);
		}
		if (!svc_register(transp, PCNFSDPROG, PCNFSDV2, pcnfsdprog_2, proto)) {
			sprintf(tempstr,
				gettxt(":88", "cannot register (program %d, version %d) on %s"),
				PCNFSDPROG, PCNFSDV2, "tcp");
			_msgout(tempstr);
			exit(1);
		}
	}

	if (transp == (SVCXPRT *)NULL) {
		_msgout(gettxt(":155", "could not create a handle"));
		exit(1);
	}
	if (_rpcpmstart) {
		(void) signal(SIGALRM, closedown);
		(void) alarm(_RPCSVC_CLOSEDOWN);
	}
/*
** The following was added by hand:
*/
	config_from_file();

#ifdef RPC_SVC_FG
	_msgout("rpc.pcnfsd version 2 ready to service requests");
#endif RPC_SVC_FG

	svc_run();
	_msgout(gettxt(":89", "svc_run returned"));
	exit(1);
	/* NOTREACHED */
}

static void
pcnfsdprog_1(rqstp, transp)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	union {
		auth_args pcnfsd_auth_1_arg;
		pr_init_args pcnfsd_pr_init_1_arg;
		pr_start_args pcnfsd_pr_start_1_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

/*
** The following was added by hand:
*/
	caller = transp;

	_rpcsvcdirty = 1;
	switch (rqstp->rq_proc) {
	case PCNFSD_NULL:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = (char *(*)()) pcnfsd_null_1;
		break;

	case PCNFSD_AUTH:
		xdr_argument = xdr_auth_args;
		xdr_result = xdr_auth_results;
		local = (char *(*)()) pcnfsd_auth_1;
		break;

	case PCNFSD_PR_INIT:
		xdr_argument = xdr_pr_init_args;
		xdr_result = xdr_pr_init_results;
		local = (char *(*)()) pcnfsd_pr_init_1;
		break;

	case PCNFSD_PR_START:
		xdr_argument = xdr_pr_start_args;
		xdr_result = xdr_pr_start_results;
		local = (char *(*)()) pcnfsd_pr_start_1;
		break;

	default:
		svcerr_noproc(transp);
		_rpcsvcdirty = 0;
		return;
	}
	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t)&argument)) {
		svcerr_decode(transp);
		_rpcsvcdirty = 0;
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t)&argument)) {
		sprintf(tempstr, gettxt(":123", "%s: cannot free arguments"),
			"pcnfsdprog_1");
		_msgout(tempstr);
		exit(1);
	}
	_rpcsvcdirty = 0;
	return;
}

static void
pcnfsdprog_2(rqstp, transp)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	union {
		v2_info_args pcnfsd2_info_2_arg;
		v2_pr_init_args pcnfsd2_pr_init_2_arg;
		v2_pr_start_args pcnfsd2_pr_start_2_arg;
		v2_pr_queue_args pcnfsd2_pr_queue_2_arg;
		v2_pr_status_args pcnfsd2_pr_status_2_arg;
		v2_pr_cancel_args pcnfsd2_pr_cancel_2_arg;
		v2_pr_admin_args pcnfsd2_pr_admin_2_arg;
		v2_pr_requeue_args pcnfsd2_pr_requeue_2_arg;
		v2_pr_hold_args pcnfsd2_pr_hold_2_arg;
		v2_pr_release_args pcnfsd2_pr_release_2_arg;
		v2_mapid_args pcnfsd2_mapid_2_arg;
		v2_auth_args pcnfsd2_auth_2_arg;
		v2_alert_args pcnfsd2_alert_2_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

/*
** The following was added by hand:
*/
	caller = transp;

	_rpcsvcdirty = 1;
	switch (rqstp->rq_proc) {
	case PCNFSD2_NULL:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = (char *(*)()) pcnfsd2_null_2;
		break;

	case PCNFSD2_INFO:
		xdr_argument = xdr_v2_info_args;
		xdr_result = xdr_v2_info_results;
		local = (char *(*)()) pcnfsd2_info_2;
		break;

	case PCNFSD2_PR_INIT:
		xdr_argument = xdr_v2_pr_init_args;
		xdr_result = xdr_v2_pr_init_results;
		local = (char *(*)()) pcnfsd2_pr_init_2;
		break;

	case PCNFSD2_PR_START:
		xdr_argument = xdr_v2_pr_start_args;
		xdr_result = xdr_v2_pr_start_results;
		local = (char *(*)()) pcnfsd2_pr_start_2;
		break;

	case PCNFSD2_PR_LIST:
		xdr_argument = xdr_void;
		xdr_result = xdr_v2_pr_list_results;
		local = (char *(*)()) pcnfsd2_pr_list_2;
		break;

	case PCNFSD2_PR_QUEUE:
		xdr_argument = xdr_v2_pr_queue_args;
		xdr_result = xdr_v2_pr_queue_results;
		local = (char *(*)()) pcnfsd2_pr_queue_2;
		break;

	case PCNFSD2_PR_STATUS:
		xdr_argument = xdr_v2_pr_status_args;
		xdr_result = xdr_v2_pr_status_results;
		local = (char *(*)()) pcnfsd2_pr_status_2;
		break;

	case PCNFSD2_PR_CANCEL:
		xdr_argument = xdr_v2_pr_cancel_args;
		xdr_result = xdr_v2_pr_cancel_results;
		local = (char *(*)()) pcnfsd2_pr_cancel_2;
		break;

	case PCNFSD2_PR_ADMIN:
		xdr_argument = xdr_v2_pr_admin_args;
		xdr_result = xdr_v2_pr_admin_results;
		local = (char *(*)()) pcnfsd2_pr_admin_2;
		break;

	case PCNFSD2_PR_REQUEUE:
		xdr_argument = xdr_v2_pr_requeue_args;
		xdr_result = xdr_v2_pr_requeue_results;
		local = (char *(*)()) pcnfsd2_pr_requeue_2;
		break;

	case PCNFSD2_PR_HOLD:
		xdr_argument = xdr_v2_pr_hold_args;
		xdr_result = xdr_v2_pr_hold_results;
		local = (char *(*)()) pcnfsd2_pr_hold_2;
		break;

	case PCNFSD2_PR_RELEASE:
		xdr_argument = xdr_v2_pr_release_args;
		xdr_result = xdr_v2_pr_release_results;
		local = (char *(*)()) pcnfsd2_pr_release_2;
		break;

	case PCNFSD2_MAPID:
		xdr_argument = xdr_v2_mapid_args;
		xdr_result = xdr_v2_mapid_results;
		local = (char *(*)()) pcnfsd2_mapid_2;
		break;

	case PCNFSD2_AUTH:
		xdr_argument = xdr_v2_auth_args;
		xdr_result = xdr_v2_auth_results;
		local = (char *(*)()) pcnfsd2_auth_2;
		break;

	case PCNFSD2_ALERT:
		xdr_argument = xdr_v2_alert_args;
		xdr_result = xdr_v2_alert_results;
		local = (char *(*)()) pcnfsd2_alert_2;
		break;

	default:
		svcerr_noproc(transp);
		_rpcsvcdirty = 0;
		return;
	}
	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t)&argument)) {
		svcerr_decode(transp);
		_rpcsvcdirty = 0;
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t)&argument)) {
		sprintf(tempstr, gettxt(":123", "%s: cannot free arguments"),
			"pcnfsdprog_2");
		_msgout(tempstr);
		exit(1);
	}
	_rpcsvcdirty = 0;
	return;
}

static
_msgout(msg)
	char *msg;
{
#ifdef RPC_SVC_FG
	if (_rpcpmstart)
		syslog(LOG_ERR, msg);
	else
		pfmt(stderr, MM_NOGET|MM_ERROR, "%s\n", msg);
#else
	syslog(LOG_ERR, msg);
#endif
	return(0);
}

static void
closedown()
{
	if (_rpcsvcdirty == 0) {
		extern fd_set svc_fdset;
		static int size;
		int i, openfd;

		if (_rpcfdtype == SOCK_DGRAM)
			exit(0);
#ifdef RLIMIT_NOFILE
 		if (size == 0) {
 			struct rlimit rl;
 
 			rl.rlim_max = 0;
 			getrlimit(RLIMIT_NOFILE, &rl);
 			if ((size = rl.rlim_max) == 0)
 				return;
 		}
#else RLIMIT_NOFILE
/* XXX could use getdtablesize() here, but I don't know who uses it */
		size = 20;
#endif RLIMIT_NOFILE
		for (i = 0, openfd = 0; i < size && openfd < 2; i++)
			if (FD_ISSET(i, &svc_fdset))
				openfd++;
		if (openfd <= 1)
			exit(0);
	}
	(void) alarm(_RPCSVC_CLOSEDOWN);
}

/*
** The following was added by hand.
*/

char *
getcallername()
{
        struct sockaddr_in actual;
        struct hostent *hp;
        static struct in_addr prev;
	static char cname[128];

        actual = *svc_getcaller(caller);

        if (memcmp((char *)&actual.sin_addr, (char *)&prev,
		 sizeof(struct in_addr)) == 0)
                return (cname);

        prev = actual.sin_addr;

        hp = gethostbyaddr((char *) &actual.sin_addr, sizeof(actual.sin_addr), 
                           AF_INET);
        if (hp == NULL) {                       /* dummy one up */
		extern char *inet_ntoa();
                strcpy(cname, inet_ntoa(actual.sin_addr));
         } else {
                strcpy(cname, hp->h_name);
        }

        return (cname);
}
