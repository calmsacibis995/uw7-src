#ident	"@(#)cplusinc:rpc/clnt_soc.h	1.4"

#ifndef _NET_RPC_CLNT_SOC_H	/* wrapper symbol for kernel use */
#define _NET_RPC_CLNT_SOC_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/clnt_soc.h	1.15"
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
 *	clnt_soc.h, client side remote procedure call interface
 *	using berkeley sockets, present only for compatibility
 *	with original rpc interface.
 */

#ifdef _KERNEL_HEADERS

#include <net/rpc/clnt.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <rpc/clnt.h>		/* REQUIRED */

#else

#include <sys/socket.h>		/* SVR4.0COMPAT */
#include <netinet/in.h>		/* SVR4.0COMPAT */
#include <rpc/xdr.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * rpc imposed limit on udp msg size
 */
#define UDPMSGSIZE		8800

#ifdef __STDC__

extern	enum clnt_stat	callrpc (const char *, u_long, u_long, u_long,
				 const xdrproc_t, char *,
				 const xdrproc_t, char *);

#else

extern	int	callrpc ();

#endif

/*
 * tcp, udp and memory based rpc.
 */

#ifdef __STDC__

extern	CLIENT		*clnttcp_create (const struct sockaddr_in *, u_long,
					 u_long, int *, u_int, u_int);
extern	CLIENT		*clntudp_create (const struct sockaddr_in *, u_long,
					 u_long, const struct timeval, int *);
extern	CLIENT		*clntudp_bufcreate(const struct sockaddr_in *, u_long,
					   u_long, const struct timeval, int *,
					   u_int, u_int);
extern	CLIENT		*clntraw_create (u_long, u_long);

#else

extern	CLIENT		*clnttcp_create ();
extern	CLIENT		*clntudp_create ();
extern	CLIENT		*clntudp_bufcreate ();
extern	CLIENT		*clntraw_create ();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_CLNT_SOC_H */
