#ident	"@(#)ihvkit:display/include/sisave.h	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

#ifndef SAVEUNDERWIN_H
#define SAVEUNDERWIN_H

#include "misc.h"
#include "window.h"
#include "pixmap.h"
#include "region.h"
#include "regionstr.h"
#include "./server/ddx/si/sidep.h"
	
/*
 * for debugging ...
#ifndef SAVETRACE
#define SAVETRACE
#endif
 */

typedef union _SUImageUnion {
	PixmapPtr	pPix;
	SIbitmap	*pCache;
} SUImageUnion;

#define SU_LOCALCACHE	0
#define SU_SDDCACHE	1

/*
 * private structure for Save-Unders
 */
typedef struct _SUPrivRec * SUPrivPtr;
typedef struct _SUPrivRec {
	WindowPtr pWin;		/* save under window */
	RegionPtr pImageClip;	/* restorable portion of pImage */
	unsigned char SUCacheType; /* LOCALCACHE or SDDCACHE */	
	SIbitmap  *pCache;	/* pointer to cached bitmap (if exists) */
} SUPrivRec;

typedef struct _SUPrivScrn {
	/*
	 * 'twas alot of other xwin specific stuff 
	 * here ... I'll leave it here as a place holder
	 */
	SUPrivPtr 	pSUPriv;	/* save-under image info */ 
} SUPrivScrn;

typedef struct _SUWindowRec * SUWindowPtr; 
typedef struct _SUWindowRec {
	WindowPtr pWin;		/* pointer to DIX window structure */
	BoxRec box;		/* inital geometry of the save-under window */ 
	unsigned char SUCacheType; /* LOCALCACHE or SDDCACHE */	
	SIbitmap  *pCache;	/* pointer to cached bitmap (if exists) */
	RegionPtr pImageClip;	/* portion of pImage that can be restored */
	SUWindowPtr next;	/* pointer to the next SUWindowRec structure */
} SUWindowRec;

extern SUWindowPtr SUWindows;	/* list of (mapped) save under windows */
extern RegionPtr   SURegion;	/* region formed by SUWindows */
extern int SUScrnPrivateIndex;	/* private index into the devPrivates array to
				/* access the current save-under image */
extern BoxPtr siSUComputeBox();/* computes bounding box for an array of pts */


/* ******************* MACROS ************************ */

/*
 * SUSupport() - expands to TRUE if screen "X" supports save-unders
 */
#define SUSupport(X) \
	((X)->saveUnderSupport != NotUseful)

/*
 * SUWindowsMapped() - expands to TRUE if there are any save-under
 * windows that are currently mapped to the screen.
 */
#define SUWindowsMapped() \
	(SUWindows != (SUWindowPtr) NULL)

/* 
 *
 * The Following (3) macros expand to TRUE if ...
 *
 *      1) the drawable is a window AND
 *
 *      2) there are any save under windows to worry about AND
 *
 *	3) the drawable/region/box intersects the region
 *         pointed to by SURegions
 */

#define SUCheckDrawable(X) \
	(SUWindowsMapped() && \
	 ((X)->type == DRAWABLE_WINDOW) && \
	 ((* (X)->pScreen->RectIn)(SURegion, \
		&(((WindowPtr)(X))->borderSize.extents)) != rgnOUT))

#define SUCheckRegion(X, Y) \
	(SUWindowsMapped() && \
	 ((X)->type == DRAWABLE_WINDOW) && \
	 ((* (X)->pScreen->RectIn)(SURegion, &((Y)->extents)) != rgnOUT))

#define SUCheckBox(X, Y) \
	(SUWindowsMapped() && \
	 ((X)->type == DRAWABLE_WINDOW) && \
	 ((* (X)->pScreen->RectIn)(SURegion, Y) != rgnOUT))

/*
 * SUPrivate() - expands to a pointer to the private save-under structure 
 * corresponding to screen "X".
 */
#define SUPrivate(X) \
	(((SUPrivScrn *)((X)->devPrivates[SUScrnPrivateIndex].ptr))->pSUPriv)

/* 
 * SUTestRestore(X) - expands to TRUE if SUPrivPtr of screen "X" is set
 * (i.e., a save-under window is being unmapped).
 */
#define SUTestRestore(X) \
	(SUPrivate(X) != (SUPrivPtr) NULL)

/*
 * SUTestNoExpose() - expands to TRUE if window "X" intersects the Image Clip
 * of window "Y" (thus, "X" should not receive expose events).
 */
#define SUTestNoExpose(X, Y) \
	((Y) && (* (X)->drawable.pScreen->RectIn)((Y)->pImageClip, &(X)->borderSize.extents))


#endif /* SAVEUNDERWIN_H */
