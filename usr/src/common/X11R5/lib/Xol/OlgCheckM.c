#ifndef NOIDENT
#ident	"@(#)olg:OlgCheckM.c	1.9"
#endif

/* Check Box Functions */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>

/* Draw a check box.
 *
 * Draw a check box of the type indicated.  Valid types are:
 *
 *	CB_CHECKED
 *	CB_DIM
 *
 * The area under the box is assumed to be cleared to the background.
 */

static void OlgDrawDiamond OL_ARGS((Screen *, Drawable, OlgAttrs *,
				    Position, Position, unsigned));

void
_OlmOlgDrawCheckBox OLARGLIST((scr, win, pInfo, x, y, flags))
  OLARG(Screen *,	scr)
  OLARG(Drawable,	win)
  OLARG(OlgAttrs *,	pInfo)
  OLARG(Position,	x)
  OLARG(Position,	y)
  OLGRA(OlBitMask,	flags)
{
    _OlgDevice	*pDev = pInfo->pDev;
    Position	xBox, yBox;
    Dimension	hStroke, vStroke;
    GC		gc;

    hStroke = OlgGetHorizontalStroke(pInfo);
    vStroke = OlgGetVerticalStroke(pInfo);
    
    if (!pDev->checks)
    {
	_OlgGetBitmaps (scr, pInfo, OLG_CHECKS);
	_OlgCBsizeBox (pDev);

		/* size of Motif toggle box is 2 points smaller
		 * than O/L checkbox in 12 point scale. Reduce, scottn...
		 */
	pDev->widthCheckBox -= 2 * OlgGetHorizontalStroke (pInfo);
	pDev->heightCheckBox -= 2 * OlgGetVerticalStroke (pInfo);
    }

    /* The checks bitmap contains two subimages.  The left one contains the
     * actual check; the right one contains the mask where the check isn't.
     * Both are the same size.
     */
    /* If the check box is dim, we have to muck with the GCs to add a
     * stipple.
     */
    if (flags & CB_DIM)
    {
	if (OlgIs3d())
	{
	    gc = OlgGetBrightGC (pInfo);
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);

	    gc = OlgGetBg3GC (pInfo);
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	}
	else
	{
	    gc = OlgGetFgGC (pInfo);
	    XSetStipple (DisplayOfScreen (scr), gc, pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	}
    }

    /* Draw the box */
    yBox = y;

    if (OlgIs3d())
    {
      if (!(flags & CB_DIAMOND)) {
	OlgDrawBox(scr, win, pInfo, x, yBox, pDev->widthCheckBox,
		   pDev->heightCheckBox, flags & CB_CHECKED);
	OlgDrawBox(scr, win, pInfo, x+hStroke, yBox+vStroke,
		   pDev->widthCheckBox-(2*hStroke), 
		   pDev->heightCheckBox-(2*vStroke), flags & CB_CHECKED);
      }
    }
    else
    {
      if (!(flags & CB_DIAMOND))
	OlgDrawBox(scr, win, pInfo, x, yBox, pDev->widthCheckBox,
		   pDev->heightCheckBox, False);
    }

    /* Draw the check, if present */
    if (flags & CB_CHECKED)
    {
      gc = OlgGetBg2GC (pInfo);
    }
    else
    {
      gc = OlgGetBg1GC (pInfo);
    }
    {
      /* Fill in interior */
      

	if (!OlgIs3d() && !(flags & CB_DIAMOND))
	  /* The box is 2d.  Draw 2nd Border */
	  OlgDrawBox(scr, win, pInfo, x+hStroke, yBox+vStroke,
		     pDev->widthCheckBox-(2*hStroke), 
		     pDev->heightCheckBox-(2*vStroke), False);

	{
	  Dimension use_width = pDev->widthCheckBox;
	  Dimension use_height = pDev->heightCheckBox;
	  
	  if (flags & CB_DIAMOND) {

	    /* see comments about ugliness in OlgDrawDiamond below */
	    
	    if (!(use_width % 2))
	      use_width++;
	    if (!(use_height % 2))
	      use_height++;

	    if (use_height != use_width) {
	      if (use_height > use_width)
		use_height = use_width;
	      else use_width = use_height;
	    }
	  }

	  XFillRectangle (DisplayOfScreen (scr), win, gc,
			  x + (hStroke << 1), yBox + (vStroke << 1),
			  use_width - (hStroke << 2),
			  use_height - (vStroke << 2));
	}
	
	if (flags & CB_DIAMOND)
	  OlgDrawDiamond(scr, win, pInfo, x, yBox, flags & CB_CHECKED);

	/* Clean up any GCs that were polluted */
	if (flags & CB_DIM)
	    if (OlgIs3d())
	    {
		XSetFillStyle (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			       FillSolid);
		XSetFillStyle (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			       FillSolid);
	    }
	    else
		XSetFillStyle (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			       FillSolid);
    }
}

/*
 * OlgDrawDiamond -- Draws the 2 chevrons of the 3-poing diamond box.  It
 * does this by drawing 9 lines in a zig-zag pattern to cover all 3 points
 * of the "width".  XFillPolygon was considered, but since it does NOT
 * include all the points in the path, XDrawLines would need to be called
 * as well, so this solution was deemed no better.
 *
 * The following shows the order of points for each chevron:
 *
 *			2
 *			5
 *			8
 *			    9 4 3
 *		1 6 7
 * and bottom:
 *			    9 4 3
 *		1 6 7
 *			8
 *			5
 *			2
 */

static void
OlgDrawDiamond OLARGLIST((scr, win, pInfo, x, y, flags))
  OLARG(Screen *,	scr)
  OLARG(Drawable,	win)
  OLARG(OlgAttrs *,	pInfo)
  OLARG(Position,	x)
  OLARG(Position,	y)
  OLGRA(unsigned,	flags)
{
  _OlgDevice *pDev = pInfo->pDev;
  XPoint topChevron[9], bottomChevron[9];

  Dimension hStroke = OlgGetHorizontalStroke(pInfo);
  Dimension vStroke = OlgGetVerticalStroke(pInfo);
  
  Dimension hCB = pDev->heightCheckBox;
  Dimension wCB = pDev->widthCheckBox;

  /* diamond needs odd height and width or it will be ugly */

  if (!(hCB % 2))
    hCB++;
  if (!(wCB % 2))
    wCB++;

  /* diamond needs same height and width, or it may STILL be ugly.  The */
  /* lines drawn with height != width are not left-right symmetrical. */
  
  if (hCB != wCB) {
    if (hCB > wCB)
      hCB = wCB;
    else wCB = hCB;
  }

#define PTS	topChevron
  
#define out_l	PTS[0]
#define out_t	PTS[1]
#define out_r	PTS[2]
#define mid_r	PTS[3]
#define mid_t	PTS[4]
#define mid_l	PTS[5]
#define in_l	PTS[6]
#define in_t	PTS[7]
#define in_r	PTS[8]
  
  out_l.x = x - hStroke;
  out_l.y = y + hCB/2;
  out_t.x = x + wCB/2;
  out_t.y = y - vStroke;
  out_r.x = x + wCB - hStroke;
  out_r.y = out_l.y - vStroke;
  mid_r.x = out_r.x - hStroke;
  mid_r.y = out_r.y;
  mid_t.x = out_t.x;
  mid_t.y = out_t.y + vStroke;
  mid_l.x = out_l.x + hStroke;
  mid_l.y = out_l.y;
  in_l.x  = mid_l.x + hStroke;
  in_l.y  = mid_l.y;
  in_t.x  = mid_t.x;
  in_t.y  = mid_t.y + vStroke;
  in_r.x  = mid_r.x - hStroke;
  in_r.y  = mid_r.y;

  if (OlgIs3d())
    XDrawLines(DisplayOfScreen(scr), win, !flags ? OlgGetBrightGC(pInfo) :
	       OlgGetBg3GC(pInfo), topChevron, 9, CoordModeOrigin);
  else
    XDrawLines(DisplayOfScreen(scr), win, OlgGetBg2GC(pInfo),
	       topChevron, 9, CoordModeOrigin);

#undef PTS
#define PTS	bottomChevron
  
  out_l.x = x;
  out_l.y = y + hCB/2 + vStroke;
  out_t.x = x + wCB/2;
  out_t.y = y + hCB;
  out_r.x = x + wCB;
  out_r.y = out_l.y - vStroke;
  mid_r.x = out_r.x - hStroke;
  mid_r.y = out_r.y;
  mid_t.x = out_t.x;
  mid_t.y = out_t.y - vStroke;
  mid_l.x = out_l.x + hStroke;
  mid_l.y = out_l.y;
  in_l.x  = mid_l.x + hStroke;
  in_l.y  = mid_l.y;
  in_t.x  = mid_t.x;
  in_t.y  = mid_t.y - vStroke;
  in_r.x  = mid_r.x - hStroke;
  in_r.y  = mid_r.y;

  if (OlgIs3d())
    XDrawLines(DisplayOfScreen(scr), win, flags ? OlgGetBrightGC(pInfo) :
	       OlgGetBg3GC(pInfo), bottomChevron, 9, CoordModeOrigin);
  else 
    XDrawLines(DisplayOfScreen(scr), win, OlgGetBg2GC(pInfo),
	       bottomChevron, 9, CoordModeOrigin);
}
