/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)prot_proc.c	1.2"
#ident	"$Header$"

/*
 *              PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *              Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *      (c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *      (c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *                All rights reserved.
 */

/*
 * prot_proc.c
 * consists all local, remote, and continuation routines:
 * local_xxx, remote_xxx, and cont_xxx.
 */

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <memory.h>
#include "prot_lock.h"
#include <nfs/nfs.h>
#include <nfs/nfssys.h>
#include <unistd.h>

remote_result nlm_result;		  /* local nlm result */
remote_result *nlm_resp = &nlm_result;	  /* ptr to klm result */
remote_result *get_res();

remote_result *remote_cancel();
remote_result *remote_lock();
remote_result *remote_test();
remote_result *remote_unlock();
remote_result *local_test();
remote_result *local_lock();
remote_result *local_unlock();
remote_result *cont_test();
remote_result *cont_lock();
remote_result *cont_unlock();
remote_result *cont_cancel();
remote_result *cont_reclaim();

extern msg_entry *retransmitted();

extern int debug, errno;
extern msg_entry *msg_q;
extern bool_t obj_cmp();
extern int obj_copy();
extern struct reclock *find_block_req();
extern void dequeue_block_req();
extern char	*xmalloc();
extern int	synccall;

struct fd_table {
	netobj	fh;
	int	fd;
	int	filemode;
	struct fd_table	*next;
	struct fd_table	*prev;
};

struct fd_table	*fd_table = NULL;
struct fd_table	*fd_freehead = NULL;

int used_fd;

print_fdtable()
{
	struct fd_table	*t;
	int i;

	if (debug)
		printf("In print_fdtable()....used_fd=%d\n", used_fd);

	for (t = fd_table; t; t = t->next) {
		if (t->fd) {
			printf("ID=%d\n", t->fd);
			for (i = 0; i < t->fh.n_len; i++) {
				printf("%02x", (t->fh.n_bytes[i] & 0xff));
			}
			printf("\n");
		}
	}
}

void
remove_fd(a)
	struct reclock *a;
{
	struct fd_table	*t;
	int i;

	if (debug)
		printf("In remove_fd() ...\n");

	for (t = fd_table; t; t = t->next) {
		if (t->fh.n_len && obj_cmp(&t->fh, &a->lck.fh) && t->fd) {
			if (debug) {
				for (i = 0; i < a->lck.fh.n_len; i++) {
					printf("%02x",
						(a->lck.fh.n_bytes[i] & 0xff));
				}
				printf("\n");
			}
			if (t == fd_table)	/* first one? */
				fd_table = fd_table->next;
			if (t->prev)
				t->prev->next = t->next;
			if (t->next)
				t->next->prev = t->prev;
			(void) memset(t->fh.n_bytes, 0, sizeof (t->fh.n_bytes));
			xfree(&(t->fh.n_bytes));
			t->fh.n_len = 0;
			t->fd = 0;
			t->next = fd_freehead;
			t->prev = NULL;
			if (fd_freehead)
				fd_freehead->prev = t;
			fd_freehead = t;
			used_fd--;
			break;
		}
	}
	if (debug)
		print_fdtable();
}

int
get_fd(a)
	struct reclock *a;
{
	struct fd_table	*t;
	int fd, cmd, i, ntries = 0;
	struct {
		char    *fh;
		int	filemode;
		int	*fd;
	} fa;

	if (debug) {
		printf("enter get_fd ....\n");
	}

	for (t = fd_table; t; t = t->next) {
		if (obj_cmp(&(t->fh), &(a->lck.fh)) && t->fd) {
			if (debug) {
				printf("Found fd entry : a = ");
				for (i = 0; i < a->lck.fh.n_len; i++) {
					printf("%02x",
						(a->lck.fh.n_bytes[i] & 0xff));
				}
				printf("\nfd_table->fh = ");
				for (i = 0; i < 32; i++) {
					printf("%02x",
						(t->fh.n_bytes[i] & 0xff));
				}
				printf("\n");
			}
			if(	t->filemode == O_RDONLY /* from RO filesys */
			&&	a->exclusive		/* write perm needed */
			){	return -2;
				/* XXX
				   Assumes no remount of the file system
				   to read-write.
				*/
			}
			return (t->fd);
		}
	}
	/*
	 * convert fh to fd
	 */
	cmd = NFS_CNVT;
	fa.fh = a->lck.fh.n_bytes;
	if (debug) {
		printf("Convert fd entry : ");
		for (i = 0; i < a->lck.fh.n_len; i++) {
			printf("%02x", (a->lck.fh.n_bytes[i] & 0xff));
		}
		printf("\n");
	}
	fa.filemode = O_RDWR;
	fa.fd = &fd;
again:
	if (_nfssys(cmd, &fa) == -1) {
		ntries++;
		if(	ntries < 2
		&&	errno == EROFS
		&&	!a->exclusive)
		{
			/* the write mode (of O_RDWR) not possible on
			   this file system; fortunately, not needed
			   for a non-exclusive lock (a.k.a., read-lock).
			 */
			fa.filemode = O_RDONLY;
			goto again;
		}
		syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
		       "get_fd", "_nfssys");
		syslog(LOG_ERR, gettxt(":127", "%s: cannot convert."),
		       "get_fd");
		if ((errno == ENOLCK) || (errno == ESTALE))
			return (-1);
		else
			return (-2);
	}

gotit:
	t = fd_freehead;
	if (fd_freehead) {
		fd_freehead = fd_freehead->next;
		if (fd_freehead)
			fd_freehead->prev = NULL;
	} else
		t = (struct fd_table *) xmalloc((u_int)
					sizeof(struct fd_table));
	
	obj_copy(&t->fh, &a->lck.fh);
	t->fd = fd;
	t->filemode = fa.filemode;
	t->next = fd_table;
	t->prev = NULL;
	if (fd_table)
		fd_table->prev = t;
	fd_table = t;
	used_fd++;

	if (debug)
		print_fdtable();
	return (fd_table->fd);
}

remote_result *
local_lock(a)
	struct reclock *a;
{
	int err, cmd;
	static struct flock ld;
	static int fd;

	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = denied;
		return (nlm_resp);
	}

	/*
	 * set the lock
	 */
	if (debug) {
		printf("enter local_lock...FD=%d\n", fd);
		pr_lock(a);
		(void) fflush(stdout);
	}
	if (a->block)
		cmd = F_RSETLKW;
	else
		cmd = F_RSETLK;
	if (a->exclusive)
		ld.l_type = F_WRLCK;
	else
		ld.l_type = F_RDLCK;
	ld.l_whence = 0;
	ld.l_start = a->lck.lox.base;
	ld.l_len = a->lck.lox.length;
	ld.l_pid = a->lck.lox.pid;
	ld.l_rpid = a->lck.lox.rpid;
	ld.l_sysid = a->lck.lox.rsys;
	ld.l_xxx = 0;
	if (debug) {
		printf("ld.l_start=%d ld.l_len=%d ld.l_rpid=%d ld.l_sysid=%x\n",
			ld.l_start, ld.l_len, ld.l_rpid, ld.l_sysid);
	}
	if ((err = fcntl(fd, cmd, &ld)) == -1) {
		if (errno == ELKBUSY) {
			nlm_resp->lstat = blocking;
			a->w_flag = 1;
		} else if (errno == EDEADLK) {
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "local_lock", "fcntl");
			nlm_resp->lstat = deadlck;
			a->w_flag = 0;
		} else if (errno == ENOLCK) {
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "local_lock", "fcntl");
			syslog(LOG_ERR, gettxt(":128", "%s: out of lock."),
			       "local_lock");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else if (((cmd == F_SETLK) || (cmd == F_RSETLK)) && (errno == EACCES)) {
			nlm_resp->lstat = denied;
		} else {
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "local_lock", "fcntl");
			syslog(LOG_ERR, gettxt(":129", "%s: cannot set a lock."),
			       "local_lock");
			nlm_resp->lstat = denied;
		}
	} else
		nlm_resp->lstat = nlm_granted;

	return (nlm_resp);
}

/*
 * choice == RPC; rpc calls to remote;
 * choice == MSG; msg passing calls to remote;
 */
remote_result *
remote_lock(a, choice)
	struct reclock *a;
	int choice;
{
	if (debug) {
		printf("enter remote_lock\n");
		pr_lock(a);
		(void) fflush(stdout);
	}

	if (choice == MSG) {	/* msg passing */
		if (nlm_call(NLM_LOCK_MSG, a, 0) == -1)
			a->rel = 1;
	}
	return(NULL);
}

remote_result *
local_unlock(a)
	struct reclock *a;
{
	int cmd;
	static struct flock ld;
	static int fd;

	if (debug)
		printf("enter local_unlock...................\n");

	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = nlm_denied;
		return (nlm_resp);
	}

	/*
	 * set the lock
	 */
	if (a->block)
		cmd = F_RSETLKW;
	else
		cmd = F_RSETLK;
	ld.l_type = F_UNLCK;
	ld.l_whence = 0;
	ld.l_start = a->lck.lox.base;
	ld.l_len = a->lck.lox.length;
	ld.l_pid = a->lck.lox.pid;
	ld.l_rpid = a->lck.lox.rpid;
	ld.l_sysid = a->lck.lox.rsys;
	ld.l_xxx = 0;
	if (debug) {
		printf("ld.l_start=%d ld.l_len=%d ld.l_pid=%d ld.l_rpid=%d ld.l_sysid=%x\n",
			ld.l_start, ld.l_len, ld.l_pid, ld.l_rpid, ld.l_sysid);
	}

	if (fcntl(fd, cmd, &ld) == -1) {
		if (errno == ELKBUSY) {
			nlm_resp->lstat = blocking;
			a->w_flag = 1;
		} else if (errno == ENOLCK) {
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "local_unlock", "fcntl");
			syslog(LOG_ERR, gettxt(":128", "%s: out of lock."),
			       "local_unlock");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else {
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "local_unlock", "fcntl");
			syslog(LOG_ERR, 
			       gettxt(":130", "%s: cannot unlock a lock."),
			       "local_unlock");
			nlm_resp->lstat = nlm_denied;
		}
	} else {
		/*
	 	 * Update fd table
	 	 */
		remove_fd(a);
		close(fd);
		nlm_resp->lstat = nlm_granted;

		/*
		 * retry for blocked locks
		 */
		synccall = 1;
		xtimer();
	}

	return (nlm_resp);
}

remote_result *
remote_unlock(a, choice)
	struct reclock *a;
	int choice;
{
	if (debug)
		printf("enter remote_unlock\n");

	if (choice == MSG) {
		if (nlm_call(NLM_UNLOCK_MSG, a, 0) == -1)
			a->rel = 1;	/* rpc error, discard */
	} else {
		syslog(LOG_ERR, gettxt(":131", "%s: rpc not supported"),
		       "remote_unlock");
		a->rel = 1;		/* rpc error, discard */
	}
	(void) remove_req_in_me(a);

	return (NULL);			/* no reply available */
}


remote_result *
local_test(a)
	struct reclock *a;
{
	int fd, cmd;
	struct flock ld;
	int	nofd = used_fd;

	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = nlm_denied;
		nlm_resp->lstat = nlm_denied;
		return (nlm_resp);
	}

	/*
	 * test the lock
	 */
	cmd = F_RGETLK;
	if (a->exclusive)
		ld.l_type = F_WRLCK;
	else
		ld.l_type = F_RDLCK;
	ld.l_whence = 0;
	ld.l_start = (a->lck.lox.base >= 0) ? a->lck.lox.base : 0;
	ld.l_len = a->lck.lox.length;
	ld.l_pid = a->lck.lox.pid;
	ld.l_rpid = a->lck.lox.rpid;
	ld.l_sysid = a->lck.lox.rsys;
	if (fcntl(fd, cmd, &ld) == -1) {
		if (errno == ELKBUSY) {
			nlm_resp->lstat = blocking;
			a->w_flag = 1;
		} else if (errno == ENOLCK) {
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "local_test", "fcntl");
			syslog(LOG_ERR, gettxt(":128", "%s: out of lock."),
			       "local_test");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else {
			syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
			       "local_test", "fcntl");
			syslog(LOG_ERR, 
			       gettxt(":132", "%s: cannot test a lock."),
			       "local_test");
			nlm_resp->lstat = nlm_denied;
		}
	} else {
		if (ld.l_type == F_UNLCK) {
			nlm_resp->lstat = nlm_granted;
			a->lck.lox.type = ld.l_type;
		} else {
			nlm_resp->lstat = nlm_denied;
			a->lck.lox.type = ld.l_type;
			a->lck.lox.base = ld.l_start;
			a->lck.lox.length = ld.l_len;
			a->lck.lox.pid = ld.l_pid;
			a->lck.lox.rpid = ld.l_rpid;
			a->lck.lox.rsys = ld.l_sysid;
		}
	}

	if (debug) {
		printf("ld.l_start=%d ld.l_len=%d ld.l_rpid=%d ld.l_sysid=%x\n",
			ld.l_start, ld.l_len, ld.l_rpid, ld.l_sysid);
	}

	if (nofd < used_fd) {
		remove_fd(a);
		close(fd);
	}
	return (nlm_resp);
}

remote_result *
remote_test(a, choice)
	struct reclock *a;
	int choice;
{
	if (debug)
		printf("enter remote_test\n");

	if (choice == MSG) {
		if (nlm_call(NLM_TEST_MSG, a, 0) == -1)
			a->rel = 1;
	} else {
		syslog(LOG_ERR, gettxt(":131", "%s: rpc not supported"),
		       "remote_test");
		a->rel = 1;
	}
	return (NULL);
}

remote_result *
remote_cancel(a, choice)
	struct reclock *a;
	int choice;
{
	msg_entry *msgp;

	if (debug)
		printf("enter remote_cancel(%x)\n", a);

	if (choice == MSG){
		if (nlm_call(NLM_CANCEL_MSG, a, 0) == -1)
			a->rel = 1;
	} else { /* rpc case */
		syslog(LOG_ERR, gettxt(":131", "%s: rpc not supported"),
		       "remote_cancel");
		a->rel = 1;
	}

	if ((msgp = retransmitted(a, KLM_LOCK)) != NULL) {
		/* msg is being processed */
		if (debug)
				printf("remove msg (%x) due to remote cancel\n",
				msgp->req);

		/* don't free the reclock here as remove_req_in_me() will do */
		/* that.						     */
		msgp->req->rel = 0;
		dequeue(msgp);
	}
	remove_req_in_me(a);

	return (NULL);
}

remote_result *
local_grant(a)
	struct reclock *a;
{
	msg_entry *msgp;
	remote_result *resp;
 
        if (debug)
                printf("enter local_grant(%x)...\n", a);
	msgp = msg_q;
        while (msgp != NULL) {

                if (same_bound(&(msgp->req->alock.lox), &(a->alock.lox)) && 
			same_type(&(msgp->req->alock.lox), &(a->alock.lox)) &&
			(msgp->req->alock.lox.pid == a->alock.lox.pid)) {

			/* upgrade request from pending to granted in */
			/* monitoring list for recovery		      */
			(void) upgrade_req_in_me(msgp->req);
			/*
			 * if the reply is for older request, set the
			 * reply result so that the nxt poll by the kernel
			 * will get this result
			 */
			if (msgp->reply != NULL) {
               			msgp->reply->lstat = klm_granted;
			/* if no reply is set before, lets save this reply */
			} else if ((resp = get_res()) != NULL) {
				resp->lstat = klm_granted;
				msgp->reply = resp;
			}
			break;
                }
                msgp = msgp->nxt;
        }
        nlm_resp->lstat = nlm_granted;
        return (nlm_resp);
}

remote_result *
remote_grant(a, choice)
	struct reclock *a;
	int choice;
{
	struct reclock *req;

	if (debug)
		printf("enter remote_grant...\n");

	/* reply to the granted lock that was queued in our blocking lock */
	/* list								  */
	if ((req = find_block_req(a)) != NULL) {
		if (choice == MSG) {
			if (nlm_call(NLM_GRANTED_MSG, req, 0) != -1)
				dequeue_block_req(req);
		} else
			syslog(LOG_ERR, gettxt(":131", "%s: rpc not supported"),
			       "remote_grant");
	}
	return (NULL);
}

remote_result *
cont_lock(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	if (debug) {
		printf("enter cont_lock (%x) ID=%d \n", a, a->lck.lox.LockID);
	}

	switch (resp->lstat) {
	case nlm_granted:
		a->rel = 0;
		if (add_mon(a, 1) == -1)
			syslog(LOG_ERR, gettxt(":114", "%s: %s failed"),
			       "cont_lock", "add_mon");
		return (resp);
	case denied:
	case nolocks:
		a->rel = 1;
		a->block = FALSE;
		a->lck.lox.granted = 0;
		return (resp);
	case deadlck:
		a->rel = 1;
		a->block = TRUE;
		a->lck.lox.granted = 0;
		return (resp);
	case blocking:
		a->rel = 0;
		a->w_flag = 1;
		a->block = TRUE;
		return (resp);
	case grace:
		a->rel = 0;
		release_res(resp);
		return (NULL);
	default:
		a->rel = 1;
		release_res(resp);
		syslog(LOG_ERR,
		       gettxt(":133", "%s: return value %d is not known"),
		       "cont_lock", resp->lstat);
		return (NULL);
	}
}


remote_result *
cont_unlock(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	if (debug)
		printf("enter cont_unlock\n");

	a->rel = 1;
	switch (resp->lstat) {
		case nlm_granted:
			return (resp);
		case denied:		/* impossible */
		case nolocks:
			return (resp);
		case blocking:		/* impossible */
			a->w_flag = 1;
			return (resp);
		case grace:
			a->rel = 0;
			release_res(resp);
			return (NULL);
		default:
			a->rel = 0;
			release_res(resp);
			syslog(LOG_ERR, 
			       gettxt(":133","%s: return value %d is not known"),
			       "cont_unlock", resp->lstat);
			return (NULL);
		}
}

remote_result *
cont_test(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	if (debug)
		printf("enter cont_test\n");

	a->rel = 1;
	switch (resp->lstat) {
	case grace:
		a->rel = 0;
		release_res(resp);
		return (NULL);
	case nlm_granted:
	case denied:
		if (debug)
			printf("lock blocked by %d, (%d, %d)\n",
				resp->lholder.svid, resp->lholder.l_offset,
				resp->lholder.l_len);
		return (resp);
	case nolocks:
		return (resp);
	case blocking:
		a->w_flag = 1;
		return (resp);
	default:
		syslog(LOG_ERR, 
		       gettxt(":133", "%s: return value %d is not known"),
		       "cont_test", resp->lstat);
		release_res(resp);
		return (NULL);
	}
}

remote_result *
cont_cancel(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	if (debug)
		printf("enter cont_cancel\n");

	return(cont_unlock(a, resp));
}

remote_result *
cont_reclaim(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	remote_result *local;

	if (debug)
		printf("enter cont_reclaim\n");
	switch (resp->lstat) {
	case nlm_granted:
	case denied:
	case nolocks:
	case blocking:
		local = resp;
		break;
	case grace:
		if (a->reclaim)
			syslog(LOG_ERR,
			       gettxt(":134", "%s: reclaim lock request (%x) is returned due to grace period, impossible!"), 
			       "cont_reclaim", a);
		local = NULL;
		break;
	default:
		syslog(LOG_ERR, 
		       gettxt(":133","%s: return value %d is not known"),
		       "cont_reclaim", resp->lstat);
		local = NULL;
		break;
	}

	if (local == NULL)
		release_res(resp);
	return (local);
}

/*ARGSUSED*/
remote_result *
cont_grant(a, resp)
        struct reclock *a;
        remote_result *resp;
{
        if (debug)
                printf("enter cont_grant...\n");
 
        return (resp);
}

