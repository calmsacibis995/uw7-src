#ident	"@(#)ksh93:src/lib/libast/vmalloc/vmdisc.c	1.1"
#include	"vmhdr.h"

/*	Change the discipline for a region.  The old discipline
**	is returned.  If the new discipline is NIL then the
**	discipline is not changed.
**
**	Written by (Kiem-)Phong Vo, kpv@research.att.com, 01/16/94.
*/
#if __STD_C
Vmdisc_t* vmdisc(Vmalloc_t* vm, Vmdisc_t* disc)
#else
Vmdisc_t* vmdisc(vm, disc)
Vmalloc_t*	vm;
Vmdisc_t*	disc;
#endif
{
	Vmdisc_t*	old_disc = vm->disc;

	if(disc)
	{
		if(disc->memoryf != old_disc->memoryf ||
		   disc->exceptf && (*disc->exceptf)(vm,VM_DCCHANGE,(Void_t*)disc,old_disc) < 0)
			return NIL(Vmdisc_t*);
		vm->disc = disc;
	}
	return old_disc;
}
