/*
 *	@(#) nteInit.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 *
 * Modification History
 *
 * S019, Fri Jun 20 11:32:40 PDT 1997, hiramc
 *	fix memory leak, call nfbCloseText8 to balance nfbInitializeText8
 * S018, Fri Jul 01 15:48:41 PDT 1994, kylec
 *	te8 fonts are not used in TBIRD.	
 * S017, 27-Jun-93, hiramc
 *      ready for 864/964 implementations, from DoubleClick
 * S016, 17-Mar-93, hiramc
 *	need to reenable extended registers after vesa mode 6 call
 *	to get the cursor to display.  Also, watch for TBIRD
 *	text interface when -DagaII and don't use DrawFontText
 * S015, 08-Nov-93, staceyc
 * 	set pix trans 928 bug flag to true by default, have never seen
 *	a 928 that doesn't have this problem
 * S014, 23-Aug-93, staceyc
 * 	set screen private cursor ptr to nil in main init code
 * S013, 23-Aug-93, staceyc
 * 	some 928's do not always read via pix trans ext correctly so
 *	mark this with a grafinfo var and avoid it in read image code
 * S012, 20-Aug-93, staceyc
 * 	basic S3 hardware cursor doesn't work for all 8bpp modes (see
 *	STB Pegasus 1280x1024-256) so flag it as unavailable in grafinfo
 *	file
 * S011, 20-Aug-93, staceyc
 * 	basic S3 hardware cursor - only works for 8bpp modes
 * S010, 05-Aug-93, staceyc
 * 	rearrange expression to avoid Microsoft compiler bug, don't use
 *	graf int 10 routine, use graf vb run as it is in tbird X server
 * S009, 02-Aug-93, staceyc
 * 	get clip registers into good shape after init of hardware
 * S008, 22-Jul-93, staceyc
 * 	change name of grafinfo var for vesa bios mode 6
 * S007, 22-Jul-93, staceyc
 * 	try to automatically determine off-screen memory location using
 *	vesa bios mode 6, note that this mode does return some bogus numbers
 *	for N9GXe cards so also need to kludge some calcs based on memory
 *	supplied with card, kludge is flagged with grafinfo variable
 * S006, 13-Jul-93, staceyc
 * 	allow for insufficient off-screen memory for te8 fonts, in that case
 *	reclaim portion of allocated memory for general work area use with
 *	tiles and stipples
 * S005, 09-Jul-93, staceyc
 * 	make room for hardware cursor
 * S004, 17-Jun-93, staceyc
 * 	init work area dimensions
 * S003, 16-Jun-93, staceyc
 * 	fast text and clipping added
 * S002, 11-Jun-93, staceyc
 * 	colormap manipulation is noop for true color
 * S001, 08-Jun-93, staceyc
 * 	initial work
 * S000, 03-Jun-93, staceyc
 * 	created
 */

#include "nteConsts.h"
#include "nteMacros.h"
#include "nteDefs.h"
#include "nteProcs.h"

extern scoScreenInfo NTE(SysInfo);
extern VisualRec NTE(Visual);
extern nfbGCOps NTE(SolidPrivOps);
extern nfbWinOps NTE(WinOps);

int NTE(Generation) = -1;
int NTE(ScreenPrivateIndex) = -1;
unsigned char NTE(RasterOps)[16];

static void
nteInitRasterOps()
{
	NTE(RasterOps)[GXclear]	 	= 0x01;
	NTE(RasterOps)[GXand] 		= 0x0C;
	NTE(RasterOps)[GXandReverse] 	= 0x0D;
	NTE(RasterOps)[GXcopy] 		= 0x07;
	NTE(RasterOps)[GXandInverted] 	= 0x0E;
	NTE(RasterOps)[GXnoop] 		= 0x03;
	NTE(RasterOps)[GXxor] 		= 0x05;
	NTE(RasterOps)[GXor] 		= 0x0B;
	NTE(RasterOps)[GXnor] 		= 0x0F;
	NTE(RasterOps)[GXequiv] 	= 0x06;
	NTE(RasterOps)[GXinvert] 	= 0x00;
	NTE(RasterOps)[GXorReverse] 	= 0x0A;
	NTE(RasterOps)[GXcopyInverted] 	= 0x04;
	NTE(RasterOps)[GXorInverted] 	= 0x09;
	NTE(RasterOps)[GXnand] 		= 0x08;
	NTE(RasterOps)[GXset]	 	= 0x02;
}

Bool
NTE(Probe)(ddxDOVersionID version,ddxScreenRequest *pReq)
{
    return ddxAddPixmapFormat(pReq->dfltDepth, pReq->dfltBpp, pReq->dfltPad);
}

Bool
NTE(InitHW)(pScreen)
    ScreenPtr pScreen;
{
	NTE(SetGraphics)(pScreen);

	return TRUE;
}

static Bool
nteInitTE8(
	ScreenRec *pScreen,
	ntePrivateData_t *ntePriv)
{
	int os_glyphs_per_oswidth, os_glyphs_per_osheight;
	int os_glyphs_per_plane, os_fonts_per_plane;
	int index, plane, i, j, osx, osy;

#if defined(TBIRD_TEXT_INTERFACE)			/*	S016	*/
		return FALSE;
#endif

	os_glyphs_per_oswidth = ntePriv->oswidth / NTE_TE8_GLYPH_SIZE;
	os_glyphs_per_osheight = (ntePriv->osheight / NTE_TE8_GLYPH_SIZE) / 2;
	os_glyphs_per_plane = os_glyphs_per_oswidth * os_glyphs_per_osheight;
	if (os_glyphs_per_plane < NFB_TEXT8_SIZE)
	{
		ntePriv->te8_font_count = 0;
		return FALSE;
	}

	NTE(WinOps).DrawFontText = NTE(DrawFontText);
	os_fonts_per_plane = os_glyphs_per_plane / NFB_TEXT8_SIZE;
	ntePriv->te8_font_count = os_fonts_per_plane * ntePriv->depth;
	if (ntePriv->te8_font_count > NTE_TE8_FONT_MAX)
		ntePriv->te8_font_count = NTE_TE8_FONT_MAX;
	ntePriv->te8_fonts = (nteTE8Font_t *)xalloc(ntePriv->te8_font_count
	    * sizeof(nteTE8Font_t));

	index = 0;
	plane = 0;
	while (index < ntePriv->te8_font_count && plane < ntePriv->depth)
	{
		i = 0;
		osx = 0;
		osy = 0;
		while (index < ntePriv->te8_font_count &&
		    i < os_fonts_per_plane)
		{
			ntePriv->te8_fonts[index].readplane = 1 << plane;
			for (j = 0; j < NFB_TEXT8_SIZE; ++j)
			{
				ntePriv->te8_fonts[index].coords[j].x =
				    osx + ntePriv->osx;
				ntePriv->te8_fonts[index].coords[j].y =
				    osy + ntePriv->osy;
				osx += NTE_TE8_GLYPH_SIZE;
				if (osx >= ntePriv->oswidth)
				{
					osx = 0;
					osy += NTE_TE8_GLYPH_SIZE;
				}
			}
			++index;
			++i;
		}
		++plane;
	}

	nfbInitializeText8(pScreen, ntePriv->te8_font_count, NTE_TE8_GLYPH_SIZE,
	    NTE_TE8_GLYPH_SIZE, NTE(DownloadFont8), 0);

	return TRUE;
}

typedef struct os_size_t {
	int x, y;
	int width, height;
	int area;
	} os_size_t;

static Bool
nteGetOffScreenDimensions(
	ScreenPtr pScreen)
{
        grafData *grafinfo = DDX_GRAFINFO(pScreen);
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);
	unsigned long disp_mem;
	unsigned short confg_reg1, confg_reg2, confg_reg;
	unsigned int regs[4];
	unsigned int pixels_per_scanline, bytes_per_scanline, scanline_count;
	os_size_t os1, os2, largest;
	int vesa_bios_mode_6_bug;

	if (! grafGetInt(grafinfo, "VESABIOS6BUG", &vesa_bios_mode_6_bug))
		vesa_bios_mode_6_bug = 0;

	NTE_OUTB(ntePriv->crtc_adr, NTE_CONFG_REG1_INDEX);
	confg_reg1 = NTE_INB(ntePriv->crtc_data);
	NTE_OUTB(ntePriv->crtc_adr, NTE_CONFG_REG2_INDEX);
	confg_reg2 = NTE_INB(ntePriv->crtc_data);
	confg_reg = (confg_reg2 << 8) | confg_reg1;


	switch (confg_reg & NTE_DISP_MEM_SIZE_MASK)
	{
	    case NTE_4MEG :
		disp_mem = NTE_1MEGABYTE * 4;
		break;
	    case NTE_3MEG :
		disp_mem = NTE_1MEGABYTE * 3;
		break;
	    case NTE_2MEG :
		disp_mem = NTE_1MEGABYTE * 2;
		break;
	    case NTE_1MEG :
		disp_mem = NTE_1MEGABYTE;
		break;
	    case NTE_HALF_MEG :
		disp_mem = NTE_1MEGABYTE / 2;
		ErrorF("nte: Card says it only has 0.5 meg of memory!\n");
		ErrorF("Half meg cards have not been tested with this ");
		ErrorF("driver, let's see what happens.\n");
		break;
	    default :
		ErrorF("nte: Invalid confg_reg=%x\n", confg_reg);
		FatalError("nte: Unable to determine card memory size!\n");
		break;
	}

	/*
	 * see S3 86C928 Video BIOS OEM Guide page 12 to find out what is
	 * going on here
	 */

#define NTE_EAX 0
#define NTE_EBX 1
#define NTE_ECX 2
#define NTE_EDX 3

	regs[NTE_EAX] = 0x4F06;
	regs[NTE_EBX] = 0x0001;
	regs[NTE_ECX] = 0;
	regs[NTE_EDX] = 0;

	grafVBRun(grafinfo, regs, 4);

/* The VESA Bios call 6 disabled the extended regs access, reenable S016 vvv */
	NTE_OUTB(ntePriv->crtc_adr, NTE_LOCK_REG_1_INDEX);
	NTE_OUTB(ntePriv->crtc_data, NTE_UNLOCK_S3_REGS);
	NTE_OUTB(ntePriv->crtc_adr, NTE_LOCK_REG_2_INDEX);
	NTE_OUTB(ntePriv->crtc_data, NTE_UNLOCK_S3CTL_EXT_REGS);
	NTE_OUTB(ntePriv->crtc_adr, NTE_SYS_CNFG_INDEX);
	NTE_OUTB(ntePriv->crtc_data,
		( NTE_INB(ntePriv->crtc_data) | NTE_ENABLE_ENHANCED_REGS_BIT ));
						/*		S016 ^^^ */

	if ((regs[NTE_EAX] & 0xFF00) != 0)
	{
		ErrorF("nte: BIOS scanline size query failed, continuing...\n");
		return FALSE;
	}

	scanline_count = regs[NTE_EDX];
	pixels_per_scanline = regs[NTE_ECX];
	bytes_per_scanline = regs[NTE_EBX];

	if (pixels_per_scanline <= 0)
		return FALSE;

	if (vesa_bios_mode_6_bug)
	{
		/* attempt to recalc correct values */
#if NTE_BITS_PER_PIXEL == 8 || NTE_BITS_PER_PIXEL == 16
		pixels_per_scanline = bytes_per_scanline / (ntePriv->depth / 8);
		scanline_count = (disp_mem  / (ntePriv->depth / 8))
		    / pixels_per_scanline;
#endif
#if NTE_BITS_PER_PIXEL == 24
		/* assume unpacked, grrrr, not good if really packed!  */
		pixels_per_scanline = bytes_per_scanline / 4;
		scanline_count = (disp_mem / 4) / pixels_per_scanline;
#endif
	}

	/* assumption that visible screen origin is 0,0 */
	os1.x = ntePriv->width;
	os1.y = 0;
	os1.width = pixels_per_scanline - ntePriv->width;
	os1.height = scanline_count;
	os1.area = os1.width * os1.height;

	os2.x = 0;
	os2.y = ntePriv->height;
	os2.width = pixels_per_scanline;
	os2.height = scanline_count - ntePriv->height;
	os2.area = os2.width * os2.height;

	if (os1.area > os2.area)
		largest = os1;
	else
		largest = os2;

	if (largest.area <= 0 || largest.area <= pixels_per_scanline)
		return FALSE;

	ntePriv->osx = largest.x;
	ntePriv->osy = largest.y;
	ntePriv->oswidth = largest.width;
	ntePriv->osheight = largest.height;

	return TRUE;
}

Bool
NTE(Init)(
	int index,
	ScreenPtr pScreen,
	int argc,
	char **argv)
{
        grafData *grafinfo = DDX_GRAFINFO(pScreen);
	nfbScrnPrivPtr pNfb;
	ntePrivateData_t *ntePriv;
	int width, height, depth, rgbbits, mmx, mmy;
	int use_hw_cursor;

	if (NTE(Generation) != serverGeneration)
	{
		NTE(ScreenPrivateIndex) = AllocateScreenPrivateIndex();
		NTE(Generation) = serverGeneration;
		nteInitRasterOps();
	}

	ntePriv = (ntePrivateData_t *)xalloc(sizeof(ntePrivateData_t));
	NTE_PRIVATE_DATA(pScreen) = ntePriv;

	/* Get mode and monitor info */
	if (! grafGetInt(grafinfo, "PIXWIDTH",  &width)  ||
	    ! grafGetInt(grafinfo, "PIXHEIGHT", &height) ||
	    ! grafGetInt(grafinfo, "DEPTH",     &depth))
	{
		ErrorF("NTE(): can't find pixel info in grafinfo file.\n");
		return FALSE;
	}
	if (depth <= 8)
	{
		if (! grafGetInt(grafinfo, "RGBBITS", &rgbbits))
			rgbbits = 6;
		NTE(Visual).bitsPerRGBValue = rgbbits;
		ntePriv->dac_shift = sizeof(unsigned short) * 8 - rgbbits;
	}

	ntePriv->width = width;
	ntePriv->height = height;
	ntePriv->depth = depth;

	if (! grafGetInt(grafinfo, "PIXTRANSEXTBUG",
	    &ntePriv->pix_trans_ext_bug))
		ntePriv->pix_trans_ext_bug = 1;

#if ! NTE_USE_IO_PORTS
	if (! grafGetMemInfo(grafinfo, NULL, NULL, NULL, &ntePriv->regs))
	{
		ErrorF("nte: Missing MEMORY in grafinfo file.\n");
		return FALSE;
	}
#endif

#if NTE_MAP_LINEAR
	if (! grafGetMemInfo(grafinfo, NULL, NULL, NULL, &ntePriv->fbPointer))
	{
		ErrorF("nte: Missing MEMORY in grafinfo file.\n");
		return FALSE;
	}
#endif /* NTE_MAP_LINEAR */

	mmx = 300; mmy = 300;  /* Reasonable defaults */

	grafGetInt(grafinfo, "MON_WIDTH",  &mmx);
	grafGetInt(grafinfo, "MON_HEIGHT", &mmy);

	if (! nfbScreenInit(pScreen, width, height, mmx, mmy))
		return FALSE;

	if (! nfbAddVisual(pScreen, &NTE(Visual)))
		return FALSE;

	pNfb = NFB_SCREEN_PRIV(pScreen);
	pNfb->protoGCPriv->ops	 = &NTE(SolidPrivOps);
	pNfb->BlankScreen	 = NTE(BlankScreen);
	pNfb->ValidateWindowPriv = NTE(ValidateWindowPriv);
	pNfb->clip_count = 1;
#if NTE_BITS_PER_PIXEL == 8
	pNfb->LoadColormap = genLoadColormap;
	pNfb->SetColor = NTE(SetColor);
#else
	pNfb->LoadColormap = NoopDDA;
	pNfb->SetColor = NoopDDA;
#endif

	if (! NTE(InitHW)(pScreen))
		return FALSE;

	if (NTE_INB(NTE_MISC_READ) & NTE_IOA_SEL)
	{
		ntePriv->crtc_adr = NTE_CRTC_ADR_COLOR;
		ntePriv->crtc_data = NTE_CRTC_DATA_COLOR;
	}
	else
	{
		ntePriv->crtc_adr = NTE_CRTC_ADR_MONO;
		ntePriv->crtc_data = NTE_CRTC_DATA_MONO;
	}

	if (! grafGetInt(grafinfo, "OSX", &ntePriv->osx) ||
	    ! grafGetInt(grafinfo, "OSY", &ntePriv->osy) ||
	    ! grafGetInt(grafinfo, "OSWIDTH", &ntePriv->oswidth) ||
	    ! grafGetInt(grafinfo, "OSHEIGHT", &ntePriv->osheight))
		if (! nteGetOffScreenDimensions(pScreen))
		{
			ntePriv->osx = 0;
			ntePriv->osy = 0;
			ntePriv->oswidth = 0;
			ntePriv->osheight = 0;
		}

	ntePriv->wax = ntePriv->osx;
	ntePriv->way = ntePriv->osy + ntePriv->osheight / 2;
	ntePriv->wawidth = ntePriv->oswidth;
	ntePriv->waheight = ntePriv->osheight / 2;

	ntePriv->clip_x = ntePriv->osx + ntePriv->oswidth;
	ntePriv->clip_y = ntePriv->osy + ntePriv->osheight;
	if (ntePriv->width > ntePriv->clip_x)
		ntePriv->clip_x = ntePriv->width;
	if (ntePriv->height > ntePriv->clip_y)
		ntePriv->clip_y = ntePriv->height;
	--ntePriv->clip_x;
	--ntePriv->clip_y;

	if (ntePriv->oswidth)
		if (! nteInitTE8(pScreen, ntePriv))
		{
			ntePriv->wax = ntePriv->osx;
			ntePriv->way = ntePriv->osy;
			ntePriv->wawidth = ntePriv->wawidth;
			ntePriv->waheight = ntePriv->osheight;
		}

	if (grafGetInt(grafinfo, "HWCURSORY", &ntePriv->hw_cursor_y))
		use_hw_cursor = TRUE;
	else
	{
		use_hw_cursor = ntePriv->oswidth >= NTE_HW_CURSOR_DATA_SIZE &&
		    ntePriv->osx == 0 && ntePriv->depth == 8;
		if (use_hw_cursor)
			if (! grafGetInt(grafinfo, "S3HWCURSOR",
			    &use_hw_cursor))
				use_hw_cursor = TRUE;
		if (use_hw_cursor)
		{
			ntePriv->hw_cursor_y = ntePriv->way +
			    ntePriv->waheight - 1;
			--ntePriv->waheight;
		}
	}

#if NTE_BITS_PER_PIXEL == 8
	ntePriv->cursor = 0;
#endif

#if NTE_MAP_LINEAR
	{
		unsigned char v;

		NTE_OUTB(ntePriv->crtc_adr, 0x50);	/* EX_SCTL_1 */
		v = NTE_INB(ntePriv->crtc_data);

		switch (v & 0xc1) {			/* CR50[7:6], CR50[0] */
		case 0x00:
			ntePriv->fbStride = 1024;
			break;
		case 0x40:
			ntePriv->fbStride = 640;
			break;
		case 0x80:
			ntePriv->fbStride = 800	;
			break;
		case 0xc0:
			ntePriv->fbStride = 1280;
			break;
		case 0x01:
			ntePriv->fbStride = 1152;
			break;
		case 0x81:
			ntePriv->fbStride = 1600;
			break;
		case 0x41:
		case 0xc1:
		default:
			ErrorF("nte: invalid system control register=%x\n", v);
			FatalError("nte: Failed to read screen pixel size\n");
			/* NOTREACHED */
		}
	}
#endif /* NTE_MAP_LINEAR */

	if (use_hw_cursor)
		NTE(CursorInitialize)(pScreen);
	else
		scoSWCursorInitialize(pScreen);

	/*
	 * This should work for most cases.
	 */
	if (((pScreen->rootDepth == 1) ? mfbCreateDefColormap(pScreen) :
		cfbCreateDefColormap(pScreen)) == 0 )
	    return FALSE;

	/* 
	 * Give the sco layer our screen switch functions.  
	 * Always do this last.
	 */
	scoSysInfoInit(pScreen, &NTE(SysInfo));

	/*
	 * Set any NFB runtime options here
	 */
	nfbSetOptions(pScreen, NFB_VERSION, 0, 0);

	NTE(RestoreGState)(pScreen);

	return TRUE;

}

Bool
NTE(FreeScreen)(
	int index,
	ScreenPtr pScreen)
{
	ntePrivateData_t *ntePriv = NTE_PRIVATE_DATA(pScreen);

#if !defined(TBIRD_TEXT_INTERFACE)			/*	S018	*/
	if (ntePriv->oswidth)
		if (ntePriv->te8_font_count)
			xfree(ntePriv->te8_fonts);
#endif
	nfbCloseText8(pScreen);		/*	S019	*/
	xfree(ntePriv);

	return TRUE;
}
