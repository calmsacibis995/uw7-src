#ident	"@(#)evc.c	1.3"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	26Jan97		rodneyh@sco.com
 *	- Changes for Gemini multiconsole support
 *	  Bracket all io to video adapter with calls ws_con_need_text and
 *	  ws_con_done_text. Note that if the functions in question don't call
 *	  out to any others we don't bother setting and clearing the
 *	  kd_con_acquired flag, but this _MUST_ be done if the functions are
 *	  ever changed to make a call out.
 *
 */

/*
 * evc.c - EVC-1 kd driver support 
 */

/* 
 * Copyright 1989 Ing. C. Olivetti & C. S.p.A.
 * All rights reserved.
 */

/* Created:  5-Jun-89 Mike Slifcak */
/* Revision History:
 *  7-Jun-89 MJS First pass.
 *  8-Jun-89 MJS debugged color map write and read routines.
 *		 removed most macro dependencies.
 * 17-Jun-89 MJS changed evc_init to return qualified mode.
 * 29-Aug-89 MJS minor changes for prototyping.
 */

#ifdef	EVC

#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <mem/immu.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <util/inline.h>
#include <util/cmn_err.h>
#include <io/ws/vt.h>
#include <io/ansi/at_ansi.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/termios.h>
#include <io/strtty.h>
#include <io/stropts.h>
#include <io/xque/xque.h>
#include <io/ws/ws.h>
#include <io/gvid/vid.h>
#include <io/gvid/vdc.h>
#include <io/gvid/evc.h>

#define VGAExtendedIndex	0x3D6
#define	VGAExtendedData		0x3D7
#define	VGAFeatureControl	0x3DA

STATIC int	evc_reset(void);
STATIC void	evc_setup(int);

extern boolean_t kd_con_acquired;			/* L000 */
extern struct vdc_info	Vdc;

struct evc_state {
	int state;
	int montype;
	unsigned int enable;
	unsigned int control;
	unsigned int config;
	unsigned long mem;
};

struct evc_state Evc = { 0 };


/*
 * The extended registers are reset to their initial values
 * when the board is reset (above).  Set the necessary values
 * for all modes first, then the extras for the indicated modes.
 */

static
unsigned short ExtRegsInit[] = {
	0x0604,
	0x6E06,
	0x010B,
	0x0028,
	0x007F,
	0x0000
};

static
unsigned short ExtRegs640x480V[] = {
	0x050B,
	0x0000
};

static
unsigned short ExtRegs1024x768E[] = {
	0x0228,
	0x0000
};

static
unsigned short ExtRegs1024x768D[] = {
	0x0228,
	0x027F,
	0x0000
};


/* 
 * int
 * evc_check(void)
 *
 * Calling/Exit State:
 *	Returns 1 if successful, else returns 0.
 *	The w_rwlock is held in exclusive mode.
 *
 * Description:
 *	Searches EISA slots 1 through 15 for EVC-1 board.
 *	if found, sets up evc info structure.
 */
int
evc_check()
{
	register unsigned int base;

	for (base = 0x1000; base <= 0xF000; base += 0x1000) {
		if (
		(inb(base+OfsEVC1BoardIdReg0) == EVC1Id0) &&
		(inb(base+OfsEVC1BoardIdReg1) == EVC1Id1) &&
		(inb(base+OfsEVC1BoardIdReg2) == EVC1Id2) &&
		(inb(base+OfsEVC1BoardIdReg3) == EVC1Id3)) {
			Evc.state = 2;
			Evc.enable	= base + OfsEVC1BoardEnableReg;
			Evc.control	= base + OfsEVC1BoardControlReg;
			Evc.config	= base + OfsEVC1BoardConfigReg;
			Evc.mem		= EVC_BASE;
			return (1);
		}
	}

	Evc.state = 1;
	return (0);
}


/*
 * STATIC int
 * evc_reset(void)
 *
 * Calling/Exit State:
 *	Returns 1 if successful, else returns 0.
 *
 * Description:
 *	resets the EVC-1 board, reads monitor type. 
 */
STATIC int
evc_reset(void)
{
	register int j, k;
	boolean_t got_con = B_FALSE;		/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0
		else
			got_con = B_TRUE;	/* L000 end */

	if (inb(Evc.enable) & EISA_IOCHKERR) {	/* NOT IMPLEMENTED. I/O error recovery */
		if(got_con)			/* L000 */
			ws_con_done_text();	/* L000 */

		return (0);
	}
	outb(Evc.enable,EISA_STARTRS);
	for (j=0; j < 20; j++ ) {	/* delay AT LEAST 500 nanoseconds */
		k = Evc.enable; Evc.enable++ ; 
		Evc.enable++; 
		Evc.enable = k;
	}
	outb(Evc.enable,EISA_STOPRS);
	outb(Evc.enable,EISA_ENABLE);
	Evc.state = 3;
	outb(Evc.config, EVC1Bus8);
	Evc.montype = (inb(Evc.control) & EVC1MonitorMask) >> 4;

	if(got_con)		/* We got the console here, L000 */
		ws_con_done_text();			/* L000 */

	return (1);
}


/*
 * int
 * evc_init(int)
 *
 * Calling/Exit State:
 *	return input mode if no EVC-1 OR EVC-1 and mode is appropriate.
 *	return DM_VGA_C80x25 if EVC-1 AND hi-res mode attempted 
 *	on low-res monitor.
 *
 *      w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions must be true.
 *              - ch_mutex basic lock is held.
 *              - evc_init is called by the active channel.
 *
 * Description:
 *	finds EVC-1, initializes it, and determines the monitor type.
 *	presets mode qualified by the monitor type.
 */
int 
evc_init(int mode)
{
	int retcode = mode;
	void evc_setup();

	if (!Evc.state) {
		if (!evc_check()) {
			return retcode; /* No EVC present */
		}
	}

	if (!evc_reset())
		return retcode;

	switch (mode) {
	case DM_EVC1024x768E:
	case DM_EVC1024x768D:
		switch (Evc.montype) {
		case MONHiRMono:
		case MONHiRColor:
			evc_setup(mode);
			break;
		default:
			evc_setup(DM_VGA_C80x25);
			retcode = DM_VGA_C80x25;
			break;
		}
		break;

	default:
		evc_setup(mode);
		break;
	}

	return retcode;
}


/*
 * STATIC void
 * evc_setup(int)
 * 
 * Calling/Exit State:
 *
 * Description:
 *	local routine that sets up EVC-1.
 *	mode must be qualified by the monitor type.
 */
STATIC void
evc_setup(int mode)
{
boolean_t got_con = B_FALSE;				/* L000 begin */

	if(!kb_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */

	outb(Evc.config, EVC1Bus8);
	outb(Evc.control,EVC1StartSetup);
	outb(0x102,1);
	outb(0x103,0x80);
	outb(Evc.control,EVC1StopSetup);
	outb(VGAExtendedIndex, 2); outb(VGAExtendedData, 3);
/* XXX */
	outb(Evc.config, EVC1Bus16);
/* XXX */

	if(got_con)					/* L000 */
		ws_con_done_text();			/* L000 */

}


/*
 * void
 * evc_finish(int)
 *
 * Calling/Exit State:
 *      w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions must be true.
 *              - ch_mutex basic lock is held.
 *              - kdv_setall is called by the active channel.
 *
 * Description:
 */
void
evc_finish(int mode)
{
	register unsigned short * pp;
	boolean_t got_con = B_FALSE;			/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L000 end */


	pp = ExtRegsInit;
	while (*pp) { 
		outw(VGAExtendedIndex, *pp); 
		pp++; 
	}

	switch (mode) {
	case DM_EVC640x480V:
		pp = ExtRegs640x480V;
		while (*pp) { 
			outw(VGAExtendedIndex, *pp); 
			pp++; 
		}
		outb(VGAFeatureControl, 1);
		break;

	case DM_EVC1024x768E:
		pp = ExtRegs1024x768E;
		while (*pp) { 
			outw(VGAExtendedIndex, *pp); 
			pp++; 
		}
		outb(VGAFeatureControl, 2);
		break;

	case DM_EVC1024x768D:
		pp = ExtRegs1024x768D;
		while (*pp) { 
			outw(VGAExtendedIndex, *pp); 
			pp++; 
		}
		outb(VGAFeatureControl, 2);
		outb(Evc.control, EVC1DirectHIGH);
		break;

	default:
		outb(VGAFeatureControl, 0);
		break;
	}

	if(got_con)				/* L000 */
		ws_con_done_text();		/* L000 */
}

/*
 * int
 * evc_info(vidstate_t *)
 *
 * Calling/Exit State:
 *	- returns 0 if no EVC, or if EVC reset failed.
 *      - w_rwlock is held in exclusice mode.
 *
 * Description:
 *	fill in Vdc info structure.
 */
int
evc_info(vidstate_t *vp)
{
	static restart;

	if (!Evc.state) {
		if (!evc_check()) {
			return 0;	/* No EVC present */
		}
	}

	if (!restart) {
		if (!evc_reset())
			return 0;
	}

	restart = 1;

	switch (Evc.montype) {
	case MONVGAMono:
		Vdc.v_info.dsply = KD_STAND_M;
		break;
	case MONVGAColor:
		Vdc.v_info.dsply = KD_STAND_C;
		break;
	case MONHiRMono:
		Vdc.v_info.dsply = KD_MULTI_M;
		break;
	case MONHiRColor:
		Vdc.v_info.dsply = KD_MULTI_C;
		break;
	default:
		Vdc.v_info.dsply = KD_UNKNOWN;
		break;
	}
	Vdc.v_type = VEVC;
	Vdc.v_info.cntlr = KD_VGA;
	return 1;
}

#else

/*
 * suppress cc empty translation warning
 */
static char evc_empty_translat_unit;

#endif	/*EVC*/
