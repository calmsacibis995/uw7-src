#ident	"@(#)libdwarf2:common/lang_name.c	1.1"

#include "dwarf2.h"
#include "libdwarf2.h"
#include "Table.h"

static Table lang_names[] =
{
	{ DW_LANG_C89,		"DW_LANG_C89" },
	{ DW_LANG_C,		"DW_LANG_C" },
	{ DW_LANG_Ada83,	"DW_LANG_Ada83" },
	{ DW_LANG_C_plus_plus,	"DW_LANG_C_plus_plus" },
	{ DW_LANG_Cobol74,	"DW_LANG_Cobol74" },
	{ DW_LANG_Cobol85,	"DW_LANG_Cobol85" },
	{ DW_LANG_Fortran77,	"DW_LANG_Fortran77" },
	{ DW_LANG_Fortran90,	"DW_LANG_Fortran90" },
	{ DW_LANG_Pascal83,	"DW_LANG_Pascal83" },
	{ DW_LANG_Modula2,	"DW_LANG_Modula2" },
	{ DW_LANG_lo_user,	"DW_LANG_lo_user" },
	{ DW_LANG_hi_user,	"DW_LANG_hi_user" },
	0
};

const char *
dwarf2_language_name(unsigned int lang)
{
	return get_name(lang_names, lang);
}
