/* $XConsortium: ct_driver.c /main/6 1996/01/12 12:16:39 kaleb $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/chips/ct_driver.c,v 3.10 1996/01/12 14:37:50 dawes Exp $ */
/*
 * Copyright 1993 by Jon Block <block@frc.com>
 * Modified by Mike Hollick <hollick@graphics.cis.upenn.edu>
 * Modified 1994 by Régis Cridlig <cridlig@dmi.ens.fr>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This driver has been collected from the net, having passed through
 * several people. It is NOT a stable or complete driver.
 *
 * It has only one time been verified to work for the Chips & Technologies
 * 65530 chipset in 640x480x256 mode. It does not correctly handle dot-clocks
 * other than 25 and 28 MHz.
 *
 * The driver code has much obsolete excluded code and has some suspect
 * bits. Notably the extended registers do not seem to ever be unlocked
 * (the code in the EnterLeave function is commented out).
 *
 */

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

/*
 * In many cases, this is sufficient for VGA16 support when VGA2 support is
 * already done
 */

#ifdef XF86VGA16
#define MONOVGA
#endif

/*
 * This header is required for drivers that implement STUBFbInit().
 */
#if !defined(MONOVGA) && !defined(XF86VGA16)
/*#include "vga256.h"*/
#endif

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
    vgaHWRec std;                                 /* good old IBM VGA */
  	/* 
	 * Any other registers or other data that the new chipset needs
	 * to be saved should be defined here.  The Init/Save/Restore
	 * functions will manipulate theses fields.  Examples of things
	 * that would go here are registers that contain bank select
	 * registers, or extended clock select bits, or extensions to 
	 * the timing registers.  Use 'unsigned char' as the type for
	 * these registers.
	 */
    unsigned char Port_3D6[128];  /* Chips & Technologies Registers */
} vgaCHIPSRec, *vgaCHIPSPtr;

/*
 * Forward definitions for the functions that make up the driver.    See
 * the definitions of these functions for the real scoop.
 */
static Bool         CHIPSProbe();
static char *       CHIPSIdent();
static Bool         CHIPSClockSelect();
static void         CHIPSEnterLeave();
static Bool         CHIPSInit();
static Bool         CHIPSValidMode();
static void *       CHIPSSave();
static void         CHIPSRestore();
static void         CHIPSAdjust();
#if 0
static void         CHIPSSaveScreen();
static void         CHIPSGetMode();
#endif
/*
 * These are the bank select functions.  There are defined in chips_bank.s
 */
extern void         CHIPSSetRead();
extern void         CHIPSSetWrite();
extern void         CHIPSSetReadWrite();

/*
 * This data structure defines the driver itself.    The data structure is
 * initialized with the functions that make up the driver and some data 
 * that defines how the driver operates.
 */
vgaVideoChipRec CHIPS = {
	/* 
	 * Function pointers
	 */
    CHIPSProbe,
    CHIPSIdent,
    CHIPSEnterLeave,
    CHIPSInit,
    CHIPSValidMode,
    CHIPSSave,
    CHIPSRestore,
    CHIPSAdjust,
    vgaHWSaveScreen,     /* CHIPSSaveScreen */
    (void (*)())NoopDDA, /* CHIPSGetMode */
    (void (*)())NoopDDA, /* CHIPSFbInit */
    CHIPSSetRead,
    CHIPSSetWrite,
    CHIPSSetReadWrite,
	/*
	 * This is the size of the mapped memory window, usually 64k.
	 */
    0x10000,
	/*
	 * This is the size of a video memory bank for this chipset.
	 */
    0x08000,
	/*
	 * This is the number of bits by which an address is shifted
	 * right to determine the bank number for that address.
	 */
    15,
	/*
	 * This is the bitmask used to determine the address within a
	 * specific bank.
	 */
    0x7FFF,
	/*
	 * These are the bottom and top addresses for reads inside a
	 * given bank.
	 */
    0x0000, 0x08000,
	/*
	 * And corresponding limits for writes.
	 */
    0x08000, 0x10000,
	/*
	 * Whether this chipset supports a single bank register or
	 * seperate read and write bank registers.  Almost all chipsets
	 * support two banks, and two banks are almost always faster
	 * (Trident 8900C and 9000 are odd exceptions).
	 */
    TRUE,
	/*
	 * If the chipset requires vertical timing numbers to be divided
	 * by two for interlaced modes, set this to VGA_DIVIDE_VERT.
	 */
    VGA_NO_DIVIDE_VERT,
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
	 * for the detected configuratio this should be set to TRUE in the
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
#define new ((vgaCHIPSPtr)vgaNewVideoState)

/*
 * If your chipset uses non-standard I/O ports, you need to define an
 * array of ports, and an integer containing the array size.  The
 * generic VGA ports are defined in vgaHW.c.
 */
static unsigned CHIPS_ExtPorts[] = {0x46E8, 0x103, 0x3D6, 0x3D7 };
static int Num_CHIPS_ExtPorts =
	(sizeof(CHIPS_ExtPorts)/sizeof(CHIPS_ExtPorts[0]));

#define CT_520   0
#define CT_530   1
#define CT_540   2
#define CT_545   3
#ifdef CT45X_SUPPORT
/* CT_451 - CT457 are not supproted */
#define CT_451   4
#define CT_452   5
#define CT_453   6
#define CT_455   7
#define CT_456   8
#define CT_457   9
#endif

static unsigned char CHIPSchipset;

/*
 * CHIPSIdent --
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
CHIPSIdent(n)
int n;
{
    static char *chipsets[] = { 
				"ct65520", "ct65530", "ct65540", "ct65545",
#ifdef CT45X_SUPPORT
				"ct451", "ct452", "ct453", "ct455",
				"ct456", "ct457",
#endif
			      };
#ifdef DEBUG	
    ErrorF("CHIPSIdent\n");
#endif
    if (n + 1 > sizeof(chipsets) / sizeof(char *))
        return(NULL);
    else
        return(chipsets[n]);
}

/*
 * CHIPSClockSelect --
 * 
 * This function selects the dot-clock with index 'no'.  In most cases
 * this is done my setting the correct bits in various registers (generic
 * VGA uses two bits in the Miscellaneous Output Register to select from
 * 4 clocks).    Care must be taken to protect any other bits in these
 * registers by fetching their values and masking off the other bits.
 *
 * This function returns FALSE if the passed index is invalid or if the
 * clock can't be set for some reason.
 */
static Bool
CHIPSClockSelect(no)
int no;
{
    static unsigned char msr_save, fcr_save;
    unsigned char temp;
    int fcr_clock=no-4;

#ifdef DEBUG
    ErrorF("CHIPSClockSelect\n");
#endif
    switch(no)
    {
        case CLK_REG_SAVE:
		/*
		 * Here all of the registers that can be affected by
		 * clock setting should be saved into static variables.
		 */
            msr_save = inb(0x3CC);
	    fcr_save = inb(0x3CA);
            break;
        case CLK_REG_RESTORE:
		/*
		 * Here all the previously saved registers are restored.
		 */
            outb(0x3C2, msr_save);
	    outb(0x3DA, fcr_save);
            break;
        default:
	    /* set MSR */
	    /* if the requested clock is greater than 2, the other
	     * register does the work */
	    if (no>=4)
	      no=2;
	    if (fcr_clock>3)
	      return FALSE;

            temp = inb(0x3CC);
	    /* this also has to mask out the sync polarity bits: No! */
/*	    if (no>=2)
	      outb(0x3C2, ( temp & 0xF3) | ((no << 2) & 0x0C) | 0xE0);
	    else
	      outb(0x3C2, ( temp & 0x73) | ((no << 2) & 0x0C)); */
            outb(0x3C2, ( temp & 0xF3) | ((no << 2) & 0x0C));

	    /* set FCR */
	    if (fcr_clock >= 0) {
		temp = inb(0x3CA);
		outb(0x3DA, ((temp & 0xFC) | fcr_clock));
	    }

	    /* debug */
#ifdef DEBUG
	    ErrorF("requested clock %i, ",no);
	    ErrorF("MSR now %X\n",inb(0x3CC));
	    ErrorF("FCR now %X\n",inb(0x3CA));
#endif
	    return(TRUE);
    }
}


/*
 * CHIPSProbe --
 *
 * This is the function that makes a yes/no decision about whether or not
 * a chipset supported by this driver is present or not.    The server will
 * call each driver's probe function in sequence, until one returns TRUE
 * or they all fail.
 *
 * Pretty much any mechanism can be used to determine the presence of the
 * chipset.  If there is a BIOS signature (e.g. ATI, GVGA), it can be read
 * via /dev/mem on most OSs, but some OSs (e.g. Mach) require special
 * handling, and others (e.g. Amoeba) don't allow reading    the BIOS at
 * all.  Hence, this mechanism is discouraged, if other mechanisms can be
 * found.    If the BIOS-reading mechanism must be used, examine the ATI and
 * GVGA drivers for the special code that is needed.    Note that the BIOS 
 * base should not be assumed to be at 0xC0000 (although most are).  Use
 * 'vga256InfoRec.BIOSbase', which will pick up any changes the user may
 * have specified in the XF86Config file.
 *
 * The preferred mechanism for doing this is via register identification.
 * It is important not only the chipset is detected, but also to
 * ensure that other chipsets will not be falsely detected by the probe
 * (this is difficult, but something that the developer should strive for).  
 * For testing registers, there are a set of utility functions in the 
 * "compiler.h" header file.    A good place to find example probing code is
 * in the SuperProbe program, which uses algorithms from the "vgadoc2.zip"
 * package (available on most PC/vga FTP mirror sites, like ftp.uu.net and
 * wuarchive.wustl.edu).
 *
 * Once the chipset has been successfully detected, then the developer needs 
 * to do some other work to find memory, and clocks, etc, and do any other
 * driver-level data-structure initialization may need to be done.
 */
static Bool
CHIPSProbe()
{
    unsigned char temp;

#ifdef DEBUG
    ErrorF("CHIPSProbe\n"); 
#endif

   /*
	 * Set up I/O ports to be used by this card.  Only do the second
	 * xf86AddIOPorts() if there are non-standard ports for this
	 * chipset.
    */
    xf86ClearIOPortList(vga256InfoRec.scrnIndex);
    xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
    xf86AddIOPorts(vga256InfoRec.scrnIndex,Num_CHIPS_ExtPorts,CHIPS_ExtPorts);

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
        if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_520))) {
            CHIPSchipset = CT_520;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_530))) {
            CHIPSchipset = CT_530;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_540))) {
            CHIPSchipset = CT_540;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_545))) {
            CHIPSchipset = CT_545;
        }
#ifdef CT54x_SUPPORT
	else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_451))) {
            CHIPSchipset = CT_451;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_452))) {
            CHIPSchipset = CT_452;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_453))) {
            CHIPSchipset = CT_453;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_455))) {
            CHIPSchipset = CT_455;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_456))) {
            CHIPSchipset = CT_456;
        } else if (!StrCaseCmp(vga256InfoRec.chipset, CHIPSIdent(CT_457))) {
            CHIPSchipset = CT_457;
        }
#endif
	else {
/*	    ErrorF("bomb 0\n"); */
            return (FALSE);
        }
        CHIPSEnterLeave(ENTER);
    }   else {
  		/*
		 * OK.  We have to actually test the hardware.  The
		 * EnterLeave() function (described below) unlocks access
		 * to registers that may be locked, and for OSs that require
		 * it, enables I/O access.  So we do this before we probe,
		 * even though we don't know for sure that this chipset
		 * is present.
		 */
      CHIPSEnterLeave(ENTER);

		/*
		 * Here is where all of the probing code should be placed.  
		 * The best advice is to look at what the other drivers are 
		 * doing.  If you are lucky, the chipset reference will tell 
		 * how to do this.  Other resources include SuperProbe/vgadoc2,
		 * and the Ferraro book.
		 */
        temp = rdinx(0x3D6,0x00);
/*
 *  Reading 0x103 causes segmentation violation, like 46E8 ???
 *  So for now just force what I want!
 *
 *  Need to look at ioctl(console_fd, PCCONIOCMAPPORT, &ior)
 *  for bsdi!
 */
        CHIPSchipset = 99;
        if (temp != 0xA5) {
	    if ((temp&0xF0)==0x70)
	      CHIPSchipset = CT_520;
	    if ((temp&0xF0)==0x80)
	      CHIPSchipset = CT_530;
	    if ((temp&0xF8)==0xD0)
	      CHIPSchipset = CT_540;
	    if ((temp&0xF8)==0xD8)
	      CHIPSchipset = CT_545;
        };
        if (CHIPSchipset==99)
        { /* failure, if no good, then leave */
			/*
			 * Turn things back off if the probe is going to fail.
			 * Returning FALSE implies failure, and the server
			 * will go on to the next driver.
			 */
            CHIPSEnterLeave(LEAVE);
#ifdef DEBUG
	    ErrorF("Bombing out!\n");
#endif
            return(FALSE);
	  }
    }

    /* configuration information */
    outb(0x3D6,0x01);
    temp = inb(0x3D7);
#ifdef DEBUG
    ErrorF("configuration register = %X\n",temp);
#endif

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
	      outb(0x3D6,0x04);
	      temp = inb(0x3D7);
	      if (CHIPSchipset==CT_540 ||CHIPSchipset==CT_545)
		switch (temp&3)
		{ case 0 :
		    vga256InfoRec.videoRam = 1024;
		    break;
		  case 1 :
		    vga256InfoRec.videoRam = 512;
		    break;
		  case 2 :
		    vga256InfoRec.videoRam = 1024;
		    break;
		  }
	      else
		switch (temp&3)
		{ case 0 :
		    vga256InfoRec.videoRam = 256;
		    break;
		  case 1 :
		    vga256InfoRec.videoRam = 512;
		    break;
		  case 3 :
		    vga256InfoRec.videoRam = 1024;
		    break;
		  }
            }

    /*
     * Again, if the user has specified the clock values in the XF86Config
     * file, we respect those choices.
     */
        if (!vga256InfoRec.clocks)
            {
        /*
         * This utility function will probe for the clock values.
         * It is passed the number of supported clocks, and a
         * pointer to the clock-select function.
         */

                vgaGetClocks(8, CHIPSClockSelect);
            }

	/*
 	 * It is recommended that you fill in the maximum allowable dot-clock
	 * rate for your chipset.  If you don't do this, the default of
	 * 90MHz will be used; this is likely too high for many chipsets.
	 * This is specified in KHz, so 90Mhz would be 90000 for this
	 * setting.
	 */
	vga256InfoRec.maxClock = 65000;

    /*
     * Last we fill in the remaining data structures.    We specify
     * the chipset name, using the Ident() function and an appropriate
     * index.    We set a boolean for whether or not this driver supports
     * banking for the Monochrome server.    And we set up a list of all
     * the vendor flags that this driver can make use of.
     */
        vga256InfoRec.chipset = CHIPSIdent(CHIPSchipset);
        vga256InfoRec.bankedMono = FALSE;
/*	OFLG_SET(OPTION_FLG1, &CHIPS.ChipOptionFlags); */
#ifndef MONOVGA
#ifdef XFreeXDGA
	vga256InfoRec.directMode = XF86DGADirectPresent;
#endif
#endif

        return(TRUE);
}

/*
 * CHIPSEnterLeave --
 *
 * This function is called when the virtual terminal on which the server
 * is running is entered or left, as well as when the server starts up
 * and is shut down.    Its function is to obtain and relinquish I/O 
 * permissions for the SVGA device.  This includes unlocking access to
 * any registers that may be protected on the chipset, and locking those
 * registers again on exit.
 */

static void 
CHIPSEnterLeave(enter)
Bool enter;
{
    unsigned char temp;

#ifdef DEBUG
    ErrorF("CHIPSEnterLeave\n");
#endif

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
         * This is a global.    The CRTC base address depends on
         * whether the VGA is functioning in color or mono mode.
         * This is just a convenient place to initialize this
         * variable.
         */
                    vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

        /*
         * Here we deal with register-level access locks.    This
         * is a generic VGA protection; most SVGA chipsets have
         * similar register locks for their extended registers
         * as well.
         */
                    /* Unprotect CRTC[0-7] */
                    outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
                    outb(vgaIOBase + 5, temp & 0x7F);

		    /* Enters Setup Mode */
/*		    outb(0x46E8, inb(0x46E8) | 16); */
		    /* Extension registers access enable */
/*		    outb(0x103, inb(0x103) | 0x80); */
            }
        else
            {
        /*
         * Here undo what was done above.
         */
		    /* Exits Setup Mode */
/*		    outb(0x46E8, inb(0x46E8) & 0xEF); */
		    /* Extension registers access disable */
/*		    outb(0x103, inb(0x103) & 0x7F); */

                    /* Protect CRTC[0-7] */
                    outb(vgaIOBase + 4, 0x11); temp = inb(vgaIOBase + 5);
                    outb(vgaIOBase + 5, (temp & 0x7F) | 0x80);

		xf86DisableIOPorts(vga256InfoRec.scrnIndex);
            }
}

/*
 * CHIPSRestore --
 *
 * This function restores a video mode.  It basically writes out all of
 * the registers that have previously been saved in the vgaCHIPSRec data 
 * structure.
 *
 * Note that "Restore" is a little bit incorrect.    This function is also
 * used when the server enters/changes video modes.  The mode definitions 
 * have previously been initialized by the Init() function, below.
 */
static void 
CHIPSRestore(restore)
vgaCHIPSPtr restore;
{
    int i;

#ifdef DEBUG    
    ErrorF("CHIPSRestore\n");
#endif

    /*
     * Whatever code is needed to get things back to bank zero should be
     * placed here.  Things should be in the same state as when the
     * Save/Init was done.
     */

    outw(0x3D6, 0x10);
    outw(0x3D6, 0x11);

    /*
     * This function handles restoring the generic VGA registers.
     */
    vgaHWRestore((vgaHWPtr)restore);

    /*
     * Code to restore any SVGA registers that have been saved/modified
     * goes here.    Note that it is allowable, and often correct, to 
     * only modify certain bits in a register by a read/modify/write cycle.
     *
     * A special case - when using an external clock-setting program,
     * this function must not change bits associated with the clock
     * selection.    This condition can be checked by the condition:
     *
     *  if (restore->std.NoClock >= 0)
     *      restore clock-select bits.
     */

    for (i=0; i<0x80; i++) {
        outb(0x3D6, i);
        outb(0x3D7, restore->Port_3D6[i]);
	
#ifdef DEBUG
	ErrorF("XR%X - %X\n",i,restore->Port_3D6[i]);
#endif
    }

#ifdef DEBUG
    ErrorF("restore clock %d\n",restore->std.NoClock);
#endif
    /* set the clock */
    if (restore->std.NoClock)
      CHIPSClockSelect(restore->std.NoClock);

    /* debug - dump out all the extended registers... */
#ifdef DEBUG
    for (i=0; i<0x80; i++) {
        outb(0x3D6, i);
	ErrorF"XR%X - %X\n",i,inb(0x3D7));
    }
#endif

    outw(0x3C4, 0x0300); /* now reenable the timing sequencer */
}

/*
 * CHIPSSave --
 *
 * This function saves the video state.  It reads all of the SVGA registers
 * into the vgaCHIPSRec data structure.  There is in general no need to
 * mask out bits here - just read the registers.
 */
static void *
CHIPSSave(save)
vgaCHIPSPtr save;
{
    int i;

#ifdef DEBUG
    ErrorF("CHIPSSave\n");
#endif

    /*
     * Whatever code is needed to get back to bank zero goes here.
     */

    outw(0x3D6, 0x10);
    outw(0x3D6, 0x11);

    /*
     * This function will handle creating the data structure and filling
     * in the generic VGA portion.
     */
    save = (vgaCHIPSPtr)vgaHWSave((vgaHWPtr)save, sizeof(vgaCHIPSRec));

    /*
     * The port I/O code necessary to read in the extended registers 
     * into the fields of the vgaCHIPSRec structure goes here.
     */
    for (i=0; i<0x80; i++) {
        outb(0x3D6, i);
        save->Port_3D6[i] = inb(0x3D7);
    }

        return ((void *) save);
}

/*
 * CHIPSInit --
 *
 * This is the most important function (after the Probe) function.  This
 * function fills in the vgaCHIPSRec with all of the register values needed
 * to enable either a 256-color mode (for the color server) or a 16-color
 * mode (for the monochrome server).
 *
 * The 'mode' parameter describes the video mode.    The 'mode' structure 
 * as well as the 'vga256InfoRec' structure can be dereferenced for
 * information that is needed to initialize the mode.    The 'new' macro
 * (see definition above) is used to simply fill in the structure.
 */
static Bool
CHIPSInit(mode)
DisplayModePtr mode;
{
    unsigned char tmp;
    int i;

#ifdef DEBUG
    ErrorF("CHIPSInit\n");
#endif

    /*
     * This will allocate the datastructure and initialize all of the
     * generic VGA registers.
     */

    if (!vgaHWInit(mode,sizeof(vgaCHIPSRec))) {
	ErrorF("bomb 1\n");
        return(FALSE);
    }

	/*
	 * Here all of the other fields of 'new' get filled in, to
	 * handle the SVGA extended registers.  It is also allowable
	 * to override generic registers whenever necessary.
	 *
	 * A special case - when using an external clock-setting program,
	 * this function must not change bits associated with the clock
	 * selection.  This condition can be checked by the condition:
	 *
	 *	if (new->std.NoClock >= 0)
	 *		initialize clock-select bits.
	 */

    new->std.Attribute[0x10] = 0x01; /* mode */
    new->std.Attribute[0x11] = 0x00; /* overscan (border) color */
    new->std.Attribute[0x12] = 0x0F; /* enable all color planes */
    new->std.Attribute[0x13] = 0x00; /* horiz pixel panning 0 */

    new->std.Graphics[0x05] = 0x00;  /* normal read/write mode */

#if 0
    new->std.CRTC[0x13] = 0x50;	     /* the display buffer width */
#else
    new->std.CRTC[0x13] <<= 1; 	/* double the width of the buffer */
#endif


    /*
     *   C&T Specific Registers
     */
    for (i=0; i<0x80; i++) {
        outb(0x3D6, i);
        new->Port_3D6[i] = inb(0x3D7);
    }

    new->Port_3D6[0x04]|= 4; /* enable addr counter bits 16-17 */
/*    new->Port_3D6[0x04] = 0xAD; */
    new->Port_3D6[0x0B] = 0x07; /* extended mode, dual pages enabled */ 
    new->Port_3D6[0x10] = 0;
    new->Port_3D6[0x11] = 0;
    new->Port_3D6[0x28] = 0x12;  /* 256-color video */
/*    new->Port_3D6[0x2D] = 0x58; */
/*    new->Port_3D6[0x2E] = 0x58; */
/*    new->Port_3D6[0x55] = 0xE4; */
/*     new->Port_3D6[0x57] = 0x07; */

    /* MH - new ones */
/*    new->Port_3D6[0x0F] = 0x11; */
/*    new->Port_3D6[0x63] = 0x41; */
/*    new->Port_3D6[0x6C] = 0x00; */
/*    new->Port_3D6[0x70] = 0x80; */
 
    return(TRUE);
}

/*
 * CHIPSAdjust --
 *
 * This function is used to initialize the SVGA Start Address - the first
 * displayed location in the video memory.  This is used to implement the
 * virtual window.
 */
static void 
CHIPSAdjust(x, y)
int x, y;
{
    /*
     * The calculation for Base works as follows:
     *
     *  (y * virtX) + x ==> the linear starting pixel
     *
     * This number is divided by 8 for the monochrome server, because
     * there are 8 pixels per byte.
     *
     * For the color server, it's a bit more complex.    There is 1 pixel
     * per byte.    In general, the 256-color modes are in word-mode 
     * (16-bit words).  Word-mode vs byte-mode is will vary based on
     * the chipset - refer to the chipset databook.  So the pixel address 
     * must be divided by 2 to get a word address.  In 256-color modes, 
     * the 4 planes are interleaved (i.e. pixels 0,3,7, etc are adjacent 
     * on plane 0). The starting address needs to be as an offset into 
     * plane 0, so the Base address is divided by 4.
     *
     * So:
     *      Monochrome: Base is divided by 8
     *      Color:
     *  if in word mode, Base is divided by 8
     *  if in byte mode, Base is divided by 4
     *
     * The generic VGA only supports 16 bits for the Starting Address.
     * But this is not enough for the extended memory.  SVGA chipsets
     * will have additional bits in their extended registers, which
     * must also be set.
     */

    /* MH - looks like we are in byte mode.... */
    /* int Base = (y * vga256InfoRec.virtualX + x) >> 3; */
    int Base = (y * vga256InfoRec.virtualX + x) >> 2;

#ifdef DEBUG
    ErrorF("CHIPSAdjust\n");
#endif

    /*
     * These are the generic starting address registers.
     */
    outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
    outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);

    /*
     * Here the high-order bits are masked and shifted, and put into
     * the appropriate extended registers.
     */

    /* MH - plug in the high order starting address bits */
    outb(0x3D6,0x0C);
    outb(0x3D7, ((Base &0xFF0000) >> 16));

}

/*
 * CHIPSValidMode --
 *
 */
static Bool
CHIPSValidMode(mode)
DisplayModePtr mode;
{
return TRUE;
}


/*
 * CHIPSSaveScreen --
 *
 * This function gets called before and after a synchronous reset is
 * performed on the SVGA chipset during a mode-changing operation.  Some
 * chipsets will reset registers that should not be changed during this.
 * If your function is one of these, then you can use this function to
 * save and restore the registers.
 *
 * Most chipsets do not require this function, and instead put
 * 'vgaHWSaveScreen' in the vgaVideoChipRec structure.
 */
#if 0
static void
STUBSaveScreen(mode)
int mode;
{
	if (mode == SS_START)
	{
		/*
		 * Save an registers that will be destroyed by the reset
		 * into static variables.
		 */

		/*
		 * Start sequencer reset.
		 */
		outw(0x3c4, 0x0100);
	}
	else
	{
		/*
		 * End sequencer reset.
		 */
		outw(0x3c4, 0x0300);

		/*
		 * Now restore those registers.
		 */
	}
}
#endif

/*
 * CHIPSGetMode --
 *
 * This function will read the current SVGA register settings and produce
 * a filled-in DisplayModeRec containing the current mode.
 *
 * Note that the is function is NOT used in XFree86 1.3, hence in a real
 * driver you should put 'NoopDDA' in the vgaVideoChipRec structure.    At
 * some point in the future, this function will be used to implement
 * interactive mode setting, and drivers will be required to supply it.
 */
#if 0
static void
CHIPSGetMode(mode)
DisplayModePtr mode;
{
#ifdef DEBUG
     fprintf(stderr,"CHIPSGetMode\n");
#endif

    /*
     * Fill in the 'mode' stucture based on current register settings.
     */
}
#endif

/*
 * CHIPSFbInit --
 *
 * This function is used to initialise chip-specific graphics functions.
 * It can be used to make use of the accelerated features of some chipsets.
 * For most drivers, this function is not required, and 'NoopDDA' is put
 * in the vgaVideoChipRec structure.
 */
#if 0
static void
CHIPSFbInit()
{

	/*
	 * Fill in the fields of cfbLowlevFuncs for which there are
	 * accelerated versions.  This struct is defined in
	 * xc/programs/Xserver/hw/xfree86/vga256/cfb.banked/cfbfuncs.h.
	 */
	cfbLowlevFuncs.fillRectSolidCopy = CHIPSFillRectSolidCopy;
	cfbLowlevFuncs.doBitbltCopy = CHIPSDoBitbltCopy;

	/*
	 * Some functions (eg, line drawing) are initialised via the
	 * cfbTEOps, cfbTEOps1Rect, cfbNonTEOps, cfbNonTEOps1Rect
	 * structs as well as in cfbLowlevFuncs.  These are of type
	 * 'struct GCFuncs' which is defined in mit/server/include/gcstruct.h.
	 */
	cfbLowlevFuncs.lineSS = CHIPSLineSS;
	cfbTEOps1Rect.Polylines = CHIPSLineSS;
	cfbTEOps.Polylines = CHIPSLineSS;
	cfbNonTEOps1Rect.Polylines = CHIPSLineSS;
	cfbNonTEOps.Polylines = CHIPSLineSS;

	/*
	 * If hardware cursor is supported, the vgaHWCursor struct should
	 * be filled in here.
	 */
	vgaHWCursor.Initialized = TRUE;
	vgaHWCursor.Init = CHIPSCursorInit;
	vgaHWCursor.Restore = CHIPSCursorRestore;
	vgaHWCursor.Warp = CHIPSCursorWarp;
	vgaHWCursor.QueryBestSize = CHIPSQueryBestSize;
	
}
#endif

