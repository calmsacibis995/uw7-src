#ident	"@(#)kdstr.c	1.33"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 */

/*
 * Modification history
 *
 *	L000	23Jan97		rodneyh@sco.com
 *	- Changes for Gemini multiconsole support
 *	  Bracketed calls to tcl_norm with calls to ws_con_need_text and
 *	  ws_con_done_text
 *	L001	7Mar97		rodneyh@sco.com
 *	- Moved KDSETFONT and KDGETFONT ioctls from here into kdvmstr.c,
 *	  kdvmstr_doioctl().
 *	- Removed compiler warnings from initialisation of Kdwsops structure.
 *	L002	30Apr97		rodneyh@sco.com
 *	- Added KDSETLCK ioctl. Used to control the lock key states on all VT's.
 *	L003	2May97		rodneyh@sco.com
 *	- Changed KDSETLED ioctl to call kdkb_update_kbstate() in order to have
 *	  the preserved LED state updated if we are not the active channel.
 *	L004	5Jun97		rodneyh@sco.com
 *	- kdinit now turns of 8042 auxiliary port if there is one.
 *	L005	16Jul97		rodneyh@sco.com
 *	- Added KDNOAUTOFOCUS to kdmioctlmsg() to control the state of the
 *	  kd_no_activate flag. kd_no_activate equal B_TRUE prevents focus
 *	  automatically shifting when a new VT is opened.
 *
 */

/*
 * Keyboard/Display Driver - STREAMS
 */

#include <io/ansi/at_ansi.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/conssw.h>
#include <io/gvid/vdc.h>
#include <io/gvid/vid.h>
#include <io/kd/kb.h>
#include <io/kd/kd.h>
#include <io/ioctl.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <io/termios.h>
#include <io/ws/8042.h>
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
#include <proc/tss.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/kdb/xdebug.h> 
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>


#define	KDHIER		1
#define	KDPL		plstr	

#ifdef TIMEOUT_KDINTR

#define KDSCANQSIZE	256

/*
 * MACRO
 * KDNEXT(struct kdscanq scanq)
 *	Index to the next scan queue slot.
 *
 * Calling/Exit State:
 *	Called from within the scanq macros.
 */
#define KDNEXT(x)	(((x) < (KDSCANQSIZE - 1)) ? ((x) + 1) : 0)

/*
 * MACRO
 * KDSCANQEMPTY(struct kdscanq scanq)
 *	Is scan queue empty?
 *
 * Calling/Exit State:
 *	Called from kdprocscan() 
 */
#define KDSCANQEMPTY(scanq) \
		((scanq)->sq_put == (scanq)->sq_get)

/*
 * MACRO
 * KDSCANQFULL(struct kdscanq scanq)
 *	Is scan queue full?
 *
 * Calling/Exit State:
 *	Called from kdprocscan() 
 */
#define KDSCANQFULL(scanq) \
		(KDNEXT((scanq)->sq_put) == (scanq)->sq_get)

/*
 * MACRO
 * KDPUTSCAN(struct kdscanq scanq, unchar rscan)
 *	Enqueue the scancode. 
 *
 * Calling/Exit State:
 *	Called from the interrupt handler.
 *
 * Remarks:
 *	If the put index is one greater than the get index (put + 1) == (get),
 *	then the buffer is marked full, otherwise space is available to
 *	enqueue the scancode.
 */
#define KDPUTSCAN(scanq, rscan) { \
		ASSERT(!(KDSCANQFULL((scanq)))); \
		(scanq)->sq_buf[(scanq)->sq_put] = (rscan); \
		(scanq)->sq_put = KDNEXT((scanq)->sq_put); \
		ASSERT(!(KDSCANQEMPTY((scanq)))); \
}

/*
 * MACRO
 * KDGETSCAN(struct kdscanq scanq, unchar rscan)
 *	Dequeue the scancode. 
 *
 * Calling/Exit State:
 *	Called from kdprocscan() 
 */
#define KDGETSCAN(scanq, rscan) { \
		(rscan) = (scanq)->sq_buf[(scanq)->sq_get]; \
		(scanq)->sq_get = KDNEXT((scanq)->sq_get); \
}

struct kdscanq {
	unchar	sq_buf[KDSCANQSIZE];	/* buffer to store scancodes */
	int	sq_put;			/* pointer to the next put location */ 
	int	sq_get;			/* pointer to the next get location */
	toid_t	sq_timeid;		/* timeout id */
} kdscanq;

#endif /* TIMEOUT_KDINTR */

int		kdclrscr(ws_channel_t *, ushort, int);
int		kdsetbase(ws_channel_t *, termstate_t *);
int		kdtone(wstation_t *, ws_channel_t *);
int		kdshiftset(wstation_t *, ws_channel_t *, int);
int		kdsetcursor(ws_channel_t *, termstate_t *);
int		kdnoop(void);
void		kdnotsysrq(kbstate_t *, int);

STATIC int	kdopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	kdclose(queue_t *, int, int, cred_t *);
STATIC int	kdwsrv(queue_t *);

STATIC void	kdmioctlmsg(queue_t *, mblk_t *);
STATIC void	kdproto(queue_t *, mblk_t *);
STATIC void	kdmiocdatamsg(queue_t *, mblk_t *);
STATIC int	kdcksysrq(charmap_t *, kbstate_t *, ushort, unchar);
STATIC void	kdconfigure(void);


extern void	kdv_init(ws_channel_t *);
extern void	kdv_setdisp(ws_channel_t *, vidstate_t *, termstate_t *, unchar);
extern int	kdv_stchar(ws_channel_t *, ushort, ushort, int);
extern int	kdv_mvword(ws_channel_t *, ushort, ushort, int, char);
extern int	kdv_shiftset(vidstate_t *, int);
extern int	kdv_undattrset(wstation_t *, ws_channel_t *, ushort *, short);
extern int	kdv_cursortype(wstation_t *, ws_channel_t *, termstate_t *);
extern int	kdv_sborder(ws_channel_t *, long);
#ifdef EVGA
extern void	evga_ext_rest(unchar);
extern void	evga_ext_init(unchar);
#endif /* EVGA */

extern void	kdvmstr_ioctl(queue_t *, mblk_t *, 
					struct iocblk *, ws_channel_t *);
extern int	kdvm_unmapdisp(ws_channel_t *, struct map_info *);

extern int	kdvt_open(ws_channel_t *, pid_t);
extern int	kdvt_close(ws_channel_t *);
extern void	kdvt_switch(ushort, pl_t);
extern int	kdvt_activate(ws_channel_t *, int);
extern int	kdvt_rel_refuse(void);
extern int	kdvt_acq_refuse(ws_channel_t *);

extern struct conssw kdconssw;

struct module_info
	kd_info = { 42, "kd", 0, 32, 256, 128 };

static struct qinit
	kd_rinit = { NULL, NULL, kdopen, kdclose, NULL, &kd_info, NULL };

static struct qinit
	kd_winit = { putq, kdwsrv, kdopen, kdclose, NULL, &kd_info, NULL };

struct streamtab
	kdinfo = { &kd_rinit, &kd_winit, NULL, NULL };

struct kdptrs {
	ws_channel_t	*k_chanpp[WS_MAXCHAN+1];
	charmap_t	*k_charpp[WS_MAXCHAN+1];
	ushort		*k_scrnpp[WS_MAXCHAN+1];
} Kdptrs = { 0 };

wstation_t	Kdws = {0};		/* wstation structure definition */
ws_channel_t	Kd0chan = {0};		/* channel 0 structure definition */
unchar		Kd0tabs[ANSI_MAXTAB];
int		kb_raw_mode = KBM_XT;
int		kd_wsinitflag = 0;
struct ext_graph kd_vdc800 = {0}; 	/* vdc800 hook structure */
uint_t		kdmontype;
boolean_t	kd_con_acquired = B_FALSE;	/* L000 */
/*
 * We needed to bind kd to processor 0 because we were having problems
 * with the PS/2 mouse. The reason being that the PS/2 mouse is single-
 * threaded and the kd is multi-threaded and since they use the same
 * hardware interface to access the device, there were intermittent 
 * mouse failures. However, the side-effect of binding kd was that the
 * the X server and all its children were now forced to also run on 
 * processor 0 (see drv_mmap/drv_unmap). In order to avoid performance
 * problem, we mark kd, kdvm and gvid drivers as single-threaded instead
 * of binding to processor 0. It will have the side-effect of keeping 
 * interrupts bound to processor 0 and will have a performance benefit
 * in that STREAMS will no longer need to stick a uniplexor (uniprocessor
 * compatibility module) between cmux and kd.
 *
 * Note: If cmux and PS/2 mouse drivers are multithreaded, then kd, kdvm
 * and gvid drivers xxxdevflag must be set to D_MP.
 */
int		kddevflag = D_MP;	/* We are new-style multithreaded */
					/* ES/MP driver */
kdcnops_t Kdwsops = {
		kdv_stchar,		/* cn_stchar */
		kdclrscr,		/* cn_clrscr */
		kdsetbase,		/* cn_setbase */
		kdvt_activate,		/* cn_activate */
		kdsetcursor,		/* cn_setcursor */
		kdtone,			/* cn_bell */
		kdv_sborder,		/* cn_setborder */
		kdshiftset,		/* cn_shiftset */
		kdv_mvword,		/* cn_mvword */
		kdv_undattrset,		/* cn_undattr */
		kdvt_rel_refuse,	/* cn_rel_refuse */
		kdvt_acq_refuse,	/* cn_acq_refuse */
		kdkb_scrl_lock,		/* cn_scrllck */
		kdv_cursortype,		/* cn_cursortype */
		kdvm_unmapdisp,		/* cn_unmapdisp */
#ifndef NO_MULTI_BYTE
		(void (*)(struct kdcnops *,
				ws_channel_t *,
				termstate_t *,
				ushort))kdnoop,		/* cn_gcl_norm */
		(void (*)())kdnoop,			/* cn_gcl_handler */
		(void (*)())kdnoop,			/* cn_gdv_xfer */
		(int (*)(struct kdcnops *,
				ws_channel_t *,
				termstate_t *))kdnoop,	/* cn_gs_alloc */
		(void (*)())kdnoop,			/* cn_gs_free */
		(void (*)())kdnoop,			/* cn_gs_chinit */
		(int (*)(ws_channel_t *,
				struct eucioc *))kdnoop,/* cn_gs_seteuc */
		(int (*)(struct kdcnops *,
				ws_channel_t *,
				termstate_t *,
				ushort))kdnoop,		/* cn_gs_ansi_cntl */
#endif /* NO_MULTI_BYTE */
};

LKINFO_DECL(kd_w_rwlock_lkinfo, "KD:WS:workstation rwlock lock", 0);
LKINFO_DECL(kd_w_mutex_lkinfo, "KD:WS:workstation mutex lock", 0);
LKINFO_DECL(kd_ch_mutex_lkinfo, "KD:CH:channel mutex lock", 0);
LKINFO_DECL(kd_cr_mutex_lkinfo, "KD:CR:charmap mutex lock", 0);
LKINFO_DECL(kd_scr_mutex_lkinfo, "KD:SCR:scrn mutex lock", 0);

extern struct font_info fontinfo[];
extern ushort kd_iotab[][MKDBASEIO];
extern struct b_param kd_inittab[];
extern struct reginfo kd_regtab[];
extern unchar kd_ramdactab[];
extern uchar_t *egafont_p[5];
extern struct cgareginfo kd_cgaregtab[];
extern struct m6845init kd_cgainittab[];
extern struct m6845init kd_monoinittab[];
extern long kdmonitor[];
extern char kd_swmode[];
extern struct attrmask kb_attrmask[];
extern int nattrmsks;
extern struct vdc_info Vdc;

#ifdef EVGA
extern int evga_inited;
extern int evga_mode;
extern unchar saved_misc_out;
extern struct at_disp_info disp_info[];
#endif	/* EVGA */


/*
 * STATIC void
 * kddoarg(void)
 *
 * Calling/Exit State:
 *	- No locking assumptions.
 *	- Called from kdinit() before kdv_init() so that monitor type
 *	  autodetection can be overwritten by the KDMONITOR value.
 * 
 * Description:
 *	Parse kd boot arguments. Currently, only the KDMONITOR parameter
 *	with values MONO and COLOR is supported. These values are specified
 *	by the user thru the interactive boot or resmgr file.
 */
STATIC void
kddoarg(void)
{
	int	num;			/* number of boards */
	int	rv;			/* return value */
	int	i;			/* index */
	cm_args_t cma;			/* argument to cm_xxx */
	rm_key_t key;			/* key to configutation params */
	char	montype[16];		/* monitor type -- color or mono */

	
	num = cm_getnbrd("kd");
	cma.cm_n = 0;
	cma.cm_param = "KDMONITOR";
	cma.cm_val = montype;
	cma.cm_vallen = sizeof(montype);
	for (i = 0; i < num; i++) {
		cma.cm_key = cm_getbrdkey("kd", i);
		if ((rv = cm_getval(&cma)) == 0) {
			if (strcmp(cma.cm_val, "MONO") == 0) {
				kdmontype = KD_STAND_M;
				break;
			} else if (strcmp(cma.cm_val, "COLOR") == 0) {
				kdmontype = KD_STAND_C;
				break;
			}
		}
	}
}


/*
 * STATIC void
 * kdconfigure(void)
 *
 * Calling/Exit State:
 *	- No locking assumptions.
 *	- Called from kdinit().
 *
 * Description:
 *	Register resources with the resmgr that are dynamically configurable.
 *	We configure the video bios base address by rounding down the video
 *	parameter table physical address to the 32K boundary.
 */
STATIC void
kdconfigure(void)
{
	int	num;			/* number of boards */
	int	rv;			/* return value */
	int	i;			/* index */
	cm_args_t cma;			/* argument to cm_xxx */
	cm_range_t vbiosaddr;		/* video bios address */
	cm_range_t val;			/* default video bios address */
	paddr_t	physaddr;		/* video parameter phys. addr */
	rm_key_t key;			/* key to configutation params */
	vidstate_t *vp = &Kdws.w_vstate;
	boolean_t found = B_FALSE;	/* flag to indicate a key was 
					 * found w/out CM_MEMADDR param
					 */
	
	/*
	 * Immediately return if the video mode parameter table are
	 * NOT in the bios area.
	 */
	if (!(vp->v_parampp))
		return;

	physaddr = vtop((caddr_t)vp->v_parampp, 0);
	ASSERT(physaddr < 0x100000);
	/* Round down to 32K boundary. */
	vbiosaddr.startaddr = physaddr & 0x8000 ? 
			      physaddr & 0xf8000 : 
			      physaddr & 0xf0000;
	vbiosaddr.endaddr = vbiosaddr.startaddr + 0x7fff;

	/*
	 * Cannot specify the video bios address in the MEMADDR field of the
	 * first key because it is used to reserve RAM space for the video 
	 * buffer and we do not support multi-valued sdevice enteries. So we
	 * have to search for the key with an uninitialized MEMADDR param.
	 */

	num = cm_getnbrd("kd");
	cma.cm_n = 0;
	cma.cm_param = CM_MEMADDR;
	cma.cm_val = &val;
	cma.cm_vallen = sizeof(val);
	for (i = 0; i < num; i++) {
		cma.cm_key = cm_getbrdkey("kd", i);
		if ((rv = cm_getval(&cma)) == 0) {
			if (val.startaddr == vbiosaddr.startaddr &&
			    val.endaddr == vbiosaddr.endaddr) {
				return;
			} else {
				continue;
			}
		} else if (rv == ENOENT && found == B_FALSE) {
			/* Save the first key without a CM_MEMADDR param. */
			key = cma.cm_key;
			found = B_TRUE;
		}
	}

	if (found == B_TRUE) {
		cma.cm_key = key;
		cma.cm_val = &vbiosaddr;
		cma.cm_vallen = sizeof(vbiosaddr);
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Failed to register video bios memory resources with 
			 *+ the in-core resource manager (resmgr) database.
			 */
			cmn_err(CE_WARN,
				"!kdconfigure: cm_addval failed with rv = 0x%x", rv);
		}
	}

	return;
}


/*
 * STATIC int 
 * kdchaninit(wstation_t *, ws_channel_t *, uchar_t *, int)
 *
 * Calling/Exit State:
 *	- called from kdinit() and kdopen().
 *	- workstation read/write lock (w_rwlock) is held in exclusive mode.
 *
 * Description:
 *	Allocate and initialize the channel data structure.
 *
 * Remarks:
 *	The charmap and scrnmap tables for all the channels are protected
 *	by a single spin lock (cr_mutex) initialized in the wstation_t. 
 */
STATIC int 
kdchaninit(wstation_t *wsp, ws_channel_t *chp, uchar_t *tabsp, int indx)
{
	/*
	 * Initialize channels simple locks and sync. variables.
	 */
	chp->ch_mutex = LOCK_ALLOC(KDHIER+1, KDPL, 
					&kd_ch_mutex_lkinfo, KM_NOSLEEP);
	chp->ch_wactsv = SV_ALLOC(KM_NOSLEEP);
	chp->ch_xquesv = SV_ALLOC(KM_NOSLEEP);
	chp->ch_qrsvsv = SV_ALLOC(KM_NOSLEEP);
	if (!chp->ch_mutex || !chp->ch_wactsv || !chp->ch_xquesv ||
	    !chp->ch_qrsvsv) 
		return (ENOMEM);

	/*
	 * Allocate tab stop information for a channel and 
	 * set the channel's termstate structure to point to it.
	 */
	if (tabsp == NULL) {
		if (!(tabsp = (unchar *) kmem_zalloc(
				ANSI_MAXTAB, KM_NOSLEEP)))
			return (ENOMEM);
	}
	chp->ch_tstate.t_tabsp = tabsp;

	/*
	 * Allocate channel attributes and initialize the channel.
	 */
	if (ws_alloc_attrs(wsp, chp, KM_NOSLEEP))
		return (ENOMEM);
	ws_chinit(wsp, chp, indx);

	/*
	 * Allocate charmap table. Note that the charmap mutex lock is not
	 * allocated because the translation tables are mutexed by the spin
	 * lock in the workstation default charmap table.
	 */
	if ((chp->ch_charmap_p = ws_cmap_alloc(
				wsp, KM_NOSLEEP)) ==  (charmap_t *) NULL) 
		return (ENOMEM);

	/*
	 * Allocate screen map table. Note that the scrn map mutex lock 
	 * is not allocated because the scrnmap tables are mutexed by the
	 * spin lock in the workstation default scrn map table.
	 */
	ws_scrn_alloc(wsp, chp); 

	return (0);
}


/*
 * void
 * kdinit(void)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	Allocate and initialize wstation and channel zero.
 */
void
kdinit(void)
{
	pl_t	opl;
	extern boolean_t kdcninitflag;
	ushort_t row, col, cursor, origin;
	boolean_t got_con = B_FALSE;			/* L000 */

	if (WS_ISINITED(&Kdws))
		return;

	if(!kd_con_acquired)				/* L000 begin */
		if(ws_con_need_text())
			return;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L000 end */

	opl = splhi();

	/*
	 * Driver is being initialized.
	 */
	Kdws.w_init = WS_ININIT;

	/*
	 * Parse any boot arguments.
	 */
	kddoarg();

	/*
	 * Initialize workstation simple locks and sync. variables.
	 */
	Kdws.w_rwlock = RW_ALLOC(KDHIER, KDPL, 
					&kd_w_rwlock_lkinfo, KM_NOSLEEP);
	Kdws.w_mutex = LOCK_ALLOC(KDHIER+2, KDPL, 
					&kd_w_mutex_lkinfo, KM_NOSLEEP);
	Kdws.w_charmap.cr_mutex = LOCK_ALLOC(KDHIER+2, KDPL, 
					&kd_cr_mutex_lkinfo, KM_NOSLEEP);
	Kdws.w_scrn.scr_mutex = LOCK_ALLOC(KDHIER+2, KDPL,
					&kd_scr_mutex_lkinfo, KM_NOSLEEP);
	Kdws.w_tonesv = SV_ALLOC(KM_NOSLEEP);
	Kdws.w_flagsv = SV_ALLOC(KM_NOSLEEP);
	if (!Kdws.w_rwlock || !Kdws.w_mutex || !Kdws.w_charmap.cr_mutex ||
	    !Kdws.w_scrn.scr_mutex || !Kdws.w_tonesv || !Kdws.w_flagsv) 
		/*
		 *+ There isn't enough memory available to allocate for
		 *+ workstations read/write lock or simple lock or 
		 *+ synchronization variables.
		 */
		cmn_err(CE_PANIC,
			"kdinit: out of memory for workstation locks amd svs");

	Kdws.w_consops = &Kdwsops;
	Kdws.w_active = 0;
	Kdws.w_ticks = 0;
	Kdws.w_switchto = (ws_channel_t *) NULL;

	/*
	 * Not used by KD, but initialize it anyway.
	 */
	Kdws.w_wsid = 0;
	Kdws.w_private = (caddr_t) NULL;

	Kdws.w_qp = (queue_t *) NULL;
	Kdws.w_mp = (mblk_t *) NULL;

	/*
	 * Save the cursor state (since console channel was
	 * partially initialized in kdcnopen) so that it can
	 * be restored later.
	 */
	if (kdcninitflag) {
		origin = Kd0chan.ch_tstate.t_origin;
		cursor = Kd0chan.ch_tstate.t_cursor; 
		row = Kd0chan.ch_tstate.t_row; 
		col = Kd0chan.ch_tstate.t_col; 
	}

	/*
	 * Perform video controller initialization.
	 */
	kdv_init(&Kd0chan);

	Kdws.w_chanpp = Kdptrs.k_chanpp;
	Kdws.w_scrbufpp = Kdptrs.k_scrnpp;

	ws_cmap_init(&Kdws, KM_NOSLEEP);
	ws_scrn_init(&Kdws, KM_NOSLEEP);

	/*
	 * Initialize Kd0chan and make it the active channel.
	 */

	if (kdchaninit(&Kdws, &Kd0chan, (uchar_t *)&Kd0tabs, 0) == ENOMEM) {
		/*
		 *+ Not enough memory to allocate for locks,
		 *+ synchronization variable, attributes or
		 *+ tab information. Check the memory 
		 *+ configured in the system.
		 */
		cmn_err(CE_PANIC,
			"kdinit: out of memory");
	}

	Kd0chan.ch_nextp = Kd0chan.ch_prevp = &Kd0chan;
	Kdws.w_chanpp[0] = &Kd0chan;
	Kdptrs.k_charpp[0] = Kd0chan.ch_charmap_p;

	/*
	 * Allocate screen buffer for channel 0 and call
	 * kdclrscr to clear the screen.
	 */

	if (!(Kdws.w_scrbufpp[0] = (ushort *) kmem_alloc(
			sizeof(ushort) * KD_MAXSCRSIZE, KM_NOSLEEP)))
		/*
		 *+ There isn't enough memory available to allocate
		 *+ for channel zero screen buffer.
		 */
		cmn_err(CE_PANIC, 
			"kdinit: out of memory for screen");

	if (!kdcninitflag) {
		kdclrscr(&Kd0chan, Kd0chan.ch_tstate.t_origin, 
			 Kd0chan.ch_tstate.t_scrsz);
		kdcninitflag = B_TRUE;
	} else {
		/*
		 * Restore the console channel cursor state.
		 */
		Kd0chan.ch_tstate.t_origin = origin;
		Kd0chan.ch_tstate.t_cursor = cursor; 
		Kd0chan.ch_tstate.t_row	= row; 
		Kd0chan.ch_tstate.t_col	= col;
	}

	if (Vdc.v_info.rsrvd) {
		/*
		 *+ There is a monitor detection mismatch between what
		 *+ was found using the DAC test and what was found using
		 *+ the BIOS call.
		 */
		cmn_err(CE_WARN,
			"!kdinit: Monitor type detection mismatch");
	}

	/*
	 * Initialize system resource database. For now reserve the
	 * video bios data area based on the location of video mode
	 * parameter table.
	 */
	kdconfigure();

	/*
	 * Driver is fully initialized.
	 */
	Kdws.w_init = WS_INITED;

	/* read scan data - clear possible spurious data */
	inb(KB_IDAT);

	/*
	 * If there is an auxiliary port on the 8042 disable it so that any
	 * interrupts from the aux port don't hang the device if there is no
	 * configured handler
	 */
	if(i8042_aux_port())				/* L004 */
		i8042_program(P8042_AUXDISAB);		/* L004 */	

	/* reset keyboard interrupts */
 	drv_setparm(SYSRINT, 1); 

	splx(opl); 

	if(got_con){					/* L000 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L000 end */

	return;
}


/*
 * void
 * kdstart(void)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	The kdstart() routine is used to initialize the
 *	keyboard. This is protected by holding the 
 *	w_rwlock in exclusive mode. This is to prevent
 *	servicing keyboard interrupts before the 
 *	keyboard state structure is equipped to 
 *	deal with it.
 */
void
kdstart(void)
{
        kbstate_t       *kbp;
        ws_channel_t       *chp;
	pl_t		pl;
	mblk_t		*bp;


	/*
	 * Do message block allocation before acquiring the w_rwlock, 
	 * since the allocb() routine lowers the ipl value to plstr. 
	 *
	 * In SVR4 the message block was allocated in kdinit(), but
	 * its moved here because according to DDI/DKI allocb() 
	 * cannot be called from an xxxinit() routine.
	 */
	if ((bp = allocb(4, BPRI_MED)) == (mblk_t *) NULL)
		/*
		 *+ There isn't enough memory available to allocate
		 *+ for workstation message block.
		 */
		cmn_err(CE_PANIC, 
			"kdstart: no msg blocks");

	/*
	 * Acquire the workstation read/write lock in exclusive mode.
	 */
	pl = RW_WRLOCK(Kdws.w_rwlock, plstr);

	Kdws.w_mp = bp;
        chp = ws_activechan(&Kdws);
        if (chp == (ws_channel_t *) NULL)
                chp = &Kd0chan;

        kbp = &chp->ch_kbstate;
	/* Initialize keyboard interface and enable keyboard interrupts. */
        kdkb_init(&Kdws.w_kbtype, kbp);

	RW_UNLOCK(Kdws.w_rwlock, pl);

	return;

}


/*
 * STATIC void
 * kd_setinitflag(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This routine sets kd_wsinitflag to 2 to signal that wsinit has run.
 */
STATIC void
kd_setinitflag(void)
{
	kd_wsinitflag = 2;
}


/*
 * STATIC int
 * kdopen(queue_t *, dev_t *, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- Return 0 on success, otherwise return error.
 *
 * Description:
 *	This routine is only called when an application
 *	(vtlmgr or wsinit) opens /dev/vtmon or /dev/kd/kd*
 *	files to set up the multiplexor. The channel data
 *	structure corressponding to the VT is allocated and
 *	initialized. The w_rwlock prevents multiple 
 *	kdopen() to run concurrently. 
 */
/* ARGSUSED */
STATIC int
kdopen(queue_t *qp, dev_t *dev_p, int flag, int sflag, cred_t *credp)
{
	int		indx;
	ws_channel_t	*chp;
	pl_t		pl;


	/*
	 * If the stream has already been opened, return
	 * EBUSY. This is principally done to prevent
	 * multiple opens of /dev/vtmon. The other minor
	 * devices are of kd are persistently linked
	 * underneath CHANMUX, which will prevent 
	 * second opens from succeeding.
	 */
	if (qp->q_ptr != (caddr_t) NULL)
		return (EBUSY);

	indx = getminor(*dev_p);

	if (indx > WS_MAXCHAN)
		return (ENODEV);

	pl = RW_WRLOCK(Kdws.w_rwlock, plstr);

	if ((chp = ws_getchan(&Kdws, indx)) == (ws_channel_t *) NULL) {
		if (!(chp = (ws_channel_t *)kmem_zalloc(
					sizeof(ws_channel_t), KM_NOSLEEP))) {
			RW_UNLOCK(Kdws.w_rwlock, pl);
			/*
			 *+ Beside channel zero, rest of the channels
			 *+ are allocated memory dynamically. There isn't 
			 *+ enough memory available to allocate for 
			 *+ channel data structure.
			 */
			cmn_err(CE_WARN, 
				"kdopen: out of memory");
			return (ENOMEM);
		}

		if (kdchaninit(&Kdws, chp, NULL, indx) == ENOMEM) {
			RW_UNLOCK(Kdws.w_rwlock, pl);
			/*
			 *+ Not enough memory to allocate for locks,
			 *+ synchronization variable, attribures or
			 *+ tab information. Check the memory
			 *+ configured in the system.
			 */
			cmn_err(CE_WARN,
				"kdopen: out of memory");
			return (ENOMEM);
		}

		Kdws.w_chanpp[indx] = chp;
		Kdptrs.k_charpp[indx] = chp->ch_charmap_p;

		/* increment count of configured channels */
		Kdws.w_nchan++;	
	}

	/*
	 * Set the q_ptr member of the read and write queues
	 * to point to the ws_channel_t structure.
	 */
	qp->q_ptr = (caddr_t)chp;
	WR(qp)->q_ptr = qp->q_ptr;
	chp->ch_qp = qp;

	if (!Kdws.w_qp)  
		Kdws.w_qp = chp->ch_qp;

	/* switch on the put and srv routines */
	qprocson(qp); 

	if (indx == 0) {
		kd_wsinitflag = 1;
		if (!itimeout((void (*)())kd_setinitflag, NULL, 2*HZ, plstr))
			kd_setinitflag();
	}

	RW_UNLOCK(Kdws.w_rwlock, pl);

	return (0);
}


/*
 * STATIC int
 * kdclose(queue_t *, int, int, cred_t *)
 * 
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	Close of /dev/kd/* (when the chanmux is being disassembled),
 *	and /dev/vtmon. Free the buffer allocated for the tab stops,
 *	release the charmap_t and scrn_t structures for the channel
 *	and finally the ws_channel_t structure itself.
 */
/* ARGSUSED */
STATIC int
kdclose(queue_t *qp, int flag, int type, cred_t *credp)
{
	ws_channel_t	*chp = (ws_channel_t *) qp->q_ptr;
	int		indx;
	pl_t		pl, oldpri;
	toid_t		tid;


	pl = RW_WRLOCK(Kdws.w_rwlock, plstr);

	/*
	 * Channel 0 never "really" closes.
	 */
	if (!chp->ch_id) {
		RW_UNLOCK(Kdws.w_rwlock, pl);
		return (0);
	}

	if (Kdws.w_qp == chp->ch_qp) {
		/* block all device interrupts especially clock interrupts */
		oldpri = splhi(); 
		/*
		 * Cancel any pending timeout() to send
		 * data up this stream.
		 */
		while (Kdws.w_timeid) {
			tid = Kdws.w_timeid;
			Kdws.w_timeid = 0;
			RW_UNLOCK(Kdws.w_rwlock, plhi); 
			untimeout(tid);
			(void) RW_WRLOCK(Kdws.w_rwlock, plhi);
		}
		/*
		 * Reset the workstation read queue pointer -- w_qp, 
		 * since the channel is being deallocated.
		 */ 
		Kdws.w_qp = (queue_t *) NULL;
		/* unblock device interrupts */
		splx(oldpri); 
	}

	flushq(WR(qp), FLUSHALL);

	/*
	 * Switch off the put and srv routines to disable
	 * any further messages to be queued here.
	 */
	qprocsoff(qp);

	chp->ch_qp = (queue_t *) NULL;
	qp->q_ptr = WR(qp)->q_ptr = (caddr_t) NULL;

#ifndef NO_MULTI_BYTE
	/*
	 * If this channel is in graphics text mode, deallocate resource
	 * associated with this mode.
	 */
	if (chp->ch_dmode == KD_GRTEXT)
		Kdws.w_consops->cn_gs_free(Kdws.w_consops, chp,
						&chp->ch_tstate);
#endif /* NO_MULTI_BYTE */

	indx = chp->ch_id;
	if (Kdws.w_scrbufpp[indx])
		kmem_free(Kdws.w_scrbufpp[indx], sizeof(ushort)*KD_MAXSCRSIZE);
	Kdws.w_scrbufpp[indx] = (ushort *) NULL;

	kmem_free(chp->ch_tstate.t_tabsp, ANSI_MAXTAB);

	ws_cmap_free(&Kdws, chp->ch_charmap_p);
	ws_scrn_free(&Kdws, chp);

	SV_DEALLOC(chp->ch_wactsv);
	SV_DEALLOC(chp->ch_xquesv);
	SV_DEALLOC(chp->ch_qrsvsv);
	LOCK_DEALLOC(chp->ch_mutex);

	kmem_free(Kdws.w_chanpp[indx], sizeof(ws_channel_t));
	Kdws.w_chanpp[indx] = (ws_channel_t *) NULL;

	/* decrement count of configured channels */
	Kdws.w_nchan--;

	RW_UNLOCK(Kdws.w_rwlock, pl);

	return (0);
}


/*
 * STATIC int
 * kdwsrv(queue_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	This is the write queue's service procedure. Based 
 *	on message type it calls appropriate procedure
 *	to process the message. 
 */
STATIC int
kdwsrv(queue_t *qp)
{
	mblk_t	*mp;


	while ((mp = getq(qp)) != (mblk_t *) NULL) {

		switch (mp->b_datap->db_type) {
		case M_PROTO:
		case M_PCPROTO:
			if ((mp->b_wptr - mp->b_rptr) == sizeof(ch_proto_t)) {
				kdproto(qp, mp);
				break;
			}

			/*
			 *+ Protocol messages are exchanged between
			 *+ channel multiplexor (CHANMUX), CHAR and
			 *+ ANSI module. An illegal or corrupted
			 *+ protocol message is received.
			 */
			cmn_err(CE_NOTE, 
				"kdwsrv: bad M_PROTO or M_PCPROTO msg");
			freemsg(mp);
			break;

		case M_DATA: {
			ws_channel_t	*chp = (ws_channel_t *)qp->q_ptr;
			termstate_t	*tsp;
			pl_t		opl;

			if(ws_con_need_text()){			/* L000 begin */
				/*
				 * We can't have the console
				 */
				putbq(qp, mp);
				return 0;
			}
			kd_con_acquired = B_TRUE;		/* L000 end */

			opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
			(void) LOCK(chp->ch_mutex, plstr);

			/*
			 * Acquire the console output lock to serialize 
			 * kernel and user-level output to console.
			 */ 
/*
			if (chp->ch_id == 0)
				console_output_lock(&kdconssw);
*/
			tsp = &chp->ch_tstate;

			/*
			 * Writes as if to /dev/null when in 
			 * KD_GRAPHICS mode.
			 */
#ifndef NO_MULTI_BYTE
			if (chp->ch_dmode == KD_GRTEXT) {
				while (mp->b_rptr != mp->b_wptr)
					Kdws.w_consops->cn_gcl_norm(
						Kdws.w_consops, chp,
						tsp, *mp->b_rptr++);
			} else
#endif /* NO_MULTI_BYTE */
			if (chp->ch_dmode != KD_GRAPHICS) {
				while (mp->b_rptr != mp->b_wptr)
					tcl_norm(Kdws.w_consops, chp, 
							tsp, *mp->b_rptr++);
			}

			/*
			 * Release the console output lock.
			 */
/*
			if (chp->ch_id == 0)
				console_output_unlock(&kdconssw, plstr);
*/
			UNLOCK(chp->ch_mutex, plstr);
			RW_UNLOCK(Kdws.w_rwlock, opl);

			kd_con_acquired = B_FALSE;	/* L000 */
			ws_con_done_text();		/* L000 */

			freemsg(mp);

			/*
			 * When all bytes are processed, call qenable()
			 * to reenable the queue and return. This is
			 * done so that output to one channel cannot
			 * monopolize STREAMS processing on the system;
			 * rather, a message is processed on each busy
			 * channel in a round-robin manner.
			 */
			qenable(qp);

			return (0);
		}

		case M_CTL:
			ws_mctlmsg(qp, mp);
			break;

		case M_IOCTL:
			kdmioctlmsg(qp, mp);
			break;

		case M_IOCDATA:
			kdmiocdatamsg(qp, mp);
			break;

		case M_STARTI:
		case M_STOPI:
		case M_READ:	/* ignore, no buffered data */
			freemsg(mp);
			break;

		case M_FLUSH:
			*mp->b_rptr &= ~FLUSHW;
			if (*mp->b_rptr & FLUSHR)
				qreply(qp, mp);
			else
				freemsg(mp);
			break;

		default:
			/*
			 *+ Only certain message types are handled by the
			 *+ the KD driver. An illegal message type is 
			 *+ received.
			 */
			cmn_err(CE_NOTE, 
				"kdwsrv: bad msg %x", mp->b_datap->db_type);
		}
	}

	return (0);
}


/*
 * STATIC void
 * kdmioctlmsg(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	Services the M_IOCTL type messages.
 */
STATIC void
kdmioctlmsg(queue_t *qp, mblk_t *mp)
{
	struct iocblk	*iocp;
	ws_channel_t	*chp = (ws_channel_t *) qp->q_ptr;
	struct strtty	*sttyp;
	mblk_t		*tmp;
	ch_proto_t	*protop;
	pl_t		opl;
	toid_t		tid;
	extern void	kd_kbio_setmode(queue_t *, mblk_t *, struct iocblk *);
	extern int	stri386ioctl(struct vnode *, int *, int, int *, int *);


	ASSERT(mp->b_wptr - mp->b_rptr == sizeof(struct iocblk));

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;

	switch (iocp->ioc_cmd) {

	case KIOCINFO:
		/*
		 * Return the short containing the bytes
		 * "kd" in it to identify the channel as
		 * belonging to the kd driver.
		 */

		iocp->ioc_rval = ('k' << 8) | 'd';
		ws_iocack(qp, mp, iocp);
		break;

	case KDGKBTYPE:
		/*
		 * return the keyboard type to the user
		 * by calling ws_copyout().
		 */

		if (!(tmp = allocb(sizeof(unchar), BPRI_MED))) {
			/*
			 *+ There isn't enough memory available to allocate
			 *+ for the message block to send the keyboard type
			 *+ information.
			 */
			cmn_err(CE_NOTE, 
				"!kdmioctlmsg: can't get msg for reply to KDKBTYPE");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}

		*(unchar *)tmp->b_rptr = Kdws.w_kbtype;
		tmp->b_wptr += sizeof(unchar);
		ws_copyout(qp, mp, tmp, sizeof(unchar));
		break;

	case KDGETLED:
		/*
		 * return the LEDs on/off state for 
		 * SCROLL LOCK, NUM LOCK and CAPS LOCK on
		 * the keyboard. ws_copyout() sends
		 * the LEDs state to the user.
		 */

		if (!(tmp = allocb(sizeof(unchar), BPRI_MED))) {
			/*
			 *+ There isn't enough memory available to allocate
			 *+ for the message block to send the LEDs state. 
			 */
			cmn_err(CE_NOTE, 
				"!kdmioctlmsg: can't get msg for reply to KDGETLED");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
		*(unchar *) tmp->b_rptr = ws_getled(&chp->ch_kbstate);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		tmp->b_wptr += sizeof(unchar);
		ws_copyout(qp, mp, tmp, sizeof(unchar));
		break;

	/* L005 begin
	 *
	 * KDAUTOACTIVATE is used to tune the value of the kd_no_activate
	 * variable which controlls whether VT's are automatically focused
	 * on open or not. kd_no_activate is defined in kd/space.c
	 */
	case KDNOAUTOFOCUS: {

		extern boolean_t kd_no_activate;
		unsigned long new_val = *(unsigned long *)mp->b_cont->b_rptr;

		kd_no_activate = (new_val) ? B_TRUE: B_FALSE;

		ws_iocack(qp, mp, iocp);

		break;

	}						/* L005 end */

	/* L002 begin
	 * 
	 * KDSETLCK is used to control the lock states on all VT's.
	 *
	 * Takes one argument, as per KDSETLED, but this value is masked into
	 * the lock state of all VT's rather than just the active VT.
	 */
	case KDSETLCK: {

		ws_channel_t *chan, *first_chan;
		unsigned char mask = *(unchar *)mp->b_cont->b_rptr;
		unsigned int rv = ECHRNG;

		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_timeid) {
			tid = Kdws.w_timeid;
			Kdws.w_timeid = 0;
			UNLOCK(Kdws.w_mutex, opl);
			untimeout(tid);
			(void) LOCK(Kdws.w_mutex, plstr);
		}
		UNLOCK(Kdws.w_mutex, opl);

		/*
		 * Take the workstation lock in exclusive mode,
		 * for all the channels in this workstation, appply the user
		 * provided mask to the lock state bits in the ws_channel_t
		 * kb_state. When we find the active channel make a call to 
		 * kdkb_setled to manipulate the keyboard lights, and then,
		 * when everything else is done send a M_PROTO message upstream
		 * the char module telling it to modify its idea of the active
		 * channel lock state.
		 */
		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

		first_chan = chan = *Kdws.w_chanpp;

		/*
		 * Walk the channel list clearing the lock states as we go.
		 * Note that we don't need to acquire any of the channel locks
		 * because we have the workstation lock in write mode. 
		 *
		 * Call kdkb to parse the flag bits and set kb_state, then
		 * send a CH_LEDSTATE M_PROTO message to the char module for
		 * every channel to make sure the char's private copy of the
		 * lock state is changed.
		 */
		while(chan != (ws_channel_t *)NULL){

			if(WS_ISACTIVECHAN(&Kdws, chan)){
				/*
				 * This is the active channel so we have to
				 * switch off the lights and use this lock
				 * state in the M_PROTO to the char module.
				 */
				kdkb_setled(&chan->ch_kbstate, mask);

			}	/* End if WS_ACTIVECHAN */
			else
				kdkb_update_kbstate(&chan->ch_kbstate, mask);

			if(!(tmp = allocb(sizeof(ch_proto_t), BPRI_HI))){

				rv = ENOMEM;
				break;
			}

			/*
			 * CH_CHR/CH_LEDSTATE channel protocol message
			 * is created to notify CHAR of the change.
			 */
			tmp->b_datap->db_type = M_PROTO;
			tmp->b_wptr += sizeof(ch_proto_t);
			/* LINTED pointer alignment */
			protop = (ch_proto_t *)tmp->b_rptr;
			protop->chp_type = CH_CTL;
			protop->chp_stype = CH_CHR;
			protop->chp_stype_cmd = CH_LEDSTATE;
			protop->chp_stype_arg =
			    (chan->ch_kbstate.kb_state & ~NONTOGGLES);

			putnext(chan->ch_qp, tmp);	/* Send the M_PROTO */

			chan = chan->ch_nextp;

			if(chan == first_chan)
				break;

		}
		
		RW_UNLOCK(Kdws.w_rwlock, opl);

		if(chan == (ws_channel_t *)NULL || !tmp)
			ws_iocnack(qp, mp, iocp, rv);	/* NACK ioctl */
		else
			ws_iocack(qp, mp, iocp);	/* ACK this ioctl */


		ws_kbtime(&Kdws);			/* Update time stamps */

		break;					/* L002 end */

	}

	case KDSETLED: {
		/*
		 * set the LEDs based on the bit mask of
		 * an ioctl argument.
		 */

		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_timeid) {
			tid = Kdws.w_timeid;
			Kdws.w_timeid = 0;
			UNLOCK(Kdws.w_mutex, opl);
			untimeout(tid);
			(void) LOCK(Kdws.w_mutex, plstr);
		}
		UNLOCK(Kdws.w_mutex, opl);

		if (!(tmp = allocb(sizeof(ch_proto_t), BPRI_HI))) {
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
		/*
		 * Only the active channel must be allowed to
		 * access the keyboard controller to set the LEDs.
		 */
		if (WS_ISACTIVECHAN(&Kdws, chp))
	                kdkb_setled(&chp->ch_kbstate,
					*(unchar *)mp->b_cont->b_rptr);
		else						/* L003 begin */
			kdkb_update_kbstate(&chp->ch_kbstate, 
				*(unchar *)mp->b_cont->b_rptr);	/* L003 end */

		/*
		 * CH_CHR/CH_LEDSTATE channel protocol message
		 * is created to notify CHAR of the change.
		 */
		tmp->b_datap->db_type = M_PROTO;
		tmp->b_wptr += sizeof(ch_proto_t);
		/* LINTED pointer alignment */
		protop = (ch_proto_t *) tmp->b_rptr;
		protop->chp_type = CH_CTL;
		protop->chp_stype = CH_CHR;
		protop->chp_stype_cmd = CH_LEDSTATE;
		protop->chp_stype_arg = 
			(chp->ch_kbstate.kb_state & ~NONTOGGLES);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		ws_iocack(qp, mp, iocp);
		ws_kbtime(&Kdws);
		qreply(qp, tmp);
		break;
	}

	case KDSETTYPEMATICS: {
		extern unchar kd_typematic;

		/*
		 * Set the TYPEMATICS based on the bit mask of
		 * an ioctl argument.
		 */

		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_timeid) {
			tid = Kdws.w_timeid;
			Kdws.w_timeid = 0;
			UNLOCK(Kdws.w_mutex, opl);
			untimeout(tid);
			(void) LOCK(Kdws.w_mutex, plstr);
		}
		UNLOCK(Kdws.w_mutex, opl);

		kd_typematic = *(unchar *)mp->b_cont->b_rptr;
		kdkb_cmd(TYPE_WARN);
		ws_iocack(qp, mp, iocp);
		break;
	}

	case KDLEDCTL: {
		uint_t ledctl;
		kbstate_t *kbp = &chp->ch_kbstate;

		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_timeid) {
			tid = Kdws.w_timeid;
			Kdws.w_timeid = 0;
			UNLOCK(Kdws.w_mutex, opl);
			untimeout(tid);
			(void) LOCK(Kdws.w_mutex, plstr);
		}
		UNLOCK(Kdws.w_mutex, opl);

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);

		ledctl = *(unchar *)mp->b_cont->b_rptr;
		if (ledctl & KDLEDCTLACQ)
			kbp->kb_state |= LEDMASK;
		else if (ledctl & KDLEDCTLREL)
			kbp->kb_state &= ~LEDMASK;

		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);

		ws_iocack(qp, mp, iocp);
		break;
	}

	case TCSETSW:
	case TCSETSF:
	case TCSETS: {
		struct termios	*tsp;

		if (!mp->b_cont) {
			ws_iocnack(qp, mp, iocp, EINVAL);
			break;
		}

		/* LINTED pointer alignment */
		tsp = (struct termios *) mp->b_cont->b_rptr;
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
	        sttyp = (struct strtty *) &chp->ch_strtty;
		sttyp->t_cflag = tsp->c_cflag;
		sttyp->t_iflag = tsp->c_iflag;
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		ws_iocack(qp, mp, iocp);
		break;
	}

	case TCSETAW:
	case TCSETAF:
	case TCSETA: {
		struct termio	*tp;

		if (!mp->b_cont) {
			ws_iocnack(qp, mp, iocp, EINVAL);
			break;
		}

		/* LINTED pointer alignment */
		tp = (struct termio *)mp->b_cont->b_rptr;
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
	        sttyp = (struct strtty *) &chp->ch_strtty;
		sttyp->t_cflag = (sttyp->t_cflag & 0xffff0000 | tp->c_cflag);
		sttyp->t_iflag = (sttyp->t_iflag & 0xffff0000 | tp->c_iflag);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		ws_iocack(qp, mp, iocp);
		break;
	}

	case TCGETA: {
		struct termio	*tp;

		if (mp->b_cont)		/* bad user supplied parameter */
			freemsg(mp->b_cont);

		if ((mp->b_cont = allocb(sizeof(struct termio), BPRI_MED)) 
					== (mblk_t *) NULL) {
			/*
			 *+ There isn't enough memory available to allocate
			 *+ for message block to send a reply to TCGETA ioctl.
			 *+ The requested size of the block is equal to
			 *+ the size of termio data structure.
			 */
			cmn_err(CE_NOTE, 
				"!kdmioctlmsg: can't get msg for reply to TCGETA");
			mp->b_datap->db_type = M_IOCNAK;
			qreply(qp, mp);
			break;
		}

		/* LINTED pointer alignment */
		tp = (struct termio *)mp->b_cont->b_rptr;
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
	        sttyp = (struct strtty *) &chp->ch_strtty;
		tp->c_iflag = (ushort) sttyp->t_iflag;
		tp->c_cflag = (ushort) sttyp->t_cflag;
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		mp->b_cont->b_wptr += sizeof(struct termio);
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = sizeof(struct termio);
		qreply(qp, mp);
		break;
	}

	case TCGETS: {
		struct termios	*tsp;

		if (mp->b_cont)		/* bad user supplied parameter */
			freemsg(mp->b_cont);

		if ((mp->b_cont = allocb(sizeof(struct termios), BPRI_MED)) 
					== (mblk_t *)NULL) {
			/*
			 *+ There isn't enough memory available to allocate
			 *+ for message block to send a reply to TCGETS ioctl.
			 *+ The requested size of the block is equal to
			 *+ the size of termios data structure.
			 */
			cmn_err(CE_NOTE, 
				"!kdmioctlmsg: can't get msg for reply to TCGETS");
			mp->b_datap->db_type = M_IOCNAK;
			qreply(qp, mp);
			break;
		}

		/* LINTED pointer alignment */
		tsp = (struct termios *)mp->b_cont->b_rptr;
		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
	        sttyp = (struct strtty *) &chp->ch_strtty;
		tsp->c_iflag = sttyp->t_iflag;
		tsp->c_cflag = sttyp->t_cflag;
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		mp->b_cont->b_wptr += sizeof(struct termios);
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = sizeof(struct termios);
		qreply(qp, mp);
		break;
	}

	case KBIO_SETMODE:
		/*
		 * This ioctl is for SCO compatibility.
		 */
		kd_kbio_setmode(qp, mp, iocp);
		break;

	case KBIO_GETMODE:
		/*
		 * This ioctl is for SCO compatibility. 
		 */
		iocp->ioc_rval = chp->ch_charmap_p->cr_kbmode;
		ws_iocack(qp, mp, iocp);
		break;

/* NOT IMPLEMENTED
	case TIOCSWINSZ:
		ws_iocack(qp, mp, iocp);
		break;

	case TIOCGWINSZ:
		ws_winsz(qp, mp, chp, iocp->ioc_cmd);
		break;
NOT IMPLEMENTED */

	case TCSBRK:
		ws_iocack(qp, mp, iocp);
		break;

	case GIO_ATTR:	
		/*
		 * Return the current display attribute - background
		 * color, foreground color etc.
		 */

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);
		iocp->ioc_rval = tcl_curattr(chp);
		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		ws_iocack(qp, mp, iocp);
		break;

	case VT_OPENQRY:
		/*
		 * Return the number of first free VT available by
		 * calling ws_freechan(). The w_rwlock is held
		 * in exclusive mode to prevent any channel
		 * being allocated/deallocated while freechan inquiry 
		 * is in progress.
		 */

		if (!(tmp = allocb(sizeof(int), BPRI_MED))) {
			/*
			 *+ There isn't enough memory available to allocate
			 *+ for message block to send a reply to VT_OPENQRY
			 *+ ioctl. The requested size of the block is equal 
			 *+ to the size of an int.
			 */
			cmn_err(CE_NOTE, 
				"!kdmioctlmsg: can't get msg for reply to VT_OPENQRY");
			ws_iocnack(qp, mp, iocp, ENOMEM);
			break;
		}

		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
		/* LINTED pointer alignment */
		*(int *)tmp->b_rptr = ws_freechan(&Kdws);
		RW_UNLOCK(Kdws.w_rwlock, opl);
		tmp->b_wptr += sizeof(int);
		ws_copyout(qp, mp, tmp, sizeof(int));
		break;

	default:
		if (stri386ioctl(NULL, &iocp->ioc_cmd, NULL, NULL, NULL) == 0) {
			kdvmstr_ioctl(qp, mp, iocp, chp);
			break;
		}

#ifdef MERGE386
		if (kdppi_ioctl(qp, mp, iocp, chp)) {
			/*
			 *+ To shut up klint
			 */
			cmn_err(CE_NOTE, 
				"!kdmioctlmsg: %x", iocp->ioc_cmd);
			break;
		}
#endif /* MERGE386 */

		/*
		 * Send a NACK message upstream because it is an 
		 * illegal ioctl.
		 */
		ws_iocnack(qp, mp, iocp, EINVAL);

#ifdef DEBUG
		/*
		 *+ Bad M_IOCTL message.
		 */
		cmn_err(CE_NOTE, 
			"!kdmioctlmsg: %x", iocp->ioc_cmd);
#endif /* DEBUG */
	} /* switch */
}


/*
 * STATIC void
 * kdproto(queue_t *qp, mblk_t *mp)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *
 * Description:
 *	Services the M_PROTO or M_PCPROTO type messages.
 */
STATIC void
kdproto(queue_t *qp, mblk_t *mp)
{
	ch_proto_t	*chprp;
	ws_channel_t	*chp;
	int		error;
	pl_t		opl;


	chp = (ws_channel_t *) qp->q_ptr;
	/* LINTED pointer alignment */
	chprp = (ch_proto_t *) mp->b_rptr;

	switch (chprp->chp_stype) {
	case CH_TCL:

		switch(chprp->chp_stype_cmd) {
		case TCL_FLOWCTL:
			/*
			 * Call tcl_scrollock() to turn on/off the
			 * the SCROLL LOCK LED on the keyboard and
			 * return.
			 */
			tcl_scrolllock(&Kdws, chp, chprp->chp_stype_arg); 
			break;

		case TCL_SEND_SCR:
			/*
			 * Call tcl_sendscr() to send the contents of 
			 * the screen to user.
			 */
                        tcl_sendscr(&Kdws, chp, &chp->ch_tstate);
			break;

		/* Enhanced Application Compatibility Support */

		case TCL_SWITCH_VT: {
			ws_channel_t *newchp;
			int  newvt = chprp->chp_stype_arg;


			if (newvt < 0 || newvt > WS_MAXCHAN)
				break;

			opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

			if (!(newchp = ws_getchan(chp->ch_wsp, newvt))) {
				RW_UNLOCK(Kdws.w_rwlock, opl);
				break;
			}

			if (!newchp->ch_opencnt) {
				RW_UNLOCK(Kdws.w_rwlock, opl);
				break;
			}

			(void) ws_activate(&Kdws, newchp, VT_NOFORCE);

			RW_UNLOCK(Kdws.w_rwlock, opl);

			break;
		}

		/* End Enhanced Application Compatibility Support */

		default: {
			opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
			(void) LOCK(chp->ch_mutex, plstr);	

			/*
			 * Acquire the console output lock to serialize
			 * kernel and user-level output to console.
			 */ 
/*
			if (chp->ch_id == 0)
				console_output_lock(&kdconssw);
*/
			/*
			 * Writes as if to /dev/null when in 
			 * KD_GRAPHICS mode. 
			 */
#ifndef NO_MULTI_BYTE
			if (chp->ch_dmode == KD_GRTEXT) {
				Kdws.w_consops->cn_gcl_handler(&Kdws, mp,
					&chp->ch_tstate, chp);
			} else
#endif /* NO_MULTI_BYTE */
			if (chp->ch_dmode != KD_GRAPHICS)
				tcl_handler(&Kdws, mp, &chp->ch_tstate, chp);

			/*
			 * Done user-level output. Release the console 
			 * output lock.
			 */
/*
			if (chp->ch_id == 0)
				console_output_unlock(&kdconssw, plstr);
*/
			UNLOCK(chp->ch_mutex, plstr);
			RW_UNLOCK(Kdws.w_rwlock, opl); 

			break;
		}
		} /* switch */

		break;

	case CH_CHR:
		switch(chprp->chp_stype_cmd) {
		case CH_CHROPEN:
			if (chp->ch_opencnt) {
				ws_openresp_chr(qp, mp, chprp, chp);
				freemsg(mp);
				return;
			}
			break;

		default:
			/*
			 *+ The CHAR module only sends a protocol message to
			 *+ indicate that its ready to receive the character
			 *+ mapping and screen mapping tables. Any other
			 *+ protocol commands are considered illegal.
			 */
                        cmn_err(CE_WARN, 
				"kdproto: received unknown CH_CHR %d",
				chprp->chp_stype_cmd);
			break;
		} /* switch */

		break;

	case CH_CHAN:
		switch (chprp->chp_stype_cmd) {
		case CH_CHANOPEN:
			/*
			 * This command is send by CHANMUX when the VT
			 * to which the stream is linked is being reopened.
			 * kdvt_open() is called to mark the channel is
			 * in use and make it the active channel. ws_openresp()
			 * sends a message upstream to CHANMUX with the
			 * error number returned by kdvt_open().
			 */
			error = kdvt_open(chp, chprp->chp_stype_arg);
			ws_openresp(qp, mp, chprp, chp, error);
			/* do not free mp since ws_openresp will */
			return;

		case CH_CHANCLOSE:
			opl = RW_WRLOCK(Kdws.w_rwlock, plstr);
			if (chp == Kdws.w_switchto)
				Kdws.w_switchto = (ws_channel_t *) NULL;

			/*
			 * If channel 0 is being closed and it is active
			 * then restore the video to default video mode.
			 */
			if (chp->ch_id == 0 && WS_ISACTIVECHAN(&Kdws, chp)) {
				vidstate_t *vp;

				vp = &chp->ch_vstate;
#ifdef EVGA
				evga_ext_rest(vp->v_cvmode);
#endif	/* EVGA */
#ifndef NO_MULTI_BYTE
				/*
				 * KLUDGE: Do not reset the display to
				 * default video mode for channel 0.
				 * This is to prevent every open to set
				 * the channel to graphics-text mode.
				 */
				if (vp->v_cvmode != vp->v_dvmode &&
				    chp->ch_dmode != KD_GRTEXT) {
#else
				if (vp->v_cvmode != vp->v_dvmode) {
#endif /* NO_MULTI_BYTE */
					kdv_setdisp(chp, vp, &chp->ch_tstate,
						Kdws.w_vstate.v_dvmode);
					kdclrscr(chp, chp->ch_tstate.t_origin,
						chp->ch_tstate.t_scrsz);
				}
#ifdef EVGA
				/* 
				 * kdv_setdisp() sets vp->v_cvmode to
				 * new mode. 
				 */
				evga_ext_init(vp->v_cvmode);
#endif /* EVGA */
			}

			/*
			 * Reset the channel and switch to the
			 * next channel in the list or channel 0.
			 */
			ws_preclose(&Kdws, chp);

			/*
			 * Reset the channel's screen buffer pointer.
			 */
			kdvt_close(chp);

			chp->ch_kbstate.kb_state = 0;

			/*
			 * Reset the scrn_t and charmap_t data structures
			 * for the channel back to the default for the 
			 * workstation. However, if the channel was closed
			 * and is still active (only true for VT 0) just
			 * turn off the keyboard LEDs but leave the screen
			 * contents alone. Otherwise, the channel has been
			 * switched out and its videostate structure's
			 * screen pointer should point to the channel's
			 * screen buffer pointer (which must be NULL at 
			 * this point because of kdvt_close()).
			 */
			ws_cmap_reset(&Kdws, chp->ch_charmap_p);
			ws_scrn_reset(&Kdws, chp);

			if (WS_ISACTIVECHAN(&Kdws, chp))
				/* turn off LEDs */
				kdkb_setled(&chp->ch_kbstate, 0);
			else
				chp->ch_vstate.v_scrp =
					Kdws.w_scrbufpp[chp->ch_id];

			if (chp->ch_id != 0) {
				ws_chinit(&Kdws, chp, chp->ch_id);
				if (Kdws.w_scrbufpp[chp->ch_id])
					kdclrscr(chp, chp->ch_tstate.t_origin,
						chp->ch_tstate.t_scrsz);
			} else {
				/*
				 * Reset attributes for VT 0. (ws_chinit() will
				 * do it for other VTs). Note that I can call
				 * ws_alloc_attrs() without checking the return
				 * val since it won't need to do kmem_zalloc().
				 * Therefore, this call will always succeed.
				 */
				ws_alloc_attrs(&Kdws, chp, KM_NOSLEEP);
                        }

			RW_UNLOCK(Kdws.w_rwlock, opl);

			/*
			 * Send a close acknowledgement message
			 * to CHANMUX.
			 */
			ws_closechan(qp, &Kdws, chp, mp);

			return;		/* don't free mp */

		default:
			/*
			 *+ The CHANMUX module only sends a protocol message 
			 *+ to open and close a channel. Any other CH_CHAN
			 *+ subtype protocol commands are considered illegal.
			 */
			cmn_err(CE_WARN, 
				"kdproto: received unknown CH_CHAN %d",
				chprp->chp_stype_cmd);
		}

		break;

	case CH_XQ:
		ws_xquemsg(chp, chprp->chp_stype_cmd);
		break;

	case CH_RAWMODE: {
		int type = chprp->chp_stype_arg;

		opl = RW_RDLOCK(Kdws.w_rwlock, plstr);
		(void) LOCK(chp->ch_mutex, plstr);	

		if (type == 0)
			chp->ch_rawmode = 0;
		else
			chp->ch_rawmode = 1;

		UNLOCK(chp->ch_mutex, plstr);
		RW_UNLOCK(Kdws.w_rwlock, opl); 

		break;
	}

	default:
		/*
		 *+ An illegal control protocol message is sent by the
		 *+ modules upstream. The allowable subtypes of control 
		 *+ protocol message are CH_TCL, CH_CHR, CH_CHAN and the
		 *+ rest are considered illegal.
		 */
		cmn_err(CE_WARN, 
			"kdproto, received unknown CH_CTL %d", 
			chprp->chp_stype);
		break;
	}

	freemsg(mp);
}


#ifdef TIMEOUT_KDINTR

STATIC void kdprocscan(void);

/*
 * void
 * kdintr(void)
 *
 * Calling/Exit State:
 *	- Acquire the 8042 mutex lock to protect the keyboard/auxiliary
 *	  controller chip.
 *	
 * Description:
 *	This routine is the interrupt handler for the keyboard. It is
 *	called when keys are pressed or released on the keyboard.
 *
 * Note:
 *	A three-tier keyboard interrupt handler. The scancode is 
 *	enqueued and is scheduled for processing later. This reduces 
 *	locking contention. In contrast to the earlier approach,
 *	processing of scancodes was done in the handler itself.
 *
 *	Only one instance of kdprocscan() must be running or scheduled to
 *	run at any time. This not only solves concurrency issues between
 *	kdintr() and kdprocscan() but also is required for correctness.
 *	For example, if two instances of kdprocscan() are executing
 *	on different processors, then one can get the make scancode
 *	and the other can get the break scancode for the same key.
 *
 *	On a multiprocessor system, if untimeout is called while any
 *	function called by the pending timeout request is running,
 *	untimeout will not return until the function completes.
 *
 * PERF:
 *	If kdprocscan() is running, then do not do untimeout() and 
 *	itimeout(). However, this may entail locking overhead.
 *
 */
void
kdintr(void)
{
	unchar		rawscan;	/* raw keyboard scan code */
	unchar		kbscan;		/* AT/XT raw scan code */
	pl_t		opl;


	if (WS_ISNOTINITED(&Kdws))	/* can't do anything anyway */
		return;

	/* acquire the i8042 keyboard lock */
	I8042_LOCK(opl);

	if (!(inb(KB_STAT) & KB_OUTBF)) {	/* no data from keyboard? */
		rawscan = inb(KB_IDAT);		/* clear spurious data */
		I8042_UNLOCK(opl);
		drv_setparm(SYSRINT, 1);	/* don't care if it succeeds */
		return;				/* return immediately */
	}

	kbscan = inb(KB_IDAT);		/* read scan data */

	/* release the i8042 keyboard lock */
	I8042_UNLOCK(opl);

#ifdef AT_KEYBOARD
	if (kb_raw_mode == KBM_AT)
		rawscan = kd_xlate_at2xt(kbscan);
	else
#endif /* AT_KEYBOARD */
		rawscan = kbscan;

	drv_setparm(SYSRINT, 1);	/* don't care if it succeeds */

	if (rawscan == KB_ACK)		/* ack from keyboard? */
		return;			/* Spurious ACK -- cmds to keyboard now polled */

	/*
	 * if its a make scancode, then generate a click sound
	 */
	if (!(rawscan & KBD_BREAK))
		kdkb_keyclick();

	/*
	 * clear any pending timeouts
	 */
	if (kdscanq.sq_timeid) {
		untimeout(kdscanq.sq_timeid);
		kdscanq.sq_timeid = 0;
	}

	ASSERT(kdscanq.sq_timeid == 0);

	/*
	 * put raw scancode into kdscanq buffer
	 */
	KDPUTSCAN(&kdscanq, kbscan);

	kdscanq.sq_timeid = itimeout((void (*)()) kdprocscan, 
					NULL, HZ/50, plstr);
}


/*
 * STATIC void
 * kdprocscan(void)
 *
 * Calling/Exit State:
 *	- The w_rwlock is acquired in exclusive mode 
 *	  to serialize channel switching/activation
 *	  operation.
 *
 * Description:
 *	The principal functions of the routine are:
 *	- to send the scancode generated by the 
 *	  keyboard upstream to the CHAR
 *	- check to see if a "system request" has
 *	  occurred to switch to another VT
 *	- enter the kernel debugger
 *	- check to see if the on/off state of 
 *	  the LEDs on the keyboard need to be
 *	  changed.
 *	- enqueue the scancodes to send it upstream.
 *
 * Note:
 *	Only one instance of kdprocscan() must be running or 
 *	scheduled to run at any time.
 */
STATIC void
kdprocscan(void)
{
	ws_channel_t	*achp;		/* active channel pointer */
	charmap_t	*cmp;		/* character map pointer */
	keymap_t	*kmp;
	kbstate_t	*kbp;		/* pointer to keyboard state */
	unchar		rawscan;	/* raw keyboard scan code */
	unchar		kbscan;		/* AT/XT raw scan code */
	unchar		scan;		/* "cooked" scan code */
	ushort		ch, okbstate;
	unchar		kbrk, oldprev;
	pl_t		opl;
	toid_t		tid;


	ASSERT(WS_ISINITED(&Kdws));

	while (!(KDSCANQEMPTY(&kdscanq))) {

		/*
		 * get a scancode to be processed
		 */
		KDGETSCAN(&kdscanq, kbscan);

#ifdef AT_KEYBOARD
		if (kb_raw_mode == KBM_AT)
			rawscan = kd_xlate_at2xt(kbscan);
		else
#endif /* AT_KEYBOARD */
			rawscan = kbscan;

		kbrk = rawscan & KBD_BREAK;

		opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

		Kdws.w_intr++;

		/*
		 * get an active channel
		 */
		achp = WS_ACTIVECHAN(&Kdws);

		ASSERT(Kdws.w_qp == achp->ch_qp);
		ASSERT(achp->ch_charmap_p != NULL);

		cmp = achp->ch_charmap_p;
		kbp = &achp->ch_kbstate;
		okbstate = kbp->kb_state;
		oldprev = kbp->kb_prevscan;

		/*
		 * ws_scanchar() requires to hold the cr_mutex (charmap
		 * basic lock) on entry to prevent any changes to the 
		 * translation tables.
		 */
		(void) LOCK(cmp->cr_mutex, plstr);

		kmp = cmp->cr_keymap_p;
		/*
		 * Translate scancode into an element in the character
		 * set or a "special" character.
		 */
		ch = ws_scanchar(cmp, kbp, rawscan, 0);

		/*
		 * Check for handling extended scan codes correctly this
		 * is because ws_scanchar() calls ws_procscan() on its own 
		 */
		if (oldprev == 0xe0 || oldprev == 0xe1)
			kbp->kb_prevscan = oldprev;
	
		/*
		 * process scan codes because keyboard generates
		 * extended scancodes.
		 */
		scan = ws_procscan(cmp, kbp, rawscan);

		UNLOCK(cmp->cr_mutex, plstr);

		if (kdkb_locked(ch, kbrk)) {
			Kdws.w_intr = 0;
			RW_UNLOCK(Kdws.w_rwlock, opl);
			return;
		}

		if (!kbrk) {
			if (WS_SPECIALKEY(kmp, kbp, scan) || kbp->kb_sysrq) {
				/* w_rwlock may be released by kdcksysrq */
				if (kdcksysrq(cmp, kbp, ch, scan) == 1)
					return;
				else {
					ASSERT(getpl() == plstr);
					(void) RW_WRLOCK(Kdws.w_rwlock, plstr); 
				}
			}
		} else if (kbp->kb_sysrq && kbp->kb_srqscan == scan) {
			Kdws.w_intr = 0;
			RW_UNLOCK(Kdws.w_rwlock, opl);
			return;
		}

		/*
		 * If a change occurred in the SCROLL LOCK, CAPS LOCK
		 * or NUM LOCK toggle keys, reprogram the LEDs
		 */
		if (KBTOGLECHANGE(okbstate, kbp->kb_state))
			kdkb_cmd(LED_WARN);

		Kdws.w_intr = 0;
		RW_UNLOCK(Kdws.w_rwlock, opl);

		opl = LOCK(Kdws.w_mutex, plstr);
		while (Kdws.w_timeid) {
			tid = Kdws.w_timeid;
			Kdws.w_timeid = 0;
			UNLOCK(Kdws.w_mutex, opl);
			untimeout(tid);
			opl = LOCK(Kdws.w_mutex, plstr);
		}
		UNLOCK(Kdws.w_mutex, opl);

		/*
		 * Call ws_enque() to store the raw scancode in the
		 * save message. If ws_enque() returns 1, schedule
		 * a timeout() to send a message upstream. Otherwise,
		 * ws_enque() sent the message upstream and there
		 * is no need for the timeout().
		 */
		if (ws_enque(Kdws.w_qp, &Kdws.w_mp, kbscan, plstr)) {
			opl = LOCK(Kdws.w_mutex, plstr);
			Kdws.w_timeid = itimeout((void(*)())ws_kbtime, &Kdws, 
							HZ/29, plstr);
			UNLOCK(Kdws.w_mutex, opl);
		}
	} /* while */

	return;
}
#endif /* TIMEOUT_KDINTR */

/*
 * void
 * kdintr(void)
 *
 * Calling/Exit State:
 *	- The w_rwlock is acquired in exclusive mode 
 *	  to serialize channel switching/activation
 *	  operation.
 *	
 * Description:
 *	This routine is the interrupt handler for the 
 *	keyboard. It is called when keys are pressed 
 *	or released on the keyboard. The principal
 *	functions of the interrupt routine are:
 *	- to send the scancode generated by the 
 *	  keyboard upstream to the CHAR
 *	- check to see if a "system request" has
 *	  occurred to switch to another VT
 *	- enter the kernel debugger
 *	- check to see if the on/off state of 
 *	  the LEDs on the keyboard need to be
 *	  changed.
 *	- enqueue the scancodes to send it upstream.
 */
void
kdintr(void)
{
	unchar		rawscan;	/* raw keyboard scan code */
	unchar		kbscan;		/* AT/XT raw scan code */
	unchar		scan;		/* "cooked" scan code */
	ws_channel_t	*achp;		/* active channel pointer */
	charmap_t	*cmp;		/* character map pointer */
	keymap_t	*kmp;
	kbstate_t	*kbp;		/* pointer to keyboard state */
	unchar		kbrk, oldprev;
	ushort		ch, okbstate;
	pl_t		opl, opl8042;
	toid_t		tid;
	uchar_t		*bufp = &kbscan;
	int		nscans; 
	int		i;
	static int	e1flagcnt = 0;	/* 0xe1 flag count */
	extern boolean_t kdcnresumed;
	extern void	kdcncleanup();

	/* The i8042_write function always checks the output buffer for scan
	 * data before it writes anything to the 8042 port. So when we enter
	 * interrupt handler, we maintain a stack of three elements (kbscans)
	 * and this gets populated as follows,
	 *
	 * check for the saved data in i8042_state structure
	 * if present save it the first element of the stack (kbscan_tos = 0)
	 * increment kbscan_tos
	 * read the scan data
	 * store it at kbscans[kbscanndx++]
	 * call disable keyboard
	 * This might again save any data in the i8042_structure.
	 * if present save it the kbscans[kbscanndx++]
	 *
	 * So potentially we can have three characters in the stack or lesser.
	 * If we don't do it this way we will lose any data saved.
	 */

	extern struct i8042 {
		int	s_spl;	/* saved spl level */
		uchar_t	s_state; /* state of 8042 */
		uchar_t	s_saved; /* indicates data was saved */
		uchar_t	s_data;	/* saved data (scan code or 320 mouse input) */
		uchar_t	s_dev2;	/* device saved character is meant for */
	} i8042_state;
	int		first_time = B_TRUE;
	unchar		kbscans[3];		/* AT/XT raw scan code stack*/
	int		kbscan_tos = 0;		/* stack top of kbscans */
	int		kbscanndx = 0;		/* kbscans stack index 
						 * from bottom */


	if (WS_ISNOTINITED(&Kdws))	/* can't do anything anyway */
		return;

	if (kdcnresumed) {
		kdcncleanup();
		kdcnresumed = B_FALSE;
	}

	/*
	 * Acquire the 8042 mutex lock to protect the keyboard/auxiliary
	 * controller chip.
	 */
	I8042_LOCK(opl8042);

	if (!(inb(KB_STAT) & KB_OUTBF)){ /* no data from keyboard? */
		rawscan = inb(KB_IDAT);	/* clear possible spurious data*/
		/* release the i8042 mutex lock */
		I8042_UNLOCK(opl8042);
		drv_setparm(SYSRINT, 1);	/* don't care if it succeeds */
		return;			/* return immediately */
	}
	if ((i8042_state.s_saved) && (!i8042_state.s_dev2)) {
		kbscans[kbscan_tos++] = i8042_state.s_data;
		i8042_state.s_saved = i8042_state.s_data = 
			i8042_state.s_dev2 =0;
	}

	kbscans[kbscan_tos++] = inb(KB_IDAT);		/* read scan data */

	drv_setparm(SYSRINT, 1);	/* don't care if it succeeds */

	while (kbscanndx < kbscan_tos){
		nscans = 1;
		kbscan = kbscans[kbscanndx++];
		if (kbscan == KB_ACK) {		/* ack from keyboard? */
			/* release the i8042 mutex lock */
			if (kbscanndx < kbscan_tos)
				continue;
			else{
				I8042_UNLOCK(opl8042);
				return;	/*Spurious ACK -- cmds to keyboard now polled*/
			}
		}

		if (e1flagcnt > 0){
			static uchar_t pausescans[] = 
				{ 0xe1, 0x1d, 0x45, 0xe1, 0x9d, 0xc5 };

			switch (e1flagcnt) {
			case 1:
			case 2:
			case 3:
			case 4:
				e1flagcnt++; 
				/* release the i8042 mutex lock */
				if (kbscanndx < kbscan_tos)
					continue;
				else{
					I8042_UNLOCK(opl8042);
					return;	/*Spurious ACK -- cmds to keyboard now polled*/
				}
			default:
				e1flagcnt = 0;
				bufp = pausescans;
				nscans = 6;
			}
		} else if (kbscan == 0xe1) {
			e1flagcnt = 1;
			/* release the i8042 mutex lock */
			if (kbscanndx < kbscan_tos)
				continue;
			else{
				I8042_UNLOCK(opl8042);
				return;	/*Spurious ACK -- cmds to keyboard now polled*/
			}
		}

		/* In keyboard interrupt. */
		Kdws.w_intr = 1;

		if (first_time == B_TRUE){
			i8042_disable_interface();
			first_time = B_FALSE;
			if ((i8042_state.s_saved) && (!i8042_state.s_dev2)) {
				kbscans[kbscan_tos++] = i8042_state.s_data;
				i8042_state.s_saved = i8042_state.s_data =
					i8042_state.s_dev2 =0;
			}
		}
		I8042_UNLOCK(opl8042);

		for (i = 0; i < nscans; i++) {
			rawscan = bufp[i];
			kbrk = rawscan & KBD_BREAK;

			opl = RW_WRLOCK(Kdws.w_rwlock, plstr);

			/*
		 	* Get a pointer to the active channel.
		 	*/
			achp = WS_ACTIVECHAN(&Kdws);
			ASSERT(Kdws.w_qp == achp->ch_qp);
			ASSERT(achp->ch_charmap_p != (charmap_t *) NULL);

			kbp = &achp->ch_kbstate;
			cmp = achp->ch_charmap_p;
			kmp = cmp->cr_keymap_p;
			oldprev = kbp->kb_prevscan;
			okbstate = kbp->kb_state;

			/*
	 	 	* ws_scanchar() requires to hold the cr_mutex (charmap
	 	 	* basic lock) on entry to prevent any changes to the 
	 	 	* translation tables.
	 	 	*/
			(void) LOCK(cmp->cr_mutex, plstr);

			/*
	 	 	* Translate scancode into an element in the character
	 	 	* set or a "special" character.
	 	 	*/
			ch = ws_scanchar(cmp, kbp, rawscan, 0);

			/*
	 	 	* Check for handling extended scan codes correctly this
	 	 	* is because ws_scanchar() calls ws_procscan() 
			* on its own. 
	 	 	*/
			if (oldprev == 0xe0 || oldprev == 0xe1)
				kbp->kb_prevscan = oldprev;

			/*
	 	 	* Process scan codes because keyboard generates
	 	 	* extended scancodes.
	 	 	*/
			scan = ws_procscan(cmp, kbp, rawscan);

			UNLOCK(cmp->cr_mutex, plstr);

			if (!kbrk)
				kdkb_keyclick();

			if (kdkb_locked(ch, kbrk)) {
				Kdws.w_intr = 0;
                		RW_UNLOCK(Kdws.w_rwlock, opl);
				continue;
			}

			if (!kbrk) {
				if (WS_SPECIALKEY(kmp, kbp, scan) 
					|| kbp->kb_sysrq) {
					/* w_rwlock may be released 
					 * by kdcksysrq 
					*/
					if(kdcksysrq(cmp, kbp, ch, scan) == 1){
						continue;
					} else {
						ASSERT(getpl() == plstr);
						(void) RW_WRLOCK(Kdws.w_rwlock,
								plstr); 
					}
				}
			} else if (kbp->kb_sysrq && kbp->kb_srqscan == scan) {
				RW_UNLOCK(Kdws.w_rwlock, opl);
				continue;
			}

			/*
	 	 	* If a change occurred in the SCROLL LOCK, CAPS LOCK
	 	 	* or NUM LOCK toggle keys, reprogram the LEDs.
	 	 	*/
			if ((KBTOGLECHANGE(okbstate, kbp->kb_state)) && 
		    	!(KBLEDMASK(kbp->kb_state)))
               			i8042_update_leds(okbstate, kbp->kb_state);

			RW_UNLOCK(Kdws.w_rwlock, opl);

			opl = LOCK(Kdws.w_mutex, plstr);
			while (Kdws.w_timeid) {
				tid = Kdws.w_timeid;
				Kdws.w_timeid = 0;
				UNLOCK(Kdws.w_mutex, opl);
				untimeout(tid);
				opl = LOCK(Kdws.w_mutex, plstr);
			}
			UNLOCK(Kdws.w_mutex, opl);

			/*
	 	 	* Call ws_enque() to store the raw scancode in the
	 	 	* save message. If ws_enque() returns 1, schedule
	 	 	* a timeout() to send a message upstream. Otherwise,
	 	 	* ws_enque() sent the message upstream and their
	 	 	* is no need for the timeout().
	 	 	*/
			if (ws_enque(Kdws.w_qp, &Kdws.w_mp, rawscan, getpl())) {
				opl = LOCK(Kdws.w_mutex, plstr);
				Kdws.w_timeid = 
					itimeout((void(*)())ws_kbtime, &Kdws, 
						HZ/29, plstr);
				UNLOCK(Kdws.w_mutex, opl);
			}
		}

		/* acquire the i8042 mutex lock */
		I8042_LOCK(opl8042);
	}

	Kdws.w_intr = 0;

	i8042_enable_interface();

	/* release the i8042 mutex lock */
	I8042_UNLOCK(opl8042);

	return;
}

/*
 * STATIC void
 * kdmiocdatamsg(queue_t *qp, mblk_t *mp)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC void
kdmiocdatamsg(queue_t *qp, mblk_t *mp)
{	
	/* LINTED pointer alignment */
	if (!((struct copyresp *) mp->b_rptr)->cp_rval)
		/* LINTED pointer alignment */
		ws_iocack(qp, mp, (struct iocblk *)mp->b_rptr);
	else
		freemsg(mp);
}


/*
 * STATIC int
 * kdcksysrq(charmap_t *, kbstate_t *, ushort, unchar)
 *
 * Calling/Exit State:
 *	- Called from kdintr().
 *	- w_rwlock is held in exclusive mode on entry, but 
 *	  is released before returning from the function.
 *	- return 1, if a valid system request, else 0
 *
 * Description:
 *	This routine system requests such as changing VTs
 *	entering the kernel debugger, rebooting the CPU etc.
 *	are processed. It takes as an argument the next scancode
 *	(scan) to process for the system request and its translation 
 *	as a character (ch).
 */
STATIC int
kdcksysrq(charmap_t *cmp, kbstate_t *kbp, ushort ch, unchar scan)
{
	keymap_t *kmp = cmp->cr_keymap_p;
	extern boolean_t kdcnsyscon;

	/*
	 * If the system request is in process, treat the
	 * the scancode being processed as an index into
	 * an srqtab_t structure and set the character ch
	 * equal to the table's at that index.
	 */
	if (kbp->kb_sysrq) {
		kbp->kb_sysrq = 0;
		if (!*(*cmp->cr_srqtabp + scan)) {
			RW_UNLOCK(Kdws.w_rwlock, getpl());
			return (0);
		}
		ch = *(*cmp->cr_srqtabp + scan);
	}

	/*
	 * If the value of the character is a VT switch command
	 * call kdvt_switch() to perform the VT switch and return
	 */
	if (ws_speckey(ch) == HOTKEY) {
		Kdws.w_intr = 0;
		/* returns with the w_rwlock lock unheld. */
		kdvt_switch(ch, getpl());
		return (1);
	}

	switch (ch) {
	case K_DBG:
		kbp->kb_sstate = kbp->kb_state;
		Kdws.w_intr = 0;
		RW_UNLOCK(Kdws.w_rwlock, getpl());
#ifndef NODEBUGGER 
		if (cdebugger != nullsys && kdcnsyscon) {

			/*
			 * Only enable the keyboard interface.
			 *
			 * Note: <i8042_enable_interface> enables both
			 *	 keyboard and auxiliary interface. So we
			 *	 have to explicitly disable the auxiliary
			 *	 interface and enable it later.
			 */
			i8042_enable_interface();
			i8042_disable_aux_interface();
			(*cdebugger)(DR_USER, NO_FRAME);
			i8042_enable_aux_interface();
		}
#endif /* !NODEBUGGER */
		kdnotsysrq(kbp, 0);
		return (1);

	case K_PNC: {
		ws_channel_t	*chp;

		kbp->kb_sstate = kbp->kb_state;
		Kdws.w_intr = 0;
		RW_UNLOCK(Kdws.w_rwlock, getpl());

		if ((console_security & CONS_PANIC_OK) && kdcnsyscon && 
			kd_wsinitflag == 2 && 
			(chp = ws_activechan(&Kdws)) != NULL &&
			(chp->ch_rawmode == 0)) {

			drv_shutdown(SD_PANIC, AD_QUERY);
		}

		kdnotsysrq(kbp, 0);
		return (1);
	}

	case K_RBT: {
		ws_channel_t	*chp;

		if ((console_security & CONS_REBOOT_OK) && kdcnsyscon && 
			kd_wsinitflag == 2 && 
			(chp = ws_activechan(&Kdws)) != NULL &&
			chp->ch_rawmode == 0) {

			RW_UNLOCK(Kdws.w_rwlock, getpl());
			drv_shutdown(SD_SOFT, AD_BOOT);
			return (0);
		}
		RW_UNLOCK(Kdws.w_rwlock, getpl());
		kdnotsysrq(kbp, 0);
		return (1);
	}

	case K_SRQ:
		/*
		 * If the character ch is a beginning of a system
		 * request, then save the keyboard state and
		 * scan code in the kbstate_t structure.
		 */ 
		if (ws_specialkey(kmp, kbp, scan)) {
			kbp->kb_sstate = kbp->kb_state;
			kbp->kb_srqscan = scan;
			kbp->kb_sysrq++;
			Kdws.w_intr = 0;
			RW_UNLOCK(Kdws.w_rwlock, getpl());
			return (1);
		}
		break;

	default:
		break;
	}

	RW_UNLOCK(Kdws.w_rwlock, getpl());

	return (0);
}


/*
 * void
 * kdnotsysrq(kbstate_t *, int)
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
void
kdnotsysrq(kbstate_t *kbp, int sysrqflg)
{
	ushort	msk;
	pl_t	pl;
	toid_t	tid;


	if ((msk = kbp->kb_sstate ^ kbp->kb_state) != 0) {

		pl = LOCK(Kdws.w_mutex, getpl());
		while (Kdws.w_timeid) {
			tid = Kdws.w_timeid;
			Kdws.w_timeid = 0;
			UNLOCK(Kdws.w_mutex, pl);
			untimeout(tid);
			pl = LOCK(Kdws.w_mutex, getpl());
		}
		UNLOCK(Kdws.w_mutex, pl);

		ws_rstmkbrk(Kdws.w_qp, &Kdws.w_mp, 
					kbp->kb_sstate, msk, getpl());
		if (sysrqflg) {
			(void) ws_enque(Kdws.w_qp, &Kdws.w_mp, 
					kbp->kb_srqscan, getpl());
			(void) ws_enque(Kdws.w_qp, &Kdws.w_mp, 
					0x80 | kbp->kb_srqscan, getpl());
		}
		ws_rstmkbrk(Kdws.w_qp, &Kdws.w_mp,
					kbp->kb_state, msk, getpl());
	}
}


/*
 * int
 * kdclrscr(ws_channel_t *, ushort, int)
 * 
 * Calling/Exit State:
 *	- w_rwlock can be held in either exclusive or shared mode.
 *	  In exclusive mode, ch_mutex lock does not need to be held,
 *	  but in shared mode channels (chp) mutex lock is also held.
 *
 * Description:
 *	Clears count successive locations starting at last.
 */
int
kdclrscr(ws_channel_t *chp, ushort last, int cnt)
{
	termstate_t	*tsp;
	unsigned char	c;


	tsp = &chp->ch_tstate;
	c = tsp->t_nfcolor | (tsp->t_nbcolor << 4);

	if (cnt)
		kdv_stchar(chp, last, (ushort)((c << 8) | ' '), cnt);

	return (0);
}


/*
 * int
 * kdtone(wstation_t *, ws_channel_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive/shared mode. 
 *
 * Description:
 *	Implement TCL_BELL functionality. Only do it if an active channel.
 */
int
kdtone(wstation_t *wsp, ws_channel_t *chp)
{
	if (WS_ISACTIVECHAN(wsp, chp))	/* active channel */
		kdkb_tone();

	return (0);
}	


/*
 * int
 * kdshiftset(wstation_t *, ws_channel_t *, int)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared mode.
 *	- ch_mutex basic lock is also held.
 *
 * Description:
 *	Perform a font shift in/shift out if requested by the active
 *	channel.
 */
int
kdshiftset(wstation_t *wsp, ws_channel_t *chp, int dir)
{
	if ((WS_INSYSINIT(wsp)) || (WS_ISACTIVECHAN(wsp, chp)))
		kdv_shiftset(&chp->ch_vstate, dir);

	return (0);

}


/*
 * MACRO
 * KDSETVAL(ushort, unchar, ushort)
 *
 * Calling/Exit State:
 *	- Called by an active channel.
 *	- w_rwlock is held in exclusive/shared mode.
 *	- ch_mutex basic lock is held, if w_rwlock is held in shared mode.
 */
#define KDSETVAL(addr, reglow, val)  { \
		int efl; \
		efl = intr_disable(); \
		outb((addr), (reglow)); \
		outb((addr) + DATA_REG, (val) & 0xFF); \
		outb((addr), (reglow) - 1); \
		outb((addr) + DATA_REG, ((val) >> 8) & 0xFF); \
		intr_restore(efl); \
}


/*
 * int
 * kdsetcursor(ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in shared or exclusive mode.
 *	- When w_rwlock is held in shared mode, then
 *	  chp->ch_mutex basic lock is also held.
 *
 * Description:
 *	If we are the active VT, then set cursor on CRT controller.
 */
int
kdsetcursor(ws_channel_t *chp, termstate_t *tsp)
{
	vidstate_t	*vp = &chp->ch_vstate;
	boolean_t	got_con = B_FALSE;		/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L000 end */

	if (WS_ISACTIVECHAN(&Kdws, chp)) {
		KDSETVAL(vp->v_regaddr, R_CURADRL, tsp->t_cursor);
	} 

	if(got_con){					/* L000 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L000 end */

	return (0);
}


/*
 * int
 * kdsetbase(ws_channel_t *, termstate_t *)
 *
 * Calling/Exit State:
 *	- w_rwlock is held in exclusive/shared mode.
 *	- The chp->ch_mutex basic lock is held, if w_rwlock 
 *	  is held in shared mode.
 *
 * Description:
 *	If we are the active VT, then set start address of CRT 
 *	controller from the current base (tsp->t_origin).
 */
int
kdsetbase(ws_channel_t *chp, termstate_t *tsp)
{
	vidstate_t	*vp = &chp->ch_vstate;
	boolean_t	got_con = B_FALSE;		/* L000 begin */

	if(!kd_con_acquired)
		if(ws_con_need_text())
			return 0;
		else{
			got_con = B_TRUE;
			kd_con_acquired = B_TRUE;
		}					/* L000 end */

	if ((WS_INSYSINIT(&Kdws)) || (WS_ISACTIVECHAN(&Kdws, chp))) {
		KDSETVAL(vp->v_regaddr, R_STARTADRL, tsp->t_origin);
	}

	if(got_con){					/* L000 begin */
		ws_con_done_text();
		kd_con_acquired = B_FALSE;
	}						/* L000 end */

	return (0);
}


/*
 * unchar *
 * kd_vdc800_ramd_p(void)
 *	VDC800 hook 
 *
 * Calling/Exit State:
 * TBD.
 */
unchar *
kd_vdc800_ramd_p(void)
{
	ws_channel_t	*achp;
	vidstate_t	*vp;


	achp = WS_ACTIVECHAN(&Kdws);
	vp = &achp->ch_vstate;
	return (&kd_ramdactab[0] + (WSCMODE(vp)->m_ramdac * 0x300));
}


/*
 * int
 * kd_vdc800_access(void)
 * 
 * Calling/Exit State:
 *	None.
 */
int
kd_vdc800_access(void)
{
	dev_t devp;


	if (ws_getctty(&devp) || (getminor(devp) != Kdws.w_active))
		return (1);

	kd_vdc800.procp = proc_ref();

	return (0);
}


/*
 * void
 * kd_vdc800_release(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
kd_vdc800_release(void)
{
	proc_unref(kd_vdc800.procp);
	kd_vdc800.procp = (struct proc *) 0;
	return;
}


/*
 * int 
 * kdnoop(void)
 *
 * Calling/Exit State:
 *	None.
 */
int
kdnoop(void)
{
	return 0;
}


/*
 * void
 * kd_kbio_setmode(queue_t *, mblk_t *, struct iocblk *)
 *
 * Calling/Exit State:
 *	- No locks are held ont entry/exit.
 */
void
kd_kbio_setmode(queue_t *qp, mblk_t *mp, struct iocblk *iocp)
{
	ws_channel_t	*chp = (ws_channel_t *)qp->q_ptr;
	int		kbmode;
	extern void	switch_kb_mode();
	pl_t		opl;


	if (!mp->b_cont) {
		ws_iocnack(qp, mp, iocp, EINVAL);
		return;
	}

	opl = LOCK(chp->ch_mutex, plstr);

	if (!(WS_ISACTIVECHAN(&Kdws, chp))) {
		UNLOCK(chp->ch_mutex, opl);
		ws_iocnack(qp, mp, iocp, EINVAL);
		return;
	}

	/* LINTED pointer alignment */
	kbmode = *(int *) mp->b_cont->b_rptr;

	switch(kbmode) {
	case KBM_AT:
		UNLOCK(chp->ch_mutex, opl);
#ifndef AT_KEYBOARD
		ws_iocnack(qp, mp, iocp, EINVAL);
		return;
#endif /* AT_KEYBOARD */
    
	case KBM_XT:
		chp->ch_charmap_p->cr_kbmode = kbmode;
		switch_kb_mode(kbmode);
		UNLOCK(chp->ch_mutex, opl);
		ws_iocack(qp, mp, iocp);
		break;

	default:
		UNLOCK(chp->ch_mutex, opl);
		ws_iocnack(qp, mp, iocp, EINVAL);
	}
}
