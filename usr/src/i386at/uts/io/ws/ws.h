#ifndef	_IO_WS_WS_H	/* wrapper symbol for kernel use */
#define	_IO_WS_WS_H	/* subject to change without notice */

#ident	"@(#)ws.h	1.35"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/kd/kd.h>		/* REQUIRED */
#include <io/strtty.h>		/* REQUIRED */
#include <io/xque/xque.h>	/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */
#include <proc/pid.h>		/* REQUIRED */
#ifndef NO_MULTI_BYTE
#include <io/ldterm/eucioctl.h>
#endif /* NO_MULTI_BYTE */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/kd.h>		/* REQUIRED */
#include <sys/strtty.h>		/* REQUIRED */
#include <sys/xque.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */
#include <sys/pid.h>		/* REQUIRED */
#ifndef NO_MULTI_BYTE
#include <sys/eucioctl.h>
#endif /* NO_MULTI_BYTE */

#endif /* _KERNEL_HEADERS */

/*
 * Definitions for the IWE.
 */
#ifndef CONSIOC
/*
 * Note this must be kept the same value as the version in kd.h
 */
#define CONSIOC		('c' << 8)	/* Upper byte of console ioctl's */
#endif

#define CON_MEM_MAPPED	(CONSIOC | 80)	/* Console memory mapped to a proc */

#define CON_LWP_NO_BIND	0x01

/*
 * The size of cno_mem_map_req structure is 0x0C (12) bytes
 */
typedef struct con_mem_map_req {

	size_t	length;			/* Requested length */
	paddr_t	address;		/* Physical address to map */
	pid_t	proc_id;		/* PID of calling process */

} con_mem_map_req_t;


#if defined(_KERNEL) || defined(_KMEMUSER)

#define	MAXVIDEOSEGS		10

/*
 * The size of map_info stucture is 0xAC (172) bytes.
 */
struct map_info {
	int	m_cnt;		/* 0x00: cnt of no. of mem. locations mapped */
	void	*m_procp;	/* 0x04: process with display mapped */
	int	m_chan;		/* 0x08: channel that owns the map currently */
	struct kd_memloc m_addr[MAXVIDEOSEGS]; /* 0x0C: display mapping info */
};

/*
 * The size of con_init_state structure is 0x28 (40) bytes
 */
typedef struct con_init_state con_init_state_t;

struct con_init_state {

	int	(*driver_init)(con_init_state_t *);	/* Driver init callback
							 * function
							 */

	int	(*on_proc)(void *);		/* LWP on proc handler */
	int	(*off_proc)(void *);		/* LWP off proc handler */
	int	(*release_access)(void *);	/* Release access handler */
	int	(*resume_access)(void *);	/* Resume access handler */
	void	*driver_data;			/* Pointer to opaque driver
						 * specific object
						 */
	struct iocblk	*iocbp;			/* Ptr to IOCTL data */
	mblk_t	*mbp;				/* Ptr to message block */
	queue_t	*qp;				/* STREAMS queue pointer */
	uint_t	flags;				/* Init option flags */

};

#define	T_ANSIMVBASE	0x00001
#define	T_BACKBRITE	0x00002
#define T_NOBACKBRITE   0x00004
#ifndef NO_MULTI_BYTE
#define	T_ALTCHARSET	0x00008	/* alternate character set selected */
#endif /* NO_MULTI_BYTE */

#define ANSI_FGCOLOR_BASE       30
#define ANSI_BGCOLOR_BASE       40


/*
 * This structure represents the tty state of the channel. The screen
 * contents of the tty is viewed as a linear array of characters (rows
 * by column long), and screen position is represented as an index into
 * this array. Associated with each character is a display attribute
 * value defining, among other things, foreground and background color,
 * should the character be underlined or blinking etc.
 *
 * The termstate structure contains members describing the default display
 * attribute, current display attribute, the cursor type (block, underline),
 * and position, thre screen dimensions (rows and columns), and information
 * about the tab stops. 
 *
 * The size of kbstate stucture is 0x54 (84) bytes.
 */
struct termstate {
	int	t_flags;	/* 0x00: terminal state flags -- see above */
	uchar_t	t_font;		/* 0x04: selected font */
	uchar_t	t_curattr;	/* 0x05: current attribute */
	uchar_t	t_normattr;	/* 0x06: normal character attribute */
	uchar_t	t_undstate;	/* 0x07: underline state */
	ushort	t_rows;		/* 0x08: no. of characters vertically */
	ushort	t_cols;		/* 0x0A: no. of characters horizontally */
	ushort	t_scrsz;	/* 0x0C: no. of characters (ch_cols*ch_rows) */
	ushort	t_origin;	/* 0x0E: upper left corner of screen buffer */
	ushort	t_cursor;	/* 0x10: cursor position (0-based) */
	ushort	t_curtyp;	/* 0x12: cursor type 1==block, 0==underline */
	ushort	t_row;		/* 0x14: current row */
	ushort	t_col;		/* 0x16: current column */
	ushort	t_sending;	/* 0x18: sending screen */
	ushort	t_sentrows;	/* 0x1A: rows sent */
	ushort	t_sentcols;	/* 0x1C: cols sent */
	uchar_t	t_pstate;	/* 0x20: parameter parsing state */
	uchar_t	t_ppres;	/* 0x21: does output ESC seq have a param */
	ushort	t_pcurr;	/* 0x22: value of current param */
	ushort	t_pnum;		/* 0x24: current param # of ESC sequence */
	ushort	t_ppar[5];	/* 0x26: parameters of ESC sequence */
	struct attrmask *t_attrmskp; /* 0x30: pointer to attribute mask array */
	uchar_t	t_nattrmsk;	/* 0x34: size of attribute mask array */
	uchar_t	t_ntabs;	/* 0x35: number of tab stops set */
	uchar_t	*t_tabsp;	/* 0x38: list of tab stops */
        ushort  t_bell_time;	/* 0x3C: */
	ushort  t_bell_freq;	/* 0x3E: */
        ushort	t_saved_row;	/* 0x40: */
	ushort	t_saved_col;    /* 0x42: saved cursor position   */
        uchar_t t_battr;        /* 0x44: Bold & Blink attributes */
        uchar_t t_cattr;        /* 0x45: Color/Reverse attributes */
        ushort  t_nfcolor;      /* 0x46: normal foreground color */
	ushort	t_nbcolor;      /* 0x48: normal background color */
	ushort	t_rfcolor;      /* 0x4A: reverse foreground video color */
	ushort	t_rbcolor;      /* 0x4C: reverse background video color */
	ushort	t_gfcolor;      /* 0x4E: graphic foreground character color */
	ushort	t_gbcolor;      /* 0x50: graphic background character color */
        uchar_t t_auto_margin;	/* 0x52: */

};

typedef struct termstate termstate_t;


/*
 * The vidstate structure represents the state of the video device. 
 * CGA/EGA/VGA devices all support the concept of video modes. Some
 * are text modes, on which standard tty output can be done (a standard
 * text mode is 80x25 color text). Other modes are graphics modes, 
 * which do not support tty-type output but rather are pixed based
 * (i.e. 640x400 16-color graphics)
 *
 * This structure contains the information about the current and default
 * video modes, the current font selected, pointers to other available
 * fonts, information about the video hardware (what controller type),
 * pointers to the parameter tables used in programming the controller
 * for the different video modes, pointers to the screen memory used
 * to store the sceen image etc.
 *
 * The size of vidstate stucture is 0xA8 (168) bytes.
 */
struct vidstate {
	uchar_t	v_cmos;		/* 0x00: cmos video controller value */
	uchar_t	v_type;		/* 0x01: video controller type */
	uchar_t	v_cvmode;	/* 0x02: current video mode */
	uchar_t	v_dvmode;	/* 0x03: default video mode */
	uchar_t	v_font;		/* 0x04: current font loaded */
	uchar_t	v_colsel;	/* 0x05: color select register byte */
	uchar_t	v_modesel;	/* 0x06: mode register byte */
	uchar_t	v_undattr;	/* 0x07: underline attribute */
	uchar_t	v_uline;	/* 0x08: underline status (on or off) */
	uchar_t	v_nfonts;	/* 0x09: number of fonts */
	uchar_t	v_border;	/* 0x0A: border attribute */
	ushort	v_scrmsk;	/* 0x0C: mask for placing text in screen mem. */
	ushort	v_regaddr;	/* 0x0E: addr of corresponding M6845 or EGA */
	uchar_t	**v_parampp;	/* 0x10 pointer to video parameters table */
	struct font_info *v_fontp; /* 0x14: pointer to font information */
	caddr_t	v_rscr;		/* 0x18: "real" address of screen memory */
	ushort	*v_scrp;	/* 0x1C: pointer to video memory */
	int	v_modecnt;	/* 0x20: number of modes supported */
	struct modeinfo *v_modesp; /* 0x24: ptr to video mode info table */
	ushort	v_ioaddrs[MKDIOADDR]; /* 0x28: valid I/O addresses */
};

typedef struct vidstate vidstate_t;


/*
 * The KD keyboards are not the terminal keyboards in that terminal
 * keyboards generate input in the form of characters in a character
 * set (ASCII for U.S. terminals), while KD keyboards generater scan
 * codes, one for the "make" (i.e. press) and one for the "break" of
 * the key (release). Theses scan codes are not a character set, but
 * must be translated by the system into whatever character set the
 * system uses. The keyboard is a dump device, and even leaves inter-
 * pretation of of the SHIFT, ALT and CTRL keys to the system.
 *
 * The kbstate structure represents the current state of the keyboard,
 * containing information about whether the shift-type keys (CTRL, ALT
 * and SHIFT) are pressed or the "toggle" keys (SCROLL LOCK, NUM LOCK,
 * CAPS LOCK) are enabled.
 *
 * The size of kbstate stucture is 0x14 (20) bytes.
 */
struct kbstate {
	uchar_t	kb_sysrq;	/* 0x00: true if last character was K_SRQ */
	uchar_t	kb_srqscan;	/* 0x01: scan code of K_SRQ */
	uchar_t	kb_prevscan;	/* 0x02: previous scancode */
	ushort	kb_state;	/* 0x04: keyboard shift/ctrl/alt state */
	ushort	kb_sstate;	/* 0x06: saved keyboard shift/ctrl/alt state */
	ushort	kb_togls;	/* 0x08: caps/num/scroll lock toggles state */
	int	kb_extkey;	/* 0x0C: extended key enable state */
	int	kb_altseq;	/* 0x1C: used to build extended codes */
};

typedef struct kbstate kbstate_t;


/*
 * A translation table used when extended key processing (a XENIX feature
 * enabled via the TIOCKBON ioctl) is activated for special processing of
 * ALT key sequences and number-pad sequences.
 */
typedef uchar_t	extkeys_t[NUM_KEYS+1][NUMEXTSTATES];


/*
 * The KD has fewer scan codes available to it than keys on the keyboard.
 * Therefore, some keys generate an "escape" scan code and then a reused
 * scan code (meaning another key may generate the same scan code) for
 * each make/break of the key. This data structure is a table used in
 * that processing. It maps the reused scan codes to a range of values
 * that the keyboard itself cannot produce (called extended scan codes).
 */
typedef uchar_t	esctbl_t[ESCTBLSIZ][2];


/*
 * Kana table to map raw scan codes that are already used in the escape
 * table to other free map table indices.
 */
typedef uchar_t esctbl2_t[ESCTBL2SIZ][2];


/*
 * This structure is used to perform "prefixed" character processing.
 * Certain <ALT> key combinations produce special characters like K_ESN
 * or K_ESO, which in turn as part of the character translation generate
 * as input escape sequence-prefixed strings.
 */
struct pfxstate {
	uchar_t val;
	uchar_t type;
};

typedef struct pfxstate pfxstate_t[K_PFXL - K_PFXF + 1];


/*
 * The charmap structure contains pointer to structures used in scan
 * code translation process.
 *
 * Note: <cr_defltp> may point to itself if the structure represents
 * the default value.
 *
 * The size of charmap stucture is 0x30 (48) bytes.
 */
struct charmap {
	lock_t		*cr_mutex;	/* 0x00: charmap basic spin lock */
	keymap_t	*cr_keymap_p;	/* 0x04: scancode to char set mapping */
	extkeys_t	*cr_extkeyp;	/* 0x08: extended code mapping */
	esctbl_t	*cr_esctblp;	/* 0x0C: 0xe0 pref. scan code mapping */
	strmap_t	*cr_strbufp;	/* 0x10: function key mapping */
	srqtab_t	*cr_srqtabp;	/* 0x14: sysrq key mapping */
	stridx_t	*cr_strmap_p;	/* 0x18: string buffer */
	pfxstate_t	*cr_pfxstrp;	/* 0x1C: */
	struct charmap	*cr_defltp;	/* 0x20: ptr to default info for ws */
        int		cr_flags;	/* 0x24: */
        int		cr_kbmode;      /* 0x28: kbd is in KBM_AT/KBM_XT mode */
        esctbl2_t	*cr_esctbl2p;	/* 0x2C: kana scan codes mapping */
};

typedef struct charmap charmap_t;


/*
 * The scrn structure contains the XENIX screen map for the workstation
 * or channel. If contains NULL (default value) or a pointer to a 
 * scrnmap_t structure (an array of characters) and a pointer to the
 * default scrn_t structure for the workstation.
 *
 * The size of scrn stucture is 0xC (12) bytes.
 */
struct scrn {
	lock_t		*scr_mutex;	/* srcn basic spin lock */
	scrnmap_t	*scr_map_p;
	struct scrn	*scr_defltp;
};

typedef struct scrn scrn_t;


/*
 * The ws_channel_info data structure is used by KD and WS to represent a
 * channel on a workstation. It contains a pointer to its STREAM read
 * queue, its channel number, assorted flags, a pointer to its workstation
 * structure, and back/forward pointers to other channels in use on the
 * workstation (they point back on the channel if this channel is the 
 * only channel being used).
 *
 * It also contains its own copies of a kbstate_t, scrn_t, vidstate_t,
 * termstate_t, and strtty structure, as well as a pointer to a charmap_t
 * structure. The implication of the channel structure containing its
 * own copies of these structures are that the associated states are
 * maintained on a per-channel basis.
 *
 * The KD driver defines the ws_channel_t structure for the home VT (vt00)
 * statically, so that it is always allocated. The structure for the
 * other VTs will be allocated dynamically as the VTs are opened. VT00
 * is treated specially because it will always need to be initialized
 * to deal with cmn_err() messages adn I/O from the kernel debugger. 
 *
 * The size of ws_channel_info stucture is 0x1FC (508) bytes.
 */
struct ws_channel_info {
	lock_t	*ch_mutex;	/* 0x00: channel basic spin lock */
	sv_t	*ch_wactsv;	/* 0x04: wait activation synch. variable */
	sv_t	*ch_xquesv;	/* 0x08: X/que synch. variable */
	sv_t	*ch_qrsvsv;	/* 0x0C: queue reserve synch. variable */
	queue_t	*ch_qp;		/* 0x10: channel read queue pointer */
	int	ch_opencnt;	/* 0x14: number of opens */
	int	ch_closing;	/* 0x18: indicates the channel is closing */
	int	ch_id;		/* 0x1C: channel id number */
	int	ch_rawmode;	/* 0x20: channel id number in raw mode */
	int	ch_slpaddr;	/* 0x24: address to sleep on */
	int	ch_flags;	/* 0x28: channel flags */
	void	*ch_procp;	/* 0x2C: process reference pointer */
	toid_t	ch_timeid;	/* 0x30: timeout id for channel switching */
	int	ch_relsig;	/* 0x34: release signal */
	int	ch_acqsig;	/* 0x38: acquire signal */
	int	ch_frsig;	/* 0x3C: free signal */
	uchar_t	ch_dmode;	/* 0x40: current display mode */
	struct wstation	*ch_wsp;/* 0x44: backward reference to the ws */ 
	kbstate_t ch_kbstate;	/* 0x48: keyboard state information */
	charmap_t *ch_charmap_p;/* 0x5C: character mapping tables */
	scrn_t	ch_scrn;	/* 0x60: channel screen buffer information */
	vidstate_t ch_vstate;	/* 0x6C: channel video state information */
	termstate_t ch_tstate;	/* 0x114: terminal state for this channel */
	struct strtty ch_strtty;/* 0x168: channel tty information */
	xqInfo	ch_xque;	/* 0x1B8: channel X/que informaton */
	struct ws_channel_info *ch_nextp;	/* 0x1EC: next chan. in linked list */
	struct ws_channel_info *ch_prevp;	/* 0x1F0: prev. chan. in linked list */
	mblk_t	*ch_iocarg;	/* 0x1F4: */
#ifdef MERGE386
	struct mcon *ch_merge;	/* 0x1F8: pointer to merge console structure */
#endif /* MERGE386 */
#ifndef NO_MULTI_BYTE
	eucioc_t *ch_euc_info;	/* EUC width structure */
	wchar_t	*ch_scrbuf;	/* screen buffer */
	uchar_t	*ch_attrbuf;	/* char attribute buffer */
	uchar_t	ch_fontinfo[4];	/* codeset font number */
	int	ch_cursor;	/* current cursor position */
	wchar_t	ch_wc;		/* current wide char */
	uchar_t	ch_wc_attr;	/* attribute associated with ch_wc */
	wchar_t	ch_wc_mask;	/* EUC mask associated with ch_wc */
	int	ch_wc_len;	/* remaining length for ch_wc */
	uchar_t	ch_scrw;	/* number of columns ch_wc occupies */
#endif /* NO_MULTI_BYTE */
};

typedef struct ws_channel_info ws_channel_t;


/*
 * The wstation structure contains state information about the workstation
 * such as the number of channels set up (and pointers to the associated
 * ws_channel_t structures), as well as copies of the primitive structures
 * above that contain the default state information that is copied to each
 * ws_channel_t as new channels are opened. The structure also contains list
 * of pointers to the scree buffers used to hold the screen image for a
 * channel when it is not active.
 *
 * An instance of this structure (the variable Kdws) is used by KD to
 * represent the console workstation state. 
 *
 * The size of wstation stucture is 0x24C (588) bytes.
 */
struct wstation {
	rwlock_t *w_rwlock;	/* 0x00: workstation reader/writer lock */
	lock_t  *w_mutex;	/* 0x04: workstation basic spin lock */
	sv_t	*w_tonesv;	/* 0x08: tone synchronization variable */
	sv_t	*w_flagsv;	/* 0x0C: modesw/chansw/mapdisp synch. var. */
	int	w_init;		/* 0x10: workstation has been initialized */
	volatile int w_intr;	/* 0x14: indicates interrupt processing */
	int	w_active;	/* 0x18: active channel */
	int	w_nchan;	/* 0x1C: number of channels */
	int	w_ticks;	/* 0x20: used for BELL functionality */
	int	w_tone;		/* 0x24: */
	int	w_flags;	/* 0x28: */
	int	w_noacquire;	/* 0x2C: */
	int	w_wsid;		/* 0x30: */
	int	w_forcechan;	/* 0x34: */
	toid_t	w_forcetimeid;	/* 0x38: */
	int	w_lkstate;	/* 0x3C: */
	int	w_clkstate;	/* 0x40: */	
	uchar_t	w_kbtype;	/* 0x44: */
	uchar_t	w_dmode;	/* 0x45: */
	queue_t	*w_qp;		/* 0x48: ptr to queue for this minor device */
	mblk_t	*w_mp;		/* 0x4C: ptr to current message block */
	toid_t	w_timeid;	/* 0x50: id for pending timeouts */
	caddr_t	w_private;	/* 0x54: used for any ws specific info */
	ws_channel_t **w_chanpp;	/* 0x58: */
	ws_channel_t *w_switchto;	/* 0x5C: */
	ushort_t **w_scrbufpp;	/* 0x60: */
	scrn_t	w_scrn;		/* 0x64: */
	vidstate_t w_vstate;	/* 0x70: workstation video state information */
	termstate_t w_tstate;	/* 0x118: */
	struct map_info	w_map;	/* 0x16C: video buffer mapping information */
	charmap_t w_charmap;	/* 0x218: default charmap for workstation */
	struct kdcnops *w_consops; /* 0x248 */
};

typedef struct wstation wstation_t;


/*
 * The kdcnops structure contains a list of pointers to functions
 * used to indirectly call routines in KD to perform operations such
 * as "sound the bell", "activate the channel", "set cursor type", etc.
 */
struct kdcnops {
	int	(*cn_stchar)(ws_channel_t *, ushort_t, ushort_t, int);
	int	(*cn_clrscr)(ws_channel_t *, ushort_t, int);
	int	(*cn_setbase)(ws_channel_t *, termstate_t *);
	int	(*cn_activate)(ws_channel_t *, int);
	int	(*cn_setcursor)(ws_channel_t *, termstate_t *);
	int	(*cn_bell)(wstation_t *, ws_channel_t *);
	int	(*cn_setborder)(ws_channel_t *, long);
	int	(*cn_shiftset)(wstation_t *, ws_channel_t *, int);
	int	(*cn_mvword)(ws_channel_t *, ushort_t, ushort_t, int, char);
	int	(*cn_undattr)(wstation_t *, ws_channel_t *, ushort_t *, short);
	int	(*cn_rel_refuse)(void);
	int	(*cn_acq_refuse)(ws_channel_t *);
	int	(*cn_scrllck)(void);
	int	(*cn_cursortype)(wstation_t *, ws_channel_t *, termstate_t *);
	int	(*cn_unmapdisp)(ws_channel_t *, struct map_info *);
#ifndef NO_MULTI_BYTE
	void    (*cn_gcl_norm)(struct kdcnops *, ws_channel_t *, termstate_t *,
				ushort);
	void    (*cn_gcl_handler)(wstation_t *, mblk_t *, termstate_t *,
				ws_channel_t *);
	void    (*cn_gdv_scrxfer)(ws_channel_t *, int);
	int     (*cn_gs_alloc)(struct kdcnops *, ws_channel_t *, termstate_t *);
	void    (*cn_gs_free)(struct kdcnops *, ws_channel_t *, termstate_t *);
	void    (*cn_gs_chinit)(wstation_t *, ws_channel_t *);
	int     (*cn_gs_seteuc)(ws_channel_t *, struct eucioc *);
	int	(*cn_gs_ansi_cntl)(struct kdcnops *, ws_channel_t *,
				termstate_t *, ushort);
#endif /* NO_MULTI_BYTE */
};

typedef struct kdcnops kdcnops_t;


#define WS_NOTINITED	0x00		/* ws is not initialized */
#define WS_ININIT	0x01		/* ws is being initialzied */
#define WS_INITED	0x02		/* ws is fully initialized */

#define	WSCMODE(x)	((struct modeinfo *)x->v_modesp + x->v_cvmode)
#define WSMODE(x, n)	((struct modeinfo *)x->v_modesp + n)
#define WSNTIM		-1

#define HOTKEY		0x10000

#define	WS_NOMODESW	0x01
#define WS_NOCHANSW	0x02
#define WS_LOCKED	0x04
#define WS_KEYCLICK	0x08
#define WS_NOMAPDISP	0x10
#define	WS_NOFONTMOD	0x20

#define CH_MAPMX	MAXVIDEOSEGS

#define CHN_UMAP	0x001
#define CHN_XMAP	0x002
#define CHN_QRSV	0x004
#define CHN_ACTV	0x008
#define CHN_PROC	0x010
#define CHN_WAIT	0x020
#define CHN_HIDN	0x040
#define CHN_WACT	0x080
#define CHN_KILLED	0x100

#define CHN_MAPPED	(CHN_UMAP | CHN_XMAP)

#define CHNFLAG(x, y)	(x->ch_flags & y)

#endif /* _KERNEL || _KMEMUSER */

#define	WS_MAXCHAN	15


#ifdef _KERNEL

#define WS_INKDINTR(wsp) \
	((wsp)->w_intr) 

#define WS_ISINITED(wsp) \
	((wsp)->w_init == WS_INITED)

#define WS_ISNOTINITED(wsp) \
	((wsp)->w_init == WS_NOTINITED)

/* required to support one kernel printf */
#define	WS_INSYSINIT(wsp)	WS_ISNOTINITED((wsp))

#define	WS_SPECIALKEY(kmp, kbp, scan) \
	(IS_SPECKEY((kmp), (scan), ws_getstate((kmp), (kbp), (scan))))

#define	WS_TRANSCHAR(kmp, kbp, scan) \
	((ushort)(kmp)->key[(scan)].map[ws_getstate((kmp), (kbp), (scan))])

#define	WS_EXT(extkeyp, scan, estate) \
	(*((extkeyp) + (scan) * NUMEXTSTATES + (estate)))

#define	WS_ACTIVECHAN(wsp) \
	(ASSERT((wsp)->w_init == WS_INITED), \
	 ASSERT((wsp)->w_chanpp != NULL), \
	 ASSERT(((ws_channel_t *)*((wsp)->w_chanpp + (wsp)->w_active))), \
	 ((ws_channel_t *)*((wsp)->w_chanpp + (wsp)->w_active)))

#define WS_ISACTIVECHAN(wsp, chp) \
	(((ws_channel_t *)*((wsp)->w_chanpp + (wsp)->w_active)) == (chp))
		
#define	WS_GETCHAN(wsp, chan) \
	(ASSERT((wsp)->w_init == WS_INITED), \
	 ASSERT((wsp)->w_chanpp != NULL), \
	 ((ws_channel_t *)*((wsp)->w_chanpp + (chan)))

struct ch_protocol;

/*
 * ws_cmap prototype declaration
 */
extern stridx_t *ws_dflt_strmap_p(void);
extern int	ws_newscrmap(scrn_t *, int);
extern int	ws_newkeymap(charmap_t *, ushort_t, keymap_t *, int, pl_t);
extern int	ws_newsrqtab(charmap_t *, int, pl_t);
extern int	ws_newstrbuf(charmap_t *, int, pl_t);
extern void	ws_strreset(charmap_t *);
extern int	ws_addstring(charmap_t *, ushort_t, uchar_t *, ushort_t);
extern int	ws_newpfxstr(charmap_t *, int, pl_t);
extern void	ws_scrn_init(wstation_t *, int);
extern void	ws_cmap_init(wstation_t *, int);
extern void	ws_scrn_alloc(wstation_t *, ws_channel_t *);
extern charmap_t *ws_cmap_alloc(wstation_t *, int);
extern void	ws_scrn_free(wstation_t *, ws_channel_t *);
extern void	ws_cmap_free(wstation_t *, charmap_t *);
extern void	ws_scrn_reset(wstation_t *, ws_channel_t *);
extern void	ws_cmap_reset(wstation_t *, charmap_t *);
extern void	ws_kbtime(wstation_t *);
extern int	ws_enque(queue_t *, mblk_t **, uchar_t, pl_t);
extern uchar_t	ws_procscan(charmap_t *, kbstate_t *, uchar_t);
extern void	ws_rstmkbrk(queue_t *, mblk_t **, ushort_t, ushort_t, pl_t);
extern void	ws_stnontog(queue_t *, mblk_t **, ushort_t, ushort_t, pl_t);
extern ushort	ws_getstate(keymap_t *, kbstate_t *, uchar_t);
extern void	ws_xferkbstat(kbstate_t *, kbstate_t *);
extern ushort	ws_transchar(keymap_t *, kbstate_t *, uchar_t);
extern int	ws_statekey(ushort, charmap_t *, kbstate_t *, uchar_t);
extern int	ws_specialkey(keymap_t *, kbstate_t *, uchar_t);
extern ushort	ws_shiftkey(ushort_t, uchar_t, keymap_t *, kbstate_t *, uchar_t);
extern int	ws_speckey(ushort_t);
extern uchar_t	ws_ext(uchar_t *, ushort_t, ushort_t);
extern ushort	ws_extkey(uchar_t, charmap_t *, kbstate_t *, uchar_t);
extern ushort	ws_esckey(ushort_t, uchar_t, charmap_t *, kbstate_t *, uchar_t);
extern ushort	ws_scanchar(charmap_t *, kbstate_t *, uchar_t, uint);
extern int	ws_toglchange(ushort_t, ushort_t);
extern uchar_t	ws_getled(kbstate_t *);

/*
 * ws_subr prototype declaration
 */
extern ws_channel_t *ws_activechan(wstation_t *);
extern ws_channel_t *ws_getchan(wstation_t *, int);
extern int	ws_freechan(wstation_t *);
extern int	ws_getchanno(minor_t);
extern int	ws_getws(minor_t);
extern int	ws_alloc_attrs(wstation_t *, ws_channel_t *, int);
extern void	ws_chinit(wstation_t *, ws_channel_t *, int);
extern void	ws_openresp(queue_t *, mblk_t *, struct ch_protocol *, 
					ws_channel_t *, unsigned long);
extern void	ws_openresp_chr(queue_t *, mblk_t *, struct ch_protocol *, 
					ws_channel_t *);
extern void	ws_preclose(wstation_t *, ws_channel_t *);
extern void	ws_closechan(queue_t *, wstation_t *, ws_channel_t *, mblk_t *);
extern int	ws_activate(wstation_t *, ws_channel_t *, int);
extern int	ws_switch(wstation_t *, ws_channel_t *, int);
extern int	ws_procmode(wstation_t *, ws_channel_t *);
extern void	ws_automode(wstation_t *, ws_channel_t *);
extern void	ws_xferwords(ushort_t *, ushort_t *,  int, char);
extern void	ws_setlock(wstation_t *, int);
extern void	ws_force(wstation_t *, ws_channel_t *, pl_t);
extern void	ws_mctlmsg(queue_t *, mblk_t *);
extern void	ws_notifyvtmon(ws_channel_t *, uchar_t);
extern void	ws_iocack(queue_t *, mblk_t *, struct iocblk *);
extern void	ws_iocnack(queue_t *, mblk_t *, struct iocblk *, int);
extern void	ws_copyout(queue_t *, mblk_t *, mblk_t *, uint_t);
extern void	ws_mapavail(ws_channel_t *, struct map_info *);
extern int	ws_notify(ws_channel_t *, int);
extern int	ws_queuemode(ws_channel_t *, int, int);
extern int	ws_xquemsg(ws_channel_t *, long);
extern int	ws_ck_kd_port(vidstate_t *, ushort_t);
extern int	ws_getctty(dev_t *);
extern void	ws_scrnres(ulong_t *, ulong_t *);
extern int	ws_getvtdev(dev_t *);

/* Multiconsole support function prototypes */

extern int	ws_con_drv_init(con_init_state_t *);
extern int	ws_con_need_text(void);
extern int	ws_con_done_text(void);
extern void	ws_con_drv_detach(void *, pid_t);

/*
 * ws_ansi prototype declarations
 */
extern int	wsansi_parse(kdcnops_t *, ws_channel_t *, uchar_t *, int);
extern int	wsansi_cntl(kdcnops_t *, ws_channel_t *, termstate_t *, ushort_t);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_WS_WS_H */
