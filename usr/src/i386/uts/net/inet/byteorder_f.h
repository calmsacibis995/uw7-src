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
#ident "$Id$"

#ifndef _NET_INET_BYTEORDER_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_BYTEORDER_F_H	/* subject to change without notice */

#define BYTE_ORDER	LITTLE_ENDIAN

#if defined(__cplusplus)
inline unsigned long
ntohl(unsigned long nl) {
	return (((nl<<24)&0xFF000000) +
		((nl<<8)&0xFF0000) +
		((nl>>8)&0xFF00) +
		((nl>>24)&0xFF));
}

inline unsigned long
htonl(unsigned long hl) {
	return (((hl<<24)&0xFF000000) +
		((hl<<8)&0xFF0000) +
		((hl>>8)&0xFF00) +
		((hl>>24)&0xFF));
}

inline unsigned short
ntohs(unsigned short ns) {
	return (((ns<<8)&0xFF00) +
		((ns>>8)&0xFF));
}

inline unsigned short
htons(unsigned short hs) {
	return (((hs<<8)&0xFF00) +
		((hs>>8)&0xFF));
}

inline unsigned short
bswaps(unsigned short us) {
	return (((us<<8)&0xFF00) +
		((us>>8)&0xFF));
}

#else

/*
 * The following macro will swap bytes in a short.
 * Warning: this macro expects 16-bit shorts and 8-bit chars
 */

#define bswaps(us)	(((unsigned short)((us) & 0xff) << 8) | \
			((unsigned short)((us) & ~0xff) >> 8))

/*
 * Macros for conversion between host and internet network byte order.
 *
 */

/*
 *	unsigned long htonl(hl)
 *	long hl;
 *	reverses the byte order of 'long hl'
 */

#define htonl(hl) __htonl(hl)
#ifdef __STDC__
__asm unsigned long __htonl(hl)
#else
asm unsigned long __htonl(hl)
#endif
{

%mem	hl;
	movl	hl, %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	clc
}

/*
 *	unsigned long ntohl(nl)
 *	unsigned long nl;
 *	reverses the byte order of 'unsigned long nl'
 */

#define ntohl(nl) __ntohl(nl)
#ifdef __STDC__
__asm unsigned long __ntohl(nl)
#else
asm unsigned long __ntohl(nl)
#endif
{
%mem	nl;
	movl	nl, %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	clc
}

/*
 *	unsigned short htons(hs)
 *	short hs;
 *
 *	reverses the byte order in hs.
 */

#define htons(hs) __htons(hs)
#ifdef __STDC__
__asm unsigned short __htons(hs)
#else
asm unsigned short __htons(hs)
#endif
{
%mem	hs;
	movl	hs, %eax
	xchgb	%ah, %al
	andl	$0xffff, %eax
	clc
}

/*
 *	unsigned short ntohs(ns)
 *	unsigned short ns;
 *
 *	reverses the bytes in ns.
 */

#define ntohs(ns) __ntohs(ns)
#ifdef __STDC__
__asm unsigned short __ntohs(ns)
#else
asm unsigned short __ntohs(ns)
#endif
{
%mem	ns;
	movl	ns, %eax
	xchgb	%ah, %al
	andl	$0xffff, %eax
	clc
}

#endif /* __cplusplus */

#endif /* _NET_INET_BYTEORDER__F_H */
