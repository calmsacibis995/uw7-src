#ifndef NOIDENT
#ident	"@(#)olg:OlgRect.c	1.10"
#endif

/* Rectangular Button Functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>

/* Draw a rectangular button.
 *
 * Draw a rectangular button to fit in the box given by x, y, width, height.
 * The label for the button is drawn by the function labelProc using label
 * as data.  Button appearance is controlled by flags.  Valid flags:
 *
 *	RB_SELECTED		button is selected
 *	RB_NOFRAME		don't draw box
 *	RB_DEFAULT		draw default ring
 *	RB_DIM			stipple borders with 50% gray
 *	RB_INSENSITIVE		stipple everything with 50% gray (this
 *					assumes that the label drawing
 *					functions will correctly stipple
 *					themselves.  With the current
 *					implementation, this is the same
 *					as RB_DIM)
 *
 */

void
_OlgDrawRectButton OLARGLIST((scr, win, pInfo, x, y, width, height, label, labelProc, flags, hMargin, vMargin))
    OLARG( Screen *,	scr)
    OLARG( Drawable,	win)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Position,	x)
    OLARG( Position,	y)
    OLARG( Dimension,	width)
    OLARG( Dimension,	height)
    OLARG( XtPointer,	label)
    OLARG( OlgLabelProc,labelProc)
    OLARG( OlBitMask,	flags)
    OLARG( Dimension,	hMargin)	/* number of strokes around label */
    OLGRA( Dimension,	vMargin)	/* number of strokes around label */
{
    register _OlgDevice	*pDev;
    Position	xLbl, yLbl;		/* position of label box */
    Position	xLblCorner, yLblCorner;	/* position of lower right of label */
    unsigned	clip;			/* must clip flag */
    XRectangle	clipRect;		/* clip rectangle */
    XRectangle	rects [4];		/* border rectangles */

    pDev = pInfo->pDev;

    /* If the button is too small, then clip */
    if (width < pDev->horizontalStroke*8 || height < pDev->verticalStroke*8)
    {
	clip = True;
	clipRect.x = x;
	clipRect.y = y;
	clipRect.width = width;
	clipRect.height = height;

        if (width < pDev->horizontalStroke*8)
		width = pDev->horizontalStroke * 8;
        if (height < pDev->verticalStroke*8)
		height = pDev->verticalStroke * 8;
    }
    else
	clip = False;

    /* Just about all aspects of draw 3-D buttons is different than 2-D
     * buttons.  Sigh.
     */
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

	/* Draw the button background. */
	XFillRectangle (DisplayOfScreen (scr), win, flags & RB_SELECTED ?
		OlgGetBg2GC (pInfo) : OlgGetBg1GC (pInfo),
		x, y, width, height);

	/* If the button is dim, add a stipple to the GCs for border drawing */
	if (flags & RB_DIM || flags & RB_INSENSITIVE)
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

	/* Draw border and default ring */
	if (!(flags & RB_NOFRAME))
	    OlgDrawBox (scr, win, pInfo, x, y, width, height,
			flags & RB_SELECTED);
	if (flags & RB_DEFAULT)
	{
	    int	hOffset = pDev->horizontalStroke * 2;
	    int vOffset = pDev->verticalStroke * 2;

	    rects [0].x = x + hOffset;
	    rects [0].y = y + vOffset;
	    rects [0].width = width - hOffset - hOffset;
	    rects [0].height = pDev->verticalStroke;

	    rects [1].x = rects [0].x;
	    rects [1].y = rects [0].y + pDev->verticalStroke;
	    rects [1].width = pDev->horizontalStroke;
	    rects [1].height = height - vOffset * 3;

	    rects [2].x = rects [1].x;
	    rects [2].y = rects [1].y + rects [1].height;
	    rects [2].width = rects [0].width;
	    rects [2].height = pDev->verticalStroke;

	    rects [3].x = rects [0].x + rects [0].width -
		pDev->horizontalStroke;
	    rects [3].y = rects [1].y;
	    rects [3].width = pDev->horizontalStroke;
	    rects [3].height = rects [1].height;

	    XFillRectangles (DisplayOfScreen (scr), win, OlgGetBg3GC (pInfo),
			     rects, 4);
	}

	/* If dim, restore the GCs */
	if (flags & RB_DIM || flags & RB_INSENSITIVE)
	{
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetBrightGC (pInfo),
			   FillSolid);
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetBg3GC (pInfo),
			   FillSolid);
	}

	/* if clipping, restore the GCs */
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
	int	borderWidth, borderHeight;

	/* Add clip rectangle to GCs */
	if (clip)
	{
	    XSetClipRectangles (DisplayOfScreen (scr), OlgGetBg1GC (pInfo),
				0, 0, &clipRect, 1, YXBanded);
	    XSetClipRectangles (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
				0, 0, &clipRect, 1, YXBanded);
	}

	/* Draw the button background.  For 2-D buttons the background is drawn
	 * within the default ring only (the window background is assumed to
	 * be drawn around the outside).
	 */
	/* Draw the button background. */
	XFillRectangle (DisplayOfScreen (scr), win, OlgGetBg1GC (pInfo),
		x + pDev->horizontalStroke * 3,
		y + pDev->verticalStroke * 3,
		width - pDev->horizontalStroke * 6,
		height - pDev->verticalStroke * 6);


	/* If the button is dim, add a stipple to the GCs for border drawing */
	if (flags & RB_DIM || flags & RB_INSENSITIVE)
	{
	    XSetStipple (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			 pDev->inactiveStipple);
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			   FillStippled);
	}

	/* Draw border, set ring, and default ring */
	if (flags & RB_NOFRAME)
	{
	    x += pDev->horizontalStroke;
	    y += pDev->verticalStroke;
	    width -= 2*pDev->horizontalStroke;
	    height -= 2*pDev->verticalStroke;
	    borderWidth = borderHeight = 0;
	}
	else
	{
	    borderWidth = pDev->horizontalStroke;
	    borderHeight = pDev->verticalStroke;
	}

	if (flags & RB_SELECTED)
	{
	    borderWidth += pDev->horizontalStroke;
	    borderHeight += pDev->verticalStroke;
	    if (flags & RB_DEFAULT)
	    {
		borderWidth += pDev->horizontalStroke;
		borderHeight += pDev->verticalStroke;
	    }
	}

	rects [0].x = x;
	rects [0].y = y;
	rects [0].width = width;
	rects [0].height = borderHeight;

	rects [1].x = x;
	rects [1].y = y + borderHeight;
	rects [1].width = borderWidth;
	rects [1].height = height - borderHeight - borderHeight;

	rects [2].x = x;
	rects [2].y = rects [1].y + rects [1].height;
	rects [2].width = width;
	rects [2].height = borderHeight;

	rects [3].x = rects [0].x + rects [0].width - borderWidth;
	rects [3].y = rects [1].y;
	rects [3].width = borderWidth;
	rects [3].height = rects [1].height;

	if (!(flags & RB_NOFRAME) || flags & RB_SELECTED)
	XFillRectangles (DisplayOfScreen (scr), win, OlgGetFgGC (pInfo),
			 rects, 4);

	if (flags & RB_NOFRAME)
	{
	    x -= pDev->horizontalStroke;
	    y -= pDev->verticalStroke;
	    width += 2*pDev->horizontalStroke;
	    height += 2*pDev->verticalStroke;
	}

	if ((flags & (RB_SELECTED | RB_DEFAULT)) == RB_DEFAULT)
	{
	    int	hOffset = pDev->horizontalStroke * 2;
	    int vOffset = pDev->verticalStroke * 2;

	    rects [0].x = x + hOffset;
	    rects [0].y = y + vOffset;
	    rects [0].width = width - hOffset - hOffset;
	    rects [0].height = pDev->verticalStroke;

	    rects [1].x = rects [0].x;
	    rects [1].y = rects [0].y + pDev->verticalStroke;
	    rects [1].width = pDev->horizontalStroke;
	    rects [1].height = height - vOffset * 3;

	    rects [2].x = rects [1].x;
	    rects [2].y = rects [1].y + rects [1].height;
	    rects [2].width = rects [0].width;
	    rects [2].height = pDev->verticalStroke;

	    rects [3].x = rects [0].x + rects [0].width -
		pDev->horizontalStroke;
	    rects [3].y = rects [1].y;
	    rects [3].width = pDev->horizontalStroke;
	    rects [3].height = rects [1].height;

	    XFillRectangles (DisplayOfScreen (scr), win, OlgGetFgGC (pInfo),
			     rects, 4);
	}

	/* If dim, restore the GCs */
	if (flags & RB_DIM || flags & RB_INSENSITIVE)
	{
	    XSetFillStyle (DisplayOfScreen (scr), OlgGetFgGC (pInfo),
			   FillSolid);
	}

	/* if clipping, restore the GCs */
	if (clip)
	{
	    XSetClipMask (DisplayOfScreen (scr), OlgGetBg1GC (pInfo), None);
	    XSetClipMask (DisplayOfScreen (scr), OlgGetFgGC (pInfo), None);
	}
    }

    /* Calculate the bounding box for the label area. */
    xLbl = x + pDev->horizontalStroke * (3 + hMargin);
    yLbl = y + pDev->verticalStroke * (3 + vMargin);
    xLblCorner = x + width - pDev->horizontalStroke * (3 + hMargin);
    yLblCorner = y + height - pDev->verticalStroke * (3 + vMargin);

    /* Draw the label if any the label area is not completely clipped out */
    if (xLbl < xLblCorner && yLbl < yLblCorner)
    {
	(*labelProc) (scr, win, pInfo, xLbl, yLbl,
		      xLblCorner - xLbl, yLblCorner - yLbl, label);
    }
}

/* OlgDrawRectButton -- Same as above, but with fixed margins.  The margins
 * are selected to give correct visual for text labels.
 */

void
OlgDrawRectButton OLARGLIST((scr, win, pInfo, x, y, width, height, label, labelProc, flags))
    OLARG( Screen *,		scr)
    OLARG( Drawable,		win)
    OLARG( OlgAttrs *,		pInfo)
    OLARG( Position,		x)
    OLARG( Position,		y)
    OLARG( Dimension,		width)
    OLARG( Dimension,		height)
    OLARG( XtPointer,		label)
    OLARG( OlgLabelProc,	labelProc)
    OLGRA( OlBitMask,		flags)
{
  _OlgDrawRectButton (scr, win, pInfo, x, y, width, height, label,
		      labelProc, flags, 3, 1);
}

/* Size a rectangular button.
 *
 * Determine the optimum size for a rectangular button.  The button size is
 * calculated to "just fit" around the label.  The size of the label is
 * determined by the function sizeProc.  Flags describing the button
 * attributes are the same as for OlgDrawRectButton.
 */

void
OlgSizeRectButton OLARGLIST((scr, pInfo, label, sizeProc, flags, pWidth, pHeight))
    OLARG( Screen *,		scr)
    OLARG( OlgAttrs *,		pInfo)
    OLARG( XtPointer,		label)
    OLARG( OlgSizeButtonProc,	sizeProc)
    OLARG( OlBitMask,		flags)
    OLARG( Dimension *,		pWidth)
    OLGRA( Dimension *,		pHeight)
{
    int		minHeight;
    register _OlgDevice	*pDev = pInfo->pDev;
    OlDefine	current_gui = OlGetGui();

    /* get the actual label size. */
    (*sizeProc) (scr, pInfo, label, pWidth, pHeight);

    /* Add in the size of the borders */
    *pWidth += pDev->horizontalStroke * ((current_gui == OL_OPENLOOK_GUI) ?
					 12 : 8); 
    *pHeight += pDev->verticalStroke * ((current_gui == OL_OPENLOOK_GUI) ?
					8 : 4); 
}
