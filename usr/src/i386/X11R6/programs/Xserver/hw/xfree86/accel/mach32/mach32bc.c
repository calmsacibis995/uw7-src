/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/mach32/mach32bc.c,v 3.4 1995/01/28 15:52:09 dawes Exp $$ */
/*
 * Copyright 1993 by Jon Tombs. Oxford University
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Jon Tombs or Oxford University shall
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. The authors  make no
 * representations about the suitability of this software for any purpose. It
 * is provided "as is" without express or implied warranty.
 * 
 * JON TOMBS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 */

/*
 * Extract from s3bcach.c
 * Adapted to Mach8, Hans Nasten. (nasten@everyware.se).
 * Adapted to Mach32, Hans Nasten. (mike@mbsun.mlb.org).
 */
/* $XConsortium: mach32bc.c /main/3 1995/11/12 17:24:27 kaleb $ */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"cfb.h"
#include	"misc.h"
#include        "xf86.h"
#include	"windowstr.h"
#include	"gcstruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include        "xf86bcache.h"

#include	"mach32.h"

extern unsigned short mach32cachemask[];
extern unsigned short *mach32cachereadmask;

/*
 * Use the GE to move a block of cache memory when compacting the cache.
 *
 * The id field in the cache blocks is used as a bitplane number, except
 * for full depth blocks when 0xff is used.
 */
void mach32CacheMoveBlock( srcx, srcy, dstx, dsty, h, w, id )
int srcx, srcy, dstx, dsty, h, w;
unsigned int id;

{
    WaitQueue(8);
    outw(EXT_SCISSOR_T, 0);
    outw(EXT_SCISSOR_L, 0);
    outw(EXT_SCISSOR_R, mach32MaxX);
    outw(EXT_SCISSOR_B, mach32MaxY);
    outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX);  
    outw(FRGD_MIX, FSS_BITBLT | MIX_SRC);
    outw(BKGD_MIX, BSS_BITBLT | MIX_SRC);
    if( id != 0xff )
      outw(WRT_MASK, mach32cachemask[id]);
    else
      outw(WRT_MASK, (short) id);

    WaitQueue(8);		/* now shift the cache */
    if( id != 0xff )
      outw(RD_MASK, mach32cachereadmask[id]);
    else
      outw(RD_MASK, (short) id);
    outw(CUR_Y, srcy);
    outw(CUR_X, srcx);
    outw(DESTX_DIASTP, dstx);
    outw(DESTY_AXSTP, dsty);
    outw(MAJ_AXIS_PCNT, w - 1);
    outw(MULTIFUNC_CNTL, MIN_AXIS_PCNT | (h - 1));
    outw(CMD, CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);

    WaitQueue(4);		/* sanity returns */
    outw(RD_MASK, 0xffff);
    outw(WRT_MASK, 0xffff);
    outw(MULTIFUNC_CNTL, PIX_CNTL | MIXSEL_FRGDMIX | COLCMPOP_F);
    outw(FRGD_MIX, FSS_FRGDCOL | MIX_SRC);
    outw(BKGD_MIX, BSS_BKGDCOL | MIX_SRC);

}
