/*
 * @(#)svgaProcs.h 11.1
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
 * svgaProcs.h
 *
 * routines for the "svga" port
 */

/* svgaCmap.c */
extern void
svgaSetColor(
        unsigned int cmap,
        unsigned int index,
        unsigned short r,
        unsigned short g,
        unsigned short b,
        ScreenPtr pScreen);

extern void
svgaLoadColormap(
	ColormapPtr pmap );

/* svgaCursor.c */
extern void
svgaInstallCursor(
        unsigned long int *image,
        unsigned int hotx,
        unsigned int hoty,
        ScreenPtr pScreen);

extern void
svgaSetCursorPos(
        unsigned int x,
        unsigned int y,
        ScreenPtr pScreen);

extern void
svgaCursorOn(
        int on,
        ScreenPtr pScreen);

extern void
svgaSetCursorColor(
        unsigned short fr,
	unsigned short fg,
	unsigned short fb,
	unsigned short br,
	unsigned short bg,
	unsigned short bb,
        ScreenPtr pScreen);

/* svgaGC.c */
extern void
svgaValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

/* svgaImage.c */
extern void
svgaReadImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
svgaDrawImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* svgaImage16.c */
extern void
svgaReadImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
svgaDrawImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* svgaInit.c */
extern Bool
svgaSetup();

extern Bool
svgaInit(
        int index,
        struct _Screen * pScreen,
        int argc,
        char **argv );

extern void
svgaCloseScreen(
        int index,
        ScreenPtr pScreen);

/* svgaScreen.c */
extern Bool
svgaBlankScreen(
        int on,
        ScreenPtr pScreen);

extern void svgaSetGraphics(
	ScreenPtr pScreen );

extern void svgaSetText(
	ScreenPtr pScreen );

extern void svgaSaveGState(
	ScreenPtr pScreen );

extern void svgaRestoreGState(
	ScreenPtr pScreen );

/* svgaWin.c */
extern void svgaValidateWindowPriv(
        struct _Window * pWin );


/* svgaRectOps.c */

extern void svgaCopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void svgaDrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void svgaDrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void svgaTileRects(
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

/* svgaMono.c */

extern void svgaDrawMonoImage(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void svgaDrawOpaqueMonoImage(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );


/* svgaFillSp.c */

extern void svgaSolidFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

extern void svgaTiledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void svgaStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void svgaOpStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

/* svgaRect.c */

extern void svgaSolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void svgaTiledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void svgaStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void svgaOpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

/* svgaBres.c */

extern void svgaSolidZeroSegs(
	GCPtr pGC,
	DrawablePtr pDrawable,
	BresLinePtr plines,
	int nlines);

extern void svgaSolidZeroSeg(
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

/* svgaGlyph.c */

extern void svgaDrawMonoGlyphs(
	nfbGlyphInfo *glyph_info,
	unsigned int nglyphs,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	nfbFontPSPtr pPS,
	DrawablePtr pDrawable );

/* svgaFont.c */

extern void svgaDrawFontText(
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

