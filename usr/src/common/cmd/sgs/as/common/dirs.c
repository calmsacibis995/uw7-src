#ident	"@(#)nas:common/dirs.c	1.7"
/*
* common/dirs.c - common assembler directive handling
*
*	Some of the directive expressions are required to be
*	evaluatable when the directive is specified.  These
*	are only those expressions whose value has an immediate
*	affect on the value of . for some section.  These are:
*
*		The second and third operand for .bss.
*		The second operand for a ".set ." directive.
*		The operands for .align.
*		The operand for .zero.
*		The (string) operands for .ascii and .string.
*		The (string) operand for .ident.
*
*	To keep .file and .version simple, these also require
*	that their (string) operand be evaluatable when specified.
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifndef __STDC__
#  include <memory.h>
#endif
#include "common/as.h"
#include "common/dirs.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/objf.h"
#include "common/sect.h"
#include "common/syms.h"
#include "align.h"	/* default alignment for bss and common */

#include "intemu.h"

static const char *const dirstr[Dir_TOTAL] =
{
	".section",	".pushsection",	".popsection",	".previous",
	".text",	".data",
	".bss",
	".comm",	".lcomm",
	".globl",	".local",	".weak",
	".set",
	".size",	".type",
	".align",	".backalign",
	".zero",
	".byte",	".2byte",	".4byte",	".8byte",
	".float",	".double",	".ext",
	".ascii",	".string",
	".file",	".ident",	".version",
};

static const char *curstr;	/* current directive name */

static const char MSGtoo[] = "too many operands for %s";
static const char MSGxpg[] = "expecting %s operand for %s, operand %d";
static const char MSGnon[] = "non-%s operand for %s, operand %d";
static const char MSGlst[] = "invalid %s directive--expecting list of %ss";
static const char MSGrng[] = "%s for %s out of range: %s";
static const char MSGpad[] = "%s has been aligned for %s";

static const char MSGnam[] = "name";
static const char MSGstr[] = "string";
static const char MSGaln[] = "alignment";
static const char MSGint[] = "integer expression";
static const char MSGval[] = "evaluatable integer expression";
static const char MSGf_i[] = "floating/integer expression";
static const char MSGs_i[] = "string/integer expression";
static const char MSGr_i[] = "relocatable/integer expression";

enum { DSyms_Text, DSyms_Data, DSyms_Bss, DSyms_TOTAL };
static Symbol *dirsyms[DSyms_TOTAL];

enum { DSect_Comment, DSect_TOTAL };
static Section *dirsect[DSect_TOTAL];

static Expr *def_align_expr;	/* default align expr for common; shared */

static Symbol *
#ifdef __STDC__
dsym(const char *str)	/* look up symbol str */
#else
dsym(str)char *str;
#endif
{
	return lookup((const Uchar *)str, (size_t)strlen(str));
}

void
#ifdef __STDC__
initdirs(void)	/* initialize symbol and sections used here */
#else
initdirs()
#endif
{
	/*
	* Assumes that sections have been initialized.
	*/
	dirsyms[DSyms_Text] = dsym(dirstr[Dir_Text]);
	dirsyms[DSyms_Data] = dsym(dirstr[Dir_Data]);
	dirsyms[DSyms_Bss] = dsym(dirstr[Dir_Bss]);
	dirsect[DSect_Comment] = dsym(".comment")->sym_sect;
	def_align_expr = ulongexpr((Ulong)BSS_COMM_ALIGN);
}

static Symbol *
#ifdef __STDC__
ident_check(int num, register Operand *op)	/* verify Operand is name */
#else
ident_check(num, op)int num; register Operand *op;
#endif
{
	register Expr *ep;

	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0
		|| ep->ex_op != ExpOp_LeafName)
	{
		error(gettxt(":749",MSGxpg), gettxt(":754",MSGnam), curstr, num);
		olstfree(op);
		return 0;
	}
	operfree(op);
	return ep->right.ex_sym;
}

static Expr *
#ifdef __STDC__
string_check(int num, register Operand *op)	/* verify Operand is string */
#else
string_check(num, op)int num; register Operand *op;
#endif
{
	register Expr *ep;

	/*
	* Don't allow ExpTy_Unknown here since all uses require an
	* immediately evaluatable string.  (Most affect the value
	* of . for the corresponding section.)  The setlessexpr()
	* call removes any .set's so that a ExpOp_LeafString
	* expression node is returned.
	*/
	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0
		|| ep->ex_type != ExpTy_String)
	{
		error(gettxt(":749",MSGxpg), gettxt(":755",MSGstr), curstr, num);
		olstfree(op);
		return 0;
	}
	ep = setlessexpr(ep);
	operfree(op);
	return ep;
}

static Expr *
#ifdef __STDC__
expr_check(int num, register Operand *op, Uint tybits)	/* verify expr type */
#else
expr_check(num, op, tybits)int num; register Operand *op; Uint tybits;
#endif
{
	const char *fmt, *msg;
	register Expr *ep;

	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0)
	{
		fmt = gettxt(":749",MSGxpg);
	err:;
		switch (tybits)
		{
		default:
			fatal(gettxt(":762","expr_check():unknown expr type bit-list: %u"),
				tybits);
			/*NOTREACHED*/
		case BIT(ExpTy_Integer):
			msg = gettxt(":757",MSGint);
			break;
		case BIT(ExpTy_Integer) | BIT(ExpTy_String):
			msg = gettxt(":760",MSGs_i);
			break;
		case BIT(ExpTy_Integer) | BIT(ExpTy_Floating):
			msg = gettxt(":759",MSGf_i);
			break;
		case BIT(ExpTy_Integer) | BIT(ExpTy_Relocatable):
			msg = gettxt(":761",MSGr_i);
			break;
		}
		error(fmt, msg, curstr, num);
		olstfree(op);
		return 0;
	}
	if (ep->ex_type == ExpTy_Unknown)
		op->oper_tybits = tybits;
	else if (BIT(ep->ex_type) & tybits)
		cutoper(op);
	else
	{
		fmt = gettxt(":750",MSGnon);
		goto err;
	}
	return ep;
}

static Eval *
#ifdef __STDC__
eval_check(int num, register Operand *op)	/* verify as evaluatable */
#else
eval_check(num, op)int num; register Operand *op;
#endif
{
	const char *fmt;
	register Expr *ep;
	register Eval *vp;

	if (op == 0 || op->oper_flags != 0 || (ep = op->oper_expr) == 0)
	{
		fmt = gettxt(":749",MSGxpg);
	err:;
		error(fmt, gettxt(":758",MSGval), curstr, num);
		olstfree(op);
		return 0;
	}
	if (ep->ex_type != ExpTy_Integer)
	{
		fmt = gettxt(":750",MSGnon);
		goto err;
	}
	vp = evalexpr(ep);
	if (vp->ev_flags & EV_LDIFF)
	{
		delayeval(vp);
		cutoper(op);
	}
	else
		operfree(op);
	return vp;
}

static Ulong
#ifdef __STDC__
align_check(int num, Operand *op)	/* verify Operand is valid align */
#else
align_check(num, op)int num; Operand *op;
#endif
{
	register Eval *vp;

	if ((vp = eval_check(num, op)) == 0)
		return 0;
	if (vp->ev_flags & (EV_OFLOW | EV_TRUNC))
	{
		error(gettxt(":752",MSGrng), gettxt(":756",MSGaln), curstr, num_tohex(vp->ev_int));
		return 0;
	}
	if (!validalign(vp->ev_ulong))
	{
		error(gettxt(":763","invalid alignment (%lu) for %s, operand %d"),
			vp->ev_ulong, curstr, num);
		return 0;
	}
	return vp->ev_ulong;
}

static Ulong *
#ifdef __STDC__
align_list(int num, Operand *op, Uint *lp)	/* verify "align,max" list */
#else
align_list(num,op,lp)int num; Operand *op; Uint *lp;
#endif
{
	static const char MSGmfill[] = "maximum fill size";
	static const char MSGpairs[] =
		"Align/max-fill pairs for %s misordered: %lu,%lu follows %lu,%lu";
	static Ulong *listbuf;	/* available space */
	static Uint listlen;	/* length of available space; always even */
	register Ulong *ulp;
	Ulong align, max;
	Eval *vp;
	Operand *op2;
	Uint cnt = 0;

	/*
	* Count number of operands, add one for final, implied maximum fill.
	*/
	for (op2 = op; op2 != 0; op2 = op2->oper_next)
		cnt++;
	if ((cnt & 0x1) != 0)
		cnt++;
	else if (cnt == 0)
	{
		error(gettxt(":749",MSGxpg), gettxt(":756",MSGaln), curstr, num);
		return 0;
	}
#ifndef ALLOC_ALGN_CHUNK
#   define ALLOC_ALGN_CHUNK 50
#endif
	if (cnt > listlen)	/* need more space */
	{
#ifdef DEBUG
		static Ulong total, wasted;

		wasted += listlen;
		if ((listlen = ALLOC_ALGN_CHUNK) < cnt)
			listlen = cnt * 2;
		total += listlen;
		if (DEBUG('a') > 0)
		{
			(void)fprintf(stderr,
				"Total align list space=%lu, wasted=%lu\n",
				total * sizeof(Ulong), wasted * sizeof(Ulong));
		}
#else
		if ((listlen = ALLOC_ALGN_CHUNK) < cnt)
			listlen = cnt * 2;
#endif	/*DEBUG*/
		listbuf = (Ulong *)alloc((void *)0, listlen * sizeof(Ulong));
	}
	/*
	* Fill in list of alignment and maximum fill pairs.
	*/
	ulp = listbuf;
	num--;
	do
	{
		if ((align = align_check(++num, op)) == 0)
			return 0;
		if ((op2 = op->oper_next) == 0)
		{
			if ((max = align - 1) < 8 && cnt == 2)	/* simplest */
			{
				static const Ulong ans[8][2] =	/* shared */
				{
					{1, 0},	{2, 1},	{3, 2},	{4, 3},
					{5, 4},	{6, 5},	{7, 6},	{8, 7},
				};

				*lp = 1;
				return (Ulong *)ans[max];
			}
		}
		else if ((vp = eval_check(++num, op2)) == 0)
			return 0;
		else if (vp->ev_flags & (EV_OFLOW | EV_TRUNC)
			|| (max = vp->ev_ulong) == 0 || max >= align)
		{
			error(gettxt(":752",MSGrng), gettxt(":764",MSGmfill), curstr, num_tohex(vp->ev_int));
			olstfree(op);
			return 0;
		}
		if (ulp > listbuf && (align >= ulp[-2] || max > ulp[-1]))
		{
			error(gettxt(":765",MSGpairs), curstr, align, max, ulp[-2], ulp[-1]);
			olstfree(op);
			return 0;
		}
		ulp[0] = align;
		ulp[1] = max;
		ulp += 2;
	} while (op2 != 0 && (op = op2->oper_next) != 0);
	*lp = cnt >> 1;
	ulp = listbuf;
	listbuf += cnt;
	listlen -= cnt;
	return ulp;
}

static void
#ifdef __STDC__
dot_section(register Operand *op)	/* .section id [int/str [int/str]] */
#else
dot_section(op)register Operand *op;
#endif
{
	Symbol *sp;
	Expr *e2 = 0, *e3 = 0;

	if ((sp = ident_check(1, op)) == 0)
		return;
	if ((op = op->oper_next) != 0)	/* a second operand */
	{
		if ((e2 = expr_check(2, op,
			BIT(ExpTy_String) | BIT(ExpTy_Integer))) == 0)
		{
			return;
		}
		if ((op = op->oper_next) != 0)	/* a third operand */
		{
			if ((e3 = expr_check(3, op,
				BIT(ExpTy_String) | BIT(ExpTy_Integer))) == 0)
			{
				return;
			}
			if (op->oper_next != 0)
			{
				error(gettxt(":748",MSGtoo), curstr);
				olstfree(op);
				return;
			}
		}
	}
	setsect(sp, e2, e3);
}

static void
#ifdef __STDC__
dot_pushsect(Operand *op)	/* .pushsection ident */
#else
dot_pushsect(op)Operand *op;
#endif
{
	Symbol *sp;
	Section *secp;

	if ((sp = ident_check(1, op)) == 0)
		return;
	if ((secp = sp->sym_sect) == 0)
	{
		error(gettxt(":750",MSGnon), gettxt(":766","section name"), curstr, 1);
		olstfree(op);
		return;
	}
	if (op->oper_next != 0)
	{
		error(gettxt(":748",MSGtoo), curstr);
		olstfree(op);
		return;
	}
	(void)stacksect(secp);
}

static void
#ifdef __STDC__
dot_popsect(Operand *op)	/* .popsection */
#else
dot_popsect(op)Operand *op;
#endif
{
	if (op != 0)
	{
		error(gettxt(":748",MSGtoo), curstr);
		olstfree(op);
		return;
	}
	if (!stacksect((Section *)0))
		error(gettxt(":767","section stack empty: too many %s directives"), curstr);
}

static void
#ifdef __STDC__
dot_previous(Operand *op)	/* .previous */
#else
dot_previous(op)Operand *op;
#endif
{
	if (op != 0)
	{
		error(gettxt(":748",MSGtoo), curstr);
		olstfree(op);
		return;
	}
	prevsect();
}

static void
#ifdef __STDC__
dot_textdata(Operand *op, int sym)	/* .text/.data */
#else
dot_textdata(op, sym)Operand *op; int sym;
#endif
{
	if (op != 0)
	{
		error(gettxt(":748",MSGtoo), curstr);
		olstfree(op);
		return;
	}
	setsect(dirsyms[sym], (Expr *)0, (Expr *)0);
}

static void
#ifdef __STDC__
dot_bss(register Operand *op)	/* .bss [ident evalexpr [evalexpr]] */
#else
dot_bss(op)register Operand *op;
#endif
{
	register Eval *vp;
	Symbol *sp;
	Ulong size, align;

	if (op == 0)	/* change to .bss section */
	{
		setsect(dirsyms[DSyms_Bss], (Expr *)0, (Expr *)0);
		return;
	}
	if ((sp = ident_check(1, op)) == 0)
		return;
	op = op->oper_next;
	if ((vp = eval_check(2, op)) == 0)
		return;
	if (vp->ev_flags & (EV_OFLOW | EV_TRUNC))
	{
		error(gettxt(":752",MSGrng), gettxt(":768","size"), curstr, num_tohex(vp->ev_int));
		olstfree(op);
		return;
	}
	size = vp->ev_ulong;
	if ((op = op->oper_next) == 0)
		align = BSS_COMM_ALIGN;
	else
	{
		if ((align = align_check(3, op)) == 0)
			return;
		if (op->oper_next != 0)
		{
			error(gettxt(":748",MSGtoo), curstr);
			olstfree(op);
			return;
		}
	}
	bsssym(sp, Bind_Local, size, align);
}

static void
#ifdef __STDC__
dot_comm(register Operand *op, int bind) /* .[l]comm id intexpr [intexpr] */
#else
dot_comm(op, bind)register Operand *op; int bind;
#endif
{
	Expr *ep;
	Expr *align;
	Symbol *sp;

	if ((sp = ident_check(1, op)) == 0)
		return;
	op = op->oper_next;
	if ((ep = expr_check(2, op, BIT(ExpTy_Integer))) == 0)
		return;
	if ((op = op->oper_next) == 0)
		align = def_align_expr;	/* shared expr here is okay */
	else
	{
		if ((align = expr_check(3, op, BIT(ExpTy_Integer))) == 0)
			return;
		if (op->oper_next != 0)
		{
			error(gettxt(":748",MSGtoo), curstr);
			olstfree(op);
			return;
		}
	}
	commsym(sp, bind, ep, align);
}

static void
#ifdef __STDC__
dot_bind(register Operand *op, int bind) /* .globl/.local/.weak id-list */
#else
dot_bind(op, bind)register Operand *op; int bind;
#endif
{
	register Symbol *sp;
	register int num = 0;

	if (op == 0)
	{
		error(gettxt(":751",MSGlst), curstr, gettxt(":754",MSGnam));
		olstfree(op);
		return;
	}
	do
	{
		if ((sp = ident_check(++num, op)) == 0)
			return;
		bindsym(sp, bind);
	} while ((op = op->oper_next) != 0);
}

static void
#ifdef __STDC__
dot_set(register Operand *op)	/* .set ident operand */
#else
dot_set(op)register Operand *op;
#endif
{
	Symbol *sp;

	if ((sp = ident_check(1, op)) == 0)
		return;
	if ((op = op->oper_next) == 0)
	{
		error(gettxt(":769","%s missing operand 2"), curstr);
		olstfree(op);
		return;
	}
	if (op->oper_next != 0)
	{
		error(gettxt(":748",MSGtoo), curstr);
		olstfree(op);
		return;
	}
	opersym(sp, op);
}

static void
#ifdef __STDC__
dot_size(register Operand *op)	/* .size ident intexpr */
#else
dot_size(op)register Operand *op;
#endif
{
	Expr *ep;
	Symbol *sp;

	if (flags & ASFLAG_TRANSITION
		&& obsdirect((const Uchar *)curstr, (size_t)0, op) != 0)
	{
		return;		/* obsolete usage */
	}
	if ((sp = ident_check(1, op)) == 0)
		return;
	op = op->oper_next;
	if ((ep = expr_check(2, op, BIT(ExpTy_Integer))) == 0)
		return;
	if (op->oper_next != 0)
	{
		error(gettxt(":748",MSGtoo), curstr);
		olstfree(op);
		return;
	}
	sizesym(sp, ep);
}

static void
#ifdef __STDC__
dot_type(register Operand *op)	/* .type ident int/str */
#else
dot_type(op)register Operand *op;
#endif
{
	Expr *ep;
	Symbol *sp;

	if (flags & ASFLAG_TRANSITION
		&& obsdirect((const Uchar *)curstr, (size_t)0, op) != 0)
	{
		return;		/* obsolete usage */
	}
	if ((sp = ident_check(1, op)) == 0)
		return;
	op = op->oper_next;
	if ((ep = expr_check(2, op,
		BIT(ExpTy_String) | BIT(ExpTy_Integer))) == 0)
	{
		return;
	}
	if (op->oper_next != 0)
	{
		error(gettxt(":748",MSGtoo), curstr);
		olstfree(op);
		return;
	}
	typesym(sp, ep);
}

static void
#ifdef __STDC__
dot_align(Operand *op)	/* .align [eval eval ...] eval [eval] */
#else
dot_align(op)Operand *op;
#endif
{
	Ulong *list;
	Uint npair;

	if ((list = align_list(1, op, &npair)) == 0)
		return;
	sectalign((Section *)0, list, npair);
}

static void
#ifdef __STDC__
dot_backalign(Operand *op)	/* .backalign id [eval eval ...] eval [eval] */
#else
dot_backalign(op)Operand *op;
#endif
{
	Section *secp;
	Symbol *label;
	Ulong *list;
	Uint npair;

	if ((label = ident_check(1, op)) == 0)
		return;
	secp = cursect();
	if (label->sym_defn != secp || label->sym_kind != SymKind_Regular)
	{
		error(gettxt(":750",MSGnon), gettxt(":770","same section label"), curstr, 1);
		return;
	}
	if ((list = align_list(2, op->oper_next, &npair)) == 0)
		return;
	sectbackalign(secp, label, list, npair);
}

static void
#ifdef __STDC__
dot_zero(Operand *op)	/* .zero evalexpr */
#else
dot_zero(op)Operand *op;
#endif
{
	register Eval *vp;

	if ((vp = eval_check(1, op)) == 0)
		return;
	if (vp->ev_flags & (EV_OFLOW | EV_TRUNC))
	{
		error(gettxt(":752",MSGrng), gettxt(":771","number of bytes"), curstr,
			num_tohex(vp->ev_int));
		olstfree(op);
		return;
	}
	if (op->oper_next != 0)
	{
		error(gettxt(":748",MSGtoo), curstr);
		olstfree(op);
		return;
	}
	sectzero((Section *)0, vp->ev_ulong);	/* don't ignore ".zero 0" */
}

static void
#ifdef __STDC__
dot_bytes(register Operand *op, Ulong size, int form) /* .byte/... expr-list */
#else
dot_bytes(op, size, form)register Operand *op; Ulong size; int form;
#endif
{
	register Expr *ep;
	register int num = 0;
	register Section *secp;

	if (op == 0)
	{
		error(gettxt(":751",MSGlst), curstr, gettxt(":761",MSGr_i));
		olstfree(op);
		return;
	}
	secp = cursect();
	do
	{
		num++;
		/*
		* The equivalent of expr_check() for Integer/Relocatable is
		* inline-expanded as this is the highest-running directive.
		*/
		if (op->oper_flags != 0 || (ep = op->oper_expr) == 0)
		{
			error(gettxt(":749",MSGxpg), gettxt(":761",MSGr_i), curstr, num);
			olstfree(op);
			return;
		}
		switch (ep->ex_type)
		{
		case ExpTy_Unknown:
			op->oper_tybits
				= BIT(ExpTy_Relocatable) | BIT(ExpTy_Integer);
			break;
		case ExpTy_Integer:
		case ExpTy_Relocatable:
			cutoper(op);
			break;
		default:
			error(gettxt(":750",MSGnon), gettxt(":761",MSGr_i), curstr, num);
			olstfree(op);
			return;
		}
		fixexpr(ep);
		sectexpr(secp, ep, size, form);
	} while ((op = op->oper_next) != 0);
}

static void
#ifdef __STDC__
dot_fltbytes(register Operand *op, int dir) /* .float/.double/.ext flt-list */
#else
dot_fltbytes(op, dir)register Operand *op; int dir;
#endif
{
	static const Ulong flt_align[2] = {FLOAT_ALIGN, FLOAT_ALIGN - 1};
	static const Ulong dbl_align[2] = {DOUBLE_ALIGN, DOUBLE_ALIGN - 1};
	static const Ulong ext_align[2] = {EXT_ALIGN, EXT_ALIGN - 1};
	register Expr *ep;
	register int num = 0;
	register Section *secp;
	Ulong sz;

	if (op == 0)
	{
		error(gettxt(":751",MSGlst), curstr, gettxt(":759",MSGf_i));
		olstfree(op);
		return;
	}
	secp = cursect();
	/*
	* Hopefully the "unsupported directive" error and the align
	* checks will be eliminated if they cannot be reached.
	*/
	switch (dir)
	{
	default:
		fatal(gettxt(":772","dot_fltbytes():unknown directive: (%d) %s"),
			dir, curstr);
		/*NOTREACHED*/
	case Dir_Float:
		/*CONSTANTCONDITION*/
		if (FLOAT_SIZE == 0)
		{
		unsup:;
			error(gettxt(":773","unsupported directive: %s"), curstr);
			olstfree(op);
			return;
		}
		sz = FLOAT_SIZE;
		dir = CodeForm_Float;
		/*CONSTANTCONDITION*/
		if (FLOAT_ALIGN > 1)
		{
			if (sectoptalign(secp, flt_align) == 0)
				warn(gettxt(":753",MSGpad), secp->sec_sym->sym_name, curstr);
		}
		break;
	case Dir_Double:
		/*CONSTANTCONDITION*/
		if (DOUBLE_SIZE == 0)
			goto unsup;
		sz = DOUBLE_SIZE;
		dir = CodeForm_Double;
		/*CONSTANTCONDITION*/
		if (DOUBLE_ALIGN > 1)
		{
			if (sectoptalign(secp, dbl_align) == 0)
				warn(gettxt(":753",MSGpad), secp->sec_sym->sym_name, curstr);
		}
		break;
	case Dir_Ext:
		/*CONSTANTCONDITION*/
		if (EXT_SIZE == 0)
			goto unsup;
		sz = EXT_SIZE;
		dir = CodeForm_Extended;
		/*CONSTANTCONDITION*/
		if (EXT_ALIGN > 1)
		{
			if (sectoptalign(secp, ext_align) == 0)
				warn(gettxt(":753",MSGpad), secp->sec_sym->sym_name, curstr);
		}
		break;
	}
	do
	{
		if ((ep = expr_check(++num, op,
			BIT(ExpTy_Floating) | BIT(ExpTy_Integer))) == 0)
		{
			return;
		}
		fixexpr(ep);
		sectexpr(secp, ep, sz, dir);
	} while ((op = op->oper_next) != 0);
}

static void
#ifdef __STDC__
dot_strlist(register Operand *op, int dir) /* .ascii/.string str-list */
#else
dot_strlist(op, dir)register Operand *op; int dir;
#endif
{
	register Expr *ep;
	register int num = 0;
	register Section *secp;

	if (op == 0)
	{
		error(gettxt(":751",MSGlst), curstr, gettxt(":755",MSGstr));
		olstfree(op);
		return;
	}
	secp = cursect();
	do
	{
		if ((ep = string_check(++num, op)) == 0)
			return;
		fixexpr(ep);
		if (dir == Dir_String)
			ep->left.ex_len++; /* savestr() adds \0 */
		sectstr(secp, ep);
	} while ((op = op->oper_next) != 0);
}

static void
#ifdef __STDC__
dot_strmisc(Operand *op, int dir)	/* .file/.ident/.version strexpr */
#else
dot_strmisc(op, dir)Operand *op; int dir;
#endif
{
	register Expr *ep;

	if ((ep = string_check(1, op)) == 0)
		return;
	if (op->oper_next != 0)
	{
		error(gettxt(":748",MSGtoo), curstr);
		olstfree(op);
		return;
	}
	switch (dir)
	{
	default:
		fatal(gettxt(":774","do_strmisc():unknown directive: (%d) %s"),
			dir, curstr);
		/*NOTREACHED*/
	case Dir_File:
		if (objfsrcstr(ep) == 0)
			error(gettxt(":775","too many %s directives"), curstr);
		break;
	case Dir_Ident:
		fixexpr(ep);
		if (ep->right.ex_str[ep->left.ex_len - 1] != '\0')
			ep->left.ex_len++;	/* savestr() adds \0 */
		sectstr(dirsect[DSect_Comment], ep);
		break;
	case Dir_Version:
		versioncheck(ep->right.ex_str, ep->left.ex_len);
		break;
	}
}

void
#ifdef __STDC__
unaligndata(const char *dir, Operand *op, Ulong sz, int form)
#else
unaligndata(dir, op, sz, form)char *dir; Operand *op; Ulong sz; int form;
#endif
{
	curstr = dir;
	dot_bytes(op, sz, form);
}

void
#ifdef __STDC__
aligndata(const char *dir, Operand *op, Ulong sz, const Ulong *al, int form)
#else
aligndata(dir, op, sz, al, form)char *dir; Operand *op; Ulong sz, *al; int form;
#endif
{
	if (sectoptalign((Section *)0, al) == 0)
		warn(gettxt(":753",MSGpad), cursect()->sec_sym->sym_name, dir);
	curstr = dir;
	dot_bytes(op, sz, form);
}

void
#ifdef __STDC__
directive(register const Uchar *s, size_t n, Oplist *p)
#else
directive(s, n, p)register Uchar *s; size_t n; Oplist *p;
#endif
{
	Operand *ops;

#ifdef DEBUG
	if (DEBUG('d') > 0)
	{
		(void)fprintf(stderr, "directive(%s,p=", prtstr(s, n));
		printoplist(p);
		(void)fputs(")\n", stderr);
	}
#endif
	if (p == 0)
		ops = 0;
	else
		ops = p->olst_first;
	/*
	* Determine which directive.  Fast (but ugly) selection code.
	* Purposefully extra-expanded to ease future new directives.
	* It would be nice if there were a tool that could generate
	* code such as the following given a simple (list of strings
	* and associated code) description.
	*/
	switch (n)
	{
	case 4: /* bss, ext, set */
		if (s[3] == 't')
		{
			if (s[2] == 'e')
			{
				if (s[1] == 's')
				{
					curstr = dirstr[Dir_Set];
					dot_set(ops);
					goto do_cut;
				}
			}
			else if (s[2] == 'x' && s[1] == 'e')
			{
				curstr = dirstr[Dir_Ext];
				dot_fltbytes(ops, Dir_Ext);
				goto do_cut;
			}
		}
		else if (s[1] == 'b' && s[2] == 's' && s[3] == 's')
		{
			curstr = dirstr[Dir_Bss];
			dot_bss(ops);
			goto do_cut;
		}
		break;
	case 5: /* byte, comm, data, file, size, text, type, weak, zero */
		if (s[4] == 'e')
		{
			if (s[2] == 'y')
			{
				if (s[3] == 't')
				{
					if (s[1] == 'b')
					{
						curstr = dirstr[Dir_Byte];
						dot_bytes(ops, (Ulong)1,
							CodeForm_Integer);
						goto do_cut;
					}
				}
				else if (s[3] == 'p')
				{
					if (s[1] == 't')
					{
						curstr = dirstr[Dir_Type];
						dot_type(ops);
						goto do_cut;
					}
				}
			}
			else if (s[2] == 'i')
			{
				if (s[3] == 'z')
				{
					if (s[1] == 's')
					{
						curstr = dirstr[Dir_Size];
						dot_size(ops);
						goto do_cut;
					}
				}
				else if (s[3] == 'l')
				{
					if (s[1] == 'f')
					{
						curstr = dirstr[Dir_File];
						dot_strmisc(ops, Dir_File);
						goto do_cut;
					}
				}
			}
		}
		else if (s[2] == 'e')
		{
			if (s[1] == 't')
			{
				if (s[3] == 'x' && s[4] == 't')
				{
					curstr = dirstr[Dir_Text];
					dot_textdata(ops, DSyms_Text);
					goto do_cut;
				}
			}
			else if (s[1] == 'z')
			{
				if (s[3] == 'r' && s[4] == 'o')
				{
					curstr = dirstr[Dir_Zero];
					dot_zero(ops);
					goto do_cut;
				}
			}
			else if (s[1] == 'w')
			{
				if (s[3] == 'a' && s[4] == 'k')
				{
					curstr = dirstr[Dir_Weak];
					dot_bind(ops, Bind_Weak);
					goto do_cut;
				}
			}
		}
		else if (s[1] == 'd')
		{
			if (s[2] == 'a' && s[3] == 't' && s[4] == 'a')
			{
				curstr = dirstr[Dir_Data];
				dot_textdata(ops, DSyms_Data);
				goto do_cut;
			}
		}
		else if (s[1] == 'c')
		{
			if (s[2] == 'o' && s[3] == 'm' && s[4] == 'm')
			{
				curstr = dirstr[Dir_Comm];
				dot_comm(ops, Bind_Global);
				goto do_cut;
			}
		}
		break;
	case 6: /* align, ascii, float, globl, ident, lcomm, local, Nbyte */
		if (s[2] == 'b')
		{
			if (s[3] == 'y' && s[4] == 't' && s[5] == 'e')
			{
				if (s[1] == '4')
				{
					curstr = dirstr[Dir_4Byte];
					dot_bytes(ops, (Ulong)4,
						CodeForm_Integer);
					goto do_cut;
				}
				else if (s[1] == '2')
				{
					curstr = dirstr[Dir_2Byte];
					dot_bytes(ops, (Ulong)2,
						CodeForm_Integer);
					goto do_cut;
				}
				else if (s[1] == '8')
				{
					curstr = dirstr[Dir_8Byte];
					dot_bytes(ops, (Ulong)8,
						CodeForm_Integer);
					goto do_cut;
				}
			}
		}
		else if (s[2] == 'l')
		{
			if (s[3] == 'o')
			{
				if (s[1] == 'g')
				{
					if (s[4] == 'b' && s[5] == 'l')
					{
						curstr = dirstr[Dir_Global];
						dot_bind(ops, Bind_Global);
						goto do_cut;
					}
				}
				else if (s[1] == 'f')
				{
					if (s[4] == 'a' && s[5] == 't')
					{
						curstr = dirstr[Dir_Float];
						dot_fltbytes(ops, Dir_Float);
						goto do_cut;
					}
				}
			}
			else if (s[3] == 'i')
			{
				if (s[1] == 'a')
				{
					if (s[4] == 'g' && s[5] == 'n')
					{
						curstr = dirstr[Dir_Align];
						dot_align(ops);
						goto do_cut;
					}
				}
			}
		}
		else if (s[1] == 'l')
		{
			if (s[2] == 'o')
			{
				if (s[3] == 'c')
				{
					if (s[4] == 'a' && s[5] == 'l')
					{
						curstr = dirstr[Dir_Local];
						dot_bind(ops, Bind_Local);
						goto do_cut;
					}
				}
			}
			else if (s[2] == 'c')
			{
				if (s[3] == 'o')
				{
					if (s[4] == 'm' && s[5] == 'm')
					{
						curstr = dirstr[Dir_Lcomm];
						dot_comm(ops, Bind_Local);
						goto do_cut;
					}
				}
			}
		}
		else if (s[1] == 'i')
		{
			if (s[2] == 'd')
			{
				if (s[3] == 'e')
				{
					if (s[4] == 'n' && s[5] == 't')
					{
						curstr = dirstr[Dir_Ident];
						dot_strmisc(ops, Dir_Ident);
						goto do_cut;
					}
				}
			}
		}
		else if (s[1] == 'a')
		{
			if (s[2] == 's')
			{
				if (s[3] == 'c')
				{
					if (s[4] == 'i' && s[5] == 'i')
					{
						curstr = dirstr[Dir_Ascii];
						dot_strlist(ops, Dir_Ascii);
						goto do_cut;
					}
				}
			}
		}
		break;
	case 7: /* double, string */
		if (s[1] == 's')
		{
			if (strncmp((const char *)&s[2], "tring", 5) == 0)
			{
				curstr = dirstr[Dir_String];
				dot_strlist(ops, Dir_String);
				goto do_cut;
			}
		}
		else if (s[1] == 'd')
		{
			if (strncmp((const char *)&s[2], "ouble", 5) == 0)
			{
				curstr = dirstr[Dir_Double];
				dot_fltbytes(ops, Dir_Double);
				goto do_cut;
			}
		}
		break;
	case 8: /* section, version */
		if (s[2] == 'e' && s[5] == 'i' && s[6] == 'o' && s[7] == 'n')
		{
			if (s[1] == 's')
			{
				if (s[3] == 'c' && s[4] == 't')
				{
					curstr = dirstr[Dir_Section];
					dot_section(ops);
					goto do_cut;
				}
			}
			else if (s[1] == 'v')
			{
				if (s[3] == 'r' && s[4] == 's')
				{
					curstr = dirstr[Dir_Version];
					dot_strmisc(ops, Dir_Version);
					goto do_cut;
				}
			}
		}
		break;
	case 9: /* previous */
		if (strncmp((const char *)&s[1], "previous", 8) == 0)
		{
			curstr = dirstr[Dir_Previous];
			dot_previous(ops);
			goto do_cut;
		}
		break;
	case 10: /* backalign */
		if (strncmp((const char *)&s[1], "backalign", 9) == 0)
		{
			curstr = dirstr[Dir_Backalign];
			dot_backalign(ops);
			goto do_cut;
		}
		break;
	case 11: /* popsection */
		if (strncmp((const char *)&s[1], "popsection", 10) == 0)
		{
			curstr = dirstr[Dir_Popsect];
			dot_popsect(ops);
			goto do_cut;
		}
		break;
	case 12: /* pushsection */
		if (strncmp((const char *)&s[1], "pushsection", 11) == 0)
		{
			curstr = dirstr[Dir_Pushsect];
			dot_pushsect(ops);
			goto do_cut;
		}
		break;
	}
	/*
	* Only here if no dot_... directive function called.
	*/
	if (flags & ASFLAG_TRANSITION && obsdirect(s, n, ops) != 0)
	{
	do_cut:;
		if (p != 0 && p->olst_first == 0)
			cutolst(p);
	}
	else
	{
		error(gettxt(":776","unknown directive: %s"), prtstr(s, n));
		olstfree(ops);
	}
}
