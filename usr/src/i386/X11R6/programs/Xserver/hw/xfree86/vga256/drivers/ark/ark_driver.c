/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/ark/ark_driver.c,v 3.6 1996/01/12 14:37:41 dawes Exp $ */
/*
 * Copyright 1994  The XFree86 Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * DAVID WEXELBLAT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Written by Harm Hanemaayer (hhanemaa@cs.ruu.nl).
 * Modified for Diamond Stealth64 Graphics 2001 by Leon Bottou
 *   (bottou@laforia.ibp.fr).
 */
/* $XConsortium: ark_driver.c /main/6 1996/01/12 12:16:25 kaleb $ */


/*
 * This is a driver for ARK Logic chips.
 * The following chips are supported:
 *
 *	- ARK1000PV (32-bit DRAM, PCI or VESA bus, 8-bit RAMDAC inteface)
 *	- ARK2000PV (64-bit DRAM, PCI or VESA bus, 16-bit RAMDAC interface)
 *
 *	The driver does not support the older ARK1000VL yet, althought it
 * 	may work if you let the driver think it is an ARK1000PV.
 *
 * This initial driver supports 8bpp only, the default max dotclock
 * is 80 MHz.
 *
 * The ARK chips use an external clock generator and RAMDAC. The
 * driver supports either 16 fixed external clock frequencies or the
 * ICS5342 programmable clock chip. Cards that have an unsupported
 * clock device will need specific support for that clock device,
 * although the standard VGA 25 MHz clock will be available yielding
 * 640x480x256.
 *
 * The driver will use linear addressing by default on both VESA-bus and
 * PCI. If this gives problems, banked operation can be selected with
 * the "nolinear" option in the Device section of the XF86Config file.
 *
 * With linear addressing enabled, 16bpp is supported given a
 * suitable RAMDAC and clock availability, as would 32bpp on the ARK2000PV.
 */

/*
 * Saved registers:
 *
 * VGA Misc Output holds bits of 0-1 of the clock select index.
 * SR11 (Video Clock Select) bits 6-7 hold bits 2-3 of the clock
 *	select index.
 * SR1D bit 0 unlocks the extended registers.
 * SR10 bits 6-7 hold the amount of video memory.
 * CR50 bits 3-7 have the chip ID:
 *	0b10010 for ARK1000PV
 *	0b10011 for ARK2000PV
 * SR15 holds the Aperture Write Index (for banking).
 * SR16 holds the Aperture Read Index.
 * SR12 holds the aperture size in bits 0-1.
 * SR13 holds the low byte of the aperture base address (bits 16-23).
 * SR14 holds the high byte of the aperature base address (bits 24-31).
 * CR40 holds extended timing bits:
 *	Bit 3 holds bit 8 of the scanline offset.
 * SR11 bits 0-1 hold the Giant Shift Register mode,
 *	bits 2-3 hold the COP pixel size.
 * CR40 Extended horizontal CRTC timing bits.
 * CR41 Extended vertical CRTC timing bits.
 * CR42 Interlace retrace value.
 * CR44 VGA Enhancement bits:
 *	Bit 0 disabled RAMDAC access.
 *	Bit 2 sets interlaced CRTC timing.
 * SR18 holds the display FIFO size and threshold.
 * CR46 is the pixel clock control register:
 *	Bit 2 selects 16-bit output to RAMDAC (8-bit if zero).
 *	Bit 6 inverts the pixel clock.
 *	(Register is only used for ARK2000PV).
 * SR20-27, SR29, SR2A are hardware cursor registers.
 * SR28 and SR2B are hardware cursor registers on the ARK2000PV.
 */

/* The following defines the default max allowed raw clock frequency. */
#define ARK_DEFAULT_MAX_RAW_CLOCK_IN_KHZ	80000

/*
 * The following indicates whether high dot clocks 8bpp modes using
 * pixel multiplexing/RAMDAC clock doubling on a 16-bit RAMDAC
 * interface is supported. It cannot work currently because the SVGA
 * server can't handle required raw clocks that are half the pixel clock.
 */
 
/* #define ARK_8BPP_MULTIPLEXING_SUPPORTED */


/*
 * These are X and server generic header files.
 */
#include "X.h"
#include "input.h"
#include "screenint.h"

/*
 * These are XFree86-specific header files
 */
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#include "vga.h"


/*
 * If the driver makes use of XF86Config 'Option' flags, the following will be
 * required
 */
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"

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

#if !defined(MONOVGA) && !defined(XF86VGA16)
#include "vga256.h"
#endif

#include "ark_cursor.h"

extern vgaHWCursorRec vgaHWCursor;



/*
 * Driver data structures.
 */
typedef struct {
	/*
	 * This structure defines all of the register-level information
	 * that must be stored to define a video mode for this chipset.
	 * The 'vgaHWRec' member must be first, and contains all of the
	 * standard VGA register information, as well as saved text and
	 * font data.
	 */
	vgaHWRec std;               /* good old IBM VGA */
  	/* 
	 * Any other registers or other data that the new chipset needs
	 * to be saved should be defined here.  The Init/Save/Restore
	 * functions will manipulate theses fields.  Examples of things
	 * that would go here are registers that contain bank select
	 * registers, or extended clock select bits, or extensions to 
	 * the timing registers.  Use 'unsigned char' as the type for
	 * these registers.
	 */
	unsigned char SR10;
	unsigned char SR11;
	unsigned char SR12;
	unsigned char SR13;
	unsigned char SR14;
	unsigned char SR15;
	unsigned char SR16;
	unsigned char SR18;
	unsigned char SR20, SR21, SR22, SR23, SR24;	/* Hardware cursor. */
	unsigned char SR25, SR26, SR27, SR29, SR2A;
	unsigned char SR28, SR2B;
	unsigned char CR40;
	unsigned char CR41;
	unsigned char CR42;
	unsigned char CR44;
	unsigned char CR46;
	/* RAMDAC registers. */
	unsigned char DACCOMMAND;
	unsigned char STG17XX[3];	/* In addition to DACCOMMAND. */
	unsigned char GENDAC[6];	/* For ICS5342 GENDAC */
} vgaArkRec, *vgaArkPtr;


/*
 * Forward definitions for the functions that make up the driver.  See
 * the definitions of these functions for the real scoop.
 */
static Bool     ArkProbe();
static char *   ArkIdent();
static Bool     ArkClockSelect();
static void     ArkEnterLeave();
static Bool     ArkInit();
static Bool	ArkValidMode();
static void *   ArkSave();
static void     ArkRestore();
static void     ArkAdjust();
static void	ArkFbInit();
/*
 * These are the bank select functions.  There are defined in stub_bank.s
 */
void     ArkSetRead();
void     ArkSetWrite();
void     ArkSetReadWrite();

/*
 * This data structure defines the driver itself.  The data structure is
 * initialized with the functions that make up the driver and some data 
 * that defines how the driver operates.
 */
vgaVideoChipRec ARK = {
	/* 
	 * Function pointers
	 */
	ArkProbe,
	ArkIdent,
	ArkEnterLeave,
	ArkInit,
	ArkValidMode,
	ArkSave,
	ArkRestore,
	ArkAdjust,
	vgaHWSaveScreen,
	(void (*)())NoopDDA,
	ArkFbInit,
	ArkSetRead,
	ArkSetWrite,
	ArkSetReadWrite,
	/*
	 * This is the size of the mapped memory window, usually 64k.
	 */
	0x10000,		
	/*
	 * This is the size of a video memory bank for this chipset.
	 */
	0x10000,
	/*
	 * This is the number of bits by which an address is shifted
	 * right to determine the bank number for that address.
	 */
	16,
	/*
	 * This is the bitmask used to determine the address within a
	 * specific bank.
	 */
	0xFFFF,
	/*
	 * These are the bottom and top addresses for reads inside a
	 * given bank.
	 */
	0x00000, 0x10000,
	/*
	 * And corresponding limits for writes.
	 */
	0x00000, 0x10000,
	/*
	 * Whether this chipset supports a single bank register or
	 * seperate read and write bank registers.
	 */
	TRUE,
	/*
	 * If the chipset requires vertical timing numbers to be divided
	 * by two for interlaced modes, set this to VGA_DIVIDE_VERT.
	 */
	VGA_DIVIDE_VERT,
	/*
	 * This is a dummy initialization for the set of option flags
	 * that this driver supports.  It gets filled in properly in the
	 * probe function, if the probe succeeds (assuming the driver
	 * supports any such flags).
	 */
	{0,},
	/*
	 * This determines the multiple to which the virtual width of
	 * the display must be rounded for the 256-color server.  This
	 * will normally be 8, but may be 4 or 16 for some servers.
	 */
	8,
	/*
	 * If the driver includes support for a linear-mapped frame buffer
	 * for the detected configuration this should be set to TRUE in the
	 * Probe or FbInit function.  In most cases it should be FALSE.
	 */
	FALSE,
	/*
	 * This is the physical base address of the linear-mapped frame
	 * buffer (when used).  Set it to 0 when not in use.
	 */
	0,
	/*
	 * This is the size  of the linear-mapped frame buffer (when used).
	 * Set it to 0 when not in use.
	 */
	0,
	/*
	 * This is TRUE if the driver has support for 16bpp for the detected
	 * configuration. It must be set in the Probe function.
	 * It most cases it should be FALSE.
	 */
	FALSE,
	/*
	 * This is TRUE if the driver has support for 32bpp for the detected
	 * configuration.
	 */
	FALSE,
	/*
	 * This is a pointer to a list of builtin driver modes.
	 * This is rarely used, and in must cases, set it to NULL
	 */
	NULL,
	/*
	 * This is a factor that can be used to scale the raw clocks
	 * to pixel clocks.  This is rarely used, and in most cases, set
	 * it to 1.
	 */
	1,
};

/*
 * This is a convenience macro, so that entries in the driver structure
 * can simply be dereferenced with 'new->xxx'.
 */
#define new ((vgaArkPtr)vgaNewVideoState)

/*
 * If your chipset uses non-standard I/O ports, you need to define an
 * array of ports, and an integer containing the array size.  The
 * generic VGA ports are defined in vgaHW.c.
 */
static unsigned Ark_ExtPorts[] = { 0x3CB, 0x400 };
static int Num_Ark_ExtPorts =
	(sizeof(Ark_ExtPorts)/sizeof(Ark_ExtPorts[0]));

#define VESA	0
#define PCI	1

static int arkChip, arkBus, arkRamdac;
static int arkDacPathWidth, arkMultiplexingThreshold;
static int arkUse8bitColorComponents;
static int arkDisplayableMemory;

#define ARK1000VL	0
#define ARK1000PV	1
#define ARK2000PV	2

static SymTabRec chipsets[] = {
	{ ARK1000VL,	"ark1000vl" },
	{ ARK1000PV,	"ark1000pv" },
	{ ARK2000PV,	"ark2000pv" },
	{ -1,		"" },
};

#define ATT490	0
#define ATT498	1
#define ZOOMDAC 2
#define STG1700 3
#define ICS5342 4

static SymTabRec ramdacs[] = {
	{ ATT490,	"att20c490" },	/* Industry-standard 8-bit DAC */
	{ ATT490,	"ark1491a" },
	{ ATT490,	"w30c491" },	/* IC Works */
	{ ATT498,	"att20c498" },	/* Industry-standard 16-bit DAC */
	{ ATT498,	"w30c498" },	/* IC Works */
	{ ZOOMDAC,	"w30c516" },	/* IC Works ZoomDac */
	{ ZOOMDAC,	"zoomdac" },
	{ STG1700,	"stg1700" },
	{ ICS5342,	"ics5342" },
	{ -1,		"" },
};


/*
 * ArkIdent --
 *
 * Returns the string name for supported chipset 'n'.  Most drivers only
 * support one chipset, but multiple version may require that the driver
 * identify them individually (e.g. the Trident driver).  The Ident function
 * should return a string if 'n' is valid, or NULL otherwise.  The
 * server will call this function when listing supported chipsets, with 'n' 
 * incrementing from 0, until the function returns NULL.  The 'Probe'
 * function should call this function to get the string name for a chipset
 * and when comparing against an XF86Config-supplied chipset value.  This
 * cuts down on the number of places errors can creep in.
 */
static char *
ArkIdent(n)
int n;
{
	if (chipsets[n].token < 0)
		return NULL;
	else
		return chipsets[n].name;
}


/*
 * ICS5342Mode(PLLMode)
 * Sets ICS5342 in DAC or PLL mode
 */
static void
ICS5342Mode(PLLMode)
Bool PLLMode;
{
	if (PLLMode)
		modinx(0x3C4,0x1C,0x80,0x80);
	else
		modinx(0x3C4,0x1C,0x80,0x00);
}


/*
 * ICS5342Init(reg, freq)
 * Set programmable clock on ICS5342
 */
static int
ICS5342Init(reg, freq)
vgaArkPtr reg;
long freq;
{
	commonCalcClock(freq, 1, 100000, 250000, &(reg->GENDAC[3]),
			&(reg->GENDAC[4]));
	reg->std.MiscOutReg &= ~0xC;
	reg->std.MiscOutReg |= 0x8;
	reg->SR11 &= ~0xC0;
	ICS5342Mode(TRUE);
	reg->GENDAC[0] = inb(0x3c6);	/* Enhanced command register */
	reg->GENDAC[2] = inb(0x3c8);	/* PLL write index */
	reg->GENDAC[1] = inb(0x3c7);	/* PLL read index */
	outb(0x3c7, 0x0e);		/* index to PLL control */
	reg->GENDAC[5] = inb(0x3c9);	/* PLL control */
	ICS5342Mode(FALSE);
	return(0);
}

/*
 * ArkClockSelect --
 * 
 * This function selects the dot-clock with index 'no'.  In most cases
 * this is done my setting the correct bits in various registers (generic
 * VGA uses two bits in the Miscellaneous Output Register to select from
 * 4 clocks).  Care must be taken to protect any other bits in these
 * registers by fetching their values and masking off the other bits.
 *
 * This function returns FALSE if the passed index is invalid or if the
 * clock can't be set for some reason.
 */
static Bool
ArkClockSelect(no)
int no;
{
	static unsigned char save1, save2;
	unsigned char temp;

	switch(no)
	{
	case CLK_REG_SAVE:
		/*
		 * Here all of the registers that can be affected by
		 * clock setting should be saved into static variables.
		 */
		save1 = inb(0x3CC);
		save2 = rdinx(0x3C4, 0x11);
		break;
	case CLK_REG_RESTORE:
		/*
		 * Here all the previously saved registers are restored.
		 */
		outb(0x3C2, save1);
		modinx(0x3C4, 0x11, 0xC0, save2);
		break;
	default:
		/* 
	 	 * These are the generic two low-order bits of the clock select 
		 */
		temp = inb(0x3CC);
		outb(0x3C2, ( temp & 0xF3) | ((no << 2) & 0x0C));
		/* Two high bits. */
		modinx(0x3C4, 0x11, 0xC0, (no & 0xC) << 4);
	}
	return(TRUE);
}


/*
 * ArkProbe --
 *
 * This is the function that makes a yes/no decision about whether or not
 * a chipset supported by this driver is present or not.  The server will
 * call each driver's probe function in sequence, until one returns TRUE
 * or they all fail.
 *
 */
static Bool
ArkProbe()
{
	int maxclock8bpp, maxclock16bpp, maxclock32bpp;
	char *clockprobed = XCONFIG_GIVEN;

	/*
	 * Set up I/O ports to be used by this card.  Only do the second
	 * xf86AddIOPorts() if there are non-standard ports for this
	 * chipset.
	 */
	xf86ClearIOPortList(vga256InfoRec.scrnIndex);
	xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
	xf86AddIOPorts(vga256InfoRec.scrnIndex, 
		       Num_Ark_ExtPorts, Ark_ExtPorts);

	/*
	 * First we attempt to figure out if one of the supported chipsets
	 * is present.
	 */
	if (vga256InfoRec.chipset)
	{
		/*
		 * This is the easy case.  The user has specified the
		 * chipset in the XF86Config file.  All we need to do here
		 * is a string comparison against each of the supported
		 * names available from the Ident() function.  If this
		 * driver supports more than one chipset, there would be
		 * nested conditionals here (see the Trident and WD drivers
		 * for examples).
		 */
		arkChip = xf86StringToToken(chipsets, vga256InfoRec.chipset);
		if (arkChip >= 0)
			ArkEnterLeave(ENTER);
		else
			return FALSE;
    	}
  	else
	{
		/*
		 * OK.  We have to actually test the hardware.  The
		 * EnterLeave() function (described below) unlocks access
		 * to registers that may be locked, and for OSs that require
		 * it, enables I/O access.  So we do this before we probe,
		 * even though we don't know for sure that this chipset
		 * is present.
		 */
		unsigned char CR50, id, rev;
		ArkEnterLeave(ENTER);

		/*
		 * Check for read/writability of all the "Software Scratch"
		 * registers.
		 */
		if (!testinx(0x3C4, 0x1E)
		|| !testinx(0x3C4, 0x1F)
		|| !testinx(0x3C4, 0x1A)
		|| !testinx(0x3C4, 0x1B)) {
			ArkEnterLeave(LEAVE);
			return FALSE;
		}

		CR50 = rdinx(vgaIOBase + 4, 0x50);
		id = CR50 >> 3;
		rev = CR50 & 7;

		switch (CR50 >> 3) {
		case 0x12 :
			arkChip = ARK1000PV;
			break;
		case 0x13 :
			arkChip = ARK2000PV;
			break;
		default :
			ErrorF("%s %s: ark: Unknown ARK chip (id = 0x%02X, rev = %d)\n",
				XCONFIG_PROBED, vga256InfoRec.name,
				id, rev);
			ArkEnterLeave(LEAVE);
			return FALSE;
		}
    	}

	vga256InfoRec.chipset = ArkIdent(arkChip);

	/*
	 * If the user has specified the amount of memory in the XF86Config
	 * file, we respect that setting.
	 */
  	if (!vga256InfoRec.videoRam)
    	{
		/*
		 * Otherwise, do whatever chipset-specific things are 
		 * necessary to figure out how much memory (in kBytes) is 
		 * available.
		 */
		unsigned char SR10;
		SR10 = rdinx(0x3C4, 0x10);
		if (arkChip == ARK1000PV)
			if ((SR10 & 0x40) == 0)
				vga256InfoRec.videoRam = 1024;
			else
				vga256InfoRec.videoRam = 2048;
		if (arkChip == ARK2000PV)
			if ((SR10 & 0xC0) == 0)
				vga256InfoRec.videoRam = 1024;
			else
			if ((SR10 & 0xC0) == 0x40)
				vga256InfoRec.videoRam = 2048;
			else
				vga256InfoRec.videoRam = 4096;
    	}

	if (rdinx(0x3C4, 0x19) & 0x80)
		arkBus = VESA;
	else
		arkBus = PCI;

	/*
	 * Parse the specified RAMDAC and determine sane maximum
	 * supported pixel clock limits. Unsupported depths will
	 * be ruled out out by a max dot clock of 0.
	 */
	arkRamdac = xf86StringToToken(ramdacs, vga256InfoRec.ramdac);
	maxclock8bpp = 0;
	maxclock16bpp = 0;
	maxclock32bpp = 0;
	arkDacPathWidth = 8;
	arkMultiplexingThreshold = 999999;
	arkUse8bitColorComponents = FALSE;
	if (vga256InfoRec.dacSpeed <= 0)
		vga256InfoRec.dacSpeed = 80000;
	switch (arkRamdac) {
	case ATT490 :	/* Industry-standard 8-bit DAC. */
		if (vga256InfoRec.dacSpeed <= 0)
			vga256InfoRec.dacSpeed = 80000;
		maxclock8bpp = vga256InfoRec.dacSpeed;
		maxclock16bpp = maxclock8bpp / 2;
		break;
	case ATT498 :	/* Industry-standard 16-bit DAC. */
	case STG1700 :	/* Same limits as 498. */
		/*
		 * Trust the DAC speed rating for 8bpp (use RAMDAC
		 * clock doubling for high clocks, 16-bit path).
		 */
		if (vga256InfoRec.dacSpeed <= 0)
			vga256InfoRec.dacSpeed = 80000;
#ifdef ARK_8BPP_MULTIPLEXING_SUPPORTED
		maxclock8bpp = vga256InfoRec.dacSpeed;
#else
		maxclock8bpp = ARK_DEFAULT_MAX_RAW_CLOCK_IN_KHZ;
#endif
		if (vga256InfoRec.dacSpeed >= 135000)
			maxclock16bpp = 110000;
		else	/* 110 MHz 8bpp rated */
			maxclock16bpp = 80000;
		maxclock32bpp = maxclock16bpp / 2;
		arkDacPathWidth = 16;
#ifdef ARK_8BPP_MULTIPLEXING_SUPPORTED
		arkMultiplexingThreshold = maxclock8bpp / 2;
#else
		arkMultiplexingThreshold = 999999;
#endif
		break;
	case ZOOMDAC :
		/*
		 * IC Works ZoomDAC, used on Hercules Stingray 64
		 * and Stingray Pro/V.
		 */
		if (vga256InfoRec.dacSpeed <= 0)
			vga256InfoRec.dacSpeed = 80000;
#ifdef ARK_8BPP_MULTIPLEXING_SUPPORTED
		maxclock8bpp = vga256InfoRec.dacSpeed;
#else
		maxclock8bpp = ARK_DEFAULT_MAX_RAW_CLOCK_IN_KHZ;
#endif
		if (arkChip == ARK1000PV) {
			/* Uses only 8-bit path to 16-bit RAMDAC. */
			if (vga256InfoRec.dacSpeed >= 135000)
				maxclock16bpp = 67500;
			else	/* 110 MHz rated. */
				maxclock16bpp = 55000;
			break;
		}
		/* ARK2000PV, 16-bit path to RAMDAC. */
		if (vga256InfoRec.dacSpeed >= 135000)
			maxclock16bpp = 135000;
		else	/* 110 MHz rated. */
			maxclock16bpp = 110000;
		maxclock32bpp = maxclock16bpp / 2;
		arkDacPathWidth = 16;
#ifdef ARK_8BPP_MULTIPLEXING_SUPPORTED
		arkMultiplexingThreshold = maxclock8bpp / 2;
#else
		arkMultiplexingThreshold = 999999;
#endif
		break;
	case ICS5342:
		/* ICS5342 GENDAC used on Diamond Stealth64 Graphics 2001 */
		if (vga256InfoRec.dacSpeed >= 135000)
			vga256InfoRec.dacSpeed = 135000;
		else if (vga256InfoRec.dacSpeed <= 0)
			vga256InfoRec.dacSpeed = 110000;	/* guess */
#ifdef ARK_8BPP_MULTIPLEXING_SUPPORTED
		maxclock8bpp = vga256InfoRec.dacSpeed;
#else
		maxclock8bpp = ARK_DEFAULT_MAX_RAW_CLOCK_IN_KHZ;
#endif
		if (arkChip == ARK1000PV) {
			/* ARK1000PV, 8-bit path to RAMDAC. */
			maxclock16bpp = vga256InfoRec.dacSpeed / 2;
		} else {
			/* ARK2000PV, 16-bit path to RAMDAC. */
			maxclock16bpp = vga256InfoRec.dacSpeed; /* Not right */
			maxclock32bpp = maxclock16bpp / 2;
			arkDacPathWidth = 16;
#ifdef ARK_8BPP_MULTIPLEXING_SUPPORTED
			arkMultiplexingThreshold = maxclock8bpp / 2;
#else
			arkMultiplexingThreshold = 999999;
#endif
		}
		break;
	default :
		/* Unknown DAC. Only allow 8pp at conservative rate. */
		if (vga256InfoRec.dacSpeed <= 0)
			vga256InfoRec.dacSpeed = 80000;
		maxclock8bpp = ARK_DEFAULT_MAX_RAW_CLOCK_IN_KHZ;
		break;
	}
	/* Adjust max dot clocks according to chip PCLK limits. */
	if (arkChip == ARK1000PV) {
		if (maxclock8bpp > 120000)
			maxclock8bpp = 120000;
		if (maxclock16bpp > 60000)
			maxclock16bpp = 60000;
	}
	if (arkChip == ARK2000PV) {
#ifdef ARK_8BPP_MULTIPLEXING_SUPPORTED
		if (maxclock8bpp > 240000)
			maxclock8bpp = 240000;
#else
		if (maxclock8bpp > 120000)
			maxclock8bpp = 120000;
#endif
		if (maxclock16bpp > 120000)
			maxclock16bpp = 120000;
		if (maxclock32bpp > 60000)
			maxclock32bpp = 60000;
	}

	if ((arkRamdac == ICS5342) &&
	    !OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions))
	{
		OFLG_SET(CLOCK_OPTION_PROGRAMABLE,
			 &vga256InfoRec.clockOptions);
		OFLG_SET(CLOCK_OPTION_ICS5342, &vga256InfoRec.clockOptions);
		clockprobed = XCONFIG_PROBED;
	}
	if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE,
		       &vga256InfoRec.clockOptions))
	{
		if (OFLG_ISSET(CLOCK_OPTION_ICS5342,
			       &vga256InfoRec.clockOptions))
		{
			int m, n, mclk;

			/* We can read MCLK from register 10 */
			ICS5342Mode(TRUE);
			outb(0x3C7, 10);
			m = inb(0x3C9) & 0x7F;
			n = inb(0x3C9) & 0x7F;
			ICS5342Mode(FALSE);
			mclk = ((1431818*(m+2)) / ((n&0x1f)+2) /
				(1<<(n>>5)) + 50) / 100;
			if (vga256InfoRec.s3MClk==0)
				vga256InfoRec.s3MClk = mclk;
			if (xf86Verbose)
			{
				ErrorF("%s %s: Using ICS5342 programmable"
				       " clock (MCLK %1.3f MHz)\n",
				       clockprobed, vga256InfoRec.name,
				       mclk / 1000.0);
			}
		}
		else
		{
			ErrorF("%s %s: Unsupported programmable clock chip.\n",
			       XCONFIG_PROBED, vga256InfoRec.name);
			ArkEnterLeave(LEAVE);
			return(FALSE);
		}
	}
	/*
	 * Again, if the user has specified the clock values in the XF86Config
	 * file, and a programmable clock is not being used, we respect
	 * those choices.
	 */
	if (!OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE, &vga256InfoRec.clockOptions)
	    && !vga256InfoRec.clocks)
    	{
		/*
		 * This utility function will probe for the clock values.
		 * It is passed the number of supported clocks, and a
		 * pointer to the clock-select function.
		 */
      		vgaGetClocks(16, ArkClockSelect);
    	}

	/*
	 * It would be nice to know the memory clock to adjust
	 * the max clocks according to DRAM bandwidth. For now
	 * just interpret the "S3MClk" value if given in the
	 * XF86Config file.
	 */
	if (vga256InfoRec.s3MClk != 0) {
		int DRAM_bandwidth, bandwidth_limit;
		DRAM_bandwidth = vga256InfoRec.s3MClk * 2;
		if (arkChip == ARK2000PV && vga256InfoRec.videoRam >= 2048)
			/* 64-bit DRAM bus. */
			DRAM_bandwidth *= 2;
	         /*
	          * Calculate highest acceptable DRAM bandwidth in Mbytes/s
	          * to be taken up by screen refresh. Satisfies
	          *	total bandwidth >= refresh bandwidth * 1.1
	          */
		bandwidth_limit = (DRAM_bandwidth * 100) / 110;
		if (maxclock8bpp > bandwidth_limit)
			maxclock8bpp = bandwidth_limit;
		if (maxclock16bpp > bandwidth_limit / 2)
			maxclock16bpp = bandwidth_limit / 2;
		if (maxclock32bpp > bandwidth_limit / 4)
			maxclock32bpp = bandwidth_limit / 4;
	}
	/*
	 * Set the raw clock dividing factor. For 16bpp with an
	 * 8-bit DAC path, the raw clock is double the pixel clock.
	 * The same happens for 32bpp with a 16-bit DAC path.
	 */
 	if (arkChip == ARK1000PV || arkDacPathWidth == 8) {
		/* 8-bit RAMDAC path. */
		if (vgaBitsPerPixel == 16) {
			ARK.ChipClockScaleFactor = 2;
			maxclock16bpp *= 2;
		}
		if (vgaBitsPerPixel == 32) {
			/* Unlikely to happen (currently can't). */
			ARK.ChipClockScaleFactor = 4;
			maxclock32bpp *= 4;
		}
	}
	if (arkDacPathWidth == 16) {
		/* 16-bit RAMDAC path. */
		if (vgaBitsPerPixel == 32) {
			ARK.ChipClockScaleFactor = 2;
			maxclock32bpp *= 2;
		}
		/*
		 * A problem arises with "multiplexing" at 8bpp.
		 * The scaling factor would have to be 0.5, and
		 * it would only apply to certain modes. The current
		 * interface does not seem to be able to handle this.
		 */
	}
	if (vgaBitsPerPixel == 8)
		vga256InfoRec.maxClock = maxclock8bpp;
	if (vgaBitsPerPixel == 16)
		vga256InfoRec.maxClock = maxclock16bpp;
	if (vgaBitsPerPixel == 32)
		vga256InfoRec.maxClock = maxclock32bpp;

	/* Use linear addressing by default. */
	if (!OFLG_ISSET(OPTION_NOLINEAR_MODE, &vga256InfoRec.options)) {
		ARK.ChipUseLinearAddressing = TRUE;
		if (vga256InfoRec.MemBase != 0)
			ARK.ChipLinearBase =
				vga256InfoRec.MemBase;
		else
			if (arkBus == PCI)
				/* PCI: get pnp information */
				ARK.ChipLinearBase =
					(rdinx(0x3C4, 0x13) << 16) +
					(rdinx(0x3C4, 0x14) << 24);
			else
				/* VESA local bus. */
				/* Pray that 2048MB works. */
				ARK.ChipLinearBase = 0x80000000;
		/*
		 * Set 16bpp and 32bpp information into the ARK chipset
		 * structure only when linear mode is enabled.
		 */
		ARK.ChipLinearSize = vga256InfoRec.videoRam * 1024;
		if (maxclock16bpp > 0)
			ARK.ChipHas16bpp = TRUE;
		if (arkChip == ARK2000PV && maxclock32bpp > 0)
			ARK.ChipHas32bpp = TRUE;
	}

	/*
	 * Last we fill in the remaining data structures.  We specify
	 * the chipset name, using the Ident() function and an appropriate
	 * index.  We set a boolean for whether or not this driver supports
	 * banking for the Monochrome server.  And we set up a list of all
	 * the option flags that this driver can make use of.
	 */
  	vga256InfoRec.bankedMono = FALSE;
 	OFLG_SET(OPTION_NOLINEAR_MODE, &ARK.ChipOptionFlags);
#ifndef MONOVGA
#ifdef XFreeXDGA
	vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
#endif

  	return TRUE;
}

/*
 * ArkFbInit --
 *      enable speedups for the chips that support it
 */
static void
ArkFbInit()
{
	int offscreen_available;

	if (xf86Verbose && ARK.ChipUseLinearAddressing)
		ErrorF("%s %s: %s: Using linear framebuffer at 0x%08X (%s)\n",
			XCONFIG_PROBED, vga256InfoRec.name,
			vga256InfoRec.chipset, ARK.ChipLinearBase,
			arkBus == PCI ? "PCI bus" : "VL bus");

	arkDisplayableMemory = vga256InfoRec.virtualX * vga256InfoRec.virtualY
		* (vgaBitsPerPixel / 8);
	offscreen_available = vga256InfoRec.videoRam * 1024 -
		arkDisplayableMemory;

	if (xf86Verbose)
		ErrorF("%s %s: %s: %d bytes off-screen video memory available\n",
			XCONFIG_PROBED, vga256InfoRec.name,
			vga256InfoRec.chipset, offscreen_available);
    
    	/*
    	 * Currently, the hardware cursor is not supported at 8bpp
    	 * due to a framebuffer code architecture issue.
    	 */
	if (!OFLG_ISSET(OPTION_SW_CURSOR, &vga256InfoRec.options)
	    && vgaBitsPerPixel != 8) {
		if (offscreen_available < 256)
			ErrorF("%s %s: %s: Not enough off-screen video"
				" memory for hardware cursor\n",
				XCONFIG_PROBED, vga256InfoRec.name,
				vga256InfoRec.chipset);
		else {
			/*
			 * OK, there's at least 256 bytes available
			 * at the end of video memory to store
			 * the cursor image, so we can use the hardware
			 * cursor.
			 */
			arkCursorWidth = 32;
			arkCursorHeight = 32;
			vgaHWCursor.Initialized = TRUE;
			vgaHWCursor.Init = ArkCursorInit;
			vgaHWCursor.Restore = ArkRestoreCursor;
			vgaHWCursor.Warp = ArkWarpCursor;
			vgaHWCursor.QueryBestSize = ArkQueryBestSize;
			if (xf86Verbose)
				ErrorF("%s %s: %s: Using hardware cursor\n",
					XCONFIG_PROBED, vga256InfoRec.name,
					vga256InfoRec.chipset);
		}
	}
}


/*
 * ArkEnterLeave --
 *
 * This function is called when the virtual terminal on which the server
 * is running is entered or left, as well as when the server starts up
 * and is shut down.  Its function is to obtain and relinquish I/O 
 * permissions for the SVGA device.  This includes unlocking access to
 * any registers that may be protected on the chipset, and locking those
 * registers again on exit.
 */
static void 
ArkEnterLeave(enter)
Bool enter;
{
	/*
	 * The value of the lock register is saved at the first
	 * "Enter" call, restored at a "Leave". This reduces the
	 * risk of messing up the registers of another chipset.
	 */
	static int enterCalled = FALSE;
	static int savedSR1D;
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

		/* 
		 * This is a global.  The CRTC base address depends on
		 * whether the VGA is functioning in color or mono mode.
		 * This is just a convenient place to initialize this
		 * variable.
		 */
      		vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

		/*
		 * Here we deal with register-level access locks.  This
		 * is a generic VGA protection; most SVGA chipsets have
		 * similar register locks for their extended registers
		 * as well.
		 */
      		/* Unprotect CRTC[0-7] */
      		outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
      		outb(vgaIOBase + 5, temp & 0x7F);
      		if (enterCalled == FALSE) {
      			savedSR1D = rdinx(0x3C4, 0x1D);
      			enterCalled = TRUE;
      		}
      		/* Set bit 0 of SR1D. */
      		modinx(0x3C4, 0x1D, 0x01, 0x01);
    	}
  	else
    	{
		/*
		 * Here undo what was done above.
		 */

		/* Protect CRTC[0-7] */
		outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
		outb(vgaIOBase + 5, (temp & 0x7F) | 0x80);

		wrinx(0x3C4, 0x1D, savedSR1D);

		xf86DisableIOPorts(vga256InfoRec.scrnIndex);
	}
}

/*
 * ArkRestore --
 *
 * This function restores a video mode.  It basically writes out all of
 * the registers that have previously been saved in the vgaArkRec data 
 * structure.
 *
 * Note that "Restore" is a little bit incorrect.  This function is also
 * used when the server enters/changes video modes.  The mode definitions 
 * have previously been initialized by the Init() function, below.
 */
static void 
ArkRestore(restore)
vgaArkPtr restore;
{
	/*
	 * Whatever code is needed to get things back to bank zero should be
	 * placed here.  Things should be in the same state as when the
	 * Save/Init was done.
	 */

	/* Set Read and Write aperture index to 0. */
	wrinx(0x3C4, 0x15, 0x00);
	wrinx(0x3C4, 0x16, 0x00);

	/*
	 * Write the extended registers fist so that textmode font
	 * restoration can succeed.
	 */
	wrinx(0x3C4, 0x10, restore->SR10);
	if (restore->std.NoClock >= 0)
		wrinx(0x3C4, 0x11, restore->SR11);
	else
		/* Don't modify the clock select bits in SR11. */
		modinx(0x3C4, 0x11, 0x3F, restore->SR11);
	wrinx(0x3C4, 0x12, restore->SR12);
	wrinx(0x3C4, 0x13, restore->SR13);
	wrinx(0x3C4, 0x14, restore->SR14);

	wrinx(0x3C4, 0x15, restore->SR15);
	wrinx(0x3C4, 0x16, restore->SR16);
	wrinx(0x3C4, 0x18, restore->SR18);

	/* Hardware cursor registers. */
	wrinx(0x3C4, 0x20, restore->SR20);
	wrinx(0x3C4, 0x21, restore->SR21);
	wrinx(0x3C4, 0x22, restore->SR22);
	wrinx(0x3C4, 0x23, restore->SR23);
	wrinx(0x3C4, 0x24, restore->SR24);
	wrinx(0x3C4, 0x25, restore->SR25);
	wrinx(0x3C4, 0x26, restore->SR26);
	wrinx(0x3C4, 0x27, restore->SR27);
	wrinx(0x3C4, 0x29, restore->SR29);
	wrinx(0x3C4, 0x2A, restore->SR2A);
	if (arkChip == ARK2000PV) {
		wrinx(0x3C4, 0x28, restore->SR28);
		wrinx(0x3C4, 0x2B, restore->SR2B);
	}

 	wrinx(vgaIOBase + 4, 0x40, restore->CR40);
 	wrinx(vgaIOBase + 4, 0x41, restore->CR41);
 	wrinx(vgaIOBase + 4, 0x42, restore->CR42);
 	wrinx(vgaIOBase + 4, 0x44, restore->CR44);
 	if (arkChip == ARK2000PV)
	 	wrinx(vgaIOBase + 4, 0x46, restore->CR46);

	/* RAMDAC registers. */
	if (arkRamdac == ATT490 || arkRamdac == ATT498
	|| arkRamdac == ZOOMDAC || arkRamdac == STG1700)
		xf86setdaccomm(restore->DACCOMMAND);
	if (arkRamdac == STG1700) {
		xf86dactocomm();
		inb(0x3C6);		/* Skip command reg. */
		outb(0x3C6, 0x03);	/* Index low. */
		outb(0x3C6, 0x00);	/* Index high. */
		outb(0x3C6, restore->STG17XX[0]); /* Primary pixel mode. */
		outb(0x3C6, restore->STG17XX[1]); /* Secondary pixel mode. */
		outb(0x3C6, restore->STG17XX[2]); /* PLL control. */
		usleep(500);
		xf86dactopel();
	}
	if (arkRamdac == ICS5342) {
		ICS5342Mode(TRUE);
		outb(0x3c6, restore->GENDAC[0]); /* Enhanced command register */
		outb(0x3c8, 2);			 /* index to f2 reg */
		outb(0x3c9, restore->GENDAC[3]); /* f2 PLL M divider */
		outb(0x3c9, restore->GENDAC[4]); /* f2 PLL N1/N2 divider */
		outb(0x3c8, 0x0e);		 /* index to PLL control */
		outb(0x3c9, restore->GENDAC[5]); /* PLL control */
		outb(0x3c8, restore->GENDAC[2]); /* PLL write index */
		outb(0x3c7, restore->GENDAC[1]); /* PLL read index */
		ICS5342Mode(FALSE);
	}

	/*
	 * This function handles restoring the generic VGA registers.
	 */
	vgaHWRestore((vgaHWPtr)restore);

}

/*
 * ArkSave --
 *
 * This function saves the video state.  It reads all of the SVGA registers
 * into the vgaArkRec data structure.  There is in general no need to
 * mask out bits here - just read the registers.
 */
static void *
ArkSave(save)
vgaArkPtr save;
{
	/*
	 * Whatever code is needed to get back to bank zero goes here.
	 */

	/* Set Read and Write aperture index to 0. */
	wrinx(0x3C4, 0x15, 0x00);
	wrinx(0x3C4, 0x16, 0x00);

	/*
	 * This function will handle creating the data structure and filling
	 * in the generic VGA portion.
	 */
	save = (vgaArkPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaArkRec));

	save->SR10 = rdinx(0x3C4, 0x10);
	save->SR11 = rdinx(0x3C4, 0x11);
	save->SR12 = rdinx(0x3C4, 0x12);
	save->SR13 = rdinx(0x3C4, 0x13);
	save->SR14 = rdinx(0x3C4, 0x14);
	save->SR15 = rdinx(0x3C4, 0x15);
	save->SR16 = rdinx(0x3C4, 0x16);
	save->SR18 = rdinx(0x3C4, 0x18);

	/* Hardware cursor registers. */
	save->SR20 = rdinx(0x3C4, 0x20);
	save->SR21 = rdinx(0x3C4, 0x21);
	save->SR22 = rdinx(0x3C4, 0x22);
	save->SR23 = rdinx(0x3C4, 0x23);
	save->SR24 = rdinx(0x3C4, 0x24);
	save->SR25 = rdinx(0x3C4, 0x25);
	save->SR26 = rdinx(0x3C4, 0x26);
	save->SR27 = rdinx(0x3C4, 0x27);
	save->SR29 = rdinx(0x3C4, 0x29);
	save->SR2A = rdinx(0x3C4, 0x2A);
	if (arkChip == ARK2000PV) {
		save->SR28 = rdinx(0x3C4, 0x28);
		save->SR2B = rdinx(0x3C4, 0x2B);
	}

	save->CR40 = rdinx(vgaIOBase + 4,  0x40);
	save->CR41 = rdinx(vgaIOBase + 4,  0x41);
	save->CR42 = rdinx(vgaIOBase + 4,  0x42);
	save->CR44 = rdinx(vgaIOBase + 4,  0x44);
 	if (arkChip == ARK2000PV)
		save->CR46 = rdinx(vgaIOBase + 4,  0x46);

	/* RAMDAC registers. */
	if (arkRamdac == ATT490 || arkRamdac == ATT498
	|| arkRamdac == ZOOMDAC || arkRamdac == STG1700)
		save->DACCOMMAND = xf86getdaccomm();
	if (arkRamdac == STG1700) {
		xf86dactocomm();
		inb(0x3C6);		/* Skip command reg. */
		outb(0x3C6, 0x03);	/* Index low. */
		outb(0x3C6, 0x00);	/* Index high. */
		save->STG17XX[0] = inb(0x3C6);	/* Primary pixel mode. */
		save->STG17XX[1] = inb(0x3C6);	/* Secondary pixel mode. */
		save->STG17XX[2] = inb(0x3C6);	/* PLL control. */
		xf86dactopel();
	}
	if (arkRamdac == ICS5342) {
		ICS5342Mode(TRUE);
		save->GENDAC[0] = inb(0x3c6);	/* Enhanced command register */
		save->GENDAC[2] = inb(0x3c8);	/* PLL write index */
		save->GENDAC[1] = inb(0x3c7);	/* PLL read index */
		outb(0x3c7, 2);			/* index to f2 reg */
		save->GENDAC[3] = inb(0x3c9);	/* f2 PLL M divider */
		save->GENDAC[4] = inb(0x3c9);	/* f2 PLL N1/N2 divider */
		outb(0x3c7, 0x0e);		/* index to PLL control */
		save->GENDAC[5] = inb(0x3c9);	/* PLL control */
		ICS5342Mode(FALSE);
	}
  	return ((void *) save);
}

/*
 * ArkInit --
 *
 * This is the most important function (after the Probe) function.  This
 * function fills in the vgaArkRec with all of the register values needed
 * to enable either a 256-color mode (for the color server) or a 16-color
 * mode (for the monochrome server).
 *
 * The 'mode' parameter describes the video mode.  The 'mode' structure 
 * as well as the 'vga256InfoRec' structure can be dereferenced for
 * information that is needed to initialize the mode.  The 'new' macro
 * (see definition above) is used to simply fill in the structure.
 */
static Bool
ArkInit(mode)
DisplayModePtr mode;
{
	int multiplexing;
	int dac16;

	/* 
	 * Determine if 8bpp clock doubling is to be used
	 * (two pixels sent over a 16-bit RAMDAC path each
	 * clock, with a raw clock half the pixel rate).
	 */
	multiplexing = 0;
	if (vgaBitsPerPixel == 8 && arkDacPathWidth == 16
	&& vga256InfoRec.clock[mode->Clock] > arkMultiplexingThreshold)
		multiplexing = 1;

	/*
	 * Scale the horizontal CRTC timings if required. This
	 * must be done before the VGA CRTC register values
	 * are initialized.
	 */
	if (ARK.ChipClockScaleFactor == 2)
		if (!mode->CrtcHAdjusted) {
			mode->CrtcHDisplay <<= 1;
			mode->CrtcHSyncStart <<= 1;
			mode->CrtcHSyncEnd <<= 1;
			mode->CrtcHTotal <<= 1;
			mode->CrtcHAdjusted = TRUE;
		}
	if (ARK.ChipClockScaleFactor == 4)
		if (!mode->CrtcHAdjusted) {
			mode->CrtcHDisplay <<= 2;
			mode->CrtcHSyncStart <<= 2;
			mode->CrtcHSyncEnd <<= 2;
			mode->CrtcHTotal <<= 2;
			mode->CrtcHAdjusted = TRUE;
		}
	if (multiplexing)
		/*
		 * This is linked to the fact that the
		 * ChipClockScaleFactor is (should be) equal to 0.5.
		 */
		if (!mode->CrtcHAdjusted) {
			mode->CrtcHDisplay >>= 1;
			mode->CrtcHSyncStart >>= 1;
			mode->CrtcHSyncEnd >>= 1;
			mode->CrtcHTotal >>= 1;
			mode->CrtcHAdjusted = TRUE;
		}

	/*
	 * This will allocate the datastructure and initialize all of the
	 * generic VGA registers.
	 */
	if (!vgaHWInit(mode,sizeof(vgaArkRec)))
		return(FALSE);

	/*
	 * Here all of the other fields of 'new' get filled in, to
	 * handle the SVGA extended registers.  It is also allowable
	 * to override generic registers whenever necessary.
	 *
	 */

	/* Select 8 or 16-bit video output to RAMDAC on 2000PV. */
	if (arkChip == ARK2000PV) {
		new->CR46 = rdinx(vgaIOBase + 4, 0x46) & ~0x04;	/* 8-bit */
		dac16 = 0;
		if (multiplexing)
			/* High resolution 8bpp with 16-bit DAC. */
			dac16 = 1;
		if (vga256InfoRec.bitsPerPixel == 16)
			/* 16bpp at pixel rate. */
			dac16 = 1;
		/* Note: with an 8-bit DAC, 16bpp is Clock * 2. */
		if (vga256InfoRec.bitsPerPixel == 32)
			/* 32bpp at Clock * 2. */
			dac16 = 1;
		if (arkDacPathWidth == 8)
			/* If an 8-bit DAC is used, forget it. */
			dac16 = 0;
		if (dac16)
			new->CR46 |= 0x04; /* 16-bit */
	}

	/*
	 * The ARK chips have a scale factor of 8 for the 
	 * scanline offset. There is one extended bit in addition
	 * to the 8 VGA bits. This results in a max logical scanline
	 * width of 4088 bytes.
	 */
	{
		int offset;
		offset = (vga256InfoRec.virtualX *
			vga256InfoRec.bitsPerPixel / 8)	>> 3;
		/* Bits 0-7 are in generic register */
		new->std.CRTC[0x13] = offset;
		/* Bit 8 resides at CR41 bit 3. */
		new->CR41 = (offset & 0x100) >> 5;
	}

	/* Set Giant Shift Register for SVGA mode and set COP pixel depth. */
	new->SR11 = rdinx(0x3C4, 0x11) & ~0x0F;
	if (vga256InfoRec.bitsPerPixel == 8)
		new->SR11 |= 0x06;
	if (vga256InfoRec.bitsPerPixel == 16)
		new->SR11 |= 0x0A;
	if (vga256InfoRec.bitsPerPixel == 32)
		if (arkChip == ARK2000PV)
			new->SR11 |= 0x0E;
		else
			/*
			 * The AR1000PV can't accelerate 32bpp, but
			 * there may be a DAC that can skip the fourth
			 * byte of each pixel, making 640x400 possible
			 * with a raw clock of 100 MHz. So program
			 * the COP for 8bpp.
			 */
			new->SR11 |= 0x06;

	/*
	 * Enable VESA Super VGA memory organisation.
	 * Also enable Linear Addressing and COP.
	 * Linear addressing also seems to be required for banked
	 * operation.
	 */
	new->SR10 = rdinx(0x3C4, 0x10) & ~0x1F;
	new->SR10 |= 0x1F;
	if (ARK.ChipUseLinearAddressing) {
		/*
		 * Linear addressing; program the aperture base address
		 * bits 16-31.
		 */
		new->SR13 = ARK.ChipLinearBase >> 16; /* Bits 16-23. */
		new->SR14 = ARK.ChipLinearBase >> 24; /* Bits 24-31. */
		/* Program the aperture size. */
		new->SR12 = rdinx(0x3C4, 0x12) & ~0x03;
		if (ARK.ChipLinearSize == 1024 * 1024)
			new->SR12 |= 0x01;
		if (ARK.ChipLinearSize == 2048 * 1024)
			new->SR12 |= 0x02;
		if (ARK.ChipLinearSize == 4096 * 1024)
			new->SR12 |= 0x03;
	}
	else {
		/* Banking; Map aperture at 0xA0000. */
		new->SR13 = 0x0A;
		new->SR14 = 0x00;
		/* Set 64K aperture (set bits 0-1 of SR12 to 0). */
		new->SR12 = rdinx(0x3C4, 0x12) & ~0x03;
	}
	/* Set banking registers to zero. */
	new->SR15 = 0;
	new->SR16 = 0;


	/* Handle the CRTC overflow bits. */
	{
		unsigned char val;
		/* Vertical Overflow. */
		val = 0;
		if ((mode->CrtcVTotal - 2) & 0x400)
			val |= 0x80;
		if ((mode->CrtcVDisplay - 1) & 0x400)
			val |= 0x40;
		/* VBlankStart is equal to VSyncStart + 1. */
		if (mode->CrtcVSyncStart & 0x400)
			val |= 0x20;
		/* VRetraceStart is equal to VSyncStart + 1. */
		if (mode->CrtcVSyncStart & 0x400)
			val |= 0x10;
		new->CR40 = val;

		/* Horizontal Overflow. */
		val = new->CR41;	/* Initialized earlier */
		if ((mode->CrtcHTotal / 8 - 5) & 0x100)
			val |= 0x80;
		if ((mode->CrtcHDisplay / 8 - 1) & 0x100)
			val |= 0x40;
		/* HBlankStart is equal to HSyncStart - 1. */
		if ((mode->CrtcHSyncStart / 8 - 1) & 0x100)
			val |= 0x20;
		/* HRetraceStart is equal to HSyncStart. */
		if ((mode->CrtcHSyncStart / 8) & 0x100)
			val |= 0x10;
		new->CR41 |= val;
	}
	/* Set VGA Enhancement register. */
	/* No interlace, standard character clock. */
	new->CR44 = rdinx(vgaIOBase + 4, 0x44) & ~0x34;
	new->CR42 = 0;
	if (mode->Flags & V_INTERLACE) {
		/* Set mid-scan vertical retrace start. */
		new->CR42 = (mode->CrtcHTotal / 8 - 5) / 2;
		new->CR44 |= 0x04;	/* Interlaced flag. */
	}

	/* Set the display FIFO threshold. */
	{
		int threshold;
		unsigned char val;
		val = rdinx(0x3C4, 0x18);
		if (arkChip == ARK1000PV) {
			threshold = 4;	/* A guess. */
			val |= 0x08;	/* Enable full FIFO (8-deep). */
			val &= ~0x07;
			val |= threshold;
		}
		if (arkChip == ARK2000PV) {
			threshold = 12;	/* A guess. */
			val &= 0x40;
			val |= 0x10;	/* 32-deep FIFO. */
			val |= (threshold & 0x0E) >> 1;
			if (threshold & 0x01)
				val |= 0x80;
			if (threshold & 0x10)
				val |= 0x20;
		}
		new->SR18 = val;
	}

	/*
	 * A special case - when using an external clock-setting program,
	 * this function must not change bits associated with the clock
	 * selection.  This condition can be checked by the condition:
	 *
	 *	if (new->std.NoClock >= 0)
	 *		initialize clock-select bits.
	 */

	if (new->std.NoClock >= 0) {
		if (OFLG_ISSET(CLOCK_OPTION_PROGRAMABLE,
			      &vga256InfoRec.clockOptions)) {
			if (OFLG_ISSET(CLOCK_OPTION_ICS5342,
				       &vga256InfoRec.clockOptions)) {
			    ICS5342Init(new,
					vga256InfoRec.clock[new->std.NoClock]);
			}
		} else {
			/* Program clock select. */
			new->std.MiscOutReg &= ~0xC;
			new->std.MiscOutReg |= (new->std.NoClock & 3) << 2;
			/* The rest of SR11 was initialized earlier. */
			new->SR11 &= ~0xC0;
			new->SR11 |= (new->std.NoClock & 0xC) << 4;
		}
	}

	/* Set up the RAMDAC registers. */

	if (arkRamdac == ATT490) {
		new->DACCOMMAND = 0x00;
		if (vgaBitsPerPixel == 16 && xf86weight.green == 5)
			/* 5-5-5 RGB. */
			new->DACCOMMAND = 0xA0;
		if (vgaBitsPerPixel == 16 && xf86weight.green == 6)
			/* 5-6-5 RGB. */
			new->DACCOMMAND = 0xC0;
	}
	if (arkRamdac == ATT498 || arkRamdac == ZOOMDAC) {
		new->DACCOMMAND = 0x00;
		if (vgaBitsPerPixel == 8 && multiplexing)
			new->DACCOMMAND = 0x20;
		if (vgaBitsPerPixel == 16 && arkDacPathWidth == 16)
			if (xf86weight.green == 5)
				new->DACCOMMAND = 0x10;
			else
				new->DACCOMMAND = 0x30;
		if (vgaBitsPerPixel == 16 && arkDacPathWidth == 8)
			/* Only 5-6-5 supported. */
			new->DACCOMMAND = 0x60;
		if (vgaBitsPerPixel == 32 && arkDacPathWidth == 16)
			new->DACCOMMAND = 0x50;
	}
	if (arkRamdac == STG1700) {
		/* This is adapted from accel/s3init.c. */
		int pixmode;
		new->DACCOMMAND = (xf86getdaccomm() & 0x06) | 0x10;
		new->STG17XX[2] = 0x00;	/* XXXX Guess. */
		pixmode = 0;
		if (vgaBitsPerPixel == 8 && multiplexing) {
			/* Enable extended modes. */
			new->DACCOMMAND |= 0x08;
			pixmode = 5;
			new->STG17XX[2] = 0x02;	/* 64-135 MHz pixclk. */
			/*
			 * XXXX What is the normal value for STG1700[2]
			 * (the PLL control register)? Assume it
			 * is zero (see above).
			 */
		}
		if (vgaBitsPerPixel == 16 && arkDacPathWidth == 16)
			if (xf86weight.green == 5) {
				new->DACCOMMAND |= 0xA8;
				pixmode = 0x02;
			}
			else {
				new->DACCOMMAND |= 0xC8;
				pixmode = 0x03;
			}
		if (vgaBitsPerPixel == 32 && arkDacPathWidth == 16) {
			new->DACCOMMAND = 0xE8;
			pixmode = 0x04;
		}
		new->STG17XX[0] = pixmode;	/* Primary pixel mode. */
		new->STG17XX[1] = pixmode;	/* Secondary pixel mode. */
	}
	if (arkRamdac == ATT490 || arkRamdac == ATT498
	|| arkRamdac == ZOOMDAC || arkRamdac == STG1700)
		if (vgaBitsPerPixel == 8 && arkUse8bitColorComponents)
			new->DACCOMMAND |= 0x02;
	if (vgaBitsPerPixel > 8)
		/* Get rid of white border. */
		new->std.Attribute[0x11] = 0x00;

	if (arkRamdac == ICS5342) {
		/* 8 bpp, 1clk/1pixel */
		new->GENDAC[0] = new->DACCOMMAND = 0;
		/* 8 bpp, 1clk/2pixel */
		if (vgaBitsPerPixel==8 && multiplexing)
			new->GENDAC[0] = new->DACCOMMAND = 0x10;
		/* 16 bpp, 2clk/1pixel */
		/* 16 bpp, 1clk/1pixel */
		if (vgaBitsPerPixel == 16 && arkDacPathWidth == 16) {
			if (xf86weight.green == 5)
				new->GENDAC[0] = new->DACCOMMAND = 0x30;
			else
				new->GENDAC[0] = new->DACCOMMAND = 0x50;
		}
		/* 32 bpp, 2clk/1pixel */
		if (vgaBitsPerPixel == 32 && arkDacPathWidth == 16)
			new->GENDAC[0] = new->DACCOMMAND = 0x70;
	}
	
	/*
	 * Hardware cursor registers.
	 * Generally the SVGA server will take care of enabling the
	 * cursor after a mode switch.
	 */
	new->SR20 = rdinx(0x3C4, 0x20);
	new->SR21 = rdinx(0x3C4, 0x21);
	new->SR22 = rdinx(0x3C4, 0x22);
	new->SR23 = rdinx(0x3C4, 0x23);
	new->SR24 = rdinx(0x3C4, 0x24);
	new->SR25 = rdinx(0x3C4, 0x25);
	new->SR26 = rdinx(0x3C4, 0x26);
	new->SR27 = rdinx(0x3C4, 0x27);
	new->SR29 = rdinx(0x3C4, 0x29);
	new->SR2A = rdinx(0x3C4, 0x2A);
	if (arkChip == ARK2000PV) {
		new->SR28 = rdinx(0x3C4, 0x28);
		new->SR2B = rdinx(0x3C4, 0x2B);
	}

	return TRUE;
}

/*
 * ArkAdjust --
 *
 * This function is used to initialize the SVGA Start Address - the first
 * displayed location in the video memory.  This is used to implement the
 * virtual window.
 */
static void 
ArkAdjust(x, y)
int x, y;
{
	int Base = ((y * vga256InfoRec.displayWidth + x)
		* (vgaBitsPerPixel / 8));

	/*
	 * 64 bit memory access when 'ark2000pv' with 'memory>=2048'
	 */
	if (arkChip == ARK2000PV && vga256InfoRec.videoRam >= 2048)
		Base >>= 3;
	else
		Base >>= 2;
	/*
	 * These are the generic starting address registers.
	 */
	outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
  	outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);

	/*
	 * Here the high-order bits are masked and shifted, and put into
	 * the appropriate extended registers.
	 */
	modinx(vgaIOBase + 4, 0x40, 0x07, (Base & 0x070000) >> 16);
}

/*
 * ArkValidMode --
 *
 */
static Bool
ArkValidMode(mode)
DisplayModePtr mode;
{
	/* Check for CRTC timing bits overflow. */
	if (mode->HTotal > 4088) {
		ErrorF("%s %s: %s: Horizontal mode timing overflow (%d)\n",
			XCONFIG_PROBED, vga256InfoRec.name,
			vga256InfoRec.chipset, mode->HTotal);
		return FALSE;
	}
	if (mode->VTotal > 2047) {
		ErrorF("%s %s: %s: Vertical mode timing overflow (%d)\n",
			XCONFIG_PROBED, vga256InfoRec.name,
			vga256InfoRec.chipset, mode->VTotal);
		return FALSE;
	}

	return TRUE;
}
