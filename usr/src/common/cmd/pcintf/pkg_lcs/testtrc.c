#ident	"@(#)pcintf:pkg_lcs/testtrc.c	1.1"
/*
 *  Test for conversions
 *
 */

#include <stdio.h>
#include "lcs.h"

main(argc, argv)
int argc;
char **argv;
{
	lcs_tbl it;
	lcs_tbl ot;
	int s1, e1, s2, e2;
	int i, j, k;
	lcs_char c;
	int ret;
	unsigned char ibuf[10];
	unsigned char buf[32];
	int mode = LCS_MODE_NO_MULTIPLE;
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
			case 'c': case 'C':
				mode |= LCS_MODE_USER_CHAR;
				break;
			}
		}
	}

	if (argc < 3) {
		printf("Usage: testtrc [-cms] in_tbl out_tbl [s1 e1 [s2 e2]]\n");
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
	argc -= 2;
	argv += 2;
	if (argc >= 3) {
		sscanf(argv[1], "%x", &s1);
		sscanf(argv[2], "%x", &e1);
		argc -= 2;
		argv += 2;
	} else {
		s1 = 0;
		e1 = 0xff;
	}
	if (argc >= 3) {
		sscanf(argv[1], "%x", &s2);
		sscanf(argv[2], "%x", &e2);
		argc -= 2;
		argv += 2;
	} else {
		s2 = 0;
		e2 = 0;
	}

	for (i = s1; i <= e1; i++) {
	    ibuf[0] = i;
	    for (j = s2; j <= e2; j++) {
		ibuf[1] = j;
		lcs_errno = 0;
		ret = lcs_convert_in(&c, 0, (char *)ibuf, 2);
		printf("%02x %02x -> %04x  ret %d err %d  ",i,j,c,ret,lcs_errno);
		ret = lcs_convert_out((char *)buf, 32, &c, 1);
		printf("  %04x -> ", c);
		for (k = 0; k < ret; k++)
			printf("%02x ", buf[k]);
		printf(" ret %d err %d\n", ret, lcs_errno);
	    }
	}
}
