#ident	"@(#)ihvkit:display/lfb256/devices/jaws/jawsInit.c	1.1"

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

#include <jaws.h>

SIint32 *jaws_regs, *jaws_curs, *jaws_cmap;

JawsRegs *jaws_regP;

static int jaws_fd;
static unsigned int saved_cntl_port, cur_cntl_port;
static float mon_w, mon_h;

static SIBool checkConfig(configP)
SIConfigP configP;

{
    int x, y, freq;
    int i;

    if (lfbstrcasecmp(configP->chipset, "Jaws")) {
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

    i = jaws_num_reg_vals;
    jaws_regP = jaws_reg_vals;

    for ( ; i; i--, jaws_regP++) {
	if ((jaws_regP->width == x) &&
	    (jaws_regP->height == y) &&
	    (jaws_regP->freq == freq)) {
	    
	    break;
	}
    }

    if (i == 0) {
	fprintf(stderr, "Unsupported mode (%dx%d @%dHz)\n", x, y, freq);
	return(SI_FAIL);
    }

    lfb.width  = jaws_regP->width;
    lfb.height = jaws_regP->height;
    lfb.stride = jaws_regP->regs.stride / sizeof(Pixel);

    return(SI_SUCCEED);
}

static SIBool init_jaws()

{
    if (SET_IOPL(3) < 0)
	perror("Error setting IOPL");

    jaws_fd = open("/dev/jaws", O_RDWR);
    if (jaws_fd < 0) {
	fprintf(stderr, "Open of /dev/jaws failed\n");
	return(SI_FAIL);
    }

    cur_cntl_port = inb(JAWS_CNTL_PORT);
    saved_cntl_port = ((cur_cntl_port & JAWS_CNTL_READABLE_BITS) ^
		       JAWS_CNTL_FLIPPED_BITS);

    cur_cntl_port &= JAWS_CNTL_MMAP_SELECT_MASK;
    cur_cntl_port |= JAWS_CNTL_JAWS_ENABLE;

    outb(JAWS_CNTL_PORT, cur_cntl_port | JAWS_CNTL_RESET);
    PAUSE;

    outb(JAWS_CNTL_PORT, cur_cntl_port);
    PAUSE;

    lfb.fb_ptr = (PixelP)mmap(0, 0x400000, PROT_READ|PROT_WRITE,
			      MAP_SHARED, jaws_fd, 0);
    if (lfb.fb_ptr == (PixelP)-1) {
	fprintf(stderr, "Jaws init: cannot map FB\n");
	return(SI_FAIL);
    }

    jaws_regs = (SIint32 *)((caddr_t)lfb.fb_ptr + JAWS_REG_BASE);
    jaws_cmap = jaws_regs + JAWS_COLOR_PALETTE;
    jaws_curs = jaws_regs + JAWS_CURSOR;

    return(SI_SUCCEED);
}

static void load_jaws_regs()

{
    register SIint32 *r = jaws_regs;
    register G332Regs *v = &jaws_regP->regs;

    /*
     * Do these first.  This is black magic.  See the G332 docs for
     * details.
     */

    *(r + JAWS_REG_BOOT_LOCATION) = v->boot_location;
    PAUSE;
    *(r + JAWS_REG_CONTROL_A) = JAWS_INIT_CONTROL_A;
    PAUSE;

#if 0

    /* 
     * According to the G332 documentation, Control B should be
     * initialized to 0 after Control A is initialized.  However, an
     * app note that was attached indicated that Control B should be
     * left alone as current chip implementations map it on to Control A.
     * 
     */

    *(r + JAWS_REG_CONTROL_B) = 0;
    PAUSE;
#endif

    *(r + JAWS_REG_HALF_SYNC) = v->half_sync;
    PAUSE;
    *(r + JAWS_REG_BACK_PORCH) = v->back_porch;
    PAUSE;
    *(r + JAWS_REG_DISPLAY) = v->display;
    PAUSE;
    *(r + JAWS_REG_SHORT_DISPLAY) = v->short_display;
    PAUSE;
    *(r + JAWS_REG_BROAD_PULSE) = v->broad_pulse;
    PAUSE;
    *(r + JAWS_REG_V_SYNC) = v->v_sync;
    PAUSE;
    *(r + JAWS_REG_V_PRE_EQUALISE) = v->v_pre_equalise;
    PAUSE;
    *(r + JAWS_REG_V_POST_EQUALISE) = v->v_post_equalise;
    PAUSE;
    *(r + JAWS_REG_V_BLANK) = v->v_blank;
    PAUSE;
    *(r + JAWS_REG_V_DISPLAY) = v->v_display;
    PAUSE;
    *(r + JAWS_REG_LINE_TIME) = v->line_time;
    PAUSE;

    *(r + JAWS_REG_LINE_START) = v->line_start;
    PAUSE;
    *(r + JAWS_REG_MEM_INIT) = v->mem_init;
    PAUSE;
    *(r + JAWS_REG_TRANSFER_DELAY) = v->transfer_delay;
    PAUSE;

    *(r + JAWS_REG_PIXEL_MASK) = 0xffffff;
    PAUSE;

    /*
     * This must be done last.  When Control A is set, the VTG is on,
     * and the timing registers will be unmodifyable.
     */
    *(r + JAWS_REG_CONTROL_A) = v->control_A;
    PAUSE;
}

static void fillFlags(flagsP)
SIFlagsP flagsP;

{
    flagsP->SIdm_version = DM_SI_VERSION_1_1;

    flagsP->SIlinelen = lfb.width;
    flagsP->SIlinecnt = lfb.height;
    flagsP->SIxppin = (int)(lfb.width / mon_w);
    flagsP->SIyppin = (int)(lfb.height / mon_h);

    flagsP->SIcursortype = CURSOR_TRUEHDWR;
    flagsP->SIcurscnt = JAWS_NUM_CURSORS;
    flagsP->SIcurswidth = JAWS_CURSOR_WIDTH;
    flagsP->SIcursheight = JAWS_CURSOR_HEIGHT;
    flagsP->SIcursmask = 0;

    flagsP->SIvisuals = jaws_visuals;
    flagsP->SIvisualCNT = jaws_num_visuals;
}

void fillFunctions(functions)
ScreenInterface *functions;

{
    functions->si_restore = jawsShutdown;
    functions->si_vt_save = jawsVTSave;
    functions->si_vt_restore = jawsVTRestore;
    functions->si_vb_onoff = jawsVideoBlank;
    functions->si_initcache = jawsInitCache;
    functions->si_flushcache = jawsFlushCache;

/* +++++ BAD HACK +++++ */
/* 
 * There is a bug in <sidep.h>.  It declares si_screen as a macro 
 * which translates to (*funcs->si_screen), where funcs is the name of 
 * the pointer passed in as functions.  Of course, this fails miserably here.
 * 
 */
#if defined(si_screen)
#undef si_screen
#endif

    functions->si_screen = jawsSelectScreen;

    functions->si_set_colormap = jawsSetCmap;
    functions->si_get_colormap = jawsGetCmap;

    functions->si_hcurs_download = jawsDownLoadCurs;
    functions->si_hcurs_turnon = jawsTurnOnCurs;
    functions->si_hcurs_turnoff = jawsTurnOffCurs;
    functions->si_hcurs_move = jawsMoveCurs;
}

/*	MISCELLANEOUS ROUTINES 		*/
/*		MANDATORY		*/

SIBool DM_InitFunction(int fd, SIScreenRec *screenP)

{
    SIConfig *configP = screenP->cfgPtr;
    SIFlags *flagsP = screenP->flagsPtr;
    SIFunctions *functions = screenP->funcsPtr;

    if (lfbInitLFB(fd, screenP, (void (*)())NULL) != SI_SUCCEED)
	return(SI_FAIL);

    if (checkConfig(configP) != SI_SUCCEED)
	return(SI_FAIL);

    if (init_jaws() != SI_SUCCEED)
	return(SI_FAIL);

    load_jaws_regs();

    fillFlags(flagsP);

    fillFunctions(functions);

    return(SI_SUCCEED);
}

SIBool jawsShutdown()

{
    outb(JAWS_CNTL_PORT, saved_cntl_port | JAWS_CNTL_RESET);
    PAUSE;

    outb(JAWS_CNTL_PORT, saved_cntl_port & JAWS_CNTL_JAWS_ENABLE);

    return(SI_SUCCEED);
}

SIBool jawsVTSave()

{
    cur_cntl_port &= ~JAWS_CNTL_JAWS_ENABLE & 0xff;
    outb(JAWS_CNTL_PORT, cur_cntl_port);

    return(SI_SUCCEED);
}

SIBool jawsVTRestore()

{
    cur_cntl_port |= JAWS_CNTL_JAWS_ENABLE;
    outb(JAWS_CNTL_PORT, cur_cntl_port);

    return(SI_SUCCEED);
}

SIBool jawsVideoBlank(on)
SIBool on;

{
    SIint32 i = jaws_regP->regs.control_A;

    if (! on)
	i |= 0x400;

    *(jaws_regs + JAWS_REG_CONTROL_A) = i;

    return(SI_SUCCEED);
}

SIBool jawsInitCache()

{
    return(SI_FAIL);
}

SIBool jawsFlushCache()

{
    return(SI_FAIL);
}

SIBool jawsSelectScreen(screen, flag)
SIint32 screen;
SIint32 flag;

{
    return(SI_FAIL);
}
