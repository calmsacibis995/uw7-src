#ident	"@(#)grand.c	1.3"
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
 * grand.c and friends.  An integer random number generator for gated.
 *
 * Both the mainline additive congruential generator, and the linear
 * congruential generator used when seeding, are borrowed intact from Knuth.
 */

/*
 * Here we implement a 32 bit additive congruential random number
 * generator.  This is fairly speedy, though it consumes about
 * 220 bytes of constant table space and a slightly greater amount
 * of data space.  We compute 32 bit random values which are scaled
 * as required.
 *
 * We use Knuth's values of b=31 and c=55 to implement this, implying
 * the need for a 55-word register.  We initialize the register with
 * a constant set of random numbers which are modified at initialization
 * by xor'ing the output of a linear congruential generator seeded
 * by a 32 bit value.  This is supposed to ensure decently random
 * output even if the seed chosen turns out to be wonky.  Re-calling
 * the initialization routine with the same seed should replay the
 * same number sequence.
 *
 * There are three routines in here.  grand_seed() takes a 32-bit
 * integer as an argument to seed the random number generator.
 * Calling grand_seed() with the same seed will restart the same
 * random number sequence.  Just about anything will do, though
 * (tv_sec ^ tv_usec) from gettimeofday() seems kind of neat.
 * grand_seed() does do a fair bit of computation, relatively speaking
 * (about 345 16x16=32 bit multiplies).
 *
 * grand(n) returns a random integer in the range (0..n-1).  The
 * random number is actually derived from the high order part of
 * the 32 bit internal value; while the low order part would be more
 * convenient to use, the high order bits should have a much longer
 * cycle time due to drift up from the low order part.  Beyond
 * additions, shifts and masks the routine does 2 multiplies if
 * n<65536, or 4 multiplies otherwise, on 32 bit machines.  On
 * 64 bit machines we can get away with a single multiply.
 *
 * grand_log2(n) returns a random integer in the range (0..(2**n)-1).
 * This avoids the multiplies in grand(), and provides a way to
 * obtain full precision 32 bit random numbers (use grand_log2(32)).
 *
 * The routine has undergone chi^2 testing to ensure the randomness
 * of the output.  The top 16 bits of the linear congruential generator
 * used in the seed routine for initialization is actually better than the
 * additive generator used to generate the numbers, but both the high-order
 * and the low-order 16 bits of the additive generator tested okay
 * (sort of marginally, if you have really high standards you might
 * want to do your own testing).  It is at least as good as the BSD
 * random()/srandom() routines (it uses the same algorithm), with
 * typically somewhat less CPU overhead if you want scaled numbers.
 */

/*
 * This is the `b' multiplier for the linear generator.  The value
 * we use is 797259821, though since we only use half the bits at
 * a time we split it in two here.
 */
#define	LCONB		797259821
#define	LCONB1		0x2f85		/* high order of 797259821 */
#define	LCONB0		0x382d		/* low order of 797259821 */

/*
 * This is the fixed table we use for initialization of the additive
 * generator.  It really should be declared const.  The numbers were
 * copied out of an old CRC math handbook with great pain.
 */
#define	TAP1		31		/* first tap */
#define	TAP2		55		/* second tap */
#define	REGSIZE		TAP2		/* largest tap also defines size */

static const U_INT32 initreg[REGSIZE] = {
 /* 1141277249, 2709928978, 4259445343, 1927830434, 2039360758, */
    0x44068241, 0xa1863c12, 0xfde1fa5f, 0x72e85ba2, 0x798e2cf6,
 /* 2394763994,  578465010,  960347592,  331920724, 1372238758, */
    0x8ebd32da, 0x227aacf2, 0x393dbdc8, 0x13c8b554, 0x51cab3a6,
 /*  912608081, 3597483205,  342647964, 3083359110,  735593562, */
    0x36654b51, 0xd66d3cc5, 0x146c649c, 0xb7c85386, 0x2bd8445a,
 /*   88240761, 2168127358, 1047427037, 2109159457, 2381957287, */
    0x05427279, 0x813aff7e, 0x3e6e77dd, 0x7db73821, 0x8df9c8a7,
 /*  387884763, 3233886172, 4041823072, 2748545107, 1096415062, */
    0x171ea6db, 0xc0c12fdc, 0xf0e95360, 0xa3d37853, 0x4159f756,
 /* 1946436445, 3644183052, 3289762975, 3211272610, 3461605135, */
    0x7404435d, 0xd935d20c, 0xc415cc9f, 0xbf6821a2, 0xce53e70f,
 /*  455979384,  113247879, 1907097750, 2369225744,  650077861, */
    0x1b2db178, 0x06c00687, 0x71ac0096, 0x8d378410, 0x26bf66a5,
 /* 1390974557, 2869077306, 4191327823, 4237048966, 3324590241, */
    0x52e8965d, 0xab02a53a, 0xf9d2964f, 0xfc8c3c86, 0xc62938a1,
 /* 2794662711, 2205917664, 4124680024, 4072885277, 1127346059, */
    0xa6932b37, 0x837ba1e0, 0xf5d99f58, 0xf2c34c1d, 0x4331ef8b,
 /* 3989332236, 2589317125, 1139650582,  936586995, 2838582711, */
    0xedc8610c, 0x9a55d805, 0x43edb016, 0x37d32ef3, 0xa93155b7,
 /* 1518811617,  715100532, 2246264507, 3845202566, 3302491113  */
    0x5a8739e1, 0x2a9f9174, 0x85e346bb, 0xe5312286, 0xc4d803e9
};

/*
 * The running storage we use, initialized by grand_seed and updated
 * by calls to the random number routines, are a 55 word register set
 * and the two tap indicies.
 */
static U_INT32 reg[REGSIZE] = { 0 };

static U_INT32 *tap1 = 0;
static U_INT32 *tap2 = 0;

/*
 * If one of the random number routines is called before the initialization
 * routine (detected by noting that tap1 and tap2 are NULL) we initialize
 * using the default seed.  When doing initialization we run through the
 * generating sequence LCONPRIME times before using the output, to allow
 * the multiplies to overflow a few times even when tiny seeds are used.
 */
#define	DEFAULT_SEED	0

#define	LCONPRIME	(5)		/* 5 times through to start */


/*
 * grand_seed - seed the random number generator with an arbitrary 32 bit value
 */
void
grand_seed __PF1(seed, U_INT32)
{
    register U_INT32 r;
#ifndef	U_INT64
    register U_INT32 r0, r1;
#endif
    register int i, j;

    /*
     * The scheme here is no doubt overkill.  We first run the
     * generator 5 times to get an ugly-looking value.  We then
     * exclusive or the high order 16 bits of the 55 subsequent
     * random values with the register initialization numbers.
     * Finally we go around again xor'ing the high order 16 bits
     * with the high order part of the initialized values.
     *
     * To avoid troubles with machines which don't do multiply
     * overflows gracefully (i.e. don't do an implicit % by the
     * machine word size) we break the multiply into 3 16x16=32
     * chunks on 32 bit machines.  If there is a 64 bit multiply
     * available, however, we use that instead.
     */
#ifndef U_INT64
    r0 = seed & 0xffff;
    r1 = (seed >> 16) & 0xffff;
    for (j = 0; j < 2; j++) {
	i = ((j == 0) ? (-LCONPRIME) : 0);
	while (i < REGSIZE) {
	    r = ((r1*LCONB0 + r0*LCONB1) << 16) + (r0*LCONB0) + 1;
	    r0 = r & 0xffff;
	    r1 = (r >> 16) & 0xffff;
	    if (j == 0) {
		if (i >= 0) {
		    reg[i] = initreg[i] ^ r1;
		}
	    } else {
		reg[i] ^= (r1 << 16);
	    }
	    i++;
	}
    }
#else	/* !U_INT64 */
    r = seed;
    for (j = 0; j < 2; j++) {
	i = ((j == 0) ? (-LCONPRIME) : 0);
	while (i < REGSIZE) {
	    r = ((U_INT32) (((u_int64) r) * ((u_int64) (LCONB)))) + 1;
	    if (j == 0) {
		if (i >= 0) {
		    reg[i] = initreg[i] ^ ((r >> 16) & 0xffff);
		}
	    } else {
		reg[i] ^= r & 0xffff0000;
	    }
	    i++;
	}
    }
#endif	/* !U_INT64 */


    /*
     * Set up the taps while we're here.
     */
    tap1 = &reg[0] + TAP1;
    tap2 = &reg[0] + TAP2;
}


/*
 * grand - return a random number in the range (0..n-1)
 */
U_INT32
grand __PF1(n, U_INT32)
{
    register U_INT32 vl;
#ifndef	U_INT64
    register U_INT32 vh, nh, nl;
    register U_INT32 r;
#endif	/* U_INT64 */

    /*
     * Decrement the taps.  We detect uninitialized usage in
     * here as well, and call the initialization routine if so.
     */
    if (tap2 == 0) {
	grand_seed(DEFAULT_SEED);
    } else if (tap2 == &reg[0]) {
	tap2 += REGSIZE;
    } else if (tap1 == &reg[0]) {
	tap1 += REGSIZE;
    }

    --tap1;
    --tap2;

    /*
     * tap2 always points at the least recently used number.  Add
     * the value at tap1 to it.  The value at tap2 is our new
     * number.  Mask in case the tap? data type has more than 32 bits
     * (hopefully the optimizer will delete the mask when compiling).
     */
    *tap2 = vl = (*tap1 + *tap2) & 0xffffffff;

    /*
     * We now scale to the range (0..n-1).  Since the high bits
     * in the random number should be better, we scale by
     * essentially multiplying the value by (n) and dividing by
     * 2^32.  On 32-bit machines we do the multiplication in 16
     * bit chunks to avoid overflow, and short circuit if the high
     * order bits of (n) are zero (a common case).  On 64 bit machines
     * we may just do the multiply.
     */
#ifndef	U_INT64
    vh = vl >> 16;
    vl &= 0xffff;
    nh = (n & 0xffffffff) >> 16;
    nl = n & 0xffff;
    r = (vl * nl) >> 16;
    r += vh * nl;
    if (nh == 0) {
	r >>= 16;
    } else {
	r += vl * nh;
	r = (r >> 16) + (vh * nh);
    }

    return r;
#else	/* U_INT64 */
    return (U_INT32) ((((u_int64) (vl)) * ((u_int64) (n & 0xffffffff))) >> 32);
#endif	/* !U_INT64 */
}


/*
 * grand_log2 - return a random number in the range (0..(2^n)-1)
 */
U_INT32
grand_log2 __PF1(n, int)
{
    register U_INT32 val;

    /*
     * Decrement the taps.  We detect uninitialized usage in
     * here as well, and call the initialization routine if so.
     */
    if (tap2 == 0) {
	grand_seed(DEFAULT_SEED);
    } else if (tap2 == &reg[0]) {
	tap2 += REGSIZE;
    } else if (tap1 == &reg[0]) {
	tap1 += REGSIZE;
    }

    --tap1;
    --tap2;

    /*
     * tap2 always points at the least recently used number.  Add
     * the value at tap1 to it.  The value at tap2 is our new
     * number.  Mask in case the tap? data type has more than 32 bits
     * (hopefully the optimizer will delete the mask when compiling).
     */
    *tap2 = val = (*tap1 + *tap2) & 0xffffffff;


    /*
     * We now scale to the range (0..(2^n)-1).  We shift over
     * (32 - n) bits to do this.
     */
    if (n >= 32) {
	return val;
    }
    if (n <= 0) {
	return (U_INT32) 0;
    }
    return val >> (32 - n);
}
