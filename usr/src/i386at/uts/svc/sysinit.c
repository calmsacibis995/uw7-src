#ident	"@(#)kern-i386at:svc/sysinit.c	1.74.21.6"
#ident	"$Header$"

/*
 * Machine dependent system initialization.
 */

#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <util/inline.h>
#include <util/types.h>
#include <mem/kma.h>
#include <mem/immu.h>
#include <mem/pmem.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/seg_kmem.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <util/plocal.h>
#include <util/ksynch.h>
#include <util/engine.h>
#include <proc/signal.h>
#include <proc/lwp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/disp.h>
#include <proc/class.h>
#include <proc/cg.h>
#include <proc/cguser.h>
#include <io/conf.h>
#include <io/conssw.h>
#include <util/var.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/ipl.h>
#include <util/mod/mod_intr.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <svc/creg.h>
#include <svc/fp.h>
#include <svc/intr.h>
#include <svc/msr.h>
#include <svc/sysinit_sync.h>
#include <svc/systm.h>
#include <svc/trap.h>
#include <svc/psm.h>
#include <svc/utsname.h>
#include <mem/kmem.h>
#include <mem/hatstatic.h>
#include <util/kdb/xdebug.h>
#include <util/ghier.h>
#include <svc/copyright.h>
#include <proc/usync.h>
#include <proc/tss.h>
#include <svc/v86bios.h>

#include <io/f_ddi.h>

#ifdef CCNUMA

#ifdef DEBUG_SBSP
#define graffiti() remote_graffiti(0xFF00000 + __LINE__)
extern lock_t panic_lock;
#else /* DEBUG_SBSP */
#define graffiti() ((void)0)
#endif /* DEBUG_SBSP */


STATIC sync_control_t sync_control;
STATIC void cg_setup_local(cgnum_t);

#endif /* CCNUMA */

#define SYNC_MSEC	1000		/* 1000 usec == 1 msec */
#define SYNC_TIMEOUT	(100 * 1000)	/* 30 seconds (??) */

#ifdef CCNUMA

void cg_clonept(cgnum_t, vaddr_t, vaddr_t, boolean_t);

#endif /* CCNUMA */

extern void double_panic(kcontext_t *);
extern void xcall_init(void);
extern void detect_cpu(void);
extern int detect_fpu(void);
extern void dma_init(void);
extern void prime_reset_code(void (*)(void));
extern void _start(void);
extern void (*reset_code)(void);
extern vaddr_t physmap2(paddr_t, vaddr_t);
extern void bs_reloc(void);

#ifdef USE_GDB
/* for gdb to work */
extern void _catchException1();
extern void _catchException3();
extern void _catchException11();
extern void _catchException13();
extern void _catchException14();
extern void gdb_init();
#endif

STATIC int chip_detect(void);
ullong_t remap_pagetable(vaddr_t, ullong_t);
STATIC void init_desc_tables(void);
STATIC void tmp_init_desc_tables(void);
STATIC void setup_priv_maps(int, int);
STATIC void pae_setup_priv_maps(int, int);
void sim_cg_init(void);
int initpsm(void);
void sysinit_final(void);
void p6dosplit(void);		/* hack code, must be removed later */
void p6memselfinit(void);	/* hack code, must be removed later */

/*
 * Title and copyright messages.  May be overridden by bootarg_parse().
 */
char *title[MAXTITLE] = {
	SYS_TITLE
};
uint_t ntitle = 1;
boolean_t title_changed = B_FALSE;
char *copyright[MAXCOPYRIGHT] = {
	COPYRIGHT_COMMON
	COPYRIGHT_FAMILY
	COPYRIGHT_PLATFORM
	"All Rights Reserved"
	"\nU.S. Pat. No. 5,349,642"
};
uint_t ncopyright = 1;
boolean_t copyright_changed = B_FALSE;

/*
 * Special features (if applicable).  May be overridden by bootarg_parse().
 */
boolean_t disable_pge = B_FALSE;
boolean_t ignore_machine_check = B_FALSE;
boolean_t disable_cache = B_FALSE;
boolean_t disable_copy_mtrrs = B_FALSE;
#ifdef CCNUMA
boolean_t using_pae = B_TRUE;
#else
boolean_t using_pae = B_FALSE;
#endif

/*
 * Until the debugger supports replicated break points, we won't be
 * able to turn on replicated text in DEBUG kernels.
 */
#if	defined(DEBUG) && defined(CCNUMA)
boolean_t replicated_text = B_FALSE;
#else /* DEBUG && CCNUMA */
boolean_t replicated_text = B_TRUE;
#endif /* DEBUG && CCNUMA */

extern void v86bios_init();

extern k_pl_t ipl;
ullong_t save_kl1pte0;
paddr_t bios_offset;
extern pte64_t *pkl1pt64_cg[MAXNUMCG];
extern void alloc_l2pt(vaddr_t);
extern paddr_t pmaplimit;	/* memory below is mapped P==V by the boot */
extern vaddr_t calloc_base;	/* where calloc mappings begin */
vaddr_t kvphysbase;		/* kernel virtual base for phystokv */
extern void mem_parse_topology(void);

/*
 * L1PTEs for the pstart page and the bios.
 */
ullong_t pstart_kl1pte;		/* pstart l1pte */
ullong_t save_kl1pte0;		/* bios l1pte */

cgnum_t	ctog[MAXNUMCPU];
int	gToLeader[MAXNUMCG];

/*
 *  Support for Machine Check Architecture (MCA)
 */

int mca_banks;				/* number of hardware reporting banks */
boolean_t mcg_ctl_present = B_FALSE;	/* MCG_CTL register present */

/*
 * Intel Workaround for the "Invalid Operand
 * with Locked Compare Exchange 8Byte (CMPXCHG8B) Instruction"
 *
 * Our workaround is to make the idt unwriteble.
 * The instruction causes a fage fault (upageflt). cr2 is pointing to
 * the INVOPFLT entry upon that fault.
 *
 */
vaddr_t p5_idt_pagebase = KVPAGE0 - (3 * MMU_PAGESIZE);	/* used by intr.s */
#define p5_std_idt_pagebase (p5_idt_pagebase + (2 * MMU_PAGESIZE))
STATIC	uint_t *p5_idt_ptep;	 /* pointer to first pte of private idt */
struct desctab p5_std_idt_desc;	 /* used by restore_std_idt() */

/*
 * Table for constructing IDT that is common across all platforms.
 * Note the entries for 15 and 19:31.  These serve solely as debugging aid
 * to catch stray interrupts before the PSM has had time to initialize the
 * interrupt controller hardware.  The right fix is to correct the problem
 * whereby the kernel debugger turns interrupts on prematurely during
 * initialization; once this is done we could reasonably replace the
 * these entries with the default t_res.
 */
extern void spurious15(void);
extern void spurious19(void);
extern void spurious20(void);
extern void spurious21(void);
extern void spurious22(void);
extern void spurious23(void);
extern void spurious24(void);
extern void spurious25(void);
extern void spurious26(void);
extern void spurious27(void);
extern void spurious28(void);
extern void spurious29(void);
extern void spurious30(void);
extern void spurious31(void);

struct idt_init idt_init[] = {
	{	DIVERR,		GATE_386TRP,	t_diverr,	GATE_KACC },
#ifdef USE_GDB
	{       SGLSTP,         GATE_386INT,_catchException1,   GATE_KACC },
#else
	{	SGLSTP,		GATE_386TRP,	t_dbg,		GATE_KACC },
#endif
	{	NMIFLT,		GATE_386INT,	t_nmi,		GATE_KACC },
#ifdef USE_GDB
	{       BPTFLT,         GATE_386INT,_catchException3,   GATE_KACC},
#else
	{	BPTFLT,		GATE_386TRP,	t_int3,		GATE_UACC },
#endif
	{	INTOFLT,	GATE_386TRP,	t_into,		GATE_UACC },
	{	BOUNDFLT,	GATE_386TRP,	t_check,	GATE_UACC },
	{	INVOPFLT,	GATE_386TRP,	t_und,		GATE_KACC },
	{	NOEXTFLT,	GATE_386TRP,	t_dna,		GATE_KACC },
	{	EXTOVRFLT,	GATE_386INT,	t_extovr,	GATE_KACC },
	{	INVTSSFLT,	GATE_386INT,	t_badtss,	GATE_KACC },
#ifdef USE_GDB
	{       SEGNPFLT,       GATE_386INT,_catchException11,  GATE_KACC },
#else
	{	SEGNPFLT,	GATE_386TRP,	t_notpres,	GATE_KACC },
#endif
	{	STKFLT,		GATE_386TRP,	t_stkflt,	GATE_KACC },
#ifdef USE_GDB
	{       GPFLT,          GATE_386INT,_catchException13,  GATE_KACC },
	{       PGFLT,          GATE_386INT,_catchException14,  GATE_KACC },
#else
	{	GPFLT,		GATE_386TRP,	t_gpflt,	GATE_KACC },
	{	PGFLT,		GATE_386INT,	t_pgflt,	GATE_KACC },
	{	15,		GATE_386INT,	spurious15,	GATE_KACC },
#endif
	{	EXTERRFLT,	GATE_386TRP,	t_coperr,	GATE_UACC },
	{	ALIGNFLT,	GATE_386TRP,	t_alignflt,	GATE_KACC },
	{	MCEFLT,		GATE_386INT,	t_mceflt,	GATE_KACC },
	{	19,		GATE_386INT,	spurious19,	GATE_KACC },
	{	20,		GATE_386INT,	spurious20,	GATE_KACC },
	{	21,		GATE_386INT,	spurious21,	GATE_KACC },
	{	22,		GATE_386INT,	spurious22,	GATE_KACC },
	{	23,		GATE_386INT,	spurious23,	GATE_KACC },
	{	24,		GATE_386INT,	spurious24,	GATE_KACC },
	{	25,		GATE_386INT,	spurious25,	GATE_KACC },
	{	26,		GATE_386INT,	spurious26,	GATE_KACC },
	{	27,		GATE_386INT,	spurious27,	GATE_KACC },
	{	28,		GATE_386INT,	spurious28,	GATE_KACC },
	{	29,		GATE_386INT,	spurious29,	GATE_KACC },
	{	30,		GATE_386INT,	spurious30,	GATE_KACC },
	{	31,		GATE_386INT,	spurious31,	GATE_KACC },
	{  32,	GATE_386INT,	devint20,	GATE_KACC },
	{  33,	GATE_386INT,	devint21,	GATE_KACC },
	{  34,	GATE_386INT,	devint22,	GATE_KACC },
	{  35,	GATE_386INT,	devint23,	GATE_KACC },
	{  36,	GATE_386INT,	devint24,	GATE_KACC },
	{  37,	GATE_386INT,	devint25,	GATE_KACC },
	{  38,	GATE_386INT,	devint26,	GATE_KACC },
	{  39,	GATE_386INT,	devint27,	GATE_KACC },
	{  40,	GATE_386INT,	devint28,	GATE_KACC },
	{  41,	GATE_386INT,	devint29,	GATE_KACC },
	{  42,	GATE_386INT,	devint2a,	GATE_KACC },
	{  43,	GATE_386INT,	devint2b,	GATE_KACC },
	{  44,	GATE_386INT,	devint2c,	GATE_KACC },
	{  45,	GATE_386INT,	devint2d,	GATE_KACC },
	{  46,	GATE_386INT,	devint2e,	GATE_KACC },
	{  47,	GATE_386INT,	devint2f,	GATE_KACC },
	{  48,	GATE_386INT,	devint30,	GATE_KACC },
	{  49,	GATE_386INT,	devint31,	GATE_KACC },
	{  50,	GATE_386INT,	devint32,	GATE_KACC },
	{  51,	GATE_386INT,	devint33,	GATE_KACC },
	{  52,	GATE_386INT,	devint34,	GATE_KACC },
	{  53,	GATE_386INT,	devint35,	GATE_KACC },
	{  54,	GATE_386INT,	devint36,	GATE_KACC },
	{  55,	GATE_386INT,	devint37,	GATE_KACC },
	{  56,	GATE_386INT,	devint38,	GATE_KACC },
	{  57,	GATE_386INT,	devint39,	GATE_KACC },
	{  58,	GATE_386INT,	devint3a,	GATE_KACC },
	{  59,	GATE_386INT,	devint3b,	GATE_KACC },
	{  60,	GATE_386INT,	devint3c,	GATE_KACC },
	{  61,	GATE_386INT,	devint3d,	GATE_KACC },
	{  62,	GATE_386INT,	devint3e,	GATE_KACC },
	{  63,	GATE_386INT,	devint3f,	GATE_KACC },
	{  64,	GATE_386INT,	devint40,	GATE_KACC },
	{  65,	GATE_386INT,	devint41,	GATE_KACC },
	{  66,	GATE_386INT,	devint42,	GATE_KACC },
	{  67,	GATE_386INT,	devint43,	GATE_KACC },
	{  68,	GATE_386INT,	devint44,	GATE_KACC },
	{  69,	GATE_386INT,	devint45,	GATE_KACC },
	{  70,	GATE_386INT,	devint46,	GATE_KACC },
	{  71,	GATE_386INT,	devint47,	GATE_KACC },
	{  72,	GATE_386INT,	devint48,	GATE_KACC },
	{  73,	GATE_386INT,	devint49,	GATE_KACC },
	{  74,	GATE_386INT,	devint4a,	GATE_KACC },
	{  75,	GATE_386INT,	devint4b,	GATE_KACC },
	{  76,	GATE_386INT,	devint4c,	GATE_KACC },
	{  77,	GATE_386INT,	devint4d,	GATE_KACC },
	{  78,	GATE_386INT,	devint4e,	GATE_KACC },
	{  79,	GATE_386INT,	devint4f,	GATE_KACC },
	{  80,	GATE_386INT,	devint50,	GATE_KACC },
	{  81,	GATE_386INT,	devint51,	GATE_KACC },
	{  82,	GATE_386INT,	devint52,	GATE_KACC },
	{  83,	GATE_386INT,	devint53,	GATE_KACC },
	{  84,	GATE_386INT,	devint54,	GATE_KACC },
	{  85,	GATE_386INT,	devint55,	GATE_KACC },
	{  86,	GATE_386INT,	devint56,	GATE_KACC },
	{  87,	GATE_386INT,	devint57,	GATE_KACC },
	{  88,	GATE_386INT,	devint58,	GATE_KACC },
	{  89,	GATE_386INT,	devint59,	GATE_KACC },
	{  90,	GATE_386INT,	devint5a,	GATE_KACC },
	{  91,	GATE_386INT,	devint5b,	GATE_KACC },
	{  92,	GATE_386INT,	devint5c,	GATE_KACC },
	{  93,	GATE_386INT,	devint5d,	GATE_KACC },
	{  94,	GATE_386INT,	devint5e,	GATE_KACC },
	{  95,	GATE_386INT,	devint5f,	GATE_KACC },
	{  96,	GATE_386INT,	devint60,	GATE_KACC },
	{  97,	GATE_386INT,	devint61,	GATE_KACC },
	{  98,	GATE_386INT,	devint62,	GATE_KACC },
	{  99,	GATE_386INT,	devint63,	GATE_KACC },
	{ 100,	GATE_386INT,	devint64,	GATE_KACC },
	{ 101,	GATE_386INT,	devint65,	GATE_KACC },
	{ 102,	GATE_386INT,	devint66,	GATE_KACC },
	{ 103,	GATE_386INT,	devint67,	GATE_KACC },
	{ 104,	GATE_386INT,	devint68,	GATE_KACC },
	{ 105,	GATE_386INT,	devint69,	GATE_KACC },
	{ 106,	GATE_386INT,	devint6a,	GATE_KACC },
	{ 107,	GATE_386INT,	devint6b,	GATE_KACC },
	{ 108,	GATE_386INT,	devint6c,	GATE_KACC },
	{ 109,	GATE_386INT,	devint6d,	GATE_KACC },
	{ 110,	GATE_386INT,	devint6e,	GATE_KACC },
	{ 111,	GATE_386INT,	devint6f,	GATE_KACC },
	{ 112,	GATE_386INT,	devint70,	GATE_KACC },
	{ 113,	GATE_386INT,	devint71,	GATE_KACC },
	{ 114,	GATE_386INT,	devint72,	GATE_KACC },
	{ 115,	GATE_386INT,	devint73,	GATE_KACC },
	{ 116,	GATE_386INT,	devint74,	GATE_KACC },
	{ 117,	GATE_386INT,	devint75,	GATE_KACC },
	{ 118,	GATE_386INT,	devint76,	GATE_KACC },
	{ 119,	GATE_386INT,	devint77,	GATE_KACC },
	{ 120,	GATE_386INT,	devint78,	GATE_KACC },
	{ 121,	GATE_386INT,	devint79,	GATE_KACC },
	{ 122,	GATE_386INT,	devint7a,	GATE_KACC },
	{ 123,	GATE_386INT,	devint7b,	GATE_KACC },
	{ 124,	GATE_386INT,	devint7c,	GATE_KACC },
	{ 125,	GATE_386INT,	devint7d,	GATE_KACC },
	{ 126,	GATE_386INT,	devint7e,	GATE_KACC },
	{ 127,	GATE_386INT,	devint7f,	GATE_KACC },
	{ 128,	GATE_386INT,	devint80,	GATE_KACC },
	{ 129,	GATE_386INT,	devint81,	GATE_KACC },
	{ 130,	GATE_386INT,	devint82,	GATE_KACC },
	{ 131,	GATE_386INT,	devint83,	GATE_KACC },
	{ 132,	GATE_386INT,	devint84,	GATE_KACC },
	{ 133,	GATE_386INT,	devint85,	GATE_KACC },
	{ 134,	GATE_386INT,	devint86,	GATE_KACC },
	{ 135,	GATE_386INT,	devint87,	GATE_KACC },
	{ 136,	GATE_386INT,	devint88,	GATE_KACC },
	{ 137,	GATE_386INT,	devint89,	GATE_KACC },
	{ 138,	GATE_386INT,	devint8a,	GATE_KACC },
	{ 139,	GATE_386INT,	devint8b,	GATE_KACC },
	{ 140,	GATE_386INT,	devint8c,	GATE_KACC },
	{ 141,	GATE_386INT,	devint8d,	GATE_KACC },
	{ 142,	GATE_386INT,	devint8e,	GATE_KACC },
	{ 143,	GATE_386INT,	devint8f,	GATE_KACC },
	{ 144,	GATE_386INT,	devint90,	GATE_KACC },
	{ 145,	GATE_386INT,	devint91,	GATE_KACC },
	{ 146,	GATE_386INT,	devint92,	GATE_KACC },
	{ 147,	GATE_386INT,	devint93,	GATE_KACC },
	{ 148,	GATE_386INT,	devint94,	GATE_KACC },
	{ 149,	GATE_386INT,	devint95,	GATE_KACC },
	{ 150,	GATE_386INT,	devint96,	GATE_KACC },
	{ 151,	GATE_386INT,	devint97,	GATE_KACC },
	{ 152,	GATE_386INT,	devint98,	GATE_KACC },
	{ 153,	GATE_386INT,	devint99,	GATE_KACC },
	{ 154,	GATE_386INT,	devint9a,	GATE_KACC },
	{ 155,	GATE_386INT,	devint9b,	GATE_KACC },
	{ 156,	GATE_386INT,	devint9c,	GATE_KACC },
	{ 157,	GATE_386INT,	devint9d,	GATE_KACC },
	{ 158,	GATE_386INT,	devint9e,	GATE_KACC },
	{ 159,	GATE_386INT,	devint9f,	GATE_KACC },
	{ 160,	GATE_386INT,	devinta0,	GATE_KACC },
	{ 161,	GATE_386INT,	devinta1,	GATE_KACC },
	{ 162,	GATE_386INT,	devinta2,	GATE_KACC },
	{ 163,	GATE_386INT,	devinta3,	GATE_KACC },
	{ 164,	GATE_386INT,	devinta4,	GATE_KACC },
	{ 165,	GATE_386INT,	devinta5,	GATE_KACC },
	{ 166,	GATE_386INT,	devinta6,	GATE_KACC },
	{ 167,	GATE_386INT,	devinta7,	GATE_KACC },
	{ 168,	GATE_386INT,	devinta8,	GATE_KACC },
	{ 169,	GATE_386INT,	devinta9,	GATE_KACC },
	{ 170,	GATE_386INT,	devintaa,	GATE_KACC },
	{ 171,	GATE_386INT,	devintab,	GATE_KACC },
	{ 172,	GATE_386INT,	devintac,	GATE_KACC },
	{ 173,	GATE_386INT,	devintad,	GATE_KACC },
	{ 174,	GATE_386INT,	devintae,	GATE_KACC },
	{ 175,	GATE_386INT,	devintaf,	GATE_KACC },
	{ 176,	GATE_386INT,	devintb0,	GATE_KACC },
	{ 177,	GATE_386INT,	devintb1,	GATE_KACC },
	{ 178,	GATE_386INT,	devintb2,	GATE_KACC },
	{ 179,	GATE_386INT,	devintb3,	GATE_KACC },
	{ 180,	GATE_386INT,	devintb4,	GATE_KACC },
	{ 181,	GATE_386INT,	devintb5,	GATE_KACC },
	{ 182,	GATE_386INT,	devintb6,	GATE_KACC },
	{ 183,	GATE_386INT,	devintb7,	GATE_KACC },
	{ 184,	GATE_386INT,	devintb8,	GATE_KACC },
	{ 185,	GATE_386INT,	devintb9,	GATE_KACC },
	{ 186,	GATE_386INT,	devintba,	GATE_KACC },
	{ 187,	GATE_386INT,	devintbb,	GATE_KACC },
	{ 188,	GATE_386INT,	devintbc,	GATE_KACC },
	{ 189,	GATE_386INT,	devintbd,	GATE_KACC },
	{ 190,	GATE_386INT,	devintbe,	GATE_KACC },
	{ 191,	GATE_386INT,	devintbf,	GATE_KACC },
	{ 192,	GATE_386INT,	devintc0,	GATE_KACC },
	{ 193,	GATE_386INT,	devintc1,	GATE_KACC },
	{ 194,	GATE_386INT,	devintc2,	GATE_KACC },
	{ 195,	GATE_386INT,	devintc3,	GATE_KACC },
	{ 196,	GATE_386INT,	devintc4,	GATE_KACC },
	{ 197,	GATE_386INT,	devintc5,	GATE_KACC },
	{ 198,	GATE_386INT,	devintc6,	GATE_KACC },
	{ 199,	GATE_386INT,	devintc7,	GATE_KACC },
	{ 200,	GATE_386INT,	devintc8,	GATE_KACC },
	{ 201,	GATE_386INT,	devintc9,	GATE_KACC },
	{ 202,	GATE_386INT,	devintca,	GATE_KACC },
	{ 203,	GATE_386INT,	devintcb,	GATE_KACC },
	{ 204,	GATE_386INT,	devintcc,	GATE_KACC },
	{ 205,	GATE_386INT,	devintcd,	GATE_KACC },
	{ 206,	GATE_386INT,	devintce,	GATE_KACC },
	{ 207,	GATE_386INT,	devintcf,	GATE_KACC },
	{ 208,	GATE_386INT,	devintd0,	GATE_KACC },
	{ 209,	GATE_386INT,	devintd1,	GATE_KACC },
	{ 210,	GATE_386INT,	devintd2,	GATE_KACC },
	{ 211,	GATE_386INT,	devintd3,	GATE_KACC },
	{ 212,	GATE_386INT,	devintd4,	GATE_KACC },
	{ 213,	GATE_386INT,	devintd5,	GATE_KACC },
	{ 214,	GATE_386INT,	devintd6,	GATE_KACC },
	{ 215,	GATE_386INT,	devintd7,	GATE_KACC },
	{ 216,	GATE_386INT,	devintd8,	GATE_KACC },
	{ 217,	GATE_386INT,	devintd9,	GATE_KACC },
	{ 218,	GATE_386INT,	devintda,	GATE_KACC },
	{ 219,	GATE_386INT,	devintdb,	GATE_KACC },
	{ 220,	GATE_386INT,	devintdc,	GATE_KACC },
	{ 221,	GATE_386INT,	devintdd,	GATE_KACC },
	{ 222,	GATE_386INT,	devintde,	GATE_KACC },
	{ 223,	GATE_386INT,	devintdf,	GATE_KACC },
	{ 224,	GATE_386INT,	devinte0,	GATE_KACC },
	{ 225,	GATE_386INT,	devinte1,	GATE_KACC },
	{ 226,	GATE_386INT,	devinte2,	GATE_KACC },
	{ 227,	GATE_386INT,	devinte3,	GATE_KACC },
	{ 228,	GATE_386INT,	devinte4,	GATE_KACC },
	{ 229,	GATE_386INT,	devinte5,	GATE_KACC },
	{ 230,	GATE_386INT,	devinte6,	GATE_KACC },
	{ 231,	GATE_386INT,	devinte7,	GATE_KACC },
	{ 232,	GATE_386INT,	devinte8,	GATE_KACC },
	{ 233,	GATE_386INT,	devinte9,	GATE_KACC },
	{ 234,	GATE_386INT,	devintea,	GATE_KACC },
	{ 235,	GATE_386INT,	devinteb,	GATE_KACC },
	{ 236,	GATE_386INT,	devintec,	GATE_KACC },
	{ 237,	GATE_386INT,	devinted,	GATE_KACC },
	{ 238,	GATE_386INT,	devintee,	GATE_KACC },
	{ 239,	GATE_386INT,	devintef,	GATE_KACC },
	{ 240,	GATE_386INT,	devintf0,	GATE_KACC },
	{ 241,	GATE_386INT,	devintf1,	GATE_KACC },
	{ 242,	GATE_386INT,	devintf2,	GATE_KACC },
	{ 243,	GATE_386INT,	devintf3,	GATE_KACC },
	{ 244,	GATE_386INT,	devintf4,	GATE_KACC },
	{ 245,	GATE_386INT,	devintf5,	GATE_KACC },
	{ 246,	GATE_386INT,	devintf6,	GATE_KACC },
	{ 247,	GATE_386INT,	devintf7,	GATE_KACC },
	{ 248,	GATE_386INT,	devintf8,	GATE_KACC },
	{ 249,	GATE_386INT,	devintf9,	GATE_KACC },
	{ 250,	GATE_386INT,	devintfa,	GATE_KACC },
	{ 251,	GATE_386INT,	devintfb,	GATE_KACC },
	{ 252,	GATE_386INT,	devintfc,	GATE_KACC },
	{ 253,	GATE_386INT,	devintfd,	GATE_KACC },
	{ 254,	GATE_386INT,	devintfe,	GATE_KACC },
	{ 255,	GATE_386INT,	devintff,	GATE_KACC },
	{ 0 },
};

struct gate_desc fpuon_noextflt, fpuoff_noextflt;

extern struct desctab_info myglobal_dt_info[NDESCTAB];
extern struct segment_desc *myglobal_ldt;
extern struct desctab mystd_idt_desc;

extern void init_console(void);
extern void inituname(void);
extern void printuname(const char *);
extern void kdb_init(void);
extern void clock_init(void);
extern void kvm_init(void);
extern void strinit(void);
extern void ddi_drvinit(void);
extern void ddi_init(void);
extern void mod_obj_kern_init(void);
extern void physkv_init();
extern void nmi_init(void);

extern void disponline(engine_t *);
extern void dispoffline(engine_t *);
#ifndef UNIPROC
extern void empty_local_runqueue(void);
#endif

/*
 * Initialize global defines of PL's for usage by DDI/DKI drivers.
 */
pl_t pl0 = PL0;
pl_t pl1 = PL1;
pl_t pl2 = PL2;
pl_t pl3 = PL3;
pl_t pl4 = PL4;
pl_t pl5 = PL5;
pl_t pl6 = PL6;
pl_t pl7 = PL7;

pl_t plbase = PLBASE;
pl_t pltimeout = PLTIMEOUT;
pl_t pldisk = PLDISK;
pl_t plstr = PLSTR;
pl_t plhi = PLHI;
pl_t invpl = INVPL;

/*
 * Initialize global priority values for use by DDI/DKI drivers.
 */
int pridisk = PRINOD;
int prinet = 27;
int pritty = 25;
int pritape = 25;
int prihi = PRIMEM;
int primed = 24;
int prilo = 5;

/*
 * Default CPU binding.  Some platforms have a binding requirement
 * on their main I/O bus.
 */
int default_bindcpu = -1;

extern void (*io_init[])(void);
extern void (*io_start[])(void);
extern void (*io_halt[])(void);
extern int bindcpu_init[];
extern int bindcpu_start[];
extern int bindcpu_halt[];

extern int intnull(void);
 
/*
 * crash_kl1pt must be declared in in a file that is loaded V=P
 * so that it's offset will equal it's symbol table address 
 * for the use of crash(1).
 *
 * This, and the other V=P variables, vmemptr and myengppriv, can
 * only be accessed before unmap_page0() has been called.
 *
 *	vmemptr - page-aligned virtual address of next unused
 *		       virtual memory (to initialize calloc()).
 *
 *	myengppriv  - virtual address of this engine's struct ppriv_pages.
 *		       The pp_kl1pt and pp_pmap pages are already in use as
 *		       the current engine's level 1 and level 2 page tables.
 */ 
extern paddr_t crash_kl1pt;
paddr_t cr_kl1pt;
extern vaddr_t vmemptr;
extern struct ppriv_pages *myengppriv;
extern struct ppriv_pages_pae *myengppriv_pae;
extern struct cgpriv_pages *mycgpriv;
vaddr_t kvper_eng = 0;
vaddr_t kl2ptes = 0;
extern char cg_mask_array[];

extern hatops_t pae_hatops;
extern hatops_t hatops32;


/*
 * For start synchronization.
 */
volatile int scg_starting;
int this_cg_num;

void selfinit(int);

#define i_am_boot()	(!upyet)

/*
 * True if we are the boot engine, coming up for the first time, and
 * hence must do our pieces of system initialization.  False for the boot
 * engine if it's been offlined and then reonlined, and for all secondary
 * processors, whether coming up for the first time or after an
 * online/offline
 */

/*
 * STATIC boolean_t
 * cpuInCG(cgnum_t cgnum, uint cpu)
 *	Test to see if a engine with with a Cpu Group.
 *
 * Calling/Exit State:
 *	Must be called while pstart is still mapped.
 */
STATIC boolean_t
cpuInCG(cgnum_t cgnum, uint cpu)
{
	return (ctog[cpu] == cgnum);	
}

#if defined(DEBUG) || defined (DEBUG_TOOLS)
/*
 * void
 * print_config(void);
 *
 *    print the system's CG-related configuration based on the
 *    results of cg_config() above.
 *
 * Calling/Exit State:
 *
 *	None.
 */
void
print_config(void)
{
	cgnum_t cgnum;
	int j;

	printf("No. of configured Processor Modules = %d \n\n",
		global_cginfo.cg_nconf);

	for (cgnum = 0; cgnum < global_cginfo.cg_nconf; cgnum++) {
		printf("CG #   : %d\n", cgnum);
		printf("no. of CPUs on CG : %d\n", cg_array[cgnum].cg_cpunum);
		printf("CPU IDs: ");
		for (j = 0; j < cg_array[cgnum].cg_cpunum; j++)
			printf("%d ", cg_array[cgnum].cg_cpuid[j]);
		printf("\n");
		printf("CG leader : %d\n\n\n", gToLeader[cgnum]);
	}
}

/*
 * void
 * print_cgarray(void)
 *
 *    print the contents of the cg_array[] data structure
 *
 * Calling/Exit State:
 * 
 *	None.
 */
void
print_cgarray(void)
{
	int i;
	int j;

	printf("\n");
	for (i = 0; i < MAXNUMCG; i++) {
		printf("\n[%d].st = %d  ", i, cg_array[i].cg_status);
		printf("[%d].b = %d   ", i, cg_array[i].cg_bind);
		printf("[%d].n = %d   ", i, cg_array[i].cg_cpunum);
		printf("[%d].py = %d\n", i, cg_array[i].cg_kl1pt_phys);
		for (j = 0; j < MAXNUMCPU; j++)
			printf("\tc[%d] = %d   ", j, cg_array[i].cg_cpuid[j]);
	}
}
#endif /* DEBUG || DEBUG_TOOLS */

/*
 * STATIC void
 * sysinit_free_bootmem()
 *	Free up memory occupied by the boot loader and by the
 *	copy of the kernel text loaded by the boot loader.
 *
 * Callint/Exit State:
 *	Called from CG0.
 */
STATIC void
sysinit_free_bootmem()
{
	paddr64_t base, end;
	struct pmem_extent *memlistp;

	memlistp = memUsed;
	while (memlistp != NULL) {
		switch(memlistp->pm_use) {
		case PMEM_KDEBUG:
		case PMEM_BOOT:
		case PMEM_KTEXT:
#ifndef CCNUMA
		case PMEM_KDATA:
#endif /* CCNUMA */
			/*
			 * Remove this memory from the used list.
			 */
			base = memlistp->pm_base;
			end = base + memlistp->pm_extent;
			pmem_list_subtract_range(&memUsed, base, end);

			/*
			 * Now, return the memory to the free list(s).
			 */
			declare_mem_free(base, end, BOOTCG);

			/*
			 * The list has changed. Therefore, we
			 * start over again.
			 */
			memlistp = memUsed;
			break;

		default:
			memlistp = memlistp->pm_next;
			break;
		}
	}
}

/*
 * void
 * sysinit(void)
 *
 *	Perform machine specific initialization.
 *
 * Calling/Exit State:
 *
 *	Called with virtual addressing mode active and the kernel
 *	virtual address space setup.  BSS has been zeroed.
 *
 *	Returns in a state ready to call main().
 */
void
sysinit(void)
{
	struct ppriv_pages *ppriv;
	struct ppriv_pages_pae *ppriv_pae;
	struct segment_desc *sd;
	struct gate_desc *gd;
	int procid;
	ulong_t cr0;
	int i, j;
	cgnum_t xcg;
	ulong_t len;
	timestruc_t tv;
	int	psm_ok;
#ifdef DEBUG
	boolean_t intcpu_default = B_FALSE;
	boolean_t plbase_after_init;
#endif

	/*
	 * Save the kernel l1 ptes for both the pstart page and
	 * the bios. Normally, these are both the same. However,
	 * they may be different on machines
	 */
#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		pte64_t *ptep;

		l.hatops = &pae_hatops;
		/*
		 * Initialization of kl1ptp is ordinarily done by selfinit, but
		 * kl1ptp must be initialized prior to entering the debugger.
		 */
		for (i = 0; i < PDPTSZ; i++) {
			l.kl1ptp64[i] = &myengppriv_pae->pp_kl1pt[i][0];
			l.kpd64[i] = (pte64_t *)(KVENG_L1PT + ((i) * MMU_PAGESIZE));
		}
		cr_kl1pt = crash_kl1pt = kvtophys64((addr_t)&myengppriv_pae->pp_kl1pt[3][0]);
		ptep = vtol1ptep64((vaddr_t) reset_code);
		pstart_kl1pte = ptep->pg_pte;
		ptep = vtol1ptep64(0);
		save_kl1pte0 = ptep->pg_pte;
	} else
#endif /* PAE_MODE */
	{
		l.hatops = &hatops32;
		l.kpd0 = (pte_t *)(KVENG_L1PT);
		cr_kl1pt = crash_kl1pt = kvtophys((addr_t)&myengppriv->pp_kl1pt[0][0]);

		/*
		 * Initialization of kl1ptp is ordinarily done by selfinit, but
		 * kl1ptp must be initialized prior to entering the debugger.
		 */
		l.kl1ptp = &myengppriv->pp_kl1pt[0][0];
		pstart_kl1pte = vtol1ptep((vaddr_t) reset_code)->pg_pte;
		save_kl1pte0 = vtol1ptep(0)->pg_pte;
	}

	/*
	 * Initialize enough stuff so we can get faults.
	 */
	tmp_init_desc_tables();

	/*
	 * Initialize l.userp to the per-engine u area address.
	 */
	l.userp = &ueng;

	/*
	 * Initialize VM86 flag (for v86bios)
	 */
	l.special_lwp &= ~SPECF_VM86; 

	/*
	 * Set address to use for PSM reporting of stray interrupts.
	 * (os_intr_dist_stray is a global variable that points to a 
	 * per-cpu structure located in plocal).
	 */
	os_intr_dist_stray = (ms_intr_dist_t *) l.idt_stray;

	/*
	 * no kernel preemptions pending
	 */
	l.prmpt_state.s_prmpt_state.s_noprmpt = 1;

	/*
	 * Initialize the lock statistics data structures.
	 */
	lkstat_init();

	/* to init calloc() */
	hat_static_init(vmemptr);

#ifdef CCNUMA
	/*
	 * The ccNUMA kernel needs to calibrate the page table
	 * allocation activity in order to synchronize it later
	 * with other CGs (not yet onlined).
	 */
	sysinit_sync_init();
	sysinit_sync(SYNC_EVENT_MAP_BOOTCG_CALLOC, SYNC_SEQUENTIAL);
#endif /* CCNUMA */

#ifdef DEBUG /* Can't ASSERT yet. */
	plbase_after_init = (getpl() == PLBASE);
#endif

	/*
	 * Retrieve any information passed from the bootstrap.
	 * calloc is available at this time, but cmn_err is not.
	 *
	 * This must come before console I/O so that boot parameters
	 * can be used to change the console device.
	 */
	bootarg_parse();

	/*
	 * Initialize console I/O.  Note that this call MAY have been made
	 * already if os_printf was called.  However, init_console is prepared
	 * to be called unnecessarily.
	 */
	cmn_err_init();
	init_console();

	/*
	 * Initialize the O/S name/version, so it can be used in the
	 * message below.
	 */
	inituname();

	/*
	 * Print system title and copyright messages.
	 */
	cmn_err(CE_CONT, "^\n");	/* In case not at beginning of line */
	for (i = 0; i < ntitle; i++)
		printuname(title[i]);	/* parameterize and print line */
	cmn_err(CE_CONT, "\n");
	for (i = 0; i < ncopyright; i++)
		cmn_err(CE_CONT, "%s\n", copyright[i]);
	cmn_err(CE_CONT, "\n");

	/*
	 * Hack code to program memory above 4G on quad P6 with an Orion
	 * chipset.
	 */
	p6dosplit();

	/*
	 * Initialize v86bios
	 */
	v86bios_init();
	
	/*
	 * Load a PSM. 
	 */
	psm_ok = initpsm();

	/*
	 * Panic if unable to load a psm.
	 */
	if (!psm_ok) {
		/*
		 *+ No PSM routine was found that could execute on this
		 *+ platform. Chose another PSM or rebuild your kernel
		 *+ with a PSM that supports this platform.
		 */
		cmn_err(CE_PANIC, "Unable to load a PSM for this platform.\n");
	}

	/*
	 * Now, parse the memory topology information.
	 */
	mem_parse_topology();

	/*
	 * Handle deferred ASSERTs.
	 */
	ASSERT(plbase_after_init);
	ASSERT(getpl() == PLBASE);

	/*
	 * Configure the HW and initialize the interrupt table.
	 * Configure is called with the temporary page tables in use.
	 *
	 * Configure fills out:
	 *	engine[] array		; one per processor
	 *	Nengine			; # of processors
	 *	topmem			; top of physical memory
	 */

	configure();

	procid = BOOTENG;
#ifdef DEBUG
	print_config();
#endif

	/*
	 * Allocate the interrupt table. This replaces the old ivect, intpri,
	 * intmp, intr_upcount, intr_bindcpu.
	 */
	intr_vect = (struct intr_vect_t*) calloc((1+os_islot_max)*sizeof(struct intr_vect_t));
	for (i = 0; i <= os_islot_max; i++)
		intr_vect[i].iv_ivect = intnull;

        /*
         * Make sure os_tick_period is set to a value that the system
         * can implement.
         */

        if (os_tick_1_res.mst_sec == 0 &&
            os_tick_period.mst_nsec < os_tick_1_res.mst_nsec)
                os_tick_period.mst_nsec = os_tick_1_res.mst_nsec;
        else if (os_tick_1_max.mst_sec == 0 &&
                 os_tick_period.mst_nsec > os_tick_1_max.mst_nsec)
                os_tick_period.mst_nsec = os_tick_1_max.mst_nsec;


	/*
	 * Allocate and initialize global LDT.
	 */

	callocrnd(8);	/* LDT must be 8-byte aligned */
	myglobal_ldt = calloc(LDTSZ * 8);

	gd = (struct gate_desc *)&myglobal_ldt[seltoi(USER_SCALL)];
	BUILD_GATE_DESC(gd, KCSSEL, sys_call, GATE_386CALL, GATE_UACC, 1);
	gd = (struct gate_desc *)&myglobal_ldt[seltoi(USER_SIGCALL)];
	BUILD_GATE_DESC(gd, KCSSEL, sig_clean, GATE_386CALL, GATE_UACC, 1);
	sd = &myglobal_ldt[seltoi(USER_CS)];
	BUILD_MEM_DESC(sd, 0, mmu_btop(uvend), UTEXT_ACC1, TEXT_ACC2);
	sd = &myglobal_ldt[seltoi(USER_DS)];
	BUILD_MEM_DESC(sd, 0, mmu_btop(UVUVWIN)+1, UDATA_ACC1, DATA_ACC2);
	sd = &myglobal_ldt[seltoi(UVWINSEL)];
	BUILD_MEM_DESC(sd, UVUVWIN, 1, UDATA_ACC1, DATA_ACC2);
	myglobal_dt_info[DT_LDT].di_table = myglobal_ldt;
	myglobal_dt_info[DT_LDT].di_size = LDTSZ * 8;

	myglobal_dt_info[DT_GDT].di_table = l.global_gdt;
	myglobal_dt_info[DT_GDT].di_size = GDTSZ * 8;

	/*
	 * Allocate per-processor local data.
	 * (The one for the current processor was already allocated
	 * and passed to us.)
	 */

	callocrnd(MMU_PAGESIZE);			/* for L1PT's */
#ifdef PAE_MODE
	if (PAE_ENABLED())
		ppriv_pae = calloc((cg_array[BOOTCG].cg_cpunum - 1) *
				   SZPPRIV_PAE);
	else
#endif /* PAE_MODE */
		ppriv = calloc((cg_array[BOOTCG].cg_cpunum - 1) * SZPPRIV);

	/*
	 * Fill out remaining engine table fields not set by configure().
	 */

	for (i = 0, j = 0; i < Nengine; i++) {
		engine[i].e_flags |= E_OFFLINE;
		if (i == BOOTENG) {           /* boot engine ? */
#ifdef PAE_MODE
			if (PAE_ENABLED())
				engine[i].e_local_pae = myengppriv_pae;
			else
#endif /* PAE_MODE */
				engine[i].e_local = myengppriv;
			engine[i].e_cglocal = mycgpriv;
			engine[i].e_flags |= E_CGLEADER;
		} else {
			/*
			 * The first CPU in each CG is the leader.
			 */
			if (gToLeader[ctog[i]] == i)
				engine[i].e_flags |= E_CGLEADER;
			/*
			 * Fill in only for cpu's belonging to this CG.
			 * The others are done by the CG-leader of other
			 * CG's.
			 */
			if (cpuInCG(BOOTCG, i)) {
#ifdef PAE_MODE
				if (PAE_ENABLED())
					engine[i].e_local_pae = &ppriv_pae[j++];
				else
#endif /* PAE_MODE */
					engine[i].e_local = &ppriv[j++];
			}
		}
	}
        ASSERT(j == (cg_array[BOOTCG].cg_cpunum - 1)); /* done with all cpu's in CG */

	/*
	 * Perform basic sanity of the kernel stack.  Since the
	 * kernel stack size is a function of sizeof(struct user),
	 * there's always the possibility the user structure won't
	 * leave enough space for the kernel stack.
	 * 3000 bytes is an estimate.
	 */
	if (UAREA_OFFSET - KSTACK_RESERVE < 3000)
		/*
		 *+ The current layout of the u-block leaves insufficient
		 *+ room for the kernel stack.  This is an internal
		 *+ inconsistency which can only be fixed by a kernel
		 *+ developer.
		 */
		cmn_err(CE_PANIC, "Not enough space for the kernel stack.");

	/*
	 * Initialize the struct modctl and struct module for the
	 * static kernel.
	 */
	mod_obj_kern_init();

#ifndef NODEBUGGER
	/*
	 * Initialize the kernel debugger(s).
	 * At this point:
	 *	BSS must be mapped and zeroed.
	 *	It must be possible to call the putchar/getchar routines
	 *		for the console device.
	 *	It must be possible to field debugger traps.
	 *		(Note: k_trap() references u.u_fault_catch)
	 *
	 * Command strings from "unixsyms -i" will be executed here
	 * by the first statically-configured kernel debugger, if any.
	 */
	kdb_init();
#endif

#ifdef USE_GDB
	/* initialize gdb after streams is initialized */
	gdb_init();
#endif

	/*
	 * Determine type of CPU/FPU/FPA.
	 * We do this here instead of in selfinit(), so we can enable
	 * the cache on i486 chips much sooner.
	 */

	(void) chip_detect();

	/*
	 * Running on an i486 or greater; enable the onchip
	 * cache.  While we're at it, set other control bits.
	 * Note that we're explicitly clearing the "Alignment
	 * Mask" bit to prevent alignment checking in user
	 * programs.  Also, enable kernel-mode write-protect
	 * checking.
	 */
	INVAL_ONCHIP_CACHE();
	cr0 = READ_MSW();
	if (disable_cache)
		cr0 |= (CR0_CD | CR0_NW);
	else
		cr0 &= ~(CR0_CD | CR0_NW);
	cr0 = (cr0 & ~CR0_AM) | CR0_WP;
	WRITE_MSW(cr0);

	/*
	 * We initialize eng_tbl_mutex early, as it will be needed before
	 * engine_init is called.
	 */

	LOCK_INIT(&eng_tbl_mutex, ENG_MUT_HIER, ENG_MINIPL,
		  &eng_tbl_lkinfo, KM_SLEEP);

#ifdef CCNUMA
	/*
	 * Start all SBSPs.
	 */
	for (xcg = 1; xcg < Ncg; ++xcg) {
		scg_starting = 1;
		online_engine_sbsp(xcg);

		/*
		 * Now, wait for the engine to start.
		 */
#ifdef DEBUG_SBSP
		bpt();
#endif
		i = SYNC_TIMEOUT;
		while (scg_starting == 1) {
#ifndef DEBUG_SBSP
			if (i == 0){
				/*
				 *+ A timeout occured waiting for the
				 *+ indicated CPU group to respond during OS
				 *+ initialization. Please check for the
				 *+ integrity of the interconnect hardware.
				 */
				cmn_err(CE_PANIC,
					"sysinit: CPU group %d timed out", xcg);
			}
			ms_time_spin(SYNC_MSEC);
			--i;
#endif /* DEBUG_SBSP */
		}
	}

	/*
	 * Let the secondary CG's sync up with the page tables
	 * built by calloc()ed up to this point.
	 *
	 *	XXX: we extend the range back to calloc_base to cover the
	 *	     the implicit callocs performed in build_table.
	 */
	sysinit_sync_range(SYNC_EVENT_MAP_BOOTCG_CALLOC, calloc_base,
			   hat_static_nextvaddr());
#endif /* CCNUMA */

	sysinit_sync(SYNC_EVENT_MAP_PHYSTOKV, SYNC_SEQUENTIAL);

	/*
	 * Build phystokv
	 */
	kvphysbase = (vaddr_t) physmap0(bios_offset,
				        PHYSTOKV_PARTIAL_COMPAT_RANGE);

	/*
	 * relocate the bootstring 
	 */
	bs_reloc();
	
	sysinit_sync(SYNC_EVENT_MAP_PHYSTOKV, SYNC_PARALLEL);

	/*
	 * Now free up the copy of the OS text loaded by the boot loader.
	 */
	sysinit_free_bootmem();

	/*
	 * Now that all CGs are running, this is a convenient time
	 * to prime the reset code to call _start() using the
	 * current values of cr0 and cr4.
	 */
	prime_reset_code(_start);

 	sysinit_sync(SYNC_EVENT_SERIALIZE1_CALLOC, SYNC_SEQUENTIAL);

	/*
	 * Initialize kernel virtual memory.
	 * Disable calloc(); enable kmem_alloc().
	 */
	kvm_init();	/* initialize kernel virtual memory and kmem_alloc() */

	ddi_drvinit();

	/*
	 * Override default CPU bindings for static H/W modules.
	 */
	for (i = 0; io_init[i] != NULL; i++) {
		if (bindcpu_init[i] == -2)
			bindcpu_init[i] = default_bindcpu;
	}
	for (i = 0; io_start[i] != NULL; i++) {
		if (bindcpu_start[i] == -2)
			bindcpu_start[i] = default_bindcpu;
	}
	for (i = 0; io_halt[i] != NULL; i++) {
		if (bindcpu_halt[i] == -2)
			bindcpu_halt[i] = default_bindcpu;
	}
	for (i = 0; i < bdevcnt; i++) {
		if (bdevsw[i].d_cpu == -2)
			bdevsw[i].d_cpu = default_bindcpu;
	}
	for (i = 0; i < cdevcnt; i++) {
		if (cdevsw[i].d_cpu == -2)
			cdevsw[i].d_cpu = default_bindcpu;
	}

	/*
	 * Initialize cross-processor interrupt
	 */
	xcall_init();

	/*
	 * Initialize STREAMS
	 */
	strinit();

	/*
	 * Initialize NMI handling + register any system NMI handlers.
	 */
	nmi_init();

	/*
	 * Start clock services.  Must do the clock handling before
	 * dispatcher, as the dispatcher calls into the scheduling
	 * classes which are known to set timeouts.  ddi_init() also
	 * needs timeouts.
	 */
	clock_init();

	/*
	 * Initialize DDI/DKI routines.
	 */
	ddi_init();

        /*
         * Initialize hardware metering hooks.
         */
        lwp_callback_init();

	/*
	 * Read the time-of-day clock to be used to initialize unix clock
	 * (hrestime and time).
	 */
	if (xtodc(&tv) == 0) {
		settime(&tv);
	}

	/*
	 * Initialize the dispatcher.
	 */
	dispinit();

	/*
	 * Initialize pmd locks
	 */
	pmd_init();

	/* declare oursleves online and advertise our level 1 page table */
	mycg = 0;
#ifdef NOTYET
	cg_array[mycg].cg_kl1pt_phys = 
		kvtophys(&engine[gToLeader[BOOTCG]].e_local->pp_kl1pt[0][0]);
#endif

	/*
	 * Intel Workaround for the "Invalid Operand
	 * with Locked Compare Exchange 8Byte (CMPXCHG8B) Instruction"
	 * Erratum.
	 */
	if (l.cpu_id == CPU_P5) {
		
		/*
		 * Set static variables common to all engines,
		 * and proceed to per-engine setup for this boot engine.
		 * For other processors, the following should be skipped, but 
		 */
		BUILD_TABLE_DESC(&p5_std_idt_desc,
				 p5_std_idt_pagebase + PAGOFF(l.std_idt),
				 IDTSZ);
		p5_idt_ptep = (uint_t *)kvtol2ptep(p5_idt_pagebase);
	}

	/*
	 * System is mapped, do self-init.
	 */

	selfinit(BOOTENG);
}

#ifdef CCNUMA
/*
 * void
 * cg_map_priv(cgnum_t cgnum)
 *
 *	Allocate virtual address for engine and cglocal pages.
 *	Physical space for these pages has already been allcocated in 
 *	build_table(). Just copy the pte's from private L2 map.
 *
 * Calling/Exit State:
 *
 *	None.
 */
void
cg_map_priv(cgnum_t cgnum)
{
	vaddr_t ppriv;
	vaddr_t cgpriv;
	pte64_t *pmap = (pte64_t *)KVENG_L2PT;
	pte64_t *tp;
	int i;

	ASSERT(PAE_ENABLED());

	callocrnd(mmu_ptob(1));
	ppriv = (vaddr_t)calloc_virtual(mmu_ptob(SZPPRIV_PAGES_PAE));
	alloc_l2pt(ppriv);
	tp = kvtol2ptep64(ppriv);
	for (i = 0; i < SZPPRIV_PAGES_PAE; i++, tp++)
		*tp = pmap[pae_pgndx(KVENG_PAGES) + i];
 	engine[gToLeader[cgnum]].e_local_pae = (struct ppriv_pages_pae *)ppriv;

	cgpriv = (vaddr_t)calloc_virtual(mmu_ptob(CGL_PAGES));
	alloc_l2pt(cgpriv);
	tp = kvtol2ptep64(cgpriv);
	for (i = 0; i < CGL_PAGES; i++, tp++)
		*tp = pmap[pae_pgndx(KVCGLOCAL) + i];
	engine[gToLeader[cgnum]].e_cglocal = (struct cgpriv_pages *)cgpriv;
}

/*  
 * void
 * cg_l2map_misc(cgnum_t cgnum)
 *
 * 	Map KVMET and KVSYSDAT virtual pages that are mapped on the BOOTCG
 * 	only(for now; this may change later).
 *
 * Calling/Exit State:
 *
 *	cg_map_kl1pt() must have been called on this cg before we 
 *	get here.
 *
 */
void
cg_l2map_misc(cgnum_t cgnum)
{
	pte64_t *cg_pereng_map;
	pte64_t *my_pereng_map = (pte64_t *)KVENG_L2PT;
	int i;
	pte64_t *cg_kl1pt;

	cg_kl1pt = (pte64_t *) physmap2((paddr_t) pkl1pt64_cg[BOOTCG],
					KVTMPPG2);

	cg_pereng_map = (pte64_t *)KVTMPPG1;
	kvtol2ptep64(cg_pereng_map)->pg_pte = cg_kl1pt[pae_ptnum(KVMET)].pg_pte;
	TLBSflush1((vaddr_t) cg_pereng_map);
	for (i = 0; i < MET_PAGES; i++)
		my_pereng_map[pae_pgndx(KVMET) + i].pg_pte = 
			cg_pereng_map[pae_pgndx(KVMET) + i].pg_pte;

	for (i = 0; i < SYSDAT_PAGES; i++)
		my_pereng_map[pae_pgndx(KVSYSDAT) + i].pg_pte = 
			cg_pereng_map[pae_pgndx(KVSYSDAT) + i].pg_pte;
}

#ifdef DEBUG
vaddr_t save_v;

/*
 * void
 * cg_verify_clonept(cgnum_t cgnum, vaddr_t start, vaddr_t end
 *	Verify proper cloning of the BOOTCG's address space.
 *
 * Calling/Exit State:
 *	Called before proper panic is possible. Therefore,
 *	just asm("hlt").
 */
void
cg_verify_clonept(cgnum_t cgnum, vaddr_t start, vaddr_t end)
{
	vaddr_t v;
	pte64_t *cg_kl1pt;
	pte64_t *cg_kl2pt;
	pte64_t cg_kl2pte;
	int ptidx;

	cg_kl1pt = (pte64_t *) physmap2((paddr_t) pkl1pt64_cg[cgnum],
					KVTMPPG2);
	graffiti();

	start = mmu_ptob(mmu_btop(start));
	end = mmu_ptob(mmu_btopr(end));

	for (v = start; v < end; v += MMU_PAGESIZE) {
		save_v = v;
		ptidx = pae_ptnum(v);
		if (cg_kl1pt[ptidx].pg_pte != 0) {
			if (kvtol1ptep64(v)->pg_pte == 0) {
				asm("hlt");
			}
			cg_kl2pt = (pte64_t *)KVTMPPG1;
			kvtol2ptep64(cg_kl2pt)->pg_pte = cg_kl1pt[ptidx].pg_pte;
			TLBSflush1((vaddr_t) cg_kl2pt);
			cg_kl2pte.pg_pte = cg_kl2pt[pae_pgndx(v)].pg_pte;	
			ASSERT(kvtol2ptep64(v)->pgm.pg_pfn ==
			       cg_kl2pte.pgm.pg_pfn);
			ASSERT(kvtol2ptep64(v)->pgm.pg_pfn ==
			       kvtol2ptep64_g(v, cgnum)->pgm.pg_pfn);
		}
	}
}
#endif /* DEBUG */

/* 
 * void
 * cg_clonept(cgnum_t cgnum, vaddr_t start, vaddr_t end, boolean_t kl2ptes_only)
 *	Sync the L2 page tables from Cpu Group "cgnum" to "mycg".
 * 
 * Calling/Exit State:
 *	Called and base level (during system initialization) and
 *	returns that way. The interrupts are not affected.
 */
void
cg_clonept(cgnum_t cgnum, vaddr_t start, vaddr_t end, boolean_t kl2ptes_only)
{
	int ptidx;
	vaddr_t va;
	pte64_t *source_ptep, *target_ptep, *l1ptep, *l2ptep, *cg_kl1pt;

	ASSERT(start <= end);
	cg_kl1pt = (pte64_t *) physmap2((paddr_t) pkl1pt64_cg[cgnum], KVTMPPG2);

	start = mmu_ptob(mmu_btop(start));
	end = mmu_ptob(mmu_btopr(end));

#if defined(DEBUG) || defined(DEBUG_TOOLS)
	cmn_err(CE_NOTE,
		"!mycg=%x, cgnum=%x: cg_clonept: start=%x end=%x\n", mycg, cgnum, start, end);
#endif /* DEBUG || DEBUG_TOOLS */

	for (va = start; va < end;) {
		graffiti();
		ptidx = pae_ptnum(va);
		if (cg_kl1pt[ptidx].pg_pte != 0) {

#if defined(DEBUG) || defined(DEBUG_TOOLS)
			cmn_err(CE_NOTE,
				"!cg_clonept: ptidx=%x, va=%x pte=%Lx\n", 
				ptidx, va, cg_kl1pt[ptidx].pg_pte);
#endif /* DEBUG || DEBUG_TOOLS */

			/*
			 * Set source_ptep to address the other CGs
			 * l2 page table. Set target to the local
			 * l2 entry for the same page.
			 */
			source_ptep = kvtol2ptep64_g(va, cgnum);
			target_ptep = kvtol2ptep64(va);
			graffiti();

			/*
			 * However, source_ptep may still be pointing
			 * into unmapped space. The first problem is
			 * that our CG might not yet have a page of
			 * KL2PTEPTEPs for the other CG.
			 */
			l1ptep = kvtol1ptep64(source_ptep);
			if (l1ptep->pg_pte == 0) {
				graffiti();
				alloc_l2pt((vaddr_t)source_ptep);
			}
			graffiti();

			/*
			 * The second problem is that the other CG's
			 * L2 page table might not yet be mapped into
			 * our KL2PTES.
			 */
			l2ptep = kvtol2ptep64(source_ptep);
			if (l2ptep->pg_pte == 0) {
				l2ptep->pg_pte = cg_kl1pt[ptidx].pg_pte;
				graffiti();
			} else {
#ifdef NOTYET
				if (l2ptep->pg_pte != cg_kl1pt[ptidx].pg_pte) {
					graffiti();
					asm("hlt");
				}
				ASSERT(l2ptep->pg_pte ==
				       cg_kl1pt[ptidx].pg_pte);
#else
				if (l2ptep->pte32.pg_low != cg_kl1pt[ptidx].pte32.pg_low ||
				    l2ptep->pte32.pg_high != cg_kl1pt[ptidx].pte32.pg_high) {
					graffiti();
					asm("hlt");
				}
				ASSERT(l2ptep->pte32.pg_low ==
				       cg_kl1pt[ptidx].pte32.pg_low);
				ASSERT(l2ptep->pte32.pg_high ==
				       cg_kl1pt[ptidx].pte32.pg_high);
#endif /* NOTYET */

			}
			graffiti();

			if (kl2ptes_only)
				goto next_pt;
			graffiti();

			/*
			 * The next problem is that we might not
			 * have an L2 page table mapping the same
			 * range on our CG. If not, allocate one.
			 */
			l1ptep = kvtol1ptep64(va);
			if (l1ptep->pg_pte == 0)  {
				graffiti();
				alloc_l2pt(va);
			}
			graffiti();

			do {
				if (source_ptep->pg_pte != 0) {
					graffiti();
					target_ptep->pg_pte =
						source_ptep->pg_pte;
				}
				graffiti();
				++source_ptep;
				++target_ptep;
				va += MMU_PAGESIZE;
			} while ((((vaddr_t) source_ptep & MMU_PAGEOFFSET) != 0)
				 && va < end);
			continue;
		} /* end if */
next_pt:
		graffiti();
		++ptidx;
		va = (va + PAE_VPTSIZE) & PAE_VPTMASK;
	}

	/*
	 * Some PTEs may have been over-written.
	 */
	TLBSflushtlb();
#ifdef DEBUG
	cg_verify_clonept(cgnum, start, end);
#endif /* DEBUG */
}

/*
 * void
 * sync_post_action(boolean_t kl2ptes_only)
 *	Sync the L2 page tables of the CGs following a phase of
 *	local L2 allocation.
 *
 *	kl2ptes_only will be set to true all we need to do is
 *	map foreign L2 page tables in our KL2PTEs.
 * 
 * Calling/Exit State:
 *	Called and base level (during system initialization) and
 *	returns that way. The interrupts are not affected.
 */
void
sync_post_action(boolean_t kl2ptes_only)
{
	cgnum_t cgnum;

	graffiti();
	if (!kl2ptes_only) {
		/*
		 * First phase of page table exchange within a
		 * sysinit_sync point. This will clone the
		 * L2 page tables allocated on other CGs
		 * to ours.
		 */
		for (cgnum = 0; cgnum < Ncg; cgnum++) {
			if (cgnum != mycg &&
			    sync_control.ss_begin_addr[cgnum] !=
					sync_control.ss_end_addr[cgnum]) {
				cg_clonept(cgnum,
					sync_control.ss_begin_addr[cgnum],
					sync_control.ss_end_addr[cgnum],
					B_FALSE);
			}
		}
	} else if (sync_control.ss_begin_addr[mycg] ==
	       sync_control.ss_end_addr[mycg]) {
		/*
		 * We didn't allocate any page tables. Thus,
		 * no foreign L2s were created. So, we have
		 * nothing to map into our KL2PTEs.
		 */
	} else {
		/*
		 * Only cloning kl2ptes here after (1) our CG allocated
		 * some page tables, and (2) the other CG cloned L2s in
		 * response. This completes the exchange of L2s by
		 * mapping the other CGs newly allocated L2s into our
		 * KL2PTEs.
		 */
		for (cgnum = 0; cgnum < Ncg; cgnum++) {
			if (cgnum != mycg) {
				cg_clonept(cgnum,
					sync_control.ss_begin_addr[mycg],
					sync_control.ss_end_addr[mycg],
					B_TRUE);
			}
		}
	}
	graffiti();
}

#ifdef DEBUG_SBSP
bpt()
{
}
#endif /* DEBUG_SBSP */

/*
 * void
 * sync_wait_barrier(sync_event_t event, sync_stage_t stage)
 *	Timed wait for an initialization barrier.
 *
 * Calling/Exit State:
 *	Called and base level (during system initialization) and
 *	returns that way. The interrupts are not affected.
 *
 * Family Dependency:
 *	This code takes advantage of ATOMIC access to integers
 *	(permitted in the generic kernel), and of STORE ORDERING
 *	of mainstore memory (an i386 family dependency).
 */
void
sync_wait_barrier(sync_event_t event, sync_barrier_t barrier)
{
	int i = SYNC_TIMEOUT;
	int stage;
	cgnum_t cgnum, num_complete, incomplete_cg, required_cgs;

	/*
	 * Barriers are processed in sequence.
	 */
#ifdef DEBUG
	if (barrier == SYNC_PRE_CALLOC) {
		ASSERT(sync_control.ss_stage[mycg] ==
			SYNC_MAKE_STAGE(event - 1, SYNC_DONE_FIXUP));
	} else {
		ASSERT(sync_control.ss_stage[mycg] ==
			SYNC_MAKE_STAGE(event, barrier - 1));
	}
#endif /* DEBUG */

	/*
	 * Record that we got to the barrier.
	 */
#ifdef DEBUG_SBSP
	if (mycg == BOOTCG) {
		bpt();
	}
#endif /* DEBUG_SBSP */
	stage = SYNC_MAKE_STAGE(event, barrier);
	sync_control.ss_stage[mycg] = stage;

	/*
	 * Barrier SYNC_PRE_CALLOC is special, it serializes
	 * all CGs through the barrier. We implement by spinning
	 * on all previous CGs achieving the following stage.
	 */
	if (barrier == SYNC_PRE_CALLOC) {
		required_cgs = mycg;
		++stage;
	} else {
		required_cgs = Ncg;
	}

	/*
	 * Now, wait for everybody else to get there.
	 */
	for (;;) {
		num_complete = 0;
		incomplete_cg = CG_NONE;
		for (cgnum = 0; cgnum < required_cgs; ++cgnum) {
			/*
			 * Since we have already reached the barrier,
			 * another CG may have already gone onto the
			 * next barrier. Therefore, we can pass the
			 * barrier if all other CGs are at the barrier
			 * or past.
			 */
			if (sync_control.ss_stage[cgnum] >= stage)
				++num_complete;
			else
				incomplete_cg = cgnum;
		}
		if (num_complete == required_cgs) {
			return;
		}
#ifndef DEBUG_SBSP
		if (i == 0){
			/*
			 *+ A timeout occured waiting for the indicated
			 *+ CPU group to respond during OS initialization.
			 *+ Please check for the integrity of the
			 *+ interconnect hardware.
			 */
			if (mycg != BOOTCG) {
				asm("hlt");
			}
			if (incomplete_cg != CG_NONE) {
				cmn_err(CE_CONT, "stage[%d] = %x\n",
					incomplete_cg,
					sync_control.ss_stage[incomplete_cg]);
			}
			cmn_err(CE_PANIC,
				"sync_wait_barrier: CPU group %d timed out"
				" at barrier (%d, %d)\n",
				incomplete_cg, event, barrier);
		}
		ms_time_spin(SYNC_MSEC);
		--i;
#endif /* DEBUG_SBSP */
	}
}

/*
 * void
 * sysinit_sync_init(void)
 *	Initialize the sysinit_sync subsystem.
 *
 * Calling/Exit State:
 *	Called and base level (during system initialization) and
 *	returns that way. The interrupts are not affected.
 */
void
sysinit_sync_init(void)
{
	cgnum_t cgnum;

	for (cgnum = 0; cgnum < MAXNUMCG; ++cgnum) {
		sync_control.ss_stage[cgnum] =
			SYNC_MAKE_STAGE(0, SYNC_DONE_FIXUP);
	}
}

/*
 * void
 * sysinit_sync(sync_event_t sync_event, sync_type_t sync_type)
 *	Serialize the execute of the CGs
 *
 * Calling/Exit State:
 *	Called and base level (during system initialization) and
 *	returns that way. The interrupts are not affected.
 *
 * Remarks:
 *	sysinit_sync points are used by the initialization code via the
 *	following paradyme:
 *
 *		sysinit_sync(... sync point name ..., SYNC_SEQUENTIAL);
 *		.... serialized code (SA) ...
 *		sysinit_sync(... sync point name ..., SYNC_PARALLEL);
 *
 *	This sysinit_sync() pair builds a critical region for which
 *	execution of code (SA) is serialized. CG0 executes SA first,
 *	CG 1 executes seconds, and so forth, until CG (Ncg -1) executes
 *	SA last.
 *
 *	Also, during the period for which calloc()s are enabled,
 *	this sysinit_sync() pair has the effect of propogating
 *	any page table growth from the calloc()ing CGs to all others.
 *
 *	sysinit_sync_range() [see below] is an alternate to
 *	sysinit_sync(... sync point name ..., SYNC_PARALLEL);
 */
void
sysinit_sync(sync_event_t sync_event, sync_type_t sync_type)
{
	extern boolean_t hat_static_callocup;

	switch(sync_type) {
	case SYNC_SEQUENTIAL:
		/*
		 * Serialize CGs through calloc()ing [or any
		 * other initialization] code.
		 */
		sync_wait_barrier(sync_event, SYNC_PRE_CALLOC);

		/*
		 * Round calloc()s up to the next page.
		 * This is necessary because calloc's cannot
		 * cross a page boundary between CGs.
		 */
		sync_control.ss_begin_addr[mycg] = 0;
		if (hat_static_callocup) {
			callocrnd(mmu_ptob(1));
			sync_control.ss_begin_addr[mycg] =
				hat_static_nextvaddr();
		}

		/*
		 * Return to callocing initialization code.
		 */
		break;

	default:
		ASSERT(sync_type == SYNC_PARALLEL);

		/*
		 * Record if any page table allocation activity has
		 * occured on this CG.
		 */
		sync_control.ss_end_addr[mycg] = 0;
		if (hat_static_callocup)
			sync_control.ss_end_addr[mycg] = hat_static_nextvaddr();
		/*
		 * Wait at the barrier until all CGs are done
		 * calloc()ing.
		 */
		sync_wait_barrier(sync_event, SYNC_DONE_CALLOC);

		/*
		 * Now, propogate the page table just created to all
		 * the CGS.
		 */
		sync_post_action(B_FALSE);
		sync_wait_barrier(sync_event, SYNC_DONE_PT);

		/*
		 * Even more page tables were created in the
		 * last step. Now propogate them to the KL2PTEs
		 * of all other CGs.
		 */
		sync_post_action(B_TRUE);

		/*
		 * We need another barrier here, for the purpose
		 * of protecting the ss_begin_addr and ss_end_addr
		 * arrays.
		 */
		sync_wait_barrier(sync_event, SYNC_DONE_FIXUP);
		
		break;
	}
}

/*
 * void
 * sysinit_sync_range(sync_event_t event, vaddr_t start, vaddr_t end)
 *	An alternate to sysinit_sync(event, SYNC_PARALLEL), explicitly
 *	specifying the range of page tables to by sync'ed to be
 *	[start, end). Note that start is rounded down to a page
 *	boundary whereas end is rounded up.
 *
 * Calling/Exit State:
 *	Called and base level (during system initialization) and
 *	returns that way. The interrupts are not affected.
 */
void
sysinit_sync_range(sync_event_t sync_event, vaddr_t start, vaddr_t end)
{
	sync_control.ss_begin_addr[mycg] = start;
	sync_control.ss_end_addr[mycg] = end;

	/*
	 * Wait at the barrier until all CGs are done
	 * calloc()ing.
	 */
	sync_wait_barrier(sync_event, SYNC_DONE_CALLOC);

	/*
	 * Now, propogate the page table just created to all
	 * the CGS.
	 */
	sync_post_action(B_FALSE);
	sync_wait_barrier(sync_event, SYNC_DONE_PT);

	/*
	 * Even more page tables were created in the
	 * last step. Now propogate them to the KL2PTEs
	 * of all other CGs.
	 */
	sync_post_action(B_TRUE);

	/*
	 * We need another barrier here, for the purpose
	 * of protecting the ss_begin_addr and ss_end_addr
	 * arrays.
	 */
	sync_wait_barrier(sync_event, SYNC_DONE_FIXUP);
}

/*
 * void
 * scg_sysinit(cgnum_t cgnum)
 * 
 * Calling/Exit State:
 *
 * 	Called by the boot processor of a non-boot CG.
 *	Running in protected mode with paging enabled.
 *	calloc() will be enabled when this routine exits.
 *
 */
void
scg_sysinit(cgnum_t cgnum)
{
	struct ppriv_pages *ppriv;
	struct segment_desc *sd;
	struct gate_desc *gd;
	extern vaddr_t calloc_start_cg0;
	extern char stext[];
	extern char sdata[];
	extern char _end[];
	ulong_t cr0;
	int i, j;
	ulong_t len;
	timestruc_t tv;
#ifdef DEBUG
	boolean_t intcpu_default = B_FALSE;
	boolean_t plbase_after_init;
#endif /* DEBUG */

	graffiti();

	/*
	 * Acknowledge start.
	 */
	scg_starting = 0;

	ASSERT(PAE_ENABLED());

	l.hatops = &pae_hatops;
	/*
	 * Initialization of kl1ptp is ordinarily done by selfinit, but
	 * kl1ptp must be initialized prior to entering the debugger.
	 */
	for (i = 0; i < PDPTSZ; i++) {
		l.kl1ptp64[i] = &myengppriv_pae->pp_kl1pt[i][0];
		l.kpd64[i] = (pte64_t *)(KVENG_L1PT + ((i) * MMU_PAGESIZE));
	}
	graffiti();

	/*
	 * Our cglocal has been mapped and we are running paged. 
	 * Set our Processor Module Id.
	 */

 	mycg = cgnum;
	myengnum = gToLeader[cgnum];
	EMASK_S_INIT(&l.eng_mask, myengnum);
	graffiti();

	/*
	 * Initialize l.userp to the per-engine u area address.
	 */
	l.userp = &ueng;

	/*
	 * no kernel preemptions pending
	 */
	l.prmpt_state.s_prmpt_state.s_noprmpt = 1;

	/* 
	 * We need to be able to allocate physical memory
	 * before we sync. with the BOOTCG. After we do, 
	 * palloc and calloc will be enabled automatically,
	 * as the flags enabling them are still set on the BOOTCG.
	 * What we are doing is a way to get physical memory to
	 * before we can sync. up the BOOTCG.
	 */
	graffiti();
	hat_static_startcgpalloc();
	graffiti();

	/* 
	 * Clone the bootCG's text mappings if we don't want to turn on
	 * replicated text.
	 */
	if (!replicated_text)
		cg_clonept(BOOTCG, (vaddr_t)stext, (vaddr_t)sdata, B_FALSE);
	graffiti();

	/*
	 * Get in sync with calloc()s on the BOOT CGs. This is
	 * needed to access engine structures, cmn_err buffers,
	 * etc.
	 */
	sysinit_sync(SYNC_EVENT_MAP_BOOTCG_CALLOC, SYNC_SEQUENTIAL);
	sysinit_sync(SYNC_EVENT_MAP_BOOTCG_CALLOC, SYNC_PARALLEL);

	/*
	 * Clone page tables for phystokv
	 */
	graffiti();
	sysinit_sync(SYNC_EVENT_MAP_PHYSTOKV, SYNC_SEQUENTIAL);
	graffiti();
	sysinit_sync(SYNC_EVENT_MAP_PHYSTOKV, SYNC_PARALLEL);
	graffiti();

	/*
	 * Now prepare to begin calloc()ing ourselves.
	 */
	sysinit_sync(SYNC_EVENT_SERIALIZE1_CALLOC, SYNC_SEQUENTIAL);

	cg_l2map_misc(BOOTCG);

	hat_static_stopcgpalloc();
	cg_map_priv(cgnum);

	if (chip_detect() == -1) {
		/* Can't online this engine. */
		l.eng->e_flags |= E_BAD;
		EVENT_SIGNAL(&eng_wait, 0);
		for (;;)
			asm("hlt");
		/* NOTREACHED */
	}

	/*
	 * Running on an i486 or greater; enable the onchip
	 * cache.  While we're at it, set other control bits.
	 * Note that we're explicitly clearing the "Alignment
	 * Mask" bit to prevent alignment checking in user
	 * programs.  Also, enable kernel-mode write-protect
	 * checking.
	 */
	INVAL_ONCHIP_CACHE();
	cr0 = READ_MSW();
	if (disable_cache)
		cr0 |= (CR0_CD | CR0_NW);
	else
		cr0 &= ~(CR0_CD | CR0_NW);
	cr0 = (cr0 & ~CR0_AM) | CR0_WP;
	WRITE_MSW(cr0);

	/*
	 * Allocate and initialize global LDT.
	 */

	callocrnd(8);	/* LDT must be 8-byte aligned */
	myglobal_ldt = calloc(LDTSZ * 8);

	gd = (struct gate_desc *)&myglobal_ldt[seltoi(USER_SCALL)];
	BUILD_GATE_DESC(gd, KCSSEL, sys_call, GATE_386CALL, GATE_UACC, 1);
	gd = (struct gate_desc *)&myglobal_ldt[seltoi(USER_SIGCALL)];
	BUILD_GATE_DESC(gd, KCSSEL, sig_clean, GATE_386CALL, GATE_UACC, 1);
	sd = &myglobal_ldt[seltoi(USER_CS)];
	BUILD_MEM_DESC(sd, 0, mmu_btop(uvend), UTEXT_ACC1, TEXT_ACC2);
	sd = &myglobal_ldt[seltoi(USER_DS)];
	BUILD_MEM_DESC(sd, 0, mmu_btop(UVUVWIN)+1, UDATA_ACC1, DATA_ACC2);
	sd = &myglobal_ldt[seltoi(UVWINSEL)];
	BUILD_MEM_DESC(sd, UVUVWIN, 1, UDATA_ACC1, DATA_ACC2);

	myglobal_dt_info[DT_LDT].di_table = myglobal_ldt;
	myglobal_dt_info[DT_LDT].di_size = LDTSZ * 8;

	myglobal_dt_info[DT_GDT].di_table = l.global_gdt;
	myglobal_dt_info[DT_GDT].di_size = GDTSZ * 8;

	cg_setup_local(cgnum);

	selfinit(gToLeader[cgnum]);
}
#endif /* CCNUMA */

#ifndef CCNUMA
#ifndef MINI_KERNEL
/*
 * init_mtrrs is initialized by mtrrinit(), which is called by io_init.
 */
extern ullong_t *init_mtrrs;
#endif /* MINI_KERNEL */
#endif /* CCNUMA */

/*
 * void
 * selfinit(int procid)
 *
 * Calling/Exit State:
 *
 *	Perform per-processor initialization.
 */
void
selfinit(int procid)
{
	engine_t *eng;
	uint_t cr0, cr4;
	uint_t bustype;
	boolean_t eng_lock = B_FALSE;
	void (**funcp)(void);
	struct intr_list *ilistp;
	int i;
	extern boolean_t user_rdtsc;
	extern boolean_t user_rdpmc;
#ifdef DEBUG
	extern char stext[];
#endif /* DEBUG */

	eng = &engine[procid];

	/*
	 * Fill out fields in l.
	 */

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		l.hatops = &pae_hatops;
		for (i = 0; i < PDPTSZ; i++) {
			l.kl1ptp64[i] = &eng->e_local_pae->pp_kl1pt[i][0];
			l.kpd64[i] = (pte64_t *)(KVENG_L1PT + (i * MMU_PAGESIZE));
		}
	} else
#endif /* PAE_MODE */
	{
		l.hatops = &hatops32;
		l.kl1ptp = &eng->e_local->pp_kl1pt[0][0];
		l.kpd0 = (pte_t *)(KVENG_L1PT);
	}
	myengnum = procid;
	l.eng = eng;
	EMASK_S_INIT(&l.eng_mask, myengnum);

	/*
	 * Initialize per-processor GDT, IDT, TSS, and segment registers.
	 */
	init_desc_tables();

	/*
	 * Hack code to program memory above 4G on quad P6 with an Orion
	 * chipset.
	 */
	p6memselfinit();

	/*
	 * Initialize the engine's view of the u area.
	 */
	l.userp = &ueng;

	/*
	 * Init miscellaneous struct plocal members.
	 */
	l.one_sec = 1;
	l.holdfastlock = 0;

	/*
	 * Initialize the base kernel interrupt handlers. 
	 * External interrupts (clock, device, etc) are still
	 * disabled and wont be initialized until later on.
	 */
	msintrinit();

	/*
	 * Determine type of CPU/FPU/FPA.
	 * (chip_detect for boot processor done in sysinit().)
	 */
	if (!i_am_boot()) {

		/*
		 * no kernel preemptions pending
		 */
		l.prmpt_state.s_prmpt_state.s_prmpt_count = 1;
		l.prmpt_state.s_prmpt_state.s_noprmpt = 1;

		if (chip_detect() == -1) {
			/* Can't online this engine. */
			l.eng->e_flags |= E_BAD;
			EVENT_SIGNAL(&eng_wait, 0);
			for (;;)
				asm("hlt");
			/* NOTREACHED */
		}

		/*
		 * Running on an i486 or greater; enable the onchip
		 * cache.  While we're at it, set other control bits.
		 * Note that we're explicitly clearing the "Alignment
		 * Mask" bit to prevent alignment checking in user
		 * programs.  Also, enable kernel-mode write-protect
		 * checking.
		 */
		INVAL_ONCHIP_CACHE();
		cr0 = READ_MSW();
		if (disable_cache)
			cr0 |= (CR0_CD | CR0_NW);
		else
			cr0 &= ~(CR0_CD | CR0_NW);
		cr0 = (cr0 & ~CR0_AM) | CR0_WP;
		WRITE_MSW(cr0);
	}

	/*
	 * Set up floating point management.
	 *
	 * There may or may not really be hardware, regardless of what
	 * fp_kind indicates.  Init the FPU if it's there.
	 * Note that init_fpu() disables the FPU when it's done.
	 */
	l.fpuon = (READ_MSW() & ~(CR0_MP|CR0_EM|CR0_TS|CR0_NE));
	l.fpuoff = (l.fpuon | CR0_EM);
	l.fpuon |= CR0_MP;
	l.fpuon |= CR0_NE;	/* 486 numeric error mode */
	init_fpu();
	if (!(fp_kind & FP_HW))
		l.fpuon = l.fpuoff;

	/*
	 * The CR0_NE bit is only set on a i486 or above systems and when
	 * set enables the standard mechanism for reporting floating-point
	 * numeric errors.
	 *
	 * When the CR0_NE bit is clear a numeric error causes the processor
	 * to stop and wait for an interrupt.
	 *
	 * On a i486SX with an i387 coprocessor should have the CR0_NE bit
	 * clear and report numeric error thru interrupt 13.
	 */
	l.fpu_external = (fp_kind & FP_HW) && !(l.fpuon & CR0_NE);

	/*
	 * If necessary, set up floating-point emulator vectors.
	 */
	if (fp_kind == FP_SW)
		fpesetvec();

	/*
	 * Enable/disable special CPU features.
	 */
	if (l.cpu_features[0] & CPUFEAT_MCE) {
		if ((l.cpu_features[0] & CPUFEAT_MCA) && !ignore_machine_check) {
			ulong_t msr_cap[2];
			ulong_t msr_enable[2] = {0xffffffff, 0xffffffff};

			_rdmsr(MCG_CAP, msr_cap);
			mca_banks = msr_cap[0] & MCG_CAP_CNT;

/*
 *  Enable reporting for all banks
 *  Don't touch bank 0; the BIOS sets these values.
 */

			for (i = 1; i < mca_banks; i++)  {
#ifndef P6_BUGS_FIXED
				if (i == 1)
					msr_enable[0] = 0xfffffffe;
				else
					msr_enable[0] = 0xffffffff;
#endif
				_wrmsr(MC0_CTL + (i * MCA_REGS), msr_enable);
			}

#ifdef MCG_CTL
			if (msr_cap[0] & MCG_CTL_PRESENT)  {
				_wrmsr(MCG_CTL, msr_enable);
				mcg_ctl_present = B_TRUE;
			}
#endif
		}

		/* enable machine-check exceptions */
		cr4 = _cr4();
		if (ignore_machine_check)
			cr4 &= ~CR4_MCE;
		else
			cr4 |= CR4_MCE;
		_wcr4(cr4);
	}

	if (l.cpu_features[0] & CPUFEAT_TSC) {
		/* enable/disable user use of rdtsc */
		cr4 = _cr4();
		if (user_rdtsc)
			cr4 &= ~CR4_TSD;
		else
			cr4 |= CR4_TSD;
		_wcr4(cr4);
	}

	if (l.cpu_id == CPU_P6) {
		/* enable/disable user use of rdpmc */
		cr4 = _cr4();
		if (user_rdpmc)
			cr4 |= CR4_PCE;
		else
			cr4 &= ~CR4_PCE;
		_wcr4(cr4);
	}
	if (PGE_ENABLED()) {
		/* Turn on PGE */
		cr4 = _cr4();
		_wcr4(cr4 | CR4_PGE);
	}

	/*
	 * Compute l.cpu_speed for spin delay loops.  The value
	 * corresponds to the mips rating of the processor at it
	 * clock speed.
	 */
	if (l.cpu_id == CPU_486)
		l.cpu_speed = (i486_lcpuspeed * l.eng->e_cpu_speed) / 100;
	else
		l.cpu_speed = (lcpuspeed * l.eng->e_cpu_speed) / 100;

	/*
	 * Pre-compute PTE value for this engine's stack extension page.
	 */
	l.kse_pte.pg_pte = kvtol2ptep(l.eng->e_local->pp_kse)->pg_pte;
#ifdef PAE_MODE
	if (PAE_ENABLED())
		l.kse_pte64.pg_pte = pae_mkpte(PG_RW|PG_V,
			 pae_pfnum(kvtophys64(l.eng->e_local_pae->pp_kse)));
	else
#endif /* PAE_MODE */
		l.kse_pte.pg_pte = mkpte(PG_RW|PG_V,
			 pfnum(kvtophys(l.eng->e_local->pp_kse)));

	/*
	 * Intel Workaround for the "Invalid Operand
	 * with Locked Compare Exchange 8Byte (CMPXCHG8B) Instruction"
	 * Erratum.
	 */
	if (l.cpu_id == CPU_P5) {
		kvtol2ptep(p5_std_idt_pagebase)->pg_pte =
			(kvtol2ptep(l.std_idt)->pg_pte & ~(PG_US|PG_RW)) | PG_REF;
		loadidt(&p5_std_idt_desc);
	}

	/*
	 * Make FPU/FPA type visible to user process.
	 */
	uvwin.uv_fp_hw = fp_kind;

	/*
	 * Initialize the DMA controller.
	 * (Must be called before driver init routines).
	 */
	if (myengnum == BOOTENG)
		dma_init();

#ifdef CCNUMA
	if (myengnum != BOOTENG && (l.eng->e_flags & E_CGLEADER)) {	

		kvm_init();		/* order in which routines */
		clock_init();	 	/* are called  is important here  */
	}
#endif
	/*
	 * Announce our availability to the dispatching sub-system.
	 */
        disponline(eng);

#ifndef NODEBUGGER
	/*
	 * Let kernel debugger know we're here.
	 */
	(*cdebugger) (DR_ONLINE, NO_FRAME);
#endif

	/*
	 * Perform misc. initialization including interrupt
	 * distribution/assignments to non-boot engine.
	 */
#ifndef CCNUMA
#ifndef MINI_KERNEL
	/*
	 * Make MTRRs on non-boot processors consistent with the ones on the
	 * BSP. The mtrrinit() is called above (it's in io_init[]), and the
	 * MTRRs are copied into the memory pointed to by init_mtrrs. 
	 * The mini-kernel is ATUP, so we don't need this code, and the mtrr
	 * is not statically linked (not configured).
	 */
	if (myengnum != BOOTENG) {
		if (l.cpu_features[0] & CPUFEAT_MTRR && !disable_copy_mtrrs) {
			copy_mtrrs(init_mtrrs);
		}
	}
#endif /* MINI_KERNEL */
#endif /* CCNUMA */	

	asm("cli");		/* make sure interrupts are off */
	ms_init_cpu();

	if (!upyet) {
		/*
		 * Sync up all CG leaders at this point so that
		 * the PSM will be prepared to take interrupts
		 * from all PCI busses.
		 */
		sysinit_sync(SYNC_EVENT_PSM, SYNC_PARALLEL);
		sysinit_sync(SYNC_EVENT_CG_MAIN, SYNC_SEQUENTIAL);
	}

	spl0();
	/*
	 * Call driver init(D2D) routines.
	 */
	if (mycg == BOOTCG)
		for (funcp = io_init, i = 0; *funcp != NULL; i++, funcp++) {
			if (bindcpu_init[i] == myengnum ||
			    (i_am_boot() && bindcpu_init[i] == -1)) {
				(**funcp)();
			}
		}

	asm("sti");

	/*
	 * enable interrupts
	 * NOTE: debugger interrupts may occur before this point.
	 */

	
	/*
	 * blow away low memory mappings (below pmaplimit) for
	 * application parocessor start.
	 */
	if (upyet)
		sysinit_final();

	if (mycg == BOOTCG) {
	    if (i_am_boot()) {
		/*
		 * Attach interrupts for those who don't do it themselves.
		 */
		for (ilistp = static_intr_list; ilistp->il_name != NULL;
								ilistp++) {
			cm_intr_attach_all(ilistp->il_name,
					   (int (*)()) ilistp->il_handler,
					   ilistp->il_devflagp, NULL);
		}
	    }

	    /*
	     * Invoke module "start" functions [start(D2DK)].
	     */
	    for (funcp = io_start, i = 0; *funcp != NULL; i++, funcp++) {
		    if (bindcpu_start[i] == myengnum ||
			(i_am_boot() && bindcpu_start[i] == -1)) {
			    (**funcp)();
		    }
	    }

	    /*
	     * Check both bdevsw and cdevsw to see if there are
	     * any drivers bound to this engine. If so, lock
	     * the engine online because engines that have
	     * bound drivers cannot be offline'd.
	     */
	    for (i = 0; i < bdevcnt; i++) {
		    if ((bdevsw[i].d_cpu == myengnum) ||
			(bdevsw[i].d_cpu == -1 && myengnum == 0 &&
			!(*bdevsw[i].d_flag & D_MP))) {
			    eng_lock = B_TRUE;
			    break;
		    }
	    }

	    if (!eng_lock) {
		    for (i = 0; i < cdevcnt; i++) {
			    if ((cdevsw[i].d_cpu == myengnum) ||
				(cdevsw[i].d_cpu == -1 && myengnum == 0 &&
				!(*cdevsw[i].d_flag & D_MP))) {
				    eng_lock = B_TRUE;
				    break;
			    }
		    }
	    }

	    if (eng_lock && !engine_disable_offline(myengnum)) {
		    /*
		     *+ When bringing a processor online, the system
		     *+ fails to lock the engine online when there
		     *+ are drivers bound to the engine.
		     */
		    cmn_err(CE_PANIC, "selfinit: failed to lock engine\n");
	    }
	}


	/* Turn on hat accounting */
	hat_online();

	/*
	 * Signal that online is complete.
	 * MUST NOT come before disponline().
	 */
	EVENT_SIGNAL(&eng_wait, 0);

	/*
	 * Perform misc. initialization including interrupt
	 * distribution/assignments to non-boot engine.
	 */
        if (myengnum == BOOTENG) {
                /*
                 * Initialize fail-safe timer or sanity clock.
                 */
                if (cm_bustypes() & CM_BUS_EISA)
                        eisa_sanity_init();
        }
}

/*
 * void
 * sysinit_final(void)
 *	Now that we're off page 0, blow away the mapping of page
 *	table 0 and various pieces mapped by the  bootloader.
 *
 * Calling/Exit State:
 *	Called from main after modinit has established a mapping
 *	for the kernel symbol table.
 */

void
sysinit_final(void)
{
	vaddr_t addr;

	for (addr = 0; addr < pmaplimit; addr += PAE_VPTSIZE)
		(void) remap_pagetable(addr, 0);
}

/*
 * Given the CPU vendor string, family and model obtained by detect_cpu(),
 * the cputable provides information identifying the processor as it
 * will be known to the kernel, and names for reporting to the user.
 *
 * We search the table for the first entry with a matching vendor ID string
 * and matching (or don't-care) family and model.  Because of the presence
 * of don't-care values, the order of entries in the table is important.
 *
 * The cpu_id value (CPU_XXX) tells the kernel what kind of processor to
 * treat this as.  Because this value is copied to l.cpu_id and used in
 * runtime checks in several places in the kernel, care should be used when
 * assigning the cpu_id for processors that aren't "GenuineIntel".
 *
 * The processor name (cpu_fullname) and abbreviated name (cpu_abbrev) are
 * for reporting to the user by various means.  The abbreviated name should
 * be at most 8 characters in length.
 *
 * Intel 386 and early 486 processors don't support the cpuid instruction, so
 * detect_cpu() leaves l.cpu_vendor empty for those cases.  Entries near the
 * end of the table, with a vendor of "", will match these.  The final entry
 * in the table causes any unrecognized processor to be treated as a 486.
 */

#define X		(~0)		/* don't-care for family and model */

STATIC const struct cputable {
	const char	*cpu_vendor;	/* vendor string from chip */
	int		cpu_family;	/* processor family from chip */
	int		cpu_model;	/* processor model from chip */
	int		cpu_id;		/* what to treat it as (CPU_XXX) */
	const char	*cpu_fullname;	/* what to call it */
	const char	*cpu_abbrev;	/* abbreviation of cpu_fullname */
} cputable[] = {
	"GenuineIntel",	5, X, CPU_P5,	"Pentium",	"Pentium",
	"GenuineIntel",	6, 3, CPU_P6,	"Pentium II",	"Pent II",
	"GenuineIntel",	6, X, CPU_P6,	"Pentium Pro",	"Pent Pro",
	"GenuineIntel",	4, X, CPU_486,	"486",		"486",
	"AuthenticAMD",	4, X, CPU_486,	"Am5x86",	"Am5x86",
	"AuthenticAMD",	5, 0, CPU_486,	"AMD-K5",	"AMD-K5",
	"AuthenticAMD",	5, 1, CPU_486,	"AMD-K5",	"AMD-K5",
	"AuthenticAMD",	5, 2, CPU_486,	"AMD-K5",	"AMD-K5",
	"AuthenticAMD",	5, 3, CPU_486,	"AMD-K5",	"AMD-K5",
	"AuthenticAMD",	5, 4, CPU_486,	"AMD-K5",	"AMD-K5",
	"AuthenticAMD",	5, 5, CPU_486,	"AMD-K5",	"AMD-K5",
	"AuthenticAMD",	5, X, CPU_486,	"AMD-K6",	"AMD-K6",
	"CyrixInstead",	4, X, CPU_486,	"5x86",		"5x86",
	"CyrixInstead",	5, X, CPU_486,	"6x86",		"6x86",
	"",		4, X, CPU_486,	"486",		"486",
	"",		3, X, CPU_386,	"386",		"386",
	NULL,		X, X, CPU_486,	"486",		"486"
};

/*
 * void
 * identify_cpu(void)
 *	Initialize cpu identification fields in plocal structure.
 *
 * Calling/Exit State:
 *	Called during processor initialization and uses only processor
 *	local or read-only data; therefore no locking is required.
 *
 * Remarks:
 *	We first call detect_cpu() to interrogate the chip.  This will
 *	set the cpu_vendor, cpu_family, cpu_model, cpu_stepping and
 *	cpu_features fields of the plocal structure; based on this
 *	information, we decide here what type of CPU (CPU_486, CPU_P5,
 *	etc.) to treat this as, and how to identify it to the user.
 *	This is done by table lookup from the cputable above, and is
 *	used to set the cpu_id, cpu_fullname and cpu_abbrev fields of
 *	the plocal structure.
 *
 *	detect_cpu() gives us raw information from the chip without
 *	interpretation.  If any massaging or adjustment needs to be
 *	done before the rest of the kernel starts making decisions
 *	based on this information, it is done here.  For example, if
 *	we've found ourselves running on a non-Intel processor, it's
 *	probably unwise to assume that the feature bits we may have
 *	gotten from the chip are compatible with Intel's, so we just
 *	discard them.
 */

void
identify_cpu(void)
{
	const struct cputable *cpu;

	detect_cpu();

	for (cpu = &cputable[0]; cpu->cpu_vendor != NULL; cpu++) {
		if (strncmp(cpu->cpu_vendor, l.cpu_vendor, NCPUVENDCH) != 0)
			continue;
		if (cpu->cpu_family != l.cpu_family && cpu->cpu_family != X)
			continue;
		if (cpu->cpu_model != l.cpu_model && cpu->cpu_model != X)
			continue;
		break;
	}

	l.cpu_id = cpu->cpu_id;
	l.cpu_fullname = cpu->cpu_fullname;
	l.cpu_abbrev = cpu->cpu_abbrev;

	/*
	 * For now, ignore the feature bits on non-Intel processors.
	 */
	if (strncmp(l.cpu_vendor, "GenuineIntel", NCPUVENDCH) != 0)
		bzero(l.cpu_features, sizeof l.cpu_features);

	/*
	 * If for whatever reason we are not going to be using PAE mode,
	 * pretend that the processor doesn't support it.
	 */
	if (!using_pae)
		l.cpu_features[0] &= ~CPUFEAT_PAE;
}

#undef X

/*
 * STATIC int
 * chip_detect(void)
 *	Detect CPU/FPU/FPA types.
 *
 * Calling/Exit State:
 *	Returns -1 if the online cannot succeed due to unsupported
 *	configurations.
 *
 *	Called when booting or when an engine is coming online for
 *	the first time, and uses only processor-local variables or
 *	globals which are constant once the system is up, so no locking
 *	is needed.
 */
STATIC int
chip_detect(void)
{
	static int detect_flags;
#define DETECT_FPU	0x01
	int my_fp_kind;

	identify_cpu();

	if (l.cpu_id == CPU_386) {
		/*
		 *+ This release of UnixWare does not support
		 *+ the Intel 386 microprocessor. Please upgrade to
		 *+ to Intel 486 hardware (or later). The last major
		 *+ release of UnixWare to support the i386 was 2.1.
		 */
		cmn_err(CE_PANIC, "i386 processor not supported; "
				  "use i486 or later");
	}

	if (i_am_boot()) {
		if (fp_kind == 0) /* auto-detect */
			detect_flags |= DETECT_FPU;
	}

	if (detect_flags & DETECT_FPU)
		my_fp_kind = detect_fpu();
	else if (!((my_fp_kind = fp_kind) & FP_HW))
		my_fp_kind = FP_NO;
	if (my_fp_kind == FP_NO && fp_kind == FP_SW && !i_am_boot())
		my_fp_kind = FP_SW;

#ifdef DEBUG
	{
		static char *fpu_name[] = {
			"no fpu", "no fpu", "80287", "80387 (or equiv)"
		};
		cmn_err(CE_CONT, "# processor %d: ", myengnum);
		if (l.cpu_vendor[0])
			cmn_err(CE_CONT, "\"%.*s\" ", NCPUVENDCH, l.cpu_vendor);
		cmn_err(CE_CONT, "%s model %d ", l.cpu_fullname, l.cpu_model);
		cmn_err(CE_CONT, "step %d;", l.cpu_stepping);
		cmn_err(CE_CONT, " %s\n", fpu_name[my_fp_kind]);
	}
#endif /* DEBUG */

	if (i_am_boot())
		fp_kind = my_fp_kind;
	else if (my_fp_kind != fp_kind) {
		if ((my_fp_kind & FP_HW) && !(fp_kind & FP_HW)) {
			/*
			 *+ All processors must have the same type
			 *+ of floating-point hardware (none or 
			 *+ 80387).  To take advantage of math
			 *+ coprocessors, you must install them for
			 *+ all processors.
			 */
			cmn_err(CE_NOTE,
			  "Mixed FPU configurations not supported;\n"
			  "\tfloating-point coprocessor for processor "
			  "%d not used.", myengnum);
		} else {
			/*
			 *+ All processors must have the same type
			 *+ of floating-point hardware (none
			 *+ or 80387).  Since the processor being
			 *+ brought online does not have the same
			 *+ math coprocessor as the boot processor,
			 *+ it cannot be used.  To fix: install a
			 *+ math coprocessor on this processor.
			 */
			cmn_err(CE_NOTE,
			  "Mixed FPU configurations not supported;\n"
			  "\tprocessor %d cannot be brought online.",
			  myengnum);
			return -1;
		}
	}

	return 0;
}

/*
 * ullong_t
 * remap_pagetable(vaddr_t vaddr, ullong_t new_l1pte)
 *
 *	Remap the first 2 (pae mode) or 4 (non-pae mode) of
 *	the virtual address space by changing the #0 level 1
 *	page directory entry.
 *
 * Calling/Exit State:
 *
 *	Called with preemption disabled.
 *
 * Usage:
 *
 *	Called to map in/out the BIOS, memory mapped P==V by the
 *	boot loader, and the reset code.
 *
 * XXX: This code does not yet use the new macros for changing pae
 *	pte in the correct word order. It truely needs it (for we can't have
 *	some speculative P6 load touching some hardware register by mistake).
 */
ullong_t
remap_pagetable(vaddr_t vaddr, ullong_t new_l1pte)
{
	ullong_t old_l1pte;

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		pte64_t *l1ptep;

		l1ptep = vtol1ptep64(vaddr);
		old_l1pte = l1ptep->pg_pte;
		l1ptep->pg_pte = new_l1pte;
	} else
#endif /* PAE_MODE */
	{
		pte_t *l1ptep;

		l1ptep = vtol1ptep(vaddr);
		old_l1pte = l1ptep->pg_pte;
		l1ptep->pg_pte = new_l1pte;
	}

	TLBSflushtlb();
	return old_l1pte;
}

/*
 * void
 * offline_self(void)
 *	Called for a processor to take itself offline.
 *
 * Calling/Exit State:
 *	None.
 */
void
offline_self(void)
{
	engine_t *eng = l.eng;

#ifndef UNIPROC
	/*
	 * There may be affinitized LWPs on the local run queue.
	 * Empty local run queue (tranfer them to global run queue)
	 * before proceeding further with offlining self.
	 */
	empty_local_runqueue();
#endif
        /*
         * Redistribute interrupts to other cpus. Once the following call
	 * completes, no other interrupts will be delivered to this cpu.
	 * However, interrupts may already be queued for this cpu. After disabling
	 * acceptance of new interrupts, we must go to spl0 to let pending
	 * interrupts complete.
         */
	DISABLE();
        ms_offline_prep();
	ENABLE();
	spl0();

	/*
	 * Restore the reset_code mapping if we are
	 * re-onlined later.
	 */
	(void) remap_pagetable((vaddr_t) reset_code, pstart_kl1pte);

	/*
	 * Let the HAT know we're no longer available.
	 */
	hat_offline();

	/*
	 * Disable all interrupts.
	 */
	splhi();

	/*
	 * Let the dispatcher know we're no longer available.
	 */
	dispoffline(eng);

	/*
	 * Flush out local KMA buffers.
	 */
	kma_offline_self();

	EVENT_SIGNAL(&eng_wait, 0);

	DISABLE();
	ms_offline_self();	/* never returns */

	/* NOTREACHED */

}


/*
 * STATIC void
 * mem_vmapin(te_t *pt, uint_t npages, addr_t vaddr, int prot)
 *
 *	Duplicately map the specified virtual pages.
 *
 * Calling/Exit State:
 *
 *	"vaddr" is the virtual address of "npages" virtually contiguous
 *	pages which are to be duplicately mapped using the array of
 *	pte's, "pt".
 *
 *	Pages are MMU_PAGESIZE bytes long.
 */
void
mem_vmapin(pte_t *pt, uint_t npages, vaddr_t vaddr, int prot)
{
	int	i;

	for (i = 0; i < npages; i++, pt++) {
		pt->pg_pte = mkpte(prot, pfnum(kvtophys(vaddr)));
		vaddr += MMU_PAGESIZE;
	}
}

/*
 * STATIC void
 * pae_mem_vmapin(pte64_t *pt, uint_t npages, addr_t vaddr, int prot)
 *
 *	Duplicately map the specified virtual pages.
 *
 * Calling/Exit State:
 *
 *	"vaddr" is the virtual address of "npages" virtually contiguous
 *	pages which are to be duplicately mapped using the array of
 *	pte's, "pt".
 *
 *	Pages are MMU_PAGESIZE bytes long.
 */
STATIC void
pae_mem_vmapin(pte64_t *pt, uint_t npages, vaddr_t vaddr, int prot)
{
	int	i;

	for (i = 0; i < npages; i++, pt++) {
		pt->pg_pte = pae_mkpte(prot, pae_pfnum(kvtophys64(vaddr)));
		vaddr += MMU_PAGESIZE;
	}
}

/*
 * STATIC void
 * setup_priv_maps(int procid, int master_procid)
 *
 *	Set up page tables for the specified processor
 *	based on the BOOTENG master processor.
 *
 * Calling/Exit State:
 *
 *	procid is the processor ID to set up.
 *
 *	procid is the processor ID to setup.
 *	master_procid is the processor ID to clone.
 *	Called from BOOTENG after creating all static kernel mappings
 *	and disabling calloc(), but before creating any user mappings.
 *
 * Description:
 *
 *	Shared page mappings are copied from the master.
 *	Per-engine mappings are setup appropriately.
 */
void
setup_priv_maps(int procid, int master_procid)
{
	struct	ppriv_pages *mppriv;
	struct	ppriv_pages *ppriv;
	pte_t	*kl1pt;
	pte_t	*pmap;
#ifdef DEBUG
	int i;
#endif

	ASSERT(procid != BOOTENG);
	if (procid == master_procid)
		return;		/* already setup */

	mppriv = engine[master_procid].e_local;	/* master ppriv */
	ppriv = engine[procid].e_local;		/* ppriv to setup */

	engine[procid].e_cglocal = engine[master_procid].e_cglocal;
	ENGINE_PLOCAL_PTR(procid)->kvpte[0] = l.kvpte[0];

	kl1pt = &ppriv->pp_kl1pt[0][0];
	pmap  = &ppriv->pp_pmap[0][0];

	/*
	 * Grab a copy of the master level 1 page table.  We need the whole
	 * thing since there are at this point some temporary kernel mappings
	 * in what is normally user address space (the "page 0" mappings).
	 *
	 * We also copy the per-engine level 2 since it contains some entries
	 * which are not actually per-engine, but global.
	 */

	bcopy(&mppriv->pp_kl1pt[0][0], kl1pt, NPGPT * sizeof(pte_t));
	bcopy(&mppriv->pp_pmap[0][0], pmap, NPGPT * sizeof(pte_t));

	/*
	 * Overwrite the per-engine level 1 & 2 entries.
	 *
	 * Note that these mappings must agree with those in build_table().
	 *
	 * We can't merely loop over the pages in struct ppriv_pages
	 * establishing consecutive mappings for each consecutive page
	 * since there exist gaps in kernel virtual space between some
	 * of the pages.
	 *
	 * Note:  KVPLOCAL must be mapped user read-write for the
	 * floating-point emulator.  See the KVPLOCAL comment near
	 * the similar mapping call in build_table().
	 */
	kl1pt[ptnum(KVPER_ENG_STATIC)].pg_pte = 
		 mkpte(PG_US | PG_RW | PG_V, pfnum(kvtophys((vaddr_t)pmap)));

	kl1pt[ptnum(l.kvpte[0])].pg_pte = 
		 mkpte(PG_US | PG_RW | PG_V, pfnum(kvtophys((vaddr_t)kl1pt)));

	mem_vmapin(&pmap[pgndx(KVPLOCAL)], PL_PAGES,
			(vaddr_t) &ppriv->pp_local[0][0], PG_US | PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVPLOCALMET)],
			PLMET_PAGES * (PAGESIZE / MMU_PAGESIZE),
			(vaddr_t) &ppriv->pp_localmet[0][0], PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVENG_L2PT)], 1,
			(vaddr_t) &ppriv->pp_pmap[0][0], PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVENG_L1PT)], 1,
			(vaddr_t) &ppriv->pp_kl1pt[0][0], PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVUVWIN)], 1,
			(vaddr_t) &ppriv->pp_uvwin[0][0], PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(UVUVWIN)], 1,
			(vaddr_t) &ppriv->pp_uvwin[0][0], PG_US | PG_V);
	mem_vmapin(&pmap[pgndx(KVUENG)], USIZE * (PAGESIZE / MMU_PAGESIZE),
		   (vaddr_t)&ppriv->pp_ublock[0][0], PG_US | PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVUENG_EXT)], KSE_PAGES,
		   (vaddr_t)&ppriv->pp_uengkse[0][0], PG_RW | PG_V);
}

/*
 * STATIC void
 * pae_setup_priv_maps(int procid)
 *
 *	Set up page tables for the specified processor
 *	based on the BOOTENG master processor.
 *
 * Calling/Exit State:
 *
 *	procid is the processor ID to set up.
 *
 *	Called from BOOTENG after creating all static kernel mappings
 *	and disabling calloc(), but before creating any user mappings.
 *
 * Description:
 *
 *	Shared page mappings are copied from the master.
 *	Per-engine mappings are setup appropriately.
 */
STATIC void
pae_setup_priv_maps(int procid, int master_procid)
{
	struct	ppriv_pages_pae *mppriv;
	struct	ppriv_pages_pae *ppriv;
	pte64_t	*kl1pt, *l1pt;
	pte64_t	*pmap;
	int	i;

	ASSERT(procid != BOOTENG);
	if (procid == master_procid)
		return;		/* already setup */

	mppriv = engine[master_procid].e_local_pae;	/* master ppriv */
	ppriv = engine[procid].e_local_pae;		/* ppriv to setup */

	engine[procid].e_cglocal = engine[master_procid].e_cglocal;

	/*
	 * We also copy the per-engine level 2 since it contains some entries
	 * which are not actually per-engine, but global.
	 */
	pmap = &ppriv->pp_pmap[0][0];
	bcopy(&mppriv->pp_pmap[0][0], pmap, PAE_NPGPT * sizeof(pte64_t));

	for (i = 0; i < PDPTSZ; i++) {
		/*
		 * Grab a copy of the master level 1 page table.  
		 * We need the whole thing since there are at this
		 * point some temporary kernel mappings in what is
		 * normally user address space (the "page 0" mappings).
		 */
		ENGINE_PLOCAL_PAE_PTR(procid)->pdpte[i].pg_pte = 
			pae_mkpte(PG_V, pae_pfnum(
				kvtophys64((vaddr_t)&ppriv->pp_kl1pt[i][0])));
		ENGINE_PLOCAL_PAE_PTR(procid)->kvpte64[i] = l.kvpte64[i];
		kl1pt = &ppriv->pp_kl1pt[i][0];
		bcopy(&mppriv->pp_kl1pt[i][0], kl1pt, PAE_NPGPT * sizeof(pte64_t));
	}

	/*
	 * Overwrite the per-engine level 1 & 2 entries.
	 *
	 * Note that these mappings must agree with those in build_table().
	 *
	 * We can't merely loop over the pages in struct ppriv_pages
	 * establishing consecutive mappings for each consecutive page
	 * since there exist gaps in kernel virtual space between some
	 * of the pages.
	 *
	 * Note:  KVPLOCAL must be mapped user read-write for the
	 * floating-point emulator.  See the KVPLOCAL comment near
	 * the similar mapping call in build_table().
	 */

#ifdef DEUBG
	/*
	 * If we get here, we are a non-boot cpu in this Processor Module.
	 * As we just copied all the pte's from pp_pmap, the second level page 
	 * table of the master_procid, and as we share our  CG-local pte's with
	 * the master_procid, we assert here that these pte's are non-zero.
	 */
	for (i = 0; i < CGL_PAGES; i++) 
		ASSERT(pmap[pae_pgndx(KVCGLOCAL + i * MMU_PAGESIZE)].pg_pte);
#endif /* DEUBG */

	kl1pt = &ppriv->pp_kl1pt[3][0];
	kl1pt[pae_ptnum(KVPER_ENG_STATIC_PAE)].pg_pte = 
		 pae_mkpte(PG_US | PG_RW | PG_V, pae_pfnum(kvtophys64((vaddr_t)pmap)));
	for (i = 0; i < PDPTSZ; i++) {
		l1pt = &ppriv->pp_kl1pt[i][0];
		kl1pt[pae_ptnum(l.kvpte64[i])].pg_pte = 
			pae_mkpte(PG_US | PG_RW | PG_V, pae_pfnum(kvtophys64((vaddr_t)l1pt)));
	}

	pae_mem_vmapin(&pmap[pae_pgndx(KVPLOCAL)], PL_PAGES,
			(vaddr_t) &ppriv->pp_local[0][0], PG_US | PG_RW | PG_V);
	pae_mem_vmapin(&pmap[pae_pgndx(KVPLOCALMET)],
			PLMET_PAGES * (PAGESIZE / MMU_PAGESIZE),
			(vaddr_t) &ppriv->pp_localmet[0][0], PG_RW | PG_V);
	pae_mem_vmapin(&pmap[pae_pgndx(KVENG_L2PT)], 1,
			(vaddr_t) &ppriv->pp_pmap[0][0], PG_RW | PG_V);
	pae_mem_vmapin(&pmap[pae_pgndx(KVENG_L1PT)], PDPTSZ,
			(vaddr_t) &ppriv->pp_kl1pt[0][0], PG_RW | PG_V);
	pae_mem_vmapin(&pmap[pae_pgndx(KVUVWIN)], 1,
			(vaddr_t) &ppriv->pp_uvwin[0][0], PG_RW | PG_V);
	pae_mem_vmapin(&pmap[pae_pgndx(UVUVWIN)], 1,
			(vaddr_t) &ppriv->pp_uvwin[0][0], PG_US | PG_V);
	pae_mem_vmapin(&pmap[pae_pgndx(KVUENG)], USIZE * (PAGESIZE / MMU_PAGESIZE),
		   (vaddr_t)&ppriv->pp_ublock[0][0], PG_US | PG_RW | PG_V);
	pae_mem_vmapin(&pmap[pae_pgndx(KVUENG_EXT)], KSE_PAGES,
		   (vaddr_t)&ppriv->pp_uengkse[0][0], PG_RW | PG_V);
}

STATIC struct segment_desc tmp_gdt[GDTSZ];
STATIC struct gate_desc tmp_idt[IDTSZ];
STATIC struct tss386 tmp_tss;
uchar_t *v86_bitmap;

/*
 * Since the TSS and I/O bitmap needs to be contiguous, they are defined 
 * together.
 */
struct {
	struct tss386 v_tss;
	uchar_t v_bitmap[IOB_MAXPORT_MAP];
} v86bios_tss;

struct tss386 *v86_tss;		/* TSS for V86 mode */

/*
 * STATIC void
 * tmp_init_desc_tables(void)
 *
 *	Setup enough of a GDT to allow IDT to be established.
 *
 * Calling/Exit State:
 *
 *	Called immediately after enabling the MMU.  This routine allows
 *	the kernel to detect various faults/traps in a relatively graceful
 *	manner.
 */
STATIC void
tmp_init_desc_tables(void)
{
	struct idt_init *id;
	struct segment_desc *sd;
	struct gate_desc *gd;
	struct desctab desctab;

	/*
	 * Build GDT
	 */
	sd = &tmp_gdt[seltoi(KCSSEL)];
	BUILD_MEM_DESC(sd, 0, SD_MAX_SEG, KTEXT_ACC1, TEXT_ACC2);
	sd = &tmp_gdt[seltoi(KDSSEL)];
	BUILD_MEM_DESC(sd, 0, SD_MAX_SEG, KDATA_ACC1, DATA_ACC2);
	sd = &tmp_gdt[seltoi(KTSSSEL)];
	BUILD_SYS_DESC(sd, &tmp_tss, sizeof(struct tss386), TSS3_KACC1,
								TSS_ACC2);

	tmp_tss.t_cr3 = _cr3();
	tmp_tss.t_ss0 = KDSSEL;
	tmp_tss.t_esp0 = 0x1000;

	sd = &tmp_gdt[seltoi(KV86TSSSEL)];

	v86_tss = (struct tss386 *) &v86bios_tss.v_tss;

	BUILD_SYS_DESC(sd, v86_tss, sizeof(v86bios_tss), 
		       TSS3_KACC1, TSS_ACC2); 
	v86bios_tss.v_bitmap[IOB_MAXPORT_MAP - 1] = 0xff; /* terminator */

	/*
	 * Build IDT
	 */
	for (gd = tmp_idt; gd < &tmp_idt[IDTSZ]; gd++)
		BUILD_GATE_DESC(gd, KCSSEL, t_res, GATE_386INT, GATE_KACC, 0);

	for (id = idt_init; id->idt_addr; id++) {
		gd = &tmp_idt[id->idt_desc];
		BUILD_GATE_DESC(gd, KCSSEL, id->idt_addr, id->idt_type,
							  id->idt_priv, 0);
	}

	gd = &tmp_idt[V86INT];
	BUILD_GATE_DESC(gd, KCSSEL, t_v86, GATE_386INT, GATE_UACC, 0);

	/*
	 * Init GDTR and IDTR to start using new stuff.
	 */
	BUILD_TABLE_DESC(&desctab, tmp_idt, IDTSZ);
	loadidt(&desctab);

	BUILD_TABLE_DESC(&desctab, tmp_gdt, GDTSZ);
	loadgdt(&desctab);

	/*
	 * Initialize the segment registers.
	 */
	setup_seg_regs();

	loadtr(KTSSSEL);
}

/*
 * STATIC void
 * init_desc_tables(void)
 *
 * Calling/Exit State:
 *
 *	Set up processor-local copy of GDT and IDT, and start using them.
 */
STATIC void
init_desc_tables(void)
{
	struct idt_init *id;
	struct segment_desc *sd;
	struct gate_desc *gd;

	/*
	 * Build GDT
	 */

	sd = &l.global_gdt[seltoi(KCSSEL)];
	BUILD_MEM_DESC(sd, 0, SD_MAX_SEG, KTEXT_ACC1, TEXT_ACC2);
	sd = &l.global_gdt[seltoi(KDSSEL)];
	BUILD_MEM_DESC(sd, 0, SD_MAX_SEG, KDATA_ACC1, DATA_ACC2);
	sd = &l.global_gdt[seltoi(KTSSSEL)];
	BUILD_SYS_DESC(sd, &l.tss, sizeof(l.tss), TSS3_KACC1, TSS_ACC2);
	sd = &l.global_gdt[seltoi(DFTSSSEL)];
	BUILD_SYS_DESC(sd, &l.dftss, sizeof(l.dftss), TSS3_KACC1, TSS_ACC2);
	sd = &l.global_gdt[seltoi(KLDTSEL)];
	BUILD_SYS_DESC(sd, myglobal_ldt, LDTSZ * 8, LDT_KACC1, LDT_ACC2);
	sd = &l.global_gdt[seltoi(KV86TSSSEL)];
	BUILD_SYS_DESC(sd, v86_tss, sizeof(struct tss386) + IOB_MAXPORT_MAP, 
		       TSS3_KACC1, TSS_ACC2);

	/*
	 * Build IDT
	 */

	cur_idtp = l.std_idt;
	for (gd = cur_idtp; gd < &cur_idtp[IDTSZ]; gd++)
		BUILD_GATE_DESC(gd, KCSSEL, t_res, GATE_386INT, GATE_KACC, 0);

	/*
	 * Initialize IDT entries common to all platforms. In particular,
	 * traps and faults IDT entries are initialized.
	 */ 
	for (id = idt_init; id->idt_addr; id++) {
		gd = &cur_idtp[id->idt_desc];
		BUILD_GATE_DESC(gd, KCSSEL, id->idt_addr, id->idt_type,
							  id->idt_priv, 0);
	}

	gd = &cur_idtp[V86INT];
	BUILD_GATE_DESC(gd, KCSSEL, t_v86, GATE_386INT, GATE_UACC, 0);

	BUILD_GATE_DESC(&cur_idtp[DBLFLT], DFTSSSEL, 0, GATE_TSS, GATE_KACC, 0);

	fpuon_noextflt = fpuoff_noextflt = cur_idtp[NOEXTFLT];

	/*
	 * Init GDTR and IDTR to start using new stuff.
	 */
	BUILD_TABLE_DESC(&mystd_idt_desc, cur_idtp, IDTSZ);
	loadidt(&mystd_idt_desc);

	BUILD_TABLE_DESC(&ueng.u_gdt_desc, l.global_gdt, GDTSZ);
	loadgdt(&ueng.u_gdt_desc);
	ueng.u_dt_infop[DT_GDT] = &myglobal_dt_info[DT_GDT];

	/*
	 * Initialize the segment registers.
	 */
	setup_seg_regs();

	/*
	 * Fill out the TSS and load the TSS register.
	 * TSS was zeroed by selfinit() when the whole plocal structure
	 * got zapped.
	 */
	l.tss.t_ss0 = KDSSEL;
	l.tss.t_esp0 = (ulong_t)UBLOCK_TO_UAREA(KVUENG);
	l.tss.t_bitmapbase = TSS_NO_BITMAP;	/* no user I/O allowed */

	/*
	 * Following are read-only when swithing tasks, thus, needed to be
	 * set in advance. Otherwise, they are set to zero, causing
	 * conflicts set below, like loadldt(KLDTSEL).
	 */
	l.tss.t_cr3 = _cr3();
	l.tss.t_ldt = KLDTSEL;
	loadtr(KTSSSEL);

	/*
	 * Indicate that we're going to use the global LDT.
	 */
	ueng.u_dt_infop[DT_LDT] = &myglobal_dt_info[DT_LDT];
	loadldt(KLDTSEL);

	/*
	 * Fill out the double-fault TSS.
	 * This TSS was zeroed by selfinit() when the whole plocal structure
	 * got zapped.  Don't set the stack pointer without checking if it
	 * has already been set; a kernel debugger may already have set it.
	 */
	if (l.dftss.t_esp == 0)
		l.dftss.t_esp = (ulong_t)UBLOCK_TO_UAREA(KVUENG);
	l.dftss.t_esp0 = l.dftss.t_esp;
	l.dftss.t_ss0 = l.dftss.t_es = l.dftss.t_ss = l.dftss.t_ds = KDSSEL;
	l.dftss.t_cs = KCSSEL;
	l.dftss.t_eip = (ulong_t)t_syserr;
	l.dftss.t_bitmapbase = TSS_NO_BITMAP;
	l.dftss.t_cr3 = _cr3();
}	

/* 
 * void
 * sim_cg_init(void)
 *
 *	Arranges for allocation of CG-local data structures
 *	and sets up private maps of application processors in this CG.
 *
 * Calling/Exit State:
 *
 *	It is called from kvm_init.
 */
void
sim_cg_init(void)
{
	int i;
	int engno;
	int cgleader = gToLeader[mycg];

	for (i = 0; i < cg_array[mycg].cg_cpunum; i++) {
		engno = cg_array[mycg].cg_cpuid[i];
		if (engno != cgleader) {
#ifdef PAE_MODE
			if (PAE_ENABLED())
				pae_setup_priv_maps(engno, cgleader);
			else
#endif /* PAE_MODE */
				setup_priv_maps(engno, cgleader);
		}
	}
}

#ifdef CCNUMA

/*
 * void
 * cg_setup_repvar(void)
 *
 *	Sets up global (CGVAR) mappings for the cglocal pages.
 *
 * Calling/Exit State:
 *
 *	This routine can only be called by the CG on which the mappings
 *	are being established.
 *
 *	Must be called only after level 2 page tables are allocated
 *	for CGVAR pages.
 *
 * Description:
 *
 *	"cg_array_base" is the base address of the region in kernel virtual
 *	range of global kernel which maps the CG-local pages. Starting at
 *	cg_array_base, each set of CGL_PAGES represents the global mappings
 *	for the CG-local pages of the next CG.
 */
void
cg_setup_repvar(void)
{
	vaddr_t source_addr, target_addr;
	ullong_t pte64;

	ASSERT(PAE_ENABLED());

	source_addr = KVCGLOCAL;
	target_addr = cg_array_base + mycg * ptob(CGL_PAGES);
	while (source_addr < KVCGLOCAL + ptob(CGL_PAGES)) {
		pte64 = kvtol2ptep64(source_addr)->pg_pte;
		FOR_EACH_CG_PTE(target_addr)
			cg_ptep->pg_pte = pte64;
		END_FOR_EACH_CG_PTE
		source_addr += ptob(1);
		target_addr += ptob(1);
	}
}

/*
 * STATIC void
 * cg_setup_local(cgnum_t cgnum)
 *
 * 	Allocate statically defined cpu group local data structures
 * 	and allocate private pages for this processor as well as
 * 	all the application processors in the CG. 
 *
 * Calling/Exit State:
 *
 * 	The translation maps for CG-local pages of this CG as well as
 *	private pages for the boot processor of this CG would have been
 *	set up when we exit. Storage will have been allocated for the private
 *	pages of application processors, but they will not have been
 *	mapped yet.
 */

STATIC void
cg_setup_local(cgnum_t cgnum)
{
	int 	i, j;
	uint_t 	pprivpages;
	struct 	ppriv_pages_pae *ppriv;    /* phys addr of my ppriv_pages */

	pprivpages = mmu_btopr(sizeof(struct ppriv_pages_pae)) * 
			(cg_array[cgnum].cg_cpunum - 1);

	callocrnd(mmu_ptob(1));
	ppriv = (struct ppriv_pages_pae *)calloc(mmu_ptob(pprivpages));

	engine[gToLeader[cgnum]].e_flags |= E_CGLEADER;

	/* set up storage for other processors in this CG */
	if (cg_array[cgnum].cg_cpunum > 1) {
		for (j = 0; j < cg_array[cgnum].cg_cpunum; j++) {
			if (cg_array[cgnum].cg_cpuid[j] != gToLeader[cgnum]) {
				i = cg_array[cgnum].cg_cpuid[j];
				engine[i].e_local_pae = ppriv++;
				engine[i].e_cglocal = 
					engine[gToLeader[cgnum]].e_cglocal;
			}
		}
	}
}
#endif /* CCNUMA */

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_kernel_addrs(void)
 *
 *	Print the value of misc. kernel virtual addresses for debugging.
 *
 * Calling/Exit State:
 *
 *	Intended to be called from a kernel debugger.
 */
void
print_kernel_addrs(void)
{
extern  char stext[];
extern  char sdata[];
#define X(v)	debug_printf("0x%x	"#v"\n", v)
	
	debug_printf("\n");

	X(KVTMPPG2);
	X(KVTMPPG1);
	X(KVMET);
	X(KVPLOCALMET);
	X(KVPLOCAL);
	X(KVCGLOCAL);
	X(KVENG_L2PT);
	X(KVENG_L1PT);
	X(KVUENG);
	X(KVUENG_REDZONE);
	X(KVUVWIN);
	X(UVUVWIN);
	X(KVFPEMUL);
	X(KVLAST_ARCH);
	
	debug_printf("\n");

	X(KVDISP_MONO);
	X(KVDISP_COLOR);
	X(KVPER_ENG);
	
	debug_printf("\n");

	X(stext);
	X(sdata);
	
	debug_printf("\n");

	X(l.kvpte[0]);
	X(KL2PTES);
	X(KL2PTES_SIZE(Ncg));
	X(kvbase);
	X(KVAVBASE);
	X(KVAVEND);
	X(KVPHYSTOKV);

	debug_printf("\n");

}

#endif /* DEBUG || DEBUG_TOOLS */


/*
 * The following code programs the quad P6 Orion chipset to configure 
 * memory above 4G. 
 *
 * Note: This is a hack and the code must be removed before the
 *	 product is shipped.
 */

#define OPB0 0xc8
#define OBP1 0xd0
#define OMC 0xa0

extern unsigned int p6splitmem;

unsigned int p6mtrr_base[] = {0,0};
unsigned int p6mtrr_range[] = {0,0};

unsigned int
orion_read(unsigned int chip, unsigned int reg)
{
	unsigned int cv;

	cv = 0x80000000;
	cv |= chip << 8;
	cv |= reg;
	outl( 0xcf8, cv );
	return inl(0xcfc);
}

void
orion_write(unsigned int chip, unsigned int reg, unsigned int val)
{
	unsigned int cv;

	cv = 0x80000000;
	cv |= chip << 8;
	cv |= reg;
	outl( 0xcf8, cv );
	outl( 0xcfc, val );
}


void
p6memselfinit(void)
{
	if (!p6splitmem)
		return;

	_wrmsr( 0x202, p6mtrr_base );
	_wrmsr( 0x203, p6mtrr_range );

	_rdmsr( 0x200, p6mtrr_base );
	_rdmsr( 0x201, p6mtrr_range );
	p6mtrr_base[0] = ((p6mtrr_base[0] >> 1) & 0xfffff000) | 6;
	p6mtrr_range[0] = ((p6mtrr_range[0] >> 1) & 0xfffff000) | 0x0800;
	_wrmsr( 0x200, p6mtrr_base );
	_wrmsr( 0x201, p6mtrr_range );
}

void
p6dosplit(void)
{
    unsigned long ms, ms2, opb_tsm, omc_hmgsa, omc_hmgea;

    if (!p6splitmem)
	return;

    opb_tsm = orion_read( OPB0, 0x40 );
    ms = opb_tsm & 0xffff;
    ms2 = ms / 2;
    cmn_err(CE_NOTE, "%d Mbytes present; moving %d Mbytes to 4G\n", ms, ms2 );
    p6mtrr_base[1] = 1;
    p6mtrr_base[0] = 6;
    p6mtrr_range[1] = 1;
    p6mtrr_range[0] = 0x0800;

    opb_tsm &= 0xffff0000;
    opb_tsm |= 4096 + ms2;

    omc_hmgsa = orion_read( OMC, 0x88 );
    omc_hmgsa &= 0xffff0000;
    omc_hmgsa |= 0xC0000000 + ms2;

    omc_hmgea = orion_read( OMC, 0x8c );
    omc_hmgea &= 0xffff0000;
    omc_hmgea |= 4095;

    orion_write( OPB0, 0x40, opb_tsm );
    orion_write( OMC, 0x88, omc_hmgsa );
    orion_write( OMC, 0x8c, omc_hmgea );
}
