#ident	"@(#)pcintf:pkg_lcs/t_block.c	1.2"
/* SCCSID(@(#)t_block.c	7.1	LCC)	* Modified: 15:35:28 10/15/90 */

/*
 *  lcs_translate_block(out, out_size, in, in_len)
 */

#include "lcs.h"
#include "lcs_int.h"


lcs_translate_block(out, out_size, in, in_len)
char *out;
int out_size;
char *in;
int in_len;
{
	lcs_char canon;
	int r, res;
	int ib;

	lcs_errno = lcs_input_bytes_processed =
	lcs_output_bytes_processed = lcs_exact_translations =
	lcs_multiple_translations = lcs_best_single_translations =
	lcs_user_default_translations = 0;

	if (in_len <= 0)
		return 0;

	res = 0;
	while (lcs_input_bytes_processed != in_len) {
		ib = lcs_input_bytes_processed;
		if ((r = lcs_convert_in(&canon, 0,
					&in[lcs_input_bytes_processed],
					in_len-lcs_input_bytes_processed)) < 0)
			return r;

		if (r) {
			if (lcs_mode & LCS_MODE_UPPERCASE)
				canon = lcs_toupper(canon);
			if (lcs_mode & LCS_MODE_LOWERCASE)
				canon = lcs_tolower(canon);

			if ((r = lcs_convert_out(out, out_size, &canon, r)) < 0)
				return r;
			res += r;
			out += r;
			out_size -= r;
		}
		if (lcs_errno) {
			lcs_input_bytes_processed = ib;
			return -1;
		}
	}
	return res;
}
