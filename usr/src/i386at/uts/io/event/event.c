#ident	"@(#)event.c	1.10"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	21Jan97		keithp@sco.com
 *	- Fixes for SCO event queue. The eventclose routine is broken in that
 *	  ws_queuemode(chp, LDEV_ATTACHQ, 0) does not clean up as expected.
 *	  Call ws_notify() directly to take char module out of queue mode.
 *
 */

/*
 * Event clone driver
 */

#include <io/conf.h>
#include <io/event/event.h>
#include <io/gvid/genvid.h>
#include <io/stream.h>
#include <io/termios.h>
#include <io/uio.h>
#include <io/xque/xque.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>

#include <io/ddi.h>	/* must come last */


#ifdef DEBUG
STATIC int event_debug = 0;
#define EVENT_DEBUG(a)		if (event_debug) cmn_err(CE_NOTE, a) 
#else
#define EVENT_DEBUG(a)
#endif /* DEBUG */


STATIC int	eventopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	eventclose(queue_t *, int, cred_t *);
STATIC int	eventwput(queue_t *, mblk_t *);

STATIC void	event_iocack(struct evchan *, mblk_t *, struct iocblk *, int);
STATIC void	event_iocnack(struct evchan *, mblk_t *, struct iocblk *, 
				int, int);
STATIC void	event_copyout(struct evchan *, mblk_t *, mblk_t *, uint); 
STATIC void	event_copyin(struct evchan *, mblk_t *, int);
STATIC void	event_do_iocdata(queue_t *, mblk_t *, struct evchan *);


static struct module_info event_info = {
	0xdfdf, "event", 0, 512, 512, 128
};

static struct qinit eventrint = {
	NULL, NULL, eventopen, eventclose,NULL,&event_info, NULL
};

static struct qinit eventwint = {
	eventwput, NULL, NULL, NULL, NULL,&event_info, NULL
};

struct streamtab eventinfo = {
	&eventrint, &eventwint, NULL, NULL
};

/*
 * evchan_dev[] and evchan_cnt are defined in master.d file 
 */
extern struct evchan evchan_dev[];	
extern int	evchan_cnt;

extern int	gviddevflag;
extern gvid_t	Gvid;
extern int	gvidflg; 
extern lock_t	*gvid_mutex;
extern sv_t	*gvidsv;
 
int eventdevflag = 0;


/*
 * STATIC int
 * eventopen(queue_t *, dev_t *, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int 
eventopen(queue_t *rqp, dev_t *devp, int flag, int sflag, cred_t *crp)
{
	struct evchan *evchanp = NULL;
	dev_t	ttyd;
	pl_t	pl;
	int	majnum, minnum, ndev, error;


	if (error = ws_getctty(&ttyd))
		return error;

 	majnum = getmajor(ttyd);
 
	pl = LOCK(gvid_mutex, plhi);

 	while (gvidflg & GVID_ACCESS) /* sleep */
		if (!SV_WAIT_SIG(gvidsv, primed - 3, gvid_mutex)) {
			/*
 			 * even if ioctl was not ours, we've
 			 * effectively handled it 
			 */
 			return (EINTR);
 		}

	gvidflg |= GVID_ACCESS; 
	
 	/* return if controlling tty is gvid */
 	if (majnum != Gvid.gvid_maj) {
		gvidflg &= ~GVID_ACCESS; 
		SV_SIGNAL(gvidsv, 0);
		UNLOCK(gvid_mutex, pl);
		return (ENXIO);
	}

 	minnum = getminor(ttyd);
#ifdef DEBUG
	cmn_err(CE_NOTE, "major %d minor %d", majnum, minnum);
#endif

 	if (minnum >= Gvid.gvid_num) {
		gvidflg &= ~GVID_ACCESS; 
		SV_SIGNAL(gvidsv, 0);
		UNLOCK(gvid_mutex, pl);
		return (ENXIO);
 	}
 
	/* done with lookup in Gvid. Release access to it */
	gvidflg &= ~GVID_ACCESS; 
	SV_SIGNAL(gvidsv, 0);
	UNLOCK(gvid_mutex, pl);

	EVENT_DEBUG(("entering eventopen\n"));

	if (sflag != CLONEOPEN) {
		EVENT_DEBUG(("invalid sflag\n"));
		return (EINVAL);
	}

	for (ndev = 0; ndev < evchan_cnt; ndev++)
		if (evchan_dev[ndev].eq_state == EVCH_CLOSE ||
				evchan_dev[ndev].eq_ttyd == ttyd)
			break;

	if (ndev >= evchan_cnt) {
		EVENT_DEBUG(("no more devices left to allocate\n"));
		return (ENODEV);
	}

#ifdef DEBUG
	cmn_err(CE_NOTE, "event ttyd is %x", ttyd);
#endif

	if (evchan_dev[ndev].eq_ttyd == ttyd) {
		EVENT_DEBUG(("EXLUSIVE OPEN once per channel\n"));
		return (EINVAL);
	}

	evchanp = &evchan_dev[ndev];
	if (xq_allocate_scoq(evchanp, &error) == NULL)
		return (error);

	evchan_dev[ndev].eq_ttyd =  ttyd;
	evchan_dev[ndev].eq_rdev =  0;
	evchan_dev[ndev].eq_chp  =  NULL;
	evchan_dev[ndev].eq_rqp  =  (caddr_t) rqp;
	evchan_dev[ndev].eq_block_msg  =  NULL;
	evchanp = &evchan_dev[ndev];
	evchanp->eq_state = EVCH_OPEN;
	evchanp->eq_xqinfo.xq_private = (caddr_t) evchanp;
	evchanp->eq_xqinfo.xq_buttons = 07; /* start with all buttons up */
	evchanp->eq_emask = 0 ; /* T_STRING|T_BUTTON|T_REL_LOCATOR; */

	rqp->q_ptr = evchanp;
	WR(rqp)->q_ptr = evchanp;
	*devp = makedevice(getemajor(*devp), ndev);

	/* enable the put and service procedures of the queue pair */
	qprocson(rqp);
	
	EVENT_DEBUG(("returning from eventopen()\n"));

	return (ndev);
}


/*
 * int
 * event_check_que(xqInfo *, dev_t, void *, int)
 *
 * Calling/Exit State:
 *	None.
 */
int
event_check_que(xqInfo *qp, dev_t rdev, void *chp, int cmd)
{
	int	ndev, type;
	void	*procp;


#ifdef DEBUG
	cmn_err(CE_NOTE, "checking for tty %x", rdev);
#endif

	for (ndev = 0; ndev < evchan_cnt; ndev++)
		if (evchan_dev[ndev].eq_state != EVCH_CLOSE &&
		    evchan_dev[ndev].eq_rdev == rdev)
			break;

	if (ndev < evchan_cnt) {
		if (cmd == LDEV_MSEATTACHQ) {
			evchan_dev[ndev].eq_xqinfo.xq_devices |= QUE_MOUSE;
			type = T_REL_LOCATOR|T_BUTTON;
		} else {
			evchan_dev[ndev].eq_xqinfo.xq_devices |= QUE_KEYBOARD;
			type = T_STRING;
		}
		evchan_dev[ndev].eq_emask |= type;
#ifdef DEBUG
		cmn_err(CE_NOTE, "event_check_que: emask %o",
				evchan_dev[ndev].eq_emask);
#endif
		/*
		 * copy the state of evchan xqInfo into the channel xqInfo.
		 * [xqInfo structure assignment (qp is a pointer to the
		 * the channel xqInfo structure which is being assigned
		 * the evchan xqInfo data structure)]
		 *
		 * Get another reference to the process to ensure that 
		 * there are two references to the process. One obtained for
		 * the sco event xqInfo and the other for the channel xqInfo.
		 * This reference is released by xq_close(). The calling
		 * sequence is:
		 *
		 * eventclose() --> ws_queuemode() --> xq_close() --> proc_unref() 
		 */
		ASSERT(proc_valid(evchan_dev[ndev].eq_xqinfo.xq_proc));
		procp = proc_ref();
		*qp = evchan_dev[ndev].eq_xqinfo;
		ASSERT(procp == qp->xq_proc);
		evchan_dev[ndev].eq_chp = chp;
		return (0);
	}

#ifdef DEBUG
	cmn_err(CE_NOTE, "checking for tty %x NOT FOUND", rdev);
#endif

	return (ENODEV);
}


/*
 * STATIC int
 * eventclose(queue_t *, int cflag, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int 
eventclose(queue_t *rqp, int cflag, cred_t *crp)
{
	struct evchan *evchanp;


	EVENT_DEBUG(("entering eventclose"));

	evchanp = (struct evchan *) rqp->q_ptr;

	if (evchanp->eq_block_msg) 
		/* LINTED pointer alignment */
		freemsg((mblk_t *) evchanp->eq_block_msg);

	/* disable the put and service procedures of the queue pair */
	qprocsoff(rqp);

	evchanp->eq_rqp = NULL;
	evchanp->eq_block_msg = NULL;

	xq_close_scoq(&evchanp->eq_xqinfo);

	evchanp->eq_state = EVCH_CLOSE;
	evchanp->eq_ttyd  = 0;
	evchanp->eq_rdev  = 0;

	if (evchanp->eq_chp)
		ws_notify(evchanp->eq_chp, 0);		/* L000 */
	evchanp->eq_chp = NULL;

	EVENT_DEBUG(("returning from eventclose\n"));

	return (0);
}


/*
 * STATIC void 
 * event_set_dev(struct evchan *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
event_set_dev(struct evchan *evchanp, mblk_t *mp)
{
	struct	event_getq_info *xp;


	/* LINTED pointer alignment */
	xp = ((struct event_getq_info *) mp->b_rptr);
	evchanp->eq_rdev = xp->einfo_rdev;
#ifdef DEBUG
	cmn_err(CE_NOTE, "event_set_dev rdev is %d", evchanp->eq_rdev);
#endif

}


/*
 * void 
 * event_wakeup(struct evchan *)
 *
 * Calling/Exit State:
 *	None.
 */
void
event_wakeup(struct evchan *evchanp)
{
	struct iocblk *iocp;
	mblk_t *mp;


	/* LINTED pointer alignment */
	if ((mp = (mblk_t *) evchanp->eq_block_msg)) {
		EVENT_DEBUG(("wake up ioctl is sent"));
		/* LINTED pointer alignment */
		iocp = (struct iocblk *)mp->b_rptr;
		event_iocack(evchanp, mp, iocp, 0);
		evchanp->eq_block_msg = NULL;
	}
}


/*
 * STATIC int 
 * eventwput(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int 
eventwput(queue_t *qp, mblk_t *mp)
{
	struct evchan *evchanp;
	struct iocblk *iocp;
	mblk_t *bp;
	int transparent;
	
	
	EVENT_DEBUG(("entering eventwput\n"));

	evchanp = (struct evchan *) qp->q_ptr;
		
	switch(mp->b_datap->db_type) {
	case M_FLUSH:
		EVENT_DEBUG(("event got flush request\n"));
		flushq(qp, FLUSHDATA);
		flushq(WR(qp), FLUSHDATA);
		freemsg(mp);
		break;

	case M_IOCDATA:
		event_do_iocdata(qp, mp, evchanp);
		break;

	case M_IOCTL:
		/* LINTED pointer alignment */
		iocp = (struct iocblk *)mp->b_rptr;
		transparent = (iocp->ioc_count == TRANSPARENT);
		if (!transparent) {
			event_iocnack(evchanp, mp, iocp, EINVAL, -1);
			EVENT_DEBUG(("NOT transparent ioctl"));
			return (0);
		}
			
		switch(iocp->ioc_cmd) {
		case EQIO_GETQP:
#ifdef DEBUG
			cmn_err(CE_NOTE, "GOT EQIO_GETQP ioctl evchanp %x",
					 evchanp);
#endif
			if (!mp->b_cont) {
				event_iocnack(evchanp, mp, iocp, EINVAL, -1);
				EVENT_DEBUG(("NO b_cont ioctl evchanp"));
				break;
			}

			bp = allocb(sizeof(caddr_t), BPRI_MED);
			if (bp == NULL) {
				event_iocnack(evchanp, mp, iocp, EAGAIN, 0);
				break;
			}

			event_set_dev(evchanp, mp->b_cont);
			evchanp->eq_state = EVCH_ACTIVE;

			/* LINTED pointer alignment */
			*((caddr_t *) bp->b_rptr) = 
				((caddr_t ) evchanp->eq_xqinfo.xq_uaddr);
			bp->b_wptr = bp->b_rptr + sizeof(caddr_t);
			event_copyout(evchanp, mp, bp, sizeof(caddr_t));
#ifdef DEBUG
			cmn_err(CE_NOTE, 
				"event address is %x", evchanp->eq_xqinfo.xq_uaddr);
#endif

			break;

		case EQIO_BLOCK: {
			QUEUE *qp;

			EVENT_DEBUG(("GOT EQIO_BLOCK ioctl"));
			
			if (evchanp->eq_state != EVCH_ACTIVE) {
				event_iocnack(evchanp, mp, iocp, ENODEV, -1);
				return (0);
			}

			/* LINTED pointer alignment */
			qp = (QUEUE *) evchanp->eq_xqinfo.xq_qaddr;
			if (qp->head != qp->tail) {
				event_iocack(evchanp, mp, iocp, 0);
				break;
			}
			evchanp->eq_block_msg = (caddr_t) mp;
#ifdef DEBUG
			cmn_err(CE_NOTE, 
				"event_block: evanchp %x, qp %x, mp %x",
				evchanp, qp, mp);
#endif
			break;
		}

		case EQIO_SETEMASK:
			EVENT_DEBUG(("GOT EQIO_SETEMASK ioctl"));

			if (!mp->b_cont) {
				event_iocnack(evchanp, mp, iocp, EINVAL, -1);
				break;
			}

			evchanp->eq_emask = 
				/* LINTED pointer alignment */
				*((event_mask_t *) mp->b_cont->b_rptr);
			event_iocack(evchanp, mp, iocp, 0);
			break;

		case EQIO_GETEMASK:
			EVENT_DEBUG(("GOT EQIO_GETEMASK ioctl"));

			bp = allocb(sizeof(event_mask_t), BPRI_MED);
			if (bp == NULL) {
				event_iocnack(evchanp, mp, iocp, EAGAIN, 0);
				break;
			}
			/* LINTED pointer alignment */
			*((event_mask_t *) bp->b_rptr) = evchanp->eq_emask;
			bp->b_wptr = bp->b_rptr + sizeof(event_mask_t);
			event_copyout(evchanp, mp, bp, sizeof(event_mask_t));
			break;

		case EQIO_SUSPEND:
			EVENT_DEBUG(("GOT EQIO_SUSPEND ioctl"));

			if (evchanp->eq_state != EVCH_ACTIVE) {
				event_iocnack(evchanp, mp, iocp, EINVAL, -1);
			} else {
				evchanp->eq_state = EVCH_SUSPENDED;
				event_iocack(evchanp, mp, iocp, 0);
			}
			break;

		case EQIO_RESUME:
			EVENT_DEBUG(("GOT EQIO_RESUME ioctl"));

			if (evchanp->eq_state != EVCH_SUSPENDED)
				event_iocnack(evchanp, mp, iocp, EINVAL, -1);
			else {
				event_iocack(evchanp, mp, iocp, 0);
				evchanp->eq_state = EVCH_ACTIVE;
			}
			break;

		default:
			EVENT_DEBUG(("GOT unknown ioctl"));

			event_iocnack(evchanp, mp, iocp, EINVAL, -1);
		}

		break;

	default:
		event_iocnack(evchanp, mp, iocp, EINVAL, -1);
	}

	EVENT_DEBUG(("return from eventwput()\n"));

	return (0);
}


/*
 * STATIC void 
 * event_iocack(struct evchan *, mblk_t *, struct iocblk *, int)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
event_iocack(struct evchan *evchanp, mblk_t *mp, struct iocblk *iocp, int rval)
{
	mblk_t	*tmp;
	queue_t *qp;


	EVENT_DEBUG(("ACKNOWLDGEMENT"));

	/* LINTED pointer alignment */
	qp = (queue_t *) evchanp->eq_rqp;
	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;
	if ((tmp = unlinkb(mp)) != (mblk_t *)NULL)
		freemsg(tmp);
	putnext(qp, mp);
}


/*
 * STATIC void 
 * event_iocnack(struct evchan *, mblk_t *, struct iocblk *, int, int)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
event_iocnack(struct evchan *evchanp, mblk_t *mp, struct iocblk *iocp, 
		int error, int rval)
{
	queue_t *qp;


	EVENT_DEBUG(("NACKNOWLDGEMENT"));

	/* LINTED pointer alignment */
	qp = (queue_t *) evchanp->eq_rqp;
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	putnext(qp,mp);
}


/*
 * STATIC void 
 * event_copyout(struct evchan *, mblk_t *, mblk_t *, uint)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
event_copyout(struct evchan *evchanp, mblk_t *mp, mblk_t *nmp, uint size)
{
	struct copyreq *cqp;
	queue_t *qp;


	EVENT_DEBUG(("BEGIN OF event_copyout"));

	/* LINTED pointer alignment */
	qp = (queue_t *) evchanp->eq_rqp;
	/* LINTED pointer alignment */
	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t) * (long *) mp->b_cont->b_rptr;
#ifdef DEBUG
	cmn_err(CE_NOTE, "Copying into user address %x",
			cqp->cq_addr);
#endif

	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *) NULL;

	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;

	if (mp->b_cont) 
		freemsg(mp->b_cont);
	mp->b_cont = nmp;

	putnext(qp, mp);

	EVENT_DEBUG(("END OF event_copyout"));
}


/*
 * STATIC void 
 * event_copyin(struct evchan *, mblk_t *, int)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
event_copyin(struct evchan *evchanp, mblk_t *mp, int size)
{
	queue_t *qp;
	struct copyreq *cqp;

	EVENT_DEBUG(("BEGIN OF event_copyin"));

	/* LINTED pointer alignment */
	qp = (queue_t *) evchanp->eq_rqp;
	/* LINTED pointer alignment */
	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t) * (long *) mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *) NULL;

	if (mp->b_cont) 
		freemsg(mp->b_cont);
	mp->b_cont = NULL;

	mp->b_datap->db_type = M_COPYIN;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);

	putnext(qp, mp);

	EVENT_DEBUG(("END OF event_copyin"));
}


/*
 * STATIC void 
 * event_do_iocdata(queue_t *, mblk_t *, struct evchan *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
event_do_iocdata(queue_t *qp, mblk_t *mp, struct evchan *evchanp)
{
	struct iocblk *iocp;
	struct copyresp *csp;


	EVENT_DEBUG(("BEGIN OF event_do_iocdata"));

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;
	/* LINTED pointer alignment */
	csp = (struct copyresp *) mp->b_rptr;

	switch (iocp->ioc_cmd) {
	case EQIO_GETEMASK:
	case EQIO_GETQP:
		EVENT_DEBUG(("IOCDATA of type EQIO_GETEMASK/EQIO_GETQP"));

		if (csp->cp_rval) {
			freemsg(mp);
			EVENT_DEBUG(("END OF event_do_iocdata"));
			return;
		}

		event_iocack(evchanp, mp, iocp, 0);
		break;
	default:
#ifdef DEBUG
		cmn_err(CE_NOTE, "IOCDATA of type %d", iocp->ioc_cmd);
#endif
		break;
	}

	EVENT_DEBUG(("END OF event_do_iocdata"));
}
