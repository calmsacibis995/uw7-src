/*
 * @(#) i128Cmap.c 11.1 97/10/22
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

#include <sys/types.h>
#include "screenint.h"

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"
#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbGlyph.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "gen/genDefs.h"
#include "gen/genProcs.h"

#include "i128Defs.h"

/*
 * i128SetColor() - set an entry in a PseudoColor colormap
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
i128SetColor(cmap, index, r, g, b, pScreen)
     unsigned int cmap;
     unsigned int index;
     unsigned short r;
     unsigned short g;
     unsigned short b;
     ScreenPtr pScreen;
{

     i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
     volatile char *pal_index = 
          i128Priv->info.global->dac_regs + I128_DAC_PAL_DAT;
     volatile char *pal_write =
          i128Priv->info.global->dac_regs + I128_DAC_WR_ADDR;
     
     r >>= 8;
     g >>= 8;
     b >>= 8;
     
     I128_WAIT_UNTIL_READY(i128Priv->engine);
     *pal_write = (unsigned char)index;
     *pal_index = (unsigned char)(r);
     *pal_index = (unsigned char)(g);
     *pal_index = (unsigned char)(b);

}
