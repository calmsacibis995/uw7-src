#ident	"@(#)ttcompat.c	1.2"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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
 * Module to intercept old V7, 4BSD and XENIX "ioctl" calls.
 */

#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/param.h>
#include <util/ksynch.h>
#include <fs/file.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <io/conf.h>
#include <io/termios.h>
#include <io/ttold.h>
#include <util/cmn_err.h>
#include <io/stream.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <mem/kmem.h>
#include <io/ttcompat/ttcompat.h>
#include <util/inline.h>

/* Enhanced Application Compatibility Support */
#include <svc/sco.h>
/* End Enhanced Application Compatibility Support */
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define MODNAME "ttcompat - Loadable ioctl translator"

MOD_STR_WRAPPER(ttco, NULL, NULL, MODNAME);

#define	TTCOMPAT_ID	42
#define	TTCOMPAT_HIER	1

STATIC int ttcompatopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int ttcompatclose(queue_t *, int, cred_t *);
STATIC int ttcompatrput(queue_t *, mblk_t *);
STATIC int ttcompatwput(queue_t *, mblk_t *);

STATIC struct module_info ttycompatmiinfo = {
	TTCOMPAT_ID,
	"ttcompat",
	0,
	INFPSZ,
	2048,
	128
};

STATIC struct qinit ttycompatrinit = {
	ttcompatrput,
	NULL,
	ttcompatopen,
	ttcompatclose,
	NULL,
	&ttycompatmiinfo
};

STATIC struct module_info ttycompatmoinfo = {
	TTCOMPAT_ID,
	"ttcompat",
	0,
	INFPSZ,
	300,
	200
};

STATIC struct qinit ttycompatwinit = {
	ttcompatwput,
	NULL,
	ttcompatopen,
	ttcompatclose,
	NULL,
	&ttycompatmoinfo
};

struct streamtab ttcoinfo = {
	&ttycompatrinit,
	&ttycompatwinit,
	NULL,
	NULL
};

STATIC void ttcompat_do_ioctl(ttcompat_state_t *, queue_t *, mblk_t *);
STATIC void ttcompat_ioctl_ack(queue_t *, mblk_t *);
STATIC void from_compat(compat_state_t *, struct termios *);
STATIC void to_compat(struct termios *, compat_state_t *);

int ttcodevflag = D_MP;

/*
 *+ Lock protecting the ttcompat_state structure
 */
STATIC LKINFO_DECL(ttcompat_lkinfo, "IO:TTCOMPAT:t_lock", 0);

/*
 * Conversion table from speed codes to true speeds.
 */
static speed_t speeds[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400
};

/*ARGSUSED*/
/*
 * STATIC int
 * ttcompatopen(queue_t *q, dev_t *dev, int oflag, int sflag, cred_t *crp)
 * 	open procedure
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC int
ttcompatopen(queue_t *q, dev_t *dev, int oflag, int sflag, cred_t *crp)
{
	ttcompat_state_t *tp;

	if (q->q_ptr != NULL)  {
		tp = (ttcompat_state_t *)q->q_ptr;
		/*
		 * Fail open if TIOCEXCL was done and its not privileged.
		 * Locking is not used because it will not guarantee anything.
		 */
		if ((tp->t_new_lflags & XCLUDE) && (drv_priv(crp) != 0)) {
			return(EBUSY);
		}
		else
			return(0);		/* already attached */
	}

	if (sflag != MODOPEN)
		return (EINVAL);

	if ((tp = (ttcompat_state_t *)
		kmem_zalloc((int)sizeof(ttcompat_state_t), KM_NOSLEEP)) == NULL)
		return (ENOSPC);

	if ((tp->t_lock = LOCK_ALLOC(TTCOMPAT_HIER, plstr,
		&ttcompat_lkinfo, KM_NOSLEEP)) == NULL) {
		kmem_free(tp, sizeof(ttcompat_state_t));
		return(ENOSPC);
	}
	q->q_ptr = (caddr_t)tp;
	WR(q)->q_ptr = (caddr_t)tp;
	qprocson(q);
	return(0);
}

/*ARGSUSED*/
/*
 * STATIC int
 * ttcompatclose(queue_t *q, int cflag, cred_t *crp)
 * 	close procedure
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC int
ttcompatclose(queue_t *q, int cflag, cred_t *crp)
{
	ttcompat_state_t *tp;

	qprocsoff(q);
	tp = (ttcompat_state_t *)q->q_ptr;
	LOCK_DEALLOC(tp->t_lock);
	kmem_free(tp, sizeof(ttcompat_state_t));
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	return(0);
}

/*
 * STATIC int
 * ttcompatrput(queue_t *q, mblk_t *mp)
 * 	Put procedure for input from driver end of stream (read queue).
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	Most messages just get passed to the next guy up; we intercept
 * 	"ioctl" replies, and if it's an "ioctl" whose reply we plan to do
 * 	something with, we do it.
 */
STATIC int
ttcompatrput(queue_t *q, mblk_t *mp)
{
	ttcompat_state_t *tp;
	struct iocblk *iocp;
	pl_t	pl;

	switch (mp->b_datap->db_type) {

	case M_IOCACK:
		ttcompat_ioctl_ack(q, mp);
		break;

	case M_IOCNAK:
		/* LINTED pointer alignment */
		iocp = (struct iocblk *)mp->b_rptr;
		tp = (ttcompat_state_t *)q->q_ptr;

		pl = LOCK(tp->t_lock, plstr);
		if (tp->t_state & TS_IOCWAIT && iocp->ioc_id == tp->t_iocid)
			tp->t_state &= ~TS_IOCWAIT;
		UNLOCK(tp->t_lock, pl);
		putnext(q, mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
	return(0);
}

/*
 * STATIC int
 * ttcompatwput(queue_t *q, mblk_t *mp)
 * 	write side put procedure
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC int
ttcompatwput(queue_t *q, mblk_t *mp)
{
	ttcompat_state_t *tp;
	struct copyreq 	*cqp;
	struct copyresp	*csp;
	struct iocblk	*iocbp;
	uint		copyin_size;

	tp = (ttcompat_state_t *)q->q_ptr;

	/*
	 * Process some M_IOCTL messages here; pass everything else down.
	 */
	switch(mp->b_datap->db_type) {

	default:
		putnext(q, mp);
		return(0);
	
	case M_IOCTL:
		/* LINTED pointer alignment */
		iocbp = (struct iocblk *)mp->b_rptr;
		switch (iocbp->ioc_cmd) {

		default:
			/*
			 * These are ioctls with no arguments or which
			 * are known to stream head; process them right away.
			 */
			ttcompat_do_ioctl(tp, q, mp);
			return(0);

		/* Enhanced Application Compatibility Support */
		case SCO_XCSETA:
		case SCO_XCSETAW:
		case SCO_XCSETAF:
			copyin_size = sizeof(struct sco_termios);
			break;
		/* End Enhanced Application Compatibility Support */

		case TIOCSETN:
			copyin_size = sizeof(struct sgttyb);
			break;
		case TIOCSLTC:
		case TIOCSETC:
		case TIOCLBIS:
		case TIOCLBIC:
		case TIOCLSET:
			copyin_size = sizeof(struct ltchars);
			break;
		} /* switch ioc_cmd */

		if (iocbp->ioc_count != TRANSPARENT) {
			putnext(q, mp);
			return(0);
		}
		mp->b_datap->db_type = M_COPYIN;
		/* LINTED pointer alignment */
		cqp = (struct copyreq *)mp->b_rptr;
		/* LINTED pointer alignment */
		cqp->cq_addr = (caddr_t) *(long *)mp->b_cont->b_rptr;
		cqp->cq_size = copyin_size;
		cqp->cq_flag = 0;
		cqp->cq_private = NULL;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		tp->t_ioccmd = iocbp->ioc_cmd;
		qreply(q, mp);
		return(0);


	case M_IOCDATA:
		/* LINTED pointer alignment */
		csp = (struct copyresp *)mp->b_rptr;

		switch (csp->cp_cmd) {

		default:
			putnext(q, mp);
			return(0);

		/* Enhanced Application Compatibility Support */
		case SCO_XCSETA:
		case SCO_XCSETAW:
		case SCO_XCSETAF:
		/* End Enhanced Application Compatibility Support */

		case TIOCSETN:
		case TIOCSLTC:
		case TIOCSETC:
		case TIOCLBIS:
		case TIOCLBIC:
		case TIOCLSET:
			if (csp->cp_rval) {	/* failure */
				freemsg(mp);
			} else {	/* make it look like an ioctl */
				mp->b_datap->db_type = M_IOCTL;
				mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);
				/* LINTED pointer alignment */
				iocbp = (struct iocblk *)mp->b_rptr;
				iocbp->ioc_count = mp->b_cont->b_wptr - mp->b_cont->b_rptr;
				iocbp->ioc_error = 0;
				iocbp->ioc_rval = 0;
				ttcompat_do_ioctl((ttcompat_state_t *)q->q_ptr, q, mp);
			}
			return(0);

		/* Enhanced Application Compatibility Support */
		case SCO_XCGETA:
		/* End Enhanced Application Compatibility Support */

		case TIOCGLTC:
		case TIOCLGET:
		case TIOCGETC:
			if (csp->cp_rval) {	/* failure */
				freemsg(mp);
			} else {
				/* LINTED pointer alignment */
				iocbp = (struct iocblk *)mp->b_rptr;
				iocbp->ioc_count = 0;
				iocbp->ioc_error = 0;
				iocbp->ioc_rval = 0;
				mp->b_datap->db_type = M_IOCACK;
				qreply(q, mp);
			}
			return(0);

		} /* switch cp_cmd */
	} /* end message switch */
}

/*
 * STATIC void
 * ttcompat_do_ioctl(ttcompat_state_t *tp, queue_t *q, mblk_t *mp)
 *
 * 	Handle old-style V7/4BSD ioctls by converting them
 *	to termios ioctls
 *
 * Calling/Exit State:
 *	t_lock cannot be held on entry.
 */
STATIC void
ttcompat_do_ioctl(ttcompat_state_t *tp, queue_t *q, mblk_t *mp)
{
	struct iocblk	*iocp;
	struct copyreq	*cqp;
	pl_t		pl;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	switch (iocp->ioc_cmd) {

	/*
	 * "get"-style calls that get translated data from the "termios"
	 * structure.  Save the existing code and pass it down as a TCGETS.
	 */

	/* Enhanced Application Compatibility Support */
	case SCO_XCGETA:
	/* End Enhanced Application Compatibility Support */

	case TIOCGETC:
	case TIOCLGET:
	case TIOCGLTC:
		if (iocp->ioc_count != TRANSPARENT) {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			iocp->ioc_rval = 0;
			qreply(q,mp);
			return;
		}
		/* LINTED pointer alignment */
		cqp = (struct copyreq *)mp->b_rptr;
		/* LINTED pointer alignment */
		cqp->cq_private = (mblk_t *)(*(long *)mp->b_cont->b_rptr);
		/*
		 * free the data buffer - it might not be sufficient
		 * driver will allocate one for termios size
		 */
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
		iocp->ioc_count = 0;
 		/* FALLTHROUGH */
	case TIOCGETP:
		pl = LOCK(tp->t_lock, plstr);
		goto dogets;

	/*
	 * "set"-style calls that set translated data into a "termios"
	 * structure.  Set our idea of the new state from the value
	 * given to us.  We then have to get the current state, so we
	 * turn this guy into a TCGETS and pass it down.  When the
	 * ACK comes back, we modify the state we got back and shove it
	 * back down as the appropriate type of TCSETS.
	 */

	/* Enhanced Application Compatibility Support */
	case SCO_XCSETA:
	case SCO_XCSETAW:
	case SCO_XCSETAF:
		pl = LOCK(tp->t_lock, plstr);
		/* LINTED pointer alignment */
		tp->t_new_sco = *((struct sco_termios *)mp->b_cont->b_rptr);
		goto dogets;
	/* End Enhanced Application Compatibility Support */

	case TIOCSETP:
	case TIOCSETN:
		pl = LOCK(tp->t_lock, plstr);
		/* LINTED pointer alignment */
		tp->t_new_sgttyb = *((struct sgttyb *)mp->b_cont->b_rptr);
		goto dogets;

	case TIOCSETC:
		pl = LOCK(tp->t_lock, plstr);
		tp->t_new_tchars = *((struct tchars *)mp->b_cont->b_rptr);
		goto dogets;

	case TIOCSLTC:
		pl = LOCK(tp->t_lock, plstr);
		tp->t_new_ltchars = *((struct ltchars *)mp->b_cont->b_rptr);
		goto dogets;

	case TIOCLBIS:
	case TIOCLBIC:
	case TIOCLSET:
		pl = LOCK(tp->t_lock, plstr);
		/* LINTED pointer alignment */
		tp->t_new_lflags = *(int *)mp->b_cont->b_rptr;
		goto dogets;

	/*
	 * "set"-style call that sets a particular bit in a "termios"
	 * structure.  We then have to get the current state, so we
	 * turn this guy into a TCGETS and pass it down.  When the
	 * ACK comes back, we modify the state we got back and shove it
	 * back down as the appropriate type of TCSETS.
	 */
	case TIOCHPCL:
		pl = LOCK(tp->t_lock, plstr);
dogets:
		tp->t_ioccmd = iocp->ioc_cmd;
		tp->t_iocid = iocp->ioc_id;
		tp->t_state |= TS_IOCWAIT;
		UNLOCK(tp->t_lock, pl);
		iocp->ioc_cmd = TCGETS;
		iocp->ioc_count = 0;	/* no data returned unless we say so */
		break;

	/*
	 * "set"-style call that sets DTR.  Pretend that it was a TIOCMBIS
	 * with TIOCM_DTR set.
	 */
	case TIOCSDTR: {
		mblk_t *datap;

		if ((datap = allocb((int)sizeof (int), BPRI_HI)) == NULL)
			goto allocfailure;
		/* LINTED pointer alignment */
		*(int *)datap->b_wptr = TIOCM_DTR;
		datap->b_wptr += (sizeof (int))/(sizeof *datap->b_wptr);
		iocp->ioc_cmd = TIOCMBIS;	/* turn it into a TIOCMBIS */
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;	/* attach the data */
		break;
	}

	/*
	 * "set"-style call that clears DTR.  Pretend that it was a TIOCMBIC
	 * with TIOCM_DTR set.
	 */
	case TIOCCDTR: {
		mblk_t *datap;

		if ((datap = allocb((int)sizeof (int), BPRI_HI)) == NULL)
			goto allocfailure;
		/* LINTED pointer alignment */
		*(int *)datap->b_wptr = TIOCM_DTR;
		datap->b_wptr += (sizeof (int))/(sizeof *datap->b_wptr);
		iocp->ioc_cmd = TIOCMBIC;	/* turn it into a TIOCMBIC */
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;	/* attach the data */
		break;
	}

	/*
	 * Translate into the S5 form of TCFLSH.
	 */
	case TIOCFLUSH: {
		/* LINTED pointer alignment */
		int flags = *(int *)mp->b_cont->b_rptr;

		switch (flags&(FREAD|FWRITE)) {

		case 0:
		case FREAD|FWRITE:
			flags = 2;	/* flush 'em both */
			break;

		case FREAD:
			flags = 0;	/* flush read */
			break;

		case FWRITE:
			flags = 1;	/* flush write */
			break;
		}
		iocp->ioc_cmd = TCFLSH;	/* turn it into a TCFLSH */
		/* LINTED pointer alignment */
		*(int *)mp->b_cont->b_rptr = flags;	/* fiddle the arg */
		break;
	}

	/*
	 * Turn into a TCXONC.
	 */
	case TIOCSTOP: {
		mblk_t *datap;

		if ((datap = allocb((int)sizeof (int), BPRI_HI)) == NULL)
			goto allocfailure;
		/* LINTED pointer alignment */
		*(int *)datap->b_wptr = 0;	/* stop */
		datap->b_wptr += (sizeof (int))/(sizeof *datap->b_wptr);
		iocp->ioc_cmd = TCXONC;	/* turn it into a XONC */
		iocp->ioc_count = 0; /* set it to !TRANSPARENT */
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;	/* attach the data */
		break;
	}

	case TIOCSTART: {
		mblk_t *datap;

		if ((datap = allocb((int)sizeof (int), BPRI_HI)) == NULL)
			goto allocfailure;
		/* LINTED pointer alignment */
		*(int *)datap->b_wptr = 1;	/* start */
		datap->b_wptr += (sizeof (int))/(sizeof *datap->b_wptr);
		iocp->ioc_cmd = TCXONC;	/* turn it into a XONC */
		iocp->ioc_count = 0; /* set it to !TRANSPARENT */
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;	/* attach the data */
		break;
	}

	case TIOCGETD:
	case TIOCSETD:
	case DIOCSETP:
	case DIOCGETP:
	case LDOPEN:
	case LDCLOSE:
	case LDCHG:
	case LDSETT:
	case LDGETT:
	/* case FIORDCHK: handled in the stream head */
	 	/* all the ioctls just get acked */	
		/* set and get allowed for only line discipline zero */
		/* if its not line discipline zero, TIOCSETD is naked */

		if ((iocp->ioc_cmd == TIOCSETD) && (*mp->b_cont->b_rptr != 0))
			mp->b_datap->db_type = M_IOCNAK;
		else
			mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_error = 0;
		iocp->ioc_count = 0;
		iocp->ioc_rval = 0;
		qreply(q,mp);
		return;
	case IOCTYPE:
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_error = 0;
		iocp->ioc_count = 0;
		iocp->ioc_rval = TIOC;
		qreply(q,mp);
		return;
	case TIOCEXCL:
		/* check for binary value of XCLUDE flag ???? */
		tp->t_new_lflags |= XCLUDE;
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_error = 0;
		iocp->ioc_count = 0;
		iocp->ioc_rval = 0;
		qreply(q,mp);
		return;
	case TIOCNXCL:
		tp->t_new_lflags &= ~XCLUDE;
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_error = 0;
		iocp->ioc_count = 0;
		iocp->ioc_rval = 0;
		qreply(q,mp);
		return;
	}

	/*
	 * We don't reply to most calls, we just pass them down,
	 * possibly after modifying the arguments.
	 */
	putnext(q, mp);
	return;

allocfailure:
	/*
	 * We needed to allocate something to handle this "ioctl", but
	 * couldn't; send nak with ENOSPC
	 */
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_error = ENOSPC;
	iocp->ioc_count = 0;
	iocp->ioc_rval = 0;
	qreply(q,mp);
}

/*
 * STATIC void
 * ttcompat_ioctl_ack(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	Called when an M_IOCACK message is seen on the read queue; if this
 * 	is the response we were waiting for, we either:
 *    	   modify the data going up (if the "ioctl" read data); since in all
 *    	   cases, the old-style returned information is smaller than or the same
 *    	   size as the new-style returned information, we just overwrite the old
 *    	   stuff with the new stuff (beware of changing structure sizes, in case
 *    	   you invalidate this)
 * 	or
 *    	   take this data, modify it appropriately, and send it back down (if
 *    	   the "ioctl" wrote data).
 * 	In either case, we cancel the "wait"; the final response to a "write"
 * 	ioctl goes back up to the user.
 * 	If this wasn't the response we were waiting for, just pass it up.
 */
STATIC void
ttcompat_ioctl_ack(queue_t *q, mblk_t *mp)
{
	ttcompat_state_t *tp;
	struct iocblk	*iocp;
	mblk_t		*datap;
	uint		copyout_size;
	struct copyreq	*cqp;
	pl_t		pl;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	tp = (ttcompat_state_t *)q->q_ptr;

	pl = LOCK(tp->t_lock, plstr);
	if (!(tp->t_state & TS_IOCWAIT) || iocp->ioc_id != tp->t_iocid) {
		/*
		 * This isn't the reply we're looking for. Move along.
		 */
		UNLOCK(tp->t_lock, pl);
		putnext(q, mp);
		return;
	}

	tp->t_state &= ~TS_IOCWAIT;
	datap = mp->b_cont;	/* mblk containing data going up */

	switch (tp->t_ioccmd) {

	/* Enhanced Application Compatibility Support */
	case SCO_XCGETA:
		{
		struct sco_termios *scop;
		struct termios *termp;

		/* LINTED pointer alignment */
		termp = (struct termios *)datap->b_rptr;
		scop = &tp->t_new_sco;

		/* Extract the data from termios */
		scop->c_iflag = 0xffff & termp->c_iflag;
		scop->c_oflag = 0xffff & termp->c_oflag;
		scop->c_cflag = 0xffff & termp->c_cflag;
		scop->c_lflag = 0xffff & termp->c_lflag;
		scop->c_line = 0;
		scop->c_ospeed = termp->c_cflag & CBAUD;
		if ((scop->c_ispeed = ((termp->c_cflag&CIBAUD)>>IBSHIFT)) == 0)
			scop->c_ispeed = scop->c_ospeed;
		bcopy((caddr_t)termp->c_cc, (caddr_t)scop->c_cc, NCC);
		scop->c_cc[SCO_8] = 0;
		scop->c_cc[SCO_9] = 0;
		scop->c_cc[SCO_VSUSP] = termp->c_cc[VSUSP];
		scop->c_cc[SCO_VSTART] = termp->c_cc[VSTART];
		scop->c_cc[SCO_VSTOP] = termp->c_cc[VSTOP];

		/* recycle the reply's buffer */
		/* The size of sco_termios is smaller than svr4 termios */
		datap->b_wptr = datap->b_datap->db_base;
		/* LINTED pointer alignment */
		scop = (struct sco_termios *)datap->b_wptr;
		*scop = tp->t_new_sco;
		datap->b_wptr += (sizeof (struct sco_termios))/(sizeof *datap->b_wptr);
		copyout_size = sizeof(struct sco_termios);
		}
		break;
	/* End Enhanced Application Compatibility Support */

	case TIOCGETP: {
		struct sgttyb *cb;

		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		datap->b_wptr = datap->b_datap->db_base;
		/* recycle the reply's buffer */
		/* LINTED pointer alignment */
		cb = (struct sgttyb *)datap->b_wptr;
		cb->sg_ispeed = tp->t_curstate.t_ispeed;
		cb->sg_ospeed = tp->t_curstate.t_ospeed;
		cb->sg_erase = tp->t_curstate.t_erase;
		cb->sg_kill = tp->t_curstate.t_kill;
		cb->sg_flags = tp->t_curstate.t_flags;
		UNLOCK(tp->t_lock, pl);
		datap->b_wptr += (sizeof (struct sgttyb))/(sizeof *datap->b_wptr);
		iocp->ioc_count = sizeof (struct sgttyb);

		/* stream head knows how to copy this out */
		iocp->ioc_rval = 0;
		iocp->ioc_cmd =  tp->t_ioccmd;
		putnext(q, mp);
		return;
	}

	case TIOCGETC:
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		datap->b_wptr = datap->b_datap->db_base;
		/* recycle the reply's buffer */
		bcopy((caddr_t)&tp->t_curstate.t_intrc, (caddr_t)datap->b_wptr,
		    sizeof (struct tchars));
		datap->b_wptr += (sizeof (struct tchars))/(sizeof *datap->b_wptr);
		copyout_size = sizeof(struct tchars);
		break;

	case TIOCGLTC:
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		datap->b_wptr = datap->b_datap->db_base;
		/* recycle the reply's buffer */
		bcopy((caddr_t)&tp->t_curstate.t_suspc, (caddr_t)datap->b_wptr,
		    sizeof (struct ltchars));
		datap->b_wptr += (sizeof(struct ltchars))/(sizeof *datap->b_wptr);
		copyout_size = sizeof(struct ltchars);
		break;

	case TIOCLGET:
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		datap->b_wptr = datap->b_datap->db_base;
		/* recycle the reply's buffer */
		/* LINTED pointer alignment */
		*(int *)datap->b_wptr = ((unsigned)tp->t_curstate.t_flags) >> 16;
		datap->b_wptr += (sizeof (int))/(sizeof *datap->b_wptr);
		copyout_size = sizeof(int);
		break;

	/* Enhanced Application Compatibility Support */
	case SCO_XCSETA:
	case SCO_XCSETAW:
	case SCO_XCSETAF:
	    {
		struct sco_termios *scop;
		struct termios *termp;
		
		/*
		 * Get the current state from the GETS data, and
		 * update it.
		 */

		/* LINTED pointer alignment */
		termp = (struct termios *)datap->b_rptr;
		scop = &tp->t_new_sco;
		termp->c_iflag = (termp->c_iflag & 0xffff0000) | scop->c_iflag;
		termp->c_oflag = (termp->c_oflag & 0xffff0000) | scop->c_oflag;
		termp->c_cflag = (termp->c_cflag & 0xffff0000) | scop->c_cflag;
		termp->c_lflag = (termp->c_lflag & 0xffff0000) | scop->c_lflag;
		bcopy((caddr_t)scop->c_cc,  (caddr_t)termp->c_cc, NCC);

		termp->c_cc[VSUSP] = scop->c_cc[SCO_VSUSP];
		termp->c_cc[VSTART] = scop->c_cc[SCO_VSTART];
		termp->c_cc[VSTOP] = scop->c_cc[SCO_VSTOP];

		/* Update the current state */
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);

		/*
		 * Send it back down as a TCSETS, TCSETSW, TCSETSF.
		 */
		switch (tp->t_ioccmd) {
		case SCO_XCSETA:
			iocp->ioc_cmd = TCSETS;
			break;
		case SCO_XCSETAW:
			iocp->ioc_cmd = TCSETSW;
			break;
		case SCO_XCSETAF:
			iocp->ioc_cmd = TCSETSF;
			break;
		}
	    }
	    goto senddown;
	/* End Enhanced Application Compatibility Support */

	case TIOCSETP:
	case TIOCSETN:
		/*
		 * Get the current state from the GETS data, and
		 * update it.
		 */
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		tp->t_curstate.t_erase = tp->t_new_sgttyb.sg_erase;
		tp->t_curstate.t_kill = tp->t_new_sgttyb.sg_kill;
		if (tp->t_new_sgttyb.sg_ispeed != tp->t_curstate.t_ispeed) {
			tp->t_curstate.t_real_ispeed =
					speeds[tp->t_new_sgttyb.sg_ispeed];
			tp->t_curstate.t_ispeed = tp->t_new_sgttyb.sg_ispeed;
		}
		if (tp->t_new_sgttyb.sg_ospeed != tp->t_curstate.t_ospeed) {
			tp->t_curstate.t_real_ospeed =
					speeds[tp->t_new_sgttyb.sg_ospeed];
			tp->t_curstate.t_ospeed = tp->t_new_sgttyb.sg_ospeed;
		}
		tp->t_curstate.t_flags =
		    (tp->t_curstate.t_flags & 0xffff0000) | (tp->t_new_sgttyb.sg_flags & 0xffff);

		/*
		 * Replace the data that came up with the updated data.
		 */
		/* LINTED pointer alignment */
		from_compat(&tp->t_curstate, (struct termios *)datap->b_rptr);

		/*
		 * Send it back down as a TCSETS or TCSETSF.
		 */
		iocp->ioc_cmd = (tp->t_ioccmd == TIOCSETP) ? TCSETSF : TCSETS;
		goto senddown;

	case TIOCSETC:
		/*
		 * Get the current state from the GETS data, and
		 * update it.
		 */
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		bcopy((caddr_t)&tp->t_new_tchars,
		    (caddr_t)&tp->t_curstate.t_intrc, sizeof (struct tchars));

		/*
		 * Replace the data that came up with the updated data.
		 */
		/* LINTED pointer alignment */
		from_compat(&tp->t_curstate, (struct termios *)datap->b_rptr);

		/*
		 * Send it back down as a TCSETS.
		 */
		iocp->ioc_cmd = TCSETS;
		goto senddown;

	case TIOCSLTC:
		/*
		 * Get the current state from the GETS data, and
		 * update it.
		 */
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		bcopy((caddr_t)&tp->t_new_ltchars,
		    (caddr_t)&tp->t_curstate.t_suspc, sizeof (struct ltchars));

		/*
		 * Replace the data that came up with the updated data.
		 */
		/* LINTED pointer alignment */
		from_compat(&tp->t_curstate, (struct termios *)datap->b_rptr);

		/*
		 * Send it back down as a TCSETS.
		 */
		iocp->ioc_cmd = TCSETS;
		goto senddown;

	case TIOCLBIS:
		/*
		 * Get the current state from the GETS data, and
		 * update it.
		 */
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		tp->t_curstate.t_flags |= (tp->t_new_lflags << 16);

		/*
		 * Replace the data that came up with the updated data.
		 */
		/* LINTED pointer alignment */
		from_compat(&tp->t_curstate, (struct termios *)datap->b_rptr);

		/*
		 * Send it back down as a TCSETS.
		 */
		iocp->ioc_cmd = TCSETS;
		goto senddown;

	case TIOCLBIC:
		/*
		 * Get the current state from the GETS data, and
		 * update it.
		 */
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		tp->t_curstate.t_flags &= ~(tp->t_new_lflags << 16);

		/*
		 * Replace the data that came up with the updated data.
		 */
		/* LINTED pointer alignment */
		from_compat(&tp->t_curstate, (struct termios *)datap->b_rptr);

		/*
		 * Send it back down as a TCSETS.
		 */
		iocp->ioc_cmd = TCSETS;
		goto senddown;

	case TIOCLSET:
		/*
		 * Get the current state from the GETS data, and
		 * update it.
		 */
		/* LINTED pointer alignment */
		to_compat((struct termios *)datap->b_rptr, &tp->t_curstate);
		tp->t_curstate.t_flags &= 0xffff;
		tp->t_curstate.t_flags |= (tp->t_new_lflags << 16);

		/*
		 * Replace the data that came up with the updated data.
		 */
		/* LINTED pointer alignment */
		from_compat(&tp->t_curstate, (struct termios *)datap->b_rptr);

		/*
		 * Send it back down as a TCSETS.
		 */
		iocp->ioc_cmd = TCSETS;
		goto senddown;

	case TIOCHPCL:
		/*
		 * Replace the data that came up with the updated data.
		 */
		/* LINTED pointer alignment */
		((struct termios *)datap->b_rptr)->c_cflag |= HUPCL;

		/*
		 * Send it back down as a TCSETS.
		 */
		iocp->ioc_cmd = TCSETS;
		goto senddown;

	default:
		/*
		 *+ Unexpected ioctl ack is received.
		 */
		cmn_err(CE_WARN,
		"ttcompat: unexpected ioctl ack., tp->t_ioccmd = 0x%x.",tp->t_ioccmd);
		UNLOCK(tp->t_lock, pl);
		putnext(q, mp);
		return;
	} /* end switch (tp->t_ioccmd) */


	/*
	 * copy out the data - ioctl transparency
	 */
	iocp->ioc_cmd =  tp->t_ioccmd;
	UNLOCK(tp->t_lock, pl);
	/*
	 * All the calls that return something return 0.
	 */
	iocp->ioc_rval = 0;
	mp->b_datap->db_type = M_COPYOUT;
	/* LINTED pointer alignment */
	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_addr = (caddr_t)cqp->cq_private;
	cqp->cq_size = copyout_size;
	cqp->cq_flag = 0;
	cqp->cq_private = NULL;
	putnext(q, mp);
	return;

senddown:
	/*
	 * Send a "get state" reply back down, with suitably-modified
	 * state, as a "set state" "ioctl".
	 */
	UNLOCK(tp->t_lock, pl);
	mp->b_datap->db_type = M_IOCTL;
	mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);
	putnext(WR(q), mp);
	return;
}

#define	FROM_COMPAT_CHAR(to, from) if ((to = from) == 0377) to = _POSIX_VDISABLE

/*
 * STATIC void
 * from_compat(compat_state_t *csp, struct termios *termiosp)
 * 	convert ttcompat data to termios format
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC void
from_compat(compat_state_t *csp, struct termios *termiosp)
{
	termiosp->c_iflag = 0;
	termiosp->c_oflag &= (ONLRET|ONOCR);

	if (csp->t_ispeed == 0)
		tcsetspeed(TCS_ALL, termiosp, 0);	/* hang up */
	else {
		tcsetspeed(TCS_IN, termiosp, csp->t_real_ispeed);
		tcsetspeed(TCS_OUT, termiosp, csp->t_real_ospeed);
	}

	if (csp->t_ispeed == B110 || csp->t_xflags & STOPB)
		termiosp->c_cflag |= CSTOPB;
	termiosp->c_lflag = csp->t_lflag;
	FROM_COMPAT_CHAR(termiosp->c_cc[VERASE], csp->t_erase);
	FROM_COMPAT_CHAR(termiosp->c_cc[VKILL], csp->t_kill);
	FROM_COMPAT_CHAR(termiosp->c_cc[VINTR], csp->t_intrc);
	FROM_COMPAT_CHAR(termiosp->c_cc[VQUIT], csp->t_quitc);
	FROM_COMPAT_CHAR(termiosp->c_cc[VSTART], csp->t_startc);
	FROM_COMPAT_CHAR(termiosp->c_cc[VSTOP], csp->t_stopc);
	termiosp->c_cc[VEOL2] = _POSIX_VDISABLE;
	FROM_COMPAT_CHAR(termiosp->c_cc[VSUSP], csp->t_suspc);
	FROM_COMPAT_CHAR(termiosp->c_cc[VDSUSP], csp->t_dsuspc);
	FROM_COMPAT_CHAR(termiosp->c_cc[VREPRINT], csp->t_rprntc);
	FROM_COMPAT_CHAR(termiosp->c_cc[VDISCARD], csp->t_flushc);
	FROM_COMPAT_CHAR(termiosp->c_cc[VWERASE], csp->t_werasc);
	FROM_COMPAT_CHAR(termiosp->c_cc[VLNEXT], csp->t_lnextc);
	if (csp->t_flags & O_TANDEM)
		termiosp->c_iflag |= IXOFF;
	if (csp->t_flags & O_LCASE) {
		termiosp->c_iflag |= IUCLC;
		termiosp->c_oflag |= OLCUC;
		termiosp->c_lflag |= XCASE;
	}
	if (csp->t_flags & O_ECHO)
		termiosp->c_lflag |= ECHO;
	if (csp->t_flags & O_CRMOD) {
		termiosp->c_iflag |= ICRNL;
		termiosp->c_oflag |= ONLCR;
		switch (csp->t_flags & O_CRDELAY) {

		case O_CR1:
			termiosp->c_oflag |= CR2;
			break;

		case O_CR2:
			termiosp->c_oflag |= CR3;
			break;
		}
	} else {
		if ((csp->t_flags & O_NLDELAY) == O_NL1)
			termiosp->c_oflag |= ONLRET|CR1;	/* tty37 */
	}
	if ((csp->t_flags & O_NLDELAY) == O_NL2)
		termiosp->c_oflag |= NL1;
	if (csp->t_flags & O_RAW) {
		termiosp->c_cflag |= CS8;
		termiosp->c_iflag &= ~(ICRNL|IUCLC);
		termiosp->c_lflag &= ~XCASE;
	} else {
		termiosp->c_iflag |= csp->t_iflag;
		if (csp->t_intrc >= 0)
			termiosp->c_iflag |= BRKINT;
		if (termiosp->c_cc[VSTOP] != 0 && termiosp->c_cc[VSTART] != 0)
			termiosp->c_iflag |= IXON;
		if (csp->t_flags & O_LITOUT)
			termiosp->c_cflag |= CS8;
		else {
			if (csp->t_flags & O_PASS8)
				termiosp->c_cflag |= CS8;
			else {
				switch (csp->t_flags & (O_EVENP|O_ODDP)) {

				case 0:
					termiosp->c_iflag |= ISTRIP;
					termiosp->c_cflag |= CS8;
					break;

				case O_EVENP:
					termiosp->c_iflag |= INPCK|ISTRIP;
					termiosp->c_cflag |= CS7|PARENB;
					break;

				case O_ODDP:
					termiosp->c_iflag |= INPCK|ISTRIP;
					termiosp->c_cflag |= CS7|PARENB|PARODD;
					break;

				case O_EVENP|O_ODDP:
					termiosp->c_iflag |= ISTRIP;
					termiosp->c_cflag |= CS7|PARENB;
					break;
				}
			}
			if (!(csp->t_xflags & NOPOST))
				termiosp->c_oflag |= OPOST;
		}
		if (!(csp->t_xflags & NOISIG))
			termiosp->c_lflag |= ISIG;
		if (!(csp->t_flags & O_CBREAK))
			termiosp->c_lflag |= ICANON;
		if (csp->t_flags & O_CTLECH)
			termiosp->c_lflag |= ECHOCTL;
	}
	switch (csp->t_flags & O_TBDELAY) {

	case O_TAB1:
		termiosp->c_oflag |= TAB1;
		break;

	case O_TAB2:
		termiosp->c_oflag |= TAB2;
		break;

	case O_XTABS:
		termiosp->c_oflag |= TAB3;
		break;
	}
	if (csp->t_flags & O_VTDELAY)
		termiosp->c_oflag |= FFDLY;
	if (csp->t_flags & O_BSDELAY)
		termiosp->c_oflag |= BSDLY;
	if (csp->t_flags & O_PRTERA)
		termiosp->c_lflag |= ECHOPRT;
	if (csp->t_flags & O_CRTERA)
		termiosp->c_lflag |= ECHOE;
	if (csp->t_flags & O_TOSTOP)
		termiosp->c_lflag |= TOSTOP;
	if (csp->t_flags & O_FLUSHO)
		termiosp->c_lflag |= FLUSHO;
	if (csp->t_flags & O_NOHANG)
		termiosp->c_cflag |= CLOCAL;
	if (csp->t_flags & O_CRTKIL)
		termiosp->c_lflag |= ECHOKE;
	if (csp->t_flags & O_PENDIN)
		termiosp->c_lflag |= PENDIN;
	if (!(csp->t_flags & O_DECCTQ))
		termiosp->c_iflag |= IXANY;
	if (csp->t_flags & O_NOFLSH)
		termiosp->c_lflag |= NOFLSH;
	if (termiosp->c_lflag & ICANON) {
		FROM_COMPAT_CHAR(termiosp->c_cc[VEOF], csp->t_eofc);
		FROM_COMPAT_CHAR(termiosp->c_cc[VEOL], csp->t_brkc);
	}
	else {
		termiosp->c_cc[VMIN] = 1;
		termiosp->c_cc[VTIME] = 0;
	}
}

#define	TO_COMPAT_CHAR(to, from) if ((to = from) == 0) to = (char)0377

/*
 * STATIC void
 * to_compat(struct termios *termiosp, compat_state_t *csp)
 * 	convert termios data into ttcompat data format
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC void
to_compat(struct termios *termiosp, compat_state_t *csp)
{	
	csp->t_lflag = 0;
	csp->t_lflag |= termiosp->c_lflag & ECHOK;
	csp->t_lflag |= termiosp->c_lflag & IEXTEN;
	csp->t_iflag = 0;
	csp->t_iflag |= termiosp->c_iflag & IMAXBEL;
	csp->t_iflag |= termiosp->c_iflag & IGNPAR;
	csp->t_xflags &= (NOISIG|NOPOST);
	csp->t_ospeed = termiosp->c_cflag & CBAUD;
	if ((csp->t_ispeed = ((termiosp->c_cflag & CIBAUD)>>IBSHIFT)) == 0)
		csp->t_ispeed = csp->t_ospeed;
	csp->t_real_ispeed = tcgetspeed(TCS_IN, termiosp);
	csp->t_real_ospeed = tcgetspeed(TCS_OUT, termiosp);
	if ((termiosp->c_cflag & CSTOPB) && csp->t_ispeed != B110)
		csp->t_xflags |= STOPB;
	TO_COMPAT_CHAR(csp->t_erase, termiosp->c_cc[VERASE]);
	TO_COMPAT_CHAR(csp->t_kill, termiosp->c_cc[VKILL]);
	TO_COMPAT_CHAR(csp->t_intrc, termiosp->c_cc[VINTR]);
	TO_COMPAT_CHAR(csp->t_quitc, termiosp->c_cc[VQUIT]);
	TO_COMPAT_CHAR(csp->t_startc, termiosp->c_cc[VSTART]);
	TO_COMPAT_CHAR(csp->t_stopc, termiosp->c_cc[VSTOP]);
	TO_COMPAT_CHAR(csp->t_suspc, termiosp->c_cc[VSUSP]);
	TO_COMPAT_CHAR(csp->t_dsuspc, termiosp->c_cc[VDSUSP]);
	TO_COMPAT_CHAR(csp->t_rprntc, termiosp->c_cc[VREPRINT]);
	TO_COMPAT_CHAR(csp->t_flushc, termiosp->c_cc[VDISCARD]);
	TO_COMPAT_CHAR(csp->t_werasc, termiosp->c_cc[VWERASE]);
	TO_COMPAT_CHAR(csp->t_lnextc, termiosp->c_cc[VLNEXT]);
	csp->t_flags &= (O_CTLECH|O_LITOUT|O_PASS8|O_ODDP|O_EVENP);
	if (termiosp->c_iflag & IXOFF)
		csp->t_flags |= O_TANDEM;
	if (!(termiosp->c_iflag & (IMAXBEL|BRKINT|IGNPAR|PARMRK|INPCK|ISTRIP|INLCR|IGNCR|ICRNL|IUCLC|IXON))
	    && !(termiosp->c_oflag & OPOST)
	    && (termiosp->c_cflag & (CSIZE|PARENB)) == CS8
	    && !(termiosp->c_lflag & (ISIG|ICANON|XCASE)))
		csp->t_flags |= O_RAW;
	else {
		if (!(termiosp->c_iflag & IXON)) {
			csp->t_startc = (char)0377;
			csp->t_stopc = (char)0377;
		}
		if ((termiosp->c_cflag & (CSIZE|PARENB)) == CS8
		    && !(termiosp->c_oflag & OPOST))
			csp->t_flags |= O_LITOUT;
		else {
			csp->t_flags &= ~O_LITOUT;
			if ((termiosp->c_cflag & (CSIZE|PARENB)) == CS8) {
				if (!(termiosp->c_iflag & ISTRIP))
					csp->t_flags |= O_PASS8;
			} else {
				csp->t_flags &= ~(O_ODDP|O_EVENP|O_PASS8);
				if (termiosp->c_cflag & PARODD)
					csp->t_flags |= O_ODDP;
				else if (termiosp->c_iflag & INPCK)
					csp->t_flags |= O_EVENP;
				else
					csp->t_flags |= O_ODDP|O_EVENP;
			}
			if (!(termiosp->c_oflag & OPOST))
				csp->t_xflags |= NOPOST;
			else
				csp->t_xflags &= ~NOPOST;
		}
		if (!(termiosp->c_lflag & ISIG))
			csp->t_xflags |= NOISIG;
		else
			csp->t_xflags &= ~NOISIG;
		if (!(termiosp->c_lflag & ICANON))
			csp->t_flags |= O_CBREAK;
		if (termiosp->c_lflag & ECHOCTL)
			csp->t_flags |= O_CTLECH;
		else
			csp->t_flags &= ~O_CTLECH;
	}
	if (termiosp->c_oflag & OLCUC)
		csp->t_flags |= O_LCASE;
	if (termiosp->c_lflag&ECHO)
		csp->t_flags |= O_ECHO;
	if (termiosp->c_oflag & ONLCR) {
		csp->t_flags |= O_CRMOD;
		switch (termiosp->c_oflag & CRDLY) {

		case CR2:
			csp->t_flags |= O_CR1;
			break;

		case CR3:
			csp->t_flags |= O_CR2;
			break;
		}
	} else {
		if ((termiosp->c_oflag & CR1)
		    && (termiosp->c_oflag & ONLRET))
			csp->t_flags |= O_NL1;	/* tty37 */
	}
	if ((termiosp->c_oflag & ONLRET) && (termiosp->c_oflag & NL1))
		csp->t_flags |= O_NL2;
	switch (termiosp->c_oflag & TABDLY) {

	case TAB1:
		csp->t_flags |= O_TAB1;
		break;

	case TAB2:
		csp->t_flags |= O_TAB2;
		break;

	case XTABS:
		csp->t_flags |= O_XTABS;
		break;
	}
	if (termiosp->c_oflag & FFDLY)
		csp->t_flags |= O_VTDELAY;
	if (termiosp->c_oflag & BSDLY)
		csp->t_flags |= O_BSDELAY;
	if (termiosp->c_lflag & ECHOPRT)
		csp->t_flags |= O_PRTERA;
	if (termiosp->c_lflag & ECHOE)
		csp->t_flags |= (O_CRTERA|O_CRTBS);
	if (termiosp->c_lflag & TOSTOP)
		csp->t_flags |= O_TOSTOP;
	if (termiosp->c_lflag & FLUSHO)
		csp->t_flags |= O_FLUSHO;
	if (termiosp->c_cflag & CLOCAL)
		csp->t_flags |= O_NOHANG;
	if (termiosp->c_lflag & ECHOKE)
		csp->t_flags |= O_CRTKIL;
	if (termiosp->c_lflag & PENDIN)
		csp->t_flags |= O_PENDIN;
	if (!(termiosp->c_iflag & IXANY))
		csp->t_flags |= O_DECCTQ;
	if (termiosp->c_lflag & NOFLSH)
		csp->t_flags |= O_NOFLSH;
	if (termiosp->c_lflag & ICANON) {
		TO_COMPAT_CHAR(csp->t_eofc, termiosp->c_cc[VEOF]);
		TO_COMPAT_CHAR(csp->t_brkc, termiosp->c_cc[VEOL]);
	}
}
