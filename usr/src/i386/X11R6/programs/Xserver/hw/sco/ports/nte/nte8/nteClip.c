/*
 *	@(#) nteClip.c 11.1 97/10/22
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
 * S001, 14-Jul-93, staceyc
 * 	clear queue checks for 24 bit modes
 * S000, 15-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

void
NTE(SetClipRegions)(
	BoxRec *pbox,
	int nbox,
	DrawablePtr pDraw)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(4);
	NTE_CLEAR_QUEUE24(4);
	if (nbox == 0)
	{
		NTE_SCISSORS_L(0);
		NTE_SCISSORS_T(0);
		NTE_SCISSORS_R(ntePriv->clip_x);
		NTE_SCISSORS_B(ntePriv->clip_y);
	}
	else
	{
		NTE_SCISSORS_L(pbox->x1);
		NTE_SCISSORS_T(pbox->y1);
		NTE_SCISSORS_R(pbox->x2 - 1);
		NTE_SCISSORS_B(pbox->y2 - 1);
	}
	NTE_END();
}
