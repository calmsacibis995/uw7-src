/*
 *	@(#)ctCmap.c	11.1	10/22/97	12:33:46
 *	@(#) ctCmap.c 58.1 96/10/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 */
/* 
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */
/*
 * ctCmap.c
 *
 * Template for machine dependent colormap routines
 */

#ident "@(#) $Id: ctCmap.c 58.1 96/10/09 "

#include <sys/types.h>
#include "scrnintstr.h"
#include "ctDefs.h"
#include "ctMacros.h"

#if CT_BITS_PER_PIXEL == 8
/*
 * ctSetColor() - set an entry in a PseudoColor colormap
 *	cmap - machine dependent colormap number
 *	index - offset into colormap
 *	r,g,b - the rgb tripple to enter in this colormap entry
 *	pScreen - pointer to X's screen struct for this screen.  Simple
 *		implementations can initially ignore this.
 *
 * NOTE: r,g,b are 16 bits.  If you have a device that uses 8 bits you
 *	should use the MOST SIGNIFICANT 8 bits.
 */
void
CT(SetColor)(cmap, index, r, g, b, pScreen)
	unsigned int cmap;
	unsigned int index;
	unsigned short r;
	unsigned short g;
	unsigned short b;
	ScreenPtr pScreen;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pScreen);
	unsigned int shift = ctPriv->dacShift;
	unsigned char red, green, blue;

#ifdef DEBUG_PRINT
	ErrorF("SetColor(cmap=%d, index=%d, r=%d, g=%d, b=%d)\n",
		cmap, index, r, g, b);
#endif

	/*
	 * Use MOST SIGNIFICANT 8 bits.
	 */
	red = (unsigned char)((r >> shift) & 0x00ff);
	green = (unsigned char)((g >> shift) & 0x00ff);
	blue = (unsigned char)((b >> shift) & 0x00ff);

	CT_SET_COLOR(index, red, green, blue);
}
#endif /* CT_BITS_PER_PIXEL == 8 */
