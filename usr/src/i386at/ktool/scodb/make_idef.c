#ident	"@(#)ktool:i386at/ktool/scodb/make_idef.c	1.1"
/*
 * make_idef - generate Intermediate Definition (idef) file for scodb
 *
 * Usage:	make_idef <filename>
 *
 * This program takes as argument an ELF module or binary that has been
 * compiled with "cc -g", and extracts the structure, union and variable
 * definitions contained in the DWARF information.  The information is
 * written to standard output in scodb Intermediate Definition (idef)
 * format.
 *
 * Another program (called ?) takes a number of such idef files and generates
 * a single output file.  This output file is taken by unixsyms and added onto
 * the end of the KDB symbol table and command strings, to be picked by scodb.
 * The intention is that a single idef file containing the core kernel
 * structures is supplied by default, and any additional definitions required
 * can be generated as idef files and added on to the structure definitions
 * used by scodb (rather like incremental linking).
 */

#include <stdio.h>
#include "libelf.h"
#include "dwarf.h"
#include <syms.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/scodb/stunv.h>

/*
 * NOTE:
 *
 * - more than NDIM subscripts may cause problems,
 */

#define MAXDIMS 4	/* maximum dimensions extractable from ELF */

#define MAXENUM 100
#define NAMEL	32
#define NDIM	4	/* maximum dimensions that scodb can handle */

extern int errno;

/*
 * For each structure/union definition, the "lines" structure is
 * populated with information as it is extracted from the DWARF
 * data.  It acts as a temporary holding area for the information
 * before being passed down to lower levels to be converted into
 * "idef" format.  It's existence is more historical than functional.
 */

struct lines {
	struct line	line;
	int	l_flags;
};

/*
 * Definition of the l_flags field.  Each bit indicates whether the
 * corresponding entry in the structure is valid or not.
 */

#define LINEF_NAME	1
#define LINEF_SCLASS	2
#define LINEF_SIZE	4
#define LINEF_OFFSET	8
#define LINEF_TYPE	16
#define LINEF_DIM	32
#define LINEF_TAG	64

/*
 * Definitions for accessing the encapsulated "line" structure
 * within the greater "lines" structure.
 */

#define l_name		line.l_name
#define l_sclass	line.l_sclass
#define l_size		line.l_size
#define l_offset	line.l_offset
#define l_type		line.l_type
#define l_dim		line.l_dim
#define l_tag		line.l_tag

/*
 * Some embedded test definitions
 */

struct test1 {
	int	a;
};

union testun {
	int a;
	char b;
	short c;
	void *d;
};

enum testenum {
	A, B, C, D
};

struct testarray {
	struct	test1 (*func)();
	int	a[2][3];
	int 	b[2][3][4];
	int	c[2][3][4][5];
	int	*d[2];
	int	**e[2];
	struct	testarray *f[2];
	struct	test1 g[2];
	int	(*h)();
	char	**i[1][2];
	void	*j;
	struct	test1	*k;
	struct	test1	**l;
	int	**m;
	void	*(*o)();
	union	testun p;
	enum	testenum q;
};

struct testbit {
	int	pad[5];
	int	a:31;
	long	b:2;
	int	c:30;
	int	d:1;
	short	e:5;
} bit;

char *testsub1() {}
void testsub2() {}
int testsub3(int a, int b) {}
struct testarray *testsub4() {}
struct testarray **testsub5() {}
int **testsub6(char *p, struct testbit *bitp) {}
void *testsub7() {}

/*
 * Array for converting between ELF and COFF basic types.
 */

int elf_to_coff_type[] = {
	-1,		/* FT_none */
	T_CHAR,		/* FT_char */
	T_CHAR,		/* FT_signed_char */
	T_UCHAR,	/* FT_unsigned_char */
	T_SHORT,	/* FT_short */
	T_SHORT,	/* FT_signed_short */
	T_USHORT,	/* FT_unsigned_short */
	T_INT,		/* FT_integer */
	T_INT,		/* FT_signed_integer */
	T_UINT,		/* FT_unsigned_integer */
	T_LONG,		/* FT_long */
	T_LONG,		/* FT_signed_long */
	T_ULONG,	/* FT_unsigned_long */
	(DT_PTR << N_BTSHFT) | T_NULL,		/* FT_pointer */
	T_FLOAT,	/* FT_float */
	T_DOUBLE,	/* FT_dbl_prec_float */
	-1,		/* FT_ext_prec_float */
	-1,		/* FT_complex */
	-1,		/* FT_dbl_prec_complex */
	-1,		/* reserved */
	T_NULL,		/* FT_void */
	-1,		/* FT_boolean */
	-1,		/* FT_ext_prec_complex */
	-1,		/* FT_label */
	T_ARG,		/* FT_long_long */
	T_ARG,		/* FT_signed_long_long */
	T_ARG		/* FT_unsigned_long_long */
};

/*
 * Array indicating the size of each of the ELF basic types.
 */

int elf_element_size[] = {
	-1,			/* FT_none */
	sizeof(char),		/* FT_char */
	sizeof(char),		/* FT_signed_char */
	sizeof(unsigned char),	/* FT_unsigned_char */
	sizeof(short),		/* FT_short */
	sizeof(short),		/* FT_signed_short */
	sizeof(unsigned short),	/* FT_unsigned_short */
	sizeof(int),		/* FT_integer */
	sizeof(int),		/* FT_signed_integer */
	sizeof(unsigned int),	/* FT_unsigned_integer */
	sizeof(long),		/* FT_long */
	sizeof(long),		/* FT_signed_long */
	sizeof(unsigned long),	/* FT_unsigned_long */
	sizeof(void *),		/* FT_pointer */
	sizeof(float),		/* FT_float */
	sizeof(double),		/* FT_dbl_prec_float */
	-1,			/* FT_ext_prec_float */
	-1,			/* FT_complex */
	-1,			/* FT_dbl_prec_complex */
	-1,			/* reserved */
	-1,			/* FT_void */
	-1,			/* FT_boolean */
	-1,			/* FT_ext_prec_complex */
	-1,			/* FT_label */
	sizeof(long long),	/* FT_long_long */
	sizeof(long long),	/* FT_signed_long_long */
	sizeof(long long)	/* FT_unsigned_long_long */
};

int verbose = 0;
char *object_name = NULL;
char *outfile_name = NULL;
int elf_fd, debug_index = -1, line_index = -1;

FILE *outf;
Elf *elf_file;
Elf32_Ehdr *elf_ehdr;
Elf_Scn *elf_scn = NULL;
Elf32_Shdr *elf_shdr;
Elf_Data *elf_debug_data;
Elf_Data *elf_line_data;

static		unsigned char	*data_ptr;
static		char *stor_name();
static		unsigned long	get_long(), peek_long();
static		unsigned short	get_short(), peek_short();
static		unsigned char	get_byte();
static		char	*get_string();

unsigned dump_structure(), dump_typedef(), dump_array(), dump_enum();
unsigned dump_subroutines();
unsigned mod_fund_type(), mod_u_d_type(), location(), subscr_data();
unsigned element_list();

void printout_line(), do_output(), do_basic_type(), do_derived_type();
void dump_data(), do_dims(), do_tabs();
void set_data_ptr();

static char *	lookuptag();
static char *	fund_type();
static int      has_arg();

main(argc, argv)
char **argv;
{
	unsigned offset = 0;

	parse_args(argc, argv);

	if ((outf = fopen(outfile_name, "w")) == NULL) {
		fprintf(stderr, "Error %d opening output file %s\n", errno, outfile_name);
		exit(1);
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		fprintf(stderr, "ELF version error\n");
		exit(1);
	}

	if ((elf_fd = open(object_name, O_RDONLY)) < 0) {
		fprintf(stderr, "Error %d opening object file %s\n",
			errno, object_name);
		exit(1);
	}

	elf_file = elf_begin(elf_fd, ELF_C_READ, (Elf *)0);

	elf_ehdr = elf32_getehdr(elf_file);
	debug_index = find_section(elf_file, ".debug");

	if (debug_index < 0) {
		fprintf(stderr, "Cannot find .debug section\n");
		exit(1);
	}

	if ((elf_debug_data = elf_getdata(elf_scn, NULL)) == NULL) {
		fprintf(stderr, "Cannot get ELF data\n");
		exit(1);
	}

	dump_data(elf_debug_data, offset);

	do_output();
}

void
set_data()
{}

void
set_text()
{}

void
do_output()
{
	int vr = IDF_VERSION;

	fwrite(IDF_MAGIC, IDF_MAGICL, 1, outf);
	fwrite(&vr, sizeof vr, 1, outf);
	d_stuns(outf);
	d_varis(outf);
	exit(0);
}

/* 
 * Find the section named by the "name" argument.  Returns the index number
 * of the section.
 */

find_section(Elf *elf_file, char *name)
{
	int i;
	char *secname;

	i = 1;
	elf_scn = NULL;

	while ((elf_scn = elf_getscn(elf_file, i)) != NULL) {
		elf_shdr = elf32_getshdr(elf_scn);
		secname = elf_strptr(elf_file, elf_ehdr->e_shstrndx, elf_shdr->sh_name);
		if (!strcmp(secname, name))
			return(i);
		i++;
	}
	return(-1);
}

void
dump_data(Elf_Data *data, unsigned offset)
{
	size_t debug_size;
	int length;
	short tag;

	debug_size = data->d_size;

	set_data_ptr(data->d_buf);

	while (offset < debug_size) {
		length = peek_long(offset);
		if (length > 4) {
			tag = peek_short(offset+4);

			switch (tag) {
			case TAG_structure_type:
			case TAG_union_type:
				offset = dump_structure(offset, 0, NULL);
				break;
			case TAG_global_variable:
				offset = dump_var(offset);
				break;
			case TAG_global_subroutine:
				offset = dump_subroutines(offset, NULL);
				break;
#ifdef NEVER
			case TAG_typedef:
				offset = dump_typedef(offset);
				break;
			case TAG_array_type:
				offset = dump_array(offset, NULL);
				break;
			case TAG_enumeration_type:
				offset = dump_enum(offset, 0);
				break;
			case TAG_subroutine_type:
			case TAG_subroutine:
			case TAG_global_subroutine:
				offset = dump_subroutines(offset, NULL);
				break;
			case TAG_local_variable:
				offset = dump_var(offset);
				break;
#endif
			default:
				/* fprintf(stderr, "*** %s at %x\n", lookuptag(tag), offset); */
				offset += length;
				break;
			}
		} else
			offset += 4;
	}
}

usage(char *name)
{
	fprintf(stderr, "Usage: %s [ -v ] -o <output file> <object>\n", name);
	exit(1);
}


#define FORMAT 1
#define NO_FORMAT 0

/*
 * Process debugging information by reading in set numbers of
 * bytes.  Since the debugging information is not contained
 * in structures, each set of bytes provides information for the
 * interpretation and size of the next set of bytes.
 * 
 * Debugging information is organized into records.  The first 4
 * bytes of a record provide the total size of that record.  The
 * next two bytes contain a tag value.  The next two bytes contain
 * an attribute value.  Each attribute has an associated data format
 * and type of information.  The number of bytes to be read following
 * the attribute information is determined by the data format and
 * can be seen by looking through the case statement of attributes
 * (AT_...).  The relevation functions are called for each attribute
 * listed and a not_interp function is provided which gives
 * uninterpreted numerical output for those attributes not in
 * the case statement.  Some attributes may not have been implemented
 * and it is easy to determine them since the only function called
 * is not_interp.
 */

unsigned
dump_structure(unsigned offset, int level, struct lines *linep)
{
	int is_struct, fund, length, sibling, member_sibling;
	int pointers, byte_size, bit_size, bit_offset;
	short tag, attr;
	unsigned ref, stroffset, new_offset, struct_size;
	char *name = NULL;
	char *struct_name = NULL;
	struct lines line;

	length = get_long(&offset);

	new_offset = offset + length - 4;

	if (length < 8)
		return(0);

	struct_size = 0;
	tag = get_short(&offset);

	switch (tag) {
	case TAG_structure_type:
		is_struct = 1;
		break;
	case TAG_union_type:
		is_struct = 0;
		break;
	default:
		fprintf(stderr, "dump_structure: not a structure/union - %x\n", offset);
		return(new_offset);
	}

	while (offset < new_offset) {
		attr = get_short(&offset);

		switch (attr) {
		case AT_name:
			if (struct_name == NULL)
				struct_name = get_string(&offset);
			else
				fprintf(stderr, "dump_structure: double AT_name entries - %x\n", offset);
			break;
		case AT_byte_size:
			struct_size = get_long(&offset);
			break;
		case AT_sibling:
			sibling = get_long(&offset);
			break;
		default:
			fprintf(stderr, "dump_structure: unknown attribute %x\n",
				attr);
			break;
		}
	}

	if (level == 1) {
		strcpy(linep->l_tag, struct_name);
		if (!(linep->l_flags & LINEF_SIZE))	/* extraneous? */
			linep->l_size = struct_size;
		if (is_struct)
			linep->l_type |= T_STRUCT;
		else
			linep->l_type |= T_UNION;
		linep->l_flags |= (LINEF_TAG | LINEF_SIZE | LINEF_TYPE);
		return(offset);
	}

	set_text();

	strcpy(line.l_name, struct_name);
	if (is_struct) {
		line.l_sclass = C_STRTAG;
		line.l_type = T_STRUCT;
	} else {
		line.l_sclass = C_UNTAG;
		line.l_type = T_UNION;
	}
	line.l_size = struct_size;
	line.l_flags = LINEF_NAME | LINEF_SCLASS | LINEF_TYPE | LINEF_SIZE;
	printout_line(&line);

	while (offset < sibling) {
		length = get_long(&offset);

		if (length <= 4)
			break;
		new_offset = offset + length - 4;

		tag = get_short(&offset);

		if (tag != TAG_member) {
			fprintf(stderr, "dump_structure: encountered non-member tag %x at offset %x\n", tag, offset);
			break;
		}

		name = NULL;
		ref = 0;
		pointers = 0;
		fund = FT_none;
		bit_size = 0;
		byte_size = 0;

		while (offset < new_offset) {

			attr = get_short(&offset);
			switch (attr) {
			case AT_sibling:
				member_sibling = get_long(&offset);
				break;
			case AT_name:
				if (name == NULL)
					name = get_string(&offset);
				break;
			case AT_fund_type:
				fund = get_short(&offset);
				break;
			case AT_user_def_type:
				ref = get_long(&offset);
				break;
			case AT_mod_fund_type:
				offset = mod_fund_type(offset, &fund, &pointers);
				break;
			case AT_mod_u_d_type:
				offset = mod_u_d_type(offset, &ref, &pointers);
				break;
			case AT_location:
				offset = location(offset, &stroffset);
				break;
			case AT_byte_size:
				byte_size = get_long(&offset);
				break;
			case AT_bit_size:
				bit_size = get_long(&offset);
				break;
			case AT_bit_offset:
				bit_offset = get_short(&offset);
				break;
			default:
				fprintf(stderr, "dump_subroutine: unknown attribute %x at offset %x\n", attr, offset);
				break;
			}
		}

		memset(&line, 0, sizeof(line));

		if (bit_size) {
			line.l_sclass = C_FIELD;
			line.l_size = bit_size;
			line.l_offset = (stroffset * 8) +
					(byte_size * 8 - (bit_offset + bit_size));
			line.l_flags = LINEF_SCLASS | LINEF_SIZE | LINEF_OFFSET;
		} else {
			if (is_struct)
				line.l_sclass = C_MOS;
			else
				line.l_sclass = C_MOU;

			line.l_offset = stroffset;
			line.l_flags = LINEF_SCLASS | LINEF_OFFSET;
		}

		strcpy(line.l_name, name);
		line.l_flags |= LINEF_NAME;
		line.l_type = 0;

		do_derived_type(&line, DT_PTR, pointers);

		if (fund != FT_none)
			do_basic_type(&line, fund);
		else {
			if (ref)
				follow_ref(&line, ref);
		}

		printout_line(&line);
	}

	strcpy(line.l_name, ".eos");
	line.l_offset = struct_size;
	line.l_sclass = C_EOS;
	strcpy(line.l_tag, struct_name);
	line.l_size = struct_size;
	line.l_flags = LINEF_NAME | LINEF_OFFSET | LINEF_SCLASS | LINEF_TAG |
			LINEF_SIZE;

	printout_line(&line);

	return(offset);
}

void
printout_line(struct lines *line)
{
	static struct stun *s = 0;
	extern struct stun *start_stun();
	static int state = 0;

	if (!(line->l_flags & LINEF_NAME))
		line->l_name[0] = '\0';
	if (!(line->l_flags & LINEF_OFFSET))
		line->l_offset = 0;
	if (!(line->l_flags & LINEF_SCLASS))
		line->l_sclass = 0;
	if (!(line->l_flags & LINEF_TYPE))
		line->l_type = 0;
	if (!(line->l_flags & LINEF_TAG))
		line->l_tag[0] = '\0';
	if (!(line->l_flags & LINEF_DIM)) {
		int i;

		for (i = 0 ; i < NDIM ; i++)
			line->l_dim[i] = 0;
	}

	if (!(line->l_flags & LINEF_SIZE))
		line->l_size = 0;
		
	if (verbose) {
		int i, open_bracket;

		fprintf(stderr, "%-15s 0x%.2x %4d %4d  0x%.2x ",
			line->l_name, line->l_sclass & 0xff, line->l_offset,
			line->l_size, line->l_type);

		open_bracket = 0;
		for (i = 0 ; i < NDIM ; i++) {
			if (line->l_dim[i]) {
				if (!open_bracket) {
					fprintf(stderr, "[");
					open_bracket = 1;
				}
				if (i > 0)
					fprintf(stderr, ",");
				fprintf(stderr, "%d", line->l_dim[i]);
			} else
				break;
		}
		if (open_bracket)
			fprintf(stderr, "] ");

		fprintf(stderr, "%s\n", line->l_tag);
	}

	if (state == 1) {
		if (line->l_sclass == C_EOS) {
			s = 0;
			state = 0;
		} else
		if (s)
			stel(s, &line->line);
	} else
	if (line->l_sclass == C_STRTAG || line->l_sclass == C_UNTAG) {
		if (line->l_name[0] != '.')	/* not a fake */
			s = start_stun(&line->line, 0);
		state = 1;
	} else
	if (line->l_sclass == C_EXT) {
		vari(&line->line);
	}
}


unsigned
dump_typedef(unsigned offset)
{
	char *typedef_name = NULL;
	int fund, pointers, sibling, length;
	unsigned ref, new_offset;
	short tag, attr;

	length = get_long(&offset);

	new_offset = offset + length - 4;

	if (length < 8)
		return(0);

	tag = get_short(&offset);

	if (tag != TAG_typedef) {
		fprintf(stderr, "dump_typedef: not a typedef - %x\n", offset);
		return(new_offset);
	}

	typedef_name = NULL;
	ref = 0;
	pointers = 0;
	fund = FT_none;

	printf("typedef ");

	while (offset < new_offset) {
		attr = get_short(&offset);

		switch (attr) {
		case AT_sibling:
			sibling = get_long(&offset);
			break;
		case AT_name:
			if (typedef_name == NULL)
				typedef_name = get_string(&offset);
			break;
		case AT_fund_type:
			fund = get_short(&offset);
			printf(" %s ", fund_type(fund));
			break;
		case AT_user_def_type:
			ref = get_long(&offset);
			break;
		case AT_mod_fund_type:
			offset = mod_fund_type(offset, &fund, &pointers);
			break;
		case AT_mod_u_d_type:
			offset = mod_u_d_type(offset, &ref, &pointers);
			break;
		default:
			break;
		}
/*
		if (ref && print_ref(ref, typedef_name))
			return(offset);
*/
	}
	while (pointers-- > 0)
		printf("*");
	
	if (typedef_name != NULL)
		printf("%s;\n", typedef_name);

	return(offset);
}

unsigned
dump_array(unsigned offset, struct lines *linep)
{
	int size, i, fund, pointers, sibling, length;
	unsigned ref, new_offset;
	short tag, attr, ordering;
	int dims[MAXDIMS];

	length = get_long(&offset);

	new_offset = offset + length - 4;
	size = -1;

	if (length < 8)
		return(0);

	tag = get_short(&offset);

	if (tag != TAG_array_type) {
		fprintf(stderr, "dump_array: not an array - %x\n", offset);
		return(new_offset);
	}

	ref = 0;
	pointers = 0;
	fund = FT_none;
	for (i = 0 ; i < MAXDIMS ; i++)
		dims[i] = 0;

	while (offset < new_offset) {
		attr = get_short(&offset);

		switch (attr) {
		case AT_sibling:
			sibling = get_long(&offset);
			break;
		case AT_ordering:
			ordering = get_short(&offset);
			break;
		case AT_subscr_data:
			offset = subscr_data(offset, &ref, &fund, &pointers, &dims[0]);
			break;
		default:
			break;
		}
	}

	if (fund != FT_none)
		do_basic_type(linep, fund);

	do_dims(dims, linep);

	if (ref)
		follow_ref(linep, ref);

	/*
	 * Calculate the size of the array.
	 */

	if (pointers)
		size = sizeof(void *);
	else
	if (ref && linep->l_flags & LINEF_SIZE)
		size = linep->l_size;
	else
		size = elf_element_size[fund];
	
	if (size == -1)
		fprintf(stderr, "dump_array: unknown size\n");
	else {
		i = 0;
		while (i < MAXDIMS && dims[i] != 0) {
			size *= dims[i];
			i++;
		}

		linep->l_size = size;
		linep->l_flags |= LINEF_SIZE;
	}

	do_derived_type(linep, DT_PTR, pointers);

	return(offset);
}

void
do_dims(int *dims, struct lines *linep)
{
	int i;

	i = 0;
	while (i < NDIM && dims[i]) {
		linep->l_dim[i] = dims[i];
		i++;
	}

	if (i < NDIM)
		linep->l_dim[i] = 0;
	
	linep->l_flags |= LINEF_DIM;

	do_derived_type(linep, DT_ARY, i);
}

void
do_derived_type(struct lines *linep, unsigned dtype, int count)
{
	int free_dtypes, max_dtypes;
	unsigned mask;

	if (count == 0)
		return;

	/*
	 * Find first free DT_* slot.
	 * Fill in "count" derived types.
	 */

	linep->l_flags |= LINEF_TYPE;

	mask = N_TMASK;			   /* mask of first derived type */
	max_dtypes = (32 - N_BTSHFT) / 2; /* maximum number of derived types */
	free_dtypes = max_dtypes;

	while (free_dtypes > 0) {
		if ((linep->l_type & mask) == DT_NON)
			break;
		mask <<= N_TSHIFT;
		free_dtypes--;
	}

	if (free_dtypes < count) {
		fprintf(stderr, "do_derived_type: no space for %d derived types\n", count);
	} else {
		mask = dtype << (N_BTSHFT +
				 (max_dtypes - free_dtypes) * N_TSHIFT);
		while (count-- > 0) {
			linep->l_type |= mask;
			mask <<= N_TSHIFT;
		}
	}
}

unsigned
dump_enum(unsigned offset, struct lines *linep)
{
	short tag, attr;
	int i, size, length, sibling;
	unsigned new_offset;
	char *name = NULL;
	char *enum_list[MAXENUM];

	for (i = 0 ; i < MAXENUM ; i++)
		enum_list[i] = NULL;

	length = get_long(&offset);

	new_offset = offset + length - 4;

	if (length < 8)
		return(offset);

	tag = get_short(&offset);

	if (tag != TAG_enumeration_type) {
		fprintf(stderr, "dump_enum: not an enumeration at offset %x\n", offset);
		return(new_offset);
	}

	while (offset < new_offset) {
		attr = get_short(&offset);

		if ((attr & ~FORM_MASK) == (AT_element_list & ~FORM_MASK)) {
			offset = element_list(offset, attr & FORM_MASK, enum_list);
		} else
		switch (attr) {
		case AT_sibling:
			sibling = get_long(&offset);
			break;
		case AT_name:
			if (name == NULL)
				name = get_string(&offset);
			break;
		case AT_byte_size:
			size = get_long(&offset);
			break;
		default:
			break;
		}
	}
	linep->l_type = T_ENUM;
	linep->l_size = sizeof(int);
	strcpy(linep->l_tag, name);
	linep->l_flags |= LINEF_TYPE | LINEF_TAG | LINEF_SIZE;

	return(offset);
}

unsigned
dump_subroutines(unsigned offset, struct lines *linep)
{
	char *name = NULL;
	char *sub_name;
	int fund, length, sibling, param_sibling, pointers;
	short tag, attr;
	unsigned value, ref, new_offset;
	struct lines line;
	int do_print;

	length = get_long(&offset);

	new_offset = offset + length - 4;

	if (length < 8)
		return(0);

	tag = get_short(&offset);

	if (tag != TAG_subroutine_type && tag != TAG_subroutine &&
	    tag != TAG_global_subroutine) {
		fprintf(stderr, "dump_subroutines: not a subroutine type - tag %x, offset %x\n", tag, offset);
		return(new_offset);
	}

	ref = 0;
	pointers = 0;
	fund = FT_none;

	while (offset < new_offset) {
		attr = get_short(&offset);

		switch (attr) {
		case AT_name:
			if (name == NULL)
				name = get_string(&offset);
			break;
		case AT_sibling:
			sibling = get_long(&offset);
			break;
		case AT_fund_type:
			fund = get_short(&offset);
			break;
		case AT_user_def_type:
			ref = get_long(&offset);
			break;
		case AT_mod_fund_type:
			offset = mod_fund_type(offset, &fund, &pointers);
			break;
		case AT_mod_u_d_type:
			offset = mod_u_d_type(offset, &ref, &pointers);
				break;
		case AT_high_pc:
		case AT_low_pc:
			value = get_long(&offset);
			break;
		case AT_location:
			offset = location(offset, &value);
			break;
		default:
			fprintf(stderr, "dump_subroutines: unknown attribute %x\n",
				attr);
			break;
		}
	}

	if (linep == NULL) {
		set_text();
		linep = &line;
		memset(linep, 0, sizeof(line));
		strcpy(linep->l_name, name);
		linep->l_sclass = C_EXT;
		linep->l_flags |= LINEF_NAME | LINEF_SCLASS;
		do_print = 1;
	} else
		do_print = 0;

	do_derived_type(linep, DT_FCN, 1);
	do_derived_type(linep, DT_PTR, pointers);

	if (fund != FT_none)
		do_basic_type(linep, fund);
	else {
		if (ref)
			follow_ref(linep, ref);
	}

		
	while (offset < sibling) {
		length = get_long(&offset);

		if (length <= 4)
			break;
		new_offset = offset + length - 4;

		tag = get_short(&offset);

		if (tag != TAG_formal_parameter &&
		    tag != TAG_unspecified_parameters) {
			offset -= 6;
			break;
		}

		ref = 0;
		pointers = 0;
		fund = FT_none;
		sub_name = NULL;

		while (offset < new_offset) {
			attr = get_short(&offset);

			switch (attr) {
			case AT_location:
				offset = location(offset, &value);
				break;
			case AT_name:
				sub_name = get_string(&offset);
				break;
			case AT_sibling:
				param_sibling = get_long(&offset);
				break;
			case AT_fund_type:
				fund = get_short(&offset);
				break;
			case AT_user_def_type:
				ref = get_long(&offset);
				break;
			case AT_mod_fund_type:
				offset = mod_fund_type(offset, &fund, &pointers);
				break;
			case AT_mod_u_d_type:
				offset = mod_u_d_type(offset, &ref, &pointers);
				break;
			default:
				fprintf(stderr, "dump_subroutines: unknown attr %x at offset %x\n", attr, offset);
				break;
			}
		}
	}

	if (do_print)
		printout_line(linep);

	return(offset);
}

dump_var(unsigned offset)
{
	char *name = NULL;
	int fund, pointers, sibling, length;
	unsigned value, ref, new_offset;
	short tag, attr;
	struct lines line;

	length = get_long(&offset);

	new_offset = offset + length - 4;

	if (length < 8)
		return(0);

	tag = get_short(&offset);

	if (tag != TAG_global_variable &&
	    tag != TAG_local_variable) {
		fprintf(stderr, "dump_var: not a var - %x\n", offset);
		return(-1);
	}

	ref = 0;
	pointers = 0;
	fund = FT_none;

	set_data();

	while (offset < new_offset) {
		attr = get_short(&offset);
		switch (attr) {
		case AT_sibling:
			sibling = get_long(&offset);
			break;
		case AT_name:
			if (name == NULL)
				name = get_string(&offset);
			break;
		case AT_fund_type:
			fund = get_short(&offset);
			break;
		case AT_user_def_type:
			ref = get_long(&offset);
			break;
		case AT_mod_fund_type:
			offset = mod_fund_type(offset, &fund, &pointers);
			break;
		case AT_mod_u_d_type:
			offset = mod_u_d_type(offset, &ref, &pointers);
			break;
		case AT_location:
			offset = location(offset, &value);
			break;
		default:
			fprintf(stderr, "dump_var: unknown attribute %x at offset %x\n", attr, offset);
			break;
		}
	}

	memset(&line, 0, sizeof(line));
	strcpy(line.l_name, name);
	line.l_sclass = C_EXT;
	line.l_flags = LINEF_NAME | LINEF_SCLASS;

	do_derived_type(&line, DT_PTR, pointers);

	if (fund != FT_none)
		do_basic_type(&line, fund);
	else {
		if (ref)
			follow_ref(&line, ref);
	}

	if (pointers) {
		line.l_size = sizeof(void *);
		line.l_flags |= LINEF_SIZE;
	}

	printout_line(&line);

	return(offset);
}

void
do_basic_type(struct lines *linep, int fund)
{
	int cofftype;

	if ((cofftype = elf_to_coff_type[fund]) == -1) {
		fprintf(stderr, "do_basic_type: unknown type %s\n",
			fund_type(fund));
		exit(1);
	}

	linep->l_type |= cofftype;
	linep->l_flags |= LINEF_TYPE;
}

void
do_tabs(int level)
{
	while (level-- > 0)
		printf("\t");
}

follow_ref(struct lines *linep, unsigned ref)
{
	int ret = 0;
	int len;
	short tag;

	len = peek_long(ref);
	tag = peek_short(ref+4);
	if (len > 4)
		switch (tag) {
		case TAG_structure_type:
		case TAG_union_type:
			dump_structure(ref, 1, linep);
			break;
		case TAG_array_type:
			dump_array(ref, linep);
			ret = 1;
			break;
		case TAG_subroutine_type:
			dump_subroutines(ref, linep);
			do_derived_type(linep, DT_FCN, 1);
			ret = 1;
			break;
		case TAG_enumeration_type:
			dump_enum(ref, linep);
			break;
		default:
			fprintf(stderr, "user_def(%x)\t", ref);
			break;
		}
	else 
		fprintf(stderr, "user_def(%x)\t", ref);
	return(ret);
}

/*
 * Returns a string name for the tag value.  This function
 * needs to be updated any time that a tag value is added or
 * changed.
 */

static char *
lookuptag( tag )
short tag;
{
	static char buf[16];

	switch ( tag ) {
	default:
		sprintf(buf, "0x%x", tag);
		return buf;
	case TAG_padding:		return "padding";
	case TAG_array_type:		return "array type";
	case TAG_class_type:		return "class type";
	case TAG_entry_point:		return "entry point";
	case TAG_enumeration_type:	return "enum type";
	case TAG_formal_parameter:	return "formal param";
	case TAG_global_subroutine:	return "global subrtn";
	case TAG_global_variable:	return "global var";
	case TAG_label:			return "label";
	case TAG_lexical_block:		return "lexical blk";
	case TAG_local_variable:	return "local var";
	case TAG_member:		return "member";
	case TAG_pointer_type:		return "pointer type";
	case TAG_reference_type:	return "ref type";
	case TAG_compile_unit:		return "compile unit";
	case TAG_string_type:		return "string type";
	case TAG_structure_type:	return "struct type";
	case TAG_subroutine:		return "subroutine";
	case TAG_subroutine_type:	return "subrtn type";
	case TAG_typedef:		return "typedef";
	case TAG_union_type:		return "union type";
	case TAG_unspecified_parameters:return "unspec parms";
	case TAG_variant:		return "variant";
	case TAG_common_block:		return "common block";
	case TAG_common_inclusion:	return "common inlude";
	case TAG_inheritance:		return "inheritance";
	case TAG_inlined_subroutine:	return "inlined subr";
	case TAG_module:		return "module";
	case TAG_ptr_to_member_type:	return "ptr to member";
	case TAG_set_type:		return "set type";
	case TAG_subrange_type:		return "subrange type";
	case TAG_with_stmt:		return "with stmt";
	}
}

/*
 * Set up the pointer used by the get_byte(), peek_short() etc. routines
 * below.
 */

void
set_data_ptr(unsigned char *p)
{
	data_ptr = p;
}

/*
 * Get 1 byte of data from the offset specified.  Update the offset.
 */

unsigned char 
get_byte(unsigned *offset)
{
	char 	*ptr;

	ptr = (char *)((unsigned)data_ptr + *offset);
	*offset += 1;
	return *ptr;
}

/*
 * Get 2 bytes of data from the offset specified.  Update the offset.
 */

unsigned short
get_short(unsigned *offset)
{
	short x;
	unsigned char    *p = (unsigned char *)&x;
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + *offset);

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        *offset += 2;
        return x;

}

/*
 * Get 2 bytes of data from the offset specified.  Do not update the offset.
 */

unsigned short
peek_short(unsigned offset)
{
	short x;
	unsigned char    *p = (unsigned char *)&x;
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + offset);

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        return x;

}

/*
 * Get 4 bytes of data from the offset specified.  Update the offset.
 */

unsigned long
get_long(unsigned *offset)
{
	long 	x;
	unsigned char	*p = (unsigned char *)&x;
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + *offset);

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        *offset += 4;
        return x;
}

/*
 * Get 4 bytes of data from the offset specified.  Do not update the offset.
 */

unsigned long
peek_long(unsigned offset)
{
	long 	x;
	unsigned char	*p = (unsigned char *)&x;
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + offset);

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        return x;
}

/*
 * Get a string of data from the offset specified.  Update the offset.
 */

char *
get_string(unsigned *offset)
{
	char *ptr;

	ptr = (char *)((unsigned)data_ptr + *offset);
	*offset += strlen(ptr) + 1;
	return ptr;
}

/*
 * The following functions are called to interpret the debugging
 * information depending on the attribute of the debugging entry.
 */

unsigned
mod_fund_type(unsigned offset, int *fund, int *pointers)
{
	short len2;
	int modcnt;
	int m;

	*pointers = 0;
	len2 = get_short(&offset);
	modcnt = len2 - 2;

	while(modcnt--) {
		m = get_byte(&offset);
		if (m == MOD_pointer_to) {
			(*pointers)++;
		}
	}

	*fund = (unsigned short) get_short(&offset);

	return(offset);
}

unsigned
mod_u_d_type(unsigned offset, unsigned *ref, int *pointers)
{
	short len2;
        int modcnt;
        int m;

	*pointers = 0;
	len2 = get_short(&offset);
	modcnt = len2 - 4;

	while(modcnt-- > 0)
	{
		m = get_byte(&offset);
		if (m == MOD_pointer_to) {
			(*pointers)++;
		}
	}

	*ref = get_long(&offset);
	return(offset);
}


unsigned
location(offset, value)
unsigned offset, *value;
{
	short           len2;
        int             o, a;

        len2 = get_short(&offset);

        while ( len2 > 0 )
        {
                o = get_byte(&offset);
		len2 -= 1;
		if( has_arg(o) )
		{
			a = get_long(&offset);
			len2 -= 4;

			/*
			 * NOTE: primitive assumption
			 */

			if (o == OP_CONST || o == OP_ADDR)
				*value = a;
		}
        }
	return(offset);
}


static char *
fund_type( f )
short f;
{
	static char buf[20];

	switch ( f ) {
	default:
		sprintf(buf, "<FT_0x%x>", f);
		return buf;
	case FT_none:			return "(none)";
	case FT_char:			return "char";
	case FT_signed_char:		return "signed char";
	case FT_unsigned_char:		return "unsigned char";
	case FT_short:			return "short";
	case FT_signed_short:		return "signed short";
	case FT_unsigned_short:		return "unsigned short";
	case FT_integer:		return "int";
	case FT_signed_integer:		return "signed int";
	case FT_unsigned_integer:	return "unsigned int";
	case FT_long:			return "long";
	case FT_signed_long:		return "signed long";
	case FT_unsigned_long:		return "unsigned long";
	case FT_pointer:		return "void\t*";
	case FT_float:			return "float";
	case FT_dbl_prec_float:		return "dbl_prec_float";
	case FT_ext_prec_float:		return "ext_prec_float";
	case FT_complex:		return "complex";
	case FT_dbl_prec_complex:	return "dbl_prec_complex";
	case FT_void:			return "void";
	case FT_boolean:		return "boolean";
	case FT_ext_prec_complex:	return "ext_prec_complex";
	case FT_label:			return "label";
	}
}

static int 
has_arg( op )
char op;
{

	switch ( op ) {
	default:
		return 0;
	case OP_UNK:		return 0;
	case OP_REG:		return 1;
	case OP_BASEREG:	return 1;
	case OP_ADDR:		return 1;
	case OP_CONST:		return 1;
	case OP_DEREF2:		return 0;
	case OP_DEREF:		return 0;
	case OP_ADD:		return 0;
	}
}

unsigned
element_list(unsigned offset, int form, char *enum_list[])
{
	int i;
	long len;
	long  word;
	char *the_string;

	i = 0;
	if (form == FORM_BLOCK2)
		len = (long)get_short(&offset);
	else
		len = (long)get_long(&offset);

	while ( len > 0 )
	{
		word = get_long(&offset);
		len -= 4;
		the_string = get_string(&offset);
		len -= (strlen(the_string) + 1);
		enum_list[i++] = the_string;
	}
	return(offset);
}

unsigned
subscr_data(unsigned offset, unsigned *ref, int *fund, int *pointers, int *dims)
{
	char    fmt;
	short   et_name;
	short	size;
	int	word;
	unsigned	stroffset, final_offset;

	size = get_short(&offset);
	final_offset = offset + size;
	while (offset < final_offset)
	{
		fmt = get_byte(&offset);
		switch (fmt)
		{
			case FMT_FT_C_C:
				et_name = get_short(&offset);
				word = get_long(&offset);
				if (word != 0)
					fprintf(stderr, "subscr_data: non-zero lower bound %d at offset %x\n", word, offset);
				word = get_long(&offset);
				*dims++ = word + 1;
				break;
			case FMT_FT_C_X:
				fprintf(stderr, "subscr_data: FMT_FT_C_X at offset %x\n", offset);
				et_name = get_short(&offset);
				word = get_long(&offset);
				offset = location(offset, &stroffset);
				break;
			case FMT_FT_X_C:
				fprintf(stderr, "subscr_data: FMT_FT_X_C at offset %x\n", offset);
				et_name = get_short(&offset);
				offset = location(offset, &stroffset);
				word = get_long(&offset);
				break;
			case FMT_FT_X_X:
				fprintf(stderr, "subscr_data: FMT_FT_X_X at offset %x\n", offset);
				et_name = get_short(&offset);
				offset = location(offset, &stroffset);
				offset = location(offset, &stroffset);
				break;
			case FMT_UT_C_C:
				fprintf(stderr, "subscr_data: FMT_UT_C_C at offset %x\n", offset);
				word = get_long(&offset);
				word = get_long(&offset);
				word = get_long(&offset);
				break;
			case FMT_UT_C_X:
				fprintf(stderr, "subscr_data: FMT_UT_C_X at offset %x\n", offset);
				word = get_long(&offset);
				word = get_long(&offset);
				offset = location(offset, &stroffset);
				break;
			case FMT_UT_X_C:
				fprintf(stderr, "subscr_data: FMT_UT_X_C at offset %x\n", offset);
				word = get_long(&offset);
				offset = location(offset, &stroffset);
				word = get_long(&offset);
				break;
			case FMT_UT_X_X:
				fprintf(stderr, "subscr_data: FMT_UT_X_X at offset %x\n", offset);
				word = get_long(&offset);
				offset = location(offset, &stroffset);
				offset = location(offset, &stroffset);
				break;
			case FMT_ET:
				et_name = get_short(&offset);
				switch(et_name)
				{
					case AT_fund_type:
						*fund = get_short(&offset);
						break;
					case AT_mod_fund_type:
						offset = mod_fund_type(offset, fund, pointers);
						break;
					case AT_user_def_type:
						*ref = get_long(&offset);
						break;
					case AT_mod_u_d_type:
						offset = mod_u_d_type(offset, ref, pointers);
						break;
					default:
						(void)fprintf(stderr, "<unknown element type 0x%x>\n", et_name);
						break;
				}
				break;
			default:
				(void)fprintf(stderr, "<unknown format 0x%x>\n", et_name);
				final_offset = 0;
				break;
		}
	}	/* end while */
	return(offset);
}


parse_args(int argc, char **argv)
{
	int c;

	if (argc < 2)
		usage(argv[0]);

	while ((c = getopt(argc, argv, "vo:")) != EOF) {
		switch (c) {
		case 'v':
			verbose = 1;
			break;
		case 'o':
			outfile_name = optarg;
			break;
		}
	}

	if (outfile_name == NULL || argc - optind < 1)
		usage(argv[0]);

	object_name = argv[optind];
}

static struct stor {
	char	*name;
	int	sclass;
} stor[] = {
	"EFCN",          -1,
	"NULL",          0,
	"AUTO",          1,
	"EXT",           2,
	"STAT",          3,
	"REG",           4,
	"EXTDEF",        5,
	"LABEL",         6,
	"ULABEL",        7,
	"MOS",           8,
	"ARG",           9,
	"STRTAG",        10,
	"MOU",           11,
	"UNTAG",         12,
	"TPDEF",         13,
	"USTATIC", 14,
	"ENTAG",         15,
	"MOE",           16,
	"REGPARM", 17,
	"FIELD",         18,
	"BLOCK",         100,
	"FCN",           101,
	"EOS",           102,
	"FILE",          103,
	"LINE", 104,
	"ALIAS", 105,
	"HIDDEN", 106,
	"SHADOW", 107
};

int nstor = sizeof(stor) / sizeof(struct stor);

static char *
stor_name(int sclass)
{
	int i;

	for (i = 0 ; i < nstor ; i++)
		if (sclass == stor[i].sclass)
			return(stor[i].name);
	
	return("?");
}
