/*
 *	@(#)s3cRectOps.c	6.1	3/20/96	10:23:31
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
 * S013, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S012, 11-May-93, staceyc
 * 	include file clean up, replace calls to assembler code with calls
 * 	to C code
 * S011	Mon Apr 05 14:57:01 PDT 1993	hiramc@sco.COM
 *	Queue size reference change for S3C_SETUP_SOLID_RECT(),
 *	was 4, is 5.  For 86C80[15], 86C928 after Kevin's changes.
 * S010	Thu Oct 29 17:40:04 PST 1992	mikep@sco.com
 *	Rewrite DrawPoints using fast lines one pixel long.
 *	Much better than rectangles.
 * S009	Wed Sep 23 08:50:04 PDT 1992	hiramc@sco.COM
 *	Remove SourceValidate calls, remove all mod comments
 *	prior to 1992.
 * X008	Fri Sep 04 16:41:58 PDT 1992	hiramc@sco.COM
 *	Proper declaration of arguments on TileRects
 * X007	Sun Jun 14 18:54:56 PDT 1992	buckm@sco.COM
 *	Fix unsigned modulo bugs in TileRects routines.
 * X006	Tue Jun 02 11:30:05 PDT 1992	hiramc@sco.COM
 *	Bug in software cursor when copying areas.
 * X005 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 * 
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

static void
s3cTileAreaSlow(
	BoxRec 		*pbox,
	DDXPointRec 	*off_screen_point,
	int 		avail_height,
	int 		avail_width,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw,
	void		(*CopyRect)());

static void
s3cTileAreaSlowXExpand(
	BoxRec 		*pbox,
	DDXPointRec 	*off_screen_point,
	int 		avail_width,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw,
	void		(*CopyRect)());


/*
 *  S3CNAME(CpyRect)() -- Copy Rectangle
 *
 *	This routine will copy a rectangle using the 86c911 blit engine.
 *
 *	Note: This routine is coded in assembly in s3cRect.s
 */

void
S3CNAME(CpyRect)(
	int		srcx,
	int		srcy,
	int		dstx,
	int		dsty,
	int		lx,
	int		ly,
	int		rop,
	int		planemask,
	int		command)
{
	S3C_CLEAR_QUEUE(4);
	S3C_PLNWENBL(planemask);
	S3C_PLNRENBL(planemask);
	S3C_SETMODE(S3C_M_ONES);
	S3C_SETFN1(S3C_FNCPYRCT, rop);

	S3C_CLEAR_QUEUE(7);
	S3C_SETX0(srcx);
	S3C_SETY0(srcy);
	S3C_SETX1(dstx);
	S3C_SETY1(dsty);
    	S3C_SETLX(lx);
	S3C_SETLY(ly);

	S3C_COMMAND(command);
}

#define S3C_SETUP_SOLID_RECT(rop, fg, planemask) \
{ \
	S3C_CLEAR_QUEUE(5); \
	S3C_PLNWENBL(planemask); \
	S3C_PLNRENBL(S3C_RPLANES); \
	S3C_SETMODE(S3C_M_ONES); \
	S3C_SETFN1(S3C_FNCOLOR1, rop); \
	S3C_SETCOL1(fg); \
}

/*
 *  S3C_SOLID_RECT() -- Solid Rectangle Fill
 *
 *	This routine will fill a rectangle using the 86c911 blit engine.
 *
 *	Note: This routine is coded in assembly in s3cRect.s
 */

#define S3C_SOLID_RECT(x, y, lx, ly, command) \
{ \
	S3C_CLEAR_QUEUE(5); \
	S3C_SETX0(x); \
	S3C_SETY0(y); \
    	S3C_SETLX(lx); \
	S3C_SETLY(ly); \
	S3C_COMMAND(command); \
}

/*
 *  S3CNAME(CopyRect)() -- Copy Rectangle
 *
 *	This routine will copy a rectangle on the display in either the 
 *	4 or 8 bit display modes.
 *
 */

void
S3CNAME(CopyRect)(
	BoxPtr 		pdstBox,
	DDXPointPtr 	psrc,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw)
{
	int		ly;
	int		lx;
	int		x0;
	int		x1;
	int		y0;
	int		y1;
	unsigned short 	command = S3C_BLIT_XP_YP_Y;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	ly = pdstBox->y2 - pdstBox->y1;
	lx = pdstBox->x2 - pdstBox->x1;
	x0 = psrc->x;
	y0 = psrc->y;
	x1 = pdstBox->x1;
	y1 = pdstBox->y1;
	--lx;
	--ly;

	if ( x1 > x0 )
	{
		x0 += lx; 
		x1 += lx;
		command &= ~S3C_CMD_YN_XP_X;
	}

	if ( y1 > y0 )
	{
		y0 += ly;  
		y1 += ly;
		command &= ~S3C_CMD_YP_XN_X;
	}

	S3CNAME(CpyRect)(x0, y0, x1, y1, lx, ly, 
		S3CNAME(RasterOps)[alu], planemask, command);
}


/*
 *  S3CNAME(CopyRect16)() -- Copy Rectangle 16
 *
 *	This routine will copy a rectangle on the display in the 16 bit
 *	display modes.
 *
 */

void
S3CNAME(CopyRect16)(
	BoxPtr 		pdstBox,
	DDXPointPtr 	psrc,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw)
{
	int		ly;
	int		lx;
	int		x0;
	int		x1;
	int		y0;
	int		y1;
	unsigned short 	command = S3C_BLIT_XP_YP_Y;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	ly = pdstBox->y2 - pdstBox->y1;
	lx = pdstBox->x2 - pdstBox->x1;
	x0 = psrc->x;
	y0 = psrc->y;
	x1 = pdstBox->x1;
	y1 = pdstBox->y1;
	--lx;
	--ly;

	if ( x1 > x0 )
	{
		x0 += lx; 
		x1 += lx;
		command &= ~S3C_CMD_YN_XP_X;
	}

	if ( y1 > y0 )
	{
		y0 += ly;  
		y1 += ly;
		command &= ~S3C_CMD_YP_XN_X;
	}

	S3CNAME(CpyRect)(x0, y0, x1, y1, lx, ly, 
		S3CNAME(RasterOps)[alu], planemask, command);

	S3CNAME(CpyRect)(x0 + 1024, y0, x1 + 1024, y1, lx, ly, 
		S3CNAME(RasterOps)[alu], planemask >> 8, command);
}


/*
 *  s3cDrawPoints() -- Draw Points
 *
 *	This routine will draw a list of points on the display. 
 *
 */

void
S3CNAME(DrawPoints)(
	DDXPointPtr 	ppt,
	unsigned int 	npts,
	unsigned long 	fg,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw)
{
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

	S3C_CLEAR_QUEUE(6);
	S3C_PLNWENBL(planemask);
	S3C_PLNRENBL(S3C_RPLANES);
	S3C_SETMODE(S3C_M_ONES);
	S3C_SETFN1(S3C_FNCOLOR1, S3CNAME(RasterOps)[alu]);
	S3C_SETCOL1(fg);
	
	/* draw fast lines of length 1 */
	S3C_SETLX(0);

	/*
	 * inline code will really speed this up. 
	 */
	do
	{
	    S3C_CLEAR_QUEUE(3);
	    S3C_SETX0(ppt->x);
	    S3C_SETY0(ppt->y);
	    S3C_COMMAND(S3C_CMD_LINE | S3C_CMD_DRAW
			| S3C_CMD_RADIAL | S3C_CMD_MULTIPLE
			| S3C_CMD_WRITE);

	    ++ppt;
	}
	while (--npts);

	return;
}

void
S3CNAME(DrawPoints16)(
	DDXPointPtr 	ppt,
	unsigned int 	npts,
	unsigned long 	fg,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw)
{
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

	S3C_CLEAR_QUEUE(5);
	S3C_PLNWENBL(planemask);
	S3C_PLNRENBL(S3C_RPLANES);
	S3C_SETMODE(S3C_M_ONES);
	S3C_SETFN1(S3C_FNCOLOR1, S3CNAME(RasterOps)[alu]);
	
	/* draw fast lines of length 1 */
	S3C_SETLX(0);

	/*
	 * inline code will really speed this up. 
	 */
	do
	{
	    S3C_CLEAR_QUEUE(8);
	    S3C_SETCOL1(fg);
	    S3C_SETX0(ppt->x);
	    S3C_SETY0(ppt->y);
	    S3C_COMMAND(S3C_CMD_LINE | S3C_CMD_DRAW
			| S3C_CMD_RADIAL | S3C_CMD_MULTIPLE
			| S3C_CMD_WRITE);
	    S3C_PLNWENBL(planemask >> 8);
	    S3C_SETCOL1(fg >> 8);
	    S3C_SETX0(ppt->x + 1024);
	    /* Y0 is already set */
	    S3C_COMMAND(S3C_CMD_LINE | S3C_CMD_DRAW
			| S3C_CMD_RADIAL | S3C_CMD_MULTIPLE
			| S3C_CMD_WRITE);
	    ++ppt;
	}
	while ( --npts );

	return;
}


/*
 *  s3cDrawSolidRects() -- Draw Solid Rectangles
 *
 *	This routine will draw a list of solid retangls on the display 
 *	in either the 4 or 8 bit display modes.
 *
 */

void
S3CNAME(DrawSolidRects)(
	BoxPtr 		pbox,
	unsigned int 	nbox,
	unsigned long 	fg,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw)
{
	int 		lx;
	int		ly;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

	S3C_SETUP_SOLID_RECT(S3CNAME(RasterOps)[alu], fg, planemask);
	
	while (nbox--)
	{
		lx = pbox->x2 - pbox->x1 - 1;
		ly = pbox->y2 - pbox->y1 - 1;

		S3C_SOLID_RECT(pbox->x1, pbox->y1, lx, ly, S3C_FILL_X_Y_DATA);

		++pbox;
	}
}


/*
 *  s3cDrawSolidRects16() -- Draw Solid Rectangles 16
 *
 *	This routine will draw a list of solid retangls on the display 
 *	in the 16 bit display modes.
 *
 */

void
S3CNAME(DrawSolidRects16)(
	BoxPtr 		pbox,
	unsigned int 	nbox,
	unsigned long 	fg,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw)
{
	int 		lx;
	int		ly;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

	while (nbox--)
	{
		lx = pbox->x2 - pbox->x1 - 1;
		ly = pbox->y2 - pbox->y1 - 1;

		S3C_SETUP_SOLID_RECT(S3CNAME(RasterOps)[alu], fg, planemask);
		S3C_SOLID_RECT(pbox->x1, pbox->y1, lx, ly, 
			S3C_FILL_X_Y_DATA);

		S3C_SETUP_SOLID_RECT(S3CNAME(RasterOps)[alu], fg >> 8,
		    planemask >> 8);
		S3C_SOLID_RECT(pbox->x1 + 1024, pbox->y1, lx, ly, 
			S3C_FILL_X_Y_DATA);

		++pbox;
	}
}


/*
 *  s3cTileRects() -- Tiled Rectangles
 *
 *	This routine will draw a list of tiles retangls on the display 
 *	in either the 4 or 8 bit display modes.
 *
 */

void
S3CNAME(TileRects)(
	BoxPtr 			pbox,
	unsigned int		nbox,			/*	X008	*/
	unsigned char 		*tile,
	unsigned int		stride,			/*	X008	*/
	unsigned int 		w,
	unsigned int 		h,
	DDXPointPtr 		patOrg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pDraw)
{
	s3cOffScreenBlitArea_t	*tile_blit_area;
	BoxRec 			off_screen_box;
	BoxRec 			screen_box;
	BoxRec 			*boxp;
	DDXPointRec 		off_screen_point;
	int 			tiles_wide;
	int			tiles_high;
	int 			avail_width;
	int 			avail_height;
	Bool 			clobber_alu;
	int			lx;
	int			ly;
	int 			xoff;
	int			yoff;
	int			i;
	int 			biggest_w;
	int			biggest_h;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

	tile_blit_area = &S3C_PRIVATE_DATA(pDraw->pScreen)->tile_blit_area;

	/*
	 * how many tiles can we fit in the off-screen area?
	 */

	tiles_wide = tile_blit_area->w / w;
	tiles_high = tile_blit_area->h / h;

	/*
	 * does the raster op allow us to clobber the destination
	 * or does it require some src/dest mix
	 */

	clobber_alu = alu == GXcopy || alu == GXcopyInverted ||
	    	alu == GXclear || alu == GXset || alu == GXinvert;

	/*
	 * When to use gen code:
	 * - our blit with alu requires at least 2x2 tiles stashed in
	 *   off-screen memory to deal with patOrg correctly, if the
	 *   tile is too big then use gen,
	 * - gen code draws a single tiled box quickly relative to s3c
	 *   code, iff the raster op allows the dest to be clobbered by
	 *   the source.  In the case where the raster op requires a mix 
	 *   with the source, s3c code will generally be faster.
	 */

	if ( tiles_wide < 2 || tiles_high < 2 || (nbox < 2 && clobber_alu) )
	{
		genTileRects(pbox, nbox, tile, stride, w, h, patOrg, alu,
			planemask, pDraw);
		return;
	}

	avail_width = (tiles_wide - 1) * w;
	avail_height = (tiles_high - 1) * h;

	/*
	 * draw one copy of the tile to off-screen
	 */

	off_screen_box.x1 = tile_blit_area->x;
	off_screen_box.y1 = tile_blit_area->y;
	off_screen_box.x2 = tile_blit_area->x + w;
	off_screen_box.y2 = tile_blit_area->y + h;

	if ( pDraw->bitsPerPixel == 1 )
	{
		/*
		 * the gen version of this routine supports bitmaps, but
		 * doesn't specify fg/bg!
		 */

		S3CNAME(DrawOpaqueMonoImage)(&off_screen_box, tile, 0, stride, 1,
		    	0, GXcopy, ~0, pDraw);
	}
	else
	{
		S3CNAME(DrawImage)(&off_screen_box, tile, stride, GXcopy, ~0, pDraw);
	}

	/*
	 * fill up the off-screen tile area based on the largest dimensions
	 * to be drawn
	 */

	biggest_h = 0;
	biggest_w = 0;

	for ( boxp = pbox, i = 0; i < nbox; ++i, ++boxp )
	{
		lx = boxp->x2 - boxp->x1;
		ly = boxp->y2 - boxp->y1;
		if ( lx > biggest_w )
			biggest_w = lx;
		if ( ly > biggest_h )
			biggest_h = ly;
	}

	biggest_w += w; /* allowance for patOrg */
	biggest_h += h;

	if ( biggest_w > tile_blit_area->w )
		biggest_w = tile_blit_area->w;

	if ( biggest_h > tile_blit_area->h )
		biggest_h = tile_blit_area->h;

	off_screen_box.x2 = tile_blit_area->x + biggest_w;
	off_screen_box.y2 = tile_blit_area->y + biggest_h;

	nfbReplicateArea(&off_screen_box, w, h, ~0, pDraw);

	/*
	 * draw the boxes
	 */

	while ( nbox-- )
	{
		lx = pbox->x2 - pbox->x1;
		ly = pbox->y2 - pbox->y1;

		/*
		 * calculate patOrg offsets into off-screen tile
		 */

		xoff = (pbox->x1 - patOrg->x) % (int) w;	/* X007 */

		if ( xoff < 0 )
			xoff += w;

		yoff = (pbox->y1 - patOrg->y) % (int) h;	/* X007 */

		if ( yoff < 0 )
			yoff += h;

		off_screen_point.x = tile_blit_area->x + xoff;
		off_screen_point.y = tile_blit_area->y + yoff;

		/*
		 * determine how to blit box to dest
		 */

		if ( lx < avail_width && ly < avail_height )
		{
			/*
			 * screen box is smaller than off-screen tiled
			 * area, so blit straight to on-screen
			 */

			S3CNAME(CopyRect)(pbox, &off_screen_point, alu, planemask,
			    	pDraw);
		}
		else
		{
			if ( clobber_alu )
			{
				/*
				 * for these raster ops it's okay to blit the
				 * tile to on-screen, then replicate the tile
				 * on-screen to the full size of the rect box
				 */

				screen_box.x1 = pbox->x1;
				screen_box.y1 = pbox->y1;

				if ( lx < avail_width )
					screen_box.x2 = screen_box.x1 + lx;
				else
					screen_box.x2 = screen_box.x1 +
					    	avail_width;

				if ( ly < avail_height )
					screen_box.y2 = screen_box.y1 + ly;
				else
					screen_box.y2 = screen_box.y1 +
					    	avail_height;

				/*
				 * blit as much as possible from off-screen
				 */

				S3CNAME(CopyRect)(&screen_box, &off_screen_point, alu,
				    	planemask, pDraw);
				    
				/*
				 * replicate rest
				 */

				nfbReplicateArea(pbox, avail_width,
				    	avail_height, planemask, pDraw);
			}
			else
			{
				/*
				 * any raster op here requires mixing src and
				 * dest - this is the slow way - fortunately
				 * not too many apps will do this - to test
				 * this run x11perf with -xor and tile ops
				 * - this code is still faster than gen
				 */

				s3cTileAreaSlow(pbox, &off_screen_point,
				    	avail_height, avail_width, alu, 
					planemask, pDraw, S3CNAME(CopyRect));
			}
		}
		++pbox;
	}
}


/*
 *  s3cTileRects16() -- Tiled Rectangles 16
 *
 *	This routine will draw a list of tiles retangls on the display 
 *	in the 16 bit display modes.
 *
 */

void
S3CNAME(TileRects16)(
	BoxPtr 			pbox,
	unsigned int		nbox,			/*	X008	*/
	unsigned char 		*tile,
	unsigned int		stride,			/*	X008	*/
	unsigned int 		w,
	unsigned int 		h,
	DDXPointPtr 		patOrg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pDraw)
{
	s3cOffScreenBlitArea_t	*tile_blit_area;
	BoxRec 			off_screen_box;
	BoxRec 			screen_box;
	BoxRec 			*boxp;
	DDXPointRec 		off_screen_point;
	int 			tiles_wide;
	int			tiles_high;
	int 			avail_width;
	int 			avail_height;
	Bool 			clobber_alu;
	int			lx;
	int			ly;
	int 			xoff;
	int			yoff;
	int			i;
	int 			biggest_w;
	int			biggest_h;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);

	tile_blit_area = &S3C_PRIVATE_DATA(pDraw->pScreen)->tile_blit_area;

	/*
	 * how many tiles can we fit in the off-screen area?
	 */

	tiles_wide = tile_blit_area->w / w;
	tiles_high = tile_blit_area->h / h;

	/*
	 * does the raster op allow us to clobber the destination
	 * or does it require some src/dest mix
	 */

	clobber_alu = alu == GXcopy || alu == GXcopyInverted ||
	    	alu == GXclear || alu == GXset || alu == GXinvert;

	/*
	 * When to use gen code:
	 * - our blit with alu requires at least 2x2 tiles stashed in
	 *   off-screen memory to deal with patOrg correctly, if the
	 *   tile is too big then use gen,
	 * - gen code draws a single tiled box quickly relative to s3c
	 *   code, iff the raster op allows the dest to be clobbered by
	 *   the source.  In the case where the raster op requires a mix
	 *   with the source, s3c code will generally be faster.
	 */

	if ( tiles_wide < 2 || tiles_high < 2 || (nbox < 2 && clobber_alu) )
	{
		genTileRects(pbox, nbox, tile, stride, w, h, patOrg, alu,
			planemask, pDraw);
		return;
	}

	avail_width = (tiles_wide - 1) * w;
	avail_height = (tiles_high - 1) * h;

	/*
	 * draw one copy of the tile to off-screen
	 */

	off_screen_box.x1 = tile_blit_area->x;
	off_screen_box.y1 = tile_blit_area->y;
	off_screen_box.x2 = tile_blit_area->x + w;
	off_screen_box.y2 = tile_blit_area->y + h;

	if ( pDraw->bitsPerPixel == 1 )
	{
		/*
		 * the gen version of this routine supports bitmaps, but
		 * doesn't specify fg/bg!
		 */

		S3CNAME(DrawOpaqueMonoImage16)(&off_screen_box, tile, 0, stride, 1,
		    	0, GXcopy, ~0, pDraw);
	}
	else
	{
		S3CNAME(DrawImage16)(&off_screen_box, tile, stride, GXcopy,~0, pDraw);
	}

	/*
	 * fill up the off-screen tile area based on the largest dimensions
	 * to be drawn
	 */

	biggest_h = 0;
	biggest_w = 0;

	for ( boxp = pbox, i = 0; i < nbox; ++i, ++boxp )
	{
		lx = boxp->x2 - boxp->x1;
		ly = boxp->y2 - boxp->y1;
		if ( lx > biggest_w )
			biggest_w = lx;
		if ( ly > biggest_h )
			biggest_h = ly;
	}

	biggest_w += w; /* allowance for patOrg */
	biggest_h += h;

	if ( biggest_w > tile_blit_area->w )
		biggest_w = tile_blit_area->w;

	if ( biggest_h > tile_blit_area->h )
		biggest_h = tile_blit_area->h;

	off_screen_box.x2 = tile_blit_area->x + biggest_w;
	off_screen_box.y2 = tile_blit_area->y + biggest_h;

	nfbReplicateArea(&off_screen_box, w, h, ~0, pDraw);

	/*
	 * draw the boxes
	 */

	while ( nbox-- )
	{
		lx = pbox->x2 - pbox->x1;
		ly = pbox->y2 - pbox->y1;

		/*
		 * calculate patOrg offsets into off-screen tile
		 */

		xoff = (pbox->x1 - patOrg->x) % (int) w;	/* X007 */

		if ( xoff < 0 )
			xoff += w;

		yoff = (pbox->y1 - patOrg->y) % (int) h;	/* X007 */

		if ( yoff < 0 )
			yoff += h;

		off_screen_point.x = tile_blit_area->x + xoff;
		off_screen_point.y = tile_blit_area->y + yoff;

		/*
		 * determine how to blit box to dest
		 */

		if ( lx < avail_width && ly < avail_height )
		{
			/*
			 * screen box is smaller than off-screen tiled
			 * area, so blit straight to on-screen
			 */

			S3CNAME(CopyRect16)(pbox, &off_screen_point, alu, planemask,
			    	pDraw);
		}
		else
		{
			if ( clobber_alu )
			{
				/*
				 * for these raster ops it's okay to blit the
				 * tile to on-screen, then replicate the tile
				 * on-screen to the full size of the rect box
				 */

				screen_box.x1 = pbox->x1;
				screen_box.y1 = pbox->y1;

				if ( lx < avail_width )
					screen_box.x2 = screen_box.x1 + lx;
				else
					screen_box.x2 = screen_box.x1 +
					    	avail_width;

				if ( ly < avail_height )
					screen_box.y2 = screen_box.y1 + ly;
				else
					screen_box.y2 = screen_box.y1 +
					    	avail_height;

				/*
				 * blit as much as possible from off-screen
				 */

				S3CNAME(CopyRect16)(&screen_box, &off_screen_point, alu,
				    	planemask, pDraw);
				    
				/*
				 * replicate rest
				 */

				nfbReplicateArea(pbox, avail_width,
				    	avail_height, planemask, pDraw);
			}
			else
			{
				/*
				 * any raster op here requires mixing src and
				 * dest - this is the slow way - fortunately
				 * not too many apps will do this - to test
				 * this run x11perf with -xor and tile ops
				 * - this code is still faster than gen
				 */

				s3cTileAreaSlow(pbox, &off_screen_point,
				    	avail_height, avail_width, alu, 
					planemask, pDraw, S3CNAME(CopyRect16));
			}
		}
		++pbox;
	}
}
/*
 *  s3cTileAreaSlow() -- Tile Area Slow
 *
 *	This routine will tile a specified area on the display in either
 *	the 4, 8, or 16 bit display modes.
 *
 */

static void
s3cTileAreaSlow(
	BoxRec 		*pbox,
	DDXPointRec 	*off_screen_point,
	int 		avail_height,
	int 		avail_width,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw,
	void		(*CopyRect)())
{
	int 		ly;
	int		chunks;
	int		extra_height;
	BoxRec 		screen_box;

	ly = pbox->y2 - pbox->y1;
	chunks = ly / avail_height;
	extra_height = ly % avail_height;
	screen_box.x1 = pbox->x1;
	screen_box.x2 = pbox->x2;
	screen_box.y1 = pbox->y1;

	while ( chunks-- )
	{
		screen_box.y2 = screen_box.y1 + avail_height;
		s3cTileAreaSlowXExpand(&screen_box, off_screen_point,
		    	avail_width, alu, planemask, pDraw, CopyRect);
		screen_box.y1 += avail_height;
	}

	if ( extra_height )
	{
		screen_box.y2 = screen_box.y1 + extra_height;
		s3cTileAreaSlowXExpand(&screen_box, off_screen_point,
		    	avail_width, alu, planemask, pDraw, CopyRect);
	}
}


/*
 *  s3cTileAreaSlowExpand() -- Tile Area Slow Expand
 *
 *	This routine will tile a specified area on the display in either
 *	the 4, 8, or 16 bit display modes.
 *
 */

static void
s3cTileAreaSlowXExpand(
	BoxRec 		*pbox,
	DDXPointRec 	*off_screen_point,
	int 		avail_width,
	unsigned char 	alu,
	unsigned long 	planemask,
	DrawablePtr 	pDraw,
	void		(*CopyRect)())
{
	int 		lx;
	int 		chunks;
	int		extra_width;
	BoxRec 		screen_box;

	lx = pbox->x2 - pbox->x1;
	chunks = lx / avail_width;
	extra_width = lx % avail_width;
	screen_box.y1 = pbox->y1;
	screen_box.y2 = pbox->y2;
	screen_box.x1 = pbox->x1;

	while ( chunks-- )
	{
		screen_box.x2 = screen_box.x1 + avail_width;
		(*CopyRect)(&screen_box, off_screen_point, alu, planemask,
		    	pDraw);
		screen_box.x1 += avail_width;
	}

	if ( extra_width )
	{
		screen_box.x2 = screen_box.x1 + extra_width;
		(*CopyRect)(&screen_box, off_screen_point, alu, planemask,
			pDraw);
	}
}

