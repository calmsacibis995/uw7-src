/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cir_alloc.c,v 3.3 1996/01/13 12:22:00 dawes Exp $ */
/*
 * cir_alloc.c,v 1.2 1994/09/11 05:52:49 scooper Exp
 * 
 * Copyright 1993 by H. Hanemaayer, Utrecht, The Netherlands
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of H. Hanemaayer not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  H. Hanemaayer makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * H. HANEMAAYER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL H. HANEMAAYER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  H. Hanemaayer, <hhanemaa@cs.ruu.nl>
 * Modified: Simon P. Cooper <scooper@vizlab.rutgers.edu>
 *
 */
/* $XConsortium: cir_alloc.c /main/5 1996/01/13 13:13:26 kaleb $f */

/*
 * This file implements functions a simple allocator of video memory for
 * scratch space, patterns etc.
 *
 * The allocator can currently only free the last request. More advanced
 * functionality can be added when it is actually required.
 */

#include "vga256.h"
#include "cfbrrop.h"
#include "mergerop.h"
#include "vgaBank.h"
#include "xf86.h"
#include "vga.h"	/* For vga256InfoRec */

#include "compiler.h"

#include "cir_driver.h"
#include "cir_inline.h"


#define ROUNDUP8(x) ((x + 7) & ~7)


static int allocatebase;	/* Video memory address of allocation space. */
static int freespace;		/* Number of bytes left for allocation. */
static int currentaddr;		/* Video address of first available space. */
static int lastaddr;		/* Video address of last allocation. */
static int lastsize;		/* Size of the last request. */

/*
 * Initialize the allocator.
 * This is called once at server start-up.
 */

int CirrusInitializeAllocator(base)
	int base;	/* Base video address of allocation space. */
{
	/* We want to align on a multiple of 8 bytes, should be OK. */
	allocatebase = CirrusMemTop;
	freespace = vga256InfoRec.videoRam * 1024 - allocatebase;
	currentaddr = allocatebase;
	lastaddr = -1;
	return freespace;
}

/*
 * Allocate a number of bytes of video memory.
 */

int CirrusAllocate(size)
	int size;
{
	if (size > freespace)
		return -1;
	lastaddr = currentaddr;
	size = ROUNDUP8(size);	/* Units of 8 bytes. */
	lastsize = size;
	freespace -= size;
	currentaddr += size;
	return currentaddr - size;
}

/* <scooper>
 * Allocate space for a cursor.  This is allocated from the top of memory as
 * there are only a fixed number of locations for the cursor.  The cursor
 * must also be aligned on either a 256 or 1024 byte boundary.
 */

int CirrusCursorAllocate(cursor)
     cirrusCurRecPtr cursor;
{
  int size;

  size = (cursor->cur_size == 0) ? 256 : 1024;

  if (size > freespace)
    return -1;
  
  cursor->cur_addr = (vga256InfoRec.videoRam * 1024) - size;
  freespace -= size;
  
  /* This is the cursor "pattern offset" (SR13) */
  cursor->cur_select = (cursor->cur_size == 0) ? 63 : 15;

  return 0;
}

/*
 * Free allocated space. Currently needs to be the last allocated one.
 */

void CirrusFree(vidaddr)
	int vidaddr;
{
	if (vidaddr != lastaddr)	/* If not allocated right before, */
		return;			/* do nothing. */
	/* Undo the last allocation. */
	currentaddr -= lastsize;
	freespace -= lastsize;
	lastaddr = -2;
}

/*
 * Upload a pattern to video memory.
 */

void CirrusUploadPattern(pattern, width, height, vidaddr, srcpitch)
	unsigned char *pattern;
	int width;	/* width of the pattern in bytes. */
	int height;	/* height of the pattern in scanlines. */
	int vidaddr;	/* offset into video memory for pattern. */
	int srcpitch;	/* Offset between scanlines in source. */
{
	int writebank, destaddr;
	int i;
	unsigned char *base;
	/* Write the image to video memory at offset vidaddr. */
	destaddr = vidaddr;
#ifdef PC98_WAB
	CIRRUSSETWRITEB_WAB(destaddr, writebank);
#else
	CIRRUSSETWRITEB(destaddr, writebank);
#endif
	base = CIRRUSWRITEBASE();
	for (i = 0; i < height; i++) {
		__memcpy(
			base + destaddr,
			pattern + i * srcpitch,
			width
			);
		destaddr += width;
#ifdef PC98_WAB
		CIRRUSCHECKWRITEB_WAB(destaddr, writebank);
#else
		CIRRUSCHECKWRITEB(destaddr, writebank);
#endif
	}
}
