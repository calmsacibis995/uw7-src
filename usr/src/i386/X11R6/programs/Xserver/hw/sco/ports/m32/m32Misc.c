/*
 * @(#) m32Misc.c 11.1 97/10/22
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
 *	Deal with sw cursor; change tile reply.
 */

#include "X.h"
#include "miscstruct.h"
#include "scrnintstr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"

void
m32QueryBestSize(class, pwidth, pheight, pScreen)
	int class;
	short *pwidth;
	short *pheight;
	ScreenPtr pScreen;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScreen);

	switch(class) {
	    case CursorShape:
		if (pM32->useSWCurs) {
			if (*pwidth > pScreen->width)
				*pwidth = pScreen->width;
			if (*pheight > pScreen->height)
				*pheight = pScreen->height;
		} else {
			m32CursInfoPtr pCI = &pM32->cursInfo;
			*pwidth  = pCI->maxw;
			*pheight = pCI->maxh;
		}
		break;
	    case TileShape:
		/* no larger prefered width */
		break;
	    case StippleShape:
		/* oh, how about width a multiple of 8 ? */
		*pwidth = (*pwidth + 7) & ~7;
		break;
	}
}

