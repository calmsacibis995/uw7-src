#ident	"@(#)ihvkit:display/vga256/devices/gd54xx/gd54xx_256.h	1.2"

/*
 *	Copyright (c) 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#include "sys/inline.h"

#define THRESH_HOLD 15
#define MAX_WIDTH 2048 			/*max width that can drawn by BLIT engine*/ 
#define MAX_HEIGHT 1024			/*max height that can drawn by BLIT engine*/
#define TILE_DB_START (1024)*(vendorInfo.virtualY + 1)
#define TILE_DATA_SIZE 64
#define BIG_TILE_OFFSCREEN_ADDRESS (TILE_DB_START) + 16*1024
#define BLT_MAX_WIDTH 2048
#define BIG_TILE_MAX_SIZE 64*64
#define MAX_TILE_WIDTH 64
#define MAX_TILE_HEIGHT 64


#define DEFAULT_GD54XX_IDLE_COUNT		50000

/*
 * Wait for engine idle.
 */

#define WAIT_FOR_ENGINE_IDLE()\
{\
	 volatile int __count = DEFAULT_GD54XX_IDLE_COUNT;\
	 while (--__count > 0)\
	 {\
		 outb(0x3CE, 0x31);\
		 if (!(inb(0x3CF) & 0x01))\
		 {\
			break;\
		 }\
	 }\
	 if ( __count <= 0)\
	 {\
		 (void) fprintf(stderr, "GD54XX: WAIT IDLE RESET\n");\
		 outb(0x3CE, 0x31);\
		 outb(0x3CF, 0x04);\
	 }\
}

/*
 * update width register
 */
#define U_WIDTH(x) \
   outw (0x3ce, 0x20 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x21 | ((x) & 0x700));

 
/*
 * update height register
 */
#define U_HEIGHT(x) \
   outw (0x3ce, 0x22 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x23 | ((x) & 0x300));

/*
 * update source pitch 
 */
#define U_SRC_PITCH(x) \
   outw (0x3ce, 0x26 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x27 | ((x) & 0x0f00));

/*
 * update destination pitch 
 */
#define U_DEST_PITCH(x) \
   outw (0x3ce, 0x24 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x25 | ((x) & 0x0f00));

/*
 * update source address
 */
#define U_SRC_ADDR(x) \
   outw (0x3ce, 0x2c | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x2d | ((x) & 0xff00));\
   outw (0x3ce, 0x2e | (((x) >> 8) & 0x1f00));

/*
 * update destination address
 */
#define U_DEST_ADDR(x) \
   outw (0x3ce, 0x28 | (((x) & 0x00ff) << 8));\
   outw (0x3ce, 0x29 | ((x) & 0xff00));\
   outw (0x3ce, 0x2a | (((x) >> 8) & 0x1f00));

/*
 * update BLIT MODE REGISTER
 */
#define U_BLTMODE(x) \
    outw (0x3ce, 0x30 | (((x) & 0x00ff) << 8));


/*
 * update ROP register
 */
#define U_ROP(x) \
    outw (0x3ce, 0x32 | (((x) & 0x00ff) << 8));

/*
 * BLIT ENGINE start/status
 */
#define BLT_START(x) \
    outw (0x3ce, 0x31 | (((x) & 0x00ff) << 8));

/*
 * Enable extended write modes 4 or 5
 * Extend GR0 from 4bits to 8 bits
 * Extend GR1 from 4bits to 8 bits
 * Extend SR2 from 4bits to 8 bits
 */
#define ENABLE_EX_WRITE_MODES(x) \
    outw (0x3ce, 0x0b | (((x) & 0x00ff) << 8));

/*
 *update register SR2 to enable 8 pixel writing
 */
#define ENABLE_8_PIXELS(x) \
    outw (0x3ce, 0x02 | (((x) & 0x00ff) << 8));

/*
 *update the writing modes
 *Write mode 4 for transparent write
 *Write mode 5 for opaque write
 */
#define U_WRITE_MODE(x) \
    outw (0x3ce, 0x05 | (((x) & 0x00ff) << 8));

/*
 * update foreground color for BLIT 
 */
#define U_BLT_FG_LOW(x) \
    outw (0x3ce, 0x01 | (((x) & 0x00ff) << 8));


/*
 * update background color for BLIT 
 */
#define U_BLT_BG_LOW(x) \
    outw (0x3ce, 0x00 | (((x) & 0x00ff) << 8));

/*
 * update BLIT Transparent Color Register
 */
#define U_BLT_TRANS_COLOR_LOW(x) \
    outw (0x3ce, 0x34 | (((x) & 0x00ff) << 8));

#define U_BLT_TRANS_COLOR_HIGH(x) \
    outw (0x3ce, 0x35 | (((x) & 0x00ff) << 8));


/*
 * update BLIT Transparent Color Mask Register
 */
#define U_BLT_TRANS_COLOR_MASK_LOW(x) \
    outw (0x3ce, 0x38 | (((x) & 0x00ff) << 8));

#define U_BLT_TRANS_COLOR_MASK_HIGH(x) \
    outw (0x3ce, 0x39 | (((x) & 0x00ff) << 8));


#define CLOBBER(flags,flag,field,fn) \
{\
	if (pflags->flags & (flag))\
	{\
		oldfns.field = pfuncs->field; \
	}\
	else\
	{\
		oldfns.field = NULL; \
	}\
	pfuncs->field = (fn); \
	pflags->flags |= (flag); \
}

#define GRAB(field, fn) \
{\
	oldfns.field = pfuncs->field; \
	pfuncs->field = (fn); \
}

#define FALLBACK(field,args) \
if (oldfns.field != NULL)\
{\
	return( (*oldfns.field) args ); \
}\
else \
{\
	return(SI_FAIL);\
}
