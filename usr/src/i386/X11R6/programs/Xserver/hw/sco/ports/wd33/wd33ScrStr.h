/*
 *  @(#) wd33ScrStr.h 11.1 97/10/22
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
 * wd33ScrStr.h - wd33 screen privates
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *      S001    Thu 18-Aug-1993 edb@sco.com
 *		remove a few obsolete items in wdScrPrivStr
 */

#include "misc.h"
#include "cursor.h"

/*  wd33 cursor          */

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

/*
 *     Glyph caching
 */
typedef struct _glMemSeg
    {                                   
        struct _glMemSeg *next;         /* if free its a pointer in freelist */
                                        /* else    pointer to next in hash list */
        int fontId;
        int glyphId;
        int xCache;                     /* x coordinate in cache              */
        int yCache;                     /* y coordinate in cache              */
        unsigned char  planeNr;         /* plane in cache                     */
    } glMemSeg;

typedef struct _glPlane
    {
        int      maxWidth;              /* max w   of character               */
        int      maxHeight;             /* max w   of character               */
        glMemSeg *memSegs;              /* NULL ... plane not allocated       */
        glMemSeg *freeList;             /* NULL ... plane full                */
        int      nrMemSegs;
    } glPlane;

/*
 *   private Screen structure
 */
typedef struct _wdScrnPriv {
	pointer		fbBase;		/* frame buffer base (virtual)       */
	int		fbSize;		/* frame buffer size                 */
	int		fbStride;	/* frame buffer width in bytes       */
        int             rowPitch;       /* used for drawing engine 2 reg     */
        int             bit11_10;       /* bits 11:10 for engine ctrl reg 2  */
	int		bankBase;	/* start addr of current bank        */
        int             cursorCache;    /* cursorCache start addr            */
        int             cursCacheSize;
        wdCursor      * curCursor;      /* currently loaded cursor           */
        int             fillX;          /* for fast fill  , left X           */
        int             fillY;          /* for fast fill  , upper Y          */
        int             tileX;          /* cache for fast tile blit, left X  */
        int             tileY;          /* cache for fast tile blit, upper Y */
        int             tilePrefX;      /* for 8x8 tile cache only , left X  */
        int             tilePrefY;      /*   upper Y ( stored linerar ! )    */
	int		tileType;	/* Pref or normal		     */
        int             tileMaxWidth;
        int             tileMaxHeight;
        int             tileSerial;     /* serial of loaded tile             */
        int             pixBytes;       /* bytes per pixel                   */
        int             planeMaskMask;  /* used for 15 16 and 24 bit code    */
        int             glCacheY;       /* glyph cache top y coordinate      */
        int             glCacheHeight;  /* glyph cache total height          */
        int             glCacheWidth;   /* glyph cache total width           */
        glPlane       * glPlaneList;    /* glyph cache planes                */
        glMemSeg      ** glHashTab;     /* glyph cache hash table            */
        int             curRegBlock;    /* current register block            */
} wdScrnPriv, *wdScrnPrivPtr;

#define WD_SCREEN_PRIV(pscreen) ((wdScrnPrivPtr) \
			((pscreen)->devPrivates[wd33ScreenPrivateIndex].ptr))

extern unsigned int wd33ScreenPrivateIndex;
