%{
#ident	"@(#)make:gram.y	1.22.1.5"

#include "defs"
#include <pfmt.h>
#define NLEFTS	1200
#define INMAX	10000
%}

%term NAME SHELLINE START COLON DOUBLECOLON EQUAL EQUIVALENT A_STRING VERSION

%union
{
	SHBLOCK	 yshblock;
	DEPBLOCK ydepblock;
	NAMEBLOCK ynameblock;
	CHARSTAR ycharstring;
}

%type	<yshblock>	SHELLINE, shlist, shellist
%type	<ynameblock>	NAME, namelist, name
%type	<ydepblock>	deplist, dlist
%type	<ycharstring>	A_STRING

%%

%{
extern	NAMEBLOCK mainname;	/* from main.c */
void	setvar();
DEPBLOCK 	pp;
NAMEBLOCK	leftp;
LINEBLOCK 	lp, lpp;
FSTATIC SHBLOCK prevshp;

struct nameblock * * lefts;
static int maxLefts = NLEFTS_BASE;


FSTATIC DEPBLOCK prevdep;
FSTATIC int	nlefts,
		sepc;
%}


file:
	| file comline
	;

comline:  START
	| START macrodef
	| START namelist deplist shellist
	{
		if ( !(mainname || IS_ON(INTRULE)) &&
		     ((lefts[0]->namep[0] != DOT) ||
		      ANY(lefts[0]->namep, SLASH)))
			mainname = lefts[0];
	        while( --nlefts >= 0) {
			leftp = lefts[nlefts];
			if ( !(leftp->septype) )
				leftp->septype = sepc;
			else if (leftp->septype != sepc)
				pfmt(stderr, MM_ERROR,
					":322:inconsistent rules lines for `%s' (bu36)\n",
						leftp->namep);
			else if (sepc==ALLDEPS &&
				 *(leftp->namep)!=DOT &&
				 $4) {
				for (lp = leftp->linep;
				     lp->nextline;
				     lp = lp->nextline)
					if (lp->shp)
						pfmt(stderr, MM_ERROR,
							":323:multiple make lines for %s (bu37)\n",
								leftp->namep);
			}
	
			lp = ALLOC(lineblock);
			lp->nextline = NULL;
			lp->depp = $3;
			lp->shp = $4;

			if (STREQ(leftp->namep, ".SUFFIXES") && $3==0)
				leftp->linep = 0;
			else if ( !(leftp->linep) )
				leftp->linep = lp;
			else {
				for (lpp = leftp->linep;
				     lpp->nextline;
				     lpp = lpp->nextline)
					;
				if (sepc==ALLDEPS && 
						leftp->namep[0]==DOT &&
		      				!ANY(leftp->namep, SLASH))
					lpp->shp = 0;
				lpp->nextline = lp;
			}
		}
	}
	| error
	;

macrodef: NAME EQUAL A_STRING
	{
		setvar((CHARSTAR) $1, $3);
	}
        | NAME EQUIVALENT A_STRING
        {
                CHARSTAR bigString, smallString;
                int     strLen=0, strPos;
                CHARSTAR  subst();

                bigString = subst($3, bigString, &strLen, &strPos);
                if ((smallString = malloc(strPos+1)) == NULL)
                        fatal(":311:out of memory");
                strcpy(smallString, bigString);
                setvar($1, smallString);
                free(bigString);
        }

	;

name:	NAME
	{
		if( !($$ = SRCHNAME((CHARSTAR) $1)) )
			$$ = makename((CHARSTAR) $1);
	}
	| NAME VERSION
	{
		if( !($$ = SRCHNAME((CHARSTAR) $1)) )
			$$ = makename((CHARSTAR) $1);
	}
	;

namelist: name
	{
		lefts[0] = $1;
		nlefts = 1;
	}
	| namelist name
	{
		lefts[nlefts++] = $2;
                if (nlefts >= maxLefts) {
                        maxLefts += NLEFTS_INC ;
                        if ((lefts = (struct nameblock * *) realloc(lefts,
                           maxLefts*sizeof(struct nameblock *))) == NULL)
                                fatal("Too many lefts");
                }

	}
	;

dlist:  sepchar
	{
		prevdep = 0;
		$$ = 0;
	}
	| dlist name
	{
		pp = ALLOC(depblock);
		pp->nextdep = NULL;
		pp->depname = $2;
		if ( !prevdep )
			$$ = pp;
		else
			prevdep->nextdep = pp;
		prevdep = pp;
	}
	;

deplist:	
	{
		fatal1(":325:Must be a separator on rules line %d (bu39)",
			yylineno);
	}
	| dlist
	;

sepchar:  COLON
	{
		sepc = ALLDEPS;
	}
	| DOUBLECOLON
	{
		sepc = SOMEDEPS;
	}
	;


shlist:	SHELLINE
	{
		$$ = $1;
		prevshp = $1;
	}
	| shlist SHELLINE
	{
		$$ = $1;
		prevshp->nextsh = $2;
		prevshp = $2;
	}
	;

shellist:
	{
		$$ = 0;
	}
	| shlist
	{
		$$ = $1;
	}
	;

%%

/*	@(#)make:gram.y	1.6 of 5/9/86	*/

#include <ctype.h>

CHARSTAR zznextc;	/* zero if need another line; otherwise points to next char */
int yylineno;
static char inmacro = NO;

static char *word;
static int word_count;
init_lex()
{
	word_count = 128;
	 word = ck_malloc(word_count);
}

int
yylex()
{
	register CHARSTAR p, q;
	CHARSTAR pword;
	static int	nextlin(), retsh();

	if ( !zznextc )
		return( nextlin() );

	while ( isspace(*zznextc) )
		++zznextc;

	if ( inmacro ) {
		inmacro = NO;
		yylval.ycharstring = copys(zznextc);
		zznextc = 0;
		return(A_STRING);
	}

	if (*zznextc == CNULL)
		return( nextlin() );

	if (*zznextc == KOLON) {
		if(*++zznextc == KOLON)	{
			++zznextc;
			return(DOUBLECOLON);
		} else
			return(COLON);
	}

	if (*zznextc == EQUALS) {
		inmacro = YES;
		++zznextc;
                if(*zznextc == EQUALS) {
                        ++zznextc;
                        return(EQUIVALENT);
                } else

			return(EQUAL);
	}

	if (*zznextc == SKOLON)
		return( retsh(zznextc) );

	p = zznextc;
	{
		int len;
		while ( !( funny[(unsigned char) *p] & TERMINAL) )
			p++;
		len = p - zznextc;
		while ( (len+1) > word_count ) {
			word_count *= 2;
			word = realloc(word,word_count);
		}
	}
	q = pword = word;

	p = zznextc;

	while ( !( funny[(unsigned char) *p] & TERMINAL) )
		*q++ = *p++;

	if (p != zznextc) {
		*q = CNULL;
		yylval.ycharstring = copys(pword);
		if (*p == RCURLY) {
			zznextc = p+1;
			return(VERSION);
		}
		if (*p == LCURLY)
			p++;
		zznextc = p;
		return(NAME);
	} else {
		pfmt(stderr, MM_ERROR,
			":326:bad character %c (octal %o), line %d (bu40)",
				*zznextc,*zznextc,yylineno);
		fatal(Nullstr);
	}
	return(0);	/* never executed */
}


static int
retsh(q)
register CHARSTAR q;
{
	extern CHARSTAR *linesptr;
	register CHARSTAR p;
	SHBLOCK sp = ALLOC(shblock);
	static int	GETC();

	for (p = q + 1; *p==BLANK || *p==TAB; ++p)
		;
	sp->nextsh = 0;
	sp->shbp = ( !fin ? p : copys(p) );
	yylval.yshblock = sp;
	zznextc = 0;
/*
 *	The following if-else "thing" eats up newlines within
 *	shell blocks.
 */
	if ( !fin ) {
		if (linesptr[0])
			while (linesptr[1] && STREQ(linesptr[1], "\n")) {
				yylineno++;
				linesptr++;
			}
	} else {
		register int c;
		while ((c = GETC()) == NEWLINE)
			yylineno++;
		if (c != EOF)
			(void)ungetc(c, fin);
	}
	return(SHELLINE);
}

char * yytext;
char * yytextEnd;
int  yytextLen;


static int
nextlin()
{
	extern CHARSTAR *linesptr;
	register char c;
	register CHARSTAR p, t;
	char 	*templin, lastch;
	CHARSTAR text, lastchp, subst();
	int	sindex();
	int 	incom, kc, nflg, poundflg;
	static void	fstack();
	static int	GETC();
        int templinLen, templinPos, lastchlen;
        int yyOffset;

again:	incom = 0;
	zznextc = 0;
	poundflg = 0;

	if ( !fin ) {
		if ( !(text = *linesptr++) )
			return(0);
		++yylineno;
		(void)copstr(yytext, text);
	} else {
		yytext[0] = CNULL;
		for (p = yytext; ; ++p) {
                        if (p >= yytextEnd) {
                                yyOffset = p - yytext ;
                                yytextLen += BUFF_INC ;
                                if((yytext=realloc( yytext, yytextLen ))==NULL)
                                        fatal(":311:out of memory") ;
                                yytextEnd = yytext + yytextLen ;
                                p = yytext + yyOffset ;
                        }

			if ((kc = GETC()) == EOF) {
				*p = CNULL;
				return(0);
			}
			else if ((kc == SKOLON) ||
				 (kc == TAB && p == yytext))
				++incom;
			else if (kc == POUND && !incom && yytext[0] != TAB) {
				poundflg++;
				kc = CNULL;
			}
			else if (kc == NEWLINE)	{
				++yylineno;
				if (p == yytext || p[-1] != BACKSLASH || poundflg)
					break;
				if(incom || yytext[0] == TAB){
					*p++ = NEWLINE;
                                        if (p >= yytextEnd) {
                                                yyOffset = p - yytext;
                                                yytextLen += BUFF_INC;
                                                if ((yytext=realloc( yytext,
                                                           yytextLen ))==NULL)
                                                   fatal(":311:out of memory");
                                                yytextEnd = yytext + yytextLen;
                                                p = yytext + yyOffset;
                                        }

				}
				else
					p[-1] = BLANK;
				nflg = YES;
				while (kc = GETC()) {
					if ( !( kc == TAB ||
					        kc == BLANK ||
					        kc == NEWLINE) )
						break;
					if (incom || yytext[0] == TAB) {
						if (nflg && kc == TAB) {
							nflg = NO;
							continue;
						}
						if (kc == NEWLINE)
							nflg = YES;
						*p++ = kc;
						 if(p >= yytextEnd) {
                                                        yyOffset=p-yytext;
                                                        yytextLen+=BUFF_INC;
                                                        if((yytext=
							     realloc(yytext,yytextLen ))==NULL)
                                                                fatal(":311:out of memory");
                                                        yytextEnd=yytext+yytextLen;
                                                        p=yytext+yyOffset;
                                                }

					}
					if (kc == NEWLINE)
						++yylineno;
				}
				if(kc == EOF) {
					*p = CNULL;
					return(0);
				}
			}
			*p = kc;
		}
		*p = CNULL;
		text = yytext;
	}
	
	if ((c = text[0]) == TAB)  
		return( retsh(text) );
/*
 *	DO include FILES HERE.
 */
	if ( !sindex(text, "include") && 
	     (text[7] == BLANK || text[7] == TAB)) {
		CHARSTAR pfile;

		for (p = &text[8]; *p != CNULL; p++)
			if(*p != TAB &&
			   *p != BLANK)
				break;
		pfile = p;
		for (;	*p != CNULL	&&
			*p != NEWLINE	&&
			*p != TAB	&&
			*p != BLANK; p++)
			;
		if (*p != CNULL)
			*p = CNULL;

/*
 *	Start using new file.
 */
		templinLen = 0;
                templin = subst(pfile, templin, &templinLen, &templinPos);
		fstack(templin, &fin, &yylineno);
		free(templin);

		goto again;
	}
	if (isalpha(c) || isdigit(c) || c==BLANK || c==DOT || c=='_')
		for (p = text+1; *p!=CNULL; p++)
			if (*p == KOLON || *p == EQUALS)
				break;

/* substitute for macros on dependency line up to the semicolon if any */
	if (*p != EQUALS) {
		for (t = yytext ; *t!=CNULL && *t!=SKOLON ; ++t)
			;

		lastchlen = strlen(t);
		lastchp = t;
		lastch = *t;
		*t = CNULL;


		templinLen = 0;
                templin = subst(yytext, templin, &templinLen, &templinPos);

                if ((templinPos + lastchlen) > templinLen) {
			templinLen = templinPos + lastchlen;
                        if ((templin = realloc(templin, templinLen)) == NULL)
                                fatal(":311:out of memory") ;
                }

		if (lastch) {
			*lastchp = lastch;
			strcat(templin, lastchp);
		}
                if (templinLen  > yytextLen) {
                        yytextLen = templinLen;
                        if ((yytext = realloc(yytext, yytextLen)) == NULL)
                                fatal(":311:out of memory") ;
                        yytextEnd = yytext+yytextLen ;
                        if (fin != NULL)
                                text = yytext;
                }

		strcpy(yytext,templin);
		free(templin);
	}

	if ( !poundflg || yytext[0] != CNULL)	{
		zznextc = text;
		return(START);
	} else
		goto again;

}


#include "stdio.h"

static int morefiles;

static struct sfiles {
	CHARSTAR sfname;
	FILE *sfilep;
	int syylno;
} sfiles[MAX_INC];


/* GETC() automatically unravels stacked include files.  During
 *	`include' file processing, when a new file is encountered,
 *	fstack will stack the FILE pointer argument. Subsequent
 *	calls to GETC with the new FILE pointer will get characters
 *	from the new file. When an EOF is encountered, GETC will
 *	check to see if the file pointer has been stacked. If so,
 *	a character from the previous file will be returned.
 */
static int
GETC()
{
	register int c;

	while ((c = getc(fin)) == EOF && morefiles) {
		(void)fclose(fin);
		yylineno = sfiles[--morefiles].syylno;
		fin = sfiles[morefiles].sfilep;
	}
	return(c);
}


/* fstack(stfname,ream,lno) is used to stack an old file pointer
 *	before the new file is assigned to the same variable. Also
 *	stacked are the file name and the old current lineno,
 *	generally, yylineno. 
 */
static	void
fstack(newname, oldfp, oldlno)
register char *newname;
register FILE **oldfp;
register int *oldlno;
{
	FILE *newfp;			/* new file descriptor */
	char *strcat();
	static int	GETC();

#ifdef MKDEBUG
	if (IS_ON(DBUG))
		printf("Include file: \"%s\"\n",newname);
#endif
	/* if unable to open file */
	if ( !(newfp = fopen(newname, "r")) ) {
		/* try sccs get of file */
		if ( !get(newname, 0) )
			fatal1(":328:cannot get %s for including",
				newname);
		if ( !(newfp = fopen(newname, "r")) )
			fatal1(":329:cannot open %s for including",
				newname);
	}
/*
 *	Stack the new file name, the old file pointer and the
 *	old yylineno;
 */
	
	if(morefiles >=  MAX_INC)
		fatal("too many include files");
	sfiles[morefiles].sfname = ck_malloc(strlen(newname) + 1);
	(void)strcpy(sfiles[morefiles].sfname, newname);
	sfiles[morefiles].sfilep = *oldfp;
	sfiles[morefiles++].syylno = *oldlno;
	yylineno = 0;
	*oldfp = newfp;				/* set the file pointer */
}

