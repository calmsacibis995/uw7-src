#ifndef NOIDENT
#ident	"@(#)olg:OlgOblongO.c	1.8"
#endif

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
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
_OloOlgDrawOblongButton OLARGLIST ((scr, win, pInfo, x, y, width, height,
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
    Position	xCorner, yCorner;	/* coordinates of lower right of box */
    Position	dfltX, dfltY;		/* origin of default ring box */
    Dimension	dfltWidth, dfltHeight;	/* dimensions of default ring box */
    _OlgDesc	*ul, *ur, *ll, *lr;	/* arcs for whatever we're drawing */
    Position	xOrigLR, yOrigLR;	/* position of upper left point in
					 *   lower right arc of default ring
					 */
    Position	xLbl, yLbl;		/* position of label box */
    Position	xLblCorner, yLblCorner;	/* position of lower right of label */
    int		tmp;			/* temporary value */

    pDev = pInfo->pDev;
    xCorner = x + width;
    yCorner = y + height;

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
    if (flags & OB_MENUITEM)
    {
	/* there is no outer ring; the default ring is used instead */
	OlgDrawFilledRBox (scr, win, pInfo, x, y, width, height,
			   &pDev->oblongDefUL, &pDev->oblongDefUR,
			   &pDev->oblongDefLL, &pDev->oblongDefLR,
			   (flags & OB_SELECTED) ? None : FB_UP);

	if (OlgIs3d() && (flags & OB_SELECTED))
	{
	    /* Draw the highlighting to make it look pressed */
	    OlgDrawRBox (scr, win, pInfo, x, y, width, height,
			 &pDev->oblongDefUL, &pDev->oblongDefUR,
			 &pDev->oblongDefLL, &pDev->oblongDefLR,
			 None);
	}
	else
	    if (flags & OB_DEFAULT)
	    {
		/* Draw default ring */
		OlgDrawRBox (scr, win, pInfo, x, y, width, height,
			     &pDev->oblongDefUL, &pDev->oblongDefUR,
			     &pDev->oblongDefLL, &pDev->oblongDefLR,
			     RB_MONO);
	    }

	ul = &pDev->oblongDefUL;
	ur = &pDev->oblongDefUR;
	ll = &pDev->oblongDefLL;
	lr = &pDev->oblongDefLR;

	dfltX = x;
	dfltY = y;
	dfltWidth = width;
	dfltHeight = height;

	/* Determine bounding box of default ring */
	if (width < (Dimension) (ul->width + ur->width))
	    xOrigLR = ul->width;
	else
	    xOrigLR = width - ur->width;

	if (height < (Dimension) (ul->height + ll->height))
	    yOrigLR = ul->height;
	else
	    yOrigLR = height - ll->height;
    }
    else
    {
	/* Get the proper corners */
	if (OlgIs3d())
	{
	    ul = &pDev->oblongB3UL;
	    ur = &pDev->oblongB3UR;
	    ll = &pDev->oblongB3LL;
	    lr = &pDev->oblongB3LR;
	}
	else
	{
	    ul = &pDev->oblongB2UL;
	    ur = &pDev->oblongB2UR;
	    ll = &pDev->oblongB2LL;
	    lr = &pDev->oblongB2LR;
	}

	OlgDrawFilledRBox (scr, win, pInfo, x, y, width, height,
			   ul, ur, ll, lr,
			   (OlgIs3d() && (flags&OB_SELECTED)) ? None : FB_UP);

	/* Draw the outer ring */
	OlgDrawRBox (scr, win, pInfo, x, y, width, height,
		     ul, ur, ll, lr,
		     (flags & OB_SELECTED) ? None : FB_UP);

	/* Determine bounding box of default ring */
	dfltX = ul->width - pDev->oblongDefUL.width;
	dfltY = ul->height - pDev->oblongDefUL.height;
	if (dfltX >= (Position) width || dfltY >= (Position) height)
	{
	    /* The interior is entirely clipped out */
	    return;
	}

	if (width < (Dimension) (ul->width + ur->width))
	{
	    xOrigLR = ul->width;
	    tmp = xOrigLR + pDev->oblongDefUR.width;
	    if (tmp > (Position) width)
	    {
		/* must clip default ring */
		tmp = width;
	    }
	    dfltWidth = tmp - dfltX;
	}
	else
	{
	    xOrigLR = width - ur->width;
	    dfltWidth = xOrigLR + pDev->oblongDefUR.width - dfltX;
	}

	if (height < (Dimension) (ul->height + ll->height))
	{
	    yOrigLR = ul->height;
	    tmp = yOrigLR + pDev->oblongDefLL.height;
	    if (tmp > (Position) height)
	    {
		/* must clip default ring */
		tmp = height;
	    }
	    dfltHeight = tmp - dfltY;
	}
	else
	{
	    yOrigLR = height - ll->height;
	    dfltHeight = yOrigLR + pDev->oblongDefLL.height - dfltY;
	}

	dfltX += x;
	dfltY += y;

	ul = &pDev->oblongDefUL;
	ur = &pDev->oblongDefUR;
	ll = &pDev->oblongDefLL;
	lr = &pDev->oblongDefLR;

	if (OlgIs3d())
	{
	    /* draw the default ring if needed */
	    if (flags & OB_DEFAULT)
		OlgDrawRBox (scr, win, pInfo,
			     dfltX, dfltY, dfltWidth, dfltHeight,
			     ul, ur, ll, lr,
			     RB_MONO);
	}
	else
	{
	    /* For 2-D buttons, fill the interior if it is selected and the
	     * default ring if it needs it.
	     */
	    if (flags & OB_SELECTED)
		OlgDrawFilledRBox (scr, win, pInfo,
				   dfltX, dfltY, dfltWidth, dfltHeight,
				   ul, ur, ll, lr,
				   None);
	    else
		if (flags & OB_DEFAULT)
		    OlgDrawRBox (scr, win, pInfo,
				 dfltX, dfltY, dfltWidth, dfltHeight,
				 ul, ur, ll, lr,
				 None);
	}
    }

    /* Calculate the bounding box for the interior of the button.  Vertically,
     * this box is inset 1 point from the default ring.  Horizontally, it
     * is completely within the caps of the default ring.
     */
    xLbl = dfltX + ul->width;
    xLblCorner = dfltX + xOrigLR;

    yLbl = dfltY + pDev->lblOrigY;
    yLblCorner = dfltY + dfltHeight - pDev->lblCornerY;

    /* clip the label area to the button bounding box */
    if (xLbl < xCorner && yLbl < yCorner)
    {
	if (xLblCorner > xCorner)
	    xLblCorner = xCorner;

	if (yLblCorner > yCorner)
	    yLblCorner = yCorner;

	/* Draw menu mark, if any. */
	if (flags & OB_MENUMARK)
	{
	    unsigned	menuFlags;

	    menuFlags = None;
	    if ((flags & OB_MENU_D) == OB_MENU_D)
		menuFlags |= MM_DOWN;
	    if (flags & OB_SELECTED)
		menuFlags |= MM_INVERT;
	    xLblCorner -= OlgDrawMenuMark (scr, win, pInfo, xLbl, yLbl,
					   xLblCorner-xLbl, yLblCorner-yLbl,
					   menuFlags);
	}

	/* Add the label */
	if (labelProc) { 
		if (xLbl < xLblCorner)
	    		(*labelProc) (scr, win, pInfo, xLbl, yLbl,
			  	xLblCorner - xLbl, yLblCorner - yLbl, label);
	}
    }

    /* If we fooled with the GCs, restore them. */
    if (flags & OB_INSENSITIVE)
	if (OlgIs3d())
	{
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			   FillSolid);
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			   FillSolid);
	}
	else
	{
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			   FillSolid);
	}

    /* Overlay any stipples */
    if (flags & OB_BUSY)
    {
	OlgDrawFilledRBox (scr, win, pInfo,
		     dfltX, dfltY, dfltWidth, dfltHeight,
		     ul, ur, ll, lr,
		     FB_BUSY);
    }
}

Dimension
_OloOlgDrawMenuMark OLARGLIST ((scr, win, pInfo, x, y, width, height, flags))
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
	pts [0].y = yMark + pDev->rightMenuMHeight - 1;
	pts [1].x = xMark;
	pts [1].y = yMark;
	pts [2].x = xMark + pDev->rightMenuMWidth - 1;
	pts [2].y = yMark + (pDev->rightMenuMHeight >> 1);
	pts [3] = pts [0];

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
      whiteGC = OlgGetBrightGC (pInfo);
      gc = OlgGetBg3GC (pInfo);

      if (clip)
	{
	  XSetClipRectangles (DisplayOfScreen (scr), whiteGC,
			      0, 0, &clipRect, 1, YXBanded);
	  
	  XSetClipRectangles (DisplayOfScreen (scr), gc, 0, 0,
			      &clipRect, 1, YXBanded);
	}
      
      XDrawLines (DisplayOfScreen (scr), win, gc, pts, 3,
		  CoordModeOrigin);
      XDrawLines (DisplayOfScreen (scr), win, whiteGC, &pts [2], 2,
		  CoordModeOrigin);
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

	XDrawLines (DisplayOfScreen (scr), win, gc, pts, 4,
		    CoordModeOrigin);

	if (clip)
	    XSetClipMask (DisplayOfScreen (scr), gc, None);
    }
    return ((flags & MM_DOWN) ? pDev->downMenuMWidth : pDev->rightMenuMWidth) +
	pDev->menuMPad;
}
