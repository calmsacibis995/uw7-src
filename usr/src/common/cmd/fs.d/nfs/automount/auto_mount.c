/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 *
 * Revision history:
 *
 * 27th Oct 1997	tonylo
 *	Fix for ul95-06701. 
 *	When the automounter first mounts a filesystem, it sends the following 
 *	error message to syslog with level ERROR:
 *
 *	get_addr: rpcb_getaddr failed for /dev/ticlts: Error 0
 *
 *	This is incorrect because it is to be expected that ticlts will fail,
 *	and so an error should not be reported if it does.
 *
 */

#ident	"@(#)auto_mount.c	1.4"
#ident	"$Header$"

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#define _NSL_RPC_ABI
#include <rpc/rpc.h>
#include <syslog.h>
#include <rpc/pmap_clnt.h>
#include <sys/mount.h>
#include <sys/mnttab.h>
#include <sys/mntent.h>
#include <netdb.h>
#include <errno.h>
#include "nfs_prot.h"
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#define NFSCLIENT
#include <nfs/mount.h>
#include <netconfig.h>
#include <netdir.h>
#include <locale.h>
#include <setjmp.h>
#include "automount.h"

#define MAXHOSTS	20
#define MAXSUBNETS	20
#define CACHE_HOST_SIZE	10
#define	MNTTAB_UID	0
#define	MNTTAB_GID	3
#define	TMPTAB		"/etc/.tmptab"
#define SEM_FILE	"/etc/.mnt.lock"
#define TIME_MAX	16
#define	MIN(a, b)	((a) < (b) ? (a) : (b))

struct mapfs *find_server();
struct filsys *already_mounted();
void add_mnttab();
void check_mnttab();
char *hasmntopt();
void free_knconf();
void nfsunmount();
void trymany();
CLIENT *get_cache_cl();
void add_cache_cl();
void del_cache_cl();
void done_cache_cl();

extern int trace;
extern int verbose;
extern struct cache_client cache_cl[];

static struct cache_host {
	char	host[MXHOSTNAMELEN+1];
	time_t	time;
};

static struct cache_host good[CACHE_HOST_SIZE];
static struct cache_host dead[CACHE_HOST_SIZE];

static long last_mnttab_time = 0;
static int cache_time = 60;	/* sec */

/*
 * Description:
 *	Checks in fs_q and tmp_q if this requested fs is already
 *	mounted.
 *		If yes in fs_q, return fs's link path.
 *		If yes in tmp_q, wait until do_unmount is done.
 *			If umounts succeed, go ahead and do mounts.
 *			If umounts do not succeed, go and check fs_q again.
 *		If no, call nfsmount to mount fs.
 * Call From:
 *	lookup
 * Entry/Exit State:
 *	no locks held on entry or exit.
 *	fsq_rwlock & tmpq_rwlock read-locked while checking fs_q.
 *	unmount_mutex while waiting for do_unmount finish.
 *	fsq_rwlock write-locked while updating it.
 */
nfsstat
do_mount(dir, me, rootfs, linkpath)
	struct autodir *dir;
	struct mapent *me;
	struct filsys **rootfs;
	char **linkpath;
{
	char mntpnt[MAXPATHLEN];
	char linkbuf[MAXPATHLEN];
	struct filsys *fs, *tfs;
	struct mapfs *mfs;
	struct mapent *m, *mapnext;
	nfsstat status = NFSERR_NOENT;
	char *prevhost;
	int imadeit;
	struct stat stbuf;
	struct q my_q;
	extern rwlock_t fsq_rwlock;
	extern rwlock_t tmpq_rwlock;
	extern int tmpq_status;
	extern int tmpq_waiting;
	extern mutex_t unmount_mutex;
	extern cond_t unmount_cond;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: do_mount: dir_name=%s\n",
			myid, dir->dir_name);

	*rootfs = NULL;
	*linkpath = "";
	prevhost = "";
	my_q.q_head = my_q.q_tail = NULL;

	/*
	 * For each map entry, check whether it is already mounted on.
	 */
recheck:
	RW_RDLOCK(&fsq_rwlock);
	RW_RDLOCK(&tmpq_rwlock);
	for (m = me ; m ; m = mapnext) {
		mapnext = m->map_next;

		(void) sprintf(mntpnt, "%s%s%s%s", tmpdir,
			       dir->dir_name, me->map_root, m->map_mntpnt);

		if (trace > 1)
			fprintf(stderr, "%d: do_mount: is '%s' mounted?\n",
				myid, mntpnt);

		/*
		 * Check if the mntpnt is on fs_q first.
		 * If yes, return its link path.
		 */
		for (fs = HEAD(struct filsys, fs_q); fs;
		     fs = NEXT(struct filsys, fs)) {
			if (strcmp(mntpnt, fs->fs_mntpnt) == 0) {
				mfs = me->map_fs;
				(void) sprintf(linkbuf,
					       "%s%s%s%s",
					       tmpdir,
					       dir->dir_name,
					       me->map_root,
					       mfs->mfs_subdir);
				if (trace > 1)
					fprintf(stderr,
						"%d: do_mount: renew link for %s\n",
						myid, linkbuf);

				*rootfs = fs->fs_rootfs;
				*linkpath = linkbuf;
				RW_UNLOCK(&tmpq_rwlock);
				RW_UNLOCK(&fsq_rwlock);
				return (NFS_OK);
			}
		}

		/*
		 * Check if the mntpnt is on tmp_q about to be
		 * unmounted.  If yes, wait until unmount is done,
		 * and check its status again.
		 */
		for (fs = HEAD(struct filsys, tmpq); fs;
		     fs = NEXT(struct filsys, fs)) {
			if (strcmp(mntpnt, fs->fs_mntpnt) == 0) {

				RW_UNLOCK(&tmpq_rwlock);
				RW_UNLOCK(&fsq_rwlock);

				MUTEX_LOCK(&unmount_mutex);
				while (tmpq_status == WORK) {
					tmpq_waiting++;
					cond_wait(&unmount_cond, &unmount_mutex);
					tmpq_waiting--;
				}
				if (tmpq_status == PASS) {
					MUTEX_UNLOCK(&unmount_mutex);
					goto doit;
				} else {
					MUTEX_UNLOCK(&unmount_mutex);
					goto recheck;
				}
			}
		}
	}
	RW_UNLOCK(&tmpq_rwlock);
	RW_UNLOCK(&fsq_rwlock);

	/*
	 * Go ahead and try to mount the requested directories.
	 */
doit:
	for (m = me ; m ; m = mapnext) {
		mapnext = m->map_next;
		
		(void) sprintf(mntpnt, "%s%s%s%s",
			       tmpdir, dir->dir_name,
			       me->map_root, m->map_mntpnt);

		tfs = NULL;
		mfs = find_server(m, &tfs, m == me, prevhost, dir);
		if (mfs == NULL)
			continue;
		
		/*
		 * It may be possible to return a symlink
		 * to an existing mount point without
		 * actually having to do a mount.
		 */
		if (me->map_next == NULL && *me->map_mntpnt == '\0') {

			/*
			 * Is it my host ?
			 */
			if (strcmp(mfs->mfs_host, self) == 0 ||
			    strcmp(mfs->mfs_host, "localhost") == 0) {
				struct stat statbuf;
				(void) strcpy(linkbuf, mfs->mfs_dir);
				(void) strcat(linkbuf, mfs->mfs_subdir);
				if (!check_hier(linkbuf) && 
				    stat(linkbuf, &statbuf) == 0) {
					*linkpath = linkbuf;
					if (trace > 1)
						fprintf(stderr,
							"%d: do_mount: it's on my host\n",
							myid);
					return (NFS_OK);
				} else {
					mfs->mfs_ignore = 1;
					prevhost = "";
					mapnext = m;
					continue;
				}
			}

			/*
			 * Is it an existing mount point?
			 * XXX Note: this is a bit risky - the
			 * mount may belong to another automount
			 * daemon - it could unmount it anytime and
			 * this symlink would then point to an empty
			 * or non-existent mount point.
			 *
			 * fsq_rwlock is read-locked at this point.
			 */
			if (tfs != NULL) {
				MUTEX_LOCK(&tfs->fs_mutex);

				syslog(LOG_ERR,
				       gettxt(":202", "%s: %s:%s is already mounted on %s"),
				       "do_mount", tfs->fs_host, tfs->fs_dir,
				       tfs->fs_mntpnt);

				(void) strcpy(linkbuf, tfs->fs_mntpnt);
				(void) strcat(linkbuf, mfs->mfs_subdir);
				*linkpath = linkbuf;
				*rootfs = tfs;
				tfs->fs_death = 2 * (time_now + max_link_time);
				MUTEX_UNLOCK(&tfs->fs_mutex);

				RW_UNLOCK(&fsq_rwlock);	/* locked in find_server */
				return (NFS_OK);
			}
		}

		if (tfs != NULL) {
			syslog(LOG_ERR,
			       "do_mount: WARNING: needed to unlock fsq_rwlock");
			RW_UNLOCK(&fsq_rwlock);	/* locked in find_server */
		}

		if (nomounts) {
			return (NFSERR_PERM);
		}

		 /* Create the mount point if necessary */
		imadeit = 0;
		if (stat(mntpnt, &stbuf) != 0) {
			if (mkdir_r(mntpnt) == 0) {
				imadeit = 1;
				if (stat(mntpnt, &stbuf) < 0) {
					syslog(LOG_ERR,
					       gettxt(":243",
						      "%s: cannot stat %s: %m"),
					       "do_mount", mntpnt);
					continue;
				}
			} else {
				syslog(LOG_ERR,
				       gettxt(":242",
					      "%s: cannot create %s: %m"),
				       "do_mount", mntpnt);
				continue;
			}
		}

		/*  Now do the mount */
		tfs = NULL;
		status = nfsmount(mfs->mfs_host, mfs->mfs_dir,
				  mntpnt, m->map_mntopts, &tfs);

		if (status == NFS_OK) {
			if (*rootfs == NULL) {
				(void) sprintf(linkbuf, "%s%s%s%s",
					       tmpdir, dir->dir_name,
					       me->map_root, mfs->mfs_subdir);
				*linkpath = linkbuf;
				*rootfs = tfs;
				tfs->fs_death = 2 * (time_now + max_link_time);
				if (trace > 1) {
					fprintf(stderr, "%d: do_mount: got rootfs=%s, linkpath=%s\n",
						myid, (*rootfs)->fs_mntpnt,
						*linkpath);
				}
			}
			tfs->fs_rootfs = *rootfs;
			tfs->fs_mntpntdev = stbuf.st_dev;
			if (stat(mntpnt, &stbuf) < 0) {
				syslog(LOG_ERR,
				       gettxt(":243", "%s: cannot stat %s: %m"),
				       "do_mount", mntpnt);
			} else {
				tfs->fs_mountdev = stbuf.st_dev;
			}

			INSQUE(my_q, tfs);
			prevhost = mfs->mfs_host;
		} else {
			if (imadeit)
				safe_rmdir(mntpnt);
			mfs->mfs_ignore = 1;
			prevhost = "";
			mapnext = m;
			continue;	/* try another server */
		}
	}

	if (*rootfs != NULL) {
		/*
		 * Everything went well.  Put in fs_q.  Add to mnttab.
		 */
		RW_WRLOCK(&fsq_rwlock);
		for (fs = TAIL(struct filsys, my_q); fs; fs = tfs) {
			tfs = PREV(struct filsys, fs);
			REMQUE(my_q, fs);
			INSQUE(fs_q, fs);
		}

		add_mnttab(*rootfs);
		RW_UNLOCK(&fsq_rwlock);

		return (NFS_OK);
	}

	return (status);
}


getsubnet_byaddr(ptr, subnet)
struct in_addr *ptr;
u_int *subnet;
{
	int  j;
	u_long i, netmask;
	u_char *bytes;
	u_int u[4];
	struct in_addr net;
	char key[128], *mask;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: getsubnet_byaddr: \n", myid);

	mask = NULL;
	i = ntohl(ptr->s_addr);
	bytes = (u_char *)(&net);
	if (IN_CLASSA(i)) {
		net.s_addr = htonl(i & IN_CLASSA_NET);
		sprintf(key, "%d.0.0.0", bytes[0]);
	} else 	if (IN_CLASSB(i)) {
		net.s_addr = htonl(i & IN_CLASSB_NET);
		sprintf(key, "%d.%d.0.0", bytes[0], bytes[1]);
	} else 	if (IN_CLASSC(i)) {
		net.s_addr = htonl(i & IN_CLASSC_NET);
		sprintf(key, "%d.%d.%d.0", bytes[0], bytes[1], bytes[2]);
	}
	if (getnetmask_byaddr(key, &mask) != 0) 
		return (-1);

	bytes = (u_char *) (&netmask);
	sscanf(mask, "%d.%d.%d.%d", &u[0], &u[1], &u[2], &u[3]);
	if (mask)
		free((char *) mask);
	for (j = 0; j < 4; j++)
		bytes[j] = u[j];
	netmask = ntohl(netmask);
	if (IN_CLASSA(i)) 
		*subnet = IN_CLASSA_HOST & netmask & i;
	else if (IN_CLASSB(i)) {
		*subnet = IN_CLASSB_HOST & netmask & i;
	} else if (IN_CLASSC(i)) 
		*subnet = IN_CLASSC_HOST & netmask & i;
	
	return (0);
}


/*
 * Description:
 *	Find hosts that are in my subnet.
 * Call From:
 *	get_mysubnet_servers
 * Entry/Exit:
 *	No locks need to be held on entry or exit.
 *	During, subnet_mutex is held during the whole routine.
 */
u_int *
get_myhosts_subnets()
{
	static int my_subnet_cnt = 0;
	static u_int my_subnets[MAXSUBNETS + 1];
	struct hostent *myhost_ptr;
	struct in_addr *ptr;
	extern mutex_t subnet_mutex;
	thread_t myid = thr_self();

	MUTEX_LOCK(&subnet_mutex);
	if (trace > 1)
		fprintf(stderr, "%d: get_myhosts_subnets: my_subnet_cnt=%d\n",
			myid, my_subnet_cnt);

	if (my_subnet_cnt)  {
		MUTEX_UNLOCK(&subnet_mutex);
		return (my_subnets);
	}
        myhost_ptr = (struct hostent *)gethostbyname(self);
	while (myhost_ptr && 
	       (ptr = (struct in_addr *) *myhost_ptr->h_addr_list++) != NULL) {
		if (my_subnet_cnt < MAXSUBNETS) {
			if (getsubnet_byaddr(ptr,
					     &my_subnets[my_subnet_cnt]) == 0)
				my_subnet_cnt++;
		}
	}
	my_subnets[my_subnet_cnt] = (u_int ) NULL;
	MUTEX_UNLOCK(&subnet_mutex);
	return (my_subnets);

}


subnet_matches(mysubnets, hs)
u_int *mysubnets;
struct hostent *hs;
{
	struct in_addr *ptr;
	u_int subnet;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: subnet_matches: mysubnets=%d\n",
			myid, *mysubnets);

	while ((ptr = (struct in_addr *) *hs->h_addr_list++) != NULL) {
		if (getsubnet_byaddr(ptr, &subnet) == 0)
			while (*mysubnets != (u_int) NULL)
				if (*mysubnets++ == subnet) 
					return (1);
	}

	return (0);
}


get_mysubnet_servers(mfs_head, hosts_ptr)
struct mapfs *mfs_head;
struct host_names *hosts_ptr;
{
	NCONF_HANDLE *nc;
	struct netconfig *nconf;
	u_int *mysubnets = (u_int *) NULL;
	struct mapfs *mfs;
	struct hostent *hs;	
	int i, cnt;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: get_mysubnet_servers: \n", myid);

	cnt = 0;
	nc = setnetconfig();
	if (nc == NULL)
		return (0);
	while (nconf = getnetconfig(nc)) {
		if ((nconf->nc_semantics == NC_TPI_CLTS) &&
		    (strcmp(nconf->nc_protofmly, NC_INET) == 0) &&
		    (strcmp(nconf->nc_proto, NC_UDP) == 0)) {
			mysubnets = get_myhosts_subnets();
			for (mfs = mfs_head, i = 0;
			     mfs && (i < MAXHOSTS); i++, mfs = mfs->mfs_next) {
				if (mfs->mfs_ignore)
					continue;
				hs = (struct hostent *) 
					gethostbyname(mfs->mfs_host);
				if (hs == NULL) 
					continue;
				if (subnet_matches(mysubnets, hs)) {
					hosts_ptr->host = mfs->mfs_host;
					hosts_ptr->penalty = mfs->mfs_penalty;
					hosts_ptr++;
					cnt++;
				}
			}
			hosts_ptr->host = (char *) NULL; /* terminate list */
		}
	}
					
	if (nc)
		endnetconfig(nc);
	return (cnt);
}


get_mynet_servers(mfs_head, hosts_ptr)
struct mapfs *mfs_head;
struct host_names *hosts_ptr;
{
	NCONF_HANDLE *nc;
	struct netconfig *nconf;
	struct mapfs *mfs;
	struct hostent *hs;	
	int mynet = 0;
	int i, cnt;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: get_mynet_servers: \n", myid);

	cnt = 0;
	nc = setnetconfig();
	if (nc == NULL)
		return (0);
	while (nconf = getnetconfig(nc)) {
		if ((nconf->nc_semantics == NC_TPI_CLTS) &&
		    (strcmp(nconf->nc_protofmly, NC_INET) == 0) &&
		    (strcmp(nconf->nc_proto, NC_UDP) == 0)) {
			hs = gethostbyname(self);
			mynet = inet_netof(*((struct in_addr *) *hs->h_addr_list));
 			for (mfs = mfs_head, i = 0;
			     mfs && (i < MAXHOSTS); i++,  mfs = mfs->mfs_next) {
				if (mfs->mfs_ignore)
					continue;
				hs = (struct hostent *) 
					gethostbyname(mfs->mfs_host);
				if (hs == NULL) 
					continue;
				if (mynet == inet_netof(*((struct in_addr *)
							  *hs->h_addr_list))) {
					hosts_ptr->host = mfs->mfs_host;
					hosts_ptr->penalty = mfs->mfs_penalty;
					hosts_ptr++;
					cnt++;
					if (trace > 2)
						fprintf(stderr, "%d: get_mynet_servers: cnt=%d, mynet=%d, host=%s\n",
							myid, cnt, mynet,
							mfs->mfs_host);
				}
			}
			hosts_ptr->host = (char *) NULL; /* terminate lilst */
		}
	}
			
	if (nc)
		endnetconfig(nc);
	return (cnt);
}


/*
 * Description:
 *	Finds the best server for this map entry.
 * Call From:
 *	do_mount
 * Entry/Exit:
 *	On entry, no locks need to be held.
 *	On exit, if a fs is founded for this map entry,
 *		then fsq_rwlock is read locked.
 */
struct mapfs *
find_server(me, fsp, rootmount, preferred, dir)
	struct mapent *me;
	struct filsys **fsp;
	int rootmount;
	char *preferred;
	struct autodir *dir;
{
	int entrycount;
	struct mapfs *mfs, *mfs_one;
	struct host_names host_array[MAXHOSTS + 1], best_host;
	/* 
	 * last entry reserved for terminating list
	 * in case there are MAXHOST servers
	 */
	int subnet_cnt, net_cnt;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: find_server: (%s, %s)\n",
			myid, me->map_mntpnt, me->map_fs->mfs_host);

	/*
	 * get addresses & see if any are myself
	 * or were mounted from previously in a
	 * hierarchical mount.
	 */
	entrycount =  subnet_cnt = net_cnt = 0;
	mfs_one = NULL;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		if (mfs->mfs_ignore)
			continue;
		mfs_one = mfs;
		if (strcmp(mfs->mfs_host, "localhost") == 0 ||
		    strcmp(mfs->mfs_host, self) == 0 ||
		    strcmp(mfs->mfs_host, preferred) == 0)
			return (mfs);
		entrycount++;
	}
	if (entrycount == 0)
		return (NULL);

	/* see if any already mounted */
	if (rootmount) {
		for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
			if (mfs->mfs_ignore)
				continue;
		   	 if (*fsp = already_mounted(mfs->mfs_host, mfs->mfs_dir,
						    me->map_mntopts)) {
				 return (mfs);
			 }
	    	}
	}
	if (entrycount == 1)
		return (mfs_one);

	subnet_cnt = get_mysubnet_servers(me->map_fs, host_array);

	if (subnet_cnt > 0) {
		trymany(host_array, mount_timeout / 2, dir, &best_host);
		if (best_host.host) /* got one */
			for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
				if (best_host.host == mfs->mfs_host) {
					return (mfs);
				}
	}
	if (subnet_cnt == entrycount)
		return (NULL);

	net_cnt =  get_mynet_servers(me->map_fs, host_array);

	if (net_cnt > subnet_cnt) {
		trymany(host_array, mount_timeout / 2, dir, &best_host);
		if (best_host.host) /* got one */
			for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
				if (best_host.host == mfs->mfs_host) {
					return (mfs);
				}
	}
	if (entrycount > net_cnt) {
		int i;
		for (i = 0, mfs = me->map_fs;
		     mfs && (i < MAXHOSTS); i++, mfs = mfs->mfs_next) {
			if (mfs->mfs_ignore)
				continue;
			host_array[i].host = mfs->mfs_host;
			host_array[i].penalty = mfs->mfs_penalty;
		}
		host_array[i].host = (char *) NULL; /* terminate the list */
		trymany(host_array, mount_timeout / 2, dir, &best_host);
		if (best_host.host) /* got one */
			for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
				if (best_host.host == mfs->mfs_host) {
					return (mfs);
				}
	}
	return (NULL);
}

/*
 * Description:
 *	Read the mnttab and correlate with internal fs information.
 * 	 Sets all fs structures to be not present and reads mnttab.
 *	 If an NFS mnttab corresponds to a fs, make present 1
 *              if there is no fs in fs_q for a NFS mnttab,
 *			check if it is in tmp_q,
 *			  if not, create it, make present 1
 *	 Any fs not marked present in fs_q will be removed.
 * 
 */
void
read_mnttab()
{
	struct stat stbuf;
	struct filsys *fs, *fsnext;
	FILE *mnttab;
	struct mnttab mt;
	int found, c, tmpq_found;
	char *tmphost, *tmppath, *p, tmpc;
	int r;
	extern rwlock_t fsq_rwlock;
	extern rwlock_t tmpq_rwlock;
	int semfile;
	struct q remq;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: read_mnttab:\n", myid);

	/* reset the present flags */
	RW_WRLOCK(&fsq_rwlock);
	RW_RDLOCK(&tmpq_rwlock);
	for (fs = HEAD(struct filsys, fs_q); fs;
	     fs = NEXT(struct filsys, fs)) {
		fs->fs_present = 0;
	}

	/* now see what's been mounted */
	if (lockmntfile(&semfile) < 0) {
		RW_UNLOCK(&tmpq_rwlock);
		RW_UNLOCK(&fsq_rwlock);
		return;
	}

	mnttab = fopen(MNTTAB, "r+");
	if (mnttab == NULL) {
		close(semfile);
		syslog(LOG_ERR, gettxt(":215", "%s: cannot open %s: %m"),
		       "read_mnttab", MNTTAB);
		RW_UNLOCK(&tmpq_rwlock);
		RW_UNLOCK(&fsq_rwlock);
		return;
	}

	if (lockf(fileno(mnttab), F_LOCK, 0L) < 0) {
		fclose(mnttab);
		close(semfile);
		syslog(LOG_ERR, gettxt(":216", "%s: cannot lock %s: %m"),
		       "read_mnttab", MNTTAB);
		RW_UNLOCK(&tmpq_rwlock);
		RW_UNLOCK(&fsq_rwlock);
		return;
	}

	for (c = 1 ;; c++) {
		if (r = getmntent(mnttab, &mt)) {
			if (r < 0)
				break;	/* EOF */
			syslog(LOG_ERR,
			       gettxt(":200", "%s: line %d: bad entry"),
			       MNTTAB, c);
			continue;
		}

		if (strcmp(mt.mnt_fstype, MNTTYPE_NFS) != 0)
			continue;

		p = strchr(mt.mnt_special, ':');
		if (p == NULL)
			continue;
		tmpc = *p;
		*p = '\0';
		tmphost = mt.mnt_special;
		tmppath = p+1;
		if (tmppath[0] != '/')
			continue;

		found = 0;
		for (fs = HEAD(struct filsys, fs_q); fs;
		     fs = NEXT(struct filsys, fs)) {
			if (strcmp(mt.mnt_mountp, fs->fs_mntpnt) == 0 &&
			    strcmp(tmphost, fs->fs_host) == 0 &&
			    strcmp(tmppath, fs->fs_dir) == 0 &&
			    (fs->fs_mine ||
			     strcmp(mt.mnt_mntopts, fs->fs_opts) == 0)) {
				fs->fs_present = 1;
			  	found++;
				break;
			}
		}

		if (!found) {
			/*
			 * If not found in fs_q, it may be on tmp_q.
			 * If on tmp_q, do not add it to fs_q again.
			 */
			tmpq_found = 0;
			for (fs = HEAD(struct filsys, tmpq); fs;
			     fs = NEXT(struct filsys, fs)) {
				if (strcmp(mt.mnt_mountp, fs->fs_mntpnt) == 0 &&
				    strcmp(tmphost, fs->fs_host) == 0 &&
				    strcmp(tmppath, fs->fs_dir) == 0 &&
				    (fs->fs_mine ||
				     strcmp(mt.mnt_mntopts, fs->fs_opts) == 0)) {
					tmpq_found++;
				}
			}

			if (!tmpq_found) {
				fs = alloc_fs(tmphost, tmppath,
					      mt.mnt_mountp, mt.mnt_mntopts);
				if (fs == NULL)
					break;
				fs->fs_present = 1;
				INSQUE(fs_q, fs);
			} else {
				if (trace > 1)
					fprintf(stderr,
						"%d: read_mnttab: WARNING: found %s in tmpq and /etc/mnttab\n",
						myid, mt.mnt_mountp);
			}
		}
		*p = tmpc;
	}
	fclose(mnttab);
	close(semfile);

	/* remove non-present fs's from fs_q */
	remq.q_head = remq.q_tail = NULL;
	for (fs = HEAD(struct filsys, fs_q); fs; fs = fsnext) {
		fsnext = NEXT(struct filsys, fs);
		if (!fs->fs_present) {
			if (trace > 1)
				fprintf(stderr, "%d: read_mnttab: fs not present (%s %s:%s)\n",
					myid, fs->fs_mntpnt, fs->fs_host, fs->fs_dir);
			REMQUE(fs_q, fs);
			INSQUE(remq, fs);
		}
	}

	RW_UNLOCK(&tmpq_rwlock);
	RW_UNLOCK(&fsq_rwlock);

	/*
	 * free fs's that are no longer present, do not want
	 * tmpq_rwlock and fsq_rwlock locked while doing this.
	 */
	for (fs = HEAD(struct filsys, remq); fs; fs = fsnext) {
		fsnext = NEXT(struct filsys, fs);
		if (fs->fs_mine) {
			syslog(LOG_ERR,
			       gettxt(":201", "%s: WARNING: %s:%s is no longer mounted"),
			       "read_mnttab", fs->fs_host, fs->fs_dir);
		}
		flush_links(fs);
		REMQUE(remq, fs);
		free_filsys(fs);
	}
		
	if (stat(MNTTAB, &stbuf) != 0)
		syslog(LOG_ERR, gettxt(":243", "%s: cannot stat %s: %m"),
		       "read_mnttab", MNTTAB);
	else
		last_mnttab_time = stbuf.st_mtime;
}

/*
 * Description:
 *	If mnttab has changed, update internal fs info.
 * Call From:
 *	main, already_mounted, do_timeouts
 * Entry/Exit:
 *	No locks held on entry, during, or exit.
 */
void
check_mnttab()
{
	struct stat stbuf;
	long mintime;

	if (stat(MNTTAB, &stbuf) < 0) {
		syslog(LOG_ERR, gettxt(":243", "%s: cannot stat %s: %m"),
		       "read_mnttab", MNTTAB);
		return;
	}
	mintime = MIN(stbuf.st_mtime, time(0) - 2);
	if (mintime > last_mnttab_time)
		read_mnttab();
}

/*
 * Description:
 *	Search the mount table to see if the given file system 
 *	is already mounted.
 * Call From:
 *	find_server
 * Entry/Exit:
 *	On entry, no locks need to be held.
 *	During, fsq_rwlock is read locked.
 *	On exit, fsq_rwlock is read locked if a fs is found.
 */
struct filsys *
already_mounted(host, fsname, opts)
	char *host;
	char *fsname;
	char *opts;
{
	struct filsys *fs;
	struct mnttab m1, m2;
	int has1, has2;
	struct autodir *dir;
	int mydir;
	extern rwlock_t fsq_rwlock;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: already_mounted: host=%s, fsname=%s\n",
			myid, host, fsname);

	check_mnttab();
	m1.mnt_mntopts = opts;

	RW_RDLOCK(&fsq_rwlock);
	for (fs = HEAD(struct filsys, fs_q); fs; 
	     fs = NEXT(struct filsys, fs)) {
		if (strcmp(fsname, fs->fs_dir) != 0)
			continue;
		if (strcmp(host, fs->fs_host) != 0)
			continue;

		/*
		 * Check it's not on one of my mount points.
		 * I might be mounted on top of a previous 
		 * mount of the same file system.
		 */
		mydir = 0;
		for (dir = HEAD(struct autodir, dir_q); dir;
		     dir = NEXT(struct autodir, dir)) {
			if ((strcmp(dir->dir_name, fs->fs_mntpnt) == 0) ||
			    (subdir(dir->dir_name, fs->fs_mntpnt))) {
				mydir = 1;
				syslog(LOG_ERR,
				       gettxt(":202",
					      "%s: %s:%s is already mounted on %s"),
				       "already_mounted",
				       host, fsname, fs->fs_mntpnt);
				break;
			}
		}
		if (mydir)
			continue;

		m2.mnt_mntopts = fs->fs_opts;
		has1 = hasmntopt(&m1, MNTOPT_RO) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_RO) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_NOSUID) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_NOSUID) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_SOFT) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_SOFT) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_INTR) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_INTR) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_NOINTR) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_NOINTR) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_SECURE) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_SECURE) != NULL;
		if (has1 != has2)
			continue;

		/*
		 * fsq_rwlock has to remain read-locked so that
		 * fs does not get removed.
		 */
		return (fs);
	}
	RW_UNLOCK(&fsq_rwlock);
	return (0);
}

/*
 * Description:
 *	Send server a courtesy umount RPC message.
 * Call from:
 *	do_unmount
 * Entry/Exit:
 *	No locks held on entry, during, or exit.
 */
void
nfsunmount(fs)
	struct filsys *fs;
{
	struct timeval timeout;
	CLIENT *cl;
	enum clnt_stat rpc_stat;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: nfsunmount: fs_mntpnt=%s\n",
			myid, fs->fs_mntpnt);

	cl = get_cache_cl(fs->fs_host);
	if (cl == NULL)
		return;

	timeout.tv_sec = 25;
	timeout.tv_usec = 0;
	rpc_stat = clnt_call(cl, MOUNTPROC_UMNT, xdr_path, (caddr_t)&fs->fs_dir,
			     xdr_void, (char *)NULL, timeout);

	if (rpc_stat != RPC_SUCCESS)
		syslog(LOG_ERR,
		       gettxt(":218", "%s: %s server not responding for %s"),
		       "nfsunmount", fs->fs_host, clnt_sperror(cl, "clnt_call"));

	done_cache_cl(fs->fs_host, cl);
}


/*
 * Description:
 *	Pings the host to check if it is up and running.
 *	Caches good and dead hosts for 5 minutes.
 * Call From:
 *	pathok, nfsmount, remount, getmapent_hosts
 * Entry/Exit:
 *	No locks held on entry or exit.
 *	During, host_mutex while checking & updating host caches.
 */
enum clnt_stat 
pingmount(hostname)
	char *hostname;
{
	CLIENT *cl = NULL;
	struct timeval tottime;
	enum clnt_stat clnt_stat;
	int i;
	extern int pingtime;
	extern mutex_t host_mutex;
	thread_t myid = thr_self();

	/*
	 * Check cached hosts first.
	 */
	MUTEX_LOCK(&host_mutex);
	for (i = 0; i < CACHE_HOST_SIZE; i++) {
		if ((! strcmp(good[i].host, hostname)) && 
		    good[i].time > time_now) {
	        	MUTEX_UNLOCK(&host_mutex);
			return (RPC_SUCCESS);
	        }
		if ((! strcmp(dead[i].host, hostname)) &&
		    dead[i].time > time_now) {
			MUTEX_UNLOCK(&host_mutex);
			return (RPC_TIMEDOUT);
		}
	}		
	MUTEX_UNLOCK(&host_mutex);

	if (trace > 1)
		fprintf(stderr, "%d: pingmount: pinging %s ...\n",
			myid, hostname);

	cl = clnt_create(hostname, NFS_PROGRAM, NFS_VERSION, "datagram_v");
	if (cl == NULL) {
		syslog(LOG_ERR,
		       gettxt(":218", "%s: %s server not responding for %s"),
		       "pingmount", hostname, clnt_spcreateerror("clnt_create"));
		clnt_stat = RPC_TIMEDOUT;
	} else {
		tottime.tv_sec = pingtime;
		tottime.tv_usec = 0;
		clnt_stat = clnt_call(cl, NULLPROC, xdr_void, 0, 
				      xdr_void, 0, tottime);
		clnt_destroy(cl);
	}

	if (trace > 1)
		fprintf(stderr, "%d: pingmount: pinged %s %s\n",
			myid, hostname,
			(clnt_stat == RPC_SUCCESS ? "OK" : "NO RESPONSE"));

	/*
	 * Update cache hosts.
	 */
	MUTEX_LOCK(&host_mutex);
	if (clnt_stat == RPC_SUCCESS) {
		for (i = 0; i < CACHE_HOST_SIZE; i++) {
			if (good[i].time <= time_now) {
				(void) strcpy(good[i].host, hostname);
				good[i].time = time_now + cache_time;
				break;
			} else if (!strcmp(good[i].host, hostname)) {
				good[i].time = time_now + cache_time;
				break;
			}
	        }

		if (trace > 1)  { 
			if (i == CACHE_HOST_SIZE)
				fprintf(stderr,
					"%d: pingmount: good host cache full\n",
					myid);
			else
				fprintf(stderr,
					"%d: pingmount: good[%d] host=%s, time=%d\n",
					myid, i, good[i].host, good[i].time);
		}
	} else {
		for (i = 0; i < CACHE_HOST_SIZE; i++) {
			if (dead[i].time <= time_now) {
				(void) strcpy(dead[i].host, hostname);
				dead[i].time = time_now + cache_time;
				break;
			} else if (!strcmp(dead[i].host, hostname)) {
				dead[i].time = time_now + cache_time;
				break;
			}
	        }

		if (trace > 1) { 
			if (i == CACHE_HOST_SIZE)
				fprintf(stderr,
					"%d: pingmount: dead host cache full\n",
					myid);
			else
				fprintf(stderr,
					"%d: pingmount: dead[%d] host=%s, time=%d\n",
					myid, i, dead[i].host, dead[i].time);
		} 
	}
	MUTEX_UNLOCK(&host_mutex);

	return (clnt_stat);
}

/*
 * ping a bunch of hosts at once and find out who
 * responds first
 */
void
trymany(host_array, timeout, dir, best_host)
     struct host_names *host_array, *best_host;
     int timeout;
     struct autodir *dir;
{
	extern enum clnt_stat nfs_cast();
	enum clnt_stat clnt_stat;
	thread_t myid = thr_self();

	if (trace > 1) {
		struct host_names *h;

		fprintf(stderr, "%d: trymany: hosts are ", myid);
		for (h = host_array; h->host; h++)
			fprintf(stderr, "%s ", h->host);
		fprintf(stderr, "\n");
	}
	
	best_host->host = NULL;

	clnt_stat = nfs_cast(host_array, best_host, timeout);

	if (trace > 1)
		fprintf(stderr, "%d: trymany: got %s\n",
			myid, (int) clnt_stat ? "no response" : best_host->host);

	if (clnt_stat)
		syslog(LOG_ERR, gettxt(":205",
				       "%s: servers are not responding: %s"),
		       dir->dir_name, clnt_sperrno(clnt_stat));
}

/*
 * Return 1 if the path s2 is a subdirectory of s1.
 */
subdir(s1, s2)
	char *s1, *s2;
{
    	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: subdir: s1=%s, s2=%s\n", myid, s1, s2);

	while (*s1 == *s2)
		s1++, s2++;
	return (*s1 == '\0' && *s2 == '/');
}

/*
 * Description:
 *	Sets all arguments and actually mounts directory.
 * Call From:
 *	do_mount, remount (this one is questionable because I cannot see
 *			   how it is possible)
 * Entry/Exit:
 *	No locks held upon entry, during, or exit.
 */
nfsstat
nfsmount(host, dir, mntpnt, opts, fsp)
	char *host, *dir, *mntpnt, *opts;
	struct filsys **fsp;
{
	struct filsys *fs;
	char netname[MAXNETNAMELEN+1];
	char remname[MAXPATHLEN];
	struct mnttab m;
	struct nfs_args args;
	int flags;
	struct fhstatus fhs;
	struct timeval timeout;
	CLIENT *cl = NULL;
	enum clnt_stat rpc_stat;
	enum clnt_stat pingmount();
	nfsstat status;
	struct stat stbuf;
	struct netbuf *get_addr();
	struct netconfig *nconf;
	struct knetconfig *get_knconf();
	void netbuf_free();
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: nfsmount: (%s, %s, %s)\n",
			myid, host, dir, mntpnt);

	/* Make sure mountpoint is safe to mount on */
	if (lstat(mntpnt, &stbuf) < 0) {
		syslog(LOG_ERR, gettxt(":243", "%s: cannot stat %s: %m"),
		       "nfsmount", mntpnt);
		return (NFSERR_NOENT);
	}

	/* A symbolic link could be dangerous e.g. -> /usr */
	if ((stbuf.st_mode & S_IFMT) == S_IFLNK) {
		if (realpath(mntpnt, remname) == NULL) {
			syslog(LOG_ERR,
			       gettxt(":94", "%s: %s failed for %s: %m"),
			       "nfsmount", "realpath", mntpnt);
			return (NFSERR_NOENT);
		}
		if (!subdir(tmpdir, remname)) {
			syslog(LOG_ERR,
			       gettxt(":206",
				      "%s: %s:%s -> %s is a dangerous symbolic link"),
			       "nfsmount", host, dir, remname);
			return (NFSERR_NOENT);
		}
	}
	
	if (pingmount(host) != RPC_SUCCESS) {
		syslog(LOG_ERR,
		       gettxt(":207", "%s: %s is not responding"),
		       "nfsmount", host);
		return (NFSERR_NOENT);
	}

	cl = get_cache_cl(host);
	if (cl == NULL)
		return (NFSERR_NOENT);

	(void) sprintf(remname, "%s:%s", host, dir);

	/*
	 * Get fhandle of remote path from server's mountd
	 */
	timeout.tv_sec = mount_timeout;
	timeout.tv_usec = 0;
	rpc_stat = clnt_call(cl, MOUNTPROC_MNT, xdr_path, (caddr_t)&dir,
			     xdr_fhstatus, (caddr_t)&fhs, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		syslog(LOG_ERR,
		       gettxt(":218", "%s: %s server not responding for %s"),
		       "nfsmount", remname, clnt_sperror(cl, "clnt_call"));
		del_cache_cl(host, cl);
		return (NFSERR_NOENT);
	}

	if (errno = fhs.fhs_status)  {
		if (verbose)
			syslog(LOG_ERR, "nfsmount: %s: %m", remname);
                if (errno == EACCES) {
			status = NFSERR_ACCES;
                } else {
			status = NFSERR_IO;
                }
		done_cache_cl(host, cl);
                return (status);
        }        

	done_cache_cl(host, cl);

	/*
	 * set mount args
	 */
	memset(&args, 0, sizeof(args));
	args.fh = (caddr_t)&fhs.fhs_fh;
	args.flags = NFSMNT_INT;	/* default is "intr" */
	args.hostname = host;
	args.flags |= NFSMNT_HOSTNAME;

	args.addr = get_addr(host, NFS_PROGRAM, NFS_VERSION, &nconf);
	if (args.addr == NULL) {
		syslog(LOG_ERR, gettxt(":208", "%s: no NFS service on %s"),
		       "nfsmount", host);
		return (NFSERR_NOENT);
	}

	args.flags |= NFSMNT_KNCONF;
	args.knconf = get_knconf(nconf);
	if (args.knconf == NULL) {
		netbuf_free(args.addr);
		return (NFSERR_NOSPC);
	}

	m.mnt_mntopts = opts;

	if (hasmntopt(&m, MNTOPT_SOFT) != NULL) {
		args.flags |= NFSMNT_SOFT;
	}
	if (hasmntopt(&m, MNTOPT_NOINTR) != NULL) {
		args.flags &= ~(NFSMNT_INT);
	}
	if (hasmntopt(&m, MNTOPT_NOAC) != NULL) {
		args.flags |= NFSMNT_NOAC;
	}
	if (hasmntopt(&m, MNTOPT_NOCTO) != NULL) {
		args.flags |= NFSMNT_NOCTO;
	}
	if (hasmntopt(&m, MNTOPT_SECURE) != NULL) {
		args.flags |= NFSMNT_SECURE;
	}
	if (args.flags & NFSMNT_SECURE) {
		/* NFSMNT_SECURE 
		 * XXX: need to support other netnames
		 * outside domain and not always just use
		 * the default conversion.
		 */
		if (!host2netname(netname, host, NULL)) {
			netbuf_free(args.addr);
			free_knconf(args.knconf);
			/* really unknown host */
			return (NFSERR_NOENT);
		}
                args.netname = netname;
		args.syncaddr = get_addr(host, RPCBPROG, RPCBVERS, &nconf);
		if (args.syncaddr) {
			args.flags |= NFSMNT_RPCTIMESYNC;
		} else {
			/*
			 * If it's a UDP transport, use the time service.
			 */
			if (strcmp(nconf->nc_protofmly, NC_INET) == 0 &&
			    strcmp(nconf->nc_proto, NC_UDP) == 0) {
				struct nd_hostserv hs;
				struct nd_addrlist *retaddrs;

				hs.h_host = host;
				hs.h_serv = "rpcbind";
				if (netdir_getbyname(nconf, &hs, &retaddrs)
				    != ND_OK) {
					netbuf_free(args.addr);
					free_knconf(args.knconf);
					syslog(LOG_ERR,
					       gettxt(":209", "%s: no time service for %s"),
					       "nfsmount1", host);
					return (NFSERR_IO);
				}
				args.syncaddr = retaddrs->n_addrs;
				((struct sockaddr_in *) args.syncaddr->buf)->sin_port
				   = IPPORT_TIMESERVER;
			} else {
				netbuf_free(args.addr);
				free_knconf(args.knconf);
				syslog(LOG_ERR,
				       gettxt(":209", "%s: no time service for %s"),
				       "nfsmount2", host);
				return (NFSERR_IO);
			}
		}
	} /* end of secure stuff */

	if (hasmntopt(&m, MNTOPT_GRPID) != NULL) {
		args.flags |= NFSMNT_GRPID;
	}
	if (args.rsize = nopt(&m, MNTOPT_RSIZE)) {
		args.flags |= NFSMNT_RSIZE;
	}
	if (args.wsize = nopt(&m, MNTOPT_WSIZE)) {
		args.flags |= NFSMNT_WSIZE;
	}
	if (args.timeo = nopt(&m, MNTOPT_TIMEO)) {
		args.flags |= NFSMNT_TIMEO;
	}
	if (args.retrans = nopt(&m, MNTOPT_RETRANS)) {
		args.flags |= NFSMNT_RETRANS;
	}
	if (args.acregmax = nopt(&m, MNTOPT_ACTIMEO)) {
		args.flags |= NFSMNT_ACREGMAX;
		args.flags |= NFSMNT_ACDIRMAX;
		args.flags |= NFSMNT_ACDIRMIN;
		args.flags |= NFSMNT_ACREGMIN;
		args.acdirmin = args.acregmin = args.acdirmax
			= args.acregmax;
	} else {
		if (args.acregmin = nopt(&m, MNTOPT_ACREGMIN)) {
			args.flags |= NFSMNT_ACREGMIN;
		}
		if (args.acregmax = nopt(&m, MNTOPT_ACREGMAX)) {
			args.flags |= NFSMNT_ACREGMAX;
		}
		if (args.acdirmin = nopt(&m, MNTOPT_ACDIRMIN)) {
			args.flags |= NFSMNT_ACDIRMIN;
		}
		if (args.acdirmax = nopt(&m, MNTOPT_ACDIRMAX)) {
			args.flags |= NFSMNT_ACDIRMAX;
		}
	}

	flags = 0;
	flags |= (hasmntopt(&m, MNTOPT_RO) == NULL) ? 0 : MS_RDONLY;
	flags |= (hasmntopt(&m, MNTOPT_NOSUID) == NULL) ? 0 : MS_NOSUID;

	if (trace > 1)
		fprintf(stderr, "%d: mount %s %s (%s) ...\n",
			myid, remname, mntpnt, opts);

	if (mount("", mntpnt, flags | MS_DATA, MNTTYPE_NFS,
		  &args, sizeof (args)) < 0) {
		syslog(LOG_ERR,
		       gettxt(":210", "%s: mount of %s on %s: %m"),
		       "nfsmount", remname, mntpnt);

		if (trace > 1)
			fprintf(stderr, "%d: mount %s FAILED\n", myid, remname);

		netbuf_free(args.addr);
		netbuf_free(args.syncaddr);
		free_knconf(args.knconf);
		return (NFSERR_IO);
	}

	if (trace > 1)
		fprintf(stderr, "%d: mount %s OK\n", myid, remname);

	/*
	 * No locks for fs here because
	 *  it is either in tmp_q (call from remount) or 
	 *  it just got allocated (call from do_mount).
	 * Either way, no other thread is accessing this fs.
	 */
	if (*fsp)
		fs = *fsp;
	else {
		fs = alloc_fs(host, dir, mntpnt, opts);
		if (fs == NULL) {
			netbuf_free(args.addr);
			netbuf_free(args.syncaddr);
			free_knconf(args.knconf);
			return (NFSERR_NOSPC);
		}
	}

	fs->fs_type = MNTTYPE_NFS;
	fs->fs_mine = 1;
	fs->fs_nfsargs = args;
	fs->fs_nfsargs.hostname = fs->fs_host;
	fs->fs_nfsargs.fh = (caddr_t)&fs->fs_rootfh;
	memcpy(&fs->fs_rootfh, &fhs.fhs_fh, sizeof fs->fs_rootfh);
	fs->fs_mflags = flags;

	*fsp = fs;

	return (NFS_OK);
}

struct knetconfig *
get_knconf(nconf)
	struct netconfig *nconf;
{
	struct stat stbuf;
	struct knetconfig *k;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: get_knconf: \n", myid);

	if (stat(nconf->nc_device, &stbuf) < 0) {
		syslog(LOG_ERR, gettxt(":243", "%s: cannot stat %s: %m"),
		       "get_knconf", nconf->nc_device);
		return (NULL);
	}
	k = (struct knetconfig *) malloc(sizeof(*k));
	if (k == NULL)
		goto nomem;
	k->knc_semantics = nconf->nc_semantics;
	k->knc_protofmly = strdup(nconf->nc_protofmly);
	if (k->knc_protofmly == NULL)
		goto nomem;
	k->knc_proto = strdup(nconf->nc_proto);
	if (k->knc_proto == NULL)
		goto nomem;
	k->knc_rdev = stbuf.st_rdev;

	return (k);

nomem:
	syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "get_knconf");
	free_knconf(k);
	return (NULL);
}

void
free_knconf(k)
	struct knetconfig *k;
{
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: free_knconf: \n", myid);

	if (k == NULL)
		return;
	if (k->knc_protofmly)
		free(k->knc_protofmly);
	if (k->knc_proto)
		free(k->knc_proto);
	free(k);
}

void
netbuf_free(nb)
	struct netbuf *nb;
{
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: netbuf_free: \n", myid);

	if (nb == NULL)
		return;
	if (nb->buf)
		free(nb->buf);
	free(nb);
}

/*
 * Description:
 *	Get the network address for the service identified by "prog"
 * 	and "vers" on "hostname".  The netconfig address is returned
 * 	in the value of "nconfp".
 * 	Use the same netconfig entry unless there is an error.
 * Call From:
 * 	nfsmount
 * Entry/Exit:
 *	No locks need to be held on entry or exit.
 *	During, nconf_mutex is held while using the static netconfig
 *		and NCONF_HANDLE entries.
 */

struct netbuf *
get_addr(hostname, prog, vers, nconfp)
	char *hostname;
	int prog, vers;
	struct netconfig **nconfp;
{
	static struct netconfig *nconf = NULL;
	static NCONF_HANDLE *nc = NULL;
	struct netbuf *nb = NULL;
	struct t_bind *tbind = NULL;
	int fd = -1;
	int retries = 0;
	extern mutex_t nconf_mutex;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: get_addr: hostname=%s, prog=%d\n",
			myid, hostname, prog);

	MUTEX_LOCK(&nconf_mutex);
	if (nconf == NULL) { 
		retries = 1;
retry:
		if (retries == 1) {
			if (nc)
				endnetconfig(nc);

			nc = setnetconfig();
			if (nc == NULL) {
				MUTEX_UNLOCK(&nconf_mutex);
				goto done;
			}

			if (trace > 1)
				fprintf(stderr,
					"%d: get_addr: new setnetconfig\n",
					myid);
		}

		/*
		 * If the port number is specified then UDP is needed.
		 * Otherwise any connectionless transport will do.
		 */
		while (nconf = getnetconfig(nc)) {
			if ((nconf->nc_flag & NC_VISIBLE) &&
			    (nconf->nc_semantics == NC_TPI_CLTS))
				break;
		}

		if (nconf == NULL) {
			MUTEX_UNLOCK(&nconf_mutex);
			goto done;
		}
	}

	if (trace > 1) {
		fprintf(stderr,
			"%d: get_addr: using nconf=(%s,%d,%d,%s,%s,%s)\n",
			myid, nconf->nc_netid, nconf->nc_semantics,
			nconf->nc_flag, nconf->nc_protofmly, nconf->nc_proto,
			nconf->nc_device);
	}

	fd = t_open(nconf->nc_device, O_RDWR, NULL);
	if (fd < 0) {
		syslog(LOG_ERR, gettxt(":94", "%s: %s failed for %s: %m"),
		       "get_addr", "t_open", nconf->nc_device);
		if (trace > 1)
			fprintf(stderr, "%d: get_addr: t_open retry\n", myid);
		retries++;
		goto retry;
	}

	tbind = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
	if (tbind == NULL) {
		syslog(LOG_ERR, gettxt(":245", "%s: no memory for %s"),
		       "get_addr", "tbind");
		MUTEX_UNLOCK(&nconf_mutex);
		goto done;
	}
	
	if (rpcb_getaddr(prog, vers, nconf, &tbind->addr, hostname) == 0) {
/* L000 vvv */
		rpc_createerr_t err = get_rpc_createerr();
		if (err.cf_stat != RPC_N2AXLATEFAILURE) {
			syslog(LOG_ERR,
		 	   gettxt(":94", "%s: %s failed for %s: %s"),
		       	   "get_addr", "rpcb_getaddr",
			   clnt_spcreateerror(nconf->nc_device));

		}
/* L000 ^^^ */
		t_free((char *) tbind, T_BIND);
		tbind = NULL;
		t_close(fd);
		fd = -1;
		if (trace > 1)
			fprintf(stderr, "%d: get_addr: rpcb_getaddr retry\n",
				myid);
		retries++;
		goto retry;
	}
	MUTEX_UNLOCK(&nconf_mutex);

	*nconfp = nconf;

	/*
	 * Make a copy of the netbuf to return
	 */
	nb = (struct netbuf *) malloc(sizeof(struct netbuf));
	if (nb == NULL) {
		syslog(LOG_ERR, gettxt(":245", "%s: no memory for %s"),
		       "get_addr", "nb");
		goto done;
	}
	*nb = tbind->addr;
	nb->buf = (char *)malloc(nb->len);
	if (nb->buf == NULL) {
		syslog(LOG_ERR, gettxt(":245", "%s: no memory for %s"),
		       "get_addr", "buf");
		free(nb);
		nb = NULL;
		goto done;
	}
	(void) memcpy(nb->buf, tbind->addr.buf, tbind->addr.len);

done:
	if (tbind)
		t_free((char *) tbind, T_BIND);
	if (fd >= 0)
		(void) t_close(fd);
	return (nb);
}

/*
 * Description:
 *	Remounts a fs that has been umounted by do_unmount.
 *	This is usually because a fs in this hierarchy cannot
 *	be umounted in do_unmount.
 * Call From:
 *	do_unmount
 * Entry/Exit:
 *	No locks held on entry, during, or exit.
 */
nfsstat
remount(fs)
	struct filsys *fs;
{
	char remname[1024];
	struct stat stbuf;
	thread_t myid = thr_self();

        if (fs->fs_nfsargs.fh == 0) {
		/*
		 * XXX: When would this ever happen?
		 */
		syslog(LOG_ERR,
		       gettxt(":213", "%s: WARNING: %s is NULL"),
		       "remount", "fs_nfsargs.fh");
                return (nfsmount(fs->fs_host, fs->fs_dir,
                                 fs->fs_mntpnt, fs->fs_opts, &fs));
        }

	(void) sprintf(remname, "%s:%s", fs->fs_host, fs->fs_dir);

	if (trace > 1)
		fprintf(stderr, "%d: remount %s %s (%s)\n",
			myid, remname, fs->fs_mntpnt, fs->fs_opts);

	if (pingmount(fs->fs_host) != RPC_SUCCESS) {
		if (fs->fs_unmounted++ < 5)
			syslog(LOG_ERR,
			       gettxt(":214",
				      "%s: %s on %s: server %s is not responding"),
			       "remount", remname, fs->fs_mntpnt, fs->fs_host);
		if (trace > 1)
			fprintf(stderr,
				"%d: remount FAIL: %s not responding\n",
				myid, fs->fs_host);
		return (NFSERR_IO);
	}
	if (stat(fs->fs_mntpnt, &stbuf) < 0) {
		syslog(LOG_ERR, gettxt(":243", "%s: cannot stat %s: %m"),
		       "remount", fs->fs_mntpnt);
		if (trace > 1)
			fprintf(stderr,
				"%d: remount FAIL: cannot stat %s\n",
				myid, fs->fs_mntpnt);
		return (NFSERR_IO);
	}
	if (mount("", fs->fs_mntpnt, fs->fs_mflags | MS_DATA, MNTTYPE_NFS, 
		  &fs->fs_nfsargs, sizeof(fs->fs_nfsargs)) < 0) {
		if (fs->fs_unmounted++ < 5)
			syslog(LOG_ERR,
			       gettxt(":210", "%s: mount of %s on %s: %m"),
			       "remount", remname, fs->fs_mntpnt);
		if (trace > 1)
			fprintf(stderr,
				"%d: remount FAIL: cannot mount %s: %s\n",
				myid, fs->fs_mntpnt, strerror(errno));
		return (NFSERR_IO);
	}

	fs->fs_mntpntdev = stbuf.st_dev;
	if (stat(fs->fs_mntpnt, &stbuf) < 0) {
		syslog(LOG_ERR, gettxt(":243", "%s: cannot stat %s: %m"),
		       "remount", fs->fs_mntpnt);
		if (trace > 1)
			fprintf(stderr,
				"%d: remount INC: cannot stat %s after mount\n",
				myid, fs->fs_mntpnt);
		return (NFSERR_IO);
	}
	fs->fs_mountdev = stbuf.st_dev;

	if (trace > 1) 
		fprintf(stderr, "%d: remount %s OK\n", myid, remname);

	return (NFS_OK);
}

/*
 * Description:
 *	Adds one or more entries to /etc/mnttab
 * Call From:
 *	do_mount, do_unmount
 * Entry/Exit:
 *	On entry, fsq_rwlock required to be write locked.
 *	During, no new locks acquired.
 *	On exit, entry locks remain locked.
 */
void
add_mnttab(rootfs)
	struct filsys *rootfs;
{
	FILE *mnttab;
	struct filsys *fs;
	struct stat stbuf;
	struct mnttab mnt;
	char remname[MAXPATHLEN];
	char tbuf[TIME_MAX];
	char opts[1024];
	int semfile;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: add_mnttab: fs_host=%s, fs_mntpnt=%s\n", 
			myid, rootfs->fs_host, rootfs->fs_mntpnt);

	if (lockmntfile(&semfile) < 0)
		return;
	
	mnttab = fopen(MNTTAB, "a");
	if (mnttab == NULL) {
		close(semfile);
		syslog(LOG_ERR, gettxt(":215", "%s: cannot open %s: %m"),
		       "add_mnttab", MNTTAB);
		return;
	}

	if (lockf(fileno(mnttab), F_LOCK, 0L) < 0) {
		fclose(mnttab);
		close(semfile);
		syslog(LOG_ERR, gettxt(":216", "%s: cannot lock %s: %m"),
		       "add_mnttab", MNTTAB);
		return;
	}
	(void) fseek(mnttab, 0L, 2); /* guarantee at EOF */

	(void) sprintf(tbuf, "%ld", time(0L));
	mnt.mnt_time = tbuf;

	for (fs = TAIL(struct filsys, fs_q); fs; 
	     fs = PREV(struct filsys, fs)) {
		if (fs->fs_rootfs != rootfs)
			continue;

		if (fs->fs_unmounted == 1) {
			syslog(LOG_ERR, gettxt(":257",
			       "%s: WARNING: %s is not mounted"), 
			       "add_mnttab", fs->fs_mntpnt);
			continue;
		}

		(void) sprintf(remname, "%s:%s", fs->fs_host, fs->fs_dir);
		mnt.mnt_special = remname;
		mnt.mnt_mountp = fs->fs_mntpnt;
		mnt.mnt_fstype = MNTTYPE_NFS;

		(void) sprintf(opts, "%s,%s=%x", fs->fs_opts,
			       MNTOPT_DEV, fs->fs_mountdev);
		mnt.mnt_mntopts = opts;

		if (putmntent(mnttab, &mnt) <= 0) {
			fclose(mnttab);
			close(semfile);
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "add_mnttab", "putmntent");
			return;
		}
	}

	fclose(mnttab);
	close(semfile);

	if (stat(MNTTAB, &stbuf) < 0)
		syslog(LOG_ERR, gettxt(":243", "%s: cannot stat %s: %m"),
		       "add_mnttab", MNTTAB);
	else
		last_mnttab_time = stbuf.st_mtime;
}

/*
 * Description:
 *	Duplicate a mnttab structure.
 * Call From:
 *	mkmntlist (before threads start)
 * Entry/Exit:
 *	No locks need to be held on entry, during, or exit.
 */
struct mnttab *
dupmnttab(mnt)
	struct mnttab *mnt;
{
	struct mnttab *new;
	void freemnttab();
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: dupmnttab: mnt->mnt_mountp=%s\n",
			myid, mnt->mnt_mountp);

	new = (struct mnttab *)malloc(sizeof(*new));
	if (new == NULL)
		goto alloc_failed;
	memset((char *)new, 0, sizeof(*new));
	new->mnt_special = strdup(mnt->mnt_special);
	if (new->mnt_special == NULL)
		goto alloc_failed;
	new->mnt_mountp = strdup(mnt->mnt_mountp);
	if (new->mnt_mountp == NULL)
		goto alloc_failed;
	new->mnt_fstype = strdup(mnt->mnt_fstype);
	if (new->mnt_fstype == NULL)
		goto alloc_failed;
	if (mnt->mnt_mntopts) {
		new->mnt_mntopts = strdup(mnt->mnt_mntopts);
		if (new->mnt_mntopts == NULL)
			goto alloc_failed;
	}
	new->mnt_time = strdup(mnt->mnt_time);
	if (new->mnt_time == NULL)
		goto alloc_failed;

	return (new);

alloc_failed:
	syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "dupmnttab");
	freemnttab(new);
	return (NULL);
}

/*
 * Free a single mnttab structure
 */
void
freemnttab(mnt)
	struct mnttab *mnt;
{
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: freemnttab: \n", myid);

	if (mnt) {
		if (mnt->mnt_special)
			free(mnt->mnt_special);
		if (mnt->mnt_mountp)
			free(mnt->mnt_mountp);
		if (mnt->mnt_fstype)
			free(mnt->mnt_fstype);
		if (mnt->mnt_mntopts)
			free(mnt->mnt_mntopts);
		if (mnt->mnt_time)
			free(mnt->mnt_time);
		free(mnt);
	}
}

/*
 * Free a list of mnttab structures
 */
void
freemntlist(mntl)
	struct mntlist *mntl;
{
	struct mntlist *mntl_tmp;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: freemntlist: \n", myid);

	while (mntl) {
		freemnttab(mntl->mntl_mnt);
		mntl_tmp = mntl;
		mntl = mntl->mntl_next;
		free(mntl_tmp);
	}
}

/*
 *
 */
struct mntlist *
mkmntlist()
{
	FILE *mnttab;
	struct mnttab mnt;
	struct mntlist *mntl_head = NULL;
	struct mntlist *mntl_prev = NULL, *mntl;
	int semfile;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: mkmntlist: \n", myid);

	if (lockmntfile(&semfile) < 0)
		return (NULL);

	mnttab = fopen(MNTTAB, "r+");
	if (mnttab == NULL) {
		close(semfile);
		syslog(LOG_ERR, gettxt(":215", "%s: cannot open %s: %m"),
		       "mkmntlist", MNTTAB);
		return (NULL);
	}

	if (lockf(fileno(mnttab), F_LOCK, 0L) < 0) {
		fclose(mnttab);
		close(semfile);
		syslog(LOG_ERR, gettxt(":216", "%s: cannot lock %s: %m"),
		       "mkmntlist", MNTTAB);
		return (NULL);
	}

	/*
	 * Build a list of mnntab structs
	 */
	while (getmntent(mnttab, &mnt) == 0) {
		mntl = (struct mntlist *) malloc(sizeof(*mntl));
		if (mntl == NULL)
			goto alloc_failed;
		if (mntl_head == NULL)
			mntl_head = mntl;
		else
			mntl_prev->mntl_next = mntl;
		mntl_prev = mntl;
		mntl->mntl_next = NULL;
		mntl->mntl_mnt = dupmnttab(&mnt);
		if (mntl->mntl_mnt == NULL)
			goto alloc_failed;
	}

	fclose(mnttab);
	close(semfile);

	return (mntl_head);

alloc_failed:
	fclose(mnttab);
	close(semfile);
	freemntlist(mntl_head);
	return (NULL);
}

/*
 * Description:
 *	Remove all entries from this rootfs's hierarchy
 *	in the mount table.
 *	If check_flag == CHECK, check if fs_unmounted is true.
 *	If check_flag == NOCHECK, remove regardless.
 * Call From:
 *	do_unmount
 * Entry/Exit:
 *	On entry, tmpq_rwlock required to be write locked.
 *	During, no new locks acquired.
 *	On exit, entry locks remain locked.
 */
void
clean_mnttab(rootfs, check_flag)
	struct filsys *rootfs;
	int check_flag;
{
	FILE *mnttab, *tmptab;
	struct mnttab mnt;
	struct stat stbuf;
	struct filsys *fs;
	int delete;
	int semfile;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: clean_mnttab: (%s)\n",
			myid, rootfs->fs_mntpnt);

	if (lockmntfile(&semfile) < 0)
		return;

	mnttab = fopen(MNTTAB, "r+");
	if (mnttab == NULL) {
		close(semfile);
		syslog(LOG_ERR, gettxt(":215", "%s: cannot open %s: %m"),
		       "clean_mnttab", MNTTAB);
		return;
	}

	if (lockf(fileno(mnttab), F_LOCK, 0L) < 0) {
		fclose(mnttab);
		close(semfile);
		syslog(LOG_ERR, gettxt(":216", "%s: cannot lock %s: %m"),
		       "clean_mnttab", MNTTAB);
		return;
	}

	tmptab = fopen(TMPTAB, "w");
	if (tmptab == NULL) {
		fclose(mnttab);
		close(semfile);
		syslog(LOG_ERR, gettxt(":215", "%s: cannot open %s: %m"),
		       "clean_mnttab", TMPTAB);
		return;
	}
		
	/*
	 * Read each entry in mnttab and put it back in
	 * tmptab minus the entries we're trying to delete.
	 */
	while (getmntent(mnttab, &mnt) == 0) {
		delete = 0;
		for (fs = rootfs; fs; fs = NEXT(struct filsys, fs)) {
			if (strcmp(mnt.mnt_mountp, fs->fs_mntpnt) == 0 &&
			    (check_flag || fs->fs_unmounted)) {
				delete = 1;
				break;
			}
		}
		if (delete)
			continue;

		if (putmntent(tmptab, &mnt) < 0) {
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "clean_mnttab", "putmntent");
			fclose(tmptab);
			fclose(mnttab);
			close(semfile);
			return;
		}
	}

	fclose(tmptab);

	(void) chmod(TMPTAB, 0444);
	(void) chown(TMPTAB, MNTTAB_UID, MNTTAB_GID);
	if (rename(TMPTAB, MNTTAB) < 0) {
		syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
		       "clean_mnttab", "rename");
		fclose(mnttab);
		close(semfile);
		return;
	}

	fclose(mnttab);
	close(semfile);

	if (stat(MNTTAB, &stbuf) < 0)
		syslog(LOG_ERR, gettxt(":243", "%s: cannot stat %s: %m"),
		       "clean_mnttab", MNTTAB);
	else
		last_mnttab_time = stbuf.st_mtime;
}

/*
 * Description:
 *	Return the value of a numeric option of the form foo=x,
 *	if option is not found or is malformed, return 0.
 */
nopt(mnt, opt)
	struct mnttab *mnt;
	char *opt;
{
	int val = 0;
	char *equal;
	char *str;

	if (str = hasmntopt(mnt, opt)) {
		if (equal = strchr(str, '=')) {
			val = atoi(&equal[1]);
		} else {
			syslog(LOG_ERR,
			       gettxt(":217", "%s: Bad numeric option %s"),
			       "nopt", str);
		}
	}
	return (val);
}

char *
mntopt(p)
	char **p;
{
	char *cp = *p;
	char *retstr;

	while (*cp && isspace(*cp))
		cp++;
	retstr = cp;
	while (*cp && *cp != ',')
		cp++;
	if (*cp) {
		*cp = '\0';
		cp++;
	}
	*p = cp;
	return (retstr);
}

/*
 * Description:
 *	Checks if mnttab entry mnt has the specified options.
 * Call from:
 * 	already_mounted, nfsmount, nopt
 * Entry/Exit:
 *	No locks held on entry, during, or exit.
 */
char *
hasmntopt(mnt, opt)
	register struct mnttab *mnt;
	register char *opt;
{
	char *f, *opts;
	char tmpopts[256];

	(void) strcpy(tmpopts, mnt->mnt_mntopts);
	opts = tmpopts;
	f = mntopt(&opts);
	for (; *f; f = mntopt(&opts)) {
		if (strncmp(opt, f, strlen(opt)) == 0)
			return (f - tmpopts + mnt->mnt_mntopts);
	}
	return (NULL);
}


/*
 * Description:
 *	Create and lock a semaphore file to protect /etc/mnttab
 *	against simultanous changes. Several programs (e.g. umount)
 *	create a temporary file and rename it to /etc/mnttab,
 *	so locking just /etc/mnttab does not suffice.
 * 	This routine must be called before /etc/mnttab is opened.
 * Call from:
 *	add_mnttab, clean_mnttab, read_mnttab, mkmntlist
 * Entry/Exit:
 *	No locks held on entry, during, or exit.
 */
lockmntfile(int *semfile)
{
	*semfile = creat(SEM_FILE, 0600);
	if (*semfile == -1 ) {
		syslog(LOG_ERR, gettxt(":242", "%s: cannot create %s: %m"),
		       "lockmntfile", SEM_FILE);
		return (-1);
	}
	if (lockf(*semfile, F_LOCK, 0L) < 0) {
		close(*semfile);
		syslog(LOG_ERR, gettxt(":216", "%s: cannot lock %s: %m"),
		       "lockmntfile", SEM_FILE);
		return (-1);
	}
	return (0);
}


/*
 * Description:
 *	Returns an entry from the client handle cache.
 * Call From:
 *	nfsmount, nfsunmount
 * Entry/Exit:
 *	No locks held upon entry or exit.
 *	During, individual client handle cache entry's mutex is locked.
 */
CLIENT *
get_cache_cl(host)
	char *host;
{
	CLIENT *cl;
	int i;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: get_cache_cl: %s\n", myid, host);

	/*
	 * Try to find it first with the client handle cache.
	 */
	for (i = 0; i < CACHE_CL_SIZE; i++) {
		MUTEX_LOCK(&cache_cl[i].mutex);
		if (!strcmp(cache_cl[i].host, host) && 
		    cache_cl[i].in_use == FALSE) {
			cache_cl[i].in_use = TRUE;
			if (trace > 1)
				fprintf(stderr,
					"%d: get_cache_cl: got entry[%d] %s\n",
					myid, i, host);
			MUTEX_UNLOCK(&cache_cl[i].mutex);
			return (cache_cl[i].cl);
		}
		MUTEX_UNLOCK(&cache_cl[i].mutex);
	}

	/*
	 * Just create a new one if none is found in cache.
	 */
	cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "udp");
	if (cl == NULL) {
		syslog(LOG_ERR,
		       gettxt(":218", "%s: %s server not responding for %s"),
		       "get_cache_cl", host, clnt_spcreateerror("clnt_create"));
		return ((CLIENT *)NULL);
	}

	if (client_bindresvport(cl) < 0) {
		syslog(LOG_ERR,
		       gettxt(":219", "%s: %s: could not bind to reserved port"),
		       "get_cache_cl", host);
		clnt_destroy(cl);
		return ((CLIENT *)NULL);
	}

	cl->cl_auth = authsys_create_default();

	add_cache_cl(host, cl);

	return (cl);
}

/*
 * Description:
 *	Delete a client handle cache entry.  This usually happens if
 *	the clnt_call() RPC call fail.
 * Call From:
 *	nfsmount
 * Entry/Exit:
 *	No locks held upon entry or exit.
 *	During, host_mutex and individual client handle cache entry's 
 *		mutex are locked.
 */
void
del_cache_cl(host, cl)
	char *host;
	CLIENT *cl;
{
	CLIENT *old_cl;
	int i;
	extern mutex_t host_mutex;
	thread_t myid = thr_self();

	if (trace >1)
		fprintf(stderr, "%d: del_cache_cl: %s\n", myid, host);

	/*
	 * Also mark this host as dead in dead hosts cache.
	 */
	MUTEX_LOCK(&host_mutex);
	for (i = 0; i < CACHE_HOST_SIZE; i++) {
		if (dead[i].time <= time_now) {
			(void) strcpy(dead[i].host, host);
			dead[i].time = time_now + cache_time;
			break;
		} else if (!strcmp(dead[i].host, host)) {
			dead[i].time = time_now + cache_time;
			break;
		}
        }
	MUTEX_UNLOCK(&host_mutex);

	/*
	 * Remove entry from client handle cache.
	 */
	for (i = 0; i < CACHE_CL_SIZE; i++) {
		MUTEX_LOCK(&cache_cl[i].mutex);
		if (!strcmp(cache_cl[i].host, host) &&
		    cache_cl[i].cl == cl &&
		    cache_cl[i].in_use == TRUE) {
			old_cl = cache_cl[i].cl;
			if (old_cl->cl_auth)
				AUTH_DESTROY(old_cl->cl_auth);
			clnt_destroy(old_cl);
			cache_cl[i].time_valid = 0;
			cache_cl[i].in_use = FALSE;
			cache_cl[i].host[0] = '\0';
			cache_cl[i].cl = NULL;
			if (trace > 1)
				fprintf(stderr,
					"%d: del_cache_cl: delete entry[%d] %s\n",
					myid, i, host);
			MUTEX_UNLOCK(&cache_cl[i].mutex);
			return;
		}
		MUTEX_UNLOCK(&cache_cl[i].mutex);
	}

	/*
	 * Not found in cache, just destroy it.
	 */
	if (cl->cl_auth)
		AUTH_DESTROY(cl->cl_auth);
	clnt_destroy(cl);

	if (trace >1)
		fprintf(stderr, "%d: del_cache_cl: destroy handle %s\n",
			myid, host);
}

/*
 * Description:
 * Call From:
 * Entry/Exit:
 */
void
add_cache_cl(host, cl)
	char *host;
	CLIENT *cl;
{
	CLIENT *old_cl;
	int i;
	int cl_cache_time = 300;
	thread_t myid = thr_self();

	for (i = 0; i < CACHE_CL_SIZE; i++) {
		MUTEX_LOCK(&cache_cl[i].mutex);
		if (!strcmp(cache_cl[i].host, host) &&
		    cache_cl[i].in_use == FALSE) {
			old_cl = cache_cl[i].cl;
			if (old_cl != NULL) {
				if (old_cl->cl_auth)
					AUTH_DESTROY(old_cl->cl_auth);
				clnt_destroy(old_cl);
			}
			cache_cl[i].cl = cl;
			cache_cl[i].time_valid = time_now + cl_cache_time;
			cache_cl[i].in_use = TRUE;
			if (trace > 1)
				fprintf(stderr,
					"%d: add_cache_cl: replace entry[%d] %s\n",
					myid, i, host);
			MUTEX_UNLOCK(&cache_cl[i].mutex);
			return;
		} else if (cache_cl[i].in_use == FALSE &&
			   cache_cl[i].time_valid <= time_now) {
			old_cl = cache_cl[i].cl;
			if (old_cl != NULL) {
				if (old_cl->cl_auth)
					AUTH_DESTROY(old_cl->cl_auth);
				clnt_destroy(old_cl);
			}
			cache_cl[i].cl = cl;
			cache_cl[i].time_valid = time_now + cl_cache_time;
			strcpy(cache_cl[i].host, host);
			cache_cl[i].in_use = TRUE;
			if (trace > 1)
				fprintf(stderr,
					"%d: add_cache_cl: put entry[%d] %s\n",
					myid, i, host);
			MUTEX_UNLOCK(&cache_cl[i].mutex);
			return;
		}
		MUTEX_UNLOCK(&cache_cl[i].mutex);
	}

	if (trace > 1)
		fprintf(stderr, "%d: add_cache_cl: cache full %s\n", myid, host);
}

void
done_cache_cl(host, cl)
	char *host;
	CLIENT *cl;
{
	int i;
	thread_t myid = thr_self();

	for (i = 0; i < CACHE_CL_SIZE; i++) {
		MUTEX_LOCK(&cache_cl[i].mutex);
		if (!strcmp(cache_cl[i].host, host) &&
		    cache_cl[i].cl == cl &&
		    cache_cl[i].in_use == TRUE) {
			cache_cl[i].in_use = FALSE;
			if (trace > 1)
				fprintf(stderr,
					"%d: done_cache_cl: done entry[%d] %s\n",
					myid, i, host);
			MUTEX_UNLOCK(&cache_cl[i].mutex);
			return;
		}
		MUTEX_UNLOCK(&cache_cl[i].mutex);
	}

	if (cl->cl_auth)
		AUTH_DESTROY(cl->cl_auth);
	clnt_destroy(cl);

	if (trace > 1)
		fprintf(stderr, "%d: done_cache_cl: destroy handle %s\n",
			myid, host);
}
