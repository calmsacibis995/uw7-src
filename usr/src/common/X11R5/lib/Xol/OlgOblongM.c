#ifndef NOIDENT
#ident	"@(#)olg:OlgOblongM.c	1.11"
#endif

#include <X11/Intrinsic.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>

/* Draw an oblong button.
 *
 * Draw an oblong button to fit in the box given by x, y, width, height.
 * The label for the button is drawn by the function labelProc using label
 * as data.  Button appearance is controlled by flags.  Valid flags:
 *
 *	OB_SELECTED		button is selected
 *	OB_DEFAULT		draw default ring
 *	OB_BUSY			stipple interior with 15% gray
 *	OB_INSENSITIVE		stipple everything with 50% gray (assumes
 *					the label takes care of itself)
 *	OB_MENUITEM		button is part of a menu
 *	OB_MENU_R		draw right-pointing menu mark
 *	OB_MENU_D		draw down-pointing menu mark
 *
 * This function makes lots of assumptions about the corners.  Aside from
 * the usual assumptions that the dimensions are all reasonable, it also
 * assumes that the default ring is completely within the outer ring.
 */

void
_OlmOlgDrawOblongButton OLARGLIST ((scr, win, pInfo, x, y, width, height,
			 label, labelProc, flags))
  OLARG( Screen	*,	scr )
  OLARG( Drawable,	win )
  OLARG( OlgAttrs *,	pInfo )
  OLARG( Position,	x )
  OLARG( Position,	y )
  OLARG( Dimension,	width )
  OLARG( Dimension,	height )
  OLARG( XtPointer,	label )
  OLARG( OlgLabelProc,	labelProc )
  OLGRA( OlBitMask,	flags )
{
    register _OlgDevice	*pDev;
    Position	dfltX, dfltY;		/* origin of default ring box */
    Dimension	dfltWidth, dfltHeight;	/* dimensions of default ring box */
    Position	xLbl, yLbl;		/* position of label box */
    Position	xShadow, yShadow;
    Dimension	wShadow, hShadow;
    Dimension	wLbl, hLbl;
    Position	xInner, yInner;
    Dimension	wInner, hInner;
    Dimension	hMargin = 2, vMargin = 1;

    unsigned	clip;			/* must clip flag */
    XRectangle	clipRect;		/* clip rectangle */
    int		diff;			/* difference in clipped values */
    
    int		tmp;			/* temporary value */
    int pts4 = _OlMax(OlScreenPointToPixel(OL_HORIZONTAL, 4, scr),
		   OlScreenPointToPixel(OL_VERTICAL, 4, scr));
    int pts2 = _OlMax(OlScreenPointToPixel(OL_HORIZONTAL, 2, scr),
		   OlScreenPointToPixel(OL_VERTICAL, 2, scr));
    int pts1 = _OlMax(OlScreenPointToPixel(OL_HORIZONTAL, 1, scr),
		   OlScreenPointToPixel(OL_VERTICAL, 1, scr));

    /* For a button with all the trimmings, from outside to inside, there */
    /* is the location cursor, the default ring, the shadow, the label. */
    /* Many of the calculations involve what may appear to be needless */
    /* multiplications, but they are not needless because for some */
    /* resolutions, pts4 != 2 * pts2. */

    pDev = pInfo->pDev;

    if ((flags & OB_MENUITEM)) {
      dfltX = dfltY =
      dfltWidth = dfltHeight = 0;
      xShadow = x; yShadow = y;
      wShadow = width; hShadow = height;
    }
    else if (flags & OB_DEFAULT) {
      dfltX = x; dfltY = y;
      dfltWidth = width; dfltHeight = height;
      xShadow = dfltX + pts4; yShadow = dfltY + pts4;
      wShadow = dfltWidth - (pts4 << 1); hShadow = dfltHeight -(pts4 << 1);
    }
    else {
      Dimension diff = pts4;

      dfltX = x + diff; dfltY = y + diff;
      dfltWidth = dfltHeight = 0;
      xShadow = dfltX; yShadow = dfltY;
      wShadow = width - (diff << 1);
      hShadow = height - (diff << 1);
    }

    xInner = xShadow + pDev->shadow_thickness;
    yInner = yShadow + pDev->shadow_thickness;
    wInner = wShadow - (pDev->shadow_thickness << 1);
    hInner = hShadow - (pDev->shadow_thickness << 1);

    if (wInner < pDev->horizontalStroke << 3 ||
	hInner < pDev->verticalStroke << 3)
    {
	clip = True;
	clipRect.x = xInner;
	clipRect.y = yInner;
	clipRect.width = wInner;
	clipRect.height = hInner;

	wInner = pDev->horizontalStroke << 3;
	hInner = pDev->verticalStroke << 3;
    }
    else
	clip = False;

    if (OlgIs3d())
    {
	      /* Add clip rectangle to GCs */
      if (clip)
	{
	  XSetClipRectangles (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			      0, 0, &clipRect, 1, YXBanded);
	  XSetClipRectangles (DisplayOfScreen (scr), OlgGetBg1GC (pInfo),
			      0, 0, &clipRect, 1, YXBanded);
	  XSetClipRectangles (DisplayOfScreen (scr), OlgGetBg2GC (pInfo),
			      0, 0, &clipRect, 1, YXBanded);
	  XSetClipRectangles (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			      0, 0, &clipRect, 1, YXBanded);
	}
      
    }

    /* If the button is insensitive, add a stipple to the GCs for
     * border drawing
     */
    if (flags & OB_INSENSITIVE)
      if (OlgIs3d())
	{
	  XSetStipple (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
		       pDev->inactiveStipple);
	  XSetFillStyle (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			 FillStippled);
	  XSetStipple (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
		       pDev->inactiveStipple);
	  XSetFillStyle (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			 FillStippled);
	}
      else
	{
	  XSetStipple (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
		       pDev->inactiveStipple);
	  XSetFillStyle (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			 FillStippled);
	}

    /* Draw the button background */
    
    XFillRectangle (DisplayOfScreen (scr), win, (flags & OB_SELECTED) ?
		    OlgGetBg2GC (pInfo) : OlgGetBg1GC (pInfo),
		    xInner, yInner, wInner, hInner);

    /* No shadow for button in a menu.  It is assumed that a menubutton */
    /* with a down arrow is in a menubar, and therefore "in a menu" */
    
    if ((flags & OB_MENUITEM))
    {
	if (OlgIs3d() && (flags & OB_SELECTED))
	{
	  /* Draw the highlighting to make it look raised */
	  OlgDrawBorderShadow(scr, win, pInfo, OL_SHADOW_OUT, 
			      pDev->shadow_thickness, xShadow, yShadow,
			      wShadow, hShadow);
	}
    }
    else
    {
      /* Draw the shadow */

      OlgDrawBorderShadow(scr, win, pInfo, (flags & OB_SELECTED) ?
			  OL_SHADOW_IN : OL_SHADOW_OUT,
			  pDev->shadow_thickness, xShadow, yShadow,
			  wShadow, hShadow);

      /* draw the default ring if necessary */
      
      if (flags & OB_DEFAULT)
	OlgDrawBorderShadow(scr, win, pInfo, OL_SHADOW_IN, pts1,
			    dfltX, dfltY, dfltWidth, dfltHeight);

    }

    /* Calculate the bounding box for the interior of the button. */
    
    xLbl = xInner + pDev->horizontalStroke * (2 + hMargin);
    yLbl = yInner + pDev->verticalStroke * (1 + vMargin);
    wLbl = wInner - 2 * (pDev->horizontalStroke * (2 + hMargin));
    hLbl = hInner - 2 * (pDev->verticalStroke * (1 + vMargin));
    
	    /* Draw menu mark, if any. */
    if ( (flags & OB_MENUMARK) &&
	 (!(flags & OB_MENUITEM) || (flags & OB_MENU_D) != OB_MENU_D) )
      {
	unsigned	menuFlags;
	
	menuFlags = None;
	if ((flags & OB_MENU_D) == OB_MENU_D)
	  menuFlags |= MM_DOWN;
	if (flags & OB_SELECTED)
	  menuFlags |= MM_INVERT;
	wLbl -= OlgDrawMenuMark (scr, win, pInfo, xLbl, yLbl,
				   wLbl, hLbl, menuFlags);
      }

	/* Add the label */
    if (wLbl > 0 && hLbl > 0)
	    (*labelProc) (scr, win, pInfo, xLbl, yLbl,
			  wLbl, hLbl, label);

    /* Overlay any stipples */
    if (flags & OB_BUSY)
    {
      XFillRectangle (DisplayOfScreen (scr), win, pInfo->pDev->busyGC,
		    xInner, yInner, wInner, hInner);
    }
	
    /* If we fooled with the GCs, restore them. */
    if (OlgIs3d()) {
      if (flags & OB_INSENSITIVE)
	{
	  XSetFillStyle (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			 FillSolid);
	  XSetFillStyle (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			 FillSolid);
	}
      if (clip)
	{
	  XSetClipMask (DisplayOfScreen (scr), OlgGetBrightGC (pInfo), None);
	  XSetClipMask (DisplayOfScreen (scr), OlgGetBg1GC (pInfo), None);
	  XSetClipMask (DisplayOfScreen (scr), OlgGetBg2GC (pInfo), None);
	  XSetClipMask (DisplayOfScreen (scr), OlgGetBg3GC (pInfo), None);
	}
    }
    else
      {
	if (flags & OB_INSENSITIVE)
	  XSetFillStyle (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			 FillSolid);
	/* if clipping, restore the GCs */
	if (clip)
	  {
	    XSetClipMask (DisplayOfScreen (scr), OlgGetBg1GC (pInfo), None);
	    XSetClipMask (DisplayOfScreen (scr), OlgGetFgGC (pInfo), None);
	  }
      }

}

Dimension
_OlmOlgDrawMenuMark OLARGLIST ((scr, win, pInfo, x, y, width, height, flags))
  OLARG( Screen *,	scr )
  OLARG( Drawable,	win )
  OLARG( OlgAttrs *,	pInfo )
  OLARG( Position,	x )
  OLARG( Position,	y )
  OLARG( Dimension,	width )
  OLARG( Dimension,	height )
  OLGRA( OlBitMask,	flags )
{
  int		xMark, yMark;
  XPoint	pts [8];
  GC		gc, whiteGC;
  char	clip = False;
  XRectangle	clipRect;
  register _OlgDevice	*pDev = pInfo->pDev;
  Dimension hStroke = OlgGetHorizontalStroke (pInfo);
  Dimension vStroke = OlgGetVerticalStroke (pInfo);
  
  if (flags & MM_JUST_SIZE) {
    if ((flags & MM_DOWN) && (pDev->downMenuMWidth == 0))
      _OlgOBdownMMDimensions (pDev);
    else if (!(flags & MM_DOWN) && (pDev->rightMenuMWidth == 0))
      _OlgOBrightMMDimensions (pDev);
    return ((flags & MM_DOWN) ? pDev->downMenuMWidth :
	    pDev->rightMenuMWidth) + pDev->menuMPad;
  }
  
  if (flags & MM_DOWN)
    {
      if (pDev->downMenuMWidth == 0)
	_OlgOBdownMMDimensions (pDev);
      
      if (flags & MM_CENTER)
	xMark = x + ((int) width - (int) pDev->downMenuMWidth) / 2;
      else
	xMark = x + width - pDev->downMenuMWidth;
      yMark = y + ((int) height - (int) pDev->downMenuMHeight) / 2;
      
      pts [0].x = xMark + (pDev->downMenuMWidth >> 1);
      pts [0].y = yMark + pDev->downMenuMHeight - 1;
      pts [1].x = xMark;
      pts [1].y = yMark;
      pts [2].x = xMark + pDev->downMenuMWidth - 1;
      pts [2].y = yMark;
      pts [3] = pts [0];
      
      /* Check if the menu mark must be clipped */
      if (xMark < x || yMark < y ||
	  (Position) (xMark + pDev->downMenuMWidth) > (Position) (x+width) ||
	  (Position) (yMark + pDev->downMenuMHeight) > (Position) (y+height))
	{
	  clip = True;
	}
    }
  /*    if (!(flags & MM_DOWN))*/
  else
    {
      if (pDev->rightMenuMWidth == 0)
	_OlgOBrightMMDimensions (pDev);
      
      if (flags & MM_CENTER)
	xMark = x + ((int) width - (int) pDev->rightMenuMWidth) / 2;
      else
	xMark = x + width - pDev->rightMenuMWidth;
      yMark = y + ((int) height - (int) pDev->rightMenuMHeight) / 2;
      
      pts [0].x = xMark;
      pts [0].y = yMark + pDev->rightMenuMHeight;
      pts [1].x = xMark;
      pts [1].y = yMark;
      pts [2].x = xMark + pDev->rightMenuMWidth;
      pts [2].y = yMark + (pDev->rightMenuMHeight >> 1);
      pts [3] = pts [0];
      
      pts [4].x = pts[0].x + hStroke;
      pts [4].y = pts[0].y - vStroke;
      pts [5].x = pts[1].x + hStroke;
      pts [5].y = pts[1].y + vStroke;
      pts [6].x = pts[2].x - hStroke;
      pts [6].y = pts[2].y;
      pts [7].x = pts[4].x - hStroke;
      pts [7].y = pts[4].y;
      
      /* Check if the menu mark must be clipped */
      if (xMark < x || yMark < y ||
	  (Position) (xMark+pDev->rightMenuMWidth) > (Position) (x+width) ||
	  (Position) (yMark+pDev->rightMenuMHeight) > (Position) (y+height))
	{
	  clip = True;
	}
    }
  
  /* Clip if we must */
  if (clip)
    {
      clipRect.x = x;
      clipRect.y = y;
      clipRect.width = width;
      clipRect.height = height;
    }
  
  /* Draw the wretched thing */
  if (OlgIs3d())
    {
      /* uninverted mark is out, not in as in OPEN LOOK */
      
      if (!(flags & MM_INVERT)) {
	whiteGC = OlgGetBg3GC (pInfo);
	gc = OlgGetBrightGC (pInfo);
      }
      else {
	whiteGC = OlgGetBrightGC (pInfo);
	gc = OlgGetBg3GC (pInfo);
      }
      
      if (clip)
	{
	  XSetClipRectangles (DisplayOfScreen (scr), whiteGC,
			      0, 0, &clipRect, 1, YXBanded);
	  
	  XSetClipRectangles (DisplayOfScreen (scr), gc, 0, 0,
			      &clipRect, 1, YXBanded);
	}
      
      {
	XDrawLines (DisplayOfScreen (scr), win, gc, pts, 3,
		    CoordModeOrigin);
	XDrawLines (DisplayOfScreen (scr), win, whiteGC, &pts [2], 2,
		    CoordModeOrigin);
	if (!(flags & MM_DOWN)) {
	  XDrawLines (DisplayOfScreen (scr), win, gc, &pts[4], 3,
		      CoordModeOrigin);
	  XDrawLines (DisplayOfScreen (scr), win, whiteGC, &pts[6], 2,
		      CoordModeOrigin);
	}
      }

      if (clip)
	{
	  XSetClipMask (DisplayOfScreen (scr), gc, None);
	  XSetClipMask (DisplayOfScreen (scr), whiteGC, None);
	}
    }
    else
    {
	gc = (flags & MM_INVERT) ? OlgGetBg1GC (pInfo) : OlgGetFgGC (pInfo);
	if (clip)
	    XSetClipRectangles (DisplayOfScreen (scr), gc, 0, 0,
				&clipRect, 1, YXBanded);

	XDrawLines (DisplayOfScreen (scr), win, gc, pts, 4, CoordModeOrigin);

	if (!(flags & MM_DOWN))
		XDrawLines (DisplayOfScreen (scr), win, gc, &pts [4], 4,
				CoordModeOrigin);

	if (clip)
	    XSetClipMask (DisplayOfScreen (scr), gc, None);
    }
    return ((flags & MM_DOWN) ? pDev->downMenuMWidth : pDev->rightMenuMWidth) +
	pDev->menuMPad;
}
