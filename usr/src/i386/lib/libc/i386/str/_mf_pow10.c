#ident	"@(#)libc-i386:str/_mf_pow10.c	1.1"
/*LINTLIBRARY*/
/*
* _mfpow10.c - Table of floating powers of 10, x86 specific.
*/

#include "synonyms.h"
#include <float.h>
#include <stddef.h>
#include "mkflt.h"

#ifndef NO_LONG_DOUBLE

const long double _mf_pow10[1 + LOG2TO5(LDBL_MANT_DIG)] =
{
	1e00L,	1e01L,	1e02L,	1e03L,	1e04L,	1e05L,	1e06L,	1e07L,
	1e08L,	1e09L,	1e10L,	1e11L,	1e12L,	1e13L,	1e14L,	1e15L,
	1e16L,	1e17L,	1e18L,	1e19L,	1e20L,	1e21L,	1e22L,	1e23L,
	1e24L,	1e25L,	1e26L,	1e27L
};

#else

const double _mf_pow10[1 + LOG2TO5(DBL_MANT_DIG)] =
{
	1e00,	1e01,	1e02,	1e03,	1e04,	1e05,	1e06,	1e07,
	1e08,	1e09,	1e10,	1e11,	1e12,	1e13,	1e14,	1e15,
	1e16,	1e17,	1e18,	1e19,	1e20,	1e21,	1e22
};

#endif
