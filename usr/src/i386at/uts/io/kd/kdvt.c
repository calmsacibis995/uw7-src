#ident	"@(#)kdvt.c	1.25"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	27Jan97		rodneyh@sco.com
 *	- Changes for multiconsole support. make sure we have acquired the
 *	  console text memory before accessing it.
 *	L001	19Jul97		rodneyh@sco.com
 *	- Change kdvt_open not to auto activate the new VT if kd_no_activate
 *	  is true. Fix for ul97-17852 and ul97-18217.
 *
 */

#include <io/ansi/at_ansi.h>
#include <io/gvid/vid.h>
#include <io/gvid/vdc.h>
#include <io/kd/kb.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/strtty.h>
#include <io/stropts.h>
#include <io/termios.h>
#include <io/ws/chan.h>
#ifndef NO_MULTI_BYTE
#include <io/ws/mb.h>
#endif /* NO_MULTI_BYTE */
#include <io/ws/tcl.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>


void		kdvt_switch(ushort, pl_t);
int		kdvt_rel_refuse_l(void);

void		(*kd_vdc800_vgamode)(void);

extern int	kdsetcursor(ws_channel_t *, termstate_t *);
extern int	kdsetbase(ws_channel_t *, termstate_t *);
extern int	kdclrscr(ws_channel_t *, ushort, int);

extern void	kdv_setuline(vidstate_t *, int);
extern void	kdv_mvuline(vidstate_t *, int);
extern void	kdv_enable(vidstate_t *);
extern void     kdv_rst(termstate_t *, vidstate_t *);
extern void	kdv_setdisp(ws_channel_t *, vidstate_t *, termstate_t *, unchar);
extern void	kdv_scrxfer(ws_channel_t *, int);
extern void	kdv_textmode(ws_channel_t *);
#ifdef EVGA
extern void	evga_ext_rest(unchar);
extern void	evga_ext_init(unchar);
#endif /* EVGA */


extern struct vdc_info	Vdc;
extern wstation_t	Kdws;
extern ws_channel_t	Kd0chan;
extern struct ext_graph kd_vdc800;		/* VDC800 hook */

extern boolean_t kd_con_acquired;		/* L000 */

#ifdef EVGA
extern int evga_inited;
extern int cur_mode_is_evga;
extern unchar saved_misc_out;
extern struct at_disp_info disp_info[];
#endif	/* EVGA */


/*
 * int
 * kdvt_open(ws_channel_t *, pid_t)
 *
 * Calling/Exit State:
 *	- No locks are held on entry or exit.
 *	- Called when WS_CHANOPEN portocol message is received
 *	  from CHANMUX.
 *
 * Description:
 *	If kernel memory is not allocated for this channel,
 *	then allocate memory for the screen buffer and insert
 *	the channel (chp) in front of the channel list. This 
 *	channel is activated if its not an active channel.
 */
int
kdvt_open(ws_channel_t *chp, pid_t ppid)
{
	extern boolean_t kd_no_activate;		/* L001 */
	ws_channel_t	*achp;
	int		indx;
	ushort		*scrp;
	pl_t		opl, oldpri;


	opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

	/*
	 * Get a pointer to the active channel.
	 */
	achp = WS_ACTIVECHAN(&Kdws);

	indx = chp->ch_id;

	if (!Kdws.w_scrbufpp[indx]) {
		if (!(scrp = (ushort *) kmem_alloc(sizeof(ushort) * 
					KD_MAXSCRSIZE, KM_NOSLEEP))) {
			/*
			 *+ There isn't enough memory available to allocate 
			 *+ for the screen buffer.
			 */
			cmn_err(CE_WARN, 
				"kdvt_open: out of memory for new screen "
				"for virtual terminal 0x%x", indx);
			/*
			 * Switch to the first virtual terminal (channel 0).
			 * Do not have to release the w_rwlock here, since
			 * ws_switch() returns without the lock held.
			 */
			kdvt_switch(K_VTF, opl); 
			return (ENOMEM);
		}

		Kdws.w_scrbufpp[indx] = scrp;
		if (chp != achp)
			chp->ch_vstate.v_scrp = scrp;

		kdclrscr(chp, chp->ch_tstate.t_origin, chp->ch_tstate.t_scrsz);
	} 

	oldpri = splhi(); 
	chp->ch_nextp = achp->ch_nextp;
	achp->ch_nextp = chp;
	chp->ch_prevp = chp->ch_nextp->ch_prevp;
	chp->ch_nextp->ch_prevp = chp;
	splx(oldpri);

	/*
	 * Do not activate if it's a login process unless it's the
	 * only open channel. 
	 */
	if (chp != achp && ppid != 1 && !kd_no_activate &&	/* L001 */
				!ws_activate(&Kdws, chp, VT_NOFORCE))
		kdvt_rel_refuse_l();

	/* increment the channel open count */
	chp->ch_opencnt++;

	RW_UNLOCK(Kdws.w_rwlock, opl);

	return (0);
}


/*
 * int
 * kdvt_close(ws_channel_t *)
 *
 * Calling/Exit State:
 *	- The w_rwlock is held in exclusive mode.
 *
 * Description:
 *	Free the channel (chp) screen buffer, provided
 *	its not channel zero. Channel zero must
 *	maintain its screen buffer to kdb/cmn_err() messages.
 */
int
kdvt_close(ws_channel_t *chp)
{
	int	id;


	id = chp->ch_id;
	if (id != 0 && Kdws.w_scrbufpp[id]) {
		chp->ch_vstate.v_scrp = (ushort *)NULL;
		kmem_free(Kdws.w_scrbufpp[id], sizeof(ushort) * KD_MAXSCRSIZE);
		Kdws.w_scrbufpp[id] = (ushort *) NULL;
	}

	return (0);
}


/*
 * STATIC int
 * kdvt_isnormal(ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is already held in exclusive mode. 
 */
int
kdvt_isnormal(ws_channel_t *chp)
{
	ws_mapavail(chp, &Kdws.w_map);

	if (chp->ch_dmode == KD_GRAPHICS || 
			CHNFLAG(chp, CHN_MAPPED) || CHNFLAG(chp, CHN_PROC))
		return (0);
	else
		return (1);
}


/*
 * void
 * kd_vdc800_vgapass(void (*vgapass_p)())
 *	VDC800 hooks 
 *
 * Calling/Exit State:
 *	None.
 */
void
kd_vdc800_vgapass(void (*vgapass_p)())
{
	kd_vdc800_vgamode = vgapass_p;
}


/*
 * void 
 * kdvt_switch(ushort, pl_t)
 *
 * Calling/Exit State:
 *	On entry, the w_rwlock lock is held in exclusive mode,
 *	but is released before returning from the function. 
 *	The priority level is set to pl.
 *
 * Description:
 *	Switch to a new channel. The type of switch is
 *	determined by ch. If ch is equal to 
 *		- NEXT/PREV - 
 *			switch to the channel pointed
 *			by ch_nextp/ch_prevp.
 *		- FRCNEXT/FRCPREV - 
 *			normal next/prev VT switch if the active
 *		        VT is normal. The normal represents
 *			indicates that the VT is not process 
 *			mode and the video is not in graphics
 *			mode. Otherwise, ws_force() is called
 *			to take violent action to ensure that
 *			a channel switch will occur.
 *				    
 * Note:
 *	Since this function is called from an interrupt
 *	level and the base level, the priority level at
 *	which the w_rwlock is released is passed as an
 *	extra argument.
 */
void
kdvt_switch(ushort ch, pl_t pl)
{
	ws_channel_t	*newchp, *chp, *vtmchp;
	extern void	kd_vdc800_release();
	int		chan = -1;


	/*
	 * Get a pointer to the active channel.
	 */
	chp = WS_ACTIVECHAN(&Kdws);

	/*
	 * check if VDC800 is open and active vt is not in proc mode 
	 */
	if (kd_vdc800.procp && (chp->ch_flags & CHN_PROC) == 0) {
		if (proc_valid(kd_vdc800.procp)) {
			kdvt_rel_refuse_l();
			RW_UNLOCK(Kdws.w_rwlock, pl);
			return;
		} else {			/* process died */
			if (kd_vdc800_vgamode)
				(*kd_vdc800_vgamode)();
			kd_vdc800_release();
		}
	}

	switch (ch) {
	case K_NEXT:
		chan = chp->ch_nextp->ch_id;
		break;

	case K_PREV:
		chan = chp->ch_prevp->ch_id;
		break;

	case K_FRCNEXT:
		if (kdvt_isnormal(chp)) {
			chan = chp->ch_nextp->ch_id;
			break;
		}
		/*
		 * Do not have to release the w_rwlock here, since
		 * ws_force() returns without the lock held.
		 */
		ws_force(&Kdws, chp, pl);
		return;

	case K_FRCPREV:
		if (kdvt_isnormal(chp)) {
			chan = chp->ch_prevp->ch_id;
			break;
		}
		/*
		 * Do not have to release the w_rwlock here, since
		 * ws_force() returns without the lock held.
		 */
		ws_force(&Kdws, chp, pl);
		return;

	default:
		if (ch >= K_VTF && ch <= K_VTL)
			chan = ch - K_VTF;
		break;
	}

	if (chan != -1 && (newchp = ws_getchan(&Kdws, chan))) {
		if (newchp->ch_opencnt || (chan == 0)) {
			if (ws_activate(&Kdws, newchp, VT_NOFORCE)) {
				RW_UNLOCK(Kdws.w_rwlock, pl);
				return;
			}
		} else if ((vtmchp = ws_getchan(&Kdws, WS_MAXCHAN))) {
			RW_UNLOCK(Kdws.w_rwlock, pl);
			ws_notifyvtmon(vtmchp, ch);
			return;
		}
	}

	kdvt_rel_refuse_l();

	RW_UNLOCK(Kdws.w_rwlock, pl);

	return;
}


/*
 * int
 * kdvt_activate(ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- force indicates if switch is foreced.
 *	- w_rwlock is already held in exclusive mode.
 *	- Return 1 to indicate a successful switch.
 *
 * Description:
 *	This routine performs the actual work of switching
 *	to a new VT. It is called by ws_activate(), which
 *	itself is called when a channel is closed, opened
 *	or the user-typed a hotkey sequence.
 */
int
kdvt_activate(ws_channel_t *chp, int force)
{
	ws_channel_t	*achp;
	vidstate_t	*vp;
	termstate_t	*tsp;
	kbstate_t	*kbp;
	unchar		tmp;
	toid_t		tid;
	ushort		ostate;
	boolean_t	got_con = B_FALSE;		/* L000 */

	extern void	switch_kb_mode(int);

	/*
	 * Do not need to check the WS_NOMODESW flag because
	 * firstly, while switching video mode, the w_rwlock is
	 * not dropped to allow other operations and secondly, 
	 * w_rwlock when held could not possibly allow operations
	 * which would require the w_rwlock (reader/writer
	 * lock) to be held in exclusive mode. 
	 */
	if (Kdws.w_flags & WS_NOCHANSW)
		return (0);

	if (chp->ch_vstate.v_scrp == NULL || 
            Kdws.w_scrbufpp[chp->ch_id] == NULL) {
		/* This channel has just been closed */
		return(0);
	}
	/*
	 * Get a pointer to the active channel.
	 */
	achp = WS_ACTIVECHAN(&Kdws);

	ws_mapavail(achp, &Kdws.w_map);

	if ((achp->ch_dmode == KD_GRAPHICS || CHNFLAG(achp, CHN_MAPPED)) &&
	    !CHNFLAG(achp, CHN_PROC)) {
		return (0);
	}

	ASSERT(getpl() == plstr);

	if(!kd_con_acquired)				/* L000 begin */
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L000 end */


	/*
	 * Block calls to kdv_setmode() and new channel activations. 
	 *
	 * In an MP system another channel switch could begin when the
	 * <w_rwlock> is dropped to send any outstanding data for the
	 * "current active channel" or "new to be active channel". Thus,
	 * its necessary to block any process initiated channel activations.
	 * Interrupt/user initiated channel activations will return unsucc-
	 * essfully, but process generated channel activations will wait.
	 */
	Kdws.w_flags |= (WS_NOMODESW|WS_NOCHANSW|WS_NOMAPDISP); 

	/*
	 * save state of current channel 
	 */
	vp = &achp->ch_vstate;
	tsp = &achp->ch_tstate;

	kdkb_sound(0);

	if (achp->ch_dmode != KD_GRAPHICS) {	/* save text mode state */
		if (!(force & VT_NOSAVE))
#ifndef NO_MULTI_BYTE
			if (achp->ch_dmode == KD_GRTEXT)
				Kdws.w_consops->cn_gdv_scrxfer(achp,
								KD_SCRTOBUF);
			else
#endif /* NO_MULTI_BYTE */
			kdv_scrxfer(achp, KD_SCRTOBUF);
		tsp->t_cursor -= tsp->t_origin;
		tsp->t_origin = 0;
	} else 
		vp->v_font = 0;
	
	vp->v_scrp = Kdws.w_scrbufpp[achp->ch_id];

	if (force & VT_NOSAVE) {
#ifndef NO_MULTI_BYTE
		/*
		 * If we are switching from graphics text mode to
		 * text mode, deallocate resources associated with
		 * graphics text mode.
		 */
		if (achp->ch_dmode == KD_GRTEXT)
			Kdws.w_consops->cn_gs_free(Kdws.w_consops, achp, tsp);
#endif /* NO_MULTI_BYTE */
		vp->v_cvmode = vp->v_dvmode; 
		achp->ch_dmode = KD_TEXT0;
		tsp->t_cols = WSCMODE(vp)->m_cols;
		tsp->t_rows = WSCMODE(vp)->m_rows;
		tsp->t_scrsz = tsp->t_rows * tsp->t_cols;
	} else
		vp->v_dvmode = vp->v_cvmode; 

	/*
	 * Clear any pending timeout.
	 */
	while (Kdws.w_timeid) {
		tid = Kdws.w_timeid;
		Kdws.w_timeid = 0;
		RW_UNLOCK(Kdws.w_rwlock, plstr);
		untimeout(tid);
		(void) RW_WRLOCK(Kdws.w_rwlock, plstr);
	}

	kbp = &achp->ch_kbstate;

	/*
	 * Transfer the shift key states to the new
	 * channel kbstate. Note that the toggle key
	 * state is maintained on a per channel basis
	 * and is not transferred here. 
	 */

	ostate = chp->ch_kbstate.kb_state;

	ws_xferkbstat(kbp, &chp->ch_kbstate);

	Kdws.w_active = chp->ch_id;
	Kdws.w_qp = chp->ch_qp;
	vp = &chp->ch_vstate;
	tsp = &chp->ch_tstate;
	kbp = &chp->ch_kbstate;

	ASSERT(Kdws.w_flags & WS_NOMODESW);

	/*
	 * Reset the state of CHAR on the currently-active VT for 
	 * shift keys by calling ws_stnontog().
	 */
	RW_UNLOCK(Kdws.w_rwlock, plstr);
	ws_stnontog(Kdws.w_qp, &Kdws.w_mp, ostate, kbp->kb_state, plstr);  
	ws_kbtime(&Kdws);
	(void) RW_WRLOCK(Kdws.w_rwlock, plstr);

#ifdef EVGA
	evga_ext_rest((unchar)cur_mode_is_evga);
#endif	/* EVGA */

	/*
	 * If the new channel is in a graphics or graphics-text
	 * mode, call kdv_rst() to program the video controller,
	 * otherwise, call kdv_setdisp() to program it correctly
	 * the new VTs text mode.
	 */
#ifndef NO_MULTI_BYTE
	if (chp->ch_dmode == KD_GRTEXT) {
		kdv_rst(tsp, vp);
	} else
#endif /* NO_MULTI_BYTE */
	if (chp->ch_dmode == KD_GRAPHICS) {
		kdv_rst(tsp, vp);
	} else {
		ASSERT(chp->ch_dmode == KD_TEXT || chp->ch_dmode == KD_TEXT1);
		kdv_setdisp(chp, vp, tsp, vp->v_cvmode);
	}

#ifdef EVGA
	evga_ext_init(vp->v_cvmode);
#endif	/* EVGA */

#ifndef NO_MULTI_BYTE
	if (chp->ch_dmode == KD_GRTEXT) {
		kdsetbase(chp, tsp);
		Kdws.w_consops->cn_gdv_scrxfer(chp, KD_BUFTOSCR);
	} else
#endif /* NO_MULTI_BYTE */
	if (chp->ch_dmode != KD_GRAPHICS) {
		if (VTYPE(V400) || DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) {
			if (vp->v_undattr == UNDERLINE) {
				kdv_setuline(vp, 1);
				kdv_mvuline(vp, 1);
			} else {
				kdv_setuline(vp, 0);
				kdv_mvuline(vp, 0);
			}
		}

		/*
		 * Copy the saved screen buffer of the VT to the
		 * video memory and position the cursor.
		 */
		kdsetbase(chp, tsp);
		kdv_scrxfer(chp, KD_BUFTOSCR);
		kdsetcursor(chp, tsp);

		/*
		 * If on a VGA controller, program the controller
		 * to interpret the blink/bright background attribute
		 * as set in the termstate_t structure.
		 */
		if (DTYPE(Kdws, KD_VGA)) {

			(void) inb(vp->v_regaddr + IN_STAT_1);
			outb(0x3c0, 0x10);	/* attribute mode control reg */
			tmp = inb(0x3c1);
			if (tsp->t_flags & T_BACKBRITE)
				outb(0x3c0, (tmp & ~0x08));
			else
				outb(0x3c0, (tmp | 0x08));
			outb(0x3c0, 0x20);	/* turn palette on */
		}
	} 

	if ((chp->ch_dmode != KD_GRAPHICS) || !(WSCMODE(vp)->m_font))
		kdv_enable(vp);

	RW_UNLOCK(Kdws.w_rwlock, plstr);

	/*
	 * Clear any pending timeout.
	 */
	(void) LOCK(Kdws.w_mutex, plstr);
	while (Kdws.w_timeid) {
		tid = Kdws.w_timeid;
		Kdws.w_timeid = 0;
		UNLOCK(Kdws.w_mutex, plstr);
		untimeout(tid);
		(void) LOCK(Kdws.w_mutex, plstr);
	}
	UNLOCK(Kdws.w_mutex, plstr);

	switch_kb_mode(chp->ch_charmap_p->cr_kbmode);
	ws_kbtime(&Kdws);

	/* Reacquire the w_rwlock before exiting. */
	(void) RW_WRLOCK(Kdws.w_rwlock, plstr);

	/*
	 * Signal any process waiting to change the video mode
	 * or to switch channel.
	 */
	Kdws.w_flags &= ~(WS_NOMODESW|WS_NOCHANSW|WS_NOMAPDISP);
	SV_SIGNAL(Kdws.w_flagsv, 0);

	if(got_con){					/* L000 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L000 end */

	return (1);
}


/*
 * int
 * kdvt_ioctl(ws_channel_t *, int, int, int *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- Return 0 on success, otherwise return an error number. 
 *
 * Description:
 *	This routine actually handles the VT-related ioctls that
 *	come from the KDVM driver, which calls this routine directly.
 */
/* ARGSUSED */
int
kdvt_ioctl(ws_channel_t *chp, int cmd, int arg, int *rvalp)
{
	ws_channel_t	*newchp;
	int		rv = 0;
	int		cnt;
	struct vt_mode	vtmode;
	struct vt_stat	vtinfo;
	pl_t		opl;


	switch (cmd) {
	case VT_GETMODE:
		/*
		 * Fill in vt_mode structure and copy it up to
		 * the user. The most significant data is whether
		 * the VT is in process mode or not. 
		 */

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
		vtmode.mode = ws_procmode(&Kdws, chp) ? VT_PROCESS : VT_AUTO;
		vtmode.waitv = CHNFLAG(chp, CHN_WAIT) ? 1 : 0;
		vtmode.relsig = chp->ch_relsig;
		vtmode.acqsig = chp->ch_acqsig;
		vtmode.frsig = chp->ch_frsig;
		/*
		 * Locks held during interrupts cannot be
		 * held during copyin/copyout
		 */
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);

		if (copyout(&vtmode, (caddr_t)arg, sizeof(vtmode)) < 0)
			rv = EFAULT;
		break;

	case VT_SETMODE:
		/*
		 * Copy the user supplied vt_mode structure and
		 * update the VT state based on the information
		 * in the structure.
		 */

		if (copyin((caddr_t)arg, &vtmode, sizeof(vtmode)) < 0) {
			rv = EFAULT;
			break;
		}

		/*
		 * <w_rwlock> is acquired in exclusive mode, because it
		 * might reset the VT from process mode to auto mode 
		 * which might necessitate unmapping of video buffer
		 * from the user address space. Since unmapping user
		 * address space requires to hold the workstation
		 * reader/writer lock in exclusive mode, the w_rwlock
		 * is acquired here in exclusive mode.
		 */
		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

		/*
		 * If the mode of the VT needs to be set to process
		 * mode, check first if the VT is already in 
		 * process mode. If not, make the owner of
		 * the VT be the current process.
		 */
		if (vtmode.mode == VT_PROCESS) {
			if (!ws_procmode(&Kdws, chp)) {
				chp->ch_procp = proc_ref();
				chp->ch_flags |= CHN_PROC;
			}
		} else if (vtmode.mode == VT_AUTO) {
			ASSERT(getpl() == plstr);
			/*
			 * The mode of the VT needs to be switched out
			 * of the process mode. ws_automode() resets
			 * the VT to normal (automatic) mode.
			 */
			if (CHNFLAG(chp, CHN_PROC)) {
				ws_automode(&Kdws, chp);
				chp->ch_procp = (void *)0;
				/* 
				 * proc_unref() is already done
				 * in ws_automode(), so its not
				 * required to do here also
				 */
				chp->ch_flags &= ~CHN_PROC;
			}
		} else {
			RW_UNLOCK(Kdws.w_rwlock, opl);
			rv = EINVAL;
			break;
		}

		ASSERT(rv == 0);

		if (vtmode.waitv)
			chp->ch_flags |= CHN_WAIT;
		else
			chp->ch_flags &= ~CHN_WAIT;

		if (vtmode.relsig) {
			if (vtmode.relsig < 0 || vtmode.relsig >= NSIG) {
				RW_UNLOCK(Kdws.w_rwlock, opl);
				rv = EINVAL;
				break;
			} else {
				chp->ch_relsig = vtmode.relsig;
			}
		}

		if (vtmode.acqsig) {
			if (vtmode.acqsig < 0 || vtmode.acqsig >= NSIG) {
				RW_UNLOCK(Kdws.w_rwlock, opl);
				rv = EINVAL;
				break;
			} else {
				chp->ch_acqsig = vtmode.acqsig;
			}
		}

		if (vtmode.frsig) {
			if (vtmode.frsig < 0 || vtmode.frsig >= NSIG) {
				RW_UNLOCK(Kdws.w_rwlock, opl);
				rv = EINVAL;
				break;
			} else {
				chp->ch_frsig = vtmode.frsig;
			}
		}

		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	case VT_RELDISP:
		/*
		 * This ioctl is done by the owner of process mode
		 * VT in response to a signal from the kernel that
		 * a VT switch out of the VT has been requested by 
		 * the user. This ioctl is also done by the process
		 * owning the process mode VT in response to a signal
		 * from the kernel that the VT is being switched into.
		 */

		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

		/*
		 * If the ioctl argument is VT_ACKACQ (the process that
		 * owns the VT is acknowledging that the VT can be
		 * made active), then cancel the timeout that was
		 * set in the case no response came from the process
		 * At this point a successful switch into a process
		 * mode VT has occurred.
		 */
		if ((int) arg == VT_ACKACQ) {
			Kdws.w_noacquire = 0;
			if (CHNFLAG(chp, CHN_ACTV)) {
				RW_UNLOCK(Kdws.w_rwlock, opl);
				untimeout(chp->ch_timeid);
				opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
				chp->ch_timeid = 0;
			}
			RW_UNLOCK(Kdws.w_rwlock, opl);
			break;
		}

		/*
		 * Otherwise, the ioctl is being sent by the process
		 * that owns the VT the user wishes to switch out of.
		 * If the argument to the ioctl is 0, the process
		 * indicating that it does not want to give up the
		 * screen, but wants the VT to remain active. This
		 * fails the VT switch (the user gets beeped), but
		 * the ioctl returns as 0 (success). Otherwise,
		 * ws_switch() is called. If it returns non-zero,
		 * the switch succeeded, and the ioctl returns 0
		 * for success. Otherwise, the VT switch failed, and
		 * the ioctl returns EBUSY. This indicates to the
		 * calling process that even though it tried to
		 * release the display, something went wrong and
		 * the VT is still active.
		 */

		if (!Kdws.w_noacquire) {
			if (chp != ws_activechan(&Kdws)) {
				RW_UNLOCK(Kdws.w_rwlock, opl);
				rv = EACCES;
				break;
			}
			if (!Kdws.w_switchto) {
				RW_UNLOCK(Kdws.w_rwlock, opl);
				rv = EINVAL;
				break;
			}
		}

                if (kd_vdc800.procp != (void *)0) {	/* VDC800 hook */
			kdvt_rel_refuse_l();
			RW_UNLOCK(Kdws.w_rwlock, opl);
			rv = EACCES;
			break;
		}

		if (!Kdws.w_switchto) {
			RW_UNLOCK(Kdws.w_rwlock, opl);
			rv = EINVAL;
			break;
		}

		RW_UNLOCK(Kdws.w_rwlock, plstr);

		/*
		 * Wait while the modesw/chansw/mapdisp is in progress.
		 */
		(void) LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_flags & (WS_NOCHANSW|WS_NOMODESW|WS_NOMAPDISP)) {
			/* In SVR4 the sleep priority was TTOPRI */
			SV_WAIT(Kdws.w_flagsv, (primed + 3), Kdws.w_mutex);
			(void) LOCK(Kdws.w_mutex, plstr);
		}
		UNLOCK(Kdws.w_mutex, plstr);

		(void) RW_WRLOCK(Kdws.w_rwlock, plstr);

		ASSERT(getpl() == plstr);
                if (arg && ws_switch(&Kdws, Kdws.w_switchto, VT_NOFORCE)) {
			RW_UNLOCK(Kdws.w_rwlock, opl);
                        break;
                }

		kdvt_rel_refuse_l();
		RW_UNLOCK(Kdws.w_rwlock, opl);

		if (arg) 
			rv = EBUSY;
		break;

	case VT_ACTIVATE:
		/*
		 * The arg is the channel no. of the VT to 
		 * be made active. If the VT is not set up
		 * by wsinit or is not in use (ch_opencnt
		 * is equal to zero), then return ENXIO,
		 * otherwise the specified channel (arg) 
		 * is made active by calling ws_activate.
		 */

		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

		if (!(newchp = ws_getchan(&Kdws, arg))) {
			RW_UNLOCK(Kdws.w_rwlock, opl);
			rv = ENXIO;
			break;
		}

		if (!newchp->ch_opencnt) {
			RW_UNLOCK(Kdws.w_rwlock, opl);
			rv = ENXIO;
			break;
		}

		RW_UNLOCK(Kdws.w_rwlock, plstr);

		/*
		 * wait while the modesw/chansw/mapdisp is in progress
		 */
		(void) LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_flags & (WS_NOCHANSW|WS_NOMODESW|WS_NOMAPDISP)) {
			/* In SVR4 the sleep priority was TTOPRI */
			SV_WAIT(Kdws.w_flagsv, (primed+3), Kdws.w_mutex);
			(void) LOCK(Kdws.w_mutex, plstr);
		}
		UNLOCK(Kdws.w_mutex, plstr);

		(void) RW_WRLOCK(Kdws.w_rwlock, plstr);
		ws_activate(&Kdws, newchp, VT_NOFORCE);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	case VT_WAITACTIVE:
		/*
		 * If VT is active or is in process mode, then
		 * return immediately, otherwise a flag is set
		 * in ws_channel_t structure and we wait until the
		 * the call to ws_activate() makes this VT active.
		 */

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);

		if (ws_procmode(&Kdws, chp) || WS_ISACTIVECHAN(&Kdws, chp)) {
			UNLOCK(chp->ch_mutex, plstr);
			RW_UNLOCK(Kdws.w_rwlock, opl);
			break;
		}

		/*
		 * Release the lock at plstr, instead of opl to prevent
		 * lowering of priority level of the channel mutex lock 
		 * below its minpl.
		 */
		RW_UNLOCK(Kdws.w_rwlock, plstr);

		ASSERT(getpl() == plstr);

		chp->ch_flags |= CHN_WACT;
		while (chp->ch_flags & CHN_WACT) {
			/* In SVR4 the sleep priority was TTOPRI */
			SV_WAIT(chp->ch_wactsv, (primed+3), chp->ch_mutex);
			(void) LOCK(chp->ch_mutex, plstr);
		}

		UNLOCK(chp->ch_mutex, opl);
		break;

	case VT_GETSTATE:
		/* 
		 * Returns the active VT no. and list of open VTs.
		 */

		/*
		 * <w_rwlock> is acquired in exclusive mode to prevent the
		 * overhead of acquiring and releasing the channel locks
		 * for each configured channel. However, the overhead may
		 * be insignificant if the number of configured channels 
		 * is relatively small and thus would be less expensive to 
		 * hold the channel lock and the w_rwlock in shared mode.
		 * Presently, its unclear what the right approach is, so
		 * for simplicity the w_rwlock is acquired in exclusive
		 * mode and if need be take the other approach.
		 */
		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

		if ((newchp = ws_activechan(&Kdws)))
			vtinfo.v_active = newchp->ch_id;
		vtinfo.v_state = 0;
		newchp = chp;
		do {
			vtinfo.v_state |= (1 << newchp->ch_id);
			newchp = newchp->ch_nextp;
		} while (newchp != chp);

		RW_UNLOCK(Kdws.w_rwlock, opl);

		if (copyout(&vtinfo, (caddr_t) arg, sizeof(vtinfo)) < 0)
			rv = EFAULT;
		break;
	
	case VT_SENDSIG:
		/*
		 * Sends specified signal to open VTs owned by
		 * the process. The vt_stat structure is copied
		 * from the user. It contains the bit mask of the
		 * VTs that the signal is sent to.
		 */

		if (copyin((caddr_t) arg, &vtinfo, sizeof(vtinfo)) < 0) {
			rv = EFAULT;
			break;
		}

		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

		for (cnt = 0; cnt < WS_MAXCHAN; cnt++) { 
			if (!(vtinfo.v_state & (1 << cnt)))
				continue;
			if (!(newchp = ws_getchan(&Kdws, cnt)))
				continue;
			if (!newchp->ch_opencnt)
				continue;
			/*
			 * signal process group of channel 
			 */
			if (!newchp->ch_qp) {
				/*
				 *+ The channel does not have a corressponding
				 *+ stream to send a signal message.
				 */
				cmn_err(CE_NOTE,
					"kdvt_ioctl: found no queue pointer for integral VT %d", cnt);
				continue;
			}

			RW_UNLOCK(Kdws.w_rwlock, opl);
			putnextctl1(newchp->ch_qp, M_SIG, vtinfo.v_signal);
			opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
		}

		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	default:
		rv = ENXIO;
		break;
	}

	/*
	 *+ VT ioctl failure.
	 */
	if (rv) {
		/*
		 *+ VT ioctl failure.
		 */
		cmn_err(CE_NOTE, 
			"!kdvt_ioctl 0x%x failed with rv 0x%x", cmd, rv);
	}

	return (rv);
}


/*
 * int
 * kdvt_rel_refuse_l(void)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode 
 *
 * Description:
 *	Called from VT when process refuses to switch.
 */
int
kdvt_rel_refuse_l(void)
{
	pl_t	oldpri;


	oldpri = splhi(); 
	if (Kdws.w_switchto)
		Kdws.w_switchto = (ws_channel_t *) NULL;
	kdkb_tone();
	splx(oldpri); 
	return (0);
}


/*
 * int
 * kdvt_rel_refuse(void)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- Workstation reader/writer lock is acquired in
 *	  in exclusive mode. This function is called
 *	  after timeout has expired.
 *
 * Description:
 *	Called from VT when process refuses to switch.
 */
int
kdvt_rel_refuse(void)
{
	pl_t	pl;
	

	pl = RW_WRLOCK(Kdws.w_rwlock, plstr);
	kdvt_rel_refuse_l();
	RW_UNLOCK(Kdws.w_rwlock, pl);
	return (0);
}


/*
 * int
 * kdvt_acq_refuse_l(ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Description:
 *	The timeout set for a process mode VT to obtain the VT
 *	has expired. We leave user in the ''limbo'' state of 
 *	having the process mode VT be the active VT. This way,
 *	if it does respond with a VT_RELDISP/VT_ACQACK ioctl,
 *	the process will indeed own the VT. If the process is 
 *	still alive, this should happen. If the process is
 *	dead, the next attemp to switch will detect that its 
 *	dead and the user will be allowed to switch out. 
 *	Otherwise, there is VT-FORCE.
 */
int
kdvt_acq_refuse_l(ws_channel_t *chp)
{
	if (!Kdws.w_noacquire)
		return (0);

	Kdws.w_switchto = (ws_channel_t *) NULL;
	chp->ch_timeid = 0;
	Kdws.w_noacquire = 0;
	kdkb_tone();
	return (0);
}


/*
 * int
 * kdvt_acq_refuse(ws_channel_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- Workstation reader/writer lock is acquired
 *	  in exclusive mode. This function is called
 *	  after timeout has expired.
 */
int
kdvt_acq_refuse(ws_channel_t *chp)
{
        pl_t	pl;


        pl = RW_WRLOCK(Kdws.w_rwlock, plstr);
	kdvt_acq_refuse_l(chp);
        RW_UNLOCK(Kdws.w_rwlock, pl);
	return (0);
}


/*
 * ws_channel_t *
 * kdvt_switch2chan0(void)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- It is called from the console suspend routine which is
 *	  called by the kernel debugger.
 *
 * Remarks:
 *	Switch to the console channel and set the display to text mode.
 *
 *	The function is similar to kdvt_activate(), however, it
 *	does not save any state of the active channel and does
 *	not coordinate with set video mode operation (kdv_setmode)
 *
 *	No locks must be acquired or released that may lower the
 *	priority. Similarly no timeouts, untimeouts or memory
 *	allocations must be done because they may acquire locks
 *	at a lower priority.
 */
ws_channel_t *
kdvt_switch2chan0(void)
{
	ws_channel_t	*ch0p, *achp = NULL;
	vidstate_t	*vp;
	termstate_t	*tsp;
	kbstate_t	*kbp;
	unchar		tmp;
	boolean_t got_con = B_FALSE;		/* L000 */


	if (WS_ISNOTINITED(&Kdws))
		return(achp);

	if ((ch0p = ws_getchan(&Kdws, 0)) == NULL)
		return(achp);

	if (WS_ISACTIVECHAN(&Kdws, ch0p)) {
		if( ch0p->ch_dmode != KD_GRAPHICS)
			return(achp);
	} else {
		achp = WS_ACTIVECHAN(&Kdws);
	}

	if(!kd_con_acquired)			/* L000 begin */
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}				/* L000 end */

	Kdws.w_active = ch0p->ch_id;
	Kdws.w_qp = ch0p->ch_qp;

	vp = &ch0p->ch_vstate;
	tsp = &ch0p->ch_tstate;
	kbp = &ch0p->ch_kbstate;

#ifdef EVGA
	evga_ext_rest((unchar)cur_mode_is_evga);
#endif	/* EVGA */

	/*
	 * If the channel zero is in a graphics mode, call
	 * kdv_textmode() to program the video controller to
	 * text mode. Otherwise, call kdv_setdisp() to program
	 * it correctly for the new VTs text mode.
	 */
	if (ch0p->ch_dmode == KD_GRAPHICS)
		kdv_textmode(ch0p);
	else
		kdv_setdisp(ch0p, vp, tsp, vp->v_cvmode);

#ifdef EVGA
	evga_ext_init(vp->v_cvmode);
#endif	/* EVGA */

	ASSERT(ch0p->ch_dmode != KD_GRAPHICS);

	if (VTYPE(V400) || DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) {
		if (vp->v_undattr == UNDERLINE) {
			kdv_setuline(vp, 1);
			kdv_mvuline(vp, 1);
		} else {
			kdv_setuline(vp, 0);
			kdv_mvuline(vp, 0);
		}
	}

	/*
	 * Copy the saved screen buffer of the VT to
	 * the video memory and position the cursor.
	 */
	kdsetbase(ch0p, tsp);
	kdv_scrxfer(ch0p, KD_BUFTOSCR);
	kdsetcursor(ch0p, tsp);

	/*
	 * If on a VGA controller, program the controller
	 * to interpret the blink/bright background attribute
	 * as set in the termstate_t structure.
	 */
	if (DTYPE(Kdws, KD_VGA)) {

		(void) inb(vp->v_regaddr + IN_STAT_1);
		outb(0x3c0, 0x10);	/* attribute mode control reg */
		tmp = inb(0x3c1);
		if (tsp->t_flags & T_BACKBRITE)
			outb(0x3c0, (tmp & ~0x08));
		else
			outb(0x3c0, (tmp | 0x08));
		outb(0x3c0, 0x20);	/* turn palette on */
	}

	kdv_enable(vp);

	ch0p->ch_flags |= CHN_ACTV;

	if(got_con){					/* L000 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L000 end */

	return(achp);
}
