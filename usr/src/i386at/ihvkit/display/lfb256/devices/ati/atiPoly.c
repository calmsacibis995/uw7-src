#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiPoly.c	1.1"

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

SIBool atiFillRects(xOrg, yOrg, count, prects)
SIint32 xOrg, yOrg;
SIint32 count;
SIRectOutlineP prects;

{
    register int l, r, t, b;
    ATI_OFF_SCRN *os;
    SIbitmapP bm;
    int sx, sy, h, w, l1, ret, fmode, dmode;
    SIGStateP gstateP = lfb_cur_GStateP;
#ifndef ATI_SLOW_EXTENSIONS
    int i, k, m;
    u_long u;
    SIint16 *p;
#endif

    if (! atiClipping)
	atiClipOn();

    fmode = gstateP->SGfillmode;
    dmode = ati_mode_trans[gstateP->SGmode];

    ATI_NEED_FIFO(3);
    outw(WRT_MASK, gstateP->SGpmask);
    outw(FRGD_COLOR, gstateP->SGfg);

#ifdef ATI_SLOW_EXTENSIONS
    outw(PIXEL_CNTL, 0xa000);
#else
    outw(ALU_FG_FN, ati_mode_trans[gstateP->SGmode]);
#endif

    switch (fmode) {
      case SGFillSolidFG:
      case SGFillSolidBG:

	ATI_NEED_FIFO(1);

#ifdef ATI_SLOW_EXTENSIONS
	if (fmode == SGFillSolidFG)
	    outw(FRGD_MIX, dmode | 0x20);
	else
	    outw(FRGD_MIX, dmode);
#else
	if (fmode == SGFillSolidFG)
	    outw(DP_CONFIG, 0x3211);
	else
	    outw(DP_CONFIG, 0x1211);
#endif

	for (; count--; prects++) {
	    ATI_NEED_FIFO(5);

	    outw(CUR_X, prects->x + xOrg);
	    outw(CUR_Y, prects->y + yOrg);

#ifdef ATI_SLOW_EXTENSIONS
	    outw(MAJ_AXIS_PCNT, prects->width - 1);
	    outw(MIN_AXIS_PCNT, prects->height - 1);
	    outw(CMD, 0x40b3);
#else
	    outw(DEST_X_START, prects->x + xOrg);
	    outw(DEST_X_END, prects->x + prects->width + xOrg);
	    outw(DEST_Y_END, prects->y + prects->height + yOrg);
#endif
	}
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
	    ATI_NEED_FIFO(2);

	    if (fmode == SGFillTile)
		outw(DP_CONFIG, 0xb211);
	    else
		outw(DP_CONFIG, 0x3231);

	    outw(LINEDRAW_OPT, 0x0004);

	    for (; count--; prects++) {
		l = prects->x + xOrg;
		r = l + prects->width;
		t = prects->y + yOrg;
		b = t + prects->height;

		if ((t == b) || (l == r))
		    continue;

		sx = (l - bm->BorgX) % bm->Bwidth;
		sy = (t - bm->BorgY) % bm->Bheight;

		for (k = 0; k < bm->Bheight; k++, t++) {
		    if (fmode == SGFillTile) {
			ATI_NEED_FIFO(bm->Bwidth + 2);

			outw(PATT_DATA_INDEX, 0x0);

			p = (SIint16 *)(bm->Bptr + sy);
			for (i = 0; i < bm->Bwidth; i+=2)
			    outw(PATT_DATA, *p++);
		    }
		    else {
			ATI_NEED_FIFO(4);

			outw(PATT_DATA_INDEX, 0x10);
			u = *(bm->Bptr + sy);
			u = atiRev(u);
			outw(PATT_DATA, u);
			outw(PATT_DATA, u >> 16);
		    }
		    outw(PATT_LENGTH, bm->Bwidth - 1);

		    for (m = t; m < b; m += bm->Bheight) {
			ATI_NEED_FIFO(4);
			outw(PATT_INDEX, sx);
			outw(CUR_X, l);
			outw(CUR_Y, m);
			outw(SCAN_X, r);
		    }

		    if (++sy == bm->Bheight)
			sy = 0;
		}
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

	    if (! ret)
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

	    for (; count--; prects++) {
		l = prects->x + xOrg;
		r = l + prects->width;
		t = prects->y + yOrg;
		b = t + prects->height;

		if ((l == r) || (t == b))
		    continue;

		sy = (t - bm->BorgY) % bm->Bheight;

		while (t < b) {
		    h = min(b - t, os->ph - sy);
		    sx = (l - bm->BorgX) % bm->Bwidth;
		    l1 = l;

		    while (l1 < r) {
			w = min(r - l1, os->pw - sx);
#ifdef ATI_SLOW_EXTENSIONS
			ATI_NEED_FIFO(7);
			outw(CUR_X, os->x + sx);
			outw(CUR_Y, os->y + sy);
			outw(DEST_X, l1);
			outw(DEST_Y, t);
			outw(MAJ_AXIS_PCNT, w - 1);
			outw(MIN_AXIS_PCNT, h - 1);
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
			outw(CUR_Y, t);
			outw(DEST_Y_END, t + h);
#endif
			l1 += w;
			sx = 0;
		    }

		    t += h;
		    sy = 0;
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
