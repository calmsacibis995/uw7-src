#ident	"@(#)char.c	1.22"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	21Mar97		rodneyh@sco.com
 *	- Changes for ALT lock support.
 *	  Added K_ALK to switch in chr_getkbent.
 *
 */

/*
 * Integrated Workstation Environment (IWE) CHAR module.
 *
 * The CHAR module handles scancode to character set
 * translation and screen-mapping. It also makes the
 * appropriate calls to DOS emulators for each
 * character/mouse input. To support the XWIN server,
 * it records the input events in the event queue. 
 */

#include <io/ansi/at_ansi.h>
#include <io/ascii.h>
#include <io/char/char.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termios.h>
#include <io/ws/chan.h>
#include <io/ws/tcl.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <io/mouse.h>
#include <mem/kmem.h>
#include <proc/proc.h>
#include <proc/tss.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>


#define	CHRHIER		1
#define	CHRPL		plstr

#ifdef DEBUG
STATIC int chr_debug = 0;
#define	DEBUG1(a)	if (chr_debug == 1) printf a
#define	DEBUG2(a)	if (chr_debug >= 2) printf a /* allocations */
#define	DEBUG3(a)	if (chr_debug >= 3) printf a /* M_CTL Stuff */
#define	DEBUG4(a)	if (chr_debug >= 4) printf a /* M_READ Stuff */
#define	DEBUG5(a)	if (chr_debug >= 5) printf a /* M_IOCTL & M_IOCDATA */
#define	DEBUG6(a)	if (chr_debug >= 6) printf a
#else
#define	DEBUG1(a)
#define	DEBUG2(a)
#define	DEBUG3(a)
#define	DEBUG4(a)
#define	DEBUG5(a)
#define	DEBUG6(a)
#endif /* DEBUG */


STATIC int	chr_open(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	chr_close(queue_t *, cred_t *);
STATIC int	chr_read_queue_put(queue_t *, mblk_t *);
STATIC int	chr_read_queue_serv(queue_t *);
STATIC int	chr_write_queue_put(queue_t *, mblk_t *);
STATIC int	chr_read_queue_serv(queue_t *);
STATIC void	chr_proc_r_proto(queue_t *, mblk_t *, charstat_t *);
STATIC void	chr_proc_r_data(queue_t *, mblk_t *, charstat_t *);
STATIC void	chr_scan(charstat_t *, unsigned char, int);
STATIC void	chr_r_charin(charstat_t *, char *, int, int);
STATIC void	chr_iocack(queue_t *, mblk_t *, struct iocblk *, int);
STATIC void	chr_iocnack(queue_t *, mblk_t *, struct iocblk *, int, int);
STATIC void	chr_copyout(queue_t *, mblk_t *, mblk_t *, 
					uint, unsigned long);
STATIC void	chr_copyin(queue_t *, mblk_t *, int, unsigned long);
STATIC void	chr_do_ioctl(queue_t *, mblk_t *, charstat_t *);
STATIC int	chr_is_special(keymap_t *, unchar, unchar);
STATIC ushort	chr_getkbent(charstat_t *, unchar, unchar);
STATIC void	chr_setkbent(charstat_t *, unchar, unchar, ushort);
STATIC void	chr_do_iocdata(queue_t *, mblk_t *, charstat_t *);
STATIC void	chr_proc_w_data(queue_t *, mblk_t *, charstat_t *);
STATIC void	chr_do_xmouse(queue_t *, mblk_t *, charstat_t *);
STATIC void	chr_do_mouseinfo(queue_t *, mblk_t *, charstat_t *);
STATIC void	chr_proc_w_proto(queue_t *, mblk_t *, charstat_t *);
STATIC int	chr_online(queue_t *);
STATIC void	chr_setchanmode(queue_t *, int);
STATIC int	chr_send_pcproto_msg(queue_t *, charstat_t *);
STATIC void	chr_notify_on_read(queue_t *, int);
STATIC int	chr_get_keymap_type(mblk_t *);

extern int	xlate_keymap(keymap_t *, keymap_t *, int);


static struct module_info chr_iinfo = {
	0,
	"char",
	0,
	MAXCHARPSZ,
	1000,
	100
};

static struct qinit chr_rinit = {
	chr_read_queue_put, 
	chr_read_queue_serv, 
	chr_open,
	chr_close,
	NULL,
	&chr_iinfo
};

static struct module_info chr_oinfo = {
	0,
	"char",
	0,
	MAXCHARPSZ,
	1000,
	100
};

static struct qinit chr_winit = {
	chr_write_queue_put,
	NULL,
	chr_open,
	chr_close,
	NULL,
	&chr_oinfo
};

struct streamtab charinfo = {
	&chr_rinit,
	&chr_winit,
	NULL,
	NULL
};

int chardevflag = D_MP;
int chr_maxmouse;

LKINFO_DECL(chr_c_mutex_lkinfo, "CHAR::char_stat mutex lock", 0);


/*
 * STATIC int
 * chr_open(queue_t *, dev_t *, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit
 *	- Return 0 for success, an errno for failure.
 *
 * Description:
 *	Character module open. Allocate state structure and
 *	send pointer to pointer to character map structure
 *	to principal stream below in the form of a M_PCPROTO
 *	message. 
 */
/*ARGSUSED*/
STATIC int
chr_open(queue_t *qp, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	charstat_t	*cp;
	int		error = 0;
	struct mouseinfo *minfop; /* just used for sizing */


	/*
	 * Return immediately if the char_stat data structure has
	 * already been allocated for the stream (not the first open
	 * of the stream).
	 */
	if (qp->q_ptr != NULL)
		return (0);		/* already attached */

	/*
	 * Allocate and initialize state structure. If kmem_zalloc() fails
	 * return ENOMEM.
	 */
	if ((cp = (charstat_t *)kmem_zalloc(
				sizeof(charstat_t), KM_SLEEP)) == NULL) {
		/*
		 *+ There isn't enough memory available to allocate
		 *+ for char_stat structure.
		 */ 
		cmn_err(CE_WARN, 
			"chr_open: open fails, can't allocate state structure");
		return (ENOMEM);
	}

	cp->c_mutex = LOCK_ALLOC(CHRHIER, CHRPL, 
					&chr_c_mutex_lkinfo, KM_NOSLEEP);
	if (!cp->c_mutex) {
		/*
		 *+ There isn't enough memory available to allocate
		 *+ for the char_stat mutex lock.
		 */
		cmn_err(CE_WARN,
			"chr_open: cannot allocate memory for c_mutex");
		return (ENOMEM);
	}

	chr_maxmouse = (1 << (8 * sizeof(minfop->xmotion) - 1)) - 1;

	/*
	 * Set ptrs in queues to point to state structure. 
	 */
	qp->q_ptr = (caddr_t) cp; 
	WR(qp)->q_ptr = (caddr_t) cp;

	/*
	 * Initialize state structure. 
	 */
	cp->c_rqp = qp;
	cp->c_wqp = WR(qp);
	cp->c_wmsg = (mblk_t *) NULL;
	cp->c_rmsg = (mblk_t *) NULL;
	cp->c_state = 0;
	cp->c_map_p = (charmap_t *) NULL;
	cp->c_scrmap_p = NULL;
	cp->c_oldbutton = 0x07;		/* Initially all buttons up */

	error = chr_online(qp);
	if (!error)
		/* switch on the put ans srv routines */
		qprocson(qp);

	return (error);
}


/*
 * STATIC int
 * chr_close(queue_t *, cred_t *)
 *	Release state structure associated with stream 
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
/* ARGSUSED */
STATIC int 
chr_close(queue_t *qp, cred_t *crp)
{
	charstat_t	*cp = (charstat_t *)qp->q_ptr;


	/* Flush the read queue */
	flushq(qp, FLUSHDATA);

	/*
	 * Switch off the put and srv routines to allow for
	 * any further messages.
	 */
	qprocsoff(qp);

	/*
	 * Release the working messages pointed by the 
	 * char_stat structure.
	 */

	/*
	 * Release the working message on the input.
	 */
	if (cp->c_rmsg != (mblk_t *) NULL) {
		freemsg(cp->c_rmsg);
		cp->c_rmsg = (mblk_t *) NULL;
	}

	/*
	 * Release the working message on the output.
	 */
	if (cp->c_wmsg != (mblk_t *) NULL) {
		freemsg(cp->c_wmsg);
		cp->c_wmsg = (mblk_t *) NULL;
	}

	/*
	 * Release the working message holding a response to 
	 * blocked MOUSEIOCREAD ioctl (an ioctl that obtains
	 * the change in mouse status since the last MOUSEIOCREAD
	 * ioctl).
	 */
	if (cp->c_heldmseread != (mblk_t *) NULL) {
		freemsg(cp->c_heldmseread);
		cp->c_heldmseread = (mblk_t *) NULL;
	}

	LOCK_DEALLOC(cp->c_mutex);

	/*
	 * Dump the associated state structure. 
 	 */
	kmem_free(cp, sizeof(charstat_t));

	qp->q_ptr = NULL;

	return (0);
}


/*
 * STATIC int
 * chr_read_queue_put(queue_t *, mblk_t *)
 *	Put procedure for input from driver end of stream (read queue).
 *	Simply enqueue all messages.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
chr_read_queue_put(queue_t *qp, mblk_t *mp)
{
	putq(qp, mp);		/* enqueue this pup */
	return (0);
}


/*
 * STATIC int
 * chr_read_queue_serv(queue_t *)
 *	Read side queued message processing.  
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC int
chr_read_queue_serv(queue_t *qp)
{
	charstat_t	*cp;
	mblk_t		*mp;
	pl_t		pl;


	cp = (charstat_t *) qp->q_ptr;

	/*
	 * Keep getting messages until none left or we honor
	 * flow control and see that the stream above us is blocked
	 * or are set to enqueue messages while an ioctl is processed.
	 */
	while ((mp = getq(qp)) != NULL) {

		if (mp->b_datap->db_type <= QPCTL && !canputnext(qp)) {
			putbq(qp, mp);
			return (0);		/* read side is blocked */
	   	}

		pl = LOCK(cp->c_mutex, CHRPL);

		/*
		 * A check is performed to see if the process that put
		 * the stream in X-queue has died. If it has, the modes
		 * are reset. This is done so that in the event that the
		 * X/windows server, the console character processing
		 * is put back to a sane state.
		 */
		if (cp->c_state & C_XQUEMDE) {
			if (!cp->c_xqinfo ||
				!proc_valid(cp->c_xqinfo->xq_proc)) {
				cp->c_state &= ~C_XQUEMDE;
				cp->c_xqinfo = NULL;
		   	}
		}

		UNLOCK(cp->c_mutex, pl);

		switch(mp->b_datap->db_type) {
		case M_FLUSH:
			/*
		 	 * Flush everything we haven't looked at yet.
		 	 */
			flushq(qp, FLUSHDATA);
			putnext(qp, mp);	/* pass it on */
			continue;

		case M_DATA:
			chr_proc_r_data(qp, mp, cp);
			continue;

		case M_PCPROTO:
		case M_PROTO:
			chr_proc_r_proto(qp, mp, cp);
			continue;

		default:
			putnext(qp, mp);	/* pass it on */
			continue;

		} /* switch */
	} /* while */

	return (0);
}


/*
 * STATIC void
 * chr_proc_r_proto(queue_t *, mblk_t *, charstat_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Remark:
 *	Currently, CHAR understands messages from KDSTR, MOUSE and
 *	MERGE386.
 */
STATIC void
chr_proc_r_proto(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	ch_proto_t	*protop;
	int		i;
	pl_t		pl;


	/*
	 * Check to see if the size of M_PROTO/M_PCPROTO data is
	 * the size of the ch_proto_t structure.
	 */
	if ((mp->b_wptr - mp->b_rptr) != sizeof(ch_proto_t)) {
		putnext(qp, mp);
		return;
	}

	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;

	switch (protop->chp_type) {
	case CH_CTL: {
		/*
		 * The message is from the KD driver below
		 */
		switch (protop->chp_stype) {
		case CH_CHR:
			/*
			 * The message relates to character mapping
			 */
			switch (protop->chp_stype_cmd) {
			case CH_LEDSTATE:
				/*
				 * Reset the LEDs. This message is sent after
				 * a VT switch or the use of the KDSETLED ioctl.
				 */
				pl = LOCK(cp->c_mutex, CHRPL);
				i = cp->c_kbstat.kb_state;
				cp->c_kbstat.kb_state = 
					(i & NONTOGGLES) | protop->chp_stype_arg;
				UNLOCK(cp->c_mutex, pl);
				break;

			case CH_CHRMAP:
				/*
				 * Set the default character maps. This message
				 * is sent by KD in response to the CH_CHROPEN
				 * message sent by the CHAR. Receipt of this
				 * message is crucial because CHAR cannot
				 * process input until it comes upstream.
				 */
				pl = LOCK(cp->c_mutex, CHRPL);
				cp->c_map_p = (charmap_t *)protop->chp_stype_arg;
				UNLOCK(cp->c_mutex, pl);
				break;

			case CH_SCRMAP:
				/*
				 * Set the default screen maps. This message
				 * is sent by KD in response to the CH_CHROPEN
				 * message sent by the CHAR.
				 */
				pl = LOCK(cp->c_mutex, CHRPL);
				cp->c_scrmap_p = (scrn_t *)protop->chp_stype_arg;
				UNLOCK(cp->c_mutex, pl);
				break;
			case CH_KBNONTOG:
				pl = LOCK(cp->c_mutex, CHRPL);
				cp->c_kbstat.kb_state &= ~NONTOGGLES;
				cp->c_kbstat.kb_state |= (ushort)protop->chp_stype_arg;
				UNLOCK(cp->c_mutex, pl);
				break;
#ifdef MERGE386
			case CH_SETMVPI: {
				chr_merge_t *mergep;

				mergep = (mp->b_cont) ? 
					 (chr_merge_t *) mp->b_cont->b_rptr : 
					 (chr_merge_t *) NULL;

				if (mergep == (chr_merge_t *) NULL ||
				   (mp->b_cont->b_wptr - (unchar *)mergep) 
						!= sizeof(chr_merge_t)) {
					break;
				}

				pl = LOCK(cp->c_mutex, CHRPL);
				cp->c_merge_kbd_ppi = mergep->merge_kbd_ppi;
				cp->c_merge_mse_ppi = mergep->merge_mse_ppi;
				cp->c_merge_mcon = mergep->merge_mcon;
				UNLOCK(cp->c_mutex, pl);
				break;
			}

			case CH_DELMVPI: 
				pl = LOCK(cp->c_mutex, CHRPL);
				cp->c_merge_kbd_ppi = NULL;
				cp->c_merge_mse_ppi = NULL;
				cp->c_merge_mcon = NULL;
				UNLOCK(cp->c_mutex, pl);
				break;
#endif /* MERGE386 */
			default:
				putnext(qp, mp);
				return;
			}

			freemsg(mp);
			return; /* case CH_CHR */

		case CH_XQ:
			/*
			 * The message relates to the management of the
			 * X/windows event queue.
			 */

			/*
			 * Set the CHAR in X/queue mode (A pointer to the
			 * xqInfo structure is part of the message)
			 */
			if (protop->chp_stype_cmd == CH_XQENAB) {
				pl = LOCK(cp->c_mutex, CHRPL);
				cp->c_xqinfo = (xqInfo *) protop->chp_stype_arg;
				cp->c_state |= C_XQUEMDE;
				UNLOCK(cp->c_mutex, pl);
				/*
				 * change the command to CH_XQENAB_ACK and
				 * message type to M_PCPROTO and send the
				 * response back downstream
				 */
				protop->chp_stype_cmd = CH_XQENAB_ACK;
				mp->b_datap->db_type = M_PCPROTO;
				qreply(qp, mp);
				chr_notify_on_read(qp, 1);
				return;
			}

			/*
			 * Reset the CHAR out of X/queue mode.
			 */
			if (protop->chp_stype_cmd == CH_XQDISAB) {
				pl = LOCK(cp->c_mutex, CHRPL);
				cp->c_xqinfo = (xqInfo *) NULL;
				cp->c_state &= ~C_XQUEMDE;
				UNLOCK(cp->c_mutex, pl);
				/*
				 * change the command byte to CH_XQDISAB_ACK
				 * and the message type to M_PCPROTO and send
				 * the response back downstream
				 */
				protop->chp_stype_cmd = CH_XQDISAB_ACK;
				mp->b_datap->db_type = M_PCPROTO;
				qreply(qp, mp);
				chr_notify_on_read(qp, 0);
				return;
			}

			putnext(qp, mp);
			return;

		default:
			putnext(qp, mp);
		} 
		break;

	} /* case CH_CTL */

	case CH_DATA:
		/*
		 * The input is either a mouse event or an already
		 * translated data from KD or an auxillary device.
		 */
		switch (protop->chp_stype) {
		case CH_MSE:
			/*
			 * This is a mouse event.
			 */
			
			/*
			 * If the CHAR is in X/queue mode, then call
			 * chr_do_xmouse() which in turn calls xq_enqueue()
			 * to place the event on the X/windows event queue.
			 */ 
			if (cp->c_state & C_XQUEMDE)
				chr_do_xmouse(qp, mp, cp);
#ifdef MERGE386
			else if (cp->c_merge_mse_ppi) {
				/*
				 * call routine supplied by the CH_SETMVPI
				 * message.
				 */
				(*(cp->c_merge_mse_ppi))(
					(struct mse_event *)mp->b_cont->b_rptr,
						cp->c_merge_mcon);
			}
#endif /* MERGE386 */
			/*
			 * Update the mouse information status. 
			 */
			chr_do_mouseinfo(qp, mp, cp);
			freemsg(mp);
			return;

		case CH_NOSCAN:
			/*
			 * Send up b_cont message to LDTERM directly. The
			 * b_cont message already contains translated data.
			 */
			if (mp->b_cont) 
				putnext(qp, mp->b_cont);
			/*
			 * free M_PROTO/M_PCPROTO message
			 */
			freeb(mp);
			return;
	
		default:
			/*
			 *+ Unknown CH_DATA message is found in input.
			 */
			DEBUG1(("chr_proc_r_proto: Found unknown CH_DATA message in input"));
			putnext(qp, mp);
			break;
		}
		return;

	default:
		/*
		 * send the message upstream
		 */
		putnext(qp, mp);
		return;
	}

	return;
}


/*
 * STATIC void
 * chr_proc_r_data(queue_t *, mblk_t *, charstat_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Remark:
 *	Treat each byte of the message as a scan code to be translated. 
 */
STATIC void
chr_proc_r_data(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	mblk_t	*bp;
	int	israw;
	pl_t	pl;


	pl = LOCK(cp->c_mutex, CHRPL);

	/*
	 * If c_map_p in char_stat is NULL, then the CH_CHR/CH_CHRMAP
	 * message from the KD has not yet been received, so free 
	 * the message.
	 */
	if (cp->c_map_p == (charmap_t *) NULL) {
		UNLOCK(cp->c_mutex, pl);
		freemsg(mp);
		return;
	}

	bp = mp;

	/*
	 * <israw> is turned on if in raw mode or X/queue mode.
	 */
	israw = cp->c_state & (C_RAWMODE);
	if (!israw && (cp->c_state & C_XQUEMDE))
                israw = (cp->c_xqinfo->xq_devices & QUE_KEYBOARD) &&
                        (cp->c_xqinfo->xq_xlate == 0);

	UNLOCK(cp->c_mutex, pl);

	/*
	 * For each data block, take the buffered bytes and pass them
	 * to chr_scan; it will translate them and put them in a message
	 * that we send up when when we're through with this message.
	 */
	while (bp) {
		while ((unsigned)bp->b_rptr < (unsigned)bp->b_wptr)
			chr_scan(cp, *bp->b_rptr++, israw);
		bp = bp->b_cont;
	} 

	freemsg(mp);		/* free the scanned message */

	/*
	 * Send up the message we stored at c_rmsg. 
	 */
	if (cp->c_rmsg != (mblk_t *) NULL) {
		putnext(qp, cp->c_rmsg);
		cp->c_rmsg = (mblk_t *) NULL;
	}

	return;
}


/*
 * STATIC void
 * chr_scan(charstat_t *, unsigned char, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	Translate the rawscan code to its mapped counterpart,
 *	using the provided WS function ws_scanchar().
 */
STATIC void
chr_scan(charstat_t *cp, unsigned char rawscan, int israw)
{
	charmap_t	*cmp;		/* char map pointer */
	kbstate_t	*kbstatp;	/* ptr to kboard state */
	ushort		ch;
	int		cnt;
	unchar		*strp, *sp;
	unchar		lbuf[3],	/* Buffer for escape sequences */
			str[4];		/* string buffer */
	pfxstate_t	*pfxstrp;
	strmap_t	*strbufp;
	stridx_t	*strmap_p;
	pl_t		pl;


	cmp = cp->c_map_p;
	kbstatp = &cp->c_kbstat;
	strp = &lbuf[0];
	
	/*
	 * Acquire the charmap lock to protect the
	 * the character mapping table. This lock is
	 * required to be held on entry to ws_scanchar().
	 */
	pl = LOCK(cmp->cr_mutex, plstr);
	pfxstrp = cmp->cr_pfxstrp;
	strbufp = cmp->cr_strbufp;
	strmap_p = cmp->cr_strmap_p;
	/*
	 * Translate the scancode to character and update the
	 * keyboard translation state structure as appropriate.
	 */
	ch = ws_scanchar(cmp, kbstatp, rawscan, israw);
	UNLOCK(cmp->cr_mutex, pl);

	if (ch & NO_CHAR)
		return;

	if (!israw) 
		switch (ch) {
		case K_SLK:	/* CTRL-F generates K_SLK */
			/*
			 * Verify that the translated scancode corressponds
			 * <SCROLL LOCK> or <PAUSE> keys. If so, send up 
			 * either CSTART or CSTOP (the opposite of the 
			 * last start or stop character sent up).
			 *
			 * <SCROLLLOCK> == 0x46 and <PAUSE> == 0x45 
			 */
			if ((rawscan == SCROLLLOCK) || (rawscan == PAUSE)) {
				ch = (cp->c_state & C_FLOWON) ? CSTART: CSTOP;
			}
			break;

		case K_BRK: {
			uchar_t	newscan;
			uchar_t	prev_tmp;
			
			/*
			 * Need to get the scan code after 'esc' 
			 * traslate for use here (and after a01
			 * keyboard translate) Note, we don't want
			 * to loose ws_procscan state so we save 
			 * the prev val.
			 */ 
			pl = LOCK(cmp->cr_mutex, plstr);
			prev_tmp = kbstatp->kb_prevscan;
			newscan = ws_procscan(cmp, kbstatp, rawscan);
			kbstatp->kb_prevscan = prev_tmp;
			UNLOCK(cmp->cr_mutex, pl);

			/*
			 * Verify that the translated scancode corressponds
			 * to the <BREAK> key, and if so, send up a NULL
			 * character followed by a M_BREAK message.
			 *
			 * <BREAK> == 0x46
			 */
			if (newscan == BREAK || ws_specialkey(
					cmp->cr_keymap_p, kbstatp, newscan)) {
				/* flush */
				chr_r_charin(cp, (char *)strp, 0, 1);
				putnextctl(cp->c_rqp, M_BREAK);
				return;
			}
			break;
		}

		default:
			break;
		}

/* 
	strp = &lbuf[0];
*/

	/* just send the event */
	if (cp->c_state & C_XQUEMDE &&
	    cp->c_xqinfo->xq_devices & QUE_KEYBOARD &&
	    cp->c_xqinfo->xq_xlate == 0) {
		lbuf[0] = (uchar_t) ch;
		chr_r_charin(cp, (char *)lbuf, 1, 0);
		return;
	}

	/*
	 * Check to see if return value from ws_scanchar() indicates
	 * that the character should be prefixed with an escape 
	 * sequence or is a function key. If not, call chr_r_charin()
	 * with just the character to send up, otherwise for the
	 * escape sequenced prefixed keys or function keys, call
	 * chr_r_charin() with the appropriate string as an argument.
	 */

	if (ch & GEN_ESCLSB) {		/* Prepend character with "<ESC>["? */
		lbuf[0] = 033;		/* Prepend <ESC> */
		lbuf[1] = '[';		/* Prepend '[' */
		lbuf[2] = (uchar_t) ch;	/* Add character */
		cnt = 3;		/* Three characters in buffer */
	} else if (ch & GEN_ESCN) {	/* Prepend character with "<ESC>N"? */
		lbuf[0] = 033;		/* Prepend <ESC> */
		lbuf[1] = 'N';		/* Prepend 'N' */
		lbuf[2] = (uchar_t) ch;	/* Add character */
		cnt = 3;		/* Three characters in buffer */
	} else if (ch & GEN_ZERO) {	/* Prepend character with 0? */
		lbuf[0] = 0;		/* Prepend 0 */
		lbuf[2] = (uchar_t) ch;	/* Add character */
		cnt = 2;		/* Two characters in buffer */
	} else if (ch & GEN_FUNC) {	/* Function key? */

		if ((int)(ch & 0xff) >= (int)K_PFXF && 
				(ushort)(ch & 0xff) <= (ushort)K_PFXL){
			ushort val;
			struct pfxstate *pfxp;

			str[0] = '\033';
			pfxp = (struct pfxstate *) pfxstrp;
			pfxp += (ch & 0xff) - K_PFXF; 
			val = pfxp->val;

			switch (pfxp->type) {
			case K_ESN:
				str[1] = 'N';
				break;
			case K_ESO:
				str[1] = 'O';
				break;
			case K_ESL:
				str[1] = '[';
				break;
			}
			str[2] = (unchar)val;
			strp = &str[0];
			cnt = 3;
		} else {
			/* Start of string */
			ushort idx, *ptr;

			ptr = (ushort *) strmap_p;

			idx = * (ptr + (ch & 0xff) - K_FUNF);
			strp = ((unchar *) strbufp) + idx;

			/* Count characters in string */
			for (cnt = 0, sp = strp; *sp != '\0'; cnt++, sp++) ;
		}
	} else {			/* Nothing special about character */
		lbuf[0] = (uchar_t) ch;	/* Put character in buffer */
		cnt = 1;		/* Only one character */
	}

	/*
	 * Put characters in data message. 
	 */
	chr_r_charin(cp, (char *)strp, cnt, 0);

	return;
}


/*
 * STATIC void
 * chr_r_charin(charstat_t *, char *, int, int);
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	Stuff the characters pointed to by bufp in the 
 *	message allocated for shipping upstream if 
 *	normal operation. VP/ix and MERGE386 hooks will 
 *	most likely be handled here, too, to some 
 *	extent, as well as the X-queue.
 */
STATIC void
chr_r_charin(charstat_t *cp, char *bufp, int cnt, int flush)
{
	mblk_t	*mp;
	pl_t	pl;


	pl = LOCK(cp->c_mutex, CHRPL);

	if (flush || (cp->c_rmsg == (mblk_t *) NULL) ||
	        (cp->c_rmsg->b_wptr >= (cp->c_rmsg->b_datap->db_lim - cnt))) {

		if (cp->c_rmsg) {
			UNLOCK(cp->c_mutex, pl);
			putnext(cp->c_rqp, cp->c_rmsg);
			pl = LOCK(cp->c_mutex, CHRPL);
		}

		cp->c_rmsg = (mblk_t *) NULL;

		if ((mp = allocb(max(CHARPSZ, cnt), BPRI_MED)) == NULL) {
			/*
			 *+ Not enough memory is available to allocate
			 *+ minimum of CHARPSZ size memory.
			 */
		        cmn_err(CE_WARN, 
				"chr_r_charin: cannot allocate message, dropping input data");
			UNLOCK(cp->c_mutex, pl);
			return;
		}
		cp->c_rmsg = mp;
	} 

	/*
	 * If we're in X/queue mode, just enqueue the event.
	 * Nothing gets sent upstream.
	 */
	if (cp->c_state & C_XQUEMDE && 
	    cp->c_xqinfo->xq_devices & QUE_KEYBOARD) {
		cp->c_xevent.xq_type = XQ_KEY;
		UNLOCK(cp->c_mutex, pl);
		while (cnt-- != 0) {
			cp->c_xevent.xq_code = *bufp++;
			(*cp->c_xqinfo->xq_addevent)(cp->c_xqinfo, &cp->c_xevent); 
		}
		return;
	}
#ifdef MERGE386
	/*
	 * If MERGE/386 is running, then the function passed in the 
	 * CH_CHR/CH_SETMVPI protocol message is called.
	 */
	else if (cp->c_merge_kbd_ppi) {
		while (cnt-- != 0) 
			(*(cp->c_merge_kbd_ppi))(*bufp++, cp->c_merge_mcon);
		UNLOCK(cp->c_mutex, pl);
		return;
	}
#endif /* MERGE386 */

	/*
	 * Add the characters to end of read message.
	 */
	while (cnt-- != 0) 
		*cp->c_rmsg->b_wptr++ = *bufp++;

	UNLOCK(cp->c_mutex, pl);

	return;
}


/*
 * STATIC int 
 * chr_write_queue_put(queue_t *qp, mblk_t *)
 *	Char module output queue put procedure. No messages are
 *	enqueued in this routine.
 * 
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC int 
chr_write_queue_put(queue_t *qp, mblk_t *mp)
{
	charstat_t *cp;


	cp = (charstat_t *) qp->q_ptr;

	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		/*
		 * This is coming from above, so we only handle the write
		 * queue here.  If FLUSHR is set, it will get turned around
		 * at the driver, and the read procedure will see it eventually.
		 */
		if (*mp->b_rptr & FLUSHW)
			flushq(qp, FLUSHDATA);
		putnext(qp, mp);
		break;

	case M_IOCTL:
		chr_do_ioctl(qp, mp, cp);
		break;

	case M_IOCDATA:
		chr_do_iocdata(qp, mp, cp);
		break;

	case M_DATA:
		chr_proc_w_data(qp, mp, cp);
		break;

	case M_PCPROTO:
	case M_PROTO:
		chr_proc_w_proto(qp, mp, cp);
		break;

	case M_READ:
		if (chr_send_pcproto_msg(qp, cp)) {
			freemsg(mp);
			break;
		}

	default:
		putnext(qp, mp);	/* pass it through unmolested */
		break;
	}

	return (0);
}


/*
 * STATIC void
 * chr_iocack(queue_t *, mblk_t *, struct iocblk *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC void
chr_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval)
{
	mblk_t	*tmp;


	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;

	if ((tmp = unlinkb(mp)) != (mblk_t *) NULL)
		freemsg(tmp);

	qreply(qp, mp);
	return;
}


/*
 * STATIC void
 * chr_iocnack(queue_t *, mblk_t *, struct iocblk *, int, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC void
chr_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error, int rval)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp, mp);
}


/*
 * STATIC void
 * chr_copyout(queue_t *, mblk_t *, mblk_t *, uint, unsigned long);
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC void
chr_copyout(queue_t *qp, mblk_t *mp, mblk_t *nmp, 
			uint size, unsigned long state)
{
	struct copyreq	*cqp;
	copy_state_t	*copyp;
	charstat_t	*cp;
	pl_t		pl;


	cp = (charstat_t *) qp->q_ptr;

	pl = LOCK(cp->c_mutex, CHRPL);

	/* LINTED pointer alignment */
	copyp = &(cp->c_copystate);

	/* LINTED pointer alignment */
	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t) *(long *) mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	copyp->cpy_arg = (unsigned long) cqp->cq_addr;
	copyp->cpy_state = state;

	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;

	if (mp->b_cont) 
		freemsg(mp->b_cont);

	mp->b_cont = nmp;

	UNLOCK(cp->c_mutex, pl);

	qreply(qp, mp);
	return;
}


/*
 * STATIC void
 * chr_copyin(queue_t *, mblk_t *, int, unsigned long)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC void
chr_copyin(queue_t *qp, mblk_t *mp, int size, unsigned long state)
{
	struct copyreq	*cqp;
	copy_state_t	*copyp;
	charstat_t	*cp;
	pl_t		pl;


	/* LINTED pointer alignment */
	cp = (charstat_t *) qp->q_ptr;

	pl = LOCK(cp->c_mutex, CHRPL);

	/* LINTED pointer alignment */
	copyp = &cp->c_copystate;

	/* LINTED pointer alignment */
	cqp = (struct copyreq *) mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t) * (long *) mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	copyp->cpy_arg = (unsigned long) cqp->cq_addr;
	copyp->cpy_state = state;
	
	if (mp->b_cont) {
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
	}

	mp->b_datap->db_type = M_COPYIN;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);

	UNLOCK(cp->c_mutex, pl);

	qreply(qp, mp);

	return;
}


/*
 * STATIC void
 * chr_do_ioctl(queue_t *, mblk_t *, charstat_t *)
 * 
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	Called when an M_IOCTL message is seen  on the write
 *	queue; does whatever we're supposed to do with it,
 *	and either replies immediately or passes it to the 
 *	next module down. If it replies immediately then
 *	M_COPYIN/M_COPYOUT messages may by sent up since the 
 *	CHAR only supports transparent ioctls. The return
 *	message is the M_IOCDATA message processed in 
 *	chr_do_iocdata(), continuing the processing of the 
 *	ioctl() system call.
 *
 *	There are several categories of these ioctls
 *		- mouse-related ioctls
 *		- VP/ix hook ioctls
 *		- keyboard mapping ioctls
 *		- function key programming ioctls 
 *		- screen mapping ioctl
 *		- extended key processing
 *		- TIOCSTI ioctl
 */
STATIC void
chr_do_ioctl(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	struct iocblk	*iocp;
	int		transparent;
	mblk_t		*nmp;
	ch_proto_t	*protop;
	pl_t		pl;


	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;
	transparent = (iocp->ioc_count == TRANSPARENT);

	switch (iocp->ioc_cmd) {

	case MOUSEIOCDELAY: {
		/*
		 * Update the char_stat structure to indicate that
		 * MOUSEIOCREAD ioctls should block until a mouse
		 * event comes upstream.
		 */
		pl = LOCK(cp->c_mutex, CHRPL);
		cp->c_state |= C_MSEBLK;
		UNLOCK(cp->c_mutex, pl);
		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case MOUSEIOCNDELAY: {
		/*
		 * Update the char_stat structure to indicate that
		 * MOUSEIOCREAD ioctls should not block, but should
		 * return the mouse status, even if it has not 
		 * changed since the last MOUSEIOCREAD.
		 */
		pl = LOCK(cp->c_mutex, CHRPL);
		cp->c_state &= ~C_MSEBLK;
		UNLOCK(cp->c_mutex, pl);
		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case MOUSEIOCREAD: {
		mblk_t *bp;
		struct mouseinfo *minfop;

		pl = LOCK(cp->c_mutex, CHRPL);

		/*
		 * If the char_stat structure is <mouse input changed>
		 * flag (C_MSEINPUT) is not set and <block MOUSEIOCREAD
		 * ioctls until the mouse state changes> flag is set, 
		 * then assign the c_heldmseread to the M_IOCTL message
		 * and return.
		 */
		if ((!(cp->c_state & C_MSEINPUT)) && (cp->c_state & C_MSEBLK)) {
			cp->c_heldmseread = mp;
			UNLOCK(cp->c_mutex, pl);
			return;
		}

		/*
		 * If the above condition is not true, allocate a data
		 * message of mouseinfo size, copy the state and send
		 * it upstream to copied out. Switch off the C_MSEINPUT 
		 * flag.
		 */

		if ((bp = allocb(sizeof(struct mouseinfo), BPRI_MED)) == NULL) {
			UNLOCK(cp->c_mutex, pl);
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		minfop = &cp->c_mouseinfo;
		bcopy(minfop, bp->b_rptr, sizeof(struct mouseinfo));
		minfop->xmotion = minfop->ymotion = 0;
		minfop->status &= BUTSTATMASK;
		cp->c_state &= ~C_MSEINPUT;
		UNLOCK(cp->c_mutex, pl);

		bp->b_wptr += sizeof(struct mouseinfo);

		if (transparent)
			chr_copyout(qp, mp, bp, sizeof(struct mouseinfo), 0);
		else {
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = sizeof(struct mouseinfo);
			if (mp->b_cont) 
				freemsg(mp->b_cont);
			mp->b_cont = bp;
			qreply(qp, mp);
		}

		break;
	}

	case TIOCSTI: { /* Simulate typing of a character at the terminal. */
		mblk_t	*bp;

		/*
		 * The permission checking has already been done at the stream
		 * head, since it has to be done in the context of the process
		 * doing the call. Special processing was done at STREAM head.
		 */

		if ((bp = allocb(1, BPRI_MED)) != NULL) {
			if ((nmp = allocb(sizeof(ch_proto_t), BPRI_MED)) == NULL) {
				freemsg(bp);
				bp = NULL;
			} else {
				*bp->b_wptr++ = *mp->b_cont->b_rptr++;
				nmp->b_datap->db_type = M_PROTO;
				/* LINTED pointer alignment */
				protop = (ch_proto_t *)nmp->b_rptr;
				nmp->b_wptr += sizeof(ch_proto_t);
				protop->chp_type = CH_DATA;
				protop->chp_stype = CH_NOSCAN;
				nmp->b_cont = bp;
				putq(cp->c_rqp, nmp);
			}
		}

		if (bp)
			chr_iocack(qp, mp, iocp, 0);
		else 
			chr_iocnack(qp, mp, iocp, ENOMEM, 0);
		break;
	}

	case KBENABLED: {
		kbstate_t *kbp;

		if (transparent) {
			kbp = &cp->c_kbstat;
			chr_iocack(qp, mp, iocp, kbp->kb_extkey);
		} else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;
	}

	case TIOCKBOF: {
		kbstate_t *kbp;

		if (transparent) {
			pl = LOCK(cp->c_mutex, CHRPL);
			kbp = &cp->c_kbstat;
			kbp->kb_extkey = 0;
			UNLOCK(cp->c_mutex, pl);
			chr_iocack(qp, mp, iocp, 0);
		} else 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;
	}

	case TIOCKBON: {
		kbstate_t *kbp;

		if (transparent) {
			pl = LOCK(cp->c_mutex, CHRPL);
			kbp = &cp->c_kbstat;
			kbp->kb_extkey = 1;
			UNLOCK(cp->c_mutex, pl);
			chr_iocack(qp, mp, iocp, 0);
		} else 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;
	}

	case KDGKBENT:
		if (transparent)
			chr_copyin(qp, mp, sizeof(struct kbentry), CHR_IN_0);
		else 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;
		
	case KDSKBENT: 
		if (transparent) 
			chr_copyin(qp, mp, sizeof(struct kbentry), CHR_IN_0);
		else 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case KDGKBMODE: {
		unsigned char val;

		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		if ((nmp = allocb(sizeof(val), BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		pl = LOCK(cp->c_mutex, CHRPL);
		val = (cp->c_state & C_RAWMODE) ? K_RAW : K_XLATE;
		UNLOCK(cp->c_mutex, pl);
		bcopy(&val, nmp->b_rptr, sizeof(val));
		nmp->b_wptr += sizeof(val);
		chr_copyout(qp, mp, nmp, sizeof(val), CHR_OUT_0);
		break;
	}
		
	case KDSKBMODE: {
		int arg;

		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		/* LINTED pointer alignment */
		arg = *(int *) mp->b_cont->b_rptr;

		if ((arg != K_RAW) && (arg != K_XLATE)) {
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			break;
		}

		if (arg == K_RAW) {
			pl = LOCK(cp->c_mutex, CHRPL);
			cp->c_state |= C_RAWMODE;
			UNLOCK(cp->c_mutex, pl);
			chr_setchanmode(qp, 1); 
		} else {
			pl = LOCK(cp->c_mutex, CHRPL);
			cp->c_state &= ~C_RAWMODE;
			UNLOCK(cp->c_mutex, pl);
			chr_setchanmode(qp, 0); 
		}

		chr_iocack(qp, mp, iocp, 0);
		break;
	}

	case SETFKEY: 
		if (transparent) 
			chr_copyin(qp, mp, sizeof(struct fkeyarg), CHR_IN_0);
		else 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case GETFKEY: 
		if (transparent)
			chr_copyin(qp, mp, sizeof(struct fkeyarg), CHR_IN_0);
		else 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case KDDFLTKEYMAP: {
		int	type = chr_get_keymap_type(mp);
		charmap_t *cmp;

		DEBUG5(("ioctl KDDFLTKEYMAP type %d\n", type));

		if (!transparent || type < 0) 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		else { 
			cmp = cp->c_map_p;
			pl = LOCK(cmp->cr_mutex, plstr);
			/*
			 * Save the keymap type since we would need it later
			 * to process the ioctl after copyin rquest is
			 * satisfied and M_IOCDATA message type is received.
			 */
			cmp->cr_flags = type;
			UNLOCK(cmp->cr_mutex, pl);
			chr_copyin(qp, mp, sizeof(struct key_dflt), CHR_IN_0);
		}
		break;
	}

	case KDDFLTSTRMAP:
		if (transparent) 
			chr_copyin(qp, mp, sizeof(struct str_dflt), CHR_IN_0);
		else 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case KDDFLTSCRNMAP:
		if (transparent)
			chr_copyin(qp, mp, sizeof(struct scrn_dflt), CHR_IN_0);
		else 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case GIO_KEYMAP: {
		int		size;
		charmap_t	*cmp;
		keymap_t	*kmp;
		int		type = chr_get_keymap_type(mp);

		DEBUG5(("GIO_KEYMAP type %d\n", type));

		if (!transparent || type < 0) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		cmp = cp->c_map_p;

		pl = LOCK(cmp->cr_mutex, plstr);

		ASSERT(cmp->cr_keymap_p->n_keys <= NUM_KEYS);

		size = (type == SCO_FORMAT
			? max(cmp->cr_keymap_p->n_keys, SCO_TABLE_SIZE)
			: max(cmp->cr_keymap_p->n_keys, USL_TABLE_SIZE));
		size *= sizeof(cmp->cr_keymap_p->key[0]);
		size += sizeof(cmp->cr_keymap_p->n_keys);
		DEBUG5(("GIO_KEYMAP: size 0x%x\n", size));
		if ((nmp = allocb(size, BPRI_MED)) == (mblk_t *) NULL) {
			UNLOCK(cmp->cr_mutex, pl);
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		/* LINTED pointer alignment */
		kmp = (keymap_t *) nmp->b_rptr;

		if (type == SCO_FORMAT)
			xlate_keymap(cmp->cr_keymap_p, kmp, type);
		else
			bcopy(cmp->cr_keymap_p, kmp, size);

		nmp->b_wptr += size;

		UNLOCK(cmp->cr_mutex, pl);

		chr_copyout(qp, mp, nmp, size, CHR_OUT_0);
		break;
	}	
	
	case PIO_KEYMAP: {
		int		size, error;
		charmap_t	*cmp;
		int		type = chr_get_keymap_type(mp);

		DEBUG5(("PIO_KEYMAP type %d\n", type));

		if (!transparent || type < 0) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		/*
		 * Check the user credentials since only root users
		 * can change keymap.
		 */
		error = drv_priv(iocp->ioc_cr);
		if (error) {
			chr_iocnack(qp, mp, iocp, error, 0);
			break;
		}

		cmp = cp->c_map_p;
		pl = LOCK(cmp->cr_mutex, plstr);
		/* save the keymap state */
		cmp->cr_flags = type;
		UNLOCK(cmp->cr_mutex, pl);
		size = sizeof(cmp->cr_keymap_p->n_keys);
		chr_copyin(qp, mp, size, CHR_IN_0);
		break;
	}	

	case PIO_SCRNMAP:
		if (transparent) 
			chr_copyin(qp, mp, sizeof(scrnmap_t), CHR_IN_0);
		else 
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case GIO_SCRNMAP: {
		scrnmap_t	*scrp;
		unsigned int	i = 0;

		/*
		 * The GIO_SCRNMAP ioctl copies the working screen map up
		 * to the user (it supplies an identity map if the screen
		 * map pointer is NULL).
		 */

		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		if ((nmp = allocb(sizeof(scrnmap_t), BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		scrp = (scrnmap_t *) NULL;

		if (cp->c_scrmap_p) {
			pl = LOCK(cp->c_scrmap_p->scr_mutex, plstr);
			if ((scrp = cp->c_scrmap_p->scr_map_p) == (scrnmap_t *)NULL)
				scrp = cp->c_scrmap_p->scr_defltp->scr_map_p;
			UNLOCK(cp->c_scrmap_p->scr_mutex, pl);
		}

		if (scrp) {
			pl = LOCK(cp->c_scrmap_p->scr_mutex, plstr);
			bcopy(scrp, nmp->b_rptr, sizeof(scrnmap_t));
			UNLOCK(cp->c_scrmap_p->scr_mutex, pl);
			nmp->b_wptr += sizeof(scrnmap_t);
		} else {
			for (i = 0; i < sizeof(scrnmap_t); i++)
				*nmp->b_wptr++ = (unsigned char) i;
		}

		chr_copyout(qp, mp, nmp, sizeof(scrnmap_t), CHR_OUT_0);
		break;
	}

	case PIO_STRMAP:
		if (transparent)
			chr_copyin(qp, mp, STRTABLN, CHR_IN_0);
		else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case PIO_STRMAP_21:
		if (transparent)
			chr_copyin(qp, mp, STRTABLN_21, CHR_IN_0);
		else
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case GIO_STRMAP:
		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		if ((nmp = allocb(STRTABLN, BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		pl = LOCK(cp->c_map_p->cr_mutex, plstr);
		bcopy(cp->c_map_p->cr_strbufp, nmp->b_rptr, STRTABLN);
		UNLOCK(cp->c_map_p->cr_mutex, pl);
		nmp->b_wptr += STRTABLN;
		chr_copyout(qp, mp, nmp, STRTABLN, CHR_OUT_0);
		break;

	case GIO_STRMAP_21:
		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		if ((nmp = allocb(STRTABLN_21, BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		pl = LOCK(cp->c_map_p->cr_mutex, plstr);
		bcopy(cp->c_map_p->cr_strbufp, nmp->b_rptr, STRTABLN_21);
		UNLOCK(cp->c_map_p->cr_mutex, pl);
		nmp->b_wptr += STRTABLN_21;
		chr_copyout(qp, mp, nmp, STRTABLN_21, CHR_OUT_0);
		break;

	case SETLOCKLOCK: 
		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		/* LINTED pointer alignment */
		if (mp->b_cont && (*(int *)mp->b_cont->b_rptr == 0)) {
			chr_iocack(qp, mp, iocp, 0);
			break;
		}

		chr_iocnack(qp, mp, iocp, EINVAL, 0);
		break;

	case KDGKBSTATE: {
		kbstate_t	*kbp;
		unchar		state = 0;
		mblk_t		*nmp;

		if (!transparent) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			break;
		}

		if ((nmp = allocb(sizeof(state), BPRI_MED)) == NULL) {
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			break;
		}

		pl = LOCK(cp->c_mutex, CHRPL);

		kbp = &cp->c_kbstat;
		if (kbp->kb_state & SHIFTSET)
			state |= SHIFTED;
		if (kbp->kb_state & CTRLSET)
			state |= CTRLED;
		if (kbp->kb_state & ALTSET)
			state |= ALTED;

		UNLOCK(cp->c_mutex, pl);

		bcopy(&state, nmp->b_rptr, sizeof(state));
		nmp->b_wptr += sizeof(state);
		chr_copyout(qp, mp, nmp, sizeof(state), CHR_OUT_0);
		break;
	}

	default:
		putnext(qp, mp);

	} /* end switch */

	return;
}


/*
 * STATIC int
 * chr_is_special(keymap_t *, unchar, unchar)
 *
 * Calling/Exit State:
 *	- charmap basic lock is held across the function.
 */
STATIC int
chr_is_special(keymap_t *kp, unchar idx, unchar table)
{
	return (kp->key[idx].spcl & (0x80 >> table));
}


/*
 * STATIC ushort
 * chr_getkbent(charstat_t *, unchar, unchar)
 *	KDGKBENT ioctl translation function for character mapping tables 
 *
 * Calling/Exit State:
 *	- charmap basic lock is held across the function.
 *	- return the value of the idx (scancode). The value
 *	  is short containing the character corressponding 
 *	  to the scancode in the low byte, and flags indicating
 *	  if the translation is affected by the <NUMLOCK>,
 *	  <CAPSLOCK>, and <CTRL> keys.
 */
STATIC ushort
chr_getkbent(charstat_t *cp, unchar table, unchar idx)
{
	keymap_t	*kp;
	ushort		val, pfx;
	struct pfxstate	*pfxp;


	kp = cp->c_map_p->cr_keymap_p;
	pfxp = (struct pfxstate *) cp->c_map_p->cr_pfxstrp;
	val = kp->key[idx].map[table];

	if (kp->key[idx].flgs & KMF_NLOCK)
		val |= NUMLCK;

	if (kp->key[idx].flgs & KMF_CLOCK)
		val |= CAPLCK;

	if (chr_is_special(kp, idx, table)) {
		if (IS_FUNKEY(val & 0xff) && 
				pfxp[(val & 0xff) - K_PFXF].type != (unchar) 0) {
			pfx = pfxp[(val & 0xff) - K_PFXF].type;	
			val = pfxp[(val & 0xff) - K_PFXF].val;

			switch(pfx) {
			case K_ESN:
				pfx = SS2PFX;
				break;
			case K_ESO:
				pfx = SS3PFX;
				break;
			case K_ESL:
				pfx = CSIPFX;
				break;
			}
			return(pfx | (unchar) val);
		}

		switch (val & 0xff) {
		case K_NOP:
			return NOKEY;
		case K_BRK:
			return BREAKKEY;
		case K_LSH:
		case K_RSH:
		case K_CLK:
		case K_NLK:
		case K_ALT:
		case K_CTL:
		case K_LAL:
		case K_RAL:
		case K_LCT:
		case K_RCT:
		case K_ALK:				/* L000 */
			return (val | SHIFTKEY);
		case K_ESN: {
/* 			ushort rv;
			rv = kp->key[idx].map[table ^ ALTED] | SS2PFX 
					| (val & 0xff00);
*/
			return (kp->key[idx].map[table ^ ALTED] | 
					SS2PFX | (val & 0xff00));
/* 			return rv; */
		}
		case K_ESO:
			return (kp->key[idx].map[table ^ ALTED] 
					| SS3PFX | (val & 0xff00));
		case K_ESL:
			return (kp->key[idx].map[table ^ ALTED] 
					| CSIPFX | (val & 0xff00));
		case K_BTAB:
			return ('Z' | CSIPFX);
		default: {
			return (val | SPECIALKEY);
		}
		}
	}

	if (kp->key[idx].map[table | CTRLED] & 0x1f)
		val |= CTLKEY;

	return val;
}


/*
 * STATIC void
 * chr_setkbent(charstat_t *, unchar, unchar, ushort)
 *	KDSKBENT ioctl translation function for character mapping tables 
 *
 * Calling/Exit State:
 *	- charmap basic is held across the function.
 */
STATIC void
chr_setkbent(charstat_t *cp, unchar table, unchar idx, ushort val)
{
	int		special = 0; 
	int		smask, pfx;
	struct pfxstate *pfxp;
	keymap_t	*kp;


	kp = cp->c_map_p->cr_keymap_p;
	pfxp = (struct pfxstate *) cp->c_map_p->cr_pfxstrp;

	if ((val & TYPEMASK) == SHIFTKEY)
		return;

	if ((val & TYPEMASK) != NORMKEY)
		val &= ~CTLKEY;

	if (chr_is_special(kp, idx, table)) {
		int old_val = kp->key[idx].map[table];

		if (IS_FUNKEY(old_val) && (old_val > K_PFXF) && 
				pfxp[old_val - K_PFXF].type != 0) {
			pfxp[old_val - K_PFXF].val = 0;
			pfxp[old_val - K_PFXF].type = 0;
		}
	}

	kp->key[idx].flgs = 0;

	/*
	 * If the upper byte of the value that is to be set in the
	 * table indicates that the key is affected by <NUMLCK>
	 * then the corressponding flags in the keyboard map entry
	 * for that index are turned on.
	 */
	if (val & NUMLCK)
		kp->key[idx].flgs |= KMF_NLOCK;

	/*
	 * If the upper byte of the value that is to be set in the
	 * table indicates that the key is affected by <CAPLCK>
	 * then the corressponding flags in the keyboard map entry
	 * for that index are turned on.
	 */
	if (val & CAPLCK)
		kp->key[idx].flgs |= KMF_CLOCK;

	smask = (0x80 >> table) + (0x80 >> (table | CTRLED));

	switch (val & TYPEMASK) {
	case BREAKKEY:
		special = smask;	
		val = K_BRK;
		break;
	case NORMKEY:
		break;
	case SPECIALKEY:
		special = smask;
		break;
	default:
		special = smask;
		val = K_NOP;
		break;
	case SS2PFX:
		pfx = K_ESN;
		goto prefix;
	case SS3PFX:
		pfx = K_ESO;
		goto prefix;
	case CSIPFX:
		if ((val & 0xff) == 'Z') {
			special = smask;
			val = K_BTAB;
			break;
		}
		pfx = K_ESL;
prefix:
		special = smask;
		if ((val & 0xff) == kp->key[idx].map[table ^ ALTED])
			val = (ushort_t) pfx;
		else {
			int	keynum;

			for (keynum = 0; keynum < (K_PFXL - K_PFXF); keynum++) {
				if (pfxp[keynum].type == 0)
					break;
			}

			if (keynum < K_PFXL - K_PFXF) {
			 	pfxp[keynum].val = val & 0xff;
				pfxp[keynum].type = (uchar_t) pfx;
				val = K_PFXF + keynum;
			} else
				val = K_NOP;
		}
		break;

	} /* end switch */

	kp->key[idx].map[table] = (unchar)val;
	kp->key[idx].map[table | CTRLED] = (val & CTLKEY) ? (val & 0x1f) : val;
	kp->key[idx].spcl = (kp->key[idx].spcl & ~smask) | special;
}


/*
 * STATIC void
 * chr_do_iocdata(queue_t *, mblk_t *, charstat_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	This routine handles the M_IOCDATA message sent
 *	by the stream head in response to the M_COPYIN/
 *	M_COPYOUT message sent by the CHAR (chr_do_ioctl). 
 *	This infact continues the processing of the ioctl()
 *	system call as the CHAR module only supports
 *	transparent ioctls.
 *
 *	There are several categories of these ioctls:
 *		- mouse-related ioctls
 *		- VP/ix hook ioctls
 *		- keyboard mapping ioctls
 *		- function key programming ioctls 
 *		- screen mapping ioctl
 *		- extended key processing
 *		- TIOCSTI ioctl
 */
STATIC void
chr_do_iocdata(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	struct iocblk	*iocp;
	struct copyresp		*csp;
	struct copyreq		*cqp;
	copy_state_t		*copyp;
	int			error;
	mblk_t			*nmp, *omp;
	pl_t			pl;


	/* LINTED pointer alignment */
	iocp = (struct iocblk *)mp->b_rptr;
	/* LINTED pointer alignment */
	csp = (struct copyresp *)mp->b_rptr;
	/* LINTED pointer alignment */
	copyp = (copy_state_t *)csp->cp_private;


	switch (iocp->ioc_cmd) {

	case GIO_SCRNMAP: /* and other M_COPYOUT ioctl types */
	case GIO_KEYMAP:
	case GIO_STRMAP:
	case GIO_STRMAP_21:
	case KDGKBMODE:	
	case KDGKBSTATE:
	case MOUSEIOCREAD:
		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		chr_iocack(qp, mp, iocp, 0);
		break;

	case GETFKEY: {
		struct fkeyarg *fp;
		unchar *charp;
		ushort *idxp;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if (copyp->cpy_state == CHR_OUT_0) {
			chr_iocack(qp, mp, iocp, 0);
			return;
		}

		if ((nmp = msgpullup(mp->b_cont, sizeof(struct fkeyarg))) == 0) {
			/*
			 *+ The message pull up of GETFKEY ioctl data 
			 *+ failed.
			 */
			cmn_err(CE_WARN,
				"chr_do_iocdata: pull up of GETFKEY ioctl data failed");

			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		/* LINTED pointer alignment */
		fp = (struct fkeyarg *) mp->b_cont->b_rptr;
		if ((int) fp->keynum < 1 || (int) fp->keynum > NSTRKEYS) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}

		pl = LOCK(cp->c_map_p->cr_mutex, plstr);

		idxp = (ushort *) cp->c_map_p->cr_strmap_p;
		idxp += fp->keynum - 1;
		fp->flen = 0;
		charp = (unchar *) cp->c_map_p->cr_strbufp + *idxp;

		while ( fp->flen < MAXFK && *charp != '\0') 
		   fp->keydef[fp->flen++] = *charp++;

		UNLOCK(cp->c_map_p->cr_mutex, pl);

		/*
		 * now copyout fkeyarg back up to user 
		 */
		/* LINTED pointer alignment */
		cqp = (struct copyreq *) mp->b_rptr;
		cqp->cq_size = sizeof(struct fkeyarg);
		cqp->cq_addr = (caddr_t) copyp->cpy_arg;
		cqp->cq_flag = 0;
		cqp->cq_private = (mblk_t *) copyp;
		copyp->cpy_state = CHR_OUT_0;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		mp->b_datap->db_type = M_COPYOUT;
		qreply(qp, mp);
		return;

	} /* GETFKEY */

	case SETFKEY: {
		struct fkeyarg	*fp;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if ((nmp = msgpullup(mp->b_cont, sizeof(struct fkeyarg))) == 0) {
			/*
			 *+ The message pull of SETFKEY ioctl data 
			 *+ failed.
			 */
			cmn_err(CE_WARN, 
				"chr_do_iocdata: pull up of SETFKEY ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		/* LINTED pointer alignment */
		fp = (struct fkeyarg *) mp->b_cont->b_rptr;
		if ((int)fp->keynum < 1 || (int)fp->keynum > NSTRKEYS) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}

		fp->keynum -= 1; /* ws_addstring assumes 0..NSTRKEYS-1 range */

		if (!ws_addstring(cp->c_map_p, fp->keynum, 
					fp->keydef, fp->flen)) {
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}

		chr_iocack(qp, mp, iocp, 0);
		return;

	} /* SETFKEY */

	case PIO_STRMAP_21:
	case PIO_STRMAP: {
		unsigned long	size;

		size = (iocp->ioc_cmd == PIO_STRMAP) ? STRTABLN : STRTABLN_21;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if ((nmp = msgpullup(mp->b_cont, size)) == 0) {
			/*
			 *+ The message pull of PIO_STRMAP ioctl data
			 *+ failed.
			 */
			cmn_err(CE_WARN, 
				"chr_do_iocdata: pull up of PIO_STRMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		if (!ws_newstrbuf(cp->c_map_p, KM_NOSLEEP, plstr)) {
			chr_iocnack(qp, mp, iocp, ENOMEM, 0);
			return;
		}

		pl = LOCK(cp->c_map_p->cr_mutex, plstr);
		bcopy(mp->b_cont->b_rptr, cp->c_map_p->cr_strbufp, size);
		UNLOCK(cp->c_map_p->cr_mutex, pl);
		(void) ws_strreset(cp->c_map_p);
		chr_iocack(qp, mp, iocp,0);
		break;

	} /* end PIO_STRMAP */	

	case PIO_SCRNMAP: {
		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if ((nmp = msgpullup(mp->b_cont, sizeof(scrnmap_t))) == 0) {
			/*
			 *+ The message pull of PIO_SCRNMAP ioctl data
			 *+ failed.
			 */
			cmn_err(CE_WARN,
				"chr_do_iocdata: pull up of PIO_SCRNMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		if (!ws_newscrmap(cp->c_scrmap_p, KM_NOSLEEP)) {
			chr_iocnack(qp, mp, iocp, ENOMEM, 0);
			return;
		}

		pl = LOCK(cp->c_scrmap_p->scr_mutex, plstr);
		bcopy(mp->b_cont->b_rptr, cp->c_scrmap_p->scr_map_p, 
					sizeof(scrnmap_t));
		UNLOCK(cp->c_scrmap_p->scr_mutex, pl);

		chr_iocack(qp, mp, iocp, 0);
		break;

	} /* end PIO_SCRNMAP */

	case PIO_KEYMAP: {
		short		numkeys;
		uint		size;
		charmap_t	*cmp;

		cmp = cp->c_map_p;

		/*
		 * If the response to M_COPYIN/M_COPYOUT request is not
		 * successful, then free the message.
		 */
		if (csp->cp_rval) {
			freemsg(mp);
			cmp->cr_flags = USL_FORMAT;
	 		return;
		}

		if (copyp->cpy_state == CHR_IN_0) {
			/* LINTED pointer alignment */
			numkeys = *(short *) mp->b_cont->b_rptr;
			numkeys = min(numkeys, NUM_KEYS);
			freemsg(mp->b_cont);
			mp->b_cont = NULL;

			/* calculate the size of copyin */
			size = numkeys * sizeof(cmp->cr_keymap_p->key[0]);
			size += sizeof(cmp->cr_keymap_p->n_keys);
			DEBUG5(("chr_do_iocdata: PIO_KEYMAP(CHR_IN_0): size = 0x%x\n", size));

			/* LINTED pointer alignment */
			cqp = (struct copyreq *) mp->b_rptr;
			cqp->cq_size = size;
			cqp->cq_addr = (caddr_t) copyp->cpy_arg;
			cqp->cq_flag = 0;
			cqp->cq_private = (mblk_t *) copyp;
			copyp->cpy_arg = numkeys;
			copyp->cpy_state = CHR_IN_1;
			mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
			mp->b_datap->db_type = M_COPYIN;
			qreply(qp, mp);
			return;
	   	}

		ASSERT(copyp->cpy_state == CHR_IN_1);

		/* calculate the size of data requesed */
		numkeys = copyp->cpy_arg;
		size = numkeys * sizeof(cmp->cr_keymap_p->key[0]) + sizeof(numkeys);
		DEBUG5(("chr_do_iocdata: PIO_KEYMAP(CHR_IN_1): size = 0x%x\n", size));

		if ((nmp = msgpullup(mp->b_cont, size)) == 0) {
			/*
			 *+ The message pull of PIO_KEYMAP ioctl data
			 *+ failed.
			 */
			cmn_err(CE_WARN,
				"chr_do_iocdata: pull up of PIO_KEYMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			cmp->cr_flags = USL_FORMAT;
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		DEBUG5(("INSTALLING new map of type %s",
			cmp->cr_flags == SCO_FORMAT ? "SCO FORMAT" : "USL_FORMAT"));

		/*
		 * If the current keymap in use is the default,
		 * ws_newkeymap() allocates a new keymap_t structure
		 * and copies the user keymap_t structure used for
		 * translating. Otherwise, it simply overlays the
		 * existing keymap with the user keymap.
		 *
		 * Note: ws_newkeymap() function resets the keymap type 
		 * to USL_FORMAT after translating sco keys to usl keys.
		 */
		/* LINTED pointer alignment */
		if (!ws_newkeymap(cmp, numkeys, (keymap_t *)mp->b_cont->b_rptr,
				KM_NOSLEEP, plstr)) {
			/*
			 *+ The PIO_KEYMAP ioctl is unsuccessful as 
			 *+ ws_newkeymap() is unable to allocate memory 
			 *+ for new keymap.
			 */ 
			cmn_err(CE_WARN,
				"chr_do_iocdata: PIO_KEYMAP: could not allocate new keymap");
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			cmp->cr_flags = USL_FORMAT;
			return;
		}

		chr_iocack(qp, mp, iocp, 0);
		break;

	} /* end PIO_KEYMAP */

	case KDDFLTSTRMAP: {
		charmap_t	*cmp;
		struct str_dflt	*kp;

		if (csp->cp_rval) {
			freemsg(mp);
	 		return;
		}

		if (copyp->cpy_state == CHR_OUT_0) {
			chr_iocack(qp, mp, iocp,0);
			return;
		}

		if ((nmp = msgpullup(mp->b_cont, sizeof(struct str_dflt))) == 0) {
			/*
			 *+ The message pull up of KDDFLTSTRMAP ioctl data
			 *+ failed.
			 */
			cmn_err(CE_WARN,
				"chr_do_iocdata: pull up of KDDFLTSTRMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		cmp = cp->c_map_p;

		pl = LOCK(cmp->cr_mutex, plstr);
	
		/* LINTED pointer alignment */
		kp = (struct str_dflt *) mp->b_cont->b_rptr;

		if (kp->str_direction == KD_DFLTGET) {
			/* LINTED pointer alignment */
			cqp = (struct copyreq *) mp->b_rptr;
			cqp->cq_size = sizeof(struct str_dflt);
			cqp->cq_addr = (caddr_t) cp->c_copystate.cpy_arg;
			cqp->cq_flag = 0;
			cqp->cq_private = (mblk_t *) copyp;
			copyp->cpy_state = CHR_OUT_0;
			mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
			mp->b_datap->db_type = M_COPYOUT;
			bcopy(cmp->cr_defltp->cr_strbufp, 
					&kp->str_map, sizeof(strmap_t));
			UNLOCK(cmp->cr_mutex, pl);
			qreply(qp, mp);
			return;
		}

		if (kp->str_direction != KD_DFLTSET) {
			UNLOCK(cmp->cr_mutex, pl);
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}

		error = drv_priv(iocp->ioc_cr);
		if (error) {
			UNLOCK(cmp->cr_mutex, pl);
			chr_iocnack(qp, mp, iocp, error, 0);
			break;
		}

		bcopy(&kp->str_map, cmp->cr_defltp->cr_strbufp, 
					sizeof(strmap_t));

		/*
		 * Release the lock since ws_strreset() is called with
		 * charmap lock unheld.
		 */
		UNLOCK(cmp->cr_mutex, pl);

		ws_strreset(cmp->cr_defltp);

		chr_iocack(qp, mp, iocp, 0);
		break;

	} /* end KEDFLTSTRMAP */

	case KDDFLTKEYMAP: {
		charmap_t	*cmp;
		struct key_dflt	*kp;
		int		type;

		/*
		 * This ioctl handles the request to overwrite the
		 * default keymap or read the current default keymap
		 * All virtual terminals (VTs) using the default
		 * keymap will pick up the changes. VTs on which the
		 * per-VT keymap has been changed are not affected.
		 */

		if (csp->cp_rval) {
			freemsg(mp);
	 		return;
		}

		if (copyp->cpy_state == CHR_OUT_0) {
			chr_iocack(qp, mp, iocp,0);
			return;
		}

		if ((nmp = msgpullup(mp->b_cont, sizeof(struct key_dflt))) == 0) {
			/*
			 *+ The message pull of KDDFLTKEYMAP ioctl data
			 *+ failed.
			 */
			cmn_err(CE_WARN,
				"chr_do_iocdata: pull up of KDDFLTKEYMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		cmp = cp->c_map_p;

		pl = LOCK(cmp->cr_mutex, plstr);

		type = cmp->cr_flags;
		/* restore the keymap type to before copyin */
		cmp->cr_flags = USL_FORMAT;

		/* LINTED pointer alignment */
		kp = (struct key_dflt *) mp->b_cont->b_rptr;

		/*
		 * Copy the map out to the user and return.
		 */ 
		if (kp->key_direction == KD_DFLTGET) {
			/* LINTED pointer alignment */
			cqp = (struct copyreq *) mp->b_rptr;
			cqp->cq_size = sizeof(struct key_dflt);
			cqp->cq_addr = (caddr_t) cp->c_copystate.cpy_arg;
			cqp->cq_flag = 0;
			cqp->cq_private = (mblk_t *) copyp;
			copyp->cpy_state = CHR_OUT_0;
			mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
			mp->b_datap->db_type = M_COPYOUT;
			DEBUG5(("KDDFLTKEYMAP get map of type %s\n",
				type == SCO_FORMAT ? "SCO FORMAT" : "USL FORMAT"));
			ASSERT(getpl() == plstr);
			if (type == SCO_FORMAT)
				xlate_keymap(cmp->cr_defltp->cr_keymap_p,
						&kp->key_map, type);
			else
				bcopy(cmp->cr_defltp->cr_keymap_p,
						&kp->key_map, sizeof(keymap_t));
			UNLOCK(cmp->cr_mutex, pl);
			qreply(qp, mp);
			return;
		}

		if (kp->key_direction != KD_DFLTSET) {
			UNLOCK(cmp->cr_mutex, pl);
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}

		/*
		 * Check the credentials, since the root can only
		 * change the default keymap.
		 */
		error = drv_priv(iocp->ioc_cr);
		if (error) {
			UNLOCK(cmp->cr_mutex, pl);
			chr_iocnack(qp, mp, iocp, error, 0);
			break;
		}

		/*
		 * Overlay the default keymap with the new keymap
		 * (part of the key_dflt structure).
		 */

		DEBUG5(("KDFFLTKEYMAP set map of type %s",
			type == SCO_FORMAT ? "SCO FORMAT" : "USL FORMAT"));

		ASSERT(getpl() == plstr);

		if (type == SCO_FORMAT) {
			/*
			 * Installing a SCO_FORMAT keymap table type, however
			 * it is stored in USL_FORMAT.
			 */
			xlate_keymap(&kp->key_map, 
				cmp->cr_defltp->cr_keymap_p, USL_FORMAT);
#ifdef DEBUG
			cmn_err(CE_NOTE, "SCO default keymap installed");
#endif /* DEBUG */
		} else {
			bcopy(&kp->key_map, cmp->cr_defltp->cr_keymap_p,
				sizeof(keymap_t));
		}

		UNLOCK(cmp->cr_mutex, pl);

		chr_iocack(qp, mp, iocp, 0);
		break;

	} /* end KDDFLTKEYMAP */

	case KDDFLTSCRNMAP: {
		scrn_t		*scrp;
		int		i;
		char		*c;
		struct scrn_dflt *kp;

		/*
		 * The KDDFLTSCRNMAP is used to read the default screen map.
		 * It copies out the default screen map or copies out the an
		 * identity map if the default screen map is NULL. Otherwise,
		 * it copies in the user-supplied screen map map, allocates
		 * space for a screen map if the default had been NULL, or
		 * overwrites the current default screen map with the user-
		 * supplied map.
		 */

		if (csp->cp_rval) {
			freemsg(mp);
	 		return;
		}

		if (copyp->cpy_state == CHR_OUT_0) {
			chr_iocack(qp, mp, iocp,0);
			return;
		}

		if ((scrp = cp->c_scrmap_p) == (scrn_t *) NULL) {
			chr_iocnack(qp, mp, iocp, EACCES, 0);
			return;
		}

		if ((nmp = msgpullup(mp->b_cont, sizeof(struct scrn_dflt))) == 0) {
			/*
			 *+ The message pull of KDDFLTSCRNMAP ioctl data
			 *+ failed.
			 */
			cmn_err(CE_WARN,
				"chr_do_iocdata: pull up of KDDFLTSCRNMAP ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		pl = LOCK(scrp->scr_mutex, plstr);

		/* LINTED pointer alignment */
		kp = (struct scrn_dflt *) mp->b_cont->b_rptr;

		if (kp->scrn_direction == KD_DFLTGET) {
			/* LINTED pointer alignment */
			cqp = (struct copyreq *) mp->b_rptr;
			cqp->cq_size = sizeof(struct scrn_dflt);
			cqp->cq_addr = (caddr_t) cp->c_copystate.cpy_arg;
			cqp->cq_flag = 0;
			cqp->cq_private = (mblk_t *) copyp;
			copyp->cpy_state = CHR_OUT_0;
			mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
			mp->b_datap->db_type = M_COPYOUT;

			if (scrp->scr_defltp->scr_map_p) {
				bcopy(scrp->scr_defltp->scr_map_p,
					&kp->scrn_map, sizeof(scrnmap_t));
			} else {
				c = (char *) &kp->scrn_map;
				for (i = 0; i < sizeof(scrnmap_t); *c++ = i++) ;
			}

			UNLOCK(scrp->scr_mutex, pl);
			qreply(qp, mp);
			return;
		}

		if (kp->scrn_direction != KD_DFLTSET) {
			UNLOCK(scrp->scr_mutex, pl);
			chr_iocnack(qp, mp, iocp, EINVAL, 0);
			return;
		}

		error = drv_priv(iocp->ioc_cr);
		if (error) {
			UNLOCK(scrp->scr_mutex, pl);
			chr_iocnack(qp, mp, iocp, error, 0);
			break;
		}

		if (!scrp->scr_defltp->scr_map_p) {
			UNLOCK(scrp->scr_mutex, pl);
			if (!ws_newscrmap(scrp->scr_defltp, KM_NOSLEEP)) {
				chr_iocnack(qp, mp, iocp, ENOMEM, 0);
				return;
			}
			pl = LOCK(scrp->scr_mutex, plstr);
		}

		bcopy(&kp->scrn_map, scrp->scr_defltp->scr_map_p, 
					sizeof(scrnmap_t));

		UNLOCK(scrp->scr_mutex, pl);

		chr_iocack(qp, mp, iocp, 0);
		break;

	} /* end KDDFLTSCRNMAP */

	case KDGKBENT: {
		struct kbentry	*kbep;
		charmap_t	*cmp;


		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if (copyp->cpy_state == CHR_OUT_0) {
			chr_iocack(qp, mp, iocp,0);
			return;
		}

		if ((nmp = msgpullup(mp->b_cont, sizeof(struct kbentry))) == 0) {
			/*
			 *+ The message pull of KDGKBENT ioctl data
			 *+ failed.
			 */
			cmn_err(CE_WARN,
				"chr_do_iocdata: pull up of KDGKBENT ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		/* LINTED pointer alignment */
		kbep = (struct kbentry *) mp->b_cont->b_rptr;

		/*
		 * Verify that the requested key index (corressponds
		 * to a scan code) is within range.
		 */
		if (kbep->kb_index >= 128) {
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			return;
		}

		cmp = cp->c_map_p;

		pl = LOCK(cp->c_map_p->cr_mutex, plstr);

		switch (kbep->kb_table) {
		case K_NORMTAB:
			kbep->kb_value = chr_getkbent(cp, NORMAL, 
							kbep->kb_index);
			break;
		case K_SHIFTTAB:
			kbep->kb_value = chr_getkbent(cp, SHIFT, 
							kbep->kb_index);
			break;
		case K_ALTTAB:
			kbep->kb_value = chr_getkbent(cp, ALT, 
							kbep->kb_index);
			break;
		case K_ALTSHIFTTAB:
			kbep->kb_value = chr_getkbent(cp, ALTSHF, 
							kbep->kb_index);
			break;
		case K_SRQTAB:
			kbep->kb_value = *((unchar *)cmp->cr_srqtabp + 
							kbep->kb_index);
			break;
		default:
			UNLOCK(cp->c_map_p->cr_mutex, pl);
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			return;
		}

		UNLOCK(cp->c_map_p->cr_mutex, pl);

		/* LINTED pointer alignment */
		cqp = (struct copyreq *) mp->b_rptr;
		cqp->cq_size = sizeof(*kbep);
		cqp->cq_addr = (caddr_t) cp->c_copystate.cpy_arg;
		cqp->cq_flag = 0;
		cqp->cq_private = (mblk_t *) copyp;
		copyp->cpy_state = CHR_OUT_0;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		mp->b_datap->db_type = M_COPYOUT;
		qreply(qp, mp);
		return;

	} /* end KDGKBENT */

	case KDSKBENT: {
		struct kbentry	*kbep;
		charmap_t	*cmp;
		ushort		numkeys;

		if (csp->cp_rval) {
			freemsg(mp);
			return;
		}

		if ((nmp = msgpullup(mp->b_cont, sizeof(struct kbentry))) == 0) {
			/*
			 *+ The message pull up of KDSKBENT ioctl data
			 *+ failed.
			 */
			cmn_err(CE_WARN,
				"chr_do_iocdata: pull up of KDSKBENT ioctl data failed");
			chr_iocnack(qp, mp, iocp, EFAULT, 0);
			return;
		}
		omp = mp->b_cont;
		mp->b_cont = nmp;
		freemsg(omp);

		/* LINTED pointer alignment */
		kbep = (struct kbentry *) mp->b_cont->b_rptr;

		/*
		 * Verify that the table index is valid.
		 */
		if (kbep->kb_index >= 128) {
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			return;
		}
		
		cmp = cp->c_map_p;
		numkeys = cmp->cr_keymap_p->n_keys;

		/*
		 * If the current keymap in use is the default,
		 * ws_newkeymap() allocates a new keymap_t structure
		 * and copies the user keymap_t structure used for
		 * translating. Otherwise, it simply overlays the
		 * existing keymap with the user keymap.
		 */
		if (!ws_newkeymap(cmp, numkeys, cmp->cr_keymap_p, 
				KM_NOSLEEP, plstr)) {
			/*
			 *+ The KDSKBENT ioctl is unsuccessful as 
			 *+ ws_newkeymap() is unable to allocate memory 
			 *+ for new keymap.
			 */ 
			cmn_err(CE_WARN, 
				"chr_do_iocdata: could not allocate new keymap");
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			return;
		}

		if (!ws_newpfxstr(cmp, KM_NOSLEEP, plstr)) {
			/*
			 *+ The KDSKBENT ioctl is unsuccessful as 
			 *+ ws_newpfxstr() is unable to allocate memory 
			 *+ for new pfxstr.
			 */ 
			cmn_err(CE_WARN, 
				"chr_do_iocdata: could not allocate new pfxstr");
			chr_iocnack(qp, mp, iocp, EAGAIN, 0);
			return;
		}

		/*
		 * Acquire the charmap basic mutex spin lock.
		 */
		pl = LOCK(cmp->cr_mutex, plstr);

		switch (kbep->kb_table) {
		case K_NORMTAB:
			chr_setkbent(cp, NORMAL, kbep->kb_index, 
						kbep->kb_value);
			break;
		case K_SHIFTTAB:
			chr_setkbent(cp, SHIFT, kbep->kb_index, 
						kbep->kb_value);
			break;
		case K_ALTTAB:
			chr_setkbent(cp, ALT, kbep->kb_index, 
						kbep->kb_value);
			break;
		case K_ALTSHIFTTAB:
			chr_setkbent(cp, ALTSHF, kbep->kb_index, 
						kbep->kb_value);
			break;
		case K_SRQTAB: {
			unchar *c;

			c = (unchar *)cmp->cr_srqtabp;
			*(c + kbep->kb_index) = kbep->kb_value;
			break;
		}
		default:
			UNLOCK(cmp->cr_mutex, pl);
			chr_iocnack(qp, mp, iocp, ENXIO, 0);
			return;
		}

		UNLOCK(cmp->cr_mutex, pl);
		chr_iocack(qp, mp, iocp, 0);
		break;

	} /* KDSKBENT case */

	default:
		putnext(qp, mp); /* not for us */
		break;

	} /* end switch */

	return;
}


/*
 * STATIC void
 * chr_proc_w_data(queue_t *, mblk_t *, charstat_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	It translates each data byte using the working XENIX screen map.
 *	If this is NULL, then use the default screen map pointer. If this
 *	too is NULL, then pass the data through unmolested. Otherwise, 
 *	change each byte of data in the message to be its value in the
 *	screen map, and send the message along.
 */
STATIC void
chr_proc_w_data(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	unchar		*chp;
	scrnmap_t	*scrnp;
	scrn_t		*map_p;
	mblk_t		*bp;
	pl_t		pl;


	if ((map_p = cp->c_scrmap_p) == (scrn_t *) NULL) {
		putnext(qp, mp);
		return;
	}

	pl = LOCK(map_p->scr_mutex, plstr);

	if ((scrnp = map_p->scr_map_p) == (scrnmap_t *) NULL) {
		if ((scrnp = map_p->scr_defltp->scr_map_p) == (scrnmap_t *) NULL) {
			UNLOCK(map_p->scr_mutex, pl);
			putnext(qp, mp);
			return;
		}
	}

	for (bp = mp; bp != (mblk_t *) NULL; bp = bp->b_cont) {
		chp = (unchar *) bp->b_rptr;
		while (chp != (unchar *) bp->b_wptr) {
			*chp = *((unchar *)scrnp + *chp);
			chp++;
		}
	}

	UNLOCK(map_p->scr_mutex, pl);

	putnext(qp, mp);

	return;
}


/*
 * STATIC void
 * chr_do_xmouse(queue_t *, mblk_t *, charstat_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
/* ARGSUSED */
STATIC void
chr_do_xmouse(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	struct mse_event *msep;
	xqEvent	*evp;
	pl_t	pl;


	msep = (struct mse_event *) mp->b_cont->b_rptr;

	pl = LOCK(cp->c_mutex, CHRPL);

	evp = &cp->c_xevent;
	evp->xq_type = msep->type;
	evp->xq_x = msep->x;
	evp->xq_y = msep->y;
	evp->xq_code = msep->code;

	UNLOCK(cp->c_mutex, pl);

	(*cp->c_xqinfo->xq_addevent)(cp->c_xqinfo, evp);

	return;
}


/*
 * STATIC void
 * chr_do_mouseinfo(queue_t *, mblk_t *, charstat_t *)
 *	Update the mouse information status.
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
/* ARGSUSED */
STATIC void
chr_do_mouseinfo(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	struct mse_event *msep;
	struct mouseinfo *minfop;
	pl_t pl;


	msep = (struct mse_event *) mp->b_cont->b_rptr;

	pl = LOCK(cp->c_mutex, CHRPL);	

	minfop = &cp->c_mouseinfo;
	minfop->status = (~msep->code & 7) | 
			((msep->code ^ cp->c_oldbutton) << 3) | 
			(minfop->status & BUTCHNGMASK) | 
			(minfop->status & MOVEMENT);

	if (msep->type == MSE_MOTION) {
		int	sum;

		minfop->status |= MOVEMENT;

		sum = minfop->xmotion + msep->x;
		if (sum >= chr_maxmouse)
			minfop->xmotion = (char)chr_maxmouse;
		else if (sum <= -chr_maxmouse)
			minfop->xmotion = -chr_maxmouse;
		else
			minfop->xmotion = (char)sum;

		sum = minfop->ymotion + msep->y;
		if (sum >= chr_maxmouse)
			minfop->ymotion = (char)chr_maxmouse;
		else if (sum <= -chr_maxmouse)
			minfop->ymotion = -chr_maxmouse;
		else
			minfop->ymotion = (char)sum;
	}

	/* Note the button state */
	cp->c_oldbutton = msep->code;

	/*
	 * Set the flag indicating that mouse input has changed
	 * mouse state.
	 */
	cp->c_state |= C_MSEINPUT;

	if (cp->c_heldmseread) {
		mblk_t *tmp;

		tmp = cp->c_heldmseread;
		cp->c_heldmseread = (mblk_t *) NULL;
		UNLOCK(cp->c_mutex, pl);
		chr_write_queue_put(WR(qp), tmp);
		return;
	}

	UNLOCK(cp->c_mutex, pl);

	return;
}


/*
 * STATIC void 
 * chr_proc_w_proto(queue_t *, mblk_t *, charstat_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC void 
chr_proc_w_proto(queue_t *qp, mblk_t *mp, charstat_t *cp)
{
	ch_proto_t	*chp;
	pl_t		pl;


	if ((mp->b_wptr - mp->b_rptr) != sizeof(ch_proto_t)) {	
		putnext(qp, mp);
		return;
	}

	/* LINTED pointer alignment */
	chp = (ch_proto_t *) mp->b_rptr;

	if (chp->chp_type != CH_CTL) {	
		putnext(qp, mp);
		return;
	}

	switch (chp->chp_stype) {
	case CH_TCL: {

		switch (chp->chp_stype_cmd) {
		case TCL_ADD_STR: {
			tcl_data_t	*tp;	
			ushort		keynum, len;

			if (mp->b_cont == (mblk_t *) NULL) {
				putnext(qp, mp);
				return;
			}

			/*
			 * Assume one data block. 
			 */
			/* LINTED pointer alignment */
			tp = (tcl_data_t *) mp->b_cont->b_rptr;

			keynum = tp->add_str.keynum;
			len = tp->add_str.len;

			/*
			 * Put tp past the tcl_data structure in the data
			 * block. Now it points to the string itself.
			 */
			tp++;

			/*
			 * If ws_addstring fails, beep the driver. 
			 */
			if (!ws_addstring(cp->c_map_p, keynum, 
						(unchar *) tp, len)) {
				chp->chp_stype_cmd  = TCL_BELL;
				freemsg(mp->b_cont);
				putnext(qp, mp);	/* ship it down */
				break;
			} else
				freemsg(mp);

			break;
		}

		case TCL_FLOWCTL:
			pl = LOCK(cp->c_mutex, CHRPL);

			if (chp->chp_stype_arg == TCL_FLOWON)
				cp->c_state |= C_FLOWON;
			else if (chp->chp_stype_arg == TCL_FLOWOFF)
				cp->c_state &= ~C_FLOWON;

			UNLOCK(cp->c_mutex, pl);

			putnext(qp, mp);
			break;

		default:
			putnext(qp, mp);
			return;
		} /* switch */
	
		break;

	} /* CH_TCL case */

	default:
		putnext(qp, mp);
		return;

	} /* switch */
}


/*
 * STATIC void
 * chr_online(queue_t *)
 *
 * Calling/Exit State:
 *	- Return ENOMEM if no memory available, otherwise return 0.
 *
 * Description:
 *	Send a protocol message downstream to let kd know that
 *	the char module is ready to accept CH_CHR messages --
 *	CH_CHRMAP, CH_SCRMAP.
 */
STATIC int 
chr_online(queue_t *qp)
{
	mblk_t *mp;
	ch_proto_t *protop;
	

	if ((mp = allocb(sizeof(ch_proto_t), BPRI_HI)) == (mblk_t *) NULL) {
		/*
		 *+ No memory available to send a CH_CHROPEN protocol
		 *+ message downstream.
		 */
		cmn_err(CE_WARN, 
			"chr_online: no memory available");
		return (ENOMEM);
	}

	mp->b_wptr += sizeof(ch_proto_t);
	mp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHR;
	protop->chp_stype_cmd = CH_CHROPEN;

	putnext(WR(qp), mp);

	return (0);
}


/*
 * STATIC void
 * chr_setchanmode(queue_t *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC void
chr_setchanmode(queue_t *qp, int rawmode)
{
	mblk_t  *mp;
	ch_proto_t *protop;


	if ((mp = allocb(sizeof(ch_proto_t), BPRI_HI)) == NULL){
		/*
		 *+ No memory available to send a CH_RAWMODE protocol
		 *+ message downstream.
		 */
		cmn_err(CE_NOTE, 
			"chr_setchanmode: cannot allocate buf");
		return;
	}

	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr += sizeof(ch_proto_t);
	/* LINTED pointer alignment */
	protop = (ch_proto_t *)mp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_RAWMODE;
	protop->chp_stype_arg = rawmode;
	protop->chp_stype_cmd = 0;
	protop->chp_chan = 0;

	putnext(qp, mp);

	return;
}


/*
 * STATIC int
 * chr_send_pcproto_msg(queue_t *, charstat_t *)
 *
 * Calling/Exit State:
 */
STATIC int
chr_send_pcproto_msg(queue_t *qp, charstat_t *cp)
{
	mblk_t	*bp;


	if ((cp->c_state & C_XQUEMDE) == 0)
		return(0);

	if ((bp = allocb(1, BPRI_MED)) == (mblk_t *)NULL) {
		return(1);
        }

	bp->b_datap->db_type = M_PCPROTO;

	qreply(qp, bp);

	return(1);
}


/*
 * STATIC void
 * chr_notify_on_read(queue_t *, int)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
chr_notify_on_read(queue_t *qp, int on)
{
	struct stroptions      *stropt;
	mblk_t                 *bp;


	/*
	 * Set stream head option for M_READ messages.
	 */
	if ((bp = allocb(sizeof (*stropt), BPRI_MED)) == (mblk_t *)NULL) {
		return;
	}

	bp->b_datap->db_type = M_SETOPTS;
	/* LINTED pointer alignment */
	stropt = (struct stroptions *)bp->b_rptr;

	if (on) {
		stropt->so_flags = SO_MREADON;
	} else {
		stropt->so_flags = SO_MREADOFF;
	}

	bp->b_wptr += sizeof (*stropt);

	putnext(qp, bp);

	return;
}


/*
 * STATIC int
 * chr_get_keymap_type(mblk_t *)
 *
 * Calling/Exit State:
 *	- Return -1 if an unknown keymap type.
 */
STATIC int
chr_get_keymap_type(mblk_t *mp)
{
	mblk_t *cont = mp->b_cont;
	struct keymap_flags *kp;
	int err = 0;


	/* LINTED pointer alignment */
	kp = (struct keymap_flags *) cont->b_rptr;

	if (kp->km_magic != KEYMAP_MAGIC) {
		/*
		 *+ Illegal keymap message
		 */
		cmn_err(CE_NOTE, 
			"chr_get_keymap_type: ERROR in KEYMAP message");
		return(-1);
	}

	if (kp->km_type != SCO_FORMAT && kp->km_type != USL_FORMAT)
		err = 1;

	mp->b_cont = cont->b_cont;
	cont->b_cont = NULL;
	freemsg(cont);

	return(err ? -1 : kp->km_type);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_charstat(charstat_t *, int)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Generic dump function that can be called (from kdb also)
 *	to print the values of various data structures that are
 *	part of the char module. It takes two arguments: a data
 *	structure pointer whose fields are to be displayed and
 *	the data structure type. Following are the values of
 *	data structure type:
 *		1 - charstat_t
 */
void
print_charstat(void *structp, int type)
{
	switch (type) {
	case 1: {
		charstat_t *cp;

		cp = (charstat_t *) structp;
		debug_printf("\n charstat_t struct: size=0x%x(%d)\n",
			sizeof(charstat_t), sizeof(charstat_t));
		debug_printf("\tc_state=0x%x, \tc_map_p=0x%x\n",
			cp->c_state, cp->c_map_p);
		debug_printf("\tc_scrmap_p=0x%x,\tc_xqinfop=0x%x\n",
			cp->c_scrmap_p, cp->c_xqinfo);
		debug_printf("\tc_rawprocp=0x%x,\n",
			cp->c_rawprocp);
		debug_printf("\tc_kbstat=0x%x, \tc_copystate=0x%x\n",
			&cp->c_kbstat, &cp->c_copystate);
		debug_printf("\tc_xeventp=0x%x, \tc_mouseinfop=0x%x\n",
			&cp->c_xevent, &cp->c_mouseinfo);
		break;
	}

	default:
		debug_printf("\nUsage (from kdb):\n");
		debug_printf("\t<struct ptr> <type (1)> print_charstat 2 call\n");
                break;

        } /* end switch */
}

#endif /* DEBUG || DEBUG_TOOLS */
