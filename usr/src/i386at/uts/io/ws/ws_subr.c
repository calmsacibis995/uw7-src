#ident	"@(#)ws_subr.c	1.49"
#ident	"$Header$"

/*
 * Modification history
 *
 *	L000	22Jan97		rodneyh@sco.com
 *	- Changes for Gemini multiconsole support
 *	  new functions ws_con_drv_init, ws_con_drv_init2, ws_con_drv_detach,
 *	  ws_con_maybe_bind, ws_con_LWP_on_proc, ws_con_LWP_off_proc,
 *	  ws_con_need_text, and ws_con_done_text
 *	L001	28Feb97		rodneyh@sco.com
 *	- Change to use new bindproc kernel function and remove our own bind
 *	  function.
 *	L002	3Mar97		rodneyh@sco.com
 *	- L001 had bindproc called procbind.
 *	L003	29Apr97		rodneyh@sco.com
 *	- Change to ws_con_drv_init, moved KMA code into seperate function to
 *	  make callable from ws_con_maybe_bind, which may be called before
 *	  memory for the multiconsole control structures has been allocated.
 *	- Changed ws_con_maybe_bind to return a status code.
 *	L004	3May97		rodneyh@sco.com
 *	- Change to ws_con_maybe_bind to check if a process is already on the
 *	  pending list. Also changed to only cmn_err to putbuf and the log
 *	  file instead of the console [unmarked].
 *	L005	24Sep97		rodneyh@sco.com
 *	- Change to ws_con_alloc_mem not to alloc memory twice and thus blow
 *	  one set of pointers which results in a bogus value being passed to
 *	  proc_valid.
 *	- Change to ws_con_drv_init2 to ensure we never add the same process to
 *	  context switch callout dispatch table more than once.
 *	- Change to ws_con_drv_init not to cmn_err if both resume and release
 *	  handler pointers are NULL which is valid.
 *	- Change to ws_con_drv_init2 to call ws_iocack to trigger the
 *	  strioccall just before we exit.
 *	- Change to ws_con_drv_init to copyb the first block of the message and
 *	  use the copy to ACK the ioctl rather than the original.
 *	- Change to ws_con_drv_init2 to free the original ioctl message after
 *	  the driver_init callback returns.
 *	L006	15Oct97		rodneyh@sco.com
 *	- Included ws_priv.h because definitions of ws_bind_t, ws_dispatch_t
 *	  and ws_con_maybe_bind prototypes moved there from ws.h. The inclusion
 *	  of proc.h in ws.h casued Platinum problems with merge requiring
 *	  the shuffle around.
 *	L007	30Oct97		rodneyh@sco.com
 *	- Replace the use of drv_getparm(LBOLT ...) with _TICKS()
 *	  in ws_switch(). We are $interface base only and so should not be
 *	  using DDI functions unless absolutely required to avoid problems
 *	  when the DDI revs.
 *	L008	11Oct97		rodneyh@sco.com
 *	- Fix for panic in ws_con_LWP_on_proc. If we failed to find the process
 *	  in the ws_disp dispatch table but the process parent chain did
 *	  contain the proc ws_max_active_callouts was not incremented, this
 *	  caused a table entry to be wasted on every on proc event quickly
 *	  leading to a table full panic. This brings in to focus two other
 *	  weaknesses in this code, i) We should dynamically grow the callout
 *	  table instead of panicing, ii) We probably should remove the complete
 *	  process tree instead of just the calling process in ws_con_drv_detach.
 *	  Howerver, this could cause a problem if a `knowlegable' process is
 *	  trying to have a particular child removed from the callout table,
 *	  eg, if it knows the child will exec, but since the searching of the
 *	  parent chain is not documented we will take that chance.
 *	  Note: This is a temporary fix to minimise risk to Gemini
 *	L009	18Oct97		rodneyh@sco.com
 *	- Change to ws_con_maybe_bind to track the number of maps made to a
 *	  process pending a cpu bind and only remoce the entry from the
 *	  ws_bind_t table when the last map is undone. Also re-enabled some
 *	  of the cmn_err's that L008 supressed. 
 *	  Note: This is a temporary fix to minimise risk due to the late stage
 *	  of the programme.
 *	  Fix for ul97-31688.
 *
 */

/*
 * Library of routines to support the Integrated Workstation 
 * Environment (IWE).
 */

#include <fs/vnode.h>
#include <io/ascii.h>
#include <io/ansi/at_ansi.h>
#include <io/conf.h>
#include <io/event/event.h>
#include <io/gvid/genvid.h>
#include <io/gvid/vid.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <io/termios.h>
#include <io/ws/chan.h>
#include <io/ws/8042.h>
#include <io/ws/ws_priv.h>	/* L006 */
#ifndef NO_MULTI_BYTE
#include <io/ws/mb.h>
#endif /* NO_MULTI_BYTE */
#include <io/ws/tcl.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <mem/kmem.h>
#include <proc/bind.h>		/* For bindproc, L001 */
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/lwp.h>		/* For lwp struct, L000 */
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <proc/pid.h>		/* For struct pid, L000 */
#include <svc/errno.h>
#include <svc/clock.h>		/* For _TICKS(), L007 */
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>	/* For engine struct, L000 */
#include <util/ghier.h>		/* For engine mutext heirarchy ,L000 */
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <fs/specfs/specfs.h>

extern wstation_t Kdws;
extern struct attrmask kb_attrmask[];

extern int	ws_maxminor;		/* defined in ws.cf/Space.c file */
extern minor_t	maxminor;
extern int	nattrmsks;

extern int	gviddevflag;
extern gvid_t	Gvid;
extern int	gvidflg;
extern lock_t	*gvid_mutex;
extern sv_t	*gvidsv;


STATIC uint_t	*ws_compatflgs;
STATIC uint_t	*ws_svr3_compatflgs;
STATIC uint_t	*ws_svr4_compatflgs;
STATIC struct kdvdc_proc *kdvdc_vt;

/* L000 begin
 *
 * Dispach table for multiconsole context switch callouts
 */

#define NONE_ON_PROC 0xABABE

extern int ws_max_callouts;			/* Defined in space.c */

ulong_t ws_callout_not_found = 0;		/* Count of missing callouts */
boolean_t ws_mcon_active = B_FALSE;
volatile boolean_t ws_release_pending = B_FALSE; /* Waiting for a resume? */
extern processorid_t ws_processor;
/*
 * OPTIMISATION:
 *		All these tables should grow dynamically from KMA, note that
 * the number of entries in the ws_bind_pid table really has nothing to do with
 * the max number of supported multiconsole devices. RGH - 20/1/97
 */

volatile int ws_max_active_callouts = 0;	/* Max callouts ever used */
int ws_pending_bind = 0;			/* Num PIDs waiting for bind */
volatile int ws_cur_drv = NONE_ON_PROC;		/* Current display mem owner */
volatile int ws_cur_tag = 0;			/* Tag of current driver */

ws_bind_t *ws_bind_proc;		/* PIDs to be maybe bound */
ws_dispatch_t *ws_disp;			/* Callout table, L000 end */

/*
 * ws_channel_t *
 * ws_activechan(wstation_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive/shared mode.
 *	- Given a (wstation_t *), return the active channel
 *	  as a (ws_channel_t *).
 *
 * Note:
 *	An equivalent WS_ACTIVECHAN and WS_ISACTIVECHAN macro also exist
 *	in ws/ws.h. It is recommended to use the macros instead of this
 *	function.
 */
ws_channel_t *
ws_activechan(wstation_t *wsp)
{
        if (wsp->w_init) {
		ASSERT(wsp->w_chanpp);
		return ((ws_channel_t *)*(wsp->w_chanpp + wsp->w_active));
	} else
		return ((ws_channel_t *) NULL);
}


/*
 * ws_channel_t *
 * ws_getchan(wstation_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	- Given a (wstation_t *) and a channel number,
 *	  return the channel as a (ws_channel_t *).
 *
 *
 * Note:
 *	An equivalent WS_GETCHAN macro also exist in ws/ws.h. It is 
 *	recommended to use the macros instead of this function.
 */
ws_channel_t *
ws_getchan(wstation_t *wsp, int chan)
{
        if (wsp->w_init) {
		ASSERT(wsp->w_chanpp);
		return ((ws_channel_t *)*(wsp->w_chanpp + chan));
	} else
		return ((ws_channel_t *) NULL);
}


/*
 * int
 * ws_freechan(wstation_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *	- If the channel is not available, it returns -1. otherwise
 *	  return the identity of an available channel.
 */
int
ws_freechan(wstation_t *wsp)
{
	int		cnt;
	ws_channel_t	*chp;


	for (cnt = 0; cnt < WS_MAXCHAN; cnt++) {
		ASSERT(wsp->w_chanpp);
		chp = *(wsp->w_chanpp + cnt);
		if (chp == NULL) 
			return (-1);
		if (!chp->ch_opencnt) 
			return (cnt);
	}

	return (-1);
}


/*
 * int
 * ws_getchanno(minor_t)
 *
 * Calling/Exit State:
 *	- No locks need be held across this function.
 */
int
ws_getchanno(minor_t cmux_minor)
{
	return (cmux_minor % WS_MAXCHAN);
}


/*
 * int
 * ws_getws(minor_t cmux_minor)
 *
 * Calling/Exit State:
 *	- No locks need be held across this function.
 */
int
ws_getws(minor_t cmux_minor)
{
	return (cmux_minor / WS_MAXCHAN);
}


/*
 * int
 * ws_alloc_attrs(wstation_t *, ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- Return 1 if unable to successfully allocate/initialze attrmask
 *	  structure, otherwise return 0.
 *	- w_rwlock is held in exclusive mode. 
 */
/* ARGSUSED */
int
ws_alloc_attrs(wstation_t *wsp, ws_channel_t *chp, int km_flag)
{
	termstate_t	*tsp;
	size_t		allocsize;


	tsp = &chp->ch_tstate;
	allocsize = sizeof(struct attrmask) * nattrmsks;

	if (!tsp->t_attrmskp) {
		tsp->t_attrmskp = kmem_zalloc(allocsize, km_flag);
	}

	if (tsp->t_attrmskp == (struct attrmask *)NULL)
		return (1);

	bcopy(&kb_attrmask[0], tsp->t_attrmskp, allocsize);

	/* reset color info -- useful for resetting VT 0 */
	tsp->t_nattrmsk = (uchar_t) nattrmsks;
	tsp->t_normattr = NORM;
	tsp->t_curattr = tsp->t_normattr;
	tsp->t_nfcolor = WHITE;		/* normal foreground color */
	tsp->t_nbcolor = BLACK;		/* normal background color */
	tsp->t_rfcolor = BLACK;		/* reverse foreground video color */
	tsp->t_rbcolor = WHITE;		/* reverse background video color */
	tsp->t_gfcolor = WHITE;		/* graphic foreground character color */
	tsp->t_gbcolor = BLACK;		/* graphic background character color */
	tsp->t_origin = 0;

	return (0);
}


/*
 * void
 * ws_chinit(wstation_t *, ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode. 
 */
void
ws_chinit(wstation_t *wsp, ws_channel_t *chp, int chan)
{
	vidstate_t	*vp;
	termstate_t	*tsp;
	unchar		cnt;


	chp->ch_wsp = wsp;
	chp->ch_opencnt = 0;
	chp->ch_procp = (void *)NULL;
	chp->ch_iocarg = NULL;
	chp->ch_rawmode = 0;
	chp->ch_relsig = SIGUSR1;
	chp->ch_acqsig = SIGUSR1;
	chp->ch_frsig = SIGUSR2;

	if (!(chp->ch_strtty.t_state & (ISOPEN | WOPEN))) {
		chp->ch_strtty.t_line = 0;
		chp->ch_strtty.t_iflag = IXON | ICRNL | ISTRIP;
		chp->ch_strtty.t_oflag = OPOST | ONLCR;
		chp->ch_strtty.t_cflag = B9600 | CS8 | CREAD | HUPCL;
		chp->ch_strtty.t_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK;
		chp->ch_strtty.t_state |= CARR_ON;
		chp->ch_strtty.t_state |= (ISOPEN | WOPEN);
	}

	chp->ch_id = chan;
	chp->ch_dmode = wsp->w_dmode;
	chp->ch_vstate = wsp->w_vstate;		/* struct copy */
	chp->ch_flags = 0;
#ifdef MERGE386
	chp->ch_merge = 0;
#endif /* MERGE386 */
	vp = &chp->ch_vstate;
	tsp = &chp->ch_tstate;
	tsp->t_sending = tsp->t_sentrows = tsp->t_sentcols = 0;

	if (vp->v_cmos == MCAP_COLOR)
		tsp->t_flags = ANSI_MOVEBASE;
	else
		tsp->t_flags = 0;

	vp->v_undattr = wsp->w_vstate.v_undattr;

	tsp->t_flags = 0;
	tsp->t_bell_time = BELLCNT;
	tsp->t_bell_freq = NORMBELL;
	tsp->t_auto_margin = AUTO_MARGIN_ON;
	tsp->t_rows = WSCMODE(vp)->m_rows;
	tsp->t_cols = WSCMODE(vp)->m_cols;
	tsp->t_scrsz = tsp->t_rows * tsp->t_cols;

	ws_alloc_attrs(wsp, chp, KM_NOSLEEP);
	bcopy (&kb_attrmask[0], tsp->t_attrmskp, 
			sizeof(struct attrmask) * nattrmsks);

 	if (vp->v_regaddr == MONO_REGBASE) {
 		tsp->t_attrmskp[1].attr = 0;
 		tsp->t_attrmskp[4].attr = 1;
 		tsp->t_attrmskp[34].attr = 7;
 	} else {
 		tsp->t_attrmskp[1].attr = BRIGHT;
 		tsp->t_attrmskp[4].attr = 0;
 		tsp->t_attrmskp[34].attr = 1;
 	}

	tsp->t_row = 0;
	tsp->t_col = 0;
	tsp->t_cursor = 0;
	tsp->t_curtyp = 0;
	tsp->t_undstate = 0;
	tsp->t_font = ANSI_FONT0;
	tsp->t_pstate = 0;
	tsp->t_ppres = 0;
	tsp->t_pcurr = 0;
	tsp->t_pnum = 0;
	tsp->t_ntabs = 9;
	for (cnt = 0; cnt < 9; cnt++)
		tsp->t_tabsp[cnt] = cnt * 8 + 8;

#ifndef NO_MULTI_BYTE
	if (gs_init_flg)
		wsp->w_consops->cn_gs_chinit(wsp, chp);
#endif /* NO_MULTI_BYTE */
}


/*
 * void
 * ws_openresp(queue_t *, mblk_t *, ch_proto_t *, ws_channel_t *, unsigned long)
 *	Expected call from principal stream upon receipt of
 *	CH_CHANOPEN message from CHANMUX
 *
 * Calling/Exit State:
 *	- No locks are held either on entry or exit.
 */
/* ARGSUSED */
void
ws_openresp(queue_t *qp, mblk_t *mp, ch_proto_t *protop, ws_channel_t *chp,
		unsigned long error)
{
	mp->b_datap->db_type = M_PCPROTO;
	protop->chp_stype = CH_PRINC_STRM;
	protop->chp_stype_cmd = CH_OPEN_RESP;
	protop->chp_stype_arg = error;
	qreply(qp, mp);
}


/*
 * void
 * ws_openresp_chr(queue_t *, mblk_t *, ch_proto_t *, ws_channel_t *)
 *	Expected call from principal stream upon receipt of
 *      CH_CHROPEN message from CHAR module.
 *
 * Calling/Exit State:
 *	- No locks are held either on entry or exit.
 */
/* ARGSUSED */
void
ws_openresp_chr(queue_t *qp, mblk_t *mp, ch_proto_t *protop, ws_channel_t *chp)
{
	mblk_t *charmp, *scrmp, *kbstatmp;


	if (!(charmp = allocb(sizeof(ch_proto_t), BPRI_HI)))
		return;

	charmp->b_datap->db_type = M_PROTO;
	charmp->b_wptr += sizeof(ch_proto_t);
	/* LINTED pointer alignment */
	protop = (ch_proto_t *) charmp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_CHR;
	protop->chp_stype_cmd = CH_CHRMAP;
	protop->chp_stype_arg = (unsigned long) chp->ch_charmap_p;
	scrmp = copymsg(charmp);
	kbstatmp = copymsg(charmp);
	if (kbstatmp)
	{
		protop = (ch_proto_t *) kbstatmp->b_rptr;
		protop->chp_stype_cmd = CH_KBNONTOG;
		protop->chp_stype_arg = (unsigned long) (chp->ch_kbstate.kb_state & NONTOGGLES);
		qreply(qp, kbstatmp);
	}
	qreply(qp, charmp);
	if (scrmp != (mblk_t *) NULL) {
		/* LINTED pointer alignment */
		protop = (ch_proto_t *) scrmp->b_rptr;
		protop->chp_stype_cmd = CH_SCRMAP;
		protop->chp_stype_arg = (unsigned long) &chp->ch_scrn;
		qreply(qp, scrmp);
        }
}

/*
 * void
 * ws_preclose(wstation_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode across the function.
 *
 * Description:
 *	The function is called before doing actual channel close in
 *	principal stream. It is called upon receipt of a CH_CLOSECHAN
 *	message from CHANMUX. 
 *
 *	It will cancel any "forced VT switch" pending timeouts,
 *	reset the channel out of process mode for VT switching, and
 *	switch the active channel to the next VT in the list or
 *	channel 0. The ws_channel_t structure for this channel is
 *	removed from the list of VTs in use.
 */
void
ws_preclose(wstation_t *wsp, ws_channel_t *chp)
{
	ws_channel_t	*achp;
	pl_t		pl, oldpri;


	wsp->w_noacquire = 0;

	if (wsp->w_forcetimeid && (wsp->w_forcechan == chp->ch_id)) {
		RW_UNLOCK(wsp->w_rwlock, (pl = getpl())); 
		untimeout(wsp->w_forcetimeid);
		pl = RW_WRLOCK(wsp->w_rwlock, pl);
		wsp->w_forcetimeid = 0;
		wsp->w_forcechan = 0;
	}

	achp = WS_ACTIVECHAN(wsp);

	chp->ch_opencnt = 0;
	ws_automode(wsp, chp);

#ifndef NO_MULTI_BYTE
	/*
	 * KLUDGE: To prevent "close" resetting the channel back
	 * to default text mode, we only reset the mode if the
	 * channel is NOT in graphics-text mode.  This avoids
	 * resetting the video mode to graphics-text mode on
	 * every open during system initialization.
	 */
	if (chp->ch_dmode != KD_GRTEXT || chp->ch_id != 0) {
		chp->ch_dmode = wsp->w_dmode;
		chp->ch_vstate = wsp->w_vstate;		/* struct copy */
	}
#else
	chp->ch_dmode = wsp->w_dmode;
	chp->ch_vstate = wsp->w_vstate;		/* struct copy */
#endif /* NO_MULTI_BYTE */

	chp->ch_flags = 0;

	if (chp == achp)
		if (achp->ch_prevp != achp)
			(void) ws_activate(wsp, chp->ch_prevp, VT_FORCE);
		else
			(void) ws_activate(wsp, ws_getchan(wsp, 0), VT_FORCE);	

	oldpri = splhi(); 
	if (chp->ch_prevp)
		chp->ch_prevp->ch_nextp = chp->ch_nextp;
	if (chp->ch_nextp)
		chp->ch_nextp->ch_prevp = chp->ch_prevp;
	chp->ch_prevp = chp->ch_nextp = chp;
	splx(oldpri); 
}


/*
 * void
 * ws_closechan(queue_t *, wstation_t *, ws_channel_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	ws_closechan() is called after principal 
 *	stream-specific close() routine is called.
 *	This routine sends up the CH_CLOSE_ACK
 *	message CHANMUX is sleeping on.
 */
/* ARGSUSED */
void
ws_closechan(queue_t *qp, wstation_t *wsp, ws_channel_t *chp, mblk_t *mp)
{
	ch_proto_t *protop;


	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_stype = CH_PRINC_STRM;
	protop->chp_stype_cmd = CH_CLOSE_ACK;
	qreply(qp, mp);
}


/*
 * int
 * ws_activate(wstation_t *, ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode across the function.
 *
 * Description:
 *	May be called from interrupt level to change active vt.
 *
 *	Tries to set the given vt on the ring to be the active vt
 *	If auto mode VT, just do the switch, else if process mode,
 *	signal the process, set active vt state to switch pending
 *	and set a timeout to wait for switch to complete or be refused.
 */
int
ws_activate(wstation_t *wsp, ws_channel_t *chp, int force)
{
	ws_channel_t	*achp;


	if (WS_ISACTIVECHAN(wsp, chp))
		return (1);

	/*
	 * Get a pointer to the active channel.
	 */
	achp = WS_ACTIVECHAN(wsp);

	if (!ws_procmode(wsp, achp) || force || proc_traced(achp->ch_procp))
		return (ws_switch(wsp, chp, force));

	ASSERT(getpl() == plstr);

	/*
	 * If switch is already requested or do not acquire flag is
	 * set because vt is in process mode, then return immediately.
	 */
        if (wsp->w_switchto || wsp->w_noacquire)
                return (0);

	ASSERT(proc_valid(achp->ch_procp));
	proc_signal(achp->ch_procp, achp->ch_relsig);
	wsp->w_switchto = chp;
	achp->ch_timeid = itimeout((void(*)())wsp->w_consops->cn_rel_refuse, 
					NULL, 10*HZ, plstr);
	return (1);
}


/*
 * int
 * ws_switch(wstation_t *, ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 */
int
ws_switch(wstation_t *wsp, ws_channel_t *chp, int force)
{
	ws_channel_t	*achp;
	ch_proto_t	*protop;
	mblk_t		*mp;
	pl_t		pl, oldpri;
	toid_t		tid;
	boolean_t	i8042lkheld = B_FALSE;


	if (wsp->w_forcetimeid )
		return (0);

	if ((mp = allocb(sizeof(ch_proto_t), BPRI_HI)) == (mblk_t *) NULL)
		return (0);

	achp = WS_ACTIVECHAN(wsp);

	/* block all device interrupts in particular the clock interrupt */
	oldpri = splhi(); 

	while (achp->ch_timeid) {
		tid = achp->ch_timeid;
		achp->ch_timeid = 0;
		RW_UNLOCK(wsp->w_rwlock, (pl = getpl()));
		untimeout(tid);
		pl = RW_WRLOCK(wsp->w_rwlock, pl);
	}

	/* unblock device interrupts */
	splx(oldpri);

	if ((*wsp->w_consops->cn_activate)(chp, force)) {
		wsp->w_switchto = (ws_channel_t *) NULL;
		achp->ch_flags &= ~CHN_ACTV;

		/*
		 * If the "current active channel" is in process mode, 
		 * then send the ch_frsig signal to the process owning 
		 * this channel.
		 */ 
		if (ws_procmode(wsp, achp) && force && 
				!proc_traced(achp->ch_procp)) { 
			ASSERT(proc_valid(achp->ch_procp));
			proc_signal(achp->ch_procp, achp->ch_frsig);
		}

		chp->ch_flags |= CHN_ACTV;

		/*
		 * If the "new to be active channel" is in process mode,
		 * then send the ch_acqsig signal to the process owning 
		 * this channel.
		 */
		if (ws_procmode(wsp, chp) && !proc_traced(chp->ch_procp)) {
			wsp->w_noacquire++;
			ASSERT(proc_valid(chp->ch_procp));
			proc_signal(chp->ch_procp, chp->ch_acqsig);
			chp->ch_timeid = itimeout(
				(void(*)())wsp->w_consops->cn_acq_refuse,
				chp, 10*HZ, plstr);
		}

		/*
		 * If new vt is waiting to become active, then wake it up. 
		 */
		if (CHNFLAG(chp, CHN_WACT)) {
			chp->ch_flags &= ~CHN_WACT;
			SV_SIGNAL(chp->ch_wactsv, 0);
		}

		mp->b_datap->db_type = M_PROTO;
		mp->b_wptr += sizeof(ch_proto_t);
		/* LINTED  pointer alignment */
		protop = (ch_proto_t *) mp->b_rptr;
		protop->chp_type = CH_CTL;
		protop->chp_stype = CH_PRINC_STRM;
		protop->chp_stype_cmd = CH_CHANGE_CHAN;
		protop->chp_tstmp = (long)_TICKS();		/* L007 */
		protop->chp_chan = chp->ch_id;
		RW_UNLOCK(wsp->w_rwlock, (pl = getpl()));
		putnext(chp->ch_qp, mp);
		pl = RW_WRLOCK(wsp->w_rwlock, pl);

		/*
		 * Do not disable the keyboard interface while kd
		 * is in its interrupt handler, since it disables
		 * and enables the keyboard interface to turn on
		 * or off the leds. 
		 *
		 * Note that there is a race condition in here. If 
		 * kd is in its interrupt handler and the i8042 lock
		 * is held while programming the leds, but just after
		 * the leds are programmed and before the following
		 * check, the kd exits its interrupt handler, thereby
		 * releasing the lock, we may then attempt to release
		 * a unheld lock. This is only true if the interrupt
		 * handler is on one processor and the vt switching
		 * on another processor.
		 *
		 * The race condition is closed by setting the 8042
		 * lock state to held if we acquire it and check for
		 * it when we attempt to release it.
		 */
		if (!(KBLEDMASK(chp->ch_kbstate.kb_state))) {
			if (!WS_INKDINTR(&Kdws)) {
				i8042_acquire();
				i8042lkheld = B_TRUE;
			}
       	        	i8042_update_leds(achp->ch_kbstate.kb_state,
       	        			  chp->ch_kbstate.kb_state);
			if (!WS_INKDINTR(&Kdws) && i8042lkheld == B_TRUE) {
				i8042_release();
				i8042lkheld = B_FALSE;
			}
		}
		return (1);
	} else {
		freemsg(mp);
		return (0);
	}
}


/*
 * int
 * ws_procmode(wstation_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive/shared mode.
 *	- w_mutex lock is acquired because ws_automode()
 *	  requires the w_mutex basic lock to be held.
 */
int
ws_procmode(wstation_t *wsp, ws_channel_t *chp)
{
	pl_t	pl;


        if (chp->ch_procp && !proc_valid(chp->ch_procp)) {
		pl = LOCK(wsp->w_mutex, plstr);
                ws_automode(wsp, chp);
		UNLOCK(wsp->w_mutex, pl);
        }

	return (CHNFLAG(chp, CHN_PROC));
}


/*
 * void
 * ws_automode(wstation_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *	- called from vt when a process control mode vt 
 *	  changes to auto mode.
 *      - w_rwlock is held in exclusive/shared mode.
 *	- If w_rwlock is held in shared mode, then channel
 *	  ch_mutex basic lock is also held.
 *      - w_mutex basic lock is held to protect w_map.
 *	- If w_rwlock is held in exclusive mode, then its
 *	  not necessary to hold either the ch_mutex or 
 *	  w_mutex lock.
 *
 * Note:
 *	The w_rwlock must be held in exclusive mode when the process
 *	is no longer valid and needs to unmap the video buffer.
 */

void
ws_automode(wstation_t *wsp, ws_channel_t *chp)
{
        struct map_info	*map_p = &wsp->w_map;
        void		*procp;


        if (WS_ISACTIVECHAN(wsp, chp) && map_p->m_procp && 
				map_p->m_procp == chp->ch_procp) {

                if (!proc_valid(chp->ch_procp)) {
			/*
			 * The process that has the video buffer 
			 * mapped is in a stale state and is merely
			 * waiting for the driver to unreference it 
			 * so that it can exit.
			 */
			proc_unref(map_p->m_procp);
                        map_p->m_procp = (void *)0;
                        chp->ch_flags &= ~CHN_MAPPED;
                        map_p->m_cnt = 0;
                        map_p->m_chan = 0;
                } else {
			/*
			 * Cannot reach here via ws_procmode().
			 *
			 * Can reach here via ws_preclose() when a 
			 * channel is being closed or from kdvt_ioctl()
			 * when the channel is being reset from process
			 * mode to auto mode.
			 *
			 * Need to verify that the process that is 
			 * reseting to auto mode has the video buffer
			 * mapped. Must not do proc_ref() when the
			 * channel is being closed because we do not
			 * have user context. 
			 */

			if (chp->ch_opencnt) {	/* from kdvt_ioctl */
				procp = proc_ref();
				if (map_p->m_procp == procp)
					(*wsp->w_consops->cn_unmapdisp)(chp, map_p);
				proc_unref(procp);
			} else {		/* from ws_preclose */
				ASSERT(chp->ch_opencnt == 0);
			}
                }
        }

	/*
	 * Release the channel reference to the process
	 * and reset the ch_procp pointer.
	 */
	
	if (chp->ch_procp)
		proc_unref(chp->ch_procp);
        chp->ch_procp = (void *) NULL;
        chp->ch_flags &= ~CHN_PROC;
        chp->ch_relsig = SIGUSR1;
        chp->ch_acqsig = SIGUSR1;
        chp->ch_frsig = SIGUSR2;
}

/*
 * void
 * ws_xferwords(ushort *, ushort *, int, char) 
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_xferwords(ushort *srcp, ushort *dstp, int cnt, char dir)
{
	switch (dir) {
	case UP:
		while (cnt--)
			*dstp-- = *srcp--;
		break;

	default:
		while (cnt--)
			*dstp++ = *srcp++;
		break;
	}
}


/*
 * void
 * ws_setlock(wstation_t *, int)
 *
 * Calling/Exit State:
 * TBD. (not called within the kd driver).
 */
void
ws_setlock(wstation_t *wsp, int lock)
{
	if (lock)
		wsp->w_flags |= KD_LOCKED;
	else
		wsp->w_flags &= ~KD_LOCKED;
}


/*
 * STATIC void
 * ws_sigkill(wstation_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode across the function.
 *	- Called when the channel is forced to be switched out.
 */
STATIC void
ws_sigkill(wstation_t *wsp, int	chan)
{
	vidstate_t	vbuf;
	ws_channel_t	*chp;
	struct map_info	*map_p;
	pl_t		opl;

	map_p = &wsp->w_map;

	chp = (ws_channel_t *)ws_getchan(wsp, chan);
	if (chp == NULL) {
		return;
	}

	bcopy(&chp->ch_vstate, &vbuf, sizeof(vidstate_t));
	ws_chinit(wsp, chp, chan);
	chp->ch_opencnt = 1;

	if (map_p->m_procp && map_p->m_chan == chp->ch_id) {
		/*
	 	 * The channel has the video buffer mapped
		 * and must unreference it, otherwise
		 * a process may never exit because the
		 * the driver has a reference to it. 
		 */
		proc_unref(map_p->m_procp);				
		bzero(map_p, sizeof(struct map_info));
	}

	bcopy(&vbuf, &chp->ch_vstate, sizeof(vidstate_t));
	chp->ch_vstate.v_cvmode = wsp->w_vstate.v_dvmode;
	wsp->w_forcetimeid = 0;
	wsp->w_forcechan = 0;

	tcl_reset(wsp->w_consops, chp, &chp->ch_tstate);

	if (chp->ch_nextp) {
		ws_activate(wsp, chp->ch_nextp, VT_NOFORCE);
	}

	opl = splhi();
	if (chp->ch_nextp)
		chp->ch_nextp->ch_prevp = chp->ch_prevp;
	if (chp->ch_prevp) 
		chp->ch_prevp->ch_nextp = chp->ch_nextp;
	chp->ch_prevp = chp->ch_nextp = chp;
	splx(opl);
}


/*
 * void
 * ws_force(wstation_t *, ws_channel_t *, pl_t)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode on entry, but
 *	  is released before exiting from the function.
 *	- This routine is called by kdvt_switch() when a channel is
 *	  required to be switched out.
 */
void
ws_force(wstation_t *wsp, ws_channel_t *chp, pl_t pl)
{
	if (chp->ch_id == 0) {
		RW_UNLOCK(wsp->w_rwlock, pl);
		return;
	}
	ws_sigkill(wsp, chp->ch_id);
	RW_UNLOCK(wsp->w_rwlock, pl);
	putnextctl1(chp->ch_qp, M_PCSIG, SIGKILL);
	putnextctl(chp->ch_qp, M_HANGUP);
}


/*
 * void
 * ws_mctlmsg(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	- No locks are held across ws_mctlmsg().
 *
 * Description:
 *	Service M_CTL type message.
 */
void
ws_mctlmsg(queue_t *qp, mblk_t *mp)
{
	struct iocblk	*iocp;

	/*
	 * Since M_CTL messages can only be generated by another module
	 * and never by a user-level process, we must check for a null
	 * size message.
	 */
	if (mp->b_wptr - mp->b_rptr != sizeof(struct iocblk)) {
		/*
		 *+ An unknown M_CTL message. It is possible that an
		 *+ unprocessed  EUC (Extended Unix Code) message
		 *+ could be recieved by the kd driver because
		 *+ internationalization module may not exist on
		 *+ the stack to trap and process the message.
		 */
		cmn_err(CE_NOTE, 
			"!ws_mctlmsg: bad M_CTL msg");
		freemsg(mp);
		return;
	}

	/* LINTED pointer alignment */
	if ((iocp = (struct iocblk *) mp->b_rptr)->ioc_cmd == MC_CANONQUERY) {
		iocp->ioc_cmd = MC_DO_CANON;
		qreply(qp, mp);
		return;
	}

#ifndef NO_MULTI_BYTE
	switch (iocp->ioc_cmd) {
	case EUC_WSET: {
		ws_channel_t       *chp = (ws_channel_t *) qp->q_ptr;
		struct eucioc   *euciocp;

		if (gs_init_flg && fnt_init_flg && mp->b_cont &&
		    (euciocp = (struct eucioc *) mp->b_cont->b_rptr)) {
			Kdws.w_consops->cn_gs_seteuc(chp, euciocp);
		}
		freemsg(mp);
		return;
	}
	default:
		break;
	}
#endif /* NO_MULTI_BYTE */

#ifdef DEBUG
	/*
	 *+ M_CTL message ioctl command is not of type MC_CANONQUERY. 
	 */
	cmn_err(CE_NOTE, 
		"ws_mctlmsg: M_CTL msg not MC_CANONQUERY");
#else
	/*
	 *+ M_CTL message ioctl command is not of type MC_CANONQUERY. 
	 */
	cmn_err(CE_NOTE, 
		"!ws_mctlmsg: M_CTL msg not MC_CANONQUERY");
#endif /* DEBUG */
	freemsg(mp);
	return;

}


/*
 * void
 * ws_notifyvtmon(ws_channel_t *, unchar)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_notifyvtmon(ws_channel_t *vtmchp, unchar ch)
{
	mblk_t	*mp;


	if (!(mp = allocb(sizeof(unchar)*1, BPRI_MED))) {
		/*
		 *+ There isn't enough memory available to allocate
		 *+ message block to be sent upstream to the vtlmgr 
		 *+ to notify channel switch.
		 */
		cmn_err(CE_WARN, 
			"!ws_notifyvtmon: can't get msg");
		return;
	}

	*mp->b_wptr++ = ch;
	putnext(vtmchp->ch_qp, mp);
}


/*
 * void
 * ws_iocack(queue_t *, mblk_t *, struct iocblk *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
void
ws_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp)
{
	mblk_t	*tmp;


	mp->b_datap->db_type = M_IOCACK;

	if ((tmp = unlinkb(mp)) != (mblk_t *) NULL)
		freeb(tmp);

	iocp->ioc_count = iocp->ioc_error = 0;
	qreply(qp, mp);
}


/*
 * void
 * ws_iocnack(queue_t *, mblk_t *, struct iocblk *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
void
ws_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = -1;
	iocp->ioc_error = error;
	qreply(qp, mp);
}


/*
 * void
 * ws_copyout(queue_t *, mblk_t *, mblk_t *, uint)
 *
 * Calling/Exit State:
 *	- No locks are held across ws_copyout().
 */
void
ws_copyout(queue_t *qp, mblk_t *mp, mblk_t *tmp, uint size)
{
	struct copyreq	*cqp;


	/* LINTED pointer alignment */
	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t)*(long *)mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)NULL;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;
	if (mp->b_cont)
		freemsg(mp->b_cont);
	mp->b_cont = tmp;
	qreply(qp, mp);
}


/*
 * void 
 * ws_mapavail(ws_channel_t *, struct map_info *)
 *
 * Calling/Exit State:
 *	Following locks are held:
 *		- w_rwlock is held in either exclusive/shared mode.
 *		- ch_mutex and w_mutex lock may also be held if
 *		  w_rwlock is held in shared mode.
 *		- It is not necessary to hold ch_mutex and w_mutex lock
 *		  if w_rwlock is held in exclusive mode.
 */
void
ws_mapavail(ws_channel_t *chp, struct map_info *map_p)
{
	if (!map_p->m_procp) {
		chp->ch_flags &= ~CHN_MAPPED;
		return;
	}

	if (!proc_valid(map_p->m_procp)) {
		/*
		 * The process that has the video buffer
		 * mapped is in a stale state and is merely
		 * waiting for the driver to unreference it so
		 * that its data structure can be deallocated
		 * and can exit.
		 */
		proc_unref(map_p->m_procp);
		map_p->m_procp = (void *) 0;
		chp->ch_flags &= ~CHN_MAPPED;
		map_p->m_cnt = 0;
		map_p->m_chan = 0;
	}
}


#define XQDISAB		0
#define XQENAB		1

/*
 * int
 * ws_notify(ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- return 0, if was able to set the X/queue mode state,
 *	  otherwise return appropriate errno.
 *
 * Description:
 *	This routines is called from ws_queuemode() to send a message
 *	upstream to indicate that the change of channel's X/queue mode
 *	state. It sleeps while an acknowledgement from the CHAR module
 *	is received.
 */
int
ws_notify(ws_channel_t *chp, int state)
{
	mblk_t		*mp;
	ch_proto_t	*protop;
	pl_t		opl;


	if (!(mp = allocb(sizeof(ch_proto_t), BPRI_MED))) {
		/*
		 *+ There isn't enough memory available to allocate
		 *+ for ch_proto_t size message block.
		 */
		cmn_err(CE_WARN, 
			"ws_notify: cannot alloc msg");
		return (ENOMEM);
	}

	mp->b_wptr += sizeof(ch_proto_t);
	mp->b_datap->db_type = M_PROTO;
	/* LINTED pointer alignment */
	protop = (ch_proto_t *) mp->b_rptr;
	protop->chp_type = CH_CTL;
	protop->chp_stype = CH_XQ;

	if (state == XQENAB) {
		protop->chp_stype_cmd = CH_XQENAB;
		protop->chp_stype_arg = (long) &chp->ch_xque;
	} else {
		protop->chp_stype_cmd = CH_XQDISAB;
		protop->chp_stype_arg = 0;
	}

	if (chp->ch_qp == chp->ch_wsp->w_qp)
		ws_kbtime(chp->ch_wsp);

	opl = LOCK(chp->ch_mutex, plstr);
	chp->ch_flags |= CHN_HIDN;
	UNLOCK((chp)->ch_mutex, opl);

	putnext(chp->ch_qp, mp);

	(void) LOCK(chp->ch_mutex, plstr);
	/*
	 * If the CHN_HIDN flag is cleared, then unlock the lock
	 */
	if (!(chp->ch_flags & CHN_HIDN))
		UNLOCK(chp->ch_mutex, plbase);
	/*
	 * CHN_HIDN is still set, so wait for the signal.  Sleep
	 * interruptibly, and return EINTR if interrupted.  In SVR4
	 * the sleep priority was PZERO+1.
	 */
	else if (!SV_WAIT_SIG(chp->ch_xquesv, primed - 2, chp->ch_mutex))
		return (EINTR);

	if (state == XQENAB && !CHNFLAG(chp, CHN_QRSV))
		/* something's wrong */
		return (EFAULT);

	return (0);
}


/*
 * int
 * ws_queuemode(ws_channel_t *, int, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- return 0, if was able to set the channel to X/queue mode, 
 *	  otherwise return appropriate errno.
 */
int
ws_queuemode(ws_channel_t *chp, int cmd, int arg)
{
#define	RESET_CHAN_QRSV(chp, pl) { \
		(pl) = LOCK((chp)->ch_mutex, plstr); \
		(chp)->ch_flags &= ~CHN_QRSV; \
		UNLOCK((chp)->ch_mutex, (pl)); \
		SV_SIGNAL((chp)->ch_qrsvsv, 0); \
}
	struct kd_quemode qmode;
	xqInfo		*xqp = &chp->ch_xque;
	int		error = 0;
	pl_t		pl;
	extern int	event_check_que(xqInfo *, dev_t, void *, int);


        switch (cmd) {
	case LDEV_MSEATTACHQ:
	case LDEV_ATTACHQ:
		pl = LOCK(chp->ch_mutex, plstr);
		while (CHNFLAG(chp, CHN_QRSV)) {
			/* PZERO+1 == primed-2 */
			if (!SV_WAIT_SIG(chp->ch_qrsvsv, primed - 2,
					chp->ch_mutex))
				return (EINTR);
			pl = LOCK(chp->ch_mutex, plstr);
		}
		UNLOCK(chp->ch_mutex, pl);

		if (xqp->xq_proc && !proc_valid(xqp->xq_proc))
			xq_close(xqp);

		if (xqp->xq_queue)      /* already in queue mode */
			return (EBUSY);

		pl = LOCK(chp->ch_mutex, plstr);
		chp->ch_flags |= CHN_QRSV;
		UNLOCK(chp->ch_mutex, pl);

		error =  event_check_que(xqp, arg, (void *)chp, cmd);
		if (error) {
			RESET_CHAN_QRSV(chp, pl);
			return (error);
		}

		error = ws_notify(chp, XQENAB);
		if (error) {
			RESET_CHAN_QRSV(chp, pl);
			return (error);
		}

		RESET_CHAN_QRSV(chp, pl);
		break;

	case KDQUEMODE:
	default:
		if (arg) {	/* enable queue mode */

			pl = LOCK(chp->ch_mutex, plstr);
			while (CHNFLAG(chp, CHN_QRSV)) {
				/* PZERO+1 == primed-2 */
				if (!SV_WAIT_SIG(chp->ch_qrsvsv, primed - 2, 
						chp->ch_mutex))
					return (EINTR);
				pl = LOCK(chp->ch_mutex, plstr);
			}
			UNLOCK(chp->ch_mutex, pl);

			if (xqp->xq_proc && !proc_valid(xqp->xq_proc))
				xq_close(xqp);

			if (xqp->xq_queue)	/* already in queue mode */
				return (EBUSY);

			pl = LOCK(chp->ch_mutex, plstr);
			chp->ch_flags |= CHN_QRSV;
			UNLOCK(chp->ch_mutex, pl);

			if (copyin((caddr_t) arg, (caddr_t) &qmode, 
					sizeof(qmode)) < 0) {
				RESET_CHAN_QRSV(chp, pl);
				return (EFAULT);
			}

			qmode.qaddr = xq_init(xqp, qmode.qsize, 
						qmode.signo, &error);
			if (!qmode.qaddr || error) {
				RESET_CHAN_QRSV(chp, pl);
				return (error ? error : EFAULT);
			}
	
			error = ws_notify(chp, XQENAB);
			if (error) { 
				RESET_CHAN_QRSV(chp, pl);
				return (error);
			}

			if (copyout((caddr_t) &qmode, (caddr_t) arg, 
					sizeof(qmode)) < 0) {
				(void) ws_notify(chp, XQDISAB);
				xq_close(xqp);
				RESET_CHAN_QRSV(chp, pl);
				return (EFAULT);
			}

			RESET_CHAN_QRSV(chp, pl);

		} else if (xqp->xq_queue) { /* disable queue mode */
			void *procp;

			procp = proc_ref();
			if (procp != xqp->xq_proc) {
				proc_unref(procp);
				return (EACCES);
			}
			proc_unref(procp);

			error =  ws_notify(chp, XQDISAB);
			if (error)
				return (error);

			xq_close(xqp);
		}
	} /* switch */

	return (0);
}


/*
 * int
 * ws_xquemsg(ws_channel_t *, long)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	An ack or a nack message is sent by the char module to
 *	indicate that the stream is set to X/queue mode or not.
 */
int
ws_xquemsg(ws_channel_t *chp, long reply)
{
	pl_t pl;


	pl = LOCK(chp->ch_mutex, plstr);	

	if (reply == CH_XQENAB_NACK)
		chp->ch_flags &= ~CHN_QRSV;

	chp->ch_flags &= ~CHN_HIDN;
	SV_SIGNAL(chp->ch_xquesv, 0);

	UNLOCK(chp->ch_mutex, pl);

	return (0);
}


/*
 * WS routine for performing ioctls. Allows the mouse add-on to
 * be protected from cdevsw[] dependencies
 */

/*
 * int
 * ws_open(dev_t, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
int
ws_open(dev_t dev, int flag, int otyp, cred_t *cr)
{
	int error;
	dev_t olddev;

	olddev = dev;
	error = drv_devopen(&dev, flag, VCHR, B_FALSE, cr);
	ASSERT(dev == olddev);	/* no clones, thanks */
	return error;
}

int
ws_close(dev_t dev, int flag, int otyp, cred_t *cr)
{
	return drv_devclose(dev, flag, VCHR, cr);
}

/*
 * int
 * ws_read(dev_t, struct uio *, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
int
ws_read(dev_t dev, struct uio *uiop, cred_t *cr)
{
	return drv_devread(dev, uiop, 0, VCHR, cr);
}


/*
 * int
 * ws_write(dev_t, struct uio *, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
int
ws_write(dev_t dev, struct uio *uiop, cred_t *cr)
{
	return drv_devwrite(dev, uiop, 0, VCHR, cr);
}


/*
 * int
 * ws_ioctl(dev_t, int, int, int, cred_t *, int *)
 *
 * Calling/Exit State:
 *	None.
 */
int
ws_ioctl(dev_t dev, int cmd, int arg, int mode, cred_t *crp, int *rvalp)
{
	return drv_devioctl(dev, cmd, arg, mode, VCHR, crp, rvalp);
}


/*
 * int
 * ws_ck_kd_port(vidstate_t *, ushort)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
int
ws_ck_kd_port(vidstate_t *vp, ushort port)
{
	int	cnt;


	for (cnt = 0; cnt < MKDIOADDR; cnt++) {
		if (vp->v_ioaddrs[cnt] == port)
			return (1);
		if (!vp->v_ioaddrs[cnt])
			break;
	}

	return (0);
}


/*
 * void
 * ws_winsz(queue_t *, mblk_t *, ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_winsz(queue_t *qp, mblk_t *mp, ws_channel_t *chp, int cmd)
{
	vidstate_t *vp = &chp->ch_vstate;
	mblk_t *tmp;


	switch (cmd) {
	case TIOCGWINSZ: {
		struct winsize	*winp;

		if ((tmp = allocb(sizeof(struct winsize), BPRI_MED)) == 
					(mblk_t *) NULL) {
			/*
			 *+ There isn't enough memory available to allocate
			 *+ winsize message block.
			 */
			cmn_err(CE_WARN, 
				"!ws_winsz: can't get msg for reply to TIOCGWINSZ");
			freemsg(mp);
			break;
		}

		/* LINTED pointer alignment */
		winp = (struct winsize *)tmp->b_rptr;
		winp->ws_row = (ushort)(WSCMODE(vp)->m_rows & 0xffff);
		winp->ws_col = (ushort)(WSCMODE(vp)->m_cols & 0xffff);
		winp->ws_xpixel = (ushort)(WSCMODE(vp)->m_xpels & 0xffff);
		winp->ws_ypixel = (ushort)(WSCMODE(vp)->m_ypels & 0xffff);
		tmp->b_wptr += sizeof(struct winsize);
		ws_copyout(qp, mp, tmp, sizeof(struct winsize));
		break;
	}
	default:
		break;
	}
}


/* 
 * int
 * ws_getctty(dev_t *)
 *
 * Calling/Exit State:
 *	- Returns the controlling tty device number in devp.
 */
int
ws_getctty(dev_t *devp)
{
	sess_t *sp;


	sp = u.u_procp->p_sessp;

	if (sp->s_vp == NULL)
		return EIO;

	*devp = sp->s_vp->v_rdev; 

	return 0;
}


/*
 * int
 * ws_getvtdev(dev_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
int
ws_getvtdev(dev_t *devp)
{
        dev_t	ttyd;
        int     majnum, error;
	pl_t	pl;


	pl = LOCK(gvid_mutex, plhi);

	while (gvidflg & GVID_ACCESS) /* sleep */
		/* In SVR4 the priority value was set to STOPRI */
                if (!SV_WAIT_SIG(gvidsv, primed - 3, gvid_mutex)) {
			/*
                         * even if ioctl was not ours, we've
                         * effectively handled it 
			 */
			return (EINTR);
                }

        gvidflg |= GVID_ACCESS;

	UNLOCK(gvid_mutex, pl);

	if ((error = ws_getctty(&ttyd)) != 0)
		return (error);

	majnum = getmajor(ttyd);

	pl = LOCK(gvid_mutex, plhi);

	/*
         * return /dev/console if controlling tty is not gvid 
	 */
	if (majnum != Gvid.gvid_maj)
		*devp = makedevice(Gvid.gvid_maj, 0);
	else
		*devp = ttyd;

	gvidflg &= ~GVID_ACCESS;

	SV_SIGNAL(gvidsv, 0);

	UNLOCK(gvid_mutex, pl);

	return (0);
}


/*
 * void
 * ws_scrnres(ulong *, ulong *)
 *
 * Calling/Exit State:
 *	Return (via two pointers to longs) the screen resolution for the
 *	active channel.  For text modes, return the number of columns and
 *	rows, for graphics modes, return the number of x and y pixels.
 *
 * NOT CALLED
 */
void
ws_scrnres(ulong *xp, ulong *yp)
{
	vidstate_t	*vp = &(ws_activechan(&Kdws)->ch_vstate);


	if (!WSCMODE(vp)->m_font) {	/* graphics mode */
		*xp = WSCMODE(vp)->m_xpels;
		*yp = WSCMODE(vp)->m_ypels;
	} else {			/* text mode */
		*xp = WSCMODE(vp)->m_cols;
		*yp = WSCMODE(vp)->m_rows;
	}
}


/*
 * The following routines support COFF-based SCO applications that
 * use KD driver ioctls that overlap with STREAMS ioctls.
 */

#define SVR3	3
#define SVR4	4

#define	WS_ALLOC_FLGS(cflgs, mno, size) { \
		(cflgs) = kmem_zalloc( \
			(BITMASK_NWORDS((mno))*(size)), KM_NOSLEEP); \
		if ((cflgs) == NULL) { \
			/* \
			 *+ Out of memory. Check memory configured in
			 *+ the system. \
			 */ \
			cmn_err(CE_WARN, \
				"WS_ALLOC_FLGS: out of memory"); \
			return; \
		} \
	}

#define	WS_REALLOC_FLGS(omno, nmno, ocflgs, ncflgs, size) { \
		WS_ALLOC_FLGS((ncflgs), ((nmno)+1), (size)); \
		bcopy((ocflgs), (ncflgs), BITMASK_NWORDS((omno))); \
		kmem_free((ocflgs), ((BITMASK_NWORDS((omno)))*(size))); \
		(ocflgs) = (ncflgs); \
	}
		

/*
 * void
 * ws_reallocflgs(int)
 * 
 * Calling/Exit State:
 *	- If kmem_zalloc fails then ws_reallocflgs() will return
 *	  without setting the global ws_maxminor number.
 */
void
ws_reallocflgs(int nmno)
{
	uint_t	*nsvrcflgs;
	int	nws_maxminor = ws_maxminor;
	struct kdvdc_proc *nkdvdc_vt;


	if (nmno <= ws_maxminor)
		return;

	while (nmno > nws_maxminor) 
		/* round up to the next power of 2 */
		nws_maxminor = (nws_maxminor >> 1) << 2;

	WS_REALLOC_FLGS(ws_maxminor, nws_maxminor, 
				ws_compatflgs, nsvrcflgs, sizeof(uint_t));
	WS_REALLOC_FLGS(ws_maxminor, nws_maxminor, 
				ws_svr3_compatflgs, nsvrcflgs, sizeof(uint_t));
	WS_REALLOC_FLGS(ws_maxminor, nws_maxminor, 
				ws_svr4_compatflgs, nsvrcflgs, sizeof(uint_t));
	WS_REALLOC_FLGS(ws_maxminor, nws_maxminor, 
				kdvdc_vt, nkdvdc_vt, sizeof(struct kdvdc_proc));
	ws_maxminor = nws_maxminor;
}

	
/*
 * void
 * ws_setcompatflgs(dev_t)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_setcompatflgs(dev_t dev)
{
        void	ws_sysv_clrcompatflgs(dev_t, int);


	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor) {
		ws_reallocflgs(getminor(dev));
	}

	if (getminor(dev) > ws_maxminor)
		return;

	BITMASKN_SET1(ws_compatflgs, getminor(dev));
	ws_sysv_clrcompatflgs(dev, SVR3);
	ws_sysv_clrcompatflgs(dev, SVR4);
}


/*
 * void
 * ws_clrcompatflgs(dev_t)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_clrcompatflgs(dev_t dev)
{
	if (getminor(dev) > ws_maxminor)
		return;

	BITMASKN_CLR1(ws_compatflgs, getminor(dev));
}


/*
 * int
 * ws_iscompatset(dev_t)
 *
 * Calling/Exit State:
 *	- Return 1 if ws_compatflgs is set for the dev minor number.
 */
int
ws_iscompatset(dev_t dev)
{
	if (getminor(dev) > ws_maxminor)
		return (0);

	return (BITMASKN_TEST1(ws_compatflgs, getminor(dev)));
}


/*
 * void
 * ws_initcompatflgs(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_initcompatflgs(void)
{
	if (ws_compatflgs == NULL) {
		WS_ALLOC_FLGS(ws_compatflgs, ws_maxminor, sizeof(uint_t));
	}

	BITMASKN_CLRALL(ws_compatflgs, BITMASK_NWORDS(ws_maxminor));
}


/*
 * void
 * ws_sysv_setcompatflgs(dev_t, int)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_sysv_setcompatflgs(dev_t dev, int arg)
{
	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor) {
		ws_reallocflgs(getminor(dev));
	}

	if (getminor(dev) > ws_maxminor)
		return;

	if (arg == SVR3) {
		BITMASKN_SET1(ws_svr3_compatflgs, getminor(dev));
	} else {
		BITMASKN_SET1(ws_svr4_compatflgs, getminor(dev));
	}

        ws_clrcompatflgs(dev);
}


/*
 * void
 * ws_sysv_clrcompatflgs(dev_t, int)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_sysv_clrcompatflgs(dev_t dev, int arg)
{
	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor)
		return;

	if (arg == SVR3) {
		BITMASKN_CLR1(ws_svr3_compatflgs, getminor(dev));
	} else {
		BITMASKN_CLR1(ws_svr4_compatflgs, getminor(dev));
	}
}


/*
 * int
 * ws_sysv_iscompatset(dev_t, int)
 *
 * Calling/Exit State:
 *	- Return 1 if ws_svrx_compatflgs is set for dev minor no., 
 *	  otherwise return 0.
 */
int
ws_sysv_iscompatset(dev_t dev, int arg)
{
	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor)
		return (0);

	if (arg == SVR3) {
		return (BITMASKN_TEST1(ws_svr3_compatflgs, getminor(dev)));
	} else {
		return (BITMASKN_TEST1(ws_svr4_compatflgs, getminor(dev)));
	}
}


/*
 * void
 * ws_sysv_initcompatflgs(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_sysv_initcompatflgs(void)
{
	if (ws_svr3_compatflgs == NULL) {
		WS_ALLOC_FLGS(ws_svr3_compatflgs, (ws_maxminor + 1), 
					sizeof(uint_t));
	}
	BITMASKN_CLRALL(ws_svr3_compatflgs, BITMASK_NWORDS(ws_maxminor));

	if (ws_svr4_compatflgs == NULL) {
		WS_ALLOC_FLGS(ws_svr4_compatflgs, (ws_maxminor + 1), 
					sizeof(uint_t));
	}
	BITMASKN_CLRALL(ws_svr4_compatflgs, BITMASK_NWORDS(ws_maxminor));
}


/*
 * void
 * ws_set_vt_proc_info(int)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Store the process reference to remap its 3.2 ioctls.
 *	It is called by kdvmstr_doioctl().
 */
void
ws_set_vt_proc_info(int index)
{
	ASSERT(index <= maxminor);

	if (index > ws_maxminor)
		ws_reallocflgs(index);

	kdvdc_vt[index].kdvdc_procp = proc_ref();
}


/*
 * int
 * ws_isvdcset(dev_t)
 *
 * Calling/Exit State:
 *	- Return 1 if a valid process has the compatibility flag set
 *	  for dev, otherwise return 0.
 */
int
ws_isvdcset(dev_t dev)
{
	void	*p;
	void	*pref;
	

	ASSERT(getminor(dev) <= maxminor);

	if (getminor(dev) > ws_maxminor)
		return (0);

	if ((p = (void *) kdvdc_vt[getminor(dev)].kdvdc_procp) != NULL) {
		if (!(proc_valid(p))) {
			proc_unref(kdvdc_vt[getminor(dev)].kdvdc_procp);
			kdvdc_vt[getminor(dev)].kdvdc_procp = (void *) NULL;
			return (0);
		}
		
		pref = proc_ref();
		if (kdvdc_vt[getminor(dev)].kdvdc_procp == pref) {
			proc_unref(pref);
			return (1);
		}
		proc_unref(pref);
	}

	return (0);
}


/*
 * void
 * ws_initvdc_compatflgs(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
ws_initvdc_compatflgs(void)
{
	if (kdvdc_vt == NULL) {
		WS_ALLOC_FLGS(kdvdc_vt, (ws_maxminor + 1), 
					sizeof(struct kdvdc_proc));
	}
}

/* L000 begin
 *
 * Multiconsole support routines
 */

/*
 * int
 * ws_con_LWP_off_proc(struct hook_args *args, proc_t *procp)
 *
 * Calling/Exit State:
 *	Called from the context switch handler in the context of the incoming
 *	LWP.
 *
 * Description:
 *	Deliver the off processor event to the appropriate driver.
 *
 * Note:
 *	This function is extremely performance critical.
 */
int
ws_con_LWP_off_proc(struct hook_args *args, proc_t *procp)
{
pl_t old_spl;
register int i;
register pid_t proc_id = procp->p_pidp->pid_id;

	if(!ws_mcon_active || ws_cur_drv == NONE_ON_PROC)
		return 0;

	/*
	 * We are making the assumtion here that the proc we expect to go
	 * off proc has. If not there is a bug somewhere and we should fix it.
	 */
	if(ws_release_pending){
		/*
		 * This is bad, we have a release pending
		 * which means someone still has access to the
		 * text memory.
		 * Deliver the resume, and the off proc event
		 * now and fool ws_con_done_text to deliver
		 * the correct on proc event.
		 */
		ASSERT(ws_cur_drv != NONE_ON_PROC);

		cmn_err(CE_WARN, "!WS: LWP off resuming");

		old_spl = splhi();

		(ws_disp[ws_cur_drv].resume_access)
				(ws_disp[ws_cur_drv].driver_data);

		ws_release_pending = B_FALSE;
	}
	else
		old_spl = splhi();

	(ws_disp[ws_cur_drv].off_proc)(ws_disp[ws_cur_drv].driver_data);
	splx(old_spl);

	ws_cur_drv = NONE_ON_PROC;

	return 0;

}	/* End fuction ws_con_LWP_off_proc */


/*
 * int
 * ws_con_LWP_on_proc(struct hook_args *args, proc_t *procp)
 *
 * Calling/Exit State:
 *	Called from the context switch handler in the context of the incoming
 *	LWP.
 *
 * Description:
 *	Deliver the on processor event to the appropriate driver.
 *
 * Note:
 *	This function is extremely performance critical.
 */
int
ws_con_LWP_on_proc(struct hook_args *args, proc_t *procp)
{
pl_t old_spl;
proc_t *parent_procp;
register int i;
register pid_t proc_id = procp->p_pidp->pid_id;


	if(!ws_mcon_active)
		return 0;

	/*
	 * Search for the pid_t, note that if we don't find it things get a
	 * little more heavyweight, but that should be extremely rare.
	 */
	for(i=0; i<ws_max_active_callouts; i++){

		if(ws_disp[i].proc_id == proc_id){
			/*
			 * Found it, set up the correct state and deliver the
			 * event after checking there is no resume pending,
			 * which hopefully won't ever happen.
			 */
found_parent:
			if(ws_release_pending){
				/*
				 * This is bad, we have a release pending
				 * which means we have missed an off proc
				 * event somewhere. We have to deliver the
				 * resume AND the off proc now. Yuck!
				 */
				ASSERT(ws_cur_drv != NONE_ON_PROC);

				cmn_err(CE_WARN,"!WS: LWP on missed off event");

				old_spl = splhi();

				(ws_disp[ws_cur_drv].resume_access)
					(ws_disp[ws_cur_drv].driver_data);

				(ws_disp[ws_cur_drv].off_proc)
					(ws_disp[ws_cur_drv].driver_data);

				ws_release_pending = B_FALSE;
			}
			else
				old_spl = splhi();

			(ws_disp[i].on_proc)(ws_disp[i].driver_data);
			splx(old_spl);

			ws_cur_drv = i;

			return 0;
		}
	}	/* End for all active callouts */

	/*
	 * We didn't find it so we have to search the parent chain here, if
	 * we find the PID in the parent chain we try to make a new dispatch
	 * table entry to avoid having to do it again.
	 */

#if 0	/* L008 */
	cmn_err(CE_NOTE, "!WS: LWP on didn't find PID");
#endif
	ws_callout_not_found += 1;

	parent_procp = procp;

	do{
		for(i=0; i<ws_max_active_callouts; i++){

			if(ws_disp[i].proc_id == parent_procp->p_ppid){
				/*
				 * Found the parent, copy its dispatch table
				 * entry into a new entry then make the
				 * dispatch the appopriate events.
				 */
				register int j = i;

				/* L008 begin
				 * Hahahack
				 */
				goto found_parent;
#if 0	/* L008 end */
				for(i=0; i<ws_max_callouts; i++){
					if(ws_disp[i].proc_id == (pid_t)0){
						
						ws_disp[i].proc_id = proc_id;
						ws_disp[i].procp =
							parent_procp->p_parent;

						ws_disp[i].on_proc = 
							ws_disp[j].on_proc;

						ws_disp[i].off_proc = 
							ws_disp[j].off_proc;

						ws_disp[i].release_access = 
					 	     ws_disp[j].release_access;

						ws_disp[i].resume_access = 
						      ws_disp[j].resume_access;
						
						ws_disp[i].driver_data = 
							ws_disp[j].driver_data;

						ws_disp[i].driver_tag = 
							ws_disp[j].driver_tag;

						ws_disp[i].ref = ws_disp[j].ref;

						/* L008
						 * Bump the count
						 */
						ws_max_active_callouts += 1;

						goto found_parent;

					}

				}	/* End search for empty ws_disp */
				/*
				 * BUG:
				 *	We should grow the table here.
				 */
				cmn_err(CE_PANIC, "WS: LWP on disp tab full");

#endif	/* L008 */
			}	/* End if parent in table*/

		}	/* End for all active callouts */

		parent_procp = parent_procp->p_parent;

	}while(parent_procp->p_ppid > 0);	/* Stop at sysproc */
	


}	/* End function ws_con_LWP_on_proc */

/*
 * void
 * ws_con_drv_detach(void *driver_data, pid_t proc_id)
 *
 * Calling/Exit State:
 *	None
 *
 * Description:
 *	Remove the process identified by driver_data or proc_id from the
 *	ws_disp dispatch table. Uses proc_id first then driver_data to find
 *	the table entry to remove, once found the process is unbound, and
 *	callouts stopped if neccessary.
 */
void
ws_con_drv_detach(void *driver_data, pid_t proc_id)
{
boolean_t found = B_FALSE;
pl_t old_spl;
int i;

	if(!ws_mcon_active){

		cmn_err(CE_WARN, "!WS: ws_con_drv_detach no callouts defined");
		return;
	}

	if(proc_id != (pid_t)0)
		for(i=0; i<ws_max_active_callouts; i++)
			if(ws_disp[i].proc_id == proc_id)
				found = B_TRUE;

	if(!found && driver_data != (void *)NULL)
		for(i=0; i<ws_max_active_callouts; i++)
			if(ws_disp[i].driver_data == driver_data)
				found = B_TRUE;

	if(!found){					/* L001 begin */

		cmn_err(CE_WARN, "!WS: ws_con_drv_detach disp entry not found");
		return;
	}						/* L001 end */

	/*
	 * Remove the entry and stop the callouts if necessary
	 * Make sure this entry is not active first, if it is deliver
	 * the appropriate relinquish event.
	 */
	if(ws_cur_drv == i){
		/*
		 * This guy has the memory mapped so deliver the off
		 * proc event before deleting the entry.
		 */
		old_spl = splhi();

		if(ws_release_pending){
			/*
			 * And there's a release pending so deliver the
			 * resume_access event before the off proc
			 * event.
			 */
			(ws_disp[i].resume_access)
					(ws_disp[i].driver_data);

			ws_release_pending = B_FALSE;

		}

		(ws_disp[i].off_proc)(ws_disp[i].driver_data);
		splx(old_spl);
		ws_cur_drv = NONE_ON_PROC;
	}
	/*
	 * Unbind the process and disable its callouts.
	 */
	if(proc_valid(ws_disp[i].ref)){			/* L001 begin */
		/*
		 * Don't unbind if we were asked not to bind this proc
		 */
		if(!(ws_disp[i].flags & CON_LWP_NO_BIND));
			unbindproc(ws_disp[i].ref);

		lwp_callback_enable(SWTCH_IN1_DETACH, ws_disp[i].procp->p_lwpp);
		lwp_callback_enable(SWTCH_OUT1_DETACH,ws_disp[i].procp->p_lwpp);

		proc_unref(ws_disp[i].ref);

	}						/* L001 end */

	/*
	 * Now remove this entry and check if we can completely disable the
	 * context switch callouts. Note: We don't need to zap the entire
	 * table entry.
	 */
	ws_disp[i].proc_id = (pid_t)0;
	ws_disp[i].procp = (proc_t *)NULL;
	ws_disp[i].driver_data = (void *)NULL;			/* L001 */
	ws_disp[i].driver_tag = 0;
	ws_disp[i].ref = (void *)NULL;				/* L001 */

	found = B_FALSE;

	for(i=0; i<ws_max_active_callouts; i++)
		if(ws_disp[i].proc_id != (pid_t)0)
			found = B_TRUE;

	if(!found){
		/*
		 * Set the dispatch table and all variables back to the
		 * init state. Then disable the context switch callouts
		 */
		ws_max_active_callouts = 0;
		ws_release_pending = B_FALSE;
		ws_cur_drv = NONE_ON_PROC;
		ws_mcon_active = B_FALSE;

		lwp_callback_enable(SWTCH_IN1_DETACH, (lwp_t *)NULL);
		lwp_callback_enable(SWTCH_OUT1_DETACH, (lwp_t *)NULL);

	}
	
	return;


}	/* End function ws_con_drv_detach */


/* L003 begin
 *
 * int
 * ws_con_alloc_mem(void)
 *
 * Calling/Exit State:
 *	Called from ws_con_drv_init or ws_con_maybe_bind. KD is currently
 *	bound to the base CPU so no multiconsole locks are held, if KD is
 * 	unbound we should lock around calls to this function.
 *
 * Description:
 *	Allocate memory via KMA for internal structures. Note, this function
 *	should be used to grow all tables if / when we support dynamic sizing
 *	of the multiconsole structures.
 */
int
ws_con_alloc_mem(void)
{
static boolean_t FirstTime = B_TRUE;

	if(!FirstTime)		/* L005 */
		return 0;	/* L005 */

	/*
	 * Allocate memory for the tables
	 */
	ws_bind_proc = (ws_bind_t *)kmem_zalloc(
		   sizeof(ws_bind_t) * ws_max_callouts, KM_NOSLEEP);

	ws_disp = (ws_dispatch_t *)kmem_zalloc(
		   sizeof(ws_dispatch_t) * ws_max_callouts, KM_NOSLEEP);

	if(ws_bind_proc == (ws_bind_t *)NULL ||ws_disp ==(ws_dispatch_t *)NULL){

		if(ws_bind_proc != (ws_bind_t *)NULL){
			kmem_free(ws_bind_proc,
				sizeof(ws_bind_t) * ws_max_callouts);

			cmn_err(CE_WARN, "WS: ws_con_alloc_mem failed to "
					"allocate memory for bind table");
		}

		if(ws_disp != (ws_dispatch_t *)NULL){
			kmem_free(ws_disp,
				sizeof(ws_dispatch_t)* ws_max_callouts);

		cmn_err(CE_WARN, "WS: ws_con_alloc_mem failed to "
					"allocate memory for dispatch table");
		}

		return ENOMEM;

	}	/* End if kmem_zalloc failed */

	FirstTime = B_FALSE;			/* L005 */

	return 0;

}	/* End function ws_con_alloc_mem, L003 end */


/* L004 begin
 *
 * boolean_t
 * ws_con_find_pending_proc(proc_t *procp, unsigned int *index)
 *
 * Calling/Exit state:
 *	Called with user context from ws_con_maybe_bind.
 *
 * Description:
 *	Search the ws_bind_proc table for procp, return B_TRUE if found and set
 *	index to the index into the array.
 * 
 */
boolean_t
ws_con_find_pending_proc(proc_t *procp, unsigned int *index)
{
	for(*index=0; *index<ws_pending_bind; *index++)
		if(ws_bind_proc[*index].procp == procp)
			return B_TRUE;

	return B_FALSE;

}	/* End function ws_con_find_pending_proc, L004 end */


/*
 * void
 * ws_con_maybe_bind(proc_t procp, boolean_t forget)
 *
 * Calling/Exit State:
 *	Called with user context by KD
 *
 * Description:
 *	If there are no active multiconsole devices add this proc_id to the
 *	list that should be bound if a multiconsole starts, otherwise bind the
 *	process now.
 *
 * L003 changed to return a status code
 *
 * L004 Note:
 *	Now only allows a proc to be in the pending bind table once, but don't
 *	return an error on any attempts to add the same proc multiple times.
 *	Also note it is still an error to try to remove a bogus entry from the
 *	list.
 *
 * L009 Note:
 *	Nore returns EAGAIN if a remove operation only decrements the ref
 *	count. ESRCH is again returned if the process is not found in the
 *	pending bind table.
 *	Also note that we now unref the process after when we bind it to
 *	prevent it from becoming a zombie when it exits. If it needs to be
 *	referenced during its bound lifetime that should be taken care of
 *	elsewhere.
 */
int
ws_con_maybe_bind(proc_t *procp, boolean_t forget)
{
static boolean_t first_time = B_TRUE;	/* L003 */
int rv;					/* L004 */
unsigned int i, j;

	if(first_time){			/* L003 begin */

		rv = ws_con_alloc_mem();
		
		if(rv) return rv;

		first_time = B_FALSE;

	}				/* L003 end */

	if(forget == B_TRUE){
		/*
		 * Remove this proc_t from the list to be bound, shuffle all
		 * the rest up and NULL the last pointer.
		 */
		if(ws_con_find_pending_proc(procp, &i)){	/* L004 */
			/*
			 * L009 note
			 * Decrement the map count and if zero remove the
			 * whole entry from the table
			 */
			if(ws_bind_proc[i].count-- == 1){	/* L009 */
				/*
				 * We can proc_unref now.
				 * Note that we never bound any processes still
				 * in this list so we must not unbind them.
				 */
				proc_unref(ws_bind_proc[i].ref);

				for(j=i; j<ws_pending_bind-1; j++){

					ws_bind_proc[j].procp =
						ws_bind_proc[j+1].procp;

					ws_bind_proc[j].ref =
						ws_bind_proc[j+1].ref;

					/* L009
					 * Shuffle the count up also.
					 */
					ws_bind_proc[j].count =
						ws_bind_proc[j+1].count;
				}

				ws_bind_proc[j].procp = (proc_t *)NULL;
				ws_bind_proc[j].ref = (void *)NULL;
				ws_pending_bind -= 1;

				rv = 0;

			}	/* End if count == 0, L009 begin */
			else
				rv = EAGAIN;		/* L009 end */
		}
		else{					/* L004 begin */

			cmn_err(CE_WARN, "!WS: ws_con_maybe_bind remove fail");
			rv = ESRCH;
		}

		return rv;				/* L004 end */

	}	/* End if forget == B_TRUE */

	if(ws_mcon_active == B_TRUE || ws_pending_bind >= ws_max_callouts){
		/*
		 * There is an active multiconsole device so we need to bind
		 * this process now, also check if there are other processes
		 * on the queue to be bound. There shouldn't be.
		 */
		if(ws_pending_bind >= ws_max_callouts)
			cmn_err(CE_WARN, "!WS: ws_con_maybe_bind force bind");

		if(bindproc(ws_processor, MYSELF))		/* L001, L002 */
			cmn_err(CE_WARN, "!WS: ws_con_maybe_bind bind failed");

		if(ws_pending_bind){
			/*
			 * This should never happen, anything on the bind list
			 * should have been bound when the first multiconsole
			 * went active, but just in case we bind everything
			 * here. Note that if a bind fails we could leave that
			 * proc on the list but by the time it would get a
			 * chance to be bound again I think the world would
			 * already have become a very smelly place.
			 *
			 * L004 begin and note:
			 *	This can happen if we have to many pending
			 * bind entries, in which case we don't want to make
			 * the cmn_err.
			 */
			if(ws_pending_bind < ws_max_callouts)
				cmn_err(CE_WARN, "!WS: ws_con_maybe_bind "
						"mcon and !0");	/* L004 end */

			for(i=0; i<ws_pending_bind; i++){

				if(!proc_valid(ws_bind_proc[i].ref)){
					/*
					 * Process has exited.
					 *
					 * L009 begin and note
					 *	- unref the proc here so that
					 *	  the zombie can be cleaned up
					 */
					proc_unref(ws_bind_proc[i].ref);
					ws_bind_proc[i].count = 0;/* L009 end */

					ws_bind_proc[i].procp = (proc_t *)NULL;
					ws_bind_proc[i].ref = (void *)NULL;

					continue;
				}
								/* L001, L002 */
				if(bindproc(ws_processor, ws_bind_proc[i].ref))
					cmn_err(CE_WARN,
					  "!WS: ws_con_maybe_bind bind failed");

				proc_unref(ws_bind_proc[i].ref);/* L009 */
				ws_bind_proc[i].count = 0;	/* L009 */
				ws_bind_proc[i].procp = (proc_t *)NULL;
				ws_bind_proc[i].ref = (void *)NULL;
			}

			ws_pending_bind = 0;	/* We've bound them all */
		}

		return 0;
	}

	/* L004 begin
	 *
	 * Check if we already have an entry for this proc in the pending
	 * bind table and if so just return OK.
	 *
	 * L009 note
	 *	- We now increment the count of maps made against this
	 *	  channel if we already have an entry in the table.
	 *
	 * Note:
	 *	Don't do the proc_ref if we already have an entry because 
	 *	we only expect to get one call to delete the pending bind from
	 *	KD.
	 */
	if(!ws_con_find_pending_proc(procp, &i)){
		/*
		 * Nothing nasty has happened so just queue this proc_t
		 * for binding later.
		 */
		ws_bind_proc[ws_pending_bind].count = 1;	/* L009 */
		ws_bind_proc[ws_pending_bind].procp = procp;
		ws_bind_proc[ws_pending_bind++].ref = proc_ref();

	}					/* End if !found, L004 end */
	else					/* L009 */
		ws_bind_proc[i].count += 1;	/* L009 */

	return 0;

}	/* End function ws_con_maybe_bind */

/*
 * int
 * ws_con_need_text(void)
 *
 * Calling/Exit State:
 *	Return 0 for sucess non-zero for error
 *
 * Description:
 *	If there is non-local video memory mapped in call the appropriate
 *	multiconsole driver to have it temporarily mapped out
 *
 * Note:
 *	We only set the release_pending flag if we actually do a release, this
 *	gives us licence to not deliver a resume because the pairing is not
 *	guaranteed for normal on / off proc callouts.
 */
int
ws_con_need_text(void)
{
pl_t old_spl;

	if(!ws_mcon_active || ws_cur_drv == NONE_ON_PROC)
		return 0;	/* No non-local video memory mapped in */

	/*
	 * Deliver the release / off proc event to the currently active driver
	 */
	if(ws_disp[ws_cur_drv].release_access){

		old_spl = splhi();
		(ws_disp[ws_cur_drv].release_access)
					(ws_disp[ws_cur_drv].driver_data);

		ws_release_pending = B_TRUE;

	}
	else{
		old_spl = splhi();
		(ws_disp[ws_cur_drv].off_proc)(ws_disp[ws_cur_drv].driver_data);
	}

	splx(old_spl);

	return 0;

}	/* End function ws_con_need_text */


/*
 * int
 * ws_con_done_text(void)
 *
 * Calling/Exit State:
 *	Return 0 
 *
 * Description:
 *	Call the currently active driver to allow it to restore its mapping
 *	to the video memory
 */
int
ws_con_done_text(void)
{
pl_t old_spl;

	if(!ws_release_pending)
		return 0;	/* No outstanding release event */

	if(ws_cur_drv == NONE_ON_PROC){

		cmn_err(CE_NOTE, "!WS: ws_con_done_text, none on proc");

		return 0;
	}

	/*
	 * Deliver the resume / on proc event to the currently active driver
	 */
	if(ws_disp[ws_cur_drv].resume_access){

		old_spl = splhi();
		(ws_disp[ws_cur_drv].resume_access)
					(ws_disp[ws_cur_drv].driver_data);
	}
	else{
		old_spl = splhi();
		(ws_disp[ws_cur_drv].on_proc)(ws_disp[ws_cur_drv].driver_data);
	}

	splx(old_spl);

	ws_release_pending = B_FALSE;

	return 0;

}	/* End function ws_con_done_text */

/*
 * int
 * ws_con_drv_init2(con_init_state_t *initp, long iocid, queue_t *qp)
 *
 * Calling/Exit State:
 *	Returns 0 for success or a non-zero failure code.
 *
 * Description:
 *	Called with user context by the stream head via strioccall. Dispatch
 *	the call out to the driver callback function, set up the dispatch table
 *	entry for this PID, generate the driver tag and enable the context
 *	switch callouts if they are not already running.
 *
 * Note:
 *	We should generate the driver tag here that will allow us to optimise
 *	on and off processor callouts.
 */
int
ws_con_drv_init2(con_init_state_t *initp, long iocid, queue_t *qp)
{
boolean_t space = B_FALSE;
int i;

	/* L005
	 * 
	 * Add check for initp->mbp NULL to the following list and also
	 * typecast all the NULL's correctly.
	 */
	if(initp == (con_init_state_t *)NULL ||
			initp->driver_init == (int (*)())NULL ||
			initp->mbp == (mblk_t *)NULL){
		/*
		 * This is major bad, we already checked the args once!
		 */
		cmn_err(CE_PANIC, "WS: ws_con_drv_init2 bogus strioccall");
		return 0;
	}

	if((initp->driver_init)(initp)){
		/*
		 * Driver init callback returned an error
		 */
		cmn_err(CE_NOTE,"!WS: ws_con_drv_init callback returned error");
		return;
	}

	/* L005 begin
	 * If this process already has an entry in the dispatch table just
	 * return here with the success code.
	 */
	for(i=0; i<ws_max_callouts; i++)
		if(ws_disp[i].proc_id == u.u_procp->p_pidp->pid_id)
			return 0;

	/*
	 * We are done with the original ioctl message and the calling driver
	 * will never get to see it again so we can free it here.
	 */
	freemsg(initp->mbp);

	/*
	 * Everything looks OK, so setup the dispatch table entry and enable
	 * the context switch hooks, note that although we already checked
	 * for free space in ws_con_drv_init there is a small chance we could
	 * have filled it in ws_con_LWP_on_proc so we have to check again here.
	 */
	if(ws_max_active_callouts >= ws_max_callouts){
		for(i=0; i<ws_max_callouts; i++)
			if(ws_disp[i].proc_id == (pid_t)0){
				space = B_TRUE;
				break;
			}
	}
	else{
		/*
		 * It's OK to use the last entry
		 */
		i = ws_max_active_callouts++;
		space = B_TRUE;
	}

	if(!space){
		cmn_err(CE_WARN, "!WS: ws_con_drv_init surprised tables full");

		return ENOMEM;	/* This return code is ignored anyway */
	}
	
	ws_disp[i].proc_id = u.u_procp->p_pidp->pid_id;
	ws_disp[i].procp = u.u_procp;
	ws_disp[i].on_proc = initp->on_proc;
	ws_disp[i].off_proc = initp->off_proc;
	ws_disp[i].release_access = initp->release_access;
	ws_disp[i].resume_access = initp->resume_access;
	ws_disp[i].driver_data = initp->driver_data;
	ws_disp[i].driver_tag = 0;
	ws_disp[i].flags = initp->flags;		/* L001 */
	ws_disp[i].ref = proc_ref();		/* unref in detach, L001 */

	if(!(initp->flags & CON_LWP_NO_BIND))
		bindproc(ws_processor, MYSELF);	/* Bind now, L001, L002 */

	if(!ws_mcon_active){
		/*
		 * Start the context switch callouts for the first time
		 */
		lwp_callback(SWTCH_IN1_ATTACH, ws_con_LWP_on_proc, NULL);
		lwp_callback(SWTCH_OUT1_ATTACH, ws_con_LWP_off_proc, NULL);

		ws_mcon_active = B_TRUE;

	}
	/*
	 * Enable callbacks for this process
	 */
	lwp_callback_enable(SWTCH_IN1_ATTACH, u.u_procp->p_lwpp);
	lwp_callback_enable(SWTCH_OUT1_ATTACH, u.u_procp->p_lwpp);

	/* L001 begin
	 *
	 * Bind all processes on the pending bind queue
	 */
	if(ws_pending_bind){

		for(i=0; i<ws_pending_bind; i++){

			if(!proc_valid(ws_bind_proc[i].ref)){
				/*
				 * Process has exited.
				 *
				 * L009 begin
				 * unref the process so we don't cause a
				 * zombie to hang around forever.
				 */
				proc_unref(ws_bind_proc[i].ref);
				ws_bind_proc[i].count = 0;	/* L009 end */

				ws_bind_proc[i].procp = (proc_t *)NULL;
				ws_bind_proc[i].ref = (void *)NULL;

				continue;
			}

			if(bindproc(ws_processor, ws_bind_proc[i].ref))/*L002*/
				cmn_err(CE_WARN,
					"!WS: ws_con_maybe_bind bind failed");

			proc_unref(ws_bind_proc[i].ref);	/* L009 */
			ws_bind_proc[i].procp = (proc_t *)NULL;
			ws_bind_proc[i].ref = (void *)NULL;
		}

		ws_pending_bind = 0;	/* We've bound them all */

	} /* End if ws_pending_bind, L001 end */

	return 0;	/* We are done, yipee! */

}	/* End function ws_con_drv_init2 */


/*
 * int
 * ws_con_drv_init(con_init_state_t *initp)
 *
 * Calling/Exit State:
 *	Return 0 for sucess non-zero for error
 *
 * Description:
 *	Validate all the parameters, allocate memory for tables if this is the
 *	first call, and arrange for strioccall to call ws_con_drv_init2 with
 *	user context to deal with the rest of the initialisation.
 *
 * Note:
 *	To change multiconsole support to be able to grow tables make a careful
 *	review of the use of ws_max_callouts and ws_max_active_callouts. Change
 *	ws_max_callouts to be the amount of space in the ws_disp table, make a
 *	new variable for the space in the ws_bind_proc table, remove the
 *	MAX_CALLOUTS tunable from ws.cf/Space.c and make a WS_ALLOC_UNIT
 *	define in this file.
 */
int
ws_con_drv_init(con_init_state_t *initp)
{
static boolean_t first_time = B_TRUE;
mblk_t *mp1;					/* L005 */


	/*
	 * Validate as much of the init args as possible.
	 */
	if(initp == NULL || initp->driver_init == NULL ||
		initp->on_proc == NULL || initp->off_proc == NULL ||
		initp->iocbp == NULL || initp->mbp == NULL){

		cmn_err(CE_WARN, "!WS: ws_con_drv_init Illegal init argument");

		return EINVAL;
			
	}	/* End if illegal init argument */

	if(initp->driver_data == 0)  /* This isn't fatal, but probably wrong */
		cmn_err(CE_NOTE, "!WS: ws_cond_drv_init driver_data NULL");

	/*
	 * Must have both or neither of these
	 *
	 * L005 change to cmn_err if both are NULL which is valid.
	 */
	if(initp->resume_access != NULL || initp->release_access != NULL)
		if(initp->resume_access == NULL ||
					initp->release_access == NULL){

			initp->resume_access = initp->release_access = NULL;
			cmn_err(CE_WARN, "!WS: ws_con_init_drv only one of "
				"resume/release entry points specified");
		}

	if(first_time){
		/*
		 * L003 begin
		 *
		 * Moved KMA code from here to ws_con_alloc_mem, here we now
		 * just call the new function
		 */
		int rv;

		rv = ws_con_alloc_mem();

		if(rv) return rv;		/* L003 end */

		first_time = B_FALSE;

	}	/* End if first_time */

	/*
	 * Now use strioccall to have the stream head call us back with
	 * user context
	 */
	if(strioccall(ws_con_drv_init2, initp,
				initp->iocbp->ioc_id, initp->qp)){
		/*
		 * Failed to schedule the callback, could be because there is
		 * already on in progess so tell the caller to try again.
		 */
		cmn_err(CE_NOTE,
		    "!WS: ws_con_drv_init failed to schedule init callback");

		return EAGAIN;
	}

	/* L005 begin
	 *
	 * ws_iocack trashes the mblk_t that it used but we must send it to
	 * have the STREAM mutex dropped before the strioccall will execute
	 * the call back. Copy the first block of the original message and 
	 * use the copy to ACK the ioctl. Note, we can't dup the message
	 * because that just increments the data block reference count.
	 */
	mp1 = copyb(initp->mbp);

	if(mp1 == (mblk_t *)NULL){
		/*
		 * Failed to copy the message block. Send an IOCNACK upstream
		 * which will cause the strioccall callback to be canceled.
		 */
		ws_iocnack(initp->qp, initp->mbp, initp->iocbp, ENOMEM);

		cmn_err(CE_WARN, "!WS: ws_con_drv_init failed to ACK ioctl");

		return ENOMEM;


	}	/* End mp1 == NULL */

	ws_iocack(initp->qp, mp1, (struct iocblk *)mp1->b_rptr); /* L005 end */

	return 0;		/* Looks good so far */


}	/* End function ws_con_drv_init */



/*
 * L000 end
 */

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_ws(void *, int)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Generic dump function that can be called (from kdb also) 
 *	to print the values of various data structures that are
 *	part of the integrated workstation environment. It takes
 *	two arguments: a data structure pointer whose fields are 
 *	to be displayed and the data structure type. Following
 *	are the values of data structure type:
 *		1 - wstation_t
 *		2 - ws_channel_t 
 *		3 - vidstate_t
 *		4 - termstate_t
 *		5 - kbstate_t
 *		6 - modeinfo
 *		7 - xqInfo
 */
void
print_ws(void *structp, int type)
{
	switch (type) {
	case 1: {
		wstation_t *wsp;

		/*
		 * wstation structure.
		 */

		wsp = (wstation_t *) structp;
		debug_printf("\n wstation struct: size=0x%x(%d)\n",
			sizeof(wstation_t), sizeof(wstation_t));
		debug_printf("\tw_flags=0x%x, \tw_qp=0x%x\n",
			wsp->w_flags, wsp->w_qp);
		debug_printf("\tw_vstatep=0x%x,\tw_tstatep=0x%x\n",
			&wsp->w_vstate, &wsp->w_tstate);
		debug_printf("\tw_mapp=0x%x, \tw_charmapp=0x%x\n",
			&wsp->w_map, &wsp->w_charmap);
		debug_printf("\tw_mp=0x%x, \tw_chanpp=0x%x\n",
			wsp->w_mp, wsp->w_chanpp);
		break;
	}

        case 2: {
		ws_channel_t *chp;

		/*
		 * ws_channel_info structure.
		 */

		chp = (ws_channel_t *) structp;
		debug_printf("\n channel struct: size=0x%x(%d)\n",
			sizeof(ws_channel_t), sizeof(ws_channel_t));
		debug_printf("\tch_id=0x%x, \tch_opencnt=0x%x,\n",
			chp->ch_id, chp->ch_opencnt);
		debug_printf("\tch_flags=0x%x, \tch_qp=0x%x,\n",
			chp->ch_flags, chp->ch_qp);
		debug_printf("\tch_kbstatep=0x%x, \tch_charmap_p=0x%x,\n",
			&chp->ch_kbstate, chp->ch_charmap_p);
		debug_printf("\tch_scrnp=0x%x, \tch_vstatep=0x%x,\n",
			&chp->ch_scrn, &chp->ch_vstate);
		debug_printf("\tch_tstatep=0x%x, \tch_strttyp=0x%x,\n",
			&chp->ch_tstate, &chp->ch_strtty);
		debug_printf("\tch_nextp=0x%x, \tch_prevp=0x%x,\n",
			chp->ch_nextp, chp->ch_prevp);
		debug_printf("\tch_xque=0x%x, \tch_iocarg=0x%x,\n",
			&chp->ch_xque, chp->ch_iocarg);
		break;
	}

	case 3: {
		vidstate_t *vp;
		int i;

		/*
		 * vidstate structure.
		 */

		vp = (vidstate_t *) structp;
		debug_printf("\n vidstate struct: size=0x%x(%d)\n",
			sizeof(vidstate_t), sizeof(vidstate_t));
		debug_printf("\tv_cmos=0x%x, \tv_type=0x%x,\n",
			vp->v_cmos, vp->v_type);
		debug_printf("\tv_cvmode=0x%x, \tv_dvmode=0x%x,\n",
			vp->v_cvmode, vp->v_dvmode);
		debug_printf("\tv_font=0x%x, \tv_colsel=0x%x,\n",
			vp->v_font, vp->v_colsel);
		debug_printf("\tv_modesel=0x%x, \tv_undattr=0x%x,\n",
			vp->v_modesel, vp->v_undattr);
		debug_printf("\tv_uline=0x%x, \tv_nfonts=0x%x,\n",
			vp->v_uline, vp->v_nfonts);
		debug_printf("\tv_border=0x%x, \tv_scrmsk=0x%x,\n",
			vp->v_border, vp->v_scrmsk);
		debug_printf("\tv_regaddr=0x%x\n",
			vp->v_regaddr);
		debug_printf("\tv_parampp=0x%x, \tv_fontp=0x%x,\n",
			vp->v_parampp, vp->v_fontp);
		debug_printf("\tv_rscr=0x%x, \tv_scrp=0x%x,\n",
			vp->v_rscr, vp->v_scrp);
		debug_printf("\tv_modecnt=0x%x, \tv_modesp=0x%x,\n",
			vp->v_modecnt, vp->v_modesp);
		for (i = 0; i < MKDIOADDR; i += 2) {
			debug_printf("\tv_ioaddrs[%d]=0x%x, \tv_ioaddrs[%d]=0x%x,\n",
				i, vp->v_ioaddrs[i], (i+1), vp->v_ioaddrs[i+1]);
		}
			
		break;
	}

	case 4: {
		termstate_t *tsp;

		/*
		 * termstate structure.
		 */

		tsp = (termstate_t *) structp;
		debug_printf("\n termstate struct: size=0x%x(%d)\n",
			sizeof(termstate_t), sizeof(termstate_t));
		debug_printf("\tt_flags=0x%x, \tt_font=0x%x,\n",
			tsp->t_flags, tsp->t_font);
		debug_printf("\tt_curattr=0x%x, \tt_normattr=0x%x,\n",
			tsp->t_curattr, tsp->t_normattr);
		debug_printf("\tt_row=0x%x, \tt_col=0x%x,\n",
			tsp->t_row, tsp->t_col);
		break;
	}

	case 5: {
		kbstate_t *kbp;

		/*
		 * kbstate structure.
		 */

		kbp = (kbstate_t *) structp;
		debug_printf("\n kbstate struct: size=0x%x(%d)\n",
			sizeof(kbstate_t), sizeof(kbstate_t));
		debug_printf("\tkb_sysrq=0x%x, \tkb_srqscan=0x%x,\n",
			kbp->kb_sysrq, kbp->kb_srqscan);
		debug_printf("\tkb_prevscan=0x%x,\n",
			kbp->kb_prevscan);
		debug_printf("\tkb_state=0x%x, \tkb_sstate=0x%x,\n",
			kbp->kb_state, kbp->kb_sstate);
		debug_printf("\tkb_togls=0x%x, \tkb_extkey=0x%x,\n",
			kbp->kb_togls, kbp->kb_extkey);
		debug_printf("\tkb_altseq=0x%x\n",
			kbp->kb_altseq);
		break;
	}

	case 6: {
		struct modeinfo *modep;

		/*
		 * modeinfo structure
		 */

		modep = (struct modeinfo *) structp;
		debug_printf("\n modeinfo struct: size=0x%x(%d)\n",
			sizeof(struct modeinfo), sizeof(struct modeinfo));
		debug_printf("\tm_cols=0x%x, \tm_rows=0x%x,\n",
			modep->m_cols, modep->m_rows);
		debug_printf("\tm_xpels=0x%x, \tm_ypels=0x%x,\n",
			modep->m_xpels, modep->m_ypels);
		debug_printf("\tm_color=0x%x, \tm_font=0x%x,\n",
			modep->m_color, modep->m_font);
		debug_printf("\tm_base=0x%x(%d), \tm_size=0x%x(%d),\n",
			modep->m_base, modep->m_base, modep->m_size, modep->m_size);
		debug_printf("\tm_params=0x%x, \tm_offset=0x%x,\n",
			modep->m_params, modep->m_offset);
		debug_printf("\tm_ramdac=0x%x, \tm_vaddr=0x%x(%d),\n",
			modep->m_ramdac, modep->m_vaddr, modep->m_vaddr);
		break;
	}

	case 7: {
		struct xqInfo *xqp; 

		/*
		 * xqInfo structure.
		 */

		xqp = (struct xqInfo *) structp;
		debug_printf("\n xqInfo struct: size=0x%x(%d)\n",
			sizeof(struct xqInfo), sizeof(struct xqInfo));
		debug_printf("\txq_queue=0x%x, \txq_private=0x%x,\n",
			xqp->xq_queue, xqp->xq_private);
		debug_printf("\txq_qtype=0x%x, \txq_buttons=0x%x,\n",
			xqp->xq_qtype, xqp->xq_buttons);
		debug_printf("\txq_devices=0x%x, \txq_xlate=0x%x,\n",
			xqp->xq_devices, xqp->xq_xlate);
		debug_printf("\txq_addevent=0x%x, \txq_ptail=0x%x,\n",
			xqp->xq_addevent, xqp->xq_ptail);
		debug_printf("\txq_psize=0x%x, \txq_signo=0x%x,\n",
			xqp->xq_psize, xqp->xq_signo);
		debug_printf("\txq_proc=0x%x, \txq_next=0x%x,\n",
			xqp->xq_proc, xqp->xq_next);
		debug_printf("\txq_prev=0x%x, \txq_uaddr=0x%x,\n",
			xqp->xq_prev, xqp->xq_uaddr);
		debug_printf("\txq_npages=0x%x\n", xqp->xq_npages);
		break;
	}

	default:
		debug_printf("\nUsage (from kdb):\n");
		debug_printf("\t<struct ptr> <type (1-7)> ws_dump 2 call\n");
		break;

	} /* end switch */
}

#endif /* DEBUG || DEBUG_TOOLS */
