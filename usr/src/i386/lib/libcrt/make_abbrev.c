#ident	"@(#)libcrt:make_abbrev.c	1.2"

#include "dwarf2.h"
#include "libdwarf2.h"
#include "abbrev.h"
#include <stdio.h>
#include <stdlib.h>

	
#ifndef COMMENTSTR
#	define COMMENTSTR	"#"
#endif /* COMMENTSTR */

static FILE				*outfile;
static const Dwarf2_Abbreviation	*table;

static void
gen_LEB128(unsigned long val)
{
	byte	buffer[2*sizeof(unsigned long)];	/* should be plenty of space */
	int	len;
	byte	*ptr = buffer;

	len = dwarf2_encode_unsigned(val, buffer);
	for (; len; --len)
	{
		(void)fprintf(outfile, "%#x", *ptr++);
		if (len > 1)
			(void)putc(',', outfile);
	}
}

/* Generate a separate .debug_abbrev section containing
** a "standard" abbreviation table.
*/
main(int argc, char *argv[])
{
	const Dwarf2_Abbreviation	*entry;
	int	i;

	if (argc != 2)
	{
		(void)fprintf(stderr, "Usage: %s file\n", argv[0]);
		exit(1);
	}

	if ((outfile = fopen(argv[1], "w")) == NULL)
	{
		(void)fprintf(stderr, "cannot open %s for writing\n", argv[1]);
		exit(1);
	}

	/* create abbreviation table  section */
	(void)fprintf(outfile, "\t.section\t.debug_abbrev\n");

	/* output Abbreviation table lable */
	(void)fprintf(outfile, "\t.globl\t%s\n", ABBREV_VERSION_ID);
	(void)fprintf(outfile, "%s:\t%s Dwarf 2 - Abbreviation Table\n",
		ABBREV_VERSION_ID, COMMENTSTR);

	/* create the entries */
	if ((table = dwarf2_gen_abbrev_table()) == 0)
	{
		(void)fprintf(stderr, "cannot create abbreviation table\n");
		exit(2);
	}

	for (i = 1, entry = &table[1]; i < DW2_last; ++i, ++entry)
	{
		const Dwarf2_Attribute *ab_entry = entry->attributes;
		int			j;

		(void)fprintf(outfile, "\t.byte\t");
		gen_LEB128(i);
		(void)fprintf(outfile, "\t%s start abbrev entry\n", COMMENTSTR);

		/* generate tag and children flag */
		(void)fprintf(outfile, "\t.byte\t");
		gen_LEB128(entry->tag);
		(void)fprintf(outfile, ";\t.byte\t");
		gen_LEB128(entry->children);
		(void)fprintf(outfile, "\t%s %s %s\n", COMMENTSTR, dwarf2_tag_name(entry->tag),
				entry->children ? "DW_CHILDREN_yes" : "DW_CHILDREN_no");

		/* generate attributes */
		for (j = entry->nattr; j; --j, ++ab_entry)
		{
			(void)fprintf(outfile, "\t.byte\t");
			gen_LEB128(ab_entry->name);
			(void)fprintf(outfile, ";\t.byte\t");
			gen_LEB128(ab_entry->form);
			(void)fprintf(outfile, "\t%s %s %s\n", COMMENTSTR,
				dwarf2_attribute_name(ab_entry->name),
				dwarf2_form_name(ab_entry->form));
		}
		(void)fprintf(outfile, "\t.byte\t0,0\t%s end entry\n", COMMENTSTR);
	}

	(void)fprintf(outfile, "\t.byte\t0\t%s end section\n", COMMENTSTR);
	(void)fprintf(outfile, "\t.type\t%s,\"object\"\n", ABBREV_VERSION_ID);
	(void)fprintf(outfile, "\t.size\t%s,.-%s\n", ABBREV_VERSION_ID, ABBREV_VERSION_ID);
	exit(0);
	/*NOTREACHED*/
}
