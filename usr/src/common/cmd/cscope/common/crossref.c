#ident	"@(#)cscope:common/crossref.c	1.7"

/*	cscope - interactive C symbol cross-reference
 *
 *	build cross-reference file
 */

#include "global.h"

/* convert long to a string */
#define	ltobase(value)	n = value; \
			s = buf + (sizeof(buf) - 1); \
			*s = '\0'; \
			digits = 1; \
			while (n >= BASE) { \
				++digits; \
				i = n; \
				n /= BASE; \
				*--s = i - n * BASE + '!'; \
			} \
			*--s = n + '!';

#define	SYMBOLINC	20	/* symbol list size increment */

long	dboffset;		/* new database offset */
BOOL	errorsfound;		/* prompt before clearing messages */
long	fileindex;		/* source file name index */
long	lineoffset;		/* source line database offset */
long	npostings;		/* number of postings */
int	nsrcoffset;		/* number of file name database offsets */
long	*srcoffset;		/* source file name database offsets */
int	symbols;		/* number of symbols */

static	char	*filename;	/* file name for warning messages */
static	long	fcnoffset;	/* function name database offset */
static	long	macrooffset;	/* macro name database offset */
static	int	msymbols = SYMBOLINC;	/* maximum number of symbols */
static	struct	symbol {	/* symbol data */
	int	type;		/* type */
	int	first;		/* index of first character in text */
	int	last;		/* index of last+1 character in text */
	int	length;		/* symbol length */
	int	fcn_level;	/* function level of the symbol */
} *symbol;

void	putcrossref();

static	void	savesymbol();

void
crossref(srcfile)
char	*srcfile;
{
	register int	i;
	register int	length;		/* symbol length */
	int	entry_no;		/* function level of the symbol */
	int	token;			/* current token */

	/* open the source file */
	if ((yyin = myfopen(srcfile, "r")) == NULL) {
		cannotopen(srcfile);
		errorsfound = YES;
		return;
	}
	filename = srcfile;	/* save the file name for warning messages */
	putfilename(srcfile);	/* output the file name */
	dbputc('\n');
	dbputc('\n');

	/* read the source file */
	initscanner(srcfile);
	fcnoffset = macrooffset = 0;
	symbols = 0;
	if (symbol == NULL) {
		symbol = (struct symbol *) mymalloc(msymbols * sizeof(struct symbol));
	}
	for (;;) {
		
		/* get the next token */
		switch (token = yylex()) {
		default:
			/* if requested, truncate C symbols */
			length = last - first;
			if (trun_syms == YES && length > 8 &&
			    token != INCLUDE && token != NEWFILE) {
				length = 8;
				last = first + 8;
			}
			/* see if the token has a symbol */
			if (length == 0) {
				savesymbol(token, entry_no);
				break;
			}
			/* update entry_no if see function entry */
			if (token == FCNDEF) {
				entry_no++;
			}
			/* see if the symbol is already in the list */
			for (i = 0; i < symbols; ++i) {
				if (length == symbol[i].length &&
				    strncmp(yytext + first, yytext +
					symbol[i].first, length) == 0 &&
					entry_no == symbol[i].fcn_level &&
				    token == symbol[i].type) {	/* could be a::a() */
					break;
				}
			}
			if (i == symbols) {	/* if not already in list */
				savesymbol(token, entry_no);
			}
			break;

		case NEWLINE:	/* end of line containing symbols */
			entry_no = 0;	/* reset entry_no for each line */
			--yyleng;	/* remove the newline */
			putcrossref();	/* output the symbols and source line */
			lineno = yylineno;	/* save the symbol line number */
			break;
			
		case LEXEOF:	/* end of file; last line may not have \n */
			
			/* if there were symbols, output them and the source line */
			if (symbols > 0) {
				putcrossref();
			}
			(void) fclose(yyin);	/* close the source file */

			/* output the leading tab expected by the next call */
			dbputc('\t');
			return;
		}
	}
}

/* save the symbol in the list */

static void
savesymbol(token, num)
int	token;
int 	num;
{
	/* make sure there is room for the symbol */
	if (symbols == msymbols) {
		msymbols += SYMBOLINC;
		symbol = (struct symbol *) myrealloc((char *) symbol,
		    msymbols * sizeof(struct symbol));
	}
	/* save the symbol */
	symbol[symbols].type = token;
	symbol[symbols].first = first;
	symbol[symbols].last = last;
	symbol[symbols].length = last - first;
	symbol[symbols].fcn_level = num;
	++symbols;
}

/* output the file name */

void
putfilename(srcfile)
char	*srcfile;
{
	/* check for file system out of space */
	/* note: dbputc is not used to avoid lint complaint */
	if (putc(NEWFILE, newrefs) == EOF) {
		cannotwrite(newreffile);
		/* NOTREACHED */
	}
	++dboffset;
	if (invertedindex == YES) {
		srcoffset[nsrcoffset++] = dboffset;
	}
	dbfputs(srcfile);
	fcnoffset = macrooffset = 0;
}

/* output the symbols and source line */

void
putcrossref()
{
	register int	i, j;
	register unsigned c;
	register BOOL	blank;		/* blank indicator */
	register int	symput = 0;	/* symbols output */
	int	type;

	/* output the source line */
	lineoffset = dboffset;
	dboffset += fprintf(newrefs, "%d ", lineno);
#if BSD && !sun
	dboffset = ftell(newrefs); /* fprintf doesn't return chars written */
#endif
	blank = NO;
	for (i = 0; i < yyleng; ++i) {
		
		/* change a tab to a blank and compress blanks */
		if ((c = yytext[i]) == ' ' || c == '\t') {
			blank = YES;
		}
		/* look for the start of a symbol */
		else if (symput < symbols && i == symbol[symput].first) {

			/* check for compressed blanks */
			if (blank == YES) {
				blank = NO;
				dbputc(' ');
			}
			dbputc('\n');	/* symbols start on a new line */
			
			/* output any symbol type */
			if ((type = symbol[symput].type) != IDENT) {
				dbputc('\t');
				dbputc(type);
			}
			else {
				type = ' ';
			}
			/* output the symbol */
			j = symbol[symput].last;
			c = yytext[j];
			yytext[j] = '\0';
			if (invertedindex == YES) {
				putposting(yytext + i, type);
			}
			writestring(yytext + i);
			dbputc('\n');
			yytext[j] = c;
			i = j - 1;
			++symput;
		}
		else {
			/* check for compressed blanks */
			if (blank == YES) {
				if (dicode2[c]) {
					c = (0200 - 2) + dicode1[' '] + dicode2[c];
				}
				else {
					dbputc(' ');
				}
			}
			/* compress digraphs */
			else if (dicode1[c] && (j = dicode2[(unsigned) yytext[i + 1]]) != 0 && 
			    symput < symbols && i + 1 != symbol[symput].first) {
				c = (0200 - 2) + dicode1[c] + j;
				++i;
			}
			dbputc((int) c);
			blank = NO;
			
			/* skip compressed characters */
			if (c < ' ') {
				++i;
				
				/* skip blanks before a preprocesor keyword */
				/* note: don't use isspace() because \f and \v
				   are used for keywords */
				while ((j = yytext[i]) == ' ' || j == '\t') {
					++i;
				}
				/* skip the rest of the keyword */
				while (isalpha(yytext[i])) {
					++i;
				}
				/* skip space after certain keywords */
				if (keyword[c].delim != '\0') {
					while ((j = yytext[i]) == ' ' || j == '\t') {
						++i;
					}
				}
				/* skip a '(' after certain keywords */
				if (keyword[c].delim == '(' && yytext[i] == '(') {
					++i;
				}
				--i;	/* compensate for ++i in for() */
			}
		}
	}
	/* ignore trailing blanks */
	dbputc('\n');
	dbputc('\n');

	/* output any #define end marker */
	/* note: must not be part of #define so putsource() doesn't discard it
	   so findcalledbysub() can find it and return */
	if (symput < symbols && symbol[symput].type == DEFINEEND) {
		dbputc('\t');
		dbputc(DEFINEEND);
		dbputc('\n');
		dbputc('\n');	/* mark beginning of next source line */
		macrooffset = 0;
	}
	symbols = 0;
}

/* output the inverted index posting */

void
putposting(term, type)
char	*term;
int	type;
{
	register long	i, n;
	register char	*s;
	register int	digits;		/* digits output */
	register long	offset;		/* function/macro database offset */
	char	buf[11];		/* number buffer */

	/* get the function or macro name offset */
	offset = fcnoffset;
	if (macrooffset != 0) {
		offset = macrooffset;
	}
	/* then update them to avoid negative relative name offset */
	switch (type) {
	case DEFINE:
		macrooffset = dboffset;
		break;
	case DEFINEEND:
		macrooffset = 0;
		return;		/* null term */
	case FCNDEF:
		fcnoffset = dboffset;
		break;
	case FCNEND:
		fcnoffset = 0;
		return;		/* null term */
	}
	/* ignore a null term caused by a enum/struct/union without a tag */
	if (*term == '\0') {
		return;
	}
	/* skip any #include secondary type char (< or ") */
	if (type == INCLUDE) {
		++term;
	}
	/* output the posting, which should be as small as possible to reduce
	   the temp file size and sort time */
	(void) fputs(term, postings);
	(void) putc(' ', postings);

	/* the line offset is padded so postings for the same term will sort
	   in ascending line offset order to order the references as they
	   appear withing a source file */
	ltobase(lineoffset);
	for (i = PRECISION - digits; i > 0; --i) {
		(void) putc('!', postings);
	}
	do {
		(void) putc(*s, postings);
	} while (*++s != '\0');
	
	/* postings are also sorted by type */
	(void) putc(type, postings);
	
	/* function or macro name offset */
	if (offset > 0) {
		(void) putc(' ', postings);
		ltobase(offset);
		do {
			(void) putc(*s, postings);
		} while (*++s != '\0');
	}
	if (putc('\n', postings) == EOF) {
		cannotwrite(temp1);
		/* NOTREACHED */
	}
	++npostings;
}

/* put the string into the new database */

void
writestring(s)
register char	*s;
{
	register unsigned c;
	register int	i;
	
	/* compress digraphs */
	for (i = 0; (c = s[i]) != '\0'; ++i) {
		if (dicode1[c] && dicode2[(unsigned) s[i + 1]]) {
			c = (0200 - 2) + dicode1[c] + dicode2[(unsigned) s[i + 1]];
			++i;
		}
		dbputc((int) c);
	}
}

/* print a warning message with the file name and line number */

void
warning(text)
char	*text;
{
	extern	int	yylineno;
	
	(void) fprintf(stderr, "cscope: \"%s\", line %d: warning: %s\n", filename, 
		yylineno, text);
	errorsfound = YES;
}
