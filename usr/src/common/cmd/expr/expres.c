/*	copyright	"%c%"	*/

#ident	"@(#)expr:expres.c	1.2"
#ident  "$Header$"

#include "expr.h"

extern	int	noarg;
extern	int	paren;
extern	char	**Av;
extern	int	Ac;
extern	int	Argi;

extern	const	char	badsyn[];
extern	const	char	zerodiv[];

char *operator[] = { 
	"|", "&", "+", "-", "*", "/", "%", ":",
	"=", "==", "<", "<=", ">", ">=", "!=",
	"match", 
	/* Enhanced Application Compatibility */
	"substr", "length", "index",
	"\0" };

int op[] = { 
	OR, AND, ADD,  SUBT, MULT, DIV, REM, MCH,
	EQ, EQ, LT, LEQ, GT, GEQ, NEQ,
	MATCH,
	/* Enhanced Application Compatibility */
	SUBSTR, LENGTH, INDEX
	};

int pri[] = {
	1,2,3,3,3,3,3,3,4,4,5,5,5,6,7,8,9,9};

char	*conj();
char	*rel();
char	*arith();
char	*match();
char	*index();
char	*length();
char	*substr();

char *expres(prior,par)  int prior, par; 
{
	int ylex, op1;
	int temp = 0;
	char *ra, *rb, *rc;
	char *r1 = (char *)0;

	ylex = yylex();
	if (ylex >= NOARG && ylex < MATCH) {
		yyerror(badsyn);
	}
	if (ylex == A_STRING) {
		r1 = Av[Argi++];
		temp = Argi;
	}
	else {
		if (ylex == '(') {
			paren++;
			Argi++;
			r1 = expres(0,Argi);
			Argi--;
		}
	}
lop:
	ylex = yylex();
	if (ylex > NOARG && ylex < MATCH) {
		op1 = ylex;
		Argi++;
		if (pri[op1-OR] <= prior ) 
			return r1;
		else {
			switch(op1) {
			case OR:
			case AND:
				r1 = conj(op1,r1,expres(pri[op1-OR],0));
				break;
			case EQ:
			case LT:
			case GT:
			case LEQ:
			case GEQ:
			case NEQ:
				r1=rel(op1,r1,expres(pri[op1-OR],0));
				break;
			case ADD:
			case SUBT:
			case MULT:
			case DIV:
			case REM:
				r1=arith(op1,r1,expres(pri[op1-OR],0));
				break;
			case MCH:
				r1=match(r1,expres(pri[op1-OR],0));
				break;
			}
			if(noarg == 1) {
				return r1;
			}
			Argi--;
			goto lop;
		}
	}
	ylex = yylex();
	if(ylex == ')') {
		if(par == Argi) {
			yyerror(badsyn);
		}
		if(par != 0) {
			paren--;
			Argi++;
		}
		Argi++;
		return r1;
	}
	ylex = yylex();
	if(ylex > MCH && ylex <= INDEX) {
		if (Argi == temp) {
			return r1;
		}
		op1 = ylex;
		Argi++;
		switch(op1) {
	/* Enhanced Application Compatibility */
		case SUBSTR:
			rc = expres(pri[op1-OR],0);
		case MATCH:
		case INDEX:
			rb = expres(pri[op1-OR],0);
		case LENGTH:
			ra = expres(pri[op1-OR],0);
			break;
	/* End Enhanced Application Compatibility */
		}
		switch(op1) {
		case MATCH: 
			r1 = match(rb,ra); 
			break;
	/* Enhanced Application Compatibility */
		case INDEX:
			r1 = index(rb,ra); 
			break;
		case SUBSTR:
			r1 = substr(rc,rb,ra); 
			break;
		case LENGTH:
			r1 = length(ra); 
			break;
	/* End Enhanced Application Compatibility */
		}
		if(noarg == 1) {
			return r1;
		}
		Argi--;
		goto lop;
	}
	ylex = yylex();
	if (ylex == NOARG) {
		noarg = 1;
	}
	return r1;
}

yylex() {
	register char *p;
	register i;

	if(Argi >= Ac) return NOARG;

	p = Av[Argi];

	if((*p == '(' || *p == ')') && p[1] == '\0' )
		return (int)*p;
	for(i = 0; *operator[i]; ++i)
		if(EQL(operator[i], p))
			return op[i];


	return A_STRING;
}

/* Enhanced Application Compatibility */
char *
substr(v, s, w)
char *v, *s, *w;
{
	register si, wi;
	register char *res;

	si = atol(s);
	wi = atol(w);
	while(--si) if(*v) ++v;

	res = v;
	
	while(wi--) if(*v) ++v;
	*v ='\0';
	return res;	
}

extern char *ltoa();

char *
length(s)
register char *s;
{
	register i = 0;
	register char *rv;

	while(*s++) ++i;

	return ltoa((long)i);
}

char *
index(s, t)
char *s, *t;
{
	register i, j;
	register char *rv;

	for(i = 0; s[i]; ++i)
		for(j = 0; t[j]; ++j)
			if(s[i]==t[j])
				return ltoa((long)++i);
	return "0";
}

/* End Enhanced Application Compatibility */
