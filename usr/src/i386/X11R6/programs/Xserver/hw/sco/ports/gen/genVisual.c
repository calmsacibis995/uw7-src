#include "X.h"
#include "Xproto.h"
#include "regionstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "colormap.h"
#include "scrnintstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbProcs.h"

#include "genDefs.h"
#include "genProcs.h"


/*
 * genValidateVisual() - validate regions of the screen with different visuals
 *
 * This routine ensures that all areas described in pRegion have the
 * correct visual.  This gen version does nothing, so if you want to
 * support multiple visuals you need to implement this routine for your
 * hardware.
 */
void
genValidateVisual(pScreen, did, regions)
	ScreenPtr pScreen;
	int did;
	RegionPtr regions;
{
}
