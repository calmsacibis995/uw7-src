#ident	"@(#)optim:i386/loops.c	1.44.1.44"

#include <stdio.h>
#include <values.h>
#include <assert.h>
#include "sched.h"
#include "optutil.h"
#include "fp_timings.h"
#include "regal.h"
#ifdef DEBUG
#define  STDERR	stderr
#define  FPRINST(P) fprinst(P)
#define	loop_of()	{ fprintf(STDERR,"loop of "); FPRINST(first); }
#endif

#define ALL_THE_LOOP(P)	P = first; P != last->forw; P = P->forw

extern NODE *prepend();	/* imull.c */
extern char *itoreg();	/* ebboptim.c */
extern live_at();		/* ebboptim.c */
extern NODE *next_fp(), *prev_fp();	/* peep.c */
extern int suppress_enter_leave;
extern int numauto;
extern unsigned full_reg();
extern void move_node();
int zflag = false; /*debugging flag*/
static NODE *first, *last; /*first and last insts. in the loop */
static BLOCK *first_block; 
static boolean  simple_done=false;
static void regals_2_index_ops();
static int index_ops_2_regals();

#define regs   ( EAX | EBX | ECX | EDX | ESI | EDI | EBI | ESP | EBP )
#define scratch_regs	( EAX | ECX | EDX )

#define LOOP_ENTRY 1
#define LOOP_EXIT  2
#define ERROR     -1
#define MAX_CAND	100
/* three defs from regal.c */
#define MAXLDEPTH	10
#define MAXWEIGHT	(MAXINT - 1000)
#define WEIGHT	8

#define i_eax 0
#define i_edx 1
#define i_ecx 2
#define i_ebx 3
#define i_esi 4
#define i_edi 5
#define i_ebi 6

#define first_reg 0
#define last_reg 6

struct m { unsigned int op;
			char *opc;
			}; 
static struct m moves[] = { { MOVB , "movb" } , 
						  { MOVW , "movw" } ,
						  { 0    , NULL   } ,  /* placeholder */
						  { MOVL , "movl" } };

typedef struct one_r {
		unsigned int reg;
		char *names[4]; /* three names and place holder to make it oplength */
		int status;
		int save_at;
	} one_reg;

/* possible values of status */

#define LIVE	0
#define FREE	1
#define NOT_USED	2

static one_reg reg_all_regs[] = {
        /*   reg    names[0]  names[1]       names[3]   status  save_at  */
            { EAX , { "%al" , "%ax" , NULL, "%eax" } , FREE , 0 } ,
            { EDX , { "%dl" , "%dx" , NULL, "%edx" } , FREE , 0 } ,
            { ECX , { "%cl" , "%cx" , NULL, "%ecx" } , FREE , 0 } ,
            { EBX , { "%bl" , "%bx" , NULL, "%ebx" } , FREE , 0 } ,
            { ESI , { NULL  , "%si" , NULL, "%esi" } , FREE , 0 } ,
            { EDI , { NULL  , "%di" , NULL, "%edi" } , FREE , 0 } ,
            { EBI , { NULL  , "%bi" , NULL, "%ebi" } , FREE , 0 } ,
        };

static one_reg fix_all_regs[] = {
        /*   reg    names[0]  names[1]       names[3]   status  save_at  */
            { EAX , { "%al" , "%ax" , NULL, "%eax" } , FREE , 0 } ,
            { EDX , { "%dl" , "%dx" , NULL, "%edx" } , FREE , 0 } ,
            { ECX , { "%cl" , "%cx" , NULL, "%ecx" } , FREE , 0 } ,
            { EBX , { "%bl" , "%bx" , NULL, "%ebx" } , FREE , 0 } ,
            { ESI , { NULL  , "%si" , NULL, "%esi" } , FREE , 0 } ,
            { EDI , { NULL  , "%di" , NULL, "%edi" } , FREE , 0 } ,
            { EBP , { NULL  , "%bp" , NULL, "%ebp" } , FREE , 0 } ,
        };

static one_reg *all_regs ;

typedef struct icand_s {
		char *name;			/* the char string, for any type. */
		int frame_offset;		/* only for regals */
		int value;			/* only for immediates */
		int estim;			/* estimated payoff */
		boolean in_byte_inst; /* does it appear with oplength == 1 */
		int reg_assigned;	/* which reg is assigned to the cand */
		boolean changed;	/* to restore after the loop or not */
		int length;			/* oplength of the restore */
	} i_cand;

static i_cand all_icands[MAX_CAND];
static i_cand sorted_icands[MAX_CAND];


static i_cand null_icand = { NULL , 0 , 0 , 0 , 0 , -1 , 0 , 0 };

/*is p a conditional jump backwards*/
/*return the jump target if it is, NULL otherwise*/
NODE *
is_back_jmp(p) NODE *p;
{
NODE *q;
	if (!iscbr(p)) return NULL;
	for (q = p; q != &n0; q = q->back) /* search for label backwards */
		if (islabel(q) && !strcmp(p->op1,q->opcode))
			return q;
	return NULL;
}/*end is_back_jmp*/

NODE *
find_last_jmp(given_jmp) NODE *given_jmp;
{
NODE *p;
	for (p = ntail.back; p != given_jmp->back; p = p->back)
		if (iscbr(p) && !strcmp(p->op1,given_jmp->op1))
			return p;
	fatal(__FILE__,__LINE__,"last jump: not found even it'self\n");
	/* NOTREACHED */
}/*end find_last_jmp*/

/*Find what registers are free in the loop.
**Mark the free and nonfree ones in all_regs[].
**return number of free regs.
*/
static unsigned int
find_available_regs_in_loop()
{
int i;
NODE *p;
unsigned int live_in_loop = 0;
unsigned int used_in_loop = 0;
unsigned int available_regs;
unsigned int saveable_regs;
int free_regs = 0;
int working_regs;


	working_regs =( EAX | EBX | ECX | EDX | ESI | EDI |  (fixed_stack ? EBP : EBI) );
	if (suppress_enter_leave) {
		live_in_loop |= EBI;
		used_in_loop |= EBI;
		all_regs[i_ebi].status = LIVE;
	}
	for (ALL_THE_LOOP(p)) {
		live_in_loop |= (p->nlive | p->sets | p->uses);
		used_in_loop |= (p->sets | p->uses);
	}/*for loop*/
	live_in_loop &= working_regs;
	used_in_loop &= working_regs;
	available_regs = working_regs & ~live_in_loop;
	saveable_regs =  working_regs & (live_in_loop & ~used_in_loop);
	for (i = first_reg; i <= last_reg; i++) {
		if ((available_regs & all_regs[i].reg) == all_regs[i].reg) {
			all_regs[i].status = FREE;
			free_regs++;
		} else if ((saveable_regs & all_regs[i].reg) == all_regs[i].reg) {
			all_regs[i].status = NOT_USED;
			free_regs++;
		} else {
			all_regs[i].status = LIVE;
		}
	}
	return free_regs;
}/* end find_available_regs_in_loop*/

/*If the original memory operand is used as an eight bit operand than it
**is not replaceable with neither one of the three registers edi, esi or
**ebp. These registers are the last ones to be chosen, therefore no other
**register is available.
*/
static boolean
reg_8_probs(entry) int entry;
{
i_cand *rgl = &sorted_icands[entry];
char *operand = rgl->name;
NODE *p;
	for (ALL_THE_LOOP(p)) {
		if (OpLength(p) == ByTE) {
			if ((p->op1 && !strcmp(operand,p->op1))
				|| (p->op2 && !strcmp(operand,p->op2)))
				return true;
		}
	}
	return false;
}/*end reg_8_probs*/


/* A loop is considered simple if it has only one entery point,
** and either only one exit (by fallthrough), or some jmps, all
** to the same target.
** Three possible return values: 0 for non optimizable, 1 if only
** one exit, 2 if there are "legal" jumps out of the loop.
**
** In case of several jumps to one target, which is out of the loop,
** return via jtarget, the name of the label.
**
** A bug in this test: the jumps may be to the same block but to 
** some different but succesive labels. Currently this will return 0.
*/
static int /*not boolean*/
is_simple_loop(jtarget) char **jtarget;
{
  extern SWITCH_TBL *get_base_label();
  NODE *p,*q;
  boolean label_found = false;
  boolean first_lbl = true;
  char *target = NULL;
  SWITCH_TBL *sw;
  REF *r;
  BLOCK *b;
	/* Special case: if there is a wild jump in the function, of the form
	** "jmp	*%eax", then no loop is simple.
	*/
	for (ALLN(p)) {
		if (   p->op == JMP
		    && *(p->op1) == '*' 
		    && !(   p->op1[1] == '.'
		         || isalpha(p->op1[1])
		         || p->op1[1] == '_')) {
			return 0;
		}
	}
	/* First test that any jump from inside the loop stays inside. */
	/* In particular, if there is a switch in the loop, verify that
	** all of the targets are inside the loop.
	*/
	for (ALL_THE_LOOP(p)) {
		if (is_jmp_ind(p)) {
			sw = get_base_label(p->op1);
			for (r = sw->first_ref ; r != sw->last_ref; r = r->nextref) {
				b = r->switch_entry;
				label_found = false;
				for (ALL_THE_LOOP(q)) {
					if (q == b->firstn) {
						label_found = true;
					}
				}
				if (!label_found) {
					return 0;
				}
			}
		} else if (   is_any_br(p)
		           && p->op1) {
			label_found = false;
			for (ALL_THE_LOOP(q)) {
				if (   is_any_label(q)
				    && !strcmp(p->op1,q->opcode)) {
					label_found = true;
					break;
				}
			}
			if (!label_found) {
				/* The jmp target is ! inside the loop. */
				if (first_lbl) {
					first_lbl = false;
					/*save the first such jmp target*/
					target = p->op1;
				} else {
					/* If this target != first target,
					   it's ! simple. */
					if (strcmp(target,p->op1)) {
						return 0;
					}
				}
			}
		}
	}
	/* Second, test that jumps from before the loop dont enter. */
	for (p = n0.forw; p != first; p = p->forw) {
		if (   is_any_br(p)
		    && p->op1) {
			label_found = false;
			for (ALL_THE_LOOP(q)) {
				if (   is_any_label(q)
				    && !strcmp(p->op1,q->opcode)) {
					return 0;
				}
			}
		}
	}
	/*Third, test that jumps from after the loop dont enter. */
	for (p = last->forw; p != &ntail; p = p->forw) {
		if (   is_any_br(p)
		    && p->op1) {
			label_found = false;
			for (ALL_THE_LOOP(q)) {
				if (   is_any_label(q)
				    && !strcmp(p->op1,q->opcode)) {
					return 0;
				}
			}
		}
	}
	*jtarget = target;
	return target == NULL ? 1 : 2;

} /* end is_simple_loop */

/* The memory reference in pivot tries to disambiguate by seems_same() from
** any other memory reference between first and last. Keep your fingers crossed,
** seems_same assumes that it's second parameter is earlier than it's first one.
** Before that test that the registers used by pivot are not set anywhere in
** the loop.
*/
static boolean
disambiged(NODE *pivot)
{
  NODE *q;
  char *operand;
  char *rival;
  unsigned int regset = 0;
  boolean retval = true; /* cant return from middle of the function */

	/* Find which operand of pivot is the memory reference.
	   point with operand. */
	if (   !isreg(pivot->op1)
	    && !isconst(pivot->op1)) {
		operand = pivot->op1;
	} else if (   !isreg(pivot->op2)
	           && !isconst(pivot->op2)) {
		operand = pivot->op2;
        } else {
                operand = pivot->op3;
	}
	if (*operand == '*') {
		++operand;
	}

	/* If this is a read only data item, it disambigs from anything */
	if (is_read_only(operand)) {
		return true;
	}

	/* Does pivot index thru a register which is changed inside the loop? */
	for (ALL_THE_LOOP(q)) {
		regset |= q->sets;
	}
	regset &= regs;
	if (scanreg(operand,false) & regset) {
		return false;
	}

	/* add memory info to the NODEs */
	for (ALL_THE_LOOP(q)) {
		/*add memory knowledge*/
		q->uses |= muses(q);
		q->sets = msets(q);
		q->idus = indexing(q);
	} /* for loop */
	/* do from first to pivot */
	for (q = first; q != pivot; q = q->forw) {
		if ((q->sets | q->uses) & MEM) {
			/* Check operand 1. */
			if (q->op1 && !isreg(q->op1)) {
				if (q->op1[0] != '*') {
					rival = q->op1;
				} else {
					rival = q->op1+1;
				}
				if (!strcmp(rival,operand)) {
					continue;
				}
			}
			if (q->op2 && !isreg(q->op2)) {
				if (q->op2[0] != '*') {
					rival = q->op2;
				} else {
					rival = q->op2+1;
				}
				if (!strcmp(rival,operand)) {
					continue;
				}
			}
			if (q->op3 && !isreg(q->op3)) {
				if (q->op3[0] != '*') {
					rival = q->op3;
				} else {
					rival = q->op3+1;
				}
				if (!strcmp(rival,operand)) {
					continue;
				}
			}
			if (seems_same(pivot,q,false)) {
				retval = false;
				break;
			}
		}
	}
	/* do from pivot to last */
	if (retval) {
		for (q = pivot->forw; q != last->forw; q = q->forw) {
			if ((q->sets | q->uses) & MEM) {
				if (q->op1 && !isreg(q->op1)) {
					if (q->op1[0] != '*') {
						rival = q->op1;
					} else {
						rival = q->op1+1;
					}
					if (!strcmp(rival,operand)) {
						continue;
					}
				}
				if (q->op2 && !isreg(q->op2)) {
					if (q->op2[0] != '*') {
						rival = q->op2;
					} else {
						rival = q->op2+1;
					}
					if (!strcmp(rival,operand)) {
						continue;
					}
				}
				if (q->op3 && !isreg(q->op3)) {
					if (q->op3[0] != '*') {
						rival = q->op3;
					} else {
						rival = q->op3+1;
					}
					if (!strcmp(rival,operand)) {
						continue;
					}
				}
				if (seems_same(q,pivot,false)) {
					retval = false;
					break;
				}
			}
		}
	}
	/* remove memory info from NODEs */
	for (ALL_THE_LOOP(q)) {
		q->uses &= ~MEM;
		q->sets &= ~MEM;
	}
	return retval;
} /* end disambiged */

/*operand is taken from a NODE. base is either all_icands or sorted_icands.
**find the structure of the operand in base[] and return a pointer to it.
**I wanted to look for immediates by value but there is a problem: if there
**is a search for $0, it will find a global variable with name != NULL and
**value == 0 and return it. must check for strcmp with "$0".
**do it only for $0, save the time for other values.
*/
static i_cand*
lookup_icand(operand,base) char *operand; i_cand *base;
{
i_cand *scan;
int i;
boolean by_off = scanreg(operand,true) & frame_reg;
boolean by_imm = isimmed(operand);
boolean is_param = fixed_stack && !by_imm && by_off && !strstr(operand,".FSZ");
int x;

	if (!operand) return NULL;
	if (is_param) return NULL;
	if (by_off) 
	{
		if( *operand == '*' ) operand++;
		x= (int)strtoul(operand,(char **)NULL,0);
	}
	else if (by_imm) x = (int)strtoul(1+operand,(char **)NULL,0);
	else x = 0;
	for (i = 0; i < MAX_CAND; i++) {
		scan = &base[i];
		if (by_off) {
            if (scan->name != NULL && x == scan->frame_offset
			&& (scanreg(scan->name,false) & frame_reg)
			&& (!fixed_stack || !strcmp(scan->name,operand))) {
				return scan;
			}
		} else if (by_imm) {
			if (isimmed(scan->name) && x == scan->value)
				return scan;
		} else {
			if (scan->name && !strcmp(operand,scan->name)) {
				return scan;
			}
		}
	}
	return NULL;
}/*end lookup_icand*/

/*function returns the number of cycles gained by using a register instead
**of memory in the given instruction.
**In a mov instruction there is no direct gain, but it is probable that
**the instruction will be deleted. It's gain should be half, rether than 1.
*/
static int 
pay_off(p) NODE *p;
{
	switch (p->op) {
		case ADDB: case ADDW: case ADDL:
		case SUBB: case SUBW: case SUBL:
		case ANDB: case ANDW: case ANDL:
		case ORB:  case ORW:  case ORL:
		case XORB: case XORW: case XORL:
			return msets(p) & MEM ? 2 : 1;

		case NEGB: case NEGW: case NEGL:
		case NOTB: case NOTW: case NOTL:
		case DECB: case DECW: case DECL:
		case INCB: case INCW: case INCL:
			return 2;

		case RCLB: case RCLW: case RCLL:
		case RCRB: case RCRW: case RCRL:
		case ROLB: case ROLW: case ROLL:
		case RORB: case RORW: case RORL:
		case SALB: case SALW: case SALL:
		case SARB: case SARW: case SARL:
		case SHLB: case SHLW: case SHLL:
		case SHRB: case SHRW: case SHRL:
			if (target_cpu == P4 ) {
				if (   (setreg(p->op1) == CL)
				    || (p->op2 == NULL)) {
					return 1;
				}  /* if */
			} else {
				/* All other processor modes */
				if (setreg(p->op1) == CL) {
					return 0;
				}  /* if */
			}  /* if */
			return 2;

		case SHLDW: case SHLDL: case SHRDW:
		case SHRDL:
			if (   target_cpu == P4
			    || p->op3 == NULL /* %CL form */) {
				return 1;
			}  /* if */
			return 0;

		case CMPB:  case CMPW:  case CMPL:
		case TESTB: case TESTW: case TESTL:
			return 1;

		case POPW: case POPL:
			return 2;

		case PUSHW: case PUSHL:
			if ( target_cpu == P4 ) {
				return 3;
			}  /* if */
			return 1;

		case XCHGB: case XCHGW: case XCHGL:
			if (target_cpu == P4) {
				return 1;
			}  /* if */
			return 0;

		case IDIVB: case IDIVW: case IDIVL:
			if (target_cpu == P4) {
				return 1;
			}  /* if */
			return 0;

		case MULB:
			if (target_cpu == P4) {
				return 5;
			}  /* if */
			return 0;

		case MULW:
			if (target_cpu == P4) {
				return 13;
			}  /* if */
			return 0;

		case MULL:
			if (target_cpu == P4) {
				return 29;
			}  /* if */
			return 0;

		case MOVB: case MOVW: case MOVL:
		case DIVB: case DIVW: case DIVL:
		case IMULB: case IMULW: case IMULL:
		case MOVSBW: case MOVSBL: case MOVSWL:
		case MOVZBW: case MOVZBL: case MOVZWL:
		/*case CALL:*/
				return 0;
		default: return ERROR;
		}
		/* NOTREACHED */
}  /*end pay_off*/

static void
estim()
{
int i;
long weight =1;
NODE *p;
long int new_estim;
int payoff;
i_cand *rgl;
int depth = 0;

	for (ALL_THE_LOOP(p)) {
		if (p->usage == LOOP_ENTRY) {	/* Adjust weight for loops. */
			depth++;
			if (depth <= MAXLDEPTH) weight *= WEIGHT;
		}
		if(p->usage == LOOP_EXIT) {		/*  Decrease weight for loop exit.	*/
			depth--;
			if (depth <= MAXLDEPTH) weight /= WEIGHT;
		}

		for (i = 1; i < MAXOPS + 1; i++) {
			if ((p->ops[i] == NULL) ||
				(rgl = lookup_icand(p->ops[i],all_icands)) == NULL)       
				continue;		/* Just examine regals */
			
			if (isimmed(rgl->name)) { /* calc payoff for an imm */
				if (isshift(p) || isimul(p) || (p->sets & ESP)) payoff = 0;
				else if (target_cpu == P6 && OpLength(p) == WoRD) payoff = 1;
				else payoff = hasdisplacement(p->op2) ? 1 : 0;
			}
			else payoff = pay_off(p);
			/* If some positive payoff - adjust the estimate. */
			if (payoff > 0) { 
				new_estim = rgl->estim;

				if(((MAXWEIGHT - new_estim) / weight) < payoff)
					new_estim = MAXWEIGHT;
				else
					new_estim += (weight * payoff);
				rgl->estim = new_estim;
			}  /* if */
		}/*for all ops[]*/
	}/*for all nodes*/
}/*end estim*/

/*This function checks whether the given operand participates in an unexpected
**instruction.
*/
static int
is_illegal_icand(pivot,idx) NODE *pivot; int idx;
{
NODE *p;
char *operand = pivot->ops[idx];
	for (ALL_THE_LOOP(p)) {
		if ((p->op1 && !strcmp(p->op1,operand)) ||
			(p->op2 && !strcmp(p->op2,operand))) {
			if (pay_off(p) == ERROR) return true;
		}
	}
	return false;
}/*end is_illegal_icand*/

static int last_icand = 0;
static void
no_icands()
{
int i;
	for (i = 0; i < MAX_CAND; i++) {
		all_icands[i] = null_icand;
		sorted_icands[i] = null_icand;
	}
	last_icand = 0;
	for (i = first_reg; i <= last_reg ; i++) {
		all_regs[i].status = FREE;
		all_regs[i].save_at = 0;
	}
}/*no_icands*/

int n_cand = 0;

static boolean
add_icand(p,m) NODE *p; int m;
{
int i;
int x = 0;
int y = 0;
boolean by_imm = isimmed(p->ops[m]);
boolean by_offset = (scanreg(p->ops[m],true) & frame_reg);
boolean is_param = fixed_stack && !by_imm && by_offset && !strstr(p->ops[m],".FSZ");

	if (!p->ops[m]) return false;
    if (is_param) return false;
    if (by_offset) { /* finding offset */
        x = p->ops[m][0] == '*' ? (int)strtoul(p->ops[m]+1,(char **)NULL,0)
				: (int)strtoul(p->ops[m],(char **)NULL,0);
        /* the * is for pointer ref */
    } else if (by_imm) {
        y = (int)strtoul(1+p->ops[m],(char **)NULL,0);
    }
	/* is the candidate already in */
	for (i = 0; i < n_cand; i++) {
		if (x) {
			if (x == all_icands[i].frame_offset)
				return false;
		} else if (by_imm && isimmed(all_icands[i].name)) {
			if (y == all_icands[i].value)
				return false;
		} else {
			if (!strcmp(p->ops[m],all_icands[i].name))
				return false;
		}
	}
	all_icands[last_icand].name = p->ops[m];
	all_icands[last_icand].frame_offset = x;
	if (isimmed(p->ops[m])) all_icands[last_icand].value = y;
	last_icand++;
	return true;
}/*end add_icand*/

/* In FIXED STACK mode, regals will be only those from the form: -X+.FSZ*(%esp)
** The parameters that look like X(%esp) should not be considered regals
** because they are parameters put on stack before calling function that use
** these parameters. Since all the regal structure is built as a hash table,
** when hash function is by the offset, we need to check the rest of the
** operand, to see if the correct form is present. If it is not, this operand
** should not enter the regals list at all (and afterwards not to the icands
** lists either.
*/

static int
find_icands()
{
NODE *p;
int i;
i_cand *c;
unsigned int r;
	n_cand = 0;
	for (ALL_THE_LOOP(p)) {
		for (i = 1; i <= 3; i++) { /* go thru operands of p */
			if (n_cand >= MAX_CAND) return n_cand;
			if (p->ops[i] == NULL) continue;
			if (isreg(p->ops[i]) || (isconst(p->ops[i]) && !isimmed(p->ops[i])))
				continue;
			if (isvolatile(p,i)) continue;
			if (is_illegal_icand(p,i)) continue;
			r = scanreg(p->ops[i],true);
			if ((r & frame_reg) == r && isregal(p->ops[i])) {
				if (add_icand(p,i)) ++n_cand;
			} else if (isimmed(p->ops[i])) {
				if (add_icand(p,i)) ++n_cand;
			} else if (disambiged(p)) {
				if (add_icand(p,i)) ++n_cand;
			}
		}/*for all ops of p*/
	}/*for all p in loop*/
	/* It may be the case that an operand disambiggs from all others in the loop
	** the first time it appears, but not in the second instruction it appears.
	** Therefore we disambig the candidate in the next appearences and remove
	** from the candidates array if it does not disambig in any of the
	** instructions.
	** The reason not to disambig in the second instruction is different lengths
	** of the different instructions.
	*/
	for (ALL_THE_LOOP(p)) {
		for (i = 1; i <= 3; i++) { /* go thru operands of p */
			if ((c = lookup_icand(p->ops[i],all_icands)) != NULL) {
				if (isconst(p->ops[i]) || is_read_only(p->ops[i])) continue;
				r = scanreg(p->ops[i],1);
				if ((r & frame_reg) == r && isregal(p->ops[i]))
					continue;
				if (!disambiged(p)) {
					*c = null_icand;
					--n_cand;
				}
			}
		}
	}
	return n_cand;
}/*end find_icands*/

static int
sort_icands (ncands,nregs) int ncands, nregs;
{
int max_estim;
i_cand *rgl = NULL;
int i;
int last_sorted = 0;
	for ( i = 0; i < MAX_CAND; i++) {
		if (isimmed(all_icands[i].name) && all_icands[i].estim == 0) {
			all_icands[i] = null_icand;
			ncands--;
		}
	}
	nregs = ncands = MIN(ncands,nregs); /* this will be the return value */
	while (ncands > 0) {
		max_estim = -1;
		for (i = 0; i < MAX_CAND; i++) { /* find best cand */
			if (all_icands[i].name && all_icands[i].estim > max_estim) {
				rgl = &all_icands[i];
				max_estim = rgl->estim;
			}
		}
		sorted_icands[last_sorted] = *rgl;
		*rgl = null_icand; /* dont find it again */
		++last_sorted;
		ncands--;
	}
	return nregs;
}/*end sort_icands*/

static int
anal_changes_to_icands()
{
NODE *p;
int i;
int len;
int changed_ones = 0;
	for (ALL_THE_LOOP(p)) {
		/* pushl x sets momory, but not location x...*/
		if (msets(p) & MEM && p->op != PUSHL && p->op != PUSHW) {
			for (i = 0; i < MAX_CAND; i++) {
				if (isimmed(sorted_icands[i].name)) continue;
				if (is_read_only(sorted_icands[i].name)) continue;
				if (sorted_icands[i].name &&
					((p->op1 && !strcmp(p->op1,sorted_icands[i].name)) ||
					(p->op2 && !strcmp(p->op2,sorted_icands[i].name)))) {
					sorted_icands[i].changed = true;
					changed_ones++;
					if ((len = OpLength(p)) > sorted_icands[i].length) {
						sorted_icands[i].length = len;
						if (len >= 4) break;
					}
				}
			}
		}
	}
	return changed_ones;
}/*end anal_changes_to_icands*/

/* return value is the number of registers to be saved before the loop
** and restored after it.
*/
static int
assign_regals_2_regs(ncands) int ncands;
{
int i,j;
int x = 0;
unsigned int regs2save = 0;
	/* go thru sorted cands and assign only FREE registers. */
	for (i = 0; i < ncands; i++) {
		if (sorted_icands[i].name == NULL) 
			fatal(__FILE__,__LINE__,"assign regals 2 regs, internal error\n");
		sorted_icands[i].in_byte_inst = reg_8_probs(i);
		for (j = first_reg; j <= last_reg; j++) {
			if (all_regs[j].status != FREE) continue;
			if (sorted_icands[i].in_byte_inst && !all_regs[j].names[0])
				continue;
			all_regs[j].status = LIVE;
			sorted_icands[i].reg_assigned = j;
			x++;
			break; /* dont assign another reg to this regal */
		}
	}
	if (x == ncands) { /* all cands were assigned to FREE registers */
		return 0;
	}
	/* make a second pass on the cands and assign NOT_USED registers */
	for (i = 0; i < ncands; i++) {
		if (sorted_icands[i].name == NULL) 
			fatal(__FILE__,__LINE__,"assign regals 2 regs, internal error\n");
		if (sorted_icands[i].reg_assigned != -1) continue;
		for (j = first_reg; j <= last_reg; j++) {
			if (all_regs[j].status != NOT_USED) continue;
			if (sorted_icands[i].in_byte_inst && !all_regs[j].names[0])
				continue;
			all_regs[j].status = LIVE;
			sorted_icands[i].reg_assigned = j;
			regs2save |= all_regs[j].reg;
			break; /* dont assign another reg to this regal */
		}
	}
	return regs2save;
}/*end assign_regals_2_regs*/

static BLOCK *
find_block_of_jtarget(jtarget) char *jtarget;
{
BLOCK *b,*target_block;
NODE *p;
boolean found_label = false;
	if (!jtarget) return NULL;
	for (b = b0.next; b; b = b->next) {
		p = b->firstn;
		while (is_any_label(p)) {
			if (!strcmp(jtarget,p->opcode)) {
				found_label = true;
				target_block = b;
				break;
			}
			p = p->forw;
		}
		if (found_label) break;
	}
	return target_block;
}/*end find_block_of_jtarget*/

static void
mark_regs2save(regs2save) unsigned int regs2save;
{
int i;
int n_regs = 0;
int save_at;
	/* count how many registers to save */
	for (i = first_reg; i <= last_reg; i++) {
		if (all_regs[i].reg & regs2save) n_regs += 4;
	}
    save_at = -inc_numauto(n_regs); /* make place on the stack */
    /* assign save places to registers */
    for (i = first_reg; i <= last_reg; i++) {
       	if (all_regs[i].reg & regs2save) {
           	all_regs[i].save_at = save_at;
           	save_at -= 4;
       	}
    }
}/*end mark_regs2save*/

static void
save_restore(regs2save) unsigned int regs2save;
{
int i;
NODE *prenew = first;
NODE *postnew = last;
	for (i = first_reg; i <= last_reg; i++) {
		if (all_regs[i].reg & regs2save) {
            if (!fixed_stack) {
			    prenew = prepend(prenew,NULL);
			    chgop(prenew,MOVL,"movl");
			    prenew->op1 = all_regs[i].names[3];
			    prenew->op2 = getspace(NEWSIZE);
			    sprintf(prenew->op2,"%d(%%ebp)",all_regs[i].save_at);
			    prenew->sets = 0;
			    prenew->uses = EBP | all_regs[i].reg;
			    addi(postnew,MOVL,"movl",prenew->op2,prenew->op1);
			    postnew->sets = all_regs[i].reg;
			    postnew->uses = EBP;
				save_regal(prenew->op2,LoNG,true,false);
            } else { /* we are in fixed stack mode */
                prenew = prepend(prenew,NULL);
                chgop(prenew,MOVL,"movl");
                prenew->op1 = all_regs[i].names[3];
                prenew->op2 = getspace(FRSIZE+ADDLSIZE);
                sprintf(prenew->op2,"%d-%s%d+%s%d(%%esp)",all_regs[i].save_at,regs_string,func_data.number,frame_string,func_data.number);
                prenew->sets = 0;
                prenew->uses = ESP | all_regs[i].reg;
                addi(postnew,MOVL,"movl",prenew->op2,prenew->op1); /* restore */
                func_data.pars -= 1; /* parameters saved are decremented because of restore */
                postnew->sets = all_regs[i].reg;
                postnew->uses = ESP;
				save_regal(prenew->op2,LoNG,true,false);
            }
		}
	}
}/*end save_restore*/

static void
insert_moves(jtarget,regs2save,chcands)
char *jtarget; unsigned int regs2save; int chcands;
{
NODE *p = first;
NODE *q = last;
BLOCK *target_block;
int i,entry;
char *new_label,*tmp;
int len;

#ifdef FLIRT
static int numexec=0;
#endif

#ifdef FLIRT
	++numexec;
#endif

	if (regs2save == 0 && chcands == 0) jtarget = NULL; /* no break treatment*/
	if (regs2save) mark_regs2save(regs2save);
	if (jtarget) {
		/* construct the jumps and new label before jtarget */
		target_block = find_block_of_jtarget(jtarget);
		new_label = newlab();
		p = prepend(target_block->firstn,NULL);
		chgop(p,LABEL,new_label);
		p->uses = p->sets = 0;
		/* We are introducing a block.  The JMP instruction's in_try_block
		   should be based on the instr that precedes it. */
		p = insert(p->back);
		FLiRT(O_INSERT_MOVES,numexec);
		chgop(p,JMP,"jmp");
		p->op1 = jtarget;
		p->uses = p->sets = 0;
		p = p->forw; /* point back to the new label */
		/* place the move instructions between new label and jtarget */
		for (i = 0; sorted_icands[i].name ; i++) {
			entry = sorted_icands[i].reg_assigned;
			if (entry != -1 && sorted_icands[i].changed) {
				len = sorted_icands[i].length -1;
				addi(p,moves[len].op,moves[len].opc,
					all_regs[entry].names[len],sorted_icands[i].name);
				p->uses = setreg(p->op1) | scanreg(p->op2,false);
				p->sets = 0;
			}
		}
		/* place the restore instructions after them (by using the same p) */
		if (regs2save) {
			for (i = first_reg; i <= last_reg; i++) {
				if (all_regs[i].reg & regs2save) {
                    if (!fixed_stack) {
					    tmp = getspace(NEWSIZE);
					    sprintf(tmp,"%d(%%ebp)",all_regs[i].save_at);
					    len = 3; /* full reg is always saved */
					    addi(p,moves[len].op,moves[len].opc,tmp,all_regs[i].names[len]);
					    p->uses = EBP;
                    } else { /* Fixed stack mode */
                        tmp = getspace(FRSIZE+ADDLSIZE);
                        sprintf(tmp,"%d-%s%d+%s%d(%%esp)",all_regs[i].save_at,regs_string,func_data.number,frame_string,func_data.number);
						len = 3;
                        addi(p,moves[len].op,moves[len].opc,tmp,all_regs[i].names[len]);
                        p->uses = ESP;
                    }
					FLiRT(O_INSERT_MOVES,numexec);
					p->sets = all_regs[i].reg;
				}
			}
		}
		/* change referenes to jtarget to reference the new label */
		for (ALL_THE_LOOP(p)) {
			if (is_any_br(p) && !strcmp(p->op1,jtarget)) {
				p->op1 = new_label;
			}
		}
	}/* endif jtarget */
	save_restore(regs2save);
	/* place moves before and after the loop */
	p = first;
	for (i = 0; sorted_icands[i].name ; i++) {
		entry = sorted_icands[i].reg_assigned;
		if (entry != -1) {
			len = sorted_icands[i].length -1;
			if (len == -1) len = 3;
			p = prepend(p,NULL);
			chgop(p,moves[len].op,moves[len].opc);
			FLiRT(O_INSERT_MOVES,numexec);
			p->op1 = sorted_icands[i].name;
			if (*p->op1 == '*')
				p->op1++;
#ifdef FLIRT
			if (isimmed(p->op1)) FLiRT(O_IMM_2_REG,numexec);
#endif
			p->op2 = all_regs[entry].names[len];
			p->uses = scanreg(p->op1,false);
			p->sets = setreg(p->op2);
			if (sorted_icands[i].changed) {
				len = sorted_icands[i].length -1;
				addi(q,moves[len].op,moves[len].opc,all_regs[entry].names[len],
					sorted_icands[i].name);
				if (*q->op2 == '*')
					q->op2++;
				q->uses = setreg(q->op1) | scanreg(q->op2,false);
				q->sets = 0;
			}
		}
	}
}/*end insert_moves*/

static void
replace_regals_w_regs()
{
NODE *p;
int i;
i_cand *rgl;
int entry;
one_reg *i_reg;
#ifdef FLIRT
static int numexec=0;
#endif

#ifdef FLIRT
	++numexec;
#endif

	for (ALL_THE_LOOP(p)) {
		for (i = 1 ; i <= 3; i++) {
			if ((rgl = lookup_icand(p->ops[i],sorted_icands)) != NULL) {
				if ((isshift(p) || isimul(p) || (p->sets & ESP))
					&& isimmed(rgl->name))
					continue;
				entry = rgl->reg_assigned;
				if (entry != -1) {
					i_reg = &all_regs[entry];
					if (p->ops[i][0] == '*') {
						p->ops[i] = getspace(6);
						p->ops[i][0] = '*';
						strcpy(&p->ops[i][1],i_reg->names[OpLength(p) -1]);
					}
				  	else {
						p->ops[i] = i_reg->names[OpLength(p) -1];
					}
					FLiRT(O_REPLACE_REGALS_W_REGS,numexec);
					new_sets_uses(p);
				}
			}
		}
	}
}/*end replace_regals_w_regs*/

#ifdef DEBUG
void
show_icands(base) i_cand *base;
{
int i;
	if (base == all_icands) fprintf(STDERR,"all ");
	else fprintf(STDERR,"sorted ");
	fprintf(STDERR,"loop of "); FPRINST(first);
	for (i = 0; i < MAX_CAND; i++) {
		if (base[i].name != NULL) {
			fprintf(STDERR,"at %d cand %s with %d \n",i,base[i].name,
			base[i].estim);
		}
	}
}/*end show_icands*/
#endif

/*driver. find the loop, test if it is optimizable and do it.*/
void
loop_regal(what_to_do)
int what_to_do;  /* ordinal number of invocation */
{
NODE *pm;
char *jtarget = NULL;
int nfree_regs;
int n_icands = 0;
unsigned int regs2save;
boolean do_regal; /* a flag to disable */
int changed_cands;
boolean  pseudo_regals = 0 ;

	COND_RETURN("loop_regal");
	ldanal(); /*count on it in find dead regs in the loop*/
	rm_dead_insts(); /* setting of dead regs are dangerous to find_free_reg */
	for (ALLN(pm)) { /* prepare for mark entry end exit of all loops */
		pm->usage = 0;
	}
	for (ALLN(pm)) { /* mark entry end exit of all loops */
		if (iscbr(pm) && lookup_regals(pm->op1,first_label)) {
			if ((first = is_back_jmp(pm)) != NULL) {
				last = find_last_jmp(pm);
				first->usage = LOOP_ENTRY;
				last->usage = LOOP_EXIT;
			}
		}
	}
	for (ALLN(pm)) {
		jtarget = NULL;
		do_regal = true;
		if (iscbr(pm) && lookup_regals(pm->op1,first_label)) {
			if ((first = is_back_jmp(pm)) == NULL) {
#ifdef DEBUG
				if (zflag > 1) {
					fprintf(STDERR,"not a back jmp");
					FPRINST(last);
				}
#endif
				continue;
			}
			last = find_last_jmp(pm);
#ifdef DEBUG
			if (zflag) loop_of();
#endif
			if (is_simple_loop(&jtarget) == 0) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"loop not simple: ");
					FPRINST(first);
				}
#endif
				continue;
			}

			no_icands();
			if ((nfree_regs = find_available_regs_in_loop()) == 0) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"no free regs in loop.\n");
				}
#endif
				do_regal = false;
			} else {
				if (what_to_do == DO_deindexing) 
					pseudo_regals = index_ops_2_regals(jtarget);
				if ((n_icands = find_icands()) == 0) {
#ifdef DEBUG
					if (zflag > 1) {
						fprintf(STDERR,"didnt find candidates for ");
						FPRINST(first);
					}
#endif
					do_regal = false;
					if (pseudo_regals) regals_2_index_ops();
					if (simple_done) {
						simple_done=false;
						ldanal();
					}
				}
			}
			if (do_regal) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"do something ");
					FPRINST(first);
				}
#endif
				COND_SKIP(continue,"Rloop %d %s\n",second_idx,first->opcode,0);
				estim();
				n_icands = sort_icands(n_icands,nfree_regs);
				if (n_icands == 0) {
					if (pseudo_regals)	regals_2_index_ops();
					if (simple_done) {
						simple_done=false;
						ldanal();
					}
					continue;
				}
				regs2save = assign_regals_2_regs(n_icands);
				changed_cands = anal_changes_to_icands();
				insert_moves(jtarget,regs2save,changed_cands);
				replace_regals_w_regs();
				if (pseudo_regals) regals_2_index_ops();
				ldanal();
				simple_done=false;
			}
		}/*endif cmp-jcc*/
	}/*end for loop*/

	return;
}/*end loop_regal*/


static char *
remove_scale(op) char *op;
{
char *tmp;
char *s,*t;
int length = strlen(op);
		tmp = (char *) getspace(length);
		for (s = op, t = tmp; *s != '('; s++) {
			*t = *s;
			t++;
		}
		*t++ = *s++; /* copy the '(' */
		if (*s == ',') s++;
		while (!isdigit(*s)) {
			*t++ = *s++;
		}
		*--t = ')';
		*++t = (char) 0;
		return tmp;
}/*end remove_scale*/

#ifdef FLIRT
static int numdescale=0;
#endif

static void
descale(compare,jtarget) NODE *compare; char *jtarget;
{
NODE *p;
NODE *increment = NULL;
unsigned int pivot_reg;
int scale = 0, shft, tmp_scale;
int m = 0;
boolean found_scale = false;
int old_val;
NODE *tmp;
BLOCK *b,*target_block;
NODE *target_label;
boolean found_label ;
char *new_label;
#ifdef DEBUG
	if (zflag) {
		fprintf(STDERR,"descale ");
		FPRINST(first);
	}
#endif
	/*The compare has to be a compare register to immediate*/
	if (! (isimmed(compare->op1) && isreg(compare->op2))) {
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"not a compare of imm to reg ");
			FPRINST(compare);
		}
#endif
		return;
	}
	else pivot_reg = setreg(compare->op2);
	/*The increment has to be either an inc or an add of a constant, and to
	**the same register as in the compare.
	**It can also be leal $num(reg),reg ; and then it changes here to an add.
	*/
	for (p = compare; p != first; p = p->back) {
		if ((m = 1, p->op == INCL) || (m = 2, p->op == ADDL)) {
			if (isreg(p->ops[m]) && samereg(p->ops[m],compare->op2)) {
				increment = p;
				break;
			}
		}
		if (p->op == LEAL
			&& isreg(p->op2)
			&& (indexing(p) == p->sets)
			&& isdigit(*p->op1)
			&& samereg(p->op2,compare->op2)) {
			char *tmp;
			increment = p;
			chgop(p,ADDL,"addl");
			FLiRT(O_LOOP_DESCALE,numdescale);
			tmp = getspace(ADDLSIZE);
			sprintf(tmp,"$%d",(int)strtoul(p->op1,(char **)NULL,0));
			p->op1 = tmp;
			p->sets |= CONCODES_AND_CARRY;
			break;
		}
	}
	if (!increment) {
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"didnt find increment, return\n");
		}
#endif
		return;
	}
	if (increment->op == ADDL &&
		! (isconst(increment->op1) && isdigit(increment->op1[1]))) {
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"add of not constant ");
			FPRINST(increment);
		}
#endif
		return;
	}
	/*The register has to be an index register with the same scale at all
	**it's appearences but the compare and increment*/
	for (ALL_THE_LOOP(p)) {
		if (p == increment || p == compare) continue;
		if ((uses_but_not_indexing(p) | p->sets) & pivot_reg) {
#ifdef DEBUG
			if (zflag) {
				fprintf(STDERR,"abuse the reg ");
				FPRINST(p);
			}
#endif
			return;
		}
		if (! (indexing(p) & pivot_reg)) continue;
		if (scanreg(p->op1,true) & pivot_reg) {
			m = 1;
		} else if (scanreg(p->op2,true) & pivot_reg) {
			m = 2;
		} else if (scanreg(p->op3,true) & pivot_reg) {
			m = 3;
		}
		if (pivot_reg & setreg(strchr(p->ops[m],'(') + 1)) {
#ifdef DEBUG
			if (zflag) {
				fprintf(STDERR,"pivot reg is base ");
				FPRINST(p);
			}
#endif
			return;
		}
		tmp_scale = has_scale(p->ops[m]);
		if (!found_scale) {
			found_scale = true;
			scale = tmp_scale;
		} else {
			if (scale != tmp_scale) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"found two different scales.\n");
				}
#endif
				return;
			}
		}
	}/*for loop*/
	if (scale == 0) {
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"scale == 0, return\n");
		}
#endif
		return;
	}
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"do the descale\n");
		}
#endif
	/*Multiply the increment and compare by scale, prepend mul index by scale 
	**and remove the scales*/
	shft = scale == 8 ? 3 : scale == 4 ? 2 : 1;
	tmp = prepend(first,itoreg(pivot_reg));
	chgop(tmp,SHLL,"shll");
	FLiRT(O_LOOP_DESCALE,numdescale);
	tmp->op1 = getspace(ADDLSIZE);
	sprintf(tmp->op1,"$%d",shft);
	tmp->op2 = itoreg(pivot_reg);
	new_sets_uses(tmp);

	if (increment->op == INCL) {
		FLiRT(O_LOOP_DESCALE,numdescale);
		chgop(increment,ADDL,"addl");
		increment->op2 = increment->op1;
		old_val = 1;
	} else {
		old_val = (int)strtoul(increment->op1+1,(char **)NULL,0);
	}
	increment->op1 = getspace(ADDLSIZE);
	sprintf(increment->op1,"$%d",old_val * scale);

	old_val = (int)strtoul(compare->op1+1,(char **)NULL,0);
	compare->op1 = getspace(ADDLSIZE);
	sprintf(compare->op1,"$%d",old_val * scale);

	for (ALL_THE_LOOP(p)) {
		if (scanreg(p->op1,true) & pivot_reg) {
			m = 1;
		} else if (scanreg(p->op2,true) & pivot_reg) {
			m = 2;
		} else if (scanreg(p->op3,true) & pivot_reg) {
			m = 3;
		} else continue;
		p->ops[m] = remove_scale(p->ops[m]);
	}

	target_block = NULL; target_label = NULL;
	found_label = false;
	if (jtarget) {
		new_label = newlab();
		/*find the block and node which are the target of the jump*/
		for (b = b0.next; b; b = b->next) {
			p = b->firstn;
			while (is_any_label(p)) {
				if (!strcmp(p->opcode,jtarget)) {
					target_block = b;
					target_label = p;
					found_label = true;
					break;
				}
				p = p->forw;
			}
			if (found_label)
				break;
		}/*for loop*/
		/*add some nodes for break treatment */
		p = insert(target_block->firstn->back); /*add a node before jmp target*/
							/* with proper in_try_block value. */
		chgop(p,JMP,"jmp"); /*make it a jmp to target */
		p->op1 = target_label->opcode; /*address the jmp to the label*/
		p->uses = p->sets = 0;
		FLiRT(O_LOOP_DESCALE,numdescale);
		p = prepend(target_block->firstn,NULL); /*add a node after the jmp*/
		chgop(p,LABEL,new_label); /*make it the new label*/
		p->uses = p->sets = 0;

		p = insert(p); /*add a node for the move instruction*/
		chgop(p,SARL,"sarl");
		p->op2 = first->back->op2;
		switch(scale) {
			case 2: p->op1 = "$1"; break;
			case 4: p->op1 = "$2"; break;
			case 8: p->op1 = "$3"; break;
		}
		new_sets_uses(p);
		for (ALL_THE_LOOP(p)) {
			if (is_any_br(p) && !strcmp(p->op1,jtarget)) {
				p->op1 = new_label;
			}
		}
	}/*endif jtarget*/

	tmp = insert(last);
	chgop(tmp,SARL,"sarl");
	tmp->op2 = first->back->op2;
	switch(scale) {
		case 2: tmp->op1 = "$1"; break;
		case 4: tmp->op1 = "$2"; break;
		case 8: tmp->op1 = "$3"; break;
	}
	new_sets_uses(tmp);
}/*end descale*/

void
loop_descale()
{
NODE *pm;
char *jtarget = NULL;

	COND_RETURN("loop_descale");

#ifdef FLIRT
	++numdescale;
#endif

	for (ALLN(pm)) {
		jtarget = NULL;
		last = pm->forw; if (!last || last == &ntail) return;
		if (pm->op == CMPL && iscbr(last) &&
			lookup_regals(last->op1,first_label)) {
			if ((first = is_back_jmp(last)) == NULL) {
#ifdef DEBUG
				if (zflag > 1) {
					fprintf(STDERR,"not a back jmp");
					FPRINST(last);
				}
#endif
				continue;
			}
			if (is_simple_loop(&jtarget) == 0) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"loop not simple: ");
					FPRINST(first);
				}
#endif
				continue;
			}
			COND_SKIP(continue,"loop %d %s\n",second_idx,first->opcode,0);
			descale(pm,jtarget);
			if (jtarget) bldgr(false);
		}/*endif cmp-jcc*/
	}/*end for loop*/
}/*end loop_descale*/


/* end of integer optimization, start fp optimization. */

#define SEEN_FST		23		/* Jordan */
#define SEEN_ARIT		34		/* Hakeem */
#define ARIT_B4_FST		11		/* Isea	  */
#define FST_B4_ARIT		32		/* Magic  */
#define MAX_CANDS		80		/* size of the array */

typedef struct cand_s {
			int frame_offset;  /* -offset(%ebp) */
			char *name;		 /* the whole string as above */
			int type;        /* preloaded or not */
			int estim;		 /* estimation of gain by using from st vs memory */
			int size;        /* size in bytes */
			boolean type_decided; /* temp to help determine the type */
			boolean	needed;  /* is it live after the loop */
			NODE *last_arit; /* for an fstp type, point to the last arit inst*/
			int	entry;       /* current location on the fp stack */
		} candidate;

typedef struct node_list {
		NODE *element;
		struct node_list *next;
		} NODE_LIST;

static NODE_LIST *open_jumps = NULL;
static candidate all_cands[MAX_CANDS];
static candidate sorted_cands[MAX_CANDS];
static candidate null_cand = { 0 , NULL , 0 , 0 , 0 , 0 , 0 , NULL , -1 };
static candidate *null_cand_p = &null_cand;
static int last_cand = 0;
char *st_i[] = { "%st(0)", "%st(1)", "%st(2)", "%st(3)", "%st(4)",
						"%st(5)", "%st(6)", "%st(7)"  };
static int n_candidates; /*number of operands to assign on stack */

/* Is the instruction one that we know how to deal with */
static boolean
is_fp_optimable(NODE *p)
{
	return (is_fld(p) || is_fst(p) || is_fp_arit(p));

} /* end is_fp_optimable */

static void
no_cands(candidate *stack[8])
{
  int i;
	for (i = 0; i < MAX_CANDS; i++) {
		all_cands[i] = null_cand;
		sorted_cands[i] = null_cand;
	}
	last_cand = 0;
	open_jumps = NULL;
	for (i = 0; i < 8; i++) {
		stack[i] = NULL;
	}
} /* end no_cands */

static void
remember_jcc(NODE *p)
{
  NODE_LIST *e;
	e = (NODE_LIST *) getspace(sizeof(NODE_LIST));
	e->element = p;
	e->next = open_jumps;
	open_jumps = e;
} /* end remember_jcc */

static void
forget_jcc(NODE *p)
{
  NODE_LIST *e,*nexte;
  char *label = p->opcode;
	if (!open_jumps) {
		return;
	}
	if (!strcmp(open_jumps->element->op1,label)) {
		open_jumps = NULL;	/* ???? This erases the full list ??? */
		return;
	}
	for (e = open_jumps; e->next; e = nexte) {
		nexte = e->next;
		if (!strcmp(e->next->element->op1,label)) {
			e->next = nexte->next;
		}
	}
} /* end forget_jcc */

/* Add an operand to the array. If it is new, place a cand structure for
** it in the next entry of all_cands, return value is 1. Otherwise return
** value is 0; follow up on the type of the candidate:
** A candidate can be first used, later set, or first set, later used, or
** only set, or only used. If it is only used it will be discarded and not
** placed on the stack. If it is first used, then it's value has to be
** loaded to the stack before the loop. Otherwise it may not have a valid 
** FP value before the loop. So this distinction is important.
** A variable may be set-before-used in one control flow of the loop and
** used-before-set in a second control flow; such variables are removed from
** the candidate set and return value is -1;
*/
static int
add_operand(NODE *p)
{
  int i;
  int x;
    x = (int)strtoul(p->op1,(char **)NULL,0);
	for (i = 0; i < last_cand; i++) {
		if (all_cands[i].frame_offset == x) {
			if (   (   open_jumps
			        || !all_cands[i].type_decided)
			    && (   (   (all_cands[i].type == SEEN_FST)
			            && !is_fstp(p))
			         || (   (all_cands[i].type == SEEN_ARIT)
			             && is_fstp(p)))) {
				all_cands[i] = null_cand;
				return -1;
			}
			switch (all_cands[i].type) {
				case SEEN_FST:
					if (is_fp_arit(p) || is_fld(p)) {
						all_cands[i].type = FST_B4_ARIT;
					}
					break;

				case SEEN_ARIT:
					/* I want an fstp here, not just fst, so something will be
					   left on the stack.
					*/
					if (is_fstp(p)) {
						all_cands[i].type = ARIT_B4_FST;
					}
					break;

				case FST_B4_ARIT:
				case ARIT_B4_FST:
					break;

				default:
					fatal(__FILE__,__LINE__,"unknown type of candidate\n");
			}
			all_cands[i].estim += gain_by_mem2st(p->op);
			return 0;
		}
	}
	/* no entry yet for this candidate */
	all_cands[last_cand].frame_offset = x;
	all_cands[last_cand].name = p->op1;
	/* This is a problem. Currently, a candidate with only fst and no fstp
	   may enter, and I'm not sure I want such a candidate.
	   Big change to the above: now, an fst counts as an arit! and not as
	   an fst. only fstp is not arit.
	*/
	all_cands[last_cand].type = is_fstp(p) ?  SEEN_FST : SEEN_ARIT ;
	all_cands[last_cand].estim = gain_by_mem2st(p->op);
	all_cands[last_cand].size = OpLength(p);
	all_cands[last_cand].needed = live_at(p,last);
	if (!open_jumps) {
		all_cands[last_cand].type_decided = true;
	}
	++last_cand;
	return 1;
} /* end add_operand */

static boolean
rm_operand(NODE *p)
{
  int i;
  int x;
	if (ismem(p->op1)) {
		x = (int)strtoul(p->op1,(char **)NULL,0);
	} else if (ismem(p->op2)) {
		x = (int)strtoul(p->op2,(char **)NULL,0);
	} else {
		return false;
	}
	for (i = 0; i < last_cand; i++) {
		if (all_cands[i].frame_offset == x) {
			all_cands[i] = null_cand;
			return true;
		}
	}
	return false;
} /* end rm_operand */

/* Scan through the loop, find operands and add them to the candidates array */
static int
find_candidates()
{
  NODE *p;
  int n_candidates = 0;

	/* Single sanity check that MAX_CANDS > 0. */
	if (n_candidates >= MAX_CANDS) {
		return n_candidates;
	}
	for (ALL_THE_LOOP(p)) {
		if (is_any_cbr(p)) {
			remember_jcc(p);
		}
		if (is_any_label(p)) {
			forget_jcc(p);
		}
		/* Only interested in FP instructions beyond this point. */
		if (!isfp(p)) {
			continue;
		}
		/* Restrict the search to local variables (REGALS) */
		if ((indexing(p) & frame_reg) == 0) {
			continue;
		}
		if (isvolatile(p,1)) {
			continue;
		}
		if (   !isregal(p->op1)
		    && !disambiged(p)) {
			continue;
		}
		if (is_fp_optimable(p)) {
			n_candidates += add_operand(p);
			if (n_candidates >= MAX_CANDS) {
				return n_candidates;
			}
		}
	}
	for (ALL_THE_LOOP(p)) {
		if ((indexing(p) & frame_reg) == 0) {
			continue;
		}
		if (isvolatile(p,1)) {
			continue;
		}
		if (   !is_fp_optimable(p)
		    && rm_operand(p)) {
			n_candidates--; 
		}
	}
	return n_candidates;

} /* end find_candidates */

/* Sort the array all_candidates filled by find_candidates into the array
** sorted candidates. Take at most as many candidates as free_regs. If
** there were less to begin with, take all. In any case throw away candidates
** with negative payof estimation.
** return the number of sorted candidates.
*/
static int
sort_candidates(int free_regs)
{
  int last_taken = 0;
  int max_estim = 0;
  int i;
  candidate *c_p = NULL;

	/* Throw away cand with negative payof estimate, and those with no
	   fst's.  Fst only will stay, they are optimizable. Also dont throw
	   away cands with zero payof; there are peepholes afterwards.
	*/
	for (i = 0; i < last_cand; i++) {
		if (all_cands[i].frame_offset != 0) {
			if (all_cands[i].estim < 0) {
				all_cands[i] = null_cand;
				--n_candidates;
			}
			if (   target_cpu != P6
			    && all_cands[i].type == SEEN_ARIT) {
				all_cands[i] = null_cand;
				--n_candidates;
			}
		}
	}
	/* No longer need four types of cands but only two. shrink them */
	for (i = 0; i < last_cand; i++) {
		if (all_cands[i].type == SEEN_ARIT) {
			all_cands[i].type = ARIT_B4_FST;
		} else if (all_cands[i].type == SEEN_FST) {
			all_cands[i].type = FST_B4_ARIT;
		}
	}
	if (n_candidates <= free_regs) {
		for (i = 0; i < last_cand; i++) {
			if (all_cands[i].frame_offset != 0) {
				sorted_cands[last_taken] = all_cands[i];
				last_taken++;
			}
		}
#ifdef DEBUG
		if (last_taken != n_candidates)
			fatal(__FILE__,__LINE__,
			"taken %d out of %d cands.\n",last_taken,n_candidates);
#endif
		return n_candidates;
	}
	if (n_candidates > free_regs) {
		n_candidates = free_regs;
	}
	while (n_candidates) {
		/*scan the original array for the best cand. */
		max_estim = -1;
		for (i = 0; i < last_cand; i++) {
			if (   all_cands[i].frame_offset != 0
			    && all_cands[i].estim > max_estim) {
				max_estim = all_cands[i].estim;
				c_p = &all_cands[i];
			}
		}
		/* Found the best cand; now place it in the sorted array */
		sorted_cands[last_taken] = *c_p;
		*c_p = null_cand;
		++last_taken;
		--n_candidates;
	} /* while loop */
	return last_taken; /*number of taken candidates*/
} /* end sort_candidates */

static void
fst2fstp()
{
  NODE *p,*pnew;
	for (ALL_THE_LOOP(p)) {
		if (   p->op != FSTL
		    && p->op != FSTS) {
			continue;
		}
		pnew = insert(p);
		pnew->op1 = p->op1;
		if (p->op == FSTL) {
			chgop(p,FSTPL,"fstpl");
			chgop(pnew,FLDL,"fldl");
		} else {
			chgop(p,FSTPS,"fstps");
			chgop(pnew,FLDS,"flds");
		}
	}
}

/* five states of entries on the simulated stack */
#define CLEAR	0
#define START	1
#define PART	2
#define OVERLAPPED	3
#define LEFTOVER	4

static int
rem_overlapping_cands()
{
  NODE *p;
  int min_offset, max_offset;
  char *access;
  int access_size;
  int i;
  int offset,location;
  int size;
  int x;
  boolean all_clear, all_parts;

	/* Find the smallest and biggest offsets in the loop */
	min_offset = max_offset = all_cands[0].frame_offset;
	for (ALL_THE_LOOP(p)) {
		if (scanreg(p->op1,true) & frame_reg) {
			offset = (int)strtoul(p->op1,(char **)NULL,0);
		} else if (scanreg(p->op2,true) & frame_reg) {
			offset = (int)strtoul(p->op2,(char **)NULL,0);
		} else {
			continue;
		}
		if (offset < min_offset) {
			min_offset = offset;
		}
		if (offset > max_offset) {
			max_offset = offset;
		}
	}
	min_offset -= 8;
	max_offset += 8;
	/* Assign an array and initialize it's entries to CLEAR */
	access_size = max_offset - min_offset;
	access = (char *) getspace(access_size);
	for (x = 0; x < access_size; x++) {
		access[x] = CLEAR;
	}
	/* Go over all operands in the loop and mark them on the array */
	/* Look for ugly overlaps and mark them as such on the array  */
	for (ALL_THE_LOOP(p)) {
		if (scanreg(p->op1,false) & frame_reg) {
			offset = (int)strtoul(p->op1,(char **)NULL,0);
		} else if (scanreg(p->op2,true) & frame_reg) {
			offset = (int)strtoul(p->op2,(char **)NULL,0);
		} else {
			continue;
		}
		location = offset - min_offset;
		size = OpLength(p);
		all_clear = true;
		for (x = location ; x < location + size; x++) {
			if (access[x] != CLEAR) {
				all_clear = false;
			}
		}
		if (all_clear) {
			access[location] = START;
			for (x = location+1; x < location + size; x++) {
				access[x] = PART;
			}
		} else {
			/* check for exact overlap - a legal case */
			all_parts = true;
			for (x = location +1; x < location + size; x++) {
				if (access[x] != PART) {
					all_parts = false;
				}
			}
			/* If not an exact overlap, mark the overlaped entries
			   as overlapped and the rest of the entries as
			   leftovers.     */
			if (   access[location] != START
			    || !all_parts) {
				for (x = location; x < location + size; x++) {
					if (access[x] != CLEAR)  {
						access[x] = OVERLAPPED;
					} else {
						access[x] = LEFTOVER;
					}
				}
			}
		}
	} /* for all nodes in the loop */

	/* Go over the candidates and remove accessed and unallowed ones */
	for (i = 0; i < last_cand; i++) {
		if (all_cands[i].frame_offset) {
			offset = all_cands[i].frame_offset;
			location = offset - min_offset;
			size = all_cands[i].size;
			for (x = location; x < location + size; x++) {
				if (access[x] == OVERLAPPED) {
					all_cands[i] = null_cand;
					n_candidates--;
					break;
				}
			}
		}
	}
	return n_candidates;

} /* end rem_overlapping_cands */

/* Find the number of registers free throughout the entire function.
** If the first block starts at entry > 0, return 0 so that this loop will
** not be optimized.  If any block increases the depth of the FP
** stack, the loop cannot be optimized.
*/

static int
find_max_free()
{
  BLOCK *b = first_block;
  int max, max1;

	if (   ((first_block->marked & 0xf) != 0)
	    && ((first_block->marked & 0xf) != ((first_block->marked >> 8) & 0xf))) {
		return 0;
	};
	max = (b->marked >> 4) & 0xf;
	while (b->lastn != last) {
		/* At least another block; get it. */
		if ((first_block->marked & 0xf)
		         != ((first_block->marked >> 8) & 0xf)) {
			return 0;
		}
		b = b->next;
		/* max number of used entries in stack */
		max1 = (b->marked >> 4) & 0xf;
		if (max1 > max) {
			max = max1;
		}
	} 
	return (8 - max);

} /* find_max_free */

/* If p->op1 is a candidate return a pointer to it's structure.
** Otherwise return a pointer to null_cand.
*/

static candidate *
is_cand(NODE *p)
{
  int i;
  int x;
  boolean by_x;
	if (!p || p->op1 == NULL) {
		return &null_cand;
	}
	if (indexing(p) == frame_reg) {
		by_x = true;
		x = (int)strtoul(p->op1,(char **)NULL,0);
	} else {
		by_x = false;
	}
	if (by_x)  {
		for (i = 0; i <  n_candidates; i++) {
			if (sorted_cands[i].frame_offset == x) {
				return &sorted_cands[i];
			}
		}
	} else {
		for (i = 0; i <  n_candidates; i++) {
			if (!strcmp(sorted_cands[i].name,p->op1)) {
				return &sorted_cands[i];
			}
		}
	}
	return &null_cand;
} /* end is_cand */

#define PROCESSED	0x1000
#define INSIDE		0x2000
#define IN_THE_LOOP(B) ((B)->marked  & INSIDE)
#define IS_CLEAR(B) (((B)->marked & PROCESSED) == 0)
#define MARK_VISITED(B)  (B)->marked |= PROCESSED

static void
mark_blocks_in_loop()
{
  BLOCK *b;
	for (b = b0.next; b; b = b->next) {
		b->marked &= 0x0FFF; /* clear bits 12-15 */
	}
	for (b = first_block; b; b = b->next) {
		b->marked |= INSIDE;
		if (b->lastn == last) {
			break;
		}
	}
} /* end mark_blocks_in_loop */


static void
reset_sorted_cands_entries()
{
  int i;
	for (i = 0; i < n_candidates; i++) {
		sorted_cands[i].entry = -1;
	}  /* for */
} /* end rset_sorted_cands_entries */


static void
set_cands_entries(candidate *stack[8])
{
  int i;
	for (i = 0; i < 8; i++) {
		if (   stack[i]
		    && stack[i] != null_cand_p) {
			stack[i]->entry = i;
		}
	}
} /* end set_cands_entries */


/* functions to simulate stack operations.
*/

static void
simulate_fld(candidate *c, candidate *stack[8])
{
  int i;
  int last;
	if (stack[7]) {
		if (stack[7] == null_cand_p) {
			fatal(__FILE__,__LINE__,"cant fld, [7] has null_cand\n");
		} else {
			fatal(__FILE__,__LINE__,
				"fld: already 8 %d, %s, cant fld; at %d\n",
				stack[7]->frame_offset,
				stack[7]->name,c?c->frame_offset:0);
		}
	}
	/* Find last entry - already know 7th entry is NULL */
	for (last = 0; stack[last] != NULL; last++) ;
	/* Last is really the last current entry + 1, the new last after
	   the candidate is loaded.*/
	for (i = last; i >= 1; i--) {
		stack[i] = stack[i - 1];
		/* Update the register numbers of the items of interest on the 
		   FP stack */
		if (   stack[i]
		    && stack[i] != null_cand_p) {
			stack[i]->entry = i;
		}
	}
	/* Now update the candidates entry, but only if not already on the
	   FP stack. */
	if (   ((stack[0] = c) != NULL)
	    && (c != null_cand_p)
	    && (c->entry < 0)) {
		c->entry = 0;
	}
} /* end simulate_fld */

static void
simulate_fst(candidate *stack[8])
{
  int i;
  int last;
	if (stack[0] == NULL) {
		fatal(__FILE__,__LINE__,"Empty stack, can't pop\n");
	}
	if (stack[0] != null_cand_p) {
		/* reset the FP register number of the item being popped */
		stack[0]->entry = -1;
	}
	/* find last entry */
	for (last = 7; stack[last] == NULL; last--);
	/* move every item up 1 stack location. */
	for (i = 0; i < last ; i++) {
		stack[i] = stack[i+1];
		/* update the register numbers of the items of interest on the 
		   FP stack */
		if (   stack[i]
		    && stack[i] != null_cand_p) {
			stack[i]->entry = i;
		}
	}
	stack[last] = NULL;
} /* end simulate_fst */

/*better have a seperate function than activate simulate_fst twice. */
static void
simulate_fcompp(candidate *stack[8])
{
  int i;
  int last;
	if (   stack[0] == NULL
	    || stack[1] == NULL) {
		fatal(__FILE__,__LINE__,"fcompp: cant pop twice\n");
	}
	/* reset the FP register number of the 2 items being popped. */
	if (stack[0] != null_cand_p) {
		stack[0]->entry = -1;
	}
	if (stack[1] != null_cand_p) {
		stack[1]->entry = -1;
	}
	/* find last entry */
	for (last = 7; stack[last] == NULL; last--);
	/* move every item up 2 stack locations. */
	for (i = 0; i < last - 1 ; i++) {
		stack[i] = stack[i+2];
		/* update the register numbers of the items of interest on the 
		   FP stack */
		if (   stack[i]
		    && stack[i] != null_cand_p) {
			stack[i]->entry = i;
		}
	}
	stack[last] = NULL;
	stack[last-1] = NULL;
} /* end simulate_fcompp */

static void
simulate_fxch(int i, candidate *stack[8])
{
  candidate *tmp;
	tmp = stack[0];
	stack[0] = stack[i];
	if (   stack[0]
	    && stack[0] != null_cand_p)  {
		stack[0]->entry = 0;
	}
	stack[i] = tmp;
	if (stack[i] != null_cand_p)  {
		stack[i]->entry = i;
	}
}

/* the target of an fstp instruction is the stack, whereas the target of an
** fstpl instruction is not the fp stack. Hence the different simulations.
*/
static void
simulate_fstp(candidate *c, int at, candidate *stack[8])
{
	stack[at] = c;
	/* It is a waste to update the "entry" field for the item just
	   popped and inserted onto the FP stack at location "at".  The
	   simulate_fst() call will just reassign it a different register
	   number. */
	simulate_fst(stack);
}

static void
change_name_of_tos(NODE *p, candidate *stack[8])
{
	stack[0] = is_cand(p);
	if (stack[0] != null_cand_p) {
		stack[0]->entry = 0;
	}
} /* end change_name_of_tos */

#ifdef DEBUG
void
show_stack(candidate *stack[8])
{
  int i;
	for (i = 0; i < 8; i++) {
		if (stack[i] != NULL) {
			fprintf(STDERR,"st[%d] = ",i);
			if (stack[i] == null_cand_p) {
				fprintf(STDERR,"  NC\n");
			} else {
				fprintf(STDERR," %d entry %d\n",
					stack[i]->frame_offset,
					stack[i]->entry);
			}
		}
	}
}
void
show_cands(candidate *base)
{
  int i;
	if (base == all_cands) {
		fprintf(STDERR,"all ");
	} else {
		fprintf(STDERR,"sorted ");
	}
	fprintf(STDERR,"loop of ");
	FPRINST(first);
	for (i = 0; i < n_candidates; i++) {
		if (base[i].name != NULL) {
			fprintf(STDERR,"at %d cand %s with %d type %d \n",i,
				base[i].name, base[i].estim,base[i].type);
		}
	}
} /* end show_cands */
#endif

static void
insert_preloads(candidate *stack[8])
{
  int i;
  NODE *last_prepended = last->forw;
  NODE *last_inserted = first->back;

	while (is_any_label(last_inserted)) {
		last_inserted = last_inserted->back;
	}
	/*insert fld instructions before the loop and fst instructions after. */
	for (i = 0; i <  n_candidates; i++) {
		if (sorted_cands[i].type == ARIT_B4_FST) {
			simulate_fld(&sorted_cands[i],stack);
			last_prepended = prepend(last_prepended,NULL);
			last_inserted = insert(last_inserted);
			switch (sorted_cands[i].size) {
				case LoNG:
					chgop(last_prepended,FSTPS,"fstps");
					chgop(last_inserted,FLDS,"flds");
					break;

				case DoBL:
					chgop(last_prepended,FSTPL,"fstpl");
					chgop(last_inserted,FLDL,"fldl");
					break;

				case TEN:
					chgop(last_prepended,FSTPT,"fstpt");
					chgop(last_inserted,FLDT,"fldt");
					break;

				default: fatal(__FILE__,__LINE__,"what oplength is that?\n");
			}
			last_prepended->op1 = sorted_cands[i].name;
			last_prepended->uses = FP0 | frame_reg;
			last_prepended->sets = FP0;
			last_inserted->op1 = last_prepended->op1;
			last_inserted->uses = FP0 | frame_reg;
			last_inserted->sets = FP0;
#ifdef DEBUG
			if (zflag) {
				fprintf(STDERR,"prepended ");
				FPRINST(last_prepended);
				fprintf(STDERR,"inserted ");
				FPRINST(last_inserted);
			}
#endif
		} 
	}
} /* end insert_preloads */

#define LAST_ARIT_	4
#define HAS_LAST_ARIT	5

static void
mark_last_arits()
{
  NODE *p;
  BLOCK *b = first_block;
  int i;
  candidate *cand;

	/* mark the last arit instruction for fst_b4 cands */
	for (ALL_THE_LOOP(p)) {
		p->usage = 0;
	}
	for (ALL_THE_LOOP(p)) {
		if (p == b->lastn->forw) {
			b = b->next;
			for (i = 0; i < n_candidates; i++) {
				sorted_cands[i].last_arit = NULL;
			}
		}
		if (   isfp(p)
		    && (cand = is_cand(p)) != null_cand_p) {
			if (   (is_fp_arit(p) || is_fld(p) || is_fstnst(p))
			    && cand->last_arit) {
				if (is_fstp(cand->last_arit)) {
					cand->last_arit->usage = HAS_LAST_ARIT;
				} else {
					cand->last_arit->usage = 0; /* unmark previous one */
				}
				cand->last_arit = p;
				p->usage = LAST_ARIT_;
			} else if (is_fstp(p)) {
				cand->last_arit = p; /* mark to allow setting */
			}
		}
	}
} /* end mark_last_arits */

static int
find_depth(candidate *stack[8])
{
  int last;
	/* find last entry */
	for (last = 0; stack[last] != NULL; last++) ;
	return last - 1;
} /* end find_depth */

/* For candidates that are first used, add fld instructions before the loop.
** For the others, only assign a slot by writting an index in the candidate
** structure.  Add fst instruction after the loop for all the candidates.
** Change the code to use the fp-stack instead.
*/
static void
regals_to_fps(BLOCK *b, candidate *stack[8])
{
  NODE *p;
  opopcode opop;
  candidate  *cur_cand = NULL;
  int depth;
  candidate *spare_stack[8];
  int i;
  static NODE *last_added = NULL;

	MARK_VISITED(b);
	set_cands_entries(stack);
	if (non_label_after(b->firstn) == non_label_after(first)) {
		if (!is_fstp(last->forw)) {
			last_added = last;
		} else {
			for (last_added = last->forw;
			     is_fstp(last_added);
			     last_added = last_added->forw )  ;
			last_added = last_added->back;
		}
	}
	/* go through the loop and change reference to candidates
	   to reference %st(i) */
	for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
		if (isfp(p)) {
			if ((cur_cand = is_cand(p)) != null_cand_p) {
				if (cur_cand->type == ARIT_B4_FST) {
					opop = mem2st(p->op);
					chgop(p,opop.op,opop.op_code);
					p->op1 = st_i[cur_cand->entry];
					p->op2 = NULL;
					if (is_fstp(p)) {
						simulate_fstp(cur_cand,cur_cand->entry,stack);
					} else if (p->op == FLD) {
						simulate_fld(cur_cand,stack);
					} else if (p->op !=FST && p->op !=FCOMP && p->op !=FCOMPP) {
						p->op2 = "%st";
					}
					p->uses = frame_reg | FP0;
					p->sets = FP0;
				} else if (cur_cand->type == FST_B4_ARIT) {
					if (is_fstp(p)) {
						if (cur_cand->entry < 0) { /*not yet on stack */
							depth = find_depth(stack);
							chgop(p,FSTP,"fstp");
							p->op1 = st_i[depth+1];
							simulate_fstp(cur_cand,depth+1,stack);
							if (cur_cand->needed) {
								p->op4 = "";
								last_added = insert(last_added);
								last_added->op1 = cur_cand->name;
								last_added->op2 = NULL;
								last_added->uses = FP0 | frame_reg;
								last_added->sets = FP0;
								switch (cur_cand->size) {
									case LoNG:
										chgop(last_added,FSTPS,"fstps");
										break;
									case DoBL:
										chgop(last_added,FSTPL,"fstpl");
										break;
									case TEN:
										chgop(last_added,FSTPT,"fstpt");
										break;
									default:
										fatal(__FILE__,__LINE__,
										"internal error, wrong length\n");
										break;
								}
							} else {
								last_added = insert(last_added);
								chgop(last_added,FSTP,"fstp");
								last_added->op1 = "%st(0)";
								last_added->op2 = NULL;
								last_added->uses = FP0 | frame_reg;
								last_added->sets = FP0;
								p->op4 = NULL;
							}
						} else {  /* already on stack (how can it be on stack?) */
							opop = mem2st(p->op);
							chgop(p,opop.op,opop.op_code);
							p->op1 = st_i[cur_cand->entry];
							p->op2 = NULL;
							if (p->op == FSTP) {
								simulate_fstp(cur_cand,cur_cand->entry,stack);
							}
						}
					} else if (cur_cand->entry >= 0) { /* not fstp, on stack */
						opop = mem2st(p->op);
						chgop(p,opop.op,opop.op_code);
						p->op1 = st_i[cur_cand->entry];
						if (is_fld(p)) {
							p->op2 = NULL;
							simulate_fld(cur_cand,stack);
						} else if (is_fst(p)) {
							p->op2 = NULL;
						} else if (p->op != FCOMP) p->op2 = "%st";
					} else {
						/* not fst, not on stack, should never happen because
						   the fstp treat left it on the stack. */
						if (is_fld(p)) {
							simulate_fld(cur_cand,stack);
						}
						if (FPOP(p)) {
							simulate_fst(stack);
						}
					}
				} else { /* type not arit_b4 and not fst_b4 */
					fatal(__FILE__,__LINE__,"fp_loop: funny type\n");
				}
			}	else { /* not a candidate */
				if (FPUSH(p->op)) {
					simulate_fld(null_cand_p,stack);
				}
				if (FPOP(p)) {
					simulate_fst(stack);
				}
				if (   p->op == FCOMPP
					|| p->op == FUCOMPP) {
					simulate_fcompp(stack);
				}
				if (p->op == FXCH) {
					if (p->op1 == NULL) {
						simulate_fxch(1,stack);
					} else {
						simulate_fxch(p->op1[4] - '0',stack);
					}
				}
			}
		} /* endif isfp */
	} /* end for loop */
	for (i = 0; i < 8; i++) {
		spare_stack[i] = stack[i];
	}
	if (   b->nextl
	    && IS_CLEAR(b->nextl)
	    && IN_THE_LOOP(b->nextl)) {
		regals_to_fps(b->nextl,stack);
	}
	for (i = 0; i < 8; i++ ) {
		if (spare_stack[i]) {
			assert(spare_stack[i] == stack[i]);
		}
	}

	if (   b->nextr
	    && IS_CLEAR(b->nextr)
	    && IN_THE_LOOP(b->nextr)) {
		regals_to_fps(b->nextr,stack);
	}
} /* end regals_to_fps */

/* Are there any FP instructions in the loop?
** Check in addition that there are no calls and no switch jumps.
*/
static boolean
has_fp()
{
  NODE *p;
  boolean retval = false;
	for (ALL_THE_LOOP(p)) {
		if (p ->op == CALL) {
			return false;
		}
		if (is_jmp_ind(p)) {
			return false;
		}
		if (isfp(p)) {
			/* True unless a call or switch jmp is found later*/
			retval = true;
		}
	}
	return retval;
} /* end has_fp */

static void
move_fxch_out_of_the_loop()
{
  NODE *p = next_fp(first);
  NODE *q = prev_fp(last);

	if (   p->op == FXCH
	    && q->op == FXCH
	    && !strcmp(p->op1,q->op1)) {
		move_node(p,first->back);
		move_node(q,last);
	}
}

static void
peep_in_the_loop()
{
  NODE *p,*q;
  NODE *pop = last->forw;
	COND_RETURN("peep_in_the_loop");
	if (!is_fst(pop)) {
		/* every candidate has an fst after the loop*/
		return;
	}
	while (   pop->op == FSTPS
	       || pop->op == FSTPL
	       || pop->op == FSTPT) {
		/* skip the preloaded ones */
		pop = pop->forw;
	}
	if (   pop->op != FSTP
	    || pop->op1[4] != '0') {
		/* non FST_B4_ARIT */
		return;
	}
	for (ALL_THE_LOOP(p)) {
		if (   p->op == FSTP
		    && p->op1[4] == '1'
		    && p->op4 == NULL) {
			q = next_fp(p);
			while (   (is_fp_arit(q) || is_fld(q))
			       && q->op1 && !isreg(q->op1)) {
				q = next_fp(q);
			}
			if (   q->op == FADD
			    && q->op1 && q->op1[4] == '1') {
				chgop(q,FADDP,"faddp");
				q->op1 = "%st";
				q->op2 = "%st(1)";
				DELNODE(p);
				DELNODE(pop);
				pop = pop->forw;
				if (pop->op != FSTP) {
					/* no more FST_B4 cands */
					return;
				}
			}
		}
	}
} /* end peep_in_the_loop */

/* Coordinate fp optimization. Mark all possible operands as candidates and
** find their estimated payoff. Sort the candidates and take the first few,
** as many as there are free stack slots. Then regals_to_fps() will do
** the changes to the function body.
*/
static void
do_fp_loop_opts ()
{
  candidate  *stack[8];
  int free_regs; /* number of fp stack slots available thru the entire loop */

	stack[0] = NULL; stack[1] = NULL;
	stack[2] = NULL; stack[3] = NULL;
	stack[4] = NULL; stack[5] = NULL;
	stack[6] = NULL; stack[7] = NULL;

	no_cands(stack);
	free_regs = find_max_free();
	if (free_regs == 0) {
		return;
	}
	if (!eiger) {
		fst2fstp();
	}
	n_candidates = find_candidates();
	if (n_candidates == 0) {
		return;
	}
	n_candidates = rem_overlapping_cands();
	if (n_candidates == 0) {
		return;
	}
	n_candidates = sort_candidates(free_regs);
	if (n_candidates > 0) {
		insert_preloads(stack);
		mark_last_arits();
		mark_blocks_in_loop();
		regals_to_fps(first_block,stack);
		peep_in_the_loop();
	} 
}/*end do_fp_loop_opts*/

typedef struct node_list1 {
			NODE *element;
			struct node_list1 *next;
			int direction;
			unsigned int idx_reg;
	} NODE_LIST1;


static NODE_LIST1 *overlapping_operands = NULL;
static NODE *pivot = NULL;
static unsigned int pivot_idx_reg;

static void
add_operand_2_disambig(NODE *p)
{
  NODE_LIST1 *e;
	for (e = overlapping_operands; e; e = e->next) {
		if (indexing(e->element) == indexing(p)) {
			return;
		}
	}
	e = (NODE_LIST1 *) getspace(sizeof(NODE_LIST1));
	e->element = p;
	e->next = overlapping_operands;
	e->idx_reg = indexing(p);
	e->direction = 0;
	overlapping_operands = e;
} /* end  add_operand_2_disambig */

/* It is assumed that the loop has no call instructions and has fp instructions.
** The loop has to be one basic block.
** Exactly one fstp of an operand which is zero offset from a register.
** The register is not changed in the loop.
** Other indexing registers are changed exactly once, by add/sub instructions.
** They are used as pointers, not as array indices.
*/
static boolean
has_run_time_potential()
{
  NODE *p;
  int i;
  NODE_LIST1 *e;
  int e_set;
  boolean satisfied = false;
  int m;
  NODE *try_pivot = first;

	pivot = NULL;
	overlapping_operands = NULL;
	if (first_block->lastn != last) {
		/* more than one block*/
		return false;
	}
	/* Find a pivot. If it is the operand of an integer instruction give
	** it up and find another one.
	*/
	while (! satisfied) {
		pivot = NULL; /* find a candidate to be a pivot */
		for (p = try_pivot->forw; p != last; p = p->forw) {
			if (is_fstp(p)) {
				for (i = first_reg; i <= last_reg; i++) {
					if (iszoffset(p->op1,all_regs[i].names[3])) {
						try_pivot = pivot = p;
						pivot_idx_reg = all_regs[i].reg;
						break;
					}
				}
			}
		}
		if (pivot == NULL) {
			satisfied = true;
			break;
		} else {
			/* verify the pivot */
			satisfied = true;
			for (ALL_THE_LOOP(p)) {
				if (   (   (   (m = 1, p->op1 && !strcmp(p->op1,pivot->op1))
				            || (m = 2, p->op2 && !strcmp(p->op2,pivot->op1))
				            || (m = 3, p->op3 && !strcmp(p->op3,pivot->op1)))
				        && (   isvolatile(p,m)
				            || !is_fp_optimable(p)))
			 	    || (p->sets & pivot_idx_reg)) {
						pivot = NULL;
						satisfied = false; /* will try to find another pivot*/
						break;
				}
			}
		}
	} /* while loop */
	if (pivot == NULL) {
		return false;
	}
	/* Collect all undisambiguated operands */
	/* For now, don't allow operands with index register, only base. */
	for (ALL_THE_LOOP(p)) {
		p->sets |= msets(p);
		p->uses |= muses(p);
	}
	for (p = first; p != pivot; p = p->forw) {
		if (   (   ismem(p->op1)
		        && (   (p->op1[0] != '(')
		            || strchr(p->op1,',')))
		    || (   ismem(p->op2)
		        && (   (p->op2[0] != '(')
		            || strchr(p->op2,',')))
		    || (   ismem(p->op3)
		        && (   (p->op3[0] != '(')
		            || strchr(p->op3,',')))) {
			return false;
		}
		if (   ((p->sets | p->uses) & MEM)
		    && (   indexing(p) != pivot_idx_reg
		        && seems_same(p,pivot,false))) {
			add_operand_2_disambig(p);
		}
	}
	for (p = pivot->forw; p != last; p = p->forw) {
		if (   (   ismem(p->op1)
		        && (   p->op1[0] != '('
		            || strchr(p->op1,',')))
		    || (   ismem(p->op2)
		        && (   p->op2[0] != '('
		            || strchr(p->op2,',')))) {
			return false;
		}
		if (   ((p->sets | p->uses) & MEM)
		    && (   indexing(p) != pivot_idx_reg
		        && seems_same(pivot,p,false))) {
			add_operand_2_disambig(p);
		}
	}
	for (ALL_THE_LOOP(p)) {
		p->sets = sets(p);
		p->uses = uses(p);
	}
	/* The overlapping operands have to change only once, and by add/sub */
	for (e = overlapping_operands; e; e = e->next) {
		e_set = false;
		for (ALL_THE_LOOP(p)) {
			if (p->sets & e->element->uses & regs) {
				if (e_set) {
					switch (p->op) {
						case ADDL: case INCL:
							if (e->direction != 1) {
								return false;
							}
							break;

						case SUBL: case DECL:
							if (-1 != e->direction) {
								return false;
							}
							break;

						default: return false;
					}
				} else {
					if (   p->op == ADDL
					    || p->op == INCL) {
						e->direction = 1;
						e_set = true;
					}
					else if (   p->op == SUBL
					         || p->op == DECL) {
						e->direction = -1;
						e_set = true;
					} else {
						return false;
					}
				}
			}
		}
	}
	return true;
} /* end has_run_time_potential */

/*change the code to make it like:
** 1. Instructions to compare the index registers and jump to either loop.
** 2. One copy of the loop.
** 3. Jump after the second copy.
** 4. Second copy of the loop.
*/

static void
duplicate_the_loop()
{
  NODE *p = first;
  NODE *p1;
  NODE_LIST1 *e;
  NODE *new_first,*new_last;
  BLOCK *b;
	/* memory disambiguation before the loop */
	for (e = overlapping_operands; e; e = e->next) {
		p = prepend(p,NULL);
		chgop(p,CMPL,"cmpl");
		p->op1 = itoreg(pivot_idx_reg);
		if (e->idx_reg) {
			p->op2 = itoreg(e->idx_reg);
		} else if (ismem(e->element->op1)) {
			p->op2 = e->element->op1;
		} else if (ismem(e->element->op2)) {
			p->op2 = e->element->op2;
		} else {
			fatal(__FILE__,__LINE__,"fp_loop: unexpected memory set\n");
		}
		p1 = insert(p);
		if (e->direction > 0) {
			chgop(p1,JBE,"jbe");
		} else if (e->direction < 0) {
			chgop(p1,JAE,"jae");
		} else
			chgop(p1,JNE,"jne");
		p1->op1 = first->opcode;
	}
	/* duplicate the loop */
	new_first = p1 = first->back;
	for (ALL_THE_LOOP(p)) {
		p1 = duplicate(p,p1);
	}
	new_first = new_first->forw;
	new_last = first->back;
	new_first->opcode = newlab();
	new_last->op1 = new_first->opcode;
	new_sets_uses(new_last);
	add_label_to_align(new_first->opcode);
	/* after the loop jump to the end of the second loop */
	p1 = insert(new_last);
	chgop(p1,JMP,"jmp");
	if (islabel(first_block->next->firstn)) {
		p1->op1 = first_block->next->firstn->opcode;
	} else {
		p = prepend(first_block->lastn->forw,NULL);
		p->op = LABEL;
		p->opcode = newlab();
		p1->op1 = p->opcode;
	}
	new_sets_uses(p1);
	/* make the loop the first copy of the loop */
	first = new_first;
	last = new_last;
	bldgr(false);
	for (b = b0.next; b; b = b->next) {
		if (b->firstn == first) {
			first_block = b;
			break;
		}
	}
}/* end duplicate_the_loop */

static void
set_a_candidate(candidate *stack[8])
{
NODE *p;
	no_cands(stack);
	n_candidates = 1;
	sorted_cands[0].frame_offset = 0;
	sorted_cands[0].name = pivot->op1;
	sorted_cands[0].estim = 0;
	sorted_cands[0].size = OpLength(pivot);
	sorted_cands[0].type_decided = true;
	sorted_cands[0].needed = true; /* not ideal but safe */
	sorted_cands[0].last_arit = NULL;
	sorted_cands[0].entry = 0;
	/* set the type */
	for (ALL_THE_LOOP(p)) {
		if (   (   p->op1
		        && !strcmp(p->op1,pivot->op1))
		    || (   p->op2
		        && !strcmp(p->op2,pivot->op1))) {
			if (is_fstp(p)) {
				sorted_cands[0].type = FST_B4_ARIT;
			} else  {
				sorted_cands[0].type = ARIT_B4_FST;
			}
			break;
		}
	}
} /* end set_a_candidate */

static void
try_again_w_rt_disamb()
{
  candidate  *stack[8];

	stack[0] = NULL; stack[1] = NULL;
	stack[2] = NULL; stack[3] = NULL;
	stack[4] = NULL; stack[5] = NULL;
	stack[6] = NULL; stack[7] = NULL;

	duplicate_the_loop();
	set_a_candidate(stack);
	insert_preloads(stack);
	regals_to_fps(first_block,stack);
	check_fp();
}

BLOCK *
find_cur_block(p) NODE *p;
{
  BLOCK *b;
  NODE *q;
	for (b = b0.next; b; b = b->next) {
		for (q = b->firstn; q != b->lastn->forw; q = q->forw) {
			if (q == p) return b;
		}
	}
/* NOTREACHED */
} /* end find_cur_block */


/* Driver of the fp loop optimization. Could be together with the int loop
** optimization. This way, however, we save time. We do not have to execute
** check_fp() when int loop optimization changed the function. We do check_fp()
** one time per function.
** This also has to do with doing only innermost loops. If we do outer loops, we
** have to either do check_fp() every time, or update b->marked.
*/

void
fp_loop()
{
  NODE *pm;
  char *jtarget;
  BLOCK *b;

#ifdef FLIRT
  static int numexec=0;
#endif

	COND_RETURN("fp_loop");

#ifdef FLIRT
	++numexec;
#endif

	bldgr(false);
	check_fp();		/* FP stack usage by the block */
	regal_ldanal(); /* what candidates are dead after the loop */
	first = last = NULL;
	b = b0.next;
	for (ALLN(pm)) {
		if (pm == b->lastn->forw) {
			b = b->next;
		}
		if (   islabel(pm)
		    && lookup_regals(pm->opcode,first_label)) {
			first = pm;
			first_block = b;
			continue;

		} else if (   iscbr(pm)
		           && first
		           && !strcmp(pm->op1,first->opcode)) {
			last = pm;
		} else {
			continue;
		}
#ifdef DEBUG
		if (zflag) {
			fprintf(STDERR,"loop of "); 
			FPRINST(first);
		}
#endif
		/*do only very simple loops, good enough for fp loops. */
		if (is_simple_loop(&jtarget) != 1) {
#ifdef DEBUG
			if (zflag) {
				fprintf(STDERR,"loop not very simple: ");
				FPRINST(first);
			}
#endif
			continue;
		}

		/*At this point we have a loop and it is simple.*/
		if (has_fp()) {
			COND_SKIP(continue,"loop %d %s\n",second_idx,first->opcode,0);
			FLiRT(O_FP_LOOP,numexec);
			do_fp_loop_opts();
			if (has_run_time_potential()) {
				COND_SKIP(continue,"Loop %d %s\n",second_idx,first->opcode,0);
				FLiRT(O_TRY_AGAIN_W_RT_DISAMB,numexec);
				try_again_w_rt_disamb();
				b = find_cur_block(pm); /* new flow graph, need to update */
			}
			/*unconditional, fpeep may have created opportunities*/
			if (!eiger) {
				move_fxch_out_of_the_loop();
			}
		}
	} /* end for loop */
} /* end fp_loop */

static boolean
ok_to_decmpl(compare) NODE *compare;
{
NODE *p;
unsigned int reg;
unsigned int idx;
int step,limit;
NODE *increment;
	if (!isconst(compare->op1) || !isreg(compare->op2)) return false;
	if (TEST_CF(last->op)) return false;
	reg = setreg(compare->op2);
	if (last->forw->nlive & reg) return false; 
	for (p = compare->back; p != first->back; p = p->back) {
		if (p->sets & CONCODES)
			if (!(p->sets & reg)) {
				return false;
			} else {
				increment = p;
				break; /* the loop increment instruction */
			}
	}
	limit = (int)strtoul(compare->op1+1,(char **)NULL,0);
	switch (p->op) {
		case INCL: case DECL:
			break;
		case ADDL: case SUBL:
			if(*(p->op1) != '$')
				return false;
			step = (int)strtoul(p->op1+1,(char **)NULL,0);
			if (step == 0 || limit % step)
				return false;
			break;
		case LEAL:
			step = (int)strtoul(p->op1,(char **)NULL,0);
			if (step == 0 || limit % step)
				return false;
			break;
		default:
			return false;
	}
	for (ALL_THE_LOOP(p)) {
		if (p != compare) {
			if ((uses_but_not_indexing(p) & ~p->sets) & reg) return false;
			if (((idx = indexing(p)) & reg)) {
				if (p->op1 && strchr(p->op1,',') && has_scale(p->op1)) 
					return false;
				if (p->op2 && strchr(p->op2,',') && has_scale(p->op2))
					return false;
			}
			/* funny way to test: there is no scale, so test if the indexing
			** is exactly reg and there is a ',' ergo it appears twice, which
			** is what we look for
			*/
			if (idx == reg) {
				if ((p->op1 && strchr(p->op1,',')) ||
					(p->op2 && strchr(p->op2,',')))
					return false;
			}
		}
		if (p != increment && (p->sets & reg))
			return false;
	}
	return true;
}/*end ok_to_decmpl*/

static void
add_fix(jtarget,imm,operand) char *jtarget, *imm, *operand;
{
NODE *p,*pnew;
char *new_label;
	/* find the label which is the jump target */
	for (ALLN(p)) {
		if (is_any_label(p) && !strcmp(p->opcode,jtarget)) {
			break;
		}
	}
	/* go back to the beginning of the basic block - skip labels */
	pnew = p;
	while (is_any_label(pnew)) pnew = pnew->back;
	/* add a jmp to jtarget - at the end of the previous block */
	pnew = insert(pnew);
	chgop(pnew,JMP,"jmp");
	pnew->op1 = jtarget;
	pnew->uses = pnew->sets = 0;
	/* after the jump, place the new label - beginning of this block */
	pnew = prepend(pnew->forw,NULL);
	pnew->op = LABEL;
	pnew->opcode = newlab();
	new_label = pnew->opcode;
	pnew->uses = pnew->sets = 0;
	/* after the new label put the addl instruction */
	pnew = insert(pnew);
	chgop(pnew,LEAL,"leal");
	pnew->op1 = getspace(LEALSIZE);
	sprintf(pnew->op1,"%s(%s)",1+imm,operand);
	pnew->op2 = operand;
	pnew->uses = pnew->sets = setreg(operand);
	/* change jmps to jtaarget to jump to the new label */
	for (ALL_THE_LOOP(p)) {
		if (is_any_br(p) && !strcmp(p->op1,jtarget))
			p->op1 = new_label;
	}
}/*end add_fix*/

static void
decmpl(compare,jtarget) NODE *compare; char *jtarget;
{
NODE *p,*q;
char sign,*t,name[100];
int x,size;
unsigned int idx;
int m;
int scale;
char *c;
NODE *new_init;
	idx = compare->uses;
	for (p = first->back; !is_any_label(p); p = p->back)
		if (p->sets & idx) break;
	if (is_any_label(p)) {
		return;
	}
	if (! (  /* loop counter initialized to zero before the loop */
		(p->op == XORL && samereg(p->op1,p->op2)) ||
		(p->op == SUBL && samereg(p->op1,p->op2)) ||
		(p->op == MOVL && !strcmp(p->op1,"$0")))
	)	{
		return;
	}
	/*if there is a compare before the loop that uses idx and can not be
	**eliminated by cmp-imm-imm, e.g. because the condition code are live
	**at the target of the jcc, which means we must do the compare, then
	**we need to use the old value for the compare and therefore can't
	**optimize.
	**This is only true if we set the new value to the idx reg before that
	**cmp inst. Now that this is changed, i.e. that we set the new value
	**before the entry label, this is no longer necessary.
	**We may still want to change refs to the reg into ref to $0, to make
	**the setting of the reg to zero a dead inst which may later be deleted.
	*/

	for (q = p->forw; q != first; q = q->forw) {
		if (q->op == CMPL && setreg(q->op2) == idx) {
			if (isimmed(q->op1)) {
				char *tmp = q->op2;
				q->op2 = "$0";
				/*use cmp imm imm to change the code. If it fails then
				**need to make the code legal again.
				*/
				if (!cmp_imm_imm(q,q->forw))
					q->op2 = tmp;
			} 
		}
	}
	if (jtarget) add_fix(jtarget,compare->op1,p->op2);
	for (p = p->forw; p != first; p = p->forw) {
		if (p->op == CMPL) {
			if (isreg(p->op1) && setreg(p->op2) == idx) {
			chgop(p,TESTL,"testl");
			p->op2 = p->op1;
			continue;
			}
		} else {
			/*for non compare instructions, a simple replace*/
			if (setreg(p->op1) == idx) p->op1 = "$0";
			if (setreg(p->op2) == idx) p->op2 = "$0";
		}
	}
	new_init = prepend(first,NULL);
	size = (int)strtoul(compare->op1+1,(char **)NULL,0);
	chgop(new_init,MOVL,"movl");
	new_init->op1 = getspace(ADDLSIZE);
	sprintf(new_init->op1,"$%d",-size);
	new_init->op2 = compare->op2;
	new_init->uses = 0;
	new_init->sets = setreg(compare->op2);
	for (ALL_THE_LOOP(p)) {
		if (indexing(p) & idx) {
			if ((t = strchr(p->op1,'(')) != NULL) {
				m = 1;
			} else if ((t = strchr(p->op2,'(')) != NULL) {
				m = 2;
			} else if ((t = strchr(p->op3,'(')) != NULL) {
				m = 3;
			}
			for (x = 0; x < 100; x++) name[x] = (char) 0;
			(void) decompose(p->ops[m],&x,name,&scale);
			c = (p->ops[m][0] == '*') ? "*" : "";
			x += size;
			if (x > 0) sign = '+';
			else sign = '-';
			p->ops[m] = getspace(strlen(p->ops[m]) + 11);
			if (x == 0)
				sprintf(p->ops[m],"%s%s%s",c,name,t);
			else if (name[0]) {
				if (fixed_stack && strstr(name,".FSZ"))
					sprintf(p->ops[m],"%s%d%c%s%s",c,x,'+',name,t);
				else
					sprintf(p->ops[m],"%s%s%c%d%s",c,name,sign,x,t);
			}
			else
				sprintf(p->ops[m],"%d%s",x,t);
		}
	}
	p = insert(last);
	chgop(p,LEAL,"leal");
	p->op1 = getspace(LEALSIZE);
	sprintf(p->op1,"%d(%s)",size,compare->op2);
	p->op2 = compare->op2;
	p->uses = p->sets = setreg(compare->op2);
	DELNODE(compare);
}/*end decmpl*/

void
loop_decmpl()
{
NODE *pm;
char *jtarget = NULL;
	COND_RETURN("loop_decmpl");
	for (ALLN(pm)) {
		jtarget = NULL;
		last = pm->forw;
		if (!last || last == &ntail) return;
		if ((pm->op == CMPL) && (iscbr(last))) {
			if ((first = is_back_jmp(last)) == NULL) {
#ifdef DEBUG
				if (zflag > 1) {
					fprintf(STDERR,"not a back jmp");
					FPRINST(last);
				}
#endif
				continue;
			}
			if (is_simple_loop(&jtarget) == 0) {
#ifdef DEBUG
				if (zflag) {
					fprintf(STDERR,"loop not simple: ");
					FPRINST(first);
				}
#endif
				continue;
			}
			COND_SKIP(continue,"loop %d %s\n",second_idx,first->opcode,0);
			if (ok_to_decmpl(pm)) decmpl(pm,jtarget);
		}/*endif cmp-jcc*/
	}/*end for loop*/
}/*end loop_decmpl*/

/********************
 *	loop_detest		*
 ********************/

#define is_AND(p)  ( (p)->op == ANDL || (p)->op == ANDW || (p)->op == ANDB )
#define is_OR(p)   ( (p)->op == ORL || (p)->op == ORW || (p)->op == ORB )
#define is_XOR(p)  ( (p)->op == XORL || (p)->op == XORW || (p)->op == XORB )
#define is_TEST(p) ( (p)->op == TESTL || (p)->op == TESTW || (p)->op == TESTB ) 

#define set_OF_CF_to_0(p) ( is_AND(p) || is_OR(p) || is_XOR(p) || is_TEST(p) )
		
ok_to_detestl(decl, go_further) NODE *decl; int go_further;
{
NODE *p;

	for( p=first->back; !is_any_label(p); p=p->back )
		if( p->sets & last->back->uses ) 
			break;
	if( is_any_label(p) ) return false;

	/* is const or bounded */
	if( (is_move(p) || is_AND(p)) && isconst(p->op1) )
	{
		int n=(int)strtoul(p->op1+1,(char **)NULL,0);
		if(go_further == 1 && n < (int) 0x7fffffff - 1) 
			/* EMPTY */
			;
		else if( n > (int) 0x80000000 + 1 ) 
			/* EMPTY */
			;
		else 
			return false;
	}
	else 
		return false;	
	
	for( p=decl->back; p!=first; p=p->back )
		if(!((p->sets & CONCODES) == 0 || set_OF_CF_to_0(p)))
			return false;
	return true;
}		

void
de_testl()
{
	move_node(last->back, first->back);
}
	
void
loop_detestl()
{
NODE *pm,*p;
int go_further;
char *jtarget = NULL;

#ifdef FLIRT
static int numexec=0;
#endif

#ifdef DEBUG
	COND_RETURN("loop_detestl");
#endif

#ifdef FLIRT
	++numexec;
#endif

	for (ALLN(pm)) {
		jtarget = NULL;
		last = pm->forw;
		if (!last || last == &ntail) return;
		if (   pm->op == TESTL
			&& isreg(pm->op1)
			&& !strcmp(pm->op1,pm->op2) 
			&& (   last->op == JG
				|| last->op == JGE 
				|| last->op == JL
				|| last->op == JLE ) ) 
		{
			for(p=pm->back; p; p=p->back)
			{
				if( is_any_br(p) || is_any_label(p) || p->op == CALL ) 
				{ 
					go_further = 0; 
					break; 
				}
				if( p->sets & CONCODES ) 
				{
					if( p->op == INCL && (last->op == JL || last->op == JLE) )
						go_further = 1;
					if( p->op == DECL && (last->op == JG || last->op == JGE) )
						go_further = 2;
					else 
						go_further = 0;
					break;
				}
			}	 
			if( !go_further ) continue;
			if ((first = is_back_jmp(last)) == NULL) continue;
			if (is_simple_loop(&jtarget) == 0) continue;
			COND_SKIP(continue,"loop_detestl %d %s\n",second_idx,first->opcode,0);
			if (ok_to_detestl(p,go_further)) 
			{
				de_testl();
				FLiRT(O_DETESTL,numexec);
			}
		}/*endif cmp-jcc*/
	}/*end for loop*/
}/*end loop_detestl*/

/*  END loop_detest */

/******* D e i n d e x i n g *******/
/* simple_deindex() accomplish deindexing in simple case ,which doesn't
 * require additional register.
 * Now without jtarget as far as  great deal of cases in SPEC 
 * satisfy this limitation.
 *
 * External dependecies : NODE's member idxs is used and set arbitrary .
 */

int
simple_deindex(pattern,base,indx,jtarget) 
char* pattern; 
unsigned base,indx;
char *jtarget;
{
NODE *p,*p1,*pi,*pnew,*set_idx=NULL;
int   i,cnt=0;
char *ch,*tmp;
int nauto;
int scale, n;
unsigned live0,live1,live2=0;

	COND_SKIP(return,"simple_deindex %d",second_idx,0,0 );

	if (target_cpu == P5) return false; /* not a win on pentium */
	
	for(ALL_THE_LOOP(p))
	{
		p->idxs = 0;
		if( p->sets & indx )
		{
			/* %index is set more than 1 time , not usual case  */
			if( set_idx ) return false ;
			if( (p->op == ADDL || p->op == SUBL) && isimmed(p->op1) 
			||   p->op == DECL || p->op == INCL )
			{
				set_idx = p;
				continue;
			}
			/* in general this case requires additional reg */
			else return false ;
		}	
		/* mark all nodes containing the pattern */
		if( !(p->uses & base) ) continue;
		for(i=1;i<=3;i++)
		{
			if( (ch=strchr(p->ops[i],'(')) == NULL ) continue ;
			cnt++; /* The number of marked nodes */
			p->idxs = i;
			break;
		}		
	}		

	if (cnt < 2 ) return false ; /* No profit */
 
/* Determ scale and increment */
	if( pattern[10] != ',' ) 
		scale = 1; /* if no scale factor */
	else
		scale = pattern[11] - '0';	

	if( set_idx->op == INCL || set_idx->op == DECL ) 
		n = 1;
	else 
		n = (int)strtoul(set_idx->op1+1,(char **)NULL,0) ;

	n = scale * n ;

	/* Does base alive immediately after the loop or at the jtarget ? */	
	for(p=last->forw;p!=&ntail;p=p->forw) if( !is_any_label(p)) break;
	live1 = (p->nlive & base | p->uses & base) ? p->nlive | p->uses : 0 ;
	if( jtarget!=NULL ){ 
		for (ALLN(p1)) {
			if (is_any_label(p1) && !strcmp(p1->opcode,jtarget)) break;
		}
		pi = p1;
		while (is_any_label(pi)) pi = pi->forw;
		live2 = (pi->nlive & base | pi->uses & base) ? pi->nlive | pi->uses : 0 ;
	}		
	pi = first;
	while( is_any_label(pi)) pi=pi->forw;
	live0 = pi->nlive | pi->uses ;
	
	
	if ( live1 || live2 )  /* base does alive */	
	{
	/* save base reg in regal */
		nauto=inc_numauto(4);
		pnew = insert(first->back); 
		chgop(pnew,MOVL,"movl");
		pnew->op1 = itoreg(base);
		pnew->op2 = (char*)getspace(NEWSIZE);
		sprintf(pnew->op2,"-%d(%%ebp)",nauto );
		pnew->uses = base | EBP ;
		pnew->sets = MEM;
		pnew->nlive = live0 | base;
	}	
 
	/* restore base reg immediately after the loop */
	if( live1 )
	{
		pnew = insert(last); 
		chgop(pnew,MOVL,"movl");
		pnew->op2 = itoreg(base);
		pnew->op1 = (char*)getspace(NEWSIZE);
		sprintf(pnew->op1,"-%d(%%ebp)",nauto);
		pnew->uses = MEM | EBP;
		pnew->sets = base;
		pnew->nlive = live1;
	}	

	/*	restore base at jtarget point */
	/* Following piece was copied from add_fix() */
	if ( live2 )
	{
		char * new_label;
		pi = p1;
		while (is_any_label(pi)) pi = pi->back;
		/* insert a jmp to jtarget - end of previous block */
		pnew = insert(pnew);
		chgop(pnew,JMP,"jmp");
		pnew->op1 = jtarget;
		pnew->uses = pnew->sets = 0;
		/* after the jump, place the new label */
		pnew = prepend(pnew->forw,NULL);
		pnew->op = LABEL;
		pnew->opcode = newlab();
		new_label = pnew->opcode;
		pnew->uses = pnew->sets = 0;
		/* after the new label put the restore instruction */
		pnew = insert(pnew); 
		chgop(pnew,MOVL,"movl");
		pnew->op2 = itoreg(base);
		pnew->op1 = (char*)getspace(NEWSIZE);
		sprintf(pnew->op1,"-%d(%%ebp)",nauto);
		pnew->uses = MEM | EBP ;
		pnew->sets = base;
		pnew->nlive = live2;
		
	/* change jmps to jtaarget to jump to the new label */
		for (ALL_THE_LOOP(p)) {
			if (is_any_br(p) && !strcmp(p->op1,jtarget))
				p->op1 = new_label;
		}
	}	 

/* insert    leal (%base,%indx,s),base   */
	p = insert(first->back);
	chgop(p,LEAL,"leal");
	p->op1 = (char*)getspace(NEWSIZE);
	strcpy(p->op1,pattern);
	p->op2 = itoreg(base);
	p->idxs = 0;
	p->nlive = first->back->nlive | base;
	p->sets = base;
	p->uses = base|indx;
	p->nlive = live0;
	
/* insert  addl	 N,%base   */

	if( set_idx )
	{
		p =prepend(set_idx,itoreg(base));
		if( set_idx->op == ADDL || set_idx->op == INCL )
			chgop(p,ADDL,"addl");
		else
			chgop(p,SUBL,"subl");
		p->op1 = (char*)getspace(ADDLSIZE);
		sprintf(p->op1,"$%d",n);
		p->idxs = 0;
		p->sets = base;
		p->uses = base;
		p->nlive = set_idx->nlive | base ;
	}

/* change pattern to (%base) throw the loop */
	for(ALL_THE_LOOP(p))
	{
		if ( !p->idxs ) continue;
		tmp = getspace(strlen(p->ops[p->idxs]));
		strcpy(tmp,p->ops[p->idxs]);
		ch = strchr(tmp,'(');
		sprintf(ch,"(%s)",itoreg(base));
		p->ops[p->idxs] = tmp;
		p->uses = p->uses & (~indx) ;
	}		
	return true;
}
	
	
   	
/* index_ops_2_regals() adds after each instruction, using base-indexed memory
 * reference, the same instruction but replaces base-indexed memory reference
 * with regal. Operands differ only in the displacement are replaced with
 * the same regal e.g.
 *
 * movl	%r1,xxx(%r2,%r3,S)  ==>   	movl %r1,xxx(%r2,%r3,S)
 *									movl %r1,-N(%ebp)         (*)
 * Where N is arbitrary number greater than numauto.
 * There is an entry for each pattern of the base-indexed memory reference 
 * (%r1,%r2,S)  in the iops_tbl[]. Pointers to each pair (*) are storied
 * in a chain of ir structures . Pointers to the first and the last link of 
 * the chain are storied in the iops_tbl. If content of %r1 or %r2 are
 * canged throw a loop iops_tbl[].flag is set. 
 * 
 *  loop before index_ops_2_regals()
 * .L1:
 * 		movl 20(%esi,%edi,4),%ecx
 *		addl %ebx,%ecx
 *		movl %ecx,20(%esi,%edi,4)
 *		incl %edi
 *		cmpl $20,%edi
 *		jle	 .L1
 *  
 *  loop after  index_ops_2_regals()
 * .L1:
 *      movl 20(%esi,%edi,4),%ecx
 * 	=>	movl -200(%ebp),%ecx
 *		addl %ebx,%ecx
 *      movl %ecx,20(%esi,%edi,4)
 *	=>	movl %ecx,-200(%ebp)
 *		incl %edi                   <- flag = true
 *		cmpl $20,%edi
 *		jle	 .L1
 *  
 * Then loop_regal() operates with this pseudo_regals as legal regals.
 *
 * If loop_regal() allocates pseudo_regal in register R, regals_2_index_ops()
 * rewrites loop , using obtained register as a base reg for memory referen-
 * ces , else it deletes pseudo_regals and returns the code to original
 * state. If iops_tbl[].flag is set it moves instruction 
 * leal	(%r1,%r2,S),R inside a loop because of %r1 or %r2 change .
 *
 *  loop before regals_2_index_ops()  R = %edx
 *
 *      movl -200(%ebp),%edx 
 * .L1:
 *      movl 20(%esi,%edi,4),%ecx
 *  =>  movl %edx,%ecx
 *		addl %ebx,%ecx
 *      movl %ecx,20(%esi,%edi,4)
 *	=>	movl %ecx,%edx
 *		incl %edi
 *		cmpl $20,%edi
 *		jle	 .L1
 *  
 *  loop after regals_2_index_ops()  
 *
 * .L1:
 *  =>  leal (%esi,%edi,4),%edx
 *  =>  movl 20(%edx),%ecx
 *		addl %ebx,%ecx
 *	=>	movl %ecx,20(%edx)
 *		incl %edi
 *		cmpl $20,%edi
 *		jle	 .L1
 *  
 */	 
         
#define NCAND 50 
/* The max number of pseudo_regals 
 */   

typedef struct ir_t{
	NODE* old; /* Pointer to the original instruction */
	NODE* new; /* Pointer to the bound pseudo_regal instruction  */ 
    struct ir_t * next;
} ir;	

 /* Each structure represents one pattern of base_ind_op */  
struct {
	char tbl_entry[NEWSIZE]; /* Pattern of base_ind_op without displacement */
	char regal[NEWSIZE];     /* Pseudo_regal bound with  base_ind_op */ 
    ir* first; 				 
	ir* last;
    unsigned ib_regs;
	boolean flag;
} iops_tbl[NCAND];
int icnt;

boolean
index_ops_2_regals(jtarget) char * jtarget;
{
extern int numauto;
NODE *p,*np;
NODE* p1;
int l;
ir* irp;
char *ch,*s;
int i,j,k;
int what_to_do;	
unsigned ib_regs,base,index;
int cnt;
int numauto_ = numauto+200;
/*  200 - arbitrary positive number , presumably greater then the number of
 *  new regals which may be created by loop_regal for spilling.
 */
boolean use_base, use_index , set_base , set_index ;
icnt = 0;

	COND_SKIP(return,"index_ops_2_regals %d %s",second_idx,first->opcode,0); 

	memset(iops_tbl,0,NCAND*sizeof(iops_tbl[0]));
	
	for (ALL_THE_LOOP(p)){
		j=0;
		for (i=1;i<=3;i++) { 
			if (p->ops[i] && strchr(p->ops[i],',') ) {
				j=1;
				break;
			}
		}	
		if (j == 0) continue; /*no indexing in this instruction */
		if (*p->ops[0] == 'f') continue; /*if it's a fp instruction */
		ch = strchr(p-> ops[i],'('); 
		j = 0;
		k = 1;
		/*Is the operand already stored in the table ? */
		while (j < icnt && (k = strcmp(ch,iops_tbl[j].tbl_entry)) != 0) j++;
		if (k == 0) { /* operand not found in the table */
			irp = ( ir * ) getspace(sizeof(ir));
			iops_tbl[j].last->next = irp; 
			iops_tbl[j].last = irp;
			irp->next = NULL;
			irp->old = p;
			p1 = irp->new = insert(p);
			p1->op = p->op;
			for(l = 0; l <= 3; l++){
				if (l == i){  
					p1->ops[l] = getspace(NEWSIZE);
					strcpy(p1->ops[l],iops_tbl[j].regal);  
				}else 
					p1->ops[l] = p->ops[l];
			}			
            p1->uses = p->uses & (~iops_tbl[j].ib_regs) | frame_reg;
            p1->sets = p->sets;
		} else {
			if (icnt > NCAND-1 ) return false;
			/* Check that base and index are changed after last indirect
			** reference
 			*/
 		 	base = setreg(ch+1);
			s = strchr(ch,',');
			index = setreg(s+1);
			ib_regs = base | index;

			if (base == 0 && (target_cpu == P6 || target_cpu == blend))
				continue;
						
			use_base = false;
			use_index= false;
			set_base = false;
			set_index= false;
			
			what_to_do = 1; 
			cnt = 0;
 			for (ALL_THE_LOOP(np)) 
 			{
				if (np->uses & ib_regs)
				{
					if ( np->op1 && (s=strchr(np->op1,'(')) && !strcmp(ch,s)
					||   np->op2 && (s=strchr(np->op2,'(')) && !strcmp(ch,s) ) 
					{
				 		if (np->op != LEAL && *np->ops[0]!= 'f' ) cnt++;
				 		if (cnt && (set_base || set_index)) 
				 		{ 
				 			what_to_do = 0; 
				 			break;
				 	    	/* base or index is set between two 
				 	    	 *  base_indexed instructions */  
				 	    }
					}
					else
					{
						if ( np->uses & base ) use_base = true;
						if ( np->uses & index ) use_index = true;
					}
				} 	
				if ( np->sets & base ) set_base = true;
				if ( np->sets & index ) set_index = true;
			}
			/* if we have only 1 base_ind_operand and we are need 
			 * instruction leal (..),%r before it , there is no
			 * profit here .
			 */  
			if ((set_base || set_index) && cnt < 2 ) continue;
			if (cnt == 0 ) continue; /*no useful instructions*/
			if( what_to_do == 0 ) continue; /*disqualified */

			if ( base && base!=EBP && base!=ESP 
			&& !use_index && !use_base && !set_base )
			/* more usial case */
			{
				if (simple_deindex(ch, base, index, jtarget)) continue;
	 		}
			if ( target_cpu == P5 ) continue;
			iops_tbl[j].flag = set_base || set_index;
			icnt++;
			irp = ( ir * ) getspace(sizeof(ir));
			(void) strcpy(iops_tbl[j].tbl_entry,ch);
			iops_tbl[j].ib_regs = ib_regs;
			iops_tbl[j].first = irp;
			iops_tbl[j].last = irp;
			irp->next = NULL;
            if (!fixed_stack)
                sprintf(iops_tbl[j].regal,"-%d(%%ebp)",numauto_+=4);
            else
                sprintf(iops_tbl[j].regal,"-%d+%s%d(%%esp)",numauto_+=4,frame_string,func_data.number);
			irp->old = p;
			p1=irp->new = insert(p);
			p1->op = p->op;
			for (l = 0; l <= 3; l++) {
				if (l == i) { 
					p1->ops[l]=getspace(NEWSIZE);
					strcpy(p1->ops[l], iops_tbl[j].regal);  
				}else 
					p1->ops[l] = p->ops[l];
			}			
            p1->uses = p->uses & (~iops_tbl[j].ib_regs) | frame_reg;
            p1->sets = p->sets;
		}	    
		
	}
	return true;
}

void 
regals_2_index_ops()
{
NODE *p,*np;
ir* irp;
char *ch;
int i,j;	
char reg[5], freg[5];
unsigned bsreg;
extern char *itoreg();

#ifdef FLIRT
static int numexec=0;
	++numexec;
#endif


	COND_SKIP(return,"regals_2_index_ops %d",second_idx,0,0); 

	for (i = 0; i < icnt; i++) {
		p=first->back;
		/* Did loop_regal process  this pseudo regal ? */
		while ( !is_any_label(p)) {
			if( p->op1 && !strcmp(iops_tbl[i].regal,p->op1) 
			&& (p->op == MOVL || p->op == MOVW || p->op == MOVB ))
				 break;
			p=p->back;
		}
		if (is_any_label(p) ) {
		/* No, it didn't . Delete pseudo_regal  */
			for ( irp = iops_tbl[i].first; irp!=NULL ; irp=irp->next ){
				DELNODE(irp->new);
			}		
		} else {
			/* change  movl pseudo_regal,%ecx  =>  leal (ind_oper),%ecx  */		
			chgop(p,LEAL,"leal");
			FLiRT(O_REGALS_2_INDEX_OPS,numexec);
			p->op1=getspace(NEWSIZE);
			sprintf(p->op1,"%s",iops_tbl[i].tbl_entry);
			p->uses = iops_tbl[i].ib_regs;
			p->sets = bsreg = full_reg(setreg(p->op2));
			strcpy(reg,p->op2); 
			p->op2=getspace(5);
			strcpy(p->op2,itoreg(bsreg));
			strcpy(freg,p->op2);
	

 			if ( iops_tbl[i].flag ) move_node(p,first);
			/* change operands */
			for ( irp = iops_tbl[i].first; irp!=NULL ; irp=irp->next ){
				for(j=1;j<=2;j++){
					if (samereg(reg,irp->new->ops[j])) {
						ch = strchr(irp->old->ops[j],'(');
						sprintf(ch,"(%s)",freg);
						irp->old->uses &= ~iops_tbl[i].ib_regs;  
						irp->old->uses |= bsreg;
						irp->old->sets = MEM;
						DELNODE(irp->new);
						FLiRT(O_REGALS_2_INDEX_OPS,numexec);
						break;
					}
				}
			}		
			/* delete restore pseudo_regals */ 			
			for ( p=last; p != &ntail ; ) {
				if (( p->op == MOVL || p->op == MOVW || p->op == MOVB ) 
				&& !strcmp(p->op1,reg) && !strcmp(p->op2,iops_tbl[i].regal)) {
					np = p->forw ;	
					FLiRT(O_REGALS_2_INDEX_OPS,numexec);
					DELNODE(p);
					p=np;
				} else p=p->forw;
			}	
		}			
	}	
	return ;
}				
#undef NCAND

void
init_loops()
{

	all_regs = fixed_stack ? fix_all_regs : reg_all_regs ;
}
