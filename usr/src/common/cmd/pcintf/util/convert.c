#ident	"@(#)pcintf:util/convert.c	1.1.1.3"
#ifdef NOSCCSID
#define SCCSID(arg)
#else
#define SCCSID(arg)	static char Sccsid[] = arg
#endif
SCCSID("@(#)convert.c	6.5	LCC");	/* Modified: 6/12/91 17:35:13 */

/*
   (c) Copyright 1989, 1990 by Locus Computing Corporation. ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.  No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/
/*
 convert description
 
 usage:
 	convert  \
 	dos2unix   [-options] ... [input [output]]
 	unix2dos /
 
 All three programs are the same file.  The dos -> unix and unix -> dos
 translation is done based on the program name.
 
 Options include: -u	 Uppercase file.
 		 -l	 Lowercase file.
 		 -f	 Force (old switch).
 		 -b	 Binary (old switch).
		 -7	 Issue warning if any character uses the 8th bit.
 		 -x	 Direct.  Don't translate.

 		 -i tbl	 Translate input using table tbl.
 		 -o tbl	 Translate output using table tbl.
 
 		 -c c	 Use c as the translate failure user_char.
 		 -m	 Allow Multiple character translations.
 		 -a	 Abort on translation failure.
		 -s	 Use best single character translations.
 		 -z	 Stop at first CTL-Z encountered.
 
 		 -d	 Perform carriage return to line feed translation.
 		 -p	 Perform line feed to carriage return translation.

		 -q	 Suppress display of warning messages.
		 -v	 Display warning messages and translation statistics.
		 -h	 Print this message.
		 -?	 Print this message.
 
 
 Options are case insensitive and may be stacked into a single arg: -mlz.
 
 Translation errors will cause a warning to be specified.
 if -a is set, delete output and print error message.
 
 The data should be processed a line at a time.
 
 Defaults for the tables should be accuired as needed.
 	dos: current codepage.
 	unix: codepage (LANG var) or 8859.

	Environment variables:
	CONVOPTS = specifies conversion options, overidden
		   by command line options
	COUNTRY  (UNIX only) specifies default country to use
	CODEPAGE (UNIX only) specifies DOS country to use (for unix2dos, etc.)
*/
#include <stdio.h>
#include <lmf.h>
#include <lcs.h>
#include <fcntl.h>
#include <string.h>

#ifdef	MSDOS
#include <dos.h>
#include <io.h>
#include <stdlib.h>
#else	/* not MSDOS */
#include <unistd.h>
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#endif	/* not MSDOS */

/* Defines for NLS */
/* N = file name, L = Lang, C = Codeset */
#ifdef	MSDOS
#define NLS_PATH "c:/pci/%N/%L.%C;c:/usr/lib/merge/%N/%L.%C"
#else	/* not MSDOS */
#define NLS_PATH "/usr/pci/%N/%L.%C:/usr/lib/merge/%N/%L.%C"
#endif	/* not MSDOS */

#define NLS_FILE "dosmsg"
#define NLS_FILE_MRG "mergemsg"
#define NLS_LANG "En"

#define NLS_DOMAIN "LCC.PCI.DOS.CONVERT"

/* Explanation: The path will be /pci/lib/en.lmf, the domain will
 * be: LCC.PCI.DOS.CONVERT
 */

/* Defines used for translation types requested 
 *
 * Table
 *	bit	3	2	1	0	value
 *
 *	D2D	x	x	0	0	0x0
 *	D2U	x	x	0	1	0x1
 *	U2D	x	x	1	0	0x2
 *	U2U	x	x	1	1	0x3
 *	L2U	0	1	x	x	0x4
 *	U2L	1	0	x	x	0x8
 */

#define D2U		0x01	/* DOS-to-UNIX translation */
#define U2D		0x02	/* UNIX-to-DOS translation */


#define L2U		0x04	/* case translation requested */
#define U2L 		0x08	/* translate lower to upper */

#define CR2LF		0x10	/* perform CR/LF to LF translation */
#define LF2CR		0x20	/* perform LF to CR/LF translation */

#define BEST_SINGLE	0x40	/* Use best single character */

#define DONT		0x80	/* Don't translate:
				 * this means merely do CR/LF
				 * conversion, not character 
				 * translation
				 */

#define NO_XLAT		0x01	/* Can't translate */

/* defines binary/nobinary translation, set in "binary" */

#define BINARY		0x01
#define NOBINARY	0x02

#ifdef	MSDOS
/* Information for Country Code, separators, etc. */
#define CURRENT	0xffff	/* used to get current country/code page */

#define MMDDYY	0	/* date order mmddyy */
#define	DDMMYY	1	/* date order ddmmyy */
#define	YYMMDD	2	/* date order yymmdd */


struct country_info {
	unsigned char	InfoID;		/* Type of info to get */
	unsigned short	CInfoSize;	/* amount of data that follows */
	unsigned short	CountryID;	/* Selected CountryID */
	unsigned short	CodePage;	/* Selected Code Page */
	unsigned short	DateFormat;	/* Date Format:
				 * 0 = m d y order
				 * 1 = d m y order
				 * 2 = y m d order
				 */
	char	Symbol[5];	/* Currency Symbol
				 * example: "DM", 0, ?, ?
				 */
	char	Sep1000[2];	/* Thousands seperator
				 * example: ",", 0
				 */
	char	Sep1[2];	/* Fractions seperator
				 * example: ".", 0
				 */
	char	SepDate[2];	/* Date seperator
				 * example: "/", 0
				 */
	char	SepTime[2];	/* Time seperator
				 * example: ":",0
				 */
	char	Format;		/* Currency Format:
				 * 0 = currency symbol, value
				 * 1 = value, currency symbol
				 * 2 = currecny symbol, space, value
				 * 3 = value, space, currency symbol
				 * 4 = currency symbol is decimal separator
				 */
	char	SigDigits;	/* Number of Significant Digits in Currency */
	char	TimeFormat;	/* Time Format:
				 * 0 = 12 hour clock
				 * 1 = 24 hour clock
				 */
	unsigned short	UpperCaseOff;	/* Offset  of Routine to UpperCaseAL */
	unsigned short	UpperCaseSeg;	/* Segment of Routine to UpperCaseAL */
	char	SepData[2];	/* Data List Separator:
				 * example: ",", 0
				 */
	char	reserved[5];	/* Reserved for future */
} ci;

#define	DOS	1
#define READ	"rb"	/* open for read only, binary */
#define WRITE	"rb+"	/* open for read/write only, binary; file must exist */
#define CREATE	"wb"
#else	/* not MSDOS */
#define READ "r"
#define WRITE "r+"
#define CREATE "w"

#define _MAX_FNAME 14
#endif	/* not MSDOS */

#undef TRUE
#undef FALSE
#define FALSE   0
#define TRUE    1

#define CR      0xd	/* carriage return */
#define LF      0xa	/* line feed */
#define CTLZ    0x1a	/* control-z */

#define MAX_LINE 512	/* maximum allowable line length */

#define OPTIONS	"?hHuUlLfFbB7xXi:I:o:O:c:C:mMaAzZdDpPqQsSvV"
#define P_OPTS	"?hHulfb7xiocmazdpqsv" /* options we show */
#define MINOPTIONS "z"
#define	DELIMITERS "\t "

unsigned short country;	/* country id */

int xlat = 0;		/* integer used to store translation options */
int binary = 0; 	/* integer used tp store binary mode translation */
int force = FALSE;	/* force translation */
int quiet = TRUE;	/* suppress display of warning messages */

int end_at_ctrl_z = 0;  /* Handle CTL-Z properly? */
int abort_on = 0;	/* Abort on first untranslatable character */
int multi_char = 0;	/* allow multiple character translations */

#ifndef DEFAULT_CHAR
#define DEFAULT_CHAR '*'
#endif

char fail_char = '\0';

FILE *input = NULL, *output = NULL;	/* our two files */
char *in_file, *out_file;	/* input and output file names */

char myname[_MAX_FNAME];	/* program name */

/* Defines for Language Character Sets */
#ifdef	MSDOS
char lcspath[] = {"c:/pci/lib;c:/usr/lib/merge/lcs"};
#else	/* not MSDOS */
char lcspath[] = {"/usr/pci/lib:/usr/lib/merge/lcs"};
#endif	/* not MSDOS */

#define DOS437		"pc437"
#define UNIX8859	"8859"

/* pointers to translation tables */
lcs_tbl input_table, output_table; 
char input_tbl_name[_MAX_FNAME] = { '\0' };
char output_tbl_name[_MAX_FNAME] = { '\0' };

char *lcs_convert();	/* lcs conversion routine */

/* Statistics */
unsigned long exact_trans = 0;
unsigned long best_single = 0;
unsigned long multi_trans = 0;
unsigned long user_default = 0;
unsigned long input_bytes = 0;
unsigned long output_bytes = 0;
unsigned long line_no=0; 

#define MAX_PTRS	50
char **make_array();	/* routine to split up char * into char ** */

extern	char * getenv();
#ifdef	MSDOS
extern  char *feature();
#else	/* not MSDOS */
extern	char * malloc();
#endif	/* not MSDOS */


/* define to determine if program name is neither dos2unix nor unix2dos */
#define D2U_OR_U2D \
	(!strcmp(myname, "dos2unix") || !strcmp(myname, "unix2dos") \
	 || !strcmp(myname, "dos2aix") || !strcmp(myname, "aix2dos")) 

static char *unixCharTableName();

/* name of code_page */
static char *code_page_name();

/*
 * Start of the actual routines.
 */

main(argc, argv)
int argc;
char *argv[];
{
	char *ptr;
	char *argbuf;	/* used for environment arguments processing */
	char **argptr;
	extern int optind;
	int lmf_fd;

#ifdef	MSDOS
	/* split out the program's name from argv[0] */
	_splitpath(argv[0], (char *) NULL, (char *) NULL, 
		&myname[0], (char *) NULL);
#else	/* not MSDOS */
	ptr = strrchr(argv[0], '/');
	strcpy(myname, (ptr == NULL) ? argv[0] : ptr+1);
#endif	/* not MSDOS */
	ptr = myname;
	/* lowercase the name */
	for(;*ptr;ptr++)
		*ptr = tolower(*ptr);

	/* Open the message file && push the domain */
	lmf_fd = lmf_open_file(NLS_FILE, NLS_LANG, NLS_PATH);
#ifndef	MSDOS	/* for MERGE */
	if ( lmf_fd < 0 )
		lmf_open_file(NLS_FILE_MRG, NLS_LANG, NLS_PATH);
#endif	/* not MSDOS */
	if (
#if defined(LMF_NO_MESSAGE_DEFAULTS) || defined(LMF_REQUIRE_MESSAGE_FILE)
	    lmf_fd < 0 ||
#else
	    lmf_fd >= 0 &&
#endif
            lmf_push_domain(NLS_DOMAIN)) {
		fprintf(stderr,
"%s: Message file missing.  Please see installation guide for information\n\
\ton how to set up NLSPATH and LANG to find the message file.\n", myname);
		exit(1);
	}

	if ( ! D2U_OR_U2D && (ptr=getenv("CONVOPTS")) != NULL) {
		if ((argbuf = malloc(strlen(ptr)+strlen(argv[0])+2)) == NULL) {
			/* argh! */
			fputs(lmf_get_message("CONVERT86",
				"Cannot find memory to process CONVOPTs!\n"),
				stderr);
			help();
			exit(1);
		}
		strcpy(argbuf, argv[0]);
		strcat(argbuf, " ");
		strcat(argbuf, ptr);
		argptr = make_array(argbuf, DELIMITERS);
		parse(count_args(argptr), argptr, OPTIONS);
		getoptinit();	/* re-initialize getopt pointers */
		free(argbuf);
	}

	parse(argc, argv, OPTIONS);

	/* Check if incompatible options selected */
	if ((binary & BINARY) && (binary & NOBINARY)) {
		fputs(lmf_get_message("CONVERT1A",
			"Incompatible options specified\n"), stderr);
		help();
		exit(1);
	}
	
	if ( (xlat & U2L) && (xlat & L2U) ) {
		fputs(lmf_get_message("CONVERT1",
			"Uppercase and Lowercase BOTH specified\n"), stderr);
		help();
		exit(1);
	}
	if ( (xlat & DONT) && (xlat & (U2L|L2U)) ) {
		fputs(lmf_get_message("CONVERT8",
			"Upper/Lower specified without translation\n"), stderr);
		help();
		exit(1);
	}

	/* Force setting of D2U if we were invoked as dos2unix */
	if (!strcmp(myname, "dos2unix") || !strcmp(myname, "dos2aix")) {
		xlat |= D2U;
		xlat |= CR2LF;
	}

	/* Force setting of U2D if we were invoked as unix2dos */
	if (!strcmp(myname, "unix2dos") || !strcmp(myname, "aix2dos")) {
		xlat |= U2D;
		xlat |= LF2CR;
	}

	/* check to see if conflicting CR/LF modes requested */
	if ((xlat & CR2LF) && (xlat & LF2CR)) {
		fputs(lmf_get_message("CONVERT2",
			"Unix2dos and Dos2unix BOTH specified\n"), stderr);
		help();
		exit(1);
	}

	/* check to see if BEST_SINGLE and fail_char are both set */
	if ((xlat & BEST_SINGLE) && (fail_char != '\0')) {
		fputs(lmf_get_message("CONVERT_B1",
		"Can't perform best single with default character set!\n"), 
		stderr);
		help();
		exit(1);
	}

	/* Ensure that we have enough files to work with
	 * two - for input and output 
	 * or
	 * one with output being stdout
	 */
	switch(argc - optind) {
		case 2:
			in_file = argv[optind++];
			out_file = argv[optind];
			break;
		case 1:
			in_file = argv[optind];
			out_file = "STDOUT";
			output = stdout;
			break;
		case 0:
			in_file = "STDIN";
			input = stdin;
			out_file = "STDOUT";
			output = stdout;
			break;

		default:
			help();
			exit(1);
	}

	/*
	 * Perform initial open checks, check to see if the files are the same
	 * and open translation tables, if required.
	 */
	init_chk(in_file, out_file);

	/*
	 * If we got this far, we can start doing some real work.
	 */
	convert();

	/* 
	 * print statistics if not quiet
	 */
	 if ( quiet != TRUE )
		 print_stats();
}


/*
 * print statistics
 */
int
print_stats()
{
	/* first, clean up */
	fclose(input);
	fflush(output);
	fclose(output);

	/* If we are not doing translations do not print these statistics. */
	if ( ! (xlat & DONT) ) {
		/* banner */
		fputs(lmf_get_message("CONVERT_S1",
			"\nTranslation Information:\n"),
			stderr);

		/* exact translations */
		if ( exact_trans > 0 )
			fputs(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("CONVERT_S2",	
				"Glyphs translated exactly:\t\t%1\n"),
				"%5lu", exact_trans),
				stderr);

		if ( multi_trans > 0 )
			/* multi-byte translations */
			fputs(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("CONVERT_S3",
				"Glyphs translated to multiple bytes:\t%1\n"),
				"%5lu", multi_trans),
				stderr);

		if ( user_default > 0 )
			/* user default translations */
			fputs(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("CONVERT_S4",
				"Glyphs translated to user default:\t%1\n"),
				"%5lu", user_default),
				stderr);

		if ( best_single > 0 )
			/* best single */	
			fputs(lmf_format_string((char *) NULL, 0, 
				lmf_get_message("CONVERT_S5",
				"Glyphs translated to best single glyph:\t%1\n"),
				"%5lu", best_single),
				stderr);

		/* total */
		fputs(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("CONVERT_S6",
			"\nTotal number of glyphs processed:\t%1\n"),
			"%5lu",
			exact_trans + best_single + multi_trans + user_default),
			stderr);
	}

	fputs(lmf_format_string((char *) NULL, 0, 
		lmf_get_message("CONVERT_S7",
		"\nProcessed %1 bytes in %2 lines into %3 output bytes\n"),
		"%lu%lu%lu",
		input_bytes, line_no, output_bytes),
		stderr);
}


/*
 * parse
 *	parse options
 */
parse(arg_cnt, args, options)
int arg_cnt;
char *args[];
char *options;
{
        extern int getopt();
        extern int optind;
        extern char *optarg;
	int c;

        while ( (c = getopt(arg_cnt, args, OPTIONS)) != EOF ) {
		switch(c) {
			case 'U':
			case 'u':	/* Uppercase file */
				xlat |= L2U;
				break;

			case 'L':
			case 'l': /* Lowercase file */
				xlat |= U2L;
				break;

			case 'F':
			case 'f': /* Force (old switch) */
				force = TRUE;
				break;

			case 'B':
			case 'b': /* Binary (old switch) */
				binary |= BINARY;
				break;

			case '7':	/* 7 bit warning mode */
				binary |= NOBINARY;
				break;

			case 'X':
			case 'x': /* Direct. Don't translate */
				xlat |= DONT;
				break;

			case 'I':
			case 'i': /* Translate input using table itbl */
				strcpy(input_tbl_name, optarg);
				break;

			case 'O':
			case 'o': /* Translate output using table otbl */
				strcpy(output_tbl_name, optarg);
				break;

			case 'C':
			case 'c': /* Use c as the translate failure user_char */
				fail_char = optarg[0];
				break;

			case 'M':
			case 'm': /* Allow Multiple character translations */
				multi_char = 1;
				break;

			case 'A':
			case 'a': /* Abort on translation failure */
				abort_on = 1;
				break;

			case 'Z':
			case 'z': /* Handle C-Z properly (dos2unix/unix2dos) */
				end_at_ctrl_z = 1;
				break;

			case 'D':
			case 'd': /* Perform the dos -> unix 
				   * carriage return to line feed translation */
				xlat |= CR2LF;
				break;

			case 'P':
			case 'p': /* Perform the unix -> dos 
				   * line to carriage return translation */
				xlat |= LF2CR;
				break;
				
			case 'q':
			case 'Q': /* Suppress display of warning messages */
				quiet = TRUE;
				break;

			case 's':
			case 'S': /* Use best single translation */
				xlat |= BEST_SINGLE;	/* One-way switch */
				break;	

			case 'v':
			case 'V': /* don't suppress warning messages */
				quiet = FALSE;
				break;

			case 'h':
			case 'H':
			case '?':
			default:
				usage();
				exit(1);
		}
        }
}

/*
 * sfputs
 *	This is a replacement function for fputs(), and is used to handle the
 *	special output conditions for this program, with regards to <CTRL>Z.
 *	
 *	First, DOS won't allow the writing of <CTRL>Z to the console.  When that
 *	character is in a buffer written to the console, only the part of the
 *	buffer before the <CTRL>Z is written.  (This may be a bug in the
 *	libraries being used when this was first written, but ...)  Files, on
 *	the other hand, seem to have no problem.
 *	
 *	Second, leaving <CTRL>Z's in the file when converting from DOS to UNIX
 *	can cause problems with some UNIX programs, so it has been decided that
 *	these characters are persona non grata, and to be excised.  Note that
 *	for some measure of efficiency, a check is made for <CTRL>Z, only when
 *	doing a DOS to UNIX conversion, and we aren't stopping at the <CTRL>Z
 *	as an EOF character.
 */

int
sfputs(string, stream)
register char *string;
FILE *stream;
{
	int wrote, do_z_check, to_write;
	char *end_string_p, *ctrl_z_p;

	/*
	 * Find the end of the string, and get a local variable to indicate
	 * if we should check for the <CTRL>Z character.
	 */

	end_string_p = string + strlen(string);
	do_z_check = !end_at_ctrl_z && ((xlat & D2U) != 0);

	/*
	 * As long as there is breath left in this string ...
	 */

	while (string < end_string_p) {
		/*
		 * If we are checking for the <CTRL>Z, then scan the string for
		 * it.  If it's found, then the maximum length to write will
		 * be shortened.  If not found, or we aren't checking for it,
		 * the maximum length is the remainder of the string.
		 */

		if (do_z_check && ((ctrl_z_p = strchr(string, CTLZ)) != NULL)) {
			to_write = ctrl_z_p - string;
			output_bytes--;
			exact_trans--;
		}
		else
			to_write = end_string_p - string;

		/*
		 * Write the data.  This will usually be the remainder of the
		 * line, unless we are checking for the <CTRL>Z.
		 */

		wrote = fwrite(string, 1, to_write, stream);
		string += wrote;

		/*
		 * If we are at the end of the string, we have nothing left
		 * to do.  Unfortunately, we can't let the loop condition
		 * check this, because of the <CTRL>Z check below.
		 */

		if (string >= end_string_p)
			return 0;

		/*
		 * We haven't read to the end of the line yet (since we are
		 * still here), so check that we stopped on a <CTRL>Z (the only
		 * legal reason for stopping).  If this isn't the case, the
		 * write failed, so return the standard fputs() indication of
		 * that.  Otherwise, move the string pointer past the <CTRL>Z.
		 */

		if (*string != CTLZ)
			return EOF;
		string++;
	}
	return 0;
}

/*
 * convert
 * 	perform conversions 
 */
convert()
{
	char *bufp;
	char *ptr = NULL;
	int hi_bit = 0;
	static char buf[MAX_LINE+4];
	int ret_val = 0;

	rewind(input);
	rewind(output); /* just to be sure */

	/* Read a line from input */
	while ( fgets(bufp = buf, MAX_LINE, input) != NULL ) {
		if (binary & NOBINARY) {
			for(ptr = bufp; *ptr; ptr++) {
				if ( *ptr & 0x80 ) {
					if ( !hi_bit && !quiet) {
						fputs(
						lmf_get_message("CONVERT80",
						"Warning! 8 bit character\n"),
						stderr);
						hi_bit++;
					}
					*ptr &= 0x7f;
				}
			}
		}

		ptr = NULL;	/* reset pointer to NULL */

		/* 
		 * If end_at_ctrl_z is set, then this is the last line that we
		 * will attempt to write to the output file, if a <CTRL>Z is
		 * found in the line.  Also, if this is a DOS-to-UNIX
		 * translation, the <CTRL>Z is removed (by putting a '\0' 
		 * on top of it). 
		 * This, of course, assumes that the buffer is large enough,
		 * which it should be.  (Extra space was allocated.)
		 */

		if (end_at_ctrl_z && (ptr = strchr(bufp, CTLZ)) != NULL) {
			*ptr = '\0';
		} 

		/* Perform cr/lf mapping */
                if ( xlat & CR2LF)
			cr2lf(bufp);
		else if ( xlat & LF2CR)
			lf2cr(bufp);
		
		if (!(xlat & DONT)) { /* Perform character translation */
			bufp = lcs_convert(bufp, &ret_val);
		} else {
			input_bytes += strlen(bufp);
			output_bytes += strlen(bufp);
			line_no++;
		}


		/*
		 * We aren't terminating at the end of the line, so we need
		 * to write the entire thing, and continue with the loop.
		 */

		if (sfputs(bufp, output) < 0) {
			fputs(lmf_get_message("CONVERT21",
				"Write to output failed!\n"),
				stderr);
		}

		/*
		 * Did we hit an untranslatable character
		 * with stop translation (LCS_MODE_STOP_XLAT) ?
		 */
		if ( ret_val == NO_XLAT ) {
			fputs(lmf_format_string((char *) NULL, 0,
			lmf_get_message("CONVERT32", 
				"\nUntranslatable character in line # %1\n"),
				"%lu", line_no),
				stderr);
			--line_no; /* we didn't complete this line 
				    * so, don't count it
				    */
			print_stats();
			exit(1);
		}

		if ( end_at_ctrl_z && ptr != NULL ) /* we found a CTL-Z */
			break;
	}
}


#define ALLOC_ERR "Unable to allocate space for translation buffer!\n"
/*
 * lcs_convert
 *	perform character translation as required.
 *	exit if unable to perform translation.
 */
char *
lcs_convert(bufp, ret_val)
char *bufp;
int *ret_val;
{
	static int xlat_bufsize = 0;
	static char *xlat_buf;
	int done = 0;

	if (xlat_bufsize == 0) {
		xlat_bufsize = 2 * MAX_LINE;
		if ((xlat_buf = malloc(xlat_bufsize)) == NULL) {
			fputs(lmf_get_message("CONVERT30",
			"Unable to allocate space for translation buffer!\n"),
			stderr);
			exit(1);
		}
	}

	while(!done) {
		if ( lcs_translate_string(xlat_buf, xlat_bufsize, bufp) < 0) {
			switch(lcs_errno) {
			case LCS_ERR_NOTFOUND:
				fputs(lmf_get_message("CONVERT45",
					"Translation table(s) not found!\n"),
					stderr);
				exit(1);
			case LCS_ERR_BADTABLE:
				fputs(lmf_get_message("CONVERT46",
					"Translation table(s) bad!\n"), stderr);
				exit(1);
			case LCS_ERR_NOTABLE:
				fputs(lmf_get_message("CONVERT31",
					"Translation tables not set!\n"), 
					stderr);
				exit(1);
			case LCS_ERR_NOSPACE:
				/* Insufficient space, hhmm */
				free(xlat_buf);
				/* double the buffer size */
				xlat_bufsize <<= 1;
				if ((xlat_buf = malloc(xlat_bufsize)) == NULL) {
					fputs(lmf_get_message("CONVERT30",
					ALLOC_ERR),
					stderr);
					exit(1);
				}
				break;
			case LCS_ERR_STOPXLAT: 
				/* Untranslatable character and abort_on */
				*ret_val = NO_XLAT;
				done++;
				break;
			default:
				/* Unknown error condition, help! */
				fputs(lmf_format_string((char *) NULL, 0,
					lmf_get_message("CONVERT42", 
					"Unknown translation error: %1\n"), 
					"%d", lcs_errno), stderr);
				exit(1);
			} /* end switch */
		} else
			done++;
	}
	best_single += lcs_best_single_translations;
	multi_trans += lcs_multiple_translations;
	user_default += lcs_user_default_translations;
	input_bytes += lcs_input_bytes_processed;
	output_bytes += lcs_output_bytes_processed;
	exact_trans += lcs_exact_translations;
	line_no++;
	return xlat_buf;
}


/*
 * lf2cr
 *	perform line feed to carriage return / line fee mapping
 *	(unix to dos)
 *
 *      '\n' at end of line becomes '\r\n' unless there is already a '\r\n'
 *
 *      Presumes on input '\n' can only occur at end of line.
 *
 *      Changes data in place.
 *
 *      Presumes that there is an extra space in the passed buffer.
 */
lf2cr(buf)
char *buf;
{
	int bufsize;

	bufsize = strlen(buf);
	if (bufsize > 0 && buf[bufsize-1] == LF) {
		if (bufsize == 1 || buf[bufsize-2] != CR) {
			buf[bufsize-1] = CR;
			buf[bufsize]   = LF;
			buf[bufsize+1] = '\0';
			input_bytes--;
		}
	}
}

/*
 * cr2lf
 *	perform carriage to line feed mapping (dos 2 unix) mapping
 *
 *      '\r\n' at end of line becomes '\n'
 *
 *      Presumes on input '\n' can only occur at end of line.
 *
 *      Changes data in place.
 */
cr2lf(buf)
char *buf;
{
	int bufsize;

	bufsize = strlen(buf);

        /* change CRLF to LF */
	if ( bufsize > 1 && buf[bufsize-2] == CR && buf[bufsize-1] == LF) {
		buf[bufsize-2] = LF;
		buf[bufsize-1] = '\0';
		input_bytes++;
		
	}
}

/*
 * init_chk
 *	open files,
 *	check if same
 *	exit on errors 
 */
init_chk(in_file, out_file)
char *in_file;
char *out_file;
{
	int ret = 0;
	int new_out = 0;	/* flag to indicate that output file is new */
	char *name_ptr;
	short mode;

	if ( input == NULL && (input = fopen(in_file, READ)) == NULL ) {
		fputs(lmf_format_string( (char *) NULL, 0,
			lmf_get_message("CONVERT4", "Cannot open %1\n"),
			"%s", in_file), stderr);
		exit(1);
	} 

	/*
	 * The file may not exist, opening it in WRITE mode checks if it does. 
	 * If it does not then clearly out_file != in_file,
	 * which means that we don't have to do a samefile() check.
	 */
	if ( output == NULL && (output = fopen(out_file, WRITE)) == NULL ) {
		if ( (output = fopen(out_file, CREATE)) == NULL) {
			fputs(lmf_format_string( (char *) NULL, 0,
				lmf_get_message("CONVERT4", "Cannot open %1\n"),
				"%s", out_file), stderr);
			exit(1);
		} else
			new_out = 1;
	} 

	/* Don't check for sameness if input is stdin or output stdout */
	if ( input != stdin && output != stdout && !new_out ) {
		ret = samefile(fileno(input), fileno(output));

		switch(ret) {

		case 0:	/* Okay, now scratch the output file */
			fclose(output);
			if ((output = fopen(out_file, CREATE)) == NULL) {
				fputs(lmf_format_string(
					(char *) NULL, 0, 
					lmf_get_message("CONVERT4",
						"Cannot open %1\n"),
					"%s", out_file), stderr);
				exit(1);
			} 
			break;

		case 1:
			fputs(lmf_format_string(
				(char *) NULL, 0,
				lmf_get_message("CONVERT5", 
					"Files %1 and %2 are identical\n"),
				"%s%s", in_file, out_file), stderr);
			exit(1);

		case -1:
			fputs(lmf_format_string(
				(char *) NULL, 0,
				lmf_get_message("CONVERT3",
					"An error occured reading %1\n"),
				"%s", in_file), stderr);
			exit(1);

		case -2:
			fputs(lmf_format_string(
				(char *) NULL, 0,
				lmf_get_message("CONVERT3",
					"An error occured reading %1\n"),
				"%s", out_file), stderr);
			exit(1);

		case -3:
			fputs(lmf_format_string(
				(char *) NULL, 0,
				lmf_get_message("CONVERT6",
				    "An error occured writing %1\n"),
				"%s", out_file), stderr);
			exit(1);
		}
	}

#ifdef MICROSOFT
	/* be sure to set binary modes for stdin */
	if ( input == stdin )
		setmode(fileno(stdin), O_BINARY);

	/* be sure to set binary modes for stdout */
	if ( output == stdout )
		setmode(fileno(stdout), O_BINARY);
#endif /* MICROSOFT */

	/* Find current DOS code page */
	get_country_info();

	/* 
	 * Now open translation tables, if required.
	 */
	if ( !(xlat & DONT) ) {
		switch (xlat & (D2U|U2D)) {
		case D2U:
			if (*input_tbl_name == '\0')
				strcpy(input_tbl_name, code_page_name());
			if (*output_tbl_name == '\0')
				strcpy(output_tbl_name, 
					unixCharTableName(out_file));
			break;

		case U2D:
			if (*input_tbl_name == '\0')
				strcpy(input_tbl_name,
					 unixCharTableName(in_file));
			if (*output_tbl_name == '\0')
				strcpy(output_tbl_name, code_page_name());
			break;
		}

		/* 
		 * check that both table names are set 
		 */
		if ( *output_tbl_name == '\0' && *input_tbl_name == '\0' ) {
			fputs(lmf_get_message("CONVERT77",
				"Input and output translation tables not given\n"), 
				stderr);
			help();
			exit(1);
		}
		if ( *output_tbl_name == '\0' ) {
			fputs(lmf_get_message("CONVERT7",
				"Output translation table not given\n"), 
				stderr);
			help();
			exit(1);
		}

		if ( *input_tbl_name == '\0' ) {
			fputs(lmf_get_message("CONVERT17",
				"Input translation table not given\n"),
				stderr);
			help();
			exit(1);
		}


		/* Set input table */
		if ((input_table = lcs_get_table(input_tbl_name, lcspath))
		     == NULL) {
			fputs(lmf_format_string( (char *) NULL, 0,
			  lmf_get_message("CONVERT10",
			  "Cannot open translation table %1\n"),
			  "%s", input_tbl_name), stderr);

			  /* Okay, now what?
			   * I vote for exit!
			   */
			   exit(1);
		}

		/*
		 * check if input and output table names are the same.
		 * If not, get the output table.
		 */
		if ( strcmp(output_tbl_name, input_tbl_name) == 0 )
			output_table = input_table;
		else if ((output_table = lcs_get_table(output_tbl_name,
		     lcspath)) == NULL) {
			fputs(lmf_format_string( (char *) NULL, 0,
			  lmf_get_message("CONVERT10",
			  "Cannot open translation table %1\n"),
			  "%s", output_tbl_name), stderr);

			  /* Okay, now what?
			   * I vote for exit!
			   */
			   exit(1);
		}

		/* Set the tables */
		if ( lcs_set_tables(output_table, input_table) ) {
			fputs(lmf_get_message("CONVERT15",
				"Invalid translation table!\n"), stderr);
			exit(1);
		}
	} /* end if (!(xlat & DONT)) */

	/* 
	 * Turn on fail_char if 
	 * BEST_SINGLE is not on
	 */
	if ( ! (xlat & BEST_SINGLE) )
		fail_char = (fail_char != '\0') ? fail_char : '*';

	/* set mode */
	mode =	((abort_on) ? LCS_MODE_STOP_XLAT : 0) |
		((fail_char != '\0') ? LCS_MODE_USER_CHAR : 0) |
		((xlat & U2L) ? LCS_MODE_LOWERCASE : 0) |
		((xlat & L2U) ? LCS_MODE_UPPERCASE : 0) |
		((!multi_char)? LCS_MODE_NO_MULTIPLE : 0);

	/* Now set the options */
	lcs_set_options(mode, fail_char, country);
}

/*
 * int
 * samefile(in_fd, out_fd)
 * int in_fd,  out_fd;
 *
 *   This function returns 0 if:
 *	the files described by in_fd and out_fd are not the same file
 *
 *   Other error returns are as follows:
 *	1 == the two files are the same file
 *	-1 == read error on input file
 *	-2 == read error on output file
 *	-3 == write error on output file
 *
 */

int
samefile(in_fd, out_fd)
int in_fd, out_fd;
{
	char	byte_save;	/* byte save location */
	char	read_buf;
	int	i, out_zl, in_zl;
	long	lseek();

	in_zl = out_zl = FALSE;
	read_buf = byte_save = 0;

	/* Get 1st byte from input file */
	i = read(in_fd, &read_buf, 1);
	if (i == 0)
		in_zl = TRUE;
	else if (i < 0) {
		return(-1);
	}

	/* Get 1st byte from output file */
	i = read(out_fd, &byte_save, 1);
	if (i == 0)
		out_zl = TRUE;
	else if (i < 0) {
		return(-2);
	}

	if ((out_zl != in_zl) || (read_buf != byte_save)) {
		return(0);	/* first bytes differ, assume different */
	}


	/* rewind output, put incremented byte from input file into output */
	lseek(out_fd, 0L, SEEK_SET);
	read_buf++;	/* new data different always than old */

	if (write(out_fd, &read_buf, 1) != 1) {
		return(-3);
	}


	/* rewind input, read  */
	lseek(in_fd, 0L, SEEK_SET);
	if (read(in_fd, &read_buf, 1) != 1) {
		return(-1);
	}

	lseek(out_fd, 0L, SEEK_SET);
	if (write(out_fd, &byte_save, 1) != 1)
		return(-3);


	byte_save ++;
	lseek(in_fd, 0L, SEEK_SET);
	lseek(out_fd, 0L, SEEK_SET);
	return(byte_save == read_buf);
}

/*
 * make_array
 *	chop up a char * into 'n' char *'s
 * 	and create a char ** (or char *[])
 *	which has pointers to all the new
 *	elements. Return the char **
 */
char **
make_array(arg, split)
char *arg;
char *split;
{
	char *head[MAX_PTRS], **p;
	p = &head[0];
	*p++ = strtok(arg, split);
	while ((*p++ = strtok(NULL, split)) != NULL)
		;
	return &head[0];
}

/*
 * count_args
 *	counts the number of arguments in a string
 */
count_args(args)
char **args;
{
	int count = 0;

	while ( *args++ != NULL)
		count++;

	return count;
}

/*
 * usage -
 *	print a usage line
 */
usage()
{
#ifdef	MSDOS
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("CONVERT_M1_D",
		"usage: %1   [/options] ... [input [output]]\n"), 
		"%s", myname), stderr);

	fputs(lmf_get_message("CONVERT_M3_D","\
Options include: /u      Uppercase file.\n\
                 /l      Lowercase file.\n\
                 /f      Force (old switch).\n\
                 /b      Binary (old switch).\n\
"), stderr);
	fputs(lmf_get_message("CONVERT_M4_D","\
                 /7      Issue warning if any character uses the 8th bit.\n\
                 /x      Direct.  Don't translate.\n\n\
                 /i tbl  Translate input using table tbl.\n\
                 /o tbl  Translate output using table tbl.\n\
\n"), stderr);
	fputs(lmf_get_message("CONVERT_M5_D","\
                 /c c    Use c as the translate failure user_char.\n\
                 /m      Allow Multiple character translations.\n\
                 /a      Abort on translation failure.\n\
                 /s      Use best single character translation.\n\
                 /z      Handle C-Z properly (dos2unix/unix2dos).\n\
\n"), stderr);
	if ( ! D2U_OR_U2D )
		fputs(lmf_get_message("CONVERT_M6_D","\
                 /d      Perform carriage return to line feed translation.\n\
                 /p      Perform line feed to carriage return translation.\n\
\n"), stderr);
	fputs(lmf_get_message("CONVERT_M7_D","\
                 /q      Suppress display of warning messages.\n\
                 /v      Display warning messages and translation statistics.\n\
                 /h or /? Print this message.\n\
"), stderr);
#else	/* not MSDOS */
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("CONVERT_M1_U",
		"usage: %1   [-options] ... [input [output]]\n"), 
		"%s", myname), stderr);

	fputs(lmf_get_message("CONVERT_M3_U", "\
Options include: -u      Uppercase file.\n\
                 -l      Lowercase file.\n\
                 -f      Force (old switch).\n\
                 -b      Binary (old switch).\n\
"), stderr);
	fputs(lmf_get_message("CONVERT_M4_U", "\
                 -7      Issue warning if any character uses the 8th bit.\n\
                 -x      Direct.  Don't translate.\n\n\
                 -i tbl  Translate input using table tbl.\n\
                 -o tbl  Translate output using table tbl.\n\
\n"), stderr);
	fputs(lmf_get_message("CONVERT_M5_U", "\
                 -c c    Use c as the translate failure user_char.\n\
                 -m      Allow Multiple character translations.\n\
                 -a      Abort on translation failure.\n\
                 -s      Use best single character translation.\n\
                 -z      Handle C-Z properly (dos2unix/unix2dos).\n\
\n"), stderr);
	if (! D2U_OR_U2D )
		fputs(lmf_get_message("CONVERT_M6_U", "\
                 -d      Perform carriage return to line feed translation.\n\
                 -p      Perform line feed to carriage return translation.\n\
\n"), stderr);
	fputs(lmf_get_message("CONVERT_M7_U", "\
                 -q      Suppress display of warning messages.\n\
                 -v      Display warning messages and translation statistics.\n\
                 -h or -? Print this message.\n\
"), stderr);
#endif	/* not MSDOS */
}

/*
 * help
 *	a simple one-liner
 */
help()
{
#ifdef	MSDOS
    if ( D2U_OR_U2D ) {
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("CONVERT_HELP00_D",
		"usage: %1 [/%2] [infile [outfile]]\n"),
		"%s%s", myname, MINOPTIONS), stderr);
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("CONVERT_HELP01_D",
		"       %1 /h or /? for detailed help\n"),
		"%s", myname), stderr);
    } else
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("CONVERT_HELP1_D",
		"usage: %1 /%2 [infile [outfile]]\n"),
		"%s%s", myname, P_OPTS), stderr);
#else	/* not MSDOS */
    if ( D2U_OR_U2D ) {
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("CONVERT_HELP00_U",
		"usage: %1 [-%2] [infile [outfile]]\n"),
		"%s%s", myname, MINOPTIONS), stderr);
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("CONVERT_HELP01_U",
		"       %1 -h or -? for detailed help\n"),
		"%s", myname), stderr);
    } else
	fputs(lmf_format_string((char *) NULL, 0,
		lmf_get_message("CONVERT_HELP1_U",
		"usage: %1 -%2 [infile [outfile]]\n"),
		"%s%s", myname, P_OPTS), stderr);
#endif	/* not MSDOS */
}

/* 
 * get_country_info:
 *	use DOS to get country information 
 */
get_country_info()
{
#ifdef	MSDOS
	union REGS regs;
	struct SREGS sregs;

	/* Get country information */
	segread(&sregs);
	regs.x.ax = 0x6501;	/* this is the call 65H arg 1 */
	regs.x.bx = CURRENT;	/* Use current code page */
	regs.x.cx = sizeof(struct country_info);
	regs.x.dx = CURRENT;	/* Use current country */
	regs.x.di = (unsigned short)&ci;
	intdosx(&regs, &regs, &sregs);
	
	if (ci.CodePage == 0) {		/* if get code page request not */
	    ci.CodePage = 437;		/* supported use some defaults */
	    ci.CountryID = 1;
	} 
	
	country = ci.CountryID;
	
#else	/* not MSDOS */
	char *ptr;

	if ((ptr = getenv("COUNTRY")) != NULL)
		country = atoi(ptr);
	else {
		if (!quiet)
		    fputs(lmf_get_message("CONVERT60", 
			"COUNTRY environment variable not set, using 1\n"),
			stderr);
		country = 1;
	}

#endif	/* not MSDOS */
}


/*
 * Given a file name that is expected to be in a UNIX format, determine the LCS
 * translation table for it.  The way in which this is done depends on the
 * OS in which this file is run:
 *
 *	MSDOS - Can do several things if the file is on a virtual drive.  If
 *		none of these work out, the UNIX default is used.
 *
 *	UNIX  - Not currently much choice here, just use the default.
 *
 * NOTE - This doesn't handle a path of STDIN or STDOUT.  Since this file
 * sets the filenames for these to "STDIN" and "STDOUT", they are automatically
 * rejected by isvirtual().
 */

static char *
unixCharTableName(pathp)
char *pathp;
{
#ifdef	MSDOS
	int drive_numb;   /* drive number (1=A) of virtual drive 
                             used to get feature from */
        int n_vdrv;       /* number of virtual drives */
        int f_vdrv;       /* drive number (1=A) of first virtual drive */
        int vdrv;         /* drive number (1=A) or a virtual drive */
	char *unix_namep; /* name of character set UNIX is using */
        
	static char vname[] = "@:"; /* name of a virtual drive */

	/*
	 * Determine the host number from the specified path.
	 */

	drive_numb = isvirtual(pathp);
        
        /* If the path isn't on a virtual drive, use the UNIX default of
         * the first virtual drive logged in.
         */        
        
	if (drive_numb == 0) {
                n_vdrv=vdrive(0);  /* number of virtual drives */
                f_vdrv=vdrive(1);  /* first virtual drive (1=A) */
                for (
                        drive_numb=f_vdrv;
                        drive_numb<f_vdrv+n_vdrv;
                        drive_numb++
                ) {
                        vname[0]=drive_numb+('A'-1);
                        if (isvirtual(vname))
                                goto found_drv;
                }
                return UNIX8859;  /* if no logged in virt drives, ret dflt */
        }
found_drv:

	/*
	 * This is on a virtual drive (which means a UNIX drive), so try to
	 * get the table name as one of the features of the remote driver.
	 * If the feature isn't specified, use the UNIX default.
	 */

	unix_namep = feature(drive_numb, "LCSTBL");
	return (unix_namep == NULL) ? UNIX8859 : unix_namep;

#else	/* not MSDOS */
	char *lang, *code_ptr;

	/*
	 * Under UNIX, see if we can pick up the code set
	 * out of the LANG environment variable,
	 * else use the UNIX8859 default.
	 */
	if ((lang = getenv("LANG")) != NULL && 
		(code_ptr = strrchr(lang, '.')) != NULL) {

		/* our code set is just beyond the '.' */
		return ++code_ptr;
	} else
		return UNIX8859;
#endif	/* not MSDOS */
}
static char *
code_page_name()
{
#ifdef	MSDOS
	static char codepage[6];
	sprintf(codepage, "pc%u", ci.CodePage);
	return &codepage[0];
#else	/* not MSDOS */
	char *ptr;

	if ((ptr = getenv("CODEPAGE")) != NULL)
		return ptr;
	else {
		if (  D2U_OR_U2D ) {
			if ( !quiet )
			    fputs(lmf_format_string((char *) NULL, 0,
				lmf_get_message("CONVERT61",
				"CODEPAGE environment variable not set, using %1\n"),
				"%s", DOS437),stderr);
			return DOS437;
		}
	}
#endif	/* not MSDOS */
}

/**** End of File ****/
