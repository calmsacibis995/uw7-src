#ident	"@(#)cplusinc:rpc/svc.h	1.4"

#ifndef _NET_RPC_SVC_H	/* wrapper symbol for kernel use */
#define _NET_RPC_SVC_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/svc.h	1.20"
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
 *	svc.h, server-side remote procedure call interface.
 *
 *	This interface must manage two items concerning remote
 *	procedure calling:
 *
 *	1) An arbitrary number of transport connections upon which
 *	rpc requests are received. They are created and registered
 *	by routines in svc_generic.c, svc_vc.c and svc_dg.c; they in
 *	turn call xprt_register and xprt_unregister.
 *
 *	2) An arbitrary number of locally registered services.
 *	Services are described by the following four data: program
 *	number, version number, "service dispatch" function, a
 *	transport handle, and a boolean that indicates whether or not
 *	the exported program should be registered with a local binder
 *	service; if true the program's number and version and the
 *	address from the transport handle are registered with the binder.
 *	These data are registered with rpcbind via svc_reg().
 *
 *	A service's dispatch function is called whenever an rpc request
 *	comes in on a transport. The request's program and version numbers
 *	must match those of the registered service. The dispatch function
 *	is passed two parameters, struct svc_req * and SVCXPRT *, defined
 *	below.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>			/* REQUIRED */
#include <net/rpc/types.h> 		/* REQUIRED */
#include <net/rpc/auth.h>		/* REQUIRED */
#include <fs/select.h>			/* REQUIRED */
#include <net/xti.h>			/* REQUIRED */
#include <net/netconfig.h>		/* REQUIRED */
#include <net/rpc/rpc_msg.h>		/* REQUIRED */
#include <net/ktli/t_kuser.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>			/* REQUIRED */
#include <rpc/types.h> 			/* REQUIRED */
#include <rpc/auth.h>			/* REQUIRED */
#include <sys/select.h>			/* REQUIRED */
#include <sys/xti.h>			/* REQUIRED */
#include <sys/netconfig.h>		/* REQUIRED */
#include <rpc/rpc_msg.h>		/* REQUIRED */
#include <sys/t_kuser.h>		/* REQUIRED */

#else

#include <rpc/rpc_com.h> 		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * transport status
 */
enum xprt_stat {
	XPRT_DIED,
	XPRT_MOREREQS,
	XPRT_IDLE
};

/*
 * Server side transport handle
 */
typedef struct __svcxprt {

#ifdef _KERNEL
	TIUSER		*xp_tiptr;	/* tli descriptor for kernel */
#else
	int		xp_fd;		/* file descriptor for user */
#define xp_sock		xp_fd
#endif

	u_short		xp_port;	/* associated port number.
					 * Obsoleted, but still used to
					 * specify whether rendezvouser
					 * or normal connection
					 */
	struct xp_ops {
#ifdef __STDC__
		bool_t	(*xp_recv)(struct __svcxprt *, struct rpc_msg *);
		enum xprt_stat (*xp_stat)(struct __svcxprt *);
		bool_t	(*xp_getargs)(const struct __svcxprt *,
				      const xdrproc_t, caddr_t);
		bool_t	(*xp_reply)(struct __svcxprt *, struct rpc_msg *);
		bool_t	(*xp_freeargs)(const struct __svcxprt *,
				       const xdrproc_t, caddr_t);
		void	(*xp_destroy)(struct __svcxprt *);
#else
		bool_t	(*xp_recv)();		/* receive incoming requests */
		enum xprt_stat (*xp_stat)();	/* get transport status */
		bool_t	(*xp_getargs)();	/* get arguments */
		bool_t	(*xp_reply)();		/* send reply */
		bool_t	(*xp_freeargs)();	/* free mem alloc'ed for args */
		void	(*xp_destroy)();	/* destroy this struct */
#endif
	} *xp_ops;

#ifndef _KERNEL
	int		xp_addrlen;	 /* length of remote addr. Obsoleted */
	char		*xp_tp;		 /* transport provider device name */
	char		*xp_netid;	 /* network token */
#endif

	struct netbuf	xp_ltaddr;	 /* local transport address */
	struct netbuf	xp_rtaddr;	 /* remote transport address */

#ifndef _KERNEL
	char		xp_raddr[16];	 /* remote address. Now obsoleted */
#endif

	struct opaque_auth xp_verf;	 /* raw response verifier */
	caddr_t		xp_p1;		 /* private: for use by svc ops */

#ifdef _KERNEL
	u_int		xp_p1len;	 /* size of p1 */
#endif
	caddr_t		xp_p2;		 /* private: for use by svc ops */
	caddr_t		xp_p3;		 /* private: for use by svc lib */

#ifndef _KERNEL
	int		xp_type;	 /* transport type */
#endif

} SVCXPRT;

/*
 * approved way of getting address of caller
 */
#define svc_getrpccaller(x) (&(x)->xp_rtaddr)

/*
 * Operations defined on an SVCXPRT handle
 *
 * SVCXPRT		*xprt;
 * struct rpc_msg	*msg;
 * xdrproc_t		 xargs;
 * caddr_t		 argsp;
 */
#define SVC_RECV(xprt, msg)				\
	(*(xprt)->xp_ops->xp_recv)((xprt), (msg))
#define svc_recv(xprt, msg)				\
	(*(xprt)->xp_ops->xp_recv)((xprt), (msg))

#define SVC_STAT(xprt)					\
	(*(xprt)->xp_ops->xp_stat)(xprt)
#define svc_stat(xprt)					\
	(*(xprt)->xp_ops->xp_stat)(xprt)

#define SVC_GETARGS(xprt, xargs, argsp)			\
	(*(xprt)->xp_ops->xp_getargs)((xprt), (xargs), (argsp))
#define svc_getargs(xprt, xargs, argsp)			\
	(*(xprt)->xp_ops->xp_getargs)((xprt), (xargs), (argsp))

#define SVC_REPLY(xprt, msg)				\
	(*(xprt)->xp_ops->xp_reply) ((xprt), (msg))
#define svc_reply(xprt, msg)				\
	(*(xprt)->xp_ops->xp_reply) ((xprt), (msg))

#define SVC_FREEARGS(xprt, xargs, argsp)		\
	(*(xprt)->xp_ops->xp_freeargs)((xprt), (xargs), (argsp))
#define svc_freeargs(xprt, xargs, argsp)		\
	(*(xprt)->xp_ops->xp_freeargs)((xprt), (xargs), (argsp))

#define SVC_DESTROY(xprt)				\
	(*(xprt)->xp_ops->xp_destroy)(xprt)
#define svc_destroy(xprt)				\
	(*(xprt)->xp_ops->xp_destroy)(xprt)

/*
 * service request
 */
struct svc_req {
	u_long		rq_prog;	/* service program number */
	u_long		rq_vers;	/* service protocol version */
	u_long		rq_proc;	/* the desired procedure */
	struct opaque_auth rq_cred;	/* raw creds from the wire */
	caddr_t		rq_clntcred;	/* read only cooked cred */
	SVCXPRT		*rq_xprt;	/* associated transport */
};

/*
 * Service registration
 *
 * svc_reg(xprt, prog, vers, dispatch, nconf)
 *	SVCXPRT *xprt;
 *	u_long prog;
 *	u_long vers;
 *	void (*dispatch)();
 *	struct netconfig *nconf;
 */

#ifdef __STDC__

extern	bool_t		svc_reg(const SVCXPRT *, u_long, u_long,
				void (*)(const struct svc_req *,
					 const SVCXPRT *),
				const struct netconfig *);	
#else

extern	bool_t		svc_reg();

#endif

/*
 * Service un-registration
 *
 * svc_unreg(prog, vers)
 *	u_long prog;
 *	u_long vers;
 */

#ifdef __STDC__

extern	void		svc_unreg (u_long, u_long);

#else

extern	void		svc_unreg ();

#endif

/*
 * transport registration.
 *
 * xprt_register(xprt)
 *	SVCXPRT *xprt;
 */

#ifdef __STDC__

extern	void		xprt_register (const SVCXPRT *);

#else

extern	void		xprt_register ();

#endif

/*
 * transport un-register
 *
 * xprt_unregister(xprt)
 *	SVCXPRT *xprt;
 */

#ifdef __STDC__

extern	void		xprt_unregister (const SVCXPRT *);

#else

extern	void		xprt_unregister ();

#endif

/*
 * When the service routine is called, it must first check to see if it
 * knows about the procedure; if not, it should call svcerr_noproc
 * and return. If so, it should deserialize its arguments via 
 * SVC_GETARGS (defined above). If the deserialization does not work,
 * svcerr_decode should be called followed by a return. Successful
 * decoding of the arguments should be followed the execution of the
 * procedure's code and a call to svc_sendreply.
 *
 * Also, if the service refuses to execute the procedure due to too-
 * weak authentication parameters, svcerr_weakauth should be called.
 * Note: do not confuse access-control failure with weak authentication!
 *
 * NB: In pure implementations of rpc, the caller always waits for a reply
 * msg. This message is sent when svc_sendreply is called. 
 * Therefore pure service implementations should always call
 * svc_sendreply even if the function logically returns void; use
 * xdr.h - xdr_void for the xdr routine. HOWEVER, connectionful rpc allows
 * for the abuse of pure rpc via batched calling or pipelining. In the
 * case of a batched call, svc_sendreply should NOT be called since
 * this would send a return message, which is what batching tries to avoid.
 * It is the service/protocol writer's responsibility to know which calls are
 * batched and which are not. Warning: responding to batch calls may
 * deadlock the caller and server processes!
 */

#ifdef __STDC__

extern	int		svc_auth_reg (int,
			enum auth_stat (*)(struct svc_req *, struct rpc_msg *));
extern	bool_t		svc_sendreply (const SVCXPRT *, const xdrproc_t,
				       caddr_t);
extern	void		svcerr_decode (const SVCXPRT *);
extern	void		svcerr_weakauth (const SVCXPRT *);
extern	void		svcerr_noproc (const SVCXPRT *);
extern	void		svcerr_progvers (const SVCXPRT *, u_long, u_long);
extern	void		svcerr_auth (const SVCXPRT *, enum auth_stat);
extern	void		svcerr_noprog (const SVCXPRT *);
extern	void		svcerr_systemerr (const SVCXPRT *);

#else

extern	int		svc_auth_reg ();
extern	bool_t		svc_sendreply ();
extern	void		svcerr_decode ();
extern	void		svcerr_weakauth ();
extern	void		svcerr_noproc ();
extern	void		svcerr_progvers ();
extern	void		svcerr_auth ();
extern	void		svcerr_noprog ();
extern	void		svcerr_systemerr();

#endif /* __STDC__ */

/*
 * lowest level dispatching -OR- who owns this process anyway.
 * Somebody has to wait for incoming requests and then call the correct
 * service routine. The routine svc_run does infinite waiting; i.e.,
 * svc_run never returns.
 *
 * Since another (co-existant) package may wish to selectively wait for
 * incoming calls or other events outside of the rpc architecture, the
 * routine svc_getreq is provided. It must be passed readfds, the
 * "in-place" results of a select call.
 */

#ifndef _KERNEL

/*
 * global keeper of rpc service descriptors in use
 * dynamic; must be inspected before each call to select 
 */
extern	fd_set		svc_fdset;
#define	svc_fds		svc_fdset.fds_bits[0]	/* compatibility */

#ifdef __STDC__

extern	void		svc_getreq (int);
extern	void		svc_getreqset (fd_set *);
extern	void		svc_getreq_common(int);
extern	void		svc_getreq_poll(struct pollfd*, int);
extern	void		svc_getreq_poll_parallel(struct pollfd *, int);
extern	void		svc_run (void);
extern	int		svc_run_parallel(int, int, int);

#else

extern	void		svc_getreqset();
extern	void		svc_getreqset();
extern	void		svc_getreq_common();
extern	void		svc_getreq_poll();
extern	void		svc_getreq_poll_parallel();
extern	void		svc_run();
extern	int		svc_run_parallel();
extern	void		rpctest_service();

#endif /* __STDC__ */

/*
 * transport independent svc_create routine.
 */

#ifdef __STDC__

extern	int		svc_create (void (*)(const struct svc_req *,
					     const SVCXPRT *),
				    u_long, u_long, const char *);
#else

extern	int		svc_create ();

#endif

/*
 * generic server creation routine. It takes a netconfig structure
 * instead of a nettype.
 */

#ifdef __STDC__

extern	SVCXPRT		*svc_tp_create (void (*)(const struct svc_req *,
						 const SVCXPRT *),
					u_long, u_long,
					const struct netconfig *);
#else

extern	SVCXPRT		*svc_tp_create ();

#endif

/*
 * generic TLI create routine
 */

#ifdef __STDC__

extern	SVCXPRT 	*svc_tli_create (int, const struct netconfig *,
					 const struct t_bind *, u_int, u_int);
#else

extern	SVCXPRT		*svc_tli_create ();

#endif

/*
 * connectionless and connectionful create routines
 */

#ifdef __STDC__

extern	SVCXPRT		*svc_vc_create (int, u_int, u_int);
extern	SVCXPRT		*svc_dg_create (int, u_int, u_int);

#else

extern	SVCXPRT		*svc_vc_create ();
extern	SVCXPRT		*svc_dg_create ();

#endif

/*
 * the routine takes any *open* TLI file
 * descriptor as its first input and is used for open connections.
 */

#ifdef __STDC__

extern	SVCXPRT		*svc_fd_create (int, u_int, u_int);

#else

extern	SVCXPRT		*svc_fd_create ();

#endif

/*
 * memory based rpc (for speed check and testing)
 */

#if __STDC__

extern	SVCXPRT		*svc_raw_create (void);

#else

extern	SVCXPRT		*svc_raw_create ();

#endif

#ifdef __STDC__

extern	int		rpc_reg(u_long, u_long, u_long,	char * (*)(caddr_t),
				const xdrproc_t, const xdrproc_t,
				const char *);

#else /* !__STDC__ */

extern int	rpc_reg();

#endif /* __STDC__ */


#ifdef PORTMAP

#include <rpc/svc_soc.h>	/* SVR4.0COMPAT */

#endif

#else	/* !_KERNEL */

/*
 * kernel based rpc
 */
extern	int	svc_tli_kcreate(struct file *, u_int, SVCXPRT **);
extern	bool_t	svc_register(u_long, u_long, dev_t, void (*)());
extern	void	svc_unregister(u_long, u_long, dev_t);
extern	void	svc_clts_kdupsave(struct svc_req *, int);
extern	int	svc_clts_kdup(struct svc_req *, caddr_t, int);
extern	void	svc_clts_kdupdone(struct svc_req *, caddr_t, int);
extern	int	svc_clts_kcreate(TIUSER *, u_int, SVCXPRT **);

#define	DUP_INPROGRESS		0x01
#define	DUP_DONE		0x02

#define	RQCRED_SIZE		400

#define	SVC_SIG_IGNORE		0x01
#define	SVC_SIG_CATCH		0x02

#define	svc_getcaller(x)	(&(x)->xp_rtaddr.buf)

/*
 * server parameters for dynamic lwp support
 */
struct	svc_param {
	lock_t			sp_lock;	/* lock to protect the struct */
	int			sp_timeout;	/* lwp existance after no work*/
	int			sp_existing;	/* total curr existing lwps */
	int			sp_serving;	/* total curr serving lwps */
	int			sp_max;		/* max # lwps for this proto */
	int			sp_min;		/* min # lwps for this proto */
	int			sp_iscreating;	/* is an lwp being created ? */
	struct	file		*sp_fp;		/* file pointer of xprt */
	void			(*sp_create)();	/* create routine */
	void			(*sp_delete)();	/* delete routine */
};

#endif /* !_KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_SVC_H */
