/*
 *  @(#) wdTileStip.c 11.1 97/10/22
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
 * wdTileStip.c
 *               wd tile and stipple helpers
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *      S000    Tue 06-Mar-1993 buckm@sco.com
 *		move wdLoadTile and wdLoadStipple here.
 *              use frame buffer access to load tiles and stipples.
 *		add wdReplicateArea() and wdCoverArea().
 *		add wdLoadFillTile16 and wdLoadFillTile24().
 *      S001    Fri 07-May-1993 edb@sco.com
 *              use separate buffer for solid color fill
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


int
wdLoadTile(
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
	    tileh <= WD_TILE_HEIGHT && !(tileh & (tileh - 1)) )
	{
	    int (*pDst)[2];
	    int t0, t1;
	    int h = tileh;

	    WD_MAP_AHEAD(wdPriv, wdPriv->tilePrefStart, WD_TILE_SIZE, pDst);
	    WAITFOR_WD();
	    while (--h >= 0)
	    {
		switch (tilew)
		{
		case 1:
		    t0 = *image;  t0 |= t1 << 8;  t0 |= t0 << 16;
		    t1 = t0;
		    break;
		case 2:
		    t0 = *(unsigned short *)image;  t0 |= t0 << 16;
		    t1 = t0;
		    break;
		case 4:
		    t0 = *(unsigned int *)image;
		    t1 = t0;
		    break;
		case 8:
		    t0 = ((unsigned int *)image)[0];
		    t1 = ((unsigned int *)image)[1];
		    break;
		}
		switch (tileh)
		{
		case 1: pDst[1][0] = t0;  pDst[1][1] = t1;
			pDst[3][0] = t0;  pDst[3][1] = t1;
			pDst[5][0] = t0;  pDst[5][1] = t1;
			pDst[7][0] = t0;  pDst[7][1] = t1;
		case 2: pDst[2][0] = t0;  pDst[2][1] = t1;
			pDst[6][0] = t0;  pDst[6][1] = t1;
		case 4: pDst[4][0] = t0;  pDst[4][1] = t1;
		case 8: pDst[0][0] = t0;  pDst[0][1] = t1;
		}
		image += stride;
		pDst += 1;
	    }

	    wdPriv->tileSerial = serialNr;
	    return( wdPriv->tileType = PREF_LOADED );
	}

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

unsigned int wdQuartetExpandTab[16] =
{
	0x00000000,	0x000000FF,	0x0000FF00,	0x0000FFFF,
	0x00FF0000,	0x00FF00FF,	0x00FFFF00,	0x00FFFFFF,
	0xFF000000,	0xFF0000FF,	0xFF00FF00,	0xFF00FFFF,
	0xFFFF0000,	0xFFFF00FF,	0xFFFFFF00,	0xFFFFFFFF
};

int
wdLoadStipple(
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
	    stiph <= WD_TILE_HEIGHT && !(stiph & (stiph - 1)) )
	{
	    int (*pDst)[2];
	    int t0, t1;
	    int c, h = stiph;

	    WD_MAP_AHEAD(wdPriv, wdPriv->tilePrefStart, WD_TILE_SIZE, pDst);
	    WAITFOR_WD();
	    while (--h >= 0)
	    {
		switch (stipw)
		{
		case 1:
		    t0 = (*image & 1) ? 0xFFFFFFFF : 0x00000000;
		    t1 = t0;
		    break;
		case 2:
		    t0 = wdQuartetExpandTab[*image & 0x3];  t0 |= t0 << 16;
		    t1 = t0;
		    break;
		case 4:
		    t0 = wdQuartetExpandTab[*image & 0xF];
		    t1 = t0;
		    break;
		case 8:
		    c = *image;
		    t0 = wdQuartetExpandTab[c & 0xF];
		    t1 = wdQuartetExpandTab[c >> 4];
		    break;
		}
		switch (stiph)
		{
		case 1: pDst[1][0] = t0;  pDst[1][1] = t1;
			pDst[3][0] = t0;  pDst[3][1] = t1;
			pDst[5][0] = t0;  pDst[5][1] = t1;
			pDst[7][0] = t0;  pDst[7][1] = t1;
		case 2: pDst[2][0] = t0;  pDst[2][1] = t1;
			pDst[6][0] = t0;  pDst[6][1] = t1;
		case 4: pDst[4][0] = t0;  pDst[4][1] = t1;
		case 8: pDst[0][0] = t0;  pDst[0][1] = t1;
		}
		image += stride;
		pDst += 1;
	    }

	    wdPriv->tileSerial = serialNr;
	    return( wdPriv->tileType = PREF_LOADED );
	}

	if( stipw <= wdPriv->tileMaxWidth && stiph <= wdPriv->tileMaxHeight )
	{
	    int pad, nibbles;
	    int h = stiph;

	    pad = stride - ((stipw + 7) >> 3);
	    nibbles = (stipw + 3) >> 2;

	    WAITFOR_WD();

	    WRITE_1_REG( CNTRL_2_IND   , ALL_DEFAULTS );
	    WRITE_2_REG( SOURCE_IND    , 0 );
	    WRITE_2_REG( DEST_IND      , wdPriv->tileStart );
	    WRITE_1_REG( DIM_X_IND     , stipw );
	    WRITE_1_REG( DIM_Y_IND     , stiph );
	    WRITE_1_REG( RASTEROP_IND  , GXcopy << 8 );
	    WRITE_1_REG( FOREGR_IND    , 0xFF );
	    WRITE_1_REG( BACKGR_IND    , 0x00 );
	    WRITE_1_REG( PLANEMASK_IND , 0xFF );

	    WRITE_1_REG( CNTRL_1_IND   , START | PACKED_MODE | 
					 SRC_IS_MONO | SRC_IS_IO_PORT );

	    do
	    {
		int c, n = nibbles;
		while ((n -= 2) >= 0)
		{
		    c = BITSWAP(*image++);
		    outw(BITBLT_IO_PORT_L, c >> 4);
		    outw(BITBLT_IO_PORT_L, c);
		}
		if (n & 1)
		{
		    c = BITSWAP(*image++);
		    outw(BITBLT_IO_PORT_L, c >> 4);
		}
		image += pad;
	    } while (--h > 0);

	    wdReplicateArea(wdPriv->tileStart, stipw << 1, stiph << 1,
			    stipw, stiph, wdPriv);

	    wdPriv->tileSerial = serialNr;
	    return( wdPriv->tileType = LOADED );
	}

	return( NOT_LOADED );
}

int
wdLoadFillTile16(
	struct _wdScrnPriv *wdPriv,
	unsigned long fg)
{
	unsigned int addr = wdPriv->fillStart;       /* S001 */

	if( wdPriv->fillColor != fg )
	{
	    int (*pDst)[2];
	    int f;

	    wdPriv->fillColor = fg;

	    WD_MAP_AHEAD(wdPriv, addr, WD_TILE_SIZE, pDst);
	    WAITFOR_WD();

	    f = fg | (fg << 16);
	    pDst[0][0] = f;  pDst[0][1] = f;
	    pDst[1][0] = f;  pDst[1][1] = f;
	    pDst[2][0] = f;  pDst[2][1] = f;
	    pDst[3][0] = f;  pDst[3][1] = f;
	    pDst[4][0] = f;  pDst[4][1] = f;
	    pDst[5][0] = f;  pDst[5][1] = f;
	    pDst[6][0] = f;  pDst[6][1] = f;
	    pDst[7][0] = f;  pDst[7][1] = f;
	}

	return( addr );
}

int
wdLoadFillTile24(
	struct _wdScrnPriv *wdPriv,
	unsigned long fg)
{
	unsigned int addr = wdPriv->fillStart;       /* S001 */

	if( wdPriv->fillColor != fg )
	{
	    int (*pDst)[2];
	    int f;

	    wdPriv->fillColor = fg;

	    WD_MAP_AHEAD(wdPriv, addr, WD_TILE_SIZE, pDst);
	    WAITFOR_WD();

	    f = fg | (fg << 24);
	    pDst[0][0] = f;
	    pDst[1][0] = f;
	    pDst[2][0] = f;
	    pDst[3][0] = f;
	    pDst[4][0] = f;
	    pDst[5][0] = f;
	    pDst[6][0] = f;
	    pDst[7][0] = f;

	    f = (fg >> 8) | (fg << 16);
	    pDst[0][1] = f;
	    pDst[1][1] = f;
	    pDst[2][1] = f;
	    pDst[3][1] = f;
	    pDst[4][1] = f;
	    pDst[5][1] = f;
	    pDst[6][1] = f;
	    pDst[7][1] = f;
	}

	return( addr );
}


wdReplicateArea(
	unsigned int srcAddr,
	unsigned int width,
	unsigned int height,
	unsigned int tilew,
	unsigned int tileh,
	register struct _wdScrnPriv *wdPriv)
{
	unsigned int dstAddr;
	unsigned int w, h;
	unsigned int n;

	if ( (w = tilew) < width )
	{
	    if ( tileh > height )
		tileh = height;
	    dstAddr = srcAddr + w;
	    if( (n = width - w) > w )
		n = w;

	    WAITFOR_WD();
	    WRITE_1_REG( CNTRL_2_IND , QUICK_START );
	    WRITE_1_REG( CNTRL_1_IND , PACKED_MODE );
	    WRITE_2_REG( SOURCE_IND  , srcAddr );
	    WRITE_1_REG( DIM_Y_IND   , tileh );
	    WRITE_1_REG( DIM_X_IND   , n );
	    WRITE_2_REG( DEST_IND    , dstAddr );

	    while( (w += n) < width )
	    {
		dstAddr += n;
		if( (n = width - w) > w )
		    n = w;
		WAITFOR_WD();
		WRITE_1_REG( DIM_X_IND , n );
		WRITE_2_REG( DEST_IND  , dstAddr );
	    }
	}

	if ( (h = tileh) < height )
	{
	    dstAddr = srcAddr + h * wdPriv->fbStride;
	    if( (n = height - h) > h )
		n = h;

	    WAITFOR_WD();
	    WRITE_1_REG( CNTRL_2_IND , QUICK_START );
	    WRITE_1_REG( CNTRL_1_IND , PACKED_MODE );
	    WRITE_2_REG( SOURCE_IND  , srcAddr );
	    WRITE_1_REG( DIM_X_IND   , width );
	    WRITE_1_REG( DIM_Y_IND   , n );
	    WRITE_2_REG( DEST_IND    , dstAddr );

	    while( (h += n) < height )
	    {
		dstAddr += n * wdPriv->fbStride;
		if( (n = height - h) > h )
		    n = h;
		WAITFOR_WD();
		WRITE_1_REG( DIM_Y_IND , n );
		WRITE_2_REG( DEST_IND  , dstAddr );
	    }
	}
}

wdCoverArea(
	unsigned int srcAddr,
	unsigned int srcWidth,
	unsigned int srcHeight,
	unsigned int dstAddr,
	unsigned int dstWidth,
	unsigned int dstHeight,
	register struct _wdScrnPriv *wdPriv)
{
	int w, h;
	int minw, minh;
	unsigned int dst;

	WAITFOR_WD();
	WRITE_2_REG( SOURCE_IND , srcAddr );

	h = dstHeight;
	do
	{
	    if( (minh = h) > srcHeight)
		minh = srcHeight;
	    w = dstWidth;
	    dst = dstAddr;
	    dstAddr += minh * wdPriv->fbStride;
	    do
	    {
		if( (minw = w) > srcWidth)
		    minw = srcWidth;
		WAITFOR_WD();
		WRITE_1_REG( DIM_X_IND , minw );
		WRITE_1_REG( DIM_Y_IND , minh );
		WRITE_2_REG( DEST_IND  , dst );
		dst += minw;
	    } while( (w -= minw) > 0 );
	} while( (h -= minh) > 0 );
}
