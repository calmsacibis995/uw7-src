#ident	"@(#)ihvkit:display/lfb256/lfbState.c	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <string.h>
#include <stdlib.h>
#include <lfb.h>

/*	MISCELLANEOUS ROUTINES		*/
/*		MANDATORY		*/

/* 
 * This routine makes a number of calls malloc() and realloc() to 
 * allocate memory.  The memory returned becomes part of the Graphic 
 * States.  In order to minimize the number of calls, this routine 
 * always allocates twice as much memory as needed.
 * 
 */

SIBool lfbDownLoadState(sindex, sflag, stateP)
SIint32 sindex, sflag;
SIGStateP stateP;

{
    register SIGStateP p = &lfbGStates[sindex];
    static int line_cnt[LFB_NUM_GSTATES];
    static int clip_cnt[LFB_NUM_GSTATES];
    static int tile_siz[LFB_NUM_GSTATES];
    static int stpl_siz[LFB_NUM_GSTATES];

    if (sflag & SetSGpmask)
	p->SGpmask = stateP->SGpmask & PMSK;

    if (sflag & SetSGmode)
	p->SGmode = stateP->SGmode;

    if (sflag & SetSGstplmode)
	p->SGstplmode = stateP->SGstplmode;

    if (sflag & SetSGfillmode)
	p->SGfillmode = stateP->SGfillmode;

    if (sflag & SetSGfillrule)
	p->SGfillrule = stateP->SGfillrule;

    if (sflag & SetSGarcmode)
	p->SGarcmode = stateP->SGarcmode;

    if (sflag & SetSGlinestyle)
	p->SGlinestyle = stateP->SGlinestyle;

    if (sflag & SetSGfg)
	p->SGfg = stateP->SGfg & PMSK;

    if (sflag & SetSGbg)
	p->SGbg = stateP->SGbg & PMSK;

    if (sflag & SetSGcmapidx)
	p->SGcmapidx = stateP->SGcmapidx;

    if (sflag & SetSGvisualidx)
	p->SGvisualidx = stateP->SGvisualidx;

    if (sflag & SetSGtile) {
	register int i, j;
	register SIbitmapP s=stateP->SGtile;
	register SIbitmapP d=p->SGtile;

	if (!d) {
	    p->SGtile = (SIbitmapP)malloc(sizeof(SIbitmap));
	    d = p->SGtile;
	    if (!d) {
		fprintf(stderr, "LFB DL State: malloc failed\n");
		return(SI_FAIL);
	    }
	}

	d->Btype   = s->Btype;
	d->BbitsPerPixel = s->BbitsPerPixel;
	d->Bwidth  = s->Bwidth;
	d->Bheight = s->Bheight;
	d->BorgX   = s->BorgX;
	d->BorgY   = s->BorgY;

	/* Calculate number of bytes needed for bitmap */
	i = BitmapStride(s) * s->Bheight;

	if (tile_siz[sindex] == 0) {
	    tile_siz[sindex] = i * 2;
	    d->Bptr = (SIArray)malloc(i * 2);
	}
	else if (tile_siz[sindex] < i) {
	    tile_siz[sindex] = i * 2;
	    d->Bptr = (SIArray)realloc(d->Bptr, i * 2);
	}

	if (! d->Bptr) {
	    tile_siz[sindex] = 0;
	    fprintf(stderr, "LFB DL State: malloc failed\n");
	    return(SI_FAIL);
	}

	memmove(d->Bptr, s->Bptr, i);
    }

    if (sflag & SetSGstipple) {
	register int i, j;
	register SIbitmapP s=stateP->SGstipple;
	register SIbitmapP d=p->SGstipple;

	if (!d) {
	    p->SGstipple = (SIbitmapP)malloc(sizeof(SIbitmap));
	    d = p->SGstipple;
	    if (!d) {
		fprintf(stderr, "LFB DL State: malloc failed\n");
		return(SI_FAIL);
	    }
	}

	d->Btype   = s->Btype;
	d->BbitsPerPixel = s->BbitsPerPixel;
	d->Bwidth  = s->Bwidth;
	d->Bheight = s->Bheight;
	d->BorgX   = s->BorgX;
	d->BorgY   = s->BorgY;

	/* Calculate number of bytes needed for bitmap */
	i = BitmapStride(s) * s->Bheight;

	if (stpl_siz[sindex] == 0) {
	    stpl_siz[sindex] = i * 2;
	    d->Bptr = (SIArray)malloc(i * 2);
	}
	else if (stpl_siz[sindex] < i) {
	    stpl_siz[sindex] = i * 2;
	    d->Bptr = (SIArray)realloc(d->Bptr, i * 2);
	}

	if (! d->Bptr) {
	    stpl_siz[sindex] = 0;
	    fprintf(stderr, "LFB DL State: malloc failed\n");
	    return(SI_FAIL);
	}

	memmove(d->Bptr, s->Bptr, i);
    }


    if (sflag & SetSGline) {
	register int i;

	i = p->SGlineCNT = stateP->SGlineCNT;

	if (line_cnt[sindex] == 0) {
	    line_cnt[sindex] = i * 2;
	    p->SGline = (SIint32 *)malloc(i * 2 * sizeof(SIint32));
	}
	else if (line_cnt[sindex] < i) {
	    line_cnt[sindex] = i * 2;
	    p->SGline = (SIint32 *)realloc(p->SGline, i * 2 * sizeof(SIint32));
	}

	if (! p->SGline) {
	    line_cnt[sindex] = 0;
	    fprintf(stderr, "LFB DL State: malloc failed\n");
	    return(SI_FAIL);
	}

	/* i is alread set to stateP->SGlineCNT */
	memmove(p->SGline, stateP->SGline, i * sizeof(SIint32));
    }

    if (sflag & SetSGcliplist) {
	register int i;

	i = p->SGclipCNT = stateP->SGclipCNT;

	if (clip_cnt[sindex] == 0) {
	    clip_cnt[sindex] = i * 2;
	    p->SGcliplist = (SIRectP)malloc(i * 2 * sizeof(SIRect));
	}
	else if (clip_cnt[sindex] < i) {
	    clip_cnt[sindex] = i * 2;
	    p->SGcliplist = (SIRectP)realloc(p->SGcliplist,
					     i * 2 * sizeof(SIRect));
	}

	if (! p->SGcliplist) {
	    clip_cnt[sindex] = 0;
	    fprintf(stderr, "LFB DL State: malloc failed\n");
	    return(SI_FAIL);
	}

	/* i is alread set to stateP->SGclipCNT */
	memmove(p->SGcliplist, stateP->SGcliplist, i * sizeof(SIRect));

	p->SGclipextent = stateP->SGclipextent;
    }

    return(SI_SUCCEED);
}

SIBool lfbGetState(sindex, sflag, stateP)
SIint32 sindex, sflag;
SIGStateP stateP;

{
    register SIGStateP p = &lfbGStates[sindex];

    if (sflag & GetSGpmask)
	stateP->SGpmask = p->SGpmask;

    if (sflag & GetSGmode)
	stateP->SGmode = p->SGmode;

    if (sflag & GetSGstplmode)
	stateP->SGstplmode = p->SGstplmode;

    if (sflag & GetSGfillmode)
	stateP->SGfillmode = p->SGfillmode;

    if (sflag & GetSGfillrule)
	stateP->SGfillrule = p->SGfillrule;

    if (sflag & GetSGarcmode)
	stateP->SGarcmode = p->SGarcmode;

    if (sflag & GetSGlinestyle)
	stateP->SGlinestyle = p->SGlinestyle;

    if (sflag & GetSGfg)
	stateP->SGfg = p->SGfg;

    if (sflag & GetSGbg)
	stateP->SGbg = p->SGbg;

    if (sflag & GetSGcmapidx)
	stateP->SGcmapidx = p->SGcmapidx;

    if (sflag & GetSGvisualidx)
	stateP->SGvisualidx = p->SGvisualidx;

    return(SI_SUCCEED);
}

SIBool lfbSelectState(sindex)
SIint32 sindex;

{
    lfb_cur_GStateP = &lfbGStates[sindex];
    lfb_cur_GState_idx = sindex;

    return(SI_SUCCEED);
}
