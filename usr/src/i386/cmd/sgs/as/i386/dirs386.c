#ident	"@(#)nas:i386/dirs386.c	1.3"
/*
* i386/dirs386.c - i386 assembler-specific directive handling
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common/as.h"
#include "common/dirs.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/sect.h"
#include "common/stmt.h"
#include "common/syms.h"
#include "dirs386.h"
#include "stmt386.h"

#include "intemu.h"

#define CodeForm_BCD	(CodeForm_Integer + 1)
#define BCD_BYTES	10	/* number of bytes in representation */

static void
#ifdef __STDC__
bcd_todata(Eval *vp, register Uchar *p)	/* convert value to BCD bytes */
#else
bcd_todata(vp,p)Eval*vp;register Uchar*p;
#endif
{
	register char *str;
	register size_t len, n;

	/*
	* Convert value to packed BCD form.  On the i386, it is a sign-
	* magnitude representation where the first byte is the least
	* significant:
	*	DD DD DD DD DD DD DD DD DD S0
	* where S represents the sign bit (0x80).
	*/
	str = num_tosdec(vp->ev_int);
	len = strlen(str);
	p += BCD_BYTES;
	*--p = 0;
	if (str[0] == '-')
	{
		if (len > 1 + (BCD_BYTES - 1) * 2)
		{
		toobig:;
			exprerror(vp->ev_expr, gettxt(":927","BCD number too big: %s"), str);
			return;
		}
		len--;
		str++;
		*p = 0x80;
	}
	else if (len > (BCD_BYTES - 1) * 2)
		goto toobig;
	/*
	* Zero-fill high-order bytes.
	*/
	for (n = BCD_BYTES - (len + 1) / 2; --n != 0; *--p = 0)
		;
	if (len & 0x1)		/* odd number of decimal digits */
		*--p = *str++ & 0xf;
	/*
	* Complete low-order bytes.
	*/
	for (n = len / 2; n != 0; n--)
	{
		*--p = (str[0] & 0xf) << 4 | (str[1] & 0xf);
		str += 2;
	}
}

void
#ifdef __STDC__
int_todata(Eval *vp, const Code *cp, Section *secp) /* fill integer object */
#else
int_todata(vp,cp,secp)Eval*vp;Code*cp;Section*secp;
#endif
{
	static const char MSGzbyte[] =
		"section contains only zero-valued bytes: %s";
	register Uchar *p = secp->sec_data;
	register size_t n = cp->code_size;

	if (cp->info.code_form == CodeForm_BCD)
	{
		if (n != BCD_BYTES)
			fatal(gettxt(":928","int_todata():bad BCD length: %lu"), (Ulong)n);
		if (p == 0)	/* permit zero values */
			goto zonly;
		bcd_todata(vp, p + cp->code_addr);
		return;
	}
	if (cp->info.code_form != CodeForm_Integer)
		fatal(gettxt(":929","int_todata():bad form: %u"), (Uint)cp->info.code_form);
	if (p == 0)	/* permit zero-valued objects */
	{
	zonly:;
		if (vp->ev_flags & (EV_OFLOW | EV_TRUNC)
			|| vp->ev_ulong != 0)
		{
			exprerror(vp->ev_expr, gettxt(":789",MSGzbyte),
				(const char *)secp->sec_sym->sym_name);
		}
		return;
	}
	if (vp->ev_minbit > cp->code_size * CHAR_BIT
		|| vp->ev_flags & EV_OFLOW)
	{
		exprwarn(vp->ev_expr, gettxt(":930","value does not fit in %lu bytes: %s"),
			(Ulong)n, num_tohex(vp->ev_int));
	}
	p += cp->code_addr;
	if (vp->ev_flags & EV_TRUNC)
		num_toulendian(vp->ev_int, (void *)p, n);
	else
	{
		register Ulong val = vp->ev_ulong;

		while (n != 0)
		{
			*p++ = val;
			val >>= CHAR_BIT;
			n--;
		}
	}
}

void
#ifdef __STDC__
directive386(const Uchar *s, size_t n, Oplist *p)	/* i386 directives */
#else
directive386(s, n, p)Uchar *s; size_t n; Oplist *p;
#endif
{
	static const char MSGtoo[] = "too many operands for %s";
	static const char MSGnop[]
		= "expecting an optional string operand for .nopsets";
	static const Ulong align2[2] = {2, 1};
	Operand *op;
	Expr *ep;

#ifdef DEBUG
	if (DEBUG('d') > 1)
	{
		(void)fprintf(stderr, "directive386(%s,p=", prtstr(s, n));
		printoplist(p);
		(void)fputs(")\n", stderr);
	}
#endif
	if (p == 0)
		op = 0;
	else
		op = p->olst_first;
	/*
	* i386-specific directives are:
	*	.bcd	expr-list
	*	.even
	*	.long	expr-list
	*	.value	expr-list
	*	.jmpbeg
	*	.jmpend
	*	.nopsets [string]
	*/
	switch (n)
	{
	case 4:
		if (s[1] == 'b' && s[2] == 'c' && s[3] == 'd')
		{
			unaligndata(".bcd", op, (Ulong)BCD_BYTES,
				CodeForm_BCD);
			if (p != 0 && p->olst_first == 0)
				cutolst(p);
			return;
		}
		break;
	case 5:
		if (s[1] == 'l')
		{
			if (s[2] == 'o' && s[3] == 'n' && s[4] == 'g')
			{
				unaligndata(".long", op, (Ulong)4,
					CodeForm_Integer);
				if (p != 0 && p->olst_first == 0)
					cutolst(p);
				return;
			}
		}
		else if (s[1] == 'e' && s[2] == 'v' && s[3] == 'e'
			&& s[4] == 'n')
		{
			if (op != 0)
				error(gettxt(":748",MSGtoo), ".even");
			else
				sectalign((Section *)0, align2, (Uint)1);
			olstfree(op);
			return;
		}
		break;
	case 6:
		if (s[1] == 'v' && s[2] == 'a' && s[3] == 'l'
			&& s[4] == 'u' && s[5] == 'e')
		{
			unaligndata(".value", op, (Ulong)2, CodeForm_Integer);
			if (p != 0 && p->olst_first == 0)
				cutolst(p);
			return;
		}
		break;
	case 7:
		if (s[1] == 'j' && s[2] == 'm' && s[3] == 'p')
		{
			if (s[4] == 'b')
			{
				if (s[5] == 'e' && s[6] == 'g')
				{
					if (op != 0)
						error(gettxt(":748",MSGtoo), ".jmpbeg");
					else
					{
						sectexpr((Section *)0,
							ulongexpr((Ulong)0xf1),
							(Ulong)1,
							CodeForm_Integer);
					}
					olstfree(op);
					return;
				}
			}
			else if (s[4] == 'e' && s[5] == 'n' && s[6] == 'd')
			{
				if (op != 0)
					error(gettxt(":748",MSGtoo), ".jmpend");
				else
				{
					sectexpr((Section *)0,
						ulongexpr((Ulong)0xffffL),
						(Ulong)2,
						CodeForm_Integer);
				}
				olstfree(op);
				return;
			}
		}
		break;
	case 8:
		if (strncmp((const char *)&s[1], "nopsets", 7) == 0)
		{
			if (op == 0)
				nopsets((Uchar *)0, (size_t)0);
			else if (op->oper_flags == 0 && op->oper_next == 0
				&& (ep = op->oper_expr) != 0
				&& ep->ex_type == ExpTy_String)
			{
				nopsets(ep->right.ex_str, ep->left.ex_len);
			}
			else
				error(gettxt(":931",MSGnop));
			olstfree(op);
			return;
		}
		break;
	}
	directive(s, n, p);	/* try common directives */
}

void
#ifdef __STDC__
obssectattr(Section *secp, const Expr *ep)	/* handle obsolete attrs */
#else
obssectattr(secp, ep)Section *secp; Expr *ep;
#endif
{
	static const char MSGattr[] = "invalid attribute string: \"%s\"";
	static int have_warned;
	register const Uchar *p = ep->right.ex_str;
	register size_t len;
	register Ulong offbits = 0;

	if (!have_warned)
	{
		warn(gettxt(":933","using obsolete semantics for .section attributes"));
		have_warned = 1;
	}
	for (len = ep->left.ex_len; len != 0; len--)
	{
		switch (*p++)
		{
		default:
			error(gettxt(":932",MSGattr), prtstr(ep->right.ex_str,
				ep->left.ex_len));
			return;
		case 'b':
			secp->attr.sec_value |= Attr_Alloc | Attr_Write;
			secp->type.sec_value = SecTy_Nobits;
			break;
		case 'i':
			secp->type.sec_value = SecTy_Progbits;
			break;
		case 'w':
			secp->attr.sec_value |= Attr_Alloc | Attr_Write;
			secp->type.sec_value = SecTy_Progbits;
			break;
		case 'x':
			secp->attr.sec_value |= Attr_Alloc | Attr_Exec;
			secp->type.sec_value = SecTy_Progbits;
			break;
		case 'n':
			offbits = Attr_Alloc;
			break;
		case 'c':
		case 'd':
		case 'l':
		case 'o':
			secp->type.sec_value = SecTy_Progbits;
			break;
		}
	}
	secp->attr.sec_value &= ~offbits;
}

static Symbol *def_sym;		/* nonzero => last .def name */
static Symbol *dot_sym;		/* nonzero => last .val was .val . */
static int scl_neg1;		/* nonzero => last .scl was .scl -1 */

static void
#ifdef __STDC__
dot_def(Expr *ep)	/* remember most recent name operand */
#else
dot_def(ep)Expr *ep;
#endif
{
	def_sym = 0;
	if (ep != 0 && ep->ex_op == ExpOp_LeafName)
		def_sym = ep->right.ex_sym;
}

static void
#ifdef __STDC__
dot_val(Expr *ep)	/* remember most recent . (dot) operand */
#else
dot_val(ep)Expr *ep;
#endif
{
	dot_sym = 0;
	if (ep != 0 && ep->ex_op == ExpOp_LeafName
		&& ep->right.ex_sym->sym_kind == SymKind_Dot)
	{
		dot_sym = ep->right.ex_sym;
	}
}

static void
#ifdef __STDC__
dot_scl(const Expr *ep)	/* remember whether most recent operand is -1 */
#else
dot_scl(ep)Expr *ep;
#endif
{
	Ulong value;

	scl_neg1 = 0;
	if (ep != 0 && ep->ex_type == ExpTy_Integer
		&& ep->ex_op == ExpOp_Negate
		&& ep->left.ex_ptr->ex_op == ExpOp_LeafInteger
		&& num_toulong(ep->left.ex_ptr->right.ex_int, &value) != 0
		&& value == 1)
	{
		scl_neg1 = 1;
	}
}

enum	/* obsolete directives */
{
	Obs_Ln,
	Obs_Def,
	Obs_Scl,
	Obs_Val,
	Obs_Tag,
	Obs_Dim,
	Obs_Line,
	Obs_Size,
	Obs_Type,
	Obs_Endef,
	Obs_TOTAL	/* not an obsolete directive */
};


int
#ifdef __STDC__
obsdirect(const Uchar *s, size_t n, Operand *op)
#else
obsdirect(s, n, op)Uchar *s; size_t n; Operand *op;
#endif
{
	static int have_warned[Obs_TOTAL];
	register Expr *ep;
	int dirno;

	/*
	* Ugly patch for COFF to ELF transition.  Take the following
	* sequence of old COFF debugging directives as the equivalent
	* of a .type and .size directive pair for the function:
	*
	*	.def	func;	.val	.;	.scl	-1;	.endef
	* becomes
	*	.type	func, "function"
	*	.size	func, .-func
	*
	* Hold on to such operands for the above three obsolete
	* directives and at each .endef, check for all three being
	* set at once.
	*
	* Also, only issue one specific obsolete warning for each
	* particular directive.
	*/
	switch (n)
	{
	default:
		return 0;
	case 0:		/* only .size or .type */
		if (op == 0 || op->oper_next != 0)
			return 0;
		if (s[1] == 's')
			dirno = Obs_Size;
		else
			dirno = Obs_Type;
		break;
	case 3:
		if (strncmp((const char *)&s[1], "ln", 2) != 0)
			return 0;
		dirno = Obs_Ln;
		break;
	case 4:
		if (op != 0 && op->oper_next == 0 && op->oper_flags == 0)
			ep = op->oper_expr;
		else
			ep = 0;
		if (strncmp((const char *)&s[1], "def", 3) == 0)
		{
			dirno = Obs_Def;
			dot_def(ep);
			break;
		}
		if (strncmp((const char *)&s[1], "scl", 3) == 0)
		{
			dirno = Obs_Scl;
			dot_scl(ep);
			break;
		}
		if (strncmp((const char *)&s[1], "val", 3) == 0)
		{
			dirno = Obs_Val;
			dot_val(ep);
			break;
		}
		if (strncmp((const char *)&s[1], "tag", 3) == 0)
		{
			dirno = Obs_Tag;
			break;
		}
		if (strncmp((const char *)&s[1], "dim", 3) == 0)
		{
			dirno = Obs_Dim;
			break;
		}
		return 0;
	case 5:
		if (strncmp((const char *)&s[1], "line", 4) != 0)
			return 0;
		dirno = Obs_Line;
		break;
	case 6:
		if (strncmp((const char *)&s[1], "endef", 5) == 0)
		{
			dirno = Obs_Endef;
			if (scl_neg1 && def_sym != 0 && dot_sym != 0)
			{
				typesym(def_sym,
					ulongexpr((Ulong)SymTy_Function));
				sizesym(def_sym, binaryexpr(ExpOp_Subtract,
					idexpr(def_sym->sym_name,
						def_sym->sym_nlen),
					idexpr(dot_sym->sym_name,
						dot_sym->sym_nlen)));
			}
			scl_neg1 = 0;
			def_sym = 0;
			dot_sym = 0;
			break;
		}
		return 0;
	}
	if (!have_warned[dirno])	/* issue one-time diagnostic */
	{
		if (n != 0)
			warn(gettxt(":934","obsolete directive ignored: %s"), prtstr(s, n));
		else
		{
			/* only .size or .type */
			warn(gettxt(":935","obsolete use of directive ignored: %s"),
				(const char *)s);
		}
		have_warned[dirno] = 1;
	}
	olstfree(op);
	return 1;
}
