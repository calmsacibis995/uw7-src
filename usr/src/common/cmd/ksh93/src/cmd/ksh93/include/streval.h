#ident	"@(#)ksh93:src/cmd/ksh93/include/streval.h	1.2"
#pragma prototyped
#ifndef SEQPOINT
/*
 * G. S. Fowler
 * D. G. Korn
 * AT&T Bell Laboratories
 *
 * long integer arithmetic expression evaluator
 */

/* The following only is needed for const */
#include	<ast.h>
#include	<math.h>

struct lval
{
	char	*value;
	double	(*fun)(double,...);
	double	ovalue;
	short	flag;
	char	isfloat;
	char	nargs;
};

struct mathtab
{
	char	fname[8];
	double	(*fnptr)(double,...);
};

#define MAXPREC		15	/* maximum precision level */
#define SEQPOINT	0200	/* sequence point */
#define NOASSIGN	0100	/* assignment legal with this operator */
#define RASSOC		040	/* right associative */
#define NOFLOAT		020	/* illegal with floating point */
#define PRECMASK	017	/* precision bit mask */

#define A_EOF		1
#define A_NEQ		2
#define A_NOT		3
#define A_MOD		4
#define A_ANDAND	5
#define A_AND		6
#define A_LPAR		7
#define A_RPAR		8
#define A_TIMES		9
#define A_PLUSPLUS	10
#define A_PLUS		11
#define A_COMMA		12
#define A_MINUSMINUS	13
#define A_MINUS		14
#define A_DIV		15
#define A_LSHIFT	16
#define A_LE		17
#define A_LT		18
#define A_EQ		19
#define A_ASSIGN	20
#define A_COLON		21
#define A_RSHIFT	22
#define A_GE		23
#define A_GT		24
#define A_QCOLON	25
#define A_QUEST		26
#define A_XOR		27
#define A_OROR		28
#define A_OR		29
#define A_REG		30
#define A_DIG		31
#define A_DOT		32


/* define error messages */
extern const unsigned char	strval_precedence[33];
extern const char		strval_states[64];
extern const char		e_moretokens[];
extern const char		e_moretokens_id[];
extern const char		e_argcount[];
extern const char		e_argcount_id[];
extern const char		e_paren[];
extern const char		e_paren_id[];
extern const char		e_badnum[];
extern const char		e_badnum_id[];
extern const char		e_badcolon[];
extern const char		e_badcolon_id[];
extern const char		e_recursive[];
extern const char		e_recursive_id[];
extern const char		e_divzero[];
extern const char		e_divzero_id[];
extern const char		e_synbad[];
extern const char		e_synbad_id[];
extern const char		e_notlvalue[];
extern const char		e_notlvalue_id[];
extern const char		e_function[];
extern const char		e_function_id[];
extern const char		e_questcolon[];
extern const char		e_questcolon_id[];
extern const char		e_incompatible[];
extern const char		e_incompatible_id[];
extern const char		e_domain[];
extern const char		e_domain_id[];
extern const char		e_overflow[];
extern const char		e_overflow_id[];
extern const char		e_singularity[];
extern const char		e_singularity_id[];
extern const struct 		mathtab shtab_math[];

/* function code for the convert function */

#define LOOKUP	0
#define ASSIGN	1
#define VALUE	2
#define ERRMSG	3

extern double strval(const char*,char**,double(*)(const char**,struct lval*,int,double));

#endif /* !SEQPOINT */
