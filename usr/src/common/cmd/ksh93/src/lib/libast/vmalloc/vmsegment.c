#ident	"@(#)ksh93:src/lib/libast/vmalloc/vmsegment.c	1.1"
#include	"vmhdr.h"

/*	Get the segment containing this address
**
**	Written by Kiem-Phong Vo, kpv@research.att.com, 02/07/95
*/

#if __STD_C
Void_t* vmsegment(Vmalloc_t* vm, Void_t* addr)
#else
Void_t* vmsegment(vm, addr)
Vmalloc_t*	vm;	/* region	*/
Void_t*		addr;	/* address	*/
#endif
{
	reg Seg_t*	seg;
	reg Vmdata_t*	vd = vm->data;

	if(!(vd->mode&VM_TRUST))
	{	if(ISLOCK(vd,0))
			return NIL(Void_t*);
		SETLOCK(vd,0);
	}

	for(seg = vd->seg; seg; seg = seg->next)
		if((uchar*)addr >= (uchar*)seg->addr &&
		   (uchar*)addr <  (uchar*)seg->baddr )
			break;

	CLRLOCK(vd,0);
	return seg ? (Void_t*)seg->addr : NIL(Void_t*);
}
