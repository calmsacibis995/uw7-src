/*
 * routines to draw glyphs using cache
 */

/*
 *	@(#) effGlyph.c 11.1 97/10/22
 */

/*
 * Modification History
 *
 * S005, 11-Nov-92, mikep
 *	remove S003.  NFB is now fixed.
 * S004, 03-Sep-92, hiramc@sco.COM
 *	Use include misc.h instead of mfb.h
 * S003, 27-Jan-92, mikep
 *	add size check to draw mono glyphs
 * S002, 18-Sep-91, staceyc
 * 	added draw mono glyphs - now called from nfb code
 * S001, 28-Aug-91, staceyc
 * 	cleanup unused code
 * S000, 28-Aug-91, staceyc
 * 	created
 */

#include "X.h"
#include "Xproto.h"
#include "misc.h"		/*	S004	*/
#include "dixfontstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "effConsts.h"
#include "effDefs.h"
#include "effMacros.h"
#include "effProcs.h"

extern int effRasterOps[];

static void
effSetupCardForGlyphBlit(
unsigned long fg,
unsigned char alu,
unsigned long writeplanemask)
{
	EFF_CLEAR_QUEUE(6);
	EFF_SETFN0(EFF_FNCOLOR0, EFF_FNNOP);
	EFF_SETFN1(EFF_FNCOLOR1, effRasterOps[alu]);
	EFF_PLNWENBL(writeplanemask);
	EFF_SETCOL1(fg);
	EFF_SETMODE(EFF_M_CPYRCT);
}

#if 0
/*
 * currently coded in assembler
 */
static void
effStretchGlyphBlit(
int x0,
int y0,
int x1,
int y1,
int lx,
int ly,
unsigned long readplanemask)
{
	EFF_CLEAR_QUEUE(8);
	EFF_PLNRENBL(readplanemask);
	EFF_SETX0(x0);
	EFF_SETY0(y0);
	EFF_SETX1(x1);
	EFF_SETY1(y1);
	EFF_SETLX(lx);
	EFF_SETLY(ly);
	EFF_COMMAND(0xC0F3);
}
#endif

static Bool
effStoreGlyph(
DrawablePtr pDraw,
unsigned char *image,
int startx,
int stride,
int w,
int h,
BoxRec *screen_box,
effGlFontID_t font_id,
unsigned long glyph_id)
{
	ScreenPtr pScreen = pDraw->pScreen;
	int index;
	effGlGlyphInfo_t glyph_info;
	BoxRec off_screen_box;
	effGlyphListInfo_t *glyph_list;

	glyph_list = &EFF_PRIVATE_DATA(pDraw->pScreen)->glyph_list;
	index = glyph_list->list_index;
	/*
	 * check to see if glyph has already been cached
	 */
	if (effGlReadCacheEntry(pScreen, font_id, glyph_id, &glyph_info))
	{
		/*
		 * store it
		 */
		if (glyph_list->list_index + 1 >= glyph_list->list_size)
			glyph_list->list = (effGlyphList_t *)
			    Xrealloc((char *)glyph_list->list,
			    (glyph_list->list_size += glyph_list->list_size) *
			    sizeof(effGlyphList_t));
		glyph_list->list[index].off_screen_point = glyph_info.coords;
		glyph_list->list[index].screen_box = *screen_box;
		glyph_list->list[index].readplane = glyph_info.readplane;

		++glyph_list->list_index;

		return TRUE;
	}
	/*
	 * see if we can cache this glyph
	 */
	if (effGlAddCacheEntry(pScreen, font_id, glyph_id, w, h, &glyph_info))
	{
		/*
		 * cache position has been allocated, now draw glyph to the
		 * off-screen cache area
		 */
		off_screen_box.x1 = glyph_info.coords.x;
		off_screen_box.y1 = glyph_info.coords.y;
		off_screen_box.x2 = off_screen_box.x1 + w;
		off_screen_box.y2 = off_screen_box.y1 + h;
		effDrawOpaqueMonoImage(&off_screen_box, image, startx, stride,
		    ~0, 0, GXcopy, glyph_info.writeplane, pDraw);
		/*
		 * now store cached glyph
		 */
		if (glyph_list->list_index + 1 >= glyph_list->list_size)
			glyph_list->list = (effGlyphList_t *)
			    Xrealloc((char *)glyph_list->list,
			    (glyph_list->list_size += glyph_list->list_size) *
			    sizeof(effGlyphList_t));
		glyph_list->list[index].off_screen_point = glyph_info.coords;
		glyph_list->list[index].screen_box = *screen_box;
		glyph_list->list[index].readplane = glyph_info.readplane;

		++glyph_list->list_index;

		return TRUE;
	}

	/*
	 * glyph not cached and unable to allocate a cache position, return
	 * failure and use draw mono image to write to screen, hopefully this
	 * will not happen too often!
	 */
	return FALSE;
}

void
effStartGlyphs(
DrawablePtr pDraw,
unsigned long fg,
unsigned char alu,
unsigned long planemask)
{
	effGlyphListInfo_t *glyph_list;

	glyph_list = &EFF_PRIVATE_DATA(pDraw->pScreen)->glyph_list;
	glyph_list->fg = fg;
	glyph_list->alu = alu;
	glyph_list->writeplane = planemask;
}

static void
effFlushGlyphs(
DrawablePtr pDraw)
{
	effGlyphListInfo_t *glyph_list;
	register int i;
	register effGlyphList_t *list;

	glyph_list = &EFF_PRIVATE_DATA(pDraw->pScreen)->glyph_list;
	
	effSetupCardForGlyphBlit(glyph_list->fg, glyph_list->alu,
	    glyph_list->writeplane);
	list = glyph_list->list;
	for (i = 0; i < glyph_list->list_index; ++i, ++list)
		effStretchGlyphBlit(list->off_screen_point.x,
		    list->off_screen_point.y, list->screen_box.x1,
		    list->screen_box.y1,
		    list->screen_box.x2 - list->screen_box.x1 - 1,
		    list->screen_box.y2 - list->screen_box.y1 - 1,
		    list->readplane);
	glyph_list->list_index = 0;
}

void
effDrawMonoGlyphs(
    nfbGlyphInfo *glyph_info,
    unsigned int nglyphs,
    unsigned long fg,
    unsigned char alu,
    unsigned long planemask,
    unsigned long font_private,
    DrawablePtr pDrawable)
{
	int i, w, h;
	BoxPtr pbox;
	void (*DrawMonoImage)();

	DrawMonoImage = (NFB_WINDOW_PRIV(pDrawable))->ops->DrawMonoImage;
	effStartGlyphs(pDrawable, fg, alu, planemask);
	for (i = 0; i < nglyphs; ++i, ++glyph_info)
	{
		pbox = &glyph_info->box;
		w = pbox->x2 - pbox->x1;
		h = pbox->y2 - pbox->y1;
		if (! effStoreGlyph(pDrawable, glyph_info->image, 0,
		    glyph_info->stride, w, h, pbox,
		    (effGlFontID_t)font_private,
		    glyph_info->glyph_id))
			(*DrawMonoImage)(&glyph_info->box, glyph_info->image, 0,
			    glyph_info->stride, fg, alu, planemask, pDrawable);
	}
	effFlushGlyphs(pDrawable);
}

