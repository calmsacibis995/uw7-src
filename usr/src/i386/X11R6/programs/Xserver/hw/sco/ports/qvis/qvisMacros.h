/*
 *	@(#) qvisMacros.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Oct 06 21:48:21 PDT 1992	mikep@sco.com
 *	- Clean up inline ifdef's
 *	- Add BIT_SWAP and GC_SERIAL number macros
 *	S001	Thu Oct 15 18:14:10 PDT 1992	mikep@sco.com
 *	- Experiment with the Wait macros.  This was a test to see
 *	if the inb caused a performance hit on the card.  It seems it
 *	does not.  This technique resulted in very good performance 
 *	improvements for the XGA and 340x0's.
 *      S002    Tue Jul 13 12:40:06 PDT 1993    davidw@sco.com
 *      - compaq waltc AGA EFS 2.0 source handoff 07/02/93
 *      S003    Fri Jul 14 12:40:06 PDT 1995    davidw@sco.com
 *      - compaq waltc AHS 5.1 handoff to fix Qvision hang
 */

/**
 *
 * Copyright 1992, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mjk       04/07/92  Originated (see RCS log)
 * mikep     07/27/92  Use _M_COFF to differ between 2.0 and 1.1 compiler
 * mikep     10/06/92  Just have one define for inline code
 * waltc     06/26/93  Use qvisPriv->pitch2 in qvisFrameBufferLoc.
 * waltc     09/28/94  Allow blit reset from qvisWaitForGlobalNotBusy.
 */

#ifndef _QVIS_MACROS_H
#define _QVIS_MACROS_H

/*
 * The hope is if these are macros, we can easily change these to allow
 * inlining of the accesses at some time.  Do NOT define QVIS_INLINE_PORTS for
 * the current SCO C compiler.  The compiler does do the inlining when the
 * "-Oi" option is used BUT the server with bus error when used.  Mike
 * Patnode at SCO is aware of this problem and is trying to work it with the
 * compiler people.  Uses the functions instead! -mjk
 * 
 * My testing has shown that the ODT 2.0 Beta DevSys compiler can properly
 * inline outp and inp.  - mjk 4/6/92
 */
#ifdef USE_INLINE_CODE
#define qvisOut16(port, val) outpw(port, val)
#define qvisOut8(port, val) outp(port, val)
#define qvisIn16(port) inpw(port)
#define qvisIn8(port) inp(port)
#else
extern void     outw();
extern void     outb();
extern short    inw();
extern char     inb();
#define qvisOut16(port, val) outw(port, val)
#define qvisOut8(port, val) outb(port, val)
#define qvisIn16(port) inw(port)
#define qvisIn8(port) inb(port)
#endif /* USE_INLINE_CODE */

/*
 * Extracts private data from a qvis screen. -mjk
 */
#define QVIS_PRIVATE_DATA(pScrn) \
   ((qvisPrivateData *)((pScrn)->devPrivates[qvisScreenPrivateIndex].ptr))

/*
 * Give a frame buffer base and an (x,y) coordinate, returns the address for
 * that pixel (for packed mode only). -mjk
 */
#define qvisFrameBufferLoc(fb_base, x, y) \
   ((unsigned char *) (fb_base + ((y) << qvisPriv->pitch2) + (x)))

/*
 * These macros assume the following declaration has been been made for the
 * routine using these macros:
 * 
 * qvisPriv = QVIS_PRIVATE_DATA(pDraw->pScreen);
 * 
 * These macros "shadow" the value of often used Q-Vision registers in memory in
 * hopes of speeding up Q-Vision state setup if the current state is already
 * right. -mjk
 */

#ifdef QVIS_SINGLE_SCREEN
#define qvisSetCurrentScreen() \
   { /* nothing */ }
#else
#define qvisSetCurrentScreen() \
   qvisOut8(BOARD_SELECT, qvisPriv->virt_controller_id)
#endif

#define qvisSetPlanarMode(mode) \
   { \
      qvisOut8(CTRL_REG1, (qvisIn8(CTRL_REG1)&CREG1_MASK)|CREG1_EXPAND_TO_FG); \
      qvisOut8(GC_INDEX, DATAPATH_REG); \
      qvisOut8(GC_DATA, mode); \
   }

#define qvisSetPackedMode(mode) \
   { \
      qvisOut8(CTRL_REG1, (qvisIn8(CTRL_REG1)&CREG1_MASK)); \
      qvisOut8(GC_INDEX,DATAPATH_REG); \
      qvisOut8(GC_DATA, mode); \
   }

#ifdef QVIS_SHADOWED

/*
 * shadowed versions of data path configuration macros
 */

#define qvisSetForegroundColor(_fg) \
   if(qvisPriv->fg != (_fg)) { \
      qvisPriv->fg = (_fg); \
      qvisOut8(GC_INDEX, GC_FG_COLOR); \
      qvisOut8(GC_DATA, (_fg)); \
   }

#define qvisSetBackgroundColor(_bg) \
   if(qvisPriv->bg != (_bg)) { \
      qvisPriv->bg = (_bg); \
      qvisOut8(GC_INDEX, GC_BG_COLOR); \
      qvisOut8(GC_DATA, (_bg)); \
   }

#define qvisSetALU(_alu) \
   if(qvisPriv->alu != (_alu)) { \
      qvisPriv->alu = (_alu); \
      qvisOut8(ROPS_REG, alu_to_hwrop[_alu]); \
   }

#define qvisSetPixelMask(mask) \
   if(qvisPriv->pixel_mask != (mask)) { \
      qvisPriv->pixel_mask = (mask); \
      qvisOut8(GS_INDEX, GS_PIXEL_MASK); \
      qvisOut8(GS_DATA, (mask)); \
   }

#define qvisSetPlaneMask(mask) \
   if(qvisPriv->plane_mask != (mask)) { \
      qvisPriv->plane_mask = (mask); \
      qvisOut8(GC_INDEX, GC_PLANE_MASK); \
      qvisOut8(GC_DATA, (mask)); \
   }

#else				/* not QVIS_SHADOWED, ie. unshadowed */

/*
 * the unshadowed versions of data path configuration macros
 */

#define qvisSetForegroundColor(_fg) \
   { \
      qvisOut8(GC_INDEX, GC_FG_COLOR); \
      qvisOut8(GC_DATA, (_fg)); \
   }

#define qvisSetBackgroundColor(_bg) \
   { \
      qvisOut8(GC_INDEX, GC_BG_COLOR); \
      qvisOut8(GC_DATA, (_bg)); \
   }

#define qvisSetALU(_alu) \
   { \
      qvisOut8(ROPS_REG, alu_to_hwrop[_alu]); \
   }

#define qvisSetPixelMask(mask) \
   { \
      qvisOut8(GS_INDEX, GS_PIXEL_MASK); \
      qvisOut8(GS_DATA, (mask)); \
   }

#define qvisSetPlaneMask(mask) \
   { \
      qvisOut8(GC_INDEX, GC_PLANE_MASK); \
      qvisOut8(GC_DATA, (mask)); \
   }

#endif				/* QVIS_SHADOWED */

/*
 * qvisWaitForBufferNotBusy - wait on the Lin/BLT Command Buffer Busy bit (bit
 * 7) in Control Register 1.
 */
#define qvisWaitForBufferNotBusy() \
   while(qvisIn8(CTRL_REG1) & 0x80) XYZ("qvisBufferBusy")

/*
 * qvisWaitForGlobalNotBusy - wait on the Global Busy bit (bit 6) in Control
 * Register 1.
 */
#define qvisWaitForGlobalNotBusy() { \
   long i = 0; \
   while (qvisIn8(CTRL_REG1) & 0x40) { \
      XYZ("qvisGlobalBusy"); \
      if (i < 250000) { \
         i++; \
      } \
      else { \
         qvisOut8(BLT_CMD0, 0x00); \
         break; \
      } \
   } \
} \

/**
 * THE RULES FOR USING THE "WAITING" MACROS:
 *
 * 1)  A routine which uses the blit OR line engines:
 *
 *     Right before engine use (because we are probably going to change
 *     the data path):
 *
 *       if(qvisPriv->engine_used) {
 *          qvisWaitForGlobalNotBusy();
 *       }
 *
 *     At end of routine:
 *
 *       qvisPriv->engine_used = TRUE;
 *
 *     For multiple engine uses, do this right after each engine "turn on"
 *     (that only use the buffered registers, ie. no data path registers,
 *     ie., more specifically what the manual says is valid for
 *     buffering writes).
 *
 *       qvisWaitForBufferNotBusy();
 *
 * 2)  A routine which does NOT use the blit or line engines:
 *
 *     Before access to frame buffer:
 *
 *       if(qvisPriv->engine_used) {
 *          qvisPriv->engine_used = FALSE;
 *          qvisWaitForGlobalNotBusy();
 *       }
 *
 * 3)  Loss of control of graphics hardware (as in qvisSaveGState):
 *
 *       qvisWaitForGlobalNotBusy();
 *
 * 4)  Restoration of control of graphics hardware (as in qvisRestoreGState):
 *
 *       qvisWaitForGlobalNotBusy();
 *
 */

/* 								S000 vvv
 * Bit swap macros
 */
#if BITMAP_BIT_ORDER == LSBFirst
#define BIT_SWAP(c)  ddxBitSwap[(c)]
#else
#define BIT_SWAP(c)  c
#endif

/* For R4 compatibility */
#ifndef NFB_GC_SERIAL_NUMBER
#define NFB_GC_SERIAL_NUMBER(pgc_or_pdraw)   ((pgc_or_pdraw)->serialNumber)
#endif								/* S000 ^^^ */


#endif				/* _QVIS_MACROS_H */
