/*
 *	@(#) effClip.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * SCO Modification History
 *
 * S005, 27-Aug-93, buckm
 *	screen privates now have card_clip and current_clip.
 * S004, 13-Jan-92, chrissc
 *	added atiSetClipRegions to deal with SCO-59-2732.  The ATI 8514
 *	emulation mode only clips effectively for 1024x768 and less.
 * S003, 31-Oct-92, mikep
 *	rearrange the arguments for effSetClipRegions to match the rest of nfb.
 * S002, 23-Oct-92, staceyc
 * 	8514/A clip registers take last drawable x2 and y2
 * S001, 13-Oct-92, staceyc
 * 	better make sure the queue has room
 * S000, 03-Sep-92, staceyc
 * 	created
 */

#include "X.h"
#include "effConsts.h"
#include "effMacros.h"
#include "effDefs.h"
#include "effProcs.h"

void
effSetClipRegions(
BoxRec *pbox,
int nbox,
DrawablePtr pDraw)
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	effPriv->current_clip = nbox ? *pbox : effPriv->card_clip;

	EFF_CLEAR_QUEUE(4);
	EFF_SETXMIN(effPriv->current_clip.x1);
	EFF_SETYMIN(effPriv->current_clip.y1);
	EFF_SETXMAX(effPriv->current_clip.x2 - 1);
	EFF_SETYMAX(effPriv->current_clip.y2 - 1);
}


void								/* S004 */
atiSetClipRegions(
BoxRec *pbox,
int nbox,
DrawablePtr pDraw)
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pDraw->pScreen);

	effPriv->current_clip = nbox ? *pbox : effPriv->card_clip;

	EFF_CLEAR_QUEUE(4);
	ATI_SETXMIN(effPriv->current_clip.x1);
	ATI_SETYMIN(effPriv->current_clip.y1);
	ATI_SETXMAX(effPriv->current_clip.x2 - 1);
	ATI_SETYMAX(effPriv->current_clip.y2 - 1);
}
