#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiInit.c	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/kd.h>

#include <ati.h>

Mach32DispRegs *mach32_regP;
Mach32WeightRegs *mach32_weightP;

extern SIBool init_graphics();
extern void uninit_graphics();

static int ati_fd;
static float mon_w, mon_h;

static SIBool checkConfig(configP)
SIConfigP configP;

{
    int x, y, freq;
    int i;

#if (BPP == 16)
    int wgt;
#elif (BPP == 24)
    char w[5];
#endif

    if (lfbstrcasecmp(configP->chipset, "Mach32")) {
	fprintf(stderr, "Wrong chipset for driver (%s)\n", configP->chipset);
	return(SI_FAIL);
    }

    if (configP->depth != BPP) {
	fprintf(stderr,
		"Bad depth %d, should be $d\n",
		configP->depth, BPP);
	return(SI_FAIL);
    }

    x = configP->disp_w;
    y = configP->disp_h;
    freq = configP->monitor_info.vfreq;
    mon_w = configP->monitor_info.width;
    mon_h = configP->monitor_info.height;

    i = mach32_num_reg_vals;
    mach32_regP = mach32_reg_vals;

    for (; i; i--, mach32_regP++) {
	if ((mach32_regP->width     == x) &&
	    (mach32_regP->height    == y) &&
	    (mach32_regP->freq      == freq)) {

	    break;
	}
    }

    if (i == 0) {
	fprintf(stderr, "Unsupported mode (%dx%d @%dHz)\n", x, y, freq);
	return(SI_FAIL);
    }

    i = mach32_num_weights;
    mach32_weightP = mach32_weight_reg_vals;

    /* The following loop is unnecessary for BPP == 8 */

#if (BPP == 16)
    wgt = atol(configP->info2vendorlib);
    for (; i; i--, mach32_weightP++) {
	if (mach32_regP->weighting == wgt)
	    break;
    }

#elif (BPP == 24)
    w = configP->info2vendorlib;
    for (; i; i--, mach32_weightP++) {
	if (! strcasecmp(mach32_regP->weighting, w))
	    break;
    }
#endif

    lfb.width  = x;
    lfb.height = y;

    /* 
     * lfb.stride is set by atiInitGraphics, as the proper value for
     * the stride cannot be determined until the FB size is known.  By
     * default, the stride will be 1024 when lfb.width <= 1024 and
     * equal lfb.width otherwise.  However, when there is not enough
     * memory to set the stride larger than the width, the stride is 
     * set equal to the width.
     *
     */

    return(SI_SUCCEED);
}

static SIBool init_ati()

{
    extern atiSaveCmap();
    int i, h;

    if (SET_IOPL(3) < 0)
	perror("Error setting IOPL");

    ati_fd = open("/dev/ati", O_RDWR);
    if (ati_fd < 0) {
	perror("Error opening /dev/ati");
	return(SI_FAIL);
    }

    if (ioctl(lfb.fd, KDSETMODE, KD_GRAPHICS) < 0) {
	perror("ioctl() error setting graphics mode");
	return(SI_FAIL);
    }

    atiSaveCmap(0);

    /*
     * The ATI BIOS stores the VRAM size in bits 2:3 of MISC_OPTIONS.
     * The bits are interpreted as follows:
     *
     *     0  512K
     *     1  1M
     *     2  2M
     *     3  4M
     *
     * Left shifting a 1 by the number in stored, we get the number of
     * 512K blocks of VRAM.
     *
     */

    i = (inb(MISC_OPTIONS) & 0x0c) >> 2; /* Don't care for the upper bits */
    lfb.memsize = 512 * 1024 * (1 << i);

    if (init_graphics(mach32_regP, mach32_weightP) == SI_FAIL) {
	ioctl(lfb.fd, KDSETMODE, KD_TEXT);
	fprintf(stderr, "Could not initialize device\n");
	return(SI_FAIL);
    }

    /*
     * Ask for 4M, even when there is less video memory.  The reason
     * is that there is no way for user code to detect when the
     * processor supports 4M pages.
     *
     * The kernel driver could detect the existence of 4M pages, and
     * could return the existence with an ioctl(), but that would make
     * the driver Intel Confidential, which defeats the purpose of all
     * this work.
     *
     * The non-visible portion of the actual FB will be used for
     * cursors, bitmaps, pixmaps, and fonts.
     *
     */

    lfb.fb_ptr = (PixelP)mmap(0, 0x400000, PROT_READ|PROT_WRITE,
				MAP_SHARED, ati_fd, 0);
    if (lfb.fb_ptr == (PixelP)-1) {
	perror("Cannot map ATI FB");
	return(SI_FAIL);
    }

    i = (ATI_CURSOR_SIZE * sizeof(SIint16) * ATI_NUM_CURSORS +
	 lfb.stride * sizeof(Pixel) - 1) /
	     (lfb.stride * sizeof(Pixel));

    h = lfb.memsize / (lfb.stride * sizeof(Pixel));

    if (h - i < MAX_SCREEN_COORD)
	h -= i;

    if (h > MAX_SCREEN_COORD)
	h = MAX_SCREEN_COORD;

    h -= lfb.height;

    ati.font_off = lfb.height;
    ati.font_h = h >> 1;

    h -= ati.font_h;
    ati.bitmap_off = ati.font_off + ati.font_h;
    ati.bitmap_h = h;

    ati.curs_off = ati.bitmap_off + ati.bitmap_h;

    return(SI_SUCCEED);
}

static void fillFlags(flagsP)
SIFlagsP flagsP;

{
    flagsP->SIdm_version = DM_SI_VERSION_1_1;

    flagsP->SIlinelen = lfb.width;
    flagsP->SIlinecnt = lfb.height;
    flagsP->SIxppin = (int)(lfb.width / mon_w);
    flagsP->SIyppin = (int)(lfb.height / mon_h);

    flagsP->SIavail_bitblt |= SSBITBLT_AVAIL;
    flagsP->SIavail_fpoly |= RECTANGLE_AVAIL |
			    TILE_AVAIL |
			    STIPPLE_AVAIL |
			    OPQSTIPPLE_AVAIL;
    flagsP->SIavail_line |= ONEBITLINE_AVAIL |
			   ONEBITSEG_AVAIL |
			   ONEBITRECT_AVAIL;
    flagsP->SIavail_font |= FONT_AVAIL |
			   OPQSTIPPLE_AVAIL |
			   STIPPLE_AVAIL;
    flagsP->SIavail_spans |= SPANS_AVAIL |
			    TILE_AVAIL |
			    STIPPLE_AVAIL |
			    OPQSTIPPLE_AVAIL;

    flagsP->SIcursortype = CURSOR_TRUEHDWR;
    flagsP->SIcurscnt = ATI_NUM_CURSORS;
    flagsP->SIcurswidth = ATI_CURSOR_WIDTH;
    flagsP->SIcursheight = ATI_CURSOR_HEIGHT;
    flagsP->SIcursmask = 0;

    flagsP->SItilewidth = 32;
    flagsP->SItileheight = 32;
    flagsP->SIstipplewidth = 32;
    flagsP->SIstippleheight = 32;

    flagsP->SIfontcnt = 8;

    flagsP->SIvisuals = ati_visuals;
    flagsP->SIvisualCNT = ati_num_visuals;
}

void fillFunctions(functions)
ScreenInterface *functions;

{
    functions->si_restore = atiShutdown;
    functions->si_vt_save = atiVTSave;
    functions->si_vt_restore = atiVTRestore;
    functions->si_vb_onoff = atiVideoBlank;
    functions->si_initcache = atiInitCache;
    functions->si_flushcache = atiFlushCache;
    functions->si_download_state = atiDownLoadState;

/* +++++ BAD HACK +++++ */
/*
 * There is a bug in <sidep.h>.  It declares si_screen as a macro
 * which translates to funcs->si_screen), where funcs is the name of
 * the pointer passed in as functions.  Of course, this fails miserably here.
 *
 */
#if defined(si_screen)
#undef si_screen
#endif

    functions->si_screen = atiSelectScreen;

    functions->si_set_colormap = atiSetCmap;
    functions->si_get_colormap = atiGetCmap;

    functions->si_hcurs_download = atiDownLoadCurs;
    functions->si_hcurs_turnon = atiTurnOnCurs;
    functions->si_hcurs_turnoff = atiTurnOffCurs;
    functions->si_hcurs_move = atiMoveCurs;

    functions->si_fillspans = atiFillSpans;

    functions->si_ss_bitblt = atiSSbitblt;

    functions->si_poly_clip = atiSetClipRect;
    functions->si_poly_fillrect = atiFillRects;

    functions->si_line_clip = atiSetClipRect;
    functions->si_line_onebitline = atiThinLines;
    functions->si_line_onebitseg = atiThinSegments;
    functions->si_line_onebitrect = atiThinRect;

    functions->si_drawarc_clip = atiSetClipRect;

    functions->si_fillarc_clip = atiSetClipRect;

    functions->si_font_check = atiCheckDLFont;
    functions->si_font_download = atiDownLoadFont;
    functions->si_font_free = atiFreeFont;
    functions->si_font_clip = atiSetClipRect;
    functions->si_font_stplblt = atiStplbltFont;
}

/*	MISCELLANEOUS ROUTINES 		*/
/*		MANDATORY		*/

SIBool DM_InitFunction(int fd, SIScreenRec *screenP)

{
    SIConfig *configP = screenP->cfgPtr;
    SIFlags *flagsP = screenP->flagsPtr;
    SIFunctions *functions = screenP->funcsPtr;

    if (lfbInitLFB(fd, screenP, &atiFlushGE) != SI_SUCCEED)
	return(SI_FAIL);

    if (checkConfig(configP) != SI_SUCCEED)
	return(SI_FAIL);

    if (init_ati() != SI_SUCCEED)
	return(SI_FAIL);

    fillFlags(flagsP);

    fillFunctions(functions);

    atiSetClipRect(0, 0, flagsP->SIlinelen, flagsP->SIlinecnt);
    atiClipOff();

    return(SI_SUCCEED);
}

SIBool atiShutdown()

{
    extern atiRestoreCmap();

    uninit_graphics();
    atiRestoreCmap(0);

    ioctl(lfb.fd, KDSETMODE, KD_TEXT);

    if (lfbShutdownLFB() != SI_SUCCEED)
	return(SI_FAIL);

    return(SI_SUCCEED);
}

static u_char *saveBuf = NULL;

SIBool atiVTSave()

{
    extern atiRestoreCmap();

    if (! saveBuf)
	saveBuf = (u_char *)malloc(lfb.memsize);

    memmove(saveBuf, lfb.fb_ptr, lfb.memsize);

    uninit_graphics();
    atiRestoreCmap(0);

    return(SI_SUCCEED);
}

SIBool atiVTRestore()

{
    extern atiRestoreCmap();

    if (init_graphics(mach32_regP, mach32_weightP) == SI_FAIL) {
	fprintf(stderr, "VT Restore could not re-initialize device\n");
	return(SI_FAIL);
    }

    memmove(lfb.fb_ptr, saveBuf, lfb.memsize);

    atiRestoreCmap(1);

    return(SI_SUCCEED);
}

SIBool atiVideoBlank(on)
SIBool on;

{
    int i;
    extern atiRestoreCmap();

    if (on)
	atiRestoreCmap(1);
    else {
	outb(DAC_W_INDEX, 0);
	for (i = 0; i < ATI_CMAP_SIZE * 3; i++)
	    outb(DAC_DATA, 0);
    }

    return(SI_SUCCEED);
}

SIBool atiInitCache()

{
    return(SI_FAIL);
}

SIBool atiFlushCache()

{
    return(SI_FAIL);
}

SIBool atiSelectScreen(screen, flag)
SIint32 screen;
SIint32 flag;

{
    return(SI_FAIL);
}
