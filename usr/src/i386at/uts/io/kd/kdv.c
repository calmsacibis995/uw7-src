#ident	"@(#)kdv.c	1.29"
#ident	"$Header$"

/*
 * Modification history
 *
 *	L000	21Jan97		keithp@sco.com
 *	- KD_DISPTYPE returns different size structure on SCO OSR5 and Gemini
 *	  Fixed by determining if SCO exec. and altering structure to suit.
 *	L001	27Jan97		rodneyh@sco.com
 *	- Changes for multiconsole support. make sure we have acquired the
 *	  console text memory before accessing it.
 *	L002	12May97		rodneyh@sco.com from stevbam@sco.com
 *	- Delta for Spartacus promotion. Looks like disable_v86bios flag has
 *	  been removed.
 *	L003	 9Jun97		jamesh@sco.com
 *	- Change address and data registers used in kdv_rstega(), 
 *	  kdv_setcursor_type() to use correct 3d4/3d5 ports instead of 3d0/3d1.
 *	  Using 3d0/3d1 works on _some_ adapters, but fails dramatically on
 *	  Chips & Technologies 655xx chipsets.
 *	L004	8Oct97		rodneyh@sco.com
 *	- Change kdv_setxenixfont() to not exit without the workstation
 *	  lock held in exclusive mode.
 *	L005	19Oct97		rodneyh@sco.com
 *      - Change kdv_setall to wait for a vertical blank period in between
 *        priming the attribute register flip-flop and programming the
 *        attribute registers. Fixes the horrid red flashes when switching
 *        screens.
 *	  Note: Only seems to solve the problem on about 75% of machines,
 *	  since only slowish (<150 MHz Pentium) machines show the problem
 * 	  we will go with this for Gemini and look for a better fix when
 *	  time allows.
 *	L006	18Dec97		brendank@sco.com
 *	- The test used by kdv_adptinit to distinguish between EGA and
 *	  VGA adapters doesn't work on all VGA adapters.  The test writes
 *	  5 to port 3c4 (sequence data register index), and then reads the
 *	  port.  If the value read back is 5, the adapter is assumed to be
 *	  VGA.  This test seems to rely on this register being read/write
 *	  on VGA adaters, but write only on EGA adatpers.  Presumably
 *	  5 is never read back from an EGA adapter.  Unfortunately, the
 *	  PS/2 technical reference states that the top 5 bits of port
 *	  3c4 are reserved.  On the vast majority of VGA adapters out 
 *	  there, these bits come back as zero, but on a few they don't.
 *	  Such adapters get treated as EGA adapters, and on the adapter
 *	  I was investigating at least, the screen goes blank, presumably
 *	  because EGA support is broken.
 *	  The solution adopted is to mask off the top 5 bits from the 
 *	  value read.  This may result in EGA adapters getting treated
 *	  as VGA, but this probably doesn't matter since EGA has been
 *	  obsolete for some time, and EGA support appears to be broken
 *	  anyway.
 */

/*
 * The set of routines here are the interface to the video
 * controller. All video operations are performed in these
 * routines.
 *
 * The I/O registers are set up as several sets of registers. 
 * Each register set is accessed through two I/O addresses,
 * the first being an address, the second being data. For
 * example, the graphics controller register set consist of 9 
 * registers. To set a value in a register, one sets the address
 * register at 0x3ce to the index of the register of interest,
 * and the data register at 0x3cf to the value desired. To set
 * the graphics controller mode register, set index 5 to a value
 * of 7, the following code is needed:
 *
 *	outb(0x3ce, 5); outb(0x3cf, 7);
 */

#include <io/ansi/at_ansi.h>
#include <io/conf.h>
#include <io/cram/cram.h>
#include <io/gvid/vdc.h>
#include <io/gvid/vid.h>
#include <io/kd/kd.h>
#include <io/stream.h>
#include <io/strtty.h>
#include <io/stropts.h>
#include <io/termio.h>
#ifndef NO_MULTI_BYTE
#include <io/ws/mb.h>
#endif /* NO_MULTI_BYTE */
#include <io/ws/tcl.h>
#include <io/ws/vt.h>
#include <io/ws/ws.h>
#include <io/xque/xque.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>
#include <proc/exec.h>					/* L000 */
#include <proc/proc.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <svc/bootinfo.h>
#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <util/inline.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <util/debug.h>

#include <svc/sco.h>					/* L000 */
#include <io/ddi.h>

/* For debugging purposes. */
#undef STATIC
#define	STATIC

/*
 * Convert segment:offset 8086 far pointer to address
 */
#define kdv_ftop(x)         ((((x) & 0xffff0000) >> 12) + ((x) & 0xffff))

/*
 * Wait for a vertical sync. This is necessary because some graphics
 * adapters (such as the CGA) may produce snow if the CPU writes to
 * adapter memory during the display interval due to conficts between
 * CPU and adapter memory access.
 */
#define WAIT_VSYNC(vp, state) { \
	int poll = 7000; \
	do { \
		drv_usecwait(10); 	/* 10 micro sec delay? */ \
		if ((inb((vp)->v_regaddr + STATUS_REG) & S_VSYNC) == (state)) \
			break; \
	} while (poll-- > 0); \
}


void		kdv_setdisp(ws_channel_t *, vidstate_t *, termstate_t *, unchar);
void		kdv_adptinit(unchar, vidstate_t *);
void		kdv_rst(termstate_t *, vidstate_t *);
int		kdv_release_fontmap(void);
void		kdv_enable(struct vidstate *);
void		kdv_scrxfer(ws_channel_t *, int);

STATIC void	kdv_disp(int);
STATIC void     kdv_setregs(char *, int);
STATIC void     kdv_rstega(termstate_t *, vidstate_t *);
STATIC void     kdv_setall(vidstate_t *, unchar);
STATIC void     kdv_setmode0(unchar);
STATIC void     kdv_ldramdac(vidstate_t *);
STATIC void     kdv_clrgraph(vidstate_t *, unchar);
STATIC ushort	kdv_mem(int, ushort *, ushort);
STATIC void     kdv_xfer(ushort *, ushort *, int, char);
STATIC int      kdv_ckherc(vidstate_t *);
STATIC int	kdv_rdsw(void);
STATIC void     kdv_params(void);
STATIC void     kdv_ldfont(vidstate_t *, unchar);
STATIC struct b_param  *kdv_getparam(unchar);
STATIC void	kdv_physmap_init(void);

extern int	kdsetcursor(ws_channel_t *, termstate_t *);
extern int	kdclrscr(ws_channel_t *, ushort, int);

#ifdef EVGA
extern void	evga_ext_rest(unchar);
extern void	evga_ext_init(unchar);
#endif /* EVGA */

extern void	vcopy(ushort_t *, ushort_t *, size_t, int);

uchar_t *egafont_p[ROM_FONT_TYPES];
caddr_t kd_romfont_mod = 0;
caddr_t chgen_vaddr;

extern boolean_t kd_con_acquired;			/* L001 */
extern wstation_t Kdws;
extern struct vdc_info Vdc;

extern struct font_info	fontinfo[];
extern struct font_info kd_romfonts[];
extern struct modeinfo	kd_modeinfo[];
extern ushort kd_iotab[][MKDBASEIO];
extern struct b_param kd_inittab[];
extern struct reginfo kd_regtab[];
extern unchar kd_ramdactab[];
extern ulong_t egafontptr[5];
extern ushort_t egafontsz[5];
extern struct cgareginfo kd_cgaregtab[];
extern struct m6845init kd_cgainittab[];
extern struct m6845init kd_monoinittab[];
extern long kdmonitor[];
extern int kd_font_offset[];
extern char kd_swmode[];
extern unchar kd_adtconv[];

extern int kdioaddrcnt;
extern int kdvmemcnt;
extern struct kd_range kd_basevmem[], kdvmemtab[];
extern ushort kdconfiotab[];

#ifdef EVGA
extern struct at_disp_info disp_info[];
extern int evga_inited;
extern int cur_mode_is_evga;
extern unchar saved_misc_out;
#endif /* EVGA */

#ifndef NO_MULTI_BYTE
/*
 * We put the gs_init_flg in the kd module; otherwise we would require
 * a stubs file if the gsd module would not be configured.
 */
int gs_init_flg = 0;	/* graphics system driver has been initialized */
int fnt_init_flg = 0;	/* font driver has been initialized */
int channel_ref_count = 0; /* number of active graphical channels */
gsfntops_t Gs;		/* filled in by the font driver */
#endif /* NO_MULTI_BYTE */

#if defined(DEBUG) || defined(DEBUG_TOOLS)
#define	KDV_GET_BIOS_PARAMS	0x01
#define	KDV_SET_BIOS_PARAMS	0x02

STATIC void kdv_getregs(vidstate_t *, char *, int);
STATIC void kdv_getall(vidstate_t *);
STATIC struct b_param kd_reginittab;
STATIC int kdv_bios;
#endif /* DEBUG || DEBUG_TOOLS */

/*
 * void
 * kdv_init(ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 * 
 * Description:
 *	Initialize the vidstate_t structure of Kdws.
 *	It is the default state for each channel as
 *	it is created. 
 */
void
kdv_init(ws_channel_t *chp)
{
	unchar		adptype;
	int		indx, cnt;
	vidstate_t	*vp = &Kdws.w_vstate;
	termstate_t	*tsp = &chp->ch_tstate;
	int		i;


	/*
	 * Map in the ROM fonts locations we got from the BIOS and store
	 * it into the drivers font pointers.
	 */
	v86bios_get_cdt();

	for (i = 0; i < ROM_FONT_TYPES; i++) {
		egafont_p[i] = (uchar_t *) physmap(kdv_ftop(egafontptr[i]), 
				(egafontsz[i] * MAX_ROM_CHAR), KM_NOSLEEP);
		if (!egafont_p[i])
			/*
			 *+ Unable to physmap the ROM fonts. Check memory
			 *+ configured in your system.
			 */
			cmn_err(CE_PANIC,
				"kdv_init: unable to physmap ROM font");
	}

	/*
	 * The font information (pointers to the font tables
	 * in the BIOS RAM of the video controller) is set
	 * up in the font_info structure and copied into the 
	 * variable kd_romfonts, which is used as a placeholder
	 * for the pointers to the ROM fonts in the event that
	 * the default fonts for the workstation are changed.
	 */

	Kdws.w_vstate.v_nfonts = ROM_FONT_TYPES; /* number of VGA fonts */
	Kdws.w_vstate.v_fontp = fontinfo;

	/*
         * fontinfo[FONT8x8].f_fontp = egafont_p[F8x8_INDX];
         * fontinfo[FONT8x14].f_fontp = egafont_p[F8x14_INDX];
         * fontinfo[FONT8x14m].f_fontp = egafont_p[F9x14_INDX];
         * fontinfo[FONT8x16].f_fontp = egafont_p[F8x16_INDX];
         * fontinfo[FONT9x16].f_fontp = egafont_p[F9x16_INDX];
	 * kd_romfonts[FONT8x8] = fontinfo[FONT8x8];
	 * kd_romfonts[FONT8x14] = fontinfo[FONT8x14];
	 * kd_romfonts[FONT8x14m] = fontinfo[FONT8x14m];
	 * kd_romfonts[FONT8x16] = fontinfo[FONT8x16];
	 * kd_romfonts[FONT9x16] = fontinfo[FONT9x16];
	 */
	for (i = 1; i <= ROM_FONT_TYPES; i++) {
		fontinfo[i].f_fontp = egafont_p[i - 1];
		kd_romfonts[i] = fontinfo[i];		/* structure copy */
	}

	vp->v_modesp = kd_modeinfo;
	vp->v_font = 0;
	adptype = (CMOSread(EB) >> 4) & 0x03;	/* get equipment byte */
	kdv_adptinit(adptype, vp);
	vp->v_cmos = adptype;

	/*
	 * Initialize the virtual mapping for the various physical
	 * mappings like character generator, MDA, CGA, EGA and VGA.
	 */
	kdv_physmap_init();
	vp->v_cvmode = vp->v_dvmode;	/* v_dvmode set by kdv_adptinit */

	switch (adptype) {
	case MCAP_MONO:
		if (kdv_ckherc(vp)) {
			vp->v_type = KD_HERCULES;
			indx = KD_HERCINDX;
		} else {
			vp->v_type = KD_MONO;
			indx = KD_MONOINDX;
		}
		for (cnt = 0; cnt < MKDBASEIO; cnt++)
			vp->v_ioaddrs[cnt] = kd_iotab[indx][cnt];
		break;

	case MCAP_COLOR40:
		vp->v_cmos = MCAP_COLOR;
		/* FALLTHROUGH */

	case MCAP_COLOR:
		vp->v_type = KD_CGA;
		for (cnt = 0; cnt < MKDBASEIO; cnt++)
			vp->v_ioaddrs[cnt] = kd_iotab[KD_COLRINDX][cnt];
		break;

	case MCAP_EGA:
		kdv_params();
		for (cnt = 0; cnt < MKDBASEIO; cnt++)
			vp->v_ioaddrs[cnt] = kd_iotab[KD_EGAINDX][cnt];
		break;

	default:
		break;
	}

#if defined(DEBUG) || defined(DEBUG_TOOLS)
	if (kdv_bios & KDV_GET_BIOS_PARAMS)
		kdv_getall(vp);
#endif /* DEBUG || DEBUG_TOOLS */

	kdv_rst(tsp, vp);
	kdv_enable(vp);

	if (DTYPE(Kdws, KD_VGA)) {
#ifdef EVC
           	if (evc_info(vp)) ; else
#endif /* EVC */
		vdc_info(vp);
		if (Vdc.v_info.dsply == KD_MULTI_M || 
		    Vdc.v_info.dsply == KD_STAND_M) {
			vp->v_cvmode = vp->v_dvmode = DM_VGAMONO80x25;
			kdv_rst(tsp, vp);
			kdv_enable(vp);
		}
	}

	/*
	 * Set up legal I/O port and video memory addresses. 
	 */
	if (kdioaddrcnt > 0 && kdioaddrcnt <= MKDCONFADDR) {
		for (cnt = 0; cnt < MKDCONFADDR; cnt++) {
			if (vp->v_ioaddrs[cnt] == 0) {
				bcopy(kdconfiotab, &vp->v_ioaddrs[cnt], 
					(sizeof(ushort) * kdioaddrcnt));
				break;
			}
		}
	}

#ifndef EVC
	if (kdvmemcnt > 0 && kdvmemcnt <= MKDCONFADDR)
		bcopy(kdvmemtab, &kd_basevmem[1], 
				(sizeof(struct kd_range) * kdvmemcnt));
#endif /* EVC */
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * STATIC void
 * kdv_getall(vidstate_t *)
 *	Get the video controller register values.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
kdv_getall(vidstate_t *vp)
{
boolean_t got_con = B_FALSE;				/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			kd_con_acquired = B_TRUE;
			got_con = B_TRUE;
		}					/* L001 end */

	kdv_getregs(vp, (char *)kd_reginittab.seqtab, I_SEQ);
	if (DTYPE(Kdws, KD_VGA))
		kd_reginittab.miscreg = inb(MISC_OUT_READ);
	ASSERT(vp->v_cvmode);
	if (WSMODE(vp, vp->v_cvmode)->m_color)
		kdv_getregs(vp, (char *)&kd_reginittab.egatab, I_EGACOLOR);
	else
		kdv_getregs(vp, (char *)&kd_reginittab.egatab, I_EGAMONO);
	kdv_getregs(vp, (char *)kd_reginittab.attrtab, I_ATTR);
	kdv_getregs(vp, (char *)kd_reginittab.graphtab, I_GRAPH);

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

}

#endif /* DEBUG || DEBUG_TOOLS */


/*
 * STATIC void
 * kdv_wait_vsync(vidstate_t *, uchar_t)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Wait for a vertical sync. This is necessary because some graphics
 *	adapters (such as the CGA) may produce snow if the CPU writes to
 *	adapter memory during the display interval due to conficts between
 *	CPU and adapter memory access.
 */
STATIC void
kdv_wait_vsync(vidstate_t *vp, uchar_t state)
{
	int	poll = 7000;
	boolean_t got_con = B_FALSE;			/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L001 end */

	do { 
		drv_usecwait(10); 	/* 10 micro sec delay? */ 
		if ((inb(vp->v_regaddr + IN_STAT_1) & S_VSYNC) == state) 
			break;
	} while (poll-- > 0);

	if(got_con)					/* L001 */
		ws_con_done_text();			/* L001 */

}


/*
 * STATIC void 
 * kdv_physmap_init(void)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.	
 */
STATIC void
kdv_physmap_init(void)
{
	caddr_t mono_vaddr;
	caddr_t color_vaddr;
	caddr_t ega_vaddr;
	caddr_t egal_vaddr;
#ifdef EVC
	caddr_t	evc_vaddr;
	caddr_t	evch_vaddr;
#endif /* EVC */
	int	i;
	extern int kd_num_vmodes;	/* number of video modes */


	mono_vaddr  = (caddr_t)physmap(MONO_BASE, MONO_SIZE, KM_NOSLEEP);
	color_vaddr = (caddr_t)physmap(COLOR_BASE, COLOR_SIZE, KM_NOSLEEP);
	ega_vaddr   = (caddr_t)physmap(EGA_BASE, EGA_SIZE, KM_NOSLEEP);
	egal_vaddr  = (caddr_t)physmap(EGA_BASE, EGA_LGSIZE, KM_NOSLEEP);
	chgen_vaddr = (caddr_t)physmap(CHGEN_BASE, CHGEN_SIZE, KM_NOSLEEP);
	if ((ega_vaddr == (caddr_t)NULL) || (egal_vaddr == (caddr_t)NULL) ||
	    (chgen_vaddr == (caddr_t)NULL) || (mono_vaddr == (caddr_t)NULL) ||
	    (color_vaddr == (caddr_t)NULL)) {
		/*
		 *+ Unable to allocate virtual mapping for the various
		 *+ physical mappings like character generator, MDA, CGA, 
		 *+ EGA and EGAL.
		 */
		cmn_err(CE_PANIC, 
			"kdv_physmap_init: no virtual memory available");
	}

#ifdef EVC
	evc_vaddr   = (caddr_t)physmap(EVC_BASE, EVC_SIZE, KM_NOSLEEP);
	evch_vaddr  = (caddr_t)physmap(EGA_BASE, EVC_HGSIZE, KM_NOSLEEP);
	if ((evc_vaddr == (caddr_t)NULL) || (evch_vaddr == (caddr_t)NULL)) {
		/*
		 *+ Unable to allocate virtual mapping for the various
		 *+ physical mappings like character generator, MDA, CGA, 
		 *+ EGA and EGAL.
		 */
		cmn_err(CE_PANIC, 
			"kdv_physmap_init: no virtual memory available");
	}
#endif /* EVC */

	/*
	 * Initialize the virtual address for the base address for
	 * all the modes in kd_modeinfo table.
	 */

	for (i = 0; i < kd_num_vmodes; i++) {

		switch (kd_modeinfo[i].m_base) {
		case MONO_BASE:
			if (kd_modeinfo[i].m_size == MONO_SIZE)
				kd_modeinfo[i].m_vaddr = mono_vaddr;
			break;

		case COLOR_BASE:
			if (kd_modeinfo[i].m_size == COLOR_SIZE) 
				kd_modeinfo[i].m_vaddr = color_vaddr;
			break;	
		
		case EGA_BASE:
			if (kd_modeinfo[i].m_size == EGA_SIZE) {
				kd_modeinfo[i].m_vaddr = ega_vaddr;
				break;
			}

			if (kd_modeinfo[i].m_size == EGA_LGSIZE) {
				kd_modeinfo[i].m_vaddr = egal_vaddr;
				break;
			}

#ifdef EVC
			if (kd_modeinfo[i].m_size == EVC_HGSIZE) {
				kd_modeinfo[i].m_vaddr = egah_vaddr;
				break;
			}
#endif /* EVC */
			break;

#ifdef EVC
		case EVC_BASE:
			if (kd_modeinfo[i].m_size == EVC_SIZE)
				kd_modeinfo[i].m_vaddr = evc_vaddr;

			break;
#endif /* EVC */

		default:
			/*
			 * Check if the base address of the video mode 
			 * entry in the table is 0 (placeholder).
			 */
			if (kd_modeinfo[i].m_base == 0x0)
				continue;
			else
				/*
				 *+ An unknown display base address 
				 *+ for the video mode for which there 
				 *+ does not exist a virtual mapping.
				 */
				cmn_err(CE_WARN,
					"!kdv_physmap_init: Unknown base "
					"address for the video mode:\n "
					"mode=0x%x m_base=0x%x m_size=0x%x", 
					i, kd_modeinfo[i].m_base, 
					kd_modeinfo[i].m_size);
			break;
		}
	}
}


/*
 * void
 * kdv_adptinit(unchar, vidstate_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode. 
 */
void
kdv_adptinit(unchar atype, vidstate_t *vp)
{
	boolean_t got_con = B_FALSE;	/* L001 */

	switch (atype) {

	case MCAP_MONO:
		vp->v_dvmode = DM_EGAMONO80x25;	/* equiv. EGA mode */
		vp->v_scrmsk = MONO_SCRMASK;
		vp->v_modesel = M_ALPHA80 | M_BLINK | M_ENABLE;
		break;

	case MCAP_COLOR40:
	case MCAP_COLOR:
		vp->v_dvmode = (atype == MCAP_COLOR) ? DM_C80x25 : DM_C40x25;
		vp->v_scrmsk = COLOR_SCRMASK;
		vp->v_modesel = kd_cgaregtab[vp->v_dvmode].cga_mode;
		if (!VTYPE(V750))
                        vdc_check(atype);	/* CGA or AT&T Rite-Vu	? */
		if (VTYPE(V400 | V750))		/* Is AT&T Adapter ? */
                        Vdc.v_mode2sel = M_UNDRLINE;
		break;

	case MCAP_EGA:
#ifdef EVC
           	if (evc_info(vp)) {		/* EVC-1 is superset of VGA */
                        vp->v_dvmode = DM_VGA_C80x25;
                        vp->v_type = KD_VGA;
                        vp->v_scrmsk = EGA_SCRMASK;
                        break;
		}
#endif /* EVC */

		vdc_check(atype);
		
		if(!kd_con_acquired)			/* L001 begin */
			if(ws_con_need_text())
				return;
			else{
				got_con = B_TRUE;
				kd_con_acquired = B_TRUE;
			}				/* L001 end */

		outb(0x3c4, 0x5);			/* test for VGA */
		if ((inb(0x3c4) & 0x7) != 0x5) {	/* L006 */
			vp->v_dvmode = kdv_rdsw();	/* EGA */
			vp->v_type = KD_EGA;
		} else {
			vp->v_dvmode = DM_VGA_C80x25;
			vp->v_type = KD_VGA;
			if (VTYPE(V600))
				WSMODE(vp, DM_ATT_640)->m_offset = 4;
			else if (VTYPE(CAS2)) {
                                WSMODE(vp, DM_ATT_640)->m_params = KD_CAS2;
				WSMODE(vp, DM_ATT_640)->m_offset = 1;
                                WSMODE(vp, DM_VDC800x600E)->m_params = KD_CAS2;
				WSMODE(vp, DM_VDC800x600E)->m_offset = 9;
			}
		}

		if(got_con){				/* L001 begin */
			ws_con_done_text();
			kd_con_acquired = B_FALSE;
		}					/* L001 end */

		vp->v_scrmsk = EGA_SCRMASK;		/* set screen mask */

		if (VTYPE(V750)) {
			if (!VSWITCH(ATTCGAMODE)) 	/* set CGA mode */
				vp->v_dvmode = DM_ENH_CGA;
			else if (VSWITCH(ATTDISPLAY) && 
					vp->v_dvmode == DM_EGAMONO80x25)
				vp->v_dvmode = DM_B80x25;
		}

		break;
	} /* end switch */
}


/*
 * void
 * kdv_setmode(ws_channel_t *, unchar)
 *	Switch the current video mode of a VT.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 */
void
kdv_setmode(ws_channel_t *chp, unchar newmode)
{
	vidstate_t	*vp = &chp->ch_vstate;
	termstate_t	*tsp = &chp->ch_tstate;
	pl_t		opl;


	if (vp->v_cvmode == newmode)
		return;

	ASSERT(getpl() == plstr);

	/*
	 * Wait until a video mode switch operation or channel switch
	 * operation is complete.
	 *
	 * Note: the flag is set when a VT switch operation is initiated
	 * by the process.
	 */
	(void) LOCK(Kdws.w_mutex, plstr);
	while (Kdws.w_flags & (WS_NOMODESW|WS_NOCHANSW)) {
		RW_UNLOCK(Kdws.w_rwlock, plstr);
		/* XXX: In SVR4.2 the sleep priority == PZERO */ 
		SV_WAIT(Kdws.w_flagsv, (primed + 1), Kdws.w_mutex);
		(void) RW_WRLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(Kdws.w_mutex, plstr);
	}
	Kdws.w_flags |= WS_NOCHANSW|WS_NOMODESW;
	UNLOCK(Kdws.w_mutex, plstr);

#ifdef EVGA
	evga_ext_rest(vp->v_cvmode);
#endif /* EVGA */

	/*
	 * Program the video for the new mode.
	 * It sets the vp->v_cvmode.
	 */		
	kdv_setdisp(chp, vp, tsp, newmode);

#ifdef EVGA
	evga_ext_init(vp->v_cvmode);
#endif /* EVGA */

	if (WSCMODE(vp)->m_color) {	/* color mode? */
		vp->v_undattr = NORM;             
	}

	/*
	 * Reset/clear the screen if the new mode is
	 * a text mode.
	 */
	if (WSCMODE(vp)->m_font) 	/* text mode? */
		tcl_reset(Kdws.w_consops, chp, tsp);

#ifndef NO_MULTI_BYTE
	if (gs_init_flg && fnt_init_flg) {
		if (chp->ch_dmode == KD_GRTEXT)
			Kdws.w_consops->cn_gs_alloc(Kdws.w_consops, chp, tsp);
		else
			Kdws.w_consops->cn_gs_free(Kdws.w_consops, chp, tsp);
	}
#endif /* NO_MULTI_BYTE */

	opl = LOCK(Kdws.w_mutex, plstr);
	Kdws.w_flags &= ~(WS_NOCHANSW|WS_NOMODESW);
	SV_SIGNAL(Kdws.w_flagsv, 0);
	UNLOCK(Kdws.w_mutex, opl);
	return;
}


/*
 * int 
 * kdv_cursortype(wstation_t *, ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *	- Only active channel is allowed to change the cursortype.
 *	- w_rwlock is held in exclusive/shared mode.
 *	- chp->ch_mutex lock is also held if w_rwlock is held in shared mode.
 *
 * Description:
 *	This routine is called when switching the VTs,
 *	changing to a text mode, or processing the TCL
 *	command to change the cursor type. It programs 
 *	the video controller to display the cursor as a
 *	block, underscore, or blank (no cursor) based on
 *	channel's termstate_t cursor type value. The 
 *	size/position of the cursor is completely under
 *	the control of the software; the characterstics
 *	of the block and underscore cursors may be 
 *	changed by changing this routine.
 */
/* ARGSUSED */
int
kdv_cursortype(wstation_t *wsp, ws_channel_t *chp, termstate_t *tsp)
{
	ushort	cur_end, new_val;
	ushort	reg_base = 0x3d0;
	boolean_t got_con = B_FALSE;		/* L001 */


	if (chp != ws_activechan(&Kdws))
		return (0);

	if (DTYPE(Kdws, KD_EGA)) {
		if (VTYPE(V750))
			vdc_lktime(0);

		switch (chp->ch_vstate.v_font) {
		case FONT8x8:
			cur_end = 0x7;
		   	if (tsp->t_curtyp == 0) 
				new_val = 0x7;
		   	else if (tsp->t_curtyp == 1) 
				new_val = 0x00;
			else {
				cur_end = 0;
				new_val = 0xe;
			}
		   	break;

		case FONT8x16:
		case FONT9x16:
			cur_end = 0xd;
	   		if (tsp->t_curtyp == 0) 
				new_val = 0xd;
	   		else if (tsp->t_curtyp == 1) 
				new_val = 0x00;
			else {
				cur_end = 0;
				new_val = 0xe;
			}
	   		break;

		case FONT8x14:
		case FONT8x14m:
			cur_end = 0xd;
		   	if (tsp->t_curtyp == 0) 
				new_val = 0xd;
		   	else if (tsp->t_curtyp == 1) 
				new_val = 0x00;
			else {
				cur_end = 0;
				new_val = 0xe;
			}
		   	break;

		default:
	   		return (0);
		}

		if(!kd_con_acquired)			/* L001 begin */
			if(ws_con_need_text())
				return 0;
			else
				got_con = B_TRUE;	/* L001 end */

		/*
		 * 0x0b is the index into register set that
		 * corressponds to the cursor end location.
		 */
		outb(reg_base + 4, 0x0b);
		outb(reg_base + 5, cur_end);

		/*
		 * 0x0a is the index into register set that
		 * corressponds to the cursor start location.
		 */
		outb(reg_base + 4, 0x0a);
		outb(reg_base + 5, new_val);

		if(got_con)				/* L001 */
			ws_con_done_text();		/* L001 */


		if (VTYPE(V750))
			vdc_lktime(1);
		return (0);
	} 

	if (DTYPE(Kdws, KD_VGA)) {

		if(!kd_con_acquired)			/* L001 begin */
			if(ws_con_need_text())
				return 0;
			else
				got_con = B_TRUE;	/* L001 end */


		if (!(inb(0x3cc) & 0x01)) 
			reg_base = 0x3b0;
		outb(reg_base + 4, 0x0b);
		cur_end = inb(reg_base + 5);
		if (tsp->t_curtyp == 0)  
			new_val = cur_end - 1;
		else if (tsp->t_curtyp == 1)  
			new_val = 0x00;
		else
			new_val = cur_end + 1;
		outb(reg_base + 4, 0x0a);
		outb(reg_base + 5, new_val);

		if(got_con)				/* L001 */
			ws_con_done_text();		/* L001 */

		return (0);
	}

 	if (DTYPE(Kdws, KD_CGA) || DTYPE(Kdws, KD_MONO)) {
		switch (chp->ch_vstate.v_font) {
		case FONT8x8:
		case FONT8x16:
		case FONT9x16:
			cur_end = 0x7;
			if (tsp->t_curtyp == 0) 
				new_val = 0x6;
		   	else if (tsp->t_curtyp == 1) 
				new_val = 0x00;
			else {
				cur_end = 0x20;
				new_val = 0x20;
			}
		   	break;

		case FONT8x14:
		case FONT8x14m:
			cur_end = 0x6;
	   		if (tsp->t_curtyp == 0) 
				new_val = 0x5;
	   		else if (tsp->t_curtyp == 1) 
				new_val = 0x00;
			else {
				cur_end = 0x20;
				new_val = 0x20;
			}
	   		break;

		default:
			return (0);
		}

		if(!kd_con_acquired)			/* L001 begin */
			if(ws_con_need_text())
				return 0;
			else
				got_con = B_TRUE;	/* L001 end */

		outb(reg_base + 4, 0x0b);
		outb(reg_base + 5, cur_end);
		outb(reg_base + 4, 0x0a);
		outb(reg_base + 5, new_val);

		if(got_con)				/* L001 */
			ws_con_done_text();		/* L001 */
	}

	return (0);
}
		

/*
 * int
 * kdv_notromfont(vidstate_t *vp)
 *
 * Calling/Exit State:
 *	- If romfont is modified, return 1 else return 0.
 */
int
kdv_notromfont(vidstate_t *vp)
{
	if (kd_romfont_mod)
		return (1);
	if (vp->v_fontp[FONT8x8].f_fontp != kd_romfonts[FONT8x8].f_fontp)
		return (1);
	if (vp->v_fontp[FONT8x14].f_fontp != kd_romfonts[FONT8x14].f_fontp)
		return (1);
	if (vp->v_fontp[FONT8x16].f_fontp != kd_romfonts[FONT8x16].f_fontp)
		return (1);
	return (0);
}
	

/*
 * void
 * kdv_setdisp(ws_channel_t *, vidstate_t *, termstate_t *, unchar)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	- ch_mutex basic lock is also held, when w_rwlock 
 *	  is in shared mode.
 *	- Only the active channel can program the video
 *	  to a new mode.
 *
 * Description:
 *	This routine programs the video for the new video
 *	mode. It also updates the termstate_t structure
 *	(i.e. number of rows, columns) for the channel
 *	if the new mode is a text mode. It calls kdv_rst()
 *	to program the hardware and kdv_enable() to 
 *	reenable scanning of the video controller.
 */
void
kdv_setdisp(ws_channel_t *chp, vidstate_t *vp, termstate_t *tsp, unchar newmode)
{
	int	newfont;
	boolean_t got_con = B_FALSE;		/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			kd_con_acquired = B_TRUE;
			got_con = B_TRUE;
		}				/* L001 end */
	
	vp->v_cvmode = newmode;

	newfont = WSCMODE(vp)->m_font;
	if (newfont) {				/* text mode? */
		chp->ch_dmode = KD_TEXT0;
		tsp->t_rows = WSCMODE(vp)->m_rows;
		tsp->t_cols = WSCMODE(vp)->m_cols;
		tsp->t_scrsz = tsp->t_cols * tsp->t_rows;
		vp->v_font = 0;			/* force a font load */
#ifndef NO_MULTI_BYTE
	/*
	 * If the EUC information for this channel has been set and we
	 * are not in text mode, assume we are in graphics text mode.
	 */
	} else if (GS_CHAN_INITIALIZED(chp)) {
		/*
		 * This assert will fail if we didn't select graphics mode.
		 * Assume we are using 8 x 16 bit fonts as default.
		 */
		ASSERT(WSCMODE(vp)->m_xpels != 0);

		/*
		 * Save the contents of the screen so that we can restore
		 * it on the graphics display.
		 */
		if (chp->ch_dmode == KD_TEXT0 && chp == ws_activechan(&Kdws))
			kdv_scrxfer(chp, KD_SCRTOBUF);

		chp->ch_dmode = KD_GRTEXT;
		tsp->t_cols = T_COLS;
		tsp->t_rows = T_ROWS;
		tsp->t_scrsz = tsp->t_cols * tsp->t_rows;
#endif /* NO_MULTI_BYTE */
	} else
		chp->ch_dmode = KD_GRAPHICS;

	if (chp == ws_activechan(&Kdws)) {
		kdv_rst(tsp, vp);
		kdv_enable(vp);
		kdv_cursortype(&Kdws, chp, tsp);
		if (WSCMODE(vp)->m_font) {	/* text? */
			tsp->t_cursor -= tsp->t_origin;
			tsp->t_origin = 0;
		}
	}

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

}


/*
 * void
 * kdv_rst(termstate_t *, vidstate_t *)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdv_rst is called by the active channel.
 *
 * Description:
 *	This routine does the bulk of the work in mode switch.
 *	It does card-specific register programming and
 *	updates the vidstate_t structure to reflect the
 *	properties of the new mode (i.e. where in the video
 *	RAM the card looks for data, what font (character
 *	box size - 8x8, 8x14, 8x16) the mode expects)
 */
void
kdv_rst(termstate_t *tsp, vidstate_t *vp)
{
	pl_t		pl;
	extern void	vdc_750cga(termstate_t *, vidstate_t *);
	boolean_t	got_con = B_FALSE;		/* L001 */


	if (CMODE(Kdws, DM_ENH_CGA) && VTYPE(V750)) {
		vdc_750cga(tsp, vp);
		return;
	}

	if (WSCMODE(vp)->m_color) {		/* color mode ? */
		vp->v_regaddr = COLOR_REGBASE;	/* register address */
	} else {				/* monochrome adapter mode */
		vp->v_regaddr = MONO_REGBASE;	/* register address */
		vp->v_undattr = UNDERLINE;	/* Set underline */
	}

	if(!kd_con_acquired)			/* L001 begin */
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}				/* L001 end */


	switch (vp->v_cmos) {
        case MCAP_MONO:
		/*
		 * Mode register (MODE_REG) is protected by acquiring
		 * the workstation mutex lock (w_mutex) since this is
		 * the only register that can be concurrently accessed
		 * by active and non-active channels.
		 */
		pl = LOCK(Kdws.w_mutex, getpl());
		outb(vp->v_regaddr + MODE_REG, M_ALPHA80);
		UNLOCK(Kdws.w_mutex, pl);
		kdv_setregs((char *)kd_monoinittab, I_6845MONO);
		pl = LOCK(Kdws.w_mutex, getpl());
		outb(vp->v_regaddr + MODE_REG, vp->v_modesel);
		UNLOCK(Kdws.w_mutex, pl);
		break;

	case MCAP_COLOR:
		kdv_disp(0);		/* Turn off display */
		/* set registers */
		kdv_setregs(
		(char *)&kd_cgainittab[kd_cgaregtab[vp->v_cvmode].cga_index], 
		I_6845COLOR);	
		/* set color */
		outb(vp->v_regaddr + COLOR_REG, 
			kd_cgaregtab[vp->v_cvmode].cga_color);	
		/* save new mode */
		vp->v_modesel = kd_cgaregtab[vp->v_cvmode].cga_mode;
		vp->v_font = WSCMODE(vp)->m_font;
           	break;

	case MCAP_EGA:
		kdv_rstega(tsp, vp);
		break;
	}

	vp->v_rscr = (caddr_t) WSCMODE(vp)->m_base;	/* Set base */
	/* XXX: vp->v_scrp = (ushort *) phystokv(vp->v_rscr); */
	/* LINTED pointer alignment */
	vp->v_scrp = (ushort *) WSCMODE(vp)->m_vaddr;

#ifdef EVGA
if ((vp->v_cvmode != ENDNONEVGAMODE+15)&&(vp->v_cvmode != ENDNONEVGAMODE+16)
 && (vp->v_cvmode != ENDNONEVGAMODE+34)&&(vp->v_cvmode != ENDNONEVGAMODE+35)) {
#endif /* EVGA */

	if (!WSCMODE(vp)->m_font) {
		if (vp->v_rscr == (caddr_t)EGA_BASE && WSCMODE(vp)->m_color) {
			/*
			 * Program the graphics controller mode register. Set 
			 * the write mode used for write access to video RAM.
			 */
			outb(0x3ce, 0x05); outb(0x3cf, 0x02);
		}

		bzero((caddr_t)vp->v_scrp, WSCMODE(vp)->m_size);

		/* XXX: Do not need this check again. */
		if (vp->v_rscr == (caddr_t)EGA_BASE && WSCMODE(vp)->m_color) {
			struct b_param	*initp;

			initp = kdv_getparam(vp->v_cvmode);
			out_reg(&kd_regtab[I_GRAPH], 0x5,
					((char *)initp->graphtab)[0x5]);
		}
	}

#ifdef EVGA
}
#endif /* EVGA */

	if(got_con){				/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}					/* L001 end */


}


/*
 * void
 * kdv_enable(vidstate_t *)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *      If the w_rwlock is held in shared mode, then following
 *      conditions must be true.
 *              - ch_mutex basic lock is held.
 *              - kdv_enable() is called by an active channel.
 */
void
kdv_enable(vidstate_t *vp)
{
	pl_t	opl;
	boolean_t got_con = B_FALSE;			/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			kd_con_acquired = B_TRUE;
			got_con = B_TRUE;
		}					/* L001 end */



	switch (vp->v_cmos) {
	case MCAP_COLOR:
		opl = LOCK(Kdws.w_mutex, plhi);
		outb(vp->v_regaddr + MODE_REG, vp->v_modesel);	/* Set mode */
		UNLOCK(Kdws.w_mutex, opl);
		if (VTYPE(V400))
                        outb(vp->v_regaddr + MODE2_REG, Vdc.v_mode2sel);
		break;

	case MCAP_EGA:
		(void) inb(vp->v_regaddr + IN_STAT_1);
		/* turn on palette */
		outb(kd_regtab[I_ATTR].ri_address, PALETTE_ENABLE);
		kdv_disp(1);
		break;
	}

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

}


/*
 * STATIC void
 * kdv_disp(int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	- w_mutex lock is acquired to serialize access
 *	  to the video controller, since mutliple terminal
 *	  control operation can concurrently access the
 *	  mode register.
 *
 * Description:
 *	Wait until 6845 is in vertical retrace, and then turn it off.
 *
 * Note:
 *	It is necessary to turn off display while changing between display
 *	modes because it prevents sudden signals reaching the monitor.
 */
STATIC void
kdv_disp(int on)
{
	vidstate_t	*vp = &Kdws.w_vstate;
	pl_t		pl;
	boolean_t	got_con = B_FALSE;		/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L001 end */

	pl = LOCK(Kdws.w_mutex, plhi);	

	if (on) {
		/* turn on the display by enabling video signals */
		vp->v_modesel |= M_ENABLE;
		outb(vp->v_regaddr + MODE_REG, vp->v_modesel);
	} else if (vp->v_modesel & M_ENABLE) {	/* Is the display enabled? */
		/* wait for vertical sync */
		WAIT_VSYNC(vp, S_VSYNC);
		/* turn off the display by disabling video signals */
		vp->v_modesel &= ~M_ENABLE;
		outb(vp->v_regaddr + MODE_REG, vp->v_modesel);
	}

	UNLOCK(Kdws.w_mutex, pl);

	if(got_con)					/* L001 */
		ws_con_done_text();			/* L001 */

}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * STATIC void
 * kdv_getregs(char *, int)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdv_getregs is called by the active channel.
 *
 * Description:
 *	Get current video controller register values and store them
 *	in the table.
 *
 *	See remarks below in kdv_setregs.
 */
STATIC void
kdv_getregs(vidstate_t *vp, char *tabp, int type)
{
 	int	index, count;
	struct reginfo	*regp;
	uchar_t	data;
	boolean_t got_con = B_FALSE;			/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */

	regp = &kd_regtab[type];
	count = regp->ri_count;
	index = (type == I_SEQ) ? 1 : 0;
	if (type == I_ATTR) {
		kdv_wait_vsync(vp, S_VSYNC);
	}
	for (; index < count; index++, tabp++) {
		data = 0;

		if (type == I_ATTR) {
			/* reset the latch */
			(void) inb(vp->v_regaddr + IN_STAT_1);
			/* load the index register, toggle the latch */
			outb(regp->ri_address, index);
			/* read the data (Note: Does not toggle the latch.) */
			data = inb(0x3c1);
		} else {
			/* in_reg is a multi-line macro */
			in_reg(regp, index, data);
		}


		*tabp = data;
	}

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */
}

#endif /* DEBUG || DEBUG_TOOLS */


/*
 * STATIC void
 * kdv_setregs(char *, int)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdv_setregs is called by the active channel.
 *
 * Description:
 *	Set register values from a table.
 *
 * Remarks:
 *	The table type provides information on the length of the table,
 *	the address register, and the data register. This routine assumes 
 *	that all registers are accessed through a common I/O address, 
 *	and that they are distinguished by the number written in the 
 *	address register just before the value is written in the data
 *	register.
 */
STATIC void
kdv_setregs(char *tabp, int type)
{
 	int	index, count;
	struct reginfo	*regp;
	boolean_t got_con = B_FALSE;			/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */

	regp = &kd_regtab[type];
	count = regp->ri_count;
	index = (type == I_SEQ) ? 1 : 0;
	for (; index < count; index++, tabp++) {
		/* out_reg is a multi-line macro */
		out_reg(regp, index, *tabp);
	}

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */
}


/*
 * STATIC void
 * kdv_rstega(termstate_t *, vidstate_t *)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdv_rstega is called by the active channel.
 *	
 * Description:
 *	This routine is called to reload the font because
 *	for EGA/VGA controllers, the copy of the font
 *	stored in the video RAM to generate characters when
 *	processing text can be blown away when going into
 *	high-resolution modes. In contrast, for CGA and 
 *	MCA, the copy of the font lives in a seperate
 *	chunk of the video RAM. Also, for EGA and VGA
 *	on color monitors can be programmed so that a
 *	certain attribute can be interpreted as either
 *	"blink" or "bright background color".
 */
STATIC void
kdv_rstega(termstate_t *tsp, vidstate_t *vp)
{
	unchar	fnt;			/* Font type for EGA mode */
	boolean_t got_con = B_FALSE;	/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}			/* L001 end */

	fnt = WSCMODE(vp)->m_font;	/* get font type for mode */

	kdv_setall(vp, vp->v_cvmode);	/* v_cvmode set in kdv_setdisp */

	if (fnt != 0 && fnt != vp->v_font) {
		kdv_ldfont(vp, fnt);
	}

	vp->v_font = fnt;		/* remember which font is loaded */

	/*
	 * Adjust cursor start and end positions. 
	 */
	if (!VTYPE(V600 | CAS2)) {

		if (fnt == FONT8x8) {
			outb(0x3d4, 0xa); outb(0x3d5, 0x6);	/* L003 */
			outb(0x3d4, 0xb); outb(0x3d5, 0x7);	/* L003 */
		} else {
			outb(0x3d4, 0xa); outb(0x3d5, 0xb);	/* L003 */
			outb(0x3d4, 0xb); outb(0x3d5, 0xc);	/* L003 */
		}

	}

	if (WS_ISINITED(&Kdws)) {
		/* Initialization was successful */
		if (vp->v_regaddr == MONO_REGBASE) {
			tsp->t_attrmskp[1].attr = 0;
			tsp->t_attrmskp[4].attr = 1;
			tsp->t_attrmskp[34].attr = 7;
		} else {
			tsp->t_attrmskp[1].attr = BRIGHT;
			tsp->t_attrmskp[4].attr = 0;
			tsp->t_attrmskp[34].attr = 1;
		}
	}

	if(got_con){				/* L001  begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}					/* L001 end */

}


/*
 * void
 * kdv_setcursor_type(int)
 *
 * Calling/Exit State:
 *	None.
 */
void
kdv_setcursor_type(int curtype)
{
	uchar_t	startsline;
	uchar_t	endsline;
	boolean_t got_con = B_FALSE;			/* L001 */


	/*
         * Adjust cursor start and end positions. 
	 */
        startsline = curtype & 0xff;
        endsline = (curtype >> 16) & 0xff;

	if(!kd_con_acquired)				/* L001 begin */
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L001 end */

        outb(0x3d4, 0xa); outb(0x3d5, startsline);	/* L003 */
        outb(0x3d4, 0xb); outb(0x3d5, endsline);	/* L003 */

	if(got_con)					/* L001 */
		ws_con_done_text();			/* L001 */

}


/*
 * int
 * kdv_stchar(ws_channel_t *, ushort, ushort, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *
 * Description:
 *	This routine is called by the workstation code
 *	that handles TCL messages to display characters
 *	(actually unsigned shorts since the data contains
 *	an attribute as well as the character) on the screens. 
 *	The screen buffer pointer in the vidstate_t structure
 *	for the channel is used as the beginning of the
 *	screen image, characters are transferred into the
 *	screen at the offset specified by the caller.
 */
int
kdv_stchar(ws_channel_t *chp, ushort dest, ushort ch, int count)
{
	ushort		scrmsk, *dstp;
	vidstate_t	*vp = &chp->ch_vstate;
	int		cnt, avail;
	boolean_t	got_con = B_FALSE;		/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */

	/*
	 * Write char to std display or virtual screen. 
	 */

	scrmsk = vp->v_scrmsk;		/* get mask for wrapping memory */
	do {
		dest &= scrmsk;		/* wrap modulo memory size */
		dstp = vp->v_scrp + dest;
		avail = scrmsk - dest + 1;
		if (count < avail)	/* it will fit without wrapping */
			avail = count;
		count -= avail;

		/*
		 * If the channel is active and is on a standard
		 * CGA controller, the display is turned off while
		 * characters are copied into the video buffer
		 * and then turned back on. In other words, on 
		 * color adapters we must write during the horizontal
		 * blanking intervals. In all other cases, a
		 * direct memory copy is performed.
		 */
		if (chp == ws_activechan(&Kdws) && ATYPE(Kdws, MCAP_COLOR) 
						&& !VTYPE(V400 | V750)) {
			if (avail < 8) {
				for (cnt = 0; cnt < avail; cnt++)
					(void)kdv_mem(KD_WRVMEM, dstp++, ch);
			} else {
				kdv_disp(0);
				for (cnt = 0; cnt < avail; cnt++)
					*dstp++ = ch;
				kdv_disp(1);
			}
		} else
			for (cnt = 0; cnt < avail; cnt++)
				*dstp++ = ch;
		dest += avail;

	} while (count != 0);

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

	return (0);
}


/*
 * STATIC void
 * kdv_setall(vidstate_t *, unchar)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdv_setall is called by the active channel.
 *
 * Description:
 *	Set the crt controller, sequencer, miscellaneous,
 *	graphics and attribute registers.
 *
 * Remarks:
 *
 *	CRTC Controller:
 *	
 *	The CRTC registers 0-7 must be write protected by setting
 *	bit 7 of CRTC register 0x11 to 1. This is necessary because
 *	these registers control timing functions and changing them
 *	could cause problems. This applies to VGA only cards.
 *
 *	Attribute Controller:
 *
 *	The attribute registers operate through a flip/flop which
 *	toggles between the address register and index register after
 *	every write. The attribute register may have been in index
 *	mode by another routine, so you should always reset it to
 *	address mode by reading from port 0x3?a (input status reg one).
 *	Additionally, the attribute registers should only be changed
 *	during vertical retrace. Because the retrace status is available
 *	through Input Status Register One, reading the register serves
 *	dual purpose of resetting the flip/flop and establishing the
 *	retrace status.
 *
 *	Sequencer Controller:
 *
 *	We should turn off video via the clocking mode register while
 *	the video controller is being programmed. The following code
 *	sequence should be added to the beginning and end of the function.
 *
 *		if (type == I_SEQ && index == 1) {
 *			(* Turn off video via bit 5, clocking mode register. *)
 *			out_reg(&kd_regtab[I_SEQ], 1, initp->seqtab[1]|0x20);
 *		}
 *
 *		(* Clear disable video set in clocking mode register. *)
 *		outb(0x3c4, 0x01); outb(0x3c5, initp->seqtab[1]);
 */
STATIC void
kdv_setall(vidstate_t *vp, unchar mode)
{
	struct b_param	*initp;
	unchar		tmp_reg;
	boolean_t	got_con = B_FALSE;		/* L001 */
	extern void	vdc_lk750(int);
	extern void	vdc_cas2extregs(vidstate_t *, uchar_t);
	extern uchar_t	vdc_unlk600(void);
	extern void	vdc_unlkcas2(void);

	if(!kd_con_acquired)				/* L001 begin */
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */

	/*
	 * Turn the display off.
	 *
	 * kdv_setall is only called by kdv_rstega, which is only
	 * called by kdv_rst. Every call to kdv_rst is followed by
  	 * a call to kdv_enable, which turns the display back on.
	 */
	kdv_disp(0);

#if defined(DEBUG) || defined(DEBUG_TOOLS)
	if (kdv_bios & KDV_SET_BIOS_PARAMS)
		initp = &kd_reginittab;
	else
		initp = kdv_getparam(mode);
#else
	initp = kdv_getparam(mode);
#endif /* DEBUG || DEBUG_TOOLS */

#ifdef EVC
	mode = evc_init(mode);	/* pass through if no EVC-1 */
#endif /* EVC */

	if (VTYPE(V750) && VSWITCH(ATTDISPLAY))
		(void)kdv_setmode0(mode);	/* sets mode0 appropriately */

	/*
	 * Set the Palette Address Source bit to 0 to clear the screen
	 * while we reprogram all the registers.
	 */
        (void) inb(vp->v_regaddr + IN_STAT_1);	/* Initialize flip-flop */
	outb(kd_regtab[I_ATTR].ri_address, 0);

	switch (Vdc.v_type) {
	case V750:
		if (mode == DM_ENH_CGA) {
			/* set Super-Vu to CGA mode */
			inb(0x3d8);
			inb(0x3d8);		/* unmask status 1/misc out register */
			outb(0x3c2, 0x23);
			vdc_lktime(0);		/* unlock timing registers */
			vdc_scrambler(0);	/* turn off scrambler logic */
		}
		break;

	case V600:
		tmp_reg = vdc_unlk600();
		outb(0x3ce, 0x0d); outb(0x3cf, 0x00);
		outb(0x3ce, 0x0e); outb(0x3cf, 0x00);
		break;

	case CAS2:
		vdc_unlkcas2(); 	/* Cascade 2-specific diddling */
		break;

	default:
		break;
	}

	/*
	 * Reset sequencer. This is necessary for preserving the contents
	 * of video memory when the Clocking mode register (port = 0x3c5,
	 * index 1) is changed.
	 */
	out_reg(&kd_regtab[I_SEQ], 0, SEQ_RST);

	/*
	 * Set miscellaneous register.
	 */
	outb(MISC_OUT, initp->miscreg);

	if (VTYPE(V750) && VSWITCH(ATTDISPLAY))
		vdc_lk750(mode);

	/*
	 * Initialize CRT controller.
	 */
	if (WSMODE(vp, mode)->m_color) {
		if (DTYPE(Kdws, KD_VGA)) {
			/*
		 	 * Clear the CRTC protect register 0-7 bit to 
			 * unlock the CRTC registers.
			 */
			out_reg(&kd_regtab[I_EGACOLOR], 0x11, 
					((char *)&initp->egatab)[0x11] & ~0x80);
		}
		kdv_setregs((char *)&initp->egatab, I_EGACOLOR);
	} else {
		if (DTYPE(Kdws, KD_VGA)) {
			/*
		 	 * Clear the CRTC protect register 0-7 bit to 
			 * unlock the CRTC registers.
			 */
			out_reg(&kd_regtab[I_EGAMONO], 0x11, 
					((char *)&initp->egatab)[0x11] & ~0x80);
		}
		kdv_setregs((char *)&initp->egatab, I_EGAMONO);
	}

	/*
	 * Initialize graphics registers.
	 */
	if (!DTYPE(Kdws, KD_VGA)) {
		outb(GRAPH_1_POS, GRAPHICS1);
		outb(GRAPH_2_POS, GRAPHICS2);
	}
	kdv_setregs((char *)initp->graphtab, I_GRAPH);

	/*
	 * Program sequencer registers.
	 */
	kdv_setregs((char *)initp->seqtab, I_SEQ);
	out_reg(&kd_regtab[I_SEQ], 0, SEQ_RUN);

	/*
	 * Turn display off. Loading the sequencer registers turns
	 * on display.
	 */
	kdv_disp(0);

	/*
	 * Initialize attribute registers.
	 */
	(void) inb(vp->v_regaddr + IN_STAT_1);	/* Initialize flip-flop */
	WAIT_VSYNC(vp, S_VSYNC);		/* L005 */
	kdv_setregs((char *)initp->attrtab, I_ATTR);

	/*
	 * v_border gets set via KDSBORDER ioctl. 
	 */
	out_reg(&kd_regtab[I_ATTR], 0x11, vp->v_border);

	if (DTYPE(Kdws, KD_VGA)) {
		/* Clear the attribute controller color select register. */
		outb(0x3c0, 0x14); outb(0x3c0, 0x00);

		switch (Vdc.v_type) {
		case V600: 
			outb(0x3ce, 0x0e);
			if (mode == DM_VDC640x400V)
				outb(0x3cf, 0x01);
			else
				outb(0x3cf, 0x00);
			outb(0x3ce, 0x0f); outb(0x3cf, tmp_reg);
			break;

		case CAS2:
			out_reg(&kd_regtab[I_SEQ], 0, SEQ_RST);
			vdc_cas2extregs(vp,mode); 
			out_reg(&kd_regtab[I_SEQ], 0, SEQ_RUN);
			break;

#ifdef EVC
           	case VEVC:
                        evc_finish(mode);
                        break;
#endif /* EVC */

		default:
			break;
		}

		kdv_ldramdac(vp);
	}

	if (!WSMODE(vp, mode)->m_font)		/* graphics */
		/* clear screen, do something special for DM_VDC640x400V */
		kdv_clrgraph(vp, mode);

	if(got_con){				/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}					/* L001 end */

}


/*
 * STATIC void
 * kdv_setmode0(unchar)
 * 
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdv_setmode0() is called by an active channel.
 *
 * Note:
 *	Should never be called for evga. 
 */
STATIC void
kdv_setmode0(unchar mode)
{
	unchar	monid;
	boolean_t got_con = B_FALSE;		/* L001 */

	if(!kd_con_acquired)			/* L001 begin */
		if(ws_con_need_text())
			return;
		else{
			kd_con_acquired = B_TRUE;
			got_con = B_TRUE;
		}				/* L001 end */


	vdc_scrambler(0);		/* disable scrambler logic */

	monid = vdc_rdmon(mode);
	if (monid & 0x02)		/* non-multimode */
		outb(0x3de, 0x00);	/* reset mode0 */
	else if (mode == DM_ATT_640)	/* multimode */
		outb(0x3de, 0x00);	/* reset mode0 */
	else
		outb(0x3de, 0x10);	/* set mode0 */

	vdc_scrambler(1);		/* reenable scrambler logic */

	if(got_con){			/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}				/* L001 end */

}


/*
 * STATIC void
 * kdv_ldramdac(vidstate_t *)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdv_ldramdac() is called by an active channel.
 *
 * Description:
 *	Load RAM DAC (digital to analog).
 *
 * Remarks:
 *	The video DAC provides analog RGB signal capability, allowing
 *	262,144 possible color combinations to be displayed on an
 *	analog RGB monitor.
 */
STATIC void
kdv_ldramdac(vidstate_t *vp)
{
 	unchar	*start, *end;
	int	efl;
	boolean_t got_con = B_FALSE;			/* L001 */


#ifdef EVGA
	/* Don't assume all vga are using m_ramdac */
    if ((WSCMODE(vp)->m_ramdac >= 0) && (WSCMODE(vp)->m_ramdac <= 3)) {
#endif /* EVGA */

	if(!kd_con_acquired)				/* L001 begin */
		if(ws_con_need_text())
			return;
		else{
			kd_con_acquired = B_TRUE;
			got_con = B_TRUE;
		}					/* L001 end */

	/*
	 * Wait for assertion BLANK to DAC retrace interval,
	 * to prevent snow.
	 */
	kdv_wait_vsync(vp, 0);

	/* disable all interrupts */
	efl = intr_disable();

	kdv_wait_vsync(vp, S_VSYNC);

	/*
	 * Write to video DAC at 0x3c8 indicates that a write sequence
	 * will occur, consisting of three successive writes at 0x3c9:
	 * six LSB (least significant bits) of red, then of green, 
	 * then of blue. 
	 */

	start = &kd_ramdactab[0] + (WSCMODE(vp)->m_ramdac * 0x300);
	end = start + 0x300;
	outb(0x3c6, 0xff);
	outb(0x3c8, 0x00);
	while (start != end)
		outb(0x3c9, *start++);

	/* restore interrupt state */
	intr_restore(efl);

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

#ifdef EVGA
    }
#endif /* EVGA */
}


/*
 * STATIC void
 * kdv_clrgraph(vidstate_t *, unchar)
 *
 * Calling/Exit State:
 *	w_rwlock is held in exclusive/shared mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdv_clrgraph() is called by an active channel.
 *
 * Description:
 *	Clear graphics controller register.
 */
STATIC void
kdv_clrgraph(vidstate_t *vp, unchar mode)
{
	int		offset;
	unchar		tmp_pr5;
	caddr_t		addr;
	size_t		size;
	boolean_t	got_con = B_FALSE;		/* L001 */
	extern uchar_t	vdc_unlk600(void);

	if(!kd_con_acquired)				/* L001 begin */
		if(ws_con_need_text)
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */

#ifdef EVGA
if ((mode != ENDNONEVGAMODE+15) && (mode != ENDNONEVGAMODE+16)
   && (mode != ENDNONEVGAMODE+34) && (mode != ENDNONEVGAMODE+35)) {
#endif /* EVGA */

	/* XXX: addr = (caddr_t) phystokv(WSMODE(vp, mode)->m_base); */
	addr = (caddr_t) WSMODE(vp, mode)->m_vaddr;
	size = (size_t) WSMODE(vp, mode)->m_size; 
	bzero(addr, size);

	if (mode == DM_VDC640x400V) {
		tmp_pr5 = vdc_unlk600();
		for (offset = 1; offset < 4; offset++) {
			outb(0x3ce, 0x09); outb(0x3cf, (0x10 * offset));
			bzero(addr, size);
		}
		outb(0x3ce, 0x09); outb(0x3cf, 0x00);
		outb(0x3ce, 0x0f); outb(0x3cf, tmp_pr5);
	}

#ifdef EVGA
}
#endif /* EVGA */

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

}


/*
 * STATIC void
 * kdv_mem(int, ushort *, ushort)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- w_mutex lock is acquired to serialize access 
 *	  to the status register.
 *
 * Description:
 *	Wait for horizontal retrace, then read a word from video memory.
 */
STATIC ushort 
kdv_mem(int op, ushort *addr, ushort word)
{
	ushort		tmp = 0;
	vidstate_t	*vp = &Kdws.w_vstate;
	pl_t		pl;
	int		efl;
	boolean_t	got_con = B_FALSE;		/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */


	pl = LOCK(Kdws.w_mutex, plhi); 

	/*
	 * Wait while memory is being read by EGA during the active display.
	 */
	while ((inb(vp->v_regaddr + STATUS_REG) & S_UPDATEOK))
		;

	/*
	 * Disable interrupts so that we can be sure of getting an 
	 * entire horizontal retrace.
	 */
	efl = intr_disable();

	/*
	 * Wait until it is safe to update buffer (during horizontal or
	 * vertical retrace).
	 */
	while (!(inb(vp->v_regaddr + STATUS_REG) & S_UPDATEOK))
		;

	if (op == KD_RDVMEM)
		tmp = *addr;
	else
		*addr = word;

	/* restore interrupt state */
	intr_restore(efl);

	UNLOCK(Kdws.w_mutex, pl); 

	if(got_con){					/* L001 end */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 begin */

	return (tmp);
}


/*
 * STATIC void
 * kdv_xfer(ushort *, ushort *, int, char)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive/shared mode.
 *	- channel for which the screen is saved/restored
 *	  is also locked.
 *
 * Description:
 *	Move words into color adapter memory.
 */
STATIC void
kdv_xfer(ushort *srcp, ushort *dstp, int cnt, char dir)
{
boolean_t got_con = B_FALSE;				/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else
			got_con = B_TRUE;		/* L001 end */

 	if (dir == UP) {
		while (cnt--)
                        *dstp-- = *srcp--;
	} else {
                while (cnt--)
                        *dstp++ = *srcp++;
	}

	if(got_con)					/* L001 */
		ws_con_done_text();			/* L001 */

}


/*
 * int
 * kdv_mvword(ws_channel_t *, ushort, ushort, int, char)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *	- This routine is called by the workstation code
 *	  for moving the screen contents around. 
 *
 * Description:
 *	Move words around in memory.
 *
 *	It sets up for a memory copy within a video RAM.
 *	For standard CGA devices, the display of characters
 *	is turned off while the copy is taking place and
 *	then reenabled. For other video controllers,
 *	direct memory copy is done.
 */
int
kdv_mvword(ws_channel_t *chp, ushort from, ushort to, int count, char dir)
{
	ushort		*buf = chp->ch_vstate.v_scrp;
	vidstate_t	*vp = &chp->ch_vstate;
	ushort		scrmsk, *srcp, *dstp;
	int		avail;
	boolean_t	got_con = B_FALSE;		/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */


	/*
         * Move chars on std display or virtual screen. 
	 */

	scrmsk = vp->v_scrmsk;

	if (dir == UP) {
		from += count - 1;
		to += count - 1;
	}

	while (count > 0) {
		from &= scrmsk;
		to &= scrmsk;
		if (dir == UP)
			avail = ((from < to) ? from : to) + 1;
		else
			avail = scrmsk - ((from > to) ? from : to) + 1;
		if (count < avail)
			avail = count;
		srcp = vp->v_scrp + from;
		dstp = vp->v_scrp + to;
		if (dir == UP) {
			if (ATYPE(Kdws, MCAP_COLOR) && !VTYPE(V400 | V750)) {
				if (avail >= 8) {
					kdv_disp(0);
					vcopy(srcp, dstp, (size_t)avail, dir);
					kdv_disp(1);
				} else {
					while (avail--) 
						kdv_mem(KD_WRVMEM, dstp--, 
							kdv_mem(KD_RDVMEM, 
								srcp--, 0));
				}
			} else
				kdv_xfer(srcp, dstp, avail, UP);
			from -= avail;
			to -= avail;
		} else {
			if (ATYPE(Kdws, MCAP_COLOR) && !VTYPE(V400 | V750)) {
				if (avail >= 8) {
					kdv_disp(0);
					vcopy(srcp, dstp, (size_t)avail, dir);
					kdv_disp(1);
				} else {
					while (avail--) 
						(void)kdv_mem(KD_WRVMEM, dstp++,
							kdv_mem(KD_RDVMEM, 
								srcp++, 0));
				}
			} else
				kdv_xfer(srcp, dstp, avail, DOWN);
			from += avail;
			to += avail;
		}
		count -= avail;
	}

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

	return (0);
}


/*
 * int
 * kdv_ckherc(vidstate_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *	- Returns 1 if its an hercules card, otherwise
 *	  0 is returned.
 *
 * Description:
 *	Test if attached adapter is really a Hercules graphics adapter
 *	rather than just an IBM monochrome display adapter.
 */
int
kdv_ckherc(vidstate_t *vp)
{
 	ushort *basep, save;
	int	rv = 0;		/* assume it is not a hercules */
	boolean_t got_con = B_FALSE;		/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else
			got_con = B_TRUE;	/* L001 end */


	outb(0x3bf, 0x02);	/* map in 2nd page of hercules memory */
	/* XXX: basep = (ushort *)phystokv(WSCMODE(vp)->m_base); */
	/* LINTED pointer alignment */
	basep = (ushort *) WSCMODE(vp)->m_vaddr;
	save = *basep;
	*basep = 0;			/* set first word of memory to zero */
	*(basep + 0x4000) = 1;		/* set first word of 2nd page to one */

	if (*(basep + 0x4000) == 1 && *basep == 0) { 
		/* probably a hercules */
		outb(0x3bf, 0x00);	/* unmap 2nd page of memory */
		rv = 1;
	}

	*basep = save;

	if(got_con)				/* L001 */
		ws_con_done_text();

	return (rv);
}


/*
 * STATIC char
 * kdv_rdsw(void)
 *	read switch settting.
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Description:
 *	Read EGA switches and convert to an operational mode.
 */
STATIC int 
kdv_rdsw(void)
{
	int	cnt;
	unchar	sw = 0;
	boolean_t got_con = B_FALSE;			/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else
			got_con = B_TRUE;		/* L001 end */


	/*
	 * Read switches, create index, and read swmode table.
	 */
	outb(MISC_OUT, 0x1);
	for (cnt = 3; cnt >= 0; cnt--) {
		outb(MISC_OUT, ((cnt << CLKSEL) + 1));
		if (inb(IN_STAT_0) & SW_SENSE)
			sw |= (1 << (3 - cnt));
	}

	if(got_con)					/* L001 */
		ws_con_done_text();			/* L001 */

	return (kd_swmode[sw & 0x0f]);
}


/*
 * STATIC void
 * kdv_params(void)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Description:
 *	The default state of the EGA registers can be read
 *	from the Video Parameter Table in BIOS memory. The 
 *	location of this table is pointed to by the Video
 *	Parameter Table pointer, a double word pointer that
 *	resides in the first location of the Table of Save
 *	Area Pointers. The Table of Save Area Pointers is
 *	pointed to by a second pointer. This pointer is
 *	called the Pointer to Table of Save Area Pointers.
 *	It resides in host memory at location 0x4a8. Thus,
 *	it takes two pointers to arrive at the actual 
 *	address of the Video Parameter Table.
 */
STATIC void
kdv_params(void)
{
	vidstate_t	*vp = &Kdws.w_vstate;


	/*
	 * XXX: 
	 *	Kdws.w_vstate.v_parampp = (unchar **)phystokv(
	 *				ftop(*(ulong *)phystokv(0x4a8))); 
	 */
	vp->v_parampp = (unchar **)physmap(
		/* LINTED pointer alignmnet */
		kdv_ftop(*(ulong *)physmap(0x4a8, sizeof(ulong), KM_NOSLEEP)),
		/* LINTED pointer alignmnet */
		sizeof(unchar *), KM_NOSLEEP);

	if (vp->v_parampp == NULL)
		/*
		 *+ Unable to allocate virtual mapping for the double 
		 *+ pointer parameter table. This table contains the
		 *+ the values for the video controller registers in
		 *+ different modes.
		 */
		cmn_err(CE_PANIC, 
			"kdv_params: no virtual memory available");
	return;
}


/*
 * STATIC void
 * kdv_ldfont(vidstate_t *, unchar)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 * 
 * Description:
 *	kdv_ldfont() loads the font for the character box
 *	size used by the current video mode (must be a text
 *	mode). The information is stored in the channels 
 *	vidstate_t structure. This may point to the ROM
 *	font or kernel space allocated for the user 
 *	supplied font.
 */
STATIC void
kdv_ldfont(vidstate_t *vp, unchar fnt)
{
	unchar		*from;		/* Source in ROM */
	unchar		*to;		/* Destination in generator */
	ushort		skip;		/* Bytes to skip when loading */
	ushort		i;		/* Counter */
	ushort		bpc;
	ulong		count;
	unchar		ccode;		/* Character code to change */
	int		pervtflag = 0;
	boolean_t	got_con = B_FALSE;	/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}				/* L001 end */


	outb(0x3c4, 0x02); outb(0x3c5, 0x04);	/* enable bit plane 2 */
	outb(0x3c4, 0x04); outb(0x3c5, 0x06);
	outb(0x3ce, 0x05); outb(0x3cf, 0x00);
	outb(0x3ce, 0x06); outb(0x3cf, 0x04);

	switch (fnt) {
	case FONT8x14m:		/* load 8x14 font first */
		from = vp->v_fontp[FONT8x14].f_fontp;
		bpc = vp->v_fontp[FONT8x14].f_bpc;
		count = vp->v_fontp[FONT8x14].f_count;
		pervtflag = !(from == kd_romfonts[FONT8x14].f_fontp);
		break;

	case FONT9x16:		/* load 8x16 font first */
		from = vp->v_fontp[FONT8x16].f_fontp;
		bpc = vp->v_fontp[FONT8x16].f_bpc;
		count = vp->v_fontp[FONT8x16].f_count;
		pervtflag = !(from == kd_romfonts[FONT8x16].f_fontp);
		break;

	default:
		from = vp->v_fontp[fnt].f_fontp;	/* point to table */
		bpc = vp->v_fontp[fnt].f_bpc;		/* bytes / character */
		count = vp->v_fontp[fnt].f_count;	/* character count */
		pervtflag = !(from == kd_romfonts[fnt].f_fontp);
		break;
	}

	/* XXX: to = (unchar *) phystokv(CHGEN_BASE); */
	to = (unchar *) chgen_vaddr;	/* Point to generator base */

	skip = 0x20 - bpc;		/* Calculate bytes to skip in gen */
	while (count-- != 0) {		/* Copy all characters from ROM */
		for (i = bpc; i != 0; i--)	/* Copy valid character */
			*to++ = *from++;	/* Copy byte of character */
		to += skip;		/* Skip over unneeded space */
	}

	/*
	 * Only apply ROM font changes for 8x14m or 9x16 if we 
	 * got base font from the ROM.
	 */
	if ((fnt == FONT8x14m && 
	     kd_romfonts[FONT8x14].f_fontp == vp->v_fontp[FONT8x14].f_fontp) ||
	    (fnt == FONT9x16 && 
	     kd_romfonts[FONT8x16].f_fontp == vp->v_fontp[FONT8x16].f_fontp)) {

		from = vp->v_fontp[fnt].f_fontp;
		while ((ccode = *from++) != 0) {
			/*
			 * XXX:
			 *	to = (unchar *) (phystokv(CHGEN_BASE) + 
			 *				ccode * 0x20);
			 */
			to = (unchar *) (chgen_vaddr + ccode * 0x20);
			for (i = bpc; i != 0; i--)
				*to++ = *from++;
		}
	}

	/*
	 * This code is used to overlay any defined user changes. Only
	 * do this is we see that VT has not set up its own private font
	 * but rather is using the ROM font.
	 */
	if (kd_romfont_mod != NULL && !pervtflag) {  /* apply font changes */
		struct char_def *cdp;
		unsigned int	numchar, j;
		rom_font_t	*rfp;

		/* LINTED pointer alignmnet */
		rfp = (rom_font_t *) kd_romfont_mod;
		numchar = rfp->fnt_numchar;
		cdp = &rfp->fnt_chars[0];
		for (j = 0; j < numchar; j++) {
			from = (unchar *) cdp + kd_font_offset[fnt];
			/*
			 * XXX:
			 *	to = (unchar *) (phystokv(CHGEN_BASE) + 
			 *				cdp->cd_index * 0x20); 
			 */
			to = (unchar *) (chgen_vaddr + cdp->cd_index * 0x20);
			for (i = bpc; i != 0; i--)
				*to++ = *from++;
			cdp++;
	       }
	}

	outb(0x3c4, 0x02); outb(0x3c5, 0x03);	/* enable bit-planes 0 and 1 */
	outb(0x3c4, 0x04); outb(0x3c5, 0x02);	/* enable extended memory */
	outb(0x3ce, 0x04); outb(0x3cf, 0x00);
	outb(0x3ce, 0x05); outb(0x3cf, 0x10);
	outb(0x3ce, 0x06); outb(0x3cf, (WSCMODE(vp)->m_color ? 0x0e : 0x0a));

	if(got_con){				/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}					/* L001 end */

}


/*
 * int
 * kdv_shiftset(vidstate_t *, int)
 *	
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is held.
 *	- Called by an active channel.
 *
 * Description:
 *	Depending on the display adapter installed and on direction bit
 *	(shift in or shift out) the alternate character set for display
 *	of text characters.
 *
 * Note:
 *	The character map select register selects which section of
 *	bit plane 2 contains the character generators in text modes.
 *	Bit plane 2 is divided into 1-4 8K sections (depending on 
 *	the amount of memory installed on the EGA). On the EGA, each
 *	of these sections may contain one character generator for a
 *	total of four. The VGA allows each section to hold two char-
 *	acter maps. Two of these four (or eight) may be selected as
 *	the primary and secondary character sets for a total of 512
 *	displayable characters (chosen from a possible 1024 or 2048).
 *	The EGA supports 256 character definitions for every 64K
 *	installed.
 *
 *	Usually character maps A and B have the same value and only
 *	256 characters are available. However, when maps A and B are
 *	programmed with different values, attribute bit 3 (intensity)
 *	is used as the character set selector (and what appears as
 *	high intensity in most programs will appear as the additonal
 *	256 characters).
 *
 *	Bit 3 of the text-attribute byte specifies whether the text
 *	(foreground) is in highlight or whether an alternate character
 *	font is used. To allow bit 3 of the text-attribute to select
 *	dual-character sets, set map A not equal to map B.
 */ 
int
kdv_shiftset(vidstate_t *vp, int dir)
{
boolean_t got_con = B_FALSE;			/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}				/* L001 end */


	if (ATYPE(Kdws, MCAP_COLOR) && VTYPE(V400)) {
		if (dir)
                        Vdc.v_mode2sel |= M_ALTCHAR;
		else
                        Vdc.v_mode2sel &= ~M_ALTCHAR;
                outb(vp->v_regaddr + MODE2_REG, Vdc.v_mode2sel);
	} else if (ATYPE(Kdws, MCAP_EGA)) {
		/*
		 * Select a character font. It specifies which two fonts
		 * and 8K byte bank of video memory is used as the source 
		 * of the dot patterns for the character generator.
		 */
		if (dir) {
			/*
			 * Select 3rd or 4th 8KB map of font map A and 1st or
			 * 2nd 8KB map of font map B. 
			 */
			out_reg(&kd_regtab[I_SEQ], 3, 0x04);
		} else {
			/*
			 * Select 1st or 2nd 8KB map of font map A and 1st or
			 * 2nd 8KB map of font map B. 
			 */
			out_reg(&kd_regtab[I_SEQ], 3, 0x00);
		}
	}

	if(got_con){				/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_TRUE;
	}					/* L001 end */

	return (0);
}


/*
 * void
 * kdv_setuline(vidstate_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 *	- ch_mutex basic lock is held, when w_rwlock is held in shared mode.
 *	- Called by an active channel, when w_rwlock is held in shared mode.
 */
void
kdv_setuline(vidstate_t *vp, int on)
{
	unchar		tmp, tmp_pr5;
	boolean_t	got_con = B_FALSE;		/* L001 */
	extern uchar_t	vdc_unlk600(void);


 	if (on)
 		Vdc.v_mode2sel |= M_UNDRLINE;
 	else
 		Vdc.v_mode2sel &= ~M_UNDRLINE;

	if(!kd_con_acquired)				/* L001 begin */
		if(ws_con_need_text())
			return;
		else{
			kd_con_acquired = B_TRUE;
			got_con = B_TRUE;
		}					/* L001 end */

 	switch (Vdc.v_type) {
 	case V750:
 		(void)inb(0x3d8);
 		(void)inb(0x3d8);
 		outb(0x3de, 5);
 		break;

 	case V600:
 		tmp_pr5 = vdc_unlk600();
 		outb(0x3ce, 0xc);
 		tmp = (inb(0x3cf) | 0x80);
 		outb(0x3ce, 0xc);
 		outb(0x3cf, tmp);
 		outb(0x3ce, 0xf);
 		outb(0x3cf, tmp_pr5);
 		break;
 
 	case CAS2:
		(void)inb(vp->v_regaddr + IN_STAT_1);
		outb(0x3c0, 0x01); 	/* change blue attribute palette reg */
 		outb(0x3c0, (on) ? 0x07 : 0x01);
 		outb(0x3c0, 0x20);	/* turn palette back on */
 		break;
 		
 	default:
 		break;
 	}

 	if ((!Vdc.v_type && (DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA))) 
#ifdef EVGA
	   || (Vdc.v_type == VEVGA)
#endif /* EVGA */
	   ) {
		/*
		 * Either display is unknown and it's of type EGA or VGA,
		 * or it's an EVGA.
		 */

		(void)inb(vp->v_regaddr + IN_STAT_1);
		outb(0x3c0, 0x01);	/* change blue attribute palette reg */
		outb(0x3c0, (on) ? 0x07 : 0x01);
		outb(0x3c0, 0x20);	/* turn palette back on */
	}

 	outb(vp->v_regaddr + MODE2_REG, Vdc.v_mode2sel);

	if(got_con){				/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}					/* L001 end */

}


/*
 * void
 * kdv_mvuline(vidstate_t *, int)
 *
 * Calling/Exit State:
 *	w_rwlock is held in shared/exclusive mode.
 *	If the w_rwlock is held in shared mode, then following
 *	conditions must be true.
 *		- ch_mutex basic lock is held.
 *		- kdv_mvuline is called by the active channel.
 */
void
kdv_mvuline(vidstate_t *vp, int up)
{
	if (VTYPE(V750) || DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) {

		boolean_t got_con = B_FALSE;		/* L001 begin */

		if(!kd_con_acquired)
			if(ws_con_need_text())
				return;
			else{
				kd_con_acquired = B_TRUE;
				got_con = B_TRUE;
			}				/* L001 end */

		if (VTYPE(V750))
			vdc_lktime(0);

		/* CRTC index register */
 		outb(vp->v_regaddr, 0x14);

		switch (vp->v_cvmode) {
		case DM_B40x25:
		case DM_C40x25:
		case DM_B80x25:
		case DM_C80x25:
			/* data register */
 			outb(vp->v_regaddr + 1, up ? 0x07 : 0x08);
			break;
		default:
			/* data register */
 			outb(vp->v_regaddr + 1, up ? 0x0d : 0x11); 
		} 

		if (VTYPE(V750))
			vdc_lktime(1);

		if(got_con){				/* L001 begin */
			ws_con_done_text();
			kd_con_acquired = B_FALSE;
		}					/* L001 end */

	}
}


/*
 * int
 * kdv_undattrset(wstation_t *, ws_channel_t *, ushort *, short)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *	- Only the active channel is allowed to set underline 
 *	  attribute setting.
 * 
 * Description:
 *	Enable or disable white char underline feature of AT&T video cards.
 */
int
kdv_undattrset(wstation_t *wsp, ws_channel_t *chp, ushort *curattrp, short param)
{
	int		activeflag;
	ws_channel_t	*achp;			/* active channel */
	termstate_t	*tsp = &chp->ch_tstate;
	vidstate_t	*vp = &chp->ch_vstate;
	unsigned char	tmp;
	boolean_t	got_con = B_FALSE;		/* L001 */


	achp = ws_activechan(wsp);
	activeflag = (chp == achp);

	if (DTYPE(Kdws, KD_VGA) && vp->v_regaddr == MONO_REGBASE)
		return (0);

	if(!kd_con_acquired)				/* L001 begin */
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */

	switch (param) {
	case 4:
		if (VTYPE(V400 | V750) || DTYPE(Kdws, KD_EGA) 
					|| DTYPE(Kdws, KD_VGA)) { 
			if (vp->v_undattr == UNDERLINE) {
				if (activeflag) {
					kdv_setuline(vp, 1);
					kdv_mvuline(vp, 1);
				}
				*curattrp |= vp->v_undattr;
			} else
				*curattrp = tsp->t_curattr;
		} else if (DTYPE(Kdws, KD_CGA))
			*curattrp = tsp->t_curattr;
		break;

	case 5:
	case 6:
		/*
		 * 5 and 6 toggle between blink and bright background.
		 */
		if (param == 5) 
			tsp->t_flags &= ~T_BACKBRITE;
		else 
			tsp->t_flags |= T_BACKBRITE;

		if (activeflag) {

			(void) inb(vp->v_regaddr + IN_STAT_1);

			/* attribute mode control register */
			outb(0x3c0, 0x10);

			if (DTYPE(Kdws, KD_VGA))
				tmp = inb(0x3c1);
			else
				tmp = 0x00 | WSCMODE(vp)->m_color << 1; 

			if (param == 5) 
				outb(0x3c0, (tmp | 0x08));
			else 
				outb(0x3c0, (tmp & ~0x08));

			/* turn palette back on */
			outb(0x3c0, 0x20);

		}
		break;

	case 38:
		if (VTYPE(V400 | V750) || DTYPE(Kdws, KD_EGA) 
					|| DTYPE(Kdws, KD_VGA)) {
 			tsp->t_attrmskp[34].attr = 0x03;
			vp->v_undattr = UNDERLINE;
		}
		break;

	case 39:
		if (VTYPE(V400 | V750) || DTYPE(Kdws, KD_EGA) 
					|| DTYPE(Kdws, KD_VGA)) { 
			/* disable underline */
 			tsp->t_attrmskp[34].attr = 0x01;
			vp->v_undattr = NORM;
			if (activeflag) {
				kdv_setuline(vp, 0);
				kdv_mvuline(vp, 0);
			}
		}
		break;

	default:
		break;
	}

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

	return (0);
}


/*
 * int
 * kdv_disptype(ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry or exit.
 * 	- MUST be called with user context for IS_SCOEXEC macro  * L000 *
 */
int
kdv_disptype(ws_channel_t *chp, int arg)
{
	vidstate_t	*vp = &chp->ch_vstate;
	pl_t		opl;
	struct kd_disparam  disp;


	opl = RW_RDLOCK(chp->ch_wsp->w_rwlock, plstr);
	(void) LOCK(chp->ch_mutex, plstr);

	if(IS_SCOEXEC)					/* L000 vvv */
	{
		struct { long type; char *addr; } sco_disp;

		sco_disp.type = vp->v_type;
		sco_disp.addr = (char *) vp->v_rscr;
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(chp->ch_wsp->w_rwlock, opl);

		if (copyout((caddr_t) &disp, (caddr_t) arg, sizeof(sco_disp)))
			return (EFAULT);
	}
	else
	{						/* L000 ^^^ */
		disp.type = vp->v_type;
		disp.addr = (char *) vp->v_rscr;
		bcopy(vp->v_ioaddrs, disp.ioaddr, MKDBASEIO * sizeof(ushort));
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(chp->ch_wsp->w_rwlock, opl);

		if (copyout((caddr_t) &disp, (caddr_t) arg, sizeof(struct kd_disparam)))
			return (EFAULT);
	}
	return (0);
}


/*
 * int
 * kdv_colordisp(void)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
int
kdv_colordisp(void)
{
	return (!(DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES)));
}


/*
 * int
 * kdv_xenixctlr(void)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit. 
 */
int
kdv_xenixctlr(void)
{
	return ((int) kd_adtconv[Kdws.w_vstate.v_type]);
}


/*
 * int
 * kdv_xenixmode(ws_channel_t *, int, int *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 */
int
kdv_xenixmode(ws_channel_t *chp, int cmd, int *rvalp)
{
	vidstate_t	*vp = &chp->ch_vstate;


	switch (cmd) {
	case MCA_GET:
		if (!DTYPE(Kdws, KD_MONO) && !DTYPE(Kdws, KD_HERCULES))
			return(EINVAL);
		break;

	case CGA_GET:
		if (!DTYPE(Kdws, KD_CGA))
			return(EINVAL);
		break;

	case EGA_GET:
		if (!DTYPE(Kdws, KD_EGA))
			return(EINVAL);
		break;

	case CONS_GET:
		break;

	default:
		return (EINVAL);
	}

	*rvalp = vp->v_cvmode;
	if (*rvalp == DM_EGAMONO80x25 && 
			(DTYPE(Kdws, KD_MONO) || DTYPE(Kdws, KD_HERCULES)))
		*rvalp = M_MCA_MODE;		/* Xenix equivalent of mode */
	else if (*rvalp == DM_ENH_B80x43 || *rvalp == DM_ENH_C80x43)
		*rvalp = *rvalp + OFFSET_80x43;	/* Xenix equivalent */

	return (0);
}


/*
 * int
 * kdv_sborder(ws_channel_t *, long)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- chp->ch_mutex basic lock is also held.
 *	- The active channel is only allowed to set border
 *	  in ega color text mode.
 */
int
kdv_sborder(ws_channel_t *chp, long arg)
{
	vidstate_t	*vp = &chp->ch_vstate;


	if (chp->ch_dmode == KD_GRAPHICS || DTYPE(Kdws, KD_CGA))
		return (ENXIO);

	if (!(DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) || VTYPE(V750))
		return (ENXIO);

	if (chp == ws_activechan(&Kdws)) {
		
		boolean_t	got_con = B_FALSE;		/* L001 begin */

		if(!kd_con_acquired)
			if(ws_con_need_text())
				return 0;
			else{
				got_con = B_TRUE;
				kd_con_acquired = B_TRUE;
			}					/* L001 end */

		/*
		 * Read in the Input State #1 register to clear
		 * the flip-flop, so that output to 0x3c0 port load
		 * an index value that points to one of the Attribute
		 * registers into the Address register itself.
		 */ 
		(void) inb(vp->v_regaddr + IN_STAT_1);
		/*
		 * Now set the 0x3c0 port (which is now acting as a
		 * data port for the overscan color register) to secondary
		 * red. Normally, this color is 0x00 (black).
		 * Note: Most EGA/VGA implementations do not operate
		 * satisfactorily  when a color other than black is
		 * selected for the border region.
		 */ 
		out_reg(&kd_regtab[I_ATTR], 0x11, (char) arg);
		outb(kd_regtab[I_ATTR].ri_address, PALETTE_ENABLE);

		if(got_con){					/* L001 begin */
			ws_con_done_text();
			kd_con_acquired = B_FALSE;
		}						/* L001 end */
	}

	vp->v_border = (unchar) arg;

	return (0);
}


/*
 * void
 * kdv_scrxfer(ws_channel_t *, int)
 * 
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive/shared mode.
 *	- chp->ch_mutex basic lock is also held, when w_rwlock is 
 *	  held in shared mode. 
 *	- Called during VT switching or mode switching to
 *	  save or restore the contents of VT being switched
 *	  out/switched in.
 *
 * Description:
 *	Move scrsize words between screen and buffer.
 */
void
kdv_scrxfer(ws_channel_t *chp, int dir)
{
	vidstate_t	*vp = &chp->ch_vstate;
	termstate_t	*tsp = &chp->ch_tstate;
	int		tmp;
	ushort		avail, scrmsk, *srcp, *dstp;
	boolean_t	got_con = B_FALSE;		/* L001 */
	

	if (chp->ch_vstate.v_scrp == NULL || 
            Kdws.w_scrbufpp[chp->ch_id] == NULL) {
		/* This channel has just been closed */
		return;
	}

	scrmsk = vp->v_scrmsk;		/* get mask for wrapping memory */
	avail = scrmsk - (tsp->t_origin & scrmsk) + 1;


	if(!kd_con_acquired)				/* L001 */
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */

	if (dir == KD_SCRTOBUF) {	/* copy from screen to bufer */
		srcp = vp->v_scrp;
		dstp = Kdws.w_scrbufpp[chp->ch_id];
	} else {	/* KD_BUFTOSCR -- copy from buffer to screen */
		srcp = Kdws.w_scrbufpp[chp->ch_id];
		dstp = vp->v_scrp;
	}

	if (ATYPE(Kdws, MCAP_COLOR) && !VTYPE(V400 | V750))
		kdv_disp(0);
		
	/*
	 * If we are in here, we can only be copying from screen to buffer.
	 */
	if (tsp->t_scrsz > avail) {	/* we must wrap, copy first chunk */
		kdv_xfer(srcp, dstp, avail, DOWN);
		srcp = vp->v_scrp;
		dstp += avail;
		avail = tsp->t_scrsz - avail;
	} else
		avail = tsp->t_scrsz;

	kdv_xfer(srcp, dstp, avail, DOWN);

	if (dir != KD_SCRTOBUF)	{	/* clean the screen */
		tmp = KD_MAXSCRSIZE;
		srcp = Kdws.w_scrbufpp[chp->ch_id];
		while (--tmp)
			*srcp = (NORM << 8 | ' ');
	}

	if (ATYPE(Kdws, MCAP_COLOR) && !VTYPE(V400 | V750))
		kdv_disp(1);

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

}


/*
 * void
 * kdv_textmode(ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *
 * Description:
 *	Restore the adapter to its default text mode. This produces
 *	a reasonable text display when switching from graphics to 
 *	text mode.
 */
void
kdv_textmode(ws_channel_t *chp)
{
	vidstate_t	*vp = &chp->ch_vstate;
	termstate_t	*tsp = &chp->ch_tstate;
	int		noscreen;	/* no previous screen to reload? */
	unchar		newmode;	/* new text mode */
	boolean_t	got_con = B_FALSE;		/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */

	noscreen = !WSCMODE(vp)->m_font;	/* no screen to restore? */
	newmode = noscreen ? Kdws.w_vstate.v_dvmode : vp->v_cvmode;

#ifdef EVGA
	evga_ext_rest(cur_mode_is_evga);
#endif /* EVGA */

	/*
	 * Change to new mode.
	 */
	kdv_setdisp(chp, vp, tsp, newmode);

#ifdef EVGA
	evga_ext_init(vp->v_cvmode);
#endif /* EVGA */

	if (noscreen)		/* no existing screen to restore? */
		kdclrscr(chp, 0, tsp->t_scrsz);
	else if (chp == ws_activechan(&Kdws)) {	/* existing and active? */
		kdv_scrxfer(chp, KD_BUFTOSCR);	/* reload screen buffer */
		kdsetcursor(chp, tsp);		/* position cursor properly */
	}

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */
}


/*
 * void
 * kdv_text1(ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *	- Only the active channel is allowed to read the state
 *	  of the cursor. 
 *
 * Description:
 *	Set up the tcl structure by reading the cursor address from the
 *	display adapter.  
 *
 * Note:
 *	Called when display is really in text mode but the driver has
 *	been setup to think that its in graphics mode- eg. VP/ix
 */
void
kdv_text1(ws_channel_t *chp)
{
	vidstate_t	*vp = &chp->ch_vstate;
	termstate_t	*tsp = &chp->ch_tstate;
	ushort		cursor, origin, port, row, col;
	int		efl;
	boolean_t	got_con = B_FALSE;			/* L001 */


	if (chp != ws_activechan(&Kdws))
		return;

	if(!kd_con_acquired)					/* L001 begin */
		if(ws_con_need_text())
			return;
		else{
			kd_con_acquired = B_TRUE;
			got_con = B_TRUE;
		}						/* L001 end */

	/* i/o address of the adapter */
	port = vp->v_regaddr;

	/*
	 * Block all interrupts (both device and 
	 * inter-processor interrupts).
	 */
	efl = intr_disable();
	
	/*
	 * Read the current cursor position. 
	 */
	outb(port, R_CURADRH);
	cursor = (inb(port + DATA_REG) << 8);
	outb(port, R_CURADRL);
	cursor += (inb(port + DATA_REG) & 0xff);

	/*
	 * Read the current origin. 
	 */
	if (DTYPE(Kdws, KD_EGA) || DTYPE(Kdws, KD_VGA)) {
		outb(port, R_STARTADRH);
		origin = (inb(port + DATA_REG) << 8);
		outb(port, R_STARTADRL);
		origin += (inb(port + DATA_REG) & 0xff);
	} else
		origin = 0;

	/*
	 * Unblock all interrupts (both device and 
	 * inter-processor interrupts).
	 */ 
	intr_restore(efl);

	/*
	 * Calculate the cooresponding tcl row and 
	 * column representation 
	 */
	row = ((ushort) (cursor - origin) / tsp->t_cols);
	col = cursor - origin - (row * tsp->t_cols);

	/*
	 * Update the tcl. 
	 */
	tsp->t_row = row;
	tsp->t_col = col;
	tsp->t_cursor = cursor;
	tsp->t_origin = origin;
	vp->v_font = 0;

#ifdef EVGA
	evga_ext_rest(cur_mode_is_evga);
#endif /* EVGA */

	kdv_setdisp(chp, vp, tsp, vp->v_cvmode);

#ifdef EVGA
	/*
	 * New mode shouldn't be evga but have this here
	 * in case any evga text modes are ever added.
	 */
	evga_ext_init(vp->v_cvmode);
#endif /* EVGA */

	kdsetcursor(chp, tsp);
	kdv_enable(vp);

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

}


STATIC struct b_param	*bios_bparam;
STATIC struct b_param	*cas2_bparam;
STATIC int		biosbparamflg = 0;
STATIC int		cas2bparamflg = 0;

/*
 * STATIC struct b_param *
 * kdv_getparam(unchar)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared/exclusive mode.
 */
STATIC struct b_param *
kdv_getparam(unchar mode)
{
 	struct b_param		*initp;
	struct modeinfo		*modep;
	vidstate_t		*vp = &Kdws.w_vstate;
	boolean_t	got_con = B_FALSE;		/* L001 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L001 end */

	modep = WSMODE(vp, mode);

        switch (modep->m_params) {
	case KD_BIOS:
		/*
		 * Locate the video mode parameter table.
		 */

		/*
		 * XXX:
		 *	initp = (struct b_param *)(
		 *		phystokv(ftop((ulong)*vp->v_parampp))) + 
		 *		modep->m_offset;
		 */
		if (!biosbparamflg) {
			bios_bparam = (struct b_param *)(
				physmap(kdv_ftop((ulong)*vp->v_parampp),
				sizeof(struct b_param) * 31, KM_NOSLEEP)); 
			if (!bios_bparam)
				/*
				 *+ Unable to physmap bios video mode 
				 *+ parameters.
				 */
				cmn_err(CE_PANIC,
					"could not allocate memory");
			biosbparamflg = 1;
		}

		initp = (struct b_param *)(bios_bparam) + modep->m_offset;
		break;

	case KD_CAS2:
		/*
		 * Locate the extended video mode standard parameter table. 
		 */

		/*
		 * XXX: 
		 *	initp = (struct b_param *)(
		 *		phystokv(ftop((ulong)*(vp->v_parampp-0x3)))) + 
		 *		modep->m_offset;
		 */
		if (!cas2bparamflg) {
			cas2_bparam = (struct b_param *)(
				physmap(kdv_ftop((ulong)*(vp->v_parampp - 0x3)),
				sizeof(struct b_param) * 23, KM_NOSLEEP));
			if (!cas2_bparam)
				/*
				 *+ Unable to physmap extended video mode 
				 *+ parameters.
				 */
				cmn_err(CE_PANIC,
					"could not allocate memory");
			cas2bparamflg = 1;
		}

		initp = (struct b_param *)(cas2_bparam) + modep->m_offset;
		break;

	default:	/* KD_TBLE */
		initp = &kd_inittab[modep->m_offset];
		break;
	}

	if(got_con){					/* L001 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L001 end */

	return (initp);
}


/*
 * int
 * kdv_setxenixfont(ws_channel_t *, int, caddr_t)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Description:
 *	This routine is called during processing of the 
 *	PIO_FONT* ioctls that are the SCO-compatible interface
 *	for changing the font for a particular character
 *	box size. The newfont is either a pointer to the
 *	kernel copy of the user font information or NULL
 *	to reset back to the ROM font.
 */
/* ARGSUSED */
int
kdv_setxenixfont(ws_channel_t *chp, int font, caddr_t useraddr)
{
	int		bpc;		/* Bytes per character */
	int		size;		/* Size of data area */
	unchar		*curfont, *oldfont;
	ws_channel_t	*tchp;
	vidstate_t	*vp = &Kdws.w_vstate;	/* Global video information */
	pl_t		pl;


	switch (font) {
	case FONT8x8:
		bpc = F8x8_BPC;
		size = F8x8_SIZE;
		curfont = vp->v_fontp[FONT8x8].f_fontp;
		break;

	case FONT8x14:
		bpc = F8x14_BPC;
		size = F8x14_SIZE;
		curfont = vp->v_fontp[FONT8x14].f_fontp;
		break;

	case FONT8x16:
		bpc = F8x16_BPC;
		size = F8x16_SIZE;
		curfont = vp->v_fontp[FONT8x16].f_fontp;
		break;

	default:
		return (EINVAL);
	}

	/*  
	 * Check to see if the default font is the ROM font.
	 * If so, allocate space for new font. 
	 */
	oldfont = curfont;
	if (kd_romfonts[font].f_fontp == curfont) { 
		/*
		 * Fail ioctl if useraddr is NULL -- no XENIX 
		 * font to release. 
		 */
		if (useraddr == NULL)
			return (EFAULT);
		curfont = (unchar *) kmem_alloc(size, KM_NOSLEEP);
		if (curfont == (unchar *) NULL)
			return (ENOMEM);
	} else { 
		/* 
		 * Maybe a reset to the ROM font if useraddr
		 * is NULL.
		 */
		if (useraddr == NULL) {
			curfont = kd_romfonts[font].f_fontp;
			bpc = kd_romfonts[font].f_bpc;
			size = kd_romfonts[font].f_count * bpc;
		}
	}

	/*
	 * Copyin new font.
	 */

	/* release the workstation read/write lock before copyin */
	RW_UNLOCK(Kdws.w_rwlock, (pl = getpl()));

	ASSERT(Kdws.w_flags & WS_NOFONTMOD);
	ASSERT(getpl() == plstr);

	if (useraddr != NULL && (copyin(useraddr, curfont, size) == -1)) {

		pl = RW_WRLOCK(Kdws.w_rwlock, pl);	/* L004 */
		return (EFAULT);
	}

	pl = RW_WRLOCK(Kdws.w_rwlock, pl);

	/*
	 * Free current font info if useraddr is NULL. 
	 */
	if (useraddr == NULL) {
		struct font_info *fp;

		fp = &vp->v_fontp[font];
		kmem_free(fp->f_fontp, fp->f_count * fp->f_bpc);
	}

	/*
	 * Update global workstation vid structure. 
	 */
	vp->v_fontp[font].f_fontp = curfont;
	vp->v_fontp[font].f_count = size/bpc;

	/*
	 * If the font has been modified using the PIO_ROMFONT
	 * ioctl, where individual changes are overlaid on top
	 * of the ROM font, free the kd_romfont_mod structure
	 * that describes those modifications.
	 */
	if (kd_romfont_mod)
		kdv_release_fontmap();

	if (oldfont != curfont) {
		int i;

		/*
		 * Update channels pointing to oldfont.
		 */
		for (i = 0; i < WS_MAXCHAN; i++) {
			tchp = ws_getchan(&Kdws, i);
			if (!tchp)
				continue;
			if (tchp->ch_vstate.v_fontp[font].f_fontp != oldfont)
				continue;
			if (tchp == ws_activechan(&Kdws))
				continue;	/* skip for the moment */
			tchp->ch_vstate.v_fontp[font].f_fontp = curfont;
			tchp->ch_vstate.v_fontp[font].f_count = size / bpc;
		}
	}
	
	/*
	 * Now do active channel. Font changed for it if its
	 * font points to oldfont.
	 */

	tchp = ws_activechan(&Kdws);
	vp = &tchp->ch_vstate;
	if (vp->v_fontp[font].f_fontp != oldfont && 
			vp->v_fontp[font].f_fontp != curfont) {
		return (0);
	}

	/*
	 * Reset font information on channel. 
	 */
	vp->v_fontp[font].f_fontp = curfont;
	vp->v_fontp[font].f_count = size / bpc;

	if (WSCMODE(vp)->m_font == 0) {		/* graphics mode */
		return (0);
	}

	kdv_ldfont(vp, WSCMODE(vp)->m_font);

	return (0);
}


/*
 * int
 * kdv_getxenixfont(ws_channel_t *, int, caddr_t)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 * 
 * Description:
 *	Function to get the font for the given font type
 *	according to XENIX ioctl interface. Note that we
 *	always get the default font, never the per-VT font.
 *	Also note that the any overlay modification to the
 *	font supplied via PIO_ROMFONT ioctl is also sent
 *	to the user. It is called while processing the 
 *	GIO_FONT* ioctl.
 */
/* ARGSUSED */
int
kdv_getxenixfont(ws_channel_t *chp, int font, caddr_t useraddr)
{
	int		size;			/* No of bytes per character */
	unchar		*from;			/* Address to copy from */
	vidstate_t	*vp = &Kdws.w_vstate;	/* Video information */
	int		bpc;			/* bytes per character */
	pl_t		pl;


	/* Check for duff argument */
	if (useraddr == NULL)
		return (EINVAL);

	/*
	 * Decide where we're going to copy from and how much.
	 */
	switch (font) {
	case FONT8x8:
	case FONT8x16:
	case FONT8x14:
		from = vp->v_fontp[font].f_fontp;
		bpc = vp->v_fontp[font].f_bpc;
		size = vp->v_fontp[font].f_count * bpc;
		break;
	default:
		return (EINVAL);
	}

	/* release the workstation read/write lock before copyout */
	RW_UNLOCK(Kdws.w_rwlock, plbase);

	ASSERT(Kdws.w_flags & WS_NOFONTMOD);

	/* Copy the data */
	if (copyout(from, useraddr, size) == -1) {
		pl = RW_WRLOCK(Kdws.w_rwlock, plstr);
		return (EFAULT);
	}
	pl = RW_WRLOCK(Kdws.w_rwlock, plstr);

	if (kd_romfont_mod) {
		int i;
		unchar *from, *to;
		rom_font_t *rfp;

		/*
		 * Follow overlay list and copyup appropriate overlays.
		 */

		/* LINTED pointer alignment */
		rfp = (rom_font_t *)kd_romfont_mod;

		for (i = 0; i < rfp->fnt_numchar; i++) {
			/* LINTED pointer alignment */
			from = (unchar *)&rfp->fnt_chars[i];
			from += kd_font_offset[font];
			to = (unchar *) useraddr + bpc * rfp->fnt_chars[i].cd_index;
			RW_UNLOCK(Kdws.w_rwlock, pl);
			ASSERT(Kdws.w_flags & WS_NOFONTMOD);
			if (copyout(from, to, bpc) == -1) {
				pl = RW_WRLOCK(Kdws.w_rwlock, plstr);
				return (EFAULT);
			}
			pl = RW_WRLOCK(Kdws.w_rwlock, plstr);
		}
	}

	return (0);
}


/*
 * int
 * kdv_modromfont(caddr_t, unsigned int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Description:
 *	This routine is called while processing the
 *	PIO_ROMFONT ioctl. It resets the font back to
 *	the ROM font, and copies in the user-supplied
 *	rom_font_t structure, and sets kd_romfont_mod
 *	pointing to it. If the argument to the ioctl
 *	is NULL rather than a user pointer to a
 *	rom_font_t structure, the kd_romfont_mod
 *	structure is released, and the ROM font
 *	becomes the default font.
 */
/* ARGSUSED */
int
kdv_modromfont(caddr_t buf, unsigned int numchar)
{
	int		font;
	vidstate_t	*vp = &Kdws.w_vstate;
	ws_channel_t	*tchp;


	if (vp->v_fontp[FONT8x8].f_fontp != kd_romfonts[FONT8x8].f_fontp)
		kdv_setxenixfont(ws_activechan(&Kdws), FONT8x8, NULL);
	if (vp->v_fontp[FONT8x14].f_fontp != kd_romfonts[FONT8x14].f_fontp)
		kdv_setxenixfont(ws_activechan(&Kdws), FONT8x14, NULL);
	if (vp->v_fontp[FONT8x16].f_fontp != kd_romfonts[FONT8x16].f_fontp)
		kdv_setxenixfont(ws_activechan(&Kdws), FONT8x16, NULL);

	if (kd_romfont_mod) {
		int	size;
		caddr_t	oldbuf;
		rom_font_t *rfp;

		/* LINTED pointer alignment */
		rfp = (rom_font_t *)kd_romfont_mod;
		size = rfp->fnt_numchar * sizeof(struct char_def)
					+ sizeof(numchar);
		oldbuf = kd_romfont_mod;
		ASSERT(getpl() == plstr);
		kd_romfont_mod = buf;
		kmem_free(oldbuf, size);
	} else
		kd_romfont_mod = buf;

	ASSERT(getpl() == plstr);
	tchp = ws_activechan(&Kdws);
	vp = &tchp->ch_vstate;
	font = WSCMODE(vp)->m_font;
	if (font == 0)		/* graphics mode */
		return (0);

	/* Does VT have its own font? */
	if (vp->v_fontp[font].f_fontp != kd_romfonts[font].f_fontp)
		return (0);

	kdv_ldfont(vp, font);

	return (0);
}


/*
 * int
 * kdv_release_fontmap(void)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive mode.
 *
 * Description:
 *	This routine is called while processing the WS_PIO_ROMFONT
 *	ioctl to unload the modified ROM font. It will reload the
 *	ROM font if the fonts are not modified per channel.
 */
int
kdv_release_fontmap(void)
{
	int		font;
	ws_channel_t	*tchp;
	vidstate_t	*vp;


	if (kd_romfont_mod) {
		int	size;
		caddr_t oldbuf;
		rom_font_t *rfp;

		/* LINTED pointer alignment */
		rfp = (rom_font_t *)kd_romfont_mod;
		size = rfp->fnt_numchar * sizeof(struct char_def) + sizeof(int);
		oldbuf = kd_romfont_mod;
		ASSERT(getpl() == plstr);
		kd_romfont_mod = NULL;
		kmem_free(oldbuf, size);
	} else 
		return (0);

	ASSERT(getpl() == plstr);
	tchp = ws_activechan(&Kdws);
	vp = &tchp->ch_vstate;
	font = WSCMODE(vp)->m_font;
	if (font == 0)		/* graphics mode */
		return (0);

	/* Does VT have its own font? */
	if (vp->v_fontp[font].f_fontp != kd_romfonts[font].f_fontp)
		return (0);

	kdv_ldfont(vp, font);

	return (0);
}
