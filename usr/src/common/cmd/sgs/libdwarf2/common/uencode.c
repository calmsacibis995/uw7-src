#ident	"@(#)libdwarf2:common/uencode.c	1.1"

#include <libdwarf2.h>

#define MASK	0x7f
#define TOP_BIT	0x80

int
dwarf2_encode_unsigned(unsigned long ul, byte *buffer)
{
	byte next_byte;
	int count = 0;

	do
	{
		next_byte = ul & MASK;
		ul >>= 7;
		if (ul)
			next_byte |= TOP_BIT;
		*buffer++ = next_byte;
		++count;
	} while (ul);
	return count;
}
