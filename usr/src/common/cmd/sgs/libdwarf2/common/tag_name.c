#ident	"@(#)libdwarf2:common/tag_name.c	1.3"

#include "dwarf2.h"
#include "libdwarf2.h"
#include "Table.h"

static Table tag_names[] =
{
	{ DW_TAG_array_type,		"DW_TAG_array_type" },
	{ DW_TAG_class_type,		"DW_TAG_class_type" },
	{ DW_TAG_entry_point,		"DW_TAG_entry_point" },
	{ DW_TAG_enumeration_type,	"DW_TAG_enumeration_type" },
	{ DW_TAG_formal_parameter,	"DW_TAG_formal_parameter" },
	{ DW_TAG_imported_declaration,	"DW_TAG_imported_declaration" },
	{ DW_TAG_label,			"DW_TAG_label" },
	{ DW_TAG_lexical_block,		"DW_TAG_lexical_block" },
	{ DW_TAG_member,		"DW_TAG_member" },
	{ DW_TAG_pointer_type,		"DW_TAG_pointer_type" },
	{ DW_TAG_reference_type,	"DW_TAG_reference_type" },
	{ DW_TAG_compile_unit,		"DW_TAG_compile_unit" },
	{ DW_TAG_string_type,		"DW_TAG_string_type" },
	{ DW_TAG_structure_type,	"DW_TAG_structure_type" },
	{ DW_TAG_subroutine_type,	"DW_TAG_subroutine_type" },
	{ DW_TAG_typedef,		"DW_TAG_typedef" },
	{ DW_TAG_union_type,		"DW_TAG_union_type" },
	{ DW_TAG_unspecified_parameters, "DW_TAG_unspecified_parameters" },
	{ DW_TAG_variant,		"DW_TAG_variant" },
	{ DW_TAG_common_block,		"DW_TAG_common_block" },
	{ DW_TAG_common_inclusion,	"DW_TAG_common_inclusion" },
	{ DW_TAG_inheritance,		"DW_TAG_inheritance" },
	{ DW_TAG_inlined_subroutine,	"DW_TAG_inlined_subroutine" },
	{ DW_TAG_module,		"DW_TAG_module" },
	{ DW_TAG_ptr_to_member_type,	"DW_TAG_ptr_to_member_type" },
	{ DW_TAG_set_type,		"DW_TAG_set_type" },
	{ DW_TAG_subrange_type,		"DW_TAG_subrange_type" },
	{ DW_TAG_with_stmt,		"DW_TAG_with_stmt" },
	{ DW_TAG_access_declaration,	"DW_TAG_access_declaration" },
	{ DW_TAG_base_type,		"DW_TAG_base_type" },
	{ DW_TAG_catch_block,		"DW_TAG_catch_block" },
	{ DW_TAG_const_type,		"DW_TAG_const_type" },
	{ DW_TAG_constant,		"DW_TAG_constant" },
	{ DW_TAG_enumerator,		"DW_TAG_enumerator" },
	{ DW_TAG_file_type,		"DW_TAG_file_type" },
	{ DW_TAG_friend,		"DW_TAG_friend" },
	{ DW_TAG_namelist,		"DW_TAG_namelist" },
	{ DW_TAG_namelist_item,		"DW_TAG_namelist_item" },
	{ DW_TAG_packed_type,		"DW_TAG_packed_type" },
	{ DW_TAG_subprogram,		"DW_TAG_subprogram" },
	{ DW_TAG_template_type_param,	"DW_TAG_template_type_param" },
	{ DW_TAG_template_value_param,	"DW_TAG_template_value_param" },
	{ DW_TAG_thrown_type,		"DW_TAG_thrown_type" },
	{ DW_TAG_try_block,		"DW_TAG_try_block" },
	{ DW_TAG_variant_part,		"DW_TAG_variant_part" },
	{ DW_TAG_variable,		"DW_TAG_variable" },
	{ DW_TAG_volatile_type,		"DW_TAG_volatile_type" },
	{ DW_TAG_lo_user,		"DW_TAG_lo_user" },
	{ DW_TAG_SCO_namespace,		"DW_TAG_SCO_namespace" },
	{ DW_TAG_SCO_namespace_alias,	"DW_TAG_SCO_namespace_alias" },
	{ DW_TAG_SCO_using_declaration,	"DW_TAG_SCO_using_declaration" },
	{ DW_TAG_SCO_using_directive,	"DW_TAG_SCO_using_directive" },
	{ DW_TAG_hi_user,		"DW_TAG_hi_user" },
	0
};

const char *
dwarf2_tag_name(unsigned int tag)
{
	return get_name(tag_names, tag);
}
