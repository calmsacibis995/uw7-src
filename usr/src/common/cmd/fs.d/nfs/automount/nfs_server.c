/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)nfs_server.c	1.2"
#ident	"$Header$"

#include <stdio.h>
#define _NSL_RPC_ABI
#include <rpc/rpc.h>
#include <syslog.h>
#include <thread.h>
#include <synch.h>
#include <unistd.h>
#include "nfs_prot.h"

#ifdef DEBUG

#define	MUTEX_LOCK(mutex) {						\
	fprintf(stderr, "%d: Locking Mutex %x ... \n", myid, mutex);	\
	mutex_lock(mutex);						\
	fprintf(stderr, "%d: Locked Mutex %x\n", myid, mutex);		\
	}

#define	MUTEX_UNLOCK(mutex) {						\
	fprintf(stderr, "%d: Unlocking Mutex %x ... \n", myid, mutex);	\
	mutex_unlock(mutex);						\
	fprintf(stderr, "%d: Unlocked Mutex %x\n", myid, mutex);	\
	}

#else

#define	MUTEX_LOCK	mutex_lock
#define	MUTEX_UNLOCK	mutex_unlock

#endif

extern int trace;

static struct dupreq {
	u_long		xid;
	struct dupreq	*next;
};
static struct dupreq *reqcache;

/*
 * Copy straight from libnsl/rpc/svc_dg.c file.  Needed to find xid
 * to determine duplicate requests.
 */
#define MAX_OPT_WORDS	32
struct svc_dg_data {
	struct netbuf	optbuf;			/* netbuf for options */
	long	opts[MAX_OPT_WORDS];		/* options */
	u_int	su_iosz;			/* size of send.recv buffer */
	u_long	su_xid;				/* transaction id */
	XDR	su_xdrs;			/* XDR handle */
	char	su_verfbody[MAX_AUTH_BYTES];	/* verifier body */
	char	*su_cache;			/* cached data, NULL if none */
};
#define REQTOXID(req)	((struct svc_dg_data *)((req)->rq_xprt->xp_p2))->su_xid

static void generic_free(char *);
static void readdir_free(readdirres *);
static void readlink_free(readlinkres *);
static int  dupreq_check(struct svc_req *);
static void dupreq_delete(struct svc_req *);

void
nfs_program_2(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	union {
		nfs_fh nfsproc_getattr_2_arg;
		sattrargs nfsproc_setattr_2_arg;
		diropargs nfsproc_lookup_2_arg;
		nfs_fh nfsproc_readlink_2_arg;
		readargs nfsproc_read_2_arg;
		writeargs nfsproc_write_2_arg;
		createargs nfsproc_create_2_arg;
		diropargs nfsproc_remove_2_arg;
		renameargs nfsproc_rename_2_arg;
		linkargs nfsproc_link_2_arg;
		symlinkargs nfsproc_symlink_2_arg;
		createargs nfsproc_mkdir_2_arg;
		diropargs nfsproc_rmdir_2_arg;
		readdirargs nfsproc_readdir_2_arg;
		nfs_fh nfsproc_statfs_2_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local_proc)();
	char *(*local_free)();
	extern attrstat *nfsproc_getattr_2_svc();
	extern attrstat *nfsproc_setattr_2_svc();
	extern void *nfsproc_root_2_svc();
	extern diropres *nfsproc_lookup_2_svc();
	extern readlinkres *nfsproc_readlink_2_svc();
	extern readres *nfsproc_read_2_svc();
	extern void *nfsproc_writecache_2_svc();
	extern attrstat *nfsproc_write_2_svc();
	extern diropres *nfsproc_create_2_svc();
	extern nfsstat *nfsproc_remove_2_svc();
	extern nfsstat *nfsproc_rename_2_svc();
	extern nfsstat *nfsproc_link_2_svc();
	extern nfsstat *nfsproc_symlink_2_svc();
	extern diropres *nfsproc_mkdir_2_svc();
	extern nfsstat *nfsproc_rmdir_2_svc();
	extern readdirres *nfsproc_readdir_2_svc();
	extern statfsres *nfsproc_statfs_2_svc();
	extern mutex_t dupreq_mutex;
	thread_t myid = thr_self();

	if (rqstp->rq_cred.oa_flavor != AUTH_UNIX) {
		svcerr_weakauth(transp);
		return;
	}

	local_free = (char *(*)()) generic_free;

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply(transp, xdr_void, (caddr_t)NULL);
		return;

	case NFSPROC_GETATTR:
		xdr_argument = xdr_nfs_fh;
		xdr_result = xdr_attrstat;
		local_proc = (char *(*)()) nfsproc_getattr_2_svc;
		break;

	case NFSPROC_SETATTR:
		xdr_argument = xdr_sattrargs;
		xdr_result = xdr_attrstat;
		local_proc = (char *(*)()) nfsproc_setattr_2_svc;
		break;

	case NFSPROC_ROOT:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local_proc = (char *(*)()) nfsproc_root_2_svc;
		break;

	case NFSPROC_LOOKUP:
		xdr_argument = xdr_diropargs;
		xdr_result = xdr_diropres;
		local_proc = (char *(*)()) nfsproc_lookup_2_svc;
		break;

	case NFSPROC_READLINK:
		xdr_argument = xdr_nfs_fh;
		xdr_result = xdr_readlinkres;
		local_proc = (char *(*)()) nfsproc_readlink_2_svc;
		local_free = (char *(*)()) readlink_free;
		break;

	case NFSPROC_READ:
		xdr_argument = xdr_readargs;
		xdr_result = xdr_readres;
		local_proc = (char *(*)()) nfsproc_read_2_svc;
		break;

	case NFSPROC_WRITECACHE:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local_proc = (char *(*)()) nfsproc_writecache_2_svc;
		break;

	case NFSPROC_WRITE:
		xdr_argument = xdr_writeargs;
		xdr_result = xdr_attrstat;
		local_proc = (char *(*)()) nfsproc_write_2_svc;
		break;

	case NFSPROC_CREATE:
		xdr_argument = xdr_createargs;
		xdr_result = xdr_diropres;
		local_proc = (char *(*)()) nfsproc_create_2_svc;
		break;

	case NFSPROC_REMOVE:
		xdr_argument = xdr_diropargs;
		xdr_result = xdr_nfsstat;
		local_proc = (char *(*)()) nfsproc_remove_2_svc;
		break;

	case NFSPROC_RENAME:
		xdr_argument = xdr_renameargs;
		xdr_result = xdr_nfsstat;
		local_proc = (char *(*)()) nfsproc_rename_2_svc;
		break;

	case NFSPROC_LINK:
		xdr_argument = xdr_linkargs;
		xdr_result = xdr_nfsstat;
		local_proc = (char *(*)()) nfsproc_link_2_svc;
		break;

	case NFSPROC_SYMLINK:
		xdr_argument = xdr_symlinkargs;
		xdr_result = xdr_nfsstat;
		local_proc = (char *(*)()) nfsproc_symlink_2_svc;
		break;

	case NFSPROC_MKDIR:
		xdr_argument = xdr_createargs;
		xdr_result = xdr_diropres;
		local_proc = (char *(*)()) nfsproc_mkdir_2_svc;
		break;

	case NFSPROC_RMDIR:
		xdr_argument = xdr_diropargs;
		xdr_result = xdr_nfsstat;
		local_proc = (char *(*)()) nfsproc_rmdir_2_svc;
		break;

	case NFSPROC_READDIR:
		xdr_argument = xdr_readdirargs;
		xdr_result = xdr_readdirres;
		local_proc = (char *(*)()) nfsproc_readdir_2_svc;
		local_free = (char *(*)()) readdir_free;
		break;

	case NFSPROC_STATFS:
		xdr_argument = xdr_nfs_fh;
		xdr_result = xdr_statfsres;
		local_proc = (char *(*)()) nfsproc_statfs_2_svc;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}

	memset(&argument, 0, sizeof(argument));

	MUTEX_LOCK(&dupreq_mutex);
	if (dupreq_check(rqstp)) {
		MUTEX_UNLOCK(&dupreq_mutex);
		return;
	}
	MUTEX_UNLOCK(&dupreq_mutex);

	if (! svc_getargs(transp, xdr_argument, (caddr_t)&argument)) {
		svcerr_decode(transp);
		return;
	}

	if (trace)
		trace_call(rqstp->rq_proc, &argument); 
	result = (*local_proc)(&argument, rqstp);
	if (trace)
		trace_return(rqstp->rq_proc, result); 

	if (result && !svc_sendreply(transp, xdr_result, (caddr_t)result)) {
		svcerr_systemerr(transp);
	}

	if (! svc_freeargs(transp, xdr_argument, (caddr_t)&argument)) {
		exit(1);
	}

	MUTEX_LOCK(&dupreq_mutex);
	dupreq_delete(rqstp);
	MUTEX_UNLOCK(&dupreq_mutex);

	(*local_free)(result);

}

int
dupreq_check(rqstp)
	struct svc_req *rqstp;
{
	struct dupreq *dr, *prev_dr;
	u_long xid;
	thread_t myid = thr_self();

	xid = REQTOXID(rqstp);
	prev_dr = NULL;
	for (dr = reqcache; dr; dr = dr->next) {
		if (dr->xid == xid) {
			if (trace > 1)
				fprintf(stderr,
					"%d: dupreq_check: DUPLICATE request %d\n",
					myid, xid);
			return (1);
		}
		prev_dr = dr;
	}

	dr = (struct dupreq *) malloc(sizeof(struct dupreq));
	if (dr == NULL) {
		syslog(LOG_ERR, gettxt(":96", "%s: no memory"), "dupreq_check");
		return (1);
	}
	dr->xid = xid;
	dr->next = (struct dupreq *)NULL;
	if (prev_dr)
		prev_dr->next = dr;
	else
		reqcache = dr;

	return (0);
}

void
dupreq_delete(rqstp)
	struct svc_req *rqstp;
{
	struct dupreq *dr, *prev_dr;
	u_long xid;

	xid = REQTOXID(rqstp); 
	prev_dr = NULL;
	for (dr = reqcache; dr; dr = dr->next) {
		if (dr->xid == xid) {
			if (prev_dr)
				prev_dr->next = dr->next;
			else
				reqcache = dr->next;
			free((char *)dr);
			return;
		}
		prev_dr = dr;
	}
	syslog(LOG_ERR, gettxt(":246", "ERROR: no entry for request %d"),
	       xid);
	return;
}

void
generic_free(result)
     char *result;
{
	if (result) {
		free(result);
	}
}

void
readdir_free(result)
     readdirres *result;
{
	struct entry *e, *nexte;

	if (result) {
		e = result->readdirres_u.reply.entries;
		while (e != NULL) {
			nexte = e->nextentry;
			if (e->name)
				free(e->name);
			free((char *)e);
			e = nexte;
		}
		free((char *)result);
	}
}

void
readlink_free(result)
     readlinkres *result;
{
	if (result) {
		if (result->readlinkres_u.data) {
			free(result->readlinkres_u.data);
		}

		free((char *)result);
	}
}

