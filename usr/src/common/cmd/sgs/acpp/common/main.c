#ident	"@(#)acpp:common/main.c	1.62.2.12"
/* main.c - top-level functions for ANSI cpp */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>
#include <pfmt.h>
#include <unistd.h>
#include <errno.h>
#ifndef  MERGED_CPP
#include "sgs.h"	/* pick up CPL_PKG and CPL_REL */
#endif
#include "cpp.h"
#include "buf.h"
#include "file.h"
#include "group.h"
#include "predicate.h"
#include "syms.h"

/* This file processes the command line, which causes operations
** on some objects which record the option flags.
*/
#define	END_OF_COMMAND_LINE	(char *)0

#ifdef MERGED_CPP
#define OPTS	"i:o:f:A:BCD:EHI:PR:SU:VX:Y:"
#else
#define OPTS	"i:o:f:A:BCD:EHI:PR:SU:VX:Y:K:"
#define USAGE	"usage: [-BCEHPSV]\
 [-Aname[(value)] ...] [-Dname[=value] ...] [-Uname ...]\
 [-Idir ...] [-X[tac]] [-Ydirectory] [-Kdollar] \
 [-K uchar|schar] [input [output]]"
#endif

#ifdef CPLUSPLUS
int cplusplus = 1;
#else
int cplusplus;
#endif

#ifdef DEBUG
int	debuglevels[26]; /* one for each lower case alphabetic char */
#	undef OPTS
#	undef USAGE
#	ifdef PERF
#		ifdef MERGED_CPP
#			define	OPTS	"i:o:f:0:p:vA:BCD:EHI:PR:SU:VX:Y:"
#		else
#			define	OPTS	"i:o:f:0:p:vA:BCD:EHI:PR:SU:VX:Y:K:"
#			define	USAGE	"usage: [-BCEHPSV]\
 [-Aname[(value)] ...] [-Dname[=value] ...] [-Uname ...]\
 [-Idir ...] [-X[tac]] [-Ydirectory] [-Kdollar] \
 [-K uchar|schar] [input [output]]"
#		endif
#	else
#		ifdef MERGED_CPP
#			define	OPTS	"i:o:f:0:vA:BCD:EHI:PR:SU:VX:Y:"
#		else
#			define	OPTS	"i:o:f:0:vA:BCD:EHI:PR:SU:VX:Y:K:"
#			define	USAGE	"Usage: [-BCEHPSV]\
 [-Aname[(value)] ...] [-Dname[=value] ...] [-Uname ...]\
 [-Idir ...] [-X[tac]] [-Ydirectory] [-Kdollar] \
 [-K uchar|schar] [input [output]]"
#		endif
#	endif
#endif
#ifdef PERF
int	perflevels[26];	/* one for each lower case alphabetic char */
#endif

static	char *	myname;
static	int  nodefaults;/* boolean: if true, no default #asserts or #defines */
static	int	nfiles;	/* number of files read in previous calls */
	char *	inputname;
static	char *	outputname;
static	char *	errorname;

#ifndef MERGED_CPP
#ifdef C_CHSIGN
int chars_signed = 1;                   /* Default treatment of "plain" chars */
#else
int chars_signed = 0;
#endif
#endif

	/* Don't force preprocessing of .i's unless
	** preprocess_doti_flag is set.  User specifies -S
	** to set the flag.
	*/
static  int preprocess_doti_flag;	

#ifdef MERGED_CPP
void	(* pp_interface)();
#endif
unsigned int	pp_flags;

int
preprocess_doti()
{
	return preprocess_doti_flag;
}

#ifdef MERGED_CPP
char *
pp_optstring()
/* returns the getopt(3C) string used by acpp */
{
	return	OPTS;
}
#endif

int
pp_init(cp, fp)
	char * cp;
	void (*fp)();
/* Given the name of the process(argv[0]) and a pointer
** to an interface routine from a merged compiler (if any),
** this routine performs the pre-command line initialization for acpp.
*/
{
	COMMENT(cp != 0);
#ifndef MERGED_CPP
	COMMENT(fp == 0);
#else
	pp_interface = fp;
#endif
	pp_flags = 0;
	myname = cp;
	ch_init();
	tk_init();
	ex_init();
	fl_init();
	gr_init();
	lx_init();
	bf_init();
	pd_init();
	COMMENT(nfiles == 0);
	COMMENT(inputname == 0);
	COMMENT(nodefaults == 0);
#ifdef MERGED_CPP
	COMMENT(outputname == 0);
	COMMENT(errorname == 0);
#endif
	return PP_SUCCESS;
}

#ifndef MERGED_CPP
int
main(argc, argv)
	int argc;
	char **argv;
/* Driving routine for standalone cpp. */
{
	extern int optind;	/* see getopt(3C)	*/
	extern char *optarg;	/* see getopt(3C)	*/
	register int c;		/* return value from getopt(3C) */

	num_init((void (*)(const char *, ...))0, pp_realloc);
	(void)pp_init(argv[0], (void (*)())0);
	while ((c = getopt(argc, argv, OPTS)) != EOF)
		(void)pp_opt(c, optarg);
	for ( ; optind < argc; optind++)
		(void)pp_opt(EOF, argv[optind]);
	pp_opt(EOF, END_OF_COMMAND_LINE);
	{
		register Token * (*lex)();
		register Token * tp;

		lex = lx_input;
#ifdef DEBUG
		if (DEBUG('l') > 0)	lex = lx_token;
#endif
		while ((tp = (*lex)()) != 0)
			(void)fprintf(stdout, "%.*s\n", (int)tp->rlen, tp->ptr.string);
	}
#ifdef PERF
	if ( PERF('s') > 0 )
		st_perf();
#endif
	if (ferror(stdout))
		fl_uerror(gettxt(":1501","stdout I/O error"));
	if (ferror(stderr))
		fl_uerror(gettxt(":469","stderr I/O error"));
	exit(fl_numerrors() > 0 ? 2 : 0);
	/*NOTREACHED*/
}
#endif

int
pp_cleanup()
/* 
** This is the place to clean up after all input processing
** is completed, and to check for any error conditions that
** may exist.
** The return value should indicate if any errors were uncovered.
** ( error checking will take place only if a '.i' file is generated.
**   Otherwise it will be done by acomp.)
*/
{
	if ( lx_dotifile() )
	{
		if (ferror(stdout))
			fl_uerror(gettxt(":1501","stdout I/O error"));
		if (ferror(stderr))
			fl_uerror(gettxt(":469","stderr I/O error"));
	}
	return PP_SUCCESS;
}

void
pp_internal(msg)	/* fatal internal error */
	const char *msg;
/* to be replaced by fl_warn */
{
	pfmt(stderr,MM_ERROR, ":551:%s: Internal error: Line %ld: %s ",
		myname, bf_lineno, msg);
	fprintf(stderr,"%s\n", strerror(errno));
	abort();
}
void
pp_printmem(p, len)
	register char *p;
	register int len;
/* "printable" version of array of characters (ASCII) */
{
	register int ch;

	while (--len >= 0)
	{
		if ((ch = *p++) & 0x80)
		{
			(void)fputs("M-", stderr);
			ch &= ~0x7f;
		}
		if (ch < ' ' || ch == 0177)
		{
			(void)putc('^', stderr);
			ch ^= 0100;
		}
		(void)putc((char)ch, stderr);
	}
}

#ifdef __STDC__
	void *
#else
	char *
#endif
pp_malloc(size)
	unsigned int size;
/* Calls MALLOC(3C) with the given argument
** and causes a fatal error if 0 is returned.
** Otherwise, it returns a pointer to the space
** that MALLOC(3C) provided.
*/ 
{
	char * cp;

	if ((cp = malloc(size)) == 0)
		FATAL(gettxt(":470","malloc() fails"), "");
	return cp;
}

void
pp_nodefaults()
/* Set a flag to inhibit the creation of pre-asserted predicates
** and pre-defined macros at start-up.
*/
{
	nodefaults++;
}

#ifdef __STDC__
	void *
#else
	char *
#endif
pp_realloc(ptr, size)
	char * ptr;
	unsigned int size;
/* Calls REALLOC(3C) with the given arguments
** and causes a fatal error if 0 is returned.
** Otherwise, it returns a pointer to the space
** that REALLOC(3C) provided.
*/ 
{
	char * cp;

	if ((cp = realloc(ptr, size)) == 0)
		FATAL(gettxt(":471","realloc() fails"), "");
	return cp;
}

int
pp_opt(ch, arg)
	int ch;
	char * arg;
/* The first arg is a command line option character, or
** EOF if a file name or end of the command line.
** The second arg is the option argument, or the
** filename, or 0 if at the end of the command line. 
** The routine processes the given option or filename,
** and diagnoses violations of syntax with a fatal error.
** At the end of the command line, this routine performs
** preprocessor intialization.
** This routine returns 0 on success, or non-zero
** on failure unless this routine doesn't return
** because of a fatal error.
*/
{
		switch (ch)
		{
		case 'i':
			if (!(arg[1] == '\0' && arg[0] == '-') ) 
			{
				inputname = arg;
				if (inputname != 0 && freopen(inputname, "r", stdin) == 0)
					FATAL(gettxt(":472","cannot open "), arg);
			}
			break;

		case 'o':
			if (!(arg[1] == '\0' && arg[0] == '-') ) 
			{
				outputname = arg;
				if (outputname != 0 && freopen(outputname, "w", stdout) == 0)
					FATAL(gettxt(":472","cannot open "), arg);
			}
			break;

		case 'f':
			errorname = arg;
			break;
#ifdef DEBUG
		case '0':	/* debugging */
		{
			register char *p = arg;

			while ((ch = *p++) != '\0')
			{
				if (ch < 'a' || ch > 'z')
				{
					(void)fprintf(stderr,
						"%s: bad debug flag %c\n",
						myname, ch);
					return PP_FAILURE;
				}
				else
					debuglevels[ch - 'a']++;
			}
			break;
		}
		case 'v':	/* this used to print version of main.c */
			break;
#endif
#ifdef PERF
		case 'p':
		{
			register char *p = arg;

			while ((ch = *p++) != '\0')
			{
				if (ch < 'a' || ch > 'z')
				{
					pfmt(stderr,MM_ERROR,
						":552:%s: bad perf flag %c\n",
						myname, ch);
					return PP_FAILURE;
				}
				else
					perflevels[ch - 'a']++;
			}
			break;
		}
#endif
		case 'A':
			pd_option(arg);
			break;

		case 'B':
			pp_flags |= F_TWO_COMMENTS;
			cplusplus = 1;
			break;

		case 'C':
			pp_flags |= F_KEEP_COMMENTS;
			break;

		case 'D':
			st_argdef(arg);
			break;

		case 'E':
			break;	/* default behavior */
		case 'H':
			pp_flags |= F_INCLUDE_FILES;
			break;

		case 'I':
			fl_addincl(arg);
			break;

#ifndef MERGED_CPP
		case 'K':
			if (strcmp(arg, "dollar") == 0)
				tk_allow_dollar_ident();
			else if (strcmp(arg, "schar") == 0)
				chars_signed = 1;
			else if (strcmp(arg, "uchar") == 0)
				chars_signed = 0;
			break;
#endif
		case 'S':
			preprocess_doti_flag = 1;
			break;
		case 'P':
			pp_flags |= F_NO_DIRECTIVES;
			break;
#ifdef CXREF 
		case 'R':
			pp_flags |= F_CXREF;
			break;
#endif

		case 'U':
			st_argundef(arg);
			break;

		case 'V':
#ifndef MERGED_CPP 
			pfmt(stderr,MM_INFO, ":553:cpp: %s %s\n", CPL_PKG, CPL_REL);
#endif
			break;

		case 'X':
		{
		/* for now - don't forget to initialize default case( -Xt ) */
		/* for now - what if -Xac? */
			switch( *arg )
			{
			case 't':
				if ( ( F_XARGMASK ^ F_Xt ) & pp_flags )
manyargs:				FATAL(gettxt(":473","-X invoked multiply with different arguments"), (char *)0 );
				pp_flags |= F_Xt;
				break;

			case 'a':
				if ( ( F_XARGMASK ^ F_Xa ) & pp_flags )
					goto manyargs;
				pp_flags |= F_Xa;
				break;

			case 'c':
				if ( ( F_XARGMASK ^ F_Xc ) & pp_flags )
					goto manyargs;
				pp_flags |= F_Xc;
				break;

			default:
				goto fatalerror;
			}
			if ( arg[1] != '\0' )
fatalerror:			FATAL(gettxt(":474","-X flag must have a single argument"),(char *)0);
			break;
		}
		case 'Y':
			fl_stdir(arg);
			break;

		case EOF:
			if (arg == END_OF_COMMAND_LINE)
			{
				if (errorname != 0)
					inputname = errorname;
#ifdef CPLUSPLUS
				pp_flags |= F_TWO_COMMENTS;
#endif
				if ( (pp_flags & F_XARGMASK) == 0 )
					pp_flags |= F_Xt;
				st_init(nodefaults);
				if (nodefaults == 0)
					pd_preassert();
				fl_next(inputname, stdin);
				break;
			}
			else if (arg[1] == '\0' && arg[0] == '-')
				nfiles++;
			else
				switch (nfiles++)
				{
				case 0:
					if (inputname != 0
					 && freopen(inputname, "r", stdin) == 0)
						FATAL(gettxt(":472","cannot open "), arg);
					else
					if (freopen(inputname = arg, "r", stdin) == 0)
						FATAL(gettxt(":472","cannot open "), arg);
					break;

				case 1: 
					if (outputname != 0
					 && freopen(outputname, "w", stdout) == 0)
						FATAL(gettxt(":472","cannot open "), arg);
					else
					if (freopen(arg, "w", stdout) == 0)
						FATAL(gettxt(":472","cannot open "), arg);
					break;
				}
			if (nfiles > 2)
				FATAL(gettxt(":475","more than 2 files specified"), (char *)0);
			break;

#ifdef MERGED_CPP
		default:return PP_IGNORED;
#else
		default:FATAL(gettxt(":1502",USAGE), (char *)0);
			break;
#endif
		}
	return PP_SUCCESS;
}
#ifdef CXREF

void
pp_xref(mp, lineno, use)
Macro *mp;
long lineno;
char use;
{
	void cx_cpp();
	void cx_cppfile();

	if (fl_curname() == 0)
		cx_cppfile(mp->namelen, mp->name, lineno, use, "predefined");
	else 
		cx_cpp(mp->namelen, mp->name, lineno, use);
}

#endif
