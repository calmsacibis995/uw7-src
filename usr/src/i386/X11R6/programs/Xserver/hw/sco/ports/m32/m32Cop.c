/*
 * @(#) m32Cop.c 11.1 97/10/22
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
 */
/*
 * m32Cop.c
 *
 * m32 coprocessor routines
 */

#include "X.h"
#include "miscstruct.h"
#include "scrnintstr.h"

#include "m32Defs.h"
#include "m32ScrStr.h"

/*
 * m32SaveCop() - save the coprocessor state.
 *	For now, we won't bother actually saving
 *	and restoring the coprocessor state.
 *	Just wait for the graphics engine to finish.
 */
m32SaveCop(pScreen)
	ScreenPtr pScreen;
{
	M32_DBG_NAME("SaveCop");
	M32_IDLE();
}

/*
 * m32RestoreCop() - restore the coprocessor state.
 *	For now, we won't bother actually saving
 *	and restoring the dynamic state.
 */
m32RestoreCop(pScreen)
	ScreenPtr pScreen;
{
}

/*
 * m32StopCop() - stop the coprocessor.
 *	Just wait for the graphics engine to finish.
 */
m32StopCop(pScreen)
	ScreenPtr pScreen;
{
	M32_DBG_NAME("StopCop");
	M32_IDLE();
}

/*
 * m32InitCop() - init the coprocessor.
 */
m32InitCop(pScreen)
	ScreenPtr pScreen;
{
	M32_DBG_NAME("InitCop");

	/* reset ge; disable intrs */
	outw(M32_SUBSYS_CNTL, 0x8000);
	outw(M32_SUBSYS_CNTL, 0x400F);

	M32_IDLE();

	/* disable dest color compare */
	outw(M32_DEST_CMP_FN, 0x0000);

	/* set src start and end unequal to avoid blit aborts */
	outw(M32_SRC_X_START, 0x0000);
	outw(M32_SRC_X_END,   0x0001);
}
