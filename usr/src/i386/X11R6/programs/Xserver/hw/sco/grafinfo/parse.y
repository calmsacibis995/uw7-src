/*
 *	@(#) parse.y 12.1 95/05/09 SCOINC
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Fri Feb 01 19:54:59 PST 1991	pavelr@sco.com
 *	- Created file.
 *	S001	Thu Apr 25 19:53:23 PST 1991	pavelr@sco.com
 *	- major changes to remove case INsensitivity and tightened
 *	defintions. Added {}(), and such stuff.
 *	S002	Mon Aug 26 15:58:02 PDT 1991	pavelr@sco.com
 *	- rewrote most of file to handle free form grammer
 *	S003	Mon Sep 09 evening  PST 1991	pavelr@sco.com
 *	- added DATA keyword
 *	S004	Tue Mar 24 1992			pavelr@sco.com
 *      - added read & write support
 *	S005	Tue Sep 08 02:43:49 PDT 1992	buckm@sco.com
 *	- add int10 support.
 *	- memory and port arrays are now variable in size.
 *	- add port ranges and portgroup support.
 *	- add named memory.
 *	S006	Sun Mar 28 01:22:21 PST 1993	buckm@sco.com
 *	- add callrom support.
 *	S007	Tue Jun 29 1993	edb@sco.com
 *	- changes to execute compiled grafinfo files
 */

/* parse.y - grammar specification for GrafInfo files
*/

%{
#include <stdio.h>
#include <string.h>
#include "grafinfo.h"

#ifdef XSERVER						/* vvv S005 vvv */
#define malloc(x)	Xalloc(x)
#endif /* XSERVER */					/* vvv S005 vvv */

/* Used to keep track of and hold parsed code */
static int codeIndex;

/* current function name */
static char funName[256];

extern grafData *parsit;

extern codeType *grafCode;

#define	SETNUM(x) grafCode[codeIndex++]=G_NUMBER; grafCode[codeIndex++]=x;
#define	SETREG(x) grafCode[codeIndex++]=G_REGISTER; grafCode[codeIndex++]=x;
%}

%union
{
	int	number;
	char	*string;
}

/* keywords */
%token	<string> VENDOR MODEL CLASS MODE

%token PROCEDURE MEMORY PORT SET IN OUT OUTW AND OR NOT SHR SHL
%token BOUT INITGRAPHICS PLUS
%token XOR WAIT

%token DATA							/* S003 */

%token READB WRITEB READW WRITEW READDW WRITEDW			/* S004 */

%token INT10 CALLROM MINUS COLON				/* S005 S006 */

%token LPAREN RPAREN LBRACE RBRACE EQUAL COMMA SEMICOLON

%token	<number>	NUMBER REG
%token	<string>	STRING IDENT

%type	<number>	expr

%%

modedesc:	opt_memory_port_list
		data_section
		procedure_list
{
	YYACCEPT;
}
	;

opt_memory_port_list:	/* nothing */
	| opt_memory_port_list memory
	| opt_memory_port_list port
	;

							/* vvv S005 vvv */
memory:		MEMORY LPAREN NUMBER COMMA NUMBER RPAREN SEMICOLON
{
	AddMem (parsit, "", $3, $5);
}
	|	MEMORY LPAREN IDENT COMMA NUMBER COMMA NUMBER RPAREN SEMICOLON
{
	AddMem (parsit, $3, $5, $7);
}
	;

port:	PORT	LPAREN port_list RPAREN SEMICOLON
	;

port_list: 	port_element
	|	port_list COMMA port_element
	;

port_element:	NUMBER
{
	AddPort (parsit, $1, 1);
}
	|	NUMBER COLON NUMBER
{
	AddPort (parsit, $1, $3);
}
	|	NUMBER MINUS NUMBER
{
	if ($3 >= $1)
		AddPort (parsit, $1, 1 + $3 - $1);
	else
		AddPort (parsit, $3, 1 + $1 - $3);
}
	|	IDENT
{
	AddPortGroup (parsit, $1);
}
	;
							/* ^^^ S005 ^^^ */

data_section:	PROCEDURE INITGRAPHICS LBRACE assignment_list RBRACE
	|	DATA LBRACE assignment_list RBRACE		/* S003 */
	;

assignment_list:	/* nothing */
	|		assignment_list assignment SEMICOLON
	;

assignment:	IDENT	EQUAL	expr
{
	AddInt (parsit, $1, $3);
}
	|	IDENT	EQUAL	STRING
{
	AddString (parsit, $1, $3);
}
	;

procedure_list:	/* nothing */
	|	procedure_list procedure
	;

procedure:	procedure_start procedure_body   /* S007 */
	;

procedure_start: PROCEDURE	IDENT
{
		codeIndex = 0;
		strcpy (funName, $2);
		if( parsit->cType == COMPILED )   /* S007 */
		    grafSkipProcedure();          /* S007 */
}
	;
                                                  /* vvv S007 vvv */
procedure_body: /* nothing, in case it has been skipped by grafSkipProcedure() */
	|	 LBRACE command_list RBRACE
{
		grafCode[codeIndex++] = OP_END;
		AddFunction(parsit, funName, grafCode, codeIndex);
}
	;
                                                  /* ^^^ S007 ^^^ */
command_list:	/* nothing */
	|	 command_list command SEMICOLON;

command: 	assign
	|	set
	|	in
	|	out
	|	outw
	|	and
	|	or
	|	xor
	|	not
	|	shr
	|	shl
	|	wait
	|	bout
        |       readb						/* S004 */
        |       writeb						/* S004 */
        |       readw						/* S004 */
        |       writew						/* S004 */
        |       readdw						/* S004 */
        |       writedw						/* S004 */
	|	int10						/* S005 */
	|	callrom						/* S006 */
	;

	/* op codes getting stuffed into code tables */

set:		SET LPAREN	REG	COMMA expr	RPAREN
{
	grafCode[codeIndex++] = OP_ASSIGN;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	SET  LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_ASSIGN;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
	;

assign:		REG	EQUAL	expr
{
	grafCode[codeIndex++] = OP_ASSIGN;
	grafCode[codeIndex++] = $1;
	SETNUM($3);
}
	|	REG	EQUAL	REG
{
	grafCode[codeIndex++] = OP_ASSIGN;
	grafCode[codeIndex++] = $1;
	SETREG($3);
}
	;

out:		OUT LPAREN	expr	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_OUT;
	SETNUM ($3);
	SETNUM ($5);
}
	|	OUT LPAREN	expr	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_OUT;
	SETNUM ($3);
	SETREG ($5);
}
	|	OUT LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_OUT;
	SETREG ($3);
	SETNUM ($5);
}
	|	OUT LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_OUT;
	SETREG ($3);
	SETREG ($5);
}
	;

outw:		OUTW LPAREN	expr	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_OUTW;
	SETNUM ($3);
	SETNUM ($5);
}
	|	OUTW LPAREN	expr	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_OUTW;
	SETNUM ($3);
	SETREG ($5);
}
	|	OUTW LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_OUTW;
	SETREG ($3);
	SETNUM ($5);
}
	|	OUTW LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_OUTW;
	SETREG ($3);
	SETREG ($5);
}
	;


in:		IN LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_IN;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	IN LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_IN;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
	;

and:		AND LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_AND;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	AND LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_AND;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
	;

or:		OR LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_OR;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	OR LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_OR;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
	;

xor:		XOR LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_XOR;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	XOR LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_XOR;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
	;

not:		NOT LPAREN	REG	RPAREN
{
	grafCode[codeIndex++] = OP_NOT;
	grafCode[codeIndex++] = $3;
}
	;

shr:		SHR LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_SHR;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	SHR LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_SHR;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
	;

shl:		SHL LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_SHL;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	SHL LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_SHL;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
	;

wait:		WAIT LPAREN	expr	RPAREN
{
	grafCode[codeIndex++] = OP_WAIT;
	grafCode[codeIndex++] = $3;
}
	;

bout:		BOUT LPAREN	expr	COMMA	expr	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_BOUT;
	grafCode[codeIndex++] = $3;
	grafCode[codeIndex++] = $5;
	grafCode[codeIndex++] = $7;
}
	;

							/* vvv S004 vvv */
readb:		READB LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_READB;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	READB LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_READB;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
        ;

readw:		READW LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_READW;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	READW LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_READW;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
        ;

readdw:		READDW LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_READDW;
	grafCode[codeIndex++] = $3;
	SETNUM($5);
}
	|	READDW LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_READDW;
	grafCode[codeIndex++] = $3;
	SETREG($5);
}
        ;

writeb:		WRITEB LPAREN	expr	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEB;
	SETNUM ($3);
	SETNUM ($5);
}
	|	WRITEB LPAREN	expr	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEB;
	SETNUM ($3);
	SETREG ($5);
}
	|	WRITEB LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEB;
	SETREG ($3);
	SETNUM ($5);
}
	|	WRITEB LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEB;
	SETREG ($3);
	SETREG ($5);
}
	;

writew:		WRITEW LPAREN	expr	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEW;
	SETNUM ($3);
	SETNUM ($5);
}
	|	WRITEW LPAREN	expr	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEW;
	SETNUM ($3);
	SETREG ($5);
}
	|	WRITEW LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEW;
	SETREG ($3);
	SETNUM ($5);
}
	|	WRITEW LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEW;
	SETREG ($3);
	SETREG ($5);
}
	;

writedw:	WRITEDW LPAREN	expr	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEDW;
	SETNUM ($3);
	SETNUM ($5);
}
	|	WRITEDW LPAREN	expr	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEDW;
	SETNUM ($3);
	SETREG ($5);
}
	|	WRITEDW LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEDW;
	SETREG ($3);
	SETNUM ($5);
}
	|	WRITEDW LPAREN	REG	COMMA	REG	RPAREN
{
	grafCode[codeIndex++] = OP_WRITEDW;
	SETREG ($3);
	SETREG ($5);
}
	;
							/* ^^^ S004 ^^^ */

							/* vvv S005 vvv */
int10:		INT10	LPAREN	REG	COMMA	expr	RPAREN
{
	grafCode[codeIndex++] = OP_INT10;
	grafCode[codeIndex++] = $3;
	grafCode[codeIndex++] = $5;
	if ( ($5 < 1) || ($5 > 8) || (($3 + $5) > NUMBER_REG) )
		graferrno = GEBADREG;
}
	;
							/* ^^^ S005 ^^^ */

							/* vvv S006 vvv */
callrom:	CALLROM LPAREN expr COMMA expr COMMA REG COMMA expr RPAREN
{
	grafCode[codeIndex++] = OP_CALLROM;
	SETNUM ($3)
	SETNUM ($5)
	grafCode[codeIndex++] = $7;
	grafCode[codeIndex++] = $9;
	if ( ($9 < 1) || ($9 > 8) || (($7 + $9) > NUMBER_REG) )
		graferrno = GEBADREG;
}
	|	CALLROM LPAREN expr COMMA REG COMMA REG COMMA expr RPAREN
{
	grafCode[codeIndex++] = OP_CALLROM;
	SETNUM ($3)
	SETREG ($5)
	grafCode[codeIndex++] = $7;
	grafCode[codeIndex++] = $9;
	if ( ($9 < 1) || ($9 > 8) || (($7 + $9) > NUMBER_REG) )
		graferrno = GEBADREG;
}
	|	CALLROM LPAREN REG COMMA expr COMMA REG COMMA expr RPAREN
{
	grafCode[codeIndex++] = OP_CALLROM;
	SETREG ($3)
	SETNUM ($5)
	grafCode[codeIndex++] = $7;
	grafCode[codeIndex++] = $9;
	if ( ($9 < 1) || ($9 > 8) || (($7 + $9) > NUMBER_REG) )
		graferrno = GEBADREG;
}
	|	CALLROM LPAREN REG COMMA REG COMMA REG COMMA expr RPAREN
{
	grafCode[codeIndex++] = OP_CALLROM;
	SETREG ($3)
	SETREG ($5)
	grafCode[codeIndex++] = $7;
	grafCode[codeIndex++] = $9;
	if ( ($9 < 1) || ($9 > 8) || (($7 + $9) > NUMBER_REG) )
		graferrno = GEBADREG;
}
	;
							/* ^^^ S006 ^^^ */

expr:		NUMBER
{
	$$=$1;
}
	|	NUMBER PLUS expr
{
	$$ = $1 + $3;
}
	;

%%

#define ERRORSTRINGSIZE	256
char *parseError;

void
yyerror(char *message)
{
	extern int grafLineno;
	if (parseError == NULL)					/* S005 */
		parseError = (char *) malloc (ERRORSTRINGSIZE);	/* S005 */
	sprintf(parseError, "%d:%s", grafLineno, message);
	graferrno = GEPARSE;
}

char *GetParseError ()
{
	return (parseError);
}
