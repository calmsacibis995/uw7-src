#ident	"@(#)nfsd.c	1.4"
#ident  "$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <tiuser.h>
#include <rpc/rpc.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/file.h>
#include <nfs/nfs.h>
#include <nfs/nfsv3.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <netconfig.h>
#include <netdir.h>
#include <locale.h>
#include <pfmt.h>

extern int t_errno;

void			usage();
void			do_one();
void			do_all();
char			*defaultproto = NC_UDP;

#define	NETSELEQ(x, y)	(!strcmp((x), (y)))

/* ARGSUSED */
void 
catch(int junk)
{
}

main(int ac, char **av)
{
	char	*dir = "/";
	int	allflag = 0;
	int	pid;
	int	i;
	extern	int optind;
	extern	char *optarg;
	void	do_one();
	char	*provider = (char *) NULL;
	char	*proto = defaultproto;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:nfsd");

	while ((i = getopt(ac, av, "ap:t:")) != EOF) {
		switch (i) {
			case 'a':
				allflag = 1;
				break;
			case 'p':
				proto = optarg;
				break;

			case 't':
				provider = optarg;
				break;

			case '?':
				usage();
				exit(1);
				/* NOTREACHED */
		}
	}

	if (optind != ac) {
		usage();
		exit(1);
	}

	/*
	 * set current and root dir to server root
	 */
	if (chroot(dir) < 0) {
		pfmt(stderr, MM_ERROR, ":33:%s failed for %s: %s\n",
		     "chroot", dir, strerror(errno));
		exit(1);
	}
	if (chdir(dir) < 0) {
		pfmt(stderr, MM_ERROR, ":33:%s failed for %s: %s\n",
		     "chdir", dir, strerror(errno));
		exit(1);
	}

#ifndef DEBUG

	/*
	 * background 
	 */
        pid = fork();
	if (pid < 0) {
		pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
		     "fork", strerror(errno));
		exit(1);
	}

	if (pid != 0) {
		exit(0);
	}

	{
		int		i;
		struct	rlimit	rl;

		/*
		 * close existing file descriptors, open "/dev/null" as
		 * standard input, output, and error, and detach from
		 * controlling terminal.
		 */
		getrlimit(RLIMIT_NOFILE, &rl);
		for (i = 0; i < rl.rlim_cur; i++)
			(void) close(i);
		(void) open("/dev/null", O_RDONLY);
		(void) open("/dev/null", O_WRONLY);
		(void) dup(1);
		(void) setsid();
	}

#endif

	if (allflag)
		do_all();
	else
		do_one(provider, proto);

	/* NOTREACHED */
}

void
do_one(char *provider, char *proto)
{
	struct	netbuf		*retaddr;
	struct	netconfig	*retnconf;
	int			sock;

	if (provider)
		sock = bind_to_provider(provider, &retaddr, &retnconf);
	else
		sock = bind_to_proto(proto, &retaddr, &retnconf);

	if (sock == -1) {
		pfmt(stderr, MM_ERROR, ":101:could not bind\n");
		exit(1);
	}

	rpcb_unset(NFS_PROGRAM, NFS_VERSION, retnconf);
	rpcb_set(NFS_PROGRAM, NFS_VERSION, retnconf, retaddr);

#ifdef NFSV3
	rpcb_unset(NFS3_PROGRAM, NFS3_VERSION, retnconf);
	rpcb_set(NFS3_PROGRAM, NFS3_VERSION, retnconf, retaddr);
#endif /* NFSV3 */

#ifdef NFSESV
	rpcb_unset(NFS_ESVPROG, NFS_ESVVERS, retnconf);
	rpcb_set(NFS_ESVPROG, NFS_ESVVERS, retnconf, retaddr);
#endif

	signal(SIGTERM, catch);
	nfssvc(sock);

	/* NOTREACHED */
}

void
do_all()
{
	struct	netconfig	*nconf;
	NCONF_HANDLE		*nc;

	if ((nc = setnetconfig()) == (NCONF_HANDLE *) NULL) {
		pfmt(stderr, MM_ERROR, ":102:%s: %s failed: %s\n",
		     "do_all", "setnetconfig", nc_sperror());
		exit(1);
	}

	while (nconf = getnetconfig(nc)) {
		if ((nconf->nc_flag & NC_VISIBLE) &&
		    nconf->nc_semantics == NC_TPI_CLTS) {
			switch (fork()) {
				case -1:
					pfmt(stderr, MM_ERROR, 
					     ":40:%s failed: %s\n",
					     "fork", strerror(errno));
					exit(1);
				case 0:
					do_one(nconf->nc_device,
							nconf->nc_proto);
					break;
				default:
					continue;
			}
		}
	}
	endnetconfig(nc);

	exit (0);
}

int
bind_to_provider(char *provider, struct netbuf **addr,
		 struct netconfig **retnconf)
{
	struct	netconfig	*nconf;
	NCONF_HANDLE 		*nc;

	if ((nc = setnetconfig()) == (NCONF_HANDLE *) NULL) {
		pfmt(stderr, MM_ERROR, ":102:%s: %s failed: %s\n",
		     "bind_to_provider", "setnetconfig", nc_sperror());
		return (-1);
	}

	while (nconf = getnetconfig(nc)) {
		if ((nconf->nc_semantics == NC_TPI_CLTS) && 
		     !strcmp(nconf->nc_device, provider)) {
			*retnconf = nconf;
			return (bindit(nconf, addr));
		}
	}
	endnetconfig(nc);

	pfmt(stderr, MM_ERROR, 
	     ":103:could not find NC_TPI_CLTS netconfig entry for protocol %s\n",
	     provider);

	return (-1);
}

int
bind_to_proto(char *proto, struct netbuf **addr, struct netconfig **retnconf)
{
	struct	netconfig	*nconf;
	NCONF_HANDLE 		*nc = NULL;

	if ((nc = setnetconfig()) == (NCONF_HANDLE *) NULL) {
		pfmt(stderr, MM_ERROR, ":102:%s: %s failed: %s\n",
		     "bind_to_proto", "setnetconfig", nc_sperror());
		return (-1);
	}

	while (nconf = getnetconfig(nc)) {
		if ((nconf->nc_semantics == NC_TPI_CLTS) && 
		    NETSELEQ(nconf->nc_proto, proto)) {
			*retnconf = nconf;
			return (bindit(nconf, addr));
		}
	}
	endnetconfig(nc);

	pfmt(stderr, MM_ERROR, 
	     ":103:could not find NC_TPI_CLTS netconfig entry for protocol %s\n",
	     proto);

	return (-1);
}

int
bindit(struct netconfig *nconf, struct netbuf **addr)
{
	struct			t_bind *tb = (struct t_bind *) NULL, *ntb;
	struct	nd_hostserv	hs;
	struct	nd_addrlist	*addrlist;
	int			fd;

	if ((nconf == (struct netconfig *) NULL) || 
	   (nconf->nc_device == (char *) NULL)) {
		pfmt(stderr, MM_ERROR, ":104:no netconfig device\n");
		return (-1);
	}

	if ((fd = t_open(nconf->nc_device, O_RDWR, (struct t_info *) NULL))
							== -1) {
		pfmt(stderr, MM_ERROR, ":60:%s failed for %s\n",
		     "t_open", nconf->nc_device);
		return (-1);
	}

	hs.h_host = HOST_SELF;
	hs.h_serv = "nfsd";
	if (netdir_getbyname(nconf, &hs, &addrlist) != 0) {
		pfmt(stderr, MM_ERROR, ":60:%s failed for %s\n",
		     "netdir_getbyname", nconf->nc_netid);
		(void) t_close(fd);
		return (-1);
	}

	if ((tb = (struct t_bind *) t_alloc(fd, T_BIND, T_ALL)) ==
						(struct t_bind *) NULL) {
		pfmt(stderr, MM_ERROR, ":60:%s failed for %s\n",
		     "t_alloc", "tb");
		(void) t_close(fd);
		return (-1);
	}

	if ((ntb = (struct t_bind *) t_alloc(fd, T_BIND, T_ALL)) ==
						(struct t_bind *) NULL) {
		pfmt(stderr, MM_ERROR, ":60:%s failed for %s\n",
		     "t_alloc", "ntb");
		if (tb != (struct t_bind *) NULL)
			(void) t_free((char *) tb, T_BIND);
		(void) t_close(fd);
		return (-1);
	}

	tb->addr = *(addrlist->n_addrs);

	if (t_bind(fd, tb, ntb) == -1) {
		pfmt(stderr, MM_ERROR, ":105:%s: %s failed\n",
		     "bindit", "t_bind");
		(void) t_free((char *) ntb, T_BIND);
		if (tb != (struct t_bind *) NULL)
			(void) t_free(( char *) tb, T_BIND);
		(void) t_close(fd);
		return (-1);
	}

	/* make sure we bound to the right address */
	if (tb->addr.len != ntb->addr.len ||
			memcmp(tb->addr.buf, ntb->addr.buf, tb->addr.len)) {
		pfmt(stderr, MM_ERROR, ":106:%s: bind to wrong address\n",
		     "bindit");
		(void) t_free((char *) ntb, T_BIND);
		(void) t_free((char *) tb, T_BIND);
		(void) t_close(fd);
		return (-1);
	}
	*addr = &ntb->addr;

	return (fd);
}

void
usage()
{
	pfmt(stderr, MM_ACTION, 
	     ":100:Usage: nfsd [ -a ] [ -p protocol ] [ -t transport ]\n");
}
