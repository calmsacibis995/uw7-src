#ident	"@(#)libdwarf2:common/btype_name.c	1.1"

#include "dwarf2.h"
#include "libdwarf2.h"
#include "Table.h"

static Table base_type_names[] =
{
	{ DW_ATE_address,	"DW_ATE_address" },
	{ DW_ATE_boolean,	"DW_ATE_boolean" },
	{ DW_ATE_complex_float,	"DW_ATE_complex_float" },
	{ DW_ATE_float,		"DW_ATE_float" },
	{ DW_ATE_signed,	"DW_ATE_signed" },
	{ DW_ATE_signed_char,	"DW_ATE_signed_char" },
	{ DW_ATE_unsigned,	"DW_ATE_unsigned" },
	{ DW_ATE_unsigned_char,	"DW_ATE_unsigned_char" },
	{ DW_ATE_lo_user,	"DW_ATE_lo_user" },
	{ DW_ATE_hi_user,	"DW_ATE_hi_user" },
	0
};

const char *
dwarf2_base_type_encoding_name(unsigned int encoding)
{
	return get_name(base_type_names, encoding);
}
