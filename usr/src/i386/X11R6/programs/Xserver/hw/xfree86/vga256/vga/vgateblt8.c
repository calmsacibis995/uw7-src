/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/vga/vgateblt8.c,v 3.1 1995/01/28 16:14:52 dawes Exp $ */
/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
*/

/* $XConsortium: vgateblt8.c /main/2 1995/11/13 09:27:15 kaleb $ */
/*
 * TEGblt - ImageText expanded glyph fonts only.  For
 * 8 bit displays, in Copy mode with no clipping.
 */


#include	"vga256.h"

#ifdef SPEEDUP
#define SIMPLER_CHECKRWO(x) {if(vgaWriteFlag) x = vgaReadWriteNext(x);}
#endif


/*
 * this code supports up to 5 characters at a time.  The performance
 * differences between 4 and 5 is usually small (~7% on PMAX) and
 * frequently negative (SPARC and Sun3), so this file is compiled
 * only once for now.  If you want to use the other options, you'll
 * need to hack cfbgc.c as well.
 */


#ifndef NGLYPHS
#define NGLYPHS 4
#define DO_COMMON
#endif


#ifdef DO_COMMON
#ifdef SPEEDUP
#define CFBTEGBLT8 speedupvga256TEGlyphBlt8
#else
#define CFBTEGBLT8 vga256TEGlyphBlt8
#endif
#endif

/*
 * On little-endian machines (or where fonts are padded to 32-bit
 * boundaries) we can use some magic to avoid the expense of getleftbits
 */

#if ((BITMAP_BIT_ORDER == LSBFirst && NGLYPHS >= 4) || GLYPHPADBYTES == 4)

#if GLYPHPADBYTES == 1
typedef unsigned char	*glyphPointer;
#define USE_LEFTBITS
#endif

#if GLYPHPADBYTES == 2
typedef unsigned short	*glyphPointer;
#define USE_LEFTBITS
#endif

#if GLYPHPADBYTES == 4
typedef unsigned int	*glyphPointer;
#endif

#define GetBitsL    c = BitLeft (*leftChar++, lshift)

#define GetBits1S   c = BitRight(*char1++, xoff1)
#define GetBits1L   GetBitsL | BitRight(*char1++, xoff1)
#define GetBits1U   c = *char1++
#define GetBits2S   GetBits1S | BitRight(*char2++, xoff2)
#define GetBits2L   GetBits1L | BitRight(*char2++, xoff2)
#define GetBits2U   GetBits1U | BitRight(*char2++, xoff2)
#define GetBits3S   GetBits2S | BitRight(*char3++, xoff3)
#define GetBits3L   GetBits2L | BitRight(*char3++, xoff3)
#define GetBits3U   GetBits2U | BitRight(*char3++, xoff3)
#define GetBits4S   GetBits3S | BitRight(*char4++, xoff4)
#define GetBits4L   GetBits3L | BitRight(*char4++, xoff4)
#define GetBits4U   GetBits3U | BitRight(*char4++, xoff4)
#define GetBits5S   GetBits4S | BitRight(*char5++, xoff5)
#define GetBits5L   GetBits4L | BitRight(*char5++, xoff5)
#define GetBits5U   GetBits4U | BitRight(*char5++, xoff5)

#else

typedef unsigned char	*glyphPointer;

#define USE_LEFTBITS

#define GetBitsL    WGetBitsL
#define GetBits1S   WGetBits1S
#define GetBits1L   WGetBits1L
#define GetBits1U   WGetBits1U

#define GetBits2S   GetBits1S Get1Bits (char2, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff2);
#define GetBits2L   GetBits1L Get1Bits (char2, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff2);
#define GetBits2U   GetBits1U Get1Bits (char2, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff2);

#define GetBits3S   GetBits2S Get1Bits (char3, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff3);
#define GetBits3L   GetBits2L Get1Bits (char3, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff3);
#define GetBits3U   GetBits2U Get1Bits (char3, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff3);

#define GetBits4S   GetBits3S Get1Bits (char4, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff4);
#define GetBits4L   GetBits3L Get1Bits (char4, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff4);
#define GetBits4U   GetBits3U Get1Bits (char4, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff4);

#define GetBits5S   GetBits4S Get1Bits (char5, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff5);
#define GetBits5L   GetBits4L Get1Bits (char5, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff5);
#define GetBits5U   GetBits4U Get1Bits (char5, tmpSrc) \
		    c |= BitRight(tmpSrc, xoff5);

#endif

#ifdef USE_LEFTBITS
extern long endtab[];

#define IncChar(c)  (c = (glyphPointer) (((char *) c) + glyphBytes))

#define Get1Bits(ch,dst)    glyphbits (ch, widthGlyph, glyphMask, dst); \
			    IncChar (ch);

#define glyphbits(bits,width,mask,dst)	getleftbits(bits,width,dst); \
					dst &= mask;

#define WGetBitsL   Get1Bits(leftChar,c); \
		    c = BitLeft (c, lshift);
#define WGetBits1S  Get1Bits (char1, c) \
		    c = BitRight (c, xoff1);
#define WGetBits1L  WGetBitsL Get1Bits (char1, tmpSrc) \
		    c |= BitRight (tmpSrc, xoff1);
#define WGetBits1U  Get1Bits (char1, c)

#else
#define WGetBitsL   GetBitsL
#define WGetBits1S  GetBits1S
#define WGetBits1L  GetBits1L
#define WGetBits1U  GetBits1U
#endif

#if NGLYPHS == 2
# define GetBitsNS GetBits2S
# define GetBitsNL GetBits2L
# define GetBitsNU GetBits2U
# define LastChar char2
#ifndef CFBTEGBLT8
# define CFBTEGBLT8 vga256TEGlyphBlt8x2
#endif
#endif
#if NGLYPHS == 3
# define GetBitsNS GetBits3S
# define GetBitsNL GetBits3L
# define GetBitsNU GetBits3U
# define LastChar char3
#ifndef CFBTEGBLT8
# define CFBTEGBLT8 vga256TEGlyphBlt8x3
#endif
#endif
#if NGLYPHS == 4
# define GetBitsNS GetBits4S
# define GetBitsNL GetBits4L
# define GetBitsNU GetBits4U
# define LastChar char4
#ifndef CFBTEGBLT8
# define CFBTEGBLT8 vga256TEGlyphBlt8x4
#endif
#endif
#if NGLYPHS == 5
# define GetBitsNS GetBits5S
# define GetBitsNL GetBits5L
# define GetBitsNU GetBits5U
# define LastChar char5
#ifndef CFBTEGBLT8
# define CFBTEGBLT8 vga256TEGlyphBlt8x5
#endif
#endif

/* another ugly giant macro */
#ifdef SPEEDUP
#define SwitchEm    hTmp = SpeedUpRowsNext[hGlenn = 0]; \
		    switch (ew) \
		    { \
		    case 0: \
		    	break; \
		    case 1: \
do { \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 \
			    Loop \
		    	} \
    hTmp = SpeedUpRowsNext[++hGlenn]; \
    SIMPLER_CHECKRWO(dst) \
} while (hTmp); \
		    	break; \
		    case 2: \
do { \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) \
			    Loop \
		    	} \
    hTmp = SpeedUpRowsNext[++hGlenn]; \
    SIMPLER_CHECKRWO(dst) \
} while (hTmp); \
		    	break; \
		    case 3: \
do { \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step StoreBits(2) \
			    Loop \
		    	} \
    hTmp = SpeedUpRowsNext[++hGlenn]; \
    SIMPLER_CHECKRWO(dst) \
} while (hTmp); \
		    	break; \
		    case 4: \
do { \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
			    StoreBits(2) Step StoreBits(3) \
			    Loop \
		    	} \
    hTmp = SpeedUpRowsNext[++hGlenn]; \
    SIMPLER_CHECKRWO(dst) \
} while (hTmp); \
		    	break; \
		    case 5: \
do { \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
			    StoreBits(2) Step StoreBits(3) Step \
			    StoreBits(4) \
			    Loop \
		    	} \
    hTmp = SpeedUpRowsNext[++hGlenn]; \
    SIMPLER_CHECKRWO(dst) \
} while (hTmp); \
		    	break; \
		    case 6: \
do { \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
 			    StoreBits(2) Step StoreBits(3) Step \
			    StoreBits(4) Step StoreBits(5) \
			    Loop \
		    	} \
    hTmp = SpeedUpRowsNext[++hGlenn]; \
    SIMPLER_CHECKRWO(dst) \
} while (hTmp); \
		    	break; \
		    case 7: \
do { \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
			    StoreBits(2) Step StoreBits(3) Step \
			    StoreBits(4) Step StoreBits(5) Step \
			    StoreBits(6) \
			    Loop \
		    	} \
    hTmp = SpeedUpRowsNext[++hGlenn]; \
    SIMPLER_CHECKRWO(dst) \
} while (hTmp); \
		    	break; \
		    case 8: \
do { \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
			    StoreBits(2) Step StoreBits(3) Step \
			    StoreBits(4) Step StoreBits(5) Step \
			    StoreBits(6) Step StoreBits(7) \
			    Loop \
		    	} \
    hTmp = SpeedUpRowsNext[++hGlenn]; \
    SIMPLER_CHECKRWO(dst) \
} while (hTmp); \
		    	break; \
		    }

#else /* SPEEDUP */
#define SwitchEm   switch (ew) \
		    { \
		    case 0: \
		    	break; \
		    case 1: \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 \
			    Loop \
		    	} \
		    	break; \
		    case 2: \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) \
			    Loop \
		    	} \
		    	break; \
		    case 3: \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step StoreBits(2) \
			    Loop \
		    	} \
		    	break; \
		    case 4: \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
			    StoreBits(2) Step StoreBits(3) \
			    Loop \
		    	} \
		    	break; \
		    case 5: \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
			    StoreBits(2) Step StoreBits(3) Step \
			    StoreBits(4) \
			    Loop \
		    	} \
		    	break; \
		    case 6: \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
 			    StoreBits(2) Step StoreBits(3) Step \
			    StoreBits(4) Step StoreBits(5) \
			    Loop \
		    	} \
		    	break; \
		    case 7: \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
			    StoreBits(2) Step StoreBits(3) Step \
			    StoreBits(4) Step StoreBits(5) Step \
			    StoreBits(6) \
			    Loop \
		    	} \
		    	break; \
		    case 8: \
		    	while (hTmp--) { \
			    GetBits; \
			    StoreBits0 FirstStep StoreBits(1) Step \
			    StoreBits(2) Step StoreBits(3) Step \
			    StoreBits(4) Step StoreBits(5) Step \
			    StoreBits(6) Step StoreBits(7) \
			    Loop \
		    	} \
		    	break; \
		    }
#endif /* SPEEDUP */

#ifdef FAST_CONSTANT_OFFSET_MODE
#define StorePixels(o,p)    dst[o] = p
#define Loop		    dst += widthDst;
#else
#ifdef SPEEDUP
#define StorePixels(o,p)    *dst = (p); dst++;
#define Loop		    dst += widthLeft;
#else
#define StorePixels(o,p)    *dst = (p); dst++; CHECKRWO(dst);
/* *dst++ = (p); CHECKRWO(dst); */
#define Loop              dst += widthLeft; CHECKRWO(dst);
#endif
#endif

#define Step		    NextBitGroup(c);

#if (BITMAP_BIT_ORDER == MSBFirst)
#define StoreBits(o)	StorePixels(o,GetPixelGroup(c));
#define FirstStep	Step
#else
#define StoreBits(o)	StorePixels(o,*((unsigned long *) (((char *) cfb8Pixels) + (c & 0x3c))));
#define FirstStep	c = BitLeft (c, 2);
#endif

void
CFBTEGBLT8 (pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	xInit, yInit;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    pointer	pglyphBase;	/* start of array of glyphs */
{
    register unsigned long  c;
    register unsigned long  *dst EDI;
    register unsigned long  leftMask, rightMask;
    register int	    hTmp;
    register int	    xoff1;
    register int	    xoff2;
#if NGLYPHS >= 3
    register int	    xoff3;
#endif
#if NGLYPHS >= 4
    register int	    xoff4;
#endif
#if NGLYPHS >= 5
    register int	    xoff5;
#endif
    register glyphPointer   char1;
    register glyphPointer   char2;
#if NGLYPHS >= 3
    register glyphPointer   char3;
#endif
#if NGLYPHS >= 4
    register glyphPointer   char4;
#endif
#if NGLYPHS >= 5
    register glyphPointer   char5;
#endif


#ifdef SPEEDUP
    int hGlenn;
#endif


    FontPtr		pfont = pGC->font;
    unsigned long	*dstLine;
    glyphPointer	oldRightChar;
    unsigned long	*pdstBase;
    glyphPointer	leftChar;
    int			widthDst, widthLeft;
    int			widthGlyph;
    int			h;
    int			ew;
    int			x, y;
    BoxRec		bbox;		/* for clipping */
    int			lshift;
    int			widthGlyphs;
#ifdef USE_LEFTBITS
    register unsigned long  glyphMask;
    register unsigned long  tmpSrc;
    register int	    glyphBytes;
#endif

    widthGlyph = FONTMAXBOUNDS(pfont,characterWidth);
    h = FONTASCENT(pfont) + FONTDESCENT(pfont);

#ifdef SPEEDUP
if ((h | widthGlyph) == 0) return;
#endif

#ifndef SPEEDUP
    if (!h)
	return;
#endif

    x = xInit + FONTMAXBOUNDS(pfont,leftSideBearing) + pDrawable->x;
    y = yInit - FONTASCENT(pfont) + pDrawable->y;
    bbox.x1 = x;
    bbox.x2 = x + (widthGlyph * nglyph);
    bbox.y1 = y;
    bbox.y2 = y + h;

    switch (RECT_IN_REGION(pGC->pScreen,  cfbGetCompositeClip(pGC), &bbox))
    {
      case rgnPART:
	cfbImageGlyphBlt8(pDrawable, pGC, xInit, yInit, nglyph, ppci, pglyphBase);
      case rgnOUT:
	return;
    }

    if (!cfb8CheckPixels (pGC->fgPixel, pGC->bgPixel))
	cfb8SetPixels (pGC->fgPixel, pGC->bgPixel);

    leftChar = 0;

    cfbGetLongWidthAndPointer(pDrawable, widthDst, pdstBase)

    BANK_FLAG(pdstBase)

#if NGLYPHS == 2
    widthGlyphs = widthGlyph << 1;
#else
#if NGLYPHS == 4
    widthGlyphs = widthGlyph << 2;
#else
    widthGlyphs = widthGlyph * NGLYPHS;
#endif
#endif

#ifdef USE_LEFTBITS
    glyphMask = endtab[widthGlyph];
    glyphBytes = GLYPHWIDTHBYTESPADDED(*ppci);
#endif

    pdstBase += y * widthDst;


#ifdef SPEEDUP
SpeedUpComputeNext((unsigned)pdstBase, h);
#endif


#ifdef DO_COMMON
    if (widthGlyphs <= 32)
#endif
    	while (nglyph >= NGLYPHS)
    	{
	    nglyph -= NGLYPHS;
	    hTmp = h;
	    dstLine = pdstBase + (x >> PWSH);
	    xoff1 = x & PIM;
	    char1 = (glyphPointer) FONTGLYPHBITS(pglyphBase, *ppci++);
	    char2 = (glyphPointer) FONTGLYPHBITS(pglyphBase, *ppci++);
	    xoff2 = xoff1 + widthGlyph;
#if NGLYPHS >= 3
	    char3 = (glyphPointer) FONTGLYPHBITS(pglyphBase, *ppci++);
	    xoff3 = xoff2 + widthGlyph;
#endif
#if NGLYPHS >= 4
	    char4 = (glyphPointer) FONTGLYPHBITS(pglyphBase, *ppci++);
	    xoff4 = xoff3 + widthGlyph;
#endif
#if NGLYPHS >= 5
	    char5 = (glyphPointer) FONTGLYPHBITS(pglyphBase, *ppci++);
	    xoff5 = xoff4 + widthGlyph;
#endif
	    oldRightChar = LastChar;
	    dst = dstLine;
	    SETRW(dst);
	    if (xoff1)
	    {
		ew = ((widthGlyphs - (PGSZB - xoff1)) >> PWSH) + 1;
#ifndef FAST_CONSTANT_OFFSET_MODE
		widthLeft = widthDst - ew;
#endif
	    	if (!leftChar)
	    	{
		    leftMask = cfbendtab[xoff1];
		    rightMask = cfbstarttab[xoff1];

#define StoreBits0	StorePixels (0,dst[0] & leftMask | \
				       GetPixelGroup(c) & rightMask);
#define GetBits GetBitsNS

		    SwitchEm

#undef GetBits
#undef StoreBits0

	    	}
	    	else
	    	{
		    lshift = widthGlyph - xoff1;
    
#define StoreBits0  StorePixels (0,GetPixelGroup(c));
#define GetBits GetBitsNL
    
		    SwitchEm
    
#undef GetBits
#undef StoreBits0
    
	    	}
	    }
	    else
	    {
#if NGLYPHS == 4
	    	ew = widthGlyph;    /* widthGlyphs >> 2 */
#else
	    	ew = widthGlyphs >> PWSH;
#endif
#ifndef FAST_CONSTANT_OFFSET_MODE
		widthLeft = widthDst - ew;
#endif

#define StoreBits0  StorePixels (0,GetPixelGroup(c));
#define GetBits	GetBitsNU

	    	SwitchEm

#undef GetBits
#undef StoreBits0

	    }
	    x += widthGlyphs;
	    leftChar = oldRightChar;
    	}
    while (nglyph--)
    {
	xoff1 = x & PIM;
	char1 = (glyphPointer) FONTGLYPHBITS(pglyphBase, *ppci++);
	hTmp = h;
	dstLine = pdstBase + (x >> PWSH);
	oldRightChar = char1;
	dst = dstLine;
	SETRW(dst);
	if (xoff1)
	{
	    ew = ((widthGlyph - (PGSZB - xoff1)) >> PWSH) + 1;
#ifndef FAST_CONSTANT_OFFSET_MODE
	    widthLeft = widthDst - ew;
#endif
	    if (!leftChar)
	    {
		leftMask = cfbendtab[xoff1];
		rightMask = cfbstarttab[xoff1];

#define StoreBits0	StorePixels (0,dst[0] & leftMask | GetPixelGroup(c) & rightMask);
#define GetBits	WGetBits1S

		SwitchEm
#undef GetBits
#undef StoreBits0

	    }
	    else
	    {
		lshift = widthGlyph - xoff1;

#define StoreBits0  StorePixels (0,GetPixelGroup(c));
#define GetBits WGetBits1L

		SwitchEm
#undef GetBits
#undef StoreBits0

	    }
	}
	else
	{
	    ew = widthGlyph >> PWSH;

#ifndef FAST_CONSTANT_OFFSET_MODE
	    widthLeft = widthDst - ew;
#endif

#define StoreBits0  StorePixels (0,GetPixelGroup(c));
#define GetBits	WGetBits1U

	    SwitchEm

#undef GetBits
#undef StoreBits0

	}
	x += widthGlyph;
	leftChar = oldRightChar;
    }
    /*
     * draw the tail of the last character
     */
    xoff1 = x & PIM;
    if (xoff1)
    {
	rightMask = cfbstarttab[xoff1];
	leftMask = cfbendtab[xoff1];
	lshift = widthGlyph - xoff1;
	dst = pdstBase + (x >> PWSH);
	SETRW(dst);

#ifndef SPEEDUP
	hTmp = h;
#else
	hTmp = SpeedUpRowsNext[hGlenn = 0];
	do {
#endif
	    while (hTmp--)
	    {
	        GetBitsL;
	        *dst = (*dst & rightMask) | (GetPixelGroup(c) & leftMask);
	        dst += widthDst;
#ifndef SPEEDUP
		CHECKRWO(dst);
#endif
	    }
#ifdef SPEEDUP
            hTmp = SpeedUpRowsNext[++hGlenn];
            SIMPLER_CHECKRWO(dst)
        } while (hTmp);
#endif


    }
}
