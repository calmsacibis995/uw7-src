#ident	"@(#)ihvkit:display/lfb256/lfb.h	1.1"

/*
 * Copyright (c) 1990, 1991, 1992, 1993 UNIX System Laboratories, Inc.
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	Copyright (c) 1993  Intel Corporation
 *	  All Rights Reserved
 */

#ifndef _LFB_H_
#define _LFB_H_

#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>

#include <sidep.h>

#if !defined(BPP)
#error Need to define BPP
#endif

#if	(BPP == 8)

#define PMSK (unsigned)0xff
#define PSZ 1
#define PFILL(p) ((u_long)p | (u_long)p<<BPP | (u_long)p<<(2*BPP) | (u_long)p<<(3*BPP))

typedef u_char Pixel, *PixelP;

#elif	(BPP == 16)

#define PMSK (unsigned)0xffff
#define PSZ 2
#define PFILL(p) ((u_long)p | (u_long)p<<BPP)

typedef u_short Pixel, *PixelP;

#elif	(BPP == 24) || (BPP == 32)

#if	(BPP == 24)
#define PMSK 0xffffff
#else
#define PMSK (unsigned)0xffffffff
#endif

#define PSZ 4
#define PFILL(p) (p)

typedef u_long Pixel, *PixelP;

#else
#error Unsupported BPP
#endif

typedef struct {
    Pixel p1, p2, p3, p4;
} Pixel4, *Pixel4P;

#define LFB_NUM_GSTATES 8

typedef struct {
    PixelP fb_ptr;
    int width, height;
    int stride;			/* measured in pixels */
    int vr_width, vr_height;
    int fd;
    int memsize;
} GenFB, *GenFB_P;

extern ScreenInterface lfbDisplayInterface;

extern GenFB lfb;

extern SIGState lfbGStates[];
extern SIGStateP lfb_cur_GStateP;
extern int lfb_cur_GState_idx;
extern void (*lfbVendorFlush)();

extern lfbstrcasecmp(char *s1, char *s2);


#define ScreenAddr(x, y) (lfb.fb_ptr + (y) * lfb.stride + (x))

/* 
 * BitmapStride() measures the size of the bitmap in bytes.
 *
 * BitmapScanL() returns a pointer (caddr_t) to the beginning of scan
 * line y in the bitmap.
 *
 * BitmapAddr() returns a pointer (caddr_t) to (x,y) in the bitmap.
 *
 * BitmapAddr() is only valid if (bm)->BbitsPerPixel is a power of two
 * greater than 8.  (C does not have pointers that point to anything 
 * smaller than a caddr_t.)
 *
 */

#if (BPP & (BPP-1))
#error Add code for BPP != 2^n
#endif

#define BitmapStride(bm) ((((bm)->Bwidth * (bm)->BbitsPerPixel + 31) & ~31) >> 3)
#define BitmapScanL(bm, y) ((caddr_t)(bm)->Bptr + \
			    (y) * BitmapStride(bm))
#define BitmapAddr(bm, x, y) (BitmapScanL((bm), (y)) + \
			      ((x) * ((bm)->BbitsPerPixel) >> 3))

/*	MISCELLANEOUS ROUTINES 	*/
/*		MANDATORY	*/

extern SIBool lfbInitLFB();
extern SIBool lfbShutdownLFB();
/* extern SIBool lfbVTSave();		*/
/* extern SIBool lfbVTRestore();	*/
/* extern SIBool lfbVideoBlank();	*/
/* extern SIBool lfbInitCache();	*/
/* extern SIBool lfbFlushCache();	*/
extern SIBool lfbDownLoadState();
extern SIBool lfbGetState();
extern SIBool lfbSelectState();
/* extern SIBool lfbSelectScreen();	*/

/*	SCANLINE AT A TIME ROUTINES	*/
/*		MANDATORY		*/

extern SILine lfbGetSL();
extern SIvoid lfbSetSL();
extern SIvoid lfbFreeSL();

/*	COLORMAP MANAGEMENT ROUTINES	*/
/*		MANDATORY		*/

/* extern SIBool lfbSetCmap();		*/
/* extern SIBool lfbGetCmap();		*/

/*	CURSOR CONTROL ROUTINES		*/
/*		MANDATORY		*/

/* extern SIBool lfbDownLoadCurs();	*/
/* extern SIBool lfbTurnOnCurs();	*/
/* extern SIBool lfbTurnOffCurs();	*/
/* extern SIBool lfbMoveCurs();		*/

/*	HARDWARE SPANS CONTROL		*/
/*		OPTIONAL		*/

extern SIBool lfbFillSpans();

/*	HARDWARE BITBLT ROUTINES	*/
/*		OPTIONAL		*/

extern SIBool lfbSSbitblt();
extern SIBool lfbMSbitblt();
extern SIBool lfbSMbitblt();

/*	HARDWARE BITBLT ROUTINES	*/
/*		OPTIONAL		*/

extern SIBool lfbSSstplblt();
extern SIBool lfbMSstplblt();
extern SIBool lfbSMstplblt();

/*	HARDWARE POLYGON FILL		*/
/*		OPTIONAL		*/

extern SIvoid lfbPolygonClip();
extern SIBool lfbFillConvexPoly();
extern SIBool lfbFillGeneralPoly();
extern SIBool lfbFillRects();

/*	HARDWARE POINT PLOTTING		*/
/*		OPTIONAL		*/

extern SIBool lfbPlotPoints();

/*	HARDWARE LINE DRAWING		*/
/*		OPTIONAL		*/

extern SIvoid lfbLineClip();
extern SIBool lfbThinLines();
extern SIBool lfbThinSegments();
extern SIBool lfbThinRect();

/*	HARDWARE DRAW ARC ROUTINE	*/
/*		OPTIONAL		*/

extern SIvoid lfbDrawArcClip();
extern SIBool lfbDrawArc();

/*	HARDWARE FILL ARC ROUTINE	*/
/*		OPTIONAL		*/

extern SIvoid lfbFillArcClip();
extern SIBool lfbFillArc();

/*	HARDWARE FONT CONTROL		*/
/*		OPTIONAL		*/

extern SIBool lfbCheckDLFont();
extern SIBool lfbDownLoadFont();
extern SIBool lfbFreeFont();
extern SIvoid lfbFontClip();
extern SIBool lfbStplbltFont();

/*	SDD MEMORY CACHING CONTROL	*/
/*		OPTIONAL		*/

extern SIBool lfbAllocCache();
extern SIBool lfbFreeCache();
extern SIBool lfbLockCache();
extern SIBool lfbUnlockCache();

/*	SDD EXTENSION INITIALIZATION	*/
/*		OPTIONAL		*/

extern SIBool lfbInitExten();

#endif /* _LFB_H_ */
