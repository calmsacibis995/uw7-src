%{
#pragma ident "@(#)bcfgparser.y	24.1"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/* here's the parser. set tabstops=3 and use a smaller font and/or resize
 * your window otherwise endure the wrapping
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "common.h"

%}

%union {
	char *strval;
	union primitives primitive;
}

%token <strval> WORD CFGDEBUG VERSION VARIABLE WHITESPACE NULLSTRING
%token <strval> DECNUM OCTNUM HEXNUM
%token DQUOTE SQUOTE JUNK MYEOF

%type <primitive> something
%type <primitive> ManyWords
%type <primitive> word

%start cfgfile

%%

cfgfile:  /* empty */
	| statementlist

statementlist: statement
	| statementlist statement
	;

statement: version
	| MYEOF							{
										 DP1(BCFGYACCSTUFF,"got_normal_EOF ");
										}
	| debug
	| assignmentlist
	| error {
		yyclearin;  /* discard lookahead */
		yyerrok;
		error(SYNTAX, "ignoring bcfg syntax error(1)...");
		/* and continue */
	  }
	;

version: VERSION DECNUM				{cfgversion=strtoul($2,NULL,10); /* treat as number */
											 ensurevalid();
								  	 		 DP2(BCFGYACCFLUFF,"cfgversion now %d\n",cfgversion);
											 free($2);
						   				}
	|     VERSION OCTNUM				{cfgversion=strtoul($2,NULL,8); /* treat as number */
											 ensurevalid();
								  	 		 DP2(BCFGYACCFLUFF,"cfgversion now %d\n",cfgversion);
											 free($2);
						   				}
	|     VERSION HEXNUM				{cfgversion=strtoul($2,NULL,16); /* treat as number */
											 ensurevalid();
								  	 		 DP2(BCFGYACCFLUFF,"cfgversion now %d\n",cfgversion);
											 free($2);
						   				}
	|     VERSION junk				{/* ignore */;}
	|		VERSION MYEOF				{error(SYNTAX,"eof after version token");
											}
	;

junk: JUNK								{/* ignore */;}
	| JUNK DECNUM						{free($2);}
	| JUNK OCTNUM						{free($2);}
	| JUNK HEXNUM						{free($2);}
	| JUNK WORD							{free($2);}
	| JUNK junk							{/* ignore */;}
	;

debug: CFGDEBUG DECNUM				{extern unsigned int cfghasdebug;
											 cfgdebug=strtoul($2,NULL,10); /* treat as number */
								 	 		 DP2(BCFGYACCFLUFF,"cfgdebug now %d\n",cfgdebug);
											 if (!cfghasdebug) {
												notice("debugging not available");
											 }
											 free($2);
											}
	|   CFGDEBUG OCTNUM				{extern unsigned int cfghasdebug;
											 cfgdebug=strtoul($2,NULL,8); /* treat as number */
								 	 		 DP2(BCFGYACCFLUFF,"cfgdebug now %d\n",cfgdebug);
											 if (!cfghasdebug) {
												notice("debugging not available");
											 }
											 free($2);
											}
	|   CFGDEBUG HEXNUM				{extern unsigned int cfghasdebug;
											 cfgdebug=strtoul($2,NULL,16); /* treat as number */
								 	 		 DP2(BCFGYACCFLUFF,"cfgdebug now %d\n",cfgdebug);
											 if (!cfghasdebug) {
												notice("debugging not available");
											 }
											 free($2);
											}
	|   CFGDEBUG junk					{ /* ignore */ ; }
	|	 CFGDEBUG MYEOF				{error(SYNTAX,"eof after cfgdebug token");
											}
	;

assignmentlist: assignment			{DP1(BCFGYACCSTUFF,"got_assignmentlist:assignment ");}
	;

assignment:  VARIABLE junk		{ 
										 /* ignore */ ; 
										 DP1(BCFGYACCSTUFF,"got_VARIABLE_junk ");
										 free($1);
										}
	| VARIABLE MYEOF				{error(SYNTAX,"eof after variable, no =value");
										}
	| VARIABLE '=' something {
								 DP1(BCFGYACCSTUFF,"got_VARIABLE_=_something ");
								 if (DoAssignment($1,$3) != 0) {
									/* error found in variable or primitive; free mem */
									PrimitiveFreeMem(&$3);
								 }
								 free($1);  /* free variable since no longer needed */
								}
	;

something: WORD							{DP1(BCFGYACCSTUFF,"in_something_got_WORD ");
												 $$.stringlist.type=STRINGLIST;/*should be type STRING */
												 $$.stringlist.string=$1;/*should be $$.string.words=$1;*/
												 $$.stringlist.uma=NULL;
												 $$.stringlist.next=(struct stringlist *)NULL;
												}
	| NULLSTRING							{DP1(BCFGYACCSTUFF,"in_something_got_NULLSTRING ");
												 $$.stringlist.type=STRINGLIST;/*should be type STRING */
												 $$.stringlist.string=$1;/*should be $$.string.words=$1;*/
												 $$.stringlist.uma=NULL;
												 $$.stringlist.next=(struct stringlist *)NULL;
												}
	| MYEOF									{error(SYNTAX,"variable=<eof> detected");
												 /* forge a return value to free(3) later */
												 $$.stringlist.type=STRINGLIST;
												 $$.stringlist.string=strdup("my_eof1");
									 			 $$.stringlist.uma=NULL;
									 			 $$.stringlist.next=(struct stringlist *)NULL;
												}
	| DQUOTE whitespace MYEOF			{
												 error(SYNTAX,"variable=\"<eof> detected");
												 /* forge a return value to free(3) later */
												 $$.stringlist.type=STRINGLIST;
												 $$.stringlist.string=strdup("my_eof2");
									 			 $$.stringlist.uma=NULL;
									 			 $$.stringlist.next=(struct stringlist *)NULL;
												}
	| DQUOTE whitespace ManyWords whitespace MYEOF {
												 error(SYNTAX,"variable=\"blah <eof> "
																  "detected (missing end \")");
												 $$=$3; /* free(3) up all we can later on */
												}
	| DQUOTE whitespace ManyWords whitespace DQUOTE			{DP1(BCFGYACCSTUFF,"got_DQ_ManyWords ");
												 $$=$3;
												}
	| SQUOTE whitespace MYEOF			{
												 error(SYNTAX,"variable='<eof> detected");
												 /* forge a return value to free(3) later */
												 $$.stringlist.type=STRINGLIST;
												 $$.stringlist.string=strdup("my_eof3");
									 			 $$.stringlist.uma=NULL;
									 			 $$.stringlist.next=(struct stringlist *)NULL;
												}
	| SQUOTE whitespace ManyWords whitespace MYEOF {
												 error(SYNTAX,"variable='blah <eof> "
																  "detected (missing end ')");
												 $$=$3; /* free(3) up all we can later on */
												}
	| SQUOTE whitespace ManyWords whitespace SQUOTE			{DP1(BCFGYACCSTUFF,"got_SQ_ManyWords ");
												 $$=$3;
												}
	;

ManyWords:  WORD				{
									 DP1(BCFGYACCSTUFF,"in_ManyWords_got_WORD ");
									 $$.stringlist.type=STRINGLIST;
									 $$.stringlist.string=$1;
									 $$.stringlist.uma=NULL;
									 $$.stringlist.next=(struct stringlist *)NULL;
									}
	| NULLSTRING				{
									 DP1(BCFGYACCSTUFF,"got_NULLSTRING ");
									 $$.stringlist.type=STRINGLIST;/*should be STRING */
									 $$.stringlist.string=$1;
									 $$.stringlist.uma=NULL;
									 $$.stringlist.next=(struct stringlist *)NULL;
									}

	| ManyWords whitespace word {
									 union primitives *sl;
									 struct stringlist *walk;
									 DP1(BCFGYACCSTUFF,"in_ManyWords_got_ManyWordsWHITESPACE_WORD ");
									 /* ok do real work here */
									 sl=(union primitives *) malloc(sizeof(union primitives));
									 if (!sl) {
 										 fatal("couldn't malloc for stringlist");
									 }
								    sl->stringlist.type = STRINGLIST;
									 sl->stringlist.uma = sl;
								    sl->stringlist.next = (struct stringlist *)NULL;
									 sl->stringlist.string = $3.stringlist.string;
								    if ($1.stringlist.next == NULL) {
									    $1.stringlist.next = &sl->stringlist;
									 } else {
                               walk=$1.stringlist.next;
									    while (walk->next != NULL) walk=walk->next;
									       walk->next = &sl->stringlist;
									 }
									 $$=$1;
									}
	;

word: WORD						{
									 DP1(BCFGYACCSTUFF,"got_WORD ");
									 $$.stringlist.type=STRINGLIST;/*should be STRING */
									 $$.stringlist.string=$1;
									 $$.stringlist.uma=NULL;
									 $$.stringlist.next=(struct stringlist *)NULL;
									}
	|  NULLSTRING				{
									 DP1(BCFGYACCSTUFF,"got_NULLSTRING ");
									 $$.stringlist.type=STRINGLIST;/*should be STRING */
									 $$.stringlist.string=$1;
									 $$.stringlist.uma=NULL;
									 $$.stringlist.next=(struct stringlist *)NULL;
									}
	;

whitespace:	/* empty */
	| WHITESPACE			{DP1(BCFGYACCFLUFF,"got_WHITESPACE ");
								 free($1);
								}
	;

%%

/* we insert this in here from libl so that the yy's can be renamed to zz
 */
#pragma ident	"@(#)libl:lib/yyless.c	1.9"
#if defined(__cplusplus) || defined(__STDC__)
void yyless(int x)
#else
yyless(x)
#endif
{
	extern char yytext[];
	register char *lastch, *ptr;
	extern int yyleng;
	extern int yyprevious;
	lastch = yytext+yyleng;
	if (x>=0 && x <= yyleng)
		ptr = x + yytext;
	else
	/*
	 * The cast on the next line papers over an unconscionable nonportable
	 * glitch to allow the caller to hand the function a pointer instead of
	 * an integer and hope that it gets figured out properly.  But it's
	 * that way on all systems .   
	 */
		ptr = (char *) x;
	while (lastch > ptr)
		yyunput(*--lastch);
	*lastch = 0;
	if (ptr >yytext)
		yyprevious = *--lastch;
	yyleng = ptr-yytext;
}
