/*
 *  @(#) wdGlyph.c 11.1 97/10/22
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
 *   wdGlyph.c      Probe and Initialize the wd Graphics Display Driver
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
 *              GC caching by checking of serial # changes
 *	S003	Thu  05-Nov-1992	edb@sco.com
 *              do local checks for loaded attributes instead of using serial#
 *	S004	Mon 09-Nov-1992	edb@sco.com
 *              fix argument type
 *	S005	Thu  07-Jan-1993	buckm@sco.com
 *              work around mono transparency chip bug.
 *      S006    Tue 09-Feb-1993 buckm@sco.com
 *              change parameter checks into assert()'s.
 *		get rid of GC caching.
 *		use all 8 planes for glyph storage.
 *		make ascend/descend lists circular with a head node.
 *		maintain a freelist of mem segs.
 *      S007    Wdn 19-May-93 edb@sco.com
 *		Args to wdDrawMonoGlyphs changed
 *      S008    Thu 27-May-1993 edb@sco.com
 *		Changes to do also 15 16 & 24 bits
 *              Remove wdLoadGlyph and wdDrawGlyph from this file
 *		and put into new file wdDrwGlyph.c
 *              
 */

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
#include "nfb/nfbRop.h"						/* S005 */
#include "nfb/nfbGlyph.h"

#include "genProcs.h"
#include "wdDefs.h"
#include "wdScrStr.h"
#include "wdProcs.h"
#include "wdBitswap.h"


#define GL_HASH_TABLE_SIZE_MASK   0x3FF
#define GL_HASH_TABLE_SIZE (GL_HASH_TABLE_SIZE_MASK + 1)

#define GL_HASH(a, b) (((a) + ((b) >> 4)) & GL_HASH_TABLE_SIZE_MASK)

#define MIN_FREE_SIZE	20


typedef struct _glMemSeg
    {					/* next must be 1st member !! */
	struct _glMemSeg *next;		/* pointer to next in hash list */
	struct _glMemSeg *ascend;	/* pointer to higher addr. mem */
	struct _glMemSeg *descend;	/* pointer to lower addr. mem */
	int fontId;
	int glyphId;
	int Addr;			/* relativ to fbBase */
	int Size;      
	unsigned short Plane;
	unsigned short free;
    } glMemSeg;


static glMemSeg *wdFindGlyph( int font_id, int glyph_id );
static glMemSeg *wdAllocGlyph(int font_id, int glyph_id, int mem_size);
static glMemSeg *wdAllocGlyphMem( int mem_size );
static void      wdFreeGlyphMem( glMemSeg *memseg );

static glMemSeg **GlHashTab;
static glMemSeg  *GlFreeList;
static glMemSeg   GlHead[WD_PLANES];

static int largestFreeSize;
static int totalCacheMemSize;

/* #define CACHE_PRINT  */

#ifdef CACHE_PRINT
static int cacheHits =0, cacheFails =0, loadFails =0;
static cacheLoops =0, cacheAccess =0;
#endif


/* wdDrawMonoGlyphs - Draws a number of glyphs defined by glyph_info.
 *            Checks whether glyph is already in cache .
 *                  ( Hash keys:  font_id and glyph_info->glyph_id )
 *            If yes it blits out of cache using fg color, alu and planemask
 *            If not it tries to find space in cache (offscreen memory) 
 *            and draws into cache as a linear pixmap
 *            If no space in cache it uses wdDrawMonoImage
 */

void
wdDrawMonoGlyphs(
    nfbGlyphInfo *glyph_info,
    unsigned int nglyphs,
    unsigned long fg,
    unsigned char alu,
    unsigned long planemask,
    nfbFontPSPtr pPS,           /* S007 */
    DrawablePtr pDraw)
{
	int i, size;
	int w, h;
	glMemSeg *memseg;
	BoxPtr gl_box;
        int font_id = pPS->private.val;    /* S007 */
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pDraw->pScreen );
        int pixb = wdPriv->pixBytes;
        void ( * draw_mono_image )();     /* S008 */
        void ( * load_glyph )();     /* S008 */
        void ( * draw_glyph )();     /* S008 */

#ifdef DEBUG_PRINT
        ErrorF("wdDrawMonoGlyphs( ,nglyphs=%d,fg=0x%x,alu=%d,planemask=0x%x, , \n",
                nglyphs,fg,alu,planemask);
#endif

        if( wdPriv->pixBytes == 1 ) {              /* S008 vvvvvvvv */
            draw_mono_image  = wdDrawMonoImage;
            load_glyph       = wdLoadGlyph8;
            draw_glyph       = wdDrawGlyph8;
        }
        else {
            draw_mono_image  = genDrawMonoImage;
            load_glyph       = wdLoadGlyph15_24;
            draw_glyph       = wdDrawGlyph15_24;
        }                                           /* S008 ^^^^^^^ */

	assert( nglyphs );

	/* S005
	 * Work around apparent chip bug.
	 * During mono transparency operations,
	 * if the alu does not involve the source,
	 * the blt'er seems to lose track of the mono pattern.
	 * We get around this bug by adjusting the alu and fg
	 * to an equivalent operation that does involve the source.
	 *
	 * We could do this transformation in wdDrawGlyph,
	 * but doing it once here seems more efficient,
	 * and it's okay to call wdDrawMonoImage
	 * with the transformed alu and fg.
	 */
	if( ! rop_needs_src[alu] )
	{
	    switch( alu )
	    {
	    case GXclear:	fg = 0;		alu = GXcopy;	break;
	    case GXset:		fg = ~0;	alu = GXcopy;	break;
	    case GXinvert:	fg = ~0;	alu = GXxor;	break;
	    case GXnoop:	return;
	    }
	}

	for (i = 0; i < nglyphs; ++i, ++glyph_info)
	{
	    gl_box = &(glyph_info->box);
	    w = gl_box->x2 - gl_box->x1;
	    h = gl_box->y2 - gl_box->y1;
	    assert( w > 0 && h > 0 );

            if(wdPriv->glyphCache == 0 ) {
		( draw_mono_image)( gl_box, glyph_info->image, 0,    /* S008 */
				 glyph_info->stride, fg, alu,
				 planemask, pDraw);
                continue;
            }

	    memseg = wdFindGlyph( font_id, glyph_info->glyph_id );
#ifdef CACHE_PRINT
	    if( memseg ) cacheHits++;
	    else         cacheFails++;
#endif
	    if( memseg == NULL )
	    {
		/* 
		 * In WDC31 a scanline may start on a dword address only 
		 * This seems to be true also for linear mode .
		 * Would be nice to find this somewhere pointed out
		 * in the manual instead of finding it the hard way !!!!
		 */
		size = ((w * pixb + 3) & ~3) * h;    /* S008 */
		memseg = wdAllocGlyph( font_id, glyph_info->glyph_id, size);

		if( memseg )
		    ( load_glyph)( w, h, glyph_info->image, glyph_info->stride,
				 memseg->Addr, memseg->Plane, pDraw);    /* S008 */
	    }
	    if( memseg )
	    {   
		( draw_glyph)( gl_box, memseg->Addr, memseg->Plane,    /* S008 */
			     fg, alu, planemask, pDraw);
	    }
	    else
	    {
		( draw_mono_image)( gl_box, glyph_info->image, 0,    /* S008 */
				 glyph_info->stride, fg, alu,
				 planemask, pDraw);
#ifdef CACHE_PRINT
		loadFails++;
#endif
	    }

#ifdef CACHE_PRINT
	    if( cacheAccess % 100 == 0 ) printCache();
#endif
	}
}


/*
 *       GLYPH  CACHE  ROUTINES
 *       ----------------------
 */

/*
 *   wdInitGlCache - Initialize hash table and freelist
 */

wdInitGlCache( wdPriv )
	wdScrnPrivPtr wdPriv;
{
	glMemSeg *head, *memseg;
	int mem_start;
	int mem_size;
	int i, plane;

	GlHashTab = (glMemSeg **)
			xalloc(GL_HASH_TABLE_SIZE * sizeof(glMemSeg *));
	for( i = 0; i < GL_HASH_TABLE_SIZE; i++ )
	{
	    GlHashTab[i] = NULL;
	}

	mem_start = wdPriv->glyphCache;
	mem_size  = wdPriv->glyphCacheSize;

	GlFreeList = NULL;

	for( plane = WD_PLANES; --plane >= 0; )
	{
	    head   = &GlHead[plane];
	    memseg = (glMemSeg *)xalloc(sizeof(glMemSeg));

	    head->ascend  = memseg;
	    head->descend = memseg;
	    head->free    = FALSE;

	    memseg->next    = GlFreeList;
	    memseg->ascend  = head;
	    memseg->descend = head;
	    memseg->Addr    = mem_start;
	    memseg->Size    = mem_size;
	    memseg->Plane   = plane;
	    memseg->free    = TRUE;

	    GlFreeList = memseg;
	}

	largestFreeSize = mem_size;
	totalCacheMemSize = mem_size * WD_PLANES;

#ifdef CACHE_CHECK
	ErrorF("CacheInit\n");
	wdCheckGlCache();
#endif
#ifdef CACHE_PRINT
	cacheHits =0; cacheFails =0; loadFails =0;
	cacheLoops =0; cacheAccess =0;
	ErrorF(" GlyphCaching initialized\n");
#endif
}

/*
 * wdDeallocGlCache - free all allocated memsegs
 */

wdDeallocGlCache( wdPriv )
	wdScrnPrivPtr wdPriv;
{
	glMemSeg *memseg, *head, *next_memseg;
	int plane;

	xfree( GlHashTab );

	for( plane = 0; plane < WD_PLANES; plane++)
	{
	    head = &GlHead[plane];
	    for (memseg = head->ascend; memseg != head; )
	    {
		next_memseg = memseg->ascend;
		xfree( memseg );
		memseg = next_memseg;
	    }
	}
}

/*
 *  wdFindGlyph - Find [font_id,glyph_id] in hash table
 *                  and return memseg or NULL
 */

static glMemSeg *
wdFindGlyph( font_id, glyph_id )
	int font_id;
	int glyph_id;
{
	int index;
	glMemSeg *memseg;

#ifdef CACHE_PRINT
	cacheAccess += 1;
#endif
	index  = GL_HASH( font_id, glyph_id );
	memseg = GlHashTab[index];

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
 *  wdAllocGlyph -  Add glyph entry in hash table,
 *                  and allocate space in off screen video mem.
 *                  Return memseg, or NULL if not enough space.
 */

static glMemSeg *
wdAllocGlyph( font_id, glyph_id, mem_size )
	int font_id;
	int glyph_id;
	int mem_size;
{
	int index;
	glMemSeg *memseg;

	if( memseg = wdAllocGlyphMem( mem_size ) )
	{
	    index = GL_HASH( font_id, glyph_id );

	    memseg->fontId  = font_id;
	    memseg->glyphId = glyph_id;
	    memseg->next    = GlHashTab[index];

	    GlHashTab[index] = memseg;
	}

	return ( memseg );
}

/*
 *  wdClearFont  - Remove all glyph memsegs with ->fontId == font_id
 *                 from the hash lists and free them.
 */

void
wdClearFont(
        nfbFontPSPtr pPS,    /* S007 */
	ScreenPtr pScreen )
{ 
	int i;
	glMemSeg *memseg, *prev_memseg;
        int font_id = pPS->private.val;    /* S007 */
	wdScrnPrivPtr wdPriv = WD_SCREEN_PRIV( pScreen );

        if(wdPriv->glyphCache == 0 ) return;   /* S008 */

#ifdef CACHE_CHECK
	ErrorF("ClearFont %x\n", font_id);
	wdCheckGlCache();
#endif
	for( i = 0; i < GL_HASH_TABLE_SIZE; i++ )
	{
	    prev_memseg = (glMemSeg *)&GlHashTab[i];
	    while ( memseg = prev_memseg->next )
	    {
		if( memseg->fontId == font_id )
		{
		    prev_memseg->next = memseg->next;
		    wdFreeGlyphMem( memseg );
		}
		else
		    prev_memseg = memseg;
	    }
	}

	/*
	 * clean up any zero-size memsegs wdFreeGlyphMem left on the freelist
	 * reset largestFreeSize while we're at it
	 */
	largestFreeSize = 0;
	prev_memseg = (glMemSeg *)&GlFreeList;
	while( memseg = prev_memseg->next )
	{
	    if( memseg->Size == 0 )
	    {
		prev_memseg->next = memseg->next;
		xfree( memseg );
	    }
	    else
	    {
		if( memseg->Size > largestFreeSize )
		    largestFreeSize = memseg->Size;
		prev_memseg = memseg;
	    }
	}

#ifdef CACHE_CHECK
	ErrorF("After Clear\n");
	wdCheckGlCache();
#endif
#ifdef CACHE_PRINT
	printCache();
#endif
}

/*
 *  wdAllocGlyphMem - Allocate space in offscreen video memory
 *                    Get the first large enough entry in the freelist
 *                    If its too big split it, and create a new memseg
 *                    for the upper part.
 */

static glMemSeg *
wdAllocGlyphMem( mem_size )
	int mem_size;         /* required size in bytes  */
{
	glMemSeg *memseg, *prev_memseg, *new_memseg;
	int left_over;

	if( mem_size >= largestFreeSize )
	    return NULL;

	prev_memseg = (glMemSeg *)&GlFreeList;
	while( memseg = prev_memseg->next )
	{
	    if( (left_over = memseg->Size - mem_size) >= 0 )
	    {
		/* If the remaining space is not too small we split
		 * the memory block and allocate a new memseg
		 */
		if( left_over >= MIN_FREE_SIZE )
		{
		    new_memseg = (glMemSeg *)xalloc(sizeof(glMemSeg));

		    new_memseg->ascend  = memseg->ascend;
		    new_memseg->descend = memseg;
		    memseg->ascend->descend = new_memseg;
		    memseg->ascend	    = new_memseg;

		    memseg->Size    -= mem_size;
		    new_memseg->Size = mem_size;
		    new_memseg->Addr = memseg->Addr + memseg->Size;
		    new_memseg->Plane = memseg->Plane;
		    new_memseg->free  = FALSE;

		    return( new_memseg );
		}
		else
		{
		    /* just remove this memseg from the freelist */
		    memseg->free = FALSE;
		    prev_memseg->next = memseg->next;

		    return( memseg );
		}
	    }
	    prev_memseg = memseg;
	}

	/* 
	 * We could not reduce largestFreeSize after previous allocations.
	 * Therefore, it can be too big, and we did not find a fit.
	 * So lets set it right and avoid unsuccessful loops
	 * through the whole list for further allocations.
	 */
	largestFreeSize = 0;
	memseg = GlFreeList;
	while( memseg )
	{
	    if( memseg->Size > largestFreeSize )
		largestFreeSize = memseg->Size;
	    memseg = memseg->next;
	}

	return NULL;
}

/*
 *  wdFreeGlyphMem - Free space in offscreen video memory,
 *		     combining adjacent memsegs.
 */

static void
wdFreeGlyphMem( memseg )
	glMemSeg *memseg;
{
	glMemSeg *lower_memseg, *higher_memseg;

	lower_memseg  = memseg->descend;
	higher_memseg = memseg->ascend;

	if( lower_memseg->free )
	{
	    /* add memseg to lower_memseg */
	    lower_memseg->Size += memseg->Size;
	    if( higher_memseg->free )
	    {
		/* add in higher_memseg also */
		lower_memseg->Size += higher_memseg->Size;
		lower_memseg->ascend = higher_memseg->ascend;
		higher_memseg->ascend->descend = lower_memseg;
		/* mark higher_memseg for freeing later */
		higher_memseg->Size = 0;
	    }
	    else
	    {
		lower_memseg->ascend   = higher_memseg;
		higher_memseg->descend = lower_memseg;
	    }
	    xfree( memseg );
	}
	else if( higher_memseg->free )
	{
	    /* add memseg to higher_memseg */
	    higher_memseg->Addr  = memseg->Addr;
	    higher_memseg->Size += memseg->Size;
	    higher_memseg->descend = lower_memseg;
	    lower_memseg->ascend   = higher_memseg;
	    xfree( memseg );
	}
	else
	{
	    /* no free neighbors */
	    memseg->free = TRUE;
	    memseg->next = GlFreeList;
	    GlFreeList   = memseg;
	}
}

#ifdef CACHE_PRINT

printCache()
{
	int nrCluster = 0, maxCluster=0, totalMem=0, nrChar=0;
	int plane;
	glMemSeg *memseg, *head;

	for( plane = 0; plane < WD_PLANES; plane++ )
	{
	    head = &GlHead[plane];
	    for (memseg = head->ascend; memseg != head; )
	    {
		if( memseg->free )
		{ 
		    if( memseg->Size >maxCluster)
			maxCluster = memseg->Size;
		    totalMem += memseg->Size;
		    nrCluster++;
		}
		else
		    nrChar++;
		memseg = memseg->ascend;
	    }
	}

	ErrorF("GlyphCaching: Total # char in cache %d\n",nrChar);
	ErrorF("              Hits=%6d  Fails=%6d  LoadFails=%6d\n",
		cacheHits,cacheFails,loadFails );
  if( cacheAccess > 0)
	ErrorF("              Average # cache seek loops %f \n",
		             ((float)cacheLoops)/cacheAccess );
	ErrorF("             totalFree=%d, nrCluster=%d, maxClusterSize=%d\n",
		totalMem,nrCluster,maxCluster);
	cacheHits = 0; cacheFails = 0; loadFails = 0; 
	cacheAccess =0; cacheLoops =0;                  
}
#endif

#ifdef CACHE_CHECK
wdCheckGlCache()
{
	glMemSeg *memseg;
	int i, len;
	int sumfree = 0;
	int glyphs = 0, queues = 0, sumsize = 0, sum2len = 0;

	ErrorF("  Free =");
	memseg = GlFreeList;
	while (memseg)
	{
	    ErrorF(" %d-%x:%x", memseg->Plane, memseg->Addr, memseg->Size);
	    if (!memseg->free)
		ErrorF("!!");
	    sumfree += memseg->Size;
	    memseg = memseg->next;
	}
	ErrorF("\n  Total free %x bytes, largestFreeSize=%x\n",
	    sumfree, largestFreeSize);

	for( i = 0; i < GL_HASH_TABLE_SIZE; i++ )
	{
	    if (memseg = GlHashTab[i])
	    {
		queues++;
		len = 0;
		do
		{
		    len++;
		    sumsize += memseg->Size;
		    if (memseg->free)
		    {
			ErrorF("  free memseg on hash queue!!\n");
			ErrorF("    %d-%x:%x ",
			    memseg->Plane, memseg->Addr, memseg->Size);
			ErrorF("fid=%x gid=%d dwn=%x up=%x\n",
			    memseg->fontId, memseg->glyphId,
			    memseg->descend, memseg->ascend);
		    }
		} while (memseg = memseg->next);
		glyphs += len;
		len -= 1;
		sum2len += len * len;
	    }
	}
	ErrorF("  %d glyphs on %d queues total %x bytes, sum2len=%d\n",
	    glyphs, queues, sumsize, sum2len);

	i = totalCacheMemSize - (sumfree + sumsize);
	if (i)
	    ErrorF("  !! MISSING MEMORY - %x\n", i);
}
#endif
