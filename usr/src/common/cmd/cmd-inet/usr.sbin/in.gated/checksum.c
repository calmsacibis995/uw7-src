#ident	"@(#)checksum.c	1.3"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#include "include.h"

/*
 * Checksum routine for Internet Protocol - Modified from 4.3+ networking in_chksum.c
 * 
 */

#define ADDCARRY(x)  (x > 65535 ? x -= 65535 : x)
#define REDUCE {l_util.l = sum; sum = l_util.s[0] + l_util.s[1]; ADDCARRY(sum);}

u_int16
inet_cksumv __PF3(v, register struct iovec *,
		  nv, register int,
		  len, register size_t)
{
    register int sum = 0;
    register int vlen = 0;
    register struct iovec *vp;
    int byte_swapped = 0;
    union {
	char c[2];
	u_int16 s;
    } s_util;
    union {
	u_int16 s[2];
	long l;
    } l_util;

    for (vp = v; nv && len; nv--, vp++) {
	register union {
	    caddr_t c;
	    u_int16 *s;
	} w;

	if (vp->iov_len == 0) {
	    continue;
	}
	w.c = vp->iov_base;
	if (vlen == -1) {

	    /*
	     * The first byte of this mbuf is the continuation of a word spanning between this mbuf and the last mbuf.
	     * 
	     * s_util.c[0] is already saved when scanning previous mbuf.
	     */
	    s_util.c[1] = *w.c;
	    sum += s_util.s;
	    w.c++;
	    vlen = vp->iov_len - 1;
	    len--;
	} else {
	    vlen = vp->iov_len;
	}
	if (len < vlen) {
	    vlen = len;
	}
	len -= vlen;

	/*
	 * Force to even boundary.
	 */
	if ((1 & GA2S(w.s)) && (vlen > 0)) {
	    REDUCE;
	    sum <<= NBBY;
	    s_util.c[0] = *w.c;
	    w.c++;
	    vlen--;
	    byte_swapped = 1;
	}

	/*
	 * Unroll the loop to make overhead from branches &c small.
	 */
	while ((vlen -= 32) >= 0) {
	    sum += w.s[0];
	    sum += w.s[1];
	    sum += w.s[2];
	    sum += w.s[3];
	    sum += w.s[4];
	    sum += w.s[5];
	    sum += w.s[6];
	    sum += w.s[7];
	    sum += w.s[8];
	    sum += w.s[9];
	    sum += w.s[10];
	    sum += w.s[11];
	    sum += w.s[12];
	    sum += w.s[13];
	    sum += w.s[14];
	    sum += w.s[15];
	    w.s += 16;
	}
	vlen += 32;
	while ((vlen -= 8) >= 0) {
	    sum += w.s[0];
	    sum += w.s[1];
	    sum += w.s[2];
	    sum += w.s[3];
	    w.s += 4;
	}
	vlen += 8;
	if (vlen == 0 && byte_swapped == 0) {
	    continue;
	}
	REDUCE;
	while ((vlen -= 2) >= 0) {
	    sum += *w.s++;
	}
	if (byte_swapped) {
	    REDUCE;
	    sum <<= NBBY;
	    byte_swapped = 0;
	    if (vlen == -1) {
		s_util.c[1] = *w.c;
		sum += s_util.s;
		vlen = 0;
	    } else {
		vlen = -1;
	    }
	} else if (vlen == -1) {
	    s_util.c[0] = *w.c;
	}
    }
    assert(!len);
    if (vlen == -1) {

	/*
	 * The last buffer has odd # of bytes. Follow the standard (the odd byte may be shifted left by 8 bits or not as
	 * determined by endian-ness of the machine)
	 */
	s_util.c[1] = 0;
	sum += s_util.s;
    }
    REDUCE;
    return ~sum & 0xffff;
}


u_int16
inet_cksum __PF2(cp, void_t,
		 length, size_t)
{
    struct iovec iovec;

    iovec.iov_base = (caddr_t) cp;
    iovec.iov_len = (int) length;

    return inet_cksumv(&iovec, 1, length);
}


#ifdef	FLETCHER_CHECKSUM

/*
 * iso_cksum.c - compute the ISO (Fletcher) checksum.  Can be used for both computing the checksum and inserting it in the
 * packet, and for checking an already-checksummed packet
 */

/*
 * The variables c0, c1 and l (and X and Y, though these were eliminated) are as in ISO 8073.
 * 
 * The limit on the number of bytes to process before doing a MOD (MAXITER) is derived from what it takes to avoid overflowing
 * the 31 bit c1 during the summation (I computed an actual limit of 4102 for the worst case packet, which is conveniently
 * close to 4096).  Doing this minimizes the number of divisions which must be done.
 * 
 * It is hard to make this checksum go fast.  It very much requires byte-at-a-time processing, and sequential processing since
 * all computations are dependent on immediately previous results, so trying to run with 32-bit loads ends up losing much of
 * the potential advantage to complexity.
 * 
 * This implementation is a compromise.  Beyond minimizing the number of divides (the old OSPF checksum was doing one extra) it
 * inlines all inner loops 8-at-a-time.  This gives compilers an opportunity to fill at least some of the load-delay slots
 * by code rearrangement.  The results of this are dependent on the particular machine and compiler, but in no case has the
 * speed been observed to be worse than the old OSPF code even for short packets, and for a few machines the speedup is a
 * factor of two or better (for MIPS machines and compilers, in particular).
 */

#define	INLINE		8		/* number of inline computations */
#define	INSHIFT		3		/* i.e. inlined by 1<<INSHIFT */
#define	INLINEMASK	(INLINE - 1)	/* mask for uneven bits */

#define	MAXITER		4096		/* number of iterations before MOD */
#define	ITERSHIFT	12		/* log2 of number of interations */
#define	ITERMASK	(MAXITER - 1)	/* mask to tell if we need mod or not */
#define	MAXINLINE	(MAXITER/INLINE)/* number of inline iterations */

#define	MODULUS		255		/* modulus for checksum */

/*
 * The arguments are as follows
 * 
 * pkt   - start of packet to be checksummed len   - contiguous length of packet to be checksummed cksum - optional pointer to
 * location of checksum in packet.  If non-NULL we initialize to zero and fill in the checksum when done.
 * 
 * If cksum is non-NULL the return value is the value which was inserted in the packet, in host byte order.  If cksum is NULL
 * the results of the checksum sum are returned.  The return value should be zero for an already-checksummed packet.
 */
u_int32
iso_cksum __PF3(pkt, void_t,
		len, size_t,
		cksum, byte *)
{
    register s_int32 c0, c1;
    register byte *cp;
    register int n;
    register int l;

    c0 = c1 = 0;
    cp = (byte *) pkt;
    l = len;

    /*
     * Initialize checksum to zero if there is one
     */
    if (cksum) {
	*cksum = *(cksum + 1) = 0;
    }

    /*
     * Process enough of the packet to make the remaining length an even multiple of MAXITER (4096).  The switch() adds a
     * lot of code, but trying to do this with less results in a big slowdown for short packets.
     */
    n = l & ITERMASK;
    switch (n & INLINEMASK) {
    case 7:
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	break;
    case 6:
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	break;
    case 5:
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	break;
    case 4:
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	break;
    case 3:
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	break;
    case 2:
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	break;
    case 1:
	c1 += (c0 += (s_int32) (*cp++));
	break;
    case 0:
	break;
    }

    n >>= INSHIFT;
    while (n-- > 0) {
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
	c1 += (c0 += (s_int32) (*cp++));
    }

    /*
     * Now process the remainder in MAXITER chunks, with a mod beforehand to avoid overflow.
     */
    n = l >> ITERSHIFT;
    if (n > 0) {
	do {
	    register int iter = MAXINLINE;

	    if (cp != (byte *) pkt) {
		c0 %= MODULUS;
		c1 %= MODULUS;
	    }
	    do {
		c1 += (c0 += (s_int32) (*cp++));
		c1 += (c0 += (s_int32) (*cp++));
		c1 += (c0 += (s_int32) (*cp++));
		c1 += (c0 += (s_int32) (*cp++));
		c1 += (c0 += (s_int32) (*cp++));
		c1 += (c0 += (s_int32) (*cp++));
		c1 += (c0 += (s_int32) (*cp++));
		c1 += (c0 += (s_int32) (*cp++));
	    } while (--iter > 0);
	} while (--n > 0);
    }

    /*
     * Take the modulus of c0 now to avoid overflow during the multiplication below.  If we're computing a checksum for the
     * packet, do it and insert it.
     */
    c0 %= MODULUS;
    if (cksum) {

	/*
	 * c1 used for Y.  Can't overflow since we're taking the difference between two positive numbers
	 */
	c1 = (c1 - ((s_int32) (((byte *) pkt + l) - cksum) * c0)) % MODULUS;
	if (c1 <= 0) {
	    c1 += MODULUS;
	}

	/*
	 * Here we know c0 has a value in the range 0-254, and c1 has a value in the range 1-255.  If we subtract the sum
	 * from 255 we end up with something in the range -254-254, and only need correct the -ve value.
	 */
	c0 = MODULUS - c1 - c0;		/* c0 used for X */
	if (c0 <= 0) {
	    c0 += MODULUS;
	}
	*cksum++ = (byte) c0;
	*cksum = (byte) c1;

	return (u_int32) ((c0 << 8) | c1);
    }

    /*
     * Here we were just doing a check.  Return the results in a single value, they should both be zero.
     */
    return (u_int32) (((c1 % MODULUS) << 8) | c0);
}

#endif /* FLETCHER_CHECKSUM */

/**/

#ifdef	MD5_CHECKSUM
/*
 * This is an implementation of the MD5 algorithm from RFC1321.  It
 * is not too dissimilar to the RFC implementation.  The basic
 * differences are that the code has been neatened up and redone
 * to keep the data-handling inline, the implementation is biased
 * towards digesting short data blocks quickly (at the expense of
 * text size, probably), and the interface has been redone to be more
 * convenient for processing protocol packets.
 *
 * The implementation takes spends 60% and 75% of the time the RFC
 * reference implementation to digest large blocks (the improvement is
 * greater on a sparc than on an i486) and is probably relatively faster
 * still for small blocks.  The text size is bloated to 2-3x that of
 * the reference implementation (increased from 2.2-3.8kB to 7-8kB).
 */

/*
 * The basic MD5 functions.  See the spec.
 */
#define	MD5_F(x, y, z)	(((~(x)) & (z)) | ((x) & (y)))
#define	MD5_G(x, y, z)	(((~(z)) & (y)) | ((z) & (x)))	/* or MD5_F(z, x, y) */
#define	MD5_H(x, y, z)	((x) ^ (y) ^ (z))
#define	MD5_I(x, y, z)	((y) ^ ((x) | (~(z))))

/*
 * A left rotation.  This might be productively recoded in assembler for
 * particular machines which have an instruction for this, though a smart
 * compiler might be able to figure this out from the code as well.
 *
 * The mask operation makes this work even if the u_int32 datatype is not
 * actually 32 bits.  With any luck the compiler will delete the mask as
 * a no-op.
 */
#ifndef	MD5_ROTL
#define	MD5_ROTL(x, s) \
    ((x) = ((x) << (s)) | (((x) >> (32 - (s))) & 0xffffffff))
#endif	/* MD5_ROTL */

/*
 * MD5 round operations.  These define the operations which are
 * done during each of the four rounds.
 */
#define	MD5_OP1(a, b, c, d, xk, s, ti) \
    do { \
	(a) += MD5_F((b), (c), (d)) + (xk) + (ti); \
	MD5_ROTL((a), (s)); \
	(a) += (b); \
    } while (0)

#define	MD5_OP2(a, b, c, d, xk, s, ti) \
    do { \
	(a) += MD5_G((b), (c), (d)) + (xk) + (ti); \
	MD5_ROTL((a), (s)); \
	(a) += (b); \
    } while (0)

#define	MD5_OP3(a, b, c, d, xk, s, ti) \
    do { \
	(a) += MD5_H((b), (c), (d)) + (xk) + (ti); \
	MD5_ROTL((a), (s)); \
	(a) += (b); \
    } while (0)

#define	MD5_OP4(a, b, c, d, xk, s, ti) \
    do { \
	(a) += MD5_I((b), (c), (d)) + (xk) + (ti); \
	MD5_ROTL((a), (s)); \
	(a) += (b); \
    } while (0)


/*
 * Initial values for A, B, C and D
 */
#define	MD5_A_INIT	0x67452301
#define	MD5_B_INIT	0xefcdab89
#define	MD5_C_INIT	0x98badcfe
#define	MD5_D_INIT	0x10325476

/*
 * Get and put routines for assembling and writing four byte words
 */
#define	MD5_GET(x, cp) \
    do { \
	register u_int32 Xtmp; \
	Xtmp = (u_int32)(*(cp)++); \
	Xtmp |= ((u_int32)(*(cp)++)) << 8; \
	Xtmp |= ((u_int32)(*(cp)++)) << 16; \
	Xtmp |= ((u_int32)(*(cp)++)) << 24; \
	(x) = Xtmp; \
    } while (0)

#define	MD5_PUT(x, cp) \
    do { \
	register u_int32 Xtmp = (x); \
	*(cp)++ = (byte) Xtmp; \
	*(cp)++ = (byte) (Xtmp >> 8); \
	*(cp)++ = (byte) (Xtmp >> 16); \
	*(cp)++ = (byte) (Xtmp >> 24); \
    } while (0)


/*
 * md5_cksum_block - checksum (perhaps incompletely) a data packet
 */
static void
md5_cksum_block __PF5(data, void_t,
		      datalen, size_t,
		      totallen, size_t,
		      incomplete, int,
		      results, u_int32 *)
{
    register u_int32 a, b, c, d;
    int alldone = 0;

    /*
     * Fetch the initial values of the accumulators
     */
    a = results[0];
    b = results[1];
    c = results[2];
    d = results[3];

    /*
     * Work on the data first
     */
    {
	register byte *dp;
	register size_t dlen;
	register u_int32 *xp;
	u_int32 x[64];

	/*
	 * Initialize data pointer/length
	 */
	dp = (byte *) data;
	dlen = datalen;

	while (dlen) {
	    xp = x;
	    if (dlen >= 64) {
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp); xp++;
		MD5_GET(*xp, dp);
		dlen -= 64;
	    } else {
		assert(!incomplete);
		switch (dlen >> 2) {
		case 15:
		    MD5_GET(*xp, dp); xp++;
		case 14:
		    MD5_GET(*xp, dp); xp++;
		case 13:
		    MD5_GET(*xp, dp); xp++;
		case 12:
		    MD5_GET(*xp, dp); xp++;
		case 11:
		    MD5_GET(*xp, dp); xp++;
		case 10:
		    MD5_GET(*xp, dp); xp++;
		case 9:
		    MD5_GET(*xp, dp); xp++;
		case 8:
		    MD5_GET(*xp, dp); xp++;
		case 7:
		    MD5_GET(*xp, dp); xp++;
		case 6:
		    MD5_GET(*xp, dp); xp++;
		case 5:
		    MD5_GET(*xp, dp); xp++;
		case 4:
		    MD5_GET(*xp, dp); xp++;
		case 3:
		    MD5_GET(*xp, dp); xp++;
		case 2:
		    MD5_GET(*xp, dp); xp++;
		case 1:
		    MD5_GET(*xp, dp); xp++;
		default:
		    break;
		}

		switch (dlen & 0x3) {
		case 3:
		    *xp = (u_int32) 0x80000000 | (u_int32)(*dp++);
		    *xp |= (u_int32)(*dp++) << 8;
		    *xp++ |= (u_int32)(*dp++) << 16;
		    break;
		case 2:
		    *xp = (u_int32) 0x800000 | (u_int32)(*dp++);
		    *xp++ |= (u_int32)(*dp++) << 8;
		    break;
		case 1:
		    *xp++ = (u_int32) 0x8000 | (u_int32)(*dp++);
		    break;
		default:
		    *xp++ = (u_int32) 0x80;
		    break;
		}

		if (xp >= &(x[15])) {
		    if (xp == &(x[15])) {
			*xp = 0;
		    }
		} else {
		    while (xp < &(x[14])) {
			*xp++ = 0;
		    }
		    *xp++ = (((u_int32)(totallen)) << 3) & 0xffffffff;
		    *xp = (u_int32)((totallen >> 29) & 0xffffffff);
		    alldone = 1;
		}
		dlen = 0;
	    }

	    /*
	     * Done, x contains the block to screw around with.  Run the
	     * MD5 algorithm over the block.  Initialize the variables
	     * we'll be using.
	     */
	    xp = x;

	    /* round 1 */
	    MD5_OP1(a, b, c, d, xp[ 0],  7, 0xd76aa478);	/* 0 */
	    MD5_OP1(d, a, b, c, xp[ 1], 12, 0xe8c7b756);	/* 1 */
	    MD5_OP1(c, d, a, b, xp[ 2], 17, 0x242070db);	/* 2 */
	    MD5_OP1(b, c, d, a, xp[ 3], 22, 0xc1bdceee);	/* 3 */
	    MD5_OP1(a, b, c, d, xp[ 4],  7, 0xf57c0faf);	/* 4 */
	    MD5_OP1(d, a, b, c, xp[ 5], 12, 0x4787c62a);	/* 5 */
	    MD5_OP1(c, d, a, b, xp[ 6], 17, 0xa8304613);	/* 6 */
	    MD5_OP1(b, c, d, a, xp[ 7], 22, 0xfd469501);	/* 7 */
	    MD5_OP1(a, b, c, d, xp[ 8],  7, 0x698098d8);	/* 8 */
	    MD5_OP1(d, a, b, c, xp[ 9], 12, 0x8b44f7af);	/* 9 */
	    MD5_OP1(c, d, a, b, xp[10], 17, 0xffff5bb1);	/* 10 */
	    MD5_OP1(b, c, d, a, xp[11], 22, 0x895cd7be);	/* 11 */
	    MD5_OP1(a, b, c, d, xp[12],  7, 0x6b901122);	/* 12 */
	    MD5_OP1(d, a, b, c, xp[13], 12, 0xfd987193);	/* 13 */
	    MD5_OP1(c, d, a, b, xp[14], 17, 0xa679438e);	/* 14 */
	    MD5_OP1(b, c, d, a, xp[15], 22, 0x49b40821);	/* 15 */

	    /* round 2 */
	    MD5_OP2(a, b, c, d, xp[ 1],  5, 0xf61e2562);	/* 16 */
	    MD5_OP2(d, a, b, c, xp[ 6],  9, 0xc040b340);	/* 17 */
	    MD5_OP2(c, d, a, b, xp[11], 14, 0x265e5a51);	/* 18 */
	    MD5_OP2(b, c, d, a, xp[ 0], 20, 0xe9b6c7aa);	/* 19 */
	    MD5_OP2(a, b, c, d, xp[ 5],  5, 0xd62f105d);	/* 20 */
	    MD5_OP2(d, a, b, c, xp[10],  9, 0x02441453);	/* 21 */
	    MD5_OP2(c, d, a, b, xp[15], 14, 0xd8a1e681);	/* 22 */
	    MD5_OP2(b, c, d, a, xp[ 4], 20, 0xe7d3fbc8);	/* 23 */
	    MD5_OP2(a, b, c, d, xp[ 9],  5, 0x21e1cde6);	/* 24 */
	    MD5_OP2(d, a, b, c, xp[14],  9, 0xc33707d6);	/* 25 */
	    MD5_OP2(c, d, a, b, xp[ 3], 14, 0xf4d50d87);	/* 26 */
	    MD5_OP2(b, c, d, a, xp[ 8], 20, 0x455a14ed);	/* 27 */
	    MD5_OP2(a, b, c, d, xp[13],  5, 0xa9e3e905);	/* 28 */
	    MD5_OP2(d, a, b, c, xp[ 2],  9, 0xfcefa3f8);	/* 29 */
	    MD5_OP2(c, d, a, b, xp[ 7], 14, 0x676f02d9);	/* 30 */
	    MD5_OP2(b, c, d, a, xp[12], 20, 0x8d2a4c8a);	/* 31 */

	    /* round 3 */
	    MD5_OP3(a, b, c, d, xp[ 5],  4, 0xfffa3942);	/* 32 */
	    MD5_OP3(d, a, b, c, xp[ 8], 11, 0x8771f681);	/* 33 */
	    MD5_OP3(c, d, a, b, xp[11], 16, 0x6d9d6122);	/* 34 */
	    MD5_OP3(b, c, d, a, xp[14], 23, 0xfde5380c);	/* 35 */
	    MD5_OP3(a, b, c, d, xp[ 1],  4, 0xa4beea44);	/* 36 */
	    MD5_OP3(d, a, b, c, xp[ 4], 11, 0x4bdecfa9);	/* 37 */
	    MD5_OP3(c, d, a, b, xp[ 7], 16, 0xf6bb4b60);	/* 38 */
	    MD5_OP3(b, c, d, a, xp[10], 23, 0xbebfbc70);	/* 39 */
	    MD5_OP3(a, b, c, d, xp[13],  4, 0x289b7ec6);	/* 40 */
	    MD5_OP3(d, a, b, c, xp[ 0], 11, 0xeaa127fa);	/* 41 */
	    MD5_OP3(c, d, a, b, xp[ 3], 16, 0xd4ef3085);	/* 42 */
	    MD5_OP3(b, c, d, a, xp[ 6], 23, 0x04881d05);	/* 43 */
	    MD5_OP3(a, b, c, d, xp[ 9],  4, 0xd9d4d039);	/* 44 */
	    MD5_OP3(d, a, b, c, xp[12], 11, 0xe6db99e5);	/* 45 */
	    MD5_OP3(c, d, a, b, xp[15], 16, 0x1fa27cf8);	/* 46 */
	    MD5_OP3(b, c, d, a, xp[ 2], 23, 0xc4ac5665);	/* 47 */

	    /* round 4 */
	    MD5_OP4(a, b, c, d, xp[ 0],  6, 0xf4292244);	/* 48 */
	    MD5_OP4(d, a, b, c, xp[ 7], 10, 0x432aff97);	/* 49 */
	    MD5_OP4(c, d, a, b, xp[14], 15, 0xab9423a7);	/* 50 */
	    MD5_OP4(b, c, d, a, xp[ 5], 21, 0xfc93a039);	/* 51 */
	    MD5_OP4(a, b, c, d, xp[12],  6, 0x655b59c3);	/* 52 */
	    MD5_OP4(d, a, b, c, xp[ 3], 10, 0x8f0ccc92);	/* 53 */
	    MD5_OP4(c, d, a, b, xp[10], 15, 0xffeff47d);	/* 54 */
	    MD5_OP4(b, c, d, a, xp[ 1], 21, 0x85845dd1);	/* 55 */
	    MD5_OP4(a, b, c, d, xp[ 8],  6, 0x6fa87e4f);	/* 56 */
	    MD5_OP4(d, a, b, c, xp[15], 10, 0xfe2ce6e0);	/* 57 */
	    MD5_OP4(c, d, a, b, xp[ 6], 15, 0xa3014314);	/* 58 */
	    MD5_OP4(b, c, d, a, xp[13], 21, 0x4e0811a1);	/* 59 */
	    MD5_OP4(a, b, c, d, xp[ 4],  6, 0xf7537e82);	/* 60 */
	    MD5_OP4(d, a, b, c, xp[11], 10, 0xbd3af235);	/* 61 */
	    MD5_OP4(c, d, a, b, xp[ 2], 15, 0x2ad7d2bb);	/* 62 */
	    MD5_OP4(b, c, d, a, xp[ 9], 21, 0xeb86d391);	/* 63 */

	    /*
	     * Update the results by adding the initial values of
	     * a, b, c and d to them, then write them back.
	     */
	    a += results[0]; results[0] = a;
	    b += results[1]; results[1] = b;
	    c += results[2]; results[2] = c;
	    d += results[3]; results[3] = d;
	}
    }

    /*
     * Now add a length block if needed to complete this
     */
    if (!incomplete && !alldone) {
	register u_int32 x0, x14, x15;

	/*
	 * The last two words in the block contain the length in bits.
	 * Make it so.
	 */
	x14 = (((u_int32)(totallen)) << 3) & 0xffffffff;
	x15 = (u_int32)((totallen >> 29) & 0xffffffff);

	/*
	 * The first word will contain either a 0x80 or a 0, depending
	 * on whether the data run had a complete block or not.
	 */
	if (datalen & 0x3f) {
	    x0 = 0;
	} else {
	    x0 = 0x80;
	}

	/* round 1 */
	MD5_OP1(a, b, c, d,  x0,  7, 0xd76aa478);	/* 0 */
	MD5_OP1(d, a, b, c,   0, 12, 0xe8c7b756);	/* 1 */
	MD5_OP1(c, d, a, b,   0, 17, 0x242070db);	/* 2 */
	MD5_OP1(b, c, d, a,   0, 22, 0xc1bdceee);	/* 3 */
	MD5_OP1(a, b, c, d,   0,  7, 0xf57c0faf);	/* 4 */
	MD5_OP1(d, a, b, c,   0, 12, 0x4787c62a);	/* 5 */
	MD5_OP1(c, d, a, b,   0, 17, 0xa8304613);	/* 6 */
	MD5_OP1(b, c, d, a,   0, 22, 0xfd469501);	/* 7 */
	MD5_OP1(a, b, c, d,   0,  7, 0x698098d8);	/* 8 */
	MD5_OP1(d, a, b, c,   0, 12, 0x8b44f7af);	/* 9 */
	MD5_OP1(c, d, a, b,   0, 17, 0xffff5bb1);	/* 10 */
	MD5_OP1(b, c, d, a,   0, 22, 0x895cd7be);	/* 11 */
	MD5_OP1(a, b, c, d,   0,  7, 0x6b901122);	/* 12 */
	MD5_OP1(d, a, b, c,   0, 12, 0xfd987193);	/* 13 */
	MD5_OP1(c, d, a, b, x14, 17, 0xa679438e);	/* 14 */
	MD5_OP1(b, c, d, a, x15, 22, 0x49b40821);	/* 15 */

	/* round 2 */
	MD5_OP2(a, b, c, d,   0,  5, 0xf61e2562);	/* 16 */
	MD5_OP2(d, a, b, c,   0,  9, 0xc040b340);	/* 17 */
	MD5_OP2(c, d, a, b,   0, 14, 0x265e5a51);	/* 18 */
	MD5_OP2(b, c, d, a,  x0, 20, 0xe9b6c7aa);	/* 19 */
	MD5_OP2(a, b, c, d,   0,  5, 0xd62f105d);	/* 20 */
	MD5_OP2(d, a, b, c,   0,  9, 0x02441453);	/* 21 */
	MD5_OP2(c, d, a, b, x15, 14, 0xd8a1e681);	/* 22 */
	MD5_OP2(b, c, d, a,   0, 20, 0xe7d3fbc8);	/* 23 */
	MD5_OP2(a, b, c, d,   0,  5, 0x21e1cde6);	/* 24 */
	MD5_OP2(d, a, b, c, x14,  9, 0xc33707d6);	/* 25 */
	MD5_OP2(c, d, a, b,   0, 14, 0xf4d50d87);	/* 26 */
	MD5_OP2(b, c, d, a,   0, 20, 0x455a14ed);	/* 27 */
	MD5_OP2(a, b, c, d,   0,  5, 0xa9e3e905);	/* 28 */
	MD5_OP2(d, a, b, c,   0,  9, 0xfcefa3f8);	/* 29 */
	MD5_OP2(c, d, a, b,   0, 14, 0x676f02d9);	/* 30 */
	MD5_OP2(b, c, d, a,   0, 20, 0x8d2a4c8a);	/* 31 */

	/* round 3 */
	MD5_OP3(a, b, c, d,   0,  4, 0xfffa3942);	/* 32 */
	MD5_OP3(d, a, b, c,   0, 11, 0x8771f681);	/* 33 */
	MD5_OP3(c, d, a, b,   0, 16, 0x6d9d6122);	/* 34 */
	MD5_OP3(b, c, d, a, x14, 23, 0xfde5380c);	/* 35 */
	MD5_OP3(a, b, c, d,   0,  4, 0xa4beea44);	/* 36 */
	MD5_OP3(d, a, b, c,   0, 11, 0x4bdecfa9);	/* 37 */
	MD5_OP3(c, d, a, b,   0, 16, 0xf6bb4b60);	/* 38 */
	MD5_OP3(b, c, d, a,   0, 23, 0xbebfbc70);	/* 39 */
	MD5_OP3(a, b, c, d,   0,  4, 0x289b7ec6);	/* 40 */
	MD5_OP3(d, a, b, c,  x0, 11, 0xeaa127fa);	/* 41 */
	MD5_OP3(c, d, a, b,   0, 16, 0xd4ef3085);	/* 42 */
	MD5_OP3(b, c, d, a,   0, 23, 0x04881d05);	/* 43 */
	MD5_OP3(a, b, c, d,   0,  4, 0xd9d4d039);	/* 44 */
	MD5_OP3(d, a, b, c,   0, 11, 0xe6db99e5);	/* 45 */
	MD5_OP3(c, d, a, b, x15, 16, 0x1fa27cf8);	/* 46 */
	MD5_OP3(b, c, d, a,   0, 23, 0xc4ac5665);	/* 47 */

	/* round 4 */
	MD5_OP4(a, b, c, d,  x0,  6, 0xf4292244);	/* 48 */
	MD5_OP4(d, a, b, c,   0, 10, 0x432aff97);	/* 49 */
	MD5_OP4(c, d, a, b, x14, 15, 0xab9423a7);	/* 50 */
	MD5_OP4(b, c, d, a,   0, 21, 0xfc93a039);	/* 51 */
	MD5_OP4(a, b, c, d,   0,  6, 0x655b59c3);	/* 52 */
	MD5_OP4(d, a, b, c,   0, 10, 0x8f0ccc92);	/* 53 */
	MD5_OP4(c, d, a, b,   0, 15, 0xffeff47d);	/* 54 */
	MD5_OP4(b, c, d, a,   0, 21, 0x85845dd1);	/* 55 */
	MD5_OP4(a, b, c, d,   0,  6, 0x6fa87e4f);	/* 56 */
	MD5_OP4(d, a, b, c, x15, 10, 0xfe2ce6e0);	/* 57 */
	MD5_OP4(c, d, a, b,   0, 15, 0xa3014314);	/* 58 */
	MD5_OP4(b, c, d, a,   0, 21, 0x4e0811a1);	/* 59 */
	MD5_OP4(a, b, c, d,   0,  6, 0xf7537e82);	/* 60 */
	MD5_OP4(d, a, b, c,   0, 10, 0xbd3af235);	/* 61 */
	MD5_OP4(c, d, a, b,   0, 15, 0x2ad7d2bb);	/* 62 */
	MD5_OP4(b, c, d, a,   0, 21, 0xeb86d391);	/* 63 */

	results[0] += a;
	results[1] += b;
	results[2] += c;
	results[3] += d;
    }
}


/*
 * md5_cksum_partial - do a partial checksum on as much of the packet
 *		       as we can safely manage.  Return the amount of
 *		       data checksummed.
 */
size_t
md5_cksum_partial __PF4(data, void_t,
			upto, void_t,
			newdata, int,
			results, u_int32 *)
{
    size_t dolen;

    if (newdata) {
	results[0] = MD5_A_INIT;
	results[1] = MD5_B_INIT;
	results[2] = MD5_C_INIT;
	results[3] = MD5_D_INIT;
    }

    dolen = ((byte *) upto - (byte *) data) & (~((size_t) 0x3f));
    if (dolen) {
	md5_cksum_block(data, dolen, (size_t) 0, 1, results);
    }

    return dolen;
}


/*
 * md5_cksum - complete/compute an MD5 checksum of the specified packet
 */
void
md5_cksum __PF5(data, void_t,
		datalen, size_t,
		totallen, size_t,
		digest, void_t,
		init, u_int32 *)
{
    register byte *dp;
    register u_int32 *tp;
    u_int32 temp[4];

    tp = temp;
    if (init) {
	tp[0] = init[0];
	tp[1] = init[1];
	tp[2] = init[2];
	tp[3] = init[3];
    } else {
	tp[0] = MD5_A_INIT;
	tp[1] = MD5_B_INIT;
	tp[2] = MD5_C_INIT;
	tp[3] = MD5_D_INIT;
    }

    md5_cksum_block(data, datalen, totallen, 0, temp);

    dp = (byte *) digest;
    MD5_PUT(*tp, dp); tp++;
    MD5_PUT(*tp, dp); tp++;
    MD5_PUT(*tp, dp); tp++;
    MD5_PUT(*tp, dp);
}
#endif	/* MD5_CHECKSUM */
