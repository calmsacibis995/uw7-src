/*	copyright	"%c%"	*/

#ident	"@(#)expr:expr.c	1.21.4.6"
#ident "$Header$"

#include  <stdlib.h>
#include  <regex.h>
#include  <locale.h>
#include  <pfmt.h>
#include  <string.h>
#include  <stdio.h>
#include  <limits.h>
#include  <ctype.h>
#include  "expr.h"

long atol();
char *ltoa(), *strcpy();
char	*expres();
void 	exit();
char	**Av;
int	Ac;
int	Argi;
int noarg;
int paren;

const char badsyn[] = ":233:Syntax error\n";
const char zerodiv[] = ":234:Division by zero\n";

static int
get1num(s, v)
char *s;
long *v;
{
	char *ep;

	if (!isdigit(s[0])) {
		if (s[0] != '-' || !isdigit(s[1]))
			return 0;
	}
	*v = strtol(s, &ep, 10);
	return *ep == '\0';
}

static int
get2num(r1, r2, v1, v2)
char *r1, *r2;
long *v1, *v2;
{
	if (get1num(r1, v1) == 0)
		return 0;
	return get1num(r2, v2);
}

char *rel(oper, r1, r2) register char *r1, *r2; 
{
	long i1, i2;

	if(get2num(r1, r2, &i1, &i2))
		i1 -= i2;
	else
		i1 = strcoll(r1, r2);
	switch(oper) {
	case EQ: 
		i1 = i1==0; 
		break;
	case GT: 
		i1 = i1>0; 
		break;
	case GEQ: 
		i1 = i1>=0; 
		break;
	case LT: 
		i1 = i1<0; 
		break;
	case LEQ: 
		i1 = i1<=0; 
		break;
	case NEQ: 
		i1 = i1!=0; 
		break;
	}
	return i1? "1": "0";
}

char *arith(oper, r1, r2) char *r1, *r2; 
{
	long i1, i2;
	register char *rv;

	if(!get2num(r1, r2, &i1, &i2))
		yyerror(":235:Non-numeric argument\n");

	switch(oper) {
	case ADD: 
		i1 = i1 + i2; 
		break;
	case SUBT: 
		i1 = i1 - i2; 
		break;
	case MULT: 
		i1 = i1 * i2; 
		break;
	case DIV: 
		if (i2 == 0)
			yyerror(zerodiv);
		i1 = i1 / i2; 
		break;
	case REM: 
		if (i2 == 0)
			yyerror(zerodiv);
		i1 = i1 % i2; 
		break;
	}
	return ltoa(i1);
}
char *conj(oper, r1, r2) char *r1, *r2; 
{
	register char *rv;

	switch(oper) {

	case OR:
		if(EQL(r1, "0") || EQL(r1, ""))
			rv = r2;
		else
			rv = r1;
		break;
	case AND:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			rv = "0";
		else if(EQL(r2, "0")
		    || EQL(r2, ""))
			rv = "0";
		else
			rv = r1;
		break;
	}
	return rv;
}

char *match(s, p)
char *s, *p;
{
	static regex_t re;
	static int gotre;
	regmatch_t rm[2];
	char msg[256];
	size_t n;
	char *q;
	int err;

	if(*p == '\0') {
		if(!gotre)
			yyerror(":199:No remembered search string"); /*no \n*/
	} else {
		if(gotre) {
			gotre = 0;
			regfree(&re);
		}
		if(*p != '^') {
			q = malloc(1 + strlen(p) + 1);
			q[0] = '^';
			strcpy(&q[1], p);
			p = q;
		}
		if((err = regcomp(&re, p, REG_ANGLES | REG_ESCNL)) != 0) {
	badre:;
			regerror(err, &re, msg, sizeof(msg));
			pfmt(stderr, MM_ERROR, ":1230:RE error: %s\n", msg);
			exit(2);
		}
		gotre = 1;
	}
	if((err = regexec(&re, s, (size_t)2, &rm[0], 0)) == REG_NOMATCH) {
		if(re.re_nsub != 0)
			return "";
		return "0";
	}
	if(err != 0)
		goto badre;
	if(re.re_nsub != 0) {
		n = rm[1].rm_eo - rm[1].rm_so;
		q = malloc(n + 1);
		memcpy(q, &s[rm[1].rm_so], n);
		q[n] = '\0';
		return q;
	}
	return ltoa((long)rm[0].rm_eo);	/* rm[0].rm_so must be 0 */
}

yyerror(s)
char *s;
{
	pfmt(stderr, MM_ERROR, s);
	if(strchr(s, '\n') == 0)
		putc('\n', stderr);
	exit(2);
}

char *ltoa(l)
long l;
{
	char num[1 + (sizeof(long) * CHAR_BIT + 2) / 3 + 1];

	(void)sprintf(num, "%ld", l);
	return strdup(num);
}

int
main(argc, argv) char **argv; 
{
	char *ans;
	size_t n;
	long val;

	Ac = argc;
	Argi = 1;
	noarg = 0;
	paren = 0;
	Av = argv;
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:expr");

	if (strcmp(argv[1], "--") == 0) {
		/* POSIX.2 says this is still a separator
		 * even though expr has no options to separate!
		 */
		Argi++;
	}
	/* POSIX.2 wants anything that looks like it could be a number
	 * to be taken that way.  There's no problem with our choice
	 * that requires it to be forced, except when there's only one
	 * arg and it's a number with leading zeroes.
	 */
	if (Argi + 1 == Ac && get1num(argv[Argi], &val)) {
		Argi++;
		ans = ltoa(val);
	} else {
		ans = expres(0,1);
	}
	if(Ac != Argi || paren != 0) {
		yyerror(badsyn);
	}
	if ((n = strlen(ans)) != 0) {
		(void)write(1, ans, n);
		(void)write(1, "\n", 1);
		if (strcmp(ans, "0") != 0)
			return 0;
	}
	return 1;
}
