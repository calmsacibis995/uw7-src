%{
/*      @(#)slink.l,v 6.2 1993/09/08 20:09:00 stevea Exp - STREAMware TCP/IP  source        */
%}
%{
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1993 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
%}
%{
/*      SCCS IDENTIFICATION        */
%}
%{
#include "defs.h"
#include "proto.h"
#define yywrap() (1)
%}

%%

[_A-Za-z][_A-Za-z0-9]*			{
						strval = savestr(yytext);
						return(T_NAME);
					}

\$[1-9][0-9]?				{
						intval = atoi(&yytext[1]) - 1;
						return(T_PARAM);
					}

\"([^"\\\n]|(\\[^\n]))*\"		{
						dostring();
						return(T_STRING);
					}

([^"={}\\ \t\n]|(\\[^ \t\n]))+		{
						dostring();
						return(T_STRING);
					}

(#[^\n]*)?\n				{
						line++;
						return(T_EOL);
					}


\\\n					{
						line++;
					}

[ \t]					;

.					{
						return(yytext[0]);
					}

%%

dostring()
{
	char *p, *ep, *s;

	strval = xmalloc(yyleng+1);
	/* copy string, removing quotes & backslashes */
	for (p = yytext, ep = p + yyleng, s = strval; p < ep; p++) {
		if (*p == '"')
			continue;
		if (*p == '\\')
			p++;
		*s++ = *p;
	}
	*s = '\0';
}
