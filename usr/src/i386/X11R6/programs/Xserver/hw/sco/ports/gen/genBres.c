/*
 * @(#) genBres.c 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */
/*
 *
 * bresenham line drawer
 *
 */

#include "X.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "colormapst.h"

#include "mfb/mfb.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "genDefs.h"
#include "genProcs.h"

void
genSolidZeroSegs(
	GCPtr pGC,
	DrawablePtr pDrawable,
	BresLinePtr plines,
	int nlines)
{

	while (nlines--)
	    {
	    genSolidZeroSeg(pGC, pDrawable, plines->signdx, plines->signdy, 
			plines->axis, plines->x1, plines->y1, plines->e, 
			plines->e1, plines->e2, plines->len );
	    plines++;
	    }

}

/*
 * This is the most straight forward way to do this if you have no line
 * drawing hardware.  If your line performance is in trouble this is
 * worth coding in assembly.
 */

void
genSolidZeroSeg(
	GCPtr pGC,
	DrawablePtr pDraw,
	int signdx,
	int signdy,
	int axis,
	int x1,
	int y1,
	int e,
	int e1,
	int e2,
	int len )
{
	void (* DrawPoints)() = (void (*)())(NFB_WINDOW_PRIV(pDraw))->ops->DrawPoints;
	int alu, planemask, fg, count;
	DDXPointRec *pts, *p;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

        alu = pGCPriv->rRop.alu;
        planemask = pGCPriv->rRop.planemask;
        fg = pGCPriv->rRop.fg;
	pts = (DDXPointRec *)ALLOCATE_LOCAL(sizeof(DDXPointRec) * len);
	p = pts;
	count = len;

        while(count--)
        { 
	     p->x = x1;
	     p->y = y1;
	     ++p;
             if( e > 0 )
             {
                   x1 += signdx; /* make diagonal step */ 
		   y1 += signdy;
                   e += e2;
             }
             else
             {                                      /* make linear step   */
                   if( axis == X_AXIS ) 
			x1 += signdx;
                   else
			y1 += signdy;
                   e += e1;
             }
        }
	(*DrawPoints)(pts, len, fg, alu, planemask, pDraw);
	DEALLOCATE_LOCAL((char *)pts);
}


#ifdef TRYME

/*
 *  The following code jumps through all sort of hoops to cut the line into 
 *  a series of spans and draw them out this way.  This could be a win
 *  if the line is near horizontal or verticle, but it loses as the line 
 *  moves diagonal.  Your mileage may vary.
 */
void genVert(struct _Box *, unsigned int, unsigned char,
    unsigned int, void (*)(), struct _Drawable *);

void genHoriz(struct _Box *, unsigned int, unsigned char,
    unsigned int, void (*)(), struct _Drawable *);

void
genSolidZeroSeg(
	GCPtr pGC,
	DrawablePtr pDraw,
	int signdx,
	int signdy,
	int axis,
	int x1,
	int y1,
	int e,
	int e1,
	int e2,
	int len )
{
	int spanlen, spanlen1, supere1, supere2, tmplen;
	register void (* DrawSolidRects)() =
			(NFB_WINDOW_PRIV(pDraw))->ops->DrawSolidRects;
	int alu, planemask, fg;
	BoxRec box;
	nfbGCPrivPtr pGCPriv = NFB_GC_PRIV(pGC);

	alu = pGCPriv->rRop.alu;
	planemask = pGCPriv->rRop.planemask;
	fg = pGCPriv->rRop.fg;

	/* All this code is common for all the loops below */
	spanlen = ((-e2) / e1);

	/* if I had the remainder from the above operation
	 * I could compute supere1 with a subtract rather
	 * than the multiply used below
	 */
	supere1 = spanlen * e1;
	supere2 = supere1 + e2;
	spanlen++;

	tmplen = 1;
	while ( e < 0 )
	{
		e+=e1;
		tmplen++;
	}

	/*

	   There is a problem with lines which are dashed or clipped
	   (sometimes).  This code makes the assumption that at there is at
	   least a beginning and an ending span which are on different
	   lines. Here is a rough example:

Normal line:
*****
     *****
          *****

Dashed line:
**..*
     *..**
          ..**.

	   Since the line is dashed, genSolidZeroSeg is called for each of
	   the shorter segments. Only one of the four segments falls on
	   two different spans - the others all fall on the same span.
	   Similar problems occur with clipping. The fix is to detect when
	   the segment being drawn will only fit on one span and only draw
	   that one span.

	*/

	if (len <= tmplen) {
		if(axis == X_AXIS)
		{
			box.x1 = x1;
			box.x2 = x1 + len * signdx;
			box.y1 = y1;
			box.y2 = y1 + 1;
			genHoriz(&box, fg, alu, planemask, DrawSolidRects,
				pDraw);
		} else {
			box.x1 = x1;
			box.x2 = x1 + 1;
			box.y1 = y1;
			box.y2 = y1 + len * signdy;
			genVert(&box, fg, alu, planemask, DrawSolidRects,
				pDraw);
		}
		return;
	}						/* S001 End */


	len = len - tmplen;
	e = e + e2 + supere1;

	if(axis == X_AXIS)
	{
		if ( signdx >= 0 )
		{
			box.x1 = x1;
			box.x2 = x1 + tmplen;
			box.y1 = y1;
			box.y2 = y1 + 1;
			genHoriz(&box, fg, alu, planemask, DrawSolidRects,
				pDraw);
			y1 += signdy;
			x1 += tmplen;

			while(len > spanlen)
			{
				int negativee;

				negativee = (e < 0);

				spanlen1 = spanlen + negativee;

				box.x1 = x1;
				box.x2 = x1 + spanlen1;
				box.y1 = y1;
				box.y2 = y1 + 1;
				genHoriz(&box, fg, alu, planemask,
					DrawSolidRects, pDraw);
				x1 += spanlen1;
				len -= spanlen1;
				e += supere2 + ( e1 & (-negativee));

				y1+=signdy;
			}
			box.x1 = x1;
			box.x2 = x1 + len;
			box.y1 = y1;
			box.y2 = y1 + 1;

			genHoriz(&box, fg, alu, planemask, DrawSolidRects,
					 pDraw);
		}
		else
		{
			x1 = x1 - tmplen + 1;
			box.x1 = x1;
			box.x2 = x1 + tmplen;
			box.y1 = y1;
			box.y2 = y1 + 1;
			genHoriz(&box, fg, alu, planemask, DrawSolidRects,
					 pDraw);
			y1 += signdy;

			while(len > spanlen)
			{
				int negativee;

				negativee = (e < 0);

				spanlen1 = spanlen + negativee;

				x1 -= spanlen1;
				box.x1 = x1;
				box.x2 = x1 + spanlen1;
				box.y1 = y1;
				box.y2 = y1 + 1;
				genHoriz(&box, fg, alu, planemask,
						DrawSolidRects, pDraw);
				len -= spanlen1;
				e += supere2 + ( e1 & (-negativee));
				y1+=signdy;
			}
			box.x1 = x1 - len;
			box.x2 = x1;
			box.y1 = y1;
			box.y2 = y1 + 1;
			genHoriz(&box, fg, alu, planemask, DrawSolidRects,
					pDraw);
		}
		
	}
	else
	{
		if ( signdy >= 0 )
		{
			box.x1 = x1;
			box.x2 = x1 + 1;
			box.y1 = y1;
			box.y2 = y1 + tmplen;
			genVert(&box, fg, alu, planemask, DrawSolidRects,
					pDraw);
			x1 += signdx;
			y1 += tmplen;

			while(len > spanlen)
			{
				int negativee;

				negativee = (e < 0);

				spanlen1 = spanlen + negativee;

				box.x1 = x1;
				box.x2 = x1 + 1;
				box.y1 = y1;
				box.y2 = y1 + spanlen1;
				genVert(&box, fg, alu, planemask,
						DrawSolidRects, pDraw);
				y1 += spanlen1;
				len -= spanlen1;
				e += supere2 + ( e1 & (-negativee));

				x1+=signdx;
			}
			box.x1 = x1;
			box.x2 = x1 + 1;
			box.y1 = y1;
			box.y2 = y1 + len;
			genVert(&box, fg, alu, planemask, DrawSolidRects,
					pDraw);
		}
		else
		{
			y1 = y1 - tmplen + 1;
			box.x1 = x1;
			box.x2 = x1 + 1;
			box.y1 = y1;
			box.y2 = y1 + tmplen;
			genVert(&box, fg, alu, planemask, DrawSolidRects,
					pDraw);
			x1 += signdx;

			while(len > spanlen)
			{
				int negativee;

				negativee = (e < 0);

				spanlen1 = spanlen + negativee;

				y1 -= spanlen1;
				box.x1 = x1;
				box.x2 = x1 + 1;
				box.y1 = y1;
				box.y2 = y1 + spanlen1;
				genVert(&box, fg, alu, planemask,
					DrawSolidRects, pDraw);
				len -= spanlen1;
				e += supere2 + ( e1 & (-negativee));
				x1+=signdx;
			}
			box.x1 = x1;
			box.x2 = x1 + 1;
			box.y1 = y1 - len;
			box.y2 = y1;
			genVert(&box, fg, alu, planemask, DrawSolidRects,
					 pDraw);
		}
	}
	return;
}

void
genVert(
    register BoxPtr pbox,
    register unsigned int fg,
    register unsigned char alu,
    register unsigned int planemask,
    register void (* DrawSolidRects)(),
    register DrawablePtr pDraw)
{
    register int y1 = pbox->y1;
    register int y2 = pbox->y2;

    if (y1 == y2)
	return;
    if (y1 > y2) {
        /*
	 * Reverse y1 & y2, but now y1 is the
	 * point not drawn instead of y2.
	 */
	pbox->y1 = y2 + 1;
	pbox->y2 = y1 + 1;
    }
    (* DrawSolidRects)(pbox, 1, fg, alu, planemask, pDraw);
}

void
genHoriz(
    register BoxPtr pbox,
    register unsigned int fg,
    register unsigned char alu,
    register unsigned int planemask,
    register void (* DrawSolidRects)(),
    register DrawablePtr pDraw)
{
    register int x1 = pbox->x1;
    register int x2 = pbox->x2;

    if (x1 == x2)
	return;

    if (x1 > x2) {
        /*
	 * Reverse x1 & x2, but now x1 is the
	 * point not drawn instead of x2.
	 */
	pbox->x1 = x2 + 1;
	pbox->x2 = x1 + 1;
    }
    (* DrawSolidRects)(pbox, 1, fg, alu, planemask, pDraw);
}

#endif
