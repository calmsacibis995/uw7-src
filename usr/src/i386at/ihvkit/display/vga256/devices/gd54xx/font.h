#ident	"@(#)ihvkit:display/vga256/devices/gd54xx/font.h	1.1"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 * Copyright (c) 1992,1993 Pittsburgh Powercomputing Corporation (PPc).
 * All Rights Reserved.
 */

#define FONT_COUNT 	8		/* max # of downloadable fonts */
#define FONT_NUMGLYPHS	256		/* max # of glyphs per font */
#define FONT_MAXWIDTH	16		/* max width of a downloadable font */
#define FONT_MAXHEIGHT	16		/* max width of a downloadable font */
#define FONT_MAXSIZE	32		/* max possible converted glyph size */
#define FONT_DB_START	1024*789	/*off-screen address from where the fonts
									  will be downloaded */
#define FONT_MEMORY_SIZE	256*32	/*Maximum memory required to download a
									  a font in off-screen memory*/
#define BLTDATA unsigned short

