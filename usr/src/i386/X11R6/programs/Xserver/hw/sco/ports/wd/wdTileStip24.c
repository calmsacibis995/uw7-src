/*
 * @(#) wdTileStip24.c 11.1 97/10/22
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
 * wdTileStip15_24.c
 *               wd tile and stipple helpers
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *      S000    Fri 16-Mar-93 edb@sco.com
 *		Created from wdTileStip.c
 *      S001    Fri 07-May-1993 edb@sco.com
 *              for color expanded stipples need to verify fg and bg
 *              ifdef debug code
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdBitswap.h"
#include "wdBankMap.h"

#ifdef NOT_YET

int
wdLoadTile15_24(
	unsigned char *image,
	unsigned int  stride,
	unsigned int  tilew,
	unsigned int  tileh,
	register struct _wdScrnPriv *wdPriv,
	unsigned long serialNr)
{

	if( wdPriv->tileSerial && (wdPriv->tileSerial == serialNr) )
		return( wdPriv->tileType );


	if( tilew <= wdPriv->tileMaxWidth && tileh <= wdPriv->tileMaxHeight )
	{
	    char *pDst;
	    int h = tileh;

	    WD_MAP_AHEAD(wdPriv, wdPriv->tileStart, tilew, pDst);
	    WAITFOR_WD();
	    memcpy(pDst, image, tilew);
	    while (--h > 0)
	    {
		image += stride;
		pDst += wdPriv->fbStride;
		WD_REMAP_AHEAD(wdPriv, pDst, tilew);
		memcpy(pDst, image, tilew);
	    }
	    WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
	    WRITE_1_REG( PLANEMASK_IND , 0xFF );
	    wdReplicateArea(wdPriv->tileStart, tilew << 1, tileh << 1,
			    tilew, tileh, wdPriv);

	    wdPriv->tileSerial = serialNr;
	    return( wdPriv->tileType = LOADED );
	}

	return( NOT_LOADED );
}

#endif

/*
 *   Color expand stipple and load into offscreen memory
 *     To be used for drawing opaque stipples
 */
int
wdLoadStipple15_24Exp(
	unsigned char *image,
	unsigned int  stride,
	unsigned int  stipw,
	unsigned int  stiph,
	unsigned int  fg,
	unsigned int  bg,
	register struct _wdScrnPriv *wdPriv,
	unsigned long serialNr)
{
        int pixb = wdPriv->pixBytes;
        int tilew, tileh;

#ifdef DEBUG_PRINT
        ErrorF( "LoadStipple15_24Exp , stipw=%d, stiph=%d\n", stipw, stiph );
#endif

	if( wdPriv->tileSerial && (wdPriv->tileSerial == serialNr) &&
            wdPriv->stipFillFg == fg  && wdPriv->stipFillBg == bg )       /* S001 */
		return( wdPriv->tileType );

        tilew = stipw * pixb;
        tileh = stiph;
	if( tilew <= wdPriv->tileMaxWidth && tileh <= wdPriv->tileMaxHeight )
	{
	    char *pDst;
	    int h = tileh;
            int w;
            register unsigned char *dst; 
            register unsigned char *src; 
            register unsigned char msk;
            unsigned int pixel;
            unsigned char *pix0 = (unsigned char *)&pixel;
            unsigned char *pix1 = pix0 +1;
            unsigned char *pix2 = pix1 +1;

	    WD_MAP_AHEAD(wdPriv, wdPriv->tileStart, tilew, pDst);
	    WAITFOR_WD();
	    do
	    {
                w = tilew;
                src = image;
                dst = pDst;
                msk = 1;
                while( w-- )
                {
                      pixel = ( *src & msk ) ? fg : bg; 
                      *dst++ = *pix0;
                      *dst++ = *pix1;
                      if( pixb >= 3) *dst++ = *pix2;
                      if( (msk <<= 1) == 0 )
                      {
                          msk = 1; src++;
                      }
                }
                if( --h <= 0 ) break;
		image += stride;
		pDst += wdPriv->fbStride;
		WD_REMAP_AHEAD(wdPriv, pDst, tilew);
	    } while( 1 );

	    WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
	    WRITE_1_REG( PLANEMASK_IND , 0xFF );
	    wdReplicateArea(wdPriv->tileStart, tilew << 1, tileh << 1,
			    tilew, tileh, wdPriv);

	    wdPriv->tileSerial = serialNr;
            wdPriv->stipFillFg = fg;       /* S001 */
            wdPriv->stipFillBg = bg;       /* S001 */
#ifdef DEBUG_PRINT
            ErrorF("LOADED\n");
#endif
	    return( wdPriv->tileType = LOADED );
	}

#ifdef DEBUG_PRINT
        ErrorF("NOT LOADED\n");
#endif
	return( NOT_LOADED );
}


int
wdLoadStipple15_24(
	unsigned char *image,
	unsigned int  stride,
	unsigned int  stipw,
	unsigned int  stiph,
	struct _wdScrnPriv *wdPriv,
	unsigned long serialNr)
{
        unsigned char *image_pnt, *image_row;
        unsigned char * tab = wdPriv->expandTab;
        int pixb = wdPriv->pixBytes;
        unsigned int bits;

#ifdef DEBUG_PRINT
        ErrorF( "LoadStipple15_24 , stipw=%d, stiph=%d\n", stipw, stiph );
#endif

	if( wdPriv->tileSerial && (wdPriv->tileSerial == serialNr) )
		return( wdPriv->tileType );

	if( stipw * pixb <= wdPriv->tileMaxWidth && stiph <= wdPriv->tileMaxHeight )
	{
	    int n, exp_nibbles;
	    int h = stiph;

            exp_nibbles = (stipw * pixb +3 ) >> 2;

	    WAITFOR_WD();

	    WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
	    WRITE_2_REG( SOURCE_IND    , 0 );
	    WRITE_2_REG( DEST_IND      , wdPriv->tileStart );
	    WRITE_1_REG( DIM_X_IND     , stipw *pixb );
	    WRITE_1_REG( DIM_Y_IND     , stiph );
	    WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
	    WRITE_1_REG( FOREGR_IND    , 0xFF );
	    WRITE_1_REG( BACKGR_IND    , 0x00 );
	    WRITE_1_REG( PLANEMASK_IND , 0xFF );

	    WRITE_1_REG( CNTRL_1_IND   , START | PACKED_MODE | 
					 SRC_IS_MONO | SRC_IS_IO_PORT );

            image_row    =  image;
	    while ( h-- > 0)
            {
		int c, n = exp_nibbles;
                image_pnt = image_row;
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
                image_row += stride;
	    }

	    wdReplicateArea(wdPriv->tileStart, (stipw * pixb) << 1, stiph << 1,
			    stipw * pixb, stiph, wdPriv);

	    wdPriv->tileSerial = serialNr;
#ifdef DEBUG_PRINT
            ErrorF("LOADED\n");
#endif
	    return( wdPriv->tileType = LOADED );
	}

#ifdef DEBUG_PRINT
        ErrorF("NOT LOADED\n");
#endif
	return( NOT_LOADED );
}

#ifdef DEBUG_PRINT                     /* S001 vvvvvvv */

printCache( wdPriv,tilew,tileh, msk)
struct _wdScrnPriv *wdPriv;
int tilew,tileh;
char msk;
{
            int w,h;
            char * pDst;
            char * dst;
            ErrorF("Stipple in cache w=%d h=%d\n");
	    WD_MAP_AHEAD(wdPriv, wdPriv->tileStart, tilew, pDst);
	    WAITFOR_WD();
	    do
	    {
                ErrorF("    ");
                w = tilew;
                dst = pDst;
                while( w-- )
                {
                      ErrorF( " %c",(*dst++ & msk) ? '*' : '_' );
                      ErrorF( "%c",(*dst++ & msk) ? '*' : '_' );
                      if( wdPriv->pixBytes < 3) continue;
                      ErrorF( "%c",(*dst++ & msk) ? '*' : '_' );
                }
                ErrorF("\n");
                if( --tileh <= 0 ) break;
		pDst += wdPriv->fbStride;
		WD_REMAP_AHEAD(wdPriv, pDst, tilew);
	    } while( 1 );
}

#endif
