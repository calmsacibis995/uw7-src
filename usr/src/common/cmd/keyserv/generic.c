/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)generic.c	1.2"
#ident	"$Header$"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/file.h>
#include "mp.h"
#include <rpc/key_prot.h>

/*
 * Generate a seed
 */
static
getseed(seed, seedsize, pass)
	char *seed;
	int seedsize;
	unsigned char *pass;
{
	int i;
	int rseed;
	struct timeval tv;

	(void)gettimeofday(&tv, (struct timezone *)NULL);
	rseed = tv.tv_sec + tv.tv_usec;
	for (i = 0; i < 8; i++) {
		rseed ^= (rseed << 8) | pass[i];
	}
	srand(rseed);

	for (i = 0; i < seedsize; i++) {
		seed[i] = (rand() & 0xff) ^ pass[i % 8];
	}
}

/*
 * Generate a random public/secret key pair
 */
genkeys(public, secret, pass)
	char *public;
	char *secret;
	char *pass;
{
	int i;

#   define BASEBITS (8*sizeof (short) - 1)
#	define BASE		(1 << BASEBITS)

	MINT *pk = itom(0);
	MINT *sk = itom(0);
	MINT *tmp;
	MINT *base = itom(BASE);
	MINT *root = itom(PROOT);
	MINT *modulus = xtom(HEXMODULUS);
	short r;
	unsigned short seed[KEYSIZE/BASEBITS + 1];
	char *xkey;

	getseed((char *)seed, sizeof (seed), (u_char *)pass);
	for (i = 0; i < KEYSIZE/BASEBITS + 1; i++) {
		r = (short) ((unsigned int)seed[i] % BASE);
		tmp = itom(r);
		mult(sk, base, sk);
		madd(sk, tmp, sk);
		mfree(tmp);
	}
	tmp = itom(0);
	mdiv(sk, modulus, tmp, sk);
	mfree(tmp);
	pow(root, sk, modulus, pk);
	xkey = mtox(sk);
	adjust(secret, xkey);
	xkey = mtox(pk);
	adjust(public, xkey);
	mfree(sk);
	mfree(base);
	mfree(pk);
	mfree(root);
	mfree(modulus);
}

/*
 * Adjust the input key so that it is 0-filled on the left
 */
static
adjust(keyout, keyin)
	char keyout[HEXKEYBYTES+1];
	char *keyin;
{
	char *p;
	char *s;

	for (p = keyin; *p; p++)
		;
	for (s = keyout + HEXKEYBYTES; p >= keyin; p--, s--) {
		*s = *p;
	}
	while (s >= keyout) {
		*s-- = '0';
	}
}
