/*
 *  @(#) wd33TileStip.c 11.1 97/10/22
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
 * wd33TileStip.c
 *               wd33 tile and stipple helpers
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *      S001    Thu 18-Aug-1993 edb@sco.com
 *		Buffer overflow caused dropping of register setting
 *	S002	Mon 12-Aug-1993		edb@sco.com
 *              wd33CoverArea needs argument src_is_mono
 *	S003	Thu 26-Aug-1993		edb@sco.com
 *		Fix bug in wd33CoverArea
 *	S004	Tue 12-Oct-1993		buckm@sco.com
 *		Stop over-writing ENG_1,CNTRL_2_IND in wd33CoverArea();
 *		it has been set correctly before arrival.
 */

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wdBitswap.h"
#include "wdBankMap.h"


int
wd33LoadTile(
	unsigned char *image,
	unsigned int  stride,
	unsigned int  tilew,
	unsigned int  tileh,
	register struct _wdScrnPriv *wdPriv,
	unsigned long serialNr)
{
	if( wdPriv->tileSerial && (wdPriv->tileSerial == serialNr) )
		return( wdPriv->tileType );

	if( tilew <= WD_TILE_WIDTH  && !(tilew & (tilew - 1)) &&
	    tileh <= WD_TILE_HEIGHT && !(tileh & (tileh - 1)))
	{
            unsigned char *img;
            register unsigned char  *src,    *pDst;
            register unsigned short *src_sh, *pDst_sh;
            int dstCountX, dstCountY;
            register int sx, dx;
            int sy, dy;
            int tilePrefStart = wdPriv->tilePrefY * wdPriv->fbStride +
                                wdPriv->tilePrefX * wdPriv->pixBytes;

	    WAITFOR_DE();
	    WD_MAP_AHEAD(wdPriv, tilePrefStart, WD_TILE_SIZE * wdPriv->pixBytes, pDst);
            pDst_sh = (unsigned short *)pDst;

            dstCountX = WD_TILE_WIDTH / tilew;
            dstCountY = WD_TILE_HEIGHT/ tileh;

            while( dstCountY-- )
            {
                img = image;
                sy = tileh;
                while( sy-- )
                {
                     dx = dstCountX;
                     if( wdPriv->pixBytes == 1) 
                     {
                          while( dx-- )
                          {
                               src = img;
                               sx = tilew;
                               while( sx-- )
                                   *pDst++ = *src++;
                          }
                     }
                     else 
                     {
                          while( dx-- )
                          {
                               src_sh = (unsigned short *)img;
                               sx = tilew;
                               while( sx-- )
                                   *pDst_sh++ = *src_sh++;
                          }
                     }
                     img += stride;
                }
            }
	    wdPriv->tileSerial = serialNr;
	    return( wdPriv->tileType = PREF_LOADED );
	}

	if( tilew <= wdPriv->tileMaxWidth && tileh <= wdPriv->tileMaxHeight )
	{
            register int curBlock = wdPriv->curRegBlock;
	    char *pDst;
	    int h = tileh;
            int tilewB = tilew * wdPriv->pixBytes;
            int tileStart = wdPriv->tileY * wdPriv->fbStride +
                            wdPriv->tileX * wdPriv->pixBytes;

	    WAITFOR_DE();
	    WD_MAP_AHEAD(wdPriv, tileStart, tilewB, pDst);
	    memcpy(pDst, image, tilewB);
	    while (--h > 0)
	    {
		image += stride;
		pDst += wdPriv->fbStride;
		WD_REMAP_AHEAD(wdPriv, pDst, tilewB);
		memcpy(pDst, image, tilewB);
	    }
	    WAITFOR_BUFF( 4 );
	    WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , 0xFFFF );
	    WRITE_REG( ENG_1, RASTEROP_IND    , GXcopy << 8 );
	    wd33ReplicateArea(wdPriv->tileX, wdPriv->tileY, tilew << 1, tileh << 1,
			    tilew, tileh, wdPriv);

	    wdPriv->tileSerial = serialNr;
            wdPriv->curRegBlock = curBlock;
	    return( wdPriv->tileType = LOADED );
	}

	return( NOT_LOADED );
}

int
wd33LoadStipple(
	unsigned char *image,
	unsigned int  stride,
	unsigned int  stipw,
	unsigned int  stiph,
	struct _wdScrnPriv *wdPriv,
	unsigned long serialNr)
{

	if( wdPriv->tileSerial && (wdPriv->tileSerial == serialNr) )
		return( wdPriv->tileType );

	if( stipw <= WD_TILE_WIDTH  && !(stipw & (stipw - 1)) &&
	    stiph <= WD_TILE_HEIGHT && !(stiph & (stiph - 1))  )
	{
            unsigned char *img;
            register unsigned char  *pDst;
            register unsigned short *pDst_sh;
            register unsigned char srcByte, msk;
            int dstCountX, dstCountY;
            register int sx, dx;
            int sy, dy;
            int tilePrefStart = wdPriv->tilePrefY * wdPriv->fbStride +
                                wdPriv->tilePrefX * wdPriv->pixBytes;

	    WAITFOR_DE();
	    WD_MAP_AHEAD(wdPriv, tilePrefStart, WD_TILE_SIZE * wdPriv->pixBytes, pDst);
            pDst_sh = (unsigned short *)pDst;

            dstCountX = WD_TILE_SIZE / stipw;
            dstCountY = WD_TILE_SIZE / stiph;

            while( dstCountY-- )
            {
                img = image;
                sy = stiph;
                while( sy-- )
                {
                     srcByte = *img;
                     dx = dstCountX;
                     if( wdPriv->pixBytes == 1) 
                     {
                          while( dx-- )
                          {
                               sx = stipw;
                               msk = 1;
                               while( sx-- ) {
                                   *pDst++ = (srcByte & msk) ? 0xFF : 0;
                                   msk <<= 1;
                               }
                          }
                     }
                     else 
                     {
                          while( dx-- )
                          {
                               sx = stipw;
                               msk = 1;
                               while( sx-- ) {
                                   *pDst_sh++ = (srcByte & msk) ? 0xFFFF : 0;
                                   msk <<= 1;
                               }
                          }
                     }
                     img += stride;
                }
            }
	    wdPriv->tileSerial = serialNr;
	    return( wdPriv->tileType = PREF_LOADED );
	}
	if( stipw <= wdPriv->tileMaxWidth && stiph <= wdPriv->tileMaxHeight )
	{
	    int pad, nibbles;
	    int h = stiph;
            register int curBlock = wdPriv->curRegBlock;

	    pad = stride - ((stipw + 7) >> 3);
	    nibbles = (stipw + 3) >> 2;

            WAITFOR_BUFF( 8 );
            WRITE_2_REG( ENG_2, BACKGR_IND_0    , 0 );
            WRITE_2_REG( ENG_2, FOREGR_IND_0    , wdPriv->planeMaskMask );
            WRITE_2_REG( ENG_2, PLANEMASK_IND_0 , wdPriv->planeMaskMask );
            WRITE_REG( ENG_1, SOURCE_X      , 0 );
            WRITE_REG( ENG_1, SOURCE_Y      , 0 );

            WAITFOR_BUFF( 8 );
            WRITE_REG( ENG_1, DEST_X        , wdPriv->tileX );
            WRITE_REG( ENG_1, DEST_Y        , wdPriv->tileY );
            WRITE_REG( ENG_1, DIM_X_IND     , stipw - 1 );
            WRITE_REG( ENG_1, DIM_Y_IND     , stiph - 1 );
            WRITE_REG( ENG_1, RASTEROP_IND  , GXcopy << 8 );
            WRITE_REG( ENG_1, CNTRL_2_IND , wdPriv->bit11_10 | BIT_6_5
                                          | COLOR_EXPAND_4 );
            WRITE_REG( ENG_1, CNTRL_1_IND , BITBLIT | SRC_IS_IO_PORT | SRC_IS_MONO );

            /* The WDC31 wants 4 bits units padded by 12 bits */

            WAITFOR_BUFF( 8 );
	    do
	    {
		int c, n = nibbles;
		while ((n -= 2) >= 0)
		{
		    c = BITSWAP(*image++);
		    outw(BITBLT_IO_PORT_L0, c >> 4);
		    outw(BITBLT_IO_PORT_L0, c);
		}
		if (n & 1)
		{
		    c = BITSWAP(*image++);
		    outw(BITBLT_IO_PORT_L0, c >> 4);
		}
		image += pad;
	    } while (--h > 0);

	    wd33ReplicateArea(wdPriv->tileX, wdPriv->tileY, stipw << 1, stiph << 1,
			    stipw, stiph, wdPriv);

	    wdPriv->tileSerial = serialNr;
            wdPriv->curRegBlock = curBlock;
	    return( wdPriv->tileType = LOADED );
	}

	return( NOT_LOADED );
}

/*
 *   !!! We assume that register block 1 ( ENG_1 ) is loaded
 */

wd33ReplicateArea(
	unsigned int srcX,
	unsigned int srcY,
	unsigned int width,
	unsigned int height,
	unsigned int tilew,
	unsigned int tileh,
	register struct _wdScrnPriv *wdPriv)
{
	unsigned int dstX;
	unsigned int dstY;
	unsigned int w, h;
	unsigned int n;
        register int curBlock = 1;

        WAITFOR_BUFF( 4 );
        WRITE_REG( ENG_1,SOURCE_X    , srcX );
        WRITE_REG( ENG_1,SOURCE_Y    , srcY );
        WRITE_REG( ENG_1,CNTRL_2_IND , wdPriv->bit11_10 | BIT_6_5 );

	if ( (w = tilew) < width )
	{
	    if ( tileh > height )
		tileh = height;
            dstX    = srcX    + w;
            dstY    = srcY;
	    if( (n = width - w) > w )
		n = w;

	    WAITFOR_BUFF( 5 );
	    WRITE_REG( ENG_1,DIM_Y_IND   , tileh-1 );
	    WRITE_REG( ENG_1,DIM_X_IND   , n-1 );
	    WRITE_REG( ENG_1,DEST_X      , dstX );
	    WRITE_REG( ENG_1,DEST_Y      , dstY );
	    WRITE_REG( ENG_1,CNTRL_1_IND , BITBLIT );

	    while( (w += n) < width )
	    {
		dstX += n;
		if( (n = width - w) > w )
		    n = w;

	        WAITFOR_BUFF( 4 );
		WRITE_REG( ENG_1,DIM_X_IND   , n-1 );
		WRITE_REG( ENG_1,DEST_X      , dstX );
	        WRITE_REG( ENG_1,DEST_Y      , dstY );
	        WRITE_REG( ENG_1,CNTRL_1_IND , BITBLIT );
	    }
	}

	if ( (h = tileh) < height )
	{
            dstX = srcX;
	    dstY = srcY + h;
	    if( (n = height - h) > h )
		n = h;

	    WAITFOR_BUFF( 5 );
	    WRITE_REG( ENG_1,DIM_X_IND   , width-1 );
	    WRITE_REG( ENG_1,DIM_Y_IND   , n-1 );
	    WRITE_REG( ENG_1,DEST_X      , dstX );
	    WRITE_REG( ENG_1,DEST_Y      , dstY );
	    WRITE_REG( ENG_1,CNTRL_1_IND , BITBLIT );

	    while( (h += n) < height )
	    {
                dstY += n;
		if( (n = height - h) > h )
		    n = h;
	        WAITFOR_BUFF( 3 );
		WRITE_REG( ENG_1,DIM_Y_IND   , n-1 );
		WRITE_REG( ENG_1,DEST_Y      , dstY );
	        WRITE_REG( ENG_1,CNTRL_1_IND , BITBLIT );
	    }
	}
}

/*
 *   !!! We assume that register block 1 ( ENG_1 ) is loaded
 */

wd33CoverArea(
	unsigned int srcX,
        unsigned int srcY,
	unsigned int srcWidth,
	unsigned int srcHeight,
	unsigned int dstX,
        unsigned int dstY,
	unsigned int dstWidth,
	unsigned int dstHeight,
        Bool         src_is_mono,
	register struct _wdScrnPriv *wdPriv)
{
	int w, h;
	int minw, minh;
	unsigned int dst_x;
	unsigned int dst_y;
        unsigned short ctrl1Cmd;
        register int curBlock = ENG_1;

        WAITFOR_BUFF( 2 );
        WRITE_REG( ENG_1,SOURCE_X    , srcX );
        WRITE_REG( ENG_1,SOURCE_Y    , srcY );
        ctrl1Cmd = BITBLIT;
        if( src_is_mono ) 
             ctrl1Cmd |= SRC_IS_MONO_COMP;

	h = dstHeight;
	do
	{
	    if( (minh = h) > srcHeight)
		minh = srcHeight;
	    w = dstWidth;
            dst_x = dstX;
            dst_y = dstY;
            dstY += minh;
	    do
	    {
		if( (minw = w) > srcWidth)
		    minw = srcWidth;
		WAITFOR_BUFF( 5 );
		WRITE_REG( ENG_1, DIM_X_IND , minw-1 );
		WRITE_REG( ENG_1, DIM_Y_IND , minh-1 );
		WRITE_REG( ENG_1, DEST_X    , dst_x );
		WRITE_REG( ENG_1, DEST_Y    , dst_y );
	        WRITE_REG( ENG_1, CNTRL_1_IND , ctrl1Cmd );
		dst_x += minw;
	    } while( (w -= minw) > 0 );
	} while( (h -= minh) > 0 );
}
