#ident	"@(#)libdwarf2:common/abbrev.c	1.2"

#include "dwarf2.h"
#include "libdwarf2.h"
#include "abbrev.h"
#include <stdlib.h>

/* NOTICE! NOTICE! NOTICE! NOTICE! NOTICE! NOTICE! NOTICE!
**
** Whenever changing/adding/or deleting an entry, update the version id
** in abbrev.h, so that the abbreviation table linked in matches what the compiler
** thinks it is putting out.
**
** Also, make sure that all the places where the corresponding debugging
** entries are generated are updated -- most of those places are not table
** driven and have a 1-to-1 hard-coded correspondence between attributes
** and their values.  Debugging entries are generated in acomp:common/elfdebug.c,
** cplusfe:common/dwarf_dbg.c, and, for some cases cplusbe:common/elfdebug.c.
**
** NOTICE! NOTICE! NOTICE! NOTICE! NOTICE! NOTICE! NOTICE!
*/

/*
** Define tables for abbreviation table entries
*/

static Dwarf2_Abbreviation *abbrev_table;

static const Dwarf2_Attribute dw2_compile_unit[] =
{
	{ DW_TAG_compile_unit , DW_CHILDREN_yes },
	{ DW_AT_comp_dir, DW_FORM_string },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_language, DW_FORM_data1 },
	{ DW_AT_producer, DW_FORM_string },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
	{ DW_AT_stmt_list, DW_FORM_data4 },
};

static const Dwarf2_Attribute dw2_compile_no_line_info[] =
{
	{ DW_TAG_compile_unit , DW_CHILDREN_yes },
	{ DW_AT_comp_dir, DW_FORM_string },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_language, DW_FORM_data1 },
	{ DW_AT_producer, DW_FORM_string },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

static const Dwarf2_Attribute dw2_base_type[] =
{
	{ DW_TAG_base_type, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_encoding, DW_FORM_data1 },
	{ DW_AT_byte_size, DW_FORM_udata },
};

static const Dwarf2_Attribute dw2_pointer_type[] =
{
	{ DW_TAG_pointer_type, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_reference_type[] =
{
	{ DW_TAG_reference_type, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_const_type[] =
{
	{ DW_TAG_const_type, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_volatile_type[] =
{
	{ DW_TAG_volatile_type, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_typedef[] =
{
	{ DW_TAG_typedef, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_struct[] =		/* struct */
{
	{ DW_TAG_structure_type, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_byte_size, DW_FORM_udata },
};

static const Dwarf2_Attribute dw2_incomplete_struct[] =	/* incomplete struct */
{
	{ DW_TAG_structure_type, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_declaration, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_union[] =		/* union */
{
	{ DW_TAG_union_type, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_byte_size, DW_FORM_udata },
};

static const Dwarf2_Attribute dw2_incomplete_union[] =	/* incomplete union */
{
	{ DW_TAG_union_type, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_declaration, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_su_member[] = /* C struct/union member */
{
	{ DW_TAG_member, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_data_member_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_csu_member[] = /* C++ class/struct/union member */
{
	{ DW_TAG_member, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_data_member_location, DW_FORM_block1 },
	{ DW_AT_SCO_mutable, DW_FORM_flag },
	{ DW_AT_artificial, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_bit_field[] =		/* bit field */
{
	{ DW_TAG_member, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_byte_size, DW_FORM_udata },
	{ DW_AT_bit_size, DW_FORM_udata },
	{ DW_AT_bit_offset, DW_FORM_data1 },
	{ DW_AT_SCO_mutable, DW_FORM_flag },
	{ DW_AT_data_member_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_enumeration[] =		/* enumeration */
{
	{ DW_TAG_enumeration_type, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_byte_size, DW_FORM_udata },
};

static const Dwarf2_Attribute dw2_incomplete_enumeration[] = /* incomplete enumeration */
{
	{ DW_TAG_enumeration_type, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_declaration, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_enumerator[] =		/* enumerator */
{
	{ DW_TAG_enumerator, DW_CHILDREN_no },
	{ DW_AT_const_value, DW_FORM_sdata },
	{ DW_AT_name, DW_FORM_string },
};

static const Dwarf2_Attribute dw2_array[] =		/* array */
{
	{ DW_TAG_array_type, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_byte_size, DW_FORM_udata },
};

static const Dwarf2_Attribute dw2_subrange[] =		/* subrange */
{
	{ DW_TAG_subrange_type, DW_CHILDREN_no },
	{ DW_AT_count, DW_FORM_udata },
};

static const Dwarf2_Attribute dw2_non_void_subr_type[] = /* subroutine type */
{
    						/* non-void return type */
	{ DW_TAG_subroutine_type, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_prototyped, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_void_subr_type[] =	/* subroutine type */
{
							/* void return type */
	{ DW_TAG_subroutine_type, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_prototyped, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_formal_param_type[] =	/* param type */
{
	{ DW_TAG_formal_parameter, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_artif_formal_param_type[] = /* artificial param type */
{
	{ DW_TAG_formal_parameter, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_artificial, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_unspecified_params[] =	/* unspecified params */
{
	{ DW_TAG_unspecified_parameters, DW_CHILDREN_no },
};

/* subroutine, non-void return type */
static const Dwarf2_Attribute dw2_non_void_subr[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_prototyped, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};
/* inlined subroutine, non-void return type */
static const Dwarf2_Attribute dw2_non_void_subr_inline[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_prototyped, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_inline, DW_FORM_data1 },
};

/* subroutine, void return type */
static const Dwarf2_Attribute dw2_void_subr[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_prototyped, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

/* inlined subroutine, void return type */
static const Dwarf2_Attribute dw2_void_subr_inline[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_prototyped, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_inline, DW_FORM_data1 },
};

/* artificial subroutine, non-void return type */
static const Dwarf2_Attribute dw2_artif_non_void_subr[] = 
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_prototyped, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

/* artificial subroutine, non-void return type */
static const Dwarf2_Attribute dw2_artif_non_void_subr_inline[] = 
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_prototyped, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_inline, DW_FORM_data1 },
};

/* artificial subroutine, void return type */
static const Dwarf2_Attribute dw2_artif_void_subr[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_prototyped, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

/* artificial subroutine, void return type */
static const Dwarf2_Attribute dw2_artif_void_subr_inline[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_prototyped, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_inline, DW_FORM_data1 },
};

static const Dwarf2_Attribute dw2_variable[] =		/* variable */
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_variable_no_decl[] =		/* variable */
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_artif_variable[] = /* artificial variable */
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_formal_param[] =	/* formal param */
{
	{ DW_TAG_formal_parameter, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_formal_param_no_decl[] =	/* formal param */
{
	{ DW_TAG_formal_parameter, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_artif_formal_param[] = /* artificial formal param */
{
	{ DW_TAG_formal_parameter, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_variable_abstract[] =		/* variable */
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_external, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_artif_variable_abstract[] = /* artificial variable */
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_external, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_formal_param_abstract[] =	/* formal param */
{
	{ DW_TAG_formal_parameter, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
};

/* artificial formal param */
static const Dwarf2_Attribute dw2_artif_formal_param_abstract[] =
{
	{ DW_TAG_formal_parameter, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_artificial, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_variable_concrete[] =		/* variable */
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_abstract_origin, DW_FORM_ref4 },
	{ DW_AT_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_formal_param_concrete[] =	/* formal param */
{
	{ DW_TAG_formal_parameter, DW_CHILDREN_no },
	{ DW_AT_abstract_origin, DW_FORM_ref4 },
	{ DW_AT_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_lexical_block[] = /* lexical block */
{
	{ DW_TAG_lexical_block, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

static const Dwarf2_Attribute dw2_lexical_block_abstract[] = /* lexical block */
{
	{ DW_TAG_lexical_block, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_lexical_block_concrete[] = /* lexical block */
{
	{ DW_TAG_lexical_block, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_abstract_origin, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

static const Dwarf2_Attribute dw2_label[] = /* label */
{
	{ DW_TAG_label, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_low_pc, DW_FORM_addr },
};

static const Dwarf2_Attribute dw2_label_abstract[] = /* label in inlined function */
{
	{ DW_TAG_label, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
};

static const Dwarf2_Attribute dw2_label_concrete[] = /* label in inlined function */
{
	{ DW_TAG_label, DW_CHILDREN_no },
	{ DW_AT_abstract_origin, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
};

static const Dwarf2_Attribute dw2_class[] =		/* class */
{
	{ DW_TAG_class_type, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_byte_size, DW_FORM_udata },
};

static const Dwarf2_Attribute dw2_incomplete_class[] =	/* incomplete class */
{
	{ DW_TAG_class_type, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_declaration, DW_FORM_flag },
};

/* member function declaration, non-void return type */
static const Dwarf2_Attribute dw2_non_void_function_decl[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_declaration, DW_FORM_flag },
};

/* member function declaration, void return type */
static const Dwarf2_Attribute dw2_void_function_decl[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_declaration, DW_FORM_flag },
};

/* virtual member function declaration, non-void return type */
static const Dwarf2_Attribute dw2_non_void_virtual_member[] = 
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_virtuality, DW_FORM_data1 },
	{ DW_AT_vtable_elem_location, DW_FORM_block1 },
	{ DW_AT_declaration, DW_FORM_flag },
};

/* virtual member function declaration, void return type */
static const Dwarf2_Attribute dw2_void_virtual_member[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_virtuality, DW_FORM_data1 },
	{ DW_AT_vtable_elem_location, DW_FORM_block1 },
	{ DW_AT_declaration, DW_FORM_flag },
};

/* member function definition */
static const Dwarf2_Attribute dw2_member_defn[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_specification, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

/* inline member function definition */
static const Dwarf2_Attribute dw2_member_inline[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_specification, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_inline, DW_FORM_data1 },
};

/* artificial member function declaration, non-void return type */
static const Dwarf2_Attribute dw2_artif_non_void_member[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_declaration, DW_FORM_flag },
};

/* artificial member function declaration, void return type */
static const Dwarf2_Attribute dw2_artif_void_member[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_declaration, DW_FORM_flag },
};

/* artificial virtual member function declaration, non-void return type */
static const Dwarf2_Attribute dw2_artif_non_void_virtual_member[] = 
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_virtuality, DW_FORM_data1 },
	{ DW_AT_vtable_elem_location, DW_FORM_block1 },
	{ DW_AT_declaration, DW_FORM_flag },
};

/* artificial virtual member function declaration, void return type */
static const Dwarf2_Attribute dw2_artif_void_virtual_member[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_artificial, DW_FORM_flag },
	{ DW_AT_virtuality, DW_FORM_data1 },
	{ DW_AT_vtable_elem_location, DW_FORM_block1 },
	{ DW_AT_declaration, DW_FORM_flag },
};

/* artificial member function definition */
static const Dwarf2_Attribute dw2_artif_member_defn[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_specification, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

/* artificial inline member function definition */
static const Dwarf2_Attribute dw2_artif_member_inline[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_specification, DW_FORM_ref4 },
	{ DW_AT_inline, DW_FORM_data1 },
};

/* base class */
static const Dwarf2_Attribute dw2_inheritance[] =
{
	{ DW_TAG_inheritance, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_accessibility, DW_FORM_data1 },
	{ DW_AT_virtuality, DW_FORM_data1 },
	{ DW_AT_data_member_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_static_member[] =	/* static data member definition */
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_specification, DW_FORM_ref4 },
	{ DW_AT_data_member_location, DW_FORM_block1 },
};

/* static data member declaration (child of class definition) */
static const Dwarf2_Attribute dw2_static_member_decl[] =
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_declaration, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_ptr_to_member[] =
{
	{ DW_TAG_ptr_to_member_type, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_containing_type, DW_FORM_ref4 },
	{ DW_AT_byte_size, DW_FORM_udata },
	{ DW_AT_use_location, DW_FORM_block1 },
};

static const Dwarf2_Attribute dw2_ptr_to_function_member[] =
{
	{ DW_TAG_ptr_to_member_type, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_containing_type, DW_FORM_ref4 },
	{ DW_AT_byte_size, DW_FORM_udata },
	{ DW_AT_use_location, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_unrefed_variable[] = /* unreferenced local variable */
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
};

static const Dwarf2_Attribute dw2_variable_decl[] = /* external variable declaration */
{
	{ DW_TAG_variable, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_decl_file, DW_FORM_udata },
	{ DW_AT_decl_line, DW_FORM_udata },
	{ DW_AT_external, DW_FORM_flag },
	{ DW_AT_declaration, DW_FORM_flag },
};

static const Dwarf2_Attribute dw2_try_block[] = /* try block */
{
	{ DW_TAG_try_block, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

static const Dwarf2_Attribute dw2_try_block_abstract[] = /* try block */
{
	{ DW_TAG_try_block, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_try_block_concrete[] = /* try block */
{
	{ DW_TAG_try_block, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_abstract_origin, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

static const Dwarf2_Attribute dw2_catch_block[] = /* catch handler */
{
	{ DW_TAG_catch_block, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

static const Dwarf2_Attribute dw2_catch_block_abstract[] = /* catch handler */
{
	{ DW_TAG_catch_block, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_catch_block_concrete[] = /* catch handler */
{
	{ DW_TAG_catch_block, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_abstract_origin, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

static const Dwarf2_Attribute dw2_thrown_type[] =
{
	{ DW_TAG_thrown_type, DW_CHILDREN_no },
	{ DW_AT_type, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_template_type_param[] =
{
	{ DW_TAG_template_type_param, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_template_value_param[] =
{
	{ DW_TAG_template_value_param, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_type, DW_FORM_ref4 },
	{ DW_AT_const_value, DW_FORM_indirect },
};

static const Dwarf2_Attribute dw2_namespace[] =
{
	{ DW_TAG_SCO_namespace, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_name, DW_FORM_string },
};

static const Dwarf2_Attribute dw2_namespace_alias[] =
{
	{ DW_TAG_SCO_namespace_alias, DW_CHILDREN_no },
	{ DW_AT_name, DW_FORM_string },
	{ DW_AT_SCO_namespace, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_using_directive[] =
{
	{ DW_TAG_SCO_using_directive, DW_CHILDREN_no },
	{ DW_AT_SCO_namespace, DW_FORM_ref4 },
};

static const Dwarf2_Attribute dw2_using_declaration[] =
{
	{ DW_TAG_SCO_using_declaration, DW_CHILDREN_no },
	{ DW_AT_SCO_using, DW_FORM_ref4 },
};

/* inlined subroutine */
static const Dwarf2_Attribute dw2_inlined_subroutine[] =
{
	{ DW_TAG_inlined_subroutine, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_abstract_origin, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

/* out-of-line instance of an inlined subroutine */
static const Dwarf2_Attribute dw2_out_of_line_instance[] =
{
	{ DW_TAG_subprogram, DW_CHILDREN_yes },
	{ DW_AT_sibling, DW_FORM_ref4 },
	{ DW_AT_abstract_origin, DW_FORM_ref4 },
	{ DW_AT_low_pc, DW_FORM_addr },
	{ DW_AT_high_pc, DW_FORM_addr },
};

static int errors;

static void
create_abbrev_entry(Abbrev_index index, const Dwarf2_Attribute *entry, int nentries)
{
	static short curr_abbrev_index;

	if (++curr_abbrev_index != (int)index)
	{
		++errors;
		return;
	}
	abbrev_table[index].tag = entry->name;
	abbrev_table[index].nattr = nentries;
	abbrev_table[index].children = entry->form;
	abbrev_table[index].attributes = nentries ? &entry[1] : 0;
}

#define CREATE_ENTRY(index, arr) \
	create_abbrev_entry(index, arr, (sizeof(arr)/sizeof(Dwarf2_Attribute))-1)

const Dwarf2_Abbreviation *
dwarf2_gen_abbrev_table()
{
	if (abbrev_table)
		return abbrev_table;

	if ((abbrev_table = (Dwarf2_Abbreviation *)malloc(
					DW2_last * sizeof(Dwarf2_Abbreviation)))
			== 0)
	{
		return 0;
	}

	errors = 0;
	abbrev_table[0].tag = 0;
	abbrev_table[0].nattr = 0;
	abbrev_table[0].children = DW_CHILDREN_no;
	abbrev_table[0].attributes = 0;

	CREATE_ENTRY(DW2_compile_unit, dw2_compile_unit);
	CREATE_ENTRY(DW2_compile_no_line_info, dw2_compile_no_line_info);
	CREATE_ENTRY(DW2_base_type, dw2_base_type);
	CREATE_ENTRY(DW2_pointer_type, dw2_pointer_type);
	CREATE_ENTRY(DW2_reference_type, dw2_reference_type);
	CREATE_ENTRY(DW2_const_type, dw2_const_type);
	CREATE_ENTRY(DW2_volatile_type, dw2_volatile_type);
	CREATE_ENTRY(DW2_typedef, dw2_typedef);
	CREATE_ENTRY(DW2_class, dw2_class);
	CREATE_ENTRY(DW2_incomplete_class, dw2_incomplete_class);
	CREATE_ENTRY(DW2_struct, dw2_struct);
	CREATE_ENTRY(DW2_incomplete_struct, dw2_incomplete_struct);
	CREATE_ENTRY(DW2_union, dw2_union);
	CREATE_ENTRY(DW2_incomplete_union, dw2_incomplete_union);
	CREATE_ENTRY(DW2_su_member, dw2_su_member);
	CREATE_ENTRY(DW2_csu_member, dw2_csu_member);
	CREATE_ENTRY(DW2_bit_field, dw2_bit_field);
	CREATE_ENTRY(DW2_enumeration, dw2_enumeration);
	CREATE_ENTRY(DW2_incomplete_enumeration, dw2_incomplete_enumeration);
	CREATE_ENTRY(DW2_enumerator, dw2_enumerator);
	CREATE_ENTRY(DW2_array, dw2_array);
	CREATE_ENTRY(DW2_subrange, dw2_subrange);
	CREATE_ENTRY(DW2_non_void_subr_type, dw2_non_void_subr_type);
	CREATE_ENTRY(DW2_void_subr_type, dw2_void_subr_type);
	CREATE_ENTRY(DW2_formal_param_type, dw2_formal_param_type);
	CREATE_ENTRY(DW2_artif_formal_param_type, dw2_artif_formal_param_type);
	CREATE_ENTRY(DW2_unspecified_params, dw2_unspecified_params);
	CREATE_ENTRY(DW2_variable, dw2_variable);
	CREATE_ENTRY(DW2_variable_no_decl, dw2_variable_no_decl);
	CREATE_ENTRY(DW2_artif_variable, dw2_artif_variable);
	CREATE_ENTRY(DW2_formal_param, dw2_formal_param);
	CREATE_ENTRY(DW2_formal_param_no_decl, dw2_formal_param_no_decl);
	CREATE_ENTRY(DW2_artif_formal_param, dw2_artif_formal_param);
	CREATE_ENTRY(DW2_variable_abstract, dw2_variable_abstract);
	CREATE_ENTRY(DW2_artif_variable_abstract,
				dw2_artif_variable_abstract);
	CREATE_ENTRY(DW2_formal_param_abstract, dw2_formal_param_abstract);
	CREATE_ENTRY(DW2_artif_formal_param_abstract,
				dw2_artif_formal_param_abstract);
	CREATE_ENTRY(DW2_variable_concrete, dw2_variable_concrete);
	CREATE_ENTRY(DW2_formal_param_concrete, dw2_formal_param_concrete);
	CREATE_ENTRY(DW2_label, dw2_label);
	CREATE_ENTRY(DW2_label_abstract, dw2_label_abstract);
	CREATE_ENTRY(DW2_label_concrete, dw2_label_concrete);
	CREATE_ENTRY(DW2_lexical_block, dw2_lexical_block);
	CREATE_ENTRY(DW2_lexical_block_abstract, dw2_lexical_block_abstract);
	CREATE_ENTRY(DW2_lexical_block_concrete, dw2_lexical_block_concrete);
	CREATE_ENTRY(DW2_void_subr, dw2_void_subr);
	CREATE_ENTRY(DW2_non_void_subr, dw2_non_void_subr);
	CREATE_ENTRY(DW2_artif_void_subr, dw2_artif_void_subr);
	CREATE_ENTRY(DW2_artif_non_void_subr, dw2_artif_non_void_subr);
	CREATE_ENTRY(DW2_member_defn, dw2_member_defn);
	CREATE_ENTRY(DW2_artif_member_defn, dw2_artif_member_defn);
	CREATE_ENTRY(DW2_non_void_subr_inline, dw2_non_void_subr_inline);
	CREATE_ENTRY(DW2_void_subr_inline, dw2_void_subr_inline);
	CREATE_ENTRY(DW2_artif_non_void_subr_inline,
				dw2_artif_non_void_subr_inline);
	CREATE_ENTRY(DW2_artif_void_subr_inline, dw2_artif_void_subr_inline);
	CREATE_ENTRY(DW2_member_inline, dw2_member_inline);
	CREATE_ENTRY(DW2_artif_member_inline, dw2_artif_member_inline);
	CREATE_ENTRY(DW2_non_void_function_decl, dw2_non_void_function_decl);
	CREATE_ENTRY(DW2_void_function_decl, dw2_void_function_decl);
	CREATE_ENTRY(DW2_non_void_virtual_member, dw2_non_void_virtual_member);
	CREATE_ENTRY(DW2_void_virtual_member, dw2_void_virtual_member);
	CREATE_ENTRY(DW2_artif_non_void_member, dw2_artif_non_void_member);
	CREATE_ENTRY(DW2_artif_void_member, dw2_artif_void_member);
	CREATE_ENTRY(DW2_artif_non_void_virtual_member,
				dw2_artif_non_void_virtual_member);
	CREATE_ENTRY(DW2_artif_void_virtual_member, dw2_artif_void_virtual_member);
	CREATE_ENTRY(DW2_inheritance, dw2_inheritance);
	CREATE_ENTRY(DW2_static_member, dw2_static_member);
	CREATE_ENTRY(DW2_static_member_decl, dw2_static_member_decl);
	CREATE_ENTRY(DW2_ptr_to_member, dw2_ptr_to_member);
	CREATE_ENTRY(DW2_ptr_to_function_member, dw2_ptr_to_function_member);
	CREATE_ENTRY(DW2_unrefed_variable, dw2_unrefed_variable);
	CREATE_ENTRY(DW2_variable_decl, dw2_variable_decl);
	CREATE_ENTRY(DW2_try_block, dw2_try_block);
	CREATE_ENTRY(DW2_try_block_abstract, dw2_try_block_abstract);
	CREATE_ENTRY(DW2_try_block_concrete, dw2_try_block_concrete);
	CREATE_ENTRY(DW2_catch_block, dw2_catch_block);
	CREATE_ENTRY(DW2_catch_block_abstract, dw2_catch_block_abstract);
	CREATE_ENTRY(DW2_catch_block_concrete, dw2_catch_block_concrete);
	CREATE_ENTRY(DW2_thrown_type, dw2_thrown_type);
	CREATE_ENTRY(DW2_template_type_param, dw2_template_type_param);
	CREATE_ENTRY(DW2_template_value_param, dw2_template_value_param);
	CREATE_ENTRY(DW2_namespace, dw2_namespace);
	CREATE_ENTRY(DW2_namespace_alias, dw2_namespace_alias);
	CREATE_ENTRY(DW2_using_directive, dw2_using_directive);
	CREATE_ENTRY(DW2_using_declaration, dw2_using_declaration);
	CREATE_ENTRY(DW2_inlined_subroutine, dw2_inlined_subroutine);
	CREATE_ENTRY(DW2_out_of_line_instance, dw2_out_of_line_instance);

	if (errors)
	{
		free(abbrev_table);
		abbrev_table = 0;
		return 0;
	}
	return abbrev_table;
}
