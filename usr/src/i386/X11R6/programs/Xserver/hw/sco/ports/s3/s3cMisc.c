/*
 *	@(#)s3cMisc.c	6.1	3/20/96	10:23:24
 *
 * 	Copyright (C) Xware, 1991-1992.
 *
 * 	The information in this file is provided for the exclusive use
 *	of the licensees of Xware. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they 
 *	include this notice and the associated copyright notice with 
 *	any such product.
 *
 *	Copyright (C) The Santa Cruz Operation, 1993
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	The information in this file is provided for the exclusive use
 *	of the licensees of SCO. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such
 *	product.
 *
 * Modification History
 *
 * S007, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S006, 11-May-93, staceyc
 * 	include file cleanup
 * X005 02-Jan-92 kevin@xware.com
 *      removed reference to unused s3cCursorData_t.
 * X004 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 * X003 31-Dec-91 kevin@xware.com
 *	added support for 16 bit 64K color modes.
 * X002 06-Dec-91 kevin@xware.com
 *	modified for style consistency, cosmetic only.
 * X001 23-Nov-91 kevin@xware.com
 *	initial modification to support 86c911 hardware cursor.
 * X000 23-Oct-91 kevin@xware.com
 *	initial source adopted from SCO's sample 8514a source (eff).
 * 
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"


/*
 *  s3cQueryBestSize() -- Query Best Size for Tile, Stipple, or Cursor
 *
 *	This routine will return the closest size to the requested 
 *	size that display can support optimaly.
 */

void
S3CNAME(QueryBestSize)(
	int 			class,
	short 			*pwidth,
	short 			*pheight,
	ScreenPtr	 	pScreen)
{
	unsigned 		width;
	unsigned		test;
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pScreen);

	switch( class )
	{
		case TileShape :
			if (*pwidth > s3cPriv->tile_blit_area.w / 2)
				*pwidth = s3cPriv->tile_blit_area.w / 2;
			if (*pheight > s3cPriv->tile_blit_area.h / 2)
				*pheight = s3cPriv->tile_blit_area.h / 2;
			break;

		case StippleShape :
			if (*pwidth > s3cPriv->off_screen_blit_area.w)
				*pwidth = s3cPriv->off_screen_blit_area.w;
			if (*pheight > s3cPriv->off_screen_blit_area.h)
				*pheight = s3cPriv->off_screen_blit_area.h;
			break;
	
		case CursorShape :
			*pwidth = s3cPriv->depth != 16 ? 
				S3C_MAX_CURSOR_SIZE : S3C_MAX_CURSOR_SIZE >> 1;
			*pheight = S3C_MAX_CURSOR_SIZE;
			break;
	}
}

