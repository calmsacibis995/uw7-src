/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/s3_svga/s3_driver.c,v 3.10 1995/05/27 03:17:28 dawes Exp $ */
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
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 * 
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 */
/* $XConsortium: s3_driver.c /main/6 1995/12/03 09:42:56 kaleb $ */

#include "X.h"
#include "input.h"
#include "screenint.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_HWlib.h"
#define XCONFIG_FLAGS_ONLY
#include "xf86_Config.h"
#include "vga.h"

  typedef struct {
     vgaHWRec std;		/* good old IBM VGA */
     unsigned char ExtStart;	/* Tseng S3 specials   CRTC 0x33/0x34/0x35 */
     unsigned char Compatibility;
     unsigned char OverflowHigh;
     unsigned char StateControl;/* TS 6 & 7 */
     unsigned char AuxillaryMode;
     unsigned char Misc;	/* ATC 0x16 */
     unsigned char SegSel;
     unsigned char s3reg[10];	/* Video Atribute */
     unsigned char s3sysreg[36];/* Video Atribute */
  }
vgaS3Rec, *vgaS3Ptr;

unsigned short chip_id;
#define S3_911_ONLY     (chip_id==0x81)
#define S3_924_ONLY     (chip_id==0x82)
#define S3_801_ONLY       (chip_id==0xa0)
#define S3_928_ONLY       (chip_id==0x90)
#define S3_911_SERIES     ((chip_id&0xf0)==0x80)
#define S3_801_SERIES     ((chip_id&0xf0)==0xa0)
#define S3_928_SERIES     ((chip_id&0xf0)==0x90)
#define S3_864_SERIES     ((chip_id&0xf0)==0xc0)
#define S3_964_SERIES     ((chip_id&0xf0)==0xd0)
#define S3_x64_SERIES     (S3_864_SERIES|S3_964_SERIES)
#define S3_801_928_SERIES (S3_801_SERIES||S3_928_SERIES|S3_x64_SERIES)
#define S3_8XX_9XX_SERIES (S3_911_SERIES||S3_801_928_SERIES|S3_x64_SERIES)
#define S3_ANY_SERIES     (S3_8XX_9XX_SERIES|S3_x64_SERIES)


static Bool S3ClockSelect ();
static char *S3Ident ();
static Bool S3Probe ();
static void S3EnterLeave ();
static Bool S3Init ();
static Bool S3ValidMode ();
static void *S3Save ();
static void S3Restore ();
static void S3Adjust ();
extern void S3SetRead ();
static Bool LegendClockSelect ();
 
vgaVideoChipRec S3_SVGA =
{
   S3Probe,
   S3Ident,
   S3EnterLeave,
   S3Init,
   S3ValidMode,
   S3Save,
   S3Restore,
   S3Adjust,
   vgaHWSaveScreen,
   (void (*)())NoopDDA,
   (void (*)())NoopDDA,
   S3SetRead,
   S3SetRead,
   S3SetRead,
   0x10000,
   0x10000,
   16,
   0xFFFF,
   0x00000, 0x10000,
   0x00000, 0x10000,
   FALSE,
   VGA_DIVIDE_VERT,
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

#define new ((vgaS3Ptr)vgaNewVideoState)

Bool (*ClockSelect) ();
short old_clock = 0;

static unsigned S3_ExtPorts[] = {0x22e8};
static int Num_S3_ExtPorts = 
   (sizeof(S3_ExtPorts)/sizeof(S3_ExtPorts[0]));

/*
 * S3Ident --
 */
char *
S3Ident (int n)
{
   static char *chipsets[] =
   {"s3"};

   if (n + 1 > sizeof (chipsets) / sizeof (char *))
        return (NULL);
   else
      return (chipsets[n]);

}


/*
 * S3ClockSelect -- select one of the possible clocks ...
 */

static Bool
S3ClockSelect (no)
     int no;
{
   static unsigned char save1, save2;
   unsigned char temp;
   
   switch(no)
   {
   case CLK_REG_SAVE:
      save1 = inb(0x3CC);
      outb(vgaIOBase + 4, 0x42);
      save2 = inb(vgaIOBase + 5);
      break; 
   case CLK_REG_RESTORE:
      outb(0x3C2, save1);
      outb(vgaIOBase + 4, 0x42);
      outb(vgaIOBase + 5, save2);
      break;
   default:
      temp = inb (0x3CC);
      outb (0x3C2, /* temp | 0x01 */ 0x3f);     /* JNT */
      outb (vgaIOBase + 4, 0x42);
      temp = (inb (vgaIOBase + 5) & 0xf0 );
      outb (vgaIOBase + 5, (temp | no));
   }
   return(TRUE);
}



/*
 * LegendClockSelect -- select one of the possible clocks ...
 */

static Bool
LegendClockSelect (no)
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
  * ICS     ET-4000 ---     --- FS0     CS0 FS1     CS1 FS2     ff0
  * flip/flop 0 output FS3     CS2 FS4     ff1     flip/flop 1 output
  * 
  * The flip/flops are loaded from CS0 and CS1.  The flip/flops are latched by
  * CS2, on the rising edge. After CS2 is set low, and then high, it is then
  * set to its final value.
  * 
  */
   unsigned char temp = inb (0x3CC);

   outb (0x3C2, (temp & 0xF3) | ((no & 0x10) >> 1) | (no & 0x04));
   outw (vgaIOBase + 4, 0x0034);
   outw (vgaIOBase + 4, 0x0234);
   outw (vgaIOBase + 4, ((no & 0x08) << 6) | 0x34);
   outb (0x3C2, (temp & 0xF3) | ((no << 2) & 0x0C));
}



/*
 * S3Probe -- check up whether a S3 based board is installed
 */

static Bool
S3Probe ()
{
   int numClocks;
   unsigned char temp;
   unsigned short config;

   xf86ClearIOPortList(vga256InfoRec.scrnIndex);
   xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_VGA_IOPorts, VGA_IOPorts);
   xf86AddIOPorts(vga256InfoRec.scrnIndex, Num_S3_ExtPorts, S3_ExtPorts);

   if (vga256InfoRec.chipset) {
      if (strncmp (vga256InfoRec.chipset, "s3", 2))
	 return (FALSE);
   }

   S3EnterLeave(ENTER);

   outb(0x3d4, 0x30);
   chip_id = inb(0x3d5);
   old_clock = inb(0x3cc);

   if (!S3_ANY_SERIES) {
      S3EnterLeave(FALSE);
      return(FALSE);
   }

      if (S3_864_SERIES )
	 ErrorF ("%s %s: S3 chipset is a Vision864.\n",
		 XCONFIG_PROBED, vga256InfoRec.name);
      else if (S3_964_SERIES )
	 ErrorF ("%s %s: S3 chipset is a Vision964.\n",
		 XCONFIG_PROBED, vga256InfoRec.name);
      else if (S3_801_928_SERIES ) {
         if (S3_801_SERIES ) {
            ErrorF ("%s %s: S3 chipset is an 801 or 805\n",
		    XCONFIG_PROBED, vga256InfoRec.name);
	 } else if (S3_928_SERIES ) {
            ErrorF ("%s %s: S3 chipset is a 928\n",
		    XCONFIG_PROBED, vga256InfoRec.name);
	 }
      } else if (S3_911_SERIES) {
         if (S3_911_ONLY ) {
            ErrorF ("%s %s: S3 chipset is a 911\n",
		    XCONFIG_PROBED, vga256InfoRec.name);
	 } else if (S3_924_ONLY ) {
            ErrorF ("%s %s: S3 chipset is a 924\n",
		    XCONFIG_PROBED, vga256InfoRec.name);
	 } else {
            ErrorF ("%s %s: S3 chipset unknown, chip_id = 0x%02x\n",
		    XCONFIG_PROBED, vga256InfoRec.name, chip_id);
	 }
      } else
         ErrorF ("%s %s: Unknown chipset : chip_id is 0x%02x\n",
		    XCONFIG_PROBED, vga256InfoRec.name, chip_id);

   outb (0x3d4, 0x36);          /* for register CR36 (CONFG_REG1), */
   config = inb (0x3d5);        /* get amount of vram installed */

   if ((config & 0x03) == 0) {
      ErrorF ("%s %s: This is an EISA card\n",
	      XCONFIG_PROBED, vga256InfoRec.name);
   }
   if ((config & 0x03) == 1) {
      ErrorF ("%s %s: This is a 386/486 localbus card\n",
	      XCONFIG_PROBED, vga256InfoRec.name);
   }
   if ((config & 0x03) == 3) {
      ErrorF ("%s %s: This is an ISA card\n",
	      XCONFIG_PROBED, vga256InfoRec.name);
   }
   if (!vga256InfoRec.videoRam) {
      if ((config & 0x20) != 0) {       /* if bit 5 is a 1, then 512k RAM */

         vga256InfoRec.videoRam = 512;
      } else {                  /* must have more than 512k */

         switch ((config & 0xC0) >> 6) {        /* look at bits 6 and 7 */
           case 0:
              vga256InfoRec.videoRam = 4096;
              break;
           case 1:
              vga256InfoRec.videoRam = 3072;
              break;
           case 2:
              vga256InfoRec.videoRam = 2048;
              break;
           case 3:
              vga256InfoRec.videoRam = 1024;
              break;
	 }
      }
#if 0
      ErrorF ("%d K of memory found\n", vga256InfoRec.videoRam);
#endif
   }
#if 0
   else ErrorF ("%d K videoram\n", vga256InfoRec.videoRam);
#endif


   if (OFLG_ISSET(OPTION_LEGEND, &vga256InfoRec.options)) {
      ClockSelect = LegendClockSelect;
      numClocks = 32;
   } else {
      ClockSelect = S3ClockSelect;
      numClocks = 16;
   }

   if (!vga256InfoRec.clocks)
      vgaGetClocks (numClocks, ClockSelect);

  vga256InfoRec.chipset = S3Ident(0);
   return (TRUE);
}



/*
 * S3EnterLeave -- enable/disable io-mapping
 */

static void
S3EnterLeave (enter)
     Bool enter;
{
   unsigned char temp;

   if (enter) {
      xf86EnableIOPorts(vga256InfoRec.scrnIndex);
      vgaIOBase = (inb(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
      vgaIOBase = 0x3D0;
      /* unlock everything */
      outb (0x3d4, 0x38);
      outb (0x3d5, 0x48);
      outb (0x3d4, 0x39);
      outb (0x3d5, 0xa5);

      /* This stuff should probably go elsewhere (DHD) */
      outb (0x3d4, 0x35);          /* select segment 0 */
      temp = inb (0x3d5);
      outb (0x3d5, temp & 0xf0);

      outb (0x3ce, 0x6);	/* set graphics mode & 64k */
      temp = inb (0x3cf);
      outb (0x3cf, temp | 5);

    /*
     * outb(0x3c0, 0x10);   this was to enable the palette outb(0x3c1, 0x41);
     */

      temp = inb (0x3CC);	/* enable ram address decoder */
      outb (0x3C2, temp | 2);
    /* vertical retrace low-end */
      outb (vgaIOBase + 4, 0x11);
      temp = inb (vgaIOBase + 5);
      outb (vgaIOBase + 5, temp & 0x7F);


   } else {
    /*
     * outb(0x3BF, 0x01);                            relock S3 special
     * outb(vgaIOBase + 8, 0xA0);
     */

      xf86DisableIOPorts(vga256InfoRec.scrnIndex);
   }
}





/*
 * S3Restore -- restore a video mode
 */

static void
S3Restore (restore)
     vgaS3Ptr restore;
{
   unsigned char i;
   extern Bool xf86Exiting;
   
   outb (0x3d4, 0x35);		/* select bank zero */
   i = inb (0x3d5);
   outb (0x3d5, (i & 0xf0));
   outb(0x3CD, 0x00); /* segment select */

   vgaHWRestore ((vgaHWPtr)restore);

  i = inb(vgaIOBase + 0x0A); /* reset flip-flop */
  outb(0x3C0, 0x36);
  outb(0x3C0, restore->Misc);

 /* restore s3 special bits */
   if (S3_801_928_SERIES) {
    /* restore 801 specific registers */

      for (i = 32; i < 35; i++) {
	 outb (0x3d4, 0x40 + i);
	 outb (0x3d5, restore->s3sysreg[i]);

      }
   }
   for (i = 0; i < 5; i++) {
      outb (0x3d4, 0x31 + i);
      outb (0x3d5, restore->s3reg[i]);
   } 
   for (i = 2 ; i < 5; i++) { /* don't restore the locks */   
      outb (0x3d4, 0x38 + i);
      outb (0x3d5, restore->s3reg[5 + i]);
   }

   for (i = 0; i < 16; i++) {
      outb (0x3d4, 0x40 + i);
      outb (0x3d5, restore->s3sysreg[i]);
   }

  if (restore->std.NoClock >= 0)
    (ClockSelect)(restore->std.NoClock);

  if (xf86Exiting)
   outb(0x3c2, old_clock);
  outw(0x3C4, 0x0300); /* now reenable the timing sequencer */


}



/*
 * S3Save -- save the current video mode
 */

static void *
S3Save (save)
     vgaS3Ptr save;
{
   int i;
   unsigned char temp;

   temp = inb(0x3CD); outb(0x3CD, 0x00); /* segment select */

   save = (vgaS3Ptr) vgaHWSave ((vgaHWPtr)save, sizeof (vgaS3Rec));


   for (i = 0; i < 5; i++) {
      outb (0x3d4, 0x31 + i);
      save->s3reg[i] = inb (0x3d5);
      outb (0x3d4, 0x38 + i);
      save->s3reg[5 + i] = inb (0x3d5);
   }

   outb (0x3d4, 0x11);		/* allow writting? */
   outb (0x3d5, 0x00);
   for (i = 0; i < 16; i++) {
      outb (0x3d4, 0x40 + i);

      save->s3sysreg[i] = inb (0x3d5);
   }

   for (i = 32; i < 35; i++) {
      outb (0x3d4, 0x40 + i);
      save->s3sysreg[i] = inb (0x3d5);
   }
  i = inb(vgaIOBase + 0x0A); /* reset flip-flop */
  outb(0x3C0,0x36); save->Misc = inb(0x3C1); outb(0x3C0, save->Misc);



   return ((void *) save);
}




/*
 * S3Init -- Handle the initialization of the VGAs registers
 */

static Bool
S3Init (mode)
     DisplayModePtr mode;
{
   int i;
   vgaHWInit (mode, sizeof (vgaS3Rec));

   new->std.Attribute[16] = 0x01;	/* use the FAST 256 Color Mode */
   new->std.CRTC[19] = vga256InfoRec.virtualX >> 3;
   new->std.CRTC[20] = 0x60;
   new->std.CRTC[23] = 0xAB;
   new->StateControl = 0x00;

   new->ExtStart = 0x00;

 /* DON'T forget to set Interlace else where!!!!!! */
   new->OverflowHigh =
      ((mode->CrtcVSyncStart & 0x400) >> 7)
      | (((mode->CrtcVDisplay - 1) & 0x400) >> 9)
      | (((mode->CrtcVTotal - 2) & 0x400) >> 5)
      | (((mode->CrtcVTotal - 2) & 0x200) >> 9)
      | (((mode->CrtcVSyncStart) & 0x200) >> 3);


  new->s3reg[0] = 0x8d;
  new->s3reg[1] = 0x00;
  new->s3reg[2] = 0x20;
  new->s3reg[3] = 0x10;
  if (mode->CrtcHDisplay > 800)
      new->s3reg[4] = 0x00;
  else
     new->s3reg[4] = 0x13;

  new->s3reg[5] = 0x48;
  new->s3reg[6] = 0xa5;
  new->s3reg[7] = 0xb5;
  new->s3reg[8] = (new->std.CRTC[0] + new->std.CRTC[4] + 1)/2;  
  new->s3reg[9] = 0x44;
  new->s3reg[10] = mode->Clock;

  outb (0x3d4, 0x40);
  i = (inb (0x3d5) & 0xf6);    /* don't touch local bus setting */
  
  new->s3sysreg[0] = (i | 0x05);
  new->s3sysreg[1] = 0x14;
  new->s3sysreg[2] = mode->Clock;
  new->s3sysreg[3] = 0x00;
  new->s3sysreg[4] = 0x00;
  new->s3sysreg[5] = 0x00;

  for (i = 6 ; i < 16 ; i++) { /* For these I don't have doc */
   outb (0x3d4, 0x40 + i);
   new->s3sysreg[i] = inb(0x3d5);
  }

  if (S3_801_928_SERIES) {
    new->s3sysreg[24] = 0x00;
    new->s3sysreg[25] = 0x0a;
    new->s3sysreg[20] = 0xa0;
    new->s3sysreg[32] = 0x2f;
    new->s3sysreg[33] = 0x81;
    new->s3sysreg[34] = 0x00;
  }


#ifdef ENHANCED_ENABLE
   if (mode->CrtcHDisplay < 800) {	/* MOVE ME - JNT */
      outw (0x4ae8, 0x0003);
   } else
      outw (0x4ae8, 0x0007);
#endif
   return TRUE;
}



/*
 * S3Adjust -- adjust the current video frame to display the mousecursor
 */

static void
S3Adjust (x, y)
     int x, y;
{
  int Base;

  Base = (y * 1024 + x) >> 2;

  outb(0x3d4, 0x31);
  outb(0x3d5, ((Base & 0x030000) >> 12) | 0x8d); 
  
  vgaIOBase = 0x3d0;
  outw(vgaIOBase + 4, (Base & 0x00FF00) | 0x0C);
  outw(vgaIOBase + 4, ((Base & 0x00FF) << 8) | 0x0D);

}


#if 0
go_linear()
{
   if (S3_801_928_SERIES) {
      int   i;
      ErrorF("go linear called \n");
    /* begin 801 sequence for going in to linear mode */
      outb (0x3d4, 0x40);
      i = inb (0x3d5) & 0xf6;
      i |= 0x0a;/* enable fast write buffer and disable
                                 * 8514/a mode */
      outb (0x3d5, (unsigned char) i);
      outb (0x3d4, 0x59);
      outb (0x3d5, 0x0a);
      outb (0x3d4, 0x53);
      outb (0x3d5, 1);
      outb (0x3d4, 0x58);
      outb (0x3d5, 0x10);       /* go on to linear mode */
   }
}
#endif

static Bool
S3ValidMode(mode)
DisplayModePtr mode;
{
  return TRUE;
}
