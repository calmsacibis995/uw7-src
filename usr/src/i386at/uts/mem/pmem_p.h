#ifndef _MEM_PMEM_P_H	/* wrapper symbol for kernel use */
#define _MEM_PMEM_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:mem/pmem_p.h	1.5"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

/*
 * Types of memory.
 */
typedef enum {
	PMEM_FREE,		/* free memory */
	PMEM_FREE_IDF,		/* free dedicated memory */
	PMEM_BKI,		/* bki data */
	PMEM_KSYM,		/* kernel symbol table data */
	PMEM_KDEBUG,		/* used by the debugger */
	PMEM_KTEXT,		/* kernel text */
	PMEM_KDATA,		/* kernel datal, bss */
	PMEM_KERNEL,		/* KERNEL calloc, etc. */
	PMEM_RESMGR,		/* resmgr data */
	PMEM_LICENSE,		/* license data */
	PMEM_MEMFSROOT_META,	/* memfs meta data */
	PMEM_MEMFSROOT_FS,	/* memfs file data */
	PMEM_BOOT,		/* in use by the boot loader */
	PMEM_PSTART,		/* in use by the pstart code */
	PMEM_RESERVED,		/* reserved by the boot loader */
	/*
	 * Special value for internal use only.
	 */
	PMEM_NONE
} pmem_use_t;

#define ONE_KB			1024LL
#define ONE_MB			1048576LL
#define ONE_GB			1073741824LL
#define FOUR_GB			4294967296LL

/*
 * Kernel Physical Memory Limit (Currently Imposed by /dev/mem)
 */
#define	KERNEL_PHYS_LIMIT	FOUR_GB		/* 2**32 == 4GB */

/*
 * End of machine memory.
 */
#define PMEM_MACHINE_END	FOUR_GB		/* 2**36 == 64GB */

/*
 * IDF_SAFETY_NET - memory reserved for non-IDF use
 */
extern uint_t		idf_safety_net;
#define IDF_SAFETY_NET	idf_safety_net

#if defined(PHYS_DEBUG) || defined(lint)

#define str(s)  #s
#define xstr(s) str(s)

/*
 * Physical mode assert
 */
#define	PHYS_ASSERT(EX)	((void)((EX) || phys_assfail(#EX, __FILE__, \
				xstr(__LINE__))))

/*
 * early print
 */
#define PHYS_PRINT	phys_printf

extern void phys_printf(char *, ...);
extern int phys_assfail(const char *, const char *, const char *);

/*
 * Debugging functions.
 */
struct pmem_extent;
extern void pmem_physprint_list(struct pmem_extent *);
extern void pmem_physprint_lists(void);

#else	/* PHYS_DEBUG || lint */

/*
 * The following definitions have the effect of turning PHYS_ASSERTs
 * and PHYS_PRINTs into no-ops if PHYS_DEBUG is undefined.  PHYS_PRINT
 * in particular has a couple of tricks in it.  It consists of 
 * a conditional expression, which always evaluates to (void)0.
 * The third operand of the conditional expression will be the
 * argument list to PHYS_PRINT cast to a void type.  For instance,
 * consider what happens with the following when PHYS_DEBUG is
 * undefined and PHYS_PRINT is defined as below:
 *
 *	PHYS_PRINT("x = 0x%x\n", x);
 *
 * becomes
 *
 *	1 ? (void)0 : (void) ("x = 0x%x\n", x);
 *
 * which ends up evaluating to just
 *
 *		(void)0
 *
 * which is a no-op.
 */
#define	PHYS_ASSERT(EX) 	((void)0)
#define PHYS_PRINT		1 ? (void)0 : (void)

#endif  /* PHYS_DEBUG || lint */

/*
 * early mode panic
 */
extern void phys_panic(char *, ...);

/*
 * Divide up some free memory according to platform rules.
 */
extern void declare_mem_free(paddr64_t, paddr64_t, cgnum_t cgnum);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_PMEM_P_H */
