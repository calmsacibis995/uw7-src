#ident	"@(#)libdwarf2:common/attr_name.c	1.3"

#include "dwarf2.h"
#include "libdwarf2.h"
#include "Table.h"

static Table attr_names[] =
{
	{ DW_AT_sibling,		"DW_AT_sibling" },
	{ DW_AT_location,		"DW_AT_location" },
	{ DW_AT_name,			"DW_AT_name" },
	{ DW_AT_ordering,		"DW_AT_ordering" },
	{ DW_AT_byte_size,		"DW_AT_byte_size" },
	{ DW_AT_bit_offset,		"DW_AT_bit_offset" },
	{ DW_AT_bit_size,		"DW_AT_bit_size" },
	{ DW_AT_stmt_list,		"DW_AT_stmt_list" },
	{ DW_AT_low_pc,			"DW_AT_low_pc" },
	{ DW_AT_high_pc,		"DW_AT_high_pc" },
	{ DW_AT_language,		"DW_AT_language" },
	{ DW_AT_discr,			"DW_AT_discr" },
	{ DW_AT_discr_value,		"DW_AT_discr_value" },
	{ DW_AT_visibility,		"DW_AT_visibility" },
	{ DW_AT_import,			"DW_AT_import" },
	{ DW_AT_string_length,		"DW_AT_string_length" },
	{ DW_AT_common_reference,	"DW_AT_common_reference" },
	{ DW_AT_comp_dir,		"DW_AT_comp_dir" },
	{ DW_AT_const_value,		"DW_AT_const_value" },
	{ DW_AT_containing_type,	"DW_AT_containing_type" },
	{ DW_AT_default_value,		"DW_AT_default_value" },
	{ DW_AT_inline,			"DW_AT_inline" },
	{ DW_AT_is_optional,		"DW_AT_is_optional" },
	{ DW_AT_lower_bound,		"DW_AT_lower_bound" },
	{ DW_AT_producer,		"DW_AT_producer" },
	{ DW_AT_prototyped,		"DW_AT_prototyped" },
	{ DW_AT_return_addr,		"DW_AT_return_addr" },
	{ DW_AT_start_scope,		"DW_AT_start_scope" },
	{ DW_AT_stride_size,		"DW_AT_stride_size" },
	{ DW_AT_upper_bound,		"DW_AT_upper_bound" },
	{ DW_AT_abstract_origin,	"DW_AT_abstract_origin" },
	{ DW_AT_accessibility,		"DW_AT_accessibility" },
	{ DW_AT_address_class,		"DW_AT_address_class" },
	{ DW_AT_artificial,		"DW_AT_artificial" },
	{ DW_AT_base_types,		"DW_AT_base_types" },
	{ DW_AT_calling_convention,	"DW_AT_calling_convention" },
	{ DW_AT_count,			"DW_AT_count" },
	{ DW_AT_data_member_location,	"DW_AT_data_member_location" },
	{ DW_AT_decl_column,		"DW_AT_decl_column" },
	{ DW_AT_decl_file,		"DW_AT_decl_file" },
	{ DW_AT_decl_line,		"DW_AT_decl_line" },
	{ DW_AT_declaration,		"DW_AT_declaration" },
	{ DW_AT_discr_list,		"DW_AT_discr_list" },
	{ DW_AT_encoding,		"DW_AT_encoding" },
	{ DW_AT_external,		"DW_AT_external" },
	{ DW_AT_frame_base,		"DW_AT_frame_base" },
	{ DW_AT_friend,			"DW_AT_friend" },
	{ DW_AT_identifier_case,	"DW_AT_identifier_case" },
	{ DW_AT_macro_info,		"DW_AT_macro_info" },
	{ DW_AT_namelist_item,		"DW_AT_namelist_item" },
	{ DW_AT_priority,		"DW_AT_priority" },
	{ DW_AT_segment,		"DW_AT_segment" },
	{ DW_AT_specification,		"DW_AT_specification" },
	{ DW_AT_static_link,		"DW_AT_static_link" },
	{ DW_AT_type,			"DW_AT_type" },
	{ DW_AT_use_location,		"DW_AT_use_location" },
	{ DW_AT_variable_parameter,	"DW_AT_variable_parameter" },
	{ DW_AT_virtuality,		"DW_AT_virtuality" },
	{ DW_AT_vtable_elem_location,	"DW_AT_vtable_elem_location" },
	{ DW_AT_lo_user,		"DW_AT_lo_user" },
	{ DW_AT_SCO_mutable,		"DW_AT_SCO_mutable" },
	{ DW_AT_SCO_namespace,		"DW_AT_SCO_namespace" },
	{ DW_AT_SCO_using,		"DW_AT_SCO_using" },
	{ DW_AT_hi_user,		"DW_AT_hi_user" },
	0
};

const char *
dwarf2_attribute_name(unsigned int attr)
{
	return get_name(attr_names, attr);
}
