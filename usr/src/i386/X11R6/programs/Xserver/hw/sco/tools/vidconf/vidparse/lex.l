%{
#pragma comment(exestr, "@(#) lex.l 12.1 95/05/09 SCOINC")
/************************************************************************* 
************************************************************************** 
**	Copyright (C) The Santa Cruz Operation, 1989-1993
**	This Module contains Proprietary Information of
**	The Santa Cruz Operation and should be treated as Confidential.
**
**
**  File Name: 		lex.l         
**  ---------
**
**  Author:		Kyle Clark
**  ------
**
**  Creation Date:	27 Nov. 1989
**  -------------
**
**  Overview:	
**  --------
**  Lexical analyzer for *.xgi files.
**
**  External Functions:
**  ------------------
** 
**  Data Structures:
**  ---------------
**
**  Bugs:
**  ----
**  1) How lists of memory addresses as opposed to address
**     ranges are specified is not yet clear.  For the time
**     being assume averything is a list of memory addresses.
**     If memory address ranges can be specified then may
**     need to add the line:
**          "?"			return(RANGE_TOKEN);
**
**  Modification History:
**  --------------------
**	S001	Fri Oct 02 11:16:52 PDT 1992	buckm
**	- Make this much more like graflib/lexit.l.
**	S002    Wdn Jun 16 1992 edb
**	- Add  tokens to detect a PROCEDURE clause,
**        ExtractCcode() and related changes
**	S003    Thu Jun 17 1992 edb
**	- Fix broken vidparse
**      S004    Thu Aug 05 18:45:26 PDT 1993    davidw@sco.com
**      - Add fix buck found to convince lex we really want longer strings
*************************************************************************** 
***************************************************************************/
%}

%{
#include <stdio.h>
#include "vptokens.h"
#include "vidparse.h"

#if YYLMAX < 256                                                /* vvv S004 */
# undef  YYLMAX
# define YYLMAX 256
#endif                                                          /* ^^^ S004 */

/* used to extract vendor/model/class/mode/procedure */

char vmcm_buf[256];

extern    nameStruct *currentStr;
extern    int         currentToken;
extern    int         in_procedure;

/* keep track of line numbers */
int grafLineno = 1;

void ResetLineNo()
{
	grafLineno = 1;
}

%}

W	[ \t]
L	[a-zA-Z]
AN	[0-9a-zA-Z_]

%%

"/*"			SkipComments();

\"[^"]*\"		{ yytext[yyleng-1] = '\0';
				return(STRING_TOKEN);
			}

VENDOR{W}+[^ \n\t]+	{ sscanf (&yytext[6],"%s", vmcm_buf);
				return(VENDOR_TOKEN);
			}

MODEL{W}+[^ \n\t]+	{ sscanf (&yytext[5],"%s", vmcm_buf);
				return(MODEL_TOKEN);
			}

CLASS{W}+[^ \n\t]+	{ sscanf (&yytext[5],"%s", vmcm_buf);
				return(CLASS_TOKEN);
			}

MODE{W}+[^ \n\t]+	{ sscanf (&yytext[4],"%s", vmcm_buf);
				return(MODE_TOKEN);
			}

MEMORY			return(MEMORY_TOKEN);
PORT			return(PORT_TOKEN);

PROCEDURE		return(PROCEDURE_TOKEN);

0[xX][0-9a-fA-F]+	return(NUMBER_TOKEN);
[0-9]+			return(NUMBER_TOKEN);

{L}{AN}*		return(IDENT_TOKEN);

-			return(DASH_TOKEN);
:			return(COLON_TOKEN);
\;			return(SEMICOLON_TOKEN);

\{			if( in_procedure )return( OPBRACE_TOKEN );

\(			if( in_procedure )return( OPPARENT_TOKEN );

[})=,+]			/* do nothing when outside procedures*/;
{W}+			/* do nothing */;

\n			grafLineno++;

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

                     
/*                           vvvv  S002  vvvv
 *
 *      ParseProcedure()  detected a PROCEDURE and IDENT token
 *      ExtractCcode()    scans the input file for the matching braces and
 *                        if fpc != NULL it writes it out to c file
 */
ExtractCcode( fpc )
FILE *fpc;
{
        char c;
        int in_comment = 0;
        int end_of_c   = 0;
        int nesting    = 0;

	while( !end_of_c )
        {
             c = input();
             if( fpc )fprintf( fpc, "%c", c);
             switch( c ) 
             {
                 case '{' : if( !in_comment && nesting == 0 )
                            {
                                 if( fpc ) printDeclaration( fpc );
                                 nesting++;
                            }
                            break;

                 case '}' : if( !in_comment ) nesting--;
                            if( nesting <= 0 ) end_of_c = 1;
                            break;

                 case '/' : if( !in_comment )
                            {
                                c = input();
                                if( c == '*' )
                                { 
                                      in_comment = 1; 
                                      if( fpc )fprintf( fpc, "%c", c); 
                                }
                                else 
                                      unput( c);
                            }
                            break;

                 case '*' : if(  in_comment )
                            {
                                c = input();
                                if( c == '/' ) 
                                {
                                      in_comment = 0;
                                      if( fpc )fprintf( fpc, "%c", c); 
                                }
                                else
                                      unput( c);
                            }
                            break;

                 case '\n': grafLineno++;
	     }
	}
}                            /*   ^^^^ S002 ^^^^ */
