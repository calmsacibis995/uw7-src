/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cirBlitMM.h,v 3.5 1995/04/09 14:14:16 dawes Exp $ */

/* Definitions for BitBLT engine communication. */
/* Using Memory-Mapped I/O. */

/* $XConsortium: cirBlitMM.h /main/3 1995/11/13 08:20:23 kaleb $ */

#if !defined(__STDC__) && !defined(__GNUC__)

/* If we don't have volatile, MMIO isn't used, but we compile anyway. */

#ifdef volatile
#undef volatile
#endif
#define volatile /**/

#endif

/* BitBLT modes. */

#define FORWARDS		0x00
#define BACKWARDS		0x01
#define SYSTEMDEST		0x02
#define SYSTEMSRC		0x04
#define TRANSPARENCYCOMPARE	0x08
#define PIXELWIDTH16		0x10
#define PIXELWIDTH32		0x30	/* 543x only. */
#define PATTERNCOPY		0x40
#define COLOREXPAND		0x80

/* MMIO addresses (offset from 0xb8000). */

#define MMIOBACKGROUNDCOLOR	0x00
#define MMIOFOREGROUNDCOLOR	0x04
#define MMIOWIDTH		0x08
#define MMIOHEIGHT		0x0a
#define MMIODESTPITCH		0x0c
#define MMIOSRCPITCH		0x0e
#define MMIODESTADDR		0x10
#define MMIOSRCADDR		0x14
#define MMIOBLTWRITEMASK	0x17
#define MMIOBLTMODE		0x18
#define MMIOROP			0x1a
#define MMIOBLTSTATUS		0x40

extern unsigned char *cirrusMMIOBase;

/* Address: the 5426 adresses 2MBytes, the 5434 can address 4MB. */

#define SETDESTADDR(dstAddr) \
  *(unsigned int *)(cirrusMMIOBase + MMIODESTADDR) = dstAddr;

#define SETSRCADDR(srcAddr) \
  *(unsigned int *)(cirrusMMIOBase + MMIOSRCADDR) = srcAddr;

#define SETSRCADDRUNMODIFIED SETSRCADDR

/* Pitch: the 5426 goes up to 4095, the 5434 can do 8191. */

#define SETDESTPITCH(dstPitch) \
  *(unsigned short *)(cirrusMMIOBase + MMIODESTPITCH) = dstPitch;

#define SETSRCPITCH(srcPitch) \
  *(unsigned short *)(cirrusMMIOBase + MMIOSRCPITCH) = srcPitch;

/* Width: the 5426 goes up to 2048, the 5434 can do 8192. */

#define SETWIDTH(fillWidth) \
  *(unsigned short *)(cirrusMMIOBase + MMIOWIDTH) = fillWidth - 1;

/* Height: the 5426 goes up to 1024, the 5434 can do 2048. */

#define SETHEIGHT(fillHeight) \
  *(unsigned short *)(cirrusMMIOBase + MMIOHEIGHT) = fillHeight - 1;

#define SETBLTMODE(m) \
  *(unsigned char *)(cirrusMMIOBase + MMIOBLTMODE) = m;

#define SETBLTWRITEMASK(m) \
  *(unsigned char *)(cirrusMMIOBase + MMIOBLTWRITEMASK) = m;

#define SETROP(rop) \
  *(unsigned char *)(cirrusMMIOBase + MMIOROP) = rop;

#define STARTBLT() \
  *(unsigned char *)(cirrusMMIOBase + MMIOBLTSTATUS) |= 0x02;

#define BLTBUSY(s) \
  s = *(volatile unsigned char *)(cirrusMMIOBase + MMIOBLTSTATUS) & 1;

/* BitBLT reset: temporarily set bit 2 of GR31 */
#define BLTRESET() \
  *(volatile unsigned char *)(cirrusMMIOBase + MMIOBLTSTATUS) ^= 0x04; \
  *(volatile unsigned char *)(cirrusMMIOBase + MMIOBLTSTATUS) ^= 0x04;

#define WAITUNTILFINISHED() CirrusBLTWaitUntilFinished()  

/* The macros for setting the background/foreground color are already
   defined using OUTs in cir_driver.h. */

/* To keep consistency with non-MMIO shadow variables, we also update the
 * shadow variables. Note that this is not done for the BitBLT registers;
 * functions that use them with port I/O while MMIO is also enabled must
 * invalidate the shadow variables.
 * Also note that the upper bytes of the potentially 32-bit color must
 * be preserved (they are used even in 8bpp on the 543x). */

#undef SETBACKGROUNDCOLOR
#define SETBACKGROUNDCOLOR(c) \
  *(unsigned char *)(cirrusMMIOBase + MMIOBACKGROUNDCOLOR) = c; \
  *(unsigned char *)(&cirrusBackgroundColorShadow) = c;

#undef SETFOREGROUNDCOLOR
#define SETFOREGROUNDCOLOR(c) \
  *(unsigned char *)(cirrusMMIOBase + MMIOFOREGROUNDCOLOR) = c; \
  *(unsigned char *)(&cirrusForegroundColorShadow) = c;

#undef SETBACKGROUNDCOLOR16
#define SETBACKGROUNDCOLOR16(c) \
  *(unsigned short *)(cirrusMMIOBase + MMIOBACKGROUNDCOLOR) = c; \
  *(unsigned short *)(&cirrusBackgroundColorShadow) = c;

#undef SETFOREGROUNDCOLOR16
#define SETFOREGROUNDCOLOR16(c) \
  *(unsigned short *)(cirrusMMIOBase + MMIOFOREGROUNDCOLOR) = c; \
  *(unsigned short *)(&cirrusForegroundColorShadow) = c;

/* 32-bit colors are exclusive to the BitBLT engine. */

#define SETBACKGROUNDCOLOR32(c) \
  *(unsigned int *)(cirrusMMIOBase + MMIOBACKGROUNDCOLOR) = c; \
  cirrusBackgroundColorShadow = c;

#define SETFOREGROUNDCOLOR32(c) \
  *(unsigned int *)(cirrusMMIOBase + MMIOFOREGROUNDCOLOR) = c; \
  cirrusForegroundColorShadow = c;
