#ident	"@(#)nfsping.c	1.3"
#ident	"$Header$"

#include <stdlib.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <rpc/rpcb_prot.h>
#include <rpc/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <nfs/nfs.h>
#include <nfs/nfssys.h>
#include <locale.h>
#include <pfmt.h>

/*
 * exit codes, have the option which causes them at the end of comment
 */
#define	RET_OK		0	/* status is running, all */
#define	RET_NONE	32	/* no daemons are running, -a */
#define	RET_ONLYRPCBD	33	/* only rpcbind running, -a */
#define	RET_ONLYBOOT	34	/* runnning is bootserver mode, -a */
#define	RET_ONLYPCNFS	35	/* running in pc server mode, -a */
#define	RET_ONLYSERV	36	/* running in only server mode, -a */
#define	RET_ONLYCLNT	37	/* running in only client mode, -a */
#define	RET_BOOTCLNT	38	/* running in bootserver & client mode, -a */
#define	RET_CLNTPC	39	/* running in client and pc server mode, -a */
#define	RET_SERVCLNT	40	/* running in server and client mode, -a */
#define	RET_NORPCBD	41	/* rpcbind is not running, -a */
#define	RET_DEFAULT	42	/* default, all*/
#define	RET_NUMOPTS	43	/* more than one option specified, all */
#define	RET_NETCON	50	/* netconfig failed for udp, all */
#define	RET_NOTOK	51	/* status is not running, all */
#define	RET_BADNAME	52	/* bad daemon name, -o */

#define NULLPROC	((u_long)0)

#define MOUNTPROG	((u_long)100005)
#define MOUNTVERS	((u_long)1)
#define MOUNTPROC_NULL	NULLPROC
#define MOUNT_D		1

#define NFS_PROGRAM	((u_long)100003)
#define NFS_VERSION	((u_long)2)
#define NFS3_PROGRAM	((u_long)100003)
#define NFS3_VERSION	((u_long)3)
#define NFS_D		2

#define KLM_PROG	((u_long)100020)
#define KLM_VERS	((u_long)1)
#define LLOCK_D		3

#define SM_PROG		((u_long)100024)
#define SM_VERS		((u_long)1)
#define STAT_D		5

#define BOOTPARAMPROG	((u_long)100026)
#define BOOTPARAMVERS	((u_long)1)
#define BOOTP_D		6

#define PCNFSDPROG	((u_long)150001)
#define PCNFSDVERS	((u_long)1)
#define PCNFS_D		7

#define RPCB_D		8

static struct timeval timeout = { 5, 0 };
enum clnt_stat rpcb_rmtcall();
void ping_proc();

static	int	errors;
static	int	allflg;
static	int	servflg;
static	int	clntflg;
static	int	oneflg;

main(int argc, char **argv)
{
	int rpcbdrunning, nfsdrunning, mountdrunning, lockdrunning;
	int statdrunning, bootpdrunning, pcnfsdrunning, biodrunning;
	int bootparam, server, client, pcnfs, opt, numopts;
	char *procname = NULL;

	extern char *optarg;
	extern int optind;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:nfsping");

	/*
	 * initialize the world
	 */
	errors = nfsdrunning = mountdrunning = lockdrunning = 0;
	statdrunning = bootpdrunning = pcnfsdrunning = biodrunning = 0;
	bootparam = server = client = pcnfs = opt = numopts = 0;

	/*
	 * get the opts
	 */
	while ((opt = getopt(argc, argv, "asco:t:")) != EOF) {
		switch(opt) {
			case 'a':
				numopts++;
				allflg++;
				break;

			case 's':
				numopts++;
				servflg++;
				break;

			case 'c':
				numopts++;
				clntflg++;
				break;

			case 'o':
				numopts++;
				oneflg++;
				procname = optarg;
				break;
			case 't':
				/* For debugging only */
				timeout.tv_sec = atoi(optarg);
				printf("nfsping: timeout value is %d\n",
				       timeout.tv_sec);
				break;
			default:
				usage();
				exit(RET_DEFAULT);
		}
	}

	/*
	 * must have one and only one opt
	 */
	if (numopts != 1) {
		usage();
		if (numopts == 0)
			exit(RET_DEFAULT);
		else
			exit(RET_NUMOPTS);
	}

	if (oneflg) {
		/*
		 * need only one daemon status
		 */
		if (!strcmp(procname, "nfsd")) {
			ping_proc(NFS_D, &nfsdrunning);
			if (nfsdrunning)
				exit(RET_OK);
			else
				exit(RET_NOTOK);
			/* if (_nfssys(NFS_ISNFSDRUN, (caddr_t)NULL)) {
				pfmt(stdout, MM_NOSTD,
				     ":56:%s is not running\n", "nfsd");
				exit(RET_NOTOK);
			} else {
				pfmt(stdout, MM_NOSTD,
				     ":58:%s is running\n", "nfsd");
				exit(RET_OK);
			} */
		}
		if (!strcmp(procname, "biod")) {
			if (_nfssys(NFS_ISBIODRUN, (caddr_t)NULL)) {
				pfmt(stdout, MM_NOSTD,
				     ":56:%s is not running\n", "biod");
				exit(RET_NOTOK);
			} else {
				pfmt(stdout, MM_NOSTD,
				     ":58:%s is running\n", "biod");
				exit(RET_OK);
			}
		}
		if (!strcmp(procname, "rpcbind")) {
			ping_proc(RPCB_D, &rpcbdrunning);
			if (rpcbdrunning)
				exit(RET_OK);
			else
				exit(RET_NOTOK);
		}
		if (!strcmp(procname, "mountd")) {
			ping_proc(MOUNT_D, &mountdrunning);
			if (mountdrunning)
				exit(RET_OK);
			else
				exit(RET_NOTOK);
		}
		if (!strcmp(procname, "lockd")) {
			ping_proc(LLOCK_D, &lockdrunning);
			if (lockdrunning)
				exit(RET_OK);
			else
				exit(RET_NOTOK);
		}
		if (!strcmp(procname, "statd")) {
			ping_proc(STAT_D, &statdrunning);
			if (statdrunning)
				exit(RET_OK);
			else
				exit(RET_NOTOK);
		}
		if (!strcmp(procname, "bootparamd")) {
			ping_proc(BOOTP_D, &bootpdrunning);
			if (bootpdrunning)
				exit(RET_OK);
			else
				exit(RET_NOTOK);
		}
		if (!strcmp(procname, "pcnfsd")) {
			ping_proc(PCNFS_D, &pcnfsdrunning);
			if (pcnfsdrunning)
				exit(RET_OK);
			else
				exit(RET_NOTOK);
		}

		/*
		 * bad daemon name
		 */
		pfmt(stderr, MM_ERROR,
		     ":59:bad process-name with -o option\n");
		exit(RET_BADNAME);
	}

	if (servflg) {
		/*
		 * need status of server daemons
		 */
		ping_proc(RPCB_D, &rpcbdrunning);
		if (!rpcbdrunning)
			exit(RET_NOTOK);

		ping_proc(NFS_D, &nfsdrunning);
		if (!nfsdrunning)
			exit(RET_NOTOK);

		ping_proc(MOUNT_D, &mountdrunning);
		if (!mountdrunning)
			exit(RET_NOTOK);

		ping_proc(LLOCK_D, &lockdrunning);
		if (!lockdrunning)
			exit(RET_NOTOK);

		ping_proc(STAT_D, &statdrunning);
		if (!statdrunning)
			exit(RET_NOTOK);

		/*
		 * all server daemons are running
		 */
		exit(RET_OK);
	}

	if (clntflg) {
		/*
		 * need status of client daemons
		 */
		if (_nfssys(NFS_ISBIODRUN, (caddr_t)NULL)) {
			pfmt(stdout, MM_NOSTD, ":56:%s is not running\n", "biod");
			exit(RET_NOTOK);
		} else
			pfmt(stdout, MM_NOSTD, ":58:%s is running\n", "biod");

		ping_proc(RPCB_D, &rpcbdrunning);
		if (!rpcbdrunning)
			exit(RET_NOTOK);

		ping_proc(LLOCK_D, &lockdrunning);
		if (!lockdrunning)
			exit(RET_NOTOK);

		ping_proc(STAT_D, &statdrunning);
		if (!statdrunning)
			exit(RET_NOTOK);

		/*
		 * all client daemons are running
		 */
		exit(RET_OK);
	}

	/*
	 * could only come here if allflg.
	 * re-initialize everything
	 */
	errors = nfsdrunning = mountdrunning = lockdrunning = 0;
	statdrunning = bootpdrunning = pcnfsdrunning = biodrunning = 0;
	bootparam = server = client = pcnfs = opt = numopts = 0;

	/*
	 * ask os if any biodaemons are running
	 */
	if (_nfssys(NFS_ISBIODRUN, (caddr_t)NULL)) {
		pfmt(stdout, MM_NOSTD, ":56:%s is not running\n", "biod");
		errors++;
	} else {
		pfmt(stdout, MM_NOSTD, ":58:%s is running\n", "biod");
		biodrunning = 1;
	}

	/*
	 * ping the rest of the nfs world
	 */
	ping_proc(RPCB_D, &rpcbdrunning);
	ping_proc(NFS_D, &nfsdrunning);
	ping_proc(MOUNT_D, &mountdrunning);
	ping_proc(LLOCK_D, &lockdrunning);
	ping_proc(STAT_D, &statdrunning);
	ping_proc(BOOTP_D, &bootpdrunning);
	ping_proc(PCNFS_D, &pcnfsdrunning);

	if (errors == 0) 
		exit(RET_OK);
	if (!rpcbdrunning) {
		if (errors == 8)
			exit(RET_NONE);
		else
			exit(RET_NORPCBD);
	}
	if ((errors == 7 ) && rpcbdrunning)
		exit(RET_ONLYRPCBD);

	if (nfsdrunning && mountdrunning && lockdrunning && statdrunning)
		server = 1;
	if (nfsdrunning && mountdrunning && bootpdrunning)
		bootparam = 1;
	if (nfsdrunning && mountdrunning && pcnfsdrunning)
		pcnfs = 1;
	if (biodrunning && lockdrunning && statdrunning)
		client = 1;

	if (bootparam && !server && !client && !pcnfs)
		exit(RET_ONLYBOOT);
	if (!bootparam && !server && !client && pcnfs)
		exit(RET_ONLYPCNFS);
	if (!bootparam && server && !client && !pcnfs)
		exit(RET_ONLYSERV);
	if (!bootparam && !server && client && !pcnfs)
		exit(RET_ONLYCLNT);
	if (bootparam && !server && client && !pcnfs)
		exit(RET_BOOTCLNT);
	if (!bootparam && !server && client && pcnfs)
		exit(RET_CLNTPC);
	if (!bootparam && server && client && !pcnfs)
		exit(RET_SERVCLNT);

	exit(RET_DEFAULT);
}

/*
 * ping_proc(which, running)
 *	Ping an nfs daemon.
 *
 * Calling/exit status:
 *	Returns a void.
 *
 * Description:
 *	Pings an nfs daemon to check if it is still alive.
 *	Uses a timeout. Exits if fatal error.
 *
 * Parameters:
 *	which			: which daemon to ping
 *	running			: pointer to int to return stat in
 *
 */
void
ping_proc(int which, int *running)
{
	enum clnt_stat stat;
	static char res, argp;
	struct netconfig *nconf_udp;
	int fd;

	if ((nconf_udp = getnetconfigent("udp")) == NULL) {
		pfmt(stderr, MM_ERROR, ":60:%s failed for %s\n",
		     "getnetconfigent", "udp");
		exit(RET_NETCON);
	}

	switch(which) {

	/*
	 * rpcbind daemon
	 */
	case RPCB_D:
		if ((stat = rpcb_rmtcall(nconf_udp, "localhost", RPCBPROG,
				RPCBVERS, NULLPROC, xdr_void, &argp,
				xdr_void, &res, timeout,
				(struct netbuf *)NULL)) != RPC_SUCCESS) {
			pfmt(stdout, MM_NOSTD, ":57:%s is not running: %s\n",
			     "rpcbind", clnt_sperrno(stat));
			errors++;
			*running = 0;
		} else {
			pfmt(stdout, MM_NOSTD, ":58:%s is running\n", "rpcbind");
			*running = 1;
		}
		break;

	/*
	 * the mount daemon
	 */
	case MOUNT_D:
		if ((stat = rpcb_rmtcall(nconf_udp, "localhost", MOUNTPROG,
				MOUNTVERS, MOUNTPROC_NULL, xdr_void, &argp,
				xdr_void, &res, timeout,
				(struct netbuf *)NULL)) != RPC_SUCCESS) {
			pfmt(stdout, MM_NOSTD, ":57:%s is not running: %s\n", 
			     "mountd", clnt_sperrno(stat));
			errors++;
			*running = 0;
		} else {
			pfmt(stdout, MM_NOSTD, ":58:%s is running\n",
			     "mountd");
			*running = 1;
		}
		break;

	/*
	 * the nfs server daemon(s)
	 */
	case NFS_D:
		*running = 0;
		if ((stat = rpcb_rmtcall(nconf_udp, "localhost", NFS_PROGRAM,
				NFS_VERSION, RFS_NULL, xdr_void, &argp,
				xdr_void, &res, timeout,
				(struct netbuf *)NULL)) != RPC_SUCCESS) {
			pfmt(stdout, MM_NOSTD, ":57:%s is not running: %s\n",
			     "nfsd", clnt_sperrno(stat));
			errors++;
		} else {
			pfmt(stdout, MM_NOSTD, ":58:%s is running\n", "nfsd");
			*running = 1;
		}

		if ((stat = rpcb_rmtcall(nconf_udp, "localhost", NFS3_PROGRAM,
				NFS3_VERSION, RFS_NULL, xdr_void, &argp,
				xdr_void, &res, timeout,
				(struct netbuf *)NULL)) != RPC_SUCCESS) {
			pfmt(stdout, MM_NOSTD, ":57:%s V3 is not running: %s\n",
			     "nfsd", clnt_sperrno(stat));
			errors++;
		} else {
			pfmt(stdout, MM_NOSTD, ":58:%s V3 is running\n", "nfsd");
			*running = 1;
		}
		break;

	/*
	 * the lock manager daemon local proc
	 */
	case LLOCK_D:
		if ((stat = rpcb_rmtcall(nconf_udp, "localhost", KLM_PROG,
				KLM_VERS, NULLPROC, xdr_void, &argp,
				xdr_void, &res, timeout,
				(struct netbuf *)NULL)) != RPC_SUCCESS) {
			pfmt(stdout, MM_NOSTD, ":57:%s is not running: %s\n",
			     "lockd", clnt_sperrno(stat));
			errors++;
			*running = 0;
		} else {
			pfmt(stdout, MM_NOSTD, ":58:%s is running\n", "lockd");
			*running = 1;
		}
		break;

	/*
	 * the status daemon
	 */
	case STAT_D:
		/*
		 * first check if statd is "working" (calling statds
		 * on other machines). this is indicated by the presense
		 * of "/etc/smworking".
		 */
		fd = open("/etc/smworking", O_RDONLY);
		if (fd != -1) {
			pfmt(stdout, MM_NOSTD, ":58:%s is running\n", "statd");
			*running = 1;
		} else {
			if ((stat = rpcb_rmtcall(nconf_udp, "localhost", SM_PROG,
		   	SM_VERS, NULLPROC, xdr_void, &argp,
		   	xdr_void, &res, timeout,
		   	(struct netbuf *)NULL)) != RPC_SUCCESS) {
				pfmt(stdout, MM_NOSTD, 
				     ":57:%s is not running: %s\n",
				     "statd", clnt_sperrno(stat));
				errors++;
				*running = 0;
			} else {
				pfmt(stdout, MM_NOSTD, ":58:%s is running\n",
				     "statd");
				*running = 1;
			}
		}

		break;

	/*
	 * the bootparam daemon
	 */
	case BOOTP_D:
		if ((stat = rpcb_rmtcall(nconf_udp, "localhost", BOOTPARAMPROG,
		   BOOTPARAMVERS, NULLPROC, xdr_void, &argp,
		   xdr_void, &res, timeout,
		   (struct netbuf *)NULL)) != RPC_SUCCESS) {
			pfmt(stdout, MM_NOSTD, 
			     ":57:%s is not running: %s\n", 
			     "bootparamd", clnt_sperrno(stat));
			errors++;
			*running = 0;
		} else {
			pfmt(stdout, MM_NOSTD, ":58:%s is running\n",
			     "bootparamd");
			*running = 1;
		}
		break;

	/*
	 * the pcnfs daemon
	 */
	case PCNFS_D:
		if ((stat = rpcb_rmtcall(nconf_udp, "localhost", PCNFSDPROG,
		   PCNFSDVERS, NULLPROC, xdr_void, &argp,
		   xdr_void, &res, timeout,
		   (struct netbuf *)NULL)) != RPC_SUCCESS) {
			pfmt(stdout, MM_NOSTD, ":57:%s is not running: %s\n",
			     "pcnfsd", clnt_sperrno(stat));
			errors++;
			*running = 0;
		} else {
			pfmt(stdout, MM_NOSTD, ":58:%s is running\n",
			     "pcnfsd");
			*running = 1;
		}
		break;
	}
}

usage()
{
	pfmt(stderr, MM_ACTION, 
	     ":55:Usage: nfsping [ -a ] [ -s ] [ -c ] [ -o process-name ]\n");
}
