#ident	"@(#)libdwarf2:common/get_name.c	1.2"

#include "dwarf2.h"
#include "Table.h"
#include <stdio.h>

const char *
get_name(Table *tab, unsigned int value)
{
	static char buf[sizeof(unsigned int) * sizeof("0xff")];

	for (; tab->name != 0; tab++)
	{
		if (tab->value == value)
			return tab->name;
	}

	(void)sprintf(buf, "%#x", value);
	return buf;
}
