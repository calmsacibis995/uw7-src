/*
 *	@(#)ctRectOps.c	11.1	10/22/97	12:10:54
 *	@(#) ctRectOps.c 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1994-1996.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */
/*
 *	SCO Modifications
 *
 *	S001	Wed Jun 01 10:34:01 PDT 1994	hiramc@sco.COM
 *	- Must use a simple char * for Microsoft inlined _asm code.
 */

#ident "@(#) $Id$"

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbRop.h"

#include "ctDefs.h"
#include "ctMacros.h"
#include "ctOnboard.h"

extern void *CT(ImageCacheLoad)();

/*******************************************************************************

				Private Routines

*******************************************************************************/

#define	InvertWithMask(dst, mask)	((dst) ^ (mask))

static void
ctDrawPointsFB(ppt, npts, fg, alu, planemask, pDraw)
DDXPointPtr ppt;
unsigned int npts;
unsigned long fg;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	CT_PIXEL *pdst;
	char * fbPtr = ctPriv->fbPointer;		/*	S001	*/

#ifdef DEBUG_PRINT
	ErrorF("DrawPointsFB(): ppt=0x%08x npts=%d fg=0x%08x alu=%d\n",
		ppt, npts, fg, alu);
#endif

	planemask &= ctPriv->allPlanes;

	if (planemask == (unsigned long)0x00000000L) {
		/*
		 * No planes.
		 */
		return;
	}

	CT_WAIT_FOR_IDLE();

	switch (alu) {
	case GXclear:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = ClearWithMask(*pdst, planemask);
			ppt++;
		}
		break;
	case GXand:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnAND(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXandReverse:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnANDREVERSE(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXcopy:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnCOPY(fg,0), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXandInverted:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnANDINVERTED(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXnoop:
		break;
	case GXxor:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnXOR(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXor:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnOR(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXnor:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnNOR(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXequiv:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnEQUIV(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXinvert:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = InvertWithMask(*pdst, planemask);
			ppt++;
		}
		break;
	case GXorReverse:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnORREVERSE(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXcopyInverted:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnCOPYINVERTED(fg,0), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXorInverted:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnORINVERTED(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXnand:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = RopWithMask(fnNAND(fg, *pdst), *pdst,
					planemask);
			ppt++;
		}
		break;
	case GXset:
		while (npts--) {
			pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
			*pdst = SetWithMask(*pdst, planemask);
			ppt++;
		}
		break;
	}

	CT_FLUSH_BITBLT(fbPtr);		/*	S001	*/
}

/*******************************************************************************

				Public Routines

*******************************************************************************/

void
CT(CopyRect)(pdstBox, psrc, alu, planemask, pDraw)
BoxPtr pdstBox;
DDXPointPtr psrc;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	unsigned long control;
	int srcx, srcy, dstx, dsty, height, width;

	if (alu == GXnoop)
		return;

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		genCopyRect(pdstBox, psrc, alu, planemask, pDraw);
		return;
	}

#ifdef DEBUG_PRINT
	CT(BitBltDebugF)("CopyRect(): dst=(%d,%d)-(%d,%d) src=(%d,%d) alu=%d\n",
		pdstBox->x1, pdstBox->y1, pdstBox->x2, pdstBox->y2,
		psrc->x, psrc->y, alu);
#endif

	width = pdstBox->x2 - pdstBox->x1;
	height = pdstBox->y2 - pdstBox->y1;

	/*
	 * Set BitBlt control register to copy using mapped alu, from screen to
	 * screen. Calculate the copy direction based on src and dst position.
	 */
	if ((psrc->y < pdstBox->y1) && ((psrc->y + height) > pdstBox->y1)) {
		/*
		 * Source is above the destination and the two overlap. Copy
		 * bottom to top, left to right.
		 */
		control = (CT(PixelOps)[alu] | CT_XINCBIT);
		srcx = psrc->x;
		srcy = psrc->y + height - 1;
		dstx = pdstBox->x1;
		dsty = pdstBox->y2 - 1;
	} else if ((psrc->y == pdstBox->y1) &&
	    (psrc->x < pdstBox->x1) && ((psrc->x + width) > pdstBox->x1)) {
		/*
		 * Source is directly to the left of the destination and the two
		 * overlap. Copy top to bottom, right to left.
		 */
		control = (CT(PixelOps)[alu] | CT_YINCBIT);
		srcx = psrc->x + width - 1;
		srcy = psrc->y;
		dstx = pdstBox->x2 - 1;
		dsty = pdstBox->y1;
	} else {
		/*
		 * Copy top to bottom, left to right.
		 */
		control = (CT(PixelOps)[alu] | CT_YINCBIT | CT_XINCBIT);
		srcx = psrc->x;
		srcy = psrc->y;
		dstx = pdstBox->x1;
		dsty = pdstBox->y1;
	}

	CT_WAIT_FOR_IDLE();
	CT_SET_BLTCTRL(control);
	CT_SET_BLTOFFSET(ctPriv->bltStride, ctPriv->bltStride);
	CT_SET_BLTSRC(CT_SCREEN_OFFSET(ctPriv, srcx, srcy));
	CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, dstx, dsty));
	CT_START_BITBLT(width, height);
}

void
CT(DrawPoints)(ppt, npts, fg, alu, planemask, pDraw)
DDXPointPtr ppt;
unsigned int npts;
unsigned long fg;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	CT_PIXEL *pdst;
	char * fbPtr = ctPriv->fbPointer;		/*	S001	*/

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		/*
		 * Call the frame buffer routine to handle planemask.
		 */
		ctDrawPointsFB(ppt, npts, fg, alu, planemask, pDraw);
		return;
	}

	switch (alu) {
	case GXnoop:
		return;
	case GXclear:
		/* implicit alu = GXcopy */
		fg = 0L;
		break;
	case GXset:
		/* implicit alu = GXcopy */
		fg = ~0L;
		break;
	case GXcopy:
		break;
	default:
		/*
		 * Call the frame buffer routine to handle non-copy ALU's.
		 */
		ctDrawPointsFB(ppt, npts, fg, alu, ~0L, pDraw);
		return;
	}

#ifdef DEBUG_PRINT
	ErrorF("DrawPoints(): ppt=0x%08x npts=%d fg=0x%08x alu=%d\n",
		ppt, npts, fg, alu);
#endif

	CT_WAIT_FOR_IDLE();

	while (npts--) {
		pdst = CT_SCREEN_ADDRESS(ctPriv, ppt->x, ppt->y);
		*pdst = (CT_PIXEL)fg;
		ppt++;
	}

	CT_FLUSH_BITBLT(fbPtr);		/*	S001	*/
}

#if (CT_BITS_PER_PIXEL == 8)  || (CT_BITS_PER_PIXEL == 16)
void
CT(DrawSolidRects)(pbox, nbox, fg, alu, planemask, pDraw)
BoxPtr pbox;
unsigned int nbox;
unsigned long fg;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);

	if (alu == GXnoop)
		return;

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		genDrawSolidRects(pbox, nbox, fg, alu, planemask,
				pDraw);
		return;
	}

#ifdef DEBUG_PRINT
	CT(BitBltDebugF)("DrawSolidRects(): pbox=0x%08x nbox=%d fg=0x%08x alu=%d\n",
		pbox, nbox, fg, alu, planemask);
#endif /* DEBUG_PRINT */

	CT_WAIT_FOR_IDLE();

	/*
	 * Set BitBlt control register to copy using mapped alu, left-to-right,
	 * top-to-bottom, using fg_color.
	 */
	CT_SET_BLTCTRL(CT(PixelOps)[alu] | CT_XINCBIT | CT_YINCBIT |
			CT_FGCOLORBIT);
	CT_SET_BLTOFFSET(0, ctPriv->bltStride);
	CT_SET_BLTFGCOLOR(fg);

	while (nbox--) {
		CT_WAIT_FOR_IDLE();
		CT_SET_BLTDST(CT_SCREEN_OFFSET(ctPriv, pbox->x1, pbox->y1));
		CT_START_BITBLT(pbox->x2 - pbox->x1, pbox->y2 - pbox->y1);
		pbox++;
	}
}
#endif /* (CT_BITS_PER_PIXEL == 8)  || (CT_BITS_PER_PIXEL == 16) */
#if (CT_BITS_PER_PIXEL == 24)
void
CT(DrawSolidRects)(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	/*
	 * Can't draw solid colors using the BitBlt engine in 24-bit mode.
	 */
	ErrorF("DrawSolidRects(): *** NOT IMPLEMENTED IN 24-BIT MODE ***\n");
}
#endif /* (CT_BITS_PER_PIXEL == 24) */

void
CT(TileRects)(pbox, nbox, tile, stride, w, h, patOrg, alu, planemask, pDraw)
BoxPtr pbox;
unsigned int nbox;
unsigned char *tile;
unsigned int stride;
unsigned int w;
unsigned int h;
DDXPointPtr patOrg;
unsigned char alu;
unsigned long planemask;
DrawablePtr pDraw;
{
	CT(PrivatePtr) ctPriv = CT_PRIVATE_DATA(pDraw->pScreen);
	unsigned int width, height;
	void *chunk;

	if ((planemask & ctPriv->allPlanes) != ctPriv->allPlanes) {
		genTileRects(pbox, nbox, tile, stride, w, h, patOrg, alu,
				planemask, pDraw);
		return;
	}

	switch (alu) {
	case GXnoop:
		return;
	case GXclear:
	case GXset:
	case GXinvert:
		/*
		 * The specified ALU ignores both tile and foreground values.
		 */
		CT(DrawSolidRects)(pbox, nbox, 0L, alu, planemask, pDraw);
		return;
	default:
		break;
	}

#ifdef DEBUG_PRINT
	ErrorF("TileRects(): tile=0x%08x stride=%d w=%d h=%d o=(%d,%d)\n",
		tile, stride, w, h, patOrg->x, patOrg->y);
#endif /* DEBUG_PRINT */

	width = w;
	height = h;
	chunk = CT(ImageCacheLoad)(pDraw->pScreen, tile, stride,
				CT_BITS_PER_PIXEL, &width, &height, 0L, 0L);
	if (chunk == (void *)0) {
		/*
		 * Failed to allocate offscreen memory.
		 */
		genTileRects(pbox, nbox, tile, stride, w, h, patOrg, alu,
				planemask, pDraw);
		return;
	}

	CT(ImageCacheCopyRects)(pbox, nbox, chunk, width, height,
				patOrg, alu, planemask, pDraw);

	CT(ImageCacheFree)(pDraw->pScreen, chunk);
}
