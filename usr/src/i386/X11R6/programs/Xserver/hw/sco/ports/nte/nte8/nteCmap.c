/*
 *	@(#) nteCmap.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 *
 * Modification History
 *
 * S003, 27-Jun-93, hiramc
 *	ready for 864/964 implementations, from DoubleClick
 * S002, 20-Aug-93, staceyc
 * 	hardware cursor implementation - check to see if we are going to
 *	clobber the hardware cursor colormap entries
 * S001, 08-Jun-93, staceyc
 * 	implemented
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

#if NTE_BITS_PER_PIXEL == 8
#if !NTE_USE_BT_DAC
void
NTE(SetColor)(
	unsigned int cmap,
	unsigned int index,
	unsigned short r,
	unsigned short g,
	unsigned short b,
	ScreenPtr pScreen)
{
	int top;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	unsigned int shift = ntePriv->dac_shift;
	CursorRec *pCursor = ntePriv->cursor;

	if (pCursor &&
	    (((index == ntePriv->cursor_fg) && (pCursor->foreRed != r ||
	    pCursor->foreGreen != g || pCursor->foreBlue != b)) ||
	    ((index == ntePriv->cursor_bg) && (pCursor->backRed != r ||
	    pCursor->backGreen != g || pCursor->backBlue != b))))
		NTE(ColorCursor)(pScreen);

	r >>= shift;
	g >>= shift;
	b >>= shift;

	if (ntePriv->depth == 8)
	{
		NTE_OUTB(NTE_PALWRITE_ADDR, index);
		NTE_OUTB(NTE_PALDATA, (unsigned char)r);
		NTE_OUTB(NTE_PALDATA, (unsigned char)g);
		NTE_OUTB(NTE_PALDATA, (unsigned char)b);
	}
	else
	{
		for (top = 0; top < 16; ++top)
		{
			NTE_OUTB(NTE_PALWRITE_ADDR, (top << 4) + index);
			NTE_OUTB(NTE_PALDATA, (unsigned char)r);
			NTE_OUTB(NTE_PALDATA, (unsigned char)g);
			NTE_OUTB(NTE_PALDATA, (unsigned char)b);
		}
	}

}
#endif /* !NTE_USE_BT_DAC */
#endif
