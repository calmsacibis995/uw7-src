/*
 *	@(#) effMisc.c 11.1 97/10/22
 */

/*
 * SCO Modification History
 *
 * S001, 11-Sep-91, staceyc
 * 	added screen ptr to parameter list
 * S000, 01-Aug-91, staceyc
 * 	added query size
 */

#include "X.h"
#include "misc.h"
#include "cursor.h"
#include "screenint.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "effConsts.h"
#include "effDefs.h"
#include "effMacros.h"
#include "effProcs.h"

void
effQueryBestSize(class, pwidth, pheight, pScreen)
int class;
short *pwidth;
short *pheight;
ScreenPtr pScreen;
{
	unsigned width, test;
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);
	effCursorData_t *cursor = &effPriv->cursor;

	switch(class)
	{
	case TileShape :
		if (*pwidth > effPriv->tile_blit_area.w / 2)
			*pwidth = effPriv->tile_blit_area.w / 2;
		if (*pheight > effPriv->tile_blit_area.h / 2)
			*pheight = effPriv->tile_blit_area.h / 2;
		break;
	case StippleShape :
		if (*pwidth > effPriv->off_screen_blit_area.w)
			*pwidth = effPriv->off_screen_blit_area.w;
		if (*pheight > effPriv->off_screen_blit_area.h)
			*pheight = effPriv->off_screen_blit_area.h;
		break;
	case CursorShape :
		*pwidth = cursor->cursor_max_size;
		*pheight = cursor->cursor_max_size;
		break;
	}
}
