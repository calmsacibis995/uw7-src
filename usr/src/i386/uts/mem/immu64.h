#ifndef _MEM_IMMU64_H	/* wrapper symbol for kernel use */
#define _MEM_IMMU64_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/immu64.h	1.1.2.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Pentium Pro on-chip MMU in Physical Address Extension (PAE) mode.
 *
 *  In PAE mode the logical page size is 4MB instead of the
 *  minimum 2MB supported by the processor. See immu.h for definitions.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Page Table Entry Definitions
 */

typedef union pte64 {    /*  page table entry  */

/*                                                   */
/*                        S/W                        */
/*                       /   \                       */
/*  +-------------------+--+--+-+-+-+-+--+--+-+-+-+  */
/*  |        pfn        |ch|lk|g|s|m|r|cd|wt|u|w|p|  */
/*  +-------------------+--+--+-+-+-+-+--+--+-+-+-+  */
/*            20          2  1 1 1 1 1  1  1 1 1 1   */
/*                                                   */
/*                                                   */
/*  +---------------------------------------------+  */
/*  |        resevered                      | pfn |  */
/*  +---------------------------------------+-----+  */
/*            28                                4    */
/*                                                   */

	struct {
		ullong_t pg_v	:  1,	/* Page is valid; i.e. present	*/
		       pg_rw	:  1,	/* Read/write			*/
		       pg_us	:  1,	/* User/supervisor		*/
		       pg_wt	:  1,	/* Write-through cache		*/
		       pg_cd	:  1,	/* Cache disable		*/
		       pg_ref	:  1,	/* Page has been referenced	*/
		       pg_mod	:  1,	/* Page has been modified	*/
		       pg_ps	:  1,	/* Page size (L1 PTEs only)	*/
		       pg_g	:  1,	/* Page global bit		*/
		       pg_lock	:  1,	/* Translation is locked	*/
		       pg_wasref : 1,	/* Reference hint		*/
		       pg_avail	:  1,	/* Software bit (not used)	*/
		       pg_pfn	: 24,	/* Physical page frame number	*/
		       pg_resv  : 28;   /* Reserved 36-63 bits          */
        } pgm;

	/*
	 * Page Directory Pointer Table Entry.
	 */
	struct {
		ullong_t pg_v   :  1,   /* Page is valid; i.e. present  */
		       pg_res2  :  2,   /* Reserved 1-2 bits            */
		       pg_wt    :  1,   /* Write-through cache          */
		       pg_cd    :  1,   /* Cache disable                */
		       pg_res4  :  4,   /* Reserved 5-8 bits            */
		       pg_sw    :  3,   /* Software bits                */
		       pg_pfn   : 24,   /* Page Directory Base Address  */
		       pg_resv  : 28;   /* Reserved 36-63 bits          */
	} pdpte;

	struct {
		uint_t  pg_low;         /* Lower 32-bit page table entry */
		uint_t  pg_high;        /* Upper 32-big page table entry */
	} pte32;

	/*
	 * Full page table entry.
	 */
	ullong_t	pg_pte;		/* 64-bit page table entry       */

} pte64_t;

/*
 * Per-prorcessor cache of L2 ptes to be unloaded/aged.
 */
typedef struct pte64_array {
        void *pte_ptpp;
        uint_t pte_pndx;
        ullong_t pte_val;
} pte64_array_t;

/*
 * Page Table
 */

#define PAE_NPGPT	512		/* Nbr of pages per page table (seg). */

typedef struct pae_ptbl {
	int page[PAE_NPGPT * sizeof(pte64_t) / sizeof(ullong_t)];
} pae_ptbl_t;


#define PAE_PNUMMASK	0x1FF		/* Mask for index in page table */
#define PAE_PTNUMMASK	0x1FF		/* Mask for index in page directory */
#define PAE_PTOFFMASK	0x1FF		/* Mask for offset into page table/dir*/
#define PAE_PNDXMASK	PAE_PTOFFMASK	/* Mask for offset into kptbl.*/
#define PAE_PGFNMASK	0xFFFFFF	/* Mask page frame nbr after shift. */
#define PAE_PFNDXMASK	0x3FFFF		/* Mask page frame nbr after shift. */
#define PAE_PTNUMSHFT	21		/* Shift for page table num from addr */
#define PAE_VPTSIZE	(1 << PAE_PTNUMSHFT) /* Virtual bytes described by */
					/* a page table.		*/
#define PAE_VPTOFFSET	(PAE_VPTSIZE - 1) /* byte offset of addr in page table */
#define PAE_VPTMASK	(~PAE_VPTOFFSET)

#ifdef NOTYET
/* Page table entry field masks */

#define PT_ADDR		0xFFC00000	/* physical page table address */
#define PG_ADDR		0xFFFFF000	/* physical page address */
#endif /* NOTYET */

/* The page number within a section. */

#define pae_ptnum(X)	(((ulong_t)(X) >> PAE_PTNUMSHFT) & PAE_PTNUMMASK)
#define pae_pgndx(x)	(((ulong_t)(x) >> PNUMSHFT) & PAE_PNDXMASK)

#define pae_pnum(X)  	(((vaddr_t)(X) >> PNUMSHFT) & PAE_PTOFFMASK) 
#define pae_pfnum(X)	(((ullong_t)(X) >> PNUMSHFT) & PAE_PGFNMASK)
#define pae_pfndx(X)	(((vaddr_t)(X) >> PNUMSHFT) & PAE_PFNDXMASK)


/*
 * Various macros used to convert to and from page table boundaries.
 * btoptblr stands for bytes to page table roundup.
 */
#define pae_btoptblr(x)	pae_ptnum((ulong_t)(x) + (PAE_VPTSIZE-1))
#define pae_btoptbl(x)	pae_ptnum((ulong_t)(x))
#define pae_ptbltob(x)	((ulong_t)(x) << PAE_PTNUMSHFT)

/*
 * Form 64-bit page table entry from modes and page frame number.
 */
#define pae_mkpte(mode,pfn)	(mode | ((ullong_t)(pfn) << PNUMSHFT))
#define PAE_PG_PS           0x00000080      /* page size bit (1=large page) */
#define	pse_pae_mkpte(mode,pfn)	(pae_mkpte(mode, pfn) | PAE_PG_PS)

/*
 * asm void
 * pae_setpte(pte64_t *src, pte64_t *dst
 *	Load the 64-bit page table entry. The high word is set first
 *	and then the low word. This is necessary to avoid making any 
 *	assumptions about store orders of two ulongs.
 *
 * Calling/Exit State:
 *	None.
 */
asm void
pae_setpte(pte64_t *src, pte64_t *dst)
{
%reg src, dst;
	movl	4(src), %eax
	movl	(src), %ecx
	movl	%eax, 4(dst)
	movl	%ecx, (dst)

%reg src; mem dst;
	movl	dst, %ecx
	movl	4(src), %edx 
	movl	%edx, 4(%ecx)
	movl	(src), %edx 
	movl	%edx, (%ecx)

%mem src; reg dst;
	movl	src, %ecx
	movl	4(%ecx), %edx
	movl	%edx, 4(dst)
	movl	(%ecx), %edx
	movl	%edx, (dst)

%mem src, dst;
	movl	src, %eax
	movl	dst, %ecx
	movl	4(%eax), %edx
	movl	%edx, 4(%ecx)
	movl	(%eax), %edx
	movl	%edx, (%ecx)
}
#pragma asm partial_optimization pae_setpte

/*
 * asm void
 * pae_clrpte(pte64_t *pte)
 *	Zero the 64-bit page table entry. The low word is zeroed first
 *	and then the high word. This is necessary to avoid making any 
 *	assumptions about store orders of two ulongs.
 *
 * Calling/Exit State:
 *	None.
 */
asm void
pae_clrpte(pte64_t *pte)
{
%reg pte;
	movl	$0, (pte)
	movl	$0, 4(pte)

%mem pte;
	movl	pte, %eax
	movl	$0, (%eax)
	movl	$0, 4(%eax)
}
#pragma asm partial_optimization pae_clrpte

#ifdef NOTYET

#define SOFFMASK	0x3FFFFF	/* Mask for page table alignment */
#define SGENDMASK	0x3FFFFC	/* Mask for page table end alignment */

#define PAGNUM(x)   (((vaddr_t)(x) >> PNUMSHFT) & PNUMMASK)
#define PAGOFF(x)   (((vaddr_t)(x)) & POFFMASK)

#endif /* NOTYET */


#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_IMMU64_H */
