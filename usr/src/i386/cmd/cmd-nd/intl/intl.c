#ident "@(#)intl.c	25.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <nl_types.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <locale.h>

#define CATNAME "lli.cat@sa"
extern int set_id;
static nl_catd catd = (nl_catd)-2;

catfprintf(FILE *f, int msg_id, char *def,
	int a, int b, int c, int d, int e, int g)
{
	char *s;

	if (catd == (nl_catd)-2) {
		setlocale(LC_ALL, "");
		catd=catopen(CATNAME, NL_CAT_LOCALE);
		if (catd == (nl_catd)-1 && errno != ENOENT) {
			fprintf(stderr,"lli.cat: Unable to open message catalogue (%s)\n", CATNAME);
			perror("lli.cat");
		}
	}
	if (catd == (nl_catd)-1) {
		fprintf(f, def, a,b,c,d,e, g);
		return;
	}
	s = catgets(catd, set_id, msg_id, def);
	fprintf(f, s, a,b,c,d,e, g);
}
