/*
 * @(#) genProcs.h 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
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
 * genProcs.h
 *
 * routines for the "gen" porting layer
 */

/* genGC.c */

extern void genGCOpsCacheInit();

extern void genGCOpsCacheReset();

extern void genHelpValidateGC(
        GCPtr pGC,
        unsigned long changes,
        DrawablePtr pDraw );

/* genRectOps.c */

extern void genCopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void genDrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void genDrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void genTileRects(
	BoxPtr pbox,
	unsigned int nbox,
	void *tile_src,
	unsigned int stride,
	unsigned int w,
	unsigned int h,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

/* genMono.c */

extern void genDrawMonoImage(
	BoxPtr pbox,
	void *image_src,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void genDrawOpaqueMonoImage(
	BoxPtr pbox,
	void *image_src,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );


/* genVisual.c */

extern void genValidateVisual( 
	struct _Screen * pScreen, 
	int did, 
	struct _Region * regions );

/* genCmap.c */

extern void genLoadColormap( ColormapPtr pmap );

/* genFillSp.c */

extern void genSolidFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

extern void genTiledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void genStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void genOpStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

/* genRect.c */

extern void genSolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void genTiledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void genStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void genOpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

/* genBres.c */

extern void genSolidZeroSegs(
	GCPtr pGC,
	DrawablePtr pDrawable,
	BresLinePtr plines,
	int nlines);

extern void genSolidZeroSeg(
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
	int len ) ;

/* genGlyph.c */

extern void genDrawMonoGlyphs(
	nfbGlyphInfo *glyph_info,
	unsigned int nglyphs,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	nfbFontPSPtr pPS,
	DrawablePtr pDrawable );

/* genFont.c */

extern void genDrawFontText(
	BoxPtr pbox,
	unsigned char *chars,
	unsigned int count,
	nfbFontPSPtr pPS,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
	DrawablePtr pDraw );

/* genStubs.c */

extern void genSolidGCOp4();
extern void genSolidGCOp5();
extern void genSolidGCOp6();
extern void genTiledGCOp3();
extern void genTiledGCOp4();
extern void genTiledGCOp5();
extern void genTiledGCOp6();
extern void genStippledGCOp3();
extern void genStippledGCOp4();
extern void genStippledGCOp5();
extern void genStippledGCOp6();
extern void genOpStippledGCOp3();
extern void genOpStippledGCOp4();
extern void genOpStippledGCOp5();
extern void genOpStippledGCOp6();
extern void genWinOp10();
extern void genWinOp11();
extern void genWinOp12();
extern void genWinOp13();
extern void genWinOp14();

/* micursor.c */

extern void miRecolorCursor(
	ScreenPtr   pScr,
	CursorPtr   pCurs,
	Bool        displayed);

