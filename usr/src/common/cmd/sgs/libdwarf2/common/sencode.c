#ident	"@(#)libdwarf2:common/sencode.c	1.2"

#include <libdwarf2.h>
#include <values.h>

#define MASK		0x7f
#define TOP_BIT		0x80
#define SIGN_BIT	0x40

int
dwarf2_encode_signed(long val, byte *buffer)
{
	byte	next_byte;
	int	count = 0;
	int	more = 1;
	int	negative = (val < 0);

	while (more)
	{
		next_byte = val & MASK;
		val >>= 7;	/* sign extension is machine specific */
		if (negative && val > 0)
		{
			val |= -(1 << (BITS(long) - 7));
		}
		if ((val == 0 && !(val & SIGN_BIT))
			|| (val == -1 && (val & SIGN_BIT)))
		{
			more = 0;
		}
		else
		{
			next_byte |= TOP_BIT;
		}
		*buffer++ = next_byte;
		++count;
	}
	return count;
}
