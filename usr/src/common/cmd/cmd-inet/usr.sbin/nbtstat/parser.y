%{
#ident "@(#)parser.y	1.3"
/*	Copyright (C) The Santa Cruz Operation, 1995.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include "nbdnld.h"

extern int lineno;
extern char *nbfile;
%}
%union {
	char		*char_p;
	int		ival;
	unsigned long	ulng;
};

%token DOM_KEYWORD
%token PRE_KEYWORD
%token DOT
%token COLON
%token <char_p> NETBIOS_NAME
%token <ival> INTEGER
%%
FILE :		LINE
     |		FILE LINE
     ;

LINE :		HOST_DECL
	|	HOST_DECL DOM_DECL
			{
				store_domainname($<char_p>2, $<char_p>1);
			}
	|	HOST_DECL DOM_DECL PRE_DECL
			{
				store_domainname($<char_p>2, $<char_p>1);
			}
	|	HOST_DECL PRE_DECL DOM_DECL
			{
				store_domainname($<char_p>3, $<char_p>1);
			}
	|	HOST_DECL PRE_DECL
	;

HOST_DECL :	IP_ADDRESS NETBIOS_NAME
			{
				store_hostname($<char_p>2, $<ulng>1);
				$<char_p>$ = $<char_p>2;
			}
	  ;

IP_ADDRESS :	INTEGER DOT INTEGER DOT INTEGER DOT INTEGER
			{
				char buf[128];
				unsigned long l;

#ifdef DEBUG
				printf("yy\tGot an IP address(%d,%d,%d,%d)\n",
					$<ival>1, $<ival>3, $<ival>5, $<ival>7
				);
#endif
				sprintf(buf, "%d.%d.%d.%d",
					$<ival>1, $<ival>3, $<ival>5, $<ival>7
				);
				if ((l = inet_addr(buf)) == INADDR_NONE) {
					yyerror("Invalid IP address");
					YYERROR;
				}
				$<ulng>$ = l;
			}
	   ;
DOM_DECL :	DOM_KEYWORD COLON NETBIOS_NAME
			{
				$<char_p>$ = $<char_p>3;
			}
	 ;
PRE_DECL :	PRE_KEYWORD
			{
			}
	 ;
%%
void
yyerror(char *s)
{
	if (yychar == 0)
		exit(0);
	else {
		fprintf(stderr, "\"%s\", line %d: error: %s\n", nbfile, lineno, s);
		exit(1);
	}
}
