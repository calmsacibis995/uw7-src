#ident	"@(#)optim:i386/ebboptim.c	1.8.7.36"
#include "sched.h" /*include optim.h and defs.h */
#include "optutil.h"
#include "regalutil.h"

/* All following optimizations have to do with values in registers.
** Therefore they operate on generalized basic block. These are
** consecutive basic blocks in which the first one begins with a
** label and the next ones do not. Hence the only entry to them
** is by fall through.
*/

static void remove_register(), rmrdmv();
static int try_forward();
static void try_backward(); 
static boolean zvtr();
int  isbase();
void  remove_base();
void remove_index();
int fflag = 0;
extern boolean swflag;
extern boolean asmflag;

void
ebboptim(opt) int opt;
/* The driver of the optimizations. Determines the first and last
** nodes of the extended basic block and calls the specific function
** according to it's parameter.
*/
{
BLOCK *b , *firstb;
int found = 0;

#ifdef DEBUG
	if (opt == COPY_PROP) {
		COND_RETURN("rmrdmv");
	} else if (opt == ZERO_PROP) {
		COND_RETURN("zvtr");
	}
#endif

	ldanal();

	for (b = firstb = b0.next ; b ; b = b->next) {
		firstb = b;
		while (!( islabel(b->lastn->forw) || isuncbr(b->lastn) 
				  || b->lastn->forw == &ntail)) 
			b = b->next;
		switch (opt) {
			case ZERO_PROP:
				found |= zvtr(firstb->firstn,b->lastn); /*zero value trace*/
				break;
			case COPY_PROP:
				rmrdmv(firstb,b->lastn); /*remove redundant mov reg,reg*/
				break;
			default:
				fatal(__FILE__,__LINE__,"unknown optimization for ebboptim\n");
		}
	}/*for loop*/
	if (found)
		ldanal();

}/*end ebboptim*/

/* A basic block level optimization. Motivation came from the following
** basic block, lhs before, rhs after:

** .L295:                                    .L295:
**	movl	%esi,%ebp              <  
**	movl	%ebx,%ecx              <  
**	addl	$4,%esi                <  
**	addl	$4,%ebx                <  
**	flds	(%edi)                    	flds	(%edi)
**	fmuls	(%ecx)                 |  	fmuls	(%ebx)
**	fadds	(%ebp)                 |  	addl	$4,%ebx
**	fstps	(%ebp)                 |  	fadds	(%esi)
**	                               >  	fstps	(%esi)
**	                               >  	addl	$4,%esi
**	cmpl	%eax,%ebx                 	cmpl	%eax,%ebx
**	jna	.L295                     	jna	.L295
**
** Here the optimization was done twice, and the two first instructions
** were deleted.
*/

void
bbopt()
{
BLOCK *b;
NODE *p,*q,*qforw;
NODE *first;
NODE *second;
NODE *last;
NODE *limit;
boolean uniq_change;
unsigned int srcreg,dstreg;
boolean first_seen;
char *tmp;
char *t,*t1,*s;
int m;
boolean success = false;
boolean memused;
boolean doit;
unsigned int idxreg;
#ifdef FLIRT
static int numexec=0;
#endif

	COND_RETURN("bbopt");

#ifdef FLIRT
	++numexec;
#endif

	bldgr(false);
	for (b = b0.next; b; b = b->next) {
		COND_SKIP(continue,"%d %s ",second_idx,b->firstn->opcode,0);
		for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (p->op == MOVL && isreg(p->op1) && isreg(p->op2)) {
				first = last = second = NULL;
				uniq_change = true;
				srcreg = setreg(p->op1);
				dstreg = setreg(p->op2);
				/* find q that uses dstreg as an index */
				for (q = p; q != b->lastn->forw; q = q->forw) {
					unsigned int qidx;
					if ((qidx = indexing(q)) == dstreg) {
						if (!first)
							first = q; /* remember the first one */
						if (!(q->sets & dstreg) && !(q->nlive & dstreg))
							last = q; /* remember the last one */
						if (q->uses & srcreg) {
							last = NULL;
							break; /*disqualify */
						}
					}
					if ((qidx & dstreg) && (qidx != dstreg)) {
						last = NULL;
						break; /*disqualify */
					}
				}
				if (!last) {
					continue; /* goto next movl*/
				}
				second = NULL;
				/* find a setting of srcreg between p and last, only one */
				/* If that inst uses or sets memory, we conservatively do
				** not move it over any other memory usage, to save anal time.
				*/
				first_seen = false;
				memused = false;
				for (q = p->forw; q != last; q = q->forw) {
					if (q == first) first_seen = true;
					/* disable if there is a direct use of dstreg, not as
					** index. It can be added, but take care for only
					** explicit usage.
					*/
					if ((q->uses & srcreg) && !(q->sets & srcreg)) {
						second = NULL;
						break;
					}
					if (uses_but_not_indexing(q) & dstreg) {
						second = NULL;
						break;
					}
					if (q->sets & dstreg) {
						second = NULL; /* disable */
						break;
					}
					if ((muses(q) & MEM) || (msets(q) & MEM)) memused = true;
					if ((uses_but_not_indexing(q) | q->sets) & srcreg) {
						if (!first_seen && !second) {
						unsigned int otherreg;
							second = q;
							 otherreg = second->uses & ~(srcreg|dstreg);
							 if ((otherreg & last->nlive)
								||changed(second,last,otherreg)) {
							 	/* we cant move second because of changing
							 	* other reg used in second
							 	*/
							 	uniq_change = false;
							 	break;
							 }
							/* might have to disable this second: */
							if (   second->uses & dstreg
								|| ((second->sets & ~CONCODES_AND_CARRY) != srcreg)
								|| (   memused
									&& (   (muses(second) & MEM)
										|| (msets(second) & MEM)))) {
								uniq_change = false;
								break;
							}
						} else {
							uniq_change = false;
							break;
						}
					}
				}
				if (!uniq_change || !second) {
					continue; /* do next movl */
				}
				if (   (second->sets & CONCODES_AND_CARRY)
					&& (last->nlive & CONCODES_AND_CARRY)) {
					continue; /* do next movl */
				}
				if (second->sets & srcreg && last->sets & srcreg)
					continue;
				limit = last->forw;
				for (q = p; q != limit; q = qforw) {
					qforw = q->forw;
					if (indexing(q) == dstreg) {
						if (scanreg(q->op1,false) == dstreg) m = 1;
						else if (scanreg(q->op2,false) == dstreg) m = 2;
						else m = 3;
						tmp = getspace(strlen(q->ops[m]));
						t = strstr(q->ops[m],p->op2);
						s = tmp;
						for (t1 = q->ops[m]; t1 != t; t1++)
							*s++ = *t1;
						*s = '\0';
						strcat(tmp,p->op1);
						t+=4;
						strcat(tmp,t);
						q->ops[m] = tmp;
						q->uses = uses(q);
						DELNODE(second);
						APPNODE(second,q);
					}
				}
				DELNODE(p);
				success = true;
				FLiRT(O_BBOPT1,numexec);
				if (b->next) b->lastn = b->next->firstn->back;
			}/*endif mov %reg,%reg*/
		}/*for all p in b */
		for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (LEAL == p->op) {
				dstreg = setreg(p->op2);
				idxreg = indexing(p);
				doit = true;
				last = NULL;
				if (idxreg & dstreg) continue;
				for (q = p->forw; q != b->lastn->forw; q = q->forw) {
					if (indexing(q) & dstreg) last = q;
				}
				if (last == NULL) continue;
				if (last->nlive & dstreg) continue;
				for (q = p->forw; q != last->forw; q = q->forw) {
					if (q->sets & (dstreg | idxreg)) {
						doit = false;
						break;
					}
					if (q->uses & dstreg) {
						if (scanreg(q->op1,0) & dstreg) m = 1;
						else if (scanreg(q->op2,0) & dstreg) m = 2;
						else if (scanreg(q->op3,0) & dstreg) m = 3;
						else {
							doit = false;
							break;
						}
						if (!iszoffset(q->ops[m],p->op2)) {
							doit = false;
							break;
						}
					}
				}
				if (doit) {
					for (q = p; q != last->forw; q = q->forw) {
						for (m = 1; m <= 3; m++) {
							if (iszoffset(q->ops[m],p->op2)) {
								q->ops[m] = p->op1;
								new_sets_uses(q);
							}
						}
					}
					DELNODE(p);
					success = true;
					FLiRT(O_BBOPT2,numexec);
				}
			}/*endif leal*/
		}
	}/*end block*/
	if (success) ldanal();
}/*end bbopt*/

static unsigned int reg_regmasx[] = {EAX,EDX,EBX,ECX,ESI,EDI,EBI};
static unsigned int fix_regmasx[] = {EAX,EDX,EBX,ECX,ESI,EDI,EBP};
static unsigned int *regmasx;

/* This module keeps track of zero values in registers. If there
** is a zero value in a register and it is used as an index, then
** change the operand so that the register will not be used.
** In addition, some optimizations are possible. If the constant
** 0 is used, replace it by a register holding zero.
** If zero value is moved from one register to another, which already
** contains zero, delete the node.
*/
extern BLOCK b0;
extern int suppress_enter_leave;
static char *reg_registers[] =
    { "%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi" , "%ebi" };
static char *fix_registers[] =
    { "%eax", "%edx", "%ebx", "%ecx", "%esi", "%edi" , "%ebp" };
static char **registers;
#define Mark_z_val(reg) zero_flags |= reg
#define Mark_non_z(reg) zero_flags &= ~(reg)
#define Have_z_val(reg) (zero_flags && (zero_flags | (reg)) == zero_flags)
int nregs;

/* full_reg() will return the 32 bit register that contains the 
input parameter in other cases it returns input */
unsigned int
full_reg(i) unsigned int i;
{
  switch (i) {
    case EAX: case AX: case Eax: case Ax: case AH: case AL:
        return EAX;
    case EBX: case BX: case Ebx: case Bx: case BH: case BL:
        return EBX;
    case ECX: case CX: case Ecx: case Cx: case CH: case CL:
        return ECX;
    case EDX: case DX: case Edx: case Dx: case DH: case DL:
        return EDX;
    case EDI: case DI: case Edi:
        return EDI;
    case ESI: case SI: case Esi:
        return ESI;
    case EBP: case BP: case Ebp:
        return EBP;
    case EBI: case BI: case Ebi:
        return EBI;

  }/*end switch*/
    /* NOTREACHED */
return 0;
} /*end full_reg*/


/* itoreg() converts register bits to register string */
char *
itoreg(i) unsigned int i;
{
  switch (i) {
    case EAX: case Eax: return "%eax";
    case EDX: case Edx: return "%edx";
    case EBX: case Ebx: return "%ebx";
    case ECX: case Ecx: return "%ecx";
    case ESI: case Esi: return "%esi";
    case EDI: case Edi: return "%edi";
    case EBP: case Ebp: return "%ebp";
    case EBI: case Ebi: return "%ebi";
    case ESP:           return "%esp";
    case AX:  case Ax:  return "%ax";
    case DX:  case Dx:  return "%dx";
    case BX:  case Bx:  return "%bx";
    case CX:  case Cx:  return "%cx";
    case SI:            return "%si";
    case DI:            return "%di";
    case AH:  return "%ah";
    case DH:  return "%dh";
    case BH:  return "%bh";
    case CH:  return "%ch";
    case AL:  return "%al";
    case DL:  return "%dl";
    case BL:  return "%bl";
    case CL:  return "%cl";
    case BI:  return "%bi";
    case BP:  return "%bp";
  /* NOTREACHED */
  }/*end switch*/
    return "";
}/*end itoreg*/

char *
itohalfreg(i) unsigned int i;
{
  switch (i) {
	case EAX:  return "%ax";
	case EDX:  return "%dx";
	case EBX:  return "%bx";
	case ECX:  return "%cx";
	case ESI:  return "%si";
	case EDI:  return "%di";
	case EBP:  return "%bp";
	case AX:  return "%ax";
	case DX:  return "%dx";
	case BX:  return "%bx";
	case CX:  return "%cx";
	case SI:  return "%si";
	case DI:  return "%di";
	case BP:  return "%bp";
	case AH:  return "%ax";
	case DH:  return "%dx";
	case BH:  return "%bx";
	case CH:  return "%cx";
	case AL:  return "%ax";
	case DL:  return "%dx";
	case BL:  return "%bx";
	case CL:  return "%cx";
	case EBI: return "%bi";
	case BI:  return "%bi";
  /* NOTREACHED */
  }/*end switch*/
}/*end itoreg*/


char *
itoqreg(i) unsigned int i;
{
  switch (i) {
	case EAX:  return "%al";
	case EDX:  return "%dl";
	case EBX:  return "%bl";
	case ECX:  return "%cl";
	case AX:  return "%al";
	case DX:  return "%dl";
	case BX:  return "%bl";
	case CX:  return "%cl";
	case AH:  return "%ah";
	case DH:  return "%dh";
	case BH:  return "%bh";
	case CH:  return "%ch";
	case AL:  return "%al";
	case DL:  return "%dl";
	case BL:  return "%bl";
	case CL:  return "%cl";
	default:  return NULL;
  /* NOTREACHED */
  }/*end switch*/
}/*end itoqreg */

/* sizeofreg(reg) returns number of bits in the register reg */
int
sizeofreg(reg) unsigned int reg;
{
  switch (reg) 
  {
	case EAX: case EDX: case EBX: case ECX: case ESI: case EDI: case EBP: 
	case ESP: case EBI: case Eax: case Edx: case Ebx: case Ecx: case Esi:
	case Edi: case Ebi: case Ebp:
		return LoNG;
	case AX: case DX: case BX: case CX: case SI: case DI: case BI: case BP:
		return WoRD;
	case AH: case DH: case BH: case CH: case AL: case DL: case BL: case CL:
		return ByTE;
  }/*end switch*/

  fatal(__FILE__,__LINE__,"sizeofreg(), %x is not a register",reg);
  /* NOTREACHED */
}/*end sizeofreg() */

static unsigned int zero_flags = 0;

/*Does instruction set a register to zero.
**true only for long instructions, that set the whole register.
*/
static boolean
set_reg_to_0(p) NODE *p;
{
	if (!isreg(p->op2))
		return false;
	switch (p->op) {
		case ROLL: case RORL: case SALL:
		case ROLW: case RORW: case SALW:
		case ROLB: case RORB: case SALB:
		case SARL: case SHLL: case SHRL:
		case SARW: case SHLW: case SHRW:
		case SARB: case SHLB: case SHRB:
			if (Have_z_val(p->sets & ~CONCODES_AND_CARRY))
				return true;
			else
				return false;
		case XORL: case SUBL:
			if (samereg(p->op1,p->op2))
				return true;
			/* FALLTHRUOGH */
		case XORW: case SUBW:
		case XORB: case SUBB:
			if (   Have_z_val(p->sets & ~CONCODES_AND_CARRY)
				&& isreg(p->op1)
				&& Have_z_val(p->uses))
				return true;
			if (   !strcmp(p->op1,"$0")
				&& Have_z_val(p->sets & ~CONCODES_AND_CARRY))
				return true;
			return false;
		case IMULB: case IMULW: case IMULL:
			if (p->op3) {
				if (!strcmp(p->op1,"$0"))
					return true;
				if (Have_z_val(p->uses))
					return true;
				return false;
			}
			/*FALLTHROUGH*/
		case ANDL: case MULL:
			if (isreg(p->op1) && Have_z_val(p->uses))
				return true;
			if (!strcmp(p->op1,"$0"))
				return true;
			/* FALLTHROUGH */
		case ANDW: case MULW:
		case ANDB: case MULB:
			if (Have_z_val(p->sets & ~CONCODES_AND_CARRY))
				return true;
			return false;
		case ADDL: case ORL:
			if (((isreg(p->op1) && Have_z_val(p->uses))
			   || !strcmp(p->op1,"$0"))
			 && Have_z_val(p->sets))
				return true;
			else
				return false;
		case MOVB: case MOVW:
			if (! Have_z_val(p->sets))
				return false;
			/* FALLTHROUGH */
		case MOVL:
		case MOVZWL: case MOVZBL:
		case MOVSBL: case MOVSWL:
		case MOVZBW: case MOVSBW:
			if (!strcmp(p->op1,"$0"))
				return true;
			if (isreg(p->op1) && Have_z_val(p->uses))
				return true;
			return false;
		case LEAL:
			if (Have_z_val(p->uses) && !hasdisplacement(p->op1))
				return true;
			else 
				return false;
		default:
			return false;
	}/*end switch*/
/*NOTREACHED*/
}/*end set_reg_to_0*/


/*main function of the zero tracing module*/

static boolean
zvtr(firsti,lasti) NODE *firsti , *lasti;
{
unsigned int pidx;
NODE *p, *nextp = NULL; /*init to prevent lint */
int i,retval = false;
int m;
boolean enter;

#ifdef FLIRT
static int numzvtr=0;
#endif

#ifdef FLIRT
  ++numzvtr;
#endif

  nregs =  !fixed_stack && suppress_enter_leave ? (NREGS-2) : (NREGS -1);
  zero_flags = 0; /*no reg is known to hold zero */
  for (p = firsti ; p != lasti->forw; p = nextp) {
	if (p->op == ASMS || is_safe_asm(p)) /*asm ends the ebb, skip the rest*/
		return retval;
	nextp = p->forw;
	COND_SKIP(continue,"%d ",second_idx,0,0);
	/*If immediate 0 is used in an instruction, we might be
	**able to replace it by a register known to hold zero.
	**do not do it for cmp, because if it is redundant, w2opt()
	**will delete it.
	**Next try the same for movb and movw, but then replace an $0 by, say, %al
	**only if %eax holds zero, not if %al holds zero. Dont go into tracking
	**values of parts of registers.
	*/
	if ((p->op == MOVL || p->op == PUSHL)
	  && strcmp(p->op1,"$0") == 0)
	  for(i = 0; i < nregs; i++)
		if (Have_z_val(regmasx[i])) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"make $0 a register: ");
				fprinst(p);
			}
#endif
		  p->op1 = registers[i];
		  p->uses |= regmasx[i];
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"became: ");
				fprinst(p);
			}
#endif
		  p->zero_op1 = true;
		  FLiRT(O_ZVTR,numzvtr);
		  retval = true;
		  break;
		}
	if ((p->op == MOVW) && strcmp(p->op1,"$0") == 0)
	  for(i = 0; i < nregs; i++)
		if (Have_z_val(regmasx[i])) {
			p->op1 = itohalfreg(L2W(regmasx[i]));
			p->uses |= L2W(regmasx[i]);
			p->zero_op1 = true;
			FLiRT(O_ZVTR,numzvtr);
		  	retval = true;
		  	break;
		}
	if ((p->op == MOVB) && strcmp(p->op1,"$0") == 0)
	  for(i = 0; i < 4; i++)  /* 4 byte addressable registers. */
		if (Have_z_val(regmasx[i])) {
			p->op1 = itoqreg(L2B(regmasx[i]));
			p->uses |= L2B(regmasx[i]);
			p->zero_op1 = true;
			FLiRT(O_ZVTR,numzvtr);
		  	retval = true;
		  	break;
		}
	/*Remove useless arithmetic instructions, when the operand
	**is known to hold zero. Just change register to immediate
	**zero, and w1opt() will take care of the rest.
	*/
	if (isreg(p->op2))
		switch (p->op) {
			case LEAL: case LEAW:
				if (*p->op1 != '(' || !Have_z_val(p->uses))
					break;
				if (!Have_z_val(p->sets & ~ CONCODES_AND_CARRY)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"make lea a xor: ");
						fprinst(p);
					}
#endif
					chgop(p,XORL,"xorl");
					p->op1 = p->op2;
					p->uses = 0;
					FLiRT(O_ZVTR,numzvtr);
					retval = true;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"became: ");
						fprinst(p);
					}
#endif
				}
				/*FALLTHROUGH*/
			case ROLL: case RORL: case SALL:
			case ROLW: case RORW: case SALW:
			case ROLB: case RORB: case SALB:
			case SARL: case SHLL: case SHRL:
			case SARW: case SHLW: case SHRW:
			case SARB: case SHLB: case SHRB:
				if (Have_z_val(p->sets & ~CONCODES_AND_CARRY)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"delete: ");
						fprinst(p);
					}
#endif
					ldelin2(p);
					DELNODE(p);
					FLiRT(O_ZVTR,numzvtr);
					retval = true;
				}
				break;
/*
			case IMULL: case IMULW: case IMULB:
			case MULL: case MULW: case MULB:
			Amigo finds all the cases of mull by 0  
*/
			case ANDB: case ANDW: case ANDL:
			if (p->nlive & CONCODES_AND_CARRY)
				break;
			if ((isreg(p->op1) && Have_z_val(scanreg(p->op1,false)))
			 || !strcmp(p->op1,"$0"))
				if (Have_z_val(p->sets & ~CONCODES_AND_CARRY)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"delnode");
						fprinst(p);
					}
#endif
					DELNODE(p);
					FLiRT(O_ZVTR,numzvtr);
					retval = true;
				}
				break;
			case SUBB: case SUBW: case SUBL:
			case ADDB: case ADDW: case ADDL:
			case ORB: case ORW: case ORL:
				if (p->nlive & CONCODES_AND_CARRY)
					break;
				if (isreg(p->op1) && Have_z_val(scanreg(p->op1,false))) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"delete arithmetic ");
						fprinst(p);
					}
#endif
					DELNODE(p);
					FLiRT(O_ZVTR,numzvtr);
					retval = true;
				} else if (   p->op != SUBB
						   && p->op != SUBW
						   && p->op != SUBL
						   && Have_z_val(p->sets & ~CONCODES_AND_CARRY)) {
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"change arithmetic to mov");
						fprinst(p);
					}
#endif
					switch (OpLength(p)) {
						case ByTE: chgop(p,MOVB,"movb"); break;
						case WoRD: chgop(p,MOVW,"movw"); break;
						case LoNG: chgop(p,MOVL,"movl"); break;
					}
					p->sets &= ~CONCODES_AND_CARRY;
					p->uses &= ~p->sets;
					FLiRT(O_ZVTR,numzvtr);
					retval = true;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"became");
						fprinst(p);
					}
#endif
				}
				break;
		}/*end switch*/
	/*If there is an operand with a register as base or index, and
	**the register is known to hold zero value, then remove
	**the register from the operand.
	*/
	if (pidx = scanreg(p->op1,true)) m = 1;
	else if (pidx = scanreg(p->op2,true)) m = 2;
	else if (pidx = scanreg(p->op3,true)) m = 3;

	if (pidx)
		for(i = 0; i < nregs; i++)
			if ((pidx  & regmasx[i]) && Have_z_val(regmasx[i])) {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"change from ");
					fprinst(p);
				}
#endif
				remove_register(p,regmasx[i],pidx,m);
				p->uses = uses(p);
				pidx &=  ~regmasx[i];
				FLiRT(O_ZVTR,numzvtr);
				retval = true;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"to ");
					fprinst(p);
				}
#endif
			}


	/* if this inst is a cmp and the next one is a jump then try to */
	/* eliminate them.                                              */
	enter = true;
	if (   (p->op == CMPL || p->op == CMPW || p->op == CMPB)
		&& p->forw->op >= JA
		&& p->forw->op <= JZ /*followed by a conditional jump*/
		&& !(p->forw->nlive & CONCODES_AND_CARRY)	/* condition codes are not live */
	) { int n1,n2;
	   if (isnumlit(p->op1))
		n2 = (int)strtoul(p->op1+1,(char **)NULL,0);
	   else if (isreg(p->op1) && Have_z_val(scanreg(p->op1,false)))
		n2 = 0;
	   else enter = false;
	   if (isnumlit(p->op2+1))
		n1 = (int)strtoul(p->op2,(char **)NULL,0);
	   else if (isreg(p->op2) && Have_z_val(scanreg(p->op2,false)))
		n1 = 0;
	   else enter = false;
	   if (enter) {
		boolean willjump = true;

		switch (p->forw->op) {
			case JA: case JNBE:
				willjump = (unsigned)n1 > (unsigned)n2;
				break;
			case JAE: case JNB:
				willjump = (unsigned)n1 >= (unsigned)n2;
				break;
			case JB: case JNAE:
				willjump = (unsigned)n1 < (unsigned)n2;
				break;
			case JBE: case JNA:
				willjump = (unsigned)n1 <= (unsigned)n2;
				break;
			case JG: case JNLE:
				willjump = n1 > n2;
				break;
			case JGE: case JNL:
				willjump = n1 >= n2;
				break;
			case JL: case JNGE:
				willjump = n1 < n2;
				break;
			case JLE: case JNG:
				willjump = n1 <= n2;
				break;
			case JE: case JZ:
				willjump = n1 == n2;
				break;
			case JNE: case JNZ:
				willjump = n1 != n2;
				break;
			default:
#ifdef DEBUG
				fatal(__FILE__,__LINE__,"MARC: jump is %d\n", p->forw->op);
#endif
			goto unk_jcc1; /*dont know how to write other jcc's in C */
		}/*end switch*/
		DELNODE(p);
		if (willjump)
			chgop(p->forw,JMP,"jmp");
		else
			DELNODE(p->forw);
			
		FLiRT(O_ZVTR,numzvtr);
		retval = true;
	   }/*endif enter*/
	}/*endif cmp-jcc*/

unk_jcc1:

	/*same as above for test - jcc */

	enter = true;
	if (   (p->op == TESTL || p->op == TESTW || p->op == TESTB)
		&& p->forw->op >= JA
		&& p->forw->op <= JZ /*followed by a conditional jump*/
		&&  !(p->forw->nlive & CONCODES_AND_CARRY)	/* condition codes are not live */
	) { int n1,n2;
	   if (isnumlit(p->op1))
		n2 = (int)strtoul(p->op1+1,(char **)NULL,0);
	   else if (isreg(p->op1) && Have_z_val(scanreg(p->op1,false)))
		n2 = 0;
	   else 
		enter = false;
	   if (isnumlit(p->op2))
		n1 = (int)strtoul(p->op2+1,(char **)NULL,0);
	   else if (isreg(p->op2) && Have_z_val(scanreg(p->op2,false)))
		n1 = 0;
	   else
		enter = false;
	   if (enter) {
		boolean willjump = true;
		int res = n1 & n2;

		switch (p->forw->op) {
			case JA: case JNBE:
				willjump = res != 0;
				break;
			case JAE: case JNB:
				willjump = true;
				break;
			case JB: case JNAE:
				willjump = res != 0;
				break;
			case JBE: case JNA:
				willjump = true;
				break;
			case JG: case JNLE:
				willjump = res > 0;
				break;
			case JGE: case JNL:
				willjump = res >= 0;
				break;
			case JL: case JNGE:
				willjump = res < 0;
				break;
			case JLE: case JNG:
				willjump = res <= 0;
				break;
			case JE: case JZ:
				willjump = res == 0;
				break;
			case JNE: case JNZ:
				willjump = res != 0;
				break;
			case JNS:
				willjump = (res & 0x8000000) == 0;
				break;
			case JS:
				willjump = (res & 0x8000000) != 0;
				break;
			default:
#ifdef DEBUG
			fatal(__FILE__,__LINE__,"MARC: jump is %d\n", p->forw->op);
#endif
				goto unk_jcc2; /*dont know how to simulate other jcc's in C*/
		}/*end switch*/
		DELNODE(p);
		if (willjump)
			chgop(p->forw,JMP,"jmp");
		else
			DELNODE(p->forw);
			
		FLiRT(O_ZVTR,numzvtr);
		retval = true;
	   }/*endif enter*/
	}/*endif test-jcc*/

unk_jcc2:

/* END OF TREATMENT OF CMP, TEST */

	/*Find out that an instruction sets a register to zero
	**and mark it.
	**If possible, change the zero setting to be of the form
	** movl %eax,%ebx , where %eax is known to hold zero.
	**rmrdmv() will prevent this assignment if possible.
	*/
	if (set_reg_to_0(p)) {
		if (   !(p->nlive & CONCODES_AND_CARRY)
			&& (OpLength(p) == LoNG)) {
			for(i = 0; i < nregs; i++)
				if (   Have_z_val(regmasx[i])
					&& regmasx[i] != (p->sets & ~CONCODES_AND_CARRY))
					break;
			if (i < nregs)  {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"reduce to mov: ");
					fprinst(p);
				}
#endif
				chgop(p,MOVL,"movl");
				p->op1 = registers[i];
				p->uses = regmasx[i];
				p->sets &= ~CONCODES_AND_CARRY;
				if (p->op3) {
					p->op2 = p->op3;
					p->op3 = NULL;
				}
				FLiRT(O_ZVTR,numzvtr);
				retval = true;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"became: ");
					fprinst(p);
				}
#endif
			}/*endif i < nregs*/
		} /* endif */
		Mark_z_val(p->sets & ~CONCODES_AND_CARRY);
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"Mark z reg op2 ");
			fprinst(p);
		}
#endif
	} else {
	/*Find out that an instruction sets a register to non zero
	**and mark it.
	*/
		for(i = 0; i < nregs; i++)
			if (p->sets & regmasx[i]) {
				Mark_non_z(regmasx[i]);
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"Mark non z reg op2 ");
				fprinst(p);
			}
#endif
		}
	}
  }/*for all nodes*/
  return retval;
}/*end zvtr*/

/* Do string operations to remove a register from the operand. Activated
** if the register is used as either a base or index and is known to hold zero.
*/
static void
remove_register(NODE *p, unsigned int reg, unsigned int pidx, int m)
{
 int length;
 char *tmpop;
 char *t;
	if (reg == pidx) {
		/* If only one register in the indexing,*/
		/* then leave only the displacement.    */
		t = p->ops[m];
		length = 0;
		while(*t != '(') {
			t++;
			length++;
		}
		tmpop = getspace((unsigned)length+1);
		(void) strncpy(tmpop,p->ops[m],length);
		tmpop[length] = (char) 0;
		p->ops[m] = tmpop;
	} else {
		/* There are both base and index*/
		/* This code assumes that the zero holding register appears
		**only once in the operand.                             */
		if (isbase(reg,p->ops[m])) {
			remove_base(&(p->ops[m]));
		} else {
			remove_index(&(p->ops[m]));
		}
	}
	if (*p->ops[m] == '\0' ) {
		/* reference a null pointer!*/
		p->ops[m] = "0";
	}
} /* end remove_register */

int
isbase(reg,op) unsigned int reg; char *op; {
	return (reg & setreg(strchr(op,'(') + 1)); /*base register*/
}/*end isbase*/

void
remove_index(op) char **op;
{
char *tmpop;
char *t;
int length;
  t = strchr(*op,',');
  length = t - *op +2;
  tmpop = getspace((unsigned)length);
  (void) strncpy(tmpop,*op,length-2);
  if (tmpop[length -3] == '(')
	tmpop[length-3] = '\0';
  else {
	tmpop[length-2] = ')';
	tmpop[length-1] = '\0';
  }
  *op = tmpop;
}/*end remove_index*/

void
remove_base(op) char **op;
{
int scale;
char *tmpop,*t,*s;
	t = strchr(*op,')');
	t--;
	if (isdigit(*t))
		scale = 1;
	else
		scale = 0;
	tmpop = getspace(strlen(*op) - 4);
	t = tmpop;
	s = *op;
	while (*s != '%')
		*t++ = *s++;
	s+= 5-scale;
	while (*t++ = *s++);
	if (*(t - 2 ) == '(' )
		*(t -2 ) = '\0';	
	*op = tmpop;
}/*end remove_base*/

/* fix_live() perform live/dead analysis over a block.  */
static void
fix_live(first,last,reg_set,reg_reset)  NODE *first, *last; 
unsigned int reg_set,reg_reset; {
unsigned int live;
	if (last->back->forw != last)
		live =  last->nlive | last->uses; /* from try_backward() */
	else
		live =  (last->nlive | last->uses) & ( ~last->sets | last->uses);
	for (last = last->back; first->back != last; last = last->back) {
		last->nlive &=  ~(reg_reset | reg_set); /* clear old live */
		last->nlive |= ( reg_set & live); /* set new live */
		live = (last->nlive | last->uses) & ( ~last->sets | last->uses);
	}
}/*end fix_live*/
/* is_new_value() will return 1 if the dest register is set and not used in 
the instruction p. It will return 2 if we can convert the instruction to less
good instruction that sets the dest register without using it. 
for every other instruction it will return 0 
*/
static int
is_new_value(p,reg) NODE *p; unsigned int reg;
{	char *t;
	switch(p->op) {
		case MOVB:	case MOVW:	case MOVL:
		case MOVZBW: case MOVZBL: case MOVZWL:
		case MOVSBW: case MOVSBL: case MOVSWL:
		case CALL: case LCALL:
				return 1;
		case LEAW: case LEAL:
			if (! isdeadcc(p)) /* can't replace the LEAL */ 
				return 1;
			t = strchr(p->op1,')') -1;
			if (*t == 2 || *t == 4 || *t == 8) /* has scale */
				return 1;
			if  (p->uses == reg)  /* add offset to reg only */
				return 1;
			if (*p->op1 == '(') /* no offset */
				return 1;
			return 2;
		case ADDL:
			if (! isdeadcc(p))
				return 0;
			if ( isreg(p->op1)  /* can be leal [%reg1,%reg2],%reg2 */
			 || *p->op1 == '$')  /* 'addl $val,%reg' => 'leal val[%reg],%reg' */
				return 2;
			return 0;
		case SUBL: 
			if (! isdeadcc(p))
				return 0;
			if (*p->op1 == '$' && isdigit(p->op1[1]))
				return 2;  /* 'subl $val,%reg' => 'leal -val[%reg],%reg' */
			return 0;
		case DECL: case INCL:
			return isdeadcc(p) ?  2 : 0;
		case SHLL:
			if (*p->op1 == '$' && p->op1[2] == 0 && 
				(p->op1[1] == '1' || p->op1[1] == '2' || p->op1[1] == '3'))
			return isdeadcc(p) ?  2 : 0;
				  
		default:
			return 0;
	}/*end switch*/
	/*NOTREACHED*/
}/*end is_new_value*/

/* new_leal() will convert the instruction to an leal instruction. It will 
be used to convert instruction that uses and sets the same register to an 
instruction that uses some registers and sets other register. 
*/ 
static void
new_leal(p) NODE *p; {
	char *tmp;
	switch(p->op) {
		case ADDL:
			if ( isreg(p->op1))  { /* can be leal (%reg1,%reg2),%reg2 */
				tmp = getspace(12);
				sprintf(tmp,"(%s,%s)",p->op1,p->op2);
				p->op1 = tmp;
			} else  { 
				tmp = getspace(strlen(p->op1) + 8);
				sprintf(tmp,"%s(%s)",&p->op1[1],p->op2);
				p->op1 = tmp;
			}
			break;
		case SUBL: 
			tmp = getspace(strlen(p->op1) + 8);
			if (p->op1[1] == '-')
				sprintf(tmp,"%s(%s)",&p->op1[2],p->op2);
			else 
				sprintf(tmp,"-%s(%s)",&p->op1[1],p->op2);
			p->op1 = tmp;
			break;
		case INCL:
			p->op2 = p->op1;
			p->op1 = getspace(8);
			sprintf(p->op1,"1(%s)",p->op2);
			break;				
		case DECL: 
			p->op2 = p->op1;
			p->op1 = getspace(9);
			sprintf(p->op1,"-1(%s)",p->op2);
			break;				
		case SHLL:
			tmp = getspace(10);
			sprintf(tmp,"(,%s,%d)",p->op2, 1 << (p->op1[1] - '0'));
			p->op1 = tmp;
			break; 
	}
	chgop(p, LEAL, "leal");
}

#define is8bit(cp)  (cp && *cp == '%' && (cp[2] == 'l'|| cp[2] == 'h'))

/* This module eliminates redundant moves from register to
**  register. If there is a move from register r1 to register r2
**  and then a series of uses of r2 then remove the mov instruction
**  and change all uses of r2 to use r1.
**  Some conditions may disable this optimization. They are looked for
**  and explained in comments.
*/
static void
try_backward(cb,movinst,firsti,lasti) BLOCK *cb; NODE *firsti, *lasti, *movinst;
{
  NODE *q,*new_set_src = NULL, *firstset = NULL, *may_new_src = NULL;
  NODE *jtarget;
  BLOCK *cb1;
  char *tmp;
  int not8adr,new_set;
  unsigned int srcreg,dstreg,use_target;
  boolean   give_up = 0;
  int movsize = OpLength(movinst);
#ifdef FLIRT
  static int numexec=0;
#endif

#ifdef FLIRT
	  ++numexec;
#endif

	if (movinst == firsti) /*  Can't go backward from the first instruction */
		return;
#ifdef DEBUG
	if (fflag) {
		fprintf(stderr,"%c move inst ",CC);
		fprinst(movinst);
	}
#endif
	srcreg = movinst->uses;
	dstreg = movinst->sets;
	not8adr = (dstreg & (ESI|EDI|(fixed_stack ? EBP : EBI))) && (srcreg & (EAX|EBX|ECX|EDX));
	for (cb1 = cb, q = movinst->back; q != firsti->back; q = q->back) {
		if (q->op == ASMS || is_safe_asm(q))
			return;
		if (q == cb1->firstn->back)
			cb1 = cb1->prev;
		if ((q->uses & srcreg) && /*dont mess with implicit uses of registers*/
			((isshift(q) && (srcreg & CL) && (isreg(q->op1)))
			|| (hard_uses(q) & srcreg)
			|| (! ((scanreg(q->op1,false) & srcreg) 
			|| (scanreg(q->op2,false) & srcreg))))
		 || (hard_sets(q) & srcreg)) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c funny uses, give up.\n",CC);
				fprinst(q);
			}
#endif
			give_up = true;
			break;
		}
		if (   (q->sets & srcreg) /*dont mess with implicit set of registers*/ 
			&& ( setreg(dst(q)) != (q->sets & ~CONCODES_AND_CARRY)) ) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"implicit set. give up.");
				fprinst(q);
			}
#endif
			break;
		}
		if (OpLength(q) == ByTE
		  && not8adr  /* Can't replace %esi with %eax if %al is used */
		  && (  (is8bit(q->op1) && (setreg(q->op1) & srcreg)) 
			  || (is8bit(q->op2) && (setreg(q->op2) & srcreg)) 
			 ) 
		) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"esi edi vs byte. give up.");
				fprinst(q);
			}
#endif
			break;
		}

		if (isbr(q)) {
			if (cb1->nextr) {
				jtarget = cb1->nextr->firstn;
				while (islabel(jtarget))
					jtarget = jtarget->forw;
				use_target = jtarget->uses | jtarget->nlive;
				if ((jtarget->sets & srcreg) && ! (jtarget->uses & srcreg))
					use_target &= ~srcreg;
			} else {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"Jump to undef location or end of block: ");
					fprinst(q);
				}
#endif
				give_up = true;
				break;
			}
			if	(!isuncbr(q) /*otherwise jtarget is undefined*/
				&& ( ( q->nlive & srcreg && use_target & srcreg)
				|| (q->nlive & dstreg && use_target & dstreg))
				) {
				give_up = true;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"src live at branch, give up.");
					fprinst(q);
				}
#endif
				break;
			}
		}
/* If we can separate between the set and the use of the src we may do it.
   We can use the src register and sets the destination register
   for example "inc %src" can be "leal 1(%src),%dest" 
   If by doing it we will get less good instruction we will do it only
   if we can't do the mov reg,reg any other way */
		if(( q->uses & q->sets & srcreg) &&  
		   ((new_set = is_new_value(q,srcreg)) != 0)){
				if ( (! may_new_src) && new_set == 2 && movsize == LoNG) {
					may_new_src = q;
			} else if (new_set == 1 && movsize == LoNG) {
				firstset = new_set_src = q;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"separate ");
					fprinst(q);
				}
#endif
				break;
			}
		}
/* Chack if we used full register and moved less then full register */
		if (q->uses & srcreg) {
			if ((movsize < LoNG) && 
				((((int) OpLength(q)) > movsize) || (indexing(q) & srcreg))
				 )  {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c big use of src. give up.\n",CC);
					fprinst(q);
					}
#endif
				give_up = true;
				break;
			}
		} else if (q->sets & srcreg) {
			int srcsize,dstsize;
			if ((q->sets & srcreg) != srcreg) { /* set less than reg */
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c set only part of src. \n",CC);
					fprinst(q);
				}
#endif
				continue;
			}
			if ((((int) OpLength(q)) > movsize) ||
				(ismove(q,&srcsize,&dstsize) && dstsize > movsize)) {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c set more then src. \n",CC);
					fprinst(q);
				}
#endif
				give_up = true;
				break;
			} else {
				firstset = q; /* A good targeting set of register */
				break; /* no further questions */
			}
		}
		if ((q->sets | q->uses | q->nlive) & dstreg) {
		   /*The old dst reg is set or used. Don't check it for the 
		     first source setting. So it must be the last test  */
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c dst is used or set. give up.\n",CC);
				fprinst(q);
			}
#endif
			give_up = true;
			break;
		}
	}/*end inner loop*/
	if (give_up) {
		if (may_new_src && srcreg != ESP) { /* Go back to separate.  */
			firstset = new_set_src = may_new_src;
		}
		else {
#ifdef DEBUG
			if (fflag) {
					fprintf(stderr,"check point 1, have to give up.\n");
			}
#endif
			return;
		}
	} else 
		may_new_src = NULL;
	if ( ! firstset || (q == firsti->back)) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, no set in block.\n");
		}
#endif
		return;
	}
	if ( firstset->nlive & dstreg) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, srcreg is set and dstreg is needed.\n");
		}
#endif
		return;
	}
	if ( srcreg & movinst->nlive){ /* Replace forward and backward */
#ifdef DEBUG
			if (fflag) {
			fprintf(stderr,"%c try change forward",CC);
				fprinst(q);
			}
#endif
		tmp = movinst->op1;
		movinst->op1 = movinst->op2;
		movinst->op2 = tmp;
		movinst->uses = dstreg;
		movinst->sets = srcreg;
		if (! try_forward(cb,movinst,lasti)) {
			movinst->uses = srcreg;
			movinst->sets = dstreg;
			movinst->op2 = movinst->op1;
			movinst->op1 = tmp;
			return;
		}
	} else
		tmp = NULL;
	if (may_new_src && may_new_src->op != LEAL) {
		new_leal(firstset);
	}
	for (q = movinst->back; q != firstset->back; q = q->back) {
		if (q->uses & srcreg || q->sets & srcreg) { 
#ifdef DEBUG
			if (fflag) {
			fprintf(stderr,"%c finally change ",CC);
				fprinst(q);
			}
#endif
			if (q == new_set_src)
				replace_registers(q,srcreg,dstreg,2);
			else
				replace_registers(q,srcreg,dstreg,3);
			new_sets_uses(q);
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c to ",CC);
				fprinst(q);
			}
#endif
		}/*endif q->uses & srcreg*/
	}/*for loop*/
	if (! tmp) {
		ldelin2(movinst);
		FLiRT(O_TRY_BACKWARD,numexec);
		DELNODE(movinst);
	} 
	fix_live(firstset,movinst,dstreg,srcreg);
	return;
}/*end try_backward*/

/*Test the conditions under which it is ok to remove the copy instruction.
**This is either if there is no instruction that make it illegal, and then
**the give_up flag is set, or if an alternative copy is met, and then the
**testing is stopped.
*/
static int
try_forward(cb,movinst,lasti)  BLOCK *cb; NODE *lasti, *movinst;
{
  NODE *q,*new_set_dst = NULL, *lastuse = NULL, *may_new_dest = NULL;
  NODE *jtarget;
  NODE *srcset = NULL;
  BLOCK *cb1;
  int not8adr,new_set;
  unsigned int srcreg,dstreg,use_target;
  boolean dst_is_changed = 0, srcset_now, give_up = 0;
  int movsize = OpLength(movinst);
#ifdef FLIRT
  static int numexec=0;
#endif

#ifdef FLIRT
	  ++numexec;
#endif

#ifdef DEBUG
	if (fflag) {
		fprintf(stderr,"%c move inst ",CC);
		fprinst(movinst);
	}
#endif
	srcreg = movinst->uses;
	dstreg = movinst->sets;
	not8adr = (srcreg & (ESI|EDI|(fixed_stack ? EBP : EBI))) && (dstreg & (EAX|EBX|ECX|EDX));
	/* go from the copy inst. to the end of the ebb, and do the checks */
	for (cb1 = cb, q = movinst->forw; q != lasti->forw; q = q->forw) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check ");
			fprinst(q);
		}
#endif
		if (q->op == ASMS || is_safe_asm(q))  /*disable */
			return 0;
		srcset_now = false; /*init*/
		if (q == cb1->lastn->forw)
			cb1 = cb1->next;
		/*dont mess with implicit uses of registers*/
		/*these uses can not be changed to use the second register*/
		if ((q->uses & dstreg) &&
			((isshift(q) && (dstreg & CL) && (isreg(q->op1)))
			|| (hard_uses(q) & dstreg)
			|| (! ((scanreg(q->op1,false) & dstreg) 
			|| (scanreg(q->op2,false) & dstreg))))
		 || (hard_sets(q) & dstreg)) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c funny uses, give up.\n",CC);
				fprinst(q);
			}
#endif
			give_up = true;
			break;
		}
		/*If there is a usage of the destination register as a byte register*/
		/*and the source register is esi, edi or ebp then can not replace them*/
		if (OpLength(q) == ByTE
		  && not8adr
		  && (  (is8bit(q->op1) && (setreg(q->op1) & dstreg)) 
			  || (is8bit(q->op2) && (setreg(q->op2) & dstreg))) 
		) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"esi edi vs byte. give up.");
				fprinst(q);
			}
#endif
			break;
		}

		/*Check if the copy is needed at the destination of a jump*/
		if (isbr(q)) {
			/*find the destination of the jump, go beyond the label(s)*/
			/*and find what registers are live there. l/d anal needs a*/
			/*little correction here, add use bits.*/
			if (cb1->nextr) {
				jtarget = cb1->nextr->firstn;
				while (islabel(jtarget))
					jtarget = jtarget->forw;
				use_target = jtarget->uses | jtarget->nlive;
				/*In the following case, the register is marked, but it's*/
				/*previous value is irrelevant.                          */
				if ((jtarget->sets & dstreg) && ! (jtarget->uses & dstreg))
					use_target &= ~dstreg;
			} else { /*nextr == NULL -> jtarget == NULL */
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"Jump to undef location or end of block: ");
					fprinst(q);
				}
#endif
				give_up = true;
				break;
			}
			/*Two cases here not to be able to eliminate the copy: */
			/*1. If the dest register is live at a branch target, or */
			/*2. If the dest reg is changed and the source reg is needed */
			if	(!isuncbr(q) /*otherwise jtarget is undefined*/
			  && ((q->nlive & dstreg && use_target & dstreg)
			  || (dst_is_changed && q->nlive & srcreg
			  && use_target & srcreg))
			) {
				give_up = true;
#ifdef DEBUG
				if (fflag) {
					if (q->nlive & dstreg && use_target & dstreg)
						fprintf(stderr,"dstreg used at target.\n");
					if (dst_is_changed && q->nlive & srcreg
						&& use_target & srcreg)
						fprintf(stderr,"dst changed, src used at target\n");
					fprinst(jtarget);
					fprintf(stderr,"dst live at branch, give up.");
					fprinst(q);
				}
#endif
				break;
			}
		}/*endif isbr*/
/* If we can separate between the set and the use of the src we may do it.
   We can use the src register and sets the destination register
   for example "inc %src" can be "leal 1(%src),%dest" 
   If by doing it we will get less good instruction we will do it only
   if we can't do the mov reg,reg any other way */
		if(( q->uses & q->sets & dstreg) && 
		   ((new_set = is_new_value(q,dstreg)) != 0)){
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"uses & sets dst, new val ");
				fprinst(q);
			}
#endif
			if ( ! srcset) {
				if ( (! may_new_dest) && new_set == 2 && movsize == LoNG) {
					may_new_dest = q;
				} else if (new_set == 1 && movsize == LoNG) {
					lastuse = new_set_dst = q;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"separate, nfq ");
						fprinst(q);
					}
#endif
					break;
				}
			} else {
				give_up = true; /*give up*/
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"sets & uses dst, src is set, give up.");
					fprinst(q);
				}
#endif
				break; /* no further questions */
			}
		}/*endif q sets dest a new value*/
		if (q->sets & srcreg) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"q sets src ");
				fprinst(q);
			}
#endif
		   /*mark the state that src reg is set. */
			if (!srcset)
				srcset_now = true;
			srcset = q;
				
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c set src ",CC);
				fprinst(srcset);
			}
#endif
		}
		if (q->uses & dstreg) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"q uses dst ");
				fprinst(q);
			}
#endif
			if ((movsize < LoNG) && 
				((((int) OpLength(q)) > movsize) || (indexing(q) & dstreg))
				 )  {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c big use of dst. give up.\n",CC);
					fprinst(q);
					}
#endif
				 give_up = true;
				 break;
			}
			lastuse = q;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"/ lastuse "); fprinst(lastuse);
			}
#endif
			if (q->sets & dstreg) { 
				if (q->nlive & srcreg) {
					give_up = true; /*give up*/
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"src live and dst set, give up ");
						fprinst(q);
					}
#endif
					break; /* no further questions */
				}
				dst_is_changed = 1;
			}
			if (((!srcset) || srcset_now ) && !(q->nlive & (dstreg|srcreg))
				 && (!(q->uses & srcreg) || !dst_is_changed)  ) {
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"src not set, dst not live, nfq");
					fprinst(q); 
				}
#endif
				break; /* no further questions */
			}
			if (srcset) {
				/* src reg was changed, now dst is used, give up*/
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"%c uses dstreg and src set, give up.\n",CC);
					fprintf(stderr,"%c uses:",CC); fprinst(q);
				}
#endif
				give_up = true; /*give up*/
				break;
			}
		} else if (q->sets & dstreg) {
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"q no uses dst, q sets dst");
				fprinst(q);
			}
#endif
			if ((q->sets & dstreg) != dstreg) { /* set less than reg */
				dst_is_changed = 1;
				lastuse = q;
			} else {
				if (!lastuse)
					lastuse = q->back;
#ifdef DEBUG
				if (fflag) {
					fprintf(stderr,"q ! use dst, q set all dst ");
					fprinst(q);
				}
#endif
				break; /* no further questions */
			}
		}
#ifdef DEBUG
		else {
			if (fflag) {
				fprintf(stderr,"q no uses no sets dst ");
				fprinst(q);
			}
		}
#endif
		if (q->uses & srcreg && dst_is_changed) {
			give_up = true;
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"src needed after dst is set.\n");
				fprinst(q);
			}
#endif
			break;
		}/*end if*/
	}/*end inner loop*/
	if (give_up) {
		/*try to recover from the give up: */
		if (may_new_dest && srcreg != ESP) {
			lastuse = new_set_dst = may_new_dest;
			if (lastuse->op != LEAL)
				new_leal(lastuse);
		} else {

#ifdef DEBUG
			if (fflag) {
					fprintf(stderr,"check point 2, have to give up.\n");
			}
#endif
			return 0;
		}
	}/*endif giveup*/
	/*giveup was not set, but other conditions may prevent the optimization*/
	if ( ! lastuse) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, no last use last change.\n");
		}
#endif
		return 0;
	}
	if ((q == lasti->forw) && (lasti->nlive & dstreg)) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, dest live in the end.\n");
		}
#endif
		return 0;
	}
#ifdef DEBUG
	else if (fflag) {
		if (q != lasti->forw)
			fprintf(stderr,"iner loop broke before end.\n");
		if (!(lasti->nlive & dstreg))
			fprintf(stderr,"dest reg dont live in the end.\n");
	}
#endif
	if ( dst_is_changed && lastuse->nlive & srcreg) {
#ifdef DEBUG
		if (fflag) {
			fprintf(stderr,"check point, dstreg is changed and srcreg is needed.\n");
		}
#endif
		return 0;
	}
	for (q = movinst; q != lastuse->forw; q = q->forw) {
		if (q->uses & dstreg || q->sets & dstreg) { 
#ifdef DEBUG
			if (fflag) {
			fprintf(stderr,"%c finally change ",CC);
				fprinst(q);
			}
#endif
			if (q == new_set_dst)
				replace_registers(q,dstreg,srcreg,1);
			else
				replace_registers(q,dstreg,srcreg,3);
			new_sets_uses(q);
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"%c to ",CC);
				fprinst(q);
			}
#endif
		}/*endif q->uses & dstreg*/
	}/*for loop*/
	fix_live(movinst,lastuse,srcreg,dstreg);
	ldelin2(movinst);
	DELNODE(movinst);
	FLiRT(O_TRY_FORWARD,numexec);
	return true;
}/*end try_forward*/

static void
rmrdmv(cb,lasti) BLOCK *cb; NODE *lasti;
{
  NODE *p;
  NODE *firsti = cb->firstn;
#ifdef DEBUG
	if (fflag) {
		fprintf(stderr,"new ebb ");
		fprinst(cb->firstn);
		fprintf(stderr,"until ");
		fprinst(lasti);
	}
#endif
	for (p = firsti; p != lasti->forw; p = p->forw) {  /* find the copy inst. */
		if (p->op == ASMS)  /* disable for all the ebb if found an asm */
			return;
		if (p == cb->lastn->forw)  /* update current basic block */
			cb = cb->next;
		if (( p->op == MOVL || p->op == MOVB || p->op == MOVW )
		   && isreg(p->op1)
		   && isreg(p->op2)
		   && !samereg(p->op1,p->op2) /*dont do it for same register*/
		   && (fixed_stack || !(p->sets == EBP)) /*save time for move esp,ebp, it fails anyway*/
		 )  {
			COND_SKIP(continue,"%d ",second_idx,0,0);
			if ( ! try_forward(cb,p,lasti)) /* copy propagation */
				try_backward(cb,p,firsti,lasti);  /* targeting */
		 }
	}/*main for loop*/
}/*end rmrdmv*/

void
replace_registers(p,old,new,limit) 
NODE *p;
unsigned int old,new;
int limit;
{
  int m;
  int length;
  unsigned int net_dst;
  char *oldname, *newname;
  char *opst,*opst2,*opst3;
  for (m = (limit == 2 ) ? 2: 1 ; m <= limit; m++) {
	  if (p->ops[m] && (old & (net_dst = scanreg(p->ops[m],false) & full_reg(old)))) {
		if (net_dst == old) {
			oldname = itoreg(old);
			newname = itoreg(new);
		}
		else if (net_dst == L2W(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(L2W(new));
		}
		else if (net_dst == L2B(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(L2B(new));
		} 
		else if (net_dst == L2H(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(L2H(new));
		} 
		else if ( net_dst ==  full_reg(old)) {
			oldname = itoreg(net_dst);
			newname = itoreg(full_reg(new));
		}
		opst = getspace(strlen(p->ops[m]));
		opst2 = p->ops[m];
		p->ops[m] = opst;
		length = strlen(newname);
		while(*opst2 != '\0') {
		  if (*opst2 == *oldname
		   && strncmp(opst2,oldname,length) == 0) {
			  for(opst3 = newname; *opst3;)
				  *opst++ = *opst3++;
			  opst2 += length;
			  continue;
		  }
		  *opst++ = *opst2++;
		}
		*opst = '\0';
	  }/*endif*/
	}/*for loop*/
}/*end replace_registers*/

/* use *%reg and not %reg in case ao function call */
/* *%reg is used by indirect call */
static char *
itostarreg(i) unsigned int i;
{
  switch (i) {
	case EAX:  return "*%eax";
	case EDX:  return "*%edx";
	case EBX:  return "*%ebx";
	case ECX:  return "*%ecx";
	case ESI:  return "*%esi";
	case EDI:  return "*%edi";
	case EBI:  return "*%ebi";
	case EBP:  return "*%ebp";
	case ESP:  return "*%esp";
  }/*end switch*/
  /* NOTREACHED */
}/*end itostarreg*/

/* Link all temp in the block and then try to eliminate them one by one */
void
rm_tmpvars()
{
	NODE *p,*q;
	AUTO_REG *reg;
	unsigned int r;
	char *r_str,*r_str1;
	BLOCK *b;

#ifdef FLIRT
	static int numexec=0;
#endif

	COND_RETURN("rm_tmpvars");

#ifdef FLIRT
	++numexec;
#endif

	bldgr(false);
	b = b0.next;
	regal_ldanal();
	for (p = n0.forw; p != &ntail; p = p->forw) {
		if (p == b->lastn->forw)
			b = b->next;
		reg = getregal(p); 
		if (! reg || reg->size == TEN) { /* does p uses regal, no long doubles */
			continue;
		}
		if ((! (reg->bits & p->nrlive)) && /* /REGAL is dead after */ 
			(! (p->sets & p->nlive)) && /*if register was set it was not used */
			(p->op !=CALL) && (p->op != LCALL) && !IS_FIX_PUSH(p)) { 
			/* Not /REGAL MEM was set */ 
				COND_SKIP(continue,"%d %s %s ",second_idx,p->opcode,dst(p));
				if (! isfp(p)) {     /* don't remove FP code */
#ifdef DEBUG
					if (fflag) { 
						fprintf(stderr," deleted reg->bits = %x ",reg->bits); 
						fprintf(stderr," p->nrlive = %8.8x \n",p->nrlive);
						fprinst(p);
					}
#endif
					ldelin2(p); /* temp was set and never used. */
					DELNODE(p); /* Remove it */
					FLiRT(O_RM_TMPVARS1,numexec);
					continue;
				} else {  /* fp setting */
					if (p->op == FSTPL || p->op == FSTPS) {
						chgop(p,FSTP,"fstp");
						p->op1 = "%st(0)";
						continue;
					} else if (p->op == FSTL || p->op == FSTS) {
						DELNODE(p);
						continue;
					}
				}
		}
		if ((r = find_free_reg(p,b,reg->bits)) != NULL) { /* reg was found */ 
#ifdef DEBUG
			if (fflag) {
				fprintf(stderr,"found  r = %s \n",itoreg(r));
				fprinst(p);
			}
#endif
		  if (!IS_FIX_PUSH(p)) {
			r_str = itoreg(r);
			for(q = p ; ; ) {
				if( reg == getregal(q)) { /* replace temp by reg */
					switch(OpLength(q)) {
						case ByTE:
							r_str1 = itoreg(L2B(r));
							break;
						case WoRD:
							r_str1 = itoreg(L2W(r));
							break;
						default:
							r_str1 = r_str;
					}
					if (isindex(q->op1,fixed_stack?ESP_STR:EBP_STR)) {
						if (q->op == MOVSBW || q->op == MOVSBL || 
										q->op == MOVZBW || q->op == MOVZBL)
							q->op1 = itoreg(L2B(r));
						else if (q->op == MOVSWL || q->op == MOVZWL)
							q->op1 = itoreg(L2W(r));
						else if (q->op == CALL)
							q->op1 = itostarreg(r);
						else 
							q->op1 = r_str1;
					} else
						q->op2 = r_str1;
					FLiRT(O_RM_TMPVARS2,numexec);
					new_sets_uses(q);
				}
				if (! (q->nrlive & reg->bits))
					break;
				else 
					q->nrlive &= ~reg->bits;
				q->nlive |= r; /* Mark reg as live */
				q= q->forw;
			}
		  }
		}
	}
}

/* new_offset will get a str for operand. Register name that is used as base 
 or index in str and the register content value in val. It will remove the 
 register from the operand and add the val to the displacement.
*/
static void
new_offset(str,reg,val)
char **str;
unsigned int reg;
int val;
{
int change = 0;
char name_buf[MAX_LABEL_LENGTH];
char *name;
int x,scale;
unsigned int base,index;
char *t,*rand,*fptr;
char sign;
	fptr = (**str == '*') ? "*" : ""; /*  function pointer call */
	t = strchr(*str,'(');
	base = setreg(1+t); /*base register*/
	t = strchr(t,',');
	if (t)
		index = setreg(t+1); /*index register*/
	else
		index = 0;
	name = (strlen(*str) < ((unsigned ) MAX_LABEL_LENGTH)) ? 
			                       name_buf : getspace(strlen(*str)); 
	(void) decompose(*str,&x,name,&scale); /*rest components*/
	if (index == reg) { /* Do index first. If not the index will become base */
		change = val * scale;
		remove_index(str);
	}
	if (base == reg) { /* The register is used as base */
		change += val;
		remove_base(str);
	}
	rand = getspace(strlen(*str) + 12);
	change +=x;
	t = strchr(*str,'(');
	if ( change == 0) { 
		if ( name[0] || t ) { 
			if (t)
				sprintf(rand,"%s%s%s",fptr,name,t);
			else
				sprintf(rand,"%s%s",fptr,name);
		} else 
			rand = "0";
	}
	else if (name[0]) {
		if (change > 0) sign = '+';
		else { 
			sign = '-';
			change = -change;
		}
		if (t)
			sprintf(rand,"%s%s%c%d%s",fptr,name,sign,change,t);
		else
			sprintf(rand,"%s%s%c%d",fptr,name,sign,change);
	} else {
		if (t)
			sprintf(rand,"%s%d%s",fptr,change,t);
		else
			sprintf(rand,"%s%d",fptr,change);
	}
	*str = rand;
}/*end new_offsets*/

static int 
reg_index(reg) unsigned int reg;
{
	switch (reg) {
		case EAX: return 0;
		case EDX: return 1;
		case ECX: return 2;
		case EBX: return 3;
		case ESI: return 4;
		case EDI: return 5;
		case EBI: return 6;
		case EBP: if (fixed_stack) return 6;
		default:
			return 7;
	}
}

/* const_index will find base/index registers with constant value and will 
** remove the register and add the value to the displacement */
/* When fixed stack mode is active, the last place in all the array variables, the
** one used before by %ebi will now be used by %ebp */

void
const_index()
{
  NODE *p;
  unsigned int pidx;
  int i,i1,i2,m,dummy;
  int const_val[NREGS -1]; /* The integer value of the constant  */
  char  *const_str[NREGS -1]; /* The constant string  */
  static int reg_regmasx[] = { EAX,EDX,ECX,EBX,ESI,EDI,EBI,0};
  static int fix_regmasx[] = { EAX,EDX,ECX,EBX,ESI,EDI,EBP,0};
  static int *regmasx;
  boolean ispwrof2();
#ifdef FLIRT
  static int numexec=0;
#endif
	COND_RETURN("const_index");

#ifdef FLIRT
	++numexec;
#endif

	regmasx = fixed_stack ? fix_regmasx : reg_regmasx;
	for (p = n0.forw; p != &ntail; p = p->forw) {
		if (   isuncbr(p)
		    || islabel(p)
		    || p->op == ASMS ||
		    is_safe_asm(p)) {

			for(i = 0; i < (NREGS -1); i++) {
				/* new EBB, reset all values */
				const_val[i] = 0;
				const_str[i] = NULL;
			}
			continue;
		}
		COND_SKIP(continue,"%d ",second_idx,0,0);

		/* If idiv by constant can be removed by w3opt mark the register */
		if (   p->op == CLTD
		    && (p->forw->op == IDIVL) 
		    && isreg(p->forw->op1)
		    && ispwrof2(m = const_val[reg_index(setreg(p->forw->op1))],&dummy)
		    && (   p->back->op != MOVL
		        || strcmp(p->back->op2, p->forw->op1))) {

			NODE * pnew, *prepend();

			pnew = prepend(p,p->op2);
			chgop(pnew, MOVL, "movl");
			pnew->op1 = getspace(ADDLSIZE);
			sprintf(pnew->op1,"$%d",m); 
			pnew->op2 = p->forw->op1;
			new_sets_uses(pnew);
			pnew->nlive |= pnew->sets; 
			lexchin(p,pnew);		/* preserve line number info */ 
			FLiRT(O_CONST_INDEX,numexec);
		}
		if (isconst(p->op1) ) {
			 if (isshift(p))  {
				/* use an unsigned int to propagate unsigned shift right*/
				if (   isdigit(p->op1[1])
				    && isreg(p->op2)
				    && ( i = const_val[reg_index(setreg(p->op2))])) {

					i1 = (int)strtoul(p->op1+1,NULL,0);
					if (p->op == SHRL) {

						unsigned int i = const_val[reg_index(setreg(p->op2))];
						i = i >> i1;
						chgop(p, MOVL, "movl");
						p->op1 = getspace(ADDLSIZE);
						sprintf(p->op1,"$%d",i);
						p->uses = 0;
						p->sets = setreg(p->op2);
					} else {
						switch (p->op) {
							case SHLL: 
								i = i << i1;
								break;
							case SHRL: 
								i = i >> i1;
								break;
							default:
								i = -1;
						}
						if ( i != -1 ) {
#ifdef DEBUG
							if (fflag) {
								fprinst(p);
								fprintf(stderr,"shift %d by constant %s: ",
									const_val[reg_index(setreg(p->op2))],p->op1 );
							}
#endif
							chgop(p, MOVL, "movl");
							p->op1 = getspace(ADDLSIZE);
							sprintf(p->op1,"$%d",i); 
							p->uses = 0;
							p->sets = setreg(p->op2);
							FLiRT(O_CONST_INDEX,numexec);
#ifdef DEBUG
							if (fflag) {
								fprinst(p);
							}
#endif
						}
					}
				}

			} else if (! p->op3 && !(p->sets & ESP)) {

				/* can't replace const in imull with op3, dont want to complicate
				** settings of esp, make fp_elimination harder.
				*/
				for ( i = 0 ; i < (NREGS-1) ; i++) {
					if (   OpLength(p) == ByTE
					    && (i > 3))  {

						/* Must use byte register */
						break;
					}
					if (   const_str[i]
					    && (! strcmp(p->op1,const_str[i]))) {

						unsigned int reg;
						switch (OpLength(p)) {
							case ByTE: reg = L2B(regmasx[i]); break;
							case WoRD: reg = L2W(regmasx[i]); break;
							case LoNG: reg = regmasx[i];      break;
						}
						p->op1 = itoreg(reg);
						p->uses |= reg; 
#ifdef DEBUG
						if (fflag) {
							fprintf(stderr,"replace %s by %s in ",const_str[i],p->op1);
							fprinst(p);
						}
#endif
					}
				}
			}
		} 
		if (pidx = scanreg(p->op1,true)) {
			m = 1;

		} else if (pidx = scanreg(p->op2,true)) {
			m = 2;

		} else if (pidx = scanreg(p->op3,true)) {
			m = 3;
		}
		if (pidx) {
			for(i = 0; i < (NREGS -1); i++) {
				if ((pidx  & regmasx[i]) && const_val[i]) { 
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"found reg %d with %d in: ",i,const_val[i]);
						fprintf(stderr," was p->ops[%d] = %s \n",m,p->ops[m]);
					}
#endif
					new_offset(&p->ops[m],regmasx[i],const_val[i]);
					new_sets_uses(p);
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr," now p->ops[%d] = %s \n",m,p->ops[m]);
						fprinst(p);
					}
#endif
				}
			}
		}

		if (   p->op == LEAL
		    && (! strchr(p->op1,'(')) 
		 /* &&  ( p->op1[1] == '-' || isdigit(p->op1[1])) */ ) {

			char *tmp;
			chgop(p, MOVL, "movl");
			tmp = p->op1;
			p->op1 = getspace(strlen(tmp) + 2); /* 1 for '$' and 1 for \0 */
			sprintf(p->op1, "$%s", tmp);
			p->uses = 0;
			p->sets = setreg(p->op2);
			FLiRT(O_CONST_INDEX,numexec);
		}
		if (   p->op == MOVL
		    && isconst(p->op1)
		    && isreg(p->op2)) {

			/* found movl $val,%reg */
			const_str[i1=reg_index(p->sets)] = p->op1;
			if (   p->op1[1] == '-'
			    || isdigit(p->op1[1]))  {
				const_val[i1] = (int)strtoul(p->op1+1,(char **)NULL,0); /* save val */
			} else {
				const_val[i1] = 0;
			}

		} else if (   p->op == MOVL
		           && isreg(p->op1)
		           && isreg(p->op2)
			/*esp, ebp are not traced, so can not propagate them. */
		           && (! ((p->sets | p->uses) & (EBP | ESP)))) {

				const_str[i1 = reg_index(p->sets)] = 
									const_str[i2 = reg_index(p->uses)];
				const_val[i1] = const_val[i2];
		} else {
			/* kill value for set registers */
			for ( i = 0 ; i < (NREGS -1) ; i++) {
				if (p->sets & regmasx[i]) {
					const_val[i] = 0;
					const_str[i] = NULL;
				}
			}
		}
	}
} /* end const_index */


/* This section's purpose is to replace sections of code comprising conditional 
 * jumps with one basic block if possible. for the sake of this purpose 
 * setcc instr. and some CF manipulations are used. setcond() is a driver
 * of this section.
 * Transformation is done in 3 phases:
 * 1) setcond() accomplishes recognition of available sequencee of basic 
 *    blocks producing appropriate actions ( e.g. if_x==0_y=1_else_y=0 ).
 *    If possible it converts "if_then_else" to "if_then" by means of
 *	  if_then_else2if_then().
 * 2) transform_b1() replaces test & jcc instructions with instructions 
 *    setting CF or EAX  and returns a value saying what transform_b2() 
 *    needs for further transformations.
 * 3) transform_b2() replaces an instruction using 1,0 with
 *    an instruction using CF or EAX . 
 * This value is the only interface between 2) and 3) so a big number 
 * of combination may be handled.
 * External dependencies  : invocation should be placed after peep()
 */

extern struct RI *reginfo;

/* This structure represents free reg used in transformation .
 * In cases of _mov?_mov? we use mov's destination reg as free reg , 
 * in other cases reg is obtained by get_free_reg(). 
 */

static struct 
{
	unsigned code;
	char *name;
	char *content;
} 
reg;

/* This type contains identifiers for several actions performed by the
 * basic block before transformation
 */
typedef enum 
{	
	NO_ACT=0, SET_=0xf0, SET_0=0x10, SET_1=0x20, SET_min1=0x40, 
	SET_X=0x100, /* set from reg still not handled */ 
	INC_DEC=0xf, INC_1=0x1, DEC_1=0x2 
} 
action_type;

/* This struct is used to keep some inform about involved bas.bl. */
static struct 
{
	BLOCK *b;
	NODE  *p, *test;      /* node changing variable */
	action_type act;
	int oplen, testlen;
}
bb1,bb2,bb3;

typedef enum { tr_fail, use_CF, use_old_reg, use_new_reg } trans_type;


/* function is used to determine action of basic block.   
 * external dependency: function was designed for call after w1opt().
 * In other cases ADD_1/SUB_1 will not be recognized as inc/dec.
 */
action_type
determ_action_type(p) NODE *p;
{
	if( ( p->op == INCL || p->op == INCW || p->op == INCB )
	)
		return INC_1;

	if( ( p->op == DECL || p->op == DECW || p->op == DECB )
	)
		return DEC_1;

	if( p->op == MOVL || p->op == MOVW || p->op == MOVB )
	{
		if( !strcmp(p->op1,"$0") ) return SET_0;

		if( !strcmp(p->op1,"$1") ) return  SET_1;

		if( !strcmp(p->op1,"$-1") ) return SET_min1;
		
		return 	SET_X;
	}
	
    if( ( p->op == XORL || p->op == XORW || p->op == XORB )
    &&  !strcmp(p->op1,p->op2) )
        return SET_0;

    return NO_ACT;
}   


/* changes   movl $1,%reg => incl %reg , if %reg contains zero.	*/
void
mov1_to_inc(act,p) action_type act; NODE *p;
{
	if( act == SET_1 )
		switch (p->op)
		{
			case MOVL : chgop(p,INCL,"incl");break;
			case MOVW : chgop(p,INCW,"incw");break;
			case MOVB : chgop(p,INCB,"incb");break;
		}
	else  /* act == SET_min1 */
		switch (p->op)
		{
			case MOVL : chgop(p,DECL,"decl");break;
			case MOVW : chgop(p,DECW,"decw");break;
			case MOVB : chgop(p,DECB,"decb");break;
		}
	p->op1 = p->op2;
	p->op2 = NULL;
	p->uses= p->sets;
	p->sets |= CONCODES;
}			

NODE *
changes_only_1_variable(p,u) NODE *p; int *u;
{
NODE *p1, *p2;

	if( is_any_label(p) ) { *u=0; return p; }  
	if( is_move(p) && ismem(p->op1) && isreg(p->op2) 
	&&  (p1=p->forw, p1) && (p2=p->forw->forw, p2) && (p1->uses & p->sets)
	&&	is_move(p2) && p2->uses &  p->sets && !strcmp(p->op1,p2->op2) )
	{
		*u=3;
		return p1;
	}	
	else
	{
		*u=1;
		return p;
	}	
}

/* function complete transformation showed bellow.
 * If A,B = ( 0,1,-1 )  it moves set_0 instruction into b1 and changes 
 * jcc condition if necessary. We assume it is invocated only from ALTsetcond.  
 *  
 	b1					b1				jcc	 .L1				movl B,%eax
    |-----i				x=A ---i		movl A,%eax				jcc	 .L1	
    b2    b3			|	   b3  		jmp  .L2				movl A,%eax
    x=A   x=B   ====>  	|	   x=B	.L1					===> .L1	
    |_____J				|______J		movl B,%eax			 .L2
    |					|			.L2
	b4					b4				...   					... 
 */
boolean 
if_then_else2if_then() 
{
BLOCK *bi;
int n=0;
NODE *p;

	/* Do we have only 1 jmp to the b2 label ? */
	for(bi=b0.next; bi!=NULL; bi=bi->next) 
		if( bi->nextr == bb3.b || bi->nextl == bb3.b ) n++;
	if( n>1 ) return false;
	
	if(bb2.act == SET_0 && bb3.act & ( SET_1 | SET_min1) )
	{
		br2opposite(bb1.b->lastn);
		swap_nodes(bb2.p,bb3.p);
		p=bb2.p; bb2.p=bb3.p; bb3.p=p;
		bb2.act = bb3.act;
		bb3.act = SET_0;
	}
    if(bb2.act & (SET_1 | SET_min1) && bb3.act == SET_0
    && !(bb2.p->sets & bb1.test->uses))
	{
        move_node(bb3.p, (bb1.test)->back);
		DELNODE(bb2.b->lastn);
		bb3.b->lastn = bb3.b->firstn;   /* reducced to label */
		reg.code = bb2.p->sets;
		reg.name = bb2.p->op2;
		reg.content = "$0";
		mov1_to_inc(bb2.act, bb2.p);
		return true;
	}
    if( target_cpu & (blend | P5 | P6 )
    && !(bb3.p->sets & (bb1.test->uses | bb2.p->uses) )
    && !(bb3.p->uses & bb2.p->sets) )
    {
        move_node(bb3.p, (bb1.test)->back);
		DELNODE(bb2.b->lastn);
		bb3.b->lastn = bb3.b->firstn;   /* reducced to label */
		return false;  
	}
	return false;
}

#define is_jmp_if_no_CF(jcc)  \
		( jcc->op == JNC || jcc->op == JAE || jcc->op == JNB )
#define is_jmp_if_CF(jcc)  \
		( jcc->op == JC || jcc->op == JB || jcc->op == JNAE )

/* Bellow we assume  jcc is following test, so OF=0 */
#define is_jmp_if_not_SF(jcc) \
		(jcc->op == JGE || jcc->op == JNL || jcc->op == JNS )
#define is_jmp_if_SF(jcc) \
		(jcc->op == JNGE || jcc->op == JL || jcc->op == JS )

#define is_test(p) ( p->op==TESTL || p->op==TESTW || p->op==TESTB )		
#define is_jmp_if_equal(p)  ( p->op==JZ || p->op==JE )


trans_type
setcc_in_reg()
{
NODE *jcc=bb1.b->lastn, *test=bb1.test, *mov0;
trans_type trt;

	if (   !reg.content
		|| strcmp(reg.content,"$0") 
		|| !(reg.code & (EAX|EBX|ECX|EDX)) )
/* if we have no reg with "$0" after previous transformation */
	{
		int i;
		i=get_free_reg(reg.code | bb2.p->nlive | test->nlive | 
					   test->sets | test->uses , 1 , test);
		if (i == -1)
			return tr_fail;
		else 
		{
			reg.code = reginfo[i].reg_number;
			reg.name = reginfo[i].regname[2]; /*quater reg */
			reg.content = "$0";
		}

        mov0 = insert(test->back);
        chgop(mov0,XORL,"xorl");
        mov0->op2= itoreg(reg.code);
        mov0->op1= mov0->op2;
        mov0->sets= reg.code;
        mov0->uses=0;
        mov0->nlive = test->back->nlive & ~CONCODES ;

		trt = use_new_reg;
	}
	else
	{
        return tr_fail;
	}	
	br2nset(jcc);
	jcc->op1=reg.name;
	jcc->uses= CONCODES | reg.code;
	jcc->sets = reg.code;
	jcc->nlive |= reg.code;			

	return  trt;	
}	

trans_type
do_if_eq_pwr2(n,pwr) int n,pwr;
{
NODE *jcc=bb1.b->lastn, *test=bb1.test;
int i,k;

	i=get_free_reg(reg.code | bb2.p->nlive | test->sets | test->uses | 
					test->nlive , bb1.testlen==ByTE, test);
	if(i == -1) return tr_fail;
	else 
	{
		int	j = (bb1.testlen==ByTE ? 2 : ( bb1.testlen==WoRD ? 1 : 0 ) );  
		reg.code = reginfo[i].reg_number;
		reg.name = reginfo[i].regname[j];
		reg.content = NULL;
	}
	
  	switch(test->op)
 	{
 		case TESTL : chgop(test,MOVL,"movl");break;
 		case TESTW : chgop(test,MOVW,"movw");break;
 		case TESTB : chgop(test,MOVB,"movb");break;
 	}	
	test->op1 = test->op2;
	test->op2 = reg.name;
	test->sets = reg.code ;
	test->nlive |= reg.code;
	/* uses not changed */
	if(n == 1) 
	{
	 	switch(bb1.testlen)
 		{
 			case LoNG : chgop(jcc,SARL,"sarl");break;
 			case WoRD : chgop(jcc,SARW,"sarw");break;
 			case ByTE : chgop(jcc,SARB,"sarb");break;
 		}	
		jcc->op1="$1";		
	}
	else
	{
	 	switch(bb1.testlen)
 		{
 			case LoNG : chgop(jcc,SALL,"sall"); k=32-pwr; break;
 			case WoRD : chgop(jcc,SALW,"salw"); k=16-pwr; break;
 			case ByTE : chgop(jcc,SALB,"salb"); k=8-pwr ; break;
 		}	
		jcc->op1 = getspace(ADDLSIZE);
		sprintf(jcc->op1,"$%d", k);		
	}
	jcc->op2 = reg.name;
	jcc->sets = reg.code | CONCODES_AND_CARRY;
	jcc->uses = reg.code;
	/* jcc->nlive isn't changed */

	return use_CF;
}

trans_type
do_test_jNSF()
{
NODE *jcc=bb1.b->lastn, *test=bb1.test;
int i;

	i=get_free_reg(reg.code | bb2.p->nlive | test->sets | test->uses | 
					test->nlive , bb1.oplen==ByTE, test);
	if (i == -1)
		return tr_fail;
	else 
	{	
		int	j = (bb1.testlen==ByTE ? 2 : ( bb1.testlen==WoRD ? 1 : 0 ) );  
		
		reg.code = reginfo[i].reg_number;
		reg.name = reginfo[i].regname[j];
		reg.content = NULL;
	}

	switch (bb1.testlen) {
		case ByTE:
			chgop(test,MOVB,"movb");
			chgop(jcc,SALB,"salb");
			break;
		case WoRD:
			chgop(test,MOVW,"movw");
			chgop(jcc,SALW,"salw");
			break;
		case LoNG:
			chgop(test,MOVL,"movl");
			chgop(jcc,SALL,"sall");
			break;
	}
	/* test-op1 */
	test->op2 = reg.name;
	test->sets = reg.code ;
	/* test->uses */
	test->nlive |= reg.code;
	/* uses is the same */
	  
	jcc->op1 = "$1";
	jcc->op2 = reg.name;
	jcc->sets=CONCODES_AND_CARRY | reg.code;
	jcc->uses= reg.code;
	jcc->nlive |= reg.code;

	return use_CF;
}

/* transform jcc to CF or to value in reg
 * function tries to identify private case, otherwise makes general 
 * transformation using setcc
 											( the number of clocks )
		case #1:	 						p4-1 p5-1
				set CF
				jcc(CF) .L			==>		set CF

		case #2 : 							p4-2,p5-2	
				testl %ecx,%ecx		==>		movl $0,%eax  
				je	  .L					cmpl %ecx,%eax
				use 1,0					    use CF
				
		case #3 :							p4-4 p5-2
				testw	%cx,%cx				movw	%cx,%ax
				jge		.L			==>		salw	$1,%ax
				use	1,0						use CF
		
		case #4 :							p5 :2 				p4: 3
				testl exp2(n),%esi	==> movl %esi,%eax			 movl %esi,%eax
 				je  .L					sall $(32-log2(n)),%ea	 andl $n,%eax
 				use 1,0					use CF					 negl %eax
 																 use  CF
 																 
 		general case :						p4-5 , p5-3 
				set CONCODES				set CONCODES 
 				jge   .L					movl  $0,%edx	
 				use 1,0						setcc %dl
											use   %edx
 * NOTES:
 * n.1 In case #1 CF is set by TEST instruction so removal of the jcc 
 *     instruction is the only action.
 */
trans_type
transform_b1()
{
NODE *jcc=bb1.b->lastn, *test=bb1.test ;
trans_type trt=tr_fail;
char *c;
							
	if( is_jmp_if_no_CF(jcc) ) /* case #1 */
	{
		DELNODE(jcc);
		return use_CF;
	}
	if( test->uses & bb2.p->sets 
	&&  !(bb2.act && bb3.act)   )
		 return tr_fail;  /* side effects */ 
	if( bb2.b->length > 1 )
		c=bb2.p->forw->op2; 
	else	
		c=dst(bb2.p);
	if( ismem( c ) && scanreg(c,false) & bb1.test->uses ) 
		return tr_fail;
	if( is_test(test) ) /* test X,X */
	{
		if( !strcmp(test->op1,test->op2) )  /* test %reg,%reg */ 
		{
			if( is_jmp_if_not_SF(jcc) ) /* case #3 */  
			{
				trt=do_test_jNSF();
				return trt;
			}
		}
		/* case #4 */
		if( isreg(test->op2) && isimmed(test->op1) 
		&& is_jmp_if_equal(jcc) )
		{
			int n ,log2n;
			n = (int)strtoul(test->op1+1,(char **)NULL,0);
			if( ispwrof2(n, &log2n) )
			{
				trt=do_if_eq_pwr2(n, log2n);
				return trt;
			}	
		}
    } /* end test X,X */

	/* next section handles general case in a less gracefull form using setcc 
	 * (on i486 setcc takes 3 cycles).  
	 */ 
	trt=setcc_in_reg();
	return trt;
}	

/* function peforms transformations showed bellow 
 *
 * case 2.1	: if_CF
 *			incl	%esi	===>	adcl	$1,%esi
 *		 
 * case 2.2 : if_EAX
 *			incl	%esi	===>	addl	%eax,%esi
 */
void 
transform_b2(trt) 
trans_type trt;
{
NODE *p=bb2.p;
unsigned op=p->op;

	if( trt == use_CF ) {
		switch( op ) 	
		{
			case INCL :	chgop(p,ADCL,"adcl");break;
			case INCW :	chgop(p,ADCW,"adcw");break;
			case INCB :	chgop(p,ADCB,"adcb");break;
			case DECL :	chgop(p,SBBL,"sbbl");break;
			case DECW :	chgop(p,SBBW,"sbbw");break;
			case DECB :	chgop(p,SBBB,"sbbb");break;
		}
		p->op2 = p->op1;
		p->op1 = "$0";
		p->uses |= CARRY_FLAG;
		p->sets |= CARRY_FLAG;

	} else if( trt == use_new_reg ) {
		switch( op ) 
		{
			case INCL :	chgop(p,ADDL,"addl");break;
			case INCW :	chgop(p,ADDW,"addw");break;
			case INCB :	chgop(p,ADDB,"addb");break;
			case DECL :	chgop(p,SUBL,"subl");break;
			case DECW :	chgop(p,SUBW,"subw");break;
			case DECB :	chgop(p,SUBB,"subb");break;
		}
		p->op2=p->op1;
		switch( bb2.oplen )
		{
			case ByTE : p->op1= itoqreg(reg.code);break;
			case WoRD : p->op1=itohalfreg(reg.code);break;
			case LoNG : p->op1= itoreg(reg.code);break;
		}
		p->uses |= reg.code;
		p->sets |= CARRY_FLAG;

	} else if( trt == use_old_reg ) {
		/* If changing instr to NEGx, must add in the fact that the CF
		   is also set.  Add it now; it will not matter if NODE is deleted. */
		p->sets |= CARRY_FLAG;
		switch( op ) 
		{
			case INCL :	case INCW : case INCB :   
				DELNODE(p); break;
			case DECL :	
				chgop(p,NEGL,"negl"); p->op1= itoreg(reg.code); break;
			case DECW :	
				chgop(p,NEGW,"negw"); p->op1=itohalfreg(reg.code);break;
			case DECB :	
				chgop(p,NEGB,"negb"); p->op1=itoqreg(reg.code); break;
		}
		/* uses are the same */
	}
}

void
setcond()
{
BLOCK *b,*b1,*b2,*b3,*b4,*b3i,*b4i;
trans_type trt;
int u ;

#ifdef FLIRT
static int numexec=0;
#endif

	COND_RETURN("setcond");

#ifdef FLIRT
	++numexec;
#endif
	bldgr(false);

	for(b=b0.next; b!=NULL; b=b->next)
	{	
		bb2.act=bb3.act=NO_ACT;
		/* we suppose that jump optimization has already been done and 
		 * blocks are textually adjacent ( before optim() ).
		 */
		b1=b;
	
		if( (b2=b1->next) == NULL) break;
		if( (b3=b2->next) == NULL) break;

		/* case -if-then- 
		 * ->b1->b2->b3->
		 *	 L_______^
		 */
		if (   b2->nextl == b1->nextr
			&&  b2->nextr == NULL 
			&&  b2->length <= 3  
			&& (bb2.p=changes_only_1_variable(b2->firstn,&u),
					u == b2->length && bb2.p)
			&& (bb2.act=determ_action_type(bb2.p)) & INC_DEC 
			&&  b1->lastn->back->sets & CONCODES_AND_CARRY )
		{
			bb1.b = b1;
			bb2.b = b2;
			bb1.test = b1->lastn->back;
			
			bb1.oplen = bb1.testlen = OpLength(b1->lastn->back);
			bb2.oplen = OpLength(bb2.p);

			reg.code = 0;
			reg.name = NULL;
			reg.content = NULL;

			if( (trt=transform_b1()) != tr_fail )
			{
				transform_b2(trt) ;
			}
			b=b2;
			FLiRT(O_SETCOND,numexec);
			continue;
		}

		if( (b4=b3->next) == NULL) break;

		/* after optim(true) we has in small files the construction
		 * when b3 is not followed by b2 textually so we do this trick:
		 */

		if( (b3i = b1->nextr) != NULL && (b4i = b2->next) != NULL 
		&&  b2->nextr == NULL && b2->nextl == b4i && b2->next != b3i 
		&&  b3i->nextl == b4i && b3i->nextr == NULL 
		&&  b1->nextr == b3i && b1->nextl == b2 && !is_any_label(b2->firstn)
		&&  b2->length <=3 && b3i->length <= 4
		&&  ( bb3.p=changes_only_1_variable(b3i->firstn->forw,&u) ) 
		&&   u == (b3i->length - 1) && bb3.p
		&&  ( bb2.p=changes_only_1_variable(b2->firstn,&u)) 
		&&   u==b2->length && bb2.p
        &&  bb2.p->op2 && bb3.p->op2 && !strcmp(bb2.p->op2,bb3.p->op2)
        &&   (bb2.act=determ_action_type(bb2.p) ) & SET_				
		&&  (bb3.act=determ_action_type(bb3.p) ) & SET_
		&&  b1->lastn->back->sets & CONCODES_AND_CARRY )
		{
			BLOCK *bj,*bi;
			int has_only_1_jmp_on_it=0;
			NODE *pi;
		
			for(bi=b0.next; bi != NULL; bi=bi->next )
			{	
				if( bi->nextl == b3i  || bi->nextr ) 
				{ 
					has_only_1_jmp_on_it++;
				}
				if( bi->next == b3i )
				{
					bj = bi;
				}
			}
			if( has_only_1_jmp_on_it !=1 ) break; /* something unexpected */
			b4 = b4i;
			b3 = b3i;

			/* move b3 after b2 */
			bj->next=b3->next;
			bj->lastn->forw = b3->lastn->forw; 
			if( bj->lastn->forw ) bj->lastn->forw->back = bj->lastn; 
			b2->lastn->forw = b3->firstn;
			b3->firstn->back = b2->lastn;
			b3->lastn->forw = b4->firstn;
			b4->firstn->back = b3->lastn;
			FLiRT(O_SETCOND,numexec);

			/*transform b2,b3 */
			pi=b3->lastn;
			b3->lastn=b3->lastn->back;
			move_node(pi, b2->lastn);
			b2->lastn=pi;
			b2->length ++;
			b3->length --;

			goto case2; 
		} 

   		/*  case -if-then-else-
		 * ->b1-->b2-->b4->
		 *	 |         ^
		 *	 ---->b3---|
		 */
		if( b1->nextr == b3 && b1->nextl == b2 && b2->nextl == b4 
		&&  b2->nextr == NULL &&  b3->nextr == NULL && b3->nextl == b4 
		&&  b2->length <= 4 && b3->length <= 4 && !islabel(b2->firstn) 
		&&  (bb2.p=changes_only_1_variable(b2->firstn,&u), u==(b2->length-1) && bb2.p) 
		&&  (bb3.p=changes_only_1_variable(b3->firstn->forw,&u),u==b3->length && bb3.p) 
		&&  bb2.p->op2 && bb3.p->op2 && !strcmp(bb2.p->op2,bb3.p->op2) 
		&&  (bb2.act=determ_action_type(bb2.p) ) & SET_
		&&  (bb3.act=determ_action_type(bb3.p) ) & SET_ 
		&&  b1->lastn->back->sets & CONCODES_AND_CARRY )
		{
case2:
			bb1.b = b1;
			bb2.b = b2;
			bb3.b = b3;
			bb1.test = b1->lastn->back;
			
			bb1.testlen = OpLength(b1->lastn->back);
			bb2.oplen = OpLength(bb2.p);
			bb3.oplen = OpLength(bb3.p);
			FLiRT(O_SETCOND,numexec);
			
			if( if_then_else2if_then() )
			{
				trt=transform_b1();
				if( trt != tr_fail ) transform_b2(trt) ;
			}
			b=b3;
			continue;
		}
	}
	sets_and_uses();
}		


/* Zero propagation changes instructions that set a register to 0, to use
** a second register, known to hold zero, and make a movl %reg1,%reg2.
** This is done hoping that copy propagation will kill it.
** This function deals with the case that copy propagation didn't kill it:
** we are left with long live ranges and false dependencies. Change it
** back to xorl %reg2,%reg2.
*/
void
clean_zero()
{
	NODE *p;
	int found = 0;
	int i;

#ifdef FLIRT
	static int numexec=0;
#endif

	COND_RETURN("clean_zero");

#ifdef FLIRT
	numexec++;
#endif

	nregs =  suppress_enter_leave ? (NREGS-2) : (NREGS -1);
	for (p = n0.forw; p != &ntail; p = p->forw) {
		if (isuncbr(p) || islabel(p) || p->op == ASMS || is_safe_asm(p))
  			zero_flags = 0; /*no reg is known to hold zero */
		if (set_reg_to_0(p)) {
			Mark_z_val(p->sets & ~CONCODES_AND_CARRY);
			if (!(p->nlive & CONCODES_AND_CARRY) && (OpLength(p) == LoNG)) {
				if (p->op != XORL ) {
					COND_SKIP(continue,"%d ",second_idx,0,0);
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"reduce to xor: ");
						fprinst(p);
					}
#endif
					chgop(p,XORL,"xorl");
					p->op1 = p->op2 = itoreg(p->sets & ~CONCODES_AND_CARRY); 
					p->uses = 0;
					p->sets |= CONCODES_AND_CARRY;
					FLiRT(O_CLEAN_ZERO,numexec);
					found = true;
					p->op3 = NULL;
#ifdef DEBUG
					if (fflag) {
						fprintf(stderr,"became: ");
						fprinst(p);
					}
#endif
				}
			}
		} else { 
			for(i = 0; i < nregs; i++)
				if (p->sets & regmasx[i])
					Mark_non_z(regmasx[i]);
		} /* endif */
	} /* for */
	if (found)
		ldanal();
}/*end clean_zero*/

/*The notion of a block b dominating another block a is used here
**heavily. There might be three different situations:
**1.block a can be reached from the top without going thru block b at all,
**2.block a can only be reached thru block b,
**3.block b separates a from the top, but a can be re-reached in a loop.
**when a regal is set in a block b, there should be no path thru dominated
**blocks (type 2) ending at a non dominated block (type 1) and the regal lives
**in all the nodes in the path, and no path of dominated block ending at a
**semi-dominated block (type 3) and regal lives in the dominated ones and
**is changed in the semi dominated one.
*/
#define DOMIN		3
#define NON_DOMIN	4
#define SEMI_DOMIN	5
#define CURRENT		6

#define ISDOMINATED(B) ((B)->index == DOMIN)
#define IS_NOT_DOMIN(B) ((B)->index == NON_DOMIN)
#define IS_SEMI_DOMIN(B) ((B)->index == SEMI_DOMIN)

static unsigned int masx[] = {EAX,EDX,EBX,ECX,ESI,EDI,EBI,EBP};

/*Is the regal in p changed in the block b?
*/

boolean
changed_at(p,b) NODE *p; BLOCK *b;
{
char *op;
	if (ismem(p->op1)) op = p->op1;
	else if (ismem(p->op2)) op = p->op2;
	else return false;
	for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
		if (msets(p) & MEM && 
			((p->op1 && !strcmp(p->op1,op)) || (p->op2 && !strcmp(p->op2,op)))){
			return true;
		}
	}
	return false;
}/*end changed_at*/

#ifdef DEBUG
extern int cflag;
#endif

static void 
kill_under(a,removed) BLOCK *a,*removed;
{
REF *r;
SWITCH_TBL *sw;
	a->marked = true;
	a->index = NON_DOMIN;
	if (a->nextl && a->nextl != removed && a->nextl->marked == false)
		kill_under(a->nextl,removed);
	if (a->nextr && a->nextr != removed && a->nextr->marked == false)
		kill_under(a->nextr,removed);
	if (is_jmp_ind(a->lastn) && ((sw = get_base_label(a->lastn->op1)) != 0)) {
		for (r = sw->first_ref; r; r = r->nextref) {
			if (sw->switch_table_name == r->switch_table_name) {
				if (r->switch_entry != removed && r->switch_entry->marked == 0)
					kill_under(r->switch_entry,removed);
			}
			if ( r == sw->last_ref )
			break;
		}
	}
}/*end kill_under*/

void
drive_kill_under(removed) BLOCK *removed;
{
BLOCK *a;
	for (a = b0.next; a; a = a->next)
		a->marked = false;
	for (a = b0.next; a; a = a->next) {
		if (a != removed && IS_NOT_DOMIN(a) && a->marked == false)
			kill_under(a,removed);
	}
}/*end drive_kill_under*/

/*
**The block pivot_b, which is the one in which p was found, should be divided
**into two blocks: from firstn to p is one, which is not dominated, and from
**p to lastn is a second one, which is dominated.
*/
boolean
search_under(p,rop,b) NODE *p; char *rop; BLOCK *b;
{
REF *r;
SWITCH_TBL *sw;
BLOCK *b1;
NODE *q;

	b->marked = 1;
#ifdef DEBUG
	if (cflag > 2) {
		fprintf(stderr,"search under block of ");
		fprinst(b->firstn);
	}
#endif
	/*A dominated block in which regal is dead. Dont search under it*/
	if (ISDOMINATED(b) && !live_in_block(p,b))
	{
		return true;
	}
#ifdef DEBUG
	else if (ISDOMINATED(b) && cflag > 2) {
		fprintf(stderr,"regal dead at dominated block\n");
	}
#endif
	/*A non dominated block in which regal lives: cant optimize*/
	if (IS_NOT_DOMIN(b)) {
		if (live_at(p,(q=first_non_label(b)))
			|| (q->op1 && !strcmp(q->op1,rop))
			|| (q->op2 && !strcmp(q->op2,rop))) {
#ifdef DEBUG
			if (cflag > 2) {
				fprintf(stderr,"under non domine ");
				fprinst(b->firstn);
			}
#endif
			return false;
		} else if (isfp(q) && OpLength(q) == DoBL && (q->uses & frame_reg)) {
			int n; int m;
			m = (int)strtoul(rop,(char **)NULL,0);
			if (scanreg(q->op1,0) & frame_reg) {
				n = (int)strtoul(q->op1,(char **)NULL,0);
			} else n = (int)strtoul(q->op2,(char **)NULL,0);
			if (n + 4 == m)
				return false;
		}
#ifdef DEBUG
		else {
			if (cflag > 2) {
				fprintf(stderr,"regal dead at non dominated block\n");
			}
		}
#endif
	}
	if (IS_SEMI_DOMIN(b) && changed_at(p,b))
	{
#ifdef DEBUG
		if (cflag > 2) {
			fprintf(stderr,"under semi domine ");
			fprinst(b->firstn);
		}
#endif
		return false;
	}
#ifdef DEBUG
	else if (IS_SEMI_DOMIN(b) && cflag > 2) {
		fprintf(stderr,"regal not changed at semi dominated block\n");
	}
#endif
	
	/*Go down the flow graph, continue searching*/
	if (b->nextl && b->nextl->marked == 0 && !search_under(p,rop,b->nextl))
		return false;
	if (b->nextr && b->nextr->marked == 0 && !search_under(p,rop,b->nextr))
		return false;
	if (is_jmp_ind(b->lastn)) {
		sw = get_base_label(b->lastn->op1);
		for (r = sw->first_ref; r; r = r->nextref) {
			if (sw->switch_table_name == r->switch_table_name) {
				b1 = r->switch_entry;
				if (b1->marked == 0 && !search_under(p,rop,b1))
					return false;
			}
		}
	}
	return true;
}/*end search_under*/

boolean
live_in_block(NODE *live, BLOCK *block)
{
NODE *p;
	for (p = block->firstn; p != block->lastn->forw; p = p->forw) {
		if (live_at(live,p)) {
			return true;
		}
	}
	return false;
}

void
clean_dead_regal_insts()
{
	NODE *p;
	AUTO_REG *reg;

	for (ALLN(p)) {
		reg = getregal(p);
		if (   reg
		    && !(reg->bits & p->nrlive)
		    && (msets(p) & MEM)
		    && p->op == MOVL) {
			COND_SKIP(break,"%d ",second_idx,0,0);
			DELNODE(p);
		}
	}
	ldanal();
	indegree(true);
	regal_ldanal();
} /* clean_dead_regal_insts */

void
rm_all_tvrs()
{
  BLOCK *a,*b;
  BLOCK *start_b;
  NODE *p;
  AUTO_REG *regal;
  unsigned int prefed_reg,replace_reg,reg;
  char *regal_operand;
  boolean replace;
  unsigned int live_regs, all_regs;
  extern int suppress_enter_leave, fp_removed;
  NODE *first,*q;
  const int first_reg = 0, last_reg = 7;
  char *reg_name,*reg_b_name,*reg_w_name,*reg_p_name;
  int m;
  char regal_star_op[NEWSIZE+1];
  boolean changed;
  extern int number_of_blocks;
#ifdef FLIRT
  static int numexec=0;
#endif

	COND_RETURN("rm_all_tvrs");

#ifdef FLIRT
	numexec++;
#endif

	/* This optimization relies on flow graph connectivity. The treatment
	** for switch statements relies on the indirect jump to be of the
	** form jmp *.LXXX(,%eax,4) but with PIC code it is jmp *%eax. So
	** with both switch and pic we give up.
	*/
	if (   pic_flag
	    && swflag) {
		return;
	}
	if (number_of_blocks >= 2000) {
		return;
	}

#ifdef DEBUG
	if (cflag > 1) {
		fprintf(stderr,"NEW ONE\n");
	}
#endif

	ldanal();
	indegree(true);
	regal_ldanal();
	all_regs = EAX|EDX|ECX|EBX|ESI|EDI;

	if (   !suppress_enter_leave
	    && !fp_removed
	    && !fixed_stack) {
		all_regs |= EBI;
	}
	/* register ebi does not exist in fixed stack */
	if (fixed_stack) {
		/* ebp is a regular reg in fixed_stack */
		all_regs |= EBP;
	}
	if (fp_removed) {
		all_regs |= EBP;
	}

	clean_dead_regal_insts(); 

	for (b = b0.next; b; b = b->next) {
#ifdef DEBUG
		if (cflag > 1) {
			fprintf(stderr,"block ");
			fprinst(b->firstn); 
		}
#endif
		for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
			/* Dont mess with call instructions; a push does not
			** set a regal even if it has it as an operand and
			** sets memory!
			*/
			changed = false;
			if (!fixed_stack) {
				if (   p->op == CALL
				    || p->op == LCALL
				    || p->op == PUSHL
				    || p->op == PUSHW) {

					continue;
				}
			} else {
				if (   p->op == CALL
				    || p->op == LCALL
				    || IS_FIX_PUSH(p) ) {

					continue;
				}
			}
			if (   (regal = getregal(p)) != NULL
			    && regal->size != TEN
			    && (msets(p) & MEM)
			    && !(muses(p) & MEM)) {

				COND_SKIP(continue,"%d %s %s ",second_idx,p->opcode,p->op2);
#ifdef DEBUG
				if (cflag > 1) {
					fprintf(stderr,"work on ");
					fprinst(p); 
				}
#endif
				if (ismem(p->op1)) {
					regal_operand = p->op1;

				} else if (ismem(p->op2)) {
					regal_operand = p->op2;

				} else if (ismem(p->op3)) {
					regal_operand = p->op3;

				} else {
					/* implicit set */
					continue;
				}
				sprintf(regal_star_op,"%c%s",'*',regal_operand);
				replace = true;
				if (   ! p->sets
				    && !live_at(p,p)) {

					if (!isfp(p)) {
						DELNODE(p);
						FLiRT(O_RM_ALL_TVRS1,numexec);
#ifdef DEBUG
						if (cflag > 1) {
							fprintf(stderr,"delete dead instruction ");
							fprinst(p); 
						}
#endif
						continue;
					} else {
						if (   p->op == FSTPL
						    || p->op == FSTPS) {

							chgop(p,FSTP,"fstp");
							FLiRT(O_RM_ALL_TVRS2,numexec);
							p->op1 = "%st(0)";
							continue;

						} else if (   p->op == FSTL
						           || p->op == FSTS) {
							DELNODE(p);
							FLiRT(O_RM_ALL_TVRS3,numexec);
#ifdef DEBUG
							if (cflag > 1) {
								fprintf(stderr,"delete dead instruction ");
								fprinst(p); 
							}
#endif
							continue;
						} else {
							continue;
						}
					}
				} /* end case of dead instruction */
				if (isfp(p)) {
					continue;
				}

				/* if regal lives at the entry to b, don't
				   optimize */
				if (   p != b->firstn
				    && needed_at(p,b)) {

					replace = false;
#ifdef DEBUG
					if (cflag > 1) {
						fprintf(stderr,"falsify 0\n");
					}
#endif
					continue;
				}
#ifdef DEBUG
				else if (cflag > 1) {
					fprintf(stderr,"regal is dead at ");
					fprinst(first_non_label(b));
				}
#endif
				/* next is the case where the regal is needed.
				** Try to replace it with an integer register.
				*/
				/* Verify that there is no dominated block 'a'
				** where regal lives, and a non-dominated block
				** b' can be reached from a, and regal lives
				** in b' too.
				** if there is no such combination, then we
				** are ok w.r.t non-dominated blocks.
				*/
				/*last block does not dominate any other block.
				*/
				if (b->next == NULL) {
					for (a = b0.next; a; a = a->next) {
						a->index = NON_DOMIN;
					}
					b->index = CURRENT;
				} else {
					drive_indegree2(b);
					/*mark the dominated blocks*/
					for (a = b0.next; a; a = a->next) {
						if (a->indeg == a->indeg2) {
							a->index = DOMIN;
#ifdef DEBUG
							if (cflag > 2) {
								fprintf(stderr,"1'st calc domin, %d ",a->indeg);
								fprinst(a->firstn);
							}
#endif
						} else {
							a->index = NON_DOMIN;
#ifdef DEBUG
							if (cflag > 2) {
								fprintf(stderr,"1'st calc not domin, %d ",a->indeg);
								fprinst(a->firstn);
							}
#endif
						}
					}
					/* a block reachable from a not
					** dominated block is also not
					** dominated.
					*/
					drive_kill_under(b);
					/* no one dominates the first block,
					** but it's indegs are 0
					*/
					b0.next->index = NON_DOMIN;

					/* for each dominated block find if
					** it's in a loop which does not
					** include b. If it is then it is not
					** really dominated, it's "semi
					** dominated".
					*/
					for (a = b0.next; a; a = a->next) {
						if (ISDOMINATED(a)) {
							drive_indegree3(a,b);
							if (a->indeg2) {
								a->index = SEMI_DOMIN;
							}
						}
					}
					b->index = CURRENT;
				}
#ifdef DEBUG
				if (cflag > 2) {
					fprintf(stderr,"final dominance\n");
					for (a = b0.next; a; a = a->next) {
						if (ISDOMINATED(a)) {
							fprintf(stderr,"dominated, ");
						} else if (IS_SEMI_DOMIN(a)) {
							fprintf(stderr,"semi , ");
						} else if (IS_NOT_DOMIN(a)) {
							fprintf(stderr,"not dom, ");
						} else if (a->index == CURRENT) {
							fprintf(stderr,"current, ");
						} else {
							fprintf(stderr,"unclear domination ");
						}
						fprinst(a->firstn);
					}
				}
#endif
				/* Now only blocks with index != 0 are
				** dominated. */
				/* clear the index field for search_under
				** to use */
				for (a = b0.next; a; a = a->next) {
					a->marked = 0;
				}
				for (a = b0.next; a; a = a->next) {
					if (   a == b
					    || ISDOMINATED(a)) {

						/*all dominated blocks*/
						if (   a == b
						    || live_in_block(p,a)) {

							if (!search_under(p,regal_operand,a)) {
								replace = false;
#ifdef DEBUG
								if (cflag > 1) {
									fprintf(stderr,"falsify 1\n");
								}
#endif
								/*not need to search under the rest*/
								break;
							}
						}
					}
				}
				if (replace) {
					/* collect the regs that are free
					** whenever regal lives */
					live_regs = all_regs;
					if (OpLength(p) == ByTE) {
					     live_regs &= ~(ESI|EDI|EBI|EBP);
					}
					if (   p->op == CALL
					    || p->op == LCALL) {

						live_regs &= ~(EAX|EDX|ECX);
					}
					start_b = (b == b0.next) ? b : b0.next->next;
					for (a = start_b; a; a = a->next) {
						if (  a == b
						    || !IS_NOT_DOMIN(a)) {
#ifdef DEBUG
							if (cflag > 1) {
								fprintf(stderr,"verify under b %d of ",a->indeg);
								fprinst(a->firstn);
							}
#endif
							first = (a == b) ? p->forw : a->firstn;
							for (q = first; q!=a->lastn->forw; q = q->forw) {
								if (   (a == b)
								    || live_at(p,q)
								    || (   q == first_non_label(a)
								        && q->op1
								        && !strcmp(q->op1,regal_operand))) {
#ifdef DEBUG
									if (cflag > 1) {

										unsigned new_live_regs;
										new_live_regs = live_regs
											& ~(q->nlive|q->uses|q->sets);
										if (live_regs != new_live_regs) {
											for (reg = 0;
											     reg < last_reg;
											     reg++) {
												if (   live_regs & masx[reg]
												    && masx[reg] &~new_live_regs) {

													fprintf(stderr,"kill %s ",
													itoreg(masx[reg]));
													fprinst(q);
												}
											}
										}
									}
#endif
									live_regs &= ~(q->nlive|q->uses|q->sets);
								}
								if (   isfp(q)
								    && regal->bits & get_regal_bits(q)){

									live_regs = 0;
#ifdef DEBUG
									if (cflag > 1) {
										fprintf(stderr,"kill it by ");
										fprinst(q); 
										fprintf(stderr,"kill all regs\n");
									}
#endif
									break;
								}
								if (   (   q->op1
								        && !strcmp(q->op1,regal_operand))
								    || (   q->op2
								        && !strcmp(q->op2,regal_operand))) {

									if (OpLength(q) == ByTE) {
										live_regs &= ~(ESI|EDI|EBI|EBP);
									}
									if (   q->op == CALL
									    || q->op == LCALL) {

										if (live_at(p,q)) {
											live_regs &= ~(EAX|EDX|ECX);
										}
									}
								}
							}
						}
					} /* for all dominated blocks */
					if (!live_regs) {
#ifdef DEBUG
						if (cflag > 1) {
							fprintf(stderr,"no live regs, continue\n");
						}
#endif
						continue;
					}
					prefed_reg = p->uses;
					if (!fp_removed) {
						prefed_reg &= ~frame_reg;
					}
					if (prefed_reg == 0) {
						prefed_reg = all_regs;
					}
					replace_reg = 0;
					for (reg = first_reg; reg <= last_reg; reg++) {
						if ((prefed_reg&live_regs&masx[reg]) == masx[reg]) {
							replace_reg = masx[reg];
							break;
						}
					}
					if (!replace_reg) {
						for (reg = first_reg; reg <= last_reg; reg++) {
							if ((live_regs&masx[reg]) == masx[reg]) {
								replace_reg = masx[reg];
								break;
							}
						}
					}
					if (!replace_reg) {
#ifdef DEBUG
						if (cflag > 1) {
							fprintf(stderr,"no reg instead\n");
						}
#endif
						continue;
					}
					reg_name = itoreg(replace_reg);
					reg_b_name = itoreg(L2B(replace_reg));
					reg_w_name = itoreg(L2W(replace_reg));
					reg_p_name = itostarreg(replace_reg);
#ifdef DEBUG
					if (cflag > 1) {
						fprintf(stderr,"on account of ");
						fprinst(p); 
					}
#endif
					changed = true;
					FLiRT(O_RM_ALL_TVRS4,numexec);
					/* found a reg, do the replace */
					for (a = b0.next; a; a = a->next) {
						if (   a == b
						    || (   !IS_NOT_DOMIN(a)
						        && needed_at(p,a))) {
#ifdef DEBUG
							if (cflag > 1) {
								fprintf(stderr,"replace under ");
								fprinst(a->firstn);
							}
#endif
							q = (a == b) ? p : a->firstn;
							for ( ;q!=a->lastn->forw; q = q->forw) {
								for (m = 1; m <= 3; m++) {
									if (   q->ops[m]
									    && !strcmp(q->ops[m],regal_operand)) {
#ifdef DEBUG
										if (cflag > 1) {
											fprintf(stderr,"from ");
											fprinst(q); 
										}
#endif
										switch (OpLength(q)) {
											case ByTE:
												q->ops[m] = reg_b_name;
												break;

											case WoRD:
												q->ops[m] = reg_w_name;
												break;

											default:
												q->ops[m] = reg_name;
												break;

										}
#ifdef DEBUG
										if (cflag > 1) {
											fprintf(stderr,"to ");
											fprinst(q); 
										}
#endif
										new_sets_uses(q);
									}
									if (   q->ops[m]
									    && !strcmp(q->ops[m],regal_star_op)) {
											q->ops[m] = reg_p_name;
											new_sets_uses(q);
									}
								}
							}
						}
#ifdef DEBUG
						else if (cflag > 1) {
							fprintf(stderr,"do not replace under ");
							fprinst(a->firstn);
						}
#endif
					}
				}
			} /* end case of getregal */
			if (changed) {
				ldanal();
				indegree(true);
#ifdef DEBUG
				if (cflag > 1) {
					BLOCK *c;
					fprintf(stderr,"corrected indegrees\n");
					for (c = b0.next; c; c = c->next) {
						fprintf(stderr,"indegree %d at ",c->indeg);
						fprinst(c->firstn);
					}
				}
#endif
				regal_ldanal();
				b = block_of(p);
			}
		} /* for p in b */
	} /* for all blocks */
} /* end rm_all_tvrs */


/* ### register tracing  */
/* This next piece of code performs register-tracing. The purpose of this 
 * tracing is to accumulate information about the register contents at any
 * stage of the program. trace_body() is the driver of the tracing , and
 * trace_optim() is the driver of trace-based optimizations.
 */

#define CLEAR		0
#define PROCESSED	1
#define  block_scanned_partitionaly 2

#define UNKOWN  0
#define KNOWN   1

#define NOTHING  0
#define REGISTER 1
#define ADDRESS  2
#define MEMORY   3
#define NUMBER   4
#define EVALUATED 5

#define RGS (EAX|ECX|EDX|EBX|EDI|ESI|EBP|EBI) 


/* altered_by_const() tries to calculate the new value settled in a register 
 * after instruction p, using old value located in reg before p, and 
 * returns true if it's possible and false otherwise.
 * Restriction:
 *		Doesn't accomplish stack evalution (%esp)
 * Assumption:
 *		Only a single reg is set by a single instruction ;
 */
boolean altered_by_const(p,val)
NODE *p; 
int	*val; /* value in reg before p instr. */
{
	int newval=0;
	char *s;
	unsigned reg=p->sets & (~CONCODES_AND_CARRY) & (~MEM) & (~ESP) ;
	unsigned op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;
	
	if( !reg ) return false; 
	/* No reg is set */

	if( op==INCL || op==INCW ||	op==INCB ) 
	{
		(*val)++;
		return true;
	}
	if( op==DECL || op==DECW ||	op==DECB ) 
	{
		(*val)--;
		return true;
	}
	if( isimmed(p->op1) 
	&& (op==ADDL || op==ADDW ||	op==ADDB ) )
	{
		(*val)+=(int)strtoul(p->op1+1,(char **)NULL,0);
		return true;
	}
	if( isimmed(p->op1)
	&& (op==SUBL || op==SUBW ||	op==SUBB ) )
	{
		(*val)-=(int)strtoul(p->op1+1,(char **)NULL,0);
		return true;
	}
	if( isimmed(p->op1) 
	&& (op == ANDL || op == ANDW || op == ANDB ))
	{
		(*val) &= (int)strtoul(p->op1+1,(char **)NULL,0);
		return true;
	}
	if( isimmed(p->op1) 
	&& (op == ORL || op ==  ORW || op ==  ORB ) )
	{
		(*val) |= (int)strtoul(p->op1+1,(char **)NULL,0);
		return true;
	}
	if( op == NEGL || op == NEGW || op == NEGB )
	{
		*val = -*val;
		return true	;
	}
	if( isimmed(p->op1) 
	&& (op == SHRL || op == SHRW || op == SHRB ) )
	{
		(*val) >>= (int)strtoul(p->op1+1,(char **)NULL,0);
		return true;
	}
	if( isimmed(p->op1) 
	&& (op == SARL || op == SARW || op == SARB ) )
	{
		int i=(int)strtoul(p->op1+1,(char **)NULL,0);
		while(i--) (*val) /= 2 ;
		return true;
	}
	if( isimmed(p->op1) 
	&& (op == SHLL || op == SHLW || op == SHLB ) )
	{
		(*val) <<= (int)strtoul(p->op1+1,(char **)NULL,0);
		return true;
	}
	if( isimmed(p->op1) 
	&& (op == SALL || op == SALW || op == SALB ) )
	{
		int i=(int)strtoul(p->op1+1,(char **)NULL,0);
		while(i--) (*val) *= 2 ;
		return true;
	}
	if( isimmed(p->op1) && p->op3 ==NULL 
	&& (op == IMULL || op == IMULW || op == IMULB ) )
	{
		(*val) *= (int)strtoul(p->op1+1,(char **)NULL,0);
		return true;
	}
	if( op==LEAL && p->uses == reg  
	&&( p->op1[0] =='(' || isdigit(p->op1[0]) 
	||  p->op1[0] =='-' && isdigit(p->op1[1]) ) )
	{
		newval = (int)strtoul(p->op1,(char **)NULL,0);
		if(s=strchr(p->op1,'('))
		{
			newval+=*val;
			if(s=strchr(s,','))
			{
				if(s=strchr(s,','))
				  newval += (*val) * (int)strtoul(s,(char **)NULL,0);
			}
		}	
		*val=newval;
		return true;
	}			
	return false;
}
	

/* array of regs */
static unsigned rgs[8]= { EAX, ECX, EDX, EBX, EDI, ESI, EBP, EBI };
/* indices for registers' array */
enum { iEAX=0,iECX=1,iEDX=2,iEBX=3,iEDI=4,iESI=5,iEBP=6,iEBI=7,iEXX=8 };

static boolean do_count_const=0;
/* to avoid count const in loops */

int 
i_reg2ind(rg) unsigned rg;
{
	switch (rg) {
		case EAX : return iEAX;       	
		case ECX : return iECX;       	
		case EDX : return iEDX;	
		case EBX : return iEBX; 	
		case EDI : return iEDI; 	
		case ESI : return iESI; 	
		case EBP : return iEBP;
		case EBI : return iEBI;
		default  : return iEXX;
	}
} 	

/* This function changes the tracing information resulting from the previous
** node, according to the current instruction.
*/
static void
trace_node(tp,p)  
trace_type *tp /* previous node */ ;
NODE *p  /* node for tracing */;
{
  unsigned ind,sreg,freg;
  int i,n;
  boolean destroyed[8] = { 0,0,0,0,0,0,0,0 };
  int value_in_reg;

/* copy contents of predecessor to trace[] */
	for (i = 0; i < 8; i++) {
		p->trace[i] = tp[i];
	}
	if ((sreg = p->sets & RGS) == 0) return;

	freg = full_reg(sreg);
	ind = i_reg2ind(freg);
	if (ind == iEXX) {
		for(i=iEAX; i<iEXX; i++) {
			if (sreg & rgs[i]) destroyed[i]=true; 
		} 
	} else {
		/* load into the reg new value e.g  movl $.L0,%eax   */
		if (p->op == MOVL || p->op == MOVW || p->op == MOVB ){
		 	if (isconst(p->op1)) {
		 	/* setting the reg by const */ 
				p->trace[ind].kind = isimmed(p->op1) ? NUMBER : ADDRESS ;
				p->trace[ind].value = p->op1;
				p->trace[ind].reg = sreg; 
				p->trace[ind].tag = KNOWN;
				return;
			}
			if (   isreg(p->op1)
			    && (n = full_reg(setreg(p->op1))) & RGS) {
			/* setting the reg by const from another reg */
				n = i_reg2ind(n);
			/* change setreg(p->op1) ==> p->uses */
				if( p->trace[n].tag == KNOWN ){ 
					p->trace[ind].kind = p->trace[n].kind;
					p->trace[ind].value = p->trace[n].value;
					p->trace[ind].reg = sreg; /*!*/ 
					p->trace[ind].tag = KNOWN;
					return;
	 			}
				else	/*  movl %r1,%r2 */
				{
					p->trace[ind].kind = REGISTER;
					p->trace[ind].value= p->op1;
					p->trace[ind].reg  = sreg;
					p->trace[ind].tag  = KNOWN;
					return;
				}	
			} else if( ismem(p->op1) ) /*  movl MEM,%r  */ {
				p->trace[ind].kind = MEMORY;
				p->trace[ind].value= p->op1;
				p->trace[ind].reg  = sreg;
				p->trace[ind].tag  = KNOWN;
				return;
			}
		}		
		/* load $0 to reg e.g.  xorl %eax,%eax */
		if (set_reg_to_0(p)) {
			p->trace[ind].reg = sreg;
			p->trace[ind].tag = KNOWN;
			p->trace[ind].kind = NUMBER;
			p->trace[ind].value = "$0";
			return;
		}	
		/* load into the reg new value e.g  
	   	   leal	.L0+4,%eax   */
		if (p->op == LEAL && !p->uses) {
			p->trace[ind].reg = sreg;
			p->trace[ind].tag = KNOWN;
			p->trace[ind].kind = ADDRESS;
			p->trace[ind].value = p->op1;
			return;
		}	

/* calculation within loops is forbidden */

		if (do_count_const) {
			/* Try to count if possible new value in the reg produced by 
			this instruction */
			char *sv=NULL;
			unsigned r1;
		
			if (   p->op1
			    && p->op2
			    && isreg(p->op1)
			    && (r1=setreg(p->op1)) == (p->uses & (~CONCODES_AND_CARRY))  
			    &&  !(r1 & freg)) {

				int idx = i_reg2ind( full_reg(r1) );

				if (idx == iEXX)
					goto trace_node_lab1;
				if (   p->trace[idx].tag == KNOWN
				    && p->trace[idx].kind == NUMBER) {
					sv = p->op1;
					p->op1 = p->trace[idx].value;
				}
			}
			if (   p->op1
			    && p->op2
			    && isimmed(p->op1)
			    && p->trace[ind].tag == KNOWN
			    && p->trace[ind].kind == NUMBER
			    && p->trace[ind].reg
			    && p->sets) {
				value_in_reg = (int)strtoul(p->trace[ind].value,(char **)NULL,0);
				if (altered_by_const(p, &value_in_reg)) {
					p->trace[ind].value = getspace(ADDLSIZE);
					sprintf(p->trace[ind].value,"$%d", value_in_reg);
					p->trace[ind].kind = EVALUATED;
					return;
				}
			}
			if (sv != NULL) p->op1 = sv;
		} /* end count */ 

trace_node_lab1:
		
		/* Otherwise, assume contents of reg has been destroyed */
		/* Enforce agressiveness here */
		destroyed[ind] = true;
	}
	/* destroy trace[reg] if the reg was destroyed by the instruction .
	 */	
	for (i=iEAX; i<iEXX; i++) { 
		if (destroyed[i]) 
			p->trace[i].tag = UNKOWN ;
	}			
	return;
} /* end trace_node */

/* This function produces a basic block tracing, using as initial information 
 * the tracing information of pf node.
 */					
static void
trace_block(tf,b)  trace_type *tf; BLOCK *b;
{
  extern SWITCH_TBL* get_base_label();
  int i, do_pass = 0 ;
  NODE *p;
  trace_type *ti;
  REF * r;
  SWITCH_TBL *sw;

	/*pi = b->node0;*/
	ti = &(b->trace0[0]);
  	if (b->marked == CLEAR) {
	/*  First  visit to  the block */ 
		do_count_const = true ;
		b->marked =  PROCESSED; 
		/*pi = b->node0 = Saveop(0,"",0,GHOST); */
		/* set first node to UNKOWN */
		for (i=iEAX; i<iEXX; i++) ti[i] = tf[i];				
	} else {
		/* Additional visit to the block */ 
		/* If the number of defined registers decreased , process the block 
 		* again. Otherwise, return.    
 		*/  
		do_count_const = false;
		 
 		for (i = iEAX; i < iEXX; i++) {
	 		if (tf[i].tag != ti[i].tag) {
				/* do we have to change first node tag ? */
				if (ti[i].tag == KNOWN ) { 
					ti[i].tag = UNKOWN;
					do_pass = 1;
				}	
	 			continue;
			}
			/* Both are undefined */
			if (tf[i].tag == UNKOWN) continue;
 				
			/* Do both regs contain different values ? */
	 		if (strcmp(tf[i].value,ti[i].value)) { 
				ti[i].tag = UNKOWN;
				do_pass = 1;
				continue;
			}	

			/* Now they have the same value , maybe in different parts of
			 * the full reg 
			 */
 			if (tf[i].reg != ti[i].reg) { 
				ti[i].tag = UNKOWN;
				do_pass = 1;
			}	
		}   

		if (do_pass == 0) {
			/* nothing to do */
			return;  
		}
	}

	/* trace first node */
	for (p = b->firstn; is_any_label(p); p = p->forw) {
		for (i = 0; i < iEXX; i++) p->trace[i].tag = UNKOWN ;
	}
	trace_node(ti,p);
	
	/* trace all body nodes */
	for( ; p != b->lastn; p = p->forw)
		trace_node(p->trace,p->forw);
		
	/* trace last node */
	if (b->nextl) trace_block(p->trace,b->nextl);
	if (b->nextr) trace_block(p->trace,b->nextr);

	/* This section serves for switches */	  
	if(b->nextr == NULL && (b->nextl == NULL 
	||(is_any_br(b->lastn) && !is_any_uncbr(b->lastn))) ){
		if (is_jmp_ind(b->lastn) 
		&& (sw = get_base_label(b->lastn->op1)) != NULL ) {
			for (r = sw->first_ref; r; r = r->nextref) {
				if (sw->switch_table_name == r->switch_table_name)
					trace_block(p->trace,r->switch_entry);
				if (r == sw->last_ref) break;
			}
		}				 
		/*else  p->op == ret */  
	}		
} /* end trace_block */

/* Driver performes tracing.
** It works on  basic blocks with calls when tracing is performed for 
** optimizations, and on basic blocks without calls for scheduling ( because 
** of memory disumbiguation ) 
*/
void 
trace_body(do_bldgr)
boolean do_bldgr ;
{
  BLOCK *b;

  trace_type null_trace[8];

	memset(&null_trace,0,sizeof(null_trace));
	zero_flags = 0;

	if (do_bldgr) {
		bldgr(false);
		set_refs_to_blocks(); 
	}
	for (b = b0.next; b ; b = b->next) b->marked = CLEAR;
	
	for (b = b0.next; b ; b = b->next)
		if (b->marked == CLEAR) trace_block(null_trace,b);
}

/* Next section contains functions performing tracing based optimizations.
 * trace_optim() is the driver of the section.
 */

static boolean find_prev_set; 
/* If set , node setting value into the reg is found */ 

/* It sereves for loops to do an extra visit to the first block in the chain
 * otherwithe section of block laing bellow the move-node is not scanned.
 */
  
/* reg_not_changed_after_set(b,p,r1,r2) is used by rm_mvXY_mvYX() */
boolean 
reg_not_changed_after_set(b,p,r1,r2)
BLOCK *b;
NODE  *p;
unsigned r1,r2;
{
  BLOCK *bi;
  unsigned f1,f2;

	if (b->marked == PROCESSED ) return true; 
	if (b->marked == block_scanned_partitionaly ) 
		b->marked = CLEAR;
	else
		b->marked = PROCESSED;
	if (p == NULL) goto DoNextBlock;
	while (p && islabel(p)) p = p->back;
	if (!p) 
		fatal(__FILE__,__LINE__,"trace_optim(): reg isn't set");

	for(; p != b->firstn->back; p = p->back) {
		f1 = f2 = 0;

		/* If either register is set by this instruction then we are
		   done, one way or the other!. */
		if (   (f1=(p->sets & r1)) != NULL
		    || (f2=(p->sets & r2)) != NULL) {
			if (f2) return false;
			if (   f1
			    && is_move(p)
			    && setreg(p->op1) == r2) { 
				find_prev_set |= true;
				return true;
			}	
			return false;
		}		
	}
DoNextBlock:
	for (bi = b0.next; bi != NULL; bi = bi->next) {
		if (bi->nextl == b || bi->nextr == b)
			if (!reg_not_changed_after_set(bi,bi->lastn,r1,r2))
				return false;
	}

	/* NOTE: The logic to walk backward through switch/cases is missing. */
	return true;
}

/* mem_not_changed_after_set(b,p,p0,r) is used by rm_mvMEMX_mvMEMX() 
** we need to  attach  temporarily p0 to the bottom of the bb for use 
** bb oriented seems_same() .
*/
boolean 
mem_not_changed_after_set(b,p,p0,r)
BLOCK *b;
NODE  *p,*p0;
unsigned r;
{
  BLOCK *bi;
  unsigned regs = scanreg(p0->op1,false);  /* regs used to addres memory. */

	if (b->marked == PROCESSED) return true; 
	if (b->marked == block_scanned_partitionaly) 
		b->marked = CLEAR;
	else
		b->marked = PROCESSED;
	if (p == NULL) goto DoNextBlock;
	p0->idus = indexing(p0);
	for ( ; p != b->firstn->back; p = p->back) {
		if (islabel(p)) continue;
		/*If call, always treat as false - potential modification. */
		if (   p->op == CALL
		    || p->op == LCALL) return false;

		/* If any of the regs used in the memory operand are modified,
		   then not talking about the same memory. */
		if (p->sets & regs) return false;

		p->idus = indexing(p);
		if (   (p->sets & r)    /* this should be a "movX  MEM,%r" */
		    || (  ismem(dst(p))
		        && seems_same(p,p0,0))  /* potentially modifying this memory? */ ) {

			if (   is_move(p)
			    && !strcmp(p->op1,p0->op1))  /* is the "movX MEM,%r" */ {
				find_prev_set = true;
				return true;
			}
			return false;
		} 
	}
DoNextBlock:
	for (bi = b0.next; bi != NULL; bi = bi->next) {
		if (bi->nextl == b || bi->nextr == b)
			if (!mem_not_changed_after_set(bi,bi->lastn,p0,r))
				return false;
	}

	/* NOTE: The logic to walk backward through switch/cases is missing. */
	return true;
}		

/*  Idea for rm_movXY_movYX() is taken from 022.li (0.5%)
 *  bb1( movl %eax,%edi ) -> bb3( movl %edi,%eax )
 *  bb2( movl %eax,%edi ) --/
 *  if %eax and %edi are not changed these moves are redundant. Here we delete 
 *  move in the bb3 , two previous moves will be detlete by rmrdmv().
 *  If the mission was succesfull, the  function returns true .
 */
boolean
rm_mvXY_mvYX()
{
  NODE *p;
  trace_type *t1;
  NODE *p1;
  BLOCK *b,*b1;
  unsigned ind,freg;
  boolean done=false;
#ifdef FLIRT
  static int numexec=0;
#endif
	
	COND_RETURNF("rm_mvXY_mvYX");
#ifdef FLIRT
	numexec++;
#endif

	for (b = b0.next; b != NULL; b = b->next) { 
		for(p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (   is_move(p)
			    && isreg(p->op1) && isreg(p->op2)) { 
				if (   p == b->firstn
				    || islabel(p->back)) {
					t1 = &(b->trace0[0]);
					p1 = NULL;
				} else {
					t1 = &(p->back->trace[0]);
					p1 = p->back;
				}
				freg = full_reg(p->uses);
				if ((ind = i_reg2ind(freg)) == iEXX) continue;
				if (   t1[ind].tag == KNOWN 
				    && t1[ind].kind == REGISTER
				    && t1[ind].reg == p->uses
				    && !strcmp(t1[ind].value,p->op2)) {
					for(b1 = b0.next; b1 != NULL; b1 = b1->next)
						b1->marked = CLEAR;
					find_prev_set = false;
					b->marked = block_scanned_partitionaly ;
					if (   reg_not_changed_after_set(b,p1,p->uses,p->sets) 
					    && find_prev_set) {
						done=true;
						DELNODE(p);
						FLiRT(O_RM_MVXY,numexec);
					}	
				}
			}
		}	
	}
	return done;
}

/*  Idea for rm_mvMEMX_mvMEMX is taken from 085.gcc
**  bb1( movl %eax,MEM ) -> bb3( movl MEM,%eax )
**  bb2( movl %eax,MEM ) --/
**  if %eax and MEM are not changed (checking for volatility of the
**  memory operand) these moves are redundant. Here we delete 
**  move in the bb3 , two previous moves will be deleted by rmrdmv().
**  If the mission was succesfull, the  function returns true .

NOTE: This description seems bogus.... code (in conjunction with
      mem_not_changed_after_set) is only making the following
      transformation:

      movl   MEM,X
      movl   MEM,X ---->   movl X,X
*/
boolean
rm_mvMEMX_mvMEMX()
{
  NODE *p,*p1;
  BLOCK *b,*b1;
  unsigned ind;
  boolean done = false;
  trace_type *t1;
#ifdef FLIRT
  static int numexec=0;
#endif
			
	COND_RETURNF("rm_mvMEMX_mvMEMX");
#ifdef FLIRT
	numexec++;
#endif

	for(b = b0.next; b != NULL; b = b->next) { 
		for(p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (   is_move(p)
			    && ismem(p->op1) && ! is_vol_opnd(p,1)
			    && isreg(p->op2) 
			    && !(scanreg(p->op1,false) & p->sets)) { 

				if (   p == b->firstn
				    || islabel(p->back))  {
					p1 = NULL;
					t1 = &(b->trace0[0]);
				} else {
					p1 = p->back;
					t1 = &(p->back->trace[0]);
				}
				for (ind=iEAX; ind < iEXX; ind++) {
					if (   t1[ind].tag == KNOWN 
					    && t1[ind].kind == MEMORY
					    && sizeofreg(t1[ind].reg) == sizeofreg(p->sets)
					    && !strcmp(t1[ind].value,p->op1)) {

						for (b1 = b0.next; b1 != NULL; b1 = b1->next)
							b1->marked = CLEAR;
						find_prev_set = false;
						b->marked = block_scanned_partitionaly ;
					
						if (   mem_not_changed_after_set(b,p1,p,t1[ind].reg)
						    && find_prev_set) {

							COND_SKIP(continue,"%d %s\n",second_idx,p->op1,0);
							FLiRT(O_RM_MVMEM,numexec);
							done=true;
							p->uses =  t1[ind].reg;  
							p->nlive |= t1[ind].reg;
		   					p->op1 =  itoreg(t1[ind].reg) ;
							break; 
						}	
					}
				}
			}	
		}	
	}
	return done;
}
			
/* If while instr movl $const,%r ,reg contains $const and reg not changed
 * we can deleet this move 
 */  
rm_rd_const_load()
{
NODE *p;
BLOCK *b;
unsigned ind,freg;
boolean done = false;
trace_type *t1;
#ifdef FLIRT
static int numexec=0;
#endif
			
COND_RETURNF("rm_rd_const_load");
#ifdef FLIRT
	numexec++;
#endif

	for (b = b0.next; b != NULL; b = b->next) { 
		for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if ((is_move(p) && isconst(p->op1) || p->op == LEAL && !p->uses)  
				&& isreg(p->op2)) { 
				if (p == b->firstn || islabel(p->back))
					t1 = &(b->trace0[0]);
				else
					t1 = &(p->back->trace[0]);
				freg = full_reg(p->sets);
				if ((ind = i_reg2ind(freg)) == iEXX) continue;
				if (t1[ind].tag == KNOWN 
					&& (t1[ind].kind == NUMBER 
						|| t1[ind].kind == ADDRESS)
					&&  t1[ind].reg == p->sets
					&& !strcmp(t1[ind].value,p->op1)) {
					done=true;
					DELNODE(p);
					FLiRT(O_RM_RD_CONST_LOAD,numexec);	
				}
			}
		}	
	}
	return done;
}

boolean
rm_rd_test()
{
NODE *p;
BLOCK *b;
unsigned ind;
boolean ret_val = false;
trace_type *t1;
	COND_RETURNF("rm_rd_test");
	for(b = b0.next; b != NULL; b = b->next) { 
		for(p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (p->op == TESTL && !strcmp(p->op1,p->op2)) {
				if (p == b->firstn || islabel(p->back)) t1 = &(b->trace0[0]);
				else t1 = &(p->back->trace[0]);
				ind = i_reg2ind(p->uses);
				if (t1[ind].tag == KNOWN
					&& t1[ind].kind == NUMBER
					&& t1[ind].reg == p->uses
					&& strcmp(t1[ind].value,"$0") != 0) {
					if (p->forw->op == JE) {
						DELNODE(p);
						DELNODE(p->forw);
						ret_val = true;
					}
				}
			}
		}
	}
	return ret_val;
}

boolean
rm_rd_cmpl()
{
NODE *p;
BLOCK *b;
unsigned ind;
boolean ret_val = false;
trace_type *t1;
	COND_RETURNF("rm_rd_cmpl");
	for(b = b0.next; b != NULL; b = b->next) { 
		for(p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (p->op == CMPL && isconst(p->op1) && isreg(p->op2)) {
				if (p == b->firstn || islabel(p->back)) t1 = &(b->trace0[0]);
				else t1 = &(p->back->trace[0]);
				ind = i_reg2ind(p->uses);
				if (t1[ind].tag == KNOWN
					&& t1[ind].kind == NUMBER
					&& t1[ind].reg == p->uses) {
					char *temp = p->op2;
					p->op2 = t1[ind].value;
					/*cmp_imm_imm returns true if it was able to remove the
					**cmp-jcc, or in case of cmp $addr,$const, it reverses
					**the order of the operands. Here we need to find out
					**that this is what is happening and reverse it back.
					*/
					if (!cmp_imm_imm(p,p->forw)) p->op2 = temp;
					else {
						if (p->back->forw == p) { /*cmp was not removed*/
							p->op1 = p->op2;
							p->op2 = temp;
							invbr(p->forw);
						} else {
							ret_val = true;
						}
					}
				}
			}
		}
	}
	return ret_val;
}

void
trace_optim()
{
	boolean do_ldanal;	

	COND_RETURN("trace_optim");
	if( pic_flag && swflag || asmflag ) return;
	trace_body(true) ;

	do_ldanal  = rm_mvXY_mvYX();
	do_ldanal |= rm_mvMEMX_mvMEMX();
	do_ldanal |= rm_rd_const_load();
	do_ldanal |= rm_rd_test();
	do_ldanal |= rm_rd_cmpl();
	
	if (do_ldanal)  ldanal();
}		

/**********************
 * trace partial stalls
 **********************/

boolean cleanse_reg();

#define regs8h ( AH|BH|CH|DH )
#define regs8l ( AL|BL|CL|DL )
#define regs8  ( regs8l | regs8h )
#define regs16 ( Ax|Bx|Cx|Dx|DI|SI|BP )
#define regs32 ( Eax|Ebx|Ecx|Edx|Edi|Esi|Ebp )
#define scratch_rgs ( EAX|ECX|EDX )
#define is_regs8h(r) ((r) & regs8h)
#define is_regs8l(r) ((r) & regs8l)
#define is_regs8(r)  ((r) & (regs8l | regs8h) )
#define is_regs16(r) ((r) & regs16)
#define is_regs32(r) ((r) & regs32)

spy_type null_spy = { 0 };
NODE null_spy_node = { 0 };

#define n_ps_regs 8
unsigned ps_sregs[n_ps_regs]; /* ps uses max 3 regs */
unsigned ps_uregs[n_ps_regs]; /* ps sets max 3 regs */

#define n_spy_regs 7
static const unsigned spy_regs[n_spy_regs]={EAX,EBX,ECX,EDX,EDI,ESI,EBP};

/* it return max possible  reg e.g Eax for EAX */
unsigned
get_max_reg(r) unsigned r;
{
unsigned rr;
    if ((rr = r & regs32) != 0 ) return rr;
    else if ((rr = r & regs16) != 0 ) return rr;
    else if ((rr = r & regs8) != 0 ) return rr;
    else fatal( __FILE__,__LINE__,"get_max_reg: no such reg");
	/* NOTREACHED */
}
   
/* set register use bits in the struct ps_uregs/sregs
** Ax and AX are considered as different registers
*/
void
fill_ps_regs( bits, ps_regs )
unsigned bits;
unsigned ps_regs[];
{
int i;
unsigned r;
    for (i = 0; i < n_spy_regs; i++) {
        if ((r = bits & spy_regs[i]) !=0) {
            ps_regs[i] = get_max_reg(r);
        }
    }
}
   

/* We consider ps may exist between any two nodes in the same function
** because of presumably short registers life time
** any part of the reg is always set
*/

static void
spy_node(pp,p)
NODE *pp /* previous node */ ;
NODE *p  /* node for tracing */;
{
unsigned i,ssv,usv;
unsigned reg, REG, last_use, last_set, last_xor;

    if (p->op < CALL) {
        p->spy.last_use = pp->spy.last_use;
        p->spy.last_xor = pp->spy.last_xor;
        p->spy.last_set = pp->spy.last_set;
        return;
    }

    if ((p->op == XORL || p->op == XORW || p->op ==XORB) && isreg(p->op1)
    && !strcmp(p->op1,p->op2)) {
        /* after xor we set last_set = last_xor , last_use=0 */
        reg = get_max_reg(setreg(p->op1));
        REG = full_reg(reg);
        p->spy.last_xor = pp->spy.last_xor & ~REG;
        p->spy.last_xor |= reg ;
        p->spy.last_set = pp->spy.last_set & ~REG;
        p->spy.last_set |= reg;
        p->spy.last_use = pp->spy.last_use & ~REG;
        p->spy.ps_regs = 0;
        return;
    }
    if ((p->op == FSTSW || p->op == FNSTSW)) {
        p->spy = pp->spy;
        p->spy.last_set = pp->spy.last_set & ~EAX;
        p->spy.last_set |= Eax;
        p->spy.ps_regs = 0;
        return;
    }

    ssv = p->sets;
    usv = p->uses;
    if (p->op == RET) {
		p->uses &= EAX;
		p->sets=0;
	} else if (p->op == CALL) {
		p->uses=(EBX|EBP|ESI|EDI);
		p->sets=(EAX|ECX|EDX);
	}

    /* copy contents of predecessor */
    p->spy.last_use = pp->spy.last_use;
    p->spy.last_xor = pp->spy.last_xor;
    p->spy.last_set = pp->spy.last_set;
    p->spy.ps_regs=0;

    memset(ps_sregs,0,sizeof(ps_sregs));
    memset(ps_uregs,0,sizeof(ps_uregs));
	fill_ps_regs (p->sets,ps_sregs);
	fill_ps_regs (p->uses,ps_uregs);

    /* modify spy according to the new sets */
    for (i = 0; i < n_ps_regs; i++) {
        reg = ps_sregs[i];
        if (reg == 0) break;
        REG = full_reg(reg);
        p->spy.last_set = p->spy.last_set & ~REG;
        p->spy.last_set |= reg;
    }

    /* Do used regs cause partial stall? */
    for (i=0; i < n_ps_regs ; i++) {
        reg = ps_uregs[i];
        if (reg == 0) break;
        REG = full_reg(reg);
        last_use = pp->spy.last_use & REG;
        last_set = pp->spy.last_set & REG;
        last_xor = pp->spy.last_xor & REG;

        if (last_set != reg) {
            if (last_xor) {
            /* if xor pereceed this instr we accumulate all used regs
               to invalidate coming idioms */

                if (last_set == last_xor) {
                /* use partof(reg) between "xor reg" and "set partof(reg)" */
                    if( is_regs32(reg) && is_regs16(last_set)
                    ||  is_regs16(reg) && is_regs8(last_set)
                    ||  is_regs8(reg) && is_regs8(last_set) && reg != last_set )
                    {
                        p->spy.ps_regs |= reg; /* mark ps */
                    }
                    p->spy.last_use |= reg;
                    continue;
                }
                /* is it supported idiom ? */
                if ((last_use == 0 || last_use == last_set)
                &&(is_regs32(last_xor) && (is_regs8l(last_set) && is_regs32(reg)
                                        || is_regs16(last_set) && is_regs32(reg)
                                        || is_regs8l(last_set) && is_regs16(reg) )
                 || is_regs16(last_xor) && is_regs8l(last_set) && is_regs16(reg)
                 || is_regs8h(last_xor) && is_regs8l(last_set) && is_regs16(reg) ) )
                {
                    ; /* EMPTY */ /* supported idioms */
                }
                else
                    p->spy.ps_regs |= reg; /* mark ps */

                /* use after xor equivalent to set */
                p->spy.last_xor &= ~REG;
                p->spy.last_set &= ~REG;
                p->spy.last_set |= reg;
                p->spy.last_use &= ~REG;
                p->spy.last_use |= reg;
            } else  /* ordinary ps , no previous xor */ {
                /* set eax -> use (ah)
                ** set partof(reg) -> use reg
                */
                if( is_regs8h(reg) && is_regs32(last_set)
                ||  is_regs32(reg) && (is_regs16(last_set)||is_regs8(last_set) )
                ||  is_regs16(reg) && is_regs8(last_set)
                ||  is_regs8(reg) && is_regs8(last_set) && reg != last_set )
                {
                    p->spy.ps_regs |= reg; /* mark ps */
                    p->spy.last_set &= ~REG;
                    p->spy.last_set |= reg;
                }
                p->spy.last_use &= ~REG;
                p->spy.last_use |= reg;
            }
        } else  /* last_set == reg */ {
            /* if xor pereceed this instr we accumulate all used regs
            ** to invalidate coming idioms else keep only last used reg
            */
            if( last_xor )
                p->spy.last_use |= reg;
            else {
                p->spy.last_use &= ~REG;
                p->spy.last_use |= reg;
            }
        }
    }
    p->sets = ssv;
    p->uses = usv;
}/*end spy_node*/


/* This function produces a basic block spy, using as initial information
** the spy information of pf node.
*/
static void
spy_block(pf,b)  NODE *pf; BLOCK *b;
{
extern SWITCH_TBL* get_base_label();
NODE *p;
spy_type *si;
REF * r;
SWITCH_TBL *sw;

	si = &b->spy0;
    if (b->marked == CLEAR) {
		/*  First  visit to  the block */
        b->marked =  PROCESSED;
    	b->spy0 = null_spy;
        /* set first node to UNKOWN */
        *si = pf->spy;
        si->next_sp = NULL;
    } else {
		/* Additional visit to the block.
		** If the number of previous set/xored/used register increas, process
		**the block again. Save this spy info. Otherwise, return.
		*/
        for ( ;si->next_sp != NULL; si = si->next_sp) {
            if ((pf->spy.last_use == si->last_use)
            	&& (pf->spy.last_set == si->last_set)
            	&& (pf->spy.last_xor == si->last_xor)) {
                return; /* nothing to do */
            }
        }
        /* save this case */
		si->next_sp = (spy_type *) getspace(sizeof(spy_type));
       	si = si->next_sp;
       	*si = pf->spy;
       	si->next_sp = NULL;
    }

	/* trace first node */
    for (p = b->firstn; is_any_label(p); p = p->forw) {
        p->spy = null_spy ; /* we dont use labels */
    }
    spy_node(pf,p);

	/* trace all body nodes */
    for( ; p != b->lastn; p = p->forw )
        spy_node(p,p->forw);

	/* trace last node */
    if (b->nextl) spy_block(p,b->nextl);
    if (b->nextr) spy_block(p,b->nextr);

	/* This section serves for switches */
    if (b->nextr == NULL && (b->nextl == NULL
    	||(is_any_br(b->lastn) && !is_any_uncbr(b->lastn)))) {
        if (is_jmp_ind(b->lastn)
        	&& (sw = get_base_label(b->lastn->op1)) != NULL ) {
            for (r = sw->first_ref; r; r = r->nextref) {
                if (sw->switch_table_name == r->switch_table_name)
                    spy_block(p,r->switch_entry);
                if (r == sw->last_ref) break;
            }
        }
        /*else  p->op == ret */ /* add here ret from spy node */
    }
}/*end spy_block*/

/* Driver performes spy PS.
 */
void 
spy_body()
{
BLOCK *b;
NODE *p;
unsigned i,REG,reg;

#ifdef DEBUG
    COND_RETURN("spy_body");
    if( getenv( "no_spy_body") )return;
#endif
	if( asmflag ) return;

	null_spy.last_use = regs32;
	null_spy.last_set = regs32;
	null_spy.last_xor = 0;
	null_spy.ps_regs = 0;
	null_spy_node.spy = null_spy;

	bldgr(false);
    set_refs_to_blocks();
    for (b = b0.next; b ; b = b->next) b->marked = CLEAR;
    
    for (b = b0.next; b ; b = b->next)
        if (b->marked == CLEAR)	spy_block(&null_spy_node, b);

    for (b = b0.next; b ; b = b->next) b->marked = 0;
    for (b = b0.next; b ; b = b->next) {
       	for (p=b->firstn; p!=b->lastn->forw; p=p->forw) {
           	if (p->spy.ps_regs && p->op != CALL && p->op != RET)
               	for (i = 0; i < n_spy_regs; i++) {
                   	reg = spy_regs[i] & p->spy.ps_regs;
                   	if (!reg) continue;
                   	if (set_movzxl(b,p,spy_regs[i]&p->spy.last_set,0))
                       	p->spy.ps_regs &= ~reg;
               	}
           	if (p->spy.ps_regs && (p->op == CALL || p->op == RET))
               	for (i=0; i<n_spy_regs; i++) {
                   	reg = spy_regs[i] & p->spy.ps_regs;
                   		if (!reg) continue;
                   	REG = full_reg(reg);
                   	if (p->nlive & REG) {
                       	if (is_regs8h( p->spy.last_set & spy_regs[i])) {
#if 0
                           	fprintf(stderr,"cant cleanse(hreg) %s in %s for ",
                           	itoreg(p->spy.last_set&spy_regs[i]),get_label());
                           	fprinst( p );
#endif
                           	continue;
                       	}
                       	if (cleanse_reg(b,p,spy_regs[i]&p->spy.last_set,0))
                           	p->spy.ps_regs &= ~REG;
                   	} else {
                       	NODE *pi = insert(p->back);
                       	chgop(pi,XORL,"xorl");
                       	pi->op1 = pi->op2 = itoreg(REG);
                       	pi->sets = REG|CONCODES_AND_CARRY;
                       	pi->nlive = p->nlive|REG;
                       	pi->spy.ps_regs = 0;
                       	p->spy.ps_regs &= ~REG;
#ifdef WD_PSDetector_DEBUG
    fprintf(stderr,"reg %s is cleansed immediately in %s",pi->op1,get_label());
    fprinst(p);
#endif
                   	}
               	}
       	}
   	}
#ifdef WD_printPS
    for(ALLN(p))
    {
        if (is_any_label(p)) continue;
        if (p->spy.ps_regs) {
            NODE *q=p;
            while( !is_any_label(q) ) q=q->back;
            fprintf(stderr,"#### Partial stall in %s in reg :  ", get_label());
            for( i=0; i<n_spy_regs; i++)
                if( (u = spy_regs[i] & p->spy.ps_regs) != 0 )
                    fprintf(stderr,"%s  ",itoreg(p->spy.last_set & full_reg(u)) );
            fprinst(p);
        }
    }
#endif
}/*end spy_body*/

/* finds block where register was set and insert xor before .
   uses ldanal info.
 */
boolean
cleanse_reg( bo, po, reg, depth ) BLOCK *bo; NODE *po; unsigned reg; int depth;
{
    BLOCK *b;
    NODE  *p;
    int c=0;
    unsigned REG;
    boolean t=true;

#ifdef DEBUG
    COND_SKIP(return,"cleanse_reg:%d ",second_idx,0,0 );
    if( getenv( "no_cleanse_reg" ) ) return false;
#endif

    if( po == NULL )
        po = bo->lastn;  /* scan block from end */
    else if( po == bo->firstn )
        goto lab_cleanse_reg_1;
    else
        po = po->back;

    REG = full_reg(reg);
    if(bo->marked & REG ) return true ;/* here already cleansed */
    bo->marked |= REG; /* mark this path( not only the block) */

    for( p=po; p != bo->firstn->back; p=p->back)
    {
        if( ++c > 4 ) depth++;;
        if( p->sets & REG )
        {

            if( is_regs32( get_max_reg(p->sets & REG) ) ) return true;
            if( p->uses & REG || p->nlive & CONCODES_AND_CARRY)
            {
                if(p->spy.last_use == reg ) continue;
#if 0
                fprintf( stderr,"###cant cleanse in %s",get_label() );
                fprinst( bo->firstn );
                fprinst( p );
#endif
                return false;
            }
            p = insert(p->back);
            chgop(p, XORL, "xorl");
            p->op1 = p->op2 = itoreg(REG);
            p->sets = REG | CONCODES_AND_CARRY;
            p->uses = 0;
            p->nlive = p->forw->nlive | REG;
            p->spy.ps_regs=0;
            if(bo->firstn == p->forw ) bo->firstn = p;
#ifdef WD_PSDetector_DEBUG
fprintf(stderr,"#reg %s is cleansed in",p->op1); fprinst( bo->firstn);
#endif
            return true;
        }
    }
lab_cleanse_reg_1:
    if(++depth > 4 ) return true;
    for( b=b0.next; b != NULL ;b=b->next)
    {
        if( b == bo ) continue;
        if( b->nextl == bo || b->nextr == bo )
            t &= cleanse_reg(b, (NODE *) NULL, reg, depth);
    }
    return t;
}

/* finds block where register was set and cha nge movx to movzxl .
   uses  ldanal info.
 */
boolean
set_movzxl( bo, po, reg, depth ) BLOCK *bo; NODE *po; unsigned reg; int depth;
{
    BLOCK *b;
    NODE  *p;
    int c=0;
    unsigned REG;
	unsigned tmp_reg;
    boolean t=true;

#ifdef DEBUG
    COND_SKIP(return,"set_movzxl:%d ",second_idx,0,0 );
    if( getenv( "no_set_movzxl" ) ) return false;
#endif

    if( po == NULL )
        po = bo->lastn;  /* scan block from end */
    else if( po == bo->firstn )
        goto lab_set_movzxl_1;
    else
        po = po->back;

    REG = full_reg(reg);
    if(bo->marked & REG ) return true ;/* here already cleansed */
    bo->marked |= REG; /* mark this path( not only the block) */

    for( p=po; p != bo->firstn->back; p=p->back)
    {
        if( ++c > 4 ) depth++;;
        if( p->sets & REG )
        {

            if( is_regs32( get_max_reg(p->sets & REG) ) ) return true;

	    if (p->op == MOVW &&
			(tmp_reg = is_regs16(p->spy.last_set & REG)) &&
			(p->nlive & full_reg(tmp_reg)) == tmp_reg ) {
			if (isimmed(p->op1))
					chgop(p, MOVL, "movl");
			else
					chgop(p, MOVZWL, "movzwl");
					p->op2 = itoreg( REG );
			}
            else if (p->op == MOVB &&
			(tmp_reg = is_regs8l(p->spy.last_set & REG)) &&
			(p->nlive & full_reg(tmp_reg)) == tmp_reg)
            {
                if (isimmed(p->op1))
                    chgop(p, MOVL, "movl");
                else
                    chgop(p, MOVZBL, "movzbl");
                p->op2 = itoreg( REG );
            }
            else
            {
                if(p->spy.last_use == reg ) continue;
#if 0
                fprintf( stderr,"cant convert to movzxl in %s",get_label() );
                fprinst( bo->firstn );
                fprinst( p );
#endif
                return false;
            }
#ifdef WD_PSDetector_DEBUG
fprintf(stderr,"#set_movzxl for %s in",p->op1); fprinst( bo->firstn);
#endif
            return true;
        }
    }
lab_set_movzxl_1:
    if(++depth > 4 ) return true;
    for( b=b0.next; b != NULL ;b=b->next)
    {
        if( b == bo ) continue;
        if( b->nextl == bo || b->nextr == bo )
            t &= set_movzxl(b, (NODE *) NULL, reg, depth);
    }
    return t;
}

void
init_ebboptim()
{

	regmasx = fixed_stack ? fix_regmasx : reg_regmasx ;
    registers = fixed_stack ? fix_registers : reg_registers ;

}
