#ident	"@(#)emap.c	1.2"
#ident	"$Header$"

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */


/*
 *
 *	Copyright (C) The Santa Cruz Operation, 1985, 1986.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 *
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
 * Eight-bit or European character mapping
 * for line discipline 0.
 *
 *	MODIFICATION HISTORY
 *	L000	17 Feb 88	scol!craig
 *	- Add extra indirections to take account of new xmap tty structure
 *	  extension.  (Also, make local routines static, and add extra
 *	  condition to emmapout to prevent incorrectly mapping nulls).
 *	L001	28 Feb 88	scol!craig
 *	- Added hooks to call nmap routines.
 *	L002	02 Mar 88	scol!craig
 *	- Changes to allow doubled dead keys and reversible compose sequences.
 *	L003	11 Mar 88	scol!craig
 *	- Changes for multiple buffer tables, added debugging code
 *	L004	11 Mar 88	scol!craig
 *	- Changes for Doug's "shortcut" with output nmaps.
 *	S005	31 Dec 88	buckm
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
#include <proc/user.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <proc/proc.h>
#include <fs/file.h>
#include <io/intmap/xmap.h>
#include <io/intmap/emap.h>
#include <io/intmap/nmap.h>
#include <svc/systm.h>
#include <util/var.h>
#include <io/stream.h>
#include <mem/kmem.h>
#include <io/ddi.h>
#include <util/cmn_err.h>

#ifdef	SCO_DEBUG
#define I18N_DEBUG(a)	 cmn_err(CE_NOTE,  a)
#endif	

extern int str_xmapalloc(struct emp_tty *);
extern void str_emunmap(queue_t *, struct emp_tty *, int);
extern int str_nmmapout1(struct emp_tty *, unsigned char);
extern int str_nmmapin(struct emp_tty *, unsigned char);
extern int str_nmmapout2(struct emp_tty *, unsigned char);

STATIC struct emap *emaddmap(struct buf *);
STATIC int emcmpmap(struct buf *, struct buf *);
STATIC int str_emchkmap(struct buf *, mblk_t *);

/* Enhanced Application Compatibility Support */

#define	isdigit(c)	(c >= '0' && c <= '9')

/*
 * int
 * str_emenmap(queue_t *q, struct emp_tty *emp_tp)
 *
 *	The LDEMAP and LDDMAP IOCTLS conflict with 
 *	SCO events LDEV_SETTYPE, LDEV_GETEV put in i386/Version4
 *	The reference in ldterm.c for LDEMAP and LDDMAP is being
 *	removed for 4.1dt.  Thus calls from ldterm for 
 *	str_emenmap() will not be done.
 *
 * Calling/Exit State:
 */

/* ARGSUSED */
int
str_emenmap(queue_t *q, struct emp_tty *emp_tp)
{

#ifdef SCO_DEBUG
	I18N_DEBUG("str_nmunmap: ");
#endif
	if (emp_tp->t_mstate == ES_NULL)
		return(ENAVAIL);

	emp_tp->t_mstate = ES_START;

#ifdef SCO_DEBUG
	I18N_DEBUG("str_emenmap: returning ");
#endif
	return(0);
}

/* End Enhanced Application Compatibility Support */

/*
 * STATIC struct emap *
 * emaddmap(struct buf *bp)
 *
 * 	Add a new emap to the system.
 * 	Check for a match with existing emaps,
 * 	and increment the use count if a match is found.
 * 	Otherwise, grab an unused emap slot,
 * 	and setup info in the emap structure.
 *
 * Calling/Exit State:
 */
STATIC struct emap *
emaddmap(struct buf *bp)
{
	struct emap *mp, *fmp;
	emp_t ep;
	int i;

#ifdef SCO_DEBUG
	I18N_DEBUG("emaddmap: ");
#endif

	fmp = (struct emap *)NULL;
	mp = &emap[0];
	i = v.v_emap;
	while (--i >= 0) {
		if (mp->e_count == 0) {
			if (fmp == (struct emap *)NULL)
				fmp = mp;
		} else {
			if (emcmpmap(bp, mp->e_bp) == 0) {
				++mp->e_count;
				return(mp);
			}
		}
		++mp;
	}

	if (fmp) {
		ep = (emp_t)paddr(bp);
		fmp->e_count = 1;
		fmp->e_bp    = bp;
		fmp->e_ndind =
		    (ep->e_cind - E_DIND) / sizeof(struct emind);
		fmp->e_ncind =
		    ((ep->e_dctab - ep->e_cind) / sizeof(struct emind)) - 1;
		fmp->e_nsind =
		    (ep->e_stab - ep->e_sind) / sizeof(struct emind);

#ifdef SCO_DEBUG
		cmn_err(CE_CONT, "emaddmap: indices=%d stab=%d sind=%d \n",
			(int)fmp->e_nsind,(int)ep->e_stab,(int)ep->e_sind);
#endif
		/* set array of pointers to each buffer in chain	*L003*/
		for (i = 0; bp; i++) {				       /*L003*/
			fmp->e_tp[i] = (emp_t)paddr(bp);	       /*L003*/
			bp = bp->av_forw;			       /*L003*/
		}						       /*L003*/
	}

	return(fmp);
}


/*
 * STATIC int
 * emcmpmap(struct buf *bp1, struct buf *bp2)
 * 	Compare two emaps; return 0 if identical.
 *
 * Calling/Exit State:
 */
STATIC int
emcmpmap(struct buf *bp1, struct buf *bp2)
{
	int i;

#ifdef SCO_DEBUG
	I18N_DEBUG("emcmpmap: ");
#endif
	do {
		for (i = 0; i < E_TABSZ; i += sizeof(long))
			if (bigetl(bp1, i) != bigetl(bp2, i))
				return(1);
		bp1 = bp1->av_forw;
		bp2 = bp2->av_forw;
		if (bp1 == (struct buf *)NULL && bp2 == (struct buf *)NULL)
			return(0);
	} while (bp1 != (struct buf *)NULL && bp2 != (struct buf *)NULL);
	return(1);
}

#define	MAPGETB(mapt,i) (((emcp_t)((mapt)[(i)>>EMBSHIFT]))[(i)&EMBMASK])/*L003*/

/*
 * int
 * str_emsetmap(queue_t *q, mblk_t *str_bp, struct emp_tty *emp_tp)
 *
 * Calling/Exit State:
 */
int
str_emsetmap(queue_t *q, mblk_t *str_bp, struct emp_tty *emp_tp)
{
	struct xmap *xmp;
	struct emap *mp;
	struct buf *bp;
	int error = 0;

#ifdef SCO_DEBUG
	I18N_DEBUG("str_emsetmap: ");
#endif
	bp = getrbuf(KM_NOSLEEP);
	if (bp == (struct buf *) NULL)
		return (ENOMEM);
	bp->av_forw = (struct buf *) NULL;
	bp->b_un.b_addr = kmem_alloc(E_TABSZ,KM_NOSLEEP);
	if (paddr(bp) == (paddr_t) NULL) {
		freerbuf(bp);
		return (ENOMEM);
	}

	if (str_emchkmap(bp, str_bp)) {		/* validate emap */
		error = EINVAL;
		goto out;
	}
	/*
	 *	Get a new tty struct extension if required
	*/
	if (emp_tp->t_xmp == (struct xmap *) NULL ) str_xmapalloc(emp_tp);
	xmp = emp_tp->t_xmp;
	/*
	 * If this line is the only user of an emap,
	 * free it now to ensure success in emaddmap().
	 */
	if ((emp_tp->t_mstate != ES_NULL) && (xmp->xm_emp->e_count == 1))
		str_emunmap(q, emp_tp,1);

	mp = (struct emap *) emaddmap(bp);
	if (mp == (struct emap *)NULL) {	/* can't add emap */
		error = ENAVAIL;
		goto out;
	}
	if (mp->e_count == 1)			/* if this is a new emap, */
		bp = (struct buf *)NULL;	/*   don't free the buf   */
	if (emp_tp->t_mstate != ES_NULL)	/* free old emap	  */
		str_emunmap(q, emp_tp,1);		/*   if we still have one */
						/* to attach new mapping  */

	emp_tp->t_mstate = ES_START;		/* emapping enabled */
	emp_tp->t_xmp->xm_emp = mp;				/*   using new map  */

  out:
	while (bp) {
		struct buf *nbp = bp->av_forw;
		kmem_free((caddr_t)paddr(bp),E_TABSZ);
		freerbuf(bp);
		bp = nbp;
	}
	return(error);
}


/*
 * void
 * str_emunmap(queue_t *q, struct emp_tty *emp_tp, int wqflg)
 * 	Disable emapping on a line.
 * 	Decrement the use count of the emap,
 * 	and free it if the count becomes zero.
 *
 * Calling/Exit State:
 */
void
str_emunmap(queue_t *q, struct emp_tty *emp_tp, int wqflg)
{
	struct emap *mp;

#ifdef SCO_DEBUG
	I18N_DEBUG("str_emunmap: ");
#endif
	if (emp_tp->t_mstate == ES_NULL)
		return;

	if (q->q_next)
		(void) putnextctl(q, M_START);
	flushq(q,FLUSHDATA);
	if (wqflg) 
		flushq(RD(q), FLUSHDATA);
	else
		flushq(WR(q), FLUSHDATA);

	mp = emp_tp->t_xmp->xm_emp;	/* pointr to map buf for this tty */
	/* null out pointer to map buffer */
	emp_tp->t_xmp->xm_emp = (struct emap *)NULL;
	emp_tp->t_mstate = ES_NULL;	/* mapng is disabled for this tty */

	/*
	 * If no other channel uses this
	 * map buffer for mapping, release
	 * the buffer back to freelist.
	 */
	if (--mp->e_count == 0) {
		struct buf *bp = mp->e_bp;
		while (bp) {
			struct buf *nbp = bp->av_forw;
			kmem_free((caddr_t)paddr(bp),E_TABSZ);
			freerbuf(bp);
			bp = nbp;
		}
		mp->e_bp = (struct buf *)NULL;
	}
#ifdef SCO_DEBUG
	I18N_DEBUG("str_emunmap: state not NULL ");
#endif
}


/*
 * int
 * str_emgetmap(mblk_t *emp_tp, struct emp_tty *bpp)
 * 	Return the current emap in effect on a line. Store the data in bpp 
 *
 * Calling/Exit State:
 */
int
str_emgetmap(struct emp_tty *emp_tp, mblk_t *bpp)
{
	struct buf *bp;
	mblk_t *obp;
	int error = 0;
	caddr_t *arg;
#ifdef SCO_DEBUG
	struct emtab *ep;
#endif

#ifdef SCO_DEBUG
	I18N_DEBUG("str_emgetmap: ");
#endif
	if (emp_tp->t_mstate == ES_NULL) {
		error = ENAVAIL;
		goto inval;
	}
	bp = emp_tp->t_xmp->xm_emp->e_bp; /* get map buffer for this tty */
	obp = bpp;
	for (;;) {
		/* copy the channel mapping table into user buffer */
		/* LINTED pointer alignment */
		arg = (caddr_t *) obp->b_rptr;
		bcopy((caddr_t)paddr(bp), (caddr_t)arg, E_TABSZ);
		obp->b_wptr += E_TABSZ;
		if ((bp = bp->av_forw) == (struct buf *)  NULL)
			break;
		if ((obp->b_cont = allocb(E_TABSZ,BPRI_HI)) == (mblk_t *) NULL) {
			error = ENOMEM;
			goto inval;
		}
		obp = obp->b_cont;
	}
	obp->b_cont = NULL;
#ifdef SCO_DEBUG
	/* LINTED pointer alignment */
	ep = (struct emtab *) bpp->b_rptr;
	cmn_err(CE_CONT, "str_emgetmap: stab=%d sind=%d \n",
		ep->e_stab,ep->e_sind);
#endif

inval:
	return(error);
}

/*
 * int
 * str_emmapin(mblk_t *bp, struct emp_tty *emp_tp)
 * 	Given a pointer to and length of a string of characters,
 * 	map the string in place and return its new length.
 * 	The string will not expand, but may contract.
 * 	Tty structure emapping fields are affected.
 *
 * Calling/Exit State:
 */
int
str_emmapin(mblk_t *bp, struct emp_tty *emp_tp)
{
	int i;
	unsigned char c;
	unsigned char mc;
	unsigned char *cp = (unsigned char *)bp->b_rptr;
	unsigned char *ocp = cp;
	int err = 0;
	struct xmap *xmp;
	struct emap *mp;
	emp_t ep;
	emip_t eip;
	emop_t eop;
	int reversed;
	int indoff, outoff;

	xmp = emp_tp->t_xmp;
	mp = xmp->xm_emp;
	ep = mp->e_tp[0];
	while ( cp < bp->b_wptr ) {
	    c = *cp++;			/* Grab a char from string */
	    mc = ep->e_imap[c];			/* Index down to the imap table */

	    if (str_nmmapin(emp_tp, c)) switch (emp_tp->t_mstate) {
	/* Enhanced Application Compatibility Support */
	    case ES_OFF:
		if ( c != ep->e_toggle) {
		    *ocp++ = c;
		    continue ;
		    }
		else
		    {
		    emp_tp->t_mstate = ES_START;
		    break;
		    }
	/* End Enhanced Application Compatibility Support */
	    case ES_START:
		if ((mc != E_ESC) || (c == E_ESC)) {
			*ocp++ = mc;	/* Substitute char w/ its map char */
			continue;
		}
	/* Enhanced Application Compatibility Support */
		if (c == ep->e_toggle)
		{
		    emp_tp->t_mstate = ES_OFF ;
		    break;
		}
	/* End Enhanced Application Compatibility Support */
		if (c == ep->e_comp) {
			emp_tp->t_mstate = ES_COMP1;
		} else {
			xmp->xm_emchar = c;
			emp_tp->t_mstate = ES_DEAD;
		}
		break;

	    case ES_COMP1:
		if (mc == E_ESC) {
	/* Enhanced Application Compatibility Support */
			if (c == ep->e_comp || 
			    (ep->e_beep > 1 && c == ep->e_comp)) {
	/* End Enhanced Application Compatibility Support */
				++err;
			} else {
				xmp->xm_emchar = c;
				emp_tp->t_mstate = ES_COMP2;
			}
		} else {
			xmp->xm_emchar = c;
			emp_tp->t_mstate = ES_COMP2;
		}
		break;

	/* Enhanced Application Compatibility Support */
	    case ES_DEC:
	    if (! isdigit(mc)) {
		++err;
		}
	    else
		{
		*ocp++ = emp_tp->t_mchar + (mc - '0') ;
		}
	    emp_tp->t_mstate = ES_START;
	    break ;
	/* End Enhanced Application Compatibility Support */
	    case ES_DEAD:
		if (mc == E_ESC) {
			if (c == ep->e_comp) {
				++err;
				emp_tp->t_mstate = ES_COMP1;
				break;
			}
			mc = c;
		}
		indoff = E_DIND;
		i = mp->e_ndind;
	/* Enhanced Application Compatibility Support */
		c = emp_tp->t_mchar; 
	/* End Enhanced Application Compatibility Support */
		goto dcsearch;

	    case ES_COMP2:
		if (mc == E_ESC) {
	/* Enhanced Application Compatibility Support */
			if (c == ep->e_comp ||
			    (ep->e_beep > 1 && c == ep->e_toggle)) {
	/* End Enhanced Application Compatibility Support */
				++err;
				emp_tp->t_mstate = ES_COMP1;
				break;
			}
			mc = c;
		}
		reversed = 0;
scanagain:
	/* Enhanced Application Compatibility Support */
		c = emp_tp->t_mchar ;
		if ( isdigit(c) && isdigit (mc) )
		{
		    emp_tp->t_mchar = (c - '0')*100 + (mc - '0')*10 ;
		    emp_tp->t_mstate = ES_DEC;
		    break;
		}
	/* End Enhanced Application Compatibility Support */

		indoff = ep->e_cind;
		i = mp->e_ncind;

  dcsearch:
	/* Enhanced Application Compatibility Support */
	 	if(ep->e_beep > 1)
			/*deadkey this character is supposed to
		    	  belong to */
			c = emp_tp->t_mchar; 
		else
	/* End Enhanced Application Compatibility Support */
		c = xmp->xm_emchar;			   /*L000*/
		while (--i >= 0) {
			eip = (emip_t)&MAPGETB(mp->e_tp, indoff);
			if (eip->e_key == c)
				break;
			indoff += sizeof(struct emind);
		}
		if (i >= 0) {
			i = ((emip_t)&MAPGETB(mp->e_tp, indoff))->e_ind;
			outoff = ep->e_dctab + i * sizeof(struct emout);
			indoff += sizeof (struct emind);
			i = ((emip_t)&MAPGETB(mp->e_tp, indoff))->e_ind - i;
			c = mc;
			while (--i >= 0) {
				eop=(emop_t)&MAPGETB(mp->e_tp, outoff);
				if (eop->e_key == c) {
					if ( emp_tp->t_mstate == ES_COMP2 && eop->e_out == E_ESC)
						goto reject;
					*ocp++ = eop->e_out;
					break;
				}
				outoff += sizeof(struct emout);
			}
		}
		if (i < 0)
		{
			if (emp_tp->t_mstate == ES_COMP2 && !reversed ) {
				reversed = 1;
				c = mc;
				mc = xmp->xm_emchar;
				xmp->xm_emchar = c;
				goto scanagain;
			}
reject:
			++err;
		}
		emp_tp->t_mstate = ES_START;
		break;

	    } /* end switch */
	   else ocp++;

	} /* end while */

	/* Enhanced Application Compatibility Support */

	if(ep->e_beep == 1)
		emp_tp->t_merr = err && ep->e_beep;
	else	
		emp_tp->t_merr = (char)err ;

	/* End Enhanced Application Compatibility Support */

	bp->b_wptr = ocp;
	return(ocp - (uchar_t *)bp->b_rptr);
}

/*
 * emcp_t
 * str_emmapout(struct emp_tty *emp_tp, unsigned char c, int *pnc)
 * 	Do output emapping; called by ttxput().
 * 	Given a character, return a pointer to and the length of
 * 	the string of characters to which it maps.
 *
 * Calling/Exit State:
 */
emcp_t
str_emmapout(struct emp_tty *emp_tp, unsigned char c, int *pnc)
{
	int i;
	struct emap *mp;
	emp_t ep;
	emcp_t ecp;
	emip_t eip;
	static unsigned char savec;
	int indoff;

	if (emp_tp->t_xmp->xm_emonmap == 0 || str_nmmapout1(emp_tp, c)) {
		mp = emp_tp->t_xmp->xm_emp;
		ep = mp->e_tp[0];
		ecp = (emcp_t)&ep->e_omap[c];
		if (*ecp == E_ESC && c != E_ESC) {
			indoff = ep->e_sind;
			i = mp->e_nsind;
			while (--i >= 0) {
				eip = (emip_t)&MAPGETB(mp->e_tp, indoff );
				if (eip->e_key == c) {
					i = eip->e_ind;
					indoff += sizeof(struct emind);
					eip = (emip_t)&MAPGETB(mp->e_tp, indoff );
					*pnc = eip->e_ind - i;
					if (*pnc == 0) {
						str_nmmapout2(emp_tp, c);
						goto nomap;
					}
					return((emcp_t)
						&MAPGETB(mp->e_tp, ep->e_stab + i));
				}
				indoff += sizeof(struct emind);
			}
		}
	} else {
nomap:
		savec = c;
		ecp = (emcp_t)&savec;
	}
	*pnc = 1;
	return(ecp);
}

/*	Enhanced Application Compatibility */
#define	MAPGETB(mapt,i) (((emcp_t)((mapt)[(i)>>EMBSHIFT]))[(i)&EMBMASK])   

/*
 * STATIC int
 * str_emchkmap(struct buf *bp, mblk_t *str_bp)
 * 	Check the validity of an emap; return 0 if ok.
 * 	A completely consistent emap is a user/utility responsibility.
 * 	We just check for indices and offsets that would cause us to
 * 	stray outside the emap.
 *
 * Calling/Exit State:
 */
STATIC int
str_emchkmap(struct buf *bp, mblk_t *str_bp)
{
	int n;
	int ndind, ncind, ndcout, nsind, nschar;
	emp_t ep;
	emip_t eip;
	int nbufs, indoff;
	emp_t emtabp[NEMBUFS];
	caddr_t	*arg;
	mblk_t *str_bp1;

#ifdef SCO_DEBUG
	I18N_DEBUG("str_emchkmap: ");
#endif
	/* LINTED pointer alignment */
	arg = (caddr_t *)str_bp->b_rptr;
	str_bp1 = str_bp;
	/* Read in first buffer */
	bcopy((caddr_t)arg, (caddr_t)paddr(bp), E_TABSZ);
	emtabp[0] = ep = (emp_t)paddr(bp);

	/* Check how many buffers needed */
	nbufs = (ep->e_stab >> EMBSHIFT) + 1;
	if (nbufs < 1 || nbufs > NEMBUFS) {
#ifdef SCO_DEBUG
		cmn_err(CE_CONT, "str_emchkmap invalid bufs %d: ",nbufs);
#endif
		goto inval;
	}

	/* Allocate and read in any extra buffers */
	for (n = 1; n < nbufs; n++) {
		if ((struct buf *) NULL == \
			(bp->av_forw = getrbuf(KM_NOSLEEP))) 
				return ENOMEM;
		bp->av_forw->b_un.b_addr = kmem_alloc(E_TABSZ,KM_NOSLEEP);
		if (paddr(bp->av_forw) == (paddr_t) NULL) {
			freerbuf(bp->av_forw);
			return ENOMEM;
		}
		bp = bp->av_forw;
		bp->av_forw = (struct buf *)NULL;
		if ( str_bp1->b_cont == NULL ) {
#ifdef SCO_DEBUG
			I18N_DEBUG("str_emchkmap bad message  ");
#endif
			goto inval;
		}
		str_bp1 = str_bp1->b_cont;
		/* LINTED pointer alignment */
		arg = (caddr_t *)str_bp1->b_rptr;
		bcopy((caddr_t)arg, (caddr_t)paddr(bp), E_TABSZ);
		emtabp[n] = (emp_t)paddr(bp);
	}

	/* check table offsets */

	n = ep->e_cind - E_DIND;
	ndind = n / sizeof(struct emind);
	if ((n < 0) || (n % sizeof(struct emind)) ||
	    (ep->e_cind > ((nbufs<<EMBSHIFT) - 2*sizeof(struct emind)))) {
		goto inval;
	}

	n = ep->e_dctab - ep->e_cind;
	ncind = n / sizeof(struct emind);
	if ((n < 0) || (n % sizeof(struct emind)) ||
	    (ep->e_dctab > ((nbufs << EMBSHIFT) - sizeof(struct emind)))) {
		goto inval;
	}

	n = ep->e_sind - ep->e_dctab;
	ndcout = n / sizeof(struct emout);
	if ((n < 0) || (n % sizeof(struct emout)) ||
	    (ep->e_sind > (nbufs << EMBSHIFT))) {
		goto inval;
	}

	n = ep->e_stab - ep->e_sind;
	nsind = n / sizeof(struct emind);
	nschar = (nbufs << EMBSHIFT) - ep->e_stab;
	if ((n < 0) || (n % sizeof(struct emind)) || (nschar < 0)) {
		goto inval;
	}

	/* check dead/compose indices */
	indoff = E_DIND;
	n = ndind + ncind;
	while (--n > 0) {
		eip = (emip_t)&MAPGETB(emtabp, indoff);
		if (eip[1].e_ind < eip[0].e_ind && eip[1].e_ind ) {
			goto inval;
		}
		indoff += sizeof(struct emind);
	}
	if ((n == 0) && (eip->e_ind > (uchar_t)ndcout) ) {
		goto inval;
	}

	/* check string indices */
	indoff = ep->e_sind;
	n = nsind;
	while (--n > 0) {
		eip = (emip_t)&MAPGETB(emtabp, indoff);
		if (eip[1].e_ind < eip[0].e_ind && eip[1].e_ind ) {
			goto inval;
		}
		indoff += sizeof(struct emind);
	}
	eip = (emip_t)&MAPGETB(emtabp, indoff);
	if ((n == 0) && (eip->e_ind > (uchar_t)nschar)) {
		goto inval;
	}

	/* looks like a usable map */
	return(0);

	/* Inconsistent map: */
inval:	
	return  EINVAL;
}
/*	End Enhanced Application Compatibility */
