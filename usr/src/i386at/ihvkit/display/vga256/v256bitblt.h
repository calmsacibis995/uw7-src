#ident	"@(#)ihvkit:display/vga256/v256bitblt.h	1.1"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/************
 * Copyrighted as an unpublished work.
 * (c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 * All rights reserved.
 ***********/

#ifndef	_VGA_BITBLT_H_
#define	_VGA_BITBLT_H_
/*
 *	FILE : v256bitblt.h
 *	
 *	DESCRIPTION: 
 *		The regions passed up from v256_split_request.
 */
#include	"sidep.h"

/*
 *	Minimum height for calling v256_rop
 */
#define	V256_MIN_ROP_HEIGHT		(20)


#define V256_FAST_TRANSFER(s,d,k,m)\
{\
	switch(v256_function)\
	{\
		case V256_COPY:\
				if (~m == 0)\
					v256_memcpy(d, s, k);\
				else\
					v256_memcpymask(d, s, k, m);\
				break;\
			case V256_XOR:\
				v256_memxor(d, s, k, m);\
				break;\
			case V256_OR:\
				v256_memor(d, s, k, m);\
				break;\
			case V256_AND:\
				v256_memand(d, s, k, m);\
				break;\
			case V256_INVERT:\
				v256_cpyinvert(d, s, k, m);\
				break;\
			case V256_OR_INVERT:\
				v256_memor_i(d, s, k, m);\
				break;\
			case V256_AND_INVERT:\
				v256_memand_i(d, s, k, m);\
				break;\
		}\
}

extern	int	v256_slbytes;

/*
 *	Function prototypes
 */
extern	int
v256_split_request(
    int x_top_left,
    int y_top_left,
    int x_bottom_right,
    int y_bottom_right,
    int *n_rects_p,
    VgaRegion *rects_p
    );

#endif	/* _VGA_BITBLT_H_ */
