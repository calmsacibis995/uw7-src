/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1993 Lachman Technology, Inc.
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
 *
 */
/*
 * ++Copyright++ 1993
 * -
 * Copyright (c) 1993 Regents of the University of California.
 * All rights reserved.
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
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
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
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

#ident "@(#)bitypes.h	1.2"

#ifndef __SYS_BITYPES_H
#define __SYS_BITYPES_H

#if !defined(i286)
/* true for vax, 68k, mips, i386, sparc, and alpha */
typedef	int		int32_t;
#else
typedef	long		int32_t;
#endif

typedef	short		int16_t;
typedef	char		int8_t;

#if !defined(i286)
typedef	unsigned int	u_int32_t;
#else
typedef	unsigned long	u_int32_t;
#endif
typedef	unsigned short	u_int16_t;
typedef	unsigned char	u_int8_t;

/*
 * Extensions from POSIX 1003.1(g), part of the _POSIX_PII_SOCKET feature.
 */
#ifdef __STDC__
typedef signed char	int8m_t;	/* signed, holds at least 8 bits */
#else
typedef char		int8m_t;	/* signed, holds at least 8 bits */
#endif

typedef unsigned char	u_int8m_t;	/* unsigned, holds at least 8 bits */
typedef short		int16m_t;	/* signed, holds at least 16 bits */
typedef unsigned short	u_int16m_t;	/* unsigned, holds at least 16 bits */

#if !defined(i286)
typedef int		int32m_t;	/* signed, holds at least 32 bits */
typedef unsigned int	u_int32m_t;	/* unsigned, holds at least 32 bits */
#else
typedef long		int32m_t;	/* signed, holds at least 32 bits */
typedef unsigned long	u_int32m_t;	/* unsigned, holds at least 32 bits */
#endif

#endif	/* __SYS_BITYPES_H */
