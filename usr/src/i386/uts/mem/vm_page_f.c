/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)kern-i386:mem/vm_page_f.c	1.10.4.2"

/*
 * VM - physical page management:  Architecture Family specific functions.
 */


#include <util/types.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/plocal.h>
#include <proc/cg.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <mem/vmparam.h>
#include <mem/hat.h>
#include <svc/systm.h>


/*
 * void
 * pagezero(page_t *pp, ulong_t off, ulong_t len)
 *
 *	Zero `len' bytes of physical page `pp' starting at byte offset
 *	`off' without changing the reference and modified bits of page.
 *
 * Calling/Exit State:
 *
 *	Must not be called from interrupt state.
 *
 * Description:
 *
 *	Uses a per-processor virtual address reserved for such quick
 *	operations.  Assumes not called from interrupt state so we
 *	don't need to worry about serializing with interrupts.
 *
 *	The function is not reentrant.
 */

void
pagezero(page_t *pp, ulong_t off, ulong_t len)
{
	ASSERT(pp != NULL);
	ASSERT(len != 0 && off + len <= PAGESIZE);

#if (PAGESIZE != MMU_PAGESIZE)
#error pagezero assumes PAGESIZE == MMU_PAGESIZE
#endif
	/*
	 * As the function is not reentrant, disable preemption.
	 */
	DISABLE_PRMPT();

	/*
	 * Fill in the temporary pte in the level 2 page table.
	 * Zero the page.
	 * Clear the pte and flush our TLB.
	 */

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		kvtol2ptep64(KVTMPPG1)->pg_pte = pae_mkpte(PG_RW | PG_V, page_pptonum(pp));

		TLBSflush1(KVTMPPG1);
		(void) bzero((void *)(KVTMPPG1 + off), len);
		kvtol2ptep64(KVTMPPG1)->pg_pte = 0;
		TLBSflush1(KVTMPPG1);
	} else
#endif /* PAE_MODE */
	{
		kvtol2ptep(KVTMPPG1)->pg_pte = mkpte(PG_RW | PG_V, page_pptonum(pp));
		TLBSflush1(KVTMPPG1);
		(void) bzero((void *)(KVTMPPG1 + off), len);
		kvtol2ptep(KVTMPPG1)->pg_pte = 0;
		TLBSflush1(KVTMPPG1);
	}
	ENABLE_PRMPT();
}


/*
 * void
 * ppcopy(page_t *frompp, page_t *topp)
 *
 *	Copy the data from the physical page represented by "frompp" to
 *	that represented by "topp".
 *
 * Calling/Exit State:
 *
 *	Must not be called from interrupt state.
 *
 * Description:
 *
 *	Uses per-processor virtual addresses reserved for such quick
 *	operations.  Assumes not called from interrupt state so we
 *	don't need to worry about serializing with interrupts.
 *
 *	The function is not reentrant.
 */

void
ppcopy(page_t *frompp, page_t *topp)
{
	DISABLE_PRMPT();
	ASSERT(!servicing_interrupt());
	ASSERT(frompp != NULL && topp != NULL);
	ASSERT_PP_IN_RANGE(frompp);
	ASSERT_PP_IN_RANGE(topp);

#if (PAGESIZE != MMU_PAGESIZE)
#error ppcopy assumes PAGESIZE == MMU_PAGESIZE
#endif
	/*
	 * Fill in the temporary ptes in the level 2 page table.
	 * Copy the page.
	 * Clear the ptes and flush our TLB.
	 */

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		kvtol2ptep64(KVTMPPG1)->pg_pte = pae_mkpte(PG_RW | PG_V, page_pptonum(frompp));
		kvtol2ptep64(KVTMPPG2)->pg_pte = pae_mkpte(PG_RW | PG_V, page_pptonum(topp));
		TLBSflush2(KVTMPPG1);

		bcopy((void *)KVTMPPG1, (void *)KVTMPPG2, MMU_PAGESIZE);

		kvtol2ptep64(KVTMPPG1)->pg_pte = 0;
		kvtol2ptep64(KVTMPPG2)->pg_pte = 0;
	} else
#endif /* PAE_MODE */
	{
		kvtol2ptep(KVTMPPG1)->pg_pte = mkpte(PG_RW | PG_V, page_pptonum(frompp));
		kvtol2ptep(KVTMPPG2)->pg_pte = mkpte(PG_RW | PG_V, page_pptonum(topp));
		TLBSflush2(KVTMPPG1);

		bcopy((void *)KVTMPPG1, (void *)KVTMPPG2, MMU_PAGESIZE);

		kvtol2ptep(KVTMPPG1)->pg_pte = 0;
		kvtol2ptep(KVTMPPG2)->pg_pte = 0;
	}
	TLBSflush2(KVTMPPG1);
	ENABLE_PRMPT();
}


/*
 * void
 * ppcopyrange(page_t *frompp, page_t *topp, uint_t off, uint_t len)
 *
 *	Copy the data from the physical page represented by "frompp" to
 *	that represented by "topp".
 *
 * Calling/Exit State:
 *
 *	Must not be called from interrupt state.
 *
 * Description:
 *
 *	Uses per-processor virtual addresses reserved for such quick
 *	operations.  Assumes not called from interrupt state so we
 *	don't need to worry about serializing with interrupts.
 *
 *	The function is not reentrant.
 */

void
ppcopyrange(page_t *frompp, page_t *topp, uint_t off, uint_t len)
{
	DISABLE_PRMPT();
	ASSERT(!servicing_interrupt());
	ASSERT(frompp != NULL && topp != NULL);
	ASSERT_PP_IN_RANGE(frompp);
	ASSERT_PP_IN_RANGE(topp);
	ASSERT(off < PAGESIZE);
	ASSERT(off + len <= PAGESIZE);

#if (PAGESIZE != MMU_PAGESIZE)
#error ppcopy assumes PAGESIZE == MMU_PAGESIZE
#endif
	/*
	 * Fill in the temporary ptes in the level 2 page table.
	 * Copy the page.
	 * Clear the ptes and flush our TLB.
	 */

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		kvtol2ptep64(KVTMPPG1)->pg_pte = pae_mkpte(PG_RW | PG_V, page_pptonum(frompp));
		kvtol2ptep64(KVTMPPG2)->pg_pte = pae_mkpte(PG_RW | PG_V, page_pptonum(topp));
		TLBSflush2(KVTMPPG1);

		bcopy((void *)(KVTMPPG1 + off), (void *)(KVTMPPG2 + off), len);

		kvtol2ptep64(KVTMPPG1)->pg_pte = 0;
		kvtol2ptep64(KVTMPPG2)->pg_pte = 0;
	} else
#endif /* PAE_MODE */
	{
		kvtol2ptep(KVTMPPG1)->pg_pte = mkpte(PG_RW | PG_V, page_pptonum(frompp));
		kvtol2ptep(KVTMPPG2)->pg_pte = mkpte(PG_RW | PG_V, page_pptonum(topp));
		TLBSflush2(KVTMPPG1);

		bcopy((void *)(KVTMPPG1 + off), (void *)(KVTMPPG2 + off), len);

		kvtol2ptep(KVTMPPG1)->pg_pte = 0;
		kvtol2ptep(KVTMPPG2)->pg_pte = 0;
	}
	TLBSflush2(KVTMPPG1);
	ENABLE_PRMPT();
}

/*
 * void
 * pzero(ppid_t ppid)
 *
 *	Zero physical page specified by ppid.
 *
 * Calling/Exit State:
 *
 *	Must not be called from interrupt state.
 *
 * Description:
 *
 *	Uses a per-processor virtual address reserved for such quick
 *	operations.  Assumes not called from interrupt state so we
 *	don't need to worry about serializing with interrupts.
 *
 *	The function is not reentrant.
 */

void
pzero(ppid_t ppid)
{
	ASSERT(ppid != NOPAGE);

#if (PAGESIZE != MMU_PAGESIZE)
#error pzero assumes PAGESIZE == MMU_PAGESIZE
#endif
	/*
	 * As the function is not reentrant, disable preemption.
	 */
	DISABLE_PRMPT();

	/*
	 * Fill in the temporary pte in the level 2 page table.
	 * Zero the page.
	 * Clear the pte and flush our TLB.
	 */

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		kvtol2ptep64(KVTMPPG1)->pg_pte = pae_mkpte(PG_RW | PG_V, ppid);
		TLBSflush1(KVTMPPG1);
		(void) bzero((void *)KVTMPPG1, MMU_PAGESIZE);
		kvtol2ptep64(KVTMPPG1)->pg_pte = 0;
	} else
#endif /* PAE_MODE */
	{
		kvtol2ptep(KVTMPPG1)->pg_pte = mkpte(PG_RW | PG_V, ppid);
		TLBSflush1(KVTMPPG1);
		(void) bzero((void *)KVTMPPG1, MMU_PAGESIZE);
		kvtol2ptep(KVTMPPG1)->pg_pte = 0;
	}
	TLBSflush1(KVTMPPG1);
	ENABLE_PRMPT();
}

/*
 * void
 * pcopy(ppid_t from_ppid, ppid_t to_ppid)
 *
 *	Copy data from one physical page to another.
 *
 * Calling/Exit State:
 *
 *	Must not be called from interrupt state.
 *
 * Description:
 *
 *	Uses per-processor virtual addresses reserved for such quick
 *	operations.  Assumes not called from interrupt state so we
 *	don't need to worry about serializing with interrupts.
 *
 *	The function is not reentrant.
 */

void
pcopy(ppid_t from_ppid, ppid_t to_ppid)
{
	ASSERT((from_ppid != NOPAGE) && (to_ppid != NOPAGE));

#if (PAGESIZE != MMU_PAGESIZE)
#error pcopy assumes PAGESIZE == MMU_PAGESIZE
#endif
	/*
	 * As the function is not reentrant, disable preemption.
	 */
	DISABLE_PRMPT();

	/*
	 * Fill in the temporary ptes in the level 2 page table.
	 * Copy the page.
	 * Clear the ptes and flush our TLB.
	 */

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		kvtol2ptep64(KVTMPPG1)->pg_pte = pae_mkpte(PG_RW | PG_V, from_ppid);
		kvtol2ptep64(KVTMPPG2)->pg_pte = pae_mkpte(PG_RW | PG_V, to_ppid);
		TLBSflush2(KVTMPPG1);

		bcopy((void *)KVTMPPG1, (void *)KVTMPPG2, MMU_PAGESIZE);

		kvtol2ptep64(KVTMPPG1)->pg_pte = 0;
		kvtol2ptep64(KVTMPPG2)->pg_pte = 0;
	} else
#endif /* PAE_MODE */
	{
		kvtol2ptep(KVTMPPG1)->pg_pte = mkpte(PG_RW | PG_V, from_ppid);
		kvtol2ptep(KVTMPPG2)->pg_pte = mkpte(PG_RW | PG_V, to_ppid);
		TLBSflush2(KVTMPPG1);

		bcopy((void *)KVTMPPG1, (void *)KVTMPPG2, MMU_PAGESIZE);

		kvtol2ptep(KVTMPPG1)->pg_pte = 0;
		kvtol2ptep(KVTMPPG2)->pg_pte = 0;
	}
	TLBSflush2(KVTMPPG1);
	ENABLE_PRMPT();
}
