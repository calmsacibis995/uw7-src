#ident	"@(#)kern-i386:util/kdb/scodb/alias.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
*	To test out getalias(), compile this file
*	with -DSTDALONE.
*
*	Standalone support is at end of the file.
*/

#include	"dbg.h"
#include	"histedit.h"
#include	"alias.h"

#ifndef STDALONE
extern struct alias scodb_alias[];
extern int scodb_nalias;
#else
# define	NALIAS	16
STATIC struct alias scodb_alias[NALIAS] = {
	{ "x",  "z a b			" },
	{ "z",	"y a!1 b!2		" },
	{ "y",	"/x !^ !* !$		" },
};
STATIC int scodb_nalias = NALIAS;
#endif

#define		BAD_SELECTOR()			\
	printf("bad ! selector in alias \"%s\" (\"%s\")\n", func, *v)

/*
*	do alias resolution
*/
NOTSTATIC
getalias(ac, av)
	int *ac;
	char ***av;
{
	int i, j;
	int hadsub, argn, first = 1;
	int nal, inc, onc;
	char *func, **v;
	char *bfp;
	char **ivc, **ovc, *obf;
	static char tbf[2][DBIBFL*2];
	static char *newvec[2][DBNARG];
	char *s, *ss;

	ivc = *av;
	inc = *ac;
	for (nal = 0;nal < ALIASLOOP;nal++) {
		hadsub = 0;
		if (!first) {
			/* last output becomes input */
			inc = onc;
			ivc = ovc;
		}
		else
			first = 0;
		func = ivc[0];
		if (*func == NOALIAS) {
			++ivc[0];
			if (!nal)	/* bump so substitution is done later */
				++nal;
			break;
		}
		/* set output to be whatever input isn't */
		if (ivc == newvec[0]) {
			ovc = newvec[1];
			obf = tbf[1];
		}
		else {
			ovc = newvec[0];
			obf = tbf[0];
		}
		for (i = 0;i < scodb_nalias;i++) {
			if (!strcmp(func, scodb_alias[i].al_name)) {
				onc = 0;
				bfp = obf;
				for (v = scodb_alias[i].al_vec;*v;v++) {
					if (bfp - obf > DBIBFL) {
tlong:						printf("Alias substitution too long.\n");
						return 0;
					}
					if (v != scodb_alias[i].al_vec)
						*bfp++ = ' ';
					s = *v;
					while (*s) {
						if (*s == SUBARG) {
							++hadsub;
							++s;
							if (*s == '^') {
								argn = 1;
								++s;
								goto onearg;
							}
							else if (*s == '$') {
								argn = inc - 1;
								++s;
								goto onearg;
							}
							else if (*s == '*') {
								++s;
								for (i = 1;i < inc;i++) {
									if (i != 1)
										*bfp++ = ' ';
									for (ss = ivc[i];*ss;ss++)
										*bfp++ = *ss;
								}
							}
							else if (numer(*s)) {
								argn = 0;
								while (numer(*s)) {
									argn *= 10;
									argn += *s - '0';
									++s;
								}
							onearg:	if (argn >= inc) {
									BAD_SELECTOR();
									return 0;
								}
								for (ss = ivc[argn];*ss;ss++)
									*bfp++ = *ss;
							}
						}
						else
							*bfp++ = *s++;
					}
				}
				if (!hadsub) {
					/* tack on other arguments */
					for (j = 1;j < inc;j++) {
						*bfp++ = ' ';
						for (ss = ivc[j];*ss;ss++)
							*bfp++ = *ss;
					}
				}
				*bfp = '\0';
				/* vectorize substituted stuff */
				onc = 0;
				s = obf;
				while (*s) {
					while (white(*s))
						++s;
					if (!*s)
						break;
					ovc[onc++] = s;
					for (;;) {
						if (!*s)
							break;
						if (white(*s)) {
							*s++ = '\0';
							break;
						}
						++s;
					}
				}
				if (bfp - obf > DBIBFL)
					goto tlong;
				ovc[onc] = 0;
				break;
			}
		}
		if (i == scodb_nalias) {
			/* no alias found */
			break;
		}
	}
	if (nal == ALIASLOOP) {
		printf("alias loop.\n");
		return 0;
	}
	if (nal) {
		*av = ivc;
		*ac = inc;
	}
	return 1;
}

#ifndef STDALONE

NOTSTATIC
c_alias(c, v)
	int c;
	char **v;
{
	int i;
	char *s;
	char **nv;
	struct alias *al = 0;

	if (c == 1) {
		for (i = 0;i < scodb_nalias;i++)
			if (scodb_alias[i].al_name[0]) {
				prst(scodb_alias[i].al_name, ALNAMEL+2);
				for (nv = scodb_alias[i].al_vec;*nv;nv++) {
					if (nv != scodb_alias[i].al_vec)
						putchar(' ');
					printf(*nv);
				}
				putchar('\n');
			}
	}
	else if (c == 2) {
		for (i = 0;i < scodb_nalias;i++)
			if (!strcmp(v[1], scodb_alias[i].al_name)) {
				prst(scodb_alias[i].al_name, ALNAMEL+2);
				for (nv = scodb_alias[i].al_vec;*nv;nv++) {
					if (nv != scodb_alias[i].al_vec)
						putchar(' ');
					printf(*nv);
				}
				putchar('\n');
			}
	}
	else {
		if (!strcmp(v[1], "alias") || !strcmp(v[1], "unalias")) {
			printf("Too dangerous to alias that.\n");
			return DB_ERROR;
		}
		for (i = 0;i < scodb_nalias;i++) {
			if (scodb_alias[i].al_name[0] == 0)
				al = &scodb_alias[i];
			if (!strcmp(v[1], scodb_alias[i].al_name))
				break;
		}
		if (i == scodb_nalias) {
			if (al == 0) {
				printf("no more slots for aliases (%d max).\n", scodb_nalias);
				return DB_ERROR;
			}
		}
		else
			al = &scodb_alias[i];
		strcpy(al->al_name, v[1]);
		s = al->al_buf;
		nv = al->al_vec;
		++v;
		while (*++v) {
			*nv++ = s;
			strcpy(s, *v);
			s += strlen(*v) + 1;
		}
		*nv = 0;
	}
	return DB_CONTINUE;
}

NOTSTATIC
c_unalias(c, v)
	int c;
	char **v;
{
	int i;

	while (*++v) {
		if (**v == '*' && !(*v)[1]) {
			if (do_all(0, "Clear all aliases? ")) {
				for (i = 0;i < scodb_nalias;i++)
					scodb_alias[i].al_name[0] = 0;
				break;
			}
			else
				continue;
		}
		else {
			for (i = 0;i < scodb_nalias;i++)
				if (!strcmp(*v, scodb_alias[i].al_name)) {
					scodb_alias[i].al_name[0] = 0;
					break;
				}
			/* don't bitch if not found... */
		}
	}
	return DB_CONTINUE;
}

#endif /* STDALONE */

#ifdef STDALONE
main() {
	static char *vect[] = {
		"x",
		"1",
		"2",
		"3",
		0
	};
	int i;
	int c = NMEL(vect) - 1;
	char **v = vect;

	printf("Input vector:\t");
	for (i = 0;i < c;i++) printf(" (%s)", v[i]); printf("\n");
	getalias(&c, &v);
	printf("Output vector:\t");
	for (i = 0;i < c;i++) printf(" (%s)", v[i]); printf("\n");
}

#endif
