/*
 *	@(#)s3cStip.c	6.1	3/20/96	10:23:38
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
 * 	updated file, removed old comments
 * X005, 01-Jan-92, kevin@xware.com
 *      updated copyright notice.
 * X004, 31-Dec-91, kevin@xware.com
 *	added support for 16 bit 64K color modes.
 * X003, 06-Dec-91, kevin@xware.com
 *	modified for style consistency, cosmetic only.
 * X002, 01-Dec-91, kevin@xware.com
 *	moved I/O bound sections of stretch blit functions to s3cSBlit.s.
 * X001, 29-Nov-91, kevin@xware.com
 *	removed support for 8514a.
 * X000, 23-Oct-91, kevin@xware.com
 *	initial source adopted from SCO's sample 8514a source (eff).
 * 
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"


/*
 *  S3CNAME(SetupStretchBlit)() -- Setup Stretch Blit
 *
 *	This routine setup the 86c911 for stretch blit.
 *
 *	Note: This routine is coded in assembly in s3cSBlit.s
 */

void
S3CNAME(SetupStretchBlit)(
	int 	rop,
	int 	fg,
	int 	readplanemask,
	int 	writeplanemask)
{
	S3C_CLEAR_QUEUE(6);
	S3C_PLNRENBL(readplanemask);
	S3C_PLNWENBL(writeplanemask);
	S3C_SETMODE(S3C_M_CPYRCT);
	S3C_SETFN0(S3C_FNCOLOR0, S3C_FNNOP);
	S3C_SETFN1(S3C_FNCOLOR1, rop);
	S3C_SETCOL1(fg);
}


/*
 *  s3cSetupOpaqueStretchBlit() -- Setup Opaque Stretch Blit
 *
 *	This routine setup the 86c911 for opaque stretch blit.
 *
 *	Note: This routine is coded in assembly in s3cSBlit.s
 */

static void
s3cSetupOpaqueStretchBlit(
	int	rop,
	int	bg,
	int 	fg,
	int 	readplanemask,
	int	writeplanemask)
{
	S3C_CLEAR_QUEUE(7);
	S3C_PLNRENBL(readplanemask);
	S3C_PLNWENBL(writeplanemask);
	S3C_SETMODE(S3C_M_CPYRCT);
	S3C_SETFN0(S3C_FNCOLOR0, rop);
	S3C_SETFN1(S3C_FNCOLOR1, rop);
	S3C_SETCOL0(bg);
	S3C_SETCOL1(fg);
}

/*
 *  S3CNAME(StretchBlit)() -- Stretch Blit
 *
 *	This routine will do a stretch blit using the 86c911 blit engine.
 *
 *	Note: This routine is coded in assembly in s3cSBlit.s
 */

void
S3CNAME(StretchBlit)(
	int	srcx,
	int	srcy,
	int	dstx,
	int	dsty,
	int	lx,
	int	ly,
	int	command)
{
	S3C_CLEAR_QUEUE(7);
	S3C_SETX0(srcx);
	S3C_SETY0(srcy);
	S3C_SETX1(dstx);
	S3C_SETY1(dsty);
	S3C_SETLX(lx);
	S3C_SETLY(ly);
	S3C_COMMAND(command);
}

/*
 *  s3cStippledFilledRects() -- Stippled Filled Rectangles
 *
 *	This routine will draw a list of stipple filled rectagles on 
 *	the display in either the 4 or 8 bit display modes.
 *
 */

void
S3CNAME(StippledFillRects)(
	GCPtr 			pGC,
	DrawablePtr		pDraw,
	BoxPtr 			pbox,
	unsigned int 		nbox)
{
	int			stipw;
	int			stiph;
	int			i;
	int			lx;
	int			ly;
	int			biggest_w;
	int			biggest_h;
	int			chunks;
	int			extra_height;
	int			y;
	int			xoff;
	int			yoff;
	int			os_height;
	int			max_height;
	DDXPointPtr 		patOrg;
	PixmapPtr 		pStip;
	BoxRec			os_box;
	unsigned char 		*pimage;
	nfbGCPrivPtr 		pGCPriv = NFB_GC_PRIV(pGC);
	s3cOffScreenBlitArea_t	*off_screen;
	unsigned long int 	fg = pGCPriv->rRop.fg;
	unsigned long int 	planemask = pGCPriv->rRop.planemask;
	unsigned char 		alu = pGCPriv->rRop.alu;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	off_screen = &(S3C_PRIVATE_DATA(pDraw->pScreen))->off_screen_blit_area;
	patOrg = &(pGCPriv->screenPatOrg);

	pStip = pGC->stipple;
	stipw = pStip->drawable.width;
	stiph = pStip->drawable.height;

	/*
	 * if stipple doesn't fit in off screen memory, unlikely but
	 * just in case...
	 */

	if ( stipw * 2 > off_screen->w || stiph * 2 > off_screen->h )
	{
		genStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * find largest rect height and width
	 */

	biggest_w = 0;
	biggest_h = 0;

	for ( i = 0; i < nbox; ++i )
	{
		lx = pbox[i].x2 - pbox[i].x1;
		if (lx > biggest_w)
			biggest_w = lx;
		ly = pbox[i].y2 - pbox[i].y1;
		if (ly > biggest_h)
			biggest_h = ly;
	}

	biggest_w += stipw;
	biggest_h += stiph;

	/*
	 * let gen deal with this, should be rare
	 */

	if (biggest_w > off_screen->w )
	{
		genStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * draw the stipple to offscreen memory
	 */

	pimage = pStip->devPrivate.ptr;
	os_box.x1 = off_screen->x;
	os_box.y1 = off_screen->y;
	os_box.x2 = os_box.x1 + stipw;
	os_box.y2 = os_box.y1 + stiph;

	S3CNAME(DrawOpaqueMonoImage)(&os_box, pimage, 0, pStip->devKind, ~0, 0,
	    	GXcopy, S3C_GENERAL_OS_PLANE, pDraw);

	/*
	 * replicate the stipple in offscreen memory such that it is
	 * at least as wide as the widest rectangle - and is as high
	 * as the highest rectange if possible
	 */

	max_height = off_screen->h - stiph;

	if ( biggest_h > max_height )
	{
		os_height = off_screen->h;
	}
	else
	{
		max_height = biggest_h - stiph;
		os_height = biggest_h;
	}

	os_box.x2 = os_box.x1 + biggest_w;
	os_box.y2 = os_box.y1 + os_height;

	nfbReplicateArea(&os_box, stipw, stiph, S3C_GENERAL_OS_PLANE, pDraw);

	S3CNAME(SetupStretchBlit)(S3CNAME(RasterOps)[alu], fg, 
		S3C_GENERAL_OS_PLANE, planemask);

	for ( i = 0; i < nbox; ++i, ++pbox )
	{
		if ( ( xoff = (pbox->x1 - patOrg->x) % stipw ) < 0 )
			xoff += stipw;

		if ( ( yoff = (pbox->y1 - patOrg->y) % stiph ) < 0 )
			yoff += stiph;
		
		ly = pbox->y2 - pbox->y1;
		lx = pbox->x2 - pbox->x1;

		if ( max_height <= 0 || lx <= 0 )
			break;

		chunks = ly / max_height;
		extra_height = ly % max_height;

		for ( y = 0; y < chunks; ++y )
		{
			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1, 
				pbox->y1 + (y * max_height),
				lx - 1, 
				max_height - 1,
				S3C_BLIT_XP_YP_Y);
		}

		if ( extra_height )
		{
			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1, 
				pbox->y1 + (chunks * max_height),
				lx - 1, 
				extra_height - 1,
				S3C_BLIT_XP_YP_Y);
		}
	}
}


/*
 *  s3cStippledFilledRects16() -- Stippled Filled Rectangles 16
 *
 *	This routine will draw a list of stipple filled rectagles on 
 *	the display in 16 bit display modes.
 *
 */

void
S3CNAME(StippledFillRects16)(
	GCPtr 			pGC,
	DrawablePtr		pDraw,
	BoxPtr 			pbox,
	unsigned int 		nbox)
{
	int			stipw;
	int			stiph;
	int			i;
	int			lx;
	int			ly;
	int			biggest_w;
	int			biggest_h;
	int			chunks;
	int			extra_height;
	int			y;
	int			xoff;
	int			yoff;
	int			os_height;
	int			max_height;
	DDXPointPtr 		patOrg;
	PixmapPtr 		pStip;
	BoxRec			os_box;
	unsigned char 		*pimage;
	nfbGCPrivPtr 		pGCPriv = NFB_GC_PRIV(pGC);
	s3cOffScreenBlitArea_t	*off_screen;
	unsigned long int 	fg = pGCPriv->rRop.fg;
	unsigned long int 	planemask = pGCPriv->rRop.planemask;
	unsigned char 		alu = pGCPriv->rRop.alu;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	off_screen = &(S3C_PRIVATE_DATA(pDraw->pScreen))->off_screen_blit_area;
	patOrg = &(pGCPriv->screenPatOrg);

	pStip = pGC->stipple;
	stipw = pStip->drawable.width;
	stiph = pStip->drawable.height;

	/*
	 * if stipple doesn't fit in off screen memory, unlikely but
	 * just in case...
	 */

	if ( stipw * 2 > off_screen->w || stiph * 2 > off_screen->h )
	{
		genStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * find largest rect height and width
	 */

	biggest_w = 0;
	biggest_h = 0;

	for ( i = 0; i < nbox; ++i )
	{
		lx = pbox[i].x2 - pbox[i].x1;
		if (lx > biggest_w)
			biggest_w = lx;
		ly = pbox[i].y2 - pbox[i].y1;
		if (ly > biggest_h)
			biggest_h = ly;
	}

	biggest_w += stipw;
	biggest_h += stiph;

	/*
	 * let gen deal with this, should be rare
	 */

	if (biggest_w > off_screen->w )
	{
		genStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * draw the stipple to offscreen memory
	 */

	pimage = pStip->devPrivate.ptr;
	os_box.x1 = off_screen->x;
	os_box.y1 = off_screen->y;
	os_box.x2 = os_box.x1 + stipw;
	os_box.y2 = os_box.y1 + stiph;

	S3CNAME(DrawOpaqueMonoImage)(&os_box, pimage, 0, pStip->devKind, ~0, 0,
	    	GXcopy, S3C_GENERAL_OS_PLANE, pDraw);

	/*
	 * replicate the stipple in offscreen memory such that it is
	 * at least as wide as the widest rectangle - and is as high
	 * as the highest rectange if possible
	 */

	max_height = off_screen->h - stiph;

	if ( biggest_h > max_height )
	{
		os_height = off_screen->h;
	}
	else
	{
		max_height = biggest_h - stiph;
		os_height = biggest_h;
	}

	os_box.x2 = os_box.x1 + biggest_w;
	os_box.y2 = os_box.y1 + os_height;

	nfbReplicateArea(&os_box, stipw, stiph, S3C_GENERAL_OS_PLANE, pDraw);

	for ( i = 0; i < nbox; ++i, ++pbox )
	{
		if ( ( xoff = (pbox->x1 - patOrg->x) % stipw ) < 0 )
			xoff += stipw;

		if ( ( yoff = (pbox->y1 - patOrg->y) % stiph ) < 0 )
			yoff += stiph;
		
		ly = pbox->y2 - pbox->y1;
		lx = pbox->x2 - pbox->x1;

		if ( max_height <= 0 || lx <= 0 )
			break;

		chunks = ly / max_height;
		extra_height = ly % max_height;

		for ( y = 0; y < chunks; ++y )
		{
			S3CNAME(SetupStretchBlit)(S3CNAME(RasterOps)[alu], fg, 
				S3C_GENERAL_OS_PLANE, planemask);

			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1, 
				pbox->y1 + (y * max_height),
				lx - 1, 
				max_height - 1,
				S3C_BLIT_XP_YP_Y);

			S3CNAME(SetupStretchBlit)(S3CNAME(RasterOps)[alu], fg >> 8, 
				S3C_GENERAL_OS_PLANE, planemask >> 8);

			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1 + 1024, 
				pbox->y1 + (y * max_height),
				lx - 1, 
				max_height - 1,
				S3C_BLIT_XP_YP_Y);
		}

		if ( extra_height )
		{
			S3CNAME(SetupStretchBlit)(S3CNAME(RasterOps)[alu], fg, 
				S3C_GENERAL_OS_PLANE, planemask);

			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1, 
				pbox->y1 + (chunks * max_height),
				lx - 1, 
				extra_height - 1,
				S3C_BLIT_XP_YP_Y);

			S3CNAME(SetupStretchBlit)(S3CNAME(RasterOps)[alu], fg >> 8, 
				S3C_GENERAL_OS_PLANE, planemask >> 8);

			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1 + 1024, 
				pbox->y1 + (chunks * max_height),
				lx - 1, 
				extra_height - 1,
				S3C_BLIT_XP_YP_Y);
		}
	}
}


/*
 *  s3cOpStippledFilledRects() -- Opaque Stippled Filled Rectangles
 *
 *	This routine will draw a list of opaque stipple filled rectagles
 *	on the display in either the 4 or 8 bit display modes.
 *
 */

void
S3CNAME(OpStippledFillRects)(
	GCPtr 			pGC,
	DrawablePtr 		pDraw,
	BoxPtr 			pbox,
	unsigned int 		nbox )
{
	int			stipw;
	int			stiph;
	int			i;
	int			lx;
	int			ly;
	int			biggest_w;
	int			biggest_h;
	int			chunks;
	int			extra_height;
	int			y;
	int			xoff;
	int			yoff;
	int			os_height;
	int			max_height;
	DDXPointPtr 		patOrg;
	DDXPointRec 		os_point;
	PixmapPtr 		pStip;
	BoxRec 			box;
	BoxRec 			os_box;
	unsigned char 		*pimage;
	nfbGCPrivPtr 		pGCPriv = NFB_GC_PRIV(pGC);
	s3cOffScreenBlitArea_t 	*off_screen;
	unsigned long int 	fg = pGCPriv->rRop.fg;
	unsigned long int 	bg = pGCPriv->rRop.bg;
	unsigned long int 	planemask = pGCPriv->rRop.planemask;
	unsigned char 		alu = pGCPriv->rRop.alu;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	off_screen = &(S3C_PRIVATE_DATA(pDraw->pScreen))->off_screen_blit_area;
	patOrg = &(pGCPriv->screenPatOrg);

	pStip = pGC->stipple;
	stipw = pStip->drawable.width;
	stiph = pStip->drawable.height;

	/*
	 * if stipple doesn't fit in off screen memory, unlikely but
	 * just in case...
	 */

	if ( stipw * 2 > off_screen->w || stiph * 2 > off_screen->h )
	{
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * find largest rect height and width
	 */

	biggest_w = 0;
	biggest_h = 0;

	for ( i = 0; i < nbox; ++i )
	{
		lx = pbox[i].x2 - pbox[i].x1;
		if (lx > biggest_w)
			biggest_w = lx;
		ly = pbox[i].y2 - pbox[i].y1;
		if (ly > biggest_h)
			biggest_h = ly;
	}

	biggest_w += stipw;
	biggest_h += stiph;

	/*
	 * let gen deal with this, should be rare
	 */

	if ( biggest_w > off_screen->w )
	{
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * draw the stipple to offscreen memory
	 */

	pimage = pStip->devPrivate.ptr;
	os_box.x1 = off_screen->x;
	os_box.y1 = off_screen->y;
	os_box.x2 = os_box.x1 + stipw;
	os_box.y2 = os_box.y1 + stiph;

	S3CNAME(DrawOpaqueMonoImage)(&os_box, pimage, 0, pStip->devKind, ~0, 0,
	    GXcopy, S3C_GENERAL_OS_PLANE, pDraw);

	/*
	 * replicate the stipple in offscreen memory such that it is
	 * at least as wide as the widest rectangle - and is as high
	 * as the highest rectange if possible
	 */

	max_height = off_screen->h - stiph;

	if ( biggest_h > max_height )
	{
		os_height = off_screen->h;
	}
	else
	{
		max_height = biggest_h - stiph;
		os_height = biggest_h;
	}

	os_box.x2 = os_box.x1 + biggest_w;
	os_box.y2 = os_box.y1 + os_height;

	nfbReplicateArea(&os_box, stipw, stiph, S3C_GENERAL_OS_PLANE, pDraw);

	s3cSetupOpaqueStretchBlit(S3CNAME(RasterOps)[alu], bg, fg, 
		S3C_GENERAL_OS_PLANE, planemask);

	for ( i = 0; i < nbox; ++i, ++pbox )
	{
		if ( ( xoff = (pbox->x1 - patOrg->x) % stipw ) < 0 )
			xoff += stipw;

		if ( ( yoff = (pbox->y1 - patOrg->y) % stiph ) < 0 )
			yoff += stiph;
		
		ly = pbox->y2 - pbox->y1;
		lx = pbox->x2 - pbox->x1;

		if ( max_height <= 0 || lx <= 0 )
			break;

		chunks = ly / max_height;
		extra_height = ly % max_height;

		for ( y = 0; y < chunks; ++y )
		{
			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1, 
				pbox->y1 + (y * max_height),
				lx - 1, 
				max_height - 1,
				S3C_BLIT_XP_YP_Y);
		}

		if ( extra_height )
		{
			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1, 
				pbox->y1 + (chunks * max_height),
				lx - 1, 
				extra_height - 1,
				S3C_BLIT_XP_YP_Y);
		}
	}
}


/*
 *  s3cOpStippledFilledRects16() -- Opaque Stippled Filled Rectangles 16
 *
 *	This routine will draw a list of opaque stipple filled rectagles
 *	on the display in the 16 bit display modes.
 *
 */

void
S3CNAME(OpStippledFillRects16)(
	GCPtr 			pGC,
	DrawablePtr 		pDraw,
	BoxPtr 			pbox,
	unsigned int 		nbox )
{
	int			stipw;
	int			stiph;
	int			i;
	int			lx;
	int			ly;
	int			biggest_w;
	int			biggest_h;
	int			chunks;
	int			extra_height;
	int			y;
	int			xoff;
	int			yoff;
	int			os_height;
	int			max_height;
	DDXPointPtr 		patOrg;
	DDXPointRec 		os_point;
	PixmapPtr 		pStip;
	BoxRec 			box;
	BoxRec 			os_box;
	unsigned char 		*pimage;
	nfbGCPrivPtr 		pGCPriv = NFB_GC_PRIV(pGC);
	s3cOffScreenBlitArea_t 	*off_screen;
	unsigned long int 	fg = pGCPriv->rRop.fg;
	unsigned long int 	bg = pGCPriv->rRop.bg;
	unsigned long int 	planemask = pGCPriv->rRop.planemask;
	unsigned char 		alu = pGCPriv->rRop.alu;
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pDraw->pScreen);

	S3C_VALIDATE_SCREEN(s3cPriv, pDraw->pScreen, pDraw->pScreen->myNum);
	off_screen = &(S3C_PRIVATE_DATA(pDraw->pScreen))->off_screen_blit_area;
	patOrg = &(pGCPriv->screenPatOrg);

	pStip = pGC->stipple;
	stipw = pStip->drawable.width;
	stiph = pStip->drawable.height;

	/*
	 * if stipple doesn't fit in off screen memory, unlikely but
	 * just in case...
	 */

	if ( stipw * 2 > off_screen->w || stiph * 2 > off_screen->h )
	{
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * find largest rect height and width
	 */

	biggest_w = 0;
	biggest_h = 0;

	for ( i = 0; i < nbox; ++i )
	{
		lx = pbox[i].x2 - pbox[i].x1;
		if (lx > biggest_w)
			biggest_w = lx;
		ly = pbox[i].y2 - pbox[i].y1;
		if (ly > biggest_h)
			biggest_h = ly;
	}

	biggest_w += stipw;
	biggest_h += stiph;

	/*
	 * let gen deal with this, should be rare
	 */

	if ( biggest_w > off_screen->w )
	{
		genOpStippledFillRects(pGC, pDraw, pbox, nbox);
		return;
	}

	/*
	 * draw the stipple to offscreen memory
	 */

	pimage = pStip->devPrivate.ptr;
	os_box.x1 = off_screen->x;
	os_box.y1 = off_screen->y;
	os_box.x2 = os_box.x1 + stipw;
	os_box.y2 = os_box.y1 + stiph;

	S3CNAME(DrawOpaqueMonoImage)(&os_box, pimage, 0, pStip->devKind, ~0, 0,
	    GXcopy, S3C_GENERAL_OS_PLANE, pDraw);

	/*
	 * replicate the stipple in offscreen memory such that it is
	 * at least as wide as the widest rectangle - and is as high
	 * as the highest rectange if possible
	 */

	max_height = off_screen->h - stiph;

	if ( biggest_h > max_height )
	{
		os_height = off_screen->h;
	}
	else
	{
		max_height = biggest_h - stiph;
		os_height = biggest_h;
	}

	os_box.x2 = os_box.x1 + biggest_w;
	os_box.y2 = os_box.y1 + os_height;

	nfbReplicateArea(&os_box, stipw, stiph, S3C_GENERAL_OS_PLANE, pDraw);

	for ( i = 0; i < nbox; ++i, ++pbox )
	{
		if ( ( xoff = (pbox->x1 - patOrg->x) % stipw ) < 0 )
			xoff += stipw;

		if ( ( yoff = (pbox->y1 - patOrg->y) % stiph ) < 0 )
			yoff += stiph;
		
		ly = pbox->y2 - pbox->y1;
		lx = pbox->x2 - pbox->x1;

		if ( max_height <= 0 || lx <= 0 )
			break;

		chunks = ly / max_height;
		extra_height = ly % max_height;

		for ( y = 0; y < chunks; ++y )
		{
			s3cSetupOpaqueStretchBlit(S3CNAME(RasterOps)[alu], 
				bg, fg, 
				S3C_GENERAL_OS_PLANE, planemask);

			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1, 
				pbox->y1 + (y * max_height),
				lx - 1, 
				max_height - 1,
				S3C_BLIT_XP_YP_Y);

			s3cSetupOpaqueStretchBlit(S3CNAME(RasterOps)[alu], 
				bg >> 8, fg >> 8, 
				S3C_GENERAL_OS_PLANE, planemask >> 8);

			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1 + 1024, 
				pbox->y1 + (y * max_height),
				lx - 1, 
				max_height - 1,
				S3C_BLIT_XP_YP_Y);
		}

		if ( extra_height )
		{
			s3cSetupOpaqueStretchBlit(S3CNAME(RasterOps)[alu], 
				bg, fg, 
				S3C_GENERAL_OS_PLANE, planemask);

			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1, 
				pbox->y1 + (chunks * max_height),
				lx - 1, 
				extra_height - 1,
				S3C_BLIT_XP_YP_Y);

			s3cSetupOpaqueStretchBlit(S3CNAME(RasterOps)[alu], 
				bg >> 8, fg >> 8, 
				S3C_GENERAL_OS_PLANE, planemask >> 8);

			S3CNAME(StretchBlit)(
				off_screen->x + xoff,
				off_screen->y + yoff,
				pbox->x1 + 1024, 
				pbox->y1 + (chunks * max_height),
				lx - 1, 
				extra_height - 1,
				S3C_BLIT_XP_YP_Y);
		}
	}
}
