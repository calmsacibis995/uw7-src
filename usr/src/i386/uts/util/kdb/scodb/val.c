#ident	"@(#)kern-i386:util/kdb/scodb/val.c	1.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * Modification History:
 *
 *	L000	scol!nadeem	12may92
 *	- added support for BTLD.  Use the sent_name() function to access
 *	  the name field of a symbol table entry rather than accessing
 *	  it directly (see sym.c).
 *	L001	scol!nadeem	11jun92
 *	- parse breakpoint names of the form #name correctly (see bkp.c for
 *	  full comment).  Call new function bplookup_hashname() instead
 *	  of bplookup().
 *	L002	scol!nadeem	1dec93
 *	- modification for getting "@<number>" operator to work correctly with
 *	  USL compiler output.  Now correctly returns the arguments of the
 *	  current function at a point before the "pushl %ebp" and
 *	  "movl %ebp,%esp" instructions have been executed.  Similar changes
 *	  applied to stack backtrace code in stack.c.
 *	L003	nadeem@sco.com	13dec94
 *	- add support for user level scodb.
 *	- don't allow evaluation of breakpoint addresses (eg: #rdwr) or 
 *	  of any kernel function calls (eg: _cr3()).  However, a limited
 *	  set of kernel function calls which are emulated internally
 *	  (eg: ps()) are allowed.
 *	- fixed generic bug in f_memv().  If we are evaluating an expression
 *	  which contains a pointer variable, and we fail to obtain the value 
 *	  of the pointer variable, then a garbage value was being returned
 *	  rather than an error message.  Fixed by making f_memv() look at the
 *	  return value passed back from drfv() when evaluating pointers.
 */

/*
*	The C operators, from the C bible, p. 49:
*
*	===============================================================
*	Level	Operator				Associativity
*	---------------------------------------------------------------
*	0F	()  []  ->  .					L->R
*	0E	!   ~   ++  --   + -  (type)  *  &  sizeof	R->L
*	0D	*   /   %					L->R
*	0C	+   -						L->R
*	0B	<<  >>						L->R
*	0A	<   <=  >   >=					L->R
*	09	==  !=						L->R
*	08	&						L->R
*	07	^						L->R
*	06	|						L->R
*	05	&&						L->R
*	04	||						L->R
*	03	?:						R->L
*	02	=   +=  -=  etc.				R->L
*	01	,						L->R
*	===============================================================
*
*	?: is not implemented here.
*	Functions are named uniformly f_0X	where X == 01-0F
*	depending on the level of syntax (see above table) the function
*	is implementing.
*/
#include	"sys/reg.h"
#include	<syms.h>
#include	"dbg.h"
#include	"histedit.h"
#include	"stunv.h"
#include	"sent.h"
#include	"dcl.h"
#include	"val.h"
#include	"bkp.h"
#include	"name.h"

/*
*	for debugging the [e]valuator
*/
#ifdef SCODBDEBUG /* NOT! */
NOTSTATIC int va_db = 0;
# define	FP(arg)		(va_db ? printf arg : 0)
#else
# define	FP(arg)
#endif

NOTSTATIC char *scodb_error2	= 0;
NOTSTATIC char *scodb_error	= 0;
STATIC int incsize = 0;	/* incsize? ++a== a+sizeof(a) : ++a==a+1 */
STATIC int f_eval;
STATIC int def_seg;

extern int sregs[];
extern int *REGP;
extern int scodb_nvar;
extern struct value scodb_var[];

/*
*	error strings
*/
STATIC    char *e_not_val	=	"not a value";
STATIC    char *e_exp_expr	=	"expected expression";
STATIC    char *e_exp_rpr	= /*(*/	"expected )";
STATIC    char *e_exp_quote	= 	"expected \"";
STATIC    char *e_ill_cast	=	"illegal cast";
STATIC    char *e_tm_args	=	"too many arguments";
STATIC    char *e_exp_comma	=	"expected comma";
STATIC    char *e_not_fcn	=	"not a function";
STATIC    char *e_exp_rbr	= /*[*/	"expected ]";
STATIC    char *e_ill_drf	=	"illegal dereferencing";
STATIC    char *e_ill_arr	=	"subscript on non-array";
STATIC    char *e_not_stun	=	"not a structure/union";
STATIC    char *e_not_elem	=	"not structure/union element";
STATIC    char *e_not_lval	=	"not an lvalue";
STATIC    char *e_not_addr	=	"not an addressable object";
STATIC    char *e_cant_drf	=	"can't dereference - bad address";
STATIC    char *e_cant_asgn	=	"can't complete assignment - bad address";
STATIC    char *e_bad_reg	=	"bad register name";
STATIC    char *e_extr_chars	=	"extra characters after expression";
STATIC    char *e_mult_seg	=	"multiple segment assignment";
STATIC    char *e_seg_lng	=	"segment value too long";
STATIC    char *e_cantf_sym	=	"can't find symbol";
STATIC    char *e_unk_bkp	=	"unknown breakpoint";
STATIC    char *e_unk_var	=	"unknown variable";
STATIC    char *e_exp_argn	=	"expected argument number";
NOTSTATIC char *e_bad_cast	=	"bad cast";
STATIC    char *e_bad_char	=	"bad character constant";
NOTSTATIC char *e_stun_unk	=	"structure type not known";
NOTSTATIC char *e_stack_ovr	=	"stack overflow danger";
NOTSTATIC char *e_cst_unmb	=	"unmatched [ in cast";/*]*/
NOTSTATIC char *e_cst_unmp	=	"unmatched ( in cast";/*)*/
NOTSTATIC char *e_cst_str	=	"unknown structure in cast";
STATIC	  char *e_bkp_notuser	=				/* L003v */
	"breakpoint addresses not valid in user level scodb";
STATIC	  char *e_fcn_notuser	=
	"that function call is not valid in user level scodb";	/* L003^ */

NOTSTATIC int  ccobas;		/* output base to print		*/
NOTSTATIC int  cclen;		/* length to print		*/
NOTSTATIC char ccpres[64];	/* string to print before value	*/
NOTSTATIC char ccposs[64];	/* string to print after value	*/
NOTSTATIC int  ccnewl;		/* print newline?		*/

STATIC	  char *stringp;

/*
*	I/O modifications
*/
STATIC
iomod(s)
	register char **s;
{
#define		CC_MB	'{'	/* mode begin */
#define		CC_ME	'}'	/* mode end */
#define		S	(*s)
	int l = 0;	/* listed yet?				*/
	int md = 0;	/* mode - command or string input	*/
	int mdx = 0;
	int np = 1;	/* number of "paren"s			*/
	int esc = 0;	/* escaped?				*/
	char *os = 0;	/* output string			*/
	char c;


	/* defaults: */
	cclen		= -1;		/* output a long	*/
	ccobas		= 16;		/* hexadecimal		*/
	ccnewl		= 1;		/* print newline	*/
	ccpres[0]	= '\0';		/* no string		*/
	ccposs[0]	= '\0';		/* no string		*/

	WHITES(s);
	if (*S == CC_MB) {	/* Mod begin */
		INC();
		while (np) {
			if (!*S) {
				scodb_error = "Unmatched {";
				return 0;
			}
			if (*S == CC_ME && np == 1)
				break;
		gmd:	switch (md) {
			case 0:	/* commands	*/
				switch (*S) {
					case '>':
						cclen  = 0;
						break;

					case 'b':
						cclen  = MD_BYTE;
						break;

					case 's':
						cclen  = MD_SHORT;
						break;

					case '2':
						ccobas = 2;
						break;

					case 'O':
					case 'o':
						ccobas = 8;
						break;

					case 'D':
					case 'd':
						ccobas = 10;
						break;

					case ':':
						md = 1;
						switch (mdx) {
							case 0:
								os = ccpres;
								break;
							case 1:
								*os = '\0';
								os = ccposs;
								break;
							default:
								scodb_error = "already have output strings.";
								return 0;
						}
						++mdx;
						break;

					case '?':
						if (!l++) {
printf("   >     Don't output a value\n");
printf("   b     Output value as a byte\n");
printf("   s     Output value as a short\n");
printf("   2     Output value in binary\n");
printf("   o     Output value in octal\n");
printf("   d     Output value in decimal\n");
printf("   :     Output string\n");
						}
						break;

					default:
						scodb_error = "Unknown mode x";
						scodb_error[13] = *S;
						return 0;
				}
				break;

			case 1:
				c = 0;
				switch (*S) {
					case '\\':
						if (esc) {
							esc = 0;
							goto gch;
						}
						++esc;
						break;
					case ':':
						if (esc) {
							esc = 0;
							goto gch;
						}
						md = 0;
						if (mdx == 2)
							break;
						goto gmd;
					case 'n':
						if (esc) {
							esc = 0;
							c = '\n';
						}
						goto gch;
					case 'c':
						if (esc) {
							esc = 0;
							ccnewl = 0;
							break;
						}
						goto gch;
					case CC_MB:
						++np;
						goto gch;
					case CC_ME:
						--np;
				gch:	default:
						if (c == 0)
							c = *S;
						*os++ = c;
						break;
				}
			}
			INC();
		}
		INC();
		if (os)
			*os = '\0';
	}
	return 1;
}

/*
*	get address, pass in vector-0
*/
NOTSTATIC
getaddrv(v, seg, off)
	char **v;
	long *seg, *off;
{
	char buf[256];
	register char *s = buf;

	CHECK_STACK(0);
	/* concat all args */
	while (*v) {
		strcpy(s, *v);
		while (*s) s++;
		*s++ = ' ';
		++v;
	}
	*s = '\0';
	return getaddr(buf, seg, off);
}

/*
*	get address, pass in string
*/
NOTSTATIC
getaddr(st, seg, off)
	char *st;
	long *seg, *off;
{
	struct value v;

	CHECK_STACK(0);
	v.va_seg = *seg;
	if (!value(st, &v, 1, 1))
		return 0;
	*seg = v.va_seg;
	*off = v.va_value;
	return 1;
}

NOTSTATIC
valuev(v, vp, evl, mv)
	char **v;
	struct value *vp;
{
	char buf[256];
	register char *s = buf;

	CHECK_STACK(0);
	/* concat all args */
	while (*v) {
		strcpy(s, *v);
		while (*s)
			s++;
		*s++ = ' ';
		++v;
	}
	*s = '\0';
	return value(buf, vp, evl, mv);
}

NOTSTATIC
value(is, v, evl, mv)
	char *is;
	struct value *v;
{
	int t_basic, t_derived;
	register char **s = &is;
	extern char scodb_stk[];

	CHECK_STACK(0);
	if (!iomod(s))
		return 0;
	v->va_flags = 0;
	f_eval = evl;
	def_seg = v->va_seg;
	stringp = scodb_stk;
	if (!f_001(s, v))
		return 0;
	if (mv) {
		MUST_VALUE(v);
		t_basic = BTYPE(v->va_cvari.cv_type);
		t_derived = v->va_cvari.cv_type >> N_BTSHFT;
		if (cclen == -1) {
			/* user did not specify! */
			cclen = MD_LONG;	/* most things */
			if (!t_derived)	/* is a basic type */
				switch (t_basic) {
					case 0:		/* void */
						cclen = 0;
						break;

					case T_CHAR:
					case T_UCHAR:
						cclen = MD_BYTE;
						break;

					case T_SHORT:
					case T_USHORT:
						cclen = MD_SHORT;
						break;
				}
		}
	}
	WHITES(s);
	if (**s) {
		scodb_error = e_extr_chars;
		return 0;
	}
	return 1;
}

#define		isp(v)	(IS_PTR((v)->va_cvari.cv_type >> N_BTSHFT))

STATIC
f_001(s, v)
	register char **s;
	struct value *v;
{
	FP((".1"));
	CHECK_STACK(0);
	if (!f_002(s, v))
		return 0;
	while (f_OP(s, ",", 1)) {
		MUST_VALUE(v);
		if (!f_002(s, v))
			return 0;
	}
	return 1;
}

STATIC
f_002(s, v)
	register char **s;
	struct value *v;
{
	int op, r, vl = 0;
	long nv;
	struct value vx;

	FP((".2"));
	CHECK_STACK(0);
	if (!f_003(s, v))
		return 0;
	if ((op = f_OP(s, "=",    1))	||
	    (op = f_OP(s, "|=",   2))	||
	    (op = f_OP(s, "^=",   3))	||
	    (op = f_OP(s, "&=",   4))	||
	    (op = f_OP(s, "<<=",  5))	||
	    (op = f_OP(s, ">>=",  6))	||
	    (op = f_OP(s, "+=",   7))	||
	    (op = f_OP(s, "-=",   8))	||
	    (op = f_OP(s, "*=",   9))	||
	    (op = f_OP(s, "/=",  10))	||
	    (op = f_OP(s, "%=",  11))	) {
		if (!is_lval(v))
			return 0;
		vx.va_flags = 0;
		if (!f_002(s, &vx)) /* f_002 because you can have a = b = c */
			return 0;
		MUST_VALUE(&vx);
		if (op != 7 && op != 8)
			v->va_flags &= ~V_ADDRESS;
		switch (op) {
			case  1:	/*	  =	*/
				nv = vx.va_value;
				break;

			case  2:	/*	 |=	*/
				nv = v->va_value | vx.va_value;
				++vl;
				break;

			case  3:	/*	 ^=	*/
				nv = v->va_value ^ vx.va_value;
				++vl;
				break;

			case  4:	/*	 &=	*/
				nv = v->va_value & vx.va_value;
				++vl;
				break;

			case  5:	/*	<<=	*/
				nv = v->va_value << vx.va_value;
				++vl;
				break;

			case  6:	/*	>>=	*/
				nv = v->va_value >> vx.va_value;
				++vl;
				break;

			case  7:	/*	 +=	*/
				nv = v->va_value;
				if (isp(v) && !isp(&vx))
					nv += f_sizeof(0, &vx.va_cvari, 0)*vx.va_value;
				else {
					++vl;
					nv += vx.va_value;
				}
				break;

			case  8:	/*	 -=	*/
				nv = v->va_value;
				if (isp(v) && !isp(&vx))
					nv -= f_sizeof(0, &vx.va_cvari, 0)*vx.va_value;
				else {
					++vl;
					nv -= vx.va_value;
				}
				break;

			case  9:	/*	 *=	*/
				nv = v->va_value * vx.va_value;
				++vl;
				break;

			case 10:	/*	 /=	*/
				nv = v->va_value / vx.va_value;
				++vl;
				break;

			case 11:	/*	 %=	*/
				nv = v->va_value % vx.va_value;
				++vl;
				break;
		}
		v->va_value = nv;
		if (vl) {
			/* v is now just an integer, or something
			   like that */
			v->va_cvari.cv_type = T_INT;
			v->va_flags = V_VALUE;
		}
		if (f_eval) {
			/*
			*	write appropriate amount of data out
			*/
			switch (v->va_cvari.cv_type) {
				case T_CHAR:
				case T_UCHAR:
					r = db_putbyte(v->va_seg, v->va_address, (unsigned char)v->va_value);
					break;

				case T_SHORT:
				case T_USHORT:
					r = db_putshort(v->va_seg, v->va_address, (unsigned short)v->va_value);
					break;

				default:
					r = db_putlong(v->va_seg, v->va_address, (unsigned long)v->va_value);
					break;
			}
			if (r == 0) {
				scodb_error = e_cant_asgn;
				return 0;
			}
		}
	}
	return 1;
}

/*
*	notice that we don't evaluate the rest of the
*	expression if the first part is true
*/
STATIC
f_003(s, v)
	register char **s;
	struct value *v;
{
	int ofeval, ev;
	struct value vx;

	FP((".3"));
	CHECK_STACK(0);
	if (!f_004(s, v))
		return 0;
	if (f_OP(s, "?", 1)) {
		MUST_VALUE(v);
		ev = v->va_value;
		if (!ev) {
			ofeval = f_eval;
			f_eval = 0;
		}
		if (!f_004(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		if (ev) {	/* got the answer */
			v->va_flags &= ~V_ADDRESS;
			v->va_value = vx.va_value;
		}
		else
			f_eval = ofeval;
		/* must get rest of it */
		if (!f_OP(s, ":", 1))
			return 0;
		if (ev) {
			ofeval = f_eval;
			f_eval = 0;
		}
		if (!f_004(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		if (!ev) {	/* got the answer */
			v->va_flags &= ~V_ADDRESS;
			v->va_value = vx.va_value;
		}
		else
			f_eval = ofeval;
	}
	return 1;
}

/*
*	notice that we don't evaluate the rest of the
*	expression if the first part is true
*/
STATIC
f_004(s, v)
	register char **s;
	struct value *v;
{
	int ofeval;
	struct value vx;

	FP((".4"));
	CHECK_STACK(0);
	if (!f_005(s, v))
		return 0;
	while (f_OP(s, "||", 1)) {
		MUST_VALUE(v);
		if (v->va_value) {
			ofeval = f_eval;
			f_eval = 0;
		}
		vx.va_flags = 0;
		if (!f_005(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		v->va_flags &= ~V_ADDRESS;
		if (!v->va_value)
			v->va_value = v->va_value || vx.va_value;
		else
			f_eval = ofeval;
	}
	return 1;
}

/*
*	notice that we don't evaluate the rest of the
*	expression if the first part is false
*/
STATIC
f_005(s, v)
	register char **s;
	struct value *v;
{
	int ofeval;
	struct value vx;

	FP((".5"));
	CHECK_STACK(0);
	if (!f_006(s, v))
		return 0;
	while (f_OP(s, "&&", 1)) {
		MUST_VALUE(v);
		if (!v->va_value) {
			ofeval = f_eval;
			f_eval = 0;
		}
		vx.va_flags = 0;
		if (!f_006(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		v->va_flags &= ~V_ADDRESS;
		if (v->va_value)
			v->va_value = v->va_value && vx.va_value;
		else
			f_eval = ofeval;
	}
	return 1;
}

STATIC
f_006(s, v)
	register char **s;
	struct value *v;
{
	struct value vx;

	FP((".6"));
	CHECK_STACK(0);
	if (!f_007(s, v))
		return 0;
	while (f_OP(s, "|", 1)) {
		MUST_VALUE(v);
		vx.va_flags = 0;
		if (!f_007(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		v->va_flags &= ~V_ADDRESS;
		v->va_value = v->va_value | vx.va_value;
	}
	return 1;
}

STATIC
f_007(s, v)
	register char **s;
	struct value *v;
{
	struct value vx;

	FP((".7"));
	CHECK_STACK(0);
	if (!f_008(s, v))
		return 0;
	while (f_OP(s, "^", 1)) {
		MUST_VALUE(v);
		vx.va_flags = 0;
		if (!f_008(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		v->va_flags &= ~V_ADDRESS;
		v->va_value = v->va_value ^ vx.va_value;
	}
	return 1;
}

STATIC
f_008(s, v)
	register char **s;
	struct value *v;
{
	struct value vx;

	FP((".8"));
	CHECK_STACK(0);
	if (!f_009(s, v))
		return 0;
	while (f_OP(s, "&", 1)) {
		MUST_VALUE(v);
		vx.va_flags = 0;
		if (!f_009(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		v->va_flags &= ~V_ADDRESS;
		v->va_value = v->va_value & vx.va_value;
	}
	return 1;
}

STATIC
f_009(s, v)
	register char **s;
	struct value *v;
{
	int op, r;
	struct value vx;

	FP((".9"));
	CHECK_STACK(0);
	if (!f_00A(s, v))
		return 0;
	while ((op = f_OP(s, "==", 1))	||
	       (op = f_OP(s, "!=", 2))	) {
		MUST_VALUE(v);
		vx.va_flags = 0;
		if (!f_00A(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		v->va_flags &= ~V_ADDRESS;
		switch (op) {
			case 1:
				r = v->va_value == vx.va_value;
				break;
			case 2:
				r = v->va_value != vx.va_value;
				break;
		}
		v->va_value = r;
	}
	return 1;
}

STATIC
f_00A(s, v)
	register char **s;
	struct value *v;
{
	int op, r;
	struct value vx;

	FP((".A"));
	CHECK_STACK(0);
	if (!f_00B(s, v))
		return 0;
	while ((op = f_OP(s, "<",  1))	||
	       (op = f_OP(s, "<=", 2))	||
	       (op = f_OP(s, ">",  3))	||
	       (op = f_OP(s, ">=", 4))	) {
		MUST_VALUE(v);
		vx.va_flags = 0;
		if (!f_00B(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		v->va_flags &= ~V_ADDRESS;
		switch (op) {
			case 1:
				r = v->va_value < vx.va_value;
				break;
			case 2:
				r = v->va_value <= vx.va_value;
				break;
			case 3:
				r = v->va_value > vx.va_value;
				break;
			case 4:
				r = v->va_value >= vx.va_value;
				break;
		}
		v->va_value = r;
	}
	return 1;
}

STATIC
f_00B(s, v)
	register char **s;
	struct value *v;
{
	int op;
	struct value vx;

	FP((".B"));
	CHECK_STACK(0);
	if (!f_00C(s, v))
		return 0;
	while ((op = f_OP(s, ">>", 1))	||
	       (op = f_OP(s, "<<", 2))	) {
		MUST_VALUE(v);
		vx.va_flags = 0;
		if (!f_00C(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		v->va_flags &= ~V_ADDRESS;
		switch (op) {
			case 1:
				v->va_value >>= vx.va_value;
				break;

			case 2:
				v->va_value <<= vx.va_value;
				break;
		}
	}
	return 1;
}

STATIC
f_00C(s, v)
	register char **s;
	struct value *v;
{
	int op, vl = 0;
	struct value vx;

	FP((".C"));
	CHECK_STACK(0);
	if (!f_00D(s, v))
		return 0;
	while ((op = f_OP(s, "+", 1))	||
	       (op = f_OP(s, "-", 2))	) {
		MUST_VALUE(v);
		vx.va_flags = 0;
		if (!f_00D(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		switch (op) {
			case 1:
				if (isp(v) && !isp(&vx))
					v->va_value += f_sizeof(1, &vx.va_cvari, 0)*vx.va_value;
				else {
					++vl;
					v->va_value += vx.va_value;
				}
				break;

			case 2:
				if (isp(v) && !isp(&vx))
					v->va_value -= f_sizeof(1, &vx.va_cvari, 0)*vx.va_value;
				else {
					++vl;
					v->va_value -= vx.va_value;
				}
				break;
		}
		if (vl) {
			/* v is now just an integer, or something
			   like that */
			v->va_cvari.cv_type = T_INT;
			v->va_flags = V_VALUE;
		}
	}
	return 1;
}

STATIC
f_00D(s, v)
	register char **s;
	struct value *v;
{
	int op;
	struct value vx;

	FP((".D"));
	CHECK_STACK(0);
	if (!f_00E(s, v))
		return 0;
	while ((op = f_OP(s, "*", 1))	||
	       (op = f_OP(s, "/", 2))	||
	       (op = f_OP(s, "%", 3))	) {
		MUST_VALUE(v);
		vx.va_flags = 0;
		if (!f_00E(s, &vx))
			return 0;
		MUST_VALUE(&vx);
		v->va_flags &= ~V_ADDRESS;
		switch (op) {
			case 1:
				v->va_value *= vx.va_value;
				break;

			case 2:
				v->va_value /= vx.va_value;
				break;

			case 3:
				v->va_value %= vx.va_value;
				break;

		}
	}
	return 1;
}

/*
*	everything about 00E is definitely a VALUE.
*/
STATIC
f_00E(s, v)
	register char **s;
	struct value *v;
{
	int op, oev, r;
	long vl;
	struct value vx;

	FP((".E(%s)", *s));
	CHECK_STACK(0);
	
	/*
	*	Try level 0x0E prefix operators
	*/

	if ((op = f_OP(s, "!",  1))	||
	    (op = f_OP(s, "~",  2))	||
	    (op = f_OP(s, "-",  3))	||
	    (op = f_OP(s, "+",  4))	||
	    (op = f_OP(s, "++", 5))	||
	    (op = f_OP(s, "--", 6))	||
	    (op = f_OP(s, "*",  7))	||
	    (op = f_OP(s, "&",  8))	) {
		if (!f_00E(s, v))
			return 0;
		if (op == 5 || op == 6) {
			if (!is_lval(v))
				return 0;
		}
		if (op < 5) {	/* !, ~, -, + */
			if ((v->va_flags & V_VALUE) == 0) {
				scodb_error = e_not_val;
				return 0;
			}
		}
		if (op < 7) {
			MUST_VALUE(v);
			v->va_flags &= ~V_ADDRESS;
		}
		switch (op) {
			case 1:		/*    !    */
				v->va_value = !(v->va_value);
				break;

			case 2:		/*    ~    */
				v->va_value = ~(v->va_value);
				break;

			case 3:		/*    -    */
				v->va_value = -(v->va_value);
				break;

			case 4:		/*    +    */
				/* nothing to do? */
				break;

			case 5:		/*   ++    */
				if (isp(v))
					v->va_value += f_sizeof(0, &v->va_cvari, 1);
				else
					++v->va_value;
				if (f_eval) {
					if (!db_putlong(v->va_seg, v->va_address, v->va_value)) {
						scodb_error = e_cant_asgn;
						return 0;
					}
				}
				break;

			case 6:		/*   --    */
				if (isp(v))
					v->va_value += f_sizeof(0, &v->va_cvari, 1);
				else
					++v->va_value;
				if (f_eval) {
					if (!db_putlong(v->va_seg, v->va_address, v->va_value)) {
						scodb_error = e_cant_asgn;
						return 0;
					}
				}
				break;

			case 7:		/*    *	   */
				if (!dereference(v))
					return 0;
				break;

			case 8:		/*    &    */
				if (!f_do_adr(v))
					return 0;
				break;
		}
		goto pofx;
	}

	if (f_OP(s, "sizeof", 1)) {
		switch (f_op_00E_CAST(s, &vx)) {
			case 1:
				break;

			case 0:
				oev = f_eval;
				f_eval = 0;
				r = f_00E(s, &vx);
				f_eval = oev;
				if (r)
					break;
				return 0;

			case -1:
				return 0;
		}
		v->va_value = f_sizeof(0, &vx.va_cvari, 0);
		v->va_cvari.cv_type = T_INT;
		v->va_flags = V_VALUE;
		goto pofx;
	}

	FP(("-.cast(%s)", *s));

	switch (f_op_00E_CAST(s, &vx)) {
		case -1:
			return 0;

		case  1:
			if (!f_00E(s, v))
				return 0;
			MUST_VALUE(v);
			if (!f_do_cast(v, &vx))
				return 0;
			goto pofx;
	}

	FP(("+.cast(%s)", *s));

	if (!f_00F(s, v))
		return 0;
	
	/*
	*	Try level 0x0E postfix operators
	*/

pofx:	if ((op = f_OP(s, "++", 1))	||
	    (op = f_OP(s, "--", 2))	) {
		if (!is_lval(v))
			return 0;
		v->va_flags &= ~V_ADDRESS;
		/* don't increment value, just add to memory */
		vl = v->va_value;
		switch (op) {
			case 1: ++vl; break;
			case 2: --vl; break;
		}
		if (f_eval) {
			if (!db_putlong(v->va_seg, v->va_address, vl)) {
				scodb_error = e_cant_asgn;
				return 0;
			}
		}
	}
	return 1;
}

STATIC
f_00F(s, v)
	register char **s;
	struct value *v;
{
	FP((".F"));
	CHECK_STACK(0);
	if (!f_010(s, v))
		return 0;
	for (;;) {
		switch (f_op_00F_ARRAY(s, v, 0)) {
			case -1: return 0; case  1: continue; }
		switch (f_op_00F_FIELD(s, v, 0)) {
			case -1: return 0; case  1: continue; }
		switch (f_op_00F_FUNC(s, v, 0))  {
			case -1: return 0; case  1: continue; }
		switch (f_op_00F_POINT(s, v, 0)) {
			case -1: return 0; case  1: continue; }
		/* none of the above! */
		break;
	}
	return 1;
}

STATIC
f_010(s, v)
	register char **s;
	struct value *v;
{
	FP(("10(%s)", *s));
	CHECK_STACK(0);
	WHITES(s);
	if (f_OP(s, "(" /*)*/, 1)) {
		FP(("10-paren:"));
		if (!f_001(s, v))
			return 0;
		if (!f_OP(s,/*(*/ ")", 1)) {
			scodb_error = e_exp_rpr;
			return 0;
		}
		return 1;
	}
	switch (f_SYMBOL(s, v))	{ case -1: return 0; case  1: return 1; }
	switch (f_NUMER(s, v))	{ case -1: return 0; case  1: return 1; }
	switch (f_CHAR(s, v))	{ case -1: return 0; case  1: return 1; }
	switch (f_STRING(s, v))	{ case -1: return 0; case  1: return 1; }
	if (f_OP(s, N_REGISTER, 1))   return f_REG(s, v);
	if (f_OP(s, N_BREAKPOINT, 1)) return f_BKP(s, v);
	if (f_OP(s, N_ARGUMENT, 1))   return f_ARG(s, v);
	if (f_OP(s, N_VARIABLE, 1))   return f_VAR(s, v);
	scodb_error = e_exp_expr;
	return 0;
}

/*
*	is it a character string?
*
*	strings are ONLY valid during a given expression!
*	ie, they get reused.
*/
STATIC
f_STRING(s, v)
	register char **s;
	struct value *v;
{
	int esc = 0, vl = 0, i;
	char c;
	char *string = stringp;		/* where to put string */
	register char *x = stringp;

	FP(("\"\""));
	WHITES(s);
	if (**s == '"') {
		INC();
		while (**s) {
			c = **s;
			INC();	/* gobble input character */
			switch (c) {
				/* escape character */
				case '\\':
					if (esc)
						goto gotchr;
					esc = 1;
					break;
				
				/* normal escapes: */
				case 'n': if (esc) c = '\n'; goto gotchr;
				case 't': if (esc) c = '\t'; goto gotchr;
				case 'f': if (esc) c = '\f'; goto gotchr;
				case 'b': if (esc) c = '\b'; goto gotchr;
				case 'a': if (esc) c = '\a'; goto gotchr;
				case 'r': if (esc) c = '\r'; goto gotchr;
				case 'v': if (esc) c = '\v'; goto gotchr;
				case '?': if (esc) c = '\?'; goto gotchr;
				case '"':
					if (esc)
						c = '\t';
					else
						goto done;

				/* octal escapes: */
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
					if (!esc)
						goto gotchr;
					vl += c - '0';
					for (i = 1;i < 3 && octal(**s);i++) {
						vl *= 8;
						vl += **s - '0';
						INC();
					}
					c = vl;
					goto gotchr;

				gotchr:		 /* dealt with escape */
				default:	 /* have a character to save */
					esc = 0; /* not escaped any more */
					*x++ = c;
					break;
			}
		}
	done:	if (!**s) {
			scodb_error = e_exp_quote;
			return -1;	/* fail miserably */
		}
		*x++ = '\0';
		stringp = x;
		v->va_value = (long)string;
		v->va_address = (long)string;
		v->va_cvari.cv_type = T_CHAR_P;
		v->va_flags = V_VALUE;		/* NOT address: not lvalue. */
		return 1;	/* success */
	}
	return 0;	/* not a string */
}

/*
*	is it a character code ('x', '\000', '\x00')
*/
STATIC
f_CHAR(s, v)
	register char **s;
	struct value *v;
{
	int vl = 0, doi = 1;

	FP(("''"));
	WHITES(s);
	if (**s == '\'') {
		INC();
		if (**s == '\\') {	/* escape code */
			INC();
			switch (**s) {
				case '"':  vl = '\"'; break;
				case '?':  vl = '\?'; break;
				case '\'': vl = '\''; break;
				case '\\': vl = '\\'; break;
				case 'a':  vl = '\a'; break;
				case 'b':  vl = '\b'; break;
				case 'f':  vl = '\f'; break;
				case 'n':  vl = '\n'; break;
				case 'r':  vl = '\r'; break;
				case 't':  vl = '\t'; break;
				case 'v':  vl = '\v'; break;

				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
					while (octal(**s)) {
						vl *= 8;
						vl += **s - '0';
						INC();
					}
					doi = 0;
					break;
				
				case 'x':
					INC();
					if (!hexdi(**s))
						goto err;
					while (hexdi(**s)) {
						vl *= 16;
						vl += **s;
						if (numer(**s))
							vl -= '0';
						else {
							vl += 10;
							if (hexcl(**s))
								vl -= 'a';
							else
								vl -= 'A';
						}
						INC();
					}
					doi = 0;
					break;

				default:
				err:
					scodb_error = e_bad_char;
					return -1;
			}
		}
		else
			vl = **s;
		if (doi)
			INC();
		if (**s != '\'')
			goto err;
		INC();
		v->va_value = vl;
		v->va_cvari.cv_type = T_INT;
		v->va_flags = V_VALUE;
		return 1;
	}
	return 0;
}

/*
*	leading 0 for octal,
*	leading $ for decimal,
*	default hex
*/
STATIC
f_NUMER(s, v)
	register char **s;
	struct value *v;
{
	int i;
	int gotseg = 0, gotoct = 0;
	int nd = 0;
	int md;
	char vl;
	int ibas;

	FP(("NU"));
	WHITES(s);
	if ((*s)[0] == '$' && numer((*s)[1])) { 	/* decimal */
		INC();		/* past the $ */
		ibas = 10;
	}
	else if ((*s)[0] == '0') { 	
		if ((*s)[1] == 'x' || (*s)[1] == 'X') {
			INC();				/* hex */
			INC();
			ibas = 16;
		} else
		if ((*s)[1] == 't' || (*s)[1] == 'T') {
			INC();				/* decimal */
			INC();
			ibas = 10;
		} else {
			/* see if it's really a hex digit */
			i = 0;
			while (hexdi((*s)[i])) {
				if (!octal((*s)[i]))
					break;
				++i;
			}
			if (hexdi((*s)[i])) {
				/* really hex! */
				ibas = 16;
			}
			else {
				gotoct = 1;			/* octal */
				ibas = 8;
			}
		}
	}
	else if (hexdi(**s)) 				/* hex */
		ibas = 16;
	else
		return 0;	/* no number here, sorry. */

	v->va_seg = def_seg;
	switch (ibas) {	/* find max number of digits */
		case 8:
			md = 11 + gotoct;
			break;

		case 10:
			md = 10;
			break;

		case 16:
			md = 8;
			break;
	}
ghxn:	v->va_value = 0;
	do {
		i = 0;
		switch (**s) {
			case '8': case '9':
				if (ibas < 9)
					goto baddig;
				/* fall through: */

			case '0': case '1': case '2':
			case '3': case '4': case '5':
			case '6': case '7':
				vl = **s - '0';
				break;

			case 'a': case 'b': case 'c':
			case 'd': case 'e': case 'f':
				++i;

			case 'A': case 'B': case 'C':
			case 'D': case 'E': case 'F':
				if (ibas < 11)
					goto baddig;
				vl = **s + 10 - (i ? 'a' : 'A');
				break;;

			baddig:
			default:
				scodb_error = "x: Bad %sdigit";
				*scodb_error = **s;
				goto err2;
		}
		v->va_value = v->va_value * ibas + vl;
		INC();
		if (++nd > md) {
			scodb_error = "%sconstant too long";
		err2:	switch (ibas) {
				case 8:
					scodb_error2 = "octal ";
					break;
				case 10:
					scodb_error2 = "decimal ";
					break;
				case 16:
					scodb_error2 = "hex ";
					break;
			}
			return -1;
		}
	} while (hexdi(**s));
	if (**s == ':') {
		if (gotseg++) {
			scodb_error = e_mult_seg;
			return -1;
		}
		if (nd > (md/2 + (md % 2))) {
			scodb_error = e_seg_lng;
			return -1;
		}
		v->va_seg = v->va_value;
		++gotseg;
		INC();
		if (!hexdi(**s)) {
			scodb_error = "No digit after segment value";
			return -1;
		}
		nd = 0;		/* reset. */
		goto ghxn;
	}
	v->va_cvari.cv_type = T_INT;
	v->va_flags = V_VALUE;
	return 1;
}

/*
*	address of memory
*	to avoid hex constants as coming up as symbols,
*	use a 0 in front of them (eg: ab == 0ab, etc)
*/
STATIC
f_SYMBOL(s, v)
	register char **s;
	struct value *v;
{
	int l;
	char bf[NAMEL];
	register char *bp = bf;
	struct sent *se, *findsymbyname();
	extern struct cvari *cvari;
	extern int nvari;
	unsigned uaddr, type;					/* L003 */

	FP(("SY"));
	CHECK_STACK(-1);
	if (!alpha(**s))
		return 0;

	/*
	*	name of symbol
	*/
	for (l = 0;l < sizeof(bf) && alnum((*s)[l]);l++)
		*bp++ = (*s)[l];
	*bp = '\0';

#ifdef USER_LEVEL						/* L003v */

	/*
	 * Before searching the kernel symbol table, check to see if
	 * the symbol refers to one of the kernel functions that are
	 * to be emulated at the user level (eg: ps()).
	 */

	if (internal_symbol(bf, &uaddr, &type)) {
		v->va_flags = type | V_ADDRESS | V_VALUE;
		v->va_value = uaddr;
		v->va_seg = -1;			/* hack meaning user address */
		v->va_address = v->va_value;
		v->va_cvari.cv_dim[0] = 0;
		v->va_cvari.cv_type = T_INT | (DT_FCN << N_BTSHFT);
		v->va_cvari.cv_size = sizeof(int);
		v->va_cvari.cv_index = -1;
		strcpy(v->va_name, s);
		INCBY(l);
		return 1;
	}
#endif								/* L003^ */

	/*
	*	do we know symbol?
	*/
	se = findsymbyname(bf);
	if (!se) {
		scodb_error = e_cantf_sym;
		return 0;
	}

	INCBY(l);
	findvalbysent(se, v);
	v->va_address = se->se_vaddr;
	if (se->se_flags & SF_TEXT) {
		v->va_flags |= V_TEXT;
		v->va_seg = REGP[T_CS];
	}
	else {
		v->va_flags |= V_DATA;
		v->va_seg = REGP[T_DS];
	}
	v->va_flags |= V_ADDRESS;
	f_memv(v, 0, 0, 0);	/* see if it's a value, also */
	return 1;
}

#ifdef USER_LEVEL						/* L003v */

/*
 * Check to see if the symbol specified is in the table of kernel functions
 * to be emulated internally (eg: ps()).  If so, fill in the structure and
 * union information to look like a function returning an integer.
 */

int
internal_symbol(char *s, unsigned *addr, unsigned *type)
{
	return(0);
}

#endif								/* L003^ */

/* 
 * The comment column in the table below refers to the offset of
 * the register on the trap stack relative to %eax.  The T_REGNAME()
 * macro is used to convert between an index in this table, and a
 * trap offset (see svc/reg.h).
 */

NOTSTATIC char *regnames[] = {
   /*  7 */	"ss",		/* int		*/
   /*  6 */	"uesp",		/* int *	*/
   /*  5 */	"efl",		/* int		*/
   /*  4 */	"cs",		/* int		*/
   /*  3 */	"eip",		/* int		*/
   /*  2 */	"ds",		/* int		*/
   /*  1 */	"es",		/* int		*/
   /*  0 */	"eax",		/* int		*/
   /* -1 */	"ecx",		/* int		*/
   /* -2 */	"edx",		/* int		*/
   /* -3 */	"ebx",		/* int		*/
   /* -4 */	"esp",		/* int *	*/
   /* -5 */	"ebp",		/* int *	*/
   /* -6 */	"esi",		/* int *	*/
   /* -7 */	"edi",		/* int *	*/
   /* -8 */	"trap",		/* int		*/
};
#define		NREGN	(sizeof(regnames)/sizeof(regnames[0]))

/*
*	all int
*/
NOTSTATIC char *sregnames[] = {
   /*  0 */	"ldtr",
   /*  1 */	"gdtr",
   /*  2 */	"idtr",
   /*  3 */	"tr",
   /*  4 */	"proc",
   /*  5 */	"cr0",
   /*  6 */	"cr1",
   /*  7 */	"cr2",
   /*  8 */	"cr3",
};

/*
*	normal registers are type (int *)
*	special registers are type (int)
*/
STATIC
f_REG(s, v)
	register char **s;
	struct value *v;
{
	int i, l;
	char bf[NAMEL];
	register char *bp = bf;

	FP(("%%"));
	CHECK_STACK(-1);
	for (l = 0;l < sizeof(bf) && alnum((*s)[l]);l++)
		*bp++ = (*s)[l];
	*bp = '\0';
	for (i = 0;i < NREGN;i++)
		if (regnames[i] && !strcmp(regnames[i], bf))
			break;
	if (i == NREGN) {
		for (i = 0;i < NSPECREG;i++)
			if (sregnames[i] && !strcmp(sregnames[i], bf))
				break;
		if (i == NSPECREG) {
			scodb_error = e_bad_reg;
			return 0;
		}
		ldsregs();
		v->va_flags = V_VALUE;
		v->va_cvari.cv_type = T_INT;
		v->va_value = sregs[i];
	}
	else {
		i = T_REGNAME(i);
		v->va_flags = V_VALUE|V_ADDRESS;
		switch (i) {
			case T_SS:	/* BAD SS */
				i = T_DS;
				v->va_flags &= ~V_ADDRESS;
				break;

			case T_EIP:
				v->va_seg = REGP[T_CS];
				break;
			
			case T_EBP:
				/* BAD SS
				v->va_seg = REGP[T_SS];
				*/
				v->va_seg = REGP[T_DS];
				break;
			
			default:
				v->va_seg = REGP[T_DS];
				break;
		}
		v->va_address = (long)&REGP[i];
		if (i == T_EDI || i == T_ESI || i == T_EBP || i == T_ESP ||
		    i == T_UESP)
			v->va_cvari.cv_type = T_INT_P;
		else
			v->va_cvari.cv_type = T_INT;
		v->va_value = REGP[i];
	}
	INCBY(l);
	return 1;
}

#ifdef USER_LEVEL						/* L003v */

STATIC
f_BKP(s, v)
register char **s;
struct value *v;
{
	scodb_error = e_bkp_notuser;
	return 0;
}

#else								/* L003^ */

/*
*	address of a breakpoint is it's offset
*	so a breakpoint can be changed that way.
*/
STATIC
f_BKP(s, v)
	register char **s;
	struct value *v;
{
	int l;
	char bf[32];
	register char *bp = bf;
	struct bkp *fbp;
	struct dbkp *fdbp;

	FP(("BK"));
	CHECK_STACK(0);
	for (l = 0;l < sizeof(bf) && alnum((*s)[l]);l++)
		*bp++ = (*s)[l];
	*bp = '\0';
	v->va_flags = V_VALUE|V_ADDRESS;
	switch (bplookup_hashname(&fbp, &fdbp, bf)) {		/* L001 */
		case FOUND_BP:
			v->va_address	= (unsigned long)&fbp->bp_off;
			v->va_value	= fbp->bp_off;
			v->va_seg	= fbp->bp_seg;
			break;

		case FOUND_DBP:
			v->va_address	= (unsigned long)&fdbp->bp_off;
			v->va_value	= fdbp->bp_off;
			v->va_seg	= fdbp->bp_seg;
			break;
		
		default:
			scodb_error = e_unk_bkp;
			return 0;
	}
	v->va_cvari.cv_type = T_INT;
	INCBY(l);
	return 1;
}

#endif								/* L003 */

STATIC
f_ARG(s, v)
	register char **s;
	struct value *v;
{
	struct value vx;
	unsigned off;						/* L002 */

	FP(("@@"));
	CHECK_STACK(0);
	if (!f_NUMER(s, &vx)) {
		scodb_error = e_exp_argn;
		return 0;
	}

	if (!db_topframe(NULL, &off))				/* L002v */
		v->va_address = REGP[T_EBP] + (4 * (vx.va_value+1));
	else
		v->va_address = off + (4 * (vx.va_value+1));	/* L002^ */

	/* dereference it now */
	if (f_eval) {
		if (!db_getlong(REGP[T_DS], v->va_address, &v->va_value)) {
			scodb_error = e_cant_drf;
			return 0;
		}
	}

	v->va_seg = REGP[T_DS];
	v->va_flags = V_VALUE|V_ADDRESS;
	v->va_cvari.cv_type = T_INT;
	return 1;
}

STATIC
f_VAR(s, v)
	register char **s;
	struct value *v;
{
	int l;
	char bf[32];
	struct value *vp;
	register char *bp = bf;

	FP(("$$"));
	CHECK_STACK(0);
	for (l = 0;l < sizeof(bf) && alnum((*s)[l]);l++)
		*bp++ = (*s)[l];
	*bp = '\0';
	if (db_lvar(bf, &vp)) {
		*v = *vp;
		INCBY(l);
		return 1;
	}
	else {
		scodb_error = e_unk_var;
		return 0;
	}
}

/*
*	(type vp)v
*/
STATIC
f_do_cast(v, vp)
	struct value *v, *vp;
{
	int tp = 0, tf = 0;
	int ptp = 0, ptf = 0;
	int t_basic, t_derived;
	int pt_basic, pt_derived;
	extern struct btype btype[];

	FP(("CA"));
	pt_basic = BTYPE(vp->va_cvari.cv_type);
	pt_derived = vp->va_cvari.cv_type >> N_BTSHFT;
	while (pt_derived) {
		switch (pt_derived & 03) {
			case DT_PTR:
			case DT_ARY:
				++ptp;
				break;

			case DT_FCN:
				++ptf;
				break;
		}
		pt_derived >>= N_TSHIFT;
	}
	if (!ptp && (ptf || (pt_basic != 0 && btype[pt_basic].bt_sz == 0))) {
		scodb_error = e_ill_cast;
		return 0;
	}

	t_basic = BTYPE(v->va_cvari.cv_type);
	t_derived = v->va_cvari.cv_type >> N_BTSHFT;
	while (t_derived) {
		switch (t_derived & 03) {
			case DT_PTR:
			case DT_ARY:
				++tp;
				break;

			case DT_FCN:
				++tf;
				break;
		}
		t_derived >>= N_TSHIFT;
	}
	if (!tp && (tf || (t_basic != 0 && btype[t_basic].bt_sz == 0))) {
		scodb_error = e_ill_cast;
		return 0;
	}
	v->va_cvari = vp->va_cvari;
	return 1;
}

/*
*	value is the address of this
*/
STATIC
f_do_adr(v)
	struct value *v;
{
	int t_basic, t_derived;

	FP(("&&"));
	CHECK_STACK(0);
	if ((v->va_flags & V_ADDRESS) == 0) {
		scodb_error = e_not_addr;
		return 0;
	}
	t_derived = v->va_cvari.cv_type >> N_BTSHFT;
	if (IS_ARY(t_derived)) {
		printf("ignored & on array\n");
	}
	else {
		v->va_flags = V_VALUE;
		v->va_value = v->va_address;
		t_derived <<= N_TSHIFT;
		t_derived |= DT_PTR;
		t_basic = BTYPE(v->va_cvari.cv_type);
		v->va_cvari.cv_type = t_basic | (t_derived << N_BTSHFT);
	}
	return 1;
}

/*
*	see if v is a value, and if so what it is
*
*	if (isbitfield),
*		fieldoff == bit field offset in "structure"
*		v will be some sort of unsigned type
*/
STATIC
f_memv(v, isbitfield, fieldoff, fieldsize)
	struct value *v;
{
	int t_basic, t_derived;
	long oaddr;

	t_basic = BTYPE(v->va_cvari.cv_type);
	t_derived = v->va_cvari.cv_type >> N_BTSHFT;
	v->va_flags &= ~V_VALUE;
	if (t_derived == 0) {
		/* is only a basic type - try to get the value */
		if (isbitfield) {
			oaddr = v->va_address;
			v->va_address += (fieldoff / 32);
			fieldoff %= 32;
			t_basic = T_ULONG;
		}
		if (!drfv(t_basic, v)) {
			if (isbitfield)
				v->va_address = oaddr;
			/* can't get value? */
			return;
		}
		v->va_flags |= V_VALUE;
		if (isbitfield) {	/* extract valid portions */
			v->va_address = oaddr;
			v->va_value <<= 32 - (fieldoff + fieldsize);
			v->va_value >>= 32 - fieldsize;
		}
	}
	else if (IS_PTR(t_derived)) {
		/* get the value of the pointer, pretending we're a long */
		if (drfv(T_ULONG, v))				/* L003 */
			v->va_flags |= V_VALUE;			/* L003 */
	}
	else {
		/* function or array, value is the address */
		v->va_value = v->va_address;
		v->va_flags |= V_VALUE;
	}
}

/*
*	*v or v[0]
*		if we dereference a pointer, it's still an lvalue!
*/
STATIC
dereference(v)
	struct value *v;
{
	int t_basic, t_derived;

	FP(("**"));
	CHECK_STACK(0);
	t_basic = BTYPE(v->va_cvari.cv_type);
	t_derived = v->va_cvari.cv_type >> N_BTSHFT;
	if (IS_ARY(t_derived) || IS_PTR(t_derived)) {
		t_derived >>= N_TSHIFT;
		v->va_cvari.cv_type = t_basic | (t_derived << N_BTSHFT);
		v->va_address = v->va_value;
		v->va_flags = V_ADDRESS;
		if (!IS_STUN(t_basic) || IS_PTR(t_derived) || IS_ARY(t_derived))
			v->va_flags |= V_VALUE;
		if (IS_PTR(t_derived) || (!IS_FCN(t_derived) && !IS_STUN(t_basic))) {
			if (drfv(t_basic, v))
				return 1;
		}
		else
			return 1;
	}
	scodb_error = e_ill_drf;
	return 0;
}

STATIC
drfv(tbasic, v)
	struct value *v;
{
	unsigned char c;
	unsigned short s;
	unsigned long l;

	if (!f_eval)
		return 1;
	switch (tbasic) {
		case T_UCHAR:
		case T_CHAR:
			if (!db_getbyte(v->va_seg, v->va_address, &c)) {
				scodb_error = e_cant_drf;
				return 0;
			}
			v->va_value = c;
			break;

		case T_USHORT:
		case T_SHORT:
			if (!db_getshort(v->va_seg, v->va_address, &s)) {
				scodb_error = e_cant_drf;
				return 0;
			}
			v->va_value = s;
			break;

		case T_UINT:
		case T_INT:
		case T_ULONG:
		case T_LONG:
			if (!db_getlong(v->va_seg, v->va_address, &l)) {
				scodb_error = e_cant_drf;
				return 0;
			}
			v->va_value = l;
			break;

		default:
			return 0;
	}
	return 1;
}

STATIC
f_op_00E_CAST(s, v)
	register char **s;
	struct value *v;
{
	int np, r, l;
	char cbuf[80];
	register char *cbp = cbuf;

	WHITES(s);
	CHECK_STACK(-1);
	if (**s == '(' /*)*/ ) {
		FP(("ca-try:"));
		l = 0;
		np = 1;
		while ((*s)[l + 1]) {
			if ((*s)[l + 1] == '(' /*)*/ )
				++np;
			else if ((*s)[l + 1] == /*(*/ ')' ) {
				if (--np == 0)
					break;
			}
			*cbp++ = (*s)[l + 1];
			++l;
		}
		*cbp = '\0';
		if (!(*s)[l + 1]) {
			/*
			*	? unmatched paren!
			*	will be delt with later
			*/
			return 0;
		}
		v->va_cvari.cv_names = v->va_name;
		r = dcl(cbuf, &v->va_cvari);
		if (r == 0) {
			FP(("ca-parens:"));
			INCBY(l+2);	/* 2==parens */
			return 1;
		}
		if (r == D_TUNKNOWN) {
			/*
			*	not a typecast
			*	no error.
			*/
			FP(("ca-unknown:"));
			return 0;
		}
		FP(("ca-fail:"));
		switch (r) {
			case D_BADINP:
				scodb_error = e_bad_cast;
				return -1;

			case D_NOCSARE:
				scodb_error = e_cst_unmb;
				return -1;

			case D_NORPR:
				scodb_error = e_cst_unmp;
				return -1;

			case D_SUNKNOWN:
				scodb_error = e_cst_str;
				return -1;
		}
	}
	FP(("ca-non:"));
	return 0;
}

/*
*	dereference and then f_op_00F_FIELD.
*/
STATIC
f_op_00F_POINT(s, v, md)
	register char **s;
	struct value *v;
{
	FP(("->"));
	CHECK_STACK(-1);
	if (f_OP(s, "->", 1)) {
		if (!dereference(v)) {
			scodb_error = e_not_stun;
			return -1;
		}
		return f_op_00F_FIELD(s, v, 1);
	}
	else
		return 0;
}

/*
*	can only field a structure/union type.
*/
STATIC
f_op_00F_FIELD(s, v, md)
	register char **s;
	struct value *v;
{
	int t_basic, t_derived, l, i;
	char bf[32];
	struct cstun *cs;
	struct cstel *ce;
	register char *bp = bf;
	extern struct cstun *stun;
	extern int nstun;

	FP((".."));
	CHECK_STACK(-1);
	if (md || f_OP(s, ".", 1)) {
		t_basic = BTYPE(v->va_cvari.cv_type);
		t_derived = v->va_cvari.cv_type >> N_BTSHFT;
		if (t_derived || (t_basic != T_STRUCT && t_basic != T_UNION)) {
			scodb_error = e_not_stun;
			return -1;
		}

		/*
		*	name of field
		*/
		for (l = 0;l < sizeof(bf) && alnum((*s)[l]);l++)
			*bp++ = (*s)[l];
		*bp = '\0';

		if (v->va_cvari.cv_index < 0) {
			scodb_error = e_stun_unk;
			return -1;
		}
		cs = &stun[v->va_cvari.cv_index];
		ce = cs->cs_cstel;
		for (i = 0;i < cs->cs_nmel;i++)
			if (!strcmp(ce[i].ce_names, bf)) {
				v->va_cvari = ce[i].ce_cvari;
				if (ce[i].ce_flags & SNF_FIELD)
					f_memv(v, 1, ce[i].ce_offset, ce[i].ce_size);
				else {
					v->va_address += ce[i].ce_offset;
					f_memv(v, 0, 0, 0);
				}
				INCBY(l);
				return 1;
			}
		scodb_error = e_not_elem;
		return -1;
	}
	return 0;
}

/*
*	can subscript:
*		array type
*		non-function pointer type
*/
STATIC
f_op_00F_ARRAY(s, v, md)
	register char **s;
	struct value *v;
{
	int t_basic, t_derived, i, tsize;
	struct value vx;
	extern struct btype btype[];
	extern struct cstun *stun;

	FP(("[]"));
	CHECK_STACK(-1);
	if (f_OP(s, "[" /*]*/, 1)) {
		t_basic = BTYPE(v->va_cvari.cv_type);
		t_derived = v->va_cvari.cv_type >> N_BTSHFT;
		if (!IS_ARY(t_derived) && (!IS_PTR(t_derived) || IS_FCN(t_derived >> N_TSHIFT))) {
			scodb_error = e_ill_arr;
			return -1;
		}
		if (!f_001(s, &vx))
			return -1;
		if (!f_OP(s,/*[*/ "]", 1)) {
			scodb_error = e_exp_rbr;
			return -1;
		}
		if (IS_ARY(t_derived)) {
			for (i = 1;i < NDIM;i++)
				v->va_cvari.cv_dim[i - 1] =
					v->va_cvari.cv_dim[i];
		}
		t_derived >>= N_TSHIFT;
		v->va_cvari.cv_type = t_basic | (t_derived << N_BTSHFT);
		tsize = f_sizeof(0, &v->va_cvari, 0);
		v->va_address = v->va_value + vx.va_value * tsize;
		f_memv(v, 0, 0, 0);
		return 1;
	}
	return 0;
}

STATIC
f_op_00F_FUNC(s, v, md)
	register char **s;
	struct value *v;
{
	int t_basic, t_derived, gotend = 0;
	int args[MXARGS], i = 0;
	struct value vx;

	FP(("()"));
	CHECK_STACK(-1);
	if (f_OP(s, "(" /*)*/, 1)) {
		t_basic = BTYPE(v->va_cvari.cv_type);
		t_derived = v->va_cvari.cv_type >> N_BTSHFT;
		if (!IS_FCN(t_derived)) {
			scodb_error = e_not_fcn;
			return -1;
		}
		t_derived >>= N_TSHIFT;
		v->va_cvari.cv_type = t_basic | (t_derived << N_BTSHFT);
		for (i = 0;i < MXARGS;i++) {
			if (f_OP(s,/*(*/ ")", 1)) {
				/* done */
				++gotend;
				break;
			}
			if (i && !f_OP(s, ",", 1)) {
				scodb_error = e_exp_comma;
				return -1;
			}
			if (!f_002(s, &vx))
				return -1;
			args[i] = vx.va_value;
		}
		if (!gotend) {
			if (!f_OP(s,/*(*/ ")", 1)) {
				scodb_error = e_tm_args;
				return -1;
			}
		}
		while (i < MXARGS)
			args[i++] = 0;
		v->va_flags &= ~V_ADDRESS;
		if (f_eval) {

#ifdef USER_LEVEL						/* L003v */

			/*
			 * Only kernel function calls which are emulated
			 * internally (eg: ps()) are allowed.
			 */

			if (v->va_seg != -1) {
				scodb_error = e_fcn_notuser;
				return -1;
			}
#endif								/* L003^ */

			v->va_value = (*((int (*)())v->va_value))(
						args[0]
#if MXARGS > 1
						,args[1]
#if MXARGS > 2
						,args[2]
#if MXARGS > 3
						,args[3]
#if MXARGS > 4
						,args[4]
#if MXARGS > 5
						,args[5]
#if MXARGS > 6
						,args[6]
#if MXARGS > 7
						,args[7]
#if MXARGS > 8
_MXARGS_cant_be_this_high_no_support_here
#endif	/* 8 */
#endif	/* 7 */
#endif	/* 6 */
#endif	/* 5 */
#endif	/* 4 */
#endif	/* 3 */
#endif	/* 2 */
#endif	/* 1 */
			);
		}
		return 1;
	}
	return 0;
}

STATIC char *ops[] = {
	"!",	"!=",	"%",	"&",
	"&&",	"(",	")",	"*",
	"+",	",",	"-",	"->",
	".",	"/",	"<",	"<<",
	"<=",	"==",	">",	">=",
	">>",	"[",	"]",	"?",
	"^",	"|",	"||",	"~",
	"=",	"|=",	"^=",	"&=",
	"<<=",	">>=",	"+=",	"-=",
	"*=",	"/=",	"%=",	":",
	"++",	"--",
	"sizeof",
	0
};

STATIC
f_OP(s, st, op)
	register char **s;
	register char *st;
{
	int st_len;
	register int l;
	register int i;

	WHITES(s);
	for (l = 0;st[l];l++)
		if ((*s)[l] != st[l])
			break;
	if (st[l])	/* not even close */
		return 0;
	st_len = l;
	for (i = 0;ops[i];i++) {
		for (l = 0;ops[i][l];l++)
			if ((*s)[l] != ops[i][l])
				break;
		if (!ops[i][l] && strcmp(ops[i], st) && l > st_len) {
			/* (*s) is part of a bigger operator */
			return 0;
		}
	}
	/* may not even recognize the string as an operator! */
	INCBY(st_len);
	return op;
}

STATIC
is_lval(v)
	struct value *v;
{
	int t_derived;

	if ((v->va_flags & V_VALUE) == 0) {
		scodb_error = e_not_lval;
		return 0;
	}
	t_derived = v->va_cvari.cv_type >> N_BTSHFT;
	if (t_derived && !IS_PTR(t_derived)) {
		scodb_error = e_not_lval;
		return 0;
	}
	/* WASADDR? */
	if ((v->va_flags & V_ADDRESS) == 0) {
		scodb_error = e_not_lval;
		return 0;
	}
	return 1;
}

/*
*	size of an lvalue
*/
NOTSTATIC
f_sizeof(md, cv, ds)
	struct cvari *cv;
{
	int t_basic, t_derived, i;
	int isptr = 0, dn = 0;
	int tsize;
	extern struct btype btype[];
	extern struct cstun *stun;

	if (md && !incsize)
		return 1;
	t_basic = BTYPE(cv->cv_type);
	t_derived = cv->cv_type >> N_BTSHFT;
	if (ds && IS_PTR(t_derived))
		t_derived >>= N_TSHIFT;
	tsize = 1;
	while (t_derived) {
		if (IS_PTR(t_derived)) {
			tsize *= 4;
			++isptr;
			break;
		}
		else if (IS_ARY(t_derived)) {
			tsize *= cv->cv_dim[dn++];
		}
		else	/* IS_FCN: rest of t_derived is ret type */
			break;
		t_derived >>= N_TSHIFT;
	}

	if (!isptr) {
		switch (t_basic) {
			case T_STRUCT:
			case T_UNION:
				if (cv->cv_index >= 0)	/* ??? */
					tsize *= stun[cv->cv_index].cs_size;
				break;
			
			default:
				i = btype[t_basic].bt_sz;
				if (i)
					tsize *= i;
				else
					tsize *= 4;
		}
	}

	return tsize;
}

NOTSTATIC
findvalbysent(se, va)
	struct sent *se;
	struct value *va;
{
	int r;
	register int i;
	char *s;
	struct cvari *cv;
	extern int nvari, scodb_ndcvari;
	extern struct cvari *cvari, scodb_dcvari[];

	cv = &va->va_cvari;
	for (i = 0;i < nvari;i++)
		if (!strcmp(sent_name(se), cvari[i].cv_names)) {  /* L000 */
			*cv = cvari[i];
			r = 1;
			goto retn;
		}
	for (i = 0;i < scodb_ndcvari;i++)
		if (*(s = scodb_dcvari[i].cv_names) &&
		    !strcmp(sent_name(se), s)) {		/* L000 */
			*cv = scodb_dcvari[i];
			r = 1;
			goto retn;
		}
	r = 0;
	cv->cv_dim[0]	= 0;
	cv->cv_size	= sizeof(int);
	cv->cv_index	= -1;
	if (se->se_flags & SF_TEXT)	/* a function */
		cv->cv_type = T_INT_F;
	else
		cv->cv_type = T_INT;
retn:
	cv->cv_names	= va->va_name;	/* reset, guarrantee pointer */
	strcpy(cv->cv_names, sent_name(se));			/* L000 */
	return r;
}

NOTSTATIC
c_type(c, v)
	char **v;
{
	int r;
	struct value vx;

	++v;
	vx.va_cvari.cv_names = vx.va_name;
	r = dclv(v, &vx.va_cvari);
	if (r == D_TUNKNOWN) {
		if (!valuev(v, &vx, 0, 0)) {
			perr();
			return DB_ERROR;
		}
	}
	else if (r != 0) {
		scodb_error = e_bad_cast;
		perr();
		return DB_ERROR;
	}
	putchar('"');
	for (r = 0;*v;r++, ++v)
		printf("%s%s", r ? " " : "", *v);
	putchar('"');
	printf(" is ");
	vx.va_cvari.cv_names[0] = '\0';
	pcvari(0, &vx.va_cvari);
	putchar('\n');
	return DB_CONTINUE;
}

/*
*	set up space for variable names;
*	we given each variable VNAMEL bytes to use and
*	initialize the first byte of each name to 0 ("unused").
*/
NOTSTATIC
db_dv_init() {
	register int i;

	for (i = 0;i < scodb_nvar;i++)
		*(scodb_var[i].va_cvari.cv_names = scodb_var[i].va_name) = '\0';
}

NOTSTATIC
c_var(c, v)
	int c;
	char **v;
{
	register int i;
	char *s, bf[9];
	long val;
	struct value *vap;
	char *symname();
	extern int *REGP;

	if (c == 1) {	/* print all variables set */
		for (i = 0;i < scodb_nvar;i++)
			if ((s = scodb_var[i].va_cvari.cv_names)[0]) {
				putchar('\t');
				prst(s, NAMEL);
				val = scodb_var[i].va_value;
				pn(bf, val);
				printf("   %s", bf);
				printf("\t%s\n", symname(val, 0));
			}
		return DB_CONTINUE;
	}
	/*
	*	see if variable is already set
	*/
	++v;
	for (i = 0;i < scodb_nvar;i++)
		if (!strcmp(*v, scodb_var[i].va_cvari.cv_names))
			break;
	if (i != scodb_nvar) {
		printf("variable already set.\n");
		return DB_ERROR;
	}
	for (i = 0;i < scodb_nvar;i++)
		if (scodb_var[i].va_cvari.cv_names[0] == '\0')
			break;
	if (i == scodb_nvar) {	/* no free ones. */
		printf("No free variables (%d total)\n", scodb_nvar);
		return DB_ERROR;
	}
	vap = &scodb_var[i];
	if (!valuev(v + 1, vap, 1, 1)) {
		*vap->va_cvari.cv_names = '\0';
		perr();
		return DB_ERROR;
	}
	vap->va_address	= (unsigned long)&vap->va_value;
	vap->va_seg	= REGP[T_DS];
	vap->va_flags	= V_VALUE|V_ADDRESS;
	strcpy(vap->va_cvari.cv_names, *v);
	return DB_CONTINUE;
}

NOTSTATIC
c_unvar(c, v)
	char **v;
{
	int er = 0;
	struct value *va;

	while (*++v) {
		if (**v == '*' && !(*v)[1]) {
			if (do_all(0, "Clear all variables? ")) {
				db_dv_init();
				break;
			}
			else
				continue;
		}
		else {
			if (db_lvar(*v, &va))
				va->va_cvari.cv_names[0] = '\0';
			else {
				printf("%s: unknown variable.\n", *v);
				++er;
			}
		}
	}
	return er ? DB_ERROR : DB_CONTINUE;
}

STATIC
db_lvar(varn, rv)
	char *varn;
	struct value **rv;
{
	int n, r;

	r = ufind(2,
		  varn,
		  (char *)scodb_var,
		  sizeof(scodb_var[0]), 
		  scodb_nvar,
		  (int)&scodb_var[0].va_cvari.cv_names - (int)&scodb_var[0],
		  -1,
		  0,
		  &n);
	if (r == FOUND_BP) {
		*rv = &scodb_var[n];
		return 1;
	}
	else
		return 0;
}

db_topframe()
{
	return 0;
}
