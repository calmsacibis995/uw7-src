#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiLine.c	1.1"

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
#include <atiLine.h>

/*	HARDWARE LINE DRAWING		*/
/*		OPTIONAL		*/

SIBool atiThinLines(xOrg, yOrg, count, pts, isCapNotLast, coordMode)
SIint32 xOrg, yOrg;
SIint32 count;
SIPointP pts;
SIint32 isCapNotLast, coordMode;

{
    SIGStateP gstateP = lfb_cur_GStateP;
    int finalCmd;
    register int lastx, lasty;
    register int thisx, thisy;
    int startx, starty;

    if (count < 2)
	return(SI_SUCCEED);

    if (! atiClipping)
	atiClipOn();

    if (gstateP->SGlinestyle != SGLineSolid)
	return(SI_FAIL);

    ATI_NEED_FIFO(2);
    outw(WRT_MASK, gstateP->SGpmask);
    outw(FRGD_COLOR, gstateP->SGfg);

#ifdef ATI_SLOW_EXTENSIONS
    ATI_NEED_FIFO(2);
    outw(FRGD_MIX, ati_mode_trans[gstateP->SGmode] | 0x20);
    outw(PIXEL_CNTL, 0xa000);

    count--;

    lastx = pts[0].x + xOrg;
    lasty = pts[0].y + yOrg;
    pts++;
    count--;

    startx = lastx;
    starty = lasty;
    if (coordMode == SICoordModeOrigin) {
	while (count--) {
	    thisx = pts[0].x + xOrg;
	    thisy = pts[0].y + yOrg;

	    DRAW_LINE(lastx, lasty, thisx, thisy, 0x2017);

	    lastx = thisx;
	    lasty = thisy;

	    pts++;
	}

	thisx = pts[0].x + xOrg;
	thisy = pts[0].y + yOrg;
    }
    else {
	while (count--) {
	    thisx = pts[0].x + lastx;
	    thisy = pts[0].y + lasty;

	    DRAW_LINE(lastx, lasty, thisx, thisy, 0x2017);

	    lastx = thisx;
	    lasty = thisy;

	    pts++;
	}

	thisx = pts[0].x + lastx;
	thisy = pts[0].y + lasty;
    }

    if (isCapNotLast ||
	((startx == thisx) &&
	 (starty == thisy)))
	finalCmd = 0x2017;
    else
	finalCmd = 0x2013;

    DRAW_LINE(lastx, lasty, thisx, thisy, finalCmd);

#else
    /*
     * The correct way to do the following would be the following:
     *
     *    ATI_NEED_FIFO(3);
     *    outw(DP_CONFIG, 0x3211);
     *    outw(LINEDRAW_OPT, 0x0004);
     *    outw(LINEDRAW_INDEX, 0);
     *
     *    while (--count > 0) {
     *        ATI_NEED_FIFO(2);
     *
     *        outw(LINEDRAW, pts[0].x);
     *        outw(LINEDRAW, pts[0].y);
     *
     *        pts++;
     *    }
     *
     * However, we noticed that there was a perfomance improvement
     * when we re-loaded the common point between the two lines.  Just
     * another one of those annoying things about performance on this
     * chip.  Of course, we don't use any of this code, since the 8514
     * commands are faster anyway.
     *
     */

    ATI_NEED_FIFO(2);
    outw(ALU_FG_FN, ati_mode_trans[gstateP->SGmode]);
    outw(DP_CONFIG, 0x3211);
    outw(LINEDRAW_OPT, 0x0004);

    count--;

    if (isCapNotLast ||
	((pts[0].x == pts[count].x) &&
	 (pts[0].y == pts[count].y)))
	finalCmd = 0x0004;
    else
	finalCmd = 0x0000;

    lastx = pts[0].x + xOrg;
    lasty = pts[0].y + yOrg;
    pts++;
    count--;

    if (coordMode == SICoordModeOrigin) {
	while (count--) {
	    ATI_NEED_FIFO(5);
	    outw(LINEDRAW_INDEX, 0);

	    outw(LINEDRAW, lastx);
	    outw(LINEDRAW, lasty);

	    lastx = pts[0].x + xOrg;
	    lasty = pts[0].y + yOrg;

	    outw(LINEDRAW, lastx);
	    outw(LINEDRAW, lasty);

	    pts++;
	}

	ATI_NEED_FIFO(6);
	outw(LINEDRAW_OPT, finalCmd);
	outw(LINEDRAW_INDEX, 0);

	outw(LINEDRAW, lastx);
	outw(LINEDRAW, lasty);
	
	lastx = pts[0].x + xOrg;
	lasty = pts[0].y + yOrg;

	outw(LINEDRAW, lastx);
	outw(LINEDRAW, lasty);
    }
    else {
	while (count--) {
	    ATI_NEED_FIFO(5);
	    outw(LINEDRAW_INDEX, 0);

	    outw(LINEDRAW, lastx);
	    outw(LINEDRAW, lasty);

	    lastx = pts[0].x + lasty;
	    lasty = pts[0].y + lastY;

	    outw(LINEDRAW, lastx);
	    outw(LINEDRAW, lasty);

	    pts++;
	}

	ATI_NEED_FIFO(6);
	outw(LINEDRAW_OPT, finalCmd);
	outw(LINEDRAW_INDEX, 0);

	outw(LINEDRAW, lastx);
	outw(LINEDRAW, lasty);
	
	lastx = pts[0].x + xOrg;
	lasty = pts[0].y + yOrg;

	outw(LINEDRAW, lastx);
	outw(LINEDRAW, lasty);
    }
#endif

    return(SI_SUCCEED);
}

SIBool atiThinSegments(xOrg, yOrg, count, segs, isCapNotLast)
SIint32 xOrg, yOrg;
SIint32 count;
SISegmentP segs;
SIint32 isCapNotLast;

{
    SIGStateP gstateP = lfb_cur_GStateP;
    int cmd;
    register int x1, y1, x2, y2;

    if (! atiClipping)
	atiClipOn();

    if (gstateP->SGlinestyle != SGLineSolid)
	return(SI_FAIL);

    ATI_NEED_FIFO(2);
    outw(WRT_MASK, gstateP->SGpmask);
    outw(FRGD_COLOR, gstateP->SGfg);

#ifdef ATI_SLOW_EXTENSIONS
    ATI_NEED_FIFO(2);
    outw(FRGD_MIX, ati_mode_trans[gstateP->SGmode] | 0x20);
    outw(PIXEL_CNTL, 0xa000);

    if (isCapNotLast)
	cmd = 0x2017;
    else
	cmd = 0x2013;

    while (count--) {
	x1 = segs->x1 + xOrg; y1 = segs->y1 + yOrg;
	x2 = segs->x2 + xOrg; y2 = segs->y2 + yOrg;

	DRAW_LINE(x1, y1, x2, y2, cmd);

	segs++;
    }

#else
    ATI_NEED_FIFO(3);
    outw(ALU_FG_FN, ati_mode_trans[gstateP->SGmode]);
    outw(DP_CONFIG, 0x3211);

    if (isCapNotLast)
	outw(LINEDRAW_OPT, 0x0004);
    else
	outw(LINEDRAW_OPT, 0x0000);

    while (count) {
	ATI_NEED_FIFO(5);

	outw(LINEDRAW_INDEX, 0);

	outw(LINEDRAW, segs->x1 + xOrg);
	outw(LINEDRAW, segs->y1 + yOrg);

	outw(LINEDRAW, segs->x2 + xOrg);
	outw(LINEDRAW, segs->y2 + yOrg);

	segs++;
	count--;
    }
#endif

    return(SI_SUCCEED);
}

SIBool atiThinRect(x1, y1, x2, y2)
SIint32 x1, y1, x2, y2;

{
    SIGStateP gstateP = lfb_cur_GStateP;

    if (! atiClipping)
	atiClipOn();

    if (gstateP->SGlinestyle != SGLineSolid)
	return(SI_FAIL);

    ATI_NEED_FIFO(5);
    outw(WRT_MASK, gstateP->SGpmask);
    outw(FRGD_COLOR, gstateP->SGfg);

#ifdef ATI_SLOW_EXTENSIONS
    ATI_NEED_FIFO(2);
    outw(FRGD_MIX, ati_mode_trans[gstateP->SGmode] | 0x20);
    outw(PIXEL_CNTL, 0xa000);

    ATI_NEED_FIFO(4);
    outw(CUR_X, x1);
    outw(CUR_Y, y1);

    if (x2 > x1) {
	outw(MAJ_AXIS_PCNT, x2 - x1);
	outw(CMD, 0x201f);
    }
    else {
	outw(MAJ_AXIS_PCNT, x1 - x2);
	outw(CMD, 0x209f);
    }

    ATI_NEED_FIFO(4);
    outw(CUR_X, x2);
    outw(CUR_Y, y1);

    if (y2 > y1) {
	outw(MAJ_AXIS_PCNT, y2 - y1);
	outw(CMD, 0x20df);
    }
    else {
	outw(MAJ_AXIS_PCNT, y1 - y2);
	outw(CMD, 0x205f);
    }

    ATI_NEED_FIFO(4);
    outw(CUR_X, x2);
    outw(CUR_Y, y2);

    if (x1 > x2) {
	outw(MAJ_AXIS_PCNT, x1 - x2);
	outw(CMD, 0x201f);
    }
    else {
	outw(MAJ_AXIS_PCNT, x2 - x1);
	outw(CMD, 0x209f);
    }

    ATI_NEED_FIFO(4);
    outw(CUR_X, x1);
    outw(CUR_Y, y2);

    if (y1 > y2) {
	outw(MAJ_AXIS_PCNT, y1 - y2);
	outw(CMD, 0x20df);
    }
    else {
	outw(MAJ_AXIS_PCNT, y2 - y1);
	outw(CMD, 0x205f);
    }

#else
    ATI_NEED_FIFO(3);
    outw(ALU_FG_FN, ati_mode_trans[gstateP->SGmode]);
    outw(DP_CONFIG, 0x3211);
    outw(LINEDRAW_OPT, 0x0004);

    ATI_NEED_FIFO(3);
    outw(CUR_X, x1);
    outw(CUR_Y, y1);
    /* SCAN_X is more than twice the speed of LINEDRAW. */
    outw(SCAN_X, x2);

    ATI_NEED_FIFO(5);
    outw(LINEDRAW_INDEX, 0);
    outw(LINEDRAW, x2);
    outw(LINEDRAW, y1);
    outw(LINEDRAW, x2);
    outw(LINEDRAW, y2);

    ATI_NEED_FIFO(3);
    outw(CUR_X, x2);
    outw(CUR_Y, y2);
    outw(SCAN_X, x1);

    ATI_NEED_FIFO(5);
    outw(LINEDRAW_INDEX, 0);
    outw(LINEDRAW, x1);
    outw(LINEDRAW, y2);
    outw(LINEDRAW, x1);
    outw(LINEDRAW, y1);
#endif

    return(SI_SUCCEED);
}
