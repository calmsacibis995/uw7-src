#ident	"@(#)acomp:common/optim.c	52.104.4.57"
/* optim.c */

/* This module contains the expression tree optimization routines. */

#include "p1.h"
#include <unistd.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>

#ifdef LINT
static int curgoal;
#define LN_OPTIM(tr,p,side)	{ curgoal = ln_getgoal(p,curgoal,side); \
                                casted = tr->flags & FF_ISCAST; \
				ln_precedence(tr); \
                                tr = optimize(tr); \
                                if (casted) \
                                    tr->flags |= FF_ISCAST; \
                                curgoal = old_curgoal; }
#endif

extern void save_fpefunc();
#ifdef FP_EMULATE
#   define save_fpefunc(fp) /* nevermind */
#else /*!FP_EMULATE*/
#include <math.h>
static void op_fpe_handler();
static jmp_buf fpe_buf;
#endif /*FP_EMULATE*/
static int op_ispow2();
static ND1 *optimize();
static ND1 *op_uand();
static ND1 *op_cascade();
static ND1 *op_right_con();
static ND1 *op_arrayref();
static ND1 *op_compare();
static int op_xtoll();
static int op_xtoull();
static FP_LDOUBLE op_lltox(), op_ulltox();
static int op_tysize();
static ND1 *op_fconfold();
static ND1 *op_iconfold();
static ND1 *op_left_con();
static ND1 *op_su();
static ND1 *op_plus();
static ND1 *op_mul();
static ND1 *op_div();
static ND1 *op_conv();
static ND1 *op_bitfield();
static ND1 *op_call();
static ND1 *op_adjust();

#define OP_NOSEFF(p)	!((p)->flags & FF_SEFF)		/* no side effects */
#define OP_ISNNCON(p)	((p)->op == ICON && (p)->sid == ND_NOSYMBOL)
#define TY_DEUNSIGN(t)  tr_deunsign(t)


#ifndef	NODBG
static void  op_oprint();
#define	ODEBUG(lev,p,s) if (o1debug > (lev)) op_oprint(p,s)
#else
#define	ODEBUG(lev,p,s)
#endif

int o1debug = 0;		/* debug flag */
static int op_foramigo;		/* don't complain about overflows */
static int op_forinit;		/* non-0 if optim for initializer; if 0,
				** certain FP operations are suppressed
				*/

INTCON	num_0;
INTCON	num_1;
INTCON	num_2;
INTCON	num_3;
INTCON	num_neg1;
INTCON	num_sc_min;
INTCON	num_sc_max;
INTCON	num_si_min;
INTCON	num_si_max;
INTCON	num_ui_max;
INTCON	num_sl_min;
INTCON	num_sl_max;
INTCON	num_ul_max;
INTCON	num_sll_min;
INTCON	num_sll_max;
INTCON	num_ull_max;

FP_LDOUBLE	flt_0;
FP_LDOUBLE	flt_1;
FP_LDOUBLE	flt_neg1;
FP_LDOUBLE	flt_sll_min_m1;
FP_LDOUBLE	flt_sll_max_p1;
FP_LDOUBLE	flt_ull_max_p1;

#define XSTR(a) #a
#define STR(a) XSTR(a)

void
op_numinit()
{
    num_init(cerror, (void *(*)())0);
    (void)num_fromulong(&num_1, 1ul);
    num_neg1 = num_1;
    (void)num_negate(&num_neg1);
    num_2 = num_1;
    (void)num_uadd(&num_2, &num_1);
    num_3 = num_2;
    (void)num_uadd(&num_3, &num_1);
    (void)num_fromslong(&num_sc_min, (long)T_SCHAR_MIN);
    (void)num_fromulong(&num_sc_max, (unsigned long)T_SCHAR_MAX);
    (void)num_fromslong(&num_si_min, (long)T_INT_MIN);
    (void)num_fromulong(&num_si_max, (unsigned long)T_INT_MAX);
    (void)num_fromulong(&num_ui_max, (unsigned long)T_UINT_MAX);
    (void)num_fromslong(&num_sl_min, (long)T_LONG_MIN);
    (void)num_fromulong(&num_sl_max, (unsigned long)T_LONG_MAX);
    (void)num_fromulong(&num_ul_max, (unsigned long)T_ULONG_MAX);
    (void)num_fromstr(&num_sll_min, STR(T_LLONG_MIN),
	sizeof(STR(T_LLONG_MIN)) - 1, (NumStrErr *)0);
    (void)num_fromstr(&num_sll_max, STR(T_LLONG_MAX),
	sizeof(STR(T_LLONG_MAX)) - 1, (NumStrErr *)0);
    (void)num_fromstr(&num_ull_max, STR(T_ULLONG_MAX),
	sizeof(STR(T_ULLONG_MAX)) - 1, (NumStrErr *)0);

    flt_0 = FP_LTOX(0L);
    flt_1 = FP_LTOX(1L);
    flt_ull_max_p1 = FP_PLUS(FP_ATOF(STR(T_ULLONG_MAX)), flt_1);
    flt_sll_max_p1 = FP_PLUS(FP_ATOF(STR(T_LLONG_MAX)), flt_1);
    flt_neg1 = FP_NEG(flt_1);
    flt_sll_min_m1 = FP_PLUS(FP_NEG(FP_ATOF(STR(T_LLONG_MIN))), flt_neg1);
}

static void
op_t1free(p)
ND1 *p;
/* Free p and all its children.  Complain if any comma ops or
** side effects are tossed when -v or -Xc.
*/
{
    if (p == 0)
	return;
    if (p->flags & FF_SEFF || p->op == COMOP) {
	if (!op_foramigo && (version & V_STD_C || verbose))
	    WERROR(gettxt(":1682", "unreached side effect or comma operator: op \"%s\""), opst[p->op]);
	t1free(p); /* no more complaints */
	return;
    }
    switch (optype(p->op)) {
    case BITYPE:
	op_t1free(p->right);
	/*FALLTHROUGH*/
    case UTYPE:
	op_t1free(p->left);
	break;
    }
    nfree(p);
}

ND1 *
op_optim(p)
ND1 *p;
{
    ODEBUG(0, p, "BEFORE OPTIM");

#ifdef LINT
    curgoal = ln_dflgoal();
#endif
    cg_treeok();		/* assume tree is okay going in */

    p = optimize(p);
    /* Structure operations sometimes get a spurious STAR operator. */
    if (p->op == STAR && TY_ISSU(p->type)) {
	nfree(p);
	p = p->left;
    }
    ODEBUG(0, p, "AFTER OPTIM");
    return p;
}

ND1 *
op_init(p)
ND1 *p;
/* Do optimizations for initializers.  Certain FP optimizations
** are enabled for initializers only.
*/
{
    op_forinit = 1;		/* flag enables special optimizations */
    p = op_optim(p);
    op_forinit = 0;		/* disable stuff again */
    return p;
}

ND1 *
op_amigo_optim(ND1 *p)
/* Do op_optim(), but for AMIGO, so *be quiet*. */
{
    op_foramigo = 1;	/* keeps overflow diagnostics off */
    p = op_optim(p);
    op_foramigo = 0;
    return p;
}

static ND1 *
optimize(p)
ND1 *p;
/* Remove unnecessary code.  Also, change implicit structure operations
** (CALL, ASSIGN, FUNARG) to explicit ones, rewrite them as appropriate
** for Pass 2.
*/
{
    ND1 *l;
    ND1 *r;
    BITOFF offset;
    OFFSET soff;
    int vol_flag = 0;		/* to save volatile flag status */
#ifdef LINT
    int old_curgoal = curgoal;
    int eval1, eval2, casted;	/* eval[12] used for eval order undefined */
#endif

    if (p->flags & FF_WASOPT)
	return p;

#ifdef LINT
    /*
    ** Check for side effects if the current goal is EFF.  If there
    ** were no side effects (ln_sides() returns 0), then set the
    ** goal to VAL to prevent future complaints.
    */
    if ((curgoal == EFF) && !ln_sides(p))
        curgoal = old_curgoal = VAL;
  
    /*
    ** Normally optimize the left subtree first (so expressions such as
    ** (a=1) && (b==a) get evaluated in the correct order),
    ** unless op is an assignment op, in which case we reverse (so
    ** a = a + 1; gets evaluated in the correct order.)
    */
    if (asgop(p->op)) {
	/* Assume all assign-ops are binary. */
	eval1 = ln_sidptr;
	LN_OPTIM(p->right, p, LN_RIGHT);
	if (p->right->op == ICON || p->right->op == FCON)
	    p = op_right_con(p);
	eval2 = ln_sidptr;
	if (optype(p->op) != LTYPE)
	    LN_OPTIM(p->left, p, LN_LEFT);
    } else if (optype(p->op) != LTYPE) {
        eval1 = ln_sidptr;
        LN_OPTIM(p->left, p, LN_LEFT);
	if (p->left->op == ICON || p->left->op == FCON)
	    p = op_left_con(p);
        if (optype(p->op) == BITYPE) {
            eval2 = ln_sidptr;
            LN_OPTIM(p->right, p, LN_RIGHT);
	    if (p->right->op == ICON || p->right->op == FCON)
		p = op_right_con(p);
        }
    }

    /*
    ** Main routines in ln_postop(); also check for evaluation order
    ** undefined.
    */
    ln_postop(p, curgoal, eval1, eval2);
    if (curgoal == EFF)
        curgoal = VAL;
#else /*!LINT*/
    /* Do left-side first to check for ||, && with constant operand. */
    if (optype(p->op) != LTYPE) {
	p->left = optimize(p->left);
	if (p->left->op == ICON || p->left->op == FCON)
	    p = op_left_con(p);
	/* p's optype could be different now. */
    }
    if (optype(p->op) == BITYPE) {
	p->right = optimize(p->right);
	if (p->right->op == ICON || p->right->op == FCON)
	    p = op_right_con(p);
	/* p's optype could be different now. */
    }
#endif /*LINT*/

    l = p->left;
    r = p->right;

    /* Check for constant folding possibilities */
    /* As a result of arithmetic promotions, ICON op FCON (and vice versa)
    ** are now FCON op FCON.
    */
    switch (optype(p->op)) {
    case UTYPE:
	if (l->op == ICON)
	    p = op_iconfold(p);
	else if (l->op == FCON)
	    p = op_fconfold(p);
	break;
    case BITYPE:
 	if ((l->op == ICON) && (r->op == ICON))
	    p = op_iconfold(p);
	else if ((l->op == FCON) && (r->op == FCON))
	    p = op_fconfold(p);
	else if (logop(p->op) && l->op == ICON && r->op == FCON)
	    p = op_fconfold(p);
	break;
    }

    switch (p->op) {
    case UNARY AND:
	switch (l->op) {
	SY_CLASS_t sc;

	case NAME:
	    /* Turn & over NAME into an ICON
	    **
	    **		&
	    **		|    =>    ICON
	    **	       NAME
	    */
	    if (l->sid > 0) {
	        if ((sc = SY_CLASS(l->sid)) == SC_AUTO)
		    break;
		else if (sc == SC_PARAM) {
#ifdef	SU_PARAM_PTR
		    if (TY_ISSU(SY_TYPE(l->sid))) {
			nfree(p);
			l->type = ty_mkptrto(l->type);
			return l;
		    }
#endif
		    break;
		}
	    }
	    (void)num_fromslong(&l->c.ival, l->c.off);
	    l->op = ICON;
	    /* FALLTHRU */
	case STRING:
	    /* Treat STRING similar to  NAME:  set ICON bit. */
	    if (l->op == STRING)
		l->sid |= TR_ST_ICON;
	    l->type = p->type;
	    nfree(p);
	    p = l;
	    break;
	case STAR:
	    /* Eliminate & over STAR 
	    **
	    **		&
	    **		|    =>    nothing
	    **		*
	    */
	    ODEBUG(0, p, "U& over STAR optimizing");
	    nfree(p);
	    nfree(l);
	    l->left->type = p->type;	 /* preserve type of p */
	    p = l->left;
	    ODEBUG(0, p, "after U&-->* optimization");
	    break;
	}

	break;
    case FUNARG:
	/* structure argument tree rewrite */
	if (TY_ISSU(p->type))
	    p = op_su(p, STARG);
	break;
    case ASSIGN:
        /* structure assignment tree rewrite */
	if (TY_ISSU(p->type)) {
	    p = op_su(p, STASG);
	    break;
	}
	/* Rewrite a = a op b to a op= b.
	**
	**	=				op=
	**    /   \			       /   \
	**  NAME   op		==>	     NAME   X
	**	 /    \
	**	NAME   X
	**
	** There's a special case when the NAME has type
	** other than int.  The trees look like:
	**
	**	=				op=
	**    /   \			       /   \
	**  NAME   CONV		==>	     CONV   X
	**	    |			       |
	**	    op			     NAME
	**	  /    \
	**	CONV	X
	**	  |
	**	NAME
	**
	** The p->right->op CONV is to the type of the =;
	** the other is to the type of op.
	** This improvement is safe only if the target machine does
	** not trap overflows, because we're changing the "size" of
	** the operation.
	*/
	if (l->op == NAME) {
	    int convcase = 0;		/* CONV case */

#ifndef TRAP_OVERFLOW
	    if (r->op == CONV) {
		convcase = 1;
		r = r->left;
	    }
#endif
	    switch (r->op) {
	    case OR:
	    case AND:
	    case ER:
	    case LS:
	    case RS:
		if (TY_ISFPTYPE(l->type))
		    break;
		/* FALLTHRU */
	    case PLUS:
	    case MINUS:
	    case MUL:
	    case DIV:
	    case MOD:
	    {
		register ND1 *rl = r->left;

		if (convcase) {
		    if (rl->op != CONV)
			break;
		    rl = rl->left;
		}

		/* left side of ASSIGN must be == to l */
		if (   rl->op == NAME
		    && rl->sid == l->sid
		    && rl->c.off == l->c.off
		    && rl->type == l->type)
		{
		    ODEBUG(1, p, "rewrite ASSIGN to ASG OP: change:");
		    /* free original top of tree, change op */
		    nfree(p->left);
		    nfree(p);
		    if (convcase)
			nfree(p->right);
		    r->op += ((ASG PLUS) - PLUS);
		    p = r;
		    p->type = l->type;		/* really for CONV case */
		    p->flags |= FF_SEFF;
		    ODEBUG(1, p, "to:");
		} /* end if */
	    } /* end block */
	    } /* end switch on right op */
	    l = p->left;			/* restore pointers */
	    r = p->right;
	} /* end if */
    	break;
    case CALL:
    case UNARY CALL:
	/* structure call tree rewrite */
		
	if (p->left->op == ICON && sy_function_type(p->left->sid) != func_unknown) 
		p->left->flags |= FF_BUILTIN;
	if (TY_ISSU(p->type))
	    p = op_su(p, (p->op == CALL ? STCALL : UNARY STCALL));
	else if (!op_forinit && (p->left->flags & FF_BUILTIN))
	    p = op_call(p);		/* check for built-in rewrites */
	break;
    case STAR:
	switch (l->op) {
	case UNARY AND:
	    /* Eliminate STAR over &
	    **
	    **	*
	    **	|    =>    nothing
	    **	&
	    */
	    nfree(l);
	    l->left->type = p->type;		/* preserve the type of STAR */
	    nfree(p);
	    p = l->left;
	    break;
	case ICON:
	    /* STAR over ICON becomes NAME
	    **
	    **	*
	    **	|        =>    NAME+val
	    ** ICON(val)
	    */
	    /* Make this into NAME.  If the value can be an offset. */
	    if (num_toslong(&l->c.ival, &soff) != 0)
		break;
	    l->c.off = soff;
	    nfree(p);
	    l->op = NAME;
	    l->type = p->type;
	    if (TY_ISVOLATILE(l->type) || (p->flags&FF_ISVOL)) 
		l->flags |= FF_ISVOL;
	    p = l;
	    break;
	case STRING:
	    /* STAR over STRING reverts to STRING.  Type is STAR's type.
	    **
	    **	*
	    **	|	=>	STRING
	    ** STRING
	    */
	    l->type = p->type;
	    l->sid &= ~TR_ST_ICON;	/* treat like pseudo NAME node */
	    nfree(p);
	    p = l;
	    break;
	}
	break;
    case ASG MINUS:
    case MINUS:
	/* Change MINUS of constant to plus of its negative, if possible. */
	if (r->op == FCON) {
	    r->c.fval = FP_NEG(r->c.fval);
	    p->op += (PLUS - MINUS);
	} else if (OP_ISNNCON(r)) {
#ifdef TRAP_OVERFLOW
	    INTCON tmp = r->c.ival;

	    if (num_negate(&tmp) == 0 || TY_ISUNSIGNED(r->type)) {
		r->c.ival = tmp;
		p->op += (PLUS - MINUS);
	    }
#else /*!TRAP_OVERFLOW*/
	    (void)num_negate(&r->c.ival);
	    p->op += (PLUS - MINUS);
	} else {
	    p->right = tr_generic(UNARY MINUS, r, r->type);
	    p->op += (PLUS - MINUS);
#endif /*TRAP_OVERFLOW*/
	}
	/*FALLTHRU*/
    case ASG PLUS:
    case PLUS:
	p = op_plus(p);
	break;
    case ASG MUL:
    case MUL:
	p = op_mul(p);
	break;	
    case ASG DIV:
    case DIV:
	p = op_div(p);
	break;
    case CONV:
	p =  op_conv(p);
	break;
    case DOT:
	/* rewrite DOT operator, turn it into a pointer to p
	** then rewrite like the STREF op.
	*/
	if (p->right->flags & FF_ISVOL)
	    l->flags |= FF_ISVOL;
	p->left = l = op_uand(l, ty_mkptrto(l->type));
	/*FALLTHRU*/
    case STREF:
	/* if a pointer to volatile su, set vol_flag */
	if (TY_ISPTR(l->type) && TY_ISVOLATILE(TY_DECREF(l->type)))
	    vol_flag = 1;
	/* rewrite STREF op, into pointer plus offset */
	/* if p is a bitfield, building the offset is different */
	if (p->flags & FF_ISFLD) {
	    p = op_bitfield(p); 
	    break;
	}
	offset = SY_OFFSET(r->sid);		/* get offset in struct */

	/* Build pointer plus. Tree will look like:
	**
	**		*
	**		|
	**		+
	**	      /   \
	**	   NAME   ICON
	*/
	if (offset) {		/* only build plus if offset is non-zero */
	    p->op = PLUS;
	    p->flags &= ~FF_ISVOL;	/* don't want the + node set */
	    p->type = ty_mkptrto(p->type);
	    p->right = tr_smicon(BITOOR(offset));
	    p = tr_generic(STAR, p, r->type); 
	    nfree(r);
	}
	else {
	    nfree(p);
	    nfree(r);
	    p = l;
	    p = tr_generic(STAR, p, r->type);
	}
	ODEBUG(1, p, "rewritten structure reference");
	p = optimize(p);
	if (vol_flag)
	    p->flags |= FF_ISVOL; 
	break;

    case NAME:
	/* Change member of ENUM into integer constant. */
	if (p->sid > 0 && SY_CLASS(p->sid) == SC_MOE) {
	    (void)num_fromslong(&p->c.ival, (long)SY_OFFSET(p->sid));
	    p->sid = ND_NOSYMBOL;
	    p->op = ICON;
	}
	break;

    /* Handle cascaded constants. */
    case OR:
    case ER:
    case AND:
    case LS:
    case RS:
	if (OP_ISNNCON(r))
	    p = op_cascade(p);
	break;
    case EQ: case NE:
    case LT: case NLT: case ULT: case UNLT:
    case LE: case NLE: case ULE: case UNLE:
    case GT: case NGT: case UGT: case UNGT:
    case GE: case NGE: case UGE: case UNGE:
    case LG: case NLG: case ULG: case UNLG:
    case LGE: case NLGE: case ULGE: case UNLGE:
	p = op_compare(p);
	break;
    } /*end switch*/

    /* Fix up assignment ops with CONV on left, if possible. */
    if (    (asgop(p->op))
	&&  ((l = p->left)->op == CONV)
	&&  (!TY_ISFPTYPE(l->type)) 
	&&  (TY_ISNUMTYPE(l->type))
	&&  (TY_ISNUMTYPE(l->left->type))
	&&  (op_tysize(l->type) >= op_tysize(l->left->type))
       ) {
	switch (p->op) {
	case ASG OR:
	case ASG ER:
	case ASG AND:
	    if (TY_ISFPTYPE(l->left->type))
		break;
	    /* FALLTHRU */
	case ASG MUL:
	case ASG PLUS:
	case ASG MINUS:
	    /* transfer CONV ops on left-side to the right-side
	    ** don't do it to ASG DIV, ASG MOD, ASG RS, ASG LS.
	    **
	    **    (CONV) A op= B  into A op= (CONV) B
	    */
	    ODEBUG(1, p, "*** ASG OP optimization ***");
	    p->left = l->left;
	    l->left = p->right;
	    l->type = p->type;
	    l->flags &= ~FF_WASOPT;	/* allow further optimization */
	    p->right = optimize(l);
	    /* unset volatile field for right-side when an ICON */
	    if (p->right->op == ICON)
		p->right->flags &= ~FF_ISVOL;
	    ODEBUG(1,  p, "*** result of ASG OP optim ***");
	    break;
	}
    }

    p->flags |= FF_WASOPT;
    return p;
}

static ND1 *
op_cascade(p)
ND1 *p;
/* Look for cascaded operators with constants on the right.
** Try to fold them if there is no overflow.
*/
{
    ND1 *l = p->left;
    ND1 *r = p->right;
    ND1 *lr;
    INTCON v;
    int is_fp_opt = 0;
    int sav_op_foramigo;

    ODEBUG(1, p, "Start cascade optimization");

    /* Look for binary above binary on the left in which either
    ** the middle op is an ICON or
    ** both the middle and right ops are FCONs (when not IEEE).
    */
    if (optype(p->op) != BITYPE || optype(l->op) != BITYPE)
	return p;
    lr = l->right;
    if (lr->op != ICON) {
	if (lr->op != FCON || r->op != FCON || ieee_fp())
	    return p;
	is_fp_opt = 1;
    }

    if (!is_fp_opt && !OP_ISNNCON(r)) {
	/* Commute operators, where possible, if overflow is not
	** a problem.  This moves constants rightward in the
	** tree in hopes of finding a place to fold.
	*/
	if (p->op == l->op) {
	    switch (p->op) {
#ifndef TRAP_OVERFLOW
	    case PLUS:
	    case MUL:
#endif
	    case AND:
	    case OR:
	    case ER:
		p->right = lr;
		l->right = r;
		/* We may interchange ICON ptr lr with int
		** expression @ r.  Adjust type.
		*/
		if (TY_ISPTR(lr->type))
		    l->type = r->type;
	    }
	}
#ifndef	TRAP_OVERFLOW
	else if (p->op == MUL && l->op == LS && OP_ISNNCON(l->right)) {
	    /* In this case the multiply may have been changed to a
	    ** left shift:
	    **
	    **		*			<<
	    **	      /   \		      /    \
	    **	     <<	  E2	  ==>	     *	  ICON
	    **	   /	\		   /   \
	    **	  E1   ICON		  E1   E2
	    */
	    p->left = l->left;
	    l->left = p;
	    p = l;
	}
#endif
	return p;
    }

    /* Now have this tree, where the top node is not an
    ** assignment operator, but the lower one might be.
    ** For appropriate combinations of OPs (a) and (b),
    ** generally transform the tree as follows and let
    ** op_Xcon_fold() and op_right_con() clean it up.
    **
    **	       OP(b)		  OP(b)
    **	      /    \		 /    \
    **	    OP(a)  CON(b)   =>	T    OP(a)
    **     /    \		    /    \
    **    T     CON(a)		CON(b)  CON(a)
    **
    ** Note that since we've regrouped the tree, there
    ** can be overflow situations introduced that would
    ** not be present in the abstract machine.  Go ahead
    ** and (silently!) fold, but only for machine that do
    ** not care about integer overflow.
    */

    /* Determine the combinations that we can deal with. */
#define OP_OP(l,t) ((t)*DSIZE+l) /* checks a pair at once */
    switch (OP_OP(l->op, p->op)) {
    default:
	return p;
    case OP_OP(PLUS, MINUS):
	l->op = MINUS;
	if (r->sid == ND_NOSYMBOL) { /* swap ICONs for PLUS over MINUS */
	    p->op = PLUS;
	    p->right = l->right;
	    l->right = r;
	    r = p->right;
	}
	break;
    case OP_OP(MINUS, MINUS): /* probably not reached */
	l->op = PLUS;
	break;
    case OP_OP(PLUS, PLUS):
    case OP_OP(MINUS, PLUS):
    case OP_OP(MUL, MUL):
    case OP_OP(OR, OR):
    case OP_OP(ER, ER):
    case OP_OP(AND, AND):
	break;
#ifndef TRAP_OVERFLOW
    case OP_OP(MUL, LS):
	/* Exchange p and l and handle like MUL over LS */
	p->op = MUL;
	l->op = LS;
	p->right = l->right;
	l->right = r;
	r = p->right;
	/*FALLTHROUGH*/
    case OP_OP(LS, MUL):
	break;
#endif
    case OP_OP(RS, RS):
#ifdef C_SIGNED_RS
	/* Shift ops are special in that their left operand's type
	** determines the operation kind.  Since we might have painted
	** a [un]signed on the op above a differently signed operand,
	** we can only collapse these when the left and left's left
	** have the same type.
	*/
	if (!TY_EQTYPE(l->type, l->left->type)) return p;
	/*FALLTHROUGH*/
#endif
    case OP_OP(LS, LS):
	/* Avoid if shifting too far. */
	num_fromulong(&v, TY_SIZE(p->type));
	if (num_usubtract(&v, &lr->c.ival) != 0) /* CON(a) > TY_SIZE() */
	    return p;
	if (num_ucompare(&v, &r->c.ival) <= 0) /* CON(a)+CON(b) >= TY_SIZE() */
	    return p;
	l->op = PLUS;
	break;
    }
    p->left = l->left;
    l->left = r;
    l->type = r->type;
    sav_op_foramigo = op_foramigo;
    op_foramigo = 1;
    r = p->right = is_fp_opt ? op_fconfold(l) : op_iconfold(l);
    op_foramigo = sav_op_foramigo;
    if (r->op == ICON || r->op == FCON)
	p = op_right_con(p);
    ODEBUG(1, p, "End cascade optimization");
    return p;
}

static ND1 *
op_plus(p)
ND1 *p;
/* Optimize PLUS or ASG PLUS node, MINUS or ASG MINUS: do
** comm/assoc optimizations
*/
{
    ND1 *r = p->right;
    ND1 *l = p->left;

    /* Move vanilla ICON's to the right (and leave pointer
    ** ICON's on the left.)
    */
    if (OP_ISNNCON(l)) {
	p->left = r;
	r = p->right = l;
	l = p->left;
    }
    ODEBUG(1, p,"Optimizing PLUS");

    /* C-tree:
    **	       +/-
    **	      /   \		        &
    **	     &    ICON(val)    =>       |
    **	     |			   NAME+/-val
    ** 	    NAME
    **
    ** This tree must be pointer addition, in which case the value
    ** on the right is a byte offset.
    */
    if (l->op == UNARY AND && OP_ISNNCON(r) && (l->left->op == NAME))
    {
	ND1 *ll = l->left;
	long off;
#ifndef LINT
	INTCON tmp;
	int space;

	switch (SY_CLASS(ll->sid)) {
	case SC_PARAM:	space = VPARAM; break;
	case SC_AUTO:	space = VAUTO; break;
	default:
	    cerror(gettxt(":278","op_plus:  unexpected class %d"), SY_CLASS(ll->sid));
	}
	(void)num_fromulong(&tmp, (unsigned long)TY_SIZE(TY_CHAR));
	if (p->op == MINUS)
	    (void)num_negate(&tmp);
	if (num_smultiply(&tmp, &r->c.ival) || num_toslong(&tmp, &off))
	    return p; /* no other optimizations */
	ll->c.off = cg_off_incr(space, ll->c.off, off);
#else
	if (num_toslong(&r->c.ival, &off))
	    return p; /* no other optimizations */
	if (p->op == MINUS)
	    off = -off;
	ll->c.off += off;
#endif
	l->type = p->type;
	nfree(p);
	nfree(r);
	return l;
    }

    /* Try to fold certain array or pointer reference operations. */

#ifndef TRAP_OVERFLOW
    if (TY_ISPTR(p->type) && !asgop(p->op))
	p = op_arrayref(p);
#endif

    /* Now move named ICON to right of PLUS to improve cascade optimizations. */
    if (p->op == PLUS && (l = p->left)->op == ICON && l->sid != ND_NOSYMBOL) {
	p->left = p->right;
	p->right = l;
    }

    /* Check for cascaded PLUS's. */
    p = op_cascade(p);
    l = p->left;
    r = p->right;

    /* Minimize number of negatives. */
    switch (p->op) {
	int change;

    case PLUS:
    case ASG PLUS:
    case MINUS:
    case ASG MINUS:
	change = 0;
	if (r->op == UNARY MINUS) {
	    nfree(r);
	    p->right = r = r->left;
	    change = 1;
	}
	else if (OP_ISNNCON(r) && num_scompare(&r->c.ival, &num_0) < 0) {
	    /* Change +/- of negative constant (back) to -/+ of positive. */
#ifdef TRAP_OVERFLOW
	    if (TY_ISSIGNED(p->type)) {
		INTCON tmp;

		tmp = r->c.ival;
		if (num_negate(&tmp) == 0) {
		    r->c.ival = tmp;
		    change = 1;
		}
	    }
#else
	    (void)num_negate(&r->c.ival);
	    change = 1;
#endif
	}
	if (change) {
	    if (p->op == PLUS || p->op == ASG PLUS)
		p->op += (MINUS - PLUS);
	    else
		p->op += (PLUS - MINUS);
	}
    }

    /* In cascaded pointer expression, try to move ICON to right,
    ** in hopes it might be used in register/offset address mode.
    ** The + case is already handled by op_cascade().
    **
    **		+|-				  -
    **	      /     \			       /    \
    **	      -     X2		==>	     +|-   ICON
    **     /     \			    /    \
    **    X1	ICON			   X1	 X2
    */
#ifndef TRAP_OVERFLOW	/* because we're rearranging operators */
    /* recheck p->op: previous optimizations may have reduced tree */
    if ((p->op == PLUS || p->op == MINUS)
	&& TY_ISPTR(p->type) && !OP_ISNNCON(r)
       ) {
	if (l->op == MINUS && OP_ISNNCON(l->right)) {
	    /* Move the ICON to the right of p. */
	    ODEBUG(1, p, "Before +|- ICON rotate");
	    p->left = l->left;
	    l->left = p;
	    p = l;
	    ODEBUG(1, p, "After +|- ICON rotate");
	}
    }
#endif
    return p;
}

#ifndef	TRAP_OVERFLOW	/* avoid because of generated constant multiply */

static ND1 *
op_arrayref(p)
ND1 *p;
/* Fold array and pointer references. */
{
    ND1 *l;
    ND1 *r = p->right;
    ND1 *rl;
    ND1 *new;
    int charcase;

    /* Look for trees that would result from an expression like
    **		array[X+C]
    **		array[X-C]
    **		array[C-X]
    ** where array is some array type, X is some index expression,
    ** C is a constant.  The idea is to distribute the scaling
    ** over the lower +/- and then the scaled C from above can be
    ** folded into the array node.
    **
    ** The tree transform looks like:
    **
    **		+|-			+|-
    **	       /   \		       /   \
    **	      &	   *|<<	     -->     +|-    \
    **	    NAME  /	\	   /    \    *|<<
    **		+|-	C2	  &    *|<<  /   \
    **	       /   \		NAME  /   \  X	  C2 
    **	      X	    C1		    C1    C2
    **
    ** A couple of notes:  (& NAME) could look like ICON if
    ** the storage class for NAME is SC_STATIC or SC_EXTERN.
    ** The expectation is that the C1 * C2 tree will be
    ** collapsed and folded into the (& NAME) tree.
    ** Of course, the type of the top tree is PTR.
    ** NOTE:  There is a special case when C2 is 1 (for char
    ** arrays), in which case the *|<< tree is missing.
    **
    ** Look for trees that would result from an expression like
    **		ptr[X+C]
    **		ptr[X-C]
    **		ptr[C-X]
    ** where ptr is some pointer type, X is some index expression,
    ** C is a constant.
    ** The tree transform looks like:
    **
    **		+|-			+|-
    **	       /   \		       /   \
    **	      p	   *|<<	     -->     +|-    \
    **	         /	\	   /    \    *|<<
    **		+|-	C2	  p    *|<<  /   \
    **	       /   \		      /   \ C1	  C2 
    **	      X	    C1		     X    C2
    **
    ** The expectation is that the C1 * C2 tree will be
    ** collapsed and can be used as part of a *(REG + offset)
    ** address mode, where the pointer expression gets evaluated
    ** into a register.  On machines with fancy double indexing
    ** address modes, the shift/multiply might get handled by
    ** the address mode, too.
    **
    ** NOTE:  There is a special case when C2 is 1 (for char
    ** arrays), in which case the *|<< tree is missing.
    **
    */

    /* Look for a match on the right side first, because it is
    ** common to the two transformations.
    */
    switch (r->op) {
    case PLUS:
    case MINUS:
	charcase = 1;
	break;
    case MUL:
    case LS:
	rl = r->left;
	if ((rl->op == PLUS || rl->op == MINUS) && OP_ISNNCON(rl->right)) {
	    charcase = 0;
	    break;
	}
	return p;			/* no match */
    default:				/* other operators */
	return p;			/* no match */
    }

    if (!OP_ISNNCON(r->right))
	return p;

    /* Right side matches.  Check the left sides for array or ptr cases. */

    if (   (l = p->left)->op == ICON
	|| (l->op == UNARY AND && l->left->op == NAME)
    ) {
	/* Array case */
	if (charcase) {
	    ODEBUG(1, p, "Begin char array index rewrite");

	    rl = r->left;		/* the non-constant expression */
	    r->left = l;
	    p->left = r;
	    p->left->type = p->type;	/* pointer arithmetic */
	    p->right = rl;

	    ODEBUG(1, p, "End char array index rewrite");
	}
	else {
	    ODEBUG(1, p, "Begin array index rewrite");

	    /* Patch up right side first. */
	    r->left = rl->left;		/* rl still has +|- tree */

	    /* Do left side tree. */
	    rl->left = l;
	    l = p->left = rl;
	    l->type = p->type;		/* pointer arithmetic */

	    new = t1alloc();		/* will be new *|<< node */
	    *new = *r;
	    new->flags &= ~FF_WASOPT;	/* will want to optimize again */
	    new->left = rl->right;
	    l->right = new;
	    new = new->right = t1alloc();	/* copy of C2 */
	    *new = *(r->right);

	    ODEBUG(1, p, "End array index rewrite");
	}
    }
    else {
    /* Assume pointer cases. */
	if (charcase) {
	    ODEBUG(1, p, "Begin char pointer index rewrite");

	    rl = r->left;		/* the non-constant expression */
	    r->left = p;
	    r->type = p->type;		/* pointer arithmetic */
	    p->right = rl;
	    p = r;

	    ODEBUG(1, p, "End char pointer index rewrite");
	}
	else {
	    /* More complicated cases. */

	    ODEBUG(1, p, "Begin pointer index rewrite");

	    /* Create final left-side tree. */
	    r->left = rl->left;		/* rl still has +|- tree */

	    /* Current p eventually becomes left side.  Put together
	    ** new p and right side.
	    */
	    /* Build new p. */
	    l = p;
	    p = t1alloc();
	    p->op = rl->op;
	    p->flags = 0;
	    p->type = l->type;
	    p->left = l;
	    p->right = rl;

	    /* Put together new MUL/LS tree with both operands. */
	    rl->left = rl->right;
	    new = t1alloc();		/* will be new ICON scale node */
	    *new = *(r->right);
	    rl->right = new;
	    rl->op = r->op;
	    p->left->right->flags &= ~FF_WASOPT;

	    ODEBUG(1, p, "End pointer index rewrite");
	}
    }
    p->left->flags &= ~FF_WASOPT;
    p->right->flags &= ~FF_WASOPT;
    p->left = optimize(p->left);
    p->right = optimize(p->right);
    return p;
}

#endif /*TRAP_OVERFLOW*/

int suppress_div_to_mul; /* turned on by cplusbe code */

static ND1 *
op_div(p)
ND1 *p;
/* Optimize DIV, ASG DIV: for floating point, multiply instead of divide
** for powers of 2.
*/
{
    ND1 *r = p->right;

    if (r->op != FCON)
	return p;

    if (FP_ISPOW2(r->c.fval) || (!ieee_fp() && !suppress_div_to_mul
#ifndef FP_EMULATE
	&& !FP_ISZERO(r->c.fval)
#endif
	))
    {
	ODEBUG(1, p, "Optimizing DIV");
	p->op += (MUL - DIV);
	r->c.fval = FP_DIVIDE(flt_1, r->c.fval);
	ODEBUG(1, p, "DIV to MUL optimization");
    }
    return p;
}

static ND1 *
op_mul(p)
ND1 *p;
/* Optimize MUL, ASG MUL node: do comm/assoc, MUL to LS optimizations. */
{
    ND1 *l = p->left;
    ND1 *r = p->right;
    int pow;

    /* Work with constants on the right. */
    if (   (l->op == ICON && r->op != ICON)
	|| (l->op == FCON && r->op != FCON)
    ) {
	p->left = r;
	p->right = l;
	l = p->left;
	r = p->right;
    }
    if (TY_ISVOLATILE(l->type))
	p->type = l->type;
    ODEBUG(1, p, "Optimizing MUL");
    /* C-tree:
    **		*				LS
    **	      /   \		=>	      /    \
    **	   NAME  ICON(pow^2)		    NAME   ICON(pow)
    */
    if (OP_ISNNCON(r) && (pow = op_ispow2(&r->c.ival)) >= 0) {
	ODEBUG(1, p, "MUL to LS Optimization");
	if (pow == 0 && !(TY_ISVOLATILE(l->type) && asgop(p->op))) {
	    /* x*1 */
	    nfree(p);
	    nfree(r);
	    return l;
	}
	if (pow > 0) {
	    num_fromslong(&r->c.ival, (long)pow);
	    p->op += (LS - MUL);
	}
    }
    /* Look for cascades of operators. */
    return op_cascade(p);
}


static int
op_ispow2(vp)
const INTCON *vp;
/* Determine if constant *vp is a power of 2. */
{
    int ans = num_highbitno(vp);
    INTCON tmp;

    if (ans > 0) {
	tmp = num_1;
	num_llshift(&tmp, ans);
	if (num_ucompare(&tmp, vp) != 0)
	    ans = -1;
    }
    return ans;
}

static ND1 *
op_bitfield(p)
ND1 *p;
/* op_bitfield() rewrites DOT and STREF subtrees whose member of
** struct/union referenced is a bitfield.  p is the DOT or STREF
** subtree.
*/
{
    BITOFF offset, off, size, align;
    ND1 *l = p->left;
    ND1 *r = p->right;
    T1WORD rtype = r->type;
    int vol_flag = 0;

    if (TY_ISPTR(l->type) && TY_ISVOLATILE(TY_DECREF(l->type))) vol_flag = 1;

#ifdef ALFIELD
    align = ALFIELD;
#else
    align = TY_ALIGN(rtype);
#endif
    /* Rewriting the bitfield DOT or STREF subtrees is different
    ** in that the offset and size of the bitfield is important.
    ** The macro SY_FLDUPACK returns the size and offset of the
    ** first argument which is the SID.
    */
    SY_FLDUPACK(r->sid, size, offset);	/* get size and offset */
    nfree(r);
    off = (offset/align) * align; 

    if (off != 0) {
	/* Build pointer plus. */
	p->op = PLUS;
	p->flags &= ~FF_ISVOL;
	p->right = tr_smicon(BITOOR(off));
	offset -= off;
    }
    else {
	nfree(p);
	p = l;
    }
    p->type = ty_mkptrto(rtype);
    p = tr_generic(STAR, p, rtype);
    if (vol_flag)
	p->flags |= FF_ISVOL;
    ODEBUG(1, p, "Before rewrite of bitfield reference");
    p = optimize(p);
    /* If the field has the size/alignment of its type, do not
    ** bother building a FLD node.  Remember that bit-fields have
    ** integral type.  (No check of type required.)
    */
    if (! ( size == TY_SIZE(rtype)
	    && (off % align) == 0
	  )
    ) {
	p = tr_generic(FLD, p, p->type);
	p->c.off = PKFIELD(size, offset);
    }
#ifndef SIGNEDFIELDS
    /* Change type to unsigned type.  Enums are treated as unsigned ints. */
    else {
	int qual = TY_GETQUAL(p->type);

	switch (TY_TYPE(p->type)) {
	case TY_ENUM:
	case TY_INT:
	    p->type = ty_mkqual(TY_UINT, qual);
	    break;
	case TY_LLONG:
	    p->type = ty_mkqual(TY_ULLONG, qual);
	    break;
	case TY_LONG:
	    p->type = ty_mkqual(TY_ULONG, qual);
	    break;
	case TY_SHORT:
	    p->type = ty_mkqual(TY_USHORT, qual);
	    break;
	case TY_CHAR:
	    p->type = ty_mkqual(TY_UCHAR, qual);
	    break;
	}
    }
#endif
    ODEBUG(1, p, "After rewrite of bitfield reference");
    return p;
}


static ND1 *
op_su(p, newop)
ND1 *p;
int newop;
/* Rewrite structure/union-related operations to explicit structure operation
** newop.  Structure/union operands become pointer to same.  Type of
** operations also becomes pointer to structure/union.  Stick a STAR
** above the new operator to retain type correctness of this subtree.
*/
{
    T1WORD otype = p->type;		/* original type */
    T1WORD tpsu = ty_mkptrto(otype);	/* new type for operands, op */
    int vol_flag = 0;			/* save volatile type status */

    switch (newop) {
    case STASG:
	/* On assignment, the type of the root is the left type.
	** When the right side is volatile, this info can be
	** lost unless we save it here.
	*/
	if (TY_ISVOLATILE(p->right->type)) vol_flag = 1;
	p->right = op_uand(p->right, tpsu);
	/* Don't put U& over RNODE. */
	if (p->left->op != RNODE) {
	    /*FALLTHRU*/
    case STARG:
	    p->left = op_uand(p->left, tpsu);
	}
	/* if the struct is volatile or a member within is vol, set field */
	if (TY_ISVOLATILE(otype) 
	    || vol_flag
	    || TY_ISMBRVOLATILE(otype))
	        p->flags |= FF_ISVOL;
	/*FALLTHRU*/
    case STCALL:
    case UNARY STCALL:
	/* Set new op and type for original node. */
	p->op = newop;
	p->type = tpsu;
	p->sttype = TY_DECREF(tpsu); /* tuck away type in sttype */
	if (newop != STARG) /* Preserve semantics, except for arg. */
	    p = tr_generic(STAR, p, otype);
	break;

    default:
	cerror(gettxt(":279","confused op_su(), op %d"), newop);
    }
    return p;
}

static ND1 *
op_conv(p)
ND1 *p;
/* optimize CONV nodes */
{
    ND1 *l = p->left;
    T1WORD ptype = p->type;
    T1WORD ltype = l->type;

    ODEBUG(0, p, "op_conv works on");
    /* free up CONV node now if both types are the same */
    if (TY_EQTYPE(ptype, ltype) == 1) {
#ifdef FP_EXTENDED
	/* All fp arithmetic done in extended precision.  A
	** cast must cause the expression to lose precision (ANSI).
	** If the cast is over a constant, optimize anyway since
	** the arithmetic is done in constant folding.  If we are
	** generating fast floating point code, ignore this ANSI
	** restriction.
	*/
	if (	(p->flags & FF_ISCAST)
	     && (l->op != ICON)
	     && (l->op != FCON)
	     &&	(TY_TYPE(ptype) == TY_FLOAT || TY_TYPE(ptype) == TY_DOUBLE)
	     && (ieee_fp())
	   )
	    return(p);
#endif
	nfree(p);
	ODEBUG(0, l,"equal types, result tree");
	return l;
    }

    /* Treat pointer type like int type if one is going to be
    ** converted to the other type anyway.  Rely on other transforms
    ** to clean up.
    */
    if (TY_ISINTTYPE(ptype) && TY_ISPTR(ltype))
	ltype = T_ptrtype;
    else if (TY_ISPTR(ptype) && TY_ISINTTYPE(ltype))
	ptype = T_ptrtype;

    /* If integral CONV is over another integral and they are
    ** the same size, free the CONV, paint over type.
    ** Or pointer CONV is over another pointer, do the same.
    */
    if (   (   (TY_ISINTTYPE(ptype) && TY_ISINTTYPE(ltype))
	    || (TY_ISPTR(ptype) && TY_ISPTR(ltype))
	   )
	&& TY_SIZE(ltype) == TY_SIZE(ptype)
	&& l->op != CONV
#ifdef SIGN_CONV
	&& TY_ISUNSIGNED(ptype) == TY_ISUNSIGNED(ltype)
#endif
    ) {
	ptype = p->type;
	ODEBUG(0, p, "CONV over same size integral");
	goto conv_paint;
    }

    if (TY_ISPTR(ltype) || TY_ISARY(ltype)) return (p);

    switch (l->op) {
    case ICON:
	if (TY_ISFPTYPE(ptype)) {
	    FP_LDOUBLE resval;
	    INTCON tmp;
	    int isunsigned = TY_ISUNSIGNED(l->type);

#ifdef NO_LDOUBLE_SUPPORT
	    if (TY_TYPE(ptype) == TY_LDOUBLE)
		cg_ldcheck();		/* check long double warning */
#endif

	    resval = (*(isunsigned ? &op_ulltox : &op_lltox))(&l->c.ival);

	    /* truncate to float/double if that's what we need */
	    if (TY_TYPE(ptype) == TY_FLOAT)
		resval = op_xtofp(resval);
	    else if (TY_TYPE(ptype) == TY_DOUBLE)
		resval = op_xtodp(resval);

	    /* Can make change if for initializer or if result is exact,
	    ** or if we are generating fast floating point code.
	    */
	    if (!op_forinit && ieee_fp()) {
		if ((*(isunsigned ? &op_xtoull : &op_xtoll))(&tmp, resval) != 0)
		    break;
		if (num_ucompare(&tmp, &l->c.ival) != 0)
		    break;
	    }
	    l->op = FCON;
	    l->c.fval = resval;
	}
	goto conv_paint;
    case FCON:
        /* a CONV over an FCON can be removed (for initializers only). */
	if (TY_ISFPTYPE(ptype)) {	/* float, double, or long double */
	    if (TY_TYPE(ptype) == TY_LDOUBLE)
#ifdef NO_LDOUBLE_SUPPORT
		cg_ldcheck();		/* check long double warning */
#else
		/* EMPTY */ ;
#endif

	    else if (TY_TYPE(ptype) == TY_FLOAT) {
		/* shrink doubles and long doubles to float */
		FP_LDOUBLE resval;

		resval = op_xtofp(l->c.fval); 
		if (errno) {
		    static const char mesg[] =
			"conversion to float is out of range"; /*ERROR*/

		    if (op_forinit)
			UERROR(gettxt(":392", mesg));
		    else
			WERROR(gettxt(":392", mesg));
		}
		/* If not for initializer, can only do this if value
		** is unchanged, or if we are generating fast floating
		** point code.
		*/
		if (!op_forinit && ieee_fp()) {
		    if (FP_CMPX(resval, l->c.fval) != FPEMU_CMP_EQUALTO)
			break;
		}
		l->c.fval = resval;
	    }
	    else if (TY_TYPE(ptype) == TY_DOUBLE) {
		/* shrink long doubles to double */
		FP_LDOUBLE resval;

		resval = op_xtodp(l->c.fval);
		if (errno) {
		    static const char mesg[] =
			"conversion to double is out of range"; /*ERROR*/
		    if (op_forinit)
			UERROR(gettxt(":393", mesg));
		    else
			WERROR(gettxt(":393", mesg));
		}
		/* If not for initializer, can only do this if value
		** is unchanged, or if we are generating fast floating
		** point code.
		*/
		if (!op_forinit && ieee_fp()) {
		    if (FP_CMPX(resval, l->c.fval) != FPEMU_CMP_EQUALTO)
			break;
		}
		l->c.fval = resval;
	    }
	} 
	else {	/* convert all else to long or unsigned long */
	    INTCON tmp;

	    if ((*(TY_ISUNSIGNED(ptype)
		? &op_xtoull : &op_xtoll))(&tmp, l->c.fval) != 0)
	    {
		static const char mesg[] =
		    "conversion of double to integral is out of range"; /*ERROR*/
		if (op_forinit)
		    UERROR(gettxt(":728", mesg));
		else {
		    WERROR(gettxt(":728", mesg));
		    break;
		}
	    }
	    /* Conversion of FP type to int always chops, so rounding
	    ** modes don't figure in here.  Can always do it.
	    */
	    l->op = ICON;
	    l->c.ival = tmp;
	    l->sid = ND_NOSYMBOL;
	}
	goto conv_paint;
    case CONV:
	/* These only apply to arithmetic types. */
	if (! (   TY_ISNUMTYPE(ptype)
	       && TY_ISNUMTYPE(ltype)
	       && TY_ISNUMTYPE(l->left->type)
	       )
	    )
	    break;

	/*  Let's look at three levels of the tree where the
	**  top two are CONV over CONV.  With the three levels
	**  we are concerned about the type sizes. In the
	**  following four cases the middle node may be tossed out:
	**
	**  	    CASE 1	CASE 2	    CASE 3		CASE 4
	** Level
	**   1	    Small	Medium	    Large		Large
	**   	      |		  |	      |			  |
	**   2	    Large	Large	    Medium[same sign]   Medium[not unsigned]
	**	      |		  |	      |			  |
	**   3	    Medium	Small	    Small[same sign]	Small[unsigned]
	**
	**  If we are generating fast floating point code, handle one
	**  additional case: the knothole from large to small to large
	**  where level 1 and level 3 have the same size.
	*/
	do {
	    T1WORD lltype = l->left->type;

	    if (op_tysize(ltype) <= op_tysize(lltype)) {
		/* knot-hole case */
		if (!(
			TY_ISFPTYPE(ptype)
		     && TY_ISFPTYPE(ltype)
		     && TY_ISFPTYPE(lltype)
		     && op_tysize(ptype) == op_tysize(lltype)
		     && !ieee_fp()
		   ))
			break;
	    }
	    else if (op_tysize(ptype) <= op_tysize(ltype)) {
		/* Cases 1, 2.  Floating point-ness of at least one
		** pair of adjacent operands must be the same.
		*/
		if (TY_ISFPTYPE(ptype) == TY_ISFPTYPE(ltype))
		    /*EMPTY*/ ;
		else if (TY_ISFPTYPE(ltype) == TY_ISFPTYPE(lltype))
		    /*EMPTY*/ ;
		else
		    break;
	    }
	    else {
		/* Check cases 3, 4. */
		/* Avoid trouble of (((double) (float)) (int))
		** where float cannot represent all ints.
		*/
		if (TY_ISFPTYPE(ltype) && !TY_ISFPTYPE(lltype)
		    	&& TY_SIZE(ltype) == TY_SIZE(lltype))
		    break;
		if (TY_ISUNSIGNED(ltype) == TY_ISUNSIGNED(lltype))
		    /*EMPTY*/ ;
		else if (!TY_ISUNSIGNED(ltype) && TY_ISUNSIGNED(lltype))
		    /*EMPTY*/ ;
		else
		    break;
	    }
	    /* All conditions met.  Toss out middle node, optimize. */
	    nfree(l);
	    p->left = l->left;
	    p = optimize(p);
	    /*CONSTANTCONDITION*/
	} while (0);
	ODEBUG(0, p, "result of CONV over CONV optimization");
	break;
    case STAR:
    {
	OFFSET adj;

	if (! (   TY_ISINTTYPE(ptype)
	       && TY_ISINTTYPE(ltype)
	       && op_tysize(ptype) <= op_tysize(ltype)
#ifdef SIGN_CONV
	       && TY_ISUNSIGNED(ptype) == TY_ISUNSIGNED(ltype)
#endif
	       )
	    )
	    return p;

	/* get adjustment in bytes of the two types */
	adj = cg_off_conv(NAME, 0, ltype, ptype);
	if (cg_off_is_err(NAME, adj))
	    return p;

	/* convert offset to bits */
	adj *= TY_SIZE(TY_CHAR);

	if (adj)
	    l->left = op_adjust(l->left, adj);
	goto conv_paint;
    }
    case NAME:
    {
	int space;
	OFFSET newoff;

	if (! (   TY_ISINTTYPE(ptype)
	       && TY_ISINTTYPE(ltype)
	       && op_tysize(ptype) <= op_tysize(ltype)
#ifdef SIGN_CONV
	       && TY_ISUNSIGNED(ptype) == TY_ISUNSIGNED(ltype)
#endif
	       )
	    )
	    return p;

	/* Can't deal with register variables that are in registers. */
	if (   (SY_FLAGS(l->sid) & SY_ISREG) != 0
	    && SY_REGNO(l->sid) != SY_NOREG
	    )
	    return p;

	/* Select correct address space in which to do address conversion. */
	switch( SY_CLASS(l->sid) ) {
	case SC_EXTERN:
	case SC_STATIC:		space = NAME;	break;
	case SC_AUTO:		space = VAUTO;	break;
	case SC_PARAM:		space = VPARAM; break;
	default:
	    cerror(gettxt(":280","strange NAME in op_conv()"));
	}
	
#ifdef	FAT_ACOMP			/* temporary */
	if (space != NAME)
	    return p;
#endif
	newoff = cg_off_conv(space, l->c.off, ltype, ptype);
	if (cg_off_is_err(space, newoff))
	    return p; 
	l->c.off = newoff;
	goto conv_paint;
    } /* end NAME case */
    }	/* end switch */
    return p;

conv_paint: ;
    if (l->op == ICON)
#ifdef LINT
    {	INTCON old_ival = l->c.ival;
#endif
	(void)tr_truncate(&l->c.ival, ptype);
#ifdef LINT
	/*
	** The value of this ICON is no longer the same; set FF_TRUNC so
	** lint will know that the constant was truncated.
	*/
	if (num_ucompare(&old_ival, &l->c.ival) != 0)
	    l->flags |= FF_TRUNC;
    }
#endif
    l->type = ptype;
    nfree(p);
    ODEBUG(0, l,"result after conv_paint");
    return l;
}

FP_LDOUBLE
op_xtofp(x)
FP_LDOUBLE x;
/* Convert long double x to a long double whose value has been 
** truncated to float precision.  Set errno if there was an error 
** while doing so.  Return the truncated-precision result.
*/
{
    errno = 0;
#ifndef FP_EMULATE
    if (setjmp(fpe_buf) != 0) {
	/* Set a value that can be used safely as a float later. */
	x = flt_0;
	goto fpe_err;			/* errno set */
    }
#endif
    save_fpefunc(op_fpe_handler);
    x = FP_XTOFP(x);
#ifndef FP_EMULATE
fpe_err:;
#endif
    save_fpefunc((void (*)()) 0);
    return x;
}

FP_LDOUBLE
op_xtodp(x)
FP_LDOUBLE x;
/* Convert long double x to a long double whose value has been 
** truncated to double precision.  Set errno if there was an error 
** while doing so.  Return the truncated-precision result.
*/
{
    errno = 0;
#ifndef FP_EMULATE
    if (setjmp(fpe_buf) != 0) {
	/* Set a value that can be used safely as a float later. */
	x = flt_0;
	goto fpe_err;			/* errno set */
    }
#endif
    save_fpefunc(op_fpe_handler);
    x = FP_XTODP(x);
#ifndef FP_EMULATE
fpe_err:;
#endif
    save_fpefunc((void (*)()) 0);
    return x;
}

static FP_LDOUBLE
op_ulltox(vp)
INTCON *vp;
/* Convert unsigned long long value to long double. */
{
    unsigned long val;

    if (num_toulong(vp, &val) == 0)
	return FP_ULTOX(val);
    return FP_ATOF(num_toudec(vp));
}

static FP_LDOUBLE
op_lltox(vp)
INTCON *vp;
/* Convert long long value to long double. */
{
    long val;

    if (num_toslong(vp, &val) == 0)
	return FP_LTOX(val);
    return FP_ATOF(num_tosdec(vp));
}

static int
op_hf2i(vp, x)
INTCON *vp;
FP_LDOUBLE x;
/* Handle the in-place rewrite of a hex floating string into
** an integer hex string that is then converted into an INTCON.
*/
{
    NumStrErr err;
    char *s, *p, *r;
    int exp;

    /* For now, assume that the string is one of the following:
    ** 0, -0, 0xH.HHHpDDD, -0xH.HHHpDDD, 0xH.HHHp-DDD, -0xH.HHHp-DDD
    ** in which "HHH" and "DDD" mean some hex and decimal digits.
    ** In other words, other than for zero, there will always be a
    ** 'x', a '.' (after exactly one hex digit) and a 'p'.
    */
    p = s = FP_XTOH(x);
    if (*p == '-')
	p++;
    if (*p++ != '0') {
bad:	cerror(gettxt(":1625","op_hf2i: bad hex floating string"));
    }
    if (*p == '\0') {
	*vp = num_0;
	return 0;
    }
    if (*p++ != 'x' || !isxdigit(*p++) || *p != '.')
	goto bad;
    p[0] = p[-1]; /* hex digit over '.' */
    p[-1] = 'x';
    p[-2] = '0';
    if (*s == '-')
	p[-3] = '-';
    s++;
    if ((r = strchr(p, 'p')) == 0)
	goto bad;
    exp = atoi(&r[1]) - 4 * (r - p) + 4;
    /* Trim off exponent and to-be-fractional digits. */
    while (exp <= -4) {
	exp += 4;
	r--;
	if (r[-1] == 'x') { /* ran out of hex digits */
	    *vp = num_0;
	    return 0;
	}
    }
    *r = '\0';
    p = s;
    if (*s == '-')
	s++;
    (void)num_fromstr(vp, s, strlen(s), &err);
    if (err.code != NUM_STRERR_NONE)
	goto bad;
    if (p != s) /* leading '-' */
	(void)num_negate(vp);
    return exp;
}

static int
op_xtoull(vp, x)
INTCON *vp;
FP_LDOUBLE x;
/* Convert long double to unsigned long long.  (-1,ULLONG_MAX+1) */
{
    int cmp;

    errno = 0;
    if ((cmp = FP_CMPX(x, flt_neg1)) == FPEMU_CMP_UNORDERED) {
	*vp = num_0;
	return EDOM;
    } else if (cmp != FPEMU_CMP_GREATERTHAN) {
	*vp = num_0;
	return ERANGE;
    } else if ((cmp = FP_CMPX(x, flt_ull_max_p1)) == FPEMU_CMP_UNORDERED) {
	*vp = num_0;
	return EDOM;
    } else if (cmp != FPEMU_CMP_LESSTHAN) {
	*vp = num_ull_max;
	return ERANGE;
    } else {
	if ((cmp = op_hf2i(vp, x)) < 0)
	    cmp = num_lrshift(vp, -cmp);
	else if (cmp > 0)
	    cmp = num_llshift(vp, cmp);
	if (cmp != 0)
	    return ERANGE;
	return 0;
    }
}

static int
op_xtoll(vp, x)
INTCON *vp;
FP_LDOUBLE x;
/* Convert long double to long long.  (LLONG_MIN-1,LLONG_MAX+1) */
{
    int cmp;

    errno = 0;
    if ((cmp = FP_CMPX(x, flt_sll_min_m1)) == FPEMU_CMP_UNORDERED) {
	*vp = num_0;
	return EDOM;
    } else if (cmp != FPEMU_CMP_GREATERTHAN) {
	*vp = num_sll_min;
	return ERANGE;
    } else if ((cmp = FP_CMPX(x, flt_sll_max_p1)) == FPEMU_CMP_UNORDERED) {
	*vp = num_0;
	return EDOM;
    } else if (cmp != FPEMU_CMP_LESSTHAN) {
	*vp = num_sll_max;
	return ERANGE;
    } else {
	if ((cmp = op_hf2i(vp, x)) < 0)
	    cmp = num_arshift(vp, -cmp);
	else if (cmp > 0)
	    cmp = num_alshift(vp, cmp);
	if (cmp != 0)
	    return ERANGE;
	return 0;
    }
}

static ND1 *
op_adjust(p, adj)
ND1 *p;
OFFSET adj;
{
    INTCON tmp;
    ND1 *q = p->right;

    /* Try to find a place to add BITOOR(adj) [byte offset]. */
    switch (p->op) {
    case MINUS:
	if (q->op != ICON)
	    break;
	adj = -adj;
	goto addicon;
    case PLUS:
	if (q->op != ICON)
	    break;
	goto addicon;
    case ICON:
	q = p;
    addicon:
	(void)num_fromslong(&tmp, BITOOR(adj));
	(void)num_sadd(&q->c.ival, &tmp);
	return p;
    }
    /* Insert an addition operation. */
    q = tr_newnode(PLUS);
    q->left = p;
    q->right = tr_smicon(BITOOR(adj));
    q->type = p->type;
    return q;
}

static ND1 *
op_left_con(p)
ND1 *p;
/* The left side of p is an ICON or FCON.  Do any reasonable
** optimizations thereupon.  Must maintain fixed condition
** that left side of tree has been optimized.  Don't change
** multiplies to shifts yet:  if there's overflow, we would
** show the wrong op.
*/
{
    ND1 *l = p->left;
    int changed = 0;

    if (optype(p->op) == BITYPE && TY_ISVOLATILE(p->right->type))
	return p;

    /* Skim off FCON's.
    ** If op is ANDAND, OROR, QUEST, turn the FCON into an ICON.
    ** When not IEEE-correct, trim off 0 PLUS/MINUS/MUL/DIV and +/-1 MUL.
    ** Otherwise quit.
    */
    if (l->op == FCON) {
	int op = p->op;
	switch (op) {
#ifndef LINT
	case CBRANCH:
	case ANDAND:
	case OROR:
	case QUEST:
	    l->c.ival = *(FP_ISZERO(l->c.fval) ? &num_0 : &num_1);
	    l->op = ICON;
	    l->type = TY_INT;
	    l->sid = ND_NOSYMBOL;
	    break;
#endif
	case PLUS:
	case MINUS:
	    if (!ieee_fp() && FP_ISZERO(l->c.fval)) {
	rightonly:
		nfree(l);
		nfree(p);
		p = p->right;
		if (op == MINUS) {
		    if (optype(p->op) != LTYPE)
			/* We are going to make p the left hand side of
			** the returned node, so we need to optimize it.
			*/
			p = optimize(p);
		    return tr_generic(UNARY MINUS, p, p->type);
		}
		if (optype(p->op) != LTYPE)
		    p->left = optimize(p->left);
	    }
	    return p;
	case MUL:
	    if (ieee_fp())
		return p;
	    if (FP_ISZERO(l->c.fval))
		goto commaswap;
	    if (FP_CMPX(l->c.fval, flt_1) == FPEMU_CMP_EQUALTO)
		goto rightonly;
	    if (FP_CMPX(l->c.fval, flt_neg1) == FPEMU_CMP_EQUALTO) {
		op = MINUS;
		goto rightonly;
	    }
	    return p;
	case DIV:
	    if (!ieee_fp() && FP_ISZERO(l->c.fval))
		goto commaswap;
	    /*FALLTHROUGH*/
	default:
	    return p;
	}
    }

    if (num_ucompare(&l->c.ival, &num_0) == 0) {
	/* Since weak symbols CAN resolve to an address of zero,
	** do not assume that a named ICON w/value of zero is "true".
	*/
	if (l->sid == ND_NOSYMBOL) switch (p->op) {
	/* For these ops, left side can be discarded. */
	case PLUS:
	case OR:
	case ER:
	    nfree(l);
	    nfree(p);
	    p = p->right;
	    changed = 1;
	    break;

#ifndef	LINT				/* LINT wants to see these */
	/* For these ops, result is zero. */
	case ANDAND:
	case LS:
	case RS:
	    op_t1free(p->right);
	    nfree(p);
	    p = l;
	    p->type = TY_INT;		/* in case it was pointer */
	    break;
#endif
	
	/* For these ops, result is zero, but we may need to do right
	** side for side effects.  Build ,OP.
	*/
	case MUL:
	case AND:
	commaswap:
	    p->op = COMOP;
	    p->left = p->right;
	    p->right = l;
	    changed = 1;
	    break;

	/* 0-x => -x */
	case MINUS:
	    p->op = UNARY MINUS;
	    nfree(l);
	    p->left = p->right;
	    p->right = ND1NIL;
	    changed = 1;
	    break;
	
	/* 0 in ?: => right side */
	case QUEST:
	{
	    ND1 *colon = p->right;

	    nfree(l);
	    nfree(p);
	    p = colon->right;
	    p->type = colon->type;
	    op_t1free(colon->left);
	    nfree(colon);
	    changed = 1;
	    break;
	}

	/* Turn conditional branch into absolute branch. */
	case CBRANCH:
	    p->op = JUMP;
	    nfree(l);
	    break;
	} /* end switch for zero cases */
    } /* end zero constant cases */
    else {
	/* This ICON has a nonzero value, so even if it's a
	** named ICON for a weak symbol that isn't defined,
	** the resulting address will be nonzero.
	*/
	switch (p->op) {
	/* Result is 1. */
#ifndef	LINT			/* LINT wants to see this */
	case OROR:
	    op_t1free(p->right);
	    nfree(p);
	    p = l;
	    /*FALLTHROUGH*/
#endif
	case ANDAND:	/* Change (possible) address constant to pure ICON. */
	case NOT:
	    l->c.ival = num_1;
	    l->type = TY_INT;		/* in case it was pointer */
	    l->sid = ND_NOSYMBOL;
	    break;
	/* Choose left side of ?: */
	case QUEST:
	{
	    ND1 *colon = p->right;

	    nfree(l);
	    nfree(p);
	    p = colon->left;
	    p->type = colon->type;
	    nfree(colon);
	    op_t1free(colon->right);
	    changed = 1;
	    break;
	}
#ifndef	LINT			/* LINT wants to see this */
	/* CBRANCH becomes NOP if operand is non-0. Assume an
	** ICON in this context will behave like a no-op.
	** (Don't use that op code to accommodate SPARC.)
	*/
	case CBRANCH:
	    nfree(l);
	    p->op = ICON;
	    p->c.ival = num_0;
	    p->sid = ND_NOSYMBOL;
	    break;
#endif
	} /* end switch on non-zero cases */
    } /* end non-zero cases */

    /* if p is a new node, we must keep the left subtree optimized */
    while (changed && optype(p->op) != LTYPE) {
	changed = 0;
	p->left = optimize(p->left);
	if (p->op == COMOP && (p->left->op == ICON || p->left->op == FCON)) {
	    /* Discard ,OP's that arise in constant expressions. */
	    nfree(p->left);
	    nfree(p);
	    p = p->right;
	    changed = 1;
	}
	if (p->op == QUEST && (p->left->op == ICON || p->left->op == FCON)) {
	    p = op_left_con(p);		/* Does not get done in optimize() */
	}
    }
    return p;
}

static ND1 *
op_right_con(p)
ND1 *p;
/* Optimize stuff with a constant on the right side
** and return modified tree.  Don't change multiplies
** to shifts yet:  if there's overflow, we show the
** wrong op.
** Assume the left side is fully optimized already.
*/
{
    ND1 *l = p->left;
    ND1 *r = p->right;
    T1WORD objtype;

    if (TY_ISVOLATILE(l->type))
	return p;

    /* Handle FCON's first. */
    if (r->op == FCON) {
	switch (p->op) {
	case ANDAND:
	case OROR:
	    r->c.ival = *(FP_ISZERO(r->c.fval) ? &num_0 : &num_1);
	    r->op = ICON;
	    r->type = TY_INT;
	    r->sid = ND_NOSYMBOL;
	    break;
	case PLUS:
	case ASG PLUS:
	case MINUS:
	case ASG MINUS:
	    if (!ieee_fp() && FP_ISZERO(r->c.fval)) {
	leftonly:
		if (asgop(p->op) && l->op == CONV) {
		    p->left = l->left;
		    nfree(l);
		}
		nfree(r);
		nfree(p);
		p = p->left;
	    }
	    break;
	case MUL:
	case ASG MUL:
	    if (ieee_fp())
		break;
	    if (FP_ISZERO(r->c.fval)) {
		if (asgop(p->op)) {
		    p->op = ASSIGN;
		    if (l->op == CONV) {
			p->left = l->left;
			nfree(l);
		    }
		    break;
	   	}
	        op_t1free(l);
		nfree(p);
		p = r;
		break;
	    }
	    if (FP_CMPX(r->c.fval, flt_1) == FPEMU_CMP_EQUALTO)
		goto leftonly;
	    if (p->op == MUL) {
	negleft:
		if (FP_CMPX(r->c.fval, flt_neg1) == FPEMU_CMP_EQUALTO) {
		    nfree(r);
		    p->op = UNARY MINUS;
		    p->right = ND1NIL;
		    p = optimize(p);
	        }
	    }
	    break;
	case DIV:
	case ASG DIV:
	    /* Do not complain about floating division by zero. */
	    if (ieee_fp())
		break;
	    if (FP_CMPX(r->c.fval, flt_1) == FPEMU_CMP_EQUALTO)
		goto leftonly;
	    if (p->op ==DIV)
		goto negleft;
	    break;
	}
	return p;
    }

    /* r->op == ICON. */
    if (r->sid == ND_NOSYMBOL && num_ucompare(&r->c.ival, &num_0) == 0) {
	switch (p->op) {
	/* For these, zero is an identity. */
	case MINUS:
	case ASG MINUS:
	    /* For pointer subtraction particularly, result type is integral;
	    ** must change type of left operand.
	    */
	    l->type = p->type;
	    /*FALLTHRU*/
	case PLUS:
	case ASG PLUS:
	case OR:
	case ASG OR:
	case ER:
	case ASG ER:
	case LS:
	case ASG LS:
	case RS:
	case ASG RS:
	    /* If ASG op, remove a CONV on the left. */
	    if (asgop(p->op) && l->op == CONV) {
		p->left = l->left;
		nfree(l);
	    }
	    nfree(r);
	    nfree(p);
	    p = p->left;
	    break;
	
	/* Division, modulus by zero. */
	case MOD:
	case ASG MOD:
	case DIV:
	case ASG DIV:
	{
	    /* Must leave the tree alone if executable, because the code
	    ** may never be reached.  (ANSI requires the module to compile
	    ** otherwise.)
	    */
	    const char *mesg = (p->op == DIV || p->op == ASG DIV)
		? gettxt(":729", "division by 0")	/*ERROR*/
		: gettxt(":730", "modulus by 0");	/*ERROR*/

	    if (op_forinit)
		UERROR(mesg);
	    else
		WERROR(mesg);
	    break;
	}

	/* For these, must do left side for effects, but result is zero.
	** Beware volatile type on left:  can't discard operation.
	** If we know the left side is a constant, we can discard it.
	*/
	case AND:
	case MUL:
	    if (l->op == ICON) {
		nfree(l);
		nfree(p);
		p = r;
	    }
	    else
		p->op = COMOP;
	    break;

	/* Result is zero.  Remove conversion on left of asgop,
	** if any, change ICON type.
	*/
	case ASG MUL:
	case ASG AND:
	    if (asgop(p->op) && l->op == CONV) {
		p->left = l->left;
		nfree(l);
	    }
	    p->op = ASSIGN;
	    p->right->type = p->type;
	    break;
	}
    } else if (r->sid == ND_NOSYMBOL) {
	int pow2;

	/* Non-zero constants on right. */
	switch (p->op) {
	case LS:
	case ASG LS:
	case RS:
	case ASG RS:
	{
	    INTCON tmp;
	    objtype = p->type;

	    /* Check for negative shift count and count too big. */
	    if (asgop(p->op) && l->op == CONV)
		objtype = l->left->type;

	    (void)num_fromulong(&tmp, (unsigned long)TY_SIZE(objtype));
	    /* 2's complement means this catches negative values, too. */
	    if (num_ucompare(&r->c.ival, &tmp) >= 0) {
		WERROR(gettxt(":1626","shift count negative or too big: %s %s"),
		    opst[p->op], num_tosdec(&r->c.ival));
		if (num_scompare(&r->c.ival, &num_0) < 0)
		    r->c.ival = num_0;	/* avoid cascading messages */
	    }
	    break;
	}
	case ASG MUL:
	case ASG DIV:
	case MUL:
	case DIV:
	    if (num_ucompare(&r->c.ival, &num_1) == 0) {
		nfree(r);
		if (asgop(p->op) && l->op == CONV) {
		    p->left = l->left;
		    nfree(l);
		}
		nfree(p);
		p = p->left;
		goto done;
	    }
	    break;

	/* Mod of 1 gives result of 0, since quotient is dividend.
	** Rewrite  x %= 1 as x = 0.  Rewrite x % 1 as x,0 to pick
	** up side effects if x is not a constant.
	** For signed mod, -1 behaves the same way.  (The remainder
	** is always 0.)
	*/
	case MOD:
	case ASG MOD:
	    if (num_ucompare(&r->c.ival, &num_1) == 0
		|| TY_ISSIGNED(p->type)
		&& num_ucompare(&r->c.ival, &num_neg1) == 0)
	    {
		r->c.ival = num_0;
		if (p->op == MOD) {
		    if (l->op == ICON) {
			nfree(l);
			nfree(p);
			p = r;
		    }
		    else
			p->op = COMOP;
		}
		else {
		    if (l->op == CONV) {
			p->left = l->left;
			nfree(l);
		    }
		    p->op = ASSIGN;
		    p->right->type = p->type;
		}
		goto done;
	    }
	    break;
	}

	/* Do other conversions if right side is power of 2. */
	if ((pow2 = op_ispow2(&r->c.ival)) >= 0) {
	    objtype = l->type;
	    if (asgop(p->op) && l->op == CONV)
		objtype = l->left->type;

	    switch (p->op) {
	    /* Turn unsigned divisions into shifts. */
	    case DIV:
	    case ASG DIV:
		if (TY_ISUNSIGNED(objtype)) {
		    /* Make a right shift. */
		    p->op += (RS - DIV);
		    (void)num_fromslong(&r->c.ival, (long)pow2);
		    if (l->type != objtype) {
			/* Delete now inapproriate CONV to match the
			** type rules expected for shift ops.
			*/
			p->left = l->left;
			nfree(l);
		    }
		}
		break;
	    /* Turn modulus into AND of value-1. */
	    case MOD:
	    case ASG MOD:
		if (TY_ISUNSIGNED(objtype)) {
		    p->op += (AND - MOD);
		    (void)num_sadd(&r->c.ival, &num_neg1);
		}
		break;
	    }
	}
    } else if (num_ucompare(&r->c.ival, &num_0) != 0) {
	/* Address constant on right with nonzero value.
	** Even if it's a weak symbol reference that isn't
	** found, the resulting address will be nonzero.
	*/
	switch (p->op) {
	case ANDAND:
	case OROR:
	    /* Turn these into vanilla integer constants with value 1. */
	    r->sid = ND_NOSYMBOL;
	    r->type = TY_INT;
	    r->c.ival = num_1;
	    break;
	}
    }
done:;
    return p;
}

static ND1 *
op_iconfold(p)
ND1 *p;
/* Perform constant folding on integer constants (ICONs).
** p contains the operator, with left and right (when binary)
** ICON operand leaf nodes.
*/
{
    static const char mesg[] = "integer overflow detected: op \"%s\""; /*ERROR*/
    ND1 *l = p->left;
    ND1 *r = p->right;
    INTCON tmp, rem;
    int cmp, shift, warned;

    ODEBUG(0, p,"In op_iconfold(), attempt to fold tree");

    warned = 0;
    if (optype(p->op) == UTYPE) {
	if (l->sid != ND_NOSYMBOL)
	    return p;
	/* Unary op above pure ICON. */
	switch (p->op) {
	default:
	    return p;
	case UNARY MINUS:
#ifdef TRAP_OVERFLOW
	    if (TY_ISSIGNED(l->type) && !op_forinit) {
		tmp = l->c.ival;
		if (num_negate(&tmp) || tr_truncate(&tmp, p->type))
		    return p;
		l->c.ival = tmp;
		break;
	    }
#endif
	    if (num_negate(&l->c.ival) && TY_ISSIGNED(l->type)) {
		if (!op_foramigo)
		    WERROR(gettxt(":733", mesg), opst[p->op]);
		warned = 1;
	    }
	    break;
	case NOT:
	    num_not(&l->c.ival);
	    break;
	case COMPL:
	    num_complement(&l->c.ival);
	    break;
	case UPLUS:
	    break;
	}
    } else {
	/* Eliminate nonpure ICONs unless it's simple pointer arithmetic. */
	if (r->sid != ND_NOSYMBOL) {
	    if (l->sid != ND_NOSYMBOL) {
		/* Okay only when subtracting the same symbol. */
		if (l->sid != r->sid || p->op != MINUS)
		    return p;
	    } else if (p->op != PLUS)
		return p;
	} else if (l->sid != ND_NOSYMBOL) {
	    if (p->op != PLUS && p->op != MINUS)
		return p;
	}
	switch (p->op) {
	default:
	    return p;
	case LT: case NLT:
	case LE: case NLE:
	case GT: case NGT:
	case GE: case NGE:
	case LG: case NLG:
	    cmp = num_scompare(&l->c.ival, &r->c.ival);
	    goto postcmp;
	case EQ: case NE:
	case ULT: case UNLT:
	case ULE: case UNLE:
	case UGT: case UNGT:
	case UGE: case UNGE:
	case ULG: case UNLG:
	    cmp = num_ucompare(&l->c.ival, &r->c.ival);
	postcmp:
	    switch (p->op) {
	    case EQ: case NLG: case UNLG:
		if (cmp == 0)
		    goto true;
		break;
	    case NE: case LG: case ULG:
		if (cmp != 0)
		    goto true;
		break;
	    case LT: case ULT: case NGE: case UNGE:
		if (cmp < 0)
		    goto true;
		break;
	    case GE: case UGE: case NLT: case UNLT:
		if (cmp >= 0)
		    goto true;
		break;
	    case GT: case UGT: case NLE: case UNLE:
		if (cmp > 0)
		    goto true;
		break;
	    case LE: case ULE: case NGT: case UNGT:
		if (cmp <= 0)
		    goto true;
		break;
	    }
	    /*FALLTHROUGH*/
	case NLGE: case UNLGE:
	false:
	    l->c.ival = num_0;
	    break;
	case LGE: case ULGE:
	true:
	    l->c.ival = num_1;
	    break;
	case ANDAND:
	    if (num_ucompare(&l->c.ival, &num_0) == 0)
		goto false;
	cmpright:
	    if (num_ucompare(&r->c.ival, &num_0) != 0)
		goto true;
	    goto false;
	case OROR:
	    if (num_ucompare(&l->c.ival, &num_0) != 0)
		goto true;
	    goto cmpright;
	case AND:
	    num_and(&l->c.ival, &r->c.ival);
	    break;
	case OR:
	    num_or(&l->c.ival, &r->c.ival);
	    break;
	case ER:
	    num_xor(&l->c.ival, &r->c.ival);
	    break;
	case LS:
	    cmp = num_tosint(&r->c.ival, &shift);
#ifdef TRAP_OVERFLOW
	    if (!op_forinit && (cmp || shift < 0 || shift >= TY_SIZE(l->type)))
		return p;
#endif
	    if (TY_ISUNSIGNED(l->type)) {
		(void)num_llshift(&l->c.ival, shift);
	    } else {
#ifdef TRAP_OVERFLOW
		tmp = l->c.ival;
		cmp = num_alshift(&tmp, shift);
		cmp |= tr_truncate(&tmp, p->type);
		if (cmp && !op_forinit)
		    return p;
		l->c.ival = tmp;
#else
		cmp |= num_alshift(&l->c.ival, shift);
#endif
		if (cmp)
		    goto oflow;
	    }
	    break;
	case RS:
	    cmp = num_tosint(&r->c.ival, &shift);
#ifdef TRAP_OVERFLOW
	    if (!op_forinit && (cmp || shift < 0 || shift >= TY_SIZE(l->type)))
		return p;
#endif
#ifdef C_SIGNED_RS
	    if (TY_ISUNSIGNED(l->type))
		(void)num_lrshift(&l->c.ival, shift);
	    else {
		cmp |= num_arshift(&l->c.ival, shift);
		if (cmp)
		    goto oflow;
	    }
#else
	    cmp |= num_lrshift(&l->c.ival, shift);
	    if (cmp && TY_ISSIGNED(l->type))
		goto oflow;
#endif
	    break;
	case PLUS:
#ifdef TRAP_OVERFLOW
	    if (TY_ISSIGNED(l->type) && !op_forinit) {
		tmp = l->c.ival;
		if (num_sadd(&tmp, &r->c.ival) || tr_truncate(&tmp, p->type))
		    return p;
		l->c.ival = tmp;
		break;
	    }
#endif
	    if (r->sid != ND_NOSYMBOL)	/* l->sid == ND_NOSYMBOL */
		l->sid = r->sid;
	    /* resulting bits are the same for signed and unsigned. */
	    if (num_sadd(&l->c.ival, &r->c.ival) && TY_ISSIGNED(l->type)) {
	oflow:
		if (!op_foramigo)
		    WERROR(gettxt(":733", mesg), opst[p->op]);
		warned = 1;
	    }
	    break;
	case MINUS:
#ifdef TRAP_OVERFLOW
	    if (TY_ISSIGNED(l->type) && !op_forinit) {
		tmp = l->c.ival;
		if (num_ssubtract(&tmp, &r->c.ival) || tr_truncate(&tmp, p->type))
		    return p;
		l->c.ival = tmp;
		break;
	    }
#endif
	    if (l->sid == r->sid) /* if not now ND_NOSYMBOL, result is ICON */
		l->sid = ND_NOSYMBOL;
	    /* resulting bits are the same for signed and unsigned. */
	    if (num_ssubtract(&l->c.ival, &r->c.ival) && TY_ISSIGNED(l->type))
		goto oflow;
	    break;
	case MUL:
#ifdef TRAP_OVERFLOW
	    if (TY_ISSIGNED(l->type) && !op_forinit) {
		tmp = l->c.ival;
		if (num_smultiply(&tmp, &r->c.ival) || tr_truncate(&tmp, p->type))
		    return p;
		l->c.ival = tmp;
		break;
	    }
#endif
	    if (TY_ISUNSIGNED(l->type))
		(void)num_umultiply(&l->c.ival, &r->c.ival);
	    else if (num_smultiply(&l->c.ival, &r->c.ival))
		goto oflow;
	    break;
	case DIV:
	case MOD:
	    if (TY_ISUNSIGNED(l->type)) {
		/* Only overflow is zero rhs.  Already diagnosed. */
		if (num_udivrem(&tmp, &rem, &l->c.ival, &r->c.ival)) {
		    if (!op_forinit)
			return p;
		    l->c.ival = num_0; /* treat as zero */
		    break;
		}
	    } else {
		/* 2's comp. QUOTIENT can also overflow for SMIN/(-1). */
		if (num_sdivrem(&tmp, &rem, &l->c.ival, &r->c.ival)) {
		    /* Overflow due to zero rhs was already diagnosed. */
		    if (num_ucompare(&r->c.ival, &num_0) == 0) {
			if (!op_forinit)
			    return p;
			l->c.ival = num_0; /* treat as zero */
		    } else if (p->op == MOD) {
			l->c.ival = num_0;
#ifdef TRAP_OVERFLOW
		    } else if (!op_forinit) {
			return p;
#endif
		    } else {
			l->c.ival = num_sll_min;
			goto oflow;
		    }
		    break;
		}
	    }
	    l->c.ival = *(p->op == MOD ? &rem : &tmp);
	    break;
	}
	nfree(r);
    }
    l->type = p->type; /* in case of pointer subtraction */
    if (tr_truncate(&l->c.ival, l->type)
	    && !warned && !op_foramigo && TY_ISSIGNED(l->type))
	WERROR(gettxt(":733", mesg), opst[p->op]);
    nfree(p);
    ODEBUG(0, l, "Successful fold, new tree is:");
    return l;
}

static ND1 *
op_fconfold(p)
ND1 *p;
/* Perform constant folding on floating point constants.
** p contains the operator, with left and right (when binary)
** FCON operand leaf nodes.
 */
{
    ND1 *l = p->left;
    FP_LDOUBLE lfval; 
    static const char mesg[] =
	"floating-point constant calculation out of range: op \"%s\""; /*ERROR*/
    int fpresult = 0;			/* 1 if op has FP result */

    lfval = l->c.fval;
    errno = 0;		/* check float/double range error */
    if (optype(p->op) == UTYPE) {	/* do we have a unary operand? */
	switch (p->op) {
	case UNARY MINUS:
	    l->c.fval = FP_NEG(lfval);
	    break;
	case NOT:
	    (void)num_fromslong(&l->c.ival, (long)FP_ISZERO(lfval));
	    /*FALLTHRU*/
	case UPLUS:
	    break;
	default:
	    return p;
	}
    } else {	/* it is a binary operand */
	FP_LDOUBLE rfval;

	rfval = p->right->c.fval;
#ifndef FP_EMULATE
	if (setjmp(fpe_buf) != 0)
	    goto fpe_stuff1;
#endif
	save_fpefunc(op_fpe_handler);

	switch (p->op) {
	case DIV:
	    fpresult = 1;		/* division gets FP result */
#ifdef FP_EMULATE
	    lfval = FP_DIVIDE(lfval, rfval);
#else /*!FP_EMULATE*/
	    if (FP_ISZERO(rfval)) {
		if (!op_forinit && ieee_fp())
		    goto fp_out;
#ifdef HUGE_VAL
		lfval = HUGE_VAL;
#else
		lfval = HUGE;
#endif
	    } else if (FP_ISZERO(lfval)) {
		if (!op_forinit && ieee_fp())
		    goto fp_out;
		fpresult = 0; /* cause lhs to be the result */
	    } else
		lfval = FP_DIVIDE(lfval, rfval);
#endif /*FP_EMULATE*/
	    break;
	case PLUS:	fpresult = 1; lfval = FP_PLUS(lfval, rfval); break;
	case MINUS:	fpresult = 1; lfval = FP_MINUS(lfval, rfval); break;
	case MUL:	fpresult = 1; lfval = FP_TIMES(lfval, rfval); break;
	/* ANDAND and OROR handled in op_right_con() */
	case EQ: case NE:
	case LT: case NLT:
	case LE: case NLE:
	case GT: case NGT:
	case GE: case NGE:
	case LG: case NLG:
	case LGE: case NLGE:
	    {	int cmp = FP_CMPX(lfval, rfval);
		INTCON *ans = &num_0;

		switch (p->op) {
		true:
		    ans = &num_1;
		    break;
		case EQ:
		    if (cmp == FPEMU_CMP_EQUALTO)
			goto true;
		    break;
		case NE:
		    if (cmp != FPEMU_CMP_EQUALTO)
			goto true;
		    break;
		case LT:
		    if (cmp == FPEMU_CMP_LESSTHAN)
			goto true;
		    break;
		case NLT:
		    if (cmp != FPEMU_CMP_LESSTHAN)
			goto true;
		    break;
		case LE:
		    if (cmp == FPEMU_CMP_LESSTHAN || cmp == FPEMU_CMP_EQUALTO)
			goto true;
		    break;
		case NLE:
		    if (cmp != FPEMU_CMP_LESSTHAN && cmp != FPEMU_CMP_EQUALTO)
			goto true;
		    break;
		case GT:
		    if (cmp == FPEMU_CMP_GREATERTHAN)
			goto true;
		    break;
		case NGT:
		    if (cmp != FPEMU_CMP_GREATERTHAN)
			goto true;
		    break;
		case GE:
		    if (cmp == FPEMU_CMP_GREATERTHAN || cmp == FPEMU_CMP_EQUALTO)
			goto true;
		    break;
		case NGE:
		    if (cmp != FPEMU_CMP_GREATERTHAN && cmp != FPEMU_CMP_EQUALTO)
			goto true;
		    break;
		case LG:
		    if (cmp == FPEMU_CMP_LESSTHAN || cmp == FPEMU_CMP_GREATERTHAN)
			goto true;
		    break;
		case NLG:
		    if (cmp != FPEMU_CMP_LESSTHAN && cmp != FPEMU_CMP_GREATERTHAN)
			goto true;
		    break;
		case LGE:
		    if (cmp != FPEMU_CMP_UNORDERED)
			goto true;
		    break;
		case NLGE:
		    if (cmp == FPEMU_CMP_UNORDERED)
			goto true;
		    break;
		}
		/* Can't always fold floating comparisons at compile time.
		** If not forced to fold and complying to IEEE and the
		** result of the compare was "unordered", we may have to
		** wait until runtime so that an exception can be raised.
		*/
		if (cmp == FPEMU_CMP_UNORDERED
			&& !(p->flags & FF_NOEXCEPT)
			&& !op_forinit && ieee_fp())
		{
			goto fp_out; /* can't fold */
		}
		l->c.ival = *ans;
		break;
	    }
	default:
	fp_out:
	    save_fpefunc((void(*)()) 0);	/* unset trap handler */
	    return (p);
	}
    }

#ifndef FP_EMULATE
fpe_stuff1: ;
#endif
    /* Could only get error on + - * / */
    if (errno) {
	if (op_foramigo)
	    /*EMPTY*/;
	if (op_forinit)
	    UERROR(gettxt(":731", mesg), opst[p->op]);
	else
	    WERROR(gettxt(":731", mesg), opst[p->op]);
    }
    save_fpefunc((void(*)()) 0);

    if (logop(p->op)){		/* clean out FCON info in logical op */
	l->op = ICON;
	l->type = TY_INT;
	l->sid = ND_NOSYMBOL;
    }
    else if (fpresult) {
	if ( TY_TYPE(p->type) == TY_FLOAT ){
	    lfval = op_xtofp(lfval);	/* truncate to float */
	    if (errno)
		WERROR(gettxt(":392","conversion to float is out of range"));
	}
	else if ( TY_TYPE(p->type) == TY_DOUBLE ){
	    lfval = op_xtodp(lfval);	/* truncate to double */
	    if (errno)
		WERROR(gettxt(":393","conversion to double is out of range"));
	}
    }

    /* Don't actually fold the floating point result unless it's for
    ** an initializer or we are generating fast floating point code.  
    ** Otherwise, free up nodes that are no longer needed.
    */
    if (fpresult && (op_forinit || !ieee_fp()))
	l->c.fval = lfval;
    if (!fpresult || op_forinit || !ieee_fp()) {
	if (optype(p->op) == BITYPE)
	    nfree(p->right);
	nfree(p);
	p = l;
    }
    return p;
}

static ND1 *
op_uand(p, t)
ND1 *p;
T1WORD t;
/* build a UNARY AND node over p, but enforce optim tree-rewrite rules.
** Some operators require the UNARY AND to be distributed within.
*/
{
    ND1 * colonop;

    switch( p->op ) {
    case QUEST:
        /* Put & over each side of :, change type of : */
	colonop = p->right;		/* assume this is the : */
	colonop->left = op_uand(colonop->left, t);
	colonop->right = op_uand(colonop->right, t);
	colonop->type = t;
	p->type = t;
	break;
    case COMOP:
	/* Put & over right side. */
	p->right = op_uand(p->right, t);
	p->type = t;
	break;
    case NAME:
	/* Treat as if address taken (which it was). */
	if (p->sid != ND_NOSYMBOL)
	    SY_FLAGS(p->sid) |= SY_UAND;
	/* FALLTHRU */
    default:
	p = optimize( tr_generic(UNARY AND, p, t) );
    }
    return p;
}

static ND1 *
op_call(p)
ND1 *p;
/* Try to recognize calls to particular built-in functions and
** convert to pure trees.  We know the left side of the callop is
** an ICON for which a builtin function use appeared.
** Look for:
**	strcpy(X, "string") -> BMOVE of "string" if X has no side effects
**	strncpy(X, "string", sizeof("string")+1) -> (same as strcpy)
**	strlen("string") -> ICON of appropriate size
*/
{
    static int first = 1;
    static char * n_bi_strcpy;
    static char * n_bi_strncpy;
    static char * n_bi_strlen;
    ND1 * tp = p->right;
    char * s = SY_NAME(p->left->sid);

    if (first) {
	first = 0;
	n_bi_strcpy	=	st_nlookup("strcpy",  sizeof("strcpy"));
	n_bi_strncpy	=	st_nlookup("strncpy", sizeof("strncpy"));
	n_bi_strlen	=	st_nlookup("strlen",  sizeof("strlen"));
    }

    if (p->op == UNARY CALL)
	/* EMPTY */;			/* only handle w/arg functions */
    /* strlen() case */
    if      (s == n_bi_strlen) {
	/* Must have single FUNARG node whose operand is STRING. */
	if (   tp->op == FUNARG
	    && (tp = tp->left)->op == STRING
	    && (tp->sid & TR_ST_WIDE) == 0
	) {
	    ODEBUG(0, p, "strlen(): before rewrite");
	    nfree(p->left);
	    nfree(p->right);
	    nfree(tp);
	    p->op = ICON;
	    /* Remember to look for NUL in string, in case of "ab\0c" */
	    (void)num_fromulong(&p->c.ival, (unsigned long)strlen(tp->string));
	    p->sid = ND_NOSYMBOL;
	    ODEBUG(0, p, "strlen(): after rewrite");
	}
    }
    /* strcpy() case */
    else if (s == n_bi_strcpy) {
	/* First argument must have no side-effects.
	** Second argument must be string.
	*/
	if (   tp->op == CM
	    && FUNARGOP(tp->left->op)
	    && (tp->left->flags & FF_SEFF) == 0
	    && (tp = tp->right)->op == FUNARG
	    && (tp = tp->left)->op == STRING
	    && (tp->sid & TR_ST_WIDE) == 0
	) {
	    /* Convert the tree as follows:
	    **
	    **		CALL				,OP
	    **	       /    \			       /   \
	    **       ICON    ,			     BMOVE  X
	    **		   /   \		     /   \
	    **		  ARG  ARG		    ICON  ,
	    **		  /     \			 / \
	    **		 X	STRING			X  STRING
	    **
	    ** Specific node mappings:
	    **		CALL	-> BMOVE
	    **		ICON	-> ICON
	    **		,	-> ,
	    **		X	-> X
	    **		STRING	-> STRING
	    **		ARG	-> ,OP
	    **		ARG	-> freed
	    */
	    ODEBUG(0, p, "strcpy(): before rewrite");
	    /* Take care of ICON. */
	    (void)num_fromulong(&p->left->c.ival, 1ul + strlen(tp->string));
	    tp = p->left;
	    tp->type = TY_INT;
	    tp->sid = ND_NOSYMBOL;

	    p->op = BMOVE;
	    p->type = TY_CHAR;

	    /* Adjust left side of CM, build ,OP */
	    tp = p->right->left;	/* first ARG node, becomes ,OP */
	    p->right->left = tp->left;
	    tp->right = tr_copy(tp->left);
	    tp->left = p;
	    tp->op = COMOP;
	    tp->type = tp->right->type;

	    /* Get rid of second ARG node. */
	    nfree(p->right->right);
	    p->right->right = p->right->right->left;

	    p = tp;
	    ODEBUG(0, p, "strcpy(): after rewrite");
	}
    }
    else if (s == n_bi_strncpy) {
	ND1 * tp2;
	ND1 * stringnode;
	unsigned long val;

	/* First argument must have no side-effects.
	** Second argument must be string.
	** Third must be integer constant.
	** The string to be copied must be at least as long as
	** the constant length requests.  (We can't force zero
	** filling.)
	*/
	if (   tp->op == CM
	    && (tp2 = tp->left)->op == CM
	    && FUNARGOP(tp2->left->op)
	    && (tp2->left->flags & FF_SEFF) == 0
	    && (stringnode = tp2->right)->op == FUNARG
	    && (stringnode = stringnode->left)->op == STRING
	    && (stringnode->sid & TR_ST_WIDE) == 0
	    && FUNARGOP(tp->right->op)
	    && (tp2 = tp->right->left)->op == ICON
	    && tp2->sid == ND_NOSYMBOL
	    && num_toulong(&tp2->c.ival, &val) == 0
	    && val <= 1 + strlen(stringnode->string)
	) {
	    /* Convert the tree as follows:
	    **
	    **		CALL				,OP
	    **	       /    \			       /   \
	    **       ICON1   ,1 		     BMOVE  X
	    **		   /   \		     /   \
	    **		  ,2   ARG3		    ICON  ,
	    **		 / \     \			 / \
	    **	       ARG1 ARG2 ICON2			X  STRING
	    **		/    \
	    **	       X   STRING
	    **
	    ** Specific node mappings:
	    **		CALL	-> BMOVE
	    **		ICON2	-> ICON
	    **		,2	-> ,
	    **		,1	-> ,OP
	    **		X	-> X
	    **		STRING	-> STRING
	    **		ARG1,2,3 -> freed
	    **		ICON1	-> freed
	    */

	    ODEBUG(0, p, "strncpy(): before rewrite");

	    /* Right now:
	    ** tp	points at ,1
	    ** tp2	points at ICON2
	    ** stringnode points at STRING
	    ** p	points at CALL
	    **
	    ** Take care of CALL -> BMOVE.
	    */
	    p->op = BMOVE;
	    nfree(p->left);
	    p->left = tp2;

	    tp2 = tp->left;		/* ,2 */

	    /* Build ,OP */
	    nfree(tp->right);		/* ARG3 */
	    tp->op = COMOP;
	    tp->type = p->type;
	    tp->left = p;
	    tp->right = tp2->left->left; /* X */
	    p->right = tp2;		/* ,2 */
	    p = tp;

	    /* Tidy up lower CM to be BMOVE's CM */
	    nfree(tp2->left);
	    tp2->left = tr_copy(tp->right);	/* X */
	    nfree(tp2->right);
	    tp2->right = stringnode;

	    ODEBUG(0, p, "strncpy(): after rewrite");
	}
    }

    return p;
}

static ND1 *
op_compare(p)
ND1 *p;
{
    FP_LDOUBLE fval;
    ND1 *l = p->left;
    ND1 *r = p->right;

    if (TY_ISINTTYPE(l->type) || TY_ISPTR(l->type)) {
	/* Convert exotic ones to simpler forms for integers and pointers. */
	switch (p->op) {
	case NLE:
	    p->op = GT;
	    break;
	case UNLE:
	    p->op = UGT;
	    break;
	case NLT:
	    p->op = GE;
	    break;
	case UNLT:
	    p->op = UGE;
	    break;
	case NGE:
	    p->op = LT;
	    break;
	case UNGE:
	    p->op = ULT;
	    break;
	case NGT:
	    p->op = LE;
	    break;
	case UNGT:
	    p->op = ULE;
	    break;
	case LG:
	case ULG:
	    p->op = NE;
	    break;
	case NLG:
	case UNLG:
	    p->op = EQ;
	    break;
	case LGE:
	case ULGE:
	    /* Always true.  Keep non-ICON subtrees for side-effects. */
	    p->op = COMOP;
	    if (l->op == ICON) {
		nfree(l);
		p->left = r;
	    } else if (r->op == ICON) {
		nfree(r);
	    } else {
		p->left = tr_build(COMOP, l, r);
		l->flags &= ~FF_WASOPT;
		r->flags &= ~FF_WASOPT;
	    }
	    p->right = tr_smicon(1L);
	    p->flags &= ~FF_WASOPT;
	    p = optimize(p);
	    break;
	case NLGE:
	case UNLGE:
	    /* Always false.  Keep non-ICON subtrees for side-effects. */
	    p->op = COMOP;
	    if (l->op == ICON) {
		nfree(l);
		p->left = r;
	    } else if (r->op == ICON) {
		nfree(r);
	    } else {
		p->left = tr_build(COMOP, l, r);
		l->flags &= ~FF_WASOPT;
		r->flags &= ~FF_WASOPT;
	    }
	    p->right = tr_smicon(0L);
	    p->flags &= ~FF_WASOPT;
	    p = optimize(p);
	    break;
	}
	return p;
    }
    if (l->op != CONV ||
	TY_TYPE(l->type) != TY_DOUBLE ||
	TY_TYPE(l->left->type) != TY_FLOAT ||
	r->op != FCON ||
	TY_TYPE(r->type) != TY_DOUBLE
       )
	return p;

    /* Make sure we do not lose precision in IEEE mode */
    fval = FP_XTOFP(r->c.fval);	
    if (ieee_fp() && FP_CMPX(fval, r->c.fval) != FPEMU_CMP_EQUALTO)
	return p;

    /* It is safe to do the optimization */
    nfree(l);
    p->left = l->left;
    r->type = TY_FLOAT;
    r->c.fval = fval;
    return p;
}

static int
op_tysize(t)
T1WORD t;
/* Use type t to return a pseudo size for the purpose of optimizing
** CONV nodes.
*/
{
    switch (TY_UNQUAL(TY_TYPE(t))) {
    case TY_CHAR:  case TY_SCHAR:  case TY_UCHAR:
    case TY_SHORT: case TY_SSHORT: case TY_USHORT:
    case TY_INT:   case TY_SINT:   case TY_UINT:
    case TY_LONG:  case TY_SLONG:  case TY_ULONG:
    case TY_LLONG: case TY_SLLONG: case TY_ULLONG:
    case TY_ENUM:
	return TY_SIZE(t);

    case TY_FLOAT:	return(TY_SIZE(TY_LLONG) + 1); /* must be > integers */
    case TY_DOUBLE:	return(TY_SIZE(TY_LLONG) + 2);
    case TY_LDOUBLE:	return(TY_SIZE(TY_LLONG) + 3);
    }
    cerror(gettxt(":281","op_tysize:  bad type"));
    /*NOTREACHED*/
}

#ifndef FP_EMULATE
static void
op_fpe_handler()
/* floating point exception handler for optim.c */
{
    errno = ERANGE;
    longjmp(fpe_buf,1);
    /*NOTREACHED*/
}
#endif /*FP_EMULATE*/


#ifndef NODBG

static void
op_oprint(p, s)
ND1 *p;
char *s;
{
    DPRINTF("\n*** %s ***\n", s);
    tr_e1print(p, "T");
    DPRINTF("---------\n");
}

#endif
