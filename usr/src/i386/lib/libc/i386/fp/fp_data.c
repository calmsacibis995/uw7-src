#ident	"@(#)libc-i386:fp/fp_data.c	1.5"
/*
 * contains the definitions
 * of the global symbols used
 * by the floating point environment
 *
 * Cannot #include <math.h> because it believes __huge_val to be a double.
 */
#include "synonyms.h"

#ifdef __STDC__
const union {
	unsigned char uc[sizeof(double) / sizeof(unsigned char)];
	double d;
} __huge_val
	= { { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f } };
#else
unsigned long __huge_val[sizeof(double) / sizeof(unsigned long)]
	= { 0, 0x7ff00000 };
#endif

int	__flt_rounds = 1;
