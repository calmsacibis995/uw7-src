#ident "@(#)util.c	29.1"
#ident "$Header$"

/*
 *	  Copyright (C) The Santa Cruz Operation, 1993-1996.
 *	  This Module contains Proprietary Information of
 *	  The Santa Cruz Operation and should be treated
 *	  as Confidential.
 */

#ifdef  _KERNEL_HEADERS

#include <io/nd/dlpi/include.h>

#else

#include "include.h"

#endif

/*
 * Send a message to the next module, or queue it up, or drop it
 */

dlpi_putnext(queue_t *q, mblk_t *mp)
{
	DLPI_PRINTF08(("DLPI: putnext(q=%x, q->q_first=%x,q->q_next=%x ,mp=%x)\n",
					q, q->q_first, q->q_next, mp));

	if (!(q->q_first) && canputnext(q))
		putnext(q, mp);
	else if (canput(q)) {
		DLPI_PRINTF08(("DLPI: putnext FAIL (queueing)\n"));
		putq(q, mp);
	} else {
		DLPI_PRINTF08(("DLPI: putnext FAIL\n"));
		freemsg(mp);
		return(0);		/* FAIL	*/
	}
	return(1);			/* SUCCEED */
}

/* Test to see if dlpi_putnext(q,mp) would succeed */
dlpi_canput(queue_t *q)
{
	if (!(q->q_first) && canputnext(q))
		return(1);
	else if (canput(q))
		return(1);

	DLPI_PRINTF08(("DLPI: canput(q=%x) FAIL\n", q));
	return(0);		/* FAIL	*/
}

/* initialize sap_counters structures */
dlpi_alloc_sap_counters(struct sap_counters *sc)
{
	DLPI_PRINTF02(("DLPI: dlpi_alloc_sap_counters(sc=%x)\n", sc));
	
	if ( !(sc->bcast = ATOMIC_INT_ALLOC(KM_NOSLEEP)) ) {
		cmn_err(CE_WARN, "dlpi_alloc_sap_counters: No memory for bcast atomic");
		return(0);
	}
	ATOMIC_INT_INIT(sc->bcast, 0);

	if ( !(sc->mcast = ATOMIC_INT_ALLOC(KM_NOSLEEP)) ) {
		cmn_err(CE_WARN, "dlpi_alloc_sap_counters: No memory for mcast atomic");
		ATOMIC_INT_DEALLOC(sc->bcast);
		return(0);
	}
	ATOMIC_INT_INIT(sc->mcast, 0);

	if ( !(sc->ucast_xid = ATOMIC_INT_ALLOC(KM_NOSLEEP)) ) {
		cmn_err(CE_WARN, "dlpi_alloc_sap_counters: No memory for ucast_xid atomic");
		ATOMIC_INT_DEALLOC(sc->bcast);
		ATOMIC_INT_DEALLOC(sc->mcast);
		return(0);
	}
	ATOMIC_INT_INIT(sc->ucast_xid, 0);

	if ( !(sc->ucast_test = ATOMIC_INT_ALLOC(KM_NOSLEEP)) ) {
		cmn_err(CE_WARN, "dlpi_alloc_sap_counters: No memory for ucast_test atomic");
		ATOMIC_INT_DEALLOC(sc->bcast);
		ATOMIC_INT_DEALLOC(sc->mcast);
		ATOMIC_INT_DEALLOC(sc->ucast_xid);
		return(0);
	}
	ATOMIC_INT_INIT(sc->ucast_test, 0);

	if ( !(sc->ucast = ATOMIC_INT_ALLOC(KM_NOSLEEP)) ) {
		cmn_err(CE_WARN, "dlpi_alloc_sap_counters: No memory for ucast atomic");
		ATOMIC_INT_DEALLOC(sc->bcast);
		ATOMIC_INT_DEALLOC(sc->mcast);
		ATOMIC_INT_DEALLOC(sc->ucast_xid);
		ATOMIC_INT_DEALLOC(sc->ucast_test);
		return(0);
	}
	ATOMIC_INT_INIT(sc->ucast, 0);

	if ( !(sc->error = ATOMIC_INT_ALLOC(KM_NOSLEEP)) ) {
		cmn_err(CE_WARN, "dlpi_alloc_sap_counters: No memory for error atomic");
		ATOMIC_INT_DEALLOC(sc->bcast);
		ATOMIC_INT_DEALLOC(sc->mcast);
		ATOMIC_INT_DEALLOC(sc->ucast_xid);
		ATOMIC_INT_DEALLOC(sc->ucast_test);
		ATOMIC_INT_DEALLOC(sc->ucast);
		return(0);
	}
	ATOMIC_INT_INIT(sc->error, 0);

	if ( !(sc->octets = ATOMIC_INT_ALLOC(KM_NOSLEEP)) ) {
		cmn_err(CE_WARN, "dlpi_alloc_sap_counters: No memory for octets atomic");
		ATOMIC_INT_DEALLOC(sc->bcast);
		ATOMIC_INT_DEALLOC(sc->mcast);
		ATOMIC_INT_DEALLOC(sc->ucast_xid);
		ATOMIC_INT_DEALLOC(sc->ucast_test);
		ATOMIC_INT_DEALLOC(sc->ucast);
		ATOMIC_INT_DEALLOC(sc->error);
		return(0);
	}
	ATOMIC_INT_INIT(sc->octets, 0);

	return(1);
}

/* wipe out sap_counters structures */
void
dlpi_dealloc_sap_counters(struct sap_counters *sc)
{
	DLPI_PRINTF02(("DLPI: dlpi_dealloc_sap_counters(sc=%x)\n", sc));
	
	ATOMIC_INT_DEALLOC(sc->bcast);
	ATOMIC_INT_DEALLOC(sc->mcast);
	ATOMIC_INT_DEALLOC(sc->ucast_xid);
	ATOMIC_INT_DEALLOC(sc->ucast_test);
	ATOMIC_INT_DEALLOC(sc->ucast);
	ATOMIC_INT_DEALLOC(sc->error);
	ATOMIC_INT_DEALLOC(sc->octets);
}

/* clear sap_counters */
void
dlpi_clear_sap_counters(struct sap_counters *sc)
{
	DLPI_PRINTF02(("DLPI: dlpi_clear_sap_counters(sc=%x)\n", sc));
	
	ATOMIC_INT_WRITE(sc->bcast, 0);
	ATOMIC_INT_WRITE(sc->mcast, 0);
	ATOMIC_INT_WRITE(sc->ucast_xid, 0);
	ATOMIC_INT_WRITE(sc->ucast_test, 0);
	ATOMIC_INT_WRITE(sc->ucast, 0);
	ATOMIC_INT_WRITE(sc->error, 0);
	ATOMIC_INT_WRITE(sc->octets, 0);
}

/*
 * Function: dlpi_allocsap
 *
 * Purpose:
 *   Return a free sap table entry.
 *
 * MP Comments:
 *   SAP table entries are allocated only at open, which is protected by the
 *   global dlpi_open_lock spin lock.  Note that under UW the open routine 
 *   can be called on any CPU unlike its OpenServer counterpart which runs
 *   the open routine on CPU 0.
 */

per_sap_info_t *
dlpi_allocsap(queue_t *q, per_card_info_t *cp, int srmode, cred_t *crp)
{
	per_sap_info_t	*sp = cp->sap_table;
	int			i;

	DLPI_PRINTF02(("DLPI: allocsap(q=%x, cp=%x, maxsaps=%d)\n",
							q, cp, cp->maxsaps));

	/* be careful about changing loop below-index of sp used as UW minor */
	for (i = cp->maxsaps; i >= 0; --i) {
		if (sp->sap_state == SAP_FREE) {
			break;
		}
		++sp;
	}

	if (i < 0)
		return ((per_sap_info_t *)NULL);

	if ( ! dlpi_alloc_sap_counters(&sp->sap_tx)) {
		return((per_sap_info_t *)NULL);
	}
	if ( ! dlpi_alloc_sap_counters(&sp->sap_rx)) {
		return((per_sap_info_t *)NULL);
	}

	sp->crp = crp;
	sp->sap_state = SAP_ALLOC;
	sp->card_info = cp;
	sp->up_queue = q;

	sp->llcmode = LLC_OFF;
	if (cp->media_specific &&
	   (cp->media_specific->media_flags & MEDIA_SOURCE_ROUTING))
		sp->srmode = srmode;
	else
		sp->srmode = SR_NON;

	sp->filter[DL_FILTER_INCOMING] = NULL;
	sp->filter[DL_FILTER_OUTGOING] = NULL;

	return(sp);
}

/*
 * Function: dlpi_findsap
 *
 * Purpose:
 *   Return a sap table entry based on the SAP value and framing type.
 *
 * MP Comments:
 *   Routines calling this function must obtain the cp->lock_sap_id
 *   lock to prevent more than one queue binding to a particular
 *   sap.
 */
per_sap_info_t *
dlpi_findsap(ulong type, ulong sap, per_card_info_t *cp)
{
	per_sap_info_t	*sp = cp->sap_table;
	int			i;

	DLPI_PRINTF10(("DLPI: findsap(type=%d,sap=%x)\n", type,sap));
	for (i=cp->maxsaps; i; --i) {
		if (sp->sap_state == SAP_BOUND && sp->sap_type == type &&
			sp->sap == sap)
				return(sp);
		++sp;
	}
	return ( (per_sap_info_t *)0 );
}

/*
 * Function: dlpi_findsnapsap
 *
 * Purpose:
 *   Return a snap sap table entry based on the SAP value and framing type.
 *
 * MP Comments:
 *   Routines calling this function must obtain the cp->lock_sap_id
 *   lock to prevent more than one queue binding to a particular
 *   sap.
 */
per_sap_info_t *
dlpi_findsnapsap(ulong prot_id, ulong type, per_card_info_t *cp)
{
	per_sap_info_t	*sp = cp->sap_table;
	int			i;

	DLPI_PRINTF10(("DLPI: findsnapsap(protid=%d,type=%x)\n", prot_id,type));
	for (i=cp->maxsaps; i; --i) {
		if (sp->sap_state == SAP_BOUND && sp->sap_type == FR_SNAP &&
			sp->sap_protid == prot_id && sp->sap == type)
				return(sp);
		++sp;
	}
	return ( (per_sap_info_t *)0 );
}


void
dlpi_dumpsaps(unsigned char * addr, int len, per_card_info_t *cp)
{
	int					 i;
	per_sap_info_t		*sp = cp->sap_table;
	struct dlpi_sapstats	*x = (struct dlpi_sapstats *)addr;
	pl_t			opl;				

	for (i = cp->maxsaps; i; --i) {
		if (sp->sap_state == SAP_BOUND) {
			x->dl_saptype = sp->sap_type;
			x->dl_sap = sp->sap;
			x->dl_llcmode = sp->llcmode;
			x->dl_srmode = sp->srmode;
			opl = freezestr(sp->up_queue);
			bcopy(sp->up_queue->q_next->q_qinfo->qi_minfo->mi_idname,
				  x->dl_user, DL_USER_SZ);
			unfreezestr(sp->up_queue, opl);
			x->dl_tx.dl_bcast = ATOMIC_INT_READ(sp->sap_tx.bcast);
			x->dl_tx.dl_mcast = ATOMIC_INT_READ(sp->sap_tx.mcast);
			x->dl_tx.dl_ucast = ATOMIC_INT_READ(sp->sap_tx.ucast);
			x->dl_tx.dl_ucast_xid = ATOMIC_INT_READ(sp->sap_tx.ucast_xid);
			x->dl_tx.dl_ucast_test = ATOMIC_INT_READ(sp->sap_tx.ucast_test);
			x->dl_tx.dl_error = ATOMIC_INT_READ(sp->sap_tx.error);
			x->dl_tx.dl_octets = ATOMIC_INT_READ(sp->sap_tx.octets);
			x->dl_rx.dl_bcast = ATOMIC_INT_READ(sp->sap_rx.bcast);
			x->dl_rx.dl_mcast = ATOMIC_INT_READ(sp->sap_rx.mcast);
			x->dl_rx.dl_ucast = ATOMIC_INT_READ(sp->sap_rx.ucast);
			x->dl_rx.dl_ucast_xid = ATOMIC_INT_READ(sp->sap_rx.ucast_xid);
			x->dl_rx.dl_ucast_test = ATOMIC_INT_READ(sp->sap_rx.ucast_test);
			x->dl_rx.dl_error = ATOMIC_INT_READ(sp->sap_rx.error);
			x->dl_rx.dl_octets = ATOMIC_INT_READ(sp->sap_rx.octets);
			x->dl_tx.dl_queue_len = qsize(WR(sp->up_queue));
			x->dl_rx.dl_queue_len = qsize(sp->up_queue);
			++x;
			len -= sizeof(struct dlpi_sapstats);
			if (!len)
					break;
		}
		++sp;
	}
}

uint
dlpi_hsh(unchar *dest, uint size)
{
	uint hsh = 0;
	uint i   = 0;

	while (i < 6) {
		hsh = *dest + (31 * hsh);
		dest++;
		i++;
	}
	return(hsh % size);
}

#ifdef DLPI_DEBUG

unsigned int dlpi_debug = 0;

void
dlpi_printf(char *fmt, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15)
{
	cmn_err(CE_CONT, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
}

#endif
