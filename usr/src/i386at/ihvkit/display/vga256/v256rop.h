#ident	"@(#)ihvkit:display/vga256/v256rop.h	1.1"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

/* 
 * This is valid only for Intel architecture.
 * Definitions to rasterop for Xwin.
 * This particular rendition is for XWIN with Screen Interface.
 */

#ifndef _RASTEROP_
#define _RASTEROP_

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include <sys/types.h>
#include <sys/dl.h>
#include <sys/at_ansi.h>
#include <sys/inline.h>

/* The type of pSrc and pDest must be a pointer to something. */

typedef SIbitmapP RASTER;

#define RASTER_DEPTH(r)	((r)->BbitsPerPixel)
#define RASTER_BITS(r)	((unsigned *) (r)->Bptr)
#define RASTER_BYTES_PER_LINE(r)	((((r)->Bwidth * RASTER_DEPTH(r) + \
					   0x1f) & ~0x1f) >> 3)
#define RASTER_HEIGHT(r)	((r)->Bheight)
#define RASTER_WIDTH(r)	((r)->Bwidth)	

#ifdef	SI_ROP_DEBUG
#define ERROR_MSG(s)	fprintf(stderr, s)
#else
#define ERROR_MSG(s)
#endif	/* SI_ROP_DEBUG */

#define ROP_CLEAR		GXclear
#define ROP_AND			GXand
#define ROP_AND_REVERSE		GXandReverse
#define ROP_COPY		GXcopy
#define ROP_AND_INVERSE		GXandInverted
#define ROP_NO_OP		GXnoop
#define ROP_XOR			GXxor
#define ROP_OR			GXor
#define ROP_NOR			GXnor
#define ROP_EQUIV		GXequiv
#define ROP_INVERT		GXinvert
#define ROP_OR_REVERSE		GXorReverse
#define ROP_COPY_INVERT		GXcopyInverted
#define ROP_OR_INVERSE		GXorInverted
#define ROP_NAND		GXnand
#define ROP_SET			GXset

#define USE_EGA()
#define FINISHED_WITH_EGA()

/*
 * Added function prototypes
 */

void
v256_rop (
    register	RASTER	pSrc, 
	register	RASTER	pDest,
    int		srcx, 
	int		srcy, 
	int		destx, 
	int		desty,
    int		width, 
	int		height,
    int		alu,
    unsigned	planeMask);

#endif

