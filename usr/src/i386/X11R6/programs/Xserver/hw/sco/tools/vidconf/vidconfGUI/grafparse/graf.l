%{
/*
 *	@(#) graf.l 12.3 95/07/25 
 */
/************************************************************************* 
************************************************************************** 
 *	Copyright (C) The Santa Cruz Operation, 1989-1993
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
**
**  File Name: 		graf.l         
**  ---------
**
**  Author:		Kyle Clark and David Wood
**  ------
**
**  Creation Date:	27 Nov. 1989 and 26 Oct. 1993
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
**
**
*************************************************************************** 
***************************************************************************/

/*
 *	S001	Sep 12 1991	pavelr
 *	- added = and ; parsing
 *      S002    Thu Nov 05 10:59:48 PST 1992    davidw@sco.com
 *      - add VIDSCRIPT parsing
 *      S003    Thu Aug 05 18:47:22 PDT 1993    davidw@sco.com
 *      - Add fix buck found to convince lex we really want longer strings
 *      S004    Mon Aug 23 09:44:39 PDT 1993    davidw@sco.com
 *      - Changed include vid.h to graf.h
 *      S005    Tue Oct 26 11:44:40 PDT 1993    davidw@sco.com
 *      - Changed file name from vc.l to graf.l.  Redefined output(c)
 *	macro to nothing. Why is output spitting out blank lines?
 *      S006    Tue Jul 25 17:44:40 PDT 1995    davidw@sco.com
 *	- Fix warning redefining output macro.
 */

#include <stdio.h>
#include <ctype.h>
#include "graf.h"

#if YYLMAX < 256                                                /* vvv S003 */
# undef  YYLMAX
# define YYLMAX 256
#endif                                                          /* ^^^ S003 */

#ifdef output
# undef output
#endif
#define output(c) /* */						/* S005 */

#define STAR '*'
#define SLASH '/'
#define QUOTE '"'
#define BLANK ' '
%}
%%
"/*"			{
			    skip_comment();
			}
\"                      {
 			    get_prompt();
			    return(PROMPT);
			}
CLASS		        {
			    return(CLASS);
			}
MODE	 		{
			    return(MODE);
			}
VENDOR	 		{
			    return(VENDOR);
			}
MODEL	 		{
			    return(MODEL);
			}
VIDSCRIPT		{				/* S002 */
			    return(VIDSCRIPT);
			}
VIDSETUP		{
			    return(VIDSETUP);
			}
PROCEDURE		{
			    return(DATA);
			}
[Pp][Oo][Rr][Tt]        {
			    return(DATA);
			}
[Mm][Ee][Mm][Oo][Rr][Yy] {
			    return(DATA);
			}
[^ \n\t,=(){}"]+        {
			    return(IDENTIFIER);
			}
[ \t,(){}]		/* ignore for now return(WSPACE_TOKEN) */;

=			{ return (EQUAL); }

;			{ return (SEMICOLON); }



.			/* ignore everything else */;
%%
skip_comment()
{   
    int currentValue = input();

    while (!feof(stdin)) 
    {
        if ((char)currentValue == STAR) 
	{
	    while ((char)(currentValue) == STAR)
	        currentValue = input();

	    if ((char)currentValue==SLASH)
	    {
	        break;
	    }
	}

	else if ((char)currentValue == SLASH) 
	{
	    if ((char)(currentValue = input())==STAR)
	    {
	        skip_comment();
		currentValue = input();
	    }
	}

	else
	    currentValue = input();
    }

}
get_prompt()
{
   /*
    * Get a prompt ending in a double quote form
    * the input.
    */
    int currentValue;

    yyleng = 0;
    while ((char)(currentValue = input()) == BLANK);
    while ((char)currentValue != QUOTE)
    {
        yytext[yyleng++] = (char)currentValue;
	currentValue = (char)input();
    }
    yytext[yyleng++] = (char)NULL;
}
