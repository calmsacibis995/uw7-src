#ident	"@(#)cscope:common/find.c	1.16"
/*	cscope - interactive C symbol or text cross-reference
 *
 *	searching functions
 */

#include "global.h"
#include <curses.h>

/* most of these functions have been optimized so their innermost loops have
 * only one test for the desired character by putting the char and 
 * an end-of-block marker (\0) at the end of the disk block buffer.
 * When the inner loop exits on the char, an outer loop will see if
 * the char is followed by a \0.  If so, it will read the next block
 * and restart the inner loop.
 */

char	*blockp;			/* pointer to current char in block */
char	block[BUFSIZ + 2];		/* leave room for end-of-block mark */
int	blocklen;			/* length of disk block read */
char	blockmark;			/* mark character to be searched for */
long	blocknumber;			/* block number */

static	char	global[] = "<global>";	/* dummy global function name */
static	char	cpattern[PATLEN + 1];	/* compressed pattern */
static	long	lastfcnoffset;		/* last function name offset */
static	POSTING	*postingp;		/* retrieved posting set pointer */
static	long	postingsfound;		/* retrieved number of postings */
static	char	*regexp;		/* regular expression */

void	boolclear();
POSTING	*boolfile();

static	BOOL	match(), matchrest();
static	POSTING	*getposting();
static	char	*lcasify();
static	void	findcalledbysub(), findterm(), putline(), putpostingref(), 
		putref(), putsource();

/* find the symbol in the cross-reference */

void
findsymbol()
{
	char	file[PATHLEN + 1];	/* source file name */
	char	function[PATLEN + 1];	/* function name */
	char	macro[PATLEN + 1];	/* macro name */
	char	symbol[PATLEN + 1];	/* symbol name */
	register int	c;
	register char	*cp;
	char	*s;
	char firstchar;		/* first character of a potential symbol */
	BOOL fcndef = NO;

	if (invertedindex == YES) {
		long	lastline = 0;
		POSTING *p;

		findterm();
		while ((p = getposting()) != NULL) {
			if (p->type != INCLUDE && p->lineoffset != lastline) {
				putpostingref(p, 0);
				lastline = p->lineoffset;
			}
		}
		return;
	}

	(void) scanpast('\t');			/* find the end of the header */
	skiprefchar();			/* skip the file marker */
	putstring(file);		/* save the file name */
	(void) strcpy(function, global);/* set the dummy global function name */
	(void) strcpy(macro, global);/* set the dummy global macro name */
	
	/* find the next symbol */
	/* note: this code was expanded in-line for speed */
	/* other macros were replaced by code using cp instead of blockp */
	cp = blockp;
	for (;;) {
		setmark('\n');
		do {	/* innermost loop optimized to only one test */
			while (*cp != '\n') {
				++cp;
			}
		} while (*(cp + 1) == '\0' && (cp = readblock()) != NULL);

		/* skip the found character */
		if (cp != NULL && *(++cp + 1) == '\0') {
			cp = readblock();
		}
		if (cp == NULL) {
			break;
		}
		/* look for a source file, function, or macro name */
		if (*cp == '\t') {
			blockp = cp;
			switch (getrefchar()) {

			case NEWFILE:		/* file name */

				/* save the name */
				skiprefchar();
				putstring(file);
			
				/* check for the end of the symbols */
				if (*file == '\0') {
					return;
				}
				progress();
				/* FALLTHROUGH */
				
			case FCNEND:		/* function end */
				(void) strcpy(function, global);
				goto notmatched;	/* don't match name */
				
			case FCNDEF:		/* function name */
				fcndef = YES;
				s = function;
				break;

			case DEFINE:		/* macro name */
				if (fileversion >= 10) {
					s = macro;
				}
				else {	
					s = symbol;
				}
				break;

			case DEFINEEND:		/* macro end */
				(void) strcpy(macro, global);
				goto notmatched;

			case INCLUDE:			/* #include file */
				goto notmatched;	/* don't match name */
			
			default:		/* other symbol */
				s = symbol;
			}
			/* save the name */
			skiprefchar();
			putstring(s);

			/* see if this is a regular expression pattern */
			if (regexp != NULL) {
				if (caseless == YES) {
					s = lcasify(s);
				}
				if (*s != '\0' && regex(regexp, s) != NULL) {
					goto matched;
				}
			}
			/* match the symbol to the text pattern */
			else if (strequal(pattern, s)) {
				goto matched;
			}
			goto notmatched;
		}
		/* if this is a regular expression pattern */
		if (regexp != NULL) {
			
			/* if this is a symbol */
			
			/**************************************************
			 * The first character may be a digraph'ed char, so
			 * unpack it into firstchar, and then test that.
			 *
			 * Assume that all digraphed chars have the 8th bit
			 * set (0200).
			 **************************************************/
			if (*cp & 0200) {	/* digraph char? */
				firstchar = dichar1[(*cp & 0177) / 8];
			}
			else {
				firstchar = *cp;
			}
			
			if (isalpha(firstchar) || firstchar == '_') {
				blockp = cp;
				putstring(symbol);
				if (caseless == YES) {
					s = lcasify(symbol);	/* point to lower case version */
				}
				else {
					s = symbol;
				}
				
				/* match the symbol to the regular expression */
				if (regex(regexp, s) != NULL) {
					goto matched;
				}
				goto notmatched;
			}
		}
		/* match the character to the text pattern */
		else if (*cp == cpattern[0]) {
			blockp = cp;

			/* match the rest of the symbol to the text pattern */
			if (matchrest()) {
				s = NULL;
		matched:
				/* output the file, function or macro, and source line */
				if (strcmp(macro, global) && s != macro) {
					putref(0, file, macro);
				}
				else if (fcndef == YES || s != function) {
					fcndef = NO;
					putref(0, file, function);
				}
				else {
					putref(0, file, global);
				}
				if (blockp == NULL) {
					return;
				}
			}
		notmatched:
			cp = blockp;
		}
	}
	blockp = cp;
}
/* find the function definition or #define */

void
finddef()
{
	char	file[PATHLEN + 1];	/* source file name */

	if (invertedindex == YES) {
		POSTING *p;

		findterm();
		while ((p = getposting()) != NULL) {
			switch (p->type) {
			case DEFINE:/* could be a macro */
			case FCNDEF:
			case CLASSDEF:
			case ENUMDEF:
			case MEMBERDEF:
			case STRUCTDEF:
			case TYPEDEF:
			case UNIONDEF:
			case GLOBALDEF:/* other global definition */
				putpostingref(p, pattern);
			}
		}
		return;
	}


	/* find the next file name or definition */
	while (scanpast('\t') != NULL) {
		switch (*blockp) {
			
		case NEWFILE:
			skiprefchar();	/* save file name */
			putstring(file);
			if (*file == '\0') {	/* if end of symbols */
				return;
			}
			progress();
			break;

		case DEFINE:		/* could be a macro */
		case FCNDEF:
		case CLASSDEF:
		case ENUMDEF:
		case MEMBERDEF:
		case STRUCTDEF:
		case TYPEDEF:
		case UNIONDEF:
		case GLOBALDEF:		/* other global definition */
			skiprefchar();	/* match name to pattern */
			if (match()) {
		
				/* output the file, function and source line */
				putref(0, file, pattern);
			}
			break;
		}
	}
}
/* find all function definitions (used by samuel only) */

void
findallfcns()
{
	char	file[PATHLEN + 1];	/* source file name */
	char	function[PATLEN + 1];	/* function name */

	/* find the next file name or definition */
	while (scanpast('\t') != NULL) {
		switch (*blockp) {
			
		case NEWFILE:
			skiprefchar();	/* save file name */
			putstring(file);
			if (*file == '\0') {	/* if end of symbols */
				return;
			}
			progress();
			/* FALLTHROUGH */
			
		case FCNEND:		/* function end */
			(void) strcpy(function, global);
			break;

		case FCNDEF:
		case CLASSDEF:
			skiprefchar();	/* save function name */
			putstring(function);

			/* output the file, function and source line */
			putref(0, file, function);
			break;
		}
	}
}

/* find the functions calling this function */

void
findcalling()
{
	char	file[PATHLEN + 1];	/* source file name */
	char	function[PATLEN + 1];	/* function name */
	char	tmpfunc[10][PATLEN + 1];/* 10 temporary function names */
	char	macro[PATLEN + 1];	/* macro name */
	char	*tmpblockp;
	int	morefuns, i;

	if (invertedindex == YES) {
		POSTING	*p;
		
		findterm();
		while ((p = getposting()) != NULL) {
			if (p->type == FCNCALL) {
				putpostingref(p, 0);
			}
		}
		return;
	}
	/* find the next file name or function definition */
	*macro = '\0';	/* a macro can be inside a function, but not vice versa */
	tmpblockp = 0;
	morefuns = 0;	/* one function definition is normal case */
	for (i = 0; i < 10; i++) *(tmpfunc[i]) = '\0';
	while (scanpast('\t') != NULL) {
		switch (*blockp) {
			
		case NEWFILE:		/* save file name */
			skiprefchar();
			putstring(file);
			if (*file == '\0') {	/* if end of symbols */
				return;
			}
			progress();
			(void) strcpy(function, global);
			break;
			
		case DEFINE:		/* could be a macro */
			if (fileversion >= 10) {
				skiprefchar();
				putstring(macro);
			}
			break;

		case DEFINEEND:
			*macro = '\0';
			break;

		case FCNDEF:		/* save calling function name */
			skiprefchar();
			putstring(function);
			for (i = 0; i < morefuns; i++)
				if ( !strcmp(tmpfunc[i], function) )
					break;
			if (i == morefuns) {
				(void) strcpy(tmpfunc[morefuns], function);
				if (++morefuns >= 10) morefuns = 9;
			}
			break;
			
		case FCNEND:
			for (i = 0; i < morefuns; i++)
				*(tmpfunc[i]) = '\0';
			morefuns = 0;
			break;

		case FCNCALL:		/* match function called to pattern */
			skiprefchar();
			if (match()) {
				
				/* output the file, calling function or macro, and source */
				if (*macro != '\0') {
					putref(1, file, macro);
				}
				else {
					tmpblockp = blockp;
					for (i = 0; i < morefuns; i++) {
						blockp = tmpblockp;
						putref(1, file, tmpfunc[i]);
					}
				}
			}
		}
	}
	morefuns = 0;
}

/* find the text in the source files */

char *
findstring()
{
	char	egreppat[2 * PATLEN];
	char	*cp, *pp;

	/* translate special characters in the regular expression */
	cp = egreppat;
	for (pp = pattern; *pp != '\0'; ++pp) {
		if (strchr(".*[\\^$+?|()", *pp) != NULL) {
			*cp++ = '\\';
		}
		*cp++ = *pp;
	}
	*cp = '\0';
	
	/* search the source files */
	return(findregexp(egreppat));
}

/* find this regular expression in the source files */

char *
findregexp(egreppat)
char	*egreppat;
{
	register int	i;
	char	*egreperror;

	/* compile the pattern */
	if ((egreperror = egrepinit(egreppat)) == NULL) {

		/* search the files */
		for (i = 0; i < nsrcfiles; ++i) {
			register char *file = filepath(srcfiles[i]);
			progress();
			if (egrep(file, refsfound, "%s <unknown> %ld ") < 0) {
				move(1, 0);
				clrtoeol();
				printw("Cannot open file %s", file);
				refresh();
			}
		}
	}
	return(egreperror);
}

/* find matching file names */

void
findfile()
{
	register int	i;
	
	for (i = 0; i < nsrcfiles; ++i) {
		char *s;
		if (caseless == YES) {
			s = lcasify(srcfiles[i]);
		}
		else {
			s = srcfiles[i];
		}
		if (regex(regexp, s) != NULL) {
			(void) fprintf(refsfound, "%s <unknown> 1 <unknown>\n", 
				srcfiles[i]);
		}
	}
}

/* find files #including this file */

void
findinclude()
{
	char	file[PATHLEN + 1];	/* source file name */

	if (invertedindex == YES) {
		POSTING *p;

		findterm();
		while ((p = getposting()) != NULL) {
			if (p->type == INCLUDE) {
				putpostingref(p, 0);
                        }
                }
                return;
        }

	/* find the next file name or function definition */
	while (scanpast('\t') != NULL) {
		switch (*blockp) {
			
		case NEWFILE:		/* save file name */
			skiprefchar();
			putstring(file);
			if (*file == '\0') {	/* if end of symbols */
				return;
			}
			progress();
			break;
			
		case INCLUDE:		/* match function called to pattern */
			skiprefchar();
			skiprefchar();	/* skip global or local #include marker */
			if (match()) {
				
				/* output the file and source line */
				putref(0, file, global);
			}
		}
	}
}

/* initialize */

FINDINIT
findinit()
{
	char	buf[PATLEN + 3];
	BOOL	isregexp = NO;
	register int	i;
	register char	*s;
	register unsigned c;

	/* remove trailing white space */
	for (s = pattern + strlen(pattern) - 1; isspace(*s); --s) {
		*s = '\0';
	}
	/* allow a partial match for a file name */
	if (field == FILENAME || field == INCLUDES) {
		
		if ((regexp = regcmp(pattern, (char *) NULL)) == NULL) {
			return(REGCMPERROR);
		}
		return(NOERROR);
	}
	/* see if the pattern is a regular expression */
	if (strpbrk(pattern, "^.[{*+$") != NULL) {
		isregexp = YES;
	}
	else {
		/* check for a valid C symbol */
		s = pattern;
		if (!isalpha(*s) && *s != '_') {
			return(NOTSYMBOL);
		}
		while (*++s != '\0') {
			if (!isalnum(*s) && *s != '_') {
				return(NOTSYMBOL);
			}
		}
		/* look for use of the -T option (truncate symbol to 8
		   characters) on a database not built with -T */
		if (trun_syms == YES && isuptodate == YES &&
		    dbtruncated == NO && s - pattern >= 8) {
			(void) strcpy(pattern + 8, ".*");
			isregexp = YES;
		}
	}
	/* if this is a regular expression or letter case is to be ignored */
	/* or there is an inverted index */
	if (isregexp == YES || caseless == YES || invertedindex == YES) {

		/* remove a leading ^ */
		s = pattern;
		if (*s == '^') {
			(void) strcpy(newpat, s + 1);
			(void) strcpy(s, newpat);
		}
		/* remove a trailing $ */
		i = strlen(s) - 1;
		if (s[i] == '$') {
			if (i > 0  && s[i-1] == '\\' ) {
				s[i-1] = '$';
			}
			s[i] = '\0';
		}
		/* if requested, try to truncate a C symbol pattern */
		if (trun_syms == YES && strpbrk(s, "[{*+") == NULL) {
			s[8] = '\0';
		}
		/* must be an exact match */
		/* note: regcmp doesn't recognize ^*keypad$ as a syntax error
		   unless it is given as a single arg */
		(void) sprintf(buf, "^%s$", s);
		if ((regexp = regcmp(buf, (char *) NULL)) == NULL) {
			return(REGCMPERROR);
		}
	}
	else {
		/* if requested, truncate a C symbol pattern */
		if (trun_syms == YES && field <= CALLING) {
			pattern[8] = '\0';
		}
		/* compress the string pattern for matching */
		s = cpattern;
		for (i = 0; (c = pattern[i]) != '\0'; ++i) {
			if (dicode1[c] && dicode2[(unsigned) pattern[i + 1]]) {
				c = (0200 - 2) + dicode1[c] + dicode2[(unsigned) pattern[i + 1]];
				++i;
			}
			*s++ = c;
		}
		*s = '\0';
	}
	return(NOERROR);
}

void
findcleanup()
{
	/* discard any regular expression */
	if (regexp != NULL) {
		free(regexp);
		regexp = NULL;
	}
}

/* match the pattern to the string */

static BOOL
match()
{
	char	string[PATLEN + 1];

	/* see if this is a regular expression pattern */
	if (regexp != NULL) {
		putstring(string);
		if (*string == '\0') {
			return(NO);
		}
		if (caseless == YES) {
			return(regex(regexp, lcasify(string)) ? YES : NO);
		}
		else {
			return(regex(regexp, string) ? YES : NO);
		}
	}
	/* it is a string pattern */
	return((BOOL) (*blockp == cpattern[0] && matchrest()));
}

/* match the rest of the pattern to the name */

static BOOL
matchrest()
{
	int	i = 1;
	
	skiprefchar();
	do {
		while (*blockp == cpattern[i]) {
			++blockp;
			++i;
		}
	} while (*(blockp + 1) == '\0' && readblock() != NULL);
	
	if (*blockp == '\n' && cpattern[i] == '\0') {
		return(YES);
	}
	return(NO);
}

/* put the reference into the file */

static void
putref(seemore, file, func)
int	seemore;
char	*file;
char	*func;
{
	FILE	*output;

	if (strcmp(func, global) == 0) {
		output = refsfound;
	}
	else {
		output = nonglobalrefs;
	}
	(void) fprintf(output, "%s %s ", file, func);
	putsource(seemore, output);
}

/* put the source line into the file */

static void
putsource(seemore, output)
int	seemore;
FILE	*output;
{
	char *tmpblockp;
	register char	*cp, nextc = '\0';
	BOOL Change = NO, retreat = NO;
	
	if (fileversion <= 5) {
		(void) scanpast(' ');
		putline(output);
		(void) putc('\n', output);
		return;
	}
	/* scan back to the beginning of the source line */
	cp = tmpblockp = blockp;
	while (*cp != '\n' || nextc != '\n') {
		nextc = *cp;
		if (--cp < block) {
			retreat = YES;
			/* read the previous block */
			(void) dbseek((blocknumber - 1) * BUFSIZ);
			cp = &block[BUFSIZ - 1];
		}
	}
	blockp = cp;
	if (*blockp != '\n' || getrefchar() != '\n' || 
		!isdigit(getrefchar()) && fileversion >= 12) {
			postmsg("Internal error: cannot get source line from database");
			myexit(1);
	}
	/* until a double newline is found */
	do {
		/* skip a symbol type */
		if (*blockp == '\t') {
			/* if retreat == YES, that means tmpblockp and blockp
			 * point to different blocks.  Offset comparison should
			 * NOT be performed until they point to the same block.
			 */
 			if (seemore && Change == NO && retreat == NO &&
				blockp > tmpblockp) {
					Change = YES;
					cp = blockp;
			}
			skiprefchar();
			skiprefchar();
		}
		/* output a piece of the source line */
		putline(output);
		if (retreat == YES) retreat = NO;
	} while (blockp != NULL && getrefchar() != '\n');
	(void) putc('\n', output);
	if (Change == YES) blockp = cp;
}

/* put the rest of the cross-reference line into the file */

static void
putline(output)
FILE	*output;
{
	register char	*cp;
	register unsigned c;
	
	setmark('\n');
	cp = blockp;
	do {
		while ((c = (unsigned)(*cp)) != '\n') {
			
			/* check for a compressed digraph */
			if (c > '\177') {
				c &= 0177;
				(void) putc(dichar1[c / 8], output);
				(void) putc(dichar2[c & 7], output);
			}
			/* check for a compressed keyword */
			else if (c < ' ') {
				(void) fputs(keyword[c].text, output);
				if (keyword[c].delim != '\0') {
					(void) putc(' ', output);
				}
				if (keyword[c].delim == '(') {
					(void) putc('(', output);
				}
			}
			else {
				(void) putc((int) c, output);
			}
			++cp;
		}
	} while (*(cp + 1) == '\0' && (cp = readblock()) != NULL);
	blockp = cp;
}

/* put the rest of the cross-reference line into the string */

void
putstring(s)
register char	*s;
{
	register char	*cp;
	register unsigned c;
	
	setmark('\n');
	cp = blockp;
	do {
		while ((c = (unsigned)(*cp)) != '\n') {
			if (c > '\177') {
				c &= 0177;
				*s++ = dichar1[c / 8];
				*s++ = dichar2[c & 7];
			}
			else {
				*s++ = c;
			}
			++cp;
		}
	} while (*(cp + 1) == '\0' && (cp = readblock()) != NULL);
	blockp = cp;
	*s = '\0';
}
/* scan past the next occurence of this character in the cross-reference */

char	*
scanpast(c)
register char	c;
{
	register char	*cp;
	
	setmark(c);
	cp = blockp;
	do {	/* innermost loop optimized to only one test */
		while (*cp != c) {
			++cp;
		}
	} while (*(cp + 1) == '\0' && (cp = readblock()) != NULL);
	blockp = cp;
	if (cp != NULL) {
		skiprefchar();	/* skip the found character */
	}
	return(blockp);
}

/* read a block of the cross-reference */

char	*
readblock()
{
	/* read the next block */
	blocklen = read(symrefs, block, BUFSIZ);
	blockp = block;
	
	/* add the search character and end-of-block mark */
	block[blocklen] = blockmark;
	block[blocklen + 1] = '\0';
	
	/* return NULL on end-of-file */
	if (blocklen == 0) {
		blockp = NULL;
	}
	else {
		++blocknumber;
	}
	return(blockp);
}

static char	*
lcasify(s)
register char	*s;
{
	static char ls[PATLEN+1];	/* largest possible match string */
	register char *lptr = ls;
	
	while(*s) {
		*lptr = tolower(*s);
		lptr++;
		s++;
	}
	*lptr = '\0';
	return(ls);
}

/* find the functions called by this function */

BOOL
findcalledby()
{
	char	file[PATHLEN + 1];	/* source file name */
	BOOL	found_caller = NO;	/* seen calling function? */
	BOOL	macro = NO;

	if (invertedindex == YES) {
		POSTING	*p;
		
		findterm();
		while ((p = getposting()) != NULL) {
			switch (p->type) {
			case DEFINE:		/* could be a macro */
			case FCNDEF:
				if (dbseek(p->lineoffset) != -1 &&
				    scanpast('\t') != NULL) {	/* skip def */
					found_caller = YES;
					findcalledbysub(srcfiles[p->fileindex], macro);
				}
			}
		}
		return(found_caller);
	}
	/* find the function definition(s) */
	while (scanpast('\t') != NULL) {
		switch (*blockp) {
			
		case NEWFILE:
			skiprefchar();	/* save file name */
			putstring(file);
			if (*file == '\0') {	/* if end of symbols */
				return(found_caller);
			}
			progress();
			break;

		case DEFINE:		/* could be a macro */
			if (fileversion < 10) {
				break;
			}
			macro = YES;
			/* FALLTHROUGH */

		case FCNDEF:
			skiprefchar();	/* match name to pattern */
			if (match()) {
				found_caller = YES;
				findcalledbysub(file, macro);
			}
			break;
		}
	}
}

/* find this term, which can be a regular expression */

static void
findterm()
{
	register char	*s;
	register int	len;
	char	prefix[PATLEN + 1];
	char	term[PATLEN + 1];

	npostings = 0;		/* will be non-zero after database built */
	lastfcnoffset = 0;	/* clear the last function name found */
	boolclear();		/* clear the posting set */

	/* get the string prefix (if any) of the regular expression */
	(void) strcpy(prefix, pattern);
	if ((s = strpbrk(prefix, ".[{*+")) != NULL) {
		*s = '\0';
	}
	/* if letter case is to be ignored */
	if (caseless == YES) {
		
		/* convert the prefix to upper case because it is lexically
		   less than lower case */
		s = prefix;
		while (*s != '\0') {
			*s = toupper(*s);
			++s;
		}
	}
	/* find the term lexically >= the prefix */
	(void) invfind(&invcontrol, prefix);
	if (caseless == YES) {	/* restore lower case */
		(void) strcpy(prefix, lcasify(prefix));
	}
	/* a null prefix matches the null term in the inverted index,
	   so move to the first real term */
	if (*prefix == '\0') {
		(void) invforward(&invcontrol);
	}
	len = strlen(prefix);
	do {
		(void) invterm(&invcontrol, term);	/* get the term */
		s = term;
		if (caseless == YES) {
			s = lcasify(s);	/* make it lower case */
		}
		/* if it matches */
		if (regex(regexp, s) != NULL) {
	
			/* add its postings to the set */
			if ((postingp = boolfile(&invcontrol, &npostings, BOOL_OR)) == NULL) {
				break;
			}
		}
		/* if there is a prefix */
		else if (len > 0) {
			
			/* if ignoring letter case and the term is out of the
			   range of possible matches */
			if (caseless == YES) {
				if (strncmp(term, prefix, len) > 0) {
					break;	/* stop searching */
				}
			}
			/* if using letter case and the prefix doesn't match */
			else if (strncmp(term, prefix, len) != 0) {
				break;	/* stop searching */
			}
		}
		/* display progress about every three seconds */
		if (++searchcount % 50 == 0) {
			progress("%ld of %ld symbols matched", searchcount, totalterms);
		}
	} while (invforward(&invcontrol));	/* while didn't wrap around */
	
	/* initialize the progress message for retrieving the references */
	searchcount = 0;
	postingsfound = npostings;
}

/* get the next posting for this term */

static POSTING *
getposting()
{
	if (npostings-- <= 0) {
		return(NULL);
	}
	/* display progress about every three seconds */
	if (++searchcount % 100 == 0) {
		progress("%ld of %ld possible references retrieved", searchcount,
		    postingsfound);
	}
	return(postingp++);
}

/* put the posting reference into the file */

static void
putpostingref(p, pat)
POSTING	*p;
char	*pat;
{
	static char	function[PATLEN + 1];	/* function name */

	if (p->fcnoffset == 0) {
		if (p->type == FCNDEF) { /* need to find the function name */
			if (dbseek(p->lineoffset) != -1) {
				scanpast(FCNDEF);
				putstring(function);
			}
		}
		else if (p->type != FCNCALL) {
			strcpy(function, global);
		}
	}
	else if (p->fcnoffset != lastfcnoffset) {
		if (dbseek(p->fcnoffset) != -1) {
			putstring(function);
			lastfcnoffset = p->fcnoffset;
		}
	}
	if (dbseek(p->lineoffset) != -1) {
		if (pat)
			putref(0, srcfiles[p->fileindex], pat);
		else
			putref(0, srcfiles[p->fileindex], function);
	}
}

/* seek to the database offset */

long
dbseek(offset)
long offset;
{
	long	n;
	int	rc = 0;
	
	if ((n = offset / BUFSIZ) != blocknumber) {
		if ((rc = lseek(symrefs, n * BUFSIZ, 0)) == -1) {
			myperror("Lseek failed");
			(void) sleep(3);
			return(rc);
		}
		(void) readblock();
		blocknumber = n;
	}
	blockp = block + offset % BUFSIZ;
	return(rc);
}

static void
findcalledbysub(file, macro)
char	*file;
BOOL	macro;
{
	/* find the next function call or the end of this function */
	while (scanpast('\t') != NULL) {
		switch (*blockp) {
		
		case DEFINE:		/* #define inside a function */
			if (fileversion >= 10) {	/* skip it */
				while (scanpast('\t') != NULL &&
				    *blockp != DEFINEEND) 
					;
			}
			break;
		
		case FCNCALL:		/* function call */

			/* output the file name */
			(void) fprintf(refsfound, "%s ", file);

			/* output the function name */
			skiprefchar();
			putline(refsfound);
			(void) putc(' ', refsfound);

			/* output the source line */
			putsource(1, refsfound);
			break;

		case DEFINEEND:		/* #define end */
			
			if (invertedindex == NO) {
				if (macro == YES) {
					return;
				}
				break;	/* inside a function */
			}
			/* FALLTHROUGH */

		case FCNDEF:		/* function end (pre 9.5) */

			if (invertedindex == NO) break;
			/* FALLTHROUGH */

		case FCNEND:		/* function end */
		case NEWFILE:		/* file end */
			return;
		}
	}
}
