#ident	"@(#)ihvkit:display/lfb256/devices/jaws/jawsCurs.c	1.1"

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

#include <macros.h>
#include <jaws.h>

static int cur_curs = -1;
static int curs_on = SI_FALSE;
static int curs_x, curs_y;


/* 
 * Room for JAWS_NUM_CURSORS.  Each cursor is JAWS_CURSOR_HEIGHT high
 * and JAWS_CURSOR_WIDTH wide.  The cursor is stored in an array of
 * SIint16 because the cursor hardware only uses 16 bits of every 
 * register.  We multiply the width by 2 because the cursor is 2 planes.
 *
 */

static SIint16 cursors
    [JAWS_NUM_CURSORS]
    [JAWS_CURSOR_HEIGHT]
    [(JAWS_CURSOR_WIDTH*2)/16];

/* 
 * Macros for going from a pair of bitmaps to a 2 plane bitmap.  p is
 * a pointer to a line in the cursor; b is the bit number to test/set.
 * TEST returns non-0 if bit b is set.  SET sets bit b.
 * 
 * Notes:
 * 
 *   - These macros are very specific to the setup we have.  TEST 
 *     expects p to point to a SIint32.  SET expects p to point to an 
 *     SIint16.
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

/* 
 * Like memmove(), but copies from an array of SIint16 to
 * an array of SIint32.
 */

SIvoid cursCopy(n)

{
    int i;
    register SIint32 *d = jaws_curs;
    register SIint16 *s = &cursors[n][0][0];

    for (i = 0; i < sizeof(cursors[0])/sizeof(SIint16); i++)
	*d++ = *s++;
}

/*	CURSOR CONTROL ROUTINES		*/
/*		MANDATORY		*/

/*
 *
 * The G332 has a three color cursor.  X11 only needs two.  We will
 * use the following combinations:
 *
 *   00  transparent
 *   01  unused
 *   10  bg
 *   11  fg
 *
 * This allows us to set one bit (10) based on the mask and the other
 * bit (01) based on the src.  We will ignore the invsrc that the CORE
 * gives us.
 *
 * The G332 cursor also has its own palette.  Due to the poor
 * definition in the SI spec, we will read the fg and bg values out of
 * the regular palette.
 *
 */

SIBool jawsDownLoadCurs(cindex, cp)
SIint32 cindex;
SICursorP cp;
{
    SIint32 *curs_cmap;
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
		"Jaws cursor DL: bad bitmap depth (src=%d msk=%d)\n",
		s->BbitsPerPixel, m->BbitsPerPixel);
	return(SI_FAIL);
    }

    if ((cp->SCwidth != s->Bwidth) ||
	(cp->SCwidth != m->Bwidth)) {

	fprintf(stderr,
		"Jaws cursor DL: bitmap width mismatch (%d src=%d msk=%d)\n",
		cp->SCwidth, s->Bwidth, m->Bwidth);
	return(SI_FAIL);
    }

    if ((cp->SCheight != s->Bheight) ||
	(cp->SCheight != m->Bheight)) {

	fprintf(stderr,
		"Jaws cursor DL: bitmap height mismatch (%d src=%d msk=%d)\n",
		cp->SCheight, s->Bheight, m->Bheight);
	return(SI_FAIL);
    }

    if ((cp->SCwidth  > JAWS_CURSOR_WIDTH) ||
	(cp->SCheight > JAWS_CURSOR_HEIGHT)) {
	fprintf(stderr,
		"Jaws cursor DL: Cursor size too large (%dx%d)\n",
		cp->SCwidth, cp->SCheight);
	return(SI_FAIL);
    }
	       
    memset(cursors[cindex], 0, sizeof(cursors[0]));

    stride = (s->Bwidth + 31) / 32;

    for (i = 0; i < cp->SCheight; i++) {
	ps = s->Bptr + stride * i;
	pm = m->Bptr + stride * i;
	p = cursors[cindex][i];

	/* 
	 * Test bit j of the current line in both the mask and the
	 * bitmap.  In the cursor, set bit 2j+1 for the mask and 2j
	 * for the bitmap.  Just in case the CORE doesn't mask the
	 * bitmap, this code will only check the bitmap if the mask
	 * was set.
	 * 
	 */

	for (j = 0; j < cp->SCwidth; j++) {
	    if (TEST(pm, j)) {
		SET(p, j+j + 1);

		if (TEST(ps, j)) {
		    SET(p, j+j);
		}
	    }
	}
    }

    curs_cmap = jaws_regs + JAWS_CURSOR_PALETTE;

    *curs_cmap++ = 0;
    *curs_cmap++ = jaws_cmap[cp->SCbg];
    *curs_cmap   = jaws_cmap[cp->SCfg];

    if (cur_curs == cindex)
	cursCopy(cindex);

    return(SI_SUCCEED);
}

SIBool jawsTurnOnCurs(cindex)
SIint32 cindex;
{
    register SIint32 *p;

    if (cindex != cur_curs) {
	cursCopy(cindex);
	cur_curs = cindex;
    }

    if (curs_on != SI_TRUE) {
	p = jaws_regs + JAWS_REG_CONTROL_A;
	*p &= 0x7fffff;
	curs_on = SI_TRUE;
    }

    return(SI_SUCCEED);
}

SIBool jawsTurnOffCurs(cindex)
SIint32 cindex;
{
    register SIint32 *p;

    if (curs_on == SI_TRUE) {
	p = jaws_regs + JAWS_REG_CONTROL_A;
	*p |= 0x800000;
	curs_on = SI_FALSE;
    }

    return(SI_SUCCEED);
}

SIBool jawsMoveCurs(cindex, x, y)
SIint32 cindex;
SIint32 x, y;
{
    register SIint32 *p;

    if ((curs_on == SI_TRUE) &&
	(cur_curs != cindex))
	jawsTurnOnCurs(cindex);

    /* 
     * The G332 cursor uses the location of the upper left corner of 
     * the cursor as the cursor position.  The cursor can be moved off 
     * screen, but only so that it does not show.  In other words, -64 
     * is the farthest that it can move up and left, and to the right 
     * it can only move to the screen width.  It is allowed to move 
     * down to the max of 2^11-1.
     * 
     */

    /* 
     * Note: Since the Jaws board actually has 2M of VRAM, we can 
     * create a virtual root window that is taller than the screen.  
     * For example, we could have a 1024x1024 mode.  All we need is to 
     * have this routine modify JAWS_REG_TOS2 to move the visible 
     * portion of the screen up and down.  We will also need to keep 
     * track of how much we adjusted the screen.
     * 
     * We should probably key off something in the configuration to 
     * let us know if the user wants this feature or not.
     * 
     */

    curs_x = max(-64, min(x, jaws_regP->width+1));
    curs_y = max(-64, min(y, 2047));

    p = jaws_regs + JAWS_CURSOR_POSITION;
    *p = ((curs_x & 0xfff) << 12) | (curs_y & 0xfff);

    return(SI_SUCCEED);
}
