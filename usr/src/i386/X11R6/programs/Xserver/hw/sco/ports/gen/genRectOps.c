/*
 * @(#) genRectOps.c 11.1 97/10/22
 *
 * Copyright (C) 1992 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */

/*
 * low level drawing primitives dependent upon xxxDrawImage 
 * and xxxReadImage
 *
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"		/* for BITMAP_BIT_ORDER */

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"

#include "genDefs.h"
#include "genProcs.h"

/*
 * genCopyRect - should work for 1, 8, 16, and 32 bits-per-pixel
 */
void
genCopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable)
{
	unsigned char *image;
	BoxRec srcbox;
	BoxRec dstbox;
	register int yend;
	register int incr;
	int width = pdstBox->x2 - pdstBox->x1;
	int height = pdstBox->y2 - pdstBox->y1;
	int log2pix;
	void (*ReadImage)();
	void (*DrawImage)();

#ifdef DEBUG_PRINT
	ErrorF("genCopyRect(pdstBox={%d,%d,%d,%d}, psrc={%d,%d}, alu=%d, planemask=0x%x)\n", pdstBox->x1, pdstBox->y1, pdstBox->x2, pdstBox->y2, psrc->x, psrc->y, alu, planemask);
#endif /* DEBUG_PRINT */

	log2pix = pDrawable->bitsPerPixel >> 4;

	/*
	 * for 1-bit pixels this allocates more than is needed
	 */
	image = (unsigned char *) ALLOCATE_LOCAL( width << log2pix );

	ReadImage = (void (*)())NFB_WINDOW_PRIV( pDrawable )->ops->ReadImage;
	DrawImage = (void (*)())NFB_WINDOW_PRIV( pDrawable )->ops->DrawImage;

	srcbox.x1 = psrc->x;
	srcbox.x2 = psrc->x + width;

	dstbox.x1 = pdstBox->x1;
	dstbox.x2 = pdstBox->x2;

	if ( pdstBox->y1 > psrc->y ) {
		yend = psrc->y - 1;
		srcbox.y1 = psrc->y + height - 1;
		dstbox.y1 = pdstBox->y2 - 1;
		incr = -1;
	} else {
		yend = psrc->y + height;
		srcbox.y1 = psrc->y;
		dstbox.y1 = pdstBox->y1;
		incr = 1;
	}

	do {
		srcbox.y2 = srcbox.y1 + 1;
		dstbox.y2 = dstbox.y1 + 1;
		(*ReadImage)( &srcbox, image, width, pDrawable );
		(*DrawImage)( &dstbox, image, width, alu, planemask, pDrawable );
		srcbox.y1 += incr;
		dstbox.y1 += incr;
	} while ( yend != srcbox.y1 );

	DEALLOCATE_LOCAL( image );

	return;
}


/*
 * genDrawPoints - should work for 1, 8, 16, and 32 bits-per-pixel 
 */
void
genDrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable)
{
	register BoxPtr pbox;
	register int nbox;
	register int nptsInit = npts;
	BoxRec singleBox;
	BoxPtr pboxInit = &singleBox;

	if (nptsInit > 1)
		pboxInit = (BoxPtr) ALLOCATE_LOCAL(npts * sizeof(BoxRec));
	pbox = pboxInit;
	nbox = nptsInit;

	while ( npts-- ) {
		pbox->x1 = ppt->x;
		pbox->y1 = ppt->y;
		pbox->x2 = ppt->x + 1;
		pbox->y2 = ppt->y + 1;
		ppt++;
		pbox++;
	}

	( *( NFB_WINDOW_PRIV( pDrawable ) )->ops->DrawSolidRects )
			( pboxInit, nbox, fg, alu, planemask, pDrawable );

	if (nptsInit > 1)
		DEALLOCATE_LOCAL(pboxInit);
	return;
}

/*
 * genDrawSolidRects - should work for 1, 8, 16, and 32 bits-per-pixel
 */
void
genDrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable)
{

	BoxRec box;
	register unsigned char *image;
	int y;
	int yend;
	int maxwidth;
	register int i;

#ifdef DEBUG_PRINT
	ErrorF("genDrawSolidRects(pbox=0x%x, nbox=%d, fg=%d, alu=%d, planemask=%d)\n", pbox, nbox, fg, alu, planemask);
#endif /* DEBUG_PRINT */

	maxwidth = pDrawable->pScreen->width;

	switch ( pDrawable->bitsPerPixel ) {
	    case 1:
		i = (maxwidth + 7) >> 3;
		image = (unsigned char *) ALLOCATE_LOCAL( i );
		(void) memset( image, fg ? 0xFF : 0x00, i );
		break;
	    case 8:
		image = (unsigned char *) ALLOCATE_LOCAL( maxwidth );
		(void) memset( image, fg, maxwidth );
		break;
	    case 16:
		image = (unsigned char *) ALLOCATE_LOCAL( maxwidth << 1 );
		for (i = 0; i < maxwidth; ++i)
			((unsigned short *)image) [i] = fg;
		break;
	    case 32:
		image = (unsigned char *) ALLOCATE_LOCAL( maxwidth << 2 );
		for (i = 0; i < maxwidth; ++i)
			((unsigned long *)image) [i] = fg;
		break;
	}

	do {
		box.x1 = pbox->x1;
		box.x2 = pbox->x2;
		for (box.y1 = pbox->y1, yend = pbox->y2; box.y1 < yend; box.y1++) {
			box.y2 = box.y1 + 1;
			( *(NFB_WINDOW_PRIV(pDrawable))->ops->DrawImage )
				( &box, image, box.x2 - box.x1, alu, planemask, pDrawable );
		}
		pbox++;
	} while (--nbox);
	DEALLOCATE_LOCAL(image);
	return;
}

/*
 * genTileRects - should work for 1, 8, 16, and 32 bits-per-pixel
 */
void
genTileRects(
	BoxPtr pbox,
	unsigned int nbox,
	void *tile_src,
	unsigned int stride,
	unsigned int w,
	unsigned int h,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable )
{
    unsigned char *tile = (unsigned char*)tile_src;
    register int xoff, yoff;
    BoxRec box;
    void (* DrawImage)();
    void (* DrawSolidRects)();
    void (* DrawOpaqueMonoImage)();

    int log2pix;

    #ifdef DEBUG_PRINT
    ErrorF("genTileRects(pbox=0x%x, nbox=%d, tile=0x%x, stride=%d, w=%d, h=%d, patOrg={%d,%d}, alu=%d, planemask=0x%x)\n", pbox, nbox, tile, stride, w, h, patOrg->x, patOrg->y, alu, planemask);
    #endif /* DEBUG_PRINT */

    DrawImage = (void (*)())(NFB_WINDOW_PRIV(pDrawable))->ops->DrawImage;
    DrawSolidRects = (void (*)())(NFB_WINDOW_PRIV(pDrawable))->ops->DrawSolidRects;
    if ( pDrawable->bitsPerPixel == 1 )
	DrawOpaqueMonoImage = (void (*)())
		(NFB_WINDOW_PRIV(pDrawable))->ops->DrawOpaqueMonoImage;

    log2pix = pDrawable->bitsPerPixel >> 4;
    switch ( alu )
    {
	case GXnoop:
	    break;
	case GXclear:
	    (*DrawSolidRects)(pbox, nbox, 0, GXcopy, planemask, pDrawable);
	    break;
	case GXset:
	    (*DrawSolidRects)(pbox, nbox, ~0, GXcopy, planemask, pDrawable);
	    break;
	case GXinvert:
	    (*DrawSolidRects)(pbox, nbox, ~0, GXinvert, planemask, pDrawable);
	    break;
	case GXcopy:
	case GXcopyInverted:

	    while ( nbox-- )
	    {
		int boxtmp;

		/*
		 * Be careful here.
		 * Since (pbox - patOrg) may be negative,
		 * the modulo operations _must_ be
		 * signed, not unsigned.
		 */
		xoff =  ( pbox->x1 - patOrg->x ) % (int) w;
		if ( xoff < 0 ) 
		    xoff += w;
		xoff = w - xoff;
		yoff =  ( pbox->y1 - patOrg->y ) % (int) h;
		if ( yoff < 0 ) 
		    yoff += h;
		yoff = h - yoff;

		/*
		 * use boxtmp to avoid overflow of the
		 * shorts box.x2 and box.y2
		 */
		box.x1 = pbox->x1;
		boxtmp = box.x1 + xoff;

		if ( boxtmp > pbox->x2 )
		    box.x2 = pbox->x2;
		else
		    box.x2 = boxtmp;

		box.y1 = pbox->y1;
		boxtmp = box.y1 + yoff;

		if ( boxtmp > pbox->y2 )
		    box.y2 = pbox->y2;
		else
		    box.y2 = boxtmp;

		if (yoff && xoff)
		{
		    if ( pDrawable->bitsPerPixel == 1 )
			(* DrawOpaqueMonoImage)(&box, 
			    tile + (( w - xoff ) >> 3 ) + ( h - yoff ) * stride,
			    ( w - xoff ) & 7, stride, 1, 0, alu, planemask,
			    pDrawable);
		    else
			(* DrawImage)(&box, tile + 
			    ( ( w - xoff ) << log2pix ) + ( h - yoff ) * stride,
			    stride, alu, planemask, pDrawable);

		}
		
		box.y1 = pbox->y1 + yoff;
		box.y2 = pbox->y1 + h;

		if (xoff && yoff != h && box.y1 < pbox->y2 )
		{
		    if ( box.y2 > pbox->y2 )
			box.y2 = pbox->y2;

		    if ( pDrawable->bitsPerPixel == 1 )
			(* DrawOpaqueMonoImage)(&box,
			    tile + ( (w - xoff) >> 3 ), (w - xoff) & 7, 
			    stride, 1, 0, alu, planemask,
			    pDrawable);
		    else
			(* DrawImage)(&box,
			    tile + ( (w - xoff) << log2pix ),
			    stride, alu, planemask, pDrawable);

		    box.x1 = pbox->x1 + xoff;
		    box.x2 = pbox->x1 + w;

		    if ( box.x2 > pbox->x2 )
			box.x2 = pbox->x2;

		    if ( xoff != w && box.x1 < pbox->x2 )
			(* DrawImage)(&box, tile, stride, alu, planemask, 
			    pDrawable);

		}
		else
		{
		    box.x1 = pbox->x1 + xoff;
		    box.x2 = pbox->x1 + w;
		    if ( box.x2 > pbox->x2 )
			box.x2 = pbox->x2;
		}

		if ( yoff && xoff != w && box.x1 < pbox->x2 )
		{
		    box.y1 = pbox->y1;
		    box.y2 = box.y1 + yoff;

		    if( box.y2 > pbox->y2 )
			box.y2 = pbox->y2;

		    (* DrawImage)(&box, tile + ( h - yoff ) * stride, stride, 
			    alu, planemask, pDrawable);
		}

		nfbReplicateArea(pbox, w, h, planemask, pDrawable);
		pbox++;
	    }
	    break;
	/* all the alu's that involve src and dst */
	default:

	    while(nbox--)
	    {

		box.x1 = pbox->x1;
		box.y1 = pbox->y1;

		xoff = ( box.x1 - patOrg->x ) % w;
		if ( xoff < 0 )
		    xoff += w;

		yoff = ( box.y1 - patOrg->y ) % h;
		if ( yoff < 0 )
		    yoff += h;

		box.x2 = min( box.x1 + w - xoff, pbox->x2 );
		box.y2 = min( box.y1 + h - yoff, pbox->y2 );

		if (w != xoff && h != yoff)
		    {
		    if ( pDrawable->bitsPerPixel == 1 )
			(* DrawOpaqueMonoImage)( &box,
			    tile + yoff * stride + ( xoff >> 3 ), xoff & 7,
			    stride, 1, 0, alu, planemask, pDrawable );
		    else
			(* DrawImage)( &box, 
			    tile + yoff * stride + ( xoff << log2pix ), 
			    stride, alu, planemask, pDrawable );
		    }

		while ( box.x2 < pbox->x2 )
		{
		    box.x1 = box.x2;
		    box.x2 += w;
		    if ( box.x2 > pbox->x2 )
			    box.x2 = pbox->x2;

		    (* DrawImage)( &box, tile + yoff * stride, stride,
			    alu, planemask, pDrawable );
		}

		while ( box.y2 < pbox->y2 )
		{
		    box.y1 = box.y2;
		    box.y2 += h;
		    if ( box.y2 > pbox->y2 )
			    box.y2 = pbox->y2;

		    box.x1 = pbox->x1;
		    box.x2 = min( box.x1 + w - xoff, pbox->x2 );

		    if (w != xoff)
			{
			if ( pDrawable->bitsPerPixel == 1 )
			    (* DrawOpaqueMonoImage)(&box,
				    tile + ( xoff >> 3 ), xoff & 7, stride,
				    1, 0, alu, planemask, pDrawable );
			else
			    (* DrawImage)( &box, tile + ( xoff << log2pix ),
				    stride, alu, planemask, pDrawable );
			}

		    while ( box.x2 < pbox->x2 )
		    {
			box.x1 = box.x2;
			box.x2 += w;
			if ( box.x2 > pbox->x2 )
				box.x2 = pbox->x2;
			(* DrawImage)( &box, tile, stride, alu, planemask,
				pDrawable );
		    }
		}

		pbox++;
	    }
	break;
    }
    return;
}
