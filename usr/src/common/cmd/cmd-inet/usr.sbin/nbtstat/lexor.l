%{
#ident "@(#)lexor.l	1.2"
#include <sys/types.h>
#include <sys/nb/nbuser.h>
#include <stdio.h>
#include <sys/errno.h>
#include "y.tab.h"
#include <locale.h>
#include "../nb_msg.h"
extern nl_catd catd;

#ifndef MSGSTR
#define MSGSTR(num,str) catgets(catd, MS_NB, (num), (str))
#endif
#ifndef MC_FLAGS
#define MC_FLAGS NL_CAT_LOCALE
#endif


extern int errno;
int lineno = 1;
char *process_nbname();

extern yyerror(char*);
%}
%%
\"[^\"]*\"		{
				yylval.char_p = process_nbname();
				return(NETBIOS_NAME);
			}
[0-9]+			{
#ifdef DEBUG_NB
				printf("Integer: %s\n", yytext);
#endif
				yylval.ival = atoi(yytext);
				return(INTEGER);
			}
#(PRE|pre)		{
#ifdef DEBUG_NB
				printf("PRE\n");
#endif
				return(PRE_KEYWORD);
			}
#(DOM|dom)		{
#ifdef DEBUG_NB
				printf("DOM\n");
#endif
				return(DOM_KEYWORD);
			}
:			{
#ifdef DEBUG_NB
				printf("COLON\n");
#endif
				return(COLON);
			}
\.			{
#ifdef DEBUG_NB
				printf("DOT\n");
#endif
				return(DOT);
			}
[a-zA-Z\!\$\%\&\(\)\^\_\'\{\}\~\+\-][a-zA-Z0-9\!\#\$\%\&\(\)\^\_\'\{\}\~\.\+\-]* {
				yylval.char_p = process_nbname();
				return(NETBIOS_NAME);
			}
[ 	]+		{
				/*printf("Whitespace\n");*/
			}
^#.*\n			{
				lineno++;
				/*printf("Line comment: (%s)\n", yytext);*/
			}
#			{
				/*printf("Partial line comment:", yytext);*/
				while (input() != '\n')
					;
				lineno++;
			}
\n			{
				lineno++;
				/*printf("EOL\n");*/
			}
%%
unsigned char
atox(char a) {
	if (isdigit(a))
		return(a-'0');
	if (isupper(a))
		return(a-'A'+10);
	if (islower(a))
		return(a-'a'+10);
}

char *
process_nbname() {
	char *p;
	char *out, *in;
	int	quote;

	if ( (p = (char *)malloc(NB_NAMESIZE)) == NULL ) {
		errno=ENOMEM;
		perror("");
		exit(1);
	}

	memset(p, ' ', NB_NAMESIZE);
	
	in = yytext;
	quote = 0;
	if ( *in == '\"' ) {
		quote = 1;
		in++;
		yytext[strlen(yytext)-1] = 0;
	}
	out = p;
	while (*in && out < p+NB_NAMESIZE) {
		if ( *in == '\\' && *(in+1) == '0' && *(in+2) == 'x' &&
			isxdigit(*(in+3)) && isxdigit(*(in+4)) ) {
			*out++ = atox(*(in+3)) << 4 | atox(*(in+4));
			in += 5;
		} else {
			if ( quote )
				*out++ = *in++;
			else
				*out++ = toupper(*in++);
		}
	}
#ifdef DEBUG_NB
	printf("NetBIOS Name: [%s]\n", yytext);
#endif
	if (*in)
		/*yyerror("NetBIOS name too long (>16 chars)");*/
		yyerror(MSGSTR(MSG_NB_NAMLEN,
		   "NetBIOS name too long (>16 chars)"));
	return(p);
}
yywrap()
{
        return 1;
}

