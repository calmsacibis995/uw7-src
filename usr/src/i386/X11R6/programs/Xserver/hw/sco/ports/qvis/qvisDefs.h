/*
 *	@(#) qvisDefs.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Oct 06 21:47:11 PDT 1992	mikep@sco.com
 *	- Add GC caching
 *      S001    Thu Oct 07 12:51:02 PDT 1993    davidw@sco.com
 *      - Integrated Compaq source handoff
 *      S002    Tue Sep 20 13:03:48 PDT 1994    davidw@sco.com
 *      - Add VOLATILE keyword - defined in server/include/compiler.h.
 */

/**
 *
 * Copyright 1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mjk       04/07/92  Originated (see RCS log)
 * mikep@sco 07/27/92  Add current_bank to screen privates
 * waltc     06/26/93  Add width, pitch, max_cursor to screen private.
 * waltc     10/05/93  Add v35_blit_bug to screen private.
 */

#ifndef _QVIS_DEFS_H
#define _QVIS_DEFS_H

#include "regionstr.h" /* needed for nfbWinStr.h */
#include "nfb/nfbWinStr.h"
#include "nfb/nfbGCStr.h"

typedef struct _qvisGlyphCacheEntry qvisGlyphCacheEntry;

struct _qvisGlyphCacheEntry {
    char            used;
    char            queued;
    unsigned int    fb_ptr;
    qvisGlyphCacheEntry *next;
    qvisGlyphCacheEntry **prev;
    short           height;
    short           width;
    unsigned long   font_id;
    unsigned int    glyph_num;
};

typedef struct _qvisGlyphQueueEntry {
    unsigned short  x;
    unsigned short  y;
    qvisGlyphCacheEntry *bucket;
}               qvisGlyphQueueEntry;

typedef struct _qvisColor {
   unsigned char    red;
   unsigned char    green;
   unsigned char    blue;
}               qvisColor;

typedef struct _qvisPrivateData {
    VOLATILE unsigned char  *fb_base;
    Bool            bad_blit_state;	/* set after cached glyph draws */
    Bool            engine_used;
    Bool            flat_mode;	/* TRUE if in flat mode, FALSE if banked */
    Bool            glyph_cache; /* TRUE if using glyph caching */
    Bool            primary; /* TRUE if the primary adapter */
    char            virt_controller_id;	/* Trition Virtual Controller (0-15) */

    /* cursor hot spots */
    int             Xhot;	/* x cursor hotspot offset */
    int             Yhot;	/* y cursor hotspot offset */

    /* glyph caching data structures */
    qvisGlyphCacheEntry **font_table;
    qvisGlyphCacheEntry *font_buckets;
    unsigned int    font_next_slot;

    /* function tables */
    nfbWinOpsPtr    qvisWinOps;
    nfbGCOps       *qvisSolidPrivOpsPtr;

    int 	    current_bank;
    int             width;	/* display width */
    int		    pitch;	/* display pitch */
    int		    pitch2;	/* display pitch as power of 2 */
    int             max_cursor; /* maximum cursor size */
    unsigned long   current_gc;					/* S000 */
#ifdef QVIS_SHADOWED
    /* these fields are needed when the registers are shadowed */
    int             alu;
    int             fg;
    int             bg;
    int             pixel_mask;
    int             plane_mask;
#endif				/* QVIS_SHADOWED */

    /* saved DAC table */
#define NUMPALENTRIES 256
    qvisColor        dac_state[NUMPALENTRIES];

    /* V35-3 ssblit bug */
    Bool            v35_blit_bug;
}               qvisPrivateData;

extern int      qvisScreenPrivateIndex;

extern unsigned char alu_to_hwrop[];

#endif				/* _QVIS_DEFS_H */
