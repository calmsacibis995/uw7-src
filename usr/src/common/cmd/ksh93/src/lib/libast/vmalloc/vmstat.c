#ident	"@(#)ksh93:src/lib/libast/vmalloc/vmstat.c	1.1"
#include	"vmhdr.h"

/*	Get statistics from a region.
**
**	Written by (Kiem-)Phong Vo, kpv@research.att.com, 01/16/94.
*/

#if __STD_C
vmstat(Vmalloc_t* vm, Vmstat_t* st)
#else
vmstat(vm, st)
Vmalloc_t*	vm;
Vmstat_t*	st;
#endif
{
	reg Seg_t*	seg;
	reg Block_t	*b, *endb;
	reg size_t	s;
	reg Vmdata_t*	vd = vm->data;

	if(!st)
		return -1;
	if(!(vd->mode&VM_TRUST))
	{	if(ISLOCK(vd,0))
			return -1;
		SETLOCK(vd,0);
	}

	st->n_busy = st->n_free = 0;
	st->s_busy = st->s_free = st->m_busy = st->m_free = 0;
	st->n_seg = 0;
	st->extent = 0;

	if(vd->mode&VM_MTLAST)
		st->n_busy = vd->pool;
	else if((vd->mode&VM_MTPOOL) && (s = vd->pool) > 0)
	{	s = ROUND(s,ALIGN);
		for(b = vd->free; b; b = SEGLINK(b))
			st->n_free += 1;
	}

	for(seg = vd->seg; seg; seg = seg->next)
	{	st->n_seg += 1;
		st->extent += seg->extent;

		b = SEGBLOCK(seg);
		endb = BLOCK(seg->baddr);

		if(vd->mode&(VM_MTDEBUG|VM_MTBEST|VM_MTPROFILE))
		{	while(b < endb)
			{	s = SIZE(b)&~BITS;
				if(ISJUNK(SIZE(b)) || !ISBUSY(SIZE(b)))
				{	if(s > st->m_free)
						st->m_free = s;
					st->s_free += s;
					st->n_free += 1;
				}
				else	/* get the real size */
				{	if(vd->mode&VM_MTDEBUG)
						s = DBSIZE(DB2DEBUG(DATA(b)));
					else if(vd->mode&VM_MTPROFILE)
						s = PFSIZE(DATA(b));
					if(s > st->m_busy)
						st->m_busy = s;
					st->s_busy += s;
					st->n_busy += 1;
				}

				b = (Block_t*)((uchar*)DATA(b) + (SIZE(b)&~BITS) );
			}
		}
		else if(vd->mode&VM_MTLAST)
		{	s = seg->free ? (SIZE(seg->free) + sizeof(Head_t)) : 0;
			st->s_free += s;
			st->s_busy += ((char*)endb - (char*)b) - s;
		}
		else if((vd->mode&VM_MTPOOL) && s > 0)
			st->n_busy += ((char*)endb - (char*)b)/s;
	}

	if((vd->mode&VM_MTPOOL) && s > 0)
	{	st->n_busy -= st->n_free;
		if(st->n_busy > 0)
			st->s_busy = st->m_busy = vd->pool;
		if(st->n_free > 0)
			st->s_free = st->m_free = vd->pool;
	}

	CLRLOCK(vd,0);
	return 0;
}
