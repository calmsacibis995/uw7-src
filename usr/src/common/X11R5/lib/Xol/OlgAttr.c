#ifndef NOIDENT
#ident	"@(#)olg:OlgAttr.c	1.19"
#endif

/* Attribute Functions */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/Converters.h>
#include <Xol/OlgP.h>

/* somewhat arbitrary threshold values for determining if a color is close
 * to black or white.
 */
#define DARK_THRESHOLD	13107
#define LIGHT_THRESHOLD	58981

#define ColorRich(SCREEN) \
	(   DefaultVisualOfScreen(SCREEN)->class != StaticColor		\
	 || DefaultVisualOfScreen(SCREEN)->map_entries > 16  )

#define LUMINANCE(c)	((unsigned long) ((c).red * 19595 + \
					  (c).green * 38470 + \
					  (c).blue * 7471) >> 16)

static OlgAttrs	*attrHead;
static Boolean	prohibit3D = False;

static OlColorTuple *
LookupColorTuple (screen, bg1)
	Screen *		screen;
	Pixel			bg1;
{
	OlColorTupleList *	list;

	Boolean			use;

	Cardinal		i;


	OlGetColorTupleList ((Widget)0, &list, &use);
	if (!list)
		return (0);
	switch (use) {
	case True:
		break;
	case False:
		return (0);
	case XtUnspecifiedBoolean:
		if (ColorRich(screen))
			return (0);
		else
			break;
	}

	for (i = 0; i < list->size; i++)
		if (bg1 == list->list[i].bg1)
			return (&list->list[i]);

	return (0);
}

/* Allocate GCs for attributes structure */

static void
allocGCs (scr, pInfo)
    Screen	*scr;
    OlgAttrs	*pInfo;
{
    XGCValues	values;
    Display	*display = DisplayOfScreen(scr);
    Window	root = RootWindowOfScreen(scr);
    Colormap	cmap = DefaultColormapOfScreen(scr);

    pInfo->bg2 = pInfo->bg3 = pInfo->bg0 = (GC) 0;

    /* Allocate the GCs */
    if (pInfo->flags & OLG_BGPIXMAP)
    {
	values.tile = pInfo->bg.pixmap;
	values.fill_style = FillTiled;
	pInfo->bg1 = pInfo->bg2 = XCreateGC (DisplayOfScreen (scr),
						 RootWindowOfScreen (scr),
						 GCFillStyle|GCTile,
						 &values);
	if (!pInfo->bg1)
		OlVaDisplayErrorMsg(	(Display *) NULL,
					OleNfileOlgAttr,
					OleTmsg1,
					OleCOlToolkitError,
					OleMfileOlgAttr_msg1);

	pInfo->flags |= OLG_ALLOCBG1;
    }
    else
    {
	if (OlgIs3d())
	{
	    OlColorTuple	*ct;

	    if ((ct = LookupColorTuple(scr, pInfo->bg.pixel))) {
		values.foreground = ct->bg2;
		pInfo->bg2 = XCreateGC (display, root, GCForeground, &values);
		if (pInfo->bg2)
			pInfo->flags |= OLG_ALLOCBG2;
		values.foreground = ct->bg3;
		pInfo->bg3 = XCreateGC (display, root, GCForeground, &values);
		if (pInfo->bg3)
			pInfo->flags |= OLG_ALLOCBG3;

		values.foreground = ct->bg0;
		pInfo->bg0 = XCreateGC (display, root, GCForeground, &values);
		if (pInfo->bg0)
		    pInfo->flags |= OLG_ALLOCBG0;
	    }
	    else
	    {
		XColor		bg0, bg2, bg3;

		OlgGetColors (scr, pInfo->bg.pixel, &bg0, &bg2, &bg3);

		if (XAllocColor (display, cmap, &bg2))
		{
		    values.foreground = bg2.pixel;
		    pInfo->bg2 = XCreateGC (display, root, GCForeground,
					    &values);
		    if (pInfo->bg2)
			pInfo->flags |= OLG_ALLOCBG2;
		}

		if (XAllocColor (display, cmap, &bg3))
		{
		    values.foreground = bg3.pixel;
		    pInfo->bg3 = XCreateGC (display, root, GCForeground,
					    &values);
		    if (pInfo->bg3)
			pInfo->flags |= OLG_ALLOCBG3;
		}

		if (XAllocColor (display, cmap, &bg0))
		{
		    values.foreground = bg0.pixel;
		    pInfo->bg0 = XCreateGC (display, root, GCForeground,
					    &values);
		    if (pInfo->bg0)
			pInfo->flags |= OLG_ALLOCBG0;
		}
	    }
	}
	else	/* 2-D */
	{
	    /* Get the foreground GC */
	    /* if colors clash, pick a new one */
	    if (pInfo->fg == pInfo->bg.pixel)
		if (pInfo->bg.pixel == BlackPixelOfScreen (scr))
		    values.foreground = WhitePixelOfScreen(scr);
	        else
		    values.foreground = BlackPixelOfScreen(scr);
	    else
		values.foreground = pInfo->fg;
	    if (pInfo->bg2 = XCreateGC (display, root, GCForeground, &values))
		pInfo->flags |= OLG_ALLOCBG2;
	    else
		OlVaDisplayErrorMsg(	(Display *) NULL,
					OleNfileOlgAttr,
					OleTmsg2,
					OleCOlToolkitError,
					OleMfileOlgAttr_msg2);
	}

	/* Get the background GC */
	values.foreground = pInfo->bg.pixel;
	if (pInfo->bg1 = XCreateGC (display, root, GCForeground, &values))
	    pInfo->flags |= OLG_ALLOCBG1;
	else
		OlVaDisplayErrorMsg(	(Display *) NULL,
					OleNfileOlgAttr,
					OleTmsg3,
					OleCOlToolkitError,
					OleMfileOlgAttr_msg3);

	/* If bg2 not allocated, use bg1 */
	if (!pInfo->bg2)
	    pInfo->bg2 = pInfo->bg1;
    }

    /* If bg3 not allocated, use black.  If bg0 was not allocated, use white */
    if (OlgIs3d())
    {
	if (!pInfo->bg3)
	    pInfo->bg3 = pInfo->pDev->blackGC;

	if (!pInfo->bg0)
	    pInfo->bg0 = pInfo->pDev->whiteGC;
    }
}

/* Create and initialize an attributes structure
 *
 * Look for an existing attributes structure with the same attributes.  If
 * not found, allocate one and get the necessary GCs.  If the colors requested
 * are close to black or white, then use gray.  Do not allow 3-D on
 * monochrome devices.
 */

OlgAttrs *
OlgCreateAttrs OLARGLIST((scr, fg, bg, bgIsPixmap, scale))
    OLARG( Screen *,	scr)
    OLARG( Pixel,	fg)
    OLARG( OlgBG *,	bg)
    OLARG( Boolean,	bgIsPixmap)
    OLGRA( Dimension,	scale)
{
    register OlgAttrs	*current;
    register _OlgDevice	*pDev;

    /* Check for monochrome screen */
    if (!prohibit3D && DefaultDepthOfScreen (scr) == 1)
    {
	prohibit3D = True;
	OlgSetStyle3D (False);
    }

    /* Get the device structure for the screen and scale */
    pDev = _OlgGetDeviceData (scr, scale);

    /* Look for an existing entry */
    current = attrHead;
    while (current)
    {
	if (current->fg == fg && current->pDev == pDev)
	    if (current->flags & OLG_BGPIXMAP)
	    {
		if (bgIsPixmap && (current->bg.pixmap == bg->pixmap))
		    break;	/* found a match */
	    }
	    else
		if (!bgIsPixmap && (current->bg.pixel == bg->pixel))
		    break;	/* winner! */

	current = current->next;
    }

    /* If we found a match, bump the reference count and return it */
    if (current)
    {
	current->refCnt++;
	return current;
    }

    /* No luck.  Create a new one. */
    current = (OlgAttrs *) XtMalloc (sizeof (OlgAttrs));
    current->next = attrHead;
    attrHead = current;

    current->refCnt = 1;
    current->pDev = pDev;
    current->fg = fg;
    current->bg = *bg;
    current->flags = bgIsPixmap ? OLG_BGPIXMAP : 0;

    allocGCs (scr, current);

    return current;
}

/* Destroy Attributes
 *
 * If no one else is using an attributes structure, free all resources
 * associated with the structure and free it.
 */

void
OlgDestroyAttrs OLARGLIST((pInfo))
    OLGRA( register OlgAttrs *,	pInfo)
{
    register OlgAttrs *prev, *current;

    if (--pInfo->refCnt > 0)
	return;

    /* Free the GCs */
    if (pInfo->flags & OLG_ALLOCBG0)
	XFreeGC (DisplayOfScreen (pInfo->pDev->scr), pInfo->bg0);
    if (pInfo->flags & OLG_ALLOCBG1)
	XFreeGC (DisplayOfScreen (pInfo->pDev->scr), pInfo->bg1);
    if (pInfo->flags & OLG_ALLOCBG2)
	XFreeGC (DisplayOfScreen (pInfo->pDev->scr), pInfo->bg2);
    if (pInfo->flags & OLG_ALLOCBG3)
	XFreeGC (DisplayOfScreen (pInfo->pDev->scr), pInfo->bg3);

    /* Find the element in the list just before this one */
    current = attrHead;
    prev = (OlgAttrs *) 0;
    while (current && current != pInfo)
    {
	prev = current;
	current = current->next;
    }

    if (!current)
	OlVaDisplayErrorMsg(	(Display *) NULL,
				OleNfileOlgAttr,
				OleTmsg4,
				OleCOlToolkitError,
				OleMfileOlgAttr_msg4);

    if (prev)
	prev->next = current->next;
    else
	attrHead = current->next;

    XtFree ((char *)current);
}

/* Set Visuals Mode
 *
 * Set the visuals mode to 2- or 3-D drawing mode.  All previously allocated
 * GCs are freed and re-allocated in the new style.
 */

void
OlgSetStyle3D OLARGLIST((draw3d))
    OLGRA( Boolean,	draw3d)
{
    register OlgAttrs	*current;

    if (prohibit3D)
	draw3d = False;

    if ((OlgIs3d() == False) == (draw3d == False))
	return;		/* nothing to do */

    _olgIs3d = !_olgIs3d;

    /* For each attributes structure in the system, free the existing GCs
     * and get fresh ones.
     */
    current = attrHead;
    while (current)
    {
	if (current->flags & OLG_ALLOCBG0)
	    XFreeGC (DisplayOfScreen (current->pDev->scr), current->bg0);
	if (current->flags & OLG_ALLOCBG1)
	    XFreeGC (DisplayOfScreen (current->pDev->scr), current->bg1);
	if (current->flags & OLG_ALLOCBG2)
	    XFreeGC (DisplayOfScreen (current->pDev->scr), current->bg2);
	if (current->flags & OLG_ALLOCBG3)
	    XFreeGC (DisplayOfScreen (current->pDev->scr), current->bg3);
	current->flags &= ~(OLG_ALLOCBG0 | OLG_ALLOCBG1 | OLG_ALLOCBG2 |
			    OLG_ALLOCBG3);

	allocGCs (current->pDev->scr, current);

	current = current->next;
    }
}

/* Get shading color values
 *
 * Get RGB values for bg0, 2, and 3 given the pixel value for bg1.  This
 * is only meaningful in 3-D mode; in 2-D mode, just use black.
 */

void
OlgGetColors OLARGLIST((scr, pixel, bg0, bg2, bg3))
    OLARG( Screen *,	scr )
    OLARG( Pixel,	pixel )
    OLARG( XColor *,	bg0 )
    OLARG( XColor *,	bg2 )
    OLGRA( XColor *,	bg3 )
{
    if (OlgIs3d())
    {
	Colormap	cmap;
	unsigned short	luminance;
	static unsigned short	lightThreshold;
	XColor		bg1;
	OlColorTuple	*ct;

	cmap = DefaultColormapOfScreen (scr);

	/* Get the brightest color that we can represent, first time only. */
	if (lightThreshold == 0)
	{
	    bg1.red = bg1.green = bg1.blue = 65535;
	    bg1.flags = DoRed | DoGreen | DoBlue;
	    if (XAllocColor (DisplayOfScreen (scr), cmap, &bg1))
		lightThreshold = LUMINANCE (bg1) * 9 / 10;
	    else
		lightThreshold = LIGHT_THRESHOLD;
	}

	bg1.pixel = pixel;
	XQueryColor (DisplayOfScreen (scr), cmap, &bg1);

	if (ColorRich(scr))
	{
	    unsigned long	red, green, blue;

	    /* bg2:  10% darker than bg1. */
	    bg2->red = (unsigned) (bg1.red * 9l) / 10;
	    bg2->green = (unsigned) (bg1.green * 9l) / 10;
	    bg2->blue = (unsigned) (bg1.blue * 9l) / 10;

	    /* bg3:  50% darker than bg1 */
	    bg3->red = bg1.red / 2;
	    bg3->green = bg1.green / 2;
	    bg3->blue = bg1.blue / 2;

	    /* bg0:  lighter than bg1 */
	    red = bg1.red + 55000;
	    green = bg1.green + 55000;
	    blue = bg1.blue + 55000;
	    bg0->red = (red > 65535) ? 65535 : red;
	    bg0->green = (green > 65535) ? 65535 : green;
	    bg0->blue = (blue > 65535) ? 65535 : blue;
	}
	else
	{
	    /* Color-poor system */
	    bg0->red = bg0->green = bg0->blue = 65535;

	    bg2->red = bg1.red;
	    bg2->green = bg1.green;
	    bg2->blue = bg1.blue;

	    bg3->red = bg3->green = bg3->blue = 0;
	}

	/* If the color is very bright or very dark, adjust bg0 or bg3
	 * such that it will be visible.
	 */
	luminance = LUMINANCE (bg1);

	if (luminance < (unsigned long) DARK_THRESHOLD)
	{
	    bg0->red = bg0->green = bg0->blue = 65535;
	    bg3->red = bg3->green = bg3->blue = 32768;
	}
	else if (luminance > (unsigned long) lightThreshold)
	{
	    bg0->red = bg0->green = bg0->blue = 32768;
	    bg3->red = bg3->green = bg3->blue = 0;
	}
    }
    else
    {
	/* 2-D -- use black */
	bg0->red = bg0->green = bg0->blue = 0;
	bg2->red = bg2->green = bg2->blue = 0;
	bg3->red = bg3->green = bg3->blue = 0;
    }

    bg0->flags = bg2->flags = bg3->flags = DoRed | DoGreen | DoBlue;
}
