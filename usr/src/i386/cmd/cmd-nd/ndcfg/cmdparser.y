%{
#pragma ident "@(#)cmdparser.y	27.2"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/* here's the cmd parser. set tabstops=3 and use a smaller font and/or resize
 * your window otherwise endure the wrapping
 * XXX run through with yacc debugging and see if any internal yacc params are
 * close to being exceeded
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <sys/cm_i386at.h>
#include "common.h"

extern unsigned char numcmdwords;

%}

%union {
	char *strval;
   union primitives primitive;  /* used by ManyWords */
}

%token <strval> WORD DEBUG LOADDIR LOADFILE HELP COUNT LOCATION DRIVERCMD 
%token <strval> SHOWINDEX SHOWDRIVER SHOWBUS SHOWNAME RESDUMP SHOWFAILOVER
%token <strval> SHOWREJECTS SHOWVARIABLE SHOWLINES QUIET VERBOSE TCL
%token <strval> DECNUM OCTNUM HEXNUM NOTHING MORE EXECCMD
%token <strval> SHOWALLTOPOLOGIES SHOWTOPO RESMGR RESSHOWUNCLAIMED
%token <strval> IDINSTALL SHOWCUSTOM SHOWCUSTOMNUM GETISAPARAMS ISAAUTODETECT
%token <strval> RESGET RESPUT SHOWSERIALTTYS BCFGPATHTOINDEX RESSHOWKEY
%token <strval> SHOWISACURRENT SHOWCUSTOMCURRENT IDREMOVE IDMODIFY
%token <strval> SHOWHELPFILE UNLOADALL BCFGHASVERIFY DANGEROUSISAAUTODETECT
%token <strval> PCISHORT PCILONG EISASHORT EISALONG MCASHORT MCALONG
%token <strval> STAMP GETSTAMP VERSION AUTHS ELEMENTTOINDEX CLEAR
%token <strval> SYSDAT NLIST XID TEST GETALLISAPARAMS NUMWORDS NUMLINES
%token <strval> IICARD ORPHANS PCIVENDOR PROMISCUOUS DETERMINEPROM
%token <strval> HPSLDUMP HPSLSUSPEND HPSLRESUME HPSLGETSTATE
%token <strval> HPSLCANHOTPLUG GETHWKEY I2OSHORT I2OLONG DLPIMDI WRITESTATUS
%token NULLSTRING 

%type <primitive> ManyWords

%start cmdfile

%%

cmdfile:  /* empty */
	| cmdlist	{YYACCEPT;}
	;

cmdlist:	cmd			{
							 EndList();/* sees if cmd generated an error & clears it */
							 if (!nflag) {
								printf("%s",PROMPT);
							 }
							 numcmdwords=0;
							}
	| cmdlist cmd     {
                      /* NOTE:  if you ever run a command and you get two 
                       * prompts this means that the offending command
                       * isn't catching the \n which makes it appear
                       * to be two separate commands, causing the double
                       * prompt.  fix the offending yacc pattern in this file.
                       */
							 EndList();/* sees if cmd generated an error & clears it */
							 if (!nflag) {
								printf("%s",PROMPT);
							 }
							 numcmdwords=0;
							}
	;

cmd: NOTHING			{ /* the whole NOTHING idea is to emulate ndscript 
                        * and ncfgBE.  we need to complain whenever we see 
								* an entirely blank line (i.e. no text, just \n)
								* we only whine when explictly in tcl mode.  simply
								* turning off the prompt with -n won't whine
								*/
PrimeErrorBuffer(2,"Unknown command received");
							 if (tclmode) {
								error(NOTBCFG,"NDCFG_UI_ERR_UNKNOWN_REQ");
							 }
							}
	| authscmd
	| bcfgpathtoindexcmd
	| bcfghasverifycmd
	| cacmd
	| clearcmd
	| countcmd
	| dangerousisaautodetectcmd
	| debugcmd
	| determinepromcmd
	| dlpimdicmd
	| drivercmd
	| dumpcmd
	| elementtoindexcmd
	| execcmd
	| getallisaparamscmd
	| gethwkeycmd
	| getisaparamscmd
	| getstampcmd
	| helpcmd
	| hpslcanhotplugcmd
	| hpsldumpcmd
	| hpslgetstatecmd
	| hpslresumecmd
	| hpslsuspendcmd
	| idinstallcmd
	| idmodifycmd
	| idremovecmd
	| iicardcmd
	| isaautodetectcmd
	| loaddircmd
	| loadfilecmd
	| locationcmd
	| morecmd
	| nlistcmd
	| numlinescmd
	| numwordscmd
	| orphanscmd
	| pcivendorcmd
	| promiscuouscmd
	| quietcmd
	| resgetcmd
	| resmgrcmd
	| resputcmd
	| resshowkeycmd
	| resshowunclaimedcmd
	| showcustomcmd
	| showcustomnumcmd
	| showfailcmd
	| showrejectscmd
	| showindexcmd
	| showdrivercmd
	| showlinescmd
	| showvariablecmd
	| showhelpfilecmd
	| shownamecmd
	| showbuscmd
	| showalltopologiescmd
	| showserialttyscmd
	| showtopocmd
	| showisacurrentcmd
	| showcustomcurrentcmd
	| stampcmd
	| sysdatcmd
	| tclcmd
	| testcmd
	| unloadallcmd
	| verbosecmd
	| versioncmd
	| writestatuscmd
	| xidcmd
	| '\n'			{/*nothing to do */}
	| error '\n' {
PrimeErrorBuffer(2,"Unknown command received");
		yyclearin;  /* discard lookahead */
		yyerrok;
      /* next line necessary for tcl mode but not necessary for non-tcl mode */
		error(NOTBCFG, "ignoring previous command");
		/* continue onwards... */
      EndList();  /* sees if cmd generated an error and clears it */
		CmdFreeWords();
	  }
	;

debugcmd: DEBUG error			{  /* error does NOT cover \n */
PrimeErrorBuffer(3,"the debug command failed");
											error(NOTBCFG,
											"debug requires a numeric argument");
											CmdFreeWords();
										}
	| DEBUG '\n'					{ 
PrimeErrorBuffer(3,"the debug command failed");
											error(NOTBCFG,
											"debug requires a numeric argument");
										}
	| DEBUG WORD '\n'				{  /* have a special case for WORD as we
											 * must call free() 
 											 */
PrimeErrorBuffer(3,"the debug command failed");
										 error(NOTBCFG,
										 "debug requires a numeric argument");
										 free($2);  /* WORD malloced with strdup */
										}
	| DEBUG DECNUM	'\n'			{
										 extern unsigned int cfghasdebug;
PrimeErrorBuffer(3,"the debug command failed");
										 cfgdebug=strtoul($2,NULL,10);
								 	 	 DP2(CMDYACCFLUFF,"cfgdebug now %d\n",cfgdebug);
										 Dflag++;  /* make debugging persistant */
										 if (!cfghasdebug) {
											error(NOTBCFG,
												"cmd debugging not available");
										 }
										 free($2);  /* DECNUM malloced with strdup */
										}
	| DEBUG OCTNUM	'\n'			{extern unsigned int cfghasdebug;
PrimeErrorBuffer(3,"the debug command failed");
										 cfgdebug=strtoul($2,NULL,8);
								 	 	 DP2(CMDYACCFLUFF,"cfgdebug now %d\n",cfgdebug);
										 Dflag++;  /* make debugging persistant */
										 if (!cfghasdebug) {
											error(NOTBCFG,
												"cmd debugging not available");
										 }
										 free($2);  /* OCTNUM malloced with strdup */
										}
	| DEBUG HEXNUM	'\n'			{extern unsigned int cfghasdebug;
PrimeErrorBuffer(3,"the debug command failed");
										 cfgdebug=strtoul($2,NULL,16);
								 	 	 DP2(CMDYACCFLUFF,"cfgdebug now %d\n",cfgdebug);
										 Dflag++;  /* make debugging persistant */
										 if (!cfghasdebug) {
											error(NOTBCFG,
												"cmd debugging not available");
										 }
										 free($2);  /* HEXNUM malloced with strdup */
										}
	;

loadfilecmd: LOADFILE WORD error	{
PrimeErrorBuffer(4,"the loadfile command which loads a bcfg file has failed");
										 error(NOTBCFG,
										 "loadfile requires a single filename argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| LOADFILE '\n'				{
PrimeErrorBuffer(4,"the loadfile command which loads a bcfg file has failed");
										 error(NOTBCFG,
										 "loadfile requires a single filename argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| LOADFILE WORD '\n'			{
PrimeErrorBuffer(4,"the loadfile command which loads a bcfg file has failed");
										 fflag=1;   /* for zzwrap: one file mode */
										 if (read1bcfg($2)) {
         							 	fatal(
									"Fatal: Severe Uncaught Error While Parsing %s",
												$2);
         									/* NOTREACHED */
      								 }
										 fflag=0;   /* allow later loaddir cmds */
										 free($2);  /* WORD malloced with strdup */
										}


loaddircmd: LOADDIR WORD error		{
PrimeErrorBuffer(5,"the loaddir cmd which loads many bcfg files has failed");
										 error(NOTBCFG,
						"loaddir requires a single directory hierarchy argument");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
										}
	| LOADDIR '\n'					{
PrimeErrorBuffer(5,"the loaddir cmd which loads many bcfg files has failed");
										 error(NOTBCFG,
						"loaddir requires a single directory hierarchy argument");
										yyclearin;  /* discard lookahead */
										yyerrok;
										}
	| LOADDIR WORD	'\n'			{
PrimeErrorBuffer(5,"the loaddir cmd which loads many bcfg files has failed");
										 LoadDirHierarchy($2);
										 free($2);  /* WORD malloced with strdup */
										}
	;

tclcmd: TCL WORD error			{
PrimeErrorBuffer(6,"the tcl command has failed");
										 error(NOTBCFG,
                               "tcl requires a numeric argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| TCL '\n'						{
PrimeErrorBuffer(6,"the tcl command has failed");
										 error(NOTBCFG,
                               "tcl requires a numeric argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| TCL WORD '\n'				{/* shouldn't get here since WORD only passed
										  * up in <INITIAL> state and we go into 
										  * <TCLSTATE> state.
										  */
PrimeErrorBuffer(6,"the tcl command has failed");
										 error(NOTBCFG,
                               "tcl requires a numeric argument(2)");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 free($2);  /* WORD malloced with strdup */
										}
	| TCL DECNUM '\n'				{
PrimeErrorBuffer(6,"the tcl command has failed");
										 tclmode=strtoul($2,NULL,10);
								 	 	 DP2(CMDYACCFLUFF,"tclmode now %d\n",tclmode);
										 free($2);  /* DECNUM malloced with strdup */
										}

locationcmd: LOCATION '\n'    {
										 int loop;
										 char index[10],version[10],fulldsp[10];
PrimeErrorBuffer(50,"the location command has failed");
										 StartList(8, "INDEX",6,
														  "BCFGVERSION",12,
                                            "FullDSP",8,
                                            "LOCATION",53);
										 for (loop=0;loop<bcfgfileindex;loop++) {
										    snprintf(index,10,"%d",loop);
											 snprintf(version,10,"%d",
														 bcfgfile[loop].version);
											 snprintf(fulldsp,10,"%d",
 												 		 bcfgfile[loop].fullDSPavail);
											 AddToList(4,index,version,fulldsp,
  														    bcfgfile[loop].location);
										 }
										 EndList();
										}
	| LOCATION error				{
PrimeErrorBuffer(50,"the location command has failed");
										 error(NOTBCFG,
											"no argument with location command");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}

drivercmd: DRIVERCMD '\n'		{int loop;
                               char num[10];
                               char *driver_name;

PrimeErrorBuffer(7,"the driver command has failed");
                               StartList(4,"BCFGINDEX",10,"DRIVER_NAME",10);
										 for (loop=0;loop<bcfgfileindex;loop++) {
                                  snprintf(num,10,"%d",loop);
                                  driver_name=
                                      StringListPrint(loop,N_DRIVER_NAME);
                                  AddToList(2,num,driver_name);
                                  if (driver_name != NULL) free(driver_name);
   									 }
										}
	| DRIVERCMD error				{
PrimeErrorBuffer(7,"the driver command has failed");
										 error(NOTBCFG,
											"no argument with driver command");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}

countcmd: COUNT '\n'				{char num[10];
PrimeErrorBuffer(8,"the count command has failed");
                               StartList(2,"BCFGCOUNT",10);
                               snprintf(num,10,"%d",bcfgfileindex);
                               AddToList(1,num);
                               EndList();
                              }
	| COUNT error					{
PrimeErrorBuffer(8,"the count command has failed");
										 error(NOTBCFG,
												"no argument with count command");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

showindexcmd: SHOWINDEX WORD error	{
PrimeErrorBuffer(9,"the showindex command has failed");
										 error(NOTBCFG,
											"showindex requires an argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| SHOWINDEX '\n'				{
PrimeErrorBuffer(9,"the showindex command has failed");
										 error(NOTBCFG,
											"showindex requires an argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWINDEX WORD '\n'		{
										 extern void cmdshowindex(char *);
PrimeErrorBuffer(9,"the showindex command has failed");
										 cmdshowindex($2);  /* pass in driver name */
										 free($2);  /* WORD malloced with strdup */
										}
	;

showbuscmd: SHOWBUS WORD error		{
PrimeErrorBuffer(10,"the showbus command has failed");
										 error(NOTBCFG,
											"showbus requires an argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| SHOWBUS '\n'					{
PrimeErrorBuffer(10,"the showbus command has failed");
										 error(NOTBCFG,
											"showbus requires an argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWBUS WORD	'\n'			{
										 int index;
                               char *bus;

/* XXX block signals here */
PrimeErrorBuffer(10,"the showbus command has failed");
										 errno=0;
										 index=strtoul($2,NULL,10);
										 if (errno) {
                                  error(NOTBCFG,"invalid index %s",$2);
										 } else {
 											if (index >= bcfgfileindex) {
    											error(NOTBCFG,
													"highest index allowed is %d",
														bcfgfileindex - 1);
 											} else {
 												bus=StringListPrint(index,N_BUS);
                                    if (bus == NULL) {
                                       error(NOTBCFG,
                                             "BUS not set for index %d",index);
                                    } else {
                                       StartList(2,"BUS",10);
													AddToList(1,bus);
                                       free(bus); /* since bus != NULL */
                                       EndList();
                                    }
											}
										 }
										 free($2);  /* WORD malloced with strdup */
										}

showdrivercmd: SHOWDRIVER WORD error	{
PrimeErrorBuffer(11,"the showdriver command has failed");
										 error(NOTBCFG,
												"showdriver requires an argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| SHOWDRIVER '\n'				{
PrimeErrorBuffer(11,"the showdriver command has failed");
										 error(NOTBCFG,
												"showdriver requires an argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWDRIVER WORD	'\n'		{int index;
										 char *driver_name;

/* XXX block signals here */
PrimeErrorBuffer(11,"the showdriver command has failed");
										 errno=0;
										 index=strtoul($2,NULL,10);
										 if (errno) {
                                  error(NOTBCFG,"invalid index %s",$2);
										 } else {
 											if (index >= bcfgfileindex) {
    											error(NOTBCFG,
													"highest index allowed is %d",
														bcfgfileindex - 1);
 											} else {
 												driver_name=
                                       StringListPrint(index,N_DRIVER_NAME);
                                    if (driver_name == NULL) {
                                       error(NOTBCFG,
                                           "DRIVER_NAME not set for index %d",
                                           index);
                                    } else {
                                       StartList(2,"DRIVER_NAME",20);
													AddToList(1,driver_name);
                                       free(driver_name); /* since != NULL */
                                       EndList();
                                    }
											}
										 }
										 free($2);  /* WORD malloced with strdup */
										}

showfailcmd: SHOWFAILOVER WORD error	{
PrimeErrorBuffer(12,"the showfailover command has failed");
										 error(NOTBCFG,
								"showfailover requires a single topology argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| SHOWFAILOVER '\n'			{
PrimeErrorBuffer(12,"the showfailover command has failed");
										 error(NOTBCFG,
								"showfailover requires a single topology argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWFAILOVER WORD	'\n'	{
PrimeErrorBuffer(12,"the showfailover command has failed");
										 error(NOTBCFG,
											"showfailover cmd not implemented yet");
/*
show all drivers indexes which
1) has failover=true AND
2) match topology argument (remember TOPOLOGY can be multivalued in bcfg) AND
3) have a valid BUS relative to what we're running on
*/
										 free($2);  /* WORD malloced with strdup */
										}
	;

showrejectscmd: SHOWREJECTS '\n' {
PrimeErrorBuffer(13,"the showrejects command has failed");
										 showrejects();
										}
	| SHOWREJECTS error			{
PrimeErrorBuffer(13,"the showrejects command has failed");
										 error(NOTBCFG,
										 "showrejects doesn't have any arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;
shownamecmd: SHOWNAME WORD error	{
PrimeErrorBuffer(14,"the showname command has failed");
										 error(NOTBCFG,
												"showname requires an index argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| SHOWNAME '\n'				{
PrimeErrorBuffer(14,"the showname command has failed");
										 error(NOTBCFG,
												"showname requires an index argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWNAME WORD '\n'			{int index;
                               char *name;

/* XXX block signals here */
PrimeErrorBuffer(14,"the showname command has failed");
										 errno=0;
										 index=strtoul($2,NULL,10);
										 if (errno) {
                                  error(NOTBCFG,"invalid index %s",$2);
										 } else {
 											if (index >= bcfgfileindex) {
    											error(NOTBCFG,
													"highest index allowed is %d",
														bcfgfileindex - 1);
 											} else {
 												name=StringListPrint(index,N_NAME);
                                    if (name == NULL) {
                                       error(NOTBCFG,
                                             "NAME not set for index %d",index);
                                    } else {
                                       StartList(2,"NAME",10);
													AddToList(1,name);
                                       free(name); /* since name != NULL */
                                       EndList();
                                    }
											}
										 }
										 free($2);  /* WORD malloced with strdup */
										}

dumpcmd: RESDUMP '\n'			{
PrimeErrorBuffer(15,"the resdump command has failed");
										 if (resdump(NULL)) {
											error(NOTBCFG,"resdump command failed");
										 }
										}
	| RESDUMP WORD '\n'			{
PrimeErrorBuffer(15,"the resdump command has failed");
										 if (resdump($2)) {
											error(NOTBCFG,"resdump command failed");
                               }
										 free($2);  /* WORD malloced with strdup */
										}
	| RESDUMP error				{
PrimeErrorBuffer(15,"the resdump command has failed");
										 error(NOTBCFG,
												"resdump has 0 or 1(the key) arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

helpcmd: HELP '\n'				{int loop,section,count;
PrimeErrorBuffer(15,"the help command has failed");
										 Pstdout("type \"help cmd_name\" to see more "
                                      "help on the following commands:");
									for (section=0;section<SEC_MAXSECTIONS;section++) {
											 if (section == SEC_GENERAL) 
                                     Pstdout("\nGENERAL COMMANDS:\n");
											 if (section == SEC_SHOW) 
                                     Pstdout("\nSHOW-ME COMMANDS:\n");
                                  if (section == SEC_FILE)
                                     Pstdout("\nFILE-RELATED COMMANDS:\n");
                                  if (section == SEC_RESMGR)
                                     Pstdout("\nRESMGR-RELATED COMMANDS:\n");
                                  if (section == SEC_CA)
                                     Pstdout("\nCA-RELATED COMMANDS:\n");
                                  if (section == SEC_ELF)
                                     Pstdout("\nELF-RELATED COMMANDS:\n");
                                  if (section == SEC_BCFG)
                                     Pstdout("\nBCFG-RELATED COMMANDS:\n");
                                  if (section == SEC_HPSL)
                                     Pstdout("\nHPSL-RELATED COMMANDS:\n");
											 count=0;
										    for (loop=0;help[loop].cmd;loop++) {
                                     if (help[loop].section == section) {
													Pstdout("%s ",help[loop].cmd);
													count++;
                                       if (count && count % 7 == 0) {
														Pstdout("\n");
													}
												 }
										    }
										 }
										 Pstdout("\n");
										}
	| HELP WORD	'\n'				{int loop,found;
PrimeErrorBuffer(15,"the help command has failed");
										 found=0;
										 strtolower($2);
										 for (loop=0;help[loop].cmd;loop++) {
										    if (!strcmp($2,help[loop].cmd)) {
											    Pstdout("%s\n",help[loop].description);
												 found++;
											 }
										 }
										 if (!found) {
										    Pstdout("no help available on %s\n",$2);
										 }
										 free($2);  /* WORD malloced with strdup */
										}
	| HELP WORD error				{error(NOTBCFG,"help has 0 or 1 parameters");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

quietcmd: QUIET '\n'				{
PrimeErrorBuffer(16,"the quiet command has failed");
										 bequiet=1;
										 Pstdout("now in quiet mode\n");
										}
	| QUIET error					{
PrimeErrorBuffer(16,"the quiet command has failed");
										 error(NOTBCFG,"quiet doesn't have args");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

verbosecmd: VERBOSE '\n'      {
PrimeErrorBuffer(17,"the verbose command has failed");
										 bequiet=0;
										 Pstdout("now in verbose mode\n");
										}
	| VERBOSE error				{
PrimeErrorBuffer(17,"the verbose command has failed");
										 error(NOTBCFG,"verbose doesn't have args");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

morecmd: MORE WORD error 		{
PrimeErrorBuffer(18,"the more command has failed");
										 error(NOTBCFG,"more has on or off argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| MORE '\n'						{
PrimeErrorBuffer(18,"the more command has failed");
										 error(NOTBCFG,"more has on or off argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| MORE WORD	'\n'				{
PrimeErrorBuffer(18,"the more command has failed");
										 candomore=1;
										 moremode=0;
										 if (!strcmp($2,"on")) {
											 get_terminal_info();  
											 /* get_terminal_info will set lines.  
											  * it can also write to stderr, generally
											  * bad for tcl mode.
											  * it can also reset candomore back to 0 
											  */
										    if (candomore) moremode=1;
										 }
										 Pstdout("moremode is %s\n",moremode?"on":"off");
										 free($2);  /* WORD malloced with strdup */
										}
	;

showvariablecmd: SHOWVARIABLE WORD WORD error {
PrimeErrorBuffer(19,"the showvariable command has failed");
												 error(NOTBCFG,
                "showvariable has bcfgindex and variable as parameters (0)");
										 		 yyclearin;  /* discard lookahead */
										 		 yyerrok;
										 		CmdFreeWords();
											   }
	| SHOWVARIABLE '\n'					{
PrimeErrorBuffer(19,"the showvariable command has failed");
												 error(NOTBCFG,
                "showvariable has bcfgindex and variable as parameters (0)");
										 		 yyclearin;  /* discard lookahead */
										 		 yyerrok;
												}
	| SHOWVARIABLE WORD '\n'			{
PrimeErrorBuffer(19,"the showvariable command has failed");
												error(NOTBCFG,
                "showvariable has bcfgindex and variable as parameters (1)");
										 		yyclearin;  /* discard lookahead */
										 		yyerrok;
										      free($2);  /* WORD malloced with strdup */
										}
	| SHOWVARIABLE WORD WORD '\n'	{
										 int index,offset;
										 char *text;

/* XXX block signals here */
PrimeErrorBuffer(19,"the showvariable command has failed");
										 errno=0;
										 index=strtoul($2,NULL,10);
										 if (errno) {
                                  error(NOTBCFG,"invalid index %s",$1);
										 } else {
 											if (index >= bcfgfileindex) {
    											error(NOTBCFG,
													"highest index allowed is %d",
														bcfgfileindex - 1);
 											} else {
                                    offset=N2O($3);
                                    if (offset == -1) {
                                       error(NOTBCFG, 
                                          "%s is an unknown variable", $3);
                                    } else {
 												   text=StringListPrint(index,offset);
                                       if (text == NULL) {
                                          error(NOTBCFG,
                                            "%s not set for index %d",$3,index);
                                       } else {
                                          StartList(2,$3,70);
													   AddToList(1,text);
                                          free(text); /* since text != NULL */
                                          EndList();
                                       }
                                    }
											}
										 }
										 free($2);  /* WORD malloced with strdup */
										 free($3);  /* WORD malloced with strdup */
										}
	;

showlinescmd: SHOWLINES WORD WORD error {
PrimeErrorBuffer(56,"the showlines command has failed");
												 error(NOTBCFG,
                "showlines has bcfgindex and variable as parameters (0)");
										 		 yyclearin;  /* discard lookahead */
										 		 yyerrok;
										 		CmdFreeWords();
											   }
	| SHOWLINES '\n'					{
PrimeErrorBuffer(56,"the showlines command has failed");
												 error(NOTBCFG,
                "showlines has bcfgindex and variable as parameters (0)");
										 		 yyclearin;  /* discard lookahead */
										 		 yyerrok;
												}
	| SHOWLINES WORD '\n'			{
PrimeErrorBuffer(56,"the showlines command has failed");
												error(NOTBCFG,
                "showlines has bcfgindex and variable as parameters (1)");
										 		yyclearin;  /* discard lookahead */
										 		yyerrok;
										      free($2);  /* WORD malloced with strdup */
										}
	| SHOWLINES WORD WORD '\n'	{
										 int index,offset,numlines,loop;
										 char *text;

/* XXX block signals here */
PrimeErrorBuffer(56,"the showlines command has failed");
										 errno=0;
										 index=strtoul($2,NULL,10);
										 if (errno) {
                                  error(NOTBCFG,"invalid index %s",$1);
										 } else {
 											if (index >= bcfgfileindex) {
    											error(NOTBCFG,
													"highest index allowed is %d",
														bcfgfileindex - 1);
 											} else {
                                    offset=N2O($3);
                                    if (offset == -1) {
                                       error(NOTBCFG, 
                                          "%s is an unknown variable", $3);
                                    } else {
													numlines=StringListNumLines(index, 
																							offset);
													if (numlines <= 0) {
														error(NOTBCFG,"%s: undefined or "
																			"0 text lines",$3);
													} else {
                                         	StartList(4,"LINEX",6,"CONTENTS",70);
														for (loop=1; loop<=numlines; loop++) {
															char linexstr[10];

															snprintf(linexstr,10,"%d",loop);
 												   		text=StringListLineX(index,
																	offset,loop);
                                       		if (text == NULL) {
                                          		error(NOTBCFG,
                                            			"%s not set for index %d",
																	$3,index);
                                       		} else {
													   		AddToList(2,linexstr,text);
                                          		free(text);
                                       		}
														}
                                          EndList();
                                    	}
												}
										 	}
										}
									 free($2);  /* WORD malloced with strdup */
									 free($3);  /* WORD malloced with strdup */
									}
	;

showalltopologiescmd: SHOWALLTOPOLOGIES '\n' {
PrimeErrorBuffer(20,"the showalltopologies command has failed");
										 error(NOTBCFG,
                                  "showalltopologies has LAN/WAN argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWALLTOPOLOGIES WORD error	{
PrimeErrorBuffer(20,"the showalltopologies command has failed");
										 error(NOTBCFG,
                                  "showalltopologies has LAN/WAN argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| SHOWALLTOPOLOGIES WORD '\n' {
PrimeErrorBuffer(20,"the showalltopologies command has failed");
										 ShowAllTopologies($2);
										 free($2);  /* it was malloced with strdup */
										}
	;

showtopocmd: SHOWTOPO '\n'		{
PrimeErrorBuffer(21,"the showtopo command has failed");
										 error(NOTBCFG,
                                  "showtopo has a topo argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWTOPO WORD error		{
PrimeErrorBuffer(21,"the showtopo command has failed");
										 error(NOTBCFG,
                                  "showtopo has a topo argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| SHOWTOPO WORD '\n'			{
PrimeErrorBuffer(21,"the showtopo command has failed");
										 ShowTopo($2);
										 free($2);  /* WORD malloced with strdup */
										}
	;

resmgrcmd: RESMGR '\n'			{
PrimeErrorBuffer(22,"the resmgr command has failed");
										 if (ResmgrDumpAll() != 0) {
                                  error(NOTBCFG,"Error in ResmgrDumpAll");
                               }
										}
	| RESMGR WORD '\n'			{
PrimeErrorBuffer(22,"the resmgr command has failed");
										 if (ResmgrDumpOne(atoi($2),1) != 0) {
 											 /* dump this key */
											error(NOTBCFG,"Error in ResmgrDumpOne");
                               }
										 free($2);  /* WORD malloced with strdup */
										}
	| RESMGR WORD error			{
PrimeErrorBuffer(22,"the resmgr command has failed");
										 error(NOTBCFG,
										 "resmgr has no or the desired key as argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

resshowunclaimedcmd:	RESSHOWUNCLAIMED '\n'	{
PrimeErrorBuffer(23,"the resshowunclaimed command has failed");
										 if (ResShowUnclaimed(0,NULL,NULL) == -1) {
											error(NOTBCFG,"Error in ResShowUnclaimed");
                               }
										}
	| RESSHOWUNCLAIMED WORD '\n'	{
PrimeErrorBuffer(23,"the resshowunclaimed command has failed");
										 if (ResShowUnclaimed(0,NULL,$2) == -1) {
											error(NOTBCFG,"Error in ResShowUnclaimed");
                               }
											free($2);  /* WORD malloced with strdup */
										}
	| RESSHOWUNCLAIMED WORD error	{
PrimeErrorBuffer(23,"the resshowunclaimed command has failed");
										 error(NOTBCFG,
                                "resshowunclaimed doesn't have any arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

isaautodetectcmd: ISAAUTODETECT ManyWords '\n' {
PrimeErrorBuffer(24,"the isaautodetect command has failed");
											if (isaautodetect(0,9999999,RM_KEY,$2)==-1) {
												error(NOTBCFG,"isaautodetect failed");
											}
										   /* Free up space used by primitive */
										   PrimitiveFreeMem(&$2);
										}
	| ISAAUTODETECT '\n'			{
PrimeErrorBuffer(24,"the isaautodetect command has failed");
										 error(NOTBCFG,
											"ISAautodetect has at least 2 args");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| ISAAUTODETECT error		{
PrimeErrorBuffer(24,"the isaautodetect command has failed");
										 error(NOTBCFG,
											"ISAautodetect has at least 2 args");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

getisaparamscmd: GETISAPARAMS WORD '\n' {
PrimeErrorBuffer(25,"the getisaparams command has failed");
											if (getisaparams($2, 0) == -1) {
												error(NOTBCFG,"getisaparams returned -1");
                                 }
										   free($2);  /* WORD malloced with strdup */
										}
	| GETISAPARAMS '\n'			{
PrimeErrorBuffer(25,"the getisaparams command has failed");
										 error(NOTBCFG,
											"getISAparams has bcfgindex arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| GETISAPARAMS WORD error	{
PrimeErrorBuffer(25,"the getisaparams command has failed");
										 error(NOTBCFG,
											"getISAparams has bcfgindex arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

getallisaparamscmd: GETALLISAPARAMS WORD '\n' {
PrimeErrorBuffer(61,"the getallisaparams command has failed");
											if (getisaparams($2, 1) == -1) {
												error(NOTBCFG,"getallisaparams returns -1");
                                 }
										   free($2);  /* WORD malloced with strdup */
										}
	| GETALLISAPARAMS '\n'		{
PrimeErrorBuffer(61,"the getallisaparams command has failed");
										 error(NOTBCFG,
											"getallISAparams has element arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| GETALLISAPARAMS WORD error	{
PrimeErrorBuffer(61,"the getallisaparams command has failed");
										 error(NOTBCFG,
											"getallISAparams has element arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

showcustomnumcmd: SHOWCUSTOMNUM WORD '\n' {
PrimeErrorBuffer(26,"the showcustomnum command has failed");
											(void) showcustomnum(1,$2);
										   free($2);  /* WORD malloced with strdup */
										}
	| SHOWCUSTOMNUM '\n'			{
PrimeErrorBuffer(26,"the showcustomnum command has failed");
										 error(NOTBCFG,
											"showcustomnum has bcfgindex arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWCUSTOMNUM WORD error	{
PrimeErrorBuffer(26,"the showcustomnum command has failed");
										 error(NOTBCFG,
											"showcustomnum has bcfgindex arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

showcustomcmd: SHOWCUSTOM WORD WORD '\n' {
PrimeErrorBuffer(27,"the showcustom command has failed");
											showcustom($2,$3);
										   free($2);  /* WORD malloced with strdup */
										   free($3);  /* WORD malloced with strdup */
										}
	| SHOWCUSTOM WORD '\n' 		{
PrimeErrorBuffer(27,"the showcustom command has failed");
										 error(NOTBCFG,"showcustom has 2 arguments");
										 free($2);  /* it was malloced with strdup */
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWCUSTOM '\n'				{
PrimeErrorBuffer(27,"the showcustom command has failed");
										 error(NOTBCFG,"showcustom has 2 arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| SHOWCUSTOM WORD WORD error		{
PrimeErrorBuffer(27,"the showcustom command has failed");
										 error(NOTBCFG,"showcustom has 2 arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

idinstallcmd: IDINSTALL ManyWords '\n'	{
PrimeErrorBuffer(28,"the idinstall command has failed");
										/* stringlist_t *slp;
										 * union primitives *sl;
                               *
										 * slp=&$2.stringlist;
										 * Pstdout("in idinstall with arguments:\n");
										 * while (slp != (struct stringlist *)NULL) {
										 *	Pstdout("stringlist: %s\n",slp->string);
										 *	slp=slp->next;
										 *}
										 */
										(void) idinstall($2);
										/* Free up space used by primitive */
										PrimitiveFreeMem(&$2);
									}
	| IDINSTALL '\n'			{
PrimeErrorBuffer(28,"the idinstall command has failed");
										error(NOTBCFG,"idinstall has at least 4 args");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| IDINSTALL error 		{
PrimeErrorBuffer(28,"the idinstall command has failed");
										error(NOTBCFG,"idinstall has at least 4 args");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

ManyWords:  WORD				{
									 DP1(CMDYACCFLUFF,"in_ManyWords_got_WORD ");
									 $$.stringlist.type=STRINGLIST;
									 $$.stringlist.uma=NULL;
									 $$.stringlist.string=$1;
									 $$.stringlist.next=(struct stringlist *)NULL;
									}
	| ManyWords WORD 			{
									 union primitives *sl;
									 struct stringlist *walk;
									 DP1(CMDYACCFLUFF,"in_ManyWords_got_ManyWords_WORD ");
									 /* ok do real work here */
									 sl=(union primitives *) malloc(sizeof(union primitives));
									 if (!sl) {
 										 fatal("couldn't malloc for stringlist");
									 }
								    sl->stringlist.type = STRINGLIST;
								    sl->stringlist.next = (struct stringlist *)NULL;
									 sl->stringlist.uma = sl; /* so we can free later */
									 sl->stringlist.string = $2;
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

execcmd: EXECCMD ManyWords '\n' {
										stringlist_t *slp;
										union primitives *sl;
										char cmd[SIZE];
                              
PrimeErrorBuffer(29,"the ! command has failed");
										cmd[0]='\0';
										slp=&$2.stringlist;
										/* Pstdout("in execcmd with arguments:\n"); */
										while (slp != (struct stringlist *)NULL) {
											/* Pstdout("stringlist: %s\n",slp->string); */
											strcat(cmd,slp->string);
											if (slp->next != NULL) strcat(cmd," ");
											slp=slp->next;
										}
										/* Pstdout("about to run cmd '%s'\n",cmd); */
										system(cmd);
										/* Free up space used by primitive */
										PrimitiveFreeMem(&$2);
									}
	| EXECCMD '\n'				{
PrimeErrorBuffer(29,"the ! command has failed");
										/* nothing to do */
									}
	| EXECCMD error 			{
PrimeErrorBuffer(29,"the ! command has failed");
										error(NOTBCFG,"Usage error with exec cmd");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

resgetcmd: RESGET '\n'		{
PrimeErrorBuffer(30,"the resget command has failed");
										error(NOTBCFG,"resget has 2 parameters");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| RESGET WORD '\n'		{
PrimeErrorBuffer(30,"the resget command has failed");
										error(NOTBCFG,"resget has 2 parameters");
										yyclearin;  /* discard lookahead */
										yyerrok;
										free($2);  /* WORD malloced with strdup */
									}
	| RESGET WORD WORD '\n'	{
PrimeErrorBuffer(30,"the resget command has failed");
										if (resget($2,$3) != 0) {
											error(NOTBCFG,"Error in resget");
     									}
										free($2);
										free($3);
									}
	| RESGET WORD WORD error		{
PrimeErrorBuffer(30,"the resget command has failed");
										error(NOTBCFG,"resget has 2 parameters");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

resputcmd: RESPUT '\n'		{
PrimeErrorBuffer(31,"the resput command has failed");
										error(NOTBCFG,"resput has 2 parameters");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| RESPUT WORD '\n'		{
PrimeErrorBuffer(31,"the resput command has failed");
										error(NOTBCFG,"resput has 2 parameters");
										yyclearin;  /* discard lookahead */
										yyerrok;
										free($2);  /* WORD malloced with strdup */
									}
	| RESPUT WORD WORD '\n'	{
PrimeErrorBuffer(31,"the resput command has failed");
										if (resput($2,$3,1)) {
											error(NOTBCFG,"error in resput");
										}
										free($2);  /* it was malloced with strdup */
										free($3);  /* it was malloced with strdup */
									}
	| RESPUT WORD WORD error {
PrimeErrorBuffer(31,"the resput command has failed");
										error(NOTBCFG,"resput has 2 parameters");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									 }
	;

showserialttyscmd: SHOWSERIALTTYS '\n'	{
PrimeErrorBuffer(32,"the showserialttys command has failed");
										showserialttys();
									}
	| SHOWSERIALTTYS error	{
PrimeErrorBuffer(32,"the showserialttys command has failed");
										error(NOTBCFG,"showserialttys doesn't have args");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;


bcfgpathtoindexcmd: BCFGPATHTOINDEX '\n' {
PrimeErrorBuffer(33,"the bcfgpathtoindex command has failed");
										error(NOTBCFG,"bcfgpathtoindex has a path "
                                    "argument.");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| BCFGPATHTOINDEX WORD '\n'	{
PrimeErrorBuffer(33,"the bcfgpathtoindex command has failed");
										bcfgpathtoindex(1,$2);
										free($2);  /* it was malloced with strdup */
									}
	| BCFGPATHTOINDEX error {
PrimeErrorBuffer(33,"the bcfgpathtoindex command has failed");
										error(NOTBCFG,"bcfgpathtoindex has a path "
                                    "argument.");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

resshowkeycmd: RESSHOWKEY '\n'	{
PrimeErrorBuffer(34,"the resshowkey command has failed");
										error(NOTBCFG,"resshowkey has an NETCFG_ELEMENT "
                                    "argument.");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| RESSHOWKEY WORD '\n'	{
PrimeErrorBuffer(34,"the resshowkey command has failed");
										/* 1 means do StartList, AddToList, and EndList */
										/* note we don't check at backup key here */
										if (resshowkey(1, $2, NULL, 0) == -1) {
											error(NOTBCFG,"Error in resshowkey");
										}
										free($2);  /* it was malloced with strdup */
									}
	| RESSHOWKEY error		{
PrimeErrorBuffer(34,"the resshowkey command has failed");
										error(NOTBCFG,"resshowkey has an NETCFG_ELEMENT "
                                    "argument.");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

showisacurrentcmd: SHOWISACURRENT '\n' {
PrimeErrorBuffer(35,"the showisacurrent command has failed");
										error(NOTBCFG,"showISAcurrent has "
                                            "an NETCFG_ELEMENT argument.");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| SHOWISACURRENT WORD '\n'	{
PrimeErrorBuffer(35,"the showisacurrent command has failed");
										if (showISAcurrent($2,1,NULL,NULL,NULL,NULL)!=0){
											error(NOTBCFG,"Error in showISAcurrent");
										}
										free($2);  /* it was malloced with strdup */
									}
	| SHOWISACURRENT error	{
PrimeErrorBuffer(35,"the showisacurrent command has failed");
										error(NOTBCFG,"showISAcurrent has "
                                            "an NETCFG_ELEMENT argument.");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

showcustomcurrentcmd: SHOWCUSTOMCURRENT '\n' {
PrimeErrorBuffer(36,"the showcustomcurrent command has failed");
										error(NOTBCFG,"showCUSTOMcurrent has "
                                            "an NETCFG_ELEMENT argument.");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| SHOWCUSTOMCURRENT WORD '\n' {
PrimeErrorBuffer(36,"the showcustomcurrent command has failed");
										if (showCUSTOMcurrent($2) != 0) {
											error(NOTBCFG,"Error in showCUSTOMcurrent");
										}
										free($2);  /* it was malloced with strdup */
									}
	| SHOWCUSTOMCURRENT error {
PrimeErrorBuffer(36,"the showcustomcurrent command has failed");
										error(NOTBCFG,"showCUSTOMcurrent has "
                                            "an NETCFG_ELEMENT argument.");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

idremovecmd: IDREMOVE '\n' {
PrimeErrorBuffer(37,"the idremove command has failed");
										error(NOTBCFG,"idremove has 2 arguments");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| IDREMOVE WORD '\n'		{
PrimeErrorBuffer(37,"the idremove command has failed");
										error(NOTBCFG,"idremove has 2 arguments");
										yyclearin;  /* discard lookahead */
										yyerrok;
										free($2);  /* it was malloced with strdup */
									}
	| IDREMOVE WORD WORD '\n'	{
PrimeErrorBuffer(37,"the idremove command has failed");
										idremove($2,$3);
										free($2);  /* it was malloced with strdup */
										free($3);  /* it was malloced with strdup */
									}
	| IDREMOVE WORD WORD error	{
PrimeErrorBuffer(37,"the idremove command has failed");
										error(NOTBCFG,"idremove has 2 arguments");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
										}
	;

idmodifycmd: IDMODIFY ManyWords '\n'	{
PrimeErrorBuffer(38,"the idmodify command has failed");
										/* stringlist_t *slp;
										 * union primitives *sl;
                               *
										 * slp=&$2.stringlist;
										 * Pstdout("in idmodify with arguments:\n");
										 * while (slp != (struct stringlist *)NULL) {
										 *	Pstdout("stringlist: %s\n",slp->string);
										 *	slp=slp->next;
										 *}
										 */
										(void) idmodify($2);
										/* Free up space used by primitive */
										PrimitiveFreeMem(&$2);
									}
	| IDMODIFY '\n'			{
PrimeErrorBuffer(38,"the idmodify command has failed");
										error(NOTBCFG,"idmodify has at least 2 args");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| IDMODIFY error 		{
PrimeErrorBuffer(38,"the idmodify command has failed");
										error(NOTBCFG,"idmodify has at least 2 args");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

showhelpfilecmd: SHOWHELPFILE '\n'	{
PrimeErrorBuffer(39,"the showhelpfile command has failed");
													error(NOTBCFG,"showhelpfile has 1 arg");
													yyclearin;  /* discard lookahead */
													yyerrok;
												}
	| SHOWHELPFILE WORD '\n'	{
PrimeErrorBuffer(39,"the showhelpfile command has failed");
										if (showhelpfile($2) != 0) {
											error(NOTBCFG,"error in showhelpfile");
										}
										free($2);  /* it was malloced with strdup */
									}
	| SHOWHELPFILE WORD error	{
PrimeErrorBuffer(39,"the showhelpfile command has failed");
												error(NOTBCFG,"showhelpfile has 1 arg");
												yyclearin;  /* discard lookahead */
												yyerrok;
										 CmdFreeWords();
										}
	;

cacmd: pcicmd
	| eisacmd
	| mcacmd
	| itwocmd
	;

pcicmd: PCISHORT '\n'		{
PrimeErrorBuffer(40,"the pcishort command has failed");
										if (ResDumpCA(NULL, 0, CM_BUS_PCI) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
									}
	|   PCISHORT WORD	'\n'	{
PrimeErrorBuffer(40,"the pcishort command has failed");
										if (ResDumpCA($2, 0, CM_BUS_PCI) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
										free($2);
									}
	|   PCISHORT WORD error	{
PrimeErrorBuffer(40,"the pcishort command has failed");
										error(NOTBCFG,"0 or 1 argument to pcishort cmd");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	|	PCILONG '\n'			{
PrimeErrorBuffer(41,"the pcilong command has failed");
										if (ResDumpCA(NULL, 1, CM_BUS_PCI) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
									}
	|  PCILONG WORD '\n'		{
PrimeErrorBuffer(41,"the pcilong command has failed");
										if (ResDumpCA($2, 1, CM_BUS_PCI) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
										free($2);
									}
	|  PCILONG WORD error	{
PrimeErrorBuffer(41,"the pcilong command has failed");
										error(NOTBCFG,"0 or 1 argument to pcilong cmd");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

eisacmd:	EISASHORT '\n'		{
PrimeErrorBuffer(42,"the eisashort command has failed");
										if (ResDumpCA(NULL, 0, CM_BUS_EISA) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
									}
	| EISASHORT WORD '\n'	{
PrimeErrorBuffer(42,"the eisashort command has failed");
										if (ResDumpCA($2, 0, CM_BUS_EISA) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
										free($2);
									}
	| EISASHORT WORD error	{
PrimeErrorBuffer(42,"the eisashort command has failed");
										error(NOTBCFG,"0 or 1 argument to eisashort cmd");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	| EISALONG '\n'			{
PrimeErrorBuffer(43,"the eisalong command has failed");
										if (ResDumpCA(NULL, 1, CM_BUS_EISA) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
									}
	| EISALONG WORD '\n'		{
PrimeErrorBuffer(43,"the eisalong command has failed");
										if (ResDumpCA($2, 1, CM_BUS_EISA) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
										free($2);
									}
	| EISALONG WORD error	{
PrimeErrorBuffer(43,"the eisalong command has failed");
										error(NOTBCFG,"0 or 1 argument to eisalong cmd");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

itwocmd:	I2OSHORT '\n'		{
PrimeErrorBuffer(76,"the i2oshort command has failed");
#ifdef CM_BUS_I2O
										if (ResDumpCA(NULL, 0, CM_BUS_I2O) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
#else
										error(NOTBCFG,"sorry, no i2oshort command");
#endif
									}
	| I2OSHORT WORD '\n'	{
PrimeErrorBuffer(76,"the i2oshort command has failed");
#ifdef CM_BUS_I2O
										if (ResDumpCA($2, 0, CM_BUS_I2O) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
#else
										error(NOTBCFG,"sorry, no i2oshort command");
#endif
										free($2);
									}
	| I2OSHORT WORD error	{
PrimeErrorBuffer(76,"the i2oshort command has failed");
										error(NOTBCFG,"0 or 1 argument to i2oshort cmd");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	| I2OLONG '\n'			{
PrimeErrorBuffer(77,"the i2olong command has failed");
#ifdef CM_BUS_I2O
										if (ResDumpCA(NULL, 1, CM_BUS_I2O) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
#else
										error(NOTBCFG,"sorry, no i2olong command");
#endif
									}
	| I2OLONG WORD '\n'		{
PrimeErrorBuffer(77,"the i2olong command has failed");
#ifdef CM_BUS_I2O
										if (ResDumpCA($2, 1, CM_BUS_I2O) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
#else
										error(NOTBCFG,"sorry, no i2olong command");
#endif
										free($2);
									}
	| I2OLONG WORD error	{
PrimeErrorBuffer(77,"the i2olong command has failed");
										error(NOTBCFG,"0 or 1 argument to i2olong cmd");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

mcacmd: MCASHORT '\n'		{
PrimeErrorBuffer(44,"the mcashort command has failed");
										if (ResDumpCA(NULL, 0, CM_BUS_MCA) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
									}
	| MCASHORT WORD '\n'		{
PrimeErrorBuffer(44,"the mcashort command has failed");
										if (ResDumpCA($2, 0, CM_BUS_MCA) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
										free($2);
									}
	| MCASHORT WORD error	{
PrimeErrorBuffer(44,"the mcashort command has failed");
										error(NOTBCFG,"0 or 1 argument to mcashort cmd");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	| MCALONG '\n'				{
PrimeErrorBuffer(45,"the mcalong command has failed");
										if (ResDumpCA(NULL, 1, CM_BUS_MCA) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
									}
	| MCALONG WORD '\n'		{
PrimeErrorBuffer(45,"the mcalong command has failed");
										if (ResDumpCA($2, 1, CM_BUS_MCA) != 0) {
											error(NOTBCFG,"problem in ResDumpCA");
										};
										free($2);
									}
	| MCALONG WORD error		{
PrimeErrorBuffer(45,"the mcalong command has failed");
										error(NOTBCFG,"0 or 1 argument to mcalong cmd");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

getstampcmd: GETSTAMP '\n'	{
PrimeErrorBuffer(46,"the getstamp command has failed");
										error(NOTBCFG,"getstamp requires filename arg");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| GETSTAMP WORD '\n'		{
										int wherefound;

PrimeErrorBuffer(46,"the getstamp command has failed");
										if (getstamp(1,$2,NULL,0,NOTBCFG,&wherefound) == 
										-1) {
											error(NOTBCFG,"error in getstamp");
										}
										free($2);
									}
	| GETSTAMP error 			{
PrimeErrorBuffer(46,"the getstamp command has failed");
										error(NOTBCFG,"getstamp requires filename arg");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

stampcmd: STAMP '\n'			{
PrimeErrorBuffer(47,"the stamp command has failed");
										error(NOTBCFG,"stamp has 2 parameters");
										yyclearin;  /* discard lookahead */
										yyerrok;
									}
	| STAMP WORD '\n'			{
PrimeErrorBuffer(47,"the stamp command has failed");
										error(NOTBCFG,"resput has 2 parameters");
										yyclearin;  /* discard lookahead */
										yyerrok;
										free($2);  /* WORD malloced with strdup */
									}
	| STAMP WORD WORD '\n'	{
PrimeErrorBuffer(47,"the stamp command has failed");
										if (stamp($2, $3, NOTBCFG) == -1) {
											error(NOTBCFG,"error in stamp");
										}
										free($2);  /* it was malloced with strdup */
										free($3);  /* it was malloced with strdup */
									}
	| STAMP WORD WORD error	{
PrimeErrorBuffer(47,"the stamp command has failed");
										error(NOTBCFG,"stamp has 2 parameters");
										yyclearin;  /* discard lookahead */
										yyerrok;
										 CmdFreeWords();
									}
	;

versioncmd: VERSION '\n'	{
										 int loop;
										 char index[10],version[10],fulldsp[10];
PrimeErrorBuffer(48,"the version command has failed");
										 StartList(6, "INDEX",6,
                                            "LOCATION",53,
														  "DRIVERVERSION",19);
										 for (loop=0;loop<bcfgfileindex;loop++) {
										    snprintf(index,10,"%d",loop);
											 AddToList(3,index, bcfgfile[loop].location,
                                              bcfgfile[loop].driverversion);
										 }
										 EndList();
									}
	| VERSION error			{
PrimeErrorBuffer(48,"the version command has failed");
										error(NOTBCFG,
                                          "no arguments with version command");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
									}
	;
 
authscmd: AUTHS '\n'			{
PrimeErrorBuffer(49,"the auths command has failed");
										if (printauths() == -1) {
											error(NOTBCFG,"the printauths cmd failed");
										}
									}
	| AUTHS error				{
PrimeErrorBuffer(49,"the auths command has failed");
										error(NOTBCFG,
                                          "no arguments with auths command");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
									}
	;

elementtoindexcmd: ELEMENTTOINDEX '\n'	{
PrimeErrorBuffer(51,"the elementtoindex command has failed");
										error(NOTBCFG,
                                    "elementtoindex command has element arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
									}
	| ELEMENTTOINDEX WORD '\n'	{
PrimeErrorBuffer(51,"the elementtoindex command has failed");
											if (elementtoindex(1,$2) == -1) {
												error(NOTBCFG,"elementtoindex failed");
											}
											free($2);  /* WORD malloced with strdup */
										}
	| ELEMENTTOINDEX error	{
PrimeErrorBuffer(51,"the elementtoindex command has failed");
										error(NOTBCFG,
                                    "elementtoindex command has element arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
									}
	;

unloadallcmd: UNLOADALL '\n' {
PrimeErrorBuffer(53,"the unloadall command has failed");
										(void) unloadall(1);
									}
	| UNLOADALL error		{
PrimeErrorBuffer(53,"the unloadall command has failed");
									error(NOTBCFG,
											"unloadall doesn't have any arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
								}

	;

bcfghasverifycmd: BCFGHASVERIFY '\n' {
PrimeErrorBuffer(54,"the bcfghasverify command has failed");
									error(NOTBCFG,"the bcfghasverify routine has a "
											"bcfgindex parameter");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
								}
	| BCFGHASVERIFY WORD '\n' {
PrimeErrorBuffer(54,"the bcfghasverify command has failed");
									if (bcfghasverify($2) != 0) {
										error(NOTBCFG,"The bcfghasverify routine failed");
									}
									free($2);  /* WORD malloced with strdup */
								}
	| BCFGHASVERIFY error {
PrimeErrorBuffer(54,"the bcfghasverify command has failed");
									error(NOTBCFG,"the bcfghasverify routine has a "
											"bcfgindex parameter");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
								}
	;

dangerousisaautodetectcmd: DANGEROUSISAAUTODETECT ManyWords '\n' {
											int z=dangerousdetect;

											dangerousdetect = 1;
PrimeErrorBuffer(24,"the isaautodetect command has failed");
											if (isaautodetect(0,9999999,RM_KEY,$2)==-1) {
												error(NOTBCFG,"dangerousisaautodetect "
																	"failed");
											}

											dangerousdetect = z;
										   /* Free up space used by primitive */
										   PrimitiveFreeMem(&$2);
										}
	| DANGEROUSISAAUTODETECT '\n'			{
PrimeErrorBuffer(24,"the isaautodetect command has failed");
										 error(NOTBCFG,
											"dangerousISAautodetect has at least 2 args");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| DANGEROUSISAAUTODETECT error		{
PrimeErrorBuffer(24,"the isaautodetect command has failed");
										 error(NOTBCFG,
											"dangerousISAautodetect has at least 2 args");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

clearcmd: CLEAR '\n'				{
PrimeErrorBuffer(55,"the isaautodetect command has failed");
										 ClearTheScreen();
										}
	| CLEAR error					{
PrimeErrorBuffer(55,"the isaautodetect command has failed");
										 error(NOTBCFG,"clear cmd doesn't have any args");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

sysdatcmd: SYSDAT '\n'			{
PrimeErrorBuffer(57,"the sysdat command has failed");
										 sysdat();
										}
	| SYSDAT error					{
PrimeErrorBuffer(57,"the sysdat command has failed");
										 error(NOTBCFG,"sysdat cmd doesn't have any args");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

nlistcmd: NLIST '\n'				{
PrimeErrorBuffer(58,"the nlist command has failed");
										error(NOTBCFG,"nlist has 1 or 2 arguments");
										yyclearin;  /* discard lookahead */
										yyerrok;
										CmdFreeWords();
										}
	| NLIST WORD '\n'				{
PrimeErrorBuffer(58,"the nlist command has failed");
										donlist(0, $2, (unsigned long)-1, 1);
										free($2);  /* WORD malloced with strdup */
										}
	| NLIST WORD DECNUM '\n'	{
PrimeErrorBuffer(58,"the nlist command has failed");
										 donlist(1, $2, strtoul($3,NULL,10), 1);
										 free($2);  /* WORD malloced with strdup */
										 free($3);  /* DECNUM malloced with strdup */
										}
	| NLIST WORD HEXNUM '\n'	{
PrimeErrorBuffer(58,"the nlist command has failed");
										 donlist(1, $2, strtoul($3,NULL,16), 1);
										 free($2);  /* WORD malloced with strdup */
										 free($3);  /* HEXNUM malloced with strdup */
										}
	| NLIST WORD OCTNUM '\n'	{
PrimeErrorBuffer(58,"the nlist command has failed");
										 donlist(1, $2, strtoul($3,NULL,8), 1);
										 free($2);  /* WORD malloced with strdup */
										 free($3);  /* OCTNUM malloced with strdup */
										}
	| NLIST WORD error			{
PrimeErrorBuffer(58,"the nlist command has failed");
										error(NOTBCFG,"nlist has 1 or 2 arguments");
										yyclearin;  /* discard lookahead */
										yyerrok;
										CmdFreeWords();
										}
	;

xidcmd: XID '\n'					{
PrimeErrorBuffer(59,"the xid command has failed");
										 error(NOTBCFG,"xid cmd has 1 argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| XID WORD '\n'				{
PrimeErrorBuffer(59,"the xid command has failed");
											doxid($2);
											free($2);
										}
	| XID WORD error						{
PrimeErrorBuffer(59,"the xid command has failed");
										 error(NOTBCFG,"xid cmd has 1 argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

testcmd: TEST '\n'				{
PrimeErrorBuffer(60,"the test command has failed");
										 error(NOTBCFG,"test cmd has 1 argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| TEST WORD '\n'				{
PrimeErrorBuffer(60,"the test command has failed");
											dotest($2);
											free($2);
										}
	| TEST error					{
PrimeErrorBuffer(60,"the test command has failed");
										 error(NOTBCFG,"test cmd has 1 argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

iicardcmd: IICARD '\n'			{
PrimeErrorBuffer(62,"the iicard command has failed");
										iicard();
										}
	| IICARD error					{
PrimeErrorBuffer(62,"the iicard command has failed");
										 error(NOTBCFG,"iicard doesn't have arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

orphanscmd: ORPHANS '\n'		{
PrimeErrorBuffer(63,"the orphans command has failed");
										orphans();
										}
	| ORPHANS error				{
PrimeErrorBuffer(63,"the orphans command has failed");
										 error(NOTBCFG,"orphans doesn't have arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

pcivendorcmd: PCIVENDOR '\n'	{
PrimeErrorBuffer(64,"the pcivendor command has failed");
										 error(NOTBCFG,"pcivendor has a vendor id arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| PCIVENDOR DECNUM '\n'		{
PrimeErrorBuffer(64,"the pcivendor command has failed");
										 showpcivendor(strtoul($2,NULL,10));
										 free($2);  /* DECNUM malloced with strdup */
										}
	| PCIVENDOR OCTNUM '\n'		{
PrimeErrorBuffer(64,"the pcivendor command has failed");
										 showpcivendor(strtoul($2,NULL,8));
										 free($2);  /* OCTNUM malloced with strdup */
										}
	| PCIVENDOR HEXNUM '\n'		{
PrimeErrorBuffer(64,"the pcivendor command has failed");
										 showpcivendor(strtoul($2,NULL,16));
										 free($2);  /* HEXNUM malloced with strdup */
										}
	| PCIVENDOR error				{
PrimeErrorBuffer(64,"the pcivendor command has failed");
										 error(NOTBCFG,"pcivendor has a vendor id arg");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

promiscuouscmd: PROMISCUOUS '\n'	{
PrimeErrorBuffer(65,"the promiscuous command has failed");
										promiscuous();
										}
	| PROMISCUOUS error			{
PrimeErrorBuffer(65,"the promiscuous command has failed");
										 error(NOTBCFG,"the promiscuous cmd doesn't "
												"have any arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

determinepromcmd: DETERMINEPROM '\n'	{
PrimeErrorBuffer(66,"the determineprom command has faled");
										 error(NOTBCFG,"the determineprom cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| DETERMINEPROM WORD '\n'	{
PrimeErrorBuffer(66,"the determineprom command has failed");
										 determineprom($2);
										 free($2);  /* WORD malloced with strdup */
										}
	| DETERMINEPROM error		{
PrimeErrorBuffer(66,"the determineprom command has element argument");
										 error(NOTBCFG,"the determineprom cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

numwordscmd: NUMWORDS '\n'		{
PrimeErrorBuffer(67,"the numwords command has failed");
												 error(NOTBCFG,
                "numwords has bcfgindex and variable as parameters (0)");
										 		 yyclearin;  /* discard lookahead */
										 		 yyerrok;
										}
	| NUMWORDS WORD '\n'			{
PrimeErrorBuffer(67,"the numwords command has failed");
												error(NOTBCFG,
                "numwords has bcfgindex and variable as parameters (1)");
										 		yyclearin;  /* discard lookahead */
										 		yyerrok;
										      free($2);  /* WORD malloced with strdup */
										}
	| NUMWORDS WORD WORD '\n'	{
										 int index,offset,numwords;
                               char numwordsstr[10];

/* XXX block signals here */
PrimeErrorBuffer(67,"the numwords command has failed");
										 errno=0;
										 index=strtoul($2,NULL,10);
										 if (errno) {
                                  error(NOTBCFG,"invalid index %s",$1);
										 } else {
 											if (index >= bcfgfileindex) {
    											error(NOTBCFG,
													"highest index allowed is %d",
														bcfgfileindex - 1);
 											} else {
                                    offset=N2O($3);
                                    if (offset == -1) {
                                       error(NOTBCFG, 
                                          "%s is an unknown variable", $3);
                                    } else {
 												   numwords=StringListNumWords(index,offset);
                                       if (numwords == -1) {
                                          error(NOTBCFG, "%s is undefined",$3);
                                       } else {
                                          snprintf(numwordsstr,10,"%d",numwords);
                                          StartList(2,$3,70);
													   AddToList(1,numwordsstr);
                                          EndList();
                                       }
                                    }
											}
										 }
										 free($2);  /* WORD malloced with strdup */
										 free($3);  /* WORD malloced with strdup */
										}
	| NUMWORDS WORD WORD error {
PrimeErrorBuffer(67,"the numwords command has failed");
										 error(NOTBCFG,
                		"numwords has bcfgindex and variable as parameters (0)");
								 		 yyclearin;  /* discard lookahead */
								 		 yyerrok;
								 		CmdFreeWords();
									   }

	;

numlinescmd: NUMLINES '\n'		{
PrimeErrorBuffer(68,"the numlines command has failed");
												 error(NOTBCFG,
                "numlines has bcfgindex and variable as parameters (0)");
										 		 yyclearin;  /* discard lookahead */
										 		 yyerrok;
										}
	| NUMLINES WORD '\n'			{
PrimeErrorBuffer(68,"the numlines command has failed");
												error(NOTBCFG,
                "numlines has bcfgindex and variable as parameters (1)");
										 		yyclearin;  /* discard lookahead */
										 		yyerrok;
										      free($2);  /* WORD malloced with strdup */
										}
	| NUMLINES WORD WORD '\n'	{
										 int index,offset,numlines;
                               char numlinesstr[10];

/* XXX block signals here */
PrimeErrorBuffer(68,"the numwords command has failed");
										 errno=0;
										 index=strtoul($2,NULL,10);
										 if (errno) {
                                  error(NOTBCFG,"invalid index %s",$1);
										 } else {
 											if (index >= bcfgfileindex) {
    											error(NOTBCFG,
													"highest index allowed is %d",
														bcfgfileindex - 1);
 											} else {
                                    offset=N2O($3);
                                    if (offset == -1) {
                                       error(NOTBCFG, 
                                          "%s is an unknown variable", $3);
                                    } else {
 												   numlines=StringListNumLines(index,offset);
                                       if (numlines == -1) {
                                          error(NOTBCFG, "%s is undefined",$3);
                                       } else {
                                          snprintf(numlinesstr,10,"%d",numlines);
                                          StartList(2,$3,70);
													   AddToList(1,numlinesstr);
                                          EndList();
                                       }
                                    }
											}
										 }
										 free($2);  /* WORD malloced with strdup */
										 free($3);  /* WORD malloced with strdup */
										}
	| NUMLINES WORD WORD error {
PrimeErrorBuffer(68,"the numlines command has failed");
										 error(NOTBCFG,
                		"numlines has bcfgindex and variable as parameters (0)");
								 		 yyclearin;  /* discard lookahead */
								 		 yyerrok;
								 		CmdFreeWords();
									   }
	;

hpsldumpcmd: HPSLDUMP '\n'		{
PrimeErrorBuffer(69,"the hpsldump command has failed");
										hpsldump();
										}
	| HPSLDUMP error				{
PrimeErrorBuffer(69,"the hpsldump command has failed");
										 error(NOTBCFG,"the hpsldump cmd doesn't "
												"have any arguments");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

hpslsuspendcmd: HPSLSUSPEND '\n' {
PrimeErrorBuffer(70,"the hpslsuspend command has failed");
										 error(NOTBCFG,"the hpslsuspend cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| HPSLSUSPEND WORD '\n'		{	
PrimeErrorBuffer(70,"the hpslsuspend command has failed");
										 hpslsuspend($2);
										 free($2);  /* WORD malloced with strdup */
										}
	| HPSLSUSPEND error			{
PrimeErrorBuffer(70,"the hpslsuspend command has failed");
										 error(NOTBCFG,"the hpslsuspend cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

hpslresumecmd: HPSLRESUME '\n' {
PrimeErrorBuffer(71,"the hpslresume command has failed");
										 error(NOTBCFG,"the hpslresume cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| HPSLRESUME WORD '\n'		{	
PrimeErrorBuffer(71,"the hpslresume command has failed");
										 hpslresume($2);
										 free($2);  /* WORD malloced with strdup */
										}
	| HPSLRESUME error			{
PrimeErrorBuffer(71,"the hpslresume command has failed");
										 error(NOTBCFG,"the hpslresume cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

hpslgetstatecmd: HPSLGETSTATE '\n' {
PrimeErrorBuffer(72,"the hpslgetstate command has failed");
										 error(NOTBCFG,"the hpslgetstate cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| HPSLGETSTATE WORD '\n'	{	
PrimeErrorBuffer(72,"the hpslgetstate command has failed");
										 hpslgetstate($2);
										 free($2);  /* WORD malloced with strdup */
										}
	| HPSLGETSTATE error			{
PrimeErrorBuffer(72,"the hpslgetstate command has failed");
										 error(NOTBCFG,"the hpslgetstate cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

hpslcanhotplugcmd: HPSLCANHOTPLUG '\n' {
PrimeErrorBuffer(73,"the hpslcanhotplug command has failed");
										 error(NOTBCFG,"the hpslcanhotplug cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| HPSLCANHOTPLUG WORD '\n'	{	
PrimeErrorBuffer(73,"the hpslcanhotplug command has failed");
										 hpslcanhotplug($2);
										 free($2);  /* WORD malloced with strdup */
										}
	| HPSLCANHOTPLUG error		{
PrimeErrorBuffer(73,"the hpslcanhotplug command has failed");
										 error(NOTBCFG,"the hpslcanhotplug cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

gethwkeycmd: GETHWKEY '\n' {
PrimeErrorBuffer(74,"the gethwkey command has failed");
										 error(NOTBCFG,"the gethwkey cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	| GETHWKEY WORD '\n'	{	
PrimeErrorBuffer(74,"the gethwkey command has failed");
										 gethwkey($2);
										 free($2);  /* WORD malloced with strdup */
										}
	| GETHWKEY error			{
PrimeErrorBuffer(74,"the gethwkey command has failed");
										 error(NOTBCFG,"the gethwkey cmd has an "
												"element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

dlpimdicmd: DLPIMDI '\n' 		{
PrimeErrorBuffer(78,"the gethwkey command has failed");
										 dlpimdi(NULL);
										}

	| DLPIMDI WORD '\n'			{
PrimeErrorBuffer(78,"the gethwkey command has failed");
										 dlpimdi($2);
										 free($2);  /* WORD malloced with strdup */
										}
	| DLPIMDI WORD error			{
PrimeErrorBuffer(78,"the gethwkey command has failed");
										 error(NOTBCFG,"the dlpimdi cmd has an "
												"optional element argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

writestatuscmd: WRITESTATUS '\n' {
PrimeErrorBuffer(79,"the writestatus command has failed");
										 error(NOTBCFG,"the writestatus cmd has a "
												"filename argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										}
	| WRITESTATUS WORD '\n'		{
PrimeErrorBuffer(79,"the writestatus command has failed");
											(void)StartStatus($2);
										}
	| WRITESTATUS WORD error	{
PrimeErrorBuffer(79,"the writestatus command has failed");
										 error(NOTBCFG,"the writestatus cmd has a "
												"filename argument");
										 yyclearin;  /* discard lookahead */
										 yyerrok;
										 CmdFreeWords();
										}
	;

%%
