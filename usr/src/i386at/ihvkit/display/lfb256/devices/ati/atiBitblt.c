#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiBitblt.c	1.1"

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

/*	HARDWARE BITBLT ROUTINES	*/
/*		OPTIONAL		*/

SIBool atiSSbitblt(sx, sy, dx, dy, wid, hgt)
SIint32 sx, sy, dx, dy, wid, hgt;
{
    SIGStateP gstateP = lfb_cur_GStateP;

#ifdef ATI_SLOW_EXTENSIONS
    register int cmd = 0xc053;
#endif

    /*
     * If the drawing mode is noop, or the GState plane mask is 0, no
     * drawing will take place.
     */

    if ((gstateP->SGmode == GXnoop) ||
	(gstateP->SGpmask == 0) ||
	(wid == 0) ||
	(hgt == 0)) {
	return (SI_SUCCEED);
    }

    if (atiClipping)
	atiClipOff();

    ATI_NEED_FIFO(3);
    outw(WRT_MASK, gstateP->SGpmask);
#ifdef ATI_SLOW_EXTENSIONS
    outw(FRGD_MIX, ati_mode_trans[gstateP->SGmode] | 0x60);
    outw(PIXEL_CNTL, 0xa000);
#else
    outw(DP_CONFIG, 0x7211);
    outw(ALU_FG_FN, ati_mode_trans[gstateP->SGmode]);
#endif

    /*
     * Check for overlap.  There are two cases we need to deal with:
     *
     * 1. The upper left corner of the dst is inside the src area, but is
     *    not on the first scan line.  In this case, copy from bottom to
     *    top.
     *
     * 2. The upper left corner of the dst is on the first scan line of the
     *    src.  In this case, we need to copy right to left along the scan
     *    lines.
     *
     * In all other cases, we will copy top to bottom, left to right.
     */

#ifdef ATI_SLOW_EXTENSIONS
    ATI_NEED_FIFO(2);
    if ((dy == sy) &&
	(dx > sx)) {
	/* Right to Left */
	outw(CUR_X, sx + wid - 1);
	outw(DEST_X, dx + wid - 1);
    }
    else {
	outw(CUR_X, sx);
	outw(DEST_X, dx);

	cmd |= 0x20;
    }

    ATI_NEED_FIFO(2);
    if (dy > sy) {
	/* Bottom to Top */
	outw(CUR_Y, sy + hgt - 1);
	outw(DEST_Y, dy + hgt - 1);
    }
    else {
	outw(CUR_Y, sy);
	outw(DEST_Y, dy);

	cmd |= 0x80;
    }

    ATI_NEED_FIFO(3);
    outw(MAJ_AXIS_PCNT, wid - 1);
    outw(MIN_AXIS_PCNT, hgt - 1);
    outw(CMD, cmd);

#else
    ATI_NEED_FIFO(6);

    if ((dy == sy) &&
	(dx > sx)) {
	/* Right to Left */
	outw(SRC_X, sx + wid);
	outw(SRC_X_START, sx + wid);
	outw(SRC_X_END, sx);

	outw(CUR_X, dx + wid);
	outw(DEST_X_START, dx + wid);
	outw(DEST_X_END, dx);
    }
    else {
	outw(SRC_X, sx);
	outw(SRC_X_START, sx);
	outw(SRC_X_END, sx + wid);

	outw(CUR_X, dx);
	outw(DEST_X_START, dx);
	outw(DEST_X_END, dx + wid);
    }

    ATI_NEED_FIFO(4);

    if (dy > sy) {
	/* Bottom to Top */
	outw(SRC_Y, sy + hgt - 1);
	outw(SRC_Y_DIR, 0);

	outw(CUR_Y, dy + hgt - 1);
	outw(DEST_Y_END, dy - 1);
    }
    else {
	outw(SRC_Y, sy);
	outw(SRC_Y_DIR, 1);

	outw(CUR_Y, dy);
	outw(DEST_Y_END, dy + hgt);
    }
#endif

    return(SI_SUCCEED);
}
