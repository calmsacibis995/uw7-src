/*
 *  @(#) wdDefs.h 11.1 97/10/22
 *
 * Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */
/*
 *     wdDefs.h     Register and bit definitions for WDC31
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *      S001    Wdn 14-Oct-1992 edb@sco.com
 *              changes to implement WD90C31 cursor
 *	S002	Thu  29-Oct-1992	edb@sco.com
 *              include compiler.h for INLINE definition
 *      S003    Thu 05-Oct-1992 edb@sco.com
 *              defines for tile caching
 *      S004    Tue 09-Feb-1993 buckm@sco.com
 *              add assert() defines.
 *		WAITFOR_WD() does not need to use a backoff algorithm.
 *		add defs for fast 8-bit fonts.
 *		move glyph defs to wdGlyph.c.
 *      S005    Thu Apr 15    edb@sco.com
 *              change #define's for fontcaching
 */


#ifdef usl
#include "sys/types.h"
#include "sys/inline.h"
#else
#include "compiler.h"     /* S002 */
#endif

/*  Register blocks */

#define SYS_CONTROL_BLOCK      0
#define BITBLT_BLOCK           1
#define CURSOR_BLOCK           2

/*  Ports           */

#define INDEX_PORT_L      0x23C0
#define INDEX_PORT_H      0x23C1
#define ACCESS_PORT_L     0x23C2
#define ACCESS_PORT_H     0x23C3
#define BITBLT_IO_PORT_L  0x23C4
#define BITBLT_IO_PORT_H  0x23C5

/* Blit register indices  */

#define CNTRL_1_IND       0x0000
#define CNTRL_2_IND       0x0001
#define SOURCE_IND        0x0002
#define DEST_IND          0x0004
#define DIM_X_IND         0x0006
#define DIM_Y_IND         0x0007
#define ROWBYTES_IND      0x0008
#define RASTEROP_IND      0x0009
#define FOREGR_IND        0x000A
#define BACKGR_IND        0x000B
#define TRANSPCOL_IND     0x000C
#define TRANSPMASK_IND    0x000D
#define PLANEMASK_IND     0x000E

#define AUTO_INC_DISABLE  0x0010 

/* Cursor register indices  */

#define CURS_CNTRL        0x0000
#define CURS_PATTERN      0x0001
#define CURS_PRIM_COLOR   0x0003
#define CURS_SEC_COLOR    0x0004
#define CURS_ORIGIN       0x0005
#define CURS_POS_X        0x0006
#define CURS_POS_Y        0x0007
#define CURS_AUX_COLOR    0x0008

/*  Control Port 1  */

                                   /* Bit 11 Activation status             */
#define START             0x0800   /*        default: Don't start          */
#define BUSY              0x0800   /*                                      */
                                   /* Bit 10 BitBlit Direction             */
#define BOT_TOP_RGT_LFT   0x0400   /*        bottom-top right-left         */
#define TOP_BOT_LFT_RGT        0   /*        to-bottom  left-right         */
                                   /* Bit 9:8 BitBlit Address Mode         */
#define PACKED_MODE       0x0100   /*        default: Planar mode          */
                                   /* Bit 7  Destination Linearity         */ 
#define DST_LINEAR        0x0080   /*        default: Dest rectangular     */
                                   /* Bit 6  Source Linearity              */ 
#define SRC_LINEAR        0x0040   /*        default: Source rectangular   */
                                   /* Bit 5:4 Destination Select           */ 
#define DST_IS_IO_PORT    0x0020   /*        default: Source is screen     */
                                   /* Bit 3:2 Source Format                */
                                   /*        Src is color image            */
#define SRC_IS_MONO_COMP  0x0004   /*        Src is mono image fr comparator*/
#define SRC_IS_FIXED_COL  0x0008   /*        Fill with fixed color         */
#define SRC_IS_MONO       0x000C   /*        Source is mono image fr host  */
                                   /* Bit 1:0 Destination Select           */ 
#define SRC_IS_IO_PORT    0x0002   /*        default: Source is screen     */

/*  Control port 2  */

                                   /* Bit 10 Interupt Enable               */
#define INTERR_ON_COMPLET 0x0400   /*        default: Don't interupt       */
                                   /* Bit  7 Quick start                   */
#define QUICK_START       0x0080   
                                   /* Bit  6 Update Destination            */
#define UPDATE_DEST       0x0040   
                                   /* Bit 5:4 Pattern select               */
#define PATTERN_8x8       0x0010
                                   /* Bit  3 Monochrome Transparency       */
#define MONO_TRANSP_ENAB  0x0008
                                   /* Bit  2 Transparency Polarity         */
#define MATCHING_PIX_OPAQ 0x0004   /*        default: matching pix. transp */
                                   /* Bit  0 Destination Transparency      */
#define TRANSP_ENAB       0x0001   

#define ALL_DEFAULTS           0   /*        All defaults                  */

/*  Cursor Control  */

#define CURS_ENAB         0x0800   /* Bit 11    1 ... Enable Cursor        */
#define CURS_DISAB        0x0000   /*           0 ... Disable Cursor       */
#define PIX32             0x0000   /* Bit 10:9  00 ... 32 x 32 pixels      */
#define PIX64             0x0200   /*           01 ... 64 x 64 pixels      */
#define PLANE_PROT        0x0100   /* Bit  8 Enable plane pprotection      */
#define MONOCHROM         0x0000   /* Bit  7:5  000 .. monochrome          */
#define TWO_COLOR         0x0020   /*           001 .. two color w. invers */
#define TWO_COLOR_SPEC    0x0040   /*           010 .. two color special   */
#define TRHEE_COLOR       0x0060   /*           011 .. three color         */

/*  Commands        */

#define SELECT_CURSOR_REG_BLOCK()  \
	outb( INDEX_PORT_L, CURSOR_BLOCK )

#define SELECT_BITBLT_REG_BLOCK()  \
	outb( INDEX_PORT_L, BITBLT_BLOCK ); \
	outb( INDEX_PORT_H, CNTRL_1_IND | AUTO_INC_DISABLE )

#define WRITE_1_REG( index, word )  \
	outw( ACCESS_PORT_L,   ((word) & 0x0FFF )           | ((index)<<12))

#define WRITE_2_REG( index, dword )  \
	outw(ACCESS_PORT_L, (((dword) & 0x001FF000) >> 12)| ((index+1)<<12));\
	outw(ACCESS_PORT_L,  ((dword) & 0x00000FFF)       | ((index)<<12))

#define READ_1_REG( word, index ) \
	outb( INDEX_PORT_H, (index) );  \
	word = inw ( ACCESS_PORT_L ) & 0xFFF; \
	outb( INDEX_PORT_H, CNTRL_1_IND | AUTO_INC_DISABLE )

#define READ_2_REG( dword, index ) /* Auto-incr bit in index must be 0 */\
	outb( INDEX_PORT_H, (index) ); \
	dword  =  inw ( ACCESS_PORT_L ) & 0x0FFF; \
	dword |= (unsigned int)(inw ( ACCESS_PORT_L ) & 0x01FF ) << 12; \
	outb( INDEX_PORT_H, CNTRL_1_IND | AUTO_INC_DISABLE )


/*  Misc  */

#define WD_TILE_WIDTH		8
#define WD_TILE_HEIGHT 		8
#define	WD_TILE_SIZE		(WD_TILE_WIDTH * WD_TILE_HEIGHT)

#define NOT_LOADED		0   /* for tile cache */
#define LOADED			1
#define PREF_LOADED		2
                                   /*   Font caching routines */
#define	WD_PLANES		8
#define	WD_FONT_WIDTH		12   /* must be multiple of 4 */
#define	WD_FONT_HEIGHT		24
#define WD_CACHE_LOCATIONS      32   /* 256 / 8 cache locations per font 
                                       ( using the planemask we can cache 8 chars in one location) */
#define WD_EXP_BUFF_SIZE        1920 /* expansion buffer for monoImages and
                                        fonts when depth >8 . Holds at least
                                        4 scanlines or character */
#ifndef DEBUG_ONLY

#define WAITFOR_WD()	while( inw( ACCESS_PORT_L ) & BUSY )

#else

#define WAITFOR_WD() \
        { \
            unsigned short cntrl_1; \
            long time; \
            READ_1_REG( cntrl_1 , CNTRL_1_IND ); \
            if( cntrl_1 &  BUSY ) \
            { \
                time = clock();\
                do \
                { \
                     READ_1_REG( cntrl_1 , CNTRL_1_IND ); \
                     if( clock() > time+5000000 )  \
                     { \
                          ErrorF(" WDC31 stopped after 5 sec\n"); \
                          WRITE_1_REG( CNTRL_1_IND, ALL_DEFAULTS ); \
                          break; \
                     } \
                } \
                while( cntrl_1 &  BUSY ); \
            } \
        }

#endif

#define COMPLETE() \
        { \
            unsigned short cntrl_1; \
            while( 1 ) \
            { \
                 READ_1_REG( cntrl_1 , CNTRL_1_IND ); \
                 if( ! (cntrl_1 &  BUSY) ) break; \
                 outw( BITBLT_IO_PORT_L, 0 ); \
                 ErrorF("?"); \
            } ; \
            ErrorF("\n"); \
         }


#ifdef DEBUG
#define assert(expr) {if (!(expr)) \
		FatalError("Assertion failed file %s, line %d: expr\n", \
			__FILE__, __LINE__); }
#else
#define assert(expr)
#endif
