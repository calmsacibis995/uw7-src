#ident	"@(#)pcintf:pkg_lcs/prim.c	1.1.1.2"
/* SCCSID(@(#)prim.c	7.2	LCC)	* Modified: 22:05:58 11/19/90 */
/*
 *  Primitive translation routines
 */

#define NO_LCS_EXTERNS

#include <fcntl.h>
#include "lcs.h"
#include "lcs_int.h"

int lcs_input_bytes_processed;

/*
 *  lcs_convert_in(out, out_size, in, in_len)
 *
 *  Converts from the input table into lcs internal.
 *  Output is placed in out for at most out_size lcs_chars.  If out_size is 0,
 *    one lcs_char will be processed.
 *  Input comes from in.  in_len contains the number of bytes, or 0 to indicate
 *    a string input.  When a string is input, a lcs_null will be placed in out.
 *
 *  Returns the number of lcs_chars in out, or -1 to indicate a fatal error.
 *  If there is only part of a multi-byte input char, lcs_errno will be set to
 *    LCS_ERR_INPUT_SPLIT.
 */

lcs_convert_in(out, out_size, in, in_len)
lcs_char *out;
int out_size;
char *in;
int in_len;
{
    register struct input_header *ih;
    struct input_dead *id;
    int ret;
    unsigned char b1, b2;
    lcs_char c;
    int db_size;
    int l;

    if (lcs_input_table == NULL ||
	strcmp(lcs_input_table->th_magic, LCS_MAGIC) ||
	lcs_input_table->th_input == NULL) {
	    lcs_errno = LCS_ERR_NOTABLE;
	    return -1;
    }
    ret = 0;
    if (in_len == 0) {
	in_len--;
	if (out_size == 1) {
	    *out = lcs_ascii('\0');
	    return ret;
	}
    }

    while (1) {
	if (in_len < 0 && *in == '\0' && out_size) {
	    *out = lcs_ascii('\0');
	    return ret;
	}
	b1 = *in++;
	b2 = *in;
	if (in_len > 0)
	    in_len--;
	for (ih = lcs_input_table->th_input; ih; ih = ih->ih_next) {
	    if (b1 >= ih->ih_start_code && b1 <= ih->ih_end_code) {
		if ((ih->ih_flags & (IH_DOUBLE_BYTE|IH_DEAD_CHAR)) &&
		    (in_len == 0 || ((in_len < 0) && b2 == '\0'))) {
			lcs_errno = LCS_ERR_INPUT_SPLIT;
			return ret;
		}
		if ((ih->ih_flags & IH_DOUBLE_BYTE) &&
		    (b2 < ih->ih_db_start || b2 > ih->ih_db_end))
			continue;
		if (ih->ih_flags & IH_DIRECT) {
		    if (ih->ih_flags & IH_DOUBLE_BYTE)
			c = (b1 << 8) + b2 + ih->ih_char_bias;
		    else
			c = b1 + ih->ih_char_bias;
		} else if (ih->ih_flags & IH_DEAD_CHAR) {
		    id = (struct input_dead *)ih->ih_table;
		    for (l = ih->ih_length; l > 0;
			 l -= sizeof(struct input_dead)) {
			if (id->id_code == b2)
			    break;
			id++;
		    }
		    if (l <= 0)
			continue;
		    c = id->id_value;
		} else {	/* table */
		    if (ih->ih_flags & IH_DOUBLE_BYTE) {
		        db_size = ih->ih_end_code - ih->ih_start_code + 1;
			c = ih->ih_table[(b2 - ih->ih_db_start)*db_size +
					 b1 - ih->ih_start_code];
		    } else
			c = ih->ih_table[b1 - ih->ih_start_code];
		}
		if (ih->ih_flags & (IH_DOUBLE_BYTE|IH_DEAD_CHAR)) {
		    in++;
		    if (in_len > 0)
			in_len--;
		    lcs_input_bytes_processed++;
		}
		break;
	    }
	}
	if (ih == NULL) {
	    lcs_errno = LCS_ERR_BADTABLE;
	    return -1;
	}
	ret++;
	*out++ = c;
	out_size--;
	lcs_input_bytes_processed++;
	if (out_size <= 0 || in_len == 0)
	    return ret;
	if (in_len < 0 && out_size == 1) {
	    *out = lcs_ascii('\0');
	    return ret;
	}
    }
}


int	 lcs_exact_translations;
int	 lcs_multiple_translations;
int	 lcs_best_single_translations;
int	 lcs_user_default_translations;
int	 lcs_output_bytes_processed;

short	 lcs_mode = 0;
lcs_char lcs_user_char;
short	 lcs_country;


#define LCS_EXACT	0x01
#define LCS_MULTI	0x02
#define LCS_BEST	0x04
#define LCS_USER	0x08
 
/*
 *  lcs_convert_out(out, out_size, in, in_len)
 *
 *  Converts from lcs internal to the output table.
 *  Output is placed in out for at most out_size bytes.
 *  Input comes from in.  in_len contains the number of lcs_chars, or 0 to
 *    indicate a string input.  When a string is input, a null will be placed
 *    in out.
 *
 *  Returns the number of bytes in out, or -1 to indicate a fatal error.
 */

lcs_convert_out(out, out_size, in, in_len)
char *out;
int out_size;
lcs_char *in;
int in_len;
{
    register struct output_header *oh;
    struct output_table *ot;
    struct multi_byte *mb;
    unsigned char *mbp;
    unsigned char b1, b2;
    int ret;
    lcs_char c;
    int i, l, len;
    int stats;

    if (lcs_output_table == NULL ||
	strcmp(lcs_output_table->th_magic, LCS_MAGIC) ||
	lcs_output_table->th_output == NULL) {
	    lcs_errno = LCS_ERR_NOTABLE;
	    return -1;
    }

    ret = 0;
    if (in_len == 0) {
	in_len--;
	out_size--;
    }
    if (out_size < 0) {
	lcs_errno = LCS_ERR_NOSPACE;
	return ret;
    }
    stats = 0;

    while (1) {
	if (in_len < 0 && *in == lcs_ascii('\0')) {
	    *out = '\0';
	    return ret;
	}
	c = *in++;
	if (in_len > 0)
	    in_len--;

	l = 1;
	for (oh = lcs_output_table->th_output; oh; oh = oh->oh_next) {
	    if (c >= oh->oh_start_code && c <= oh->oh_end_code) {
		if (((oh->oh_flags & OH_NO_LOWER) && ((c & 0x80) == 0)) ||
		    ((oh->oh_flags & OH_NO_UPPER) && (c & 0x80)))
			continue;
		if (oh->oh_flags & OH_DIRECT_CELL) {
		    stats |= LCS_EXACT;
		    if (oh->oh_flags & OH_DIRECT_ROW) {
			b1 = ((unsigned int)(c + oh->oh_char_bias) >> 8) & 0xff;
			b2 = (c + oh->oh_char_bias) & 0xff;
			l = 2;
		    } else 
			b1 = (c + oh->oh_char_bias) & 0xff;
		} else {	/* table */
		    if ((oh->oh_flags & (OH_NO_LOWER|OH_NO_UPPER)) &&
			((c & 0xff00) != (oh->oh_start_code & 0xff00)))
			i = ((((c >> 8) - (oh->oh_start_code >> 8)) << 7) +
			     (c & 0xff) - (oh->oh_start_code & 0xff)) << 1;
		    else
			i = (c - oh->oh_start_code) << 1;

		    ot = (struct output_table *)oh; /* KLUDGE for MSC 5.1 L */
		    if (oh->oh_flags & OH_TABLE_4B) {
			ot = (struct output_table *)&oh->oh_table[i << 1];
			if (ot->ot_flags & OT_HAS_2B) {
			    stats |= LCS_EXACT;
			    b1 = ot->ot_2chars[0];
			    b2 = ot->ot_2chars[1];
			    l = 2;
			    break;
			}
		    } else
			ot = (struct output_table *)&oh->oh_table[i];

		    if (ot->ot_flags & OT_NOT_EXACT) {
			if (lcs_mode & LCS_MODE_STOP_XLAT) {
			    if (in_len < 0)
				*out = '\0';
			    lcs_errno = LCS_ERR_STOPXLAT;
			    return ret;
			}
			if ((lcs_mode & LCS_MODE_NO_MULTIPLE) == 0 &&
			    (ot->ot_flags & OT_HAS_MULTI)) {
			    stats |= LCS_MULTI;
			    mbp = lcs_output_table->th_multi_byte;
			    len = lcs_output_table->th_multi_length;
			    while (len > 0) {
				mb = (struct multi_byte *)mbp;
				mbp += mb->mb_len;
				len -= mb->mb_len;
				if (c == mb->mb_code)
				    break;
			    }
			    if (len > 0) {
				if (mb->mb_length <= 2) {
				    b1 = mb->mb_text[0];
				    b2 = mb->mb_text[1];
				    l = mb->mb_length;
				    break;
				}
				if (out_size < (int)(mb->mb_length)) {
				    if (in_len < 0)
					*out = '\0';
				    lcs_errno = LCS_ERR_NOSPACE;
				    return ret;
				}
				mbp = mb->mb_text;
				for (i = 1; i < (int)(mb->mb_length); i++)
				    *out++ = *mbp++;
				out_size -= mb->mb_length - 1;
				ret += mb->mb_length - 1;
				lcs_output_bytes_processed += mb->mb_length - 1;
				b1 = *mbp;
			    }
			    break;
			}
			if (lcs_mode & LCS_MODE_USER_CHAR) {
			    stats |= LCS_USER;
			    c = lcs_user_char;
			    if (c & 0xff00) {
				b1 = (c >> 8) & 0xff;
				b2 = c & 0xff;
				l = 2;
			    } else
				b1 = c & 0xff;
			} else {
			    stats |= LCS_BEST;
			    b1 = ot->ot_char;
			}
		    } else {
			stats |= LCS_EXACT;
			b1 = ot->ot_char;
		    }
		}
		break;
	    }
	}
	if (oh == NULL) {
	    if (lcs_mode & LCS_MODE_STOP_XLAT) {
		if (in_len < 0)
		    *out = '\0';
		lcs_errno = LCS_ERR_STOPXLAT;
		return ret;
	    }
	    if (lcs_mode & LCS_MODE_USER_CHAR) {
		stats |= LCS_USER;
		c = lcs_user_char;
	    } else {
		stats |= LCS_BEST;
		c = lcs_output_table->th_default;
	    }
	    if (c & 0xff00) {
		b1 = (c >> 8) & 0xff;
		b2 = c & 0xff;
		l = 2;
	    } else
		b1 = c & 0xff;
	}
	if (out_size < l) {
	    if (in_len < 0)
		*out = '\0';
	    lcs_errno = LCS_ERR_NOSPACE;
	    return ret;
	}
	*out++ = b1;
	if (l == 2)
	    *out++ = b2;
	ret += l;
	lcs_output_bytes_processed += l;
	if (stats & LCS_EXACT)
	    lcs_exact_translations++;
	if (stats & LCS_MULTI)
	    lcs_multiple_translations++;
	if (stats & LCS_USER)
	    lcs_user_default_translations++;
	if (stats & LCS_BEST)
	    lcs_best_single_translations++;
	stats = 0;
	    
	if ((out_size -= l) == 0) {
	    if (in_len < 0)
		*out = '\0';
	    if (in_len > 0 || (in_len < 0 && *in != lcs_ascii('\0'))) {
		lcs_errno = LCS_ERR_NOSPACE;
		return ret;
	    }
	}
	if (in_len == 0)
	    return ret;
    }
}
