/*
 *	@(#)s3cProcs.h	6.1	3/20/96	10:23:29
 *
 * 	Copyright (C) Xware, 1991-1992.
 *
 * 	The information in this file is provided for the exclusive use
 *	of the licensees of Xware. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they 
 *	include this notice and the associated copyright notice with 
 *	any such product.
 *
 *	Copyright (C) The Santa Cruz Operation, 1993
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	The information in this file is provided for the exclusive use
 *	of the licensees of SCO. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such
 *	product.
 *
 * Modification History
 *
 * S014, 02-Jun-93, staceyc
 * 	declaration for cursor color routine that is called from cmap code
 * S013, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S012, 11-May-93, staceyc
 * 	include file clean up, replace calls to assembler code with calls
 * 	to C code
 * S011, 05-Apr-93, staceyc
 * 	removed ancient unused functions, cleaned up file
 * S010, 29-Oct-92, mikep@sco.com
 *	Add s3cValidateWindowGC16().  Nix old mod history.
 * S009, 05-Sep-93, hiramc@sco.com
 *	Proper argument declaration on TileRects to eliminate compiler
 *	warnings.  And the argument to GlClearCache
 * X008, 02-Jan-92, kevin@xware.com
 *      added support for mode selectable hardware or software cursor.
 */

#ifndef S3CPROCS_H
#define S3CPROCS_H

/*
 * s3cBres.c
 */

extern void
S3CNAME(FillZeroSeg)(
	struct _GC		*pGC,
	DrawablePtr		pDraw,
	int 			signdx,
	int 			signdy,
	int 			axis,
	int 			x,
	int 			y,
	int 			e,
	int 			e1,
	int 			e2,
	int 			len);

extern void
S3CNAME(FillZeroSeg16)(
	struct _GC		*pGC,
	DrawablePtr		pDraw,
	int 			signdx,
	int 			signdy,
	int 			axis,
	int 			x,
	int 			y,
	int 			e,
	int 			e1,
	int 			e2,
	int 			len);

/*
 * s3cCmap.c 
 */

extern void 	
S3CNAME(SetColor)(
        unsigned int 		cmap,
        unsigned int 		index,
        unsigned short 		r,
        unsigned short 		g,
        unsigned short 		b,
        ScreenPtr 		pScreen);

extern void 
S3CNAME(RestoreColormap)(
	ScreenPtr 		pScreen);

extern void 
S3CNAME(LoadColormap)(
	ColormapPtr 		pmap);

/*
 * s3cGC.c 
 */

extern void	
S3CNAME(ValidateWindowGC)(
        struct _GC 		*pGC,
        unsigned long 		changes,
        struct _Drawable 	*pDraw );

extern void	
S3CNAME(ValidateWindowGC16)(
        struct _GC 		*pGC,
        unsigned long 		changes,
        struct _Drawable 	*pDraw );

/*
 * s3cGlCache.c
 */

extern void 
S3CNAME(GlCacheInit)(
	ScreenPtr 		pScreen);

extern void 
S3CNAME(GlCacheClose)(
	ScreenPtr 		pScreen);

extern void 
S3CNAME(GlClearCache)(
	int	 		fontID,			/*	X009	*/
	ScreenPtr 		pScreen);

extern Bool 
S3CNAME(GlReadCacheEntry)(
	ScreenPtr 		pScreen, 
	s3cGlFontID_t 		font_id,
	unsigned int 		byteOffset, 
	s3cGlGlyphInfo_t 	*glyph_info);

extern Bool 
S3CNAME(GlAddCacheEntry)(
	ScreenPtr 		pScreen, 
	s3cGlFontID_t 		font_id,
	unsigned int 		byteOffset, 
	int 			glyph_width, 
	int 			glyph_height,
	s3cGlGlyphInfo_t 	*glyph_info);

/*
 * s3cGlyph.c
 */

extern void
S3CNAME(DrawMonoGlyphs)(
	nfbGlyphInfo 	*glyph_info,
	unsigned int 	nglyphs,
	unsigned long 	fg,
	unsigned char 	alu,
	unsigned long 	planemask,
	unsigned long 	font_private,
	DrawablePtr 	pDrawable);

extern void
S3CNAME(DrawMonoGlyphs16)(
	nfbGlyphInfo 	*glyph_info,
	unsigned int 	nglyphs,
	unsigned long 	fg,
	unsigned char 	alu,
	unsigned long 	planemask,
	unsigned long 	font_private,
	DrawablePtr 	pDrawable);

/*
 * s3cHWCursor.c
 */

extern void	
S3CNAME(HWCursorInitialize)(
	ScreenPtr		pScreen);

/*
 * s3cImage.c 
 */

extern void 	
S3CNAME(DrawImage)(
        BoxPtr 			pbox,
        unsigned char 		*image,
        unsigned int 		stride,
        unsigned char 		alu,
        unsigned long 		planemask,
        DrawablePtr 		pDraw);

extern void 	
S3CNAME(DrawImage16)(
        BoxPtr 			pbox,
        unsigned char 		*image,
        unsigned int 		stride,
        unsigned char 		alu,
        unsigned long 		planemask,
        DrawablePtr 		pDraw);

extern void 	
S3CNAME(ReadImage)(
        BoxPtr 			pbox,
        unsigned char 		*image,
        unsigned int 		stride,
        DrawablePtr 		pDraw);

extern void 	
S3CNAME(ReadImage16)(
        BoxPtr 			pbox,
        unsigned char 		*image,
        unsigned int 		stride,
        DrawablePtr 		pDraw);

/*
 * s3cInit.c
 */

extern Bool 	
S3CNAME(Probe)(
	ddxDOVersionID 		version, 
	ddxScreenRequest 	*pReq);

extern Bool 	
S3CNAME(Init)(
	int 			index, 
	struct _Screen 		*pScreen, 
	int 			argc,
        char 			**argv);
/*
 * s3cMisc.c
 */

extern void
S3CNAME(QueryBestSize)(
	int 			class,
	short 			*pwidth,
	short 			*pheight,
	ScreenPtr	 	pScreen);

/*
 * s3cMono.c
 */

extern void 
S3CNAME(DrawMonoImage)(
	register BoxPtr 	pbox,
	unsigned char 		*image,
	unsigned int 		startx,
	unsigned int 		stride,
	unsigned long 		fg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(DrawMonoImage16)(
	register BoxPtr 	pbox,
	unsigned char 		*image,
	unsigned int 		startx,
	unsigned int 		stride,
	unsigned long 		fg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(DrawOpaqueMonoImage)(
	register BoxPtr 	pbox,
	unsigned char 		*image,
	unsigned int 		startx,
	unsigned int 		stride,
	unsigned long 		fg,
	unsigned long 		bg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(DrawOpaqueMonoImage16)(
	register BoxPtr 	pbox,
	unsigned char 		*image,
	unsigned int 		startx,
	unsigned int 		stride,
	unsigned long 		fg,
	unsigned long 		bg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pWin);

/*
 * s3cRectOps.c 
 */

extern void 
S3CNAME(CopyRect)(
	register BoxPtr 	pdstBox,
	register DDXPointPtr 	psrc,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(CopyRect16)(
	register BoxPtr 	pdstBox,
	register DDXPointPtr 	psrc,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(DrawPoints)(
	register DDXPointPtr 	ppt,
	register unsigned int 	npts,
	register unsigned long 	fg,
	register unsigned char 	alu,
	register unsigned long 	planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(DrawPoints16)(
	register DDXPointPtr 	ppt,
	register unsigned int 	npts,
	register unsigned long 	fg,
	register unsigned char 	alu,
	register unsigned long 	planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(DrawSolidRects)(
	register BoxPtr 	pbox,
	register unsigned int 	nbox,
	register unsigned long 	fg,
	register unsigned char 	alu,
	register unsigned long 	planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(DrawSolidRects16)(
	register BoxPtr 	pbox,
	register unsigned int 	nbox,
	register unsigned long 	fg,
	register unsigned char 	alu,
	register unsigned long 	planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(TileRects)(
	register BoxPtr 	pbox,
	unsigned int		nbox,			/*	X009	*/
	unsigned char 		*tile,
	unsigned int		stride,			/*	X009	*/
	register unsigned int 	w,
	register unsigned int 	h,
	register DDXPointPtr 	patOrg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pWin);

extern void 
S3CNAME(TileRects16)(
	register BoxPtr 	pbox,
	unsigned int		nbox,			/*	X009	*/
	unsigned char 		*tile,
	unsigned int		stride,			/*	X009	*/
	register unsigned int 	w,
	register unsigned int 	h,
	register DDXPointPtr 	patOrg,
	unsigned char 		alu,
	unsigned long 		planemask,
	DrawablePtr 		pWin);

/*
 * s3cSWCursor.c
 */

extern void	
S3CNAME(SWCursorInitialize)(
	ScreenPtr		pScreen);

/*
 * s3cScreen.c
 */

extern Bool 	
S3CNAME(BlankScreen)(
        int			on,
        ScreenPtr		pScreen);

extern void 	
S3CNAME(SetGraphics)(
	ScreenPtr		pScreen);

extern void 	
S3CNAME(SetText)(
	ScreenPtr 		pScreen);

extern void 
S3CNAME(SaveGState)(
	ScreenPtr 		pScreen);

extern void 
S3CNAME(RestoreGState)(
	ScreenPtr 		pScreen);

extern Bool 	
S3CNAME(CloseScreen)(
        int 			index,
        ScreenPtr 		pScreen);

/*
 * s3cStip.c
 */

extern void
S3CNAME(StippledFillRects)(
	GCPtr 			pGC,
	DrawablePtr		pDraw,
	BoxPtr 			pbox,
	unsigned int 		nbox);

extern void
S3CNAME(StippledFillRects16)(
	GCPtr 			pGC,
	DrawablePtr		pDraw,
	BoxPtr 			pbox,
	unsigned int 		nbox);

extern void
S3CNAME(OpStippledFillRects)(
	GCPtr 			pGC,
	DrawablePtr 		pDraw,
	BoxPtr 			pbox,
	unsigned int 		nbox);

extern void
S3CNAME(OpStippledFillRects16)(
	GCPtr 			pGC,
	DrawablePtr 		pDraw,
	BoxPtr 			pbox,
	unsigned int 		nbox);

/* 
 * s3cWin.c 
 */

extern void 	
S3CNAME(ValidateWindowPriv)(
        struct _Window 		*pWin);

extern void
S3CNAME(CpyRect)(int srcx, int srcy, int dstx, int dsty, int lx,
	int ly, int rop, int planemask, int command);
extern void
S3CNAME(SetupStretchBlit)(int rop, int fg, int readplanemask,
	int writeplanemask);
extern void
S3CNAME(StretchBlit)(int srcx, int srcy, int dstx, int dsty, int lx,
	int ly, int command);
extern void S3CNAME(HWCursorOn)(int on, ScreenPtr pScreen);
extern void S3CNAME(InitializeMultihead)(ScreenRec *pScreen);
extern void S3CNAME(HWColorCursor)(ScreenPtr pScreen, CursorPtr pCursor);

#endif
