#ident	"@(#)chanmap.c	1.2"
#ident	"$Header$"
/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

/* 
 * Routines used by ldterm to handle XENIX channel mapping.
 */

#include <util/param.h>
#include <util/types.h>
#include <io/termios.h>
#include <io/termio.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/intmap/nmap.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#define CHANMAP_INCLUDE
#include <io/ldterm/ldterm.h>
#include <io/ddi.h>
/*
#include <io/ldterm/euc.h>
#include <io/ldterm/eucioctl.h>
*/

#define READ_EMAP_BUFS 1

/*
 * external function declarations
 */

extern int str_emmapin(mblk_t *, struct emp_tty *);
extern void str_emunmap(queue_t *, struct emp_tty *, int);
extern int str_emgetmap(struct emp_tty *, mblk_t *);
extern int str_emsetmap(queue_t *, mblk_t *, struct emp_tty *);
extern int str_nmgetmap(struct emp_tty *, mblk_t *);
extern int str_nmsetmap(queue_t *, mblk_t *, struct emp_tty *);
extern void str_nmunmap(queue_t *, struct emp_tty *, int);
extern void ldterm_outchar(uchar_t, queue_t *, int, lstate_t *);

/*
 * void
 * chanmap_close(queue_t *q, lstate_t *tp)
 *	free t_emap_i and disable emapping on a line.
 *
 * Calling/Exit State:
 *	called by ldtermclose.
 */
void
chanmap_close(queue_t *q, lstate_t *tp)
{
	/* 
	 * Enhanced Application Compatibility
 	 * The following for internationalization:
	 */

	if (tp->t_emap_i.emap_mp)
		freemsg(tp->t_emap_i.emap_mp);
	tp->t_emap_i.emap_mp = tp->t_emap_i.emap_lmp =  NULL;
	/* free up buffers holding codeset maps */
	str_emunmap(q, &tp->t_emap, 0); 
	str_nmunmap(q, &tp->t_emap, 0);

	 /* End Enhanced Application Compatibility */
}

/*
 * void
 * chanmap_data(queue_t *q, mblk_t *mp, lstate_t *tp)
 *	do XENIX channel mapping on M_DATA message
 *
 * Calling/Exit State:
 *	called by ldtermrsrv().
 */
void
chanmap_data(queue_t *q, mblk_t *mp, lstate_t *tp)
{
	mblk_t *bp, *cmp, *omp;
	int ebsize;

	bp = mp;
	omp = (mblk_t *) NULL;
	while (bp) {
	   cmp = unlinkb(bp);
	   ebsize = bp->b_wptr - bp->b_rptr;

	   if (ebsize != 0) {
		ebsize = str_emmapin(bp,&tp->t_emap);
		if (tp->t_emap.t_merr && (tp->t_modes.c_lflag & ECHO)) {
			if (ebsize == 0) ebsize = 1;
			ldterm_outchar(CTRL('g'), WR(q), ebsize, tp);
		}
	   }
	   if (omp == NULL)
		omp = bp;
	   else {
		omp->b_cont = bp;
		omp = bp;
	   }
	   bp = cmp;
	}
}

/*
 * void
 * chanmap_output_msg(lstate_t *tp, int bsize, mblk_t **impp, 
 *	mblk_t **ibp, mblk_t **ipbpp)
 *
 * Calling/Exit State:
 * 	called by ldterm_output_msg()
 */
void
chanmap_output_msg(lstate_t *tp, int bsize, mblk_t **impp, 
	mblk_t **ibpp, mblk_t **ipbpp)
{
	mblk_t *imp, *ibp, *ipbp;
	mblk_t *mapbp;
	emcp_t emp;	/* this is a unchar * to mapped characters */
	unchar c;
	int i;
	extern emcp_t str_emmapout();

	ibp = *ibpp;
	imp = *impp;
	ipbp = *ipbpp;

#define MAXEMSIZE 10	/* give us a little extra room */

	if ((mapbp = allocb(bsize + MAXEMSIZE, BPRI_HI)) == NULL) {
		/*
		 *+ Kernel cannot allocate memory for a streams message.
		 */
		cmn_err(CE_NOTE,"chanmap_output_msg: allocb failed. Unable to perform channel mapping");
		return;
	}

	while (ibp->b_rptr != ibp->b_wptr) {
		i = 0;
		if ((mapbp->b_wptr - mapbp->b_rptr) > bsize)
			goto insertmapbp;
		c = *ibp->b_rptr++;
		emp = str_emmapout(&tp->t_emap, c, &i);
		if (i == 0) continue;
		if (i > (mapbp->b_datap->db_lim - mapbp->b_wptr)) {
			/* attempt to copy message */
			mblk_t *cmp;

			cmp = allocb(i+mapbp->b_wptr - mapbp->b_rptr,BPRI_HI);
			if (cmp == NULL)
				goto insertmapbp; /* punt extra chars */
			while (mapbp->b_rptr != mapbp->b_wptr)
				*cmp->b_wptr++ = *mapbp->b_rptr++;
			freemsg(mapbp);
			mapbp = cmp;
		}
		while (i>0) {
			*mapbp->b_wptr++ = *emp++;
			i--;
		}
	}
	/* successfully mapped entire ibp */
	mapbp->b_cont = ibp->b_cont; 
	freeb(ibp);
	ibp = mapbp;
	goto attach_ibp_to_ipbp;

insertmapbp: 

	/* we did not do all of ibp, so save what's left for next loop */
	mapbp->b_cont = ibp;
	ibp = mapbp;

attach_ibp_to_ipbp:

	if (ipbp == NULL) { /* ibp is first block of message */
		imp = ibp;
		ipbp = imp;
	}
	else {
		ipbp->b_cont = ibp;
		ipbp = ibp;
	}

	*ibpp = ibp;
	*impp = imp;
	*ipbpp = ipbp;
}

/*
 * STATIC void
 * chanmap_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval)
 *	send an ACK msg.
 *
 * Calling/Exit State:
 */
STATIC void
chanmap_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval)
{
	mblk_t	*tmp;

	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;
	if ((tmp = unlinkb(mp)) != (mblk_t *)NULL)
		freemsg(tmp);
	qreply(qp,mp);
}

/*
 * STATIC void
 * chanmap_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, 
 * 		int error, int rval)
 *	send a NACK msg.
 *
 * Calling/Exit State:
 */
STATIC void
chanmap_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, 
	int error, int rval)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp,mp);
}

/*
 * void
 * chanmap_do_iocdata(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	called by ldtermwput.
 *	return 1, if we processed the msg;
 *	return 0, if we did not find a match. Calling function is
 *		  responsible for passing the msg along.
 */
int
chanmap_do_iocdata(queue_t *q, mblk_t *mp)
{
	lstate_t *tp;
	struct copyresp *crsp;
	struct iocblk *iocp;
	struct emtab *ep;

	/* LINTED pointer alignment */
	crsp = (struct copyresp *)mp->b_rptr;
	tp = (lstate_t *)q->q_ptr;
	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;

	switch (iocp->ioc_cmd) {
	default:
		/* not for us */
		break;

		/* Enhanced Application Compatability */
	case LDGMAP: 
	case NMGMAP: 
		if (crsp->cp_rval) { /* already nak'ked for us */
			freemsg(mp);
			return(1);
		}

		chanmap_iocack(q,mp,iocp,0);
		return(1);

	case LDSMAP: {
		struct copyreq *crqp;
		int error, nbuf;
		mblk_t *tmp;
		if (crsp->cp_rval) { /* already nak'ked for us */
			freemsg(mp);
			return(1);
		}
		if ((tmp = msgpullup(mp->b_cont, E_TABSZ)) == 0) {
#ifdef DEBUG
			cmn_err(CE_NOTE,
				"CHANMAP: LDSMAP msgpullup failed:state %d \n",
					tp->t_emap_i.emap_reqstate);
#endif
			chanmap_iocnack(q,mp,iocp,EINVAL,-1);
			return(1);
			
		}
		freemsg(mp->b_cont);
		mp->b_cont = tmp;
		switch(tp->t_emap_i.emap_reqstate) {
			default: 
#ifdef DEBUG
				cmn_err(CE_NOTE, 
			     	"CHANMAP: LDSMAP: bad emap state %d\n",
					tp->t_emap_i.emap_reqstate);
#endif
				chanmap_iocnack(q,mp,iocp,EINVAL,-1);
				return(1);
			case READ_EMAP_BUFS: 
				if ( tp->t_emap_i.emap_nbuf == -1 ) {
				   /* LINTED pointer alignment */
				   ep = (struct emtab *)mp->b_cont->b_rptr;
				   nbuf =  (int) (ep->e_stab >> EMBSHIFT ) ;
				   if ( nbuf < 0 || nbuf >= NEMBUFS ) {
#ifdef DEBUG
				cmn_err(CE_NOTE,
					"CHANMAP: LDSMAP bad nbuf %d \n", nbuf);
#endif
				      chanmap_iocnack(q,mp,iocp,EINVAL,-1);
				      return(1);
				   }
				   tp->t_emap_i.emap_nbuf = nbuf;
				   tp->t_emap_i.emap_lmp = mp->b_cont;
				   tp->t_emap_i.emap_mp = mp->b_cont;
				   tp->t_emap_i.emap_mp->b_cont = NULL;
				   mp->b_cont = NULL;
				}
				else {
				   tp->t_emap_i.emap_lmp->b_cont = mp->b_cont;
				   tp->t_emap_i.emap_lmp = mp->b_cont;
				   tp->t_emap_i.emap_lmp->b_cont = NULL;
				   mp->b_cont = NULL;
				}
#ifdef DEBUG
				cmn_err(CE_NOTE,
				   "CHANMAP: LDSMAP uaddr=%x nbuf=%d\n",
				tp->t_emap_i.emap_uaddr,tp->t_emap_i.emap_nbuf);
#endif
				if ( tp->t_emap_i.emap_nbuf == 0) 
					goto alldone;
		   		tp->t_emap_i.emap_nbuf--; 
		   		tp->t_emap_i.emap_uaddr +=  E_TABSZ;
		   		tp->t_emap_i.emap_reqstate = READ_EMAP_BUFS;
				/* LINTED pointer alignment */
				crqp = (struct copyreq *)mp->b_rptr;
				crqp->cq_addr =  tp->t_emap_i.emap_uaddr;
				crqp->cq_size =  E_TABSZ;
				crqp->cq_flag = 0;
				mp->b_datap->db_type = M_COPYIN;
				qreply(q,mp);
				return(1);
				
alldone:
				/* set to unknown state */
				tp->t_emap_i.emap_reqstate = 99; 
				tp->t_emap_i.emap_nbuf = -1;
		   		tp->t_emap_i.emap_uaddr = (caddr_t) NULL;
				error = str_emsetmap(q,tp->t_emap_i.emap_mp,&tp->t_emap);
				freemsg(tp->t_emap_i.emap_mp);
				tp->t_emap_i.emap_mp = NULL;
				tp->t_emap_i.emap_lmp = NULL;
				if (error) {
#ifdef DEBUG
				   cmn_err(CE_NOTE,
				   "CHANMAP: str_emsetmap error=%d\n", error);
#endif
					chanmap_iocnack(q,mp,iocp,error,-1);
					return(1);
				}
		
				chanmap_iocack(q,mp,iocp,0);
				return(1);
		}
		/* NOTREACHED */
		break;
		}
	case NMSMAP: {
		int error;
		mblk_t *tmp;
		if (crsp->cp_rval) { /* already nak'ked for us */
			freemsg(mp);
			return(1);
		}
		if ((tmp = msgpullup(mp->b_cont,E_TABSZ)) == 0) {
#ifdef DEBUG
			cmn_err(CE_NOTE, "CHANMAP: NMSMAP msgpullup failed\n");
#endif
			chanmap_iocnack(q, mp, iocp, EINVAL, -1);
			return(1);
		}
		freemsg(mp->b_cont);
		mp->b_cont = tmp;
		error = str_nmsetmap(q,mp->b_cont,&tp->t_emap);
		if (error) {
#ifdef DEBUG
			cmn_err(CE_NOTE,
		   		"CHANMAP: str_nmsetmap error=%d\n", error);
#endif
			chanmap_iocnack(q,mp,iocp,error,-1);
			return(1);
		}
		chanmap_iocack(q,mp,iocp,0);
		return(1);
		}
	}
	return(0);
	/* End Enhanced Application Compatability */
}

/*
 * int
 * chanmap_do_ioctl(queue_t *q, mblk_t *mp, struct iocblk *iocp, lstate_t *tp)
 *	handle XENIX channel mapping ioctls.
 *
 * Calling/Exit State:
 * 	Called by ldterm_do_ioctl
 *	return 1, if we processed the ioctl;
 *	return 0, if we did not find a match. Calling function is
 *		  responsible for passing the msg along.
 */
int
chanmap_do_ioctl(queue_t *q, mblk_t *mp, struct iocblk *iocp, lstate_t *tp)
{
	switch (iocp->ioc_cmd) {

	/* Enhanced Application Compatability */
	case NMSMAP: 
	case LDSMAP:  {
		struct copyreq *crqp;
		if (iocp->ioc_count != TRANSPARENT) {
			chanmap_iocnack(q, mp, iocp, EINVAL, -1);
			return(1);
		}
		crqp = (struct copyreq *)iocp;
		/* LINTED pointer alignment */
		crqp->cq_addr = * (caddr_t *)mp->b_cont->b_rptr;
		crqp->cq_size =  E_TABSZ;
		crqp->cq_private = (mblk_t *)  NULL;
		crqp->cq_flag = 0;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		mp->b_datap->db_type = M_COPYIN;
		if (iocp->ioc_cmd == LDSMAP) {
		   tp->t_emap_i.emap_uaddr = crqp->cq_addr; 
		   tp->t_emap_i.emap_reqstate =  READ_EMAP_BUFS;
		   tp->t_emap_i.emap_nbuf =  (int) -1;
		   tp->t_emap_i.emap_mp = (mblk_t *)NULL;
		   tp->t_emap_i.emap_lmp = (mblk_t *)NULL;
		}
		qreply(q, mp);
		return(1);
	}

	case LDGMAP:  {
		mblk_t *datamp;
		struct copyreq *crqp;
		int error, nbuf;
		struct emtab *ep;

		if (iocp->ioc_count != TRANSPARENT) {
			chanmap_iocnack(q, mp, iocp, EINVAL, -1);
			return(1);
		}
		if ((datamp = allocb(E_TABSZ,BPRI_HI)) == (mblk_t *) NULL) {
			chanmap_iocnack(q, mp, iocp, ENOMEM, -1);
			return(1);
		}
		error = str_emgetmap(&tp->t_emap, datamp);
		if (error) {
#ifdef DEBUG
			cmn_err(CE_NOTE, 
			   "CHANMAP:str_emgetmap error=%d \n", error);
#endif
			freemsg(datamp);
			datamp = NULL;
			chanmap_iocnack(q,mp,iocp,error,-1);
			return(1);
		}
		/* LINTED pointer alignment */
		ep = (struct emtab *)datamp->b_rptr;
		nbuf = (int) (ep->e_stab  >> EMBSHIFT) + 1;
#ifdef DEBUG
		cmn_err(CE_NOTE,
			"CHANMAP:LDGMAP e_stab=%d nbuf =%d \n",
				ep->e_stab, nbuf);
#endif
		crqp = (struct copyreq *)iocp;
		/* LINTED pointer alignment */
		crqp->cq_addr = * (caddr_t *)mp->b_cont->b_rptr;
		freeb(mp->b_cont);
		crqp->cq_size = nbuf * E_TABSZ;
		crqp->cq_private = (mblk_t *) NULL;
		crqp->cq_flag = 0;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		mp->b_cont = datamp;
		mp->b_datap->db_type = M_COPYOUT;
		qreply(q,mp);
		return(1);
	}
	case NMGMAP: {
		mblk_t *datamp;
		struct copyreq *crqp;
		int error;

		if (iocp->ioc_count != TRANSPARENT) {
			chanmap_iocnack(q,mp,iocp,EINVAL,-1);
			return(1);
		}
		if ((datamp = allocb(E_TABSZ,BPRI_HI)) == (mblk_t *) NULL) {
			chanmap_iocnack(q,mp,iocp,ENOMEM,-1);
			return(1);
		}
		error = str_nmgetmap(&tp->t_emap,datamp);
		if (error) {
#ifdef DEBUG
		cmn_err(CE_NOTE,
			"CHANMAP:str_nmgetmap error=%d\n",error);
#endif
			freemsg(datamp);
			datamp = NULL;
			chanmap_iocnack(q,mp,iocp,error,-1);
			return(1);
		}
		crqp = (struct copyreq *)iocp;
		/* LINTED pointer alignment */
		crqp->cq_addr = * (caddr_t *)mp->b_cont->b_rptr;
		freeb(mp->b_cont);
		crqp->cq_size = E_TABSZ;
		crqp->cq_private = (mblk_t *) NULL;
		crqp->cq_flag = 0;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		mp->b_cont = datamp;
		mp->b_datap->db_type = M_COPYOUT;
		qreply(q,mp);
		return(1);
	}
	/* Enhanced Application Compatibility Support */
/*	The LDEMAP and LDDMAP IOCTLS conflict with 
**	SCO events LDEV_SETTYPE, LDEV_GETEV put in i386/Version4
**	The reference in chanmap.c and tty1.c for LDEMAP and LDDMAP is being
**	removed for 4.1dt
**
**	case LDEMAP:
**	case LDDMAP: 
**		if (iocp->ioc_count != TRANSPARENT) {
**			chanmap_iocnack(q,mp,iocp,EINVAL,-1);
**			return(1);
**		}
**		if (iocp->ioc_cmd == LDEMAP) {
**			int	error;
**#ifdef DEBUG
**			I18N_DEBUG( "CHANMAP: LDEMAP\n");
**#endif
**			if(error = str_emenmap(q,&tp->t_emap)){
**				chanmap_iocnack(q,mp,iocp,error,-1);
**				return(1);
**			}
**		}
**		else {
**#ifdef DEBUG
**			I18N_DEBUG( "CHANMAP: LDDMAP\n");
**#endif
**			str_emdsmap(q,&tp->t_emap);
**		}
**		chanmap_iocack(q,mp,iocp,0);
**		return(1);
*/
	/* End Enhanced Application Compatibility Support */

	case LDNMAP: 
	case NMNMAP: 
		if (iocp->ioc_count != TRANSPARENT) {
			chanmap_iocnack(q, mp, iocp, EINVAL, -1);
			return(1);
		}
		if (iocp->ioc_cmd == LDNMAP) {
#ifdef DEBUG
			cmn_err(CE_CONT, "CHANMAP: LDNMAP\n");
#endif
			str_emunmap(q, &tp->t_emap, 1);
		}
		else {
#ifdef DEBUG
			cmn_err(CE_CONT, "CHANMAP: NMNMAP\n");
#endif
			str_nmunmap(q, &tp->t_emap, 1);
		}
		chanmap_iocack(q, mp, iocp, 0);
		return(1);
	}
	return(0);
}
