#ident	"@(#)ksh93:src/cmd/ksh93/sh/streval.c	1.2"
#pragma prototyped
/*
 * G. S. Fowler
 * D. G. Korn
 * AT&T Bell Laboratories
 *
 * arithmetic expression evaluator
 *
 * NOTE: all operands are evaluated as both the parse
 *	 and evaluation are done on the fly
 */

#include	"streval.h"
#include	<ctype.h>
#include	<error.h>
#include	"FEATURE/externs"

#define MAXLEVEL	9

extern char	*gettxt();

struct vars			 /* vars stacked per invocation		*/
{
	const char*	nextchr; /* next char in current expression	*/
	const char*	errchr;	 /* next char after error		*/
	struct lval	errmsg;	 /* error message text			*/
	const char*	errstr;  /* error string			*/
	unsigned char	paren;	 /* parenthesis level			*/
	char		isfloat; /* set when floating number		*/
	char		wascomma;/* incremented by comma operator	*/
	double		ovalue;  /* value at comma operator		*/
};


#define getchr()	(*cur.nextchr++)
#define peekchr()	(*cur.nextchr)
#define ungetchr()	(cur.nextchr--)

#define pushchr(s)	{struct vars old;old=cur;cur.nextchr=((char*)(s));cur.errmsg.value=0;cur.errstr=0;cur.paren=0;
#define popchr()	cur=old;}
#define ERROR(msg)	return(seterror(msg))

static struct vars	cur;
static char		noassign;	/* set to skip assignment	*/
static int		level;
static double		(*convert)(const char**,struct lval*,int,double);
				/* external conversion routine		*/
static double		expr(int);	/* subexpression evaluator	*/
static double		seterror(const char[]);	/* set error message string	*/


/*
 * evaluate an integer arithmetic expression in s
 *
 * (double)(*convert)(char** end, struct lval* string, int type, double value)
 *     is a user supplied conversion routine that is called when unknown 
 *     chars are encountered.
 * *end points to the part to be converted and must be adjusted by convert to
 * point to the next non-converted character; if typ is ERRMSG then string
 * points to an error message string
 *
 * NOTE: (*convert)() may call strval()
 */

double strval(const char *s, char** end, double(*conv)(const char**,struct lval*,int,double))
{
	register double	n;
	int wasfloat;

	pushchr(s);
	cur.isfloat = 0;
	convert = conv;
	if(level++ >= MAXLEVEL)
		(void)seterror(gettxt(e_recursive_id,e_recursive));
	else
		n = expr(0);
	if (cur.errmsg.value)
	{
		if(cur.errstr) s = cur.errstr;
		(void)(*convert)( &s , &cur.errmsg, ERRMSG, n);
		cur.nextchr = cur.errchr;
		n = 0;
	}
	if (end) *end = (char*)cur.nextchr;
	if(level>0) level--;
	wasfloat = cur.isfloat;
	popchr();
	cur.isfloat |= wasfloat;
	return(n);
}

/*   
 * evaluate a subexpression with precedence
 */

static double expr(register int precedence)
{
	register int	c, op;
	register double	n, x;
	int		wasop, incr=0;
	struct lval	lvalue, assignop;
	const char	*pos;
	char		invalid=0;

	while ((c=getchr()) && isspace(c));
	switch (c)
	{
	case 0:
		if(precedence>5)
			ERROR(gettxt(e_moretokens_id,e_moretokens));
		return(0);

	case '-':
		incr = -2;
		/* FALL THRU */
	case '+':
		incr++;
		if(c != peekchr())
		{
			/* unary plus or minus */
			n = incr*expr(2*MAXPREC-1);
			incr = 0;
		}
		else /* ++ or -- */
		{
			invalid = 1;
			getchr();
		}
		break;

	case '!':
		n = !expr(2*MAXPREC-1);
		break;
	case '~':
	{
		long nl;
		n = expr(2*MAXPREC-1);
		nl = (long)n;
		if(nl != n)
			ERROR(gettxt(e_incompatible_id,e_incompatible));
		else
			n = ~nl;
		break;
	}
	default:
		ungetchr();
		invalid = 1;
		break;
	}
	wasop = invalid;
	lvalue.value = 0;
	lvalue.fun = 0;
	while(1)
	{
		cur.errchr = cur.nextchr;
		if((c=getchr()) >= sizeof(strval_states))
			op = (c=='|' ? A_OR: (c=='^'?A_XOR:A_REG));
		else if((op=strval_states[c])==0)
			continue;
		switch(op)
		{
			case A_EOF:
				ungetchr();
				break;
			case A_DIG:
#ifdef future
				n = c - '0';
				while((c=getchr()), isdigit(c))
					n = (10*n) + (c-'0');
				wasop = 0;
				ungetchr();
				continue;
#endif
			case A_REG:	case A_DOT:
				op = 0;
				break;
			case A_QUEST:
				if(*cur.nextchr==':')
				{
					cur.nextchr++;
					op = A_QCOLON;
				}
				break;
			case A_LT:	case A_GT: 
				if(*cur.nextchr==c)
				{
					cur.nextchr++;
					op -= 2;
					break;
				}
				/* FALL THRU */
			case A_NOT:	case A_COLON:
				c = '=';
				/* FALL THRU */
			case A_ASSIGN:
			case A_PLUS:	case A_MINUS:
			case A_OR:	case A_AND:
				if(*cur.nextchr==c)
				{
					cur.nextchr++;
					op--;
				}
		}
		assignop.value = 0;
		if(op && wasop++ && op!=A_LPAR)
			ERROR(gettxt(e_synbad_id,e_synbad));
		/* check for assignment operation */
		if(peekchr()== '=' && !(strval_precedence[op]&NOASSIGN))
		{
			if(!noassign && (!lvalue.value || precedence > 2))
				ERROR(gettxt(e_notlvalue_id,e_notlvalue));
			assignop = lvalue;
			getchr();
			c = 3;
		}
		else
		{
			c = (strval_precedence[op]&PRECMASK);
			c *= 2;
		}
		/* from here on c is the new precedence level */
		if(lvalue.value && (op!=A_ASSIGN))
		{
			if(noassign)
				n = 1;
			else
			{
				pos = cur.nextchr;
				n = (*convert)(&cur.nextchr, &lvalue, VALUE, n);
				if (cur.nextchr!=pos)
				{
					if(cur.errmsg.value = lvalue.value)
						cur.errstr = cur.nextchr;
					ERROR(gettxt(e_synbad_id,e_synbad));
				}
			}
			if(cur.nextchr==0)
				ERROR(gettxt(e_badnum_id,e_badnum));
			if(!(strval_precedence[op]&SEQPOINT))
				lvalue.value = 0;
			invalid = 0;
		}
		if(invalid && op>A_ASSIGN)
			ERROR(gettxt(e_synbad_id,e_synbad));
		if(precedence >= c)
			goto done;
		if(strval_precedence[op]&RASSOC)
			c--;
		if(c < 2*MAXPREC && !(strval_precedence[op]&SEQPOINT))
		{
			wasop = 0;
			x = expr(c);
		}
		if((strval_precedence[op]&NOFLOAT) && !noassign && cur.isfloat)
			ERROR(gettxt(e_incompatible_id,e_incompatible));
		switch(op)
		{
		case A_RPAR:
			if(!cur.paren)
				ERROR(gettxt(e_paren_id,e_paren));
			if(invalid)
				ERROR(gettxt(e_synbad_id,e_synbad));
			goto done;

		case A_COMMA:
			wasop = 0;
			cur.wascomma++;
			cur.ovalue = n;
			n = expr(c);
			lvalue.value = 0;
			break;

		case A_LPAR:
		{
			char	savefloat = cur.isfloat;
			double (*fun)(double,...);
                        int nargs = lvalue.nargs;
			fun = lvalue.fun;
			lvalue.fun = 0;
			cur.wascomma=0;
			cur.isfloat = 0;
			if(!invalid)
				ERROR(gettxt(e_synbad_id,e_synbad));
			cur.paren++;
			n = expr(1);
			cur.paren--;
			if(fun)
			{
				if(cur.wascomma+1 != nargs)
					ERROR(e_argcount);
				if(cur.wascomma)
					n = (*fun)(cur.ovalue,n);
				else
					n = (*fun)(n);
				cur.isfloat = ((void*)fun!=(void*)floor);
			}
			cur.isfloat |= savefloat;
			if (getchr() != ')')
				ERROR(gettxt(e_paren_id,e_paren));
			wasop = 0;
			break;
		}

		case A_PLUSPLUS:
			incr = 1;
			goto common;
		case A_MINUSMINUS:
			incr = -1;
		common:
			x = n;
			wasop=0;
		case A_ASSIGN:
			if(!noassign && !lvalue.value)
				ERROR(gettxt(e_notlvalue_id,e_notlvalue));
			n = x;
			assignop = lvalue;
			lvalue.value = 0;
			break;

		case A_QUEST:
			if(!n)
				noassign++;
			x = expr(1);
			if(!n)
				noassign--;
			if(getchr()!=':')
				ERROR(gettxt(e_questcolon_id,e_questcolon));
			if(n)
			{
				n = x;
				noassign++;
				(void)expr(c);
				noassign--;
			}
			else
				n = expr(c);
			lvalue.value = 0;
			wasop = 0;
			break;

		case A_COLON:
			(void)seterror(gettxt(e_badcolon_id,e_badcolon));
			break;

		case A_OR:
			n = (long)n | (long)x;
			break;

		case A_QCOLON:
		case A_OROR:
			if(n)
			{
				noassign++;
				expr(c);
				noassign--;
			}
			else
				n = expr(c);
			if(op==A_OROR)
				n = (n!=0);
			lvalue.value = 0;
			wasop=0;
			break;

		case A_XOR:
			n = (long)n ^ (long)x;
			break;

		case A_NOT:
			ERROR(gettxt(e_synbad_id,e_synbad));

		case A_AND:
			n = (long)n & (long)x;
			break;

		case A_ANDAND:
			if(n==0)
			{
				noassign++;
				expr(c);
				noassign--;
			}
			else
				n = (expr(c)!=0);
			lvalue.value = 0;
			wasop=0;
			break;

		case A_EQ:
			n = n == x;
			break;

		case A_NEQ:
			n = n != x;
			break;

		case A_LT:
			n = n < x;
			break;

		case A_LSHIFT:
			n = (long)n << (long)x;
			break;

		case A_LE:
			n = n <= x;
			break;

		case A_GT:
			n = n > x;
			break;

		case A_RSHIFT:
			n = (long)n >> (long)x;
			break;

		case A_GE:
			n = n >= x;
			break;

		case A_PLUS:
			n +=  x;
			break;

		case A_MINUS:
			n -=  x;
			break;

		case A_TIMES:
			n *=  x;
			break;

		case A_DIV:
			if(x!=0)
			{
				if(cur.isfloat)
					n /=  x;
				else
					n =  (long)n / (long)x;
				break;
			}

		case A_MOD:
			if(x!=0)
				n = (long)n % (long)x;
			else if(!noassign)
				ERROR(gettxt(e_divzero_id,e_divzero));
			break;

		default:
			if(!wasop)
				ERROR(gettxt(e_synbad_id,e_synbad));
			wasop = 0;
			pos = --cur.nextchr;
			lvalue.isfloat = 0;
			n = (*convert)(&cur.nextchr, &lvalue, LOOKUP, n);
			if (cur.nextchr == pos)
			{
				if(cur.errmsg.value = lvalue.value)
					cur.errstr = pos;
				ERROR(gettxt(e_synbad_id,e_synbad));
			}
			cur.isfloat |= lvalue.isfloat;
	
			/* check for function call */
			if(lvalue.fun)
				continue;
			/* this handles ++x and --x */
			if(!noassign && incr)
			{
				if(cur.isfloat)
					ERROR(gettxt(e_incompatible_id,e_incompatible));
				if(lvalue.value)
				{
					pos = cur.nextchr;
					n = (*convert)(&cur.nextchr, &lvalue, VALUE, n);
					if (cur.nextchr!=pos)
					{
						if(cur.errmsg.value = lvalue.value)
							cur.errstr=cur.nextchr;
						ERROR(gettxt(e_synbad_id,e_synbad));
					}
				}
				n += incr;
				incr = 0;
				goto common;
			}
			break;
		}
		invalid = 0;
		if(!noassign && assignop.value)
			{
			/*
			 * Here the output of *convert must be reassigned to n
			 * in case a cast is done in *convert.
			 * The value of the increment must be subsequently
			 * subtracted for the postincrement return value
			*/
			n=(*convert)(&cur.nextchr,&assignop,ASSIGN,n+incr);
			n -= incr;
		}
		incr = 0;
	}
 done:
	cur.nextchr = cur.errchr;
	return(n);
}

/*
 * set error message string
 */

static double seterror(const char *msg)
{
	if(!cur.errmsg.value)
		cur.errmsg.value = ERROR_translate(msg,0);
	cur.errchr = cur.nextchr;
	cur.nextchr = "";
	level = 0;
	return(0);
}

#ifdef _mem_name_exception
#undef error
    int matherr(struct exception *ep)
    {
	const char *message;
	switch(ep->type)
	{
	    case DOMAIN:
		message = gettxt(e_domain_id,e_domain);
		break;
	    case SING:
		message = gettxt(e_singularity_id,e_singularity);
		break;
	    case OVERFLOW:
		message = gettxt(e_overflow_id,e_overflow);
		break;
	    default:
		return(1);
	}
	level=0;
	error(ERROR_exit(1),message,ep->name);
	return(0);
    }
#endif /* _mem_name_exception */
