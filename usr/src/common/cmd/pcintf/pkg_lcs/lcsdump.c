#ident	"@(#)pcintf:pkg_lcs/lcsdump.c	1.1.1.2"
/* SCCSID(@(#)lcsdump.c	7.2	LCC)	* Modified: 22:10:10 11/19/90 */
/*
 *  lcsdump - dump a compiled character set
 */

#define NO_LCS_EXTERNS

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "lcs.h"
#include "lcs_int.h"

struct table_header *lcs_get_table();

struct table_header *tbl;

char *substr();


main(argc, argv)
int argc;
char **argv;
{
	struct input_header *ih;
	struct output_header *oh;
	char *ptr;

	if (argc != 2) {
		fprintf(stderr, "usage: lcsdump table\n");
		exit(1);
	}

	/* NULL out the ".lcs" extender if it is given 
	 * this is because the lcs library routine automatically
	 * appends a ".lcs" extension.
	 */
	if ((ptr = substr(*++argv, ".lcs")) != NULL)
		*ptr = '\0';

	if ((tbl = lcs_get_table(*argv, NULL)) == NULL) {
		fprintf(stderr, "lcsdump: table %s not found; lcs_errno = %d\n",
			*argv, lcs_errno);
		exit(2);
	}
	printf("$\tTable %s\n\n", *argv);
	printf("$hexadecimal\n");
	if (tbl->th_default < 0x100)
		printf("$default_char %02x\n", tbl->th_default);
	else
		printf("$default_char %02x %02x\n", (tbl->th_default & 0xff),
			((tbl->th_default >> 8) & 0xff));
	if (tbl->th_input != NULL) {
		printf("\n$input\n");
		for (ih = tbl->th_input; ih; ih = ih->ih_next)
			print_input_table(ih);
	}
	if (tbl->th_output != NULL) {
		printf("\n$output\n");
		for (oh = tbl->th_output; oh; oh = oh->oh_next)
			print_output_table(oh);
	}
}

print_input_table(ih)
struct input_header *ih;
{
	int i, size;
	unsigned char jj, kk;
	struct input_dead *dp;

	printf("%02x-%02x\t", ih->ih_start_code, ih->ih_end_code);
	if (ih->ih_flags & IH_DEAD_CHAR) {
		printf("dead_char");
		if (ih->ih_char_bias)
			printf(" char_bias %04x ", ih->ih_char_bias);
		printf("\n");
		dp = (struct input_dead *)ih->ih_table;
		for (i = ih->ih_length; i > 0; i -= sizeof(struct input_dead)) {
			printf("\t%02x  %04x\n", dp->id_code, dp->id_value);
			dp++;
		}
		printf("\n");
		return;
	}
	if (ih->ih_flags & IH_DIRECT)
		printf("direct");
	else
		printf("table");
	if (ih->ih_flags & IH_DOUBLE_BYTE) {
		printf(" double_byte %02x-%02x", ih->ih_db_start,ih->ih_db_end);
		size = ih->ih_db_end - ih->ih_db_start + 1;
	}
	if (ih->ih_char_bias)
		printf(" char_bias %04x ", ih->ih_char_bias);
	printf("\n");
	if ((ih->ih_flags & IH_DIRECT) == 0) {
		for (jj = ih->ih_db_start; jj <= ih->ih_db_end; jj++) {
			for (kk = ih->ih_start_code;
			     kk <= ih->ih_end_code; kk++) {
				if (ih->ih_flags & IH_DOUBLE_BYTE)
					printf("\t%02x %02x  %04x\n", kk, jj,
					    ih->ih_table[(jj - ih->ih_db_start)*
						size + kk - ih->ih_start_code]);
				else
					printf("\t%02x  %04x\n", kk,
					    ih->ih_table[kk-ih->ih_start_code]);
			}
		}
		printf("\n");
	}
}


print_output_table(oh)
struct output_header *oh;
{
	int size;
	unsigned short ii;
	struct output_table *ot;

	printf("%04x-%04x\t", oh->oh_start_code, oh->oh_end_code);
	if (oh->oh_flags & OH_DIRECT_CELL)
		printf("direct_cell ");
	if (oh->oh_flags & OH_DIRECT_ROW)
		printf("direct_row ");
	if (oh->oh_flags & OH_TABLE_4B)
		printf("table_4b ");
	if ((oh->oh_flags & (OH_DIRECT_CELL|OH_DIRECT_ROW|OH_TABLE_4B)) == 0)
		printf("table ");
	if (oh->oh_flags & OH_NO_LOWER)
		printf("no_lower ");
	if (oh->oh_flags & OH_NO_UPPER)
		printf("no_upper ");
	if (oh->oh_char_bias)
		printf("char_bias %04x ", oh->oh_char_bias);
	printf("\n");
	if (oh->oh_flags & (OH_DIRECT_CELL|OH_DIRECT_ROW))
		return;

	size = (oh->oh_flags & OH_TABLE_4B) ? 4 : 2;
	ot = (struct output_table *)&oh->oh_table[0];
	for (ii = oh->oh_start_code; ii <= oh->oh_end_code; ii++) {
		if (((oh->oh_flags & OH_NO_UPPER) && (ii & 0x80)) ||
		    ((oh->oh_flags & OH_NO_LOWER) && ((ii & 0x80) == 0)))
			continue;
		printf("\t%04x  %02x\t", ii, ot->ot_char);
		if (ot->ot_flags & OT_NOT_EXACT)
			printf("not_exact ");
		if (ot->ot_flags & OT_HAS_MULTI) {
			printf("has_multi ");
			print_multi(ii);
		}
		if (ot->ot_flags & OT_HAS_2B)
			printf("has_2b %02x %02x", ot->ot_2chars[0],
						   ot->ot_2chars[1]);
		printf("\n");
		if (size == 2)
			ot = (struct output_table *)&ot->ot_2chars[0];
		else
			ot++;
	}
}

print_multi(code)
int code;
{
	struct multi_byte *mb;
	unsigned char *mbp;
	int len;
	unsigned short ii;

	mbp = tbl->th_multi_byte;
	len = tbl->th_multi_length;
	while (len > 0) {
		mb = (struct multi_byte *)mbp;
		mbp += mb->mb_len;
		if (code != mb->mb_code)
			continue;
		for (ii = 0; ii < mb->mb_length; ii++)
			printf("%02x ", mb->mb_text[ii]);
		return;
	}
}

/*
 * substr -
 *	return pointer to substr of string1
 *	containing n chars in order in string2
 */
char *
substr(string1, string2, n)
char *string1;
char *string2;
int n;
{
	char *matcher, *pointer;

	/* start at beginning of string1
	 * and course through the string
	 * searching for 1st char of string2
	 */
	pointer = string1;
	while(matcher = strchr(pointer, string2[0])) {
		if ( !strncmp(matcher, string2, n))
			return matcher;
		else {
			/* set pointer to character beyond strchr match */
			pointer = ++matcher;
		}
	}
	return (char *) NULL;
}
