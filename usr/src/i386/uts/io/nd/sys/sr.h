#ifndef _IO_ND_SYS_SR_H  /* wrapper symbol for kernel use */
#define _IO_ND_SYS_SR_H  /* subject to change without notice */

#ident "@(#)sr.h	29.1"
#ident "$Header$"
/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef MAX_ROUTES
#define MAX_ROUTES 2
#endif
#define HASH_MODULUS  509             /* number of hash heads - prime # */

#define MAX_SEGMENTS	14
#define MAX_ROUTE_SZ	(MAX_SEGMENTS * sizeof(ushort))

struct  route_info {
	unchar	ri_control0;
	unchar	ri_control1;
	ushort	ri_segments[MAX_SEGMENTS];
};

	/* Broadcast indicator bits, for control0 field */
#define	SRF	0x00	/* Specifically Routed Frame */
#define STE	0xc0	/* Spanning Tree Explorer Frame */
#define ARE	0x80	/* All Routes Explorer Frame */
#define IS_SRF(x) (((x) & 0x80)==SRF)
#define IS_STE(x) (((x) & 0xc0)==STE)
#define IS_ARE(x) (((x) & 0xc0)==ARE)
#define LEN_MASK	0x1f	/* Route Length mask */

	/* Direction indicator bits, for control1 field */
#define DIR_FORWARD	0x00		/* Interpret Route Left to Right */
#define DIR_BACKWARD	0x80		/* Interpret Route Right to Left */
#define DIR_BIT		0x80

	/* Maximum frame size, for control1 field */
#define LEN_516		0x00
#define LEN_1500	0x10
#define LEN_2052	0x20
#define LEN_4472	0x30
#define LEN_8144	0x40
#define LEN_11407	0x50
#define LEN_17800	0x60
#define LEN_NOT_SPEC	0x70

	/* Route Indicator bit of source address */
#define ROUTE_INDICATOR	0x80
#define FDDI_ROUTE_INDICATOR	0x01

struct route {
	struct route	       *r_next, *r_back, *hsh_next;
	time_t			r_timeout;	/* in seconds */
	time_t			r_last_tx;	/* time of last tx */
	unchar			r_mac_addr[6];
	unchar			r_state;
	unchar			r_list;
	ulong			r_ARP_mon;
	ulong 			r_tx_mon;
	ulong			r_STE_ucs;
	struct route_info	r_info;
};

/* following timing parameters are in seconds */
struct route_param {
	int 		tx_resp;    /* timeout for responding to rx */
	int 		rx_ARE;	    /* window for rejecting more AREs */
	int		rx_STE_bcs; /* # STE bcs before invalidating route entry
				     * and find new route */
	int		rx_STE_ucs; /* # STE ucs before invalidating route entry
				     * and find new route */
	int 		max_tx;	    /* upper limit for tx "recur" window */
	int		min_tx;	    /* lower limit for tx "recur" window */
	int		tx_recur;   /* detected "recurs" before tx STE */
	int		ARE_disa;   /* disable sending ARE frames */
};

struct route_table {
	lock_t		*lock_rt;
	struct route_param
			*parms;
	uint		nroutes;	/* number of routes in route_table */
	uint		ninuse;		/* #Currently in use */
	struct route	*free;		/* Free list head */
	struct route	*free_b;	/* Free list tail */
	struct route	*inuse;		/* List head for 'SR_IN_USE' */
	struct route 	*inuse_b;	/* tail for above */
	struct route	*disco;		/* List head for other states */
	struct route	*disco_b;	/* tail for above */
		/* Routes */
	struct route	no_route,	/* 'No route' */
			ste_route,	/* 'Single-route broadcast' */
			are_route;	/* 'All-routes broadcast' */
        struct route    *hash_heads[HASH_MODULUS];
	struct route	*routes;
};

#if defined(__STDC__) && !defined(_NO_PROTOTYPE)

/* int	dlpiSR_init(per_card_info_t *cp); */
int	dlpiSR_make_header(per_card_info_t *cp, unchar *, unchar *);
int	dlpiSR_primitives(per_card_info_t *cp, queue_t *, mblk_t *);
/* void	dlpiSR_rx_parse(per_card_info_t *cp, struct per_frame_info *); */
/* void	dlpiSR_auto_rx(per_card_info_t *cp, struct per_frame_info *f); */

#endif
 
#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_ND_SYS_SR_H */
