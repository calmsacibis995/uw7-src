/*
 *  @(#) wd33Procs.h 11.1 97/10/22
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
 * wd33Procs.h          routines for the WD driver
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *	S001	Tue 17-Aug-1993	edb@sco.com
 *              Add DrawFontText8
 *      S002    Thu 18-Aug-1993 edb@sco.com
 *		Removed AllocPlane and replaced by AllocGlyphMem
 */

/* wd33Cmap.c */
extern void
wd33SetColor(
        unsigned int cmap,
        unsigned int index,
        unsigned short r,
        unsigned short g,
        unsigned short b,
        ScreenPtr pScreen);

/* wd33LoadColormap */
extern void
wd33LoadColormap(
        ColormapPtr cmap);

/* wd33Cursor.c */
extern void
wd33InstallCursor(
        unsigned long int *image,
        unsigned int hotx,
        unsigned int hoty,
        ScreenPtr pScreen);

extern void
wd33SetCursorPos(
        unsigned int x,
        unsigned int y,
        ScreenPtr pScreen);

extern void
wd33CursorOn(
        int on,
        ScreenPtr pScreen);

extern void
wd33SetCursorColor(
        unsigned short fr,
	unsigned short fg,
	unsigned short fb,
	unsigned short br,
	unsigned short bg,
	unsigned short bb,
        ScreenPtr pScreen);

/* wd33GC.c */
extern void
wd33ValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

extern void
wd33ValidateWindowGC15(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

extern void
wd33ValidateWindowGC16(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

extern void
wd33ValidateWindowGC24(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

/* wd33Image.c */
extern void
wd33ReadImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
wd33DrawImage(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* wd33Image16.c */
extern void
wd33ReadImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
wd33DrawImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* wd33RectOps.c */
extern void
wd33CopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wd33DrawMonoImage(
	BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wd33DrawOpaqueMonoImage(
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
wd33DrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wd33DrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void
wd33TileRects(
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


/* wd33Init.c */
extern Bool wd33Probe();

extern Bool wd33Init(
        int index,
        struct _Screen * pScreen,
        int argc,
        char **argv );

extern Bool wd33InitMem(
        struct _Screen * pScreen,
        int width,
        int height,
        int depth );

extern Bool
wd33CloseScreen(
        int index,
        ScreenPtr pScreen);

/* wd33Screen.c */
extern Bool
wd33BlankScreen(
        int on,
        ScreenPtr pScreen);

extern void wd33SetGraphics(
	ScreenPtr pScreen );

extern void wd33SetText(
	ScreenPtr pScreen );

extern void wd33SaveGState(
	ScreenPtr pScreen );

extern void wd33RestoreGState(
	ScreenPtr pScreen );

/* wd33Win.c */
extern void wd33ValidateWindowPriv(
        struct _Window * pWin );

/*           */
extern void wd33PolyRectangle(
        DrawablePtr pDraw, 
        GCPtr pGC, 
        int nrects,
        xRectangle *pRects);

/*  wd33Bres.c */

extern void wd33SolidZeroSegs(
        GCPtr gc,
        DrawablePtr pDraw,
        BresLinePtr pline,
        int nlines);

extern void wd33SolidZeroSeg(
	GCPtr pGC,
	DrawablePtr pDraw,
	int signdx,
	int signdy,
	int axis,
	int x1,
	int y1,
	register int e,
	int e1,
	int e2,
	int len );

/* wd33Font.c */

extern void wd33DrawFontText8(
	BoxPtr pbox,
	unsigned char *chars,
        unsigned int count,
        nfbFontPSPtr pPS,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
	DrawablePtr pDraw);

/*  wdGlyph.c */

extern void wd33DrawMonoGlyphs(
        nfbGlyphInfo *glyph_info,
        unsigned int nglyphs,
        unsigned long fg,
        unsigned char alu,
        unsigned long planemask,
        nfbFontPSPtr pPS,
        DrawablePtr pDrawable);

extern void wd33ClearFont( 
        struct _nfbFontPS *,
        ScreenPtr pScreen);

extern glMemSeg *wd33AllocGlyphMem(
        wdScrnPrivPtr wdPriv,
        int w,
        int h ,
        int nchar);

/*  wdDrwGlyph.c */

extern void wd33LoadGlyph(
        wdScrnPrivPtr wdPriv,
        int w,
        int h,
        unsigned char *image,
        unsigned int stride,
        int dstX,
        int dstY,
        int dstPlane );

extern void wd33DrawGlyph(
        BoxPtr pbox,
        int cacheX,
        int cacheY,
        int cachePlane,
        unsigned long fg,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* wd33FillSp.c */

extern void wd33SolidFS(
	struct _GC * pGC,
	struct _Drawable * pDraw,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned n ) ;

extern void wd33TiledFS(
	struct _GC * pGC,
	struct _Drawable * pDraw,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void wd33StippledFS(
	struct _GC * pGC,
	struct _Drawable * pDraw,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void wd33OpStippledFS(
	struct _GC * pGC,
	struct _Drawable * pDraw,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned n ) ;
/*
 * wd33FillRct.c
 */
extern void wd33SolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox );

extern void wd33TiledFillRects(
        GCPtr pGC,
        DrawablePtr pDraw,
        BoxPtr pbox,
        unsigned int nbox);

extern void wd33StippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern void wd33OpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern int
wd33LoadStipple(
        unsigned char *stipple,
        unsigned int stride,
        unsigned int w,
        unsigned int h,
        wdScrnPrivPtr wdPriv,
        unsigned long serialNr);

extern int
wd33LoadTile(
        unsigned char *tile,
        unsigned int stride,
        unsigned int w,
        unsigned int h,
        wdScrnPrivPtr wdPriv,
        unsigned long serialNr);

extern void
wd33QueryBestSize(
        int class,
        short *pwidth,
        short *pheight,
        ScreenPtr pScreen);

