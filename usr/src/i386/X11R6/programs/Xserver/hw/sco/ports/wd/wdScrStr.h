/*
 *  @(#) wdScrStr.h 11.1 97/10/22
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
 * wdScrStr.h - wd screen privates
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 31-Aug-1992	edb@sco.com
 *              copied fr. R4 driver
 *	S001	Fri 09-Oct-1992	buckm@sco.com
 *		get rid of unused fbVirToPhys field in screen private.
 *      S002    Wdn 14-Oct-1992 edb@sco.com
 *              changes to implement WD90C31 cursor
 *	S003	Thu  29-Oct-1992	edb@sco.com
 *              GC caching by checking of serial # changes
 *      S004    Thu 05-Oct-1992 edb@sco.com
 *              data structure changes for tile and stipple caching
 *      S005    Thu 05-Nov-1992 edb@sco.com
 *              replaced macro NFB_GC_SERIAL_NUMBER
 *	S006	Mon 09-Nov-1992	edb@sco.com
 *              fixed argument type
 *	S007	Wdn 12-02-1992	edb@sco.com
 *              make CHECK_SERIAL a noop
 *	S008	Fri 12-Feb-1993	buckm@sco.com
 *		get rid of GC caching.
 *		add bankBase, fontBase, fillColor.
 *		move glyph cache structures to wdGlyph.c.
 *      S009    Thr Apr 15    edb@sco.com
 *              extend wdScrnPriv 
 *      S010    Fri 07-May-1993 edb@sco.com
 *              add start addr for solid color fill  in _wdScrnPriv
 *              add fg and bg for color expanded stipples in cache  in _wdScrnPriv
 */

#include "misc.h"
#include "cursor.h"

/*  wd cursor          */

typedef struct _wdCursor
     {
           int width, height;
           int patSize;
           unsigned char * patAddr;
     /*       ready to load into registers : */
           long  memAddr;
           short patType;
           short primColor;
           short secColor;
           short origin;
     } wdCursor;

typedef struct _wdOffScreenBlit {
           int x, y;
           int w, h;
     } wdOffScreenBlit;

typedef struct _wdScrnPriv {
	pointer		fbBase;		/* frame buffer base (virtual) */
	int		fbSize;		/* frame buffer size */
	int		fbStride;	/* frame buffer width in bytes */
	int		bankBase;	/* start addr of current bank */
	int             cursorCache;    /* cursorCache start addr            */
        int             cursCacheSize;
        wdCursor      * curCursor;      /* currently loaded cursor           */
        int             fillStart;      /* for fast fill  in 15 16 and 24    */    /* S010  */
        int             tileStart;      /* tile blit area start addr         */
        int             tilePrefStart;  /* fast tile blit area start addr    */
	int		tileType;	/* Pref or normal		     */
        int             tileMaxWidth;
        int             tileMaxHeight;
        int             tileSerial;     /* serial of loaded tile             */
	int		fillColor;	/* current 16/24-bit fill color	     */
        int             stipFillFg;     /* fg for color expanded stipple     */    /* S010  */
        int             stipFillBg;     /* bg for color expanded stipple     */    /* S010  */
	int		fontBase;	/* location of downloaded fonts	     */
        int             fontSprite;     /* nr bytes per character in cache   */
        int             maxFontsInCache; /* max nr of fonts to be cached     */   /* S009 vvv */
        int             pixBytes;       /* bytes per pixel                   */
        int             planeMaskMask;  /* used for 15 16 and 24 bit code    */
        char *          expandBuff;     /* buffer for 15 16 and 24 bit code  */
        char *          expandTab;      /* table for speeding up 15 16 24    */   /* S009 ^^^ */
        int             glyphCache;     /* glyph Cache start addr            */
        int             glyphCacheSize;
} wdScrnPriv, *wdScrnPrivPtr;

#define WD_SCREEN_PRIV(pscreen) ((wdScrnPrivPtr) \
			((pscreen)->devPrivates[wdScreenPrivateIndex].ptr))

extern unsigned int wdScreenPrivateIndex;

