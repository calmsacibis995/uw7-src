#ident	"@(#)ihvkit:display/lfb256/devices/jaws/jaws_i.c	1.1"

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

#include <jawsregs.h>

/*
 * Register initialization
 *
 * Row 1: x, y, freq
 * Row 2: register values
 * Row 3: bpp specific register values
 * The first value is bpp, the last is the line stride.
 */

#if (BPP == 8)
JawsRegs jaws_reg_vals[] = {
    {
	1280, 1024, 60,
	45, 15, 63, 320, 110, 203, 6, 6, 6, 64, 2048, 420,
	0, 1000, 24, 0xbc3371, 1280,
    },
    {
	1152, 900, 66,
	43, 13, 46, 288, 105, 177, 6, 6, 6, 64, 1800, 366,
	0, 1000, 24, 0xbc3371, 1152,
    },
    {
	1024, 768, 70,
	41, 17, 36, 256, 91, 161, 12, 6, 6, 52, 1536, 330,
	0, 1000, 24, 0xbc3371, 1024,
    },
    {
	1024, 768, 60,
	40, 17, 41, 256, 85, 160, 12, 6, 6, 52, 1536, 342,
	0, 1000, 24, 0xbc3371, 1024,
    },
    {
	800, 600, 72,
	38, 15, 17, 200, 71, 118, 74, 12, 12, 34, 1200, 258,
	0, 1008, 16, 0xbc3371, 800,
    },
    {
	800, 600, 60,
	37, 14, 16, 200, 63, 107, 8, 2, 2, 44, 1200, 274,
	0, 1009, 15, 0xbc3371, 800,
    },
    {
	640, 480, 73,
	36, 5, 34, 160, 51, 95, 6, 18, 18, 38, 960, 218,
	0, 1008, 16, 0xbc3371, 640,
    },
#ifdef BROKEN_MODE
    {
	640, 480, 60,
	35, 10, 16, 160, 61, 97, 4, 20, 20, 46, 960, 198,
	0, 1009, 15, 0xbc3371, 640,
    }
#endif
};

#endif

#if (BPP == 16)
JawsRegs jaws_reg_vals[] = {
    {
	1152, 900, 66,
	43, 13, 46, 288, 105, 177, 6, 6, 6, 64, 1800, 366,
	0, 488, 24, 0xdc3371, 2304,
    },
    {
	1120, 832, 70,
	0x2c, 0x10, 0x47, 0x118, 0x4e, 0xb5, 0x10, 0x10, 0x10, 0x50, 0x680, 194,
	0, 0x1e8, 0x18, 0xdc3311, 2240,
    },
    {
	1024, 768, 70,
	41, 17, 36, 256, 91, 161, 12, 6, 6, 52, 1536, 330,
	0, 488, 24, 0xdc3371, 2048,
    },
    {
	1024, 768, 60,
	40, 17, 41, 256, 85, 160, 12, 6, 6, 52, 1536, 342,
	0, 488, 24, 0xdc3371, 2048,
    },
    {
	800, 600, 72,
	38, 15, 17, 200, 71, 118, 74, 12, 12, 34, 1200, 258,
	0, 496, 16, 0xdc3371, 1600,
    },
    {
	800, 600, 60,
	37, 14, 16, 200, 63, 107, 8, 2, 2, 44, 1200, 274,
	0, 497, 15, 0xdc3371, 1600,
    },
    {
	640, 480, 73,
	36, 5, 34, 160, 51, 95, 6, 18, 18, 38, 960, 218,
	0, 496, 16, 0xdc3371, 1280,
    },
#ifdef BROKEN_MODE
    {
	640, 480, 60,
	35, 10, 16, 160, 61, 97, 4, 20, 20, 46, 960, 198,
	0, 497, 15, 0xdc3371, 1280,
    }
#endif
};

#endif

int jaws_num_reg_vals = sizeof(jaws_reg_vals) / sizeof(JawsRegs);
