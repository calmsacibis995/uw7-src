/*
 * @(#) m32Mem.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 27-Jul-93, buckm
 *	Created.
 * S001, 30-Aug-93, buckm
 *	Signal sw cursor differently.
 *	Configure off-screen info in screen privates.
 */
/*
 * m32Mem.c
 *
 * Mach-32 memory layout and configuration routines.
 */

#include "X.h"
#include "miscstruct.h"
#include "scrnintstr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"

/*
 * m32InitMem() - init memory layout
 */
Bool
m32InitMem(pM32, width, height, depth)
	m32ScrnPrivPtr pM32;
	int width, height, depth;
{
	m32CursInfoPtr pCI = &pM32->cursInfo;
	m32TE8InfoPtr  pTI = &pM32->te8Info;
	m32OSInfoPtr   pOS = &pM32->osInfo;
	int maxrow, maxoff;
	int row, off;
	int stride;
	int pbl2;		/* log2(_bytes_ per pixel) */

	/* disable memory partitioning */
	outb(M32_MEM_BNDRY, 0);

	switch (depth) {
	    case 8:	pbl2 = 0;  break;
	    case 16:	pbl2 = 1;  break;
#ifdef NOTYET
	    case 24:	pbl2 = ?;  break;
#endif
	}

	/* determine width of frame buffer */
	stride = ((width << pbl2) + 127) & ~127;
	pM32->pixBytesLog2 = pbl2;
	pM32->fbStride	   = stride;
	pM32->fbPitch	   = stride >> pbl2;

	/* determine usable height of frame buffer */
	maxoff = m32SizeMem();
	maxrow = (maxoff + stride - 1) / stride;
	if (maxrow > M32_MAX_DIMENSION) {
		maxrow = M32_MAX_DIMENSION;
		maxoff = maxrow * stride;
	}

	/*
	 * screen display is at beginning of frame buffer
	 * determine beginning of off-screen memory
	 */
	row = height;
	off = stride * height;

	/* allocate some linear stuff at end of frame buffer */

	/* cursor */
	if (! pM32->useSWCurs) {
		if ((maxoff - off) >= M32_SPRITE_BYTES) {
			maxoff -= M32_SPRITE_BYTES;
			pCI->offset = maxoff;
			pCI->addr.y = maxoff / stride;
			pCI->addr.x = (maxoff % stride) >> pbl2;
		} else {
			pM32->useSWCurs = 1;
		}
	}

	/* te8 fonts */
	if ((maxoff - off) >= M32_TE8_TOTAL_BYTES) {
		maxoff -= M32_TE8_TOTAL_BYTES;
		pTI->offset = maxoff;
		pTI->addr.y = maxoff / stride;
		pTI->addr.x = (maxoff % stride) >> pbl2;
	} else {
		pTI->offset = 0;
	}

	/* figure out what off-screen area we have left */

	maxrow = maxoff / stride;		/* last+1 whole row left */

	pOS->width  = pM32->fbPitch;
	pOS->height = maxrow - row;
	pOS->addr.x = 0;
	pOS->addr.y = row;

	/*
	 * if the area at the end of the frame buffer is small,
	 * check for a strip to the right of the screen.
	 * (800x600 16-bit w/1Meg RAM)
	 */
	if ((pOS->height < 32) && ((pM32->fbPitch - width) >= 32)) {
		pOS->width  = pM32->fbPitch - width;
		pOS->height = maxrow;
		pOS->addr.x = width;
		pOS->addr.y = 0;
	}

	return (off <= maxoff);
}

/*
 * m32SizeMem() - size the m32 frame buffer
 */
int
m32SizeMem()
{
	int sz, m;

	m = inw(M32_MISC_OPTIONS);

	m = (m >> 2) & 0x3;			/* mem size bits */

	sz = (512 * 1024) << m;			/* 512K to 4Meg */

	return sz;
}

/*
 * m32RestoreMem() - restore m32 off-screen memory
 *	we won't really save or restore anything;
 *	just mark it invalid.
 */
m32RestoreMem(pScr)
	ScreenPtr pScr;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScr);
}
