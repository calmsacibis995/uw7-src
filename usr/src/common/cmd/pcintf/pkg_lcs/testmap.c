#ident	"@(#)pcintf:pkg_lcs/testmap.c	1.1"
/*
 *  Test for conversions
 *
 */

#include <stdio.h>
#include "lcs.h"

char ibuf[256];
char obuf[2048];


main(argc, argv)
int argc;
char **argv;
{
	lcs_tbl it;
	lcs_tbl ot;
	register int i;
	int ret;
	int mode = LCS_MODE_NO_MULTIPLE;
	short def_ch;
	char *arg;

	while (argc > 3) {
		if (argv[1][0] != '-')
			break;
		arg = *++argv;
		argc--;
		while (*++arg) {
			switch (*arg) {
			case 'm': case 'M':
				mode &= ~LCS_MODE_NO_MULTIPLE;
				break;
			case 's': case 'S':
				mode |= LCS_MODE_STOP_XLAT;
				break;
			case 'u': case 'U':
				mode |= LCS_MODE_UPPERCASE;
				break;
			case 'l': case 'L':
				mode |= LCS_MODE_LOWERCASE;
				break;
			case 'c': case 'C':
				mode |= LCS_MODE_USER_CHAR;
				break;
			}
		}
	}
	if (argc != 3) {
		printf("Usage: testmap [-clmsu] in_tbl out_tbl\n");
		exit(1);
	}
	if ((it = lcs_get_table(argv[1], NULL)) == NULL) {
		printf("t: couldn't load table %s\n", argv[1]);
		exit(1);
	}
	if ((ot = lcs_get_table(argv[2], NULL)) == NULL) {
		printf("t: couldn't load table %s\n", argv[2]);
		exit(1);
	}
	lcs_set_tables(ot, it);
	lcs_set_options(mode, '*', 1);
	for (i = 0; i <= 0xff; i++)
		ibuf[i] = i;
	printf("exact %d, multi %d, best %d, user %d, input %d, output %d\n",
		lcs_exact_translations, 
		lcs_multiple_translations, 
		lcs_best_single_translations, 
		lcs_user_default_translations, 
		lcs_input_bytes_processed,
		lcs_output_bytes_processed);
	ret = lcs_translate_block(obuf, 2048, ibuf, 256);
	for (i = 0; i < lcs_output_bytes_processed; i++) {
		printf(" %02x", obuf[i]);
		if ((i & 0x0f) == 0x0f)
			printf("\n");
	}
	printf("\nret %d, errno %d\n", ret, lcs_errno);
	printf("exact %d, multi %d, best %d, user %d, input %d, output %d\n",
		lcs_exact_translations, 
		lcs_multiple_translations, 
		lcs_best_single_translations, 
		lcs_user_default_translations, 
		lcs_input_bytes_processed,
		lcs_output_bytes_processed);
}
