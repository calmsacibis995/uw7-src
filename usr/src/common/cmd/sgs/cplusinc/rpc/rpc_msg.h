#ident	"@(#)cplusinc:rpc/rpc_msg.h	1.3"

#ifndef _NET_RPC_RPC_MSG_H	/* wrapper symbol for kernel use */
#define _NET_RPC_RPC_MSG_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/rpc_msg.h	1.16"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	rpc_msg.h, remote procedure call message definition.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/rpc/types.h> 	/* REQUIRED */
#include <net/rpc/xdr.h>	/* REQUIRED */
#include <net/rpc/auth.h>	/* REQUIRED */
#include <net/rpc/clnt.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h> 		/* REQUIRED */
#include <rpc/xdr.h>		/* REQUIRED */
#include <rpc/auth.h>		/* REQUIRED */
#include <rpc/clnt.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define RPC_MSG_VERSION		((u_long) 2)
#define RPC_SERVICE_PORT	((u_short) 2048)

/*
 * Bottom up definition of an rpc message.
 * NOTE: call and reply use the same overall
 * struct but * different parts of unions
 * within it.
 */

enum msg_type {
	CALL=0,
	REPLY=1
};

enum reply_stat {
	MSG_ACCEPTED=0,
	MSG_DENIED=1
};

enum accept_stat {
	SUCCESS=0,
	PROG_UNAVAIL=1,
	PROG_MISMATCH=2,
	PROC_UNAVAIL=3,
	GARBAGE_ARGS=4,
	SYSTEM_ERR=5
};

enum reject_stat {
	RPC_MISMATCH=0,
	AUTH_ERROR=1
};

/*
 * Reply part of an rpc exchange
 */

/*
 * Reply to an rpc request that was accepted by the server.
 * Note: there could be an error even though the request was
 * accepted.
 */
struct accepted_reply {
	struct opaque_auth		ar_verf;
	enum accept_stat		ar_stat;
	union {
		struct {
			u_long		low;
			u_long		high;
		} AR_versions;
		struct {
			caddr_t		where;
			xdrproc_t	proc;
		} AR_results;
		/*
		 * and many other null cases
		 */
	} ru;
#define	ar_results	ru.AR_results
#define	ar_vers		ru.AR_versions
};

/*
 * Reply to an rpc request that was rejected by the server.
 */
struct rejected_reply {
	enum reject_stat		rj_stat;
	union {
		struct {
			u_long		low;
			u_long		high;
		} RJ_versions;
		enum auth_stat		RJ_why;	/* why auth did not work */
	} ru;
#define	rj_vers	ru.RJ_versions
#define	rj_why	ru.RJ_why
};

/*
 * Body of a reply to an rpc request.
 */
struct reply_body {
	enum reply_stat rp_stat;
	union {
		struct accepted_reply RP_ar;
		struct rejected_reply RP_dr;
	} ru;
#define	rp_acpt	ru.RP_ar
#define	rp_rjct	ru.RP_dr
};

/*
 * Body of an rpc request call.
 */
struct call_body {
	u_long cb_rpcvers;		/* must be equal to 2 */
	u_long cb_prog;
	u_long cb_vers;
	u_long cb_proc;
	struct opaque_auth cb_cred;
	struct opaque_auth cb_verf; 	/* protocol specific, provided by client */
};

/*
 * The rpc message
 */
struct rpc_msg {
	u_long			rm_xid;
	enum msg_type		rm_direction;
	union {
		struct call_body RM_cmb;
		struct reply_body RM_rmb;
	} ru;
#define	rm_call		ru.RM_cmb
#define	rm_reply	ru.RM_rmb
};
#define	acpted_rply	ru.RM_rmb.ru.RP_ar
#define	rjcted_rply	ru.RM_rmb.ru.RP_dr


/*
 * XDR routines to handle a rpc message, to pre-serialize
 * the static part of a rpc message, and to handle a rpc reply.
 * also, routine which fills in the error part of a reply message.
 */

#ifdef __STDC__

extern bool_t	xdr_accepted_reply(XDR *, const struct accepted_reply *);
extern bool_t	xdr_callmsg (XDR *, const struct rpc_msg *);
extern bool_t	xdr_callhdr (XDR *, const struct rpc_msg *);
extern bool_t	xdr_opaque_auth(XDR *, const struct opaque_auth *);
extern bool_t	xdr_rejected_reply(XDR *, const struct rejected_reply *);
extern bool_t	xdr_replymsg (XDR *, const struct rpc_msg *);
extern void	_seterr_reply (struct rpc_msg *, struct rpc_err *);

#else

extern bool_t	xdr_accepted_reply();
extern bool_t	xdr_callmsg ();
extern bool_t	xdr_callhdr ();
extern bool_t	xdr_opaque_auth();
extern bool_t	xdr_rejected_reply();
extern bool_t	xdr_replymsg ();
extern void	_seterr_reply ();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_RPC_MSG_H */
