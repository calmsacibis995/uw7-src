#ident	"@(#)ksh93:src/cmd/ksh93/data/strdata.c	1.2"
#pragma prototyped
/*
 * data for string evaluator library
 */

#include	"FEATURE/options"
#include	"streval.h"

const unsigned char strval_precedence[33] =
	/* opcode	precedence,assignment  */
{
	/* DEFAULT */		MAXPREC|NOASSIGN,
	/* DONE */		0|NOASSIGN|RASSOC,
	/* NEQ */		10|NOASSIGN,
	/* NOT */		MAXPREC|NOASSIGN,
	/* MOD */		14,
	/* ANDAND */		6|NOASSIGN|SEQPOINT,
	/* AND */		9|NOFLOAT,
	/* LPAREN */		MAXPREC|NOASSIGN|SEQPOINT,
	/* RPAREN */		1|NOASSIGN|RASSOC|SEQPOINT,
	/* TIMES */		14,
	/* PLUSPLUS */		15|NOASSIGN|NOFLOAT|SEQPOINT,
	/* PLUS */		13,	
	/* COMMA */		1|NOASSIGN|SEQPOINT,
	/* MINUSMINUS */	15|NOASSIGN|NOFLOAT|SEQPOINT,
	/* MINUS */		13,
	/* DIV */		14,
	/* LSHIFT */		12|NOFLOAT,
	/* LE */		11|NOASSIGN,
	/* LT */		11|NOASSIGN,	
	/* EQ */		10|NOASSIGN,
	/* ASSIGNMENT */	2|RASSOC,
	/* COLON */		0|NOASSIGN,
	/* RSHIFT */		12|NOFLOAT,	
	/* GE */		11|NOASSIGN,
	/* GT */		11|NOASSIGN,
	/* QCOLON */		3|NOASSIGN|SEQPOINT,
	/* QUEST */		3|NOASSIGN|SEQPOINT|RASSOC,
	/* XOR */		8|NOFLOAT,
	/* OROR */		5|NOASSIGN|SEQPOINT,
	/* OR */		7|NOFLOAT,
	/* DEFAULT */		MAXPREC|NOASSIGN,
	/* DEFAULT */		MAXPREC|NOASSIGN,
	/* DEFAULT */		MAXPREC|NOASSIGN
};

/*
 * This is for arithmetic expressions
 */
const char strval_states[64] =
{
	A_EOF,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,
	A_REG,	0,	0,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,
	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,
	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,	A_REG,

	0,	A_NOT,	A_REG,	A_REG,	A_REG,	A_MOD,	A_AND,	A_REG,
	A_LPAR,	A_RPAR,	A_TIMES,A_PLUS,	A_COMMA,A_MINUS,A_DOT,	A_DIV,
	A_DIG,	A_DIG,	A_DIG,	A_DIG,	A_DIG,	A_DIG,	A_DIG,	A_DIG,
	A_DIG,	A_DIG,	A_COLON,A_REG,	A_LT,	A_ASSIGN,A_GT,	A_QUEST

};



const char e_argcount[]         = "%s: function has wrong number of arguments";
const char e_argcount_id[]	= ":395";
const char e_badnum[]		= "%s: bad number";
const char e_badnum_id[]	= ":13";
const char e_moretokens[]	= "%s: more tokens expected";
const char e_moretokens_id[]	= ":190";
const char e_paren[]		= "%s: unbalanced parenthesis";
const char e_paren_id[]		= ":191";
const char e_badcolon[]		= "%s: invalid use of :";
const char e_badcolon_id[]	= ":192";
const char e_divzero[]		= "%s: divide by zero";
const char e_divzero_id[]	= ":193";
const char e_synbad[]		= "%s: arithmetic syntax error";
const char e_synbad_id[]	= ":194";
const char e_notlvalue[]	= "%s: assignment requires lvalue";
const char e_notlvalue_id[]	= ":195";
const char e_recursive[]	= "%s: recursion too deep";
const char e_recursive_id[]	= ":34";
const char e_questcolon[]	= "%s: ':' expected for '?' operator";
const char e_questcolon_id[]	= ":196";
const char e_function[]		= "%s: unknown function";
const char e_function_id[]	= ":197";
const char e_incompatible[]	= "%s: operands have incompatible types";
const char e_incompatible_id[]	= ":198";
const char e_overflow[]		= "%s: overflow exception";
const char e_overflow_id[]	= ":199";
const char e_domain[]		= "%s: domain exception";
const char e_domain_id[]	= ":200";
const char e_singularity[]	= "%s: singularity exception";
const char e_singularity_id[]	= ":201";

typedef double (*mathf)(double,...);

const struct mathtab shtab_math[] =
{
	"\01abs",		(mathf)fabs,
	"\01acos",		(mathf)acos,
	"\01asin",		(mathf)asin,
	"\01atan",		(mathf)atan,
	"\02atan2",		(mathf)atan2,
	"\01cos",		(mathf)cos,
	"\01cosh",		(mathf)cosh,
	"\01exp",		(mathf)exp,
	"\01floor",		(mathf)floor,
	"\02fmod",		(mathf)fmod,
	"\02hypot",		(mathf)hypot,
	"\01int",		(mathf)floor,
	"\01log",		(mathf)log,
	"\02pow",		(mathf)pow,
	"\01sin",		(mathf)sin,
	"\01sinh",		(mathf)sinh,
	"\01sqrt",		(mathf)sqrt,
	"\01tan",		(mathf)tan,
	"\01tanh",		(mathf)tanh,
	"",			(mathf)0
};
