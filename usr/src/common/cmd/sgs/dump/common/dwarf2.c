#ident	"@(#)dump:common/dwarf2.c	1.9"

/* This file contains all of the functions necessary
 * to interpret DWARF II sections.
 */

#include        <stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>

#include	"libelf.h"
#include	"dwarf2.h"
#include	"libdwarf2.h"
#include	"dump.h"

static	const Dwarf2_Abbreviation	*abbrev_table;
static	int			address_size = sizeof(void *);

static unsigned short
get_2bytes(unsigned char *ptr)
{
	unsigned short	x;
	unsigned char	*p = (unsigned char *)&x;

	*p = *ptr; ++ptr; ++p;
	*p = *ptr;
	return x;
}

static unsigned long
get_4bytes(unsigned char *ptr)
{
	unsigned long	x;
	unsigned char	*p = (unsigned char *)&x;

	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr;
	return x;
}

#if LONG_LONG
static unsigned long long
get_8bytes(unsigned char *ptr)
{
	unsigned long long	x;
	unsigned char	*p = (unsigned char *)&x;

	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr;
	return x;
}
#endif

void
dump_dwarf2_abbreviations(Elf *elf, const char *filename)
{
	unsigned char		*p_data;
	size_t			size;

	if (!p_flag)
	{
		(void)printf("    ***** DWARF2 ABBREVIATION TABLE *****\n");
		(void)printf("%-7s %-20s %-13s %-18s %s\n",
			"Index",
			"Tag",
			"Children",
			"Dwarf2_Attribute",
			"Form");
	}

	if ((p_data = (unsigned char *)get_scndata(p_abbreviations->p_sd,
				&size)) == NULL)
	{
		(void)fprintf(stderr, "%s: %s: no data in %s section\n",
			prog_name, filename, p_abbreviations->scn_name);
		return;
	}

	while(size)
	{
		unsigned int		bytes_used;
		const Dwarf2_Abbreviation	*table;
		int			index = 1;

		if ((abbrev_table = dwarf2_get_abbreviation_table(p_data, size, &bytes_used)) == 0)
		{
			(void)fprintf(stderr, "%s: cannot allocate space\n", prog_name);
			return;
		}
		for (table = &abbrev_table[1]; table->tag; table++, index++)
		{
			int i;
			const Dwarf2_Attribute *attr = table->attributes;

			(void)printf("%5d   %-20s %-13s ",
				index, dwarf2_tag_name(table->tag),
				table->children ? "yes" : "no");
			(void)printf("%-18s %-10s\n",
				dwarf2_attribute_name(attr->name),
				dwarf2_form_name(attr->form));
			for (i = 1, ++attr; i < table->nattr; i++, attr++)
			{
				(void)printf("%-42s %-18s %-10s\n", "",
					dwarf2_attribute_name(attr->name),
					dwarf2_form_name(attr->form));
			}
		}
		p_data += bytes_used;
		size -= bytes_used;
		dwarf2_delete_abbreviation_table((Dwarf2_Abbreviation *)abbrev_table);
	}
}

static size_t
print_header(unsigned char *p_data, size_t *file_length, 
	unsigned long *abbrev_offset)
{
	short		version;
	char		size;

	*file_length = get_4bytes(p_data);
	p_data += 4;
	version = get_2bytes(p_data);
	p_data += 2;
	*abbrev_offset = get_4bytes(p_data);
	p_data += 4;
	size = *p_data;
	p_data++;

	(void)printf("length = %d, version = %d, abbreviation offset = %#x, address size = %d\n",
		*file_length, version, *abbrev_offset, size);
	return 11;
}

static size_t
print_block2(unsigned char *p_data)
{
	size_t	len = get_2bytes(p_data++);
	int	i;
	for (i = 0; i < len; i++)
	{
		(void)printf("%#x ", *p_data++);
		if (len%10 == 0)
			putchar('\n');
	}
	return len + 2;
}

static size_t
print_block4(unsigned char *p_data)
{
	size_t	len = get_4bytes(p_data++);
	int	i;
	for (i = 0; i < len; i++)
	{
		(void)printf("%#x ", *p_data++);
		if (len%10 == 0)
			putchar('\n');
	}
	return len + 4;
}

static size_t
get_constant(unsigned char *p_data, int form, unsigned long *val, 
	const char *filename)
{
	switch (form)
	{
	case DW_FORM_data1:
		*val = (unsigned long) *p_data;
		return 1;

	case DW_FORM_data2:
		*val = (unsigned long) get_2bytes(p_data);
		return 2;

	case DW_FORM_data4:
		*val = (unsigned long) get_4bytes(p_data);
		return 4;

	case DW_FORM_udata:
		return dwarf2_decode_unsigned(val, p_data);

	case DW_FORM_sdata:
	{
		long lval;
		size_t len = dwarf2_decode_signed(&lval, p_data);
		*val = (unsigned long) lval;
		return len;
	}

	default:
		(void)fprintf(stderr, "%s: %s: bad form %#x for constant\n",
			prog_name, filename, form);
		return 0;

	}
}

static size_t
print_data1(unsigned char *p_data)
{
	(void)printf("%#x\n", *(unsigned char *)p_data);
	return 1;
}

static size_t
print_data2(unsigned char *p_data)
{
	(void)printf("%#x\n", get_2bytes(p_data));
	return 2;
}

static size_t
print_data4(unsigned char *p_data)
{
	(void)printf("%#x\n", get_4bytes(p_data));
	return 4;
}

static size_t
print_data8(unsigned char *p_data)
{
#if LONG_LONG
	(void)printf("%#Lx\n", get_8bytes(p_data));
#else
	printf("unimplemented\n");
#endif
	return 8;
}

static size_t
print_string(unsigned char *p_data)
{
	size_t	len = strlen((char *)p_data);
	(void)printf("\"%s\"\n", p_data);
	return len+1;
}

static size_t
print_addr(unsigned char *p_data)
{
	if (address_size == 4)
		return print_data4(p_data);
	else if (address_size == 8)
		return print_data8(p_data);
}

static size_t
print_block(unsigned char *p_data)
{
	unsigned long	len;
	size_t		size;
	int		i;

	size = dwarf2_decode_unsigned(&len, p_data);
	p_data += size;
	for (i = 0; i < len; i++)
	{
		(void)printf("%#x ", *p_data++);
		if (len%10 == 0)
			putchar('\n');
	}
	return len + size;
}

static size_t
print_block1(unsigned char *p_data)
{
	size_t	len = *p_data++;
	int	i;
	for (i = 0; i < len; i++)
	{
		(void)printf("%#x ", *p_data++);
	}
	putchar('\n');
	return len + 1;
}

static size_t
print_flag(unsigned char *p_data)
{
	(void)printf("%d\n", *(unsigned char *)p_data);
	return 1;
}

static size_t
print_sdata(unsigned char *p_data)
{
	long	val;
	size_t	len;

	len = dwarf2_decode_signed(&val, p_data);
	printf("%d\n", val);
	return len;
}

static size_t
print_strp(unsigned char *p_data)
{
	printf("unimplemented\n");
	return 4;
}

static size_t
print_udata(unsigned char *p_data)
{
	unsigned long	val;
	size_t		len;

	len = dwarf2_decode_unsigned(&val, p_data);
	printf("%u ", val);
	return len;
}

static size_t
print_ref_addr(unsigned char *p_data)
{
	if (address_size == 4)
		return print_data4(p_data);
	else if (address_size == 8)
		return print_data8(p_data);
}


static size_t
print_ref1(unsigned char *p_data, int comp_unit)
{
	(void)printf("%d:%#x\n", comp_unit, *p_data);
	return 1;
}


static size_t
print_ref2(unsigned char *p_data, int comp_unit)
{
	printf("%d:%#x\n", comp_unit, get_2bytes(p_data));
	return 2;
}

static size_t
print_ref4(unsigned char *p_data, int comp_unit)
{
	printf("%d:%#x\n", comp_unit, get_4bytes(p_data));
	return 4;
}

static size_t
print_ref8(unsigned char *p_data, int comp_unit)
{
#if LONG_LONG
	printf("%d:%#Lx\n", comp_unit, get_8bytes(p_data));
#else
	printf("unimplemented\n");
#endif
	return 8;
}

static size_t
print_ref_udata(unsigned char *p_data, int comp_unit)
{
	unsigned long	val;
	size_t		len;

	len = dwarf2_decode_unsigned(&val, p_data);
	printf("%d:%#x\n", comp_unit, val);
	return len;
}

static size_t
print_using_form(unsigned char *p_data, int form, const char *filename,
	int comp_unit)
{
	unsigned long	indirect_form;
	size_t		nbytes;

	switch (form)
	{
	case DW_FORM_block:	return print_block(p_data);	
	case DW_FORM_addr:	return print_addr(p_data);	
	case DW_FORM_block2:	return print_block2(p_data);	
	case DW_FORM_block4:	return print_block4(p_data);	
	case DW_FORM_data2:	return print_data2(p_data);	
	case DW_FORM_data4:	return print_data4(p_data);	
	case DW_FORM_data8:	return print_data8(p_data);	
	case DW_FORM_string:	return print_string(p_data);	
	case DW_FORM_block1:	return print_block1(p_data);	
	case DW_FORM_data1:	return print_data1(p_data);	
	case DW_FORM_flag:	return print_flag(p_data);	
	case DW_FORM_sdata:	return print_sdata(p_data);	
	case DW_FORM_strp:	return print_strp(p_data);	
	case DW_FORM_ref_addr:	return print_ref_addr(p_data);
	case DW_FORM_ref1:	return print_ref1(p_data, comp_unit);
	case DW_FORM_ref2:	return print_ref2(p_data, comp_unit);	
	case DW_FORM_ref4:	return print_ref4(p_data, comp_unit);	
	case DW_FORM_ref8:	return print_ref8(p_data, comp_unit);	
	case DW_FORM_ref_udata:	return print_ref_udata(p_data, comp_unit);
	case DW_FORM_udata:
		nbytes = print_udata(p_data);	
		putchar('\n');
		return nbytes;

	case DW_FORM_indirect:
		nbytes = dwarf2_decode_unsigned(&indirect_form,
			p_data);
		p_data += nbytes;
		nbytes += print_using_form(p_data, (int)indirect_form,
			filename, comp_unit);
		return nbytes;
	default:
		(void)fprintf(stderr, "%s: %s: unrecognized form %#x\n",
			prog_name, filename, form);
		return 0;
	}
}

static size_t
get_size(unsigned char *p_data, int form, size_t *size, const char *filename)
{
	switch (form)
	{
	case DW_FORM_block1:
		*size = *p_data;
		return 1;
	case DW_FORM_block2:
		*size = get_2bytes(p_data);
		return 2;
	case DW_FORM_block4:
		*size = get_4bytes(p_data);
		return 4;

	case DW_FORM_block:
	{
		unsigned long	val;
		unsigned int	len;
		len = dwarf2_decode_unsigned(&val, p_data);
		*size = val;
		return len;
	}

	default:
		(void)fprintf(stderr, "%s: %s: bad form %#x for block\n",
			prog_name, filename, form);
		return 0;
	}
}

static char *level_indicator = "||||||||||||||||||||||||||||||||||||||||||||||||";

static size_t
print_location(unsigned char *p_data, size_t size, int level)
{
	unsigned char	*p_end = p_data + size;
	int		first_time = 1;

	if (!size)
	{
		putchar('\n');
		return 0;
	}

	while (p_data < p_end)
	{
		unsigned int	opcode = *p_data++;
		size_t		len = 0;

		if (first_time)
		{
			first_time = 0;
		}
		else
		{
			(void)printf("%.*s %-30s ", level, level_indicator, "");
		}

		(void)printf("%s ", dwarf2_location_op_name(opcode));
		switch (opcode)
		{
		case DW_OP_addr:
			len = print_addr(p_data);
			break;

		case DW_OP_const1u:
		case DW_OP_const1s:
		case DW_OP_pick:
		case DW_OP_xderef_size:
			len = print_data1(p_data);
			break;

		case DW_OP_const2u:
		case DW_OP_const2s:
		case DW_OP_skip:
		case DW_OP_bra:
			len = print_data2(p_data);
			break;

		case DW_OP_const4u:
		case DW_OP_const4s:
			len = print_data4(p_data);
			break;

		case DW_OP_const8u:
		case DW_OP_const8s:
			len = print_data8(p_data);
			break;

		case DW_OP_constu:
		case DW_OP_plus_uconst:
		case DW_OP_regx:
		case DW_OP_piece:
			len = print_udata(p_data);
			putchar('\n');
			break;

		case DW_OP_SCO_reg_pair:
			len = print_udata(p_data);
			len += print_udata(p_data+len);
			putchar('\n');
			break;

		case DW_OP_consts:
		case DW_OP_fbreg:
		case DW_OP_breg0:
		case DW_OP_breg1:
		case DW_OP_breg2:
		case DW_OP_breg3:
		case DW_OP_breg4:
		case DW_OP_breg5:
		case DW_OP_breg6:
		case DW_OP_breg7:
		case DW_OP_breg8:
		case DW_OP_breg9:
		case DW_OP_breg10:
		case DW_OP_breg11:
		case DW_OP_breg12:
		case DW_OP_breg13:
		case DW_OP_breg14:
		case DW_OP_breg15:
		case DW_OP_breg16:
		case DW_OP_breg17:
		case DW_OP_breg18:
		case DW_OP_breg19:
		case DW_OP_breg20:
		case DW_OP_breg21:
		case DW_OP_breg22:
		case DW_OP_breg23:
		case DW_OP_breg24:
		case DW_OP_breg25:
		case DW_OP_breg26:
		case DW_OP_breg27:
		case DW_OP_breg28:
		case DW_OP_breg29:
		case DW_OP_breg30:
		case DW_OP_breg31:
			len = print_sdata(p_data);
			break;
		
		case DW_OP_bregx:
			len = print_sdata(p_data);
			len += print_udata(p_data);
			putchar('\n');
			break;

		default:
			putchar('\n');
			break;
		}
		p_data += len;
	}
	return size;
}

static size_t
print_encoding(unsigned char *p_data, int form, const char *filename)
{
	unsigned long	val;
	size_t		len = get_constant(p_data, form, &val, filename);
	(void)printf("%s\n", dwarf2_base_type_encoding_name(val));
	return len;
}

static size_t
print_language(unsigned char *p_data, int form, const char *filename)
{
	unsigned long	val;
	size_t		len = get_constant(p_data, form, &val, filename);

	(void)printf("%s\n", dwarf2_language_name(val));
	return len;
}

static unsigned char *
print_record(unsigned char *p_data, unsigned char *p_end, 
	unsigned long offset, int level, const char *filename, 
	int comp_unit, int *end_of_chain)
{
	const Dwarf2_Abbreviation	*abbr;
	const Dwarf2_Attribute	*attribute;
	int			i;
	unsigned char		*p_start = p_data;
	unsigned long		index;
	const char		*tag_name;

	p_data += dwarf2_decode_unsigned(&index, p_data);
	abbr = &abbrev_table[index];
	if (index)
		tag_name = dwarf2_tag_name(abbr->tag);
	else
	{
		if (end_of_chain)
			*end_of_chain = 1;
		tag_name = "NULL_ENTRY";
	}

	(void)printf("%.*s%s (%d:%#x):\n",
		level, level_indicator, tag_name, comp_unit, offset);


	for (i = abbr->nattr, attribute = abbr->attributes; i; i--, attribute++)
	{
		(void)printf("%.*s%-10s %-20s ",
			level, level_indicator, "",
			dwarf2_attribute_name(attribute->name));

		switch (attribute->name)
		{
		case DW_AT_use_location:
			if (attribute->form == DW_FORM_ref4)
			{
				p_data += print_using_form(p_data, 
					attribute->form, filename, comp_unit);
				break;
			}
			/*FALL-THROUGH*/
		case DW_AT_location:
		case DW_AT_data_member_location:
		case DW_AT_vtable_elem_location:
		{
			size_t	size;
			p_data += get_size(p_data, attribute->form, 
				&size, filename);
			p_data += print_location(p_data, size, level);
			break;
		}
		case DW_AT_encoding:
			p_data += print_encoding(p_data, 
				attribute->form, filename);
			break;

		case DW_AT_language:
			p_data += print_language(p_data, 
				attribute->form, filename);
			break;

		default:
			p_data += print_using_form(p_data, 
				attribute->form, filename, comp_unit);
			break;
		}
	}
	(void)printf("%.*s\n", level, level_indicator);

	if (abbr->children)
	{
		int	chain_end = 0;
		while ((p_data < p_end) && !chain_end)
		{
			unsigned long	choffset = offset + 
				(p_data - p_start);
			p_data = print_record(p_data, p_end, choffset,
				level+1, filename, comp_unit, &chain_end);
		}
	}
	return p_data;
}

/*
 * Get the debugging data and call print_record to process it.
 */

void
dump_dwarf2_debug(Elf *elf, const char *filename, SCNTAB *pscn)
{
	size_t		size, abbrev_size;
	unsigned char	*p_data;
	unsigned char	*abbrev_data;
	unsigned char	*p_end;
	unsigned char	*p_start;
	size_t		file_length;
	size_t		header_length;
	int		comp_unit = 0;

	if (!p_flag)
	{
		(void)printf("    ***** DEBUGGING INFORMATION *****\n");
		(void)printf("           %-20s %s\n\n",
			"Dwarf2_Attribute",
			"Value");
	}

	if (!p_abbreviations)
	{
		/* no .debug_abbreviation section - use standard shared
		   abbreviation table */
		if ((abbrev_table = dwarf2_gen_abbrev_table()) == 0)
		{
			(void)fprintf(stderr, "%s: cannot allocate space\n",
					prog_name);
			return;
		}
	}
	else if ((abbrev_data = 
		(unsigned char *)get_scndata(p_abbreviations->p_sd,
			&abbrev_size)) == NULL)
	{
		(void)fprintf(stderr, "%s: %s: no data in %s section\n",
			prog_name, filename, p_abbreviations->scn_name);
		return;
	}

	if ((p_data = (unsigned char *)get_scndata(pscn->p_sd, &size)) == NULL)
	{
		(void)fprintf(stderr, "%s: %s: no data in %s section\n",
			prog_name, filename, pscn->scn_name);
		return;
	}

	while (size)
	{
		unsigned long		abbrev_offset;

		comp_unit++;
		p_start = p_data;
		header_length = print_header(p_data, &file_length,
			&abbrev_offset);
		p_end = p_data + file_length + 4;
			/* 4 is size of length field */
		p_data += header_length;
		if (p_abbreviations)
		{
			static unsigned long prev_abbrev_offset;
			if (abbrev_table && (abbrev_offset != prev_abbrev_offset))
				dwarf2_delete_abbreviation_table(
					(Dwarf2_Abbreviation *)abbrev_table);
			if (!abbrev_table || (abbrev_offset != prev_abbrev_offset))
			{
				abbrev_table = dwarf2_get_abbreviation_table(
						abbrev_data + abbrev_offset,
						abbrev_size - abbrev_offset, 0);
			}
			if (!abbrev_table)
			{
				(void)fprintf(stderr, 
					"%s: cannot allocate space\n",
					prog_name);
				return;
			}
			prev_abbrev_offset = abbrev_offset;
		}
		while (p_data < p_end)
		{
			unsigned long	offset = p_data - p_start;

			p_data = print_record(p_data, p_end, offset,
				0, filename, comp_unit, 0);
		}
		size -= file_length + 4;
	}
	return;
}


/* dump line information */
void
dump_dwarf2_line(Elf *elf, const char *filename, SCNTAB *pscn)
{
	size_t		size;
	unsigned char	*p_data;
	unsigned int	old_opcode_base = 0;
	int		*opcode_lengths = 0;

	if (!p_flag)
	{
		(void)printf("    ***** LINE NUMBER INFORMATION *****\n");
	}
	if ((p_data = (unsigned char *)get_scndata(pscn->p_sd, &size)) == NULL)
	{
		(void)fprintf(stderr, "%s: %s: no data in %s section\n",
			prog_name, filename, pscn->scn_name);
		return;
	}

	while(size)
	{
		int		comp_len;
		unsigned char	*endptr;
		unsigned char	*prologue_end;
		unsigned int	version;
		unsigned int	minimum_instruction_length;
		int		line_base;
		unsigned int	line_range;
		unsigned int	opcode_base;
		unsigned int	op255_advance;

		/* state machine variables */
		unsigned long	address = 0;
		unsigned long	file = 1;
		unsigned long	line = 1;
		int		end_sequence = 0;

		/* These are not really used */
		unsigned long	column = 0;
		int		is_stmt;
		int		basic_block;

		int		i;

		/* read statement prologue */
		comp_len = get_4bytes(p_data);
		p_data += 4;
		endptr = p_data + comp_len;
		size -= (comp_len + 4);
		version = get_2bytes(p_data);
		p_data += 2;
		prologue_end = p_data + get_4bytes(p_data);
		printf("Compilation unit entry: length %d; version %d; prolog length %d\n", 
			comp_len, version, prologue_end-p_data);
		p_data += 4;
		minimum_instruction_length = *p_data++;
		is_stmt = *p_data++;
		line_base = (signed char)*p_data++;
		line_range = *p_data++;
		opcode_base = *p_data++;
		printf("Minimum instruction length: %d\n",
			minimum_instruction_length);
		printf("Default is statement: %d\n", is_stmt);
		printf("Line base: %d\n", line_base);
		printf("Line range: %d\n", line_range);
		printf("Opcode base: %d\n", opcode_base);

		if (version != 2 || prologue_end > endptr)
		{
			(void)(fprintf, stderr, "%s: %s:Badly formed line number prolog: %s\n",
				prog_name, filename, pscn->scn_name);
			return;
		}

		/* read standard opcodes lengths table */
		if (opcode_base != old_opcode_base)
		{
			if (opcode_lengths)
				free(opcode_lengths);
			opcode_lengths = (int *)malloc((opcode_base +1)
				* sizeof(int));
			if (!opcode_lengths)
			{
				(void)(fprintf, stderr, "%s: cannot malloc space\n",
					prog_name);
				return;
			}
			old_opcode_base = opcode_base;
		}
		opcode_lengths[0] = 0;
		for (i = 1; i < opcode_base; i++)
		{
			opcode_lengths[i] = *p_data++;
		}
		printf("Opcode lengths:");
		for(i = 1; i < opcode_base; i++)
			printf(" %d", opcode_lengths[i]);
		printf("\n\nInclude Directories:\n");
		i = 0;
		while(*p_data)
		{
			int len = strlen((char *)p_data);
			i++;
			printf("%-5d %s\n", i, p_data);
			p_data += len + 1;
		}
		p_data++;
		printf("\nFile Table:\n");
		printf("%-6s %-9s %-30s %8s %s\n", 
			"File",
			"Directory",
			"LastModified",
			"Size",
			"Name");
		i = 1;
		while(*p_data)
		{
			unsigned long	index, fsize, mtime;
			char		*name;
			int		len;
			char		tstr[BUFSIZ];
			name = (char *)p_data;
			len = strlen(name);
			p_data += len + 1;
			p_data += dwarf2_decode_unsigned(&index, p_data);
			p_data += dwarf2_decode_unsigned(&mtime, p_data);
			p_data += dwarf2_decode_unsigned(&fsize, p_data);
			cftime(tstr, 0, (time_t *)&mtime);

			printf("%-6d %-9d %-30s %8d %s\n", i, index, tstr, fsize, name);
			i++;
		}
		p_data++;
		op255_advance = (255 - opcode_base)/line_range;
		printf("\nStatement Program:\n");
		printf("%-14s %7s %12s %11s %8s %5s\n",
			"Opcode",
			"PCAdvance",
			"LineAdvance",
			"Address",
			"Line",
			"File");
		while(p_data < endptr && !end_sequence)
		{
			unsigned long	advance;
			long		sadvance;
			unsigned char	save_opcode;
			unsigned char	opcode = *p_data++;
			switch (opcode)
			{
			case 0:
			{
				/* extended opcodes */
				unsigned long	osize;
				unsigned char	subop;
				p_data += dwarf2_decode_unsigned(&osize, p_data);
				subop = *p_data++;
				switch (subop)
				{
				case DW_LNE_end_sequence:	
					printf("end_sequence\n\n");
					end_sequence = 1;
					break;
				case DW_LNE_set_address:
					address = (unsigned long)get_4bytes(p_data);
					printf("%-16s %7s %12s %#11x %8s %5s\n",
						"set_address", "", "",
						address, "", "");
					p_data += 4;
					break;
				case DW_LNE_define_file:	/* MORE */
					printf("define_file\n");
				default:	/* skip if not recognized */
					p_data += osize - 1;
					break;
				}
			}
			break;

			case DW_LNS_copy:
				printf("%-16s %7s %12s %#11x %8d %5d\n",
					"copy", "", "",
					address, line, file);
				basic_block = 0;
				break;

			case DW_LNS_advance_pc:
				p_data += dwarf2_decode_unsigned(&advance, p_data);
				advance *= minimum_instruction_length;
				address += advance;
				printf("%-16s %7d %12s %#11x %8s %5s\n",
					"advance_pc", advance, "",
					address, "", "");
				break;

			case DW_LNS_advance_line:
				p_data += dwarf2_decode_signed(&sadvance, p_data);
				line += sadvance;
				printf("%-16s %7s %12d %11s %8d %5s\n",
					"advance_line", "", sadvance,
					"", line, "");
				break;

			case DW_LNS_set_file:
				p_data += dwarf2_decode_unsigned(&file, p_data);
				printf("%-16s %7s %12s %11s %8s %5d\n",
					"set_file", "", "",
					"", "", file);
				break;

			case DW_LNS_set_column:
				p_data += dwarf2_decode_unsigned(&column, p_data);
				printf("set_column %d\n", column);
				break;

			case DW_LNS_negate_stmt:	
				is_stmt = !is_stmt;
				printf("negate_stmt: is statment: %d\n", is_stmt);
				break;

			case DW_LNS_fixed_advance_pc:	
				advance = get_2bytes(p_data);	
				p_data += 2;
				address += advance;
				printf("%-16s %7s %12s %#11x %8s %5s\n",
					"fixed_advance_pc", advance, "",
					address, "", "");
				break;

			case DW_LNS_const_add_pc:
				address += op255_advance; 
				printf("%-16s %7s %12s %#11x %8s %5s\n",
					"const_advance_pc", op255_advance, "",
					address, "", "");
				break;

			case DW_LNS_set_basic_block:	/* unused */
				printf("set_basic_block\n");
				basic_block = 1;
				break;

			default:
				if (opcode < opcode_base)
				{
					/* skip over unknown standard opcodes */
					fprintf(stderr,"%s: %s: Unknown standard opcode in %s\n",
						prog_name, filename, pscn->scn_name);
					 
					for (i = 0; i < opcode_lengths[opcode]; i++)
						p_data += dwarf2_decode_unsigned(&advance, p_data);
					continue;
				}

				/* decode special opcodes */
				save_opcode = opcode;
				opcode -= opcode_base;
				advance = opcode/line_range;
				sadvance = line_base + (opcode % line_range);
				address += advance;
				line += sadvance;
				printf("special_%-8d %7d %12d %#11x %8d %5d\n",
					save_opcode, advance, sadvance,
						address, line, file);
				break;
			}
		}

	}
	if (opcode_lengths)
		free(opcode_lengths);
}
