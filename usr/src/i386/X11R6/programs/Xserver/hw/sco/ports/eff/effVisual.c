/*
 *	@(#) effVisual.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History
 * S002, 26-Sep-91, staceyc
 * 	fixed include files
 * S001, 26-Jun-91, buckm
 * 	new sgi source: args changed.
 * S000, 24-Jun-91, staceyc
 * 	included some files to clean up compile
 */
#include "X.h"
#include "Xproto.h"
#include "regionstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "colormap.h"
#include "scrnintstr.h"
#include "nfbGCStr.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbScrStr.h"
#include "nfbProcs.h"
#include "effConsts.h"
#include "effMacros.h"
#include "effDefs.h"
#include "effProcs.h"


/*
 * effValidateVisual() - validate regions of the screen with different visuals
 *
 * This routine ensures that all areas described in pRegion have the
 * correct visual.
 */
void
effValidateVisual(pScreen)
	ScreenPtr pScreen;
{
}
