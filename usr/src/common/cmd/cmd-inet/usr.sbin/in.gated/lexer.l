%{
#ident	"@(#)lexer.l	1.4"
#ident	"$Header$"
/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#define	GATED_LEXER
#define	INCLUDE_STAT
#define	INCLUDE_TIME
#define	INCLUDE_CTYPE

#include "include.h"
#include "parse.h"
#include "parser.h"

#undef	malloc
#define	malloc(x)	task_mem_malloc((task *) 0, x)
#undef	realloc
#define	realloc(x, y)	task_mem_realloc((task *) 0, x, y)
#undef	free
#define	free(x)		task_mem_free((task *) 0, x)

#define	printf(fmt, arg)	trace_tf(trace_global, TR_PARSE, 0, (fmt, arg));
#undef	yywrap
#define	yywrap	parse_eof

#define	parse_newname(fip)	{ \
    register char *new_name; \
    parse_filename = (new_name = (char *) rindex((fip)->fi_name, '/')) ? ++new_name : (fip)->fi_name; \
    }

#ifdef	YY_START
#define	YBEGIN(state) \
do { \
    trace_tf(trace_global, \
	     TR_PARSE, \
	     0, \
	     ("yylex: %s TRANSITION from %u to %u at %s:%u", \
	      parse_where(), \
	      YY_START, \
	      (state), \
	      __FILE__, \
	      __LINE__)); \
    BEGIN (state); \
} while (0)
#else	/* YY_START */
#define	YBEGIN(state) \
do { \
    trace_tf(trace_global, \
	     TR_PARSE, \
	     0, \
	     ("yylex: %s TRANSITION to %u at %s:%u", \
	      parse_where(), \
	      (state), \
	      __FILE__, \
	      __LINE__)); \
    BEGIN (state); \
} while (0)
#endif	/* YY_START */
    
#define	ATOI(str, store) \
	{ \
	    register char *Xcp = (str); \
	    register u_long Xacc = 0; \
	    while (*Xcp) { \
		Xacc = (Xacc << 3) + (Xacc << 1) + *Xcp++ - '0'; \
	    } \
	    (store) = Xacc; \
	}

#if	defined(FLEX_SCANNER)
#ifdef	CSRIMALLOC
#define	YY_USER_INIT	mal_verify(1)
#endif	/* CSRIMALLOC */
#define	FLEX_BUFFER_STATE
#define	SAVE_YY_STATE(fip)	(fip)->fi_state = YY_CURRENT_BUFFER
#define	NEW_YY_FILE(fip, file)	yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE))
#define	DELETE_YY_STATE(fip) yy_delete_buffer(YY_CURRENT_BUFFER)
#define	SET_YY_STATE(fip)	yy_switch_to_buffer((fip)->fi_state)
#else
#define	SAVE_YY_STATE(fip)	(fip)->fi_FILE = yyin
#define	NEW_YY_FILE(fip, file)	(fip)->fi_FILE = (file)
#define	DELETE_YY_STATE(fip)	fip->fi_FILE = NULL
#define	SET_YY_STATE(fip)	yyin = (fip)->fi_FILE
#endif

struct fi_info {
    char *fi_name;			/* File name */
    int fi_lineno;			/* Line number */
#ifdef	FLEX_BUFFER_STATE
    YY_BUFFER_STATE fi_state;		/* Input buffer state */
#else	/* FLEX_BUFFER_STATE */
    FILE *fi_FILE;			/* File pointer */
#endif	/* FLEX_BUFFER_STATE */
};

static struct fi_info parse_fi[FI_MAX+1];		/* Table of input files */
static int fi_file = 0;				/* Index into file table */
char *parse_directory = (char *) 0;

PROTOTYPE(parse_include,
	  static int,
	  (char *));
PROTOTYPE(parse_eof,
	  static int,
	  (void));

#ifdef	FLEX_SCANNER

#define	YYTEXT	yytext

int yylineno = 0;

#undef	YY_INPUT
#define	YY_INPUT(buf, result, max_size) \
{ \
    (result) = read(fileno(yyin), (buf), (size_t) (max_size)); \
    if ((result) < 0) { \
	YY_FATAL_ERROR("flex: YY_INPUT: read: %m"); \
    } else if ((result) == 0) { \
	(result) = YY_NULL; \
    } \
}

#define	YY_NEWLINE	yylineno++

#ifdef	YY_FATAL_ERROR
#undef	YY_FATAL_ERROR
#endif	/* YY_FATAL_ERROR */
#define	YY_FATAL_ERROR(msg)	{ trace_log_tf(trace_global, 0, LOG_ERR, (msg)); assert(FALSE); }

#else	/* FLEX_SCANNER */

#define	YYTEXT	(char *) yytext
#define	YY_NEWLINE

#endif	/* FLEX_SCANNER */

%}

%s CONFIG PP
%a 3000

nl				\n
ws				[ \t]+
digit				[0-9]
letter				[A-Za-z]
hexdigit			[0-9A-Fa-f]
octet				(([01]?{digit}?{digit})|((2([0-4]{digit}))|(25[0-5])))
dot				\.
hyphen				\-
special				[{}()\.\-+:\|\?\*,]
percent				\%
semi				\;
usec				{dot}{digit}{1,6}

comment				\#.*
ccomment			"/*"
bstring				\<[^>]*\>
qstring				\"[^"]*\"
number				{digit}+
hexpair				{hexdigit}{hexdigit}
hexstring			0[xX]{hexpair}+
hhexstring			{hexpair}+({hyphen}{hexpair}+)+
octetstring			{octet}({dot}{octet})+
isoaddr				{hexdigit}{hexdigit}{dot}{hexdigit}{4}({dot}{hexdigit}+)*
keyword				{letter}{letter}+
name				{letter}(({letter}|{digit}|{hyphen})*({letter}|{digit}))*
hname				{name}(({dot}{name})+{dot}?)?

%%
{comment}	{
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s COMMENT",
				  parse_where()));
		}
{ccomment}	{
			char ttchar;
			static const char *search = "*/";
			const char *sp = search;
			char buf[BUFSIZ];

			strcpy(buf, parse_where());

			while ((ttchar = input())) {
			    trace_tf(trace_global,
				     TR_PARSE,
				     0,
				     ("lex: %s comment: %c(%x)",
				      parse_where(),
				      ttchar,
				      ttchar));
			    if (ttchar == (char) EOF) {
				(void) sprintf(parse_error, "lex: unterminated C constant at %s",
					       buf);
				return L_ERROR;
			    } else if (ttchar == *sp) {
				if (!*++sp) {
				    break;
				}
			    } else {
				if (ttchar == '\n') {
				    YY_NEWLINE;
				}
				sp = search;
			    }
			}
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s COMMENT",
				  parse_where()));
		}
{ws}		{ ; }
{bstring}	{
			yylval.str.ptr = YYTEXT;
			yylval.str.len = yyleng;
			yylval.str.type = L_STRING;
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s L_STRING: %s length %d",
				  parse_where(),
				  yylval.str.ptr,
				  yylval.str.len));
			return yylval.str.type;
		}
{qstring}	{
			yylval.str.ptr = YYTEXT;
			yylval.str.len = yyleng;
			yylval.str.type = L_STRING;
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s L_STRING: %s length %d",
				  parse_where(),
				  yylval.str.ptr,
				  yylval.str.len));
			return yylval.str.type;
		}

{name}	{
			int key_token;

			yylval.str.ptr = YYTEXT;
			yylval.str.len = yyleng;
			key_token = parse_keyword(YYTEXT, (u_int) yyleng);
			if (key_token) {
			    yylval.str.type = key_token;
			    trace_tf(trace_global,
				     TR_PARSE,
				     0,
				     ("lex: %s KEYWORD: %s (%d)",
				      parse_where(),
				      YYTEXT,
				      key_token));
			} else {
			    yylval.str.type = L_KEYWORD;
			    trace_tf(trace_global,
				     TR_PARSE,
				     0,
				     ("lex: %s L_KEYWORD: %s length: %d",
				      parse_where(),
				      yylval.str.ptr,
				      yylval.str.len));
			}
			return yylval.str.type;
		}
<PP>{nl}	{
			YY_NEWLINE;
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s L_EOS",
				  parse_where()));
			YBEGIN(CONFIG);
			return L_EOS;
		}

<CONFIG>{nl}	{
			YY_NEWLINE;
		}
<CONFIG>{semi}	{
    			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s L_EOS",
				  parse_where()));
			return L_EOS;
		}
<CONFIG>^{percent}{ws}?[iI][nN][cC][lL][uU][dD][eE]{ws}?{qstring}{ws}?{nl} {
    			register char *cp = YYTEXT;
			char *name;

			while (*cp++ != '"') ;
			name = cp;
			while (*cp != '"') {
			    cp++;
			}
			*cp = (char) 0;
			cp = name;

#ifdef	FLAT_FS
			name = task_mem_strdup((task *) 0, cp);
#else	/* FLAT_FS */
			switch (*cp) {
			case '/':
			    name = task_mem_strdup((task *) 0,
						   cp);
			    break;

			default:
			    name = task_mem_malloc((task *) 0,
						   (size_t) (strlen(cp) + strlen(parse_directory) + 2));

			    (void) strcpy(name, parse_directory);
			    (void) strcat(name, "/");
			    (void) strcat(name, cp);
			}
#endif	/* FLAT_FS */
			
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s INCLUDE '%s'",
				  parse_where(),
				  name));

			/* Count the newline in this file */
    			YY_NEWLINE;

			if (parse_include(name)) {
			    return L_ERROR;
			}
			/* String is freed by parse_eof() */
}
<CONFIG>^{percent}{ws}?[dD][iI][rR][eE][cC][tT][oO][rR][yY]{ws}?{qstring}{ws}?{nl} {
    			register char *cp = YYTEXT;
			char *name;
#ifndef	NO_STAT
			struct stat stbuf;
#endif	/* NO_STAT */

			while (*cp++ != '"') ;
			name = cp;
			while (*cp != '"') {
			    cp++;
			}
			*cp-- = (char) 0;
			if (*cp == '/') {
			    *cp-- = (char) 0;
			}

#ifndef	NO_STAT
			if (stat(name, &stbuf) < 0) {
			    (void) sprintf(parse_error, "stat(%s): %m",
					   name);
			    return L_ERROR;
			}

			switch (stbuf.st_mode & S_IFMT) {
			case S_IFDIR:
			    break;

			default:
			    (void) sprintf(parse_error, "%s is not a directory",
					   name);
			    return L_ERROR;
			}
#endif	/* NO_STAT */

			if (parse_directory) {
			    task_mem_free((task *) 0,
					  parse_directory);
			}
			parse_directory = task_mem_strdup((task *) 0,
							  name);
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s included file prefeix now %s",
				  parse_where(),
				  parse_directory));
}
<CONFIG>^{percent}	{
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s '%s'",
				  parse_where(),
				  YYTEXT));
			YBEGIN(PP);
			return (char) *yytext;
		}
<CONFIG>{hname}	{
			yylval.str.ptr = YYTEXT;
			yylval.str.len = yyleng;
			yylval.str.type = L_HNAME;
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s HNAME: %s length %d",
				  parse_where(),
				  yylval.str.ptr,
				  yylval.str.len));
			return yylval.str.type;
		}
<CONFIG>{special}	{
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s '%s'",
				  parse_where(),
				  YYTEXT));
			return (char) *yytext;
		}
<CONFIG>{number}	{
			ATOI(YYTEXT, yylval.num);
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s L_NUMBER: %d",
				  parse_where(),
				  yylval.num));
			return L_NUMBER;
		}
<CONFIG>{usec}	{
			char usecs[7];
			register int i;
			register char *cp = YYTEXT;

			for (i = 0; i < 6; i++) {
			    usecs[i] = *cp ? *cp++ : '0';
			}

			ATOI(usecs, yylval.num);
			trace_tf(trace_global,
				 TR_PARSE,
				 0,
				 ("lex: %s L_USEC: %u",
				  parse_where(),
				  yylval.num));
			return L_USEC;
		}
<CONFIG>{isoaddr}	{
#define	atox(c)	(isdigit(c) ? (c - '0') : ( isupper(c) ? (c - 'A' + 10) : (c - 'a' + 10) ))
#ifdef	PROTO_ISO
			register int i = 0;
			register u_char acc = 0;
			register char *cp;
			register byte *dp;

			for (cp = YYTEXT; *cp; cp++) {
			    if (*cp != '.') {
				i++;
			    }
			}
			yylval.hex.len = (i + 1)/2;
			yylval.hex.ptr = (byte *) task_mem_malloc((task *) 0, yylval.hex.len);
			yylval.hex.type = L_ISOADDR;
			dp = yylval.hex.ptr;
			cp = YYTEXT;

			/* Pick up string ignoring trailing dots */
			i = 0;
			do {
			    switch (*cp) {
			    case '.':
				/* Ignore dots */
				break;

			    case 0:
				if (i) {
				    /* Stuff the trailing nibble */

				    *dp = acc;
				}
				/* This will cause the loop to break */
				cp = (char *) 0;
				break;

			    default:
				if ((i = !i)) {
				    /* First nibble */

				    acc = atox(*cp) << 4;
				} else {
				    /* Second nibble */

				    *dp++ = acc | atox(*cp);
				}
				break;
			    }			    
			} while (cp++);

			if (TRACE_TF(trace_global, TR_PARSE)) {
			    tracef("lex: %s L_ISOADDR: length %d ",
				   parse_where(),
				   yylval.hex.len);
			    for (i = 0; i < yylval.hex.len; i++) {
				tracef("%02x",
				       yylval.hex.ptr[i]);
			    }
			    trace_only_tf(trace_global,
					  0,
					  (NULL));
			}
			return yylval.hex.type;
#else	/* PROTO_ISO */
			return (char) *yytext;
#endif	/* PROTO_ISO */
		    }
<CONFIG>{octetstring}  {
    			int i;
    			char *cp;

			/* First count the number of octets */
			yylval.hex.len = 1;
			for (cp = YYTEXT; (cp = (char *) index(cp, '.')); *cp++ = (char) 0) {
			    yylval.hex.len++;
			}

			yylval.hex.ptr = (byte *) task_mem_malloc((task *) 0, yylval.hex.len);
			yylval.hex.type = L_OCTETSTRING;

			cp = YYTEXT;
			for (i = 0; i < yylval.hex.len; i++) {
			    ATOI(cp, yylval.hex.ptr[i]);
			    while (*cp++) ;
			}

			if (TRACE_TF(trace_global, TR_PARSE)) {
			    tracef("lex: %s L_OCTETSTRING: length %d ",
				   parse_where(),
				   yylval.hex.len);
			    for (i = 0; i < yylval.hex.len; i++) {
				tracef("%s%u",
				       i ? "." : "",
				       yylval.hex.ptr[i]);
			    }
			    trace_only_tf(trace_global,
					  0,
					  (NULL));
			}

			return L_OCTETSTRING;
		    }
<CONFIG>{hhexstring}	{
			int i = 0;
			char *cp;
			byte *dp;

			for (cp = YYTEXT; *cp; cp++) {
			    if (*cp != '-') {
				i++;
			    }
			}
			yylval.hex.len = (i + 1)/2;
			yylval.hex.ptr = (byte *) task_mem_malloc((task *) 0, yylval.hex.len);
			dp = yylval.hex.ptr;
			cp = YYTEXT;
			yylval.hex.type = L_HEXSTRING;
			for (; *cp; cp++, dp++) {
			    if (*cp == '-') {
				cp++;
			    }
			    *dp = atox(*cp) << 4;
			    cp++;
			    if (*cp == '-') {
				cp++;
			    }
			    *dp |= atox(*cp);
			}
			if (TRACE_TF(trace_global, TR_PARSE)) {
			    tracef("lex: %s L_HEXSTRING: length %d ",
				   parse_where(),
				   yylval.hex.len);
			    for (i = 0; i < yylval.hex.len; i++) {
				tracef("%02x",
				       yylval.hex.ptr[i]);
			    }
			    trace_only_tf(trace_global,
					  0,
					  (NULL));
			}
			return yylval.hex.type;
		    }
<CONFIG>{hexstring}	{
			int i = yyleng - 2;
    			char *cp = YYTEXT + 2;
			byte *dp;

			yylval.hex.len = (i + 1)/2;
			yylval.hex.ptr = (byte *) task_mem_malloc((task *) 0, yylval.hex.len);
			dp = yylval.hex.ptr;
			yylval.hex.type = L_HEXSTRING;
			for (; *cp; cp += 2, dp++) {
			    *dp = (atox(cp[0]) << 4) | atox(cp[1]);
			}
			if (TRACE_TF(trace_global, TR_PARSE)) {
			    tracef("lex: %s L_HEXSTRING: length %d 0x",
				   parse_where(),
				   yylval.hex.len);
			    for (i = 0; i < yylval.hex.len; i++) {
				tracef("%02x",
				       yylval.hex.ptr[i]);
			    }
			    trace_only_tf(trace_global,
					  0,
					  (NULL));
			}
			return yylval.hex.type;
		    }
.		{
    			(void) sprintf(parse_error, "unrecognized character: `%s'",
				       YYTEXT);
			return L_ERROR;
		}
%%

#if	!defined(FLEX_SCANNER) || !defined(YY_USER_ACTION)

/* For parsers that do not define yyrestart() */
/* Define it as a function so we get an error if it is already defined */
void
yyrestart(fp)
FILE *fp;
{
    yyin = fp;
}
#else	/* !defined(FLEX_SCANNER) || !defined(YY_USER_ACTION) */

/* Assign yyin the first time and call yyrestart() other times */
#define	YYRESTART_MAGIC

#endif	/* !defined(FLEX_SCANNER) || !defined(YY_USER_ACTION) */


/*
 *	Open the first file (called from parse_parse).  If no file specified,
 *	assume stdin (for testing ).
 */
int
parse_open(name)
char *name;
{
#if	defined(YYRESTART_MAGIC)
    static int first;
#endif	/* defined(YYRESTART_MAGIC) */
    struct fi_info *fip;
    FILE *file;

    fip = &parse_fi[fi_file = 0];

    if (name) {
	file = fopen(name, "r");
	if (!file) {
	    trace_log_tf(trace_global,
			 0,
			 LOG_ERR,
			 ("parse_open: error opening %s: %m",
			  name));
	    return TRUE;
	}
    } else {
#if	YYDEBUG != 0
	name = "stdin";
#else	/* YYDEBUG */
	trace_log_tf(trace_global,
		     0,
		     LOG_ERR,
		     ("parse_open: no file specified"));
	return TRUE;
#endif	/* YYDEBUG */
    }

    yylineno = 1;
#ifdef	YYRESTART_MAGIC
    if (first++) {
	yyrestart(file);
    } else {
	yyin = file;
    }
#else	/* YYRESTART_MAGIC */
    yyrestart(file);
#endif	/* YYRESTART_MAGIC */

    fip->fi_name = name;
    parse_newname(fip);
    trace_tf(trace_global,
	     TR_PARSE,
	     0,
	     ("parse_open: reading %s (%d)",
	      fip->fi_name,
	      fileno(file)));
	
    YBEGIN(CONFIG);
    return FALSE;
}


/*
 *	Process an include directive, read another input file
 */
static int
parse_include __PF1(name, char *)
{
    struct fi_info *fip;
    FILE *file;

    /* Verify file name is unique (this is easy to fool) */
    for (fip = parse_fi; fip <= &parse_fi[fi_file]; fip++) {
	if (fip->fi_name && !strcmp(name, fip->fi_name)) {
	    (void) sprintf(parse_error, "recursive includes: %s",
			   name);
	    return TRUE;
	}
    }

    /* Check nesting level */
    if (fi_file >= FI_MAX) {
	(void) sprintf(parse_error, "too many levels of includes");
	return TRUE;
    }

    /* Try to open new file */
    file = fopen(name, "r");
    if (!file) {
	(void) sprintf(parse_error, "error opening %s: %m",
		       name);
	return TRUE;
    }

    /* Update line number of current file and make sure we have correct file pointer */
    fip = &parse_fi[fi_file++];
    SAVE_YY_STATE(fip);
    fip->fi_lineno = yylineno;

    fip++;
    fip->fi_name = name;
    yylineno = 1;
    yyin = file;
    NEW_YY_FILE(fip, file);
    parse_newname(fip);
    trace_tf(trace_global,
	     TR_PARSE,
	     0,
	     ("parse_include: %s now reading %s (%d)",
	      parse_where(),
	      fip->fi_name,
	      fileno(file)));

    return FALSE;
}


/* parse_eof - process end of file on current input file */
static int
parse_eof()
{
    struct fi_info *fip;

    if (fi_file < 0) {
	return TRUE;
    }
    
    fip = &parse_fi[fi_file];
    trace_tf(trace_global,
	     TR_PARSE,
	     0,
	     ("parse_eof: %s EOF on %s (%d)",
	      parse_where(),
	      fip->fi_name,
	      fileno(yyin)));
    if (fclose(yyin)) {
	trace_log_tf(trace_global,
		     0,
		     LOG_ERR,
		     ("parse_eof: error closing %s: %m",
		      fip->fi_name));
    }
    task_mem_free((task *) 0, fip->fi_name);
    fip->fi_name = NULL;
    parse_filename = NULL;

    if (fi_file-- > 0) {
	DELETE_YY_STATE(fip);
	fip = &parse_fi[fi_file];
	parse_newname(fip);
	yylineno = fip->fi_lineno;
	SET_YY_STATE(fip);
	trace_tf(trace_global,
		 TR_PARSE,
		 0,
		 ("parse_eof: %s resuming %s (%d)",
		  parse_where(),
		  fip->fi_name,
		  fileno(yyin)));
	return FALSE;
    }
    yyin = (FILE *) 0;
    trace_tf(trace_global,
	     TR_PARSE,
	     0,
	     ("parse_eof: %s EOF",
	      parse_where()));
    return TRUE;
}
