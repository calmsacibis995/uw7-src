#ident	"@(#)libdwarf2:common/line_table.c	1.2"

#include "dwarf2.h"
#include "libdwarf2.h"

#define	DIRECTORY_BLOCK	10
#define FILE_BLOCK	50
#define LINE_BLOCK	500

static unsigned char	*p_data;
static unsigned char	*endptr;
static unsigned char	*prologue_end;	/* start of line table itself */
static unsigned char	*file_table;	/* start of file table */

/* Values needed to interpret statement program */
static unsigned int	minimum_instruction_length;
static int		line_base;
static unsigned int	line_range;
static unsigned int	opcode_base;
static unsigned int	op255_advance;
static int		is_stmt;

/* The malloc'd tables */
static unsigned int	old_opcode_base;
static int		*opcode_lengths;

static const char	**include_directories;
static unsigned int	directory_entries;
static unsigned int	directory_entries_used;

static Dwarf2_File_entry *source_files;
static unsigned int	file_entries;
static unsigned int	file_entries_used;

static Dwarf2_Line_entry *line_table;
static unsigned int	line_entries;
static unsigned int	line_entries_used;

static unsigned short
get_2bytes()
{
	unsigned short	x;
	unsigned char	*p = (unsigned char *)&x;

	*p = *p_data; ++p_data; ++p;
	*p = *p_data; ++p_data;
	return x;
}

static unsigned long
get_4bytes()
{
	unsigned long	x;
	unsigned char	*p = (unsigned char *)&x;

	*p = *p_data; ++p_data; ++p;
	*p = *p_data; ++p_data; ++p;
	*p = *p_data; ++p_data; ++p;
	*p = *p_data; ++p_data;
	return x;
}

/* read standard opcodes lengths table */
static int
read_opcode_lengths()
{
	int i;
	if (opcode_base != old_opcode_base)
	{
		if (opcode_lengths)
			free(opcode_lengths);
		opcode_lengths = (int *)malloc((opcode_base + 1) * sizeof(int));
		if (!opcode_lengths)
			return 0;
		old_opcode_base = opcode_base;
	}
	opcode_lengths[0] = 0;
	for (i = 1; i < opcode_base; i++)
	{
		opcode_lengths[i] = *p_data++;
	}
	return 1;
}

int *
dwarf2_get_opcode_length_table(unsigned long *nentries)
{
	*nentries = old_opcode_base + 1;
	return opcode_lengths;
}

static int
read_include_directories()
{
	if (!include_directories)
	{
		if ((include_directories = (const char **)malloc(sizeof(const char **) 
							* DIRECTORY_BLOCK)) == 0)
			return 0;
		directory_entries = DIRECTORY_BLOCK;
	}
	directory_entries_used = 1;
	include_directories[0] = 0;
	while (*p_data)
	{
		if (directory_entries_used == directory_entries)
		{
			directory_entries += DIRECTORY_BLOCK;
			include_directories = (const char **)realloc(include_directories,
							sizeof(const char **) * directory_entries);
		}
		if (!include_directories)
			return 0;

		include_directories[directory_entries_used++] = (char *)p_data;
		p_data += strlen((char *)p_data) + 1;
	}
	p_data++;
	file_table = p_data;
}

const char **
dwarf2_get_include_table(unsigned long *nentries)
{
	*nentries = directory_entries_used;
	return include_directories;
}

const Dwarf2_File_entry *
dwarf2_get_file_table(unsigned long *nentries)
{
	Dwarf2_File_entry	*data;
	unsigned long	index, fsize, mtime;

	p_data = file_table;
	if (!source_files)
	{
		if ((source_files = (Dwarf2_File_entry *)malloc(sizeof(Dwarf2_File_entry) 
						* FILE_BLOCK)) == 0)
			return 0;
		file_entries = FILE_BLOCK;
	}
	file_entries_used = 1;
	source_files[0].name = 0;
	source_files[0].dir_index = 0;
	while (*p_data)
	{
		if (file_entries_used == file_entries)
		{
			file_entries += FILE_BLOCK;
			source_files = (Dwarf2_File_entry *)realloc(source_files,
						sizeof(Dwarf2_File_entry) * file_entries);
		}
		if (!source_files)
			return 0;

		data = &source_files[file_entries_used++];
		data->name = (char *)p_data;
		p_data += strlen(data->name) + 1;
		p_data += dwarf2_decode_unsigned(&index, p_data);
		data->dir_index = index;
		p_data += dwarf2_decode_unsigned(&mtime, p_data);
		data->time = (time_t)mtime;
		p_data += dwarf2_decode_unsigned(&fsize, p_data); /* unused, for now */
	}

	*nentries = file_entries_used;
	return source_files;
}

int
dwarf2_read_line_header(unsigned char *data_ptr, unsigned long *comp_len)
{
	unsigned int	version;
	size_t		len;

	p_data = data_ptr;

	/* read statement prologue */
	len = get_4bytes();
	endptr = p_data + len;
	*comp_len = len + 4;
	version = get_2bytes();
	prologue_end = p_data + get_4bytes();
	minimum_instruction_length = *p_data++;
	is_stmt = *p_data++;
	line_base = (signed char)*p_data++;
	line_range = *p_data++;
	opcode_base = *p_data++;

	if (version != 2 || prologue_end > endptr)
		return 0;

	/* Have to read through opcode and directory tables to find start of
	   file name table, might as well go ahead and set up the file table */
	if (!read_opcode_lengths()
		|| !read_include_directories())
		return 0;
	return 1;
}

static int
add_line(unsigned long addr, unsigned long line, unsigned long file)
{
	Dwarf2_Line_entry	*data;

	if (!line_table)
	{
		if ((line_table = (Dwarf2_Line_entry *)malloc(sizeof(Dwarf2_Line_entry) 
						* LINE_BLOCK)) == 0)
			return 0;
		line_entries = LINE_BLOCK;
	}
	else if (line_entries_used == line_entries)
	{
		line_entries += LINE_BLOCK;
		line_table = (Dwarf2_Line_entry *)realloc(line_table,
					sizeof(Dwarf2_Line_entry) * line_entries);
	}
	if (!line_table)
		return 0;

	data = &line_table[line_entries_used++];
	data->address = addr;
	data->line = line;
	data->file_index = file;
	return 1;
}

const Dwarf2_Line_entry *
dwarf2_get_line_table(unsigned long *nentries)
{
	/* state machine variables */
	unsigned long	address = 0;
	unsigned long	file = 1;
	unsigned long	line = 1;
	int		end_sequence = 0;

	/* These are not really used */
	unsigned long	column = 0;
	int		basic_block;

	int		i;

	p_data = prologue_end;
	op255_advance = (255 - opcode_base)/line_range;
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
				end_sequence = 1;
				break;
			case DW_LNE_set_address:
				address = (unsigned long)get_4bytes();
				break;
			case DW_LNE_define_file:	/* MORE */
			default:	/* skip if not recognized */
				p_data += osize - 1;
				break;
			}
			break;
		}

		case DW_LNS_copy:
			if (!add_line(address, line, file))
				return 0;
			basic_block = 0;
			break;

		case DW_LNS_advance_pc:
			p_data += dwarf2_decode_unsigned(&advance, p_data);
			advance *= minimum_instruction_length;
			address += advance;
			break;

		case DW_LNS_advance_line:
			p_data += dwarf2_decode_signed(&sadvance, p_data);
			line += sadvance;
			break;

		case DW_LNS_set_file:
			p_data += dwarf2_decode_unsigned(&file, p_data);
			break;

		case DW_LNS_set_column:
			p_data += dwarf2_decode_unsigned(&column, p_data);
			break;

		case DW_LNS_negate_stmt:	
			is_stmt = !is_stmt;
			break;

		case DW_LNS_fixed_advance_pc:	
			advance = get_2bytes();	
			address += advance;
			break;

		case DW_LNS_const_add_pc:
			address += op255_advance; 
			break;

		case DW_LNS_set_basic_block:	/* unused */
			basic_block = 1;
			break;

		default:
			if (opcode < opcode_base)
			{
				/* skip over unknown standard opcodes */
				 
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
			if (!add_line(address, line, file))
				return 0;
			break;
		}
	}

	*nentries = line_entries_used;
	if (!line_table && !line_entries_used)
	{
		/* no line numbers -- not an error case so still should
		** return a non-null pointer
		*/
		if ((line_table = (Dwarf2_Line_entry *)malloc(
						sizeof(Dwarf2_Line_entry) 
						* LINE_BLOCK)) == 0)
			return 0;
		line_entries = LINE_BLOCK;
		line_table[0].address = 0;
		line_table[0].line = 0;
		line_table[0].file_index = 0;
	}
	return line_table;
}

void
dwarf2_reset_line_table(int free_space)
{
	if (free_space)
	{
		if (line_table)
		{
			free(line_table);
			line_table = 0;
		}
		line_entries = 0;
	}
	line_entries_used = 0;
}
