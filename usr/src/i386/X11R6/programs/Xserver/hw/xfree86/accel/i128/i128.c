/* $XConsortium: i128.c /main/5 1996/01/21 14:31:43 kaleb $ */
/*
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Robin Cutshaw not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Robin Cutshaw makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ROBIN CUTSHAW DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ROBIN CUTSHAW BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/i128/i128.c,v 3.6 1996/01/20 02:48:15 dawes Exp $ */

#include "i128.h"
#include "i128reg.h"
#include "xf86_HWlib.h"
#include "xf86_PCI.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "Ti302X.h"

extern char *xf86VisualNames[];
extern int defaultColorVisualClass;

static Bool i128ValidMode(
#if NeedFunctionPrototypes
    DisplayModePtr
#endif
); 


ScrnInfoRec i128InfoRec =
{
   FALSE,			/* Bool configured */
   -1,				/* int tmpIndex */
   -1,				/* int scrnIndex */
   i128Probe,			/* Bool (* Probe)() */
   i128Initialize,		/* Bool (* Init)() */
   i128ValidMode,		/* void (* ValidMode)() */
   i128EnterLeaveVT,		/* void (* EnterLeaveVT)() */
   (void (*)())NoopDDA,		/* void (* EnterLeaveMonitor)() */
   (void (*)())NoopDDA,		/* void (* EnterLeaveCursor)() */
   i128AdjustFrame,		/* void (* AdjustFrame)() */
   i128SwitchMode,		/* Bool (* SwitchMode)() */
   i128PrintIdent,		/* void (* PrintIdent)() */
   8,				/* int depth */
   {5, 6, 5},			/* xrgb weight */
   8,				/* int bitsPerPixel */
   PseudoColor,			/* int defaultVisual */
   -1, -1,			/* int virtualX,virtualY */
   -1,				/* int displayWidth */
   -1, -1, -1, -1,		/* int frameX0, frameY0, frameX1, frameY1 */
   {0, },			/* OFlagSet options */
   {0, },			/* OFlagSet clockOptions */   
   {0, },              		/* OFlagSet xconfigFlag */
   NULL,			/* char *chipset */
   NULL,			/* char *ramdac */
   0,				/* int dacSpeed */
   0,				/* int clocks */
   {0, },			/* int clock[MAXCLOCKS] */
   0,				/* int maxClock */
   4096,			/* int videoRam */
   0x0,                         /* int BIOSbase */  
   0,				/* unsigned long MemBase */
   240, 180,			/* int width, height */
   0,				/* unsigned long  speedup */
   NULL,			/* DisplayModePtr modes */   
   NULL,			/* DisplayModePtr pModes */   
   NULL,			/* char           *clockprog */
   -1,			        /* int textclock */
   FALSE,			/* Bool           bankedMono */
   "I128",			/* char           *name */
   {0, },			/* xrgb blackColour */
   {0, },			/* xrgb whiteColour */
   i128ValidTokens,		/* int *validTokens */
   I128_PATCHLEVEL,		/* char *patchlevel */
   0,				/* int IObase */
   0,				/* int PALbase */
   0,				/* int COPbase */
   0,				/* int POSbase */
   0,				/* int instance */
   0,				/* int s3Madjust */
   0,				/* int s3Nadjust */
   0,				/* unsigned long VGABase */
   0,				/* int s3RefClk */
   0,				/* int suspendTime */
   0,				/* int offTime */
   -1,				/* int s3BlankDelay */
#ifdef XFreeXDGA
   0,				/* int directMode */
   NULL,			/* Set Vid Page */
   0,				/* unsigned long physBase */
   0,				/* int physSize */
#endif
};

short i128alu[16] =
{
   MIX_CLEAR,
   MIX_NOR,
   MIX_AND_INVERTED,
   MIX_COPY_INVERTED,
   MIX_AND_REVERSE,
   MIX_INVERT,
   MIX_XOR,
   MIX_NAND,
   MIX_AND,
   MIX_EQUIV,
   MIX_NOOP,
   MIX_OR_INVERTED,
   MIX_COPY,
   MIX_OR_REVERSED,
   MIX_OR,
   MIX_SET
};

static SymTabRec i128DacTable[] = {
   { TI3025_DAC,	"ti3025" },
   { IBM524_DAC,	"ibm524" },
   { IBM528_DAC,	"ibm528" },
   { -1,		"" },
};

extern miPointerScreenFuncRec xf86PointerScreenFuncs;
Bool  (*i128ClockSelectFunc) ();
static Bool ti3025ClockSelect();

ScreenPtr i128savepScreen;
Bool  i128DAC8Bit = FALSE;
Bool  i128DACSyncOnGreen = FALSE;
Bool  i128PowerSaver = FALSE;
int i128DisplayWidth;
int i128Weight;
int i128AdjustCursorXPos = 0;
pointer i128VideoMem = NULL;
struct i128io i128io;
struct i128mem i128mem;
unsigned long i128io_config1_save, i128io_config2_save;
int i128hotX, i128hotY;
Bool i128BlockCursor, i128ReloadCursor;
int i128CursorStartX, i128CursorStartY, i128CursorLines;
int i128RamdacType = UNKNOWN_DAC;

extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed, xf86Verbose;

void (*i128ImageReadFunc)(
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, unsigned long
#endif
);
void (*i128ImageWriteFunc)(
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, short, unsigned long
#endif
);
void (*i128ImageFillFunc)(
#if NeedFunctionPrototypes
    int, int, int, int, char *, int, int, int, int, int, short, unsigned long
#endif
);

/*
 * i128PrintIdent -- print identification message
 */
void
i128PrintIdent()
{
#ifdef I128_ACCEL
  ErrorF("  %s: accelerated server for I128 graphics adaptors (Patchlevel %s)\n",
	 i128InfoRec.name, i128InfoRec.patchLevel);
#else
  ErrorF("  %s: server for I128 graphics adaptors (Patchlevel %s)\n",
	 i128InfoRec.name, i128InfoRec.patchLevel);
#endif
}


/*
 * i128Probe -- find the card on the PCI bus
 */

Bool
i128Probe()
{
   DisplayModePtr pMode, pEnd;
   int i, tx, ty;
   int maxDisplayWidth, maxDisplayHeight;
   unsigned short ioaddr, iobase;
   OFlagSet validOptions;
   unsigned PCI_CtrlIOPorts[] = { 0xCF8, 0xCFA };
   int Num_PCI_CtrlIOPorts = 2;
   unsigned PCI_DevIOPorts[16];
   int Num_PCI_DevIOPorts = 16;
   unsigned I128_IOPorts[] = { 0x0000, 0x0000 };
   int Num_I128_IOPorts = 2;
   unsigned char n, m, p, mdc;
   float mclk;
   struct pci_config_reg *pcrp;

   xf86scanpci();
   i = 0;
   while ((pcrp = pci_devp[i]) != (struct pci_config_reg *)NULL) {
      if ((pcrp->_device_vendor == I128_DEVICE_ID1) ||
          (pcrp->_device_vendor == I128_DEVICE_ID2))
        break;
      i++;
   }

   iobase = (unsigned short )pcrp->_base5 & 0xFF00;

   for (i=0; i<11; i++)  /* 11 long I/O address registers (0x00-0x28) */
      PCI_DevIOPorts[i] = iobase + (i*4);

   xf86AddIOPorts(i128InfoRec.scrnIndex, 11, PCI_DevIOPorts);
   xf86EnableIOPorts(i128InfoRec.scrnIndex);

   i128io.rbase_g = inl(iobase)        & 0xFFFFFF00;
   i128io.rbase_w = inl(iobase + 0x04) & 0xFFFFFF00;
   i128io.rbase_a = inl(iobase + 0x08) & 0xFFFFFF00;
   i128io.rbase_b = inl(iobase + 0x0C) & 0xFFFFFF00;
   i128io.rbase_i = inl(iobase + 0x10) & 0xFFFFFF00;
   i128io.rbase_e = inl(iobase + 0x14) & 0xFFFF8003;
   i128io.id =      inl(iobase + 0x18) & 0x7FFFFFFF;
   i128io.config1 = inl(iobase + 0x1C) & 0xF3333F1F;
   i128io.config2 = inl(iobase + 0x20) & 0xFFF70F03;
   i128io.soft_sw = inl(iobase + 0x28) & 0x0000FFFF;

#ifdef DEBUG
   printf("  PCI Registers\n");
   printf("    MW0_AD    0x%08x  addr 0x%08x  %spre-fetchable\n",
	    pcrp->_base0, pcrp->_base0 & 0xFFC00000,
	    pcrp->_base0 & 0x8 ? "" : "not-");
   printf("    MW1_AD    0x%08x  addr 0x%08x  %spre-fetchable\n",
	    pcrp->_base1, pcrp->_base1 & 0xFFC00000,
	    pcrp->_base1 & 0x8 ? "" : "not-");
   printf("    XYW_AD(A) 0x%08x  addr 0x%08x\n",
	    pcrp->_base2, pcrp->_base2 & 0xFFC00000);
   printf("    XYW_AD(B) 0x%08x  addr 0x%08x\n",
	    pcrp->_base3, pcrp->_base3 & 0xFFC00000);
   printf("    RBASE_G   0x%08x  addr 0x%08x\n",
	    pcrp->_base4, pcrp->_base4 & 0xFFFF0000);
   printf("    IO        0x%08x  addr 0x%08x\n",
	    pcrp->_base5, pcrp->_base5 & 0xFFFFFF00);
   printf("    RBASE_E   0x%08x  addr 0x%08x  %sdecode-enabled\n\n",
	    pcrp->_baserom, pcrp->_baserom & 0xFFFF8000,
	    pcrp->_baserom & 0x1 ? "" : "not-");

   printf("  IO Mapped Registers\n");
   printf("    RBASE_G   0x%08x  addr 0x%08x\n",
	    i128io.rbase_g, i128io.rbase_g & 0xFFFFFF00);
   printf("    RBASE_W   0x%08x  addr 0x%08x\n",
	    i128io.rbase_w, i128io.rbase_w & 0xFFFFFF00);
   printf("    RBASE_A   0x%08x  addr 0x%08x\n",
	    i128io.rbase_a, i128io.rbase_a & 0xFFFFFF00);
   printf("    RBASE_B   0x%08x  addr 0x%08x\n",
	    i128io.rbase_b, i128io.rbase_b & 0xFFFFFF00);
   printf("    RBASE_I   0x%08x  addr 0x%08x\n",
	    i128io.rbase_i, i128io.rbase_i & 0xFFFFFF00);
   printf("    RBASE_E   0x%08x  addr 0x%08x  size 0x%x\n\n",
	    i128io.rbase_e, i128io.rbase_e & 0xFFFF8000, i128io.rbase_e & 0x7);

   printf("  Miscellaneous IO Registers\n");
   printf("    ID        0x%08x\n", i128io.id);
   printf("    CONFIG1   0x%08x\n", i128io.config1);
   printf("    CONFIG2   0x%08x\n", i128io.config2);
   printf("    SOFT_SW   0x%08x\n", i128io.soft_sw);
#endif

   i128io_config1_save = i128io.config1;
   i128io_config2_save = i128io.config2;

   /* enable all of the memory mapped windows */

   i128io.config1 &= 0xF300201D;
   i128io.config1 |= 0x00333F00;
   outl(iobase + 0x1C, i128io.config1);

   i128io.config2 &= 0xFF000000;
   i128io.config2 |= 0x00500000;
   outl(iobase + 0x20, i128io.config2);

   xf86DisableIOPorts(i128InfoRec.scrnIndex);
   xf86ClearIOPortList(i128InfoRec.scrnIndex);

   xf86ProbeFailed = FALSE;

   ErrorF("%s %s: I128 revision (%d)\n", 
	  XCONFIG_PROBED, i128InfoRec.name, i128io.id&0x7);

   OFLG_ZERO(&validOptions);
   OFLG_SET(OPTION_SHOWCACHE, &validOptions);
   OFLG_SET(OPTION_DAC_8_BIT, &validOptions);
   OFLG_SET(OPTION_SYNC_ON_GREEN, &validOptions);
   OFLG_SET(OPTION_POWER_SAVER, &validOptions);
   xf86VerifyOptions(&validOptions, &i128InfoRec);

   if (xf86Verbose)
      ErrorF("%s %s: card type: PCI\n", XCONFIG_PROBED, i128InfoRec.name);

   i128InfoRec.videoRam = 2048;  /* default to 2MB */
   if (i128io.config1 & 0x04)    /* 128 bit mode   */
      i128InfoRec.videoRam <<= 1;
   if (i128io.id & 0x0400)       /* 2 banks VRAM   */
      i128InfoRec.videoRam <<= 1;

   if (xf86Verbose)
      ErrorF("%s %s: videoram:  %dk\n", 
              XCONFIG_GIVEN, i128InfoRec.name, i128InfoRec.videoRam);

   if (xf86bpp < 0)
      xf86bpp = i128InfoRec.depth;

   if (xf86weight.red == 0 || xf86weight.green == 0 || xf86weight.blue == 0)
      xf86weight = i128InfoRec.weight;

   switch (xf86bpp) {
      case 8:
	 i128Weight = RGB8_PSEUDO;
         break;
      case 16:
         if (xf86weight.red==5 && xf86weight.green==5 && xf86weight.blue==5) {
	    i128InfoRec.depth = 15;
	    i128Weight = RGB16_555;
         } else if (xf86weight.red==5 &&
                    xf86weight.green==6 && xf86weight.blue==5) {
	    i128InfoRec.depth = 16;
	    i128Weight = RGB16_565;
         } else {
	    ErrorF(
	     "Invalid color weighting %1d%1d%1d (only 555 and 565 are valid)\n",
	     xf86weight.red,xf86weight.green,xf86weight.blue);
	    return(FALSE);
         }
         i128InfoRec.bitsPerPixel = 16;
         if (i128InfoRec.defaultVisual < 0)
	    i128InfoRec.defaultVisual = TrueColor;
         if (defaultColorVisualClass < 0)
	    defaultColorVisualClass = i128InfoRec.defaultVisual;
         break;
      case 24:
      case 32:
         xf86bpp = 32;
         i128InfoRec.depth = 24;
         i128InfoRec.bitsPerPixel = 32;
	 i128Weight = RGB32_888;
         xf86weight.red =  xf86weight.green = xf86weight.blue = 8;
         if (i128InfoRec.defaultVisual < 0)
	    i128InfoRec.defaultVisual = TrueColor;
         if (defaultColorVisualClass < 0)
	    defaultColorVisualClass = i128InfoRec.defaultVisual;
         break;
      default:
         ErrorF("Invalid value for bpp.  Valid values are 8, 16, 24 and 32.\n");
         return(FALSE);
   }

   if (i128InfoRec.bitsPerPixel > 8 &&
       defaultColorVisualClass >= 0 && defaultColorVisualClass != TrueColor) {
      ErrorF("Invalid default visual type: %d (%s)\n", defaultColorVisualClass,
	     xf86VisualNames[defaultColorVisualClass]);
      return(FALSE);
   }

   maxDisplayWidth = 4096;
   maxDisplayHeight = 2048;

   if (i128InfoRec.virtualX > maxDisplayWidth) {
      ErrorF("%s: Virtual width (%d) is too large.  Maximum is %d\n",
	     i128InfoRec.name, i128InfoRec.virtualX, maxDisplayWidth);
      return (FALSE);
   }
   if (i128InfoRec.virtualY > maxDisplayHeight) {
      ErrorF("%s: Virtual height (%d) is too large.  Maximum is %d\n",
	     i128InfoRec.name, i128InfoRec.virtualY, maxDisplayHeight);
      return (FALSE);
   }
 
   /* Now we can map the rest of the chip into memory */

   i128mem.mw0_ad =  xf86MapVidMem(0, 0, (pointer)(pcrp->_base0 & 0xFFC00000),
                                   i128InfoRec.videoRam * 1024);
   i128VideoMem = (pointer )i128mem.mw0_ad;
#ifdef TOOMANYMMAPS
   i128mem.mw1_ad =  xf86MapVidMem(0, 1, (pointer)(pcrp->_base1 & 0xFFC00000),
                                   i128InfoRec.videoRam * 1024);
#endif
   i128mem.xyw_ada = xf86MapVidMem(0, 2, (pointer)(pcrp->_base2 & 0xFFC00000),
                                   i128InfoRec.videoRam * 1024);
#ifdef TOOMANYMMAPS
   i128mem.xyw_adb = xf86MapVidMem(0, 3, (pointer)(pcrp->_base3 & 0xFFC00000),
                                   i128InfoRec.videoRam * 1024);
#endif
   i128mem.rbase_g = (unsigned long *)xf86MapVidMem(0, 4,
			(pointer)(pcrp->_base4 & 0xFFFF0000), 64 * 1024);
   i128mem.rbase_w = i128mem.rbase_g + ( 8 * 1024)/4;
   i128mem.rbase_a = i128mem.rbase_g + (16 * 1024)/4;
   i128mem.rbase_b = i128mem.rbase_g + (24 * 1024)/4;
   i128mem.rbase_i = i128mem.rbase_g + (32 * 1024)/4;
   i128mem.rbase_g_b = (unsigned char *)i128mem.rbase_g;

   if (pcrp->_device_vendor == I128_DEVICE_ID1) {
      if (i128io.id & 0x0400)       /* 2 banks VRAM   */
	 i128RamdacType = IBM528_DAC;
      else
	 i128RamdacType = TI3025_DAC;
   } if (pcrp->_device_vendor == I128_DEVICE_ID2) {
      if (i128io.config1 & 0x40000000)       /* 2 banks VRAM   */
	 i128RamdacType = IBM524_DAC;
      else
	 i128RamdacType = IBM528_DAC;
   }

   switch(i128RamdacType) {
      case TI3025_DAC:
         /* verify that the ramdac is a TVP3025 */

         i128mem.rbase_g_b[INDEX_TI] = TI_ID;
         if (i128mem.rbase_g_b[DATA_TI] != TI_VIEWPOINT25_ID) {
            ErrorF("%s: Ti3025 Ramdac not found.\n", i128InfoRec.name);
            return(FALSE);
         }
         OFLG_SET(CLOCK_OPTION_TI3025, &i128InfoRec.clockOptions);
         i128InfoRec.ramdac = "ti3025";

         i128mem.rbase_g_b[INDEX_TI] = TI_PLL_CONTROL;
         i128mem.rbase_g_b[DATA_TI] = 0x00;
         i128mem.rbase_g_b[INDEX_TI] = TI_MCLK_PLL_DATA;
         n = i128mem.rbase_g_b[DATA_TI]&0x7f;
         i128mem.rbase_g_b[INDEX_TI] = TI_PLL_CONTROL;
         i128mem.rbase_g_b[DATA_TI] = 0x01;
         i128mem.rbase_g_b[INDEX_TI] = TI_MCLK_PLL_DATA;
         m = i128mem.rbase_g_b[DATA_TI]&0x7f;
         i128mem.rbase_g_b[INDEX_TI] = TI_PLL_CONTROL;
         i128mem.rbase_g_b[DATA_TI] = 0x02;
         i128mem.rbase_g_b[INDEX_TI] = TI_MCLK_PLL_DATA;
         p = i128mem.rbase_g_b[DATA_TI]&0x03;
         i128mem.rbase_g_b[INDEX_TI] = TI_MCLK_DCLK_CONTROL;
         mdc = i128mem.rbase_g_b[DATA_TI];
         if (mdc&0x08)
	    mdc = (mdc&0x07)*2 + 2;
         else
	    mdc = 1;
         mclk = ((1431818 * ((m+2) * 8)) / (n+2) / (1 << p) / mdc + 50) / 100;

         if (xf86Verbose)
            ErrorF("%s %s: Using TI 3025 programmable clock (MCLK %1.3f MHz)\n",
	           XCONFIG_PROBED, i128InfoRec.name, mclk / 1000.0);
	 break;

      case IBM524_DAC:
         i128InfoRec.ramdac = "ibm524";
         OFLG_SET(CLOCK_OPTION_IBMRGB, &i128InfoRec.clockOptions);
         ErrorF("%s: Ramdac %s not supported.\n",
		i128InfoRec.name, i128InfoRec.ramdac);
         return(FALSE);

      case IBM528_DAC:
         i128InfoRec.ramdac = "ibm528";
         OFLG_SET(CLOCK_OPTION_IBMRGB, &i128InfoRec.clockOptions);
         ErrorF("%s: Ramdac %s not supported.\n",
		i128InfoRec.name, i128InfoRec.ramdac);
         return(FALSE);

      default:
         ErrorF("%s: Unknown Ramdac.\n", i128InfoRec.name);
         return(FALSE);
   }

   if (xf86Verbose)
      ErrorF("%s %s: Ramdac type: %s\n",
         XCONFIG_PROBED, i128InfoRec.name, i128InfoRec.ramdac);

   if (i128InfoRec.dacSpeed <= 0) {
      if (i128io.config2&0x80000000)
	 i128InfoRec.dacSpeed = 220000;
      else
	 i128InfoRec.dacSpeed = 175000;
   }
   i128InfoRec.maxClock = i128InfoRec.dacSpeed;
   
   if (xf86Verbose)
      ErrorF("%s %s: Ramdac speed: %d\n",
	     OFLG_ISSET(XCONFIG_DACSPEED, &i128InfoRec.xconfigFlag) ?
	     XCONFIG_GIVEN : XCONFIG_PROBED, i128InfoRec.name,
	     i128InfoRec.dacSpeed / 1000);

   OFLG_SET(CLOCK_OPTION_PROGRAMABLE, &i128InfoRec.clockOptions);


   tx = i128InfoRec.virtualX;
   ty = i128InfoRec.virtualY;
   pMode = i128InfoRec.modes;
   if (pMode == NULL) {
      ErrorF("No modes supplied in XF86Config\n");
      return (FALSE);
   }
   pEnd = NULL;
   do {
      DisplayModePtr pModeSv;

      pModeSv = pMode->next;
      /*
       * xf86LookupMode returns FALSE if it ran into an invalid
       * parameter 
       */
      if (!xf86LookupMode(pMode, &i128InfoRec)) {
	 xf86DeleteMode(&i128InfoRec, pMode);
      } else if (pMode->HDisplay > maxDisplayWidth) {
	 ErrorF("%s %s: Width of mode \"%s\" is too large (max is %d)\n",
		XCONFIG_PROBED, i128InfoRec.name, pMode->name, maxDisplayWidth);
	 xf86DeleteMode(&i128InfoRec, pMode);
      } else if (pMode->VDisplay > maxDisplayHeight) {
	 ErrorF("%s %s: Height of mode \"%s\" is too large (max is %d)\n",
		XCONFIG_PROBED, i128InfoRec.name, pMode->name, maxDisplayHeight);
	 xf86DeleteMode(&i128InfoRec, pMode);
      } else if ((pMode->HDisplay * (1 + pMode->VDisplay) *
                 (i128InfoRec.bitsPerPixel/8)) > i128InfoRec.videoRam * 1024) {
	 ErrorF("%s %s: Too little memory for mode \"%s\"\n", XCONFIG_PROBED,
		i128InfoRec.name, pMode->name);
	 xf86DeleteMode(&i128InfoRec, pMode);
      } else if (((tx > 0) && (pMode->HDisplay > tx)) ||
		 ((ty > 0) && (pMode->VDisplay > ty))) {
	 ErrorF("%s %s: Resolution %dx%d too large for virtual %dx%d\n",
		XCONFIG_PROBED, i128InfoRec.name,
		pMode->HDisplay, pMode->VDisplay, tx, ty);
	 xf86DeleteMode(&i128InfoRec, pMode);
      } else {
	 /*
	  * Successfully looked up this mode.  If pEnd isn't
	  * initialized, set it to this mode.
	  */
	 if (pEnd == (DisplayModePtr) NULL)
	    pEnd = pMode;

	 i128InfoRec.virtualX = max(i128InfoRec.virtualX, pMode->HDisplay);
	 i128InfoRec.virtualY = max(i128InfoRec.virtualY, pMode->VDisplay);
	 pMode->SynthClock = i128InfoRec.clock[pMode->Clock];
      }
      pMode = pModeSv;
   } while (pMode != pEnd);

   if ((tx != i128InfoRec.virtualX) || (ty != i128InfoRec.virtualY))
      OFLG_CLR(XCONFIG_VIRTUAL,&i128InfoRec.xconfigFlag);

   if (i128InfoRec.virtualX <= 640)
      i128DisplayWidth = 640;
   else if (i128InfoRec.virtualX <= 800)
      i128DisplayWidth = 800;
   else if (i128InfoRec.virtualX <= 1024)
      i128DisplayWidth = 1024;
   else if (i128InfoRec.virtualX <= 1152)
      i128DisplayWidth = 1152;
   else if (i128InfoRec.virtualX <= 1280)
      i128DisplayWidth = 1280;
   else if (i128InfoRec.virtualX <= 1600)
      i128DisplayWidth = 1600;
   else if (i128InfoRec.virtualX <= 1920)
      i128DisplayWidth = 1920;
   else
      i128DisplayWidth = 2048;

   if (OFLG_ISSET(OPTION_DAC_8_BIT, &i128InfoRec.options) ||
       (i128InfoRec.bitsPerPixel > 8))
      i128DAC8Bit = TRUE;

   if (i128DAC8Bit && xf86Verbose && i128InfoRec.bitsPerPixel == 8)
      ErrorF("%s %s: Putting RAMDAC into 8-bit mode\n",
         XCONFIG_GIVEN, i128InfoRec.name);

   if (OFLG_ISSET(OPTION_SYNC_ON_GREEN, &i128InfoRec.options)) {
      i128DACSyncOnGreen = TRUE;
      if (xf86Verbose)
	 ErrorF("%s %s: Putting RAMDAC into sync-on-green mode\n",
		   XCONFIG_GIVEN, i128InfoRec.name);
   }

   if (xf86Verbose) {
      if (i128InfoRec.bitsPerPixel == 8)
	 ErrorF("%s %s: Using %d bits per RGB value\n",
		XCONFIG_PROBED, i128InfoRec.name,
		i128DAC8Bit ?  8 : 6);
      else if (i128InfoRec.bitsPerPixel == 16)
	 ErrorF("%s %s: Using 16 bpp.  Color weight: %1d%1d%1d\n",
		XCONFIG_GIVEN, i128InfoRec.name, xf86weight.red,
		xf86weight.green, xf86weight.blue);
      else if (i128InfoRec.bitsPerPixel == 32)
	 ErrorF("%s %s: Using sparse 32 bpp.  Color weight: %1d%1d%1d\n",
		XCONFIG_GIVEN, i128InfoRec.name, xf86weight.red,
		xf86weight.green, xf86weight.blue);
   }

   if (xf86Verbose) {
      ErrorF("%s %s: Virtual resolution set to %dx%d\n", 
             OFLG_ISSET(XCONFIG_VIRTUAL,&i128InfoRec.xconfigFlag) ?
                 XCONFIG_GIVEN : XCONFIG_PROBED,
             i128InfoRec.name,
	     i128InfoRec.virtualX, i128InfoRec.virtualY);
   }

   if (OFLG_ISSET(OPTION_POWER_SAVER, &i128InfoRec.options))
      i128PowerSaver = TRUE;

   return TRUE;  /* End of i128Probe() */
}


Bool
i128ProgramTi3025(freq)
int freq;
{
   unsigned char tmp, misc_ctrl, aux_ctrl, oclk, col_key, mux1_ctrl, mux2_ctrl;
   unsigned char n, m, p;
   int nl, ml, pl;

   if (freq < 20000) {
      ErrorF("%s %s: Specified dot clock (%.3f) too low for TI 3025",
	     XCONFIG_PROBED, i128InfoRec.name, freq / 1000.0);
      return(FALSE);
   }

   Ti3025CalcNMP(freq, &nl, &ml, &pl);
   n = (unsigned char )nl;
   m = (unsigned char )ml;
   p = (unsigned char )pl;

   tmp = i128mem.rbase_g_b[INDEX_TI];

   /*
    * Reset the clock data index
    */
   i128mem.rbase_g_b[INDEX_TI] = TI_PLL_CONTROL;
   i128mem.rbase_g_b[DATA_TI] = 0x00;

   /*
    * Now output the clock frequency
    */
   i128mem.rbase_g_b[INDEX_TI] = TI_PIXEL_CLOCK_PLL_DATA;
   i128mem.rbase_g_b[DATA_TI] = n;
   i128mem.rbase_g_b[DATA_TI] = m;
   i128mem.rbase_g_b[DATA_TI] = p | TI_PLL_ENABLE;

#ifdef NOTYET
   /*
    * Program the MCLK to 57MHz
    */
   i128mem.rbase_g_b[INDEX_TI] = TI_MCLK_PLL_DATA;
   i128mem.rbase_g_b[DATA_TI] = 0x05;
   i128mem.rbase_g_b[DATA_TI] = 0x05;
   i128mem.rbase_g_b[DATA_TI] = 0x05;
#endif

   switch (i128InfoRec.bitsPerPixel) {
	case 8:
		misc_ctrl = (i128DAC8Bit ? TI_MC_8_BPP : 0)
			    | TI_MC_INT_6_8_CONTROL;
		aux_ctrl  = TI_AUX_SELF_CLOCK | TI_AUX_W_CMPL;
   		oclk      = TI_OCLK_S | TI_OCLK_V4 | TI_OCLK_R8;
		col_key   = TI_COLOR_KEY_CMPL;
		break;
	case 16:
		misc_ctrl = 0x00;
		aux_ctrl  = 0x00;
   		oclk      = TI_OCLK_S | TI_OCLK_V4 | TI_OCLK_R4;
		col_key   = 0x00;
		break;
	case 32:
		misc_ctrl = 0x00;
		aux_ctrl  = 0x00;
   		oclk      = TI_OCLK_S | TI_OCLK_V4 | TI_OCLK_R2;
		col_key   = 0x00;
		break;
   }
   switch(i128InfoRec.depth) {
	case 8:
		mux1_ctrl = TI_MUX1_PSEUDO_COLOR;
		mux2_ctrl = TI_MUX2_BUS_PC_D8P64;
		break;
	case 15:
		mux1_ctrl = TI_MUX1_3025D_555;
		mux2_ctrl = TI_MUX2_BUS_DC_D15P64;
		break;
	case 16:
		mux1_ctrl = TI_MUX1_3025D_565;
		mux2_ctrl = TI_MUX2_BUS_DC_D16P64;
		break;
	case 24:
		mux1_ctrl = TI_MUX1_3025D_888;
		mux2_ctrl = TI_MUX2_BUS_DC_D24P64;
		break;
   }

   i128mem.rbase_g_b[INDEX_TI] = TI_CURS_CONTROL;
   i128mem.rbase_g_b[DATA_TI] = TI_CURS_SPRITE_ENABLE | TI_CURS_X_WINDOW_MODE;

   i128mem.rbase_g_b[INDEX_TI] = TI_TRUE_COLOR_CONTROL;
   i128mem.rbase_g_b[DATA_TI] = 0x00;  /* 3025 mode, vga, 8/4bit */

   i128mem.rbase_g_b[INDEX_TI] = TI_VGA_SWITCH_CONTROL;
   i128mem.rbase_g_b[DATA_TI] = 0x00;

   i128mem.rbase_g_b[INDEX_TI] = TI_GENERAL_CONTROL;
   i128mem.rbase_g_b[DATA_TI] = 0x00;

   i128mem.rbase_g_b[INDEX_TI] = TI_MISC_CONTROL;
   i128mem.rbase_g_b[DATA_TI] = misc_ctrl;

   i128mem.rbase_g_b[INDEX_TI] = TI_AUXILIARY_CONTROL;
   i128mem.rbase_g_b[DATA_TI] = aux_ctrl;

   i128mem.rbase_g_b[INDEX_TI] = TI_COLOR_KEY_CONTROL;
   i128mem.rbase_g_b[DATA_TI] = col_key;

   i128mem.rbase_g_b[INDEX_TI] = TI_MUX_CONTROL_1;
   i128mem.rbase_g_b[DATA_TI] = mux1_ctrl;

   i128mem.rbase_g_b[INDEX_TI] = TI_MUX_CONTROL_2;
   i128mem.rbase_g_b[DATA_TI] = mux2_ctrl;

   i128mem.rbase_g_b[INDEX_TI] = TI_INPUT_CLOCK_SELECT;
   i128mem.rbase_g_b[DATA_TI] = TI_ICLK_PLL;

   i128mem.rbase_g_b[INDEX_TI] = TI_OUTPUT_CLOCK_SELECT;
   i128mem.rbase_g_b[DATA_TI] = oclk;

   usleep(150000);
   return(TRUE);
}


/*
 * i128ValidMode --
 *
 */
static Bool
i128ValidMode(mode)
DisplayModePtr mode;
{
return TRUE;
}
