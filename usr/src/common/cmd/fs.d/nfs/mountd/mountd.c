#ident	"@(#)mountd.c	1.3"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <syslog.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netconfig.h>
#include <netdir.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/resource.h>
#include <nfs/nfs.h>
#include <nfs/nfssys.h>
#include <rpcsvc/mount.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include "sharetab.h"

#define	DNS_RESOLV_FILE	"/etc/resolv.conf"
#define	DNS_KEYWORD	"domain"

static char	dns_domain[MAXHOSTNAMELEN+1];
static int	dns_mod_time;

static void	get_dns_domain();

#define	MAXRMTABLINELEN	(MAXPATHLEN + MAXHOSTNAMELEN + 2)
#define	MIN(a,b)	((a) < (b) ? (a) : (b))

char			RMTAB[] = "/etc/rmtab";
char			*domain;
int			program = MOUNTPROG;
static			nfs_portmon = 1;

void			rmtab_load();
void			rmtab_delete();
long			rmtab_insert();

struct	groups		**newgroup();
struct	exports		**newexport();
void			mnt();
char			*exmalloc();
void			log_cant_reply();
void			mount();
void			check_sharetab();
void			sh_free();

/*
 * mountd's version of a "struct mountlist". It is the same except
 * for the added ml_pos field.
 */
struct mountdlist {
	char	*ml_name;
	char	*ml_path;
	struct	mountdlist *ml_nxt;
	long	ml_pos;		/* position of mount entry in RMTAB */
};

struct	mountdlist	*mountlist;

/*
 * cached share list
 */
struct sh_list {
	struct sh_list *shl_next;
	struct share    shl_sh;
} *sh_list;

main(int argc, char **argv)
{
	struct	rlimit	rl;
	extern	char	*optarg;
	char		c;
	int		i;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:mountd");
	
	while ((c = getopt(argc, argv, "np:")) != (char)EOF) {
		switch(c) {
		case 'p':
			program = atoi(optarg);
			if (program <= 0) {
				pfmt(stderr, MM_ERROR, 
				     ":87:bad program number\n");
				exit(1);
			}
			break;
	        case 'n':
			nfs_portmon = 0;
			break;
		default :
			usage();
			exit(1);
		}
	}

	if (nfs_portmon)
		_nfssys(NFS_PORTMON_ON, (caddr_t)NULL);
	else
		_nfssys(NFS_PORTMON_OFF, (caddr_t)NULL);

#ifndef DEBUG

	switch(fork()) {
	case 0:
		break;
	case -1:
		pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
		     "fork", strerror(errno));
		exit(1);
	default:
		exit(0);
	}

	/*
	 * close existing file descriptors, open "/dev/null" as
	 * standard input, output, and error, and detach from
	 * controlling terminal.
	 */
	getrlimit(RLIMIT_NOFILE, &rl);
	for (i = 0 ; i < rl.rlim_cur ; i++)
		(void) close(i);
	(void) open("/dev/null", O_RDONLY);
	(void) open("/dev/null", O_WRONLY);
	(void) dup(1);
	(void) setsid();

#endif

	openlog("mountd", LOG_PID, LOG_DAEMON);

	/*
	 * create datagram service
	 */
	if (svc_create(mnt, program, MOUNTVERS, "datagram_v") == 0) {
		syslog(LOG_ERR, 
		       gettxt(":88", 
			      "cannot register (program %d, version %d) on %s"),
		       program, MOUNTVERS, "datagram_v");
		exit(1);
	}

	/*
	 * create connection oriented service
	 */
	if (svc_create(mnt, program, MOUNTVERS, "circuit_v") == 0) {
		syslog(LOG_ERR, 
		       gettxt(":88",
			      "cannot register (program %d, version %d) on %s"),
		       program, MOUNTVERS, "circuit_v");
		exit(1);
	}

#ifdef YPNFS
	/*
	 * initialize the NIS world
	 */
	(void) yp_get_default_domain(&domain);
#endif

	/*
	 * start serving 
	 */
	rmtab_load();
	svc_run();
	syslog(LOG_ERR, gettxt(":89", "svc_run returned"));

	/* NOTREACHED */
}

/*
 * server procedure switch routine 
 */
void
mnt(struct svc_req *rqstp, SVCXPRT *transp)
{
	switch (rqstp->rq_proc) {
	case NULLPROC:
		errno = 0;
		if (!svc_sendreply(transp, xdr_void, (char *)0))
			log_cant_reply(transp);
		return;
	case MOUNTPROC_MNT:
		mount(rqstp);
		return;
	case MOUNTPROC_DUMP:
		errno = 0;
		if (!svc_sendreply(transp, xdr_mountlist, (char *)&mountlist))
			log_cant_reply(transp);
		return;
	case MOUNTPROC_UMNT:
		umount(rqstp);
		return;
	case MOUNTPROC_UMNTALL:
		umountall(rqstp);
		return;
	case MOUNTPROC_EXPORT:
	case MOUNTPROC_EXPORTALL:
		export(rqstp);
		return;
	default:
		svcerr_noproc(transp);
		return;
	}
}

/*
 * get the client's hostname from the transport handle
 * If the name is not available then return "(anon)".
 */
struct nd_hostservlist *
getclientsnames(SVCXPRT *transp)
{
	struct	netbuf			*nbuf;
	struct	netconfig		*nconf;
	static	struct nd_hostservlist	*serv;
	static	struct nd_hostservlist	anon_hsl;
	static	struct nd_hostserv	anon_hs;
	static	char			anonname[] = "(anon)";
	static	struct nd_hostservlist	local_hsl;
	static 	char			localname[] = "localhost";
	static	char			servname[] = "";
	static	char			myname[256];
	int				x;
	char				*hp;
	struct nd_hostserv 		*hostservp;

	gethostname(myname, sizeof(myname));

	/*
	 * set up anonymous client
	 */
	anon_hs.h_host = anonname;
	anon_hs.h_serv = servname;
	anon_hsl.h_cnt = 1;
	anon_hsl.h_hostservs = &anon_hs;

	/*
	 * set up localhost client
	 */
	local_hsl.h_cnt = 2;
	if ((local_hsl.h_hostservs == NULL) &&
	    ((local_hsl.h_hostservs = (struct nd_hostserv *)
	     calloc(local_hsl.h_cnt, sizeof(struct nd_hostserv))) == NULL)) {
		return (NULL);
        }
	hostservp = local_hsl.h_hostservs;
	hostservp->h_host = localname;
	hostservp->h_serv = servname;
	hostservp++;
	hostservp->h_host = myname;
	hostservp->h_serv = servname;

	if (serv) {
		netdir_free((char *) serv, ND_HOSTSERVLIST);
		serv = NULL;
	}

	nconf = getnetconfigent(transp->xp_netid);
	if (nconf == NULL) {
		syslog(LOG_ERR, 
		       gettxt(":90", "%s failed for %s: %s"),
		       "getnetconfigent", transp->xp_netid, nc_sperror());
		return (&anon_hsl);
	}

	nbuf = svc_getrpccaller(transp);
	if (nbuf == NULL) {
		freenetconfigent(nconf);
		return (&anon_hsl);
	}

	if (netdir_getbyaddr(nconf, &serv, nbuf)) {
		freenetconfigent(nconf);
		return (&anon_hsl);
	}
	freenetconfigent(nconf);

	/* make all hostnames lower case. */
	for(x = 0; x < serv->h_cnt; x++) {
		hp = serv->h_hostservs[x].h_host;
		while (*hp) {
			*hp = (char)tolower(*hp);
			hp++;
		}
	}

	/*
	 * check to see if it is the local machine if it is then
	 * return that its the localhost.
	 */

	for(x=0; x < serv->h_cnt; x++) {
		if (!strcmp(serv->h_hostservs[x].h_host,localname))
			return(&local_hsl);
		if (!strcmp(serv->h_hostservs[x].h_host,myname))
			return(&local_hsl);

	}

	return (serv);
}

void
log_cant_reply(SVCXPRT *transp)
{
	struct	nd_hostservlist	*clnames;
	int			saverrno;
	char			*name;

	saverrno = errno;
	clnames = getclientsnames(transp);
	if (clnames == NULL)
		return;
	name = clnames->h_hostservs->h_host;

	errno = saverrno;
	if (errno == 0)
		syslog(LOG_ERR, gettxt(":91", "could not send reply to %s"), 
		       name);
	else
		syslog(LOG_ERR, gettxt(":92", "could not send reply to %s: %m"), 
		       name);
}

/*
 * check mount requests, add to mounted list if ok 
 */
void
mount(struct svc_req *rqstp)
{
	struct	nd_hostservlist	*clnames;
	struct	fhstatus	fhs;
	struct	mountdlist	*ml;
	struct	share		*sh;
	struct	share		*findentry();
	struct	stat		st;
	SVCXPRT			*transp;
	fhandle_t		fh;
	char			*path, rpath[MAXPATHLEN];
	char			*gr, *grl;
	char			**aliases;
	int			i, restricted;
	char			*name;

	transp = rqstp->rq_xprt;
	path = NULL;
	fhs.fhs_status = 0;

	if (nfs_portmon) {
		struct sockaddr	*ca;

		ca = (struct sockaddr *) svc_getrpccaller(transp)->buf;

		if ((ca->sa_family == AF_INET) &&
		    (ntohs(((struct sockaddr_in *) ca)->sin_port) >=
		     IPPORT_RESERVED)) {
			syslog(LOG_ERR,
			       "mount request from an unprivileged port");
			fhs.fhs_status = EPERM;
			goto done;
		}
	}

	clnames = getclientsnames(transp);
	if (clnames == NULL) {
		fhs.fhs_status = EACCES;
		goto done;	
	}
	name = clnames->h_hostservs[0].h_host;

	if (!strcmp(name,"localhost"))
	{
		/*
		 * use the name used in the authentication
		 * hope they always use auth unix
		 */
		name =
		  ((struct authunix_parms *)rqstp->rq_clntcred)->aup_machname; 
	} 

	if (!svc_getargs(transp, xdr_path, (caddr_t)&path)) {
		svcerr_decode(transp);
		return;
	}

	if (lstat(path, &st) < 0) {
		fhs.fhs_status = EACCES;
		goto done;	
	}

	(void) strcpy(rpath, path);

	if (nfs_getfh(rpath, &fh) < 0) {
		fhs.fhs_status = errno == EINVAL ? EACCES : errno;
		syslog(LOG_DEBUG, 
		       gettxt(":93", "mount request: %s failed for %s: %s"),
		       "nfs_getfh", path, strerror(errno));
		goto done;
	}

	sh = findentry(rpath);
	if (sh == NULL) {
		fhs.fhs_status = EACCES;
		goto done;
	}

	/*
	 * check "ro" list (used to be "access" list)
	 * Try hostnames first - then netgroups.
	 */

	restricted = 0;

	grl = getshareopt(sh->sh_opts, SHOPT_RO);
	if (grl != NULL) {
		if (*grl == '\0') {
			/*
			 * no list
			 */
			goto done;
		}
		restricted++;
		if (in_list(grl, clnames))
			goto done;
	}

#ifdef YPNFS

	grl = getshareopt(sh->sh_opts, SHOPT_RO);
	if (grl != NULL) {
		while ((gr = strtok(grl, ":")) != NULL) {
			grl = NULL;
			for (i = 0 ; i < clnames->h_cnt ; i++) {
				name = clnames->h_hostservs[i].h_host;
				if (in_netgroup(gr, name, domain)) {
					goto done;
				}
			}
		}
	}

#endif
	
	/*
	 * Check "rw" list (hostname only)
	 */
	grl = getshareopt(sh->sh_opts, SHOPT_RW);
	if (grl != NULL) {
		if (*grl == '\0') {
			/*
			 * no list
			 */
			goto done;
		}
		restricted++;
		if (in_list(grl, clnames))
			goto done;
	}

	/*
	 * Check "root" list (hostname only)
	 */

	grl = getshareopt(sh->sh_opts, SHOPT_ROOT);
	if (grl != NULL) {
		if (in_list(grl, clnames))
			goto done;
	}

	if (restricted)
		fhs.fhs_status = EACCES;

done:
	if (fhs.fhs_status == 0)
		fhs.fhs_fh = fh;
	errno = 0;
	if (!svc_sendreply(transp, xdr_fhstatus, (char *)&fhs))
		log_cant_reply(transp);
	if (path != NULL)
		svc_freeargs(transp, xdr_path, (caddr_t)&path);
	if (fhs.fhs_status)
		return;

	/*
	 * add an entry for this mount to the mount list.
	 * first check whether it's there already, the client
	 * may have crashed and be rebooting.
	 */
	for (ml = mountlist; ml != NULL ; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_path, rpath) == 0) {
			if (strcmp(ml->ml_name, name) == 0) {
				return;
			}
		}
	}

	/*
	 * add this mount to the mount list.
	 */
	ml = (struct mountdlist *) exmalloc(sizeof(struct mountdlist));
	ml->ml_path = (char *) exmalloc(strlen(rpath) + 1);
	(void) strcpy(ml->ml_path, rpath);
	ml->ml_name = (char *) exmalloc(strlen(name) + 1);
	(void) strcpy(ml->ml_name, name);
	ml->ml_nxt = mountlist;
	ml->ml_pos = rmtab_insert(name, rpath);
	mountlist = ml;

	return;
}

struct share *
findentry(char *path)
{
	struct	share	*sh;
	struct	sh_list	*shp;
	char		*p1, *p2;

	check_sharetab();

	for (shp = sh_list ; shp ; shp = shp->shl_next) {
		sh = &shp->shl_sh;
		for (p1 = sh->sh_path, p2 = path ; *p1 == *p2 ; p1++, p2++)
			if (*p1 == '\0') {
				/*
				 * exact match
				 */
				return (sh);
			}

		if ((*p1 == '\0' && *p2 == '/' ) ||
		    (*p1 == '\0' && *(p1-1) == '/') ||
		    (*p2 == '\0' && *p1 == '/' && *(p1+1) == '\0')) {
			if (issubdir(path, sh->sh_path))
				return (sh);
		}
	}

	return ((struct share *) 0);
}

#ifdef YPNFS

#define MAXGRPLIST 256
/*
 * use cached netgroup info if the previous call was
 * from the same client.  two lists are maintained
 * here: groups the client is a member of, and groups
 * the client is not a member of.
 */
int
in_netgroup(char *group, char *hostname, char *domain)
{
	static	char	prev_hostname[MAXHOSTNAMELEN+1];
	static	char	grplist[MAXGRPLIST+1], nogrplist[MAXGRPLIST+1];
	char		key[256];
	char		*ypline = NULL;
	int		yplen;
	char		*gr, *p;
	static		time_t last;
	time_t		time();
	time_t		time_now;
	static		int cache_time = 30;

	time_now = time((long *)0);
	if (time_now > last + cache_time ||
	    strcmp(hostname, prev_hostname) != 0) {
		last = time_now;
		(void) strcpy(key, hostname);
		(void) strcat(key, ".");
		(void) strcat(key, domain);
		if (yp_match(domain, "netgroup.byhost", key,
		    strlen(key), &ypline, &yplen) == 0) {
			(void) strncpy(grplist, ypline, MIN(yplen, MAXGRPLIST));
			free(ypline);
		} else {
			grplist[0] = '\0';
		}
		nogrplist[0] = '\0';
		(void) strcpy(prev_hostname, hostname);
	}

	for (gr = grplist ; *gr ; gr = p ) {
		for (p = gr ; *p && *p != ',' ; p++)
			;
		if (strncmp(group, gr, p - gr) == 0)
			return (1);
		if (*p == ',')
			p++;
	}

	for (gr = nogrplist ; *gr ; gr = p ) {
		for (p = gr ; *p && *p != ',' ; p++)
			;
		if (strncmp(group, gr, p - gr) == 0)
			return (0);
		if (*p == ',')
			p++;
	}

	if (innetgr(group, hostname, (char *)NULL, domain)) {
		if (strlen(grplist)+1+strlen(group) > MAXGRPLIST)
			return (1);
		if (*grplist)
			(void) strcat(grplist, ",");
		(void) strcat(grplist, group);
		return (1);
	} else {
		if (strlen(nogrplist)+1+strlen(group) > MAXGRPLIST)
			return (0);
		if (*nogrplist)
			(void) strcat(nogrplist, ",");
		(void) strcat(nogrplist, group);
		return (0);
	}
}

#endif /* YPNFS */

void
check_sharetab()
{
	FILE *f;
	struct stat st;
	static long last_sharetab_time;
	struct share *sh;
	struct sh_list *shp, *shp_prev;
	int res, c = 0;

	/*
	 * read in /etc/dfs/sharetab if it has changed 
	 */

	if (stat(SHARETAB, &st) != 0) {
		syslog(LOG_ERR, gettxt(":94", "%s: %s failed for %s: %m"), 
		       "check_sharetab", "stat", SHARETAB);
		return;
	}

	if (st.st_mtime == last_sharetab_time)
		return;

	sh_free(sh_list);
	sh_list = NULL;
	
	f = fopen(SHARETAB, "r");
	if (f == NULL)
		return;

	while ((res = getshare(f, &sh)) > 0) {
		c++;
		if (strcmp(sh->sh_fstype, "nfs") != 0)
			continue;

		shp = (struct sh_list *)malloc(sizeof(struct sh_list));
		if (shp == NULL)
			goto alloc_failed;
		if (sh_list == NULL)
			sh_list = shp;
		else
			shp_prev->shl_next = shp;
		shp_prev = shp;
		memset((char *)shp, 0, sizeof(struct sh_list));
		shp->shl_sh.sh_path = strdup(sh->sh_path);
		if (shp->shl_sh.sh_path == NULL)
			goto alloc_failed;
		if (sh->sh_opts) {
			shp->shl_sh.sh_opts = strdup(sh->sh_opts);
			if (shp->shl_sh.sh_opts == NULL)
				goto alloc_failed;
		}
	}

	if (res < 0)
		syslog(LOG_ERR, gettxt(":95", "%s: invalid at line %d\n"),
			SHARETAB, c + 1);

	(void) fclose(f);
	last_sharetab_time = st.st_mtime;

	return;

alloc_failed:
	syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "check_sharetab");
	sh_free(sh_list);
	sh_list = NULL;
	(void) fclose(f);

	return;
}

void
sh_free(struct sh_list *shp)
{
	struct	sh_list	*next;

	while (shp) {
		if (shp->shl_sh.sh_path)
			free(shp->shl_sh.sh_path);
		if (shp->shl_sh.sh_opts)
			free(shp->shl_sh.sh_opts);
		next = shp->shl_next;
		free((char *)shp);
		shp = next;
	}
}

/*
 * remove an entry from mounted list 
 */
umount(struct svc_req *rqstp)
{
	char			*path;
	struct	mountdlist	*ml, *oldml;
	struct	nd_hostservlist	*clnames;
	SVCXPRT			*transp;
	char			*name;

	transp = rqstp->rq_xprt;
	path = NULL;
	if (!svc_getargs(transp, xdr_path, (caddr_t)&path)) {
		svcerr_decode(transp);
		return;
	}

	errno = 0;
	if (!svc_sendreply(transp, xdr_void, (char *)NULL))
	log_cant_reply(transp);

	clnames = getclientsnames(transp);
	name = clnames->h_hostservs->h_host;
	if (!strcmp(name,"localhost"))
	{
		/*
		 * use the name used in the authentication
		 * hope they always use auth unix
		 */
		name =
		  ((struct authunix_parms *)rqstp->rq_clntcred)->aup_machname; 
	} 

	if (clnames != NULL) {
		oldml = mountlist;
		for (ml = mountlist; ml != NULL;
		     oldml = ml, ml = ml->ml_nxt) {
			if (strcmp(ml->ml_path, path) == 0 &&
			    strcmp(ml->ml_name, name ) == 0) {
				if (ml == mountlist) {
					mountlist = ml->ml_nxt;
				} else {
					oldml->ml_nxt = ml->ml_nxt;
				}
				rmtab_delete(ml->ml_pos);
				free(ml->ml_path);
				free(ml->ml_name);
				free((char *)ml);
				break;
			}
		}
	}
	svc_freeargs(transp, xdr_path, (caddr_t)&path);
}

/*
 * remove all entries for one machine from mounted list 
 */
umountall(struct svc_req *rqstp)
{
	struct	mountdlist	*ml, *oldml;
	struct	nd_hostservlist	*clnames;
	SVCXPRT			*transp;
	char			*name;

	transp = rqstp->rq_xprt;
	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}

	/*
	 * We assume that this call is asynchronous and made via rpcbind
	 * callit routine.  Therefore return control immediately. The error
	 * causes rpcbind to remain silent, as apposed to every machine
	 * on the net blasting the requester with a response. 
	 */
	svcerr_systemerr(transp);
	clnames = getclientsnames(transp);
	if (clnames == NULL)
		return;

	name = clnames->h_hostservs->h_host;
	if (!strcmp(name,"localhost"))
		name =
		  ((struct authunix_parms *)rqstp->rq_clntcred)->aup_machname; 

	oldml = mountlist;
	for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_name, name) == 0) {
			if (ml == mountlist) {
				mountlist = ml->ml_nxt;
				oldml = mountlist;
			} else {
				oldml->ml_nxt = ml->ml_nxt;
			}
			rmtab_delete(ml->ml_pos);
			free(ml->ml_path);
			free(ml->ml_name);
			free((char *)ml);
		} else {
			oldml = ml;
		}
	}
}

/*
 * send current export list 
 */
export(struct svc_req *rqstp)
{
	struct	exports	*ex;
	struct	exports	**tail;
	struct	groups	*groups;
	struct	groups	**grtail;
	struct	share	*sh;
	struct	sh_list	*shp;
	char		*grl;
	SVCXPRT		*transp;
	char		*gr;

	transp = rqstp->rq_xprt;
	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}

	check_sharetab();

	ex = NULL;
	tail = &ex;
	for (shp = sh_list ; shp ; shp = shp->shl_next) {
		sh = &shp->shl_sh;

		grl = getshareopt(sh->sh_opts, SHOPT_RO);
		groups = NULL;
		grtail = &groups;
		if (grl != NULL) {
			while ((gr = strtok(grl, ":")) != NULL) {
				grl = NULL;
				grtail = newgroup(gr, grtail);
			}
		}
		grl = getshareopt(sh->sh_opts, SHOPT_RW);
		if (grl != NULL) {
			while ((gr = strtok(grl, ":")) != NULL) {
				grl = NULL;
				grtail = newgroup(gr, grtail);
			}
		}
		tail = newexport(sh->sh_path, groups, tail);
	}

	errno = 0;
	if (!svc_sendreply(transp, xdr_exports, (char *)&ex))
		log_cant_reply(transp);
	freeexports(ex);
}

freeexports(struct exports *ex)
{
	struct	groups	*groups, *tmpgroups;
	struct	exports	*tmpex;

	while (ex) {
		groups = ex->ex_groups;
		while (groups) {
			tmpgroups = groups->g_next;
			free(groups->g_name);
			free((char *)groups);
			groups = tmpgroups;
		}
		tmpex = ex->ex_next;
		free(ex->ex_name);
		free((char *)ex);
		ex = tmpex;
	}
}

struct groups **
newgroup(char *name, struct groups **tail)
{
	struct	groups	*new;
	char		*newname;

	new = (struct groups *) exmalloc(sizeof(*new));
	newname = (char *) exmalloc(strlen(name) + 1);
	(void) strcpy(newname, name);

	new->g_name = newname;
	new->g_next = NULL;
	*tail = new;

	return (&new->g_next);
}

struct exports **
newexport(char *name, struct groups *groups, struct exports **tail)
{
	struct	exports	*new;
	char		*newname;

	new = (struct exports *) exmalloc(sizeof(*new));
	newname = (char *) exmalloc(strlen(name) + 1);
	(void) strcpy(newname, name);

	new->ex_name = newname;
	new->ex_groups = groups;
	new->ex_next = NULL;
	*tail = new;

	return (&new->ex_next);
}

char *
exmalloc(int size)
{
	char	*ret;

	if ((ret = (char *) malloc((u_int)size)) == 0) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "exmalloc");
		exit(1);
	}

	return (ret);
}

usage()
{
	pfmt(stderr, MM_ACTION, ":249:Usage: mountd\n");
}

FILE	*f;

void
rmtab_load()
{
	struct	mountdlist	*ml;
	char			*path;
	char			*name;
	char			*end;
	char			line[MAXRMTABLINELEN];

	mountlist = NULL;
	f = fopen(RMTAB, "r");
	if (f != NULL) {
		while (fgets(line, sizeof(line), f) != NULL) {
			name = line;
			path = strchr(name, ':');
			if (*name != '#' && path != NULL) {
				*path = 0;
				path++;
				end = strchr(path, '\n');
				if (end != NULL) {
					*end = 0;
				}
				ml = (struct mountdlist *) 
					exmalloc(sizeof(struct mountdlist));
				ml->ml_path = (char *)
					exmalloc(strlen(path) + 1);
				(void) strcpy(ml->ml_path, path);
				ml->ml_name = (char *)
					exmalloc(strlen(name) + 1);
				(void) strcpy(ml->ml_name, name);
				ml->ml_nxt = mountlist;
				mountlist = ml;
			}
		}
		(void) fclose(f);
		(void) truncate(RMTAB, (off_t)0);
	} 
	f = fopen(RMTAB, "w+");
	if (f != NULL) {
		for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
			ml->ml_pos = rmtab_insert(ml->ml_name, ml->ml_path);
		}
	}
}

long
rmtab_insert(char *name, char* path)
{
	long	pos;

	if (f == NULL || fseek(f, 0L, 2) == -1) {
		return (-1);
	}

	pos = ftell(f);
	if (fprintf(f, "%s:%s\n", name, path) == EOF) {
		return (-1);
	}

	(void) fflush(f);

	return (pos);
}

void
rmtab_delete(long pos)
{
	if (f != NULL && pos != -1 && fseek(f, pos, 0) == 0) {
		(void) fprintf(f, "#");
		(void) fflush(f);
	}
}

/*
 * Compares the client's name to the clients in the access list.
 * If there is a match, return 1.  Otherwise, return 0.
 * Only check for fully qualified DNS names if either the
 * name from the access list or the client name is a DNS pathname.
 */
in_list(char *clientlist, struct nd_hostservlist *clnames)
{
	char	*client, *name, *clientp=NULL, *namep=NULL;
	int	i, clientlen, namelen;

	while ((client = strtok(clientlist, ":")) != NULL) {
		clientlist = NULL;
		for (i = 0 ; i < clnames->h_cnt ; i++) {
			name = clnames->h_hostservs[i].h_host;

			if (strcmp(client, name) == 0)
				return(1);

			/*
			 * If they are both full DNS pathnames or 
			 *    they are both NOT full DNS pathnames,
			 * then they do not match.
			 */
			clientp = strchr(client, '.');
			namep = strchr(name, '.');

			if ((clientp == NULL && namep == NULL) ||
			    (clientp != NULL && namep != NULL))
				continue;

			/* Set the DNS domain if not set yet. */
			get_dns_domain();
			if (dns_domain[0] == '\0')
				continue;

			if (namep != NULL) {
				namelen = strlen(name) - strlen(namep);

				if ((strlen(client) == namelen) &&
				    (strncmp(client, name, namelen) == 0) &&
				    (strcmp(++namep, dns_domain) == 0))
					return(1);
			} else {
				clientlen = strlen(client) - strlen(clientp);

				if ((strlen(name) == clientlen) &&
				    (strncmp(client, name, clientlen) == 0) &&
				    (strcmp(++clientp, dns_domain) == 0))
					return(1);
			}
		}
	}
	return(0);
}

/*
 * Get the DNS local domain from /etc/resolv.conf and
 * set the dns_domain variable.  If /etc/resolv.conf has not
 * been modified since we last read it, then dns_domain
 * is still valid, so just return.
 */
void
get_dns_domain()
{
	register FILE *fp;
	register char *cp, *dp;
	char buf[BUFSIZ];
	int i;
	struct stat statbuf;
	int keyword_size;

	if (stat(DNS_RESOLV_FILE, &statbuf) < 0) {
		syslog(LOG_ERR, gettxt(":94", "%s: %s failed for %s: %m"),
		       "get_dns_domain", "stat", DNS_RESOLV_FILE);
		dns_domain[0] = '\0';
		return;
	}

	if (dns_mod_time == statbuf.st_mtime)
		return;

	dns_mod_time = statbuf.st_mtime;
	dns_domain[0] = '\0';

	fp = fopen(DNS_RESOLV_FILE, "r");
	if (fp == (FILE *)NULL)
		return;

	keyword_size = strlen(DNS_KEYWORD);

	while (fgets(buf, BUFSIZ, fp) != NULL) {
		if (strncmp(buf, DNS_KEYWORD, keyword_size) == 0) {
			cp = buf + keyword_size;

			while (*cp == ' ' || *cp == '\t')
				cp++;

			for (i = 0; i < MAXHOSTNAMELEN && *cp != '\0'; i++)
				dns_domain[i] = (char)tolower(*cp++);

			if((cp = strchr(dns_domain, '\n')) != NULL)
				*cp = '\0';
			else
				dns_domain[MAXHOSTNAMELEN] = '\0';

			break;
		}
	}
	fclose(fp);

	return;
}
