/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/et4000w32/w32/w32vars.c,v 3.5 1995/01/28 15:51:15 dawes Exp $ */
/*******************************************************************************
                        Copyright 1994 by Glenn G. Lai

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyr notice appear in all copies and that
both that copyr notice and this permission notice appear in
supporting documentation, and that the name of Glenn G. Lai not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

Glenn G. Lai DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

Glenn G. Lai
P.O. Box 4314
Austin, Tx 78765
glenn@cs.utexas.edu)
4/6/94
*******************************************************************************/
/* $XConsortium: w32vars.c /main/5 1995/11/12 16:37:57 kaleb $ */

#include "w32.h"

ByteP W32Buffer;
VByteP ACL; 

LongP MBP0, MBP1, MBP2; 
ByteP MMU_CONTROL;

ByteP ACL_SUSPEND_TERMINATE,
      ACL_OPERATION_STATE,
      ACL_SYNC_ENABLE,
      ACL_INTERRUPT_MASK,
      ACL_INTERRUPT_STATUS,
      ACL_ACCELERATOR_STATUS;

WordP ACL_X_POSITION,
      ACL_Y_POSITION;

LongP ACL_PATTERN_ADDRESS,
      ACL_SOURCE_ADDRESS;

WordP ACL_PATTERN_Y_OFFSET,
      ACL_SOURCE_Y_OFFSET,
      ACL_DESTINATION_Y_OFFSET;

ByteP ACL_VIRTUAL_BUS_SIZE,     /* only for w32 and w32i */
      ACL_XY_DIRECTION,     	/* only for w32 and w32i */
      ACL_PIXEL_DEPTH;          /* only for w32p_rev_A and w32p_rev_B */

ByteP ACL_PATTERN_WRAP,
      ACL_SOURCE_WRAP;

WordP ACL_X_COUNT,
      ACL_Y_COUNT;

ByteP ACL_ROUTING_CONTROL,
      ACL_RELOAD_CONTROL,
      ACL_BACKGROUND_RASTER_OPERATION,
      ACL_FOREGROUND_RASTER_OPERATION;

LongP ACL_DESTINATION_ADDRESS,

      /* only for w32p_rev_A and w32p_rev_B */
      ACL_MIX_ADDRESS;

WordP ACL_MIX_Y_OFFSET,
      ACL_ERROR_TERM,
      ACL_DELTA_MINOR,
      ACL_DELTA_MAJOR;

/*
 *  True for w32 and w32i 
 */
Bool W32;
Bool W32i;
Bool W32OrW32i;
Bool W32p;
Bool W32pa;
Bool W32pCAndLater;
Bool FrameBuffer;

ByteP W32BytePtr;
WordP W32WordPtr;
LongP W32LongPtr;

/* 
 *  Each points to a 4x2 wrap
 */
long W32Foreground;
long W32Background;
long W32Pattern;

long W32BltCount;
long W32BltHop;
long W32BoxCount;
long W32BoxHop;
long W32PointCount;
long W32PointHop;

/*
 *
 * Sixteen X ops in total
 *
 */
int W32OpTable[] = {
    0x00, /* Xclear		0 */
    0x88, /* Xand		src AND dst */
    0x44, /* XandReverse	src AND NOT dst */
    0xcc, /* Xcopy		src */
    0x22, /* XandInverted	NOT src AND dst */
    0xaa, /* Xnoop		dst */
    0x66, /* Xxor		src XOR dst */
    0xee, /* Xor		src OR dst */
    0x11, /* Xnor		NOT src AND NOT dst */
    0x99, /* Xequiv		NOT src XOR dst */
    0x55, /* Xinvert		NOT dst */
    0xdd, /* XorReverse		src OR NOT dst */
    0x33, /* XcopyInverted	NOT src */
    0xbb, /* XorInverted	NOT src OR dst */
    0x77, /* Xnand		NOT src OR NOT dst */
    0xff  /* Xset		1 */
};


#if 0
FILE *GlennDebug;
#endif
