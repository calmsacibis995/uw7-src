#ident	"@(#)ihvkit:display/include/simskbits.h	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */


/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved
********************************************************/
/* $Header$ */

#define sistartpartial(i)	(0xffffffff << ((i) << PSH))
#define siendpartial(i)		(((i) == 0)? 0xffffffff: ~sistartpartial(i))
#define	sistarttab(i)		(((i) == 0)? 0: sistartpartial(i))
#define siendtab(i)		(~sistartpartial(i))

extern unsigned int QuartetBitsTable[];
extern unsigned char si_PPW_to_PWSH[];

/*
 * ==========================================================================
 * Converted from mfb to support memory-mapped color framebuffer by smarks@sun, 
 * April-May 1987.
 *
 * The way I did the conversion was to consider each longword as an
 * array of four bytes instead of an array of 32 one-bit pixels.  So
 * getbits() and putbits() retain much the same calling sequence, but
 * they move bytes around instead of bits.  Of course, this entails the
 * removal of all of the one-bit-pixel dependencies from the other
 * files, but the major bit-hacking stuff should be covered here.
 *
 * I've created some new macros that make it easier to understand what's 
 * going on in the pixel calculations, and that make it easier to change the 
 * pixel size.
 *
 * name	    1-bpp   8-bpp   explanation
 * ----	    ---	    ---	    -----------
 * PPW	    32	    4	    pixels per word
 * PLST	    31	    3	    last pixel in a word (should be PPW-1)
 * PIM	    0x1f    0x03    pixel index mask (index within a word)
 * PWSH	    5	    2	    pixel-to-word shift
 * PSZ	    1	    8	    pixel size (bits)
 * PMSK	    0x01    0xFF    single-pixel mask
 * PSH      0       3       multiply factor shift value.  (*PSZ) 1*8 = 1<<3
 *
 * Note that even with these new macros, there are still many dependencies 
 * in the code on the fact that there are 32 bits (not necessarily pixels) 
 * in each word.  These macros remove the dependency that 1 pixel == 1 bit.
 *
 * The PFILL macro has been changed to a function, si_pfill(depth, val).
 * It takes one pixel and * replicates it throughout a word.  
 * Examples: for monochrome, PFILL(1) => 0xffffffff, PFILL(0) => 0x00000000.  
 * For 8-bit color, PFILL(0x5d) => 0x5d5d5d5d.  This macro is used primarily 
 * for replicating a plane mask into a word.
 *
 * Color framebuffers operations also support the notion of a plane
 * mask.  This mask determines which planes of the framebuffer can be
 * altered; the others are left unchanged.  I have added another
 * parameter to the putbits and putbitsrop macros that is the plane
 * mask.
 * ==========================================================================
 */

extern	int PPW;
extern  int PLST;
extern	int PWSH;
extern	int PSZ;
extern	int PSH;
extern	int PMSK;
#define	PIM		PLST

#define SET_PSZ(bitsPerPixel) { \
    PSZ = bitsPerPixel; \
    PMSK = (1 << PSZ) - 1; \
    PPW = 32 / PSZ; \
    PLST = (PPW - 1); \
    PWSH = si_PPW_to_PWSH[PPW]; \
    PSH  = (5 - PWSH); \
  }

/* the following notes use the following conventions:
SCREEN LEFT				SCREEN RIGHT
in this file and maskbits.c, left and right refer to screen coordinates,
NOT bit numbering in registers.

sistarttab[n] 
	pixels[0,n-1] = 0's	pixels[n,PPW-1] = 1's
siendtab[n] =
	pixels[0,n-1] = 1's	pixels[n,PPW-1] = 0's

sistartpartial[], siendpartial[]
	these are used as accelerators for doing putbits and masking out
bits that are all contained between longword boudaries.  the extra
256 bytes of data seems a small price to pay -- code is smaller,
and narrow things (e.g. window borders) go faster.

the names may seem misleading; they are derived not from which end
of the word the bits are turned on, but at which end of a scanline
the table tends to be used.

look at the tables and macros to understand boundary conditions.
(careful readers will note that starttab[n] = ~endtab[n] for n != 0)

-----------------------------------------------------------------------
these two macros depend on the screen's bit ordering.
in both of them x is a screen position.  they are used to
combine bits collected from multiple longwords into a
single destination longword, and to unpack a single
source longword into multiple destinations.

SCRLEFT(dst, x)
	takes dst[x, PPW] and moves them to dst[0, PPW-x]
	the contents of the rest of dst are 0 ONLY IF
	dst is UNSIGNED.
	is cast as an unsigned.
	this is a right shift on the VAX, left shift on
	Sun and pc-rt.

SCRRIGHT(dst, x)
	takes dst[0,x] and moves them to dst[PPW-x, PPW]
	the contents of the rest of dst are 0 ONLY IF
	dst is UNSIGNED.
	this is a left shift on the VAX, right shift on
	Sun and pc-rt.


the remaining macros are cpu-independent; all bit order dependencies
are built into the tables and the two macros above.

maskbits(x, w, startmask, endmask, nlw)
	for a span of width w starting at position x, returns
a mask for ragged pixels at start, mask for ragged pixels at end,
and the number of whole longwords between the ends.

maskpartialbits(x, w, mask)
	works like maskbits(), except all the pixels are in the
	same longword (i.e. (x&0xPIM + w) <= PPW)

mask32bits(x,w,startmask,endmask)
	as maskbits, but does not calculate nlw.  used in 
	`siTextHelper' to put down glyphs <= 32 bits wide.

getbits(psrc, x, w, dst)
	starting at position x in psrc (x < PPW), collect w
	pixels and put them in the screen left portion of dst.
	psrc is a longword pointer.  this may span longword boundaries.
	it special-cases fetching all w bits from one longword.

	+--------+--------+		+--------+
	|    | m |n|      |	==> 	| m |n|  |
	+--------+--------+		+--------+
	    x      x+w			0     w
	psrc     psrc+1			dst
			m = PPW - x
			n = w - m

	implementation:
	get m pixels, move to screen-left of dst, zeroing rest of dst;
	get n pixels from next word, move screen-right by m, zeroing
		 lower m pixels of word.
	OR the two things together.

putbits(src, x, w, pdst, planemask)
	starting at position x in pdst, put down the screen-leftmost
	w bits of src.  pdst is a longword pointer.  this may
	span longword boundaries.
	it special-cases putting all w bits into the same longword.

	+--------+			+--------+--------+
	| m |n|  |		==>	|    | m |n|      |
	+--------+			+--------+--------+
	0     w				     x     x+w
	dst				pdst     pdst+1
			m = PPW - x
			n = w - m

	implementation:
	get m pixels, shift screen-right by x, zero screen-leftmost x
		pixels; zero rightmost m bits of *pdst and OR in stuff
		from before the semicolon.
	shift src screen-left by m, zero bits n-32;
		zero leftmost n pixels of *(pdst+1) and OR in the
		stuff from before the semicolon.

putbitsrop(src, x, w, pdst, planemask, ROP)
	like putbits but calls DoRop with the rasterop ROP (see cgidef.h for
	DoRop)

getleftbits(psrc, w, dst)
	get the leftmost w (w<=PPW) bits from *psrc and put them
	in dst.  this is used by the siGlyphBlt code for glyphs
	<=PPW bits wide.
*/

#include	"X.h"
#include	"Xmd.h"
#include	"servermd.h"
#if	(BITMAP_BIT_ORDER == MSBFirst)
#define SCRLEFT(lw, n)	((lw) << ((n) << PSH))
#define SCRRIGHT(lw, n)	((lw) >> ((n) << PSH))
#else	/* (BITMAP_BIT_ORDER == LSBFirst) */
#define SCRLEFT(lw, n)	((lw) >> ((n) << PSH))
#define SCRRIGHT(lw, n)	((lw) << ((n) << PSH))
#endif	/* (BITMAP_BIT_ORDER == MSBFirst) */

/* PPc:md - new 1-bit specific shifts */
#if	(BITMAP_BIT_ORDER == MSBFirst)
#define SCRLEFT1(lw, n) ((lw) << (n))
#define SCRRIGHT1(lw, n) ((lw) >> (n))
#else	/* (BITMAP_BIT_ORDER == LSBFirst) */
#define SCRLEFT1(lw, n)	((lw) >> (n))
#define SCRRIGHT1(lw, n) ((lw) << (n))
#endif	/* (BITMAP_BIT_ORDER == MSBFirst) */

#define maskbits(x, w, startmask, endmask, nlw) \
    startmask = sistarttab((x)&PIM); \
    endmask = siendtab(((x)+(w)) & PIM); \
    if (startmask) \
	nlw = (((w) - (PPW - ((x)&PIM))) >> PWSH); \
    else \
	nlw = (w) >> PWSH;

#define maskpartialbits(x, w, mask) \
    mask = sistartpartial((x) & PIM) & siendpartial(((x) + (w)) & PIM);

#define mask32bits(x, w, startmask, endmask) \
    startmask = sistarttab((x)&PIM); \
    endmask = siendtab(((x)+(w)) & PIM);


#define getbits(psrc, x, w, dst) \
if ( ((x) + (w)) <= PPW) \
{ \
    dst = SCRLEFT(*(psrc), (x)); \
} \
else \
{ \
    int m; \
    m = PPW-(x); \
    dst = (SCRLEFT(*(psrc), (x)) & siendtab(m)) | \
	  (SCRRIGHT(*((psrc)+1), m) & sistarttab(m)); \
}


#define putbits(src, x, w, pdst, planemask) \
if ( ((x)+(w)) <= PPW) \
{ \
    unsigned long tmpmask; \
    maskpartialbits((x), (w), tmpmask); \
    tmpmask &= planemask; \
    *(pdst) = (*(pdst) & ~tmpmask) | (SCRRIGHT(src, x) & tmpmask); \
} \
else \
{ \
    unsigned long m; \
    unsigned long n; \
    unsigned long pm = planemask; \
    m = PPW-(x); \
    n = (w) - m; \
    *(pdst) = (*(pdst) & (siendtab(x) | ~pm)) | \
	(SCRRIGHT(src, x) & (sistarttab(x) & pm)); \
    *((pdst)+1) = (*((pdst)+1) & (sistarttab(n) | ~pm)) | \
	(SCRLEFT(src, m) & (siendtab(n) & pm)); \
}

#define putbitsrop(src, x, w, pdst, planemask, rop) \
if ( ((x)+(w)) <= PPW) \
{ \
    unsigned long tmpmask; \
    unsigned long t1, t2; \
    maskpartialbits((x), (w), tmpmask); \
    tmpmask &= planemask; \
    t1 = SCRRIGHT((src), (x)); \
    t2 = DoRop(rop, t1, *(pdst)); \
    *(pdst) = (*(pdst) & ~tmpmask) | (t2 & tmpmask); \
} \
else \
{ \
    unsigned long m; \
    unsigned long n; \
    unsigned long t1, t2; \
    unsigned long pm = planemask; \
    m = PPW-(x); \
    n = (w) - m; \
    t1 = SCRRIGHT((src), (x)); \
    t2 = DoRop(rop, t1, *(pdst)); \
    *(pdst) = (*(pdst) & (siendtab(x) | ~pm)) | (t2 & (sistarttab(x) & pm));\
    t1 = SCRLEFT((src), m); \
    t2 = DoRop(rop, t1, *((pdst) + 1)); \
    *((pdst)+1) = (*((pdst)+1) & (sistarttab(n) | ~pm)) | \
	(t2 & (siendtab(n) & pm)); \
}

#define getleftbits(psrc, w, dst) \
    dst = *(psrc);
