/*
 * @(#) svgaCmap.c 11.1
 *
 * Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */
/*
 * svgaCmap.c
 *
 * Template for machine dependent colormap routines
 */

#include <sys/types.h>
#include "screenint.h"

/*
 * svgaSetColor() - set an entry in a PseudoColor colormap
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
svgaSetColor(cmap, index, r, g, b, pScreen)
     unsigned int cmap;
     unsigned int index;
     unsigned short r;
     unsigned short g;
     unsigned short b;
     ScreenPtr pScreen;
{
#ifdef DEBUG_PRINT
  ErrorF("svgaSetColor(cmap=%d, index=%d, r=%d, g=%d, b=%d)\n",
         cmap, index, r, g, b);
#endif
  outb(0x3C8, index);
  outb(0x3C9, (r >> 10));
  outb(0x3C9, (g >> 10));
  outb(0x3C9, (b >> 10));
}
