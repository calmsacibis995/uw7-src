#ident	"@(#)cplusinc:rpc/rpcb_prot.h	1.3"

#ifndef _NET_RPC_RPCB_PROT_H	/* wrapper symbol for kernel use */
#define _NET_RPC_RPCB_PROT_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/rpcb_prot.h	1.15"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/rpc/types.h> 	/* REQUIRED */
#include <net/rpc/xdr.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h>		/* REQUIRED */
#include <rpc/xdr.h>		/* REQUIRED */

#else

#include <rpc/types.h> 		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * The following procedures are supported by the protocol in version 3:
 *
 * RPCBPROC_NULL() returns ()
 * 	takes nothing, returns nothing
 *
 * RPCBPROC_SET(rpcb) returns (bool_t)
 * 	TRUE is success, FALSE is failure. Registers the tuple
 *	[prog, vers, address, owner, netid].
 *	Finds out owner and netid information on its own.
 *
 * RPCBPROC_UNSET(rpcb) returns (bool_t)
 *	TRUE is success, FALSE is failure. Un-registers tuple
 *	[prog, vers, netid]. addresses is ignored.
 *	If netid is NULL, unregister all.
 *
 * RPCBPROC_GETADDR(rpcb) returns (string).
 *	0 is failure. Otherwise returns the universal address where the
 *	triple [prog, vers, netid] is registered. Ignore address and owner.
 *
 * RPCBPROC_DUMP() RETURNS (rpcblist_ptr)
 *	used to dump the entire rpcbind maps
 *
 * RPCBPROC_CALLIT(rpcb_rmtcallargs)
 * 	RETURNS (rpcb_rmtcallres);
 * 	Calls the procedure on the remote machine. If it is not registered,
 *	this procedure is quiet; i.e. it does not return error information!!!
 *	This routine only passes null authentication parameters.
 *	It has no interface to xdr routines for RPCBPROC_CALLIT.
 *
 * RPCBPROC_GETTIME() returns (int).
 *	Gets the remote machines time
 *
 * RPCBPROC_UADDR2TADDR(strint) RETURNS (struct netbuf)
 *	Returns the netbuf address from universal address.
 *
 * RPCBPROC_TADDR2UADDR(struct netbuf) RETURNS (string)
 *	Returns the universal address from netbuf address.
 *
 * END OF RPCBIND VERSION 3 PROCEDURES
 */
/*
 * Except for RPCBPROC_CALLIT, the procedures above are carried over to
 * rpcbind version 4. Those below are added or modified for version 4.
 * NOTE: RPCBPROC_BCAST HAS THE SAME FUNCTIONALITY AND PROCEDURE NUMBER
 * AS RPCBPROC_CALLIT.
 *
 * RPCBPROC_BCAST(rpcb_rmtcallargs)
 * 	RETURNS (rpcb_rmtcallres);
 * 	Calls the procedure on the remote machine. If it is not registered,
 *	this procedure IS quiet; i.e. it DOES NOT return error information!!!
 *	This routine should be used for broadcasting and nothing else.
 *
 * RPCBPROC_GETVERSADDR(rpcb) returns (string).
 *	0 is failure. Otherwise returns the universal address where the
 *	triple [prog, vers, netid] is registered. Ignore address and owner.
 *	Same as RPCBPROC_GETADDR except that if the given version number
 *	is not available, the address is not returned.
 *
 * RPCBPROC_INDIRECT(rpcb_rmtcallargs)
 * 	RETURNS (rpcb_rmtcallres);
 * 	Calls the procedure on the remote machine. If it is not registered,
 *	this procedure is NOT *//*quiet; i.e. it DOES return error information!!!
 * 	as any normal application would expect.
 *
 * RPCBPROC_GETADDRLIST(rpcb) returns (rpcb_entry_list_ptr).
 *	Same as RPCBPROC_GETADDR except that it returns a list of all the
 *	addresses registered for the combination (prog, vers) (for all
 *	transports).
 *
 * RPCBPROC_GETSTAT(void) returns (rpcb_stat_byvers)
 *	Returns the statistics about the kind of requests received by rpcbind.
 */

/*
 * A mapping of (program, version, network ID) to address
 */
struct rpcb {
	u_long	r_prog;			/* program number */
	u_long	r_vers;			/* version number */
	char	*r_netid;		/* network id */
	char	*r_addr;		/* universal address */
	char	*r_owner;		/* owner of the mapping */
};
typedef	struct rpcb		rpcb;
typedef	struct rpcb		RPCB;

#ifdef __STDC__

extern bool_t xdr_rpcb(XDR *, RPCB *);

#else /* !__STDC__ */

extern bool_t xdr_rpcb();

#endif /* __STDC__ */

/*
 * A list of mappings
 *
 * Below are two definitions for the rpcblist structure. This is done because
 * xdr_rpcblist() is specified to take a struct rpcblist **, rather than a
 * struct rpcblist * that rpcgen would produce. One version of the rpcblist
 * structure (actually called rp__list) is used with rpcgen, and the other is
 * defined only in the header file for compatibility with the specified
 * interface.
 */

struct rp__list {
	rpcb rpcb_map;
	struct rp__list *rpcb_next;
};
typedef struct rp__list rp__list;
bool_t xdr_rp__list();

struct rpcblist {
	RPCB rpcb_map;
	struct rpcblist *rpcb_next;
};

typedef struct rpcblist rpcblist;
typedef struct rpcblist RPCBLIST;

typedef rpcblist *rpcblist_ptr;
bool_t xdr_rpcblist_ptr();

#ifdef __STDC__

extern	bool_t xdr_rpcblist(XDR *, rpcblist**);

#else

bool_t xdr_rpcblist();

#endif


/*
 * Arguments of remote calls
 */

#ifdef _KERNEL
struct rpcb_rmtcallargs {
	u_long prog;			/* program number */
	u_long vers;			/* version number */
	u_long proc;			/* procedure number */
	u_long arglen;			/* arg len */
	caddr_t args_ptr;		/* argument */
	xdrproc_t xdr_args;		/* XDR routine for argument */
};
#else
struct rpcb_rmtcallargs {
	u_long prog;
	u_long vers;
	u_long proc;
	struct {
		u_int args_len;
		char *args_val;
	} args;
};
typedef struct rpcb_rmtcallargs rpcb_rmtcallargs;
#endif

#ifdef __STDC__

extern bool_t xdr_rpcb_rmtcallargs(XDR *, struct rpcb_rmtcallargs *);

#else /* !__STDC__ */

extern bool_t xdr_rpcb_rmtcallargs();

#endif /* __STDC__ */

/*
 * Client-side only representation of rpcb_rmtcallargs structure.
 *
 * The routine that XDRs the rpcb_rmtcallargs structure must deal with the
 * opaque arguments in the "args" structure. xdr_rpcb_rmtcallargs() needs to
 * be passed the XDR routine that knows the args' structure. This routine
 * doesn't need to go over-the-wire (and it wouldn't make sense anyway) since
 * the application being called already knows the args structure. So we use a
 * different "XDR" structure on the client side, r_rpcb_rmtcallargs, which
 * includes the args' XDR routine.
 */
struct r_rpcb_rmtcallargs {
	u_long prog;
	u_long vers;
	u_long proc;
	struct {
		u_int args_len;
		char *args_val;
	} args;
	xdrproc_t	xdr_args;	/* encodes args */
};


/*
 * Results of the remote call
 */

#ifdef _KERNEL
struct rpcb_rmtcallres {
	char *addr_ptr;			/* remote universal address */
	u_long resultslen;		/* results length */
	caddr_t results_ptr;		/* results */
	xdrproc_t xdr_results;		/* XDR routine for result */
};
#else
struct rpcb_rmtcallres {
	char *addr;
	struct {
		u_int results_len;
		char *results_val;
	} results;
};
#endif
typedef struct rpcb_rmtcallres rpcb_rmtcallres;

#ifdef __STDC__

extern bool_t xdr_rpcb_rmtcallres(XDR *, struct rpcb_rmtcallres *);

#else /* !__STDC__ */

extern bool_t xdr_rpcb_rmtcallres();

#endif /* __STDC__ */

/*
 * Client-side only representation of rpcb_rmtcallres structure.
 */
struct r_rpcb_rmtcallres {
	char *addr;
	struct {
		u_int results_len;
		char *results_val;
	} results;
	xdrproc_t	xdr_res;	/* decodes results */
};

/*
 * rpcb_entry contains a merged address of a service on a particular
 * transport, plus associated netconfig information. A list of rpcb_entrys
 * is returned by RPCBPROC_GETADDRLIST. See netconfig.h for values used
 * in r_nc_* fields.
 */

struct rpcb_entry {
	char *r_maddr;
	char *r_nc_netid;
	u_long r_nc_semantics;
	char *r_nc_protofmly;
	char *r_nc_proto;
};
typedef struct rpcb_entry rpcb_entry;
bool_t xdr_rpcb_entry();

/*
 * A list of addresses supported by a service.
 */

struct rpcb_entry_list {
	rpcb_entry rpcb_entry_map;
	struct rpcb_entry_list *rpcb_entry_next;
};
typedef struct rpcb_entry_list rpcb_entry_list;
bool_t xdr_rpcb_entry_list();

typedef rpcb_entry_list *rpcb_entry_list_ptr;
bool_t xdr_rpcb_entry_list_ptr();

/*
 * rpcbind statistics
 */

#define rpcb_highproc_2 RPCBPROC_CALLIT
#define rpcb_highproc_3 RPCBPROC_TADDR2UADDR
#define rpcb_highproc_4 RPCBPROC_GETSTAT
#define RPCBSTAT_HIGHPROC 13
#define RPCBVERS_STAT 3
#define RPCBVERS_4_STAT 2
#define RPCBVERS_3_STAT 1
#define RPCBVERS_2_STAT 0

/* Link list of all the stats about getport and getaddr */

struct rpcbs_addrlist {
	u_long prog;
	u_long vers;
	int success;
	int failure;
	char *netid;
	struct rpcbs_addrlist *next;
};
typedef struct rpcbs_addrlist rpcbs_addrlist;
bool_t xdr_rpcbs_addrlist();

/* Link list of all the stats about rmtcall */

struct rpcbs_rmtcalllist {
	u_long prog;
	u_long vers;
	u_long proc;
	int success;
	int failure;
	int indirect;
	char *netid;
	struct rpcbs_rmtcalllist *next;
};
typedef struct rpcbs_rmtcalllist rpcbs_rmtcalllist;
bool_t xdr_rpcbs_rmtcalllist();

typedef int rpcbs_proc[RPCBSTAT_HIGHPROC];
bool_t xdr_rpcbs_proc();

typedef rpcbs_addrlist *rpcbs_addrlist_ptr;
bool_t xdr_rpcbs_addrlist_ptr();

typedef rpcbs_rmtcalllist *rpcbs_rmtcalllist_ptr;
bool_t xdr_rpcbs_rmtcalllist_ptr();

struct rpcb_stat {
	rpcbs_proc info;
	int setinfo;
	int unsetinfo;
	rpcbs_addrlist_ptr addrinfo;
	rpcbs_rmtcalllist_ptr rmtinfo;
};
typedef struct rpcb_stat rpcb_stat;
bool_t xdr_rpcb_stat();

/*
 * One rpcb_stat structure is returned for each version of rpcbind
 * being monitored.
 */

typedef rpcb_stat rpcb_stat_byvers[RPCBVERS_STAT];
bool_t xdr_rpcb_stat_byvers();

/*
 * We don't define netbuf in RPCL, since it would contain structure member
 * names that would conflict with the definition of struct netbuf in
 * <xti.h>. Instead we merely declare the XDR routine xdr_netbuf() here,
 * and implement it ourselves in rpc/rpcb_prot.c.
 */
#ifdef __STDC__

extern	bool_t xdr_netbuf(XDR *, struct netbuf *);

#else

bool_t xdr_netbuf();

#endif

#define RPCBPROG		((u_long)100000)
#define RPCBVERS		((u_long)3)

/*
 * All the defined procedures of rpcbind.
 */
#define RPCBPROC_NULL		((u_long)0)
#define RPCBPROC_SET		((u_long)1)
#define RPCBPROC_UNSET		((u_long)2)
#define RPCBPROC_GETADDR	((u_long)3)
#define RPCBPROC_DUMP		((u_long)4)
#define RPCBPROC_CALLIT		((u_long)5)
#define RPCBPROC_GETTIME	((u_long)6)
#define RPCBPROC_UADDR2TADDR	((u_long)7)
#define RPCBPROC_TADDR2UADDR	((u_long)8)
#define RPCBVERS4		((u_long)4)
#define RPCBPROC_BCAST		((u_long)RPCBPROC_CALLIT)
#define RPCBPROC_GETVERSADDR	((u_long)9)
#define RPCBPROC_INDIRECT	((u_long)10)
#define RPCBPROC_GETADDRLIST	((u_long)11)
#define RPCBPROC_GETSTAT	((u_long)12)

extern bool_t *rpcbproc_set_3();
extern bool_t *rpcbproc_unset_3();
extern char **rpcbproc_getaddr_3();
extern rpcblist_ptr *rpcbproc_dump_3();
extern rpcb_rmtcallres *rpcbproc_callit_3();
extern u_int *rpcbproc_gettime_3();
extern struct netbuf *rpcbproc_uaddr2taddr_3();
extern char **rpcbproc_taddr2uaddr_3();
extern bool_t *rpcbproc_set_4();
extern bool_t *rpcbproc_unset_4();
extern char **rpcbproc_getaddr_4();
extern rpcblist_ptr *rpcbproc_dump_4();
extern rpcb_rmtcallres *rpcbproc_bcast_4();
extern u_int *rpcbproc_gettime_4();
extern struct netbuf *rpcbproc_uaddr2taddr_4();
extern char **rpcbproc_taddr2uaddr_4();
extern char **rpcbproc_getversaddr_4();
extern rpcb_rmtcallres *rpcbproc_indirect_4();
extern rpcb_entry_list_ptr *rpcbproc_getaddrlist_4();
extern rpcb_stat *rpcbproc_getstat_4();

#define	RPCBVERS_3		RPCBVERS
#define	RPCBVERS_4		RPCBVERS4

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_RPCB_PROT_H */
