/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)auto_proc.c	1.3"
#ident	"$Header$"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#define _NSL_RPC_ABI
#include <rpc/types.h>
#include <syslog.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/xdr.h>
#include <rpc/rpc.h>
#include <netinet/in.h>
#include <unistd.h>
#include "nfs_prot.h"
#define NFSCLIENT
#include <nfs/mount.h>
#include "automount.h"

static struct cache_rq {
	struct autodir	*dir;
	char		name[NFS_MAXNAMLEN];
	int		count;
	int 		looking;
	int		status;			/* status of the lookup() call */
	time_t		time_valid;
	cond_t		cond;
	mutex_t		mutex;
	struct cache_rq	*next;
};
struct cache_rq *lookup_rq;

int  rq_check();
static int time_request = 10*60;
int num_requests;
extern int trace;

#define MAX_REQUESTS	10	/* MAX_REQUESTS should be > MAX_THREADS */

#define DO_LOOKUP	0
#define WAIT_LOOKUP	1
#define ERROR_LOOKUP	2

/*
 * add up sizeof (valid + fileid + name + cookie) - strlen(name)
 */
#define ENTRYSIZE (3 * BYTES_PER_XDR_UNIT + NFS_COOKIESIZE)

/*
 * sizeof(status + eof)
 */
#define JUNKSIZE (2 * BYTES_PER_XDR_UNIT)

#define	UPDATE_DEATH(link) {					\
	RW_RDLOCK(&fsq_rwlock);					\
	if ((fs = link->link_fs) != NULL) {			\
		MUTEX_LOCK(&fs->fs_mutex);			\
		fs->fs_death = time_now + max_link_time;	\
		MUTEX_UNLOCK(&fs->fs_mutex);			\
	}							\
	RW_UNLOCK(&fsq_rwlock);					\
	link->link_death = time_now + max_link_time;		\
	}

#define	VN_RELEASE(avnode) {				\
	MUTEX_LOCK(&avnode->vn_mutex);			\
	avnode->vn_count--;				\
	MUTEX_UNLOCK(&avnode->vn_mutex);		\
	}

/* ARGSUSED */
attrstat *
nfsproc_getattr_2_svc(fh, rqstp)
	nfs_fh *fh;
	struct svc_req *rqstp;
{
	struct avnode *avnode;
	attrstat *astat;
	thread_t myid = thr_self();

	astat = (attrstat *) malloc( sizeof(* astat));
	if (astat == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_getattr");
		return ((attrstat *)NULL);
	}

	avnode = fhtovn(fh);
	if (avnode == NULL)
		astat->status = NFSERR_STALE;
	else {
		astat->status = NFS_OK;
		astat->attrstat_u.attributes = avnode->vn_fattr;

		MUTEX_UNLOCK(&avnode->vn_mutex);
	}

	return (astat);
}

/* ARGSUSED */
attrstat *
nfsproc_setattr_2_svc(args, rqstp)
	sattrargs *args;
	struct svc_req *rqstp;
{
	attrstat *astat;
	struct avnode *avnode;
	thread_t myid = thr_self();

	astat = (attrstat *) malloc( sizeof(* astat));
	if (astat == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_setattr");
		return ((attrstat *)NULL);
	}

	avnode = fhtovn(&args->file);
	if (avnode == NULL)
		astat->status = NFSERR_STALE;
	else {
		astat->status = NFSERR_ROFS;

		MUTEX_UNLOCK(&avnode->vn_mutex);
	}

	return (astat);
}

/* ARGSUSED */
void *
nfsproc_root_2_svc(args, rqstp)
	void *args;
	struct svc_req *rqstp;
{
	return (NULL);
}

diropres *
nfsproc_lookup_2_svc(args, rqstp)
	diropargs *args;
	struct svc_req *rqstp;
{
	struct avnode *avnode, *lvnode;
	struct autodir *dir;
	diropres *res;
	nfsstat status;
	struct cache_rq *rq;
	int rq_state;
	char *name;
	struct link *link;
	struct filsys *fs;
	extern rwlock_t fsq_rwlock;
	thread_t myid = thr_self();

	res = (diropres *) malloc( sizeof(* res));
	if (res == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_lookup");
		return ((diropres *)NULL);
	}
	res->status = NFS_OK;

	avnode = fhtovn(&args->dir);
	if (avnode == NULL) {
		res->status = NFSERR_STALE;
		return (res);
	}
	if (avnode->vn_type != VN_DIR) {
		res->status = NFSERR_NOTDIR;
		MUTEX_UNLOCK(&avnode->vn_mutex);
		return (res);
	}
	dir = (struct autodir *)avnode->vn_data;
	name = args->name;
	MUTEX_UNLOCK(&avnode->vn_mutex);

	if (name[0] == '.' &&
	    (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
		res->diropres_u.diropres.file = dir->dir_vnode.vn_fh;
		res->diropres_u.diropres.attributes = dir->dir_vnode.vn_fattr;

		return (res);
	}

recheck:
	if ((link = findlink(dir, name)) != NULL) {
		if (link->link_death > time_now) {
			UPDATE_DEATH(link);

			res->diropres_u.diropres.file = link->link_vnode.vn_fh;
			res->diropres_u.diropres.attributes = 
			                         link->link_vnode.vn_fattr;

			MUTEX_UNLOCK(&link->link_vnode.vn_mutex);

			return (res);
		}
		if (trace > 1)
			fprintf(stderr,
				"%d: nfs_lookup: %s fs REMOVE\n",
				myid, link->link_path);

		MUTEX_UNLOCK(&link->link_vnode.vn_mutex);
	}

	rq_state = rq_check(dir, name, &rq);

	if (rq_state == WAIT_LOOKUP) {
		MUTEX_LOCK(&rq->mutex);
		while (rq->looking == TRUE)
			cond_wait(&rq->cond, &rq->mutex);
		rq->count--;
		if (rq->status == NFS_OK) {
			MUTEX_UNLOCK(&rq->mutex);
			goto recheck;
		} else {
			res->status = rq->status;
			MUTEX_UNLOCK(&rq->mutex);
			return (res);
		}
	} else if (rq_state == ERROR_LOOKUP) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_lookup");
		res->status = NFSERR_NOSPC;
		return (res);
	}

	/*
	 * If lookup() returns NFS_OK, lvnode has its vn_mutex locked.
	 */
	status = lookup(dir, name, &lvnode, rqstp->rq_clntcred);

	if (status != NFS_OK) {
		res->status = status;
	} else {
		res->diropres_u.diropres.file = lvnode->vn_fh;
		res->diropres_u.diropres.attributes = lvnode->vn_fattr;

		MUTEX_UNLOCK(&lvnode->vn_mutex);
	}

	MUTEX_LOCK(&rq->mutex);
	rq->status = status;
	rq->looking = FALSE;
	if (rq->count > 1)
		cond_broadcast(&rq->cond);
	rq->count--;
	MUTEX_UNLOCK(&rq->mutex);

	return (res);
}
			
readlinkres *
nfsproc_readlink_2_svc(fh, rqstp)
	nfs_fh *fh;
	struct svc_req *rqstp;
{
	readlinkres *res;
	nfsstat status;
	struct avnode *avnode, *lvnode = NULL;
	struct link *link, *alink;
	struct filsys *fs;
	struct autodir *dir;
	char *name;
	struct cache_rq *rq;
	int rq_state;
	extern rwlock_t fsq_rwlock;
	thread_t myid = thr_self();

	res = (readlinkres *) malloc( sizeof(* res));
	if (res == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_readlink");
		return ((readlinkres *)NULL);
	}
	res->readlinkres_u.data = (char *)NULL;
	res->status = NFS_OK;

	avnode = fhtovn(fh);
	if (avnode == NULL) {
		res->status = NFSERR_STALE;
		return (res);
	}
	if (avnode->vn_type != VN_LINK) {
		res->status = NFSERR_NXIO;
		MUTEX_UNLOCK(&avnode->vn_mutex);
		return (res);
	}
	alink = (struct link *)(avnode->vn_data);

	if (alink->link_death > time_now) {
		UPDATE_DEATH(alink);

		res->readlinkres_u.data = strdup(alink->link_path);
		if (res->readlinkres_u.data == NULL) {
			syslog(LOG_ERR, gettxt(":96", "%s: no memory"), 
			       "nfs_readlink");
			res->status = NFSERR_NOSPC;
		}

		MUTEX_UNLOCK(&avnode->vn_mutex);
		return (res);
	}

	dir = alink->link_dir;
	name = alink->link_name;

	/*
	 * Increment avnode's vn_count before releasing lock.
	 */
	avnode->vn_count++;
	MUTEX_UNLOCK(&avnode->vn_mutex);

recheck:
	if ((link = findlink(dir, name)) != NULL) {
		if (link->link_death > time_now) {
			UPDATE_DEATH(link);

			res->readlinkres_u.data = strdup(link->link_path);
			if (res->readlinkres_u.data == NULL) {
				syslog(LOG_ERR, gettxt(":96", "%s: no memory"), 
				       "nfs_readlink");
				res->status = NFSERR_NOSPC;
			}

			MUTEX_UNLOCK(&link->link_vnode.vn_mutex);

			VN_RELEASE(avnode);

			return (res);
		}
		if (trace > 1)
			fprintf(stderr,
				"%d: nfs_readlink: %s fs REMOVE\n",
				myid, link->link_path);

		MUTEX_UNLOCK(&link->link_vnode.vn_mutex);
	}

	rq_state = rq_check(dir, name, &rq);
		
	if (rq_state == WAIT_LOOKUP) {
		MUTEX_LOCK(&rq->mutex);
		while (rq->looking == TRUE)
			cond_wait(&rq->cond, &rq->mutex);
		rq->count--;
		if (rq->status == NFS_OK) {
			MUTEX_UNLOCK(&rq->mutex);
			goto recheck;
		} else {
			res->status = rq->status;
			MUTEX_UNLOCK(&rq->mutex);

			VN_RELEASE(avnode);

			return (res);
		}
	} else if (rq_state == ERROR_LOOKUP) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_readlink");

		res->status = NFSERR_NOSPC;

		VN_RELEASE(avnode);

		return (res);
	}

	/*
	 * If lookup() returns NFS_OK, lvnode has its vn_mutex locked.
	 */
	status = lookup(dir, name, &lvnode, rqstp->rq_clntcred);

	if (avnode != lvnode)
		MUTEX_LOCK(&avnode->vn_mutex);

	avnode->vn_count--;

	if (status != NFS_OK) {
		if (dir->dir_vnode.vn_type == VN_LINK) {
			syslog(LOG_ERR,
			       gettxt(":258", "%s: zero out link (%s, %s)"),
			       "nfs_readlink", dir->dir_name, name);
			ZERO_LINK(alink);
		} else {
			syslog(LOG_ERR,
			       gettxt(":259", "%s: invalidate link (%s, %s)"),
			       "nfs_readlink", dir->dir_name, name);
			avnode->vn_valid = 0;
		}

		res->status = status;

		MUTEX_UNLOCK(&avnode->vn_mutex);
	} else {
		if (avnode != lvnode)
			MUTEX_UNLOCK(&avnode->vn_mutex);

		link = (struct link *)(lvnode->vn_data);

		UPDATE_DEATH(link);

		res->readlinkres_u.data = strdup(link->link_path);
		if (res->readlinkres_u.data == NULL) {
			syslog(LOG_ERR, gettxt(":96", "%s: no memory"),
			       "nfs_readlink");
			res->status = NFSERR_NOSPC;
		}

		MUTEX_UNLOCK(&lvnode->vn_mutex);
	}

	MUTEX_LOCK(&rq->mutex);
	rq->status = status;
	rq->looking = FALSE;
	if (rq->count > 1)
		cond_broadcast(&rq->cond);
	rq->count--;
	MUTEX_UNLOCK(&rq->mutex);

	return (res);
}

/* ARGSUSED */
readres *
nfsproc_read_2_svc(args, rqstp)
	readargs *args;
	struct svc_req *rqstp;
{
	readres *res;

	res = (readres *) malloc( sizeof(* res));
	if (res == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_read");
		return ((readres *)NULL);
	}

	res->status = NFSERR_ISDIR;	/* XXX: should return better error */
	return (res);
}

/* ARGSUSED */
void *
nfsproc_writecache_2_svc(args, rqstp)
	void *args;
	struct svc_req *rqstp;
{
	return (NULL);
}	

/* ARGSUSED */
attrstat *
nfsproc_write_2_svc(args, rqstp)
	writeargs *args;
	struct svc_req *rqstp;
{
	attrstat *res;
	res = (attrstat *) malloc( sizeof(* res));
	if (res == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_write");
		return ((attrstat *)NULL);
	}

	res->status = NFSERR_ROFS;	/* XXX: should return better error */
	return (res);
}

/* ARGSUSED */
diropres *
nfsproc_create_2_svc(args, rqstp)
	createargs *args;
	struct svc_req *rqstp;
{
	diropres *res;

	res = (diropres *) malloc( sizeof(* res));
	if (res == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_create");
		return ((diropres *)NULL);
	}

	res->status = NFSERR_ROFS;
	return (res);
}

/* ARGSUSED */
nfsstat *
nfsproc_remove_2_svc(args, rqstp)
	diropargs *args;
	struct svc_req *rqstp;
{
	nfsstat *status;

	status = (nfsstat *) malloc( sizeof(* status));
	if (status == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_remove");
		return ((nfsstat *)NULL);
	}

	*status = NFSERR_ROFS;
	return (status);
}

/* ARGSUSED */
nfsstat *
nfsproc_rename_2_svc(args, rqstp)
	renameargs *args;
	struct svc_req *rqstp;
{
	nfsstat *status;

	status = (nfsstat *) malloc( sizeof(* status));
	if (status == NULL) {
		syslog(LOG_ERR,
		       gettxt(":96", "%s: no memory"), "nfs_rename");
		return ((nfsstat *)NULL);
	}

	*status = NFSERR_ROFS;
	return (status);
}

/* ARGSUSED */
nfsstat *
nfsproc_link_2_svc(args, rqstp)
	linkargs *args;
	struct svc_req *rqstp;
{
	nfsstat *status;

	status = (nfsstat *) malloc( sizeof(* status));
	if (status == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_link");
		return ((nfsstat *)NULL);
	}
	*status = NFSERR_ROFS;
	return (status);
}

/* ARGSUSED */
nfsstat *
nfsproc_symlink_2_svc(args, rqstp)
	symlinkargs *args;
	struct svc_req *rqstp;
{
	nfsstat *status;

	status = (nfsstat *) malloc( sizeof(* status));
	if (status == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_symlink");
		return ((nfsstat *)NULL);
	}
	*status = NFSERR_ROFS;
	return (status);
}

/* ARGSUSED */
diropres *
nfsproc_mkdir_2_svc(args, rqstp)
	createargs *args;
	struct svc_req *rqstp;
{
	diropres *res;

	res = (diropres *) malloc( sizeof(* res));
	if (res == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_mkdir");
		return ((diropres *)NULL);
	}
	res->status = NFSERR_ROFS;
	return (res);
}

/* ARGSUSED */
nfsstat *
nfsproc_rmdir_2_svc(args, rqstp)
	diropargs *args;
	struct svc_req *rqstp;
{
	nfsstat *status;

	status = (nfsstat *) malloc( sizeof(* status));
	if (status == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_rmdir");
		return ((nfsstat *)NULL);
	}
	*status = NFSERR_ROFS;
	return (status);
}

/* ARGSUSED */
readdirres *
nfsproc_readdir_2_svc(args, rqstp)
	readdirargs *args;
	struct svc_req *rqstp;
{
	readdirres *res;
	struct avnode *avnode, *lvnode;
	struct entry **entp;
	struct autodir *dir;
	struct link *link;
	int cookie;
	int count;
	int entrycount;
	thread_t myid = thr_self();

	res = (readdirres *) malloc( sizeof(* res));
	if (res == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_readdir1");
		return ((readdirres *)NULL);
	}

	avnode = fhtovn(&args->dir);
	if (avnode == NULL) {
		res->status = NFSERR_STALE;
		return (res);
	}
	if (avnode->vn_type != VN_DIR) {
		res->status = NFSERR_NOTDIR;
		MUTEX_UNLOCK(&avnode->vn_mutex);
		return (res);
	}
	dir = (struct autodir *)avnode->vn_data;
	MUTEX_UNLOCK(&avnode->vn_mutex);

	cookie = *(unsigned *)args->cookie;
	count = args->count - JUNKSIZE;

	entrycount = 0;
	entp = &(res->readdirres_u.reply.entries);
	*entp = NULL;

	RW_RDLOCK(&dir->dir_rwlock);
	for (link = HEAD(struct link, dir->dir_head); link;
	     link = NEXT(struct link, link)) {
		if (count <= ENTRYSIZE) 
			goto full;
		if (entrycount++ < cookie)
			continue;

		lvnode = &link->link_vnode;
		MUTEX_LOCK(&lvnode->vn_mutex);

		if (link->link_death && time_now >= link->link_death) {
			MUTEX_UNLOCK(&lvnode->vn_mutex);
			continue;
		}
		*entp = (struct entry *) malloc(sizeof(entry));
		if (*entp == NULL) {
			MUTEX_UNLOCK(&lvnode->vn_mutex);
			syslog(LOG_ERR, gettxt(":96", "%s: no memory"),
			       "nfs_readdir2");
			break;
		}
		(*entp)->fileid = link->link_vnode.vn_fattr.fileid;
		if (link->link_death && time_now >= link->link_death)
			(*entp)->fileid = 0;
		else
			(*entp)->fileid = link->link_vnode.vn_fattr.fileid;
		(*entp)->name = strdup(link->link_name);
		if ((*entp)->name == NULL) {
			free((char *) *entp);
			MUTEX_UNLOCK(&lvnode->vn_mutex);
			syslog(LOG_ERR, gettxt(":96", "%s: no memory"),
			       "nfs_readdir3");
			break;
		}
		*(unsigned *)((*entp)->cookie) = ++cookie;
		(*entp)->nextentry = NULL;
		entp = &(*entp)->nextentry;
		count -= (ENTRYSIZE + strlen(link->link_name));
		MUTEX_UNLOCK(&lvnode->vn_mutex);
	}
full:
	RW_UNLOCK(&dir->dir_rwlock);

	if (count > ENTRYSIZE)
		res->readdirres_u.reply.eof = TRUE;
	else
		res->readdirres_u.reply.eof = FALSE;
	res->status = NFS_OK;

	return (res);
}

/* ARGSUSED */
statfsres *
nfsproc_statfs_2_svc(fh, rqstp)
	nfs_fh *fh;
	struct svc_req *rqstp;
{
	statfsres *res;

	res = (statfsres *) malloc( sizeof(* res));
	if (res == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "nfs_statfs");
		return ((statfsres *)NULL);
	}

	res->status = NFS_OK;
	res->statfsres_u.reply.tsize = 512;
	res->statfsres_u.reply.bsize = 512;
	res->statfsres_u.reply.blocks = 0;
	res->statfsres_u.reply.bfree = 0;
	res->statfsres_u.reply.bavail = 0;
	return (res);
}

/*
 * Description:
 *	Finds a request from the lookup_rq linked list, reuse an old
 *	entry, or creates a new one.
 * Call From:
 *	nfsproc_lookup_2_svc, nfsproc_readlink_2_svc
 * Entry/Exit:
 *	No locks held on entry or exit.
 * 	During, lookup_mutex is held while going thru lookup_rq.
 */
rq_check(dir, name, return_rq)
	struct autodir *dir;
	char *name;
	struct cache_rq **return_rq;
{
	int retval = DO_LOOKUP;
	struct cache_rq *rq, *lru_rq = NULL;
	extern mutex_t lookup_mutex;
	thread_t myid = thr_self();

	if (trace > 1)
		fprintf(stderr, "%d: rq_check: (%s, %s)\n",
			myid, dir->dir_name, name);

	MUTEX_LOCK(&lookup_mutex);

	for (rq = lookup_rq; rq; rq = rq->next) {
		MUTEX_LOCK(&rq->mutex);
		if (rq->dir == dir &&
		    (dir->dir_vnode.vn_type == VN_LINK ||
		     strcmp(rq->name, name) == 0)) {
			rq->count++;
			rq->time_valid = time_now + time_request;
			*return_rq = rq;

			if (rq->looking == TRUE) {
				MUTEX_UNLOCK(&rq->mutex);
				retval = WAIT_LOOKUP;
				goto done;
			} else {
				rq->looking = TRUE;
				MUTEX_UNLOCK(&rq->mutex);
				goto done;
			}
		} else if (rq->count == 0) {
			if (lru_rq == NULL)
				lru_rq = rq;
			else if (rq->time_valid < lru_rq->time_valid)
				lru_rq = rq;
		}
		MUTEX_UNLOCK(&rq->mutex);
	}

	if (num_requests < MAX_REQUESTS) {
		rq = (struct cache_rq *) malloc(sizeof(struct cache_rq));
		if (rq == NULL) {
			retval = ERROR_LOOKUP;
			goto done;
		}
		cond_init(&rq->cond, USYNC_THREAD, NULL);
		mutex_init(&rq->mutex, USYNC_THREAD, NULL);
		num_requests++;

		rq->next = lookup_rq;
		lookup_rq = rq;

		if (trace > 1)
			fprintf(stderr, "%d: rq_check: new (%s, %s) (%d)\n",
				myid, dir->dir_name, name, num_requests);
	} else {
		rq = lru_rq;
		if (trace > 1)
			fprintf(stderr, "%d: rq_check: reuse %s for %s\n",
				myid, rq->name, name);
	}

	rq->dir = dir;
	(void) strcpy(rq->name, name);
	rq->count = 1;
	rq->looking = TRUE;
	rq->time_valid = time_now + time_request;

	*return_rq = rq;

done:
	if (retval == ERROR_LOOKUP)
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "rq_check");

	MUTEX_UNLOCK(&lookup_mutex);
	return (retval);
}
