/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cir_driver.c,v 3.48 1996/01/13 12:22:06 dawes Exp $ */
/*
 * cir_driver.c,v 1.10 1994/09/14 13:59:50 scooper Exp
 *
 * Copyright 1993 by Bill Reynolds, Santa Fe, New Mexico
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Bill Reynolds not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Bill Reynolds makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * BILL REYNOLDS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL BILL REYNOLDS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Bill Reynolds, bill@goshawk.lanl.gov
 * Modifications: David Dawes, <dawes@physics.su.oz.au>
 * Modifications: Piercarlo Grandi, Aberystwyth (pcg@aber.ac.uk)
 * Modifications: Simon P. Cooper, <scooper@vizlab.rutgers.edu>
 * Modifications: Wolfgang Jung, <wong@cs.tu-berlin.de>
 * Modifications: Harm Hanemaayer, <hhanemaa@cs.ruu.nl>
 *
 */
/* $XConsortium: cir_driver.c /main/14 1996/01/13 13:14:49 kaleb $ */

/* 
 * Modifications to this file for the Cirrus 62x5 chips and color LCD
 * displays were made by Prof. Hank Dietz, Purdue U. School of EE, W.
 * Lafayette, IN, 47907-1285.  These modifications were made very
 * quickly and tested only on a Sager 8200 laptop running Linux SLS
 * 1.03, where they appear to work.  In any case, both Hank and Purdue
 * offer these modifications with the same disclaimers and conditions
 * invoked by Bill Reynolds above:  use these modifications at your
 * own risk and don't blame us.  Neither should you infer that Purdue
 * endorses anything.
 *
 *					hankd@ecn.purdue.edu
 */

/*
 * Note: defining ALLOW_OUT_OF_SPEC_CLOCKS will allow the driver to program
 * clock frequencies higher than those recommended in the Cirrus data book.
 * If you enable this, you do so at your OWN risk, and YOU RISK DAMAGING
 * YOUR HARDWARE.  You have been warned.
 */

#undef ALLOW_OUT_OF_SPEC_CLOCKS
#ifdef ALLOW_OUT_OF_SPEC_CLOCKS
#define MAX_OUT_OF_SPEC_CLOCK	100500
#endif

/* Allow pixel multiplexing for the 5434 in 256 color modes to support */
/* dot clocks up to 110 MHz (later chip versions may go up to 135 MHz). */

#define ALLOW_8BPP_MULTIPLEXING

/* Allow optional Memory-Mapped I/O on 543x. */

#if defined(__GNUC__) || defined(__STDC__)

#define CIRRUS_SUPPORT_MMIO

#endif

/* Allow optional linear addressing. */

#define CIRRUS_SUPPORT_LINEAR


#include "X.h"
#include "input.h"
#include "screenint.h"
#include "dix.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Procs.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "vga.h"
#include "region.h"
#include "vgaPCI.h"

#ifdef XFreeXDGA
#include "X.h"
#include "Xproto.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif


#ifdef XF86VGA16
#define MONOVGA
#endif

/*
 * This driver supports 16-color (mono, vga16), and in the SVGA server
 * 8bpp, 16bpp and 32bpp depending on vgaBitsPerPixel.
 */

#include "cir_driver.h"
#include "cir_alloc.h"
#ifndef MONOVGA
#include "vga256.h"
#endif

int cirrusChip;
int cirrusChipRevision;
int cirrusBusType;
Bool cirrusUseBLTEngine = FALSE;
Bool cirrusUseMMIO = FALSE;
Bool cirrusMMIOFlag = FALSE;
unsigned char *cirrusMMIOBase = NULL;
Bool cirrusUseLinear = FALSE;
Bool cirrusFavourBLT = FALSE;
Bool cirrusAvoidImageBLT = FALSE;
int cirrusDRAMBandwidth;
int cirrusDRAMBandwidthLimit;
int cirrusReprogrammedMCLK = 0;

#define CLAVGA2_ID  0x06
#define CLGD5420_ID 0x22
#define CLGD5422_ID 0x23
#define CLGD5424_ID 0x25
#define CLGD5426_ID 0x24
#define CLGD5428_ID 0x26
#define CLGD5429_ID 0x27
#define CLGD6205_ID 0x02
#define CLGD6215_ID 0x22  /* Hmmm... looks like a 5420 or 5422 */
#define CLGD6225_ID 0x32
#define CLGD6235_ID 0x06	/* It's not 0x12. */
				/* XXXX need to add 6245. */
#define CLGD5434_OLD_ID 0x29
#define CLGD5434_ID 0x2A	/* CL changed the ID at the last minute. */
#define CLGD5430_ID 0x28
#define CLGD5436_ID 0x2B

#define CLGD7541_ID 0x0a	/* guess */
#define CLGD7542_ID 0x0b
#define CLGD7543_ID 0x0c

#define Is_62x5(x)  ((x) >= CLGD6205 && (x) <= CLGD6235)
#define Is_754x(x)  ((x) >= CLGD7541 && (x) <= CLGD7543)

/* <scooper>
 * The following will need updating for other chips in the cirrus
 * family that support a hardware cursor.  I only have data for the 542x
 * series.
 * The 543x should be compatible. -HH
 */

#define Has_HWCursor(x) (((x) >= CLGD5422 && (x) <= CLGD5429) || \
    (x) == CLGD5430 || (x) == CLGD5434 || (x) == CLGD5436)

/* Define a structure for the HIDDEN DAC cursor colours */
typedef struct {
  unsigned char red;		/* DAC red */
  unsigned char green;		/* DAC green */
  unsigned char blue;		/* DAC blue */
} DACcolourRec;
				/* For now, only save a couple of the */
				/* extensions. */
typedef struct {
  vgaHWRec std;               /* good old IBM VGA */
  unsigned char GR9;		/* Graphics Offset1 */
  unsigned char GRA;		/* Graphics Offset2 */
  unsigned char GRB;		/* Graphics Extensions Control */
  unsigned char GRF;		/* Display Compression Control */
  unsigned char SR7;		/* Extended Sequencer */
  unsigned char SRE;		/* VCLK Numerator */
  unsigned char SRF;		/* DRAM Control */
  unsigned char SR10;		/* Graphics Cursor X Position [7:0]         */
  unsigned char SR10E;		/* Graphics Cursor X Position [7:5] | 10000 */
  unsigned char SR11;		/* Graphics Cursor Y Position [7:0]         */
  unsigned char SR11E;		/* Graphics Cursor Y Position [7:5] | 10001 */
  unsigned char SR12;		/* Graphics Cursor Attributes Register */
  unsigned char SR13;		/* Graphics Cursor Pattern Address */
  unsigned char SR16;		/* Performance Tuning Register */
  unsigned char SR17;		/* Configuration/Extended Control Register */
  unsigned char SR1E;		/* VCLK Denominator */
  unsigned char SR1F;		/* MCLK Register */
  unsigned char CR19;		/* Interlace End */
  unsigned char CR1A;		/* Miscellaneous Control */
  unsigned char CR1B;		/* Extended Display Control */
  unsigned char CR1D;		/* Overlay Extended Control Register */
  unsigned char CR2A;		/* 754x */
  unsigned char CR2B;		/* 754x */
  unsigned char CR2C;		/* 754x */
  unsigned char CR2D;		/* 754x */
  unsigned char RAX;		/* 62x5 LCD Timing -- TFT HSYNC */
  unsigned char HIDDENDAC;	/* Hidden DAC Register */
  DACcolourRec  FOREGROUND;     /* Hidden DAC cursor foreground colour */
  DACcolourRec  BACKGROUND;     /* Hidden DAC cursor background colour */
} vgacirrusRec, *vgacirrusPtr;

unsigned char SavedExtSeq;

static Bool lcd_is_on = FALSE;	/* for 62x5 */

static Bool     cirrusProbe();
static char *   cirrusIdent();
static Bool     cirrusClockSelect();
static void     cirrusEnterLeave();
static Bool     cirrusInit();
static Bool     cirrusValidMode();
static void *   cirrusSave();
static void     cirrusRestore();
static void     cirrusAdjust();
static void	cirrusFbInit();

extern void     cirrusSetRead();
extern void     cirrusSetWrite();
extern void     cirrusSetReadWrite();

extern void     cirrusSetRead2MB();
extern void     cirrusSetWrite2MB();
extern void     cirrusSetReadWrite2MB();

extern void 	cirrusCursorInit();
extern void 	cirrusRestoreCursor();
extern void 	cirrusWarpCursor();
extern void 	cirrusQueryBestSize();

extern vgaHWCursorRec vgaHWCursor;
extern cirrusCurRec cirrusCur;

int	CirrusMemTop;

#ifdef PC98
extern void crtswitch();
#endif

vgaVideoChipRec CIRRUS = {
#ifndef PC98_WAB
  cirrusProbe,			/* ChipProbe()*/
  cirrusIdent,			/* ChipIdent(); */
  cirrusEnterLeave,		/* ChipEnterLeave() */
  cirrusInit,			/* ChipInit() */
  cirrusValidMode,		/* ChipValidMode() */
  cirrusSave,			/* ChipSave() */
  cirrusRestore,		/* ChipRestore() */
  cirrusAdjust,			/* ChipAdjust() */
  vgaHWSaveScreen,		/* ChipSaveScreen() */
  (void (*)())NoopDDA,		/* ChipGetMode() */
  cirrusFbInit,			/* ChipFbInit() */
  cirrusSetRead,		/* ChipSetRead() */
  cirrusSetWrite,		/* ChipSetWrite() */
  cirrusSetReadWrite,	        /* ChipSetReadWrite() */
#ifndef CIRRUS_SUPPORT_MMIO  
  0x10000,			/* ChipMapSize */
#else
  0x20000,
#endif
  0x08000,			/* ChipSegmentSize, 32k*/
  15,				/* ChipSegmentShift */
  0x7FFF,			/* ChipSegmentMask */
  0x00000, 0x08000,		/* ChipReadBottom, ChipReadTop  */
  0x08000, 0x10000,		/* ChipWriteBottom,ChipWriteTop */
  TRUE,				/* ChipUse2Banks, Uses 2 bank */
  VGA_DIVIDE_VERT,		/* ChipInterlaceType -- don't divide verts */
  {0,},				/* ChipOptionFlags */
  8,				/* ChipRounding */
  FALSE,			/* ChipUseLinearAddressing */
  0,				/* ChipLinearBase */
  0x100000,			/* ChipLinearSize */
  TRUE,				/* ChipHas16bpp */
  TRUE,				/* ChipHas32bpp */
  NULL,				/* ChipBuiltinModes */
  1,				/* ChipClockScaleFactor */
#else
  cirrusProbe,			/* ChipProbe()*/
  cirrusIdent,			/* ChipIdent(); */
  cirrusEnterLeave,		/* ChipEnterLeave() */
  cirrusInit,			/* ChipInit() */
  cirrusValidMode,		/* ChipValidMode() */
  cirrusSave,			/* ChipSave() */
  cirrusRestore,		/* ChipRestore() */
  cirrusAdjust,			/* ChipAdjust() */
  (void (*)())NoopDDA,		/* ChipSaveScreen() */
  (void (*)())NoopDDA,		/* ChipGetMode() */
  cirrusFbInit,			/* ChipFbInit() */
  cirrusSetRead,		/* ChipSetRead() */
  cirrusSetWrite,		/* ChipSetWrite() */
  cirrusSetReadWrite,	        /* ChipSetReadWrite() */
#ifndef CIRRUS_SUPPORT_MMIO  
  0x08000,			/* ChipMapSize */
#else
  0x20000,
#endif
  0x04000,			/* ChipSegmentSize, 16k*/
  14,				/* ChipSegmentShift */
  0x3FFF,			/* ChipSegmentMask */
  0x00000, 0x04000,		/* ChipReadBottom, ChipReadTop  */
  0x04000, 0x08000,		/* ChipWriteBottom,ChipWriteTop */
  TRUE,				/* ChipUse2Banks, Uses 2 bank */
  VGA_DIVIDE_VERT,		/* ChipInterlaceType -- don't divide verts */
  {0,},				/* ChipOptionFlags */
  8,				/* ChipRounding */
  FALSE,			/* ChipUseLinearAddressing */
  0,				/* ChipLinearBase */
  0x10000,			/* ChipLinearSize */
  TRUE,				/* ChipHas16bpp */
  TRUE,				/* ChipHas32bpp */
  NULL,				/* ChipBuiltinModes */
  1,				/* ChipClockScaleFactor */
#endif
};

/*
 * This exists only to force some OS's not to use a 386 I/O bitmap for
 * I/O protection, which is MUCH slower than full I/O permissions.
 */
static unsigned Cirrus_IOPorts[] = { 0x400 };

/*
 * Note: To be able to use 16K bank granularity, we would have to half the
 * read and write window sizes, because (it seems) cfb.banked can't handle
 * a bank granularity different from the segment size.
 * This means that we have to define a seperate set of banking routines in
 * accel functions where the 16K hardware granularity is used.
 */
int cirrusBankShift = 10;

typedef struct {
  unsigned char numer;
  unsigned char denom;
  } cirrusClockRec;

static cirrusClockRec cirrusClockTab[] = {
  { 0x4A, 0x2B },		/* 25.227 */
  { 0x5B, 0x2F },		/* 28.325 */
  { 0x45, 0x30 }, 		/* 41.164 */
  { 0x7E, 0x33 },		/* 36.082 */
  { 0x42, 0x1F },		/* 31.500 */
  { 0x51, 0x3A },		/* 39.992 */
  { 0x55, 0x36 },		/* 45.076 */
  { 0x65, 0x3A },		/* 49.867 */
  { 0x76, 0x34 },		/* 64.983 */
  { 0x7E, 0x32 },		/* 72.163 */
  { 0x6E, 0x2A },		/* 75.000 */
  { 0x5F, 0x22 },		/* 80.013 */   /* all except 5420 */
  { 0x7D, 0x2A },		/* 85.226 */   /* 5426 and 5428 */
  /* These are all too high according to the databook.  They can be enabled
     with the "16clocks" option  *if* this driver has been compiled with
     ALLOW_OUT_OF_SPEC_CLOCKS defined. [542x only] */
  { 0x58, 0x1C },		/* 89.998 */
  { 0x49, 0x16 },		/* 95.019 */
  { 0x46, 0x14 },		/* 100.226 */
  { 0x53, 0x16 },		/* 108.035 */
  { 0x5C, 0x18 },		/* 110.248 */
  { 0x6D, 0x1A },		/* 120.050 */
  { 0x58, 0x14 },		/* 125.998 */
  { 0x6D, 0x18 },		/* 130.055 */
  { 0x42, 0x0E },		/* 134.998 */
};

/* Doubled clocks for 16-bit clocking mode with pixel clock ~< 45 MHz. */

static cirrusClockRec cirrusDoubleClockTab[] = {
  { 0x51, 0x2E },		/* 50.424 =~ 2 * 25.227 */
  { 0x5B, 0x2E },		/* 55.649 =~ 2 * 28.325 */
  { 0x5C, 0x20 }, 		/* 82.328 =~ 2 * 41.164 */
  { 0x7E, 0x32 },		/* 72.163 =~ 2 * 36.082 */
  { 0x42, 0x1E },		/* 62.999 =~ 2 * 31.500 */
  { 0x5F, 0x22 }		/* 80.012 =~ 2 * 39.992 */
};

/* Lowest clock for which palette clock doubling is required on the 5434. */
#define CLOCK_PALETTE_DOUBLING_5434 85500

#define NUM_CIRRUS_CLOCKS (sizeof(cirrusClockTab)/sizeof(cirrusClockRec))

/* CLOCK_FACTOR is double the osc freq in kHz (osc = 14.31818 MHz) */
#define CLOCK_FACTOR 28636

/* clock in kHz is (numer * CLOCK_FACTOR / (denom & 0x3E)) >> (denom & 1) */
#define CLOCKVAL(n, d) \
     ((((n) & 0x7F) * CLOCK_FACTOR / ((d) & 0x3E)) >> ((d) & 1))

static int cirrusClockLimit[] = {
#ifdef MONOVGA
  /* Clock limits for 16-color (planar) mode. */
  80100,	/* 5420 */
  80100,	/* 5422 */
  80100,	/* 5424 */
  85500,	/* 5426 */
  85500,	/* 5428 */
  85500,	/* 5429 */
  65100,	/* 6205  The 62x5 are speced for 65 MHz at 5V, and */
  65100,	/* 6215  40 MHz at 3.3V. */
  65100,	/* 6225 */
  65100,	/* 6235 */
  85500,	/* 5434 */
  85500,	/* 5430 */
  85500,	/* 5434 */
  80100,	/* 7543 */
#else 
  /* Clock limits for 256-color mode. */
  50200,	/* 5420 */
  80100,	/* 5422 */
  80100,	/* 5424 */
  85500,	/* 5426 */
  85500,	/* 5428 */
  85500,	/* 5429 */
  45100,	/* 6205 */
  45100,	/* 6215 */
  45100,	/* 6225 */
  45100,	/* 6235 */
  /*
   * The 5434 should be able to do 110+ MHz, but requires a mode of operation
   * not yet supported by this server to do it.  Without this it is limited
   * to 85MHz.
   * Multiplexing has now been added, but is untested -- HH.
   */
#if defined(ALLOW_8BPP_MULTIPLEXING)
  110300,	/* 5434 */
#else
  85500,	/* 5434 */
#endif
  85500,	/* 5430 */
  135100,	/* 5436 */
  80100,	/* 7541 */
  80100,	/* 7542 */
  80100,	/* 7543 */
#endif
};

static int cirrusClockLimit16bpp[] = {
  /* Clock limits for 16bpp mode. */
  0,		/* 5420 */
  /* VCLK is doubled automatically for clocks < 42.300. */
  40100,	/* 5422 */
  40100,	/* 5424 */
  45100,	/* 5426 */
  45100,	/* 5428 */
  50000,	/* 5429 */
  0, 0, 0, 0,	/* 62x5 */
  85500,	/* 5434 (with >= 2MB DRAM) */
  50000,	/* 5430 */
  85500,	/* 5436 */
};

static int cirrusClockLimit32bpp[] = {
  /* Clock limits for 32bpp mode (5434-only). */
  0, 0, 0,	/* 5420/2/4 */
  0, 0, 0,	/* 5426/8/9 */
  0, 0, 0, 0,	/* 62x5 */
  45100,	/* 5434 */
  0,		/* 5430 */
  45100,	/* 5436 */
};

#define new ((vgacirrusPtr)vgaNewVideoState)

static SymTabRec chipsets[] = {
  { CLGD5420,	"clgd5420" },
  { CLGD5422,	"clgd5422" },
  { CLGD5424,	"clgd5424" },
  { CLGD5426,	"clgd5426" },
  { CLGD5428,	"clgd5428" },
  { CLGD5429,	"clgd5429" },
  { CLGD5430,	"clgd5430" },
  { CLGD5434,	"clgd5434" },
  { CLGD5436,	"clgd5436" },
  { CLGD6205,	"clgd6205" },
  { CLGD6215,	"clgd6215" },
  { CLGD6225,	"clgd6225" },
  { CLGD6235,	"clgd6235" },
  { CLGD7541,	"clgd7541" },
  { CLGD7542,	"clgd7542" },
  { CLGD7543,	"clgd7543" },
  { -1,		"" },
};

/*
 * cirrusIdent -- 
 */
static char *
cirrusIdent(n)
     int n;
{
  if (chipsets[n].token < 0)
    return(NULL);
  else 
    return(chipsets[n].name);
}

/*
 * cirrusCheckClock --
 *	check if the clock is supported by the chipset
 */
static Bool
cirrusCheckClock(chip, clockno)
  int chip;
  int clockno;
{
  unsigned clockval;

  if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions))
      clockval = vga256InfoRec.clock[clockno];
  else
      clockval = CLOCKVAL(cirrusClockTab[clockno].numer,
		          cirrusClockTab[clockno].denom);

  if (clockval > cirrusClockLimit[chip])
  {
    ErrorF("CIRRUS: clock %7.3f is too high for %s (max is %7.3f)\n",
	   clockval / 1000.0, xf86TokenToString(chipsets, chip),
	   cirrusClockLimit[chip] / 1000.0);

#ifdef ALLOW_OUT_OF_SPEC_CLOCKS
    if (OFLG_ISSET(OPTION_16CLKS, &vga256InfoRec.options))
      {
	ErrorF ("CIRRUS: Out of spec. clocks option is enabled\n");
	return (TRUE);
      }
#endif

    return(FALSE);
  }
  return(TRUE);
}

/*
 * cirrusClockSelect --
 *      select one of the possible clocks ...
 */
static Bool
cirrusClockSelect(no)
     int no;
{
  static unsigned char save1, save2, save3;
  unsigned char temp;
  int SR,SR1;


#ifdef DEBUG_CIRRUS
  fprintf(stderr,"Clock NO = %d\n",no);
#endif

#if 0
  SR = 0x7E; SR1 = 0x33;	/* Just in case.... */
#endif

  switch(no)
       {
     case CLK_REG_SAVE:
       save1 = inb(0x3CC);
       outb(0x3C4, 0x0E);
       save2 = inb(0x3C5);
       outb(0x3C4, 0x1E);
       save3 = inb(0x3C5);
       break;
     case CLK_REG_RESTORE:
       outb(0x3C2, save1);
       outw(0x3C4, (save2 << 8) | 0x0E);
       outw(0x3C4, (save3 << 8) | 0x1E);
       break;
     default:
       if (!cirrusCheckClock(cirrusChip, no))
	    return(FALSE);

       if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions)) {
           if (vgaBitsPerPixel == 16 && cirrusChip <= CLGD5424)
           	/* Use the clocking mode whereby the programmed VCLK */
           	/* is double the pixel rate. */
               CirrusSetClock(vga256InfoRec.clock[no] * 2);
           else
               CirrusSetClock(vga256InfoRec.clock[no]);
           return TRUE;
       }

       SR = cirrusClockTab[no].numer;
       SR1 = cirrusClockTab[no].denom;

#ifndef MONOVGA
       if (vgaBitsPerPixel == 16 && cirrusChip <= CLGD5424) {
	   /* Use the clocking mode whereby the programmed VCLK */
	   /* is double the pixel rate. */
	   SR = cirrusDoubleClockTab[no].numer;
	   SR1 = cirrusDoubleClockTab[no].denom;
       }
#endif
				/*  Use VCLK3 for these extended clocks */
       temp = inb(0x3CC);
       outb(0x3C2, temp | 0x0C );
  
#ifdef DEBUG_CIRRUS
       fprintf(stderr,"Misc = %x\n",temp);
       fprintf(stderr,"Miscactual = %x\n",(temp & 0xF3) | 0x0C);
#endif
  
				/* Set SRE and SR1E */
       outb(0x3C4,0x0E);
       temp = inb(0x3C5);
       outb(0x3C5,(temp & 0x80) | (SR & 0x7F));
  
#ifdef DEBUG_CIRRUS
       fprintf(stderr,"SR = %x\n",temp);
       fprintf(stderr,"SRactual = %x\n",(temp & 0x80) | (SR & 0x7F));
#endif

       outb(0x3C4,0x1E);
       temp = inb(0x3C5);
       outb(0x3C5,(temp & 0xC0) | (SR1 & 0x3F));
  
#ifdef DEBUG_CIRRUS
       fprintf(stderr,"SR1 = %x\n",temp);
       fprintf(stderr,"SR1actual = %x\n",(temp & 0xC0) | (SR1 & 0x3F));
#endif
       break;
       }
       return(TRUE);
}

/*
 * cirrusNumClocks --
 *	returns the number of clocks available for the chip
 */
static int
cirrusNumClocks(chip)
     int chip;
{
     cirrusClockRec *rec, *end = cirrusClockTab + NUM_CIRRUS_CLOCKS;

     /* 
      * The 62x5 chips can do marvelous things, but the
      * LCD panels connected to them don't leave much
      * option.  The following forces the cirrus chip to
      * use the slowest clock -- which appears to be what
      * my LCD panel likes best.  Faster clocks seem to
      * cause the LCD display to show noise when things are
      * moved around on the screen.
      */
     /* XXXX might be better/safer to reduce the value in clock limit tab */
     if (lcd_is_on) 
       {
	 return(1);
       }

#ifdef ALLOW_OUT_OF_SPEC_CLOCKS
     if (OFLG_ISSET(OPTION_16CLKS, &vga256InfoRec.options))
       {
	 return (NUM_CIRRUS_CLOCKS);
       }
#endif
     
     for (rec = cirrusClockTab; rec < end; rec++)
          if (CLOCKVAL(rec->numer, rec->denom) > cirrusClockLimit[chip])
               return(rec - cirrusClockTab);
     return(NUM_CIRRUS_CLOCKS);
}

/*
 * cirrusProbe -- 
 *      check up whether a cirrus based board is installed
 */
static Bool
cirrusProbe()
{  
     int cirrusClockNo, i;
     unsigned char lockreg,IdentVal;
     unsigned char id, rev, partstatus;
     unsigned char temp;
     
     /*
      * Set up I/O ports to be used by this card
      */
     xf86ClearIOPortList(vga256InfoRec.scrnIndex);
     xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
     xf86AddIOPorts(vga256InfoRec.scrnIndex, sizeof(Cirrus_IOPorts) /
         sizeof(Cirrus_IOPorts[0]), Cirrus_IOPorts);

     if (vga256InfoRec.chipset)
	  {
	  if (!StrCaseCmp(vga256InfoRec.chipset, "cirrus"))
	       {
               ErrorF("\ncirrus is no longer valid.  Use one of\n");
	       ErrorF("the names listed by the -showconfig option\n");
	       return(FALSE);
               }
          if (!StrCaseCmp(vga256InfoRec.chipset, "clgd543x"))
	       {
               ErrorF("\nclgd543x is no longer valid.  Use one of\n");
	       ErrorF("the names listed by the -showconfig option\n");
	       return(FALSE);
               }
	  cirrusChip = xf86StringToToken(chipsets, vga256InfoRec.chipset);
	  if (cirrusChip >= 0)
	       {
	       cirrusEnterLeave(ENTER); /* Make the timing regs writable */
	       }
	  else
	       {
	       return(FALSE);
	       }
	  }
     else
	  {
	  unsigned char old;
	  xf86EnableIOPorts(vga256InfoRec.scrnIndex);
	  old = rdinx(0x3c4, 0x06);
	  cirrusEnterLeave(ENTER); /* Make the timing regs writable */
	  
	  /* Kited the following from the Cirrus */
	  /* Databook */
	  
	  /* If it's a Cirrus at all, we should be */
	  /* able to read back the lock register */
	  /* we wrote in cirrusEnterLeave() */
	  
	  outb(0x3C4,0x06);
	  lockreg = inb(0x3C5);
	  
	  /* Ok, if it's not 0x12, we're not a Cirrus542X or 62x5. */
	  if (lockreg != 0x12)
	       {
	       wrinx(0x3c4, 0x06, old);
	       cirrusEnterLeave(LEAVE);
	       return(FALSE);
	       }
	  
	  /* OK, it's a Cirrus. Now, what kind of */
	  /* Cirrus? We read in the ident reg, */
	  /* CRTC index 27 */
	  
	  
	  outb(vgaIOBase+0x04, 0x27); IdentVal = inb(vgaIOBase+0x05);
	  
	  cirrusChip = -1;
	  id  = (IdentVal & 0xFc) >> 2;
	  rev = (IdentVal & 0x03);

	  outb(vgaIOBase + 0x04, 0x25); partstatus = inb(vgaIOBase + 0x05);
          cirrusChipRevision = 0x00;

	  switch( id )
	       {
	     case CLGD7541_ID:
	       cirrusChip = CLGD7541;
	       break;
	     case CLGD7542_ID:
	       cirrusChip = CLGD7542;
	       break;
	     case CLGD7543_ID:
	       cirrusChip = CLGD7543;
	       break;
	     case CLGD5420_ID:
#if 0	/* Conflicts with CL-GD6235. */
	     case CLAVGA2_ID:		/* AVGA2 uses 5402 */
#endif
	       cirrusChip = CLGD5420;	/* 5420 or 5402 */
	       /* Check for CL-GD5420-75QC-B */
	       /* It has a Hidden-DAC register. */
	       outb(0x3C6, 0x00);
	       outb(0x3C6, 0xFF);
	       inb(0x3C6); inb(0x3c6); inb(0x3C6); inb(0x3C6);
	       if (inb(0x3C6) != 0xFF)
	           cirrusChipRevision = 0x01;	/* 5420-75QC-B */
	       break;
	     case CLGD5422_ID:
	       cirrusChip = CLGD5422;
	       break;
	     case CLGD5424_ID:
	       cirrusChip = CLGD5424;
	       break;
	     case CLGD5426_ID:
	       cirrusChip = CLGD5426;
	       break;
	     case CLGD5428_ID:
	       cirrusChip = CLGD5428;
	       break;
	     case CLGD5429_ID:
	       if (partstatus >= 0x67)
	           cirrusChipRevision = 0x01;	/* >= Rev. B, fixes BLT */
	       cirrusChip = CLGD5429;
	       break;

	     /* 
	      * LCD driver chips...  the +1 options are because
	      * these chips have one more bit of chip rev level
	      */
	     case CLGD6205_ID:
	     case CLGD6205_ID + 1:
	       cirrusChip = CLGD6205;
	       break;
#if 0
	     /* looks like a 5420...  oh well...  close enough for now */
	     case CLGD6215_ID:
	     /* looks like a 5422...  oh well...  close enough for now */
	     case CLGD6215_ID + 1:
	       cirrusChip = CLGD6215;
	       break;
#endif
	     case CLGD6225_ID:
	     case CLGD6225_ID + 1:
	       cirrusChip = CLGD6225;
	       break;
	     case CLGD6235_ID:
	     case CLGD6235_ID + 1:
	       cirrusChip = CLGD6235;
	       break;

	     /* 'Alpine' family. */
	     case CLGD5434_ID:
	       if ((partstatus & 0xC0) == 0xC0) {
	          /*
	           * Better than Rev. ~D/E/F.
	           * Handles 60 MHz MCLK and 135 MHz VCLK.
	           */
	          cirrusChipRevision = 0x01;
	          cirrusClockLimit[CLGD5434] = 135100;
	       }
	       else
	       if (partstatus == 0x8E)
	       	  /* Intermediate revision, supports 135 MHz VCLK. */
	          cirrusClockLimit[CLGD5434] = 135100;
	       cirrusChip = CLGD5434;
	       break;

	     case CLGD5430_ID:
	       cirrusChip = CLGD5430;
	       break;

	     case CLGD5436_ID:
	       cirrusChip = CLGD5436;
	       break;

	     case CLGD5434_OLD_ID:

	     default:
	       ErrorF("Unknown Cirrus chipset: type 0x%02x, rev %d\n", id, rev);
	       if (id == CLGD5434_OLD_ID)
	          ErrorF("Old pre-production ID for clgd5434 -- please report\n");
	       cirrusEnterLeave(LEAVE);
	       return(FALSE);
	       break;
	       }
	  
	  if (cirrusChip == CLGD5430 || cirrusChip == CLGD5434 ||
	      cirrusChip == CLGD5436) {
	      /* Write sane value to Display Compression Control */
	      /* Register, which may be corrupted by pvga1 driver */
	      /* probe. */
	      outb(0x3ce, 0x0f);
	      temp = inb(0x3cf) & 0xc0;
	      outb(0x3cf, temp);
	  }

	  }
     
     /* OK, we are a Cirrus */

     vga256InfoRec.chipset = xf86TokenToString(chipsets, cirrusChip);

#ifndef MONOVGA
#ifdef XFreeXDGA
     /* we support direct Video mode */

     vga256InfoRec.directMode = XF86DGADirectPresent;
#endif

     if (vgaBitsPerPixel == 16 &&
     (Is_62x5(cirrusChip) || cirrusChip == CLGD5420)) {
         ErrorF("%s %s: %s: Cirrus 62x5 and 5420 chipsets not supported "
             "in 16bpp mode\n",
             XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset);
         CIRRUS.ChipHas16bpp = FALSE;
     }
#endif

     /* 
      * Try to determine special LCD-oriented stuff...
      *
      * [1] LCD only, CRT only, or simultaneous
      * [2] Panel type (if LCD is enabled)
      *
      * Currently, this isn't used for much, but I've put it
      * into this driver so that you'll at least have a clue
      * that the driver isn't the right one for your chipset
      * if it incorrectly identifies the panel configuration.
      * Later versions of this driver will probably do more
      * with this info than just print it out....
      */
     if (Is_62x5(cirrusChip) || Is_754x(cirrusChip)) 
	  {
	  /* Unlock the LCD registers... */
	  if( Is_754x(cirrusChip) )
	      outb(vgaIOBase + 4, 0x2d);
	  else
	      outb(vgaIOBase + 4, 0x1D);
	  temp = inb(vgaIOBase + 5);
	  outb(vgaIOBase + 5, (temp | 0x80));

	  /* LCD only, CRT only, or simultaneous? */
	  outb(vgaIOBase + 4, 0x20);
	  switch (inb(vgaIOBase + 5) & 0x60) 
	       {
	     case 0x60:
	        lcd_is_on = TRUE;
	        ErrorF("CIRRUS: Simultaneous LCD + VGA display\n");
	        break;
	     case 0x40:
	        ErrorF("CIRRUS: VGA display output only\n");
	        break;
	     case 0x20:
	        lcd_is_on = TRUE;
	        ErrorF("CIRRUS: LCD panel display only\n");
	        break;
	     default:
	        ErrorF("CIRRUS: Neither LCD panel nor VGA display!\n");
	        ErrorF("CIRRUS: Probably not a Cirrus CLGD62x5!\n");
	        ErrorF("CIRRUS: Use this driver at your own risk!\n");
	       }

          /* What type of LCD panel do we have? */
	  if (lcd_is_on) 
	       {
	       if (Is_754x(cirrusChip) )
		    {
		    outb(vgaIOBase + 4, 0x43); /* access fine dotclk delay */
		    outb(vgaIOBase + 5, 0x2a); /* set to 2 pixels */
		    outb(vgaIOBase + 4, 0x2c); /* this is the 754x register */
		    temp = (inb(vgaIOBase + 5) | 0x20) & 0xF7;
		    outb(vgaIOBase + 5, temp); /* enable shadow write */
	            }
	       else
	            outb(vgaIOBase + 4, 0x1c);
	       switch (inb(vgaIOBase + 5) & 0xc0) 
		    {
		  case 0xc0:
		    ErrorF("CIRRUS: TFT active color LCD detected\n");
		    break;
		  case 0x80:
		    ErrorF("CIRRUS: STF passive color LCD detected\n");
		    break;
		  case 0x40:
		    ErrorF("CIRRUS: Grayscale plasma display detected\n");
		    break;
		  default:
		    ErrorF("CIRRUS: Monochrome LCD detected\n");
		    ErrorF("CIRRUS: enabling option clgd6225_lcd\n");
		    OFLG_SET(OPTION_CLGD6225_LCD, &vga256InfoRec.options);
		    }
	       }

	  /* Lock the LCD registers... */
	  if(Is_754x(cirrusChip) )
	      outb(vgaIOBase + 4, 0x2d);
	  else
	      outb(vgaIOBase + 4, 0x1D);
	  temp = inb(vgaIOBase + 5);
	  outb(vgaIOBase + 5, (temp & 0x7f));
          }


     if (!vga256InfoRec.videoRam) 
	  {
	  if (Is_62x5(cirrusChip)) 
	       {
	       /* 
		* According to Ed Strauss at Cirrus, the 62x5 has 512k.
		* That's it.  Period.
		*/
	       vga256InfoRec.videoRam = 512;
	       }
	  else 
	  if (HAVE543X()) {
	  	/* The scratch register method does not work on the 543x. */
	  	/* Use the DRAM bandwidth bit and the DRAM bank switching */
	  	/* bit to figure out the amount of memory. */
	  	unsigned char SRF;
	  	vga256InfoRec.videoRam = 512;
	  	outb(0x3c4, 0x0f);
	  	SRF = inb(0x3c5);
	  	if (SRF & 0x10)
	  		/* 32-bit DRAM bus. */
	  		vga256InfoRec.videoRam *= 2;
	  	if ((SRF & 0x18) == 0x18)
	  		/* 64-bit DRAM data bus width; assume 2MB. */
	  		/* Also indicates 2MB memory on the 5430. */
	  		vga256InfoRec.videoRam *= 2;
	  	if (cirrusChip != CLGD5430 && (SRF & 0x80))
	  		/* If DRAM bank switching is enabled, there */
	  		/* must be twice as much memory installed. */
	  		/* (4MB on the 5434) */
	  		vga256InfoRec.videoRam *= 2;
	  }
	  else
	       {
	       unsigned char memreg;

				/* Thanks to Brad Hackenson at Cirrus for */
				/* this bit of undocumented black art....*/
	       outb(0x3C4,0x0A);
	       memreg = inb(0x3C5);
	  
	       switch( (memreg & 0x18) >> 3 )
		    {
		  case 0:
		    vga256InfoRec.videoRam = 256;
		    break;
		  case 1:
		    vga256InfoRec.videoRam = 512;
		    break;
		  case 2:
		    vga256InfoRec.videoRam = 1024;
		    break;
		  case 3:
		    vga256InfoRec.videoRam = 2048;
		    break;
		    }

	       if (cirrusChip >= CLGD5422 && cirrusChip <= CLGD5429 &&
	       vga256InfoRec.videoRam < 512) {
	       		/* Invalid amount for 542x -- scratch register may */
	       		/* not be set by some BIOSes. */
		  	unsigned char SRF;
		  	vga256InfoRec.videoRam = 512;
	  		outb(0x3c4, 0x0f);
		  	SRF = inb(0x3c5);
		  	if (SRF & 0x10)
		  		/* 32-bit DRAM bus. */
		  		vga256InfoRec.videoRam *= 2;
		  	if ((SRF & 0x18) == 0x18)
		  		/* 2MB memory on the 5426/8/9 (not sure). */
		  		vga256InfoRec.videoRam *= 2;
		  	}
	       }
	  }

#ifndef MONOVGA
     if (vgaBitsPerPixel == 32 &&
     ((cirrusChip != CLGD5434 && cirrusChip != CLGD5436) ||
     vga256InfoRec.videoRam < 2048)) {
         ErrorF("%s %s: %s: Only clgd5434 with 2048K supports 32bpp\n",
             XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset);
         CIRRUS.ChipHas32bpp = FALSE;
     }
#endif

     /* 
      * Banking granularity is 16k for the 5426, 5428 or 5429
      * when allowing access to 2MB, and 4k otherwise 
      */
     if (vga256InfoRec.videoRam > 1024)
          {
          CIRRUS.ChipSetRead = cirrusSetRead2MB;
          CIRRUS.ChipSetWrite = cirrusSetWrite2MB;
          CIRRUS.ChipSetReadWrite = cirrusSetReadWrite2MB;
	  cirrusBankShift = 8;
          }

#ifndef MONOVGA
     /*
      * Determine the MCLK that will be used (possibly reprogrammed).
      * Calculate the available DRAM bandwidth from the MCLK setting.
      */
     {
         unsigned char MCLK, SRF;
         outb(0x3c4, 0x0f);
         SRF = inb(0x3c5);
         if (cirrusChip == CLGD5434 && cirrusChipRevision >= 0x01)
             /* 5434 rev. E+ supports 60 MHz MCLK in packed-pixel mode. */
             cirrusReprogrammedMCLK = 0x22;
         if ((cirrusChip >= CLGD5424 && cirrusChip <= CLGD5429) ||
         HAVE543X()) {
             outb(0x3c4, 0x1f);
             MCLK = inb(0x3c5) & 0x3f;
             if (OFLG_ISSET(OPTION_SLOW_DRAM, &vga256InfoRec.options))
                 cirrusReprogrammedMCLK = 0x1c;
             if (OFLG_ISSET(OPTION_MED_DRAM, &vga256InfoRec.options))
                 cirrusReprogrammedMCLK = 0x1f;
             if (OFLG_ISSET(OPTION_FAST_DRAM, &vga256InfoRec.options))
                 cirrusReprogrammedMCLK = 0x22;
         }
         else
             /* 5420/22/62x5 have fixed MCLK settings. */
             switch (SRF & 0x03) {
             case 0 : MCLK = 0x1c; break;
             case 1 : MCLK = 0x19; break;
             case 2 : MCLK = 0x17; break;
             case 3 : MCLK = 0x15; break;
             }
         if (cirrusReprogrammedMCLK > 0)
             MCLK = cirrusReprogrammedMCLK;

         /* Approximate DRAM bandwidth in K/s (8-bit page mode accesses),
          * corresponds with MCLK frequency / 2 (2 cycles per access). */
         cirrusDRAMBandwidth = 14318 * MCLK / 16;
         if (vga256InfoRec.videoRam >= 512)
             /* At least 16-bit access. */
             cirrusDRAMBandwidth *= 2;
         if (cirrusChip != CLGD5420 &&
         (cirrusChip < CLGD6205 || cirrusChip > CLGD6235) &&
         vga256InfoRec.videoRam >= 1024)
             /* At least 32-bit access. */
             cirrusDRAMBandwidth *= 2;
         if ((cirrusChip == CLGD5434 || cirrusChip == CLGD5436)
         && vga256InfoRec.videoRam >= 2048)
             /* 64-bit access. */
             cirrusDRAMBandwidth *= 2;
         /*
          * Calculate highest acceptable DRAM bandwidth to be taken up
          * by screen refresh. Satisfies
          *	total bandwidth >= refresh bandwidth * 1.1
          */
         cirrusDRAMBandwidthLimit = (cirrusDRAMBandwidth * 10) / 11;
     }

     /*
      * Adjust the clock limits for inadequate amounts of memory
      * and for 16bpp/32bpp modes.
      * In cases where DRAM bandwidth is the limiting factor, we
      * require the total bandwidth to be at least 10% higher than the
      * bandwidth required for screen refresh (which is what the Cirrus
      * databook recommends for 'acceptable' performance).
      */

     if (vgaBitsPerPixel == 8) {
         if (cirrusChip >= CLGD5420 && cirrusChip <= CLGD5429 &&
         vga256InfoRec.videoRam <= 512)
             cirrusClockLimit[cirrusChip] = cirrusDRAMBandwidthLimit;
#ifdef ALLOW_8BPP_MULTIPLEXING
         if ((cirrusChip == CLGD5434 || cirrusChip == CLGD5436)
         && vga256InfoRec.videoRam <= 1024)
             /* DRAM bandwidth limited. This translates to allowing
              * 90 MHz with MCLK = 0x1c, 95 MHz with MCLK = 0x1f,
              * 100 MHz with 0x22. */
             cirrusClockLimit[cirrusChip] = cirrusDRAMBandwidthLimit;
#endif
     }

     if (vgaBitsPerPixel == 16) {
         memcpy(cirrusClockLimit, cirrusClockLimit16bpp,
             LASTCLGD * sizeof(int));
         if (cirrusChip >= CLGD5422 && cirrusChip <= CLGD5429
         && vga256InfoRec.videoRam <= 512)
                 cirrusClockLimit[cirrusChip] = 0;
         if ((cirrusChip >= CLGD5426 && cirrusChip <= CLGD5429)
         || cirrusChip == CLGD5430 || ((cirrusChip == CLGD5434
         || cirrusChip == CLGD5436) && vga256InfoRec.videoRam <= 1024))
                 /* Allow 45 MHz with MCLK = 0x1c, 50 MHz with 0x1f+. */
                 cirrusClockLimit[cirrusChip] = cirrusDRAMBandwidthLimit / 2;
     }

     if (vgaBitsPerPixel == 32) {
         memcpy(cirrusClockLimit, cirrusClockLimit32bpp,
             LASTCLGD * sizeof(int));
         if (vga256InfoRec.videoRam <= 1024)
             cirrusClockLimit[cirrusChip] = 0;
         else
             /* Allow 45 MHz with MCLK = 0x1c, 50 MHz with MCLK = 0x1f+. */
             cirrusClockLimit[cirrusChip] = cirrusDRAMBandwidthLimit / 4;
     }
#endif

     cirrusClockNo = cirrusNumClocks(cirrusChip);
     if (!vga256InfoRec.clocks)
          if (OFLG_ISSET(OPTION_PROBE_CLKS, &vga256InfoRec.options))
	       vgaGetClocks(cirrusClockNo, cirrusClockSelect);
	  else
	       {
	       if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions))
	           {
	           vga256InfoRec.clocks = cirrusClockNo;
	           for (i = 0; i < cirrusClockNo; i++)
		       vga256InfoRec.clock[i] =
		          CLOCKVAL(cirrusClockTab[i].numer, cirrusClockTab[i].denom);
		   }
	       }
     else
          if (vga256InfoRec.clocks > cirrusClockNo)
	       {
		 ErrorF("%s %s: %s: Too many Clocks specified in configuration file.\n",
			XCONFIG_PROBED, vga256InfoRec.name,
			vga256InfoRec.chipset);
		 ErrorF("\t\tAt most %d clocks may be specified\n",
			cirrusClockNo);
	       }

     vga256InfoRec.bankedMono = TRUE;
#ifdef ALLOW_OUT_OF_SPEC_CLOCKS
     vga256InfoRec.maxClock = MAX_OUT_OF_SPEC_CLOCK;
#else
     vga256InfoRec.maxClock = cirrusClockLimit[cirrusChip];
#endif
     /* Initialize option flags allowed for this driver */
#ifdef ALLOW_OUT_OF_SPEC_CLOCKS
     OFLG_SET(OPTION_16CLKS, &CIRRUS.ChipOptionFlags);
     ErrorF("CIRRUS: Warning: Out of spec clocks can be enabled\n");
#endif
     OFLG_SET(OPTION_NOACCEL, &CIRRUS.ChipOptionFlags);
     OFLG_SET(OPTION_PROBE_CLKS, &CIRRUS.ChipOptionFlags);
     OFLG_SET(OPTION_LINEAR, &CIRRUS.ChipOptionFlags);
     if ((cirrusChip >= CLGD5424 && cirrusChip <= CLGD5429) || HAVE543X()) {
         OFLG_SET(OPTION_SLOW_DRAM, &CIRRUS.ChipOptionFlags);
         OFLG_SET(OPTION_MED_DRAM, &CIRRUS.ChipOptionFlags);
         OFLG_SET(OPTION_FAST_DRAM, &CIRRUS.ChipOptionFlags);
         OFLG_SET(OPTION_FIFO_CONSERV, &CIRRUS.ChipOptionFlags);
         OFLG_SET(OPTION_FIFO_AGGRESSIVE, &CIRRUS.ChipOptionFlags);
#ifdef PC98
	 OFLG_SET(OPTION_EPSON_MEM_WIN,&CIRRUS.ChipOptionFlags);
	 OFLG_SET(OPTION_NEC_CIRRUS,&CIRRUS.ChipOptionFlags);
	 OFLG_SET(OPTION_GA98NB1,&CIRRUS.ChipOptionFlags);
	 OFLG_SET(OPTION_GA98NB2,&CIRRUS.ChipOptionFlags);
	 OFLG_SET(OPTION_GA98NB4,&CIRRUS.ChipOptionFlags);
	 OFLG_SET(OPTION_WAP,&CIRRUS.ChipOptionFlags);
#endif
     }
     if ((cirrusChip >= CLGD5426 && cirrusChip <= CLGD5429) || HAVE543X()) {
         OFLG_SET(OPTION_NO_2MB_BANKSEL, &CIRRUS.ChipOptionFlags);
         OFLG_SET(OPTION_NO_BITBLT, &CIRRUS.ChipOptionFlags);
         OFLG_SET(OPTION_FAVOUR_BITBLT, &CIRRUS.ChipOptionFlags);
         OFLG_SET(OPTION_NO_IMAGEBLT, &CIRRUS.ChipOptionFlags);
     }
#ifdef CIRRUS_SUPPORT_MMIO
     if (cirrusChip == CLGD5429 || HAVE543X())
         OFLG_SET(OPTION_MMIO, &CIRRUS.ChipOptionFlags);
#endif

     /* <scooper>
      *	The Hardware cursor, if the chip is capable, can be turned off using
      * the "sw_cursor" option.
      */

     if (Has_HWCursor(cirrusChip)) {
        OFLG_SET(OPTION_SW_CURSOR, &CIRRUS.ChipOptionFlags);
     }

     return(TRUE);
}

#ifndef MONOVGA

extern GCOps cfb16TEOps1Rect, cfb16TEOps, cfb16NonTEOps1Rect, cfb16NonTEOps;
extern GCOps cfb32TEOps1Rect, cfb32TEOps, cfb32NonTEOps1Rect, cfb32NonTEOps;

#endif

/*
 * cirrusFbInit --
 *      enable speedups for the chips that support it
 */
static void
cirrusFbInit()
{
  int size;

#ifndef MONOVGA
  int useSpeedUp;

  useSpeedUp = vga256InfoRec.speedup & SPEEDUP_ANYWIDTH;
  
  cirrusBusType = CIRRUS_BUS_FAST;
  if (cirrusChip >= CLGD5422) {
  	/* It is possible to read the configuration register */
  	/* to find out the bus interface. This is only implemented on */
  	/* later 542x cards and on the 543x. The only function that */
  	/* uses this is solid filling on the 5426, for which framebuffer */
  	/* color expansion is much faster than the BitBLT engine on a */
  	/* local bus. */
  	outb(0x3c4, 0x17);
  	switch ((inb(0x3c5) >> 3) & 7) {
  	case 2 :	/* VLB > 33 MHz */
  	case 6 :	/* VLB at 33 MHz or less */
  		cirrusBusType = CIRRUS_BUS_VLB;
  		break;
  	case 4 :	/* PCI */
  		cirrusBusType = CIRRUS_BUS_PCI;
  		break;
  	case 7 :	/* ISA */
  		cirrusBusType = CIRRUS_BUS_ISA;
  		break;
  	/* In other cases (e.g. undefined), assume 'fast' bus. */
  	}
  }

  cirrusUseBLTEngine = FALSE;
  if (cirrusChip == CLGD5426 || cirrusChip == CLGD5428 ||
  cirrusChip == CLGD5429 || HAVE543X())
      {
      cirrusUseBLTEngine = TRUE;
      if (OFLG_ISSET(OPTION_NO_BITBLT, &vga256InfoRec.options))
          cirrusUseBLTEngine = FALSE;
      else {
          if (cirrusChip == CLGD5429 && cirrusChipRevision == 0) {
              ErrorF("%s %s: %s: CL-GD5429 Rev A detected\n",
                XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.chipset);
              cirrusAvoidImageBLT = TRUE;
              }
          }
          if (OFLG_ISSET(OPTION_NO_IMAGEBLT, &vga256InfoRec.options))
              cirrusAvoidImageBLT = TRUE;
          if (xf86Verbose && cirrusAvoidImageBLT)
              ErrorF("%s %s: %s: Not using system-to-video BitBLT\n",
                XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.chipset);
     }

#endif

    if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions))
        if (xf86Verbose)
            ErrorF("%s %s: %s: Using programmable clocks\n",
	        XCONFIG_PROBED, vga256InfoRec.name,
	        vga256InfoRec.chipset);

  /*
   * Report the internal MCLK value of the card, and change it if the
   * "fast_dram" or "slow_dram" option is defined.
   */
  if (cirrusChip == CLGD5424 || cirrusChip == CLGD5426 ||
      cirrusChip == CLGD5428 || cirrusChip == CLGD5429 ||
      HAVE543X())
      {
      unsigned char SRF, SR1F;
      outb(0x3c4, 0x0f);
      SRF = inb(0x3c5);
      outb(0x3c4, 0x1f);
      SR1F = inb(0x3c5);
      if (xf86Verbose)
          ErrorF(
              "%s %s: %s: Internal memory clock register is 0x%02x (%s RAS)\n",
              XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset,
              SR1F & 0x3f, (SRF & 4) ? "Standard" : "Extended");
      
      if (cirrusReprogrammedMCLK > 0)
      	  /*
      	   * The MCLK will be programmed to a different value.
      	   * 
      	   * 0x1c, 51 MHz	Option "slow_dram"
      	   * 0x1f, 55 MHz	Option "med_dram"
      	   * 0x22, 61 MHz	Option "fast_dram"
      	   * 0x25, 66 MHz
      	   * 
	   * The official spec for the 542x is 50 MHz, but some cards are
	   * overclocked.
      	   *
      	   * The 5434 is specified for 50 MHz, but new revisions can do
      	   * 60 MHz in packed-pixel mode. The 5429 and 5430 are probably
      	   * speced for 60 MHz.
      	   */
	  if (xf86Verbose) {
	      if (cirrusChip == CLGD5434 && cirrusChipRevision >= 0x01) {
                  ErrorF("%s %s: %s: CL-GD5434 rev. E+, will program 0x22 MCLK\n",
                      XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset);
	          if (OFLG_ISSET(OPTION_FAST_DRAM, &vga256InfoRec.options) ||
	          OFLG_ISSET(OPTION_MED_DRAM, &vga256InfoRec.options) ||
	          OFLG_ISSET(OPTION_SLOW_DRAM, &vga256InfoRec.options))
                  ErrorF("%s %s: %s: Memory clock overridden by option\n",
                      XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.chipset);
	      }
              ErrorF("%s %s: %s: Internal memory clock register set to 0x%02x\n",
                XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.chipset,
                cirrusReprogrammedMCLK);
	  }
      }

#ifndef MONOVGA

    if (xf86Verbose)
        ErrorF("%s %s: %s: Approximate DRAM bandwidth for drawing: %d of %d MB/s\n",
            XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.chipset,
            (cirrusDRAMBandwidth - vga256InfoRec.clock[
            vga256InfoRec.modes->Clock] * vgaBitsPerPixel / 8) / 1000,
            cirrusDRAMBandwidth / 1000);

#ifdef CIRRUS_SUPPORT_LINEAR
    if (xf86LinearVidMem() &&
    OFLG_ISSET(OPTION_LINEAR, &vga256InfoRec.options)) {
        cirrusUseLinear = TRUE;
        if (vga256InfoRec.MemBase != 0)
            CIRRUS.ChipLinearBase = vga256InfoRec.MemBase;
        else
            if (HAVE543X()) {
                if (cirrusBusType == CIRRUS_BUS_ISA) {
		    ErrorF("%s %s: %s: Must specify MemBase for ISA bus linear "
		           "addressing\n", XCONFIG_PROBED,
		           vga256InfoRec.name, vga256InfoRec.chipset);
		    cirrusUseLinear = FALSE;
                    goto nolinear;
		}
                if (cirrusBusType == CIRRUS_BUS_VLB)
                    /* 64MB is standard. */
        	    /* A documented jumper option is 2048MB. */
        	    /* 32MB is an option for more recent cards. */
        	    CIRRUS.ChipLinearBase = 0x4000000;	/* 64MB */
                if (cirrusBusType == CIRRUS_BUS_PCI) {
		    cirrusUseLinear = FALSE;
		    if (vgaPCIInfo && vgaPCIInfo->Vendor == PCI_VENDOR_CIRRUS) {
                    	/*
                    	 * Known devices:
                    	 * 0x00A0	5430, 5440
                    	 * 0x00A8	5434
                    	 * 0x00AC	5436
                    	 * Assume 0x00A? are graphics chipsets.
      			 */
                    	if (vgaPCIInfo->MemBase != 0) {
                    		CIRRUS.ChipLinearBase =
                    		    vgaPCIInfo->MemBase & 0xFF000000;
                    		cirrusUseLinear = TRUE;
                    	}
			else
			    ErrorF("%s %s: %s: Can't find valid PCI "
			        "Base Address\n", XCONFIG_PROBED,
			        vga256InfoRec.name, vga256InfoRec.chipset);
                    } else
		        ErrorF("%s %s: %s: Can't find PCI device in "
		               "configuration space\n", XCONFIG_PROBED,
		               vga256InfoRec.name, vga256InfoRec.chipset);
		    if (!cirrusUseLinear)
			goto nolinear;
		}
	    }
            else {
                /* Some recent 542x-based cards should map at 64MB, others */
                /* can only map at 14MB. */
                if (cirrusChip == CLGD5429)
        	    CIRRUS.ChipLinearBase = 0x03E00000;		/* 62MB */
        	else {
	            ErrorF("%s %s: %s: Must specify MemBase for 542x linear "
		           "addressing\n", XCONFIG_PROBED,
		           vga256InfoRec.name, vga256InfoRec.chipset);
		    cirrusUseLinear = FALSE;
                    goto nolinear;
                }
            }
        CIRRUS.ChipLinearSize = vga256InfoRec.videoRam * 1024;
        if (xf86Verbose)
            ErrorF("%s %s: %s: Using linear framebuffer at 0x%08x (%dMB)\n",
	        OFLG_ISSET(XCONFIG_MEMBASE, &vga256InfoRec.xconfigFlag) ?
		XCONFIG_GIVEN : XCONFIG_PROBED,
		vga256InfoRec.name, vga256InfoRec.chipset,
	        CIRRUS.ChipLinearBase, (unsigned int)CIRRUS.ChipLinearBase
	        / (1024 * 1024));
	if (cirrusChip >= CLGD5422 && cirrusChip <= CLGD5428 &&
	OFLG_ISSET(OPTION_FAST_DRAM, &vga256InfoRec.options))
            ErrorF("%s %s: %s: Warning: fast_dram option not recommended "
                "with linear addressing\n",
                XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.chipset);
    }
nolinear:
    if (cirrusUseLinear)
        CIRRUS.ChipUseLinearAddressing = TRUE;
#endif

  CirrusMemTop = vga256InfoRec.virtualX * vga256InfoRec.virtualY
      * (vgaBitsPerPixel / 8);
  size = CirrusInitializeAllocator(CirrusMemTop);

  if (xf86Verbose)
    ErrorF("%s %s: %s: %d bytes off-screen memory available\n",
	   XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset, size);
    
  if (Has_HWCursor(cirrusChip) &&
      !OFLG_ISSET(OPTION_SW_CURSOR, &vga256InfoRec.options))
    {
#if 0
      if (HasLargeHWCursor(cirrusChip))
	{
	  cirrusCur.cur_size = 1;
	  cirrusCur.width = 64;
	  cirrusCur.height = 64;
	}
      else
#endif
	{
	  cirrusCur.cur_size = 0;
	  cirrusCur.width = 32;
	  cirrusCur.height = 32;
	}

      if (!CirrusCursorAllocate(&cirrusCur))
	{	
	  vgaHWCursor.Initialized = TRUE;
	  vgaHWCursor.Init = cirrusCursorInit;
	  vgaHWCursor.Restore = cirrusRestoreCursor;
	  vgaHWCursor.Warp = cirrusWarpCursor;  
	  vgaHWCursor.QueryBestSize = cirrusQueryBestSize;

	  if (xf86Verbose)
	    {
	      ErrorF( "%s %s: %s: Using hardware cursor\n",
		     XCONFIG_PROBED, vga256InfoRec.name,
		     vga256InfoRec.chipset);
	    }
	}
      else
	{
	  ErrorF( "%s %s: %s: Failed to allocate hardware cursor in offscreen ram,\n\ttry reducing the virtual screen size\n",
		 XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset);
	}
    }	  

  if (!OFLG_ISSET(OPTION_NOACCEL, &vga256InfoRec.options)
  && !(cirrusChip == CLGD5420 && cirrusChipRevision == 1)) {
    if (xf86Verbose)
      {
        ErrorF ("%s %s: %s: Using accelerator functions\n",
	    XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset);
      }

    /* Accel functions are available on all chips except 5420-75QC-B; some
     * use the BitBLT engine if available. */

    if (vgaBitsPerPixel == 8) {
        vga256LowlevFuncs.doBitbltCopy = CirrusDoBitbltCopy;
        vga256LowlevFuncs.fillRectSolidCopy = CirrusFillRectSolidCopy;
        vga256LowlevFuncs.fillBoxSolid = CirrusFillBoxSolid;

        /* Hook special op. fills (and tiles): */
        vga256TEOps1Rect.PolyFillRect = CirrusPolyFillRect;
        vga256NonTEOps1Rect.PolyFillRect = CirrusPolyFillRect;
        vga256TEOps.PolyFillRect = CirrusPolyFillRect;
        vga256NonTEOps.PolyFillRect = CirrusPolyFillRect;

        vga256TEOps1Rect.PolyGlyphBlt = CirrusPolyGlyphBlt;
        vga256TEOps.PolyGlyphBlt = CirrusPolyGlyphBlt;
	/*
	 * If using the BitBLT engine but avoiding image blit,
	 * prefer the framebuffer routines for ImageGlyphBlt.
	 * The color-expand text functions would be allowable, but
	 * the use of "no_imageblt" generally implies a fast host bus.
	 */
	if (!cirrusAvoidImageBLT) {
	    vga256LowlevFuncs.teGlyphBlt8 = CirrusImageGlyphBlt;
            vga256TEOps1Rect.ImageGlyphBlt = CirrusImageGlyphBlt;
            vga256TEOps.ImageGlyphBlt = CirrusImageGlyphBlt;
       }
    }

    CirrusInvalidateShadowVariables();

    if (HAVEBITBLTENGINE()) {
    	/* Need 256 bytes for BitBLT fills. */
        cirrusBLTPatternAddress = CirrusAllocate(256);
        if (cirrusBLTPatternAddress == -1) {
            cirrusUseBLTEngine = FALSE;
            ErrorF("%s %s: %s: Too little space: cannot use BitBLT engine\n",
	        XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset);

	}
	else {
	    if (xf86Verbose) {
              ErrorF("%s %s: %s: Using BitBLT engine\n",
	             XCONFIG_PROBED, vga256InfoRec.name, vga256InfoRec.chipset);
              }

#ifdef CIRRUS_INCLUDE_COPYPLANE1TO8
	    if (vgaBitsPerPixel == 8 && !cirrusAvoidImageBLT)
	        vga256LowlevFuncs.copyPlane1to8 = CirrusCopyPlane1to8;
#endif	
	    if (vgaBitsPerPixel == 16) {
	        if (!cirrusAvoidImageBLT) {
		    cfb16TEOps1Rect.ImageGlyphBlt = CirrusImageGlyphBlt;
	            cfb16TEOps.ImageGlyphBlt = CirrusImageGlyphBlt;
	        }
        	cfb16TEOps1Rect.CopyArea = Cirrus16CopyArea;
        	cfb16NonTEOps1Rect.CopyArea = Cirrus16CopyArea;
        	cfb16TEOps.CopyArea = Cirrus16CopyArea;
        	cfb16NonTEOps.CopyArea = Cirrus16CopyArea;
/*	        xf86Info.currentScreenCopyWindow = CirrusCopyWindow; */
	    }
	    if (vgaBitsPerPixel == 32) {
	        if (!cirrusAvoidImageBLT) {
		    cfb32TEOps1Rect.ImageGlyphBlt = CirrusImageGlyphBlt;
	            cfb32TEOps.ImageGlyphBlt = CirrusImageGlyphBlt;
	        }
        	cfb32TEOps1Rect.CopyArea = Cirrus32CopyArea;
        	cfb32NonTEOps1Rect.CopyArea = Cirrus32CopyArea;
        	cfb32TEOps.CopyArea = Cirrus32CopyArea;
        	cfb32NonTEOps.CopyArea = Cirrus32CopyArea;
/*	        xf86Info.currentScreen->CopyWindow = CirrusCopyWindow; */
	    }
            if (OFLG_ISSET(OPTION_FAVOUR_BITBLT, &vga256InfoRec.options))
                /* Use BitBLT engine in more cases. */
                cirrusFavourBLT = TRUE;
	}
    }
    if ((vga256InfoRec.virtualX & 31) != 0)
        ErrorF("%s %s: %s: Warning: virtual screen width not multiple of 32\n",
            XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.chipset);

#ifdef CIRRUS_SUPPORT_MMIO
    /* Optional Memory-Mapped I/O. */
    /* Register is set in init function. */
    if ((HAVE543X() || cirrusChip == CLGD5429)
    && OFLG_ISSET(OPTION_MMIO, &vga256InfoRec.options)) {
        cirrusUseMMIO = TRUE;
        /* We can't set cirrusMMIOBase, since vgaBase hasn't been */
        /* mapped yet. For now we do that in the init function. */
        ErrorF("%s %s: %s: Using memory-mapped I/O\n",
            XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.chipset);
        if (cirrusUseBLTEngine) {
            if (vgaBitsPerPixel == 8) {
                vga256TEOps1Rect.PolyGlyphBlt = CirrusMMIOPolyGlyphBlt;
                vga256TEOps.PolyGlyphBlt = CirrusMMIOPolyGlyphBlt;
		if (!cirrusAvoidImageBLT) {
                    vga256LowlevFuncs.teGlyphBlt8 = CirrusMMIOImageGlyphBlt;
                    vga256TEOps1Rect.ImageGlyphBlt = CirrusMMIOImageGlyphBlt;
                    vga256TEOps.ImageGlyphBlt = CirrusMMIOImageGlyphBlt;
                }
		/* These functions need to be initialized in the GC handling */
		/* of vga256. */
	        vga256TEOps1Rect.FillSpans = CirrusFillSolidSpansGeneral;
	        vga256TEOps.FillSpans = CirrusFillSolidSpansGeneral;
	        vga256LowlevFuncs.fillSolidSpans = CirrusFillSolidSpansGeneral;

		vga256TEOps1Rect.Polylines = CirrusMMIOLineSS;
		vga256NonTEOps1Rect.Polylines = CirrusMMIOLineSS;
		vga256TEOps.Polylines = CirrusMMIOLineSS;
		vga256NonTEOps.Polylines = CirrusMMIOLineSS;
		vga256TEOps1Rect.PolySegment = CirrusMMIOSegmentSS;
		vga256NonTEOps1Rect.PolySegment = CirrusMMIOSegmentSS;
		vga256TEOps.PolySegment = CirrusMMIOSegmentSS;
		vga256TEOps.PolySegment = CirrusMMIOSegmentSS;

		vga256TEOps1Rect.PolyRectangle = Cirrus8PolyRectangle;
		vga256NonTEOps1Rect.PolyRectangle = Cirrus8PolyRectangle;
		vga256TEOps.PolyRectangle = Cirrus8PolyRectangle;
		vga256NonTEOps.PolyRectangle = Cirrus8PolyRectangle;
#if 0
	        vga256LowlevFuncs.fillRectSolidCopy = CirrusMMIOFillRectSolid;
	        vga256LowlevFuncs.fillBoxSolid = CirrusMMIOFillBoxSolid;
#endif
             }
            else
            if (vgaBitsPerPixel == 16) {
                if (!cirrusAvoidImageBLT) {
		    cfb16TEOps1Rect.ImageGlyphBlt = CirrusMMIOImageGlyphBlt;
	            cfb16TEOps.ImageGlyphBlt = CirrusMMIOImageGlyphBlt;
	        }
	        cfb16TEOps1Rect.FillSpans = CirrusFillSolidSpansGeneral;
	        cfb16TEOps.FillSpans = CirrusFillSolidSpansGeneral;
	        cfb16NonTEOps1Rect.FillSpans = CirrusFillSolidSpansGeneral;
	        cfb16NonTEOps.FillSpans = CirrusFillSolidSpansGeneral;
	        cfb16TEOps1Rect.PolyFillRect = CirrusPolyFillRect;
	        cfb16TEOps.PolyFillRect = CirrusPolyFillRect;
	        cfb16NonTEOps1Rect.PolyFillRect = CirrusPolyFillRect;
	        cfb16NonTEOps.PolyFillRect = CirrusPolyFillRect;
#if 0
	        cfb16TEOps1Rect.PolyRectangle = Cirrus16PolyRectangle;
	        cfb16TEOps.PolyRectangle = Cirrus16PolyRectangle;
	        cfb16NonTEOps1Rect.PolyRectangle = Cirrus16PolyRectangle;
	        cfb16NonTEOps.PolyRectangle = Cirrus16PolyRectangle;
#endif
            }
            else { /* vgaBitsPerPixel == 32 */
                if (!cirrusAvoidImageBLT) {
		    cfb32TEOps1Rect.ImageGlyphBlt = CirrusMMIOImageGlyphBlt;
	            cfb32TEOps.ImageGlyphBlt = CirrusMMIOImageGlyphBlt;
	        }
	        cfb32TEOps1Rect.FillSpans = CirrusFillSolidSpansGeneral;
	        cfb32TEOps.FillSpans = CirrusFillSolidSpansGeneral;
	        cfb32NonTEOps1Rect.FillSpans = CirrusFillSolidSpansGeneral;
	        cfb32NonTEOps.FillSpans = CirrusFillSolidSpansGeneral;
	        cfb32TEOps1Rect.PolyFillRect = CirrusPolyFillRect;
	        cfb32TEOps.PolyFillRect = CirrusPolyFillRect;
	        cfb32NonTEOps1Rect.PolyFillRect = CirrusPolyFillRect;
	        cfb32NonTEOps.PolyFillRect = CirrusPolyFillRect;
#if 0
	        cfb32TEOps1Rect.PolyRectangle = Cirrus32PolyRectangle;
	        cfb32TEOps.PolyRectangle = Cirrus32PolyRectangle;
	        cfb32NonTEOps1Rect.PolyRectangle = Cirrus32PolyRectangle;
	        cfb32NonTEOps.PolyRectangle = Cirrus32PolyRectangle;
#endif
            }
        }
    }
#endif

  }

#endif	/* not MONOVGA */
}

/*
 * cirrusEnterLeave -- 
 *      enable/disable io-mapping
 */
static void 
cirrusEnterLeave(enter)
     Bool enter;
{
  static unsigned char temp;

#ifndef MONOVGA
#ifdef XFreeXDGA
  if (vga256InfoRec.directMode&XF86DGADirectGraphics && !enter) {
      cirrusHideCursor();
      return;
  }
#endif
#endif
  if (enter)
       {

       xf86EnableIOPorts(vga256InfoRec.scrnIndex);

#ifdef PC98
       crtswitch(1);
#else
				/* Are we Mono or Color? */
       vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

       outb(0x3C4,0x06);
       outb(0x3C5,0x12);	 /* unlock cirrus special */
#endif

				/* Put the Vert. Retrace End Reg in temp */

       outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);

				/* Put it back with PR bit set to 0 */
				/* This unprotects the 0-7 CRTC regs so */
				/* they can be modified, i.e. we can set */
				/* the timing. */

       outb(vgaIOBase + 5, temp & 0x7F);

    }
  else
       {

       outb(0x3C4,0x06);
       outb(0x3C5,0x0F);	 /*relock cirrus special */

#ifdef PC98
       crtswitch(0);
#endif

       xf86DisableIOPorts(vga256InfoRec.scrnIndex);
    }
}

/*
 * cirrusRestore -- 
 *      restore a video mode
 */
static void 
cirrusRestore(restore)
  vgacirrusPtr restore;
{
  unsigned char i;


  outw(0x3CE, 0x0009);	/* select bank 0 */
  outw(0x3CE, 0x000A);

  outb(0x3C4,0x0F);		/* Restoring this registers avoids */
  outb(0x3C5,restore->SRF);	/* textmode corruption on 2Mb cards. */

  outb(0x3C4,0x07);		/* This will disable linear addressing */
  outb(0x3C5,restore->SR7);	/* if enabled. */

#ifndef MONOVGA
#ifdef ALLOW_8BPP_MULTIPLEXING
  if ((cirrusChip == CLGD5434 || cirrusChip == CLGD5436)
  || vgaBitsPerPixel != 8) {
      outb(0x3c6, 0x00);
      outb(0x3c6, 0xff);
      inb(0x3c6); inb(0x3c6); inb(0x3c6); inb(0x3c6);
      outb(0x3c6, restore->HIDDENDAC);
  }
#else
  if (vgaBitsPerPixel != 8) {
      /*
       * Write to DAC. This is very delicate, and the it can lock up
       * the bus if not done carefully. The access count for the DAC
       * register can be such that the first write accesses either the
       * VGA LUT pixel mask register or the Hidden DAC register.
       */
      outb(0x3c6, 0x00);	/* Reset access count. */
      outb(0x3c6, 0xff);	/* Write 0xff to pixel mask. */
      inb(0x3c6); inb(0x3c6); inb(0x3c6); inb(0x3c6);
      outb(0x3c6, restore->HIDDENDAC);
  }
#endif
#endif

  /*
   * Restore a few standard VGA register to help textmode font restoration.
   */
  outb(0x3ce, 0x08);
  outb(0x3cf, restore->std.Graphics[0x08]);
  outb(0x3ce, 0x00);
  outb(0x3cf, restore->std.Graphics[0x00]);
  outb(0x3ce, 0x01);
  outb(0x3cf, restore->std.Graphics[0x01]);
  outb(0x3ce, 0x02);
  outb(0x3cf, restore->std.Graphics[0x02]);
  outb(0x3ce, 0x05);
  outb(0x3cf, restore->std.Graphics[0x05]);

  vgaHWRestore((vgaHWPtr)restore);

/*  unsigned char GR9;		 Graphics Offset1 */
/*  unsigned char GRA;		 Graphics Offset2 */
/*  unsigned char GRB;		 Graphics Extensions Control */
/*  unsigned char SR7;		 Extended Sequencer */
/*  unsigned char SRE;		 VCLK Numerator */
/*  unsigned char SRF;		 DRAM Control */
/*  unsigned char SR10;		 Graphics Cursor X Position [7:0]         */
/*  unsigned char SR10E;	 Graphics Cursor X Position [7:5] | 10000 */
/*  unsigned char SR11;		 Graphics Cursor Y Position [7:0]         */
/*  unsigned char SR11E;	 Graphics Cursor Y Position [7:5] | 10001 */
/*  unsigned char SR12;		 Graphics Cursor Attributes Register */
/*  unsigned char SR13;		 Graphics Cursor Pattern Address */
/*  unsigned char SR16;		 Performance Tuning Register */
/*  unsigned char SR17;		 Configuration/Extended Control */
/*  unsigned char SR1E;		 VCLK Denominator */
/*  unsigned char CR19;		 Interlace End */
/*  unsigned char CR1A;		 Miscellaneous Control */
/*  unsigned char CR1B;		 Extended Display Control */
/*  unsigned char CR1D;		 Overlay Extended Control Register */
/*  unsigned char HIDDENDAC;	 Hidden DAC register */
/*  DACcolourRec  FOREGROUND;    Hidden DAC cursor foreground colour */
/*  DACcolourRec  BACKGROUND;    Hidden DAC cursor background colour */
  
  outw(0x3C4, 0x0100);				/* disable timing sequencer */

  outb(0x3CE,0x09);
  outb(0x3CF,restore->GR9);

  outb(0x3CE,0x0A);
  outb(0x3CF,restore->GRA);

  outb(0x3CE,0x0B);
  outb(0x3CF,restore->GRB);

  if (HAVE543X()) {
       outb(0x3ce, 0x0f);
       outb(0x3cf, restore->GRF);
  }

  if (restore->std.NoClock >= 0)
       {
       outb(0x3C4,0x0E);
       outb(0x3C5,restore->SRE);
       }

  if (Has_HWCursor(cirrusChip))
    {
      /* Restore the hardware cursor */
      outb (0x3C4, 0x13);
      outb (0x3C5, restore->SR13);
      
      outb (0x3C4, restore->SR10E);
      outb (0x3C5, restore->SR10);
      
      outb (0x3C4, restore->SR11E);
      outb (0x3C5, restore->SR11);

      outb (0x3C4, 0x12);
      outb (0x3C5, restore->SR12);
    }

  if ((cirrusChip >= CLGD5424 && cirrusChip <= CLGD5429) || HAVE543X())
       {
       /* Restore the Performance Tuning Register on these chips only. */
       outb(0x3C4,0x16);
       outb(0x3C5,restore->SR16);
       }

  if (HAVE543X()) {
       outb(0x3c4, 0x17);
       outb(0x3c5, restore->SR17);
  }

  if (restore->std.NoClock >= 0)
       {
       outb(0x3C4,0x1E);
       outb(0x3C5,restore->SR1E);
       }

  if ((cirrusChip >= CLGD5424 && cirrusChip <= CLGD5429) || HAVE543X()) {
      outb(0x3c4, 0x1f);	/* MCLK register */
      outb(0x3c5, restore->SR1F);
  }

  outb(vgaIOBase + 4,0x19);
  outb(vgaIOBase + 5,restore->CR19);

  outb(vgaIOBase + 4,0x1A);
  outb(vgaIOBase + 5,restore->CR1A);

  outb(vgaIOBase + 4, 0x1B);
  outb(vgaIOBase + 5,restore->CR1B);

  if (cirrusChip == CLGD5434 || cirrusChip == CLGD5436) {
      outb(vgaIOBase + 4, 0x1D);
      outb(vgaIOBase + 5, restore->CR1D);
  }

  if (Is_754x(cirrusChip)) {
      /* Does something need to be unlocked here?? */
      outb(vgaIOBase + 4, 0x2d);
      i = inb(vgaIOBase + 5);
      outb(vgaIOBase + 5, (i & ~0x01) | (restore->CR2D & 0x01));
  }

  if (cirrusChip == CLGD6225) 
       {
       /* Unlock the LCD registers... */
       outb(vgaIOBase + 4, 0x1D);
       i = inb(vgaIOBase + 5);
       outb(vgaIOBase + 5, (i | 0x80));

       /* Restore LCD HSYNC value */
       outb(vgaIOBase + 4, 0x0A);
       outb(vgaIOBase + 5, restore->RAX);
#if 0
       fprintf(stderr, "RAX restored to %d\n", restore->RAX);
#endif

       /* Lock the LCD registers... */
       outb(vgaIOBase + 4, 0x1D);
       i = inb(vgaIOBase + 5);
       outb(vgaIOBase + 5, (i & 0x7f));
       }

#ifndef MONOVGA

  CirrusInvalidateShadowVariables();

#endif
}

/*
 * cirrusSave -- 
 *      save the current video mode
 */
static void *
cirrusSave(save)
     vgacirrusPtr save;
{
  unsigned char             temp1, temp2;

  
#if defined(PC98_WAB)||defined(PC98_GANB_WAP)
  vgaIOBase = 0x3D0;
#else
  vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
#endif

  outb(0x3CE, 0x09);
  temp1 = inb(0x3CF);
  outb(0x3CF, 0x00);	/* select bank 0 */
  outb(0x3CE, 0x0A);
  temp2 = inb(0x3CF);
  outb(0x3CF, 0x00);	/* select bank 0 */

  save = (vgacirrusPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgacirrusRec));


/*  unsigned char GR9;		 Graphics Offset1 */
/*  unsigned char GRA;		 Graphics Offset2 */
/*  unsigned char GRB;		 Graphics Extensions Control */
/*  unsigned char SR7;		 Extended Sequencer */
/*  unsigned char SRE;		 VCLK Numerator */
/*  unsigned char SRF;		 DRAM Control */
/*  unsigned char SR10;		 Graphics Cursor X Position [7:0]         */
/*  unsigned char SR10E;	 Graphics Cursor X Position [7:5] | 10000 */
/*  unsigned char SR11;		 Graphics Cursor Y Position [7:0]         */
/*  unsigned char SR11E;	 Graphics Cursor Y Position [7:5] | 10001 */
/*  unsigned char SR12;		 Graphics Cursor Attributes Register */
/*  unsigned char SR13;		 Graphics Cursor Pattern Address */
/*  unsigned char SR1E;		 VCLK Denominator */
/*  unsigned char CR19;		 Interlace End */
/*  unsigned char CR1A;		 Miscellaneous Control */
/*  unsigned char CR1B;		 Extended Display Control */
/*  unsigned char CR1D;		 Overlay Extended Control Register */
/*  unsigned char HIDDENDAC;	 Hidden DAC register */
/*  DACcolourRec  FOREGROUND;    Hidden DAC cursor foreground colour */
/*  DACcolourRec  BACKGROUND;    Hidden DAC cursor background colour */

  save->GR9 = temp1;

  save->GRA = temp2;

  outb(0x3CE,0x0B);		
  save->GRB = inb(0x3CF); 

  if (HAVE543X()) {
      outb(0x3ce, 0x0f);
      save->GRF = inb(0x3cf);
  }

  outb(0x3C4,0x07);
  save->SR7 = inb(0x3C5);

  outb(0x3C4,0x0E);
  save->SRE = inb(0x3C5);

  outb(0x3C4,0x0F);
  save->SRF = inb(0x3C5);

  if (Has_HWCursor(cirrusChip))
    {
      /* Hardware cursor */
      outb (0x3C4, 0x10);
      save->SR10E = inb (0x3C4);
      save->SR10  = inb (0x3C5);

      outb (0x3C4, 0x11);
      save->SR11E = inb (0x3C4);
      save->SR11  = inb (0x3C5);
  
      outb (0x3C4, 0x12);
      save->SR12 = inb (0x3C5);
  
      outb (0x3C4, 0x13);
      save->SR13 = inb (0x3C5);
    }  

  if ((cirrusChip >= CLGD5424 && cirrusChip <= CLGD5429) || HAVE543X()) 
       {
       /* Save the Performance Tuning Register on these chips only. */
        outb(0x3C4,0x16);
        save->SR16 = inb(0x3C5);
       }

  if (HAVE543X())
       {
       outb(0x3c4,0x17);
       save->SR17 = inb(0x3c5);
       }

  outb(0x3C4,0x1E);
  save->SR1E = inb(0x3C5);

  if ((cirrusChip >= CLGD5424 && cirrusChip <= CLGD5429) || HAVE543X()) {
      outb(0x3c4, 0x1f);		/* Save the MCLK register. */
      save->SR1F = inb(0x3c5);
  }

  outb(vgaIOBase + 4,0x19);
  save->CR19 = inb(vgaIOBase + 5);

  outb(vgaIOBase + 4,0x1A);
  save->CR1A = inb(vgaIOBase + 5);

  outb(vgaIOBase + 4, 0x1B);
  save->CR1B = inb(vgaIOBase + 5);

  if (cirrusChip == CLGD5434 || cirrusChip == CLGD5436) {
      outb(vgaIOBase + 4, 0x1D);
      save->CR1D = inb(vgaIOBase + 5);
  }

  if (Is_754x(cirrusChip)) {
      outb(vgaIOBase + 4, 0x2D);
      save->CR2D = inb(vgaIOBase + 5);
  }

#ifndef MONOVGA
#ifdef ALLOW_8BPP_MULTIPLEXING
  if (cirrusChip == CLGD5434 || cirrusChip == CLGD5436
  || vgaBitsPerPixel != 8) {
      outb(0x3c6, 0x00);
      outb(0x3c6, 0xff);
      inb(0x3c6); inb(0x3c6); inb(0x3c6); inb(0x3c6);
      save->HIDDENDAC = inb(0x3c6);
  }
#else
  if (vgaBitsPerPixel != 8) {
      outb(0x3c6, 0x00);	/* Reset access count. */
      outb(0x3c6, 0xff);	/* Write 0xff to pixel mask. */
      inb(0x3c6); inb(0x3c6); inb(0x3c6); inb(0x3c6);
      save->HIDDENDAC = inb(0x3c6);
  }
#endif
#endif

  if (cirrusChip == CLGD6225) 
       {
       /* Unlock the LCD registers... */
       outb(vgaIOBase + 4, 0x1D);
       temp1 = inb(vgaIOBase + 5);
       outb(vgaIOBase + 5, (temp1 | 0x80));

       /* Save current LCD HSYNC value */
       outb(vgaIOBase + 4, 0x0A);
       save->RAX = inb(vgaIOBase + 5);
#if 0
       fprintf(stderr, "RAX saved as %d\n", save->RAX);
#endif

       /* Lock the LCD registers... */
       outb(vgaIOBase + 4, 0x1D);
       temp1 = inb(vgaIOBase + 5);
       outb(vgaIOBase + 5, (temp1 & 0x7f));
       }

  return ((void *) save);
}

/*
 * cirrusInit -- 
 *      Handle the initialization, etc. of a screen.
 */
static Bool
cirrusInit(mode)
     DisplayModePtr mode;
{
#ifndef MONOVGA 
#ifdef ALLOW_8BPP_MULTIPLEXING
     int multiplexing;

     multiplexing = 0;
     if (vgaBitsPerPixel == 8
     && (cirrusChip == CLGD5434 || cirrusChip == CLGD5436)
     && vga256InfoRec.clock[mode->Clock] > CLOCK_PALETTE_DOUBLING_5434) {
         /* On the 5434, enable palette doubling mode for clocks > 85.5 MHz. */
         multiplexing = 1;
         /* The actual DAC register value is set later. */
         /* The CRTC is clocked at VCLK / 2, so we must half the */
         /* horizontal timings. */
         if (!mode->CrtcHAdjusted) {
            mode->CrtcHDisplay >>= 1;
            mode->CrtcHSyncStart >>= 1;
            mode->CrtcHTotal >>= 1;
            mode->CrtcHSyncEnd >>= 1;
            mode->CrtcHAdjusted = TRUE;
         }
     }
#endif
#endif

  if (mode->VTotal >= 1024 && !(mode->Flags & V_INTERLACE)
  && !mode->CrtcVAdjusted) {
      /* For non-interlaced vertical timing >= 1024, the vertical timings */
      /* are divided by 2 and VGA CRTC 0x17 bit 2 is set. */
      mode->CrtcVDisplay >>= 1;
      mode->CrtcVSyncStart >>= 1;
      mode->CrtcVSyncEnd >>= 1;
      mode->CrtcVTotal >>= 1;
      mode->CrtcVAdjusted = TRUE;
  }

  if (!vgaHWInit(mode,sizeof(vgacirrusRec)))
    return(FALSE);

/*  unsigned char GR9;		 Graphics Offset1 */
/*  unsigned char GRA;		 Graphics Offset2 */
/*  unsigned char GRB;		 Graphics Extensions Control */
/*  unsigned char SR7;		 Extended Sequencer */
/*  unsigned char SRE;		 VCLK Numerator */
/*  unsigned char SRF;		 DRAM Control */
/*  unsigned char SR10;		 Graphics Cursor X Position [7:0]         */
/*  unsigned char SR10E;	 Graphics Cursor X Position [7:5] | 10000 */
/*  unsigned char SR11;		 Graphics Cursor Y Position [7:0]         */
/*  unsigned char SR11E;	 Graphics Cursor Y Position [7:5] | 10001 */
/*  unsigned char SR12;		 Graphics Cursor Attributes Register */
/*  unsigned char SR13;		 Graphics Cursor Pattern Address */
/*  unsigned char SR16;		 Performance Tuning Register */
/*  unsigned char SR17;		 Configuration/Extended Control */
/*  unsigned char SR1E;		 VCLK Denominator */
/*  unsigned char CR19;		 Interlace End */
/*  unsigned char CR1A;		 Miscellaneous Control */
/*  unsigned char CR1B;		 Extended Display Control */
/*  unsigned char CR1D;		 Overlay Extended Control Register */
/*  unsigned char HIDDENDAC;	 Hidden DAC register */
/*  DACcolourRec  FOREGROUND;    Hidden DAC cursor foreground colour */
/*  DACcolourRec  BACKGROUND;    Hidden DAC cursor background colour */
  
				/* Set the clock regs */

     if (new->std.NoClock >= 0)
          {
          unsigned char tempreg;
          int SRE, SR1E;
          int usemclk;
          
          if (new->std.NoClock >= NUM_CIRRUS_CLOCKS)
               {
               ErrorF("Invalid clock index -- too many clocks in XF86Config\n");
               return(FALSE);
               }
				/* Always use VLCK3 */

          new->std.MiscOutReg |= 0x0C;

	  if (!cirrusCheckClock(cirrusChip, new->std.NoClock))
	       return (FALSE);

	  if (cirrusReprogrammedMCLK > 0)
	      new->SR1F = cirrusReprogrammedMCLK;
	  else {
 	      outb(0x3c4, 0x1f);		/* MCLK register. */
	      new->SR1F = inb(0x3c5);
	  }

	  if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions)) {
	      if (vgaBitsPerPixel == 16 && cirrusChip <= CLGD5424)
	          CirrusFindClock(vga256InfoRec.clock[new->std.NoClock] * 2,
	              &SRE, &SR1E, &usemclk);
	      else
	          CirrusFindClock(vga256InfoRec.clock[new->std.NoClock],
	              &SRE, &SR1E, &usemclk);
	      if (usemclk && (cirrusChip == CLGD5428 || cirrusChip == CLGD5429
	      || HAVE543X())) {
	          new->SR1F |= 0x40;	/* Use MCLK as VLCK. */
	          SR1E &= 0xfe;	        /* Clear bit 0 of SR1E. */
	      }
	  }
	  else {
              SRE = cirrusClockTab[new->std.NoClock].numer;
              SR1E = cirrusClockTab[new->std.NoClock].denom;
#ifndef MONOVGA
       if (vgaBitsPerPixel == 16 && cirrusChip <= CLGD5424) {
	   /* Use the clocking mode whereby the programmed VCLK */
	   /* is double the pixel rate. */
	   SRE = cirrusDoubleClockTab[new->std.NoClock].numer;
	   SR1E = cirrusDoubleClockTab[new->std.NoClock].denom;
       }
#endif
	  }
				/* Be nice to the reserved bits... */
          outb(0x3C4,0x0E);
          tempreg = inb(0x3C5);
          new->SRE = (tempreg & 0x80) | (SRE & 0x7F);

          outb(0x3C4,0x1E);
          tempreg = inb(0x3C5);
          new->SR1E = (tempreg & 0xC0) | (SR1E & 0x3F);
          }
     
#ifndef MONOVGA
     if (vgaBitsPerPixel > 8)
         /* This is for 16bpp and 32bpp. */
     	 /* In 32bpp mode, the value is multiplied by two by the chip. */ 
         new->std.CRTC[0x13] = vga256InfoRec.virtualX >> 2;
     else
         new->std.CRTC[0x13] = vga256InfoRec.virtualX >> 3;
#endif

				/* Enable Dual Banking */
     new->GRB = 0x01;
#ifdef CIRRUS_SUPPORT_LINEAR
     if (cirrusUseLinear)
     	 /* Linear addressing requires single-banking to be set. */
         new->GRB = 0x00;
#endif         

     /* Initialize the read and write bank such a way that we initially */
     /* have an effective 64K window at the start of video memory. */
     new->GR9 = 0x00;
     new->GRA = (CIRRUS.ChipSetRead != cirrusSetRead) ? 0x02 : 0x08;

     /* Write sane value to Display Compression Control register, which */
     /* might be corrupted by other chipset probes. */
     if (HAVE543X()) {
         outb(0x3ce, 0x0f);
         new->GRF = inb(0x3cf) & 0xc0;
     }

     outb(0x3C4,0x0F);
     new->SRF = inb(0x3C5);

#ifdef PC98_WAB
     new->SRF &= 0xF7;
     new->SRF |= 0x10;
     outb(0x3C4,0x0F);
     outb(0x3C5,new->SRF);

#endif /* PC98_WAB */
     /* This following bit was not set correctly. */
     /* It is vital for correct operation at high dot clocks. */
 
     if ((cirrusChip >= CLGD5422 && cirrusChip <= CLGD5429)
     || HAVE543X())
	 {
         new->SRF |= 0x20;	/* Enable 64 byte FIFO. */
         }

#ifndef MONOVGA
     if ((cirrusChip >= CLGD5424 && cirrusChip <= CLGD5429) || HAVE543X())
         {
         int fifoshift_5430;
         int pixelrate, bandwidthleft, bandwidthused, percent;
	 /* Now set the CRT FIFO threshold (in 4 byte words). */
	 outb(0x3C4,0x16);
	 new->SR16 = inb(0x3C5) & 0xF0;

         /* The 5430 has extra 4 levels of buffering; adjust the FIFO */
         /* threshold values for that chip. */
	 fifoshift_5430 = 0;
	 if (cirrusChip == CLGD5430)
	     fifoshift_5430 = 4;

	 /*
	  * Calculate the relative amount of video memory bandwidth
	  * taken up by screen refresh to help with FIFO threshold
	  * setting.
	  */
         pixelrate = vga256InfoRec.clock[new->std.NoClock];
         if (vgaBitsPerPixel == 32)
         	bandwidthused = pixelrate * 4;
         else
         if (vgaBitsPerPixel == 16)
         	bandwidthused = pixelrate * 2;
         else
	 	bandwidthused = pixelrate;
	 bandwidthleft = cirrusDRAMBandwidth - bandwidthused;
	 /* Relative amount of bandwidth left for drawing. */
	 percent = bandwidthleft * 100 / cirrusDRAMBandwidth;
         
	 /* We have an option for conservative, or aggressive setting. */
	 /* The default is something in between. */

	 if (cirrusChip == CLGD5434 || cirrusChip == CLGD5436) {
	 	int threshold;
	 	int hsyncdelay;
	 	/*
	 	 * Small difference between HTotal and HSyncStart
	 	 * (first and second horizontal numbers) requires
	 	 * much higher FIFO threshold setting. It might be
	 	 * worthwhile to automatically modify the timing.
	 	 */
	 	hsyncdelay = mode->CrtcHSyncStart - mode->CrtcHDisplay;
	 	/* The 5434 has 16 extra levels of buffering. */
	 	if (OFLG_ISSET(OPTION_FIFO_AGGRESSIVE, &vga256InfoRec.options))
			/* Aggressive setting, effectively 16. */
			threshold = 0;
		else {
			/* Default FIFO setting for 5434. */
			threshold = 1;	/* Effectively 17. */
			if (cirrusDRAMBandwidth <= 250000) {
				if (bandwidthused >= 125000)
					threshold = 4;
				if (bandwidthused >= 155000)
					threshold = 8;
				if (hsyncdelay <= 16)
					threshold = 10;
			}
			if (cirrusDRAMBandwidth <= 210000) {
				if (bandwidthused >= 125000)
					threshold = 8;
				if (bandwidthused >= 155000)
					threshold = 12;
				if (hsyncdelay <= 16)
					threshold = 12;
			}
			if (OFLG_ISSET(OPTION_FIFO_CONSERV,
			&vga256InfoRec.options)) {
				threshold = 8;
				if (bandwidthused >= 125000)
					threshold = 12;
				if (bandwidthused >= 155000)
					threshold = 14;
			}
		}
		new->SR16 |= threshold;
	 }
         else {
		 /* XXXX Is 0 required for interlaced modes on some chips? */
		 int threshold;
	         threshold = 8;
	         if (OFLG_ISSET(OPTION_FIFO_CONSERV, &vga256InfoRec.options)) {
	         	/* Conservative FIFO threshold setting. */
	         	if (bandwidthused >= 59000)
	         		threshold = 12;
	         	if (bandwidthused >= 64000)
	         		threshold = 14;
	         	if (bandwidthused >= 79000)
	         		threshold = 15;
	         }
	         else
		 if (!OFLG_ISSET(OPTION_FIFO_AGGRESSIVE, &vga256InfoRec.options)) {
		 	/* Default FIFO threshold setting. */
		 	if (bandwidthused >= 64000)
		 		threshold = 10;
		 	if (bandwidthused >= 71000)
		 		threshold = 12;
		 	if (cirrusChip < CLGD5428) {
			 	if (bandwidthused >= 74000)
			 		threshold = 14;
		 		if (bandwidthused >= 83000)
		 			threshold = 15;
		 	}
		 	else {
		 		/* Based on the observation that the 5428 */
		 		/* BIOS 77 MHz 1024x768 mode uses 12. */
		 		if (bandwidthused >= 79000)
		 			threshold = 14;
		 		if (bandwidthused >= 88000)
		 			threshold = 15;
		 	}
		 }
		 else {
		 	/* Aggressive setting. */
		 	if (bandwidthused >= 69000)
		 		threshold = 10;
		 	if (bandwidthused >= 79000)
		 		threshold = 12;
		 	if (bandwidthused >= 88000)
		 		threshold = 14;
		 }
		 /* Agressive FIFO threshold setting is always 8. */
		 new->SR16 |= threshold - fifoshift_5430;
         } /* endelse */
         } /* endif */
#endif

     if (cirrusChip == CLGD5430
     && !OFLG_ISSET(OPTION_NO_2MB_BANKSEL, &vga256InfoRec.options))
     	  /* The 5430 always uses DRAM 'bank switching' bit. */
          new->SRF |= 0x80;

     if (CIRRUS.ChipSetRead != cirrusSetRead)
	  {
	  new->GRB |= 0x20;	/* Set 16k bank granularity */
	  if (cirrusChip != CLGD5434 && cirrusChip != CLGD5436)
#ifdef MONOVGA
	      if (vga256InfoRec.virtualX * vga256InfoRec.virtualY / 2 >
	      (1024 * 1024)
#endif
#if !defined(MONOVGA)
	      /* We want to be able to use all off-screen memory in */
	      /* principle. */
	      if (vga256InfoRec.videoRam == 2048
#if 0
	      if (vga256InfoRec.virtualX * vga256InfoRec.virtualY + 256 >
	      (1024 * 1024)
#endif
#endif
	      && !OFLG_ISSET(OPTION_NO_2MB_BANKSEL, &vga256InfoRec.options))
	          new->SRF |= 0x80;	/* Enable the second MB. */
	  				/* This may be a bad thing for some */
	  				/* 2Mb cards. */
	  }

#ifdef MONOVGA
     new->SR7 = 0x00;		/* Use 16 color mode. */
#else
     /*
      * There are two 16bpp clocking modes.
      * 'Clock / 2 for 16-bit/Pixel Data' clocking mode (0x03).
      * This works on all 542x chips, but requires VCLK to be twice
      * the pixel rate.
      * The alternative method, '16-bit/Pixel Data at Pixel Rate' (0x07),
      * is supported from the 5426 (this is the way to go for high clocks
      * on the 5434), with VCLK at pixel rate.
      * Both modes use normal 8bpp CRTC timing.
      *
      * On the 5434, there's also a 32bpp mode (0x09), that appears to want
      * the VCLK at pixel rate and the same CRTC timings as 8bpp (i.e.
      * nicely compatible).
      */
     if (vgaBitsPerPixel == 8) {
#ifdef ALLOW_8BPP_MULTIPLEXING
         if (multiplexing)
             new->SR7 = 0x07;	/* 5434 palette clock doubling mode */
         else
#endif
             new->SR7 = 0x01;	/* Tell it to use 256 Colors */
     }
     if (vgaBitsPerPixel == 16) {
         if (cirrusChip <= CLGD5424)
             /* Use the double VCLK mode. */
             new->SR7 = 0x03;
         else
             new->SR7 = 0x07;
     }
     if (vgaBitsPerPixel == 32)
         new->SR7 = 0x09;
#endif

#ifdef CIRRUS_SUPPORT_LINEAR
     if (cirrusUseLinear)
         new->SR7 |= 0x0E << 4;	/* Map at 14Mb. */
#endif

     if (mode->VTotal >= 1024 && !(mode->Flags & V_INTERLACE))
         /* For non-interlaced vertical timing >= 1024, the pogrammed */
         /* vertical timings are multiplied by 2 by setting this bit. */
         new->std.CRTC[0x17] |= 0x04;

				/* Fill up all the overflows - ugh! */
#ifdef DEBUG_CIRRUS
     fprintf(stderr,"Init: CrtcVSyncStart + 1 = %x\n\
CrtcHsyncEnd>>3 = %x\n\
CrtcHDisplay>>3 -1 = %x\n\
VirtX = %x\n",
	     mode->CrtcVSyncStart + 1,
	     mode->CrtcHSyncEnd >> 3, 
	     (mode->CrtcHDisplay >> 3) - 1,
	     vga256InfoRec.virtualX>>4);
#endif
     
     new->CR1A = (((mode->CrtcVSyncStart + 1) & 0x300 ) >> 2)
	  | (((mode->CrtcHSyncEnd >> 3) & 0xC0) >> 2);

     if (mode->Flags & V_INTERLACE) 
	    {
				/* ``Half the Horizontal Total'' which is */
				/* really half the value in CR0 */

	    new->CR19 = ((mode->CrtcHTotal >> 3) - 5) >> 1;
	    new->CR1A |= 0x01;
	    }
     else new->CR19 = 0x00;

     /* Extended logical scanline length bit. */
#ifdef MONOVGA
     new->CR1B = (((vga256InfoRec.virtualX>>4) & 0x100) >> 4)
	  | 0x22;
#else
     if (vgaBitsPerPixel > 8)
         new->CR1B = (((vga256InfoRec.virtualX>>2) & 0x100) >> 4) | 0x22;
     else
         new->CR1B = (((vga256InfoRec.virtualX>>3) & 0x100) >> 4) | 0x22;
#endif	  

     if (cirrusChip == CLGD5434 || cirrusChip == CLGD5436) {
          outb(vgaIOBase + 4, 0x1D);
          /* Set display start address bit 19 to 0. */
          new->CR1D = inb(vgaIOBase + 5) & 0x7f;
     }

     if (Is_754x(cirrusChip)) {
	  /* Clear the LSB for 800x600 modes */
	  if (mode->HDisplay == 800)
	       new->CR2D = 0x00;
	  else
	       new->CR2D = 0x01;
     }

     if (HAVE543X() || cirrusChip == CLGD5429) {
          outb(0x3c4, 0x17);
          new->SR17 = inb(0x3c5);
#ifdef CIRRUS_SUPPORT_MMIO
          /* Optionally enable Memory-Mapped I/O. */
          if (OFLG_ISSET(OPTION_MMIO, &vga256InfoRec.options)) {
     	       /* Set SR17 bit 2. */
               new->SR17 |= 0x04;
               if (cirrusChip != CLGD5434)
               	   /* Clear bit 6 to select mmio address space at 0xB8000. */
                   new->SR17 &= ~0x40;
          }
#endif
     }

     new->HIDDENDAC = 0;
#ifndef MONOVGA
     /*
      * Set the Hidden DAC register to the proper value for 16bpp,
      * 32bpp and high dot clock 8bpp mode.
      */
     if (vgaBitsPerPixel == 16) {
         if (xf86weight.red == 5 && xf86weight.green == 5
         && xf86weight.blue == 5)
             /* 5-5-5 RGB mode */
             if (cirrusChip >= CLGD5426)
                 new->HIDDENDAC = 0xd0;	/* Double edge mode. */
             else
                 new->HIDDENDAC = 0xf0; /* Single edge mode (double VLCK). */
         if (xf86weight.red == 5 && xf86weight.green == 6
         && xf86weight.blue == 5)
             /* 5-6-5 RGB mode */
             if (cirrusChip >= CLGD5426)
                 new->HIDDENDAC = 0xd1;
             else
                 new->HIDDENDAC = 0xe1;
     }
     if (vgaBitsPerPixel == 32)
         /* Set 24-bit color 8-8-8 RGB mode. */
         new->HIDDENDAC = 0xe5;

#ifdef ALLOW_8BPP_MULTIPLEXING
     if (multiplexing) {
         new->HIDDENDAC = 0x4A;
     }
#endif
#endif

     if (cirrusChip == CLGD6225) 
	  {
	  /* Don't ask me why the following number works, but it
	   * does work for a Sager 8200 using the BIOS initialization
	   * of the LCD for all other functions.  Without this, the
	   * Sager's display is 8 pixels left and 1 down from where
	   * it should be....  If things are shifted on your display,
	   * the documentation says to +1 for each 8 columns you want
	   * to move left...  but it seems to work in the opposite
	   * direction on my screen.  Anyway, this works for me, and
	   * it is easy to play with if it doesn't work for you.
	   */
	  new->RAX = 12;
          }

#ifdef CIRRUS_SUPPORT_MMIO
	/*
	 * Ugly hack to get MMIO base address.
         * Registers are mapped at 0xb8000; that is at an offset of
         * 0x18000 from vgaBase (0xa0000).
         */
	if (cirrusUseMMIO && cirrusMMIOBase == NULL)
		cirrusMMIOBase = (unsigned char *)vgaBase + 0x18000;
#endif

  return(TRUE);
}

/*
 * cirrusAdjust --
 *      adjust the current video frame to display the mousecursor
 */
static void 
cirrusAdjust(x, y)
     int x, y;
{
     unsigned char CR1B, CR1D, tmp;
#ifdef MONOVGA
     int Base = (y * vga256InfoRec.displayWidth + x);
     unsigned char lsb = Base & 7;
     Base >>= 3;
#else
     int Base = (y * vga256InfoRec.displayWidth + x);
     if (vgaBitsPerPixel == 16)
         Base <<= 1;
     if (vgaBitsPerPixel == 32)
         Base <<= 2;
     Base >>= 2;
#endif
     outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
     outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);

     outb(vgaIOBase + 4,0x1B); CR1B = inb(vgaIOBase + 5);
     outb(vgaIOBase + 5,(CR1B & 0xF2) | ((Base & 0x060000) >> 15)
	  | ((Base & 0x010000) >> 16) );
     if (cirrusChip == CLGD5434 || cirrusChip == CLGD5436) {
         outb(vgaIOBase + 4, 0x1d); CR1D = inb(vgaIOBase + 5);
         outb(vgaIOBase + 5, (CR1D & 0x7f) | ((Base & 0x080000) >> 12));
     }

#if 0
	/* Set the lowest two bits. */
	inb(vgaIOBase + 0xA);		/* Set ATC to addressing mode */
	outb(0x3C0, 0x13 + 0x20);	/* Select ATC reg 0x13 */
	tmp = inb(0x3C1) & 0xF0;
	outb(0x3C0, tmp | lsb);
#endif
}

/*
 * cirrusValidMode --
 *
 */
static Bool
cirrusValidMode(mode)
DisplayModePtr mode;
{
return TRUE;
}

