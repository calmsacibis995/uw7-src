#ident	"@(#)nas:common/eval.c	1.8"
/*
* common/eval.c - common assembler expression evaluation
*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "common/as.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/gram.h"
#include "common/sect.h"
#include "common/syms.h"

#include "intemu.h"
#ifdef FLOATEXPRS
#   include "fpemu.h"
#endif

static Integer	zero_int;
static Integer	one_int;
static Integer	low16_int;

static
#ifdef __STDC__
void *
ev_num_alloc(void *p, size_t nb) /* (re)allocation request from intemu */
#else
char *
ev_num_alloc(p, nb)char *p; size_t nb;
#endif
{
	static union intlist
	{
		Integer		num;	/* actual number */
		union intlist	*free;	/* free list link */
	} *avail;
	register union intlist *np;
#ifdef DEBUG
	static Ulong reused;
#endif

	if (nb == 0)	/* deallocate Integer */
	{
#ifdef DEBUG
		if (DEBUG('a') > 2)
		{
			(void)fprintf(stderr, "Returning Integer @%#lx\n",
				(Ulong)p);
		}
#endif
		np = (union intlist *)p;
		np->free = avail;
		avail = np;
		return 0;
	}
	if (nb != sizeof(Integer))	/* misc. (re)alloc request */
		return alloc(p, nb);
	/*
	* Must be a request for a new Integer.
	*/
#ifndef ALLOC_INTS_CHUNK
#   define ALLOC_INTS_CHUNK 300
#endif
	if ((np = avail) != 0)
	{
#ifdef DEBUG
		reused++;
#endif
		avail = np->free;
	}
	else
	{
		static union intlist *int_allo, *end_allo;

		if ((np = int_allo) == end_allo)
		{
#ifdef DEBUG
			if (DEBUG('a') > 0)
			{
				static Ulong total;

				(void)fprintf(stderr,
					"Total Integers=%lu @%lu, reused=%lu\n",
					total += ALLOC_INTS_CHUNK,
					(Ulong)sizeof(union intlist),
					reused);
			}
#endif
			np = (union intlist *)alloc((void *)0,
				ALLOC_INTS_CHUNK * sizeof(union intlist));
			end_allo = np + ALLOC_INTS_CHUNK;
		}
		int_allo = np + 1;
	}
#ifdef __STDC__
	return np;
#else
	return (char *)np;
#endif
}

void
#ifdef __STDC__
initeval(void)	/* initialize */
#else
initeval()
#endif
{
	/*CONSTANTCONDITION*/
	if ((1 + ((Uchar)~(Uint)0)) != BIT(CHAR_BIT))
		fatal(gettxt(":777","initeval():bad CHAR_BIT value: %d"), CHAR_BIT);
	num_init(fatal, ev_num_alloc);
	(void)num_fromulong(&zero_int, (Ulong)0);
	(void)num_fromulong(&one_int, (Ulong)1);
	(void)num_fromulong(&low16_int, (Ulong)0xffff);
}

#ifdef FLOATEXPRS
static void
#ifdef __STDC__
evaltofp(Float *dst, const Eval *vp)	/* convert int eval to float */
#else
evaltofp(dst, vp)Float *dst; Eval *vp;
#endif
{
	static const char MSGrng[] =
		"integer out of range for floating conversion: %s";
	char *sval = 0;
	Ulong ulval;
	long lval;

	errno = 0; /* Different versions of stdio may leave errno set;  */ 
		   /* fpemu_ultox, fpemu_ltox, and fpemu_atox can set errno, */
		   /* so ensure it's clear first. 				*/    
	if (num_toulong(vp->ev_int, &ulval) == 0)
		*dst = fpemu_ultox(ulval);
	else if (num_toslong(vp->ev_int, &lval) == 0)
		*dst = fpemu_ltox(lval);
	else	/* go through printable decimal intermediate */
	{
		if (vp->ev_flags & EV_CONST)	/* can't be negative */
			sval = num_toudec(vp->ev_int);
		else
			sval = num_tosdec(vp->ev_int);
		*dst = fpemu_atox(sval);
	}
	if (errno != 0)
	{
		errno = 0;	/* ERANGE can be misleading */
		if (sval == 0)
		{
			if (vp->ev_flags & EV_CONST)	/* can't be negative */
				sval = num_toudec(vp->ev_int);
			else
				sval = num_tosdec(vp->ev_int);
		}
		exprerror(vp->ev_expr, gettxt(":778",MSGrng), sval);
	}
}
#endif	/*FLOATEXPRS*/

static void
#ifdef __STDC__
eval(register const Expr *ep, register Eval *vp) /* evaluate int/rel[/flt] expr */
#else
eval(ep,vp)register Expr *ep; register Eval *vp;
#endif
{
	static const char MSGsub[] =
		"defined relocatable values from the same section required, op -";
#ifdef FLOATEXPRS
	Float rhs_flt;
#endif
	Symbol *sp;
	Eval rhs;
	Integer rhs_int;
	int shift;

	if (ep == 0)
		fatal(gettxt(":780","eval():null expression"));
	rhs.ev_flags = 0;
	if ((int)ep->ex_op >= ExpOp_OPER_BinaryFirst)
	{
		if ((int)ep->ex_op < ExpOp_OPER_BinaryLast)
		{
			rhs.ev_int = &rhs_int;
#ifdef FLOATEXPRS
			rhs.ev_flt = &rhs_flt;
#endif
			rhs.ev_expr = ep->right.ex_ptr;
			eval(ep->right.ex_ptr, &rhs);
			vp->ev_flags |= rhs.ev_flags & (EV_OFLOW | EV_LDIFF);
		}
		eval(ep->left.ex_ptr, vp);	/* share lhs with parent */
	}
	/*
	* Handle this operation.  The types are assumed to be correct for
	* the operation: exprtype() should have rejected them prior to now.
	*/
	switch (ep->ex_op)
	{
	default:
		fatal(gettxt(":781","eval():unknown operator: %s"), tokstr((int)ep->ex_op));
		/*NOTREACHED*/

		/*
		* Leaf operators.
		*/
	case ExpOp_LeafFloat:
#ifdef FLOATEXPRS
		vp->ev_flags |= EV_FLOAT | EV_CONST;
		*vp->ev_flt = *ep->right.ex_flt;
		return;
#endif
	case ExpOp_LeafRegister:
	case ExpOp_LeafString:
		fatal(gettxt(":782","eval():inappropriate leaf: %s"), tokstr(ep->ex_op));
		/*NOTREACHED*/
	case ExpOp_LeafName:	/* value depends on symbol kind */
		switch ((sp = ep->right.ex_sym)->sym_kind)
		{
		case SymKind_Dot:	/* current section's value of . */
			vp->ev_sym = 0;
			vp->ev_pic = 0;
			vp->ev_mask = 0;
			vp->ev_sec = cursect();
			vp->ev_dot = vp->ev_sec->sec_last;
			(void)num_fromulong(vp->ev_int, vp->ev_sec->sec_size);
			vp->ev_flags |= EV_RELOC;
			break;
		case SymKind_Set:	/* value of the .set symbol */
			eval(sp->addr.sym_oper->oper_expr, vp);
			/*
			* Special case for TOKTY_IDPICOPs: if this .set
			* is the subject of one, relocation code needs
			* to know that this name was so modified.
			*/
			if ((vp->ev_flags & EV_RELOC)
				&& ep->ex_cont == Cont_Expr)
			{
				int op = ep->parent.ex_ptr->ex_op;

				if (op == ExpOp_Pic_GOT || op == ExpOp_Pic_PLT)
					vp->ev_sym = sp;
			}
			break;
		case SymKind_GOT:	/* "_GLOBAL_OFFSET_TABLE_" */
			vp->ev_flags |= EV_G_O_T;
			/*FALLTHROUGH*/
		default:	/* regular or common symbol */
			if ((ep = sp->addr.sym_expr) != 0)
			{
				if (ep->ex_op != ExpOp_LeafCode)
				{
					fatal(gettxt(":783","eval():nonlabel name: %s"),
						(const char *)sp->sym_name);
				}
				vp->ev_sym = sp;
				goto codeval;
			}
			else if (sp->sym_exty != ExpTy_Relocatable)
			{
				fatal(gettxt(":784","eval():null expr symbol: %s"),
					(const char *)sp->sym_name);
			}
			else	/* (as yet) undefined symbol */
			{
				vp->ev_sym = sp;
				vp->ev_pic = 0;
				vp->ev_mask = 0;
				vp->ev_sec = 0;
				vp->ev_dot = 0;
				*vp->ev_int = zero_int;
				vp->ev_flags |= EV_RELOC;
			}
			break;
		}
		return;
	case ExpOp_LeafCode:
		vp->ev_sym = 0;
	codeval:;
		vp->ev_pic = 0;
		vp->ev_mask = 0;
		vp->ev_sec = ep->left.ex_sect;
		vp->ev_dot = ep->right.ex_code;
		(void)num_fromulong(vp->ev_int, ep->right.ex_code->code_addr);
		vp->ev_flags |= EV_RELOC;
		return;
	case ExpOp_LeafInteger:
		vp->ev_flags |= EV_CONST;
		*vp->ev_int = *ep->right.ex_int;
		return;

		/*
		* Unary operators.
		*/
	case ExpOp_Complement:
		num_complement(vp->ev_int);
		break;
	case ExpOp_UnaryAdd:
		break;
	case ExpOp_Negate:
#ifdef FLOATEXPRS
		if (ep->ex_type == ExpTy_Floating)
		{
			*vp->ev_flt = fpemu_neg(*vp->ev_flt);
			break;
		}
#endif
		if (num_negate(vp->ev_int) != 0)
			vp->ev_flags |= EV_OFLOW;
		break;
	case ExpOp_SegmentPart:
	case ExpOp_OffsetPart:
		fatal(gettxt(":785","eval():<s> and <o> are not implemented"));
		/*NOTREACHED*/
	case ExpOp_Pic_PC:
	case ExpOp_Pic_GOT:
	case ExpOp_Pic_PLT:
	case ExpOp_Pic_GOTOFF:
	case ExpOp_Pic_BASEOFF:
		/*
		* Perform no computation.  Just note the presence of the
		* operation for relocation use.
		*/
		vp->ev_pic = ep->ex_op;
		break;
	case ExpOp_LowMask:	/* only compute if not relocatable */
		if (vp->ev_flags & EV_RELOC)
			vp->ev_mask = ep->ex_op;
		else
			num_and(vp->ev_int, &low16_int);
		break;
	case ExpOp_HighMask:	/* only compute if not relocatable */
		if (vp->ev_flags & EV_RELOC)
			vp->ev_mask = ep->ex_op;
		else
		{
			(void)num_lrshift(vp->ev_int, 16);
			num_and(vp->ev_int, &low16_int);
		}
		break;
	case ExpOp_HighAdjustMask:	/* only compute if not relocatable */
		if (vp->ev_flags & EV_RELOC)
			vp->ev_mask = ep->ex_op;
		else
		{
			Ulong val;

			(void)num_toulong(vp->ev_int, &val);
			(void)num_lrshift(vp->ev_int, 16);
			if (val & BIT(15))
				(void)num_uadd(vp->ev_int, &one_int);
			num_and(vp->ev_int, &low16_int);
		}
		break;

		/*
		* Binary operators.
		*/
	case ExpOp_Multiply:
#ifdef FLOATEXPRS
		if (ep->ex_type == ExpTy_Floating)
		{
			if (!(rhs.ev_flags & EV_FLOAT))
				evaltofp(&rhs_flt, &rhs);
			else if (!(vp->ev_flags & EV_FLOAT))
			{
				evaltofp(vp->ev_flt, vp);
				vp->ev_flags |= EV_FLOAT;
			}
			*vp->ev_flt = fpemu_mul(*vp->ev_flt, rhs_flt);
			if (errno != 0)
			{
				errno = 0;
				vp->ev_flags |= EV_OFLOW;
			}
			break;
		}
#endif
		if (num_smultiply(vp->ev_int, &rhs_int) != 0)
			vp->ev_flags |= EV_OFLOW;
		break;
	case ExpOp_Divide:
#ifdef FLOATEXPRS
		if (ep->ex_type == ExpTy_Floating)
		{
			if (!(rhs.ev_flags & EV_FLOAT))
				evaltofp(&rhs_flt, &rhs);
			else if (!(vp->ev_flags & EV_FLOAT))
			{
				evaltofp(vp->ev_flt, vp);
				vp->ev_flags |= EV_FLOAT;
			}
			*vp->ev_flt = fpemu_div(*vp->ev_flt, rhs_flt);
			if (errno != 0 || fpemu_iszero(rhs_flt))
			{
				errno = 0;
				vp->ev_flags |= EV_OFLOW;
			}
			break;
		}
#endif
		if (num_sdivide(vp->ev_int, &rhs_int) != 0)
			vp->ev_flags |= EV_OFLOW;
		break;
	case ExpOp_Remainder:
		if (num_sremainder(vp->ev_int, &rhs_int) != 0)
			vp->ev_flags |= EV_OFLOW;
		break;
	case ExpOp_Subtract:
#ifdef FLOATEXPRS
		if (ep->ex_type == ExpTy_Floating)
		{
			if (!(rhs.ev_flags & EV_FLOAT))
				evaltofp(&rhs_flt, &rhs);
			else if (!(vp->ev_flags & EV_FLOAT))
			{
				evaltofp(vp->ev_flt, vp);
				vp->ev_flags |= EV_FLOAT;
			}
			rhs_flt = fpemu_neg(rhs_flt);
			*vp->ev_flt = fpemu_add(*vp->ev_flt, rhs_flt);
			if (errno != 0)
			{
				errno = 0;
				vp->ev_flags |= EV_OFLOW;
			}
			break;
		}
#endif
		if (rhs.ev_flags & EV_RELOC)
		{
			if (!(vp->ev_flags & EV_RELOC))
			{
			badsub:;
				exprerror(ep, gettxt(":779",MSGsub));
			}
			else if (vp->ev_sec == 0 || rhs.ev_sec == 0)
			{
				if (vp->ev_sym != rhs.ev_sym)
					goto badsub;
				else if (vp->ev_sym == 0)
					fatal(gettxt(":786","eval():RELOC w/out base"));
			}
			else if (vp->ev_sec != rhs.ev_sec)
				goto badsub;
			vp->ev_flags &= ~EV_RELOC;
			vp->ev_flags |= EV_LDIFF;
		}
		if (num_ssubtract(vp->ev_int, &rhs_int) != 0)
			vp->ev_flags |= EV_OFLOW;
		break;
	case ExpOp_Add:
#ifdef FLOATEXPRS
		if (ep->ex_type == ExpTy_Floating)
		{
			if (!(rhs.ev_flags & EV_FLOAT))
				evaltofp(&rhs_flt, &rhs);
			else if (!(vp->ev_flags & EV_FLOAT))
			{
				evaltofp(vp->ev_flt, vp);
				vp->ev_flags |= EV_FLOAT;
			}
			*vp->ev_flt = fpemu_add(*vp->ev_flt, rhs_flt);
			if (errno != 0)
			{
				errno = 0;
				vp->ev_flags |= EV_OFLOW;
			}
			break;
		}
#endif
		if (num_sadd(vp->ev_int, &rhs_int) != 0)
			vp->ev_flags |= EV_OFLOW;
		if (rhs.ev_flags & EV_RELOC)
		{
			vp->ev_sym = rhs.ev_sym;
			vp->ev_pic = rhs.ev_pic;
			vp->ev_mask = rhs.ev_mask;
			vp->ev_sec = rhs.ev_sec;
			vp->ev_dot = rhs.ev_dot;
			vp->ev_flags |= EV_RELOC;
		}
		break;
	case ExpOp_Nand:
		num_complement(&rhs_int);
		/*FALLTHROUGH*/
	case ExpOp_And:
		num_and(vp->ev_int, &rhs_int);
		break;
	case ExpOp_Or:
		num_or(vp->ev_int, &rhs_int);
		break;
	case ExpOp_Xor:
		num_xor(vp->ev_int, &rhs_int);
		break;
	case ExpOp_LeftShift:
		if (num_tosint(&rhs_int, &shift) != 0
			|| num_llshift(vp->ev_int, shift) != 0)
		{
			vp->ev_flags |= EV_OFLOW;
		}
		break;
	case ExpOp_RightShift:
		if (num_tosint(&rhs_int, &shift) != 0
			|| num_lrshift(vp->ev_int, shift) != 0)
		{
			vp->ev_flags |= EV_OFLOW;
		}
		break;
	case ExpOp_Maximum:
		if (num_ucompare(vp->ev_int, &rhs_int) < 0)
			*vp->ev_int = *rhs.ev_int;
		break;
	case ExpOp_LCM:
		if (num_lcm(vp->ev_int, &rhs_int) != 0)
			vp->ev_flags |= EV_OFLOW;
		break;
	}
	vp->ev_flags &= ~EV_CONST;	/* only here for non-leaf operators */
}

Eval *
#ifdef __STDC__
evalexpr(const Expr *ep)	/* evaluate int/rel[/flt] expression */
#else
evalexpr(ep)Expr *ep;
#endif
{
	NumSize size;
	static Eval res;
	static Integer int_res;

	res.ev_expr = ep;
	res.ev_flags = 0;
	switch (ep->ex_type)
	{
#ifdef FLOATEXPRS
		static Float flt_res;

	case ExpTy_Floating:
		if (ep->ex_op == ExpOp_LeafFloat)	/* short cut */
		{
			res.ev_flt = (Float *)ep->right.ex_flt;
			res.ev_flags = EV_FLOAT | EV_CONST;
			return &res;
		}
		res.ev_flt = &flt_res;
		eval(ep, &res);
		return &res;
#endif
	case ExpTy_Integer:
		if (ep->ex_op == ExpOp_LeafInteger)	/* short cut */
		{
			res.ev_int = (Integer *)ep->right.ex_int;
			res.ev_flags = EV_CONST;
			break;
		}
		/*FALLTHROUGH*/
	case ExpTy_Relocatable:
		res.ev_int = &int_res;
		eval(ep, &res);
		break;
	default:
		fatal(gettxt(":787","evalexpr():inappropriate expr type: %u"),
			(Uint)ep->ex_type);
		/*NOTREACHED*/
	}
	num_size(res.ev_int, &size);
	res.ev_nubit = size.sz_nubit;
	res.ev_nsbit = size.sz_nsbit;
	res.ev_minbit = size.sz_minbit;
	if (num_toulong(res.ev_int, &res.ev_ulong) != 0)
		res.ev_flags |= EV_TRUNC;
	return &res;
}

void
#ifdef __STDC__
evalcopy(Eval *vp) /* take EV_CONST result and put into modifiable object */
#else
evalcopy(vp)Eval *vp;
#endif
{
	static Integer intcopy;
#ifdef FLOATEXPRS
	static Float fltcopy;

	if (vp->ev_flags & EV_FLOAT)
	{
		fltcopy = *vp->ev_flt;
		vp->ev_flt = &fltcopy;
	}
	else
#endif
	{
		intcopy = *vp->ev_int;
		vp->ev_int = &intcopy;
	}
	vp->ev_flags &= ~EV_CONST;
}

void
#ifdef __STDC__
flt_todata(Eval *vp, const Code *cp, Section *secp) /* fill floating object */
#else
flt_todata(vp, cp, secp)Eval *vp; Code *cp; Section *secp;
#endif
{
#ifdef FLOATEXPRS
	static const char MSGtoobig[] =
		"floating value too big for %s precision: %s";
	static const char MSGzbyte[] =
		"section contains only zero-valued bytes: %s";
	fpemu_x_t ext_prec;
	fpemu_d_t double_prec;
	fpemu_f_t float_prec;
	Uchar *src, *p;
	Uint sz;

	if (!(vp->ev_flags & EV_FLOAT))
	{
		evaltofp(&ext_prec, vp);
		vp->ev_flt = &ext_prec;
	}
	switch (cp->info.code_form)
	{
	default:
		fatal(gettxt(":790","flt_todata():inappropriate code form: %u"),
			(Uint)cp->info.code_form);
		/*NOTREACHED*/
	case CodeForm_Float:
		float_prec = fpemu_xtof(*vp->ev_flt);
		sz = sizeof(float_prec);
		src = sizeof(float_prec) + (Uchar *)&float_prec;
		break;
	case CodeForm_Double:
		double_prec = fpemu_xtod(*vp->ev_flt);
		sz = sizeof(double_prec);
		src = sizeof(double_prec) + (Uchar *)&double_prec;
		break;
	case CodeForm_Extended:
		sz = sizeof(ext_prec);
		src = sizeof(ext_prec) + (Uchar *)vp->ev_flt;
		break;
	}
	if (errno != 0)
	{
		errno = 0;	/* can be misleading */
		exprerror(vp->ev_expr, gettxt(":788",MSGtoobig),
			cp->info.code_form == CodeForm_Float ? gettxt(":1471","float")
			: cp->info.code_form == CodeForm_Double ? gettxt(":1472","double")
			: gettxt(":1473","-unknown-"), fpemu_xtoa(*vp->ev_flt));
	}
	if (sz != cp->code_size)
	{
		fatal(gettxt(":814","flt_todata():data size (%u) != target size (%u)"),
			sz, (Uint)cp->code_size);
	}
	if ((p = secp->sec_data) == 0)
	{
		/*
		* Permit zero-valued floating objects if zero-fill section.
		*/
		do
		{
			if (*--src == 0)
				continue;
			exprerror(vp->ev_expr, gettxt(":789",MSGzbyte),
				(const char *)secp->sec_sym->sym_name);
			break;
		} while (--sz != 0);
		return;
	}
	/*
	* Floating emulation code provides the correct byte order.
	*/
	p += cp->code_addr + sz;
	do
	{
		*--p = *--src;
	} while (--sz != 0);
#else
	fatal(gettxt(":792","flt_todata():initialization of floating object attempted"));
#endif	/*FLOATEXPRS*/
}

void
#ifdef __STDC__
subeval(Eval *vp, Ulong val)	/* modify: eval -= Ulong (signed) */
#else
subeval(vp,val)Eval *vp; Ulong val;
#endif
{
	NumSize size;
	Integer num;

	vp->ev_expr = 0;	/* no longer the result of any expression */
	vp->ev_flags &= ~(EV_OFLOW | EV_TRUNC);
	(void)num_fromulong(&num, val);
	if (num_ssubtract(vp->ev_int, &num) != 0)
		vp->ev_flags |= EV_OFLOW;
	num_size(vp->ev_int, &size);
	vp->ev_nubit = size.sz_nubit;
	vp->ev_nsbit = size.sz_nsbit;
	vp->ev_minbit = size.sz_minbit;
	if (num_toulong(vp->ev_int, &vp->ev_ulong) != 0)
		vp->ev_flags |= EV_TRUNC;
}

static struct reeval	/* one to-be-reevaluated expression's early result */
{
	Integer		num;	/* full integer result */
	const Expr	*expr;	/* originating expression */
	Uint		flags;	/* EV_RELOC kept; EV_LDIFF added */
	struct reeval	*next;
} *reeval_list;

void
#ifdef __STDC__
delayeval(Eval *vp)	/* reevaluate expression at processing end */
#else
delayeval(vp)Eval *vp;
#endif
{
	static struct reeval *avail, *endavail;
	register struct reeval *rp;

	if (vp->ev_expr == 0)
		fatal(gettxt(":793","delayexpr():Eval not derived from expr"));
	else if (vp->ev_flags & EV_FLOAT)
		fatal(gettxt(":794","delayexpr():floating expression not reevaluatable"));
#ifndef ALLOC_EVAL_CHUNK
#   define ALLOC_EVAL_CHUNK 100
#endif
	if ((rp = avail) == endavail)
	{
#ifdef DEBUG
		if (DEBUG('a') > 0)
		{
			static Ulong total;

			(void)fprintf(stderr,
				"Total struct reevals=%lu @%lu ea.\n",
				total += ALLOC_EVAL_CHUNK,
				(Ulong)sizeof(struct reeval));
		}
#endif
		avail = rp = (struct reeval *)alloc((void *)0,
			ALLOC_EVAL_CHUNK * sizeof(struct reeval));
		endavail = rp + ALLOC_EVAL_CHUNK;
	}
	avail = rp + 1;
	rp->next = reeval_list;
	reeval_list = rp;
	/*
	* Save our own copy of the result.  If both EV_RELOC and ev_dot
	* are set, keep a copy of the difference from ev_dot, not the
	* beginning of the associated section.
	*/
	rp->num = *vp->ev_int;
	if (!(vp->ev_flags & EV_RELOC))
		rp->flags = 0;
	else if (vp->ev_dot == 0)
		rp->flags = EV_RELOC;
	else
	{
		Integer val;

		num_fromulong(&val, vp->ev_dot->code_addr);
		num_ssubtract(&rp->num, &val);
		rp->flags = EV_RELOC | EV_LDIFF;
	}
	rp->expr = vp->ev_expr;
}

void
#ifdef __STDC__
reevaluate(void)	/* do all reevals */
#else
reevaluate()
#endif
{
	static const char MSGdiff[] =
		"expression reevaluation differs: was";
	register struct reeval *rp;
	register Eval *vp;
	const char *msg;

	for (rp = reeval_list; rp != 0; rp = rp->next)
	{
		vp = evalexpr(rp->expr);
		/*
		* Check the shape of the two evaluations first.
		* Subtract the value of dot if appropriate.
		*/
		if (rp->flags & EV_RELOC)
		{
			msg = 0;
			if (!(vp->ev_flags & EV_RELOC))
				msg = gettxt(":1474","%s relocatable");
			else if (rp->flags & EV_LDIFF)
			{
				if (vp->ev_dot == 0)
					msg = gettxt(":1475","%s based on defined name");
				else
					subeval(vp, vp->ev_dot->code_addr);
			}
			else if (vp->ev_dot != 0)
				msg = gettxt(":1476","%s based on undefined name");
			if (msg != 0)
			{
				exprerror(rp->expr, msg, gettxt(":795",MSGdiff));
				continue;
			}
		}
		/*
		* Then, check the old and new values.
		*/
		if (num_ucompare(&rp->num, vp->ev_int) != 0)
		{
			char was[2 + NUMSIZE / 4 + 1 + 1];

			(void)strcpy(was, num_tohex(&rp->num));
			exprerror(rp->expr, gettxt(":1470","%s %s, now %s"), gettxt(":795",MSGdiff),
				was, num_tohex(vp->ev_int));
		}
	}
}
