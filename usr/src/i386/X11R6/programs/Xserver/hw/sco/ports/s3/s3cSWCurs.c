/*
 *	@(#)s3cSWCurs.c	6.1	3/20/96	10:23:33
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
 * S008, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S007, 11-May-93, staceyc
 * 	include file clean up, replace calls to assembler code with calls
 * 	to C code
 * S006, 05-Apr-93, staceyc
 * 	general cleanup
 * S005, 29-Oct-92, hiramc@sco.com
 *	Set pointer before use.  Remove modification comments before 1992
 * S004, 14-Jun-92, buckm@sco.com
 *	Use source and mask bits properly;
 *	Use pixel values instead of rgb values.
 * X003, 11-Jan-92, kevin@xware.com
 *      added support for software cursor for 1280x1024 display modes.
 * X002, 02-Jan-92, kevin@xware.com
 *      updated to effDispCur.c 1.10, changed name to s3cSWCurs.c and 
 *	added SW prefix to labels.
 * X001, 01-Jan-92, kevin@xware.com
 *      updated copyright notice.
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

extern WindowPtr	*WindowTable;

#define	S3C_MIN(a, b)	( a < b ? a : b )

/*
 *  s3cSWRealizeCursor() -- Realize the Software Cursor
 *
 *	This is a null routine.
 *
 */

static Bool
s3cSWRealizeCursor(
	ScreenPtr 		pScreen,
	CursorPtr 		pCursor)
{
	return TRUE;
}


/*
 *  s3cSWUnrealizeCursor() -- Unrealize the Software Cursor
 *
 *	This is a null routine.
 *
 */

static Bool
s3cSWUnrealizeCursor(
    	ScreenPtr		pScreen,
    	CursorPtr		pCursor)
{
	return TRUE;
}


/* 
 *  s3cSWPutUpCursor() -- Put Up Software Cursor
 *
 *	This routine will blit the software cursor image on the display
 *	in either the 4 or 8 bit display modes.
 *
 */

static Bool
s3cSWPutUpCursor(
	ScreenPtr 		pScreen,
	CursorPtr 		pCursor,
	int			x,
	int			y,
	unsigned long 		source,
	unsigned long		mask)
{
	int			lx;
	int			ly;
	int			sx;
	int			sy;
	int			mx;
	int			my;
	int			stride;
	BoxRec 			box;
	s3cPrivateData_t 	*s3cPriv;
	s3cSWCursorData_t 	*s3cSWCurs;

	s3cPriv = S3C_PRIVATE_DATA(pScreen);
	s3cSWCurs = S3C_SWCURSOR_DATA(pScreen);			/* S005 */
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

	if( ! pCursor )					/*	S005	*/
		return;
	
	if ( pCursor != s3cSWCurs->current_cursor )
	{
		/*
		 * copy the new cursor bitmaps into offscreen memory
		 */

		stride = PixmapBytePad(pCursor->bits->width, 1);


		s3cSWCurs->width = 
			S3C_MIN(pCursor->bits->width, S3C_MAX_CURSOR_SIZE);
		s3cSWCurs->height = 
			S3C_MIN(pCursor->bits->height, S3C_MAX_CURSOR_SIZE);

		box.x1 = s3cSWCurs->source.x;
		box.y1 = s3cSWCurs->source.y;
		box.x2 = box.x1 + s3cSWCurs->width;
		box.y2 = box.y1 + s3cSWCurs->height;

		S3CNAME(DrawOpaqueMonoImage)(&box, pCursor->bits->mask, 
			0, stride, 1, 0, GXcopy, S3C_GENERAL_OS_PLANE, 
			&WindowTable[pScreen->myNum]->drawable);

		S3CNAME(DrawOpaqueMonoImage)(&box, pCursor->bits->source, 
			0, stride, 1, 0, GXand, S3C_GENERAL_OS_PLANE, 
			&WindowTable[pScreen->myNum]->drawable);

		box.x1 = s3cSWCurs->mask.x;
		box.y1 = s3cSWCurs->mask.y;
		box.x2 = box.x1 + s3cSWCurs->width;
		box.y2 = box.y1 + s3cSWCurs->height;

		S3CNAME(DrawOpaqueMonoImage)(&box, pCursor->bits->mask, 
			0, stride, 1, 0, GXcopy, S3C_GENERAL_OS_PLANE, 
			&WindowTable[pScreen->myNum]->drawable);

		S3CNAME(DrawOpaqueMonoImage)(&box, pCursor->bits->source, 
			0, stride, 1, 0, GXandInverted, S3C_GENERAL_OS_PLANE, 
			&WindowTable[pScreen->myNum]->drawable);

		s3cSWCurs->current_cursor = pCursor;
	}	
	
	/*
	 * update the cursor position, and perform any clipping
	 */

	lx = s3cSWCurs->width;
	ly = s3cSWCurs->height;

	sx = s3cSWCurs->source.x;
	sy = s3cSWCurs->source.y;
	mx = s3cSWCurs->mask.x;
	my = s3cSWCurs->mask.y;

	if ( x < 0 )
	{
		sx += -x;
		mx += -x;
		lx += x;
		x = 0;
	}
	
	if ( y < 0 )
	{
		sy += -y;
		my += -y;
		ly += y;
		y = 0;
	}

	if ( x + lx >= s3cPriv->width )
		lx -= x + lx - s3cPriv->width;

	if ( y + ly >= s3cPriv->height )
		ly -= y + ly - s3cPriv->height;

	/*
	 * blit the cursor image onto the screen
	 */

	S3CNAME(SetupStretchBlit)(S3CNAME(RasterOps)[GXcopy], mask,
		S3C_GENERAL_OS_PLANE, S3C_ALLPLANES);
	S3CNAME(StretchBlit)(mx, my, x, y, lx - 1, ly - 1, S3C_BLIT_XP_YP_Y);

	S3CNAME(SetupStretchBlit)(S3CNAME(RasterOps)[GXcopy], source,
		S3C_GENERAL_OS_PLANE, S3C_ALLPLANES); 
	S3CNAME(StretchBlit)(sx, sy, x, y, lx - 1, ly - 1, S3C_BLIT_XP_YP_Y);

	return TRUE;
}


/* 
 *  s3cSWSaveUnder() -- Save Area Under Software Cursor
 *
 *	This routine will save the pixmap area under the software cursor
 *	in either the 4 or 8 bit display modes.
 *
 */

static Bool
s3cSWSaveUnderCursor(
	ScreenPtr 		pScreen,
	int			x,
	int			y,
	int			w,
	int			h)
{
	int			ly;
	int			lx;
	s3cPrivateData_t 	*s3cPriv;
	s3cSWCursorData_t 	*s3cSWCurs;

	s3cPriv = S3C_PRIVATE_DATA(pScreen);
	s3cSWCurs = S3C_SWCURSOR_DATA(pScreen);
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

	w = S3C_MIN(w, S3C_MAX_CURSOR_SAVE);
	h = S3C_MIN(h, S3C_MAX_CURSOR_SAVE);

	lx = w + ( x < 0 ? x - 1 : 0 ) 
		- ( x + w > s3cPriv->width ? x + w - s3cPriv->width : 0 );

	ly = h + ( y < 0 ? y - 1 : 0 ) 
		- ( y + h > s3cPriv->height ? y + h - s3cPriv->height : 0 );
	
	if ( lx <= 0 || ly <= 0 )
		return;

	S3CNAME(CpyRect)( x < 0 ? 0 : x, y < 0 ? 0 : y,
		s3cSWCurs->save.x, s3cSWCurs->save.y, lx - 1, ly - 1,
		S3CNAME(RasterOps)[GXcopy], S3C_ALLPLANES, S3C_BLIT_XP_YP_Y);

	return TRUE;
}


/* 
 *  s3cSWRestoreUnder() -- Restore Area Under Software Cursor
 *
 *	This routine will restore the pixmap area under the software cursor
 *	in either the 4 or 8 bit display modes.
 *
 */

static Bool
s3cSWRestoreUnderCursor(
	ScreenPtr 		pScreen,
	int			x,
	int			y,
	int			w,
	int			h)
{
	int			ly;
	int			lx;
	s3cPrivateData_t 	*s3cPriv;
	s3cSWCursorData_t 	*s3cSWCurs;

	s3cPriv = S3C_PRIVATE_DATA(pScreen);
	s3cSWCurs = S3C_SWCURSOR_DATA(pScreen);
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

	w = S3C_MIN(w, S3C_MAX_CURSOR_SAVE);
	h = S3C_MIN(h, S3C_MAX_CURSOR_SAVE);

	lx = w + ( x < 0 ? x - 1 : 0 ) 
		- ( x + w > s3cPriv->width ? x + w - s3cPriv->width : 0 );
	ly = h + ( y < 0 ? y - 1 : 0 ) 
		- ( y + h > s3cPriv->height ? y + h - s3cPriv->height : 0 );
	
	if ( lx <= 0 || ly <= 0 )
		return;

	S3CNAME(CpyRect)(s3cSWCurs->save.x, s3cSWCurs->save.y, 
		x < 0 ? 0 : x, y < 0 ? 0 : y, lx - 1, ly - 1,
		S3CNAME(RasterOps)[GXcopy], S3C_ALLPLANES, S3C_BLIT_XP_YP_Y);

	return TRUE;
}


/* 
 *  s3cSWMoveCursor() -- Move Software Cursor
 *
 *	This routine will move the software cursor in either the 4 or 8
 *	bit display modes.
 *
 */

static Bool
s3cSWMoveCursor(
	ScreenPtr		pScreen,
	CursorPtr		pCursor,
	int			x,
	int			y,
	int			w,
	int			h,
	int			dx,
	int			dy,
	unsigned long		source,
	unsigned long		mask)
{
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

	s3cSWRestoreUnderCursor(pScreen, x, y, w, h);
	s3cSWPutUpCursor(pScreen, pCursor, x + dx, y + dy, source, mask);
	
	return TRUE;
}


/* 
 *  s3cSWChangeSave() -- Change and Save Area Under Software Cursor
 *
 *	This routine will change and save the pixmap area under the 
 *	software cursor in either the 4 or 8 bit display modes.
 *
 */

static Bool
s3cSWChangeSave(
	ScreenPtr 		pScreen,
	int			x,
	int			y,
	int			w,
	int			h,
	int			dx,
	int			dy)
{
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

	s3cSWRestoreUnderCursor(pScreen, x + dx, y + dy, w, h);
	s3cSWSaveUnderCursor(pScreen, x, y, w, h);

	return TRUE;
}

static miSpriteCursorFuncRec	s3cSWCursFuncs = 
{
    	s3cSWRealizeCursor,
    	s3cSWUnrealizeCursor,
    	s3cSWPutUpCursor,
    	s3cSWSaveUnderCursor,
    	s3cSWRestoreUnderCursor,
    	s3cSWMoveCursor,
    	s3cSWChangeSave
};

/* 
 *  s3cSWCursorInitialize() -- Initialize the Software Cursor
 *
 *	This routine will register the cursor movement routines for the 
 *	hardware cursor.
 *
 */

void
S3CNAME(SWCursorInitialize)(
	ScreenPtr 		pScreen)
{
	s3cSWCursorData_t 	*s3cSWCurs;

	if ( !scoSpriteInitialize(pScreen, &s3cSWCursFuncs) )
	    FatalError("s3cSWCursorInitialize(): Cannot initialize cursor\n");
		
	s3cSWCurs = S3C_SWCURSOR_DATA(pScreen);
	s3cSWCurs->current_cursor = (CursorPtr)0;
}
