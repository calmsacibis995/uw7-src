#ident	"@(#)ihvkit:display/lfb256/lfbBitblt.c	1.1"

/*
 * Copyright (c) 1990, 1991, 1992, 1993 UNIX System Laboratories, Inc.
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	Copyright (c) 1993  Intel Corporation
 *	  All Rights Reserved
 */

/*
 * This code needs a lot of work.  There are probably a number of 8bpp
 * dependencies that need to be caught.
 *
 * This code also suffers a lack of error checking.  Some of the
 * errors that it is known not to check for are:
 *
 * + Bad bitmaps
 * + Off screen coordinates (or does the core clip properly?)
 * + Area off bitmap
 *
 * This code only works with Z pixmaps.  This does not seem to impact
 * anything.
 *
 */

#include <lfb.h>

#define COPY_NORMAL   0
#define COPY_BOT_TOP  1
#define COPY_RGT_LFT  2

static void doRop_lr(pSrc, pDst, wid)
PixelP pSrc, pDst;
int wid;

{
    long  t, *s, *sEnd, *d, *dEnd;
    int cnt, mode;
    Pixel *d1, *s1;

    mode = lfb_cur_GStateP->SGmode;

#ifdef FAST_MEMMOVE
    switch (mode) {
      case GXcopy:
	memmove(pDst, pSrc, wid * PSZ);
	return;
      case GXclear:
	memset(pDst, 0, wid * PSZ);
	return;
      case GXset:
	memset(pDst, 0xff, wid * PSZ);
	return;
    }
#endif

    switch (mode) {
      case GXclear:
	{
#if (PSZ == 1)
	    d1 = (PixelP)(((int)pDst + 0x3) & ~0x3);
	    wid -= d1 - pDst;

	    for (; pDst < d1; pDst++)
		pDst[0] = 0;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst[0] = 0;
		pDst++;
		wid--;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d += 4) {
		d[0] = 0;
		d[1] = 0;
		d[2] = 0;
		d[3] = 0;
	    }
#endif

	    dEnd = d + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d++)
		d[0] = 0;

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst + wid;

	    for (; pDst < d1; pDst++)
		pDst[0] = 0;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3)
		pDst[0] = 0;
#endif
	}
	break;

      case GXand:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] &= pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] &= pSrc[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] &= s[0];
		d[1] &= s[1];
		d[2] &= s[2];
		d[3] &= s[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] &= s[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] &= pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] &= pSrc[0];
#endif
	}
	break;

      case GXandReverse:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = pSrc[0] & ~pDst[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = pSrc[0] & ~pDst[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = s[0] & ~d[0];
		d[1] = s[1] & ~d[1];
		d[2] = s[2] & ~d[2];
		d[3] = s[3] & ~d[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = s[0] & ~d[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = pSrc[0] & ~pDst[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = pSrc[0] & ~pDst[0];
#endif
	}
	break;

      case GXcopy:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = pSrc[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = s[0];
		d[1] = s[1];
		d[2] = s[2];
		d[3] = s[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = s[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = pSrc[0];
#endif
	}
	break;

      case GXandInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] &= ~pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] &= ~pSrc[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] &= ~s[0];
		d[1] &= ~s[1];
		d[2] &= ~s[2];
		d[3] &= ~s[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] &= ~s[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] &= ~pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] &= ~pSrc[0];
#endif
	}
	break;

      case GXnoop:
	/* We should never get here */
	break;

      case GXxor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] ^= pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] ^= pSrc[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] ^= s[0];
		d[1] ^= s[1];
		d[2] ^= s[2];
		d[3] ^= s[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] ^= s[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] ^= pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] ^= pSrc[0];
#endif
	}
	break;

      case GXor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] |= pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] |= pSrc[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] |= s[0];
		d[1] |= s[1];
		d[2] |= s[2];
		d[3] |= s[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] |= s[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] |= pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] |= pSrc[0];
#endif
	}
	break;

      case GXnor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = ~(pSrc[0] | pDst[0]);
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = ~(pSrc[0] | pDst[0]);
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = ~(s[0] | d[0]);
		d[1] = ~(s[1] | d[1]);
		d[2] = ~(s[2] | d[2]);
		d[3] = ~(s[3] | d[3]);
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = ~(s[0] | d[0]);

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = ~(pSrc[0] | pDst[0]);
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = ~(pSrc[0] | pDst[0]);
#endif
	}
	break;

      case GXequiv:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] ^= ~pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] ^= ~pSrc[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] ^= ~s[0];
		d[1] ^= ~s[1];
		d[2] ^= ~s[2];
		d[3] ^= ~s[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] ^= ~s[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] ^= ~pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] ^= ~pSrc[0];
#endif
	}
	break;

      case GXinvert:
	{
#if (PSZ == 1)
	    d1 = (PixelP)(((int)pDst + 0x3) & ~0x3);
	    wid -= d1 - pDst;

	    for (; pDst < d1; pDst++)
		pDst[0] = ~pDst[0];
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst[0] = ~pDst[0];
		pDst++;
		wid--;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d += 4) {
		d[0] = ~d[0];
		d[1] = ~d[1];
		d[2] = ~d[2];
		d[3] = ~d[3];
	    }
#endif

	    dEnd = d + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d++)
		d[0] = ~d[0];

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst + wid;

	    for (; pDst < d1; pDst++)
		pDst[0] = ~pDst[0];
#elif (PSZ == 2)
	    if ((int)pDst & 0x3)
		pDst[0] = ~pDst[0];
#endif
	}
	break;

      case GXorReverse:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = pSrc[0] | ~pDst[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = pSrc[0] | ~pDst[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = s[0] | ~d[0];
		d[1] = s[1] | ~d[1];
		d[2] = s[2] | ~d[2];
		d[3] = s[3] | ~d[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = s[0] | ~d[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = pSrc[0] | ~pDst[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = pSrc[0] | ~pDst[0];
#endif
	}
	break;

      case GXcopyInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = ~pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = ~pSrc[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = ~s[0];
		d[1] = ~s[1];
		d[2] = ~s[2];
		d[3] = ~s[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = ~s[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = ~pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = ~pSrc[0];
#endif
	}
	break;

      case GXorInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] |= ~pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] |= ~pSrc[0];
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] |= ~s[0];
		d[1] |= ~s[1];
		d[2] |= ~s[2];
		d[3] |= ~s[3];
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] |= ~s[0];

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] |= ~pSrc[0];
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] |= ~pSrc[0];
#endif
	}
	break;

      case GXnand:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = ~(pSrc[0] & pDst[0]);
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = ~(pSrc[0] & pDst[0]);
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = ~(s[0] & d[0]);
		d[1] = ~(s[1] & d[1]);
		d[2] = ~(s[2] & d[2]);
		d[3] = ~(s[3] & d[3]);
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = ~(s[0] & d[0]);

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = ~(pSrc[0] & pDst[0]);
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = ~(pSrc[0] & pDst[0]);
#endif
	}
	break;

      case GXset:
	{
#if (PSZ == 1)
	    d1 = (PixelP)(((int)pDst + 0x3) & ~0x3);
	    wid -= d1 - pDst;

	    for (; pDst < d1; pDst++)
		pDst[0] = PMSK;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst[0] = PMSK;
		pDst++;
		wid--;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d += 4) {
		d[0] = PFILL(PMSK);
		d[1] = PFILL(PMSK);
		d[2] = PFILL(PMSK);
		d[3] = PFILL(PMSK);
	    }
#endif

	    dEnd = d + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d++)
		d[0] = PFILL(PMSK);

	    pDst = (PixelP)d;


#if (PSZ == 1)
	    d1 = pDst + wid;

	    for (; pDst < d1; pDst++)
		pDst[0] = PMSK;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3)
		pDst[0] = PMSK;
#endif
	}
	break;
    }
}

static void doRop_rl(pSrc, pDst, wid)
PixelP pSrc, pDst;
int wid;

{
    long  t, *s, *sEnd, *d, *dEnd;
    int cnt, mode;
    Pixel *d1, *s1;

    mode = lfb_cur_GStateP->SGmode;

#ifdef FAST_MEMMOVE
    switch (mode) {
      case GXcopy:
	memmove(pDst, pSrc, wid * PSZ);
	return;
      case GXclear:
	memset(pDst, 0, wid * PSZ);
	return;
      case GXset:
	memset(pDst, 0xff, wid * PSZ);
	return;
    }
#endif

    pSrc += wid;
    pDst += wid;

    switch (mode) {
      case GXclear:
	{
#if (PSZ == 1)
	    d1 = (PixelP)((int)pDst & ~0x3);
	    wid -= pDst - d1;

	    while (pDst > d1) {
		pDst--;
		pDst[0] = 0;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		wid--;
		pDst[0] = 0;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d -= 4;
		d[3] = 0;
		d[2] = 0;
		d[1] = 0;
		d[0] = 0;
	    }
#endif

	    dEnd = d - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d--;
		d[0] = 0;
	    }

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst - wid;

	    while (pDst > d1) {
		pDst--;
		pDst[0] = 0;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		pDst[0] = 0;
	    }
#endif
	}
	break;

      case GXand:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] &= pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] &= pSrc[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] &= s[3];
		d[2] &= s[2];
		d[1] &= s[1];
		d[0] &= s[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] &= s[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] &= pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] &= pSrc[0];
	    }
#endif
	}
	break;

      case GXandReverse:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (s1 < pSrc) {
		pSrc--;
		pDst--;
		pDst[0] = pSrc[0] & ~pDst[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = pSrc[0] & ~pDst[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = s[3] & ~d[3];
		d[2] = s[2] & ~d[2];
		d[1] = s[1] & ~d[1];
		d[0] = s[0] & ~d[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = s[0] & ~d[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = pSrc[0] & ~pDst[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = pSrc[0] & ~pDst[0];
	    }
#endif
	}
	break;

      case GXcopy:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = pSrc[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = s[3];
		d[2] = s[2];
		d[1] = s[1];
		d[0] = s[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = s[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = pSrc[0];
	    }
#endif
	}
	break;

      case GXandInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] &= ~pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] &= ~pSrc[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] &= ~s[3];
		d[2] &= ~s[2];
		d[1] &= ~s[1];
		d[0] &= ~s[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] &= ~s[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] &= ~pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] &= ~pSrc[0];
	    }
#endif
	}
	break;

      case GXnoop:
	/* We should never get here */
	break;

      case GXxor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] ^= pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] ^= pSrc[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] ^= s[3];
		d[2] ^= s[2];
		d[1] ^= s[1];
		d[0] ^= s[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] ^= s[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] ^= pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] ^= pSrc[0];
	    }
#endif
	}
	break;

      case GXor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] |= pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] |= pSrc[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] |= s[3];
		d[2] |= s[2];
		d[1] |= s[1];
		d[0] |= s[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] |= s[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] |= pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] |= pSrc[0];
	    }
#endif
	}
	break;

      case GXnor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = ~(pSrc[0] | pDst[0]);
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = ~(pSrc[0] | pDst[0]);
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = ~(s[3] | d[3]);
		d[2] = ~(s[2] | d[2]);
		d[1] = ~(s[1] | d[1]);
		d[0] = ~(s[0] | d[0]);
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = ~(s[0] | d[0]);
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = ~(pSrc[0] | pDst[0]);
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = ~(pSrc[0] | pDst[0]);
	    }
#endif
	}
	break;

      case GXequiv:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] ^= ~pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] ^= ~pSrc[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] ^= ~s[3];
		d[2] ^= ~s[2];
		d[1] ^= ~s[1];
		d[0] ^= ~s[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] ^= ~s[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] ^= ~pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] ^= ~pSrc[0];
	    }
#endif
	}
	break;

      case GXinvert:
	{
#if (PSZ == 1)
	    d1 = (PixelP)((int)pDst & ~0x3);
	    wid -= pDst - d1;

	    while (pDst > d1) {
		pDst--;
		pDst[0] = ~pDst[0];
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		wid--;
		pDst[0] = ~pDst[0];
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d -= 4;
		d[3] = ~d[3];
		d[2] = ~d[2];
		d[1] = ~d[1];
		d[0] = ~d[0];
	    }
#endif

	    dEnd = d - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d--;
		d[0] = ~d[0];
	    }

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst - wid;

	    while (pDst > d1) {
		pDst--;
		pDst[0] = ~pDst[0];
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		pDst[0] = ~pDst[0];
	    }
#endif
	}
	break;

      case GXorReverse:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = pSrc[0] | ~pDst[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = pSrc[0] | ~pDst[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = s[3] | ~d[3];
		d[2] = s[2] | ~d[2];
		d[1] = s[1] | ~d[1];
		d[0] = s[0] | ~d[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = s[0] | ~d[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = pSrc[0] | ~pDst[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = pSrc[0] | ~pDst[0];
	    }
#endif
	}
	break;

      case GXcopyInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = ~pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = ~pSrc[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = ~s[3];
		d[2] = ~s[2];
		d[1] = ~s[1];
		d[0] = ~s[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = ~s[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = ~pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = ~pSrc[0];
	    }
#endif
	}
	break;

      case GXorInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] |= ~pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] |= ~pSrc[0];
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] |= ~s[3];
		d[2] |= ~s[2];
		d[1] |= ~s[1];
		d[0] |= ~s[0];
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] |= ~s[0];
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] |= ~pSrc[0];
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] |= ~pSrc[0];
	    }
#endif
	}
	break;

      case GXnand:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = ~(pSrc[0] & pDst[0]);
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = ~(pSrc[0] & pDst[0]);
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = ~(s[3] & d[3]);
		d[2] = ~(s[2] & d[2]);
		d[1] = ~(s[1] & d[1]);
		d[0] = ~(s[0] & d[0]);
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = ~(s[0] & d[0]);
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = ~(pSrc[0] & pDst[0]);
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = ~(pSrc[0] & pDst[0]);
	    }
#endif
	}
	break;

      case GXset:
	{
#if (PSZ == 1)
	    d1 = (PixelP)((int)pDst & ~0x3);
	    wid -= pDst - d1;

	    while (pDst > d1) {
		pDst--;
		pDst[0] = PMSK;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		wid--;
		pDst[0] = PMSK;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d -= 4;
		d[0] = PFILL(PMSK);
		d[1] = PFILL(PMSK);
		d[2] = PFILL(PMSK);
		d[3] = PFILL(PMSK);
	    }
#endif

	    dEnd = d - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d--;
		d[0] = PFILL(PMSK);
	    }

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst - wid;

	    while (pDst > d1) {
		pDst--;
		pDst[0] = PMSK;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		pDst[0] = PMSK;
	    }
#endif
	}
	break;
    }
}

static void doRop_lr_msk(pSrc, pDst, wid)
PixelP pSrc, pDst;
int wid;

{
    long  t, msk, nmsk, *s, *sEnd, *d, *dEnd;
    int cnt, mode;
    Pixel *d1, *s1, mskp, nmskp;

    mskp = lfb_cur_GStateP->SGpmask;
    mode = lfb_cur_GStateP->SGmode;

#ifdef FAST_MEMMOVE
    if (mskp == PMSK) {
	switch (mode) {
	  case GXcopy:
	    memmove(pDst, pSrc, wid * PSZ);
	    return;
	  case GXclear:
	    memset(pDst, 0, wid * PSZ);
	    return;
	  case GXset:
	    memset(pDst, 0xff, wid * PSZ);
	    return;
	}
    }
#endif

    msk = PFILL(mskp);		/* pcc reloads the value for each access */
    nmskp = ~mskp;
    nmsk = ~msk;

    switch (mode) {
      case GXclear:
	{
#if (PSZ == 1)
	    d1 = (PixelP)(((int)pDst + 0x3) & ~0x3);
	    wid -= d1 - pDst;

	    for (; pDst < d1; pDst++)
		pDst[0] &= nmskp;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst[0] &= nmskp;
		pDst++;
		wid--;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d += 4) {
		d[0] &= nmsk;
		d[1] &= nmsk;
		d[2] &= nmsk;
		d[3] &= nmsk;
	    }
#endif

	    dEnd = d + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d++)
		d[0] &= nmsk;

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst + wid;

	    for (; pDst < d1; pDst++)
		pDst[0] &= nmskp;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3)
		pDst[0] &= nmskp;
#endif
	}
	break;

      case GXand:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & (s[0] & d[0]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] & d[1]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] & d[2]));
		d[3] = (nmsk & d[3]) | (msk & (s[3] & d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & (s[0] & d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & pDst[0]));
#endif
	}
	break;

      case GXandReverse:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & ~pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & ~pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & (s[0] & ~d[0]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] & ~d[1]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] & ~d[2]));
		d[3] = (nmsk & d[3]) | (msk & (s[3] & ~d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & (s[0] & ~d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & ~pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & ~pDst[0]));
#endif
	}
	break;

      case GXcopy:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & pSrc[0]);
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & pSrc[0]);
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & s[0]);
		d[1] = (nmsk & d[1]) | (msk & s[1]);
		d[2] = (nmsk & d[2]) | (msk & s[2]);
		d[3] = (nmsk & d[3]) | (msk & s[3]);
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & s[0]);

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & pSrc[0]);
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & pSrc[0]);
#endif
	}
	break;

      case GXandInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] & pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] & pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & (~s[0] & d[0]));
		d[1] = (nmsk & d[1]) | (msk & (~s[1] & d[1]));
		d[2] = (nmsk & d[2]) | (msk & (~s[2] & d[2]));
		d[3] = (nmsk & d[3]) | (msk & (~s[3] & d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & (~s[0] & d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] & pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] & pDst[0]));
#endif
	}
	break;

      case GXnoop:
	/* We should never get here */
	break;

      case GXxor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] ^ pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] ^ pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & (s[0] ^ d[0]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] ^ d[1]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] ^ d[2]));
		d[3] = (nmsk & d[3]) | (msk & (s[3] ^ d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & (s[0] ^ d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] ^ pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] ^ pDst[0]));
#endif
	}
	break;

      case GXor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & (s[0] | d[0]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] | d[1]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] | d[2]));
		d[3] = (nmsk & d[3]) | (msk & (s[3] | d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & (s[0] | d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | pDst[0]));
#endif
	}
	break;

      case GXnor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] | pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] | pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & ~(s[0] | d[0]));
		d[1] = (nmsk & d[1]) | (msk & ~(s[1] | d[1]));
		d[2] = (nmsk & d[2]) | (msk & ~(s[2] | d[2]));
		d[3] = (nmsk & d[3]) | (msk & ~(s[3] | d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & ~(s[0] | d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] | pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] | pDst[0]));
#endif
	}
	break;

      case GXequiv:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] ^ pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] ^ pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & (~s[0] ^ d[0]));
		d[1] = (nmsk & d[1]) | (msk & (~s[1] ^ d[1]));
		d[2] = (nmsk & d[2]) | (msk & (~s[2] ^ d[2]));
		d[3] = (nmsk & d[3]) | (msk & (~s[3] ^ d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & (~s[0] ^ d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] ^ pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] ^ pDst[0]));
#endif
	}
	break;

      case GXinvert:
	{
#if (PSZ == 1)
	    d1 = (PixelP)(((int)pDst + 0x3) & ~0x3);
	    wid -= d1 - pDst;

	    for (; pDst < d1; pDst++)
		pDst[0] ^= mskp;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst[0] ^= mskp;
		pDst++;
		wid--;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d += 4) {
		d[0] ^= msk;
		d[1] ^= msk;
		d[2] ^= msk;
		d[3] ^= msk;
	    }
#endif

	    dEnd = d + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d++)
		d[0] ^= msk;

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst + wid;

	    for (; pDst < d1; pDst++)
		pDst[0] ^= mskp;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3)
		pDst[0] ^= mskp;
#endif
	}
	break;

      case GXorReverse:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | ~pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | ~pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & (s[0] | ~d[0]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] | ~d[1]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] | ~d[2]));
		d[3] = (nmsk & d[3]) | (msk & (s[3] | ~d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & (s[0] | ~d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | ~pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | ~pDst[0]));
#endif
	}
	break;

      case GXcopyInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~pSrc[0]);
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~pSrc[0]);
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & ~s[0]);
		d[1] = (nmsk & d[1]) | (msk & ~s[1]);
		d[2] = (nmsk & d[2]) | (msk & ~s[2]);
		d[3] = (nmsk & d[3]) | (msk & ~s[3]);
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & ~s[0]);

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~pSrc[0]);
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~pSrc[0]);
#endif
	}
	break;

      case GXorInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] | pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] | pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & (~s[0] | d[0]));
		d[1] = (nmsk & d[1]) | (msk & (~s[1] | d[1]));
		d[2] = (nmsk & d[2]) | (msk & (~s[2] | d[2]));
		d[3] = (nmsk & d[3]) | (msk & (~s[3] | d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & (~s[0] | d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] | pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] | pDst[0]));
#endif
	}
	break;

      case GXnand:
	{

#if (PSZ == 1)
	    s1 = (PixelP)(((int)pSrc + 0x3) & ~0x3);
	    wid -= s1 - pSrc;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] & pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] & pDst[0]));
		pSrc++;
		pDst++;
		wid--;
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s += 4, d += 4) {
		d[0] = (nmsk & d[0]) | (msk & ~(s[0] & d[0]));
		d[1] = (nmsk & d[1]) | (msk & ~(s[1] & d[1]));
		d[2] = (nmsk & d[2]) | (msk & ~(s[2] & d[2]));
		d[3] = (nmsk & d[3]) | (msk & ~(s[3] & d[3]));
	    }
#endif

	    sEnd = s + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)sEnd - (PixelP)s;

	    for (; s < sEnd; s++, d++)
		d[0] = (nmsk & d[0]) | (msk & ~(s[0] & d[0]));

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc + wid;

	    for (; pSrc < s1; pSrc++, pDst++)
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] & pDst[0]));
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3)
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] & pDst[0]));
#endif
	}
	break;

      case GXset:
	{
#if (PSZ == 1)
	    d1 = (PixelP)(((int)pDst + 0x3) & ~0x3);
	    wid -= d1 - pDst;

	    for (; pDst < d1; pDst++)
		pDst[0] |= mskp;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst[0] |= mskp;
		pDst++;
		wid--;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d + ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d += 4) {
		d[0] |= msk;
		d[1] |= msk;
		d[2] |= msk;
		d[3] |= msk;
	    }
#endif

	    dEnd = d + (wid * PSZ / sizeof(long));
	    wid -= (PixelP)dEnd - (PixelP)d;

	    for (; d < dEnd; d++)
		d[0] |= msk;

	    pDst = (PixelP)d;


#if (PSZ == 1)
	    d1 = pDst + wid;

	    for (; pDst < d1; pDst++)
		pDst[0] |= mskp;
#elif (PSZ == 2)
	    if ((int)pDst & 0x3)
		pDst[0] |= mskp;
#endif
	}
	break;
    }
}

static void doRop_rl_msk(pSrc, pDst, wid)
PixelP pSrc, pDst;
int wid;

{
    long  t, msk, nmsk, *s, *sEnd, *d, *dEnd;
    int cnt, mode;
    Pixel *d1, *s1, mskp, nmskp;

    mskp = lfb_cur_GStateP->SGpmask;
    mode = lfb_cur_GStateP->SGmode;

#ifdef FAST_MEMMOVE
    if (mskp == PMSK) {
	switch (mode) {
	  case GXcopy:
	    memmove(pDst, pSrc, wid * PSZ);
	    return;
	  case GXclear:
	    memset(pDst, 0, wid * PSZ);
	    return;
	  case GXset:
	    memset(pDst, 0xff, wid * PSZ);
	    return;
	}
    }
#endif

    msk = PFILL(mskp);		/* pcc reloads the value for each access */
    nmskp = ~mskp;
    nmsk = ~msk;

    pSrc += wid;
    pDst += wid;

    switch (mode) {
      case GXclear:
	{
#if (PSZ == 1)
	    d1 = (PixelP)((int)pDst & ~0x3);
	    wid -= pDst - d1;

	    while (pDst > d1) {
		pDst--;
		pDst[0] &= nmskp;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		wid--;
		pDst[0] &= nmskp;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d -= 4;
		d[3] &= nmsk;
		d[2] &= nmsk;
		d[1] &= nmsk;
		d[0] &= nmsk;
	    }
#endif

	    dEnd = d - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d--;
		d[0] &= nmsk;
	    }

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst - wid;

	    while (pDst > d1) {
		pDst--;
		pDst[0] &= nmskp;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		pDst[0] &= nmskp;
	    }
#endif
	}
	break;

      case GXand:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & (s[3] & d[3]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] & d[2]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] & d[1]));
		d[0] = (nmsk & d[0]) | (msk & (s[0] & d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & (s[0] & d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & pDst[0]));
	    }
#endif
	}
	break;

      case GXandReverse:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (s1 < pSrc) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & ~pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & ~pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & (s[3] & ~d[3]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] & ~d[2]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] & ~d[1]));
		d[0] = (nmsk & d[0]) | (msk & (s[0] & ~d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & (s[0] & ~d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & ~pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] & ~pDst[0]));
	    }
#endif
	}
	break;

      case GXcopy:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & pSrc[0]);
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & pSrc[0]);
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & s[3]);
		d[2] = (nmsk & d[2]) | (msk & s[2]);
		d[1] = (nmsk & d[1]) | (msk & s[1]);
		d[0] = (nmsk & d[0]) | (msk & s[0]);
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & s[0]);
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & pSrc[0]);
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & pSrc[0]);
	    }
#endif
	}
	break;

      case GXandInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] & pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] & pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & (~s[3] & d[3]));
		d[2] = (nmsk & d[2]) | (msk & (~s[2] & d[2]));
		d[1] = (nmsk & d[1]) | (msk & (~s[1] & d[1]));
		d[0] = (nmsk & d[0]) | (msk & (~s[0] & d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & (~s[0] & d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] & pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] & pDst[0]));
	    }
#endif
	}
	break;

      case GXnoop:
	/* We should never get here */
	break;

      case GXxor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] ^ pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] ^ pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & (s[3] ^ d[3]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] ^ d[2]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] ^ d[1]));
		d[0] = (nmsk & d[0]) | (msk & (s[0] ^ d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & (s[0] ^ d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] ^ pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] ^ pDst[0]));
	    }
#endif
	}
	break;

      case GXor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & (s[3] | d[3]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] | d[2]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] | d[1]));
		d[0] = (nmsk & d[0]) | (msk & (s[0] | d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & (s[0] | d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | pDst[0]));
	    }
#endif
	}
	break;

      case GXnor:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] | pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] | pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & ~(s[3] | d[3]));
		d[2] = (nmsk & d[2]) | (msk & ~(s[2] | d[2]));
		d[1] = (nmsk & d[1]) | (msk & ~(s[1] | d[1]));
		d[0] = (nmsk & d[0]) | (msk & ~(s[0] | d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & ~(s[0] | d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] | pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] | pDst[0]));
	    }
#endif
	}
	break;

      case GXequiv:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] ^ pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] ^ pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & (~s[3] ^ d[3]));
		d[2] = (nmsk & d[2]) | (msk & (~s[2] ^ d[2]));
		d[1] = (nmsk & d[1]) | (msk & (~s[1] ^ d[1]));
		d[0] = (nmsk & d[0]) | (msk & (~s[0] ^ d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & (~s[0] ^ d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] ^ pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] ^ pDst[0]));
	    }
#endif
	}
	break;

      case GXinvert:
	{
#if (PSZ == 1)
	    d1 = (PixelP)((int)pDst & ~0x3);
	    wid -= pDst - d1;

	    while (pDst > d1) {
		pDst--;
		pDst[0] ^= mskp;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		wid--;
		pDst[0] ^= mskp;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d -= 4;
		d[3] ^= msk;
		d[2] ^= msk;
		d[1] ^= msk;
		d[0] ^= msk;
	    }
#endif

	    dEnd = d - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d--;
		d[0] ^= msk;
	    }

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst - wid;

	    while (pDst > d1) {
		pDst--;
		pDst[0] ^= mskp;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		pDst[0] ^= mskp;
	    }
#endif
	}
	break;

      case GXorReverse:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | ~pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | ~pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & (s[3] | ~d[3]));
		d[2] = (nmsk & d[2]) | (msk & (s[2] | ~d[2]));
		d[1] = (nmsk & d[1]) | (msk & (s[1] | ~d[1]));
		d[0] = (nmsk & d[0]) | (msk & (s[0] | ~d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & (s[0] | ~d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | ~pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (pSrc[0] | ~pDst[0]));
	    }
#endif
	}
	break;

      case GXcopyInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~pSrc[0]);
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~pSrc[0]);
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & ~s[3]);
		d[2] = (nmsk & d[2]) | (msk & ~s[2]);
		d[1] = (nmsk & d[1]) | (msk & ~s[1]);
		d[0] = (nmsk & d[0]) | (msk & ~s[0]);
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & ~s[0]);
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~pSrc[0]);
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~pSrc[0]);
	    }
#endif
	}
	break;

      case GXorInverted:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] | pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] | pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & (~s[3] | d[3]));
		d[2] = (nmsk & d[2]) | (msk & (~s[2] | d[2]));
		d[1] = (nmsk & d[1]) | (msk & (~s[1] | d[1]));
		d[0] = (nmsk & d[0]) | (msk & (~s[0] | d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & (~s[0] | d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] | pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & (~pSrc[0] | pDst[0]));
	    }
#endif
	}
	break;

      case GXnand:
	{

#if (PSZ == 1)
	    s1 = (PixelP)((int)pSrc & ~0x3);
	    wid -= pSrc - s1;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] & pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		wid--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] & pDst[0]));
	    }
#endif

	    s = (long *)pSrc;
	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    sEnd = s - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s -= 4;
		d -= 4;
		d[3] = (nmsk & d[3]) | (msk & ~(s[3] & d[3]));
		d[2] = (nmsk & d[2]) | (msk & ~(s[2] & d[2]));
		d[1] = (nmsk & d[1]) | (msk & ~(s[1] & d[1]));
		d[0] = (nmsk & d[0]) | (msk & ~(s[0] & d[0]));
	    }
#endif

	    sEnd = s - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)s - (PixelP)sEnd;

	    while (s > sEnd) {
		s--;
		d--;
		d[0] = (nmsk & d[0]) | (msk & ~(s[0] & d[0]));
	    }

	    pSrc = (PixelP)s;
	    pDst = (PixelP)d;

#if (PSZ == 1)
	    s1 = pSrc - wid;

	    while (pSrc > s1) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] & pDst[0]));
	    }
#elif (PSZ == 2)
	    if ((int)pSrc & 0x3) {
		pSrc--;
		pDst--;
		pDst[0] = (nmskp & pDst[0]) | (mskp & ~(pSrc[0] & pDst[0]));
	    }
#endif
	}
	break;

      case GXset:
	{
#if (PSZ == 1)
	    d1 = (PixelP)((int)pDst & ~0x3);
	    wid -= pDst - d1;

	    while (pDst > d1) {
		pDst--;
		pDst[0] = mskp;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		wid--;
		pDst[0] |= mskp;
	    }
#endif

	    d = (long *)pDst;

#ifndef COMPILER_UNROLLS_LOOPS
	    dEnd = d - ((wid * PSZ / sizeof(long)) & ~0x3);
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d -= 4;
		d[0] |= msk;
		d[1] |= msk;
		d[2] |= msk;
		d[3] |= msk;
	    }
#endif

	    dEnd = d - (wid * PSZ / sizeof(long));
	    wid -= (PixelP)d - (PixelP)dEnd;

	    while (d > dEnd) {
		d--;
		d[0] |= msk;
	    }

	    pDst = (PixelP)d;

#if (PSZ == 1)
	    d1 = pDst - wid;

	    while (pDst > d1) {
		pDst--;
		pDst[0] |= mskp;
	    }
#elif (PSZ == 2)
	    if ((int)pDst & 0x3) {
		pDst--;
		pDst[0] |= mskp;
	    }
#endif
	}
	break;
    }
}

static void small_doRop_lr(pSrc, pDst, wid, copyDir)
PixelP pSrc, pDst;
int wid, copyDir;
    
{
    PixelP sEnd;
    Pixel t, mskp;
    
    sEnd = pSrc + wid;
    
    for (; pSrc < sEnd; pSrc++, pDst++) {
	switch (lfb_cur_GStateP->SGmode) {
	  case GXclear:
	    t = 0;
	    break;
	  case GXand:
	    t = *pSrc & *pDst;
	    break;
	  case GXandReverse:
	    t = *pSrc & ~*pDst;
	    break;
	  case GXcopy:
	    t = *pSrc;
	    break;
	  case GXandInverted:
	    t = ~*pSrc & *pDst;
	    break;
	  case GXnoop:
	    t = *pDst;
	    break;
	  case GXxor:
	    t = *pSrc ^ *pDst;
	    break;
	  case GXor:
	    t = *pSrc | *pDst;
	    break;
	  case GXnor:
	    t = ~(*pSrc | *pDst);
	    break;
	  case GXequiv:
	    t = ~*pSrc ^ *pDst;
	    break;
	  case GXinvert:
	    t = ~*pDst;
	    break;
	  case GXorReverse:
	    t = *pSrc | ~*pDst;
	    break;
	  case GXcopyInverted:
	    t = ~*pSrc;
	    break;
	  case GXorInverted:
	    t = ~*pSrc | *pDst;
	    break;
	  case GXnand:
	    t = ~(*pSrc & *pDst);
	    break;
	  case GXset:
	    t = PMSK;
	}
	
	if ((mskp =lfb_cur_GStateP->SGpmask) == PMSK)
	    *pDst = t;
	else
	    *pDst = ((*pDst & ~mskp) | (t & mskp));
    }
}

static void small_doRop_rl(pSrc, pDst, wid, copyDir)
PixelP pSrc, pDst;
int wid, copyDir;
    
{
    PixelP sEnd;
    Pixel t, mskp;
    
    sEnd = pSrc;
    pSrc += wid;
    pDst += wid;
    
    while (pSrc > sEnd) {
	pSrc--;
	pDst--;

	switch (lfb_cur_GStateP->SGmode) {
	  case GXclear:
	    t = 0;
	    break;
	  case GXand:
	    t = *pSrc & *pDst;
	    break;
	  case GXandReverse:
	    t = *pSrc & ~*pDst;
	    break;
	  case GXcopy:
	    t = *pSrc;
	    break;
	  case GXandInverted:
	    t = ~*pSrc & *pDst;
	    break;
	  case GXnoop:
	    t = *pDst;
	    break;
	  case GXxor:
	    t = *pSrc ^ *pDst;
	    break;
	  case GXor:
	    t = *pSrc | *pDst;
	    break;
	  case GXnor:
	    t = ~(*pSrc | *pDst);
	    break;
	  case GXequiv:
	    t = ~*pSrc ^ *pDst;
	    break;
	  case GXinvert:
	    t = ~*pDst;
	    break;
	  case GXorReverse:
	    t = *pSrc | ~*pDst;
	    break;
	  case GXcopyInverted:
	    t = ~*pSrc;
	    break;
	  case GXorInverted:
	    t = ~*pSrc | *pDst;
	    break;
	  case GXnand:
	    t = ~(*pSrc & *pDst);
	    break;
	  case GXset:
	    t = PMSK;
	}
	
	if ((mskp =lfb_cur_GStateP->SGpmask) == PMSK)
	    *pDst = t;
	else
	    *pDst = ((*pDst & ~mskp) | (t & mskp));
    }
}

static SIBool doBitblt(pSrc, pDst, strideSrc, strideDst, wid, hgt, copyDir)
PixelP pSrc, pDst;
int strideSrc, strideDst;	/* Measured in pixels */
int wid, hgt, copyDir;

{
    int i;

    if ((wid == 0) || (hgt == 0))
	return(SI_SUCCEED);

    /*
     * If the drawing mode is noop, or the GState plane mask is 0, no
     * drawing will take place.
     */

    if ((lfb_cur_GStateP->SGmode == GXnoop) ||
	(lfb_cur_GStateP->SGpmask == 0)) {
	return (SI_SUCCEED);
    }

    /* 
     * Flush the graphics hardware
     *
     */
    if (lfbVendorFlush)
	(*lfbVendorFlush)();

#if (PSZ == 1)
    /*
     * PSZ == 1 is special.  The regular rop routines blindly copy up to 3 
     * bytes when they adjust the pointer to a long boundary.  This can 
     * kill us if there are less bytes than they would copy.  Other cases 
     * are safe.
     */

    if (wid < 3) {
	switch (copyDir) {

	  case COPY_BOT_TOP:	/* Bottom to top, left to right */

	    pSrc += strideSrc * (hgt - 1);
	    pDst += strideDst * (hgt - 1);

	    for (; hgt--; pSrc -= strideSrc, pDst -= strideDst)
		small_doRop_lr(pSrc, pDst, wid, copyDir);

	    break;

	  case COPY_RGT_LFT:	/* Top to bottom, right to left */

	    for (; hgt--; pSrc += strideSrc, pDst += strideDst)
		small_doRop_rl(pSrc, pDst, wid, copyDir);

	    break;

	  case COPY_NORMAL:		/* Top to bottom, left to right */

	    for (; hgt--; pSrc += strideSrc, pDst += strideDst)
		small_doRop_lr(pSrc, pDst, wid, copyDir);

	    break;
	}

	return(SI_SUCCEED);
    }
#endif

    if (lfb_cur_GStateP->SGpmask == PMSK) {
	switch (copyDir) {

	  case COPY_BOT_TOP:	/* Bottom to top, left to right */

	    pSrc += strideSrc * (hgt - 1);
	    pDst += strideDst * (hgt - 1);

	    for (; hgt--; pSrc -= strideSrc, pDst -= strideDst)
		doRop_lr(pSrc, pDst, wid);

	    break;

	  case COPY_RGT_LFT:	/* Top to bottom, right to left */

	    for (; hgt--; pSrc += strideSrc, pDst += strideDst)
		doRop_rl(pSrc, pDst, wid);

	    break;

	  case COPY_NORMAL:		/* Top to bottom, left to right */

	    for (; hgt--; pSrc += strideSrc, pDst += strideDst)
		doRop_lr(pSrc, pDst, wid);

	    break;
	}
    }
    else {
	switch (copyDir) {

	  case COPY_BOT_TOP:	/* Bottom to top, left to right */

	    pSrc += strideSrc * (hgt - 1);
	    pDst += strideDst * (hgt - 1);

	    for (; hgt--; pSrc -= strideSrc, pDst -= strideDst)
		doRop_lr_msk(pSrc, pDst, wid);

	    break;

	  case COPY_RGT_LFT:	/* Top to bottom, right to left */

	    for (; hgt--; pSrc += strideSrc, pDst += strideDst)
		doRop_rl_msk(pSrc, pDst, wid);

	    break;

	  case COPY_NORMAL:		/* Top to bottom, left to right */

	    for (; hgt--; pSrc += strideSrc, pDst += strideDst)
		doRop_lr_msk(pSrc, pDst, wid);

	    break;
	}
    }

    return(SI_SUCCEED);
}

/*	HARDWARE BITBLT ROUTINES	*/
/*		OPTIONAL		*/

SIBool lfbSSbitblt(sx, sy, dx, dy, wid, hgt)
SIint32 sx, sy, dx, dy, wid, hgt;
{
    SIBool val;
    int copyDir = COPY_NORMAL;

    /*
     * Check for overlap.  There are two cases we need to deal with:
     *
     * 1. The upper left corner of the dst is inside the src area, but is
     *    not on the first scan line.  In this case, copy from bottom to
     *    top.
     *
     * 2. The upper left corner of the dst is on the first scan line of the
     *    src.  In this case, we need to copy right to left along the scan
     *    lines.
     *
     * In all other cases, we will copy top to bottom, left to right.
     */

    if (dy > sy)
	copyDir = COPY_BOT_TOP;
    else if (dy == sy)
	if (dx > sx)
	    copyDir = COPY_RGT_LFT;

    val = doBitblt(ScreenAddr(sx, sy), ScreenAddr(dx, dy),
		   lfb.stride, lfb.stride,
		   wid, hgt, copyDir);

    return(val);
}

SIBool lfbMSbitblt(src, sx, sy, dx, dy, wid, hgt)
SIbitmapP src;
SIint32 sx, sy, dx, dy, wid, hgt;
{
    SIBool val;

    if (src->BbitsPerPixel != BPP)
	fprintf(stderr, "MS Bitblt: Bad depth");

    if ((src->Btype != Z_BITMAP) &&
	(src->Btype != Z_PIXMAP))
	return(SI_FAIL);

    val = doBitblt((PixelP)BitmapAddr(src, sx, sy), ScreenAddr(dx, dy),
		   BitmapStride(src) / sizeof(Pixel), lfb.stride,
		   wid, hgt, COPY_NORMAL);

    return(val);
}

SIBool lfbSMbitblt(dst, sx, sy, dx, dy, wid, hgt)
SIbitmapP dst;
SIint32 sx, sy, dx, dy, wid, hgt;
{
    SIBool val;

    if (dst->BbitsPerPixel != BPP)
	fprintf(stderr, "SM Bitblt: Bad depth");

    if ((dst->Btype != Z_BITMAP) &&
	(dst->Btype != Z_PIXMAP))
	return(SI_FAIL);

    val = doBitblt(ScreenAddr(sx, sy), (PixelP)BitmapAddr(dst, dx, dy),
		   lfb.stride, BitmapStride(dst) / sizeof(Pixel),
		   wid, hgt, COPY_NORMAL);

    return(val);
}
