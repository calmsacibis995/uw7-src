#ident	"@(#)ihvkit:display/lfb256/FB/ati/ati.c	1.1"

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

#ifndef _KERNEL
#define _KERNEL
#endif

#include <sys/types.h>
#include <stddef.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/vmparam.h>
#include <sys/immu.h>
#include <sys/kmem.h>
#include <sys/ioctl.h>
#include <sys/cmn_err.h>
#include <sys/ddi.h>
#include <sys/moddefs.h>
#include <sys/cred.h>

/* Detect ESMP vs standard SVR4.2.  SI86IOPL is a new sysi86 function
 * in ESMP.  This is not a clean way to detect ESMP, but the 
 * alternative was to have the makefile run uname and detect ESMP.
 *
 * This will be used during the start routine to grant permission to 
 * the calling process to access the ATI ports.
 *
 */

#include <sys/sysi86.h>

#ifdef SI86IOPL
#define ESMP
#endif

#ifdef ESMP
#include <sys/iobitmap.h>
#endif

#include "atinames.h"

/* For detecting EISA cards */
#define EISA_ID_PORT 0x0c80
#define ATI_EISA_ID_0 0x00448906
#define ATI_EISA_ID_1 0x01448906

/* 
 * The FB physical address.  When 0, it is not valid.  (This would 
 * imply that the FB was mapped in over the interrupt vectors and base 
 * 640K RAM.
 *
 */

paddr_t ati_fb_location = 0;

int atidevflag = 0;

char ati_id_string[] = "ATI v1.0";
char ati_copyright[] = "Copyright (c) 1993 Intel Corp., All Rights Reserved";

/* 
 * ATI Mach32 and DAC ports that the user needs access to.  For 
 * simplicity in reading and writing this code, ports that are aliases 
 * of other ports (e.g. DEST_X/SRC_X/DIASTP) are all enabled.  The 
 * enabling only happens at device open, and the extra few lookups in 
 * the TSS IO protection bitmap should be noticeable.
 *
 */

unsigned short ati_ports[] = {
    ADVFUNC_CNTL, ADVFUNC_CNTL+1,
    ALU_BG_FN, ALU_BG_FN+1,
    ALU_FG_FN, ALU_FG_FN+1,
    APERTURE_CNTL, APERTURE_CNTL+1,
    ATT_MODE_CNTL,
    AXSTP, AXSTP+1,
    BKGD_COLOR, BKGD_COLOR+1,
    BKGD_MIX, BKGD_MIX+1,
    BOUNDS_BOTTOM, BOUNDS_BOTTOM+1,
    BOUNDS_LEFT, BOUNDS_LEFT+1,
    BOUNDS_RIGHT, BOUNDS_RIGHT+1,
    BOUNDS_TOP, BOUNDS_TOP+1,
    BRES_COUNT, BRES_COUNT+1,
    CHIP_ID, CHIP_ID+1,
    CLOCK_SEL, CLOCK_SEL+1,
    CMD, CMD+1,
    CMP_COLOR, CMP_COLOR+1,
    CONFIG_STATUS_1, CONFIG_STATUS_1+1,
    CONFIG_STATUS_2, CONFIG_STATUS_2+1,
    CRT_OFFSET_HI, CRT_OFFSET_HI+1,
    CRT_OFFSET_LO, CRT_OFFSET_LO+1,
    CRT_PITCH, CRT_PITCH+1,
    CURSOR_COLOR_0,
    CURSOR_COLOR_1,
    CURSOR_OFFSET_HI, CURSOR_OFFSET_HI+1,
    CURSOR_OFFSET_LO, CURSOR_OFFSET_LO+1,
    CUR_X, CUR_X+1,
    CUR_Y, CUR_Y+1,
    DAC_DATA,
    DAC_MASK,
    DAC_R_INDEX,
    DAC_W_INDEX,
    DEST_CMP_FN, DEST_CMP_FN+1,
    DEST_COLOR_CMP_MASK, DEST_COLOR_CMP_MASK+1,
    DEST_X, DEST_X+1,
    DEST_X_END, DEST_X_END+1,
    DEST_X_START, DEST_X_START+1,
    DEST_Y, DEST_Y+1,
    DEST_Y_END, DEST_Y_END+1,
    DIASTP, DIASTP+1,
    DISP_CNTL,
    DISP_STATUS,
    DP_CONFIG, DP_CONFIG+1,
    ERR_TERM, ERR_TERM+1,
    EXT_CURSOR_COLOR_0, EXT_CURSOR_COLOR_0+1,
    EXT_CURSOR_COLOR_1, EXT_CURSOR_COLOR_1+1,
    EXT_FIFO_STATUS, EXT_FIFO_STATUS+1,
    EXT_GE_CONFIG, EXT_GE_CONFIG+1,
    EXT_GE_STATUS, EXT_GE_STATUS+1,
    EXT_SCISSOR_B, EXT_SCISSOR_B+1,
    EXT_SCISSOR_L, EXT_SCISSOR_L+1,
    EXT_SCISSOR_R, EXT_SCISSOR_R+1,
    EXT_SCISSOR_T, EXT_SCISSOR_T+1,
    EXT_SHORT_STROKE, EXT_SHORT_STROKE+1,
    FIFO_OPT, FIFO_OPT+1,
    FIFO_TEST_DATA, FIFO_TEST_DATA+1,
    FIFO_TEST_TAG,
    FRGD_COLOR, FRGD_COLOR+1,
    FRGD_MIX, FRGD_MIX+1,
    GE_OFFSET_HI, GE_OFFSET_HI+1,
    GE_OFFSET_LO, GE_OFFSET_LO+1,
    GE_PITCH, GE_PITCH+1,
    GE_STAT, GE_STAT+1,
    HORZ_CURSOR_OFFSET,
    HORZ_CURSOR_POSN, HORZ_CURSOR_POSN+1,
    HORZ_OVERSCAN, HORZ_OVERSCAN+1,
    H_DISP,
    H_SYNC_STRT,
    H_SYNC_WID,
    H_TOTAL,
    INPUT_CLK_SEL,
    LINEDRAW, LINEDRAW+1,
    LINEDRAW_INDEX, LINEDRAW_INDEX+1,
    LINEDRAW_OPT, LINEDRAW_OPT+1,
    LOCAL_CONTROL, LOCAL_CONTROL+1,
    MAJ_AXIS_PCNT, MAJ_AXIS_PCNT+1,
    MAX_WAITSTATES, MAX_WAITSTATES+1,
    MEM_BNDRY, MEM_BNDRY+1,
    MEM_CFG, MEM_CFG+1,
    MEM_CNTL, MEM_CNTL+1,
    MIN_AXIS_PCNT, MIN_AXIS_PCNT+1,
    MISC_CNTL, MISC_CNTL+1,
    MISC_OPTIONS, MISC_OPTIONS+1,
    MUX_CNTL,
    OUTPUT_CLK_SEL,
    OVERSCAN_BLUE_24,
    OVERSCAN_COLOR_8,
    OVERSCAN_GREEN_24,
    OVERSCAN_RED_24,
    PATTERN_H, PATTERN_H+1,
    PATTERN_L, PATTERN_L+1,
    PATT_DATA, PATT_DATA+1,
    PATT_DATA_INDEX, PATT_DATA_INDEX+1,
    PATT_INDEX, PATT_INDEX+1,
    PATT_LENGTH, PATT_LENGTH+1,
    PCI_CNTL, PCI_CNTL+1,
    PIXEL_CNTL, PIXEL_CNTL+1,
    PIX_TRANS, PIX_TRANS+1,
    RD_MASK, RD_MASK+1,
    ROM_ADDR_1, ROM_ADDR_1+1,
    R_EXT_GE_CONFIG, R_EXT_GE_CONFIG+1,
    R_H_SYNC_STRT,
    R_H_SYNC_WID,
    R_H_TOTAL_AND_DISP, R_H_TOTAL_AND_DISP+1,
    R_MISC_CNTL, R_MISC_CNTL+1,
    R_SRC_X, R_SRC_X+1,
    R_SRC_Y, R_SRC_Y+1,
    R_V_DISP, R_V_DISP+1,
    R_V_SYNC_STRT, R_V_SYNC_STRT+1,
    R_V_SYNC_WID, R_V_SYNC_WID+1,
    R_V_TOTAL, R_V_TOTAL+1,
    SCAN_X, SCAN_X+1,
    SCISSOR_B, SCISSOR_B+1,
    SCISSOR_L, SCISSOR_L+1,
    SCISSOR_R, SCISSOR_R+1,
    SCISSOR_T, SCISSOR_T+1,
    SCRATCH_PAD_0, SCRATCH_PAD_0+1,
    SCRATCH_PAD_1, SCRATCH_PAD_1+1,
    SHADOW_CTL, SHADOW_CTL+1,
    SHADOW_SET, SHADOW_SET+1,
    SHORT_STROKE, SHORT_STROKE+1,
    SRC_X, SRC_X+1,
    SRC_X_END, SRC_X_END+1,
    SRC_X_START, SRC_X_START+1,
    SRC_Y, SRC_Y+1,
    SRC_Y_DIR, SRC_Y_DIR+1,
    SUBSYS_CNTL, SUBSYS_CNTL+1,
    SUBSYS_STATUS, SUBSYS_STATUS+1,
    VERT_CURSOR_OFFSET,
    VERT_CURSOR_POSN, VERT_CURSOR_POSN+1,
    VERT_LINE_CNTR, VERT_LINE_CNTR+1,
    VERT_OVERSCAN, VERT_OVERSCAN+1,
    V_DISP, V_DISP+1,
    V_SYNC_STRT, V_SYNC_STRT+1,
    V_SYNC_WID, V_SYNC_WID+1,
    V_TOTAL, V_TOTAL+1,
    WRT_MASK, WRT_MASK+1,
    0x0,
};

/*
 * Make the ATI Mach32 driver demand loadable.  This allows the X
 * support to be an add-on package without requiring a kernel rebuild.
 *
 */

static int ati_load();
void atistart();

MOD_DRV_WRAPPER(ati, ati_load, NULL, NULL, "ATI Mach32 driver");

static int ati_load()

{
    atistart();
    if (!ati_fb_location)
	return(ENODEV);

    return(0);
}

void atistart()

{
    int t, t1, t2;
    addr_t rombase;

    cmn_err (CE_CONT, "%s %s\n", ati_id_string, ati_copyright);

    /* 
     * Calculate the base address of the ROM.  Use the formula given 
     * in Appendix B of the Programmer's guide, and multiply by 16 to 
     * convert a segment into a physical address.
     *
     */

    t = inb(ROM_ADDR_1);
    t &= 0x7f;
    t <<= (7 + 4);		/* mul 80h + segment shift */
    t += 0xc0000;
    rombase = physmap((paddr_t)t, 0x100, KM_SLEEP);

    if (! rombase) {
	cmn_err(CE_WARN, "ATI board BIOS not present.");
	return;
    }
	
    /* 
     * Search for an ATI card.  See App Note AN0003 - 11/09/92,
     * "Detecting the Mach32 video adapter" for details.
     *
     */

    /* Search for the ATI signature */
    if (strcmp(rombase+0x31, "761295520")) {
	cmn_err(CE_WARN, "ATI board not present.");

	/* Release the mapped BIOS address */
	physmap_free(rombase, 0x100, KM_SLEEP);

	return;
    }

    /* Detect an ATI Mach32 */
    if (inw(EXT_FIFO_STATUS) == 0xffff) {
	cmn_err(CE_WARN, "ATI board is not Mach32 based.");
	return;
    }

    outw(SUBSYS_CNTL, 0x8000);
    outw(SUBSYS_CNTL, 0x400f);
    outw(SRC_X, 0x0555);
    while(inw(EXT_FIFO_STATUS))
	;
    if (inw(R_SRC_X) != 0x0555) {
	cmn_err(CE_WARN, "ATI board is not Mach32 based.");
	return;
    }

    outw(SRC_X, 0x02aa);
    while(inw(EXT_FIFO_STATUS))
	;
    if (inw(R_SRC_X) != 0x02aa) {
	cmn_err(CE_WARN, "ATI board is not Mach32 based.");
	return;
    }

    cmn_err(CE_CONT, "ATI board detected", t);

    /* 
     * Search for the ATI chipset in the EISA slots.
     * 
     */
    
    for (t = 1; t < 0x10; t++) {
	t1 = inl((t << 12) + EISA_ID_PORT);
	if ((t1 == ATI_EISA_ID_0) ||
	    (t1 == ATI_EISA_ID_1)) {

	    cmn_err(CE_CONT, " in EISA slot %d", t);
	    break;
	}
    }

    /* 
     * Calculate the FB address.  The algorithm is as follows:
     *
     * Look at 
     * if (ROM_BASE:62[0] == 0)
     *     if ((CHIP_ID > 0) &&
     *         ((BUS_TYPE == PCI) ||
     *          (4M aperture enabled)))
     *         address is in MEM_CNFG[15:4].
     *     else
     *         address is in MEM_CNFG[14:8].
     * else
     *     addr[6:0] in MEM_CNFG[14:8]
     *     addr[11:7] in SCRATCH_PAD_0+1[4:0].
     *
     * See App Note AN0010, "Using the Mach32 Aperature Interface,"
     * for more details.
     *
     *
     * Notes:
     *
     * ROM_BASE:62[0] is used to indicate that an extended address is 
     * being used.  This was necessary for Rev3 chips.
     *
     * The bus type is in CONFIG_STATUS_1[3:1].  The 4M aperture is
     * always enabled for PCI.
     *
     * For Rev6, CONFIG_STATUS_2[13] specifies whether the aperature
     * is 4G or 128M.
     *
     */
    
    t = inw(MEM_CFG);
    if ((*(rombase+0x62) & 1) == 0) {
	if (inb(CHIP_ID) &&
	    (((inb(CONFIG_STATUS_1) & 0x0e) == 0x0e) ||
	     (inw(CONFIG_STATUS_2) & 0x2000)))
	    ati_fb_location = (t >> 4) & 0x0fff;
	else
	    ati_fb_location = (t >> 8) & 0x007f;
    }
    else {
	t1 = (inb(SCRATCH_PAD_0+1) & 0x1f) << 7;
	ati_fb_location = ((t >> 8) & 0x7f) | t1;
    }

    cmn_err(CE_CONT, " at %dM\n", ati_fb_location);

    /* 
     * Force the FB address to a 4M boundary.  This allows the
     * Pentium(tm) CPU to use one 4M page for the FB when the kernel
     * supports 4M pages.  No change is necessary in the driver,
     * mmap() will automatically use a 4M page if the request is for 
     * 4M and the physical address is 4M aligned.
     *
     */
    
    if (ati_fb_location & 3) {
	t2 = ati_fb_location;

	if ((*(rombase+0x62) & 1) == 0) {
	    if (inb(CHIP_ID) &&
		(((inb(CONFIG_STATUS_1) & 0x0e) == 0x0e) ||
		 (inw(CONFIG_STATUS_2) & 0x2000))) {
		if (ati_fb_location <= 0xffc)
		    t += 0x40;
		t &= 0xffcf;
		ati_fb_location = (t >> 4) & 0x0fff;
		outw(MEM_CFG, t);
	    }
	    else {
		if (ati_fb_location <= 0x7c)
		    t += 0x400;
		t &= 0xfcff;
		ati_fb_location = (t >> 8) & 0x007f;
		outw(MEM_CFG, t);
	    }
	}
	else {

	    /* 
	     * We can only adjust MEM_CFG, so only take those bits
	     * into account.
	     *
	     */

	    if ((ati_fb_location & 0x7f) <= 0x7c)
		t += 0x400;
	    t &= 0xfcff;
	    ati_fb_location = ((t >> 8) & 0x7f) | t1;
	    outw(MEM_CFG, t);
	}

	cmn_err(CE_NOTE,
		"Adjusting ATI FB address to 4M boundary (%dM -> %dM).",
		t2, ati_fb_location);
    }

    ati_fb_location <<= 20;

    /* Release the mapped BIOS address */
    physmap_free(rombase, 0x100, KM_SLEEP);

    return;
}

/* 
 * This routine is provided for debugging purposes.  If the X server
 * dumps, the screen is not restored to text mode and there is no easy 
 * way for the user to do this manually.  This routine provides the 
 * user with a method of doig it from KDB, if that is an option.  
 * Since the X server usually only crashes for debuggers, this should 
 * not be a problem.
 *
 */

void atireset()

{
    int t;

    outw(SUBSYS_CNTL, 0x8000);
    outw(SUBSYS_CNTL, 0x400f);
    t = inw(CLOCK_SEL);
    outw(CLOCK_SEL, t & 0xfffe);
    outb(ADVFUNC_CNTL, 2);
    outw(EXT_GE_CONFIG, 0x201a);
}

/* ARGSUSED */
int atiopen(devp, flag, type, cr)
dev_t *devp;
int flag;
int type;
struct cred *cr;

{
    /* 
     * The only thing that would prevent us from using the ATI Mach32 
     * is if the card were not detected.  If the FB address is 0, the 
     * card was not detected.
     *
     */

    if (! ati_fb_location)
	return(ENXIO);

    /* Enable the IO ports for the user */
#ifdef ESMP
    iobitmapctl(IOB_ENABLE, ati_ports);
#else
    enableio(ati_ports);
#endif

    return(0);
}

/* ARGSUSED */
int aticlose(dev, flag, cr)
dev_t dev;
int flag;
struct cred *cr;

{
    return(0);
}

/* 
 * IOCTL is provided as an expansion option
 *
 */

/* ARGSUSED */
int atiioctl(dev, cmd, arg, flag, cr, rvalp)
dev_t dev;
int cmd;
int arg;
int flag;
struct cred *cr;
int *rvalp;

{
    switch(cmd) {
      default:
	break;
    }
    
    return(0);
}

/* 
 * The kernel entry point for mmap() will provide a 4M page on three
 * conditions:
 *
 *     - The kernel is 4M page aware.  The code for this in Intel 
 *       Proprietary, so this is not a given.
 *     - The request is 4M aligned
 *     - The request is for a multiple of 4M
 *
 * This means that the if the user wants a 4M page, and all the
 * benefits associated with it, they will need to ask for 4M, and this 
 * routine will need to return a 4M aliigned address for offset 0.  
 * This routine is still called for every 4K page, though.
 *
 * The upshot is that even though there isn't always 4M on FB, allow 
 * the user to map in the 4M range.  The init code will attempt to 
 * force the address to a 4M aligned address.  If the user walks off 
 * the end of the world, the sea monsters will get them.
 *
 */

/*ARGSUSED*/
atimmap(dev, off, prot)
dev_t dev;
register off_t off;

{
    if ((off < 0) ||
	(off >= 0x400000))
	return(NOPAGE);

    return(hat_getppfnum(ati_fb_location + off, PSPACE_MAINSTORE));
}
