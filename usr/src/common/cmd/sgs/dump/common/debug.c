#ident	"@(#)dump:common/debug.c	1.28"

/* This file contains all of the functions necessary
 * to interpret debugging information and line number information.
 */

#include        <stdio.h>
#include        <malloc.h>

#include	"libelf.h"
#include        "sgs.h"
#include	"dwarf.h"
#include	"dump.h"
#include 	"ccstypes.h"

#define FORMAT 1
#define NO_FORMAT 0

static		unsigned char	*p_debug_data, *p_line_data, *ptr;
static 		long		length = 0;
static 		int		no_elemtype = 0;
static		long		current, nextoff;

static		long	get_long();
static		short	get_short();
static		unsigned char	get_byte();
static unsigned char	*get_string();
static		void	print_record(),
			not_interp(),
			user_def_type(),
			mod_fund_type(),
			location(),
			mod_u_d_type();
static unsigned	char	*the_string;

static char *	lookuptag();
static char *	lookupattr();
static char *	lookupfmt();
static char *	lang();
static char *	fund_type();
static char *	modifier();
static char *	op();
static int      has_arg();
static char *	order();
static void     element_list();
static void     subscr_data();
static void     line_info();
static void     print_line();
static void     friends();
static int	Offset=0;



/*
 * Get the debugging data and call print_record to process it.
 */

void
dump_debug(elf, filename)
Elf *elf;
char * filename;
{

	extern SCNTAB *p_debug, *p_line;
        Elf_Scn         *scn;
	size_t		dbg_size;

	if(!p_flag)
	{
		(void)printf("    ***** DEBUGGING INFORMATION *****\n");
		(void)printf("%-10s %-10s %-13s %-18s %s\n",
			"Offset",
			"Size",
			"Tag",
			"Attribute",
			"Value");
	}

	if ( (p_debug_data = (unsigned char *)get_scndata(p_debug->p_sd, &dbg_size)) == NULL)
	{
		(void)fprintf(stderr, "%s: %s: no data in %s section\n",
			prog_name, filename, p_debug->scn_name);
		return;
	}

	ptr = p_debug_data;
	print_record(p_debug->p_shdr->sh_offset, dbg_size, filename);

}


/*
 * Process debugging information by reading in set numbers of
 * bytes.  Since the debugging information is not contained
 * in structures, each set of bytes provides information for the
 * interpretation and size of the next set of bytes.
 * This code is machine dependent and will require updates to
 * any functions that read bytes (get_long, get_short, get_byte,
 * get_string) to be valid on machines with different byte ordering.
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

static void
print_record( offset, size, filename)
Elf32_Off	offset;
size_t		size;
char		*filename;
{
	long	word;
	short	sword;
	short 	tag;
	short	attrname;
	short   len2;
	unsigned char	attrform;

  current = 0;
  while ( current < size)
  { 
	(void)printf("\n");
	word = get_long();

	if (word > size) {
			extern int retVal;
		(void)fprintf(stderr, "%s : %s : bad debug entry\n", 
			prog_name, filename);
			retVal |= 0x62;
			return;
	}

	if ( word <= 8)
	{
		(void)printf("\n0x%-8lx 0x%-8lx\n", Offset, word);
		Offset += 4;
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

	length = word - 4;
	(void)printf("\n0x%-8lx 0x%-8lx ", Offset, word);
	Offset += word;
	nextoff = offset + word;

	tag = get_short();
	(void)printf("%-13s ", lookuptag(tag) );

	
	while(length > 0)
	{
		attrname = get_short();
		attrform = (unsigned char)(attrname & FORM_MASK);
		(void)printf("%-18s ", lookupattr(attrname) );
		switch( attrname )
		{
			case AT_padding:
				(void)printf("(FORM_NONE)\n");
                                break;
                        case AT_sibling:
                                word = get_long();
				if(word != 0)
					(void)printf("0x%lx\n", word);
				else
					(void)printf("\n");
				break;
			case AT_location:
			case AT_string_length:
			case AT_return_addr:
				location(NO_FORMAT);
                                break;
                        case AT_name:
			case AT_comp_dir:
			case AT_producer:
				the_string = get_string();
                                (void)printf("%s\n",the_string);
                                break;
                        case AT_fund_type:
                                sword = get_short();
				(void)printf("%s\n", fund_type(sword) );
				break;
                        case AT_mod_fund_type:
                                mod_fund_type();
				break;
                        case AT_user_def_type:
                                user_def_type();
				break;
                        case AT_mod_u_d_type:
                                mod_u_d_type();
				break;
			case AT_ordering:
				sword = get_short();
				(void)printf("%s\n", order(sword) );
				break;
			case AT_byte_size:
			case AT_bit_size:
			case AT_stride_size:
                                word = get_long();
				(void)printf("%d\n", word);
                                break;
                        case AT_bit_offset:
                                sword = get_short();
				(void)printf("%d\n", sword);
                                break;
                        case AT_stmt_list:
                        case AT_low_pc:
                        case AT_high_pc:
			case AT_start_scope:
			case AT_member:
			case AT_discr:
			case AT_common_reference:
			case AT_containing_type:
			case AT_specification:
                                word = get_long();
				(void)printf("0x%lx\n", word);
                                break;
                        case AT_language:
                                word = get_long();
				(void)printf("%s\n", lang(word) );
                                break;
                        case AT_subscr_data:
				subscr_data(); 
				break;
			case AT_inline:
			case AT_is_optional:
			case AT_private:
			case AT_program:
			case AT_protected:
			case AT_prototyped:
			case AT_pure_virtual:
			case AT_virtual:
				/* value not important - presence of attribute
				 * is key
				 */
				(void)get_string();
				printf("\n");
				break;
			case AT_friends:
				friends();
				break;
			case AT_discr_value:
			case AT_const_value:
			case AT_default_value:
			case AT_lower_bound:
			case AT_upper_bound:
				not_interp( attrform );
                                break;
                        default:
				if ((attrname & ~FORM_MASK) == 
					(AT_element_list & ~FORM_MASK))
					element_list(attrform);

				else
                                	not_interp( attrform );
                                break;
                }
		if(length)
			(void)printf("%-36s", " ");
	}
  }
  return;
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
 * Return a string name for an attribute value.  This function
 * needs to be updated any time that an attribute value is added
 * or changed
 */

static char *
lookupattr( attr )
short attr;
{
	static char buf[20];
	short	    attrname;

	/* element list used to be block2,now block 4 */
	if ((attr & ~FORM_MASK) == (AT_element_list & ~FORM_MASK))
		return("element_list");
	switch ( attr ) {
	case AT_padding:	return "padding";
	case AT_sibling:	return "sibling";
	case AT_location:	return "location";
	case AT_name:		return "name";
	case AT_fund_type:	return "fund_type";
	case AT_mod_fund_type:	return "mod_fund_type";
	case AT_user_def_type:	return "user_def_type";
	case AT_mod_u_d_type: 	return "mod_u_d_type";
	case AT_ordering:	return "ordering";
	case AT_subscr_data:	return "subscr_data";
	case AT_byte_size:	return "byte_size";
	case AT_bit_offset:	return "bit_offset";
	case AT_bit_size:	return "bit_size";
	case AT_stmt_list:	return "stmt_list";
	case AT_low_pc:		return "low_pc";
	case AT_high_pc:	return "high_pc";
	case AT_language:	return "language";
	case AT_member:		return "member";
	case AT_discr:		return "discr";
	case AT_discr_value:	return "discr_value";
	case AT_string_length:	return "string_length";
	case AT_common_reference:	return "common_reference";
	case AT_comp_dir:	return "comp_dir";
	case AT_containing_type:	return "containing_type";
	case AT_friends:	return "friends";
	case AT_inline:		return "inline";
	case AT_is_optional:	return "is_optional";
	case AT_private:	return "private";
	case AT_producer:	return "producer";
	case AT_program:	return "program";
	case AT_protected:	return "protected";
	case AT_prototyped:	return "prototyped";
	case AT_public:		return "public";
	case AT_pure_virtual:	return "pure_virtual";
	case AT_return_addr:	return "return_addr";
	case AT_specification:	return "specification";
	case AT_start_scope:	return "start_scope";
	case AT_stride_size:	return "stride_size";
	case AT_virtual:	return "virtual";
	default:
		/* some attribute take multiple forms - check here */
		attrname = attr & ~FORM_MASK;
		switch(attrname) {
			case AT_const_value:	return "const_value";
			case AT_default_value:	return "default_value";
			case AT_lower_bound:	return "lower_bound";
			case AT_upper_bound:	return "upper_bound";
		}
		sprintf(buf, "0x%x", attr);
		return buf;
	}
}


/*
 * Get 1 byte of data.  Decrement the length by 1 byte
 * and the no_elemtype by 1 byte.  The length is used
 * globally by all functions. no_elemtype is used globally
 * only by subscr_data.
 */

static unsigned char 
get_byte()
{
	unsigned char 	*p;

	p = ptr; 
	++ptr;
	length -= 1;
	no_elemtype -= 1;
	return *p;
}

/*
 * Get 2 bytes of data.  Decrement the length by 2 bytes
 * and the no_elemtype by 2 bytes.  The length is used
 * globally by all functions. no_elemtype is used globally
 * only by subscr_data.
 */

static short
get_short()
{
	short x;
	unsigned char    *p = (unsigned char *)&x;

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        length -= 2;
        no_elemtype -= 2;
        return x;

}

/*
 * Get 4 bytes of data.  Decrement the length by 4 bytes
 * and the no_elemtype by 4 bytes.  The length is used
 * globally by all functions. no_elemtype is used globally
 * only by subscr_data.
 */

static long
get_long()
{
	long 	x;
	unsigned char	*p = (unsigned char *)&x;

	*p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr; ++p;
        *p = *ptr; ++ptr;
        length -= 4;
        no_elemtype -= 4;
        return x;
}

/*
 * Get string_length bytes of data.  Decrement the length by
 * string_length bytes
 * and the no_elemtype by string_length bytes.  The length is used
 * globally by all functions. no_elemtype is used globally
 * only by subscr_data.
 */

static unsigned char *
get_string()
{
	unsigned char	*s;
	register 	int	len;
	
	len = strlen((char *)ptr) +1;
	s = (unsigned char *)malloc(len);
	memcpy(s,ptr,len);
	ptr += len;
	length -= len;
	no_elemtype -= len;
	return s;

}


/*
 * Print out the uninterpreted numerical value
 * of the associated attribute data depending on
 * the data format.
 */

static void
not_interp( attrform ) 
unsigned char attrform;
{
        short   len2;
        long    word;

        switch( attrform )
        {
                case FORM_NONE: break;
                case FORM_ADDR:
                case FORM_REF:  word = get_long();
				(void)printf("<0x%lx>\n", word);
				break;
                case FORM_BLOCK2:       len2 = get_short();
                                        length -= len2;
                                        ptr += len2;
					(void)printf("0x%x\n", len2);
					break;
                case FORM_BLOCK4:       word = get_long();
                                        length -= word;
                                        ptr += word;
					(void)printf("0x%lx\n", word);
					break;
                case FORM_DATA2:        len2 = get_short();
					(void)printf("0x%x\n", len2);
					break;
                case FORM_DATA8:        word = get_long();
					(void)printf("0x%lx ", word);
					break;
                case FORM_DATA4:        word = get_long();
					(void)printf("0x%lx\n", word);
					break;
                case FORM_STRING:       word = strlen((char *)ptr) + 1;
                                        length -= word;
					(void)printf("%s\n", ptr);
                                        ptr += word;
					break;
                default:
                        printf("<unknown form: 0x%x>\n", (attrform) );
			length = 0;
        }
}


/*
 * The following functions are called to interpret the debugging
 * information depending on the attribute of the debugging entry.
 */

static void
user_def_type()
{
	long	x;
	x = get_long();
	(void)printf("0x%lx\n", x);
}

static void
mod_fund_type()
{
	short len2, x;
	int modcnt;
	char *p;
	int i;

	len2 = get_short();
	modcnt = len2 - 2;
	while(modcnt--)
	{
		(void)printf("%s\n", modifier(*ptr++) );
		(void)printf("%-55s", " ");
		length--; no_elemtype--;
	}
	x = get_short();
	(void)printf("%s\n",fund_type(x) );
	
}

static void
mod_u_d_type()
{
	short len2;
        int modcnt;
        char *p;
        int i;
	long	x;

	len2 = get_short();
	modcnt = len2 - 4;
	while(modcnt--)
	{
		(void)printf("%s ", modifier(*ptr++) );
		length--; no_elemtype--;
	}
/*	x = p_debug->p_shdr->sh_offset + get_long() - p_debug->p_shdr->sh_addr; */
	x = get_long();
	(void)printf("\n%-55s0x%lx\n", " ",  x);
	
}

static void
location(fmt)
int fmt;
{
	short           len2;
        unsigned int	o;
	int		a;
	long		x;

        len2 = get_short();
	if(len2 && fmt)
		(void)printf("%-55s", " ");
        while ( len2 > 0 )
        {
                o = get_byte();
		len2 -= 1;
		(void)printf("%s ", op(o) );
		switch (has_arg(o))
		{
		case 2:
			a = get_long();
			len2 -= 4;
			(void)printf("0x%x ", a);
			/*FALL-THROUGH*/
		case 1:
			a = get_long();
			len2 -= 4;
			(void)printf("0x%x\n", a);
			break;
		case 0:
			break;
		}
		if(len2)
			(void)printf("%-55s", " ");
        }
}


static char *
lang( l )
long l;
{
	static char buf[20];

	switch ( l ) {
	default:
		sprintf(buf, "<LANG_0x%lx>", l);
		return buf;
	case LANG_UNK:			return "LANG_UNK";
	case LANG_C89:			return "LANG_C89";
	case LANG_C:			return "LANG_C";
	case LANG_ADA83:		return "LANG_ADA83";
	case LANG_C_PLUS_PLUS:		return "LANG_C_PLUS_PLUS";
	case LANG_COBOL74:		return "LANG_COBOL74";
	case LANG_COBOL85:		return "LANG_COBOL85";
	case LANG_FORTRAN77:		return "LANG_FORTRAN77";
	case LANG_FORTRAN90:		return "LANG_FORTRAN90";
	case LANG_PASCAL83:		return "LANG_PASCAL83";
	case LANG_MODULA2:		return "LANG_MODULA2";
	}
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
	case FT_none:			return "FT_none";
	case FT_char:			return "FT_char";
	case FT_signed_char:		return "FT_unsigned_char";
	case FT_unsigned_char:		return "FT_unsigned_char";
	case FT_short:			return "FT_short";
	case FT_signed_short:		return "FT_signed_short";
	case FT_unsigned_short:		return "FT_unsigned_short";
	case FT_integer:		return "FT_integer";
	case FT_signed_integer:		return "FT_signed_integer";
	case FT_unsigned_integer:	return "FT_unsigned_integer";
	case FT_long:			return "FT_long";
	case FT_signed_long:		return "FT_signed_long";
	case FT_unsigned_long:		return "FT_unsigned_long";
	case FT_pointer:		return "FT_pointer";
	case FT_float:			return "FT_float";
	case FT_dbl_prec_float:		return "FT_dbl_prec_float";
	case FT_ext_prec_float:		return "FT_ext_prec_float";
	case FT_complex:		return "FT_complex";
	case FT_dbl_prec_complex:	return "FT_dbl_prec_complex";
	case FT_void:			return "FT_void";
	case FT_boolean:		return "FT_boolean";
	case FT_ext_prec_complex:	return "FT_ext_prec_complex";
	case FT_label:			return "FT_label";
#if LONG_LONG
	case FT_long_long:			return "FT_long_long";
	case FT_signed_long_long:		return "FT_signed_long_long";
	case FT_unsigned_long_long:		return "FT_unsigned_long_long";
#endif
	}
}

static
char *modifier( m )
char m;
{
	static char buf[20];

	switch ( m ) {
	default:
		sprintf(buf, "<MOD_0x%x>", m);
		return buf;
	case MOD_none:		return "MOD_none";
	case MOD_pointer_to:	return "MOD_pointer_to";
	case MOD_reference_to:	return "MOD_reference_to";
	case MOD_const:		return "MOD_const";
	case MOD_volatile:	return "MOD_volatile";
	}
}

static char *
op( a )
unsigned char a;
{
	static char buf[20];

	switch ( a ) {
	default:
		sprintf(buf, "<OP_0x%x>", a);
		return buf;
	case OP_UNK:		return "OP_UNK";
	case OP_REG:		return "OP_REG";
	case OP_BASEREG:	return "OP_BASEREG";
	case OP_ADDR:		return "OP_ADDR";
	case OP_CONST:		return "OP_CONST";
	case OP_DEREF2:		return "OP_DEREF2";
	case OP_DEREF:		return "OP_DEREF";
	case OP_ADD:		return "OP_ADD";
	case OP_SCO_REG_PAIR:	return "OP_SCO_REG_PAIR";
	}
}

static int 
has_arg( op )
unsigned char op;
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
	case OP_SCO_REG_PAIR:	return 2;
	}
}

static char *
order(a)
short a;
{
	static char buf[20];

	switch (a)
	{
		case ORD_row_major:	return "ORD_row_major";
		case ORD_col_major:	return "ORD_col_major";
		default:		sprintf(buf, "<OP_0x%x>", a);
			 		return buf;
	}
}

static void
element_list(form)
unsigned char form;
{
	long len;
	long  word;
	unsigned char *the_string;

	if (form == FORM_BLOCK2)
		len = (long)get_short();
	else
		len = (long)get_long();

	while ( len > 0 )
	{
		word = get_long();
		len -= 4;
		len -= (strlen((char *)ptr) + 1);
		the_string = get_string();
		(void)printf("0x%lx %s\n", word, the_string);

		if(len)
			(void)printf("%-55s", " ");
	}
}

static void
subscr_data()
{
	static char buf[20];
	unsigned char    fmt;
	short   et_name;
	short	sword;
	long	word;

	no_elemtype = get_short();
	while (no_elemtype)
	{
		fmt = get_byte();
		(void)printf("%s\n", lookupfmt(fmt) );
		switch (fmt)
		{
			case FMT_FT_C_C:
				et_name = get_short();
				(void)printf("%-55s", " ");
				(void)printf("%s\n", fund_type(et_name) );
				word = get_long();
				(void)printf("%-55s", " ");
				(void)printf("lobound = 0x%lx\n", word);
				word = get_long();
				(void)printf("%-55s", " ");
				(void)printf("hibound = 0x%lx\n", word);
				break;
			case FMT_FT_C_X:
				et_name = get_short();
				(void)printf("%-55s", " ");
				(void)printf("%s\n", fund_type(et_name) );
				word = get_long();
				(void)printf("%-55s", " ");
				(void)printf("lobound = 0x%lx\n", word);
				location(FORMAT);
				break;
			case FMT_FT_X_C:
				et_name = get_short();
				(void)printf("%-55s", " ");
				(void)printf("%s\n", fund_type(et_name) );
				location(FORMAT);
				word = get_long();
				(void)printf("%-55s", " ");
				(void)printf("hibound = 0x%lx\n", word);
				break;
			case FMT_FT_X_X:
				et_name = get_short();
				(void)printf("%-55s", " ");
				(void)printf("%s\n", fund_type(et_name) );
				location(FORMAT);
				location(FORMAT);
				break;
			case FMT_UT_C_C:
				(void)printf("%-55s", " ");
				user_def_type();
				word = get_long();
				(void)printf("%-55s", " ");
				(void)printf("lobound = 0x%lx\n", word);
				word = get_long();
				(void)printf("%-55s", " ");
				(void)printf("hibound = 0x%lx\n", word);
				break;
			case FMT_UT_C_X:
				(void)printf("%-55s", " ");
				user_def_type();
				word = get_long();
				(void)printf("%-55s", " ");
				(void)printf("lobound = 0x%lx\n", word);
				location(FORMAT);
				break;
			case FMT_UT_X_C:
				(void)printf("%-55s", " ");
				user_def_type();
				location(FORMAT);
				word = get_long();
				(void)printf("%-55s", " ");
				(void)printf("hibound = 0x%lx\n", word);
				break;
			case FMT_UT_X_X:
				(void)printf("%-55s", " ");
				user_def_type();
				location(FORMAT);
				location(FORMAT);
				break;
			case FMT_ET:
				et_name = get_short();
				switch(et_name)
				{
					case AT_fund_type:
						sword = get_short();
						(void)printf("%-55s", " ");
						(void)printf("%s\n", fund_type(sword) );
						break;
					case AT_mod_fund_type:
						(void)printf("%-55s", " ");
						mod_fund_type();
						break;
					case AT_user_def_type:
						(void)printf("%-55s", " ");
						user_def_type();
						break;
					case AT_mod_u_d_type:
						(void)printf("%-55s", " ");
						mod_u_d_type();
						break;
					default:
						(void)printf("%-55", " ");
						(void)printf("<unknown element type 0x%x>\n", et_name);
						break;
				}
				break;
			default:
				(void)printf("%-55s", " ");
				(void)printf("<unknown format 0x%x>\n", et_name);
				no_elemtype = 0;
				break;
		}
		if(no_elemtype)
			(void)printf("%-55s", " ");
	}	/* end while */
}


static char *
lookupfmt( fmt )
unsigned char fmt;
{
	static char buf[10];

	switch ( fmt )
	{
		default:
			sprintf(buf, "0x%x", fmt);
			return buf;
		case FMT_FT_C_C:	return "FMT_FT_C_C";
		case FMT_FT_C_X:	return "FMT_FT_C_X";
		case FMT_FT_X_C:	return "FMT_FT_X_C";
		case FMT_FT_X_X:	return "FMT_FT_X_X";
		case FMT_UT_C_C: 	return "FMT_UT_C_C";
		case FMT_UT_C_X:	return "FMT_UT_C_X";
		case FMT_UT_X_C:	return "FMT_UT_X_C";
		case FMT_UT_X_X:	return "FMT_UT_X_X";
		case FMT_ET:		return "FMT_ET";
	}
}


/*
 * Print line number information.  Get the line number data
 * and call print_line to print it out.  Input is an ELF file
 * descriptor and the filename.
 */

void
dump_line(elf, filename)
Elf *elf;
char * filename;
{

	extern SCNTAB *p_line;
        Elf_Scn         *scn;
	size_t		l_size;

	if(!p_flag)
	{
		(void)printf("    ***** LINE NUMBER INFORMATION *****\n");
		(void)printf("%-12s%-12s%s\n",
			"LineNo",
			"LinePos",
			"Pcval");
	}

	if ( (p_line_data = (unsigned char *)get_scndata(p_line->p_sd, &l_size)) == NULL)
	{
		(void)fprintf(stderr, "%s: %s: no data in %s section\n",
			prog_name, filename, p_line->scn_name);
		return;
	}

	ptr = p_line_data;
	print_line(p_line->p_shdr->sh_offset, l_size, filename);
}


/*
 * Print line number information.  Input is section header offset of
 * the line number section, the size, and the filename.  The first 4
 * bytes contain the length of the line number information for the first
 * source file linked in if the file is an executable, and the total
 * size if the file is a relocatable object.  The size is the size of
 * the entire section.  Print out line information until length is 0.
 * If size > 0, read in the next 4 bytes for the length of the next 
 * part of the line number information.  There will be one sub-section
 * for each file that was linked together, including library files.
 */

static void
print_line(off, size, filename)
long off;
long size;
char *filename;
{
	long  line;
	long  pcval;
	long  base_address;
	long delta;
	short stmt;
	extern int retVal;
	
	while (size > 0)
	{
		length = get_long();
		length -= 4;
		size -= 4;
		base_address = get_long();
		size -= 4;
	
		if(size < length-4)
		{
			(void)fprintf(stderr, "%s: %s: bad line info section -  size=%ld length-4=%ld\n", prog_name, filename, size, length-4);
			retVal |= 0x28;
			return;
		}
		while(length > 0)
		{
			line = get_long();
			size -= 4;
			stmt = get_short();
			size -= 2;
			delta = get_long();
			size -= 4;
			pcval = base_address + delta;
	
			(void)printf("%-12ld%-12d0x%lx\n",
				line,
				stmt,
				pcval);
		}
	}
}

static void
friends()
{
	short	len;
	long	word;

	len = get_short();
	while ( len > 0 )
	{
		word = get_long();
		len -= 4;
		(void)printf("0x%lx\n", word);

		if(len)
			(void)printf("%-48s", " ");
	}
}
