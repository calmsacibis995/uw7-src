#ident	"@(#)pcintf:pkg_lcs/set_tab.c	1.2"
/* SCCSID(@(#)set_tab.c	7.1	LCC)	* Modified: 15:34:48 10/15/90 */
/*
 *  lcs_set_tables(out_tbl, in_tbl)
 *
 *  Set translation tables
 */

#define NO_LCS_EXTERNS

#include <fcntl.h>
#include "lcs.h"
#include "lcs_int.h"


lcs_set_tables(out_tbl, in_tbl)
lcs_tbl out_tbl;
lcs_tbl in_tbl;
{
	if (strcmp(out_tbl->th_magic, LCS_MAGIC) ||
	    strcmp(in_tbl->th_magic, LCS_MAGIC)) {
		lcs_errno = LCS_ERR_BADTABLE;
		return -1;
	}
	lcs_output_table = out_tbl;
	lcs_input_table = in_tbl;
	return 0;
}


/*
 *  lcs_set_options(mode, user_char, country)
 *
 *  Set translation options
 */

lcs_set_options(mode, user_char, country)
short mode;
unsigned short user_char;
short country;
{
	lcs_mode = mode;
	lcs_user_char = user_char;
	lcs_country = country;
	return 0;
}
