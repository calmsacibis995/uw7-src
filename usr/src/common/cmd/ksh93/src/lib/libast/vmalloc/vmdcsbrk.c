#ident	"@(#)ksh93:src/lib/libast/vmalloc/vmdcsbrk.c	1.1"
#include	"vmhdr.h"

/*	A discipline to get memory using sbrk().
**
**	Written by (Kiem-)Phong Vo, kpv@research.att.com, 01/16/94.
*/
#if __STD_C
static Void_t* sbrkmem(Vmalloc_t* vm, Void_t* caddr, size_t csize, size_t nsize,
			Vmdisc_t* disc)
#else
static Void_t* sbrkmem(vm, caddr, csize, nsize, disc)
Vmalloc_t*	vm;	/* region doing allocation from		*/
Void_t*		caddr;	/* current address			*/
size_t		csize;	/* current size				*/
size_t		nsize;	/* new size				*/
Vmdisc_t*	disc;	/* discipline structure			*/
#endif
{
	reg uchar*	addr;
	reg int		size;
	NOTUSED(vm);
	NOTUSED(disc);

	/* manipulating an existing segment, see if still own current address */
	if(csize > 0 && sbrk(0) != (uchar*)caddr+csize)
		return NIL(Void_t*);

	/* do this because sbrk() uses 'int' argument */
	if(nsize > csize)
		size =  (int)(nsize-csize);
	else	size = -(int)(csize-nsize);

	if((addr = sbrk(size)) == (uchar*)(-1))
		return NIL(Void_t*);
	else	return csize == 0 ? (Void_t*)addr : caddr;
}


Vmdisc_t _Vmdcsbrk = { sbrkmem, NIL(Vmexcept_f), 0 };
