#ifndef ABBREV_H
#define ABBREV_H
#ident	"@(#)sgs-inc:common/abbrev.h	1.1"

/************************************************
 *						*
 *	DWARF2 abbreviation table indices	*
 *						*
 ************************************************/

/* Change version string whenever changing/adding to/deleteing from
** Abbrev_index, or changing/adding/or deleteing an entry in abbrev.c,
** so that the abbreviation table linked in matches what the compiler
** thinks it is putting out
*/

#define ABBREV_VERSION_ID	"__abbr_table_1.0"
typedef enum
{
	DW2_none,
	DW2_compile_unit,
	DW2_compile_no_line_info,
	DW2_base_type,
	DW2_pointer_type,
	DW2_reference_type,
	DW2_const_type,
	DW2_volatile_type,
	DW2_typedef,
	DW2_class,
	DW2_incomplete_class,
	DW2_struct,
	DW2_incomplete_struct,
	DW2_union,
	DW2_incomplete_union,
	DW2_su_member,
	DW2_csu_member,
	DW2_bit_field,
	DW2_enumeration,
	DW2_incomplete_enumeration,
	DW2_enumerator,
	DW2_array,
	DW2_subrange,
	DW2_non_void_subr_type,
	DW2_void_subr_type,
	DW2_formal_param_type,
	DW2_artif_formal_param_type,
	DW2_unspecified_params,
	DW2_variable,
	DW2_variable_no_decl,
	DW2_artif_variable,
	DW2_formal_param,
	DW2_formal_param_no_decl,
	DW2_artif_formal_param,
	DW2_variable_abstract,
	DW2_artif_variable_abstract,
	DW2_formal_param_abstract,
	DW2_artif_formal_param_abstract,
	DW2_variable_concrete,
	DW2_formal_param_concrete,
	DW2_label,
	DW2_label_abstract,
	DW2_label_concrete,
	DW2_lexical_block,
	DW2_lexical_block_abstract,
	DW2_lexical_block_concrete,
	DW2_void_subr,
	DW2_non_void_subr,
	DW2_artif_void_subr,
	DW2_artif_non_void_subr,
	DW2_member_defn,
	DW2_artif_member_defn,
	DW2_non_void_subr_inline,
	DW2_void_subr_inline,
	DW2_artif_non_void_subr_inline,
	DW2_artif_void_subr_inline,
	DW2_member_inline,
	DW2_artif_member_inline,
	DW2_non_void_function_decl,
	DW2_void_function_decl,
	DW2_non_void_virtual_member,
	DW2_void_virtual_member,
	DW2_artif_non_void_member,
	DW2_artif_void_member,
	DW2_artif_non_void_virtual_member,
	DW2_artif_void_virtual_member,
	DW2_inheritance,
	DW2_static_member,
	DW2_static_member_decl,
	DW2_ptr_to_member,
	DW2_ptr_to_function_member,
	DW2_unrefed_variable,
	DW2_variable_decl,
	DW2_try_block,
	DW2_try_block_abstract,
	DW2_try_block_concrete,
	DW2_catch_block,
	DW2_catch_block_abstract,
	DW2_catch_block_concrete,
	DW2_thrown_type,
	DW2_template_type_param,
	DW2_template_value_param,
	DW2_namespace,
	DW2_namespace_alias,
	DW2_using_directive,
	DW2_using_declaration,
	DW2_inlined_subroutine,
	DW2_out_of_line_instance,
	DW2_last
} Abbrev_index;

#endif /* ABBREV_H */
