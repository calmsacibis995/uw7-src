#ident	"@(#)kdvmstr.c	1.27"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	22Jan97		rodneyh@sco.com
 *	- Changes for Gemini multiconsole support
 *	  Includes new CON_MEM_MAPPED ioctl and call to ws_con_maybe_bind
 *	L001	5Mar97		rodneyh@sco.com
 *	- Fix for some L000 crap in kdvmstr_doioctl.
 *	  Change two ws_iocnack calls in rv = error code.
 *	  Remove getq() rubbish and correctly get ioctl arguments.
 *	L002	7Mar97		rodneyh@sco.com
 *	- Moved KDGETFONT and KDSETFONT ioctl processing here from kdmioctlmsg.
 *	L003	25Apr97		rodneyh@sco.com
 *	- Fix L000, check for failure of copyin was backwards.
 *	- Change usage of ws_con_maybe_bind to reflect it now has a return
 *	  value.
 *	L004	4Jun97		rodneyh@sco.com
 *	- Change to CON_MEM_MAPPED ioctl to only proc_unref() and clear map
 *	  state during the last expected unmap.
 *	L005	8Oct97		rodneyh@sco.com
 *	- Change to kdvm_modeioctl() and evga_modeioctl() error cases
 *	  to not exit whilst holding the workstation lock in exclusive mode.
 *	  Fix for ul97-27548.
 *	- Change to kdvm_xenixmap() to reaquire the lock before exiting.
 *	L006	15Oct97		rodneyh@sco.com
 *	- Include ws/ws_priv.h because prototype of ws_con_maybe_bind moved
 *	  from ws.h
 *	L007	19Nov97		rodneyh@sco.com
 *	- Changed handling of CON_MEM_MAPPED ioctl to not use the workstation
 *	  structure for tracking how many mapping operations the user
 *	  process has made on this channel. ws_con_maybe_bind now tracks this
 *	  privately via the ws_bind_t array. This prevented multiple instances
 *	  of the X server and other graphics apps from cleanly closing down
 *	  and returning their VT's to text mode.
 *	  Fix for ul97-31688.
 *
 */

/*
 * The STREAMS video memory driver for the integral console workstation.
 *
 * It contains routines to support video memory mapping to user
 * address space. The kdvmstr_ioctl() calls strioccall() to schedule
 * kdvmstr_doioctl() to be run in stream head where it has the user 
 * context available.
 */

#include <acc/priv/privilege.h>
#include <io/ansi/at_ansi.h>
#include <io/event/event.h>
#include <io/gvid/vdc.h>
#include <io/gvid/vid.h>
#include <io/kd/kb.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termios.h>
#include <io/uio.h>
#include <io/ws/chan.h>
#ifndef NO_MULTI_BYTE
#include <io/ws/mb.h>
#endif /* NO_MULTI_BYTE */
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/ws/ws_priv.h>		/* L006 */
#include <io/xque/xque.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/iobitmap.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>	/* Must come last. */


struct kdvm_iocarg {
	ws_channel_t    *kdvm_chp;
	struct iocblk   kdvm_iocb;
	int             kdvm_arg;
};

int		kdvm_unmapdisp(ws_channel_t *, struct map_info *);
int		kdvm_xenixmap(ws_channel_t *, int, int *);
int		kdvm_map(void *, ws_channel_t *, struct map_info *, 
					struct kd_memloc *);

STATIC int	kdvmstr_doioctl(struct kdvm_iocarg *, long, queue_t *, int *);
STATIC int	kdvm_modeioctl(ws_channel_t *, int);
STATIC int	kdvm_xenixioctl(ws_channel_t *, int, int, int *, int *);
STATIC int	kdvm_xenixdoio(ws_channel_t *, int, int);
STATIC int	kdvm_mapdisp(ws_channel_t *, struct map_info *, int);
#ifdef EVGA
STATIC int	evga_modeioctl(ws_channel_t *, int);

extern int	evga_init(int);
#endif /* EVGA */

extern void	kdv_setmode(ws_channel_t *, unchar);
extern int	kdv_disptype(ws_channel_t *, int);
extern int	kdv_colordisp(void);
extern int	kdv_xenixctlr(void);
extern int	kdv_xenixmode(ws_channel_t *, int, int *);
extern void	kdv_textmode(ws_channel_t *);
extern void     kdv_text1(ws_channel_t *);
extern int	kdv_setxenixfont(ws_channel_t *, int, caddr_t);
extern int	kdv_getxenixfont(ws_channel_t *, int, caddr_t);
extern int	kdv_modromfont(caddr_t, unsigned int);
extern int	kdv_release_fontmap(void);
extern int	kdv_sborder(ws_channel_t *, long);
extern void	kdv_scrxfer(ws_channel_t *, int);

extern int	kdsetcursor(ws_channel_t *, termstate_t *);
extern int	kdsetbase(ws_channel_t *, termstate_t *);

extern int	kdvt_ioctl(ws_channel_t *, int, int, int *);

struct kd_range	kd_basevmem[MKDIOADDR] = {
		{ 0xa0000, 0xc0000 },
#ifdef	EVC
		{ (unsigned long)0xD0000000, (unsigned long)0xD00FFFFF },
#endif	/* EVC */
};

extern wstation_t Kdws;
extern struct vdc_info Vdc;
extern int kdvmemcnt;
extern struct kd_range kdvmemtab[];
extern struct font_info kd_romfonts[];

#ifdef EVGA
extern int evga_inited;  
extern int new_mode_is_evga;
extern int evga_mode;
extern int evga_num_disp;
extern struct at_disp_info disp_info[];
extern unsigned long evga_type;
#endif /* EVGA */


/*
 * void
 * kdvmstr_ioctl(queue_t *, mblk_t *, struct iocblk *, ws_channel_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
void
kdvmstr_ioctl(queue_t *qp, mblk_t *mp, struct iocblk *iocbp, ws_channel_t *chp)
{
	struct kdvm_iocarg  *kvmp;


	if (chp->ch_iocarg == NULL) {
		chp->ch_iocarg = allocb(sizeof(struct kdvm_iocarg), BPRI_MED);
		if (chp->ch_iocarg == NULL) {
			/*
			 *+ There is not enough memory to allocate for
			 *+ struct kdvm_iocarg. Check system configuration.
			 */
			cmn_err(CE_NOTE, 
				"kdvmstr_ioctl: can't allocate memory");
			ws_iocnack(qp, mp, iocbp, ENOMEM);
			return;
		}
	}

	/* LINTED pointer alignment */
	kvmp = (struct kdvm_iocarg *) chp->ch_iocarg->b_rptr;
	kvmp->kdvm_iocb = *iocbp;
	kvmp->kdvm_chp  = chp;

	if (mp->b_cont != NULL)
		/* LINTED pointer alignment */
		kvmp->kdvm_arg = *(int *)mp->b_cont->b_rptr;
	else
		kvmp->kdvm_arg = -1;

	strioccall(kdvmstr_doioctl, kvmp, iocbp->ioc_id, qp);

	ws_iocack(qp, mp, iocbp);
}



/*
 * STATIC int
 * kdvmstr_doioctl(struct kdvm_iocarg *, long, queue_t *, int *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry or exit.
 */
/* ARGSUSED */
STATIC int
kdvmstr_doioctl(struct kdvm_iocarg *kvmp, long iocid, queue_t *qp, int *rvalp)
{
	struct iocblk	*iocbp;
	ws_channel_t	*chp;
	termstate_t	*tsp;
	int		indx, cnt, cmd, arg;
	int		rv = 0; 
	int		hit = 1;
	void		*procp;
	pl_t		opl;
	extern int	cgi_mapclass(ws_channel_t *, int, int *);
	extern void	ws_set_vt_proc_info(int);


	chp = kvmp->kdvm_chp;
	iocbp = &kvmp->kdvm_iocb;
	cmd   = iocbp->ioc_cmd;
	arg   = kvmp->kdvm_arg;
	indx  = chp->ch_id;
	tsp = &chp->ch_tstate;

	switch (cmd) {

	/* Enhanced Application Compatibility Support */

        case SCO_MAP_CLASS:
                cgi_mapclass(chp, arg, rvalp);
                break;

        case CONS_BLANKTIME:
                if (arg < 0)
                        rv = EINVAL;
                break;

	case LDEV_MSESETTYPE:
        case LDEV_SETTYPE:
        case LDEV_SETRATIO:
                break;

        case LDEV_ATTACHQ:
        case LDEV_MSEATTACHQ:
#ifdef DEBUG
		/*
		 *+ To shut up klint.
		 */
                cmn_err(CE_NOTE, 
			"Calling ws_queuemode(chp, LDEV_ATTACHQ, %x)", arg);
#endif /* DEBUG */
                rv = ws_queuemode(chp, cmd, arg);
                break;

        case CONSADP:
                if ((chp = ws_activechan(&Kdws)) == NULL) {
                        *rvalp = rv = -1;
                } else {
                        rv = 0;
                        *rvalp = chp->ch_id;
                }
                break;

	/* End Enhanced Application Compatibility Support */

	case KDSBORDER:	/* set border in ega color text mode */
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
		rv = kdv_sborder(chp, arg);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	case KDDISPTYPE: /* return display information to user */
		rv = kdv_disptype(chp, arg);
		break;

        case KDVDCTYPE3_2:
                ws_set_vt_proc_info(indx);
		/* FALLTHROUGH */

	case KDVDCTYPE:	/* return VDC controller/display information */
		rv = vdc_disptype(chp, arg);
		break;

	case KIOCSOUND:
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		if (WS_ISACTIVECHAN(&Kdws, chp))
			kdkb_sound(arg);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		rv = 0;
		break;

	case KDSETMODE:
		/* Here mode pertains to graphics or text. */
		if (chp->ch_vstate.v_scrp == NULL || 
            	    Kdws.w_scrbufpp[chp->ch_id] == NULL) {
			/* This channel has just been closed */
                       	*rvalp = rv = -1;
			break;
		}
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);

		switch ((int) arg) {
		case KD_TEXT0:
		case KD_TEXT1:
			(void) LOCK(chp->ch_mutex, plstr);

			if (!CHNFLAG(chp, CHN_XMAP) && CHNFLAG(chp, CHN_UMAP)) {
				UNLOCK(chp->ch_mutex, plstr);
				rv = EIO;
				break;
			}

			if (chp->ch_dmode == arg) {
				UNLOCK(chp->ch_mutex, plstr);
				break;
			}

#ifndef NO_MULTI_BYTE
			/*
			 * If we are switching from graphics text mode to
			 * text mode, deallocate resources associated with
			 * graphics text mode.
			 */
			if (chp->ch_dmode == KD_GRTEXT)
				Kdws.w_consops->cn_gs_free(Kdws.w_consops,
								chp, tsp);
#endif /* NO_MULTI_BYTE */

			chp->ch_dmode = (unchar)arg;

			if (arg == KD_TEXT0)
				kdv_textmode(chp);
			else
				kdv_text1(chp);

			UNLOCK(chp->ch_mutex, plstr);
			break;
		
		case KD_GRAPHICS: 
			(void) LOCK(chp->ch_mutex, plstr);

			if (chp->ch_dmode == KD_GRAPHICS) {
				UNLOCK(chp->ch_mutex, plstr);
				break;
			}

			ASSERT(getpl() == plstr);

#ifndef NO_MULTI_BYTE
			/*
			 * If we are switching from graphics text mode to
			 * text mode, deallocate resources associated with
			 * graphics text mode.
			 */
			if (chp->ch_dmode == KD_GRTEXT)
				Kdws.w_consops->cn_gs_free(Kdws.w_consops,
								chp, tsp);
#endif /* NO_MULTI_BYTE */

			kdv_scrxfer(chp, KD_SCRTOBUF);

			chp->ch_dmode = KD_GRAPHICS;

			/*
 			 * If start address has changed, we must re-zero
			 * the screen buffer so as to not confuse VP/ix.
			 */
			tsp = &chp->ch_tstate;
			if (tsp->t_origin) {
				tsp->t_cursor -= tsp->t_origin;
				tsp->t_origin = 0;
				kdsetbase(chp, tsp);
				kdsetcursor(chp, tsp);
				kdv_scrxfer(chp, KD_BUFTOSCR);
			}

			UNLOCK(chp->ch_mutex, plstr);
			break;

#ifndef NO_MULTI_BYTE
		/*
		 * WE SHOULD CHECK WHETHER OR WHAT SHOULD BE DONE HERE.
		 */
		case KD_GRTEXT:
			break;
#endif /* NO_MULTI_BYTE */
		} /* arg switch */

		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	case KDGETMODE:
		if (copyout(&chp->ch_dmode, (caddr_t) arg, 
				sizeof(chp->ch_dmode)) < 0)
			rv = EFAULT;

		return (rv);

	case KDQUEMODE:	/* enable/disable special queue mode */
		rv = ws_queuemode(chp, cmd, arg);
		break;

	case KDMKTONE:
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);

		/*
		 * The w_rwlock is released here to prevent
		 * holding the lock while the callee may go 
		 * to sleep. The same reasoning applies for 
		 * not holding the basic channel lock before 
		 * calling kdkb_mktone().
		 */
		if (WS_ISACTIVECHAN(&Kdws, chp)) {
			RW_UNLOCK(Kdws.w_rwlock, opl);
			kdkb_mktone(chp, arg);
			return (rv);
		}

		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	case KDMAPDISP:
		/*
		 * Check if any chansw/modesw/mapdisp operations is 
		 * in progress.
		 */
		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_flags & (WS_NOCHANSW|WS_NOMODESW|WS_NOMAPDISP)) {
			SV_WAIT(Kdws.w_flagsv, (primed+1), Kdws.w_mutex);
			opl = LOCK(Kdws.w_mutex, plstr);
		}
		Kdws.w_flags |= WS_NOMAPDISP|WS_NOCHANSW;
		UNLOCK(Kdws.w_mutex, opl);

		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
		rv = kdvm_mapdisp(chp, &Kdws.w_map, arg);
		Kdws.w_flags &= ~(WS_NOMAPDISP|WS_NOCHANSW);
		SV_SIGNAL(Kdws.w_flagsv, 0);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

#ifndef NO_MULTI_BYTE				/* L002 begin */

	case KDGETFONT:
	case KDSETFONT:
		/*
		 * Just push these at the fnt driver, make sure its loaded
		 * first though.
		 */
		if(!fnt_init_flg){

			rv = EINVAL;
			break;
		}
		
		if(cmd == KDGETFONT){

			if(Gs.fnt_setfont == NULL){
				/*
				 * Its not loaded propetly, this probably means
				 * that the fnt driver is pre-gemini
				 */
				rv = ENODEV;
				break;
			}
			rv = Gs.fnt_setfont(chp, (caddr_t)arg);

		}else{
			if(Gs.fnt_getfont == NULL){

				rv = ENODEV;
				break;
			}
			rv = Gs.fnt_setfont(chp, (caddr_t)arg);
		}

		break;

#endif	/* NO_MULTI_BYTE */			/* L002 end */

	case CON_MEM_MAPPED: {			/* L000 begin */

		void *procp;
		struct con_mem_map_req mem_req;

		/* 
		 * Replaces KDMAPDISP.
		 * We get the chance to change the physical address the user
		 * wants to map but we don't really want too.
		 *
		 * Note, we have user context here thanks to strioccall
		 *
		 */

		/*
		 * Figure out if its a startup or shutdown request
		 */
		if(arg == -1){			/* Arguments missing, L001 */

			rv = EINVAL;
			break;
		}

		if(copyin((caddr_t)arg, &mem_req,		/* L001 */
			sizeof(struct con_mem_map_req)) == -1){	/* L003 */

			rv = EFAULT;
			break;
		}

		if(mem_req.length){

			/*
			 * This is a map request, first check the channel state
			 */
			opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

			if(chp != ws_activechan(&Kdws) ||
					chp->ch_dmode != KD_GRAPHICS){
			
				(void)RW_UNLOCK(Kdws.w_rwlock, opl);
				rv = EACCES;			/* L001 */

				break;
			}

#if 0			/* L007 */

			/*
			 * Save state for later
			 */
			if(!(Kdws.w_map.m_cnt)++){

				Kdws.w_map.m_procp = proc_ref();
				Kdws.w_map.m_chan = chp->ch_id;
				chp->ch_flags |= CHN_UMAP;
			}

#endif			/* L007 */

			chp->ch_flags |= CHN_UMAP;	/* L007 */

			(void)RW_UNLOCK(Kdws.w_rwlock, opl);

			/*
			 * Tell WS we need to bind the LWPs in this proc if a
			 * multiconsole graphics session is started.
			 */
			rv = ws_con_maybe_bind(u.u_procp, B_FALSE);  /* L003 */
		}
		else{
			/*
			 * This is an unmap indication
			 */
			opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

#if 0			/* L007 */

			procp = proc_ref();
			if(Kdws.w_map.m_procp != procp){

				proc_unref(procp);
				RW_UNLOCK(Kdws.w_rwlock, opl);
				rv = EACCES;			/* L001 */

				break;
			}
			proc_unref(procp);

#endif			/* L007 */

			if(chp != ws_activechan(&Kdws) ||
					chp->ch_dmode != KD_GRAPHICS){
			
				(void)RW_UNLOCK(Kdws.w_rwlock, opl);
				rv = EACCES;			/* L001 */

				break;
			}

			chp->ch_vstate.v_font = 0;

#if 0			/* L007 */
			
			/*
			 * NOTE:
			 *	We should wait here until all channel switch
			 *	operations have completed
			 *
			 * L004 COMMENT:
			 *	Only proc_unref and clear the state if this is
			 *	the last unmap.
			 */
			if(Kdws.w_map.m_cnt == 1){
				/*
				 * This is the last unmap so, unref the proc
				 * refs in the map operation
				 */
				ASSERT(proc_valid(Kdws.w_map.m_procp));
				proc_unref(Kdws.w_map.m_procp);

				/*
				 * Clear out all the state we saved
				 */
				Kdws.w_map.m_procp = (void *)0;
				chp->ch_flags &= ~CHN_MAPPED;
				Kdws.w_map.m_chan = 0;
				Kdws.w_map.m_cnt = 0;

			}	/* End if last unmap */

#endif			/* L007 */

			RW_UNLOCK(Kdws.w_rwlock, opl);

			/*
			 * Tell WS no more callouts.
			 */
			rv = ws_con_maybe_bind(u.u_procp, B_TRUE);  /* L003 */

			if(!rv)					/* L007 */
				chp->ch_flags &= ~CHN_MAPPED;	/* L007 */

			if(rv == EAGAIN){			/* L007 begin */
				/*
				 * ws_con_maybe_bind returns EAGAIN if the
				 * count of maps made by this channel is non
				 * zero after the current unmap operation so
				 * we make sure the channel mapped flag is set
				 * here. Note that there are many places that
				 * this flag will now be erronously cleared in
				 * the KD and WS drivers.
				 */
				chp->ch_flags |= CHN_MAPPED;
				rv = 0;

			}					/* L007 end */

		}	/* End else this is a CON_MEM_MAPPED unmap */

		break;

	}			/* End CON_MEM_MAPPED, L000 end */

	case KDUNMAPDISP:
		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
		procp = proc_ref();
		if (Kdws.w_map.m_procp != procp) {
			proc_unref(procp);
			RW_UNLOCK(Kdws.w_rwlock, opl);
			rv = EACCES;
			break;
		}
		proc_unref(procp);
		RW_UNLOCK(Kdws.w_rwlock, opl);

		/*
		 * Check if any chansw/modesw/mapdisp operations is 
		 * in progress.
		 */
		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_flags & (WS_NOCHANSW|WS_NOMODESW|WS_NOMAPDISP)) {
			SV_WAIT(Kdws.w_flagsv, (primed+1), Kdws.w_mutex);
			opl = LOCK(Kdws.w_mutex, plstr);
		}
		Kdws.w_flags |= WS_NOMAPDISP|WS_NOCHANSW;
		UNLOCK(Kdws.w_mutex, opl);

		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
		rv = kdvm_unmapdisp(chp, &Kdws.w_map);
		Kdws.w_flags &= ~(WS_NOMAPDISP|WS_NOCHANSW);
		SV_SIGNAL(Kdws.w_flagsv, 0);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	case KDENABIO:
		iobitmapctl(IOB_ENABLE, chp->ch_vstate.v_ioaddrs);
		break;

	case KDDISABIO:
		iobitmapctl(IOB_DISABLE, chp->ch_vstate.v_ioaddrs);
		break;

	case KDADDIO: {
		vidstate_t	*vp;

		if (pm_denied(CRED(), P_SYSOPS))
			return (EPERM);

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);

		vp = &chp->ch_vstate;

		for (indx = 0; indx < MKDIOADDR; indx++) {
			if (!vp->v_ioaddrs[indx]) {
				vp->v_ioaddrs[indx] = (ushort) arg;
				break;
			}
		}

		if (indx == MKDIOADDR)
			rv = EIO;

		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;
	}

	case KDDELIO: {
		vidstate_t	*vp;

		if (pm_denied(CRED(), P_SYSOPS))
			return (EPERM);

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);

		vp = &chp->ch_vstate;

		for (indx = 0; indx < MKDIOADDR; indx++) {
			if (vp->v_ioaddrs[indx] != (ushort)arg)
				continue;
			for (cnt = indx; cnt < (MKDIOADDR - 1); cnt++)
				vp->v_ioaddrs[cnt] = vp->v_ioaddrs[cnt + 1];
			vp->v_ioaddrs[cnt] = (ushort)0;
			break;
		}

		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;
	}

	case WS_PIO_ROMFONT: { 
		/*
		 * Replace character within a font. Note not all characters
		 * within a font are replaced.
		 */

		unsigned int numchar, size;
		caddr_t newbuf;
		
		if (copyin((caddr_t) arg, &numchar, sizeof(numchar)) == -1)
			return (EFAULT);

		if (numchar > MAX_ROM_CHAR)
			return (EINVAL);

		if (numchar == 0) {
			opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
			rv = kdv_release_fontmap(); 
			RW_UNLOCK(Kdws.w_rwlock, opl);
			return (rv);
		}

		size = sizeof(numchar) + numchar * sizeof(struct char_def);
		newbuf = (caddr_t)kmem_alloc(size, KM_SLEEP);
		if (newbuf == NULL)
			return (ENOMEM);

		if (copyin((caddr_t)arg, newbuf, size) == -1)
			return (EFAULT);

		/*
		 * Check if any fontmod operation is in progress.
		 */
		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_flags & (WS_NOFONTMOD|WS_NOCHANSW)) {
			SV_WAIT(Kdws.w_flagsv, primed + 1, Kdws.w_mutex);
			opl = LOCK(Kdws.w_mutex, plstr);
		}
		Kdws.w_flags |= WS_NOFONTMOD|WS_NOCHANSW;
		UNLOCK(Kdws.w_mutex, plstr);

		(void) RW_WRLOCK(Kdws.w_rwlock, plstr);
		rv = kdv_modromfont(newbuf, numchar);
		Kdws.w_flags &= ~(WS_NOFONTMOD|WS_NOCHANSW);
		SV_SIGNAL(Kdws.w_flagsv, 0);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;
	}

#ifdef EVGA
	case KDEVGA:
		if (!DTYPE(Kdws, KD_VGA)) {
			/*
			 * Card has already been successfully 
			 * identified; can't be evga.
			 */
			rv = EINVAL;
		} else {
			opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
			/*
			 * Card was a vanilla-type VGA, could
			 * be an evga.
			 */
			rv = evga_init(arg);
			RW_UNLOCK(Kdws.w_rwlock, opl);
		}

		break;
#endif /* EVGA */

	default:
		/*
		 * Mode change hits the default case. Cmd is new mode
		 * or'ed with MODESWITCH.
		 */

		switch (cmd & 0xffffff00) {
		case VTIOC:		/* VT ioctl */
			rv = kdvt_ioctl(chp, cmd, arg, rvalp);
			return (rv);

                case USL_OLD_MODESWITCH: /* old USL mode switch ioctl */
                        cmd = (cmd & ~IOCTYPE) | MODESWITCH;
			/* FALLTHROUGH */

		case MODESWITCH:	/* UNIX mode switch ioctl */
			rv = kdvm_modeioctl(chp, cmd);
			break;

#ifdef EVGA
		case EVGAIOC:		/* evga mode switch ioctl */
		    	if (evga_inited)
				rv = evga_modeioctl(chp, cmd);
		    	else 
				rv = ENXIO;
			break;
#endif /* EVGA */

		default:
			rv = kdvm_xenixioctl(chp, cmd, arg, rvalp, &hit);
			if (hit == 1)			/* Xenix ioctl */
				break;
		}

		break;
	}

	return (rv);
}


/*
 * STATIC int
 * kdvm_modeioctl(ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
/* ARGSUSED */
STATIC int
kdvm_modeioctl(ws_channel_t *chp, int cmd)
{
	int		rv = 0;
	ws_channel_t	*achp;
	void		*procp;
	pl_t		opl1, opl2;
#ifdef EVGA
	int		generic_mode;
#endif /* EVGA */


	cmd &= ~O_MODESWITCH;
	cmd |= MODESWITCH;

#ifdef EVGA
	if (evga_inited) {
		generic_mode = 0;
	}
#endif /* EVGA */

	/*
	 * If kd has been intialized for evga, then DTYPE is still
	 * KD_VGA so modes that require only DTYPE of KD_VGA should
	 * succeed. Requests for modes that require other DTYPEs 
	 * or have additional requirements may fail.
	 */

	switch (cmd) {
	/* have to check for Xenix modes */
	case SW_ATT640:
		if (!(VTYPE(V750) && VSWITCH(ATTDISPLAY)) && 
		    !VTYPE(V600 | CAS2)) 
			return (ENXIO);
		break;

	case SW_ENHB80x43:
	case SW_ENHC80x43:
		if ((VTYPE(V600)) || 
		    (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA)))
			return (ENXIO);
		cmd -= OFFSET_80x43;
		break;

	case SW_VGAB40x25:
	case SW_VGAB80x25:
	case SW_VGAC40x25:
	case SW_VGAC80x25:
	case SW_VGAMONO80x25:
	case SW_VGA640x480C:
	case SW_VGA640x480E:
	case SW_VGA320x200:
		if (!DTYPE(Kdws, KD_VGA))
			return (ENXIO);
		break;

	case SW_MCAMODE:
		if (!(DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES)))
			return (ENXIO);
		break;

	case SW_VDC640x400V:
		if (!VTYPE(V600))
			return (ENXIO);
		break;

	case SW_VDC800x600E:
		/*
		 * Not VDC-600 or CAS-2. 
		 */
		if (!VTYPE(V600 | CAS2) || (Vdc.v_info.dsply != KD_MULTI_M &&
					 Vdc.v_info.dsply != KD_MULTI_C)) {
#ifdef EVGA
			if (evga_inited) {
				generic_mode = SW_GEN_800x600;
			} else {
#endif /* EVGA */
				return (ENXIO);
#ifdef EVGA
			}
#endif /* EVGA */
		}
		break;

	case SW_CG320_D:
	case SW_CG640_E:
	case SW_ENH_MONOAPA2:
	case SW_VGAMONOAPA:
	case SW_VGA_CG640:
	case SW_ENH_CG640:
	case SW_ENHB40x25:
	case SW_ENHC40x25:
	case SW_ENHB80x25:
	case SW_ENHC80x25:
	case SW_EGAMONO80x25:
		if (!(DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)))
			return (ENXIO);
		break;

	case SW_CG640x350:
	case SW_EGAMONOAPA:
		if (!(DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)))
			return (ENXIO);
		/*
		 * For all VGA and the VDC 750, switch from F to F*
		 * since we know we have enough memory.
		 * For other EGA cards that we can't identify, keep 
		 * your fingers crossed and hope that mode F works
		 */
		if (VTYPE(V750) || DTYPE(Kdws, KD_VGA))
			cmd += 2;
		break;

	case SW_B40x25:
	case SW_C40x25:
	case SW_B80x25:
	case SW_C80x25:
	case SW_BG320:
	case SW_CG320:
	case SW_BG640:
		if (DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES))
			return (ENXIO);
		break;
#ifdef	EVC
	case SW_EVC1024x768E: 
		/*
		 * EVC-1 with hi-res monitor only 
		 */
		if (!VTYPE(VEVC) || (Vdc.v_info.dsply != KD_MULTI_M &&
					Vdc.v_info.dsply != KD_MULTI_C)) {
#ifdef EVGA
			if (evga_inited) {
				generic_mode = SW_GEN_1024x768;
			} else {
#endif /* EVGA */
				return (ENXIO);
#ifdef EVGA
			}
#endif /* EVGA */
		}
		break;

	case SW_EVC1024x768D:
		/*
		 * EVC-1 with hi-res monitor only. 
		 */
		if (!VTYPE(VEVC) || (Vdc.v_info.dsply != KD_MULTI_M &&
					Vdc.v_info.dsply != KD_MULTI_C)) 
                        return (ENXIO);
		break;

	case SW_EVC640x480V:
		if (!VTYPE(VEVC))
                        return (ENXIO);
		break;
#endif	/* EVC */

#ifdef  EVGA
	/*
	 * Temporary kludge for X server. 
	 */
	case TEMPEVC1024x768E:
		if (evga_inited) {
			generic_mode = SW_GEN_1024x768;
		} else
			return (ENXIO);
		break;
#endif /* EVGA */

	default:
		return (ENXIO);
	}

#ifdef EVGA
	if (evga_inited && generic_mode) {
		rv = evga_modeioctl(chp, generic_mode);
		return(rv);
	}
#endif /* EVGA */

	ASSERT(rv == 0);

	opl1 = RW_WRLOCK(Kdws.w_rwlock, plstr);

	achp = WS_ACTIVECHAN(&Kdws);

	opl2 = LOCK(Kdws.w_mutex, plstr);
	ws_mapavail(achp, &Kdws.w_map);
	procp = proc_ref();
	if ((Kdws.w_map.m_procp != procp) && (!CHNFLAG(chp, CHN_XMAP) 
					&& CHNFLAG(chp, CHN_UMAP))) {
		proc_unref(procp);
		UNLOCK(Kdws.w_mutex, opl2);
		RW_UNLOCK(Kdws.w_rwlock, opl1);		/* L005 */
		return(EIO);
	}
	proc_unref(procp);
	UNLOCK(Kdws.w_mutex, opl2);

	if (cmd == SW_MCAMODE) {
		/* Use bogus mode to force reset */
		chp->ch_vstate.v_dvmode = 0xff;
		kdv_setmode(chp, DM_EGAMONO80x25);
	} else 
		kdv_setmode(chp, (unchar)(cmd & KDMODEMASK));

	RW_UNLOCK(Kdws.w_rwlock, opl1);

	return (rv);
}


#ifdef EVGA

/*
 * STATIC int
 * evga_modeioctl(ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC int
evga_modeioctl(ws_channel_t *chp, int cmd)
{
	int		rv = 0;
	int		i, gen_mode;
	int		color, x, y;
	int		newmode;
	ws_channel_t	*achp;
	void		*procp;
	struct at_disp_info *disp;
	pl_t		opl1, opl2;


	gen_mode = (cmd & EVGAMODEMASK);
	color = 16;			/* for most modes */

	switch (gen_mode) {
	case GEN_640x350:
		x = 640; 
		y = 350;
		break;

	case GEN_640x480:
		x = 640; 
		y = 480;
		break;

	case GEN_720x540:
		x = 720; 
		y = 540;
		break;

	case GEN_800x560:
		x = 800; 
		y = 560;
		break;

	case GEN_800x600:
		x = 800; 
		y = 600;
		break;

	case GEN_960x720:
		x = 960; 
		y = 720;
		break;

	case GEN_1024x768:
	case GEN_1024x768x2:
	case GEN_1024x768x4:
		x = 1024; 
		y = 768;

		switch (gen_mode) {
		case GEN_1024x768x2:
			color = 2;
			break;
		case GEN_1024x768x4:
			color = 4;
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}

	/*
	 * Look for a match in disp_info, given the board type 	
	 * present, the resolution requested, and the number of
	 * (implicit) colors requested.       	
	 */
	for (i = 0, disp = disp_info; i < evga_num_disp; i++, disp++) {
		if ((evga_type == disp->type) &&
		    (x == disp->xpix) && (y == disp->ypix) &&
		    (color == disp->colors)) { 
			break;			/* Found a match */
		}
	}

	if (i >= evga_num_disp) {		/* failure */
		return (ENXIO);
	}

	ASSERT(rv == 0);

	opl1 = RW_WRLOCK(Kdws.w_rwlock, plstr);

	achp = WS_ACTIVECHAN(&Kdws);

	opl2 = LOCK(Kdws.w_mutex, plstr);
	ws_mapavail(achp, &Kdws.w_map);
	procp = proc_ref();
	if ((Kdws.w_map.m_procp != procp) && (!CHNFLAG(chp, CHN_XMAP) 
					&& CHNFLAG(chp, CHN_UMAP))) {
		proc_unref(procp);
		UNLOCK(Kdws.w_mutex, pl2);
		RW_UNLOCK(Kdws.w_rwlock, opl1);		/* L005 */
		return (EIO);
	}
	proc_unref(procp);
	UNLOCK(Kdws.w_mutex, opl2);

	/*
	 * Convert relative offset (from beginning of evga modes)
 	 * to absolute offset into kd_modeinfo.
 	 */
	newmode = i + ENDNONEVGAMODE + 1;
	kdv_setmode(chp, (unchar)newmode);

	RW_UNLOCK(Kdws.w_rwlock, opl1);

	return (rv);
}
#endif /* EVGA */


/*
 * STATIC int
 * kdvm_xenixioctl(ws_channel_t *, int, int, int *, int *) 
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
/* ARGSUSED */
STATIC int
kdvm_xenixioctl(ws_channel_t *chp, int cmd, int arg, int *rvalp, int *ioc_hit) 
{
	int	tmp;
	int	rv = 0;
	pl_t	opl;


	switch (cmd) {
	case GIO_COLOR:
		*rvalp = kdv_colordisp();
		break;

	case CONS_CURRENT:
		*rvalp = kdv_xenixctlr();
		break;

	case MCA_GET:
	case CGA_GET:
	case EGA_GET:
	case VGA_GET:
	case CONS_GET:
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
		rv = kdv_xenixmode(chp, cmd, rvalp);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	case MAPMONO:
	case MAPCGA:
	case MAPEGA:
	case MAPVGA:
	case MAPSPECIAL:
	case MAPCONS:
		/*
		 * Acquire the workstation reader/writer lock in 
		 * exclusive mode.
		 */
		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
		rv = kdvm_xenixmap(chp, cmd, rvalp);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	case MCAIO:
	case CGAIO:
	case EGAIO:
	case VGAIO:
	case CONSIO:
		rv = kdvm_xenixdoio(chp, cmd, arg);
		break;

	case SWAPMONO:
		if (DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES))
			break;

		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA)) {
			rv = EINVAL;
			break;
		}

		switch (chp->ch_vstate.v_cvmode) {
		case DM_EGAMONO80x25:
		case DM_EGAMONOAPA:
		case DM_ENHMONOAPA2:
			break;
		default:
			rv = EINVAL;
		}

		break;

	case SWAPCGA:
		if (DTYPE(Kdws, KD_CGA))
			break;

		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA)) {
			rv = EINVAL;
			break;
		}

		switch (chp->ch_vstate.v_cvmode) {
		case DM_B40x25:
		case DM_C40x25:
		case DM_B80x25:
		case DM_C80x25:
		case DM_BG320:
		case DM_CG320:
		case DM_BG640:
		case DM_CG320_D:
		case DM_CG640_E:
			break;
		default:
			rv = EINVAL;
		}

		break;

	case SWAPEGA:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA)) {
			rv = EINVAL;	
			break;
		}

		/*
		 * Fail for any VGA mode or non-standard EGA mode. 
		 */
		if (chp->ch_vstate.v_cvmode >= DM_VGA_C40x25) 
			rv = EINVAL;

		break;

	case SWAPVGA:
		if (!DTYPE(Kdws, KD_VGA)) {
			rv = EINVAL;	
			break;
		}

		/*
		 * Fail for these non-standard VGA modes. 
		 */
		switch (chp->ch_vstate.v_cvmode) {
		case DM_ENH_CGA:
		case DM_ATT_640:
		case DM_VDC800x600E:
		case DM_VDC640x400V:
			rv = EINVAL;
			break;
		default:
			break;
		}

		break;

	case PIO_FONT8x8:
		tmp = FONT8x8;
		goto piofont;

	case PIO_FONT8x14:
		tmp = FONT8x14;
		goto piofont;

	case PIO_FONT8x16:
		tmp = FONT8x16;
piofont:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA))
			return(EINVAL);

		/*
		 * Check if any fontmod operation is in progress.
		 */
		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_flags & (WS_NOFONTMOD|WS_NOCHANSW)) {
			SV_WAIT(Kdws.w_flagsv, primed + 1, Kdws.w_mutex);
			opl = LOCK(Kdws.w_mutex, plstr);
		}
		Kdws.w_flags |= WS_NOFONTMOD|WS_NOCHANSW;
		UNLOCK(Kdws.w_mutex, plstr);

		(void) RW_WRLOCK(Kdws.w_rwlock, plstr);
		rv = kdv_setxenixfont(chp, tmp, (caddr_t)arg);
		Kdws.w_flags &= ~(WS_NOFONTMOD|WS_NOCHANSW);
		SV_SIGNAL(Kdws.w_flagsv, 0);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;
	
	case GIO_FONT8x8:
		tmp = FONT8x8;
		goto giofont;

	case GIO_FONT8x14:
		tmp = FONT8x14;
		goto giofont;

	case GIO_FONT8x16:
		tmp = FONT8x16;
giofont:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA))
			return(EINVAL);

		/*
		 * Check if any fontmod operation is in progress.
		 */
		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_flags & (WS_NOFONTMOD|WS_NOCHANSW)) {
			SV_WAIT(Kdws.w_flagsv, primed + 1, Kdws.w_mutex);
			opl = LOCK(Kdws.w_mutex, plstr);
		}
		Kdws.w_flags |= WS_NOFONTMOD|WS_NOCHANSW;
		UNLOCK(Kdws.w_mutex, plstr);

		(void) RW_WRLOCK(Kdws.w_rwlock, plstr);
		rv = kdv_getxenixfont(chp, tmp, (caddr_t)arg);
		Kdws.w_flags &= ~(WS_NOFONTMOD|WS_NOCHANSW);
		SV_SIGNAL(Kdws.w_flagsv, 0);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		break;

	case KDDISPINFO: {
		struct kd_dispinfo dinfo;

		switch (Kdws.w_vstate.v_type) {
		case KD_MONO:
		case KD_HERCULES:
			dinfo.vaddr = 
				(caddr_t)(dinfo.physaddr = (paddr_t)MONO_BASE);
			dinfo.size = MONO_SIZE;
			break;
		case KD_CGA:
			dinfo.vaddr = 
				(caddr_t)(dinfo.physaddr = (paddr_t)COLOR_BASE);
			dinfo.size = COLOR_SIZE;
			break;
		case KD_VGA:
		case KD_EGA: 
			/*
			 * Assume as we do for MONOAPA2 that the EGA card
			 * has 128K of RAM. Still have the fingers crossed.
			 * Not an issue for VGA -- we always have 128K. 
			 */
			dinfo.vaddr = 
				(caddr_t)(dinfo.physaddr = (paddr_t)EGA_BASE);
			dinfo.size = EGA_LGSIZE;
			break;
		default:
			return (EINVAL);
		}

		if (copyout(&dinfo, (caddr_t)arg, 
				sizeof(struct kd_dispinfo)) < 0)
			rv = EFAULT;
		break;
	}

	case CONS_GETINFO: {
		struct vid_info vinfo;
		termstate_t *tsp;
		
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
		tsp = &chp->ch_tstate;
		vinfo.size = sizeof(struct vid_info);
		vinfo.m_num = chp->ch_id;
		vinfo.mv_row = tsp->t_row;
		vinfo.mv_col = tsp->t_col;
		vinfo.mv_rsz = tsp->t_rows;
		vinfo.mv_csz = tsp->t_cols;
		vinfo.mv_norm.fore = tsp->t_curattr & 0x07;
		vinfo.mv_norm.back = (int)(tsp->t_curattr & 0x70) >> 4;
		vinfo.mv_rev.fore = 0; /* reverse always black on white */
		vinfo.mv_rev.back = 7;
		vinfo.mv_grfc.fore = 7;/* match graphics with background info */
		vinfo.mv_grfc.back = 0;
		/*
                 * AND with 0xf to strip the SG and SR bits if SB is set. 
		 */
		vinfo.mv_ovscan =  chp->ch_vstate.v_border & 0xf;
		vinfo.mk_keylock = chp->ch_kbstate.kb_state / CAPS_LOCK;
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		if (copyout(&vinfo, (caddr_t)arg, sizeof(struct vid_info)) < 0)
			return (EFAULT);
		break;
	}

	case CONS_6845INFO: {
		struct m6845_info minfo;

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
		minfo.size = sizeof(struct m6845_info);
		minfo.cursor_type = chp->ch_tstate.t_curtyp;
		minfo.screen_top = chp->ch_tstate.t_origin;
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		if (copyout(&minfo, (caddr_t)arg, 
				sizeof(struct m6845_info)) < 0)
			return (EFAULT);
		break;
	}

	case EGA_IOPRIVL:
		if (!DTYPE(Kdws, KD_EGA))
			return(EINVAL);

		/*
		 * No locks are held because iobitmapctl() can block.
		 */
		*rvalp = iobitmapctl(IOB_CHECK, chp->ch_vstate.v_ioaddrs);
		break;

	case VGA_IOPRIVL:
		if (!DTYPE(Kdws, KD_VGA))
			return(EINVAL);

		/*
		 * No locks are held because iobitmapctl() can block. 
		 */
		*rvalp = iobitmapctl(IOB_CHECK, chp->ch_vstate.v_ioaddrs);
		break;

	case PGAIO:
	case PGAMODE:
	case MAPPGA:
	case PGA_GET:
	case SWAPPGA:
	case CGAMODE:
	case EGAMODE:
	case VGAMODE:
	case MCAMODE:
	case KIOCNONDOSMODE:
	case SPECIAL_IOPRIVL:
	case INTERNAL_VID:
	case EXTERNAL_VID:
		rv = EINVAL;
		break;

	default:
		*ioc_hit = 0;
		break;
	}

	return (rv);
}


/*
 * STATIC int
 * kdvm_mapdisp(ws_channel_t *, struct map_info *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *	- On success, return with a valid reference to the current process.
 *
 * Remarks:
 *	Only the active channel can map the video display
 *	to its address space.
 *
 *	The reference to the process will only be released
 *	when kdvm_unmapdisp() is called. This is conditioned
 *	upon successful mapping of the video buffer to the
 *	user address space.
 */
STATIC int
kdvm_mapdisp(ws_channel_t *chp, struct map_info *map_p, int arg)
{
	struct kd_memloc memloc;
	void	*procp;
	int	rv;


	if (chp != ws_activechan(&Kdws) || chp->ch_dmode != KD_GRAPHICS)
		return (EACCES);

	ws_mapavail(chp, map_p);

	/*
	 * release the lock before copyin as per DDI/DKI
	 */
	RW_UNLOCK(chp->ch_wsp->w_rwlock, plbase);

	if (copyin((caddr_t)arg, &memloc, sizeof(memloc)) < 0) {
		(void) RW_WRLOCK(chp->ch_wsp->w_rwlock, plstr);
		return (EFAULT);
	}

	(void) RW_WRLOCK(chp->ch_wsp->w_rwlock, plstr);

	procp = proc_ref();
	if (map_p->m_procp && map_p->m_procp != procp || 
			map_p->m_cnt == CH_MAPMX) {
		proc_unref(procp);
		return (EBUSY);
	}

	/*
	 * If the video buffer is successfully mapped to user
	 * address space, then do not unreference the process
	 * till it releases the device memory.
	 */
	if (rv = kdvm_map(procp, chp, map_p, &memloc)) {
		proc_unref(procp);
		return (rv);
	}

	return (0);
}


/*
 * int
 * kdvm_map(void *, ws_channel_t *, struct map_info *, struct kd_memloc *)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in exclusive mode on entry through exit.
 *      - chp must be an active channel.
 *	- return 0 on success, otherwise return appropriate errno.
 */
int
kdvm_map(void *procp, ws_channel_t *chp, struct map_info *map_p,
                struct kd_memloc *memp)
{
        ulong	start, end;
        int	cnt;
	int	err = 0;


        if (memp->length <= 0)
                return (EINVAL);

	/*
	 * Check requested physical range for validity.
	 */
	start = (ulong) memp->physaddr;
	end = (ulong) memp->physaddr + memp->length;
	for (cnt = 0; cnt < kdvmemcnt + 1; cnt++) {
		if ((start >= kd_basevmem[cnt].start) &&
		    (end <= kd_basevmem[cnt].end))
			break;
	}

        if (cnt == kdvmemcnt + 1)
                return (EACCES);

	/*
	 * First free any pages user has in requested range.
	 */

	/* copy the kd_memloc struct */
        map_p->m_addr[map_p->m_cnt] = *memp;

	/*
	 * Release the workstation lock before mapping the video buffer
	 * to user address space since drv_mmap() can block.
	 */
	RW_UNLOCK(Kdws.w_rwlock, plbase);

        /*
         * Map in the virtual memory as a set of device pages.
         */
        err = drv_mmap(0, (paddr_t)memp->physaddr, memp->length, 
		(vaddr_t *)&memp->vaddr, PROT_USER|PROT_READ|PROT_WRITE,
                PROT_ALL, MAP_FIXED);
	if (err) {
		drv_munmap((vaddr_t)memp->vaddr, memp->length);
		/* acquire the lock before returning */
		(void) RW_WRLOCK(Kdws.w_rwlock, plstr);
		return (EIO);
	}

	if (memp->ioflg)
		iobitmapctl(IOB_ENABLE, chp->ch_vstate.v_ioaddrs);

	(void) RW_WRLOCK(Kdws.w_rwlock, plstr);

        if (!map_p->m_cnt) {
		ASSERT(proc_valid(procp));
                map_p->m_procp = procp;
                map_p->m_chan = chp->ch_id;
                chp->ch_flags |= CHN_UMAP;
        }
        map_p->m_cnt++;

        return (0);
}


/*
 * int
 * kdvm_unmapdisp(ws_channel_t *, struct map_info *)
 *
 * Calling/Exit State:
 *	- w_rwlock reader/writer lock is held in exclusive mode.
 *	- return 0 on success, otherwise return appropriate errno.
 *
 * Remarks:
 *	drv_munmap() releases the video buffer mapped by the process.
 *
 *	There must already exists a reference to the process
 *	when KDMAPDISP ioctl was requested.
 *
 *	Channel performing the unmap must be an active channel.
 */
int
kdvm_unmapdisp(ws_channel_t *chp, struct map_info *map_p)
{
        int     cnt;


        if (chp != ws_activechan(&Kdws) || chp->ch_dmode != KD_GRAPHICS)
                return (EACCES);

        chp->ch_vstate.v_font = 0;      /* font munged */

	/*
	 * Release w_rwlock because drv_munmap()/iobitmapctl() can 
	 * block. Set the ipl to plbase.
	 */
	RW_UNLOCK(Kdws.w_rwlock, plbase);

        for (cnt = 0; cnt < map_p->m_cnt; cnt++) {
		drv_munmap((vaddr_t)map_p->m_addr[cnt].vaddr, 
					map_p->m_addr[cnt].length);
		if (map_p->m_addr[cnt].ioflg)
			iobitmapctl(IOB_DISABLE, chp->ch_vstate.v_ioaddrs);
        }

	/*
	 * Reacquire the above released reader/writer lock in 
	 * exclusive mode.
	 */
	(void) RW_WRLOCK(Kdws.w_rwlock, plstr);

	/* unreference the process that was referenced in kdvm_mapdisp() */
	ASSERT(proc_valid(map_p->m_procp));
	proc_unref(map_p->m_procp);
	map_p->m_procp = (void *) 0;
	chp->ch_flags &= ~CHN_MAPPED;
	map_p->m_cnt = 0;
	map_p->m_chan = 0;
	return (0);
}


/*
 * int
 * kdvm_xenixmap(ws_channel_t *, int, int *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *	- On success, return with a valid reference to the current process.
 *
 * Note:
 *	Only active channel is allowed to map physical address to
 *	virtual address.
 */
int
kdvm_xenixmap(ws_channel_t *chp, int cmd, int *rvalp)
{
	struct map_info		*map_p = &Kdws.w_map;
	struct kd_memloc	memloc;
	vidstate_t		*vp = &chp->ch_vstate;
	void			*procp;
	int			rv;


	switch (cmd) {
	case MAPMONO:
		if (!DTYPE(Kdws, KD_MONO) && !DTYPE(Kdws, KD_HERCULES))
			return (EINVAL);
		break;

	case MAPCGA:
		if (!DTYPE(Kdws, KD_CGA))
			return (EINVAL);
		break;

	case MAPEGA:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA))
			return (EINVAL);
		break;

	case MAPVGA:
		if (!DTYPE(Kdws, KD_VGA))
			return (EINVAL);
		break;

	case MAPSPECIAL:
		return (EINVAL);

	default:
		break;
	}

	if (chp != ws_activechan(&Kdws))
		return (EACCES);

	ws_mapavail(chp, map_p);

	procp = proc_ref();
	if (map_p->m_procp && map_p->m_procp != procp || 
				map_p->m_cnt == CH_MAPMX) {
		proc_unref(procp);
		return (EBUSY);
	}

	/*
	 * Release the w_rwlock because map_addr() can
	 * potentially sleep.
	 */
	RW_UNLOCK(Kdws.w_rwlock, plbase);

	/*
	 * Find the physical address and size of screen memory,
	 * and allocate virtual address to map to it.
	 */

	memloc.physaddr = (char *) WSCMODE(vp)->m_base;
	memloc.length = WSCMODE(vp)->m_size;

	(void) map_addr((vaddr_t *)&memloc.vaddr, memloc.length, 0, 0);
	if (!memloc.vaddr) {
		proc_unref(procp);
		(void) RW_WRLOCK(Kdws.w_rwlock, plstr);		/* L005 */
		return (EFAULT);
	}

	memloc.ioflg = 1;

	(void) RW_WRLOCK(Kdws.w_rwlock, plstr);

	/*
	 * If the video buffer is successfully mapped to
	 * user address space, then do not unreference the
	 * process till it releases the device memory.
	 */
	if (rv = kdvm_map(procp, chp, map_p, &memloc)) {
		proc_unref(procp);
		return (rv);
	}

	chp->ch_flags |= CHN_XMAP;
	*rvalp = (int) memloc.vaddr;
	return (0);
}


/*
 * STATIC int
 * kdvm_xenixdoio(ws_channel_t *, int, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC int
kdvm_xenixdoio(ws_channel_t *chp, int cmd, int arg)
{
	struct port_io_arg portio;
	int		indone = 0;
	int		cnt;
	pl_t		opl;
	extern int	ws_ck_kd_port(vidstate_t *, ushort);


	switch (cmd) {
	case MCAIO:
		if (!DTYPE(Kdws, KD_MONO) && !DTYPE(Kdws, KD_HERCULES))
			return(EINVAL);
		break;

	case CGAIO:
		if (!DTYPE(Kdws, KD_CGA))
			return(EINVAL);
		break;

	case EGAIO:
		if (!DTYPE(Kdws, KD_EGA) && !DTYPE(Kdws, KD_VGA))
			return(EINVAL);
		break;

	case VGAIO:
		if (!DTYPE(Kdws, KD_VGA))
			return(EINVAL);
		break;

	case CONSIO:
		break;

	default:
		return (EINVAL);
	}

	if (copyin((caddr_t) arg, &portio, sizeof(struct port_io_arg)) < 0)
		return (EFAULT);

	opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

	for (cnt = 0; cnt < 4 && portio.args[cnt].port; cnt++) {
		if (!ws_ck_kd_port(&chp->ch_vstate, portio.args[cnt].port)) {
			RW_UNLOCK(Kdws.w_rwlock, opl);
			return (EINVAL);
		}

		switch (portio.args[cnt].dir) {
		case IN_ON_PORT:
			portio.args[cnt].data = inb(portio.args[cnt].port);
			indone++;
			break;

		case OUT_ON_PORT:
			outb(portio.args[cnt].port, portio.args[cnt].data);
			break;

		default:
			RW_UNLOCK(Kdws.w_rwlock, opl);
			return (EINVAL);
		}
	}

	RW_UNLOCK(Kdws.w_rwlock, opl);

	if (indone && copyout(&portio, (caddr_t) arg, sizeof(portio)) < 0)
		return (EFAULT);

	return (0);
}
