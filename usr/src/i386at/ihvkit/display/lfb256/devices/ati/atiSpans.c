#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiSpans.c	1.1"

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

/*	HARDWARE SPANS CONTROL		*/
/*		OPTIONAL		*/

SIBool atiFillSpans(count, ptsIn, widths)
SIint32 count;
SIPointP ptsIn;
SIint32 *widths;

{
    ATI_OFF_SCRN *os;
    SIbitmapP bm;
    int sx, sy, y, w, l, l1, r, ret, fmode, dmode;
    SIGStateP gstateP = lfb_cur_GStateP;
#ifndef ATI_SLOW_EXTENSIONS
    int i;
    u_long u;
    SIint16 *p;
#endif

    if (atiClipping)
	atiClipOff();

    fmode = gstateP->SGfillmode;
    dmode = ati_mode_trans[gstateP->SGmode];

    ATI_NEED_FIFO(4);
    outw(WRT_MASK, gstateP->SGpmask);
    outw(FRGD_COLOR, gstateP->SGfg);

#ifdef ATI_SLOW_EXTENSIONS
    outw(PIXEL_CNTL, 0xa000);
#else
    outw(LINEDRAW_OPT, 0x0004);
    outw(ALU_FG_FN, ati_mode_trans[gstateP->SGmode]);
#endif

    switch (fmode) {
      case SGFillSolidFG:
      case SGFillSolidBG:

#ifdef ATI_SLOW_EXTENSIONS
	ATI_NEED_FIFO(1);
	if (fmode == SGFillSolidFG)
	    outw(FRGD_MIX, dmode | 0x20);
	else
	    outw(FRGD_MIX, dmode);

	for (; count-- > 0; ptsIn++, widths++) {
	    if (*widths > 0) {
		ATI_NEED_FIFO(4);
		outw(CUR_X, ptsIn->x);
		outw(CUR_Y, ptsIn->y);
		outw(MAJ_AXIS_PCNT, *widths);
		outw(CMD, 0x201f);
	    }
	}
#else
	ATI_NEED_FIFO(1);
	if (fmode == SGFillSolidFG)
	    outw(DP_CONFIG, 0x3211);
	else
	    outw(DP_CONFIG, 0x1211);

	for (; count--; ptsIn++, widths++) {
	    if (*widths > 0) {
		ATI_NEED_FIFO(3);

		outw(CUR_X, ptsIn->x);
		outw(CUR_Y, ptsIn->y);
		outw(SCAN_X, ptsIn->x + *widths);
	    }
	}
#endif

	break;

      case SGFillStipple:
      case SGFillTile:

#if (BPP > 8) && !defined(ATI_SLOW_EXTENSIONS)
#error Need to handle tiles when BPP > 8
#endif

	if (fmode == SGFillTile)
	    bm = gstateP->SGtile;
	else
	    bm = gstateP->SGstipple;

#ifndef ATI_SLOW_EXTENSIONS
	ATI_NEED_FIFO(1);
	if (gstateP->SGstplmode == SGOPQStipple)
	    outw(ALU_BG_FN, dmode);
	else
	    outw(ALU_BG_FN, 0x03);

	if (bm->Bwidth <= 32) {
	    ATI_NEED_FIFO(1);

	    if (fmode == SGFillTile)
		outw(DP_CONFIG, 0xb211);
	    else
		outw(DP_CONFIG, 0x3231);

	    for (; count--; ptsIn++, widths++) {
		if (!*widths)
		    continue;

		l = ptsIn->x;
		r = l + *widths;
		y = ptsIn->y;

		sx = (l - bm->BorgX) % bm->Bwidth;
		sy = (y - bm->BorgY) % bm->Bheight;

		if (fmode == SGFillTile) {
		    ATI_NEED_FIFO(bm->Bwidth + 1);

		    outw(PATT_DATA_INDEX, 0x0);

		    p = (SIint16 *)(bm->Bptr + sy);
		    for (i = 0; i < bm->Bwidth; i+=2)
			outw(PATT_DATA, *p++);
		}
		else {
		    ATI_NEED_FIFO(1);

		    outw(PATT_DATA_INDEX, 0x10);

		    u = *(bm->Bptr + sy);
		    u = atiRev(u);
		    outw(PATT_DATA, u);
		    outw(PATT_DATA, u >> 16);
		}

		ATI_NEED_FIFO(2);
		outw(PATT_LENGTH, bm->Bwidth - 1);
		outw(PATT_INDEX, sx);

		ATI_NEED_FIFO(3);
		outw(CUR_X, ptsIn->x);
		outw(CUR_Y, ptsIn->y);
		outw(SCAN_X, ptsIn->x + *widths);
	    }
	}
	else {
#endif

	    if (fmode == SGFillTile) {
		ret = atiSetupTile(gstateP->SGtile);
		os = &atiOffTiles[lfb_cur_GState_idx];
#ifndef ATI_SLOW_EXTENSIONS
		ATI_NEED_FIFO(1);
		outw(DP_CONFIG, 0x7211);
#endif
	    }
	    else {
		ret = atiSetupStpl(gstateP->SGstipple);
		os = &atiOffStpls[lfb_cur_GState_idx];
#ifndef ATI_SLOW_EXTENSIONS
		ATI_NEED_FIFO(1);
		outw(DP_CONFIG, 0x3271);
#endif
	    }

	    if (!ret)
		return(SI_FAIL);

#ifdef ATI_SLOW_EXTENSIONS
	    if ((fmode == SGFillStipple) &&
		(gstateP->SGstplmode == SGStipple)) {

		ATI_NEED_FIFO(4);
		outw(PIXEL_CNTL, 0xa0c0);
		/* RD_MASK is 2 because IBM rotated the mask */
		outw(RD_MASK, 0x02);
		outw(FRGD_MIX, dmode | 0x20);
		outw(BKGD_MIX, 0x03);
	    }
	    else {
		ATI_NEED_FIFO(1);
		outw(FRGD_MIX, dmode | 0x60);
	    }
#else
	    ATI_NEED_FIFO(1);
	    if (gstateP->SGstplmode == SGOPQStipple)
		outw(ALU_BG_FN, dmode);
	    else
		outw(ALU_BG_FN, 0x03);
#endif

	    for (; count--; ptsIn++, widths++) {
		if (!*widths)
		    continue;

		l = ptsIn->x;
		r = l + *widths;
		y = ptsIn->y;

		sy = (y - bm->BorgY) % bm->Bheight;
		sx = (l - bm->BorgX) % bm->Bwidth;
		l1 = l;

		while (l1 < r) {
		    w = min(r - l1, os->pw - sx);

#ifdef ATI_SLOW_EXTENSIONS
		    ATI_NEED_FIFO(7);
		    outw(CUR_X, os->x + sx);
		    outw(CUR_Y, os->y + sy);
		    outw(DEST_X, l1);
		    outw(DEST_Y, y);
		    outw(MAJ_AXIS_PCNT, w - 1);
		    outw(MIN_AXIS_PCNT, 0);
		    outw(CMD, 0xc0b3);
#else
		    ATI_NEED_FIFO(5);
		    outw(SRC_X, os->x + sx);
		    outw(SRC_X_START, os->x + sx);
		    outw(SRC_X_END, os->x + sx + w);
		    outw(SRC_Y, os->y + sy);
		    outw(SRC_Y_DIR, 1);

		    ATI_NEED_FIFO(5);
		    outw(CUR_X, l1);
		    outw(DEST_X_START, l1);
		    outw(DEST_X_END, l1 + w);
		    outw(CUR_Y, y);
		    outw(DEST_Y_END, y + 1);
#endif

		    l1 += w;
		    sx = 0;
		}
	    }
#ifndef ATI_SLOW_EXTENSIONS
	}
#endif

	break;

      default:
	return(SI_FAIL);
    }

    return(SI_SUCCEED);
}
