#ident	"@(#)acpp:common/file.c	1.52.4.9"
/* file.c - keep track of original and included files	*/

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <pfmt.h>
#include <unistd.h>
#include "cpp.h"
#include "buf.h"
#include "file.h"

#ifdef DEBUG
#ifdef __STDC__
#	define	DBGCALLS(func,cp)	if (DEBUG('f') > 1)\
			(void)fprintf(stderr, #func "(\"%s\")\n", cp);
#else
#	define	DBGCALLS(func,cp)	if (DEBUG('f') > 1)\
			(void)fprintf(stderr, "func(\"%s\")\n", cp);
#endif
#	define	DBGSEARCH(path)	if ( DEBUG('i') > 1)	\
			(void)fprintf(stderr, "search for %s\n", (path))
#else
#	define	DBGCALLS(func,cp)
#	define	DBGSEARCH(path)
#endif


#ifdef	__STDC__

#include <stdarg.h>
#define	FUNHDR(name,arg1name) \
name(const char *arg1name,...) { \
    va_list args; \
    va_start(args, arg1name);
/* 
** ANSI C variable number of arguments
*/

#else

#include <varargs.h>
#define FUNHDR(name,arg1name) \
name(arg1name, va_alist) \
char *arg1name; \
va_dcl \
{ \
    va_list args; \
    va_start(args);

/* 
** Non ANSI C variable number of arguments
*/
#endif

#ifdef MERGED_CPP
#ifdef __STDC__
extern void record_incdir(char *, int);
extern int record_start_of_src_file(int    seq_number,
				     int    line_number,
				     char  *file_name,
				     int    is_include_file);
extern void record_end_of_src_file(int	seq_number);
#else
extern void record_incdir();
extern int record_start_of_src_file();
extern void record_end_of_src_file();
#endif  /* __STDC__ */
#endif  /* MERGED_CPP */

#ifndef DFLTINC
#	define	DFLTINC	"/usr/include"
#endif
#define MAXERRORS	12	/* maximum errors allowed before killing process */
#define STACKSZ	10

extern	int	errno;
/* This module contains the code that handles entering and
** leaving files. The record of files is kept on a stack.
*/

typedef struct _file_ {
	FILE * file;	/* file descriptor	*/
	char * name;	/* file name		*/
	long lineno;	/* last line number read in file	*/
	int newlines;	/* newlines unaccounted for in `lineno' */
	long outline;	/* last line number written out		*/
	long cur;	/* current location in input buffer	*/
	long eod;	/* end of amount read into input buffer */
	long prev;	/* offset of last return in input buffer*/
} File;

static	File *	filelist;		/* pointer to base of dynamic stack */
static	int	idx;			/* index to filelist	*/
static	int	stklimit = STACKSZ;	/* current limit to idx	*/
/* A depiction of the File stack after "source.c" included "inc1.h",
** which in turn included "inc2.h":
**
**		< FILE*, char *> < other fields	       >
**		________________________________________
**		| NOT USED	| 	 NOT USED	| [stklimit-1]	
**		|_______________|_______________________|
**				.
**				.
**				.
**		________________________________________
**		| "inc2.h"	| 	 NOT USED	| [2] 
**filelist+idx->|_______________|_______________________|
**		| "inc1.h"	| state of "inc1.h"	| [1]	
**		|_______________|_______________________|
**		| "source.c"	| state of "source.c"	| [0]	
**    filelist->|_______________|_______________________|
*/

/* The #include directory search paths are maintained with these variables */
static	Token	usritk;			/* "/usr/include/" token */
static	Token *	dirtp =	&usritk;	/* top of stack of -I directories */
static	Token *	eodirtp = &usritk;	/* end of stack of -I directories */
static	Token *	stdtp = &usritk;	/* "standard" search directory */
/* A depiction of the file inclusion directory search stack after processing
** the command line `cpp -I./inc1 -I./inc2 -I./inc3 ...`:
**			__________
**	dirtp	-> 	|	 |-> "./inc1"
**			|________|
**				|		
**			________v_
**	 		|	 |-> "./inc2"
**			|________|
**				|
**			________v_
**	eodirtp	->	|	 |-> "./inc3"
**			|________|
**				|
**			________v_
**	stdtp	->	| usritk |-> "/usr/include/" (DFLTINC)
**			|________|
**				|
**				0
*/

static	int	nerrors;	/* number of errors in all files	*/
static	int	nwarns;		/* number of warnings in all files	*/
static	long	outline;	/* most recently written line number	*/
static	int	saidfile;	/* boolean: printed a `# n "file"' yet? */
static	int	isdotisource;	/* boolean: is original source a .i file ? */

static	void	sayfile(	/* void */ );
static	void	nofilemsg(	/* Token *	*/ );
static	void	openfile(	/* Token *	*/ );

extern	int	access();	/* library routine */

void
fl_addincl(dir)
	char *dir;
/* Given a pointer to a directory path from a -I command line option,
** this routine adds it to a directory search list #include directives.
*/
{
	register Token * tp;

	DBGCALLS(addincl,dir);
	(tp=tk_new())->rlen = (unsigned short) strlen(dir);
	tp->ptr.string = dir;
	tp->code = C_String; /* null terminated, no quotes */
	tp->next = stdtp;
	if (dirtp == stdtp)
		dirtp = tp;
	else
		eodirtp->next = tp;
	eodirtp = tp;
#ifdef DEBUG
	if (DEBUG('f') > 1)
	{
		(void)fputs( "addincl returns with -I stack:", stderr);
		tk_prl( dirtp);
	}
#endif
}

#ifdef INCCOMP
int
fl_baseline()
/* Returns the current line number in the original file.
** If the current file is an included header, this will return
** the line of the #include in the original file.
*/
{
	return idx?(filelist[0].lineno):bf_lineno;
}
#endif

char *
fl_basename()
/* Returns the name of the original file. */
{
	return filelist->name;
}

FILE*
fl_curfile()
/* Returns a pointer to the current file descriptor. */
{
	return filelist[idx].file;
}

char*
fl_curname()
/* Returns the current file name. */
{
	return filelist[idx].name;
}

char *
fl_incdir_name ()
/* Starting with the include directory pointed to by "dirtp", pass
** back a pointer to the include directory at the top of the stack.
** On each subsequent call, return the next include directory.
*/
{
    static int init_if_zero = 0;
    static Token *dir_token = 0;
    char * dir_name;

    if (init_if_zero == 0) {
	dir_token = dirtp;
	init_if_zero = 1;
    }  /* if */
    if (dir_token) {
	dir_name = dir_token->ptr.string;
	dir_token = dir_token->next;
    }  else  {
	dir_name = (char *)0;
    }  /* if */
    return dir_name;
}  /* fl_incdir_name */


int
fl_dotisource()
/* returns "true" if the original source file is
** a `.i' file, else "false".
*/
{
	return	isdotisource;
}

Token *
fl_error(itp)
/* Given a pointer to a sequence of Tokens from a #error directive,
** this routine prints out an
** error message and cause the process to exit().
*/
	register Token *itp;
{
	register Token * tp;

#ifdef DEBUG
	if (DEBUG('f') > 0)
		(void)fprintf(stderr, "fl_error(itp=%#lx)\n", itp);
#endif
	if ( itp == 0 )
		pfmt(stderr,MM_ERROR,":539:\"%s\", line %ld: #error\n", fl_curname(), bf_lineno);
	else
	{
		register unsigned int len; /* length of message */
# if 0
		len = itp->rlen;
		for (tp = itp; tp->next != 0 && tp->next->ptr.string[0] != '\n'; )
		{
			if (tp->code == C_BadInput)
				TKERROR(gettxt(":437","bad token in #error directive"), tp);
			len += tp->next->rlen;
			tp = tp->next;
		}
		if (tp != itp)	tk_merge(itp, tp, len);
#else
		tp = itp;
		len = 0;
		do {	len += tp->rlen;
			if (tp->code == C_BadInput)
				TKERROR(gettxt(":437","bad token in #error directive"), tp);
			tp = tp->next;
		} while (tp != 0 && tp->ptr.string[0] != '\n');
#endif
#ifdef DEBUG
		if (DEBUG('f') > 3)	tk_pr(itp, '\n');
#endif
		(void)pfmt(stderr,MM_ERROR, ":540:\"%s\", line %ld: #error: %.*s\n",
		 fl_curname(), bf_lineno, (int) len, itp->ptr.string);
	}
	exit(2);
	/* NOTREACHED */
	return (Token *)0;
}

void
fl_fatal(msg, pstr)
	const char *msg;
	const char *pstr;
/* Given an error message and a (possibly null) perror() message, this routine
** prints fatal error message and exit()'s
*/
{
	char * name;

	if (name = fl_curname())
	{
		pfmt(stderr,MM_ERROR, ":541:\"%s\", line %ld: fatal: %s%c",
		 name, bf_lineno, msg, pstr != 0 ? ' ' : '\n');
	}
	else
	{
		pfmt(stderr,MM_ERROR, ":542:command line: fatal: %s%c", 
		 msg, pstr != 0 ? ' ' : '\n');
	}
	if (pstr != 0)
		pfmt(stderr,MM_NOSTD,":380:%s: %s\n",pstr,strerror(errno));
	exit(2);
}

Token *
fl_include(tp)
	register Token *tp;
/* Given a pointer to sequence of Tokens from a # include directive,
** this routine diagnoses any syntax errors.
** If the directive is valid, a openfile() is called to attempt
** a search and open the included file.
*/
{
	Token* filetp;		/* pointer to {<}	*/
	register unsigned int len;	/* length of {<} {name} {>} construct */

#ifdef DEBUG
	if (DEBUG('d') > 0)
		(void)fprintf(stderr, "doinclude(tp=%#lx)\n", tp);
#endif
	if (tp == 0)
	{
		UERROR(gettxt(":438","#include directive missing file name"));
		return (Token *)0;
	}
	if ((tp = tk_rmws(ex_directive(tp))) == 0)
	{
		UERROR( gettxt(":439","no file name after expansion" ));
		return tp;
	}
	switch (tp->code)
	{
	case C_String:
	case C_Header:
		if (tp->rlen == 2)
		{
			UERROR(gettxt(":440", "empty file name" ));
			return tp;
		}
		tk_extra(tp->next);
		openfile(tp);
		return 0;

	case C_LessThan:
		filetp = tp;
		for (len = 1; (tp = tp->next) != 0; )
		{
			switch (tp->code)
			{
			case C_WhiteSpace:
				tp->ptr.string[0] = ' ';
				len += (tp->rlen = 1);
				tp->next = tk_rmws(tp->next);
				continue;

			case C_GreaterThan:
				if (++len == 2)
				{
					UERROR(gettxt(":441","empty header name"));
					return filetp;
				}
				tk_merge(filetp, tp, len);
				tk_extra(filetp->next);
				filetp->code = C_Header;
				openfile(filetp);
				return 0;

			default:len += tp->rlen;
				continue;
			}
		}
		UERROR(gettxt(":442","no closing \">\" in \"#include <...\""));
		return filetp;

	default:UERROR(gettxt(":443","bad file specification"));
		return tp;
	}
}

void
fl_init()
/* Initializes the data structures in file.c */
{
	COMMENT(idx == 0);
	COMMENT(stklimit == STACKSZ);
	filelist = (File *)pp_malloc(sizeof(File) * STACKSZ);
	filelist->name = (char *)0;
	filelist->file = (FILE *)0;
	usritk.ptr.string = DFLTINC;
	usritk.rlen = (unsigned short) strlen(usritk.ptr.string);
	usritk.code = C_String;
#ifdef MERGED_CPP
	record_incdir(usritk.ptr.string, usritk.rlen);
#endif
}


		
int
fl_isoriginal()
/* Returns an indication of whether the current file is the original file */
{
	return !idx;
}

static Token *
doline(tp, mode, infoflag)
/* Given a sequence of Tokens from a `# <number>'  or `#line' directive,
** a mode of operation (see bf_tokenize function comment for mode descriptions),
** and a flag to indicate which directive, this routine diagnoses any syntax error
** and changes the state of  the preprocessor in accordance with the semantics of 
** the directive.
*/
	register Token *tp;
	int mode, infoflag;
{
	static	int wrotefile;	/* boolean: have output #file directive yet ? */
	register char * cp = NULL;

	long	number;		/* value for line number	*/
	switch (tp->code)
	{
		char *cp;	/* where strtol() ends */
		char **pptr;	/* pointer to `cp' */
		char * buf;	/* number buffer */

	case C_BadInput:	/* for `cpp -X[ac]` of `#line 08' */
		/* could warn here about "invalid token"  */
		tp->code = C_I_Constant;
		/*FALLTHRU*/
	case C_I_Constant:	
		pptr = &cp;
		buf = ch_saven( tp->ptr.string, tp->rlen);
		buf[tp->rlen] = '\0';
		number = strtol(buf, pptr, 10);
		if (tp->rlen != (int)(cp - buf))
			goto bad_number;
		if (number == 0)
		{
			if ( infoflag )
			    UERROR(gettxt(":444", "0 is invalid in # <number> directive" ));
			else
			    UERROR(gettxt(":445", "0 is invalid in #line directive" ));
			return tp;
		}
		if ((tp->next = tk_rmws(tp->next)) == 0)
			break;
		if (tp->next->code == C_String)
			tp = tp->next;
		else
			if ( infoflag )
			    WARN(gettxt(":446", "string literal expected after # <number>" ));
			else
			    WARN( gettxt(":447","string literal expected after #line <number>" ));
		tk_extra(tp->next);
		tp->next = 0;
		break;

	default:
bad_number:     if (infoflag) 
		    UERROR(gettxt(":448", "identifier or digit sequence expected after \"#\""));
	        else
		    UERROR(gettxt(":449", "digit sequence expected after \"#line\"" ));
	        return tp;
	}
	COMMENT(tp->code == C_I_Constant || tp->code == C_String);
	switch (tp->code)
	{
		unsigned int	len;

	case C_String:
		len = tp->rlen - 1;
		if ( idx )	/* original source filename not malloc'd */
			free( filelist[idx].name );
		tp->ptr.string[len] = '\0';
		cp = pp_malloc(len);
		(void) memcpy(cp, tp->ptr.string+1, len);
		filelist[idx].name = cp;
		if ( (!infoflag) && (mode == B_text) )
		{
			if (wrotefile == 0)
			{
				(void)fprintf(stdout, "#file\t\"%s\"\n", cp);
				wrotefile++;
			}
		}
#ifdef MERGED_CPP
		else if (mode == B_tokens)
			(* pp_interface)(PP_LINE, tp);
#endif
		/*FALLTHRU*/
	case C_I_Constant:
		bf_lineno = number - 1;
		bf_newlines = 0;
#ifdef MERGED_CPP
		(void) record_start_of_src_file(PP_CURSEQNO(), number, cp,
						0 /* FALSE */);
#endif
	}
	if (mode == B_text)	sayfile();

	saidfile = 1;
	return tp;
}

Token *
fl_line(tp, mode)
/* Given a sequence of Tokens from a # line directive and a mode of operation
** (see bf_tokenize function comment for mode descriptions), this routine calls
** doline() to diagnose any syntax errors and change the state of the preprocessor
** in accordance with the semantics of the directive.
*/
	register Token *tp;
	int mode;
{
	int infoflag = 0;

#ifdef DEBUG
	if (DEBUG('f') > 0)
	{
		(void)fprintf(stderr, "fl_line() called with:\n");
		tk_prl(tp);
	}
#endif
	if ( tp == 0)
	{
		UERROR(gettxt(":450", "no tokens in #line directive" ));
		return tp;
	}
	if ((tp = tk_rmws(ex_directive(tp))) == 0)
	{
		UERROR(gettxt(":451", "no tokens after expansion" ));
		return 0;
	}
	return( doline(tp, mode, infoflag) );
}

Token *
fl_lineinfo(tp, mode)
/* Given a sequence of Tokens from a `# <number>' compiler line information directive
** and a mode of operation (see bf_tokenize function comment for mode descriptions),
** this routine calls doline() to diagnose any syntax errors and change the state of
** the preprocessor in accordance with the semantics of the directive.
*/
	register Token *tp;
	int mode;
{
	int infoflag = 1;

#ifdef DEBUG
	if (DEBUG('f') > 0)
	{
		(void)fprintf(stderr, "fl_lineinfo() called with:\n");
		tk_prl(tp);
	}
#endif
	COMMENT(tp != 0);
	if ((tp = tk_rmws(ex_directive(tp))) == 0)
	{
		UERROR(gettxt(":452", "no tokens after expansion" ));
		return 0;
	}
	return( doline(tp, mode, infoflag) );
}

void
fl_next(fn,fp)
char* fn;
FILE* fp;
/* Given a file name and a file descriptor, this routine
** saves state information of the current file (if any)
** and pushes a new file on a stack.
** The two arguments are assumed to correspond to a newly opened file.
** The two arguments must be non-null, except that if the name for
** the base file on the stack ("original" source file) is null, the
** file is named "<stdin>".
*/
{
	register File	*p;
	int is_include_file = 0;	/* Assume FALSE. */

	p = filelist + idx;
	if ( p->name )
	{
		if ( pp_flags & F_INCLUDE_FILES )
			(void)fprintf(stderr, "%*s\n", idx*4 + strlen(fn), fn);
		p->lineno = bf_lineno;
		p->newlines = bf_newlines;
		p->outline = outline;
		p->cur = bf_cur;
		p->eod = bf_eod;
		p->prev = bf_prev;
		bf_cur = bf_eod;
		bf_lineno = 0;
		bf_newlines = 0;
		outline = 0;
		COMMENT(idx < stklimit);
		if (++idx == stklimit)
		{
			stklimit += STACKSZ;
			filelist = (File *)pp_realloc((char*) filelist, sizeof(File) * stklimit);
			p = filelist + idx;
		}
		else
			p++;
		is_include_file = 1;	/* TRUE. */
	}
	else
	{
		unsigned int len;	/* length of original source file name */

		COMMENT(isdotisource == 0);
		if (fn != 0)
		{
			len = strlen(fn);
			if (fn[len - 1] == 'i'
			 && len > 1
			 && fn[len - 2] == '.')
				isdotisource++;
		}
#ifdef DEBUG
		if (DEBUG('f') > 1)
			(void)printf("isdotisource=%d\n", isdotisource);
#endif
	}
	p->file = fp;
	p->name = fn?fn:"<stdin>";
	bf_ptr[bf_eod] = BF_SENTINEL;
	saidfile = 0;
#ifdef MERGED_CPP
	(void) record_start_of_src_file(bf_seqno + 1, 1, fn, is_include_file);
#endif
}

int
fl_numerrors()
/* Returns the number of errors that occurred in all files */
{
	return nerrors;
}

int
fl_numwarns()
/* Returns the number of warnings that occurred in all files */
{
	return nwarns;
}

void
fl_prev()
/* This routine pops the file stack, restoring the previous file
** and its state into being. This routine is responsible for closing the file.
*/
{
	register File	*p;

	p = filelist + idx;
	free(p->name);
    	(void)fclose(p->file);
#ifdef MERGED_CPP
	record_end_of_src_file(bf_seqno);
#endif  /* MERGED_CPP */
	if ( idx )
	{
		--idx;
		--p;
		bf_lineno = p->lineno;
		bf_newlines = p->newlines;
		outline = p->outline;
		bf_cur = p->cur;
		bf_eod = p->eod;
		bf_prev = p->prev;
		bf_ptr[bf_eod] = BF_SENTINEL;
		saidfile = 0;
	}
	COMMENT(idx >= 0);
    	return;
}

void
fl_sayline()
/* Called before writing a line, this routine prints a "#" line information
** comment if the next line number to be output does not correspond
** to the number of physical lines read by the preprocessor.
*/
{
#ifdef DEBUG
	if (DEBUG('l') > 0)
		(void)fprintf(stderr, "fl_sayline() called with: bf_lineno=%ld, outline=%ld\n",
		 bf_lineno, outline);
#endif
	if ((pp_flags & F_NO_DIRECTIVES) == 0)
	{
		if (saidfile == 0)
		{
			(void)printf("# %ld \"%s\"\n", bf_lineno, filelist[idx].name);
			saidfile = 1;
			outline = bf_lineno;
		}
		else if (++outline != bf_lineno)
		{
			(void)printf("# %ld\n", bf_lineno);
			/*  may want to produce (bf_lineno - outline)
			** '\n's instead of # comment
			*/
			outline = bf_lineno;
		}
	}
}

int
fl_stdhdr()
/* Returns "true" if the current file is a header from
** the "standard" file inclusion search directory,
** else "false".
*/
{
	char * curname;	/* current file name */

	curname = fl_curname();
	if (strncmp(curname, stdtp->ptr.string, stdtp->rlen) == 0 
	 && curname[stdtp->rlen] == '/')
		return 1;
	else
		return 0;
}
void
fl_stdir(dir)
	char * dir;
/* Given a pointer to the argument of a -Y command line option,
** this routine changes the standard default search directory,
** which normally is /usr/include/.
*/
{
	register char * cp;

	DBGCALLS(dr_stdir,dir);
	for( cp = dir; *cp != '\0'; )
		cp++;
	stdtp->ptr.string = dir;
	stdtp->rlen = (int)(cp - dir);
#ifdef MERGED_CPP
	record_incdir(stdtp->ptr.string, stdtp->rlen);
#endif
#ifdef DEBUG
	if (DEBUG('f') > 1)
	{
		(void)fputs( "st_stdir returns with stdtp:", stderr);
		tk_pr( stdtp, '\n');
	}
#endif
}

void
fl_tkerror(msg, tp)
	const char * msg;
	Token * tp;
/* Given a message an a pointer to a token, this routine prints
** out the message and the spelling of the token on stderr;
** and increments the error count.
*/
{
	char* name;

	nerrors++;
	if (name=fl_curname())
	{
		pfmt(stderr,MM_ERROR,
		 ":543:\"%s\", line %ld: %s: ", name, bf_lineno, msg);
	}
	else
	{
		pfmt(stderr,MM_ERROR,
		 ":544:command line: %s: ", msg );
	}
	pp_printmem(tp->ptr.string, (int) tp->rlen);
	putc('\n',stderr);
	if (nerrors > MAXERRORS)
		FATAL(gettxt(":1503","too many errors"), (char *)0);
}

void
fl_tkwarn(msg, tp)
	const char * msg;
	Token * tp;
/* Given a message an a pointer to a token, this routine prints
** out the message and the spelling of the token on stderr.
*/
{
	char* name;

	nwarns++;
	if (name=fl_curname())
	{
		pfmt(stderr,MM_WARNING,
		 ":545:\"%s\", line %ld: %s: ", name, bf_lineno, msg);
	}
	else
	{
		pfmt(stderr,MM_WARNING,
		 ":546:command line: %s: ", msg );
	}
	pp_printmem(tp->ptr.string, (int) tp->rlen);
	putc('\n',stderr);
}

void
FUNHDR(fl_uerror, msg)
/* Given a warning message string, this routine prints it out on stderr
** and increments an error counter.
*/
/*{*/
	{
		char* name;
	
		nerrors++;
		if (name = fl_curname())
		{
			pfmt(stderr,MM_ERROR, ":547:\"%s\", line %ld: ",
		 	name, bf_lineno);
			(void)vfprintf(stderr,msg,args);
		}
		else
		{
			pfmt(stderr,MM_ERROR, ":548:command line: " );
			(void)vfprintf(stderr,msg,args);
		}
    		va_end(args);
		putc('\n',stderr);
		if (nerrors > MAXERRORS)
			FATAL(gettxt(":1503","too many errors"), (char *)0);
	}
}

void
FUNHDR(fl_warn, msg)
/* Given a warning message string, this routine prints it out on stderr */
/*{*/
	{
		char* name;

		nwarns++;
		if (name = fl_curname()) 
		{
			pfmt(stderr,MM_WARNING,":549:\"%s\", line %ld: ",
				name, bf_lineno);
			(void)vfprintf(stderr,msg,args);
		}
		else
		{
			pfmt(stderr,MM_WARNING, ":550:command line: " );
			(void)vfprintf(stderr,msg,args);
		}
		putc('\n',stderr);
		va_end(args);
	}
}

static void
nofilemsg(inputp)
	register Token * inputp;
/* Given a pointer a modified Token that contains the name
** of a file, this routine prints out a diagnostic indicating
** that the file could not be found.
*/
{
	register Token * tp;	/* holds name of file */

	COMMENT(inputp != 0);
	COMMENT(inputp->rlen > 2);
	COMMENT(inputp->ptr.string[0] == '/'
	  ||	inputp->ptr.string[0] == '\"'
	  ||	inputp->ptr.string[0] == '<');
	COMMENT(inputp->rlen > 2);
	COMMENT(inputp->ptr.string[inputp->rlen - 1] == '\0');
	tp = tk_new();
	tp->code = C_Goofy;
	tp->rlen = inputp->rlen;
	tp->ptr.string = inputp->ptr.string ;
	if ( inputp->code == C_String ) {	/* restore original token spelling*/
		tp->ptr.string[0] = '"';
		tp->ptr.string[tp->rlen - 1] = '"';
	}
	else if ( inputp->code == C_Header ) {
		tp->ptr.string[0] = '<';
		tp->ptr.string[tp->rlen - 1] = '>';
	}
	if (errno == EMFILE)
		TKERROR(gettxt(":453","cannot open include file (too many open files)"), tp);
	else
		TKERROR(gettxt(":454","cannot find include file"), tp);
	(void)tk_rm(tp);
}

static void
openfile(filetp)
	register Token * filetp;
/* Given a pointer to a "file" or <file> Token from a #include directive,
** this routine finds the file in accordance with the search rules and opens it.
** If there is no such file in the searched directories, this routine diagnoses
** the error.
*/
{
	register char * path;	/* search path mandated by ANSI C */
	register FILE * file;	/* descriptor of newly included file */

	COMMENT(filetp != 0);
	COMMENT(filetp->code == C_Header || filetp->code == C_String);
#ifdef DEBUG
	if ( DEBUG('i') > 2)
	{
		(void)fputs("file token:", stderr);
		tk_pr(filetp, '\n');
	}
#endif
	filetp->ptr.string[filetp->rlen-1] = '\0';
	if (filetp->ptr.string[1] == '/') 
	{
		if (strncmp(filetp->ptr.string + 1, "/usr/include/", 13) == 0)
			WARN(gettxt(":455","#include of /usr/include/... may be non-portable"));
		path = pp_malloc(filetp->rlen);
		(void) strcpy(path, filetp->ptr.string + 1);
		DBGSEARCH(path);
		if (file = fopen(path, "r"))
			fl_next(path, file);
		else
		{
			nofilemsg(filetp);
			free(path);
		}
	}

	else
	{
		filetp->ptr.string[0] = '/';
		switch (filetp->code)
		{
		case C_String:
		{
			register int len;	/* num chars in file directory	*/
			register char * cp;	/* chars in current file path	*/
			char * curname;		/* current file path */
			char * slash;		/* last `/' in current file path */

			cp = curname = fl_curname();
			slash = (char *)0;
			do
			{
				if (*cp == '/')	slash = cp;
			} while (*cp++ != '\0');
			len = slash ? (int)(slash - curname) : 1;
			path = pp_malloc(filetp->rlen + len);
			(void) memcpy(path, slash ? curname : ".", len);
			(void) memcpy(path + len, filetp->ptr.string, filetp->rlen);
			DBGSEARCH(path);
				
			if (file = fopen(path,"r")) 
			{
				fl_next(path, file);
				break;
			}
			else {
				free(path); 
				if (errno == EMFILE) {
					nofilemsg(filetp);
					break;
				}
			}
		}
			/*FALLTHRU*/
		case C_Header:
		{
			register Token * dtp;	/* directory Token pointer  */

			dtp = dirtp;
			do
			{
				path = pp_malloc(dtp->rlen + filetp->rlen);
				(void) strncpy(path, dtp->ptr.string, dtp->rlen);
				(void) strcpy(path + dtp->rlen, filetp->ptr.string);
				DBGSEARCH(path);
				if (file = fopen(path, "r"))
				{
					fl_next(path, file);
					goto ret;
				}
				else
					free(path);
			} while ((dtp = dtp->next) != 0);
			nofilemsg(filetp);
		}
		}
	}
ret:	(void)tk_rm(filetp);
}

static void
sayfile()
/* Called when entering a new file, this routine
** prints out the line number of the next line
** and file information for the compiler.
*/
{
#ifdef DEBUG
	if (DEBUG('l') > 0)
		(void)fprintf(stderr, "sayfile called with: bf_lineno=%ld, outline=%ld\n",
		 bf_lineno, outline);
#endif
	if ((pp_flags & F_NO_DIRECTIVES) == 0)
	{
		(void)printf("# %ld \"%s\"\n", bf_lineno+1, filelist[idx].name);
		outline = bf_lineno;
	} 
}
