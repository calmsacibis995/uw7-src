#ident	"@(#)mount.c	1.4"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#define	NFSCLIENT

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mount.h>	/* exit code definitions */
#include <sys/mnttab.h>
#include <sys/mntent.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <nfs/nfs.h>
#include <nfs/nfsv3.h>
#include <nfs/nfs_clnt.h>
#include <nfs/mount.h>
#include <nfs/nfssys.h>
#include <rpcsvc/mount.h>
#include <tiuser.h>
#include <netconfig.h>
#include <netdir.h>
#include <locale.h>
#include <pfmt.h>

#define MNTTYPE_NFS	"nfs"
#define BIGRETRY	10000
#define RET_NFSDOWN	79

/*
 * maximum length of RPC header for NFS messages
 */
#define NFS_RPC_HDR	432

#ifdef __STDC__

static	void	usage(void);
static	int	mount_nfs(struct mnttab *, int, int);
static	int	set_args(int *, struct nfs_args *, char *, struct mnttab *);
static	int	make_secure(struct nfs_args *, char *, struct netconfig **);
static	struct netbuf *get_addr(char *, int, int, int, struct netconfig **);
static	int	get_fh(struct nfs_args *, char *, char *);
static	int	getaddr_nfs(struct nfs_args *, char *, struct netconfig **);
static	void	addtomnttab(struct mnttab *);
static	int	retry(struct mnttab *, int);
static	AUTH	*my_authsys_create_default();

#else /* __STDC__ */

static	void	usage();
static	int	mount_nfs();
static	int	set_args();
static	int	make_secure();
static	struct netbuf *get_addr();
static	int	get_fh();
static	int	getaddr_nfs();
static	void	addtomnttab();
static	int	retry();
static	AUTH	*my_authsys_create_default();

#endif /* __STDC__ */

extern int _nderror;

struct	t_info	tinfo;
char		*strdup();
int		bg;
int		retries = BIGRETRY;
u_short		nfs_port;
int		server_pre4dot0;

main(argc, argv)
	int argc;
	char **argv;
{
	struct mnttab mnt, *mntp;
	extern char *optarg;
	extern int optind;
	char optbuf[256];
	int ro = 0, Vflg = 0;
	int r, c, ret;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:nfs mount");

	mnt.mnt_mntopts = optbuf;
	(void) strcpy(optbuf, "rw");

	/*
	 * Set options
	 */
	while ((c = getopt(argc, argv, "ro:V")) != EOF) {
		switch (c) {
		case 'r':
			ro++;
			break;
		case 'o':
			strcpy(mnt.mnt_mntopts, optarg);
			break;
		case 'V':
			Vflg++;
			break;
		default:
			usage();
		}
	}
	if (argc - optind != 2)
		usage();

	/*
	 * XXX- check if at least rpcbind and biod are running
	 */
	if ((ret = check_nfs())) {
		if (ret == 2) {
			pfmt(stderr, MM_ERROR,
			     ":2:%s is not running, cannot mount %s\n",
			     "nfsd", argv[optind+1]);
			exit(RET_NFSDOWN);
		} else if (ret == 3) {
			pfmt(stderr, MM_ERROR,
			     ":2:%s is not running, cannot mount %s\n",
			     "rpcbind", argv[optind+1]);
			exit(RET_NFSDOWN);
		}
		/*
		 * getnetconfig failed --- ignore
		 */
	}

	mnt.mnt_special = argv[optind];
	mnt.mnt_mountp  = argv[optind+1];
	mntp = &mnt;

	r = mount_nfs(mntp, ro, Vflg);

	if (r == RET_RETRY && retries)
		r = retry(mntp, ro);

	exit (r);
}

static void
usage()
{
	pfmt(stderr, MM_ACTION,
	     ":1:Usage: mount [-r] [-o options] server:path directory\n");
	exit (RET_USAGE);
}

static int
mount_nfs(mntp, ro, Vflg)
	struct mnttab *mntp;
	int ro, Vflg;
{
	struct nfs_args nfs_args;
	char *p, *fsname, *fshost, *fspath;
	struct netconfig *nconf = NULL;
	int mntflags = 0;
	int r;
	int mount_errno;	/* capture errno for mount() */

	memset(&nfs_args, 0, sizeof (struct nfs_args));

	mntp->mnt_fstype = MNTTYPE_NFS;

	/*
	 * split server:/dir into two strings: fshost & fspath
	 */
	fsname = (char *) strdup(mntp->mnt_special);
	if (fsname == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n", 
		     "mount_nfs", "fsname");
		return (RET_MALLOC);
	}
	p = (char *) strchr(fsname, ':');
	if (p == NULL) {
		pfmt(stderr, MM_ERROR, 
		     ":4:not nfs file system, use 'server:path' format\n");
		return (RET_HP);
	}
	*p++ = '\0';
	fshost = fsname;
	fspath = p;

	if (r = set_args(&mntflags, &nfs_args, fshost, mntp))
		return (r);
	else if (Vflg)
		exit(RET_OK);

	if (nfs_args.flags & NFSMNT_PRE4dot0) {
		server_pre4dot0 = 1;
		pfmt(stderr, MM_WARNING,
		     ":5:server is exporting pre-4.0 nfs, limit groups to 8\n");
	}

	if (ro) {
		mntflags |= MS_RDONLY;
		if (p = strstr(mntp->mnt_mntopts, "rw"))	/* "rw"->"ro" */
			if (*(p+2) == ',' || *(p+2) == '\0')
				*(p+1) = 'o';
	}

	if (r = get_fh(&nfs_args, fshost, fspath))
		return (r);
	/* now we have come this far, if the mount fails after this we must do
	 * a remote umount before exiting RM
	 */
	if (getaddr_nfs(&nfs_args, fshost, &nconf) < 0) {
		/* RM must tell other side that it should umount the resource */
		rem_umount(&nfs_args, fshost, fspath);
		/* RM */
		return (RET_ADDR);
	}

	if (nfs_args.flags & NFSMNT_SECURE) {
		if (make_secure(&nfs_args, fshost, &nconf) < 0) {
			/* RM must tell other side that it should umount the resource */
			rem_umount(&nfs_args, fshost, fspath);
			/* RM */
			return (RET_SEC);
		}
	}

	if (mount("", mntp->mnt_mountp, mntflags | MS_DATA, MNTTYPE_NFS,
		&nfs_args, sizeof (nfs_args)) < 0) {
		mount_errno = errno;
		pfmt(stderr, MM_ERROR, ":6:error mounting %s: %s\n",
		     mntp->mnt_mountp, strerror(errno));
		/* RM must tell other side that it should umount the resource */
		rem_umount(&nfs_args, fshost, fspath);
		/* RM */
		if (mount_errno == EBUSY)
			return (RET_EBUSY);
		else
			return (RET_MISC);
	}
	addtomnttab(mntp);
	free(fsname);

	return (RET_OK);
}

#define MNTOPT_INTR	"intr"
#define MNTOPT_PORT	"port"
#define MNTOPT_SECURE	"secure"
#define MNTOPT_RSIZE	"rsize"
#define MNTOPT_WSIZE	"wsize"
#define MNTOPT_TIMEO	"timeo"
#define MNTOPT_RETRANS	"retrans"
#define MNTOPT_ACTIMEO	"actimeo"
#define MNTOPT_ACREGMIN	"acregmin"
#define MNTOPT_ACREGMAX	"acregmax"
#define MNTOPT_ACDIRMIN	"acdirmin"
#define MNTOPT_ACDIRMAX	"acdirmax"
#define MNTOPT_NOAC	"noac"
#define MNTOPT_BG	"bg"
#define MNTOPT_FG	"fg"
#define MNTOPT_RETRY	"retry"
#define MNTOPT_SUID	"suid"
#define MNTOPT_pre4dot0	"pre4.0"
#define MNTOPT_MAXLWPS	"maxthreads"
#define MNTOPT_V3	"v3"

char *optlist[] = {
#define OPT_RO		0
	MNTOPT_RO,
#define OPT_RW		1
	MNTOPT_RW,
#define OPT_QUOTA	2
	MNTOPT_QUOTA,
#define OPT_NOQUOTA	3
	MNTOPT_NOQUOTA,
#define OPT_SOFT	4
	MNTOPT_SOFT,
#define OPT_HARD	5
	MNTOPT_HARD,
#define OPT_NOSUID	6
	MNTOPT_NOSUID,
#define OPT_NOAUTO	7
	MNTOPT_NOAUTO,
#define OPT_GRPID	8
	MNTOPT_GRPID,
#define OPT_REMOUNT	9
	MNTOPT_REMOUNT,
#define OPT_NOSUB	10
	MNTOPT_NOSUB,
#define OPT_INTR	11
	MNTOPT_INTR,
#define OPT_PORT	12
	MNTOPT_PORT,
#define OPT_SECURE	13
	MNTOPT_SECURE,
#define OPT_RSIZE	14
	MNTOPT_RSIZE,
#define OPT_WSIZE	15
	MNTOPT_WSIZE,
#define OPT_TIMEO	16
	MNTOPT_TIMEO,
#define OPT_RETRANS	17
	MNTOPT_RETRANS,
#define OPT_ACTIMEO	18
	MNTOPT_ACTIMEO,
#define OPT_ACREGMIN	19
	MNTOPT_ACREGMIN,
#define OPT_ACREGMAX	20
	MNTOPT_ACREGMAX,
#define OPT_ACDIRMIN	21
	MNTOPT_ACDIRMIN,
#define OPT_ACDIRMAX	22
	MNTOPT_ACDIRMAX,
#define OPT_BG		23
	MNTOPT_BG,
#define OPT_FG		24
	MNTOPT_FG,
#define OPT_RETRY	25
	MNTOPT_RETRY,
#define OPT_NOAC	26
	MNTOPT_NOAC,
#define OPT_SUID	27
	MNTOPT_SUID,
#define OPT_PRE4dot0	28
	MNTOPT_pre4dot0,
#define OPT_MAXLWPS	29
	MNTOPT_MAXLWPS,
#define OPT_V3		30
	MNTOPT_V3,
	NULL
};

#define bad(val) (val == NULL || !isdigit(*val))

static int
set_args (mntflags, args, fshost, mnt)
	int *mntflags;
	struct nfs_args *args;
	char *fshost;
	struct mnttab *mnt;
{
	char *saveopt, *optstr, *opts, *val;

	args->flags = 0;
	args->flags |= NFSMNT_HOSTNAME;
	args->hostname = fshost;
	optstr = opts = strdup(mnt->mnt_mntopts);
	if (opts == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n", 
		     "set_args", "opts");
		return (RET_MALLOC);
	}
	while (*opts) {
		saveopt = opts;
		switch (getsubopt(&opts, optlist, &val)) {
		case OPT_RO:
			*mntflags |= MS_RDONLY;
			break;
		case OPT_RW:
			*mntflags &= ~(MS_RDONLY);
			break;
		case OPT_QUOTA:
		case OPT_NOQUOTA:
			break;
		case OPT_SOFT:
			args->flags |= NFSMNT_SOFT;
			break;
		case OPT_HARD:
			args->flags &= ~(NFSMNT_SOFT);
			break;
		case OPT_NOSUID:
			*mntflags |= MS_NOSUID;
			break;
		case OPT_SUID:
			*mntflags &= ~(MS_NOSUID);
			break;
		case OPT_NOAUTO:
			break;
		case OPT_GRPID:
			args->flags |= NFSMNT_GRPID;
			break;
		case OPT_REMOUNT:
			break;
		case OPT_INTR:
			args->flags |= NFSMNT_INT;
			break;
		case OPT_NOAC:
			args->flags |= NFSMNT_NOAC;
			break;
		case OPT_PRE4dot0:
			args->flags |= NFSMNT_PRE4dot0;
			break;
		case OPT_V3:
			args->flags |= NFSMNT_V3;
			break;
		case OPT_MAXLWPS:
			args->flags |= NFSMNT_LWPSMAX;
			if (bad(val))
				goto badopt;
			args->lwpsmax = atoi(val);
			break;
			break;
		case OPT_PORT:
			if (bad(val))
				goto badopt;
			nfs_port = htons(atoi(val));
			break;

		case OPT_SECURE:
			args->flags |= NFSMNT_SECURE;
			break;

		case OPT_RSIZE:
			args->flags |= NFSMNT_RSIZE;
			if (bad(val))
				goto badopt;
			args->rsize = atoi(val);
			break;
		case OPT_WSIZE:
			args->flags |= NFSMNT_WSIZE;
			if (bad(val))
				goto badopt;
			args->wsize = atoi(val);
			break;
		case OPT_TIMEO:
			args->flags |= NFSMNT_TIMEO;
			if (bad(val))
				goto badopt;
			args->timeo = atoi(val);
			break;
		case OPT_RETRANS:
			args->flags |= NFSMNT_RETRANS;
			if (bad(val))
				goto badopt;
			args->retrans = atoi(val);
			break;
		case OPT_ACTIMEO:
			args->flags |= NFSMNT_ACDIRMIN;
			args->flags |= NFSMNT_ACDIRMAX;
			args->flags |= NFSMNT_ACREGMIN;
			args->flags |= NFSMNT_ACREGMAX;
			if (bad(val))
				goto badopt;
			args->acregmin = atoi(val);
			if (args->acregmin <= 0 || args->acregmin > ACMAXMAX)
				goto badopt;
			args->acdirmin = args->acdirmax = args->acregmax =
			  args->acregmin;
			break;
		case OPT_ACREGMIN:
			args->flags |= NFSMNT_ACREGMIN;
			if (bad(val))
				goto badopt;
			args->acregmin = atoi(val);
			break;
		case OPT_ACREGMAX:
			args->flags |= NFSMNT_ACREGMAX;
			if (bad(val))
				goto badopt;
			args->acregmax = atoi(val);
			break;
		case OPT_ACDIRMIN:
			args->flags |= NFSMNT_ACDIRMIN;
			if (bad(val))
				goto badopt;
			args->acdirmin = atoi(val);
			break;
		case OPT_ACDIRMAX:
			args->flags |= NFSMNT_ACDIRMAX;
			if (bad(val))
				goto badopt;
			args->acdirmax = atoi(val);
			break;
		case OPT_BG:
			bg++;
			break;
		case OPT_FG:
			bg = 0;
			break;
		case OPT_RETRY:
			if (bad(val))
				goto badopt;
			retries = atoi(val);
			break;
		default:
			goto badopt;
		}
	}
	free(optstr);
	return (RET_OK);

badopt:
	pfmt(stderr, MM_ERROR, ":7:invalid option: %s\n", saveopt);
	free(optstr);
	return (RET_INVOP);
}

#include <netinet/in.h>

static int
make_secure(args, hostname, nconfp)
	struct nfs_args *args;
	char *hostname;
	struct netconfig **nconfp;
{
	static char netname[MAXNETNAMELEN+1];

	args->flags |= NFSMNT_SECURE;

	/*
	 * XXX: need to support other netnames
	 * outside domain and not always just use
	 * the default conversion.
	 */
	if (!host2netname(netname, hostname, NULL)) {
		pfmt(stderr, MM_ERROR, ":8:%s failed: %s is not known\n",
		     "host2netname", hostname);
		return(-1);
	}
	args->netname = netname;

	/*
	 * Get the network address for the time service on
	 * the server.  If an RPC based time service is
	 * not available then try the old UDP time service
	 * if it's a UDP transport.
	 */
	args->syncaddr = get_addr(hostname, RPCBPROG, RPCBVERS, 0, nconfp);
	if (args->syncaddr != NULL) {
		args->flags |= NFSMNT_RPCTIMESYNC;
	} else {
		struct nd_hostserv hs;
		struct nd_addrlist *retaddrs;

		hs.h_host = hostname;
		hs.h_serv = "rpcbind";
		if (netdir_getbyname(*nconfp, &hs, &retaddrs) != ND_OK) {
			goto err;
		}
		args->syncaddr = retaddrs->n_addrs;
		((struct sockaddr_in *) args->syncaddr->buf)->sin_port
			= IPPORT_TIMESERVER;
	}
	return (0);

err:
	pfmt(stderr, MM_ERROR, ":9:%s: no time service for %s\n",
	     "make-secure", hostname);
	return (-1);
}

/*
 * Get the network address on "hostname" for program "prog"
 * with version "vers".  If the port number is specified (non zero)
 * then try for a UDP transport and set the port number of the
 * resulting IP address.
 * Finally, ping the null procedure of that service.
 *
 * If the address of a netconfig pointer was passed then
 * if it's not null use it as the netconfig otherwise
 * assign the address of the netconfig that was used to
 * establish contact with the service.
 */
static struct netbuf *
get_addr(hostname, prog, vers, port, nconfp)
	char *hostname;
	int prog, vers, port;
	struct netconfig **nconfp;
{
	struct netbuf *nb = NULL;
	struct t_bind *tbind = NULL;
	struct netconfig *nconf;
	static NCONF_HANDLE *nc = NULL;
	enum clnt_stat cs;
	CLIENT *cl = NULL;
	struct timeval tv;
	int fd = -1;

	if (nconfp && *nconfp) {
		nconf = *nconfp;
	} else {
		nc = setnetconfig();
		if (nc == NULL)
			goto done;

retry:
		/*
		 * If the port number is specified then UDP is needed.
		 * Otherwise any connectionless transport will do.
		 */
		while (nconf = getnetconfig(nc)) {
			if ((nconf->nc_flag & NC_VISIBLE) &&
			     nconf->nc_semantics == NC_TPI_CLTS) {
				if (port == 0)
					break;
				if ((strcmp(nconf->nc_protofmly, NC_INET) == 0) &&
				    (strcmp(nconf->nc_proto, NC_UDP) == 0))
					break;
			} 
		}
		if (nconf == NULL) {
			/* If we got this far, the host or svc doesn't exist */
			pfmt(stderr, MM_ERROR,
			     ":10:%s: (host %s, program %d, version %d) is not found on any transport\n",
			     "get_addr", hostname, prog, vers);
			exit (RET_ADDR);
		}
	}

	if ((fd = t_open(nconf->nc_device, O_RDWR, &tinfo)) == -1) {
		if (nc)
			goto retry;
		else
			goto done;
	}

	tbind = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
	if (tbind == NULL) 
		if (nc) {
			(void) t_close(fd);
			fd = -1;
			goto retry;
		} else
			goto done;

	if (rpcb_getaddr(prog, vers, nconf, &tbind->addr, hostname) == 0) {
		if (nc) {
			(void) t_free((caddr_t)tbind, T_BIND);
			tbind = NULL;
			(void) t_close(fd);
			fd = -1;
			goto retry;
		} else
			goto done;
	}

	if (port)
		((struct sockaddr_in *) tbind->addr.buf)->sin_port = port;

	cl = clnt_tli_create(fd, nconf, &tbind->addr, prog, vers, 0, 0);
	if (cl == NULL) {
		if (nc) {
			(void) t_free((caddr_t)tbind, T_BIND);
			tbind = NULL;
			(void) t_close(fd);
			fd = -1;
			goto retry;
		} else
			goto done;
	}
	if (client_bindresvport(cl) < 0) {
		pfmt(stderr, MM_ERROR, ":11:could not bind to reserved port\n");
		clnt_destroy(cl);
		cl = NULL;
		(void) t_free((caddr_t)tbind, T_BIND);
		tbind = NULL;
		(void) t_close(fd);
		fd = -1;
		goto retry;
	}
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	cl->cl_auth = my_authsys_create_default();
	cs = clnt_call(cl, NULLPROC, xdr_void, 0, xdr_void, 0, tv);
	if (cs != RPC_SUCCESS) {
		if (nc) {
			if (cl->cl_auth)
				AUTH_DESTROY(cl->cl_auth);
			clnt_destroy(cl);
			cl = NULL;
			(void) t_free((caddr_t)tbind, T_BIND);
			tbind = NULL;
			(void) t_close(fd);
			fd = -1;
			goto retry;
		} else
			goto done;
	}

	/*
	 * Make a copy of the netbuf to return
	 */
	nb = (struct netbuf *)malloc(sizeof(struct netbuf));
	if (nb == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n",
		     "get_addr", "nb");
		goto done;
	}
	*nb = tbind->addr;
	nb->buf = (char *)malloc(nb->maxlen);
	if (nb->buf == NULL) {
		pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n", 
		     "get_addr", "buf");
		free(nb);
		nb = NULL;
		goto done;
	}
	(void) memcpy(nb->buf, tbind->addr.buf, nb->len);
	if (nconfp && *nconfp == NULL) {
		*nconfp = getnetconfigent(nconf->nc_netid);
		if (*nconfp == NULL) {
			pfmt(stderr, MM_ERROR, ":3:%s: no memory for %s\n", 
			     "get_addr", "nconfp");
			free(nb);
			nb = NULL;
			goto done;
		}
	}

done:
	if (cl) {
		if (cl->cl_auth)
			AUTH_DESTROY(cl->cl_auth);
		clnt_destroy(cl);
	}
	if (tbind)
		t_free((char *) tbind, T_BIND);
	if (fd >= 0)
		(void) t_close(fd);
	if (nc)
		endnetconfig(nc);
	return (nb);
}

/*
 * get fhandle of remote path from server's mountd
 */
static int
get_fh(args, fshost, fspath)
	struct nfs_args *args;
	char *fshost, *fspath;
{
	static struct fhstatus fhs;
	CLIENT *cl;
	enum clnt_stat rpc_stat;
	struct timeval timeout = { 25, 0};

	cl = clnt_create(fshost, MOUNTPROG, MOUNTVERS, "datagram_v");
	if (cl == NULL) {
		pfmt(stderr, MM_ERROR, 
		     ":12:%s:%s: server not responding for %s\n",
		     fshost, fspath, 
		     clnt_spcreateerror("clnt_create"));

		/* certain errors are fatal. Give up on them */
		if (_nderror == ND_NOHOST || _nderror == ND_NOSERV)
			exit (RET_SERV);
		return (RET_RETRY);
	}
	if (client_bindresvport(cl) < 0) {
		pfmt(stderr, MM_ERROR, ":11:could not bind to reserved port\n");
		clnt_destroy(cl);
		return (RET_RETRY);
	}

	cl->cl_auth = my_authsys_create_default();
	rpc_stat = clnt_call(cl, MOUNTPROC_MNT, xdr_path, (caddr_t)&fspath,
			     xdr_fhstatus, (caddr_t)&fhs, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		pfmt(stderr, MM_ERROR, 
		     ":12:%s:%s server not responding for %s\n",
		     fshost, fspath, 
		     clnt_sperror(cl, "clnt_call"));
		if (cl->cl_auth)
			AUTH_DESTROY(cl->cl_auth);
		clnt_destroy(cl);
		return (RET_RETRY);
	}
	if (cl->cl_auth)
		AUTH_DESTROY(cl->cl_auth);
	clnt_destroy(cl);

	if ((errno = fhs.fhs_status) != 0) {
		switch (errno) {
		case EACCES:
			pfmt(stderr, MM_ERROR, ":13:access denied for %s:%s\n",
			     fshost, fspath);
			return (RET_ACCESS);
		case ENOENT:
			pfmt(stderr, MM_ERROR, ":14:%s is not shared in %s\n",
			     fspath, fshost);
			return (RET_NOENT);
		default:
			pfmt(stderr, MM_NOGET|MM_ERROR, 
			     "%s:%s: %s\n", fshost, fspath, strerror(errno));
			return (RET_ACCESS);
		}
	}
	args->fh = (caddr_t) &fhs.fhs_fh;
	return (RET_OK);
}


/* RM 17.7.91
 * do an unmount on the remote machine
*/

rem_umount(args, fshost, fspath)
	struct nfs_args *args;
	char *fshost, *fspath;
{
        struct timeval timeout = { 25, 0};
	CLIENT *cl;
	enum clnt_stat rpc_stat;
	int status = RET_OK;

	cl = clnt_create(fshost, MOUNTPROG, MOUNTVERS, "datagram_v");
	if (cl == NULL) {
		pfmt(stderr, MM_ERROR, 
		     ":12:%s:%s: server not responding for %s\n",
		     fshost, fspath, clnt_spcreateerror("clnt_create"));
		return (RET_RETRY);
	}
	if (client_bindresvport(cl) < 0) {
		pfmt(stderr, MM_ERROR, ":11:could not bind to reserved port\n");
		clnt_destroy(cl);
		return (RET_RETRY);
	}

	cl->cl_auth = my_authsys_create_default();
	rpc_stat = clnt_call(cl, MOUNTPROC_UMNT, xdr_path, (caddr_t)&fspath,
			     xdr_void, (caddr_t)NULL, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		pfmt(stderr, MM_ERROR, 
		     ":12:%s:%s server not responding for %s\n",
		     fshost, fspath, clnt_sperror(cl, "clnt_call"));
		status = RET_RETRY;
	}
	if (cl->cl_auth)
		AUTH_DESTROY(cl->cl_auth);
	clnt_destroy(cl);

	return (status);
}
/* RM  17.7.91 */

/*
 * Fill in the address for the server's NFS service and
 * fill in a knetconfig structure for the transport that
 * the service is available on.
 */
static int
getaddr_nfs(args, fshost, nconfp)
	struct nfs_args *args;
	char *fshost;
	struct netconfig **nconfp;
{
	struct stat sb;
	struct netconfig *nconf;
	static struct knetconfig knconf;

	if ((args->flags & NFSMNT_V3) == 0)
		args->addr = get_addr(fshost, NFS_PROGRAM, NFS_VERSION,
			      nfs_port, nconfp);
	else
		args->addr = get_addr(fshost, NFS3_PROGRAM, NFS3_VERSION,
			      nfs_port, nconfp);

	if (args->addr == NULL) {
		pfmt(stderr, MM_ERROR, ":15:%s: NFS service not responding\n",
		     fshost);
		return (-1);
	}
	nconf = *nconfp;

	if (stat(nconf->nc_device, &sb) < 0) {
		pfmt(stderr, MM_ERROR, 
		     ":23:%s: cannot %s %s: %s\n",
		     "getaddr_nfs", "stat", nconf->nc_device, strerror(errno));
		return (-1);
	}

	knconf.knc_semantics = nconf->nc_semantics;
	knconf.knc_protofmly = nconf->nc_protofmly;
	knconf.knc_proto = nconf->nc_proto;
	knconf.knc_rdev = sb.st_rdev;

	/* make sure we don't overload the transport */
	if (tinfo.tsdu > 0 && tinfo.tsdu < NFS_MAXDATA + NFS_RPC_HDR) {
		args->flags |= (NFSMNT_RSIZE | NFSMNT_WSIZE);
		if (args->rsize == 0 || args->rsize > tinfo.tsdu - NFS_RPC_HDR)
			args->rsize = tinfo.tsdu - NFS_RPC_HDR;
		if (args->wsize == 0 || args->wsize > tinfo.tsdu - NFS_RPC_HDR)
			args->wsize = tinfo.tsdu - NFS_RPC_HDR;
	}

	args->flags |= NFSMNT_KNCONF;
	args->knconf = &knconf;
	return (0);
}

#define TIME_MAX 16
#define MNTINFO_DEV "dev"

/*
 * add a new entry to the /etc/mnttab file
 */
static void
addtomnttab(mntp)
	struct mnttab *mntp;
{
	FILE *fd;
	char tbuf[TIME_MAX];

	lockmntfile();
	fd = fopen(MNTTAB, "a");
	if (fd == NULL) {
		pfmt(stderr, MM_ERROR, 
		     ":20:%s: cannot open %s: %s\n",
		     "addtomnttab", MNTTAB, strerror(errno));
		exit (RET_MNT_OPEN);
	}

	if (lockf(fileno(fd), F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_ERROR, 
		     ":21:%s: cannot lock %s: %s\n",
		     "addtomnttab", MNTTAB, strerror(errno));
		fclose(fd);
		exit (RET_MNT_LOCK);
	}
	(void) fseek(fd, 0L, 2); /* guarantee at EOF */

	(void) sprintf(tbuf, "%ld", time(0L));
	mntp->mnt_time = tbuf;

	putmntent(fd, mntp);
	fclose(fd);
}

static int
retry(mntp, ro)
	struct mnttab *mntp;
	int ro;
{
	int delay = 5;
	int count = retries;
	int r;

	if (bg) {
		if (fork() > 0)
			return (RET_OK);
		pfmt(stderr, MM_INFO, ":16:background mounting %s\n", 
		     mntp->mnt_mountp);
	}
	pfmt(stderr, MM_INFO, ":17:retry mounting %s\n", mntp->mnt_mountp);

	while (count--) {
		if ((r = mount_nfs(mntp, ro, 0)) == RET_OK) {
			pfmt(stderr, MM_INFO, ":18:%s mounted OK\n",
			     mntp->mnt_mountp);
			return (RET_OK);
		}
		if (r != RET_RETRY)
			break;

		sleep(delay);
		delay *= 2;
		if (delay > 120)
			delay = 120;
	}
	pfmt(stderr, MM_ERROR, ":19:giving up mounting on %s\n", 
	     mntp->mnt_mountp);
	return (RET_GIVE_UP);
}

/*
 * Create and lock a semaphore file to protect /etc/mnttab
 * against simultanous changes. Several programs (e.g. umount)
 * create a temporary file and rename it to /etc/mnttab,
 * so locking just /etc/mnttab does not suffice.
 * lockmntfile() must be called before /etc/mnttab is opened.
 */

#define SEM_FILE	"/etc/.mnt.lock"

lockmntfile()
{
	int sem_file;

	if ((sem_file = creat(SEM_FILE,0600)) == -1) {
		pfmt(stderr, MM_WARNING,
		     ":22:%s: cannot create %s: %s\n",
		     "lockmntfile", SEM_FILE, strerror(errno));
	}
	if (lockf(sem_file,F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_WARNING,
		     ":21:%s: cannot lock %s: %s\n",
		     "lockmntfile", SEM_FILE, strerror(errno));
	}
}

check_nfs()
{
 	enum clnt_stat stat;
	static char res, argp;
	struct netconfig *nconf_udp;
	struct timeval timeout = { 10, 0};

/*
        if ((nconf_udp = getnetconfigent("udp")) == NULL) {
		return(1);
	}

	if ((stat = rpcb_rmtcall(nconf_udp, "localhost", RPCBPROG,
				 RPCBVERS, NULLPROC, xdr_void, &argp, 
				 xdr_void, &res, timeout, 
				 (struct netbuf *)NULL)) != RPC_SUCCESS)
			return(3);
*/
	if (_nfssys(NFS_ISBIODRUN, (caddr_t)NULL))
			return(2);

	return(0);
}

AUTH *
my_authsys_create_default()
{
	char	machname[MAX_MACHINE_NAME + 1];
	int	len;
	uid_t	uid;
	gid_t	gid;
	gid_t	gids[NGRPS];
	AUTH	*dummy;

	if (gethostname(machname, MAX_MACHINE_NAME) == -1) {
		return ((AUTH *)NULL);
	}

	machname[MAX_MACHINE_NAME] = 0;
	uid = geteuid();
	gid = getegid();
	if ((len = getgroups(NGRPS, gids)) < 0) {
		return ((AUTH *)NULL);
	}

	if (server_pre4dot0)
		len = 8;

	dummy = authsys_create(machname, uid, gid, len, gids);
	return (dummy);
}
