#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiMisc.c	1.1"

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


#include <ati.h>

/*
 * The following code is a C adaptation of the ASM code presented in ATI
 * App Note AN0005 - 11/12/92, "Mach32 mode setting without the BIOS."  The
 * primary differences are to make it compatible with the higher level code
 * below and to use the same names as those found in the "Programmer's
 * Guide to the Mach32 Registers".
 *
 */

#define DACtype (inb(CONFIG_STATUS_1+1) & 0x0e)

/*
 * Many of these DACs have multiple part numbers.  The names used here are
 * those found in the APP note.  Known alternates are:
 *
 *     ATT20C491     SIERRA SC11483/11486/11488
 *     BT481         ATT ???
 *     TI            TI/ATI 34075, ATI 68875
 *
 */

#define ATI68830_DAC    0
#define ATT20C491_DAC   2
#define TI_DAC          4
#define BROOKTREE_DAC   6
#define BT481_DAC       8
#define ATI68860_DAC   10


void passth_8514()

{
    register u_int t;

    t = inw(CLOCK_SEL);
    outw(CLOCK_SEL, t | 0x01);
}

void passth_vga()

{
    register u_int t;

    t = inw(CLOCK_SEL);
    outw(CLOCK_SEL, t & 0xfffe);
}

void set_blank_adj(u_int delay)

{
    register u_char t;

    t = inb(R_MISC_CNTL+1);
    outb(MISC_CNTL+1, (t & 0xf0) | delay);
}

#if (BPP == 8)
void init_dac(int mode)

{
    register u_int t;

    t = 0x0c;
    if (DACtype == ATI68830_DAC)
	t = 0x04;
    set_blank_adj(t);

    switch (DACtype) {
      case ATT20C491_DAC:
      case BT481_DAC:
	outw(EXT_GE_CONFIG, 0x101a);
	outb(ATT_MODE_CNTL, 0);
	break;

      case TI_DAC:
	outw(EXT_GE_CONFIG, 0x201a);
	outb(INPUT_CLK_SEL, 0);
	outb(OUTPUT_CLK_SEL, 0x30);
	outb(MUX_CNTL, 0x2d);
	break;
    }

    outw(EXT_GE_CONFIG, mode);
    outb(DAC_MASK, 0xff);
}

void init_dac_mux()

{
    register u_int t;

    if (DACtype == TI_DAC) {
	t = inw(CLOCK_SEL);
	outw(CLOCK_SEL, (t & 0xff00) | 0x11);
	outw(EXT_GE_CONFIG, 0x201a);
	outb(OUTPUT_CLK_SEL, 9);
	outb(MUX_CNTL, 0x1d);
	outb(INPUT_CLK_SEL, 1);
	outw(EXT_GE_CONFIG, 0x011a);
	set_blank_adj(1);
	outw(CLOCK_SEL, t);
    }
}
#elif (BPP == 16)
init_dac(int mode)

{
    register u_int t;
    t = 0x0c;
    if (DACtype == ATI68830_DAC)
	t = 0x04;
    set_blank_adj(t);

    switch (DACtype) {
      case ATT20C491_DAC:
      case BT481_DAC:
	outb(DAC_MASK, 0);
	outw(EXT_GE_CONFIG, 0x101a);

	if (mode & 0xc0) {
	    if (DACtype == BT481_DAC)
		t =  0xe8;
	    else
		t = 0xc2;
	}
	else {
	    if (DACtype == BT481_DAC)
		t = 0xa8;
	    else
		t = 0xa2;
	}
	outb(ATT_MODE_CNTL, t);

	t = inw(CLOCK_SEL)
	if (t & 0xc0) {
	    t &= 0xff3f;
	}
	else {
	    if ((t & 0x3c) == 0x30) {
		t &= 0xffc3;
		t |= 0x2c;
	    }
	}
	outw(CLOCK_SEL, t);

	/* Fall through to the default case */

      default:
	outw(EXT_GE_CONFIG, mode);
	break;

      case TI_DAC:
	outb(DAC_MASK, 0);
	set_blank_adj(1);
	outw(EXT_GE_CONFIG, 0x201a);
	outb(INPUT_CLK_SEL, 1);

	t = inw(CLOCK_SEL)
	if (t & 0xc0) {
	    outw(CLOCK_SEL, t & 0xff3f);

	    if (inb(R_H_TOTAL_AND_DISP) == 0x4f)
		set_blank_adj(2);

	    t = 0x8;
	}
	else {
	    t = 0x0;
	}
	outb(OUTPUT_CLK_SEL, t);

	outb(MUX_CNTL, 0x0d);
	outw(EXT_GE_CONFIG, mode | 0x4000);

	break;
    }

}
#endif

void uninit_dac()

{
    register u_int t;

    passth_8514();

    switch(DACtype) {

      case TI_DAC:
	t = inw(LOCAL_CONTROL);
	outw(LOCAL_CONTROL, t | 0x08);
	/* Fall through to the ATT/ATT20C491 case */

      case BT481_DAC:
      case ATT20C491_DAC:
	init_dac(0x001a);
	outw(EXT_GE_CONFIG, 0x201a);
	outb(OUTPUT_CLK_SEL, 0);
	break;
    }

    outw(EXT_GE_CONFIG, 0x1a);

    passth_vga();
}

/*
 * The following C code is a modified version of the C code presented
 * in ATI App Note AN0005 - 11/12/92, "Mach32 mode setting without the
 * BIOS."
 *
 */

void unlock_shadowset(set)
u_int set;

{
    if (set) {
	outb(SHADOW_SET, set);	/* Don't need the upper bits */
	outb(SHADOW_CTL, 0);
    }
}

void lock_shadowset(set)
u_int set;

{
    if (set) {
	outb(SHADOW_SET, set);	/* Don't need the upper bits */
	outb(SHADOW_CTL, 0x7f);
    }

    outb(SHADOW_SET, 0);	/* Don't need the upper bits */
}

void load_shadowset(set, valueP)
u_int set;
Mach32DispRegs *valueP;

{
    if (set > 2)
	return;

    unlock_shadowset(set);

    outb(DISP_CNTL, 0x53);

    outb(H_TOTAL, valueP->h_total);
    outb(H_DISP, valueP->h_disp);
    outb(H_SYNC_STRT, valueP->h_sync_strt);
    outb(H_SYNC_WID, valueP->h_sync_wid);

    outw(V_TOTAL, valueP->v_total);
    outw(V_DISP, valueP->v_disp);
    outw(V_SYNC_STRT, valueP->v_sync_strt);
    outb(V_SYNC_WID, valueP->v_sync_wid);

    outb(DISP_CNTL, valueP->disp_cntl);
    outw(CLOCK_SEL, valueP->clock_sel);

    outw(HORZ_OVERSCAN, 0);
    outw(VERT_OVERSCAN, 0);

    outb(OVERSCAN_COLOR_8, 0);
    outb(OVERSCAN_BLUE_24,  0);
    outb(OVERSCAN_GREEN_24, 0);
    outb(OVERSCAN_RED_24,   0);

    lock_shadowset(set);
}

void set_mode(set)
u_int set;

{
    register int t;

    switch(set) {
      case 1:
	t = 3;
	break;

      case 2:
	t = 7;
	break;

      case 0:
      default:
	t = 2;
	break;
    }

    outb(ADVFUNC_CNTL, t);
}

void set_mem_offset()

{
    int c;

    outb(MEM_BNDRY, 0);
    c = inb(MEM_CFG);
    outb(MEM_CFG, (c & 0xf0) | 0x02);

    outb(GE_OFFSET_HI, 0);
    outb(CRT_OFFSET_HI, 0);
    outw(GE_OFFSET_LO, 0);
    outw(CRT_OFFSET_LO, 0);
}

SIBool check_mode(valueP, weightP, pitch)
Mach32DispRegs *valueP;
Mach32WeightRegs *weightP;
int *pitch;

{
    int i;

    i = valueP->width * valueP->height * sizeof(Pixel) +
	ATI_CURSOR_SIZE * sizeof(SIint16) * ATI_NUM_CURSORS;

    if (i > lfb.memsize) {
	fprintf(stderr,
		"Not enough memory (%d) to support %dx%d\n",
		lfb.memsize, valueP->width, valueP->height);
	return(SI_FAIL);
    }

#if (BPP == 8)
    if ((valueP->width == 1280) &&
	(valueP->interlace == 0) &&
	(DACtype != TI_DAC)) {

	fprintf(stderr, "1280x1024 non-interlace not supported by DAC\n");
	return(SI_FAIL);
    }

#elif (BPP == 16)
    if ((weightP->weighting > 600) &&
	(DACtype != TI_DAC) &&
	(DACtype != ATI68830_DAC)) {

	fprintf(stderr, "664 and 655 weightings not supported by DAC\n");
	return(SI_FAIL);
    }

#elif (BPP == 24)
    if (DACtype == ATI68830_DAC) {
	fprintf(stderr, "DAC does not support 24 Bpp modes\n");
	return(SI_FAIL);
    }

    if ((strcasecmp(weightP->weighting, "rgba") ||
	 strcasecmp(weightP->weighting, "abgr")) &&
	(DACtype != TI_DAC)) {

	fprintf(stderr, "RGBa and aBGR weightings not supported by DAC\n");
	return(SI_FAIL);
    }

    if ((valueP->width > 640) &&
	(DACtype != TI_DAC)) {

	ErrofF("DAC only supports 640x480 BGR\n");
	return (SI_FAIL);
    }

    if (strcasecmp(weightP->weighting, "rgb") &&
	(DACtype == BROOKTREE_DAC)) {

	fprintf(stderr, "DAC only supports BGR weighting\n");
	return(SI_FAIL);
    }
#endif

    return(SI_SUCCEED);
}

SIBool init_graphics(valueP, weightP)
Mach32DispRegs *valueP;
Mach32WeightRegs *weightP;

{
    int pitch, i;

    if (check_mode(valueP, weightP, &pitch) == SI_FAIL)
	return(SI_FAIL);

    load_shadowset(1, valueP);
    set_mode(1);

    /* Reset the Engine & FIFO */
    outw(SUBSYS_CNTL, 0x800f);
    outw(SUBSYS_CNTL, 0x4000);

    init_dac(weightP->def_ext_ge_config);

#if (BPP == 8)
    if ((valueP->width == 1280) &&
	(valueP->interlace == 0))
	init_dac_mux();
#endif

    set_mem_offset();

    i = lfb.memsize / (valueP->width * sizeof(Pixel));
    if ((i > MAX_SCREEN_COORD) && (valueP->width < 1024))
	pitch = 1024;
    else
	pitch = valueP->width;

    lfb.stride = pitch;
    pitch >>= 3;
    outb(GE_PITCH, pitch);
    outb(CRT_PITCH, pitch);

    return(SI_SUCCEED);
}

void uninit_graphics()

{
    Mach32DispRegs *r;
    int i;

    for (i = 0, r = mach32_reg_vals; i < mach32_num_reg_vals; i++, r++) {
	if ((r->width == 640) &&
	    (r->height == 480) &&
	    (r->interlace == 1))
	    load_shadowset(1, r);

	if ((r->width == 1024) &&
	    (r->height == 768) &&
	    (r->interlace == 1))
	    load_shadowset(2, r);
    }

    uninit_dac();
    set_mode(0);
}
