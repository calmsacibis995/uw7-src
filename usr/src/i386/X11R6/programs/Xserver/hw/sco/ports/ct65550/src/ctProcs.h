/*
 *	@(#)ctProcs.h	11.1	10/22/97	12:35:00
 *	@(#) ctProcs.h 61.1 97/02/26 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1997.
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 * Copyright (C) 1994 Double Click Imaging, Inc.
 */
/*
 * ctProcs.h
 *
 * routines for the "ct" port
 */

#ifndef _CT_PROCS_H
#define _CT_PROCS_H

#ident "@(#) $Id: ctProcs.h 61.1 97/02/26 "

#include "ctDefs.h"

/* ctCmap.c */
extern void
CT(SetColor)(
        unsigned int cmap,
        unsigned int index,
        unsigned short r,
        unsigned short g,
        unsigned short b,
        ScreenPtr pScreen);

extern void
CT(LoadColormap)(
	ColormapPtr pmap );

/* ctCursor.c */
extern void CT(InstallCursor)(
        unsigned long int *image,
        unsigned int hotx,
        unsigned int hoty,
        ScreenPtr pScreen);

extern void CT(SetCursorPos)(
        unsigned int x,
        unsigned int y,
        ScreenPtr pScreen);

extern void CT(CursorOn)(
        int on,
        ScreenPtr pScreen);

extern void CT(SetCursorColor)(
        unsigned short fr,
	unsigned short fg,
	unsigned short fb,
	unsigned short br,
	unsigned short bg,
	unsigned short bb,
        ScreenPtr pScreen);

/* ctGC.c */
extern Bool CT(CreateGC)(
	GCPtr pGC);

extern void CT(ValidateWindowGC)(
	GCPtr pGC,
	Mask changes,
	DrawablePtr pDraw);

/* ctImage.c */
extern void CT(ReadImage)(
        BoxPtr pbox,
        void *image,
        unsigned int stride,
        DrawablePtr pDraw );

extern void CT(DrawImage)(
        BoxPtr pbox,
        void *image,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

extern void CT(DrawImageFB)(
        BoxPtr pbox,
        void *image,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

/* ctInit.c */
extern Bool CT(Probe)();

extern Bool CT(Init)(
        int index,
        struct _Screen * pScreen,
        int argc,
        char **argv );

extern void CT(FreeScreen)(
        int index,
        ScreenPtr pScreen);

/* ctScreen.c */
extern Bool CT(BlankScreen)(
        int on,
        ScreenPtr pScreen);

extern void CT(SetGraphics)(
	ScreenPtr pScreen );

extern void CT(SetText)(
	ScreenPtr pScreen );

extern void CT(SaveGState)(
	ScreenPtr pScreen );

extern void CT(RestoreGState)(
	ScreenPtr pScreen );

/* ctWin.c */
extern void CT(ValidateWindowPriv)(
        struct _Window * pWin );


/* ctRectOps.c */

extern void CT(CopyRect)(
	BoxPtr pdstBox,
	DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw );

extern void CT(DrawPoints)(
	DDXPointPtr ppt,
	unsigned int npts,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw );

extern void CT(DrawSolidRects)(
	BoxPtr pbox,
	unsigned int nbox,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw );

extern void CT(TileRects)(
	BoxPtr pbox,
	unsigned int nbox,
	void *tile,
	unsigned int stride,
	unsigned int w,
	unsigned int h,
	DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw );

/* ctMono.c */

extern void CT(DrawMonoImage)(
	BoxPtr pbox,
	void *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw );

extern void CT(DrawOpaqueMonoImage)(
	BoxPtr pbox,
	void *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pDraw );


/* ctFillSp.c */

extern void CT(SolidFS)(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

extern void CT(TiledFS)(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void CT(StippledFS)(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned int n ) ;

extern void CT(OpStippledFS)(
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned n ) ;

/* ctRect.c */

extern void CT(SolidFillRects)(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void CT(TiledFillRects)(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void CT(StippledFillRects)(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

extern void CT(OpStippledFillRects)(
	GCPtr pGC,
	DrawablePtr pDraw,
	BoxPtr pbox,
	unsigned int nbox ) ;

/* ctBres.c */

extern void CT(SolidZeroSegs)(
	GCPtr pGC,
	DrawablePtr pDraw,
	BresLinePtr plines,
	int nlines);

extern void CT(SolidZeroSeg)(
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

/* ctGlyph.c */

extern void CT(DrawMonoGlyphs)(
	nfbGlyphInfo *glyph_info,
	unsigned int nglyphs,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	nfbFontPSPtr pPS,
	DrawablePtr pDraw );

/* ctFont.c */

extern void CT(DrawFontText)(
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

#endif /* _CT_PROCS_H */
