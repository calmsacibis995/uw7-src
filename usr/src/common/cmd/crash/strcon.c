#ident	"@(#)crash:common/cmd/crash/strcon.c	1.1.2.1"
/* based on blf's OS5 crash/eval.c 25.4 94/12/06 */

/*
 * Integral expression evaluator.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/vmparam.h>
#include <sys/var.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include "crash.h"

extern int errno;

/*
 * Most C operators are provided, with similar precedence and
 * associativity to ANSI C.  The logical operators (!, <, <=,
 * ==, !=, >, and >=) are not provided; note that ! could then
 * be confused with crash's pipe historical command.  (crash now
 * alsos recognize | for pipes; there is no danger of confusion
 * with the binary inclusive-bitwise-OR operator).  An additional
 * binary operator is provided:  A#B  is A rounded up to the next
 * multiple of B.  (There is no danger of confusion with the #
 * this-is-a-PID pseudo-operator recognized by some commands.)
 *
 * 26 saved values named $a..$z are provided.  The special
 * saved value $$ is the last sucessfully evaluated expression.
 *
 * This is mostly compatible with crash(ADM)s previous braindead
 * expression evalutor, which was only ``( value binop value )''
 * where the parens where required, <value> was a number or symbol,
 * and <binop> was one of + - & | * /.
 *
 *	expr	::=	assign \0
 *
 *	assign	::=	ior
 *			$ letter = assign
 *			$ letter += assign
 *			$ letter -= assign
 *			$ letter *= assign
 *			$ letter /= assign
 *			$ letter #= assign	(see  A # B  below)
 *			$ letter %= assign
 *			$ letter &= assign
 *			$ letter |= assign
 *			$ letter ^= assign
 *			$ letter <<= assign
 *			$ letter >>= assign
 *
 *	ior	::=	xor
 *			xor | ior
 *
 *	xor	::=	and
 *			and ^ xor
 *
 *	and	::=	shift
 *			shift & and
 *
 *	shift	::=	add
 *			add << shift
 *			add >> shift
 *
 *	add	::=	multi
 *			multi + add
 *			multi - add
 *
 *	multi	::=	unary
 *			unary * multi
 *			unary / multi
 *			unary % multi
 *			unary # multi	A # B  is A rounded up to the next
 *					     multiple of B -- see adb(CP).
 *	unary	::=	term
 *			- unary
 *			~ unary
 *			& unary		*CAUTION* a no-op (for historical comp)
 *			* unary		*CAUTION* always dereferences a long
 *
 *	term	::=	identifer
 *			number
 *			$ letter
 *			( assign )
 *
 * Also unlike C, identifiers are always treated as pointers rather than
 * rvals; i.e., "foo" doesn't mean "the value stored at location foo" but
 * "the value of foo itself; i.e., foo's address".  Use "*foo" to get the
 * (longword) value stored at the location whose address is foo's value.
 * This is for historical compatibility with crash's previous expressions.
 */

static char	exprterminals[]	= "$=.|^&<>+-*/%#~() \t";

static char	*cursor;		/* current position in string	*/
static char	*oldcursor;		/* beginning of current token	*/

static unsigned	default_radix;		/* default input number radix	*/

/*
 * These really belong to strcon.c, but symtab.c wants to look at them,
 * and symtab.o is more likely to be wanted in a crash nucleus library
 */
extern long	savevalue[27];		/* $$ and $letter saved values	*/

/*forward*/ static long	expr(void);
/*forward*/ static long	assign(void);
/*forward*/ static long strtonum(char *);
/*forward*/ static int  pidleproc(char *);

/* convert command argument to number */
long
strcon(char *cp, char default_format)
{
	switch (default_format) {
	case 'h':	default_radix = 16; break;
	case 'o':	default_radix =  8; break;
	case 'b':	default_radix =  2; break;
	case 'd':
	default :	default_radix = 10; break;
	}
	cursor = cp;
	return (savevalue[0] = expr());
}

/* convert possibly physical address argument to unsigned long long number */
phaddr_t
strcon64(char *cp, int phys)
{
	extern unsigned long long strtoull();	/* not yet in stdlib.h */
	long toplong;
	phaddr_t num;
	char *extra;

	/*
	 * Sorry, this support for 36-bit physical addresses lies
	 * outside all the expression handling for 32-bit values:
	 * I hesitate to interfere with the rest of the module.
	 */

	if (phys && pae) {
		errno = 0;
		num = strtoull(cp, &extra, 16);
		if (extra > cp && *extra == '\0') {
			toplong = highlong(num);
			if (errno || (toplong >> 4))
				error("%s does not fit into 36 bits\n", cp);
			if (!toplong)
				savevalue[0] = (long)num;
			return num;
		}
	}

	default_radix = 16;
	cursor = cp;
	savevalue[0] = expr();
	return (phaddr_t)((unsigned long)savevalue[0]);
}

/* convert args[optind] to number, or range pair if "a-b" */
boolean_t
getargs(long limit, long *begin, long *end)
{
	char decimalbuf[11];
	char default_format;
	char *string, *process, *specp;
	long savevaluesave;
	int n;

	string = args[optind];
	if (limit == PROCARGS) {
		/* special handling for "#pid" or "idle" */
		if ((*begin = pidleproc(string)) != PROCARGS) {
			*end = *begin;
			return B_TRUE;	/* though may be -1 for idle */
		}
		limit = vbuf.v_proc;
		process = "process ";
	}
	else
		process = "";

	specp = strpbrk(string, exprterminals + 2 /* skip $= */);
	n = specp - string;	/* yes, specp may be null */
	if (n <= 0 || n >= sizeof(decimalbuf))
		n = 0;
	else if (*specp == '.' && *(specp + 1) == '.') {
		/* allow but don't encourage OS5's ".." for range */
		specp += 2;
		if (*specp == '\0'
		||  strpbrk(specp, exprterminals + 2) != NULL)
			n = 0;
	}
	else if (*specp++ != '-' || *specp == '\0'
	||   strpbrk(specp, exprterminals + 2) != NULL)
		n = 0;

	/*
	 * getpage() uses getargs() with limit MAX_PFN+1,
	 * and displays page frame numbers in hexadecimal.
	 */
	default_format = (hexmode || limit >= 0x10000)? 'h': 'd';

	if (n == 0) {
		/*
		 * Either simpler or more complicated than "n-m",
		 * so treat this as one argument via the usual
		 * expression processing, not as a range pair.
		 * But should it default to hex or to decimal??
		 * Most callers of getargs() expect either a slot
		 * number (usually decimal, but hex is better for
		 * getpage()) or an address (usually hex): ugh!!
		 * But at least none of the getargs() callers are
		 * now using it for a physical or process address:
		 * so we can reject values between limit & kvbase.
		 * Try hex first because more chars valid in hex.
		 */
		savevaluesave = savevalue[0];
		*end = *begin = strcon(string,'h');
		if ((vaddr_t)(*begin) >= kvbase)
			return B_FALSE;	/* maybe good, but not in the range */
		if (default_format == 'd') {
			savevalue[0] = savevaluesave;
			*end = *begin = strcon(string,'d');
		}
		if (*begin >= 0 && *begin < limit)
			return B_TRUE;
		if (limit <= 0)
			error("table of size %d has no entries\n", limit);
		error((default_format == 'd')?
			"%s%d is out of range 0-%d\n":
			"%s%x is out of range 0-%x\n",
			process, *begin, limit-1);
	}

	strncpy(decimalbuf, string, n)[n] = '\0';
	*begin = strcon(decimalbuf,default_format);
	*end = strcon(specp,default_format);
	if (*begin > *end)
		error("%s is an invalid range\n",string);
	if (*end >= limit)
		*end = limit - 1;
	if (*begin >= 0 && *begin < limit)
		return B_TRUE;
	if (limit <= 0)
		error("table of size %d has no entries\n", limit);
	prerrmes("%s%s is out of range 0-", process, decimalbuf);
	error((default_format == 'd')? "%d\n": "%x\n", limit-1);
}

/* validate and return process slot input argument */
int
valproc(char *string)
{
	int slot;

	if ((slot = pidleproc(string)) != PROCARGS)
		return slot;
	slot = strcon(string,'d');
	if (slot < 0 || slot >= vbuf.v_proc)
		error("process %d is out of range 0-%d\n",
			slot, vbuf.v_proc - 1);
	return slot;
}

/* setproc: valproc on optarg */
int
setproc(void)
{
	return valproc(optarg);
}

/*
 * Internal functions
 */

/* check process argument for "#pid" or "idle" */
static int
pidleproc(char *string)
{
	pid_t pid;
	int slot;

	if (*string == '#') {
		pid = strcon(string+1,'d');
		if ((slot = pid_to_slot(pid)) < 0)
			error("%d is not a valid process id\n", pid);
		return slot;
	}
	if (strcmp(string, "idle") == 0)
		return -1;
	return	PROCARGS;
}

static char
slurp(void)
{
	while (*cursor && isspace(*cursor))
		++cursor;
	oldcursor = cursor;
	return *cursor;
}

/*
 * Convert a letter into a savevalue[]'s value and index.
 *
 * N.b.	Do _not_ use islower(S)!  In non-English locales,
 *	other char values may be considered letters, which
 *	could result in an out-of-bounds savevalue[] index.
 *	This algorithm does, of course, assume ASCII .....
 */
static long
letter(char c, int *pindex)
{
	if ('a' <= c && c <= 'z') {
		*pindex = c - 'a' + 1;
		return savevalue[*pindex];
	}
	if (c == '$') {
		*pindex = 0;
		return savevalue[0];
	}
	error("$%c is not a saved value\n", c?c:' ');
	/* NOTREACHED */
}

static long
term(void)
{
	char		buf[LINESIZE];
	char		*cp;
	long		num;
	unsigned	n;
	int		junk;

	switch (slurp()) {
	case '(':
		cursor++;
		num = assign();
		if (slurp() != ')') {
			error("missing )\n");
			/* NOTREACHED */
		}
		cursor++;
		break;
	case '$':
		cursor++;
		num = letter(slurp(), &junk);
		cursor++;
		break;
	default:
		/* allow anomalous symbols ".apic_common" and ".dev_common" */
		cp = strpbrk(cursor + (*cursor == '.'), exprterminals);
		if (cp == NULL)
			n = strlen(cursor);
		else
			n = cp - cursor;
		strncpy(buf, cursor, n)[n] = '\0';
		if ((cp = strchr(buf, ':')) != NULL) {
			*cp++ = '\0';
			num = structfind(buf, cp)->offset;
		}
		else
			num = strtonum(buf);
		cursor += n;
		break;
	case '\0':
		error("unexpected end of expression\n");
		/* NOTREACHED */
	}
	/* do .fieldname here to get right precedence on *l.procp */
	while (slurp() == '.') {
		cp = strpbrk(++cursor, exprterminals);
		if (cp == NULL)
			n = strlen(cursor);
		else
			n = cp - cursor;
		strncpy(buf, cursor, n)[n] = '\0';
		if ((cp = strchr(buf, ':')) != NULL) {
			*cp++ = '\0';
			num += structfind(buf, cp)->offset;
		}
		else
			num += structfind("", buf)->offset;
		cursor += n;
	}
	return num;
}

static long
unary(void)
{
	char	*refcursor;
	long	addr;

	switch (slurp()) {
	case '-':	cursor++;	return -unary();
	case '~':	cursor++;	return ~unary();
	case '&':	cursor++;	return  unary();
	case '*':	refcursor = cursor++;
			readmem(unary(), 1, Cur_proc,
				&addr, sizeof(addr), refcursor);
			return addr;
	default:	return term();
	}
}

static long
multi(void)
{
	long	lhs, rhs;
	char	c;
	char	*divcursor;

	lhs = unary();
	switch (c = slurp()) {
	case '*':
		cursor++;
		lhs *= multi();
		break;
	case '/':
	case '%':
	case '#':
		divcursor = ++cursor;
		if ((rhs = multi()) == 0) {
			oldcursor = divcursor;
			error("division by 0\n");
			/* NOTREACHED */
		}
		switch (c) {
		case '/':	lhs /= rhs;				break;
		case '%':	lhs %= rhs;				break;
		case '#':	lhs = ((lhs + rhs - 1)/rhs)*rhs;	break;
		}
		break;
	}
	return lhs;
}

static long
add(void)
{
	long	num;

	num = multi();
	switch (slurp()) {
	case '+':	cursor++;	num += add();	break;
	case '-':	cursor++;	num -= add();	break;
	}
	return num;
}

static long
shift(void)
{
	long	num;

	num = add();
	switch (slurp()) {
	case '<':
		if (*++cursor != '<') {
			error("missing <\n");
			/* NOTREACHED */
		}
		cursor++;
		num <<= shift();
		break;
	case '>':
		if (*++cursor != '>') {
			error("missing >\n");
			/* NOTREACHED */
		}
		cursor++;
		num >>= shift();
		break;
	}
	return num;
}

static long
and(void)
{
	long	num;

	num = shift();
	if (slurp() == '&') {
		cursor++;
		num &= and();
	}
	return num;
}

static long
xor(void)
{
	long	num;

	num = and();
	if (slurp() == '^') {
		cursor++;
		num ^= xor();
	}
	return num;
}

static long
ior(void)
{
	long	num;

	num = xor();
	if (slurp() == '|') {
		cursor++;
		num |= ior();
	}
	return num;
}

static long
assign(void)
{
	long	lval, rval;
	int	n;
	char	oper;
	char	*dolcursor;

	if (slurp() != '$')
		return ior();

	dolcursor = cursor++;
	lval = letter(slurp(), &n);
	cursor++;
	oper = slurp();
	switch (oper) {
	case '<':
	case '>':
		if (*(cursor+1) != oper) {
			cursor = dolcursor;
			return ior();
		}
		cursor++;
		/* FALLTHRU */
	case '+':
	case '-':
	case '*':
	case '/':
	case '#':
	case '%':
	case '|':
	case '&':
	case '^':
		if (*(cursor+1) == '=') {
			/* FALLTHRU */
	case '=':					/* Duff's device */
			break;
		}
		/* FALLTHRU */
	default:
		cursor = dolcursor;
		return ior();
	}
	/* allow assignment to $$: it does no harm */
	cursor++;
	if (oper == '=')
		lval = assign();
	else {
		dolcursor = ++cursor;
		rval = assign();
		switch (oper) {
		case '/':
		case '%':
		case '#':
			if (rval == 0) {
				oldcursor = dolcursor;
				error("division by 0\n");
				/* NOTREACHED */
			}
			break;
		}
		switch (oper) {
		case '+':	lval += rval;				break;
		case '-':	lval -= rval;				break;
		case '*':	lval *= rval;				break;
		case '/':	lval /= rval;				break;
		case '%':	lval %= rval;				break;
		case '#':	lval = ((lval + rval - 1)/rval)*rval;	break;
		case '&':	lval &= rval;				break;
		case '|':	lval |= rval;				break;
		case '^':	lval ^= rval;				break;
		case '<':	lval <<= rval;				break;
		case '>':	lval >>= rval;				break;
		default:
			error("INTERNAL ERROR - Unknown operator %c\n", oper);
			/* NOTREACHED */
		}
	}
	return (savevalue[n] = lval);
}

static long
expr(void)
{
	long	num;

	num = assign();
	if (slurp()) {
		error("junk after expression\n");
		/* NOTREACHED */
	}
	return num;
}

/*
 * string to number conversion 
 */
static long
strtonum(register char *s)
{
	int	radix;
	char	*extra;
	char	*emesg;
	long	num;
	int	sym_legal;
	struct  syment *sp;

	if (*s == '0') {
		switch (*++s) {
		case '\0':
			return 0;
		case 'X':
		case 'x':
			radix = 16;
			s--;		/* let strtoul(S) deal with 0x... */
			break;
		case 'T':		/* like adb(CP) */
		case 't':		/* like adb(CP) */
		case 'D':
		case 'd':
			radix = 10;
			s++;
			break;
		case 'B':
		case 'b':
			radix =  2;
			s++;
			break;
		default:
			radix = hexmode? 16: 8;	/* who needs octal?? */
			s--;		/* let strtoul(S) deal with 0... */
			break;
		}

		sym_legal = 0;
		if (!isxdigit(*s))	/* no "leading" signs or whitespace */
			goto illegal;
	}
	else if (*s != '\0') {
		sym_legal = !isdigit(*s);
		radix = hexmode? 16: default_radix;
	}
	else {
		sym_legal = 1;
		goto illegal;
	}

	errno = 0;
	num = strtoul(s, &extra, radix);

	if (extra > s) {
		switch (*extra) {	/* ought to check for wrapping... */
		case 'M': case 'm':	num *= 1024;		/* FALLTHRU */
		case 'K': case 'k':	num *= 1024;	extra++;	break;
		case 'L': case 'l':	num *=    4;	extra++;	break;
		case 'W': case 'w':	num *=    2;	extra++;	break;
		}
		if (*extra == '\0') {
			if (errno)
				error("%s does not fit into 32 bits\n", s);
			return num;
		}
	}

	/*
	 * Note: so that we can leave this symsrch() until
	 * after strtoul() has failed, we demand that there be
	 * no symbols ambiguous with hex: symtab.c prefixes
	 * any such symbols with a semi-colon, e.g. ";fd"
	 */
	if (sym_legal && (sp = symsrch(s)) != NULL)
		return sp->n_value;

illegal:
	switch (radix) {
	case 16:	emesg = " hex";		break;
	case 10:	emesg = " decimal";	break;
	case  8:	emesg = "n octal";	break;
	case  2:	emesg = " binary";	break;
	default:	emesg = "";		break;
	}
	error(sym_legal?
		"%s is neither a%s number nor found in symbol table\n":
		"%s is not a%s number\n", s, emesg);
}
