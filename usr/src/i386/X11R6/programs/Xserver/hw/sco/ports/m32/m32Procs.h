/*
 * @(#) m32Procs.h 11.1 97/10/22
 *
 * Copyright (C) The Santa Cruz Operation, 1993.
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * SCO MODIFICATION HISTORY
 *
 * S000, 27-Jul-93, buckm
 *	Created.
 * S001, 31-Aug-93, buckm
 *	Add and delete some funcs.
 * S002, 21-Sep-94, davidw
 *	Correct compiler warnings.
 */
/*
 * m32Procs.h
 *
 * Mach-32 routines.
 */

/* m32Cmap.c */

extern void
m32SetColor(
        unsigned int cmap,
        unsigned int index,
        unsigned short r,
        unsigned short g,
        unsigned short b,
        ScreenPtr pScreen);

extern void
m32LoadColormap(
	ColormapPtr pmap);

/* m32GC.c */

extern void
m32ValidateWindowGC(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

/* m32Image8.c */

extern void
m32ReadImage8(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw);

extern void
m32DrawImage8(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* m32Image16.c */

extern void
m32ReadImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        DrawablePtr pDraw);

extern void
m32DrawImage16(
        BoxPtr pbox,
        void *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* m32Init.c */

extern Bool m32Probe();

extern Bool m32Init(
        int index,
        ScreenPtr pScreen,
        int argc,
        char **argv);

extern void							/* S002 */
m32FreeScreen(
        int index,
        ScreenPtr pScreen);

/* m32Screen.c */

extern Bool
m32BlankScreen(
        int on,
        ScreenPtr pScreen);

extern void m32SetGraphics(
	ScreenPtr pScreen);

extern void m32SetText(
	ScreenPtr pScreen);

extern void m32SaveGState(
	ScreenPtr pScreen);

extern void m32RestoreGState(
	ScreenPtr pScreen);

/* m32Win.c */

extern void m32ValidateWindowPriv(
        struct _Window * pWin);

/* m32RectOps.c */

extern void m32CopyRect(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void m32DrawPoints(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void m32PolyPoint(
	DrawablePtr pDraw,
	GCPtr pGC,
	int mode,
	int npt,
	struct _xPoint *ppt);

extern void m32DrawSolidRects(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void m32TileRects8(
	BoxPtr pbox,
	unsigned int nbox,
	void *tile,						/* S002 */
	unsigned int stride,
	unsigned int tilew,
	unsigned int tileh,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void m32TileRects16(
	BoxPtr pbox,
	unsigned int nbox,
	void *tile,						/* S002 */
	unsigned int stride,
	unsigned int tilew,
	unsigned int tileh,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

/* m32Mono.c */

extern void m32DrawMonoImage(
	BoxPtr pbox,
	void *image,						/* S002 */
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void m32DrawOpaqueMonoImage(
	BoxPtr pbox,
	void *image,						/* S002 */
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void m32DrawMonoGlyphs(
	struct _nfbGlyphInfo *pGI,
	unsigned int nglyph,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	unsigned long font_priv,
	DrawablePtr pDraw);

/* m32FillSp.c */

extern void m32SolidFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n);

extern void m32TiledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n);

extern void m32StippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n);

extern void m32OpStippledFS(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n);

extern void m32FillSpans(
	DrawablePtr pDraw,
	GCPtr pGC,
	unsigned int nspans,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	int fSorted);

/* m32FillRct.c */

extern void m32SolidFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern void m32TiledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern void m32StippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

extern void m32OpStippledFillRects(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox);

/* m32Line.c */

extern void m32SolidZeroSeg(
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

extern void m32LineSS(
	DrawablePtr pDraw,
	GCPtr pGC,
	int mode,
	unsigned int npt,
	DDXPointPtr pptInit);

extern void m32SegmentSS(
	DrawablePtr pDraw,
	GCPtr pGC,
	unsigned int nseg,
	struct _xSegment *pSeg);

extern void m32RectangleSS(
	DrawablePtr pDraw,
	GCPtr pGC,
	int nrects,
	struct _xRectangle *pRects);

extern void m32LineSD(
	DrawablePtr pDraw,
	GCPtr pGC,
	int mode,
	unsigned int npt,
	DDXPointPtr pptInit);

extern void m32SegmentSD(
	DrawablePtr pDraw,
	GCPtr pGC,
	unsigned int nseg,
	struct _xSegment *pSeg);

extern void m32RectangleSD(
	DrawablePtr pDraw,
	GCPtr pGC,
	int nrects,
	struct _xRectangle *pRects);

/* m32Font.c */

extern void m32DrawFontText(
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

extern void m32DownloadFont8(
	unsigned char **bits,
	int count,
	int width,
        int height,
	int stride,
	int index,
	ScreenPtr pScreen);

/* m32Misc.c */

extern void m32QueryBestSize(
	int class,
	short *pwidth,
	short *pheight,
	ScreenPtr pScreen);

/* m32Clip.c */

extern void m32SetClip(
	BoxPtr pbox,
	int nbox,
	DrawablePtr pDraw);
