/* $XConsortium: sis86c201.c /main/3 1996/01/13 13:14:59 kaleb $ */
/*
 * Copyright 1995 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Alan Hourihane, alanh@fairlite.demon.co.uk
 *
 * ToDo: 	Set 256/64K/16M colours
 *		Fix Interlacing
 *		Hardware Cursor ?
 *		(Linear ? - might have done this - can't test)
 *
 * Currently only works for VGA16 with Non-Interlaced modes.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/sis/sis86c201.c,v 3.2 1996/01/13 12:22:16 dawes Exp $ */

#include "X.h"
#include "input.h"
#include "screenint.h"
#include "dix.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "vga.h"
#include "vgaPCI.h"

#ifdef XF86VGA16
#define MONOVGA
#endif

#ifndef MONOVGA
#error "Not yet supported at 256 colours"
#endif

typedef struct {
	vgaHWRec std;          		/* std IBM VGA register 	*/
	unsigned char ClockReg;
	unsigned char DualBanks;
	unsigned char BankReg;
	unsigned char DispCRT;
	unsigned char ReadBank;
	unsigned char WriteBank;
} vgaSISRec, *vgaSISPtr;

static Bool SISClockSelect();
static char *SISIdent();
static Bool SISProbe();
static void SISEnterLeave();
static Bool SISInit();
static Bool SISValidMode();
static void *SISSave();
static void SISRestore();
static void SISFbInit();
static void SISAdjust();
extern void SISSetRead();
extern void SISSetWrite();
extern void SISSetReadWrite();

vgaVideoChipRec SIS = {
  SISProbe,
  SISIdent,
  SISEnterLeave,
  SISInit,
  SISValidMode,
  SISSave,
  SISRestore,
  SISAdjust,
  vgaHWSaveScreen,
  (void (*)())NoopDDA,
  SISFbInit,
  SISSetRead,
  SISSetWrite,
  SISSetReadWrite,
  0x10000,
  0x10000,
  16,
  0xffff,
  0x00000, 0x10000,
  0x00000, 0x10000,
  TRUE,                           
  VGA_DIVIDE_VERT,
  {0,},
  16,				
  FALSE,
  0,
  0,
  FALSE, /* 16bpp - not yet ! */	
  FALSE, /* 32bpp - not yet ! */
  NULL,
  1,
};

#define new ((vgaSISPtr)vgaNewVideoState)

#define SIS86C201 0

int SISchipset;
Bool sisUseLinear = FALSE;

/*
 * SISIdent --
 */
static char *
SISIdent(n)
	int n;
{
	static char *chipsets[] = {"sis86c201", };

	if (n + 1 > sizeof(chipsets) / sizeof(char *))
		return(NULL);
	else
		return(chipsets[n]);
}

/*
 * SISClockSelect --
 * 	select one of the possible clocks ...
 */
static Bool
SISClockSelect(no)
	int no;
{
	static unsigned char save1, save2;
	unsigned char temp;

	/*
	 * CS0 and CS1 are in MiscOutReg
	 *
	 * CS2,CS3,CS4 are in 0x3C4 index 7
	 * But - only active when CS0/CS1 are set.
	 */
	switch(no)
	{
	case CLK_REG_SAVE:
		save1 = inb(0x3CC);
		outb(0x3C4, 0x07);
		save2 = inb(0x3C5);
		break;
	case CLK_REG_RESTORE:
		outb(0x3C2, save1);
		outw(0x3C4, (save2 << 8) | 0x07);
		break;
	default:
		/*
		 * Do CS0 and CS1 and set them - makes index 7 valid
		 */
		temp = inb(0x3CC);
		outb(0x3C2, (temp & 0xF3) | 0x0C);

		outb(0x3C4, 0x07); temp = inb(0x3C5) & 0xF0;
		outb(0x3C5, no | temp);
	}
	return(TRUE);
}

/* 
 * SISProbe --
 * 	check up whether a SIS 86C201 based board is installed
 */
static Bool
SISProbe()
{
  	int numClocks;
  	unsigned char temp;

	/*
         * Set up I/O ports to be used by this card
	 */
	xf86ClearIOPortList(vga256InfoRec.scrnIndex);
	xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);

	SISchipset = -1;

  	if (vga256InfoRec.chipset)
    	{
		/*
		 * If chipset from XF86Config doesn't match...
		 */
		if (!StrCaseCmp(vga256InfoRec.chipset, SISIdent(0)))
			SISchipset = SIS86C201;
    	}
	else
	{
		/* Aparently there are only PCI based 86C201's */

		if (vgaPCIInfo && vgaPCIInfo->Vendor == PCI_VENDOR_SIS)
		{
			switch(vgaPCIInfo->ChipType)
			{
			case PCI_CHIP_SG86C201: 	/* 86C201 */
				SISchipset = SIS86C201;
#ifndef MONOVGA
/* PCI space doesn't seem to have this set, check out FbInit */
#if PCI_BASE_0
				if (vgaPCIInfo->MemBase != 0)
				{
				    SIS.ChipLinearBase = vgaPCIInfo->MemBase;
				    isUseLinear = TRUE;
				}
#endif
#endif
				break;
			}
		}
		if (SISchipset == -1)
			return (FALSE);
		vga256InfoRec.chipset = SISIdent(SISchipset);
	}

	SISEnterLeave(ENTER);
	
 	/* 
	 * How much Video Ram have we got?
	 */
    	if (!vga256InfoRec.videoRam)
    	{
		unsigned char temp;

		outb(0x3C4, 0x0F); 
		temp = inb(0x3C5);

		switch (temp & 0x03) 
		{
		case 0: 
			vga256InfoRec.videoRam = 1024;
			break;
		case 1:
			vga256InfoRec.videoRam = 2048;
			break;
		case 2: 
			vga256InfoRec.videoRam = 4096;
			break;
		}
     	}

	/*
	 * If clocks are not specified in XF86Config file, probe for them
	 */
    	if (!vga256InfoRec.clocks)
	{
		numClocks = 16;
		vgaGetClocks(numClocks, SISClockSelect);
	}

#ifndef MONOVGA
	/* MaxClock set at 90MHz for 256 - ??? */
	OFLG_SET(OPTION_NOLINEAR_MODE, &SIS.ChipOptionFlags);
#else
	/* Set to 130MHz at 16 colours */
	vga256InfoRec.maxClock = 130000;
#endif

    	return(TRUE);
}

/*
 * SISFbInit --
 *	enable speedups for the chips that support it
 */
static void
SISFbInit()
{
#ifndef MONOVGA
	/*
	 * The PCI Configuration Space doesn't seem to set a base
	 * address for the linear aperture. We can do this with the
	 * linear registers. But - We must use MemBase to do this.
	 */
	unsigned char temp_lo, temp_hi;

	outb(0x3C4, 0x20);
	temp_lo = inb(0x3C5); 		/* Get Linear Status */

	outb(0x3C4, 0x21);
	temp_hi = inb(0x3C5);

	if (vga256InfoRec.MemBase != 0)
	{
		temp_lo = (vga256InfoRec.MemBase / (512*1024)) & 0x00FF;
		temp_hi = ((vga256InfoRec.MemBase / (512*1024)) & 0xFF00) >> 8;
		outw(0x3C4, (temp_lo << 8) | 0x20);
		outw(0x3C4, ((temp_hi | 0x30) << 8) | 0x21);
	}

	/* Linear address is in 512K segments */
	SIS.ChipLinearBase = ((temp_hi << 8) | temp_lo) * (512*1024);

	if ( (SIS.ChipLinearBase == 0) ||
	     (OFLG_ISSET(OPTION_NOLINEAR_MODE, &vga256InfoRec.options)) )
		sisUseLinear = FALSE;

	if (xf86LinearVidMem() && sisUseLinear)
	{
		SIS.ChipLinearSize = vga256InfoRec.videoRam * 1024;
		ErrorF("%s %s: Using Linear Frame Buffer at 0x0%x, Size %dMB\n",
			XCONFIG_PROBED, vga256InfoRec.name,
			SIS.ChipLinearBase, SIS.ChipLinearSize/1048576);
		outb(0x3C4, 0xF021);	/* Enable Linear */
	}

	if (sisUseLinear)
		SIS.ChipUseLinearAddressing = TRUE;
	else
		SIS.ChipUseLinearAddressing = FALSE;
#endif /* MONOVGA */
}

/*
 * SISEnterLeave --
 * 	enable/disable io-mapping
 */
static void
SISEnterLeave(enter)
	Bool enter;
{
  	unsigned char temp;

  	if (enter)
    	{
      		xf86EnableIOPorts(vga256InfoRec.scrnIndex);
		vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
      		outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
      		outb(vgaIOBase + 5, temp & 0x7F);

		outw(0x3C4, 0x8605); 	/* Unlock Specials */
    	}
  	else
    	{
		outw(0x3C4, 0x0005);	/* Lock Specials */

      		xf86DisableIOPorts(vga256InfoRec.scrnIndex);
    	}
}

/*
 * SISRestore --
 *      restore a video mode
 */
static void
SISRestore(restore)
     	vgaSISPtr restore;
{
	outw(0x3C4, ((restore->BankReg) << 8) | 0x06);
	outw(0x3C4, ((restore->ClockReg) << 8) | 0x07);
	outw(0x3C4, ((restore->DualBanks) << 8) | 0x0B);
	outw(0x3C4, ((restore->DispCRT) << 8) | 0x27);
	
	/*
	 * Now restore generic VGA Registers
	 */
	vgaHWRestore((vgaHWPtr)restore);
}

/*
 * SISSave --
 *      save the current video mode
 */
static void *
SISSave(save)
     	vgaSISPtr save;
{
  	save = (vgaSISPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaSISRec));

	outb(0x3C4, 0x06); save->BankReg = inb(0x3C5);
	outb(0x3C4, 0x07); save->ClockReg = inb(0x3C5);
	outb(0x3C4, 0x0B); save->DualBanks = inb(0x3C5);
	outb(0x3C4, 0x27); save->DispCRT = inb(0x3C5);

  	return ((void *) save);
}

/*
 * SISInit --
 *      Handle the initialization, etc. of a screen.
 */
static Bool
SISInit(mode)
    	DisplayModePtr mode;
{
	unsigned char temp;

	/*
	 * Initialize generic VGA registers.
	 */
	vgaHWInit(mode, sizeof(vgaSISRec));
  
	new->std.CRTC[19] = vga256InfoRec.virtualX >>
		(mode->Flags & V_INTERLACE ? 3 : 4);

	new->BankReg = 0x02;

	if (mode->Flags & V_INTERLACE)
		new->BankReg |= 0x20;

	new->DualBanks = 0x08;

	if (new->std.NoClock >= 0)
		new->ClockReg = (new->std.NoClock >> 2);

        return(TRUE);
}

/*
 * SISAdjust --
 *      adjust the current video frame to display the mousecursor
 */

static void 
SISAdjust(x, y)
	int x, y;
{
	unsigned char temp;
	int base;

	base = (y * vga256InfoRec.displayWidth + x + 3) >> 3;

  	outw(vgaIOBase + 4, (base & 0x00FF00) | 0x0C);
	outw(vgaIOBase + 4, ((base & 0x00FF) << 8) | 0x0D);

	outb(0x3C4, 0x27); temp = inb(0x3C5) & 0xF0;
	if (base > 0xFFFF)
		temp |= (base & 0xFF0000) >> 16;
	outw(0x3C4, (temp << 8) | 0x27);
}

/*
 * SISValidMode --
 *
 */
static Bool
SISValidMode(mode)
DisplayModePtr mode;
{
return TRUE;
}
