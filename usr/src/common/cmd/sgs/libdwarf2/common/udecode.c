#ident	"@(#)libdwarf2:common/udecode.c	1.1"

#include <libdwarf2.h>

#define MASK		0x7f
#define TOP_BIT		0x80
#define SHIFT_AMT	7

int
dwarf2_decode_unsigned(unsigned long *presult, byte *buffer)
{
	unsigned long	result = 0;
	int		shift = 0;
	byte		next_byte;
	int		count = 0;

	for (;;)
	{
		next_byte = *buffer++;
		++count;
		result |= ((next_byte & MASK) << shift);
		if (!(next_byte & TOP_BIT))
			break;
		shift += SHIFT_AMT;
	} 
	*presult = result;
	return count;
}
