#ident	"@(#)kern-i386:proc/resource_f.c	1.5.1.1"
#ident	"$Header$"

#include <mem/as.h>
#include <mem/seg_dz.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/types.h>
#include <svc/errno.h>

/*
 * int rlimit_stack(rlim_t curlimit, rlim_t newlimit)
 *	Change the AS layout to accomodate the new stack limits.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	At the exit of this function, the stack would be able to grow to
 *	to the new limit or would be shrunk by the new limit.
 *
 * Remarks:
 *	This function is processor dependent in choosing the range in
 *	which it grows (clips) the stack. In this architecture where the
 *	stack grows from high to low, the stack is grown (clipped)
 *	from the low end of the existing stack. On architectures where
 *	the stack grows low to high, the stack is grown (clipped) at the
 *	high end.
 */
int
rlimit_stack(rlim_t curlimit, rlim_t newlimit)
{
	proc_t *p = u.u_procp;
	struct as *as = p->p_as;
	struct segdz_crargs a;
	int err = 0;
	vaddr_t base;
	size_t size;
	vaddr_t lowend;

	ASSERT(getpl() == PLBASE);
	ASSERT(curlimit != newlimit);

	as_wrlock(as);

	if (newlimit > curlimit) {
		if (p->p_stkbase == UVSTACK) {
			ASSERT(p->p_stksize <= UVMAX_STKSIZE);
			if (p->p_stksize == min(UVMAX_STKSIZE, newlimit))
				goto bye;
			lowend = UVSTACK - min(UVMAX_STKSIZE, newlimit);
		} else {
			lowend = ptob(1);
		}
		base = p->p_stkbase - p->p_stksize;
		if (base == lowend)
			goto bye;
		a.flags = SEGDZ_CONCAT;
		a.prot = PROT_ALL;
		size = min(newlimit - curlimit, base - lowend);
		base -= size;
		err =  as_map(as, base, size, segdz_create, &a);
		if (!err) {
			LOCK(&p->p_mutex, PLHI);
			p->p_stksize += size;
			UNLOCK(&p->p_mutex, PLBASE);
		}
	} else {
		if (p->p_stksize <= newlimit)
			goto bye;
		base = p->p_stkbase - p->p_stksize;
		size = (p->p_stkbase - newlimit) - base;
		err = as_unmap(as, base, size);
		if (!err) {
			LOCK(&p->p_mutex, PLHI);
			p->p_stksize -= size;
			UNLOCK(&p->p_mutex, PLBASE);
		}
	}
bye:
	as_unlock(as);
	return err;
}
