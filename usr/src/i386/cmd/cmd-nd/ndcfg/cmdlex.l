%{
#pragma ident "@(#)cmdlex.l	27.2"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/*
 * set tabstop=3 throughout entire file
 *
 * COMMAND lexer
 *  reads commands from user after bcfg file read in
 */
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "tmpcmdtab.h" /* token codes from parser */

/* YYLMAX says how big a physical line will be in receiving user input.  If
 * we go this many characters without seeing a newline then tmpcmdlex.c
 * barfs with "Input line too long, limit YYLMAX"
 * the default is 200 which is too small for a large idinstall command with
 * many arguments.
 * vi/ex uses LINE_MAX in limits.h which is 2048.
 * after much debate, careful design reviews, and dissertations, and
 * a coin toss, we pick 5K
 */
#ifdef YYLMAX
#undef YYLMAX
#endif
#define YYLMAX (5 * 1024)

unsigned int cmdlinenum=1;

/* if we hit EOF while reading cmds, it's ok to quit */
int
yywrap(void)
{
   return(1);
}

/* this routine is necessary because if the text we feed to the
 * user isn't valid and causes the error yacc routine to be executed
 * then we must free up any and all WORDs that we allocated space
 * for with strdup (which calls malloc)
 */

#define MAXWORDS 200
char *cmdword[MAXWORDS];
unsigned int numcmdwords;

char *
Mycmdstrdup(const char *s)
{
   char *ret;

   if (numcmdwords >= MAXWORDS) {
      notice("Mycmdstrdup: ignoring word '%s'",s);
      return(NULL);  /* to emulate real strdup */
   }
   ret=cmdword[numcmdwords]=strdup(s);
   if (ret != NULL) numcmdwords++;
   return(ret);
}

/* likewise, if we hit error then we must remember to free up any and
 * all WORDs that were sent up to the parser.  
 */
void
CmdFreeWords(void)
{
   int loop;

   for (loop=0;loop < numcmdwords; loop++) {
      free(cmdword[loop]);
   }
   numcmdwords=0;
   return;
}

/* we want to know what characters we read and send them to our log
 * file.  redefining output() and ECHO won't do anything because
 * output() is only called for unmatched characters.  Since we match
 * all characters below (the final '.'), output will never be called
 * automatically by the lexer.  Instead, we use a LOG macro which
 * will log the received yytext to our logging file, if supplied
 * with the 'l' flag.
 */

#define LOGYYTEXT logyytext()

extern u_int lflag;
extern FILE *logfilefp;

void
logyytext(void)
{
   time_t now;

   if (lflag > 0 && logfilefp != NULL) {
      time(&now);
      fprintf(logfilefp,"%s'%s' at %s",PROMPT,yytext,ctime(&now));
   }
}

%}

%s DEBUGSTATE TCLSTATE NOTINITIAL RESPUTSTATE STAMPSTATE WANTSPACE 
%s NLIST1 NLIST2 VENDORSTATE

%%

[ \t]*#.*					{
								 DP1(CMDLEXFLUFF,"see '#'\n"); 
								 /* comment; ignore it and don't eat nl too */ ;
								}
<INITIAL>\n					{ /* a newline all by itself is nothing */
								 DP1(CMDLEXFLUFF,"NOTHING ");
								 if (logfilefp != NULL) {
									 time_t now;

									 time(&now);
									 fprintf(logfilefp,"%s'\\n' at %s",
										PROMPT,ctime(&now));
								 }
								 return NOTHING;
								}
<DEBUGSTATE>\n				|
<NLIST1>\n					|
<NLIST2>\n					|
<VENDORSTATE>\n			|
<TCLSTATE>\n				|
<WANTSPACE>\n				|
<RESPUTSTATE>\n			|
<STAMPSTATE>\n				|
<NOTINITIAL>\n				{ 
								 /* any text follwed by a newline: return newline */
								 DP1(CMDLEXFLUFF,"return '\\n' ");
								 cmdlinenum++; 
								 if (logfilefp != NULL) {
									 time_t now;

									 time(&now);
									 fprintf(logfilefp,"%s'\\n' at %s",
										PROMPT,ctime(&now));
								 }
								 BEGIN INITIAL;/*end DEBUGSTATE, TCLSTATE, NOTINITIAL*/
								 return '\n';
								}
<INITIAL>","				{  /* XXX why is this here??? */
								 DP1(CMDLEXRETURN,"COMMA ");
								 BEGIN NOTINITIAL;
								 LOGYYTEXT;
								 return '\n';
								}
<INITIAL>[ \t]+			|
<DEBUGSTATE>[ \t]+		|
<NLIST1>[ \t]+				|
<NLIST2>[ \t]+				|
<VENDORSTATE>[ \t]+		|
<TCLSTATE>[ \t]+			|
<RESPUTSTATE>[ \t]+		|
<STAMPSTATE>[ \t]+		|
<NOTINITIAL>[ \t]+		{
									DP1(CMDLEXFLUFF,"ignore_space_or_tab ");
									LOGYYTEXT;
									/* ignore */;
								}
<INITIAL>^version[s]?	|
<INITIAL>^VERSION[S]?	{
									DP1(CMDLEXFLUFF,"VERSION ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return VERSION;
								}
<INITIAL>^location[s]?	|
<INITIAL>^LOCATION[S]?	{
									DP1(CMDLEXFLUFF,"LOCATION ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return LOCATION;
								}
<INITIAL>^driver[s]?		|
<INITIAL>^DRIVER[S]?		{
									DP1(CMDLEXFLUFF,"DRIVERCMD ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return DRIVERCMD;
								}
<INITIAL>^resget			|
<INITIAL>^RESGET			{
									DP1(CMDLEXFLUFF,"RESGET ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return RESGET;
								}
<INITIAL>^getstamp		|
<INITIAL>^GETSTAMP		{
									DP1(CMDLEXFLUFF,"GETSTAMP ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return GETSTAMP;
								}
<INITIAL>^stamp			|
<INITIAL>^STAMP			{
									DP1(CMDLEXFLUFF,"STAMP ");
									BEGIN STAMPSTATE;
									LOGYYTEXT;
									return STAMP;
								}
<INITIAL>^resput			|
<INITIAL>^RESPUT			{
									DP1(CMDLEXFLUFF,"RESPUT ");
									BEGIN RESPUTSTATE;
									LOGYYTEXT;
									return RESPUT;
								}
<INITIAL>^elementtoindex	|
<INITIAL>^ELEMENTTOINDEX	{
									DP1(CMDLEXFLUFF,"ELEMENTTOINDEX ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return ELEMENTTOINDEX;
								}
<INITIAL>^unloadall		|
<INITIAL>^UNLOADALL		{
									DP1(CMDLEXFLUFF,"UNLOADALL ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return UNLOADALL;
								}
<INITIAL>^showfailover	|
<INITIAL>^SHOWFAILOVER	{
									DP1(CMDLEXFLUFF,"SHOWFAILOVER ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWFAILOVER;
								}
<INITIAL>^more				|
<INITIAL>^MORE				{
									DP1(CMDLEXFLUFF,"MORE ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return MORE;
								}
<INITIAL>^showvariable	|
<INITIAL>^SHOWVARIABLE	{
									DP1(CMDLEXFLUFF,"SHOWVARIABLE ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWVARIABLE;
								}
<INITIAL>^showlines		|
<INITIAL>^SHOWLINES		{
									DP1(CMDLEXFLUFF,"SHOWLINES ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWLINES;
								}
<INITIAL>^quiet			|
<INITIAL>^QUIET			{
									DP1(CMDLEXFLUFF,"QUIET ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return QUIET;
								}
<INITIAL>^resmgr			|
<INITIAL>^RESMGR			{
									DP1(CMDLEXFLUFF,"RESMGR ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return RESMGR;
								}
<INITIAL>^isaautodetect	|
<INITIAL>^ISAautodetect	|
<INITIAL>^ISAAUTODETECT	{
									DP1(CMDLEXFLUFF,"ISAAUTODETECT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return ISAAUTODETECT;
								}
<INITIAL>^dangerousisaautodetect |
<INITIAL>^dangerousISAautodetect |
<INITIAL>^DANGEROUSISAAUTODETECT {
									DP1(CMDLEXFLUFF,"DANGEROUSISAAUTODETECT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return DANGEROUSISAAUTODETECT;
								}
<INITIAL>^getisaparams	|
<INITIAL>^getISAparams	|
<INITIAL>^showisaparams	|
<INITIAL>^showISAparams	|
<INITIAL>^SHOWISAPARAMS	|
<INITIAL>^GETISAPARAMS	{
									DP1(CMDLEXFLUFF,"GETISAPARAMS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return GETISAPARAMS;
								}
<INITIAL>^getallisaparams	|
<INITIAL>^getallISAparams	|
<INITIAL>^showallisaparams	|
<INITIAL>^showallISAparams	|
<INITIAL>^SHOWALLISAPARAMS	|
<INITIAL>^GETALLISAPARAMS	{
									DP1(CMDLEXFLUFF,"GETALLISAPARAMS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return GETALLISAPARAMS;
								}
<INITIAL>^showcustomnum	|
<INITIAL>^SHOWCUSTOMNUM	{
									DP1(CMDLEXFLUFF,"SHOWCUSTOMNUM ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWCUSTOMNUM;
								}
<INITIAL>^showcustom		|
<INITIAL>^SHOWCUSTOM		{
									DP1(CMDLEXFLUFF,"SHOWCUSTOM ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWCUSTOM;
								}
<INITIAL>^idinstall		|
<INITIAL>^IDINSTALL		{
									DP1(CMDLEXFLUFF,"IDINSTALL ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return IDINSTALL;
								}
<INITIAL>^idremove		|
<INITIAL>^IDREMOVE		{
									DP1(CMDLEXFLUFF,"IDREMOVE ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return IDREMOVE;
								}
<INITIAL>^idmodify		|
<INITIAL>^IDMODIFY		{
									DP1(CMDLEXFLUFF,"IDMODIFY ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return IDMODIFY;
								}
<INITIAL>hpsldump			|
<INITIAL>HPSLDUMP			{
									DP1(CMDLEXFLUFF,"HPSLDUMP ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return HPSLDUMP;
								}
<INITIAL>hpslsuspend		|
<INITIAL>HPSLSUSPEND		{
									DP1(CMDLEXFLUFF,"HPSLSUSPEND ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return HPSLSUSPEND;
								}
<INITIAL>hpslresume		|
<INITIAL>HPSLRESUME		{
									DP1(CMDLEXFLUFF,"HPSLRESUME ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return HPSLRESUME;
								}
<INITIAL>hpslgetstate	|
<INITIAL>HPSLGETSTATE	{
									DP1(CMDLEXFLUFF,"HPSLGETSTATE ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return HPSLGETSTATE;
								}
<INITIAL>hpslcanhotplug	|
<INITIAL>HPSLCANHOTPLUG	{
									DP1(CMDLEXFLUFF,"HPSLCANHOTPLUG ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return HPSLCANHOTPLUG;
								}
<INITIAL>gethwkey			|
<INITIAL>GETHWKEY			{
									DP1(CMDLEXFLUFF,"GETHWKEY ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return GETHWKEY;
								}
<INITIAL>^promiscuous	|
<INITIAL>^PROMISCUOUS	{
									DP1(CMDLEXFLUFF,"PROMISCUOUS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return PROMISCUOUS;
								}
<INITIAL>^determineprom	|
<INITIAL>^DETERMINEPROM	{
									DP1(CMDLEXFLUFF,"DETERMINEPROM ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return DETERMINEPROM;
								}
<INITIAL>^iicard			|
<INITIAL>^IICARD			{
									DP1(CMDLEXFLUFF,"IICARD ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return IICARD;
								}
<INITIAL>^orphans	|
<INITIAL>^ORPHANS	{
									DP1(CMDLEXFLUFF,"ORPHANS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return ORPHANS;
								}
<INITIAL>^pcivendor		|
<INITIAL>^PCIVENDOR		{
									DP1(CMDLEXFLUFF,"PCIVENDOR ");
									BEGIN VENDORSTATE;
									LOGYYTEXT;
									return PCIVENDOR;
								}
<INITIAL>resshowunclaimed	|
<INITIAL>RESSHOWUNCLAIMED	{
									DP1(CMDLEXFLUFF,"RESSHOWUNCLAIMED ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return RESSHOWUNCLAIMED;
								}
<INITIAL>^showalltopologies	|
<INITIAL>^SHOWALLTOPOLOGIES 	|
<INITIAL>^showtopologies 		|
<INITIAL>^SHOWTOPOLOGIES	{
									DP1(CMDLEXFLUFF,"SHOWALLTOPOLOGIES ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWALLTOPOLOGIES;
								}
<INITIAL>^showhelpfile	|
<INITIAL>^SHOWHELPFILE	{
									DP1(CMDLEXFLUFF,"SHOWHELPFILE ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWHELPFILE;
								}
<INITIAL>^showtopo		|
<INITIAL>^SHOWTOPO		{
									DP1(CMDLEXFLUFF,"SHOWTOPO ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWTOPO;
								}
<INITIAL>^verbose			|
<INITIAL>^VERBOSE			{
									DP1(CMDLEXFLUFF,"VERBOSE ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return VERBOSE;
								}
<INITIAL>^clear			|
<INITIAL>^CLEAR			{
									DP1(CMDLEXFLUFF,"CLEAR ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return CLEAR;
								}
<INITIAL>^dlpimdi			|
<INITIAL>^DLPIMDI			{
									DP1(CMDLEXFLUFF,"DLPIMDI ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return DLPIMDI;
								}
<INITIAL>^showindex		|
<INITIAL>^SHOWINDEX		{
									DP1(CMDLEXFLUFF,"SHOWINDEX ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWINDEX;
								}
<INITIAL>^showname		|
<INITIAL>^SHOWNAME		{  
									DP1(CMDLEXFLUFF,"SHOWNAME ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWNAME;
								}
<INITIAL>^showrejects	|
<INITIAL>^SHOWREJECTS	{
									DP1(CMDLEXFLUFF,"SHOWREJECTS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWREJECTS;
								}
<INITIAL>^showdriver		|
<INITIAL>^SHOWDRIVER		{
									DP1(CMDLEXFLUFF,"SHOWDRIVER ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWDRIVER;
								}
<INITIAL>^showbus			|
<INITIAL>^SHOWBUS			{ 
									DP1(CMDLEXFLUFF,"SHOWBUS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWBUS;
								}
<INITIAL>^showisacurrent	|
<INITIAL>^showISAcurrent	|
<INITIAL>^SHOWISACURRENT	{
									DP1(CMDLEXFLUFF,"SHOWISACURRENT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWISACURRENT;
								}
<INITIAL>^showcustomcurrent	|
<INITIAL>^showCUSTOMcurrent	|
<INITIAL>^SHOWCUSTOMCURRENT	{
									DP1(CMDLEXFLUFF,"SHOWCUSTOMCURRENT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWCUSTOMCURRENT;
								}
<INITIAL>^loaddir			|
<INITIAL>^LOADDIR			{
									DP1(CMDLEXFLUFF,"LOADDIR ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return LOADDIR;
								}
<INITIAL>^loadfile		|
<INITIAL>^LOADFILE		{
									DP1(CMDLEXFLUFF,"LOADFILE ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return LOADFILE;
								}
<INITIAL>^bcfghasverify |
<INITIAL>^BCFGHASVERIFY {
									DP1(CMDLEXFLUFF,"BCFGHASVERIFY ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return BCFGHASVERIFY;
								}
<INITIAL>^DEBUG			|
<INITIAL>^debug			{
									DP1(CMDLEXFLUFF,"DEBUG ");
									BEGIN DEBUGSTATE;
									LOGYYTEXT;
									return DEBUG;
								}
<INITIAL>^showserialttys	|
<INITIAL>^SHOWSERIALTTYS	{
									DP1(CMDLEXFLUFF,"SHOWSERIALTTYS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SHOWSERIALTTYS;
								}
<INITIAL>^bcfgpathtoindex	|
<INITIAL>^BCFGPATHTOINDEX	{
									DP1(CMDLEXFLUFF,"BCFGPATHTOINDEX ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return BCFGPATHTOINDEX;
								}
<INITIAL>^nlist			|
<INITIAL>^NLIST			{
									DP1(CMDLEXFLUFF,"NLIST ");
									BEGIN NLIST1;
									LOGYYTEXT;
									return NLIST;
								}
<INITIAL>^numwords		|
<INITIAL>^NUMWORDS		{
									DP1(CMDLEXFLUFF,"NUMWORDS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return NUMWORDS;
								}
<INITIAL>^numlines		|
<INITIAL>^NUMLINES		{
									DP1(CMDLEXFLUFF,"NUMLINES ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return NUMLINES;
								}
<INITIAL>^sysdat			|
<INITIAL>^SYSDAT			{
									DP1(CMDLEXFLUFF,"SYSDAT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return SYSDAT;
								}
<INITIAL>^test				|
<INITIAL>^TEST				{
									DP1(CMDLEXFLUFF,"TEST ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return TEST;
								}
<INITIAL>^xid				|
<INITIAL>^XID				{
									DP1(CMDLEXFLUFF,"XID ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return XID;
								}
<INITIAL>^resshowkey		|
<INITIAL>^RESSHOWKEY		{
									DP1(CMDLEXFLUFF,"RESSHOWKEY ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return RESSHOWKEY;
								}
<INITIAL>^tcl				|
<INITIAL>^TCL				{
									DP1(CMDLEXFLUFF,"TCL ");
									BEGIN TCLSTATE;
									LOGYYTEXT;
									return TCL;
								}
<INITIAL>^auths			|
<INITIAL>^AUTHS			{
									DP1(CMDLEXFLUFF,"AUTHS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return AUTHS;
								}
<INITIAL>^count			|
<INITIAL>^COUNT			{
									DP1(CMDLEXFLUFF,"COUNT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return COUNT;
								}
<INITIAL>writestatus		|
<INITIAL>WRITESTATUS		{
									DP1(CMDLEXFLUFF,"WRITESTATUS ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return WRITESTATUS;
								}
<INITIAL>^resdump			|
<INITIAL>^RESDUMP			{
									DP1(CMDLEXFLUFF,"RESDUMP ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return RESDUMP;
								}
<INITIAL>^pcishort		|
<INITIAL>^PCISHORT		{
									DP1(CMDLEXFLUFF,"PCISHORT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return PCISHORT;
								}
<INITIAL>^i2oshort		|
<INITIAL>^I2Oshort		|
<INITIAL>^I2OSHORT		{
									DP1(CMDLEXFLUFF,"I2OSHORT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return I2OSHORT;
								}
<INITIAL>^i2olong			|
<INITIAL>^I2Olong			|
<INITIAL>^I2OLONG			{
									DP1(CMDLEXFLUFF,"I2OLONG ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return I2OLONG;
								}
<INITIAL>^pcilong			|
<INITIAL>^PCILONG			{
									DP1(CMDLEXFLUFF,"PCILONG ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return PCILONG;
								}
<INITIAL>^eisashort		|
<INITIAL>^EISASHORT		{
									DP1(CMDLEXFLUFF,"EISASHORT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return EISASHORT;
								}
<INITIAL>^eisalong		|
<INITIAL>^EISALONG		{
									DP1(CMDLEXFLUFF,"EISALONG ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return EISALONG;
								}
<INITIAL>^mcashort		|
<INITIAL>^MCASHORT		{
									DP1(CMDLEXFLUFF,"MCASHORT ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return MCASHORT;
								}
<INITIAL>^mcalong			|
<INITIAL>^MCALONG			{
									DP1(CMDLEXFLUFF,"MCALONG ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return MCALONG;
								}
<INITIAL>^"!"				{
									DP1(CMDLEXFLUFF,"EXECCMD ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return EXECCMD;
								}
<INITIAL>^"?"				|
<INITIAL>^help				|
<INITIAL>^HELP				{
									DP1(CMDLEXFLUFF,"HELP ");
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return HELP;
								}
<INITIAL>^[qQ]$			|
<INITIAL>^bye				|
<INITIAL>^BYE				|
<INITIAL>^exit				|
<INITIAL>^EXIT				|
<INITIAL>^quit				|
<INITIAL>^QUIT				{
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									return 0;   /* EOF */
								}
<WANTSPACE>\}				{
									/* don't include '{' in the returned WORD */
									DP1(CMDLEXFLUFF,"CLOSEBRACE ");
									/* since we can have additional arguments go back
									 * to NOTINITIAL state
									 */
									BEGIN NOTINITIAL;
									LOGYYTEXT;
									/* don't return anything here either */
								}
<INITIAL>[A-Za-z0-9+\0133\0135()/\.\$=_-]+				|
<NLIST1>[A-Za-z0-9+\0133\0135\041()/\.\$\,\"\'\\\*=\|:_-]+	|
<NOTINITIAL>[A-Za-z0-9+\0133\0135\041()/\.\$\,\"\'\\\*=\|:_-]+\{?	|
<RESPUTSTATE>[A-Za-z0-9+\0133\0135()/\.\$\,\"\'\\\*=\|:_-]+		|
<STAMPSTATE>[A-Za-z0-9+\0133\0135()/\.\$\,\"\'\\\*=\|:_-]+			|
<WANTSPACE>[A-Za-z0-9+\t\0133\0135\040\`\~\!\@\#\%\^\&()/\.\$\,\"\'\\\*=\|_:;\?\<\>-]+	{
/* in the above lines:
 * diff between NOTINITIAL and RESPUTSTATE:
 *   NOTINITIAL adds a '!'
 *   RESPUTSTATE does not
 * diff between WANTSPACE and NOTINITIAL:
 *   WANTSPACE adds ' '
 *   NOTINITIAL does not
 * RESPUTSTATE and STAMPSTATE are the same
 */
								  /* brackets for CUSTOM[1] & IDTUNE[x] */
									DP2(CMDLEXRETURN,"WORD(%s) ",yytext);
									yylval.strval=Mycmdstrdup(yytext);

									/* see if we have an open brace i.e.  FOO={ */
									if ((strlen(yytext) > 1) && 
										 (yytext[strlen(yytext)-1] == '{') && 
										 (YYSTATE == NOTINITIAL)) {
										BEGIN WANTSPACE;
									}

									if (YYSTATE == INITIAL) BEGIN NOTINITIAL;
									if (YYSTATE == RESPUTSTATE) BEGIN WANTSPACE;
									if (YYSTATE == STAMPSTATE) BEGIN WANTSPACE;
									if (YYSTATE == NLIST1) BEGIN NLIST2;
									LOGYYTEXT;
									return WORD; 
								}
<NLIST2>0[xX][0-9a-fA-F]+				|
<VENDORSTATE>0[xX][0-9a-fA-F]+		|
<DEBUGSTATE>0[xX][0-9a-fA-F]+			{
									DP1(CMDLEXRETURN,"HEXNUM "); 
									/* hex check comes first */
									yylval.strval=Mycmdstrdup(yytext);
									LOGYYTEXT;
									return HEXNUM;
								}
<NLIST2>0[0-9]+			|
<VENDORSTATE>0[0-9]+		|
<DEBUGSTATE>0[0-9]+		{
									DP1(CMDLEXRETURN,"OCTNUM "); 
									/* put octal check before decimal */
									yylval.strval=Mycmdstrdup(yytext);
									LOGYYTEXT;
									return OCTNUM;
								}
<NLIST2>[0-9]+				|
<VENDORSTATE>[0-9]+		|
<DEBUGSTATE>[0-9]+		{
									DP1(CMDLEXRETURN,"DECNUM "); 
									/* decimal check must come last */
									yylval.strval=Mycmdstrdup(yytext);
									LOGYYTEXT;
									return DECNUM;
								}
<TCLSTATE>[0-9]+			{  /* decimal numbers only for tcl command */
									DP1(CMDLEXRETURN,"DECNUM "); 
									/* decimal check must come last */
									yylval.strval=Mycmdstrdup(yytext);
									LOGYYTEXT;
									return DECNUM;
								}
.								{
									/* unknown character or user supplied text
									 * with debug instead of a numeric argument
									 */
									error(NOTBCFG,
											"Unexpected character '%s' in line %d",
											yytext,cmdlinenum); 
								}
%% 
