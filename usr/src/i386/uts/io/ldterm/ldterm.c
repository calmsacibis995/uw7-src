#ident	"@(#)kern-i386:io/ldterm/ldterm.c	1.45.3.1"
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
 * Standard Streams Terminal Line Discipline module.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <io/termios.h>
#include <io/termio.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <proc/signal.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <io/ldterm/euc.h>
#include <io/ldterm/eucioctl.h>
#include <io/ldterm/ldterm.h>
#include <svc/systm.h>
#include <proc/cred.h>
#include <util/ksynch.h>
#include <mem/kmem.h>
#include <io/ddi.h>

STATIC int ldtermopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp);
STATIC int ldtermclose(queue_t *q, int cflag, cred_t *crp);
STATIC int ldtermrput(queue_t *q, mblk_t *mp);
STATIC int ldtermrsrv(queue_t *q);
STATIC int ldtermwput(queue_t *q, mblk_t *mp);

#ifdef DEBUG
STATIC int ldterm_debug = 0;
#define	DEBUG1(a)	if (ldterm_debug == 1) printf a
#define	DEBUG2(a)	if (ldterm_debug >= 2) printf a /* allocations */
#define	DEBUG3(a)	if (ldterm_debug >= 3) printf a /* M_CTL Stuff */
#define	DEBUG4(a)	if (ldterm_debug >= 4) printf a /* M_READ Stuff */
#define	DEBUG5(a)	if (ldterm_debug >= 5) printf a
#define	DEBUG6(a)	if (ldterm_debug >= 6) printf a
#define	DEBUG7(a)	if (ldterm_debug >= 7) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#define DEBUG3(a)
#define DEBUG4(a)
#define DEBUG5(a)
#define DEBUG6(a)
#define DEBUG7(a)
#endif /* DEBUG */

#define LDHIER	1	/* ldterm lock hierarchy value */

/*
 * Since most of the buffering occurs either at the stream head or in
 * the "message currently being assembled" buffer, we have a relatively
 * small input queue, so that blockages above us get reflected fairly
 * quickly to the module below us.  We also have a small maximum packet
 * size, since you can put a message of that size on an empty queue no
 * matter how much bigger than the high water mark it is.
 */
STATIC struct module_info ldtermmiinfo = {
	0x0bad,
	"ldterm",
	0,
	256,
	512,
	200
};

STATIC struct qinit ldtermrinit = {
	ldtermrput,
	ldtermrsrv,
	ldtermopen,
	ldtermclose,
	NULL,
	&ldtermmiinfo
};

STATIC struct module_info ldtermmoinfo = {
	0x0bad,
	"ldterm",
	0,
	INFPSZ,
	1,
	0
};

STATIC struct qinit ldtermwinit = {
	ldtermwput,
	NULL,
	ldtermopen,
	ldtermclose,
	NULL,
	&ldtermmoinfo
};

struct streamtab ldterminfo = {
	&ldtermrinit,
	&ldtermwinit,
	NULL,
	NULL
};

STATIC void ldtermopen_wakeup(long);
STATIC mblk_t *ldterm_docanon(mblk_t *, mblk_t *, int, queue_t *, lstate_t *);
STATIC int ldterm_unget(lstate_t *);
STATIC void ldterm_trim(lstate_t *);
STATIC void ldterm_rubout(uchar_t, queue_t *, int, lstate_t *);
STATIC int ldterm_tabcols(lstate_t *);
STATIC void ldterm_erase(queue_t *, int, lstate_t *);
STATIC void ldterm_werase(queue_t *, int, lstate_t *);
STATIC void ldterm_tokerase(queue_t *, int, lstate_t *);
STATIC void ldterm_kill(queue_t *, int, lstate_t *);
STATIC void ldterm_reprint(queue_t *, int, lstate_t *);
STATIC mblk_t *ldterm_dononcanon(mblk_t *, mblk_t *, int, queue_t *, lstate_t *);
STATIC int ldterm_echo(uchar_t, queue_t *, int, lstate_t *);
STATIC void ldterm_outstring(uchar_t *, int, queue_t *, int, lstate_t *);
STATIC mblk_t *newmsg(lstate_t *);
STATIC void ldterm_msg_upstream(queue_t *, lstate_t *);
STATIC mblk_t *ldterm_output_msg(queue_t *, mblk_t *, mblk_t *, lstate_t *, int, int);
STATIC int movtuc(int, uchar_t *, uchar_t *, uchar_t *);
STATIC void ldterm_flush_output(uchar_t, queue_t *, lstate_t *);
STATIC void ldterm_dosig(queue_t *, int, uchar_t, int, int, int);
STATIC void ldterm_do_ioctl(queue_t *, mblk_t *);
STATIC int chgstropts(struct termios *, lstate_t *, queue_t *);
STATIC void ldterm_ioctl_reply(queue_t *, mblk_t *);
STATIC void ldterm_vmin_timeout_l(queue_t *);
STATIC void ldterm_vmin_timeout(queue_t *);
STATIC void ldterm_adjust_modes(lstate_t *);
STATIC void ldterm_euc_erase(queue_t *, int, lstate_t *);
STATIC void ldterm_eucwarn(lstate_t *);
STATIC void cp_eucwioc(eucioc_t *, eucioc_t *, int);
STATIC int ldterm_memwidth(unchar, eucioc_t *);
STATIC unsigned char ldterm_dispwidth(unchar, eucioc_t *, int);

/*
 * ldterm_outchar() is also used by io/intmap/chanmap.c
 */
void ldterm_outchar(uchar_t, queue_t *, int, lstate_t *);

int ldtermdevflag = D_MP;

/*
 *+ t_lock is a per instantiation spin lock that protects the associated
 *+ state information
 */
LKINFO_DECL(ldterm_lkinfo, "LDTERM::t_lock", 0);

STATIC struct termios initmodes = {
	BRKINT|ICRNL|IXON|ISTRIP,	/* iflag */
	OPOST|ONLCR|TAB3,		/* oflag */
	0,				/* cflag */
	ISIG|ICANON|ECHO|ECHOK,	/* lflag , note IEXTEN is turned off, no extensions*/
	{	CINTR,
		CQUIT,
		CERASE,
		CKILL,
		CEOF,
		CEOL,
		CEOL2,
		CNSWTCH,
		CSTART,
		CSTOP,
		CSUSP,
		CNUL,
		CRPRNT,
		CFLUSH,
		CWERASE,
		CLNEXT,
		0			/* nonexistent STATUS */
	}
};

/*
 * Locking strategy: tp->t_lock protects the entire state structure.  The
 * lock is always acquired at plstr, and routines called with the lock
 * held will maintain the ipl at plstr throughout
 */

/*
 * int
 * ldtermopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	Line discipline open.  Allocate space for state information,
 *	initialize it, and enable put procedures.
 *
 * Calling/Exit State:
 *	Called with no locks held and put procedure disabled.  On return, put
 *	procedure is enabled and state information is initialized.
 */

/*ARGSUSED*/
STATIC	int
ldtermopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	lstate_t *tp;
	mblk_t *bp;
	mblk_t *qryp;
	struct iocblk *qiocp;
	struct stroptions *strop;

	if (q->q_ptr != NULL)
		return (0);		/* already attached */

	if (sflag != MODOPEN)
		return (EINVAL);

	/* note that this is a kmem_zalloc.  KM_NOSLEEP is specified to
	 * to honor the possibility of O_NONBLOCK or O_NDELAY.  If we
	 * can't even get enough memory to set up our state structure,
	 * just give up.
	 */
	if ((tp = (lstate_t *) kmem_zalloc((int)sizeof(lstate_t), KM_NOSLEEP)) == NULL)
		return (ENOSPC);

	if ((tp->t_lock = LOCK_ALLOC(LDHIER, plstr, &ldterm_lkinfo, KM_NOSLEEP)) == NULL) {
		kmem_free(tp, sizeof(lstate_t));
		return(ENOSPC);
	}
	if ((tp->t_event = EVENT_ALLOC(KM_NOSLEEP)) == NULL) {
		LOCK_DEALLOC(tp->t_lock);
		kmem_free(tp, sizeof(lstate_t));
		return(ENOSPC);
	}
	tp->t_modes = initmodes;
	tp->t_amodes = initmodes;

	q->q_ptr = (caddr_t)tp;
	WR(q)->q_ptr = (caddr_t)tp;
	/*
	 * The following for EUC:
	 */
	tp->eucwioc.eucw[0] = 1;	/* ASCII mem & screen width */
	tp->eucwioc.scrw[0] = 1;
	tp->t_maxeuc = 1;	/* the max length in memory bytes of an EUC character */


	/*
	 * Set the "vmin" and "vtime" values to 1 and 0, turn on
	 * message-nondiscard mode (as we're in ICANON mode), and
	 * turn on "old-style NODELAY" mode.
	 */
	while ((bp = allocb((int)sizeof (struct stroptions), BPRI_MED)) == NULL) {
		tp->t_bid = bufcall(sizeof(struct stroptions), BPRI_MED,
		    ldtermopen_wakeup, (long)tp->t_event);
		if (tp->t_bid == NULL) {
			LOCK_DEALLOC(tp->t_lock);
			EVENT_DEALLOC(tp->t_event);
			kmem_free(tp, sizeof(lstate_t));
			q->q_ptr = NULL;
			WR(q)->q_ptr = NULL;
			return (ENOSPC);
		}
		if (EVENT_WAIT_SIG(tp->t_event, primed) == B_FALSE) {
			if (tp->t_bid)
				unbufcall(tp->t_bid);
			/* Dump the state structure, then unlink it */
			LOCK_DEALLOC(tp->t_lock);
			EVENT_DEALLOC(tp->t_event);
			kmem_free(tp, sizeof(lstate_t));
			q->q_ptr = NULL;
			WR(q)->q_ptr = NULL;
			return (EINTR);
		}
		tp->t_bid = 0;
	}
	/* LINTED pointer alignment */
	strop = (struct stroptions *) bp->b_wptr;
	strop->so_flags =
	    SO_READOPT|SO_NDELON|SO_ISTTY;
	strop->so_readopt = RMSGN;
	bp->b_wptr += sizeof (struct stroptions);
	bp->b_datap->db_type = M_SETOPTS;
	putnext(q, bp);

	/*
	 * Find out if the module below us does canonicalization; if so,
	 * we won't do it ourselves.
	 */

	while ( !(qryp = allocb(sizeof (struct iocblk), BPRI_HI))) {
		tp->t_bid = bufcall(sizeof(struct iocblk), BPRI_HI, ldtermopen_wakeup, (long)tp->t_event);
		if (tp->t_bid == NULL) {
			LOCK_DEALLOC(tp->t_lock);
			EVENT_DEALLOC(tp->t_event);
			kmem_free(tp, sizeof(lstate_t));
			q->q_ptr = NULL;
			WR(q)->q_ptr = NULL;
			return (ENOSPC);
		}
		if (EVENT_WAIT_SIG(tp->t_event, primed) == B_FALSE) {
			if (tp->t_bid)
				unbufcall(tp->t_bid);
			/* Dump the state structure, then unlink it */
			LOCK_DEALLOC(tp->t_lock);
			EVENT_DEALLOC(tp->t_event);
			kmem_free(tp, sizeof(lstate_t));
			q->q_ptr = NULL;
			WR(q)->q_ptr = NULL;
			return (EINTR);
		}
		tp->t_bid = 0;
	}
	/* Formulate an M_CTL message; The first block looks like 
	 * an iocblk. Set the command and datasize. The actual data
	 * will be in the b_cont field.
	 */

	qryp->b_wptr = qryp->b_rptr + sizeof (struct iocblk);
	qryp->b_datap->db_type = M_CTL;
	/* LINTED pointer alignment */
	qiocp = (struct iocblk *) qryp->b_rptr;
	qiocp->ioc_count = 0;	/* count is valid for return only */
	qiocp->ioc_error = 0;
	qiocp->ioc_rval = 0;
	qiocp->ioc_cmd = MC_CANONQUERY;

	qprocson(q);

	putnext(WR(q), qryp);

	return (0);	/* this can become a controlling TTY */
}

/*
 * void
 * ldtermopen_wakeup(long addr)
 *	Wake up an lwp that has been sleeping, waiting for memory.
 *
 * Calling/Exit State:
 *	Posts an even for lwp's that are awaiting memory.  No locks held.
 */

STATIC	void
ldtermopen_wakeup(long addr)
{
	EVENT_SIGNAL((event_t *)addr, 0);
}

/*
 * int
 * ldtermclose(queue_t *q, int cflag, cred_t *crp)
 *	Line discipline close.  Clean up relevant information in the
 *	stream, disable put procedure, and free memory
 *
 * Calling/Exit State:
 *	No locks held.  All related information freed on return.
 */

/*ARGSUSED*/
STATIC	int
ldtermclose(queue_t *q, int cflag, cred_t *crp)
{
	lstate_t *tp;
	mblk_t *bp;
	struct stroptions *strop;
	int tid;
	pl_t ipl;

	tp = (lstate_t *) q->q_ptr;
	qprocsoff(q);

	/*
	 * if there is a pending timeout, clear it
	 * Note: after the timeout is cleared, there can be no concurrent
	 * activity, so don't need locks anymore.  t_tid can change if
	 * the timeout fires and reschedules itself before we do the
	 * untimeout, so TS_TACT is the key..
	 */
	ipl = LOCK(tp->t_lock, plstr);
	while (tp->t_tid && (tp->t_state & TS_TACT)) {
		tp->t_state &= ~TS_TACT;
		DEBUG4 (("ldtermclose: timer active untimeout called \n"));
		tid = tp->t_tid;
		UNLOCK(tp->t_lock, ipl);
		untimeout(tid);
		ipl = LOCK(tp->t_lock, plstr);
		tp->t_state &= ~TS_RTO;
		tp->t_lbolt = 0;
	}
	UNLOCK(tp->t_lock, ipl);

	if (tp->t_message != NULL)
		freemsg(tp->t_message);
	/*
	 * turn on byte-stream mode, and turn off "old-style NODELAY" mode.
	 */
	bp = allocb((int)sizeof (struct stroptions), BPRI_MED);
	/*
	 * If the allocb fails, don't wait for memory - we're going to get
	 * popped don't want to wait around forever.
	 */
	if (bp) {
		/* LINTED pointer alignment */
		strop = (struct stroptions *) bp->b_wptr;
		strop->so_flags = SO_READOPT|SO_NDELOFF;
		strop->so_readopt = RNORM;
		bp->b_wptr += sizeof (struct stroptions);
		bp->b_datap->db_type = M_SETOPTS;
		putnext(q, bp);
	}
	/*
	 * Restart output, since it's probably got nowhere to
	 * go anyway, and we're probably not going to see
	 * another ^Q for a while.
	 */
	if (tp->t_state & TS_TTSTOP) {
		tp->t_state &= ~TS_TTSTOP;
		(void) putnextctl(WR(q), M_START);
	}
	/* 
 	 * The following for EUC:
	 */
	if (tp->t_eucp_mp)
		freemsg(tp->t_eucp_mp);

	chanmap_close(q, tp);

	/* Dump the state structure, then unlink it */
	LOCK_DEALLOC(tp->t_lock);
	EVENT_DEALLOC(tp->t_event);
	kmem_free(tp, sizeof(lstate_t));
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	return(0);
}

/*
 * int
 * ldtermrput(queue_t *q, mblk_t *mp)
 *	Read side put procedure.  Takes message mp, makes first pass
 *	over it, either queueing it for the service procedure or
 *	processing it immediately.
 * Calling/Exit State:
 *	No locks held on entry or exit.  On return, message has been
 *	appropriately dispatched.
 */

STATIC	int
ldtermrput(queue_t *q, mblk_t *mp)
{
	lstate_t *tp;
	uchar_t c;
	queue_t *wrq;		/* write queue of ldterm mod */
	mblk_t *bp;
	mblk_t *tbp;
	struct iocblk *qryp;
	uchar_t *readp;
	uchar_t *writep;
	struct termios *emodes;		/* effective modes set by driver */
	struct termios locmodes;	/* local copy of t_mode */
	int flushflag;
	long count;
	pl_t ipl;

	wrq = WR(q);
	tp = (lstate_t *)q->q_ptr;

	switch (mp->b_datap->db_type) {

	default:
		putq(q, mp);
		return(0);

	/* 
	 *  Send these up unmolested
	 *
	 */
	case M_PCSIG:

		putnext(q, mp);
		return(0);

	case M_BREAK:

		/*
		 * We look at the apparent modes here instead of the effective
		 * modes. Effective modes cannot be used if IGNBRK, BRINT
		 * and PARMRK have been negotiated to be handled by the
		 * driver. Since M_BREAK should be sent upstream only if
		 * break processing was not already done, it should be ok
		 * to use the apparent modes. 
		 */
		
		ipl = LOCK(tp->t_lock, plstr);
		if (!(tp->t_amodes.c_iflag & IGNBRK )) {
			if (tp->t_amodes.c_iflag & BRKINT) {
				flushflag = !(tp->t_modes.c_lflag & NOFLSH);
				UNLOCK(tp->t_lock, ipl);
				ldterm_dosig(q, SIGINT, '\0', M_PCSIG, FLUSHRW, flushflag);
				freemsg (mp);
				return(0);
			} else if (tp->t_amodes.c_iflag & PARMRK) {
					/*
					 *  Send '\377','\0', '\0', if room
					 */
					UNLOCK(tp->t_lock, ipl);
					if (canputnext(q)) {
						mp->b_datap->db_type = M_DATA;
						*mp->b_wptr++ = (unsigned char) '\377';
						*mp->b_wptr++ = '\0';
						*mp->b_wptr++ = '\0';
						putnext(q, mp);
					} else {
						freemsg(mp);
					}
					return(0);
				} else {
					/*
					 * Act as if a '\0' came in, send if
					 * room
					 */
					UNLOCK(tp->t_lock, ipl);
					if (canputnext(q)) {
						mp->b_datap->db_type = M_DATA;
						*mp->b_wptr++ = '\0';
						putnext(q, mp);
					} else {
						freemsg(mp);
					}
					return(0);
			}
		
		} else {
			freemsg(mp);
		}
		UNLOCK(tp->t_lock, ipl);
		return(0);

	case M_CTL:
		DEBUG3(("ldtermrput: M_CTL received\n"));
		/* The M_CTL has been standardized to look like an M_IOCTL
		 * message.
		 */

		if ((mp->b_wptr - mp->b_rptr) != sizeof (struct iocblk)) {
			DEBUG3 (("Non standard M_CTL received by the ldterm module\n"));
			/* May be for someone else; pass it on */
			putnext(q, mp);
			return(0);
		}
		/* LINTED pointer alignment */
		qryp = (struct iocblk *) mp->b_rptr;

		switch (qryp->ioc_cmd) {

		case MC_PART_CANON:

			DEBUG3(("ldtermrput: M_CTL Query Reply\n"));
			if (!mp->b_cont) {
				DEBUG3 (("No information in Query Message\n"));
				break;
			}
			if ((mp->b_cont->b_wptr - mp->b_cont->b_rptr) ==
			     sizeof (struct termios)) {
				DEBUG3(("ldtermrput: M_CTL GrandScheme\n"));
				/* elaborate turning off scheme */
				/* LINTED pointer alignment */
				emodes = (struct termios *) mp->b_cont->b_rptr;
				ipl = LOCK(tp->t_lock, plstr);
				bcopy ((caddr_t)emodes, (caddr_t)&tp->t_dmodes, sizeof (struct termios));
				ldterm_adjust_modes (tp);
				UNLOCK(tp->t_lock, ipl);
				break;
			} else {
				DEBUG3 (("Incorrect query replysize\n"));
				break;
			}

		case MC_NO_CANON:
			ipl = LOCK(tp->t_lock, plstr);
			tp->t_state |= TS_NOCANON;
			/*
			 * Note: this is very nasty.  It's not
			 * clear what the right thing to do
			 * with a partial message is; 
			 * We throw it out
			 */
			if (tp->t_message != NULL) {
				freemsg(tp->t_message);
				tp->t_message = NULL;
				tp->t_endmsg = NULL;
				tp->t_msglen = 0;
				tp->t_rocount = 0;
				tp->t_rocol = 0;
				if (tp->t_state & TS_MEUC) {
					ASSERT(tp->t_eucp_mp);
					tp->t_eucp = tp->t_eucp_mp->b_rptr;
					tp->t_eucleft = 0;
				}
			}
			UNLOCK(tp->t_lock, ipl);
			break;

		case MC_DO_CANON:
			ipl = LOCK(tp->t_lock, plstr);
			tp->t_state &= ~TS_NOCANON;
			UNLOCK(tp->t_lock, ipl);
			break;
		default:
			DEBUG3 (("Unknown M_CTL Message\n"));
			break;
		}
		putnext(q, mp);	/* In case anyone else has to see it */
		return(0);
		
	case M_DATA:
		break;
	}

	/*
	 * We only get here on M_DATAs
	 */
	drv_setparm(SYSRAWC, msgdsize(mp));
	ipl = freezestr(q);
	(void) strqget(q, QCOUNT, 0, &count);
	unfreezestr(q, ipl);

	ipl = LOCK(tp->t_lock, plstr);
	/*
	 * copy structure under lock and use throughout
	 */
	locmodes = tp->t_modes;

	/*
	 * Flow control: send "start input" message if blocked and
	 * our queue is below its low water mark, and we haven't done
	 * a tflow with TCIOFF.
	 */
	if ((locmodes.c_iflag & IXOFF) && (tp->t_state & TS_TBLOCK)
	    && count <= TTXOLO && !(tp->t_state & TS_FLOW) ) {
		tp->t_state &= ~TS_TBLOCK;
		UNLOCK(tp->t_lock, ipl);
		(void) putnextctl(wrq, M_STARTI);
		ipl = LOCK(tp->t_lock, plstr);
		DEBUG1 (("M_STARTI down\n"));
	}

	/*
	 * If somebody below us ("intelligent" communications board,
	 * pseudo-tty controlled by an editor) is doing
	 * canonicalization, don't scan it for special characters.
	 */
	if (tp->t_state & TS_NOCANON) {
		UNLOCK(tp->t_lock, ipl);
		putq(q, mp);
		return(0);
	}
	UNLOCK(tp->t_lock, ipl);

	bp = mp;

	do {
		readp = bp->b_rptr;
		writep = readp;
		/**tk_nin += bp->b_wptr - readp; **/
		if (locmodes.c_iflag & (INLCR|IGNCR|ICRNL|IUCLC|IXON)
		    || locmodes.c_lflag & (ISIG|ICANON)) {
			/*
			 * We're doing some sort of non-trivial processing
			 * of input; look at every character.
			 */
			while (readp < bp->b_wptr) {
				c = *readp++;

				if (locmodes.c_iflag & ISTRIP)
					c &= 0177;

				/*
				 * First, check that this hasn't been escaped
				 * with the "literal next" character.
				 */
				ipl = LOCK(tp->t_lock, plstr);
				if (tp->t_state & TS_PLNCH) {
					tp->t_state &= ~TS_PLNCH;
					tp->t_modes.c_lflag &= ~FLUSHO;
					UNLOCK(tp->t_lock, ipl);
					*writep++ = c;
					continue;
				}
				UNLOCK(tp->t_lock, ipl);

				/*
				 * Setting a special character to NUL disables
				 * it, so if this character is NUL, it should
				 * not be compared with any of the special
				 * characters.  It should, however, restart
				 * frozen output if IXON and IXANY are set.
				 */
				if (c == '\0') {
					ipl = LOCK(tp->t_lock, plstr);
					if (locmodes.c_iflag & IXON
					    && tp->t_state & TS_TTSTOP
					    && locmodes.c_iflag & IXANY) {
						tp->t_state  &= ~TS_TTSTOP;
						UNLOCK(tp->t_lock, ipl);
						(void) putnextctl(wrq, M_START);
						ipl = LOCK(tp->t_lock, plstr);
					}
					tp->t_modes.c_lflag &= ~FLUSHO;
					UNLOCK(tp->t_lock, ipl);
					*writep++ = c;
					continue;
				}

				/*
				 * If stopped, start if you can; if running,
				 * stop if you must.
				 */
				if (locmodes.c_iflag & IXON) {
					ipl = LOCK(tp->t_lock, plstr);
					if (tp->t_state & TS_TTSTOP) {
						if (c == locmodes.c_cc[VSTART]
						    || locmodes.c_iflag & IXANY) {
							tp->t_state &= ~TS_TTSTOP;
							UNLOCK(tp->t_lock, ipl);
							(void) putnextctl(wrq, M_START);
						}
						else
							UNLOCK(tp->t_lock, ipl);
					} else {
						if (c == locmodes.c_cc[VSTOP]) {
							tp->t_state |= TS_TTSTOP;
							UNLOCK(tp->t_lock, ipl);
							(void) putnextctl(wrq, M_STOP);
						}
						else
							UNLOCK(tp->t_lock, ipl);
					}
					if (c == locmodes.c_cc[VSTOP]
					    || c == locmodes.c_cc[VSTART])
						continue;
				}
				/*
				 * Check for "literal next" character
				 * and "flush output" character.
				 */
				if (locmodes.c_lflag & (ISIG|ICANON)) {
					if ((locmodes.c_lflag & IEXTEN) && c == locmodes.c_cc[VLNEXT]) {
						/*
						 * Remember that we saw
						 * a "literal next"
						 * while scanning
						 * input, but leave it
						 * in the message so
						 * that the service
						 * routine can see it
						 * too.
						 */
						ipl = LOCK(tp->t_lock, plstr);
						tp->t_state |= TS_PLNCH;
						tp->t_modes.c_lflag &= ~FLUSHO;
						UNLOCK(tp->t_lock, ipl);
						*writep++ = c;
						continue;
					}
					if ((locmodes.c_lflag & IEXTEN) && c == locmodes.c_cc[VDISCARD]) {
						ldterm_flush_output(c, wrq, tp);
						continue;
					}
				}

				ipl = LOCK(tp->t_lock, plstr);
				tp->t_modes.c_lflag &= ~FLUSHO;
				UNLOCK(tp->t_lock, ipl);

				/*
				 * Check for signal-generating characters.
				 */
				if (locmodes.c_lflag & ISIG) {
					flushflag = !(locmodes.c_lflag & NOFLSH);
					if (c == locmodes.c_cc[VINTR]) {
						ldterm_dosig(q, SIGINT, c, M_PCSIG, FLUSHRW, flushflag);
						if (flushflag) {
							freemsg(mp);
							return(0);
						}
						continue;
					}
					if (c == locmodes.c_cc[VQUIT]) {
						ldterm_dosig(q, SIGQUIT, c, M_PCSIG, FLUSHRW, flushflag);
						if (flushflag) {
							freemsg(mp);
							return(0);
						}
						continue;
					}
					if (c == locmodes.c_cc[VSWTCH]) {
						/* This case used to handle SXT */
						continue;
					}
					if (c == locmodes.c_cc[VSUSP]) {
						ldterm_dosig(q, SIGTSTP, c, M_PCSIG, FLUSHR, flushflag);
						if (flushflag) {
							freemsg(mp);
							return(0);
						}
						continue;
					}
					if (c == locmodes.c_cc[VDSUSP]) {
						/*
						 * This is a best approximation
						 * of the DSUSP semantic
						 */
						tbp = NULL;
						ipl = LOCK(tp->t_lock, plstr);
						if (tp->t_message)
							tp->t_state |= TS_DSUSP;
						else {
							tbp = allocb(4, BPRI_MED);
							if (tbp) {
								UNLOCK(tp->t_lock, ipl);
								*tbp->b_wptr++ = c;
								putnext(q, tbp);
								(void) putnextctl1(q, M_SIG, SIGTSTP);
								ipl = LOCK(tp->t_lock, plstr);
							}
							tbp = NULL;
						}
						if ((tp->t_echomp = allocb(4, BPRI_HI)) != NULL) {
							(void) ldterm_echo(c, WR(q), 4, tp);
							/* using mp lets us avoid an extra lock */
							tbp = tp->t_echomp;
							tp->t_echomp = NULL;
						}
						UNLOCK(tp->t_lock, ipl);
						if (tbp)
							putnext(WR(q), tbp);
						continue;
					}
				}

				/*
				 * Throw away CR if IGNCR set, or turn
				 * it into NL if ICRNL set.
				 */
				if (c == '\r') {
					if (locmodes.c_iflag & IGNCR)
						continue;
					if (locmodes.c_iflag & ICRNL)
						c = '\n';
				} else {
					/*
					 * Turn NL into CR if INLCR set.
					 */
					if (c == '\n'
					    && locmodes.c_iflag & INLCR)
						c = '\r';
				}

				/*
				 * Map upper case input to lower case if
				 * IUCLC flag set.
				 */
				if (locmodes.c_iflag & IUCLC
				    && c >= 'A' && c <= 'Z')
					c += 'a' - 'A';

				/*
				 * Put the possibly-transformed character
				 * back in the message.
				 */
				*writep++ = c;
			}

			/*
			 * If we didn't copy some characters because
			 * we were ignoring them, fix the size of the
			 * data block by adjusting the write pointer.
			 */
			bp->b_wptr -= (readp - writep);
		} else {
			/*
			 * We won't be doing anything other than possibly
			 * stripping the input.
			 */
			if (locmodes.c_iflag & ISTRIP) {
				while (readp < bp->b_wptr)
					*writep++ = *readp++ & 0177;
			}
			ipl = LOCK(tp->t_lock, plstr);
			tp->t_modes.c_lflag &= ~FLUSHO;
			UNLOCK(tp->t_lock, ipl);
		}

	} while ((bp = bp->b_cont) != NULL);	/* next block, if any */


	/*
	 * Flow control: send "stop input" message if our queue is
	 * approaching its high-water mark. The message will be dropped
	 * on the floor in the service procedure, if we cannot ship it
	 * up and we have had it upto our neck!
	 * 
	 */
	ipl = freezestr(q);
	(void) strqget(q, QCOUNT, 0, &count);
	unfreezestr(q, ipl);
	ipl = LOCK(tp->t_lock, plstr);
	if ((locmodes.c_iflag & IXOFF) && !(tp->t_state & TS_TBLOCK)
		    && count >= TTXOHI) {
		tp->t_state |= TS_TBLOCK;
		UNLOCK(tp->t_lock, ipl);
		(void) putnextctl(wrq, M_STOPI);
		DEBUG1 (("M_STOPI down\n"));
	}
	else
		UNLOCK(tp->t_lock, ipl);

	/*
	 * Queue the message for service procedure.
	 */

	putq(q, mp);
	return(0);
}

/*
 * int
 * ldtermrsrv(queue_t *q)
 *	Read side service procedure.  Handles canonical and non-canonical data
 *	processing (e.g. erase/kill and escape ('\') processing, gathering into
 *	messages, upper/lower case input mapping).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.  On return, message has either been
 *	processed or requeued because of flow control.
 */

STATIC	int
ldtermrsrv(queue_t *q)
{
	lstate_t *tp;
	mblk_t *mp;
	mblk_t *bp;
	mblk_t *bpt;
	mblk_t *bcont;
	struct iocblk *iocp;
	int ebsize;
	long count;
	pl_t ipl;

	tp = (lstate_t *)q->q_ptr;

	ipl = LOCK(tp->t_lock, plstr);
	if (tp->t_state & TS_RESCAN) {
		/*
		 * Canonicalization was turned on or off.
		 * Put the message being assembled back in the input queue,
		 * so that we rescan it.
		 */
		if (tp->t_message != NULL) {
			DEBUG5 (("RESCAN WAS SET; put back in q\n"));
			putbq(q, tp->t_message);
			tp->t_message = NULL;
			tp->t_endmsg = NULL;
			tp->t_msglen = 0;
		}
		tp->t_state &= ~TS_RESCAN;
	}
	UNLOCK(tp->t_lock, ipl);

	bpt = NULL;

	while ((mp = getq(q)) != NULL) {
		if (mp->b_datap->db_type <= QPCTL && !canputnext(q)) {
			putbq(q, mp);
			goto out;	/* read side is blocked */
		}
		switch (mp->b_datap->db_type) {

		default:
			putnext(q, mp);	/* pass it on */
			continue;

		case M_FLUSH:
			if (!(*mp->b_rptr & FLUSHR)) {
				/* not for us, pass it on */
				putnext(q, mp);
				continue;
			}
			/*
			 * Flush everything we haven't looked at yet.
			 */
			flushq(q, FLUSHDATA);

			/*
			 * Flush everything we have looked at.
			 */
			ipl = LOCK(tp->t_lock, plstr);
			freemsg(tp->t_message);
			tp->t_message = NULL;
			tp->t_endmsg = NULL;
			tp->t_msglen = 0;
			tp->t_rocount = 0;
			tp->t_rocol = 0;
			if (tp->t_state & TS_MEUC) {	/* EUC multi-byte */
				ASSERT(tp->t_eucp_mp);
				tp->t_eucp = tp->t_eucp_mp->b_rptr;
			}
			UNLOCK(tp->t_lock, ipl);
			putnext(q, mp);		/* pass it on */
			continue;

		case M_HANGUP:
			/*
			 * Flush everything we haven't looked at yet.
			 */
			flushq(q, FLUSHDATA);

			/*
			 * Flush everything we have looked at.
			 */
			ipl = LOCK(tp->t_lock, plstr);
			freemsg(tp->t_message);
			tp->t_message = NULL;
			tp->t_endmsg = NULL;
			tp->t_msglen = 0;
		/* should we set read request tp->t_rd_request to NULL? **/
			tp->t_rocount = 0;	/* if it hasn't been typed, */
			tp->t_rocol = 0;	/* it hasn't been echoed :-) */
			if (tp->t_state & TS_MEUC) {
				ASSERT(tp->t_eucp_mp);
				tp->t_eucp = tp->t_eucp_mp->b_rptr;
			}
			/*
			 * Restart output, since it's probably got 
			 * nowhere to go anyway, and we're probably not
			 * going to see another ^Q for a while.
			 */
			if (tp->t_state & TS_TTSTOP) {
				tp->t_state &= ~TS_TTSTOP;
				UNLOCK(tp->t_lock, ipl);
				(void) putnextctl(WR(q), M_START);
			}
			else
				UNLOCK(tp->t_lock, ipl);

			/*
			 * This message will travel up the read queue, flushing
			 * as it goes, get turned around at the stream head,
			 * and travel back down the write queue, flushing as
			 * it goes.
			 */
			(void) putnextctl1(q, M_FLUSH, FLUSHW);

			/*
			 * This message will travel down the write queue, flushing
			 * as it goes, get turned around at the driver,
			 * and travel back up the read queue, flushing as
			 * it goes.
			 */
			(void) putctl1(WR(q), M_FLUSH, FLUSHR);
			(void) putnext(q, mp);
			continue;

		case M_IOCACK:

			/*
			 * Augment whatever information the driver is
			 * returning  with the information we supply.
			 */
			ldterm_ioctl_reply(q, mp);
			continue;

		case M_DATA:
			break;
		}

		/*
		 * This is an M_DATA message.
		 */

		/* do XENIX channel mapping first, if set */
		if (LDTERM_CHANMAP(tp)) {
			chanmap_data(q, mp, tp);
		}

		/*
		 * If somebody below us ("intelligent" communications board,
		 * pseudo-tty controlled by an editor) is doing
		 * canonicalization, don't scan it for special characters.
		 */
		ipl = LOCK(tp->t_lock, plstr);
		if (tp->t_state & TS_NOCANON) {
			UNLOCK(tp->t_lock, ipl);
			putnext(q, mp);
			continue;
		}
		else
			UNLOCK(tp->t_lock, ipl);

		bp = mp;

		ebsize = bp->b_wptr - bp->b_rptr;
		if (ebsize > EBSIZE)
			ebsize = EBSIZE;
		/*
	 	 * ldterm_vmin_timeout may interrupt this code
		 * and  set t_endmsg to zero during noncanonical 
		 * processing.
		 */

		ipl = LOCK(tp->t_lock, plstr);
		if ((bpt = newmsg(tp)) != NULL) {
			do {
				bcont = bp->b_cont;
				if (CANON_MODE) {
					UNLOCK(tp->t_lock, ipl);
					bpt = ldterm_docanon(bp, bpt, ebsize, 
						q, tp);
					/* Release this block. */
					freeb(bp);
					/* Reacquire lock for next iteration */
					ipl = LOCK(tp->t_lock, plstr);
				} else {
					bpt = ldterm_dononcanon(bp, bpt, ebsize,
					    q, tp);
				}
				if (bpt == NULL) {
					/*
					 *+ Kernel could not allocate memory for
					 *+ a streams message.  This indicates
					 *+ that there is not enough physical
					 *+ memory on the machine or that memory
					 *+ is being lost by the kernel.
					 */
					cmn_err(CE_WARN, "ldtermrsrv: out of blocks\n");
					freemsg(bcont);
					break;
				}
			} while ((bp = bcont) != NULL);
		}

		/*
		 * Send whatever we echoed downstream.
		 */
		if (tp->t_echomp != NULL) {
			/* about to reuse mp at top of loop, this allows
			 * one less lock call */
			mp = tp->t_echomp;
			tp->t_echomp = NULL;
			UNLOCK(tp->t_lock, ipl);
			putnext(WR(q), mp);
		}
		else
			UNLOCK(tp->t_lock, ipl);
	}

out:
	/*
	 * Flow control: send start message if blocked and
	 * our queue is below its low water mark and we haven't
	 * done a tclow with TCIOFF.
	 */
	ipl = freezestr(q);
	(void) strqget(q, QCOUNT, 0, &count);
	unfreezestr(q, ipl);
	ipl = LOCK(tp->t_lock, plstr);
	if ((tp->t_modes.c_iflag & IXOFF) && (tp->t_state & TS_TBLOCK)
	    && count <= TTXOLO && !(tp->t_state & TS_FLOW) ) {
		tp->t_state &= ~TS_TBLOCK;
		UNLOCK(tp->t_lock, ipl);
		(void) putctl(WR(q), M_STARTI);
	}
	else
		UNLOCK(tp->t_lock, ipl);
	return(0);
}

/*
 * mblk_t *
 * ldterm_docanon(mblk_t *bp, mblk_t *bpt, int ebsize, queue_t *q, lstate_t *tp)
 *	Do canonical processing on the characters in bp.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.  On return, characters have been 
 * 	processed and placed into the "canonicalized" message.
 */

STATIC mblk_t *
ldterm_docanon(mblk_t *bp, mblk_t *bpt, int ebsize, queue_t *q, lstate_t *tp)
{
	queue_t *wrq;
	uchar_t c;
	int i;
	pl_t ipl;

	wrq = WR(q);
	while ((bp->b_rptr < bp->b_wptr) && (bpt != NULL)) {
		c = *bp->b_rptr++;
		ipl = LOCK(tp->t_lock, plstr);
		/*
		 * If the previous character was the "literal next" character,
		 * treat this character as regular input.
		 */
		if (tp->t_state & TS_SLNCH) {
			goto escaped;
		}

		/*
		 * Setting a special character to NUL disables it, so if this
		 * character is NUL, it should not be compared with any of the
		 * special characters.
		 */
		if (c == '\0') {
			tp->t_state &= ~TS_QUOT;
			goto escaped;
		}

		/*
		 * If this character is the literal next character, 
		 * echo it as '^', backspace over it, and record that fact.
		 */
		if ((tp->t_modes.c_lflag & IEXTEN) && 
		    (c == tp->t_modes.c_cc[VLNEXT])) {
			if (tp->t_modes.c_lflag & ECHO)
				ldterm_outstring((uchar_t *)"^\b", 2, 
					wrq, ebsize, tp);
			tp->t_state |= TS_SLNCH;
			UNLOCK(tp->t_lock, ipl);
			continue;
		}

		/*
		 * Check for the editing characters.
		 * EUC: we can't use "codeset" because that may change
		 * around on us.  Just look at the value of the end byte
		 * in the canonical buffer (it's in t_endmsg->wptr-1).
		 * That provides a clue to what we're doing;
		 * if that's got the high bit set, then we're
		 * in business - do an EUC character erase.
		 */
		if (c == tp->t_modes.c_cc[VERASE]) {
			if (tp->t_state & TS_QUOT) {
				/*
				 * Get rid of the backslash, and put the erase
				 * character in its place.
				 */
				ldterm_erase(wrq, ebsize, tp);
				bpt = tp->t_endmsg;
				goto escaped;
			} else {
				if ((tp->t_state & TS_MEUC) &&
				     NOTASCII(*(tp->t_endmsg->b_wptr - 1)))
					ldterm_euc_erase(wrq, ebsize, tp);
				else
					ldterm_erase(wrq, ebsize, tp);
				bpt = tp->t_endmsg;
				UNLOCK(tp->t_lock, ipl);
				continue;
			}
		}

		if ((tp->t_modes.c_lflag & IEXTEN) && 
		    (c == tp->t_modes.c_cc[VWERASE])) {
			/*
			 * Do "ASCII word" or "EUC token" erase.
			 */
			if (tp->t_state & TS_MEUC)
				ldterm_tokerase(wrq, ebsize, tp);
			else
				ldterm_werase(wrq, ebsize, tp);
			bpt = tp->t_endmsg;
			UNLOCK(tp->t_lock, ipl);
			continue;
		}

		if (c == tp->t_modes.c_cc[VKILL]) {
			if (tp->t_state & TS_QUOT) {
				/*
				 * Get rid of the backslash, and 
				 * put the kill character in its place.
				 */
				ldterm_erase(wrq, ebsize, tp);
				bpt = tp->t_endmsg;
				goto escaped;
			} else {
				ldterm_kill(wrq, ebsize, tp);
				bpt = tp->t_endmsg;
				UNLOCK(tp->t_lock, ipl);
				continue;
			}
		}

		if ((tp->t_modes.c_lflag & IEXTEN) && 
		    (c == tp->t_modes.c_cc[VREPRINT])) {
			ldterm_reprint(wrq, ebsize, tp);
			UNLOCK(tp->t_lock, ipl);
			continue;
		}

		/*
		 * If the preceding character was a backslash:
		 *     if the current character is an EOF, get rid of 
		 *     the backslash and treat the EOF as data;
		 *     if we're in XCASE mode and the current character is part
		 *     of a backslash-X escape sequence, process it;
		 *     otherwise, just treat the current character normally.
		 */
		if (tp->t_state & TS_QUOT) {
			tp->t_state &= ~TS_QUOT;
			if (c == tp->t_modes.c_cc[VEOF]) {
				/*
				 * EOF character.
				 * Since it's escaped, get rid of the backslash
				 * and put the EOF character in its place.
				 */
				ldterm_erase(wrq, ebsize, tp);
				bpt = tp->t_endmsg;
			} else {
				/*
				 * If we're in XCASE mode, and the current
				 * character is part of a backslash-X sequence,
				 * get rid of the backslash and replace the
				 * current character with what that sequence
				 * maps to.
				 */
				if ((tp->t_modes.c_lflag & XCASE)
				    && imaptab[c] != '\0') {
					ldterm_erase(wrq, ebsize, tp);
					bpt = tp->t_endmsg;
					c = imaptab[c];
				}
			}
		} else {
			/*
			 * Previous character wasn't backslash; 
			 * check whether this was the EOF character.
			 */
			if (c == tp->t_modes.c_cc[VEOF]) {
				/*
				 * EOF character.
				 * Don't echo it unless ECHOCTL is set, 
				 * don't stuff it in the current line, 
				 * but send the line up the stream.
				 */
				if ((tp->t_modes.c_lflag & ECHOCTL)
				    && (tp->t_modes.c_lflag & IEXTEN)
				    && (tp->t_modes.c_lflag & ECHO)) {
					i = ldterm_echo(c, wrq, ebsize, tp);
					while (i > 0) {
						ldterm_outchar('\b', wrq, 
							ebsize, tp);
						i--;
					}
				}
				bpt->b_datap->db_type = M_DATA;
				ldterm_msg_upstream(q, tp);
				bpt = newmsg(tp);
				UNLOCK(tp->t_lock, ipl);
				continue;
			}
		}

escaped:
		/*
		 * First, make sure we can fit one WHOLE EUC char in the buffer.
		 *  This is one place where we have overhead even if not 
		 * in multi-byte mode; the overhead is subtracting 
		 * tp->t_maxeuc from MAX_CANON before checking.
		 *
		 * Allows 256 bytes in the buffer before throwing awaying the
		 * the overflow of characters.
		 *
		 * lock held at this point
		 */
		if (tp->t_msglen > ((MAX_CANON + 1) - (int)tp->t_maxeuc)) {
			/*
			 * Byte will cause line to overflow, or the next EUC
			 * won't fit:
			 * Ring the bell or discard all input, and 
			 * don't save the byte away.
			 */
			if (tp->t_modes.c_iflag & IMAXBEL) {
				ldterm_outchar(CTRL('g'), wrq, ebsize, tp);
				UNLOCK(tp->t_lock, ipl);
				continue;
			} else {
				/* MAX_CANON processing.
				 * free everything in the current line and 
				 * start with the current character as the
				 * first character.
				 */
				DEBUG7(("ldterm_docanon: MAX_CANON processing\n"));
				freemsg(tp->t_message);
				tp->t_message = NULL;
				tp->t_endmsg = NULL;
				tp->t_msglen = 0;
				tp->t_rocount = 0; /* if it hasn't been typed */
				tp->t_rocol = 0;   /* it hasn't been echoed   */
				if (tp->t_state & TS_MEUC) {
					ASSERT(tp->t_eucp_mp);
					tp->t_eucp = tp->t_eucp_mp->b_rptr;
				}
				tp->t_state &= ~TS_SLNCH;
				if ((bpt = newmsg(tp)) == NULL) {
					UNLOCK(tp->t_lock, ipl);
					break;
				}
			}
		}

		/*
		 * Add the character to the current line.
		 */
		if (bpt->b_wptr >= bpt->b_datap->db_lim) {
			/*
			 * No more room in this mblk; save this one away, and
			 * allocate a new one.
			 */
			bpt->b_datap->db_type = M_DATA;
			if ((bpt = allocb(IBSIZE, BPRI_MED)) == NULL) {
				UNLOCK(tp->t_lock, ipl);
				break;
			}

			/*
			 * Chain the new one to the end of the old one, and
			 * mark it as the last block in the current line.
			 */
			tp->t_endmsg->b_cont = bpt;
			tp->t_endmsg = bpt;
		}
		*bpt->b_wptr++ = c;
		tp->t_msglen++; /* message length in BYTES */

		/*
		 * In multi-byte mode, we have to keep track of where we are.
		 * The first bytes of EUC chars get the full count for the
		 * whole character.  We don't do any column calculations here,
		 * but we need the information for when we do.
		 * We could come across cases where we are getting garbage
		 * on the line, but we're in multi-byte mode.  In that case,
		 * we may see ASCII come in the middle of what should have
		 * been an EUC character. Call ldterm_eucwarn...eventually,
		 * a warning message will be printed about it.
		 */
		if (tp->t_state & TS_MEUC) {
			/* if in a multi-byte char already */
			if (tp->t_eucleft) {
				--tp->t_eucleft;
				*tp->t_eucp++ = 0;   /* is a subsequent byte */
				if (ISASCII(c))
					ldterm_eucwarn(tp);
			}
			else {	/* is the first byte of an EUC, or is ASCII */
				if (ISASCII(c)) {
					*tp->t_eucp++ = ldterm_dispwidth(c, &tp->eucwioc, tp->t_modes.c_lflag & ECHOCTL);
				}
				else {
					*tp->t_eucp = ldterm_dispwidth(c, &tp->eucwioc, tp->t_modes.c_lflag & ECHOCTL);
					tp->t_eucleft = ldterm_memwidth(c, &tp->eucwioc) - 1;
					++(tp->t_eucp);
				}
			}
		}

		/* 
		 * EOL2/XCASE should be conditioned with IEXTEN to be truly
		 * POSIX conformant.
		 * This is going to cause problems for pre-SVR4.0 programs
		 * that don't know about IEXTEN. Hence EOL2/IEXTEN is not
		 *  conditioned with IEXTEN.
		 */
		if (!(tp->t_state & TS_SLNCH)
		    && (c == '\n' || (c != '\0' && (c == tp->t_modes.c_cc[VEOL]
		   || (c == tp->t_modes.c_cc[VEOL2]))))) {
			/*
			 * It's a line-termination character; send the line
			 * up the stream.
			 */
			bpt->b_datap->db_type = M_DATA;
			ldterm_msg_upstream(q, tp);
			if (tp->t_state & TS_MEUC) {
				ASSERT(tp->t_eucp_mp);
				tp->t_eucp = tp->t_eucp_mp->b_rptr;
			}
			if ((bpt = newmsg(tp)) == NULL) {
				UNLOCK(tp->t_lock, ipl);
				break;
			}
		} else {
			/*
			 * Character was escaped with LNEXT.
			 */
			if (tp->t_rocount++ == 0)
				tp->t_rocol = tp->t_col;
			tp->t_state &= ~(TS_SLNCH|TS_QUOT);
			if (c == '\\')
				tp->t_state |= TS_QUOT;
		}

		/*
		 * Echo it.
		 */
		if (tp->t_state & TS_ERASE) {
			tp->t_state &= ~TS_ERASE;
			if (tp->t_modes.c_lflag & ECHO)
				ldterm_outchar('/', wrq, ebsize, tp);
		}

		if (tp->t_modes.c_lflag & ECHO)
			(void) ldterm_echo(c, wrq, ebsize, tp);
		else {
			/*
			 * Echo NL when ECHO turned off, if ECHONL flag is set.
			 */
			if (c == '\n' && (tp->t_modes.c_lflag & ECHONL))
				ldterm_outchar(c, wrq, ebsize, tp);
		}
		UNLOCK(tp->t_lock, ipl);
	} /* end while */
	return (bpt);
}

/*
 * int
 * ldterm_unget(lstate_t *tp)
 *	Unget a character
 *
 * Calling/Exit State:
 *	Called with t_lock held.  On return, last character is available as
 *	input.
 */

STATIC
ldterm_unget(lstate_t *tp)
{
	mblk_t *bpt;

	if ((bpt = tp->t_endmsg) == NULL)
		return(-1);	/* no buffers */
	if (bpt->b_rptr == bpt->b_wptr)
		return(-1);	/* zero-length record */
	tp->t_msglen--;	/* one fewer character */
	return(*--bpt->b_wptr);
}

/*
 * void
 * ldterm_trim(lstate_t *tp)
 *	Trim empty message block off end of current message.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC void
ldterm_trim(lstate_t *tp)
{
	mblk_t *bpt;
	mblk_t *bp;

	ASSERT(tp->t_endmsg);
	bpt = tp->t_endmsg;

	if (bpt->b_rptr == bpt->b_wptr) {
		/*
		 * This mblk is now empty.
		 * Find the previous mblk; throw this one away, unless
		 * it's the first one.
		 */
		bp = tp->t_message;
		if (bp != bpt) {
			while (bp->b_cont != bpt) {
				ASSERT(bp->b_cont);
				bp = bp->b_cont;
			}
			bp->b_cont = NULL;
			freeb(bpt);
			tp->t_endmsg = bp;	/* point to that mblk */
		}
	}
}


/*
 * void
 * ldterm_rubout(uchar_t c, queue_t *q, int ebsize, lstate_t *tp)
 *	Rubout one character from the current line being built for tp
 *	as cleanly as possible.  q is the write queue for tp.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 *
 * Description:
 *	Most of this can't be applied to multi-byte processing.  We do our
 *	own thing for that... See the "ldterm_eucerase" routine.  We never
 *	call ldterm_rubout on a multi-byte or multi-column character.
 */

STATIC void
ldterm_rubout(uchar_t c, queue_t *q, int ebsize, lstate_t *tp)
{
	int tabcols;
	static uchar_t crtrubout[] = "\b \b\b \b";
#define	RUBOUT1	&crtrubout[3]	/* rub out one position */
#define	RUBOUT2	&crtrubout[0]	/* rub out two positions */

	if (!(tp->t_modes.c_lflag & ECHO))
		return;
	if (tp->t_modes.c_lflag & ECHOE) {
		/*
		 * "CRT rubout"; try erasing it from the screen.
		 */
		if (tp->t_rocount == 0) {
			/*
			 * After the character being erased was echoed,
			 * some data was written to the terminal; we
			 * can't erase it cleanly, so we just reprint the
			 * whole line as if the user had typed the
			 * reprint character.
			 */
			ldterm_reprint(q, ebsize, tp);
			return;
		} else {
			switch (typetab[c]) {

			case ORDINARY:
				if ((tp->t_modes.c_lflag & XCASE) && omaptab[c])
					ldterm_outstring(RUBOUT1, 3, q, ebsize,
					    tp);
				ldterm_outstring(RUBOUT1, 3, q, ebsize, tp);
				break;

			case VTAB:
			case BACKSPACE:
			case CONTROL:
			case RETURN:
			case NEWLINE:
				if ((tp->t_modes.c_lflag & ECHOCTL)
				    && (tp->t_modes.c_lflag & IEXTEN))
					ldterm_outstring(RUBOUT2, 6, q, ebsize,
					    tp);
				break;

			case TAB:
				if (tp->t_rocount < tp->t_msglen) {
					/*
					 * While the tab being erased was
					 * expanded, some data was written
					 * to the terminal; we can't erase it
					 * cleanly, so we just reprint the
					 * whole line as if the user had typed
					 * the reprint character.
					 */
					ldterm_reprint(q, ebsize, tp);
					return;
				}
				tabcols = ldterm_tabcols(tp);
				while (--tabcols >= 0)
					ldterm_outchar('\b', q, ebsize, tp);
				break;
			}
		}
	} else if ((tp->t_modes.c_lflag & ECHOPRT)
		    && (tp->t_modes.c_lflag & IEXTEN)) {
		/*
		 * "Printing rubout"; echo it between \ and /.
		 */
		if (!(tp->t_state & TS_ERASE)) {
			ldterm_outchar('\\', q, ebsize, tp);
			tp->t_state |= TS_ERASE;
		}
		(void) ldterm_echo(c, q, ebsize, tp);
	} else
		(void) ldterm_echo(tp->t_modes.c_cc[VERASE], q, ebsize, tp);
	tp->t_rocount--;	/* we "unechoed" this character */
}

/*
 * int
 * ldterm_tabcols(lstate_t *tp)
 *	Find the number of characters the tab we just deleted took up by
 *	zipping through the current line and recomputing the column number.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC
ldterm_tabcols(lstate_t *tp)
{
	int col;
	mblk_t *bp;
	uchar_t *readp;
	uchar_t *endp;
	uchar_t c;

	col = tp->t_rocol;
	/*
	 * If we're doing multi-byte stuff, zip through the list of
	 * widths to figure out where we are (we've kept track).
	 */
	if (tp->t_state & TS_MEUC) {
		ASSERT(tp->t_eucp_mp);
		readp = tp->t_eucp_mp->b_rptr;
		endp = tp->t_eucp;
		endp--;
		while (readp < endp) {
			switch (*readp) {
			case EUC_TWIDTH:	/* it's a tab */
				col |= 07;	/* bump up */
				col++;
				break;
			case EUC_BSWIDTH:	/* backspace */
				if (col)
					col--;
				break;
			case EUC_NLWIDTH:	/* newline */
				if ((tp->t_modes.c_lflag & ECHOCTL)
				    && (tp->t_modes.c_lflag & IEXTEN))
					col += 2;
				else if (tp->t_modes.c_oflag & ONLRET)
					col = 0;
				break;
			case EUC_CRWIDTH:	/* return */
				col = 0;
				break;
			default:
				col += *readp;
			}
			++readp;
		}
		goto eucout;	/* finished! */
	}
	bp = tp->t_message;
	do {
		readp = bp->b_rptr;
		while (readp < bp->b_wptr) {
			c = *readp++;
			if ((tp->t_modes.c_lflag & ECHOCTL)
			    && (tp->t_modes.c_lflag & IEXTEN)) {
				if (c <= 037 && c != '\t' && c != '\n'
				    || c == 0177) {
					col++;
					continue;
				}
			}

			/*
			 * Column position calculated here.
			 */
			switch (typetab[c]) {

			/* Ordinary characters; advance by one. */
			case ORDINARY:
				col++;
				break;

			/* Non-printing characters; nothing happens. */
			case CONTROL:
				break;

			/* Backspace */
			case BACKSPACE:
				if (col != 0)
					col--;
				break;

			/* Newline; column depends on flags. */
			case NEWLINE:
				if (tp->t_modes.c_oflag & ONLRET)
					col = 0;
				break;

			/* tab */
			case TAB:
				col |= 07;
				col++;
				break;

			/* vertical motion */
			case VTAB:
				break;

			/* carriage return */
			case RETURN:
				col = 0;
				break;
			}
		}
	} while ((bp = bp->b_cont) != NULL);	/* next block, if any */

	/*
	 * "col" is now the column number before the tab.
	 * "tp->t_col" is still the column number after the tab,
	 * since we haven't erased the tab yet.
	 * Thus "tp->t_col - col" is the number of positions the tab
	 * moved.
	 */
eucout:
	col = tp->t_col - col;
	if (col > 8)
		col = 8;		/* overflow screw */
	return (col);
}

/*
 * void
 * ldterm_erase(queue_t *q, int ebsize, lstate_t *tp)
 *	Erase a single character; We ONLY ONLY deal with ASCII or single-column
 *	single-byte EUC.  For multi-byte characters, see "ldterm_euc_erase".
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC void
ldterm_erase(queue_t *q, int ebsize, lstate_t *tp)
{
	int c;

	if ((c = ldterm_unget(tp)) != -1) {
		ldterm_rubout((uchar_t)c, q, ebsize, tp);
		ldterm_trim(tp);
		if (tp->t_state & TS_MEUC)
			--tp->t_eucp;
	}
}

/*
 * void
 * ldterm_werase(queue_t *q, int ebsize, lstate_t *tp)
 *	Erase an entire word, single-byte EUC only.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC void
ldterm_werase(queue_t *q, int ebsize, lstate_t *tp)
{
	int c;

	/*
	 * Erase trailing white space, if any.
	 */
	while ((c = ldterm_unget(tp)) == ' ' || c == '\t') {
		ldterm_rubout((uchar_t)c, q, ebsize, tp);
		ldterm_trim(tp);
	}

	/*
	 * Erase non-white-space characters, if any.
	 */
	while (c != -1 && c != ' ' && c != '\t') {
		ldterm_rubout((uchar_t)c, q, ebsize, tp);
		ldterm_trim(tp);
		c = ldterm_unget(tp);
	}
	if (c != -1) {
		/*
		 * We removed one too many characters; put the last one
		 * back.
		 */
		tp->t_endmsg->b_wptr++;	/* put 'c' back */
		tp->t_msglen++;
	}
}

/*
 * void
 * ldterm_tokerase(queue_t *q, int ebsize, lstate_t *tp)
 *	This is EUC equivalent of "word erase".  "Word erase"
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 *
 * Description:
 *	"Word erase" *	only makes sense in languages which space between
 *	words, and it's presumptuous for us to attempt "word erase" when we
 *	don't know anything *	about what's really going on.  It makes no
 *	sense for many languages, as *	the criteria for defining words and
 *	tokens may be completely different.
 * 
 *	In the TS_MEUC case (which is how we got here), we define a token to be
 *	space- or tab-delimited, and erase one of them.  It helps to have this
 *	for command lines, but it's otherwise useless for text editing
 *	applications; you need more sophistication than we can provide here.
 */

STATIC void
ldterm_tokerase(queue_t *q, int ebsize, lstate_t *tp)
{
	int c;
	int i;
	unchar *ip;

	/*
	 * ip points to the width of the actual bytes.  t_eucp points
	 * one byte beyond, where the next thing will be inserted.
	 */
	ip = tp->t_eucp - 1;
	/*
	 * Erase trailing white space, if any.
	 */
	while ((c = ldterm_unget(tp)) == ' ' || c == '\t') {
		ldterm_rubout((uchar_t)c, q, ebsize, tp);
		ldterm_trim(tp);
		tp->t_eucp--;
		--ip;
	}

	/*
	 * Erase non-white-space characters, if any.  The outer loop
	 * bops through each byte in the buffer.  EUC is removed, as is
	 * ASCII, one byte at a time. The inner loop (for) is only executed
	 * for first bytes of EUC.  The inner loop erases the number of
	 * columns required for the EUC char.  We check for ASCII first, and
	 * ldterm_rubout knows about ASCII.  We DON'T check for special values
	 * such as EUC_TWIDTH and friends.
	 */
	while (c != -1 && c != ' ' && c != '\t') {
		if (ISASCII(c))
			ldterm_rubout((uchar_t)c, q, ebsize, tp);
		else if (*ip) {
			/*
			 * erase for number of columns required for this EUC
			 * character.  Hopefully, matches ldterm_dispwidth!
			 */
			for (i = 0; i < (int) *ip; i++)
				ldterm_rubout(' ', q, ebsize, tp);
		}
		ldterm_trim(tp);
		tp->t_eucp--;
		--ip;
		c = ldterm_unget(tp);
	}
	if (c != -1) {
		/*
		 * We removed one too many characters; put the last one
		 * back.
		 */
		tp->t_endmsg->b_wptr++;	/* put 'c' back */
		tp->t_msglen++;
	}
}

/*
 * void
 * ldterm_kill(queue_t *q, int ebsize, lstate_t *tp)
 *	Kill an entire line, erasing each character one-by-one (if ECHOKE
 *	is set) or just echoing the kill character, followed by a newline
 *	(if ECHOK is set).  Multi-byte processing is included here.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC void
ldterm_kill(queue_t *q, int ebsize, lstate_t *tp)
{
	int c;
	int i;
	unchar *ip;

	if ((tp->t_modes.c_lflag & ECHOKE)
	    && (tp->t_modes.c_lflag & IEXTEN)
	    && (tp->t_msglen == tp->t_rocount)) {
		if (tp->t_state & TS_MEUC) {
			ip = tp->t_eucp - 1;
			/*
			 * This loop similar to "tokerase" above.
			 */
			while ((c = ldterm_unget(tp)) != (-1)) {
				if (ISASCII(c))
					ldterm_rubout((uchar_t)c, q, ebsize, tp);
				else if (*ip) {
					for (i = 0; i < (int) *ip; i++)
						ldterm_rubout(' ', q, ebsize, tp);
				}
				ldterm_trim(tp);
				tp->t_eucp--;
				--ip;
			}
		}
		else {
			while ((c = ldterm_unget(tp)) != -1) {
				ldterm_rubout((uchar_t)c, q, ebsize, tp);
				ldterm_trim(tp);
			}
		}
	} else {
		(void) ldterm_echo(tp->t_modes.c_cc[VKILL], q, ebsize, tp);
		if (tp->t_modes.c_lflag & ECHOK)
			(void) ldterm_outchar('\n', q, ebsize, tp);
		while (ldterm_unget(tp) != -1)
			ldterm_trim(tp);
		tp->t_rocount = 0;
		if (tp->t_state & TS_MEUC)
			tp->t_eucp = tp->t_eucp_mp->b_rptr;
	}
	tp->t_state &= ~(TS_QUOT|TS_ERASE|TS_SLNCH);
}

/*
 * void
 * ldterm_reprint(queue_t *q, int ebsize, lstate_t *tp)
 *	Reprint the current input line.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 *
 * Description:
 *	We assume c_cc has already been checked.
 */

STATIC void
ldterm_reprint(queue_t *q, int ebsize, lstate_t *tp)
{
	mblk_t *bp;
	uchar_t *readp;

	if (tp->t_modes.c_cc[VREPRINT] != (uchar_t)0)
		(void) ldterm_echo(tp->t_modes.c_cc[VREPRINT], q, ebsize, tp);
	ldterm_outchar('\n', q, ebsize, tp);

	bp = tp->t_message;
	do {
		readp = bp->b_rptr;
		while (readp < bp->b_wptr)
			(void) ldterm_echo(*readp++, q, ebsize, tp);
	} while ((bp = bp->b_cont) != NULL);	/* next block, if any */

	tp->t_state &= ~TS_ERASE;
	tp->t_rocount = tp->t_msglen;	/* we reechoed the entire line */
	tp->t_rocol = 0;
}

/*
 * mblk_t *
 * ldterm_dononcanon(mblk_t *bp, mblk_t *bpt, int ebsize, queue_t *q lstate_t *tp)
 *	Non canonical processing.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC mblk_t *
ldterm_dononcanon(mblk_t *bp, mblk_t *bpt, int ebsize, queue_t *q, lstate_t *tp)
{
	queue_t *wrq;
	uchar_t *rptr;
	int bytes_in_bp;
	int roomleft;
	int bytes_to_move;
	uchar_t c;
	mblk_t *mp;
#ifdef DEBUG
	int free_flag;
#endif


	wrq = WR(q);
#ifdef DEBUG
	free_flag = 0;
#endif
	bytes_in_bp = bp->b_wptr - bp->b_rptr;
	rptr = bp->b_rptr;
	while (bytes_in_bp != 0) {
		roomleft = bpt->b_datap->db_lim - bpt->b_wptr;
		if (roomleft == 0) {
			/*
			 * No more room in this mblk; save this one
			 * away, and allocate a new one.
			 */
			if ((bpt = allocb(IBSIZE, BPRI_MED)) == NULL) {
				freeb(bp);
				DEBUG4 (("ldterm_do_noncanon: allcob failed\n"));
				return (bpt);
			}

			/*
			 * Chain the new one to the end of the old
			 * one, and mark it as the last block in the
			 * current lump.
			 */
			tp->t_endmsg->b_cont = bpt;
			tp->t_endmsg = bpt;
			roomleft = IBSIZE;
		}
DEBUG5(("roomleft=%d, bytes_in_bp=%d, tp->t_rd_request=%d\n",roomleft, bytes_in_bp, tp->t_rd_request));
		/* if there is a read pending before this data got here
		 * move bytes according to the minimum of room left in
		 * this buffer, bytes in the message and byte count
		 * requested in the read. If there is no read pending,
		 * move the minimum of the first two 
		 */
		if (tp->t_rd_request == 0) 
			bytes_to_move = MIN(roomleft, bytes_in_bp);
		else
			bytes_to_move = MIN(MIN(roomleft, bytes_in_bp),tp->t_rd_request);
		/* bytes_to_move may be 0, but saves lock round trip to do it here */
		tp->t_msglen += bytes_to_move;
DEBUG5(("Bytes to move = %d\n", bytes_to_move));
		if (bytes_to_move ==0)
			break;
		bcopy((caddr_t)rptr, (caddr_t)bpt->b_wptr,
		    (uint_t)bytes_to_move);
		bpt->b_wptr += bytes_to_move;
		rptr += bytes_to_move;
		bytes_in_bp -= bytes_to_move;
	}
	/*
	 * Echo the data in this message.
	 */
	if (tp->t_modes.c_lflag & ECHO) {
		rptr = bp->b_rptr;
		while (rptr < bp->b_wptr)
			(void) ldterm_echo(*rptr++, wrq, ebsize, tp);
	} else {
		if (tp->t_modes.c_lflag & ECHONL) {
			/*
			 * Echo NL, even though ECHO is not set.
			 */
			rptr = bp->b_rptr;
			while (rptr < bp->b_wptr) {
				c = *rptr++;
				if (c == '\n')
					ldterm_outchar(c, wrq, ebsize, tp);
			}
		}
	}

	if (bytes_in_bp == 0) {
		DEBUG4 (("bytes_in_bp is zero\n"));
		freeb(bp);
	} 
#ifdef DEBUG
	else
		free_flag = 1; /* for debugging only */
#endif

	/* Sending data upstream is dictated by VMIN/VTIME. If vmin
	 * is satisfied, *AND* read request is pending at stream head,
	 * then send data up and cancel any timeouts in progress.
	 * VTIME has to be handled intelligently to eliminate
	 * unsetting callout table entries and software interrupts.
	 * when the first char arrives, if VTIME is set, timer is started.
	 * Subsequent characters only clears RTO flag (does not untimeout).
	 */

	tp->t_state &= ~TS_RTO;
	tp->t_lbolt = 0;
DEBUG4 (("Unsetting TS_RTO, msglen = %d\n", tp->t_msglen));
	if (tp->t_msglen >= (int)V_MIN) {
		DEBUG4 (("VMIN Ready\n"));
		ldterm_msg_upstream (q, tp);
	}

	else if (V_TIME) {
		DEBUG4 (("ldterm_dononcanon VTIME  set\n"));
		(void) drv_getparm(LBOLT, &tp->t_lbolt);
		/*
		 * The LBOLT value can wrap around to zero if the system stays
		 * up long enough.  If we happen to sample it at that point,
		 * pretend we got it one tick earlier, because we want to use
		 * zero as a special case.  (XXX - a better solution might be
		 * to use a t_state flag to indicate whether or not the t_lbolt
		 * value is meaningful.)
		 */
		if (tp->t_lbolt == 0)
			tp->t_lbolt--;

		if (!(tp->t_state & TS_TACT)) {
			DEBUG4 (("ldterm_dononcanon ldterm_vmin_timeout called\n"));
			ldterm_vmin_timeout_l (q);
		}
	}

#ifdef DEBUG
	if (free_flag)
		DEBUG4 (("CAUTION message block not freed\n"));
#endif

	mp = newmsg(tp);
	return (mp);
}


/*
 * int
 * ldterm_echo(uchar_t c, queue_t *q, int ebsize, lstate_t *tp)
 *	Echo a typed byte to the terminal.
 *
 * Calling/Exit State:
 *	Returns the number of bytes printed.  Bytes of EUC characters drop
 *	through the ECHOCTL stuff and are just output as themselves.
 *	Called with t_lock held
 */

STATIC int
ldterm_echo(uchar_t c, queue_t *q, int ebsize, lstate_t *tp)
{
	int i;

	if (!(tp->t_modes.c_lflag & ECHO))
		return (0);
	i = 0;

/*
 * Echo control characters (c <= 37) only if the ECHOCTRL
 * flag is set as ^X. 
 */

	if ((tp->t_modes.c_lflag & ECHOCTL)
	    && (tp->t_modes.c_lflag & IEXTEN)) {
		if (c <= 037 && c != '\t' && c != '\n') {
			ldterm_outchar('^', q, ebsize, tp);
			i++;
			if (tp->t_modes.c_oflag & OLCUC)
				c += 'a' - 1;
			else
				c += 'A' - 1;
		} else if (c == 0177) {
			ldterm_outchar('^', q, ebsize, tp);
			i++;
			c = '?';
		}
		ldterm_outchar(c, q, ebsize, tp);
		return (i + 1);
		/* echo only special control character and the Bell */
	} else if ((c > 037 && c != 0177) || c == '\t' || c == '\n' || c == '\r'
		|| (tp->t_modes.c_cflag & CSIZE) <= CS6
		|| c == '\b' || c == 007 || c == tp->t_modes.c_cc[VKILL]) {
		ldterm_outchar(c, q, ebsize, tp);
		return (i + 1);
	}
	return(0);
}

/*
 * void
 * ldterm_outchar(uchar_t c, queue_t *q, int bsize, lstate_t *tp)
 *	Put a character on the output queue.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

void
ldterm_outchar(uchar_t c, queue_t *q, int bsize, lstate_t *tp)
{
	mblk_t *curbp;

	/*
	 * Don't even look at the characters unless we
	 * have something useful to do with them.
	 */
	if ( LDTERM_CHANMAP(tp) || (tp->t_modes.c_oflag & OPOST)
	    || ((tp->t_modes.c_lflag & XCASE) && CANON_MODE)) {
		mblk_t *mp;

		if ((mp = allocb(4, BPRI_HI)) == NULL) {
			/*
			 *+ Kernel could not allocate memory for
			 *+ a streams message.  This indicates
			 *+ that there is not enough physical
			 *+ memory on the machine or that memory
			 *+ is being lost by the kernel.
			 */
			cmn_err(CE_WARN,"ldterm: (ldterm_outchar) out of blocks\n");
			return;
		}
		*mp->b_wptr++ = c;
		tp->t_echomp = ldterm_output_msg(q, mp, tp->t_echomp, tp, bsize, 1);
	} else {
		if ((curbp = tp->t_echomp) != NULL) {
			while (curbp->b_cont != NULL)
				curbp = curbp->b_cont;
			if (curbp->b_datap->db_lim == curbp->b_wptr) {
				mblk_t *newbp;

				if ((newbp = allocb(bsize, BPRI_HI)) == NULL) {
					/*
					 *+ Kernel could not allocate memory for
					 *+ a streams message.  This indicates
					 *+ that there is not enough physical
					 *+ memory on the machine or that memory
					 *+ is being lost by the kernel.
					 */
					cmn_err(CE_WARN,"ldterm: (ldterm_outchar) out of blocks\n");
					return;
				}
				curbp->b_cont = newbp;
				curbp = newbp;
			}
		} else {
			if ((curbp = allocb(bsize, BPRI_HI)) == NULL) {
				/*
				 *+ Kernel could not allocate memory for
				 *+ a streams message.  This indicates
				 *+ that there is not enough physical
				 *+ memory on the machine or that memory
				 *+ is being lost by the kernel.
				 */
				cmn_err(CE_WARN,"ldterm: (ldterm_outchar) out of blocks\n");
				return;
			}
			tp->t_echomp = curbp;
		}
		*curbp->b_wptr++ = c;
	}
}

/*
 * void
 * ldterm_outstring(uchar_t *cp, int len, queue_t *q, int bsize, lstate_t *tp)
 *	Copy a string, of length len, to the output queue.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC void
ldterm_outstring(uchar_t *cp, int len, queue_t *q, int bsize, lstate_t *tp)
{
	while (len > 0) {
		ldterm_outchar(*cp++, q, bsize, tp);
		len--;
	}
}


/*
 * mblk_t *
 * newmsg(lstate_t *tp)
 *	Allocate a new message block.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC mblk_t *
newmsg(lstate_t *tp)
{
	mblk_t *bp;

	/*
	 * If no current message, allocate a block
	 * for it.
	 */
	if ((bp = tp->t_endmsg) == NULL) {
		if ((bp = allocb(IBSIZE, BPRI_MED)) == NULL) {
			/*
			 *+ Kernel could not allocate memory for
			 *+ a streams message.  This indicates
			 *+ that there is not enough physical
			 *+ memory on the machine or that memory
			 *+ is being lost by the kernel.
			 */
			cmn_err(CE_WARN,"ldterm: (ldtermrsrv) out of blocks\n");
			return (bp);
		}
		tp->t_message = bp;
		tp->t_endmsg = bp;
	}
	return (bp);
}

/*
 * void
 * ldterm_msg_upstream(queue_t *q, lstate_t *tp)
 *	Send a message back upstream.
 *
 * Calling/Exit State:
 *	Called with t_lock held, stay at plstr throughout.
 */

STATIC void
ldterm_msg_upstream(queue_t *q, lstate_t *tp)
{
	mblk_t *bp;
	int flag;
	
	bp = tp->t_message;
	tp->t_message = NULL;
	tp->t_endmsg = NULL;
	tp->t_msglen = 0;
	tp->t_rocount = 0;
	tp->t_rd_request = 0;
	flag = tp->t_state & TS_DSUSP;
	tp->t_state &= ~TS_DSUSP;
	if (tp->t_state & TS_MEUC) {
		ASSERT(tp->t_eucp_mp);
		tp->t_eucp = tp->t_eucp_mp->b_rptr;
		/* can't reset everything, as we may have other input */
	}
	if (bp) {
		/* 
		 * update count of canch characters. 
		 */
		if (CANON_MODE) {
			drv_setparm(SYSCANC, msgdsize(bp));
		}
		UNLOCK(tp->t_lock, plstr);
		putnext(q, bp);
		if (flag)
			(void) putnextctl1(q, M_SIG, SIGTSTP);
		(void) LOCK(tp->t_lock, plstr);
	}
}

/*
 * int
 * ldtermwput(queue_t *q, mblk_t *mp)
 *	Line discipline output queue put procedure.
 *
 * Calling/Exit State:
 *	Called with no locks held.
 */

STATIC int
ldtermwput(queue_t *q, mblk_t *mp)
{
	lstate_t *tp;
	pl_t ipl;
	int tid;

	tp = (lstate_t *)q->q_ptr;

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		/*
		 * This is coming from above, so we only handle the write
		 * queue here.  If FLUSHR is set, it will get turned around
		 * at the driver, and the read procedure will see it
		 * eventually.
		 */
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		putnext(q, mp);
		break;

	case M_IOCDATA:
		if (chanmap_do_iocdata(q, mp) == 0) {
			/* msg is not processed by chanmap_do_iocdata*/
			putnext(q, mp);
		}
		break;


	case M_IOCTL:
		ldterm_do_ioctl(q, mp);
		break;

	case M_READ:
		DEBUG1 (("ldtermwput:M_READ RECEIVED\n"));
		/* Stream head needs data to satisfy timed read.
		 * Has meaning only if ICANON flag is off indicating
		 * raw mode 
		 */

		ipl = LOCK(tp->t_lock, plstr);

		/* LINTED pointer alignment */
		tp->t_rd_request = *(uint_t *) mp->b_rptr;

		if (RAW_MODE) {
			/* need to stay at plstr throughout this piece */

			if (newmsg(tp) != NULL)  {
				if (V_MIN == 0) {
					if (V_TIME == 0) /* vmin = 0,vtime = 0 */
						ldterm_msg_upstream (RD(q), tp);
					else {	/* vmin = 0, vtime > 0 */
						if (tp->t_msglen)
							ldterm_msg_upstream (RD(q), tp);
						else { /* start timer only is there is not one active */
							if (!(tp->t_state & TS_TACT)) {
								DEBUG4 (("M_READ VTIME  set and timer not active\n"));
								tp->t_state &= ~TS_RTO;
								tp->t_lbolt = 0;
								ldterm_vmin_timeout_l (RD (q));
							}
						}
					}
				} else { 		/* vmin > 0 */
					if (tp->t_msglen >= (int)V_MIN)
						ldterm_msg_upstream (RD(q), tp);
					else if (V_TIME > 0 && tp->t_msglen > 0) {
						/*
						 * POSIX 7.1.1.7.1
						 * Case A: MIN > 0, TIME > 0
						 * If data is in the buffer at 
						 * the time of the read(), the
						 * result shall be as if data 
						 * had been received immediately
						 * after the read().
						 */
						if (tp->t_state & TS_TACT) {
							tid = tp->t_tid;
							tp->t_state &= ~TS_TACT;
							UNLOCK(tp->t_lock, ipl);
							untimeout(tid);
							ipl = LOCK(tp->t_lock, plstr);
							/*
							 * need to recheck 
							 * TS_ACTIVE because a
							 * char may arrive after
							 * untimeout
							 */
							if (!(tp->t_state & TS_TACT)) {
								DEBUG4 (("M_READ VTIME  set and timer not active\n"));
								tp->t_state &= ~TS_RTO;
								tp->t_lbolt = 0;
								ldterm_vmin_timeout_l (RD (q));
							}
						}
						else {
							DEBUG4 (("M_READ VTIME set and about to start timer\n"));
							tp->t_state &= ~TS_RTO;
							tp->t_lbolt = 0;
							ldterm_vmin_timeout_l (RD (q));
						}
					}
					/* if msglen = 0 and vmin has any value
					 * timeout should be started only after
					 * atleast one char has arrived.
					 */
				}
			} else  /* should do bufcall, really! */
				/*
				 *+ Kernel could not allocate memory for
				 *+ a streams message.  This indicates
				 *+ that there is not enough physical
				 *+ memory on the machine or that memory
				 *+ is being lost by the kernel.
				 */
				cmn_err(CE_WARN,"ldterm: (ldtermwput) out of blocks\n");
		}
		UNLOCK(tp->t_lock, ipl);
		/*
		 * pass M_READ down
	         */
		putnext (q, mp);
		break;

	case M_DATA:
		ipl = LOCK(tp->t_lock, plstr);
		if ((tp->t_modes.c_lflag & FLUSHO)
		    && (tp->t_modes.c_lflag & IEXTEN)) {
			UNLOCK(tp->t_lock, ipl);
			freemsg(mp);	/* drop on floor */
			break;
		}
		tp->t_rocount = 0;
		/*
		 * Don't even look at the characters unless we
		 * have something useful to do with them.
		 */
		if (LDTERM_CHANMAP(tp) || (tp->t_modes.c_oflag & OPOST)
		    || ((tp->t_modes.c_lflag & XCASE) && CANON_MODE)) {
			mp = ldterm_output_msg(q, mp, (mblk_t *)NULL, 
					       tp, OBSIZE, 0);
			if (mp == NULL) {
				UNLOCK(tp->t_lock, ipl);
				break;
			}
		}
		if ((tp->t_amodes.c_lflag & PENDIN)
		    && (tp->t_modes.c_lflag & IEXTEN)) {
			if (tp->t_message != NULL) {
				tp->t_state |= TS_RESCAN;
				qenable(RD(q));
			}
		}
		UNLOCK(tp->t_lock, ipl);
		/* Update count of out chars */
		drv_setparm(SYSOUTC, msgdsize(mp));
		putnext(q, mp);
		break;

	default:
		putnext(q, mp);	/* pass it through unmolested */
		break;
	}
	return(0);
}

/*
 * mblk_t *
 * ldterm_output_msg(queue_t *q, mblk_t *imp, mblk_t *omp, lstate_t *tp, int bsize, int echoing)
 *	Perform output processing on a message, accumulating the output
 *	characters in a new message.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC mblk_t *
ldterm_output_msg(queue_t *q, mblk_t *imp, mblk_t *omp, lstate_t *tp,
		int bsize, int echoing)
{
	mblk_t *ibp;	/* block we're examining from input message */
	mblk_t *ipbp;	/* block before ibp in input message */
	mblk_t *obp;	/* block we're filling in output message */
	mblk_t **contpp;/* where to stuff pointer to newly-allocated block */
	uchar_t c;
	int count;
	int ctype;
	int bytes_left;

	mblk_t *bp;	/* block to stuff an M_DELAY message in */


	/*
	 * Allocate a new block into which to put bytes.
	 * If we can't, we just drop the rest of the message on the
	 * floor.
	 */
#define	NEW_BLOCK()	{ \
			if ((obp = allocb(bsize, BPRI_MED)) == NULL) \
				goto outofbufs; \
			*contpp = obp; \
			contpp = &obp->b_cont; \
			bytes_left = obp->b_datap->db_lim - obp->b_wptr; \
			}

	ibp = imp;
	ipbp = (mblk_t *) NULL;

	/*
	 * When we allocate the first block of a message, we should stuff
	 * the pointer to it in "omp".  All subsequent blocks should
	 * have the pointer to them stuffed into the "b_cont" field of the
	 * previous block.  "contpp" points to the place where we should
	 * stuff the pointer.
	 *
	 * If we already have a message we're filling in, continue doing
	 * so.
	 */
	if ((obp = omp) != NULL) {
		for (; obp->b_cont != NULL; obp = obp->b_cont)
			;
		contpp = &obp->b_cont;
		bytes_left = obp->b_datap->db_lim - obp->b_wptr;
	} else {
		contpp = &omp;
		omp = NULL;
		bytes_left = 0;
	}

	do {
		int nsize;
		nsize = ibp->b_wptr - ibp->b_rptr;

                /* if channel mapping, map ibp */
                if (nsize && LDTERM_CHANMAP(tp)) {
			chanmap_output_msg(tp, nsize, &imp, &ibp, &ipbp);
			if (!(tp->t_modes.c_oflag & OPOST))
				return(ibp);
		}

		while (ibp->b_rptr < ibp->b_wptr) {
			/*
			 * Make sure there's room for one more
			 * character.  At most, we'll need "t_maxeuc"
			 * bytes.
			 */
			if ((bytes_left < (int) tp->t_maxeuc))
				NEW_BLOCK();

			/*
			 * If doing XCASE processing (not very likely,
			 * in this day and age), look at each character
			 * individually.
			 */
			if ((tp->t_modes.c_lflag & XCASE) && CANON_MODE) {
				c = *ibp->b_rptr++;

				/*
				 * If character is mapped on output, put out
				 * a backslash followed by what it is
				 * mapped to.
				 */
				if (omaptab[c] != 0
				    && (!echoing || c != '\\')) {
					tp->t_col++;	/* backslash is an ordinary character */
					*obp->b_wptr++ = '\\';
					bytes_left--;
					if (bytes_left == 0)
						NEW_BLOCK();
					c = omaptab[c];
				}

				/*
				 * If no other output processing is required,
				 * push the character into the block and
				 * get another.
				 */
				if (!(tp->t_modes.c_oflag & OPOST)) {
					tp->t_col++;
					*obp->b_wptr++ = c;
					bytes_left--;
					continue;
				}

				/*
				 * OPOST output flag is set.
				 * Map lower case to upper case if OLCUC flag
				 * is set.
				 */
				if ((tp->t_modes.c_oflag & OLCUC)
				    && c >= 'a' && c <= 'z')
					c -= 'a' - 'A';
			} else {
/*
 * Copy all the ORDINARY characters, possibly mapping upper case to lower
 * case.  We use "movtuc", STOPPING when we can't move some character.
 * For multi-byte or multi-column EUC, we can't depend on the regular tables.
 * Rather than just drop through to the "big switch" for all characters, it
 * _might_ be faster to let "movtuc" move a bunch of characters.  Chances
 * are, even in multi-byte mode we'll have lots of ASCII going through.
 * We check the flag once, and call movtuc with the appropriate table as
 * an argument.
 */
				int bytes_to_move;
				int bytes_moved;

				bytes_to_move = ibp->b_wptr - ibp->b_rptr;
				if (bytes_to_move > bytes_left)
					bytes_to_move = bytes_left;
				if (tp->t_state & TS_MEUC) {
					bytes_moved = movtuc(bytes_to_move,
					 ibp->b_rptr, obp->b_wptr,
					 (tp->t_modes.c_oflag & OLCUC ? elcuctab : enotrantab));
				}
				else {
					bytes_moved = movtuc(bytes_to_move,
					 ibp->b_rptr, obp->b_wptr,
					 (tp->t_modes.c_oflag & OLCUC ? lcuctab : notrantab));
				}
/*
 * We're save to just do this column calculation, because if TS_MEUC is set,
 * we used the proper EUC tables, and won't have copied any EUC bytes.
 */
				tp->t_col += bytes_moved;
				ibp->b_rptr += bytes_moved;
				obp->b_wptr += bytes_moved;
				bytes_left -= bytes_moved;
				if (ibp->b_rptr >= ibp->b_wptr)
					continue;	/* moved all of block */
				if (bytes_left == 0)
					NEW_BLOCK();
				c = *ibp->b_rptr++;	/* stopper */
			}
/*
 * If the driver has requested, don't process output flags.  However, if
 * we're in multi-byte mode, we HAVE to look at EVERYTHING going out to
 * maintain column position properly. Therefore IF the driver says don't
 * AND we're not doing multi-byte, then don't do it.  Otherwise, do it.
 * 
 * NOTE:  Hardware USUALLY doesn't expand tabs properly for multi-byte
 * situations anyway; that's a known problem with the 3B2 "PORTS" board
 * firmware, and any other hardware that doesn't ACTUALLY know about the
 * current EUC mapping that WE are using at this very moment.  The problem
 * is that memory width is INDEPENDENT of screen width - no relation - so
 * WE know how wide the characters are, but an off-the-host board probably
 * doesn't.  So, until we're SURE that the hardware below us can correctly
 * expand tabs in a multi-byte/multi-column EUC situation, we do it
 * ourselves.
 */
			/*
			 * Map <CR>to<NL> on output if OCRNL flag set.
			 * ONLCR processing is not done if OCRNL is set.

			 */
			if (c == '\r' && (tp->t_modes.c_oflag & OCRNL)) {
				c = '\n';
				ctype = typetab[c];
				goto jocrnl;
			}
			ctype = typetab[c];

			/*
			 * Map <NL> to <CR><NL> on output if ONLCR
			 * flag is set.
			 */
			if (c == '\n' && (tp->t_modes.c_oflag & ONLCR)) {
				if (!(tp->t_state & TS_TTCR)) {
					tp->t_state |= TS_TTCR;
					c = '\r';
					ctype = typetab['\r'];
					--ibp->b_rptr;
				} else
					tp->t_state &= ~TS_TTCR;
			}

/*
* Delay values and column position calculated here.  For EUC chars in
* multi-byte mode, we use "t_eucign" to help calculate columns.  When
* we see the first byte of an EUC, we set t_eucign to the number of
* bytes that will FOLLOW it, and we add the screen width of the WHOLE
* EUC character to the column position.  In particular, we can't count
* SS2 or SS3 as printing characters.  Remember, folks, the screen width
* and memory width are independent - no relation.
* We could have dropped through for ASCII, but we want to catch any
* bad characters (i.e., t_eucign set and an ASCII char received) and
* possibly report the garbage situation.
*/
			jocrnl:

			count = 0;
			switch (ctype) {

			case T_SS2:
			case T_SS3:
			case ORDINARY:
				if (tp->t_state & TS_MEUC) {
					if (NOTASCII(c)) {
						*obp->b_wptr++ = c;
						bytes_left--;
						/* In middle of EUC? */
						if (tp->t_eucign) {
							--tp->t_eucign;
							break;
						}
						else {
							tp->t_col +=
							   ldterm_dispwidth(c,
							     &tp->eucwioc,
							     tp->t_modes.c_lflag & ECHOCTL);
							tp->t_eucign =
							    ldterm_memwidth(c, &tp->eucwioc) - 1;
						}
					}
					else {
						if (tp->t_eucign) {
							tp->t_eucign = 0;
							ldterm_eucwarn(tp);
						}
						tp->t_col++;
						*obp->b_wptr++ = c;
						bytes_left--;
					}
				}
				else {	/* ho hum, ASCII mode... */
					tp->t_col++;
					*obp->b_wptr++ = c;
					bytes_left--;
				}
				break;

/*
* If we're doing ECHOCTL, we've already mapped the thing during the process
* of canonising.  Don't bother here, as it's not one that we did.
*/
			case CONTROL:
				*obp->b_wptr++ = c;
				bytes_left--;
				break;

/*
* This is probably a backspace received, not one that we're
* echoing.  Let it go as a single-column backspace.
*/
			case BACKSPACE:
				if (tp->t_col)
					tp->t_col--;
				if (tp->t_modes.c_oflag & BSDLY) {
					if (tp->t_modes.c_oflag & OFILL)
						count = 1;
					else
						count = 3;
				}
				*obp->b_wptr++ = c;
				bytes_left--;
				break;

			case NEWLINE:
				if (tp->t_modes.c_oflag & ONLRET)
					goto cr;
				if ((tp->t_modes.c_oflag & NLDLY) == NL1)
					count = 2;
				*obp->b_wptr++ = c;
				bytes_left--;
				break;

			case TAB:
/*
* Map '\t' to spaces if XTABS flag is set.  The calculation of "t_eucign"
* has probably insured that column will be correct, as we bumped t_col by the
* DISP width, not the memory width.
*/
				if ((tp->t_modes.c_oflag & TABDLY) == XTABS) {
					for (;;) {
						*obp->b_wptr++ = ' ';
						bytes_left--;
						tp->t_col++;
						if ((tp->t_col & 07) == 0)
							break;	/* every 8th */
						/*
						 * If we don't have room to
						 * fully expand this tab in
						 * this block, back up to
						 * continue expanding it
						 * into the next block.
						 */
						if (obp->b_wptr >= obp->b_datap->db_lim) {
							ibp->b_rptr--;
							break;
						}
					}
				} else {
					tp->t_col |= 07;
					tp->t_col++;
					if (tp->t_modes.c_oflag & OFILL) {
						if (tp->t_modes.c_oflag & TABDLY)
							count = 2;
					} else {
						switch (tp->t_modes.c_oflag & TABDLY) {
						case TAB2:
							count = 6;
							break;

						case TAB1:
							count = 1 + (tp->t_col | ~07);
							if (count < 5)
								count = 0;
							break;
						}
					}
					*obp->b_wptr++ = c;
					bytes_left--;
				}
				break;

			case VTAB:
				if ((tp->t_modes.c_oflag & VTDLY)
				    && !(tp->t_modes.c_oflag & OFILL))
					count = 127;
				*obp->b_wptr++ = c;
				bytes_left--;
				break;

			case RETURN:
				/*
				 * Ignore <CR> in column 0 if ONOCR flag set.
				 */
				if (tp->t_col == 0
				    && (tp->t_modes.c_oflag & ONOCR))
					break;

			cr:
				switch (tp->t_modes.c_oflag & CRDLY) {

				case CR1:
					if (tp->t_modes.c_oflag & OFILL)
						count = 2;
					else
						count = tp->t_col % 2;
					break;

				case CR2:
					if (tp->t_modes.c_oflag & OFILL)
						count = 4;
					else
						count = 6;
					break;

				case CR3:
					if (tp->t_modes.c_oflag & OFILL)
						count = 0;
					else
						count = 9;
					break;
				}
				tp->t_col = 0;
				*obp->b_wptr++ = c;
				bytes_left--;
				break;
			}

			if (count != 0) {
				if (tp->t_modes.c_oflag & OFILL) {
					do {
						if (bytes_left == 0)
							NEW_BLOCK();
						if (tp->t_modes.c_oflag & OFDEL)
							*obp->b_wptr++ = CDEL;
						else
							*obp->b_wptr++ = CNUL;
						bytes_left--;
					} while (--count != 0);
				} else {
					if ((tp->t_modes.c_lflag & FLUSHO)
					    && (tp->t_modes.c_lflag & IEXTEN)) {
						freemsg(omp);	/* drop on floor */
					} else {
						/* Update count of out chars */
						drv_setparm(SYSOUTC, msgdsize(omp));
						/*
						 * it's a lot of work to pass
						 * in the ipl argument just for
						 * a putnext.  Just stay where
						 * were - that should be ok
						 */
						UNLOCK(tp->t_lock, getpl());
						putnext(q, omp);
						(void) LOCK(tp->t_lock, plstr);
						/*
						 * Send M_DELAY downstream
						 */
						if (( bp = allocb( 1, BPRI_MED)) != NULL) {
							bp->b_datap->db_type = M_DELAY;
							*bp->b_wptr++ = (char)count;
							UNLOCK(tp->t_lock, getpl());
							putnext(q, bp);
							(void) LOCK(tp->t_lock, plstr);
						}
					}
					bytes_left = 0;
					/*
					 * We have to start a new message;
					 * the delay introduces a break
					 * between messages.
					 */
					omp = NULL;
					contpp = &omp;
				}
			}
			/**} **/
		}
	} while ((ibp = ibp->b_cont) != NULL);	/* next block, if any */

outofbufs:
	freemsg(imp);
	return (omp);
#undef NEW_BLOCK
}


/*
 * int
 * movtuc(int size, uchar_t *from, uchar_t *origto, uchar_t *table)
 *	Copy characters from "from" to "origto", possibly mapping them as
 *	they are copied.
 *
 * Calling/Exit State:
 *	t_lock held on entry and exit.
 */

STATIC int
movtuc(int size, uchar_t *from, uchar_t *origto, uchar_t *table)
{
	uchar_t *to;
	uchar_t c;

	to = origto;
	while (size != 0 && (c = table[*from++]) != 0) {
		*to++ = c;
		size--;
	}
	return (to - origto);
}

/*
 * void
 * ldterm_flush_output(uchar_t c, queue_t *q, lstate_t *tp)
 *	Flush output queues (including those below us) and mark state that
 *	we are flushing (or clear status if called with FLUSHO set).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.  If FLUSHO set on entry, clear it
 *	and return, else it is set on exit.
 */

STATIC void
ldterm_flush_output(uchar_t c, queue_t *q, lstate_t *tp)
{
	pl_t ipl;
	mblk_t *bp;

	/* Already conditioned with IEXTEN during VDISCARD processing*/
	ipl = LOCK(tp->t_lock, plstr);
	if (tp->t_modes.c_lflag & FLUSHO)
		tp->t_modes.c_lflag &= ~FLUSHO;
	else {
		flushq(q, FLUSHDATA);	/* flush our write queue */
		UNLOCK(tp->t_lock, ipl);
		(void) putnextctl1(q, M_FLUSH, FLUSHW);	/* flush the ones below us */
		ipl = LOCK(tp->t_lock, plstr);
		if ((tp->t_echomp = allocb(EBSIZE, BPRI_HI)) != NULL) {
			(void) ldterm_echo(c, q, 1, tp);
			if (tp->t_msglen != 0)
				ldterm_reprint(q, EBSIZE, tp);
			if (tp->t_echomp != NULL) {
				bp = tp->t_echomp;
				tp->t_echomp = NULL;
				UNLOCK(tp->t_lock, ipl);
				putnext(q, bp);
				ipl = LOCK(tp->t_lock, plstr);
			}
		}
		tp->t_modes.c_lflag |= FLUSHO;
	}
	UNLOCK(tp->t_lock, ipl);
}

/*
 * void
 * ldterm_dosig(queue_t *q, int sig, uchar_t c, int mtype, int mode, int flushflag)
 *	Generate a signal ("sig") to the lwp and send it in a message of type
 *	"mtype".  Send a flush if necessary.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.  Signal "sig" sent upstream.
 */

STATIC void
ldterm_dosig(queue_t *q, int sig, uchar_t c, int mtype, int mode, int flushflag)
{
	lstate_t *tp;
	mblk_t *mp;
	pl_t ipl;

	tp = (lstate_t *) q->q_ptr;
	/*
	 * c == \0 is brk case; need to flush on BRKINT even
         * if noflsh is set.
         */
	if (flushflag || (c == '\0')) {
		if (mode) {
			/*
			 * Flush read or write side
			 * Restart the input or output
			 */

			if (mode & FLUSHR) {
				flushq(q, FLUSHDATA);
				(void) putnextctl1(WR(q), M_FLUSH, FLUSHR);
				ipl = LOCK(tp->t_lock, plstr);
				if (tp->t_state & TS_TBLOCK) {
					tp->t_state &= ~TS_TBLOCK;
					tp->t_state &= ~TS_FLOW;
					UNLOCK(tp->t_lock, ipl);
					(void) putnextctl(WR(q), M_STARTI);
				}
				else
					UNLOCK(tp->t_lock, ipl);
			}
			if (mode & FLUSHW) {
				flushq(WR(q), FLUSHDATA);
				(void) putnextctl1(q, M_FLUSH, FLUSHW);
				ipl = LOCK(tp->t_lock, plstr);
				if (tp->t_state & TS_TTSTOP) {
					tp->t_state &= ~TS_TTSTOP;
					UNLOCK(tp->t_lock, ipl);
					(void) putnextctl(WR(q), M_START);
				}
				else
					UNLOCK(tp->t_lock, ipl);
			}
		}
	}
	ipl = LOCK(tp->t_lock, plstr);
	tp->t_state &= ~TS_QUOT;
	UNLOCK(tp->t_lock, ipl);
	(void) putnextctl1(q, mtype, sig);

	if (c != '\0') {
		ipl = LOCK(tp->t_lock, plstr);
		if ((tp->t_echomp = allocb(4, BPRI_HI)) != NULL) {
			(void) ldterm_echo(c, WR(q), 4, tp);
			/* using mp lets us avoid an extra lock */
			mp = tp->t_echomp;
			tp->t_echomp = NULL;
			UNLOCK(tp->t_lock, ipl);
			putnext(WR(q), mp);
		}
		else
			UNLOCK(tp->t_lock, ipl);
	}
	return;
}

/*
 * void
 * ldterm_do_ioctl(queue_t *q, mblk_t *mp)
 *	Called when an M_IOCTL message is seen on the write queue; does whatever
 *	we're supposed to do with it, and either replies immediately or passes
 *	it to the next module down.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

STATIC void
ldterm_do_ioctl(queue_t *q, mblk_t *mp)
{
	lstate_t *tp;
	struct iocblk *iocp;
	struct eucioc *euciocp;	/* needed for EUC ioctls */
	pl_t ipl;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	tp = (lstate_t *)q->q_ptr;


	switch (iocp->ioc_cmd) {

	case TCSETS:
	case TCSETSW:
	case TCSETSF: {
		/*
		 * Set current parameters and special characters.
		 */
		struct termios *cb;
		struct termios oldmodes;

		if (! mp->b_cont)	/* THIS CAN HAPPEN! */
			goto setgetfail;

		/* LINTED pointer alignment */
		cb = (struct termios *) mp->b_cont->b_rptr;

		ipl = LOCK(tp->t_lock, plstr);
		oldmodes = tp->t_amodes;
		tp->t_amodes = *cb;
		if ((tp->t_amodes.c_lflag & PENDIN)
		    && (tp->t_modes.c_lflag & IEXTEN)) {
			if (tp->t_message != NULL) {
				tp->t_state |= TS_RESCAN;
				qenable(RD(q));
			}
		}

		bcopy((caddr_t)tp->t_amodes.c_cc, (caddr_t)tp->t_modes.c_cc, NCCS);

		/* ldterm_adjust_modes does not deal with cflags */
		tp->t_modes.c_cflag = tp->t_amodes.c_cflag;

		ldterm_adjust_modes (tp);
		if (chgstropts(&oldmodes, tp, RD(q)) == (-1)) {
			iocp->ioc_error = EAGAIN;
			UNLOCK(tp->t_lock, ipl);
			goto setgetfail2;
		}

		/*
		 * The driver may want to know about the following iflags:
		 * IGNBRK, BRKINT, IGNPAR, PARMRK, INPCK, IXON, IXANY.
		 */
		UNLOCK(tp->t_lock, ipl);
		putnext(q, mp);
		return;
	}

	case TCSETA:
	case TCSETAW:
	case TCSETAF: {
		/*
		 * Old-style "ioctl" to set current parameters and
		 * special characters.
		 * Don't clear out the unset portions, leave them as
		 * they are.
		 */
		struct termio *cb;
		struct termios oldmodes;

		if (! mp->b_cont) {	/* THIS CAN HAPPEN! */
			goto setgetfail;
		}

		/* LINTED pointer alignment */
		cb = (struct termio *) mp->b_cont->b_rptr;

		ipl = LOCK(tp->t_lock, plstr);
		oldmodes = tp->t_amodes;
		tp->t_amodes.c_iflag =
		    (tp->t_amodes.c_iflag & 0xffff0000 | cb->c_iflag);
		tp->t_amodes.c_oflag =
		    (tp->t_amodes.c_oflag & 0xffff0000 | cb->c_oflag);
		tp->t_amodes.c_cflag =
		    (tp->t_amodes.c_cflag & 0xffff0000 | cb->c_cflag);
		tp->t_amodes.c_lflag =
		    (tp->t_amodes.c_lflag & 0xffff0000 | cb->c_lflag);

		bcopy((caddr_t)cb->c_cc, (caddr_t)tp->t_modes.c_cc, NCC);
		/* TCGETS returns amodes, so update that too */
		bcopy((caddr_t)cb->c_cc, (caddr_t)tp->t_amodes.c_cc, NCC);

		/* ldterm_adjust_modes does not deal with cflags */
		tp->t_modes.c_cflag = tp->t_amodes.c_cflag;

		ldterm_adjust_modes (tp);
		if (chgstropts(&oldmodes, tp, RD(q)) == (-1)) {
			iocp->ioc_error = EAGAIN;
			UNLOCK(tp->t_lock, ipl);
			goto setgetfail2;
		}

		/*
		 * The driver may want to know about the following iflags:
		 * IGNBRK, BRKINT, IGNPAR, PARMRK, INPCK, IXON, IXANY.
		 */
		UNLOCK(tp->t_lock, ipl);
		putnext(q, mp);
		return;
	}

	case TCFLSH:
		/*
		 * Do the flush on the write queue immediately, and queue
		 * up any flush on the read queue for the service procedure
		 * to see.  Then turn it into the appropriate M_FLUSH message,
		 * so that the module below us doesn't have to know about
		 * TCFLSH.
		 */
		if (! mp->b_cont) {
			goto setgetfail;
		}
		ASSERT(mp->b_datap != NULL);
		/* LINTED pointer alignment */
		if (*(int *) mp->b_cont->b_rptr == 0) {
			ASSERT(mp->b_datap != NULL);
			(void) putnextctl1(q, M_FLUSH, FLUSHR);
			(void) putctl1(RD(q), M_FLUSH, FLUSHR);
		/* LINTED pointer alignment */
		} else if (*(int *) mp->b_cont->b_rptr == 1) {
			flushq(q, FLUSHDATA);
			ASSERT(mp->b_datap != NULL);
			(void) putnextctl1(q, M_FLUSH, FLUSHW);
			(void) putnextctl1(RD(q), M_FLUSH, FLUSHW);
		/* LINTED pointer alignment */
		} else if (*(int *) mp->b_cont->b_rptr == 2) {
			flushq(q, FLUSHDATA);
			ASSERT(mp->b_datap != NULL);
			(void) putnextctl1(q, M_FLUSH, FLUSHRW);
			(void) putctl1(RD(q), M_FLUSH, FLUSHRW);
		} else {
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			iocp->ioc_rval = (-1);
			iocp->ioc_count = 0;
			qreply(q, mp);
			return;
		}
		ASSERT(mp->b_datap != NULL);
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_rval = 0;
		iocp->ioc_count = 0;
		qreply(q, mp);
		return;

	case TCXONC:
		if (! mp->b_cont) {
			goto setgetfail;
		}
		/* LINTED pointer alignment */
		switch (*(int *) mp->b_cont->b_rptr) {
		case 0:
			ipl = LOCK(tp->t_lock, plstr);
			if (!(tp->t_state & TS_TTSTOP)) {
				tp->t_state |= TS_TTSTOP;
				UNLOCK(tp->t_lock, ipl);
				(void) putnextctl(q, M_STOP);
			}
			else
				UNLOCK(tp->t_lock, ipl);
			break;

		case 1:
			ipl = LOCK(tp->t_lock, plstr);
			if (tp->t_state & TS_TTSTOP) {
				tp->t_state &= ~TS_TTSTOP;
				UNLOCK(tp->t_lock, ipl);
				(void) putnextctl(q, M_START);
			}
			else
				UNLOCK(tp->t_lock, ipl);
			break;

		case 2:
			ipl = LOCK(tp->t_lock, plstr);
			tp->t_state |= TS_TBLOCK;
			tp->t_state |= TS_FLOW;
			UNLOCK(tp->t_lock, ipl);
			(void) putnextctl(q, M_STOPI);
			break;

		case 3:
			ipl = LOCK(tp->t_lock, plstr);
			tp->t_state &= ~TS_TBLOCK;
			tp->t_state &= ~TS_FLOW;
			UNLOCK(tp->t_lock, ipl);
			(void) putnextctl(q, M_STARTI);
			break;

		default:
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			iocp->ioc_rval = (-1);
			iocp->ioc_count = 0;
			qreply(q, mp);
			return;
		}
		ASSERT(mp->b_datap != NULL);
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_rval = 0;
		iocp->ioc_count = 0;
		qreply(q, mp);
		return;

	case TIOCSTI: { /* Simulate typing of a character at the terminal. */
		struct msgb *nmp;

		if (! mp->b_cont) {
			goto setgetfail;
		}

		/* 
		 * break apart the message, sending the first part 
		 * that has the ioctl up as a ACK. and the bcont
		 * part to the ldterm's read side put procedure as
		 * a M_DATA.
		 */

		nmp = unlinkb(mp);
		nmp->b_datap->db_type = M_DATA;
		put(RD(q), nmp);

		ASSERT(mp->b_datap != NULL);
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_rval = 0;
		iocp->ioc_count = 0;
		qreply(q, mp);
		return;

	}


/* 
 * TCSBRK is expected to be handled by the driver. The reason its
 * left for the driver is that when the argument to TCSBRK is zero
 * driver has to drain the data and sending a M_IOCACK from LDTERM
 * before the driver drains the data is going to cause problems.
 */

	/*
	 * The following are EUC related ioctls.  For EUC_WSET,
	 * we have to pass the information on, even though we ACK
	 * the call.  It's vital in the EUC environment that
	 * everybody downstream knows about the EUC codeset
	 * widths currently in use; we therefore pass down the
	 * information in an M_CTL message.  It will bottom out
	 * in the driver.
	 */
	case EUC_WSET: {

		struct iocblk *riocp;	/* only needed for EUC_WSET */
		mblk_t *dmp, *dmp_cont;
		int i;

		/*
		 * If the user didn't supply any information, NAK it.
		 */
		if ((! mp->b_cont) ||
		    (! (euciocp = (struct eucioc *) mp->b_cont->b_rptr))) {
/*
 * protocol failure comes here - either there wasn't a buffer attached
 * when there should have been, or something else is amiss.
 */
setgetfail:
			iocp->ioc_error = EPROTO;
/*
 * Streams resource failures and others can come here, with ioc_error
 * already set (e.g., to ENOSR or whatever).
 */
setgetfail2:
			iocp->ioc_count = 0;
			iocp->ioc_rval = (-1);
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
		}
		/*
		 * Check here for reasonableness.  If anything will take
		 * more than EUC_MAXW columns or more than EUC_MAXW bytes
		 * following SS2 or SS3, then just reject it out of hand.
		 * It's not impossible for us to do it, it just isn't
		 * reasonable.  So far, in the world, we've seen the
		 * absolute max columns to be 2 and the max number of
		 * bytes to be 3.  This allows room for some expansion
		 * of that, but it probably won't even be necessary.
		 * At the moment, we return a "range" error.  If you
		 * really need to, you can push EUC_MAXW up to over 200;
		 * it doesn't make sense, though, with only a CANBSIZ sized
		 * input limit (usually 256)!
		 */
		for (i = 0; i < 4; i++) {
			if ((euciocp->eucw[i] > EUC_MAXW) ||
			    (euciocp->scrw[i] > EUC_MAXW)) {
				iocp->ioc_error = ERANGE;
				goto setgetfail2;
			}
		}
		/*
		 * Otherwise, save the information in tp, force codeset 0
		 * (ASCII) to be one byte, one column.
		 */
		ipl = LOCK(tp->t_lock, plstr);
		cp_eucwioc(euciocp, &tp->eucwioc, EUCIN);
		tp->eucwioc.eucw[0] = tp->eucwioc.scrw[0] = 1;
		/*
		 * Now, check out whether we're doing multibyte processing.
		 * if we are, we need to allocate a block to hold the
		 * parallel array.
		 * By convention, we've been passed what amounts to a
		 * CSWIDTH definition.  We actually NEED the number of
		 * bytes for Codesets 2 & 3.  
		 */
		tp->t_maxeuc = 0;	/* reset to say we're NOT */
		tp->t_state &= ~TS_MEUC;
		/*
		 * We'll set TS_MEUC if we're doing multi-column OR multi-
		 * byte OR both.  It makes things easier...  NOTE:  If we
		 * fail to get the buffer we need to hold display widths,
		 * then DON'T let the TS_MEUC bit get set!
		 */
		for (i = 0; i < 4; i++) {
			if (tp->eucwioc.eucw[i] > tp->t_maxeuc)
				tp->t_maxeuc = tp->eucwioc.eucw[i];
			if (tp->eucwioc.scrw[i] > 1)
				tp->t_state |= TS_MEUC;
		}
		if ((tp->t_maxeuc > 1) || (tp->t_state & TS_MEUC)) {
			if (!tp->t_eucp_mp) {
				if (! (tp->t_eucp_mp = allocb(CANBSIZ, BPRI_HI))) {
					tp->t_maxeuc = 1;
					tp->t_state &= ~TS_MEUC;
					iocp->ioc_error = ENOSR;
					UNLOCK(tp->t_lock, ipl);
					goto setgetfail2;
				}
			}
			/*
			 * here, if there's junk in the canonical buffer,
			 * then move the eucp pointer past it, so we don't
			 * run off the beginning.  This is a total botch,
			 * but will hopefully keep stuff from getting too
			 * messed up until the user flushes this line!
			 */
			if (tp->t_msglen) {
				tp->t_eucp = tp->t_eucp_mp->b_rptr;
				for (i = tp->t_msglen; i; i--)
					*tp->t_eucp++ = 1;
				tp->t_eucp = tp->t_eucp_mp->b_rptr + tp->t_msglen;
			}
			else
				tp->t_eucp = tp->t_eucp_mp->b_rptr;
			tp->t_state |= TS_MEUC;	/* doing multi-byte handling */
		}
		else if (tp->t_eucp_mp) {
			freemsg(tp->t_eucp_mp);
			tp->t_eucp_mp = NULL;
			tp->t_eucp = NULL;
		}

		UNLOCK(tp->t_lock, ipl);
		/*
		 * If we are able to allocate two blocks (the iocblk and
		 * the associated data), then pass it downstream, otherwise
		 * we'll need to NAK it, and drop whatever we WERE able to
		 * allocate.
		 */
		if (! (dmp = allocb(sizeof(struct iocblk), BPRI_HI))) {
			iocp->ioc_error = ENOSR;
			goto setgetfail2;
		}
		if (! (dmp_cont = allocb(EUCSIZE, BPRI_HI))) {
			freemsg(dmp);
			iocp->ioc_error = ENOSR;
			goto setgetfail2;
		}
		/*
		 * We got both buffers.  Copy out the EUC information
		 * (as we received it, not what we're using!) & pass it on.
		 */
		bcopy((caddr_t)mp->b_cont->b_rptr, (caddr_t)dmp_cont->b_wptr, EUCSIZE);
		dmp_cont->b_wptr += EUCSIZE;
		dmp->b_cont = dmp_cont;
		dmp->b_datap->db_type = M_CTL;
		dmp_cont->b_datap->db_type = M_DATA;
		/* LINTED pointer alignment */
		riocp = (struct iocblk *) dmp->b_rptr;
		riocp->ioc_count = iocp->ioc_count;
		riocp->ioc_error = 0;
		riocp->ioc_rval = 0;
		riocp->ioc_cmd = EUC_WSET;
		dmp->b_wptr += sizeof(struct iocblk);
		putnext(q, dmp);
		/*
		 * Now ACK the ioctl.
		 */
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_error = 0;
		iocp->ioc_rval = 0;
		qreply(q, mp);
		return;
	}

	case EUC_WGET:
		if (! mp->b_cont)
			goto setgetfail;	/* protocol error */
		euciocp = (struct eucioc *) mp->b_cont->b_rptr;
		ipl = LOCK(tp->t_lock, plstr);
		cp_eucwioc(&tp->eucwioc, euciocp, EUCOUT);
		UNLOCK(tp->t_lock, ipl);
		iocp->ioc_count = EUCSIZE;
		iocp->ioc_error = iocp->ioc_rval = 0;
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_error = 0;
		iocp->ioc_rval = 0;
		qreply(q, mp);
		return;

	}

	if (chanmap_do_ioctl(q, mp, iocp, tp)) {
		/* the ioctl is processed */
		return;
	}

	putnext(q, mp);
}

/*
 * int
 * chgstropts(struct termios *oldmodep, lstate_t *tp, queue_t *q)
 *	Send an M_SETOPTS message upstream if any mode changes are being made
 *	that affect the stream head options.  Also send an M_FLUSH message
 *	upstream if canonical mode is being turned on; a pile of non-canonical
 *	input will look very confusing when we switch to "message-nondiscard"
 *	mode - it will appear to be bunches of characters separated by EOFs.
 *
 * Calling/Exit State:
 *	t_lock held on entry and exit.  Returns -1 if allocb fails, else 0.
 *
 */

STATIC int
chgstropts(struct termios *oldmodep, lstate_t *tp, queue_t *q)
{
	struct stroptions optbuf;
	mblk_t *bp;
	int tid;

	if (V_TIME == 0) {
		/* don't leave vestige timer if VTIME is cleared */
		if (tp->t_tid && (tp->t_state & TS_TACT)) {
			tp->t_state &= ~TS_TACT;
			tid = tp->t_tid;
			UNLOCK(tp->t_lock, plstr);
			untimeout(tid);
			(void) LOCK(tp->t_lock, plstr);
			tp->t_state &= ~TS_RTO;
			tp->t_lbolt = 0;
			tp->t_tid = 0;
		}
	}

	optbuf.so_flags = 0;
	if ((oldmodep->c_lflag ^ tp->t_modes.c_lflag) & ICANON) {
		/*
		 * Canonical mode is changing state; switch the stream head
		 * to message-nondiscard or byte-stream mode.  Also, rerun
		 * the service procedure so it can change its mind about
		 * whether to send data upstream or not.
		 */
		if (CANON_MODE) {
			DEBUG4 (("CHANGING TO CANON MODE\n"));
			optbuf.so_flags = SO_READOPT|SO_MREADOFF;
			optbuf.so_readopt = RMSGN;

			/* if there is a pending raw mode timeout, clear it */

			if (tp->t_tid && (tp->t_state & TS_TACT)) {
				tp->t_state &= ~TS_TACT;
				DEBUG4 (("chgstropts: timer active untimeout called \n"));
				tid = tp->t_tid;
				UNLOCK(tp->t_lock, plstr);
				untimeout(tid);
				LOCK(tp->t_lock, plstr);
				tp->t_state &= ~TS_RTO;
				tp->t_lbolt = 0;
				tp->t_tid = 0;
			}
		} else {
			DEBUG4 (("CHANGING TO RAW MODE\n"));
			optbuf.so_flags = SO_READOPT|SO_MREADON;
			optbuf.so_readopt = RNORM;
		}
	}

	if ((oldmodep->c_lflag ^ tp->t_modes.c_lflag) & TOSTOP) {
		/*
		 * The "stop on background write" bit is changing.
		 */
		if (tp->t_modes.c_lflag & TOSTOP)
			optbuf.so_flags |= SO_TOSTOP;
		else
			optbuf.so_flags |= SO_TONSTOP;
	}

	if (optbuf.so_flags != 0) {
		if ((bp = allocb(sizeof (struct stroptions), BPRI_HI)) == NULL) {
			return(-1);
		}
		/* LINTED pointer alignment */
		*(struct stroptions *) bp->b_wptr = optbuf;
		bp->b_wptr += sizeof (struct stroptions);
		bp->b_datap->db_type = M_SETOPTS;
		DEBUG4 (("M_SETOPTS to stream head\n"));
		UNLOCK(tp->t_lock, plstr);
		putnext(q, bp);
		(void) LOCK(tp->t_lock, plstr);
	}
	return(0);
}


/*
 * void
 * ldterm_ioctl_reply(queue_t *q, mblk_t *mp)
 *	Called when an M_IOCACK message is seen on the read queue; modifies
 *	the data being returned, if necessary, and passes the reply up.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

STATIC void
ldterm_ioctl_reply(queue_t *q, mblk_t *mp)
{
	lstate_t *tp;
	struct iocblk *iocp;
	pl_t ipl;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;
	tp = (lstate_t *) q->q_ptr;

	switch (iocp->ioc_cmd) {

	case TCGETS: {
		/*
		 * Get current parameters and return them to stream head
		 * eventually.
		 */
		struct termios *cb =
		    /* LINTED pointer alignment */
		    (struct termios *) mp->b_cont->b_rptr;

		/* cflag has cflags sent upstream by the driver */
		ulong_t cflag = cb->c_cflag;

		ipl = LOCK(tp->t_lock, plstr);
		*cb = tp->t_amodes;
		UNLOCK(tp->t_lock, ipl);
		if (cflag != 0)
			cb->c_cflag = cflag;	/* set by driver */
		break;
	}

	case TCGETA: {
		/*
		 * Old-style "ioctl" to get current parameters and
		 * return them to stream head eventually.
		 */
		struct termio *cb =
		    /* LINTED pointer alignment */
		    (struct termio *) mp->b_cont->b_rptr;

		ipl = LOCK(tp->t_lock, plstr);
		cb->c_iflag = tp->t_amodes.c_iflag;	/* all except the */
		cb->c_oflag = tp->t_amodes.c_oflag;	/* cb->c_cflag */
		cb->c_lflag = tp->t_amodes.c_lflag;

		if (cb->c_cflag == 0)	/* not set by driver */
			cb->c_cflag = tp->t_amodes.c_cflag;

		cb->c_line = 0;
		bcopy((caddr_t)tp->t_amodes.c_cc, (caddr_t)cb->c_cc, NCC);
		UNLOCK(tp->t_lock, ipl);
		break;
	}
	}
	putnext(q, mp);
}


/*
 * void
 * ldterm_vmin_timeout(queue_t *q)
 *	Called from timeout routine to handle VMIN/VTIME processing.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

STATIC void
ldterm_vmin_timeout(queue_t *q)
{
	lstate_t *tp;
	pl_t ipl;

	tp = (lstate_t *)q->q_ptr;
	ipl = LOCK(tp->t_lock, plstr);
	ldterm_vmin_timeout_l(q);
	UNLOCK(tp->t_lock, ipl);
}

/*
 * void
 * ldterm_vmin_timeout_l(queue_t *q)
 *	Called indirectly from timeout routine or directly from
 *	ldterm_dononcanon.
 *
 * Calling/Exit State:
 *	t_lock held on entry and exit.
 */

STATIC void
ldterm_vmin_timeout_l(queue_t *q)
{
	lstate_t *tp;
	clock_t now;
	long ticks;

	DEBUG4(("ldterm_vmin_timeout:\n"));
	/* 
	 * LDTERM  may be popped when a function is pending in callout table.
	 */
	if (!q) {
		DEBUG4( ("ldterm_vmin_timeout returning with a bad q pointer\n"));
	 	return;
	}

	tp = (lstate_t *)q->q_ptr;


	if (!tp) {
		DEBUG4( ("ldterm_vmin_timeout returning with a bad tp pointer\n"));
	 	return;
	}

	tp->t_state &= ~TS_TACT;

	if (CANON_MODE) {
		DEBUG4 (("OOPS: CANON MODE\n"));
		tp->t_state &= ~TS_RTO;
		tp->t_lbolt = 0;
		return;
	}
	/* if VMIN has any value, the timer is to be started after one
	 * char has been received only (if need be). Hence t_msglen
	 * should never be equal to zero here if VMIN > 0.
	 * However, this can happens in some situations like flush
	 * followed by input before the timer expires etc.
	 */

	if ((tp->t_msglen == 0) && (V_MIN > 0)) {
		DEBUG4 (("OOPS: Timer messed up\n"));
		tp->t_state &= ~TS_RTO;
		tp->t_lbolt = 0;
		return;				
	}
	if (!(tp->t_state & TS_RTO )) {
		tp->t_state |= TS_RTO | TS_TACT;
		DEBUG4( ("calling timeout from ldterm_vmin_timeout\n"));
		(void) drv_getparm(LBOLT, &now);
		ticks = (long) (V_TIME * (HZ / 10));
		if (tp->t_lbolt) {
			ticks -= (now - tp->t_lbolt);
		}
		tp->t_tid = itimeout(ldterm_vmin_timeout, (void *)q, ticks, plstr);
		tp->t_lbolt = 0;
		if (tp->t_tid == 0) {
			/*
			 *+ Kernel could not allocate memory for
			 *+ a timeout.  This indicates
			 *+ that there is not enough physical
			 *+ memory on the machine or that memory
			 *+ is being lost by the kernel.
			 */
			cmn_err(CE_WARN,"ldterm: unable to schedule timeout for VTIME processing\n");
			tp->t_state &= ~(TS_RTO | TS_TACT);
		}
		return;
	}
	/* Send RAW blocks off here */
	tp->t_state &= ~TS_RTO;
	tp->t_lbolt = 0;
	ldterm_msg_upstream (q, tp);
	DEBUG4 (("VMIN READY\n"));
}

/*
 * void
 * ldterm_adjust_modes(lstate_t *tp)
 *	Routine to adjust termios flags to be processed by the line discipline.
 *
 * Calling/Exit State:
 *	Called with t_lock held.  On return from this routine, we will have
 *	the following fields set in tp structure -->
 *		tp->t_modes:	modes the line discipline will process
 *		tp->t_amodes: modes the user process thinks the line
 *		discipline is processing.
 *
 * Description:
 *	Driver below sends a termios structure, with the flags the driver
 *	intends to process. XOR'ing the driver sent termios structure with
 *	current termios structure with the default values (or set by ioctls
 *	from userland), we come up with a new termios structrue, the flags
 *	of which will be used by the line discipline in processing input and
 *	output.
 */

STATIC void
ldterm_adjust_modes(lstate_t *tp)
{
	DEBUG6 (("original iflag = %o\n", tp->t_modes.c_iflag)); 
	tp->t_modes.c_iflag = tp->t_amodes.c_iflag & ~(tp->t_dmodes.c_iflag);
	tp->t_modes.c_oflag = tp->t_amodes.c_oflag & ~(tp->t_dmodes.c_oflag);
	tp->t_modes.c_lflag = tp->t_amodes.c_lflag & ~(tp->t_dmodes.c_lflag);
	DEBUG6 (("driver iflag = %o\n", tp->t_dmodes.c_iflag)); 
	DEBUG6 (("apparent iflag = %o\n", tp->t_amodes.c_iflag)); 
	DEBUG6 (("effective iflag = %o\n", tp->t_modes.c_iflag)); 

	/* No negotiation of clfags  c_cc array special characters */
	/* Copy from amodes to modes already done by TCSETA/TCSETS code*/
}

/*
 * void
 * ldterm_euc_erase(queue_t *q, int ebsize, lstate_t *tp)
 *	Erase one EUC SUPPLEMENTARY character.  If TS_MEUC is set AND this is
 *	an EUC character (NOT! an ASCII character!), then this should be called
 *	instead of ldterm_erase.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 *
 * Remarks:
 *	We'd better be pointing to the last byte.  If we aren't, it will get
 *	messed up.
 */

STATIC void
ldterm_euc_erase(queue_t *q, int ebsize, lstate_t *tp)
{
	int i;
	int ung;
	unchar *p;
	unchar *bottom;

	if (tp->t_eucleft) {
		/* What to do now? */
		ldterm_eucwarn(tp);
		return;	/* ignore it */
	}
	bottom = tp->t_eucp_mp->b_rptr;
	p = tp->t_eucp - 1;	/* previous byte */
	ung = 1;	/* number of bytes to un-get from buffer */
	/*
	 * go through the buffer until we find the beginning of the
	 * multi-byte char.
	 */
	while ((*p == 0) && (p > bottom)) {
		p--;
		++ung;
	}
	/*
	 * Now, "ung" is the number of bytes to unget from the buffer and
	 * "*p" is the disp width of it.
	 * Fool "ldterm_rubout" into thinking we're rubbing out ASCII
	 * characters.  Do that for the display width of the character.
	 */
	for (i = 0; i < ung; i++) {	/* remove from buf */
		if (ldterm_unget(tp) != (-1))
			ldterm_trim(tp);
	}
	for (i = 0; i < (int) *p; i++)	/* remove from screen */
		ldterm_rubout(' ', q, ebsize, tp);
	/*
	 * Adjust the parallel array pointer.  Zero out the contents of
	 * parallel array for this position, just to make sure...
	 */
	tp->t_eucp = p;
	*p = 0;
}

/*
 * void
 * ldterm_eucwarn(lstate_t *tp)
 *	This is kind of a safety valve.  Whenever we see a bad sequence come
 *	up, we call eucwarn.  It just tallies the junk until a threshold is
 *	reached.  Then it prints ONE message on the console and not any more.
 *	Hopefully, we can catch garbage; maybe it will be useful to somebody.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 */

STATIC void
ldterm_eucwarn(lstate_t *tp)
{
	++tp->t_eucwarn;
#ifdef DEBUG
	if ((tp->t_eucwarn > EUC_WARNCNT) && !(tp->t_state & TS_WARNED)) {
		/*
		 *+ Debugging information
		 */
		cmn_err(CE_WARN,"ldterm: tty at addr %x in multi-byte mode --\n",tp);
		/*
		 *+ Debugging information
		 */
		cmn_err(CE_WARN, "Over %d bad EUC characters this session\n", EUC_WARNCNT);
		tp->t_state |= TS_WARNED;
	}
#endif
}

/*
 * void
 * cp_eucwoic(eucioc_t *from, eucioc_t *to, int dir)
 *	Copy an "eucioc_t" structure.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 *
 * Description:
 *	We use the structure with incremented values for Codesets 2 & 3.
 *	The specification in eucioctl is that the same values as the CSWIDTH
 *	definition at user level are passed to us.  When we copy it "in" to
 *	ourselves, we do the increment.  That allows us to avoid treating each
 *	character set separately for "t_eucleft" purposes.  When we copy it
 *	"out" to return it to the user, we decrement the values so the user
 *	gets what it expects, and it matches CSWIDTH in the environment
 *	(if things are consistent!).
 */

void
cp_eucwioc(eucioc_t *from, eucioc_t *to, int dir)
{
	bcopy((caddr_t)from, (caddr_t)to, EUCSIZE);
	if (dir == EUCOUT) {	/* copying out to user */
		if (to->eucw[2])
			--to->eucw[2];
		if (to->eucw[3])
			--to->eucw[3];
	}
	else {			/* copying in */
		if (to->eucw[2])
			++to->eucw[2];
		if (to->eucw[3])
			++to->eucw[3];
	}
}

/*
 * int
 * ldterm_memwidth(unchar c, eucioc_t *w)
 *	Take the first byte of an EUC (or an ASCII char) and return its
 *	memory width.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 *
 * Description:
 *	The routine could have been implemented to use only the codeset
 *	number, but that would require the caller to have that value available.
 *	 Perhaps the user doesn't want to make the extra call or keep the
 *	value of codeset around.  Therefore, we use the actual character with
 *	which they're concerned.  This should never be called with anything
 *	but the first byte of an EUC, otherwise it will return a garbage value.
 */

STATIC int
ldterm_memwidth(unchar c, eucioc_t *w)
{
	if (ISASCII(c))
		return 1;
	switch (c) {
		case SS2: return(w->eucw[2]);
		case SS3: return(w->eucw[3]);
		default:  return(w->eucw[1]);
	}
}

/*
 * unsigned char
 * ldterm_dispwidth(unchar c, eucioc_t *w, int mode)
 *	Take the first byte of an EUC (or ASCII) and return the display width.
 *
 * Calling/Exit State:
 *	Called with t_lock held.
 *
 * Description:
 * Since this is intended mostly for multi-byte handling, it returns
 *	EUC_TWIDTH for tabs so they can be differentiated from EUC characters
 *	(assumption: EUC require fewer than 255 columns).  Also, if it's a
 *	backspace and !flag, it returns EUC_BSWIDTH.  Newline & CR also depend
 *	on flag.  This routine SHOULD be cleaner than this, but we have the
 *	situation where we may or may not be counting control characters as
 *	having a column width.  Therefore, the computation of ASCII is pretty
 *	messy.  The caller will be storing the value, and then switching on it
 *	when it's used.  We really should define the EUC_TWIDTH and other
 *	constants in a header so that the routine could be used in other
 *	modules in the kernel.
 */

STATIC unsigned char
ldterm_dispwidth(unchar c, eucioc_t *w, int mode)
{
	if (ISASCII(c)) {
		if (c <= '\037') {
			switch (c) {
			case '\t':	return EUC_TWIDTH;
			case '\b':	return(mode ? 2 : EUC_BSWIDTH);
			case '\n':	return(mode ? 2 : EUC_NLWIDTH);
			case '\r':	return(mode ? 2 : EUC_CRWIDTH);
			default:	return(mode ? 2 : 0);
			}
		}
		return 1;
	}
	switch (c) {
		case SS2: return(w->scrw[2]);
		case SS3: return(w->scrw[3]);
		default:  return(w->scrw[1]);
	}
}
