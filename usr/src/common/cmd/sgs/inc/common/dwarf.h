#ident	"@(#)sgs-inc:common/dwarf.h	1.8"

#ifndef DWARF_H
#define DWARF_H

/* dwarf.h - manifest constants used in the .debug section of ELF files */


/* the "tag" - the first short of any legal record */

#define	TAG_padding			0x0000
#define	TAG_array_type			0x0001
#define	TAG_class_type			0x0002
#define	TAG_entry_point			0x0003
#define	TAG_enumeration_type		0x0004
#define	TAG_formal_parameter		0x0005
#define	TAG_global_subroutine		0x0006
#define	TAG_global_variable		0x0007
#define	TAG_label			0x000a
#define	TAG_lexical_block		0x000b
#define	TAG_local_variable		0x000c
#define	TAG_member			0x000d
#define	TAG_pointer_type		0x000f
#define	TAG_reference_type		0x0010
#define	TAG_compile_unit		0x0011
#define	TAG_string_type			0x0012
#define	TAG_structure_type		0x0013
#define	TAG_subroutine			0x0014
#define	TAG_subroutine_type		0x0015
#define TAG_typedef			0x0016
#define	TAG_union_type			0x0017
#define	TAG_unspecified_parameters	0x0018
#define	TAG_variant			0x0019
#define	TAG_common_block		0x001a
#define	TAG_common_inclusion		0x001b
#define	TAG_inheritance			0x001c
#define	TAG_inlined_subroutine		0x001d
#define	TAG_module			0x001e
#define	TAG_ptr_to_member_type		0x001f
#define	TAG_set_type			0x0020
#define	TAG_subrange_type		0x0021
#define	TAG_with_stmt			0x0022
#define	TAG_lo_user			0x8000
#define	TAG_hi_user			0xffff

/* old name for compatibility */
#define TAG_source_file			TAG_compile_unit

/* attribute forms are encoded as part */
/* of the attribute name and must fit */
/* into 4 bits */

#define	FORM_MASK	0xf

#define	FORM_NONE	0x0	/* error */
#define	FORM_ADDR	0x1	/* relocated address */
#define	FORM_REF	0x2	/* reference to another .debug entry */
#define	FORM_BLOCK2	0x3	/* block with 2-byte length */
#define	FORM_BLOCK4	0x4	/* block with 4-byte length (unused) */
#define	FORM_DATA2	0x5	/* 2 bytes */
#define	FORM_DATA4	0x6	/* 4 bytes */
#define	FORM_DATA8	0x7	/* 8 bytes (two 4-byte values) */
#define	FORM_STRING	0x8	/* NUL-terminated string */


/* attribute names, halfwords with low 4 bits indicating the form */

#define	AT_padding	 (0x0000|FORM_NONE)	/* just padding */
#define	AT_sibling	 (0x0010|FORM_REF)	/* next owned declaration */
#define	AT_location	 (0x0020|FORM_BLOCK2)	/* location description */
#define	AT_name		 (0x0030|FORM_STRING)	/* symbol name */
#define	AT_fund_type	 (0x0050|FORM_DATA2)	/* fund type enum */
#define	AT_mod_fund_type (0x0060|FORM_BLOCK2)	/* modifiers & fund type enum */
#define	AT_user_def_type (0x0070|FORM_REF)	/* type entry */
#define	AT_mod_u_d_type  (0x0080|FORM_BLOCK2)	/* modifiers & type entry ref */
#define	AT_ordering	 (0x0090|FORM_DATA2)	/* array row/column major */
#define	AT_subscr_data	 (0x00a0|FORM_BLOCK2)	/* list of array dim info */
#define	AT_byte_size	 (0x00b0|FORM_DATA4)	/* number bytes per instance */
#define	AT_bit_offset	 (0x00c0|FORM_DATA2)	/* number bits padding */
#define	AT_bit_size	 (0x00d0|FORM_DATA4)	/* number bits per instance */
#define	AT_element_list	 (0x00f0|FORM_BLOCK4)	/* list of enum data elements */
#define	AT_stmt_list	 (0x0100|FORM_DATA4)	/* offset in .line sect */
#define	AT_low_pc	 (0x0110|FORM_ADDR)	/* first machine instr */
#define	AT_high_pc	 (0x0120|FORM_ADDR)	/* beyond last machine instr */
#define	AT_language	 (0x0130|FORM_DATA4)	/* compiler enumeration */
#define	AT_member	 (0x0140|FORM_REF)	/* class description */
#define	AT_discr	 (0x0150|FORM_REF)	/* discriminant entry */
#define	AT_discr_value	 (0x0160|FORM_BLOCK2)	/* value of discr */
#define	AT_string_length (0x0190|FORM_BLOCK2)	/* runtime string size */
#define	AT_common_reference (0x01a0|FORM_REF)	/* referenced common block*/
#define	AT_comp_dir 	(0x01b0|FORM_STRING)	/* current working dir of compiler*/
#define	AT_const_value	0x01c0		/* constant valued object - can have multiple forms*/
#define	AT_containing_type 	(0x01d0|FORM_REF)/* class containing ptr to member type*/
#define	AT_default_value 0x01e0	/* default value of parameter -can have multiple forms */
#define	AT_friends	 (0x01f0|FORM_BLOCK2)	/* list of friends*/
#define	AT_inline	 (0x0200|FORM_STRING)	/* declaration of inlined function*/
#define	AT_is_optional	 (0x0210|FORM_STRING)	/* FORTRAN optional parameters*/
#define	AT_lower_bound	 0x0220	/* lower bound of subrange - can have multiple forms */
#define	AT_program	 (0x0230|FORM_STRING)	/* FORTRAN main program*/
#define	AT_private	 (0x0240|FORM_STRING)	/* prvivate class members*/
#define	AT_producer	 (0x0250|FORM_STRING)	/* compiler version and vendor */
#define	AT_protected	 (0x0260|FORM_STRING)	/* protected class members*/
#define	AT_prototyped	 (0x0270|FORM_STRING)	/* function is prototyped*/
#define	AT_public	 (0x0280|FORM_STRING)	/* public class members*/
#define	AT_pure_virtual	 (0x0290|FORM_STRING)	/* pure virtual member function*/
#define	AT_return_addr	(0x02a0|FORM_BLOCK2)	/* where return addr is stored*/
#define	AT_specification (0x02b0|FORM_REF)	/* ptr from inlined subroutine instance to declaration */
#define	AT_start_scope	 (0x02c0|FORM_DATA4)	/* scope of object begins after enclosing scope*/
#define	AT_stride_size	 (0x02e0|FORM_DATA4)	/* non-standard element size */
#define	AT_upper_bound	 0x02f0	/* upper bound of subrange - can have multiple forms */
#define	AT_virtual	 (0x0300|FORM_STRING)	/* virtual member function*/
#define	AT_lo_user	 0x2000
#define	AT_hi_user	 0x3ff0



/* atoms which compose a location description; must fit in a byte */

#define	OP_UNK		0x00	/* error */
#define	OP_REG		0x01	/* push register (number) */
#define	OP_BASEREG	0x02	/* push value of register (number) */
#define	OP_ADDR		0x03	/* push address (relocated address) */
#define	OP_CONST	0x04	/* push constant (number) */
#define	OP_DEREF2	0x05	/* pop, deref and push 2 bytes (as a long) */
#define	OP_DEREF	0x06	/* pop, deref and push 4 bytes (as a long) */
#define	OP_ADD		0x07	/* pop top 2 items, add, push result */

#define OP_SCO_REG_PAIR	0x81	/* long long register pair, in OP vendor-specific range */

/* old name for compatibility */
#define OP_DEREF4	OP_DEREF

/* fundamental types; must fit in two bytes */
#define	FT_none			0x0000	/* error */
#define	FT_char			0x0001	/* "plain" char */
#define	FT_signed_char		0x0002
#define	FT_unsigned_char	0x0003
#define	FT_short		0x0004	/* "plain" short */
#define	FT_signed_short		0x0005
#define	FT_unsigned_short	0x0006
#define	FT_integer		0x0007	/* "plain" integer */
#define	FT_signed_integer	0x0008
#define	FT_unsigned_integer	0x0009
#define	FT_long			0x000a	/* "plain" long */
#define	FT_signed_long		0x000b
#define	FT_unsigned_long	0x000c
#define	FT_pointer		0x000d	/* (void *) */
#define	FT_float		0x000e
#define	FT_dbl_prec_float	0x000f
#define	FT_ext_prec_float	0x0010
#define	FT_complex		0x0011
#define	FT_dbl_prec_complex	0x0012
#define	FT_void			0x0014
#define	FT_boolean		0x0015
#define	FT_ext_prec_complex	0x0016
#define	FT_label		0x0017
#define FT_long_long		0x0018
#define FT_signed_long_long	0x0019
#define FT_unsigned_long_long	0x001a
#define	FT_lo_user		0x8000
#define	FT_hi_user		0xffff


/* type modifiers; must fit in a byte */

#define	MOD_none		0x00	/* error */
#define	MOD_pointer_to		0x01
#define	MOD_reference_to	0x02
#define	MOD_const		0x03
#define	MOD_volatile		0x04
#define	MOD_lo_user		0x80
#define	MOD_hi_user		0xff


/* the "format" byte for array descriptions; formed from three */
/* one-bit fields */

#define	FMT_FT	0		/* fundamental type */
#define	FMT_UDT	1		/* user-defined type */

#define	FMT_CONST	0	/* 4-byte constant */
#define	FMT_EXPR	1	/* block with 2-byte length (loc descr) */

#define	FMT_FT_C_C	( (FMT_FT <<2) | (FMT_CONST<<1) | (FMT_CONST) )
#define	FMT_FT_C_X	( (FMT_FT <<2) | (FMT_CONST<<1) | (FMT_EXPR)  )
#define	FMT_FT_X_C	( (FMT_FT <<2) | (FMT_EXPR <<1) | (FMT_CONST) )
#define	FMT_FT_X_X	( (FMT_FT <<2) | (FMT_EXPR <<1) | (FMT_EXPR)  )
#define	FMT_UT_C_C	( (FMT_UDT<<2) | (FMT_CONST<<1) | (FMT_CONST) )
#define	FMT_UT_C_X	( (FMT_UDT<<2) | (FMT_CONST<<1) | (FMT_EXPR)  )
#define	FMT_UT_X_C	( (FMT_UDT<<2) | (FMT_EXPR <<1) | (FMT_CONST) )
#define	FMT_UT_X_X	( (FMT_UDT<<2) | (FMT_EXPR <<1) | (FMT_EXPR)  )

#define	FMT_ET		8	/* element type */


/* ordering of arrays */

#define	ORD_row_major	0
#define ORD_col_major	1

/* language - allocated 4 bytes, but should fit in 2*/

enum LANG {
	LANG_UNK 		= 0,
	LANG_C89 		= 0x0001,
	LANG_C	 		= 0x0002,
	LANG_ADA83 		= 0x0003,
	LANG_C_PLUS_PLUS 	= 0x0004,
	LANG_COBOL74 		= 0x0005,
	LANG_COBOL85 		= 0x0006,
	LANG_FORTRAN77 		= 0x0007,
	LANG_FORTRAN90 		= 0x0008,
	LANG_PASCAL83 		= 0x0009,
	LANG_MODULA2 		= 0x000a,
	LANG_LO_USER		= 0x8000,
	LANG_HI_USER		= 0xffff
};

/* old name for compatibility */
#define	LANG_ANSI_C_V1		LANG_C89

#endif /* DWARF_H */
