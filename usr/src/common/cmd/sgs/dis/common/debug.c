#ident	"@(#)dis:common/debug.c	1.13"

#include        <stdio.h>
#include        <malloc.h>
#include	<string.h>
#include	<memory.h>

#include	"dis.h"
#include	"structs.h"
#include	"libelf.h"
#include        "sgs.h"
#include	"dwarf.h"
#include	"ccstypes.h"
#include	"data32.h"
#include	"dwarf2.h"
#include	"libdwarf2.h"

extern	int	trace; /* for debugging */
extern	int	file_byte;


static		Elf32_Shdr      *shdr;
static          Elf_Data        *debug_data;
static		unsigned char	*p_debug_data, *ptr;
static 		long		length = 0;
static		long		current;

static		long	get_long();
static		short	get_short();
static		char	get_byte();
static unsigned char	*get_string();
static		void	dwarf1(),
			dwarf2(),
			skip_attribute(),
			not_interp();

void	get_debug_info(),
	get_debug_line_info(),	 
	print_debug_line(),
	print_line();

void
get_debug_info()
{

	extern  Elf     *elf;
	extern	int	debug;
	extern	char	*fname;
	extern	int	Dwarf;
        Elf_Scn         *scn;

	if ( (scn = elf_getscn(elf, debug)) == NULL)
	{
		(void) fprintf(stderr, 
		"%dis: %s: failed to get debugging information; limited functionality\n"
		, SGS, fname);
		debug = 0;
		return;
	}
	else
		if ((shdr = elf32_getshdr(scn)) != 0)
		{
			debug_data = 0;
			if ((debug_data=elf_getdata(scn, debug_data))==0 || debug_data->d_size == 0)
                        {
				(void) fprintf(stderr,
                                "%sdis: no data in section .debug\n", SGS);
				debug = 0;
                                return;		
			}
			else
			{
				p_debug_data = (unsigned char *)debug_data->d_buf;
				ptr = p_debug_data;
				if (Dwarf ==1)
					dwarf1( shdr->sh_offset , shdr->sh_size);
				if (Dwarf == 2)
					dwarf2( shdr->sh_offset , shdr->sh_size);

			}
		}
}

/*ARGSUSED*/
static void
dwarf1( offset, size)
Elf32_Off	offset;
size_t		size;
{
	extern	void	build_labels();
	long	word;
	short	sword;
	short	attrname;
	short   len2;
	unsigned	char	*tag_source_name;
	short		tag;
	long 		tag_address;

  current = 0;
  while ( current < size)
  { 
	tag_source_name = NULL;

	word = get_long();

	if ( word <= 8)
	{
		if (trace) (void)printf("\n0x%-10lx\n", word);
		if(word < 4)
		{
			current += 4;
		}
		else
		{
			current += word;
			ptr += word - 4;
		}
		continue;
	}
	else
		current += word;

	if (trace) (void)printf("\n0x%-10lx", word);

	length = word - 4;

	tag = get_short();

	if (tag == TAG_label)
	{
		while(length > 0)
		{
			attrname = get_short();
			if (attrname == AT_name)
			{
				tag_source_name = get_string();
				if (trace)
                                	(void)printf("%s\n",tag_source_name);
			}
			else if (attrname == AT_low_pc)
			{
				tag_address = get_long();
				if (trace)
					(void)printf("0x%lx\n", tag_address);
			}
			else 
				not_interp(attrname);
		}
		build_labels(tag_source_name, tag_address);	
	}
	else
		ptr += length;
    }
  return;
}

static char 
get_byte()
{
	unsigned char 	*p;

	p = ptr; 
	++ptr;
	length -= 1;
	return *p;
}

static short
get_short()
{
	short x;

	if (file_byte == ELFDATA2MSB)
		x = MGET_SHORT(ptr);
	else
		x = LGET_SHORT(ptr);

	ptr += 2;
        length -= 2;
        return x;

}

static long
get_long()
{
	long 	x;

	if (file_byte == ELFDATA2MSB)
		x = MGET_LONG(ptr);
	else
		x = LGET_LONG(ptr);

	ptr += 4;
        length -= 4;
        return x;
}

static unsigned char *
get_string()
{
	unsigned char	*s;
	register 	int	len;
	
	len = strlen((char *)ptr) +1;
	s = (unsigned char *)malloc(len);
	(void)memcpy(s,ptr,len);
	ptr += len;
	length -= len;
	return s;

}


static void
not_interp( attrname ) 
short	attrname;
{
        short   len2;
        long    word;

        switch( attrname & FORM_MASK )
        {
                case FORM_NONE: break;
                case FORM_ADDR:
                case FORM_REF:  word = get_long();
				if (trace) (void)printf("<0x%lx>\n", word);
				break;
                case FORM_BLOCK2:       len2 = get_short();
                                        length -= len2;
                                        ptr += len2;
					if (trace) (void)printf("0x%x\n", len2);
					break;
                case FORM_BLOCK4:       word = get_long();
                                        length -= word;
                                        ptr += word;
					if (trace) (void)printf("0x%lx\n", word);
					break;
                case FORM_DATA2:        len2 = get_short();
					if (trace) (void)printf("0x%x\n", len2);
					break;
                case FORM_DATA8:        word = get_long();
					if (trace) (void)printf("0x%lx ", word);
					break;
                case FORM_DATA4:        word = get_long();
					if (trace) (void)printf("0x%lx\n", word);
					break;
                case FORM_STRING:       word = strlen((char *)ptr) + 1;
                                        length -= word;
					if (trace) (void)printf("%s\n", ptr);
                                        ptr += word;
					break;
                default:
			if (trace)
                        (void)printf("<unknown form: 0x%x>\n", (attrname & FORM_MASK) );
			length = 0;
        }
}


void
print_line(current, ptr_line, size_line)
long current;
unsigned	char *ptr_line;
size_t	size_line;
{
	extern	char	*fname;
	long  line;
	long  pcval;
	long  base_address;
	long  size;
	long delta;


	ptr = ptr_line;
	size = size_line;
	
	while (size > 0)
	{
		length = get_long();
		length -= 4;
		size -= 4;
		base_address = get_long();
		size -= 4;
	
		if(size < length-4)
		{
			(void)fprintf(stderr, "%sdis: %s: bad line info section -  size=%ld length-4=%ld\n", SGS, fname, size, length-4);
			return;
		}
		while(length > 0)
		{
			line = get_long();
			size -= 4;
			(void)get_short();
			size -= 2;
			delta = get_long();
			size -= 4;
			pcval = base_address + delta;

			if (current == pcval)
			 (void)printf("[%ld]", line);
			else
			if (current < pcval)
				return; /* can return because line
					   number info in ascending
					   order */
		}
	}
}

typedef unsigned long offset_t;
typedef unsigned long form_t;


typedef struct lineTable *LineTableList;

typedef struct lineTable {
	Dwarf2_Line_entry	*table;
	Dwarf2_Line_entry	*next;
	unsigned int		count;
} lineTable;


extern	int	trace; /* for debugging */
extern	int	file_byte;

const int header_size = 11;   /* 4 (length) + 2 (version) + 
				 4(abbreviation offset) + 1 (sizeof address) */
static Dwarf2_Abbreviation	*abbrev_table;
static Dwarf2_Abbreviation	*table;



static		Elf_Data      *debug_data;
static		Elf_Data      *abbrev_data;
static		Elf32_Shdr      *shdr;
static		Elf32_Shdr      *abbrev_shdr;
static		unsigned char	*p_debug_data, *ptr;
static		unsigned char	*p_debug_abbrev_data ;
static		long		current;
static		size_t		addr_size;



static void
dwarf2( offset, size)
Elf32_Off	offset;
size_t		size;
{

	extern	Elf	*elf;
	extern	char	*fname;
	extern	int	debug_abbrev;
	extern	void	build_labels();
	Dwarf2_Abbreviation	*abbr;
	const Dwarf2_Attribute	*attribute;
	unsigned char	*end = ptr + size;
	unsigned	char	*p_data;
	unsigned	char	*abbrev_start = 0;
	unsigned	char	*abbrev_end = 0;
	unsigned	char	*tag_source_name;
	unsigned	char	*unitend;
	long 		tag_address;
	short		tag;
	unsigned long 	index;
        Elf_Scn         *abbrev_scn;
        Elf_Scn         *info_scn;
	size_t 		abbrev_off;
	size_t		curabbrev_loc = ~0ul;
	size_t		abbrev_len;
	size_t		dbglen;
	size_t		dbgend;
	int		version;
	int		i;


/* get .debug_abbrev section if possible */

	if (debug_abbrev == 0)
	{
		abbrev_table = (Dwarf2_Abbreviation *) dwarf2_gen_abbrev_table();

	} else 
	{

		if ( (abbrev_scn = elf_getscn(elf, debug_abbrev)) == NULL)
		{
			(void) fprintf(stderr, 
			"%dis: %s: failed to get debugging information; limited functionality\n"
			, SGS, fname);
			debug_abbrev = 0;
			return;
		}
		if ((abbrev_shdr = elf32_getshdr(abbrev_scn)) != 0) 
		{
			abbrev_data = 0;
			if ((abbrev_data= elf_getdata(abbrev_scn, abbrev_data))==0
				|| abbrev_data->d_size == 0) 
			{
				(void) fprintf(stderr,
				"%sdis: no data in debugging section(s)\n", SGS);
				debug_abbrev = 0;
				return;		
			}
		}

		abbrev_start = abbrev_data->d_buf;
		abbrev_len =  abbrev_data->d_size;
		
	}
	
	/* process debugging information looking for labels */
	while ( ptr < end )
	{ 


		dbglen = get_long();
		unitend = ptr + dbglen;
		length = dbglen + 4;
		version = get_short();
		abbrev_off = get_long();
		if ((addr_size = get_byte()) != 4)
			(void) fprintf(stderr,
				"%sdis: incorrect address size\n", SGS);
		if (curabbrev_loc != abbrev_off)
		{

			if (abbrev_table != 0)
				dwarf2_delete_abbreviation_table((Dwarf2_Abbreviation *) abbrev_table);

			abbrev_table = dwarf2_get_abbreviation_table(abbrev_start + abbrev_off, abbrev_len - abbrev_off, 0);
		
			curabbrev_loc = abbrev_off;		
		}
		while (ptr < unitend)
		{

			tag_source_name = NULL;
			
			ptr += dwarf2_decode_unsigned(&index, ptr);
			abbr = &abbrev_table[index];


			for (i = abbr->nattr, attribute = abbr->attributes; i; i--, attribute++)

			{
				if (abbr->tag == DW_TAG_label)
				{
					if (attribute->name == DW_AT_name)
					{
						tag_source_name = get_string();
						if (trace)
							(void)printf("%s\n",tag_source_name);
					}
					else if (attribute->name == DW_AT_low_pc)
					{
							tag_address = get_long();
						if (trace)
							(void)printf("0x%lx\n", tag_address);
					}
					build_labels(tag_source_name, tag_address);	
				}
				else 
					skip_attribute(attribute->form);
			}
		}
	}
}



static void
skip_attribute( form ) 
unsigned long form;
{
	form_t	indirect_form;
	size_t          bytes_skipped = 0;
	long            l;
        unsigned long   ul;



         switch (form)
        {
        case DW_FORM_ref_addr:
        case DW_FORM_addr:      bytes_skipped = addr_size;      break;
        case DW_FORM_ref1:
        case DW_FORM_flag:
        case DW_FORM_data1:     bytes_skipped = 1;              break;
        case DW_FORM_ref2:
        case DW_FORM_data2:     bytes_skipped = 2;              break;
        case DW_FORM_strp:
        case DW_FORM_ref4:
        case DW_FORM_data4:     bytes_skipped = 4;              break;
        case DW_FORM_ref8:
        case DW_FORM_data8:     bytes_skipped = 8;              break;
        case DW_FORM_block1:    bytes_skipped = get_byte();     break;
        case DW_FORM_block2:    bytes_skipped = get_short();    break;
        case DW_FORM_block4:    bytes_skipped = get_long();    break;
        case DW_FORM_string:    bytes_skipped = strlen((char *)ptr) + 1; break;

        case DW_FORM_sdata:
                bytes_skipped = dwarf2_decode_signed(&l, ptr);
                break;

        case DW_FORM_ref_udata:
        case DW_FORM_udata:
                bytes_skipped = dwarf2_decode_unsigned(&ul, ptr);
                break;

        case DW_FORM_block:
                ptr += dwarf2_decode_unsigned(&ul, ptr);
                bytes_skipped = (size_t)ul;
                break;

        case DW_FORM_indirect:
                ptr += dwarf2_decode_unsigned(&indirect_form, ptr);

                skip_attribute(indirect_form);
                break;

	default:
		if (trace)
		(void)printf("<unknown form: 0x%x>\n", (form) );
		length = 0;
	}
        ptr += bytes_skipped;

}

/* get line number table information */
static	unsigned long size =  0 ;

Dwarf2_Line_entry *
get_line_table(line_sect, sect_size)
unsigned char *line_sect;
unsigned long  sect_size;
{
	extern unsigned int	LineTableSize;
	Dwarf2_Line_entry *line_table;
	LineTableList 	line_table_ptr;
	LineTableList 	line_table_last;
	LineTableList 	line_table_first;
	unsigned long	num_lines;
	unsigned long	num_bytes;
	unsigned long	line_cnt =0;
	unsigned long	bytecnt = 0;
	int	first = 1;
	int	i;
	int	index=0;

	while (bytecnt < sect_size)
	{

		if (dwarf2_read_line_header(line_sect + bytecnt, &num_bytes))
		{
			line_table_ptr = (LineTableList)calloc(1,sizeof(lineTable));
			line_table_ptr->table = dwarf2_get_line_table(&num_lines);
			line_table_ptr->count = num_lines;

			if (first)
			{
				line_table_last = line_table_first = line_table_ptr;
				first = 0;
			}
			else
			{
				line_table_last->next = line_table_ptr;
				line_table_last= line_table_ptr;
			}
				
			line_cnt += num_lines;
			bytecnt += num_bytes;
		}



		
	} 

	if ((line_table = (Dwarf2_Line_entry *) 
		malloc(line_cnt * sizeof(Dwarf2_Line_entry)))  == 0)
			return (NULL);

	/* traverse line number list and copy each table into LineTable */
	for (line_table_ptr = line_table_first; line_table_ptr->next; line_table_ptr= line_table_ptr->next);
		{
		(void)	memcpy(&line_table[index], line_table_ptr->table, line_table_ptr->count * sizeof(Dwarf2_Line_entry));
			index += line_table_ptr->count;
		}
		

	LineTableSize = line_cnt;
	size = 0;
	return(line_table);
	
}

void
print_debug_line(current)
long current;
{
	extern Dwarf2_Line_entry *LineTable;
	extern unsigned int	LineTableSize;
	Dwarf2_Line_entry *ptr = &LineTable[size]; 
	Dwarf2_Line_entry *end = &LineTable[LineTableSize]; 


	while (ptr < end)
	{
		if (current == ptr->address)
		{
			(void)printf("[%ld]", ptr->line);
			return;
		}
		if (current < ptr->address)
			return;
		ptr++;
		size++;
	}
	
}
