#ident	"@(#)stand:i386/boot/stage3/mmu.c	1.1"
#ident	"$Header$"

/*
 * MMU-specific paging support.
 */

#include <boot.h>
#include <stage3.h>

typedef struct {
	int	pg_v	:  1,	/* Page is valid; i.e. present	*/
		pg_rw	:  1,	/* Read/write			*/
		pg_us	:  1,	/* User/supervisor		*/
		pg_wt	:  1,	/* Write-through cache		*/
		pg_cd	:  1,	/* Cache disable		*/
		pg_ref	:  1,	/* Page has been referenced	*/
		pg_mod	:  1,	/* Page has been modified	*/
		pg_ps	:  1,	/* Page size (L1 PTEs only)	*/
		pg_g	:  1,	/* Page global bit		*/
		pg_sw	:  3,	/* S/W-defined bits		*/
		pg_pfn	: 20;	/* Physical page frame number	*/
} pte_t;

#define NBPP	4096		/* # bytes per MMU page */
#define NBPPT	4096		/* # bytes per page table */
#define NPGPT	(NBPPT / sizeof (pte_t))

int NL2PT = 8;			/* # L2 page tables to allocate */

pte_t *l1pt;			/* Level 1 page table (global for mmu_asm) */
STATIC pte_t *l2pt;		/* Level 2 page table free list */
STATIC pte_t *l2end;		/* End of free list */

#define BTOPFN(n)		((n) / NBPP)
#define VTOL1PTEP(v)		(l1pt + (v) / (NBPP * NPGPT))
#define VTOL2PTEP(v, l1ptep)	\
		((pte_t *)(l1ptep->pg_pfn * NBPP) + ((v) / NBPP) % NPGPT)

void
mmu_init(void)
{
	memseg_t *mp;
	ulong_t msize = 0, sz;

	/* Adjust # L2 page tables to account for mapping unused memory */
	for (mp = memsegs; mp != NULL; mp = mp->next) {
		if (mp->base >= pmaplimit)
			continue;
		sz = mp->nbytes;
		if (pmaplimit - mp->base < sz)
			sz = pmaplimit - mp->base;
		msize += sz;
	}
	NL2PT += msize / (NPGPT * NBPP);

	/*
	 * Allocate L1 and L2 page tables. Since we're not allowed to
	 * dynamically allocate them in mmu_map(), we have to make a
	 * conservative guess here, and if we run out we're SOL.
	 */
	l1pt = malloc3((NL2PT + 1) * NBPPT, NBPPT, boot_use);
	memzero(l1pt, (NL2PT + 1) * NBPPT);
	l2pt = (pte_t *)((char *)l1pt + NBPPT);
	l2end = (pte_t *)((char *)l2pt + NL2PT * NBPPT);
}

void
mmu_map(ulong_t vaddr, ulong_t paddr, ulong_t nbytes)
{
	pte_t *ptep;
	ulong_t endpfn = BTOPFN(vaddr + nbytes - 1);

#ifdef DEBUG2
printf("mmu_map(%u, %u, %u)", vaddr, paddr, nbytes);
#endif
	ASSERT(vaddr % NBPP == paddr % NBPP);

	for (;;) {
		ptep = VTOL1PTEP(vaddr);
		if (*(int *)ptep == 0) {
			if (l2pt == l2end)
				fatal("not enough page tables");
			ptep->pg_pfn = BTOPFN((ulong_t)l2pt);
			l2pt = (pte_t *)((char *)l2pt + NBPPT);
			ptep->pg_rw = 1;
			ptep->pg_v = 1;
#ifdef DEBUG3
printf("  l1 %u@%u", *(int *)ptep, ptep);
#endif
		}
		ptep = VTOL2PTEP(vaddr, ptep);
		ptep->pg_pfn = BTOPFN(paddr);
		ptep->pg_rw = 1;
		ptep->pg_v = 1;
#ifdef DEBUG3
printf("  l2 %u@%u\n", *(int *)ptep, ptep);
#endif
		if (BTOPFN(vaddr) == endpfn)
			break;
		vaddr += NBPP;
		paddr += NBPP;
	}
}
