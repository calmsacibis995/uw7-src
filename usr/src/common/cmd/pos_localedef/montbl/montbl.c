/*	copyright	"%c%"	*/

#ident	"@(#)pos_localedef:common/cmd/pos_localedef/montbl/montbl.c	1.2.6.4"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <limits.h>

#define SKIPWHITE(ptr)	while(isspace(*ptr)) ptr++

#define MONSIZ	15		/* number of lconv members taken care by montbl */
#define NUMSTR	7		/* number of string members */
#define NUMCHAR	8		/* number of char members */

#define INT_CURR_SYMBOL		0
#define CURRENCY_SYMBOL		1
#define MON_DECIMAL_POINT	2
#define MON_THOUSANDS_SEP	3
#define MON_GROUPING		4
#define POSITIVE_SIGN		5
#define NEGATIVE_SIGN		6
#define INT_FRAC_DIGITS	0
#define FRAC_DIGITS	1
#define P_CS_PRECEDES	2
#define P_SEP_BY_SPACE	3
#define N_CS_PRECEDES	4
#define N_SEP_BY_SPACE	5
#define P_SIGN_POSN	6
#define N_SIGN_POSN	7

extern char	*optarg;
extern int	optind;

char	*getstr();
int	getnum();

int	lineno;

/*
 * main - process input file, set values for lconv members, 
 *        and write to output file
 * 
 * If the input file is empty, montbl will set LC_MONETARY
 * with the default values.  All string members have zero length
 * strings and all integer members have CHAR_MAX as described in
 * the montbl(1M) manual page.
 */
main(argc, argv)
int argc;
char **argv;
{
	FILE	*infile, *outfile;
	char	line[BUFSIZ];
	int	i;
	char	*ptr, *endptr;
	int	offset;
	int	c;
	int	errflag = 0;
	char	*outname = NULL;
	static char	*monstrs[NUMSTR] = { "","","","","","","" };
	static char	monchars[NUMCHAR] = { CHAR_MAX,CHAR_MAX,CHAR_MAX,CHAR_MAX,
					      CHAR_MAX,CHAR_MAX,CHAR_MAX,CHAR_MAX };
	static struct lconv	mon = { "","","","","","","","","","",
					CHAR_MAX,CHAR_MAX,CHAR_MAX,CHAR_MAX,
					CHAR_MAX,CHAR_MAX,CHAR_MAX,CHAR_MAX };

	/* Process command line */
	while ((c = getopt(argc, argv, "o:")) != -1) {
		switch(c) {
		case 'o':
			outname = optarg;
			break;
		case '?':
			errflag++;
		}
	}
	if (errflag || optind != argc - 1) {
		fprintf(stderr, "usage: montbl [-o outfile] infile\n");
		exit(1);
	}
	if (outname == NULL)
		outname = "LC_MONETARY";
	if ((infile = fopen(argv[optind], "r")) == NULL) {
		fprintf(stderr, "Cannot open input: %s\n", argv[optind]);
		exit(1);
	}

	/* Process input file */
	i = 0;
	lineno = 0;
	while (fgets(line, BUFSIZ, infile) != NULL) {
		lineno++;
		if (strlen(line) == BUFSIZ-1 && line[BUFSIZ-1] != '\n') {
			fprintf(stderr,"line %d: line too long\n", lineno);
			exit(1);
		}
		ptr = line;
		SKIPWHITE(ptr);
		if (*ptr == '#' || *ptr == '\0')	/* comment or blank line */
			continue;

		if ((endptr = strpbrk(ptr," \t")) == NULL) {
			  	/*
			  	 * Format must have spaces or tabs
				 * between <keyword> and <value>.
				 */
				fprintf(stderr,"line %d: illegal format\n", lineno);
				exit(1);
		}
		*endptr = '\0';
		endptr++;
		SKIPWHITE(endptr);

		if (strcmp(ptr,"int_curr_symbol") == 0) {
			monstrs[INT_CURR_SYMBOL] = getstr(endptr);
		} else if (strcmp(ptr,"currency_symbol") == 0) {
			monstrs[CURRENCY_SYMBOL] = getstr(endptr);
		} else if (strcmp(ptr,"mon_decimal_point") == 0) {
			monstrs[MON_DECIMAL_POINT] = getstr(endptr);
		} else if (strcmp(ptr,"mon_thousands_sep") == 0) {
			monstrs[MON_THOUSANDS_SEP] = getstr(endptr);
		} else if (strcmp(ptr,"mon_grouping") == 0) {
			monstrs[MON_GROUPING] = getstr(endptr);
		} else if (strcmp(ptr,"positive_sign") == 0) {
			monstrs[POSITIVE_SIGN] = getstr(endptr);
		} else if (strcmp(ptr,"negative_sign") == 0) {
			monstrs[NEGATIVE_SIGN] = getstr(endptr);
		} else if (strcmp(ptr,"int_frac_digits") == 0) {
			monchars[INT_FRAC_DIGITS] = getnum(endptr);
		} else if (strcmp(ptr,"frac_digits") == 0) {
			monchars[FRAC_DIGITS] = getnum(endptr);
		} else if (strcmp(ptr,"p_cs_precedes") == 0) {
			monchars[P_CS_PRECEDES] = getnum(endptr);
		} else if (strcmp(ptr,"p_sep_by_space") == 0) {
			monchars[P_SEP_BY_SPACE] = getnum(endptr);
		} else if (strcmp(ptr,"n_cs_precedes") == 0) {
			monchars[N_CS_PRECEDES] = getnum(endptr);
		} else if (strcmp(ptr,"n_sep_by_space") == 0) {
			monchars[N_SEP_BY_SPACE] = getnum(endptr);
		} else if (strcmp(ptr,"p_sign_posn") == 0) {
			monchars[P_SIGN_POSN] = getnum(endptr);
		} else if (strcmp(ptr,"n_sign_posn") == 0) {
			monchars[N_SIGN_POSN] = getnum(endptr);
		} else {
			fprintf(stderr,"line %d: unrecognized keyword\n", lineno);
			exit(1);
		}
	} /* while() */
	fclose(infile);

	/* initialize data structures for output */
	i = 0;
	offset = 0;
	mon.decimal_point = NULL;
	mon.thousands_sep = NULL;
	mon.grouping = NULL;
	mon.int_curr_symbol = (char *)offset;
	offset += strlen(monstrs[i++]) + 1;
	mon.currency_symbol = (char *)offset;
	offset += strlen(monstrs[i++]) + 1;
	mon.mon_decimal_point = (char *)offset;
	offset += strlen(monstrs[i++]) + 1;
	mon.mon_thousands_sep = (char *)offset;
	offset += strlen(monstrs[i++]) + 1;
	mon.mon_grouping = (char *)offset;
	offset += strlen(monstrs[i++]) + 1;
	mon.positive_sign = (char *)offset;
	offset += strlen(monstrs[i++]) + 1;
	mon.negative_sign = (char *)offset;

	i = 0;
	mon.int_frac_digits = monchars[i++];
	mon.frac_digits = monchars[i++];
	mon.p_cs_precedes = monchars[i++];
	if (mon.p_cs_precedes > 1 && mon.p_cs_precedes < CHAR_MAX)
		fprintf(stderr, "Bad value for p_cs_precedes\n");
	mon.p_sep_by_space = monchars[i++];
	if (mon.p_sep_by_space > 1 && mon.p_sep_by_space < CHAR_MAX)
		fprintf(stderr, "Bad value for p_sep_by_space\n");
	mon.n_cs_precedes = monchars[i++];
	if (mon.n_cs_precedes > 1 && mon.n_cs_precedes < CHAR_MAX)
		fprintf(stderr, "Bad value for n_cs_precedes\n");
	mon.n_sep_by_space = monchars[i++];
	if (mon.n_sep_by_space > 1 && mon.n_sep_by_space < CHAR_MAX)
		fprintf(stderr, "Bad value for n_sep_by_space\n");
	mon.p_sign_posn = monchars[i++];
	if (mon.p_sign_posn > 4 && mon.p_sign_posn < CHAR_MAX)
		fprintf(stderr, "Bad value for p_sign_posn\n");
	mon.n_sign_posn = monchars[i];
	if (mon.n_sign_posn > 4 && mon.n_sign_posn < CHAR_MAX)
		fprintf(stderr, "Bad value for n_sign_posn\n");

	/* write out data file */
	if ((outfile = fopen(outname, "w")) == NULL) {
		fprintf(stderr, "Cannot open output file\n");
		exit(1);
	}
	if (fwrite(&mon, sizeof(struct lconv), 1, outfile) != 1) {
		fprintf(stderr, "Cannot write to output file, %s\n", outname);
		exit(1);
	}
	for (i=0; i < NUMSTR; i++) {
		if (fwrite(monstrs[i],strlen(monstrs[i])+1, 1, outfile) != 1) { 
			fprintf(stderr, "Cannot write to output file, %s\n", outname);
			exit(1);
		}
	}
	exit(0);
	/* NOTREACHED */
} /* main() */

/*
 * getstr - get string value after keyword
 */
char *
getstr(ptr)
char *ptr;
{
	char	buf[BUFSIZ];
	char	*nptr;
	int	j;

	if (*ptr++ != '"') {
		fprintf(stderr,"line %d: bad string\n", lineno);
		exit(1);
	}
	j = 0;
	while (*ptr != '"') {
		if (*ptr == '\\') {
			switch (ptr[1]) {
			case '"': buf[j++] = '"'; ptr++; break;
			case 'n': buf[j++] = '\n'; ptr++; break;
			case 't': buf[j++] = '\t'; ptr++; break;
			case 'f': buf[j++] = '\f'; ptr++; break;
			case 'r': buf[j++] = '\r'; ptr++; break;
			case 'b': buf[j++] = '\b'; ptr++; break;
			case 'v': buf[j++] = '\v'; ptr++; break;
			case 'a': buf[j++] = '\7'; ptr++; break;
			case '\\': buf[j++] = '\\'; ptr++; break;
			default:
				if (ptr[1] == 'x') {
					ptr += 2;
					buf[j++] = strtol(ptr, &nptr, 16);
					if (nptr != ptr) {
						ptr = nptr;
						continue;
					} else
						buf[j-1] = 'x';
				} else if (isdigit(ptr[1])) {
					ptr++;
					buf[j++] = strtol(ptr, &nptr, 8);
					if (nptr != ptr) {
						ptr = nptr;
						continue;
					} else
						buf[j-1] = *ptr;
				} else
					buf[j++] = ptr[1];
			}
		} else if (*ptr == '\n' || *ptr == '\0') {
			fprintf(stderr,"line %d: bad string\n", lineno);
			exit(1);
		} else
			buf[j++] = *ptr;
		ptr++;
	} /* while() */
	ptr++;
	SKIPWHITE(ptr);
	if (*ptr != '\0') {
		fprintf(stderr,"line %d: illegal format\n", lineno);
		exit(1);
	}
	buf[j] = '\0';
	if ((ptr = strdup(buf)) == NULL) {
		fprintf(stderr, "Out of space\n");
		exit(1);
	}
	return(ptr);
}

/*
 * getnum - get integer value after keyword
 */
int
getnum(ptr)
char *ptr;
{

	int val;
	char *end;

	val = (int) strtol(ptr, &end, 0);
	SKIPWHITE(end);
	if (val > CHAR_MAX || end == ptr || *end != '\0') {
		fprintf(stderr, "line %d: bad value specified\n", lineno);
		exit(1);
	}
	return(val);
}

