/*
 *	@(#) effProcs.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1984-1988, 1989, 1990.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * Modification History
 *
 * S034, 19-Jan-93, chrissc
 *	added ATI clip routine.
 * S033, 12-Dec-92, mikep
 *	add effSolidFS.
 * S032, 04-Dec-92, mikep
 * 	replace Text8 routines with DrawFontText
 * S031, 20-Nov-92, staceyc
 * 	remove all remnants of third party hardware mods
 * S030, 01-Nov-92, mikep
 * 	rearrange arguments of Font, and Clip routines to match nfb.
 * S029, 26-Oct-92, staceyc
 * 	somehow or another this file got trashed, this is the restoration
 * S028, 23-Oct-92, staceyc
 * 	merged pixelworks code back into eff
 * S027, 22-Oct-92, staceyc
 * 	move stretch blit code into here so that any compiler that is smart
 *	enough can inline
 * S026, 15-Oct-92, staceyc
 * 	added gcc inline code
 * S025, 04-Sep-92, mikep
 *	remove cursor close routine
 * S024, 04-Sep-92, staceyc
 * 	fast text added
 * S023, 03-Sep-92, hiramc
 *	Better declaration of arguments in effTileRects to eliminate compile
 *	warnings, and argument to GlClearCache
 * S022, 14-Nov-91, staceyc
 * 	512K card support
 * S021, 26-Sep-91, staceyc
 * 	VGA 8514 DAC save and restore routines
 * S020, 24-Sep-91, staceyc
 * 	glyph cache flushing routine
 * S019, 20-Sep-91, staceyc
 * 	slow AP draw mono for broken Matrox
 * S018, 18-Sep-91, staceyc
 * 	removed everything relating to image and poly glyphs - added draw mono
 *	glyphs
 * S017, 11-Sep-91, staceyc
 * 	added screen ptr to query best size
 * S016, 04-Sep-91, staceyc
 * 	procs for save/restore of off-screen card state
 * S015, 28-Aug-91, staceyc
 * 	removed unused glyph code
 * S014, 28-Aug-91, staceyc
 * 	new grafinfo api
 * S013, 28-Aug-91, staceyc
 * 	general code cleanup - added glyph list routines - fast rect routine
 * S012, 20-Aug-91, staceyc
 * 	fix up parameter decs for the image routines
 * S011, 16-Aug-91, staceyc
 * 	removed delete glyph cache entry routine from globally available
 *	routines
 * S010, 15-Aug-91, staceyc
 * 	glyph caching routines
 * S009, 12-Aug-91, staceyc
 * 	font routines
 * S008, 09-Aug-91, staceyc
 * 	stipple routines
 * S007, 01-Aug-91, staceyc
 * 	added query best size routine
 * S006, 16-Jul-91, staceyc
 * 	cursor init stuff added
 * S005, 28-Jun-91, staceyc
 * 	give init hardware a screen pointer
 * S004, 27-Jun-91, staceyc
 * 	some new colormap routines
 * S003, 26-Jun-91, buckm
 * 	new sgi source: ValidateVisual args changed.
 * S002, 24-Jun-91, staceyc
 * 	fixed decls so they match nfb
 * S001, 21-Jun-91, staceyc
 * 	Bres line drawing added
 * S000, 21-Jun-91, staceyc
 * 	copy rects added - changed all gen to eff
 */

#ifndef EFFPROCS_H
#define EFFPROCS_H

#ifndef __GNUC__
#define __inline__
#endif

#ifdef __GNUC__

static __inline__ void
outb(
     unsigned short port,
     char val)
{
  __asm__ volatile("out%B0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
outw(
     unsigned short port,
     unsigned short val)
{
  __asm__ volatile("out%W0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ unsigned char
inb(
     unsigned short port)
{
  unsigned char ret;
  __asm__ volatile("in%B0 (%1)" :
		   "=a" (ret) :
		   "d" (port));
  return ret;
}

static __inline__ unsigned short
inw(
     unsigned short port)
{
  unsigned short ret;
  __asm__ volatile("in%W0 (%1)" :
		   "=a" (ret) :
		   "d" (port));
  return ret;
}

static __inline__ void
insw(
unsigned short port,
unsigned char *string,
unsigned int count)
{
	__asm__ volatile ("rep; insw"
			  : /* no outputs */
			  : "D" (string), "d" (port), "c" (count)
			  : "di", "cx" /* clobbered */);
}

static __inline__ void
outsw(
unsigned short port,
unsigned char *string,
unsigned int count)
{
	__asm__ volatile ("rep; outsw"
			  : /* no outputs */
			  : "S" (string), "d" (port), "c" (count)
			  : "si", "cx" /* clobbered */);
}

static __inline__ unsigned char *
memcpy(
unsigned char *dst,
unsigned char *src,
int count)
{
	int new_count;

	new_count = count >> 2;
	__asm__ volatile ("rep; smovl"
		:
		: "S" (src), "D" (dst), "c" (new_count)
		: "si", "di", "cx");
	count &= 0x3;
	__asm__ volatile ("rep; smovb"
		:
		: "c" (count)
		: "cx");
}

#endif

/* effCmap.c */
extern void
effSetColor(
        unsigned int cmap,
        unsigned int index,
        unsigned short r,
        unsigned short g,
        unsigned short b,
        ScreenPtr pScreen);

/* effCursor.c */
extern void
effInstallCursor(
        unsigned long int *image,
        unsigned int hotx,
        unsigned int hoty,
        ScreenPtr pScreen);

extern void
effSetCursorPos(
        unsigned int x,
        unsigned int y,
        ScreenPtr pScreen);

extern void
effCursorOn(
        int on,
        ScreenPtr pScreen);

extern void
effSetCursorColor(
        unsigned short fr,
	unsigned short fg,
	unsigned short fb,
	unsigned short br,
	unsigned short bg,
	unsigned short bb,
        ScreenPtr pScreen);

/* effImage.c */
extern void
effReadImage(
        BoxPtr pbox,
        unsigned char *imagearg,
        unsigned int stride,
        DrawablePtr pDraw );

extern void
effDrawImage(
        BoxPtr pbox,
        unsigned char *imagearg,
        unsigned int stride,
        unsigned char alu,
        unsigned long planemask,
        DrawablePtr pDraw);

extern Bool effProbe(ddxDOVersionID version, ddxScreenRequest *pReq);

extern Bool effInit(int index, struct _Screen * pScreen, int argc,
        char **argv);

extern void effInitHW(ScreenPtr pScreen);

extern Bool
effCloseScreen(
        int index,
        ScreenPtr pScreen);

/* effScreen.c */
extern Bool
effBlankScreen(
        int on,
        ScreenPtr pScreen);

/* effWin.c */
extern void effValidateWindowPriv(
        struct _Window * pWin );

/* effGC.c */
extern void effValidateWindowGC(
        struct _GC * pGC,
        unsigned long /* Mask */ changes,
        struct _Drawable * pDraw );

/* effRectOps.c */

extern void effCopyRect(
	register BoxPtr pdstBox,
	register DDXPointPtr psrc,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pWin );

extern void effDrawMonoImage(
	register BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pWin );

extern void effDrawOpaqueMonoImage(
	register BoxPtr pbox,
	unsigned char *image,
	unsigned int startx,
	unsigned int stride,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pWin );

extern void effDrawPoints(
	register DDXPointPtr ppt,
	register unsigned int npts,
	register unsigned long fg,
	register unsigned char alu,
	register unsigned long planemask,
	DrawablePtr pWin );

extern void effDrawSolidRects(
	register BoxPtr pbox,
	register unsigned int nbox,
	register unsigned long fg,
	register unsigned char alu,
	register unsigned long planemask,
	DrawablePtr pWin );

extern void effTileRects(
	register BoxPtr pbox,
	unsigned int nbox,			/*	S023	*/
	unsigned char *tile,
	unsigned int stride,			/*	S023	*/
	register unsigned int w,
	register unsigned int h,
	register DDXPointPtr patOrg,
	unsigned char alu,
	unsigned long planemask,
	DrawablePtr pWin );

/* effFillSp.c */
extern void effSolidFS( 
	GCPtr pGC,
	DrawablePtr pDraw,
	DDXPointPtr ppt,
	unsigned int *pwidth,
	unsigned npts );

/* effVisual.c */
extern void effValidateVisual(struct _Screen * pScreen);

/* micursor.c */

extern void miRecolorCursor(
	ScreenPtr   pScr,
	CursorPtr   pCurs,
	Bool        displayed);

extern void effSetGraphics(ScreenPtr pScreen);
extern void effSetText(ScreenPtr pScreen);

extern void effFillZeroSeg(struct _GC *, struct _Drawable *, int, int, int,
	int, int, int, int, int, int);

extern void effRestoreColormap(ScreenPtr pScreen);

extern void effLoadColormap(ColormapPtr pmap);

extern Bool effDCInitialize(ScreenPtr);

extern void effQueryBestSize(int class, short *pwidth, short *pheight,
	ScreenPtr pScreen);

extern void effStippledFillRects(GCPtr pGC, DrawablePtr pDraw, BoxPtr pbox,
	unsigned int nbox);

extern void effOpStippledFillRects(GCPtr pGC, DrawablePtr pDraw, BoxPtr pbox,
	unsigned int nbox);

extern void effHelpValidateGC(GCPtr pGC, Mask changes, DrawablePtr pDraw);

extern Bool effRealizeFont(ScreenPtr pscr, FontPtr pFont);

extern Bool effUnrealizeFont(ScreenPtr pscr, FontPtr pFont);

extern int effRasterOps[];

static __inline__ void
effSetupCardForOpaqueBlit(
unsigned long fg,
unsigned long bg,
unsigned char alu,
unsigned long readplanemask,
unsigned long writeplanemask)
{
	EFF_CLEAR_QUEUE(7);
	EFF_SETFN0(EFF_FNCOLOR0, effRasterOps[alu]);
	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);
	EFF_SETMODE(EFF_M_CPYRCT);
	EFF_PLNRENBL(readplanemask);
	EFF_PLNWENBL(writeplanemask);
	EFF_SETCOL0(bg);
	EFF_SETCOL1(fg);
}

static __inline__ void
effSetupCardForBlit(
unsigned long fg,
unsigned char alu,
unsigned long readplanemask,
unsigned long writeplanemask)
{
	EFF_CLEAR_QUEUE(6);
	EFF_SETFN0(EFF_FNCOLOR0, EFF_FNNOP);
	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);
	EFF_PLNRENBL(readplanemask);
	EFF_PLNWENBL(writeplanemask);
	EFF_SETCOL1(fg);
	EFF_SETMODE(EFF_M_CPYRCT);
}

static __inline__ void
effStretchBlit(
DrawablePtr pDraw,
DDXPointPtr psrc,
BoxRec *pdest)
{
	int lx, ly;

	lx = pdest->x2 - pdest->x1;
	ly = pdest->y2 - pdest->y1;
	if (ly <= 0 || lx <= 0)
		return;
	
	EFF_CLEAR_QUEUE(7);
	EFF_SETX0(psrc->x);
	EFF_SETY0(psrc->y);
	EFF_SETX1(pdest->x1);
	EFF_SETY1(pdest->y1);
	EFF_SETLX(lx - 1);
	EFF_SETLY(ly - 1);
	EFF_COMMAND(0xC0F3);
}

static __inline__ void
effResetCardFromBlit()
{
}

extern void effGlCacheInit(ScreenPtr pScreen);

extern void effGlCacheClose(ScreenPtr pScreen);

extern void effGlClearCache(int fontID, ScreenPtr pScreen);	/*	S023 */

extern Bool effGlReadCacheEntry(ScreenPtr pScreen, effGlFontID_t font_id,
	unsigned int byteOffset, effGlGlyphInfo_t *glyph_info);

extern Bool effGlAddCacheEntry(ScreenPtr pScreen, effGlFontID_t font_id,
	unsigned int byteOffset, int glyph_width, int glyph_height,
	effGlGlyphInfo_t *glyph_info);

extern void effBlamOutBox(int x, int y, int lx, int ly);

extern void effStretchGlyphBlit(int x0, int y0, int x1, int y1, int lx,
	int ly, unsigned long readplanemask);

extern void effSaveGState(ScreenPtr pScreen);
extern void effRestoreGState(ScreenPtr pScreen);

extern void effDrawMonoGlyphs(nfbGlyphInfo *glyph_info, unsigned int nglyphs,
    unsigned long fg, unsigned char alu, unsigned long planemask,
    unsigned long font_private, DrawablePtr pDrawable);

extern void effAPBlockOutW(unsigned char *, int);
extern void effSlowAPBlockOutW(unsigned char *, int);
extern void effGlFlushCache(ScreenPtr pScreen);
extern void effSaveDACState(ScreenPtr pScreen);
extern void effRestoreDACState(ScreenPtr pScreen);

extern void effSetClipRegions(BoxRec *pbox, int nbox, DrawablePtr pDraw);
extern void atiSetClipRegions(BoxRec *pbox, int nbox, DrawablePtr pDraw); /* S034 */

extern void effInitializeText8(ScreenRec *pScreen);
extern void effDownloadFont8( unsigned char **bits, int count, int width,
        int height, int stride, int index, ScreenRec *pScreen);
extern void effDrawFontText(BoxPtr pbox,
        unsigned char *chars, unsigned int count,
        unsigned short glyph_width,
        int index,
	unsigned long fg, unsigned long bg, unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
        DrawablePtr pDraw);
extern void effCloseText8(ScreenPtr pScreen);

#endif

