#ident "@(#)sr.c	29.3"
#ident "$Header$"

/*
 *	  Copyright (C) The Santa Cruz Operation, 1993-1997.
 *	  This Module contains Proprietary Information of
 *	  The Santa Cruz Operation and should be treated
 *	  as Confidential.
 */
#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/srstate.h>
#include <io/nd/dlpi/include.h>

#else

#include "srstate.h"
#include "include.h"

#endif

static time_t time;

/*
 * A toggle to stop clearing of route table whenever a user starts lli.
 * Can be useful for testing.
 */

#ifdef SR_DEBUG
	int	done_reset = 1;
#endif

STATIC unchar	dlpi_len_to_framesz(uint len);
STATIC uint	dlpi_framesz_to_len(unchar fsz);
STATIC void	dlpi_hsh_link(struct route_table *rt, struct route *rp,int hsh);
STATIC void	dlpi_hsh_unlink(struct route_table *rt, struct route *rp);
STATIC void	dlpi_free_rp(struct route_table *rt, struct route *rp);
STATIC void	dlpi_unlink(struct route_table *rt, struct route *rp,
			    struct route **rt_headp, struct route **rt_tailp);
STATIC void	dlpi_unlink_rp(struct route_table *rt, struct route *rp);
STATIC void	dlpi_link_inuse(struct route_table *rt, struct route *rp);
STATIC void	dlpi_link_disco(struct route_table *rt, struct route *rp);

STATIC struct route *	dlpi_get_new_slot(struct route_table *rt);
STATIC struct route *	dlpi_get_new_rp(struct route_table *rt, uint hsh);
STATIC struct route *	dlpi_lookup_route(struct route_table *rt,
						    unchar *dest, uint hsh);
STATIC void	dlpi_store_route(struct route *rp, struct route_info *ri);
STATIC void	dlpiSR_reset(struct route_table *rt, uint nroutes);
STATIC mblk_t	*dlpi_srtable_add(mblk_t *headmp, mblk_t *mp, struct route *rp);
STATIC int	dlpi_send_dl_srtable_ack(struct route_table *rt, queue_t *q);
STATIC int	dlpi_do_set_srparms(struct route_table *rt,
						    union DL_primitives *srp);

LKINFO_DECL(dlpi_sr_lkinfo, "DLPI::Source Routing mutex lock", 0);

STATIC unchar
dlpi_len_to_framesz(uint len)
{
	if (len <= 516)
		return(LEN_516);

	if (len <= 1500)
		return(LEN_1500);

	if (len <= 2052)
		return(LEN_2052);

	if (len <= 4472)
		return(LEN_4472);

	if (len <= 8144)
		return(LEN_8144);

	if (len <= 11407)
		return(LEN_11407);

	if (len <= 17800)
		return(LEN_17800);

	return(LEN_NOT_SPEC);
}

STATIC uint
dlpi_framesz_to_len(unchar fsz)
{
	static uint lookup[] = { 516, 1500, 2052, 4472, 8144, 11407, 17800 };

	return(lookup[(fsz >> 4) & 0x07]);
}

STATIC void
dlpi_hsh_link(struct route_table *rt, struct route *rp, int hsh)
{
	/* Make new entry the head of the hash list */
	rp->hsh_next = rt->hash_heads[hsh];
	rt->hash_heads[hsh] = rp;
}

STATIC void
dlpi_hsh_unlink(struct route_table *rt, struct route *rp)
{
	int		hsh;
	struct route	*hrp;

	hsh = dlpi_hsh(rp->r_mac_addr, HASH_MODULUS);
	hrp = rt->hash_heads[hsh];
	if (!hrp) {
		cmn_err(CE_WARN, "Source Routing hash head empty in unlink");
		return;
	} else if (hrp == rp) {
		rt->hash_heads[hsh] = rp->hsh_next;
		return;
	} else while (hrp->hsh_next) {
		if (hrp->hsh_next == rp) {
			hrp->hsh_next = rp->hsh_next;
			return;
		}
		hrp = hrp->hsh_next;
	}
	cmn_err(CE_WARN, "Source Routing hash unlink can't find route");
}

/*
 * put rp back into free list and decrement inuse count
 * and reset state to be 0
 */

STATIC void
dlpi_free_rp(struct route_table *rt, struct route *rp)
{	
	dlpi_hsh_unlink(rt, rp);
	rp->r_next = rt->free;
	rp->r_back = (struct route *)0;
	if (rt->free)
		(rt->free)->r_back = rp;
	rt->free = rp;
	if (!(rt->free_b))
		rt->free_b = rp;
	--rt->ninuse;
	rp->r_state = rp->r_list = 0;
	rp->r_timeout = rp->r_last_tx = 0;
	rp->r_ARP_mon = rp->r_tx_mon = rp->r_STE_ucs = 0;
}

STATIC void
dlpi_unlink(struct route_table *rt, struct route *rp, struct route **rt_headp,
	     struct route **rt_tailp)
{
	if (*rt_headp == rp)
		*rt_headp = rp->r_next;
	if (rp->r_next)
		rp->r_next->r_back = rp->r_back;
	else
		*rt_tailp = rp->r_back;
	if (rp->r_back)
		rp->r_back->r_next = rp->r_next;
}

STATIC void
dlpi_unlink_rp(struct route_table *rt, struct route *rp)
{
	if (rp->r_list == R_DISCO)
		dlpi_unlink(rt, rp, &rt->disco, &rt->disco_b);
	else
		dlpi_unlink(rt, rp, &rt->inuse, &rt->inuse_b);
}

/*
 * link in inuse and set r_list to INUSE
 */

STATIC void
dlpi_link_inuse(struct route_table *rt, struct route *rp)
{
	rp->r_next = rt->inuse;
	rp->r_back = (struct route *)0;
	if (rt->inuse)
		(rt->inuse)->r_back = rp;
	else
		rt->inuse_b = rp;
	rt->inuse = rp;
	rp->r_list = R_INUSE;
}


STATIC void	/* link in disco and set r_list to DISCO */
dlpi_link_disco(struct route_table *rt, struct route *rp)
{
	rp->r_next = rt->disco;
	rp->r_back = (struct route *)0;
	if (rt->disco)
		(rt->disco)->r_back = rp;
	else
		rt->disco_b = rp;
	rt->disco = rp;
	rp->r_list = R_DISCO;
}


STATIC struct route *
dlpi_get_new_slot(struct route_table *rt)
{
	struct route *rp = (struct route *)0;
	drv_getparm(TIME, &time);
	if ((rt->disco) && (rt->disco->r_timeout <= time)) { 
		/* unclutter the table by getting rid of temporary entries */ 
		rp = rt->disco_b;
		dlpi_unlink(rt, rp, &rt->disco, &rt->disco_b);
		dlpi_hsh_unlink(rt, rp);
		--rt->ninuse;
	}
	else {
		if ( rp = rt->free) { 
			if (rt->free == rp)
				rt->free = rp->r_next;
			if (rp->r_next)
				rp->r_next->r_back = rp->r_back;
			if (rp->r_back)
				rp->r_back->r_next = rp->r_next;
			if (rt->free_b == rp)
				rt->free_b = (struct route *)0;
		}
		else {
			if (rt->inuse) {
				rp = rt->inuse_b;
				dlpi_unlink(rt, rp, &rt->inuse, &rt->inuse_b);
				dlpi_hsh_unlink(rt, rp);
			}
			else if (rt->disco) {	/* disco has all entries */	
				rp = rt->disco_b;
				dlpi_unlink(rt, rp, &rt->disco, &rt->disco_b);
				dlpi_hsh_unlink(rt, rp);
			}
			--rt->ninuse;
		}
	}
	++rt->ninuse;	
	bzero(rp, sizeof(struct route));	
	return (rp);
}

STATIC struct route *
dlpi_get_new_rp(struct route_table *rt, uint hsh)
{
	struct route *newrp;

	newrp = dlpi_get_new_slot(rt);
	dlpi_hsh_link(rt, newrp, hsh);
	return(newrp);
}

STATIC struct route *
dlpi_lookup_route(struct route_table *rt, unchar *dest, uint hsh)
{
	register struct route *rp;

	rp = rt->hash_heads[hsh];
	while (rp) {
		if ( !(rp->r_state) ) {		/* nothing there */
			cmn_err(CE_WARN, "Source Routing hash entry w/zero state");
			break;
		}
		if ( *((ulong *)(rp->r_mac_addr)) == *((ulong *)(dest)) &&
		     *((ushort*)(rp->r_mac_addr+4)) == *((ushort*)(dest+4))) {
			return(rp);
		}
		rp = rp->hsh_next;
	}
	return ((struct route *)0);
}  

STATIC void
dlpi_store_route(struct route *rp, struct route_info *ri)
{
	int len;

	len = ri->ri_control0 & LEN_MASK;
	if (len>2) {
		ri->ri_control1 ^= DIR_BIT;	/* Flip the direction bit */
		ri->ri_control0 &= ~STE;	/* Turn route into an SRF */
		bcopy(ri, &rp->r_info, len);	/* Store route */
	} else
		rp->r_info.ri_control0 = 0;	/* Store 'No ROUTE' */
}

STATIC void
dlpiSR_reset(struct route_table *rt, uint nroutes)
{
	int i;
	pl_t sp;

	if (! nroutes)
		return;
	sp = LOCK_RT(rt);

	rt->nroutes = HASH_MODULUS;	/* statistic reported by ndstat */
	bzero(rt->routes, sizeof(struct route) * nroutes);
	bzero(rt->hash_heads, sizeof(struct route *) * HASH_MODULUS);

	rt->nroutes = nroutes;
	rt->ninuse = 0;
	rt->inuse = rt->disco = rt->disco_b = rt->inuse_b = (struct route *)0;
	rt->free = rt->routes;
	rt->routes[0].r_next = &rt->routes[1];
	for (i = 1; i < (nroutes - 1); ++i) {
		rt->routes[i].r_next = &(rt->routes[i+1]);
		rt->routes[i].r_back = &(rt->routes[i-1]);
	}
	if (nroutes == 2) {
		rt->routes[1].r_back = &rt->routes[0];
	} else {
		rt->routes[nroutes-1].r_back = &rt->routes[nroutes-2];
	}
	rt->free_b = &rt->routes[nroutes-1];
	UNLOCK_RT(rt, sp);
}

void
dlpiSR_uninit(per_card_info_t *cp)
{
	register struct route_table *rt = cp->route_table;

	if (rt->lock_rt) {
		LOCK_DEALLOC(rt->lock_rt);
	}
}

/*
 * This routine is called during media init if source routing is desired.
 */
dlpiSR_init(per_card_info_t *cp)
{
	register struct route_table *rt = cp->route_table;

	cmn_err(CE_NOTE, "!dlpiSR_init() cp %x\n", cp);

	if ( !(rt->lock_rt = LOCK_ALLOC(DLPIHIER, plstr,
			&dlpi_sr_lkinfo, KM_NOSLEEP)) ) {
		cmn_err(CE_WARN, "dlpiSR_init: Not enough memory for lock");
		return(0);              /* FAILED */
	}

#ifdef SR_DEBUG
	if (done_reset)
#endif
	dlpiSR_reset(rt, cp->maxroutes);

	rt->ste_route.r_info.ri_control0 = STE | 2;
	rt->ste_route.r_info.ri_control1 = DIR_FORWARD |
					dlpi_len_to_framesz(cp->mac_max_sdu);
	rt->are_route.r_info.ri_control0 = ARE | 2;
	rt->are_route.r_info.ri_control1 = DIR_FORWARD |
					dlpi_len_to_framesz(cp->mac_max_sdu);
	rt->no_route.r_info.ri_control0 = 0;
	return(1);		/* SUCCESS */
}

/*
 * Interpret the Routing Information field, and store the information in
 * the per_frame_info structure passed in
 */

dlpiSR_rx_parse(per_card_info_t *cp, struct per_frame_info *f)
{
	struct route_info *ri = (struct route_info *)f->frame_data;

	if (f->route_present)
	{
		int len = ri->ri_control0 & LEN_MASK;
#ifdef SR_DEBUG
		cmn_err(CE_CONT, "dlpiSR_rx_parse: route present, ");
#endif 		
		f->route_len = len;
		f->route_info = f->frame_data;
		f->frame_data += len;

		/*
		 * The MAC specific code assumed that the routing information
		 * field did not exist, and hence incorrectly set the frame_sap
		 * field.  Now we know the length of the route field, and hence
		 * where the LLC header starts, correct the frame_sap field
		 */

		f->frame_sap = *((unchar *)f->frame_data);
	}
	else
	{
#ifdef SR_DEBUG
		cmn_err(CE_CONT, "dlpiSR_rx_parse: route NOT present,");
#endif 		
		f->route_info = 0;
		f->route_len = 0;
	}

#ifdef SR_DEBUG
	cmn_err(CE_CONT, "ri->ri_controll0  = %x\n", ri->ri_control0);
#endif 		
}

/* The following two routines deal with tx and rx processing, respectively.
 * They are the only routines updating the source routing cache in the 
 * routing table - they lock the table before any accesses.  SR entries
 * are linked into two LRU lists, the inuse and the discovering list.  They
 * are always unlinked before they are used and relinked again into the
 * appropriate list; the re-linking provides the correct LRU ordering of the
 * lists.
 * 
 * The timeout timer is used in three different contexts:
 * 1) in got_STE for the ARE response window.
 * 2) by r_ARP_mon in the got_route state for checking the response time; 
 *    though r_ARP_mon will not be set if the timer is already in use for (3).
 * 3) for the ARE suppression window in got_route; an ARE stomps on any ARP
 *    and takes over the timeout timer.
 * 
 * r_ARP_mon is used in two contexts:
 * 1) in got_STE, to hold the flag to send an ARE - if the response is still 
 *    within the time window, and
 * 2) in got_route, for keeping track of received STE bc's interleaved with 
 *    SRFs transmitted in response (within the response window);
 *    r_ARP_mon is incremented with every received STE bc and with every trans-
 *    mitted SRF.  If r_ARP_mon is not odd after a received STE, or is not even 
 *    after transmitting an SRF, then we know that we didn't have an STE paired 
 *    with an SRF response, and r_ARP_mon is reset.  If, on the other hand, we
 *    got rx_STE_bcs number of STE bcs and SRF tx pairs, then we dump the route
 *    entry and send an ARE (if they are enabled - otherwise we send an STE). 
 *    
 *    The idea here is that we drop the cached route when we detect that
 *    ARP broadcasts are repeated, despite the fact that we "responded"
 *    with SRFs - leading us to assume that the SRF responses never
 *    were received, presumably because of a broken bridge in the cached
 *    route.  Responding with an ARE will find a new route and also cause the
 *    new route to be cached by the recipient. 
 *
 * r_STE_ucs keeps a count of STE ucs received without any intervening SRFs.
 * If that count exceeds rx_STE_ucs, then  we drop the route as above.  The
 * idea here is that STEs were intermingled with SRFs by the sender; if all
 * we see are the STEs, then the cached route used by the SRFs is bad, and
 * we drop it etc.
 *
 * r_tx_mon keeps track of tx's within a certain window and will substitute
 * an STE for an SRF every so often in order to generate the intermingled STEs
 * and SRFs that are also monitored on the receiving side (see r_STE_ucs above)
*/

int
dlpiSR_make_header(per_card_info_t *cp, unchar *dest, unchar *ri_field)
{
	struct route_table *rt = cp->route_table;
	unchar *src = cp->local_mac_addr;

	struct route *rp;
	uint len;
	pl_t sp;
	unchar discard = 0; 
	unchar send_ste = 0; 
	uint hsh;

#ifdef SR_DEBUG
	unchar control0;

	cmn_err(CE_CONT, "dlpiSR_make_header; MAC addr = %b:%b:%b:%b:%b:%b:\n",
	dest[0], dest[1], dest[2], dest[3], dest[4], dest[5]);
#endif

	drv_getparm(TIME, &time);

	hsh = dlpi_hsh(dest, HASH_MODULUS);

	sp = LOCK_RT(rt);

	rp = dlpi_lookup_route(rt, dest, hsh);
	if (!rp) { 			/* no entry found - no entry made */
		rp = &rt->ste_route;	/* also used for broadcast (route */
					/* is never found with broadcast!) */
#ifdef SR_DEBUG
		cmn_err(CE_CONT, "make_header: rp not found\n");
#endif
	}
	else {	/* entry found - but might be replaced with STE or ARE frame */
		/* - after which rp no longer points to the entry - caution! */ 
		dlpi_unlink_rp(rt, rp);		
		dlpi_link_inuse(rt, rp);

		if (rp->r_state == SR_GOT_STE) {	
#ifdef SR_DEBUG
			cmn_err(CE_CONT, "make_header: state = SR_GOT_STE\n");
#endif
			if (rp->r_ARP_mon) {   /* ARP_mon has flag for tx ARE */
				rp->r_ARP_mon = 0;
				if (rp->r_timeout >= time) 
					rp = &rt->are_route;
			}
		}
		else {  /* state = Got Route */
#ifdef SR_DEBUG
			cmn_err(CE_CONT, "make_header: state = GOT_ROUTE\n");
#endif
			if (rp->r_ARP_mon) { /* monitoring STE bc/SRF pairs */
				if ((rp->r_ARP_mon & 1) && (rp->r_timeout >= time)){
					/* inside STE/SRF response window and
					 * r_ARP_mon is odd meaning
					 * rx STE bc/tx SRF sequence is in sync
					 * so increment r_ARP_mon for next tx
					 * should be an SRF
					 */
					++rp->r_ARP_mon;
					if ((rp->r_ARP_mon/2)
					    > rt->parms->rx_STE_bcs) {
						/* STE bc/SRF sequence exceeds
						 * maximum.
						 */
						discard = 1;
						rp->r_timeout = 0;
					}
				}
				else {
					/* outside STE/SRF response window 
					 * or
					 * r_ARP_mon is even implying this is
					 * the 2nd tx SRF in a row.
					 * so reset sequence counter and timer
					 */
					rp->r_ARP_mon = 0;			
					rp->r_timeout = 0;
				}
			}


			if (rp->r_STE_ucs > rt->parms->rx_STE_ucs)
				discard = 1;

			if ((rp->r_last_tx + rt->parms->max_tx) > time) {
				if ((rp->r_last_tx + rt->parms->min_tx) < time) {
					/* we are inside sliding window */
					if ((++rp->r_tx_mon) > rt->parms->tx_recur) {
						rp->r_tx_mon = 0;
						send_ste = 1;
					}
				} /* else have txs before window */
			}
			else	/* are beyond sliding window - reset */
				rp->r_tx_mon = 0;
			rp->r_last_tx = time;

			if (discard) {
#ifdef SR_DEBUG
				cmn_err(CE_CONT, "make_header: discarding entry\n");
#endif
				dlpi_unlink_rp(rt, rp);
				dlpi_free_rp(rt, rp);
				if (rt->parms->ARE_disa)
	 				rp = &rt->ste_route;
				else
					rp = &rt->are_route;
			}
			else {
				if (send_ste)
					rp = &rt->ste_route;
			}
		}
	}
	len = rp->r_info.ri_control0 & LEN_MASK;
	if (len)
		bcopy(&rp->r_info, ri_field, len);


#ifdef SR_DEBUG
	control0 = rp->r_info.ri_control0;
	if (IS_STE(control0))
		cmn_err(CE_CONT, "Sent STE route size (%d)\n", len);
	else if (IS_ARE(control0))
		cmn_err(CE_CONT, "Sent ARE route size (%d)\n", len);
	else if (len)
			cmn_err(CE_CONT, "Sent SRF route size (%d)\n", len);
		else
			cmn_err(CE_CONT, "Sent NO route size (%d)\n", len);

#endif

	UNLOCK_RT(rt, sp);
	return(len);
}

/*
 * Receive frame processing.  Interpret the Routing information field 
 * and set up entry as needed
 */

void
dlpiSR_auto_rx(per_card_info_t *cp, struct per_frame_info *f)
{
	register unchar control0;
	struct route_table *rt = cp->route_table;
	int sp;
	uint hsh;
	struct route_info *ri;
	struct route *rp;

	drv_getparm(TIME, &time);

	if (f->route_present) {
		ri = (struct route_info *)f->route_info;
#ifdef SR_DEBUG
		cmn_err(CE_CONT, "auto_rx: route present");
#endif
	} else {
		ri = &rt->no_route.r_info;
#ifdef SR_DEBUG
		cmn_err(CE_CONT, "auto_rx: route not present");
#endif
	}
	control0 = ri->ri_control0;

#ifdef SR_DEBUG
	cmn_err(CE_CONT, " control0 = %x; ", ri->ri_control0);
	cmn_err(CE_CONT, " MAC addr = %b:%b:%b:%b:%b:%b:\n",
	rp->r_mac_addr[0],
	rp->r_mac_addr[1],
	rp->r_mac_addr[2],
	rp->r_mac_addr[3],
	rp->r_mac_addr[4],
	rp->r_mac_addr[5]);
#endif
	hsh = dlpi_hsh(f->frame_src, HASH_MODULUS);
	sp = LOCK_RT(rt);
	rp = dlpi_lookup_route(rt, f->frame_src, hsh);
	if (!rp) {	
/**************************************************************************
 *			 NO Entry 
 *************************************************************************/
		rp = dlpi_get_new_rp(rt, hsh);				
		if (!rp)
			return;
		bcopy(f->frame_src, rp->r_mac_addr, 6);
		dlpi_store_route(rp, ri);		
#ifdef SR_DEBUG
		cmn_err(CE_CONT, "New Route Table Entry");
		cmn_err(CE_CONT, "; MAC addr = %b:%b:%b:%b:%b:%b:\n",
		rp->r_mac_addr[0],
		rp->r_mac_addr[1],
		rp->r_mac_addr[2],
		rp->r_mac_addr[3],
		rp->r_mac_addr[4],
		rp->r_mac_addr[5]);
#endif

		if (IS_STE(control0)) {
 			/* 1) broadcast, or  				*/
			/* 2) sending station had lost its entry, or 	*/
			/* 3) sending station doing auto-reroute check 	*/
			/* (3) is highly unlikely, since we (receiving) */
			/* don't have an entry any longer. Responding to */
			/* (2) with an STE (instead of an ARE) is OK  	*/
			/* after timeout or loss of entry in disco queue */
			if ((rt->parms->ARE_disa) || !(f->route_present)
			     || (int)(control0 & LEN_MASK) <= 2) {
				rp->r_state = SR_GOT_ROUTE;
			}
			else {
				rp->r_state = SR_GOT_STE;
				rp->r_ARP_mon = 1;    /* tx ARE */ 			
			}
			rp->r_timeout = time + rt->parms->tx_resp;
			dlpi_link_disco(rt, rp);
		}
		else {	/* ARE or SRF or NO */
			rp->r_state = SR_GOT_ROUTE;
			if (IS_ARE(control0)) { 
				/* don't update route with AREs for a while */
				rp->r_timeout = time + rt->parms->rx_ARE;
				/* if broadcast put into discovering list */
				if (f->dest_type == FR_UNICAST)
					dlpi_link_inuse(rt, rp);
				else
					dlpi_link_disco(rt, rp);
			}
			else	/* SRF or NO */
				dlpi_link_inuse(rt, rp);		
		}
	}
	else {	/* found entry */
		dlpi_unlink_rp(rt, rp);
		if (rp->r_state == SR_GOT_STE) {
/**************************************************************************
 *				Got_STE state
 *************************************************************************/
			/* this could be pretty stale, so ... */
			dlpi_store_route(rp, ri);	/* in any case */	
			if (IS_STE(control0)) {	/* reset & repeat the */
						/* checks for ARE_disa  */
						/* and route not present */
				if ((rt->parms->ARE_disa) || !(f->route_present)
				    || (int)(control0 & LEN_MASK) <= 2) {
					rp->r_state = SR_GOT_ROUTE;
				}
				else {
					if (rp->r_timeout < time)
						rp->r_ARP_mon = 1;  /* tx ARE */ 
				} 
				rp->r_timeout = time + rt->parms->tx_resp;
				dlpi_link_disco(rt, rp);
			}
			else {	/* got ARE or SRF */
				rp->r_state = SR_GOT_ROUTE;
				rp->r_ARP_mon = 0;
				if (IS_ARE(control0)) 
					rp->r_timeout = time + rt->parms->rx_ARE;
				else
					rp->r_timeout = 0;
				dlpi_link_inuse(rt, rp);
			}
		}
		else {	
/**************************************************************************
 *				Got_ROUTE state
 **************************************************************************/
			if (IS_STE(control0)) {
			/* check for interleaved rx STE bc and tx SRF for */
			/* ARP retry, or repeated rx STE uc's for re-routing */	

				/* only make checks when route present */
				if ( (f->route_present) &&
				    (int)(control0 & LEN_MASK) >= 2) {
					if (f->dest_type != FR_UNICAST) {
		 				if (rp->r_ARP_mon) {
						   /* have started monitoring
						    * STE/SRF pairs
						    */
						   if ( (rp->r_ARP_mon & 1) ||
						        (rp->r_timeout < time) )
						      /* no tx of SRF after last
						       * STE bc received
						       * or
						       * outside response window
						       * after last STE/SRF pair
						       * reset counter to 0
						       * first
						       */
						   	rp->r_ARP_mon = 0;

						   /* count this STE frame */
						   ++rp->r_ARP_mon;
						}
						else 
						if (rp->r_timeout < time)
						/* outside ARE receive window 
						 * start counting from this
						 * frame
						 */
						     ++rp->r_ARP_mon;

						/* set reponse window timer */
						rp->r_timeout = time +
							      rt->parms->tx_resp;
					}
					else  /* unicast STE */
						++rp->r_STE_ucs;
				} else { /* no route, STE local */
					rp->r_tx_mon = 0;
					rp->r_STE_ucs = 0;
					rp->r_ARP_mon = 0;
					rp->r_timeout = time+rt->parms->tx_resp;
					dlpi_store_route(rp, ri);
				}
			}
			else {	/* is SRF or ARE */
				rp->r_tx_mon = 0;
				rp->r_STE_ucs = 0;
				if (IS_ARE(control0)) {
					if (rp->r_ARP_mon) {
						/* stop counting STE/SRF pairs
						 * and take over timer 
						 */
						rp->r_ARP_mon = 0;
						rp->r_timeout = 0;
					}	 
					if (rp->r_timeout < time) {
						/* outside ARE receive window */
						dlpi_store_route(rp, ri);
						rp->r_timeout= time +
							       rt->parms->rx_ARE;
					}
				}
				else {
					/* stop counting STE/SRF pairs */
					rp->r_ARP_mon = 0;
					rp->r_timeout = 0;
					dlpi_store_route(rp, ri);
				}
			}
			dlpi_link_inuse(rt, rp);
		}	
	}
	UNLOCK_RT(rt, sp);
}

/******************************************************************************
 * These functions return information about the internal
 * state of the SR route table.
 ******************************************************************************/

STATIC mblk_t *
dlpi_srtable_add(mblk_t *headmp, mblk_t *mp, struct route *rp)
{
	struct sr_table_entry *srp;
	uint	l;

	if (!mp || mp->b_wptr+sizeof(struct sr_table_entry) >= mp->b_datap->db_lim) {
		mblk_t *bp;

		/* was using MAXBSIZE here, but if strmaxblk is set to be
		 * less than MAXBSIZE, allocb() will return failure. So just
		 * set it an arbitrary size, say 4k.
		 */
		if ( !(bp = allocb(4096, BPRI_LO)) )
			return ((mblk_t *)0);
		if (mp)
			mp->b_cont = bp;
		else
			headmp->b_cont = bp;
		mp = bp;
	}
	srp = (struct sr_table_entry *)mp->b_wptr;
	mp->b_wptr += sizeof(struct sr_table_entry);
	bcopy(rp->r_mac_addr, srp->sr_remote_mac, 6);
	srp->sr_state = rp->r_state;
	srp->sr_timeout = rp->r_timeout;
	srp->sr_max_pdu = dlpi_framesz_to_len(rp->r_info.ri_control1);
	if (l = rp->r_info.ri_control0 & LEN_MASK)
		l = (l-2) >> 1;
	srp->sr_route_len = l;
	if (l && rp->r_info.ri_control1 & DIR_BACKWARD) {
		/* Route stored backwards, must re-order segments */
		ushort *x, *y;

		--l;
		x=rp->r_info.ri_segments+l;
		y=srp->sr_route;
		for (; l; --l) {
			*y = ((*x) & 0xf0ff) | ((*(x-1)) & 0x0f00);
			++y;
			--x;
		}
		*y = (*x) & 0xf0ff;
	} else {
		/* Route stored forwards */
		bcopy(rp->r_info.ri_segments, srp->sr_route, sizeof(ushort)*l);
	}
	return(mp);
}

STATIC int
dlpi_send_dl_srtable_ack(struct route_table *rt, queue_t *q)
{
	struct route *rp;
	int x;
	int count, sp;
	mblk_t *mp, *bp;
	dl_srtable_ack_t *ap;

	drv_getparm(TIME, &time);

	if ( !(mp = allocb(sizeof(dl_srtable_ack_t), BPRI_LO)) )
		return(0);
	mp->b_datap->db_type = M_PCPROTO;
	mp->b_wptr += sizeof(dl_srtable_ack_t);
	ap = (dl_srtable_ack_t *)mp->b_rptr;
	ap->dl_primitive = DL_SRTABLE_ACK;

	ap->dl_time_now = time;
	ap->dl_route_table_sz = rt->nroutes;
	ap->dl_routes_in_use = rt->ninuse;
  	ap->dl_tx_resp = rt->parms->tx_resp;
 	ap->dl_rx_ARE = rt->parms->rx_ARE;
  	ap->dl_rx_STE_bcs = rt->parms->rx_STE_bcs;
 	ap->dl_rx_STE_ucs = rt->parms->rx_STE_ucs;
 	ap->dl_rx_ARE = rt->parms->rx_ARE;
  	ap->dl_max_tx = rt->parms->max_tx;
 	ap->dl_min_tx = rt->parms->min_tx;
  	ap->dl_tx_recur = rt->parms->tx_recur;
 	ap->dl_ARE_disa = rt->parms->ARE_disa;
	
	sp = LOCK_RT(rt);
	count = 0;
	bp = (mblk_t *)0;
	for (rp = rt->disco; rp; rp=rp->r_next) {
		++count;
		if (!(bp = dlpi_srtable_add(mp, bp, rp)))
			goto end;
	}
	for (rp = rt->inuse; rp; rp=rp->r_next) {
		++count;
		if (!(bp = dlpi_srtable_add(mp, bp, rp)))
			goto end;
	}
end:
	UNLOCK_RT(rt, sp);
	ap->dl_srtable_len = count * sizeof(struct sr_table_entry);
	qreply(q, mp);
	return(1);
}

STATIC int
dlpi_do_set_srparms(struct route_table *rt, union DL_primitives *srp)
{
	register dl_set_srparms_req_t *brp;
	unchar *cp;

#ifdef SR_DEBUG
	cmn_err(CE_CONT, "old paramters: %x %x %x %x %x %x %x %x\n",
	rt->parms->tx_resp,
	rt->parms->rx_ARE,
	rt->parms->rx_STE_bcs,
	rt->parms->rx_STE_ucs,
	rt->parms->max_tx,
	rt->parms->min_tx,
	rt->parms->tx_recur,
	rt->parms->ARE_disa);
#endif
	brp=(dl_set_srparms_req_t *)srp;
	if (brp->dl_parms_len != sizeof(struct route_param))
		return(0);
	cp = (unchar *)brp;
	cp += brp->dl_parms_offset;
	bcopy(cp, rt->parms, sizeof(struct route_param));

#ifdef SR_DEBUG
	cmn_err(CE_CONT, "new paramters: %x %x %x %x %x %x %x %x\n",
	rt->parms->tx_resp,
	rt->parms->rx_ARE,
	rt->parms->rx_STE_bcs,
	rt->parms->rx_STE_ucs,
	rt->parms->max_tx,
	rt->parms->min_tx,
	rt->parms->tx_recur,
	rt->parms->ARE_disa);
#endif
	return(1);
}

int
dlpiSR_primitives(per_card_info_t *cp, queue_t *q, mblk_t *mp)
{
	struct route_table *rt = cp->route_table;
	union DL_primitives *prim = (union DL_primitives *)mp->b_rptr;
	dl_ok_ack_t *okack;

#ifdef SR_DEBUG
	cmn_err(CE_CONT, "dlpiSR_primitives: %x\n", prim->dl_primitive);
#endif

	switch (prim->dl_primitive)
	{
	case DL_SRTABLE_REQ:	/* Request entire source routing table */
		if (!dlpi_send_dl_srtable_ack(rt, q))
			return(0);
		break;

	case DL_SET_SRPARMS_REQ: /* Request to change SR module params */
		if (dlpi_do_set_srparms(rt, prim))
			goto ok_ack;
		return(0);

	case DL_CLR_SR_REQ:	/* clear the source routing table  */
		dlpiSR_reset(rt, rt->nroutes);
		goto ok_ack;

	default:
		return(0);
	}
	freemsg(mp);
	return(1);

ok_ack:
	okack = (dl_ok_ack_t *)mp->b_rptr;
	okack->dl_correct_primitive = prim->dl_primitive;
	okack->dl_primitive = DL_OK_ACK;
	mp->b_wptr = mp->b_rptr + sizeof(dl_ok_ack_t);
	qreply(q,mp);
	return(1);
}

