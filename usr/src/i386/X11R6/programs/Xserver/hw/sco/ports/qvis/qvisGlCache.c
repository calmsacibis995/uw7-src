/*
 *	@(#) qvisGlCache.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1994.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *      S001    Wed Sep 21 08:13:43 PDT 1994    davidw@sco.com
 *	- Correct compiler warnings.
 *      - Add VOLATILE keyword.
 *      - 2nd arg to GlClearCache changed to unique structureID in everest.
 *      S000    Wed May 12 11:10:46 PDT 1993    davidw@sco.com
 *      - Use USE_INLINE_CODE #define instead of SemiInlinedSquirt.
 *	And updated a comment that NFB handles width/height = zero.
 */
/**
 *
 * qvisGlCache.c
 *
 * Copyright 1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mjk       04/07/92  Originated (see RCS log)
 */
/*
 * If QUEUE_GLYPHS is defined, each glyph in the cache to be drawn
 * is put on a queue and then flushed all together.  Otherwise, glyphs
 * are blitted immediately.
 *
 * Using QUEUE_GLYPHS, appears to make the server doing
 * "./x11perf -range ftext,tr24itext" about 4-6% faster on small
 * glyph text operations.  The bottom line is DEFINE the QUEUE_GLYPHS
 * symbol.
 */
#define QUEUE_GLYPHS

#include "assert.h"

#include "xyz.h"
#include "X.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGlyph.h"

#include "qvisHW.h"
#include "qvisDefs.h"
#include "qvisMacros.h"
#include "qvisProcs.h"

#ifdef NDEBUG
#define QVIS_GLYPH_HASH_TABLE_SIZE 3072	/* production value */
#else
#define QVIS_GLYPH_HASH_TABLE_SIZE 35	/* smaller to expose hash table bugs */
#endif
#define QVIS_MAX_CACHED_GLYPHS 1024	/* four 32x32 chars/line, 256 lines */
#ifdef NDEBUG
#define QVIS_MAX_GLYPH_QUEUE_SIZE 256	/* production value */
#else
#define QVIS_MAX_GLYPH_QUEUE_SIZE 5	/* smaller to expose glyph queue bugs */
#endif

/*
 * GLYPH_LOC: given a cached glyph number, return it's frame buffer location
 * (a "linear address format" address).  Realize it is a bit address.  We use
 * the area at (768,0) to (1023,1023).
 * 
 * The addresses are bit addresses.  That is what the <<3 is for.
 * 
 * XXX Even when the resolution is lower 800x600 or 640x480, we still just use
 * the same area even though there is really more off screen memory.
 */
#define GLYPH_LOC(num) ((unsigned int) ((1024*768)<<3) + (num) * (128<<3))

#ifdef QUEUE_GLYPHS
/*
 * glyph_queue - a data structure used to queue up the writes for a sequence
 * of cached glyphs.  qvisEnqueGlyph will queue up the glyph;
 * qvisFlushGlyphQueue will make sure the glyphs are actually drawn to the
 * screen.
 */
static qvisGlyphQueueEntry glyph_queue[QVIS_MAX_GLYPH_QUEUE_SIZE];
static unsigned int cur_glyph_queue_entry;
#endif /* QUEUE_GLYPHS */

qvisPrivateData *qvisGlyphCacheScrPriv;

/*
 * QVIS_RND and QVIS_HASH - the macros used for calculating the glyph cache
 * hash function.
 * 
 * These are stolen out of effGlCache.c; the comment in that file reads: "the
 * hash macro, have tried a few different implementations (see Numerical
 * Recipies in C, page 211) and this gives about the best performance."  The
 * hash function has not been "tuned" for qvis but probably works well.
 */
#define QVIS_RND(a) \
   (((a) * 129749) >> 14)
#define QVIS_HASH(font_id, glyph_num, nelm) \
   (((QVIS_RND((font_id) << 8)) + QVIS_RND(glyph_num)) % (nelm))

/*
 * qvisInitGlyphCache - allocates and initializaes the array of bucket entry
 * point for the glyph cache hash table; allocates and initializes the actual
 * array of buckets; set the next victim index to zero; adds all this info to
 * the qvisPriv for the screen.  TRUE returned is successful; FALSE otherwise.
 */
Bool
qvisInitGlyphCache(pScreen)
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);
    register qvisGlyphCacheEntry **font_table;
    register qvisGlyphCacheEntry *font_buckets;
    int             i;

    XYZ("qvisInitGlyphCache-entered");
    font_table = (qvisGlyphCacheEntry **)
	xalloc(QVIS_GLYPH_HASH_TABLE_SIZE * sizeof(qvisGlyphCacheEntry *));
    font_buckets = (qvisGlyphCacheEntry *)
	xalloc(QVIS_MAX_CACHED_GLYPHS * sizeof(qvisGlyphCacheEntry));

    if ((!font_table) || (!font_buckets)) {
	XYZ("qvisInitGlyphCache-AllocFailure");
	xfree(font_table);
	xfree(font_buckets);
	return FALSE;
    }
    for (i = 0; i < QVIS_GLYPH_HASH_TABLE_SIZE; i++) {
	font_table[i] = NULL;
    }
    for (i = 0; i < QVIS_MAX_CACHED_GLYPHS; i++) {
	font_buckets[i].used = FALSE;
	font_buckets[i].queued = FALSE;
	font_buckets[i].fb_ptr = GLYPH_LOC(i);
    }
    qvisPriv->font_table = font_table;
    qvisPriv->font_buckets = font_buckets;
    qvisPriv->font_next_slot = 0;
    return TRUE;
}

/*
 * qvisFreeGlyphCache - frees the memory allocated for the hash table entry
 * array and the space for the actual buckets.
 */
void
qvisFreeGlyphCache(pScreen)
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);

    XYZ("qvisFreeGlyphCache-entered");
    assert(qvisPriv->glyph_cache == TRUE);
    xfree(qvisPriv->font_table);
    xfree(qvisPriv->font_buckets);
}

/*
 * qvisAddGlyphToCache - allocates new qvisGlyphCache entry for
 * <font_id,glyph_num> in the glyph cache for the screen of the given
 * qvisPriv.  NULL is returned if the add fails; so code using
 * qvisGlyphCacheEntry must be able to handle the case where a
 * qvisGlyphCacheEntry can not be allocated, ie. don't use the glyph cache to
 * draw the particular glyph.
 * 
 * The allocated bucket is just the "next" bucket in the entire bucket list.
 * There is no free list implemented.  This avoid any possible expensive
 * "victim selection" code.  Since the bucket list has 2K entries, it is
 * probably reasonable to assume simple incrementing through the list to find
 * next victims is probably not too bad a policy.  We should test this
 * assumption.
 * 
 * NOTE: the routine assumes that the <font_id,glyph_num> is NOT already in the
 * table; otherwise duplicate entries will exist.
 */
static qvisGlyphCacheEntry *
qvisAddGlyphToCache(qvisPriv, font_id, glyph_num)
    qvisPrivateData *qvisPriv;
    long            font_id;
    unsigned int    glyph_num;
{
    qvisGlyphCacheEntry **table;
    qvisGlyphCacheEntry *buckets;
    unsigned int    index;
    register qvisGlyphCacheEntry *bucket;
    int             next_slot;

    XYZ("qvisAddGlyphToCache-entered");
    next_slot = qvisPriv->font_next_slot;
    table = qvisPriv->font_table;
    buckets = qvisPriv->font_buckets;
    /*
     * naive (but fast) victim selection
     */
    bucket = &buckets[next_slot];
    if (bucket->used == TRUE) {
	XYZ("qvisAddGlyphToCache-BucketUsed");
	/*
	 * Must murder an active bucket!  Realize if the bucket is already
	 * queued (unlikely), we shouldn't kill it!
	 */
	if (bucket->queued == TRUE) {
	    XYZ("qvisAddGlyphToCache-AlreadyQueued");
	    return NULL;
	}
	if (bucket->next != NULL) {
	    bucket->next->prev = bucket->prev;
	    *(bucket->prev) = bucket->next;
	} else {
	    *(bucket->prev) = NULL;
	}
    }
    /*
     * Ok, we got an allocated bucket.  Let's start filling in the fields we
     * need.
     */
    bucket->queued = FALSE;
    bucket->used = TRUE;
    index = QVIS_HASH(font_id, glyph_num, QVIS_GLYPH_HASH_TABLE_SIZE);
    assert(index < QVIS_GLYPH_HASH_TABLE_SIZE);
    /*
     * Note: we put the bucket at the front of the list because it is likely
     * we will access it soon (and it's easy to stick it at the front).
     */
    if (table[index] != NULL) {
	table[index]->prev = &(bucket->next);
    }
    bucket->next = table[index];
    bucket->prev = &table[index];
    table[index] = bucket;
    /*
     * Increment our next_slot counter realizing it is modulo
     * QVIS_MAX_CACHED_GLYPHS.
     */
    if (next_slot == QVIS_MAX_CACHED_GLYPHS - 1) {
	qvisPriv->font_next_slot = 0;
    } else {
	qvisPriv->font_next_slot = next_slot + 1;
    }
    return bucket;
}

/*
 * qvisDrawOffScreenGlyph - the code to actually draw a given glyph into
 * off-screen memory (to fb location fb_ptr).  The glyph is drawn in packed
 * mode as a planar data.  This is because we will want to do a planar
 * screen-to-screen bit-blit to get the character drawn.
 */
static void
qvisDrawOffScreenGlyph(qvisPriv, glyph_ptr, height, width, glyph_info)
    qvisPrivateData *qvisPriv;
    unsigned int    glyph_ptr;
    unsigned int    height;
    unsigned int    width;
    nfbGlyphInfo   *glyph_info;
{
    VOLATILE unsigned char  *fb_ptr = qvisPriv->fb_base + (glyph_ptr >> 3);
    unsigned char  *pline;
    register unsigned char *pimage;
    VOLATILE register unsigned char *fb;
    unsigned int    widthinbytes;
    register int    stride;
    unsigned int    j, k;

    /*
     * Assert that character to be drawn fits inside the 32x32 pixel
     * space we have for it.
     */
    assert((width > 0) && (width <= 32));
    assert((height > 0) && (height <= 32));

    if (qvisPriv->engine_used) {
	qvisPriv->engine_used = FALSE;
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPackedMode(0x4);
    stride = glyph_info->stride;
    widthinbytes = ((width - 1) >> 3) + 1;

    pline = glyph_info->image;

    /*
     * XXX This double loop should be a single loop for strides
     * 1-4 (the only possible strides).  See the QuickStride code
     * in qvisUncachedDrawMonoGlyphs. -mjk
     *
     * The hope is that the speed of this code is not critical to
     * overall performance because it is only used when glyphs
     * have to be drawn to off-screen memory.  The main code path
     * SHOULD (hopefully) be that the glyph is already in off-screen
     * memory.
     */
    for (j = 0; j < height; j++, pline += stride, fb_ptr += 4) {
	pimage = pline;
	fb = fb_ptr;
	for (k = 0; k < widthinbytes; k++) {
	    *fb++ = *pimage++;
	}
    }
}

#if 0 /* never; use assembly */
/*
 * qvisSquirt - squirt out the out's needed to screen-to-screen blit for glyph
 * caching.  If there is no inlining of the out's, this should be written in
 * assembly.
 * 
 * SEE: qvisSquirt.s
 */
static void
qvisSquirt(fb_ptr, h, w, x, y)
    unsigned int    fb_ptr;
    short           w;
    short           h;
    short           x;
    short           y;
{
    qvisOut16(Y0_BREG, fb_ptr >> 16);
    qvisOut16(X0_BREG, fb_ptr & 0xffff);
    qvisOut16(HEIGHT_REG, h);
    qvisOut16(WIDTH_REG, w);
    qvisOut16(X1_BREG, x);
    qvisOut16(Y1_BREG, y);
    qvisOut8(BLT_CMD0, 0x1);
}
#endif

#ifdef QUEUE_GLYPHS
/*
 * qvisFlushGlyphQueue - draws to the screen all the glyphs that have been
 * enqued on the glyph queue.
 * 
 * NOTE: queued glyphs MUST be flushed before the next X11 request is processed
 * otherwise protocol violations are possible.  Additionally, the glyph queue
 * is a single global data structure; you USE IT then you FLUSH IT before
 * anyone else gets a chance to use it.
 */
static void
qvisFlushGlyphQueue()
{
    qvisGlyphQueueEntry *entry;
    qvisGlyphCacheEntry *bucket;
    unsigned int    fb_ptr;
    register int    prev_height;
    register int    prev_width;
    register int    prev_y;

    XYZ("qvisFlushGlyphQueue-entered");

    if (cur_glyph_queue_entry == 0) {
	return;
    }
    if (qvisGlyphCacheScrPriv->engine_used) {
	qvisWaitForGlobalNotBusy();
    }
    qvisSetPlanarMode(0x51);
    qvisOut8(BLT_CMD1, 0x80);
    qvisOut16(SOURCE_PITCH_REG, 1);

    entry = &glyph_queue[0];

    prev_height = -1;
    prev_width = -1;
    prev_y = -1;

    do {
	XYZ("qvisFlushGlyphQueue-FlushingGlyph");
	bucket = entry->bucket;
	fb_ptr = bucket->fb_ptr;
	/*
	 * Idempotently say this bucket no longer queued - it may ACTUALLY be
	 * on the queue other times but that will be resolved by the time
	 * qvisGlushGlyphQueue is finished.
	 */
	bucket->queued = FALSE;

#ifdef USE_INLINE_CODE					/* S000 */
	qvisOut16(Y0_BREG, fb_ptr >> 16);
	qvisOut16(X0_BREG, fb_ptr & 0xffff);

	if (bucket->height != prev_height) {
	    qvisOut16(HEIGHT_REG, prev_height = bucket->height);
	}
	if (bucket->width != prev_width) {
	    qvisOut16(WIDTH_REG, prev_width = bucket->width);
	}
	qvisOut16(X1_BREG, entry->x);
	if (entry->y != prev_y) {
	    qvisOut16(Y1_BREG, prev_y = entry->y);
	}
	/* turn on engine */
	qvisOut8(BLT_CMD0, 0x1);
#else
	qvisSquirt(fb_ptr, bucket->height, bucket->width, entry->x, entry->y);
#endif				/* USE_INLINE_CODE */

	/* Check for buffers not busy */
	qvisWaitForBufferNotBusy();

	/* next step of loop */
	entry++;
	cur_glyph_queue_entry--;
    } while (cur_glyph_queue_entry > 0);
    /*
     * Doing the kind of screen-to-screen blit we do above seems to mess up
     * the state of the blit engine so it will do the wrong thing the next
     * time the qvisCopyRect blit is done.
     * 
     * SEE: qvisCopyRect
     */
    qvisGlyphCacheScrPriv->bad_blit_state = TRUE;
    qvisGlyphCacheScrPriv->engine_used = TRUE;
}
#endif /* QUEUE_GLYPHS */

#ifdef QUEUE_GLYPHS
#  define qvisHandleGlyph(bucket, x, y) \
   qvisEnqueGlyph(bucket, x, y)
#else
#  define qvisHandleGlyph(bucket, x, y) \
   qvisBlitGlyph(bucket, x, y)
#endif

#ifndef QUEUE_GLYPHS
#ifdef DontInline_qvisBlitGlyph
/*
 * qvisBlitGlyph - used if we are going to blit glyphs directly
 * instead of putting them on a queue
 */
static void
qvisBlitGlyph(bucket, x, y)
    qvisGlyphCacheEntry *bucket;
    unsigned int    x;
    unsigned int    y;
{
    XYZ("qvisBlitGlyph-entered");
    qvisWaitForBufferNotBusy();
    qvisSetPlanarMode(0x51);
    qvisOut8(BLT_CMD1, 0x80);
    qvisSquirt(bucket->fb_ptr, bucket->height, bucket->width, 
       (short) x, (short) y);
    qvisGlyphCacheScrPriv->engine_used = TRUE;
}
#else
#define qvisBlitGlyph(bucket, x, y) \
    { \
        XYZ("qvisBlitGlyph-entered"); \
        qvisWaitForBufferNotBusy(); \
        qvisSetPlanarMode(0x51); \
        qvisOut8(BLT_CMD1, 0x80); \
        qvisSquirt(bucket->fb_ptr, bucket->height, bucket->width, \
            (short) x, (short) y); \
        qvisGlyphCacheScrPriv->engine_used = TRUE; \
    }
#endif /* DontInline_qvisBlitGlyph */
#endif /* QUEUE_GLYPHS */

#ifdef QUEUE_GLYPHS
#ifdef DontInline_qvisEnqueuGlyph
/*
 * qvisEnqueGlyph - put a bucket (with a valid glyph draw to off-screen
 * memory) on the glyph queue to be drawn at (x,y).
 */
static void
qvisEnqueGlyph(bucket, x, y)
    qvisGlyphCacheEntry *bucket;
    unsigned int    x;
    unsigned int    y;
{
    register qvisGlyphQueueEntry *entry;

    XYZ("qvisEnqueGlyph-entered");
    /*
     * If the glyph queue is completely full (unlikely), then we need to call
     * qvisFlushGlyphQueue to make space.
     */
    if (cur_glyph_queue_entry == QVIS_MAX_GLYPH_QUEUE_SIZE) {
	XYZ("qvisEnqueGlyph-MustFlushQueue");
	qvisFlushGlyphQueue();
    }
    /*
     * It is critical to make the bucket as "queued" so that no later
     * qvisAddGlyphToCache can steal the bucket away before the next
     * qvisFlushGlyphQueue.
     */
    bucket->queued = TRUE;
    entry = &glyph_queue[cur_glyph_queue_entry];
    entry->bucket = bucket;
    entry->x = x;
    entry->y = y;
    cur_glyph_queue_entry++;
}
#else
/*
 * macro version of qvisEnqueGlyph to eliminate a procedure call
 * from the
 */
#define qvisEnqueGlyph(QVIS__bucket, QVIS__x, QVIS__y) \
   { \
      register qvisGlyphQueueEntry *QVIS__entry; \
      XYZ("qvisEnqueGlyph-entered"); \
      if (cur_glyph_queue_entry == QVIS_MAX_GLYPH_QUEUE_SIZE) { \
         XYZ("qvisEnqueGlyph-MustFlushQueue"); \
         qvisFlushGlyphQueue(); \
      } \
      QVIS__bucket->queued = TRUE; \
      QVIS__entry = &glyph_queue[cur_glyph_queue_entry]; \
      QVIS__entry->bucket = QVIS__bucket; \
      QVIS__entry->x = QVIS__x; \
      QVIS__entry->y = QVIS__y; \
      cur_glyph_queue_entry++; \
   }
#endif /* DontInline_qvisEnqueuGlyph */
#endif /* QUEUE_GLYPHS */

/*
 * qvisTryToCacheAndHandleGlyph - knowing the glyph is NOT in the cache,
 * try to add the glyph to the cache and enque the glyph on the glyph
 * queue to be drawn later.  We return TRUE if we are able to do this;
 * FALSE means we were not able and the glyph must be drawn immediately.
 */
static          Bool
qvisTryToCacheAndHandleGlyph(qvisPriv, glyph_info, font_id, glyph_num)
    qvisPrivateData *qvisPriv;
    nfbGlyphInfo   *glyph_info;
    long            font_id;
    unsigned int    glyph_num;
{
    BoxPtr          pbox;
    short  height;
    short  width;
    register qvisGlyphCacheEntry *bucket;

    XYZ("qvisTryToCacheAndHandleGlyph-entered");
    pbox = &glyph_info->box;
    height = pbox->y2 - pbox->y1;
    /*
     * Bail out if height > 32 since max size for every off-screen glyph is
     * 32x32.
     * 
     * NFB layer now protects against heights and widths == zero.
     * Apparently, the Open Look cursor font has two glyphs that 
     * are of height == zero.
     * 
     * As Kenny Rogers says, "Know when to fold 'em."
     */
    if ((height <= 0) || (height > 32)) {
	XYZ("qvisTryToCacheAndHandleGlyph-TooHigh");
	return FALSE;
    }
    /*
     * Bail out if width > 32 since max size for every off-screen glyph is
     * 32x32.
     * 
     * Also protect ourselves from widths == 0.
     */
    width = pbox->x2 - pbox->x1;
    if ((width <= 0) || (width > 32)) {
	XYZ("qvisTryToCacheAndHandleGlyph-TooWide");
	return FALSE;
    }
    bucket = qvisAddGlyphToCache(qvisPriv, font_id, glyph_num);
    if (bucket == NULL) {
	XYZ("qvisTryToCacheAndHandleGlyph-CouldntAllocateBucket");
	return FALSE;
    }
    bucket->height = height;
    bucket->width = width;
    bucket->font_id = font_id;
    bucket->glyph_num = glyph_num;
    qvisDrawOffScreenGlyph(qvisPriv, bucket->fb_ptr,
			  height, width, glyph_info);
    qvisHandleGlyph(bucket, pbox->x1, pbox->y1);
    return TRUE;
}

/*
 * qvisInvalidateGlyphCache - if the screen switching facility of the SCO
 * server is used, we can not gaurantee that the off-screen glyph cache is
 * still available so we need to invalidate it.
 * 
 * SEE: qvisRestoreGState
 */
void
qvisInvalidateGlyphCache(qvisPriv)
    qvisPrivateData *qvisPriv;
{
    register qvisGlyphCacheEntry **font_table;
    register qvisGlyphCacheEntry *font_buckets;
    int             i;

    XYZ("qvisInvalidateGlyphCache-entered");
    assert(qvisPriv->glyph_cache == TRUE);
    font_table = qvisPriv->font_table;
    font_buckets = qvisPriv->font_buckets;
    for (i = 0; i < QVIS_GLYPH_HASH_TABLE_SIZE; i++) {
	font_table[i] = NULL;
    }
    for (i = 0; i < QVIS_MAX_CACHED_GLYPHS; i++) {
	font_buckets[i].used = FALSE;
	font_buckets[i].queued = FALSE;
	/* make sure frame buffer ptr is still correct */
	assert(font_buckets[i].fb_ptr == GLYPH_LOC(i));
    }
    qvisPriv->font_next_slot = 0;
}

#ifdef FuncVersOf_qvisGlyphCacheEntry
/*
 * qvisGlyphCacheEntry - deterines if the <font_id,glyph_num> entry exists in
 * the font_table glyph cache hash table; returns the bucket for the glyph if
 * it exists in the table; otherwise returns NULL.
 */
static qvisGlyphCacheEntry *
qvisFindCachedGlyph(font_table, font_id, glyph_num)
    qvisGlyphCacheEntry **font_table;
    long            font_id;
    unsigned int    glyph_num;
{
    unsigned int    index;
    qvisGlyphCacheEntry *bucket;

    XYZ("qvisFindCachedGlyph-entered");
    index = QVIS_HASH(font_id, glyph_num, QVIS_GLYPH_HASH_TABLE_SIZE);
    assert(index < QVIS_GLYPH_HASH_TABLE_SIZE);
    bucket = font_table[index];
    while (bucket != NULL) {
	XYZ("qvisFindCachedGlyph-SearchingBucketList");
	if ((bucket->font_id == font_id) && (bucket->glyph_num == glyph_num)) {
	    return bucket;
	}
	bucket = bucket->next;
    }
    return NULL;
}
#endif				/* FuncVersOf_qvisGlyphCacheEntry */

void 
qvisCachedDrawMonoGlyphs(
			   nfbGlyphInfo * glyph_info,
			   unsigned int nglyphs,
			   unsigned long fg,
			   unsigned char alu,
			   unsigned long planemask,
			   unsigned long font_private,
			   DrawablePtr pDrawable)
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pDrawable->pScreen);
    Bool            success;
    BoxPtr          pbox;
    unsigned int    i;
#ifndef NDEBUG
    int             loop_detector;
#endif

    XYZ("qvisCachedDrawMonoGlyphs-entered");
    assert(qvisPriv->glyph_cache == TRUE);
    qvisSetCurrentScreen();

    qvisGlyphCacheScrPriv = qvisPriv;

    if (qvisPriv->engine_used) {
	qvisWaitForGlobalNotBusy();
    }
    qvisSetForegroundColor((unsigned char) fg);
    qvisSetALU(alu);
    qvisSetPlaneMask((unsigned char) planemask);
#ifndef QUEUE_GLYPHS
    qvisOut16(SOURCE_PITCH_REG, 1);
#endif

    for (i = 0; i < nglyphs; i++, glyph_info++) {
#ifdef FuncVersOf_qvisFindCachedGlyph
	qvisGlyphCacheEntry *entry;

	entry = qvisFindCachedGlyph(qvisPriv->font_table, font_private,
				   glyph_info->glyph_id);
	if (entry != NULL) {
	    XYZ("qvisCachedDrawMonoGlyphs-InCache");
	    pbox = &glyph_info->box;
	    qvisHandleGlyph(entry, pbox->x1, pbox->y1);
	} else {
	    XYZ("qvisCachedDrawMonoGlyphs-NotInCache");
	    success = qvisTryToCacheAndHandleGlyph(qvisPriv, glyph_info, font_private,
						 glyph_info->glyph_id);
	    if (!success) {
		XYZ("qvisCachedDrawMonoGlyphs-CouldntPutInCache");
		qvisUncachedDrawMonoGlyphs(glyph_info, 1, fg, alu, planemask,
					  font_private, pDrawable);
	    }
	}
#else /* FuncVersOf_qvisFindCachedGlyph */
	/*
	 * This is a version of the above with the text of qvisFindCachedGlyph
	 * inlined into the routine.
	 */
	unsigned int    index;
	register qvisGlyphCacheEntry *bucket;
	unsigned int    glyph_num = glyph_info->glyph_id;

        /*
	 * Pretend we are in qvisFinCachedGlyph for XYZ; all XYZ macros
	 * here should be prefixed qvisFindCachedGlyph.
	 */
	XYZ("qvisFindCachedGlyph-entered");
	index = QVIS_HASH(font_private, glyph_num, QVIS_GLYPH_HASH_TABLE_SIZE);
	assert(index < QVIS_GLYPH_HASH_TABLE_SIZE);
	bucket = qvisPriv->font_table[index];
#ifndef NDEBUG
	loop_detector = 0;
#endif
	while (bucket != NULL) {
	    XYZ("qvisFindCachedGlyph-SearchingBucketList");
	    if ((bucket->font_id == font_private) && (bucket->glyph_num == glyph_num)) {
		XYZ("qvisCachedDrawMonoGlyphs-InCache");
		pbox = &glyph_info->box;
		qvisHandleGlyph(bucket, pbox->x1, pbox->y1);
		goto NextGlyph;
	    }
	    bucket = bucket->next;
# ifndef NDEBUG
	    loop_detector++;
	    if (loop_detector >= QVIS_MAX_CACHED_GLYPHS) {
		FatalError("Loop detected glyph cache during lookup\n");
	    }
# endif
	}
	XYZ("qvisCachedDrawMonoGlyphs-NotInCache");
	success = qvisTryToCacheAndHandleGlyph(qvisPriv, glyph_info,
            font_private, glyph_info->glyph_id);
	if (!success) {
	    XYZ("qvisCachedDrawMonoGlyphs-CouldntPutInCache");
	    qvisUncachedDrawMonoGlyphs(glyph_info, 1, fg, alu, planemask,
				      font_private, pDrawable);
	}
      NextGlyph:;
#endif				/* FuncVersOf_qvisGlyphCacheEntry */
    }
#ifdef QUEUE_GLYPHS
    /*
     * Ok, we are done - write out all cached glyphs now - hopefully there
     * are a lot and we do this very fast.
     */
    qvisFlushGlyphQueue();
#else
    qvisGlyphCacheScrPriv->bad_blit_state = TRUE;
#endif
}

#ifndef NDEBUG
static void
DEBUG_CHECK_FOR_LOOPS(bucket)
    qvisGlyphCacheEntry *bucket;
{
    int             i;

    for (i = 0; i < QVIS_MAX_CACHED_GLYPHS; i++, bucket++) {
	register int    loop_detector;
	register qvisGlyphCacheEntry *bucket2;

	if (bucket->used) {
	    bucket2 = bucket;
	    loop_detector = 0;
	    while (bucket2 != NULL) {
		loop_detector++;
		if (loop_detector >= QVIS_MAX_CACHED_GLYPHS) {
		    FatalError(
				  "Loop statically detected in glyph cache! num = %d\n", i);
		}
		bucket2 = bucket2->next;
	    }
	}
    }
}
#else
#define DEBUG_CHECK_FOR_LOOPS(x)
#endif

/*
 * qvisGlClearCache - when a font is totally closed, we should tranverse the
 * glyph cache and kill all cache entries for that font.  If we knew that
 * font_id's never used again (or not reincarnated for a long time at least),
 * we could probably get away with out this routine.
 */
void
qvisGlyphCacheClearFont(font_id, pScreen)
#ifdef agaII
    unsigned long   font_id;
#else
    struct _nfbFontPS * font_id;				/* S001 */
#endif
    ScreenPtr       pScreen;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pScreen);
    register qvisGlyphCacheEntry *bucket;
    int             i;

    XYZ("qvisGlyphCacheClearFont-entered");
    assert(qvisPriv->glyph_cache == TRUE);
    bucket = qvisPriv->font_buckets;
    DEBUG_CHECK_FOR_LOOPS(bucket);
    /*
     * Loop over all the glyph buckets - would it be faster to traverse they
     * hash table since all the buckets traversed we would know are in use?
     * Probably not because most buckets should (hopefully) be in use - and
     * we can do an assertion sanity check.
     */
    for (i = 0; i < QVIS_MAX_CACHED_GLYPHS; i++, bucket++) {
	/*
	 * No buckets should be queued unless a qvisCachedDrawMonoGlyphs
	 * routine is running.
	 */
	assert(bucket->queued == FALSE);
	/*
	 * Is the bucket used and for the font_id we are expiring?
	 */
	if ((bucket->used == TRUE) && 
			(bucket->font_id == (unsigned long)font_id)) { /*S001*/
	    XYZ("qvisGlyphCacheClearFont-ClearGlyph");
	    if (bucket->next != NULL) {
		XYZ("qvisGlyphCacheClearFont-LastBucketOnList");
		bucket->next->prev = bucket->prev;
		*(bucket->prev) = bucket->next;
	    } else {
		XYZ("qvisGlyphCacheClearFont-NotLastBucketOnList");
		*(bucket->prev) = NULL;
	    }
	    /*
	     * Very important.  Mark bucket unused now!
	     */
	    bucket->used = FALSE;
	}
    }
    DEBUG_CHECK_FOR_LOOPS(qvisPriv->font_buckets);
}
