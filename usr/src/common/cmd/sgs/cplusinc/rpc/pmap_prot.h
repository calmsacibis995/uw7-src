#ident	"@(#)cplusinc:rpc/pmap_prot.h	1.3"

#ifndef _NET_RPC_PMAP_PROT_H	/* wrapper symbol for kernel use */
#define _NET_RPC_PMAP_PROT_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/pmap_prot.h	1.15"
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
 *	pmap_prot.h, protocol for the local binder service, or pmap.
 */

#ifdef _KERNEL_HEADERS

#include <net/rpc/types.h> 	/* REQUIRED */

#elif defined(_KERNEL)

#include <rpc/types.h> 		/* REQUIRED */

#else

#include <rpc/types.h> 		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *
 * PMAPPROC_NULL() returns ()
 * 	takes nothing, returns nothing
 *
 * PMAPPROC_SET(struct pmap) returns (bool_t)
 * 	TRUE is success, FALSE is failure. Registers the tuple
 *	[prog, vers, prot, port].
 *
 * PMAPPROC_UNSET(struct pmap) returns (bool_t)
 *	TRUE is success, FALSE is failure. Un-registers pair
 *	[prog, vers]. prot and port are ignored.
 *
 * PMAPPROC_GETPORT(struct pmap) returns (long unsigned).
 *	0 is failure. Otherwise returns the port number where the pair
 *	[prog, vers] is registered. It may lie!
 *
 * PMAPPROC_DUMP() RETURNS (struct pmaplist *)
 *
 * PMAPPROC_CALLIT(unsigned, unsigned, unsigned, string<>)
 * 	RETURNS (port, string<>);
 *
 *	usage: encapsulatedresults = PMAPPROC_CALLIT(prog, vers, proc,
 *	encapsulatedargs);
 *
 * 	Calls the procedure on the local machine. If it is not registered,
 *	this procedure is quite; ie it does not return error information!!!
 *	This procedure only is supported on rpc/udp and calls via
 *	rpc/udp. This routine only passes null authentication parameters.
 *	This file has no interface to xdr routines for PMAPPROC_CALLIT.
 *
 *	The service supports remote procedure calls on udp/ip or tcp/ip
 *	socket 111.
 */

#define PMAPPROG		((u_long)100000)
#define PMAPVERS		((u_long)2)
#define PMAPVERS_PROTO		((u_long)2)
#define PMAPVERS_ORIG		((u_long)1)

#define PMAPPROC_NULL		((u_long)0)
#define PMAPPROC_SET		((u_long)1)
#define PMAPPROC_UNSET		((u_long)2)
#define PMAPPROC_GETPORT	((u_long)3)
#define PMAPPROC_DUMP		((u_long)4)
#define PMAPPROC_CALLIT		((u_long)5)
#define	PMAPPORT		111

/*
 * A mapping of (program, version, protocol) to port number
 */

struct pmap {
	u_long pm_prog;
	u_long pm_vers;
	u_long pm_prot;
	u_long pm_port;
};
typedef	struct pmap		pmap;
typedef pmap			PMAP;

#ifdef __STDC__

extern bool_t xdr_pmap(XDR *, PMAP *);

#else /* !__STDC__ */

extern bool_t xdr_pmap();
  
#endif /* __STDC__ */

/*
 * Supported values for the "prot" field
 */

#define PMAP_IPPROTO_TCP	6
#define PMAP_IPPROTO_UDP	17

/*
 * A list of mappings
 *
 * Below are two definitions for the pmaplist structure. This is done because
 * xdr_pmaplist() is specified to take a struct pmaplist **, rather than a
 * struct pmaplist * that rpcgen would produce. One version of the pmaplist
 * structure (actually called pm__list) is used with rpcgen, and the other is
 * defined only in the header file for compatibility with the specified
 * interface.
 */

struct pm__list {
	pmap		pml_map;
	struct pm__list	*pml_next;
};
typedef	struct pm__list		pm__list;
bool_t				xdr_pm__list();

typedef	pm__list		*pmaplist_ptr;
bool_t				xdr_pmaplist_ptr();

typedef	struct pm__list		pmaplist;
typedef	struct pm__list		PMAPLIST;

struct pmaplist {
	PMAP		pml_map;
	struct pmaplist	*pml_next;
};

#ifdef __STDC__

extern	bool_t			xdr_pmaplist(XDR *, pmaplist**);

#else

bool_t				xdr_pmaplist();

#endif

/*
 * Client-side only representation of rmtcallargs structure.
 *
 * The routine that XDRs the rmtcallargs structure must deal with the
 * opaque arguments in the "args" structure. xdr_rmtcall_args() needs to be
 * passed the XDR routine that knows the args' structure. This routine
 * doesn't need to go over-the-wire (and it wouldn't make sense anyway) since
 * the application being called knows the args structure already. So we use a
 * different "XDR" structure on the client side, p_rmtcallargs, which includes
 * the args' XDR routine.
 */
struct p_rmtcallargs {
	u_long		prog;
	u_long		vers;
	u_long		proc;
	struct {
		u_int	args_len;
		char	*args_val;
	} args;
	xdrproc_t	xdr_args;	/* encodes args */
};

#ifdef __STDC__

extern	bool_t			xdr_rmtcallargs (XDR *, struct p_rmtcallargs *);

#else

extern	bool_t			xdr_rmtcallargs ();

#endif

/*
 * Client-side only representation of rmtcallres structure.
 */
struct p_rmtcallres {
	u_long		port;
	struct {
		u_int	res_len;
		char	*res_val;
	} res;
	xdrproc_t	xdr_res;	/* decodes res */
};

#ifdef __STDC__

extern	bool_t			xdr_rmtcallres (XDR *, struct p_rmtcallres *);

#else

extern	bool_t			xdr_rmtcallres ();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_PMAP_PROT_H */
