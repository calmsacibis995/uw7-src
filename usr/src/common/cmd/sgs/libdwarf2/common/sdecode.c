#ident	"@(#)libdwarf2:common/sdecode.c	1.1"

#include <libdwarf2.h>

#define MASK		0x7f
#define TOP_BIT		0x80
#define SIGN_BIT	0x40
#define SHIFT_AMT	7

int
dwarf2_decode_signed(long *presult, byte *buffer)
{
	long	result = 0;
	int	shift = 0;
	byte	next_byte;
	int	count = 0;

	for (;;)
	{
		next_byte = *buffer++;
		++count;
		result |= ((next_byte & MASK) << shift);
		shift += SHIFT_AMT;
		if (!(next_byte & TOP_BIT))
			break;
	} 
	if ((shift < sizeof(long) * 8) && (next_byte & SIGN_BIT))
	{
		/* sign extend */
		result |= -(1 << shift);
	}
	*presult = result;
	return count;
}
