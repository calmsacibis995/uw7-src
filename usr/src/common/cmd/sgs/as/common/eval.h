#ident	"@(#)nas:common/eval.h	1.2"
/*
* common/eval.h - common assembler expression evaluation
*
* Depends on:
*	"common/as.h"
*/

#define EV_OFLOW 0x01	/* overflow during evaluation */
#define EV_TRUNC 0x02	/* ev_ulong has truncated value */
#define EV_RELOC 0x04	/* ev_{sec,sym,dot,pic,mask} are set: reloc. */
#define EV_G_O_T 0x08	/* ev_sym is SymKind_GOT (see syms.c) */
#define EV_LDIFF 0x10	/* evaluation included symbol subtraction */
#define EV_FLOAT 0x20	/* result is instead pointed to by ev_flt */
#define EV_CONST 0x40	/* entire expression is a single constant */

struct t_eval_	/* result of an expression evaluation */
{
	Integer		*ev_int;	/* integer result value */
	Float		*ev_flt;	/* floating result value */
	const Expr	*ev_expr;	/* the expression evaluated */
	Section		*ev_sec;	/* base section for reloc value */
	Symbol		*ev_sym;	/* base symbol for reloc value */
	Code		*ev_dot;	/* referenced .-equivalent code */
	Ulong		ev_ulong;	/* (truncated) integer result */
	Uint		ev_nubit;	/* min. # of integer bits, unsigned */
	Uint		ev_nsbit;	/* min. # of integer bits, signed */
	Uint		ev_minbit;	/* minimum of ev_n{s,u}bit */
	Uint		ev_flags;	/* EV_* bits */
	int		ev_pic;		/* ExpOp_Pic_* applied */
	int		ev_mask;	/* ExpOp_{Low,High,HighAdjust} */
};

	/*
	* EV_OFLOW is independent of the other EV_* flags, except probably
	* for EV_CONST.  If EV_FLOAT is set, at most only EV_OFLOW and
	* EV_CONST can also be present.  (EV_FLOAT can only be set if
	* FLOATEXPRS is defined.)  EV_TRUNC and EV_LDIFF are otherwise
	* independent of the other flags.  EV_G_O_T can only be set if
	* EV_RELOC is set.
	*
	* If EV_LDIFF is set, the evaluation included at least one
	* subtraction of relocatable subexpressions.  The resulting
	* difference thus depends on the current base addresses for the
	* two subexpressions.  If addresses for the containing section
	* have not yet been fixed [walksect() has not completed that
	* section], the result of the evaluation could change.  When an
	* early evaluation must occur, but the result includes EV_LDIFF,
	* delayeval() and reevaluate() allow for a final double check
	* to see if the value did change.
	*
	* If EV_RELOC is set, the result of the evaluation is the offset,
	* either from the symbol (ev_sec/ev_dot are zero) or from the
	* beginning of the section (ev_sec/ev_dot are nonzero).
	*
	* ev_sec -- zero iff the relocation is based on an undefined name.
	* ev_dot -- zero iff the relocation is based on an undefined name.
	* ev_sym -- zero iff the relocation is only based on some . (dot).
	*
	* Normally, any .set names in an expression are ignored during
	* evaluation.  An exception is made for .set names that are the
	* subject of a TOKTY_IDPICOP operator (@PLT and @GOT).  Since
	* these operations must be bound to a name without any offset, a
	* second result (e.g. specifying the distance from the .set name)
	* is not needed.
	*
	* If ev_sec, ev_dot, and ev_sym are all nonzero (and ev_sym is not
	* a .set symbol), the offset from the symbol can be constructed by
	* subtracting the address of the label (ev_dot->code_addr) from
	* the current value.  See subeval().
	*
	* If EV_RELOC is set and ev_pic and/or ev_mask are set, then the
	* offset is the result up to the first of the appropriate operators
	* in the walk up the tree.
	*/

#ifdef __STDC__
void	initeval(void);			/* initialize */
Eval	*evalexpr(const Expr *);	/* evaluate int/flt/rel expr */
void	evalcopy(Eval *);		/* make eval modifiable */
void	subeval(Eval *, Ulong);		/* modify: eval -= Ulong (signed) */
void	delayeval(Eval *);		/* reeval expr at end */
void	reevaluate(void);		/* do all reevals */
void	flt_todata(Eval *, const Code *, Section *); /* fill floating */

		/* implementation provides */
void	int_todata(Eval *, const Code *, Section *); /* fill nonfloating */
#else
Eval	*evalexpr();
void	initeval(), evalcopy(), subeval(), delayeval(), reevaluate();

void	int_todata();
#endif
