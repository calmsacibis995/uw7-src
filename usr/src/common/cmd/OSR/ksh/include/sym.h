#ident	"@(#)OSRcmds:ksh/include/sym.h	1.1"
#pragma comment(exestr, "@(#) sym.h 25.3 93/01/20 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992.			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 * Modification History
 *
 *	L000	scol!markhe	11 Nov 92
 *	- changed type of message variables from 'const char' to MSG
 */

/*
 *	UNIX shell
 *	S. R. Bourne
 *	Rewritten by David Korn
 */


/* symbols for parsing */
#define NOTSYM	'!'
#define DOSYM	0405
#define FISYM	0420
#define EFSYM	0422
#define ELSYM	0421
#define INSYM	0412
#define BRSYM	0406
#define KTSYM	0450
#define THSYM	0444
#define ODSYM	0441
#define ESSYM	0442
#define IFSYM	0436
#define FORSYM	0435
#define WHSYM	0433
#define UNSYM	0427
#define CASYM	0417
#define PROCSYM	0460
#define SELSYM	0470
#define TIMSYM	0474
#define BTSTSYM (SYMREP|'[')
#define ETSTSYM (SYMREP|']')
#define TESTUNOP	0466
#define TESTBINOP	0467

#define SYMREP	04000
#define ECSYM	(SYMREP|';')
#define ANDFSYM	(SYMREP|'&')
#define ORFSYM	(SYMREP|'|')
#define APPSYM	(SYMREP|'>')
#define DOCSYM	(SYMREP|'<')
#define EXPRSYM	(SYMREP|'(')
#define SYMALT1	01000
#define SYMALT2	010000
#define ECASYM	(SYMALT1|'&')
#define COOPSYM	(SYMALT1|'|')
#define IPROC	(SYMALT1|'(')
#define OPROC	(SYMALT2|'(')
#define EOFSYM	02000
#define SYMFLG	0400

/* arg to `sh_parse' */
#define NLFLG	1
#define MTFLG	2

/* odd chars */
#undef ESCAPE
#define DQUOTE	'"'
#define SQUOTE	'`'
#define DOLLAR	'$'
#define LBRACE	'{'
#define RBRACE	'}'
#define LPAREN	'('
#define RPAREN	')'
#define ESCAPE	'\\'
#define	COMCHAR	'#'		/* comment delimiter */
#define ENDOF	0
#define LITERAL	'\''		/* single quote */


/* wdset flags */
/*  KEYFLAG defined in defs.h,  the others must have a different value */
#define IN_CASE		1
#define CAN_ALIAS	2
#define IN_TEST		4
#define TEST_OP1	8
#define TEST_OP2	16

extern SYSTAB		tab_reserved;
extern SYSTAB		tab_options;
extern SYSTAB		tab_attributes;
extern MSG	e_unexpected;				/* L000 begin */
extern MSG	e_unmatched;
#ifdef DEVFD
    extern MSG		e_devfd;			/* L000 end */
#endif	/* DEVFD */
