/*
 * @(#) mgaProcs.h 11.1 97/10/22
 *
 * Copyright (C) 1994-1995 The Santa Cruz Operation, Inc.
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
 *	SCO Modifications
 *
 *	S002	Thu Jun  1 16:56:14 PDT 1995	brianm@sco.com
 *		Merged in new code from Matrox.
 *	S001	Wed May 25 16:54:49 PDT 1994	hiramc@sco.COM
 *		Must have mgaSolidZeroSeg for Tbird,
 *		can use mgaSolidZeroSegs for everest.
 */

/*
 * mgaProcs.h
 *
 * routines for the "mga" port
 */

/* mgaCmap.c */
extern void
mgaSetColor(
        unsigned int cmap,
        unsigned int index,
        unsigned short r,
        unsigned short g,
        unsigned short b,
        ScreenPtr pScreen);

extern void
mgaLoadColormap(
	ColormapPtr pmap );

/* mgaCursor.c */
extern void
mgaInstallCursor(
        unsigned long int *image,
        unsigned int hotx,
        unsigned int hoty,
        ScreenPtr pScreen);

extern void
mgaSetCursorPos(
        unsigned int x,
        unsigned int y,
        ScreenPtr pScreen);

extern void
mgaCursorOn(
        int on,
        ScreenPtr pScreen);

extern void
mgaSetCursorColor(
        unsigned short fr,
	unsigned short fg,
	unsigned short fb,
	unsigned short br,
	unsigned short bg,
	unsigned short bb,
        ScreenPtr pScreen);

/* mgaGC.c */
extern void
mgaValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

/* mgaImage.c */
extern void
mgaReadImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
mgaDrawImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* mgaInit.c */
extern Bool mgaProbe();

extern Bool mgaInit(
        int index,
        struct _Screen * pScreen,
        int argc,
        char **argv );

extern Bool
mgaFreeScreen(
        int index,
        ScreenPtr pScreen);

/* mgaScreen.c */
extern void mgaSetClipRegions(
	BoxRec *pbox,
	int nbox,
	DrawablePtr pDraw);

extern Bool
mgaBlankScreen(
        int on,
        ScreenPtr pScreen);

extern void mgaSetGraphics(
	ScreenPtr pScreen );

extern void mgaSetText(
	ScreenPtr pScreen );

extern void mgaSaveGState(
	ScreenPtr pScreen );

extern void mgaRestoreGState(
	ScreenPtr pScreen );

/* mgaWin.c */
extern void mgaValidateWindowPriv(
        struct _Window * pWin );


/* mgaRectOps.c */

extern void mgaCopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void mgaDrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void mgaDrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void mgaTileRects(
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

/* mgaMono.c */

extern void mgaDrawMonoImage(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );

extern void mgaDrawOpaqueMonoImage(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDrawable );


/* mgaFillSp.c */

extern void mgaSolidFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

extern void mgaTiledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void mgaStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void mgaOpStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

/* mgaRect.c */

extern void mgaSolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void mgaTiledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void mgaStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void mgaOpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

/* mgaBres.c */

#ifdef	agaII

extern void mgaSolidZeroSeg(
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
	int len );

#else

extern void mgaSolidZeroSegs(
	GCPtr pGC,
	DrawablePtr pDrawable,
	BresLinePtr plines,
	int nlines);

#endif

extern void mgaSolidZeroSeg(
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

/* mgaGlyph.c */

extern void mgaDrawMonoGlyphs(
	nfbGlyphInfo *glyph_info,
	unsigned int nglyphs,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	nfbFontPSPtr pPS,
	DrawablePtr pDrawable );

/* mgaFont.c */

extern void mgaDrawFontText(
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

