#ident	"@(#)debugger:libsymbol/i386/Coff_Bit.C	1.1"

#include "Coffbuild.h"
#include <limits.h>

// Figure out location offset for bit field relative
// to enclosing structure, and
// proper bit offset for bit fields;
// should be number of bits to left of most significant bit.
//
// For offset, Coff records number of bits from lowest address to
// beginning of field
int
coff_bit_loc(int type, int bit_off, int bit_size, int &loc, int &off)
{
	int	sz;
	int	bits;

	switch(type)
	{
	default:
		return 0;
	case T_CHAR:
		sz = sizeof(char);
		break;
	case T_SHORT:
		sz = sizeof(short);
		break;
	case T_INT:
		sz = sizeof(int);
		break;
	case T_LONG:
		sz = sizeof(long);
		break;
	case T_UCHAR:
		sz = sizeof(unsigned char);
		break;
	case T_USHORT:
		sz = sizeof(unsigned short);
		break;
	case T_UINT:
		sz = sizeof(unsigned int);
		break;
	case T_ULONG:
		sz = sizeof(unsigned long);
		break;
	}
	bits = sz * CHAR_BIT;
	loc = (bit_off / bits) * sz;
	bit_off %= bits;
	off = bits - (bit_off + bit_size);
	return 1;
}
