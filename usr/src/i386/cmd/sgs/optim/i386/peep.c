#ident	"@(#)optim:i386/peep.c	1.1.3.53"
/* peep.c
**
**	Intel 386 peephole improvement driver
**
**
** This module contains the driver for the Intel 386 peephole improver.
*/


#include "optim.h"
#include "database.h"
#include "optutil.h"
#include "regal.h"
#include <malloc.h>

extern char *strstr();
extern int fp_removed;
extern int first_run;
extern BLOCK *find_cur_block();
extern int tflag;			/* non-0 to disable redundant load op */
extern int Tflag;			/* non-0 to display t debugging       */

static void prld();			/* routine to print live/dead data */
static void rdfree();

void peep()
{

	COND_RETURN("peep");

	window(6, w6opt);		/* do 6-instruction sequences */
	window(5, w5opt);		/* do 5-instruction sequences */
	window(4, w4opt);		/* do 4-instruction sequences */
	window(3, w3opt);		/* do 3-instruction sequences */
	window(2, w2opt);		/* do 2-instruction sequences */
	window(1, w1opt);		/* do 1-instruction sequences */
	first_run = 0;
	window(2, w2opt);		/* do 2-instruction sequences */
	window(3, w3opt);		/* do 3-instruction sequences */
	window(4, w4opt);		/* do 4-instruction sequences */

	window(1, w1opt);		/* now repeat to clean up stragglers */
	window(2, w2opt);
	window(1, w1opt);
	sets_and_uses();		/* update sets and uses fields of NODEs */
    
    return;
}

static const nregs = 7;
static const first_non_scr  = 3;
static unsigned int reg_regmasx[] = { EAX, EDX, ECX, ESI, EDI, EBX, EBI };
static unsigned int fix_regmasx[] = { EAX, EDX, ECX, ESI, EDI, EBX, EBP };
static char *regnames[] = { "%eax","%edx", "%ecx", "%esi", "%edi", "%ebx", "%ebp" };
static NODE *push_reg[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static NODE *pop_reg[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static int live_after_call[] = { 0, 0, 0, 0, 0, 0, 0 };
static int is_used[] = { 0, 0, 0, 0, 0, 0, 0 };
/*is_used is not boolean, it's three way:
			0 - not used.
			1 - used.
			2 - used and can not be replaced by other registers - hard usage.
*/
#define NOT_USED	0
#define USED		1
#define HARD_USED	2

static void
init_arrays()
{
int reg;
	for (reg = 0; reg < nregs; reg++) {
		push_reg[reg] = pop_reg[reg] = NULL;
		is_used[reg] = 0;
		live_after_call[reg] = 0;
	}
}/*end init_arrays*/

static void
try_to_free_ecx()
{
NODE *p;
boolean pop_found = false;
unsigned int explicit_regs;
	for (ALLN(p)) {
		explicit_regs = 0;
		if (p->op1) explicit_regs |= scanreg(p->op1,0);
		if (p->op2) explicit_regs |= scanreg(p->op2,0);
		if (explicit_regs & ECX && (p->uses | p->sets) & ECX) {
			if (p->op != POPL) {
				return;
			}
			else if (setreg(p->op1) == ECX && !(p->nlive & ECX))
				pop_found = true;
		}
	}
	if (!pop_found) return;
	/*found some pop ecx, and nothing else going on with ECX!!*/
	for (ALLN(p)) {
		if (p->op == POPL && setreg(p->op1) == ECX) {
			chgop(p,ADDL,"addl");
			p->op1 = "$4";
			p->op2 = "%esp";
			p->uses = 0;
			p->sets = ESP | CONCODES_AND_CARRY;
		}
	}
}

void
rmrdpp()
{
  NODE *p;
  int add_stack;
  int nfound = 0;
  int has_ret = 0;
  int reg;
  int sc_reg, usr_reg;
  boolean marked;
  extern void replace_registers();
  unsigned int *regmasx;

	COND_RETURN("rmrdpp");

	if (fixed_stack) {
		regmasx = fix_regmasx;
	} else {
		regmasx = reg_regmasx;
	}
	init_arrays();
	ldanal();
	for(p = ntail.back; p != &n0; p = p->back) {
		if (   (p->op == RET)
		    || (p->op == LRET)) {
			has_ret = true;
			break;
		}
	}
	if (! has_ret) {
		return;
	}
	for (ALLN(p)) {
		/* find push ebx, push esi and push edi */
		if (!fixed_stack) {
			if (p->op == PUSHL) {
				for (reg = first_non_scr; reg < nregs; reg++) {
					if ((p->uses&~ESP) == regmasx[reg]) {
						if (! push_reg[reg]) {
							push_reg[reg] = p;
							++nfound;
						}
					}
				}
			}
		} else {
			for (reg = first_non_scr; reg < nregs; reg++) {
				if (IS_FIX_PUSH_REG(p,regnames[reg])) {
 					if (! push_reg[reg]) {
						push_reg[reg] = p;
						++nfound;
					}
				}
			}
		}
		if (nfound == 4) {
			break;
		}
	} /* end for loop */

	/* If there already is no push for some reg, mark it, so it will
	** not be deleted later (Segmentation fault core dumped)  ??????
	** Mark it as not replaceable with a scratch register. 
	*/
	for (reg = first_non_scr; reg < nregs; reg++) {
		if (! push_reg[reg]) {
			is_used[reg] = HARD_USED;
		}
	}
	nfound = 0;

	/* Find the pop instructions in the epilogue */
	for (p=ntail.back; p != &n0; p=p->back) {
		if (!fixed_stack) {
			if (p->op == POPL) {
				for (reg = first_non_scr; reg < nregs; reg++) {
					if ((p->sets&~ESP) == regmasx[reg]) {
						if (! pop_reg[reg]) {
							pop_reg[reg] = p;
							++nfound;
						}
					}
				}
			}
		} else {
			for (reg = first_non_scr; reg < nregs; reg++) {
				if (IS_FIX_POP_REG(p,regnames[reg])) {
					if (! pop_reg[reg]) {
						pop_reg[reg] = p;
						++nfound;
					}
				}
			}
		}
		if (nfound == 4) {
			break;
		}
	} /* for loop */

	if (   !eiger
	    && !fixed_stack) {
		try_to_free_ecx();
	}

	/* Find which of the registers are used in the function. */
	for (ALLN(p)){
		marked = false;
		for (reg = first_non_scr; reg < nregs; reg++) {
			if (   p->op == RET
			    || p == push_reg[reg]
			    || p == pop_reg[reg]) {
				marked = true;
				break;
			}
		}
		if (marked) {
			continue;
		}
		if (   p->op == CALL
		    || p->op == LCALL) {
			for (reg = first_non_scr; reg < nregs; reg++) {
				if (p->nlive & regmasx[reg]) {
					live_after_call[reg] = true;
				}
			}
		}
		for (reg = 0; reg < nregs; reg++) {
			if (   p->op != CALL
			    && p->op != LCALL
			    && ((p->uses | p->sets) & regmasx[reg])) {
				if (hard_uses(p) & regmasx[reg]) {
					is_used[reg] = HARD_USED;
				} else if (is_used[reg] != HARD_USED) {
					/* replaceable */
					is_used[reg] = USED; /* replaceable */
				}
			}
		}
	} /* for loop */

	/* Try to change user registers to scratch registers */
	for (sc_reg = 0; sc_reg < first_non_scr; sc_reg++) {
		if (is_used[sc_reg] == NOT_USED) {
			for (usr_reg = first_non_scr; usr_reg < nregs; usr_reg++) {
				if ((is_used[usr_reg] == USED) && (!live_after_call[usr_reg])) {
					for (ALLN(p)) {
						replace_registers(p,regmasx[usr_reg],regmasx[sc_reg],3);
						new_sets_uses(p);
					}
					is_used[usr_reg] = NOT_USED;
					is_used[sc_reg] = USED;
					break; /* do not change with this scratch again */
				}
			}
		}
	}

	/* Delete the redundant push pop instructions */
	add_stack = 0;
	for (reg = first_non_scr; reg < nregs; reg++) {
		if (is_used[reg] == NOT_USED) {
			DELNODE(push_reg[reg]);
			DELNODE(pop_reg[reg]);
			add_stack += 4;
		}
	}
	/* Information needed later for frame pointer elimination */
	for (ALLN(p)) {
		if(p->op == CALL) {
			p->opm -= add_stack;
		}
	}
	if (   fixed_stack
	    && func_data.regs_spc == add_stack) {
		 /* After removing all redundant saves and restores,
		 *  if the function does not use the stack
		 *  at all (except the add and sub inst.), then
		 *  the .set, the add and the sub inst. can be 
		 *  removed.
		 */
		func_data.regs_spc = 0 ; /* needed to control offsets */
	}
} /* end rmrdpp */



/* Print live/dead data for all instruction nodes */



	static void
prld()
{
    register NODE * p;

    for (ALLN(p))			/* for all instruction nodes... */
    {
	PUTCHAR(CC);			/* write comment char */

	PRINTF("(live: 0x%7.7x)", p->nlive); /* print live/dead data */
	prinst(p);			/* print instruction */
    }
    return;
}

/*
 * This section of code, pays attention to register loads,
 * and tries to do redundant load removal.  This is done,
 * by watching mov instructions, and keeping track of what
 * values are stored in the actual register.  If the mov
 * is reloading a register with the same contents, then the
 * mov is deleted.
 */
#define NUMVALS 20

static struct	rld {
	char	*rname;			/* register name */
	int	rnum;			/* register number    */
	int	rsrc;			/* size of the mov source   */
	int	rdst;			/* size of the mov dest reg */
	int	mvtype;			/* type of move instruction */
	char	*ropnd;			/* Actual operand */
} rreg[NUMVALS];

static NODE *ccodes;

static int numcache;	/* number of real values currently in cache */

#define FORALL(rp,n) \
	for (rp = rreg, n = numcache; n > 0; ++rp) \
		if (rp->ropnd != NULL ? n-- : 0)


	static void
insertcache(rname, srcsize, dstsize, movetype, ropnd)
/* mark that register rname has the operand ropnd */
char *rname, *ropnd;
int srcsize, dstsize, movetype;
{
	register struct rld *rp;
	int n;

	if (!strcmp(rname, ropnd)) return;

	FORALL(rp,n)
		if (!strcmp(rp->rname, rname) && !strcmp(rp->ropnd, ropnd)
		&&  rp->rsrc == srcsize && rp->rdst == dstsize
		&&  rp->mvtype == movetype)
			return; /* It is already stored  */
	for (rp = rreg, n = NUMVALS; n > 0; ++rp, --n)
		if (rp->ropnd == NULL) {
			rp->rname = rname;        /* register name */
			rp->rnum = setreg(rname); /* register number    */
			rp->rsrc = srcsize;       /* size of the mov source   */
			rp->rdst = dstsize;       /* size of the mov dest reg */
			rp->mvtype = movetype;   /* type of move instruction */
			rp->ropnd = ropnd;	      /* Actual operand */
			++numcache;
			return;
		}
}


	static void
rdfree(arg)
struct rld * arg;
{
	register struct rld *rp;

	if (arg) {
		arg->ropnd = NULL;
		--numcache;
	}
	else {
		FORALL(rp,numcache)
			rp->ropnd = NULL;
		ccodes = NULL;
	}
}


	static void
rdfreemem(dop, dsize)
char *dop;
int dsize;
{
	register int offs1, offs2;
	int n;
	register struct rld *rp;
	boolean onstack = isiros(dop), aliased = !lookup_regals(dop,
								first_regals);

	FORALL(rp,n)
		if (!isreg(rp->ropnd) && *rp->ropnd != '$')
			if (onstack) {
				if (aliased)
					if (!isiros(rp->ropnd) ||
                        !strstr(rp->ropnd, fixed_stack?ESP_STR:EBP_STR) ||
                        !strstr(dop, fixed_stack?ESP_STR:EBP_STR))
						rdfree(rp);
					else {
						offs1 = (int)strtoul(dop,(char **)NULL,0);
						offs2 = (int)strtoul(rp->ropnd,(char **)NULL,0);
						if (offs1 < offs2) {
							if (dsize == -1 ||
							    offs1 + dsize >=
							    offs2)
							rdfree(rp);
						} else if (offs2 + rp->rdst >=
							   offs1)
							rdfree(rp);
					}
			}
			else if (isiros(rp->ropnd)) {
				if (!lookup_regals(rp->ropnd, first_regals))
					rdfree(rp);
			}
			else if (isdigit(*rp->ropnd) || *rp->ropnd == '-' ||
				 *rp->ropnd == '(')
				rdfree(rp);
			else if (dop == NULL || isdigit(*dop) || *dop == '-' ||
				*dop == '(')
				rdfree(rp);
			else {
				char *p1, *p2, t1, t2;

				p1 = rp->ropnd; p2 = dop;
				while (*p1 && (isalnum(*p1) || *p1 == '_' ||
						*p1 == '.'))
					++p1;
				while (*p2 && (isalnum(*p2) || *p2 == '_' ||
						*p2 == '.'))
					++p2;
				t1 = *p1, t2 = *p2;
				*p1 = '\0'; *p2 = '\0';
				if (!strcmp(rp->ropnd, dop))
					rdfree(rp);
				*p1 = t1; *p2 = t2;
			}
}


static char *
other_set_inst(n, srcsize, dstsize)
NODE *n;
int  *srcsize, *dstsize;
{
	if ((n->op == XORB || n->op == XORW || n->op == XORL)
	 && isreg(n->op1)
	 && isreg(n->op2)
	 && !strcmp(n->op1, n->op2)
	) {
		if (n->op == XORB)
			*srcsize = 1, *dstsize = 1;
		else if (n->op == XORW)
			*srcsize = 2, *dstsize = 2;
		else
			*srcsize = 4, *dstsize = 4;
		return "$0";
	}
	else if (n->op == LEAL) {
		char *tmp = (char *)malloc(strlen(n->op1)+2);
		*tmp = '&';
		strcpy(tmp+1, n->op1);
		*srcsize = *dstsize = 4;
		return tmp;
	}

	return NULL;
}


static void
replace_operands(p)
register NODE *p;
{
	register struct rld *rp;
	int n;
	boolean nodice = false;

	switch (p->op) {

		case CMPB:  case CMPW:  case CMPL:
		case TESTB: case TESTW: case TESTL:

		{
		register char *r1, *r2, *c1, *c2;
		boolean reg1, reg2;

		c1 = isnumlit(p->op1) ? p->op1 : NULL;
		r1 = isreg(p->op1) ? p->op1 : NULL;
		c2 = isnumlit(p->op2) ? p->op2 : NULL;
		r2 = isreg(p->op2) ? p->op2 : NULL;

		if (!isvolatile(p,1))
		   FORALL(rp,n)
			if (rp->rsrc == rp->rdst
			&&  OpLength(p) == rp->rsrc) {
				if (!strcmp(p->op1, rp->ropnd)) {
					if (!c1 && isnumlit(rp->rname))
						c1 = rp->rname;
					else if (!r1)
						r1 = rp->rname;
				}
				else if (!strcmp(p->op1, rp->rname) && !c1 
						&& isnumlit(rp->ropnd))
					c1 = rp->ropnd;
				if (c1 && r1)
					break;
			}
		if (!isvolatile(p,2))
		   FORALL(rp,n)
			if (rp->rsrc == rp->rdst
			&&  OpLength(p) == rp->rsrc) {
				if (!strcmp(p->op2, rp->ropnd)) {
					if (!c2 && isnumlit(rp->rname))
						c2 = rp->rname;
					else if (!r2 && !isconst(rp->rname))
						r2 = rp->rname;
				}
				else if (!strcmp(p->op2, rp->rname) && !c2 
						&& isnumlit(rp->ropnd))
					c2 = rp->ropnd;
				if (c2 && r2)
					break;
			}

		reg1 = isreg(r1), reg2 = isreg(r2);
		if (reg1 && reg2)
			p->op1 = r1, p->op2 = r2;
		else if (c1 && reg2)
			p->op1 = c1, p->op2 = r2;
		else if (reg1 || reg2) {
			if (r1)
				p->op1 = r1;
			if (r2)
				p->op2 = r2;
		}
		}

		break;

					
		case IMULB: case IMULW: case IMULL:
		if (!p->op2) nodice = true;

		if (p->op3) {
			if (!isreg(p->op2))
				FORALL(rp,n)
					if (!strcmp(p->op2, rp->ropnd)
					 && !isvolatile(p,2)
					 && rp->rsrc == rp->rdst
					 && OpLength(p) == rp->rsrc
					 && isreg(rp->rname))
						{ /* found an operand to replace */
						p->op2 = /*strdup*/(rp->rname);
						break;
						}
			break;
		}
		goto cont;


		case MOVSBW: case MOVSBL: case MOVSWL:
		case MOVZBW: case MOVZBL: case MOVZWL:
		case MULB: case MULW: case MULL:
		case DIVB: case DIVW: case DIVL:
		case IDIVB: case IDIVW: case IDIVL:
			nodice = true;
			/* FALLTHROUGH */

		case ORB:  case ORW:  case ORL:
		case ADCB: case ADCW: case ADCL:
		case ADDB: case ADDW: case ADDL:
		case ANDB: case ANDW: case ANDL:
		case MOVB: case MOVW: case MOVL:
		case SBBB: case SBBW: case SBBL:
		case SUBB: case SUBW: case SUBL:
		case XORB: case XORW: case XORL:
		case PUSHW: case PUSHL:

cont:		
		{
			char *cst = NULL, *reg = NULL;

			FORALL(rp,n)
	   			if (!strcmp(p->op1, rp->ropnd)
				 && !isvolatile(p,1)
				 && rp->rsrc == rp->rdst
				 && OpLength(p) == rp->rsrc
				 && (!nodice || *rp->rname != '$'))
					{ /* found an operand to replace */
					if (isreg(rp->rname)) {
						if (!reg || !p->op2 ||
						    !strcmp(rp->rname, p->op2))
							reg = rp->rname;
					} else
						cst = rp->rname;
					}
			if (reg && (p->sets & ESP)) break;
            if (!fixed_stack) {
                if (cst && p->op != PUSHW && p->op != PUSHL && !isvolatile(p,1) &&
                    !strcmp(cst, "$0")) {
                        p->op1 = cst;
                }
                else if (reg && !isvolatile(p,1))
                    p->op1 = reg;
                else if (cst && !isreg(p->op1) && !isvolatile(p,1))
                {
                    p->op1 = cst;
                }
                break;
            } else {
                if (cst && !IS_FIX_PUSH(p) && !isvolatile(p,1) &&
                    !strcmp(cst, "$0")) {
                        p->op1 = cst;
                    }
                else if (reg && !isvolatile(p,1))
                    p->op1 = reg;
                else if (cst && !isreg(p->op1) && !isvolatile(p,1)) {
                    p->op1 = cst;
                }
                break;
             }
		}
	}
}


void
rmrdld()
{
	register NODE *p;
	int n;
	register struct rld *rp;
	char *dop, *sop;
	unsigned rnum;
	int src1, dst1, movetype;
#ifdef FLIRT
	static int numexec=0;
#endif

	COND_RETURN("rmrdld");

#ifdef FLIRT
	++numexec;
#endif


	if (tflag)
		return;
	rdfree((struct rld*)NULL);
	for (ALLN(p))			/* for all instruction nodes... */
	{
 	   if (Tflag) {
		 PRINTF("%c rmrdld: ", CC);
		 prinst(p);	/* print instruction */
	   }
      if (!IS_FIX_POP(p) && !IS_FIX_PUSH(p)) {
		if (isuncbr(p) || islabel(p) || p->op == ASMS ||
			(p->op >= SMOVB && p->op <= SSTOL) || is_safe_asm(p))
			rdfree((struct rld*)NULL);	/* free up the list between breaks */
		else if (p->op == CALL || p->op == LCALL) {
			rnum = EAX | EDX | ECX;
			FORALL(rp,n) {
				if (rnum & rp->rnum
				 || rnum & scanreg(rp->ropnd, false))
					rdfree(rp);
			}
			ccodes = NULL;
			rdfreemem((char *)NULL, -1);
		} else if ( !isvolatile(p,1) && !isvolatile(p,2) && 
		  ( (sop = p->op1, ismove(p, &src1, &dst1)) != 0 ||
		    (sop = other_set_inst(p, &src1, &dst1)) != 0)    ) {
			ccodes = NULL;
			if (!isreg(p->op2)) {
				dop = p->op2;
				FORALL(rp,n) {
					if (!strcmp(p->op1, rp->rname)
					 && !strcmp(dop, rp->ropnd)
					 && rp->rsrc == src1
					 && rp->rdst == dst1
					 && !isvolatile(p,2)
					   ) {
						/* found a redundant load */
						if (Tflag) {
							PRINTF("%c removed",CC);
							prinst(p);
						}
						COND_SKIP(continue,"%d ",second_idx,0,0);
						ldelin2(p);
						DELNODE(p);
						FLiRT(O_RMRDLD,numexec);
						dop = NULL;
						break;
					}
				}
				if (dop == NULL) continue;

				if (!isreg(p->op1)) {
					replace_operands(p);
					new_sets_uses(p);
				}
				rnum = 0;
				FORALL(rp,n)
					if (!strcmp(dop, rp->ropnd)) {
						rnum |= rp->rnum;
						rdfree(rp);
					}
				if (rnum)
				   FORALL(rp,n)
					if (rnum & scanreg(rp->ropnd, false))
						rdfree(rp);
				rdfreemem(dop, dst1);
				/*else*/ /* if (src1 == dst1) */
				insertcache(p->op1, src1, dst1, 'l', dop);
				if (isreg(p->op1))
				   FORALL(rp,n) {
					if (!strcmp(p->op1, rp->rname)
					&&  (isreg(rp->ropnd) ||
					     *rp->ropnd == '$')
					&&  rp->rsrc == src1
					&&  rp->rdst == dst1
				   	   )
				   	   insertcache(rp->ropnd, src1, dst1,
						       'l', dop);
					}
				else	/* literal ? */
				   FORALL(rp,n)
					if (!strcmp(p->op1, rp->ropnd)
					&&  rp->rsrc == src1
					&&  rp->rdst == dst1
					   )
				   	   insertcache(rp->rname, src1, dst1,
						       'l', dop);
			} else {
				movetype = src1 == dst1 ? 'l' : p->opcode[3];
				FORALL(rp,n) {
					if (!strcmp(p->op2, rp->rname)
					 && !strcmp(sop, rp->ropnd)
					 && rp->rsrc == src1
					 && rp->rdst == dst1
					 && rp->mvtype == movetype
					 && !isvolatile(p,1)
					   ) { /* found a redundant load */
						if (Tflag) {
							PRINTF("%c removed",CC);
							prinst(p);
						}
						COND_SKIP(continue,"%d ",second_idx,0,0);
						ldelin2(p);
						DELNODE(p);
						FLiRT(O_RMRDLD,numexec);
						sop = NULL;
						break;
					}
				}
				if (sop == NULL) continue;

				if (!isreg(p->op1)) {
					replace_operands(p);
					new_sets_uses(p);
				}
				/* time to change the move */
				rnum = scanreg(p->op2, false);
				FORALL(rp,n) {
					if (rnum & rp->rnum
					 || rnum & scanreg(rp->ropnd, false))
						rdfree(rp);
				}
				if (!(rnum & scanreg(sop, false))) {
					insertcache(p->op2, src1, dst1,
						    movetype, sop);
					if (isreg(sop)) {
					   FORALL(rp,n)
						if (!strcmp(sop, rp->rname)) {
						  if (rp->rdst == rp->rsrc &&
						      src1 == dst1)
						    insertcache(p->op2,
								rp->rsrc, dst1,
								'l', rp->ropnd);
						  else if (*rp->ropnd != '$')
						     if (rp->rdst == rp->rsrc)
						        insertcache(p->op2,
								rp->rsrc, dst1,
								movetype,
								rp->ropnd);
						     else if (src1 == dst1)
						        insertcache(p->op2,
								rp->rsrc, dst1,
								rp->mvtype,
								rp->ropnd);
						}
					   if (src1 == dst1)
						  insertcache(sop, dst1, src1,
							'l', p->op2);
					}
					else if (*sop == '$') {
					   FORALL(rp,n)
						if (!strcmp(sop, rp->ropnd)
						&&  rp->rdst == src1)
					   	   insertcache(p->op2, src1,
							dst1, 'l', rp->rname);
						else if (!strcmp(sop, rp->rname)
						     &&  rp->rdst == src1)
					   	   insertcache(p->op2, src1,
							dst1, 'l', rp->ropnd);
					}
					else {
					   FORALL(rp,n)
						if (!strcmp(sop, rp->ropnd)
						&&  rp->rsrc == src1
						&&  rp->rdst == dst1
						&&  rp->mvtype == movetype)
					   	   insertcache(p->op2,
							rp->rdst, dst1,
							movetype, rp->rname);
					}
				}
			}
		} else {
			rnum = p->sets;
			dop = dst(p);		/* dst doesn't return NULL */
			if (isreg(dop) || *dop == '\0')
				dop = NULL;
			replace_operands(p);
			new_sets_uses(p);
			if (rnum)
			   FORALL(rp,n) {
				if (rnum & rp->rnum
				 || rnum & scanreg(rp->ropnd, false)) {
					rdfree(rp);
				}
			}
			if (dop != NULL) {
			   FORALL(rp,n)
				if (!strcmp(dop, rp->ropnd))
					rdfree(rp);
			   rdfreemem(dop, -1);
			}

			if (rnum == CONCODES && !dop)
				if (ccodes)
					if (same_inst(p, ccodes)) {
						COND_SKIP(continue,"%d ",second_idx,0,0);
						ldelin2(p);
						DELNODE(p);
						FLiRT(O_RMRDLD,numexec);
					}
					else
						ccodes = p;
				else
					ccodes = p;
			else if (dop || rnum)
				ccodes = NULL;
		}
      } /* of not fixed push or pop */
	}
}/*end rmrdld*/


static unsigned int
tmpsets(p)
NODE *p;
{
	unsigned int regs = p->uses | p->sets, tmpregs = 0;;

	if (regs & EAX)
		tmpregs = EAX;
	if (regs & ECX)
		tmpregs |= ECX;
	if (regs & EDX)
		tmpregs |= EDX;

	return tmpregs;
}


static boolean
gettmpreg(reg, p)
unsigned *reg;
NODE *p;
{
	if (*reg)
		*reg &= ~(p->nlive | tmpsets(p));
	else {
		if (!(p->nlive & EAX) && !(p->uses & EAX) && !(p->sets & EAX))
			*reg |= EAX;
		if (!(p->nlive & ECX) && !(p->uses & ECX) && !(p->sets & ECX))
			*reg |= ECX;
		if (!(p->nlive & EDX) && !(p->uses & EDX) && !(p->sets & EDX))
			*reg |= EDX;
	}

	return *reg != 0;
}

static char
*tmp_reg_size(reg, inst_size)
unsigned *reg;
int inst_size;	/* better be ByTE , WoRD, or LoNG */
{
	if (*reg & ECX) {
		*reg = ECX;
		if (inst_size == ByTE)
			return "%cl";
		else if (inst_size == WoRD)
			return "%cx";
		else
			return "%ecx";
	}
	else if (*reg & EDX) {
		*reg = EDX;
		if (inst_size == ByTE)
			return "%dl";
		else if (inst_size == WoRD)
			return "%dx";
		else
			return "%edx";
	}
	else if (*reg & EAX) {
		*reg = EAX;
		if (inst_size == ByTE)
			return "%al";
		else if (inst_size == WoRD)
			return "%ax";
		else
			return "%eax";
	}
	/* NOTREACHED */
}
	/* found a consecutive instruction with the same literal */

#ifdef FLIRT
	static int numreplace=0;
#endif

static int
check_and_replace(first,last,tmpreg,literal)
NODE *first,*last;
unsigned int tmpreg;
char *literal;
{	
	unsigned val = 0;
	boolean found = false;
	NODE *p;
	NODE *prepend(), *pnew;
	char *dest;
	first->extra2 = 1;
	for ( p = first ; p != last ; p= p->forw) {
		if (p->sets & ESP)
			p->extra2 = 0;
		tmpreg &= ~ (p->nlive | p->sets | p->uses);
		if (p->extra2) {
			if (p->op == MOVL && isreg(p->op2))
				val += 2; /* Create mov %reg,%reg that might be removed */
			else
				val += hasdisplacement(p->op2) ? 3 : 1;
			if ( val > 3)
				break;
		} else	if ( isbr(p) )
			val /= 2;

	}
	if (tmpreg == 0)
		return 0;
	if ( val < 4)
		return 0;
	dest = tmp_reg_size(&tmpreg, LoNG);
	COND_SKIP(return 0,"prepend %d movl	%s,%s before ",second_idx,literal,dest);
	pnew = prepend(first, dest);
	if (!strcmp(literal, "$0") && ! (CONCODES & first->nlive)) {
		FLiRT(O_REPLACE_CONSTS,numreplace);
		chgop(pnew, XORL, "xorl");
		pnew->op1 = pnew->op2 = dest;
		pnew->uses =0;
		pnew->sets = tmpreg | CONCODES_AND_CARRY;
	} else {
		FLiRT(O_REPLACE_CONSTS,numreplace);
		chgop(pnew, MOVL, "movl");
		pnew->op1 = literal;
		pnew->op2 = dest;
		pnew->uses =0;
		pnew->sets = tmpreg;
	}
	for ( p = first ; p != last ; p= p->forw) {
		if (! p->extra2)
			continue;
		p->op1 = tmp_reg_size(&tmpreg,OpLength(p));
		p->uses |= setreg(p->op1);
		if ( (! found)
			&& ( p->op == MOVL || p->op == MOVB || p->op == MOVW )
   					&& isreg(p->op2)
   					&& !samereg(p->op1,p->op2) /* movX %reg,%reg. */
	       )	 
			found = true; /* rmrdmv() will try to remove it */
	}
	return found;
}


int
replace_consts(run)
boolean run;
{
	register NODE *p, *first = NULL;
	boolean looking = true, isone,found = false,found2 = false;
	char *literal = NULL;
#define NONE 0
#define OUT  3
	int strikes = NONE;
	unsigned found_tmpreg =0, tmpreg = 0;

 	COND_RETURNF("replace_consts");

#ifdef FLIRT
	numreplace++;
#endif

	for (ALLN(p))			/* for all instruction nodes... */
	{
		p->extra2 = 0;
		if (isuncbr(p) || islabel(p) || is_safe_asm(p) ||
		    p->op == ASMS || p->op == CALL || p->op == LCALL) {
			if (found2)
				found = check_and_replace(first,p,found_tmpreg,literal);
			found2 = false;
			looking = true;
			first = NULL;
			strikes = NONE;
			tmpreg = 0;
		}
		else if (!p->op1 || *p->op1 != '$') {
			if (!looking && (++strikes == OUT ||
					 !(tmpreg &= ~tmpsets(p)))) {
				if (found2)
					found = check_and_replace(first,p,found_tmpreg,literal);
				found2 = false;
				looking = true;
				first = NULL;
				strikes = NONE;
				tmpreg = 0;
			}
		} else {
			switch (p->op) {
			   case ADDL:  case ADDW:  case ADDB:
			   case ANDL:  case ANDW:  case ANDB:
			   case CMPL:  case CMPW:  case CMPB:
			   case MOVL:  case MOVW:  case MOVB:
			   case ORL:   case ORW:   case ORB:
			   case PUSHL: case PUSHW:
			   case SUBL:  case SUBW:  case SUBB:
			   case TESTL: case TESTW: case TESTB:
			   case XORL:  case XORW:  case XORB:
				isone = true;
				break;
			   default:
				isone = false;
				break;
			}
			if (
			   isone
			   && (!p->op2 || strcmp(p->op2, "%esp") || !looking)
			   && gettmpreg(&tmpreg,p)
			  ) {
				if (looking) {
					if (found2)
						found = check_and_replace(first,p,found_tmpreg,literal);
					found2 = false;
					looking = false;
					first = p;
					literal = p->op1;
				}
				else if (strcmp(p->op1, literal) == 0) {
	/* found a consecutive instruction with the same literal */
					found2 = true;
					found_tmpreg = tmpreg;
					p->extra2 = 1;
				}
				else if (run) {
					if (found2)
						found = check_and_replace(first,p,found_tmpreg,literal);
					found2 = false;
					literal = p->op1;
					first = p;
					strikes = NONE;
					tmpreg = 0;
					if (!gettmpreg(&tmpreg,p))
						looking = true, first = NULL;
				}
				else if (!looking && ++strikes == OUT) {
					if (found2)
						found = check_and_replace(first,p,found_tmpreg,literal);
					found2 = false;
					looking = true;
					first = NULL;
					strikes = NONE;
					tmpreg = 0;
				}
			}
		}
	}
	if (found2)
		found = check_and_replace(first,p,found_tmpreg,literal);
	return found;
}

/*floating point peephole optimizations. */
#include "fp_timings.h"
extern char *st_i[]; /* in loops.c */

static boolean changed_between();
#define break_bb(p)	(p && (is_any_br(p) || is_any_label(p) || is_safe_asm(p) \
					|| p->op == CALL || p->op == LCALL || p->op == ASMS))

static NODE *first;	/* pointer to first window node */
static NODE *opf;	/* pointer to predecessor of first window node */
NODE *first_fp; /* pointer to the first fp inst in the function */
NODE *last_fp;  /* pointer to last fp instruction in the function */

/*next_fp checks for last_fp and returns a special return value. prev_fp
**checks for last_fp. If these function will be called from outside 
**fp_window, these two pointers will be non static and initialized at init().
*/
static NODE fp0,fp1;

NODE *
next_fp(p) NODE *p;
{
NODE *q;
	if (!p) return NULL;
	if (p == last_fp) return &fp1;
	if (p == &fp0) return first_fp;
	if (p == &fp1) return NULL;
	for (q = p->forw; q != &ntail; q = q->forw) {
		if (isfp(q) || break_bb(q)) return q;
	}
	return NULL;
}

NODE *
prev_fp(p) NODE *p;
{
NODE *q;
	if (!p) return NULL;
	if (p == first_fp) return &fp0;
	if (p == &fp1) return last_fp;
	if (p == &fp0) return NULL;
	for (q = p->back; q; q = q->back) {
		if (isfp(q) || break_bb(q)) return q;
	}
	return NULL;
}

static boolean
is_rev_op(op) unsigned int op;
{
	switch (op) {
		case FADD: case FSUB: case FSUBR: case FDIV: case FDIVR: case FMUL:
		case FADDP: case FSUBP: case FSUBRP: case FDIVP: case FDIVRP:
		case FMULP:
		case FSUBL: case FDIVL: case FSUBRL: case FDIVRL:
		case FSUBS: case FDIVS: case FSUBRS: case FDIVRS:
		case FADDL: case FADDS: case FMULL: case FMULS:
			return true;
		default: return false;
	}
	/* NOTREACHED */
}/*end is_rev_op*/

static boolean
is_arit_push(p) NODE *p;
{
	return (is_fp_arit(p) && FPUSH(p->op));
}/*end is_arit_push*/

static boolean
is_arit_pop(p) NODE *p;
{
	return (is_fp_arit(p) && FPOP(p));
}/*end is_arit_pop*/

void
rev_op_add_pop(p) NODE *p;
{
	switch (p->op) {
		case FADD: chgop(p,FADDP,"faddp");   return;
		case FSUB: chgop(p,FSUBRP,"fsubrp"); return;
		case FSUBR: chgop(p,FSUBP,"fsubp");  return;
		case FDIV: chgop(p,FDIVRP,"fdivrp"); return;
		case FDIVR: chgop(p,FDIVP,"fdivp");  return;
		case FMUL: chgop(p,FMULP,"fmulp");   return;
		case FCOM:	chgop(p,FCOMP,"fcomp");	return;
		case FUCOM:	chgop(p,FUCOMP,"fucomp");	return;
		default:
			fatal(__FILE__,__LINE__,"rev_op_add_pop: unexpected op %d\n",p->op);
	}
}/*end rev_op_add_pop*/

void
add_pop(p) NODE *p;
{
	switch (p->op) {
		case FADD: chgop(p,FADDP,"faddp");   return;
		case FSUB: chgop(p,FSUBP,"fsubp"); return;
		case FSUBR: chgop(p,FSUBRP,"fsubrp");  return;
		case FDIV: chgop(p,FDIVP,"fdivp"); return;
		case FDIVR: chgop(p,FDIVRP,"fdivrp");  return;
		case FMUL: chgop(p,FMULP,"fmulp");   return;
		case FST:	chgop(p,FSTP,"fstp");	 return;
		case FCOM:	chgop(p,FCOMP,"fcomp");	return;
		case FUCOM:	chgop(p,FUCOMP,"fucomp");	return;
		default: fatal(__FILE__,__LINE__,"add_pop: unexpected op %d\n",p->op);
	}
}/*end add_pop*/

static void
rev_op(p) NODE *p;
{
	switch (p->op) {
		case FADD: case FADDL: case FADDS: case FMULL: case FMULS:
		case FMUL:  return;
		case FSUB: chgop(p,FSUBR,"fsubr"); return;
		case FSUBR: chgop(p,FSUB,"fsub");  return;
		case FDIV: chgop(p,FDIVR,"fdivr"); return;
		case FDIVR: chgop(p,FDIV,"fdiv");  return;
		case FSUBL: chgop (p, FSUBRL, "fsubrl"); return;
		case FDIVL: chgop (p, FDIVRL, "fdivrl"); return;
		case FSUBRL: chgop (p, FSUBL, "fsubl"); return;
		case FDIVRL: chgop (p, FDIVL, "fdivl"); return;
		case FSUBS: chgop (p, FSUBRS, "fsubrs"); return;
		case FDIVS: chgop (p, FDIVRS, "fdivrs"); return;
		case FSUBRS: chgop (p, FSUBS, "fsubs"); return;
		case FDIVRS: chgop (p, FDIVS, "fdivs"); return;
		default: fatal(__FILE__,__LINE__,"rev_op: unexpected op %d\n",p->op);
	}
}/*end rev_op*/

static void
rem_pop(p) NODE *p;
{
	switch(p->op) {
		case FADDP:	chgop(p,FADD,"fadd"); return;
		case FSUBP:	chgop(p,FSUB,"fsub"); return;
		case FSUBRP:	chgop(p,FSUBR,"fsubr"); return;
		case FMULP:	chgop(p,FMUL,"fmul"); return;
		case FDIVP:	chgop(p,FDIV,"fdiv"); return;
		case FDIVRP:	chgop(p,FDIVR,"fdivr"); return;
		default: fatal(__FILE__,__LINE__,"rem_pop: unexpected op\n");
	}/*end switch*/
	/* NOTREACHED */
}/*end rem_pop*/

static void
rem_pop_s2m(p,len) NODE *p; int len;
{
	switch(p->op) {
		case FADDP:
			switch(len) {
				case LoNG: chgop(p,FADDS,"fadds"); return;
				case DoBL: chgop(p,FADDL,"faddl"); return;
				default:
					fatal(__FILE__,__LINE__,
					"rem_pop: unexpected length, op = %d len = %d\n",p->op,len);
			}
			break;
		case FSUBP:
			switch(len) {
				case LoNG: chgop(p,FSUBS,"fsubs"); return;
				case DoBL: chgop(p,FSUBL,"fsubl"); return;
				default:
					fatal(__FILE__,__LINE__,
					"rem_pop: unexpected length, op = %d len = %d\n",p->op,len);
			}
			break;
		case FSUBRP:
			switch(len) {
				case LoNG: chgop(p,FSUBRS,"fsubrs"); return;
				case DoBL: chgop(p,FSUBRL,"fsubrl"); return;
				default:
					fatal(__FILE__,__LINE__,
					"rem_pop: unexpected length, op = %d len = %d\n",p->op,len);
			}
			break;
		case FMULP:
			switch(len) {
				case LoNG: chgop(p,FMULS,"fmuls"); return;
				case DoBL: chgop(p,FMULL,"fmull"); return;
				default:
					fatal(__FILE__,__LINE__,
					"rem_pop: unexpected length, op = %d len = %d\n",p->op,len);
			}
			break;
		case FDIVP:
			switch(len) {
				case LoNG: chgop(p,FDIVS,"fdivs"); return;
				case DoBL: chgop(p,FDIVL,"fdivl"); return;
				default:
					fatal(__FILE__,__LINE__,
					"rem_pop: unexpected length, op = %d len = %d\n",p->op,len);
			}
			break;
		case FDIVRP:
			switch(len) {
				case LoNG: chgop(p,FDIVRS,"fdivrs"); return;
				case DoBL: chgop(p,FDIVRL,"fdivrl"); return;
				default:
					fatal(__FILE__,__LINE__,
					"rem_pop: unexpected length, op = %d len = %d\n",p->op,len);
			}
			break;
		default:
		 fatal(__FILE__,__LINE__,"rem_pop_s2m: dont know this op %d\n",p->op);
	}
}/*end rem_pop_s2m*/

boolean
is_fcom(NODE *p)
{
	if (p == NULL) return false;
	switch (p->op) {
		case FCOMPP: case FCOM: case FCOMS: case FCOML: case FCOMP:
		case FCOMPS: case FCOMPL:
		case FUCOM: case FUCOMP:
		/* case FCOMPP: case FUCOMPP:      why were these excluded? */
		return true;
		default: return false;
	}
}

void
fix_size(p,length) NODE *p; unsigned int length;
{
	if (length == DoBL )
		switch (p->op) {
			case FSUBS: chgop(p,FSUBL,"fsubl"); return;  
			case FDIVS: chgop(p,FDIVL,"fdivl"); return;  
			case FSUBRS: chgop(p,FSUBRL,"fsubrl"); return;  
			case FDIVRS: chgop(p,FDIVRL,"fdivrl"); return; 
			case FADDS: chgop(p,FADDL,"faddl"); return;  
			case FMULS: chgop(p,FMULL,"fmull"); return; 
			default: return;
		}
	if (length == LoNG )
		switch (p->op) {
			case FSUBL: chgop(p,FSUBS,"fsubs"); return;  
			case FDIVL: chgop(p,FDIVS,"fdivs"); return;  
			case FSUBRL: chgop(p,FSUBRS,"fsubrs"); return;  
			case FDIVRL: chgop(p,FDIVRS,"fdivrs"); return; 
			case FADDL: chgop(p,FADDS,"fadds"); return;  
			case FMULL: chgop(p,FMULS,"fmuls"); return;  
			default: return;
		}
	/* NOTREACHED */
}/*end fix_size */


#ifdef FLIRT
	static int fpeeps=0;
#endif

static boolean
w1fpopt(pf,pl) NODE *pf,*pl;
{
	/* faddp %st,%st(0) -> fstp %st(0) */
	if ((pf->op == FADDP || pf->op == FMULP)
	 && pf->op1
	 && !strcmp(pf->op1,"%st")
	 && pf->op2[4] == '0'
	 ) {
		chgop(pf,FSTP,"fstp");
		pf->op1 = pf->op2;
		pf->op2 = NULL;
		FLiRT(O_FPEEP01,fpeeps);
		return true;
	 }
	 /* fst %st(0) -> delnode */
	 if (pf->op == FST && !strcmp(pf->op1,"%st(0)")) {
		DELNODE(pf);
		return true;
	 }
	 return false;
}/*end w1fpopt*/

extern struct RI *reginfo;

static boolean
w2fpopt(pf,pl) NODE *pf,*pl;
{
char *tmp;
unsigned int cop1 = pf->op;
unsigned int cop2 = pl->op;
	/* fadd %st(1),%st      ->   faddp %st,%st(1)
	** fstp %st(1)
	*/
	if (pf->op == FADD
		&& pf->op1
		&& pf->op1[4] == '1'
		&& pl->op == FSTP
		&& isreg(pl->op1)
		&& pl->op1[4] == '1')
	{
		/*fprintf(stderr,"fpeep 1 "); fprinst(pf); fprinst(pl);*/
		chgop(pf,FADDP,"faddp");
		tmp = pf->op2;
		pf->op2 = pf->op1;
		pf->op1 = tmp;
		DELNODE(pl);
		/*fprintf(stderr,"to "); fprinst(pf);*/
		FLiRT(O_FPEEP02,fpeeps);
		return true;
	}
	/* fop  %st(i),%st   fop_p %st,%st(i)
	** fstp %st(i)
	*/
	if (!eiger
		&& pl->op == FSTP
		&& isreg(pf->op1)
		&& isreg(pf->op2)
		&& is_fp_arit(pf)
		&& !strcmp(pl->op1,pf->op1)
	) { char *swap;
		wchange();
		swap = pf->op1;
		pf->op1 = pf->op2;
		pf->op2 = swap;
		add_pop(pf);
		DELNODE(pl);
		return true;
	}
	/* fld %st(i)       -> delnode
	** fop_p %st,%st(1) -> fop st(i),%st
	*/
	if (!eiger
		&& cop1 == FLD
		&& is_arit_pop(pl)
		&& !strcmp(pl->op2,"%st(1)")
	) {
		wchange();
		pl->op1 = pf->op1;
		pl->op2 = "%st";
		DELNODE(pf);
		rem_pop(pl);
		rev_op(pl);
		return true;
	}
	/* fld %st(0)
	** fop_p %st,%st(i) -> fop_r %st,%st(i-1)
	*/
	if (!eiger
		&& cop1 == FLD
		&& is_rev_op(cop2)
		&& !strcmp(pf->op1,"%st(0)")
		&& !strcmp(pl->op1,"%st")
	) { int i = pl->op2[4] - '0';
		if (i == 0) goto opt2_out;
		wchange();
		DELNODE(pf);
		rem_pop(pl);
		pl->op2 = st_i[i-1];
		return true;
	}
opt2_out:
	/* fop	%st,%st(i)	-> fop_r_p	%st(i),%st
	** fstp	%st(i)
	*/
	if (pl->op == FSTP
		&& isreg(pf->op1)
		&& isreg(pf->op2)
		&& is_rev_op(pf->op)
		&& !FPOP(pf)
		&& !strcmp(pl->op1,pf->op2))
	{
		char *tmp;
		/*fprintf(stderr,"fpeep 2 "); fprinst(pf); fprinst(pl);*/
		rev_op_add_pop(pf);
		tmp = pf->op1;
		pf->op1 = pf->op2;
		pf->op2 = tmp;
		DELNODE(pl);
		/*fprintf(stderr,"to "); fprinst(pf);*/
		FLiRT(O_FPEEP03,fpeeps);
		return true;
	}
	/* fld mem
	** fop_p	%st,%st(1)   -> fop mem
	*/
	if ((pf->op == FLDS || pf->op == FLDL || pf->op == FLDT)
		&& !isreg(pf->op1)
		&& isreg(pl->op1)
		&& isreg(pl->op2)
		&& is_arit_push(pl)
		&& !changed_between(pf,pl))
	{
		/*fprintf(stderr,"fpeep 3 "); fprinst(pf); fprinst(pl);*/
		pl->op2 = NULL;
		pl->op1 = pf->op1;
		rem_pop_s2m(pl,OpLength(pf));
		DELNODE(pf);
		/*fprintf(stderr,"to "); fprinst(pl);*/
		FLiRT(O_FPEEP04,fpeeps);
		return true;
	}
	/* fld	%st(0)
	** fstp	mem       ->  fst	mem
	*/
	if (pf->op == FLD
		&& isreg(pf->op1)
		&& pf->op1[4] == '0'
		&& pl->op == FSTP
		&& !isreg(pl->op1))
	{
		/*fprintf(stderr,"fpeep 4 "); fprinst(pf); fprinst(pl);*/
		chgop(pl,FST,"fst");
		DELNODE(pf);
		/*fprintf(stderr,"to "); fprinst(pl);*/
		FLiRT(O_FPEEP05,fpeeps);
		return true;
	}
	/* fld	%st(0)
	** fstp	%st(i) i > 0  -> fst %st(i-1)
	*/
	if (pf->op == FLD
		&& pl->op == FSTP
		&& pl->op1[4] != '0' )
	{
		char *tmp = getspace(6);
		int i = pl->op1[4] - '0';
		/*fprintf(stderr,"fpeep 5 "); fprinst(pf); fprinst(pl);*/
		sprintf(tmp,"%%st(%d)",i-1);
		pl->op1 = tmp;
		DELNODE(pf);
		/*fprintf(stderr,"to "); fprinst(pl);*/
		FLiRT(O_FPEEP06,fpeeps);
		return true;
	}
	/* fstp mem 		->  fst mem
	** fld	mem
	*/
	if (pf->op == FSTPS
		&& pl->op == FLDS
		&& !strcmp(pf->op1,pl->op1)
		&& !changed_between(pf,pl))
	{
		/*fprintf(stderr,"fpeep 6 "); fprinst(pf); fprinst(pl);*/
		chgop(pf,FSTS,"fsts");
		DELNODE(pl);
		/*fprintf(stderr,"to "); fprinst(pf);*/
		FLiRT(O_FPEEP07,fpeeps);
		return true;
	}
	if (pf->op == FSTPL
		&& pl->op == FLDL
		&& !strcmp(pf->op1,pl->op1)
		&& !changed_between(pf,pl))
	{
		/*fprintf(stderr,"fpeep 7 "); fprinst(pf); fprinst(pl);*/
		chgop(pf,FSTL,"fstl");
		DELNODE(pl);
		/*fprintf(stderr,"to "); fprinst(pf);*/
		FLiRT(O_FPEEP08,fpeeps);
		return true;
	}
	/* fld %st(0)
	** fop_p %st(1),%st  fop %st(0),%st
	*/
	if (pf->op == FLD
		&& pf->op1[4] == '0'
		&& is_arit_pop(pl)
		&& isreg(pl->op1)
		&& isreg(pl->op2)
		&& pl->op1[4] == '1')
	{
		/*fprintf(stderr,"fpeep 8 "); fprinst(pf); fprinst(pl);*/
		DELNODE(pf);
		rem_pop(pl);
		pl->op1 = "%st(0)";
		/*fprintf(stderr,"to "); fprinst(pl);*/
		FLiRT(O_FPEEP09,fpeeps);
		return true;
	}
	/*
	** fsts mem1    fstps mem1
	** fstps mem2   movl mem1,%eax
	** 				movl %eax,mem2
	*/
	if (target_cpu != P6
	 && pf->op == FSTS
	 && pl->op == FSTPS
	) {
		int i;
		char *tmp;
		NODE *pnew;

		i = get_free_reg(pf->nlive,0,pf);
		if (i == -1 ) return false;
		tmp = reginfo[i].regname[0];
		chgop(pf,FSTPS,"fstps");
		chgop(pl,MOVL,"movl");
		pl->op2 = tmp;
		pnew = insert(pl);
		chgop(pnew,MOVL,"movl");
		pnew->op1 = tmp;
		FLiRT(O_FPEEP10,fpeeps);
		pnew->op2 = pl->op1;
		pl->op1 = pf->op1;
		new_sets_uses(pl);
		new_sets_uses(pnew);
	}
	return false;
}/*end w2fpopt*/

static boolean
w3fpopt(pf,pl) NODE *pf,*pl;
{
NODE *p2 = next_fp(pf);
unsigned int cop1 = pf->op;
unsigned int cop2 = p2->op;
unsigned int cop3 = pl->op;
opopcode opop;
int x;
	/* fstp mem2                fst mem2
	** fld op1         -->
	** fop mem2                 fop_r op1
	*/
	if ((cop1 == FSTPS || cop1 == FSTPL)
		&& (cop2 == FLD  || cop2 == FLDL || cop2 == FLDS)
		&& is_rev_op(cop3)
		&& !strcmp(pf->op1,pl->op1)
		&& !changed_between(pf,pl))
	{
		/*fprintf(stderr,"fpeep 9 "); fprinst(pf); fprinst(p2); fprinst(pl);*/
		if (cop1 == FSTPS) chgop(pf,FSTS,"fsts");
		else if (cop1 == FSTPL) chgop(pf,FSTL,"fstl");
		rev_op(pl);
		if (p2->op == FLD) {
			opop = mem2st(pl->op);
			chgop(pl,opop.op,opop.op_code);
			x = p2->op1[4] -'0' +1; /*cant be st7, its invalid after the fstp*/
			pl->op1 = st_i[x];
			pl->op2 = "%st";
		} else {
			pl->op1 = p2->op1;
			if (OpLength(pl) != OpLength(p2))
				fix_size(pl,OpLength(p2));
		}
		DELNODE(p2);
		/*fprintf(stderr,"to "); fprinst(pf); fprinst(pl);*/
		FLiRT(O_FPEEP11,fpeeps);
		return true;
	}
	return false;
}/*end w3fpopt*/

static boolean
w4fpopt(pf,pl) NODE *pf,*pl;
{
NODE *p2 = next_fp(pf);
NODE *p3 = next_fp(p2);
unsigned int cop1 = pf->op;
unsigned int cop2 = p2->op;
unsigned int cop4 = pl->op;
boolean live_at();

	/* fstpl REGAL   DELNODE
	** any f_load    no change
	** any fp_arit    no change
	** fcompl REGAL   fcompp
	*/
	if (cop1 == FSTPL 
		&& FPUSH(cop2)
		&& cop4 == FCOMPL
		&& is_any_farit(p3)
		&& !FPOP(p3) /* defensive, should never happen */
		&& ISREGAL(pf->op1)
		&& !strcmp(pf->op1,pl->op1)
		&& !live_at(pl,pl)
	) {
		DELNODE(pf);
		chgop(pl,FCOMPP,"fcompp");
		pl->op1 = NULL;
		FLiRT(O_FPEEP12,fpeeps);
		return true;
	}
	return false;
}/*end w4fpopt*/

static void
fp_window(size, func) register int size; boolean (*func)();
{
static NODE *initfpw();
NODE *last;
int i;
	first = last = NULL;
	if (first_fp == NULL) return;
	/* initialize special nodes */
	fp0.forw = first_fp; fp1.back = last_fp;
	fp0.back = NULL; fp1.forw = NULL;
	/* find first window */
	if ((last = initfpw(first_fp,size)) == NULL) return;
	/* move window through code */
	for (opf = prev_fp(first); ; opf = prev_fp(first)) {
		COND_SKIP(goto advance,"w%d %d",size,second_idx,0);
		if ((*func)(first, last) == true) {
			if (size > 1) {
			/* move window back in case there is an overlapping improvement */
				for (i = 2; i <= size; i++)
					if ((opf = prev_fp(opf)) == &fp0) {
						break;
					}
				if ((last = initfpw(opf,size)) == NULL)
					return;
				continue;
			}/*endif size > 1*/
		}/*endif func == true*/
		/* move window ahead */
#ifdef DEBUG
		advance:
#endif
		if ((last = next_fp(last)) == &fp1) return;
		first = next_fp(first);
		if (!first || !last) return;
		if (break_bb(last) && (last = initfpw(next_fp(last),size)) == NULL)
			return;
	}/*for loop*/
}/*end fp_window*/

/* find first available window, p is first in the window. */
static NODE *
initfpw(p,size) register NODE *p; int size;
{
int i;
	if ((first = p) == NULL) return NULL;
	/* move p down until window is large enough */
	for (i = 1; i <= size; i++) {
		if (p == &fp1) /* no more windows */
			return NULL;
		if (break_bb(p)) { /* restart scan */
			first = next_fp(p);
			i = 0;
		}
		p = next_fp(p);
	}
	return prev_fp(p);
}/*end initfpw*/

extern unsigned int muses(), msets(); /* in sched.c */

void
fpeep()
{
NODE *p,*q,*r;
NODE *p_forw = NULL;
unsigned int qcop;
int m,x;
int counter;

	COND_RETURN("fpeep");

#ifdef FLIRT
	fpeeps++;
#endif

	find_first_and_last_fps();
	if (!eiger) {
		fp_window(4,w4fpopt);
	}
	if (!eiger) {
		fp_window(3,w3fpopt);
	}
	fp_window(2,w2fpopt);
	fp_window(3,w3fpopt);
	fp_window(1,w1fpopt);
	/* window of unknown size */
	for (p = n0.forw; p != &ntail; p = p_forw) {
		p_forw = p->forw;
		if (break_bb(p)) {
			continue;
		}
		if (!isfp(p)) {
			continue;
		}
		COND_SKIP(continue,"wd %d %s %s ",second_idx,p->opcode,p->op1);
		if (   p->op == FLD
		    && !strcmp(p->op1,"%st(0)")) {
			for (q = p->forw; q != &ntail ; q = q->forw) {
				qcop = q->op;
				if (break_bb(q)) {
					break;
				}
				if (!isfp(q)) {
					continue;
				}
				if (FNOST(q->op)) {
					continue;
				}
				if (   q->op == FSTP
				    && !strcmp(q->op1,"%st(1)")) {
					for (r = p->forw; r != q; r = r->forw) {
						if (isfp(r)) {
							for (m = 1; m <= 2; m++) {
								if (   r->ops[m]
								    && !strncmp(r->ops[m],"%st(",4)) {

									x = r->ops[m][4] - '0';
									if (x > 0) {
										x--;
										r->ops[m] = getspace(6);
										sprintf(r->ops[m],"%%st(%d)",x);
									}
								}
							}
						}
					}
					DELNODE(p);
					FLiRT(O_FPEEP,fpeeps);	
					DELNODE(q);
					break;
				} else {
					if (   FPUSH(qcop)
					    || FPOP(q)
					    || qcop == FCOMPP
					    || qcop == FUCOMPP ) {

						break;
					}
				}
			}
		} /* end peephole */

		if (p->op == FSTP && p->op1[4] == '0') {
			counter = 0;
			for (q = p->back; q != &n0; q = q->back) {
				if (break_bb(q)) {
					break;
				}
				if (!isfp(q)) {
					continue;
				}
				if (   !is_fp_arit(q)
				    && !is_fld(q)
				    && !is_fst(q)) {
					break;
				}
				if (   (msets(q) & MEM)
				    && live_at(q,p)) {
					break;
				}
				if (FPOP(q)) {
					counter++;
				}
				if (   q->sets
				    && q->sets != FP0) {
					break;
				}
				if (is_fld(q)) {
					if (counter) {
						counter--;
					} else {
						for (r = q; r != p->forw; r = r->forw)
							if(isfp(r)) {
								FLiRT(O_FPEEP,fpeeps);
								DELNODE(r);
							}
						break;
					}
				}
			}
		} /* end peephole */

		if (!eiger && p->op == FST) {
			/*remove redundant fst, before a next fstp*/
			for (q = p->forw; !break_bb(q); q = q->forw) {
				if (   isfp(q)
				    && q->op != FSTP
				    && !is_fp_arit(q)) {
					break;
				}
				if (   is_fp_arit(q)
				    && (sets(q) & (FP1|FP2))) {
					/*only 0,1,2 are modeled*/
					if (   !strcmp(q->op1,p->op1)
					    || !strcmp(q->op2,p->op1))  {
						/*sets the operand of fst*/
						break;
					}
				}
				if (   q->op == FSTP
				    && !strcmp(q->op1,p->op1)) {
					DELNODE(p);
					break;
				}
			}
		}
		if (   !eiger
		    && p->op == FLD
		    && !strcmp(p->op1,"%st(1)")) {
			for (q = p->forw; !break_bb(q); q = q->forw) {
				if (   isfp(q)
				    && q->op != FSTP
				    && !is_fp_arit(q)) {
					break;
				}
				if (is_fp_arit(q)) {
					if (muses(q) & MEM) {
						break;
					}
					if ( ! (   (   !strcmp(q->op1,"%st")
					            && !strcmp(q->op2,"%st(1)"))
					        || (   !strcmp(q->op1,"%st(1)")
					            && !strcmp(q->op2,"%st")))) {
						break;
					}
				}
				if (q->op == FSTP) {
					if (strcmp(q->op1,"%st(2)")) {
						break;
					} else {
						for (r = p->forw; r != q; r = r->forw) {
							char *swap = r->op1;
							r->op1 = r->op2;
							r->op2 = swap;
						}
						DELNODE(p);
						DELNODE(q);
						break;
					}
				}
			}
		} /* end fld st(1) peephole */

		if (!eiger && p->op == FLD) {
			/* fld st(i)                 -> fxch st(i)
			** several fp-arit st(i),st  -> fp-arit st(j-1),st
			** fstp st(i+1)              -> fxch st(i)
			*/
			int i = p->op1[4] - '0';
			if (   i == 0
			    || i == 1) {
				continue;
			}
			for (q = p->forw; !break_bb(q); q = q->forw) {
				if (isfp(q)) {
					if (   (q->op != FSTP)
					    && !is_fp_arit(q)) {
						/* don't optimize*/
						break;
					}
					if (is_fp_arit(q)) {
						char *iop;

						if (muses(q) & MEM) {
							continue;
						}
						if (!strcmp(q->op1,"%st")) {
							iop = q->op2;
						} else if (!strcmp(q->op2,"%st")) {
							iop = q->op1;
						} else {
							/*dont optimize*/
							break;
						}
						if (iop[4] == '0') {
							/*cant work off of st(0)*/
							break;
						}
					}
					if (q->op == FSTP) {
						if (q->op1[4] - '0' != i+1) {
							break;
						} else {
							for (r = p->forw; r != q; r = r->forw) {
								char *iop;
								int j;
								int m;
								if (muses(r) & MEM) {
									continue;
								}
								if (is_fp_arit(r)) {
									if (!strcmp(r->op1,"%st")) {
										iop = r->op2;
										m = 2;
									} else {
										iop = r->op1;
										m = 1;
									}
									j = iop[4] - '0';
									r->ops[m] = st_i[j-1];
								}
							}
							chgop(p,FXCH,"fxch");
							chgop(q,FXCH,"fxch");
							q->op1 = st_i[i];
							break;
						}
					}
				}
				
			}
		} /* end FLD peephole */
	} /* for all nodes */

	sets_and_uses();
} /* end fpeep */

/* The first instruction containes a memory operand. This function return
** 1 if this operand is changed between the first and the second instructions,
** and 0 if not.
*/
static boolean
changed_between(p1,p2) NODE *p1, *p2;
{
NODE *p;
char *operand = NULL;
unsigned int regs;
	/* find the operand and do sanity checks */
	if (ismem(p1->op1)) operand = p1->op1;
	else if (ismem(p1->op2)) operand = p1->op2;
	else fatal(__FILE__,__LINE__,"change_between_ops: no memory reference\n");
	regs = scanreg(operand,false);
	for (p = p1->forw; p != p2; p = p->forw) {
		if (p->sets & regs) return true;
		if (msets(p) & MEM) {
			if (p->op1 && ismem(p->op1) && !strcmp(operand,p->op1)) return true;
			if (p->op2 && ismem(p->op2) && !strcmp(operand,p->op2)) return true;
		}
	}
	return false;
}/*end changed_between*/

void
rm_dead_insts()
{
NODE *pf;
unsigned int regs_set;
char *dp;

#ifdef FLIRT
	static int numexec=0;
#endif

	COND_RETURN("rm_dead_insts");

#ifdef FLIRT
	++numexec;
#endif

	for (ALLN(pf)) {
        if (!fixed_stack)
            if (pf->op == MOVL && pf->uses == ESP && pf->sets == EBP)
                continue;
		if (!((regs_set=sets(pf)) & pf->nlive)	/* all regs set dead? */
			&&  regs_set != 0		/* leave instructions that don't
						** set any regs alone */
			&&  !(regs_set & (FP0 | FP1 | FP2 | FP3 | FP4 | FP5 | FP6 |FP7))
					/* don't mess with fp instr */
			&&  ! isbr(pf)			/* some branches set variables
					** and jump:  keep them */
			&&  (isdead(dp=dst(pf),pf) ||	/* are the destination and ... */
	     		(!*dp && (
	      		pf->op == TESTL || pf->op == CMPL || pf->op == TESTW ||
	      		pf->op == TESTB || pf->op == CMPW || pf->op == CMPB ||
	      		pf->op == SAHF)))  /* maybe SETcc when implemented? */
			&&  (pf->op1 == NULL || !isvolatile( pf,1 )) /* are operands non- */
			&&  (pf->op2 == NULL || !isvolatile( pf,2 )) /* volatile? */
			&&  (pf->op3 == NULL || !isvolatile( pf,3 ))
		)
    	{
			ldelin2(pf);			/* preserve line number info */
			mvlivecc(pf);			/* preserve condition codes line info */
			FLiRT(O_RM_DEAD_INSTS,numexec);
			DELNODE(pf);			/* discard instruction */
    	}
	}
}/*end rm_dead_insts*/
