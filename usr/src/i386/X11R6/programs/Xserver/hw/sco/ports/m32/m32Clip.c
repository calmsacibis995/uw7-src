/*
 * @(#) m32Clip.c 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 10-Aug-93, buckm
 *	Created.
 * S001, 21-Sep-94, davidw
 *	Corrected compiler warnings.
 */

#include "X.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "colormapst.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"					/* S001 vv*/
#include "nfb/nfbGlyph.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"					/* S001 ^^*/
#include "nfb/nfbScrStr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"

#define	MAX_CLIP_X	2048
#define	MAX_CLIP_Y	2048

void
m32SetClip(pbox, nbox, pDraw)
	BoxPtr pbox;
	int nbox;
	DrawablePtr pDraw;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pDraw->pScreen);

	if (nbox == 0) {
		pM32->clip.x1 = 0;
		pM32->clip.y1 = 0;
		pM32->clip.x2 = MAX_CLIP_X;
		pM32->clip.y2 = MAX_CLIP_Y;
	} else {
		pM32->clip = *pbox;
	}

	M32_CLEAR_QUEUE(4);
	outw(M32_EXT_SCISSOR_L, pM32->clip.x1);
	outw(M32_EXT_SCISSOR_T, pM32->clip.y1);
	outw(M32_EXT_SCISSOR_R, pM32->clip.x2 - 1);
	outw(M32_EXT_SCISSOR_B, pM32->clip.y2 - 1);
}

void
m32InitClip(pScr)
	ScreenPtr pScr;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScr);

	pM32->clip.x1 = 0;
	pM32->clip.y1 = 0;
	pM32->clip.x2 = MAX_CLIP_X;
	pM32->clip.y2 = MAX_CLIP_Y;

	M32_CLEAR_QUEUE(4);
	outw(M32_EXT_SCISSOR_L, pM32->clip.x1);
	outw(M32_EXT_SCISSOR_T, pM32->clip.y1);
	outw(M32_EXT_SCISSOR_R, pM32->clip.x2 - 1);
	outw(M32_EXT_SCISSOR_B, pM32->clip.y2 - 1);
}

void
m32RestoreClip(pScr)
	ScreenPtr pScr;
{
	m32ScrnPrivPtr pM32 = M32_SCREEN_PRIV(pScr);

	M32_CLEAR_QUEUE(4);
	outw(M32_EXT_SCISSOR_L, pM32->clip.x1);
	outw(M32_EXT_SCISSOR_T, pM32->clip.y1);
	outw(M32_EXT_SCISSOR_R, pM32->clip.x2 - 1);
	outw(M32_EXT_SCISSOR_B, pM32->clip.y2 - 1);
}
