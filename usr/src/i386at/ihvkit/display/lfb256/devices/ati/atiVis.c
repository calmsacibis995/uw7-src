#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiVis.c	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <ati.h>

#if (BPP == 8)
SIVisual ati_visuals[] = {
    {
	PseudoColor, BPP, ATI_NUM_CMAPS, ATI_CMAP_SIZE, ATI_BITS_RGB,
	0, 0, 0, 0, 0, 0,
    }
};
#endif

#if (BPP == 16)
SIVisual ati_visuals[] = {
};
#endif

int ati_num_visuals = sizeof(ati_visuals) / sizeof(SIVisual);
