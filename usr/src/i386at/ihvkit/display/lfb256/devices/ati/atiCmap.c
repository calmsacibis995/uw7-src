#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiCmap.c	1.1"

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

/*
 * Cmap 0 is for the original color.  Cmap 1 is for the X server.
 *
 */

static u_char saved_cmap[2][ATI_CMAP_SIZE*3];


atiSaveCmap(map)
int map;

{
    int i;
    u_char *p;

    p = &saved_cmap[map][0];

    outb(DAC_MASK, 0xff);
    outb(DAC_R_INDEX, 0);
    for (i = 0; i < ATI_CMAP_SIZE * 3; i++) {
	*p++ = inb(DAC_DATA);
    }
}

atiRestoreCmap(map)
int map;

{
    int i;
    u_char *p;

    p = &saved_cmap[map][0];

    outb(DAC_MASK, 0xff);
    outb(DAC_W_INDEX, 0);
    for (i = 0; i < ATI_CMAP_SIZE * 3; i++) {
	outb(DAC_DATA, *p++);
    }
}

/*	COLORMAP MANAGEMENT ROUTINES	*/
/*		MANDATORY		*/

SIBool atiSetCmap(visual, cmap, colors, count)
SIint32 visual;
SIint32 cmap;
SIColor *colors;
SIint32 count;

{
    register int i;
    register u_int c;

    if (ati_visuals[visual].SVtype == PseudoColor) {
	for (; count; count--, colors++) {
	    if (colors->SCpindex > ATI_CMAP_SIZE)
		continue;

	    i = colors->SCpindex;
	    outb(DAC_W_INDEX, i);
	    c = (u_int)colors->SCred >> 10;
	    saved_cmap[1][3*i+0] = c;
	    outb(DAC_DATA, c);
	    c = (u_int)colors->SCgreen >> 10;
	    saved_cmap[1][3*i+1] = c;
	    outb(DAC_DATA, c);
	    c = (u_int)colors->SCblue >> 10;
	    saved_cmap[1][3*i+2] = c;
	    outb(DAC_DATA, c);
	}
    }
    else {
	fprintf(stderr, "Set colormap: Bad visual (%d)\n", visual);
	return(SI_FAIL);
    }

    return(SI_SUCCEED);
}

SIBool atiGetCmap(visual, cmap, colors, count)
SIint32 visual;
SIint32 cmap;
SIColor *colors;
SIint32 count;

{
    register int i;

    if (ati_visuals[visual].SVtype == PseudoColor) {
	for (; count; count--, colors++) {
	    i = colors->SCpindex;
	    colors->SCred   = saved_cmap[1][3*i+0] << 10;
	    colors->SCgreen = saved_cmap[1][3*i+1] << 10;
	    colors->SCblue  = saved_cmap[1][3*i+2] << 10;
	}
    }
    else {
	fprintf(stderr, "Get colormap: Bad visual (%d)\n", visual);
	return(SI_FAIL);
    }

    return(SI_SUCCEED);
}
