/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/vga/vga.c,v 3.44 1996/01/12 14:38:57 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 */
/* $XConsortium: vga.c /main/18 1996/01/12 13:10:23 kaleb $ */

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "mipointer.h"
#include "cursorstr.h"
#include "gcstruct.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"
#include "vga.h"
#include "vgaPCI.h"

#ifdef PC98
#include "pc98_vers.h"
#endif

#ifdef XFreeXDGA
#include "extnsionst.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#if !defined(MONOVGA) && !defined(XF86VGA16)
#include "vga256.h"
#include "cfb16.h"
#include "cfb32.h"
#include "vgabpp.h"
#endif

#ifdef PC98_EGC
#define EGC_PLANE	0x4a0
#define EGC_MODE	0x4a4
#define EGC_FGC		0x4a6
#define EGC_COPY_MODE	0x2cac
#endif


#ifndef XF86VGA16
#ifdef MONOVGA
extern void mfbDoBitbltCopy();
extern void mfbDoBitbltCopyInverted();
#ifdef BANKEDMONOVGA
extern void mfbDoBitbltTwoBanksCopy();
extern void mfbDoBitbltTwoBanksCopyInverted();
void (*ourmfbDoBitbltCopy)();
void (*ourmfbDoBitbltCopyInverted)();
#endif
#else
unsigned long useSpeedUp = 0;
extern void speedupvga256TEGlyphBlt8();
extern void speedupvga2568FillRectOpaqueStippled32();
extern void speedupvga2568FillRectTransparentStippled32();
extern void OneBankvgaBitBlt();
#endif /* MONOVGA */
#endif /* !XF86VGA16 */

extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed;
extern char *xf86VisualNames[];
extern Bool miDCInitialize();

ScrnInfoRec vga256InfoRec = {
  FALSE,		/* Bool configured */
  -1,			/* int tmpIndex */
  -1,			/* int scrnIndex */
  vgaProbe,		/* Bool (* Probe)() */
  vgaScreenInit,	/* Bool (* Init)() */
  vgaValidMode,		/* Bool (* ValidMode)() */
  vgaEnterLeaveVT,	/* void (* EnterLeaveVT)() */
  (void (*)())NoopDDA,		/* void (* EnterLeaveMonitor)() */
  (void (*)())NoopDDA,		/* void (* EnterLeaveCursor)() */
  vgaAdjustFrame,	/* void (* AdjustFrame)() */
  vgaSwitchMode,	/* Bool (* SwitchMode)() */
  vgaPrintIdent,        /* void (* PrintIdent)() */
#ifdef MONOVGA
  1,			/* int depth */
  {0, 0, 0},            /* xrgb weight */
  1,			/* int bitsPerPixel */
  StaticGray,		/* int defaultVisual */
#else
#ifdef XF86VGA16
  4,			/* int depth */
#else
  8,			/* int depth */
#endif
  {5, 6, 5},            /* xrgb weight */
  8,			/* int bitsPerPixel */
  PseudoColor,		/* int defaultVisual */
#endif
  -1, -1,		/* int virtualX,virtualY */
  -1,                   /* int displayWidth */
  -1, -1, -1, -1,	/* int frameX0, frameY0, frameX1, frameY1 */
  {0, },		/* OFlagSet options */
  {0, },		/* OFlagSet clockOptions */
  {0, },                /* OFlagSet xconfigFlag */
  NULL,			/* char *chipset */
  NULL,			/* char *ramdac */
  0,			/* int dacSpeed */
  0,			/* int clocks */
  {0, },		/* int clock[MAXCLOCKS] */
  DEFAULT_MAX_CLOCK,	/* int maxClock */
  0,			/* int videoRam */
#ifndef	PC98_EGC
  0xC0000,		/* int BIOSbase */
#else
  0xE8000,		/* int BIOSbase */
#endif
  0,			/* unsigned long MemBase, unused for this driver */
  240, 180,		/* int width, height */
  0,			/* unsigned long speedup */
  NULL,			/* DisplayModePtr modes */
  NULL,			/* MonPtr monitor */
  NULL,			/* char *clockprog */
  -1,                   /* int textclock */
  FALSE,		/* Bool bankedMono */
#ifdef MONOVGA
  "VGA2",		/* char *name */
#else
#ifdef XF86VGA16
  "VGA16",		/* char *name */
#else
  "SVGA",		/* char *name */
#endif
#endif
  {0, 0, 0},		/* xrgb blackColour */ 
  {0x3F, 0x3F, 0x3F},	/* xrgb whiteColour */ 
#ifdef MONOVGA
  vga2ValidTokens,	/* int *validTokens */
  VGA2_PATCHLEVEL,	/* char *patchLevel */
#else
#ifdef XF86VGA16
  vga16ValidTokens,	/* int *validTokens */
  VGA16_PATCHLEVEL,	/* char *patchLevel */
#else
  vga256ValidTokens,	/* int *validTokens */
  SVGA_PATCHLEVEL,	/* char *patchLevel */
#endif
#endif
  0,			/* int IObase */
  0,			/* int PALbase */
  0,			/* int COPbase */
  0,			/* int POSbase */
  0,			/* int instance */
  0,			/* int s3Madjust */
  0,			/* int s3Nadjust */
  0,			/* int s3MClk */
  0,			/* unsigned long VGAbase */
  0,			/* int s3RefClk */
  0,			/* int suspendTime */
  0,			/* int offTime */
  -1,			/* int s3BlankDelay */
#ifdef XFreeXDGA
  0,                    /* int directMode */
  NULL,                 /* Set Vid Page */
  0,                    /* unsigned long physBase */
  0,                    /* int physSize */
#endif
};

pointer vgaOrigVideoState = NULL;
pointer vgaNewVideoState = NULL;
pointer vgaBase = NULL;
pointer vgaVirtBase = NULL;
pointer vgaLinearBase = NULL;
vgaPCIInformation *vgaPCIInfo = NULL;

void (* vgaEnterLeaveFunc)(
#if NeedFunctionPrototypes
    Bool
#endif
) = (void (*)())NoopDDA;
Bool (* vgaInitFunc)(
#if NeedFunctionPrototypes
    DisplayModePtr
#endif
) = (Bool (*)())NoopDDA;
Bool (* vgaValidModeFunc)(
#if NeedFunctionPrototypes
    DisplayModePtr
#endif
) = (Bool (*)())NoopDDA;
void * (* vgaSaveFunc)(
#if NeedFunctionPrototypes
    void *
#endif
) = (void *(*)())NoopDDA;
void (* vgaRestoreFunc)(
#if NeedFunctionPrototypes
    void *
#endif
) = (void (*)())NoopDDA;
void (* vgaAdjustFunc)(
#if NeedFunctionPrototypes
    int,
    int
#endif
) = (void (*)())NoopDDA;
void (* vgaSaveScreenFunc)() = vgaHWSaveScreen;
void (* vgaFbInitFunc)() = (void (*)())NoopDDA;
void (* vgaSetReadFunc)() = (void (*)())NoopDDA;
void (* vgaSetWriteFunc)() = (void (*)())NoopDDA;
void (* vgaSetReadWriteFunc)() = (void (*)())NoopDDA;
int vgaMapSize;
int vgaSegmentSize;
int vgaSegmentShift;
int vgaSegmentMask;
void *vgaReadBottom;
void *vgaReadTop;
void *vgaWriteBottom;
void *vgaWriteTop =    (pointer)&writeseg; /* dummy for linking */
Bool vgaReadFlag;
Bool vgaWriteFlag;
Bool vgaUse2Banks;
int  vgaInterlaceType;
OFlagSet vgaOptionFlags;
extern Bool vgaPowerSaver;
extern Bool clgd6225Lcd;
Bool vgaUseLinearAddressing;
int vgaPhysLinearBase;
int vgaLinearSize;
int vgaBitsPerPixel = 0;

#ifdef MONOVGA
int vgaReadseg=0;
int vgaWriteseg=0;
int vgaReadWriteseg=0;
#endif

int vgaIOBase;

static ScreenPtr savepScreen = NULL;
static PixmapPtr ppix = NULL;
static Bool (* saveInitFunc)();
static void * (* saveSaveFunc)();
static void (* saveRestoreFunc)();
static void (* saveAdjustFunc)();
static void (* saveSaveScreenFunc)();
static void (* saveSetReadFunc)();
static void (* saveSetWriteFunc)();
static void (* saveSetReadWriteFunc)();

vgaHWCursorRec vgaHWCursor;

#ifdef MONOVGA
static int validDepth = 1;
#endif
#ifdef XF86VGA16
static int validDepth = 4;
#endif

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
extern int defaultColorVisualClass;

#define Drivers vgaDrivers

extern vgaVideoChipPtr Drivers[];

/*
 * vgaRestore -- 
 *	Wrap the chip-level restore function in a protect/unprotect.
 */
void
vgaRestore(mode)
     pointer mode;
{
  vgaProtect(TRUE);
  (vgaRestoreFunc)(mode);
  vgaProtect(FALSE);
}

/*
 * vgaPrintIdent --
 *     Prints out identifying strings for drivers included in the server
 */
void
vgaPrintIdent()
{
  int            i, j, n, c;
  char		 *id;

#ifdef MONOVGA
  ErrorF("  %s: server for monochrome VGA (Patchlevel %s):\n      ",
         vga256InfoRec.name, vga256InfoRec.patchLevel);
#else
#ifdef XF86VGA16
  ErrorF("  %s: server for 4-bit colour VGA (Patchlevel %s):\n      ",
         vga256InfoRec.name, vga256InfoRec.patchLevel);
#ifdef	PC98_EGC
  ErrorF("\tmodified for PC98 EGC (Patchlevel %s):\n      ",PC98_VGA16_PL);
#endif
#else
  ErrorF("  %s: server for 8-bit colour SVGA (Patchlevel %s):\n      ",
         vga256InfoRec.name, vga256InfoRec.patchLevel);
#if defined(PC98_WAB)||defined(PC98_GANB_WAP)||defined(PC98_NKV) \
	||defined(PC98_NEC_CIRRUS)||defined(PC98_NEC480)
  ErrorF("\tmodified for PC98 WAB/WAP GA-98NB EPSON-NKV \n\tNEC-CIRRUS PEGC(Patchlevel %s):\n      ",PC98_SVGA_PL);
#endif
#ifdef PC98_WAB
  ErrorF("\tThis server was compiled for PC98 WAB.\n      ");
#endif
#ifdef PC98_GANB_WAP
  ErrorF("\tThis server was compiled for PC98 GA-98NB and WAP.\n      ");
#endif
#ifdef PC98_NKV
  ErrorF("\tThis server was compiled for PC98 EPSON-NKV and NKV2.\n      ");
#endif
#ifdef PC98_NEC_CIRRUS
  ErrorF("\tThis server was compiled for PC98 NEC-CIRRUS.\n      ");
#endif
#ifdef PC98_NEC480
  ErrorF("\tThis server was compiled for PC98 PEGC(640x480,256colors).\n      ");
#endif
#endif
#endif
  n = 0;
  c = 0;
  for (i=0; Drivers[i]; i++)
    for (j = 0; id = (Drivers[i]->ChipIdent)(j); j++, n++)
    {
      if (n)
      {
        ErrorF(",");
        c++;
        if (c + 1 + strlen(id) < 70)
        {
          ErrorF(" ");
          c++;
        }
        else
        {
          ErrorF("\n      ");
          c = 0;
        }
      }
      ErrorF("%s", id);
      c += strlen(id);
    }
  ErrorF("\n");

#if defined(PC98_WAB)||defined(PC98_GANB_WAP)||defined(PC98_NKV) \
	||defined(PC98_NEC_CIRRUS)||defined(PC98_NEC480)
  ErrorF("Supported video boards:\n \t%s \n    ",PC98_SVGA_BOARDS);
#endif
#ifdef	PC98_EGC
  ErrorF("Supported video boards:\n \t%s \n    ",PC98_VGA16_BOARDS);
#endif
}


/*
 * vgaProbe --
 *     probe and initialize the hardware driver
 */
Bool
vgaProbe()
{
  int            i, j, k;
  DisplayModePtr pMode, pEnd, pmaxX = NULL, pmaxY = NULL;
  int            maxX, maxY;
  int            needmem, rounding;
  int            tx,ty;

#if !defined(MONOVGA) && !defined(XF86VGA16)
  /*
   * Chipsets that don't support 16bpp/32bpp must have ChipHas16bpp/
   * ChipHas32bpp in the ChipRec structure set to FALSE after a
   * successful probe.
   */
  if (xf86bpp < 0) {
      xf86bpp = vga256InfoRec.depth;
  }
  if (xf86weight.red == 0 || xf86weight.green == 0 || xf86weight.blue == 0) {
      xf86weight = vga256InfoRec.weight;
  }
  vgaBitsPerPixel = xf86bpp;
  if (vgaBitsPerPixel != 8) {
      if (vgaBitsPerPixel != 16 && vgaBitsPerPixel != 32) {
          ErrorF("\n%s %s: Unsupported bpp for SVGA server (%d)\n",
              XCONFIG_GIVEN, vga256InfoRec.name, vgaBitsPerPixel);
	  return(FALSE);
      }
      /*
       * First override a few entries in the ScrnInfoRec structure for
       * 16/32bpp (which may not be entirely necessary).
       *
       * With 5-5-5 RGB, we don't want to use a depth of 15; depth 16 works
       * and is much faster with cfb16.
       */
      vga256InfoRec.depth = vgaBitsPerPixel;
      if (vgaBitsPerPixel == 32)
          xf86weight.red = xf86weight.green = xf86weight.blue = 8;
      vga256InfoRec.weight = xf86weight;
      vga256InfoRec.bitsPerPixel = vgaBitsPerPixel;
      vga256InfoRec.blackColour.red = 0;
      vga256InfoRec.blackColour.green = 0;
      vga256InfoRec.blackColour.blue = 0;
      if (xf86weight.green == 5) {
          vga256InfoRec.whiteColour.red = 0x1f;
          vga256InfoRec.whiteColour.green = 0x1f;
          vga256InfoRec.whiteColour.blue = 0x1f;
      }
      if (xf86weight.green == 6) {
          vga256InfoRec.whiteColour.red = 0x1f;
          vga256InfoRec.whiteColour.green = 0x3f;
          vga256InfoRec.whiteColour.blue = 0x1f;
      }
      if (xf86weight.green == 8) {
          vga256InfoRec.whiteColour.red = 0xff;
          vga256InfoRec.whiteColour.green = 0xff;
          vga256InfoRec.whiteColour.blue = 0xff;
      }
      /* Handle the default visual setting. */
      if (vga256InfoRec.defaultVisual < 0)
          vga256InfoRec.defaultVisual = TrueColor;
      if (defaultColorVisualClass < 0)
          defaultColorVisualClass = vga256InfoRec.defaultVisual;
      if (defaultColorVisualClass != TrueColor) {
	  ErrorF("Invalid default visual type: %d (%s)\n",
		 defaultColorVisualClass,
		 xf86VisualNames[defaultColorVisualClass]);
	  return(FALSE);
      }
  }
#else
  if (vga256InfoRec.depth != validDepth) {
    ErrorF("\n%s %s: Unsupported bpp for %s server (%d)\n",
           XCONFIG_GIVEN, vga256InfoRec.name, vga256InfoRec.name,
           vga256InfoRec.depth);
	  return(FALSE);
  }
#endif

#ifndef PC98
  /* First do a general PCI probe (unless disabled) */
  if (!OFLG_ISSET(OPTION_NO_PCI_PROBE, &vga256InfoRec.options)) {
    vgaPCIInfo = vgaGetPCIInfo();
  }
#endif

  for (i=0; Drivers[i]; i++)
  {
    vgaSaveScreenFunc = Drivers[i]->ChipSaveScreen;
    if ((Drivers[i]->ChipProbe)())
      {
        xf86ProbeFailed = FALSE;
#ifdef MONOVGA
#ifdef BANKEDMONOVGA
        /*
         * In the mono mode we use, the memory is divided into 4 planes
         * so we can only effectively use 1/4 of the total.  For cards
         * with < 256K there should be fewer planes.
         */
        if (vga256InfoRec.videoRam <= 256 || !vga256InfoRec.bankedMono)
            needmem = Drivers[i]->ChipMapSize * 8;
        else
	    needmem = vga256InfoRec.videoRam / 4 * 1024 * 8;
#else /* BANKEDMONOVGA */
	needmem = Drivers[i]->ChipMapSize * 8;
#endif /* BANKEDMONOVGA */
	rounding = 32;
#else /* MONOVGA */
#ifdef XF86VGA16
#ifdef PC98_EGC
	needmem = Drivers[i]->ChipMapSize * 8;
#else
	needmem = vga256InfoRec.videoRam / 4 * 1024 * 8;
#endif
	rounding = 32;
#else
	needmem = vga256InfoRec.videoRam * 1024;
	rounding = Drivers[i]->ChipRounding;
	/*
	 * Correct 'needmem' for 16/32bpp.
	 * For 8/16/32bpp, needmem is defined as the max. number of pixels.
	 */
	needmem /= (vgaBitsPerPixel / 8);
#endif /* XF86VGA16 */
#endif /* MONOVGA */

        if (xf86Verbose)
        {
	  ErrorF("%s %s: chipset:  %s\n",
             OFLG_ISSET(XCONFIG_CHIPSET,&vga256InfoRec.xconfigFlag) ? 
                    XCONFIG_GIVEN : XCONFIG_PROBED ,
             vga256InfoRec.name,
             vga256InfoRec.chipset);
#ifdef MONOVGA
	  ErrorF("%s %s: videoram: %dk (using %dk)",
#else
#ifdef XF86VGA16
	  ErrorF("%s %s: videoram: %dk (using %dk)",
#else
	  ErrorF("%s %s: videoram: %dk",
#endif
#endif
		 OFLG_ISSET(XCONFIG_VIDEORAM,&vga256InfoRec.xconfigFlag) ? 
                    XCONFIG_GIVEN : XCONFIG_PROBED ,
                 vga256InfoRec.name,
	         vga256InfoRec.videoRam
#ifdef MONOVGA
                 , needmem / 8 / 1024
#endif
#ifdef XF86VGA16
                 , needmem / 2 / 1024
#endif
	         );
	

	  for (j=0; j < vga256InfoRec.clocks; j++)
	  {
	    if ((j % 8) == 0)
	      ErrorF("\n%s %s: clocks:", 
                OFLG_ISSET(XCONFIG_CLOCKS,&vga256InfoRec.xconfigFlag) ? 
                    XCONFIG_GIVEN : XCONFIG_PROBED ,
                vga256InfoRec.name);
	    ErrorF(" %6.2f", (double)vga256InfoRec.clock[j]/1000.0);
	  }
	  ErrorF("\n");
        }

	/* Scale raw clocks to give pixel clocks if driver requires it */
	if (Drivers[i]->ChipClockScaleFactor > 1) {
	  for (j = 0; j < vga256InfoRec.clocks; j++)
	    vga256InfoRec.clock[j] /= Drivers[i]->ChipClockScaleFactor;
	  vga256InfoRec.maxClock /= Drivers[i]->ChipClockScaleFactor;
	  if (xf86Verbose) {
	    ErrorF("%s %s: Effective pixel clocks available:\n",
		   XCONFIG_PROBED, vga256InfoRec.name);
	    for (j=0; j < vga256InfoRec.clocks; j++)
	    {
	      if ((j % 8) == 0)
	      {
		if (j != 0)
		  ErrorF("\n");
	        ErrorF("%s %s: pixel clocks:", XCONFIG_PROBED,
		       vga256InfoRec.name);
	      }
	      ErrorF(" %6.2f", (double)vga256InfoRec.clock[j] / 1000.0);
	    }
	    ErrorF("\n");
	  }
	}

#if !defined(MONOVGA) && !defined(XF86VGA16)
        /*
         * If bpp is 16 or 32, make sure the driver supports it.
         */
        if ((vgaBitsPerPixel == 16 && !Drivers[i]->ChipHas16bpp)
        || (vgaBitsPerPixel == 32 && !Drivers[i]->ChipHas32bpp)) {
            ErrorF("\n%s %s: %dbpp not supported for this chipset\n",
                XCONFIG_GIVEN, vga256InfoRec.name, vgaBitsPerPixel);
	    Drivers[i]->ChipEnterLeave(LEAVE);
	    return(FALSE);
        }
#endif

	vgaEnterLeaveFunc = Drivers[i]->ChipEnterLeave;
	vgaInitFunc = Drivers[i]->ChipInit;
	vgaValidModeFunc = Drivers[i]->ChipValidMode;
	vgaSaveFunc = Drivers[i]->ChipSave;
	vgaRestoreFunc = Drivers[i]->ChipRestore;
	vgaAdjustFunc = Drivers[i]->ChipAdjust;
	vgaFbInitFunc = Drivers[i]->ChipFbInit;
#ifndef PC98_EGC
	vgaSetReadFunc = Drivers[i]->ChipSetRead;
	vgaSetWriteFunc = Drivers[i]->ChipSetWrite;
	vgaSetReadWriteFunc = Drivers[i]->ChipSetReadWrite;
#endif		 
	vgaMapSize = Drivers[i]->ChipMapSize;
	vgaSegmentSize = Drivers[i]->ChipSegmentSize;
	vgaSegmentShift = Drivers[i]->ChipSegmentShift;
	vgaSegmentMask = Drivers[i]->ChipSegmentMask;
	vgaReadBottom = (pointer)Drivers[i]->ChipReadBottom;
	vgaReadTop = (pointer)Drivers[i]->ChipReadTop;
	vgaWriteBottom = (pointer)Drivers[i]->ChipWriteBottom;
	vgaWriteTop = (pointer)Drivers[i]->ChipWriteTop;
	vgaUse2Banks = Drivers[i]->ChipUse2Banks;
	vgaInterlaceType = Drivers[i]->ChipInterlaceType;
	vgaOptionFlags = Drivers[i]->ChipOptionFlags;
	OFLG_SET(OPTION_POWER_SAVER, &vgaOptionFlags);
	OFLG_SET(OPTION_CLGD6225_LCD, &vgaOptionFlags);
	OFLG_SET(OPTION_NO_PCI_PROBE, &vgaOptionFlags);

	xf86VerifyOptions(&vgaOptionFlags, &vga256InfoRec);

	if (OFLG_ISSET(OPTION_POWER_SAVER, &vga256InfoRec.options))
	    vgaPowerSaver = TRUE;
	if (OFLG_ISSET(OPTION_CLGD6225_LCD, &vga256InfoRec.options))
	    clgd6225Lcd = TRUE;

	/* if Virtual given: is the virtual size too big? */
#ifdef BANKEDMONOVGA
	if (vga256InfoRec.virtualX > (2048)) {
		ErrorF("%s: Virtual width %i exceeds max. virtual width %i\n",
		       vga256InfoRec.name, vga256InfoRec.virtualX, (2048));
		vgaEnterLeaveFunc(LEAVE);
		return(FALSE);
	}
	if (vga256InfoRec.virtualX*vga256InfoRec.virtualY <= 8*vgaMapSize)
		/* may be unbanked */
		vga256InfoRec.displayWidth = vga256InfoRec.virtualX;
	else if (vga256InfoRec.virtualX > (1024))
		vga256InfoRec.displayWidth=2048;
	     else vga256InfoRec.displayWidth=1024;

	if (vga256InfoRec.virtualX > 0 &&
	    vga256InfoRec.displayWidth * vga256InfoRec.virtualY > needmem)
	  {
	    ErrorF("%s: Too little memory for virtual resolution\n"
                   "      %d (display width %d) x %d\n",
                   vga256InfoRec.name, vga256InfoRec.virtualX,
                   vga256InfoRec.displayWidth, vga256InfoRec.virtualY);
            vgaEnterLeaveFunc(LEAVE);
	    return(FALSE);
	  }
#else
	if (vga256InfoRec.virtualX > 0 &&
	    vga256InfoRec.virtualX * vga256InfoRec.virtualY > needmem)
	  {
	    ErrorF("%s: Too little memory for virtual resolution %d %d\n",
                   vga256InfoRec.name, vga256InfoRec.virtualX,
                   vga256InfoRec.virtualY);
            vgaEnterLeaveFunc(LEAVE);
	    return(FALSE);
	  }
#endif

        maxX = maxY = -1;
	tx = vga256InfoRec.virtualX;
	ty = vga256InfoRec.virtualY;

        if (Drivers[i]->ChipBuiltinModes) {
          pEnd = pMode = vga256InfoRec.modes = Drivers[i]->ChipBuiltinModes;
          ErrorF("%s %s: Using builtin driver modes\n", XCONFIG_PROBED,
                 vga256InfoRec.name);
	  do {
            ErrorF("%s %s: Builtin Mode: %s\n", XCONFIG_PROBED,
                   vga256InfoRec.name, pMode->name);
	    if (pMode->HDisplay > maxX)
	    {
	      maxX = pMode->HDisplay;
	      pmaxX = pMode;
	    }
	    if (pMode->VDisplay > maxY)
	    {
	      maxY = pMode->VDisplay;
	      pmaxY = pMode;
	    }
            pMode = pMode->next;
          } while (pMode != pEnd);
        } else if (vga256InfoRec.modes == NULL) {
          ErrorF("No modes supplied in XF86Config\n");
	  vgaEnterLeaveFunc(LEAVE);
	  return(FALSE);
        } else {
	  pMode = vga256InfoRec.modes;
	  pEnd = (DisplayModePtr) NULL;
	  do {
	    DisplayModePtr pModeSv;

	    /*
	     * xf86LookupMode returns FALSE if it ran into an invalid
	     * parameter
	     */
	    if(xf86LookupMode(pMode, &vga256InfoRec) == FALSE) {
	      pModeSv = pMode->next;
	      xf86DeleteMode(&vga256InfoRec, pMode);
	      pMode = pModeSv; 
	    } else if (pMode->HDisplay * pMode->VDisplay > needmem) {
	      pModeSv=pMode->next;
	      ErrorF("%s %s: Insufficient video memory for all resolutions\n",
		     XCONFIG_PROBED, vga256InfoRec.name);
	      xf86DeleteMode(&vga256InfoRec, pMode);
	      pMode = pModeSv;
	    } else if (((tx > 0) && (pMode->HDisplay > tx)) || 
		       ((ty > 0) && (pMode->VDisplay > ty))) {
	      pModeSv=pMode->next;
	      ErrorF("%s %s: Resolution %dx%d too large for virtual %dx%d\n",
		     XCONFIG_PROBED, vga256InfoRec.name,
		     pMode->HDisplay, pMode->VDisplay, tx, ty);
	      xf86DeleteMode(&vga256InfoRec, pMode);
	      pMode = pModeSv;
	    } else {
	      /*
	       * Successfully looked up this mode.  If pEnd isn't 
	       * initialized, set it to this mode.
	       */
	      if (pEnd == (DisplayModePtr) NULL)
		pEnd = pMode;

	      if (pMode->HDisplay > maxX)
	      {
		maxX = pMode->HDisplay;
		pmaxX = pMode;
	      }
	      if (pMode->VDisplay > maxY)
	      {
		maxY = pMode->VDisplay;
		pmaxY = pMode;
	      }

	      pMode = pMode->next;
	    }
	  } while (pMode != pEnd);
        }

        vga256InfoRec.virtualX = max(maxX, vga256InfoRec.virtualX);
        vga256InfoRec.virtualY = max(maxY, vga256InfoRec.virtualY);
#ifdef BANKEDMONOVGA
	if (vga256InfoRec.virtualX > (2048)) {
		ErrorF("%s: Max. width %i exceeds max. virtual width %i\n",
			vga256InfoRec.name, vga256InfoRec.virtualX, (2048));
		vgaEnterLeaveFunc(LEAVE);
		return(FALSE);
	}
	/* Now that modes are resolved and max. extents are found,
	 * test size again */
	if (vga256InfoRec.virtualX*vga256InfoRec.virtualY <= 8*vgaMapSize)
		/* may be unbanked */
		vga256InfoRec.displayWidth = vga256InfoRec.virtualX;
	else 
	  {
             if (vga256InfoRec.virtualX > (1024))
		vga256InfoRec.displayWidth=2048;
	     else vga256InfoRec.displayWidth=1024;
	     if (xf86Verbose)
		ErrorF("%s %s: Display width set to %i\n",
			XCONFIG_PROBED, vga256InfoRec.name,
		        vga256InfoRec.displayWidth);
          }
#endif

#ifndef BANKEDMONOVGA
	if (vga256InfoRec.virtualX % rounding)
	  {
	    vga256InfoRec.virtualX -= vga256InfoRec.virtualX % rounding;
	    ErrorF(
	     "%s %s: Virtual width rounded down to a multiple of %d (%d)\n",
	     XCONFIG_PROBED, vga256InfoRec.name, rounding,
	     vga256InfoRec.virtualX);
            if (vga256InfoRec.virtualX < maxX)
            {
              ErrorF(
               "%s: Rounded down virtual width (%d) is too small for mode %s",
	       vga256InfoRec.name, vga256InfoRec.virtualX, pmaxX->name);
              vgaEnterLeaveFunc(LEAVE);
              return(FALSE);
            }
	  }

	if ( vga256InfoRec.virtualX * vga256InfoRec.virtualY > needmem)
	{
          if (vga256InfoRec.virtualX != maxX ||
              vga256InfoRec.virtualY != maxY)
	    ErrorF(
              "%s: Too little memory to accomodate virtual size and mode %s\n",
               vga256InfoRec.name,
               (vga256InfoRec.virtualX == maxX) ? pmaxX->name : pmaxY->name);
          else
	    ErrorF("%s: Too little memory to accomodate modes %s and %s\n",
                   vga256InfoRec.name, pmaxX->name, pmaxY->name);
          vgaEnterLeaveFunc(LEAVE);
	  return(FALSE);
	}
#else
	if ( vga256InfoRec.displayWidth * vga256InfoRec.virtualY > needmem) {
		ErrorF("%s: Too little memory to accomodate display width %i"
			" and virtual height %i\n",
			vga256InfoRec.name,
			vga256InfoRec.displayWidth, vga256InfoRec.virtualY);
		vgaEnterLeaveFunc(LEAVE);
		return(FALSE);
	}
#endif
	if ((tx != vga256InfoRec.virtualX) || (ty != vga256InfoRec.virtualY))
            OFLG_CLR(XCONFIG_VIRTUAL,&vga256InfoRec.xconfigFlag);

#ifndef BANKEDMONOVGA
        vga256InfoRec.displayWidth = vga256InfoRec.virtualX;
#endif
        if (xf86Verbose)
          ErrorF("%s %s: Virtual resolution set to %dx%d\n",
                 OFLG_ISSET(XCONFIG_VIRTUAL,&vga256InfoRec.xconfigFlag) ? 
                    XCONFIG_GIVEN : XCONFIG_PROBED ,
                 vga256InfoRec.name,
                 vga256InfoRec.virtualX, vga256InfoRec.virtualY);

#if !defined(XF86VGA16)
#if !defined(MONOVGA)
	if ((vga256InfoRec.speedup & ~SPEEDUP_ANYWIDTH) &&
            vga256InfoRec.virtualX != 1024)
	  {
	    ErrorF(
              "%s %s: SpeedUp code selection modified because virtualX != 1024\n",
              XCONFIG_PROBED, vga256InfoRec.name);
	    vga256InfoRec.speedup &= SPEEDUP_ANYWIDTH;
	    OFLG_CLR(XCONFIG_SPEEDUP,&vga256InfoRec.xconfigFlag);
	  }

        /*
         * Currently the 16bpp and 32bpp modes use stock cfb with linear
         * addressing, so avoid the SpeedUp code for these depths.
         */
      if (vgaBitsPerPixel == 8) {

        useSpeedUp = vga256InfoRec.speedup & SPEEDUP_ANYCHIPSET;
        if (useSpeedUp && xf86Verbose)
          ErrorF("%s %s: Generic SpeedUps selected (Flags=0x%x)\n",
               OFLG_ISSET(XCONFIG_SPEEDUP,&vga256InfoRec.xconfigFlag) ?
                    XCONFIG_GIVEN : XCONFIG_PROBED , 
               vga256InfoRec.name, useSpeedUp);

        /* We deal with the generic speedups here */
	if (useSpeedUp & SPEEDUP_TEGBLT8)
	{
	  vga256LowlevFuncs.teGlyphBlt8 = speedupvga256TEGlyphBlt8;
	  vga256TEOps1Rect.ImageGlyphBlt = speedupvga256TEGlyphBlt8;
	  vga256TEOps.ImageGlyphBlt = speedupvga256TEGlyphBlt8;
	}

	if (useSpeedUp & SPEEDUP_RECTSTIP)
	{
	  vga256LowlevFuncs.fillRectOpaqueStippled32 = 
	    speedupvga2568FillRectOpaqueStippled32;
	  vga256LowlevFuncs.fillRectTransparentStippled32 = 
	    speedupvga2568FillRectTransparentStippled32;
	}

	if (!vgaUse2Banks)
	{
	  vga256LowlevFuncs.vgaBitblt = OneBankvgaBitBlt;
	}

      } /* endif vgaBitsPerPixel == 8 */

	/* Initialise chip-specific enhanced fb functions */
	vgaHWCursor.Initialized = FALSE;
	(*vgaFbInitFunc)();

	/* The driver should now have determined whether linear */
	/* addressing is possible */
	vgaUseLinearAddressing = Drivers[i]->ChipUseLinearAddressing;
	vgaPhysLinearBase = Drivers[i]->ChipLinearBase;
	vgaLinearSize = Drivers[i]->ChipLinearSize;

#ifdef XFreeXDGA
	if (vgaUseLinearAddressing) {
	    vga256InfoRec.physBase = vgaPhysLinearBase;
	    vga256InfoRec.physSize = vgaLinearSize;
	} else {
	    vga256InfoRec.physBase = 0xA0000 + Drivers[i]->ChipWriteBottom;
	    vga256InfoRec.physSize = Drivers[i]->ChipSegmentSize;
	    vga256InfoRec.setBank = vgaSetVidPage;;
	}
#endif

	/* Currently linear addressing is required for 16/32bpp. */
	/* Bail out if it is not enabled. */
	if (vgaBitsPerPixel > 8 && !vgaUseLinearAddressing) {
 	    ErrorF("%s: Linear addressing is required for %dbpp\n",
 	    vga256InfoRec.name, vgaBitsPerPixel);
	    vgaEnterLeaveFunc(LEAVE);
	    return(FALSE);
	}

#else
#ifdef BANKEDMONOVGA
	if (vgaUse2Banks)
	{
	  ourmfbDoBitbltCopy = mfbDoBitbltTwoBanksCopy;
	  ourmfbDoBitbltCopyInverted = mfbDoBitbltTwoBanksCopyInverted;
	}
	else
	{
	  ourmfbDoBitbltCopy = mfbDoBitbltCopy;
	  ourmfbDoBitbltCopyInverted = mfbDoBitbltCopyInverted;
	}
#else
	ourmfbDoBitbltCopy = mfbDoBitbltTwoBanksCopy;
	ourmfbDoBitbltCopyInverted = mfbDoBitbltTwoBanksCopyInverted;
#endif /* BANKEDMONOVGA */
#endif /* !MONOVGA */
#endif /* !XF86VGA16 */

	return TRUE;
      }
  }

  vgaSaveScreenFunc = vgaHWSaveScreen;
  
  if (vga256InfoRec.chipset)
    ErrorF("%s: '%s' is an invalid chipset", vga256InfoRec.name,
	       vga256InfoRec.chipset);
  return FALSE;
}


/*
 * vgaScreenInit --
 *      Attempt to find and initialize a VGA framebuffer
 *      Most of the elements of the ScreenRec are filled in.  The
 *      video is enabled for the frame buffer...
 */

Bool
vgaScreenInit (scr_index, pScreen, argc, argv)
    int            scr_index;    /* The index of pScreen in the ScreenInfo */
    ScreenPtr      pScreen;      /* The Screen to initialize */
    int            argc;         /* The number of the Server's arguments. */
    char           **argv;       /* The arguments themselves. Don't change! */
{
  int displayResolution = 75;    /* default to 75dpi */
  extern int monitorResolution;

  if (serverGeneration == 1) {
#ifdef PC98_WAB
    vgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xE0000,
			    vgaMapSize);
#else
#ifdef PC98_GANB_WAP
    vgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xF00000,
			    vgaMapSize);
#else
#ifdef PC98_NKV
    vgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xF00000,
			    vgaMapSize);
#else
#ifdef	PC98_NEC_CIRRUS
    vgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xF00000,
			    vgaMapSize);
#else
#if	defined(PC98_EGC) || defined(PC98_NEC480)
    vgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xA8000,
			    vgaMapSize);
#else    
    vgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xA0000,
			    vgaMapSize);
#endif /* PC98_EGC  PC98_NE480 */
#endif /* PC98_NEC_CIRRUS */
#endif /* PC98_NKV */
#endif /* PC98_GANB_WAP */
#endif /* PC98_WAB  */
    if (vgaUseLinearAddressing)
        vgaLinearBase = xf86MapVidMem(scr_index, LINEAR_REGION,
        			      (pointer)vgaPhysLinearBase,
        			      vgaLinearSize);

#ifdef MONOVGA
    if (vga256InfoRec.displayWidth * vga256InfoRec.virtualY >= vgaMapSize * 8)
    {                                                     /* ^ mfb bug */
      ErrorF("%s %s: Using banked mono vga mode\n", 
          XCONFIG_PROBED, vga256InfoRec.name);
      vgaVirtBase = (pointer)VGABASE;
    }
    else
    {
      ErrorF("%s %s: Using non-banked mono vga mode\n", 
          XCONFIG_PROBED, vga256InfoRec.name);
      vgaVirtBase = vgaBase;
    }
#else
#ifdef XF86VGA16
    vgaVirtBase = vgaBase;
#else
    vgaVirtBase = (pointer)VGABASE;
#endif
#endif

    /* This doesn't mean anything yet for SVGA 16/32bpp (no banking). */
    if (vgaUseLinearAddressing)
    {
      vgaReadBottom = vgaLinearBase;
      vgaReadTop = (void *)((unsigned int)vgaLinearSize
      			    + (unsigned int)vgaLinearBase);
      vgaWriteBottom = vgaLinearBase;
      vgaWriteTop = (void *)((unsigned int)vgaLinearSize
      			    + (unsigned int)vgaLinearBase);
      vgaSegmentSize = vgaLinearSize;	/* override */
      vgaSegmentMask = vgaLinearSize - 1;
      vgaSetReadFunc = (void (*)())NoopDDA;
      vgaSetWriteFunc = (void (*)())NoopDDA;
      vgaSetReadWriteFunc = (void (*)())NoopDDA;
    }
    else
    {
      vgaReadBottom  = (void *)((unsigned int)vgaReadBottom
			        + (unsigned int)vgaBase); 
      vgaReadTop     = (void *)((unsigned int)vgaReadTop
			        + (unsigned int)vgaBase); 
      vgaWriteBottom = (void *)((unsigned int)vgaWriteBottom
			        + (unsigned int)vgaBase); 
      vgaWriteTop    = (void *)((unsigned int)vgaWriteTop
			        + (unsigned int)vgaBase); 
    }
  }

  if (!(vgaInitFunc)(vga256InfoRec.modes))
    FatalError("%s: hardware initialisation failed\n", vga256InfoRec.name);

  /*
   * This function gets called while in graphics mode during a server
   * reset, and this causes the original video state to be corrupted.
   * So, only initialise vgaOrigVideoState if it hasn't previously been done
   * DHD Dec 1991.
   */
  if (!vgaOrigVideoState)
    vgaOrigVideoState = (pointer)(vgaSaveFunc)(vgaOrigVideoState);
  vgaRestore(vgaNewVideoState);
#ifndef DIRTY_STARTUP
  vgaSaveScreen(NULL, FALSE); /* blank the screen */
#endif
  (vgaAdjustFunc)(vga256InfoRec.frameX0, vga256InfoRec.frameY0);

  /*
   * Take display resolution from the -dpi flag if specified
   */

  if (monitorResolution)
    displayResolution = monitorResolution;

  /*
   * Inititalize the dragon to color display
   */
#ifndef XF86VGA16
#ifdef MONOVGA
  if (!mfbScreenInit(pScreen,
		     (pointer) vgaVirtBase,
		     vga256InfoRec.virtualX,
		     vga256InfoRec.virtualY,
		     displayResolution, displayResolution,
		     vga256InfoRec.displayWidth))
#else
  if (vgaBitsPerPixel == 8)
      if (!vga256ScreenInit(pScreen,
		     (pointer) vgaVirtBase,
		     vga256InfoRec.virtualX,
		     vga256InfoRec.virtualY,
		     displayResolution, displayResolution,
		     vga256InfoRec.displayWidth))
          return(FALSE);
  if (vgaBitsPerPixel == 16)
      if (!vga16bppScreenInit(pScreen,
		     vgaLinearBase,
		     vga256InfoRec.virtualX,
		     vga256InfoRec.virtualY,
		     displayResolution, displayResolution,
		     vga256InfoRec.displayWidth))
          return(FALSE);
  if (vgaBitsPerPixel == 32)
      if (!vga32bppScreenInit(pScreen,
		     vgaLinearBase,
		     vga256InfoRec.virtualX,
		     vga256InfoRec.virtualY,
		     displayResolution, displayResolution,
		     vga256InfoRec.displayWidth))
#endif
    return(FALSE);
#else /* XF86VGA16 */
  Init16Output(pScreen,
		     (pointer) vgaVirtBase,
		     vga256InfoRec.virtualX,
		     vga256InfoRec.virtualY,
		     displayResolution, displayResolution,
		     vga256InfoRec.displayWidth);
#endif /* XF86VGA16 */

  pScreen->CloseScreen = vgaCloseScreen;
  pScreen->SaveScreen = vgaSaveScreen;
  pScreen->whitePixel = 1;
  pScreen->blackPixel = 0;
  XF86FLIP_PIXELS();
#ifndef MONOVGA
  /* For 16/32bpp, the cfb defaults are OK. */
  if (vgaBitsPerPixel <= 8) { /* For 8bpp SVGA and VGA16 */
      pScreen->InstallColormap = vgaInstallColormap;
      pScreen->UninstallColormap = vgaUninstallColormap;
      pScreen->ListInstalledColormaps = vgaListInstalledColormaps;
      pScreen->StoreColors = vgaStoreColors;
  }
#endif
  
  if (vgaHWCursor.Initialized)
  {
    xf86PointerScreenFuncs.WarpCursor = vgaHWCursor.Warp;
    pScreen->QueryBestSize = vgaHWCursor.QueryBestSize;
    vgaHWCursor.Init(0, pScreen);
  }
  else
  {
    miDCInitialize (pScreen, &xf86PointerScreenFuncs);
  }

#ifndef XF86VGA16
#ifdef MONOVGA
  if (!mfbCreateDefColormap(pScreen))
#else
  if (!cfbCreateDefColormap(pScreen))
#endif
    return(FALSE);
#else /* XF86VGA16 */
  vga16CreateDefColormap(pScreen);
#endif /* XF86VGA16 */

#ifndef DIRTY_STARTUP
  /* Fill the screen with black */
  if (serverGeneration == 1)
  {
#ifdef PC98_NEC480
	{ int i;
		extern short *vramwindow;

		outb(0xa8, 0xff);
		outb(0xaa, 0x00);
		outb(0xac, 0x00);
		outb(0xae, 0x00);
		for(i=0;i<10;i++) {
			*(vramwindow+2) = i;
			memset(vgaBase, 0xff, 0x8000);
		}
		*(vramwindow+2) = 0;
	}
#else /* PC98_NEC480 */
#if (defined(MONOVGA) && !defined (BANKEDMONOVGA)) /* || defined(XF86VGA16) */
    memset(vgaBase,pScreen->blackPixel,vgaSegmentSize);
#else
#ifdef PC98_EGC
#if 1
    {
      outw(EGC_PLANE, 0);
      outw(EGC_MODE, EGC_COPY_MODE);
      outw(EGC_FGC, pScreen->blackPixel);
      memset(vgaBase, 0xff, 0x8000);
    }
#else
    vgaFillSolid( pScreen->blackPixel, GXcopy, 0x0F /* planes */, 0, 0,
		 pScreen->width, pScreen->height );
#endif
#else
    pointer    vgaVirtPtr;
    pointer    vgaPhysPtr;

#if !defined(BANKEDMONOVGA)
    if (vgaBitsPerPixel > 8)
        /* Currently 16/32bpp uses linear addressing. */
        memset(vgaLinearBase, 0, vga256InfoRec.videoRam * 1024);
    else /* 8bpp: */
#endif

    for (vgaVirtPtr = vgaVirtBase;
#if defined(MONOVGA) || defined(XF86VGA16)
        vgaVirtPtr<(pointer)((char *)vgaVirtBase+(vga256InfoRec.videoRam*256));
#else
        vgaVirtPtr<(pointer)((char *)vgaVirtBase+(vga256InfoRec.videoRam*1024));
#endif
        vgaVirtPtr = (pointer)((char *)vgaVirtPtr + vgaSegmentSize))
        {
#if defined(MONOVGA)
	    if (vgaVirtBase == vgaBase)
            {
                /* Not using banking mode */
                vgaPhysPtr = vgaBase;
            }
            else
#endif
            {
                /* Set the bank, then clear it */
                vgaPhysPtr=vgaSetWrite(vgaVirtPtr);
            }
            memset(vgaPhysPtr,pScreen->blackPixel,vgaSegmentSize);
        }
#endif /* !PC98_EGC */
#endif /* MONOVGA */
#endif /* !PC98_NEC480 */
  }
  vgaSaveScreen(NULL, TRUE); /* unblank the screen */
#endif /* ! DIRTY_STARTUP */

  savepScreen = pScreen;

  return(TRUE);
}

static void saveDummy() {}

/*
 * vgaEnterLeaveVT -- 
 *      grab/ungrab the current VT completely.
 */

void
vgaEnterLeaveVT(enter, screen_idx)
     Bool enter;
     int screen_idx;
{
  BoxRec  pixBox;
  RegionRec pixReg;
  DDXPointRec pixPt;
  PixmapPtr   pspix;
  ScreenPtr   pScreen = savepScreen;

  if (!xf86Resetting && !xf86Exiting)
    {
      pixBox.x1 = 0; pixBox.x2 = pScreen->width;
      pixBox.y1 = 0; pixBox.y2 = pScreen->height;
      pixPt.x = 0; pixPt.y = 0;
      (pScreen->RegionInit)(&pixReg, &pixBox, 1);
#if !defined(MONOVGA) && !defined(XF86VGA16)
      if (vgaBitsPerPixel == 8)
          pspix = (PixmapPtr)pScreen->devPrivate;
      if (vgaBitsPerPixel == 16)
          pspix = (PixmapPtr)pScreen->devPrivates[cfb16ScreenPrivateIndex].ptr;
      if (vgaBitsPerPixel == 32)
          pspix = (PixmapPtr)pScreen->devPrivates[cfb32ScreenPrivateIndex].ptr;
#else
      pspix = (PixmapPtr)pScreen->devPrivate;
#endif
    }

  if (enter)
    {
      vgaInitFunc = saveInitFunc;
      vgaSaveFunc = saveSaveFunc;
      vgaRestoreFunc = saveRestoreFunc;
      vgaAdjustFunc = saveAdjustFunc;
      vgaSaveScreenFunc = saveSaveScreenFunc;
      vgaSetReadFunc = saveSetReadFunc;
      vgaSetWriteFunc = saveSetWriteFunc;
      vgaSetReadWriteFunc = saveSetReadWriteFunc;
      
      xf86MapDisplay(screen_idx, VGA_REGION);
      if (vgaUseLinearAddressing)
        xf86MapDisplay(screen_idx, LINEAR_REGION);

      (vgaEnterLeaveFunc)(ENTER);
#ifdef XFreeXDGA
      if (vga256InfoRec.directMode & XF86DGADirectGraphics) {
	/* Should we do something here or not ? */
      } else
#endif
      {
        vgaOrigVideoState = (pointer)(vgaSaveFunc)(vgaOrigVideoState);
      }
      vgaRestore(vgaNewVideoState);
#ifdef SCO
      /*
       * This is a temporary fix for et4000's, it shouldn't affect the other
       * drivers and the user doesn't notice it so, here it is.  What does it
       * fix, et4000 driver leaves the screen blank when you switch back to it.
       * A mode toggle, even on the same mode, fixes it.
       */
      vgaSwitchMode(vga256InfoRec.modes);
#endif

      /*
       * point pspix back to vgaVirtBase, and copy the dummy buffer to the
       * real screen.
       */
      if (!xf86Resetting)
      {
	ScrnInfoPtr pScr = XF86SCRNINFO(pScreen);

	if (vgaHWCursor.Initialized)
	{
	  vgaHWCursor.Init(0, pScreen);
	  vgaHWCursor.Restore(pScreen);
	  vgaAdjustFunc(pScr->frameX0, pScr->frameY0);
	}

        if ((pointer)pspix->devPrivate.ptr != (pointer)vgaVirtBase && ppix)
        {
#if !defined(MONOVGA) && !defined(XF86VGA16)
          if (vgaBitsPerPixel == 8)
	      pspix->devPrivate.ptr = (pointer)vgaVirtBase;
	  else
	      pspix->devPrivate.ptr = vgaLinearBase;
#else
	  pspix->devPrivate.ptr = (pointer)vgaVirtBase;
#endif
#ifndef XF86VGA16
#ifdef MONOVGA
	  mfbDoBitblt(&ppix->drawable, &pspix->drawable, GXcopy, &pixReg,
                      &pixPt);
#else
          if (vgaBitsPerPixel == 8)
	      vga256DoBitblt(&ppix->drawable, &pspix->drawable, GXcopy,
	          &pixReg, &pixPt, 0xFF);
          if (vgaBitsPerPixel == 16)
	      cfb16DoBitblt(&ppix->drawable, &pspix->drawable, GXcopy,
	          &pixReg, &pixPt, 0xFFFF);
          if (vgaBitsPerPixel == 32)
	      cfb32DoBitblt(&ppix->drawable, &pspix->drawable, GXcopy,
	          &pixReg, &pixPt, 0xFFFFFFFF);
#endif
#else /* XF86VGA16 */
	  vgaRestoreScreenPix(pScreen,ppix);
#endif /* XF86VGA16 */
        }
      }
      if (ppix) {
        (pScreen->DestroyPixmap)(ppix);
        ppix = NULL;
      }
    }
  else
    {

      /* Make sure that another driver hasn't disabeled IO */    
      xf86MapDisplay(screen_idx, VGA_REGION);
      if (vgaUseLinearAddressing)
        xf86MapDisplay(screen_idx, LINEAR_REGION);

      (vgaEnterLeaveFunc)(ENTER);

      /*
       * Create a dummy pixmap to write to while VT is switched out.
       * Copy the screen to that pixmap
       */
      if (!xf86Exiting)
      {
        ppix = (pScreen->CreatePixmap)(pScreen, pScreen->width,
                                        pScreen->height, pScreen->rootDepth);
        if (ppix)
        {
#ifndef XF86VGA16
#ifdef MONOVGA
	  mfbDoBitblt(&pspix->drawable, &ppix->drawable, GXcopy, &pixReg,
                      &pixPt, 0xFF);
#else
          if (vgaBitsPerPixel == 8)
	      vga256DoBitblt(&pspix->drawable, &ppix->drawable, GXcopy,
	          &pixReg, &pixPt, 0xFF);
          if (vgaBitsPerPixel == 16)
	      cfb16DoBitblt(&pspix->drawable, &ppix->drawable, GXcopy,
	          &pixReg, &pixPt, 0xFFFF);
          if (vgaBitsPerPixel == 32)
	      cfb32DoBitblt(&pspix->drawable, &ppix->drawable, GXcopy,
	          &pixReg, &pixPt, 0xFFFFFFFF);
#endif
#else /* XF86VGA16 */
	  vgaSaveScreenPix(pScreen,ppix);
#endif /* XF86VGA16 */
	  pspix->devPrivate.ptr = ppix->devPrivate.ptr;
        }
        (vgaSaveFunc)(vgaNewVideoState);
      }
      /*
       * We come here in many cases, but one is special: When the server aborts
       * abnormaly. Therefore there MUST be a check whether vgaOrigVideoState
       * is valid or not.
       */
#ifdef XFreeXDGA
      if (vga256InfoRec.directMode & XF86DGADirectGraphics) {      
         /* make sure we are in linear mode */
         /* hide any harware cursors */
         (vgaEnterLeaveFunc)(LEAVE);
      } else
#endif
      {
         if (vgaOrigVideoState)
            vgaRestore(vgaOrigVideoState);

            (vgaEnterLeaveFunc)(LEAVE);

            xf86UnMapDisplay(screen_idx, VGA_REGION);
            if (vgaUseLinearAddressing)
            {
               xf86UnMapDisplay(screen_idx, LINEAR_REGION);
            }
      }
  
      saveInitFunc = vgaInitFunc;
      saveSaveFunc = vgaSaveFunc;
      saveRestoreFunc = vgaRestoreFunc;
      saveAdjustFunc = vgaAdjustFunc;
      saveSaveScreenFunc = vgaSaveScreenFunc;
      saveSetReadFunc = vgaSetReadFunc;
      saveSetWriteFunc = vgaSetWriteFunc;
      saveSetReadWriteFunc = vgaSetReadWriteFunc;
      
      vgaInitFunc = (Bool (*)())saveDummy;
      vgaSaveFunc = (void * (*)())saveDummy;
      vgaRestoreFunc = (void (*)())saveDummy;
      vgaAdjustFunc = (void (*)())saveDummy;
      vgaSaveScreenFunc = saveDummy;
      vgaSetReadFunc = saveDummy;
      vgaSetWriteFunc = saveDummy;
#ifdef XFreeXDGA
      if (!(vga256InfoRec.directMode & XF86DGADirectGraphics))
#endif
        vgaSetReadWriteFunc = saveDummy;
      
    }
}

/*
 * vgaCloseScreen --
 *      called to ensure video is enabled when server exits.
 */
Bool
vgaCloseScreen(screen_idx, pScreen)
    int		screen_idx;
    ScreenPtr	pScreen;
{
  /*
   * Hmm... The server may shut down even if it is not running on the
   * current vt. Let's catch this case here.
   */
  xf86Exiting = TRUE;
  if (xf86VTSema)
    vgaEnterLeaveVT(LEAVE, screen_idx);
  else if (ppix) {
    /*
     * 7-Jan-94 CEG: The server is not running on the current vt.
     * Free the screen snapshot taken when the server vt was left.
     */
    (savepScreen->DestroyPixmap)(ppix);
    ppix = NULL;
  }
#ifdef	PC98_EGC
  /* clear screen */
  {
    outw(EGC_PLANE, 0);
    outw(EGC_MODE, EGC_COPY_MODE);
    outw(EGC_FGC, 0);
    memset(vgaBase, 0xff, 0x8000);
  }
  outb(0x6a, 0x07);
  outb(0x6a, 0x04);
  outb(0x6a, 0x06);
/* outb((hiresoflg)?0xa4:0x7c, 0x00); */
  outb(0x7c, 0x00);
#endif
#ifdef PC98_NEC480
	{ int i;
		extern short *vramwindow;

		outb(0xa8, 0xff);
		outb(0xaa, 0x00);
		outb(0xac, 0x00);
		outb(0xae, 0x00);
		for(i=0;i<10;i++) {
			*(vramwindow+2) = i;
			memset(vgaBase, 0xff, 0x8000);
		}
		*(vramwindow+2) = 0;
	}
	outb(0x6a, 0x07);
	outb(0x6a, 0x04);
	outb(0x6a, 0x06);
	outb(0x7c, 0x00);
#endif /* PC98_NEC480 */
  return(TRUE);
}

/*
 * vgaAdjustFrame --
 *      Set a new viewport
 */
void
vgaAdjustFrame(x, y)
     int x, y;
{
  (vgaAdjustFunc)(x, y);
}

/*
 * vgaSwitchMode --
 *     Set a new display mode
 */
Bool
vgaSwitchMode(mode)
     DisplayModePtr mode;
{
  if ((vgaInitFunc)(mode))
  {
    vgaRestore(vgaNewVideoState);
    if (vgaHWCursor.Initialized)
    {
      vgaHWCursor.Restore(savepScreen);
    }
    return(TRUE);
  }
  else
  {
    ErrorF("Mode switch failed because of hardware initialisation error\n");
    return(FALSE);
  }
}

/*
 * vgaValidMode --
 *     Validate a mode for VGA architecture. Also checks the chip driver
 *     to see if the mode can be supported.
 */
Bool
vgaValidMode(mode)
     DisplayModePtr mode;
{
  if ((vgaValidModeFunc)(mode))
  {
    return(TRUE);
  }
  else
  {
    return(FALSE);
  }
}
