#ident	"@(#)cplusinc:rpc/clnt.h	1.3"

#ifndef _NET_RPC_CLNT_H	/* wrapper symbol for kernel use */
#define _NET_RPC_CLNT_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/clnt.h	1.27"
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
 *	clnt.h, client side remote procedure call interface.
 *
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/rpc/types.h> 	/* REQUIRED */
#include <net/rpc/auth.h>	/* REQUIRED */
#include <net/xti.h>		/* REQUIRED */
#include <proc/cred.h>		/* REQUIRED */
#include <net/ktli/t_kuser.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h>		/* REQUIRED */
#include <rpc/auth.h>		/* REQUIRED */
#include <sys/xti.h>		/* REQUIRED */
#include <sys/cred.h>		/* REQUIRED */
#include <sys/t_kuser.h>	/* REQUIRED */

#else

#include <sys/types.h>		/* SVR4.0COMPAT */
#include <rpc/rpc_com.h>	/* SVR4.0COMPAT */
#include <rpc/xdr.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * Rpc calls return an enum clnt_stat.
 * This should be looked at more, since
 * each implementation is required to
 * live with this (implementation
 * independent) list of errors.
 */

enum clnt_stat {
	RPC_SUCCESS=0,			/* call succeeded */
	/*
	 * local errors
	 */
	RPC_CANTENCODEARGS=1,		/* can't encode arguments */
	RPC_CANTDECODERES=2,		/* can't decode results */
	RPC_CANTSEND=3,			/* failure in sending call */
	RPC_CANTRECV=4,			/* failure in receiving result */
	RPC_TIMEDOUT=5,			/* call timed out */
	RPC_INTR=18,			/* call interrupted */
	RPC_UDERROR=23,			/* recv got uderr indication */
	/*
	 * remote errors
	 */
	RPC_VERSMISMATCH=6,		/* rpc versions not compatible */
	RPC_AUTHERROR=7,		/* authentication error */
	RPC_PROGUNAVAIL=8,		/* program not available */
	RPC_PROGVERSMISMATCH=9,		/* program version mismatched */
	RPC_PROCUNAVAIL=10,		/* procedure unavailable */
	RPC_CANTDECODEARGS=11,		/* decode arguments error */
	RPC_SYSTEMERROR=12,		/* generic "other problem" */
	/*
	 * rpc_call & clnt_create errors
	 */
	RPC_UNKNOWNHOST=13,		/* unknown host name */
	RPC_UNKNOWNPROTO=17,		/* unknown protocol */
	RPC_UNKNOWNADDR=19,		/* Remote address unknown */
	RPC_NOBROADCAST=21,		/* Broadcasting not supported */
	/*
	 * rpcbind errors
	 */
	RPC_RPCBFAILURE=14,		/* the pmapper failed in its call */
#define RPC_PMAPFAILURE RPC_RPCBFAILURE
	RPC_PROGNOTREGISTERED=15,	/* remote program is not registered */
	RPC_N2AXLATEFAILURE=22,		/* Name to address translation failed */
	/*
	 * Misc error in the TLI library
	 */
	RPC_TLIERROR=20,
	/*
	 * unspecified error
	 */
	RPC_FAILED=16
};

/*
 * Error info. for a rpc call.
 */
struct rpc_err {
	enum clnt_stat re_status;	/* error status, see above */
	union {
		struct {
			int RE_errno;	/* related system error */
			int RE_t_errno;	/* related tli error number */
		} RE_err;
		enum auth_stat RE_why;	/* why the auth error occurred */
		struct {
			u_long low;	/* lowest verion supported */
			u_long high;	/* highest verion supported */
		} RE_vers;
		struct {		/* maybe meaningful if RPC_FAILED */
			long s1;
			long s2;
		} RE_lb;		/* life boot & debugging only */
	} ru;
#define	re_errno	ru.RE_err.RE_errno
#define	re_terrno	ru.RE_err.RE_t_errno
#define	re_why		ru.RE_why
#define	re_vers		ru.RE_vers
#define	re_lb		ru.RE_lb
};


/*
 * Client rpc handle. It is created by
 * individual implementations. The client
 * is responsible for initializing auth.
 */
typedef struct __client {
	AUTH	*cl_auth;			/* client auth. handle */
	struct clnt_ops {

#ifdef __STDC__

#ifdef _KERNEL
		enum clnt_stat	(*cl_call)(struct __client *, u_long,
					xdrproc_t, caddr_t, xdrproc_t,
					caddr_t, struct timeval,
					struct netbuf *, u_int, int);
#else
		enum clnt_stat	(*cl_call)(struct __client *, const u_long,
					   const xdrproc_t, caddr_t,
					   const xdrproc_t, caddr_t,
					   const struct timeval);
#endif
		void		(*cl_abort)(struct __client *);
		void		(*cl_geterr)(const struct __client *,
					struct rpc_err *);
		bool_t		(*cl_freeres)(struct __client *,
					      const xdrproc_t, caddr_t);
		void		(*cl_destroy)(struct __client *);
		bool_t		(*cl_control)(struct __client *, uint, char *);

#else

		enum clnt_stat	(*cl_call)();	/* call remote procedure */
		void		(*cl_abort)();	/* abort a call */
		void		(*cl_geterr)();	/* get specific error code */
		bool_t		(*cl_freeres)();/* free results */
		void		(*cl_destroy)();/* destroy this structure */
		bool_t		(*cl_control)();/* the ioctl() of rpc */

#endif

	} *cl_ops;
	caddr_t			cl_private;	/* private stuff */

#ifndef _KERNEL

	char			*cl_netid;	/* network token */
	char			*cl_tp;		/* device name */

#else

	sv_t			cl_sv;		/* CLIENT sync. var */

#endif
} CLIENT;

/*
 * Timers used for the pseudo-transport
 * protocol when using datagrams.
 */
struct rpc_timers {
	u_short		rt_srtt;	/* smoothed round-trip time */
	u_short		rt_deviate;	/* estimated deviation */
	u_long		rt_rtxcur;	/* current (backed-off) rto */
};

/*
 * Feedback values used for possible
 * congestion and rate control.
 */
#define	FEEDBACK_REXMIT1	1	/* first retransmit */
#define	FEEDBACK_OK		2	/* no retransmits */

#define	RPCSMALLMSGSIZE		400	/* a more reasonable packet size */

#define	KNC_STRSIZE		128	/* maximum len of knetconfig strings */

/*
 * Kernel netconfig struct, holds a netconfig
 * entry describing a transport.
 */
struct knetconfig {
	unsigned long	knc_semantics;	/* token name */
	char		*knc_protofmly;	/* protocol family */
	char		*knc_proto;	/* protocol */
	dev_t		knc_rdev;	/* device id */
	unsigned long	knc_unused[8];	/* unused */
};

#ifdef	_KERNEL

#define	CLNT_CALL(rh, proc, xargs, argsp, xres, resp, secs, sin,	    \
			pre4dot0, poll_type)				    \
	((*(rh)->cl_ops->cl_call)(rh, proc, xargs, argsp, xres, resp,	    \
			secs, sin, pre4dot0, poll_type))

#define	clnt_call(rh, proc, xargs, argsp, xres, resp, secs, sin,	    \
			pre4dot0, poll_type)				    \
	((*(rh)->cl_ops->cl_call)(rh, proc, xargs, argsp, xres, resp,	    \
			secs, sin, pre4dot0, poll_type))

#else

/*
 * enum clnt_stat
 * CLNT_CALL(rh, proc, xargs, argsp, xres, resp, timeout)
 * 	CLIENT *rh;
 *	u_long proc;
 *	xdrproc_t xargs;
 *	caddr_t argsp;
 *	xdrproc_t xres;
 *	caddr_t resp;
 *	struct timeval timeout;
 */
#define	CLNT_CALL(rh, proc, xargs, argsp, xres, resp, secs)	\
	((*(rh)->cl_ops->cl_call)(rh, proc, xargs, argsp, xres, resp, secs))
#define	clnt_call(rh, proc, xargs, argsp, xres, resp, secs)	\
	((*(rh)->cl_ops->cl_call)(rh, proc, xargs, argsp, xres, resp, secs))

#endif

/*
 * void
 * CLNT_ABORT(rh);
 * 	CLIENT *rh;
 */
#define	CLNT_ABORT(rh)	((*(rh)->cl_ops->cl_abort)(rh))
#define	clnt_abort(rh)	((*(rh)->cl_ops->cl_abort)(rh))

/*
 * struct rpc_err
 * CLNT_GETERR(rh);
 * 	CLIENT *rh;
 */
#define	CLNT_GETERR(rh, errp)	((*(rh)->cl_ops->cl_geterr)(rh, errp))
#define	clnt_geterr(rh, errp)	((*(rh)->cl_ops->cl_geterr)(rh, errp))

/*
 * bool_t
 * CLNT_FREERES(rh, xres, resp);
 * 	CLIENT *rh;
 *	xdrproc_t xres;
 *	caddr_t resp;
 */
#define	CLNT_FREERES(rh,xres,resp) ((*(rh)->cl_ops->cl_freeres)(rh,xres,resp))
#define	clnt_freeres(rh,xres,resp) ((*(rh)->cl_ops->cl_freeres)(rh,xres,resp))

/*
 * bool_t
 * CLNT_CONTROL(cl, request, info)
 *	CLIENT *cl;
 *	u_int request;
 *	char *info;
 */
#define	CLNT_CONTROL(cl, rq, in) ((*(cl)->cl_ops->cl_control)(cl, rq, in))
#define	clnt_control(cl, rq, in) ((*(cl)->cl_ops->cl_control)(cl, rq, in))


/*
 * Control operations that apply to all transports.
 */
#define	CLSET_TIMEOUT		1	/* set timeout (timeval) */
#define	CLGET_TIMEOUT		2	/* get timeout (timeval) */
#define	CLGET_SERVER_ADDR	3	/* get server's address (sockaddr) */
#define	CLGET_FD		6	/* get connections file descriptor */
#define	CLGET_SVC_ADDR		7	/* get server's address (netbuf) */
#define	CLSET_FD_CLOSE		8	/* close fd while clnt_destroy */
#define	CLSET_FD_NCLOSE		9	/* Do not close fd while clnt_destroy */
#define CLGET_XID		10	/* Get xid */
#define CLSET_XID		11	/* Set xid */
#define CLGET_VERS		12	/* Get version number */
#define CLSET_VERS		13	/* Set version number */

/*
 * Connectionless only control operations.
 */
#define	CLSET_RETRY_TIMEOUT 4		/* set retry timeout (timeval) */
#define	CLGET_RETRY_TIMEOUT 5		/* get retry timeout (timeval) */

/*
 * void
 * CLNT_DESTROY(rh);
 * 	CLIENT *rh;
 */
#define	CLNT_DESTROY(rh)	((*(rh)->cl_ops->cl_destroy)(rh))
#define	clnt_destroy(rh)	((*(rh)->cl_ops->cl_destroy)(rh))


/*
 * RPCTEST is a test program which is accessable on every rpc
 * transport/port. It is used for testing, performance evaluation,
 * and network administration.
 */

#define	RPCTEST_PROGRAM		((u_long)1)
#define	RPCTEST_VERSION		((u_long)1)
#define	RPCTEST_NULL_PROC	((u_long)2)
#define	RPCTEST_NULL_BATCH_PROC	((u_long)3)

/*
 * by convention, procedure 0 takes null arguments and returns them
 */
#define	NULLPROC		((u_long)0)

/*
 * Below are the client handle creation routines for the various
 * implementations of client side rpc. They can return NULL if a
 * creation failure occurs.
 */

#ifndef _KERNEL

/*
 * Generic client creation routine. Supported protocols are which belong
 * to the nettype name space
 */

#ifdef __STDC__

extern	CLIENT		*clnt_create(const char *, u_long, u_long,
				     const char *);
/*
 *
 * 	char *hostname;			-- hostname
 *	u_long prog;			-- program number
 *	u_long vers;			-- version number
 *	char *nettype;			-- network type
 */

#else

extern	CLIENT		*clnt_create();

#endif

/*
 * generic client creation routine. Supported protocols are which belong
 * to the nettype name space.
 */

#ifdef __STDC__

extern	CLIENT		*clnt_create_vers(const char *, u_long, u_long *,
					  u_long, u_long, const char *);
/*
 *	char *host;		-- hostname
 *	u_long prog;		-- program number
 *	u_long *vers_out;	-- servers highest available version number
 *	u_long vers_low;	-- low version number
 *	u_long vers_high;	-- high version number
 *	char *nettype;		-- network type
 */

#else

extern	CLIENT		*clnt_create_vers();

#endif

/*
 * generic client creation routine. It takes a netconfig structure
 * instead of nettype
 */

#ifdef __STDC__

extern	CLIENT		*clnt_tp_create(const char *, u_long, u_long,
					const struct netconfig *);
/*
 *	char *hostname;			-- hostname
 *	u_long prog;			-- program number
 *	u_long vers;			-- version number
 *	struct netconfig *netconf; 	-- network config structure
 */

#else

extern CLIENT		*clnt_tp_create();

#endif

/*
 * generic TLI create routine
 */

#ifdef __STDC__

extern	CLIENT		*clnt_tli_create(int, const struct netconfig *,
					 const struct netbuf *, u_long, u_long,
					 u_int, u_int);
/*
 *	register int fd;		-- fd
 *	struct netconfig *nconf;	-- netconfig structure
 *	struct netbuf *svcaddr;		-- servers address
 *	u_long prog;			-- program number
 *	u_long vers;			-- version number
 *	u_int sendsz;			-- send size
 *	u_int recvsz;			-- recv size
 */

#else

extern	CLIENT		*clnt_tli_create();

#endif

/*
 * low level clnt create routine for connectionful transports, e.g. tcp.
 */

#ifdef __STDC__

extern	CLIENT		*clnt_vc_create(int, const struct netbuf *, u_long,
					u_long, u_int, u_int);
/*
 *	int fd;				-- open file descriptor
 *	struct netbuf *svcaddr;		-- servers address
 *	u_long prog;			-- program number
 *	u_long vers;			-- version number
 *	u_int sendsz;			-- buffer recv size
 *	u_int recvsz;			-- buffer send size
 */

#else

extern	CLIENT		*clnt_vc_create();

#endif

/*
 * low level clnt create routine for connectionless transport.
 */

#ifdef __STDC__

extern	CLIENT		*clnt_dg_create(int, const struct netbuf *, u_long,
					u_long, u_int, u_int);
/*
 *	int fd;				-- open file descriptor
 *	struct netbuf *svcaddr;		-- servers address
 *	u_long program;			-- program number
 *	u_long version;			-- version number
 *	u_int sendsz;			-- buffer recv size
 *	u_int recvsz;			-- buffer send size
 */

#else

extern	CLIENT		*clnt_dg_create();

#endif

/*
 * memory based rpc (for speed check and testing)
 * CLIENT *
 * clnt_raw_create(prog, vers)
 *	u_long prog;			-- program number
 *	u_long vers;			-- version number
 */

#ifdef __STDC__

extern	CLIENT		*clnt_raw_create(u_long, u_long);

#else

extern	CLIENT		*clnt_raw_create ();

#endif

/*
 * print why creation failed
 */

#ifdef __STDC__

extern	void		clnt_pcreateerror(const char *);
extern	char		*clnt_spcreateerror (const char *);

#else

extern	void		clnt_pcreateerror();
extern	char		*clnt_spcreateerror();

#endif

/*
 * like clnt_perror(), but is more verbose in its output
 */

#ifdef __STDC__

extern	void		clnt_perrno(enum clnt_stat);

#else

void clnt_perrno ();

#endif

/*
 * print an error message, given the client error code
 */

#ifdef __STDC__

extern	void		clnt_perror(const CLIENT *, const char *);
extern	char		*clnt_sperror (const CLIENT *, const char *);

#else

extern	void		clnt_perror();
extern	char		*clnt_sperror();

#endif

/*
 * if a creation fails, the following allows the user to figure out why.
 */
typedef struct {
	enum	clnt_stat	cf_stat;
	/*
	 * useful when cf_stat == RPC_PMAPFAILURE 
	 */
	struct	rpc_err		cf_error;
} rpc_createerr_t;

int				set_rpc_createerr(rpc_createerr_t);
rpc_createerr_t			get_rpc_createerr(void);

#ifdef _REENTRANT

const rpc_createerr_t		*_rpc_createerr(void);

#define	rpc_createerr		(*_rpc_createerr())

#else

extern	rpc_createerr_t		rpc_createerr;

#endif /* _REENTRANT */

/*
 * the simplified interface:
 * enum clnt_stat
 * rpc_call(host, prognum, versnum, procnum, inproc, in, outproc, out, nettype)
 *	char *host;
 *	u_long prognum, versnum, procnum;
 *	xdrproc_t inproc, outproc;
 *	char *in, *out;
 *	char *nettype;
 */

#ifdef __STDC__

extern	enum clnt_stat		rpc_call(const char *, u_long, u_long, u_long,
					 const xdrproc_t, const char *,
					 const xdrproc_t, char *,
					 const char *);
#else

extern	enum clnt_stat		rpc_call();

#endif

/*
 * RPC broadcast interface
 * The call is broadcasted to all locally connected nets.
 *
 * extern enum clnt_stat
 * rpc_broadcast(prog, vers, proc, xargs, argsp, xresults, resultsp,
 *			eachresult, nettype)
 *	u_long		prog;		-- program number
 *	u_long		vers;		-- version number
 *	u_long		proc;		-- procedure number
 *	xdrproc_t	xargs;		-- xdr routine for args
 *	caddr_t		argsp;		-- pointer to args
 *	xdrproc_t	xresults;	-- xdr routine for results
 *	caddr_t		resultsp;	-- pointer to results
 *	resultproc_t	eachresult;	-- call with each result obtained
 *	char		*nettype;	-- Transport type
 *
 * For each valid response received, the procedure eachresult is called.
 * Its form is:
 *		done = eachresult(resp, raddr, nconf)
 *			bool_t done;
 *			caddr_t resp;
 *			struct netbuf *raddr;
 *			struct netconfig *nconf;
 * where resp points to the results of the call and raddr is the
 * address if the responder to the broadcast. nconf is the transport
 * on which the response was received.
 *
 * extern enum clnt_stat
 * rpc_broadcast_exp(prog, vers, proc, xargs, argsp, xresults, resultsp,
 *			eachresult, inittime, waittime, nettype)
 *	u_long		prog;		-- program number
 *	u_long		vers;		-- version number
 *	u_long		proc;		-- procedure number
 *	xdrproc_t	xargs;		-- xdr routine for args
 *	caddr_t		argsp;		-- pointer to args
 *	xdrproc_t	xresults;	-- xdr routine for results
 *	caddr_t		resultsp;	-- pointer to results
 *	resultproc_t	eachresult;	-- call with each result obtained
 *	int 		inittime;	-- how long to wait initially
 *	int 		waittime;	-- maximum time to wait
 *	char		*nettype;	-- Transport type
 */

#ifdef __STDC__

typedef	bool_t		(*resultproc_t)(caddr_t, ... /* for backward compat */);
extern	enum clnt_stat	rpc_broadcast(u_long, u_long, u_long,
				      const xdrproc_t, caddr_t,
				      const xdrproc_t, caddr_t,
				      const resultproc_t, const char *);
extern	enum clnt_stat	rpc_broadcast_exp(u_long, u_long, u_long,
					  const xdrproc_t, caddr_t,
					  const xdrproc_t, caddr_t,
					  const resultproc_t, int, int,
					  const char *);

#else

typedef	bool_t		(*resultproc_t)();
extern	enum clnt_stat	rpc_broadcast();
extern	enum clnt_stat	rpc_broadcast_exp();

#endif

#endif /* !_KERNEL */

/*
 * copy error message to buffer.
 */

#ifdef __STDC__

extern	const char	*clnt_sperrno(enum clnt_stat);

#else

extern	char		*clnt_sperrno();

#endif

#ifdef PORTMAP

/*
 * for backward compatibility
 */
#include <rpc/clnt_soc.h>		/* SVR4.0COMPAT */

#endif

#ifdef _KERNEL

/*
 * connectionless mode internal kernel routines
 */

extern	void		clnt_clts_reopen(CLIENT *, u_long, u_long,
				struct knetconfig *);
extern	void		clnt_clts_setxid(CLIENT *, u_long);
extern	int		clnt_clts_settimers(CLIENT *, struct rpc_timers *,
				struct rpc_timers *, unsigned int, void (*)(),
				caddr_t);
extern	void		clnt_clts_init(CLIENT *, struct netbuf *, int,
				struct cred *);
extern	int		clnt_clts_kcreate(TIUSER *, dev_t, struct netbuf *,
				u_long, u_long, u_int, int, struct cred *,
				CLIENT **);
extern	int		clnt_tli_kcreate(struct knetconfig *, struct netbuf *,
				u_long, u_long, u_int, int, struct cred *,
				CLIENT **);
/*
 * Alloc_xid presents an interface which kernel RPC clients
 * should use to allocate their XIDs. Its implementation
 * may change over time (for example, to allow sharing of
 * XIDs between the kernel and user-level applications, so
 * all XID allocation should be done by calling alloc_xid().
 */
extern	u_long		alloc_xid(_VOID);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_RPC_CLNT_H */
