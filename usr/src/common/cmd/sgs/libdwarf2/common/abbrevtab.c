#ident	"@(#)libdwarf2:common/abbrevtab.c	1.2"

#include "libdwarf2.h"
#include <string.h>
#include <stdlib.h>

void
dwarf2_delete_abbreviation_table(Dwarf2_Abbreviation *abbrev_table)
{
	Dwarf2_Abbreviation *table;

	if (!abbrev_table)
		return;
	for (table = &abbrev_table[1]; table->tag; table++)
		free((Dwarf2_Attribute *)table->attributes);
	free((Dwarf2_Abbreviation *)abbrev_table);
}

#define BLOCK_SIZE	50

Dwarf2_Abbreviation *
dwarf2_get_abbreviation_table(unsigned char *p_start, unsigned int max_size,
	unsigned int *bytes_used)
{
	unsigned char		*p_end;
	unsigned char		*p_data = p_start;
	int			entries;

	/* Leave enough room for a large number of tags and attributes.
	 * The code below allows more anyway.
	 */
	int			abbr_max_size = BLOCK_SIZE;
	Dwarf2_Abbreviation	abbr[BLOCK_SIZE];
	Dwarf2_Abbreviation	*abbr_start = abbr;
	Dwarf2_Abbreviation	*pabbr = abbr;
	Dwarf2_Abbreviation	*table = 0;

	int			attr_max_size = BLOCK_SIZE;
	Dwarf2_Attribute	attr[BLOCK_SIZE];
	Dwarf2_Attribute	*attr_start = attr;

	/* clear out zero element */
	(void)memset(pabbr, 0, sizeof(Dwarf2_Abbreviation));
	++pabbr;
	entries = 1;

	p_end = p_data + max_size;
	while (p_data < p_end)
	{
		unsigned long		index;
		Dwarf2_Attribute	*pattr;
		int			attr_count;

		if (++entries > abbr_max_size)
		{
			size_t needed = sizeof(Dwarf2_Abbreviation)
						* (abbr_max_size + BLOCK_SIZE);
			if (abbr_start == abbr)
			{
				if ((abbr_start = malloc(needed)) == 0)
					goto out;
				(void)memcpy(abbr_start, abbr, sizeof(abbr));
			}
			else
			{
				if ((abbr_start = (Dwarf2_Abbreviation *)realloc(abbr_start,
										needed)) == 0)
					goto out;
			}
			pabbr = abbr_start + abbr_max_size;
			abbr_max_size += BLOCK_SIZE;
		}

		p_data += dwarf2_decode_unsigned(&index, p_data);
		if (!index)
		{
			/* last entry in this section */
			(void)memset(pabbr, 0, sizeof(Dwarf2_Abbreviation));
			break;
		}
		p_data += dwarf2_decode_unsigned(&pabbr->tag, p_data);
		pabbr->children = *p_data++;

		pattr = attr_start;
		attr_count = 0;
		while (p_data < p_end)
		{
			if (++attr_count > attr_max_size)
			{
				size_t needed = sizeof(Dwarf2_Attribute)
							* (attr_max_size + BLOCK_SIZE);
				if (attr_start == attr)
				{
					if ((attr_start = malloc(needed)) == 0)
						goto out;
					(void)memcpy(attr_start, attr, sizeof(attr));
				}
				else
				{
					if ((attr_start = (Dwarf2_Attribute *)realloc(attr_start,
									needed)) == 0)
						goto out;
				}
				pattr = attr_start + attr_max_size;
				attr_max_size += BLOCK_SIZE;
			}
			p_data += dwarf2_decode_unsigned(&pattr->name, p_data);
			p_data += dwarf2_decode_unsigned(&pattr->form, p_data);
			if (pattr->name == 0)
			{
				attr_count--;
				break;
			}
			pattr++;
		}
		pabbr->nattr = attr_count;
		if ((pattr = (Dwarf2_Attribute *)malloc(
					attr_count * sizeof(Dwarf2_Attribute))) == 0)
			goto out;
		(void)memcpy(pattr, attr_start,
				attr_count * sizeof(Dwarf2_Attribute));
		pabbr->attributes = pattr;
		++pabbr;
	}
	if (bytes_used)
		*bytes_used = p_data - p_start;

	if ((table = (Dwarf2_Abbreviation *)malloc(entries * sizeof(Dwarf2_Abbreviation))) != 0)
	{
		(void)memcpy(table, abbr_start, entries * sizeof(Dwarf2_Abbreviation));
	}
out:
	if (abbr_start != abbr)
		free (abbr_start);
	if (attr_start != attr)
		free (attr_start);
	return table;
}
