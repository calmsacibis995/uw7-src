#ident	"@(#)nas:i386/stmt386.c	1.31"
/*
* i386/stmt386.c - i386 assembler instructions and statements
*/
#include <stdio.h>
#include <unistd.h>
#include "common/as.h"
#include "common/eval.h"
#include "common/expr.h"
#include "common/stmt.h"
#include "common/sect.h"
#include "common/syms.h"
#include "dirs386.h"
#include "chkgen.h"
#include "relo386.h"
#include "stmt386.h"


/* Define OLD_AS_COMPAT to get instruction encodings consistent
** with the way the old assembler did things.
*/
#undef OLD_AS_COMPAT

typedef Uchar Opclass;		/* big enough to hold an operand class */

/* Table of register information, indexed by register number
** (i.e., Reg_ecx):
**	register class
**	register's encoding in an instruction, values 0-7
**	register's presumed size as an operand (in bytes)
**	register's print-name.
**
** Obviously the entries must be in the order of the register
** numbers declared in stmt386.h.
*/

typedef struct {
    Opclass r_flags;			/* register's operand class */
    Uchar r_code;			/* register's instruction encoding */
    Uchar r_size;			/* register's presumed size (bytes) */
    const char * r_name;		/* register's print name */
} r_info;


static const r_info reginfo[Reg_TOTAL] = {
    { OC_R32, 0, 4, "%eax" },	{ OC_R32, 1, 4, "%ecx" },
    { OC_R32, 2, 4, "%edx" },	{ OC_R32, 3, 4, "%ebx" },
    { OC_R32, 4, 4, "%esp" },	{ OC_R32, 5, 4, "%ebp" },
    { OC_R32, 6, 4, "%esi" },	{ OC_R32, 7, 4, "%edi" },

    { OC_R16, 0, 2, "%ax" },	{ OC_R16, 1, 2, "%cx" },
    { OC_R16, 2, 2, "%dx" },	{ OC_R16, 3, 2, "%bx" },
    { OC_R16, 4, 2, "%sp" },	{ OC_R16, 5, 2, "%bp" },
    { OC_R16, 6, 2, "%si" },	{ OC_R16, 7, 2, "%di" },

    { OC_R8, 0, 1, "%al" },	{ OC_R8, 1, 1, "%cl" },
    { OC_R8, 2, 1, "%dl" },	{ OC_R8, 3, 1, "%bl" },
    { OC_R8, 4, 1, "%ah" },	{ OC_R8, 5, 1, "%ch" },
    { OC_R8, 6, 1, "%dh" },	{ OC_R8, 7, 1, "%bh" },

    { OC_SEG, 0, 2, "%es" },	{ OC_SEG, 1, 2, "%cs" },
    { OC_SEG, 2, 2, "%ss" },	{ OC_SEG, 3, 2, "%ds" },
    { OC_SEG, 4, 2, "%fs" },	{ OC_SEG, 5, 2, "%gs" },

    { OC_SPEC, 0, 4, "%cr0" },	{ OC_SPEC, 2, 4, "%cr2" },
    { OC_SPEC, 3, 4, "%cr3" },  { OC_SPEC, 4, 4, "%cr4" },

    { OC_SPEC, 3, 4, "%tr3" },	{ OC_SPEC, 4, 4, "%tr4" },
    { OC_SPEC, 5, 4, "%tr5" },	{ OC_SPEC, 6, 4, "%tr6" },
    { OC_SPEC, 7, 4, "%tr7" },

    { OC_SPEC, 0, 4, "dr0" },	{ OC_SPEC, 1, 4, "dr1" },
    { OC_SPEC, 2, 4, "dr2" },	{ OC_SPEC, 3, 4, "dr3" },
    { OC_SPEC, 6, 4, "dr6" },	{ OC_SPEC, 7, 4, "dr7" },
 
    { OC_ST, 0, 8, "%st" },
    { OC_STn, 0, 8, "%st(0)" },	{ OC_STn, 1, 8, "%st(1)" },
    { OC_STn, 2, 8, "%st(2)" },	{ OC_STn, 3, 8, "%st(3)" },
    { OC_STn, 4, 8, "%st(4)" },	{ OC_STn, 5, 8, "%st(5)" },
    { OC_STn, 6, 8, "%st(6)" },	{ OC_STn, 7, 8, "%st(7)" },
};


/* This is a table of segment prefixes by segment register
** number, where the segment register number comes from the
** table above.
*/
static const Uchar seg_reg[] = {
	0x26,	/* Reg_es */	0x2E,	/* Reg_cs */
	0x36,	/* Reg_ss */	0x3E,	/* Reg_ds */
	0x64,	/* Reg_fs */	0x65,	/* Reg_gs */
	0xFF, 0xFF	/* dummy values */
};

/* The code_impdep field of a Code struct is used to contain small values
** and flags specific to the i386 part of "as".  Note, some flags must
** be visible in the common portion of "as" and are therefore defined
** in common/sect.h
*/
#define	CODE_NIBBLE		0x0f
/* define CODE_P5_BR_LABEL	0x10	defined in common/sect.h */
/* define CODE_P5_0F_NOT_8X	0x20	defined in common/sect.h */
/* define CODE_NO_CC_PAD	0x40	defined in common/sect.h */
					/* force this padding code sequence
					   to contain nops which DO NOT
					   change the condition code. */
/* define CODE_PREFIX_PSEUDO_OP	0x80	defined in common/sect.h */
					/* this "code" instruction is a
					   prefix pseudo op associated
					   with the following instruction. */

/* This field (nibble) is used to keep track of how many unchecked operands
** there are.  When they have all been checked, the entire instruction
** can be checked.
*/
#define code_unchecked code_impdep

/* This field (nibble) is reused to keep track of the largest extent for
** the variable-size portion of an instruction.  The value is
** never allowed to get smaller, in keeping with span-dependent
** code constraints.
*/
#define code_varsize code_impdep

#define NOP_code 0x90
#define WORD_code 0x66		/* word-operand override */
#define JMP8_code 0xEB		/* code for 1-byte displacement jmp */
#define	FWAIT_code 0x9B		/* code for wait/fwait */

/* Macros to check whether a computed expression's value fits in
** "n" signed bits.  The difference between the last two is whether
** a label difference is permitted; it is not in ALWAYS_FITS_IN().
** FITLONG() is true when the unsigned long value was not truncated
** and those bits act like a signed "n" or fewer bit value.
*/
#define FITLONG(vp, n) ((vp->ev_flags & EV_TRUNC) == 0 && \
		((vp->ev_ulong & ~MASK((n)-1)) == 0 || \
		(vp->ev_ulong & ~MASK((n)-1)) == ~MASK((n)-1)))

#define FITSIGN(vp, n, m) ((vp->ev_flags & (m)) == 0 && \
		(vp->ev_nsbit <= (n) || FITLONG(vp, n)))

#define FITS_IN(vp, n) FITSIGN(vp, n, EV_RELOC|EV_OFLOW)

#define ALWAYS_FITS_IN(vp, n) FITSIGN(vp, n, EV_LDIFF|EV_RELOC|EV_OFLOW)

/* Macro that defines bad operand combination. */
#define	BAD_COMBO	((Ushort) ~0)

#ifdef P5_ERR_41
/* Macros and variables associated with the Pentium Erratum #41 -
   Incorrect Decode of Certain 0F Instructions */

#define P5_BR_WINDOW	31	/* number of bytes in the window for a branch
				   target before the candidate 0f prefix
				   instruction (other than 0f80 - 0f8f) */
#define P5_PEEK_SZ 40		/* One half of the size of the small buffer
				   used to expand instructions and associated
				   relocatable info */
#define ZERO_F_PREFIX	(Uchar)0x0F

static Uchar P5_pseudo_expand;	/* switch to signal expansion must also note
				   relocatable operands. */
static Eval P5_pad_eval;	/* expression evaluation for padding at end of
				   section. */
static Uchar inst_buf[2 * P5_PEEK_SZ];	/* buffer to "peek" at instruction(s)
					   bytes. */

static int P5_0f8x_sites;       /* number of sites that have a potential
                                   0x0f8x bit pattern 33 bytes from
                                   candidate 0f!8x instruction */
static int P5_pad_bytes_added;	/* number of bytes added as nops between
				   instructions */
static int P5_pad_at_sect_end;	/* number of bytes added at the end of a section
				   to eliminate the possibility of a problem 
				   introduced at link time */
#endif

#ifdef __STDC__
static void doinst(const Inst *, Oplist *);
static void chk1oper(Operand *);
static void chk_list(Code *, const chklist_t *);
static void do_oper(Operand *);
static int gen_getregno(Expr *);
static Uchar gen_seg_override(const Operand *);
static size_t gen_lit(Operand *, int, Section *, Uchar *);
static size_t gen_slashr(int, Operand *, Section *, Uchar *);
static size_t gen_pcrel(Operand *, Section *, int, size_t, int);
static void invalidoper(Code *);
static void invalidreg(Inst386 *, Operand *);
#else
static void doinst();
static void chk1oper();
static void chk_list();
static void do_oper();
static int gen_getregno();
static Uchar gen_seg_override();
static size_t gen_lit();
static size_t gen_slashr();
static size_t gen_pcrel();
static void invalidoper();
static void invalidreg();
#endif

/* Produce a diagnostic relating to an operand.  Mark operand as erroneous. */
#define opererror(s,op) \
	op->oper_info = OC_error,				\
	backerror((Ulong) op->parent.oper_olst->olst_file,	\
			op->parent.oper_olst->olst_line, s)
#define regopererror(s,r,op) \
	op->oper_info = OC_error,				\
	backerror((Ulong) op->parent.oper_olst->olst_file,	\
			op->parent.oper_olst->olst_line, s,	\
			reginfo[r].r_name)


/* Produce warning relating to an operand. */
#define operwarn(s,op) \
	backwarn((Ulong) op->parent.oper_olst->olst_file,	\
			op->parent.oper_olst->olst_line, s)


/* Return register encoding number for register expression. */
#define gen_regcode(ep) (reginfo[gen_getregno(ep)].r_code)


/* Check an operand.  If all operands for an instruction have
** been checked, then check the instruction itself.
*/

/*ARGSUSED*/
void
#ifdef __STDC__
operinst(const Inst *instp, Operand *op) /* final single operand check */
#else
operinst(instp, op)Inst *instp; Operand *op;
#endif
{
    Code *cp = op->parent.oper_olst->olst_code;

    if ((cp->code_unchecked & CODE_NIBBLE) == 0)
	fatal(gettxt(":963","operinst():all operands checked already"));
    /* Check the operand if we know everything about it. */
    if (extyamode(op) != ExpTy_Unknown)
	do_oper(op);			/* out of line check */
    return;
}

#ifdef DEBUG
void
#ifdef __STDC__
printoperand(const Operand *op)	/* output contents of operand */
#else
printoperand(op)Operand *op;
#endif
{
	if (op == 0)
	{
		(void)fputs("(Operand*)0", stderr);
		return;
	}
	if (op->oper_flags & Amode_Literal)
	{
		if (op->oper_flags != Amode_Literal)
		{
			fatal(gettxt(":964","printoperand():complex literal operand: %u"),
				(Uint)op->oper_flags);
		}
		(void)putc('$', stderr);
	}
	if (op->oper_flags & Amode_Indirect)
		(void)putc('*', stderr);
	if (op->oper_flags & Amode_Segment)
	{
		printexpr(op->oper_amode.seg);
		(void)putc(':', stderr);
	}
	if (op->oper_flags & Amode_FPreg)
	{
		(void) fputs("%st(", stderr);
		printexpr(op->oper_expr);
		(void)putc(')', stderr);
	}
	else if (op->oper_expr != 0)
		printexpr(op->oper_expr);
	if (op->oper_flags & Amode_BIS)
	{
		(void)putc('(', stderr);
		if (op->oper_amode.base != 0)
			printexpr(op->oper_amode.base);
		if (op->oper_amode.index != 0)
		{
			(void)putc(',', stderr);
			printexpr(op->oper_amode.index);
			if (op->oper_amode.scale != 0)
			{
				(void)putc(',', stderr);
				printexpr(op->oper_amode.scale);
			}
		}
		else if (op->oper_amode.scale != 0)
			fatal(gettxt(":965","printoperand():scale w/out index"));
		(void)putc(')', stderr);
	}
}
#endif	/*DEBUG*/


/* Return currently known type for an operand.  This routine gets
** called from common and mdp code.  Although the common code never
** calls us with oper_flags == 0, it's convenient, for the mdp code,
** to allow that case.
*/

int
#ifdef __STDC__
extyamode(register const Operand *op)
#else
extyamode(op)register Operand *op;
#endif
{
	register Expr *ep;

	if (op->oper_flags == 0)
	    return( op->oper_expr->ex_type );
	ep = op->oper_expr;
	if ((op->oper_flags & Amode_Literal) != 0)
	{
		if (ep == 0)
			fatal(gettxt(":966","extyamode():expr-less literal"));
		if (ep->ex_type == ExpTy_Unknown)
			return ExpTy_Unknown;
		return ExpTy_Operand;
	}
	if (ep != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	if ((ep = op->oper_amode.seg) != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	if ((ep = op->oper_amode.base) != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	if ((ep = op->oper_amode.index) != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	if ((ep = op->oper_amode.scale) != 0 && ep->ex_type == ExpTy_Unknown)
		return ExpTy_Unknown;
	return ExpTy_Operand;
}

int
#ifdef __STDC__
setamode(register const Operand *op)	/* fix amode as .set; return type */
#else
setamode(op)register Operand *op;
#endif
{
	register Expr *ep;
	register int exty = ExpTy_Operand;

	if (op->oper_flags == 0)
		fatal(gettxt(":967","setamode():single expression operand"));
	if ((ep = op->oper_expr) != 0)
	{
		fixexpr(ep);
		ep->ex_cont = Cont_Set;
		/* Early exit for literal, %st(n) */
		if ((op->oper_flags & (Amode_Literal|Amode_FPreg)) != 0)
			return ExpTy_Operand;
		if (ep->ex_type == ExpTy_Unknown)
			exty = ExpTy_Unknown;
	}
	if ((ep = op->oper_amode.seg) != 0)
	{
		fixexpr(ep);
		ep->ex_cont = Cont_Set;
		if (ep->ex_type == ExpTy_Unknown)
			exty = ExpTy_Unknown;
	}
	if (op->oper_flags & Amode_BIS)
	{
		if ((ep = op->oper_amode.base) != 0)
		{
			fixexpr(ep);
			ep->ex_cont = Cont_Set;
			if (ep->ex_type == ExpTy_Unknown)
				exty = ExpTy_Unknown;
		}
		if ((ep = op->oper_amode.index) != 0)
		{
			fixexpr(ep);
			ep->ex_cont = Cont_Set;
			if (ep->ex_type == ExpTy_Unknown)
				exty = ExpTy_Unknown;
		}
		if ((ep = op->oper_amode.scale) != 0)
		{
			fixexpr(ep);
			ep->ex_cont = Cont_Set;
			if (ep->ex_type == ExpTy_Unknown)
				exty = ExpTy_Unknown;
		}
	}
	return exty;
}

void
#ifdef __STDC__
amodefree(Operand *op)	/* return amode expressions to free list */
#else
amodefree(op)Operand *op;
#endif
{
	Expr *ep;

	if ((ep = op->oper_amode.seg) != 0)
		exprfree(ep);
	if ((ep = op->oper_amode.base) != 0)
		exprfree(ep);
	if ((ep = op->oper_amode.index) != 0)
		exprfree(ep);
	if ((ep = op->oper_amode.scale) != 0)
		exprfree(ep);
}


/* Check operands for proper form. */


/* Substitute for any .set symbols in an operand.  The general
** form is shown below, and those parts that can be substituted
** are underlined.  Note the relationship to the operand parsing
** in parse.c.
**
**	[seg:][expr][([r][,r[,expr]])]
**	 ---   ----    -   -  ----
**	       ----------------------
**	 ----------------------------
**
*/

static void
#ifdef __STDC__
setsubst(Operand *op)
#else
setsubst(op) Operand *op;
#endif
{
    Expr *ep;
    Operand *setop;			/* operand that's subject of .set */

    op->oper_info = ~0;			/* Pick absurd invalid value so we
					** can tell (chk1oper()) if there has
					** been an error yet.
					*/
    /* As a first step, substitute for individual pieces. */
    if (op->oper_flags & Amode_Segment) {
	ep = op->oper_amode.seg;
	if (ep->ex_type == ExpTy_Register && ep->ex_op != ExpOp_LeafRegister)
	    op->oper_amode.seg = setlessexpr(ep);
    }
    if (op->oper_flags & Amode_BIS) {
	if ((ep = op->oper_amode.base) != 0) {
	    if (   ep->ex_type == ExpTy_Register
		&& ep->ex_op != ExpOp_LeafRegister
	    )
		op->oper_amode.base = setlessexpr(ep);
	}
	if ((ep = op->oper_amode.index) != 0) {
	    if (   ep->ex_type == ExpTy_Register
		&& ep->ex_op != ExpOp_LeafRegister
	    )
		op->oper_amode.index = setlessexpr(ep);
	}
    }

    if ((ep = op->oper_expr) == 0)
	return;				/* no further substitutions */

    /* Follow chain of .set's for "main" expression until we find
    ** one that isn't a simple ".set x,y".  Then look for possible
    ** substitutions of large pieces.
    */
    while (ep->ex_op == ExpOp_LeafName) {
	Symbol *sp = ep->right.ex_sym;

	if (sp->sym_kind != SymKind_Set)
	    break;
	if (sp->sym_exty == ExpTy_Unknown)
	    fatal(gettxt(":968","setsubst():unknown symbol type?"));
	/* Check for "complex" operand. */
	if ((setop = sp->addr.sym_oper)->oper_flags != 0)
	    break;
	if ((ep = setop->oper_expr) == 0)
	    fatal(gettxt(":969","setsubst():can't follow .set chain"));
    }

    switch(ep->ex_type) {
    default:
	/* %st(n) case handled elsewhere, and better. */
	if ((op->oper_flags & Amode_FPreg) == 0)
	    opererror(gettxt(":970","operand must be integer or relocatable"), op);
	break;
    case ExpTy_Register:
	if (op->oper_flags & Amode_BIS)
	    regopererror(gettxt(":971","invalid use of register in operand: %s"),
		ep->right.ex_reg, op);
	/*FALLTHROUGH*/
    case ExpTy_Integer:
    case ExpTy_Relocatable:
	op->oper_expr = ep;
	break;
    case ExpTy_Operand:
	/* "setop" could have embedded .set's in it that have not
	** yet been resolved.  Resolve them.
	*/
	setsubst(setop);

	/* Only excitement is when .set operand has Amode_ bits. */
	if (setop->oper_flags != 0) {
	    /* No duplicate flags.  Can't have FPreg in one and
	    ** anything in the other.
	    */
	    if (   (setop->oper_flags & op->oper_flags) != 0
		|| (setop->oper_flags & Amode_FPreg) != 0 &&    op->oper_flags
		|| (   op->oper_flags & Amode_FPreg) != 0 && setop->oper_flags
	    )
		opererror(gettxt(":972",".set substitution produces invalid operand"), op);
	    else {
		op->oper_flags |= setop->oper_flags;
		op->oper_expr = setop->oper_expr;
		if (setop->oper_flags & Amode_Segment)
		    op->oper_amode.seg = setop->oper_amode.seg;
		if (setop->oper_flags & Amode_BIS) {
		    op->oper_amode.base = setop->oper_amode.base;
		    op->oper_amode.index = setop->oper_amode.index;
		    op->oper_amode.scale = setop->oper_amode.scale;
		}
	    }
	}
	break;
    }
    return;
}


/* Process a single operand:  do any substitutions resulting
** from .set symbols.  Check the operand.  Decrement the
** associated instruction's count of operands to check.
** If zero, call the instruction's checking routine for
** operands.  The operand is presumed to have completely
** known expression types.
*/
static void
#ifdef __STDC__
do_oper(Operand *op)
#else
do_oper(op) Operand *op;
#endif
{
    int oper_cnt;
    Code *cp = op->parent.oper_olst->olst_code;

    if ((oper_cnt = cp->code_unchecked & CODE_NIBBLE) == 0)
	fatal(gettxt(":973","do_oper():all operands checked?"));
    setsubst(op);
    chk1oper(op);

    /* If we have complete information about all the operands,
    ** check the operand combinations.  If there is a list,
    ** try chk_list first.  Then try the check routine, if
    ** there is one.
    */
    cp->code_unchecked = (cp->code_unchecked & ~CODE_NIBBLE) |
			 ((--oper_cnt) & CODE_NIBBLE);
    if ( oper_cnt == 0 ) {
	Inst386 *ip3 = (Inst386 *) cp->info.code_inst;

	if (ip3->chklist)
	    chk_list(cp, ip3->chklist);
	if (ip3->operchk)
	    (*ip3->operchk)(cp);
    }
    return;
}

/* Check a list of operand combinations for a match.  There
** may be one or two operands to match.  If a match is found,
** the number of the matching combination is stored in
** olst_combo.  Otherwise a diagnostic is printed if neither
** of the operands has operand class OC_error.
*/

static void
#ifdef __STDC__
chk_list(Code *cp, const chklist_t *clp)
#else
chk_list(cp, clp) Code *cp; chklist_t *clp;
#endif
{
    Operand *first = cp->data.code_olst->olst_first;
    Operand *second;
    Inst386 *ip3 = (Inst386 *) cp->info.code_inst;
    Uchar checkval;
    Uchar l_auxno, r_auxno;		/* auxiliary numbers */
    const chklist_t *clp2;
    Expr *ep;
    
    /* A single operand is treated as the right operand. */
    if ((ep = first->oper_expr) != 0 && ep->ex_type == ExpTy_Register)
	r_auxno = ep->right.ex_reg + 1;
    else
	r_auxno = ip3->opersize;
    

    if ((second = first->oper_next) != 0) {
	/* First (left, syntactically) operand is second in i386 book. */
	checkval = CASEVAL(second->oper_info, first->oper_info);
	if ((ep = second->oper_expr) != 0 && ep->ex_type == ExpTy_Register)
	    l_auxno = ep->right.ex_reg + 1;
	else
	    l_auxno = ip3->opersize;
    }
    else
	checkval = CASEVAL(first->oper_info,0);

    /* Look for matching list entry. */
    for (clp2 = clp; clp2->cl_combo != 0; ++clp2) {
	if (clp2->cl_combo != checkval) continue;
	/* Look for matching register or operand size on both sides. */
	if (clp2->cl_l_auxno != 0 && clp2->cl_l_auxno != l_auxno)
	    continue;
	if (clp2->cl_r_auxno != 0 && clp2->cl_r_auxno != r_auxno)
	    continue;

	/* Found a match.  Record combination in operand list. */
	cp->data.code_olst->olst_combo = clp2 - clp;
#ifdef DEBUG
	if (DEBUG('C') > 0) {
	    /* print mnemonic and combination number */
	    (void) fprintf(stderr, "%s\t%d\n",
			(const char *) ((Inst *)ip3)->inst_name, (int)(clp2-clp));
	}
#endif
	return;
    }
    /* Suppress diagnostic if one of the operands has type OC_error:
    ** presumably a diagnostic has been issued already.
    */
    if (   first->oper_info != OC_error
	&& (second == 0 || second->oper_info != OC_error)
	)
	invalidoper(cp);

    /* Choose combination that won't match any in check-routine. */
    cp->data.code_olst->olst_combo = BAD_COMBO;
    return;
}
    
/* Check a single operand for context-independent correctness. */

static void
#ifdef __STDC__
chk1oper(Operand * op)
#else
chk1oper(op) Operand * op;
#endif
{
    int fl = op->oper_flags;
    Expr * ep;

    /* Assume operand is okay. */
    if (op->oper_info != OC_error)
	op->oper_info = OC_MEM;		/* assume a memory operand if no
					** errors yet; if an error is detected,
					** opererror() will overwrite
					*/

    /* Check literal for proper types. */
    if (fl & Amode_Literal) {
	switch( op->oper_expr->ex_type ){
	case ExpTy_Integer:
	case ExpTy_Relocatable:
	    op->oper_info = OC_LIT;
	    break;			/* these are okay */
	default:
	    opererror(gettxt(":974","literal must be integer or relocatable"), op);
	}
	return;
    }

    if (   (fl & Amode_Indirect) != 0
	&& ((op->parent.oper_olst->olst_code->info.code_inst)->chkflags & IF_STAR) == 0
    ) {
	operwarn(gettxt(":975","'*' indirection invalid here"), op);
	fl = op->oper_flags &= ~Amode_Indirect;
    }

    /* Check segment. */
    if (fl & Amode_Segment) {
	ep = op->oper_amode.seg;
	if (ep->ex_op != ExpOp_LeafRegister)
		opererror(gettxt(":0","segment register expected before colon") , op);
	else if (reginfo[ep->right.ex_reg].r_flags != OC_SEG)
	    	regopererror(gettxt(":976","invalid segment register: %s"), ep->right.ex_reg, op);
	/* Segment register cannot modify register operand (without '*'). */
	if (   (ep = op->oper_expr) != 0
	    && ep->ex_type == ExpTy_Register
	    && (fl & Amode_Indirect) == 0
	    )
	    operwarn(gettxt(":977","segment override ineffective with register operand"), op);

    }
    /* Check floating register. */
    if (fl & Amode_FPreg) {
	static const char mesg[] =
		"floating point register designator must be integer, 0-7";
	ep = op->oper_expr;
	if (ep->ex_type != ExpTy_Integer)
	    opererror(mesg, op);
	else {
	    Eval * vp;
	    vp = evalexpr(ep);
	    if (   (vp->ev_flags & (EV_OFLOW|EV_TRUNC)) != 0
		|| vp->ev_ulong > 7
	    )
		opererror(mesg, op);
	    else {
		if (vp->ev_flags & EV_LDIFF)	/* register for later */
		    delayeval(vp);

		ep = regexpr(Reg_st0 + vp->ev_ulong);
		ep->ex_cont = Cont_Operand;
		ep->parent.ex_oper = op;
		op->oper_expr = ep;
		fl = op->oper_flags &= ~Amode_FPreg;
	    }
	}
    }

    /* If no base/index/displacement, operand must be register,
    ** integer, or relocatable.
    */
    if ((fl & Amode_BIS) == 0) {
	ep = op->oper_expr;
	switch( ep->ex_type ){
	default:
	    break;			/* setsubst() already detected these */
	case ExpTy_Register:
	{
	    Inst386 *ip3 = (Inst386 *) op->parent.oper_olst->olst_code->info.code_inst;
	    if (ep->ex_op != ExpOp_LeafRegister)
		fatal(gettxt(":978","chk1oper():confused register"));
	    op->oper_info = reginfo[ep->right.ex_reg].r_flags;
	    if (((Inst *) ip3)->chkflags & IF_RSIZE) {
		if (ip3->opersize != reginfo[ep->right.ex_reg].r_size)
		    invalidreg(ip3, op);
	    }
	    break;
	}
	case ExpTy_Integer:
	case ExpTy_Relocatable:
	    break;
	}
    }
    else {
	/* Base and index must be 32-bit registers; scale must be
	** integer and 1, 2, 4, 8.
	*/
	if ((ep = op->oper_amode.base) != 0) {
	    if (ep->ex_op != ExpOp_LeafRegister)
		opererror(gettxt(":0","index must be a register"), op);
	    else if (reginfo[ep->right.ex_reg].r_flags != OC_R32) {
		/* There's one special case (uugh!):  %dx can look
		** like base register for in/out instructions (IF_BASE_DX
		** set).
		*/
		if (   ep->right.ex_reg != Reg_dx
		    || (op->parent.oper_olst->
			    olst_code->
				info.code_inst->chkflags & IF_BASE_DX) == 0
		    ) {
			regopererror(gettxt(":979","base must be 32-bit register: %s"),
			    ep->right.ex_reg, op);
		}
	    }
	}
	if ((ep = op->oper_amode.index) != 0) {
	    if (  ep->ex_op != ExpOp_LeafRegister )
		opererror(gettxt(":0","index must be a register"), op);
	    else if (reginfo[ep->right.ex_reg].r_flags != OC_R32) {
		regopererror(gettxt(":980",
			"index must be 32-bit register: %s"),
			ep->right.ex_reg, op);
	    }
	    else if (ep->right.ex_reg == Reg_esp) {
		opererror(gettxt(":981","index register cannot be %%esp"), op);
	    }
	}
	if ((ep = op->oper_amode.scale) != 0) {
	    static const char MSGscale[] =
		"scale must be integer with value 1, 2, 4, or 8";

	    if (ep->ex_type != ExpTy_Integer)
		opererror(gettxt(":982",MSGscale), op);
	    else {
		/* Check for valid scale value. */
		Eval * vp = evalexpr(ep);

		switch ( vp->ev_ulong ){
		case 1:
		case 2:
		case 4:
		case 8:
		    if ((vp->ev_flags & (EV_OFLOW|EV_TRUNC)) == 0) {
			if (vp->ev_flags & EV_LDIFF)
			    delayeval(vp);
			break;
		    }
		    /* error on overflow/value too big */
		    /*FALLTHRU*/
		default:
		    opererror(gettxt(":982",MSGscale), op);
		    break;
		}
	    }
	}
    }
    return;
}


/* Print "invalid operand combination" diagnostic. */
static void
#ifdef __STDC__
invalidoper(Code *cp)
#else
invalidoper(cp) Code *cp;
#endif
{
    static const char mesg[] = "invalid operand combination: %s";
    Oplist *parent = cp->data.code_olst;

    backerror((Ulong) parent->olst_file, parent->olst_line,
		mesg, (const char *) cp->info.code_inst->inst_name);
    return;
}

/* Print "invalid register for instruction: %s in %s". */
static void
#ifdef __STDC__
invalidreg(Inst386 *ip3, Operand *op)
#else
invalidreg(ip3, op) Inst386 *ip3; Operand *op;
#endif
{
    backerror((Ulong) op->parent.oper_olst->olst_file,
			op->parent.oper_olst->olst_line,
			"invalid register for instruction: %s in %s",
			reginfo[op->oper_expr->right.ex_reg].r_name,
			(const char *) ((Inst *) ip3)->inst_name);

    op->oper_info = OC_error;		/* mark operand as erroneous */
    return;
}


static size_t
#ifdef __STDC__
gen_nopsets(Section *secp, Code *cp)	/* set whether nop-fill can set cc */
#else
gen_nopsets(secp, cp)Section *secp; Code *cp;
#endif
{
	secp->sec_impdep = (cp->code_impdep & CODE_NIBBLE);
	return 0;
}


void
#ifdef __STDC__
nopsets(const Uchar *str, size_t len)	/* note nop-fill attributes */
#else
nopsets(str, len)Uchar *str; size_t len;
#endif
{
	static const Inst inst_nopsets
		= {(const Uchar *)".nopsets", gen_nopsets, 0, 0};
	Section *secp;
	Code *cp;
	Uchar ccval;

	if (str == 0)
		ccval = 0;
	else if (len == 2 && str[0] == 'c' && str[1] == 'c')
		ccval = 1;
	else
	{
		error(gettxt(":983","invalid .nopsets operand: \"%s\""), prtstr(str, len));
		return;
	}
	secp = cursect();
	cp = secp->sec_last;
	sectfinst(secp, &inst_nopsets, (Oplist *)0);
	cp->code_impdep = (cp->code_impdep & ~CODE_NIBBLE) + ccval;
}


void
#ifdef __STDC__
stmt(const Uchar *str, size_t len, Oplist *olp)	/* handle statement */
#else
stmt(str, len, olp)Uchar *str; size_t len; Oplist *olp;
#endif
{
	const Inst * ip;
#ifdef DEBUG
	if (DEBUG('I') > 0)
	{
		(void)fprintf(stderr, "stmt(%s,ops=", prtstr(str, len));
		printoplist(olp);
		(void)fputs(")\n", stderr);
	}
#endif
	if (str[0] == '.')	/* must be a directive */
	{
		directive386(str, len, olp);
		return;
	}
	if ((ip = findinst(str, len)) != 0)
		doinst(ip, olp);
}


/* Do initial processing on instruction:
**	1.  Count the operands
**	2.  Check operand count
**	3.  Build Code entry for the instruction
**	4.  For each operand, check its health.
*/
static void
#ifdef __STDC__
doinst(const Inst *ip, Oplist *olp)
#else
doinst(ip, olp) Inst *ip; Oplist *olp;
#endif
{
    Uchar numops = 0;
    Operand * op;
    Inst386 *ip3 = (Inst386 *) ip;

    if (olp) {
	for (op = olp->olst_first; op != 0; op = op->oper_next) {
	    /* Fix each expression. */
	    if (op->oper_expr)
		fixexpr(op->oper_expr);
	    if (op->oper_flags & Amode_BIS) {
		if (op->oper_amode.base != 0)
		    fixexpr(op->oper_amode.base);
		if (op->oper_amode.index != 0)
		    fixexpr(op->oper_amode.index);
		if (op->oper_amode.scale != 0)
		    fixexpr(op->oper_amode.scale);
	    }
	    ++numops;
	}
    }
    
    if (numops < ip3->minops)
	error(gettxt(":984","too few operands: %s"), (const char *) ip->inst_name);
    else if (numops > ip3->maxops)
	error(gettxt(":985","too many operands: %s"), (const char *) ip->inst_name);
    else {
	Section * secp = cursect();
	Code * cp = secp->sec_last;	/* current last will be new Code */

	if (ip->chkflags & IF_VARSIZE)
	    sectvinst(secp, ip, olp);
	else
	    sectfinst(secp, ip, olp);

	cp->code_unchecked = (cp->code_unchecked & ~CODE_NIBBLE) | numops;

	if (olp) {
	    for (op = olp->olst_first; op != 0; op = op->oper_next) {
		if (extyamode(op) != ExpTy_Unknown)
		    do_oper(op);	/* checking in line */
	    }
	}
    }
    return;
}
 
/*********************************************************
**							**
**	Routines to Check Operand Combinations		**
**							**
*********************************************************/
/* The routines below get called to check combinations of
** operands.  Unless otherwise noted, chk_list() has already
** been called, and these routines just select special case
** encodings.  In rare instances, the routines here do all
** the checking.
**
** For those cases where there are both a check- and a
** gen-routine for an instruction, the check routine will
** be found just before the gen-routine.
*/


/* Check arithmetic/logical instructions.  Select a small-form
** literal if we know it always fits.  Otherwise leave the
** large-form encoding as the one to use.  If the left (in the
** table) operand is al/ax/eax, use the special encoding for it.
*/

/*ARGSUSED*/
void
#ifdef __STDC__
chk_ar2(Code *cp)
#else
chk_ar2(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;

    switch ( olp->olst_combo ){
	Eval *vp;
    case ar2_rm32_lit:		/* r32,$ */
#ifdef OLD_AS_COMPAT
	/* Old assembler chose %eax encoding always. */
	if ( gen_getregno(olp->olst_first->oper_next->oper_expr) == Reg_eax )
	    olp->olst_combo += ar2_eax_lit - ar2_rm32_lit;
	else {
	    vp = evalexpr(olp->olst_first->oper_expr);
	    if (ALWAYS_FITS_IN(vp, 8))
		olp->olst_combo += ar2_lit8_32 - ar2_rm32_lit;
	}
#else
	vp = evalexpr(olp->olst_first->oper_expr);
	if (ALWAYS_FITS_IN(vp, 8))
	    olp->olst_combo += ar2_lit8_32 - ar2_rm32_lit;
	else if ( gen_getregno(olp->olst_first->oper_next->oper_expr) == Reg_eax )
	    olp->olst_combo += ar2_eax_lit - ar2_rm32_lit;
#endif
	break;
    case ar2_rm32_lit+1:	/* m32,$ */
	vp = evalexpr(olp->olst_first->oper_expr);
	if (ALWAYS_FITS_IN(vp, 8))
	    olp->olst_combo += ar2_lit8_32 - ar2_rm32_lit;
	break;
    case ar2_rm16_lit:		/* r16,$ */
	if (gen_getregno(olp->olst_first->oper_next->oper_expr) == Reg_ax) {
	    olp->olst_combo += ar2_ax_lit - ar2_rm16_lit;
	    break;
	}
	/*FALLTHRU*/
    case ar2_rm16_lit+1:	/* m16,$ */
	vp = evalexpr(olp->olst_first->oper_expr);
	if (ALWAYS_FITS_IN(vp, 8))
	    olp->olst_combo += ar2_lit8_16 - ar2_rm16_lit;
	break;

    case ar2_r8_lit:		/* r8,$ */
	if (gen_getregno(olp->olst_first->oper_next->oper_expr) == Reg_al)
	    olp->olst_combo += ar2_al_lit - ar2_r8_lit;
	break;
    }
	
    return;
}


/* Check in/out instructions.  Select proper encoding based on
** instruction (operand) size.  Verify, for "m" case, that the
** operand looks like "(%dx)".
*/

void
#ifdef __STDC__
chk_inout(Code *cp)
#else
chk_inout(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;

    if (olp->olst_combo == cl_inout_dx) {
	/* Make sure operand has proper form. */
	Operand *op = olp->olst_first;

	if (   op->oper_expr != 0
	    || (op->oper_flags & Amode_BIS) == 0
	    || op->oper_amode.base == 0
	    || op->oper_amode.base->right.ex_reg != Reg_dx
	    || op->oper_amode.index != 0
	    || op->oper_amode.scale != 0
	)
	    opererror(gettxt(":986","operand must be \"(%%dx)\" or literal"), op);
    }

    /* Choose appropriate encoding variant of literal or (%dx). */
    switch ( ((Inst386 *)cp->info.code_inst)->opersize ) {
    default:	fatal(gettxt(":987","chk_inout():bad size"));	/*NOTREACHED*/
    case 4:	++olp->olst_combo;	/*FALLTHRU*/
    case 2:	++olp->olst_combo;	/*FALLTHRU*/
    case 1:	break;
    }
    return;
}

/* Check int instruction:  select special case for 3. */

/*ARGSUSED*/
void
#ifdef __STDC__
chk_int(Code *cp)
#else
chk_int(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Eval *vp;

    if (olp->olst_combo == 0) {
	vp = evalexpr(olp->olst_first->oper_expr);
	if (ALWAYS_FITS_IN(vp, 8) && vp->ev_ulong == 3)
	    olp->olst_combo += cl_int_3 - 0;
    }
    return;
}


/* Most of checking for mov instructions is taken care of by
** chk_list().  Change move of special register to appropriate
** variant.  For move between register and memory, if the register
** is %[e]a[xl] and the memory operand is a pure offset, choose the
** special encoding.
*/

void
#ifdef __STDC__
chk_mov(Code *cp)
#else
chk_mov(cp) Code *cp;
#endif
{
    Ushort combo = cp->data.code_olst->olst_combo;
    Operand *spreg = 0;

    /* Remember that operands in table are reversed from operand
    ** order of assembly syntax.
    */
    switch( combo ){
	Operand *op;
    case mov_spreg:		/* special from CPU register */
	spreg = cp->data.code_olst->olst_first;
	break;
    case mov_spreg+1:		/* CPU register from special */
	spreg = cp->data.code_olst->olst_first->oper_next;
	break;
    case mov_regmem:
    case mov_regmem+2:
    case mov_regmem+4:
	/* Use special form if %[e]a[xl] and not base/index/scale. */
	op = cp->data.code_olst->olst_first;
	if (   (op->oper_flags & Amode_BIS) == 0
	    && gen_regcode(op->oper_next->oper_expr) == 0
	    )
	    combo += mov_axmoff - mov_regmem;
	break;
    case mov_memreg:
    case mov_memreg+2:
    case mov_memreg+4:
	/* Use special form if %[e]a[xl] and not base/index/scale. */
	op = cp->data.code_olst->olst_first;
	if (   (op->oper_next->oper_flags & Amode_BIS) == 0
	    && gen_regcode(op->oper_expr) == 0
	    )
	    combo += mov_moffax - mov_memreg;
	break;
    }
    /* Get the register number so we can discern its type and
    ** choose the correct gen_list variant.
    */
    if (spreg) {
	switch( gen_getregno(spreg->oper_expr) ) {
	case Reg_cr0: case Reg_cr2: case Reg_cr3: case Reg_cr4:
	    combo += mov_cr - mov_spreg;
	    break;
	case Reg_dr0: case Reg_dr1: case Reg_dr2: case Reg_dr3:
	case Reg_dr6: case Reg_dr7:
	    combo += mov_dr - mov_spreg;
	    break;
	case Reg_tr3: case Reg_tr4: case Reg_tr5: case Reg_tr6:
	case Reg_tr7:
	    combo += mov_tr - mov_spreg;
	    break;
	default:
	    fatal(gettxt(":988","chk_mov():confused special register"));
	}
    }
    cp->data.code_olst->olst_combo = combo;
    return;
}


/* Check pop instruction:  choose variant for pop-segment-reg.
** Check operand size for register.
*/

void
#ifdef __STDC__
chk_pop(Code *cp)
#else
chk_pop(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    int instoper = ((Inst386 *) cp->info.code_inst)->opersize;
    int opersize = 0;

    switch( olp->olst_combo ){
	Ushort combo;
    case pop_r16:	opersize = 2; break;
    case pop_r32:	opersize = 4; break;
    case pop_sreg:
	/* Avoid operand size check:  allow popw or popl of segment. */
	switch( gen_getregno(olp->olst_first->oper_expr) ){
	case Reg_cs:	combo = pop_sreg; opersize = 100; break; /* gets diagnostic */
	case Reg_ss:	combo = pop_ss; break;
	case Reg_ds:	combo = pop_ds; break;
	case Reg_es:	combo = pop_es; break;
	case Reg_fs:	combo = pop_fs; break;
	case Reg_gs:	combo = pop_gs; break;
	}
	if (instoper == 2)
	    combo += pop_sreg_w - pop_sreg;	/* choose popw versions */

	olp->olst_combo = combo;
	break;
    }
    if (opersize && opersize != instoper)
	invalidreg((Inst386 *) cp->info.code_inst, olp->olst_first);
    return;
}

/* Check push instruction:  choose variant for push-segment-reg.,
** literal.  Check operand size for register.
*/

void
#ifdef __STDC__
chk_push(Code *cp)
#else
chk_push(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    int instoper = ((Inst386 *) cp->info.code_inst)->opersize;
    int opersize = 0;

    switch( olp->olst_combo ){
	Ushort combo;
	Eval *vp;
    case push_r16:	opersize = 2; break;
    case push_r32:	opersize = 4; break;
    case push_sreg:
	/* Avoid operand size check:  allow pushw/pushl. */
	switch( gen_getregno(olp->olst_first->oper_expr) ){
	case Reg_cs:	combo = push_cs; break;
	case Reg_ss:	combo = push_ss; break;
	case Reg_ds:	combo = push_ds; break;
	case Reg_es:	combo = push_es; break;
	case Reg_fs:	combo = push_fs; break;
	case Reg_gs:	combo = push_gs; break;
	}
	if (instoper == 2)
	    combo += push_sreg_w - push_sreg;	/* choose pushw versions */

	olp->olst_combo = combo;
	break;
    case push_imm8:
	/* Push word, rather than long, if instruction requires. */
	if (instoper == 2)
	    ++olp->olst_combo;

	/* Use long-form immediate if we know short-form won't fit. */
	vp = evalexpr(olp->olst_first->oper_expr);
	if (! ALWAYS_FITS_IN(vp, 8))
	    olp->olst_combo += push_imm - push_imm8;
	break;
    }
    if (opersize && opersize != instoper)
	invalidreg((Inst386 *) cp->info.code_inst, olp->olst_first);
    return;
}


/* Check shift family of instructions.  Make sure any register
** operand (for the second operand) is the same size as the
** instruction expects.  (Can't use IF_RSIZE flag because %cl
** would fail for word/long shifts.)
*/

void
#ifdef __STDC__
chk_shift(Code *cp)
#else
chk_shift(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *second = olp->olst_first->oper_next;
    Expr *ep;

    if (second != 0) {
	if ((ep = second->oper_expr) != 0 && ep->ex_type == ExpTy_Register) {
	    Inst386 *ip3 = (Inst386 *)cp->info.code_inst;

	    if (reginfo[gen_getregno(ep)].r_size != ip3->opersize)
		invalidreg(ip3, second);
	}
	/* 3 byte format on 486 takes only 2 cycles */
	if (   olp->olst_combo >= shft_imm_s
	    && olp->olst_combo < shft_imm_e
	    && proc_type == ProcType_386
	) {
	    Eval *vp = evalexpr(olp->olst_first->oper_expr);

	    if(ALWAYS_FITS_IN(vp, 2) && vp->ev_ulong == 1)
		olp->olst_combo += shft_imm_1 - shft_imm_s;
	}
    }
    return;
}


/*************************************************
**						**
**	Routines to Generate Instructions	**
**						**
*************************************************/
/* In most cases, the binary encoding of an instruction is produced
** by gen_list(), for which an operand combination ("combo") has
** been stored in olst_combo.  Special code must be used for some
** instructions, however, to deal with special cases, too many
** operands, or combinations that gen_list() doesn't support.
*/

/* All the generate routines return the size of their encoding.
** If the code's section's sec_data field is non-zero, the actual
** binary encoding is stored in the appropriate place.  The calls
** that occur when sec_data is zero are from setvarsz(), to get
** the size of variable-size instructions.
*/


/* Generate code based on a genlist_t.  The list is pointed to
** by the Inst386, and the selected combination is in olst_combo,
** selected (usually) by chk_list().
**
** If the operand encoding has no size-varying components, the
** instruction's code_kind is changed to CodeKind_FixInst.  In
** any case, the size of the operand encoding is returned.
**
** Other assumptions:
**	1. If there are two operands:
**	  a) One is assumed to be "memory", the other "register".
**	  b) If one is a literal, it behaves like the "register" operand.
**	2. This routine works for single-operand instructions.  The
**		operand may be treated as memory, register, or both.
**
** These flags in the genlist_t affect processing:
**	GL_MEMRIGHT	treat the right (second) operand in the chkgen
**			tables as the "memory" operand
**	GL_PREFIX	generate 0x0F prefix byte
**	GL_FWAIT	generate "fwait" prefix byte
**	GL_OVERRIDE	generate (word) operand size override byte
**	GL_PLUS_R	operand is "+r" form:  add register code to opcode
**	GL_MOFFSET	memory operand is pure memory offset (no /r form)
**	GL_SLASH_R	generate /r-type operand:  'r' value is from register
**			operand
**	GL_SLASH_N	generate /n-type operand:  'n' value is from
**			genlist_t entry
**	GL_IMM		generate immediate operand whose size is that of
**			the instruction operand (i.e., 1 for byte, 2 for
**			word, 4 for long)
**	GL_IMM8		generate 8-bit immediate value (only)
**	GL_PCREL	generate PC-relative branch; the size, either 1 or
**			4-byte, comes from the genlist_t entry.
**
** GL_SLASH_R, GL_SLASH_N, GL_MOFFSET, and GL_PCREL are mutually
** exclusive.
*/

static Uchar *p0;	/* beginning of encoding started by gen_list */

size_t
#ifdef __STDC__
gen_list(Section *secp, Code *cp)
#else
gen_list(secp, cp) Section *secp; Code *cp;
#endif
{
    const genlist_t *glp =
	&((Inst386 *)cp->info.code_inst)->
		genlist[cp->data.code_olst->olst_combo];
    Uchar *p;
    Uchar dummy[20];		/* Room to store the encoding of the largest
    				** instruction.  Use this space instead of
				** secp->sec_data when the latter is 0 if
				** we're just getting the instruction's size,
				** rather than storing its bytes.
				*/
    Ushort flags = glp->gl_flags;
    Operand *regop;
    Operand *memop;
    int gen = 0;	/* set to non-zero if we're generating code */

    /* Select the register operand and the memory operand.  If
    ** there's only one operand, it serves as both.  If MEMRIGHT
    ** (which refers to the chkgen table) is set, the memory
    ** operand is on the right in the table (left or first in
    ** the operand list).  MEMRIGHT is never set for one-operand
    ** instructions.
    */
    if ((flags & GL_MEMRIGHT) == 0) {
	regop = cp->data.code_olst->olst_first;
	memop = regop->oper_next;
	/* If there's only one operand, treat it as both kinds. */
	if (memop == 0)
	    memop = regop;
    }
    else {
	memop = cp->data.code_olst->olst_first;
	regop = memop->oper_next;
    }

    /* Establish initial, running pointers to data. */
    if ((p = secp->sec_data) == 0)
	p = &dummy[0];
    else {
	gen = 1;
	p += cp->code_addr;
    }
    p0 = p;

    if (flags & GL_OVERRIDE)
	*p++ = WORD_code;

    if (memop->oper_flags & Amode_Segment)
	*p++ = gen_seg_override(memop);

    if (flags & (GL_PREFIX|GL_FWAIT)) {	/* mutually exclusive */
	if (flags & GL_PREFIX)
	    *p++ = 0x0F;
	if (flags & GL_FWAIT)
	    *p++ = FWAIT_code;
    }

    *p = glp->gl_opcode;

    /* For +r, "memop" is really the register operand,
    ** because regop is a literal.
    */
    if (flags & GL_PLUS_R)
	*p += gen_regcode(memop->oper_expr);
    ++p;

    if (flags & (GL_SLASH_R|GL_SLASH_N)) {	/* mutually exclusive */
	p += gen_slashr(
		    (flags & GL_SLASH_R) ? gen_regcode(regop->oper_expr) : glp->gl_slashn,
		    memop, secp, p);
    }
    else if (flags & GL_PCREL)
	/* glp->gl_slashn contains minimum PC-rel size. */
	p += gen_pcrel(regop, secp, glp->gl_slashn, cp->code_addr+(p-p0), 1);
    else if (flags & GL_MOFFSET) {
	/* moff[8/16/32] */
	if (gen) {
	    Eval *vp = evalexpr(memop->oper_expr);

	    if (   (vp->ev_flags & EV_OFLOW) != 0
		|| vp->ev_minbit > 32
	    )
		opererror(gettxt(":989","memory operand too big"), memop);
	    else {
		if ((vp->ev_flags & EV_RELOC) != 0) {
#ifdef P5_ERR_41
		    if (P5_pseudo_expand) {
			memset(p + P5_PEEK_SZ, 'R', 4);
		    } else
#endif
		    {
			if (vp->ev_flags & EV_CONST)
			    evalcopy(vp);
			relocaddr(vp, p, secp);
		    }
		}  /* if */
		gen_value(vp, 4, p);
	    }
	}
	p += 4;
	cp->code_kind = CodeKind_FixInst;	/* inst. has no varying size */
    }
    else
	cp->code_kind = CodeKind_FixInst;	/* inst. has no varying size */


    if (flags & (GL_IMM8|GL_IMM)) {
	/* GL_IMM8 takes precedence for imposing size. */
	int size = (flags & GL_IMM8) ?
		1 : ((Inst386 *)cp->info.code_inst)->opersize;
	if (gen)
	    (void) gen_lit(regop, size, secp, p);
	p += size;
    }
#ifdef P5_ERR_41
    if (flags & GL_PREFIX) {
	/* All instructions with the 0x0F prefix that are processed through
	   this routine are 0f NOT 8x instructions.  Many have already been
	   picked up directly by the mnemonic from the i386 instruction
	   flags.  There are several mov, push, pop, and imul variations
	   which must be caught at this time. */
	cp->code_impdep |= CODE_P5_0F_NOT_8X;
    }  /* if */
#endif
    return p - p0;
}


static int
#ifdef __STDC__
sets(register const Code *cp)	/* guess at reg set by inst, if any */
#else
sets(cp)register Code *cp;
#endif
{
	Operand *op;
	Expr *ep;

	/*
	* Take a pretty simple-minded approximation at this.
	* If the last operand of an instruction is just a register,
	* assume that the instruction sets it.
	*/
	if (cp == 0 || (cp->code_kind != CodeKind_VarInst
		&& cp->code_kind != CodeKind_FixInst)
		|| (op = cp->data.code_olst->last.olst_last) == 0
		|| op->oper_flags != 0
		|| (ep = op->oper_expr)->ex_type != ExpTy_Register)
	{
		return -1;
	}
	return ep->right.ex_reg;
}

static int
#ifdef __STDC__
bases(register const Code *cp)	/* reg used as base by inst, if any */
#else
bases(cp)register Code *cp;
#endif
{
	register Operand *op;
	register Expr *ep;

	/*
	* Scan each operand of instruction.  If a base register is
	* used, return that register; otherwise -1.
	*/
	if (cp == 0 || (cp->code_kind != CodeKind_VarInst
		&& cp->code_kind != CodeKind_FixInst)
		|| (op = cp->data.code_olst->olst_first) == 0)
	{
		return -1;
	}
	do	/* for each operand */
	{
		if ((ep = op->oper_amode.base) != 0
			&& ep->ex_type == ExpTy_Register)
		{
			return ep->right.ex_reg;
		}
	} while ((op = op->oper_next) != 0);
	return -1;
}

static int
#ifdef __STDC__
padchoice(int prevsets, int nextbase)	/* return [0,5] as best fit */
#else
padchoice(prevsets, nextbase)int prevsets, nextbase;
#endif
{
	if (nextbase == Reg_edi)
	{
		if (prevsets == Reg_esi)
			return 0;
		return 1;
	}
	else if (nextbase == Reg_esi)
	{
		if (prevsets == Reg_edi)
			return 2;
		return 3;
	}
	else
	{
		if (prevsets == Reg_esi)
			return 4;
		return 5;
	}
}


/*
** This routine generates no-operation code to fill space that results from
** a .align, .backalign, or .set ., xxx directive.
**
** The encodings are chosen based on three factors:
**  1. the instructions that immediately precede and follow the padding,
**	(rset and rbase guess at the appropriate registers)
**  2. whether the target is a 486 or a 386, and
**	(assumes a 486)
**  3. whether the padding is permitted to modify the condition codes.
**	(can be modified only if secp->sec_impdep is set)
**
** The backbone of the do-nothing instructions are different versions of
** leal 0(%reg),%reg.  There are 3, 4, 6, and 7 byte encodings available.
** The registers %eax, %esi, and %edi are used, generally with a strong
** preference for %edi and %esi over %eax.  The choice among the various
** instructions is governed by 1. above; it is implemented by padchoice().
**
** Each instruction takes 1 clock on the 486, assuming no interlock due
** to use of a just-set register as a base.  Each instruction takes 2
** clocks on the 386 except for nop which takes 3.
**/
#define L3eax	0x8d, 0x40, 0			/* 3 byte: reg+disp8 */
#define L3esi	0x8d, 0x76, 0
#define L3edi	0x8d, 0x7f, 0
#define L4eax	0x8d, 0x44, 0x20, 0		/* 4 byte: modR/M+SIB+disp8 */
#define L4esi	0x8d, 0x74, 0x26, 0
#define L4edi	0x8d, 0x7c, 0x27, 0
#define L6eax	0x8d, 0x80, 0, 0, 0, 0		/* 6 byte: reg+disp32 */
#define L6esi	0x8d, 0xb6, 0, 0, 0, 0
#define L6edi	0x8d, 0xbf, 0, 0, 0, 0
#define L7eax	0x8d, 0x84, 0x20, 0, 0, 0, 0	/* 7 byte: modR/M+SIB+disp32 */
#define L7esi	0x8d, 0xb4, 0x26, 0, 0, 0, 0
#define L7edi	0x8d, 0xbc, 0x27, 0, 0, 0, 0

void
#ifdef __STDC__
gennops(Section *secp, const Code *cp, const Code *prev) /* generate nops */
#else
gennops(secp, cp, prev)Section *secp; Code *cp, *prev;
#endif
{
	static const Uchar lea3[6][3] =
	{
		{L3eax},	{L3esi},	/* next base is %edi */
		{L3eax},	{L3edi},	/* next base is %esi */
		{L3edi},	{L3esi},	/* otherwise */
	};
	static const Uchar lea4[6][4] =
	{
		{L4eax},	{L4esi},	/* next base is %edi */
		{L4eax},	{L4edi},	/* next base is %esi */
		{L4edi},	{L4esi},	/* otherwise */
	};
	static const Uchar lea6[6][6] =
	{
		{L6eax},	{L6esi},	/* next base is %edi */
		{L6eax},	{L6edi},	/* next base is %esi */
		{L6edi},	{L6esi},	/* otherwise */
	};
	static const Uchar lea7[6][7] =
	{
		{L7eax},	{L7esi},	/* next base is %edi */
		{L7eax},	{L7edi},	/* next base is %esi */
		{L7edi},	{L7esi},	/* otherwise */
	};
	static const Uchar lea8[6][8] =
	{
		{L4edi, L4esi},	{L4esi, L4eax},	/* next base is %edi */
		{L4esi, L4edi},	{L4edi, L4eax},	/* next base is %esi */
		{L4edi, L4esi},	{L4esi, L4edi},	/* otherwise */
	};
	static const Uchar lea10[6][10] =
	{
		{L3edi, L7esi},	{L3esi, L7eax},	/* next base is %edi */
		{L3esi, L7edi},	{L3edi, L7eax},	/* next base is %esi */
		{L3edi, L7esi},	{L3esi, L7edi},	/* otherwise */
	};
	static const Uchar lea11[6][11] =
	{
		{L4edi, L7esi},	{L4esi, L7eax},	/* next base is %edi */
		{L4esi, L7edi},	{L4edi, L7eax},	/* next base is %esi */
		{L4edi, L7esi},	{L4esi, L7edi},	/* otherwise */
	};
	static const Uchar lea12[6][12] =
	{
		{L6edi, L6esi},	{L6esi, L6eax},	/* next base is %edi */
		{L6esi, L6edi},	{L6edi, L6eax},	/* next base is %esi */
		{L6edi, L6esi},	{L6esi, L6edi},	/* otherwise */
	};
	static const Uchar lea13[6][13] =
	{
		{L6edi, L7esi},	{L6esi, L7eax},	/* next base is %edi */
		{L6esi, L7edi},	{L6edi, L7eax},	/* next base is %esi */
		{L6edi, L7esi},	{L6esi, L7edi},	/* otherwise */
	};
	static const Uchar lea14[6][14] =
	{
		{L7edi, L7esi},	{L7esi, L7eax},	/* next base is %edi */
		{L7esi, L7edi},	{L7edi, L7eax},	/* next base is %esi */
		{L7edi, L7esi},	{L7esi, L7edi},	/* otherwise */
	};
	register Ulong sz;
	register Uchar *p;
	register const Uchar *q;
	register int rset, rbase;
	int	 can_chg_cc;

	if ((sz = cp->data.code_skip) == 0)	/* nothing to do */
		return;
#ifdef P5_ERR_41
	can_chg_cc = (secp->sec_impdep != 0)
			&& ((cp->code_impdep & CODE_NO_CC_PAD) == 0);
#else
	can_chg_cc = (secp->sec_impdep != 0)
#endif
	p = secp->sec_data + cp->code_addr;
	rset = sets(prev);
	rbase = bases(cp->code_next);	/* only used for final bytes */
	for (;;)
	{
		Ulong chunk;

		if ((chunk = sz) > 15)
			chunk = 14;	/* longest best fit */
		switch (chunk)
		{
		case 1:
			if (proc_type == ProcType_386 && can_chg_cc != 0)
				p[0] = 0xf8;	/* clc */
			else
				p[0] = 0x90;	/* nop */
			break;
		case 2:
			if (can_chg_cc != 0)	/* cmpl %edi,%edi */
			{
				p[0] = 0x3b;
				p[1] = 0xff;
			}
			else if (rbase == Reg_edi)	/* movl %esi,%esi */
			{
				p[0] = 0x8b;
				p[1] = 0xf6;
			}
			else	/* movl %edi,%edi */
			{
				p[0] = 0x8b;
				p[1] = 0xff;
			}
			break;
		case 3:
			if (can_chg_cc != 0)
			{
			modcc3:;
				p[0] = 0x83;
				if (rbase == Reg_edi)
					p[1] = 0xc6;	/* addl $0,%esi */
				else
					p[1] = 0xc7;	/* addl $0,%edi */
				p[2] = 0;
				break;
			}
		fill3:;
			q = &lea3[padchoice(rset, rbase)][0];
			goto copy3;
		case 4:
			q = &lea4[padchoice(rset, rbase)][0];
			goto copy4;
		case 5:
			if (can_chg_cc != 0)	/* cmpl $0,%eax */
			{
			modcc5:;
				p[0] = 0x3d;
				p[1] = 0;
				p[2] = 0;
				p[3] = 0;
				p[4] = 0;
				break;
			}
			p[0] = 0x8b;	/* movl %eax,%eax */
			p[1] = 0xc0;
			p += 2;
			rset = Reg_eax;
			goto fill3;
		case 6:
			if (can_chg_cc != 0)	/* cmpl $0,%edi */
			{
			modcc6:;
				p[0] = 0x81;
				p[1] = 0xff;
				p[2] = 0;
				p[3] = 0;
				p[4] = 0;
				p[5] = 0;
				break;
			}
			q = &lea6[padchoice(rset, rbase)][0];
			goto copy6;
		case 7:
		fill7:;
			q = &lea7[padchoice(rset, rbase)][0];
			goto copy7;
		case 8:
			if (can_chg_cc != 0)	/* 2+6 */
			{
				p[0] = 0x3b;	/* cmpl %edi,%edi */
				p[1] = 0xff;
				p += 2;
				goto modcc6;
			}
			else if (proc_type == ProcType_386)
			{
				q = &lea8[padchoice(rset, rbase)][0];
				goto copy8;
			}
			*p++ = 0x90;	/* nop */
			rset = Reg_eax;
			goto fill7;
		case 9:
			if (can_chg_cc != 0)	/* 6+3 */
			{
				p[0] = 0x81;
				p[1] = 0xff;
				p[2] = 0;
				p[3] = 0;
				p[4] = 0;
				p[5] = 0;
				p += 6;
				goto modcc3;
			}
			p[0] = 0x8b;	/* movl %eax,%eax */
			p[1] = 0xc0;
			p += 2;
			rset = Reg_eax;
			goto fill7;
		case 10:
			if (can_chg_cc != 0)	/* 5+5 */
			{
			modcc10:;
				p[0] = 0x3d;	/* cmpl $0,%eax */
				p[1] = 0;
				p[2] = 0;
				p[3] = 0;
				p[4] = 0;
				p += 5;
				goto modcc5;
			}
			q = &lea10[padchoice(rset, rbase)][0];
			goto copy10;
		case 11:
			if (can_chg_cc != 0)	/* 6+5 */
			{
				p[0] = 0x81;	/* cmpl $0,%edi */
				p[1] = 0xff;
				p[2] = 0;
				p[3] = 0;
				p[4] = 0;
				p[5] = 0;
				p += 6;
				goto modcc5;
			}
			q = &lea11[padchoice(rset, rbase)][0];
			goto copy11;
		case 12:
			if (can_chg_cc != 0)	/* 6+6 */
			{
				p[0] = 0x81;	/* cmpl $0,%edi */
				p[1] = 0xff;
				p[2] = 0;
				p[3] = 0;
				p[4] = 0;
				p[5] = 0;
				p += 6;
				goto modcc6;
			}
			q = &lea12[padchoice(rset, rbase)][0];
			goto copy12;
		case 13:
			if (can_chg_cc != 0)	/* 6+7 */
			{
				p[0] = 0x81;	/* cmpl $0,%edi */
				p[1] = 0xff;
				p[2] = 0;
				p[3] = 0;
				p[4] = 0;
				p[5] = 0;
				p += 6;
				rset = Reg_eax;
				goto fill7;
			}
		fill13:;
			q = &lea13[padchoice(rset, rbase)][0];
			goto copy13;
		case 14:
			if (sz > 14)	/* will be more to fill */
			{
				if (rset == Reg_edi
					|| rset < 0 && rbase == Reg_edi)
				{
					q = &lea14[1][0];
				}
				else
					q = &lea14[3][0];
				rset = Reg_eax;
				goto copy14;
			}
		fill14:;
			q = &lea14[padchoice(rset, rbase)][0];
			goto copy14;
		case 15:
			if (can_chg_cc != 0)	/* 5+5+5 */
			{
				p[0] = 0x3d;	/* cmpl $0,%eax */
				p[1] = 0;
				p[2] = 0;
				p[3] = 0;
				p[4] = 0;
				p += 5;
				goto modcc10;
			}
			if (proc_type == ProcType_386)
			{
				p[0] = 0x8b;	/* movl %eax,%eax */
				p[1] = 0xc0;
				p += 2;
				rset = Reg_eax;
				goto fill13;
			}
			*p++ = 0x90;	/* nop */
			rset = Reg_eax;
			goto fill14;
			/*
			* Tail-merged copying from the lea tables.
			*/
		copy14:;
			p[13] = q[13];
		copy13:;
			p[12] = q[12];
		copy12:;
			p[11] = q[11];
		copy11:;
			p[10] = q[10];
		copy10:;
			p[9] = q[9];
			p[8] = q[8];
		copy8:;
			p[7] = q[7];
		copy7:;
			p[6] = q[6];
		copy6:;
			p[5] = q[5];
			p[4] = q[4];
		copy4:;
			p[3] = q[3];
		copy3:;
			p[2] = q[2];
			p[1] = q[1];
			p[0] = q[0];
			break;
		}
		if ((sz -= chunk) == 0)
			return;
		p += chunk;
	}
}


/* Useful service routines. */


/* Get register number from expression. */

static int
#ifdef __STDC__
gen_getregno(Expr *ep)
#else
gen_getregno(ep) Expr *ep;
#endif
{
    if (ep == 0)
	fatal(gettxt(":990","gen_getregno():no expression"));
    if (ep->ex_op != ExpOp_LeafRegister)
	fatal(gettxt(":991","gen_getregno():not a register"));
    return( ep->right.ex_reg );
}

    
/* Return segment override byte for operand. */

static Uchar
#ifdef __STDC__
gen_seg_override(const Operand *op)
#else
gen_seg_override(op) Operand *op;
#endif
{
    if ((op->oper_flags & Amode_Segment) == 0)
	fatal(gettxt(":1468","gen_seg_override():no segment"));
    
    return seg_reg[gen_regcode(op->oper_amode.seg)];
}


/* Generate encoding for a value. */
void
#ifdef __STDC__
gen_value(Eval *vp, int size, Uchar *p)
#else
gen_value(vp, size, p) Eval *vp; int size; Uchar *p;
#endif
{
    Ulong value = vp->ev_ulong;

    /* Generate low-to-high order bytes in host-independent way.
    ** Assume CHAR_BIT is the same for host and target machines.
    */
    switch( size ) {
    default:	fatal(gettxt(":992","gen_value():can't handle size %d"), size);
		/*NOTREACHED*/
/* Select byte, from right to left; rely on implicit truncation. */
#define BYTE(i) ((Uchar) (value >> ((i) * CHAR_BIT)))
    case 4:	p[3] = BYTE(3);	/*FALLTHRU*/
    case 3:	p[2] = BYTE(2);	/*FALLTHRU*/
    case 2:	p[1] = BYTE(1);	/*FALLTHRU*/
    case 1:	p[0] = BYTE(0);	/*FALLTHRU*/
    }
    return;
}


/* Generate code for literal.  Return number of bytes generated.
** "size" is number of bytes for literal.  Accept either signed
** or unsigned numbers that fit the available space.
*/

static size_t
#ifdef __STDC__
gen_lit(Operand *op, int size, Section *secp, Uchar *p)
#else
gen_lit(op, size, secp, p)
Operand *op; int size; Section *secp; Uchar *p;
#endif
{
    Expr *ep;
    Eval *vp;

    if ((ep = op->oper_expr) == 0)
	fatal(gettxt(":993","gen_lit():no expr"));

    switch (ep->ex_type) {
    default: fatal(gettxt(":994","gen_lit():bad expr. type"));
	/*NOTREACHED*/
    case ExpTy_Relocatable:
    case ExpTy_Integer:
	break;
    }

    vp = evalexpr(ep);
    /* Allow value if it fits as either signed or unsigned. */
    if ((vp->ev_flags & EV_OFLOW) == 0 &&
	(vp->ev_minbit <= size * CHAR_BIT || FITLONG(vp, size * CHAR_BIT)))
	    ; /*EMPTY*/
    else
	operwarn(gettxt(":995","literal value does not fit"), op);

    if ((vp->ev_flags & EV_RELOC) != 0) {
	if (size < 4)
	    opererror(gettxt(":996","only 4-byte literal can be relocatable"), op);
	else
#ifdef P5_ERR_41
	    if (P5_pseudo_expand) {
		memset(p + P5_PEEK_SZ, 'R', 4 /* size */);
	    } else
#endif
	    {
		if (vp->ev_flags & EV_CONST)
		    evalcopy(vp);
		relocaddr(vp, p, secp);
	    }
    }
    if ((vp->ev_flags & EV_G_O_T) != 0)
	vp->ev_ulong += p-p0;
    gen_value(vp, size, p);
    return size;
}


/* Output various /r, /n forms of addressing.  For the purposes of
** this routine, one operand is considered a memory operand.  regcode
** is a register number (or alternate opcode value).  The operand
** may have an arbitrary memory (or register) addressing
** expression.  Store the encoding for output if the section's memory
** pointer is non-null.  Return the length of the whole affair in
** either case.
*/

static size_t
#ifdef __STDC__
gen_slashr(int regcode, Operand *memop, Section *secp, Uchar *p)
#else
gen_slashr(regcode, memop, secp, p)
int regcode; Operand *memop; Section *secp; Uchar *p;
#endif
{
    Expr *base = 0;
    Expr *index = 0;
    int scale = 0;
    Eval *disp = 0;
    int dispsize = 0;
    Uchar rmbyte;			/* mod r/m byte */
    Ushort sib = 0;			/* SIB byte + flag bit*/
    int varies = 0;			/* non-zero if size could vary */
    Code *cp = memop->parent.oper_olst->olst_code;

    if (memop->oper_flags & Amode_BIS) {
	base = memop->oper_amode.base;
	index = memop->oper_amode.index;
	if (memop->oper_amode.scale)
	    /* Value has already been checked */
	    scale = evalexpr(memop->oper_amode.scale)->ev_ulong;
    }
    /* Figure out how big a displacement we have. */
    if (memop->oper_expr) {
	switch( memop->oper_expr->ex_type ) {
	default: fatal(gettxt(":997","gen_slashr():bad expr type"));
	    /*NOTREACHED*/
	case ExpTy_Register:
	    dispsize = -1;		/* signal register */
	    break;
	case ExpTy_Integer:
	    disp = evalexpr(memop->oper_expr);
	    if (FITS_IN(disp, 8)) {
		dispsize = 1;
		if (disp->ev_flags & EV_LDIFF)
		    varies = 1;
		if (base != 0 && disp->ev_ulong == 0) {
		    dispsize = 0;
		    disp = 0;	/* give no displacement a try */
		}
	    }
	    else {
	disp32:;
		dispsize = 4;
		if (secp->sec_data != 0) {
		    if (disp->ev_flags & EV_OFLOW || disp->ev_minbit > 32)
			opererror(gettxt(":998","displacement too big"), memop);
		}
	    }
	    if (dispsize < (int)(cp->code_varsize & CODE_NIBBLE))
		dispsize = cp->code_varsize & CODE_NIBBLE;
	    else
		cp->code_varsize = (cp->code_varsize & ~CODE_NIBBLE) |
				   (dispsize & CODE_NIBBLE);
	    break;
	case ExpTy_Relocatable:
	    disp = evalexpr(memop->oper_expr);
	    goto disp32;
	}
    }

    if ((memop->oper_flags & Amode_BIS) == 0) {
	if (dispsize < 0) {
	    /* "Memory" operand is actually register. */
	    rmbyte = gen_regcode(memop->oper_expr) | 0xC0;
	    dispsize = 0;
	}
	else {
	    dispsize = 4;		/* pure displacement is 4 bytes */
	    rmbyte = 0x5;
	}
    }
    else {
	/* Figure out cases where we need a SIB. */
	rmbyte = base ? gen_regcode(base) : 0;

	/* 4 and 5 (%esp and %ebp) are special cases because of
	** the encoding.  %esp gets handled via a SIB byte (which
	** code we fall into).  %ebp is special when there's no
	** displacement.  Also special is the case where there's
	** an index and no base or displacement.  Create a dummy
	** 0 displacement.  This shouldn't happen very often.
	*/
	if (   disp == 0
	    && (rmbyte == 5 || (base == 0 && index != 0))
	) {
	    disp = evalexpr(ulongexpr((Ulong) 0));
	    dispsize = 1;
	}
	if (index != 0 || rmbyte == 4) {
	    /* rmbyte == 4 is special case of %esp as base.
	    ** No base == 5.
	    */
	    if (base == 0)
		rmbyte = 0x5;
	    sib = index ? (gen_regcode(index) << 3) : (0x4 << 3);
	    switch( scale ){
	    case 2:	sib |= 0x40; break;
	    case 4:	sib |= 0x80; break;
	    case 8:	sib |= 0xC0; break;
	    }
	    sib |= rmbyte | 0x100;	/* so byte is never zero */
	    rmbyte = 0x4;		/* new value */
	}
	/* An index without a base always takes a disp32 and has
	** MOD == 0.
	*/
	if (index != 0 && base == 0)
	    dispsize = 4;
	else if (dispsize)
	    rmbyte |= (dispsize == 1 ? 0x40 : 0x80);
    }

    if (secp->sec_data) {
	*p++ = rmbyte | regcode << 3;	/* add in /r part */
	if (sib)
	    *p++ = sib;
	if (disp) {
	    if ((disp->ev_flags & EV_RELOC) != 0) {
#ifdef P5_ERR_41
		if (P5_pseudo_expand) {
		    memset(p + P5_PEEK_SZ, 'R', dispsize);
		} else
#endif
		{
		    if (disp->ev_flags & EV_CONST)
			evalcopy(disp);
		    relocaddr(disp, p, secp);
		}
	    }  /* if */
            if ((disp->ev_flags & EV_G_O_T) != 0)
                disp->ev_ulong += p-p0;
	    gen_value(disp, dispsize, p);
	}
    }
    if (! varies)
	cp->code_kind = CodeKind_FixInst;
    return( 1 + (sib != 0) + dispsize );
}


/* Generate code for PC-relative operand.  PC-relative instructions
** have effective addresses relative to their end.  "offset" is the
** offset in the section (secp) of the start of the PC-relative part.
** "size" is the requested PC-relative size in bytes, presumably 
** 1 or 4.  If "gen" is zero, don't actually generate any code.
*/
static size_t
#ifdef __STDC__
gen_pcrel(Operand *op, Section *secp, int size, size_t offset, int gen)
#else
gen_pcrel(op, secp, size, offset, gen)
Operand *op; Section *secp; int size; size_t offset; int gen;
#endif
{
    Eval *vp;
    int ispcrel = 1;			/* assume jump is PC-relative */
    Uchar *p;
    Code *cp = op->parent.oper_olst->olst_code;

    if (size < (int) (cp->code_varsize & CODE_NIBBLE))
	size = cp->code_varsize & CODE_NIBBLE;	/* Don't let operand size shrink. */

    /* Determine the size of the PC-relative part.
    ** If the operand isn't a constant, or if it's in another
    ** section, or if it uses one of the PIC relocation types, we
    ** always require a 4-byte displacement.  The displacement is
    ** calculated relative to the end of the PC-relative part.
    */
    vp = evalexpr(op->oper_expr);
    if (vp->ev_flags & EV_CONST)
	evalcopy(vp);
    if (   (vp->ev_flags & EV_RELOC) == 0
	|| vp->ev_sec != secp
	|| vp->ev_pic != 0
	|| ((flags & ASFLAG_KRELOC) != 0 && 
		(cp->info.code_inst->chkflags & IF_KRELOC) != 0)
    ) {
	ispcrel = 0;
	/* Integer offset value must be adjusted to reflect relocation
	** relative to '.', rather than '.+4'.
	*/
	subeval(vp, (Ulong) 4);
	size = 4;
    }
    else {
	/* PC-relative displacement.  Figure out if a small
	** displacement will work.
	*/
	subeval(vp, (Ulong) offset + size);
	if (vp->ev_nsbit > (size * CHAR_BIT)) {
	    /* Reevaluate expression, assuming large size, which changes
	    ** displacement value.
	    */
	    subeval(vp, (Ulong) 4 - size);
	    size = 4;
	}
    }

    if ((vp->ev_flags & EV_OFLOW) != 0 || vp->ev_nsbit > 32)
	opererror(gettxt(":999","branch target too distant"), op);

    if ((p = secp->sec_data) != 0 && gen) {
	/* Produce code and suitable relocation type. */
	p += offset;

        if (! ispcrel) {
#ifdef P5_ERR_41
            if (P5_pseudo_expand) {
                memset(p + P5_PEEK_SZ, 'R', size);
            } else
#endif
                relocpcrel(vp, p, secp);
        }  /* if */
	gen_value(vp, size, p);
    }
    /* Instruction won't change size if it's big now. */
    if (size == 4)
	cp->code_kind = CodeKind_FixInst;

    cp->code_varsize = (cp->code_varsize & ~CODE_NIBBLE) |
			(size & CODE_NIBBLE);/* remember size of variable portion */
    return size;
}


/* Generate code for long jmp to "target". */

static size_t
#ifdef __STDC__
gen_dojmp(Operand *target, Section *secp, Ulong off, int gen)
#else
gen_dojmp(target, secp, off, gen)
Operand *target; Section *secp; Ulong off; int gen;
#endif
{
    int opersize = gen_pcrel(target, secp, 1, off+1, 0);
    Uchar *p;

    if ((p = secp->sec_data) != 0 && gen) {
	p[off] = (opersize == 1 ? JMP8_code : 0xE9);
	(void) gen_pcrel(target, secp, opersize, off+1, 1);
    }
    return( opersize + 1 );
}


/* Generate code for clr variants.  gen_list() can handle
** register cases.  For memory cases, produce a suitable
** literal after using gen_list().
*/
size_t
#ifdef __STDC__
gen_clr(Section *secp, Code *cp)
#else
gen_clr(secp, cp) Section *secp; Code *cp;
#endif
{
    int instoff = gen_list(secp, cp);

    if (cp->data.code_olst->olst_combo >= clr_mem) {
	Eval *vp = evalexpr(ulongexpr((Ulong) 0));	/* create a zero */
	int opersize = ((Inst386 *)cp->info.code_inst)->opersize;

	if (secp->sec_data != 0)
	    gen_value(vp, opersize, secp->sec_data + cp->code_addr + instoff);
	instoff += opersize;
    }
    return instoff;
}


/* Generate code for enter $iw,$ib */

size_t
#ifdef __STDC__
gen_ent(Section *secp, Code *cp)
#else
gen_ent(secp, cp) Section *secp; Code *cp;
#endif
{
    Uchar *p;
    Operand *left = cp->data.code_olst->olst_first;
    Operand *right = left->oper_next;

    if ((p = secp->sec_data) == 0)
	fatal(gettxt(":1000","gen_ent():called for size"));
    
    p += cp->code_addr;

    *p++ = 0xC8;
    p += gen_lit(left, 2, secp, p);
    (void) gen_lit(right, 1, secp, p);
    return 4;				/* instruction size */
}


/* Check fxch.  There are 0- and 1-operand forms.  For the
** latter, use the standard check routine.
** chk_list() has not been called before we get here.
*/

void
#ifdef __STDC__
chk_fxch(Code *cp)
#else
chk_fxch(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;

    if (olp->olst_first != 0)
	chk_list(cp, cl_fld0);
    return;
}


/* Generate code for fxch.  If there's an operand, use gen_list().
** Otherwise (and this is painful), put out the explicit bits for
** fxch	%st(1).
*/

size_t
#ifdef __STDC__
gen_fxch(Section *secp, Code *cp)
#else
gen_fxch(secp, cp) Section *secp; Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Uchar *p;

    cp->code_kind = CodeKind_FixInst;	/* it's always a fixed size */

    if (olp->olst_first != 0)
	return gen_list(secp, cp);
    if ((p = secp->sec_data) != 0) {
	p += cp->code_addr;

	p[0] = 0xD9;
	p[1] = 0xC8 + 1;
    }
    return 2;
}


/* Check imul forms.  The idea is to recognize the form with
** the immediate operand and run the check on a different set
** of operands after removing the immediate.
** chk_list() has not been called before we get here.
*/

void
#ifdef __STDC__
chk_imul(Code *cp)
#else
chk_imul(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *first = olp->olst_first;

    if (first->oper_info != OC_LIT) {
	/* Simpler case:  just do the ordinary chk_list. */
	/* Only literal case can have 3 operands. */
	if (first->oper_next != 0 && first->oper_next->oper_next != 0)
	    invalidoper(cp);
	else
	    chk_list(cp, cl_imul);
    }
    else if (first->oper_next == 0)
	invalidoper(cp);		/* literal cases have more operands */
    else {
	Ushort combo;
	Eval *vp;

	/* Temporarily remove the first operand, check against
	** forms that take one or two more operands, then restore
	** operand list.
	*/
	olp->olst_first = first->oper_next;
	chk_list(cp, &cl_imul[imul_imm8]);
	olp->olst_first = first;

	/* Get "combo" offset from start of entire cl_imul list. */
	combo = olp->olst_combo + imul_imm8-0;

	vp = evalexpr(first->oper_expr);

	/* Use large-form literal if value may change size or is
	** too big now.
	*/
	if (! ALWAYS_FITS_IN(vp, 8))
	    combo += imul_imm - imul_imm8;
	olp->olst_combo = combo;
    }
    return;
}


/* Generate code for imul.  For combinations that don't involve
** an immediate, just generate the code.  Otherwise, remove the
** immediate operand (as in chk_imul), generate the code,
** generate the immediate operand, the restore the immediate.
*/

size_t
#ifdef __STDC__
gen_imul(Section *secp, Code *cp)
#else
gen_imul(secp, cp) Section *secp; Code *cp;
#endif
{
    Ushort combo = cp->data.code_olst->olst_combo;
    int instoff;

    if (combo < imul_imm8)
	instoff = gen_list(secp, cp);
    else {
	Oplist *olp = cp->data.code_olst;
	Operand *first = olp->olst_first;
	int size;

	olp->olst_first = first->oper_next;
	instoff = gen_list(secp, cp);		/* generate most of inst. */
	olp->olst_first = first;		/* restore operand list */
	if (combo < imul_imm)
	    size = 1;				/* small form */
	else
	    size = ((Inst386 *) cp->info.code_inst)->opersize;
	if (secp->sec_data)
	    (void) gen_lit(first, size, secp,
			secp->sec_data + cp->code_addr + instoff);
	instoff += size;
    }
    return instoff;
}


/* Check unconditional jump and call.  A '*' is required if we're
** doing any base/index/displacement addressing or if the operand
** is a register.
*/

void
#ifdef __STDC__
chk_jmp(Code *cp)
#else
chk_jmp(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *op = olp->olst_first;

    if (   (op->oper_flags & Amode_Indirect) == 0
	&& (
		(op->oper_flags & Amode_BIS) != 0
	    ||  (op->oper_expr->ex_type == ExpTy_Register)
	    )
    ) {
	operwarn(gettxt(":1001","'*' required for address mode"), op);
	op->oper_flags |= Amode_Indirect;
    }
    /* If the expression doesn't look like something that can be
    ** handled by PC-relative addressing, use straight mod r/m mode.
    */
    if (   olp->olst_combo == jmp_pcr8
	&& (op->oper_flags & (Amode_BIS|Amode_Indirect)) != 0
    )
	olp->olst_combo = jmp_mem;
    return;
}


/* Generate code for jmp. */
size_t
#ifdef __STDC__
gen_jmp(Section *secp, Code *cp)
#else
gen_jmp(secp, cp) Section *secp; Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *dst = olp->olst_first;
    int instoff;

    /* If we still think we can use a 1-byte PC-relative, check and
    ** possibly make it 4-byte.
    */
    if (olp->olst_combo == jmp_pcr8) {
	/* addr+1 for opcode */
	if (gen_pcrel(dst, secp, 1, cp->code_addr+1, 0) > 1)
	    olp->olst_combo = jmp_pcr32;
    }
    instoff = gen_list(secp, cp);

#ifdef OLD_AS_COMPAT
    /* Old assembler put NOP's after jmp's with 4-byte PC-relative address. */
    if (olp->olst_combo == jmp_pcr32) {
	Uchar *p;

	if ((p = secp->sec_data) != 0)
	    p[cp->code_addr+instoff] = NOP_code;
	++instoff;
    }
#endif
    return instoff;
}


/* Check long unconditional jump and call.  A '*' is required if we're
** doing any base/index/displacement addressing or if the operand
** is a register, but only for the r/m operand case (not two literals).
*/

/*ARGSUSED*/
void
#ifdef __STDC__
chk_ljmp(Code *cp)
#else
chk_ljmp(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *op = olp->olst_first;

    if (   olp->olst_combo >= cl_ljmp_rm
        && (op->oper_flags & Amode_Indirect) == 0
	&& (
		(op->oper_flags & Amode_BIS) != 0
	    ||  (op->oper_expr->ex_type == ExpTy_Register)
	    )
    ) {
	operwarn(gettxt(":1001","'*' required for address mode"), op);
	op->oper_flags |= Amode_Indirect;
    }
    return;
}


/* Generate code for ljmp/lcall.  Combination 0 is special:
** two literals.
*/

size_t
#ifdef __STDC__
gen_ljmp(Section *secp, Code *cp)
#else
gen_ljmp(secp, cp) Section *secp; Code *cp;
#endif
{
    Uchar *p;

    if (cp->data.code_olst->olst_combo >= cl_ljmp_rm)
	return gen_list(secp,cp);
    
    /* Two literals case. */
    if ((p = secp->sec_data) != 0) {
	Operand *op = cp->data.code_olst->olst_first;
	p += cp->code_addr;

	*p++ = ((Inst386 *)cp->info.code_inst)->code[0];
	p += gen_lit(op->oper_next, 4, secp, p);
	(void) gen_lit(op, 2, secp, p);
    }
    cp->code_kind = CodeKind_FixInst;	/* size completely known */
    return 1 + 4 + 2;
}



/* Generate code for instructions that take no operands.  These
** have fixed instruction encodings, which we can just plop down
** in the section.
*/

size_t
#ifdef __STDC__
gen_nop(Section *secp, Code *cp)
#else
gen_nop(secp, cp) Section *secp; Code *cp;
#endif
{
    int nbytes = cp->info.code_inst->inst_minsz;
    Uchar *ip = ((Inst386 *) cp->info.code_inst)->code;
    Uchar *p;

    if ((p = secp->sec_data) == 0)
	fatal(gettxt(":1002","gen_nop():called to get size"));

    p += cp->code_addr;

    p[0] = ip[0];
    if (nbytes > 1) {
	p[1] = ip[1];
	if (nbytes > 2)
	    p[2] = ip[2];
    }
    return nbytes;
}

/* Generate code for cmov instruction. These have the form
 * of a prefix override, opcode followed by a mod R/M byte
 * indicating the registers involved.  It treats the right
 * operand as memory and the left operand as register.
 * (GL_MEMRIGHT).
*/

size_t
#ifdef __STDC__
gen_cmov(Section *secp, Code *cp)
#else
gen_cmov(secp, cp) Section *secp; Code *cp;
#endif
{
    Inst386 *ip3 = (Inst386 *) cp->info.code_inst;
    int nbytes = cp->info.code_inst->inst_minsz;
    Uchar *ip = ((Inst386 *) cp->info.code_inst)->code;
    Operand *regop;
    Operand *memop;
    Uchar *p,*p0;
    Uchar dummy[5];	/* in case not really generating */

    if ((p = secp->sec_data) == 0)
		p = &dummy[0];
    else
		p += cp->code_addr;

    p0 = p;
    if (ip3->opersize == 2)
		*p++ = WORD_code;

    *p++ = ip3->code[0];
    *p++ = ip3->code[1];
    memop = cp->data.code_olst->olst_first;
    regop = memop->oper_next;
    p += gen_slashr(gen_regcode(regop->oper_expr),memop,secp,p);

    return(p - p0);
}


/* Generate code for instructions that optionally take no operands.  These
** have fixed instruction encodings, which we can just plop down
** in the section.
*/

size_t
#ifdef __STDC__
gen_optnop(Section *secp, Code *cp)
#else
gen_optnop(secp, cp) Section *secp; Code *cp;
#endif
{
    int nbytes = cp->info.code_inst->inst_minsz;
    Uchar *ip = ((Inst386 *) cp->info.code_inst)->code;
    Uchar *p;

    /* First check if the instruction has any operands.  If not, the encoding
     * and size comes from instree and is handled as a simple case.
     */
    if (cp->data.code_olst->olst_first == 0) {
	if ((p = secp->sec_data) == 0)
	    cp->code_kind = CodeKind_FixInst;
	else {
	    p += cp->code_addr;
	    p[0] = ip[0];
	    if (nbytes > 1) {
		p[1] = ip[1];
		if (nbytes > 2)
		    p[2] = ip[2];
	    }
	}
	return nbytes;
    }
    return gen_list(secp,cp);	/* must have operands this time */
}


/* Check correctness of PC-relative branches. */

void
#ifdef __STDC__
chk_pcr(Code *cp)
#else
chk_pcr(cp) Code *cp;
#endif
{
    Operand *first = cp->data.code_olst->olst_first;

    if (first->oper_flags & Amode_Segment)
	operwarn(gettxt(":1003","segment override ineffective with conditional branch"), first);

    /* Allow any memory operand that doesn't have indirection or
    ** base/index/scale.
    */
    if (   first->oper_info != OC_MEM
	|| (first->oper_flags & (Amode_Indirect|Amode_BIS)) != 0
	)
	invalidoper(cp);
    return;
}


/* Generate code for most PC-relative instructions.  This group all
** has a short (byte) and long (32-bit) displacement form.  The short
** form code is in the instruction table, and the long form requires
** an escape byte, followed by the short form + 0x10.
*/

size_t
#ifdef __STDC__
gen_pcr(Section *secp, Code *cp)
#else
gen_pcr(secp, cp) Section *secp; Code *cp;
#endif
{
    Uchar *p;
    Operand *target = cp->data.code_olst->olst_first;
    Uchar opcode = ((Inst386 *) cp->info.code_inst)->code[0];
    int instoff = 0;
    int opersize;

    if ((p = secp->sec_data) != 0)
	p += cp->code_addr;

#if 0	/* segment override has no effect */
    if ((target->oper_flags & Amode_Segment) != 0 && p)
	p[instoff++] = gen_seg_override(target, p);
#endif

    /* Get size of operand; minimum size is 1 byte.  Allow for 1-byte
    ** opcode.  Don't generate anything yet.
    */
    opersize = gen_pcrel(target, secp, 1, cp->code_addr+instoff+1, 0);

    /* Now lay down the code. */
    if (p) {
	if (opersize == 1) {
	    p[instoff] = opcode;
	    ++instoff;
	}
	else {
	    p[instoff+0] = 0x0F;
	    p[instoff+1] = opcode + 0x10;
	    instoff += 2;
	}
        instoff += gen_pcrel(target, secp, opersize, instoff + cp->code_addr, 1);
    }
    else
	instoff += 1 + (opersize != 1) + opersize;
    return( instoff );
}

/* Generate code for PC-relative instructions that only have a byte
** displacement.  The instruction code is in the table.
** The long form is:
**		INST <lab0>
**		jmp <lab1>
**	<lab0:>
**		jmp dest
**	<lab1:>
*/

size_t
#ifdef __STDC__
gen_pc8(Section *secp, Code *cp)
#else
gen_pc8(secp, cp) Section *secp; Code *cp;
#endif
{
    Uchar *p;
    Operand *target = cp->data.code_olst->olst_first;
    Uchar opcode = ((Inst386 *) cp->info.code_inst)->code[0];
    int instoff = 0;
    int opersize;
    Uchar jmplen;

    if ((p = secp->sec_data) != 0)
	p += cp->code_addr;

#if 0	/* segment override has no effect */
    if ((target->oper_flags & Amode_Segment) != 0 && p)
	p[instoff++] = gen_seg_override(target, p);
#endif

    /* Get size of operand; minimum size is 1 byte.  Allow for 1-byte
    ** opcode.  Don't generate anything yet.
    */
    opersize = gen_pcrel(target, secp, 1, cp->code_addr+instoff+1, 0);

    if (opersize == 1) {
	/* Easy case:  short form. */
	if (p)
	    p[instoff] = opcode;
	++instoff;

	instoff +=
	    gen_pcrel(target, secp, opersize, instoff + cp->code_addr, p ? 1 : 0);
    }
    else {
	/* need jmp around jmp around jmp */
	instoff += 4;
	jmplen =
	    gen_dojmp(target, secp, instoff + cp->code_addr, p ? 1 : 0);
	if (p) {
	    p[instoff-4] = opcode;
	    p[instoff-3] = 2;
	    p[instoff-2] = JMP8_code;
	    p[instoff-1] = jmplen;
#ifdef OLD_AS_COMPAT
	    p[instoff+jmplen] = NOP_code;
	}
	++instoff;
#else
	}
#endif
	instoff += jmplen;
    }
    return( instoff );
}


/* Check ret instruction.
** chk_list() has not been called for this instruction.
*/

void
#ifdef __STDC__
chk_ret(Code *cp)
#else
chk_ret(cp) Code *cp;
#endif
{
    Operand *op = cp->data.code_olst->olst_first;

    if (op && op->oper_info != OC_LIT)
	invalidoper(cp);
    return;
}



/* Generate code for "ret".  Depends on whether there's an
** operand.  With-operand form is opcode from table - 1.
*/

size_t
#ifdef __STDC__
gen_ret(Section *secp, Code *cp)
#else
gen_ret(secp, cp) Section *secp; Code *cp;
#endif
{
    Operand *op = cp->data.code_olst->olst_first;
    Uchar *p;

    cp->code_kind = CodeKind_FixInst;	/* size is fixed, regardless */

    if ((p = secp->sec_data) != 0)
	p += cp->code_addr;

    if (op == 0) {
	if (p)
	    p[0] = ((Inst386 *)cp->info.code_inst)->code[0];
	return 1;
    }
    /* Case when there's a literal. */
    if (p) {
	*p++ = ((Inst386 *)cp->info.code_inst)->code[0] - 1;
	(void) gen_lit(op, 2, secp, p);
    }
    return 3;
}


/* Check double-shift instructions.  If there are three operands,
** the first must be a literal.  The assembler syntax implies that
** the two-operand form has an implicit %cl.
** chk_list() has not been called for these instructions.
*/

void
#ifdef __STDC__
chk_shxd(Code *cp)
#else
chk_shxd(cp) Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *first = olp->olst_first;
    int imm_form = 0;			/* 1 if $imm8 form */

    if (first->oper_next->oper_next != 0) {
	imm_form = 1;
	if (first->oper_info != OC_LIT)
	    invalidoper(cp);
    }

    /* If $imm8 form, strip first operand, check others, restore. */
    if (imm_form)
	olp->olst_first = first->oper_next;
    chk_list(cp, cl_shxd);
    if (imm_form) {
	olp->olst_first = first;
	/* Choose different encoding for $imm8 form. */
	olp->olst_combo += cl_shxd_imm8 - 0;
    }
    return;
}


/* Generate code for double-size shift.  If 3-operand form,
** strip off first operand temporarily, use gen_list(),
** generate appropriate code for $imm8 if necessary.
*/

size_t
#ifdef __STDC__
gen_shxd(Section *secp, Code *cp)
#else
gen_shxd(secp, cp) Section *secp; Code *cp;
#endif
{
    Oplist *olp = cp->data.code_olst;
    Operand *first = olp->olst_first;
    int imm_form = olp->olst_combo >= cl_shxd_imm8;
    int instoff;
    
    if (imm_form)
	olp->olst_first = first->oper_next;
    instoff = gen_list(secp, cp);
    if (imm_form) {	/* is $imm8 form */
	Uchar *p;

	olp->olst_first = first;
	if ((p = secp->sec_data) != 0) {
	    p += cp->code_addr + instoff;
	    (void) gen_lit(first, 1, secp, p);
	}
	++instoff;
    }
    return instoff;
}


/* Check string instructions:  movs/smov/slod/scmp/scas/ssto/ins/outs.
** If there's an operand, it must be a segment register.
** Turn the apparent register operand into one that just
** has a segment override (and nothing else).  This allows
** us to use gen_segment_override later.
*/

void
#ifdef __STDC__
chk_str(Code *cp)
#else
chk_str(cp) Code *cp;
#endif
{
    Operand *op = cp->data.code_olst->olst_first;

    if (op->oper_info != OC_SEG)
	invalidoper(cp);
    else {
	/* Move the segment register into the segment override position.
	** That way we can use gen_seg_override() ultimately.
	*/
	if (cp->info.code_inst->chkflags & IF_NOSEG)
	    operwarn(gettxt(":1004","segment override ineffective for instruction"), op);
	op->oper_flags |= Amode_Segment;
	op->oper_amode.seg = op->oper_expr;
    }
    return;
}


/* Generate code for string instructions:
** movs/smov/slod/scmp/scas/ssto/ins/outs.
** There may be a segment override, a size override, and an opcode.
*/

size_t
#ifdef __STDC__
gen_str(Section *secp, Code *cp)
#else
gen_str(secp, cp) Section *secp; Code *cp;
#endif
{
    Inst386 *ip3 = (Inst386 *) cp->info.code_inst;
    Operand *op = cp->data.code_olst->olst_first;
    Uchar *p, *pbeg;
    Uchar dummy[3];			/* in case not really generating */

    if ((p = secp->sec_data) == 0)
	p = &dummy[0];
    else
	p += cp->code_addr;
    pbeg = p;

    cp->code_kind = CodeKind_FixInst;	/* fixed size hereafter */

    if (op && (op->oper_flags & Amode_Segment))
	*p++ = gen_seg_override(op);
    
    if (ip3->opersize == 2)
	*p++ = WORD_code;
    *p++ = ip3->code[0];

    return p - pbeg;
}

#ifdef P5_ERR_41
void
#ifdef __STDC__
chk_P5_0F_issues(Code *cp)
#else
chk_P5_0F_issues(cp) Code *cp;
#endif
/* Flag the various instructions and return points that are a
   specific concern of the Pentium Errata 41 work-around
   code.
*/
{
    Ulong flags;

    flags = cp->info.code_inst->chkflags;
    /* flag all 0f !8x instructions */
    if (flags & IF_P5_0F_PREFIX)
	cp->code_impdep |= CODE_P5_0F_NOT_8X;
    /* flag all prefix pseudo instructions */
    if (flags & IF_PREFIX_PSEUDO_OP)
	cp->code_impdep |= CODE_PREFIX_PSEUDO_OP;
    /* flag each statement following a call */
    if (flags & IF_CALL) {
	/* The next instruction location will be the target of a branch,
	   i.e. return from the function. */
	cp->code_next->code_impdep |= CODE_P5_BR_LABEL;
    }  /* if */
}

static int
#ifdef __STDC__
P5_check_conflict(Section* secp, Code *cp, Ulong ZeroF_addr)
#else
P5_check_conflict(secp, cp, ZeroF_addr)
Section* secp; Code *cp; Ulong ZeroF_addr;
#endif
/*
**	return_value:	 0   - no padding required.
**			+n   - n bytes of padding needed before the
**			       instruction denoted by "cp".
**			-1   - 1 byte of padding following the code
**			       pointed to by "cp" will suffice.
*/
{
    Ulong sz;
    Uchar *bp;
    int pass_no = 0;
    int not_finished = 1;
    Eval *vp;

    bp = secp->sec_data;
    while (not_finished) {
	pass_no++;
get_additional_inst:
	switch(cp->code_kind) {

	default:
		fatal(gettxt(":0","P5_check_conflict(): unknown code kind: %u"),
		      (Uint)cp->code_kind);
		/*NOTREACHED*/
	case CodeKind_RelSym:
	case CodeKind_RelSec:
		fatal(gettxt(
		     ":0","P5_check_conflict(): bad code kind (reloc)"));
		/*NOTREACHED*/
	case CodeKind_Zero:
		/* A zero byte in either position in question means this is
		   not a problem. */
		if (cp->data.code_skip != 0)
			return 0;
		break;
	case CodeKind_FixInst:
	case CodeKind_VarInst:
		{   int sz;
		    sz = (*cp->info.code_inst->inst_gen)(secp, cp);
		    if (sz != cp->code_size) {
			/* The instruction size has changed. */
			if (sz < cp->code_size)
			    fatal(gettxt(":0",
			      "P5_check_conflict():smaller varinst encoding"));
			if ((cp->code_size = sz) != sz)
			    fatal(gettxt(":0",
			      "P5_check_conflict():varinst encoding too big"));
			recalc_sect_addrs(secp, cp, cp->code_addr,
					  "P5_check_conflict()");
		    }  /* if */
		    /* If this is a "prefix" pseudo-op instruction, get the following
		       instruction if needed to expand the first byte needed
		       (ZeroF_addr). */
		    if (   (cp->code_impdep & CODE_PREFIX_PSEUDO_OP)
			&& (pass_no == 1)
			&& (cp->code_next != 0)
			&& (cp->code_next->code_addr <= ZeroF_addr)) {
			cp = cp->code_next;
			goto get_additional_inst;
		    }  /* if */
		}
		break;
	case CodeKind_Skipalign:
		sz = cp->data.code_skip;
		goto P5_chk_pad_sz;

	case CodeKind_Pad:
		sz = cp->info.code_more->data.more_code->code_addr
		       + cp->data.code_skip - cp->code_addr;
		goto P5_chk_pad_sz;

	case CodeKind_Align:
		sz = cp->code_next->code_addr - cp->code_addr;
P5_chk_pad_sz:
		/* Padding in the first position means this is not 
		   a problem - no padding needed. */
		if ( pass_no == 1) {
			if (sz != 0)
				return 0;
		} else {
 			/* If on other than the first pass, and sz is > 1 byte,
 			   there is the distinct possibility that an leal or
			   or addl (0x8x) instruction will be used for the
			   padding.  Since byte 33 is known to be a problem,
			   insert 1 byte of padding (will be 0x90) between
			   bytes 33 and 34. */
 			if (sz == 1)
				return 0;
			if (sz != 0)
				return -1;
		}  /* if */
		break;
	case CodeKind_Expr:
		/* Expand the expression into the buffer.  Need to check
		   the actual value of the byte(s). */
		vp = evalexpr(cp->data.code_expr);
		if (vp->ev_flags & EV_RELOC)
		{
			memset(bp + cp->code_addr + P5_PEEK_SZ,
			       'R', cp->code_size);
			break;
		}
		switch (cp->info.code_form)
		{
		case CodeForm_Float:
		case CodeForm_Double:
		case CodeForm_Extended:
			flt_todata(vp, cp, secp);
			break;
		default:
			int_todata(vp, cp, secp);
			break;
		}
		break;
	case CodeKind_String:
		/* Expand the string value; need to check the values of the
		   byte(s).  Note, the string may be longer than the small
		   buffer that we are working in; adjust the buffer pointer
		   in secp->sec_data if necessary and only expand a portion
		   of the string.  Note: adjust "bp" also. */
		if (ZeroF_addr < cp->code_addr) {
			/* Only need the very first byte of the string to
			   complete the test. */
			bp[cp->code_addr] = *cp->info.code_more->data.more_str;
		}  else {
			int len;
			secp->sec_data = bp = &inst_buf[1] - ZeroF_addr;
			if ((cp->code_addr + cp->data.code_skip -1)
						== ZeroF_addr) {
				len = 1;
			} else {
				len = 2;
			}  /* if */
			(void) memcpy((void *) &bp[ZeroF_addr],
			      (const void *) &cp->info.code_more->data.more_str
						[ZeroF_addr - cp->code_addr],
			      len);
		}  /* if */
		break;
	case CodeKind_Backalign:
		/* A Backalign does not consume space.  The actual alignment,
		   if any, occurs at the corresponding Skipalign. */ 
		break;
	case CodeKind_Unset:
		/* If we have hit this, the ZeroF_addr did point to an 0x0f
		   at the end of the section; need 1 pad byte at the end
		   of the section. */
		return 1;
		/*NOTREACHED*/
	}  /* switch */

	if (pass_no == 1) {
		if (   bp[ZeroF_addr + P5_PEEK_SZ] != 'R'  /* not relocatable */
		    && bp[ZeroF_addr] != 0x0F) {
			/* The byte at 33 bytes beyond the 0f !8x instruction
			   is not and cannot be a 0x0F. */
			return 0;
		}  /* if */
	}  /* if */
	cp = cp->code_next;
	if ((ZeroF_addr + 1) < cp->code_addr) {
		/* The second byte in question is also in the instruction
		   just expanded. */
		not_finished = 0;
	}  /* if */
    }  /* while */

    if (   bp[ZeroF_addr + 1 + P5_PEEK_SZ] != 'R'
	&& (bp[ZeroF_addr + 1] & 0xF0) != 0x80  ) {
	/* The second byte is safe(not 0x8x); no padding needed. */
	return 0;
    } else if (pass_no > 1) {
	/* The 0x0f8x straddles 2 instructions; add 1 byte padding between
	   the two instructions. */
	return -1;
    }  /* if */

    /* The 0x0f8x bit pattern is within the single instruction.  Walk backward
       a byte at a time until a point is located that will avoid the 
       condition.  Remember, a safety pad (0 byte) preceeds the start of the
       first instruction. */
    ZeroF_addr -= 1;		 /* Point back  to the byte before the 0x0f
				    offset. */
    /* pass_no = 1;		    pass_no is already a 1. */
    while (   (   bp[ZeroF_addr + P5_PEEK_SZ] == 'R'
               || bp[ZeroF_addr] == ZERO_F_PREFIX)
	   && (   bp[ZeroF_addr + 1 + P5_PEEK_SZ] == 'R'
	       || (bp[ZeroF_addr+1] & 0xF0) == 0x80)) {
	/* Still have a potential problem.  Obviously around some relocatable
	   operand. */
	pass_no++;
	ZeroF_addr--;
    }  /* while */
    return pass_no;
}

boolean_t
#ifdef __STDC__
is_call_next_loc(Code *cp)	/* Check if the CALL in Code cp is a
				   call to the very next location... 
				   i.e. the PIC code call to get the 
				   current location counter. */
#else
is_call_next_loc(cp) Code *cp;
#endif
{
    Operand *op;
#ifdef DEBUG
    if (DEBUG('P') > 2)
    {
        (void)fprintf(stderr, "stmt(%s,ops=", cp->info.code_inst->inst_name);
        printoplist(cp->data.code_olst);
        (void)fputs(")\n", stderr);
    }
#endif

    /* Should only be call with single operand - avoid LCALL. */
    if ((op = cp->data.code_olst->olst_first) ==
				cp->data.code_olst->last.olst_last) {
	if (   op->oper_info == OC_MEM
	    && op->oper_flags == 0 ) {

	    Expr *expr = op->oper_expr;

	    if (   expr->ex_type == ExpTy_Relocatable
		&& expr->ex_op   == ExpOp_LeafName ) {

		Symbol *sym = expr->right.ex_sym;

		if (sym->sym_kind == SymKind_Regular) {
		    expr = sym->addr.sym_expr;
		    if (   expr
		        && expr->ex_op == ExpOp_LeafCode
			&& expr->right.ex_code == cp->code_next)
			return B_TRUE; 
		}  /* if */
	    }  /* if */
	}  /* if */
    }  /* if */
    return B_FALSE;
}

int
#ifdef __STDC__
pentium_bug(Section *secp) /* Check for possible Pentium erratta candidates
			      and insert padding/nop */
#else
pentium_bug(secp) Section *secp;
#endif
{
    register Code *cp;
    int pad_num;
    int change = 0;
    int accum_problem_stats;
    Code *last_label = 0;
    Code *prev_cp = 0;

#ifdef DEBUG
    if (DEBUG('P') > 0) {
	(void)fprintf(stderr, "P5 ERR-41: for section %s\n",
		      secp->sec_sym->sym_name);
    }  /* if */
#endif
    accum_problem_stats = flags & ASFLAG_P5_ERR;
    P5_pseudo_expand = 1;
    for (cp = secp->sec_code; cp != 0 ; prev_cp = cp, cp = cp->code_next) {
	if (cp->code_impdep & CODE_P5_BR_LABEL){
	    last_label = cp;
	}  /* if */
	if (cp->code_impdep & CODE_P5_0F_NOT_8X) {
#ifdef DEBUG
	    if (DEBUG('P') > 1) {
		(void)fprintf(stderr,
			      "    0F_NOT_8F at %#lx; last_label at %#lx\n",
			      cp->code_addr,
			      (last_label ? last_label->code_addr : 0));
	    }  /* if */
#endif
	    /* Check if branching to any of the 30 bytes that
	       precede this instruction or to this instruction.
	       Assume that if within the first 30 bytes of a section
	       and no label, that it may be within a .init and .fini
	       section and a potential problem, */
	    if ((cp->code_addr - (last_label ? last_label->code_addr : 0))
					< P5_BR_WINDOW) {

		int i = 1;
		int sz;
		int trailing_pad = 0;
		Ulong ZeroF_addr;
		Code *work_cp, *next_cp, *last_of_prefixed_set_cp;

		pad_num = 0;
#ifdef DEBUG
		if (DEBUG('P') > 0) {
		    (void)fprintf(stderr,
			          "    0F_NOT_8F candidate at offset %#lx\n",
				  cp->code_addr);
		}  /* if */
#endif
		/* Now must look 33-34 bytes ahead from the 0F prefix. */
		/* Find section offset of 0F prefix - not ncessarily the first
		   byte of the instruction at "cp"; but it is knowned to
		   be an instruction. */
		secp->sec_data = &inst_buf[1] - cp->code_addr;
		sz = (*cp->info.code_inst->inst_gen)(secp, cp);

		if (sz != cp->code_size) {
		    /* The instruction size has changed. */
		    if (sz < cp->code_size)
			fatal(gettxt(":0",
				   "pentium_bug():smaller varinst encoding"));
		    if ((cp->code_size = sz) != sz)
			fatal(gettxt(":0",
			           "pentium_bug():varinst encoding too big"));
		    recalc_sect_addrs(secp, cp, cp->code_addr,
					  "pentium_bug()");
		}  /* if */

		ZeroF_addr = cp->code_addr + 33;
		while (inst_buf[i++] != ZERO_F_PREFIX) {
		    ZeroF_addr++;
		}

		/* Now look 33 - 34 bytes ahead from the address of that
		   of the 0F !8x prefix. */
		if (ZeroF_addr >= secp->sec_size) {
		    /* The section ends;  will need to add padding to insure
		       that ld or rtld will not create a problem later. */
		    trailing_pad = pad_num =
			                ZeroF_addr - secp->sec_size +1;
		} else {
		    Uchar code_kind;

		    for (prev_cp = cp, work_cp = cp->code_next;
			 (   (work_cp != 0)
			  && ((next_cp = work_cp->code_next) != 0)
			  && (next_cp->code_addr <= ZeroF_addr));
			 work_cp = next_cp) {
			/* Watch for one of the prefix pseudo-ops which
			   cannot be separated from the following inst.
			   - OR - the CALL to set up PIC addressability. */
			if (   work_cp->code_impdep & CODE_PREFIX_PSEUDO_OP
			    || (   (   work_cp->code_kind == CodeKind_FixInst
				    || work_cp->code_kind == CodeKind_VarInst)
				&& work_cp->info.code_inst->chkflags & IF_CALL
				&& is_call_next_loc(work_cp)) ) {
			    /* Force the PIC initialization call to "look"
			       like a prefix pseudo op. */
			    work_cp->code_impdep |= CODE_PREFIX_PSEUDO_OP;
			    /* Loop ahead - without bumping prev_cp
			       and work_cp. */
			    while (   (last_of_prefixed_set_cp = next_cp, next_cp = next_cp->code_next) != 0
				   && (last_of_prefixed_set_cp->code_impdep & CODE_PREFIX_PSEUDO_OP)) {
				; /* nothing loop */
			    }  /* empty while */
			    if (   (next_cp == 0)
				|| (next_cp->code_addr > ZeroF_addr )) {
				/* target is in inst. pair */
				break;
			    }  else {
				/* Not in this inst. set; bump up as normally
				   done - skipping inst. set. */
				prev_cp = last_of_prefixed_set_cp;
			    }  /* if */
			} else {
			    prev_cp = work_cp;
			}  /* if */
		    }  /* for */

		    /* work_cp now points at the instruction or padding
		       containing the first byte that must be checked. */
		    memset(&inst_buf[0], 0, 2 * P5_PEEK_SZ);
		    secp->sec_data =&inst_buf[1] - work_cp->code_addr;
		    pad_num = P5_check_conflict(secp, work_cp, ZeroF_addr);
		    if ((ZeroF_addr + 1) >= secp->sec_size)
			trailing_pad = pad_num;
		    if (pad_num < 0) {
#ifdef DEBUG
			if (DEBUG('P') > 2) {
			    (void)fprintf(stderr,
				      "    -- bytes 33-34 straddle 2 code\n");
			}  /* if */
#endif
			pad_num = -pad_num;
			if (! (work_cp->code_impdep & CODE_PREFIX_PSEUDO_OP)) {
			    prev_cp = work_cp;
			} /* if */
		    }  /* if */
		    /* Either prev_cp or the next Code should point to some
		       instruction.  If not, we may be within some data
		       embedded in an executable section. */
		    if (   ((code_kind = prev_cp->code_kind) == CodeKind_FixInst)
			|| (code_kind == CodeKind_VarInst)
			|| ((code_kind = prev_cp->code_next->code_kind) == CodeKind_FixInst)
			|| (code_kind == CodeKind_VarInst)) {
			/* This is OK. */
			;
		    } else {
			/* Add the padding following the 0f !8x instruction. */
			prev_cp = cp;
		    }  /* if */
		}  /* if */
		if (trailing_pad != 0) {
		    /* Add padding/nops at end of a section.  There are
		       currently no 0f 8x instructions used for nops. */
#ifdef DEBUG
		    int sav_debug_c;

		    if (DEBUG('P') > 0) {
			sav_debug_c = DEBUG('c');
			DEBUG('c') = DEBUG('P');
			if (DEBUG('P') > 1) {
			    (void)fprintf(stderr,
					  "    -- trailing pad of %d\n",
					  trailing_pad);
			}  /* if */
		    }  /* if */
#endif
		    change = 1;
		    P5_pad_eval.ev_sec = secp;
		    P5_pad_eval.ev_dot = secp->sec_last;
		    P5_pad_eval.ev_ulong = secp->sec_last->code_addr +
						trailing_pad;
		    /* Check if the last actual code was a CodeKind_Align;
		       if so, need to duplicate after the padding. */
		    work_cp = secp->sec_prev;

		    sectpad(secp, &P5_pad_eval);

#ifdef DEBUG
		    if (DEBUG('P') > 0)
			DEBUG('c') = sav_debug_c;
#endif
		    if (work_cp->code_kind == CodeKind_Align) {
#ifdef DEBUG
			if (DEBUG('P') > 2) {
			    (void)fprintf(stderr,
				  "    -- restoring .align to end of section\n");
			}  /* if */
#endif
			sectalign(secp, work_cp->data.code_align,
				  work_cp->code_size);
		    }  /* if */
		    if (accum_problem_stats != 0) {
			P5_pad_at_sect_end += trailing_pad;
			P5_0f8x_sites++;
		    }  /* if */

		} else if (pad_num != 0) {
		    /* "prev_cp" points to the instruction after which "pad_num"
		       bytes of padding/nops are required. */
		    change = 1;
#ifdef DEBUG
		    if (DEBUG('P') > 1) {
			(void)fprintf(stderr,
				      "    -- pad of %d bytes at loc: %#lx\n",
				      pad_num, prev_cp->code_next->code_addr);
		    }  /* if */
#endif
		    /* Note: If the padding happens to be placed within
			     a SkipAlign - BackAlign range, this added
			     padding may not have an immediate effect.
			     Multiple iterations may be necessary to 
			     effectively avoid the problem bit pattern.
		    */
		    /* Insert the padding now. */
		    sectinsertpad(secp, prev_cp, pad_num);

		    /* Recalculate the section addresses of the remaining
		       Code statements following the insertion. */
		    if (prev_cp) {
			recalc_sect_addrs(secp, prev_cp, prev_cp->code_addr,
					  "pentium_bug()");
		    } else {
			recalc_sect_addrs(secp, secp->sec_code, 0,
					  "pentium_bug()");
		    }  /* if */
		    if (accum_problem_stats != 0) {
			P5_pad_bytes_added += pad_num;
			P5_0f8x_sites++;
		    }  /* if */
		}  /* if */
	    }  /* if */
	}  /* if */
    }  /* for */
    secp->sec_data = 0;		/* reset to NULL */
    P5_pseudo_expand = 0;
    return change;
}


void
#ifdef __STDC__
P5_err_report_stats(const char * file_name)	/* report padding info,
						   if requested */
#else
P5_err_report_stats(file_name)	char * file_name;
#endif
{
    (void)fprintf(stderr,
	"P5_ERR_41: %s:\tsites = %d, nop/padding = %d, trailing padding = %d\n",
		  file_name,
		  P5_0f8x_sites, P5_pad_bytes_added,
		  P5_pad_at_sect_end);
}
#endif
