#ident	"@(#)optim:i386/prolog.c	1.29"

#include "optim.h"
#include "optutil.h"
#include "regalutil.h"
#include "database.h"

#define CREGS (REGS & ~ CONCODES_AND_CARRY)
#define USERREGS (ESI|EDI|EBX|EBP) /*SAVEDREGS also include EBI*/

#ifdef DEBUG
extern int bflag;
#endif
extern NODE *copy_p_after_q();

static BLOCK *prolog, *epilog, *body;
static boolean save_is_push;
static unsigned int read_regs, defined_regs;

static boolean
is_lazy_func()
{
	prolog = b0.next;
	/* Identify the structure of a prolog that ends with a conditional jump,
	** and one of the targets ends with a return
	*/
	if (! iscbr(prolog->lastn)) {
#ifdef DEBUG
		if (bflag > 1) {
			fprintf(stderr,"first block doesn't conditional jump: unlazy\n");
		}
#endif
		return false;
	}
	if (prolog->lastn->nlive & CONCODES_AND_CARRY) {
#ifdef DEBUG
		if (bflag > 1) {
			fprintf(stderr,"a next jump after the prolog jump: give_up\n");
		}
#endif
		return false;
	}
	if (prolog->nextr->lastn->op == RET) {
		if (prolog->nextl->lastn->op == RET) {
#ifdef DEBUG
			if (bflag > 1) {
				fprintf(stderr,"both blocks return: give_up\n");
			}
#endif
			return false;
		}
		body = prolog->nextl;
		epilog = prolog->nextr;
	} else if (prolog->nextl->lastn->op == RET) {
		body = prolog->nextr;
		epilog = prolog->nextl;
		if (islabel(epilog->firstn)) {
#ifdef DEBUG
			if (bflag > 1) {
				fprintf(stderr,"epilog is fallthru and it has a label: give_up\n");
			}
#endif
			return false;
		}
	} else {
#ifdef DEBUG
		if (bflag > 1) {
			fprintf(stderr,"no subsequent basic block returns: unlazy\n");
		}
#endif
		return false;
	}
	return true;
}

static void
mark_dependent_instructions(NODE *from, NODE *to)
{
  NODE *p;
  tdependent set_memory = { NULL , NULL};
  tdependent use_memory = { NULL , NULL};
  tdependent *last_mem_set = &set_memory;
  tdependent *last_mem_use = &use_memory;
  extern unsigned int muses(), msets();

	/* Add memory use info and init usage. Also, do not count esp as a
	** resourse for the save - restore instructions.
	*/
	if (save_is_push) {
		read_regs &= ~ESP;
		defined_regs &= ~ESP;
	}
	for (p = from; p != to->back; p = p->back) {
		p->uses |= muses(p);	/* muses doesn't include indexing! */
		p->sets = msets(p);
		p->usage = 0;
		if (save_is_push) {
			p->sets &= ~ESP;
			p->uses &= ~ESP;
		}
	}
	for (p = from; p != to->back; p = p->back) {
		if (   ((p->uses | p->sets) & defined_regs)
		    || (p->sets & read_regs)) {
			read_regs |= p->uses & CREGS;
			defined_regs |= p->sets & CREGS;
			p->usage = 1;
		}
		if (IS_FIX_PUSH(p)) {
			/* only reg analysis for the saves */
			continue;
		}
		if (p->sets & MEM) {
			tdependent *t;
			if (   (   p->op1
			        && ismem(p->op1)
			        && (   !(scanreg(p->op1,1) & ESP)
			            || isvolatile(p,1)))
			    || (   p->op2
			        && ismem(p->op2)
			        && (  !(scanreg(p->op2,1) & ESP)
			            || isvolatile(p,2)))
			    || (   p->op2
			        && ismem(p->op2)
			        && (  !(scanreg(p->op2,1) & ESP)
			            || isvolatile(p,2)))) {
#ifdef DEBUG
				if (bflag > 2) {
					fprintf(stderr,"seting a global or volatile mem, stay ");
					fprinst(p);
				}
#endif
				p->usage = 1;
				read_regs |= p->uses & CREGS;
				defined_regs |= p->sets & CREGS;
			} else {
				/* disambig from all insts at the list, then add to the list */
				for (t = set_memory.next ; t; t = t->next) {
					if (seems_same(p,t->depends,0)) {
#ifdef DEBUG
						if (bflag > 2) {
							fprintf(stderr,"setting a set mem, stay ");
							fprinst(p);
						}
#endif
						p->usage = 1;
						read_regs |= p->uses & CREGS;
						defined_regs |= p->sets & CREGS;
						break;
					}
				}
				if (p->usage == 0) {
					for (t = use_memory.next ; t; t = t->next) {
						if (seems_same(p,t->depends,0)) {
#ifdef DEBUG
							if (bflag > 2) {
								fprintf(stderr,"seting a used mem, stay ");
								fprinst(p);
							}
#endif
							p->usage = 1;
							read_regs |= p->uses & CREGS;
							defined_regs |= p->sets & CREGS;
							break;
						}
					}
				}
			}
			last_mem_set->next = (tdependent *) getspace(sizeof(tdependent));
			last_mem_set = last_mem_set->next;
			last_mem_set->depends = p;
			last_mem_set->next = NULL;
		} /* endif p->sets MEM */
		if (p->uses & MEM) {
			tdependent *t;
			/* disambig from all insts at the list, then add to the list */
			if (   (   p->op1
			        && ismem(p->op1)
			        && (   !(scanreg(p->op1,1) & ESP)
			            || isvolatile(p,1)))
			    || (   p->op2
			        && ismem(p->op2)
			        && (   !(scanreg(p->op2,1) & ESP)
			            || isvolatile(p,2)))
			    || (   p->op3
			        && ismem(p->op3)
			        && (   !(scanreg(p->op3,1) & ESP)
			            || isvolatile(p,3)))) {
#ifdef DEBUG
				if (bflag > 2) {
					fprintf(stderr,"using a global or volatile, so what? ");
					fprinst(p);
				}
#endif
				p->usage = 1;
				read_regs |= p->uses & CREGS;
				defined_regs |= p->sets & CREGS;
			} else {
				for (t = set_memory.next ; t; t = t->next) {
					if (seems_same(p,t->depends,0)) {
#ifdef DEBUG
						if (bflag > 2) {
							fprintf(stderr,"using a set mem, stay ");
							fprinst(t->depends);
							fprinst(p);
						}
#endif
						p->usage = 1;
						read_regs |= p->uses & CREGS;
						defined_regs |= p->sets & CREGS;
						break;
					}
#ifdef DEBUG
					  else {
						if (bflag > 2) {
							fprintf(stderr,"memory different: ");
							fprinst(t->depends);
							fprinst(p);
						}
					}
#endif
				}
			}
			/* add to the list */
			last_mem_use->next = (tdependent *) getspace(sizeof(tdependent));
			last_mem_use = last_mem_use->next;
			last_mem_use->depends = p;
			last_mem_use->next = NULL;
		} /* endif p->uses MEM */
	} /* for all p */
	/* restore correct usage info */
	for (p = from; p != to->back; p = p->back) {
		new_sets_uses(p);
	}
} /* end mark_dependent_instructions */

static boolean
process_prolog(unsigned int *lazy_regs)
{
  NODE *p;
  NODE *compare_inst = NULL;
  unsigned int saved_regs = 0;

	for (p = prolog->firstn; p != prolog->lastn->forw; p = p->forw) {
		if (IS_FIX_PUSH(p)) {
			saved_regs |= setreg(p->op1);
		}
	}
	if (saved_regs == 0) {
#ifdef DEBUG
		if (bflag > 1) {
			fprintf(stderr,"this function does not save regs at all: unlazy\n");
		}
#endif
		return false;
	}
	if ((prolog->lastn->nlive & USERREGS) == (saved_regs & USERREGS)) {
#ifdef DEBUG
		if (bflag > 1) {
			fprintf(stderr,"using all saved regs: unlazy\n");
		}
#endif
		return false;
	}
	for (p = prolog->lastn; p != &n0; p = p->back) {
		if (p->sets & CONCODES_AND_CARRY) {
			compare_inst = p;
			break;
		}
	}
	if (compare_inst == NULL) {
#ifdef DEBUG
		if (bflag > 1) {
			fprintf(stderr,"funny function, doesn't set cc for jcc: give_up\n");
		}
#endif
		return false;
	}
	/* initialize resource- usage data with whatever is used by the cmp inst. */
	read_regs |= compare_inst->uses & CREGS;
	mark_dependent_instructions(prolog->lastn,prolog->firstn);
	/* make sure the entry label, cmp and jcc don't move */
	n0.forw->usage = 1;
	compare_inst->usage = 1;
	prolog->lastn->usage = 1;

	/* Make the SUB instruction movable. If there is access to params, rewrite
	** the operand to compensate for the move of the SUB.
	** also count the moving saves, to know how to compensate for the params.
	** All of these only if the save were converted from MOVs to PUSHs.
	*/
	for (p = n0.forw; p != compare_inst; p = p->forw) {
		/* Don't need to force the value, it is supposed to get the right
		** value by the dependency analysis.
		     if (p->op == SUBL && p->sets & ESP) p->usage = !save_is_push;
		*/
		if (IS_FIX_PUSH(p) && p->usage == 0) {
			*lazy_regs |= setreg(p->op1);
		}
	}
	if (*lazy_regs == 0) {
#ifdef DEBUG
		if (bflag > 1) {
			fprintf(stderr,"no moving saves: unlazy\n");
		}
#endif
		return false;
	}

	/* A problem only if the saves are pushs:
	** if there is a reference to an auto/temp, i.e. negative off of
	** esp, and that reference stays in the prolog, then disallow the
	** optimization. This is because in that case the SUB instruction
	** can not move to the body, on the other hand it must move.
	** The only solution is to split it into two SUBs, and that may lose
	** if the function is doing it's long path most of the time.
	*/
	if (save_is_push) {
		for (p = n0.forw; p != compare_inst; p = p->forw) {
			char *s;
			if (IS_FIX_PUSH(p)) {
				continue;
			}
			if (   (p->usage == 1)
			    && (   (((s = p->op1) != NULL) && (scanreg(p->op1,true) & ESP))
			        || (((s = p->op2) != NULL) && (scanreg(p->op2,true) & ESP)))) {

				if (*s == '-' || !strstr(s,".FSZ")) {
#ifdef DEBUG
					if (bflag > 1) {
						fprintf(stderr,"temps in the prolog: give_up\n");
					}
#endif
					return false;
				}
			}
		}
	}
	return true;
} /* end process_prolog */

static boolean
process_epilog()
{
  NODE *p;
  unsigned int reg;
  NODE *first_inst = first_non_label(epilog);

	for (p = epilog->lastn; p != epilog->firstn->back; p = p->back) {
		if (p->sets & EAX) {
			read_regs |= EAX | p->uses;
			break;
		}
	}
	mark_dependent_instructions(epilog->lastn,epilog->firstn);

	/* If the save are done with pushs, then the epilog must not reference
	** the local frame, because it's not going to have one.
	** Only test the instructions that need to stay.
	*/
	for (p = epilog->firstn; p != epilog->lastn; p = p->forw) {
		char *s;
		if (   p->usage == 0
		    || (IS_FIX_POP(p))) {
			continue;
		}
		if (save_is_push) {
			if (   (   ((s = p->op1) != NULL)
			        && (scanreg(p->op1,1) & ESP))
			    || (   ((s = p->op2) != NULL)
			        && (scanreg(p->op2,1) & ESP))) {

				if (   *s == '-'
				    || !strstr(s,".FSZ")) {
#ifdef DEBUG
					if (bflag > 1) {
						fprintf(stderr,"function needs frame at epilog: give_up\n");
					}
#endif
					return false;
				}
			}
		}
		read_regs |= USERREGS & p->sets;
		if (   ((reg = ((p->uses | p->sets) & (EAX|EDX|ECX))) != 0)
		    && (first_inst->nlive & reg)) {
			read_regs |= reg;
		}
	}
	/* If all registers need to be saved in the prolog, no gain. */
	if ((read_regs & USERREGS) == USERREGS) {
#ifdef DEBUG
		if (bflag > 1) {
			fprintf(stderr,"epilog forces no gain, give_up\n");
		}
#endif
		return false;
	}
	return true;
}

static void
match_saves_and_restores()
{
  unsigned int save_order[4];
  int m,total_saves;
  BLOCK *b;
  NODE *p;
	/* Record the order between the saves, need to reorder the restores.
	** At this point, look for saves in the prolog and in the first basic block
	** of the function. "body" points there.
	** Then, find epilog and reorder the restores to be in the same order.
	** Do this epilog treatment before be make the new, lazy epilog which
	** has fewer restores and might need a special case.
	** All of this is necessary only if the saves and resotres are done with
	** push / pop instructions. This is the case if fix_frame is suppressed
	** or if we target for P5, which prefers those instructions.
	*/
	m = 0;
	for (p = n0.forw; p != body->lastn->forw; p = p->forw) {
		if (IS_FIX_PUSH(p)) {
			save_order[m++] = setreg(p->op1);
		}
	}
	total_saves = m-1;
	for (b = b0.next; b; b = b->next) {
		if (b->lastn->op == RET) { /* found an epilog */
			m = total_saves;
			for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
				if (IS_FIX_POP(p)) {
					if ((p->sets & USERREGS) != save_order[m]) {
						NODE *q;
						for (q = p; q->op != RET; q = q->forw) {
							if (   IS_FIX_POP(q)
							    && ((q->sets & USERREGS) == save_order[m])) {
								move_node(q,p->back);
								/* need to redo this pop */
								p = p->back;
								break;
							}
						} /* for all q */
					} /* endif wrong order */

					m--;

				} /* endif is_fix_pop */
			} /* for all p */
		} /* endif RET */
	} /* for all b */
} /* match_saves_and_restores */

static void
compensate_params(NODE *from, NODE *to, int shift)
{
  NODE *p;

	/* Find access to params that were left in the prolog, rewrite the operand
	** to conpensate for the move of the SUB instruction forward.
	*/
	for (p = from; p != to->forw; p = p->forw) {
		int m;
		if (   (   (m=1, p->op1)
		        && scanreg(p->op1,1) & ESP
		        && *p->op1 != '-')
		    || (   (m=2, p->op2)
		        && scanreg(p->op2,1) & ESP
		        && *p->op2 != '-')
		    || (   (m=3, p->op3)
		        && scanreg(p->op3,1) & ESP
		        && *p->op3 != '-')) {
			/* rewrite p->ops[m] */
			char *s = getspace(LEALSIZE);
			int offset = (int)strtoul(p->ops[m],(char **)NULL,0);
			sprintf(s,"%d(%%esp)",offset-shift);
			p->ops[m] = s;
		}
	}
} /* compensate_params */

static void
mark_restores(needed_regs) unsigned int needed_regs;
{
  NODE *p;
  boolean sub_usage;

	epilog->lastn->usage = 1;
	for (p = epilog->firstn; p != epilog->lastn->forw; p = p->forw) {
		if (   IS_FIX_POP(p)
		    && (p->sets & needed_regs)) {
			p->usage = 1;
		}
	}
	for (p = prolog->firstn; p != prolog->lastn->forw; p = p->forw) {
		if (   p->op == SUBL
		    && !strcmp(p->op2,"%esp")) {
			sub_usage = p->usage;
			break;
		}
	}
	for (p = epilog->firstn; p != epilog->lastn->forw; p = p->forw) {
		if (  p->op == ADDL
		    && !strcmp(p->op2,"%esp")) {
			p->usage = sub_usage;
			break;
		}
	}
} /* mark_restores */

/* This works only for fixed stack code. It assumes that the registers save
** instructions are moves, rather than push. This is also the reason why this
** transformation has to be before convert_mov_to_push, which may change the
** moves to pushes, according to the target cpu.
** This will improve more than it does now if short prologs will make more use
** of scratch registers, rather than user regs.
*/

void
lazy_register_save()
{
  NODE *p, *p_forw;
  NODE *srcn, *dstn;
  boolean dup_epilog;
  unsigned int lazy_regs = 0;
  int moving_saves = 0;

	COND_RETURN("lazy_register_save");
	if (! is_lazy_func()) {
		return;
	}
	/* Conditions are different if the save is done with PUSH or MOVL */
	for (ALLN(p)) {
		if (IS_FIX_PUSH(p)) {
			if (p->op == PUSHL) {
				save_is_push = true;
			} else {
				save_is_push = false;
			}
			break;
		}
	}
	if (! process_epilog()) {
		return;
	}
	if (! process_prolog(&lazy_regs)) {
		return;
	}

#ifdef DEBUG
	if (bflag > 1) {
		fprintf(stderr,"change the code\n");
	}
#endif

	mark_restores(USERREGS & ~lazy_regs);

	/* Place the moving instructions in the function body. If the body starts
	** with a label then the fallthru is different from the taken: if it is a
	** fallthru, must place the moving instructions before the label. If it is
	** the taken, must be placed after the label.
	** currently, dup_epilog is true iff the body is the fallthru, so test that.
	*/
	dup_epilog = body == prolog->nextl;
	dstn = body->firstn;
	if (!dup_epilog) {
		while (islabel(dstn)) {
			dstn = dstn->forw;
		}
	}
	dstn = dstn->back;
	for (p = prolog->firstn; p != prolog->lastn; /* EMPTY */ ) {
		p_forw = p->forw;
		if (p->usage == 0) {
			if (IS_FIX_PUSH(p)) {
				moving_saves+=4;
			}
			move_node(p,dstn);
			dstn = p;
		}
		p = p_forw;
	}
	if (save_is_push) {
		match_saves_and_restores();
		compensate_params(prolog->firstn,prolog->lastn,moving_saves);
	}
	if (dup_epilog) {
		NODE *new_first;
		/* Make a new, short epilog and point to it. */
		/* If there are references to parameters in the epilog, rewrite them
		** not to assume FSZ was SUB-ed from esp.
		*/
		dstn = ntail.back;
		dstn = insert(dstn);
		dstn->op = LABEL;
		dstn->opcode = newlab();
		prolog->lastn->op1 = dstn->opcode;
		new_first = dstn;
		for (srcn=epilog->firstn;srcn!=epilog->lastn->forw;srcn=srcn->forw) {
			if (islabel(srcn)) {
				continue;
			}
			if (   IS_FIX_POP(srcn)
			    && (srcn->sets & lazy_regs)) {
				continue;
			}
			if (srcn->usage == 0) {
				continue;
			}
			dstn = copy_p_after_q(srcn,dstn);
		}
		if (save_is_push) {
			compensate_params(new_first,dstn,moving_saves);
		}
	} else {
		for (srcn=epilog->firstn;srcn!=epilog->lastn->forw;srcn=srcn->forw) {
			if (   IS_FIX_POP(srcn)
			    && (srcn->sets & lazy_regs)) {
				DELNODE(srcn);
			}
			if (srcn->usage == 0) {
				DELNODE(srcn);
			}
		}
		if (save_is_push) {
			compensate_params(epilog->firstn,epilog->lastn,moving_saves);
		}
	}
} /* lazy_register_save */


/* Remove redundant macros: if there is no need to use the FSZn or RSZn
** macros, because their value became zero due to optimizations, then
** rewrite the operands not to use them, and don't define the macros.
** This does not make the generated code run any faster. However:
**    1. it is already here
**    2. more important, several bugs in other optimizations were found
**       by inconsistencies discovered here
** Therefore, it's a good sanity check.
*/
void
rmrdmac()
{
  NODE *p,*sub_node = NULL;
  boolean are_autos = false;
  int offset;
  char *s,*first_par;
  char *tmp;
	
	COND_RETURN("rmrdmac");

	set_FSZ = true;
	for (ALLN(p)) {
		if (   (p->op == SUBL)
		    && samereg(p->op2,"%esp")
		    && strstr(p->op1,".FSZ")) {
			sub_node = p;
			continue;
		}
		if (   (p->op == ADDL)
		    && samereg(p->op2,"%esp") 
		    && strstr(p->op1,".FSZ")) {
			continue;
		}

		if (   (   p->op1
		        && usesreg(p->op1,"%esp")
			&& (   *p->op1 == '-'
		            || !strstr(p->op1,".FSZ")))
		    || (   p->op2
		        && usesreg(p->op2,"%esp")
			&& (   *p->op2 == '-'
		            || !strstr(p->op2,".FSZ")))) {

			are_autos = true;

			/* In the old days, the SUB instruction had to be sequentially
			** before any reference to esp, because it was always in the
			** prolog. Since we have lazy function optimization, it may
			** be in a basic block which may be placed any where. It still
			** has to execute before any reference to esp, but recognising
			** that may now be harder, in those cases. So there is no
			** enforcement here.
			** The break stmt below used to be unconditional, that's all.
			*/
			if (sub_node) {
				break;
			}
		}
	}
	/* sub was removed  earlier */
	if (!sub_node) {
		return;
	}

	if (!are_autos) {
		set_FSZ = false;
		DELNODE(sub_node);
		for (ALLN(p)) {
			if (   (p->op == ADDL)
			    && samereg(p->op2,"%esp")
			    && strstr(p->op1,".FSZ")) {
				DELNODE(p);
			}
		}
		for (ALLN(p)) {
			if (   p->op1
			    && strstr(p->op1,".FSZ")) {
				s = (*p->op1 == '*' || *p->op1 == '$') ? p->op1 + 1 : p->op1;

			} else if (   p->op2
			           && strstr(p->op2,".FSZ")) {
				s = (*p->op2 == '*' || *p->op2 == '$') ? p->op2 + 1 : p->op2;

			} else {
				s = NULL;
			}

			if (   s
			    && (   (isdigit(*s))
			        || (strstr(s,".FSZ")))) /* in case of .FSZ(%esp) */ {
				first_par = strstr(s,"(");
				offset = (int)strtoul(s,(char **)NULL,0);
				tmp = getspace(FRSIZE);
				if (offset) {
					sprintf(tmp,"%d%s",offset,first_par);
				} else {
					sprintf(tmp,"%s",first_par);  /* creates (%esp) */
				}
				strcpy(s,tmp);
			}
		}
	}
	if (func_data.regs_spc == 0) {
		/* clear all parameters with RSZ */
		for (ALLN(p)) {
			if (ismem(p->op1)) {
				s = p->op1;

			} else if (ismem(p->op2)) {
				s = p->op2;

			} else {
				s = NULL;
			}
			if (s && strstr(s,"RSZ")) {
				/* remove the -.RSZ2 from operand */
				tmp = strchr(s,'+');
				offset = (int)strtoul(((*s == '*') ? &s[1] : s),(char **)NULL,0);
				first_par = getspace(FRSIZE);
				sprintf(first_par,"%d%s",offset,tmp);
				strcpy(s,first_par);
			}

		} /* for */
	} /* if */
} /* rmrdmac */

/* Change the user register saves and restores from moves to push and pops.
** On the fixed stack code style, this is important for the Pentium. Code
** runs faster with push VS move, and this change is important.
** It also involves moving the SUB instruction, that makes space on the
** frame. If we use moves to save, it must be before the moves. With pushes,
** it has to be after them.
*/

void
convert_mov_to_push()
{
  int reg_saved = 0;
  int reg_saved_in_prolog = 0; /*in lazy functions, some are saved after */
  int offset=0;
  char *defered = "";
  NODE *p,*first_pop,*sub_node,*end;
  int m;
  char *s,*tmp;
  unsigned int idx;
  unsigned int save_order[4]; /*the order of the save instructions*/
  int i = 0;
  boolean first_restore = true;
  boolean inside_prolog = true;

	COND_RETURN("convert_mov_to_push");

	/* In this transformation we have to change the way we access parameters
	** since we change the .set of FSZ?? to be smaller by the sizeof the
	** reg save area, we need to add that to the offset of the parameters, but
	** not to the locals.
	**
	** If there are operands who use esp as index and a ** 2'nd reg as index,
	** we do not (generaly) know if that's an auto or a parameter.  Moreover,
	** loop_decmpl may confuse things because with it's adjustments it can
	** assign a negative number to the index reg and change a negative constant
	** offset to be positive, which will confuse this transformation.
	** That is also the original reason that loop_decmpl() must be after
	** remove_enter_leave().  Instead of trying to find out (as in
	** remove_enter_leave()), let's just give up this transformation.
	*/

	for (i = 3; i >= 0; i--) {
		save_order[i] = 0;
	}
	for (ALLN(p)) {
		idx = 0; /* BUG FIX */
		if (p->op1) {
			idx = scanreg(p->op1,false);
		}
		if (   (idx & ESP)
		    && (idx & (EAX|EBX|ECX|EDX|ESI|EDI|EBP))) {
			return;
		}
		if (p->op2) {
			idx = scanreg(p->op2,false);
		}
		if (   (idx & ESP)
		    && (idx & (EAX|EBX|ECX|EDX|ESI|EDI|EBP))) {
			return;
		}
		if (p->op3) {
			idx = scanreg(p->op3,false);
		}
		if (   (idx & ESP)
		    && (idx & (EAX|EBX|ECX|EDX|ESI|EDI|EBP))) {
			return;
		}
	}

	for (p = n0.forw; p != &ntail; /* EMPTY */ ) {
	  NODE *p_forw;
		/*p->forw may change in the loop*/
		p_forw = p->forw;
		if (iscbr(p)) {
			inside_prolog = false;
		}
		if (IS_FIX_PUSH(p)) {
			save_order[reg_saved] = setreg(p->op1);
			i = reg_saved++;
			if (inside_prolog) {
				reg_saved_in_prolog++;
			}
			chgop(p,PUSHL,"pushl");
			func_data.regs_spc -= 4;
			if (func_data.regs_spc < 0 ) {
				 fatal(__FILE__,__LINE__,"removing more regs than exsist\n");
			}
			p->op2 = (char *) NULL;
		}
		if (IS_FIX_POP(p)) {
			if (first_restore) {
				i = reg_saved - 1;
				first_restore = false;
			}
			if (save_order[i] == setreg(p->op2)) {
				/*In FIFO order*/
				i--;
				chgop(p,POPL,"popl");
				p->op1 = p->op2;
				p->op2 = (char *) NULL;
			} else {
				/* Not in correct FIFO order, bring it here. */
				NODE *q;
				boolean found = false;
				for (q = p; q && (q != &ntail) && (q->op != RET); q = q->forw) {
					if (   IS_FIX_POP(q)
					    && (setreg(q->op2) == save_order[i])) {
						move_node(q,p->back);
						found = true;
						i--;
						chgop(q,POPL,"popl");
						q->op1 = q->op2;
						q->op2 = (char *) NULL;
						p_forw = p;
						break;
					}
				}
				if (!found) {
					/*this reg is not restored in this epilog. try to
					**match this restore with the next reg in the save order*/
					i--;
					p_forw = p;
				}
			}
		} else {
			/* Not a restore. */
			/* This assumes all restores in one epilog are consequtive!
			** and also that there is at least one other inst between
			** two different epilogs.*/
			first_restore = true;
		}

		p = p_forw;
	} /* for all nodes */

	if (reg_saved == 0) {
		/* if there are no changes to be done, don't bother */
		return;
	}
	func_data.frame -= reg_saved * 4 ;

	/* If frame size does not divide by eight, this is the time to fix
	   it, before changing the offsets of the parameters */
	if (func_data.frame % 8) {
		func_data.frame += 4;
	}
	
	/* Fixing offsets of positive offset from FSZ parameters */
	inside_prolog = true;
	for (ALLN(p)) {
		if (iscbr(p)) {
			inside_prolog = false;
		}
		if (   (   (m = 2, s=p->op2)
		        && ((*s == '*') ? isdigit(s[1]) : isdigit(*s))
		        && usesreg(s,"%esp")
		        && strstr(s,".FSZ"))

		    || (   (m = 1, s=p->op1)
		        && ((*s == '*') ? isdigit(s[1]) : isdigit(*s))
		        && usesreg(s,"%esp")
		        && strstr(s,".FSZ"))

		    || (   (m = 3, s=p->op3)
		        && ((*s == '*') ? isdigit(s[1]) : isdigit(*s))
		        && usesreg(s,"%esp")
		        && strstr(s,".FSZ"))) {

			tmp = strstr(s,"+");	
			if (*s == '*') {
				defered = "*";
				offset = (int)strtoul(s+1,(char **)NULL,0);
			} else {
				defered = "";
				offset = (int)strtoul(s,(char **)NULL,0);
			}
			/* One more check: If this use of .FSZnn is in a call
			   to a function that returns a struct and the call is
			   an indirect from a stack locations, the offset must
			   be adjusted  for the push of the temp struct return
			   address.  If the pointer to the function is the first
			   thing on the stack (-4+.FSZnn) then that offset now
			   looks like it is a param - not a -value.

			   This would typically be checked with:
			       if (   (p->op == CALL  || p->op == LCALL)
			           && (p->op3)
			           && !(strcmp(p->op3, "/TMPSRET"))
			           && offset > 0 )
					DON'T DO THE FOLLOWING 

			   In reality, offset 0 should normally be the 
			   function return address, and should never have been
			   addressed.
			*/
			if (offset <= 0 ) {
				continue;
			}
			/* what is deducted from FSZ is added to params */
			if (inside_prolog) {
				offset += reg_saved_in_prolog * 4;
			} else {
				offset += reg_saved * 4 ;
			}
			s = getspace(FRSIZE);
			sprintf(s,"%s%d%s",defered,offset,tmp);
			p->ops[m] = s;
		}
	}

	/* Moving the subl instruction after the push instructions.
	   Moving the addl instruction before the pop instructions.
	*/
	for (ALLN(p)) {
		if (   (p->op == SUBL)
		    && strstr(p->op1,"$.FSZ") && samereg(p->op2,"%esp")) {
			sub_node = p;
			p = p->forw;
			while (IS_FIX_PUSH(p)) {
				p = p->forw ;
			}
			/* p points to the first instruction after the push'es */
			DELNODE(sub_node);
			INSNODE(sub_node,p);
		}
		if (IS_FIX_POP(p)) {
			first_pop = p;
			while (   p->op != ADDL
			       || setreg(p->op2) != ESP
			       || !strstr(p->op1,".FSZ")) {
				p = p->forw ;
			}
			/* p points now to add instruction */	
			end = p->forw;	/* to prevent endless loops */
			DELNODE(p);
			INSNODE(p,first_pop);
			p = end ;
		}
	}
	sets_and_uses();
}

/* Data on params that are passed in regs, and are assigned by amigo to a
** user reg.
*/
typedef struct s {
	char *reg_name;		/* the reg used accross the call*/
	int regal_offset;	/* the unused stack loc. */
	unsigned int reg;	/* the user reg assigned to the arg*/
} assign_item;

/* Associate the reg used accross the call with the stack location used in
** the function body.
** The spill is needed because we cant use the param area, because the caller,
** knowing it passed the args in regs, may decide it doesn't need a frame and
** may not provide an area.
*/
typedef struct x {
	char *reg_name;		/* string of reg used accross call */
	char *regal_name;	/* taken from regal comment */
	unsigned int reg;	/* the reg used accross call */
	AUTO_REG *regal;	/* the struct in regal database */
	boolean replaceable;	/* can replace param by reg in func body
				   if they have disjoint lifetimes and reg
				   is dead at all calls*/
	unsigned int assigned_user_reg;
	char *assigned_reg_name;
	char spill[FRSIZE];	/* if cant replace, must spill */
} assoc_item;

assoc_item assoc_table[3];	/* at most the number of scratch regs is needed */
assign_item assign_table[4];	/* number of user registers */

static assoc_item null_item = {NULL , NULL , 0 , NULL , 0 , 0 , NULL , { 0 } };
static assign_item null_assign_item = { NULL, NULL, 0 };
static int last_assigned;

void
init_assigned_params()
{
	last_assigned = 0;
	assign_table[0] = assign_table[1] = assign_table[2] = assign_table[3]
		= null_assign_item;
}

extern AUTO_REG *regals[];	/*in regalutil.c*/
#define INVALID     -1		/* as in regalutil.c*/
extern int hash_regal(char *);
extern char *itoreg();

static unsigned int
get_assigned_regal(char *regal)
{
  int i;
  int x = (int)strtoul(regal,(char **)NULL,0);

	if (!fixed_stack) {
		x += 4;
	}
	for (i = 0; i < last_assigned; i++) {
		if (x == assign_table[i].regal_offset) {
			return assign_table[i].reg;
		}
	}
	return 0;
}

/* called from parse_regal */
void
save_assign(char *param, char *regname)
{
	assign_table[last_assigned].regal_offset = (int)strtoul(param,(char **)NULL,0);
	assign_table[last_assigned].reg_name = strdup(regname);
	assign_table[last_assigned].reg = setreg(regname);
	last_assigned++;
}

/* Should have used something from regalutil but isregal return boolean and
** not AUTO_REG *, and getregal expects NODE * and not char *.
*/
AUTO_REG *
get_orig_regal(char *operand)
{
  int entry;
  AUTO_REG *r;
  int x = (int)strtoul(operand,(char **)NULL,0);
  char tmp_op[LEALSIZE];

	if (!fixed_stack) {
		x += 4;
		sprintf(tmp_op,"%d(%%ebp)",x);
		entry = hash_regal(tmp_op);
		operand = tmp_op;
	} 
	if (x <= 0) {
		return NULL;
	}
	entry = hash_regal(operand);
	for (r = regals[entry]; r != NULL ; r = r->reg_hash_next) {
		if (   r->valid
		    && r->bits != INVALID
		    && (int)strtoul(r->reglname,(char **)NULL,0) == x) {

			return r;
		}
	}
	return NULL;
}

/* Return value indicates the number of arg in regs
** in this function.
*/
static int
associate()
{
  NODE *p;
  int index = 0;

	assoc_table[0] = assoc_table[1] = assoc_table[2] = null_item;
	for (ALLN(p)) {
		if (   islabel(p)
		    && p->opcode[0] == '.') {

			break;	
		}
		if (   p->op == MOVL
		    && (setreg(p->op1) & (EAX|ECX|EDX))
		    && scanreg(p->op2,false) & frame_reg) {

			assoc_table[index].reg_name = p->op1;
			assoc_table[index].regal_name = p->op2;
			assoc_table[index].replaceable = true;
			assoc_table[index].reg = setreg(p->op1);
			assoc_table[index].regal = get_orig_regal(p->op2);
			if (assoc_table[index].regal == NULL) {
				if ((assoc_table[index].assigned_user_reg
					= get_assigned_regal(p->op2)) == 0) {
					fatal(__FILE__,__LINE__,
					      "Arg not in REGAL list %s\n",p->op2);
				}
				assoc_table[index].assigned_reg_name =
					itoreg(assoc_table[index].assigned_user_reg);
			}
			++index;
		}
	}
	return index;
} /* end associate */

#define piclab(P) (islabel(P) && (P)->back->op == CALL && \
	!strcmp((P)->opcode,(P)->back->op1))

void
args_2_regs()
{
  NODE *p,*q;
  int index;
  NODE *body = NULL, *first = NULL, *second = NULL, *jump = NULL , *btar = NULL;
  int nargs;
  int cp;

	COND_RETURN("args_2_regs");

	if ((nargs = associate()) == 0) {
		return;
	}
	ldanal();
	regal_ldanal();
	for (ALLN(p)) {
		if (islabel(p) && p->opcode[0] == '.') {
			body = p;
			break;
		}
	}
	if (fixed_stack) {
		cp = 0;
	} else {
		cp = 4;
	}
	if (!body) {
		fatal(__FILE__,__LINE__,"no function body\n");
	}
	/* The arg can't live at calls and at insts where reg lives
	** Also, if the args is an operand of an fp inst, it is not replaceable
	*/
	for (p = body ; p && p != &ntail; p = p->forw) {
		for (index = 0; index < nargs; index++) {
			if (p->op == RET) {
				/* All regs appear live at ret */
				continue;
			}
			if (assoc_table[index].replaceable) {
				if (   p->op == CALL
				    || p->nlive & assoc_table[index].reg) {

					/* Exactly one of assigned_user_reg and regal is non zero */
					if (   (assoc_table[index].assigned_user_reg & p->nlive)
					    || (   assoc_table[index].regal
					        && p->nrlive & assoc_table[index].regal->bits)) {
						assoc_table[index].replaceable = false;
					}
				}
				if (   !assoc_table[index].assigned_user_reg
				    && isfp(p)
				    && (   (    p->op1
				            && !strcmp(p->op1,assoc_table[index].regal->reglname))
					|| (    p->op2
				            && !strcmp(p->op2,assoc_table[index].regal->reglname)))) {

					assoc_table[index].replaceable = false;
				}
				if (   assoc_table[index].assigned_user_reg
				    && ((hard_uses(p) | hard_sets(p))
				            & assoc_table[index].assigned_user_reg)) {

					assoc_table[index].replaceable = false;
				}
			}
		}
	}
	/* For the args that cant be register allocated, assign spill locations */
	for (index = 0; index < nargs; index++) {
		if (   !assoc_table[index].replaceable
		    && !assoc_table[index].assigned_user_reg) {

			int spill = -inc_numauto(4);
			if (fixed_stack) {
				sprintf(assoc_table[index].spill,"%d-%s%d+%s%d(%%esp)"
				,spill,regs_string,func_data.number,frame_string
				,func_data.number);
			} else {
				sprintf(assoc_table[index].spill,"%d(%%ebp)",spill);
			}
			save_regal(assoc_table[index].spill,LoNG,true,false);
		}
	}
	/* For amigo assigned args, if they are replaceable then remove
	** the amigo assignment. If they are not replaceable, change the
	** source of the amigo assignment from stack operand to scratch reg
	*/
	for (index = 0; index < nargs; index++) {
		if (assoc_table[index].assigned_user_reg) {
			for (q = body->forw;
			     q && (!islabel(q)||piclab(q));
			     q = q->forw) {

				if (   is_move(q)
				    && samereg(q->op2,assoc_table[index].assigned_reg_name)
				    && (scanreg(q->op1,0) & frame_reg)
				    && ((int)strtoul(q->op1,(char **)NULL,0)
				          == cp + (int)strtoul(assoc_table[index].regal_name,(char **)NULL,0))) {

					if (assoc_table[index].replaceable) {
						DELNODE(q);
					} else {
						switch (OpLength(q)) {
						case LoNG:
							q->op1 = assoc_table[index].reg_name;
							q->uses = assoc_table[index].reg;
							break;

						case WoRD:
							q->op1 = itoreg(L2W(assoc_table[index].reg));
							q->uses = L2W(assoc_table[index].reg);
							break;

						case ByTE:
							q->op1 = itoreg(L2B(assoc_table[index].reg));
							q->uses = L2B(assoc_table[index].reg);
							break;

						}
					}
					break;
				}
			}
		}
	}

	/* Change the function body to use the regs instead of the stack */
	for (p = body->forw ; p && p != &ntail; p = p->forw) {
		if (p->save_restore != 0) {
			continue;
		}
		for (index = 0; index < nargs; index ++) {
			char *org_opnd;
			unsigned int as_reg;

			if (assoc_table[index].assigned_user_reg) {
				if (assoc_table[index].replaceable) {
					as_reg = assoc_table[index].assigned_user_reg;
					if (   p->op1
					    && scanreg(p->op1,0) & as_reg) {

						replace_registers(p,as_reg,assoc_table[index].reg,1);
						if (p->uses & as_reg) {
							if (p->uses & R16MSB) {
								p->uses |= assoc_table[index].reg;
							} else if (p->uses & R24MSB) {
								p->uses |= L2W(assoc_table[index].reg);
							} else {
								p->uses |= L2B(assoc_table[index].reg);
							}
							p->uses &= ~as_reg;
						}
						if (p->sets & as_reg) {
							if (p->sets & R16MSB) {
								p->sets |= assoc_table[index].reg;
							} else if (p->sets & R24MSB) {
								p->sets |= L2W(assoc_table[index].reg);
							} else { 
								p->sets |= L2B(assoc_table[index].reg);
							}
							p->sets &= ~as_reg;
						}
					}
					if (   p->op2
					    && scanreg(p->op2,0) & as_reg) {

						replace_registers(p,as_reg,assoc_table[index].reg,2);
						if (p->uses & as_reg) {
							if (p->uses & R16MSB) {
								p->uses |= assoc_table[index].reg;
							} else if (p->uses & R24MSB) {
								p->uses |= L2W(assoc_table[index].reg);
							} else {
								p->uses |= L2B(assoc_table[index].reg);
							}
							p->uses &= ~as_reg;
						}
						if (p->sets & as_reg) {
							if (p->sets & R16MSB) {
								p->sets |= assoc_table[index].reg;
							} else if (p->sets & R24MSB) {
								p->sets |= L2W(assoc_table[index].reg);
							} else { 
								p->sets |= L2B(assoc_table[index].reg);
							}
							p->sets &= ~as_reg;
						}
					}
					if (   p->op3
					    && scanreg(p->op3,0) & as_reg) {

						replace_registers(p,as_reg,assoc_table[index].reg,3);
						if (p->uses & as_reg) {
							if (p->uses & R16MSB) {
								p->uses |= assoc_table[index].reg;
							} else if (p->uses & R24MSB) {
								p->uses |= L2W(assoc_table[index].reg);
							} else {
								p->uses |= L2B(assoc_table[index].reg);
							}
							p->uses &= ~as_reg;
						}
						if (p->sets & as_reg) {
							if (p->sets & R16MSB) {
								p->sets |= assoc_table[index].reg;
							} else if (p->sets & R24MSB) {
								p->sets |= L2W(assoc_table[index].reg);
							} else { 
								p->sets |= L2B(assoc_table[index].reg);
							}
							p->sets &= ~as_reg;
						}
					}
				} /* endif replaceable */
			} else {
				/* not an amigo assigned reg */
				org_opnd = assoc_table[index].regal->reglname;
				if (assoc_table[index].replaceable) {
					if (   p->op1
					    && !strcmp(p->op1,org_opnd)) {

						switch (OpLength(p)) {
						case LoNG:
							p->op1 = assoc_table[index].reg_name;
							break;
						case WoRD:
							p->op1 = itoreg(L2W(assoc_table[index].reg));
							break;
						case ByTE:
							p->op1 = itoreg(L2B(assoc_table[index].reg));
							break;
						}
						new_sets_uses(p);

					} else if (   p->op1
					           && *(p->op1) == '*' 
					           && !strcmp(p->op1+1,org_opnd)) {

						p->op1 = getspace(6);
						p->op1[0] = '*';
						strcpy(&(p->op1[1]),assoc_table[index].reg_name);
						new_sets_uses(p);
					}
					if (   p->op2
					    && !strcmp(p->op2,org_opnd)) {
						switch (OpLength(p)) {

						case LoNG:
							p->op2 = assoc_table[index].reg_name;
							break;
						case WoRD:
							p->op2 = itoreg(L2W(assoc_table[index].reg));
							break;
						case ByTE:
							p->op2 = itoreg(L2B(assoc_table[index].reg));
							break;
						}
						new_sets_uses(p);
					} else if (   p->op2
					           && *(p->op2) == '*'
					           && !strcmp(p->op2+1,org_opnd)) {

						p->op2 = getspace(6);
						p->op2[0] = '*';
						strcpy(&(p->op2[1]),assoc_table[index].reg_name);
						new_sets_uses(p);
					}
					if (   p->op3
					    && !strcmp(p->op3,org_opnd)) {
						switch (OpLength(p)) {

						case LoNG:
							p->op3 = assoc_table[index].reg_name;
							break;
						case WoRD:
							p->op3 = itoreg(L2W(assoc_table[index].reg));
							break;
						case ByTE:
							p->op3 = itoreg(L2B(assoc_table[index].reg));
							break;
						}
						new_sets_uses(p);
					} else if (   p->op3
					           && *(p->op3) == '*'
					           && !strcmp(p->op3+1,org_opnd)) {

						p->op3 = getspace(6);
						p->op3[0] = '*';
						strcpy(&(p->op3[1]),assoc_table[index].reg_name);
						new_sets_uses(p);
					}
				} else {
				/* not replaceable, need to spill */
					if (   p->op1
					    && !strcmp(p->op1,org_opnd)) {

						p->op1 = assoc_table[index].spill;

					} else if (   p->op1
					           && *(p->op1) == '*' 
					           && !strcmp(p->op1+1,org_opnd)) {

						p->op1 = getspace(LEALSIZE);
						p->op1[0] = '*';
						strcpy(&(p->op1[1]),assoc_table[index].spill);
					}
					if (   p->op2
					    && !strcmp(p->op2,org_opnd)) {

						p->op2 = assoc_table[index].spill;

					} else if (   p->op2
					           && *(p->op2) == '*' 
					           && !strcmp(p->op2+1,org_opnd)) {

						p->op2 = getspace(LEALSIZE);
						p->op2[0] = '*';
						strcpy(&(p->op2[1]),assoc_table[index].spill);
					}
					if (   p->op3
					    && !strcmp(p->op3,org_opnd)) {

						p->op2 = assoc_table[index].spill;

					} else if (   p->op3
					           && *(p->op3) == '*' 
					           && !strcmp(p->op3+1,org_opnd)) {

						p->op3 = getspace(LEALSIZE);
						p->op3[0] = '*';
						strcpy(&(p->op2[1]),assoc_table[index].spill);
					}
				}
			}
		}/* for all args */
	} /* end walk the body */

	for (p = n0.forw; p != body; p = p->forw) {
		if (islabel(p)) {
			if (!first) {
				first = p;
			} else if (!second) {
				second = p;
				jump = p->back;
			} else {
				break;
			}
		}
	}
	if (fixed_stack) {
		for (p = body->forw; p && !islabel(p); p = p->forw) {
			if (p->op == SUBL && setreg(p->op2) == ESP) {
				btar = p;
				break;
			}
		}
		if (btar == NULL) {
			fatal(__FILE__,__LINE__,"fixed frame w/o subl\n");
		}
		while (IS_FIX_PUSH(btar->forw)) {
			btar = btar->forw;
		}
	} else {
		for (p = body->forw; p && !islabel(p); p = p->forw) {
			if (   p->op == PUSHL
			    && setreg(p->op1) == EBP) {

				btar = p;
				break;
			}
		}
		p = btar->forw;
		if (   p->op == MOVL
		    && setreg(p->op1) == ESP
		    && setreg(p->op2) == EBP) {

			btar = p;
		}
		if (   btar->forw->op == SUBL
		    && setreg(btar->forw->op2) == ESP) {

			btar = btar->forw;
		}
		while (   (btar->forw->op == PUSHL)
		       && (setreg(btar->forw->op1) & (EDI|ESI|EBX|EBI))) {

			btar = btar->forw;
		}
	}

	/* Change the entries, so that the first one loads stacks to regs and
	** falls thru to the second entry, and the second entry no longer spills
	*/
	for (p = second; p != body; p = p->forw) {
		if (p->op == MOVL) {
			for (index = 0; index < nargs; index++) {
				char *swap;

				if (   strcmp(p->op1,assoc_table[index].reg_name)
				    || strcmp(p->op2,assoc_table[index].regal_name)) {

					continue;
				}
				if (assoc_table[index].replaceable) {
					swap = p->op1;
					p->op1 = p->op2;
					p->op2 = swap;
					new_sets_uses(p);
					move_node(p,first);
					break;

				} else if (assoc_table[index].assigned_user_reg) {
					swap = p->op1;
					p->op1 = p->op2;
					p->op2 = swap;
					new_sets_uses(p);
					move_node(p,first);
      					for (q = body->forw; q && !islabel(q) ; q = q->forw) {
						if (   is_move(q)
						    && samereg(q->op2,assoc_table[index].assigned_reg_name)
						    && (scanreg(q->op1,0) & frame_reg)
						    && strstr(q->op1,".FSZ")
						    && (strtoul(q->op1,(char **)NULL,0) == strtoul(assoc_table[index].regal_name,(char **)NULL,0))) {

							switch (OpLength(q)) {
							case LoNG:
								q->op1 = assoc_table[index].reg_name;
								break;
							case WoRD:
								q->op1 = itoreg(L2W(assoc_table[index].reg));
								break;
							case ByTE:
								q->op1 = itoreg(L2B(assoc_table[index].reg));
								break;
							}
							q->uses = uses(q);
						}
					} /* for q */
					break;

				} else  {
					/* not replaceable and not allocated, have to spill */
					NODE *pnew;

					swap = p->op1;
					p->op1 = p->op2;
					p->op2 = swap;
					new_sets_uses(p);
					pnew = insert(p);
					chgop(pnew,MOVL,"movl");
					pnew->op1 = p->op2;
					pnew->op2 = assoc_table[index].spill;
					new_sets_uses(pnew);
					move_node(p,first);
					move_node(pnew,btar);
					break;
				}
			} /* for index */
		} /* endif op == MOVL */
	} /* end for p = second */

	DELNODE(jump);
	regs_for_incoming_args = 0;
	for ( index = 0; index < nargs; index++) {
		if (assoc_table[index].replaceable) {
			regs_for_incoming_args |= assoc_table[index].reg;
		}
	}
} /* end args_2_regs */
