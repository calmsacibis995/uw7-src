/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)mp.h	1.2"
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
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
/*	@(#)mp.h 1.7 88/08/19 SMI; from UCB 5.1 5/30/85	*/

#ifndef _mp_h
#define _mp_h

struct mint {
	int len;
	short *val;
};
typedef struct mint MINT;

extern MINT *itom();
extern MINT *xtom();
extern char *mtox();
extern short *xalloc();
extern void mfree();

#define	FREE(x)	xfree(&(x))		/* Compatibility */

#endif /*!_mp_h*/
