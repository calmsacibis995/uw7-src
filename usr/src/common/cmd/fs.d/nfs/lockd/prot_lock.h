/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)prot_lock.h	1.2"
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

#include <rpc/rpc.h>
#include <syslog.h>
#include "nlm_prot.h"
#include "klm_prot.h"
#include "lockf.h"

typedef struct nlm_testres remote_result;
#define lstat stat.stat
#define lholder stat.nlm_testrply_u.holder

/*
 * extended flock info
 * l_xxx and l_rpid are also defined in flock.h.  Be sure to
 * change flock.h if any of them get changed.
 */
#define l_xxx   l_pad[0]
#define l_rpid  l_pad[1]

/*
 * Flock call.
 * used to be in /usr/include/sys/file.h
 */
#define LOCK_SH         1       /* shared lock */
#define LOCK_EX         2       /* exclusive lock */
#define LOCK_NB         4       /* don't block when locking */
#define LOCK_UN         8       /* unlock */

#define NLM_LOCK_RECLAIM	16
#define MSG 	0		/* choices of comm to remote svr */
#define RPC 	1		/* choices of comm to remote svr */

#define MAXLEN		0x7fffffff
#define lck 		alock
#define svr		server_name
#define caller		caller_name
#define clnt		clnt_name
#define fh_length		fh.n_len
#define fh_bytes	fh.n_bytes
#define oh_len		oh.n_len
#define oh_bytes	oh.n_bytes
#define cookie_len	cookie.n_len
#define cookie_bytes	cookie.n_bytes

/*
#define granted		nlm_granted
*/
#define denied		nlm_denied
#define nolocks 	nlm_denied_nolocks
#define blocking	nlm_blocked
#define grace		nlm_denied_grace_period
#define deadlck		nlm_deadlck
#define rpc_error	6

/*
 * warning:  struct alock consists of klm_lock and nlm_lock,
 * it has to be modified if either structure has been modified!!!
 */
struct alock {
	netobj cookie;

	/* from klm_prot.h */
        char *server_name;
        netobj fh;

	/* from lockf.h */
	struct data_lock lox;

	/* addition from nlm_prot.h */
	char *caller_name;
	netobj oh;
	int svid;
	u_int l_offset;
        u_int l_len;

	/* addition from lock manager */
	char *clnt_name;
	int op;
};


struct reclock {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct alock alock;
	bool_t reclaim;
	int state;

	/* auxiliary structure */
	SVCXPRT *transp; /* transport handle for delayed response due to */ 
			 /* blocking or grace period */
	int rel;
	int w_flag;

	struct reclock *prev; 
	struct reclock *next;
};
typedef struct reclock reclock;

/*
 * Lock manager vp->v_filocks
 */
struct lm_vnode {
        char *server_name;
        netobj fh;
        struct reclock *exclusive;	/* exclusive locks */
	struct reclock *shared;	/* shared locks */
	struct reclock *pending;	/* pending locks */
	struct lm_vnode *prev;
	struct lm_vnode *next;
	struct reclock *reclox;
	int    rel;
};
typedef struct lm_vnode lm_vnode;

struct timer {
	/* timer goes off when exp == curr */
	int exp;
	int curr;
};
typedef struct timer timer;

/*
 * msg passing structure
 */
struct msg_entry {
	reclock *req;
	remote_result *reply;
	timer t;
	int proc; /* procedure name that req is sent to; needed for reply purpose */
	struct msg_entry *prev;
	struct msg_entry *nxt;
};
typedef struct msg_entry msg_entry;

struct priv_struct {
	int pid;
	int *priv_ptr;
};
