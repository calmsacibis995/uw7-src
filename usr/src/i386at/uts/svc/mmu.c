#ident	"@(#)kern-i386at:svc/mmu.c	1.49.9.5"
#ident	"$Header$"

/*
 * Build page tables when booting the system.
 *
 * These routines are executed when still executing under the page
 * tables build by the boot loader. Memory below the PMAPLIMIT is
 * mapped P==V.
 */

#include <io/kd/kd.h>
#include <mem/hatstatic.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/immu64.h>
#include <mem/tuneable.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <mem/pmem.h>
#include <proc/cg.h>
#include <svc/creg.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/kdb/xdebug.h>
#include <util/inline.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <svc/v86bios.h>

extern boolean_t pse_supported(void);
extern vaddr_t kvper_eng;
extern char stext[];	/* stext = beginning of kernel text */
extern char sdata[];	/* stext = beginning of kernel data */
extern char sbss[];	/* sbss = start of kernel bss */
extern char _etext[];	/* _etext = last byte of kernel text plus 1 */
extern char _edata[];	/* _edata = last byte of kernel data plus 1 */
extern char _end[];	/* _edata = last byte of kernel bss plus 1 */
extern cgnum_t this_cg_num;
extern void xprintf(int, const char *, VA_LIST, const char *);
extern void xcmn_panic(const char *, VA_LIST);
extern boolean_t using_pae;
extern paddr_t pmaplimit;	/* memory below is mapped P==V by the boot */
extern paddr_t pstart_l1addr;
extern paddr_t bios_limit;
extern paddr_t bios_offset;
extern boolean_t replicated_text;

/*
 * Variables in pstart.s (before relocation)
 */
extern void switch_pae_template(pte_t *);
extern void reset_code_template(pte_t *);
extern uint_t reset_cr0_template;
extern uint_t reset_cr3_template;
extern uint_t reset_cr4_template;
extern paddr_t gdtbase_template;
extern void (*reset_function_template)(void);
extern paddr_t reset_real_template;
extern paddr_t reset_prot_template;

/*
 * flags for xprintf
 */
#define TO_PUTBUF       0x1
#define TO_CONSOLE      0x2

/*
 * Variables in pstart.s (after relocation)
 */
STATIC	void		(*switch_pae)(pte_t *);
	void		(*reset_code)(void);
	uint_t		*reset_cr0;
	uint_t		*reset_cr3;
	uint_t		*reset_cr4;
	void		(**reset_function)(void);
	paddr_t		*reset_real;
	paddr_t		*reset_prot;
	paddr_t		*gdtbase;

#ifdef RESET_CODE_DEBUG

#define RELOCATE_FUNC(symbol, type, disp)	( \
	symbol = (type) ((paddr_t) symbol ## _template + (disp)), \
	phys_printf(#symbol " at 0x%x\n", (symbol)) \
)

#define RELOCATE_SYM(symbol, disp)	( \
	symbol = (void *) ((paddr_t) &symbol ## _template + (disp)), \
	phys_printf(#symbol " at 0x%x\n", (symbol)) \
)

#define FIXUP_SYM(symbol, offset, target) 	( \
	phys_printf("fixing up " #symbol " at 0x%x, old value 0x%x\n", \
		(caddr_t)(symbol) + (offset), \
		*((paddr_t *)((caddr_t)(symbol) + (offset)))), \
	*((paddr_t *)((caddr_t)(symbol) + (offset))) += (disp), \
	phys_printf("fixed up " #symbol " new value 0x%x\n", \
		*((paddr_t *)((caddr_t)(symbol) + (offset)))) \
)

#else /* RESET_CODE_DEBUG */

#define RELOCATE_FUNC(symbol, type, disp)	\
	(symbol = (type) ((paddr_t) symbol ## _template + (disp)))

#define RELOCATE_SYM(symbol, disp)	\
	(symbol = (void *) ((paddr_t) &symbol ## _template + (disp)))

#define FIXUP_SYM(symbol, offset, target)	\
	(*((paddr_t *)((caddr_t)(symbol) + (offset))) += (disp))

#endif /* RESET_CODE_DEBUG */


typedef struct {
	char	p_contents[MMU_PAGESIZE];
} page_contents_t;

/*
 * Physical address of the level 1 page table of the boot engine.
 * Used by kernel crashdump analyzers which assume that the physical
 * address of this variable is equal to the symbol table value.
 *
 * Note: This variable must be declared in a file that will be loaded V=P.
 */

paddr_t crash_kl1pt = (paddr_t)0;
int crash_paeenabled = 0;

/*
 * The following values are set here and exported (via _start())
 * to sysinit():
 *
 * vmemptr	- virtual address of next unused virtual memory.
 *		  This is page-aligned when exported to sysinit().
 *
 * myengppriv	- virtual address of this engine's struct ppriv_pages.
 *
 *		  The pp_kl1pt and pp_pmap pages are already in use as the
 *		  current engine's page tables when exported to sysinit().
 *
 * mycgpriv	- virtual address of this engine's struct cgpriv_pages.
 *
 *		  will be exported to sysinit()/prmd_init().
 */

vaddr_t calloc_base;			/* where calloc mappings begin */
vaddr_t vmemptr;			/* next unused virtual memory */
struct ppriv_pages *myengppriv = (struct ppriv_pages *)0;
struct ppriv_pages_pae *myengppriv_pae = (struct ppriv_pages_pae *)0;
					/* virt addr of my ppriv_pages */
pte_t *pkl1pt;				/* kernel level 1 page table */

#ifdef PAE_MODE
pte64_t *pkl1pt64_cg[MAXNUMCG];		/* kernel level 1 page table */
#define pkl1pt64	(pkl1pt64_cg[this_cg_num])
STATIC	pte64_t *pul1pt64; 	/* user level 1 page table for page0 */
pte_t *pkpdpt;			/* kernel page directory pointer table */

#ifdef CCNUMA
STATIC	pte64_t	*pkl2ptes;
#endif
#endif /* PAE_MODE */

pte_t 	*bootcg_kl1pt;

struct cgpriv_pages *mycgpriv;	/* virt addr of my ppriv_pages */

STATIC	void	build_table();
STATIC	void	pcreate_map(vaddr_t, uint_t, paddr_t, int);
STATIC	void	pae_build_table(void);
STATIC	void	pae_pcreate_map(vaddr_t, uint_t, paddr_t, int);
STATIC paddr64_t phys_palloc(uint_t);
STATIC void	premap(vaddr_t, paddr_t, size_t, int prot);

/*
 * The pcreate_map table for use by build_table_common.
 */
typedef struct {
	vaddr_t pmt_vaddr;
	uint_t	pmt_size;
	paddr_t	pmt_paddr;
	int	pmt_prot;
} pcreate_map_table_t;
STATIC pcreate_map_table_t pcreate_map_table[] = {
	/*
	 * Allocate and map the per-engine pages for the current
	 * engine (plocal, u-area, kernel stack extension, ...)
	 *
	 * Note that these mappings must agree with those in
	 * setup_priv_maps().
	 *
	 * KVUENG note:
	 *
	 * The 80386 B1-step Errata #13 workaround requires the
	 * portion of the kernel stack used to return to user mode to
	 * have its page table entry set for user read access, but it
	 * does not actually require the user to be able to read the
	 * stack.
	 *
	 * Further, since the 80x86 does not allow separate page
	 * protections to be specified for user and supervisor mode,
	 * and since the kernel needs write permission to this area,
	 * we grant the user both read and write permission to the
	 * entire KVUENG.
	 *
	 * We rely on 80x86 segmentation limits to restrict the
	 * user from both reading and writing this area.
	 *
	 * KVPLOCAL note:
	 *
	 * User read and write permissions are also needed for
	 * floating- point emulator access to the floating-point
	 * state in l.fpe_kstate. This access will be via a separate
	 * USER_FP segment, which will not allow access to the rest
	 * of KVPLOCAL.
	 */

	/* 0 */ {KVPLOCAL, (PL_PAGES * MMU_PAGESIZE),
				0, PG_US | PG_RW | PG_V},
	/* 1 */ {KVPLOCALMET, (PLMET_PAGES * PAGESIZE), 0,PG_RW | PG_V},
	/* 2 */ {KVENG_L2PT, MMU_PAGESIZE, 0,PG_RW | PG_V},
	/* 3 */ {KVENG_L1PT, MMU_PAGESIZE, 0,PG_RW | PG_V},
	/* 4 */ {KVUVWIN, MMU_PAGESIZE, 0,PG_RW | PG_V},
	/* 5 */ {UVUVWIN, MMU_PAGESIZE, 0,PG_US | PG_V},
	/* 6 */ {KVUENG, (USIZE * PAGESIZE), 0, PG_US | PG_RW | PG_V},
	/* 7 */ {KVUENG_EXT, (KSE_PAGES * MMU_PAGESIZE),
				0, PG_RW | PG_V},
	/*
	 * KVUENG_REDZONE: per-engine kernel stack redzone; not mapped
	 */

	/*
	 * Map various hardware addresses.
	 */

	/* 8 */ {KVDISP_MONO, NP_DISP_MONO * MMU_PAGESIZE,
				MONO_BASE, PG_RW | PG_V},
	/* 9 */ {KVDISP_COLOR, NP_DISP_COLOR * MMU_PAGESIZE,
				COLOR_BASE, PG_RW | PG_V},

	/*
	 * Set up a mapping for page 0, for communicating with
	 * the BIOS ROM.
	 */
	/* 10 */ {KVPAGE0, MMU_PAGESIZE, 0, PG_RW | PG_V | PG_US},

	/* 11 */ {V86BIOS_VIDEO, V86BIOS_SIZE,
		  V86BIOS_VIDEO, PG_RW | PG_V | PG_US | PG_CD},

	/*
	 * Set up mapping for CG local data.
	 */

	/* 12 */ {KVCGLOCAL, (CGL_PAGES * MMU_PAGESIZE),
			0, PG_US | PG_RW | PG_V},
#ifdef CCNUMA
	/* 13 */ {KVENG_PAGES, (SZPPRIV_PAGES_PAE * MMU_PAGESIZE), 0, PG_RW | PG_V},
#endif /* CCNUMA */

	{0, 0, 0, 0}
};


#if defined(PHYS_DEBUG) || defined(lint)

/*
 * STATIC void
 * phys_assfail(const char *assert_expr, const char *file, const char *line)
 *	Print a failure message if an assert fails during early startup.
 *
 * Calling/Exit State:
 *	assert_expr is the failed assertion.
 *	file and line are the source file and line number where the
 *		assertion is located.
 *
 *	No return from this procedure, it goes into a spin loop
 *	after calling printf to print its argument on the console.
 */
int
phys_assfail(const char *assert_expr, const char *file, const char *line)
{
	phys_panic("assertion failed: %s, file: %s, line: %s",
		   assert_expr, file, line);
}

#define PHYS_BCMP(v, p, s)	phys_bcmp(v, p, s)

/*
 * void
 * phys_bcmp(uint_t *va, uint_t *pa, size_t size)
 *	Compare two buffers of memory.
 *
 * Calling/Exit State:
 *	None.
 */
void
phys_bcmp(uchar_t *va, uchar_t *pa, size_t size)
{
	while (size > 0) {
		if (*va != *pa) {
			phys_panic("phys_bcmp: va %x, pa %x, *va %x, *pa %x",
				va, pa, *va, *pa);
		}
		++va;
		++pa;
		--size;
	}
}

#else /* PHYS_DEBUG || lint */

#define PHYS_BCMP(v, p, s)	((void)0)

#endif	/* PHYS_DEBUG || lint */

#ifdef DEBUG_SBSP

#define GRAFFITI_SIZE 0x100
int graffiti_buf[GRAFFITI_SIZE];
int gridx=0;
volatile int break_line=0;
#define graffiti() (*(graffiti_buf + gridx) = __LINE__, \
		    gridx = (gridx + 1) % GRAFFITI_SIZE)
#define stop_here() { \
	graffiti(); \
	while (__LINE__ == break_line) \
		; \
}

typedef struct {
	int	tr_lineno;
	char	*tr_format;
	uint_t	tr_arg1;
	uint_t	tr_arg2;
	uint_t	tr_arg3;
	uint_t	tr_arg4;
} trace_buf_t;

#define TRACE_POINT(fmt, arg1, arg2, arg3, arg4) {			\
	early_trace_buf[early_trace_index].tr_lineno = __LINE__;	\
	early_trace_buf[early_trace_index].tr_format = (fmt);		\
	early_trace_buf[early_trace_index].tr_arg1 = (uint_t)(arg1);	\
	early_trace_buf[early_trace_index].tr_arg2 = (uint_t)(arg2);	\
	early_trace_buf[early_trace_index].tr_arg3 = (uint_t)(arg3);	\
	early_trace_buf[early_trace_index].tr_arg4 = (uint_t)(arg4);	\
	if (++early_trace_index == EARLY_TRACE_BUF_SIZE) {		\
		early_trace_index = 0;					\
	}								\
}
#define TRACE_POINT0(fmt) \
	TRACE_POINT((fmt), 0, 0, 0, 0)
#define TRACE_POINT1(fmt, arg1) \
	TRACE_POINT((fmt), (arg1), 0, 0, 0)
#define TRACE_POINT2(fmt, arg1, arg2) \
	TRACE_POINT((fmt), (arg1), (arg2), 0, 0)
#define TRACE_POINT3(fmt, arg1, arg2, arg3) \
	TRACE_POINT((fmt), (arg1), (arg2), (arg3), 0)

#define EARLY_TRACE_BUF_SIZE	10000
trace_buf_t	early_trace_buf[EARLY_TRACE_BUF_SIZE];
int		early_trace_index;

void
print_early_trace(void)
{
	trace_buf_t *trp, *start_trp, *last_trp;

	trp = start_trp = &early_trace_buf[early_trace_index];
	last_trp = &early_trace_buf[EARLY_TRACE_BUF_SIZE];
	do {
		if (trp->tr_lineno != 0) {
			debug_printf("line %d ", trp->tr_lineno);
			debug_printf(trp->tr_format, trp->tr_arg1,
				     trp->tr_arg2, trp->tr_arg3, trp->tr_arg4);
			debug_printf("\n");
		}
		++trp;
		if (trp == last_trp)
			trp = early_trace_buf;
	} while (trp != start_trp);
}

void
remote_graffiti(int line) {
	*(graffiti_buf + gridx) = line;
	gridx = (gridx + 1) % GRAFFITI_SIZE;
	while (line == break_line)
		;
}

#else

#define graffiti()
#define stop_here()
#define TRACE_POINT0(fmt)
#define TRACE_POINT1(fmt, arg1)
#define TRACE_POINT2(fmt, arg1, arg2)
#define TRACE_POINT3(fmt, arg1, arg2, arg3)
#define TRACE_POINT(fmt, arg1, arg2, arg3, arg4)

#endif /* DEBUG_SBSP */

/*
 * Delay following each print
 */
#define	PHYS_IDLE	0x04000000
#define _PAUSE		{ int i; for (i = 0 ; i < PHYS_IDLE ; ++i) ; }

/*
 * void
 * phys_printf(char *fmt, ...)
 *	Print formatted I/O during early mode startup operations.
 *
 * Calling/Exit State:
 *	The boot loader console has already been opened via
 *	console_openchan.
 */
void
phys_printf(char *fmt, ...)
{
	VA_LIST ap;

	VA_START(ap, fmt);

	xprintf(TO_CONSOLE|TO_PUTBUF, fmt, ap, NULL);

	_PAUSE;
}

/*
 * void
 * phys_panic(char *fmt, ...)
 *	Panic during during early mode startup operations.
 *
 *	If a debugger is present, then go through the standard
 *	panic routine (in order to allow for better debuggability).
 *
 * Calling/Exit State:
 *	The boot loader console has already been opened via
 *	console_openchan.
 */
void
phys_panic(char *fmt, ...)
{
	VA_LIST ap;

	VA_START(ap, fmt);

	if (*cdebugger != nullsys)
		xcmn_panic(fmt, ap);

	printf("\nPANIC: ");
	xprintf(TO_CONSOLE|TO_PUTBUF, fmt, ap, NULL);
	printf("\nLooping forever.\n");
	/* infinite spin loop */
	for (;;)
		continue;
	/*NOTREACHED*/
}

/*
 * void
 * start(char *argv)
 *	Initial Kernel Entry Point.
 *
 * Calling/Exit State:
 *	Called from the boot loader 
 *
 *	argv points to the boot loader arguments (which are
 *	in the BKI segment).
 *
 *	Called while running in protected mode with paging on
 *	(non-PAE mode in all cases).
 *
 *	Initialize the kernel page table and turn on mapping.
 *
 *	Called from start() with physical addressing mode active.
 *
 *	Returns with virtual addressing mode active and the kernel
 *	virtual address space setup.  BSS is zeroed.  The following
 *	global variables have been set (so they may be passed by 
 *	_start() as arguments to sysinit()):
 *
 *	memNOTused - linked list of all the physical memory segments that
 *		       are not used by kernel text and data.
 *
 *	vmemptr	   - page-aligned virtual address of next unused
 *		     virtual memory (to initialize calloc()).
 *
 *	myengppriv - virtual address of this engine's struct ppriv_pages.
 *		     The pp_kl1pt and pp_pmap pages are already in use as
 *		     the current engine's level 1 and level 2 page tables.
 */
void
start(char *argv)
{
	extern char *bootstring;
	extern void _start(void);
	extern void make_bootinfo(void);
	extern void find_unused_mem(void);

	if (this_cg_num == BOOTCG) {
		/*
		 * Save a pointer to the boot args.
		 */
		bootstring = argv;

		/*
		 * TEMPORARY: make a fake bootinfo from boot parameters
		 *
		 * Also, initialize the temporary console.
		 */
		make_bootinfo();

		/*
		 * Fill out the the memNOTused list with all the physical
		 * memory segment that are not used by kernel text and data.
		 */
		find_unused_mem();
	}

	/*
	 * Build kernel page tables.
	 * Start allocating physical memory at the first page past
	 * the end of the kernel (including symbol table if present).
	 */

	PHYS_PRINT("start: calling pae_supported\n");
	graffiti();
#ifdef PAE_MODE
	if (using_pae) {
		PHYS_PRINT("start: calling pae_build_table\n");
		graffiti();
		pae_build_table();
	} else
#endif /* PAE_MODE */
	{
		PHYS_PRINT("start: calling build_table\n");
		graffiti();
		build_table();
	}

	/*
	 * Load CR3 with kernel level 1 page table root
	 * and enable paging.
	 */
	PHYS_PRINT("start: enable paging\n");
	if (pse_supported())
		_wcr4(_cr4() | CR4_PSE);
	graffiti();
#ifdef PAE_MODE
	if (using_pae) {
		PHYS_PRINT("enable_paging: calling switch_pae\n");
		PHYS_PRINT("enable_paging: pkpdpt = %x\n", pkpdpt);
		crash_paeenabled = 1;
		(*switch_pae)(pkpdpt);
		graffiti();
	} else
#endif /* PAE_MODE */
	{
		PHYS_PRINT("enable_paging: loading l1pt\n");
		PHYS_PRINT("enable_paging: pkl1pt = %x\n", pkl1pt);
		WRITE_PTROOT((ulong_t)pkl1pt);
		graffiti();
	}
	PHYS_PRINT("start: exit\n");
	graffiti();
	_start();
}

/*
 * void
 * prime_reset_code(void (*function)(void))
 *	Set for the reset_code (when invoked by a PSM) to
 *	call the function specified by the argument.
 *
 * Calling/Exit State:
 *	Called during system initialization while the reset_code
 *	is still mapped P==V.
 */
void
prime_reset_code(void (*function)(void))
{
	*reset_cr0 = _cr0();
	*reset_cr3 = _cr3();
	if (pse_supported() || pae_supported())
		*reset_cr4 = _cr4();
	else
		*reset_cr4 = 0;
	*reset_function = function;
}

/*
 * STATIC vaddr_t
 * build_table_common(void)
 *
 *	The common portion of build_table and pae_build_table.
 *
 * Calling/Exit State:
 *
 *	Called during early system initialization while running
 *	with the page table prepared by the boot laoder.
 *
 *	Returns the virtual address of the next available address
 *	in the calloc space.
 *
 *	Also sets variable calloc_base (when called on CG0).
 */

STATIC vaddr_t
build_table_common(void)
{
	vaddr_t vaddr;
	paddr_t paddr, src;
	uint_t msize;
	uint_t size;
	struct mets *pmet;			/* phys addr of metrics */
	struct pmem_extent *memlistp;
	uint_t cgprivpages;
	struct cgpriv_pages *cgpriv;        /* phys addr of my cgpriv_pages */
	pcreate_map_table_t *tp;
	int prot, npages;
	uint_t disp;
#ifdef PHYS_DEBUG
	boolean_t mapped_text = B_FALSE;
	boolean_t mapped_data = B_FALSE;
	boolean_t mapped_resmgr = B_FALSE;
	boolean_t mapped_license = B_FALSE;
	boolean_t mapped_ksym = B_FALSE;
	boolean_t mapped_boot = B_FALSE;
	boolean_t mapped_pstart = B_FALSE;
	boolean_t mapped_bki = B_FALSE;
	boolean_t mapped_reserved = B_FALSE;
#endif /* PHYS_DEBUG */

	PHYS_PRINT("build_table_common: enter\n");

	/*
	 * Allocate physical memory for cglocal pages.
	 */
	cgprivpages = mmu_btopr(sizeof(struct cgpriv_pages));
	cgpriv = (struct cgpriv_pages *) phys_palloc(cgprivpages);

	/*
	 * fill in some more pcreate_map_table entries
	 */
	pcreate_map_table[11].pmt_paddr += bios_offset;
	pcreate_map_table[12].pmt_paddr = (paddr_t)&cgpriv->cg_local[0][0];

	/*
	 * do pcreate_map()s, driven by the table
	 */
	for (tp = pcreate_map_table; tp->pmt_size != 0; ++tp) {
		pcreate_map(tp->pmt_vaddr, tp->pmt_size,
			    tp->pmt_paddr, tp->pmt_prot);
	}

	/*
	 * Map kernel text, data, bss, symbol table, boot, pstart,
	 * reserved areas below the bios-limit, 
	 * certain sections below section
	 * according to the memUsed list.  Text segment is mapped readonly,
	 * data segments (including symbol table if present) are mapped
	 * read/write. (readonly is only effective on the 80486 and above).
	 */

	vaddr = (vaddr_t)stext;
        PHYS_PRINT("stext = 0x%x\n", stext);
	for (memlistp = memUsed; memlistp != NULL; memlistp = memlistp->pm_next) {
		src = memlistp->pm_base;
		msize = memlistp->pm_extent;
		PHYS_PRINT("build table: memUsed <0x%x, 0x%x> use %d\n",
			src, src + msize, memlistp->pm_use);
		/* Page zero is reserved for the bios. */
		PHYS_ASSERT(memlistp->pm_use == PMEM_RESERVED || src >= ptob(1));
		PHYS_ASSERT(msize > 0);
		prot = (PG_RW | PG_V);
		switch (memlistp->pm_use) {
		case PMEM_PSTART:
#ifdef PHYS_DEBUG
			mapped_pstart = B_TRUE;
#endif /* PHYS_DEBUG */
			PHYS_PRINT("mapping type %d <vaddr, len> = <%x, %x>\n",
				(long) memlistp->pm_use,
				(long) src, (long) msize);
			PHYS_ASSERT(msize == PAGESIZE);
			pcreate_map(src, msize, src, prot);
#ifdef CCNUMA
			if (this_cg_num != BOOTCG)
				continue;
#endif /* CCNUMA */
			bzero((void *) src, PAGESIZE);
			bcopy((void *) reset_code_template, (void *) src,
			      MMU_PAGESIZE);
			disp = src - (paddr_t) reset_code_template;
			RELOCATE_FUNC(switch_pae, void (*)(pte_t *), disp);
			RELOCATE_FUNC(reset_code, void (*)(void), disp);
			RELOCATE_SYM(reset_cr0, disp);
			RELOCATE_SYM(reset_cr3, disp);
			RELOCATE_SYM(reset_cr4, disp);
			RELOCATE_SYM(reset_function, disp);
			RELOCATE_SYM(gdtbase, disp);
			RELOCATE_SYM(reset_prot, disp);
			RELOCATE_SYM(reset_real, disp);

			/*
			 * Now do the fixups.
			 *	 1 and 2 are the displacements of
			 *	 the fixup targets from the labels,
			 *	 respectively.
			 */
			FIXUP_SYM(reset_prot, 1, disp);
			FIXUP_SYM(reset_real, 2, disp);
			FIXUP_SYM(gdtbase, 0, disp);
#ifdef CCNUMA
			/*
			 * In the ccNUMA case, we set up for
			 * start of the SBSP processors.
			 * They will start using the page
			 * tables presented by the boot loader.
			 */
			prime_reset_code((void (*)(void)) start);
#endif /* CCNUMA */
			break;

		case PMEM_RESERVED:
			/*
			 * Setup for V86 bios and frame buffer use.
			 */
			if (src >= bios_limit)
				continue;
			if (src + msize > bios_limit)
				msize = bios_limit - src;
#ifdef PHYS_DEBUG
			mapped_reserved = B_TRUE;
#endif /* PHYS_DEBUG */
			prot |= PG_CD | PG_US;
			PHYS_PRINT("mapping type %d <vaddr, len> = <%x, %x>\n",
				(long) memlistp->pm_use,
				(long) src, (long) msize);
			pcreate_map(src, msize, src - bios_offset, prot);
			break;

		case PMEM_BKI:
		case PMEM_KSYM:
		case PMEM_KDEBUG:
		case PMEM_BOOT:
#ifdef PHYS_DEBUG
			if (memlistp->pm_use == PMEM_BKI)
				mapped_bki = B_TRUE;
			if (memlistp->pm_use == PMEM_KSYM)
				mapped_ksym = B_TRUE;
			if (memlistp->pm_use == PMEM_BOOT)
				mapped_boot = B_TRUE;
#endif /* PHYS_DEBUG */
			PHYS_PRINT("mapping type %d <vaddr, len> = <%x, %x>\n",
				(long) memlistp->pm_use,
				(long) src, (long) msize);
			pcreate_map(src, msize, src, prot);
			break;

		case PMEM_KTEXT:
#ifndef USE_GDB
			prot = PG_V;
#endif
#ifndef CCNUMA
		case PMEM_KDATA:
#endif /* CCNUMA */
			PHYS_ASSERT((vaddr & PSE_PAGEOFFSET) == 0);
#ifdef PHYS_DEBUG
			if (memlistp->pm_use == PMEM_KTEXT)
				mapped_text = B_TRUE;
			if (memlistp->pm_use == PMEM_KDATA) {
				mapped_data = B_TRUE;
				PHYS_ASSERT(vaddr == (vaddr_t)sdata);
			}
#endif /* PHYS_DEBUG */
			/*
			 * Use a PSE mapping for text/text if PSE is supported
			 * on this engine, and if we can allocate the
			 * memory for it. The memory will be preferentially
			 * non-DMAable.
			 */
			npages = mmu_btopr(msize);
			if (replicated_text && pse_supported() && 
			    (paddr = phys_palloc3(npages, PSE_PAGESIZE,
						  pmaplimit)) != PALLOC_FAIL) {
				/*
				 * Copy the text and map it in.
				 */
				PHYS_ASSERT((paddr & PSE_PAGEOFFSET) == 0);
				PHYS_PRINT("creating PSE mapping "
					   " of type %d at vaddr %x\n",
					memlistp->pm_use, (long) vaddr);
				bcopy((void *) src, (void *) paddr, msize);
#ifndef CCNUMA
				if (memlistp->pm_use == PMEM_KDATA)
					premap(vaddr, paddr, msize, prot);
#endif /* CCNUMA */
				/*
				 * Round up the mapping size to the next
				 * PSE page boundary to allow pcreate_map
				 * to use a PSE mapping.
				 */
				msize = (msize + PSE_PAGEOFFSET) & PSE_PAGEMASK;
				pcreate_map(vaddr, msize, paddr, prot);

				/*
				 * roundup virtual address to the next
				 * PSE_PAGESIZE boundary
				 */
				 vaddr += msize;
			} else {
				/*
				 * Copy the memory a page at a time, hopefully
				 * into non-DMAable pages.
				 */
				PHYS_PRINT("mapping type %d <vaddr, len> = "
					   "<%x, %x>\n",
					memlistp->pm_use,
					(long) vaddr, (long) msize);
				for (size = 0;
				     size < msize;
				     size += MMU_PAGESIZE) {
					paddr = phys_palloc(1);
					pcreate_map(vaddr, MMU_PAGESIZE,
						    paddr, prot);
					bcopy((void *) src, (void *) paddr,
					      MMU_PAGESIZE);
#ifndef CCNUMA
					if (memlistp->pm_use == PMEM_KDATA)
						premap(vaddr, paddr,
							MMU_PAGESIZE, prot);
#endif /* CCNUMA */
					src += MMU_PAGESIZE;
					vaddr += MMU_PAGESIZE;
				}
				if (memlistp->pm_use == PMEM_KTEXT)
					vaddr = roundup(vaddr, PSE_PAGESIZE);
			}
			break;

#ifdef CCNUMA
		/*
		 * PERF:
		 *	The CCNUMA kernel does not currently support a copy
		 *	of the kernel data. In the future we plan to stripe
		 *	the kernel data across the CGs.
		 */
		case PMEM_KDATA:
#endif /* CCNUMA */
		case PMEM_RESMGR:
		case PMEM_LICENSE:
		case PMEM_MEMFSROOT_META:
		case PMEM_MEMFSROOT_FS:
#ifdef PHYS_DEBUG
			if (memlistp->pm_use == PMEM_KDATA) {
				mapped_data = B_TRUE;
				PHYS_ASSERT(vaddr == (vaddr_t)sdata);
			}
			if (memlistp->pm_use == PMEM_RESMGR)
				mapped_resmgr = B_TRUE;
			if (memlistp->pm_use == PMEM_LICENSE)
				mapped_license = B_TRUE;
#endif /* PHYS_DEBUG */
			PHYS_PRINT("mapping type %d <vaddr, len> = <%x, %x>\n",
				(long) memlistp->pm_use,
				(long) vaddr, (long) msize);
			pcreate_map(vaddr, msize, src, prot);
			memlistp->pm_vaddr = vaddr;
			vaddr += msize;
			break;

		default:
			PHYS_PRINT("build_table: default use %d\n",
				   memlistp->pm_use);
			PHYS_ASSERT(memlistp->pm_use != PMEM_FREE);
			PHYS_ASSERT(memlistp->pm_use < PMEM_NONE);
			break;
		}
	}
	PHYS_ASSERT(mapped_text);
	PHYS_ASSERT(mapped_data);
	PHYS_ASSERT(mapped_resmgr);
	PHYS_ASSERT(mapped_license);	
	PHYS_ASSERT(mapped_ksym);
	PHYS_ASSERT(mapped_boot);
	PHYS_ASSERT(mapped_pstart);
	PHYS_ASSERT(mapped_bki);
	PHYS_ASSERT(mapped_reserved);

#ifdef CCNUMA
	if (this_cg_num != BOOTCG)
		return vaddr;
#endif /* CCNUMA */

	/*
	 * Map the set of per-engine pages for the current engine
	 * in a global kernel virtual address (so that it can
	 * be referenced by engine[].e_local).
	 *
	 * Remember the corresponding virtual address to later
	 * pass to sysinit().
	 */
	calloc_base = vaddr;
	msize = mmu_ptob(cgprivpages);
	pcreate_map(vaddr, msize, (paddr_t)cgpriv, PG_RW | PG_V);
	mycgpriv = (struct cgpriv_pages *)vaddr; 
	/* vaddr of my cgpriv_pages */
	vaddr += msize;

	/* Other misc. data */
	pmet = (struct mets *) phys_palloc(MET_PAGES * (PAGESIZE/MMU_PAGESIZE));
	pcreate_map(KVMET, (MET_PAGES * PAGESIZE), (paddr_t)pmet,
		    PG_RW | PG_V);
	paddr = phys_palloc(SYSDAT_PAGES * (PAGESIZE/MMU_PAGESIZE));
	pcreate_map(KVSYSDAT, (SYSDAT_PAGES * PAGESIZE), paddr, PG_RW | PG_V);

	return vaddr;
}

/*
 * STATIC void
 * build_table(void)
 *
 *	Build the initial page tables (non-PAE mode case) for the kernel.
 *
 * Calling/Exit State:
 *
 *	Called during early system initialization while running
 *	with the page table prepared by the boot laoder.
 *      vmemptr    - page-aligned virtual address of next unused
 *
 *                   virtual memory.
 *
 *      myengppriv - virtual address of this engine's struct ppriv_pages.
 *                   The pp_kl1pt and pp_pmap pages are already in use as
 *                   the current engine's level 1 and level 2 page tables.
 *
 *      mycgpriv   - virtual address of this CG's struct cgpriv_pages.
 */

STATIC void
build_table(void)
{
	uint_t pprivpages;
	struct ppriv_pages *pmyengppriv;    /* phys addr of my ppriv_pages */
	struct plocal *l;
	pte_t *ptr;
	pte_t *kl1pt = (pte_t *)0;	/* kernel level 1 page table */
	vaddr_t vaddr;
	uint_t msize;

	PHYS_PRINT("build_table: enter\n");

	/*
	 * The per-engine pages must fit in a single level 2 page table.
	 */
	kvper_eng = KVPER_ENG_STATIC;
	PHYS_ASSERT(KVLAST_PLAT >= kvper_eng);

	/*
	 * Allocate the set of per-engine pages for the current engine.
	 * The allocation for the rest of the engines happens later
	 * in sysinit().
	 */
	pprivpages = mmu_btopr(sizeof(struct ppriv_pages));
	pmyengppriv = (struct ppriv_pages *) phys_palloc(pprivpages);

	/*
	 * Set up temporary global pointers to the level 1 & 2 page tables
	 * allocated for this engine in struct ppriv_pages.
	 * (Used only by routines in this file.)
	 *
	 * Then fill in the level 1 pointers to the level 2 page tables for:
	 *	- the per-engine pages (4 Meg)
	 *	- the KL2PTES array of global kernel level 2 ptes
	 *
	 * This gets us to the point where we can call pcreate_map().
	 */

	pkl1pt = &pmyengppriv->pp_kl1pt[0][0];
	/* Get physical address of the per-engine L2 page table page */
	ptr = &pmyengppriv->pp_pmap[0][0];
	/* Map per-engine L2 page table page */
	pkl1pt[ptnum(KVPER_ENG_STATIC)].pg_pte =
				mkpte(PG_US | PG_RW | PG_V, pfnum(ptr));

	l = (struct plocal *)&pmyengppriv->pp_local[0][0];
	kvper_eng -= ptbltob(1);
	l->kvpte[0] = kvper_eng;
        PHYS_PRINT("l->kvpte[0] = 0x%x\n", kvper_eng);

	/*
	 * The non-ccNUMA kernel needs no KL2PTEs
	 */
	kl2ptes = kvper_eng;

	/*
	 * Reference back to L1 pte.
	 */
	/* Assert KVPTE on page table boundary */
	PHYS_ASSERT(pgndx(l->kvpte[0]) == 0);
	pkl1pt[ptnum(l->kvpte[0])].pg_pte =
				mkpte(PG_RW | PG_V, pfnum(pkl1pt));

	/*
	 * fill in pcreate_map_table entries
	 */
	pcreate_map_table[0].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_local[0][0];
	pcreate_map_table[1].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_localmet[0][0];
	pcreate_map_table[2].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_pmap[0][0];
	pcreate_map_table[3].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_kl1pt[0][0];
	pcreate_map_table[4].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_uvwin[0][0];
	pcreate_map_table[5].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_uvwin[0][0];
	pcreate_map_table[6].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_ublock[0][0];
	pcreate_map_table[7].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_uengkse[0][0];

	vaddr = build_table_common();

	/*
	 * Map the set of per-engine pages for the current engine
	 * in a global kernel virtual address (so that it can
	 * be referenced by engine[].e_local).
	 *
	 * Remember the corresponding virtual address to later
	 * pass to sysinit().
	 */
	msize = mmu_ptob(pprivpages);
	pcreate_map(vaddr, msize, (paddr_t)pmyengppriv, PG_RW | PG_V);
	myengppriv = (struct ppriv_pages *)vaddr; 
	/* vaddr of my ppriv_pages */
	vaddr += msize;

	/*
	 * Start allocating virtual memory at the
	 * next available (page aligned) virtual address.
	 */
	vmemptr = roundup(vaddr, MMU_PAGESIZE);
}

#ifdef PAE_MODE
/*
 * STATIC void
 * pae_build_table(void)
 *
 *	Build the initial page tables (non-PAE mode case) for this CG.
 *
 * Calling/Exit State:
 *
 *	Returns with the kernel virtual address space setup.
 *	The following global variables have been set:
 *
 *	vmemptr	   - page-aligned virtual address of next unused
 *		     virtual memory.
 *
 *	myengppriv - virtual address of this engine's struct ppriv_pages.
 *		     The pp_kl1pt and pp_pmap pages are already in use as
 *		     the current engine's level 1 and level 2 page tables.
 *
 *	mycgpriv   - virtual address of this CG's struct cgpriv_pages.
 */
STATIC void
pae_build_table(void)
{
	uint_t pprivpages;
	struct ppriv_pages_pae *pmyengppriv; /* phys addr of my ppriv_pages */
	struct plocal *l;
	pte64_t *pdptep;
	pte64_t *ptr;
	pte64_t *kl1pt = (pte64_t *)0;	/* kernel level 1 page table */
	int i;
	uint_t disp;
	vaddr_t vaddr;
	uint_t msize;

	PHYS_PRINT("pae_build_table: enter\n");

	/*
	 * The per-engine pages must fit in a single level 2 page table.
	 */
	kvper_eng = KVPER_ENG_STATIC_PAE;
	PHYS_ASSERT(KVLAST_PLAT >= KVPER_ENG_STATIC);

	/*
	 * Allocate the set of per-engine pages for the current engine.
	 * The allocation for the rest of the engines happens later
	 * in sysinit().
	 */
	pprivpages = mmu_btopr(sizeof(struct ppriv_pages_pae));
	pmyengppriv = (struct ppriv_pages_pae *) phys_palloc(pprivpages);

	/*
	 * Set up temporary global pointers to the level 1 & 2 page tables
	 * allocated for this engine in struct ppriv_pages.
	 * (Used only by routines in this file.)
	 *
	 * Then fill in the level 1 pointers to the level 2 page tables for:
	 *	- the per-engine pages (4 Meg)
	 *	- the KL2PTES array of global kernel level 2 ptes
	 *
	 * This gets us to the point where we can call pcreate_map().
	 */
	pkpdpt = (pte_t *)&pmyengppriv->pp_local[0][0];
	pdptep = (pte64_t *)&pmyengppriv->pp_local[0][0];
	for (i = 0; i < PDPTSZ; i++, pdptep++) {
		/* PDPT should be below 4G */
		pdptep->pte32.pg_high = 0;
		pdptep->pte32.pg_low =
			((uint_t)&pmyengppriv->pp_kl1pt[i][0]) | PG_V;
	}

	l = (struct plocal *)&pmyengppriv->pp_local[0][0];
	pul1pt64 = &pmyengppriv->pp_kl1pt[0][0];
	pkl1pt64 = &pmyengppriv->pp_kl1pt[3][0];
	/* Get physical address of the per-engine L2 page table page */
	ptr = &pmyengppriv->pp_pmap[0][0];
	/* Map per-engine L2 page table page */
	pkl1pt64[pae_ptnum(KVPER_ENG_STATIC_PAE)].pg_pte =
			pae_mkpte(PG_US | PG_RW | PG_V, pae_pfnum(ptr));

	/*
	 * Reference back to L1 pte.
	 */
	for (i = 0; i < PDPTSZ; i++) {
		kl1pt = &pmyengppriv->pp_kl1pt[i][0];
		l->kvpte64[i] = kvper_eng - pae_ptbltob(1);
		/* Assert KVPTE on page table boundary */
		PHYS_ASSERT(pae_pgndx(l->kvpte64[i]) == 0);
		pkl1pt64[pae_ptnum(l->kvpte64[i])].pg_pte =
				pae_mkpte(PG_RW | PG_V, pae_pfnum(kl1pt));
		kvper_eng= kvper_eng - (pae_ptbltob(1));
	}

#ifdef CCNUMA
	/*
	 * Allocate the KL2PTEPTEs for this CGs.
	 */
	kl2ptes = kvper_eng - KL2PTES_SIZE(MAXNUMCG);
	pkl2ptes = (pte64_t *)phys_palloc(1);
	pkl1pt64[pae_ptnum(kl2ptes) + this_cg_num].pg_pte =
			pae_mkpte(PG_RW | PG_V, pae_pfnum(pkl2ptes));
	PHYS_PRINT("KL2PTES = 0x%x, KL2PTES_SIZE = 0x%x\n",
		   kl2ptes, KL2PTES_SIZE(MAXNUMCG));
	PHYS_PRINT("PTESPERCG=%x pte = 0x%x\n", PTESPERCG, pkl1pt64[pae_ptnum(kl2ptes) + this_cg_num].pte32.pg_low);
#else
	/*
	 * The non-ccNUMA kernel needs no KL2PTEs.
	 */
	kl2ptes = kvper_eng;
#endif /* CCNUMA */

	PHYS_PRINT("l=%x pkpdpt=%x PPRIVSZ_PAE=%x, PPRIVSZ=%x\n", 
			l, pkpdpt, SZPPRIV_PAE, SZPPRIV);
	PHYS_PRINT("l.pdpte=%x\n", l->pdpte[0].pte32.pg_low);
	PHYS_PRINT("l.pdpte=%x\n", l->pdpte[1].pte32.pg_low);
	PHYS_PRINT("l.pdpte=%x\n", l->pdpte[2].pte32.pg_low);
	PHYS_PRINT("l.pdpte=%x\n", l->pdpte[3].pte32.pg_low);
	PHYS_PRINT("pae_build_table: pte=%x\n", 
			pkl1pt64[pae_ptnum(l->kvpte64[3])].pte32.pg_low);
	PHYS_PRINT("pae_build_table: kvpte=%x\n", l->kvpte64[3]);
	PHYS_PRINT("pae_build_table: kvper_eng=%x\n", kvper_eng);
	PHYS_PRINT("pae_build_table: pkl1pt64=%x\n", pkl1pt64);

	/*
	 * fill in pcreate_map_table entries
	 */
	pcreate_map_table[0].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_local[0][0];
	pcreate_map_table[1].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_localmet[0][0];
	pcreate_map_table[2].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_pmap[0][0];
	pcreate_map_table[3].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_kl1pt[0][0];
	pcreate_map_table[3].pmt_size = PDPTSZ * MMU_PAGESIZE;
	pcreate_map_table[4].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_uvwin[0][0];
	pcreate_map_table[5].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_uvwin[0][0];
	pcreate_map_table[6].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_ublock[0][0];
	pcreate_map_table[7].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_uengkse[0][0];
#ifdef CCNUMA
	pcreate_map_table[13].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_localmet[0][0];
#endif /* CCNUMA */

	vaddr = build_table_common();

	/*
	 * Map the set of per-engine pages for the current engine
	 * in a global kernel virtual address (so that it can
	 * be referenced by engine[].e_local).
	 *
	 * Remember the corresponding virtual address to later
	 * pass to sysinit().
	 */

	if (this_cg_num == BOOTCG) {
		msize = mmu_ptob(pprivpages);
		pcreate_map(vaddr, msize, (paddr_t)pmyengppriv, PG_RW | PG_V);

		myengppriv_pae = (struct ppriv_pages_pae *)vaddr; /* vaddr of my ppriv_pages_pae */
		vaddr += msize;

		/*
		 * Start allocating virtual memory at the
		 * next available (page aligned) virtual address.
		 */
		vmemptr = roundup(vaddr, MMU_PAGESIZE);
	}

#ifdef PAE_DEBUG
	for (i = 0; i < PAE_NPGPT; i++) {
		if (pkl1pt64[i].pg_pte == 0)
			continue;
		PHYS_PRINT("pkl1pt64 (%x): pte32.low=%x pte32.high=%x\n", i,
			pkl1pt64[i].pte32.pg_low, pkl1pt64[i].pte32.pg_high);
	}
	kl1pt = pkl1pt64 + pae_ptnum(KVPER_ENG_STATIC_PAE);
	kl1pt = (pte64_t *)(kl1pt->pg_pte & MMU_PAGEMASK);
	for (i = 0; i < PAE_NPGPT; i++) {
		if (kl1pt[i].pg_pte == 0)
			continue;
		PHYS_PRINT("per-eng (%x): pte32.low=%x pte32.high=%x\n", i,
			kl1pt[i].pte32.pg_low, kl1pt[i].pte32.pg_high);
	}
#ifdef CCNUMA
	PHYS_PRINT("kl2ptes=%x\n", kl2ptes);
#endif /* CCNUMA */
#endif /* PAE_DEBUG */
}
#endif /* PAE_MODE */

/*
 * STATIC void
 * pcreate_map(vaddr_t vaddr, uint_t size, paddr_t paddr, int prot)
 *
 *	Create a page table for a given virtual address range.
 *	Allocate memory to hold the page table entries if needed.
 *
 * Calling/Exit State:
 *
 *	This run while executing under the boot loader page tables.
 *
 *	vaddr is the starting (page-aligned) virtual address to map.
 *	size is the size in bytes of the virtual range to map;
 *		it must be an integral number of pages.
 *	paddr is the starting (page-aligned) physical address to map
 *		at vaddr.  "size" bytes of physically contiguous
 *		memory starting at "paddr" is mapped.
 *	prot are the i386 hardware page protections to use for the mapping.
 *
 *	Pages are MMU_PAGESIZE bytes long.
 */

STATIC void
pcreate_map(vaddr_t vaddr, uint_t size, paddr_t paddr, int prot)
{
	pte_t	*ptr;
	vaddr_t	va;
	vaddr_t	ea;

	/* Round to MMU page boundaries */
	va = vaddr & ~(MMU_PAGESIZE - 1);
	ea = roundup(vaddr + size, MMU_PAGESIZE);

#ifdef PAE_MODE
	if (using_pae) {
		pae_pcreate_map(va, ea - va, paddr, prot);
		return;
	}
#endif /* PAE_MODE */

	/*
	 * Can we use a PSE mapping?
	 */
	if ((va & PSE_PAGEOFFSET) == 0 && ((size & PSE_PAGEOFFSET) == 0) &&
	    pse_supported()) {
		/*
		 * create PSE page mappings
		 */
		while (va != ea) {
			pkl1pt[ptnum(va)].pg_pte =
				pse_mkpte(prot, pfnum(paddr));
			va += PSE_PAGESIZE;
			paddr += PSE_PAGESIZE;
		}
		PHYS_PRINT("PSE mapping created\n");
		return;
	}

	while (va != ea) {
		ptr = pkl1pt + ptnum(va);
		if (ptr->pg_pte == 0) {
			paddr_t tmppa = phys_palloc(1);
			ptr->pg_pte = mkpte(PG_RW | PG_V | PG_US, pfnum(tmppa));
		}

		/*
		 * Fill out a level 2 page table entry.
		 */

		ptr = (pte_t *)(ptr->pg_pte & MMU_PAGEMASK);
		ptr += pgndx((uint_t)va);
		ptr->pg_pte = mkpte(prot, pfnum(paddr));

		va += MMU_PAGESIZE; 
		paddr += MMU_PAGESIZE;
	}
}

#ifdef PAE_MODE
/*
 * STATIC void
 * pae_pcreate_map(vaddr_t va, uint_t size, paddr_t paddr, int prot)
 *
 *	Create a page table for a given virtual address range.
 *	Allocate memory to hold the page table entries if needed.
 *
 * Calling/Exit State:
 *
 *	This run while executing under the boot loader page tables.
 *
 *	vaddr is the starting (page-aligned) virtual address to map.
 *	size is the size in bytes of the virtual range to map;
 *		it must be an integral number of pages.
 *	paddr is the starting (page-aligned) physical address to map
 *		at vaddr.  "size" bytes of physically contiguous
 *		memory starting at "paddr" is mapped.
 *	prot are the i386 hardware page protections to use for the mapping.
 *
 *	Pages are MMU_PAGESIZE bytes long.
 *
 * Note:
 *	paddr probably should not be above 4G.
 */
STATIC void
pae_pcreate_map(vaddr_t va, uint_t size, paddr_t paddr, int prot)
{
	vaddr_t ea = va + size;
	pte64_t	*ptr;

	PHYS_PRINT("pae_pcreate_map: va %x, size %x, paddr %x, prot %x\n",
		   va, size, paddr, prot);

	/*
	 * Can we use a PSE mapping?
	 *
	 * Note: we make the conservative assumption here that a processor
	 *	 could support PAE but not PSE.
	 */
	if ((va & PSE_PAGEOFFSET) == 0 && ((size & PSE_PAGEOFFSET) == 0) &&
	    pse_supported()) {
		/*
		 * create PSE page mappings
		 */
		while (va != ea) {
			pkl1pt64[pae_ptnum(va)].pg_pte =
				pse_pae_mkpte(prot, pae_pfnum(paddr));
			va += (PSE_PAGESIZE >> 1);
			paddr += (PSE_PAGESIZE >> 1);
		}
		PHYS_PRINT("PAE-PSE mapping created\n");
		return;
	}

	while (va != ea) {
		if (va < kvbase)
			ptr = pul1pt64 + pae_ptnum(va);
		else
			ptr = pkl1pt64 + pae_ptnum(va);
		if (ptr->pg_pte == 0) {
			paddr_t tmppa = phys_palloc(1);
			ptr->pg_pte = pae_mkpte(PG_RW | PG_V | PG_US, pae_pfnum(tmppa));
#ifdef CCNUMA
			if (KADDR(va)) {
				/*
				 * Level 2 page tables for kernel addrs
				 * (top 1 G) are themselves virtually
				 * mapped in KL2PTES[].
				 */
				PHYS_ASSERT(pkl2ptes[kl2ptesndx(va)].pg_pte ==
					0);

				pkl2ptes[kl2ptesndx(va)].pg_pte =
					pae_mkpte(PG_RW | PG_V, pae_pfnum(tmppa));
				PHYS_PRINT("va=%x, kl2ptesndx=%x\n", va, kl2ptesndx(va));
			}
#endif /* CCNUMA */
		}

		/*
		 * Fill out a level 2 page table entry.
		 */

		ptr = (pte64_t *)(ptr->pg_pte & MMU_PAGEMASK);
		ptr += pae_pgndx((uint_t)va);
		ptr->pg_pte = pae_mkpte(prot | PG_US, pae_pfnum(paddr));

		va += MMU_PAGESIZE; 
		paddr += MMU_PAGESIZE;
	}
}
#endif /* PAE_MODE */

/*
 * STATIC paddr64_t
 * phys_palloc(uint_t npages)
 *
 *	Return the physical address of ``npages'' of contiguous
 *	zeroed memory.
 *
 * Calling/Exit State:
 *
 *	Called during initialization before calloc starts operating.
 *
 *	Panics if the memory is not available.
 */

STATIC paddr64_t
phys_palloc(uint_t npages)
{
	paddr64_t ret_paddr;

	ret_paddr = phys_palloc3(npages, MMU_PAGESIZE, pmaplimit);
	if (ret_paddr == PALLOC_FAIL)
		phys_panic("phys_palloc: out of memory!");

	return ret_paddr;
}

#ifndef CCNUMA
/*
 * STATIC void
 * premap(vaddr_t va, paddr_t pa, size_t msize, int prot)
 *
 *	Remap the virtual range from <va, va + msize> to
 *	the physical memory <pa, pa + msize> with protections
 *	given by prot.
 *
 * Calling/Exit State:
 *
 *	Called while still executing under the boot loader page tables.
 *	These tables (non-PAE mode) are modified in order to perform
 *	the remap.
 */

STATIC void
premap(vaddr_t va, paddr_t pa, size_t msize, int prot)
{
	vaddr_t	dva;
	vaddr_t	ea;
	paddr_t dpa = pa;
	pte_t *bkl1ptep = (pte_t *) READ_PTROOT();
	pte_t *ptr;

	/* Round to MMU page boundaries */
	ea = roundup(va + msize, MMU_PAGESIZE);
	dva = va = va & ~(MMU_PAGESIZE - 1);

	/*
	 * ASSERT that are not changing the data space.
	 */
	PHYS_BCMP((uchar_t *) va, (uchar_t *)pa, ea - va);

	while (dva < ea) {
		ptr = (pte_t *)
			(bkl1ptep[ptnum(dva)].pg_pte & MMU_PAGEMASK);
		ptr += pgndx((uint_t)dva);
		ptr->pg_pte = mkpte(prot, pfnum(dpa));

		dva += MMU_PAGESIZE; 
		dpa += MMU_PAGESIZE;
	}

	/*
	 * Flush the TLB.
	 */
	WRITE_PTROOT((ulong_t)bkl1ptep);

	/*
	 * ASSERT that we did not change the data space.
	 */
	PHYS_PRINT("premap: WRITE_PTROOT complete\n");
	PHYS_BCMP((uchar_t *) va, (uchar_t *)pa, ea - va);
	PHYS_PRINT("premap: PHYS_BCMP complete\n");
}
#endif /* CCNUMA */
