%{
#pragma ident "@(#)bcfglex.l	24.1"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 * set tabstop=3 
 *
 * bcfg lexer
 *
 * The idea of this is to emulate a UnixWare 2.1 bcfg file
 * and allow room for future (syntax) enhancements.
 * We must also allow the dcu to source in the bcfg file so a bcfg file
 * must be a valid shell script ***in the sense that you can only set
 * environment variables***
 * That's why our "extensions" ($version 1, ADAPTER:) must have a leading
 * pound sign so as to keep the dcu happy but still implementing OSR5 
 * stanza-like AOF functionality.
 *
 */
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "tmpbcfgtab.h" /* token codes from parser */

/* ZZLMAX says how big a physical line will be in a bcfg file.  If
 * we go this many characters without seeing a newline then tmpbcfglex.c
 * barfs with "Input line too long, limit ZZLMAX"
 * the default is 200 which is too small for some bcfg files
 * vi/ex uses LINE_MAX in limits.h which is 2048.
 * after much debate, careful design reviews, and dissertations, and
 * a coin toss, we pick 5K
 */
#ifdef ZZLMAX
#undef ZZLMAX
#endif
#define ZZLMAX (5 * 1024)

unsigned int bcfglinenum;
unsigned int cfgdebug, cfgversion, cfghasdebug;
unsigned int insingle;
unsigned int indouble;
unsigned int quotestate;
unsigned int incomment;
extern char nullstring[];
unsigned int returnedeof;

/* in case one bcfg file terminates in a bizarre state, we must reset its
 * state back to normal.  Typical case:  unmatched quotes.
 */
void
ResetBcfgLexer(void)
{
   BEGIN INITIAL;
   insingle=indouble=incomment=0;
   quotestate=NOQUOTES;
   returnedeof=0;
}

#ifdef DEBUGONLY
/* because we use the standard zzlex routine, end of file conditions
 * aren't handled well, so bcfgparser.y could still be in some state where it's
 * expecting an ending double quote etc.  by returning MYEOF this gives
 * the parser a way to return to its initial state, ready to handle
 * another bcfg file.  Without this then the next bcfg file will see a
 * #$version which returns VERSION which bcfgparser.y isn't ready for
 * and freaks out.
 */
int
zzlex(void)
{
   DP1(BCFGLEXFLUFF, "in zzlex");
   if (returnedeof == 0) {
      int ret=Realzzlex();
      if (ret == 0) {
         /* end of file reached */
         returnedeof=1;
         DP1(BCFGLEXFLUFF, "return_MYEOF_in_zzlex ");
         return(MYEOF);
      }
      return(ret);
   } else {
      /* end of file reached and we've returned MYEOF token, now return EOF */
      return(0);
   }
}
#define zzlex Realzzlex
#endif   /* DEBUGONLY */

%}

%s VERSIONSTATE DEBUGSTATE SAWVARIABLE SAWEQUAL

%%

[ \t]*#						{
								 DP1(BCFGLEXFLUFF,"see '#'\n"); 
								 /* remember that lex will match biggest pattern
								  * so that #ADAPTER and #$version will always match
								  * over just a plain '#' token
								  */
								 incomment++;
								 /* comment; ignore it and don't eat nl too */ ;
								}
\n								{
								 char nextc;
								 DP1(BCFGLEXFLUFF,"see nl ");
								 incomment=0; /* shell comments extend to EOL */
								 if (YYSTATE == DEBUGSTATE) {
									 error(SYNTAX,
                                  "no number supplied for $debug in line %d",
											 bcfglinenum);
									 quotestate=NOQUOTES; /* also sets back to INITIAL */
								 } else
								 if (YYSTATE == VERSIONSTATE) {
									 error(SYNTAX,
                                "no number supplied for $version in line %d",
										  bcfglinenum);
									 quotestate=NOQUOTES; /* also sets back to INITIAL */
								 } else 
								 if (YYSTATE == SAWVARIABLE) {
									 error(SYNTAX, 
										"missing equal sign in line %d",bcfglinenum);
									 quotestate=NOQUOTES; /* also sets back to INITIAL */
								 }
								 bcfglinenum++; 
								 if ((nextc=input()) == 0) {
									 /* we call ResetBcfgLexer later to BEGIN INITIAL */
									 DP1(BCFGLEXRETURN,"MYEOF ");
									 return MYEOF;
								 } else {
									 unput(nextc);
								 }
								 if (quotestate == NOQUOTES) { /* possibly from above */
									/* allow another define, debug or section def. */
									BEGIN INITIAL;  
									if (YYSTATE == DEBUGSTATE || 
										 YYSTATE == VERSIONSTATE ||
										 YYSTATE == SAWVARIABLE) {
										DP1(BCFGLEXRETURN,"JUNK ");
										return JUNK;  /* keep parser happy */
									}
								 } else {
									/* inside a quote -- add newline to buffer */
									DP1(BCFGLEXRETURN,"return_NL_as_WORD ");
									yylval.strval=strdup("\n");
									return WORD;
								 }
								}
<INITIAL>^"#$debug"		{
								 /* don't have to worry about incomment here */
								 BEGIN DEBUGSTATE;
								 DP1(BCFGLEXRETURN,"DEBUG ");
								 return CFGDEBUG;
								}
<INITIAL>^"#$version"	{
								 /* don't have to worry about incomment here */
								 BEGIN VERSIONSTATE;
								 /* version is number */
								 DP1(BCFGLEXRETURN,"VERSION ");
								 return VERSION; 
								}
<INITIAL>^"#MANIFEST:"$	{
								 /* don't have to worry about incomment here */
								 if (ensurev1()) {
									section = MANIFEST; 
									DP1(BCFGLEXSECTION,"now in manifest\n");
								 }
								}
<INITIAL>^"#DRIVER:"$	{
								 /* don't have to worry about incomment here */
								 if (ensurev1()) {
									section = DRIVER; 
									DP1(BCFGLEXSECTION,"now in driver\n");
								 }
								}
<INITIAL>^"#ADAPTER:"$	{
								 /* don't have to worry about incomment here */
								 if (ensurev1()) {
									section = ADAPTER; 
									DP1(BCFGLEXSECTION,"now in adapter\n");
								 }
								}
<SAWEQUAL>\"\"				{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"NULLSTRING ");
									if (quotestate == NSYD || 
										  quotestate == YSYD) {
										/* already in a double quote! */
									  error(SYNTAX, "already in a double quote!");
									}
									yylval.strval=strdup(nullstring);
									return NULLSTRING;
								 }
								}
<SAWEQUAL>\"				{
								 if (!incomment) {
									/* add ability to have single double quote in 
									 * text as needed by passing it up
									 * as a word -- but as long as its imbedded
                            * in single quotes.  StringListPrint does
									 * necessary massaging to turn this
									 * separate WORD back into a double quote
									 */
									if ((quotestate == YSND) || (quotestate == YSYD)) {
										DP1(BCFGLEXRETURN,"DQUOTE_AS_WORD ");
										yylval.strval=strdup(yytext);
										/* don't modify quotestate as when we
										 * get a newline we check to see if we're still
										 * in some kind of quote to pass up nl_as_word 
										 */
										return WORD;
									} 
									if (quotestate == NOQUOTES) {
									  quotestate = NSYD;
									} else 
									if (quotestate == YSND) {
										 quotestate = YSYD;     /* never reached now */
									} else
									if (quotestate == YSYD) { 
										 quotestate = YSND;     /* never reached now */
									} else
									if (quotestate == NSYD) {
										 quotestate = NOQUOTES;
									}
									indouble=!indouble;
									DP1(BCFGLEXRETURN,"DQUOTE ");
									return DQUOTE; 
								 }
								}
<SAWEQUAL>\'\'				{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"NULLSTRING ");
									if (quotestate == YSND ||
										  quotestate == YSYD) {
										/* already in a single quote! */
										error(SYNTAX, "already in a single quote!");
									}
									yylval.strval=strdup(nullstring);
									return NULLSTRING;
								 }
								}
<SAWEQUAL>\'				{
								 if (!incomment) {
									/* add ability to have single quote in 
									 * text as apostrophe by passing it up
									 * as a word -- but as long as its imbedded
                            * in double quotes.  StringListPrint does
									 * necessary massaging to turn this
									 * separate WORD back into an apostrophe
									 */
									if ((quotestate == NSYD) || (quotestate == YSYD)) {
										DP1(BCFGLEXRETURN,"SQUOTE_AS_WORD ");
										yylval.strval=strdup(yytext);
										/* don't modify quotestate as when we
										 * get a newline we check to see if we're still
										 * in some kind of quote to pass up nl_as_word 
										 */
										return WORD;
									} 
									if (quotestate == NOQUOTES) {
									  quotestate = YSND;
									} else 
									if (quotestate == YSND) {
										 quotestate = NOQUOTES;
									} else
									if (quotestate == YSYD) {
										 quotestate = NSYD;   /* never reached now */
									} else
									if (quotestate == NSYD) {
										 quotestate = YSYD;   /* never reached now */
									}
									insingle=!insingle;
									DP1(BCFGLEXRETURN,"SQUOTE ");
									return SQUOTE; 
								 }
								}
<SAWEQUAL>","				{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"COMMA_AS_WORD ");
									yylval.strval=strdup(yytext);
									return WORD;
								 }
								}
<VERSIONSTATE>[a-zA-Z+()/\.\$=_-]+	{
								 if (!incomment) {
									error(SYNTAX, 
									"$version requires a number not '%s' in line %d",
									yytext,bcfglinenum);
									/* BEGIN INITIAL; not yet; may be more text */
									DP1(BCFGLEXRETURN,"JUNK ");
									return JUNK;  /* to keep parser happy */
								 }
								}
<DEBUGSTATE>[a-zA-Z+()/\.\$=_-]+	{
								 if (!incomment) {
									error(SYNTAX, 
									"$debug requires a number not '%s' in line %d",
									yytext,bcfglinenum);
									/* BEGIN INITIAL; not yet; may be more text */
									DP1(BCFGLEXRETURN,"JUNK ");
									return JUNK;  /* to keep parser happy */
								 }
								}
<INITIAL>[ \t]+			{
								 if (!incomment) {
									/* ignore */;
									DP1(BCFGLEXFLUFF,"in_INITIAL_ignore_space_or_tab ");
								 }
								}
<INITIAL>[a-zA-Z_]+[0-9]*[0-9\0133\0135]*		{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"VARIABLE "); 
									BEGIN SAWVARIABLE;
									yylval.strval=strdup(yytext);
									return VARIABLE; 
								 }
								}
<SAWVARIABLE>"="			{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"EQUAL "); 
									BEGIN SAWEQUAL;
									return '=';
								 }
								}
<SAWEQUAL>[A-Za-z0-9\+\-()/\.\,\$\{\}\?\`:=_]+	{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"WORD ");
									/* allow /._ and numbers in file names
									 * also allow { and } so environment variables
									 * can be set in certain places like CONFIG_CMDS
									 */ 
									yylval.strval=strdup(yytext); /* 0-9 also for CUSTOMx */
									return WORD; 
								 }
								}
<VERSIONSTATE>[ \t]+		{/* ignore */;}
<DEBUGSTATE>[ \t]+		{/* ignore */;}
<SAWEQUAL>[ \t]+$			{/* ignore whitespace specifically at end of line */;}
<SAWEQUAL>[ \t]+			{
								 if (!incomment) {
									if (quotestate == NOQUOTES) {
										 error(SYNTAX, 
		"no whitespace unless surrounded by single or double quotes in line %d",
										bcfglinenum);
									}
									DP1(BCFGLEXRETURN,"WHITESPACE ");
									yylval.strval=strdup(yytext);
									return WHITESPACE;
								 }
								}
<DEBUGSTATE>0[xX][0-9a-fA-F]+			{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"HEXNUM "); /* hex check comes first */
									yylval.strval=strdup(yytext);
									BEGIN INITIAL; 
                           /* leave DEBUGSTATE; we got our number */
									return HEXNUM;
								 }
								}
<DEBUGSTATE>0[0-9]+		{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"OCTNUM "); /* put octal check before decimal */
									yylval.strval=strdup(yytext);
									BEGIN INITIAL; 
									/* leave DEBUGSTATE; we got our number */
									return OCTNUM;
								 }
								}
<DEBUGSTATE>[0-9]+		{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"DECNUM "); /* decimal check must come last */
									yylval.strval=strdup(yytext);
									BEGIN INITIAL; 
									/* leave DEBUGSTATE; we got our number */
									return DECNUM;
								 }
								}
<VERSIONSTATE>0[xX][0-9a-fA-F]+			{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"HEXNUM "); /* hex check comes first */
									yylval.strval=strdup(yytext);
									BEGIN INITIAL; 
									/* leave VERSIONSTATE; we got our number */
									return HEXNUM;
								 }
								}
<VERSIONSTATE>0[0-9]+	{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"OCTNUM "); /* put octal check before decimal */
									yylval.strval=strdup(yytext);
									BEGIN INITIAL; /* leave VERSIONSTATE; we got our number */
									return OCTNUM;
								 }
								}
<VERSIONSTATE>[0-9]+		{
								 if (!incomment) {
									DP1(BCFGLEXRETURN,"DECNUM "); /* decimal check must come last */
									yylval.strval=strdup(yytext);
									BEGIN INITIAL; /* leave VERSIONSTATE; we got our number */
									return DECNUM;
								 }
								}
.								{
								 if (!incomment) {
									error(SYNTAX,
											"Unexpected character '%s' in line %d",
											yytext,bcfglinenum); 
								 }
								}
%% 

