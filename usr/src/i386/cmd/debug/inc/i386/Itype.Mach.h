/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef machineItype_h
#define machineItype_h
#ident	"@(#)debugger:inc/i386/Itype.Mach.h	1.4"

typedef char		Ichar;	/* signed target char */
typedef	char		Iint1;	/* signed 1 byte int */
typedef	short		Iint2;	/* signed 2 byte int */
typedef	long		Iint4;	/* signed 4 byte int */
#if LONG_LONG
typedef long long	Iint8;	/* signed 8 byte int */
#endif

typedef unsigned char	Iuchar;	/* unsigned target char */
typedef	unsigned char	Iuint1;	/* unsigned 1 byte int */
typedef	unsigned short	Iuint2;	/* unsigned 2 byte int */
typedef	unsigned long	Iuint4;	/* unsigned 4 byte int */
#if LONG_LONG
typedef unsigned long long	Iuint8;	/* unsigned 8 byte int */
#endif

typedef float		Isfloat; /* ANSI single prec. floating pt */
typedef double		Idfloat; /* ANSI double precision floating pt */
typedef long double	Ixfloat; /* ANSI long double */

#include "Iaddr.h"
/*typedef unsigned long Iaddr;	holds a target address */
typedef unsigned long	Ibase;	/* holds a target segment base */
typedef unsigned long	Ioffset; /* holds a target segment base */

#endif

