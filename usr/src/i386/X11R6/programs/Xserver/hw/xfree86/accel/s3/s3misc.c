/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3/s3misc.c,v 3.39 1996/01/26 09:09:41 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Thomas Roell makes no
 * representations about the suitability of this software for any purpose. It
 * is provided "as is" without express or implied warranty.
 * 
 * THOMAS ROELL AND KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THOMAS ROELL OR KEVIN E. MARTIN BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 * 
 * Rewritten for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
 * 
 * Header: /home/src/xfree86/mit/server/ddx/xf86/accel/s3/RCS/s3.c,v 2.0
 * 1993/02/22 05:58:13 jon Exp
 * 
 * Modified by Amancio Hasty and Jon Tombs
 * 
 */
/* $XConsortium: s3misc.c /main/19 1996/01/26 14:26:42 kaleb $ */


#include "cfb.h"
#include "cfb16.h"
#include "cfb32.h"
#include "pixmapstr.h"
#include "fontstruct.h"
#include "s3.h"
#include "regs3.h"
#include "xf86_HWlib.h"
#include "ICD2061A.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "s3linear.h"

#ifdef XFreeXDGA
#include "X.h"
#include "Xproto.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef PC98
#include "s3pc98.h"
#endif
 
extern char s3Mbanks;
extern Bool s3Mmio928;
extern unsigned long s3MemBase;

extern miPointerScreenFuncRec xf86PointerScreenFuncs;

static Bool s3TryAddress();
extern ScreenPtr s3savepScreen;
static PixmapPtr ppix = NULL;
extern Bool  s3Localbus;
extern Bool  s3VLB;
extern Bool  s3LinearAperture;
extern int s3BankSize;
extern int s3DisplayWidth;
extern pointer vgaBase;
extern pointer vgaBaseLow;
extern pointer vgaBaseHigh;
extern pointer s3VideoMem;
extern unsigned char s3Port59;
extern unsigned char s3Port5A;
extern unsigned char s3Port31;
extern Bool s3PowerSaver;

extern Bool xf86Exiting, xf86Resetting, xf86ProbeFailed;
extern int xf86Verbose;

static Bool AlreadyInited = FALSE;
static Bool s3ModeSwitched = FALSE;

#ifdef PC98
extern int pc98BoardType;
#endif

/*
 * s3Initialize -- Attempt to find and initialize a VGA framebuffer Most of
 * the elements of the ScreenRec are filled in.  The video is enabled for the
 * frame buffer...
 */

Bool
s3Initialize(scr_index, pScreen, argc, argv)
     int   scr_index;		/* The index of pScreen in the ScreenInfo */
     ScreenPtr pScreen;		/* The Screen to initialize */
     int   argc;		/* The number of the Server's arguments. */
     char **argv;		/* The arguments themselves. Don't change! */
{
   int displayResolution = 75;	/* default to 75dpi */
   extern int monitorResolution;
   int i;
   unsigned char *pat;

   s3Unlock();	  /* for restarts */
   
   /* for clips */
   s3ScissB = ((s3InfoRec.videoRam * 1024) / s3BppDisplayWidth) - 1;

 /*
  * Initialize the screen, saving the original state for Save/Restore
  * function
  */

   if (serverGeneration == 1) {
      unsigned long addr = 0;
      s3BankSize = 0x10000;
      s3LinApOpt = 0x14;

      if (OFLG_ISSET(OPTION_STB_PEGASUS, &s3InfoRec.options) &&
	  !OFLG_ISSET(OPTION_NOLINEAR_MODE, &s3InfoRec.options) &&
	  s3Mmio928) {
	vgaBaseLow = xf86MapVidMem(scr_index, VGA_REGION,
				   (pointer)0xA0000, s3BankSize);
	vgaBaseHigh = xf86MapVidMem(scr_index, EXTENDED_REGION,
				    (pointer)0x7C0A0000, s3BankSize);
	vgaBase = vgaBaseLow;
      } else {
	/* First, map the vga window -- it is always required */
#ifndef PC98
	vgaBase = xf86MapVidMem(scr_index, VGA_REGION, (pointer)0xA0000,
				s3BankSize);
#else
      switch(pc98BoardType & 0xf0 ){
      case PW:
      case PW805I:
            vgaBase = xf86MapVidMem(scr_index, VGA_REGION, 
                                      (pointer)(PW_WinAdd << 16), s3BankSize);
            ErrorF("   PC98: PW mem-access\n");
	    break;
      case PCSKB:
      case PCHKB:
            vgaBase = xf86MapVidMem(scr_index, VGA_REGION,
                                      (pointer)(XKB_WinAdd << 16), s3BankSize);
            ErrorF("   PC98: SKB,HKB mem-access\n");
	    break;
      case PCSKB4:
            vgaBase = xf86MapVidMem(scr_index, VGA_REGION,
                                      (pointer)(SKB4_WinAdd << 16), s3BankSize);
            ErrorF("   PC98: SKB4 mem-access(not work,yet)\n");
	    break;
      case NECWAB:
            vgaBase = xf86MapVidMem(scr_index, VGA_REGION,
                                      (pointer)(NEC_WinAdd << 16), s3BankSize);
            ErrorF("   PC98: NEC-WAB(C Bus) mem-access\n");
	    break;
      case PWLB:
            vgaBase = xf86MapVidMem(scr_index, LINEAR_REGION,
                                      (pointer)((PWLB_WinAdd << 16) + 0xA0000),
							s3BankSize);
            ErrorF("   PC98: PW localbus mem-access\n");
	    break;
      default: 
            vgaBase = xf86MapVidMem(scr_index, VGA_REGION, 
				      (pointer)0xA0000,s3BankSize);
      }
#endif /* PC98 */
      }

      s3Init(s3InfoRec.modes);

      if (xf86LinearVidMem() &&
	  !OFLG_ISSET(OPTION_NOLINEAR_MODE, &s3InfoRec.options)) {
	 /* Now, see if we can map a high buffer */
	 if (s3Localbus && !S3_924_ONLY(s3ChipId) &&
	     !OFLG_ISSET(OPTION_NO_MEM_ACCESS, &s3InfoRec.options)) {
	    long i;
	    long *poker;
	    unsigned long pVal;
	    unsigned char reg53tmp = 0;
	    Bool CachedFrameBuffer = FALSE;

	    pVal = 0x12345678;

	    s3InitEnvironment();
	    s3ImageWriteNoMem(0, 0, 4 / s3Bpp, 1, (char *) &pVal, 4 / s3Bpp,
			      0, 0, (short) s3alu[GXcopy], ~0);	 

	    if (S3_801_928_SERIES (s3ChipId)) {
	       outb(vgaCRIndex, 0x59);
	       if (S3_x64_SERIES(s3ChipId)) 
		  if (s3InfoRec.MemBase != 0) 
		     outb(vgaCRReg, s3InfoRec.MemBase >> 24);
		  else if (s3MemBase != 0)
		     outb(vgaCRReg, s3MemBase >> 24);
		  else
		     outb(vgaCRReg, 0xf3);
	       else
#ifdef PC98_PWLB
		if (pc98BoardType == PWLB)
		  outb(vgaCRReg, 0x00);
		else
#endif
		  outb(vgaCRReg, 0x03);

	       outb(vgaCRIndex, 0x5a);
	       if (S3_x64_SERIES(s3ChipId)) 
		  if (s3InfoRec.MemBase != 0) 
		     outb(vgaCRReg, (s3InfoRec.MemBase >> 16) & 0x80);
		  else if (s3MemBase != 0)
		     outb(vgaCRReg, (s3MemBase >> 16) & 0x80);
		  else
		     outb(vgaCRReg, 0x00);
	       else
#ifdef PC98_PWLB
	       if (pc98BoardType == PWLB)
		 outb(vgaCRReg, 0x40);
	       else
#endif
	          outb(vgaCRReg, 0x00);
	       outb (vgaCRIndex, 0x58);
	       if (s3InfoRec.videoRam <= 1024) {
		  s3LinApOpt=0x15;
	       } else if (s3InfoRec.videoRam <= 2048) {
		  s3LinApOpt=0x16;
	       } else {
		  s3LinApOpt=0x17;
	       }
       	       s3BankSize = s3InfoRec.videoRam * 1024;
	       s3EnableLinear();
	    }
	 
	    /*
	     * XXXX This is for debugging only.  It attempts to find
	     * which values of LAW are decoded (see s3TryAddress() below).
	     */
	    if (OFLG_ISSET(OPTION_FB_DEBUG, &s3InfoRec.options)) {
	       for (i = 0xff; i >= 3; i--) { /* 4080Mb..48Mb stepsize 16Mb */
		  addr = (i << 24);

	          s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
				          (pointer)addr, 4096);
	          poker = (long *) s3VideoMem; 
	          s3TryAddress(poker, pVal, addr, 0);
	          xf86UnMapVidMem(scr_index, LINEAR_REGION, s3VideoMem, 4096);
	       }
	    }
		  
	    /*
	     * If a MemBase value was given in the XF86Config, skip the LAW
	     * probe and use the high 10 bits for the hw part of LAW.
	     * Normally only 6 bits are set in hw, but the Diamond Stealth
	     * Pro is different.
	     */
	    if (s3InfoRec.MemBase != 0) {
	       addr = (s3InfoRec.MemBase & 0xffc00000);
	       s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
					  (pointer)addr, s3BankSize);
	       s3DisableLinear();
	       outb(vgaCRIndex, 0x5a);
	       if (S3_x64_SERIES(s3ChipId)) {
		  outb(vgaCRReg, (addr >> 16) & 0xff);
		  outb(vgaCRIndex, 0x59);
		  outb(vgaCRReg, (addr >> 24) & 0xff);
	       }
	       else 
		  outb(vgaCRReg, 0xc0);
	       s3EnableLinear();

	       s3LinearAperture = TRUE;
	       ErrorF("%s %s: Local bus LAW is 0x%03lXxxxxx\n", 
		      XCONFIG_GIVEN, s3InfoRec.name, (addr >> 20));
	    } else {
	       if (S3_x64_SERIES(s3ChipId) && 
		   !(S3_TRIO64_SERIES(s3ChipId) &&
		     /*s3InfoRec.videoRam > 1024 &&*/ s3VLB)) {
		  /* So far, only tested for the PCI ELSA W2000Pro */
		  s3DisableLinear();
	          outb(vgaCRIndex, 0x59);
	          addr = inb(vgaCRReg) << 8;
	          outb(vgaCRIndex, 0x5a);
	          addr |= inb(vgaCRReg);
	          addr <<= 16;
		  s3EnableLinear();

                  if (OFLG_ISSET(OPTION_FB_DEBUG, &s3InfoRec.options)) {
		     ErrorF("Read LAW as 0x%08X \n", addr);
	          }
		  s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
                                             (pointer)addr, s3BankSize);
		  poker = (long *) s3VideoMem;
		  if (s3TryAddress(poker, pVal, addr, 1)) {
		     s3LinearAperture = TRUE;
		     if (xf86Verbose) {
			ErrorF("%s %s: Local bus LAW is 0x%08lX\n", 
			       XCONFIG_PROBED, s3InfoRec.name, addr);
		     }
		  } else {
		     s3LinearAperture = FALSE;
		     CachedFrameBuffer = TRUE;
		     if (xf86Verbose) {
			ErrorF("%s %s: Local bus LAW is 0x%08lX %s\n", 
			       XCONFIG_PROBED, s3InfoRec.name, addr,
			       "but linear fb not usable");
		     }
		     xf86UnMapVidMem(scr_index, LINEAR_REGION, s3VideoMem,
				     s3BankSize);
		  }
	       } else {
#ifdef PC98_PWLB
		 if (pc98BoardType == PWLB) {
		     unsigned long addr = (PWLB_WinAdd << 16) + 0x400000;

		     s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
						(pointer)addr, s3BankSize);
		     poker = (long *) s3VideoMem; 

		     if (s3TryAddress(poker, pVal, addr, 1)) {
			ErrorF("%s %s: Local bus LAW is 0x%08X\n", 
			     XCONFIG_PROBED, s3InfoRec.name, addr);
			s3LinearAperture = TRUE;
		     }
		 } else {
#endif
	          for (i = 0xff; i >= 3; i--) { /* 4080Mb..48Mb stepsize 16Mb */
		     addr = (i << 24);

		     s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
						(pointer)addr, s3BankSize);
		     poker = (long *) s3VideoMem; 

		     if (s3TryAddress(poker, pVal, addr, 1)) {
			/* We found some ram, but is it ours? */
	       
			s3DisableLinear();
			/* move it up by 12MB */
			outb(vgaCRIndex, 0x5a);
			outb(vgaCRReg, 0xC0);
			s3EnableLinear();

			if (!s3TryAddress(poker, pVal, addr, 2)) {
			   addr += (0x0C<<20);
			   xf86UnMapVidMem(scr_index, LINEAR_REGION,
					   s3VideoMem, s3BankSize);
			   s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
						      (pointer)addr,
						      s3BankSize);
		     
			   if (s3TryAddress((long *)s3VideoMem, pVal,
					    addr, 3)) {
			      ErrorF("%s %s: Local bus LAW is 0x%03Xxxxxx\n", 
				     XCONFIG_PROBED, s3InfoRec.name,
				     (i << 4 | 0x0c));
			      s3LinearAperture = TRUE;
			      break;
			   }
			   /*
			    * Special test for Diamond Stealth Pro.
			    * On this card, bits 6 and 7 of register 0x5a
			    * control address bits A29 and A26 rather than
			    * A22 and A23. Don't ask we why.
			    */
			   addr = (i << 24) | 0x24000000;
			   xf86UnMapVidMem(scr_index, LINEAR_REGION,
					   s3VideoMem, s3BankSize);
			   s3VideoMem = xf86MapVidMem(scr_index, LINEAR_REGION,
						      (pointer)addr,
						      s3BankSize);
			   if (s3TryAddress((long *)s3VideoMem, pVal,
					    addr, 4)) {
			      ErrorF("%s %s: Local bus LAW is 0x%03Xxxxxx\n", 
				     XCONFIG_PROBED, s3InfoRec.name,
				     (i | 0x24) << 4);
			      s3LinearAperture = TRUE;
			      break;
			   }
			} else {
			   ErrorF("%s %s: linear framebuffer found, but",
				  XCONFIG_PROBED, s3InfoRec.name);
			   ErrorF(" it appears to be in a cachable\n");
			   ErrorF("\t address range.\n");
			   CachedFrameBuffer = TRUE;
			}
			s3DisableLinear();
			outb(vgaCRIndex, 0x5a);
			outb(vgaCRReg, 0x00);	/* reset for next probe */
			s3EnableLinear();
		     }
		     xf86UnMapVidMem(scr_index, LINEAR_REGION, s3VideoMem,
				     s3BankSize);
		  }
#ifdef PC98_PWLB
		 }
#endif
	       }
	    }

            /* The UnMap can unmap all mapped regions, so make sure vgaBase is
	       still mapped */
	    xf86MapDisplay(scr_index, VGA_REGION);

	    if (!s3LinearAperture) {
	       ErrorF("%s %s: %s local bus framebuffer -",
		      XCONFIG_PROBED, s3InfoRec.name,
	              (CachedFrameBuffer? "Can't use": "Couldn't find"));
	       ErrorF(" linear access disabled\n");
	       s3BankSize = 0x10000;
	       s3VideoMem = NULL;
	       addr = 0xA0000;
	    }
            s3DisableLinear();
         }
      }

      /* No linear mapping */
      if (!s3VideoMem) {
	 s3VideoMem = vgaBase;
	 addr = 0xA0000;
	 /* If using VGA aperture, set it up */
	 if (s3BankSize == 0x10000) {
#ifndef PC98
	    outb(vgaCRIndex, 0x59);
	    outb(vgaCRReg, 0x00);
	    outb(vgaCRIndex, 0x5a);
	    outb(vgaCRReg, 0x0a);
#else
	    switch( pc98BoardType & 0xf0 ){
		case PCSKB4:
		    outb(vgaCRIndex, 0x59);
		    outb(vgaCRReg, SKB4_WinAdd >> 8);
		    outb(vgaCRIndex, 0x5a);
		    outb(vgaCRReg, SKB4_WinAdd & 0xff);
		    break;
		case NECWAB:
		    outb(vgaCRIndex, 0x59);
		    outb(vgaCRReg, 0x00);
		    outb(vgaCRIndex, 0x5a);
		    outb(vgaCRReg, NEC_WinAdd );
		    break;
		default:
		    outb(vgaCRIndex, 0x59);
		    outb(vgaCRReg, 0x00);
		    outb(vgaCRIndex, 0x5a);
		    outb(vgaCRReg, 0x0a);
	    }
#endif
	    s3LinApOpt = 0x14;
	 }
      }

#ifdef XFreeXDGA
      s3InfoRec.physBase = addr;
      s3InfoRec.physSize = s3BankSize;
#endif

        
      /* Save CR59, CR5A for future calls to s3Init() */
      outb(vgaCRIndex, 0x59);
      s3Port59 = inb(vgaCRReg);
      outb(vgaCRIndex, 0x5A);
      s3Port5A = inb(vgaCRReg);
   } else
      s3Init(s3InfoRec.modes);

   s3InitEnvironment();
   AlreadyInited = TRUE;

   s3ImageInit();


   /* now testing Trio32 BITBLT bug 
    * using a 2*2 pixel pattern BLT from (0,0) to (0,2) */

   pat = (unsigned char*) Xcalloc(s3Bpp);
   
   /* init source pattern:   2*2 pixel checker pattern (glyph source) */
   pat[0 * s3Bpp] = pat[3 * s3Bpp] = 0;
   pat[1 * s3Bpp] = pat[2 * s3Bpp] = 1;
   (*s3ImageWriteFunc)(0, 0, 2, 2, (char*)pat, 2 * s3Bpp, 
		       0, 0, (short) s3alu[GXcopy], ~0);
   
   /* first test original BITBLT; then workaround */
   for (s3Trio32FCBug = 0; s3Trio32FCBug < 2; s3Trio32FCBug++) {

      /* init destination (all bits set) */
      for(i = 0; i < s3Bpp * 2 * 2; i++) 
	 pat[i] = (char)0xff;
      s3ImageWriteNoMem(0, 2, 2, 2, (char*)pat, 2 * s3Bpp,
			0, 0, (short) s3alu[GXcopy], ~0);

      BLOCK_CURSOR;

      if (s3Trio32FCBug) {
	 WaitQueue16_32(2, 3);
	 S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_OR);
	 S3_OUTW32(BKGD_COLOR, 0);
      } else {
	 WaitQueue(1);
	 S3_OUTW(BKGD_MIX, BSS_BKGDCOL | MIX_DST);
      }

      WaitQueue16_32(3, 6);
      S3_OUTW32(FRGD_COLOR, 1);
      S3_OUTW32(RD_MASK, 1);
      S3_OUTW32(WRT_MASK, ~0);

      WaitQueue(4);
      S3_OUTW(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_EXPBLT | COLCMPOP_F);
      S3_OUTW(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
      S3_OUTW(CUR_X, 0);
      S3_OUTW(CUR_Y, 0);
	 
      WaitQueue(5);
      S3_OUTW(DESTX_DIASTP, 0);
      S3_OUTW(DESTY_AXSTP, 2);
      S3_OUTW(MAJ_AXIS_PCNT, 1);
      S3_OUTW(MULTIFUNC_CNTL, MIN_AXIS_PCNT | 1);

      S3_OUTW(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

      /* read back the destination */
      WaitIdle();
      (*s3ImageReadFunc)(0, 2, 2, 2, (char*)pat, 2 * s3Bpp, 0, 0, ~0);

#ifdef DEBUG_TRI32_BITBLT_TEST
      for(i = 0; i < s3Bpp * 2 * 2; i++)
	 ErrorF("%s %s: BITBLT %2d %02x\n",
		XCONFIG_PROBED, s3InfoRec.name, i, pat[i]&0xff);
      ErrorF("\n");
#endif
      if (pat[0] == 0xff && pat[3 * s3Bpp] == 0xff)
	 break;  /* BITBLT worked */
   }
   if (s3Trio32FCBug > 1) {
      ErrorF("%s %s: WARNING: BITBLT malfunction --\n"
	     "\tplease report to XFree86@XFree86.Org\n",
	     XCONFIG_PROBED, s3InfoRec.name);
   }
   else if (s3Trio32FCBug) {
      ErrorF("%s %s: BITBLT malfunction, using workaround code\n",
	     XCONFIG_PROBED, s3InfoRec.name);
   }
   xfree(pat);

   /* restore read mask */
   WaitQueue16_32(1, 2);
   S3_OUTW32(RD_MASK, 0);

   xf86InitCache(s3CacheMoveBlock);
   s3FontCache8Init();

   /*
    * Take display resolution from the -dpi flag if specified
    */

   if (monitorResolution)
      displayResolution = monitorResolution;

   if (!s3ScreenInit(pScreen,
		     (pointer) s3VideoMem,
		     s3InfoRec.virtualX, s3InfoRec.virtualY,
		     displayResolution, displayResolution,
		     s3DisplayWidth))
      return (FALSE);

   pScreen->CloseScreen = s3CloseScreen;
   pScreen->SaveScreen = s3SaveScreen;


#if 0
	miDCInitialize (pScreen, &xf86PointerScreenFuncs);
#else
 	switch (s3InfoRec.bitsPerPixel) {
	case 8:
	    pScreen->InstallColormap = s3InstallColormap;
	    pScreen->UninstallColormap = s3UninstallColormap;
	    pScreen->ListInstalledColormaps = s3ListInstalledColormaps;
	    pScreen->StoreColors = s3StoreColors;
	    break;       
	case 16:
        case 32:
	    pScreen->InstallColormap = cfbInstallColormap;
	    pScreen->UninstallColormap = cfbUninstallColormap;
	    pScreen->ListInstalledColormaps = cfbListInstalledColormaps;
	    pScreen->StoreColors = (void (*)())NoopDDA;
	}
	pScreen->QueryBestSize = s3QueryBestSize;
	xf86PointerScreenFuncs.WarpCursor = s3WarpCursor;
	(void)s3CursorInit(0, pScreen);
#endif
   s3savepScreen = pScreen;
   return (cfbCreateDefColormap(pScreen));

}


/*
 * s3EnterLeaveVT -- grab/ungrab the current VT completely.
 */

void
s3EnterLeaveVT(enter, screen_idx)
     Bool  enter;
     int screen_idx;
{
   PixmapPtr pspix;
   ScreenPtr pScreen = s3savepScreen;

   if (!xf86Exiting && !xf86Resetting) {
      /* cfbGetScreenPixmap(pScreen) */
        switch (s3InfoRec.bitsPerPixel) {
        case 8:
            pspix = (PixmapPtr)pScreen->devPrivate;
            break;
        case 16:
            pspix =
                  (PixmapPtr)pScreen->devPrivates[cfb16ScreenPrivateIndex].ptr;
            break;
        case 32:
            pspix =
                  (PixmapPtr)pScreen->devPrivates[cfb32ScreenPrivateIndex].ptr;
            break;
	}
   }

   if (enter) {
      xf86MapDisplay(screen_idx, VGA_REGION);
      if (s3VideoMem != vgaBase)
	 xf86MapDisplay(screen_idx, LINEAR_REGION);

      if (!xf86Resetting) {
	 ScrnInfoPtr pScr = XF86SCRNINFO(pScreen);

         s3Unlock();
	 s3Init(s3InfoRec.modes);
         s3InitEnvironment();
	 AlreadyInited = TRUE;
	 s3RestoreDACvalues();
	 s3ImageInit();
   	 s3FontCache8Init();
	 s3RestoreColor0(pScreen);
         (void)s3CursorInit(0, pScreen);
	 s3RestoreCursor(pScreen);
	 s3AdjustFrame(pScr->frameX0, pScr->frameY0);


	 if ((pointer)pspix->devPrivate.ptr != s3VideoMem && ppix) {
	    pspix->devPrivate.ptr = s3VideoMem;
	    (*s3ImageWriteFunc)(0, 0, pScreen->width, pScreen->height,
			        ppix->devPrivate.ptr,
			        PixmapBytePad(s3DisplayWidth, 
					      pScreen->rootDepth),
			        0, 0, s3alu[GXcopy], ~0);
	 }
      }
      if (ppix) {
	 (pScreen->DestroyPixmap)(ppix);
	 ppix = NULL;
      }
   } else {
      /* Make sure that another driver hasn't disabled IO */
      xf86MapDisplay(screen_idx, VGA_REGION);
      if (s3VideoMem != vgaBase)
	 xf86MapDisplay(screen_idx, LINEAR_REGION);

      if (!xf86Exiting) {
	 ppix = (pScreen->CreatePixmap)(pScreen, s3DisplayWidth,
					pScreen->height, pScreen->rootDepth);
	 if (ppix) {
	    (*s3ImageReadFunc)(0, 0, pScreen->width, pScreen->height,
			       ppix->devPrivate.ptr,
			       PixmapBytePad(s3DisplayWidth, 
					     pScreen->rootDepth),
			       0, 0, ~0);
	    pspix->devPrivate.ptr = ppix->devPrivate.ptr;
	 }
      }
#ifdef XFreeXDGA
      if (s3InfoRec.directMode & XF86DGADirectGraphics) {
	s3HideCursor();
      } else
#endif
        if (AlreadyInited) {
	  s3CleanUp();
          AlreadyInited = FALSE;
        }

      xf86UnMapDisplay(screen_idx, VGA_REGION);
      if (s3VideoMem != vgaBase)
         xf86UnMapDisplay(screen_idx, LINEAR_REGION);
#ifdef XFreeXDGA
      if (s3InfoRec.directMode & XF86DGADirectGraphics) {
        /* make sure we are in linear mode */
	s3EnableLinear();
      }
#endif
   }
}

/*
 * s3CloseScreen -- called to ensure video is enabled when server exits.
 */

Bool
s3CloseScreen(screen_idx, pScreen)
     int screen_idx;
     ScreenPtr pScreen;
{

 /*
  * Hmm... The server may shut down even if it is not running on the current
  * vt. Let's catch this case here.
  */
   xf86Exiting = TRUE;
   if (xf86VTSema)
      s3EnterLeaveVT(LEAVE, screen_idx);
   else if (ppix) {
      /*
       * 7-Jan-94 CEG: The server is not running on the current vt.
       * Free the screen snapshot taken when the server vt was left.
       */
      (s3savepScreen->DestroyPixmap)(ppix);
      ppix = NULL;
   }
   return (TRUE);
}

static OsTimerPtr suspendTimer = NULL, offTimer = NULL;
extern CARD32 ScreenSaverTime;

/*
 * s3OffMode -- put the screen into power off mode.
 */

static CARD32
s3OffMode(timer, now, arg)
     OsTimerPtr timer;
     CARD32 now;
     pointer arg;
{
   unsigned char sync;
   Bool on = (Bool)arg;

   if (!s3PowerSaver) return(0);

   if (xf86VTSema) {
      /* the server is running on the current vt */
      /* so just go for it */

      outb(vgaCRIndex, 0x17);
      sync = inb(vgaCRReg);

      if (on) {
	 sync |= 0x80;			/* enable sync   */
      } else {
	 sync &= ~0x80;			/* disable sync */
      }

      usleep(10000);
      outb(vgaCRReg, sync);
   }
   if (offTimer) {
      TimerFree(offTimer);
      offTimer = NULL;
   }
   return(0);
}

/*
 * s3SuspendMode -- put the screen into suspend mode.
 */

static CARD32
s3SuspendMode(timer, now, arg)
     OsTimerPtr timer;
     CARD32 now;
     pointer arg;
{
   unsigned char extsync;
   Bool on = (Bool)arg;

   if (!s3PowerSaver) return(0);

   if (xf86VTSema) {
      /* the server is running on the current vt */
      /* so just go for it */

      /* only supported at the moment for Trio, 864/964 and later */
      if (S3_x64_SERIES(s3ChipId)) {

	 /* We assume that the screen is already blanked */
	 outw(0x3C4, 0x0608);		/* unlock SRD */
	 outb(0x3C4, 0x0D);
	 extsync = inb(0x3C5);

	 if (on) {
	    extsync &= 0x0F;			/* enable both syncs */
	 } else {
	    extsync = (extsync & 0x0F) | 0x10;	/* turn off hsync only */
	 }
	 usleep(10000);
	 outw(0x3C4, (extsync << 8) | 0x0D);
	 outw(0x3C4, 0x0008);		/* lock SRD */
      }
      if (!on && s3InfoRec.offTime != 0) {
	 if (s3InfoRec.offTime > s3InfoRec.suspendTime &&
	     s3InfoRec.offTime > ScreenSaverTime) {

	    int timeout;

	    /* Setup timeout for s3OffMode() */
	    if (s3InfoRec.suspendTime < ScreenSaverTime)
	       timeout = s3InfoRec.offTime - ScreenSaverTime;
	    else
	       timeout = s3InfoRec.offTime - s3InfoRec.suspendTime;

	    offTimer = TimerSet(offTimer, 0, timeout,
			        s3OffMode, (pointer)FALSE);
	 } else {
	    s3OffMode(NULL, 0, (pointer)FALSE);
	 }
      }
   }
   if (suspendTimer) {
      TimerFree(suspendTimer);
      suspendTimer = NULL;
   }
   return(0);
}

/*
 * s3SaveScreen -- blank the screen.
 */

Bool
s3SaveScreen(pScreen, on)
     ScreenPtr pScreen;
     Bool  on;
{
   unsigned char scrn;

   if (on)
      SetTimeSinceLastInputEvent();

   if (xf86VTSema) {
      /* the server is running on the current vt */
      /* so just go for it */

      outb(0x3C4,1);
      scrn = inb(0x3C5);

      if (on) {
	 scrn &= 0xDF;			/* enable screen */
      } else {
	 scrn |= 0x20;			/* blank screen */
      }

      /* Turn off Off and Suspend mode */
      if (s3PowerSaver && on) {
	 s3OffMode(NULL, 0, (pointer)TRUE);
	 s3SuspendMode(NULL, 0, (pointer)TRUE);
      }

      outw(0x3C4, 0x0100);              /* syncronous reset */
      outw(0x3C4, (scrn << 8) | 0x01); /* change mode */
      outw(0x3C4, 0x0300);              /* end reset */

      if (s3PowerSaver && !on) {
	 if (s3InfoRec.suspendTime != 0) {
	    if (s3InfoRec.suspendTime > ScreenSaverTime) {
	       suspendTimer = TimerSet(suspendTimer, 0,
				       s3InfoRec.suspendTime - ScreenSaverTime,
				       s3SuspendMode, (pointer)FALSE);
	    } else {
	      s3SuspendMode(NULL, 0, (pointer)FALSE);
	    }
	 } else if (s3InfoRec.offTime != 0) {
	    if (s3InfoRec.offTime > ScreenSaverTime) {
	       offTimer = TimerSet(offTimer, 0,
				   s3InfoRec.offTime - ScreenSaverTime,
				   s3OffMode, (pointer)FALSE);
	    } else {
	       s3OffMode(NULL, 0, (pointer)FALSE);
	    }
	 }
      }
   }
   return (TRUE);
}

static debugcache = 0;

Bool
s3SwitchMode(mode)
     DisplayModePtr mode;
{
  WaitIdle();  /* wait till idle */

   if (OFLG_ISSET(OPTION_SHOWCACHE, &s3InfoRec.options)) {
      debugcache++;
      if (debugcache & 1) {
        mode = mode->prev;
        s3InfoRec.modes=mode;
      }
   }

   if (s3Init(mode)) {
      s3ModeSwitched = TRUE;
#if notyet
      s3Restore(vgaNewVideoState);
#endif
      s3ImageInit(); /* mode switching changes the memory bank */
      s3RestoreCursor(s3savepScreen);
      return (TRUE);
   } else {
      ErrorF("Mode switch failed because of hardware initialisation error\n");
      s3ImageInit();
      return (FALSE);
   }
}

void
s3AdjustFrame(int x, int y)
{
   int   Base, origBase;
   unsigned char tmp;
   extern int s3AdjustCursorXPos;  /* for s3Cursor.c */
   extern int s3HDisplay;

   if (OFLG_ISSET(OPTION_SHOWCACHE, &s3InfoRec.options)) {
      if ( debugcache & 1)
         y += 512;
   }
      
   if (x > s3DisplayWidth - s3HDisplay)
      x  = s3DisplayWidth - s3HDisplay;

   /* so may S3 cards have problems with some odd base addresses, 
    * to catch them all only even base values will be used.
    */

   origBase = (y * s3DisplayWidth + x) * s3Bpp;
   Base = (origBase >> 2) & ~1;

   if (S3_964_SERIES(s3ChipId) && DAC_IS_BT485_SERIES) {
      if ((Base & 0x3f) >= 0x3c) 
	 Base = (Base & ~0x3f) | 0x3b;
      else if (s3Bpp>1 && (Base & 0x3f) == 0x3a) 
	 Base = (Base & ~0x3f) | 0x39;
      else if (s3Bpp>2 && (Base & 0x1f) == 0x1a) 
	 Base = (Base & ~0x1f) | 0x19;
   }

   if (S3_964_SERIES(s3ChipId) 
       && (DAC_IS_TI3025 || DAC_IS_TI3026 || DAC_IS_IBMRGB)) {
      int px, py, a;
      miPointerPosition(&px, &py);
      if (s3Bpp==1)
	 a = 4-1;
      else 
	 a = 8-1;
      if (px-x > s3HDisplay/2)
	 Base = ((origBase + a*4) >> 2) & ~1;
      Base &= ~a;
   }

#if 0
   /* wait for vertical retrace */
   while ((inb(vgaIOBase + 0x0A) & 0x08) == 0x00) ;
   while ((inb(vgaIOBase + 0x0A) & 0x08) == 0x08) ;
   while ((inb(vgaIOBase + 0x0A) & 0x08) == 0x00) ;
#endif

   outb(vgaCRIndex, 0x31);
   outb(vgaCRReg, ((Base & 0x030000) >> 12) | s3Port31);
   s3Port51 &= ~0x03;
   s3Port51 |= ((Base & 0x0c0000) >> 18);
   outb(vgaCRIndex, 0x51);
   /* Don't override current bank selection */
   tmp = (inb(vgaCRReg) & ~0x03) | (s3Port51 & 0x03);
   outb(vgaCRReg, tmp);

   outw(vgaCRIndex, (Base & 0x00FF00) | 0x0C);
   outw(vgaCRIndex, ((Base & 0x00FF) << 8) | 0x0D);

   s3AdjustCursorXPos = (origBase - (Base << 2)) / s3Bpp;

   if (s3ModeSwitched) {
      s3ModeSwitched = FALSE;
      s3RestoreCursor(s3savepScreen);
   } else {
      s3RepositionCursor(s3savepScreen);
   }

}

/*
 * Force optimising compilers to read *addr
 */
static Bool
s3TryAddress(addr, value, physaddr, stage)
     long *addr;
     long value;
     long physaddr;
     int stage;
{
   long tmp;

   tmp = *addr;

   if (OFLG_ISSET(OPTION_FB_DEBUG, &s3InfoRec.options)) {
      if (stage == 0 && tmp != 0xFFFFFFFF) {
	 ErrorF("Memory found at 0x%08X (read 0x%08X)\n", physaddr, tmp);
      } else {
	 ErrorF("Stage %d: address = 0x%08X, read 0x%08X\n", stage, physaddr,
		tmp);
      }
   }
   if (tmp == value)
      return TRUE;
   else
      return FALSE;
}


void s3SetVidPage(vPage)
   int vPage;
{
  s3BankSelect(vPage);
}
