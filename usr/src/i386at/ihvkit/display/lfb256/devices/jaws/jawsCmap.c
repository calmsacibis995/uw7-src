#ident	"@(#)ihvkit:display/lfb256/devices/jaws/jawsCmap.c	1.1"

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


/*	COLORMAP MANAGEMENT ROUTINES	*/
/*		MANDATORY		*/

SIBool jawsSetCmap(visual, cmap, colors, count)
SIint32 visual;
SIint32 cmap;
SIColor *colors;
SIint32 count;

{
    if (jaws_visuals[visual].SVtype == PseudoColor) {
	for (; count; count--, colors++) {
	    *(jaws_cmap + colors->SCpindex) =
		((colors->SCred   & 0xff00) << 8 |
		 (colors->SCgreen & 0xff00)      |
		 (colors->SCblue  & 0xff00) >> 8);

	    PAUSE;
	}
    }
    else {
	fprintf(stderr, "Jaws set cmap: visual  (%d)\n", visual);
	return(SI_FAIL);
    }

    return(SI_SUCCEED);
}

SIBool jawsGetCmap(visual, cmap, colors, count)
SIint32 visual;
SIint32 cmap;
SIColor *colors;
SIint32 count;

{
    int c;

    if (jaws_visuals[visual].SVtype == PseudoColor) {
	for (; count; count--, colors++) {
	    c = *(jaws_cmap + colors->SCpindex);
	    colors->SCred   = c & 0xff0000 >> 8;
	    colors->SCgreen = c & 0x00ff00;
	    colors->SCblue  = c & 0x0000ff << 8;

	    PAUSE;
	}
    }
    else {
	fprintf(stderr, "Jaws get cmap: visual  (%d)\n", visual);
	return(SI_FAIL);
    }

    return(SI_SUCCEED);
}
