#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiState.c	1.1"

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
 * This routine is provided so that the LFB layer can flush the
 * graphics engine before touching the FB itself.
 *
 */

void atiFlushGE()

{
    ATI_FLUSH_GE();
}

SIBool nextRect(ATI_OFF_SCRN *os, int w, int h)

{
    static int cur_x = 0, cur_y = 0;
    static int max_h = 0;
    int i;

    if ((os->valid == DATA_INVALID) &&
	(w <= os->w) &&
	(h <= os->h)) {

	os->valid = STRUCT_VALID;

	return(SI_SUCCEED);
    }

    if ((w > lfb.stride) ||
	(h > ati.bitmap_h)) {
	return(SI_FAIL);
    }

    if ((cur_x + w) > lfb.stride) {
	cur_x = 0;
	cur_y += max_h;
	max_h = 0;
    }

    if ((cur_y + h) > ati.bitmap_h) {
	cur_x = 0;
	max_h = 0;
	cur_y = 0;

	for (i = 0; i < LFB_NUM_GSTATES; i++) {
	    atiOffTiles[i].valid = STRUCT_INVALID;
	    atiOffStpls[i].valid = STRUCT_INVALID;
	}
    }

    os->x = cur_x;
    os->y = cur_y + ati.bitmap_off;
    os->w = w;
    os->h = h;
    os->valid = STRUCT_VALID;

    cur_x += w;
    if (h > max_h)
	max_h = h;

    return(SI_SUCCEED);
}

dummy()
{}


#ifdef ATI_SLOW_EXTENSIONS
/*
 * Expand a bit pattern so that each bit is replicated across a whole
 * pixel.  len bits are processed.  The least significant bit of each
 * byte in the pattern is processed first.
 *
 */

void atiBit2Pix(PixelP pix, u_long *bit, int len, Pixel c0, Pixel c1)

{
    register int i, j;
    register u_int k;
    Pixel4 conv[16];

    conv[ 0].p1 = c0; conv[ 0].p2 = c0; conv[ 0].p3 = c0; conv[ 0].p4 = c0;
    conv[ 1].p1 = c1; conv[ 1].p2 = c0; conv[ 1].p3 = c0; conv[ 1].p4 = c0;
    conv[ 2].p1 = c0; conv[ 2].p2 = c1; conv[ 2].p3 = c0; conv[ 2].p4 = c0;
    conv[ 3].p1 = c1; conv[ 3].p2 = c1; conv[ 3].p3 = c0; conv[ 3].p4 = c0;
    conv[ 4].p1 = c0; conv[ 4].p2 = c0; conv[ 4].p3 = c1; conv[ 4].p4 = c0;
    conv[ 5].p1 = c1; conv[ 5].p2 = c0; conv[ 5].p3 = c1; conv[ 5].p4 = c0;
    conv[ 6].p1 = c0; conv[ 6].p2 = c1; conv[ 6].p3 = c1; conv[ 6].p4 = c0;
    conv[ 7].p1 = c1; conv[ 7].p2 = c1; conv[ 7].p3 = c1; conv[ 7].p4 = c0;
    conv[ 8].p1 = c0; conv[ 8].p2 = c0; conv[ 8].p3 = c0; conv[ 8].p4 = c1;
    conv[ 9].p1 = c1; conv[ 9].p2 = c0; conv[ 9].p3 = c0; conv[ 9].p4 = c1;
    conv[10].p1 = c0; conv[10].p2 = c1; conv[10].p3 = c0; conv[10].p4 = c1;
    conv[11].p1 = c1; conv[11].p2 = c1; conv[11].p3 = c0; conv[11].p4 = c1;
    conv[12].p1 = c0; conv[12].p2 = c0; conv[12].p3 = c1; conv[12].p4 = c1;
    conv[13].p1 = c1; conv[13].p2 = c0; conv[13].p3 = c1; conv[13].p4 = c1;
    conv[14].p1 = c0; conv[14].p2 = c1; conv[14].p3 = c1; conv[14].p4 = c1;
    conv[15].p1 = c1; conv[15].p2 = c1; conv[15].p3 = c1; conv[15].p4 = c1;

    j = len / 32;
    for (i = 0; i < j; pix += 32, i++) {
	k = bit[i];
	*((Pixel4P)pix    ) = conv[(int)(k      ) & 0xf];
	*((Pixel4P)pix + 1) = conv[(int)(k >>  4) & 0xf];
	*((Pixel4P)pix + 2) = conv[(int)(k >>  8) & 0xf];
	*((Pixel4P)pix + 3) = conv[(int)(k >> 12) & 0xf];
	*((Pixel4P)pix + 4) = conv[(int)(k >> 16) & 0xf];
	*((Pixel4P)pix + 5) = conv[(int)(k >> 20) & 0xf];
	*((Pixel4P)pix + 6) = conv[(int)(k >> 24) & 0xf];
	*((Pixel4P)pix + 7) = conv[(int)(k >> 28) & 0xf];

/*
 * This call is bogus, but it gets around an apparent compiler bug in
 * the SVR4.2 (P14.1 - STC) compiler.
 */
	dummy();

    }

    len &= 31;
    j = bit[i];

    for (i = 0; i < len; i++) {
	pix[i] = (j & 1) ? c1 : c0;
	j >>= 1;
    }
}

#else

/*
 * Expand a bit pattern so that each bit is replicated across a whole
 * pixel.  len bits are processed.  The least significant bit of each
 * byte in the pattern is processed first.
 *
 */

void atiBit2Pix(PixelP pix, u_long *bit, int len)

{
    register int i, j;
    register u_int k;
    static Pixel4 conv[] = {
	{(Pixel) 0, (Pixel) 0, (Pixel) 0, (Pixel) 0},
	{(Pixel)~0, (Pixel) 0, (Pixel) 0, (Pixel) 0},
	{(Pixel) 0, (Pixel)~0, (Pixel) 0, (Pixel) 0},
	{(Pixel)~0, (Pixel)~0, (Pixel) 0, (Pixel) 0},
	{(Pixel) 0, (Pixel) 0, (Pixel)~0, (Pixel) 0},
	{(Pixel)~0, (Pixel) 0, (Pixel)~0, (Pixel) 0},
	{(Pixel) 0, (Pixel)~0, (Pixel)~0, (Pixel) 0},
	{(Pixel)~0, (Pixel)~0, (Pixel)~0, (Pixel) 0},
	{(Pixel) 0, (Pixel) 0, (Pixel) 0, (Pixel)~0},
	{(Pixel)~0, (Pixel) 0, (Pixel) 0, (Pixel)~0},
	{(Pixel) 0, (Pixel)~0, (Pixel) 0, (Pixel)~0},
	{(Pixel)~0, (Pixel)~0, (Pixel) 0, (Pixel)~0},
	{(Pixel) 0, (Pixel) 0, (Pixel)~0, (Pixel)~0},
	{(Pixel)~0, (Pixel) 0, (Pixel)~0, (Pixel)~0},
	{(Pixel) 0, (Pixel)~0, (Pixel)~0, (Pixel)~0},
	{(Pixel)~0, (Pixel)~0, (Pixel)~0, (Pixel)~0},
    };

    j = len / 32;
    for (i = 0; i < j; pix += 32, i++) {
	k = bit[i];
	*((Pixel4P)pix    ) = conv[(int)(k      ) & 0xf];
	*((Pixel4P)pix + 1) = conv[(int)(k >>  4) & 0xf];
	*((Pixel4P)pix + 2) = conv[(int)(k >>  8) & 0xf];
	*((Pixel4P)pix + 3) = conv[(int)(k >> 12) & 0xf];
	*((Pixel4P)pix + 4) = conv[(int)(k >> 16) & 0xf];
	*((Pixel4P)pix + 5) = conv[(int)(k >> 20) & 0xf];
	*((Pixel4P)pix + 6) = conv[(int)(k >> 24) & 0xf];
	*((Pixel4P)pix + 7) = conv[(int)(k >> 28) & 0xf];

/*
 * This call is bogus, but it gets around an apparent compiler bug in
 * the SVR4.2 (P14.1 - STC) compiler.
 */
	dummy();

    }

    len &= 31;
    j = bit[i];

    while (len--) {
	*pix++ = (j & 1) ? (Pixel)~0 : 0;
	j >>= 1;
    }
}

#endif

/*
 * atiRev reverses the bits in each byte of a u_long.  The Mach32 is
 * weird when it comes to the mono pattern registers.  PATT_DATA[16]
 * holds the lower 16 bits of the pattern.  Bit 7 controls the
 * leftmost pixel drawn.  Bits 0 and 15 are in the middle of the
 * pattern, and bit 8 is the rightmost bit.
 *
 */

u_long atiRev(u_long n)

{
    static u_char revTbl[] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
    };

    u_int ret;

    ret = revTbl[n & 0xff];
    ret |= revTbl[n >> 8 & 0xff] << 8;
    ret |= revTbl[n >> 16 & 0xff] << 16;
    ret |= revTbl[n >> 24 & 0xff] << 24;

    return(ret);
}

SIBool atiSetupStpl(SIbitmapP stpl)

{
    int i;
    ATI_OFF_SCRN *os = &atiOffStpls[lfb_cur_GState_idx];
    int pw, ph, repw = 0, reph = 0;
    int bw = stpl->Bwidth, bh = stpl->Bheight;
#ifdef ATI_SLOW_EXTENSIONS
    SIGStateP gstateP = lfb_cur_GStateP;
    int fg, bg;

    if (gstateP->SGstplmode == SGStipple) {
	fg = 0x01;
	bg = 0;
    }
    else {
	fg = gstateP->SGfg;
	bg = gstateP->SGbg;
    }
#endif

    if (os->valid == STRUCT_VALID) {
#ifdef ATI_SLOW_EXTENSIONS
	if ((os->fg == fg) &&
	    (os->bg == bg))
#endif

	    return(SI_SUCCEED);
    }
    else {
	pw = bw;
	while (pw < ATI_PIX_W) {
	    pw += pw;
	    repw++;
	}

	ph = bh;
	while (ph < ATI_PIX_H) {
	    ph += ph;
	    reph++;
	}

	if (! nextRect(os, bw, bh))
	    return(SI_FAIL);
    }

    ATI_FLUSH_GE();
    for (i = 0; i < bh; i++) {
#ifdef ATI_SLOW_EXTENSIONS
	atiBit2Pix(lfb.fb_ptr + (os->y + i) * lfb.stride + os->x,
		   (u_long *)BitmapScanL(stpl, i),
		   bw,
		   bg, fg);
#else
	atiBit2Pix(lfb.fb_ptr + (i + os->y) * lfb.stride + os->x,
		   (u_long *)BitmapScanL(stpl, i),
		   bw);
#endif
    }

    ATI_NEED_FIFO(2);
    outw(FRGD_MIX, 0x67);
    outw(PIXEL_CNTL, 0xa000);

    i = atiClipping;
    if (i)
	atiClipOff();

    while (repw) {
	ATI_NEED_FIFO(7);
	outw(CUR_X, os->x);
	outw(CUR_Y, os->y);
	outw(DEST_X, os->x + bw);
	outw(DEST_Y, os->y);
	outw(MAJ_AXIS_PCNT, bw - 1);
	outw(MIN_AXIS_PCNT, bh - 1);
	outw(CMD, 0xc0b3);

	repw--;
	bw += bw;
    }

    while (reph) {
	ATI_NEED_FIFO(7);
	outw(CUR_X, os->x);
	outw(CUR_Y, os->y);
	outw(DEST_X, os->x);
	outw(DEST_Y, os->y + bh);
	outw(MAJ_AXIS_PCNT, bw - 1);
	outw(MIN_AXIS_PCNT, bh - 1);
	outw(CMD, 0xc0b3);

	reph--;
	bh += bh;
    }

    if (i)
	atiClipOn();

    os->fg = fg;
    os->bg = bg;
    os->pw = pw;
    os->ph = ph;

    return(SI_SUCCEED);
}

SIBool atiSetupTile(SIbitmapP tile)

{
    int i;
    ATI_OFF_SCRN *os = &atiOffTiles[lfb_cur_GState_idx];
    int pw, ph, repw = 0, reph = 0;
    int bw = tile->Bwidth, bh = tile->Bheight;

    if (os->valid != STRUCT_VALID) {
	pw = bw;
	while (pw < ATI_PIX_W) {
	    pw += pw;
	    repw++;
	}

	ph = bh;
	while (ph < ATI_PIX_H) {
	    ph += ph;
	    reph++;
	}

	if (! nextRect(os, pw, ph))
	    return(SI_FAIL);

	ATI_FLUSH_GE();
	for (i = 0; i < bh; i++) {
	    memmove(lfb.fb_ptr + (os->y + i) * lfb.stride + os->x,
		    BitmapScanL(tile, i),
		    bw * sizeof(Pixel));
	}

	ATI_NEED_FIFO(2);
	outw(FRGD_MIX, 0x67);
	outw(PIXEL_CNTL, 0xa000);

	i = atiClipping;
	if (i)
	    atiClipOff();

	while (repw) {
	    ATI_NEED_FIFO(7);
	    outw(CUR_X, os->x);
	    outw(CUR_Y, os->y);
	    outw(DEST_X, os->x + bw);
	    outw(DEST_Y, os->y);
	    outw(MAJ_AXIS_PCNT, bw - 1);
	    outw(MIN_AXIS_PCNT, bh - 1);
	    outw(CMD, 0xc0b3);

	    repw--;
	    bw += bw;
	}

	while (reph) {
	    ATI_NEED_FIFO(7);
	    outw(CUR_X, os->x);
	    outw(CUR_Y, os->y);
	    outw(DEST_X, os->x);
	    outw(DEST_Y, os->y + bh);
	    outw(MAJ_AXIS_PCNT, bw - 1);
	    outw(MIN_AXIS_PCNT, bh - 1);
	    outw(CMD, 0xc0b3);

	    reph--;
	    bh += bh;
	}

	if (i)
	    atiClipOn();

	os->pw = pw;
	os->ph = ph;
    }

    return(SI_SUCCEED);
}

SIBool atiDownLoadState(sindex, sflag, stateP)
SIint32 sindex, sflag;
SIGStateP stateP;

{
    if (sflag & SetSGtile)
	atiOffTiles[sindex].valid = DATA_INVALID;

    if (sflag & SetSGstipple)
	atiOffStpls[sindex].valid = DATA_INVALID;

    return(lfbDownLoadState(sindex, sflag, stateP));
}
