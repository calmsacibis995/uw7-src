#ident	"@(#)OSRcmds:grep/grep.c	1.1"
#pragma comment(exestr, "@(#) grep.c 25.8 94/11/25 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1994 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 * 	Modification History
 *
 *	rewrite		scol!markhe	3 aug 92
 *	- fgrep/egrep and grep needed to be merged for XPG4.
 *	  also need to use regcomp() and regexec() for XPG4 compliance
 *	L000		scol!markhe	2 Oct 92
 *	- if input line is not terminated with a newline, then do not output
 *	  a newline on match
 *	L001		scol!markhe	2 Oct 92
 *	- backwards compatibility, if the `pattern_file' is an empty file
 *	  then match NO lines, -v inverts this to match ALL lines.  This
 *	  overrides any other expressions or pattern files given.
 *	L002		scol!markhe	15 Jan 93
 *	- bug fix.  with -l option, filename should not have a colon
 *	  appended
 *	L003		scol!markhe	30 Apr 93
 *	- case independent search was setting skip[] and bitmap[] incorrectly,
 *	  causing the matching of some upper case leters not to occur.
 *	L004		scol!gregw	16 Nov 93
 *	- backwards compatibility, if the `pattern_list' is empty then this
 *	  should match every line except when -x is given then it matches
 *	  every blank line.
 *	L005		scol!anthonys	27 Jul 94
 *	- Correct the name used for stdin when using the "-l" option.
 *	L006		scol!anthonys	28 Oct 94
 *	- Fix a dereference of a null pointer when a pattern file is
 *	  size 0.
 *	L007		scol!ianw	25 Nov 94
 *	- Replaced basename() function prototype with an include of libgen.h.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<locale.h>
#include	<string.h>
#include	<memory.h>
#include	<limits.h>
#include	<ctype.h>
#include	<regex.h>
#include	<libgen.h>					/* L007 */
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>
/* #include	<errormsg.h> */
#ifdef	INTL
#  include	<locale.h>
#  include	<nl_types.h>
#  include	"grep_msg.h"
#else
#  define	MSGSTR(num, str)	(str)
#  define	catclose(x)
#endif	/* INTL */


/* block size to use when reporting a match with the -b option */
#define		BLKSIZE		512

/*
 * structure to hold all the patterns
 */
struct pattern_list_t {
	int		len;		/* length of pattern */
	char		*pattern;	/* pattern itself */
	regex_t		preg;		/* compiled expression (if necessary) */
};

extern int	errno;

/* flags */
static int	egrep;		/* run as "egrep" */
static int	fgrep;		/* run as "fgrep" */

static int	bflag;		/* precede each line by the block number */
static int	cflag;		/* write only count of selected lines */
static int	eflag;		/* one or more patterns for search */
static int	Eflag;		/* use extended regular expressions */
static int	fflag;		/* read patterns from file */
static int	Fflag;		/* use fixed strings */
static int	hflag;		/* do not prepended file name on output */
static int	iflag;		/* case independent */
static int	lflag;		/* write only file names containing matches */
static int	nflag;		/* precede each line with line number */
static int	qflag;		/* quite - no output */
static int	sflag;		/* suppress error messages */
static int	vflag;		/* select lines not matching */
static int	xflag;		/* match entire lines only */

static char	*filename;	/* name of file being searched */
static FILE	*fp;		/* file pointer to `filename' */
static int	line_num;	/* line number in current file */
static int	match_count;	/* number of matches for current file */
static int	match_total;	/* number of matches for all files */
static int	num_patterns;	/* number of patterns to try against */
static int	all_match;	/* match all lines */
static int	no_match;	/* set when empty pattern file given  L001 */

/* holds all patterns to try matching against */
static struct pattern_list_t	*pattern_list = NULL;
static struct pattern_list_t	no_pattern_list = {0, ""};	/* L006 */

#ifdef	INTL
static	nl_catd	catd;
#endif	/* INTL */

char *command_name;

/* prototypes */
static void process_pattern_file(const char *);
static void process_pattern_list(char *);
static void compile_single_char_string(char *, int, int *);
static void execute_single_char_string(const char *, int, const int *);
static void compile_bitmap_string(unsigned char *);
static void execute_bitmap_string(const unsigned char *);
static void compile_expression_string(int);
static void execute_expression_string(void);
static void display_match(const char *, int);			/* L000 */
static void usage(void);

/*
 * Synopsis
 *	grep [-E|-F] [-c|-l|-q] [-bhinsvx] -e pattern_list [-f pattern_file...]
 *	     [file...]
 *	grep [-E|-F] [-c|-l|-q] [-bhinsvx] [-e pattern_list] -f pattern_file...
 *	     [file...]
 *	grep [-E|-F] [-c|-l|-q] [-bhinsvx] pattern_list [file...]
 *	     [file...]
 *
 *
 * Exit status
 *	0	one or more lines were selected
 *	1	no lines were selected
 *	2	an error occurred
 */
void
main(int argc, char **argv)
{
	unsigned char bitmap[32];    /* bitmap for multi-fixed string search */
	int skip[256];		     /* used for single-fixed string search */
	int exp_flag = 0;	     /* flags to regcomp() */
	int stdin_flag = 0;	     /* using standard input? */
	int open_error = 0;	     /* set if an open fails */
	int c;
	extern char *optarg;
	extern int optopt;
	extern int optind;
	extern int opterr;

	/* get name program executed under */
	command_name = basename(argv[0]);

	if (!strcmp(command_name, "fgrep")) {
		Fflag++;
		fgrep++;
	} else if (!strcmp(command_name, "egrep")) {
		Eflag++;
		egrep++;
		exp_flag |= REG_EXTENDED;
	}

#ifdef	INTL
	setlocale(LC_ALL, "");
	catd = catopen(MF_GREP, MC_FLAGS);
#endif	/* INTL */

	opterr = 0;
	while ((c = getopt(argc, argv, "bce:Ef:Fhilnqsvxy")) != -1) {
		switch(c) {
		case 'b':
			bflag++;
			break;
		case 'c':
			cflag++;
			break;
		case 'e':
			eflag++;
			process_pattern_list(optarg);
			break;
		case 'E':
			if (egrep || fgrep)
				usage();
			Eflag++;
			exp_flag |= REG_EXTENDED;
			break;
		case 'f':
			fflag++;
			process_pattern_file(optarg);
			break;
		case 'F':
			if (egrep || fgrep)
				usage();
			Fflag++;
			break;
		case 'h':
			hflag++;
			break;
		case 'y':
		case 'i':
			iflag++;
			/* in case we are using BREs or EREs */
			exp_flag |= REG_ICASE;
			break;
		case 'l':
			lflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'q':
			qflag++;
			break;
		case 's':
			sflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		case '?':
			errorl(MSGSTR(GREP_OPT, "-%c illegal option"), optopt);
			usage();
		}
	}

	if (Eflag && Fflag) {
		errorl(MSGSTR(GREP_E_AND_F, "-E and -F options are mutually exclusive"));
		usage();
	}

	if ((cflag && lflag) || (cflag && qflag) || (lflag && qflag)) {
		errorl(MSGSTR(GREP_C_L_AND_Q, "-c, -l and -q options are mutually exclusive"));
		usage();
	}

	if (!eflag && !fflag && argc != optind)
		/*
		 * no pattern(s) given with either the
		 * -e or -f option, so the next argument
		 * should be a pattern_list
		 */
		process_pattern_list(argv[optind++]);

	if (no_match == 0 && num_patterns == 0) {		/* L001 */
		errorl(MSGSTR(GREP_NPAT, "no pattern given"));
		usage();
		/* NOTREACHED */
	}

	if ((argc - optind) < 2)
		/* do not display file name on output */
		hflag++;

	if (no_match) {						/* L001 begin */
		/*
		 * match an empty pattern file onto
		 * matching a single fixed string
		 */
		Fflag++;
		num_patterns = 1;
		pattern_list = &no_pattern_list;		/* L006 */
	}							/* L001 end*/

	if (Fflag) {
		/* fixed string */
		if (num_patterns == 1)
			/* only one pattern so can compile for a fast match */
			compile_single_char_string(pattern_list[0].pattern, pattern_list[0].len, skip);
		else
			compile_bitmap_string(bitmap);
	} else
		compile_expression_string(exp_flag);

	if (optind == argc) {
		/* no file names given, therefore use stdin */
		optind--;
		stdin_flag++;
	}

	/* for each file name argument... */
	for (; optind != argc; optind++) {
		if (stdin_flag) {
			filename = MSGSTR(GREP_STDIN, "(standard input)");	/* L005 */
			fp = stdin;
		} else {
			filename = argv[optind];
			if ((fp = fopen(filename, "r")) == NULL) {
				/* no error message in silent mode */
				if (!sflag)
					psyserrorl(errno, MSGSTR(GREP_OPEN, "cannot open file %s for reading"), filename);
				open_error++;
				continue;
			}
		}

		/* zero counters */
		match_count = line_num = 0;

		if (Fflag)
			if (num_patterns == 1)
				execute_single_char_string(pattern_list[0].pattern, pattern_list[0].len, skip);
			else
				execute_bitmap_string(bitmap);
		else
			execute_expression_string();

		if (cflag)
			/* display number of matches in file */
			display_match(NULL, 0);			/* L000 */

		if (!stdin_flag)
			fclose(fp);

		match_total += match_count;
	}

	exit(open_error?2:(match_total?0:1));
}

static void
process_pattern_file(const char *filename)
{
	FILE *fp;
	int size;
	char *tmp;
	struct stat statbuf;

	/* get the size of the file */
	if (stat(filename, &statbuf) == -1) {
		psyserrorl(errno, MSGSTR(GREP_STAT, "cannot access file %s"), filename);
		exit(2);
	}

	/*							  L001 begin
	 * trap zero length pattern files,
	 * even if we have already found a zero length file
	 * still process other pattern files - they may
	 * contain an error
	 */
	if (statbuf.st_size == 0) {
		no_match++;
		return;
	}							/* L001 end */

	if ((tmp = malloc(statbuf.st_size+1)) == NULL) {
		errorl(MSGSTR(GREP_MEM_READ, "unable to allocate memory needed to read in pattern file %s"), filename);
		exit(2);
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		psyserrorl(errno, MSGSTR(GREP_OPEN_PAT, "error opening pattern file %s for reading"), filename);
		exit(2);
	}

	if ((size = fread(tmp, 1, statbuf.st_size, fp)) != statbuf.st_size) {
		psyserrorl(errno, MSGSTR(GREP_READ_PAT, "unable to read all of pattern file %s"), filename);
		exit(2);
	}

	fclose(fp);

	tmp[size] = '\0';

	process_pattern_list(tmp);

	return;
}

static void
process_pattern_list(register char *list)
{
	/* for each pattern in the list... */
	do {
		if (pattern_list == NULL) {
			/* first pattern */
			num_patterns = 1;
			if ((pattern_list = malloc(sizeof(struct pattern_list_t))) == NULL) {
				errorl(MSGSTR(GREP_MEM_PAT, "out of memory for building pattern list"));
				exit(2);
			}
		} else {
			/* pattern other than first */
			num_patterns++;
			if ((pattern_list = realloc(pattern_list, num_patterns * sizeof(struct pattern_list_t))) == NULL) {
				errorl(MSGSTR(GREP_MEM_PAT, "out of memory for building pattern list"));
				exit(2);
			}
		}

		pattern_list[num_patterns-1].len = 0;
		pattern_list[num_patterns-1].pattern = list;

		while (*list != '\n' && *list != '\0') {
			pattern_list[num_patterns-1].len++;
			list++;
		}

		if (pattern_list[num_patterns-1].len == 0)
			all_match++;

		if (*list)
			*list++ = '\0';
	} while (*list);

	return;
}

static void
compile_single_char_string(register char *pattern, int len, register int *skip)
{
	register int j;

	if (no_match) {						/* L001 begin */
		/*
		 * invert the setting of the invert flag,
		 * not nice, but makes code in execute_single_char_string
		 * cleaner
		 */
		vflag = vflag ? 0 : 1;
		return;
	}							/* L001 end */

	for (j=0; j < 256; j++)
		skip[j] = len;

	for (j=0; j < len; j++) {
		skip[(unsigned char)pattern[j]] = len-j-1;	/* L003 begin */
		if (iflag) {
			if (isupper(pattern[j])) {
				pattern[j] = tolower(pattern[j]);
				skip[(unsigned char)pattern[j]] = len-j-1;
			} else if (islower(pattern[j])) {
				skip[(unsigned char)toupper(pattern[j])] = len-j-1;
			}
		}						/* L003 end */
	}
}

static void
execute_single_char_string(register const char *pattern, int pattern_len, register const int *skip)
{
	register int i;
	int newline;						/* L000 */
	char buffer[LINE_MAX+1];
	int string_len;
	int t;
	int j;

read_line:
	newline = 0;						/* L000 */
	/* read in the next line */
	if (fgets(buffer, LINE_MAX+1, fp) == NULL)
		return;

	string_len = strlen(buffer);

	/* remove a possible newline character from the end */
	if (buffer[string_len-1] == '\n')
		buffer[--string_len] = '\0', newline++;		/* L000 */

	line_num++;

	if (no_match)						/* L001 begin */
		if (vflag)
			goto read_line;
		else
			goto found;				/* L001 end */

	/* check for empty string */
	if (all_match && !xflag)				/* L004 */
		goto found;

	/* quick check to see if it is possible to match this line */
	if (string_len < pattern_len || (xflag && string_len != pattern_len))
		goto fail;

	/*
	 * perform right-left string searching on buffer
	 */
	for (j=i=pattern_len-1; j >= 0; i--, j--)
		while ((unsigned char) (iflag?tolower(buffer[i]):buffer[i]) != (unsigned char) pattern[j]) {
			t = skip[(unsigned char) buffer[i]];
			i += (pattern_len-j > t)? pattern_len-j : t;
			if (i >= string_len) {
fail:
				if (vflag) {
					display_match(buffer, newline);/* L000*/
					if (lflag)
						return;
				}
				goto read_line;
			}
			j = pattern_len-1;
		}

found:
	/*
	 * found a match on the current line
	 */
	if (!vflag) {
		display_match(buffer, newline);			/* L000 */
		if (lflag)
			return;
	}

	goto read_line;

}

static void
compile_bitmap_string(register unsigned char *bitmap)
{
	register int code;
	register int i;
	int j;

	memset(bitmap, 0, 32);

	/* for each fixed string... */
	for (i=0; i < num_patterns; i++) {
		code = (unsigned char) pattern_list[i].pattern[0];
		bitmap[code/8] |= (1 << (code%8));
		if (iflag) {
								/* L003 begin */
			if (isupper(pattern_list[i].pattern[0]))
				/* also set the lower case bit */
				code = tolower(pattern_list[i].pattern[0]);
			else if (islower(pattern_list[i].pattern[0]))
				/* also set the upper case bit */
				code = toupper(pattern_list[i].pattern[0]);

			bitmap[code/8] |= (1 << (code%8));	/* L003 end */
		}
	}

	/* force each fixed string into lower case... */
	if (iflag)
		for (i=0; i < num_patterns; i++)
			for (j=0; j < pattern_list[i].len; j++)
				pattern_list[i].pattern[j] = tolower(pattern_list[i].pattern[j]);

	return;
}

static void
execute_bitmap_string(register const unsigned char *bitmap)
{
	int newline;						/* L000 */
	char buffer[LINE_MAX+1];
	int string_len;
	int i, j, k;

read_line:
	newline = 0;						/* L000 */
	if (fgets(buffer, LINE_MAX+1, fp) == NULL)
		return;

	string_len = strlen(buffer);
	if (buffer[string_len-1] == '\n')
		buffer[--string_len] = '\0', newline++;		/* L000 */

	line_num++;

	if (all_match && !xflag)				/* L004 */
		goto found;

	for (i=0; i < string_len; i++)
		if (bitmap[buffer[i]/8] & (1 << (buffer[i]%8)))
			for (j=0; j < num_patterns; j++) {
				if (string_len < pattern_list[j].len || (xflag && string_len != pattern_list[j].len))
					continue;
				for (k=0; k < pattern_list[j].len; k++)
					if ((unsigned char) ((iflag)?tolower(buffer[i+k]):buffer[i+k]) != (unsigned char) pattern_list[j].pattern[k])
						break;
				if (k == pattern_list[j].len) {
					/* match */
found:
					if (!vflag) {
						display_match(buffer, newline);									/* L000 */
						if (lflag)
							return;
					}
					goto read_line;
				}
			}

	if (vflag) {
		display_match(buffer, newline);			/* L000 */
		if (lflag)
			return;
	}

	goto read_line;

}



static void
compile_expression_string(int exp_flag)
{
	char expression[LINE_MAX+3];
	char *ptr;
	int status;
	int i;

	/* for each expression... */
	for (i=0; i < num_patterns; i++) {
		if (xflag) {
			/*
			 * add anchors so only matches complete lines
			 */
			expression[0] = '^';
			memcpy(&expression[1], pattern_list[i].pattern, pattern_list[i].len);
			expression[pattern_list[i].len+1] = '$';
			expression[pattern_list[i].len+2] = '\0';
			ptr = expression;
		} else
			ptr = pattern_list[i].pattern;

		if (status = regcomp(&pattern_list[i].preg, ptr, exp_flag | REG_NOSUB)) {
			char errbuf[100];	/* buffer for error message */

			regerror(status, &pattern_list[i].preg, errbuf, sizeof(errbuf));
			errorl(errbuf);
			exit(2);
		}
	}

	return;
}

static void
execute_expression_string(void)
{
	int newline;						/* L000 */
	char buffer[LINE_MAX+1];
	int string_len;
	int j;

read_line:
	newline = 0;						/* L000 */
	if (fgets(buffer, sizeof(buffer), fp) == NULL)
		return;

	string_len = strlen(buffer);
	if (buffer[string_len-1] == '\n')
		buffer[string_len-1] = '\0', newline++;		/* L000 */

	line_num++;

	if (all_match && !xflag)				/* L004 */
		goto found;

	for (j=0; j < num_patterns; j++)
		if (regexec(&pattern_list[j].preg, buffer, 0, (regmatch_t *) NULL, 0) == 0) {
			/* match */
found:
			if (!vflag) {
				display_match(buffer, newline);	/* L000 */
				if (lflag)
					return;
			}
			goto read_line;
		}

	if (vflag) {
		display_match(buffer, newline);			/* L000 */
		if (lflag)
			return;
	}
	goto read_line;

}


static void
display_match(const char *line, int newline)			/* L000 */
{
	if (!qflag) {
		if (line != NULL) {
			match_count++;
			if (cflag)
				return;
		}
		if (lflag) {
			printf("%s\n", filename);		/* L002 */
			return;
		} else if (!hflag)
			printf("%s:", filename);
		if (cflag) {
			printf("%d\n", match_count);
			return;
		}
		if (bflag)
			printf("%ld:", (ftell(fp)-1)/BLKSIZE);
		if (nflag)
			printf("%d:", line_num);
		printf( "%s", line);				/* L000 */
		if (newline)					/* L000 */
			putchar('\n');				/* L000 */
	} else
		/* in q-mode we exit on first match */
		exit(0);

	return;
}

static void
usage(void)
{
	if (fgrep || egrep)
		fprintf(stderr, MSGSTR(GREP_F_USAGE, "%s [-c|-l|-q] [-bhinsvx] [-e pattern_list] [-f pattern_file] [pattern_list][file...]\n"), command_name);
	else
		fprintf(stderr, MSGSTR(GREP_USAGE, "%s [-E|-F] [-c|-l|-q] [-bhinsvx] [-e pattern_list] [-f pattern_file] [pattern_list] [file...]\n"), command_name);
	exit(2);
}
