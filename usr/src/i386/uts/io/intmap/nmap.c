#ident	"@(#)nmap.c	1.2"
#ident	"$Header$"

/*
 *	Copyright (C) The Santa Cruz Operation, 1988-1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * The code marked with symbols from the list below, is owned
 * by The Santa Cruz Operation Inc., and represents SCO value
 * added portions of source code requiring special arrangements
 * with SCO for inclusion in any product.
 *  Symbol:		 Market Module:
 * SCO_BASE 		Platform Binding Code 
 * SCO_ENH 		Enhanced Device Driver
 * SCO_ADM 		System Administration & Miscellaneous Tools 
 * SCO_C2TCB 		SCO Trusted Computing Base-TCB C2 Level 
 * SCO_DEVSYS 		SCO Development System Extension 
 * SCO_INTL 		SCO Internationalization Extension
 * SCO_BTCB 		SCO Trusted Computing Base TCB B Level Extension 
 * SCO_REALTIME 	SCO Realtime Extension 
 * SCO_HIGHPERF 	SCO High Performance Tape and Disk Device Drivers	
 * SCO_VID 		SCO Video and Graphics Device Drivers (2.3.x)		
 * SCO_TOOLS 		SCO System Administration Tools				
 * SCO_FS 		Alternate File Systems 
 * SCO_GAMES 		SCO Games
 */
							/* BEGIN SCO_INTL */
/*
 *	No-mapping routines - complement those in emap.c
 *
 *	MODIFICATION HISTORY
 *	created	18 Feb 88	scol!craig
 *	L001	31 Mar 88	scol!craig
 *	- Split emmapout into two to implement Doug's "shortcut" to
 *	  speed up catting /etc/termcap on the console.
 *	S002	31 Dec 88	buckm
 *	- Changes for MP.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <fs/buf.h>
#include <proc/signal.h>
#include <proc/seg.h>
#include <io/conf.h>
#include <mem/page.h>
#include <proc/user.h>
#include <mem/immu.h>
#include <proc/proc.h>
#include <fs/file.h>
#include <io/intmap/nmap.h>
#include <io/intmap/emap.h>
#include <io/intmap/xmap.h>
#include <svc/systm.h>
#include <util/var.h>
#include <io/stream.h>
#include <mem/kmem.h>
#include <io/ddi.h>
#include <util/cmn_err.h>

#ifdef	SCO_DEBUG
#define I18N_DEBUG(a)	 cmn_err(CE_CONT,  a)
#endif

extern int str_xmapalloc(struct emp_tty *);

void str_nmunmap(queue_t *, struct emp_tty *, int);

STATIC struct nxmap * nmaddmap(struct buf *);
STATIC int nmcmpmap(struct buf *, struct buf *);

/*
 * STATIC struct nxmap *
 * nmaddmap(struct buf *bp)
 *
 * 	Add a new nmap to the systnm.
 * 	Check for a match with existing nmaps,
 * 	and increment the use count if a match is found.
 * 	Otherwise, grab an unused nmap slot,
 * 	and setup info in the nmap structure.
 *
 * Calling/Exit State:
 */
STATIC struct nxmap *
nmaddmap(struct buf *bp)
{
	struct nxmap *mp, *fmp;
	int i;

#ifdef SCO_DEBUG
	I18N_DEBUG("nmaddmap: ");
#endif

	fmp = (struct nxmap *)NULL;
	mp = &nxmap[0];
	i = v.v_emap;
	while (--i >= 0) {
		if ( mp->n_count)
		++mp;
	}
	mp = &nxmap[0];
	i = v.v_emap;
	while (--i >= 0) {
		if (mp->n_count == 0) {
			if (fmp == (struct nxmap *)NULL)
				fmp = mp;
		} else {
		   if (mp->n_bp != (struct buf *) NULL ) {
			if (nmcmpmap(bp, mp->n_bp) == 0) {
				++mp->n_count;
				return(mp);
			}
		   }
		}
		++mp;
	}

	if (fmp) {
		bzero((caddr_t)fmp, sizeof(struct nxmap));
		fmp->n_count = 1;
		fmp->n_bp    = bp;
		fmp->n_p     = (nmp_t)paddr(bp);
	}

#ifdef SCO_DEBUG
	cmn_err(CE_CONT, "nmaddmap: added i=%d ",i);
#endif
	return(fmp);
}


/*
 * STATIC int
 * nmcmpmap(struct buf *bp1, struct buf *bp2)
 * 	Compare two nmaps; return 0 if identical.
 *
 * Calling/Exit State:
 */
STATIC int
nmcmpmap(struct buf *bp1, struct buf *bp2)
{
	int i;

#ifdef SCO_DEBUG
	I18N_DEBUG("nmcmpmap: ");
#endif
	for (i = 0; i < E_TABSZ; i += sizeof(long)) {
		if (bigetl(bp1, i) != bigetl(bp2, i)) {
			return(1);
		}
	}
	return(0);
}


/*
 * STATIC int
 * nmchkmap(struct buf *bp)
 * 	Check the validity of an nmap; return 0 if ok.
 * 	A completely consistent nmap is a user/utility responsibility.
 * 	We just check for indices and offsets that would cause us to
 * 	stray outside the nmap.
 *
 * Calling/Exit State:
 */
STATIC int
nmchkmap(struct buf *bp)
{
	nmp_t nmp;
	nmsp_t nmsp;
	nmcp_t nmcp;
	short nseqs, i;

#ifdef SCO_DEBUG
	I18N_DEBUG("nmchkmap: ");
#endif
	nmp = (nmp_t)paddr(bp);

	/* Check for table obviously bigger than buffer */
	if ((nseqs = nmp->n_aseqs) >
		((E_TABSZ - 2) / (sizeof(short) + sizeof(struct nmseq))))
		return(1);

	/* Check that each sequence begins immediately after the last one,
	   and that the whole string is within the buffer */
	nmsp = (nmsp_t)(((nmcp_t)nmp) + 2 + nseqs * sizeof(short));
	for (i = 0; i < nseqs; i++) {
		if (nmsp != (nmsp_t)(((nmcp_t)nmp) + nmp->n_seqidx[i]))
			return(1);
		nmcp = (nmcp_t)(nmsp->n_nmseq);	/* Stupid compiler! */
		do {
			if (nmcp > ((nmcp_t)nmp) + E_TABSZ)
				return(1);
		} while (*nmcp++);
		nmsp = (nmsp_t)nmcp;
	}

	/* looks like a usable map */
	return(0);
}

/*
 * int
 * str_nmsetmap(queue_t *q, mblk_t *str_bp, struct emp_tty *emp_tp)
 * 	Enable nmapping with a new nmap on a line.
 * 	Called from ttiocom().
 *
 * Calling/Exit State:
 */
int
str_nmsetmap(queue_t *q, mblk_t *str_bp, struct emp_tty *emp_tp)
{
	struct xmap *xmp;
	struct nxmap *mp;
	struct buf *bp;
	int error = 0;

#ifdef SCO_DEBUG
	I18N_DEBUG("str_nmsetmap: ");
#endif
	bp = getrbuf(KM_NOSLEEP);	/* get an outboard buf to hold nmap */
	if (bp == (struct buf *) NULL )
		return (ENOMEM);
	bp->av_forw = (struct buf *) NULL;
	bp->b_un.b_addr = kmem_alloc(E_TABSZ,KM_NOSLEEP);
	if (paddr(bp) == (paddr_t) NULL) {
		freerbuf(bp);
		return (ENOMEM);
	}
	bcopy((caddr_t)str_bp->b_rptr, (caddr_t)paddr(bp), E_TABSZ);
	if (nmchkmap(bp)) {		/* validate nmap */
		error = EINVAL;
		goto out;
	}
	/*
	 *	Get a new tty struct extension if required
	 */
	if (emp_tp->t_xmp == (struct xmap *)NULL) str_xmapalloc(emp_tp);
	xmp = emp_tp->t_xmp;
	/*
	 * If this line is the only user of an nmap,
	 * free it now to ensure success in nmaddmap().
	 */
	if ((xmp->xm_nmp != (struct nxmap *)NULL) &&
	    (xmp->xm_nmp->n_count == 1))
		str_nmunmap(q, emp_tp, 1);
	mp = nmaddmap(bp);
	if (mp == (struct nxmap *)NULL) {	/* can't add nmap */
#ifdef SCO_DEBUG
		I18N_DEBUG("str_nmsetmap: nmaddmap failed ");
#endif
		error = ENAVAIL;
		goto out;
	}
	if (mp->n_count == 1)			/* if this is a new nmap, */
		bp = (struct buf *)NULL;	/*   don't free the buf   */
	if (xmp->xm_nmp != (struct nxmap *)NULL)	/* free old nmap	  */
		str_nmunmap(q, emp_tp, 1);		/*   if we still have one */

	xmp->xm_nmp = mp;			/* use new nmap */

  out:
	if (bp) {
		kmem_free((caddr_t)paddr(bp),E_TABSZ);
		freerbuf(bp);
	}
	return(error);
}

/*
 * int
 * str_nmgetmap(mblk_t *emp_tp, struct emp_tty *bpp)
 * 	Return the current nmap in effect on a line.
 *
 * Calling/Exit State:
 */
int
str_nmgetmap(struct emp_tty *emp_tp, mblk_t *bpp)
{
	struct buf *bp;
	struct xmap *xmp;
	int error = 0;
	caddr_t *arg;

#ifdef SCO_DEBUG
	I18N_DEBUG("str_nmgetmap: ");
#endif
	if ((xmp = emp_tp->t_xmp) == (struct xmap *)NULL ||
	    xmp->xm_nmp == (struct nxmap *)NULL) {
		error = ENAVAIL;
	}
	else {
		/* get map buffer for this tty */
		bp = emp_tp->t_xmp->xm_nmp->n_bp;
		/* LINTED pointer alignment */
		arg = (caddr_t *) bpp->b_rptr;
		bcopy((caddr_t)paddr(bp), (caddr_t)arg, E_TABSZ);
		bpp->b_wptr += E_TABSZ;
	}
	return(error);
}

/*
 * void
 * str_nmunmap(queue_t *q, struct emp_tty *emp_tp, int wqflg)
 * 	Disable nmapping on a line.
 * 	Decrement the use count of the nmap,
 * 	and free it if the count becomes zero.
 * 	Called from ttioctl() and nmsetmap().
 *
 * Calling/Exit State:
 */
void
str_nmunmap(queue_t *q, struct emp_tty *emp_tp, int wqflg)
{
	struct xmap *xmp;
	struct nxmap *mp;

#ifdef SCO_DEBUG
	I18N_DEBUG("str_nmunmap: ");
#endif
	if ((xmp = emp_tp->t_xmp) == (struct xmap *)NULL ||
	    xmp->xm_nmp == (struct nxmap *)NULL)
		return;

	if (q->q_next)
		(void) putnextctl(q, M_START);
	flushq(q,FLUSHDATA);
	if (wqflg)
		flushq(RD(q), FLUSHDATA);
	else
		flushq(WR(q), FLUSHDATA);

	mp = xmp->xm_nmp;
	xmp->xm_nmp = (struct nxmap *)NULL;

 	if (--mp->n_count == 0) {
		kmem_free((caddr_t)paddr(mp->n_bp),E_TABSZ);
		freerbuf(mp->n_bp);
		mp->n_bp = (struct buf *)NULL;
	}
#ifdef SCO_DEBUG
	I18N_DEBUG("str_nmunmap: returning ");
#endif
}

/*
 * int
 * str_nmmapin(struct emp_tty *emp_tp, unsigned char c)
 * 	Decide whether input char should be mapped; called by emmapin().
 * 	Returns non-zero if char should be mapped.
 * 	Tty structure extension (struct xmap) nmapping fields are affected.
 *
 * Calling/Exit State:
 */
int
str_nmmapin(struct emp_tty *emp_tp, unsigned char c)
{
    struct xmap *xmp;
    nmp_t nmp;
    nmsp_t nmsp, newseqp;
    nmcp_t oscp, nscp;
    int i;

    /*
     *  If no nmap is set up, map
     */
    if ((xmp = emp_tp->t_xmp) == (struct xmap *)NULL ||
	xmp->xm_nmp == (struct nxmap *)NULL)
	return(1);

    nmp = xmp->xm_nmp->n_p;

scan:
    /*
     *  If in a trailer, don't map
     */
    if (xmp->xm_nmincnt) {
	xmp->xm_nmincnt--;
	return(0);
    }

    /*
     *  If in a lead-in, check next char
     */
    if (xmp->xm_nmiseqn) {
	nmsp = (nmsp_t)(((caddr_t)nmp) + nmp->n_seqidx[xmp->xm_nmiseqn - 1]);
				/* Get pointer to current sequence structure */
	if (!*(xmp->xm_nmiseqp)) {		/* End of lead-in */
	    xmp->xm_nmiseqn = 0;
	    xmp->xm_nmincnt = nmsp->n_nmcnt;
	    goto scan;
	}
	if (c == *(xmp->xm_nmiseqp)) {	/* Lead-in continues */
	    xmp->xm_nmiseqp++;
	    return(0);			/* Don't map */
	}
	/* Scan following sequences for match */
	i = xmp->xm_nmiseqn - 1;
	while (++i < (int)nmp->n_iseqs) {
	    newseqp = (nmsp_t)(((caddr_t)nmp) + nmp->n_seqidx[i]);
	    oscp = (nmcp_t)nmsp->n_nmseq;	/* Stupid compiler! */
	    nscp = (nmcp_t)newseqp->n_nmseq;	/* Stupid compiler! */
	    while (*(oscp++) == *(nscp++)) {
		if (oscp >= (nmcp_t)xmp->xm_nmiseqp) {
		    /*** ASSERT(oscp == xmp->xm_nmiseqp); ***/
		    if (!*nscp) {		/* At end of new lead-in */
			xmp->xm_nmiseqn = 0;
			xmp->xm_nmincnt = newseqp->n_nmcnt;
			goto scan;
		    }
		    if (c == *nscp) {		/* New lead-in continues */
			xmp->xm_nmiseqn = i + 1;
			xmp->xm_nmiseqp = (char *)nscp + 1;
			return(0);
		    }
		    break;			/* New lead-in doesn't match */
		}
	    }
	}
	xmp->xm_nmiseqn = 0;
	xmp->xm_nmincnt = 0;
	return(1);
    }

    /*
     *	Check for first char of any lead-in
     */
    for (i = 0; i < (int)nmp->n_iseqs; i++) {
	nmsp = (nmsp_t)(((caddr_t)nmp) + nmp->n_seqidx[i]);
	if (c == *(nmsp->n_nmseq)) {
	    xmp->xm_nmiseqn = i + 1;
	    xmp->xm_nmiseqp = (char *)nmsp->n_nmseq + 1;
	    return(0);
	}
    }

    /*
     *	Not in a control sequence, so map
     */
    return(1);
}


/*
 * int
 * str_nmmapout1(struct emp_tty *emp_tp, unsigned char c)
 *
 * 	Decide whether output char should be mapped; called by emmapout().
 * 	Returns non-zero if char should be mapped.
 * 	Tty structure extension (struct xmap) nmapping fields are affected.
 * 	nmmapout1 does not check for starting new sequences - this is assumed
 * 	to be detected by emmapout finding a null string in it's output map.
 *
 * Calling/Exit State:
 */
int
str_nmmapout1(struct emp_tty *emp_tp, unsigned char c)
{
    struct xmap *xmp;
    nmp_t nmp;
    nmsp_t nmsp, newseqp;
    nmcp_t oscp, nscp;
    int i;

    /*
     *  If no nmap is set up, map
     */
    if ((xmp = emp_tp->t_xmp) == (struct xmap *)NULL)		/*begin	*L001*/
	return(1);

    if (xmp->xm_nmp == (struct nxmap *)NULL) {
	xmp->xm_emonmap = 0;
	return(1);
    }								/*end	*L001*/

    nmp = xmp->xm_nmp->n_p;

scan:
    /*
     *  If in a trailer, don't map
     */
    if (xmp->xm_nmoncnt) {
	xmp->xm_nmoncnt--;
	return(0);
    }

    /*
     *  If in a lead-in, check next char
     */
    if (xmp->xm_nmoseqn) {
	nmsp = (nmsp_t)(((caddr_t)nmp) + nmp->n_seqidx[xmp->xm_nmoseqn - 1]);
				/* Get pointer to current sequence structure */
	if (!*(xmp->xm_nmoseqp)) {		/* End of lead-in */
	    xmp->xm_nmoseqn = 0;
	    xmp->xm_nmoncnt = nmsp->n_nmcnt;
	    goto scan;
	}
	if (c == *(xmp->xm_nmoseqp)) {	/* Lead-in continues */
	    xmp->xm_nmoseqp++;
	    return(0);			/* Don't map */
	}
	/* Scan following sequences for match */
	i = xmp->xm_nmoseqn - 1;
	while (++i < (int)nmp->n_aseqs) {
	    newseqp = (nmsp_t)(((caddr_t)nmp) + nmp->n_seqidx[i]);
	    oscp = (nmcp_t)nmsp->n_nmseq;	/* Stupid compiler! */
	    nscp = (nmcp_t)newseqp->n_nmseq;	/* Stupid compiler! */
	    while (*(oscp++) == *(nscp++)) {
		if (oscp >= (nmcp_t)xmp->xm_nmoseqp) {
		    /*** ASSERT(oscp == xmp->xm_nmoseqp); ***/
		    if (!*nscp) {		/* At end of new lead-in */
			xmp->xm_nmoseqn = 0;
			xmp->xm_nmoncnt = newseqp->n_nmcnt;
			goto scan;
		    }
		    if (c == *nscp) {		/* New lead-in continues */
			xmp->xm_nmoseqn = i + 1;
			xmp->xm_nmoseqp = (char *)nscp + 1;
			return(0);
		    }
		    break;			/* New lead-in doesn't match */
		}
	    }
	}
	xmp->xm_nmoseqn = 0;
	xmp->xm_nmoncnt = 0;
	xmp->xm_emonmap = 0;					       /*L001*/
	return(1);
    }

    /*
     *	Return true without checking for a new sequence.
     *	Actually, if we get here, nmmapout shouldn't have been called
     *	in the first place.
     */
    xmp->xm_emonmap = 0;					       /*L001*/
    return(1);
}

/*
 * int
 * str_nmmapout2(struct emp_tty *emp_tp, unsigned char c)
 *	This is the missing part of emmapout1:
 *
 * Calling/Exit State:
 */
int
str_nmmapout2(struct emp_tty *emp_tp, unsigned char c)
{
    struct xmap *xmp;
    nmp_t nmp;
    nmsp_t nmsp;
    int i;

    /*
     *  If no nmap is set up, return TRUE (shouldn't really happen).
     */
    if ((xmp = emp_tp->t_xmp) == (struct xmap *)NULL ||
	xmp->xm_nmp == (struct nxmap *)NULL)
	return(1);

    nmp = xmp->xm_nmp->n_p;

    /*
     *	Check for first char of any lead-in
     */
    for (i = (int)nmp->n_iseqs; i < (int)nmp->n_aseqs; i++) {
	nmsp = (nmsp_t)(((caddr_t)nmp) + nmp->n_seqidx[i]);
	if (c == *(nmsp->n_nmseq)) {
	    xmp->xm_nmoseqn = i + 1;
	    xmp->xm_nmoseqp = (char *)nmsp->n_nmseq + 1;
    	    xmp->xm_emonmap = 1;				       /*L001*/
	    return(0);
	}
    }

    /*
     *	Didn't find one, return TRUE (shouldn't happen).
     */
    xmp->xm_emonmap = 0;					       /*L001*/
    return(1);
}								/*end	*L001*/
