/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*
 * Copyright (c) 1982-1995
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ident "@(#)in_f.h	1.7"
#ident "$Header$"

#ifndef _netinet_in_f_h        /* wrapper symbol for kernel use */
#define _netinet_in_f_h        /* subject to change without notice */

/*
 * Internet address (a structure for historical reasons) 
 */
struct in_addr {
	union {
		struct { unsigned char s_b1, s_b2, s_b3, s_b4; } __S_un_b;
		struct { unsigned short s_w1, s_w2; } __S_un_w;
		unsigned long __S_addr;
	} __S_un;
#define s_addr	__S_un.__S_addr
};

#if !(defined (_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
#define S_un_b __S_un_b
#define S_un_w __S_un_w
#define S_addr __S_addr
#define S_un __S_un
#endif

/*
 * Definitions of bits in internet address integers. On subnets, the
 * decomposition of addresses to host and net parts is done according to
 * subnet mask, not the masks here. 
 */
#define	IN_CLASSA(i)		(((long)(i) & 0x80000000) == 0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		0x00ffffff
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(i)		(((long)(i) & 0xc0000000) == 0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		0x0000ffff
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(i)		(((long)(i) & 0xe0000000) == 0xc0000000)
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		0x000000ff

#define	IN_CLASSD(i)		(((long)(i) & 0xf0000000) == 0xe0000000)
#define IN_CLASSD_NET           0xf0000000      /* These ones aren't really */
#define IN_CLASSD_NSHIFT        28              /* net and host fields, but */
#define IN_CLASSD_HOST          0x0fffffff      /* routing needn't know.    */
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#define	IN_EXPERIMENTAL(i)	(((long)(i) & 0xe0000000) == 0xe0000000)
#define	IN_BADCLASS(i)		(((long)(i) & 0xf0000000) == 0xf0000000)

#define	INADDR_ANY		(u_long)0x00000000
#define INADDR_LOOPBACK		(u_long)0x7f000001
#define	INADDR_BROADCAST	(u_long)0xffffffff	/* must be masked */

#define INADDR_NONE		(u_long)0xffffffff	/* -1 return */

#define	IN_LOOPBACKNET		127			/* official! */

#define	INADDR_UNSPEC_GROUP	(u_long)0xe0000000	/* 224.0.0.0   */
#define	INADDR_ALLHOSTS_GROUP	(u_long)0xe0000001	/* 224.0.0.1   */
#define	INADDR_MAX_LOCAL_GROUP 	(u_long)0xe00000ff	/* 224.0.0.255 */

#endif /* _netinet_in_f_h */
