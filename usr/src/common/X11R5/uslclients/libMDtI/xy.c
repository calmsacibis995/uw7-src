#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:xy.c	1.3"
#endif

#include <X11/Intrinsic.h>
#include "DtI.h"

Boolean
DmIntersectItems(DmItemPtr items, Cardinal num_items,
		 WidePosition x, WidePosition y, Dimension w, Dimension h)
{
/* coord's for existing icon */
#define IX	( item->x )
#define IY	( item->y )
#define IW	( (WidePosition)iw )
#define IH	( (WidePosition)ih )

/* coord's for icon to be positioned */
#define NX	x
#define NY	y
#define NW	xw
#define NH	yh

#define INRANGE(L,m,H)	((L) <= (m) && (m) <= (H))

    register WidePosition	xw = x + w - 1;		/* right-most 'x' */
    register WidePosition	yh = y + h - 1;		/* bottom-most 'y' */
    register Dimension	iw, ih;
    register DmItemPtr	item;

    for (item = items; item < items + num_items; item++)
    {
	iw = (Dimension)(ITEM_X(item) + ITEM_WIDTH(item) - 1);
	ih = (Dimension)(ITEM_Y(item) + ITEM_HEIGHT(item) - 1);
	if (ITEM_MANAGED(item) && (IX != 0 || IY != 0) &&
	    ((INRANGE(IX, NX, IW) || INRANGE(IX, NW, IW) ||
	      INRANGE(NX, IX, NW) || INRANGE(NX, IW, NW)) &&
	     (INRANGE(IY, NY, IH) || INRANGE(IY, NH, IH) ||
	      INRANGE(NY, IY, NH) || INRANGE(NY, IY, NH))))
	    return(True);
    }

    return(False);
}

void
DmGetAvailIconPos(Widget box, DmItemPtr items, Cardinal num_items,
		  Dimension item_width, Dimension item_height,
		  Dimension wrap_width,
		  Dimension grid_width, Dimension grid_height,
		  WidePosition * ret_x, WidePosition * ret_y)
{
    Dimension	margin;
    WidePosition	x, y;
    WidePosition	init_x;
    

    margin = XmConvertUnits(box, XmHORIZONTAL, Xm100TH_POINTS, ICON_MARGIN *100,
			    XmPIXELS);

    init_x = (WidePosition)((item_width > grid_width) ? margin :
			  margin + (WidePosition)(grid_width - item_width) / 2);
    x = init_x;
    y = (WidePosition)margin;

    while (DmIntersectItems(items, num_items, x, y, item_width, item_height))
    {
	/* overlapped with each others, so readjust the target... */
	x += grid_width;
	if ((Dimension)(x + item_width) > wrap_width)
	{
	    x = init_x;
	    y += grid_height;
	}

    }

    *ret_x = x;
    *ret_y = VertJustifyInGrid(y, item_height, grid_height);

}				/* end of DmGetAvailIconPos */

