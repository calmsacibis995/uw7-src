#ident	"@(#)libdwarf2:common/form_name.c	1.1"

#include "dwarf2.h"
#include "libdwarf2.h"
#include "Table.h"

static Table form_names[] =
{
	{ DW_FORM_addr,		"DW_FORM_addr" },
	{ DW_FORM_block2,	"DW_FORM_block2" },
	{ DW_FORM_block4,	"DW_FORM_block4" },
	{ DW_FORM_data2,	"DW_FORM_data2" },
	{ DW_FORM_data4,	"DW_FORM_data4" },
	{ DW_FORM_data8,	"DW_FORM_data8" },
	{ DW_FORM_string,	"DW_FORM_string" },
	{ DW_FORM_block,	"DW_FORM_block" },
	{ DW_FORM_block1,	"DW_FORM_block1" },
	{ DW_FORM_data1,	"DW_FORM_data1" },
	{ DW_FORM_flag,		"DW_FORM_flag" },
	{ DW_FORM_sdata,	"DW_FORM_sdata" },
	{ DW_FORM_strp,		"DW_FORM_strp" },
	{ DW_FORM_udata,	"DW_FORM_udata" },
	{ DW_FORM_ref_addr,	"DW_FORM_ref_addr" },
	{ DW_FORM_ref1,		"DW_FORM_ref1" },
	{ DW_FORM_ref2,		"DW_FORM_ref2" },
	{ DW_FORM_ref4,		"DW_FORM_ref4" },
	{ DW_FORM_ref8,		"DW_FORM_ref8" },
	{ DW_FORM_ref_udata,	"DW_FORM_ref_udata" },
	{ DW_FORM_indirect,	"DW_FORM_indirect" },
	0
};

const char *
dwarf2_form_name(unsigned int form)
{
	return get_name(form_names, form);
}
