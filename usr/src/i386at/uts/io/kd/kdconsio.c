#ident	"@(#)kdconsio.c	1.39"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	5Jun97		rodneyh@sco.com from hamilton@dg-rtp.dg.com
 *	- Fix for some keys mysteriously not working if KDB entered early on
 *	  with an kdb.rc file. Uninitialised variable in kdcnopen.
 *
 */
#include <io/ansi/at_ansi.h>
#include <io/conssw.h>
#include <io/cram/cram.h>
#include <io/gvid/vdc.h>
#include <io/gvid/vid.h>
#include <io/kd/kb.h>
#include <io/kd/kd.h>
#include <io/ioctl.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termios.h>
#include <io/ws/8042.h>
#include <io/ws/chan.h>
#include <io/ws/tcl.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <svc/errno.h>
#include <svc/uadmin.h>
#include <util/cglocal.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/kdb/xdebug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>	/* Must come last */


#define KVBASE_MONO	(char *)KVDISP_MONO
#define KVBASE_COLOR	(char *)KVDISP_COLOR


STATIC dev_t	kdcnopen(minor_t, boolean_t, const char *);
STATIC void	kdcnclose(minor_t, boolean_t);
STATIC int	kdcnputc(minor_t, int);
STATIC int	kdcngetc(minor_t);
STATIC void	kdcnsuspend(minor_t);
STATIC void	kdcnresume(minor_t);
STATIC void	kdcnadptinit(unsigned char, vidstate_t *);

void		kdcncleanup(void);

boolean_t	kdsuspended;
boolean_t	kdcnresumed;
boolean_t	kdcninitflag;
boolean_t	kdcnsyscon;
ws_channel_t	*kdswitchedchan = NULL;	/* channel switched from in suspend */
charmap_t	kdcncmap;	/* console character map */
unchar		kdcntabs[ANSI_MAXTAB];
struct conssw	kdconssw = {
	kdcnopen, kdcnclose, kdcnputc, kdcngetc, kdcnsuspend, kdcnresume
};

extern wstation_t Kdws;
extern kdcnops_t Kdwsops;
extern struct vdc_info Vdc;
extern ws_channel_t Kd0chan;
extern int kd_wsinitflag; 

extern struct modeinfo kd_modeinfo[];
extern stridx_t kdstrmap;
extern pfxstate_t kdpfxstr;
extern strmap_t kdstrbuf;
extern keymap_t kdkeymap;
extern extkeys_t ext_keys;
extern esctbl_t kdesctbl;
extern srqtab_t srqtab;
extern struct kb_shiftmkbrk kb_mkbrk[];
extern ushort kb_shifttab[];
extern struct attrmask kb_attrmask[];
extern struct cgareginfo kd_cgaregtab[];
extern esctbl2_t kdesctbl2;			/* L000 */

/*
 * STATIC dev_t
 * kdcnopen(minor_t minor, boolean_t syscon, const char *params)
 *
 * Calling/Exit State:
 *      Assumed to be called very early in the system initialization
 *	process. Sets the video buffer address of the adapter board.
 *
 * Description:
 *	Perform any console initialization needed before printf()
 *	can be called. This includes determining and initializing
 *	the video buffer address fo cnputc().
 */
/* ARGSUSED */
STATIC dev_t
kdcnopen(minor_t minor, boolean_t syscon, const char *params)
{
	vidstate_t	*vp = &Kd0chan.ch_vstate;
	termstate_t	*tsp = &Kd0chan.ch_tstate;
	kbstate_t	*kbp = &Kd0chan.ch_kbstate;
	charmap_t	*cmp = &kdcncmap; 
	uchar_t		adptype;
	int		cnt;
	extern major_t	cmux_major;
	pl_t		pl;


	/* For now, only support minor 0 */
	if (minor != 0)
		return NODEV;

	if (syscon)
		kdcnsyscon = B_TRUE;

	if (kdcninitflag)
		return makedevice(cmux_major, minor);

	pl = splhi();

	vp->v_modesp = kd_modeinfo;
	vp->v_font = 0;
        adptype = (CMOSread(EB) >> 4) & 0x03;
        kdcnadptinit(adptype, vp);
        vp->v_cmos = adptype;
        vp->v_cvmode = vp->v_dvmode;
	if (WSCMODE(vp)->m_color) {
		vp->v_regaddr = COLOR_REGBASE;
		vp->v_rscr = (caddr_t)WSCMODE(vp)->m_base;
		/* LINTED pointer alignment */
		vp->v_scrp = (ushort *)KVBASE_COLOR;
	} else {
		vp->v_regaddr = MONO_REGBASE;
		vp->v_rscr = (caddr_t)WSCMODE(vp)->m_base;
		/* LINTED pointer alignment */
		vp->v_scrp = (ushort *)KVBASE_MONO;
	}

	/*
	 * Initialize terminal state.
	 */

        tsp->t_flags = 0;
        tsp->t_rows = WSCMODE(vp)->m_rows;
        tsp->t_cols = WSCMODE(vp)->m_cols;
        tsp->t_scrsz = tsp->t_rows * tsp->t_cols;
        tsp->t_normattr = NORM;
        tsp->t_origin = 0;
        tsp->t_row = 0;
        tsp->t_col = 0;
        tsp->t_cursor = 0;
        tsp->t_curtyp = 0;
        tsp->t_undstate = 0;
        tsp->t_curattr = tsp->t_normattr;
        tsp->t_font = ANSI_FONT0;
        tsp->t_pstate = 0;
        tsp->t_ppres = 0;
        tsp->t_pcurr = 0;
        tsp->t_pnum = 0;
        tsp->t_ntabs = 9;
	tsp->t_tabsp = kdcntabs;
        for (cnt = 0; cnt < 9; cnt++)
                tsp->t_tabsp[cnt] = cnt * 8 + 8;
	tsp->t_auto_margin = AUTO_MARGIN_ON;
	
	/*
	 * Initialize the charmap tables.
	 */

	Kd0chan.ch_charmap_p = cmp;
        cmp->cr_defltp = cmp;
        cmp->cr_keymap_p = (keymap_t *) &kdkeymap;
        cmp->cr_extkeyp =  (extkeys_t *) ext_keys;
        cmp->cr_esctblp =  (esctbl_t *) kdesctbl;
        cmp->cr_strbufp =  (strmap_t *) kdstrbuf;
        cmp->cr_srqtabp =  (srqtab_t *) srqtab;
        cmp->cr_strmap_p = ws_dflt_strmap_p();
        cmp->cr_pfxstrp = (pfxstate_t *) kdpfxstr;
	cmp->cr_esctbl2p = (esctbl2_t *) &kdesctbl2;	/* L000 */

	/*
	 * Initialize keyboard state.
	 */

	kbp->kb_state = 0;
	inb(KB_IDAT);

	/* clear/reset screen */
	tcl_reset(&Kdwsops, &Kd0chan, tsp);

	/* Set the console init flag. */
	kdcninitflag = B_TRUE; 

	splx(pl);

	return makedevice(cmux_major, minor);
}


/*
 * STATIC void
 * kdcnclose(minor_t minor, boolean_t syscon)
 *
 * Calling/Exit State:
 *	Called with minor and syscon matching a previous call to kdcnopen.
 */
/* ARGSUSED */
STATIC void
kdcnclose(minor_t minor, boolean_t syscon)
{
	if (syscon)
		kdcnsyscon = B_FALSE;
}


/*
 * STATIC int
 * kdcnputc(minor_t, int)
 *	Put a character on console device.
 *
 * Calling/Exit State:
 *	- Return 1 if the device is in graphics mode to
 *	  indicate that the device is busy. Also return 1
 *	  if the character was successfully displayed.
 */
/* ARGSUSED */
STATIC int
kdcnputc(minor_t minor, int ch)
{
	ws_channel_t       *chp = &Kd0chan;
	termstate_t	*tsp;


	if (!kdsuspended && kd_wsinitflag) { 
		if (chp->ch_dmode == KD_GRAPHICS)
			return(1);		/* discard character */
	}

	wsansi_parse(&Kdwsops, chp, (unchar *)&ch, 1);

	return(1);
}


/*
 * STATIC int
 * kdcngetc(minor_t minor)
 *      Get a character from the console.
 *
 * Calling/Exit State:
 *	- Returns ch if it successfully read a character
 *	  from the console, otherwise return -1.
 *
 * TODO:
 *	Quit out of kdb if accidentally get into it.
 */
/* ARGSUSED */
STATIC int
kdcngetc(minor_t minor)
{
	ushort		ch;		/* processed scan code */
	unchar		rawscan, kbrk;	/* raw keyboard scan code */
	ws_channel_t       *chp = &Kd0chan;
	charmap_t       *cmp;
	keymap_t        *kmp;
	kbstate_t       *kbp;
	ushort		okbstate;
 	unchar 		status;

	/*
	 * If no character in keyboard output buffer, return -1
	 */
 	status = inb(KB_STAT);

 	if ((status & KB_OUTBF) == 0)
        	return(-1);

	/* read raw data */
        rawscan = inb(KB_OUT);

	/*
	 * Ignore any data from the auxiliary device (mouse).
	 */
	if (status & KB_AUXOUTBUF)
		return(-1);

        kdkb_force_enable();

	/* get the default console character map and keymap */
        cmp = Kd0chan.ch_charmap_p;
        kmp = cmp->cr_keymap_p;
	kbp = &chp->ch_kbstate;
        kbrk = rawscan & KBD_BREAK;
	okbstate = kbp->kb_state;

        /*
         * Call ws_scanchar() to convert scan code to a character.
         * ws_scanchar() returns a short, with flags in the top byte and the
         * character in the low byte.
         * A legal ascii character will have the top 9 bits off.
         */
        ch = ws_scanchar(cmp, kbp, rawscan, 0);
        (void) ws_shiftkey(ch, (rawscan & ~KBD_BREAK), kmp, kbp, kbrk);

	/*
	 * If a change occurred in the SCROLL LOCK, CAPS LOCK
	 * or NUM LOCK toggle keys, reprogram the LEDs.
	 */
	if (KBTOGLECHANGE(okbstate, kbp->kb_state)) {
		/*
		 * Disable both the keyboard and auxiliary interface, but
		 * enable only the keyboard interface after updating the LEDs.
		 *
		 * Note: <i8042_enable_interface> enables both keyboard and
		 *       auxiliary interface. So we have to explicitly disable
		 *       the auxiliary interface.
		 */
		i8042_disable_interface();
		i8042_update_leds(okbstate, kbp->kb_state);
		i8042_enable_interface();
		i8042_disable_aux_interface();
	}

        if (ch & 0xFF80) {
#ifndef NODEBUGGER
		if (ch == K_DBG)
                        (*cdebugger)(DR_USER, NO_FRAME);
#endif /* !NODEBUGGER */

                return(-1);
        } else {
                return(ch);
        }
}


/*
 * STATIC void
 * kdcnsuspend(minor_t minor)
 *	Suspend normal input processing in preparation for cngetc.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
kdcnsuspend(minor_t minor)
{
	extern ws_channel_t *kdvt_switch2chan0(void);
	uchar_t cb;

	ASSERT(getpl() == plhi);

	kdsuspended = B_TRUE;
	kdswitchedchan = kdvt_switch2chan0();
	/*
	 * Reset the NONTOGGLES keys -- ALT, CTRL, SHIFT, but preserve
	 * the CAPS, NUM and SCROLL keys.
	 */
	Kd0chan.ch_kbstate.kb_state &= (CAPS_LOCK|NUM_LOCK|SCROLL_LOCK|KANA_LOCK);
	i8042_enable_interface();
	i8042_disable_aux_interface();
	/*
	 * Disable keyboard interrupts.
	 */
	i8042_write(KB_ICMD, KB_RCB);
	cb = i8042_read();
	i8042_write(KB_ICMD, KB_WCB);
	i8042_write(KB_IDAT, cb &= ~KB_EOBFI);
}


/*
 * STATIC void
 * kdcncleanup(void)
 *	Cleanup the channel we switched from when entering kdb. Also
 * 	reset NONTOGGLES keys states.
 *
 * Calling/Exit State:
 *	None.
 */
void
kdcncleanup(void)
{
	extern	void kdnotsysrq();

	if (kdswitchedchan != NULL) {
		putnextctl1(kdswitchedchan->ch_qp, M_PCSIG, SIGKILL);
		putnextctl(kdswitchedchan->ch_qp, M_HANGUP);
		kdswitchedchan = NULL;
	}
	/*
	 * Reset the NONTOGGLES keys -- ALT, CTRL, SHIFT, but preserve
	 * the CAPS, NUM and SCROLL keys.
	 */
	Kd0chan.ch_kbstate.kb_state &= (CAPS_LOCK|NUM_LOCK|SCROLL_LOCK);
	kdnotsysrq(&Kd0chan.ch_kbstate, 0);
}


/*
 * STATIC void
 * kdcnresume(minor_t minor)
 *	Resume normal input processing after cngetc.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
kdcnresume(minor_t minor)
{
	uchar_t cb;

	ASSERT(getpl() == plhi);

	kdsuspended = B_FALSE;

	if (!WS_ISINITED(&Kdws)) 
		return;

	/*
	 * Re-enable keyboard interrupts.
	 */
	i8042_write(KB_ICMD, KB_RCB);
	cb = i8042_read();
	i8042_write(KB_ICMD, KB_WCB);
	i8042_write(KB_IDAT, cb |= KB_EOBFI);

	i8042_enable_interface();
	i8042_enable_aux_interface();

	kdcnresumed = B_TRUE;
}

/*
 * STATIC void
 * kdcnadptinit(uchar_t, vidstate_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
kdcnadptinit(uchar_t atype, vidstate_t *vp)
{
	uchar_t	*ega_misc_byte;


        switch (atype) {
        case MCAP_MONO:
                vp->v_dvmode = DM_EGAMONO80x25; /* equiv. EGA mode */
                vp->v_scrmsk = MONO_SCRMASK;
                vp->v_modesel = M_ALPHA80 | M_BLINK | M_ENABLE;
		vp->v_type = KD_MONO;
                break;

        case MCAP_COLOR40:
        case MCAP_COLOR:
                vp->v_dvmode = (atype == MCAP_COLOR) ? DM_C80x25 : DM_C40x25;
                vp->v_scrmsk = COLOR_SCRMASK;
                vp->v_modesel = kd_cgaregtab[vp->v_dvmode].cga_mode;
		vp->v_type = KD_CGA;
                break;

        case MCAP_EGA:
		vp->v_type = KD_EGA;
		ega_misc_byte = (uchar_t *)0x487;
		if (*ega_misc_byte & EGA_MONTYPE)
			vp->v_dvmode = DM_EGAMONO80x25;
		else
			vp->v_dvmode = DM_C80x25;
		vp->v_scrmsk = EGA_SCRMASK;	/* set screen mask */
                break;

	default: 
		break;
        }
}
