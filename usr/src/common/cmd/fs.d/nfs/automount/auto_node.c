/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)auto_node.c	1.2"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/mnttab.h>
#define _NSL_RPC_ABI
#include <rpc/types.h>
#include <syslog.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/xdr.h>
#include <sys/tiuser.h>
#include <rpc/clnt.h>
#include <netinet/in.h>
#include <unistd.h>
#include "nfs_prot.h"
#define NFSCLIENT
#include <nfs/mount.h>
#include "automount.h"

FILE *file_open();
void getmapbyname();
void dirinit();
void free_link();

struct internal_fh {
	int	fh_pid;
	long	fh_time;
	int	fh_num;
};

static int fh_cnt = 3;
struct q fh_q_hash[FH_HASH_SIZE];

extern int trace;

/*
 * Description:
 *	Initializes a new file handle for an avnode structure
 *	and inserts this avnode in the fh_q_hash hashed queue.
 * Call From:
 *	dirinit, makelink
 * Entry/Exit:
 *	On entry, fh_rwlock required to be write locked.
 *	During, no new locks acquired.
 *	On exit, entry locks remain locked.
 */
new_fh(avnode)
	struct avnode *avnode;
{
	register struct internal_fh *ifh =
		(struct internal_fh *)(&avnode->vn_fh);
	extern int right_pid;
	extern time_t right_time;
	thread_t myid = thr_self();

	if (trace > 1)	
		fprintf(stderr, "%d: new_fh: fh_cnt=%d\n", myid, fh_cnt + 1);
	
	ifh->fh_pid = right_pid;
	ifh->fh_time = right_time;

	ifh->fh_num = ++fh_cnt;

	INSQUE(fh_q_hash[ifh->fh_num % FH_HASH_SIZE], avnode);
}

/*
 * Description:
 *	Removes this avnode from the fh_q_hash hashed queue.
 * Call From:
 *	free_link
 * Entry/Exit:
 *	On entry, fh_rwlock required to be write locked.
 *		  avnode's vn_mutex required to be locked.
 *	During, no new locks acquired.
 *	On exit, entry locks remain locked.
 */
free_fh(avnode)
	struct avnode *avnode;
{
	register struct internal_fh *ifh =
		(struct internal_fh *)(&avnode->vn_fh);
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: free_fh: fh_num=%d in hash=%d\n", 
			myid, ifh->fh_num, ifh->fh_num % FH_HASH_SIZE);

	REMQUE(fh_q_hash[ifh->fh_num % FH_HASH_SIZE], avnode);
}

/*
 * Description:
 *	Converts a file handle to an avnode.
 * Call From:
 *	nfsproc_getattr_2_svc, nfsproc_setattr_2_svc, nfsproc_lookup_2_svc,
 *	nfsproc_readlink_2_svc, nfsproc_readdir_2_svc
 * Entry/Exit:
 *	On entry, no locks held.
 *	During, fh_rwlock is read locked.
 *	On exit, if an avnode is found, its vn_mutex is locked.
 */
struct avnode *
fhtovn(fh)
	nfs_fh *fh;
{
	register struct internal_fh *ifh = 
		(struct internal_fh *)fh;
	int num;
	struct avnode *avnode;
	extern int right_pid;
	extern time_t right_time;
	extern rwlock_t fh_rwlock;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: fhtovn: fh_num=%d in hash=%d\n", 
			myid, ifh->fh_num, ifh->fh_num % FH_HASH_SIZE);
	
	if (ifh->fh_pid != right_pid || ifh->fh_time != right_time)
		return ((struct avnode *) 0);

	num = ifh->fh_num;

	RW_RDLOCK(&fh_rwlock);
	avnode = HEAD(struct avnode, fh_q_hash[num % FH_HASH_SIZE]);
	while (avnode) {
		ifh = (struct internal_fh *)(&avnode->vn_fh);
		if (num == ifh->fh_num) {
			MUTEX_LOCK(&avnode->vn_mutex);

			if (avnode->vn_valid) {
				RW_UNLOCK(&fh_rwlock);
				return (avnode);
			} else {
				MUTEX_UNLOCK(&avnode->vn_mutex);
				RW_UNLOCK(&fh_rwlock);
				return ((struct avnode *)0);
			}
		}
		avnode = NEXT(struct avnode, avnode);
	}
	RW_UNLOCK(&fh_rwlock);

	return ((struct avnode *)0);
}

/*
 * Description:
 *	Returns the fileid number from the file handle
 * Call From:
 *	dirinit, makelink
 * Entry/Exit:
 *	On entry and exit, the avnode's vn_mutex should be held.
 */
int
fileid(avnode)
	struct avnode *avnode;
{
	register struct internal_fh *ifh =
		(struct internal_fh *)(&avnode->vn_fh);

	return (ifh->fh_num);
}

/*
 * Description:
 *	Initializes the dir_q queue.
 * Call From:
 *	main, loadmaster_yp, loaddirect_yp, loadmaster_file, loaddirect_file
 *	(before any threads are started)
 * Entry/Exit:
 *	No locks held on entry or exit.
 */
void
dirinit(mntpnt, map, opts, isdirect)
	char *mntpnt, *map, *opts;
	int isdirect;
{
	struct autodir *dir;
	register fattr *fa;
	struct stat stbuf;
	int mydir = 0;
	struct link *link;
	extern int verbose;
	char *check_hier();
	char *check_stacked();
	char *opt_check();
	char *p;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: dirinit: (%s, %s, %s, %d)\n", 
			myid, mntpnt, map, opts, isdirect);

	for (dir = HEAD(struct autodir, dir_q); dir;
	     dir = NEXT(struct autodir, dir))
		if (strcmp(dir->dir_name, mntpnt) == 0)
			return;

	p = mntpnt + (strlen(mntpnt) - 1);
	if (*p == '/')
		*p = '\0';	/* trim trailing / */
	if (*mntpnt != '/') {
		syslog(LOG_ERR,
		       gettxt(":220", "%s: mountpoint %s must start with a /"),
		       "dirinit", mntpnt);
		return;
	}
	if (p = check_hier(mntpnt)) {
		syslog(LOG_ERR,
		       gettxt(":221", "%s: hierarchical mountpoint: %s and %s"),
		       "dirinit", p, mntpnt);
		return;
	}
	if (p = check_stacked(mntpnt)) {
		syslog(LOG_ERR,
		       gettxt(":222", "%s: %s is already mounted at %s"),
		       "dirinit", p, mntpnt);
		return;
	}
	if (p = opt_check(opts)) {
		syslog(LOG_ERR,
		       gettxt(":223", "%s: WARNING: default option %s ignored for map %s"),
		       "dirinit", p, map);
	}

	/*
	 * If it's a direct map then call dirinit
	 * for every map entry. Try first for a local
	 * file, then a NIS map.
	 */
	if (strcmp(mntpnt, "/-") == 0) {
		/* the third parameter specifies type of map
		 * 0 = master map , 1 = direct map
		 */
		(void) getmapbyname(map, opts, 1);
		return;
	}

	/*
	 * Check whether the map (local file or NIS) exists
	 */
	if (*map != '-')
		if (map_exists(map) != 0) {
			if (verbose)
				syslog(LOG_ERR,
				       gettxt(":224", "%s: map %s is not found"),
				       "dirinit", map);
			return;
		}


	/*
	 * Create a mount point if necessary
	 */
	if (lstat(mntpnt, &stbuf) == 0) {
		if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
			syslog(LOG_ERR,
			       gettxt(":225", "%s: %s is not a directory"),
			       "dirinit", mntpnt);
			return;
		}
		if (verbose && !emptydir(mntpnt) && !strcmp(map, "-null"))
			syslog(LOG_ERR,
			       gettxt(":226", "%s: WARNING: %s not empty!"),
			       "dirinit", mntpnt);
	} else {
		if (mkdir_r(mntpnt)) {
			syslog(LOG_ERR,
			       gettxt(":227", "%s: %s: cannot create directory %s: %m"),
			       "dirinit", "mkdir_r", mntpnt);
			return;
		}
		mydir = 1;
	}

	dir = (struct autodir *)malloc(sizeof *dir);
	if (dir == NULL)
		goto alloc_failed;
	memset((char *)dir, 0, sizeof *dir);
	dir->dir_name = strdup(mntpnt);
	if (dir->dir_name == NULL) {
		free((char *)dir);
		goto alloc_failed;
	}
	dir->dir_map = strdup(map);
	if (dir->dir_map == NULL) {
		free((char *)dir->dir_name);
		free((char *)dir);
		goto alloc_failed;
	}
	dir->dir_opts = strdup(opts);
	if (dir->dir_opts == NULL) {
		free((char *)dir->dir_name);
		free((char *)dir->dir_map);
		free((char *)dir);
		goto alloc_failed;
	}
	dir->dir_remove = mydir;
	rwlock_init(&dir->dir_rwlock, USYNC_THREAD, NULL);
	mutex_init(&dir->dir_vnode.vn_mutex, USYNC_THREAD, NULL);
	INSQUE(dir_q, dir);

	new_fh(&dir->dir_vnode);
	dir->dir_vnode.vn_data = (char *)dir;
	dir->dir_vnode.vn_valid = 1;
	dir->dir_vnode.vn_count = 0;
	fa = &dir->dir_vnode.vn_fattr;
	fa->nlink = 1;
	fa->uid = 0;
	fa->gid = 0;
	fa->size = 512;
	fa->blocksize = 512;
	fa->rdev = 0;
	fa->blocks = 1;
	fa->fsid = 0;
	fa->fileid = fileid(&dir->dir_vnode);
	(void) gettimeofday((struct timeval *)&fa->atime, (struct timezone *)0);
	fa->mtime = fa->atime;
	fa->ctime = fa->atime;

	if (!isdirect) {
		/* The mount point is a directory.
		 * Set up links for it's "." and ".." entries.
		 */
		dir->dir_vnode.vn_type = VN_DIR;
		fa->type = NFDIR;
		fa->mode = NFSMODE_DIR + 0555;

		link = makelink(dir, "." , NULL, "");
		if (link == NULL)
			goto alloc_failed;
		link->link_vnode.vn_fattr.fileid = fileid(&link->link_vnode);
		MUTEX_UNLOCK(&link->link_vnode.vn_mutex); 

		link = makelink(dir, "..", NULL, "");
		if (link == NULL)
			goto alloc_failed;
		link->link_vnode.vn_fattr.fileid = fileid(&link->link_vnode);
		MUTEX_UNLOCK(&link->link_vnode.vn_mutex);
	} else {
		/* The mount point is direct-mapped. Set it
		 * up as a symlink to the real mount point.
		 */
		dir->dir_vnode.vn_type = VN_LINK;
		fa->type = NFLNK;
		fa->mode = NFSMODE_LNK + 0777;
		fa->size = strlen(mntpnt) + strlen(tmpdir);

		link = (struct link *)malloc(sizeof *link);
		if (link == NULL)
			goto alloc_failed;
		dir->dir_vnode.vn_data = (char *)link;
		link->link_dir = dir;
		link->link_name = strdup(mntpnt);
		if (link->link_name == NULL) {
			free((char *)link);
			goto alloc_failed;
		}
		link->link_fs = NULL;
		link->link_path = NULL;
		link->link_death = 0;
		link->link_vnode.vn_valid = 1;
		link->link_vnode.vn_count = 0;
		mutex_init(&link->link_vnode.vn_mutex, USYNC_THREAD, NULL);
	}
	return;

alloc_failed:
	syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "dirinit");
	return;
}

/*
 *  Check whether the mount point is a
 *  subdirectory or a parent directory
 *  of any previously mounted automount
 *  mount point.
 */
char *
check_hier(mntpnt)
	char *mntpnt;
{
	register struct autodir *dir;
	register char *p, *q;
	thread_t myid = thr_self();

	if (trace > 2) 
		fprintf(stderr, "%d: check_hier: mntpnt=%s\n", myid, mntpnt);

	for (dir = TAIL(struct autodir, dir_q) ; dir ; 
	     dir = PREV(struct autodir, dir)) {
		if (strcmp(dir->dir_map, "-null") == 0)
			continue;
		p = dir->dir_name;
		q = mntpnt;
		for (; *p == *q ; p++, q++)
			if (*p == '\0')
				break;
		if (*p == '/' && *q == '\0')
			return (dir->dir_name);
		if (*p == '\0' && *q == '/')
			return (dir->dir_name);
		if (*p == '\0' && *q == '\0')
			return (dir->dir_name);
	}
	return (NULL);	/* it's not a subdir or parent */
}

/*
 * Check whether there's something
 * already mounted at the mountpoint.
 */
char *
check_stacked(mntpnt)
	char *mntpnt;
{
	extern struct mntlist *current_mounts;
	struct mntlist *mntl;
	struct mnttab *mnt;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: check_stacked: mntpnt=%s\n", myid, mntpnt);

	for (mntl = current_mounts; mntl; mntl = mntl->mntl_next) {
		mnt = mntl->mntl_mnt;

		if (strcmp(mntpnt, mnt->mnt_mountp) == 0)
			return (mnt->mnt_special);
	}
	return (NULL);
}

#include <dirent.h>

emptydir(name)
	char *name;
{
	DIR *dirp;
	struct dirent *d;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: emptydir: name=%s\n", myid, name);

	dirp = opendir(name);
	if (dirp == NULL) 
		return (0);
	while (d = readdir(dirp)) {
		if (strcmp(d->d_name, ".") == 0)
			continue;
		if (strcmp(d->d_name, "..") == 0)
			continue;
		break;
	}
	(void) closedir(dirp);
	if (d)
		return (0);
	return (1);
}

/*
 * Description:
 *	Finds a link or allocates a new one.
 * Call From:
 *	dirinit, lookup
 * Entry/Exit:
 *	On entry, no locks held.
 *	During, fh_rwlock and dir->dir_rwlock are write locked
 *		to insert newly allocated link to dir->dir_head.
 *	On exit, if a link is found, its vn_mutex is locked.
 */
struct link *
makelink(dir, name, fs, linkpath)
	struct autodir *dir;
	char *name;
	struct filsys *fs;
	char *linkpath;
{
	struct link *link;
	register fattr *fa;
	struct avnode *avnode;
	extern rwlock_t fsq_rwlock;
	extern rwlock_t fh_rwlock;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr,
			"%d: makelink: dir_name=%s, name=%s, linkpath=%s\n",
			myid, dir->dir_name, name, linkpath);

	/*
	 * If findlink returns with a link, its vn_mutex is locked.
	 */
	link = findlink(dir, name);
	if (link == NULL) {
		if (trace > 1)
			fprintf(stderr, "%d: makelink: alloc new link for %s\n",
				myid, name);

		link = (struct link *)malloc(sizeof *link);
		if (link == NULL)
			goto alloc_failed;

		link->link_name = strdup(name);
		if (link->link_name == NULL) {
			free((char *)link);
			goto alloc_failed;
		}
		link->link_path = NULL;
		link->link_death = 0;
		
		avnode = &link->link_vnode;
		avnode->vn_valid = 1;
		avnode->vn_count = 0;
		avnode->vn_data = (char *)link;
		avnode->vn_type = VN_LINK;
		mutex_init(&avnode->vn_mutex, USYNC_THREAD, NULL);

		/*
		 * Insert into queues only after all data is in.
		 */
		RW_WRLOCK(&fh_rwlock);
		RW_WRLOCK(&dir->dir_rwlock);

		INSQUE(dir->dir_head, link);
		new_fh(avnode);

		RW_UNLOCK(&dir->dir_rwlock);
		RW_UNLOCK(&fh_rwlock);

		/*
		 * Lock vn_mutex before updating other fields.
		 */
		MUTEX_LOCK(&avnode->vn_mutex);
	} 
	avnode = &link->link_vnode;

	link->link_dir = dir;
	link->link_fs = NULL;
	if (link->link_path) {
		free(link->link_path);
		link->link_path = NULL;
	}
	if (linkpath) {
		link->link_path = strdup(linkpath);
		if (link->link_path == NULL) {
			avnode->vn_valid = 0;
			MUTEX_UNLOCK(&avnode->vn_mutex);
			syslog(LOG_ERR, gettxt(":245", "%s: no memory for %s"),
			       "makelink", "link_path");
			return (NULL);
		}
	}
	link->link_death = time_now + max_link_time;

	fa = &avnode->vn_fattr;
	fa->type = NFLNK;
	fa->mode = NFSMODE_LNK + 0777;
	fa->nlink = 1;
	fa->uid = 0;
	fa->gid = 0;
	fa->size = strlen(linkpath);
	fa->blocksize = 512;
	fa->rdev = 0;
	fa->blocks = 1;
	fa->fsid = 0;
	fa->fileid = fileid(avnode);
	(void) gettimeofday((struct timeval *)&fa->atime, (struct timezone *)0);
	fa->mtime = fa->atime;
	fa->ctime = fa->atime;

	if (fs) {
		RW_RDLOCK(&fsq_rwlock);

		MUTEX_LOCK(&fs->fs_mutex);
		if (fs != fs->fs_rootfs)
			syslog(LOG_ERR,
			       gettxt(":251", "%s: ERROR: not using the root fs"),
			       "makelink");
		fs->fs_death = time_now + max_link_time;
		MUTEX_UNLOCK(&fs->fs_mutex);

		link->link_fs = fs;

		RW_UNLOCK(&fsq_rwlock);
	}

	/*
	 * vn_mutex is still held.
	 */
	return (link);

alloc_failed:
	syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "makelink");
	return (NULL);
}

/*
 * Description:
 *	Free memory for a link structure.
 * Call From:
 *	do_timeouts
 * Entry/Exit:
 *	On entry, link's vn_mutex, fh_rwlock, and dir->dir_rwlock are locked.
 */
void
free_link(link)
	struct link *link;
{
	struct autodir *dir;
	struct avnode *avnode;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: free_link: link_name=%s\n",
			myid, link->link_name);

	dir = link->link_dir;
	avnode = &link->link_vnode;

	/*
	 * First, remove link from dir_head queue that may lead to the link.
	 */
	REMQUE(dir->dir_head, link);

	/*
	 * Second, remove link's avnode from fh_q_hash queue that may lead
	 * to the link's avnode.
	 */
	free_fh(avnode);

	if (link->link_name)
		free(link->link_name);
	if (link->link_path)
		free(link->link_path);

	MUTEX_UNLOCK(&avnode->vn_mutex);
	mutex_destroy(&avnode->vn_mutex);

	free((char *)link);
}

/*
 * Description:
 *	Finds a valid link for a particular dir.
 * Call From:
 *	makelink, nfsproc_readlink_2_svc, nfsproc_lookup_2_svc
 * Entry/Exit:
 *	On entry, no dir and avnode locks should be held.
 *	On exit, link's vn_mutex is locked.
 */
struct link *
findlink(dir, name)
	struct autodir *dir;
	char *name;
{
	struct link *link;
	struct avnode *avnode;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: findlink: (%s, %s)\n",
			myid, dir->dir_name, name);

	RW_RDLOCK(&dir->dir_rwlock);
	for (link = HEAD(struct link, dir->dir_head); link;
	     link = NEXT(struct link, link)) {
		avnode = &link->link_vnode;
		MUTEX_LOCK(&avnode->vn_mutex);
		if (strcmp(name, link->link_name) == 0) {
			if (avnode->vn_valid) {
				RW_UNLOCK(&dir->dir_rwlock);
				return (link);
			} else {
				MUTEX_UNLOCK(&avnode->vn_mutex);
				RW_UNLOCK(&dir->dir_rwlock);
				return ((struct link *) 0);
			}
		}
		MUTEX_UNLOCK(&avnode->vn_mutex);
	}
	RW_UNLOCK(&dir->dir_rwlock);
	return ((struct link *) 0);
}

/*
 * Description:
 *	Free a filsys structure.
 * Call From:
 *	read_mnttab, do_unmount
 * Entry/Exit:
 *	On entry, tmpq_rwlock may be locked.
 *	During, no new locks acquired.
 *	On exit, entry locks remain locked.
 */
free_filsys(fs)
	struct filsys *fs;
{
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: free_filsys: (%s, %s)\n",
			myid, fs->fs_host, fs->fs_mntpnt);

	mutex_destroy(&fs->fs_mutex);
	if (fs->fs_host)
		free(fs->fs_host);
	if (fs->fs_dir)
		free(fs->fs_dir);
	if (fs->fs_mntpnt)
		free(fs->fs_mntpnt);
	if (fs->fs_opts)
		free(fs->fs_opts);
	netbuf_free(fs->fs_nfsargs.addr);
	netbuf_free(fs->fs_nfsargs.syncaddr);
	free_knconf(fs->fs_nfsargs.knconf);
	free((char *)fs);
}

/*
 * Description:
 *	Allocates a new filsys structure.
 * Call From:
 *	read_mnttab, nfsmount
 * Entry/Exit:
 *	No locks need to be held on entry, during, or exit.
 */
struct filsys *
alloc_fs(host, dir, mntpnt, opts)
	char *host, *dir, *mntpnt, *opts;
{
	struct filsys *fs;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: alloc_fs: (%s, %s, %s)\n",
			myid, host, dir, mntpnt);

	fs = (struct filsys *)malloc(sizeof *fs);
	if (fs == NULL)
		goto alloc_failed;
	memset((char *)fs, 0, sizeof *fs);

	fs->fs_rootfs = fs;
	fs->fs_host = strdup(host);
	if (fs->fs_host == NULL) {
		free((char *)fs);
		goto alloc_failed;
	}
	fs->fs_dir = strdup(dir);
	if (fs->fs_dir == NULL) {
		free(fs->fs_host);
		free((char *)fs);
		goto alloc_failed;
	}
	fs->fs_mntpnt = strdup(mntpnt);
	if (fs->fs_mntpnt == NULL) {
		free(fs->fs_dir);
		free(fs->fs_host);
		free((char *)fs);
		goto alloc_failed;
	}
	if (opts != NULL) {
		fs->fs_opts = strdup(opts);
		if (fs->fs_opts == NULL) {
			free(fs->fs_mntpnt);
			free(fs->fs_dir);
			free(fs->fs_host);
			free((char *)fs);
			goto alloc_failed;
		}
	} else
		fs->fs_opts = NULL;

	mutex_init(&fs->fs_mutex, USYNC_THREAD, NULL);

	return (fs);

alloc_failed:
	syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "alloc_fs");
	return (NULL);
}

my_insque(head, item)
	struct q *head, *item;
{
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: my_insque: \n", myid);

	item->q_next = head->q_head;
	item->q_prev = NULL;
	head->q_head = item;
	if (item->q_next)
		item->q_next->q_prev = item;
	if (head->q_tail == NULL)
		head->q_tail = item;
}

my_remque(head, item)
	struct q *head, *item;
{
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: my_remque: \n", myid);

	if (item->q_prev)
		item->q_prev->q_next = item->q_next;
	else
		head->q_head = item->q_next;
	if (item->q_next)
		item->q_next->q_prev = item->q_prev;
	else
		head->q_tail = item->q_prev;
	item->q_next = item->q_prev = NULL;
}

/*
 * Description:
 *	Timeout routine that checks if any fs should be unmounted.
 * Call From:
 *	auto_run_parallel
 * Entry/Exit:
 *	No locks need to held on entry or exit.
 *	During, fh_rwlock, dir_rwlock locks, fsq_rwlock, and tmpq_rwlock
 *		are write locked.
 */
do_timeouts()
{
	struct autodir *dir;
	struct link *link, *nextlink;
	struct filsys *fs, *nextfs;
	struct avnode *avnode;
	int remove = 0;
	extern int trace, syntaxok;
	extern rwlock_t fsq_rwlock;
	extern rwlock_t tmpq_rwlock;
	extern rwlock_t fh_rwlock;
	thread_t myid = thr_self();

	if (trace > 1) 
		fprintf(stderr, "%d: do_timeouts: enter\n", myid);

	check_mnttab();
	syntaxok = 1;

	RW_WRLOCK(&fh_rwlock);
	for (dir = HEAD(struct autodir, dir_q); dir;
	     dir = NEXT(struct autodir, dir)) {
		RW_WRLOCK(&dir->dir_rwlock);
		for (link = HEAD(struct link, dir->dir_head); link;
		     link = nextlink) {
			nextlink = NEXT(struct link, link);
			avnode = &link->link_vnode;
			
			MUTEX_LOCK(&avnode->vn_mutex);
			if (trace > 1)
				fprintf(stderr,
					"%d: do_timeouts: link (%d, %d) %s\n",
					myid, avnode->vn_valid, avnode->vn_count,
					link->link_name);

			if (avnode->vn_valid) {
				if (link->link_death && 
				    link->link_death <= time_now) {
					ZERO_LINK(link);
				}
				MUTEX_UNLOCK(&avnode->vn_mutex);
			} else {
				if (avnode->vn_count == 0) {
					free_link(link);
				} else {
					MUTEX_UNLOCK(&avnode->vn_mutex);
				}
			}
		}
		RW_UNLOCK(&dir->dir_rwlock);
	}
	RW_UNLOCK(&fh_rwlock);

	/*
	 * Check fs_q to see if any fs need to be unmounted.
	 * If found one, remove it from fs_q and put in tmp_q.
	 */
	RW_WRLOCK(&fsq_rwlock);
	for (fs = HEAD(struct filsys, fs_q); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (trace > 1) {
			if (fs == fs->fs_rootfs) {
				fprintf(stderr,
					"%d: do_timeouts: rootfs (%d, %d) %s\n",
					myid, fs->fs_mine,
					(fs->fs_rootfs->fs_death <= time_now),
					fs->fs_mntpnt);
			} else {
				fprintf(stderr,
					"%d: do_timeouts: fs (%d, %d) %s\n",
					myid, fs->fs_mine,
					(fs->fs_rootfs->fs_death <= time_now),
					fs->fs_mntpnt);
			}
		}
		       
		if (fs->fs_mine &&
		    fs == fs->fs_rootfs &&
		    fs->fs_death <= time_now) {
			RW_WRLOCK(&tmpq_rwlock);
			tmpq.q_head = tmpq.q_tail = NULL;
			do_remove(fs);
			RW_UNLOCK(&tmpq_rwlock);
			remove = 1;
			break;
		}
	}
	RW_UNLOCK(&fsq_rwlock);

	/*
	 * If there is a fs to be unmounted, go ahead and unmount it.
	 */
	if (remove)
		do_unmount(CONT);

	if (trace > 1) 
		fprintf(stderr, "%d: do_timeouts: exit\n", myid);
}

/*
 * Description:
 *	Finds a link that points to this fs and set it's
 *	link_fs to null.
 * Call From:
 *	read_mnttab
 * Entry/Exit:
 *	On entry, no locks held.
 *	During, fh_rwlock is read locked.
 *		avnode's vn_mutex is locked.
 *	On exit, during locks are released.
 */
flush_links(fs)
	struct filsys *fs;
{
	struct link *link;
	struct avnode *avnode;
	int i;
	extern rwlock_t fh_rwlock;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: flush_links: fs_mntpnt=%s\n",
			myid, fs->fs_mntpnt);

	RW_RDLOCK(&fh_rwlock);
	for (i = 0; i < FH_HASH_SIZE; i++) {
		avnode = HEAD(struct avnode, fh_q_hash[i]);
		for (; avnode; avnode = NEXT(struct avnode, avnode)) {
			if (avnode->vn_type != VN_LINK)
				continue;
			MUTEX_LOCK(&avnode->vn_mutex);
			link = (struct link *)avnode->vn_data;
			if (link->link_fs == fs) {
				ZERO_LINK(link);
			}
			MUTEX_UNLOCK(&avnode->vn_mutex);
		}
	}
	RW_UNLOCK(&fh_rwlock);
}

