#ident	"@(#)cplusinc:rpc/rpcb_clnt.h	1.4"

#ifndef _NET_RPC_RPCB_CLNT_H	/* wrapper symbol for kernel use */
#define _NET_RPC_RPCB_CLNT_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/rpcb_clnt.h	1.14"
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
 *	rpcb_clnt.h, supplies C routines to get to the rpcbind
 *	services.
 *
 *	Usage:
 *
 *	success = rpcb_set(program, version, nconf, address);
 *	success = rpcb_unset(program, version, nconf);
 *	success = rpcb_getaddr(program, version, nconf, host);
 *	head = rpcb_getmaps(nconf, host);
 *	clnt_stat = rpcb_rmtcall(nconf, host, program, version, procedure,
 *		xdrargs, argsp, xdrres, resp, tout, addr_ptr)
 * 	clnt_stat = rpc_broadcast(program, version, procedure,
 *		xdrargs, argsp,	xdrres, resp, eachresult, nettype)
 *		(like rpcb_rmtcall, except the call is broadcasted to all
 *		locally connected nets. For each valid response received,
 *		the procedure eachresult is called. Its form is:
 *		done = eachresult(resp, raddr, netconf)
 *			bool_t done;
 *			caddr_t resp;
 *			struct netbuf *raddr;
 *			struct netconfig *netconf;
 *		where resp points to the results of the call and raddr is the
 *		address if the responder to the broadcast. netconf is the
 *		on which the response came.
 *	success = rpcb_gettime(host, timep)
 *	uaddr = rpcb_taddr2uaddr(nconf, taddr);
 *	taddr = rpcb_uaddr2uaddr(nconf, uaddr);
 */

#ifdef _KERNEL_HEADERS

#include <net/rpc/types.h>	/* REQUIRED */
#include <net/rpc/rpcb_prot.h>	/* REQUIRED */
#include <net/netconfig.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <rpc/types.h>		/* REQUIRED */
#include <rpc/rpcb_prot.h>	/* REQUIRED */
#include <sys/netconfig.h>	/* REQUIRED */

#else

#include <netconfig.h>		/* REQUIRED */
#include <rpc/types.h> 		/* REQUIRED */
#include <rpc/rpcb_prot.h> 	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef __STDC__

extern bool_t		rpcb_set(u_long, u_long, const struct netconfig *,
					const struct netbuf *);
extern bool_t		rpcb_unset(u_long, u_long, const struct netconfig *);
extern rpcblist		*rpcb_getmaps(const struct netconfig *, const char *);
extern enum clnt_stat	rpcb_rmtcall(const struct netconfig *, const char *,
				     u_long, u_long, u_long,
				     const xdrproc_t, caddr_t,
				     const xdrproc_t, caddr_t,
				     const struct timeval, struct netbuf *);
extern bool_t		rpcb_getaddr(u_long, u_long, const struct netconfig *,
				     struct netbuf *, const char *);
extern bool_t		rpcb_gettime(const char *, time_t *);
extern char		*rpcb_taddr2uaddr(struct netconfig *, struct netbuf *);
extern struct netbuf	*rpcb_uaddr2taddr(struct netconfig *, char *);

#else

extern bool_t		rpcb_set();
extern bool_t		rpcb_unset();
extern rpcblist		*rpcb_getmaps();
extern enum clnt_stat	rpcb_rmtcall();
extern bool_t		rpcb_getaddr();
extern bool_t		rpcb_gettime();
extern char		*rpcb_taddr2uaddr();
extern struct netbuf	*rpcb_uaddr2taddr();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_RPCB_CLNT_H */
