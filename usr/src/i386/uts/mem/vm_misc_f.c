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

#ident	"@(#)kern-i386:mem/vm_misc_f.c	1.32.6.3"
#ident	"$Header$"

/*
 * UNIX machine dependent virtual memory support.
 */

#include <mem/as.h>
#include <mem/hat.h>
#include <mem/immu.h>
#include <mem/immu64.h>
#include <mem/lock.h>
#include <mem/mem_hier.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/vmparam.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <svc/systm.h>
#include <svc/trap.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <proc/cg.h>

#define SEG_OVERLAP(seg, saddr, eaddr)	\
	((seg)->s_base < (eaddr) && ((seg)->s_base + (seg)->s_size > (saddr)))

/*
 * int
 * valid_va_range(vaddr_t *basep, u_int *lenp, u_int minlen, int dir)
 * 	Determine whether [base, base+len] contains a mapable range of
 * 	addresses at least minlen long. 
 *
 * Calling/Exit State:
 * 	No special requiments for MP.
 *
 *	If the specifed range is mapable, non-zero is returned; otherwise
 *	0 is returned.
 *
 * Remarks:
 *	On some architectures base and len may be adjusted if
 * 	required to provide a mapable range. That is why they are
 *	passed as pointers. The dir argument is used to specify the 
 *	relationship of base and len (i.e. whether len is added to or 
 *	subtracted from base).
 */
/* ARGSUSED */
int
valid_va_range(vaddr_t *basep, u_int *lenp, u_int minlen, int dir)
{
	vaddr_t hi, lo;

	lo = *basep;
	hi = lo + *lenp;
	if (hi < lo ) 		/* overflow */
		return(0);
	if (hi - lo < minlen)
		return (0);
	return (1);
}

/*
 * void
 * map_addr(vaddr_t *addrp, uint_t len, off64_t off, int align)
 * 	map_addr() is called when the system is to choose a
 * 	virtual address for the user.  We pick an address
 * 	range which is just below uvend.
 *
 * Calling/Exit State:
 *	- the target address space should be writer locked by the caller.
 *	- this function returns with the lock state unchanged.
 *
 * Description:
 * 	addrp is a value/result parameter. On input it is a hint from the user
 * 	to be used in a completely machine dependent fashion. We decide to
 * 	completely ignore this hint.  On output it is NULL if no address can
 *	be found in the current processes address space or else an address
 *	that is currently not mapped for len bytes with a page of red zone on
 *	either side.
 * 	If align is true, then the selected address will obey the alignment
 *	constraints of a vac machine based on the given off value. On the
 *	machines without a virtual address cache, this arg is ignored.
 */
/*ARGSUSED*/
void
map_addr(vaddr_t *addrp, uint_t len, off64_t off, int align)
{
	proc_t *p = u.u_procp;
	struct as *as = p->p_as;
	vaddr_t  base;
	uint_t   slen;

	len = (len + PAGEOFFSET) & PAGEMASK;

	/*
	 * Redzone for each side of the request. This is done to leave
	 * one page unmapped between segments. This is not required, but
	 * it's useful for the user because if their program strays across
	 * a segment boundary, it will catch a fault immediately making
	 * debugging a little easier.
	 */
	len += 2 * PAGESIZE;

	/*
	 * Look for a large enough hole in the address space to allocate
	 * dynamic memory.  First, look in the gap between stack and text,
	 * if any. If we find a hole there, use the lower portion. 
	 * If there is no gap, or there is not a large enough hole there,
	 * look above the break base.
	 */
	if (p->p_stkgapsize >= len) {
		base = p->p_stkbase;
		slen = p->p_stkgapsize;
		if (as_gap(as, len, &base, &slen, AH_LO, (vaddr_t)NULL) == 0) {
			*addrp = base + PAGESIZE;
			return;
		}
	}

	base = p->p_brkbase;
	slen = uvend - base;

	if (as_gap(as, len, &base, &slen, AH_HI, (vaddr_t)NULL) == 0) {
		/*
		 * as_gap() returns the 'base' address of a hole of
		 * 'slen' bytes.  We want to use the top 'len' bytes
		 * of this hole, and set '*addrp' accordingly.
		 * The addition of PAGESIZE is to allow for the redzone
		 * page on the low end.
		 */
		ASSERT(slen >= len);
		*addrp = base + (slen - len) + PAGESIZE;
	} else
		*addrp = NULL;  /* no more virtual space available */
}

/*
 * void
 * _Compat_map_addr(vaddr_t *addrp, uint_t len, off32_t off, int align)
 *
 *	This is a compatibility interface for fski.1 filesystems,
 *	and should be used only by such filesystems.
 *
 *	It is not invoked directly by name; fski.1 filesystems call
 *	map_addr but the call is redirected here at link time.
 */
void
_Compat_map_addr(vaddr_t *addrp, uint_t len, off32_t off, int align)
{
	map_addr(addrp, len, off, align);
}

#ifdef PAE_MODE
vaddr_t
pae_execstk_addr(size_t size, uint_t *hatflagp)	
{
	struct seg *sseg, *seg;
	vaddr_t	redzone_addr, redzone_endaddr, addr, base;
	vaddr_t pt_round_down, pt_round_up;
	int err;
	boolean_t no_overlap = B_TRUE;
 	u_int len;

	/*
	 * We are looking for a hole of 'size' bytes with a free page
	 * both before and aft (so segment concatenation does not occur).
	 * We first look for a slot with its end aligned properly for the
	 * user stack and falling inside of page table(s) which aren't used
	 * for anything else, so that later, we can use the page table
	 * directly at the new location, instead of copying ptes.
	 * If we can't find one, then we drop the alignment and empty
	 * page table constraints. 
	 *
	 * Since our stack grows downwards, we must be careful not to use
	 * slots where spilling of the aft end (into the free page) doesn't
	 * look like stack growth.  This allows the copyarglist code to
	 * skip string length checks.
	 */
	ASSERT(UVBASE == 0);

	*hatflagp = 1;

	addr = (u.u_stkbase - size) & (PAE_VPTSIZE - 1);

	if ((sseg = seg = u.u_procp->p_as->a_segs) == NULL)
		return (vaddr_t)addr;

	if (addr == 0)
		addr = PAE_VPTSIZE;

	/* start addr offest by 1 guard page */
	redzone_addr = addr - PAGESIZE;
	redzone_endaddr = addr + size + PAGESIZE;

	while (redzone_endaddr < uvend) {
		pt_round_down = pae_btoptbl(redzone_addr) * PAE_VPTSIZE;
		pt_round_up = pae_btoptblr(redzone_endaddr) * PAE_VPTSIZE;
		do {
			/* 
			 * Check if a segment intersects with this
			 * page table.
			 */
			if (SEG_OVERLAP(seg, pt_round_down, pt_round_up)) {
				no_overlap = B_FALSE;
				break;
			}
		} while (seg = seg->s_next, seg != sseg);

		if (no_overlap)
			return (vaddr_t)addr;

		/* this page table is not empty */
		addr += PAE_VPTSIZE;
		redzone_addr += PAE_VPTSIZE;
		redzone_endaddr += PAE_VPTSIZE;
		no_overlap = B_TRUE;
		seg = sseg;
	}	/* while loop */
	/*
	 * At this point we have come to a stage, where we could not
	 * find a free page table in the address space.
	 */
	*hatflagp = 0;

	base = PAGESIZE;
	len = uvend - base;

	err = as_gap(u.u_procp->p_as, size + 2 * PAGESIZE, &base, &len, AH_HI,
		(vaddr_t)NULL);
	if (!err)
		return (vaddr_t)base + PAGESIZE;

	return (vaddr_t)0;
}
#endif /* PAE_MODE */

/*
 * vaddr_t
 * execstk_addr(size_t size, uint_t *hatflagp)	
 *	Find a hole size bytes in length in the current address space,
 *	map it in, and return its address to the caller. Attempt to
 *	meet a number of machine-dependent alignment criteria to allow
 *	hat_exec to optimize the final stack relocation. Indicate the
 * 	success of this attempt by setting the outarg hatflagp.
 *
 * Calling/Exit State:
 *	Called as part of the exec sequence after all but one LWP has been
 *	destroyed making this a single-threaded process. The caller passes
 *	in the size (in bytes) needed to contain the new stack and a pointer 
 *	to the hatflagp outarg.
 *
 *	On success, the location of the newly mapped portion of the address 
 *	space is returned. The outarg hatflagp is used to indicate to the 
 *	caller that various machine-specific aligment criteria were met when 
 *	placing the stack build area in the old address space such that the
 *	stack may be relocated in the new address space in the most efficient
 *	manner possible. Generally this involves placement on page-table
 *	boundaries which avoids the necessity of copying individual PTEs.
 *	See hat_exec for more details.
 *
 *	On failure, a NULL pointer is returned. This will only happen if the
 *	current address space so completely mapped that a sufficiently 
 *	large hole cannot be mapped.
 *
 * Remarks:
 *	The current error return semantics disallow the possibility of
 *	mapping the stack build area starting at virtual zero. This is
 *	only a problem on machines with stacks that grow from low to high  
 *	addresses.
 *
 *	The hole must be large enough to accomodate the request size plus
 *	two additional pages worth of "guard-band" which remain unmapped.
 *
 */	
vaddr_t
execstk_addr(size_t size, uint_t *hatflagp)	
{
	struct seg *sseg, *seg;
	vaddr_t	redzone_addr, redzone_endaddr, addr, base;
	vaddr_t pt_round_down, pt_round_up;
	int err;
	boolean_t no_overlap = B_TRUE;
 	u_int len;

#ifdef PAE_MODE
	if (PAE_ENABLED())
		return pae_execstk_addr(size, hatflagp);
#endif /* PAE_MODE */

	/*
	 * We are looking for a hole of 'size' bytes with a free page
	 * both before and aft (so segment concatenation does not occur).
	 * We first look for a slot with its end aligned properly for the
	 * user stack and falling inside of page table(s) which aren't used
	 * for anything else, so that later, we can use the page table
	 * directly at the new location, instead of copying ptes.
	 * If we can't find one, then we drop the alignment and empty
	 * page table constraints. 
	 *
	 * Since our stack grows downwards, we must be careful not to use
	 * slots where spilling of the aft end (into the free page) doesn't
	 * look like stack growth.  This allows the copyarglist code to
	 * skip string length checks.
	 */
	ASSERT(UVBASE == 0);

	*hatflagp = 1;

	addr = (u.u_stkbase - size) & (VPTSIZE - 1);

	if ((sseg = seg = u.u_procp->p_as->a_segs) == NULL)
		return (vaddr_t)addr;

	if (addr == 0)
		addr = VPTSIZE;

	/* start addr offest by 1 guard page */
	redzone_addr = addr - PAGESIZE;
	redzone_endaddr = addr + size + PAGESIZE;

	while (redzone_endaddr < uvend) {
		pt_round_down = btoptbl(redzone_addr) * VPTSIZE;
		pt_round_up = btoptblr(redzone_endaddr) * VPTSIZE;
		do {
			/* 
			 * Check if a segment intersects with this
			 * page table.
			 */
			if (SEG_OVERLAP(seg, pt_round_down, pt_round_up)) {
				no_overlap = B_FALSE;
				break;
			}
		} while (seg = seg->s_next, seg != sseg);

		if (no_overlap)
			return (vaddr_t)addr;

		/* this page table is not empty */
		addr += VPTSIZE;
		redzone_addr += VPTSIZE;
		redzone_endaddr += VPTSIZE;
		no_overlap = B_TRUE;
		seg = sseg;
	}	/* while loop */
	/*
	 * At this point we have come to a stage, where we could not
	 * find a free page table in the address space.
	 */
	*hatflagp = 0;

	base = PAGESIZE;
	len = uvend - base;

	err = as_gap(u.u_procp->p_as, size + 2 * PAGESIZE, &base, &len, AH_HI,
		(vaddr_t)NULL);
	if (!err)
		return (vaddr_t)base + PAGESIZE;

	return (vaddr_t)0;
}

/*
 * void
 * modify_code(vaddr_t oldfunc, vaddr_t newfunc)
 *	This function modifies kernel code on the fly. "oldfunc" will be
 * 	replaced by a jump to "newfunc".
 *
 * Calling/Exit State:
 *	At this point, we better make sure that "oldfunc" is not being
 *	executed!
 *
 * Description:
 * 	The code is modified by using the indirect jump instruction
 *	using the eax register to the new function. This jump instruction
 * 	is written over the existing text segment for "oldfunc".
 */
void
modify_code(vaddr_t oldfunc, vaddr_t newfunc)
{
	unsigned char instr[7];
	int i, num_pages;
	int cg;
	int num_bytes_chg = 7; /* number of bytes overwriting oldfunc */
	vaddr_t	modaddr;

#define      PFN(ptep)       ((ptep)->pgm.pg_pfn)
#define      PFN64(ptep)     ((ptep)->pgm.pg_pfn)
	

#ifdef DEBUG
	debug_printf("modify_code: oldfunc = %x newfunc = %x\n",oldfunc,newfunc);
#endif
	/*
	 * opcode for "movl immediate value to eax"
	 */
	instr[0] = 0xb8;

	*(ulong_t *)&instr[1] = newfunc;
	/*
	 * opcode for indirect jmp via eax
	 */
	instr[5] = 0xff;
	instr[6] = 0xe0;


	/*
	 * to deal with replicated text we need to 
	 * create a temporary mapping of the target
	 * virtual address for each cg, and write the
	 * modified code there.
	 * 
	 * we have to be careful not to use any functions
	 * that may use the overwritten code (eg any locks)
	 * within this loop.
	 */
	for(cg = 0; cg < Ncg; cg++) {
		int eng;

		if (!IsCGOnline(cg))
			continue;

		/* pick a processor on the cg */
		eng = cg_array[cg].cg_cpuid[0];

		for ( i = 0 ; i < num_bytes_chg ; i++ ) {
			modaddr = (vaddr_t)oldfunc + i ;
			if((i == 0) || ((modaddr & MMU_PAGEOFFSET) == 0)) {
#ifdef PAE_MODE
				if (PAE_ENABLED()) 
					kvtol2ptep64(KVTMPPG1)->pg_pte = 
						pae_mkpte(PG_RW|PG_V,
							pae_dkvtoppid(modaddr,eng));
				else
#endif /* PAE_MODE */
					kvtol2ptep(KVTMPPG1)->pg_pte = 
						mkpte(PG_RW|PG_V,
							dkvtoppid32(modaddr,eng));
					
				TLBSflush1(KVTMPPG1);
			}
			*((char*)(KVTMPPG1 + (modaddr&MMU_PAGEOFFSET))) = 
				instr[i];
		}
	}

#ifdef PAE_MODE
	if (PAE_ENABLED()) 
		kvtol2ptep64(KVTMPPG1)->pg_pte = 0;
	else
#endif /* PAE_MODE */
		kvtol2ptep(KVTMPPG1)->pg_pte = 0;

	TLBSflush1(KVTMPPG1);


}
