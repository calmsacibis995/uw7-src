/*
 *	@(#)ctMask.c	11.1	10/22/97	12:10:47
 *	@(#) ctMask.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1994-1996.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */

#ident "@(#) $Id$"

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "ctDefs.h"
#include "ctMacros.h"
#include "ctOnboard.h"

static void *tile_chunk = (void *)0;		/* XXX: move into scrPriv */
static unsigned long last_plane_mask = 0L;	/* XXX: move into scrPriv */

Bool
CT(MaskPatternInit)(pScreen)
ScreenPtr pScreen;
{
	int ii;
	unsigned long v, plane_mask;
	CT_PIXEL *p;
	unsigned long *pl;

	tile_chunk = (void *)0;

#if (CT_BITS_PER_PIXEL == 8)
	/*
	 * allocate a 64 pixel by 256 pixel array in off-screen memory.
	 *
	 * (Note: 64 = 8 * 8)
	 */
	tile_chunk = CT_MEM_ALLOC(pScreen,
					(8 * 8),
					256,
					(8 * 8) * (CT_BITS_PER_PIXEL/8));
	if (!tile_chunk)
		return (FALSE);
	p = CT_MEM_VADDR(tile_chunk);
	pl = (unsigned long *)p;
	for (plane_mask=0;plane_mask<=0xff;plane_mask++) {
		v =	(plane_mask << 24) |
			(plane_mask << 16) |
			(plane_mask <<  8) |
			(plane_mask <<  0);
		for (ii=0;ii<(8*8*(CT_BITS_PER_PIXEL/8));ii += sizeof(v)) {
			*pl++ = v;
		}
	}
#endif /* (CT_BITS_PER_PIXEL == 8) */
#if (CT_BITS_PER_PIXEL == 16)
	/*
	 * allocate a 8 pixel by 8 pixel by 1 array in off-screen memory.
	 */
	tile_chunk = CT_MEM_ALLOC(pScreen,
					8,
					8,
					8 * (CT_BITS_PER_PIXEL/8));
	if (!tile_chunk)
		return (FALSE);
	p = CT_MEM_VADDR(tile_chunk);
	plane_mask = 0xffff;
	for (ii=0;ii<(8*8);ii++) {
		*p++ = plane_mask;
	}
	last_plane_mask = plane_mask;
#endif /* (CT_BITS_PER_PIXEL == 16) */
#if (CT_BITS_PER_PIXEL == 24)
	/*
	 * No pattern registers in 24 bit mode.
	 */
	tile_chunk = (void *)0;
#endif /* (CT_BITS_PER_PIXEL == 24) */
	return (TRUE);
}

void
CT(MaskPatternClose)(pScreen)
ScreenPtr pScreen;
{
#if (CT_BITS_PER_PIXEL == 8) || (CT_BITS_PER_PIXEL == 16)
	if (!tile_chunk)
		return;
	CT_MEM_FREE(pScreen, tile_chunk);
	tile_chunk = (void *)0;
#endif /* (CT_BITS_PER_PIXEL == 8) || (CT_BITS_PER_PIXEL == 16) */
}

unsigned long
CT(MaskPatternOffset)(plane_mask)
unsigned long plane_mask;
{
	int nn;
	unsigned long offset;

	if (!tile_chunk)
		return (0L);

	offset = CT_MEM_OFFSET(tile_chunk, 0, 0);

#if (CT_BITS_PER_PIXEL == 8)
	/* use a pre-setup pattern */
	nn = plane_mask & 0xff;
#endif /* (CT_BITS_PER_PIXEL == 8) */
#if (CT_BITS_PER_PIXEL == 16)
	/* build a pattern on the fly */
	plane_mask &= 0xffff;
	if (plane_mask != last_plane_mask) {
		CT_PIXEL *p;
		int ii;

		p = CT_MEM_VADDR(tile_chunk);
		for (ii=0;ii<(8*8);ii++) {
			*p++ = plane_mask;
		}
		last_plane_mask = plane_mask;
	}
	nn = 0;
#endif /* (CT_BITS_PER_PIXEL == 16) */

	return (offset + (nn * (8 * 8 * (CT_BITS_PER_PIXEL/8))));
}
