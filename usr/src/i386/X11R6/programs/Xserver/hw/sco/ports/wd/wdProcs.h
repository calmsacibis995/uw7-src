/*
 *  @(#) wdProcs.h 11.1 97/10/22
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
 * wdProcs.h          routines for the WD driver
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *      S001    Thu 05-Oct-1992 edb@sco.com
 *              implement tiled and stippled GC ops
 *	S002	Mon 09-Nov-1992	edb@sco.com
 *              fix argument type
 *	S003	Tue 09-Feb-1993	buckm@sco.com
 *              Add font, 15,16,24 bit routines.
 *      S004    Thr Apr 15    edb@sco.com
 *              Add wdDrawFontText15_24 and wdDownloadFont25_24
 *      S005    Wdn 19-May-93    edb@sco.com
 *              change argument in wdDrawMonoGlyphs
 *      S006    Wdn 19-May-93    edb@sco.com
 *              change argument in wdClearFont()
 *      S007    Tue 25-May-93    edb@sco.com
 *              rename wdDrawFontText()
 *              rename wdLoadGlyph() wdDrawGlyph()
 *              add wdLoadGlyph15_24() wdDrawGlyph15_24()
 */

/* wdCmap.c */
extern void
wdSetColor(
        unsigned int cmap,
        unsigned int index,
        unsigned short r,
        unsigned short g,
        unsigned short b,
        ScreenPtr pScreen);

/* wdLoadColormap */
extern void
wdLoadColormap(
        ColormapPtr cmap);

/* wdCursor.c */
extern void
wdInstallCursor(
        unsigned long int *image,
        unsigned int hotx,
        unsigned int hoty,
        ScreenPtr pScreen);

extern void
wdSetCursorPos(
        unsigned int x,
        unsigned int y,
        ScreenPtr pScreen);

extern void
wdCursorOn(
        int on,
        ScreenPtr pScreen);

extern void
wdSetCursorColor(
        unsigned short fr,
	unsigned short fg,
	unsigned short fb,
	unsigned short br,
	unsigned short bg,
	unsigned short bb,
        ScreenPtr pScreen);

/* wdGC.c */
extern void
wdValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

extern void
wdValidateWindowGC15(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

extern void
wdValidateWindowGC16(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

extern void
wdValidateWindowGC24(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

/* wdImage.c */
extern void
wdReadImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
wdDrawImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* wdImage15.c */
extern void
wdReadImage15(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
wdDrawImage15(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* wdImage16.c */
extern void
wdReadImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
wdDrawImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* wdImage24.c */
extern void
wdReadImage24(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
wdDrawImage24(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* wdRectOps.c */
extern void
wdCopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawMonoImage(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawOpaqueMonoImage(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdTileRects(
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

/* wdRectOps15.c */
extern void
wdCopyRect15(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawSolidRects15(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawPoints15(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

/* wdRectOps16.c */
extern void
wdCopyRect16(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawSolidRects16(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawPoints16(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

/* wdRectOps24.c */
extern void
wdCopyRect24(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawSolidRects24(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wdDrawPoints24(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

/* wdInit.c */
extern Bool wdProbe();

extern Bool wdInit(
        int index,
        struct _Screen * pScreen,
        int argc,
        char **argv );

extern Bool wdInitMem(
        struct _Screen * pScreen,
        int width,
        int height,
        int depth );

extern Bool
wdCloseScreen(
        int index,
        ScreenPtr pScreen);

/* wdScreen.c */
extern Bool
wdBlankScreen(
        int on,
        ScreenPtr pScreen);

extern void wdSetGraphics(
	ScreenPtr pScreen );

extern void wdSetText(
	ScreenPtr pScreen );

extern void wdSaveGState(
	ScreenPtr pScreen );

extern void wdRestoreGState(
	ScreenPtr pScreen );

/* wdWin.c */
extern void wdValidateWindowPriv(
        struct _Window * pWin );

/*           */
extern void wdPolyRectangle(
        DrawablePtr pDraw, 
        GCPtr pGC, 
        int nrects,
        xRectangle *pRects);

/*  wdLine.c */

extern void wdSegmentSS(
        DrawablePtr pDraw, 
        GCPtr pGC, 
        unsigned int nseg,
        xSegment *pSeg);

extern void wdLineSS(
        DrawablePtr pDraw, 
        GCPtr pGC, 
        int mode,
        unsigned int npt, 
        DDXPointPtr pptInit);

extern void wdLineSD(
        DrawablePtr pDraw, 
        GCPtr pGC, 
        int mode,
        unsigned int npt, 
        DDXPointPtr pptInit);

/*  wdBres.c */

extern void wdSolidZeroSeg(
        GCPtr gc,
        DrawablePtr pDraw,
        int signdx, 
        int signdy, 
        int axis, 
        int x, 
        int y, 
        int e, 
        int e1,
        int e2, 
        int len);

/* wdFont.c */

extern void wdDownloadFont8(
	unsigned char **ppbits,
	int count,
	int width,
	int height,
	int stride,
	int index,
	ScreenPtr pScreen);

extern void wdDownloadFont15_24(
	unsigned char **ppbits,
	int count,
	int width,
	int height,
	int stride,
	int index,
	ScreenPtr pScreen);

extern void wdClearFont8( 
	int index,
	ScreenPtr pScreen);

extern void wdDrawFontText8(
	BoxPtr pbox,
	unsigned char *chars,
	unsigned int count,
	unsigned short width,
	int index,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
	DrawablePtr pDraw);

extern void wdDrawFontText15_24(
	BoxPtr pbox,
	unsigned char *chars,
	unsigned int count,
	unsigned short width,
	int index,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
	DrawablePtr pDraw);

/*  wdGlyph.c */

extern void wdDrawMonoGlyphs(
        nfbGlyphInfo *glyph_info,
        unsigned int nglyphs,
        unsigned long fg,
        unsigned char alu,
        unsigned long planemask,
        nfbFontPSPtr pPS,           /* S005 */
        DrawablePtr pDrawable);

extern void wdClearFont( 
        struct _nfbFontPS *,        /* S006 */
        ScreenPtr pScreen);

/*  wdDrwGlyph.c */

extern void wdLoadGlyph8(
        int w,
        int h,
        unsigned char *image,
	unsigned int stride,
        int dstAddr,
        int dstPlane,
	DrawablePtr pDraw);

extern void wdDrawGlyph8(
        BoxPtr pbox,
        int cacheAddr,
        int cachePlane,
	unsigned long fg,
        unsigned char alu,
	unsigned long planemask,
        DrawablePtr pDraw);

/*  wdDrwGlyph24.c */

extern void wdLoadGlyph15_24(
        int w,
        int h,
        unsigned char *image,
	unsigned int stride,
        int dstAddr,
        int dstPlane,
	DrawablePtr pDraw);

extern void wdDrawGlyph15_24(
        BoxPtr pbox,
        int cacheAddr,
        int cachePlane,
	unsigned long fg,
        unsigned char alu,
	unsigned long planemask,
        DrawablePtr pDraw);

/* wdFillSp.c */

extern void wdSolidFS(
	struct _GC * pGC,
	struct _Drawable * pDraw,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned n ) ;

extern void wdTiledFS(
	struct _GC * pGC,
	struct _Drawable * pDraw,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void wdStippledFS(
	struct _GC * pGC,
	struct _Drawable * pDraw,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void wdOpStippledFS(
	struct _GC * pGC,
	struct _Drawable * pDraw,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned n ) ;
/*
 * wdFillRct.c
 */
extern void wdSolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox );

extern void wdTiledFillRects(
        GCPtr pGC,
        DrawablePtr pDraw,
        BoxPtr pbox,
        unsigned int nbox);

extern void wdStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern void wdStippledFillRects15_24(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern void wdOpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern void wdOpStippledFillRects15_24(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern int
wdLoadStipple(
        unsigned char *stipple,
        unsigned int stride,
        unsigned int w,
        unsigned int h,
        wdScrnPrivPtr wdPriv,
        unsigned long serialNr);

extern int
wdLoadStipple_15_24(
        unsigned char *stipple,
        unsigned int stride,
        unsigned int w,
        unsigned int h,
        wdScrnPrivPtr wdPriv,
        unsigned long serialNr);

extern int
wdLoadTile(
        unsigned char *tile,
        unsigned int stride,
        unsigned int w,
        unsigned int h,
        wdScrnPrivPtr wdPriv,
        unsigned long serialNr);

extern int
wdLoadTile15_24(
        unsigned char *tile,
        unsigned int stride,
        unsigned int w,
        unsigned int h,
        wdScrnPrivPtr wdPriv,
        unsigned long serialNr);

extern void
wdQueryBestSize(
        int class,
        short *pwidth,
        short *pheight,
        ScreenPtr pScreen);

