/*
 * @(#) xxxProcs.h 11.1 97/10/22
 *
 * Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
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
 * xxxProcs.h
 *
 * routines for the "xxx" port
 */

/* xxxCmap.c */
extern void
xxxSetColor(
        unsigned int cmap,
        unsigned int index,
        unsigned short r,
        unsigned short g,
        unsigned short b,
        ScreenPtr pScreen);

extern void
xxxLoadColormap(
	ColormapPtr pmap );

/* xxxCursor.c */
extern void
xxxInstallCursor(
        unsigned long int *image,
        unsigned int hotx,
        unsigned int hoty,
        ScreenPtr pScreen);

extern void
xxxSetCursorPos(
        unsigned int x,
        unsigned int y,
        ScreenPtr pScreen);

extern void
xxxCursorOn(
        int on,
        ScreenPtr pScreen);

extern void
xxxSetCursorColor(
        unsigned short fr,
	unsigned short fg,
	unsigned short fb,
	unsigned short br,
	unsigned short bg,
	unsigned short bb,
        ScreenPtr pScreen);

/* xxxGC.c */
extern void
xxxValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

/* xxxImage.c */
extern void
xxxReadImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
xxxDrawImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* xxxInit.c */
extern Bool xxxSetup();

extern Bool xxxInit(
        int index,
        struct _Screen * pScreen,
        int argc,
        char **argv );

extern void
xxxCloseScreen(
        int index,
        ScreenPtr pScreen);

/* xxxScreen.c */
extern Bool
xxxBlankScreen(
        int on,
        ScreenPtr pScreen);

extern void xxxSetGraphics(
	ScreenPtr pScreen );

extern void xxxSetText(
	ScreenPtr pScreen );

extern void xxxSaveGState(
	ScreenPtr pScreen );

extern void xxxRestoreGState(
	ScreenPtr pScreen );

/* xxxWin.c */
extern void xxxValidateWindowPriv(
        struct _Window * pWin );


/* xxxRectOps.c */

extern void xxxCopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void xxxDrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void xxxDrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void xxxTileRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned char *tile,
	unsigned int stride,
	unsigned int w,
	unsigned int h,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

/* xxxMono.c */

extern void xxxDrawMonoImage(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void xxxDrawOpaqueMonoImage(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );


/* xxxFillSp.c */

extern void xxxSolidFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

extern void xxxTiledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void xxxStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void xxxOpStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

/* xxxRect.c */

extern void xxxSolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void xxxTiledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void xxxStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void xxxOpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

/* xxxBres.c */

extern void xxxSolidZeroSegs(
	GCPtr pGC,
	DrawablePtr pDrawable,
	BresLinePtr plines,
	int nlines);

extern void xxxSolidZeroSeg(
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

/* xxxGlyph.c */

extern void xxxDrawMonoGlyphs(
	nfbGlyphInfo *glyph_info,
	unsigned int nglyphs,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	nfbFontPSPtr pPS,
	DrawablePtr pDrawable );

/* xxxFont.c */

extern void xxxDrawFontText(
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

