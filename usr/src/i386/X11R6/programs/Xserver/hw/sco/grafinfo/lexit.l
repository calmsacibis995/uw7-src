%{
/*
 *	@(#) lexit.l 12.1 95/05/09 SCOINC
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 */
/*
 *	S000	Fri Feb 01 19:53:23 PST 1991	pavelr@sco.com
 *	- Created file.
 *	S001	Thu Apr 25 19:53:23 PST 1991	pavelr@sco.com
 *	- major changes to remove case INsensitivity
 *	S002	Mon 09 Sep evening  PST 1991	pavelr@sco.com
 *	- recognize DATA keyword
 *	S003	Tue Mar 24 1992			pavelr@sco.com
 *      - added read & write support
 *	S004	Thu Aug 27 14:21:23 PDT 1992	buckm@sco.com
 *	- add int10 support.
 *	- add tokens for colon and minus.
 *	S005	Sun Mar 28 22:23:55 PST 1993	buckm@sco.com
 *	- add callrom support.
 *	- convince lex we would like longer maximum lines.
 *	S006	Tue Jun 29 1993	edb@sco.com
 *	- skip procedure clause in case there is a compiled grafinfo file
 */
%}

%{
	/* lexit.l - source for lexical scanner for GrafInfo files.
	   Points to note:
	   - vendor, model, class, and mode tokens return the value
	     of the following string in yylval. This was done because
	     language is NOT context free. This seemed to be the easiest
	     way to do it.
	 */

#include <stdlib.h>
#include <stdio.h>
#include "y.tab.h"
#include "grafinfo.h"

#if YYLMAX < 256						/* vvv S005 */
# undef	 YYLMAX
# define YYLMAX	256
#endif								/* ^^^ S005 */

/* keep track of line numbers */
int grafLineno=1;

/* used to extract vendor/model/class/mode */
static char s[40];

void ResetLineNo ()
{
grafLineno = 1;
}

%}

D	[0-9]
L	[a-zA-Z]
W	[ \t]
DL	[0-9a-zA-Z]

%%

"/*"		SkipComments();

\"[^"]*\"		{yytext[yyleng-1] = '\0'; yylval.string = &yytext[1]; 
			    return (STRING);
}

VENDOR{W}+[^ \n\t]+	{sscanf (&yytext[6],"%s", s);
				yylval.string = s;
				return (VENDOR);
}

MODEL{W}+[^ \n\t]+	{sscanf (&yytext[5],"%s", s);
				yylval.string = s;
				return (MODEL);
}

CLASS{W}+[^ \n\t]+	{sscanf (&yytext[5],"%s", s);
				yylval.string = s;
				return (CLASS);
}

MODE{W}+[^ \n\t]+	{sscanf (&yytext[4],"%s", s);
				yylval.string = s;
				return (MODE);
}

PROCEDURE		return (PROCEDURE);
MEMORY			return (MEMORY);
PORT			return (PORT);

set			return (SET);
in			return (IN);
out			return (OUT);
outw			return (OUTW);
and			return (AND);
or			return (OR);
not			return (NOT);
shr			return (SHR);
shl			return (SHL);
bout			return (BOUT);
xor			return (XOR);
wait			return (WAIT);

readb			return (READB);				/* S003 */
writeb			return (WRITEB);			/* S003 */
readw			return (READW);				/* S003 */
writew			return (WRITEW);			/* S003 */
readdw			return (READDW);			/* S003 */
writedw			return (WRITEDW);			/* S003 */

int10			return (INT10);				/* S004 */
callrom			return (CALLROM);			/* S005 */

InitGraphics		return (INITGRAPHICS);

DATA			return (DATA);		/* S002 */

r{D}+				{GetRegister (); return (REG);};

0[xX][0-9a-fA-F]+		{sscanf (yytext, "%i", &yylval.number); return (NUMBER);};
0[0-7]+				{sscanf (yytext, "%i", &yylval.number); return (NUMBER);};
{D}+				{sscanf (yytext, "%i", &yylval.number); return (NUMBER);};
{L}[a-zA-Z0-9_]*			{strcpy(s,yytext);  yylval.string = s; return (IDENT);}
{W}+				;
\n				grafLineno++;

\{				return (LBRACE);
\}				return (RBRACE);

\(				return (LPAREN);
\)				return (RPAREN);

=				return (EQUAL);

\,				return (COMMA);

\;				return (SEMICOLON);

:				return (COLON);			/* S004 */

\+				return (PLUS);

-				return (MINUS);			/* S004 */

%%

/*
 * Skip over comments and keep track of line numbers. Taken from the lex docs.
 */

SkipComments()
{
	char c;	
	for (;;) {
		while (((c=input()) !='*') && c != '\n');
		if (c == '\n') {
			grafLineno++;
			continue;
		}
		if ((c=input()) != '/') {
			unput(c);
		}
			else return ;
	}
}

/* This seemed to be a good place to check for a valid register
   number. This could have been done in parse.y but it would have
   to have been done in many places there.
*/

GetRegister ()
{
	extern int graferrno;
	sscanf (yytext, "r%d", &yylval.number);
	if ((yylval.number < 0) || (yylval.number >= NUMBER_REG)) {
		graferrno = GEBADREG;
	}
}

/*                                      vvv S006 vvv   */ 
/*    
 *   If we found a loadable C_file we have to skip the procedure body
 *   It could contain C_code which our parser can't process
 */

grafSkipProcedure()
{
        char     c;
        int in_comment = 0;
        int end_of_c   = 0;
        int nesting    = 0;

	while( !end_of_c )
        {
         
             c = input();
             switch( c ) 
             {
                 case '{' : if( !in_comment ) nesting++;
                            break;

                 case '}' : if( !in_comment ) nesting--;
                            if( nesting <= 0 ) end_of_c = 1;
                            break;

                 case '/' : if( !in_comment )
                            {
                                c = input();
                                if( c == '*' )    in_comment = 1; 
                                else              unput( c);
                            }
                            break;

                 case '*' : if(  in_comment )
                            {
                                c = input();
                                if( c == '/' )    in_comment = 0;
                                else              unput( c);
                            }
                            break;

                 case '\n': grafLineno++;
                            break;

                 default:
                            break;
	     }
	}
}
/*                                      ^^^ S006 ^^^   */ 
