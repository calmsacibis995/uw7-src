#ident	"@(#)vdc.c	1.18"
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
 *	L001	12May97		rodneyh@sco.com from stevbam@sco.com
 *	- Delta for Spartacus promotion. Looks like disable_v86bios flag has
 *	  been removed.
 *
 */

#include <io/ansi/at_ansi.h>
#include <io/gvid/vid.h>

#include <io/ansi/at_ansi.h>
#include <io/gvid/vdc.h>
#include <io/gvid/vid.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termios.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <mem/kmem.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <svc/bootinfo.h>

#include <io/ddi.h>
extern boolean_t kd_con_acquired;			/* L000 */

/*
 * Convert segment:offset 8086 far pointer to address
 */
#define vdc_ftop(x)	((((x) & 0xffff0000) >> 12) + ((x) & 0xffff))


STATIC void     vdc_wrnpar2(int);
STATIC void     vdc_wrnpar(int);
STATIC int      vdc_ckidstr(char *, char *, int);
STATIC unchar	vdc_rd750sw(void);

extern void	kdv_adptinit(unchar, vidstate_t *);
extern void	kdv_rst(termstate_t *, vidstate_t *);


long vdcmonitor[] = {
	KD_MULTI_C,
	KD_MULTI_M,
	KD_STAND_C,
	KD_STAND_M,
};

extern struct vdc_info Vdc;
extern int AssumeVDCType;
extern wstation_t Kdws;
extern uint_t kdmontype;

#ifdef EVGA
extern int evga_inited;
#endif	/* EVGA */

extern struct vertim kd_l400[];
extern struct vertim kd_l350[];



/*
 * int
 * vdc_disptype(ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
int
vdc_disptype(ws_channel_t *chp, int arg)
{
	vidstate_t	*vp = &chp->ch_vstate;
	pl_t		pl;


	pl = RW_WRLOCK(chp->ch_wsp->w_rwlock, plstr);

#ifdef	EVC
	/*
	 * Hopefully evc_info() returns 0 if an evga board is present.
	 * If not, then evga won't work.
	 */
	if (evc_info(vp)) ; else
#endif	/* EVC */

	vdc_info(vp);

	RW_UNLOCK(chp->ch_wsp->w_rwlock, pl);

	if (copyout((caddr_t)&Vdc.v_info, (caddr_t)arg, 
				sizeof(struct kd_vdctype))) 
		return (EFAULT);

	return (0);
}


/*
 * void
 * vdc_lktime(int)
 * 
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions are true.
 *		- called by an active channel.
 *		- active channel mutex lock is also held.
 *
 * Description:
 *	Lock/Unlock Super-Vu card horizontal and vertical timing registers.
 */
void
vdc_lktime(int dir)
{
boolean_t got_con = B_FALSE;				/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */

	outb(0x3d4, 0x00);
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3d5, (dir) ? 0xaa : 0x55);

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */

}


/*
 * void
 * vdc_scrambler(int)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions are true.
 *		- called by an active channel.
 *		- active channel mutex lock is also held.
 *
 * Description:
 *	Set/Reset Super-Vu Scrambler logic circuit.
 */
void
vdc_scrambler(int dir)
{

boolean_t got_con = B_FALSE;				/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */

	outb(0x3d4, 0x1f);
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3d5, (dir) ? 0x55 : 0xaa);

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */
}


/*
 * void
 * vdc_lk750(int)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions are true.
 *		- called by an active channel.
 *		- active channel mutex lock is also held.
 *
 * XXX NEW IMPLEMENTED 
 */
void
vdc_lk750(int mode)
{
	int	indx;
	boolean_t got_con = B_FALSE;			/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L000 end */


	vdc_lktime(0);

	if (mode == DM_ATT_640) {	/* lock 400 line mode */
		for (indx = 0; indx < 15; indx++) {
			outb(0x3d4, kd_l400[indx].v_ind);
			outb(0x3d5, kd_l400[indx].v_val);
		}
	} else {			/* lock 350 line mode */
		vdc_scrambler(0);
		for (indx = 0; indx < 5; indx++) {
			outb(0x3d4, kd_l350[indx].v_ind);
			outb(0x3d5, kd_l350[indx].v_val);
		}
		vdc_scrambler(1);
	}

	vdc_lktime(1);

	if(got_con){					/* L000 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L000 end */

}


/*
 * unchar
 * vdc_unlk600(void)
 *
 * Calling/Exit State:
 *      w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions are true.
 *              - called by an active channel.
 *		- active channel mutex lock is also held.
 */
unchar
vdc_unlk600(void)
{
	unchar tmp_pr5;
	boolean_t got_con = B_FALSE;			/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */

	outb(0x3ce, 0xf);
	tmp_pr5 = inb(0x3cf);
	outb(0x3ce, 0xf);
	outb(0x3cf, 0x5);

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */

	return(tmp_pr5);
}


/*
 * void
 * vdc_unlkcas2(void)
 *
 * Calling/Exit State:
 *      w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions are true.
 *              - called by an active channel.
 *		- active channel mutex lock is also held.
 */
void
vdc_unlkcas2(void)
{
 	unchar	tmp_reg;
	boolean_t got_con = B_FALSE;			/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */

	outb(0x3c4, 0x06); outb(0x3c5, 0xec);
	outb(0x3c4, 0xaf); outb(0x3c5, 0x04);
	outb(0x3c4, 0x84); outb(0x3c5, 0x00);
	outb(0x3c4, 0xa7); outb(0x3c5, 0x00);
	outb(0x3c4, 0x91);
	tmp_reg = (inb(0x3c5) & 0x7f);
	outb(0x3c4, 0x91);
	outb(0x3c5, tmp_reg);

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */
}


/*
 * STATIC void
 * vdc_wrnpar2(int)
 *
 * Calling/Exit State:
 *      w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions are true.
 *              - called by an active channel.
 *		- active channel mutex lock is also held.
 *
 * Description:
 *	Write Super-Vu (Paradise NPAR register).
 */
STATIC void
vdc_wrnpar2(int value)
{
boolean_t got_con = B_FALSE;				/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */

	outb(0x3d4, 0x1a);
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3d5, value);

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */
}


/*
 * STATIC void
 * vdc_wrnpar(int)
 *
 * Calling/Exit State:
 *      w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions are true.
 *              - called by an active channel.
 *		- active channel mutex lock is also held.
 *
 * Description:
 *	Write Super-Vu (Paradise Extended Mode Register).
 */
STATIC void
vdc_wrnpar(int value)
{
boolean_t got_con = B_FALSE;				/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3db, value);

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */
}


/*
 * STATIC unchar
 * vdc_rd750sw(void)
 *	Read VDC 750 controller switches.
 *
 * Calling/Exit State:
 *      w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions are true.
 *              - called by an active channel.
 *		- active channel mutex lock is also held.
 *
 * Description:
 *	Read Super-Vu card dip switch positions 5 and 6.
 *	Switch 5 - power up in AT&T mode or EGA mode
 *	Switch 6 - AT&T monitor or EGA monitor attached
 */
STATIC unchar
vdc_rd750sw(void)
{
 	unchar sw = 0;
	boolean_t got_con = B_FALSE;			/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L000 end */

	vdc_wrnpar(0x80);		/* set to EGA mode */
	outb(MISC_OUT, 0x01);
	inb(0x3d8);
	inb(0x3d8);
	if (inb(IN_STAT_0) & SW_SENSE)
		sw = 1;
	outb(MISC_OUT, 0x05);
	inb(0x3d8);
	inb(0x3d8);
	if (inb(IN_STAT_0) & SW_SENSE)
		sw |= 0x02;
	outb(MISC_OUT, 0x01);

	if(got_con){					/* L000 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L000 end */

	return(sw);
}


/*
 * void
 * vdc_750cga(termstate_t *tsp, vidstate_t *vp)
 *
 * Calling/Exit State:
 *      w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions are true.
 *              - called by an active channel.
 *		- active channel mutex lock is also held.
 */
void
vdc_750cga(termstate_t *tsp, vidstate_t *vp)
{
	boolean_t got_con = B_FALSE;			/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L000 end */

	vdc_lktime(1);		/* lock timing registers */
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3c2, 0xa7);
	vdc_wrnpar2(0x94);
	vdc_scrambler(0);	/* disable scrambler logic */
	outb(0x3de, 0x10);
	outb(0x3c2, 0xa7);
	inb(0x3da);
	outb(0x3c0, 0x10);	/* set non-flicker mode */
	outb(0x3c0, 0x00);
	vdc_wrnpar2(0x94);
	vdc_wrnpar(0xc1);	/* set non-ega mode */
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3de, 0x55);	/* unlock AT&T mode2 register */
	inb(0x3d8);
	inb(0x3d8);
	outb(0x3d8, 0x2d);
	outb(0x3de, 0x00);	/* clear Mode 0 bit in mode2 reg */
	vp->v_cmos = MCAP_COLOR;
	kdv_adptinit(MCAP_COLOR, vp);
	vp->v_type = KD_CGA;
	vp->v_cvmode = vp->v_dvmode;
	kdv_rst(tsp, vp);	/* Set display state */

	if(got_con){					/* L000 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L000 end */
}


/*
 * void
 * vdc_check(unchar)
 *
 * Calling/Exit State:
 *      - w_rwlock is held in exclusive mode.
 *
 * Description:
 *	Is color card actually a Rite-Vu card?
 */
void
vdc_check(unchar adptype)
{
	char	*addrp;
	ushort	*addr1p, *addr2p;
	ushort	save1, save2;


	if (adptype == MCAP_EGA) {

		boolean_t got_con = B_FALSE;		/* L000 */

		/* XXX: addrp = (char *)phystokv(V750_IDADDR); */
		addrp = (char *)physmap(V750_IDADDR, 5, KM_NOSLEEP);
		if (addrp == NULL)
			return;

		if(!kd_con_acquired)			/* L000 begin */
			if(ws_con_need_text())
				return;
			else{
				got_con = B_TRUE;
				kd_con_acquired = B_TRUE;
			}				/* L000 end */

		/*
		 * Is EGA type card actually a Super-Vu card?
		 */
		if (AssumeVDCType == 1 || vdc_ckidstr(addrp, "22790", 5)) {
			Vdc.v_type = V750;

			/*
			 * Read sw's 5, 6 on Super-Vu. 
			 */
			if ((Vdc.v_switch = vdc_rd750sw()) & ATTDISPLAY) {
				/*
				 * Use fast clock lock positive sync
				 * polarity alpha double scan enabled.
				 */
				vdc_wrnpar(0x81);
				inb(0x3d8);
				inb(0x3d8);
				outb(0x3c2, 0x80);
				vdc_wrnpar2(0xe0);
			}

			if(got_con){			/* L000 begin*/
				ws_con_done_text();
				kd_con_acquired = B_FALSE;
			}				/* L000 end */

			physmap_free(addrp, 5, 0);
			return;
		}
		physmap_free(addrp, 5, 0);

		/* XXX: addrp = (char *)phystokv(V600_IDADDR); */
		addrp = (char *)physmap(V600_IDADDR, 6, KM_NOSLEEP);
		if (addrp == NULL){
			if(got_con){			/* L000 begin*/
				ws_con_done_text();
				kd_con_acquired = B_FALSE;
			}				/* L000 end */
			return;
		}

		if (AssumeVDCType == 2 || vdc_ckidstr(addrp, "003116", 6)) {
			Vdc.v_type = V600;
			physmap_free(addrp, 6, 0);
			if(got_con){			/* L000 begin*/
				ws_con_done_text();
				kd_con_acquired = B_FALSE;
			}				/* L000 end */
			return;
		}
		physmap_free(addrp, 6, 0);

		/* XXX: addrp = (char *)phystokv(CAS2_IDADDR); */
		addrp = (char *)physmap(CAS2_IDADDR, 6, KM_NOSLEEP);
		if (addrp == NULL){
			if(got_con){			/* L000 begin*/
				ws_con_done_text();
				kd_con_acquired = B_FALSE;
			}				/* L000 end */
			return;
		}

		if (AssumeVDCType == 3 || (vdc_ckidstr(addrp, "C02000", 6)
				&& !(inb(0x78) & 0x8))) {
			Vdc.v_type = CAS2;
			physmap_free(addrp, 6, 0);
			if(got_con){			/* L000 begin*/
				ws_con_done_text();
				kd_con_acquired = B_FALSE;
			}				/* L000 end */
			return;
		}
		physmap_free(addrp, 6, 0);

	} else if (adptype == MCAP_COLOR || adptype == MCAP_COLOR40) {

		boolean_t got_con = B_FALSE;		/* L000 */

		/* XXX: addr1p = (ushort *) phystokv(COLOR_BASE + 0x0001); */
		addr1p = (ushort *) physmap(COLOR_BASE + 0x0001, 
						sizeof(ushort), KM_NOSLEEP);
		/* XXX: addr2p = (ushort *) phystokv(COLOR_BASE + 0x4001); */
		addr2p = (ushort *) physmap(COLOR_BASE + 0x4001, 
						sizeof(ushort), KM_NOSLEEP);
		if (addr1p == NULL || addr2p == NULL)
			return;

		if(!kd_con_acquired)				/* L000 begin */
			if(ws_con_need_text())
				return;
			else{
				got_con = B_TRUE;
				kd_con_acquired = B_TRUE;
			}					/* L000 end */

		save1 = *addr1p;
		save2 = *addr2p;
		*addr2p = 0xa5;		/* Set a word in second page */
		*addr1p = 0;		/* Will overwrite if no second page */

		if (*addr2p != 0xa5) {
                        *addr2p = save2;
			physmap_free((caddr_t)addr1p, sizeof(ushort), 0);
			physmap_free((caddr_t)addr2p, sizeof(ushort), 0);
			if(got_con){			/* L000 begin*/
				ws_con_done_text();
				kd_con_acquired = B_FALSE;
			}				/* L000 end */
                        return;
		}

		/*
                 * Is a Rite-Vu (probably) 
		 */
		*addr1p = save1;	/* restore the value */
		*addr2p = save2;	/* restore the value */

		/*
                 * Have to hard code base register address because the
                 * v_regaddr field isn't set yet.  We know it's a color card.
                 */
                outb(COLOR_REGBASE + STATUS_REG, 0x00);

		drv_usecwait(10);

		if ((inb(COLOR_REGBASE + STATUS_REG) & 0xc0) == 0xc0)
			Vdc.v_type = V400;

		physmap_free((caddr_t)addr1p, sizeof(ushort), 0);
		physmap_free((caddr_t)addr2p, sizeof(ushort), 0);

		if(got_con){				/* L000 begin*/
			ws_con_done_text();
			kd_con_acquired = B_FALSE;
		}					/* L000 end */
	}
}


/*
 * void
 * vdc_600regs(char *tabp)
 *
 * Calling/Exit State:
 *	None.
 */
void
vdc_600regs(char *tabp)
{
	int	indx;
	boolean_t got_con = B_FALSE;			/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */


	for (indx = 0x9; indx < 0xf; indx++, tabp++) {
		outb(0x3ce, indx);
		outb(0x3cf, *tabp);
	}

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */
}


/*
 * int
 * vdc_mon_type(vidstate_t *, unchar)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 */
int
vdc_mon_type(vidstate_t *vp, unchar color)
{
	int	rv = 0;
	int	efl;
	boolean_t got_con = B_FALSE;			/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */

	ASSERT(color == LOAD_COLOR || color == LOAD_MONO);

	efl = intr_disable();

	/* need to start at beginning of retrace */
	while (!(inb(vp->v_regaddr+ STATUS_REG) & S_UPDATEOK))
		;
	while ((inb(vp->v_regaddr + STATUS_REG) & S_UPDATEOK))
		;

	/* load color 0 with a test color to determine the monitor type */
	outb(0x3c6, 0xff);
	outb(0x3c8, 0x00);
	if (color == LOAD_COLOR) {
		/* load dac for color test */
		outb(0x3c9, 0x12);
		outb(0x3c9, 0x12);
		outb(0x3c9, 0x12);
	} else if (color == LOAD_MONO) {
		/* load dac for mono test */
		outb(0x3c9, 0x04);
		outb(0x3c9, 0x10);
		outb(0x3c9, 0x04);
	} else {
		intr_restore(efl);

		if(got_con)					/* L000 */
			ws_con_done_text();			/* L000 */

		return (rv);
	}
		
	/* need to start at beginning of retrace */
	while (!(inb(vp->v_regaddr + STATUS_REG) & S_UPDATEOK))
		;
	while ((inb(vp->v_regaddr + STATUS_REG) & S_UPDATEOK))
		;

	/* Delay to avoid Cascade 2 timing problem. */
	drv_usecwait(10);
	if (inb(0x3c2) & 0x10)
		rv = 1;

	/* reset dac for color 0 to black (0,0,0) */
	outb(0x3c6, 0xff);
	outb(0x3c8, 0x00);
	outb(0x3c9, 0x00);
	outb(0x3c9, 0x00);
	outb(0x3c9, 0x00);

	intr_restore(efl);

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */

	return (rv);
}


/*
 * void
 * vdc_info(vidstate_t *vp)
 * 
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *	- called from kdv_init and to service KDVDCTYPE ioctls.
 */
void
vdc_info(vidstate_t *vp)
{
	/*
	 * if kd has been initialized for EVGA then the 
	 * Vdc information is already established and
	 * immutable, so leave it alone.         
	 */

#ifdef EVGA
    if (!evga_inited ) {
#endif	/* EVGA */

	Vdc.v_info.cntlr = vp->v_type;
	Vdc.v_info.dsply = KD_UNKNOWN;

	switch (Vdc.v_info.cntlr) {
	case KD_CGA:
        case KD_EGA:

		switch (Vdc.v_type) {
		case V400:
		case V750:
                        vdc_scrambler(0);
                        Vdc.v_info.dsply = vdcmonitor[vdc_rdmon(vp->v_dvmode)];

                        switch (Vdc.v_type) {
                        case V400:
                                Vdc.v_info.cntlr = KD_VDC400;
                                break;
                        case V750:
                                Vdc.v_info.cntlr = KD_VDC750;
                                break;
                        }

                        vdc_scrambler(1);
                        break;

		default:
                        break;
		}

                break;

	case KD_VGA: {
		struct vdc_info *pvdc;

		/*
		 * vdc_mon_type() returns non-zero if the DAC test indicates
		 * that a monitor of the requested type was found.
		 */
		if (vdc_mon_type(vp, LOAD_COLOR)) {
			if (VTYPE(V600 | CAS2)) {
				Vdc.v_info.cntlr = KD_VDC600;
				Vdc.v_info.dsply = KD_MULTI_C;
			} else {
				Vdc.v_info.dsply = KD_STAND_C;
			}
		} else if (vdc_mon_type(vp, LOAD_MONO)) {
			Vdc.v_info.dsply = KD_STAND_M;
		} else {
			if (VTYPE(CAS2))
				Vdc.v_info.dsply = KD_STAND_M;
			else
				Vdc.v_info.dsply = KD_MULTI_C;
		}

		/*
		 * Verify that the monitor type detected using the DAC test
		 * matches with the type detected using the BIOS call. If
		 * they do not match, then flag it as a warning.
		 */
		pvdc = (struct vdc_info *) v86bios_vdc_info();
		
		if ((pvdc->v_info.dsply != Vdc.v_info.dsply) &&
		    (!(VTYPE(V600 | CAS2)))) {
			Vdc.v_info.dsply = pvdc->v_info.dsply;
			/* indicate a mismatch in monitor type detection */
			Vdc.v_info.rsrvd = 1;
		}

		physmap_free((caddr_t)pvdc, sizeof(struct vdc_info), 0);
			
		break;
	}

	default:
		break;
	}

#ifdef EVGA
    }   /* endif (!evga_inited ) */
#endif	/* EVGA */

	/*
	 * Overwrite the display type found above with what the
	 * user has set through the resmgr/boot file.
	 */
	if (kdmontype != KD_UNKNOWN) {
		ASSERT(kdmontype == KD_STAND_C || kdmontype == KD_STAND_M);
		Vdc.v_info.dsply = kdmontype;
	}
}


/*
 * STATIC int
 * vdc_ckidstr(char *, char *, int)
 *
 * Calling/Exit State:
 *	- Return 1, if the string stored at addrp is equal
 *	  to the string passed in the argument, otherwise
 *	  return 0.
 */
STATIC int
vdc_ckidstr(char *addrp, char *strp, int cnt)
{
	int	tcnt;
	int	rv = 1;					/* L000 begin */
	boolean_t got_con = B_FALSE;

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else
			got_con = B_TRUE;		/* L000 end */

	for (tcnt = 0; tcnt < cnt; tcnt++) {
		if (*addrp++ != strp[tcnt])
			rv = 0;				/* L000 */
	}

	if(got_con)					/* L000 begin */
		ws_con_done_text();

	return rv;					/* L000 end */
}


/*
 * unchar
 * vdc_rdmon(unchar)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Description:
 *	Read monitor id bits for AT&T display type.
 */
unchar
vdc_rdmon(unchar mode)
{
	unchar	monbits;
	boolean_t got_con = B_FALSE;			/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */


	outb(0x3de, 0x10);
	monbits = (inb(0x3da) >> 4);		/* read id bits 4, 5 */

	if (monbits & 0x02) {			/* non-multimode */
		outb(0x3de, 0x00);		/* reset mode0 */
	} else {				/* multimode */
		if (mode == DM_ATT_640)
			outb(0x3de, 0x00);	/* reset mode0 */
		else
			outb(0x3de, 0x10);	/* set mode0 */
	}

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */

	return (monbits & 0x03);
}


/*
 * void
 * vdc_cas2extregs(vidstate_t *, unchar)
 * 
 * Calling/Exit State:
 *      w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions must be true.
 *              - vdc_cas2extregs() is called by the active channel.
 *		- active channel mutex lock is also held.
 */
/* ARGSUSED */
void
vdc_cas2extregs(vidstate_t *vp, unchar mode)
{
 	unchar	*regp;
	ulong	*tabp;
	int	offset;
	boolean_t got_con = B_FALSE;			/* L000 */

        offset = WSMODE(vp, mode)->m_offset;

	switch (mode) {
	case DM_ATT_640:
	case DM_VDC800x600E:
		tabp = (ulong *) vp->v_parampp - 4;
		break;

	default:
		/* LINTED pointer alignment */
                tabp = (ulong *) ((unchar *)vp->v_parampp - 6);
		break;
	}

	/*
	 * Number of extended video modes is unknown. We assume that the 
	 * maximum number of extended video modes is equivalent to the
	 * the number of regular video modes. In order to cirvumvent the
	 * problem, the size of physical space we map is greater than or
	 * equal to the offset we take into the video parameter table.
	 */

	if(!kd_con_acquired)				/* L000 begin */
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */

	/*
	 * XXX: 
	 *	regp = (unchar *)phystokv(ftop(*tabp)) + (0x1c * offset); 
	 */
	regp = (uchar_t *) physmap(vdc_ftop(*tabp), 
				sizeof(struct b_param) * 31, KM_NOSLEEP);
	if (regp == NULL)
		return;
	regp += (0x1c * offset);

	ASSERT((0x1c * offset + 0x05) <= (sizeof(struct b_param) * 31));
	outb(0x3c4, 0x86); outb(0x3c5, *(regp + 0x05));
	ASSERT((0x1c * offset + 0x17) <= (sizeof(struct b_param) * 31));
	outb(0x3c4, 0xa4); outb(0x3c5, *(regp + 0x17));

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */

	physmap_free((caddr_t)regp, sizeof(struct b_param) * 31, 0);
}
