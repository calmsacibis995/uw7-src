#ident	"@(#)acpp:common/xpr.c	1.51.1.10"
/* xpr.c - parse and evaluate constant expressions */

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "cpp.h"
#include "syms.h"
#include "file.h"

#define BITMASK(n) (((n)==SZLONG)?-1L:((1L<<(n))-1))
#ifdef	DEBUG
#ifdef __STDC__
#	define DBGCALL(num,funcname,tp)\
		if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, #funcname "() called with: ");\
			tk_pr(tp, '\n');\
		}
#	define DBGCALLL(num,funcname,tp)\
		if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, #funcname "() called with: ");\
			tk_prl(tp);\
		}
#	define DBGRET(num,funcname,retval)\
                if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, #funcname "() returns:");\
			prifval(retval);\
		}
#	define DBGRETN(num,funcname,retnum)\
		if ( DEBUG('x') > (num) )\
			(void)fprintf(stderr, #funcname "() return value = %ld\n",(retnum) );
#else
#	define DBGCALL(num,funcname,tp)\
		if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, "funcname() called with: ");\
			tk_pr(tp, '\n');\
		}
#	define DBGCALLL(num,funcname,tp)\
		if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, "funcname() called with: ");\
			tk_prl(tp);\
		}
#	define DBGRET(num,funcname,retval)\
                if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, "funcname() returns:");\
			prifval(retval);\
		}
#	define DBGRETN(num,funcname,retnum)\
		if ( DEBUG('x') > (num) )\
			(void)fprintf(stderr, "funcname() return value = %ld\n",(retnum) );
#endif
#else
#	define DBGCALL(num,funcname,tp)
#	define DBGCALLL(num,funcname,tp)
#	define DBGRET(num,funcname,retval)		
#	define DBGRETN(num,funcname,retnum)		
#endif
#define SZCHAR 		8
#define SZLONG		SZINT
#define SZINT 		32

#ifdef __STDC__

typedef enum _Action {
	A_reduce,
	A_shift,
	A_accept,
	A_error
} Action;
typedef enum _Expect {
	E_number,
	E_operator
} Expect;
typedef enum _Type {
	T_signed,
	T_unsigned
} Type;

#else	/* 2.1 disallows assignment of enum member to an int */

typedef long wchar_t;

typedef int	 Action;
typedef int	 Boolean;
typedef int	 Expect;
typedef int	 Type;
#	define	A_reduce	0
#	define	A_shift		1
#	define	A_accept	2
#	define	A_error		3
#	define	E_number	0
#	define	E_operator	1
#	define	T_signed	0
#	define	T_unsigned	1
#endif

/* Parses and evaluates the constant expression in condition inclusion
** directives. Operator precedence parsing is used.
** Evaluation is done by a push-down automaton that has
** two stacks, one for operators and the other for operands.
*/

typedef struct tree {
	struct tree	*next;	/* when on operator/number stack */
	struct tree	*left;
	struct tree	*right;
	unsigned short	code;
	unsigned char	unsgnd;
	IntNum		value;
} Tree;

static Token *	curtoken;	/* current token */
static Expect	expect;		/* what next token is expected to be */
static int	input_op;	/* code corresponding to curtoken */
static Tree *	nstp;		/* top of number stack	*/
static Tree * 	ostp;		/* top of operator stack*/

static IntNum	num0;
static IntNum	num1;
static IntNum	numsmax;
static int	oflow;

static	Action 	action();
static	void	freetree();
static	void	lxchar();
static	void	lxicon();
static  int	precedence();
#ifdef DEBUG
static  void  	prifval();
#endif
static	void 	primary();
static	void  	reduce();
static	Action	reducechk();
static	void  	shift();

static const int 	prec[]= { /* precedence table */
	10,	/* C_RParen	 )	*/
	11,	/* C_Comma	 ,	*/ /* not allowed in ANSI C */
	12,	/* C_Question	 ?	*/
	12,	/* C_Colon	 :	*/
	13,	/* C_LogicalOR	 ||	*/
	14,	/* C_LogicalAND	 &&	*/
	15,	/* C_InclusiveOR |	*/
	16,	/* C_ExclusiveOR ^	*/
	17,	/* C_BitwiseAND	 &	*/
	18,	/* C_Equal		==	*/
	18,	/* C_NotEqual		!=	*/
	19,	/* C_GreaterThan	>	*/
	19,	/* C_NotGreaterThan	!>	*/
	19,	/* C_GreaterEqual	>=	*/
	19,	/* C_NotGreaterEqual	!>=	*/
	19,	/* C_LessThan		<	*/
	19,	/* C_NotLessThan	!<	*/
	19,	/* C_LessEqual		<=	*/
	19,	/* C_NotLessEqual	!<=	*/
	19,	/* C_LessGreater	<>	*/
	19,	/* C_NotLessGreater	!<>	*/
	19,	/* C_LessGreaterEqual	<>=	*/
	19,	/* C_NotLessGreaterEqual!<>=	*/
	20,	/* C_LeftShift	 <<	*/
	20,	/* C_RightShift	 >>	*/
	21,	/* C_Plus	 +	*/
	21,	/* C_Minus	 -	*/
	22,	/* C_Mult	 *	*/
	22,	/* C_Div	 /	*/
	22,	/* C_Mod	 %	*/
	23,	/* C_UnaryPlus	 +	*/
	23,	/* C_UnaryMinus	 -	*/
	23,	/* C_Complement	 ~	*/
	23,	/* C_Not	 !	*/
	24,	/* C_LParen 	 (	*/
	100	/* C_Operand	any integral constant	*/
};

static Action
action()
/* This routine examines the current input Token and judges what
** action the automaton should take.
** (i.e shift, reduce, accept or give an error).
*/
{
#ifdef DEBUG
	if ( DEBUG('x') > 4 )
	{
		(void)fprintf( stderr, "action() called on: ");
		if ( curtoken )
			tk_pr( curtoken, '\n' );
		else
			fputc( '\n', stderr );
	}
#endif
	if (curtoken == 0)
	{
		if (ostp == 0)
			return A_accept;
		else if (expect == E_number)
		{
			UERROR(gettxt(":526","number expected"));
			return A_error;
		}
		else
			return reducechk();
	}
	if (TK_CONDEXPR(curtoken))
		if (TK_ISCONSTRAINED(curtoken))
			input_op = C_Operand;
		else
			input_op = curtoken->code;
	else if (curtoken->code == C_Sharp)
	{
		COMMENT(fl_numerrors() >= 1);
		return A_error;
	}
	else
	{
		TKERROR(gettxt(":527","token not allowed in directive"), curtoken);
		return A_error;
	}
	switch (input_op)
	{
	case C_Operand:
	case C_Not:
	case C_Complement:
		if ( expect == E_operator )
		{
			UERROR(gettxt(":528","missing operator" ));
			return A_error;
		}
		break;

	case C_RParen:
		if ( expect == E_number )
		{
			UERROR(gettxt(":529", "unexpected \")\"" ));
			return A_error;
		}
		break;

	case C_Plus:
		if ( expect == E_number )	input_op = C_UnaryPlus;
		break;

	case C_Minus:
		if ( expect == E_number )	input_op = C_UnaryMinus;
		break;

	case C_LParen:
		if ( expect == E_operator )
		{
			UERROR(gettxt(":530", "unexpected \"(\"" ));
			return A_error;
		}
		break;

	default:if(input_op >= C_Question && input_op <= C_Mod && expect==E_number)
		{	/* for now - is there an easier way to say this ?- binary ops ? */
			UERROR(gettxt(":531","missing operand"));
			return A_error;
		}
	}
	if  ((ostp == 0) || (precedence((int)ostp->code) < precedence(input_op)))
		return A_shift;
	else if (precedence((int)ostp->code) > precedence(input_op))
		return reducechk();
	else 
		switch (input_op)
		{
		case C_Colon:
			if (ostp->code == C_Colon)	return reducechk();
			/*FALLTHRU*/
		case C_RParen:
		case C_UnaryPlus:
		case C_UnaryMinus:
		case C_Complement:
		case C_Not:
			return A_shift;

		case C_Question:
			if (	(ostp->code == C_Question) ||
			     	(ostp->code == C_Colon)	
			   )	
				return A_shift;
		default:return reducechk();
		}
}

static void
eval(tree)
Tree *tree;
/* Recursively evaluates the expression tree rooted at tree.  Put the numeric
** result in tree->value.  Free left and right subtrees.
*/
{
	Tree *left = tree->left;
	Tree *right = tree->right;
	Tree *t;
	int c;

#ifdef DEBUG
	if ( DEBUG('x') > 1 )
		(void)fprintf(stderr, "eval() called on: %13s\n",
			tk_saycode(tree->code));
#endif
	switch (TK_ENUMNO(tree->code))
	{
	CASE(C_Equal)
	CASE(C_NotEqual)
	CASE(C_GreaterThan)
	CASE(C_GreaterEqual)
	CASE(C_LessThan)
	CASE(C_LessEqual)
#ifndef NO_NCEG
	CASE(C_NotGreaterThan)
	CASE(C_NotGreaterEqual)
	CASE(C_NotLessThan)
	CASE(C_NotLessEqual)
	CASE(C_LessGreater)
	CASE(C_NotLessGreater)
	CASE(C_LessGreaterEqual)
	CASE(C_NotLessGreaterEqual)
#endif /*NO_NCEG*/
		eval(left);
		eval(right);
		c = (*(left->unsgnd || right->unsgnd
			? &num_ucompare : &num_scompare))
				(&left->value, &right->value);
		break;
	CASE(C_InclusiveOR)
	CASE(C_ExclusiveOR)
	CASE(C_BitwiseAND)
	CASE(C_LeftShift)
	CASE(C_RightShift)
	CASE(C_Plus)
	CASE(C_Minus)
	CASE(C_Mult)
	CASE(C_Div)
	CASE(C_Mod)
		eval(right);
		/*FALLTHROUGH*/
	CASE(C_UnaryPlus) /* shouldn't happen */
	CASE(C_UnaryMinus)
	CASE(C_Complement)
	CASE(C_Not)
		eval(left);
		tree->value = left->value;
		break;
	}
	switch (TK_ENUMNO(tree->code))
	{
	CASE(C_Question)
		eval(left);
		if (num_ucompare(&left->value, &num0) != 0)
			t = right->left;
		else
			t = right->right;
		eval(t);
		tree->value = t->value;
		break;

	CASE(C_LogicalOR)
		eval(left);
		if (num_ucompare(&left->value, &num0) != 0)
		{
			tree->value = num1;
			break;
		}
	evalright:
		eval(right);
		c = num_ucompare(&right->value, &num0);
	logical:
		tree->value = *(c ? &num1 : &num0);
		break;

	CASE(C_LogicalAND)
		eval(left);
		if (num_ucompare(&left->value, &num0) == 0)
		{
			tree->value = num0;
			break;
		}
		goto evalright;

	CASE(C_InclusiveOR)
		num_or(&tree->value, &right->value);
		break;

	CASE(C_ExclusiveOR)
		num_xor(&tree->value, &right->value);
		break;

	CASE(C_BitwiseAND)
		num_and(&tree->value, &right->value);
		break;

	CASE(C_Equal)
#ifndef NO_NCEG
	CASE(C_NotLessGreater)
#endif
		c = c == 0;
		goto logical;

	CASE(C_NotEqual)
#ifndef NO_NCEG
	CASE(C_LessGreater)
#endif
		goto logical;

	CASE(C_GreaterThan)
#ifndef NO_NCEG
	CASE(C_NotLessEqual)
#endif
		c = c > 0;
		goto logical;

	CASE(C_GreaterEqual)
#ifndef NO_NCEG
	CASE(C_NotLessThan)
#endif
		c = c >= 0;
		goto logical;

	CASE(C_LessThan)
#ifndef NO_NCEG
	CASE(C_NotGreaterEqual)
#endif
		c = c < 0;
		goto logical;

	CASE(C_LessEqual)
#ifndef NO_NCEG
	CASE(C_NotGreaterThan)
#endif
		c = c <= 0;
		goto logical;

#ifndef NO_NCEG
	CASE(C_LessGreaterEqual)
		c = 0;
		goto logical;

	CASE(C_NotLessGreaterEqual)
		c = 1;
		goto logical;
#endif /*NO_NCEG*/

	CASE(C_LeftShift)
	CASE(C_RightShift)
		oflow |= num_tosint(&right->value, &c);
		if (tree->code == C_LeftShift)
			oflow |= num_llshift(&tree->value, c);
#ifdef C_SIGNED_RS
		else if (!tree->unsgnd)
			oflow |= num_arshift(&tree->value, c);
#endif
		else
			oflow |= num_lrshift(&tree->value, c);
		break;

	CASE(C_Plus)
		oflow |= (*(tree->unsgnd ? &num_uadd : &num_sadd))
			(&tree->value, &right->value);
		break;

	CASE(C_Minus)
		oflow |= (*(tree->unsgnd ? &num_usubtract : &num_ssubtract))
			(&tree->value, &right->value);
		break;

	CASE(C_Mult)
		oflow |= (*(tree->unsgnd ? &num_umultiply : &num_smultiply))
			(&tree->value, &right->value);
		break;

	CASE(C_Div)
		c = (*(tree->unsgnd ? &num_udivide : &num_sdivide))
			(&tree->value, &right->value);
		if (c && num_ucompare(&right->value, &num0) == 0)
		{
			c = 0;
			WARN(gettxt(":532","division by zero"));
		}
		oflow |= c;
		break;

	CASE(C_Mod)
		c = (*(tree->unsgnd ? &num_uremainder : &num_sremainder))
			(&tree->value, &right->value);
		if (c && num_ucompare(&right->value, &num0) == 0)
		{
			c = 0;
			WARN(gettxt(":532","modulus by zero"));
		}
		oflow |= c;
		break;

	CASE(C_UnaryPlus)
#ifdef DEBUG
		fprintf(stderr, "UnaryPlus in eval()\n");
#endif
		break;

	CASE(C_UnaryMinus)
		c = num_negate(&tree->value);
		if (!tree->unsgnd)
			oflow |= c;
		break;

	CASE(C_Complement)
		num_complement(&tree->value);
		break;

	CASE(C_Not)
		num_not(&tree->value);
		break;

	CASE(C_LParen)
		FATAL(gettxt(":736","C_LParen in eval()"), "");
		break;
	}
	tree->code = C_Operand;
#ifdef DEBUG
	if ( DEBUG('x') > 2 )
	{
		(void)fprintf(stderr, "eval() returns %s\n",
			num_tosdec(&tree->value));
	}
#endif
}

static void
freetree(t)
register Tree *t;
/* Recursively free tree rooted at t. */ 
{
	if (t->left)
		freetree(t->left);
	if (t->right)
		freetree(t->right);
	free(t);
}

static unsigned long
escape(p) 
char **p;
/* Return the escaped value of *p.  Also, make sure that *p is the
** next input character upon return.
*/
{
	register char *cp = *p;
	register unsigned long val;

	switch( *cp )
	{
#ifdef DEBUG
	case '\n':
		FATAL("new-line gotten by bf_tokenize()", "");
		/*NOTREACHED*/
#endif
	default: if (isprint(*cp))
			WARN(gettxt(":259", "dubious escape: \\%c"), *cp );
		else
			WARN(gettxt(":260", "dubious escape: \\<%#x>"), *cp );
		val = *cp;
		break;
	case '?':
	case '\'':
	case '"':
	case '\\':
		val = *cp;
		break;
	case 'n':
		val = '\n';
		break;
	case 'r':
		val = '\r';
		break;
	case 'a':
#ifdef __STDC__
		val = '\a';
#else
		val = '\007';
#endif
#ifdef TRANSITION
		if ( pp_flags & F_Xt )
			WARN(gettxt(":258","\\a is ANSI C \"alert\" character"));
#endif
		break;
	case 'b':
		val = '\b';
		break;
	case 't':
		val = '\t';
		break;
	case 'f':
		val = '\f';
		break;
	case 'v':
		val = '\v';
		break;
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		val = *cp-'0';
		cp++;
		if( *cp >= '0' && *cp <= '7' ) {
			val = (val << 3) | (*cp-'0');
			cp++; 
			if( *cp >= '0' && *cp <= '7' )
				val = (val << 3) | (*cp-'0');
			else 
				cp--;
		}
		else 
			cp--;
		break;

	case 'x':
	{
		int n = 0;	/* seen digit? */
		int overflow = 0;
		unsigned long digit;

		val = 0;
		for (;;)
		{
			switch (*++cp)
			{
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			case '8': case '9':
				digit = *cp - '0';
				break;

			case 'a': case 'b': case 'c':
			case 'd': case 'e': case 'f':
				digit = 10 + *cp - 'a';
				break;

			case 'A': case 'B': case 'C':
			case 'D': case 'E': case 'F':
				digit = 10 + *cp - 'A';
				break;

			default:cp--;
				goto end_hex;
			}
			n = 1;
			if (val & (~((unsigned long)(~0L) >> 4)))
				overflow = 1;
			val = (val << 4) | digit;
		}
end_hex: 	
#ifdef TRANSITION
		if ( pp_flags & F_Xt )
			WARN(gettxt(":261", "\\x is ANSI C hex escape" ));
		else
#endif
		if (n == 0) 
			WARN(gettxt(":262", "no hex digits follow \\x" ));
		if (n == 0)	
			val = 'x';
		else if (overflow)
			WARN(gettxt(":263", "overflow in hex escape" ));
		break;
	} /* end case */
	} /* end switch */
	*p = cp;
	return val;
}

static void
lxchar(Tree *tree)
/* Evaluate character constant input Token.
** The behavior of multiple characters in a character constant
** is implementation defined, as is an escape sequence not represented
** in the execution character set.
*/
{
	static const char toolong[] = "character constant too long"; /* WARN */
	static const char empty[] = "empty character constant"; /* UERROR */
	unsigned long val, num;
    	char *cp;
	int i;

	DBGCALL(4,lxchar,curtoken);
	COMMENT(curtoken->code == C_C_Constant);
    	tree->code  = C_Operand;
	cp = curtoken->ptr.string;
	i=0;
	if (*cp == 'L')		/* multibyte char constant */
	{
		unsigned int len;
		int wclen;
		wchar_t wc;

		len = curtoken->rlen - 3;
		cp += 2;
		if (len == 0) 
			UERROR(gettxt(":111",empty));
		else if (*cp == '\\') {
			++cp;
			(void)num_fromulong(&tree->value, escape(&cp));
			if (*cp != '\'')
				WARN(gettxt(":397",toolong));
		}
		else {
			wclen = mbtowc(&wc, cp, len);
			(void)num_fromulong(&tree->value, (unsigned long)wc);
			if (len > 1)
				WARN(gettxt(":397",toolong));
			else if (wclen <= 0)  {
				UERROR(gettxt(":256", "invalid multibyte character"));
				tree->value = num0;
			}
		}
		DBGRET(0, lxchar, tree);
		return;
	}
	/*
	* Here we assume that a native "long" is big enough
	* to hold the target's "int".
	*/
	while( *++cp  != '\'' )
	{
		switch( *cp )
		{
		case '\\':
			++cp;
			val = escape(&cp);
			if (val & (~BITMASK(SZCHAR))) {
				WARN(gettxt(":255","character escape does not fit in character"));
				val &= BITMASK(SZCHAR);	/* truncate value */
			}
			break;
		default:
			val = *cp;
			break;
		}
		/* sign extend */
		if (chars_signed && (val & (1 << (SZCHAR - 1))))
			val |= ~BITMASK(SZCHAR);
		if (i == 0)
			num = val;
		else if (i < SZINT/SZCHAR)
		{
#ifdef RTOLBYTES
			num <<= SZCHAR;
			num |= val & BITMASK(SZCHAR);
#else
			num &= BITMASK(SZCHAR*i);
			num |= val << (SZCHAR*i);
#endif
		}
		++i;
	}
	if (chars_signed)
		(void)num_fromslong(&tree->value, *(long *)&num);
	else
		(void)num_fromulong(&tree->value, num);
	if( i == 0 )
		UERROR(gettxt(":111",empty));
	else if( i > 1 )  {
		TKWARN(gettxt(":534","more than one character honored in character constant"),
								curtoken);
		if( i > SZINT/SZCHAR)
			WARN(gettxt(":397",toolong));
	}
	DBGRET(3,lxchar,tree);
    	return;
}

static void
lxicon(Tree *tree)
{
	NumStrErr err;

	DBGCALL(4,lxicon,curtoken);
	(void)num_fromstr(&tree->value, curtoken->ptr.string,
		curtoken->rlen, &err);
#ifdef TRANSITION
	/*
	* Special check for "octal digits" 8 or 9.
	* We don't bother to handle big versions of these.
	*/
	if (err.code == NUM_STRERR_INVALID
		&& (*err.next == '8' || *err.next == '9'))
	{
		unsigned char *ptr = (unsigned char *)curtoken->ptr.string;
		unsigned long val = 0;
		size_t cnt = 1;

		COMMENT(pp_flags & F_Xt && *ptr == '0');
		WARN(gettxt(":330", "bad octal digit: '%c'"), *err.next);
		while (isdigit(*++ptr) && ++cnt <= curtoken->rlen)
		{
			val <<= 3;
			val |= *ptr - '0';
		}
		(void)num_fromulong(&tree->value, val);
		err.next = (char *)ptr;
		if (cnt >= curtoken->rlen)
			err.code = NUM_STRERR_NONE;
	}
#endif /*TRANSITION*/
	tree->code = C_Operand;
	if (err.code == NUM_STRERR_INVALID) /* should be suffix */
	{
		err.code = NUM_STRERR_NONE;
		if (err.next[0] == 'u' || err.next[0] == 'U')
			tree->unsgnd = 1;
		else if (err.next[0] != 'l' && err.next[0] != 'L')
			err.code = NUM_STRERR_INVALID;
		else if (&curtoken->ptr.string[curtoken->rlen - 1] <= err.next)
			/*EMPTY*/;
		else if (err.next[1] == 'u' || err.next[1] == 'U')
			tree->unsgnd = 1;
		else if (err.next[1] != 'l' && err.next[1] != 'L')
			err.code = NUM_STRERR_INVALID;
	}
	if (!tree->unsgnd && (err.code == NUM_STRERR_RANGE
		|| num_ucompare(&tree->value, &numsmax) > 0))
	{
#ifdef TRANSITION
		if (pp_flags & F_Xt)
		{
			TKWARN(gettxt(":535", "operand treated as unsigned"),
				curtoken);
		}
#endif
		tree->unsgnd = 1;
	}
	if (err.code == NUM_STRERR_RANGE)
		TKWARN(gettxt(":1630", "constant value too big"), curtoken);
	else if (err.code != NUM_STRERR_NONE)
		FATAL("malformed integer constant", "");
	DBGRET(3,lxicon,tree);
}

static int
precedence(code)
	int code;
/* Given a code that identifies a Token, this routine returns
** a precedence of that Token.
*/
{
	return prec[TK_ENUMNO(code) - TK_ENUMNO(C_RParen)];
}

#ifdef DEBUG
static void
prifval(Tree *tree)
{
	(void)fprintf(stderr, " %s ", num_tohex(&tree->value));
	if (tree->unsgnd) {
		(void)fprintf(stderr, "(unsigned)=%s\n",
			num_toudec(&tree->value));
	} else {
		(void)fprintf(stderr, "(signed)=%s\n",
			num_tosdec(&tree->value));
	}
}
#endif

static void
primary(Tree *tree)
/* This routine returns the value of what the ANSI standard grammar
** calls "primary expressions." This assumes that the current
** input Token is valid - the tokenization routines are
** counted on to catch all syntax errors before this stage of processing.
*/
{
	register Token *tp = curtoken;
	
	DBGCALL(3,primary,tp);
    	switch(tp->code)
	{
    	case C_Identifier:
		tree->value = num0;
		tree->code = C_Operand;
		break;
    	case C_C_Constant:
		lxchar(tree);
		break;
	case C_I_Constant:
		lxicon(tree);
		break;
	}
}

static  void
reduce()
/* Reduces by popping the operator stack and (if applicable) popping
** operands off of the number stack and pushing the resulting expression tree.
** Keeps track whether the next token is expected to be an operand.
*/
{
	Tree *t;
	int isunsigned = -1;	/* -1 unknown; 0 signed; 1 unsigned */

#ifdef DEBUG
	if ( DEBUG('x') > 1 )
		(void)fprintf(stderr, "reduce() called on: %13s <-> %-13s\n",
			tk_saycode(ostp->code), tk_saycode(input_op));
#endif
	switch (TK_ENUMNO(ostp->code))
	{
	CASE(C_Colon)
		/* t1 ? t2 : t3;
		** Form tree that looks like
		** 	?
		**     / \
		**    t1  :
		**	 / \
		**      t2  t3
		*/
		t = ostp->next;			/* ? */
		t->left = nstp->next->next;	/* t1 */
		t->right = ostp;		/* :  */
		t->right->left = nstp->next;	/* t2 */
		t->right->right = nstp;		/* t3 */
		if (t->right->left->unsgnd || t->right->right->unsgnd)
			t->unsgnd = t->right->unsgnd = 1;
		ostp = ostp->next->next;
		t->next = nstp->next->next->next; 
		nstp->next = nstp->next->next = nstp->next->next->next = 0;
		nstp = t;
		break;

	CASE(C_LogicalOR)
	CASE(C_LogicalAND)
	CASE(C_Equal)
	CASE(C_NotEqual)
	CASE(C_GreaterThan)
	CASE(C_GreaterEqual)
	CASE(C_LessThan)
	CASE(C_LessEqual)
#ifndef NO_NCEG
	CASE(C_NotGreaterThan)
	CASE(C_NotGreaterEqual)
	CASE(C_NotLessThan)
	CASE(C_NotLessEqual)
	CASE(C_LessGreater)
	CASE(C_NotLessGreater)
	CASE(C_LessGreaterEqual)
	CASE(C_NotLessGreaterEqual)
#endif /*NO_NCEG*/
		isunsigned = 0;
		/*FALLTHROUGH*/
	CASE(C_InclusiveOR)
	CASE(C_ExclusiveOR)
	CASE(C_BitwiseAND)
	CASE(C_LeftShift)
	CASE(C_RightShift)
	CASE(C_Plus)
	CASE(C_Minus)
	CASE(C_Mult)
	CASE(C_Div)
	CASE(C_Mod)
		/* Binary Operators */
		t = ostp;
		ostp = ostp->next;
		t->left = nstp->next;
		t->right = nstp;
		if (isunsigned < 0)
		{
			if (t->code == C_LeftShift || t->code == C_RightShift)
			{
				/* shift signedness only depends on lhs */
				if (t->left->unsgnd)
					t->unsgnd = 1;
			}
			else if (t->left->unsgnd || t->right->unsgnd)
				t->unsgnd = 1;
		}
		t->next = nstp->next->next;
		nstp->next = nstp->next->next = 0;
		nstp = t;
		break;

	CASE(C_UnaryPlus)
		t = ostp;
		ostp = t->next;
		freetree(t);
		break;

	CASE(C_UnaryMinus)
	CASE(C_Complement)
	CASE(C_Not)
		t = ostp;
		ostp = t->next;
		t->left = nstp;
		t->next = nstp->next;
		nstp->next = 0;
		nstp = t;
		t->unsgnd = t->left->unsgnd;
		break;

	CASE(C_LParen)
		t = ostp;
		ostp = t->next->next;
		freetree(t->next);
		freetree(t);
		break;

#ifdef DEBUG
	default:
		fprintf(stderr, "Internal error: illegal code in reduce()\n");
		break;
#endif
	}
	expect = E_operator;
}

static Action
reducechk()
/* At this point, action() has decided the automaton should reduce.
** This routine, extending the error checking in action(),
** looks for syntax errors in the use of the paired operators:
** '(',')' and '?',':'. Some syntax errors are manifested in attempts
** to reduce on mismatched operator pairs.
** This routine will return A_error upon finding a syntax violation,
** else it will return A_reduce.
*/
{
	switch (ostp->code)
	{
	case C_LParen:
		if ( ostp->next && ostp->next->code == C_RParen )
			break;
		/*FALLTHRU*/
	case C_RParen:
		UERROR(gettxt(":536","mismatched parentheses"));
		return A_error;

	case C_Colon:
		if ( ostp->next && ostp->next->code == C_Question )
			break;
		/*FALLTHRU*/
	case C_Question:
		UERROR(gettxt(":537","mismatched \"?\" and \":\""));
		return A_error;
	}
	return A_reduce;
}

static void
shift()
/* This routine pushes the Tree representing the input Token on the correct 
** stack (either the number stack or the operator stack)
** and makes sure the next token is pointed to.
** Keeps track whether the next token is expected to be an operand.
*/
{
	Tree *tree;

	DBGCALL(1,shift,curtoken);
	tree = pp_malloc(sizeof(Tree));
	tree->left = 0;
	tree->right = 0;
	tree->unsgnd = 0;
	switch (input_op)
	{
	case C_Operand:
		primary(tree);
		tree->next = nstp;
		nstp = tree;
		expect = E_operator;
		return;
	case C_LParen:
		tree->code = C_RParen;
		expect = E_number;
		break;
	case C_RParen:
		tree->code = C_LParen;
		expect = E_operator;
		break;
	default:
		tree->code = input_op;
		expect = E_number;
		break;
	}
	tree->next = ostp;
	ostp = tree;
}

int
xp_value(tp)
	Token * tp;
/* Given a pointer to a Token list that comprises a
** pre-macro expansion constant expression
** in a conditional inclusion directive,
** This routine initializes and runs the automaton
** to evaluate the constant expression.
** It return false if expression evaluates to 0, else it returns true.
** It returns true on a syntax error (C Issue 4.1 compatible behavior).
*/
{
	static int first = 1;
	int retval;

	DBGCALLL(1,xp_value,tp);
	COMMENT(tp != 0);
	COMMENT(nstp == 0);
	COMMENT(ostp == 0);
	if (first) /* set up special big integer values */
	{
		first = 0;
		(void)num_fromulong(&num1, 1ul);
		num_complement(&numsmax);
		(void)num_lrshift(&numsmax, 1);
	}
	if ((curtoken = tk_rmws(ex_condexpr(tp))) == 0)
	{
		UERROR(gettxt(":538", "empty constant expression after macro expansion" ));
		DBGRETN(0,xp_value,1L); 
		return 1;
	}
	for(expect = E_number; ; )
	{
		switch (action())
		{
		case A_shift:
			shift();
			curtoken = tk_rmws(tk_rm(curtoken));
			continue;

		case A_reduce:
			reduce();
			continue;

		case A_accept:
			COMMENT(curtoken == 0);
			COMMENT(ostp == 0);
			COMMENT(nstp != 0 && nstp->next == 0);
			eval(nstp);
			retval = num_ucompare(&nstp->value, &num0);
			freetree(nstp);
			nstp = 0;
			DBGRETN(0,xp_value,(long)retval);
		    	return retval;

		case A_error:
			if (curtoken != 0)	tk_rml(curtoken);
			while (nstp != 0) {
				register Tree *t = nstp->next;
				freetree(nstp);
				nstp = t;
			}
			while (ostp != 0) {
				register Tree *t = ostp->next;
				freetree(ostp);
				ostp = t;
			}
			DBGRETN(0,xp_value,1L); 
			return 1; 
		}
	}
}
