/*
 *	@(#) nteProcs.h 11.1 97/10/22
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
 * S005, 20-Aug-93, staceyc
 * 	cursor coloring routine added
 * S004, 17-Jun-93, staceyc
 * 	stipples and tiles added
 * S003, 16-Jun-93, staceyc
 * 	glyph, font, and clipping procs added
 * S002, 11-Jun-93, staceyc
 * 	mono image routines added
 * S001, 09-Jun-93, staceyc
 * 	solid rects, blit, points, and bres lines added
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#ifndef NTEPROCS_H
#define NTEPROCS_H

extern void NTE(SetColor)(
	unsigned int cmap,
	unsigned int index,
	unsigned short r,
        unsigned short g,
	unsigned short b,
	ScreenPtr pScreen);

extern void
NTE(InstallCursor)(
        unsigned long int *image,
        unsigned int hotx,
        unsigned int hoty,
        ScreenPtr pScreen);

extern void
NTE(SetCursorPos)(
        unsigned int x,
        unsigned int y,
        ScreenPtr pScreen);

extern void
NTE(CursorOn)(
        int on,
        ScreenPtr pScreen);

extern void
NTE(SetCursorColor)(
        unsigned short fr,
	unsigned short fg,
	unsigned short fb,
	unsigned short br,
	unsigned short bg,
	unsigned short bb,
        ScreenPtr pScreen);

extern void
NTE(ValidateWindowGC)(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

extern void
NTE(ReadImage)(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
NTE(DrawImage)(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

extern Bool NTE(Probe)();

extern Bool NTE(Init)(
        int index,
        struct _Screen * pScreen,
        int argc,
        char **argv );

extern Bool
NTE(FreeScreen)(
        int index,
        ScreenPtr pScreen);

extern Bool
NTE(BlankScreen)(
        int on,
        ScreenPtr pScreen);

extern void NTE(SetGraphics)(
	ScreenPtr pScreen );

extern void NTE(SetText)(
	ScreenPtr pScreen );

extern void NTE(SaveGState)(
	ScreenPtr pScreen );

extern void NTE(RestoreGState)(
	ScreenPtr pScreen );

extern void NTE(ValidateWindowPriv)(
        struct _Window * pWin );

extern void NTE(CopyRect)(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void NTE(DrawSolidRects)(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void NTE(DrawPoints)(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void NTE(SolidZeroSeg)(
	GCPtr pGC,
	DrawablePtr pDraw,
	int signdx,
	int signdy,
	int axis,
	int x1,
	int y1,
	int e,
	int e1,
	int e2,
	int len);

extern void NTE(DrawMonoImage)(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void NTE(DrawOpaqueMonoImage)(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void NTE(DrawMonoGlyphs)(
	nfbGlyphInfo *glyph_info,
	unsigned int nglyphs,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	nfbFontPSPtr pPS,
	DrawablePtr pDraw);

extern void NTE(SetClipRegions)(
	BoxRec *pbox,
	int nbox,
	DrawablePtr pDraw);

extern void NTE(DownloadFont8)(
	unsigned char **bits, 
	int count, 
	int width, 
	int height, 
	int stride,
	int index,
	ScreenRec *pScreen);

extern void NTE(DrawFontText)(
	BoxPtr pbox,
	unsigned char *chars,
	unsigned int count,
	unsigned short glyph_width,
	int index,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
	DrawablePtr pDraw);

extern void NTE(TileRects)(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned char *tile,
	unsigned int stride,
	unsigned int w,
	unsigned int h,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void NTE(TileAreaSlow)(
	BoxRec *pbox,
	DDXPointRec *src,
	int max_height,
	int max_width,
	unsigned char alu,
	unsigned long planemask,
	DrawableRec *pDraw);

extern void NTE(OpStippledFillRects)(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern void NTE(StippledFillRects)(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

#if NTE_BITS_PER_PIXEL == 8
extern void NTE(ColorCursor)(ScreenPtr pScreen);
#endif

#endif
