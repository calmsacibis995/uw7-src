#ifndef NOIDENT
#ident	"@(#)olg:OlgSlider.c	1.12"
#endif

/* Slider functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>
#include <Xol/SliderP.h>


/* muldiv - Multiply two numbers and divide by a third.
 *
 * Calculate m1*m2/q where m1, m2, and q are integers.  Be careful of
 * overflow.
 */
#define muldiv(m1, m2, q)	((m2)/(q) * (m1) + (((m2)%(q))*(m1))/(q));


void
OlgDrawSlider OLARGLIST((w, pInfo))
    OLARG(Widget,	w)
    OLGRA(OlgAttrs *, pInfo)
{
	(*_olmOlgDrawSlider)(w, pInfo);
}

void
OlgUpdateSlider OLARGLIST((w, pInfo, flags))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLGRA( OlBitMask,	flags)
{
	(*_olmOlgUpdateSlider)(w, pInfo, flags);
}

/* Determine the size of a slider anchor.  This is only correct for
 * the 12 point scale.  The anchor contains two stroke widths of pad
 * in the direction of slider motion.
 */

void
OlgSizeSliderAnchor OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,		w)
    OLARG( OlgAttrs *,		pInfo)
    OLARG( Dimension *,		pWidth)
    OLGRA( Dimension *, pHeight)
{
    SliderWidget	sw = (SliderWidget)w;

    if (sw->slider.endBoxes)
    {
	if (sw->slider.orientation == OL_HORIZONTAL)
	{
	    *pWidth=OlScreenPointToPixel(OL_HORIZONTAL, 6, pInfo->pDev->scr) +
		2*pInfo->pDev->horizontalStroke;
	    *pHeight=OlScreenPointToPixel(OL_VERTICAL, 11, pInfo->pDev->scr);
	}
	else
	{
	    *pHeight=OlScreenPointToPixel(OL_VERTICAL, 6, pInfo->pDev->scr) +
		2*pInfo->pDev->verticalStroke;
	    *pWidth=OlScreenPointToPixel(OL_HORIZONTAL, 11, pInfo->pDev->scr);
	}
    }
    else
	*pWidth = *pHeight = 0;
}

/* Determine the size of a slider elevator.  This is only correct for
 * the 12 point scale.  The elevator is padded all around by one stroke width.
 */

void
OlgSizeSliderElevator OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *, pHeight)
{
	(*_olmOlgSizeSliderElevator)(w, pInfo, pWidth, pHeight);
}

void
OlgSizeSlider OLARGLIST((w, pInfo, pWidth, pHeight))
    OLARG( Widget,	w)
    OLARG( OlgAttrs *,	pInfo)
    OLARG( Dimension *,	pWidth)
    OLGRA( Dimension *, pHeight)
{
	(*_olmOlgSizeSlider)(w, pInfo, pWidth, pHeight);
}
