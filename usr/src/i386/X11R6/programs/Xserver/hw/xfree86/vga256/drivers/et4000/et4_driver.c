/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/et4000/et4_driver.c,v 3.23 1996/01/12 14:38:12 dawes Exp $
 *
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
 */
/* $XConsortium: et4_driver.c /main/16 1996/01/12 12:17:07 kaleb $ */

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

#ifdef W32_ACCEL_SUPPORT
#include "w32.h"
#endif

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

#ifndef MONOVGA
#include "vga256.h"
#endif

#ifdef W32_ACCEL_SUPPORT
typedef struct {
  unsigned char cmd_reg;
  unsigned char PLL_f2_M;
  unsigned char PLL_f2_N;
  unsigned char PLL_ctrl;
  unsigned char PLL_w_idx;
  unsigned char PLL_r_idx;
  } ICS5341state;
#endif

typedef struct {
  vgaHWRec std;               /* good old IBM VGA */
  unsigned char ExtStart;     /* Tseng ET4000 specials   CRTC 0x33/0x34/0x35 */
  unsigned char Compatibility;
  unsigned char OverflowHigh;
  unsigned char StateControl;    /* TS 6 & 7 */
  unsigned char AuxillaryMode;
  unsigned char Misc;           /* ATC 0x16 */
  unsigned char SegSel;
#ifdef W32_SUPPORT
  unsigned char VSConf2;        /* CRTC 0x37 */
#endif
#ifdef W32_ACCEL_SUPPORT
  ICS5341state gendac;
#endif
#ifndef MONOVGA
  unsigned char RCConf;       /* CRTC 0x32 */
#endif
  } vgaET4000Rec, *vgaET4000Ptr;


#define TYPE_ET4000		0
#define TYPE_ET4000W32		1
#define TYPE_ET4000W32I		2
#define TYPE_ET4000W32P		3
#define TYPE_ET4000W32Pc	4

static Bool     ET4000Probe();
static char *   ET4000Ident();
static Bool     ET4000ClockSelect();
static Bool     LegendClockSelect();
#ifdef W32_ACCEL_SUPPORT
static Bool     ICS5341ClockSelect();
static Bool     STG1703ClockSelect();
#endif
static void     ET4000EnterLeave();
static Bool     ET4000Init();
static Bool     ET4000ValidMode();
static void *   ET4000Save();
static void     ET4000Restore();
static void     ET4000Adjust();
static void     ET4000FbInit();
extern void     ET4000SetRead();
extern void     ET4000SetWrite();
extern void     ET4000SetReadWrite();
extern void     ET4000W32SetRead();
extern void     ET4000W32SetWrite();
extern void     ET4000W32SetReadWrite();

static unsigned char 	save_divide = 0;
#ifndef MONOVGA
static unsigned char    initialRCConf = 0x70;
#ifdef W32_SUPPORT
/* these should be taken from the "Saved" register set instead of this way */
static unsigned char    initialCompatibility = 0x18;
static unsigned char    initialVSConf2 = 0x0b;
#endif
#endif
static int et4000_type;

vgaVideoChipRec ET4000 = {
  ET4000Probe,
  ET4000Ident,
  ET4000EnterLeave,
  ET4000Init,
  ET4000ValidMode,
  ET4000Save,
  ET4000Restore,
  ET4000Adjust,
  vgaHWSaveScreen,
  (void (*)())NoopDDA,
  ET4000FbInit,
  ET4000SetRead,
  ET4000SetWrite,
  ET4000SetReadWrite,
  0x10000,
  0x10000,
  16,
  0xFFFF,
  0x00000, 0x10000,
  0x00000, 0x10000,
  TRUE,                                 /* Uses 2 banks */
  VGA_NO_DIVIDE_VERT,
  {0,},
  8,
  FALSE,
  0,
  0,
  FALSE,
  FALSE,
  NULL,
  1,
};

#define new ((vgaET4000Ptr)vgaNewVideoState)

static SymTabRec chipsets[] = {
  { TYPE_ET4000,	"et4000" },
#ifdef W32_SUPPORT
  { TYPE_ET4000W32,	"et4000w32" },
  { TYPE_ET4000W32I,	"et4000w32i" },
  { TYPE_ET4000W32P,	"et4000w32p" },
#endif
  { -1,			"" },
};

Bool (*ClockSelect)();

static unsigned ET4000_ExtPorts[] = {0x3B8, 0x3BF, 0x3CD, 0x3D8,
#ifdef W32_SUPPORT
	0x217a, 0x217b,		/* These last two are W32 specific */
#endif
};

static int Num_ET4000_ExtPorts = 
	(sizeof(ET4000_ExtPorts)/sizeof(ET4000_ExtPorts[0]));

/*
 * ET4000Ident
 */

static char *
ET4000Ident(n)
     int n;
{
  if (chipsets[n].token < 0)
    return(NULL);
  else
    return(chipsets[n].name);
}


/*
 * ET4000ClockSelect --
 *      select one of the possible clocks ...
 */

static Bool
ET4000ClockSelect(no)
     int no;
{
  static unsigned char save1, save2, save3, save4;
  unsigned char temp;

  switch(no)
  {
    case CLK_REG_SAVE:
      save1 = inb(0x3CC);
      outb(vgaIOBase + 4, 0x34); save2 = inb(vgaIOBase + 5);
      outb(0x3C4, 7); save3 = inb(0x3C5);
      if( et4000_type > TYPE_ET4000 )
      {
         outb(vgaIOBase + 4, 0x31); save4 = inb(vgaIOBase + 5);
      }
      break;
    case CLK_REG_RESTORE:
      outb(0x3C2, save1);
      outw(vgaIOBase + 4, 0x34 | (save2 << 8));
      outw(0x3C4, 7 | (save3 << 8));
      if( et4000_type > TYPE_ET4000 )
      {
         outw(vgaIOBase + 4, 0x31 | (save4 << 8));
      }
      break;
    default:
      temp = inb(0x3CC);
      outb(0x3C2, ( temp & 0xf3) | ((no << 2) & 0x0C));
      outb(vgaIOBase + 4, 0x34);	/* don't nuke the other bits in CR34 */
      temp = inb(vgaIOBase + 5);
      outw(vgaIOBase + 4, 0x34 | ((temp & 0xFD) << 8) | ((no & 0x04) << 7));

#ifndef OLD_CLOCK_SCHEME
      {
         outb(vgaIOBase + 4, 0x31);
         temp = inb(vgaIOBase + 5);
         outb(vgaIOBase + 5, (temp & 0x3f) | ((no & 0x10) << 2));
         outb(0x3C4, 7); temp = inb(0x3C5);
         outb(0x3C5, (save_divide ^ ((no & 0x8) << 3)) | (temp & 0xBF));
      }
#else
      {
         outb(0x3C4, 7); temp = inb(0x3C5);
         outb(0x3C5, (save_divide ^ ((no & 0x10) << 2)) | (temp & 0xBF));
      }
#endif
  }
  return(TRUE);
}

/*
 * LegendClockSelect --
 *      select one of the possible clocks ...
 */

static Bool
LegendClockSelect(no)
     int no;
{
  /*
   * Sigma Legend special handling
   *
   * The Legend uses an ICS 1394-046 clock generator.  This can generate 32
   * different frequencies.  The Legend can use all 32.  Here's how:
   *
   * There are two flip/flops used to latch two inputs into the ICS clock
   * generator.  The five inputs to the ICS are then
   *
   * ICS     ET-4000
   * ---     ---
   * FS0     CS0
   * FS1     CS1
   * FS2     ff0     flip/flop 0 output
   * FS3     CS2
   * FS4     ff1     flip/flop 1 output
   *
   * The flip/flops are loaded from CS0 and CS1.  The flip/flops are
   * latched by CS2, on the rising edge. After CS2 is set low, and then high,
   * it is then set to its final value.
   *
   */
  static unsigned char save1, save2;
  unsigned char temp;

  switch(no)
  {
    case CLK_REG_SAVE:
      save1 = inb(0x3CC);
      outb(vgaIOBase + 4, 0x34); save2 = inb(vgaIOBase + 5);
      break;
    case CLK_REG_RESTORE:
      outb(0x3C2, save1);
      outw(vgaIOBase + 4, 0x34 | (save2 << 8));
      break;
    default:
      temp = inb(0x3CC);
      outb(0x3C2, (temp & 0xF3) | ((no & 0x10) >> 1) | (no & 0x04));
      outw(vgaIOBase + 4, 0x0034);
      outw(vgaIOBase + 4, 0x0234);
      outw(vgaIOBase + 4, ((no & 0x08) << 6) | 0x34);
      outb(0x3C2, (temp & 0xF3) | ((no << 2) & 0x0C));
  }
  return(TRUE);
}

#ifdef W32_ACCEL_SUPPORT
/*
 * ICS5341ClockSelect --
 *      programmable clock chip
 */

static Bool
ICS5341ClockSelect(freq)
     int freq;
{
   Bool result = TRUE;

   switch(freq)
   {
   case CLK_REG_SAVE:
   case CLK_REG_RESTORE:
      result = ET4000ClockSelect(freq);
      break;
   default:
      {
        /*
	 * right now this is never called
	 * the code programs the clocks directly :-(
	 */
        ET4000gendacSetClock(freq, 2); /* can't fail */
        result = ET4000ClockSelect(2);
        usleep(150000);
      }
   }
   return(result);
}
/*
 * STG1703ClockSelect --
 *      programmable clock chip
 */

static Bool
STG1703ClockSelect(freq)
     int freq;
{
   Bool result = TRUE;

   switch(freq)
   {
   case CLK_REG_SAVE:
   case CLK_REG_RESTORE:
      result = ET4000ClockSelect(freq);
      break;
   default:
      {
        /*
	 * right now this is never called
	 * the code programs the clocks directly :-(
	 */
        ET4000stg1703SetClock(freq, 2); /* can't fail */
        result = ET4000ClockSelect(2);
        usleep(150000);
      }
   }
   return(result);
}
#endif


/*
 * ET4000Probe --
 *      check up whether a Et4000 based board is installed
 */

static Bool
ET4000Probe()
{
  int numClocks, i;

  /*
   * Set up I/O ports to be used by this card
   */
  xf86ClearIOPortList(vga256InfoRec.scrnIndex);
  xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
  xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_ET4000_ExtPorts, ET4000_ExtPorts);

  if (vga256InfoRec.chipset)
    {
      et4000_type = xf86StringToToken(chipsets, vga256InfoRec.chipset);
      if (et4000_type < 0)
          return FALSE;
      ET4000EnterLeave(ENTER);
    }
  else
    {
      unsigned char temp, origVal, newVal;

      ET4000EnterLeave(ENTER);
      /*
       * Check first that there is a ATC[16] register and then look at
       * CRTC[33]. If both are R/W correctly it's a ET4000 !
       */
      temp = inb(vgaIOBase+0x0A); 
      outb(0x3C0, 0x16 | 0x20); origVal = inb(0x3C1);
      outb(0x3C0, origVal ^ 0x10);
      outb(0x3C0, 0x16 | 0x20); newVal = inb(0x3C1);
      outb(0x3C0, origVal);
      if (newVal != (origVal ^ 0x10))
	{
	  ET4000EnterLeave(LEAVE);
	  return(FALSE);
	}

      outb(vgaIOBase+0x04, 0x33);          origVal = inb(vgaIOBase+0x05);
      outb(vgaIOBase+0x05, origVal ^ 0x0F); newVal = inb(vgaIOBase+0x05);
      outb(vgaIOBase+0x05, origVal);
      if (newVal != (origVal ^ 0x0F))
	{
	  ET4000EnterLeave(LEAVE);
	  return(FALSE);
	}

      et4000_type = TYPE_ET4000;

#ifdef W32_SUPPORT
      /*
       * Now check for an ET4000/W32.
       * Test for writability of 0x3cb.
       */
      origVal = inb(0x3cb);
      outb(0x3cb, 0x33);	/* Arbitrary value */
      newVal = inb(0x3cb);
      outb(0x3cb, origVal);
      if (newVal == 0x33)
        {
          /* We have an ET4000/W32. Now determine the type. */
          outb(0x217a, 0xec);
          temp = inb(0x217b) >> 4;
          switch (temp) {
          case 0 : /* ET4000/W32 */
              et4000_type = TYPE_ET4000W32;
              break;
          case 1 : /* ET4000/W32i */
          case 3 : /* ET4000/W32i rev b */ 
          case 11: /* ET4000/W32i rev c */
              et4000_type = TYPE_ET4000W32I;
              break;
          case 2 : /* ET4000/W32p rev a */
          case 5 : /* ET4000/W32p rev b */
              et4000_type = TYPE_ET4000W32P;
          case 6 : /* ET4000/W32p rev d */
          case 7 : /* ET4000/W32p rev c */
              et4000_type = TYPE_ET4000W32Pc;
              break;
          default :
              ErrorF("%s %s: ET4000W32: Unknown type. Try chipset override.\n",
                  XCONFIG_PROBED, vga256InfoRec.name);
	      ET4000EnterLeave(LEAVE);
	      return(FALSE);
	  }
        }
#endif

    }

  /*
   * Detect how much memory is installed
   */
  if (!vga256InfoRec.videoRam)
    {
      unsigned char config;
      
      outb(vgaIOBase+0x04, 0x37); config = inb(vgaIOBase+0x05);
      
      switch(config & 0x03) {
      case 1: vga256InfoRec.videoRam = 256; break;
      case 2: vga256InfoRec.videoRam = 512; break;
      case 3: vga256InfoRec.videoRam = 1024; break;
      }

      if (config & 0x80) vga256InfoRec.videoRam <<= 1;

#ifdef W32_SUPPORT
      /* Check for interleaving on W32i/p. */
      if (et4000_type >= TYPE_ET4000W32I) {
          outb(vgaIOBase+0x04, 0x32);
          config = inb(vgaIOBase+0x05);
          if (config & 0x80) vga256InfoRec.videoRam <<= 1;
      }
#endif
    }

#ifdef W32_SUPPORT
  /*
   * If more than 1MB of RAM is available on the W32i/p, use the
   * W32-specific banking function that can address 4MB.
   */ 
  if (et4000_type > TYPE_ET4000 && vga256InfoRec.videoRam > 1024) {
      ET4000.ChipSetRead = ET4000W32SetRead;
      ET4000.ChipSetWrite= ET4000W32SetWrite;
      ET4000.ChipSetReadWrite = ET4000W32SetReadWrite;
  }
#endif

  /* Initialize option flags allowed for this driver */
  OFLG_SET(OPTION_LEGEND, &ET4000.ChipOptionFlags);
  OFLG_SET(OPTION_HIBIT_HIGH, &ET4000.ChipOptionFlags);
  OFLG_SET(OPTION_HIBIT_LOW, &ET4000.ChipOptionFlags);
#ifndef MONOVGA
  OFLG_SET(OPTION_PCI_BURST_ON, &ET4000.ChipOptionFlags);
  OFLG_SET(OPTION_PCI_BURST_OFF, &ET4000.ChipOptionFlags);
  OFLG_SET(OPTION_W32_INTERLEAVE_ON, &ET4000.ChipOptionFlags);
  OFLG_SET(OPTION_W32_INTERLEAVE_OFF, &ET4000.ChipOptionFlags);
  OFLG_SET(OPTION_FAST_DRAM, &ET4000.ChipOptionFlags);
#endif

#ifdef W32_ACCEL_SUPPORT
  if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions))
  {
    if (OFLG_ISSET(CLOCK_OPTION_ICS5341, &vga256InfoRec.clockOptions))
    {
      ClockSelect = ICS5341ClockSelect;
      numClocks = 3;
    }
    else if (OFLG_ISSET(CLOCK_OPTION_STG1703, &vga256InfoRec.clockOptions))
    {
      ClockSelect = STG1703ClockSelect;
      numClocks = 3;
    }
    else
    {
      ErrorF("Unsuported programmable Clock chip.\n");
      ET4000EnterLeave(LEAVE);
      return(FALSE);
    }
  }
  else
#endif
  {
    if (OFLG_ISSET(OPTION_LEGEND, &vga256InfoRec.options))
      {
        ClockSelect = LegendClockSelect;
        numClocks   = 32;
      }
    else
      {
        ClockSelect = ET4000ClockSelect;
        if( et4000_type > TYPE_ET4000 )
           numClocks = 32;
	else
           numClocks = 16;
      }
  }   
  
  if (OFLG_ISSET(OPTION_HIBIT_HIGH, &vga256InfoRec.options))
    {
      if (OFLG_ISSET(OPTION_HIBIT_LOW, &vga256InfoRec.options))
        {
          ET4000EnterLeave(LEAVE);
          FatalError(
             "\nOptions \"hibit_high\" and \"hibit_low\" are incompatible\n");
        }
      save_divide = 0x40;
    }
  else if (OFLG_ISSET(OPTION_HIBIT_LOW, &vga256InfoRec.options))
    save_divide = 0;
  else
    {
      /* Check for initial state of divide flag */
      outb(0x3C4, 7);
      save_divide = inb(0x3C5) & 0x40;
      ErrorF("%s %s: ET4000: Initial hibit state: %s\n", XCONFIG_PROBED,
             vga256InfoRec.name, save_divide & 0x40 ? "high" : "low");
    }

#ifndef MONOVGA
    /* Save initial RCConf value */
    outb(vgaIOBase + 4, 0x32); initialRCConf = inb(vgaIOBase + 5);
#ifdef W32_SUPPORT
    if (et4000_type > TYPE_ET4000) {
      /* Save initial Auxctrl (CRTC 0x34) value */
      outb(vgaIOBase + 4, 0x34); initialCompatibility = inb(vgaIOBase + 5);
      /* Save initial VSConf2 (CRTC 0x37) value */
      outb(vgaIOBase + 4, 0x37); initialVSConf2 = inb(vgaIOBase + 5);
    }
#endif
#endif

  if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions)) {
    if (!vga256InfoRec.clocks) vgaGetClocks(numClocks, ClockSelect);
  }
  
  vga256InfoRec.chipset = xf86TokenToString(chipsets, et4000_type);
  vga256InfoRec.bankedMono = TRUE;
#ifndef MONOVGA
#ifdef XFreeXDGA
  vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
#endif

  return(TRUE);
}


/*
 * ET4000FbInit --
 *	initialise the cfb SpeedUp functions
 */

static void
ET4000FbInit()
{
#ifndef MONOVGA
  int useSpeedUp;

  if (vga256InfoRec.videoRam > 1024)
    useSpeedUp = vga256InfoRec.speedup & SPEEDUP_ANYCHIPSET;
  else
    useSpeedUp = vga256InfoRec.speedup & SPEEDUP_ANYWIDTH;
  if (useSpeedUp && xf86Verbose)
  {
    ErrorF("%s %s: ET4000: SpeedUps selected (Flags=0x%x)\n", 
	   OFLG_ISSET(XCONFIG_SPEEDUP,&vga256InfoRec.xconfigFlag) ?
           XCONFIG_GIVEN : XCONFIG_PROBED,
           vga256InfoRec.name, useSpeedUp);
  }
  if (useSpeedUp & SPEEDUP_FILLRECT)
  {
    vga256LowlevFuncs.fillRectSolidCopy = speedupvga256FillRectSolidCopy;
  }
  if (useSpeedUp & SPEEDUP_BITBLT)
  {
    vga256LowlevFuncs.doBitbltCopy = speedupvga256DoBitbltCopy;
  }
  if (useSpeedUp & SPEEDUP_LINE)
  {
    vga256LowlevFuncs.lineSS = speedupvga256LineSS;
    vga256LowlevFuncs.segmentSS = speedupvga256SegmentSS;
    vga256TEOps1Rect.Polylines = speedupvga256LineSS;
    vga256TEOps1Rect.PolySegment = speedupvga256SegmentSS;
    vga256TEOps.Polylines = speedupvga256LineSS;
    vga256TEOps.PolySegment = speedupvga256SegmentSS;
    vga256NonTEOps1Rect.Polylines = speedupvga256LineSS;
    vga256NonTEOps1Rect.PolySegment = speedupvga256SegmentSS;
    vga256NonTEOps.Polylines = speedupvga256LineSS;
    vga256NonTEOps.PolySegment = speedupvga256SegmentSS;
  }
  if (useSpeedUp & SPEEDUP_FILLBOX)
  {
    vga256LowlevFuncs.fillBoxSolid = speedupvga256FillBoxSolid;
  }
#endif /* MONOVGA */
}


/*
 * ET4000EnterLeave --
 *      enable/disable io-mapping
 */

static void 
ET4000EnterLeave(enter)
     Bool enter;
{
  unsigned char temp;

#ifndef MONOVGA
#ifdef XFreeXDGA
  if (vga256InfoRec.directMode&XF86DGADirectGraphics && !enter)
    return;
#endif
#endif

  if (enter)
    {
      xf86EnableIOPorts(vga256InfoRec.scrnIndex);

      vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
      outb(0x3BF, 0x03);                           /* unlock ET4000 special */
      outb(vgaIOBase + 8, 0xA0);
      outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
      outb(vgaIOBase + 5, temp & 0x7F);
    }
  else
    {
      outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
      outb(vgaIOBase + 5, temp | 0x80);
      outb(vgaIOBase + 8, 0x00);
      outb(0x3D8, 0x29);
      outb(0x3BF, 0x01);                           /* relock ET4000 special */

      xf86DisableIOPorts(vga256InfoRec.scrnIndex);
    }
}





/*
 * ET4000Restore --
 *      restore a video mode
 */

static void 
ET4000Restore(restore)
  vgaET4000Ptr restore;
{
  unsigned char i;
  unsigned char m,n;
  unsigned char pllctr;

  outb(0x3CD, 0x00); /* segment select */

#ifdef W32_ACCEL_SUPPORT
  if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions))
  {
    if (OFLG_ISSET(CLOCK_OPTION_ICS5341, &vga256InfoRec.clockOptions))
    {
       /* Restore ICS 5341 GenDAC Command and PLL registers */
       outb(vgaIOBase + 4, 0x31);
       i = inb(vgaIOBase + 5);
       outb(vgaIOBase + 4, 0x31);
       outb(vgaIOBase + 5, i | 0x40);

#if EXTENDED_DEBUG
       outb(0x3c7,2);				/* clock f2 */
       m = inb(0x3c9);
       n = inb(0x3c9);
       ErrorF("--------  Old Clock f2: %9d\n",
              14318 * (m + 2) / ((n & 0x1f)+2) / (1 << ((n & 0x60) >> 5)) );
       outb(0x3c7,0x0e);			/* pll ctrl */
       pllctr = inb(0x3c9);
       ErrorF("--------  Old PLL Ctrl: 0x%2x\n",pllctr);
       pllctr = inb(0x3c6);
       ErrorF("--------  Old Cmd Reg: 0x%2x\n",pllctr);
#endif
       outb(0x3c6, restore->gendac.cmd_reg);      /* Enhanced command register*/
       outb(0x3c8, 2);                            /* index to f2 reg */
       outb(0x3c9, restore->gendac.PLL_f2_M);     /* f2 PLL M divider */
       outb(0x3c9, restore->gendac.PLL_f2_N);     /* f2 PLL N1/N2 divider */

       outb(0x3c8, 0x0e);                         /* index to PLL control */
       outb(0x3c9, restore->gendac.PLL_ctrl);     /* PLL control */
       outb(0x3c8, restore->gendac.PLL_w_idx);    /* PLL write index */
       outb(0x3c7, restore->gendac.PLL_r_idx);    /* PLL read index */

#if EXTENDED_DEBUG
       outb(0x3c7,2);				/* clock f2 */
       m = inb(0x3c9);
       n = inb(0x3c9);
       ErrorF("--------  New Clock f2: %9d\n",
              14318 * (m + 2) / ((n & 0x1f)+2) / (1 << ((n & 0x60) >> 5)) );
       outb(0x3c7,0x0e);			/* pll ctrl */
       pllctr = inb(0x3c9);
       ErrorF("--------  New PLL Ctrl: 0x%2x\n",pllctr);
       pllctr = inb(0x3c6);
       ErrorF("--------  New Cmd Reg: 0x%2x\n\n",pllctr);
       outb(0x3c8, restore->gendac.PLL_w_idx);    /* PLL write index */
       outb(0x3c7, restore->gendac.PLL_r_idx);    /* PLL read index */
#endif
       outb(vgaIOBase + 4, 0x31);
       outb(vgaIOBase + 5, i & ~0x40);
    }
    if (OFLG_ISSET(CLOCK_OPTION_STG1703, &vga256InfoRec.clockOptions))
    {
       /* Restore STG 1703 GenDAC Command and PLL registers 
        * we share one data structure with the gendac code, so the names
	* are not too good.
	*/

#if EXTENDED_DEBUG
       m = STG1703getIndex(0x24);
       n = inb(0x3c6);				/* use autoincrement */
       ErrorF("--------  Old Clock f2: %9d\n",
              14318 * ((m&0x7f) + 2) / ((n&0x1f)+2) / (1 << ((n&0x60) >> 5)) );
       pllctr = STG1703getIndex(3);
       ErrorF("--------  Old Pixel Mode Select: 0x%2x\n",pllctr);
       STG1703magic(0);
       xf86dactopel();
       pllctr = xf86getdaccomm();
       ErrorF("--------  Old Cmd Reg: 0x%2x\n",pllctr);
#endif
       STG1703setIndex(0x24,restore->gendac.PLL_f2_M);
       outb(0x3c6,restore->gendac.PLL_f2_N);      /* use autoincrement */
       STG1703setIndex(0x03,restore->gendac.PLL_ctrl);/* write same value to */
       outb(0x3c6,restore->gendac.PLL_ctrl);	  /* primary and secondary 
       						   * pixel mode select register 
						   */
       STG1703magic(0);
       xf86dactopel();
       xf86setdaccomm(restore->gendac.cmd_reg);   /* write enh command reg */

#if EXTENDED_DEBUG
       m = STG1703getIndex(0x24);
       n = inb(0x3c6);				/* use autoincrement */
       ErrorF("--------  New Clock f2: %9d\n",
              14318 * ((m&0x7f) + 2) / ((n&0x1f)+2) / (1 << ((n&0x60) >> 5)) );
       pllctr = STG1703getIndex(3);
       ErrorF("--------  New Pixel Mode Select: 0x%2x\n",pllctr);
       STG1703magic(0);
       xf86dactopel();
       pllctr = xf86getdaccomm();
       ErrorF("--------  New Cmd Reg: 0x%2x\n\n",pllctr);
#endif
    }
  }
#endif
 
  vgaHWRestore((vgaHWPtr)restore);

  outw(0x3C4, (restore->StateControl << 8)  | 0x06);
  outw(0x3C4, (restore->AuxillaryMode << 8) | 0x07);
  i = inb(vgaIOBase + 0x0A); /* reset flip-flop */
  outb(0x3C0, 0x36); outb(0x3C0, restore->Misc);
  outw(vgaIOBase + 4, (restore->ExtStart << 8)      | 0x33);
  if (restore->std.NoClock >= 0)
    outw(vgaIOBase + 4, (restore->Compatibility << 8) | 0x34);
  outw(vgaIOBase + 4, (restore->OverflowHigh << 8)  | 0x35);
#ifdef W32_SUPPORT  
  if (et4000_type > TYPE_ET4000)
    outw(vgaIOBase + 4, (restore->VSConf2 << 8)  | 0x37);
#endif
#ifndef MONOVGA
#ifdef WHY_WOULD_YOU_RESTRICT_THAT_TO_THIS_OPTION
  if (OFLG_ISSET(OPTION_FAST_DRAM, &vga256InfoRec.options))
#endif
    outw(vgaIOBase + 4, (restore->RCConf << 8)  | 0x32);
#endif
  outb(0x3CD, restore->SegSel);

  /*
   * This might be required for the Legend clock setting method, but
   * should not be used for the "normal" case because the high order
   * bits are not set in NoClock when returning to text mode.
   */
  if (OFLG_ISSET(OPTION_LEGEND, &vga256InfoRec.options))
    if (restore->std.NoClock >= 0)
      {
	vgaProtect(TRUE);
        (ClockSelect)(restore->std.NoClock);
	vgaProtect(FALSE);
      }
}



/*
 * ET4000Save --
 *      save the current video mode
 */

static void *
ET4000Save(save)
     vgaET4000Ptr save;
{
  unsigned char             i;
  unsigned char             temp1, temp2;

  /*
   * we need this here , cause we MUST disable the ROM SYNC feature
   * this bit changed with W32p_rev_c...
   */
     outb(vgaIOBase + 4, 0x34); temp1 = inb(vgaIOBase + 5);
  if( et4000_type < TYPE_ET4000W32Pc) {
     outb(vgaIOBase + 5, temp1 & 0x1F);
  }
  temp2 = inb(0x3CD); outb(0x3CD, 0x00); /* segment select */

  save = (vgaET4000Ptr)vgaHWSave((vgaHWPtr)save, sizeof(vgaET4000Rec));
  save->Compatibility = temp1;
  save->SegSel = temp2;

  outb(vgaIOBase + 4, 0x33); save->ExtStart     = inb(vgaIOBase + 5);
  outb(vgaIOBase + 4, 0x35); save->OverflowHigh = inb(vgaIOBase + 5);
#ifdef W32_SUPPORT  
  if (et4000_type > TYPE_ET4000)
    outb(vgaIOBase + 4, 0x37); save->VSConf2 = inb(vgaIOBase + 5);
#endif
#ifndef MONOVGA
#ifdef WHY_WOULD_YOU_RESTRICT_THAT_TO_THIS_OPTION
  if (OFLG_ISSET(OPTION_FAST_DRAM, &vga256InfoRec.options))
#endif
    outb(vgaIOBase + 4, 0x32); save->RCConf = inb(vgaIOBase + 5);
#endif
  outb(0x3C4, 6); save->StateControl  = inb(0x3C5);
  outb(0x3C4, 7); save->AuxillaryMode = inb(0x3C5);
  save->AuxillaryMode |= 0x14;
  i = inb(vgaIOBase + 0x0A); /* reset flip-flop */
  outb(0x3C0,0x36); save->Misc = inb(0x3C1); outb(0x3C0, save->Misc);
#ifdef W32_ACCEL_SUPPORT
  if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions))
  {
    if (OFLG_ISSET(CLOCK_OPTION_ICS5341, &vga256InfoRec.clockOptions))
    {
      /* Restore ICS 5341 GenDAC Command and PLL registers */
      outb(vgaIOBase + 4, 0x31);
      i = inb(vgaIOBase + 5);
      outb(vgaIOBase + 5, i | 0x40);

      save->gendac.cmd_reg = inb(0x3c6);      /* Enhanced command register */
      save->gendac.PLL_w_idx = inb(0x3c8);    /* PLL write index */
      save->gendac.PLL_r_idx = inb(0x3c7);    /* PLL read index */
      outb(0x3c7, 2);                         /* index to f2 reg */
      save->gendac.PLL_f2_M = inb(0x3c9);     /* f2 PLL M divider */
      save->gendac.PLL_f2_N = inb(0x3c9);     /* f2 PLL N1/N2 divider */
      outb(0x3c7, 0x0e);                      /* index to PLL control */
      save->gendac.PLL_ctrl = inb(0x3c9);     /* PLL control */

      outb(vgaIOBase + 5, i & ~0x40);
    }
    if (OFLG_ISSET(CLOCK_OPTION_STG1703, &vga256InfoRec.clockOptions))
    {
      /* Restore STG 1703 GenDAC Command and PLL registers 
       * unfortunately we reuse the gendac data structure, so the 
       * field names are not really good.
       */

      xf86dactopel();
      save->gendac.cmd_reg = xf86getdaccomm();/* Enhanced command register */
      save->gendac.PLL_f2_M = STG1703getIndex(0x24);    
                                              /* f2 PLL M divider */
      save->gendac.PLL_f2_N = inb(0x3c6);     /* f2 PLL N1/N2 divider */
      save->gendac.PLL_ctrl = STG1703getIndex(0x03);    
                                              /* pixel mode select control */
    }
  }
#endif

  return ((void *) save);
}



/*
 * ET4000Init --
 *      Handle the initialization of the VGAs registers
 */

static Bool
ET4000Init(mode)
     DisplayModePtr mode;
{

#ifdef W32_ACCEL_SUPPORT
  int pixMuxShift = 0;

  /* define pixMuxShift depending on mode, pixel multiplexing and ramdac type */

  if (mode->Flags & V_PIXMUX)
  {
     if ((OFLG_ISSET(CLOCK_OPTION_ICS5341, &vga256InfoRec.clockOptions)) 
          && (W32RamdacType==ICS5341_DAC))
     {
        pixMuxShift =  mode->Flags & V_DBLCLK ? 1 : 0;
     }  
     if ((OFLG_ISSET(CLOCK_OPTION_STG1703, &vga256InfoRec.clockOptions)) 
          && (W32RamdacType==STG1703_DAC))
     {
        pixMuxShift =  mode->Flags & V_DBLCLK ? 1 : 0;
     }  
  }

  if (!mode->CrtcHAdjusted) {
     if (pixMuxShift > 0) {
        /* now divide the horizontal timing parameters as required */
        mode->CrtcHTotal     >>= pixMuxShift;
        mode->CrtcHDisplay   >>= pixMuxShift;
        mode->CrtcHSyncStart >>= pixMuxShift;
        mode->CrtcHSyncEnd   >>= pixMuxShift;
     }
     else if (pixMuxShift < 0) {
        /* now multiply the horizontal timing parameters as required */
        mode->CrtcHTotal     <<= -pixMuxShift;
        mode->CrtcHDisplay   <<= -pixMuxShift;
        mode->CrtcHSyncStart <<= -pixMuxShift;
        mode->CrtcHSyncEnd   <<= -pixMuxShift;
     }
     mode->CrtcHAdjusted = TRUE;
  }
#endif


  if (!vgaHWInit(mode,sizeof(vgaET4000Rec)))
    return(FALSE);



#ifndef MONOVGA
  new->std.Attribute[16] = 0x01;  /* use the FAST 256 Color Mode */
  new->std.CRTC[19] = vga256InfoRec.virtualX >> 3;
#endif
  new->std.CRTC[20] = 0x60;
  new->std.CRTC[23] = 0xAB;
  new->StateControl = 0x00; 
  new->AuxillaryMode = 0xBC;
  new->ExtStart = 0x00;

  new->OverflowHigh = (mode->Flags & V_INTERLACE ? 0x80 : 0x00)
    | 0x10
      | ((mode->CrtcVSyncStart & 0x400) >> 7 )
	| (((mode->CrtcVDisplay -1) & 0x400) >> 8 )
	  | (((mode->CrtcVTotal -2) & 0x400) >> 9 )
	    | (((mode->CrtcVSyncStart) & 0x400) >> 10 );

#ifdef MONOVGA
  new->Misc = 0x00;
#else
  new->Misc = 0x80;
#endif

#ifndef MONOVGA
  new->RCConf = initialRCConf;
  if (OFLG_ISSET(OPTION_FAST_DRAM, &vga256InfoRec.options))
  {
    /*
     *  make sure Trsp is no more than 75ns
     *            Tcsw is 25ns
     *            Tcsp is 25ns
     *            Trcd is no more than 50ns
     * Timings assume SCLK = 40MHz
     *
     * Note, this is experimental, but works for me (DHD)
     */
    /* Tcsw, Tcsp, Trsp */
    new->RCConf &= ~0x1F;
    if (initialRCConf & 0x18)
      new->RCConf |= 0x08;
    /* Trcd */
    new->RCConf &= ~0x20;
  }
#ifdef W32_SUPPORT
  /*
   * Here we make sure that CRTC regs 0x34 and 0x37 are untouched, except for 
   * some bits we want to change. 
   * Notably bit 7 of CRTC 0x34, which changes RAS setup time from 4 to 0 ns 
   * (performance),
   * and bit 7 of CRTC 0x37, which changes the CRTC FIFO low treshold control.
   * At really high pixel clocks, this will avoid lots of garble on the screen 
   * when something is being drawn. This only happens WAY beyond 80 MHz 
   * (those 135 MHz ramdac's...)
   */
   if (et4000_type > TYPE_ET4000) {
     new->Compatibility = (initialCompatibility & 0x7F) | 0x80;
     new->VSConf2 = initialVSConf2;
     if (vga256InfoRec.clock[mode->Clock] > 80000)
       new->VSConf2 = (new->VSConf2 & 0x7f) | 0x80;
   }

   if (et4000_type >= TYPE_ET4000W32P)
   {
      /*
       * next, we check the PCI Burst option and turn that on or of
       * which is done with bit 4 in CR34
       */
      if (OFLG_ISSET(OPTION_PCI_BURST_OFF, &vga256InfoRec.options))
         new->Compatibility &= 0xEF;
      if (OFLG_ISSET(OPTION_PCI_BURST_ON, &vga256InfoRec.options))
         new->Compatibility |= 0x10;
   }
   if (et4000_type >= TYPE_ET4000W32I)
   {
     /*
      * now on to the memory interleave setting (CR32 bit 7)
      */
      if (OFLG_ISSET(OPTION_W32_INTERLEAVE_OFF, &vga256InfoRec.options))
         new->RCConf &= 0x7F;
      if (OFLG_ISSET(OPTION_W32_INTERLEAVE_ON, &vga256InfoRec.options))
         new->RCConf |= 0x80;
   }
#endif

#endif
    
  /* Set clock-related registers when not Legend
   * cannot really SET the clock here yet, since the ET4000Save()
   * is called LATER, so it would save the wrong state...
   * and since they use the ET4000Restore() to actually SET vga regs,
   * we can only set the clock there (that's why we copy Synthclock into
   * the other struct.
   */

#ifdef W32_ACCEL_SUPPORT
    if (    (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions))
         && (    (W32RamdacType == STG1703_DAC)
              || (W32RamdacType == ICS5341_DAC)) )
    { 
      /* for pixmux: must use post-div of at least 4!
       */

#if EXTENDED_DEBUG
       ErrorF("--------  Requested Clock %9d\n",mode->SynthClock);
#endif
       if (mode->Flags & V_PIXMUX)
       {
         commonCalcClock(mode->SynthClock,2,100000,vga256InfoRec.dacSpeed*2+1, 
         		 &(new->gendac.PLL_f2_M), &(new->gendac.PLL_f2_N));
         if( W32RamdacType == STG1703_DAC ) {
            new->gendac.cmd_reg |= 8;
            new->gendac.PLL_ctrl = 0x05;              /* set DAC to 2*8 mode */
         } else if( W32RamdacType == ICS5341_DAC ) {
            new->gendac.cmd_reg = 0x10;               /* set DAC to 2*8 mode */
            new->gendac.PLL_ctrl = 0;
         }
         new->Misc = (new->Misc & 0xCF) | 0x20;   /* bits 5 and 4 set 8/16 bit 
	 					   * DAC mode, at the W32 side 
						   * (DAC needs to be set, too)
                                                   * here we set it to 16-bit 
						   * mode 
						   */

         /* set doubleword adressing -- seems to be needed for <1280 modes 
	  * to get correct screen 
	  */
#if THIS_SHOULD_BE_NECESSARY_BUT_FAILS
         new->std.CRTC[0x14] = (new->std.CRTC[0x14] & 0x9F) | 0x40;
#endif
         new->std.CRTC[0x17] = (new->std.CRTC[0x17] & 0xFB);
         
         /* to avoid blurred vertical line during flyback, disable H-blanking 
	  * (better solution needed !!!) 
	  */
         new->std.CRTC[0x02] = 0xff;
       }
       else
       {
         commonCalcClock(mode->SynthClock,0,100000,vga256InfoRec.dacSpeed*2+1, 
         		 &(new->gendac.PLL_f2_M), &(new->gendac.PLL_f2_N));
         if( W32RamdacType == STG1703_DAC ) {
            new->gendac.cmd_reg |= 8;
            new->gendac.PLL_ctrl = 0x00;              /* set DAC to 1*8 mode */
         } else if( W32RamdacType == ICS5341_DAC ) {
            new->gendac.cmd_reg = 0x00;               /* set DAC to 1*8 mode */
            new->gendac.PLL_ctrl = 0;
         }
         new->Misc = (new->Misc & 0xCF);   /* 8 bit DAC mode */
       }
       new->gendac.PLL_w_idx = 0;
       new->gendac.PLL_r_idx = 0;
       
       /* the programmed clock will be on clock index 2 */
       /* disable MCLK/2 and MCLK/4 */
       new->AuxillaryMode = (new->AuxillaryMode & 0xBE);   
       /* clear CS2: we need clock #2 */
       new->Compatibility = (new->Compatibility & 0xFD);   
       new->std.MiscOutReg = (new->std.MiscOutReg & 0xF3) | 0x08; 
       new->std.NoClock = 2;
    }
    else
#endif

    if (new->std.NoClock >= 0)
    {
      new->AuxillaryMode = (save_divide ^ ((new->std.NoClock & 8) << 3)) |
                           (new->AuxillaryMode & 0xBF);
      new->Compatibility = (new->Compatibility & 0xFD) | 
      				((new->std.NoClock & 0x04) >> 1);
    }

  return(TRUE);
}



/*
 * ET4000Adjust --
 *      adjust the current video frame to display the mousecursor
 */

static void 
ET4000Adjust(x, y)
     int x, y;
{
#ifdef MONOVGA
  int Base = (y * vga256InfoRec.displayWidth + x + 3) >> 3;
#else
  int Base = (y * vga256InfoRec.displayWidth + x + 1) >> 2;
#endif

  outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
  outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);
  outw(vgaIOBase + 4, ((Base & 0x0F0000) >> 8) | 0x33);
}

/*
 * ET4000ValidMode --
 *
 */
static Bool
ET4000ValidMode(mode)
DisplayModePtr mode;
{
return TRUE;
}
