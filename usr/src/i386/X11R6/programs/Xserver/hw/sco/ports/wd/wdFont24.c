/*
 * @(#) wdFont24.c 11.1 97/10/22
 *
 * Copyright (C) 1993 The Santa Cruz Operation, Inc.
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
 * wdFont24.c - routines for 15 16 and 24 -bit terminal-emulator fonts.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Fri Apr 02 1993	edb@sco.com
 *	- Created from wdFont.c.
 *      S001    Thu 22-Apr-1993 edb@sco.com
 *      - Change code for downloading the stipples
 *      S002    Fri 07-May-1993 edb@sco.com
 *      - ifdef debug code
 */

#include <stdio.h>
#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbRop.h"
#include "nfb/nfbScrStr.h"

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdBitswap.h"
#include "wdProcs.h"


/*
 *   Divide characters up in columns and Download into offscreen mem
 */
void
wdDownloadFont15_24(
	unsigned char **ppbits,
	int count,
	int width,
	int height,
	int stride,
	int index,
	ScreenPtr pScreen)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pScreen);
	unsigned char *image_pnt, *image_row;
        unsigned char *expanded_pnt, *expanded_row;
        unsigned char * tab = wdPriv->expandTab;
	int exp_nibbles;
        int dstAddr, char_mask;
        int pixb = wdPriv->pixBytes;

#ifdef DEBUG_PRINT 
	ErrorF("Load Font 24 16 15 #%d %dx%d by %d\n",
                         index, width, height, count);
#endif

        WAITFOR_WD();

        WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
	WRITE_2_REG( SOURCE_IND    , 0 );
        WRITE_1_REG( DIM_X_IND     , width * pixb );
        WRITE_1_REG( DIM_Y_IND     , height );
        WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
        WRITE_1_REG( FOREGR_IND    , 0xFF );
        WRITE_1_REG( BACKGR_IND    , 0x00 );

        WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | DST_LINEAR |
				     SRC_IS_MONO | SRC_IS_IO_PORT );


        exp_nibbles = (width * pixb +3 ) >> 2;
	dstAddr = wdPriv->fontBase + index * wdPriv->fontSprite * WD_CACHE_LOCATIONS;
        char_mask = 0x01;

	while (count-- > 0) 
        {                         /* Character loop */
		int h = height;
                int w;
                unsigned int bits;


                WRITE_1_REG( PLANEMASK_IND , char_mask );
	        WRITE_2_REG( DEST_IND , dstAddr );

		image_row    =  *ppbits++;
		h = height;
		while (h-- > 0)
                {
		    int c, n = exp_nibbles;
                    image_pnt    = image_row;                                   /* S001 vvvv */
                    do
                    {
                        bits = (unsigned int)(*image_pnt++)* pixb;
                        c = BITSWAP( *(tab + bits ));
			outw(BITBLT_IO_PORT_L, c >> 4); if( --n <= 0 ) break;
			outw(BITBLT_IO_PORT_L, c);      if( --n <= 0 ) break;
                        c = BITSWAP( *(tab + bits +1 ));
			outw(BITBLT_IO_PORT_L, c >> 4); if( --n <= 0 ) break;
			outw(BITBLT_IO_PORT_L, c);      if( --n <= 0 ) break; 
                        if( pixb < 3) continue;
                        c = BITSWAP( *(tab + bits +2 ));
			outw(BITBLT_IO_PORT_L, c >> 4); if( --n <= 0 ) break;
			outw(BITBLT_IO_PORT_L, c);          --n;
                    } while ( n );
                    image_row    += stride;                                     /* S001 ^^^^^ */
		}
        
                if( char_mask == 0x80 )
                {
		     dstAddr  += wdPriv->fontSprite;
                     char_mask = 1;
                }
                else 
                     char_mask <<= 1;
	} 
}

void
wdDrawFontText15_24(
	BoxPtr pbox,
	unsigned char *chars,
	unsigned int count,
	unsigned short width,
	int index,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	unsigned char transparent,
	DrawablePtr pDraw)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);

	unsigned int srcAddr, dstAddr;
	int height, control2, cnt;
        int font_base, char_mask;
        int pixb = wdPriv->pixBytes;
        unsigned char *pm, *fgb;
	unsigned char *char_pnt;
        void (* draw_solid )();

#ifdef DEBUG_PRINT
	ErrorF("DrawFontText15_24( count=%d,width=%d,index=%d,fg=%x,bg=%x,alu=%x,pmask=%x\n",
	                      count,width,index,fg,bg,alu,planemask);
#endif
        /* 
         * This routines draws the whole string 2 or 3 times with 1 byte offset
         * using the 1st 2nd or 3rd byte from planemask and fg
         * Therefore we can only use the transparent blitting 
         */
	if ( !transparent)           /* fill with background color */
        {
             switch( pDraw->depth )
             {
                 case 15: draw_solid = wdDrawSolidRects15;  break;
                 case 16: draw_solid = wdDrawSolidRects16;  break;
                 case 24: draw_solid = wdDrawSolidRects24;  break;
             }    
             (draw_solid)( pbox, 1, bg, alu, planemask, pDraw);
        }
	control2 = QUICK_START | UPDATE_DEST | MONO_TRANSP_ENAB ;

	/* 
	 * Work around apparent chip bug.
	 * During mono transparency operations,
	 * if the alu does not involve the source,
	 * the blt'er seems to lose track of the mono pattern.
	 * We get around this bug by adjusting the alu and fg
	 * to an equivalent operation that does involve the source.
	 */
	if(!rop_needs_src[alu]) {
		switch(alu) {
		case GXclear:	fg = 0;		alu = GXcopy;	break;
		case GXset:	fg = 0xFF;	alu = GXcopy;	break;
		case GXinvert:	fg = 0xFF;	alu = GXxor;	break;
		case GXnoop:	return;
		}
	}

	height = pbox->y2 - pbox->y1;
        dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1 * pixb;
        font_base = wdPriv->fontBase + index * wdPriv->fontSprite * WD_CACHE_LOCATIONS;

        WAITFOR_WD();

        WRITE_1_REG( CNTRL_2_IND    , control2 );
        WRITE_1_REG( DIM_X_IND      , width * pixb );
        WRITE_1_REG( DIM_Y_IND      , height );
        WRITE_1_REG( RASTEROP_IND   , alu << 8 );
        WRITE_1_REG( BACKGR_IND     , bg );
        WRITE_1_REG( TRANSPCOL_IND  , 0xFF );
        WRITE_1_REG( CNTRL_1_IND, PACKED_MODE | SRC_IS_MONO_COMP | SRC_LINEAR );

        pm  = (unsigned char *)&planemask; 
        fgb = (unsigned char *)&fg;
        while ( pixb-- ) 
        {
	     WAITFOR_WD();
             WRITE_2_REG( DEST_IND       , dstAddr );  dstAddr++;
             if( *pm != 0 )
             { 
                 WRITE_1_REG( FOREGR_IND     , *fgb );
                 WRITE_1_REG( PLANEMASK_IND  , *pm  );
                 char_pnt = chars;
                 cnt      = count;
	         while (cnt-- > 0) {
	             srcAddr = font_base + (*char_pnt >>3) * wdPriv->fontSprite;
                     /* dstAddr updated by WD90C31 */
                     char_mask = 1 << (*char_pnt++ & 7);
		     WAITFOR_WD();
                     WRITE_1_REG( TRANSPMASK_IND , char_mask );
		     WRITE_2_REG( SOURCE_IND     , srcAddr );
                 }
	     }
             fgb++; pm++;
        }
}
