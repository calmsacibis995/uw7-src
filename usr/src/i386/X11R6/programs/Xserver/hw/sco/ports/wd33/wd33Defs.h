/*
 *  @(#) wd33Defs.h 11.1 97/10/22
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
 *     wd33Defs.h     Register and bit definitions for WDC33
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *	S001	Tue 17-Aug-1993	edb@sco.com
 *              Put hash coding here
 */


#ifdef usl
#include <sys/types.h>
#include <sys/inline.h>
#else
#include "compiler.h" 
#endif

/*  Register blocks */

#define SYS_C                  0
#define ENG_1                  1
#define CURSR                  2
#define ENG_2                  3

/*  Ports           */

#define INDEX_PORT_L      0x23C0
#define INDEX_PORT_H      0x23C1
#define ACCESS_PORT_L     0x23C2
#define ACCESS_PORT_H     0x23C3
#define BITBLT_IO_PORT_L0 0x23C4
#define BITBLT_IO_PORT_H0 0x23C5
#define BITBLT_IO_PORT_L1 0x23C6
#define BITBLT_IO_PORT_H1 0x23C7
#define LINE_DRAW_K1      0x23C8
#define LINE_DRAW_K2      0x23CA
#define LINE_DRAW_ERR     0x23CC
#define CMD_BUFF_CTRL     0x23CE

/* Drawing engine 1 registers  */

#define CNTRL_1_IND       0x0000
#define CNTRL_2_IND       0x0001
#define SOURCE_X          0x0002
#define SOURCE_Y          0x0003
#define DEST_X            0x0004
#define DEST_Y            0x0005
#define DIM_X_IND         0x0006
#define DIM_Y_IND         0x0007
#define RASTEROP_IND      0x0008
#define LEFT_CLIP         0x0009
#define RIGHT_CLIP        0x000A
#define TOP_CLIP          0x000B
#define BOTTOM_CLIP       0x000C
#define REGISTER_BL_IND   0x000F

/* Drawing engine 2 registers  */

#define MAP_BASE          0x0000
#define ROW_PITCH         0x0001
#define FOREGR_IND_0      0x0002
#define FOREGR_IND_1      0x0003
#define BACKGR_IND_0      0x0004
#define BACKGR_IND_1      0x0005
#define TRANSPCOL_IND_0   0x0006
#define TRANSPCOL_IND_1   0x0007
#define TRANSPMASK_IND_0  0x0008
#define TRANSPMASK_IND_1  0x0009
#define PLANEMASK_IND_0   0x000A
#define PLANEMASK_IND_1   0x000B

/* Cursor register indices  */

#define CURS_CNTRL        0x0000
#define CURS_PATTERN      0x0001
#define CURS_PRIM_COLOR   0x0003
#define CURS_SEC_COLOR    0x0004
#define CURS_ORIGIN       0x0005
#define CURS_POS_X        0x0006
#define CURS_POS_Y        0x0007
#define CURS_AUX_COLOR    0x0008

/*  Drawing engine Control Reg 1  */

/*                        0x0000      Bit 15:12 Register Index   0         */
                                   /* Bit 11:9  Drawing Modes              */
#define NOOP              0x0000   /*        No Op    - Stop Drawing       */
#define BITBLIT           0x0200   /*        BitBlit                       */
#define LINE_STRIP        0x0400   /*        Line Strip                    */
#define TRAPEZOID_FILL    0x0600   /*        Trapezoidal fill              */
#define BRESENHAM_LINE    0x0800   /*        Bresenham line                */

                                   /* Bit 8  X Direction pos/neg           */
#define RIGHT_LEFT        0x0100   /*        default: X positive           */
                                   /* Bit 7  Y Direction pos/neg           */
#define BOTTOM_TOP        0x0080   /*        default: Y positive           */
                                   /* Bit 6  Major movement X or Y         */
#define MAJOR_Y           0x0040   /*        default: major Y              */
                                   /* Bit 5   Source Select                */ 
#define SRC_IS_IO_PORT    0x0020   /*        default: Source is screen     */
                                   /* Bit 4:3 Source Format                */
#define SRC_IS_MONO_COMP  0x0008   /*        Src is mono image fr comparator*/
#define SRC_IS_FIXED_COL  0x0010   /*        Fill with fixed color         */
#define SRC_IS_MONO       0x0018   /*        Source is mono image fr host  */
                                   /* Bit 2   Pattern enable               */
#define PATTERN_8x8       0x0004
                                   /* Bit 1   Destination Select           */ 
#define DST_IS_IO_PORT    0x0002   /*       default: Destination is screen */
                                   /* Bit 0   Controls Bresenh linedrawing */
#define LAST_PIXEL_OFF    0x0001   /*       default: Last pixel is on      */


/*  Drawing engine Control Reg 2  */

/*                        0x1000      Bit 15:12 Register Index     1       */
                                   /* Bit 11:10 BitBlit Address Mode       */
#define PACKED_8_BITS     0x0400   /*        default: planar 4 bits/pixel  */
#define PACKED_16_BITS    0x0800   /*                                      */
#define DEST_TRANSP_ENAB  0x0200   /* Bit 9 Enable destination transparency */
                                   /*                                      */
#define MATCHING_PIX_OPAQ 0x0100   /* Bit 8 Color transparency             */
                                   /*        default: matching pix. transp */
#define MONO_TRANSP_ENAB  0x0080   /* Bit 7 Monochrome Transparency        */
                                   /*                                      */
                                   /* Bit 6:5 Reserved, should be 11       */
#define BIT_6_5           0x0060   /*                                      */
                                   /* Bit 4  Data path FIFO Depth          */
#define FIFO_2            0x0010   /*        default: FIFO 4 levels deep   */
                                   /* Bit 3  Host BitBlit trough memory    */
#define HBLT_TROUGH_MEM   0x0008   /*        default: BitBlit through port */
                                   /* Bit 2:0 HBLT color expand control    */
#define COLOR_EXPAND_2    0x0002   /*        Use 2 bits per CPU write      */
#define COLOR_EXPAND_4    0x0003   /*        Use 4 bits per CPU write      */
#define COLOR_EXPAND_8    0x0004   /*        Use 8 bits per CPU write      */
#define COLOR_EXPAND_16   0x0005   /*        Use 16 bits per CPU write     */


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

/*  Command buffer          */

#define DE_BUSY           0x0080  /* Bit 7   read: Drawing engine busy     */
#define ABORT_DE_CMD      0x0080  /* Bit 7   write: Abort DE command       */
#define ENABLE_CMD_BUFF   0x0020  /* Bit 5   Enable CMD buffer             */
#define DISABLE_CMD_BUFF  0x0000  /*         Clear Int. ,disable CMD buffer*/
#define CMD_BUFF_LOC      0x000F  /* Bit 3:0 Available cmd buffer locations*/

/*  Commands        */

#define AUTO_INC_DISABLE   0x1000

#define WRITE_REG( block, index, word )  \
        if( (block) != curBlock )  \
        outw( INDEX_PORT_L,  ( curBlock = (block) ) ); \
	outw( ACCESS_PORT_L,   ((word) & 0x0FFF )   | ((index)<<12))

#define WRITE_2_REG( block, index, word )  \
        if( (block) != curBlock )  \
        outw( INDEX_PORT_L,  ( curBlock = (block) ) ); \
	outw( ACCESS_PORT_L,   ((word)     & 0x0FF )   | ((index)<<12)); \
	outw( ACCESS_PORT_L,  (((word)>>8) & 0x0FF )   | ((index+1)<<12))

#define READ_REG( word, block, index ) \
	outb( INDEX_PORT_L, (block) |  ((index)<<8) | AUTO_INC_DISABLE );  \
	word = inw( ACCESS_PORT_L ) & 0xFFF; curBlock = block 


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
/*
 *   Hash table macros for glyph and fontcaching
 */
#define GL_HASH_TABLE_SIZE_MASK   0x3FF
#define GL_HASH_TABLE_SIZE (GL_HASH_TABLE_SIZE_MASK + 1)

#define GL_HASH(a, b) (((a) + ((b) >> 4)) & GL_HASH_TABLE_SIZE_MASK)


/*
 *   WD90C33 syncronization
 */

#define WAITFOR_BUFF( n )	while( (inw( CMD_BUFF_CTRL ) & CMD_BUFF_LOC ) > 8-n )

#ifndef DEBUG_ONLY

#define WAITFOR_DE()	while( inw( CMD_BUFF_CTRL ) & DE_BUSY  )

#else

#define WAITFOR_DE() \
        { \
            unsigned short cntrl_1; \
            long time; \
            cntrl_1 = inw( CMD_BUFF_CTRL ); \
            if( cntrl_1 &  DE_BUSY  ) \
            { \
                time = clock();\
                do \
                { \
                     cntrl_1 = inw( CMD_BUFF_CTRL ); \
                     if( clock() > time+5000000 )  \
                     { \
                          ErrorF(" CMD_BUFF_CTRL = %x\n", cntrl_1); \
                          ErrorF(" Drawing Engine stopped after 5 sec\n"); \
                          outw( CMD_BUFF_CTRL, ABORT_DE_CMD | ENABLE_CMD_BUFF ); \
                          cntrl_1 = inw( CMD_BUFF_CTRL ); \
                     } \
                } \
                while( cntrl_1 &  DE_BUSY  ); \
            } \
        }

#endif

#define COMPLETE() \
        { \
            unsigned short cntrl_1,n=0; \
            while( 1 ) \
            { \
                 cntrl_1 = inw( CMD_BUFF_CTRL ); \
                 if( ! (cntrl_1 &  DE_BUSY) ) break; \
                 outd( BITBLT_IO_PORT_L0, 0 ); \
                 if( n++ > 100 ) break; \
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
