/*
 *  @(#) wdFont.c 11.1 97/10/22
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
 * wdFont.c - routines for 8-bit terminal-emulator fonts.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Mar 02 21:47:21 PST 1993	buckm@sco.com
 *	- Created.
 *      S001    Thr Apr 15    edb@sco.com
 *      - change storage allocation for cached fonts in order to allow a
 *        variable number of cached fonts
 *      S002    Tue 25-May-93 edb@sco.com
 *      - rename wdDrawFontText to wdDrawFontText8
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

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdBitswap.h"


void
wdDownloadFont8(
	unsigned char **ppbits,
	int count,
	int width,
	int height,
	int stride,
	int index,
	ScreenPtr pScreen)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pScreen);
	unsigned char *image;
	int pad, nibbles, dstAddr;
        int char_mask;
        int nr =0,addr;

#ifdef DEBUG_PRINT
	ErrorF("Load Font8 #%d %dx%d by %d\n", index, width, height, count);
#endif

	pad = stride - ((width + 7) >> 3);
	nibbles = (width + 3) >> 2;
        dstAddr = wdPriv->fontBase + index * wdPriv->fontSprite * WD_CACHE_LOCATIONS;  /* S001 */
        char_mask = 0x01;  /* S001 */

        WAITFOR_WD();

        WRITE_1_REG( CNTRL_2_IND   , QUICK_START );
	WRITE_2_REG( SOURCE_IND    , 0 );
        WRITE_1_REG( DIM_X_IND     , width );
        WRITE_1_REG( DIM_Y_IND     , height );
        WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
        WRITE_1_REG( FOREGR_IND    , 0xFF );
        WRITE_1_REG( BACKGR_IND    , 0x00 );

        WRITE_1_REG( CNTRL_1_IND   , PACKED_MODE | DST_LINEAR |
				     SRC_IS_MONO | SRC_IS_IO_PORT );
        /* The WDC31 wants 4 bits units padded by 12 bits */

	do {                          /* character loop */
		int h = height;
		image = *ppbits++;
                WRITE_1_REG( PLANEMASK_IND , char_mask );
                WRITE_2_REG( DEST_IND , dstAddr );

		do {
			int c, n = nibbles;
			while ((n -= 2) >= 0) {
				c = BITSWAP(*image++);
				outw(BITBLT_IO_PORT_L, c >> 4);
				outw(BITBLT_IO_PORT_L, c);
			}
			if (n & 1) {
				c = BITSWAP(*image++);
				outw(BITBLT_IO_PORT_L, c >> 4);
			}
			image += pad;
		} while (--h > 0);
                if( char_mask == 0x80 )                   /* S001 vvvvv */
                {
                     char_mask = 1;
                     dstAddr  += wdPriv->fontSprite;
                }
                else
                     char_mask <<= 1;                     /* S001 ^^^^^ */
	} while (--count > 0);
}

void
wdClearFont8(
	int index,
	ScreenPtr pScreen)
{
#ifdef DEBUG_PRINT
	ErrorF("Clear Font8 #%d\n", index);
#endif
}

void
wdDrawFontText8(
	BoxPtr pbox,
	unsigned char *chars,
	unsigned int count,
	unsigned short width,
	int index,
	unsigned long fg,
	unsigned long bg,
	unsigned char alu,
	unsigned long planemask,
	Bool transparent,
	DrawablePtr pDraw)
{
        wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV(pDraw->pScreen);
	unsigned int srcAddr, dstAddr;
	int height, control2;
        int font_base, char_mask;

#ifdef DEBUG_PRINT
	ErrorF("DrawFontText8( count=%d,width=%d,index=%d,fg=%x,bg=%x,alu=%x,pmask=%x\n",
	                      count,width,index,fg,bg,alu,planemask);
#endif
	height = pbox->y2 - pbox->y1;
        dstAddr = pbox->y1 * wdPriv->fbStride + pbox->x1;

	control2 = QUICK_START | UPDATE_DEST;

	if (transparent) {
		control2 |= MONO_TRANSP_ENAB;
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
	}

        font_base = wdPriv->fontBase + index * wdPriv->fontSprite * WD_CACHE_LOCATIONS;  /* S001 */

        WAITFOR_WD();

        WRITE_1_REG( CNTRL_2_IND    , control2 );
        WRITE_2_REG( DEST_IND       , dstAddr );
        WRITE_1_REG( DIM_X_IND      , width );
        WRITE_1_REG( DIM_Y_IND      , height );
        WRITE_1_REG( RASTEROP_IND   , alu << 8 );
        WRITE_1_REG( FOREGR_IND     , fg );
        WRITE_1_REG( BACKGR_IND     , bg );
        WRITE_1_REG( TRANSPCOL_IND  , 0xFF );
        WRITE_1_REG( PLANEMASK_IND  , planemask & 0xFF );
        WRITE_1_REG( CNTRL_1_IND, PACKED_MODE | SRC_IS_MONO_COMP | SRC_LINEAR );

	while (count-- > 0) {
                srcAddr = font_base + (*chars >>3) * wdPriv->fontSprite;  /* S001 */
                /* dstAddr updated by WD90C31 */
                char_mask = 1 << (*chars++ & 7);  /* S001 */
                WAITFOR_WD();
                WRITE_1_REG( TRANSPMASK_IND , char_mask );
                WRITE_2_REG( SOURCE_IND     , srcAddr );
	}
}
