/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cir_driver.h,v 3.20 1996/01/13 12:22:08 dawes Exp $ */
/*
 *
 * Copyright 1993 by Simon P. Cooper, New Brunswick, New Jersey, USA.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Simon P. Cooper not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Simon P. Cooper makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * SIMON P. COOPER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL SIMON P. COOPER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Simon P. Cooper, <scooper@vizlab.rutgers.edu>
 * Modified: Harm Hanemaayer, <hhanemaa@cs.ruu.nl>
 *
 * cir_driver.h,v 1.8 1994/09/14 13:58:59 scooper Exp
 */
/* $XConsortium: cir_driver.h /main/10 1996/01/13 13:14:55 kaleb $ */

#define CIRRUS_INCLUDE_COPYPLANE1TO8

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

extern void CirrusFillRectSolidCopy();		/* GXcopy fill. */
extern void CirrusFillRectSolidGeneral();	/* Non-GXcopy fill. */
/* In cir_blt.c: */
extern void CirrusPolyBitBlt();

/* LowlevelFuncs: */

extern void CirrusDoBitbltCopy();
extern void CirrusFillBoxSolid();

extern void CirrusFillRectOpaqueStippled32();
extern void CirrusFillRectTransparentStippled32();
extern void CirrusFillRectTile();

/* Higher level functions: */

extern void CirrusSegmentSS();
extern void CirrusLineSS();

extern void CirrusImageGlyphBlt();
extern void CirrusPolyGlyphBlt();
extern void CirrusMMIOImageGlyphBlt();
extern void CirrusMMIOPolyGlyphBlt();

extern void CirrusPolyFillRect();
extern void CirrusFillSpans();

/* Low-level graphics display functions: */

/* In cir_blt.c: */
extern void CirrusBitBlt();
#ifdef CIRRUS_INCLUDE_COPYPLANE1TO8
extern void CirrusCopyPlane1to8();
#endif

/* In cir_imageblt.s:  */
#if NeedFunctionPrototypes
extern void CirrusImageWriteTransfer( int w, int h, void *srcaddr,
	int srcwidth, void *vaddr );
extern void CirrusImageReadTransfer( int w, int h, void *srcaddr,
	int srcwidth, void *vaddr );
extern void CirrusBitmapTransfer( int bytewidth, int h, int bwidth,
	void *srcaddr, void *vaddr );
#else
extern void CirrusImageWriteTransfer();
extern void CirrusImageReadTransfer();
extern void CirrusAlignedBitmapTransfer();
extern void CirrusBitmapTransfer();
#endif

/* In cir_colorexp.c: */
#if NeedFunctionPrototypes
extern void CirrusColorExpand32bitFill( int x, int y, int w, int h,
	unsigned long *bits, int sh, int sox, int soy, int bg, int fg,
	int destpitch );
extern void Cirrus32bitFillSmall( int x, int y, int w, int h,
	unsigned long *bits, int sh, int sox, int soy, int bg, int fg,
	int destpitch );
extern void CirrusLatchedBitBlt( int x1, int y1, int x2, int y2,
	int w, int h, int destpitch );
extern void CirrusLatchedBitBltReversed( int x1, int y1, int x2, int y2,
	int w, int h, int destpitch );
extern void CirrusSimpleBitBlt( int x1, int y1, int x2, int y2,
  	int w, int h, int destpitch );
extern void CirrusColorExpandFillTile32( int x, int y, int w, int h,
	unsigned char *src, int tpitch, int twidth, int theight, int toy,
	int destpitch );
#else
extern void CirrusColorExpand32bitFill();
extern void Cirrus32bitFillSmall();
extern void CirrusLatchedBitBlt();
extern void CirrusLatchedBitBltReversed();
extern void CirrusSimpleBitBlt();
extern void CirrusColorExpandFillTile32();
#endif
/* In cir_blitter.c: */
#if NeedFunctionPrototypes
extern void CirrusBLTColorExpand8x8PatternFill( unsigned destaddr, int fg,
	int bg,	int w, int h, int destpitch, int rop, unsigned long pword1,
	unsigned long pword2 );
extern void CirrusMMIOBLTColorExpand8x8PatternFill( unsigned destaddr, int fg,
	int bg,	int w, int h, int destpitch, int rop, unsigned long pword1,
	unsigned long pword2 );
extern void CirrusBLT8x8PatternFill( unsigned destaddr, int w, int h,
	unsigned char *pattern, int patternpitch, int destpitch, int rop );
extern void CirrusMMIOBLT8x8PatternFill( unsigned destaddr, int w, int h,
	unsigned char *pattern, int patternpitch, int destpitch, int rop );
extern void CirrusBLT16x16PatternFill( unsigned destaddr, int w, int h,
	unsigned char *pattern, int patternpitch, int destpitch, int rop );
extern void CirrusMMIOBLT16x16PatternFill( unsigned destaddr, int w, int h,
	unsigned char *pattern, int patternpitch, int destpitch, int rop );
extern void CirrusBLTBitBlt( unsigned dstAddr, unsigned srcAddr,
	int dstPitch, int srcPitch, int w, int h, int dir );
extern void CirrusMMIOBLTBitBlt( unsigned dstAddr, unsigned srcAddr,
	int dstPitch, int srcPitch, int w, int h, int dir );
extern void CirrusBLTWaitUntilFinished(void);	
extern void CirrusMMIOBLTWaitUntilFinished(void);	
#else
extern void CirrusBLTColorExpand8x8PatternFill();
extern void CirrusBLT8x8PatternFill();
extern void CirrusBLT16x16PatternFill();
extern void CirrusBLTBitBlt();
extern void CirrusBLTWaitUntilFinished();
extern void CirrusMMIOBLTColorExpand8x8PatternFill();
extern void CirrusMMIOBLT8x8PatternFill();
extern void CirrusMMIOBLT16x16PatternFill();
extern void CirrusMMIOBLTBitBlt();
extern void CirrusMMIOBLTWaitUntilFinished();
#endif
extern void CirrusInvalidateShadowVariables();
/* In cir_im.c: */
extern void CirrusBLTImageWrite();
extern void CirrusMMIOBLTImageWrite();
extern void CirrusBLTImageRead();
extern void CirrusBLTWriteBitmap();
extern void CirrusMMIOBLTWriteBitmap();

/* Functions defined in cir_blt16.c: */
extern RegionPtr Cirrus16CopyArea();
extern RegionPtr Cirrus32CopyArea();
extern void CirrusCopyWindow();
/* cir_fillsp.c */
extern void CirrusFillSolidSpansGeneral();
extern void CirrusMMIOFillRectSolid();
extern void CirrusMMIOFillBoxSolid();
extern void CirrusMMIOBanded32x32PatternFill();
extern void CirrusMMIOPoly32x32PatternFill();
extern void CirrusWriteSolidPattern();
/* cir_line.c */
extern void CirrusMMIOLineSS();
extern void CirrusMMIOSegmentSS();
/* cir_orect.c */
extern void Cirrus8PolyRectangle();
extern void Cirrus16PolyRectangle();
extern void Cirrus32PolyRectangle();

_XFUNCPROTOEND

/* Card type variables; used for selection of accel routines. */

extern int cirrusChip;
extern int cirrusBusType;
extern Bool cirrusUseBLTEngine;
extern Bool cirrusUseMMIO;
extern Bool cirrusUseLinear;
extern Bool cirrusFavourBLT;
extern Bool cirrusAvoidImageBLT;

extern Bool cirrusMMIOFlag;
extern Bool cirrusDoBackgroundBLT;
extern Bool cirrusBLTisBusy;
extern int cirrusBLTPatternAddress;

extern int CirrusMemTop;
extern int cirrusBankShift;

extern int cirrusWriteModeShadow,	/* I/O register shadow variables */
    cirrusPixelMaskShadow,
    cirrusModeExtensionsShadow,
    cirrusBltSrcAddrShadow,
    cirrusBltDestPitchShadow,
    cirrusBltSrcPitchShadow,
    cirrusBltHeightShadow,
    cirrusBltModeShadow,
    cirrusBltRopShadow;
extern unsigned int cirrusForegroundColorShadow,
    cirrusBackgroundColorShadow;


#define CLGD5420    0
#define CLGD5422    1
#define CLGD5424    2
#define CLGD5426    3
#define CLGD5428    4
#define CLGD5429    5
#define CLGD6205    6
#define CLGD6215    7
#define CLGD6225    8
#define CLGD6235    9
#define CLGD5434    10
#define CLGD5430    11
#define CLGD5436    12
#define CLGD7541    13
#define CLGD7542    14
#define CLGD7543    15
#define LASTCLGD    CLGD7543

#define CIRRUS_BUS_SLOW 0
#define CIRRUS_BUS_FAST 1
#define CIRRUS_BUS_ISA 0
#define CIRRUS_BUS_VLB 1
#define CIRRUS_BUS_PCI 2


#define CROP_0			0x00	/*     0 */
#define CROP_1			0x0E	/*     1 */
#define CROP_SRC		0x0D	/*     S */
#define CROP_DST		0x06	/*     D */
#define CROP_NOT_SRC		0xD0	/*    ~S */
#define CROP_NOT_DST		0x0B	/*    ~D */
#define CROP_AND		0x05	/*   S.D */
#define CROP_SRC_AND_NOT_DST	0x09	/*  S.~D */
#define CROP_NOT_SRC_AND_DST	0x50	/*  ~S.D */
#define CROP_NOR		0x90	/* ~S.~D */
#define CROP_OR			0x6D	/*   S+D */
#define CROP_SRC_OR_NOT_DST	0xAD	/*  S+~D */
#define CROP_NOT_SRC_OR_DST	0xD6	/*  ~S+D */
#define CROP_NAND		0xDA	/* ~S+~D */
#define CROP_XOR		0x59	/*  S~=D */
#define CROP_XNOR		0x95	/*   S=D */

/* Array that maps from alu to Cirrus ROP. */

extern int cirrus_rop[];

/* Look-up table for per-byte bit order reversal. */

extern unsigned char byte_reversed[];

typedef struct
{
  int cur_size;
  int cur_select;
  int width;
  int height;
  int cur_addr;
  int hotX;
  int hotY;
  int shiftX;
  int shiftY;
  CursorPtr pCurs;
  int skewed;
} cirrusCurRec, *cirrusCurRecPtr;

#define HAVE543X() (cirrusChip >= CLGD5434)

#define HAVEBITBLTENGINE() (cirrusUseBLTEngine)

#define HAVEBLTWRITEMASK() (cirrusChip == CLGD5429 || \
	cirrusChip == CLGD5430 || cirrusChip == CLGD5436)

#define SETWRITEMODE(n) \
	if (n != cirrusWriteModeShadow) { \
		unsigned char tmp; \
		cirrusWriteModeShadow = n; \
		outb(0x3ce, 0x05); \
		tmp = inb(0x3cf) & 0x20; \
		outb(0x3cf, tmp | (n)); \
	}

#define SETFOREGROUNDCOLOR(c) \
	outw(0x3ce, 0x01 + ((c) << 8));
#if 0
	if ((unsigned char)c != (unsigned char)cirrusForegroundColorShadow) { \
		*(unsigned char *)(&cirrusForegroundColorShadow) = c; \
		outw(0x3ce, 0x01 + ((c) << 8)); \
	}
#endif

#define SETBACKGROUNDCOLOR(c) \
	outw(0x3ce, 0x00 + ((c) << 8));
#if 0
	if ((unsigned char)c != (unsigned char)cirrusBackgroundColorShadow) { \
		*(unsigned char *)(&cirrusBackgroundColorShadow) = c; \
		outw(0x3ce, 0x00 + ((c) << 8)); \
	}
#endif

#define SETFOREGROUNDCOLOR16(c) \
	outw(0x3ce, 0x01 + ((c) << 8)); \
	outw(0x3ce, 0x11 + ((c) & 0xff00));
#if 0
	if ((unsigned short)c != (unsigned short)cirrusForegroundColorShadow) { \
		*(unsigned short *)(&cirrusForegroundColorShadow) = c; \
		outw(0x3ce, 0x01 + ((c) << 8)); \
		outw(0x3ce, 0x11 + ((c) & 0xff00)); \
	}
#endif

#define SETBACKGROUNDCOLOR16(c) \
	outw(0x3ce, 0x00 + ((c) << 8)); \
	outw(0x3ce, 0x10 + ((c) & 0xff00));
#if 0
	if ((unsigned short)c != (unsigned short)cirrusBackgroundColorShadow) { \
		*(unsigned short *)(&cirrusBackgroundColorShadow) = c; \
		outw(0x3ce, 0x00 + ((c) << 8)); \
		outw(0x3ce, 0x10 + ((c) & 0xff00)); \
	}
#endif

#if 0	/* Seems to cause problems. */
#define SETPIXELMASK(m) \
{ \
	int __pixmask; \
	__pixmask = m; \
	if (__pixmask != cirrusPixelMaskShadow) { \
		cirrusPixelMaskShadow = __pixmask; \
		outw(0x3c4, 0x02 + ((__pixmask) << 8)); \
	} \
}
#else
#define SETPIXELMASK(m) \
	outw(0x3c4, 0x02 + ((m) << 8));
#endif

#define ENHANCEDWRITES16	0x10
#define EIGHTDATALATCHES	0x08
#define EXTENDEDWRITEMODES	0x04
#define BY8ADDRESSING		0x02
#define DOUBLEBANKED		0x01
#define SINGLEBANKED		0x00

#define SETMODEEXTENSIONS(m) \
	if (m != cirrusModeExtensionsShadow) { \
		unsigned char tmp; \
		cirrusModeExtensionsShadow = m; \
		outb(0x3ce, 0x0b); \
		tmp = inb(0x3cf) & 0x20; \
		outb(0x3cf, tmp | (m)); \
	}

/* We use a seperate banking routine with 16K granularity for some accel. */
/* functions. Knows about different bank granularity for 2Mb cards. */

#define setwritebank(n) \
	outw(0x3ce, 0x0a + ((n) << cirrusBankShift));

#define setreadbank(n) \
	outw(0x3ce, 0x09 + ((n) << cirrusBankShift));

#define setbank setreadbank

#ifdef PC98_WAB
#define setwritebank_pc98(n) \
	outw(0x3ce, 0x0a + ((n) << (cirrusBankShift - 1)));

#define setreadbank_pc98(n) \
	outw(0x3ce, 0x09 + ((n) << (cirrusBankShift - 1)));
#endif

/* Set up banking at video address addr. Bank is set, addr adjusted. */
#ifdef PC98_WAB
#define CIRRUSSETREAD(addr) \
	if (!cirrusUseLinear) { \
		setreadbank(addr >> 14); \
		setwritebank((addr >> 14) + 1); \
		addr &= 0x3fff; \
	}
#else
#define CIRRUSSETREAD(addr) \
	if (!cirrusUseLinear) { \
		setreadbank(addr >> 14); \
		addr &= 0x3fff; \
	}
#endif

#ifdef PC98_WAB
#define CIRRUSSETWRITE(addr) \
	if (!cirrusUseLinear) { \
		setreadbank(addr >> 14); \
		setwritebank((addr >> 14) + 1); \
		addr &= 0x3fff; \
	}
#else
#define CIRRUSSETWRITE(addr) \
	if (!cirrusUseLinear) { \
		setwritebank(addr >> 14); \
		addr &= 0x3fff; \
	}
#endif

#ifdef PC98_WAB
#define CIRRUSSETREAD_WAB(addr) \
	if (!cirrusUseLinear) { \
		setreadbank_pc98(addr >> 13); \
		addr &= 0x1fff; \
	}

#define CIRRUSSETWRITE_WAB(addr) \
	if (!cirrusUseLinear) { \
		setwritebank_pc98(addr >> 13); \
		addr &= 0x1fff; \
	}
#endif

#ifdef PC98_WAB
#define CIRRUSSETSINGLE(addr) \
	if (!cirrusUseLinear) { \
		setreadbank_pc98(addr >> 13); \
		setwritebank_pc98((addr >> 13)+1); \
		addr &= 0x1fff; \
	}
#else
#define CIRRUSSETSINGLE CIRRUSSETREAD
#endif

/* Similar, but also assigns the bank value to a variable. */
#ifdef PC98_WAB
#define CIRRUSSETREADB(addr, bank) \
	if (!cirrusUseLinear) { \
		bank = addr >> 14; \
		setreadbank(bank); \
		setwritebank(bank + 1); \
		addr &= 0x3fff; \
	}
#else
#define CIRRUSSETREADB(addr, bank) \
	if (!cirrusUseLinear) { \
		bank = addr >> 14; \
		setreadbank(bank); \
		addr &= 0x3fff; \
	}
#endif

#ifdef PC98_WAB
#define CIRRUSSETWRITEB(addr, bank) \
	if (!cirrusUseLinear) { \
		bank = addr >> 14; \
		setreadbank(bank); \
		setwritebank(bank + 1); \
		addr &= 0x3fff; \
	}
#else
#define CIRRUSSETWRITEB(addr, bank) \
	if (!cirrusUseLinear) { \
		bank = addr >> 14; \
		setwritebank(bank); \
		addr &= 0x3fff; \
	}
#endif

#ifdef PC98_WAB
#define CIRRUSSETREADB_WAB(addr, bank) \
	if (!cirrusUseLinear) { \
		bank = addr >> 13; \
		setreadbank_pc98(bank); \
		addr &= 0x1fff; \
	}

#define CIRRUSSETWRITEB_WAB(addr, bank) \
	if (!cirrusUseLinear) { \
		bank = addr >> 13; \
		setwritebank_pc98(bank); \
		addr &= 0x1fff; \
	}
#endif

#define CIRRUSSETSINGLEB CIRRUSSETREADB

/* Adjust the banking address, and maximize the size of the banking */
/* region for the current address/bank. */
#ifdef PC98_WAB
#define CIRRUSCHECKREADB(addr, bank) \
	if (!cirrusUseLinear && addr >= 0x4000) { \
		addr -= 0x4000; \
		bank++; \
		if (addr >= 0x4000) { \
			addr -= 0x4000; \
			bank++; \
		} \
		setreadbank(bank); \
		setwritebank(bank + 1); \
	}
#else
#define CIRRUSCHECKREADB(addr, bank) \
	if (!cirrusUseLinear && addr >= 0x4000) { \
		addr -= 0x4000; \
		bank++; \
		if (addr >= 0x4000) { \
			addr -= 0x4000; \
			bank++; \
		} \
		setreadbank(bank); \
	}
#endif

#ifdef PC98_WAB
#define CIRRUSCHECKWRITEB(addr, bank) \
	if (!cirrusUseLinear && addr >= 0x4000) { \
		addr -= 0x4000; \
		bank++; \
		if (addr >= 0x4000) { \
			addr -= 0x4000; \
			bank++; \
		} \
		setreadbank(bank); \
		setwritebank(bank + 1); \
	}
#else
#define CIRRUSCHECKWRITEB(addr, bank) \
	if (!cirrusUseLinear && addr >= 0x4000) { \
		addr -= 0x4000; \
		bank++; \
		if (addr >= 0x4000) { \
			addr -= 0x4000; \
			bank++; \
		} \
		setwritebank(bank); \
	}
#endif

#ifdef PC98_WAB
#define CIRRUSCHECKSINGLEB(addr, bank) \
	if (!cirrusUseLinear && addr >= 0x4000) { \
		bank += addr >> 14; \
		addr &= 0x3fff; \
		setreadbank(bank); \
		setwritebank(bank+1); \
	}
#else
#define CIRRUSCHECKSINGLEB(addr, bank) \
	if (!cirrusUseLinear && addr >= 0x4000) { \
		bank += addr >> 14; \
		addr &= 0x3fff; \
		setbank(bank); \
	}
#endif

/* Bank adjust and maximimize size of banking region for routines that */
/* write from bottom to top. */
#ifdef PC98_WAB
#define CIRRUSCHECKREVERSEDREADB(addr, bank, pitch) \
	if (!cirrusUseLinear && addr + (pitch) <= 0x4000) { \
		addr += 0x4000; \
		bank--; \
		if (addr + (pitch) <= 0x4000) { \
			addr += 0x4000; \
			bank--; \
		} \
		setreadbank(bank); \
		setwritebank(bank+1); \
	}
#else
#define CIRRUSCHECKREVERSEDREADB(addr, bank, pitch) \
	if (!cirrusUseLinear && addr + (pitch) <= 0x4000) { \
		addr += 0x4000; \
		bank--; \
		if (addr + (pitch) <= 0x4000) { \
			addr += 0x4000; \
			bank--; \
		} \
		setreadbank(bank); \
	}
#endif

#ifdef PC98_WAB
#define CIRRUSCHECKREVERSEDWRITEB(addr, bank, pitch) \
	if (!cirrusUseLinear && addr + (pitch) <= 0x4000) { \
		addr += 0x4000; \
		bank--; \
		if (addr + (pitch) <= 0x4000) { \
			addr += 0x4000; \
			bank--; \
		} \
		setreadbank(bank); \
		setwritebank(bank + 1); \
	}
#else
#define CIRRUSCHECKREVERSEDWRITEB(addr, bank, pitch) \
	if (!cirrusUseLinear && addr + (pitch) <= 0x4000) { \
		addr += 0x4000; \
		bank--; \
		if (addr + (pitch) <= 0x4000) { \
			addr += 0x4000; \
			bank--; \
		} \
		setwritebank(bank); \
	}
#endif

#ifdef PC98_WAB
#define CIRRUSCHECKREADB_WAB(addr, bank) \
	if (!cirrusUseLinear && addr >= 0x2000) { \
		addr -= 0x2000; \
		bank++; \
		setreadbank_pc98(bank); \
	}

#define CIRRUSCHECKWRITEB_WAB(addr, bank) \
	if (!cirrusUseLinear && addr >= 0x2000) { \
		addr -= 0x2000; \
		bank++; \
		setwritebank_pc98(bank); \
	}

#define CIRRUSCHECKSINGLEB_WAB(addr, bank) \
	if (!cirrusUseLinear && addr >= 0x2000) { \
		bank += addr >> 13; \
		addr &= 0x1fff; \
		setbank(bank); \
	}

/* Bank adjust for routines that write from bottom to top. */
#define CIRRUSCHECKREVERSEDREADB_WAB(addr, bank, pitch) \
	if (!cirrusUseLinear && addr + (pitch) <= 0x2000) { \
		addr += 0x2000; \
		bank--; \
		if (addr + (pitch) <= 0x2000) { \
			addr += 0x2000; \
			bank--; \
		} \
		setreadbank_pc98(bank); \
	}

#define CIRRUSCHECKREVERSEDWRITEB_WAB(addr, bank, pitch) \
	if (!cirrusUseLinear && addr + (pitch) <= 0x2000) { \
		addr += 0x2000; \
		bank--; \
		if (addr + (pitch) <= 0x2000) { \
			addr += 0x2000; \
			bank--; \
		} \
		setwritebank_pc98(bank); \
	}
#endif

/* The pointer base address of the video read/write window. */
#define CIRRUSREADBASE() (cirrusUseLinear ? (unsigned char *)vgaLinearBase \
	: (unsigned char *)vgaBase)
#ifdef PC98_WAB
#define CIRRUSWRITEBASE() (cirrusUseLinear ? (unsigned char *)vgaLinearBase \
	: (unsigned char *)vgaBase + 0x4000)
#else
#define CIRRUSWRITEBASE() (cirrusUseLinear ? (unsigned char *)vgaLinearBase \
	: (unsigned char *)vgaBase + 0x8000)
#endif
#define CIRRUSSINGLEBASE CIRRUSREADBASE
#define CIRRUSBASE CIRRUSREADBASE

/* Number of scanlines that fit in banking region (arbitrary number */
/* for linear addressing mode). */
#ifdef PC98_WAB
#define CIRRUSSINGLEREGIONLINES(addr, pitch) (cirrusUseLinear ? 0xf0000 \
	: (0x8000 - (addr)) / (pitch))
#else
#define CIRRUSSINGLEREGIONLINES(addr, pitch) (cirrusUseLinear ? 0xf0000 \
	: (0x10000 - (addr)) / (pitch))
#endif

#ifdef PC98_WAB
#define CIRRUSWRITEREGIONLINES(addr, pitch) (cirrusUseLinear ? 0xf0000 \
	: (0x4000 - (addr)) / (pitch))
#else
#define CIRRUSWRITEREGIONLINES(addr, pitch) (cirrusUseLinear ? 0xf0000 \
	: (0x8000 - (addr)) / (pitch))
#endif

#define CIRRUSREVERSEDWRITEREGIONLINES(addr, pitch) \
	(cirrusUseLinear ? 0xf0000 : (addr) / (pitch))


#if !defined(__GNUC__) || defined(NO_INLINE)
#undef __inline__
#define __inline__ /**/
#endif
