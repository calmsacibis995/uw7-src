/*
 *	@(#) nfbProcs.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * nfbProcs.h
 *
 * extern declarations for nfb procedures
 *
 * SCO Modification History
 *
 * S013, 11-Nov-93, buckm
 *	nfb{Real,Unreal}izeFont() return Bool.
 *	fix args to nfbSwap funcs.
 * S012, 20-Apr-93, buckm
 *	add nfbDrawFontText.
 *	fix DoBitBlt decl's.
 * S011, 06-Apr-93, buckm
 *	get rid of ProtoScreenInit and ProtoCloseScreen.
 *	routines from nfbExpose.c, nfbLayerVT.c, nfbValTree, and nfbNewCmap.c
 *	are no longer among us.
 * S010, 26-Feb-93, buckm
 *	rename text8 routines again
 * S009, 05-Dec-92, mikep
 *	rename text8 routines
 * S008, 20-Nov-92, staceyc
 * 	new unrealize text8 font parameter to nfb text8 init routine, also
 *	cleaned up a number function decs that refer to non-existant types
 * S007, 27-Oct-92, mikep
 *	dashed line + segment routines
 * S006, 16-Oct-92, staceyc
 * 	text8 init routines
 * S005, 20-Sep-92, mikep
 *	added image routines
 * S004, 14-Sep-92, mikep
 *	added swap routines
 * S003, 01-Sep-92, staceyc
 * 	added point to point lines
 * S002, 31-Aug-92, staceyc
 * 	filled polygon added
 * S001, 28-Aug-92, staceyc
 * 	added nfb poly rects
 * S000, 26-Aug-92, staceyc
 * 	added text8 routines
 */

#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "validate.h"


/* nfbBitBlt.c */
extern void nfbDoBitBltSS(
	struct _Drawable *,	/* pDraw */
	struct _Region *,	/* prgnDst */
	int dx,			/* dx */
	int dy,			/* dy */
	unsigned char,		/* alu */
	unsigned long		/* planemask */
) ;

extern void nfbDoBitBltPS(
	struct _Drawable *,	/* pDst */
	struct _Drawable *,	/* pSrc */
	struct _Region *,	/* prgnDst */
	int dx,			/* dx */
	int dy,			/* dy */
	unsigned char,		/* alu */
	unsigned long		/* planemask */
) ;

extern void nfbDoBitBltSP(
	struct _Drawable *,	/* pDst */
	struct _Drawable *,	/* pSrc */
	struct _Region *,	/* prgnDst */
	int dx,			/* dx */
	int dy,			/* dy */
	unsigned char,		/* alu */
	unsigned long		/* planemask */
) ;

extern struct _Region * nfbCopyArea(
	struct _Drawable *,	/* pSrcDrawable */
	struct _Drawable *,	/* pDstDrawable */
	struct _GC *,		/* pGC */
	int,			/* srcx */
	int,			/* srcy */
	unsigned int,		/* width */
	unsigned int,		/* height */
	int,			/* dstx */
	int			/* dsty */
) ;

extern struct _Region * nfbCopyPlane(
	struct _Drawable *,	/* pSrcDrawable */
	struct _Drawable *,	/* pDstDrawable */
	struct _GC *,		/* pGC */
	int,			/* srcx */
	int,			/* srcy */
	unsigned int,		/* width */
	unsigned int,		/* height */
	int,			/* dstx */
	int,			/* dsty */
	unsigned long		/* bitPlane */
) ;

extern struct _Region * nfbCopyPlaneToScreen(
	struct _Drawable *,	/* pSrcDrawable */
	struct _Drawable *,	/* pDstDrawable */
	struct _GC *,		/* pGC */
	int,			/* srcx */
	int,			/* srcy */
	unsigned int,		/* width */
	unsigned int,		/* height */
	int,			/* dstx */
	int,			/* dsty */
	unsigned long		/* bitPlane */
) ;

extern unsigned long * nfbGetPlane(
	DrawablePtr	pDraw,
	int	planeNum,
	int	sx, 
	int sy, 
	int w, 
	int h,
	unsigned long *result
) ;

/* nfbCmap.c */
extern int nfbListInstalledColormaps(
	struct _Screen *,	/* pScreen */
	struct _ColormapRec *	/* pmaps */
) ;

extern void nfbInstallColormap( struct _ColormapRec * ) ;

extern void nfbUninstallColormap( struct _ColormapRec * ) ;

extern void nfbDestroyColormap( struct _ColormapRec * ) ;

extern void nfbStoreColors(
	struct _ColormapRec *,	/* pmap */
	int,			/* ndef */
	xColorItem *		/* pdefs */
) ;

/* nfbFillSp.c */
extern void nfbFillSpans(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	unsigned int nInit,
	struct _DDXPoint * pptInit,
	unsigned int *pwidthInit,
	int fSorted ) ;

extern void nfbFillSpansClipped(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	unsigned int nspans,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	int fSorted ) ;

/* nfbGetSp.c */
extern void nfbGetSpans(
	struct _Drawable * pDrawable,		/* drawable from which to get bits */
	int wMax,			/* largest value of all *pwidths */
	register struct _DDXPoint * ppt,	/* points to start copying from */
	int *pwidth,			/* list of number of bits to copy */
	int nspans,			/* number of scanlines to copy */
	unsigned int *pdstStart ) ;

/* nfbPntWin.c */
extern void nfbPaintWindowBorder(
	struct _Window * pWin,
	struct _Region * pRegion,
	int what ) ;

extern void nfbPaintWindowBackground(
	struct _Window * pWin,
	struct _Region * pRegion,
	int what ) ;

/* nfbRep.c */
extern void nfbReplicateArea(
	struct _Box * pbox,
	unsigned int tilew,
	unsigned int tileh,
	unsigned long planemask,
	struct _Drawable * pDraw ) ;

/* nfbScrInit.c */
extern Bool nfbCloseScreen(
	unsigned int index,
	struct _Screen * pScreen ) ;

extern Bool nfbScreenInit(
	struct _Screen * pScreen,
	int xsize,
	int ysize,
	int mmx,
	int mmy ) ;

extern Bool nfbAddVisual(
	struct _Screen * pScreen,
	struct _Visual * pVisual ) ;

extern void nfbFreeVisual(
	struct _Screen * pScreen ) ;

extern void nfbReturnFromScreenSwitch(
	struct _Screen * pScreen);

/* nfbSetSp.c */
extern void nfbSetSpans(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	unsigned int *psrc,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned int nspans,
	int fSorted ) ;

extern void nfbSetSpansClipped(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	unsigned int *psrc,
	struct _DDXPoint * ppt,
	unsigned int *pwidth,
	unsigned int nspans,
	int fSorted ) ;

/* nfbWindow.c */
extern Bool nfbCreateWindow( struct _Window * pWin ) ;

extern Bool nfbDestroyWindow( struct _Window * pWin ) ;

extern Bool nfbMapWindow( struct _Window * pWin ) ;

extern Bool nfbPositionWindow(
	struct _Window * pWin,
	int x,
	int y ) ;

extern Bool nfbUnmapWindow( struct _Window * pWin ) ;

extern void nfbCopyWindow(
	struct _Window * pWin,
	struct _DDXPoint ptOldOrg,
	struct _Region * prgnSrc ) ;

extern Bool nfbChangeWindowAttributes(
	struct _Window * pWin,
	unsigned long mask ) ;

/* nfbGC.c */
extern Bool nfbCreateGC( struct _GC * )  ;

/* nfbScreen.c */
extern Bool nfbSaveScreen(
	struct _Screen * pScreen,
	int on ) ;

/* nfbCacheCursor.c */
extern Bool nfbDisplayCacheCursor(
	struct _Screen * pScr,
	struct _Cursor * pCurs ) ;

extern Bool nfbRealizeCacheCursor(
	struct _Screen * pScr,
	struct _Cursor * pCurs ) ;

extern Bool nfbUnrealizeCacheCursor(
	struct _Screen * pScr,
	struct _Cursor * pCurs ) ;

/* nfbCursor.c */
extern Bool nfbDisplayCursor(
	struct _Screen * pScr,
	struct _Cursor * pCurs ) ;

extern Bool nfbRealizeCursor(
	struct _Screen * pScr,
	struct _Cursor * pCurs ) ;

extern Bool nfbUnrealizeCursor(
	struct _Screen * pScr,
	struct _Cursor * pCurs ) ;

extern void nfbCursorLimits(
	struct _Screen * pScr,         /* Screen on which limits are desired */
	struct _Cursor * pCurs,        /* Cursor whose limits are desired */
	struct _Box * pHotBox,         /* Limits for pCursor's hot point */
	struct _Box * pResultBox ) ;

extern Bool nfbSetCursorPosition(
	struct _Screen * pScr,
	unsigned int x, /* XXX should these be unsigned or signed?? */
	unsigned int y,
	Bool generateEvent ) ;

/* nfbImgGBlt.c */
extern void nfbImageGlyphBlt(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	int x,
	int y,
	unsigned int nglyph,
	struct _CharInfo **ppci,              /* array of character info */
	unsigned char *pglyphBase ) ;

/* nfbPlyGBlt.c */
extern void nfbSolidPolyGlyphBlt(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	int x,
	int y,
	unsigned int nglyph,
	struct _CharInfo **ppci,              /* array of character info */
	unsigned char *pglyphBase ) ;


/* nfbFillRct.c */
extern void nfbPolyFillRect(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	unsigned int nrectFill,
	struct _xRectangle *prectInit ) ;

/* nfbPolyPnt.c */
extern void nfbPolyPoint(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	int mode,               /* Origin or Previous */
	unsigned int npt,
	struct _xPoint *pptInit ) ;

extern void nfbPolyPointClipped(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	int mode,               /* Origin or Previous */
	unsigned int npt,
	struct _xPoint *ppt ) ;

/* nfbPixmap.c */
extern struct _Pixmap * nfbCreatePixmap(
	struct _Screen * pScreen,
	unsigned int width,
	unsigned int height,
	unsigned int depth ) ;

/* nfbLine.c */
extern void nfbLineSS(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	int mode,               /* Origin or Previous */
	unsigned int npt,
	struct _DDXPoint * pptInit ) ;

extern void nfbLineSSC(DrawablePtr pDraw,
	GCPtr pGC,
	int mode,
	unsigned int npt,
	DDXPointPtr pptInit);

/* nfbSeg.c */
extern void nfbSegmentSS(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	unsigned int nseg,
	xSegment * pSeg ) ;

extern void nfbSegmentSSC(DrawablePtr pDraw,
	GCPtr pGC,
	unsigned int nseg,
	xSegment *pSeg);

/* nfbDSeg.c */
extern void nfbSegmentSD(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	unsigned int nseg,
	xSegment * pSeg ) ;

/* nfbDLine.c */
extern void nfbLineSD(
	struct _Drawable * pDraw,
	struct _GC * pGC,
	int mode,               /* Origin or Previous */
	unsigned int npt,
	struct _DDXPoint * pptInit ) ;

/* nfbZeroArc.c */
extern void nfbZeroPolyArc() ;

/* nfbPushPix.c */
extern void nfbPushPixels(	
	GCPtr pGC,
	PixmapPtr pBitMap,
	DrawablePtr pDrawable,
    	int dx, 
	int dy,
	int xOrg,
	int yOrg);

/* nfbFont.c */
extern Bool nfbRealizeFont(
	ScreenPtr pScreen,
	FontPtr pFont);

extern Bool nfbUnrealizeFont(
	ScreenPtr pScreen,
	FontPtr pFont);

extern void nfbRestoreText8Fonts(
	ScreenPtr pScreen);

void
nfbDrawFontText(
	BoxPtr pbox,
	unsigned char *chars,
	unsigned int count,
	struct _nfbFontPS *pPS,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
	DrawablePtr pDraw);

/* nfbText8.c */
extern int nfbPolyTEText8(
	DrawablePtr pDraw,
	GCPtr pGC,
	int x,
	int y,
	int count,
	char *chars);

extern void nfbImageTEText8(
	DrawablePtr pDraw,
	GCPtr pGC,
	int x,
	int y,
	int count,
	char *chars);

/* nfbPlyRect.c */
extern void nfbPolyRectangle(
	DrawablePtr pDraw,
	GCPtr pGC,
	int nrects,
	xRectangle *pRects);

/* nfbPolygon.c */
extern void nfbFillPolygon(DrawablePtr dst,
	GCPtr pgc,
	int shape,
	int mode,
	int count,
	DDXPointPtr pPts);

/* nfbSImage.c */
extern void nfbSwapDrawMonoImage(
	BoxPtr pbox,
	void *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void nfbSwapDrawOpaqueMonoImage(
	BoxPtr pbox,
	void *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void nfbSwapDrawImage(
	BoxPtr pbox,
	void *image,
	unsigned int stride,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw);

extern void nfbSwapValidateWindowPriv(
	WindowPtr pWin);

extern Bool nfbSwapRealizeCursor(
        ScreenPtr pScreen,
        CursorPtr pCursor);

extern Bool nfbSwapDisplayCursor(
        ScreenPtr pScreen,
        CursorPtr pCursor);

/* nfbImage.c */
extern void nfbPutImage(
	DrawablePtr pDraw,
	GCPtr pGC,
	int depth, 
	int x, 
	int y, 
	int w, 
	int h,
	int leftPad,
	unsigned int format,
	char *pImage);


extern void nfbGetImage(
	DrawablePtr pDrawable,
	int sx,
	int sy,
	int w,
	int h,
	unsigned int format,
	unsigned long planeMask,
	pointer	pdstLine);

void nfbInitializeText8(ScreenPtr pScreen, int font_count, int max_width,
	int max_height,
	void (*DownloadFont8)(unsigned char **, int, int, int, int, int,
	    ScreenPtr),
	void (*ClearFont8)(int, ScreenPtr));
void nfbCloseText8(ScreenRec *pScreen);

