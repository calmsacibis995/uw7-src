/*
 *	@(#) effInit.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * Modification History
 *
 * S051, 04-Sep-97, hiramc
 *	do not need the ioctl SW_VGA12 in gemini
 * S050, 16-Nov-94, brianm
 *	added in initialization to current_clip from card_clip.
 * S049, 22-Nov-93, staceyc
 * 	init screen blank state
 * S048, 15-Sep-93, mikep
 *	current clip is meaningless at effHWInit time.  Use card_clip.
 *	remove obsolete GC Cache calls
 * S047, 27-Aug-93, buckm
 *	screen privates now have card_clip and current_clip.
 * S046, 28-Jan-93, staceyc
 * 	boards using the C&T 82C481 have a problem that needs a flag in the
 *	screen private
 * S045, 19-Jan-92, chrissc
 *	added ati specific clip routine.
 * S044, 20-Nov-92, staceyc
 * 	remove all remnants of third party hardware mods
 * S043, 20-Nov-92, staceyc
 * 	extra parameter to nfb for text8 init
 * S042, 01-Nov-92, mikep
 *	add more options to nfbSetOptions()
 * S041, 23-Oct-92, staceyc
 * 	merged pixelworks driver back into eff code
 * S040, 16-Oct-92, staceyc
 * 	move text8 init code into nfb
 * S039, 15-Sep-92, mikep
 *	Remove setting of pScreen->CloseScreen.  NFB drivers shouldn't
 *	do that.
 * S038, 15-Sep-92, mikep
 *	Remove call to nfb close screen, add call to nfb set options.
 * S037, 04-Sep-92, mikep
 *	Remove Cursor close routine
 * S036, 04-Sep-92, staceyc
 * 	fast text and clipping added, and its mogrify, read your Calvin and
 *	Hobbes
 * S035, 13-Mar-92, mikep
 *	add call to effInitTransMorgrify()
 * S034, 14-Nov-91, staceyc
 * 	512K card support
 * S033, 28-Oct-91, staceyc
 * 	move dac save routine to init instead of inithw
 * S032, 13-Oct-91, mikep@sco.com
 *	call scoSysInit() last to avoid FatalError() conflicts.
 * S031, 10-Oct-91, mikep@sco.com
 *      change sco.h to scoext.h.  Remove commonDDX.h
 * S030, 26-Sep-91, mikep@sco.com
 *	simplify effInit() via nfbScreenInit().  Remove initialization code
 *	from effProbe().
 * S029, 26-Sep-91, staceyc
 * 	save initial VGA 8514 DAC state
 * S028, 24-Sep-91, staceyc
 * 	don't save cache in memory during screen switch
 * S027, 21-Sep-91, mikep@sco.com
 *	change genGC routines to nfbGC routines.
 * S026, 20-Sep-91, staceyc
 * 	backked out nugget size code
 * S025, 18-Sep-91, staceyc
 * 	go back to using gen gcops caching code
 * S024, 13-Sep-91, staceyc
 * 	replaced exits with fatal error calls
 * S023, 13-Sep-91, buckm
 *	nfbGenericScreenInit changed to nfbProtoScreenInit;
 *	nfbCloseScreen changed to nfbProtoCloseScreen.
 * S022, 12-Sep-91, pavelr
 *	added NULL parameter to grafExec call
 * S021, 09-Sep-91, mikep@sco.com
 * 	add call to ddxAddPixmapFormat()
 * S020, 09-Sep-91, staceyc
 * 	added rgb significant bits and nugget size
 * S019, 05-Sep-91, staceyc
 * 	allow off-screen memory to be moved to any x, y coord
 * S018, 04-Sep-91, staceyc
 * 	move off-screen memory metrics to grafinfo file
 * S017, 28-Aug-91, staceyc
 * 	new grafinfo api
 * S016, 28-Aug-91, staceyc
 * 	reworked command queue use - code cleanup - init glyph list struct
 * S015, 21-Aug-91, staceyc
 * 	initialize stipple off screen blit area metrics
 * S014, 15-Aug-91, staceyc
 * 	added glyph caching init and close
 * S013, 13-Aug-91, staceyc
 * 	replaced gen GC code with eff version
 * S012, 06-Aug-91, staceyc
 * 	removed debug code for mod S010
 * S011, 06-Aug-91, mikep
 *	add class to genGCOpsCacheInit()
 * S010, 06-Aug-91, staceyc
 * 	draw mono image support
 * S009, 26-Jul-91, staceyc
 * 	hardware cursor init
 * S008, 23-Jul-91, staceyc
 * 	move grafinfo stuff to probe - this will be passed to probe later
 * S007, 16-Jul-91, staceyc
 * 	change cursor init paramters per mikep
 * S006, 16-Jul-91, staceyc
 * 	new cursor init
 * S005, 28-Jun-91, staceyc
 * 	incorporated grafinfo - support the two standard 8514 resolutions
 *	and plane depths - still some problems with depths of 4
 * S004, 25-Jun-91, mikep@sco.com
 * 	rearranged include files
 * S003, 25-Jun-91, staceyc
 * 	initial probe work
 * S002, 24-Jun-91, staceyc
 * 	added includes for a cleaner compile
 * S001, 18-Jun-91, staceyc
 * 	raster ops and close code added
 * S000, 14-Jun-91, staceyc@sco.com
 * 	first attempt at HW init
 */

#ifdef usl
#include <sys/types.h>
#include <sys/kd.h>
#else
#include <sys/console.h>
#endif
#include "X.h"
#include "misc.h"
#include "gcstruct.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "colormap.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "colormapst.h"
#include "scoext.h"
#include "ddxScreen.h"
#include "nfbDefs.h"
#include "nfbWinStr.h"
#include "nfbGCStr.h"
#include "nfbScrStr.h"
#include "nfbProcs.h"
#include "genDefs.h"
#include "genProcs.h"
#include "effConsts.h"
#include "effMacros.h"
#include "effDefs.h"
#include "effProcs.h"

extern nfbWinOps effWinOps;				/* S045 */

int effRasterOps[16];
int effScreenPrivateIndex = -1;

static void effBuildRasterOps();

static void
effGrafinfoMustGetInt(
grafData *graf_data,
char *token,
int *value)
{
	if (grafGetInt(graf_data, token, value) == FAILURE)
		FatalError("EFF: need grafinfo value for %s\n", token);
}

/*
 * effProbe() - test for graphics hardware
 *
 * This routine tests for your particular graphics board, and returns
 * true if its present, false otherwise.
 */
Bool
effProbe(
ddxDOVersionID version,
ddxScreenRequest *pReq)
{
	int present;
	unsigned int width, height, nplanes;


	if (! grafQuery(pReq->grafinfo, "IGNOREPROBE"))
	{
		EFF_OUT(EFF_ERROR_ACC, 0x0123);
		present = EFF_INW(EFF_ERROR_ACC) == 0x0123;

		if (! present)
			return FALSE;
	}

	effBuildRasterOps();
	effInitTransMorgrify();					/* S035 */

	if (grafGetInt(pReq->grafinfo, "DEPTH", &nplanes) == FAILURE)
		return FALSE;

	if (nplanes != 4 && nplanes != 8)
	{
		ErrorF("effProbe(): bad grafinfo DEPTH(%d)\n", nplanes);
		return FALSE;
	}

	if (!ddxAddPixmapFormat(pReq->dfltDepth, pReq->dfltBpp, pReq->dfltPad))
		return FALSE;

	if (grafGetInt(pReq->grafinfo, "PIXWIDTH", &width) == FAILURE ||
	    grafGetInt(pReq->grafinfo, "PIXHEIGHT", &height) == FAILURE)
		return FALSE;

	return TRUE;
}

/*
 * effInitHW(ScreenPtr)
 *
 * Template for machine dependent hardware initialization code
 * This should do everything it takes to get the machine ready to draw.
 */
void
effInitHW(ScreenPtr pScreen)
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);

#if ! defined(usl)		/*	S051	*/
	/*
	 * If the grafinfo says so, put the VGA into graphics mode
	 * The sco layer will reset when the server exits.
	 */
	if (grafQuery(effPriv->graf_data, "SETVGA"))
		ioctl(1, SW_VGA12, 0);
#endif

	if (grafExec(effPriv->graf_data, "SetGraphics", NULL) == FAILURE)
	       FatalError("EFF: SetGraphics not specified in grafinfo file!\n");

	EFF_CLEAR_QUEUE(8);
	EFF_OUT(EFF_CONTROL, 0x9000);                       /* reset brecon */
	EFF_OUT(EFF_SEC_DECODE, 0x5006);                      /* set config */
	EFF_OUT(EFF_CONTROL, 0x400F);                        /* set constat */
	EFF_SETCOL0(0);
	EFF_SETCOL1(EFF_ALLPLANES);
	EFF_PLNWENBL(EFF_ALLPLANES);          /* enable all planes for write */
	EFF_PLNRENBL(EFF_ALLPLANES);           /* enable all planes for read */
	EFF_SETFN0(EFF_FNCOLOR0, EFF_FNREPLACE);

	EFF_CLEAR_QUEUE(8);
	EFF_SETFN1(EFF_FNCOLOR1, EFF_FNREPLACE);
	EFF_SETMODE(EFF_M_ONES);
	EFF_SETPAT0(0);
	EFF_SETPAT1(0);
	EFF_SETXMIN(effPriv->card_clip.x1);			/* S048 */
	EFF_SETYMIN(effPriv->card_clip.y1);
	EFF_SETXMAX(effPriv->card_clip.x2 - 1);
	EFF_SETYMAX(effPriv->card_clip.y2 - 1);
}

Bool
effInit(index, pScreen, argc, argv)
	int index;
	ScreenPtr pScreen;
	int argc;
	char **argv;
{
	effPrivateData_t *effPriv;
	unsigned int off_screen_width;
	int off_screen_x2, off_screen_y2, rgb_bits;
	effCursorData_t *effScrCurs;
	grafData *grafinfo;
	int repbug;
	nfbScrnPrivPtr pNfb;
	int mmx, mmy;
	int i;
	extern VisualRec effVisual;
	extern scoScreenInfo effSysInfo;
	extern nfbGCOps effSolidPrivOps;

#ifdef DEBUG
	ErrorF("effInit(index=%d pScreen=0x%.8x)\n", index, pScreen);
#endif

	effScreenPrivateIndex = AllocateScreenPrivateIndex();
	effPriv = (effPrivateData_t *)xalloc(sizeof(effPrivateData_t));
	EFF_PRIVATE_DATA(pScreen) = effPriv;

	grafinfo = DDX_GRAFINFO(pScreen);
	effPriv->graf_data = grafinfo;
	effGrafinfoMustGetInt(grafinfo, "PIXWIDTH", &effPriv->width);
	effGrafinfoMustGetInt(grafinfo, "PIXHEIGHT", &effPriv->height);
	effGrafinfoMustGetInt(grafinfo, "DEPTH", &effPriv->depth);

	if (effPriv->width <= 0 || effPriv->height <= 0 || effPriv->depth <= 0)
	{
		ErrorF("8514a: grafinfo error -");
		ErrorF(" invalid dimensions (%dx%dx%d)\n", effPriv->width,
		    effPriv->height, effPriv->depth);
		return 0;
	}

	if ( !grafGetInt(grafinfo, "MON_WIDTH",  &mmx) ||
	     !grafGetInt(grafinfo, "MON_HEIGHT", &mmy)) {
	    mmx = 274; mmy = 207;	/* Assume 14" diagonal */
	}

	if (grafGetInt(grafinfo, "RGBBITS", &rgb_bits) != FAILURE)
		effVisual.bitsPerRGBValue = rgb_bits;
	effPriv->dac_shift = sizeof(short) * 8 - effVisual.bitsPerRGBValue;
	effVisual.nplanes = effPriv->depth;
	effVisual.ColormapEntries = 1 << effPriv->depth;

	effPriv->screen_blanked = FALSE;

	if (grafGetInt(grafinfo, "REPBUG", &repbug) != FAILURE && repbug)
		effPriv->APBlockOutW = effSlowAPBlockOutW;
	else
		effPriv->APBlockOutW = effAPBlockOutW;

	effGrafinfoMustGetInt(grafinfo, "OFFSCRX", &effPriv->all_off_screen.x);
	effGrafinfoMustGetInt(grafinfo, "OFFSCRY", &effPriv->all_off_screen.y);
	effGrafinfoMustGetInt(grafinfo, "OFFSCRW", &effPriv->all_off_screen.w);
	effGrafinfoMustGetInt(grafinfo, "OFFSCRH", &effPriv->all_off_screen.h);

	if (grafGetInt(grafinfo, "PALWRITE", &effPriv->eff_pal.write_addr)
	    == FAILURE)
		effPriv->eff_pal.write_addr = EFF_PALWRITE_ADDR;
	if (grafGetInt(grafinfo, "PALREAD", &effPriv->eff_pal.read_addr)
	    == FAILURE)
		effPriv->eff_pal.read_addr = EFF_PALREAD_ADDR;
	if (grafGetInt(grafinfo, "PALMASK", &effPriv->eff_pal.mask) == FAILURE)
		effPriv->eff_pal.mask = EFF_PALMASK;
	if (grafGetInt(grafinfo, "PALDATA", &effPriv->eff_pal.data) == FAILURE)
		effPriv->eff_pal.data = EFF_PALDATA;

	if (grafQuery(grafinfo, "ATICLIPREG"))			/* S045 */
                effWinOps.SetClipRegions = atiSetClipRegions;

	/* S046 */
	if (grafGetInt(grafinfo, "F82C481", &effPriv->f82c481) == FAILURE)
		effPriv->f82c481 = 0;

	effScrCurs = &effPriv->cursor;
	effScrCurs->save_max_size = EFF_MAX_CURSOR_SIZE;
	effScrCurs->cursor_max_size = EFF_MAX_CURSOR_SIZE - SPRITE_PAD * 2;

	/*
	 * see if off-screen is wide enough, need room for the cursor, mask,
	 * save area, and tile blit
	 */
	off_screen_width = effScrCurs->save_max_size +
	    effScrCurs->cursor_max_size * 2 + 10;

	if (effPriv->all_off_screen.h < 128 || off_screen_width >=
	    effPriv->all_off_screen.w)
		FatalError("%s %d %s",
		    "EFF: off-screen memory needs at least 128 lines and be >",
		    off_screen_width, " pixels wide.\n");

	/*
	 * define hardware clipping limits
	 */
	off_screen_x2 = effPriv->all_off_screen.x +
	    effPriv->all_off_screen.w;
	off_screen_y2 = effPriv->all_off_screen.y +
	    effPriv->all_off_screen.y;
	effPriv->card_clip.x1 = 0;
	effPriv->card_clip.y1 = 0;
	effPriv->card_clip.x2 = max(effPriv->width,  off_screen_x2);
	effPriv->card_clip.y2 = max(effPriv->height, off_screen_y2);
	effPriv->current_clip = effPriv->card_clip; /* S050 */

	/*
	 * cursor off-screen memory metrics
	 */
	effScrCurs->save_under.x = effPriv->all_off_screen.x;
	effScrCurs->save_under.y = effPriv->all_off_screen.y;
	effScrCurs->cursor_bitmap.x = effScrCurs->save_under.x +
	    effScrCurs->save_max_size;
	effScrCurs->cursor_bitmap.y = effPriv->all_off_screen.y;
	effScrCurs->mask_bitmap.x = effScrCurs->cursor_bitmap.x +
	    effScrCurs->cursor_max_size;
	effScrCurs->mask_bitmap.y = effPriv->all_off_screen.y;

	/*
	 * glyph cache, mono image, stipple off-screen metrics
	 */
	effPriv->off_screen_blit_area.x = effPriv->all_off_screen.x;
	effPriv->off_screen_blit_area.y = effPriv->all_off_screen.y +
	    effScrCurs->save_max_size;
	effPriv->off_screen_blit_area.w = effPriv->all_off_screen.w;
	effPriv->off_screen_blit_area.h = effPriv->all_off_screen.y +
	    effPriv->all_off_screen.h - effPriv->off_screen_blit_area.y;

	/*
	 * tile build area
	 */
	effPriv->tile_blit_area.x = effScrCurs->mask_bitmap.x +
	    effScrCurs->cursor_max_size;
	effPriv->tile_blit_area.y = effPriv->all_off_screen.y;
	effPriv->tile_blit_area.w = effPriv->all_off_screen.w +
	    effPriv->all_off_screen.x - effPriv->tile_blit_area.x;
	effPriv->tile_blit_area.h = effScrCurs->save_max_size;

	effPriv->glyph_list.list_size = 256;        /* resizable in the code */
	effPriv->glyph_list.list_index = 0;
	effPriv->glyph_list.list = (effGlyphList_t *)
	    Xalloc(effPriv->glyph_list.list_size * sizeof(effGlyphList_t));

	if (!nfbScreenInit(pScreen, effPriv->width, effPriv->height, mmx, mmy))
		return FALSE;

	if (!nfbAddVisual(pScreen, &effVisual))
		return FALSE;

	pScreen->QueryBestSize = effQueryBestSize;

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->protoGCPriv->ops	 = &effSolidPrivOps;
	pNfb->SetColor		 = effSetColor;
	pNfb->LoadColormap	 = effLoadColormap;
	pNfb->BlankScreen	 = effBlankScreen;
	pNfb->ClearFont 	 = effGlClearCache;
	pNfb->ValidateWindowPriv = effValidateWindowPriv;

	pNfb->clip_count	 = 1;

	/*
	 * fast 8 bit text data setup
	 */
	effInitializeText8(pScreen);
	nfbInitializeText8(pScreen, effPriv->text8_font_count, EFF_TEXT8_WIDTH,
	    EFF_TEXT8_HEIGHT, effDownloadFont8, 0);

	/*
	 * if it has vga and we can talk to the card now then
	 * dac state
	 */
	if (! grafQuery(effPriv->graf_data, "IGNOREPROBE"))
		effSaveDACState(pScreen);
	effInitHW(pScreen);

	effDCInitialize(pScreen);
	effGlCacheInit(pScreen);

	if (!cfbCreateDefColormap(pScreen))
	    return FALSE;

	scoSysInfoInit(pScreen, &effSysInfo);

	nfbSetOptions(pScreen, NFB_VERSION, 0, 0);

	return TRUE;
}

Bool
effCloseScreen(index, pScreen)
	int index;
	ScreenPtr pScreen;
{
	effPrivateData_t *effPriv = EFF_PRIVATE_DATA(pScreen);

	effGlCacheClose(pScreen);                   /* free glyph cache data */
	effCloseText8(pScreen);
	nfbCloseText8(pScreen);
	Xfree((char *)effPriv->glyph_list.list);             /* glyph buffer */
	Xfree((char *)effPriv);                     /* free eff private data */

	return TRUE;
}

static void effBuildRasterOps()
{
	effRasterOps[GXclear]	 	= 0x01;
	effRasterOps[GXand] 		= 0x0C;
	effRasterOps[GXandReverse] 	= 0x0D;
	effRasterOps[GXcopy] 		= 0x07;
	effRasterOps[GXandInverted] 	= 0x0E;
	effRasterOps[GXnoop] 		= 0x03;
	effRasterOps[GXxor] 		= 0x05;
	effRasterOps[GXor] 		= 0x0B;
	effRasterOps[GXnor] 		= 0x0F;
	effRasterOps[GXequiv] 		= 0x06;
	effRasterOps[GXinvert] 		= 0x00;
	effRasterOps[GXorReverse] 	= 0x0A;
	effRasterOps[GXcopyInverted] 	= 0x04;
	effRasterOps[GXorInverted] 	= 0x09;
	effRasterOps[GXnand] 		= 0x08;
	effRasterOps[GXset]	 	= 0x02;
}

