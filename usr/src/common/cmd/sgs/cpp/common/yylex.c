#ident	"@(#)cpp:common/yylex.c	1.12.1.2"

#include "y.tab.h"
#ifdef FLEXNAMES
#	define NCPS	128
#else
#	define NCPS	8
#endif
extern int ncps;	/* actual number of chars. */
#ifdef CXREF
extern int xline;
#endif
extern int yylval;

#define isid(a)  ((fastab+COFF)[a]&IB)
#define IB 1
/* 	Adjust characters if they are signed extended */
/*	#if '\377' < 0		Don't use here, old cpp doesn't understand */
#define COFF 0

yylex()
{
	static int ifdef = 0;
	static unsigned char *op2[] = {(unsigned char *)"||",  (unsigned char *)"&&" , (unsigned char *)">>", (unsigned char *)"<<", (unsigned char *)">=", (unsigned char *)"<=", (unsigned char *)"!=", (unsigned char *)"=="};
	static int  val2[] = {OROR, ANDAND,  RS,   LS,   GE,   LE,   NE,   EQ};
	static unsigned char *opc = (unsigned char *)"b\bt\tn\nf\fr\r\\\\";
	extern unsigned char fastab[];
	extern unsigned char *outp, *inp, *newp;
	extern int flslvl;
	register unsigned char savc, *s;
	unsigned char *skipbl();
	int val;
	register unsigned char **p2;
	struct symtab
	{
		unsigned char *name;
		unsigned char *value;
	} *sp, *lookup();

	for ( ;; )
	{
		newp = skipbl( newp );
		if ( *inp == '\n' ) 		/* end of #if */
			return( stop );
		savc = *newp;
		*newp = '\0';
		if ( *inp == '/' && inp[1] == '*' )
		{
			/* found a comment with -C option, still toss here */
			*newp = savc;
			outp = inp = newp;
			continue;
		}
		for ( p2 = op2 + 8; --p2 >= op2; )	/* check 2-char ops */
			if ( strcmp( *p2, inp ) == 0 )
			{
				val = val2[ p2 - op2 ];
				goto ret;
			}
		s =(unsigned char *) "+-*/%<>&^|?:!~(),";		/* check 1-char ops */
		while ( *s )
			if ( *s++ == *inp )
			{
				val= *--s;
				goto ret;
			}
		if ( *inp<='9' && *inp>='0' )		/* a number */
		{
			if ( *inp == '0' )
				yylval= ( inp[1] == 'x' || inp[1] == 'X' ) ?
					tobinary( inp + 2, 16 ) :
					tobinary( inp + 1, 8 );
			else
				yylval = tobinary( inp, 10 );
			val = number;
		}
		else if ( isid( *inp ) )
		{
			if ( strcmp( inp, "defined" ) == 0 )
			{
				ifdef = 1;
				++flslvl;
				val = DEFINED;
			}
			else
			{
				if ( ifdef != 0 )
				{
					register unsigned char *p;
					register int savech;

					/* make sure names <= ncps chars */
					if ( ( newp - inp ) > ncps )
						p = inp + ncps;
					else
						p = newp;
					savech = *p;
					*p = '\0';
					sp = lookup( inp, -1 );
					*p = savech;
					ifdef = 0;
					--flslvl;
				}
				else
					sp = lookup( inp, -1 );
#ifdef CXREF
				ref(inp, xline);
#endif
				yylval = ( sp->value == 0 ) ? 0 : 1;
				val = number;
			}
		}
		else if ( *inp == '\'' )	/* character constant */
		{
			val = number;
			if ( inp[1] == '\\' )	/* escaped */
			{
				unsigned char c;

				if ( newp[-1] == '\'' )
					newp[-1] = '\0';
				s = opc;
				while ( *s )
					if ( *s++ != inp[2] )
						++s;
					else
					{
						yylval = *s;
						goto ret;
					}
				if ( inp[2] <= '9' && inp[2] >= '0' )
					yylval = c = tobinary( inp + 2, 8 );
				else
					yylval = inp[2];
			}
			else
				yylval = inp[1];
		}
		else if ( strcmp( "\\\n", inp ) == 0 )
		{
			*newp = savc;
			continue;
		}
		else if ( inp[0] == '#' && isid( savc ) ) /* predicate */
		{
			/* #<id>(<tokens>) -- always false */
			unsigned char *savinp, *savoutp, *savnewp;
			int cnt;

			*newp = savc;
			savinp = inp;
			savoutp = outp;
			savnewp = newp;
			flslvl++;
			newp = skipbl( newp );
			if ( !isid( *inp ) )
			{
			backout:;
				inp = savinp;
				outp = savoutp;
				newp = savnewp;
				flslvl--;
				goto badchar;
			}
			newp = skipbl( newp );
			if ( *inp != '(' )
				goto backout;
			cnt = 1;
			do /* parens nest--find matching close */
			{
				newp = skipbl( newp );
				if ( *inp == '\n' )
					goto backout;
				if ( *inp == '(' )
					cnt++;
				else if ( *inp == ')' )
					cnt--;
			} while ( cnt != 0 );
			flslvl--;
			savc = *newp;
			yylval = 0;
			val = number;
		}
		else
		{
			*newp = savc;
		badchar:;
			pperror( "Illegal character %c in preprocessor if",
				*inp );
			continue;
		}
	ret:
		/* check for non-ident after defined (note need the paren!) */
		if ( ifdef && val != '(' && val != DEFINED )
		{
			pperror( "\"defined\" modifying non-identifier \"%s\" in preprocessor if", inp );
			ifdef = 0;
			flslvl--;
		}
		*newp = savc;
		outp = inp = newp;
		return( val );
	}
}

tobinary( st, b )
	unsigned char *st;
{
	int n, c, t;
	unsigned char *s;
	int warned = 0;

	n = 0;
	s = st;
	while ( c = *s++ )
	{
		switch( c )
		{
		case '8': case '9':
			if (b <= 8 && !warned) {
				ppwarn("Illegal octal number %s", st);
				warned = 1;
			}
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7':
			t = c - '0';
			break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
			t = (c - 'a') + 10;
			if ( b > 10 )
				break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
			t = (c - 'A') + 10;
			if ( b > 10 )
				break;
		default:
			t = -1;
			if ( c == 'l' || c == 'L' )
				if ( *s == '\0' )
					break;
			pperror( "Illegal number %s", st );
		}
		if ( t < 0 )
			break;
		n = n * b + t;
	}
	return( n );
}
