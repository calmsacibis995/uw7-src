/*
 *	@(#)s3cCmap.c	6.1	3/20/96	10:23:04
 *
 * 	Copyright (C) Xware, 1991-1992.
 *
 * 	The information in this file is provided for the exclusive use
 *	of the licensees of Xware. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they 
 *	include this notice and the associated copyright notice with 
 *	any such product.
 *
 *	Copyright (C) The Santa Cruz Operation, 1993
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	The information in this file is provided for the exclusive use
 *	of the licensees of SCO. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such
 *	product.
 *
 * Modification History
 *
 * S009, 02-Jun-93, staceyc
 * 	make sure we don't clobber fake cursor DAC entry, without making a new
 *	one
 * S008, 17-May-93, staceyc
 * 	support for multiheaded S3 cards
 * S007, 11-May-93, staceyc
 * 	include file cleanup
 * S006	Fri Nov 06 08:46:18 PST 1992	hiramc@sco.COM
 *	- add gcstruct.h for new items in nfbScrStr.h
 *	- remove modification comments before 1992
 * X005 02-Jan-92 kevin@xware.com
 *      added replication of values in top of pallete, may be required 
 *	for some hardware implementations.
 * X004 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"

/*
 *  s3cSetColor() -- Set PsuedoColor Colormap Entry
 *
 *	This routine will set a single entry in the physical colormap
 *	entry in the DAC.
 *
 */

void
S3CNAME(SetColor)(
	unsigned int 		cmap,
	unsigned int 		index,
	unsigned short 		r,
	unsigned short 		g,
	unsigned short 		b,
	ScreenPtr 		pScreen)
{
	int			top;
	s3cPrivateData_t	*s3cPriv = S3C_PRIVATE_DATA(pScreen);
	unsigned int 		shift = s3cPriv->dac_shift;
        s3cHWCursorData_t *s3cHWCurs = S3C_HWCURSOR_DATA(pScreen);

	if ( index > pScreen->visuals[0].ColormapEntries )
		return;

	/* 
	 * only top grafinfo RGBBITS bits sent to DAC 
	 */

	r >>= shift;
	g >>= shift;
	b >>= shift;

	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

	S3C_OUTB(S3C_PALMASK, 0xFF);
		
	if ( s3cPriv->depth == 8 )
	{
		S3C_OUTB(S3C_PALWRITE_ADDR, index);
		S3C_OUTB(S3C_PALDATA, (unsigned char)r);
		S3C_OUTB(S3C_PALDATA, (unsigned char)g);
		S3C_OUTB(S3C_PALDATA, (unsigned char)b);
	}
	else
	{
		for ( top = 0; top < 16; ++top )
		{
			S3C_OUTB(S3C_PALWRITE_ADDR, (top << 4) + index);
			S3C_OUTB(S3C_PALDATA, (unsigned char)r);
			S3C_OUTB(S3C_PALDATA, (unsigned char)g);
			S3C_OUTB(S3C_PALDATA, (unsigned char)b);
		}
	}

	if (s3cPriv->cursor_type == S3C_CURSOR_TYPE_HW &&
	    (index == s3cHWCurs->fore.pixel || index == s3cHWCurs->mask.pixel))
	{
		S3CNAME(HWColorCursor)(pScreen, s3cHWCurs->current_cursor);
	}
}


/*
 *  S3CNAME(RestoreColormap)() -- Restore PsuedoColor Colormap 
 *
 *	This routine will restore the entire colormap.
 *
 */

void
S3CNAME(RestoreColormap)(
	ScreenPtr 	pScreen)
{
	nfbScrnPriv 	*devPriv = NFB_SCREEN_PRIV(pScreen);
	ColormapPtr 	pmap = devPriv->installedCmap;

	(*devPriv->LoadColormap)(pmap);
}


/*
 *  s3cLoadColormap() -- Load a PsuedoColor Colormap
 *
 *	This routine will load the entire colormap into the phyisical 
 *	colormap in the DAC.
 *
 */

void
S3CNAME(LoadColormap)(
	ColormapPtr 	pmap)
{
	ScreenPtr 		pScreen = pmap->pScreen;
	register Entry 		*pent;
	register int 		i;
	int			nents;
	unsigned char		r;
	unsigned char		g;
	unsigned char		b;
	s3cPrivateData_t 	*s3cPriv = S3C_PRIVATE_DATA(pmap->pScreen);
	unsigned int 		shift;
	int			top;
        s3cHWCursorData_t *s3cHWCurs = S3C_HWCURSOR_DATA(pmap->pScreen);

	if ( pmap == NULL )
		return;

	shift = s3cPriv->dac_shift;
	nents = pmap->pVisual->ColormapEntries;

	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);
	
	S3C_OUTB(S3C_PALMASK, 0xFF);

	for ( pent = pmap->red, i = 0; i < nents; i++, pent++ )
	{
		if ( pent->fShared )
		{
			r = pent->co.shco.red->color >> shift;
			g = pent->co.shco.green->color >> shift;
			b = pent->co.shco.blue->color >> shift;
		}
		else
		{
			r = pent->co.local.red >> shift;
			g = pent->co.local.green >> shift;
			b = pent->co.local.blue >> shift;
		}

		if ( s3cPriv->depth == 8 )
		{
			S3C_OUTB(S3C_PALWRITE_ADDR, i);
			S3C_OUTB(S3C_PALDATA, r);
			S3C_OUTB(S3C_PALDATA, g);
			S3C_OUTB(S3C_PALDATA, b);
		}
		else
		{
			for ( top = 0; top < 16; ++top )
			{
				S3C_OUTB(S3C_PALWRITE_ADDR, (top << 4) + i);
				S3C_OUTB(S3C_PALDATA, (unsigned char)r);
				S3C_OUTB(S3C_PALDATA, (unsigned char)g);
				S3C_OUTB(S3C_PALDATA, (unsigned char)b);
			}
		}
	}
	if (s3cPriv->cursor_type == S3C_CURSOR_TYPE_HW)
	{
		/*
		 * force a cursor recolor
		 */
		s3cHWCurs->current_colormap = 0;
		S3CNAME(HWColorCursor)(pScreen, s3cHWCurs->current_cursor);
	}
}
