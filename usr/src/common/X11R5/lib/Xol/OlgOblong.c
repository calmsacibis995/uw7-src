#ifndef NOIDENT
#ident	"@(#)olg:OlgOblong.c	1.14"
#endif

/* Oblong Button Functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/Intrinsic.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>


void
OlgDrawOblongButton OLARGLIST((scr, win, pInfo, x, y, width, height, label, labelProc, flags))
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
  (*_olmOlgDrawOblongButton)(scr, win, pInfo, x, y, width, height, label,
			     labelProc, flags);
}
  
/* Size an oblong button.
 *
 * Determine the optimum size for an oblong button.  The button size is
 * calculated to "just fit" around the label.  The size of the label is
 * determined by the function sizeProc.  Flags describing the button
 * attributes are the same as for OlgDrawOblongButton.
 */

void
OlgSizeOblongButton OLARGLIST((scr, pInfo, label, sizeProc, flags, pWidth, pHeight))
    OLARG( Screen *,		scr)
    OLARG( OlgAttrs *,		pInfo)
    OLARG( XtPointer,		label)
    OLARG( OlgSizeButtonProc,	sizeProc)
    OLARG( OlBitMask,		flags)
    OLARG( Dimension *,		pWidth)
    OLGRA( Dimension *,		pHeight)
{
    unsigned	minHeight;
    register _OlgDevice	*pDev = pInfo->pDev;

    if (OlGetGui() == OL_MOTIF_GUI) {
      OlgSizeRectButton(scr,pInfo,label,sizeProc,flags,pWidth,pHeight);
      if (flags & OB_MENUMARK)
	{
	  if ((flags & OB_MENU_D) != OB_MENU_D)
	    {
	      if (pDev->rightMenuMWidth == 0)
		_OlgOBrightMMDimensions (pDev);
	      
	      *pWidth += pDev->rightMenuMWidth + pDev->menuMPad;
	      *pHeight = _OlMax ((int) (pDev->rightMenuMHeight),
			      (int) *pHeight);
	    }
	}
    }
    else {

    /* determine the size of label that can fit without stretching the
     * corners.
     */
    minHeight = pDev->oblongDefUL.height + pDev->oblongDefLR.height -
	pDev->lblOrigY - pDev->lblCornerY;

    /* get the actual label size to find how much the corners must be
     * separated.
     */
    (*sizeProc) (scr, pInfo, label, pWidth, pHeight);
    if (minHeight < *pHeight)
	*pHeight = *pHeight - minHeight;
    else
	*pHeight = 0;

    /* Add in the size of the menu mark */
    if (flags & OB_MENUMARK)
    {
	if ((flags & OB_MENU_D) == OB_MENU_D)
	{
	    if (pDev->downMenuMWidth == 0)
		_OlgOBdownMMDimensions (pDev);

	    *pWidth += pDev->downMenuMWidth + pDev->menuMPad;
	    minHeight += *pHeight;
	    *pHeight = _OlMax ((int) (pDev->downMenuMHeight - minHeight),
			    (int) *pHeight);
	}
	else
	{
	    if (pDev->rightMenuMWidth == 0)
		_OlgOBrightMMDimensions (pDev);

	    *pWidth += pDev->rightMenuMWidth + pDev->menuMPad;
	    minHeight += *pHeight;
	    *pHeight = _OlMax ((int) (pDev->rightMenuMHeight - minHeight),
			    (int) *pHeight);
	}
    }

    /* Add in the size of the corners */
    if (flags & OB_MENUITEM)
    {
	*pWidth += pDev->oblongDefUL.width + pDev->oblongDefUR.width;
	*pHeight += pDev->oblongDefUL.height + pDev->oblongDefLL.height;
    }
    else
	if (OlgIs3d())
	{
	    *pWidth += pDev->oblongB3UL.width + pDev->oblongB3UR.width;
	    *pHeight += pDev->oblongB3UL.height + pDev->oblongB3LL.height;
	}
	else
	{
	    *pWidth += pDev->oblongB2UL.width + pDev->oblongB2UR.width;
	    *pHeight += pDev->oblongB2UL.height + pDev->oblongB2LL.height;
	}
  }
}

/* Calculate the dimensions of a menu mark pointing down.  This should be
 * sensitive to the scale, but for the moment, this is correct for the
 * 12 point scale only.
 */

void
_OlgOBdownMMDimensions (pDev)
    _OlgDevice	*pDev;
{
    pDev->downMenuMWidth = OlScreenPointToPixel(OL_HORIZONTAL, 7, pDev->scr);
    pDev->downMenuMHeight = OlScreenPointToPixel(OL_VERTICAL, 7, pDev->scr);
    pDev->menuMPad = OlScreenPointToPixel(OL_HORIZONTAL, 7, pDev->scr);
}

/* Calculate the dimensions of a menu mark pointing right.  This should be
 * sensitive to the scale, but for the moment, this is correct for the
 * 12 point scale only.
 */

void
_OlgOBrightMMDimensions (pDev)
    _OlgDevice	*pDev;
{
    pDev->rightMenuMWidth = OlScreenPointToPixel(OL_HORIZONTAL, 7, pDev->scr);
    pDev->rightMenuMHeight = OlScreenPointToPixel(OL_VERTICAL, 7, pDev->scr);
    pDev->menuMPad = OlScreenPointToPixel(OL_HORIZONTAL, 7, pDev->scr);
}

/* Draw a menu mark in the box given.  The mark can be right or down;
 * centered or right justified.  The mark is always centered vertically.
 * The width of the mark is returned.  Valid flags:
 *
 *	MM_DOWN		draw down, else draw right
 *	MM_CENTER	center justify, else right justify
 *	MM_INVERT	draw with background color, else foreground (2-D only)
 */

Dimension
OlgDrawMenuMark OLARGLIST ((scr, win, pInfo, x, y, width, height, flags))
  OLARG( Screen *,	scr )
  OLARG( Drawable,	win )
  OLARG( OlgAttrs *,	pInfo )
  OLARG( Position,	x )
  OLARG( Position,	y )
  OLARG( Dimension,	width )
  OLARG( Dimension,	height )
  OLGRA( OlBitMask,	flags )
{
  return (*_olmOlgDrawMenuMark)(scr, win, pInfo, x, y, width, height, flags);
}
