#ident	"@(#)optim:i386/optutil.c	1.1.5.70"
/* optutil.c
**
**	Optimizer utilities for Intel 386 code improver (PCI).
**
**
** This module contains utility routines for the various peephole
** window optimizations.
*/

/* #include <ctype.h> -- optim.h takes care of this */
/* #include "defs" -- optim.h takes care of this; machine-dependent defs */
/* #include "optim.h"			/* machine independent optimizer defs */
#include "sched.h"
#include "optutil.h"
extern int pic_flag;
extern int fp_removed;

extern int suppress_enter_leave;
struct RI reg_reginfo[] = {
                         { EAX, { "%eax", "%ax", "%al"}},
                         { EDX, { "%edx", "%dx", "%dl"}},
                         { ECX, { "%ecx", "%cx", "%cl"}},
                         { EBX, { "%ebx", "%bx", "%bl"}},
                         { ESI, { "%esi", "%si", NULL }},
                         { EDI, { "%edi", "%di", NULL }},
                         { EBI, { "%ebi", "%bi", NULL }}
                        };

struct RI fix_reginfo[] = {
                         { EAX, { "%eax", "%ax", "%al"}},
                         { EDX, { "%edx", "%dx", "%dl"}},
                         { ECX, { "%ecx", "%cx", "%cl"}},
                         { EBX, { "%ebx", "%bx", "%bl"}},
                         { ESI, { "%esi", "%si", NULL }},
                         { EDI, { "%edi", "%di", NULL }},
                         { EBP, { "%ebp", "%bp", NULL }}
                        };

struct RI *reginfo;

/* get free reg -
**   returns the number (in structure RI) of a free found register)
**/
int
get_free_reg(cant,byte_reg,cn)
unsigned int  cant;
int byte_reg;
NODE *cn;
{
int i,temp,nregs;
NODE *p = cn;
unsigned int near_use = cant;
static int regorder[] = { 1,2,0,3,4,5,6 };

	if ( cn == NULL )	{ /* Restart it */
		regorder[0] = 1;
		regorder[1] = 2;
		regorder[2] = 0;
		return -1;
	}
	nregs = byte_reg ? 4 : (suppress_enter_leave ?  6 : 7);
	/* first try to issue a register which is not used in the previous
	** and the next instructions.
	*/
	p = p->back;
	if (p->op != CALL) near_use |= uses(p) | sets(p) | p->nlive;
	p = p->forw->forw;
	if (p->op != CALL) near_use |= uses(p) | sets(p) | p->nlive;
	for (i=0; i < nregs; i++) {
	  	if (! (near_use & reginfo[regorder[i]].reg_number)) {
	  		i = regorder[i];
			if (target_cpu & (P5 | blend)) { 
				temp = regorder[0];
				regorder[0] = regorder[1];
				regorder[1] = regorder[2];
				regorder[2] = temp;
			}
			return i;
		}
	}
	/* Then try to find anyone, in a round robin method */
	for (i=0; i < nregs; i++) {
	  	if (! (cant & reginfo[regorder[i]].reg_number)) {
	  		i = regorder[i];
			if (target_cpu & (P5 | blend)) { 
				temp = regorder[0];
				regorder[0] = regorder[1];
				regorder[1] = regorder[2];
				regorder[2] = temp;
			}
			return i;
		}
	}
    return -1;
}

BLOCK *
block_of(p) NODE *p;
{
BLOCK *b;
NODE *q;
	for (b = b0.next; b; b = b->next) {
		for (q = b->firstn; q != b->lastn->forw; q = q->forw) {
			if (q == p) return b;
		}
	}
	/* NOTREACHED */
}/*end block_of*/

char *
w2l(wreg) char *wreg;
{
unsigned int reg_idx = setreg(wreg);
	switch (reg_idx) {
		case AX: return "%al";
		case BX: return "%bl";
		case CX: return "%cl";
		case DX: return "%dl";
		default:
			fatal(__FILE__,__LINE__,"w2l: not a byteable register: %s\n",wreg);
	}
	/* NOTREACHED */
}/*end w2l*/

char *
w2h(wreg) char *wreg;
{
unsigned int reg_idx = setreg(wreg);
	switch (reg_idx) {
		case AX: return "%ah";
		case BX: return "%bh";
		case CX: return "%ch";
		case DX: return "%dh";
		default:
			fatal(__FILE__,__LINE__,"w2l: not a byteable register: %s\n",wreg);
	}
	/* NOTREACHED */
}/*end w2h*/

NODE *
first_non_label(b) BLOCK *b;
{
NODE *p = b->firstn;
	while (is_any_label(p)) p = p->forw;
	return p;
}

NODE *
non_label_after(p) NODE *p;
{
	while (is_any_label(p)) p = p->forw;
	return p;
}

boolean
reverse_ptr_err(NODE *br)
{
	if (br->op == JNE || br->op == JNZ) return true;
	return false;
}

boolean
reverse_func_const(NODE *br)
{
short op = br->op;
	if (op == JB || op == JBE || op == JL || op == JLE || op == JNA
		|| op == JNAE || op == JNE || op == JNZ)
		return true;
	return false;
}

/* can p change to an unsigned jmp */
boolean
is_unsable_br(p) NODE *p;
{
	switch (p->op) {
		case JG: case JGE: case JL: case JLE:
		case JNG: case JNGE: case JNL: case JNLE:
		case JA: case JAE: case JB: case JBE: case JE: case JNZ:
		case JNA: case JNAE: case JNB: case JNBE: case JNE: case JZ:
			return true;
		default:
			return false;
	}
}

void
sign2unsign(p) NODE *p;
{
	switch (p->op) {
		case JG: chgop(p,JA,"ja"); return;
		case JGE: chgop(p,JAE,"jae"); return;
		case JL: chgop(p,JB,"jb"); return;
		case JLE: chgop(p,JBE,"jbe"); return;
		case JNG: chgop(p,JNA,"jna"); return;
		case JNGE: chgop(p,JNAE,"jnae"); return;
		case JNL: chgop(p,JNB,"jnb"); return;
		case JNLE: chgop(p,JNBE,"jnbe"); return;
		case JA:
		case JAE:
		case JB:
		case JBE:
		case JNA:
		case JNAE:
		case JNB:
		case JNBE:
		case JE:
		case JNZ:
		case JNE:
		case JZ: return; /* no change */
		default: fatal(__FILE__,__LINE__,"non unsignable op %s\n",p->op);
	}
	/* NOTREACHED */
}/*end sign2unsign*/

void
unsign2sign(p) NODE *p;
{
	switch (p->op) {
		case JG:
		case JGE:
		case JL:
		case JLE:
		case JNG:
		case JNGE:
		case JNL:
		case JNLE: return;
		case JA: chgop(p,JG,"jg"); return;
		case JAE: chgop(p,JGE,"jge"); return;
		case JB: chgop(p,JL,"jl"); return;
		case JBE: chgop(p,JLE,"jle"); return;
		case JNA: chgop(p,JNG,"jng"); return;
		case JNAE: chgop(p,JNGE,"jnge"); return;
		case JNB: chgop(p,JNL,"jnl"); return;
		case JNBE: chgop(p,JNLE,"jnle"); return;
		case JE:
		case JNZ:
		case JNE:
		case JZ: return; /* no change */
		default: fatal(__FILE__,__LINE__,"non unsignable op %s\n",p->op);
	}
	/* NOTREACHED */
}/*end unsign2sign*/

/* insert -- insert new instruction node
**
** This routine inserts a new instruction node after the one pointed
** to.  The fields in the node are initialized to null values.
*/

NODE *					/* return pointer to the new node */
insert(pn)
NODE * pn;				/* node to add pointer after */
{
    NODE * new = Saveop(0,"",0,GHOST);	/* create isolated node, init. */

    APPNODE(new,pn);			/* append new node after current */

    return(new);			/* return pointer to the new one */
}
/* chgop -- change op code number and op code string in node
**
** This routine changes the op code information in a node.  The
** operand information is unaffected.
*/

void
chgop(n,op,op_code)
NODE * n;				/* pointer to node to change */
unsigned int op;					/* op code number */
char * op_code;				/* pointer to op code string */
{
    n->op = (unsigned short)op;		/* set op code number */
    n->opcode = op_code;		/* and string */
    return;
}

/*Put a copy of p after q. */
NODE *
duplicate(p,q) NODE *p,*q;
{
NODE *pnew;
int i;
	pnew = insert(q);
	chgop(pnew,p->op,p->opcode);
	for (i = 1; i <= 3; i++) pnew->ops[i] = p->ops[i];
	pnew->sets = p->sets;
	pnew->uses = p->uses;
	pnew->userdata = p->userdata;
	return pnew;
}

/* isdyadic -- is instruction dyadic
**
** This routine tells whether an instruction is dyadic according
** to the following conditions:
**
**	1.	It takes two operands.
**	2.	It reads the first operand, writes the second.
**	3.	It sets the condition codes according to the value stored in
**		the second operand.
**	4.	The operands have normal address mode meanings:  disallow,
**		for example, LEAL, which does address arithmetic of op1.
*/

boolean
isdyadic(n)
NODE * n;				/* instruction node to test */
{
    switch(n->op)			/* dispatch on op code number */
    {
    /* Check for single operand forms of shift instructions. *.
    case SALB: case SALW: case SALL:	/* added 5/6/96 */
    case SARB: case SARW: case SARL:
    case SHLB: case SHLW: case SHLL:
    case SHRB: case SHRW: case SHRL:
	if (n->op2 == NULL)  return (false);
	/* FALLTHROUGH */

    case ADDB: case ADDW: case ADDL:
    case ANDB: case ANDW: case ANDL:
    case MOVB: case MOVW: case MOVL:
    case MOVSBW: case MOVSBL: case MOVSWL:
    case MOVZBL: case MOVZBW: case MOVZWL:
    case ORB:  case ORW:  case ORL:
    case SUBB: case SUBW: case SUBL:
    case XORB: case XORW: case XORL:
	return(true);			/* all of these are dyadic */
    default:
	return(false);			/* others are not */
    }
}
/* istriadic -- is instruction triadic?
**
** This routine serves a similar function to isdyadic, except that
** it also returns the related dyadic op code number and string.
** The only instructions included here are the obvious ones.  They
** are assumed to have integer operands (by callers).
*/

/* define macro to use below */

#define CHANGE(op,new,newst)	case op: *pnewop=new;*pnewopcode=newst;break;


boolean
istriadic(n,pnewop,pnewopcode)
NODE * n;				/* node to test */
int * pnewop;				/* place to put equiv. dyadic # */
char ** pnewopcode;			/* place to put equiv. dyadic string */
{
	if ( ( n->op == IMULL ||
	       n->op == IMULW ||
	       n->op == IMULB ) &&
	     isnumlit(n->op1) &&
	     isreg(n->op3) ) {		/* imul[bwl]  $X,op,reg case */
		*pnewop = n->op;
		switch ( n->op ) {
		    case IMULL:
			*pnewopcode = "imull";
			break;
		    case IMULW:
			*pnewopcode = "imulw";
			break;
		    case IMULB:
			*pnewopcode = "imulb";
			break;
		}
		return( true );
	}

	return(false);
}
/* is2dyadic -- is instruction dyadic
**
** This routine is similar to isdyadic() but has the following
** conditions:
**
**	1.	It takes two operands.
**	2.	It reads at least the first operand (and doesn't
**		write it).
**	3.	The first operand can be register, memory, or
**		immediate mode (if it's memory or immediate, the
**		other operand must be register).
**	4.	The operands have normal address mode meanings:  disallow,
**		for example, LEAL, which does address arithmetic of op1.
*/

boolean
is2dyadic(n)
NODE * n;				/* instruction node to test */
{
    switch(n->op)			/* dispatch on op code number */
    {
    case ADDB: case ADDW: case ADDL:
    case ANDB: case ANDW: case ANDL:
    case CMPB: case CMPW: case CMPL:
    case MOVB: case MOVW: case MOVL:
    case ORB:  case ORW:  case ORL:
    case SUBB: case SUBW: case SUBL:
    case XORB: case XORW: case XORL:
	return(true);			/* all of these are dyadic */
    default:
	return(false);			/* others are not */
    }
}
/* isfp -- is instruction a floating point instruction? */

	boolean
isfp(n)
NODE *n;
{
    switch(n->op)
    {
    case F2XM1: case FABS: case FADD: case FADDL:
    case FADDP: case FADDS: case FBLD: case FBSTP: case FCHS:
    case FCLEX: case FCOM: case FCOML: case FCOMP: case FCOMPL:
    case FCOMPP: case FCOMPS: case FCOMS: case FDECSTP: case FDIV:
	case FCMOVB: case FCMOVE: case FCMOVBE: case FCMOVU:
	case FCMOVNB: case FCMOVNE: case FCMOVNBE: case FCMOVNU:
    case FDIVL: case FDIVP: case FDIVR: case FDIVRL: case FDIVRP:
    case FDIVRS: case FDIVS: case FFREE: case FIADD: case FIADDL:
    case FICOM: case FICOML: case FICOMP: case FICOMPL: case FIDIV:
    case FIDIVL: case FIDIVR: case FIDIVRL: case FILD: case FILDL:
    case FILDLL: case FIMUL: case FIMULL: case FINCSTP: case FINIT:
    case FIST: case FISTL: case FISTP: case FISTPL: case FISTPLL:
    case FISUB: case FISUBL: case FISUBR: case FISUBRL: case FLD:
    case FLD1: case FLDCW: case FLDENV: case FLDL: case FLDL2E:
    case FLDL2T: case FLDLG2: case FLDLN2: case FLDPI: case FLDS:
    case FLDT: case FLDZ: case FMUL: case FMULL: case FMULP:
    case FMULS: case FNCLEX: case FNINIT: case FNOP: case FNSAVE:
    case FNSTCW: case FNSTENV: case FNSTSW: case FPATAN: case FPREM:
    case FPTAN: case FRNDINT: case FRSTOR: case FSAVE: case FSCALE:
    case FSETPM: case FSQRT: case FST: case FSTCW: case FSTENV:
    case FSTL: case FSTP: case FSTPL: case FSTPS: case FSTPT:
    case FSTS: case FSTSW: case FSUB: case FSUBL: case FSUBP:
    case FSUBR: case FSUBRL: case FSUBRP: case FSUBRS: case FSUBS:
    case FTST: case FWAIT: case FXAM: case FXCH: case FXTRACT:
    case FYL2X: case FYL2XP1: case FCOS: case FPREM1: case FSIN: 
    case FSINCOS: case FUCOM: case FUCOMP: case FUCOMPP:
      return(true);
    default:
      return(false);
    }
}

boolean
is_fp_arit(p) NODE *p;
{
	switch (p->op) {
		case FADD: case FADDS: case FADDL: case FADDP: case FSUB:
		case FSUBS: case FSUBL: case FSUBP: case FSUBR: case FSUBRS:
		case FSUBRL: case FSUBRP: case FMUL: case FMULS: case FMULL:
		case FMULP: case FDIV: case FDIVS: case FDIVL: case FDIVP:
		case FDIVR: case FDIVRS: case FDIVRL: case FDIVRP:
		case FCOM: case FCOMS: case FCOML: case FUCOM:
		/*case FCOMP: case FCOMPS: case FCOMPL: case FUCOMP: */
			return true;
		default:
			return false;
	}
}/*end is_fp_arit*/

boolean
is_fp_i_arit(p) NODE *p;
{
  switch (p->op) {
      case FIADDL: case FISUBL: case FISUBRL: case FIMULL: case FIDIVL:
      case FIDIVRL: case FICOML:
      /*case FICOMPL:*/
          return true;
      default:
          return false;
  }
}/*end is_fp_i_arit*/

boolean
is_any_farit(p) NODE *p;
{
  return is_fp_arit(p) || is_fp_i_arit(p);
}


	boolean
is_byte_instr(n)
NODE *n;
{
	switch (n->op) {
	case ADCB: case ADDB: case DIVB: case IDIVB: case MULB: 
	case SUBB: case SBBB: case ANDB: case ORB: case XORB:
	case CMPB: case TESTB: case MOVB: case IMULB:
	case MOVSBL: case MOVSBW: case MOVZBW: case MOVZBL:
	case CMPXCHGB: case XADDB:
		return true;
	default:
		return false;
	}
}

/* makelive -- make register live
**
** This routine marks a register operand as live.
*/

void
makelive(s,n)
char * s;				/* operand string (to %r0) */
NODE * n;				/* pointer to node to enliven */
{
    n->nlive |= setreg(s);
    return;
}



/* makedead -- make register dead
**
** This routine marks a register operand as dead.
*/

void
makedead(s,n)
char * s;				/* operand string (to %rn) */
NODE * n;				/* pointer to node to kill */
{
    n->nlive &= ~ setreg(s);
    return;
}
/* isindex -- is operand an index off a register?
**
** This routine checks whether an operand string is of the
** form:
**
**	n(%rn)
**
** and whether it uses a designated register.
**
** Actually, it cheats:  it just checks for an initial digit or sign followed
** by a (.  In fact the ( test is a cheat, because we just check to see
** if there's a ( in the string.
*/

boolean
isindex(s,r)
char * s;				/* operand string to test */
char * r;				/* register operand string to check
					** for
					*/

{
    if(s != NULL && *s == '*')
		s++;
    return(
		usesreg(s,r)
	    &&	(isdigit(*s) || *s == '-') /* could be leading sign, too */
	    &&  strchr(s,'(') != (char *) 0
	    &&  strchr(s,',') == (char *) 0	/* discard n(%rx,%ry,m) */
	    );
}

/* Read only data items look different: .L134.RO */
boolean
is_read_only(op) char *op;
{
char *t;
	if (!op) return false;
	if ((*op) != '.') return false;
	t = op +1;
	t = strchr(t,'.');
	if (!t) return false;
	t++;
	return (*t++ == 'R' && *t == 'O');
}/*end is_read_only*/

/* ismove -- is instruction a move?
**
** This routine determines whether an instruction is a move-type.
** There is a multiplicity of move instructions which copy, zero-
** or sign-extend, and truncate.  This routine returns the natural
** size of the source and destination operands.
*/

boolean
ismove(n,srcsize,dstsize)
NODE * n;				/* pointer to instruction node */
int * srcsize;				/* place to put size of source
					** operand (in bytes)
					*/
int * dstsize;				/* place to put size of destination */
{
    register int srctemp = 1;		/* temporary source size */
    register int dsttemp = 4;		/* temporary destination size */
/* Initial values chosen on basis of most common value of each. */

    switch (n->op)			/* dispatch on op-code number */
    {
    case MOVZBW:
			dsttemp = 2;	break;
    case MOVZBL:
					break;
    case MOVZWL:
	srctemp = 2;			break;

    case MOVB:
			dsttemp = 1;	break;
    case MOVW:
	srctemp = 2;	dsttemp = 2;	break;
    case MOVL:
	srctemp = 4;			break;

    case MOVSBW:
			dsttemp = 2;	break;
    case MOVSBL:
					break;
    case MOVSWL:
	srctemp = 2;			break;

    default:
	return(false);			/* not a move instruction */
    }
    *srcsize = srctemp;			/* copy out correct values */
    *dstsize = dsttemp;
    return(true);			/* found a move */
}
/* is_legal_op1 -- check that operand size in bytes is legal for this
		     opcode.  This is a very conservative check which
		     fixes a bug in one of the 2 instruction peepholes
		     (reorder loads) */
	boolean
is_legal_op1(oprndsize,n)
int oprndsize;
NODE *n;
{
	switch(oprndsize) {
	case 1: return (is_byte_instr(n));
	case 2: /* inclusion of single op instructions is harmless,
		   albeit (perhaps) useless */
		switch(n->op) {
		case ADCW: case ADDW: 
		case CMOVOW: case CMOVNOW: case CMOVBW:
		case CMOVCW: case CMOVNAEW: case CMOVAEW: case CMOVNCW:
		case CMOVNBW: case CMOVEW: case CMOVZW: case CMOVNEW:
		case CMOVNZW: case CMOVNAW: case CMOVBEW: case CMOVAW:
		case CMOVNBEW: case CMOVSW: case CMOVNSW: case CMOVPW:
		case CMOVPEW: case CMOVNPW: case CMOVPOW: case CMOVLW:
		case CMOVNGEW: case CMOVNLW: case CMOVGEW: case CMOVNGW:
		case CMOVLEW: case CMOVGW: case CMOVNLEW:
		case DECW: case DIVW: case IDIVW:
		case IMULW: case MULW: case INCW: case NEGW: case SBBW:
		case SUBW: case ANDW: case ORW: case XORW: case CLRW:
		case RCLW: case RCRW: case ROLW: case RORW: case SALW:
		case SARW: case SHLW: case SHRW:
		case CMPW: case TESTW: case CWTL: case CWTD: case LEAW:
		case MOVW: case MOVSWL: case MOVZWL: case NOTW: case PUSHW:
		case XCHGW: case SCAW: case SCMPW: case SLODW: case SMOVW:
			return true;
		default:
			return false;
		}
	case 4:
		switch(n->op) {
		case ADCL: case ADDL: case DECL: case DIVL: case IDIVL:
		case IMULL: case MULL: case INCL: case NEGL: case SBBL:
		case SUBL:
		case CMOVCL: case CMOVNAEL: case CMOVAEL: case CMOVNCL:
		case CMOVNBL: case CMOVEL: case CMOVZL: case CMOVNEL:
		case CMOVNZL: case CMOVNAL: case CMOVBEL: case CMOVAL:
		case CMOVNBEL: case CMOVSL: case CMOVNSL: case CMOVPL:
		case CMOVPEL: case CMOVNPL: case CMOVPOL: case CMOVLL:
		case CMOVNGEL: case CMOVNLL: case CMOVGEL: case CMOVNGL:
		case CMOVLEL: case CMOVGL: case CMOVNLEL:
		case ANDL: case ORL: case XORL: case CLRL: case RCLL:
		case RCRL: case ROLL: case RORL: case SALL: case SARL:
		case SHLL: case SHRL: case CMPL:
		case TESTL: case CLTD: case LEAL:
		case MOVW:
		case NOTL: case PUSHL: case XCHGL: case SCAL:
		case SCMPL: case SLODL: case SMOVL:
			return true;
		default:
			return false;
		}
	default:
		return false;
	}
}

/*did reg change between first and second instructions.           */
boolean
changed(first,second,regs) NODE *first,*second; unsigned int regs;
{
 NODE *p;
	if (!fp_removed && (regs == EBP)) return false;
	if (fixed_stack && (regs == frame_reg)) return false;
 	for (p=first->forw; p != second; p = p->forw) {
		if (!p) return true; /*fell off end of the function */
		if (p->sets & regs) return true;
		if (islabel(p))	return true; /*if not in same BB, give up */
	}
 	return false;
}/*end changed*/

/* iszoffset -- is operand zero offset from register
**
** This routine determines whether a given operand is an indexed
** operation from a designated register, where the index is zero.
*/

boolean
iszoffset(operand,reg)
register char * operand;		/* operand string */
char * reg;				/* register string */
{

/* We use a quick and dirty strategy here:  we test the operand for
** zero, followed by (, with matching register numbers in the right
** place.  We assume register designations are "%eXX".
*/

    return(	operand != NULL
	    &&  *operand == '('
	    &&  *(operand+1) == *reg
	    &&  *(operand+2) == *(reg+1)
	    &&  *(operand+3) == *(reg+2)
	    &&  *(operand+4) == *(reg+3)
	    &&  *(operand+5) == ')'
	  );
}
/* exchange -- exchange node and successor
**
** This routine reverses the linkages of two nodes so the first is
** the second and the second is the first.
*/

void
exchange(first)
NODE * first;				/* pointer to first of two nodes */
{

/* This can probably be done with fewer temporaries, but it's a lot
** easier to understand what's going on this way....
*/

    NODE * prev = first->back;		/* predecessor of first node */
    NODE * second = first->forw;	/* second node of pair to exchange */
    NODE * next = second->forw;		/* successor of second node */

    prev->forw = second;		/* second will now be first */
    second->back = prev;

    first->forw = next;			/* successor of old first is now
					** 'next'
					*/
    next->back=first;			/* 'next's' predecessor now 'first' */

    second->forw = first;
    first->back = second;		/* re-link connection of node pair */

    return;
}
/* isnumlit -- is operand a numeric literal
**
** This routine tests operand strings for the form
**
**	$[-]n
**
** where n is a positive integer.
*/

boolean
isnumlit(s)
char * s;				/* pointer to operand string */
{
    if (s == NULL || *s++ != '$')
	return(false);
    
    if (*s == '-')			/* sign okay */
	s++;
    
    while (*s != '\0')			/* check for all digits */
	if ( ! isdigit(*s++) )
	    return(false);
    
    return(true);			/* passed all tests */
}
/* undefinedCC -- does the instruction leave any of the condition 
**                undefined
**
** Check if the opcode of the instruction pointed to by  "p"
** is for an instruction that leaves the CC undefined.  It is
** implemented as series of range checks and for the shift
** instructions a check is made for a known (immediate)
** shift of one.
** The rotate instructions do not set all the CC flags.
**/

boolean
undefinedCC (NODE * p)
{
	unsigned int op;
	op =  p->op;
	if (   (op >= DIVB  && op <= IMULL)
		|| (op >= MULB  && op <= MULL)
		|| (op >= BTCL  && op <= BTW)
		|| (op >= AAA   && op <= DAS)
		|| (op >= RCLB  && op <= RORL)
		|| (op >= SHLDW && op <= SHRDL))
	
		return true;

	/* The shift instructions SAL SAR SHL SHR only set the
	   significant CC flags when a single bit shift is done. */
	if (   (op >= SALB  && op <= SHRL)
		&& (p->op2 != NULL) /* This is a 2 operand shift - i.e not an implicit
							   shift by 1 bit.  Check for am immediate 1. */ 
		&& (strcmp(p->op1,"$1") != 0)) {

		return true;
	}  /* if */

	return false;

}  /* undefinedCC */

/* usesvar -- does first operand use second?
**
** This routine is a generalization of "usesreg":  it returns true
** if the first operand uses the second.  An example is
** v1 = "*p", v2 = "p".  v1 uses the contents of v2.
*/

boolean
usesvar(v1,v2)
char * v1;				/* using operand */
char * v2;				/* used operand */
{
    if (v1 == NULL || v2 == NULL)
	return(false);
    if (isreg(v2) && usesreg(v1,v2))
	return(true);			/* handle register case specially */

    if (*v1 == '*')			/* check for initial indirect */
	v1++;

    return(strcmp(v1,v2) == 0);		/* result is result of compare */
}


/* These routines try to preserve the line number information.
   The criterion for saving or discarding line number information is
   that the state of the machine from a C point of view should be the
   same at any given line both before and after the optimization.
   If it would not be then the line number is deleted.
   The exceptions to the above are that the state of the condition codes 
   and of dead registers is ignored. 
   
	May 1983 */

/* NOTE: All instruction nodes that these routines assume will be tossed
	  have their uniqids set to IDVAL.  For example, in the merges,
	  the uniqids of the original instructions are set to IDVAL
	  and then the uniqid of the resultant instruction is computed.
	  This has the effect of setting the uniqid of the instruction 
	  not corresponding to the resultant to IDVAL. This fact is used 
	  in some instances where lnmrginst2 is called -- see w2opt.c */

/* delete instruction -- save line number
	if the instruction below has no line number or has a line number
	greater than the present instruction, the present number 
	is transfered down */

void
ldelin(nodep)
NODE * nodep;
{
	if(nodep->uniqid == IDVAL)
		return;

	if(nodep->forw != &ntail && 
	   (nodep->forw->uniqid > nodep->uniqid 
		 || nodep->forw->uniqid == IDVAL))
		nodep->forw->uniqid = nodep->uniqid;

	nodep->uniqid = IDVAL;

}

/* more conservative version of lndelinst that does not overwrite
	line numbers below */
void
ldelin2(nodep)
NODE *nodep;
{
	if(nodep == IDVAL)
		return;

	if(nodep->forw != &ntail && nodep->forw->uniqid == IDVAL)
		nodep->forw->uniqid = nodep->uniqid;

	nodep->uniqid = IDVAL;

}


/* exchange instructions
	The line number of the first instruction, if any, is given to the
	second instruction.  The line number of the second instruction is 
	deleted. */
void
lexchin(nodep1,nodep2)
NODE *nodep1, *nodep2;
{
	if(nodep1->uniqid != IDVAL) {
		nodep2->uniqid = nodep1->uniqid;
		nodep1->uniqid = IDVAL;
	}
	else {
		nodep2->uniqid = IDVAL;
	}
}


/* merge instructions - case 1
	The result instruction is given the line number of the
	top instruction, if it exists, or else it is given the line
	number of the bottom instruction. */
void
lmrgin1(nodep1,nodep2,resnode)
NODE *nodep1, *nodep2, *resnode;
{
	IDTYPE val1, val2;

	val1 = nodep1->uniqid;
	val2 = nodep2->uniqid;

	nodep1->uniqid = nodep2->uniqid = IDVAL;

	if(val1 != IDVAL)
		resnode->uniqid = val1;
	else 
		resnode->uniqid = val2;

}


/* instruction merge - case 2
	The resultant instruction is given the line number of the
	top instruction, if it exists, else it is not given a line number */
/*
void
lmrgin2(nodep1,nodep2,resnode)
NODE *nodep1, *nodep2, *resnode;
{
	IDTYPE val1;

	val1 = nodep1->uniqid;

	nodep1->uniqid = nodep2->uniqid = IDVAL;

	resnode->uniqid = val1;

}
*/

/* instruction merge - case 3
	The resultant instruction is given the line number of the first
	instruction, if it exists. The instruction below the pair to be 
	merged is given the line number of the second instruction, 
	if it exists */
void
lmrgin3(nodep1,nodep2,resnode)
NODE *nodep1, *nodep2, *resnode;
{
	IDTYPE val1;

	ldelin2(nodep2);

	val1 = nodep1->uniqid;

	nodep1->uniqid = nodep2->uniqid = IDVAL;

	resnode->uniqid = val1;
}

/* isiros  --  is operand either immediate, register or on stack?
**
** This routine returns true if the the operand 
** pointed to by its argument has address mode
**
**		immediate,
**		register, or
**		offset from a stack pointer (sp).
**
** This test is provided for checking operands to see whether they
** can be memory mapped i/o references.
*/

boolean
isiros( op )
char *op;
{
	return( op != NULL &&
		(
		  *op == '$' ||
		  *op == '%' ||
		  ( *op != '*' && usesreg( op, "%esp" )) ||
		  (! fp_removed && ( *op != '*' && usesreg( op, "%ebp" ))) ||
		  (fixed_stack && scanreg(op,false) & frame_reg)
		)
	      );
}

/* Check if the operand of an instruction node is volatile. */
	boolean
isvolatile(node,opno)
NODE *node;
int opno;
{
extern enum CC_Mode ccmode;
if (is_vol_opnd(node,opno))
	return true;		/* operand is marked volatile */
else if (ccmode == Transition && !isiros(node->ops[opno]))
	return true;		/* transition mode: any static duration */
				/* object is treated as volatile */
else if ((node->op > SAFE_ASM) || (node->sasm == SAFE_ASM)) return true;
else return false;
}

/* reverse jump in node p */
	void
revbr(p) NODE *p;
{

	switch (p->op) {
	    case JZ: p->op = JNZ; p->opcode = "jnz"; break;
	    case JNZ: p->op = JZ; p->opcode = "jz"; break;
	    case JS: p->op = JNS; p->opcode = "jns"; break;
	    case JNS: p->op = JS; p->opcode = "js"; break;
	    case JP: p->op = JNP; p->opcode = "jnp"; break;
	    case JNP: p->op = JP; p->opcode = "jp"; break;
	    case JO: p->op = JNO; p->opcode = "jno"; break;
	    case JNO: p->op = JO; p->opcode = "jo"; break;
	    case JPO: p->op = JPE; p->opcode = "jpe"; break;
	    case JPE: p->op = JPO; p->opcode = "jpo"; break;
	    case JE: p->op = JNE; p->opcode = "jne"; break;
	    case JNE: p->op = JE; p->opcode = "je"; break;
	    case JL: p->op = JGE; p->opcode = "jge"; break;
	    case JLE: p->op = JG; p->opcode = "jg"; break;
	    case JG: p->op = JLE; p->opcode = "jle"; break;
	    case JGE: p->op = JL; p->opcode = "jl"; break;
	    case JB: p->op = JNB; p->opcode = "jnb"; break;
	    case JBE: p->op = JNBE; p->opcode = "jnbe"; break;
	    case JNB: p->op = JB; p->opcode = "jb"; break;
	    case JNBE: p->op = JBE; p->opcode = "jbe"; break;
	    case JA: p->op = JNA; p->opcode = "jna"; break;
	    case JAE: p->op = JNAE; p->opcode = "jnae"; break;
	    case JNA: p->op = JA; p->opcode = "ja"; break;
	    case JNAE: p->op = JAE; p->opcode = "jae"; break;
	    case JC: p->op = JNC; p->opcode = "jnc"; break;
	    case JNC: p->op = JC; p->opcode = "jc"; break;
	    }
	}

/* invert jump in node p */
	void
invbr(p) NODE *p;
{
	switch (p->op) {
	    case JZ:	/* these are reversable */
	    case JNZ:
	    case JE:
	    case JNE:
		break;
	    case JNGE: case JL:  p->op = JG; p->opcode = "jg"; break;
	    case JNG:  case JLE: p->op = JGE; p->opcode = "jge"; break;
	    case JNLE: case JG:  p->op = JL; p->opcode = "jl"; break;
	    case JNL:  case JGE: p->op = JLE; p->opcode = "jle"; break;

	    case JNAE: case JB:  case JC: p->op = JA; p->opcode = "ja"; break;
	    case JNA:  case JBE: p->op = JAE; p->opcode = "jae"; break;
	    case JNBE: case JA:  p->op = JB; p->opcode = "jb"; break;
	    case JNB:  case JAE: case JNC: p->op = JBE; p->opcode = "jbe"; break;
	    }
	}

int
is_fp_commutative(p) NODE *p;
{
	if (!ieee_flag) return true;
	/* This is dependent on peep-hole having already limited search to
	   those code sequences involving SAHF. */
	switch (p->op) {
		case JZ:	case JNZ:
		case JP:	case JNP:	return true;
	}  /* switch */
	return false;
}

#if 0 /* for new setcond */
/* convert jcc to setcc  */
void 
br2set(p2) NODE *p2;
{
  switch(p2->op){
            case JA   : p2->op = SETA  ; p2->opcode = "seta"  ; break;
            case JAE  : p2->op = SETAE ; p2->opcode = "setae" ; break;
            case JB   : p2->op = SETB  ; p2->opcode = "setb"  ; break;
            case JBE  : p2->op = SETBE ; p2->opcode = "setbe" ; break;
            case JC   : p2->op = SETC  ; p2->opcode = "setc"  ; break;
            case JE   : p2->op = SETE  ; p2->opcode = "sete"  ; break;
            case JG   : p2->op = SETG  ; p2->opcode = "setg"  ; break;
            case JGE  : p2->op = SETGE ; p2->opcode = "setge" ; break;
            case JL   : p2->op = SETL  ; p2->opcode = "setl"  ; break;
            case JLE  : p2->op = SETLE ; p2->opcode = "setle" ; break;
            case JNA  : p2->op = SETNA ; p2->opcode = "setna" ; break;
            case JNAE : p2->op = SETNAE; p2->opcode = "setnae"; break;
            case JNB  : p2->op = SETNB ; p2->opcode = "setnb" ; break;
            case JNBE : p2->op = SETNBE; p2->opcode = "setnbe"; break;
            case JNC  : p2->op = SETNC ; p2->opcode = "setnc" ; break;
            case JNE  : p2->op = SETNE ; p2->opcode = "setne" ; break;
            case JNG  : p2->op = SETNG ; p2->opcode = "setng" ; break;
            case JNGE : p2->op = SETL  ; p2->opcode = "setl"  ; break;
            case JNL  : p2->op = SETNL ; p2->opcode = "setnl" ; break;
            case JNLE : p2->op = SETNLE; p2->opcode = "setnle"; break;
            case JNO  : p2->op = SETNO ; p2->opcode = "setno" ; break;
            case JNP  : p2->op = SETNP ; p2->opcode = "setnp" ; break;
            case JNS  : p2->op = SETNS ; p2->opcode = "setns" ; break;
            case JNZ  : p2->op = SETNZ ; p2->opcode = "setnz" ; break;
            case JO   : p2->op = SETO  ; p2->opcode = "seto"  ; break;
            case JP   : p2->op = SETP  ; p2->opcode = "setp"  ; break;
            case JPE  : p2->op = SETPE ; p2->opcode = "setpe" ; break;
            case JPO  : p2->op = SETPO ; p2->opcode = "setpo" ; break;
            case JS   : p2->op = SETS  ; p2->opcode = "sets"  ; break;
            case JZ   : p2->op = SETZ  ; p2->opcode = "setz"  ; break;
      }
} /* of br2set() */

/* return byte register for scratch reg */
char *
sreg2b(resreg) int resreg;
{
char *regw;
      switch (resreg){
            case EAX : case AX : case AL : regw="%al";break;
            case EBX : case BX : case BL : regw="%bl";break;
            case ECX : case CX : case CL : regw="%cl";break;
            case EDX : case DX : case DL : regw="%dl";break;
            case AH : regw="%ah";break;
            case BH : regw="%bh";break;
            case CH : regw="%ch";break;
            case DH : regw="%dh";break;
            default : printf("error : sreg2b convert only eax/ebx/ecx/edx \n");
                      break; 
     }
     return (regw) ;
} /* end sreg2b() */     

#endif

/* convert jcc to oposite setcc */
void
br2nset(p2) NODE *p2;
{

 switch(p2->op){
            case JA   : p2->op = SETNA ; p2->opcode = "setna" ; break;
            case JAE  : p2->op = SETNAE; p2->opcode = "setnae"; break;
            case JB   : p2->op = SETNB ; p2->opcode = "setnb" ; break;
            case JBE  : p2->op = SETNBE; p2->opcode = "setnbe"; break;
            case JC   : p2->op = SETNC ; p2->opcode = "setnc" ; break;
            case JE   : p2->op = SETNE ; p2->opcode = "setne" ; break;
            case JG   : p2->op = SETNG ; p2->opcode = "setng" ; break;
            case JGE  : p2->op = SETL  ; p2->opcode = "setl"  ; break;
            case JL   : p2->op = SETNL ; p2->opcode = "setnl" ; break;
            case JLE  : p2->op = SETNLE; p2->opcode = "setnle"; break;
            case JNA  : p2->op = SETA  ; p2->opcode = "seta"  ; break;
            case JNAE : p2->op = SETAE ; p2->opcode = "setae" ; break;
            case JNB  : p2->op = SETB  ; p2->opcode = "setb"  ; break;
            case JNBE : p2->op = SETBE ; p2->opcode = "setbe" ; break;
            case JNC  : p2->op = SETC  ; p2->opcode = "setc"  ; break;
            case JNE  : p2->op = SETE  ; p2->opcode = "sete"  ; break;
            case JNG  : p2->op = SETG  ; p2->opcode = "setg"  ; break;
            case JNGE : p2->op = SETGE ; p2->opcode = "setge" ; break;
            case JNL  : p2->op = SETL  ; p2->opcode = "setl"  ; break;
            case JNLE : p2->op = SETLE ; p2->opcode = "setle" ; break;
            case JNO  : p2->op = SETO  ; p2->opcode = "seto"  ; break;
            case JNP  : p2->op = SETP  ; p2->opcode = "setp"  ; break;
            case JNS  : p2->op = SETS  ; p2->opcode = "sets"  ; break;
            case JNZ  : p2->op = SETZ  ; p2->opcode = "setz"  ; break;
            case JO   : p2->op = SETNO ; p2->opcode = "setno" ; break;
            case JP   : p2->op = SETNP ; p2->opcode = "setnp" ; break;
            case JPE  : p2->op = SETPO ; p2->opcode = "setpo" ; break;
            case JPO  : p2->op = SETPE ; p2->opcode = "setpe" ; break;
            case JS   : p2->op = SETNS ; p2->opcode = "setns" ; break;
            case JZ   : p2->op = SETNZ ; p2->opcode = "setnz" ; break;
    }
} /* end of br2nset */    

/* convert jcc to oposite jcc */
void 
br2opposite(p2) NODE *p2;
{
	switch(p2->op)
	{
	    case JA   : p2->op = JNA ; p2->opcode = "jna" ; break;
        case JAE  : p2->op = JNAE; p2->opcode = "jnae"; break;
        case JB   : p2->op = JNB ; p2->opcode = "jnb" ; break;
        case JBE  : p2->op = JNBE; p2->opcode = "jnbe"; break;
        case JC   : p2->op = JNC ; p2->opcode = "jnc" ; break;
        case JE   : p2->op = JNE ; p2->opcode = "jne" ; break;
        case JG   : p2->op = JNG ; p2->opcode = "jng" ; break;
        case JGE  : p2->op = JL  ; p2->opcode = "jl"  ; break;
        case JL   : p2->op = JNL ; p2->opcode = "jnl" ; break;
        case JLE  : p2->op = JNLE; p2->opcode = "jnle"; break;
        case JNA  : p2->op = JA  ; p2->opcode = "ja"  ; break;
        case JNAE : p2->op = JAE ; p2->opcode = "jae" ; break;
        case JNB  : p2->op = JB  ; p2->opcode = "jb"  ; break;
        case JNBE : p2->op = JBE ; p2->opcode = "jbe" ; break;
        case JNC  : p2->op = JC  ; p2->opcode = "jc"  ; break;
        case JNE  : p2->op = JE  ; p2->opcode = "je"  ; break;
        case JNG  : p2->op = JG  ; p2->opcode = "jg"  ; break;
        case JNGE : p2->op = JGE ; p2->opcode = "jge" ; break;
        case JNL  : p2->op = JL  ; p2->opcode = "jl"  ; break;
        case JNLE : p2->op = JLE ; p2->opcode = "jle" ; break;
        case JNO  : p2->op = JO  ; p2->opcode = "jo"  ; break;
        case JNP  : p2->op = JP  ; p2->opcode = "jp"  ; break;
        case JNS  : p2->op = JS  ; p2->opcode = "js"  ; break;
        case JNZ  : p2->op = JZ  ; p2->opcode = "jz"  ; break;
        case JO   : p2->op = JNO ; p2->opcode = "jno" ; break;
        case JP   : p2->op = JNP ; p2->opcode = "jnp" ; break;
        case JPE  : p2->op = JPO ; p2->opcode = "jpo" ; break;
        case JPO  : p2->op = JPE ; p2->opcode = "jpe" ; break;
        case JS   : p2->op = JNS ; p2->opcode = "jns" ; break;
		case JZ   : p2->op = JNZ ; p2->opcode = "jnz" ; break;
    }
}     



	char *
dst(p) NODE *p; { /* return pointer to dst operand string */

  static char	*nullstr = "";
  unsigned int op;
	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;
	switch (op) {
	    case AAA: case AAD: case AAM: case AAS:
	    case DAA: case DAS: case LAHF:
	    case DIVB: case IDIVB: case IMULB: case MULB:
	    case INB: case INW: case INL:
	    case CWTL:
		return("%eax");
	    case CBTW:
			return "%ax";
	    case CWTD:
			return "%dx";
	    case CLTD:
			return "%edx";
	    case DIVW: case DIVL: case IDIVW: case IDIVL:
	    case IMULW: case IMULL: case MULW: case MULL:
		if ( (op == IMULW || op == IMULL) && p->op3 != NULL )
			return (p->op3);
		if (p->op2 != NULL)
			return(p->op2);
		return("%eax");
	    case FABS: case FADD: case FADDL: case FADDP:
	    case FADDS: case FCHS: case FDIV: case FDIVL:
	    case FDIVP: case FDIVR: case FDIVRL: case FDIVRP:
	    case FCMOVB: case FCMOVE: case FCMOVBE: case FCMOVU:
	    case FCMOVNB: case FCMOVNE: case FCMOVNBE: case FCMOVNU:
	    case FDIVRS: case FDIVS: case FIADD: case FIADDL:
	    case FIDIV: case FIDIVL: case FIDIVR: case FIDIVRL:
	    case FILD: case FILDL: case FILDLL: case FIMUL:
	    case FIMULL: case FISUB: case FISUBL: case FISUBR:
	    case FISUBRL: case FLD: case FLD1: case FLDL:
	    case FLDL2E: case FLDL2T: case FLDLG2: case FLDLN2:
	    case FLDPI: case FLDS: case FLDT: case FLDZ:
	    case FMUL: case FMULL: case FMULP: case FMULS:
	    case FSUB: case FSUBL: case FSUBP: case FSUBR:
	    case FSUBRL: case FSUBRP: case FSUBRS: case FSUBS:
	    case FCOS: case FPREM1: case FSIN: case FSINCOS:
			if (p->op2 != NULL && strcmp(p->op2, "%st") != 0)
			return(p->op2);
		return("%st(0)");
	    case CLRB: case CLRL: case CLRW:
	    case DECB: case DECL: case DECW:  case BSWAP:
	    case FIST: case FISTL: case FISTP: case FISTPL:
	    case FISTPLL: case FST: case FSTL: case FSTP:
	    case FSTPL: case FSTPS: case FSTPT: case FSTS:
	    case INCB: case INCL: case INCW:
	    case NEGB: case NEGL: case NEGW:
	    case NOTB: case NOTL: case NOTW:
	    case POPL: case POPW:
	    case FSTSW: case FSTCW:
	    case FNSTSW: case FNSTCW:
	    case SETA: case SETAE: case SETB: case SETBE: case SETC: case SETE:
	    case SETG: case SETGE: case SETL: case SETLE: case SETNA: case SETNAE:
	    case SETNB: case SETNBE: case SETNC: case SETNE: case SETNG: case SETNL:
	    case SETNLE: case SETNO: case SETNP: case SETNS: case SETNZ: case SETO:
  	    case SETP: case SETPE: case SETPO: case SETS: case SETZ:
		return (p->op1);
	    case ADCB: case ADCL: case ADCW:
	    case ADDB: case ADDL: case ADDW:
	    case ANDB: case ANDL: case ANDW:
	    case CMOVCW: case CMOVNAEW: case CMOVAEW: case CMOVNCW:
	    case CMOVNBW: case CMOVEW: case CMOVZW: case CMOVNEW:
	    case CMOVNZW: case CMOVNAW: case CMOVBEW: case CMOVAW:
	    case CMOVNBEW: case CMOVSW: case CMOVNSW: case CMOVPW:
	    case CMOVPEW: case CMOVNPW: case CMOVPOW: case CMOVLW:
	    case CMOVNGEW: case CMOVNLW: case CMOVGEW: case CMOVNGW:
	    case CMOVLEW: case CMOVGW: case CMOVNLEW:
	    case LEAL: case LEAW:
	    case MOVB: case MOVL: case MOVSBL:
	    case MOVSBW: case MOVSWL: case MOVW:
	    case MOVZBL: case MOVZBW: case MOVZWL:
	    case ORB: case ORL: case ORW:
	    case SBBB: case SBBL: case SBBW:
	    case SUBB: case SUBL: case SUBW:
	    case XCHGB: case XCHGL: case XCHGW:
	    case XORB: case XORL: case XORW:
	    case LARL: case LARW:case LSLL:
	    case LSLW: case LSSL:
	    case BTCL: case BTCW:
	    case BTL:  case BTW:
	    case BTRL: case BTRW:
	    case BTSL: case BTSW:
		return (p->op2);
	    case RCLB: case RCLL: case RCLW:
	    case RCRB: case RCRL: case RCRW:
	    case ROLB: case ROLL: case ROLW:
	    case RORB: case RORL: case RORW:
	    case SALB: case SALL: case SALW:
	    case SARB: case SARL: case SARW:
	    case SHLB: case SHLL: case SHLW:
	    case SHRB: case SHRL: case SHRW:
		/* Check for single operand form of the instruction. */
		return ((p->op2 ? p->op2 : p->op1));
	    case SHLDW: case SHLDL: case SHRDW: case SHRDL:
		if ( p->op3 != NULL )
			return (p->op3);
		return(p->op2);
	    case MACRO:
		if( p->sasm )
			return( dst( (NODE*) p->sasm ) );
		else
			return nullstr;	
	    default:
		return (nullstr);
	}
}

	boolean
samereg(cp1,cp2) register char *cp1, *cp2; { /* return true if same register */

	char c1,c2;
	if (cp1 == NULL || cp2 == NULL || *cp1 != '%' || *cp2 != '%')
		return(false);
/*	it was:
	if (scanreg(cp1,false) & scanreg(cp2,false))
		return(true);
	return(false);
	In this case if cp1 == %eax,%edx), cp2 == %edx will return true,  should 
	return false.
*/
	if (cp1[1] == 'e') 
		cp1++;
	if (cp2[1] == 'e') 
		cp2++;
	if (cp1[1] != cp2[1] )
		return false;
	c1 = cp1[2];
	c2 = cp2[2];
	if( c1 == 't' ) {	
		if (c2 != 't')
			return false;
		return (setreg(cp1) == setreg(cp2));
	}	
	if (c1 == c2 ) 
		return true; /* %{e}ax == %{e}ax */
	if  (c1 == 'x' && (c2 == 'l' || c2 == 'h'))
		return true; /* %al is the same reg as %ax and as %eax */ 
	if  (c2 == 'x' && (c1 == 'l' || c1 == 'h'))
		return true;
	return false; /* %al is not the same reg as %ah */
}
	boolean
usesreg(cp1,cp2) register char *cp1, *cp2; { /*return true if cp2 used in cp1*/

	while(*cp1 != '\0') {
		if (*cp1 == '%' && samereg(cp2,cp1) == true)
			return(true);
		cp1++;
		}
	return(false);
	}

    unsigned int
uses(p) NODE *p; { /* set register use bits */

    extern int rvregs;
    unsigned using;
    int op;

    op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;
    using = scanreg(p->op1,true) | scanreg(p->op2,true) | scanreg(p->op3,true);
    switch (op) {
    case SHLDW: case SHLDL: case SHRDW: case SHRDL:
	if (p->op3 != NULL)
		using |= (setreg(p->op2) | setreg(p->op3));
	else
		using |= (setreg(p->op1) | setreg(p->op2));
	break;
    case ROLL:  case ROLW:  case ROLB:
    case RORL:  case RORW:  case RORB:
    case SALL:  case SALW:  case SALB:
    case SARL:  case SARW:  case SARB:
    case SHLL:  case SHLW:  case SHLB:
    case SHRL:  case SHRW:  case SHRB:
    case XCHGL: case XCHGW: case XCHGB:
    case ADDL:  case ADDW:  case ADDB:
    case ANDL:  case ANDW:  case ANDB:
    case ORL:   case ORW:   case ORB:
    case CMPL:  case CMPW:  case CMPB:
    case BSFL:  case BSFW:  case BSRL:
    case BSRW:  case BSWAP: case  BTCL:
    case  BTCW: case  BTL:  case  BTRL:
    case  BTRW: case  BTSL: case  BTSW:
    case  BTW:  case XADDB: case XADDW:  case XADDL:
        using |= (setreg(p->op1) | setreg(p->op2));
        break;
    case  CMPXCHGB: case  CMPXCHGL: case CMPXCHGW:
        using |= (setreg(p->op1) | setreg(p->op2) | EAX); /* AX, AL */
        break;
    case ADCL:  case ADCW:  case ADCB:
    case RCLL:  case RCLW:  case RCLB:
    case RCRL:  case RCRW:  case RCRB:
    case SBBL:  case SBBW:  case SBBB:
        using |= (setreg(p->op1) |
                setreg(p->op2) | CARRY_FLAG);
        break;
    case MULB:  case IMULB:
        using |= AL | setreg(p->op1);
        break;
    case MULW:  case IMULW:
        using |= setreg(p->op1);
        if (p->op2 == NULL)
            using |= AX|DX;
        else
            using |= setreg(p->op2);
        break;
    case MULL: case IMULL:
        using |= setreg(p->op1);
        if (p->op2 == NULL)
            using |= EAX|EDX;
        else
            using |= setreg(p->op2);
        break;
    case DIVB:
    case IDIVB:
        using |= AX | setreg(p->op1);
        break;
    case DIVW:  case IDIVW:
        using |= AX|DX | setreg(p->op1);
        break;
    case DIVL: case IDIVL:
        using |= EAX|EDX | setreg(p->op1);
        break;
    case DECL:  case DECW:  case DECB:
    case INCL:  case INCW:  case INCB:
    case NEGL:  case NEGW:  case NEGB:
    case NOTL:  case NOTW:  case NOTB:
    case MOVSBL:  case MOVSBW:  case MOVSWL:
    case MOVZBL:  case MOVZBW:  case MOVZWL:
    case MOVL:  case MOVW:  case MOVB:
    case LEAL:  case LEAW:
    case BOUNDL: case BOUNDW:
    case LARL: case LARW: case LSLW: case LSLL:
    case VERW:
        using |= setreg(p->op1);
        break;
    
    case CMOVEW: case CMOVZW:  case CMOVNEW: case CMOVNZW: 
    case CMOVGW: case CMOVGEW: case CMOVNGW: case CMOVNGEW:
    case CMOVLW: case CMOVLEW: case CMOVNLW: case CMOVNLEW:
    case CMOVSW: case CMOVNSW: case CMOVOW:  case CMOVNOW:
    case CMOVPW: case CMOVNPW: case CMOVPEW: case CMOVPOW: 
    case CMOVEL: case CMOVZL:  case CMOVNEL: case CMOVNZL: 
    case CMOVGL: case CMOVGEL: case CMOVNGL: case CMOVNGEL:
    case CMOVLL: case CMOVLEL: case CMOVNLL: case CMOVNLEL:
    case CMOVSL: case CMOVNSL: case CMOVOL:  case CMOVNOL:
    case CMOVPL: case CMOVNPL: case CMOVPEL: case CMOVPOL:
        using |= (setreg(p->op1) | setreg(p->op2) | CONCODES);
        break;
    
    case CMOVAW: case CMOVBEW: case CMOVNAW: case CMOVNBEW:
	case CMOVAL: case CMOVBEL: case CMOVNAL: case CMOVNBEL: 
    	using |= (setreg(p->op1) | setreg(p->op2) | CONCODES_AND_CARRY);
        break;

    case CMOVAEW:  case CMOVBW:  case CMOVCW:  
    case CMOVNAEW: case CMOVNBW: case CMOVNCW:
	case CMOVAEL:  case CMOVBL:  case CMOVCL:  
    case CMOVNAEL: case CMOVNBL: case CMOVNCL:
		using |= (setreg(p->op1) | setreg(p->op2) | CARRY_FLAG);
        break;

    case PUSHL: case PUSHW:
        using |= setreg(p->op1) | ESP;
        break;
    case PUSHFL: case PUSHFW:
        using |=  ESP | CONCODES_AND_CARRY;
        break;
    case PUSHA: case PUSHAL: case PUSHAW:
        using = ESP | EAX | EBX | ECX | EDX | EDI | ESI | EBP;
        break;
    case POPL: case POPW: case POPA: case POPAL: case POPAW:
    case POPFL: case POPFW:
        using |= ESP;
        break;
    case SAHF:
        using = AH;
        break;
    case LCALL: case CALL:
        using |= ESP;
        if (pic_flag && strchr(p->op1,'@')) using |= EBX;
		using |= p->zero_op1 & REGS;
		if (suppress_enter_leave && !fixed_stack) using |= EBP;
        break;
    case LDS:   case LES:
    case LDSL:  case LDSW:  case LESL:  case LESW:
    case LFSL:  case LFSW:  case LGSL:  case LGSW:
    case LSSW:  case LSSL:  case NOP:
    case INVD:  case INVLPG: case WBINVD:
    case JMP:   case LJMP:
    case CLTS:
        break;
    case AAS:
		using = AL | CONCODES;
		break;
    case AAD:  case AAM:
    case CBTW:
        using = AL;
        break;
    case CWTL:
    case CWTD:
        using = AX;
        break;
    case CLTD:
        using = EAX;
        break;
    case AAA:
        using = EAX | CONCODES;
        break;
    case DAA:  case DAS:
        using = EAX | CONCODES_AND_CARRY;
        break;
    case XLAT:
        using = EBX | AL;
        break;
    case LOOP: case JCXZ:
        using = ECX;
        break;
    case LOOPE:  case LOOPNE:  case LOOPNZ:
    case LOOPZ:  case REP:  case REPNZ:  case REPZ: case REPE: case REPNE:
        using = ECX;
        break;
    case SCAB: case SCASB: case SSCAB:
        using = EDI | AL;
        break;
    case SCAW: case SSCAW: case SCASW:
        using = EDI | AX;
        break;
    case SCAL: case SCASL: case SSCAL:
        using = EDI | EAX;
        break;
    case SSTOB: case STOSB:
        using = EDI | AL;
        break;
    case SSTOW: case STOSW:
        using = EDI | AX;
        break;
    case SSTOL: case STOSL:
        using = EDI | EAX;
        break;
    case SLODB:  case SLODW:  case SLODL:
    case LODSB:  case LODSW:  case LODSL:
        using = ESI;
        break;
    case MOVSL:
    case SCMPL:  case SCMPW:  case SCMPB:
    case SMOVL:  case SMOVW:  case SMOVB:
    case CMPSB:  case CMPSL:  case CMPSW:
        using = ESI | EDI;
        break;
    case ENTER:
        using = ESP | EBP;
        break;
    case LEAVE:
        using = EBP;
        break;
    case OUTB:  case OUTW:  case OUTL:
        using |= EAX;
        if (p->op1 == NULL)
            using |= EDX;
        break;
    case INB:  case INW:  case INL:
    case OUTSB:  case OUTSW:  case OUTSL:
        if (p->op1 == NULL)
            using |= DX;
        break;
    case INSB:  case INSW:  case INSL:
            using |= EDX | EDI;
        break;
    case SUBL:  case SUBW:  case SUBB:
    case XORL:  case XORW:  case XORB:
        /* if xor %ax,%ax (say) then value in %ax is irrelevent */
        if(!samereg(p->op1,p->op2))
            using |= setreg(p->op2) | setreg(p->op1);;
        break;
    case TESTL:  case TESTW:  case TESTB:
        using |= setreg(p->op1);
        if (p->op2 != NULL)
            using |= setreg(p->op2);
        break;

    /*
     * The following instructions are for the iAPX287 math
     * co-processor. These instructions can use the iAPX286
     * registers for memory reference. Therefore return 'usage'.
     */
    case FIADD: case FIADDL:    case FICOM: case FICOML:
    case FICOMP:    case FICOMPL:   case FIDIV: case FIDIVL:
    case FIDIVR:    case FIDIVRL:   case FILD:  case FILDL:
    case FILDLL:    case FIMUL: case FIMULL:    case FIST:
    case FISTL: case FISTP: case FISTPL:    case FISTPLL:
    case FISUB: case FISUBL:    case FISUBR:    case FISUBRL:
    case FSTENV:    case FNSTENV:   case FLDENV:
    case FSAVE: case FNSAVE:    case FRSTOR:
    case FBLD:  case FBSTP:
    case FADDS: case FADDL: case FCOMS: case FCOML:
    case FCOMPS:    case FCOMPL:    case FDIVS: case FDIVL:
    case FDIVRS:    case FDIVRL:    case FLDS:  case FLDL:
    case FLDT:  case FMULS: case FMULL: case FSTS:
    case FSTL:  case FSTPS: case FSTPL: case FSTPT:
    case FSUBS: case FSUBL: case FSUBRS:    case FSUBRL:
    case FUCOM: case FUCOMP: case FUCOMPP:
        using |= setreg(p->op1) | FP0;
        break;
    case FLDCW: case FSTCW: case FNSTCW:    case FSTSW:
    case FNSTSW:
        break;
        /*
     * The instructions beginning with an F are for the iAPX287
     * math co-processor. These instructions make no use of
     * iAPX386 registers. These instructions all use the
     * iAPX287 stack. (These are the fp register direct insts.)
     */
    case FADD:  case FADDP: case FCOM:
    case FCOMP: case FDIV:  case FDIVP:
    case FDIVR: case FDIVRP:    case FFREE:
    case FLD:   case FMUL:  case FMULP:
    case FST:   case FSTP:  case FSUB:
    case FSUBP: case FSUBR: case FSUBRP:
    case FXCH:
        using |= setreg(p->op1) | setreg(p->op2);
        break;

    case ARPL:   case BOUND:  case CLC:   case CLD:     case CLI:
    case CLRL:   case CLRW:   case CLRB:
    case CMC:    case CTS:    case ESC:
    /*
     * The instructions beginning with an F are for the iAPX287
     * math co-processor. These instructions make no use of
     * iAPX286 registers. The ones up to the next comment take
     * no operands.
     */

    case FINIT: case FCLEX: case F2XM1: case FABS:
    case FCHS:  case FCOMPP:    case FDECSTP:   case FINCSTP:
    case FLD1:  case FLDL2E:    case FLDL2T:    case FLDLG2:
    case FLDLN2:    case FLDPI: case FLDZ:  case FNCLEX:
    case FNINIT:    case FNOP:  case FPATAN:    case FPREM:
    case FPTAN: case FRNDINT:   case FSCALE:    case FSETPM:
    case FSQRT: case FTST:  case FWAIT: case FXAM:
    case FXTRACT:   case FYL2X: case FYL2XP1:
    case HLT:    case INS:    case INT:   case INTO:
    case LAR:    case LGDT:   case LIDT:  case LLDT:    case LMSW:
    case LOCK:   case LRET:   case LSL:   case LTR:     case OUTS:
    case WAIT:   case SGDT:   case SIDT:  case SLDT:    case SMSW:
    case STC:    case STD:    case STI:   case STR:     case VERR:
    case FSIN:   case FCOS:   case FPREM1: case FSINCOS:
        using = 0;
        break ;
    
    case JE:     case JG:     case JGE:   case JL:      case JLE:
    case JNE:    case JNG:    case JNGE:  case JNL:     case JNLE:
    case JNO:    case JNP:    case JNS:   case JNZ:     case JO:
    case JP:     case JPE:    case JPO:   case JS:      case JZ:      
    case SETE:   case SETG:   case SETGE: case SETL:    case SETLE: 
    case SETNE:  case SETNG:  case SETNL: case SETNLE:  case SETNO:
    case SETNP:  case SETNS:  case SETNZ: case SETO:    case SETP:
    case SETPE:  case SETPO:  case SETS:  case SETZ:
        using |= CONCODES;
        break;

    case JA:     case JBE:    case JNA:   case JNBE:    
    case LAHF:
    case SETA:   case SETBE:  case SETNA: case SETNBE:    
        using |= CONCODES_AND_CARRY;
        break;

    case JAE:    case JB:     case JC:    case JNAE:    case JNB:   
    case JNC:
    case SETAE:  case SETB:   case SETC:  case SETNAE:  case SETNB:
    case SETNC: 
        using |=  CARRY_FLAG;
        break;

    case FCMOVE:  case FCMOVU:
    case FCMOVNE: case FCMOVNU:
         using |= setreg(p->op1) | FP0 | CONCODES ;
         break;

    case FCMOVBE: case FCMOVNBE: 
         using |= setreg(p->op1) | FP0 | CONCODES_AND_CARRY ;
         break;

    case FCMOVB:  case FCMOVNB: 
         using |= setreg(p->op1) | FP0 | CARRY_FLAG ;
         break;

    case GHOST:/*case TAIL:*/ case MISC:  case FILTER:
    case LABEL:  case HLABEL: case DHLABEL: case SAFE_ASM:
#if EH_SUP
	case EHLABEL:
#endif
        using = 0;
        break;
    case RET: case IRET:
        if (rvregs == 1)
          using = REGS & ~(EDX | ECX | FP1 | EBX | ESI | EDI | EBI | CONCODES_AND_CARRY);
        else if (rvregs == 2)
          using = REGS & ~(ECX | FP1 | EBX | ESI | EDI | EBI | CONCODES_AND_CARRY);
        else
          using = REGS & ~(EAX | EDX | ECX | FP1 | EBX | ESI | EDI | EBI | CONCODES_AND_CARRY);
        break;
    case MACRO:
        using = p->uses; break;
    default:
        using = (unsigned) (REGS | MEM);  /* supposed to be everything. */
    }
    return(using);
}


 /* set register use bits, use but not index explicitely. */
	unsigned int
uses_but_not_indexing(p) NODE *p;
{
int i;
char *t = NULL;
unsigned int using;
	for (i = 1; i <= 3; i++) {
		if (p->ops[i] && (t = strchr(p->ops[i],'(')) != NULL)  {
			*t = (char) 0;
			break;
		}
	}
	using = uses(p);
	if (t) *t = '(';
	return using;
}/*end uses_but_not_indexing*/

int
find_pointer_write(read_pointer)
int *read_pointer;
{	
	NODE *p;
	char *cp;
	int i;
	
	*read_pointer = false;
	for(ALLN(p)) {
		if (p->op == SMOVL || p->op == SMOVW || p->op == SMOVB
	        || p->op == CALL || p->op == LCALL || isvolatile(p,1) 
	        || isvolatile(p,2) || isvolatile(p,3) 
		) 
			return true; /* Write to unknown address */
		if (p->op == LEAL)
			continue; /* no read or write by LEAL */ 
		for( i = 1; i < 3 ; i++) {
			if ((cp = p->ops[i]) == NULL) continue;
			if (*cp == '*')
				cp++;
			if (cp == NULL || *cp == 0  /* no usage */
		       || *cp == '%' /* register usage */ 
			   || *cp == '$' /* constant usage */ 
		       || *cp == '_' ||  *cp == '.'|| isalpha(*cp) /* label usage */ 
		       || (scanreg(cp,false) & frame_reg) /* stack,  no global usage */ 
		       || (! strchr(cp,'(')) )  /* no pointer part in address */
				continue; /* No pointer usage */
			else { /* found usage of num(%reg) it can point to any global*/ 
				if (cp == dst(p) || isxchg(p))
					return true;  /* Write to pointer */
				*read_pointer = true;
			}
		}
	}
	return false;
} /* end find_pointer_write */

/* returned value is a bitvector. Each bit corresponds to a register name, for
   each of %eax, %ebc, %ecx, and %edx there are three bits. For example %eax:
   %ah, %al, %Eax. If scanreg finds %ah it sets on AH, if scanreg finds
   %al it sets on AL, if scanreg finds %ax or %eax it sets on Eax|AH|AL
   changed to return AX separately.
*/
/* hash table for scanreg and setreg */

static	unsigned eregs[] = {EAX,EBX,ECX,EDX,0,0,0,BP,BI,EBP,DI,0,AX,BX,CX,DX,
                        EBI,0,EDI,AL,BL,CL,DL,EBI,0,SI,ESP,AH,BH,CH,DH,0,0,ESI,
						FP0,FP1,FP2,FP3,FP4,FP5,FP6,FP7};
static char index_array[] = {70 ,90,0,0,78,0,0,0,91,0,0,0,0,0,0,0,85};


unsigned int
scanreg(cp,flag) register char *cp; int flag; { /* determine registers 
						   referenced in operand */

	int set,tmp;
	                                /*      operand     flag        */
	if (cp == NULL)                 /*              true    false   */
	        return (0);             /*                              */
	if (flag && *cp == '%')         /*      %r      dead    live    */
	        return (0);             /*      n(%r)   live    live    */
	set = 0;                        /*      addr    dead    dead    */
	while (*cp != '\0') {           /*      '\0'    dead    dead    */
		if (*cp == '%') {
			if (cp[1] == 'e') { /* %e?? */	
				set |= eregs[23 + cp[2] - cp[3]];
				cp+=4;
				continue;
			} 
			if ((tmp = index_array[cp[2] - 'h']) != 0) { /* %?? */
				set |= eregs[cp[1] - tmp];
				cp += 3;
			} else {			/* st(x) */
				if (cp[3] != '(') {
					set |= FP0;
					cp+=3;
				} else {
					set |= eregs[cp[4] - 14];
					cp+=6;
				}
			}
		}
		else
			cp++;
	}
	return set; 
}

unsigned int
setreg(cp) register char *cp; { /* determine registers operand bits */

	int tmp;
	if (cp == NULL || *cp != '%')
		return (0);		/* not a register */
	if (cp[1] == 'e')  /* %e?? */	
		return (eregs[23 + cp[2] - cp[3]]);
	if ((tmp = index_array[cp[2] - 'h']) != 0)  /* %?? */
		return eregs[cp[1] - tmp];
	if (cp[3] != '(')  /* st(x) */
		return FP0;
	return  eregs[cp[4] - 14];
}
/* set register destination bits */
	unsigned int
sets(p) NODE *p;
{

  	extern int rvregs;
	char *cp;
	int op;

	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;

	switch (op) {
	case LABEL: case HLABEL: case DHLABEL:
#if EH_LAB
	case EHLABEL:
#endif
		return 0;
	case CBTW:
		return AX;
	case LAHF:
	case CWTL:
	case INB:  case XLAT:
	case INL:   case INW:
		return(EAX);
	case INSB: case INSL: case INSW:
		return (EDI);
	case AAA:  case AAD:  case AAM:  case AAS:
	case DAA:  case DAS: 
	case DIVB:  case IDIVB:  case IMULB:
	case MULB:
		return(EAX | CONCODES_AND_CARRY);
	case IMULW: case IMULL:
	case SHRDW: case SHRDL:
	case SHLDW: case SHLDL:
		if (p->op3 != NULL)
			return( setreg(p->op3) | CONCODES_AND_CARRY);
		else if(p->op2 != NULL)
			return( setreg(p->op2) | CONCODES_AND_CARRY);
		else
			return(EAX | EDX | CONCODES_AND_CARRY);
	case DIVL:  case IDIVL: case MULL: 
		return(EAX | EDX | CONCODES_AND_CARRY);
	case DIVW:  case IDIVW: case MULW:
		return(AX | DX | CONCODES_AND_CARRY);
	case SLODB:  case SLODW:  case SLODL:
	case LODSB:  case LODSL:  case LODSW:
		return(EAX | ESI);
	case LOOP:  case LOOPE:  case LOOPNE:  case LOOPNZ:
	case LOOPZ: case REP:  case REPNZ:  case REPZ: case REPE: case REPNE:
		return(ECX);
	case SCAL:  case SCAW:  case SCAB: 
	case SSCAB: case SSCAL: case SSCAW:
		return(CONCODES_AND_CARRY | EDI);
	case CWTD:
		return(DX);
	case CLTD:
		return(EDX);
	case XCHGL: case XCHGW: case XCHGB:
	case XADDB: case XADDW: case XADDL:
		return( setreg(p->op1) | setreg(p->op2) | CONCODES_AND_CARRY);
	case CMPXCHGB: case CMPXCHGW: case CMPXCHGL:
		return( setreg(p->op1) | setreg(p->op2)  | CONCODES_AND_CARRY);
	case SCMPL:  case SCMPW:  case SCMPB:
	case CMPSB:  case CMPSL:  case CMPSW:
		return(CONCODES_AND_CARRY | ESI | EDI);
	case SMOVL:  case SMOVW:  case SMOVB: case MOVSL:
		return(ESI | EDI);
	case SSTOL:  case SSTOW:  case SSTOB:
		return(EDI);
	case ENTER:  case LEAVE: 
		return( ESP | EBP);
	case RET: /* maybe other 387 registers */
		if (rvregs == 1)
		  return (EDX | ECX | FP1 | EBX | ESI | EDI | CONCODES_AND_CARRY);
		else if (rvregs == 2)
		  return (ECX | FP1 | EBX | ESI | EDI | CONCODES_AND_CARRY);
		else
		  return (EAX | EDX | ECX | FP1 | EBX | ESI | EDI | CONCODES_AND_CARRY);
	case CALL:   case LCALL:
		if (!strcmp(p->op1,p->forw->opcode)) /*pic call*/
			return (CONCODES_AND_CARRY);
		else
			return( EAX | ECX | EDX | FP0 | CONCODES_AND_CARRY ) ;
	case CMPL:   case CMPW:   case CMPB:
	case TESTL:  case TESTW:  case TESTB:
	case SAHF:   case BTL:    case BTW:
		return(CONCODES_AND_CARRY);
	case INCB:  case INCW:  case INCL:
	case DECB:  case DECW:  case DECL:
		return( CONCODES | setreg(p->op1) );
	case NEGB:  case NEGW:  case NEGL:
		return( CONCODES_AND_CARRY | setreg(p->op1) );
	case RCLL:  case RCLW:  case RCLB:
	case RCRL:  case RCRW:  case RCRB:
	case ROLL:  case ROLW:  case ROLB:
	case RORL:  case RORW:  case RORB:
	case SALL:  case SALW:  case SALB:
	case SARL:  case SARW:  case SARB:
	case SHLL:  case SHLW:  case SHLB:
	case SHRL:  case SHRW:  case SHRB:
		/* Check for single operand version of the instruction. */
		if (p->op2 == NULL)
			return( CONCODES_AND_CARRY | setreg(p->op1) );
		/* FALLTHROUGH */
	case ADDB:  case ADDW:  case ADDL:
	case ADCB:  case ADCW:  case ADCL:
	case SUBB:  case SUBW:  case SUBL:
	case SBBB:  case SBBW:  case SBBL:
	case ANDB:  case ANDW:  case ANDL:
	case XORB:  case XORW:  case XORL:
	case ORB:   case ORW:   case ORL:
	case BSFL:  case BSFW:  case BSRL:
	case BSRW:
	case BTCL:   case BTCW:   case BTRL:   
	case BTRW:   case BTSL:   case BTSW:   
		return( CONCODES_AND_CARRY | setreg(p->op2) );
	case MOVB:  case MOVW:  case MOVL:
	case MOVSBL: case MOVSBW: case MOVSWL:
	case MOVZBL: case MOVZBW: case MOVZWL:
	case CMOVCW: case CMOVNAEW: case CMOVAEW: case CMOVNCW:
	case CMOVNBW: case CMOVEW: case CMOVZW: case CMOVNEW:
	case CMOVNZW: case CMOVNAW: case CMOVBEW: case CMOVAW:
	case CMOVNBEW: case CMOVSW: case CMOVNSW: case CMOVPW:
	case CMOVPEW: case CMOVNPW: case CMOVPOW: case CMOVLW:
	case CMOVNGEW: case CMOVNLW: case CMOVGEW: case CMOVNGW:
	case CMOVLEW: case CMOVGW: case CMOVNLEW:
	case CMOVCL: case CMOVNAEL: case CMOVAEL: case CMOVNCL:
	case CMOVNBL: case CMOVEL: case CMOVZL: case CMOVNEL:
	case CMOVNZL: case CMOVNAL: case CMOVBEL: case CMOVAL:
	case CMOVNBEL: case CMOVSL: case CMOVNSL: case CMOVPL:
	case CMOVPEL: case CMOVNPL: case CMOVPOL: case CMOVLL:
	case CMOVNGEL: case CMOVNLL: case CMOVGEL: case CMOVNGL:
	case CMOVLEL: case CMOVGL: case CMOVNLEL:
		return( setreg(p->op2) );
	case PUSHW: case PUSHL: case PUSHA: case PUSHFL: case PUSHFW:
		return( ESP );
	case POPW:  case POPL:
		return( ESP | setreg(dst(p)) );
	case POPA: case POPAL: case POPAW:
		return( ESP | EDI | ESI | EBP | EBX | EDX | ECX | EAX );
	case POPFL: case POPFW:
		return (ESP| CONCODES_AND_CARRY);
	case MACRO:
		return p->sets;
	default:
		if ( (cp = dst(p)) == NULL )
			return(0);
		else
			return( setreg(cp) );
	}
}

	boolean
isdead(cp,p) char *cp; NODE *p; { /* true iff *cp is dead after p */

	int reg;

	if (cp == NULL || *cp != '%')
		return(false);
	reg = setreg(cp);
	if ((p->nlive & reg) == 0)
		return(true);
	    else
		return(false);
	}


	char *
getp(p) NODE *p; { /* return pointer to jump destination operand */

	switch (p->op) {
	    default:
		return(p->op1);
	    case RET:  case LRET: case LJMP:
	        return(NULL);
	    }
	}


	char *
newlab() { /* generate a new label */

	static unsigned int lbn = 0;
	char *c;

	c = getspace((unsigned int)(lbn < 100 ? 5 : (lbn < 1000 ? 6 : (lbn < 10000 ? 7 : 13))));
	sprintf(c, ".Z%d", lbn);
	lbn++;
	return(c);
	}


#include "debug.h" 	/* for line info routines */
#include "regal.h"
extern int Aflag;

     	void
prinst(p) register NODE *p; { /* print instruction */
	if (p->uniqid != IDVAL)
		print_line_info((int)p->uniqid);
	switch (p->op) {
		case LABEL:
			if (lookup_regals(p->opcode, first_label)) {
#ifdef DEBUG
				if (lflag > 1) fprintf(stderr,"loop label %s\n",p->opcode);
				if (!Aflag) {
					if (p->op1 && (target_cpu == P4))
						printf("	.backalign	%s,16,7,4",p->op1);
					else
						printf("	.align	16,7,4");
					if (lflag)
						printf("	%c	Loop\n",CC);
					else
						printf("\n");
				} else if (lflag) printf("%c Loop\n",CC);
#else
				if (!Aflag) {
					if (p->op1 && (target_cpu == P4))
						printf("	.backalign	%s,16,7,4",p->op1);
					else
						printf("	.align	16,7,4");
					printf("\n");
				}
#endif
			}
			printf("%s:\n", p->opcode);
			break;
		case ASMS:
		case SAFE_ASM:
		case HLABEL:
		case DHLABEL:
			printf("%s\n", p->opcode);
			break;
#if EH_SUP
		case EHLABEL:
			printf("%s:\n", p->opcode);
			break;
#endif
		case MISC:
			printf("	%s\n", p->opcode);
			break;
		case JMP:
		case LJMP:
		case RET:
			printf("	%s	", p->opcode);
			if (p->op1 != NULL)
				printf("%s", p->op1);
				if (target_cpu == P4 && !Aflag)
					printf("\n	.align	16,7,4\n");
			else if (!Aflag)
				printf("\n	.align	4\n");
			else
				printf("\n");
			break;
		case PUSHL:
			if (p->op4 == NULL)
				printf("	%s	%s\n", p->opcode,p->op1);
			else
				printf("	%s	%s%s\n", p->opcode,p->op1,p->op4);
			break;
		default:
			printf("	%s	", p->opcode);
			if (p->op1 != NULL)
				printf("%s", p->op1);
			if (p->op2 != NULL)
				printf(",%s", p->op2);
			if (p->op3 != NULL && p->op != CALL) /* /TMPSRET can be op3 */
				printf(",%s", p->op3);
			if (p->op4 != NULL)
				printf("%s", p->op4);
			printf("\n");
			break;
	}
}

	boolean
ishlp(p) register NODE *p; { /* return true if a fixed label present */

	for (; islabel(p); p=p->forw)
		if (ishl(p))
			return(true);
	return(false);
}


int
is_jmp_ind(p) NODE *p;
{
	return (p
	     && (p->op == JMP || p->op == LJMP)
		 && p->op1
		 && *(p->op1) == '*'
		 && (p->op1[1] == '.' || isalpha(p->op1[1]) || p->op1[1] == '_')
		 );
}/*end is_jmp_ind*/

SWITCH_TBL * 
get_base_label(op) char *op; {
char *t, ch;
SWITCH_TBL *sw;
	op++;
	for (t = op ; (*t != (char) 0) && (*t != '(') ; t++)
	;
	ch = *t;
	*t = 0;
	for (sw = sw0.next_switch ; 
	     sw && strcmp(op,sw->switch_table_name) ; 
		 sw = sw->next_switch)
	;
	*t = ch;
	return sw;
}/*end get_base_label*/

/* Decompose an offset of an operand into a string, then
** '+' or '-', then a number.
** Or the number first, then the string.
** Also check for presence of a scaling factor.
** s is the operand, the only "in parameter"
** ip is a munber which may preceed the '('
** namep is a label which may preceed the '(', with or without a number
** scale is the scaling factor, 1 if none.
** return value is the length of namep.
*/
int
decompose(s,ip,namep,scale) char *s; int *ip; char *namep; int *scale;
{
  int r;
  int n=0;
  char *t;

	if (!s) {
		*ip = 0;
		return 0;
	}
	if ( *s == '*') {
		/* Jump/call indirect */
		s++;
	}
	t = s;
	if (isalpha(*t) || *t == '_' || *t == '.') {
		/* the label comes first */
		while (   isalpha(*t)
		       || *t == '_'
		       || *t == '.'
		       || isdigit(*t)
		       || *t == '@') {

			t++;
			n++;
		}
    		if (n==0) {
			*namep = NULL;
    		} else {
			(void) strncpy(namep,s,n);
			namep[n] = '\0';
		}
    		if (*t == '+') {
			t++;
		}
    		r=n;
		*ip = (int)strtoul(t,NULL,0);

  	} else {
		/*number first*/
		*ip = (int)strtoul(s,&t,0);
		if (*t == '+')
			t++;
		n = 0;
		s=t;
		while (   isalpha(*t)
		       || *t == '_'
		       || *t == '.' 
		       || isdigit(*t)
		       || *t == '@') {

			t++;
			n++;
		}
		if (n==0) {
			*namep = NULL;
		} else {
			(void) strncpy(namep,s,n);
			namep[n] = '\0';
		}
		r = n;
	} /* end else, case of number first*/

	t = strchr(s,')');
	if (t) {
		t--;
		switch(*t) {
	  		case '2':
			case '4':
			case '8':
				*scale = *t - '0';
				break;
	  		default:
				*scale = 1;
				break;
		} /* end switch */
  	} /* endif t */
  	return r;
} /* end decompose */

int
has_scale(op) char *op;
{
char *t;
	t = strchr(op,')');
	if (!t) return 0;
	t--;
	if (!isdigit(*t)) return 0;
	else return (int)strtoul(t,(char **)NULL,0);
}/*end has_scale*/

unsigned int
hard_uses(p) NODE *p;
/*registers used implicitely and may also be used explicitely*/
{
	int op;

	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;

	switch(op) {
		case MULB: case IMULB:
		case MULW: case IMULW:
		case MULL: case IMULL:
		case DIVB: case IDIVB:
			return EAX;
		case DIVW: case IDIVW:
		case DIVL: case IDIVL:
			return (EAX | EDX);
		case PUSHW: case PUSHL:
		case POPW: case POPL:
			return ESP;
		case REP:
			return ECX;
		case SMOVL: case SMOVW: case SMOVB:
		case SCMPB: case SCMPW: case SCMPL:
			return EDI | ESI;
		case SSTOL: case SSTOW: case SSTOB: case STOSL: case STOSW: case STOSB:
		case SCAB:  case SCAW:  case SCAL: case SCASB: case SCASL: case SCASW:
		case SSCAB:  case SSCAW:  case SSCAL:
			return EDI|EAX;
		case SAHF:
			return AH;
		default:
			return 0;
	}/*end switch*/
}/*end hard_uses*/

	unsigned int
hard_sets(p) NODE *p;
{

	int op;

	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;

	switch (op) {
	case LAHF:  case CBTW:  case CWTL:
	case INB:  case XLAT:
	case INL:   case INW:
		return(EAX);
	case INSB: case INSL: case INSW:
		return (EDI);
	case AAA:  case AAD:  case AAM:  case AAS:
	case DAA:  case DAS: 
	case DIVB:  case IDIVB:  case IMULB:
	case MULB:
		return(EAX | CONCODES_AND_CARRY);
	case IMULW:  case IMULL:
		if(p->op2 == NULL)
			return(EAX | EDX | CONCODES_AND_CARRY);
		else return 0;
	case DIVL:  case IDIVL:
	case DIVW:  case IDIVW:
	case MULL:  case MULW:
		return(EAX | EDX | CONCODES_AND_CARRY);
	case SLODB:  case SLODW:  case SLODL:
	case LODSB:  case LODSL:  case LODSW:
		return(EAX | ESI);
	case LOOP:  case LOOPE:  case LOOPNE:  case LOOPNZ:
	case LOOPZ: case REP:  case REPNZ:  case REPZ: case REPE: case REPNE:
		return(ECX);
	case SCAL:  case SCAW:  case SCAB: 
	case SSCAB: case SSCAL: case SSCAW:
		return(CONCODES_AND_CARRY | EDI);
	case CWTD:  case CLTD:
		return(EAX | EDX);
	case SCMPL:  case SCMPW:  case SCMPB:
	case CMPSB:  case CMPSL:  case CMPSW:
		return(CONCODES_AND_CARRY | ESI | EDI);
	case SMOVL:  case SMOVW:  case SMOVB: case MOVSL:
		return(ESI | EDI);
	case SSTOL:  case SSTOW:  case SSTOB:
		return(EDI);
	case ENTER:  case LEAVE: 
		return( ESP | EBP);
	case CALL:   case LCALL:
		switch (p->rv_regs) {
			case 3:
				return (EAX | EDX | ECX);
			case 2:
				return (EAX | EDX);
			case 1:
			default:
				return (EAX);
			case 0:
				return 0;
		}  /* switch */
	case CMPL:   case CMPW:   case CMPB:
	case TESTL:  case TESTW:  case TESTB:
	case SAHF:   case BTL:    case BTW:
		return(CONCODES_AND_CARRY);
	case PUSHW: case PUSHL: case PUSHA: case PUSHFL: case PUSHFW:
		return( ESP );
	case POPW:  case POPL:
		return( ESP ); 
	case POPA: case POPAL: case POPAW:
		return( ESP | EDI | ESI | EBP | EBX | EDX | ECX | EAX );
	case POPFL: case POPFW:
		return (ESP| CONCODES_AND_CARRY);
	case FSTSW: case FNSTSW:
		return AX;
	default:
			return(0);
	}
}

boolean
change_by_const(p,reg,amount) NODE *p; unsigned int reg; int *amount;
{
int op;
	if (! (p->sets & reg)) /* sanity check */
		return false;

	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;

	switch (op) {
		case INCL:
			*amount = 1;
			return true;
		case DECL:
			*amount = -1;
			return true;
		case PUSHL:
			*amount = -4;
			return true;
		case CALL:
			if (! p->op3)  /* it's not /TMPSRET */
				return false;
			/* FALLTHRUOGH */
		case POPL:
			if (reg == ESP) {
				*amount = 4;
				return true;
			}
			return false;
		case LEAL:
			if (strchr(p->op1,',') == NULL
			 && p->idus == p->sets
			 && p->idus == reg
			 && isdigit(p->op1[0])) {
				*amount = (int)strtoul(p->op1,NULL,10);
				return true;
			}
			return false;
		case ADDL:
			if (isconst(p->op1) && isdigit(p->op1[1])) {
				*amount = (int)strtoul(p->op1+1,(char **)NULL,0);
				return true;
			}
			return false;
		case SUBL:
			if (isconst(p->op1) && isdigit(p->op1[1])) {
				*amount = -(int)strtoul(p->op1+1,(char **)NULL,0);
				return true;
			}
			return false;
		default:	return false;
	}/*end switch*/
	/* NOTREACHED */
}/*end change_by_const*/

/*The pairability of most of the instruction depend only on the opcode.
**These are taken from the database. There are some ecception:
**1. any instruction with both immediate and displacement is not pairable.
**2. rotateby 1 is pairable in the U pipe.
**   all other rotates are not pairable.
**3. shift by 1 and by immediate is pairable in the U pipe.
**   shift of memory is not pairable.
**4. test  a. reg,reg - pairable.
**         b. imm,eax - pairable.
**         c. imm/reg,reg/mem - not pairable.
**5. pushl / popl memory is not pairable.
*/
int
pairable(p) NODE *p;
{
		/* immediate and displacement */
		if (isconst(p->op1) && hasdisplacement(p->op2))
			return X86;
		else  switch (p->op) {
				case RCLB: case RCLW: case RCLL:
				case RCRB: case RCRW: case RCRL:
				case ROLB: case ROLW: case ROLL:
				case RORB: case RORW: case RORL:
					if (p->op2 == NULL) return WD1;
					return X86;
				case SALB: case SALW: case SALL:
				case SARB: case SARW: case SARL:
				case SHLB: case SHLW: case SHLL:
				case SHRB: case SHRW: case SHRL:
					if (p->op2 == NULL) return WD1;
					if (isconst(p->op1) && isreg(p->op2)) return WD1;
					return X86;
				case TESTB: case TESTW: case TESTL:
					if (isreg(p->op1) && isreg(p->op2)) return WDA;
					if (isconst(p->op1) && !strcmp(p->op2,"%eax")) return WDA;
					return X86;
				case PUSHL: case PUSHW: case POPW: case POPL:
					if (ismem(p->op1)) return X86;
					else return WDA;
				default:
					return opcodata[p->op - FIRST_opc].pairability;
		}
		/* NOTREACHED */
}/*end pairable*/



int
check_double()
{	NODE *p;
	int count = 0;
	int offset;
	if (target_cpu == P3)
		return false;
    for (p = n0.forw; p != &ntail; p = p->forw) 
		if (OpLength(p) == DoBL &&  (scanreg(p->op1,false) & (EBP | ESP))) {
			offset = (int)strtoul(p->op1,(char **)NULL,0);
			if (offset < 0)
				count += (int)strtoul(p->op1,(char **)NULL,0) % 8 ? -1 : 1;
		}
	return (count > 0);
}


#ifndef DEBUG
#define DEBUG
#endif
#ifdef DEBUG
#include <stdio.h>
void
dprinst(p) NODE *p;
{
	if (p->op == CALL || p->op == LCALL || isuncbr(p))
		if (p->op1)
			printf("\t%s\t%s\n",p->opcode,p->op1);
		else
			printf("\t%s\n",p->opcode);
	else if (islabel(p)) printf("%s:\n",p->opcode);
	else prinst(p);
}/*end dprinst*/

		void
fprinst(p) register NODE *p; { /* print instruction */

	FILE save;
	save = *stdout;
	*stdout = *stderr;
	dprinst(p);
	*stdout = save;
}	

void
dprtext(s,first,cnt,regal) char *s; NODE *first; int cnt; boolean regal;
{
    void plive();
    register NODE * p;
	FILE save;
	save = *stdout;
	*stdout = *stderr;

    if (!first)
	first = n0.forw;

    for (p = first; p != &ntail; p = p->forw) {
		fprintf(stderr,"%s",s);
		if (regal) fprintf(stderr,"%8.8x ",p->nrlive);
		else plive(p->nlive,0);
		dprinst(p);
		--cnt;
		if (cnt == 0)
		break;
    }
	*stdout = save;
	fprintf(stderr,"\n\n");
	fflush(stderr);
    return;
}

void
dpr(p) NODE *p;
{
    void plive();
	FILE save;
	save = *stdout;
	*stdout = *stderr;

	fprintf(stderr,"\n");

	plive(p->nlive,0);
	prinst(p);
	*stdout = save;
	fprintf(stderr,"\n");
	fflush(stderr);
    return;
}


	void
plive1(vector,flush) unsigned int vector; int flush;
{
	if (vector&Eax)
		fprintf(stderr,"AX ");
	if (vector&Edx)
		fprintf(stderr,"DX ");
	if (vector&Ecx)
		fprintf(stderr,"CX ");
	if (vector&Ebx)
		fprintf(stderr,"BX ");
	if (vector&Ebi)
		fprintf(stderr,"Ebi ");
	if (vector&Esi)
		fprintf(stderr,"Esi ");
	if (vector&Edi)
		fprintf(stderr,"Edi ");

	if (vector&Ax)
		fprintf(stderr,"Ax ");
	if (vector&Dx)
		fprintf(stderr,"Dx ");
	if (vector&Cx)
		fprintf(stderr,"Cx ");
	if (vector&Bx)
		fprintf(stderr,"Bx ");

	if (vector&AL)
		fprintf(stderr,"AL ");
	if (vector&DL)
		fprintf(stderr,"DL ");
	if (vector&CL)
		fprintf(stderr,"CL ");
	if (vector&BL)
		fprintf(stderr,"BL ");
	if (vector&AH)
		fprintf(stderr,"AH ");
	if (vector&DH)
		fprintf(stderr,"DH ");
	if (vector&CH)
		fprintf(stderr,"CH ");
	if (vector&BH)
		fprintf(stderr,"BH ");
	if (vector&ESI)
		fprintf(stderr,"SI ");
	if (vector&EDI)
		fprintf(stderr,"DI ");
	if (vector&SI)
		fprintf(stderr,"Si ");
	if (vector&DI)
		fprintf(stderr,"Di ");
	if (vector&BI)
		fprintf(stderr,"Bi ");
	if (vector&ESP)
		fprintf(stderr,"SP ");
	if (vector&EBP)
		fprintf(stderr,"BP ");
	if (vector&EBI)
		fprintf(stderr,"BI ");
	if (vector&CONCODES)
		fprintf(stderr,"CC ");
	if (vector& CARRY_FLAG)
		fprintf(stderr,"CF ");
	fprintf(stderr,":    ");
	if (flush)
		fputc('\n', stderr);
	fflush(stderr);
}

/*	static */ void
plive(vector,flush) unsigned int vector; int flush;
{
	if (vector&Eax)
		fprintf(stderr,"AX ");
	else
		fprintf(stderr,"   ");
	if (vector&Edx)
		fprintf(stderr,"DX ");
	else
		fprintf(stderr,"   ");
	if (vector&Ecx)
		fprintf(stderr,"CX ");
	else
		fprintf(stderr,"   ");
	if (vector&Ebx)
		fprintf(stderr,"BX ");
	else
		fprintf(stderr,"   ");

	if (vector&AL)
		fprintf(stderr,"AL ");
	else
		fprintf(stderr,"   ");
	if (vector&DL)
		fprintf(stderr,"DL ");
	else
		fprintf(stderr,"   ");
	if (vector&CL)
		fprintf(stderr,"CL ");
	else
		fprintf(stderr,"   ");
	if (vector&BL)
		fprintf(stderr,"BL ");
	else
		fprintf(stderr,"   ");
	if (vector&AH)
		fprintf(stderr,"AH ");
	else
		fprintf(stderr,"   ");
	if (vector&DH)
		fprintf(stderr,"DH ");
	else
		fprintf(stderr,"   ");
	if (vector&CH)
		fprintf(stderr,"CH ");
	else
		fprintf(stderr,"   ");
	if (vector&BH)
		fprintf(stderr,"BH ");
	else
		fprintf(stderr,"   ");
	if (vector&ESI)
		fprintf(stderr,"SI ");
	else
		fprintf(stderr,"   ");
	if (vector&EDI)
		fprintf(stderr,"DI ");
	else
		fprintf(stderr,"   ");
	if (vector&EBI)
		fprintf(stderr,"BI ");
	else
		fprintf(stderr,"   ");
	if (vector&EBP)
		fprintf(stderr,"EBP ");
	else
		fprintf(stderr,"   ");
	if (vector&ESP)
		fprintf(stderr,"ESP ");
	else
		fprintf(stderr,"   ");
	if (vector&CONCODES)
		fprintf(stderr,"CC ");
	else
		fprintf(stderr,"   ");
	if (vector& CARRY_FLAG)
		fprintf(stderr,"CF ");
	else
		fprintf(stderr,"   ");
	fprintf(stderr,":    ");
	if (flush)
		fputc('\n', stderr);
	fflush(stderr);
}

void
prdag(b) BLOCK * b;
{
void prnode();
NODE * p;
int i = 0;
	for (p = b->firstn; p != b->lastn->forw; p=p->forw) {
		i++;
		fprintf(stderr,"\nINSTRUCTION  %d\n",i);
		prnode(p);
	}/* for loop*/
}/*end prdag*/

void
prnode(p) NODE * p;
{
tdependent * tp;
	fprintf(stderr,"p = ");
	fprinst(p);
	fprintf(stderr,"parents: %d  ",p->nparents);
	fprintf(stderr,"width: %d  ",p->dependents);
	fprintf(stderr,"depth: %d\n",p->chain_length);
	plive(p->uses,0); fprintf(stderr,"uses\n");
	plive(p->sets,0); fprintf(stderr,"sets\n");
	plive(p->idus,0); fprintf(stderr,"idus\n");
	plive(p->idxs,0); fprintf(stderr,"idxs\n");
	if (target_cpu & (P5 | blend)) {
  		fprintf(stderr,"pairtype: ");
  		switch (p->exec_info.pairtype) {
    		case WD1:
				fprintf(stderr,"WD1\n");
				break;
    		case WD2:
				fprintf(stderr,"WD2\n");
				break;
    		case WDA:
				fprintf(stderr,"WDA\n");
				break;
    		case X86:
				fprintf(stderr,"X86\n");
				break;
    		default:
				fprintf(stderr,"unknown\n");
				break;
  		}/*end switch*/
	}/*endif*/
	if (p->dependent) {
		fprintf(stderr,"______________________dependents_______________\n");
		for (tp = p->dependent; tp ; tp = tp->next) {
			fprintf(stderr,"parents: %d  ",tp->depends->nparents);
			fprinst(tp->depends);
		}
	}
	if (p->anti_dep) {
  		fprintf(stderr,"_________________anti_dependents_______________\n");
  		for (tp = p->anti_dep; tp ; tp = tp->next) {
    		fprintf(stderr,"parents: %d  ",tp->depends->nparents);
    		fprinst(tp->depends);
    	}
  	}
	if (p->agi_dep) {
  		fprintf(stderr,"__________________agi_dependents_______________\n");
  		for (tp = p->agi_dep; tp ; tp = tp->next) {
    		fprintf(stderr,"parents: %d  ",tp->depends->nparents);
    		fprinst(tp->depends);
    	}
  	}
	if (p->may_dep) {
  		fprintf(stderr,"__________________may_dependents_______________\n");
  		for (tp = p->may_dep; tp ; tp = tp->next) {
    		fprintf(stderr,"parents: %d  ",tp->depends->nparents);
    		fprinst(tp->depends);
    	}
  	}
}/*end prnode*/

void
sprtext(s) char *s;
{
	NODE *p;
			for(ALLN(p)) {
				printf("%c%s ",CC,s);
				dprinst(p);
			}
}/*end sprtext*/

void
fsprtext(s) char *s;
{
	NODE *p;
			for(ALLN(p)) {
				fprintf(stderr,"/%s ",s);
				fprinst(p);
			}
}/*end fsprtext*/
#endif


void
swap_nodes(n1,n2) NODE *n1, *n2;
{
    NODE *t;
    if( n1->forw == n2 ){
        if( (n1->forw=n2->forw) != NULL )
            n1->forw->back=n1;
        n2->forw=n1;
        if( (n2->back=n1->back) != NULL )
            n2->back->forw=n2;
        n1->back=n2;
    }
    else if(n2->forw==n1){
        if( (n2->forw=n1->forw) !=NULL )
            n2->forw->back=n2;
        n1->forw=n2;
        if( (n1->back=n2->back) !=NULL )
            n1->back->forw=n1;
        n2->back=n1;
    }
    else {
        t=n1->forw;
        if( (n1->forw=n2->forw) != NULL )
            n1->forw->back=n1;
        if( (n2->forw=t) != NULL )
            n2->forw->back=n2;
        t=n1->back;
        if( (n1->back=n2->back) != NULL )
            n1->back->forw=n1;
        if( (n2->back=t) != NULL )
            n2->back->forw=n2;
    }
}

/* yank  n1 from the old place  and put in after n2 */
void
move_node (n1,n2) NODE *n1, *n2;
{
    if(n2->forw==n1) return ;
    /* yank */
    if( n1->forw != NULL ) n1->forw->back = n1->back;
    if( n1->back != NULL ) n1->back->forw = n1->forw;
    /* put in */
    if( (n1->forw=n2->forw) != NULL ) n2->forw->back=n1;
    n2->forw=n1;
    n1->back=n2;
}

NODE *
copy_p_after_q(p,q) NODE *p,*q;
{
NODE *newp = insert(q);
int i;
	newp->op = p->op;
	for (i = 0; i < 6; i++) newp->ops[i] = p->ops[i];
	newp->sets = p->sets;
	newp->uses = p->uses;
	return newp;
}

/* function returns target node of any jmp */
NODE *
find_label( p) NODE * p;
{
char* jtarget= p->ops[1];
    for (ALLN(p)) {
        if (is_any_label(p) && !strcmp(p->ops[0],jtarget))
        return p;
    }
    return NULL;
}

/* Function marks stores and saves at field pairtype, for fixed stack code. It
 * is called by local.c at athe very beggining of the optimizations, just before
 * sets_and_uses(). */
void
set_save_restore()
{
NODE *p,*retp;
unsigned int reg;
unsigned int marked_regs = 0; /*for dup entries, dont inc regs_spc twice*/

    /* Find saves and mark them */

	for (p = ntail.back; p != n0.forw; p = p->back) {
		if (p->op == RET) {
			retp = p;
			break;
		}
	}
	if (fixed_stack) {
		for (p = retp; !islabel(p) && p != n0.forw; p = p->back) {
			if (p->op == MOVL && (setreg(p->op2) & (EDI|ESI|EBX|EBP)) 
				&& (scanreg(p->op1,false) & ESP)) {
				p->save_restore = RESTORE;
			}
		}
		for (p = retp; p && p != &ntail; p = p->forw) {
			if (p->op == MOVL && 
				((reg = setreg(p->op1)) & (EDI|ESI|EBX|EBP))
				&& (scanreg(p->op2,false) & ESP)) {
				if (!(reg & marked_regs))  { 
					p->save_restore = SAVE;
					func_data.regs_spc += 4;
					marked_regs |= reg;
				}
			}
		}
	} else {
		for (p = retp; !islabel(p) && p != n0.forw; p = p->back) {
			if (p->op == POPL && setreg(p->op1) & (EDI|ESI|EBX|EBI))
				p->save_restore = RESTORE;
		}
		for (p = retp; p && p != &ntail; p = p->forw) {
			if (p->op == PUSHL && setreg(p->op1) & (EDI|ESI|EBX|EBI))
				p->save_restore = SAVE;
		}
	}
}/*end set_save_restore*/

#if EH_SUP
NODE *
find_try_block_end(NODE *start)
{
NODE *p;
	if (!start->in_try_block) {
		for (p = start; p && p != &ntail; p = p->forw) {
			if (islabel(p)) return p;
		}
	} else {
		for (p = start; p && p != &ntail; p = p->forw) {
			if (p->in_try_block != start->in_try_block)
				return p;
		}
	}
	return NULL;
}
#endif


void
init_optutil()
{
	reginfo = fixed_stack ? fix_reginfo : reg_reginfo ;
}
