#ifndef _MEM_IMMU_H	/* wrapper symbol for kernel use */
#define _MEM_IMMU_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/immu.h	1.21.6.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	80x86 on-chip MMU
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

typedef union pte {    /*  page table entry  */

/*                                                   */
/*                        S/W                        */
/*                       /   \                       */
/*  +-------------------+--+--+-+-+-+-+--+--+-+-+-+  */
/*  |        pfn        |ch|lk|g|s|m|r|cd|wt|u|w|p|  */
/*  +-------------------+--+--+-+-+-+-+--+--+-+-+-+  */
/*            20          2  1 1 1 1 1  1  1 1 1 1   */
/*                                                   */
	struct {
		uint_t pg_v	:  1,	/* Page is valid; i.e. present	*/
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
		       pg_chidx	:  1,	/* Cached pagepool chunk index	*/

		       pg_pfn	: 20;	/* Physical page frame number	*/
	} pgm;
	uint_t	pg_pte;		/* Full page table entry	*/
} pte_t;

typedef struct ptes_array {
	void *pte_ptpp;
	uint_t pte_pndx;
	uint_t pte_val;
} pte_array_t;

#define MAXL2_PTES	100

/*
 * Page Table
 */

#define NPGPT		1024	/* Nbr of pages per page table (seg). */

typedef struct ptbl {
	int page[NPGPT];
} ptbl_t;

/* Page table entry dependent constants */

#define NBPP		4096		/* Number of bytes per page */
#define NBPPT		4096		/* Number of bytes per page table */
#define BPTSHFT		12 		/* LOG2(NBPPT) if exact */
#define NPTPP		1		/* Nbr of page tables per page.	*/
#define NPTPPSHFT	0		/* Shift for NPTPP. */

/*
 * Following added because svid says ulimit works in 512 byte units. So we must
 * have something independent of the blocksize of the file system implementation
 */
#define NUPP		8		/* Number ulimit blocks per page    */
#define UPPSHFT		3		/* Shift for ulimit blocks per page */

#define PNUMSHFT	12		/* Shift for page number from addr. */
#define PNUMMASK	0x3FF		/* Mask for index in page table */
#define POFFMASK        0xFFF		/* Mask for offset into page. */
#define PTOFFMASK	0x3FF		/* Mask for offset into page table/dir*/
#define PNDXMASK	PTOFFMASK	/* Mask for offset into kptbl.*/
#define PGFNMASK	0xFFFFF		/* Mask page frame nbr after shift. */
#define PTNUMSHFT	22		/* Shift for page table num from addr */
#define PTSIZE		4096		/* Page table size in bytes */
#define PTMASK		(PTSIZE - 1)	/* Mask for page table size */
#define VPTSIZE		(1<<PTNUMSHFT)	/* Virtual bytes described by	*/
					/* a page table.		*/
#define VPTOFFSET	(VPTSIZE-1)	/* byte offset of addr in page table */
#define VPTMASK		(~VPTOFFSET)

/* Page table entry field masks */

#define PT_ADDR		0xFFC00000	/* physical page table address */
#define PG_ADDR		0xFFFFF000	/* physical page address */
#define PG_G		0x00000100	/* page global bit */
#define PG_PS		0x00000080	/* page size bit (1=large page) */
#define PG_M		0x00000040	/* modify bit */
#define PG_REF		0x00000020	/* reference bit */
#define PG_CD		0x00000010	/* cache disable */
#define PG_WT		0x00000008	/* write-through cache */
#define PG_US		0x00000004	/* 0=supervisor, 1=user */
#define PG_RW		0x00000002	/* 0=read-only, 1=read/write */
#define PG_V		0x00000001	/* page valid bit */
#define PG_P		PG_V		/* for source compatibility */
#define PTE_RW		(PG_RW|PG_US)
#define PTE_PROTMASK	PTE_RW

#define ORD_PG_V	0		/* ordinal number of PG_V bit */
#define ORD_PG_M	6		/* ordinal number of PG_M bit */
#define ORD_PG_REF	5		/* ordinal number of PG_REF bit */
#define ORD_PG_LOCK	9		/* ordinal number of PG_LOCK bit */
#define	ORD_PG_WASREF	10
#define	ORD_PG_CHIDX	11

/* Software PTE bits */

#define PG_LOCK		0x00000200	/* 0=not locked, 1=locked */
#define PG_WASREF	0x00000400	/* cached page reference */
#define PG_CHIDX	0x00000800	/* cached page chunk index (p_chidx) */
#define CHIDX_RES	2		/* resolution of PG_CHIDX (1 bits) */

/* The page number within a section. */

#define ptnum(X)	((ulong_t)(X) >> PTNUMSHFT)

#define pgndx(x)	(((ulong_t)(x) >> PNUMSHFT) & PNDXMASK)

/* Round down page table address */

#define pttrunc(p)	((int *) ((vaddr_t)(p) & ~(PTSIZE-1)))
#define ptalign(p)	pttrunc(p)

#define pnum(X)  	(((vaddr_t)(X) >> PNUMSHFT) & PTOFFMASK) 
#define pfnum(X)	(((vaddr_t)(X) >> PNUMSHFT) & PGFNMASK)

/* Following added because svid says ulimit works in 512 byte units, so we must
   have something independent of the blocksize of the file system implementation

	Ulimit blocks (512 bytes each) and pages.
 */

#define utop(UU)	(((UU) + NUPP -1) >> UPPSHFT)

/*
 * Various macros used to convert to and from page table boundaries.
 * btoptblr stands for bytes to page table roundup.
 */
#define btoptblr(x)	ptnum((ulong_t)(x) + (VPTSIZE-1))
#define btoptbl(x)	ptnum((ulong_t)(x))
#define ptbltob(x)	((ulong_t)(x) << PTNUMSHFT)

/* Form page table entry from modes and page frame number */

#define mkpte(mode, pfn)	(mode | ((uint_t)(pfn) << PNUMSHFT))

/*	The following macros are used to set/check the value
 *	of the bits in a page descriptor (table) entry 
 *
 *	Atomic instruction is available to clear the present bit,
 *	other bits are set or cleared in a word operation.
 */

#define PG_ISVALID(pte) 	((pte)->pgm.pg_v)
#define PG_SETVALID(pte)	((pte)->pg_pte |= PG_V)
#define PG_CLRVALID(pte)	((pte)->pg_pte &= ~PG_V)

#define PG_SETMOD(pte)   	((pte)->pg_pte |= PG_M)	
#define PG_CLRMOD(pte)   	((pte)->pg_pte &= ~PG_M)	

#define PG_SETREF(pte)    	((pte)->pg_pte |= PG_REF)
#define PG_CLRREF(pte)    	((pte)->pg_pte &= ~PG_REF)

#define PG_ISWRITEABLE(pte)	((pte)->pgm.pg_rw)
#define PG_ISVALID_WRITEABLE(pte) \
				(((pte)->pg_pte & (PG_V|PG_RW)) == (PG_V|PG_RW))
#define PG_CLRW(pte)		((pte)->pg_pte &= ~(PG_RW))

#define PG_SETPROT(pte,b)	((pte)->pg_pte |= b)	/* Set r/w access */
#define PG_CLRPROT(pte)		((pte)->pg_pte &= ~(PTE_PROTMASK))

#define SOFFMASK	0x3FFFFF	/* Mask for page table alignment */
#define SGENDMASK	0x3FFFFC	/* Mask for page table end alignment */

#define PAGNUM(x)   (((vaddr_t)(x) >> PNUMSHFT) & PNUMMASK)
#define PAGOFF(x)   (((vaddr_t)(x)) & POFFMASK)


/*
 *  4MB Page Size support
 *  (For processors with Page Size Extension feature.)
 */

#define P4OFFMASK       0x3FFFFF	/* Mask for offset into 4MB page. */
#define PAG4OFF(x)	(((vaddr_t)(x)) & P4OFFMASK)

#define PAGE4SIZE	0x400000
#define PAGE4SHIFT	22
#define PAGE4OFFSET	(PAGE4SIZE - 1)
#define PAGE4MASK	(~PAGE4OFFSET)
#define PAGE4DIFF	(PAGE4SHIFT - PAGESHIFT)

#define p4tob(x)	((x) << PAGE4SHIFT)
#define p4top(x)	((x) << PAGE4DIFF)
#define ptop4(x)	((x) >> PAGE4DIFF)
#define ptop4r(x)	(((x) + ((PAGE4SIZE / PAGESIZE) - 1)) >> PAGE4DIFF)
#define btop4(x)	(((vaddr_t)(x)) >> PAGE4SHIFT)
#define btop4r(x)	((((vaddr_t)(x) + PAGE4OFFSET) >> PAGE4SHIFT))

#define mk4pde(mode,x)	((mode) | PG_PS | ((uint_t)(x) << PAGE4SHIFT))

#define PG_IS4MB(pde) 	((pde)->pgm.pg_ps)

/*
 * Definitions for page size extension (PSE).  PSE is available on some
 *	Intel processors.  PSE allows entries in the page directory to
 *	map 4MB physical pages directly.
 *
 */
/*
 * Relate PSE_PAGESIZE to bytes
 *	PSE_PAGESIZE	size of a PSE page in bytes
 *	PSE_PAGESHIFT	log2(PSE_PAGESIZE)
 *	PSE_PAGEOFFSET	mask for obtaining offset into PSE page from address
 *	PSE_PAGEMASK	mask offset out of address
 */
#define	PSE_PAGESIZE	0x400000
#define	PSE_PAGESHIFT	22
#define	PSE_PAGEOFFSET	(PSE_PAGESIZE - 1)
#define	PSE_PAGEMASK	(~PSE_PAGEOFFSET)

#define PSE_PAGOFF(x)   (((vaddr_t)(x)) & PSE_PAGEOFFSET)

/*
 * Conversion macros:
 *	psetob:		pse pages to bytes
 *	btopse:		bytes to pse pages
 *	btopser:	bytes to pse pages, rounding up
 */
#define psetob(p)	((ulong_t)(p) << PSE_PAGESHIFT)
#define	btopse(b)	((ulong_t)(b) >> PSE_PAGESHIFT)
#define	btopser(b)	(((ulong_t)(b) + PSE_PAGEOFFSET) >> PSE_PAGESHIFT)

#define psetob64(p)	((ullong_t)(p) << PSE_PAGESHIFT)
#define btopse64(b)	((ullong_t)(b) >> PSE_PAGESHIFT)
#define btopser64(b)	(((ullong_t)(b) + PSE_PAGEOFFSET) >> PSE_PAGESHIFT)

/*
 * Relate size of PSE pages to standard pages
 *	PSE_NPAGES	size of a PSE page in pages
 *	PSE_NPGSHIFT	log2(PSE_NPAGES)
 *	PSE_NPGOFFSET	mask for obtaining offset from standard pfn
 *	PSE_NPGMASK	mask offset out of standard pfn
 */
#define	PSE_NPAGES	(PSE_PAGESIZE  / PAGESIZE)
#define	PSE_NPGSHIFT	(PSE_PAGESHIFT - PAGESHIFT)
#define	PSE_NPGOFFSET	(PSE_NPAGES - 1)
#define	PSE_NPGMASK	(~PSE_NPGOFFSET)

/*
 * Conversion macros:
 *	psetop:	pse pages to standard pages
 *	ptopse:	standard pages to pse pages
 *	ptopser:	standard pages to pse pages, rounding up
 */
#define	psetop(npse)	((ulong_t)(npse) << PSE_NPGSHIFT)
#define	ptopse(npgs)	((ulong_t)(npgs) >> PSE_NPGSHIFT)
#define	ptopser(npgs)	(((ulong_t)(npgs) + PSE_NPAGES - 1) \
					>> PSE_NPGSHIFT)

#define	pse_pfntopsefn(pfn)	((pfn) >> PSE_NPGSHIFT)
#define	pse_psefntopfn(psefn)	((psefn) << PSE_NPGSHIFT)
#define	pse_phystopfn(paddr)	(pse_pfntopsefn(phystopfn(paddr)))
#define	pse_pfntophys(psefn)	(pfntophys(pse_psefntopfn(psefn)))

#define pse_mkpte(mode,pfn)	(mkpte(mode, pfn) | PG_PS)

#define	PG_ISPSE(pde)		((pde)->pgm.pg_ps != 0)

/*
 *    Between physical address and physical page frame number.
 */

#define phystopfn(paddr)	((paddr_t)(paddr) >> PNUMSHFT)
#define pfntophys(pfn)  	((uint_t)(pfn) << PNUMSHFT)

#define phystopfn64(paddr)	((paddr64_t)(paddr) >> PNUMSHFT)
#define pfntophys64(pfn)  	((paddr64_t)(pfn) << PNUMSHFT)

/* Find (page_t *) from a PTE (mappings to pages only!) */
#define	pteptopp(ptep) page_numtopp_safe((ptep)->pgm.pg_pfn)

/* Requestor Privilege Level for selectors */

#define SEL_RPL		0x03

/*
 * Emable PAE mode only for non-UNIPROC kernels
 */
#ifndef UNIPROC
#define PAE_MODE	1
#endif

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_IMMU_H */
