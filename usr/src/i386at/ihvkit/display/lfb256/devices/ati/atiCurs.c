#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiCurs.c	1.1"

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
 * See comments in <ati.h> regarding CURSOR_BUG_R3
 *
 */

#ifdef CURSOR_BUG_R3
#undef ATI_CURSOR_WIDTH
#define ATI_CURSOR_WIDTH ((lfb.width==1280)?62:64)
#endif

#define CursorOffset(curs, y) \
    (ati.curs_off * lfb.stride * sizeof(Pixel) + \
     ((curs) * ATI_CURSOR_SIZE + (y) * ATI_CURSOR_STRIDE) * sizeof(SIint16))

#define CursorScanL(curs, y) \
    ((SIint16 *)(lfb.fb_ptr + ati.curs_off * lfb.stride) + \
     (curs) * ATI_CURSOR_SIZE + \
     (y) * ATI_CURSOR_STRIDE)

static int cur_curs = -1;
static int curs_on = SI_FALSE;
static u_int curs_colors[ATI_NUM_CURSORS][2];

/*
 * See the comments in atiMoveCurs() for a description of these
 * variables.
 *
 */

#ifdef CURSOR_BUG_R3
static int leftBlank=1, topBlank=0;
#else
static int leftBlank=0, topBlank=0;
#endif

/*
 * Macros for going from a pair of bitmaps to a 2 plane bitmap.  p is
 * a pointer to a line in the cursor; b is the bit number to test/set.
 * TEST returns non-0 if bit b is set.  SET sets bit b.  CLEAR clears
 * bit b.
 *
 * Notes:
 *
 *   - These macros are very specific to the setup we have.  TEST
 *     expects p to point to a SIint32.  SET and CLEAR expect p to
 *     point to an SIint16.
 *
 *   - TEST does not return 0/1.  It returns 2^b.  This is only useful
 *     when testing a bit.
 *
 *   - These macros do not check b against a width.  The caller must
 *     do so.
 *
 */

#define TEST(p,b) (*((p) + (b)/32) & (1 << ((b)%32)))
#define SET(p,b) *((p)+(b)/16) |= 1 << ((b)%16)
#define CLEAR(p,b) *((p)+(b)/16) &= ~(1 << ((b)%16))

/*	CURSOR CONTROL ROUTINES		*/
/*		MANDATORY		*/

/*
 *
 * The ATI Mach32 has a two color cursor with transparency as follows:
 *
 *   00  Color 0
 *   01  Color 1
 *   10  Transparent
 *   11  Complement
 *
 * Color 0 is used for the background and color 1 for the forground.
 * This allows us to set the lower bit based on the src.  The mask
 * will determine the upper bit.  When the mask is clear, we will not
 * set the upper bit.
 *
 */

SIBool atiDownLoadCurs(cindex, cp)
SIint32 cindex;
SICursorP cp;
{
    SIbitmapP m, s;
    register int i, j, stride;
    register SIArray ps, pm;
    register SIint16 *p;

    /*
     * There is so much redundant info here that we will do the error
     * checking that we shouldn't have to do.
     */
    m = cp->SCmask;
    s = cp->SCsrc;

    if ((s->BbitsPerPixel != 1) ||
	(m->BbitsPerPixel != 1)) {

	fprintf(stderr,
		"DL cursor: bad bitmap depth (src=%d msk=%d)\n",
		s->BbitsPerPixel, m->BbitsPerPixel);
	return(SI_FAIL);
    }

    if ((cp->SCwidth != s->Bwidth) ||
	(cp->SCwidth != m->Bwidth)) {

	fprintf(stderr,
		"DL cursor: bitmap width mismatch (%d src=%d msk=%d)\n",
		cp->SCwidth, s->Bwidth, m->Bwidth);
	return(SI_FAIL);
    }

    if ((cp->SCheight != s->Bheight) ||
	(cp->SCheight != m->Bheight)) {

	fprintf(stderr,
		"DL cursor: bitmap height mismatch (%d src=%d msk=%d)\n",
		cp->SCheight, s->Bheight, m->Bheight);
	return(SI_FAIL);
    }

    if (cp->SCwidth > ATI_CURSOR_WIDTH) {
	cp->SCwidth = ATI_CURSOR_WIDTH;
    }

    if (cp->SCheight > ATI_CURSOR_HEIGHT) {
	cp->SCheight = ATI_CURSOR_HEIGHT;
    }

    if ((cindex == cur_curs) &&
	(curs_on == SI_TRUE)) {
	/* Turn off the cursor */
	outb(CURSOR_OFFSET_HI+1, 0x00);
    }

    memset(CursorScanL(cindex, 0), 0xaa, ATI_CURSOR_SIZE * sizeof(SIint16));

    stride = (s->Bwidth + 31) / 32;

    for (i = 0; i < cp->SCheight; i++) {
	ps = s->Bptr + stride * i;
	pm = m->Bptr + stride * i;
	p = CursorScanL(cindex, i);

	/*
	 * Test bit j of the current line in both the mask and the
	 * bitmap.  In the cursor, set bit 2j+1 when the mask is clear
	 * and set bit 2j when the the mask and bitmap are set.  Just
	 * in case the CORE doesn't mask the bitmap, this code will
	 * only check the bitmap if the mask was set.
	 *
  	 */

	for (j = 0; j < cp->SCwidth; j++) {
	    if (TEST(pm, j)) {
#ifdef CURSOR_BUG_R3
		CLEAR(p, j+j+3);
#else
		CLEAR(p, j+j+1);
#endif
		if (TEST(ps, j)) {
#ifdef CURSOR_BUG_R3
		    SET(p, j+j+2);
#else
		    SET(p, j+j);
#endif
		}
	    }
	}
    }

#if (BPP == 8)
    curs_colors[cindex][0] = cp->SCbg;
    curs_colors[cindex][1] = cp->SCfg;

    if (cindex == cur_curs) {
	outb(CURSOR_COLOR_0, cp->SCbg);
	outb(CURSOR_COLOR_1, cp->SCfg);

	if (curs_on == SI_TRUE)
	/* Turn on the cursor */
	outb(CURSOR_OFFSET_HI+1, 0x80);
    }

#else
#error Need cursor color code for BPP > 8
#endif

    return(SI_SUCCEED);
}

SIBool atiTurnOnCurs(cindex)
SIint32 cindex;
{
    register int i;

    if (cindex != cur_curs) {
	/* Turn off the cursor */
	if (curs_on == SI_TRUE) {
	    outb(CURSOR_OFFSET_HI+1, 0x00);
	    curs_on = SI_FALSE;
	}

	cur_curs = cindex;

	outb(HORZ_CURSOR_OFFSET, leftBlank);
	outb(VERT_CURSOR_OFFSET, topBlank);

	i = CursorOffset(cindex, topBlank) >> 2;
	outw(CURSOR_OFFSET_LO, i & 0xffff);
	outb(CURSOR_OFFSET_HI, (i >> 16) & 0x0f);

#if (BPP == 8)
	outb(CURSOR_COLOR_0, curs_colors[cindex][0]);
	outb(CURSOR_COLOR_1, curs_colors[cindex][1]);
#else
#error Need cursor color code for BPP > 8
#endif

    }

    if (curs_on == SI_FALSE) {
	outb(CURSOR_OFFSET_HI+1, 0x80);
	curs_on = SI_TRUE;
    }

    return(SI_SUCCEED);
}

SIBool atiTurnOffCurs(cindex)
SIint32 cindex;
{
    if (curs_on == SI_TRUE) {
	outb(CURSOR_OFFSET_HI+1, 0x00);
	curs_on = SI_FALSE;
    }

    return(SI_SUCCEED);
}

SIBool atiMoveCurs(cindex, x, y)
SIint32 cindex;
SIint32 x, y;
{
    register int i;

    if ((curs_on == SI_TRUE) &&
	(cur_curs != cindex))
	atiTurnOnCurs(cindex);

    /*
     * The Mach32 cursor uses an 11 bit field for the cursor position.  The
     * cursor hot spot is the upper left corner of the cursor.  As a
     * result, the cursor cannot move off the left or the top of the
     * screen.
     *
     * The solution is as follows:
     *
     * For the left side, set HORZ_CURSOR_OFFSET when the cursor needs
     * to move off screen.  This tells the chip to skip this number of
     * pixels in to the cursor definition.  (It also makes the cursor
     * narrower, which means that it does the right thing.)
     *
     * The top is more difficult.  VERT_CURSOR_OFFSET blanks the bottom
     * portion of the cursor, not the top.  The solution is to adjust
     * CURSOR_OFFSET_{HI,LO} when the cursor is off the top of the screen.
     * Then set VERT_CURSOR_OFFSET to the number of scan lines to blank
     * from the bottom.
     *
     */

    /*
     * Note: The Mach32 uses different registers to determine the
     * Graphics Engine drawing area and the display area.  When used
     * properly, these registers can be used to create a virtual root
     * window that is larger than the screen area.  For example, we
     * could have a root window of 1536x1280 displayed on a 1024x768 monitor.
     *
     * This routine could modify CRT_OFFSET_{HI,LO} to adjust the
     * visible portion of the screen. It would also need to keep track
     * of how much we adjusted the screen.
     *
     * The Graphics Engine would be set up to use the larger screen
     * area at initialization.  The virtual root window dimensions
     * would be used for the GE and for the size reported back to the
     * CORE server.  The display size would be known only to portions
     * of the ATI SDD.
     *
     */

    x = max(min(x, 2047), -ATI_CURSOR_WIDTH + 1);
    y = max(min(y, 2047), -ATI_CURSOR_HEIGHT + 1);

#ifdef CURSOR_BUG_R3
    if (x < 0) {
	leftBlank = 1-x;
	x = 0;
	outb(HORZ_CURSOR_OFFSET, leftBlank);
    }
    else if (leftBlank > 1) {
	leftBlank = 1;
	outb(HORZ_CURSOR_OFFSET, 1);
    }
#else
    if (x < 0) {
	leftBlank = -x;
	x = 0;
	outb(HORZ_CURSOR_OFFSET, leftBlank);
    }
    else if (leftBlank) {
	leftBlank = 0;
	outb(HORZ_CURSOR_OFFSET, 0);
    }
#endif

    if (y < 0) {
	topBlank = -y;
	y = 0;
	outb(VERT_CURSOR_OFFSET, topBlank);

	i = CursorOffset(cindex, topBlank) >> 2;
	outw(CURSOR_OFFSET_LO, i & 0xffff);
	outb(CURSOR_OFFSET_HI, (i >> 16) & 0x0f);
    }
    else if (topBlank) {
	topBlank = 0;

	i = CursorOffset(cindex, topBlank) >> 2;
	outw(CURSOR_OFFSET_LO, i & 0xffff);
	outb(CURSOR_OFFSET_HI, (i >> 16) & 0x0f);

	outb(VERT_CURSOR_OFFSET, 0);
    }

    outw(HORZ_CURSOR_POSN, x & 0x07ff);
    outw(VERT_CURSOR_POSN, y & 0x07ff);

    return(SI_SUCCEED);
}
