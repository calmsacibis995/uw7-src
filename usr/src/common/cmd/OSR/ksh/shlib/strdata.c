#ident	"@(#)OSRcmds:ksh/shlib/strdata.c	1.1"
#pragma comment(exestr, "@(#) strdata.c 25.3 93/01/20 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 * data for string evaluator library
 */

/*
 *  Modification History
 *
 *	L000	scol!markhe	25 Nov 92
 *	- moved declarations of messages strings to msg.c
 */

#include	"defs.h"					/* L000 */
#include	"streval.h"

const struct Optable optable[] =
	/* opcode	precedence,assignment  opname */
{
	{DEFAULT,	MAXPREC|NOASSIGN,	0177, 0 },
	{DONE,		0|NOASSIGN,		0 , 0 },
	{NEQ,		9|NOASSIGN,		'!', '=' },
	{NOT,		MAXPREC|NOASSIGN,	'!', 0 },
	{MOD,		13|NOFLOAT,		'%', 0 },
	{ANDAND,	5|NOASSIGN|SEQPOINT,	'&', '&' },
	{AND,		8|NOFLOAT,		'&', 0 },
	{LPAREN,	MAXPREC|NOASSIGN|SEQPOINT,'(', 0 },
	{RPAREN,	1|NOASSIGN,		')', 0 },
	{TIMES,		13,			'*', 0 },
#ifdef future
	{PLUSPLUS,	14|NOASSIGN|NOFLOAT|SEQPOINT, '+', '+'},
#endif
	{PLUS,		12,			'+', 0 },
#ifdef future
	{MINUSMINUS,	14|NOASSIGN|NOFLOAT|SEQPOINT, '-', '-'},
#endif
	{MINUS,		12,			'-', 0 },
	{DIVIDE,	13,			'/', 0 },
#ifdef future
	{COLON,		2|NOASSIGN,		':', 0 },
#endif
	{LSHIFT,	11|NOFLOAT,		'<', '<' },
	{LE,		10|NOASSIGN,		'<', '=' },
	{LT,		10|NOASSIGN,		'<', 0 },
	{EQ,		9|NOASSIGN,		'=', '=' },
	{ASSIGNMENT,	2|RASSOC,		'=', 0 },
	{RSHIFT,	11|NOFLOAT,		'>', '>' },
	{GE,		10|NOASSIGN,		'>', '=' },
	{GT,		10|NOASSIGN,		'>', 0 },
#ifdef future
	{QCOLON,	3|NOASSIGN|SEQPOINT,	'?', ':' },
	{QUEST,		3|NOASSIGN|SEQPOINT|RASSOC,	'?', 0 },
#endif
	{XOR,		7|NOFLOAT,		'^', 0 },
	{OROR,		5|NOASSIGN|SEQPOINT,	'|', '|' },
	{OR,		6|NOFLOAT,		'|', 0 }
};
