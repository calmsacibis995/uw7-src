#ident	"@(#)ihvkit:display/lfb256/devices/ati/atiGlob.c	1.1"

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

ATI ati;

/*
 * Default clipping off.  This still requires some work to be done
 * during initialization as the Mach32 scissor registers need to be set.
 *
 */
int atiClipping = 0;

/*
 * The following store the locations of the off-screen memory objects.
 *
 */
ATI_OFF_SCRN atiOffTiles[LFB_NUM_GSTATES];
ATI_OFF_SCRN atiOffStpls[LFB_NUM_GSTATES];

SIFontInfo atiFontInfo[ATI_NUM_FONTS];
SIGlyphP atiGlyphs[ATI_NUM_FONTS];

/* The following table translates from the X11 GC.mode to the ATI
 * chip's equivalent.  The order is dependent on the X11 definition.
 * X11 lists the modes in the following order:
 *
 *     GXClear		0
 *     GXand		src & dst
 *     GXandReverse	src & (~dst)
 *     GXcopy		src
 *     GXandInverted	(~src) & dst
 *     GXnoop		dst
 *     GXxor		src ^ dst
 *     GXor		src | dst
 *     GXnor		(~src) & (~dst)
 *     GXequiv		(~src) ^ dst
 *     GXinvert		~dst
 *     GXorReverse	src | (~dst)
 *     GXcopyInverted	~src
 *     GXorInverted	(~src) | dst
 *     GXnand		(~src) | (~dst)
 *     GXset		~0
 *
 */

int ati_mode_trans[] = {
    0x01, 0x0c, 0x0d, 0x07, 0x0e, 0x03, 0x05, 0x0b,
    0x0f, 0x06, 0x00, 0x0a, 0x04, 0x09, 0x08, 0x02,
};
