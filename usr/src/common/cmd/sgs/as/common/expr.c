#ident	"@(#)nas:common/expr.c	1.15"
/*
* common/expr.c - common assembler expressions
*/
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "common/as.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/gram.h"
#include "common/sect.h"
#include "common/stmt.h"
#include "common/syms.h"

#include "intemu.h"
#ifdef FLOATEXPRS
#   include "fpemu.h"
#endif

#ifndef ALLOC_EXPR_CHUNK
#   define ALLOC_EXPR_CHUNK 5000
#endif
static Expr *expr_avail;	/* Expr free list */

static Expr *
#ifdef __STDC__
alloexpr(void)	/* allocate ALLOC_EXPR_CHUNK new Expr's */
#else
alloexpr()
#endif
{
	register Expr *ep;
	register int i = ALLOC_EXPR_CHUNK;

#ifdef DEBUG
	if (DEBUG('a') > 0)
	{
		static Ulong total;

		(void)fprintf(stderr, "Total Exprs=%lu @%lu ea.\n",
			total += ALLOC_EXPR_CHUNK, (Ulong)sizeof(Expr));
	}
#endif
	expr_avail = ep = (Expr *)alloc((void *)0,
		ALLOC_EXPR_CHUNK * sizeof(Expr));
	for (; --i != 0; ep++)
		ep->parent.ex_ptr = &ep[1];
	ep->parent.ex_ptr = 0;
	return expr_avail;
}

Expr *
#ifdef __STDC__
regexpr(int regno)	/* build expression from register */
#else
regexpr(regno)int regno;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "regexpr(regno=%d)\n", regno);
#endif
	if ((ep = expr_avail) == 0)
		ep = alloexpr();
	expr_avail = ep->parent.ex_ptr;
	ep->ex_op = ExpOp_LeafRegister;
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	ep->left.ex_ptr = 0;
	ep->right.ex_reg = regno;
	ep->ex_type = ExpTy_Register;
	return ep;
}

Expr *
#ifdef __STDC__
idexpr(const Uchar *str, size_t len)	/* build expression from name */
#else
idexpr(str, len)Uchar *str; size_t len;
#endif
{
	register Expr *ep;
	register Symbol *sp;

#ifdef DEBUG
	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "idexpr(%.*s)\n", len, str);
#endif
	if ((ep = expr_avail) == 0)
		ep = alloexpr();
	expr_avail = ep->parent.ex_ptr;
	ep->ex_op = ExpOp_LeafName;
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	sp = lookup(str, len);
	ep->right.ex_sym = sp;
	ep->ex_type = sp->sym_exty;
	ep->ex_mods = sp->sym_mods;
	ep->left.ex_ptr = sp->sym_uses;
	sp->sym_uses = ep;
	return ep;
}

Expr *
#ifdef __STDC__
strexpr(const Uchar *str, size_t len)	/* build expression from string */
#else
strexpr(str, len)Uchar *str; size_t len;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "strexpr(%s)\n", prtstr(str, len));
#endif
	if ((ep = expr_avail) == 0)
		ep = alloexpr();
	expr_avail = ep->parent.ex_ptr;
	ep->ex_op = ExpOp_LeafString;
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	ep->left.ex_len = len;
	ep->right.ex_str = str;	   /* savestr() done later, if needed */
	ep->ex_type = ExpTy_String;
	return ep;
}

Expr *
#ifdef __STDC__
intexpr(const Uchar *str, size_t len)	/* build expr from integer token */
#else
intexpr(str, len)Uchar *str; size_t len;
#endif
{
	register Expr *ep;
	NumStrErr err;

#ifdef DEBUG
	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "intexpr(%.*s)\n", len, str);
#endif
	if ((ep = expr_avail) == 0)
		ep = alloexpr();
	expr_avail = ep->parent.ex_ptr;
	ep->ex_op = ExpOp_LeafInteger;
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	ep->left.ex_ptr = 0;
	ep->right.ex_int = num_fromstr((Integer *)0,
		(const char *)str, len, &err);
	if (err.emsg != 0)
		error(err.emsg);
	ep->ex_type = ExpTy_Integer;
	return ep;
}

Expr *
#ifdef __STDC__
ulongexpr(Ulong val)	/* build expression from Ulong */
#else
ulongexpr(val)Ulong val;
#endif
{
	register Expr *ep;

	if ((ep = expr_avail) == 0)
		ep = alloexpr();
	expr_avail = ep->parent.ex_ptr;
	ep->ex_op = ExpOp_LeafInteger;
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	ep->left.ex_ptr = 0;
	ep->right.ex_int = num_fromulong((Integer *)0, val);
	ep->ex_type = ExpTy_Integer;
	return ep;
}

#ifndef ALLOC_FLTS_CHUNK
#   define ALLOC_FLTS_CHUNK 200
#endif
static union fltlist
{
	Float		flt;	/* actual value */
	union fltlist	*free;	/* free list link */
} *flt_avail;

Expr *
#ifdef __STDC__
fltexpr(const Uchar *str, size_t len)	/* build expr from floating num */
#else
fltexpr(str, len)Uchar *str; size_t len;
#endif
{
#ifdef FLOATEXPRS
	register Expr *ep;
	register union fltlist *flp;
#ifdef DEBUG
	static Ulong reused;

	if (DEBUG('e') > 0)
		(void)fprintf(stderr, "fltexpr(%.*s)\n", len, str);
#endif
	if ((ep = expr_avail) == 0)
		ep = alloexpr();
	expr_avail = ep->parent.ex_ptr;
	ep->ex_op = ExpOp_LeafFloat;
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	ep->left.ex_ptr = 0;
	if ((flp = flt_avail) != 0)
	{
#ifdef DEBUG
		reused++;
#endif
		flt_avail = flp->free;
	}
	else
	{
		static union fltlist *flt_allo, *end_allo;

		if ((flp = flt_allo) == end_allo)
		{
#ifdef DEBUG
			if (DEBUG('a') > 0)
			{
				static Ulong total;

				(void)fprintf(stderr,
					"Total Floats=%lu @%lu, reused=%lu\n",
					total += ALLOC_FLTS_CHUNK,
					(Ulong)sizeof(union fltlist),
					reused);
			}
#endif
			flp = (union fltlist *)alloc((void *)0,
				ALLOC_FLTS_CHUNK * sizeof(union fltlist));
			end_allo = flp + ALLOC_FLTS_CHUNK;
		}
		flt_allo = flp + 1;
	}
	ep->right.ex_flt = &flp->flt;
	/*
	* The parsing guarantees that there exists at least one
	* byte past the character sequence that denotes the floating
	* constant.  The floating emulation and the tokenization
	* agree about what characters end a floating constant, so it's
	* safe to pass the character pointer and pretend it's a string.
	*/
	errno = 0; /* Different versions of stdio may leave errno set;	 */ 
		   /* fpemu_atox can set errno, so ensure it's clear first. */
	flp->flt = fpemu_atox((const char *)str);
	if (errno != 0)
	{
		errno = 0;	/* ERANGE can be misleading */
		error(gettxt(":796","floating constant out of range: %.*s"), len, str);
	}
	ep->ex_type = ExpTy_Floating;
	return ep;
#else
	fatal(gettxt(":797","fltexpr():creation of floating expression attempted"));
#endif	/*FLOATEXPRS*/
}

Expr *
#ifdef __STDC__
dotexpr(Section *secp)	/* build expression from code (for dot's value) */
#else
dotexpr(secp)Section *secp;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
	{
		(void)fprintf(stderr, "dotexpr(%s)\n",
			secp->sec_sym->sym_name);
	}
#endif
	if ((ep = expr_avail) == 0)
		ep = alloexpr();
	expr_avail = ep->parent.ex_ptr;
	ep->ex_op = ExpOp_LeafCode;
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	ep->left.ex_sect = secp;
	ep->right.ex_code = secp->sec_last;
	ep->ex_type = ExpTy_Relocatable;
	return ep;

}

void		/* set *fp and *lp to where expression originated */
#ifdef __STDC__
exprfrom(Ulong *fp, Ulong *lp, register const Expr *ep)
#else
exprfrom(fp, lp, ep)Ulong *fp, *lp; register Expr *ep;
#endif
{
	/*
	* Only called just prior to error message print.
	*/
	if (ep == 0)	/* can only guess */
	{
		*fp = curfileno;
		*lp = curlineno;
		return;
	}
	for (;;)	/* only loop for Cont_Expr */
	{
		switch (ep->ex_cont)
		{
		default:
			fatal(gettxt(":798","exprfrom():unknown context: %u"),
				(Uint)ep->ex_cont);
			/*NOTREACHED*/
		case Cont_Unknown:	/* still being built */
			*fp = curfileno;
			*lp = curlineno;
			return;
		case Cont_Expr:
			ep = ep->parent.ex_ptr;
			continue;
		case Cont_Set:
			if (!ep->parent.ex_oper->oper_setsym)
				fatal(gettxt(":799","exprfrom():not .set sym w/Cont_Set"));
			*fp = ep->parent.ex_oper->parent.oper_sym->sym_file;
			*lp = ep->parent.ex_oper->parent.oper_sym->sym_line;
			return;
		case Cont_Label:
			*fp = ep->parent.ex_sym->sym_file;
			*lp = ep->parent.ex_sym->sym_line;
			return;
		case Cont_Operand:
			if (ep->parent.ex_oper->oper_setsym)
				fatal(gettxt(":800","exprfrom():.set sym w/Cont_Operand"));
			if (ep->parent.ex_oper->parent.oper_olst == 0)
			{
				*fp = curfileno;
				*lp = curlineno;
				return;
			}
			else
			{
				*fp = ep->parent.ex_oper->
					parent.oper_olst->olst_file;
				*lp = ep->parent.ex_oper->
					parent.oper_olst->olst_line;
			}
			return;
		case Cont_Oplist:	/* bypassed operand node */
			*fp = ep->parent.ex_olst->olst_file;
			*lp = ep->parent.ex_olst->olst_line;
			return;
		}
	}
}

void
#ifdef __STDC__
exprtype(register Expr *ep)	/* check and set expression type(s) */
#else
exprtype(ep)register Expr *ep;
#endif
{
	static const char MSGexp[] = "invalid expression operand type, op %s";
	static const char MSGopr[] = "invalid operand expression type";
	register Expr *el, *er;
	register int result;
	int complain = 0;
	Symbol *sp;
	Operand *op;
	Oplist *olp;

	/*
	* Check for type mismatches and set the resulting type
	* for each expression, propagating the results upward,
	* if they differ from the current version.  Recursively
	* set all expressions that depend on an identifier, if
	* an ExpOp_LeafName is reached.  If the usage context
	* of an expression is a .set symbol, then continue to
	* propagate types if the symbol's expression type changes.
	*
	* This function is called (for the top of the tree) while
	* the expression is being built, and when a symbol's type
	* changes in the symbol table.  In this case, the operator
	* is ExpOp_LeafName, and all the expressions that make use
	* of the same name will be handled by the recursive call.
	*/
	for (;;)
	{
		el = ep->left.ex_ptr;	/* used by all valid cases */
		result = ExpTy_Unknown;	/* by default */
		switch (ep->ex_op)
		{
		default:
			fatal(gettxt(":803","exprtype():unknown operator: %s"),
				tokstr((int)ep->ex_op));
			/*NOTREACHED*/
		case ExpOp_LeafRegister:
		case ExpOp_LeafString:
		case ExpOp_LeafInteger:
		case ExpOp_LeafFloat:
		case ExpOp_LeafCode:
			fatal(gettxt(":804","exprtype():inappropriate leaf operator: %s"),
				tokstr((int)ep->ex_op));
			/*NOTREACHED*/
		case ExpOp_LeafName:
			/*
			* Only here when the type for a symbol is
			* determined.  Propagate the type to all
			* expressions that make use of the symbol.
			*/
			if (el != 0)
				exprtype(el);	/* fix next expr's types */
			result = ep->right.ex_sym->sym_exty;
			ep->ex_mods = ep->right.ex_sym->sym_mods;
			break;
		case ExpOp_Remainder:
		case ExpOp_And:
		case ExpOp_Or:
		case ExpOp_Xor:
		case ExpOp_Nand:
		case ExpOp_LeftShift:
		case ExpOp_RightShift:
		case ExpOp_Maximum:
		case ExpOp_LCM:
			/*
			* Most binary operators only allow
			* integer left and right types and
			* produce integer results.
			*/
			er = ep->right.ex_ptr;
			if (el->ex_type == ExpTy_Integer)
			{
				if (er->ex_type == ExpTy_Integer)
					result = ExpTy_Integer;
				else if (er->ex_type != ExpTy_Unknown)
					complain = 1;
			}
			else if (el->ex_type != ExpTy_Unknown
				|| er->ex_type != ExpTy_Integer
				&& er->ex_type != ExpTy_Unknown)
			{
				complain = 1;
			}
			break;
		case ExpOp_Multiply:
		case ExpOp_Divide:
			/*
			* Multiply, divide allow floating and integer types.
			*/
			switch (ep->right.ex_ptr->ex_type)
			{
			case ExpTy_Integer:
				switch (result = el->ex_type)
				{
				case ExpTy_Unknown:
				case ExpTy_Integer:
				case ExpTy_Floating:
					break;
				default:
					result = ExpTy_Unknown;
					complain = 1;
					break;
				}
				break;
			case ExpTy_Floating:
				switch (el->ex_type)
				{
				case ExpTy_Integer:
				case ExpTy_Floating:
					result = ExpTy_Floating;
					/*FALLTHROUGH*/
				case ExpTy_Unknown:
					break;
				default:
					complain = 1;
					break;
				}
				break;
			case ExpTy_Unknown:
				switch (el->ex_type)
				{
				case ExpTy_Unknown:
				case ExpTy_Integer:
				case ExpTy_Floating:
					break;
				default:
					complain = 1;
					break;
				}
				break;
			default:
				complain = 1;
				break;
			}
			break;
		case ExpOp_Add:
		case ExpOp_Subtract:
			/*
			* The combinations are more complex for binary +/-:
			*
			*		RIGHT
			*   -\+ int flt rel unk other	    KEY
			*      +---+---+---+---+---	u - unknown
			*   int| i | f |e\r| u | e	i - integer
			* L    +---+---+---+---+---	f - floating
			* E flt| f | f | e | u | e	r - relocatable
			* F    +---+---+---+---+---	e - error
			* T rel| r | e |i\e| u | e
			*      +---+---+---+---+---
			*   unk| u | u | u | u | e
			*      +---+---+---+---+---
			* other| e | e | e | e | e
			*/
			er = ep->right.ex_ptr;
			switch (er->ex_type)
			{
			case ExpTy_Unknown:
				switch (el->ex_type)
				{
				case ExpTy_Unknown:
				case ExpTy_Integer:
				case ExpTy_Relocatable:
				case ExpTy_Floating:
					break;
				default:
					complain = 1;
					break;
				}
				break;
			case ExpTy_Integer:
				switch (result = el->ex_type)
				{
				case ExpTy_Unknown:
				case ExpTy_Integer:
				case ExpTy_Relocatable:
				case ExpTy_Floating:
					break;
				default:
					result = ExpTy_Unknown;
					complain = 1;
					break;
				}
				break;
			case ExpTy_Floating:
				switch (el->ex_type)
				{
				case ExpTy_Integer:
				case ExpTy_Floating:
					result = ExpTy_Floating;
					/*FALLTHROUGH*/
				case ExpTy_Unknown:
					break;
				default:
					complain = 1;
					break;
				}
				break;
			case ExpTy_Relocatable:
				if (ep->ex_op == ExpOp_Add)
				{
					if (el->ex_type == ExpTy_Integer)
						result = ExpTy_Relocatable;
					else if (el->ex_type != ExpTy_Unknown)
						complain = 1;
				}
				else if (el->ex_type == ExpTy_Relocatable)
				{
					/*
					* Both sides must either have no
					* modifiers, or be EXPMOD_GOTOFF
					* or EXPMOD_BASEOFF.
					*/
					if (el->ex_mods != er->ex_mods
						|| ((el->ex_mods & ~EXPMOD_OFFSET) != 0))
					{
						complain = 1;
						break;
					}
					result = ExpTy_Integer;
				}
				else if (el->ex_type != ExpTy_Unknown)
					complain = 1;
				break;
			default:
				complain = 1;
				break;
			}
			/*
			* If the result is relocatable, then the relocatable
			* operand must either be only EXPMOD_GOTOFF or
			* EXPMOD_BASEOFF or 0.
			*/
			if (result == ExpTy_Relocatable)
			{
				if (el->ex_type == ExpTy_Relocatable)
				{
					if ((el->ex_mods & ~EXPMOD_OFFSET) != 0)
					{
						complain = 1;
					}
					else
						ep->ex_mods = el->ex_mods;
				}
				else	/* right is relocatable */
				{
					if ((er->ex_mods & ~EXPMOD_OFFSET) != 0)
					{
						complain = 1;
					}
					else
						ep->ex_mods = er->ex_mods;
				}
			}
			break;
		case ExpOp_UnaryAdd:
		case ExpOp_Negate:
			/*
			* Unary + and - allow floating and integer types.
			*/
			switch (result = el->ex_type)
			{
			case ExpTy_Unknown:
			case ExpTy_Integer:
			case ExpTy_Floating:
				break;
			default:
				result = ExpTy_Unknown;
				complain = 1;
				break;
			}
			break;
		case ExpOp_SegmentPart:	/* i386 only */
		case ExpOp_OffsetPart:	/* i386 only */
			fatal(gettxt(":805","exprtype():<s>,<o> not implemented"));
			/*NOTREACHED*/
		case ExpOp_Complement:
			/*
			* Unary ~ only allows integer type.
			*/
			if (el->ex_type == ExpTy_Integer)
				result = ExpTy_Integer;
			else if (el->ex_type != ExpTy_Unknown)
				complain = 1;
			break;
		case ExpOp_LowMask:		/* i860 only */
		case ExpOp_HighMask:		/* i860 only */
		case ExpOp_HighAdjustMask:	/* i860 only */
			/*
			* @l, @h, and @ha allow both integer and
			* relocatable types.  However, at most one
			* of these operations can be done to a
			* relocatable type.
			*/
			switch (result = el->ex_type)
			{
			case ExpTy_Relocatable:
				if (el->ex_mods & EXPMOD_MASK)
				{
			default:
					result = ExpTy_Unknown;
					complain = 1;
					break;
				}
				ep->ex_mods = el->ex_mods | EXPMOD_MASK;
				/*FALLTHROUGH*/
			case ExpTy_Integer:
			case ExpTy_Unknown:
				break;
			}
			break;
		case ExpOp_Pic_PC:
		case ExpOp_Pic_GOT:
		case ExpOp_Pic_PLT:
			/*
			* These PIC operators can only be applied to
			* relocatable types.  No modifiers can have
			* been applied to the expression.
			*/
			ep->ex_mods = EXPMOD_PIC;
		pictype:;
			if (el->ex_mods != 0)
				complain = 1;
			else if (el->ex_type == ExpTy_Relocatable)
				result = ExpTy_Relocatable;
			else if (el->ex_type != ExpTy_Unknown)
				complain = 1;
			break;
			/*
			* GOTOFF and BASEOFF differ from the other PIC
			* operators * in that they permit further 
			* arithmetic except for the difference of 
			* two relocatables with only one having GOTOFF 
			* or BASEOFF.  No other modifiers
			* can have been applied to the expression.
			*/
		case ExpOp_Pic_GOTOFF:
			ep->ex_mods = EXPMOD_GOTOFF;
			goto pictype;
		case ExpOp_Pic_BASEOFF:
			ep->ex_mods = EXPMOD_BASEOFF;
			goto pictype;
		}
		if (complain)	/* issue diagnostic */
		{
			exprerror(ep, gettxt(":801",MSGexp), tokstr((int)ep->ex_op));
			complain = 0;
		}
		if (ep->ex_type == result)
			return;		/* no use propagating further */
		ep->ex_type = result;
		switch (ep->ex_cont)
		{
		default:
			fatal(gettxt(":806","exprtype():unknown context: %u"),
				(Uint)ep->ex_cont);
			/*NOTREACHED*/
		case Cont_Label:
			fatal(gettxt(":807","exprtype():at label context"));
			/*NOTREACHED*/
		case Cont_Expr:
			ep = ep->parent.ex_ptr;
			break;
		case Cont_Oplist:
			fatal(gettxt(":808","exprtype():at oplist context"));
			/*NOTREACHED*/
		case Cont_Unknown:
			return;		/* expression still being built */
		case Cont_Set:
			/*
			* Update expression type for .set symbol.
			* Propagate the new type to all dependent
			* expressions, if any.
			*/
			op = ep->parent.ex_oper;
			if (!op->oper_setsym)
				fatal(gettxt(":809","exprtype():not .set w/Cont_Set"));
			sp = op->parent.oper_sym;
			if (sp->sym_exty != ExpTy_Unknown)
				return;		/* already handled */
			if (op->oper_flags != 0)
				result = extyamode(op);
			if (result == ExpTy_Unknown)
				return;		/* don't bother */
			sp->sym_exty = result;
			sp->sym_mods = ep->ex_mods;
			if ((ep = sp->sym_uses) == 0)
				return;
			break;
		case Cont_Operand:
			/*
			* Check expected type, if any, for the operand.
			* Otherwise, if it is an instruction operand,
			* check it out.
			*/
			op = ep->parent.ex_oper;
			if (op->oper_tybits != 0)
			{
				if (!(op->oper_tybits & BIT(result)))
				{
					if ((olp = op->parent.oper_olst) == 0)
						fatal(gettxt(":810","exprtype():olp==0 [1]"));
					backerror((Ulong)olp->olst_file,
						olp->olst_line, gettxt(":812",MSGopr));
				}
			}
			else if ((olp = op->parent.oper_olst) == 0)
				fatal(gettxt(":811","exprtype():olp==0 [2]"));
			else if (olp->olst_code != 0)
				operinst(olp->olst_code->info.code_inst, op);
			return;
		}
	}
}

Expr *
#ifdef __STDC__
setlessexpr(register const Expr *ep) /* return typed expr, skipping .set's */
#else
setlessexpr(ep)register Expr *ep;
#endif
{
	register Symbol *sp;

	if (ep->ex_type == ExpTy_Unknown)
		fatal(gettxt(":791","setlessexpr():typeless expression; looping possible"));
	while (ep->ex_op == ExpOp_LeafName)
	{
		sp = ep->right.ex_sym;
		if (sp->sym_kind != SymKind_Set)
			fatal(gettxt(":813","setlessexpr():non-.set name"));
		if ((ep = sp->addr.sym_oper->oper_expr) == 0)
			fatal(gettxt(":802","setlessexpr():no expr for simple type .set"));
	}
	return (Expr *)ep;
}

void
#ifdef __STDC__
fixexpr(register Expr *ep) /* "." -> ExpOp_LeafCode and save strings */
#else
fixexpr(ep)register Expr *ep;
#endif
{
	Section *secp;
	Symbol *sp;

	while ((int)ep->ex_op >= ExpOp_OPER_LeafLast) /* until reach leaf */
	{
		if ((int)ep->ex_op < ExpOp_OPER_BinaryLast)
			fixexpr(ep->right.ex_ptr);
		ep = ep->left.ex_ptr;
	}
	if (ep->ex_op == ExpOp_LeafName)
	{
		sp = ep->right.ex_sym;
		sp->sym_refd = 1;	/* symbol referenced */
		if (sp->sym_kind == SymKind_Dot)
		{
			/*
			* Overwrite identifier (.) with ExpOp_LeafCode
			* node.  This breaks the chain of uses, but
			* they are never accessed for . since . cannot
			* be defined by the user.
			*/
			ep->ex_op = ExpOp_LeafCode;
			secp = cursect();
			ep->left.ex_sect = secp;
			ep->right.ex_code = secp->sec_last;
		}
		return;
	}
	else if (ep->ex_op != ExpOp_LeafString)
		return;
	ep->right.ex_str = savestr(ep->right.ex_str, ep->left.ex_len);
}

Expr *
#ifdef __STDC__
unaryexpr(int op, register Expr *el)	/* build unary expression */
#else
unaryexpr(op, el)int op; register Expr *el;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
	{
		(void)fprintf(stderr, "unaryexpr(op=%s,el=", tokstr(op));
		printexpr(el);
		(void)fputs(")\n", stderr);
	}
#endif
	if (op == ExpOp_Subtract)
		op = ExpOp_Negate;
	else if (op == ExpOp_Add)
		op = ExpOp_UnaryAdd;
	else if (op < ExpOp_OPER_UnaryFirst || op >= ExpOp_OPER_UnaryLast)
		fatal(gettxt(":815","unaryexpr():non-unary operator %s"), tokstr(op));
	if ((ep = expr_avail) == 0)
		ep = alloexpr();
	expr_avail = ep->parent.ex_ptr;
	ep->ex_op = op;
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	ep->left.ex_ptr = el;
	ep->right.ex_ptr = 0;
	ep->ex_type = ExpTy_Unknown;
	el->parent.ex_ptr = ep;
	el->ex_cont = Cont_Expr;
	exprtype(ep);
	return ep;
}

Expr *
#ifdef __STDC__
binaryexpr(int op, register Expr *el, register Expr *er) /* build binary */
#else
binaryexpr(op, el, er)int op; register Expr *el, *er;
#endif
{
	register Expr *ep;

#ifdef DEBUG
	if (DEBUG('e') > 0)
	{
		(void)fprintf(stderr, "binaryexpr(op=%s,el=", tokstr(op));
		printexpr(el);
		(void)fputs(",er=", stderr);
		printexpr(er);
		(void)fputs(")\n", stderr);
	}
#endif
	if (op < ExpOp_OPER_BinaryFirst || op >= ExpOp_OPER_BinaryLast)
		fatal(gettxt(":816","binaryexpr():non-binary opertor %s"), tokstr(op));
	if ((ep = expr_avail) == 0)
		ep = alloexpr();
	expr_avail = ep->parent.ex_ptr;
	ep->ex_op = op;
	ep->ex_cont = Cont_Unknown;
	ep->parent.ex_ptr = 0;
	ep->ex_mods = 0;
	ep->left.ex_ptr = el;
	ep->right.ex_ptr = er;
	ep->ex_type = ExpTy_Unknown;
	el->parent.ex_ptr = ep;
	er->parent.ex_ptr = ep;
	el->ex_cont = Cont_Expr;
	er->ex_cont = Cont_Expr;
	exprtype(ep);
	return ep;
}

void
#ifdef __STDC__
exprfree(Expr *ep)	/* return entire tree to free list */
#else
exprfree(ep)Expr *ep;
#endif
{
	Expr **epp;

	if ((int)ep->ex_op >= ExpOp_OPER_BinaryFirst)
	{
		if ((int)ep->ex_op < ExpOp_OPER_BinaryLast)
			exprfree(ep->right.ex_ptr);
		exprfree(ep->left.ex_ptr);
	}
#ifdef DEBUG
	if (DEBUG('a') > 2)
		(void)fprintf(stderr, "Returning Expr @%#lx\n", (Ulong)ep);
#endif
	switch (ep->ex_op)	/* special cases */
	{
	case ExpOp_LeafName:
		/*
		* Disconnect name from uses list.  This node is almost
		* always the first on the list.
		*/
		for (epp = &ep->right.ex_sym->sym_uses; *epp != ep;
			epp = &(*epp)->left.ex_ptr)
		{
		}
		*epp = ep->left.ex_ptr;
		break;
	case ExpOp_LeafInteger:
		/*
		* Permit reuse of number.
		*/
		num_free((Integer *)ep->right.ex_int);
		break;
#ifdef FLOATEXPRS
	case ExpOp_LeafFloat:
#ifdef DEBUG
		if (DEBUG('a') > 2)
		{
			(void)fprintf(stderr, "Returning Float @%#lx\n",
				(Ulong)ep->right.ex_flt);
		}
#endif
		((union fltlist *)ep->right.ex_flt)->free = flt_avail;
		flt_avail = (union fltlist *)ep->right.ex_flt;
		break;
#endif	/*FLOATEXPRS*/
	}
	ep->parent.ex_ptr = expr_avail;
	expr_avail = ep;
}

#ifndef ALLOC_OPER_CHUNK
#   define ALLOC_OPER_CHUNK 3000
#endif
static Operand *oper_avail;	/* Operand free list */

Operand *
#ifdef __STDC__
operand(Expr *ep)	/* build simple single operand */
#else
operand(ep)Expr *ep;
#endif
{
	static const Operand empty = {0};
	register Operand *op;
#ifdef DEBUG
	static Ulong reused;

	if (DEBUG('e') > 0)
	{
		(void)fputs("operand(ep=", stderr);
		printexpr(ep);
		(void)fputs(")\n", stderr);
	}
#endif
	if ((op = oper_avail) != 0)
	{
#ifdef DEBUG
		reused++;
#endif
		oper_avail = op->parent.oper_free;
	}
	else
	{
		static Operand *oper_allo, *end_allo;

		if ((op = oper_allo) == end_allo)
		{
#ifdef DEBUG
			if (DEBUG('a') > 0)
			{
				static Ulong total;

				(void)fprintf(stderr,
					"Total Operands=%lu @%lu, reused=%lu\n",
					total += ALLOC_OPER_CHUNK,
					(Ulong)sizeof(Operand), reused);
			}
#endif
			op = (Operand *)alloc((void *)0,
				ALLOC_OPER_CHUNK * sizeof(Operand));
			end_allo = op + ALLOC_OPER_CHUNK;
		}
		oper_allo = op + 1;
	}
	*op = empty;
	if ((op->oper_expr = ep) != 0)
	{
		ep->ex_cont = Cont_Operand;
		ep->parent.ex_oper = op;
	}
	return op;
}

void
#ifdef __STDC__
operfree(Operand *op)	/* return entire operand to their free lists */
#else
operfree(op)Operand *op;
#endif
{
	Operand **opp;
	Expr *ep;

	if (op == 0)
		return;
#ifdef DEBUG
	if (DEBUG('a') > 2)
		(void)fprintf(stderr, "Returning Operand @%#lx\n", (Ulong)op);
#endif
	if (op->oper_flags != 0)
		amodefree(op);
	if ((ep = op->oper_expr) != 0)
		exprfree(ep);
	opp = &op->parent.oper_olst->olst_first;
	while (*opp != op)
		opp = &(*opp)->oper_next;
	*opp = op->oper_next;
	if (op->oper_setsym != (Uchar)~(Uint)0)	/* not on free list */
	{
		op->oper_setsym = (Uchar)~(Uint)0;	/* is on free list */
		op->parent.oper_free = oper_avail;
		oper_avail = op;
	}
}

void
#ifdef __STDC__
cutoper(Operand *op)	/* return only operand node to free list */
#else
cutoper(op)Operand *op;
#endif
{
	Expr *ep;

	if (op->oper_flags != 0)
		fatal(gettxt(":817","cutoper():cannot bypass amoded operand"));
#ifdef DEBUG
	if (DEBUG('a') > 2)
		(void)fprintf(stderr, "Returning Operand @%#lx\n", (Ulong)op);
#endif
	if ((ep = op->oper_expr) != 0)
	{
		ep->ex_cont = Cont_Oplist;
		ep->parent.ex_olst = op->parent.oper_olst;
	}
	op->oper_setsym = (Uchar)~(Uint)0;	/* is on free list */
	op->parent.oper_free = oper_avail;
	oper_avail = op;
}

#ifndef ALLOC_OLST_CHUNK
#   define ALLOC_OLST_CHUNK 2000
#endif
static Oplist *olst_avail;	/* Oplist free list */

Oplist *
#ifdef __STDC__
oplist(register Oplist *list, register Operand *op) /* add operand to list */
#else
oplist(list, op)register Oplist *list; register Operand *op;
#endif
{
#ifdef DEBUG
	static Ulong reused;

	if (DEBUG('e') > 0)
	{
		(void)fprintf(stderr, "oplist(list=%#lx,op=%#lx)\n",
			(Ulong)list, (Ulong)op);
	}
#endif
	if (list != 0)	/* append to existing list */
		list->last.olst_last->oper_next = op;
	else if ((list = olst_avail) != 0)	/* first time for this list */
	{
#ifdef DEBUG
		reused++;
#endif
		olst_avail = list->last.olst_free;
		list->olst_first = op;
		list->olst_code = 0;
		list->olst_line = curlineno;
		list->olst_file = curfileno;
	}
	else	/* first time for this list and no linked free list */
	{
		static Oplist *olst_allo, *end_allo;

		if ((list = olst_allo) == end_allo)
		{
#ifdef DEBUG
			if (DEBUG('a') > 0)
			{
				static Ulong total;

				(void)fprintf(stderr,
					"Total Oplists=%lu @%lu, reused=%lu\n",
					total += ALLOC_OLST_CHUNK,
					(Ulong)sizeof(Oplist), reused);
			}
#endif
			list = (Oplist *)alloc((void *)0,
				ALLOC_OLST_CHUNK * sizeof(Oplist));
			end_allo = list + ALLOC_OLST_CHUNK;
		}
		olst_allo = list + 1;
		list->olst_first = op;
		list->olst_code = 0;
		list->olst_line = curlineno;
		list->olst_file = curfileno;
	}
	if ((list->last.olst_last = op) != 0)	/* only 0 for no-op instrs */
		op->parent.oper_olst = list;
	return list;
}

void
#ifdef __STDC__
olstfree(Operand *op)	/* return entire operand list to free lists */
#else
olstfree(op)Operand *op;
#endif
{
	Oplist *olp;

	if (op == 0 || op->oper_setsym == (Uchar)~(Uint)0) /* Oplist lost */
		return;
	olp = op->parent.oper_olst;
#ifdef DEBUG
	if (DEBUG('a') > 2)
		(void)fprintf(stderr, "Returning Oplist @%#lx\n", (Ulong)olp);
#endif
	for (op = olp->olst_first; op != 0; op = op->oper_next)
		operfree(op);
	olp->last.olst_free = olst_avail;
	olst_avail = olp;
	olp->olst_first = (Operand *)olp;	/* pretend it's still used */
}

void
#ifdef __STDC__
cutolst(Oplist *olp)	/* return oplist node to free list */
#else
cutolst(olp)Oplist *olp;
#endif
{
#ifdef DEBUG
	if (DEBUG('a') > 2)
		(void)fprintf(stderr, "Returning Oplist @%#lx\n", (Ulong)olp);
#endif
	olp->last.olst_free = olst_avail;
	olst_avail = olp;
	olp->olst_first = (Operand *)olp;	/* pretend it's still used */
}

void		/* Code being moved from "oldcp" to "newcp" */
		/* adjust the ex_code field of any ExpTy_LeafCode */
		/* in any node of this expression. */
#ifdef __STDC__
movexcode(Expr *exp, Code *newcp, Code *oldcp)
#else
movexcode(exp, newcp, oldcp)Expr *exp; Code *newcp, *oldcp;
#endif
{
	register Uchar op;
#ifdef DEBUG
#endif
	while (exp != NULL)
	{
		if ((op = exp->ex_op) == ExpOp_LeafCode)
		{
			if (exp->right.ex_code == oldcp)
				exp->right.ex_code = newcp;
		}
		else if (op >= ExpOp_OPER_LeafFirst
				&& op < ExpOp_OPER_LeafLast ) {
			return;
		}
		else if (op >= ExpOp_OPER_BinaryFirst
				&& op < ExpOp_OPER_BinaryLast)
		{
			movexcode(exp->right.ex_ptr, newcp, oldcp);
		}
		/* Walk down the left tree. */
		exp = exp->left.ex_ptr;
	}
}

void		/* Code being moved from "oldcp" to "newcp" */
		/* adjust the ex_code field of any ExpTy_LeafCode */
		/* in any of the operands. */
#ifdef __STDC__
movopexcode(Oplist *olp, Code *newcp, Code *oldcp)
#else
movopexcode(olp, newcp, oldcp)Oplist *olp; Code *newcp, *oldcp;
#endif
{
	Operand * op;
#ifdef DEBUG
#endif
	for (op = olp->olst_first; op != 0; op = op->oper_next)
		movexcode(op->oper_expr, newcp, oldcp);
}

#ifdef DEBUG

void
#ifdef __STDC__
printexpr(const Expr *ep)	/* output contents of expr */
#else
printexpr(ep)Expr *ep;
#endif
{
	if (ep == 0)
	{
		(void)fputs("(Expr*)0", stderr);
		return;
	}
	switch (ep->ex_type)	/* prefix expression with type */
	{
	default:
		(void)fprintf(stderr, "{%d?", ep->ex_type);
		break;
	case ExpTy_Register:
		(void)fputs("{%", stderr);
		break;
	case ExpTy_Operand:
		(void)fputs("{o", stderr);
		break;
	case ExpTy_Floating:
		(void)fputs("{f", stderr);
		break;
	case ExpTy_String:
		(void)fputs("{s", stderr);
		break;
	case ExpTy_Integer:
		(void)fputs("{i", stderr);
		break;
	case ExpTy_Relocatable:
		(void)fputs("{r", stderr);
		break;
	case ExpTy_Unknown:
		(void)fputs("{u", stderr);
		break;
	}
	if (ep->ex_mods == 0)
		(void)putc('}', stderr);
	else
		(void)fprintf(stderr, ",%#x}", ep->ex_mods);
	switch (ep->ex_op)	/* handle leaves specially */
	{
	case ExpOp_LeafRegister:
		(void)fprintf(stderr, "%%%d", ep->right.ex_reg);
		return;
	case ExpOp_LeafName:
		(void)fputs((char *)ep->right.ex_sym->sym_name, stderr);
		return;
	case ExpOp_LeafString:
		(void)fprintf(stderr, "\"%s\"",
			prtstr(ep->right.ex_str, ep->left.ex_len));
		return;
	case ExpOp_LeafInteger:
		(void)fputs(num_tohex(ep->right.ex_int), stderr);
		return;
	case ExpOp_LeafFloat:
#ifdef FLOATEXPRS
		(void)fputs(fpemu_xtoa(*ep->right.ex_flt), stderr);
#else
		(void)fprintf(stderr, "%#lx", (Ulong)ep->right.ex_flt);
#endif
		return;
	case ExpOp_LeafCode:
		(void)fprintf(stderr, ".=<%s>",
			ep->left.ex_sect->sec_sym->sym_name);
		return;
	}
	(void)putc('[', stderr);
	if ((int)ep->ex_op >= ExpOp_OPER_BinaryFirst
		&& (int)ep->ex_op < ExpOp_OPER_BinaryLast)
	{
		printexpr(ep->left.ex_ptr);
		(void)fputs(tokstr((int)ep->ex_op), stderr);
		printexpr(ep->right.ex_ptr);
	}
	else if ((int)ep->ex_op >= ExpOp_OPER_PostfixFirst
		&& (int)ep->ex_op < ExpOp_OPER_PostfixLast)
	{
		printexpr(ep->left.ex_ptr);
		(void)fputs(tokstr((int)ep->ex_op), stderr);
	}
	else if ((int)ep->ex_op >= ExpOp_OPER_PrefixFirst
		&& (int)ep->ex_op < ExpOp_OPER_PrefixLast)
	{
		(void)fputs(tokstr((int)ep->ex_op), stderr);
		printexpr(ep->left.ex_ptr);
	}
	else
	{
		fatal(gettxt(":818","printexpr():unknown group for operator: %s"),
			tokstr((int)ep->ex_op));
	}
	(void)putc(']', stderr);
}

void
#ifdef __STDC__
printoplist(const Oplist *olp)	/* output contents of operand list */
#else
printoplist(olp)Oplist *olp;
#endif
{
	Operand *op;

	if (olp == 0)
	{
		(void)fputs("(Oplist*)0", stderr);
		return;
	}
	for (op = olp->olst_first;; op = op->oper_next)
	{
		printoperand(op);
		if (op == olp->last.olst_last)
			break;
		(void)putc(',', stderr);
	}
}

#endif	/*DEBUG*/
