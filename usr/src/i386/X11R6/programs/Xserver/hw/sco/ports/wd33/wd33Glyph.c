/*
 *  @(#) wd33Glyph.c 11.1 97/10/22
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
 *   wd33Glyph.c
 */
/*
 *
 *      SCO MODIFICATION HISTORY
 *
 *	S000	Mon 01-July-1993	edb@sco.com
 *              copied fr. wd driver and modified
 *	S001	Mon 12-Aug-1993		edb@sco.com
 *              Remove code used to workaround a chip bug in wd90c31
 *              ( problem for alus not needing the source )
 *	S002	Tue 17-Aug-1993	edb@sco.com
 *              Put hash coding macros into wd33Defs.h
 *      S003    Thu 18-Aug-1993 edb@sco.com
 *		Changed AllocGlyphMem to be used in wd33Font.c also
 *      S004    Tue 31-Aug-1993 edb@sco.com
 *		minor change related to fixing of the screen switch bug in wd33Font.c
 *	S005	Mon 13-Sep-1993 edb@sco.com
 *		Argument font_id dependent on #define agaII
 */
#include <stdio.h>

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"


#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbRop.h"
#include "nfb/nfbGlyph.h"

#include "wd33Defs.h"
#include "wd33ScrStr.h"
#include "wd33Procs.h"
#include "wdBitswap.h"


static glMemSeg *FindGlyph( wdScrnPrivPtr wdPriv,int font_id, int glyph_id );
static glMemSeg *AllocGlyph( wdScrnPrivPtr wdPriv,int font_id, int glyph_id, int w, int h);
static glPlane * AllocPlane( wdScrnPrivPtr wdPriv ,int w, int h, int nchar );

#define MARGIN   3  /* fonts within a margin of 3 pixels are allocated in same plane */
#define MINSIZE  6  /* characters smaller than that should be drawn with DrawMonoImage */


#ifdef CACHE_PRINT
static int cacheHits =0, cacheFails =0, loadFails =0;
static cacheLoops =0, cacheAccess =0;
#endif

int nextFontId = 1;

/* wd33DrawMonoGlyphs - Draws a number of glyphs defined by glyph_info.
 *            Checks whether glyph is already in cache .
 *                  ( Hash keys:  font_id and glyph_info->glyph_id )
 *            If yes it blits out of cache using fg color, alu and planemask
 *            If not it tries to find space in cache (offscreen memory) 
 *            and draws into cache as a linear pixmap
 *            If no space in cache it uses wdDrawMonoImage
 */

void
wd33DrawMonoGlyphs(
    nfbGlyphInfo *glyph_info,
    unsigned int nglyphs,
    unsigned long fg,
    unsigned char alu,
    unsigned long planemask,
#ifndef agaII
    nfbFontPSPtr pPS,               /* S005 */
#else
    int font_id,
#endif
    DrawablePtr pDraw)
{
	int i;
	int w, h;
	glMemSeg *memseg;
	BoxPtr gl_box;
#ifndef agaII
        int font_id = pPS->private.val;               /* S005 */
#endif
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        int pixb = wdPriv->pixBytes;

#ifdef DEBUG_PRINT
        ErrorF("wd33DrawMonoGlyphs( font_id=%d ,nglyphs=%d,fg=0x%x,alu=%d,planemask=0x%x, \n",
                font_id,nglyphs,fg,alu,planemask);
	fflush( stderr );
#endif

#ifndef agaII
        if( font_id <=0 ) 
            pPS->private.val = font_id = ++nextFontId;    /* S004 */
#endif

	assert( nglyphs );

	for (i = 0; i < nglyphs; ++i, ++glyph_info)
	{
	    gl_box = &(glyph_info->box);
	    w = gl_box->x2 - gl_box->x1;
	    h = gl_box->y2 - gl_box->y1;
	    assert( w > 0 && h > 0 );

            if(wdPriv->glCacheHeight == 0 || (w < MINSIZE && h < MINSIZE) ) 
            {
		wd33DrawMonoImage( gl_box, glyph_info->image, 0,
				 glyph_info->stride, fg, alu,
				 planemask, pDraw);
                continue;
            }

	    memseg = FindGlyph( wdPriv,font_id, glyph_info->glyph_id );

#ifdef CACHE_PRINT
	    if( memseg ) cacheHits++;
	    else         cacheFails++;
#endif
	    if( memseg == NULL )
	    {
		memseg = AllocGlyph( wdPriv,font_id, glyph_info->glyph_id, w, h);

		if( memseg )
		    wd33LoadGlyph( wdPriv,w, h, glyph_info->image, glyph_info->stride,
				 memseg->xCache, memseg->yCache, memseg->planeNr);
	    }
	    if( memseg )
	    {   
		wd33DrawGlyph(  gl_box, memseg->xCache, memseg->yCache, memseg->planeNr,
                                 fg, alu, planemask, pDraw);
	    }
	    else
	    {
		wd33DrawMonoImage( gl_box, glyph_info->image, 0,
				 glyph_info->stride, fg, alu,
				 planemask, pDraw);
#ifdef CACHE_PRINT
		loadFails++;
#endif
	    }

#ifdef CACHE_PRINT
	    if( cacheAccess % 100 == 0 ) printCache( wdPriv );
#endif
	}
}


/*
 *       GLYPH  CACHE  ROUTINES
 *       ----------------------
 */

/*
 *   wd33InitGlCache - Initialize hash table and freelist
 */

wd33InitGlCache( wdPriv )
	wdScrnPrivPtr wdPriv;
{
	int i;
        int nrPlanes = wdPriv->pixBytes << 3;
        glPlane * plane;
        
	wdPriv->glHashTab = 
            (glMemSeg **) xalloc( GL_HASH_TABLE_SIZE * sizeof(glMemSeg *));

	for( i = 0; i < GL_HASH_TABLE_SIZE; i++ )
	{
	    wdPriv->glHashTab[i] = NULL;
	}

        wdPriv->glPlaneList = plane = 
            (glPlane *)xalloc( nrPlanes * sizeof(glPlane));

        for( i = 0; i< nrPlanes; i++)
        {
            plane->memSegs   = NULL;
            plane->nrMemSegs = 0;
            plane->freeList  = NULL;
            plane++;
        }
            
#ifdef CACHE_PRINT
	cacheHits =0; cacheFails =0; loadFails =0;
	cacheLoops =0; cacheAccess =0;
	ErrorF(" GlyphCaching initialized\n");
#endif
}

/*
 * wd33DeallocGlCache - free all allocated memsegs
 */

wd33DeallocGlCache( wdPriv )
	wdScrnPrivPtr wdPriv;
{
	int i;
        int nrPlanes = wdPriv->pixBytes << 3;
        glPlane * plane;

	xfree( wdPriv->glHashTab );

        plane = wdPriv->glPlaneList;
	for( i = 0; i< nrPlanes; i++,plane++)
            if( plane->memSegs != NULL )
		xfree( plane->memSegs );
	
        xfree( wdPriv->glPlaneList );
}

/*
 *  FindGlyph - Find [font_id,glyph_id] in hash table
 *                  and return memseg or NULL
 */

static glMemSeg *
FindGlyph( wdPriv,font_id, glyph_id )
        wdScrnPrivPtr wdPriv;
	int font_id;
	int glyph_id;
{
	int index;
	glMemSeg *memseg;

#ifdef CACHE_PRINT
	cacheAccess += 1;
#endif
	index  = GL_HASH( font_id, glyph_id );
	memseg = wdPriv->glHashTab[index];

	while( memseg )
	{
#ifdef CACHE_PRINT
	    cacheLoops += 1; 
#endif
	    if( memseg->fontId == font_id && memseg->glyphId == glyph_id )
		return ( memseg );
	    memseg = memseg->next;
	}
	return NULL;
}

/*
 *  AllocGlyph -  Add glyph entry in hash table,
 *                  and allocate space in off screen video mem.
 *                  Return memseg, or NULL if not enough space.
 */

static glMemSeg *
AllocGlyph( wdPriv,font_id, glyph_id, w, h )
        wdScrnPrivPtr wdPriv;
	int font_id;
	int glyph_id;
	int w, h;
{
	int index;
	glMemSeg *memseg;

        if( w < h ) w = h;  /* don't get an odd size plane */
        else        h = w;

	if( memseg = wd33AllocGlyphMem( wdPriv, w, h, 1 ) )
	{
	    index = GL_HASH( font_id, glyph_id );

	    memseg->fontId  = font_id;
	    memseg->glyphId = glyph_id;
	    memseg->next    = wdPriv->glHashTab[index];

	    wdPriv->glHashTab[index] = memseg;
	}
	return ( memseg );
}

/*
 *  wd33ClearFont  - Remove all glyph memsegs with ->fontId == font_id
 *                 from the hash lists and free them.
 */

void
wd33ClearFont(
        nfbFontPSPtr pPS, 
	ScreenPtr pScreen )
{ 
	int i, nseg;
	glMemSeg *memseg, *prev_memseg, *next_memseg;
        glPlane  *plane;
        int font_id = pPS->private.val;
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );
        int nrPlanes = wdPriv->pixBytes << 3;

        if(wdPriv->glCacheHeight == 0 ) return;

#ifdef CACHE_PRINT
	ErrorF("wd33ClearFont() font_id=%d)\n", font_id);
#endif
	for( i = 0; i < GL_HASH_TABLE_SIZE; i++ )
	{
            prev_memseg = NULL;
	    memseg = wdPriv->glHashTab[i];
	    while ( memseg )
	    {
                next_memseg = memseg->next;
		if( memseg->fontId == font_id )
		{
                    /* remove from hash list       */
                    if( prev_memseg != NULL )
		         prev_memseg->next = next_memseg;
                    else
                         wdPriv->glHashTab[i] = next_memseg;

                    /* insert into planes freelist */
                    plane = wdPriv->glPlaneList + memseg->planeNr;
                    memseg->next = plane->freeList;
                    plane->freeList = memseg;
                    memseg->fontId = 0;
		}
		else
		    prev_memseg = memseg;
                memseg = next_memseg;
	    }
	}

	/*
	 * check whether a plane is completely empty
         * if yes then scratch its memsegs
	 */
        plane = wdPriv->glPlaneList;
        for( i=0; i < nrPlanes; i++, plane++)
        {
             memseg = plane->freeList;
             nseg = 0;
             while( memseg ) 
             {
                nseg++;
                memseg = memseg->next;
             }
             if( plane->memSegs != NULL && nseg == plane->nrMemSegs )
             {
                xfree( plane->memSegs );
                plane->memSegs = NULL;
                plane->freeList = NULL;
                plane->nrMemSegs = 0;
             }
        }

#ifdef CACHE_PRINT
	printCache( wdPriv );
#endif
}

/*
 *  wd33AllocGlyphMem - Allocate space in offscreen video memory
 *                      go through array glPlanes and find a fitting w,h pair.
 *                      We allow a glyph to be 0 to 3 pixles smaller than
 *                      the max size in this plane.
 *                      If not found find free plane and initialize with w,h
 *                      if no free glPlane find first one which fits w,h and has 
 *                      space in freelist.
 */

glMemSeg *
wd33AllocGlyphMem(  wdPriv, w, h, nchar )
wdScrnPrivPtr wdPriv;
int w,h;
int nchar;
{
        int i,nrPlanes;
        int count;
        int x,y;
	glMemSeg *memseg;
	glMemSeg *memseg_prev = NULL;
	glMemSeg *memseg_last;
        glPlane  *plane;
        glPlane  *plane_fit = NULL;
        glPlane  *plane_ok  = NULL;

#ifdef DEBUG_PRINT
        ErrorF("wd33AllocGlyphMem( , w=%d, h=%d, nchar=%d )\n",w,h,nchar);
#endif
        if( h > wdPriv->glCacheHeight )
                return( NULL );

        plane      = wdPriv->glPlaneList;
        nrPlanes   = wdPriv->pixBytes << 3;
        /*
         * find a plane with size close to maxSize
         */
        for( i=0; i < nrPlanes; i++, plane++ )
             if( plane->memSegs  != NULL  &&
                 plane->freeList != NULL  &&
                 w <= plane->maxWidth     && 
                 h <= plane->maxHeight  )
             { 
                 plane_ok = plane; 
                 if( w >= plane->maxWidth - MARGIN && 
                     h >= plane->maxHeight- MARGIN ) 
                      plane_fit = plane;
             }

        plane = plane_fit;
        if( plane == NULL ) plane = AllocPlane( wdPriv, w, h, nchar );
        if( plane == NULL ) plane = plane_ok;
        if( plane == NULL ) return ( NULL );
                 
        /*
         * Find a number of nchar consecutive memsegs
         * take out of freelist and return
         */
        memseg = memseg_last = plane->freeList;
        count = 1;
        while( memseg_last )
        {
             if( count == nchar ) {
                  if( memseg_prev ) memseg_prev->next = memseg_last->next;
                  else              plane->freeList   = memseg_last->next;
                  memseg_last->next = 0;
                  return( memseg );
             }
             if( memseg_last->next != memseg_last + 1) {
                  count = 1;
                  memseg_prev = memseg_last;
                  memseg = memseg_last->next;
             }
             else
                  count++;
             memseg_last = memseg_last->next;
        }
        return( NULL );
}

/*
 *    Find an empty plane in glyph cache and initialize
 */

static glPlane * AllocPlane( wdPriv ,w, h, nchar )
wdScrnPrivPtr wdPriv;
int w, h;
int nchar;
{
        int i, x,y;
        int nrPlanes;
	glMemSeg *memseg;
        glPlane  *plane;
        int maxchar;

        maxchar = ( wdPriv->glCacheWidth / w) *
                  ( wdPriv->glCacheHeight/ h);
        if( maxchar < nchar )
             return NULL;

        plane      = wdPriv->glPlaneList;
        nrPlanes   = wdPriv->pixBytes << 3;
        for( i=0; i < nrPlanes; i++, plane++)
             if( plane->memSegs == NULL)
             {
                  plane->nrMemSegs = maxchar;
                  plane->memSegs   = memseg  = (glMemSeg *)xalloc( 
                                     plane->nrMemSegs * sizeof( glMemSeg));
                  plane->maxWidth  = w;
                  plane->maxHeight = h;
                  plane->freeList  = memseg;

                  y = 0;
                  while( y+h <= wdPriv->glCacheHeight )
                  {
                       x = 0;
                       while( x+w <= wdPriv->glCacheWidth )
                       {
                            memseg->xCache = x;
                            memseg->yCache = y + wdPriv->glCacheY ;
                            memseg->planeNr  = i ;       
                            memseg->next   = memseg+1;
                            memseg++;
                            x += w;
                       }
                       y += h;
                  }
                  memseg--;
                  memseg->next = NULL;
#ifdef CACHE_PRINT
                  ErrorF(" New plane nr=%d allocated, %d locations \n",
                            i,plane->nrMemSegs );
#endif
                  return( plane);
             }
        return( NULL );
}
#ifdef CACHE_PRINT

printCache( wdPriv )
wdScrnPrivPtr wdPriv;
{
        int i;
        int nrPlanes = wdPriv->pixBytes << 3;
        glPlane *plane;

	int nrPl = 0,  nrChar=0, nrFree = 0, nrSegs = 0;
	glMemSeg *memseg;

        plane = wdPriv->glPlaneList;
	for( i = 0; i < nrPlanes; i++,plane++ )
	{
            memseg = plane->memSegs;
            if( memseg == NULL ) continue;
            nrPl++;
            nrSegs += plane->nrMemSegs;
            memseg = plane->freeList;
            while( memseg )
            {
                nrFree++;
                memseg = memseg->next;
            }
        }

	for( i = 0; i < GL_HASH_TABLE_SIZE; i++ )
	{
	    memseg = wdPriv->glHashTab[i];
	    while ( memseg )
	    {
                nrChar++;
                memseg = memseg->next; 
	    }
	}

	ErrorF("GlyphCaching: %d char in cache,%d free locations, total %d \n",
                              nrChar, nrFree, nrSegs);
        ErrorF("              %d planes out of %d allocated\n", nrPl, nrPlanes);
	ErrorF("              Hits=%6d  Fails=%6d  LoadFails=%6d\n",
		cacheHits,cacheFails,loadFails );
        if( nrChar + nrFree != nrSegs )
           ErrorF(" *** inconsistency in cache admin ***\n");
        else
        {
           if( cacheAccess > 0)
	       ErrorF("              Average # cache seek loops %f \n",
		             ((float)cacheLoops)/cacheAccess );
        }
	cacheHits = 0; cacheFails = 0; loadFails = 0; 
	cacheAccess =0; cacheLoops =0;                  
}
#endif

