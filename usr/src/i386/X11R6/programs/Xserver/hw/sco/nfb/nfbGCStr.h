/*
 *	@(#) nfbGCStr.h 12.1 95/05/09 SCOINC
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *
 * SCO Modification History
 *
 * S006, 12-Jul-94, mikep
 *	Change FillZeroSeg to FillZeroSegs
 * S005, 04-Dec-92, mikep
 *      revamp S004.  Put PtToPt lines in MiscOps.  Move FillPolygon
 *	to nfbGCops and call it FillPolygons.
 *	Let's do FillZeroSegs in Gecko
 * S004, 01-Nov-92, mikep
 *      move clipping required routines into their own structure
 * S003, 14-Sep-92, mikep
 *      added saveops to GC private
 * S002, 01-Sep-92, staceyc
 * 	declare polygon parameters correctly!
 * S001, 31-Aug-92, staceyc
 * 	fill polygon added to gc ops
 * S000, 25-Aug-92, staceyc
 * 	chewed up one reserved gc op for poly text 8
 */

/*
 * nfbGCStr.h
 *
 * Copyright 1989, Silicon Graphics, Inc.
 *
 * Author: Jeff Weinstein
 *
 * structure definitions for No Frame Buffer(nfb) porting layer.
 *
 */
#ifndef _NFB_GC_STR_H
#define _NFB_GC_STR_H

#include "fontstruct.h"
#include "gcstruct.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "window.h"
#include "windowstr.h"

typedef struct _nfbGCOps {
	void (* FillRects) (GCPtr pGC, DrawablePtr pDraw, BoxPtr pbox, 
			unsigned int nbox ) ;
	void (* FillSpans) (GCPtr pGC, DrawablePtr pDraw, DDXPointPtr ppt,
				unsigned int *pwidth, unsigned int npts) ;
	void (* FillZeroSegs) (GCPtr pGC, DrawablePtr pDraw, BresLinePtr plines,
				int nlines );
	void (* FillPolygons) (GCPtr pGC, DrawablePtr pDraw, DDXPointPtr *pppt,
				int *pnpts, int npgns, int shape, int fill) ;
	void (* GCOp5) () ;   /* reserved for expansion */
	void (* GCOp6) () ;   /* reserved for expansion */
	} nfbGCOps, *nfbGCOpsPtr ;

typedef struct _nfbGCMiscOps {					/* S005 */
	void (*PolyZeroSeg)( GCPtr pGC, DrawablePtr pDraw, BoxPtr pline, 
			     int nlines );
	} nfbGCMiscOps, *nfbGCMiscOpsPtr ;

/*
 * Some operations can use a reduced raster-op so that we can avoid
 * read-modify-write by tweaking plane mask and foreground.
 */
typedef struct _nfbReducedRop {
	unsigned long fg ;
	unsigned long bg ;
	unsigned char alu ;
	unsigned long planemask ;
	unsigned int fillstyle ;
	char *stipple ;	/* used when we have two color tiles */
			/* stipple should be PixmapPtr */
	} nfbReducedRop ;

typedef struct _nfbGCPriv {
/* DO NOT CHANGE THESE FIELDS, OR ADD ANY IN FRONT OF THEM, THEY MUST
 * BE EXACTLY THE SAME AS IN MFB AND CFB!!!!
 */
	unsigned char rop;		/* reduction of rasterop to 1 of 3 */
	unsigned char ropOpStip;	/* rop for opaque stipple */
	unsigned char ropFillArea;	/*  == alu, rop, ropOpStip */
	unsigned fExpose:1;		/* callexposure handling ? */
	unsigned freeCompClip:1;
	PixmapPtr pRotatedPixmap;
	RegionPtr pCompositeClip;	/* FREE_CC or REPLACE_CC */
	unsigned long xor, and;		/* reduced rop values for cfb */

/* End of DO NOT CHANGE fields */

	unsigned char lastDrawableType ;
	unsigned char lastDrawableDepth ;
	nfbGCOpsPtr ops ;
	nfbReducedRop hwRop ;
	nfbReducedRop rRop ;
	DDXPointRec screenPatOrg ;/* screen relative tile/stip offset */
	nfbGCMiscOpsPtr miscops ; /* misc ops, possibly non-complient S005 */
	nfbGCOpsPtr saveops ;
	DevUnion devPrivate ;
	} nfbGCPriv, *nfbGCPrivPtr ;


#endif /* _NFB_GC_STR_H */
