/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)auto_all.c	1.2"
#ident	"$Header$"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define _NSL_RPC_ABI
#include <rpc/rpc.h>
#include <syslog.h>
#include <rpc/pmap_clnt.h>
#include <sys/mnttab.h>
#include <sys/mntent.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include "nfs_prot.h"
#define NFSCLIENT
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#include <nfs/mount.h>
#include "automount.h"

#define CHECK	0
#define NOCHECK	1

void safe_rmdir();

extern int trace;
extern int verbose;

int tmpq_status;
int tmpq_waiting;

char *optlist[] = {
	MNTOPT_RO,	MNTOPT_RW,	MNTOPT_GRPID,	MNTOPT_SOFT,
	MNTOPT_HARD,	MNTOPT_NOSUID,	MNTOPT_INTR,	MNTOPT_SECURE,
	MNTOPT_NOAC,	MNTOPT_NOCTO,	MNTOPT_PORT,	MNTOPT_RETRANS,
	MNTOPT_RSIZE,	MNTOPT_WSIZE,	MNTOPT_TIMEO,	MNTOPT_ACTIMEO,
	MNTOPT_ACREGMIN,MNTOPT_ACREGMAX,MNTOPT_ACDIRMIN,MNTOPT_ACDIRMAX,
	MNTOPT_QUOTA,	MNTOPT_NOQUOTA, MNTOPT_NOINTR,	MNTOPT_SUID,
	MNTOPT_BG,
	NULL
};

char *
opt_check(opts)
	char *opts;
{
	char buf[256];
	char lastbuf[256];
	register char *p, *pe, *pb;
	register char **q;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: opt_check: opts=%s\n", myid, opts);

	if (strlen(opts) > (size_t) 254)
		return (NULL);

	(void) strcpy(buf, opts);
	pb = buf;

	while (p = strtok_r(pb, ",", (char **)&lastbuf)) {
		pb = NULL;
		if (pe = strchr(p, '='))
			*pe = '\0';
		for (q = optlist ; *q ; q++) {
			if (strcmp(p, *q) == 0)
				break;
		}
		if (*q == NULL) {
			if (trace > 1)
				fprintf(stderr, "%d: opt_check: p=%s\n",
					myid, p);
			return (p);
		}
	}
	return (NULL);
}

/*
 * Description:
 *	Remove a set of fs from fs_q and put in on tmp_q.
 * Call From:
 *	do_timeouts, windup
 * Entry/Exit:
 *	On entry, fsq_rwlock required to be write locked.
 *		  tmpq_rwlock required to be write locked.
 *	During, no new locks acquired.
 *	On exit, entry locks remain locked.
 */
do_remove(fsys)
	struct filsys *fsys;
{
	struct filsys *fs, *nextfs;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: do_remove: (%s)\n", myid, fsys->fs_mntpnt);

	tmpq_status = WORK;
	tmpq_waiting = 0;
	for (fs = HEAD(struct filsys, fs_q); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (fs->fs_rootfs == fsys) {
			REMQUE(fs_q, fs);
			INSQUE(tmpq, fs);
		}
	}
}

/*
 * Description:
 *	Unmount fs's that are on tmpq.
 * 		If successful, clean mnttab and return.
 *		If unsuccessful (i.e. mountpoint busy)
 *			remount unmounted fs's and put them back on fs_q.
 * Call From:
 *	do_timeouts, windup
 * Entry/Exit:
 *	On entry, no locks are held.
 *	During, tmpq_rwlock is held write locked when removing from tmp_q.
 *		fsq_rwlock is held write locked when inserting back in fs_q.
 *		unmount_mutex is held when finished and broadcasting.
 *	On exit, no locks are held.
 */
do_unmount(exiting)
	int exiting;
{
	struct filsys *fs, *nextfs, *rootfs;
	dev_t olddev;
	int newdevs;
	extern rwlock_t fsq_rwlock;
	extern rwlock_t tmpq_rwlock;
	extern mutex_t unmount_mutex;
	extern cond_t unmount_cond;
	thread_t myid = thr_self();

	/* walk backwards trying to unmount */
	for (fs = TAIL(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = PREV(struct filsys, fs);
		if (trace > 1)
			fprintf(stderr, "%d: unmount: (%s, %d) ?\n",
				myid, fs->fs_mntpnt, fs->fs_unmounted);
		if (fs->fs_unmounted)
			continue;

		if (trace > 1)
			fprintf(stderr, "%d: unmount (%d) %s: ...\n",
				myid, (fs == fs->fs_rootfs), fs->fs_mntpnt);
		if (!pathok(fs) || umount(fs->fs_mntpnt) < 0) { 
			if (trace > 1)
				fprintf(stderr, "%d: unmount (%d) %s: BUSY\n",
					myid, (fs == fs->fs_rootfs),
					fs->fs_mntpnt);
			goto inuse;
		} else {
			fs->fs_unmounted = 1;
			if (trace > 1)
				fprintf(stderr, "%d: unmount (%d) %s: OK\n",
					myid, (fs == fs->fs_rootfs),
					fs->fs_mntpnt);
		}
	}

	/* all ok - walk backwards removing directories */
	RW_WRLOCK(&tmpq_rwlock);

	clean_mnttab(HEAD(struct filsys, tmpq), CHECK);

	for (fs = TAIL(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = PREV(struct filsys, fs);
		nfsunmount(fs);
		safe_rmdir(fs->fs_mntpnt);
		REMQUE(tmpq, fs);
		free_filsys(fs);
	}

	RW_UNLOCK(&tmpq_rwlock);

	MUTEX_LOCK(&unmount_mutex);
	tmpq_status = PASS;
	if (tmpq_waiting)
		cond_broadcast(&unmount_cond);
	MUTEX_UNLOCK(&unmount_mutex);

	/* success */
	return (1);

inuse:
	/* not ok - remount previous unmounted ones */
	newdevs = 0;
	for (fs = HEAD(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (fs->fs_unmounted) {
			olddev = fs->fs_mountdev;
			if (remount(fs) == NFS_OK) {
				fs->fs_unmounted = 0;
				if (fs->fs_mountdev != olddev)
					newdevs++;
			}
		}
	}

	RW_WRLOCK(&fsq_rwlock);
	RW_WRLOCK(&tmpq_rwlock);

	/* remove entries with old dev ids */
	if (newdevs) {
		if (trace > 1)
			fprintf(stderr,
				"%d: do_unmount: newdevs=%d, cleaning %s\n",
				myid, newdevs,
				HEAD(struct filsys, tmpq)->fs_mntpnt);
		clean_mnttab(HEAD(struct filsys, tmpq), NOCHECK);
		rootfs = HEAD(struct filsys, tmpq)->fs_rootfs;
	}

	/* put things back on the correct list */
	for (fs = HEAD(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		
		if (fs == fs->fs_rootfs) {
			fs->fs_death = time_now + max_link_time;

			/* if exiting, do not try to unmount fs again */
			if (exiting)
				fs->fs_mine = 0;
		}
		REMQUE(tmpq, fs);
		INSQUE(fs_q, fs);
	}

	/* put updated entries back */
	if (newdevs) {
		if (trace > 1)
			fprintf(stderr,
				"%d: do_unmount: newdevs=%d, adding %s\n",
				myid, newdevs, rootfs->fs_mntpnt);
		add_mnttab(rootfs);
	}

	RW_UNLOCK(&tmpq_rwlock);
	RW_UNLOCK(&fsq_rwlock);

	MUTEX_LOCK(&unmount_mutex);
	tmpq_status = FAIL;
	if (tmpq_waiting)
		cond_broadcast(&unmount_cond);
	MUTEX_UNLOCK(&unmount_mutex);

	return (0);
}


/*
 * Description:
 *	Check a path prior to using it.  This avoids hangups in
 *	the umount system call from dead mount points in the 
 * 	path to the mount point we're trying to unmount.
 * 	If all the mount points ping OK then return 1.
 * Call from:
 * 	do_unmount
 * Entry/Exit:
 *	No locks held on entry, during, or exit.
 */
int
pathok(tfs)
	struct filsys *tfs;
{
	extern dev_t tmpdev;
	struct filsys *fs, *nextfs;
	enum clnt_stat pingmount();
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: pathok: (%s)\n", myid, tfs->fs_mntpnt);

	while (tfs->fs_mntpntdev != tmpdev && tfs->fs_rootfs != tfs) {
		for (fs = HEAD(struct filsys, tmpq); fs; fs = nextfs) {
			nextfs = NEXT(struct filsys, fs);
			if (tfs->fs_mntpntdev == fs->fs_mountdev)
				break;
		}
		if (fs == NULL) {
			syslog(LOG_ERR,
			       gettxt(":189",
			         "%s: could not find devid %04x(%04x) for %s"),
			       "pathok", tfs->fs_mntpntdev & 0xFFF, 
			       tmpdev & 0xFFF, tfs->fs_mntpnt);
			return (1);
		}
		if (trace > 1)
			fprintf(stderr, "%d: pathok: %s ... \n",
				myid, fs->fs_mntpnt);
		if (pingmount(fs->fs_host) != RPC_SUCCESS) {
			if (trace > 1)
				fprintf(stderr, "%d: pathok: %s is dead\n",
					myid, fs->fs_mntpnt);
			return (0);
		}
		if (trace > 1)
			fprintf(stderr, "%d: pathok: %s is okay\n",
				myid, fs->fs_mntpnt);
		tfs = fs;
	}
	return (1);
}


/*
 * Description:
 *	Frees an exports structure.
 * Call from:
 * 	getmapent_hosts
 * Entry/Exit:
 *	No locks held on entry, during, or exit.
 */
void
freeex(ex)
	struct exports *ex;
{
	struct groups *groups, *tmpgroups;
	struct exports *tmpex;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: freeex: \n", myid);

	while (ex) {
		free(ex->ex_name);
		groups = ex->ex_groups;
		while (groups) {
			free(groups->g_name);
			tmpgroups = groups->g_next;
			free((char *)groups);
			groups = tmpgroups;
		}
		tmpex = ex->ex_next;
		free((char *)ex);
		ex = tmpex;
	}
}


/*
 * Description:
 *	Recursively makes a directory and its path.
 * Call from:
 * 	main, dirinit, do_mount, mkdir_r
 * Entry/Exit:
 *	No locks held on entry, during, or exit.
 */
mkdir_r(dir)
	char *dir;
{
	int err;
	char *slash;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: mkdir_r: dir=%s\n", myid, dir);

	if (mkdir(dir, 0555) == 0 || errno == EEXIST)
		return (0);
	if (errno != ENOENT)
		return (-1);
	slash = strrchr(dir, '/');
	if (slash == NULL)
		return (-1);
	*slash = '\0';
	err = mkdir_r(dir);
	*slash++ = '/';
	if (err || !*slash)
		return (err);
	return (mkdir(dir, 0555));
}


/*
 * Description:
 *	Removes a directory created by the automounter.
 * Call from:
 * 	do_mount, do_unmount, windup
 * Entry/Exit:
 *	No locks held on entry, during, or exit.
 */
void
safe_rmdir(dir)
	char *dir;
{
	extern dev_t tmpdev;
	struct stat stbuf;
	thread_t myid = thr_self();

	if (trace > 2)
		fprintf(stderr, "%d: safe_rmdir: dir=%s\n", myid, dir);

	if (stat(dir, &stbuf)) {
		return;
	}
	if (stbuf.st_dev == tmpdev) {
		(void) rmdir(dir);
	}
}
