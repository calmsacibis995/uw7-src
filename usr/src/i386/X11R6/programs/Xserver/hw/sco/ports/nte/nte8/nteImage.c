/*
 *	@(#) nteImage.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 *
 * Modification History
 *
 * S009, 27-Jun-93, hiramc
 *      ready for 864/964 implementations, from DoubleClick
 * S008, 23-Aug-93, staceyc
 * 	mods for 928's with broken reads from pix trans ext, setup as
 *	a flag in grafinfo file
 * S007, 20-Aug-93, staceyc
 * 	move wait for idle check to just before the drawing command
 * S006, 04-Aug-93, staceyc
 * 	added wait for idle check in draw image code, this fixes nasty bug
 *	where, if the graphics engine is busy when you start to download
 *	an image, the event queue gets corrupt(!) - the event queue sits just
 *	up from the framebuffer in virtual memory space, but other than this
 *	factoid I have no idea why the corruption occurs - note that even
 *	though the corruption was occurring the data was still being drawn
 *	correctly, weird
 * S005, 14-Jul-93, staceyc
 * 	clear queue checks for 24 bit modes
 * S004, 13-Jul-93, staceyc
 * 	mods so code can be used for both i/o ports and mem mapped ports
 * S003, 16-Jun-93, staceyc
 * 	don't need to wait for idle for draw image, correctly use data ready
 *	flag in read image
 * S002, 11-Jun-93, staceyc
 * 	fixed problems with dwords, bit fix may require performance improvement
 * S001, 08-Jun-93, staceyc
 * 	initial work, still problems with word/dword alignment, call into S3
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

void 
NTE(ReadImage)(
	BoxPtr pbox,
	void *image,
	unsigned int stride,
	DrawablePtr pDraw)
{
	int height, width, width_bytes, dwords, i;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);
	unsigned char *image_p = image;
	unsigned short *pix_trans16;
	unsigned char *pix_trans;
	unsigned short *image_p16;

	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;
	width_bytes = width * sizeof(ntePixel);
	dwords = (width_bytes >> 2) << 2;

#if ! NTE_MAP_LINEAR

#if ! NTE_USE_IO_PORTS
	pix_trans16 = (unsigned short *)ntePriv->regs;
	pix_trans = (unsigned char *)pix_trans16;
#endif

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(6);
	NTE_CLEAR_QUEUE24(7);
	NTE_FRGD_MIX(NTE_CPU_SOURCE, NTE(RasterOps)[GXcopy]);
	NTE_CURX(pbox->x1);
	NTE_CURY(pbox->y1);
    	NTE_MAJ_AXIS_PCNT(width - 1);
	NTE_MIN_AXIS_PCNT(height - 1);
	NTE_WAIT_FOR_IDLE();
	NTE_CMD(NTE_READ_Z_DATA);
	NTE_WAIT_FOR_DATA();
	NTE_END();

#if ! NTE_USE_IO_PORTS
	if (dwords == width_bytes && ! ntePriv->pix_trans_ext_bug)
		while (height--)
		{
			memcpy(image_p, pix_trans, dwords);
			image_p += stride;
		}
	else
#endif
	{
		unsigned int leftovers, last_index;

		leftovers = width_bytes & 1;
		width_bytes /= 2;
		last_index = width_bytes * 2;
		while (height--)
		{
			image_p16 = (unsigned short *)image_p;
			i = width_bytes;
			while (i--)
				NTE_PIX_TRANS_IN(*image_p16++, *pix_trans16);
			if (leftovers)
				NTE_PIX_TRANS_IN(image_p[last_index],
				    *pix_trans16);
			image_p += stride;
		}
	}
#else /* NTE_MAP_LINEAR */
	{
		unsigned char *pdst;
		ntePixel *psrc;

		/* get pixel address of (x, y) */
		psrc = ((ntePixel *)ntePriv->fbPointer) +
		       (pbox->y1 * ntePriv->fbStride) +
		       pbox->x1;

		pdst = (unsigned char *)image;

		NTE_WAIT_FOR_IDLE();

		while (height--) {
			ntePixel *s, *d;
			int w;

			w = width;
			s = psrc;
			d = (ntePixel *)pdst;
			while (w--) {
				*d++ = *s++;
			}
			psrc += ntePriv->fbStride;	/* pixels */
			pdst += stride;			/* bytes */
		}
	}
#endif /* NTE_MAP_LINEAR */
}

void 
NTE(DrawImage)(
	BoxPtr pbox,
	void *image,
	unsigned int stride,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw)
{
	int height, width, width_bytes, dwords, i;
	unsigned short *pix_trans16;
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pDraw->pScreen);
	unsigned char *image_p = image;
	unsigned short *image_p16;
	unsigned char *pix_trans;

	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;
#if 1

#if ! NTE_USE_IO_PORTS
	pix_trans16 = (unsigned short *)ntePriv->regs;
	pix_trans = (unsigned char *)pix_trans16;
#endif
	width_bytes = width * sizeof(ntePixel);
	dwords = (width_bytes >> 2) << 2;

	NTE_BEGIN(ntePriv->regs);
	NTE_CLEAR_QUEUE(7);
	NTE_CLEAR_QUEUE24(8);
	NTE_WRT_MASK(planemask);
	NTE_FRGD_MIX(NTE_CPU_SOURCE, NTE(RasterOps)[alu]);
	NTE_CURX(pbox->x1);
	NTE_CURY(pbox->y1);
    	NTE_MAJ_AXIS_PCNT(width - 1);
	NTE_MIN_AXIS_PCNT(height - 1);
	NTE_WAIT_FOR_IDLE();
	NTE_CMD(NTE_WRITE_Z_DATA);
	NTE_END();

#if ! NTE_USE_IO_PORTS
	if (dwords == width_bytes)
		while (height--)
		{
			memcpy(pix_trans, image_p, dwords);
			image_p += stride;
		}
	else
#endif
	{
		width_bytes = (width_bytes + 1) / 2;
		while (height--)
		{
			image_p16 = (unsigned short *)image_p;
			i = width_bytes;
			while (i--)
				NTE_PIX_TRANS(*pix_trans16, *image_p16++);
			image_p += stride;
		}
	}
#else /* NTE_MAP_LINEAR */
	{
		unsigned char *psrc;
		ntePixel *pdst;

		/* get pixel address of (x, y) */
		pdst = ((ntePixel *)ntePriv->fbPointer) +
		       (pbox->y1 * ntePriv->fbStride) +
		       pbox->x1;

		psrc = (unsigned char *)image;

		NTE_WAIT_FOR_IDLE();

		while (height--) {
			ntePixel *s, *d;
			int w;

			w = width;
			s = (ntePixel *)psrc;
			d = pdst;
			while (w--) {
				*d++ = *s++;
			}
			pdst += ntePriv->fbStride;	/* pixels */
			psrc += stride;			/* bytes */
		}
	}
#endif /* NTE_MAP_LINEAR */
}
