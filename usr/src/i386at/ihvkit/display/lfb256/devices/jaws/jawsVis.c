#ident	"@(#)ihvkit:display/lfb256/devices/jaws/jawsVis.c	1.1"

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

#include <jaws.h>

#if (BPP == 8)
SIVisual jaws_visuals[] = {
    {PseudoColor, 8, 1, 256, 8, 0, 0, 0, 0, 0, 0}
};
#endif

#if (BPP == 16)
SIVisual jaws_visuals[] = {
};
#endif

int jaws_num_visuals = sizeof(jaws_visuals) / sizeof(SIVisual);
