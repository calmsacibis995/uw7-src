#ident	"@(#)optim:i386/sched.c	1.63"
#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include "sched.h"      /* includes "optim.h" and "defs" */
#include "optutil.h"
#include "regalutil.h"

typedef NODE * NODEP;
typedef tdependent * tdependentp;
#define BLOCK_SIZE_LIMIT	100
typedef struct  { NODE *setting;
				  boolean isconstant;
				} aset;

static unsigned int regmasx[NREGS] = {EAX,EDX,EBX,ECX,ESI,EDI,EBP,ESP};
static int depths[NREGS]; /*eight registers, without CC */
static int fullregs[] = {Eax,Edx,Ebx,Ecx,Esi,Edi,Ebp};
static int halfregs[] = {AX,DX,BX,CX,SI,DI,BP};
static int justhalfs[] = {Ax,Dx,Bx,Cx};
static int qregs[] = { AL|AH , DL|DH , BL|BH , CL|CH };
static aset   setsR[NREGS];  /*eight registers and CC */
static NODEP  sets_by_var[NREGS];
static NODEP  usesR[NREGS];
static NODEP  setsH[7];      /*for 16 bit registers*/
static NODEP  setsQ[4];      /*for pairs of 8 bit registers*/
static NODEP  last_volatile;
static NODEP  next_of_prefix;
static tdependent  usesMEM, setsMEM;
static int concurrent_value = 0;
#ifdef DEBUG
int vflag = 0;
#endif
static clistmem m0,m1,*fp0;
static NODE *prev;
static NODE * last_is_br;
static BLOCK * b;
static alist a0 = { NULL , NULL };
static alist *acur = &a0;
static int alloc_index=0;
static int float_used = 0;
static tdependent dummy_to_show_agi;
static NODE *last_float = NULL; /* Last FP instruction. Place fxch aftr FP 
                                  or as a filler for AGI and it will be 
                                  paired. (in most cases)                   */

static void convert_block_to_macroop();
extern BLOCK b0;      /* declared in optim.c */
extern NODE  n0;      /* declared in optim.c */
extern NODE *insert();
extern int fp_removed;	/* declared in local.c */
extern int double_aligned;	/* declared in local.c */
extern void set_refs_to_blocks();
extern void trace_body();
extern boolean swflag;

/* functions declared in this module  */
static void  bldag(), prun(), init_block(), update_static_vars(),
			 installCCdep(), install_dependencies(), /*dis2prefix(),*/
			 nail(),  calc_depth(), calc_width(),
			 update_one(), isdependent(),
			 initnode(), relevants(),installM(),
			 make_clist(), update_clist(), dispatch(),
      		 pick1or2(), dispatch1or2(), remove1or2(),
			 check_fp_stack(), check_fp_block(),
			 reorder(), install1(),init_alloc(),
			 count_depths_before_reorder(), fix_offsets(),
			 rebuildag(), reprun(), p6_pick_411(), p6_prescheduler(),
			 add_fp_dependency();

static	int   before(), natural_order(), agi_depends(), depth(),
			  no_gain_in_schedule(),
			  depth_4list();
int		      seems_same(),decompose();
static	NODE  *pick_one();
static BLOCK  *find_big_block_end();
static unsigned int  base();
static void  set_fp_regs(), clean_fp_regs(), reset_fp_reg(),
			 fp_start(), fp_set_resource(), init_fregs(), pair(),
			 freefreg(), fp_ticks();
void	     check_fp();

static void static_analysis(),p6_dispatch(),p6_pick(),
			init_pick(),p6_update_clist();

static unsigned int fmsets(), fmuses();
static NODE *fp(), *EX86(), *islefthy(), *isright(), *Eunsafe_right(),
			*Eanti_dependent(), *Edependent();
static int find_weight(), fp_time(), check_for_fxch();
void clean_function();

extern unsigned msets(), muses(), indexing();
static tdependent * next_dep_str();
int  get_latency(),get_uops();
unsigned get_ports();

#define has_agi_dep(p) ((p)->agi_dep != NULL)
#define makes_new_agi(p) ((p)->usage == AGI)

void
schedule()        /* driver of the scheduler */
{
#ifdef USE_MACRO
	BLOCK *b3=NULL, *b1=NULL;
#endif

	COND_RETURN("schedule");
	init_alloc();
	bldgr(true);
	set_refs_to_blocks();
	trace_body(false);
	if (target_cpu == P5)
		check_fp();

	if (!eiger) convert_block_to_macroop();
	for (b=b0.next; b != NULL ; b=b->next) { /*foreach block*/
		if (   (target_cpu == P4 && no_gain_in_schedule())
		    || b->length <= 1 ) 
		{
			if (   !eiger
			    && (b->ltest || b1 || b3)) goto l_macro;
			continue;
		}
     		init_block(true);
#if we_know_the_branch_is_flacky
		if (target_cpu == P6 )  p6_prescheduler();
#endif
		COND_SKIP(continue,"%d %s ",second_idx,b->firstn->opcode,0);
     		/* special case: all labels at the beginning of the block */
     		/* must remain first. optput them.                        */
     		/* the labels are NOT counted by bldgr() into b->length.  */
     		while (islabel(b->firstn)) {
			initnode(b->firstn);
        		b->firstn = b->firstn->forw; /*advance the block's beginning */ 
     		}/*end while loop*/
		prev = b->firstn->back; /*last node before present block*/

     		/* second special case: if the last instruction is a      */
     		/* branch, then force it to be the last one. dependencies */
     		/* dont take care of this.                                */
		/* Do it also for a call to a .label which appears in Pic */
       		if (   isbr(b->lastn)
		    || is_safe_asm(b->lastn)
		    || b->lastn->op == ASMS
		    || (   b->lastn->op == CALL
			&& b->lastn->op1[0] == '.')) {
       			last_is_br = b->lastn;
       			b->lastn = b->lastn->back;
       			b->length--;
	   		initnode(last_is_br);
	 	} /*endif*/

    		if (target_cpu == P5)
	  		set_fp_regs(b); /* replace float stack with regs */
		else
	  		concurrent_value = 0;

		if (b->length )
    			bldag(); /*construct dependency graph*/
		count_depths_before_reorder(); /* mark values of index regs at nodes */
		prun();  /*reorder the instructions*/
		if (target_cpu == P5)
      			if (float_used)
   	 			clean_fp_regs(b);
		if (target_cpu == P6) {
			rebuildag();
			reprun();
		}
		if (last_is_br) { /*put it back*/
	  		b->lastn = last_is_br;
	  		prev = b->lastn;
	  		if (   (target_cpu == P5)
			    && (   b->lastn->back->exec_info.pairtype == WDA
				|| b->lastn->back->exec_info.pairtype == WD1)
			    && b->lastn->op != ASMS
			    && !is_safe_asm(b->lastn)
	  		    && b->lastn->back->usage == ALONE) {   /*pairable, wasnt*/
				b->lastn->back->usage = ON_X;		   /*paired by sched,*/
		 		b->lastn->usage = ON_J;                /*paired with last*/
#ifdef DEBUG
				if (vflag) b->lastn->back->op4 = "\t\t/leftH";
#endif
	  		} else {
				b->lastn->usage = ALONE;
	  		}

		}

l_macro:
		if (!eiger) {
			if( b1 && b1->ltest ) /* if b1 contains macro make b1 current */
				b=b1;
			if (!b1 && b3)			/* else make first b3 current */
				{ b=b3;b3=NULL; } 

			/* Do internal blocks if exist */
			if ( b->ltest ) 
			{
				save_macro *ptr = (void*)b->ltest;
				NODE *pi;
			
				if ( !b1 )  
				/* first visit to block containing macro .
			   	save first block for case when it contains more then 1 macro 
			   	and b3 to proceed when all macros will be processed 
				*/
				{
					b1 = b;
					b3 = ptr->b3; /* keep last real block */ 
					if( b->lastn != ptr->mo ) 
						b3->lastn = b->lastn;
					else
						b3->lastn = b3->firstn ;
					/* if all instructions of b3 were moved upstairs */
				}	

				/* restore b1,b2,b3  and reduce b to current b1 */
				b->ltest = (void*)ptr->next; 
				b->nextl = ptr->b2;
				b->nextr = ptr->b3;
				b->next  = ptr->b2;
				b->length = ptr->b1_length; 
				/* it's bug because of perculate scheduling but meanwhile ...*/

				b->lastn = ptr->b1_lastn;

				/* expand mo to previous state */

				ptr->mo->back->forw = ptr->b1_lastn;
				ptr->mo->forw->back = ptr->b3_firstn;
				ptr->b3_firstn->forw = ptr->mo->forw;
				ptr->b1_lastn->back = ptr->mo->back;


				pi=(NODE*) ptr->mo->sasm;
				if (pi)
				{
					pi->op1=ptr->mo->op1;
					pi->op2=ptr->mo->op2;
				}	
				/* DELNODE(mo); */
				if ( !b->ltest ) b1=NULL; /* no more macros in the b */
				else b1 = b;
			}
		}  /*endif !eiger*/
 	}/*foreach block*/
}/*end schedule*/

static void
bldag()
/*construct dependency graph for the instructions of a basic block*/
{
NODE * p;
int i;
int dummy;
int found_unexpected = false;

	for (p= b->firstn; p != b->lastn->forw; p = p->forw) {
    		initnode(p);        /* initialize fields in instruction NODEs */

		if (isprefix(p)) {
			p->uses |= muses(p->forw);
			p->sets |= msets(p->forw);
			p->idxs |= (target_cpu & (P5 | blend)) ? indexing(p->forw)
							       : base(p->forw);
		}
		/*special case: some opcodes are really prefixes. next operation */
  		/*must be scheduled after them, and resource dependencies dont   */
  		/*take care of that.                                             */
  		else if (isprefix(p->back)) {
			install1(p->back,p,DEP);
			continue;
		}

  		/*if p is hard to handle, nail it: make it dependent on every*/
  		/*former instruction, and every later instruction on it.     */
		found_unexpected |= Unexpected(p);

  		/* build the DAG.                                            */
  		install_dependencies(p);
  		update_static_vars(p);
	}/*for loop*/

	if (found_unexpected) nail();

	/*Have to add anti dependencies which were not found in the         */
	/*forward loop. Do it in a backwards loop.                          */
	/*These occur when there are several consecutive uses of a register */
	/*and then one set. The forward loop finds only the dependency      */
	/*between the last use and the set, but there has to be dependency  */
	/*between each use and the set.                                     */
	for (i=0; i < NREGS; i++) {
		setsR[i].setting = NULL;
		setsR[i].isconstant = false;
		sets_by_var[i] = NULL;
	}
	for (p = b->lastn; p != b->firstn->back; p = p->back) {
		if (isprefix(p->back))
			continue;
		if (isprefix(p)) {
			p->sets |= p->forw->sets;
			p->uses |= p->forw->uses;
			p->idus |= p->forw->idus;
			p->idxs |= (target_cpu & (P5|blend)) ? indexing(p->forw)
							     : base(p->forw);
		}
		/* If p sets esp and ebp is frame pointer, then instructions that
		** use ebp with negative offset should anti-depend on instructions that
		** set esp, so that there shall not be a reference below esp.
		*/
		if (!fp_removed) {
			if (p->idus & EBP && setsR[7].setting)
	  			install1(p,setsR[7].setting,ANTI);
		}
  		for (i = 0; i < NREGS; i++) {
    			if (p->uses & regmasx[i] && setsR[i].setting)
	  			install1(p,setsR[i].setting,ANTI);
    			if (p->idus & regmasx[i] && setsR[i].setting)
				if (! setsR[i].isconstant )
	  				install1(p,setsR[i].setting,ANTI);
				else if (sets_by_var[i])
					install1(p,sets_by_var[i],ANTI);
			/*mark the setting instructions */
			if (p->sets & regmasx[i]) {
	  			setsR[i].setting = p;
				if (change_by_const(p,regmasx[i],&dummy))
					setsR[i].isconstant = true;
				else {
					setsR[i].isconstant = false;
					sets_by_var[i] = p;
				}
			}
  		} /*for loop*/
	} /*for loop*/
	 
	installCCdep();
	add_fp_dependency(b);

	/* calculate for each node the number of it's dependent sons, */
	/* and the length of the longest chain under it.              */
	calc_width();
	calc_depth(); 
}  /* end bldag*/

static void
init_block(boolean first_time)
{
int i;
	if (first_time) last_is_br = NULL;
	last_volatile = NULL;
	acur = &a0;
	alloc_index = 0;
	for (i=0; i< NREGS; i++) {
		depths[i] = 0;
	}
	for (i=0; i< NREGS; i++) {
		setsR[i].isconstant = false;
  		setsR[i].setting = NULL;
  		usesR[i] = NULL;
		sets_by_var[i] = NULL;
	}
	for (i=0; i<7; i++) {
  		setsH[i] = NULL;
	}
	for (i=0; i<4; i++) {
  		setsQ[i] = NULL;
	}
	usesMEM.next = setsMEM.next =  NULL;
}  /*end init_block*/

static void
initnode(p) NODE *p;
{
	p->dependent = p->may_dep = p->anti_dep = p->agi_dep = NULL;
	p->dependents = p->chain_length = p->nparents = 0;
	p->extra = p->extra2 = 0;
	p->usage = NONE;
	p->uses = muses(p); /*uses directly, not as index registers. */
	p->sets = msets(p);
	/*In  P5, p->idus == p->idxs. In intel486, p->idus is all the indexing
	**but p->idxs is only the base register. A register is considered (here)
	**as base, as opposed to index, if base and index registers can not be
	**swapped.
	*/
	p->idus = indexing(p);
	p->idxs = (target_cpu & (P5 | blend)) ? indexing(p) : base(p);

	/* If and instruction has an immediate operand and no displacement
	** then it will be a passimization to allow reordering it over
	** a change by const - this would add the displacement and make it
	** two cycle on 486 and unpairable on p5. disallow it by the following:
	*/
	if (  p->op1
	    && isconst(p->op1)
	    && p->op2
	    && !hasdisplacement(p->op2))
		p->uses |= p->idus;

        /* Trick: have to add special dependency, use seems_same to do it*/
	if (   p->op == ADDL
	    && p->sets == (ESP | CONCODES_AND_CARRY)
	    && (   p->forw->op == RET
		|| p->forw->forw->op == RET)) {
		p->sets = (unsigned) ESP | MEM | CONCODES_AND_CARRY;
	}
	else if (   p->op == SUBL
		 && p->sets == (ESP | CONCODES_AND_CARRY)
		 && b->firstn == n0.forw->forw) {
		p->sets = (unsigned) ESP | MEM | CONCODES_AND_CARRY;
	} else if (p->op == CALL && p->op3)
		p->sets |= ESP; /* /TMPSRET function it will do one POP */
	if (target_cpu & (P5 | blend))
		p->exec_info.pairtype = pairable(p);
	else if (target_cpu == P6) {
		p->exec_info.rs_info.uops = get_uops(p);
		p->exec_info.rs_info.ports = get_ports(p);
		/*p->exec_info.rs_info.latency = get_latency(p);*/
	}
}  /*end initnode*/

/*mark p as the last instruction to use the registers it uses, set the
**registers it sets, etc.
*/
static void
update_static_vars(p)
NODE *p;
{
int i;
int dummy;

	for (i = 0; i < NREGS; i++) {
  		if (p->sets & regmasx[i]) {
			setsR[i].setting = p;
			if (change_by_const(p,regmasx[i],&dummy)) {
				setsR[i].isconstant = true;
			} else {
				setsR[i].isconstant = false;
				sets_by_var[i] = p;
			}
		}
  		if (p->uses & regmasx[i])
			usesR[i] = p;
	}  /*for loop*/
	if (target_cpu == P4) {
		for (i = 0; i < 7; i++) {
  			if ((p->sets & halfregs[i]) && ! (p->sets & fullregs[i]))
     			setsH[i] = p;
		}  /*for loop*/
		for (i = 0; i < 4; i++) {
  			if ((p->sets & qregs[i]) && ! (p->sets & justhalfs[i]))
	 			setsQ[i] = p;
		}  /*for loop*/
	}
	if (p->sets & MEM)
		update_one(p,&setsMEM);
	if (p->uses & MEM)
		update_one(p,&usesMEM);
	if (   isvolatile(p,1)
	    || isvolatile(p,2)
	    || isvolatile(p,3))
		last_volatile = p;
}/*end update_static_vars*/

/*add an instruction to a list of instructions reading from / writting to
**memory.*/
static void
update_one(p,reg) NODE *p; tdependent * reg;
{
 tdependent *tmp;
	tmp = reg->next;
	reg->next = next_dep_str();
	reg->next->next = tmp;
	reg->next->depends = p;
}/*end update_one*/

/*make p dependent on previous instructions using the same resources*/
static void
install_dependencies(p)
NODE *p;
{
tdependentp sures,possibles;
int i;

	/*Look explicitely for all kinds of dependencies and install them.
	**Do not look for ANTI dependencies because the forward loop is 
	**not suited for that, it is done in a backwards loop in bldag after
	**the return from this function.
	*/

	for (i=0; i < NREGS; i++) {  /* for all registers */
		/*Output dependency: p sets vs the previous setting. */
  		if (p->sets & regmasx[i]) {
			if (setsR[i].setting && !(ispp(setsR[i].setting) && p->uses&ESP))
      			install1(setsR[i].setting,p,DEP);
  		}  /*endif p->sets & regmasx[i]*/
		/*Dependency between different sized registers causes one clock
		**penalty on the intel486, not on the P5. On the P5 we mark it
		**as dependency.
		*/
  		if (i < 7) {  /*seven word addressable registers*/
  			if (p->uses & regmasx[i] && setsH[i])       /* 16 - 32 bit*/
				if (target_cpu == P4)
	  				install1(setsH[i],p,AGI);
				else
	  				install1(setsH[i],p,DEP);
  		}  /*endif i < 7*/
		if (i < 4) { /*four byte addressable registers*/
  			if (p->uses & regmasx[i] && setsQ[i])      /*  8 - 32 bit*/
				if (target_cpu == P4)
	  				install1(setsQ[i],p,AGI);
				else
	  				install1(setsQ[i],p,DEP);
  		}  /*endif i < 4*/
		/*Look for a regular dependence, where the previous instruction
		**set a register and the present instruction uses it.
		**If this register is %esp then make the dependence an ANTI
		**dependence. For the intel486 there is no difference. For the P5
		**it enables the two instruction to be selected together as a pair,
		**which is alright with the processor.
		**Only direct usage of registers is stored in p->uses, not indexing.
		*/
  		if (p->uses & regmasx[i]) 
			if (setsR[i].setting && !(ispp(setsR[i].setting) && p->idus&ESP))
	  			install1(setsR[i].setting,p,DEP);
			else if (setsR[i].setting)
	  			install1(setsR[i].setting,p,ANTI);
		/*p->idus is used to account for dependencies which are not covered by
		**neither p->uses (no indexing there) nor by p->idxs (only base reg.).
		**Look through it only for dependencies, not for AGI dependencies. 
		**that is only through p->idxs.
		**The usage of p->idus is to add dependencies to previous setting
		**instruction which change the registers not by a constant.
		**Through p->uses any previous setting was installed.
		*/
		if ((p->idus & regmasx[i]) && (setsR[i].setting))
			if (!setsR[i].isconstant) {
				/*add dependencies as in p->uses, only if !isconstant*/
				if (!(ispp(setsR[i].setting) && p->idus&ESP))
	  				install1(setsR[i].setting,p,DEP);
				else
	  				install1( setsR[i].setting,p,ANTI);
			}
			/*if the last set was by a constant, there may be stored a previous
			**instruction which sets the register not by constant. A dependency
			**to that instruction has to be added.
			*/
			else if (sets_by_var[i]) 
				install1(sets_by_var[i],p,DEP);
		/*Here look for AGI dependencies. If the present instruction uses
		**a register as an index and a previous one set it, not by constant,
		**then it is an AGI.
		**An exception in the P5: if the present instruction has a prefix,
		**there will not be a penalty.
		*/
  		if (p->idxs & regmasx[i])
			if (   setsR[i].setting
			    && !setsR[i].isconstant
			    && !(ispp(setsR[i].setting)
			    && p->uses&ESP))
				if (target_cpu == P4 || !Hasprefix(p))
	  				install1(setsR[i].setting,p,AGI);

 	}  /* for all registers loop*/
 
	/*Every volatile instruction depends on the previous volatile one*/
	if (   last_volatile
	    && (   isvolatile(p,1)
	        || isvolatile(p,2)
	        || isvolatile(p,3))) {
		install1(last_volatile,p,ANTI);
	}


	/*             Treat memory references seperately.            */
	/* only plain dependency, no anti or agi dependencies.        */

 	if (p->sets & MEM) {
   		if (setsMEM.next) {                    /*output dependency*/
     			relevants(p,setsMEM.next,&sures,&possibles);
     			if (sures)
       				installM(p,sures,1); 
	 		if (possibles)
	   			installM(p,possibles,0);
   		}  /*endif setsMEM.next != NULL*/
   		if (usesMEM.next) {                      /*anti dependency*/
     			relevants(p,usesMEM.next,&sures,&possibles);
     			if (sures)
       				installM(p,sures,1); 
	 		if (possibles)
	   			installM(p,possibles,0);
   		}  /*endif usesMEM.next != NULL*/
 	}  /*endif (p->sets & MEM) */
 	if (p->uses & MEM) {                          /*     dependency*/
   		if (setsMEM.next) {
     			relevants(p,setsMEM.next,&sures,&possibles);
     			if (sures)
       				installM(p,sures,1);
	 		if (possibles)
	   			installM(p,possibles,0);
   		}  /*endif setsMEM.next*/
 	}  /*endif (puses & MEM) */
}  /*end install_dependencies */

/*install one (1) dependency, that setsREG.
**father is the NODE on which to hang.
**depend is a pointer to the dependency pointer in the NODE.
**depp is the dependent NODE to be hanged.
**type is the dependency type, to verify if there allready
**exists a dependency between the two nodes.
**In that case dont add a dependency.
*/
static void
install1(father,son,type) 
NODE *father;  NODE * son; int type;
{
tdependent **header;
tdependent *tmp;
int dtype;
	isdependent(father,son,&header,&dtype);
	if (dtype == NO_DEP) {
		if (   target_cpu != P6
		    && type < AGI && agi_depends(father,son)) {
			type = AGI;
		}
		tmp = father->depend[type];
		father->depend[type] = next_dep_str();
		father->depend[type]->depends = son;
		father->depend[type]->next = tmp;
		son->nparents++;
	}  /*endif dtype == NO_DEP*/
	else if (dtype < type) {
		tmp = father->depend[type];
		father->depend[type] = *header;
		*header = (*header)->next;
		father->depend[type]->next = tmp;
	}  /*end else if*/
}  /*end install1*/


static void
installCCdep()
{
	unsigned CCreg = CARRY_FLAG;
	boolean done = false;

	/* Loop through both CONCODES and CARRY_FLAG to track CC
	   dependencies. */
	do {
		NODE *p,*q,*last_set = NULL,*last_use = NULL;

    	p=b->lastn;
    	for (; p != b->firstn->back; p = p->back) {
			if (p->sets & CCreg) {
				if (last_use != NULL) { /* Use of CC depends on setting */
					install1(p,last_use,ANTI); /* Can be in the same cycle */
					last_use = NULL;
				}  /* if */
				if (last_set == NULL)
					last_set = p; /* last set before use of condition code */
				else  /* Every other sets must come before the last set */
					if (last_set->op != CALL)
						install1(p,last_set,ANTI);
        	}  /* if */
			if (p->uses & CCreg) {
				last_use = p;
				if (last_set != NULL) {
					for (q = p; q != last_set->forw; q = q->forw) {
						if (p != q && q->sets & CCreg)
							/* make last use before all sets  */
							install1(p,q,ANTI);
					} /* for */
					last_set = NULL;
				}  /* if */
			}  /* if */
    	}  /* for */
		if (CCreg == CONCODES)
			done = true;
		else
			CCreg = CONCODES;
	}  while ( ! done);
}  /*end installCCdep*/


/*check whether there is a dependency between father and son, if so,
**of what type is it.
*/
static void
isdependent(father,son,header,type)
NODE *father, *son; tdependent ***header; int *type;
{
tdependent **tp;
int i;
	for (i = ANTI ; i <= AGI ; i++) {
		for (tp = &father->depend[i]; *tp; tp = &(*tp)->next) {
			if ((*tp)->depends == son) {
	  			*type = i;
	  			*header = tp;
	  			return;
			}  /*endif*/
		}  /* for */
	}  /* for */
  	*type = NO_DEP;
  	*header = NULL;
}/*end isdependent*/

/*install dependencies between one son and many parents. Used only
**in the context of dependency via the memory resource.
*/
static void
installM(son,parents,sure) NODE *son; tdependent *parents;int sure;
{
tdependent *tp;
	if (sure)
    		for (tp = parents; tp; tp = tp->next)
	  		install1(tp->depends,son,DEP);
  	else
    		for (tp = parents; tp; tp = tp->next)
	  		install1(tp->depends,son,MAY);
}  /*end installM*/


/*build list of instructions which cant be proven not to depend
**on p, out of the list of all memory refernces.
**If there is a certain equality between two memory locations references
**by two instructions, put them on same_list. If equality is unresolvable,
**put them on unsafe_list.
*/

static void
relevants (p,setsmem,same_list,unsafe_list)
NODE * p; tdependent *setsmem; tdependent **same_list,**unsafe_list;
{
tdependent *tmp; 
tdependent * tp;
boolean use_trace = !(pic_flag && swflag);
*same_list = *unsafe_list = NULL;
	for (tp = setsmem; tp ; tp = tp->next) {
		switch (seems_same(p,tp->depends,use_trace)) {
	  		case different:
			  	break;
	  		case same:
				  tmp = *same_list;
				  *same_list = next_dep_str();
				  (*same_list)->next = tmp;
				  (*same_list)->depends = tp->depends;
				  break;
	  		case unsafe:
				  tmp = *unsafe_list;
				  *unsafe_list = next_dep_str();
				  (*unsafe_list)->next = tmp;
				  (*unsafe_list)->depends = tp->depends;
				  break;
    		}  /*end switch*/
  	}  /*for loop*/
}  /*end relevants*/

/* Function determines base & index of the given operand */
void
get_base_indx(s, pbase, pindx) char* s; unsigned int *pbase, *pindx;
{
char* ss;
	*pbase = *pindx = 0;
	ss=strchr(s,'(');
	if ( !ss )  return;
	if (*++ss == ',') {
		*pindx = setreg(++ss);
		return;
	}
	*pbase = setreg(ss);
	if ( (ss = strchr(ss,',')) != NULL ) *pindx = setreg(ss);
	return;
}

/*try to prove that two memory refernces are not the same return
**value 1 if they seem the same, 0 if they proven different.
**2 if dont know.
**p2 is an earlier instruction in the block then p1.
*/

/* name1 and name2 are common to seems_same and decompose */
static char name1_buf[MAX_LABEL_LENGTH],name2_buf[MAX_LABEL_LENGTH];
int
seems_same(p1,p2,use_trace) NODE *p1, *p2; boolean use_trace;
{
int y1,y2,x1 =0,x2 =0;
unsigned char c;
int n1 = 0, n2 = 0;
int scale1,scale2;
int m1,m2;           /*pointers to the memory referncing operands*/
unsigned pidx1,pidx2;
char *name1,*name2;
unsigned int frame_pointer;
char *regal_op1, *regal_op2;

#define noex -1

#define iEXX 8

#define UKNOWN  0
#define KNOWN   1

#define NOTHING  0
#define REGISTER 1
#define ADDRESS  2
#define MEMORY   3
#define NUMBER   4
	
	extern int i_reg2ind();
	int bb=noex, ss=noex;
	unsigned int p1_base,p1_indx, p2_base,p2_indx;
	int	i_p1_base,i_p1_indx, i_p2_base,i_p2_indx;	 
	
	regal_op1 = regal_op2 = (char *) 0;
	pidx1 = p1->idus;
	pidx2 = p2->idus;
	if (fp_removed)
		if (double_aligned)
			frame_pointer = EBP | ESP;
		else 
			frame_pointer = ESP;
	else 
		frame_pointer = EBP;
	if (fixed_stack)
		frame_pointer = ESP; 
	/* if we optimized ebp elimination for double alignment, and P1 indexes
	** esp and P2 indexes ebp, then the first one is a parameter and the
	** second one is an auto, ergo they are different.
	*/
	if (frame_pointer == (EBP | ESP))
		if (   (pidx1 == EBP && pidx2 == ESP)
		    || (pidx1 == ESP && pidx2 == EBP))
			return different;
 	/* find which are the operands referencing memory in both p1,p2  */
	if (ismem(p1->op1))
		m1 = 1;
	else if (ismem(p1->op2))
		m1 = 2;
	else if (ismem(p1->op3))
		m1 = 3;
	else
		m1 = 0;
	if (ismem(p2->op1))
		m2 = 1;
	else if (ismem(p2->op2))
		m2 = 2;
	else if (ismem(p2->op3))
		m2 = 3;
	else
		m2 = 0;
	/* read only data items are different from any thing else */
	/* currently, nobody calls seems same with two read instructions, the 
	** second test is for completeness.
	*/
	if (   is_read_only(p1->ops[m1])
	    || is_read_only(p2->ops[m2]))
		if (!strcmp(p1->ops[m1],p2->ops[m2])) return same;
		else return different;

	if (   (p1->op == ADDL || p1->op == SUBL)
	    && p1->sets == (ESP | MEM | CONCODES_AND_CARRY)) {
		c = m2 ? *p2->ops[m2] : (char) 0;
		return (isalpha(c) || c == '_' || c == '.') ? different : unsafe;

	} else if (   (p2->op == ADDL || p2->op == SUBL)
		   && p2->sets == (ESP | MEM | CONCODES_AND_CARRY)) {
		c = m1 ? *p1->ops[m1] : (char) 0;
		return (isalpha(c) || c == '_' || c == '.') ? different : unsafe;
	}
	name1 = (strlen(p1->ops[m1]) < ((unsigned ) MAX_LABEL_LENGTH)) ?
					name1_buf : getspace(strlen(p1->ops[m1]));
	name2 = (strlen(p2->ops[m2]) < ((unsigned ) MAX_LABEL_LENGTH)) ? 
			                name2_buf : getspace(strlen(p2->ops[m2])); 

	*name1 = *name2 = '\0';
	if (m1) n1 = decompose(p1->ops[m1],&x1,name1,&scale1);
	if (m2) n2 = decompose(p2->ops[m2],&x2,name2,&scale2);
	/* if one operand is of the form n(%ebp) and was commented by
	** the compiler for being assigned to a register, then it is
	** different from the second one.
	** if ebp elimination was done, then it became m(%esp)
	*/
	if (fp_removed || fixed_stack) {
		if (!double_aligned) { /*autos and params via esp */
			y1 = p1->ebp_offset;
			y2 = p2->ebp_offset;
		} else { /* autos via esp, params via ebp */
			if (pidx1 & EBP)
				y1 = x1;
			else if (pidx1 & ESP)
				y1 = p1->ebp_offset;
			else
				y1 = 0;
			if (pidx2 & EBP)
				y2 = x2;
			else if (pidx2 & ESP)
				y2 = p2->ebp_offset;
			else
				y2 = 0;
		}
	} else {
		y1 = x1;
		y2 = x2;
	}

	/* get operands which contain regals */
	regal_op1 = getspace(NEWSIZE);
	regal_op2 = getspace(NEWSIZE);
	if (fixed_stack) {
		sprintf(regal_op1,"%d+%s%d(%%esp)",y1,frame_string,func_data.number);
		sprintf(regal_op2,"%d+%s%d(%%esp)",y2,frame_string,func_data.number);
	} else {
		sprintf(regal_op1,"%d(%%ebp)",y1);
		sprintf(regal_op2,"%d(%%ebp)",y2);
	}

	/* if both are regals                               */
	if (   (   double_aligned
		&& (   pidx1 == ESP
		    || pidx1 == EBP)
		&& (   pidx2 == ESP
		    || pidx2 == EBP))
	    || (   !double_aligned
		&& (pidx1 == frame_pointer)
		&& (pidx2 == frame_pointer))) {

		if (   p1->op != CALL
		    && isregal(regal_op1)
		    && p2->op != CALL
		    && isregal(regal_op2)) {
			if ((absdiff(y1,y2)) < (int) (MAX((OpLength(p1)),(OpLength(p2)))))
				return same;
			else
				return different;
		}  /* if */
	}  /* if */
	/* if p1 is regal and p2 is not                     */

	if (   p1->op != CALL
	    && (   (   double_aligned
		    && (   pidx1 == ESP
			|| pidx1 == EBP))
	  	|| (   !double_aligned
		    && ((pidx1 & frame_pointer) == frame_pointer))) 
	    && isregal(regal_op1)) {

		if (   (   m2
			&& !strcmp(p1->ops[m1],p2->ops[m2]))
		    || ((absdiff(y1,y2)) < (int) (MAX((OpLength(p1)),(OpLength(p2))))))
			return same;
		else
			return different;
	}  /* if */

	/* if p2 is regal  and p1 is not                    */

	if (   p2->op != CALL
	    && (   (   double_aligned
		    && (   pidx2 == ESP
			|| pidx2 == EBP))
	  	|| (   !double_aligned
		    && ((pidx2 & frame_pointer) == frame_pointer)))
	    && isregal(regal_op2)) {
		if (   (   m1
			&& !strcmp(p1->ops[m1],p2->ops[m2]))
		    || ((absdiff(y1,y2)) < (int) (MAX((OpLength(p1)),(OpLength(p2))))))
			return same;
		else
			return different;
	 }  /* if */


 	if (   isprefix(p1)
	    || isprefix(p2))
		return unsafe;
 	if (   p1->op == CALL
	    || p2->op == CALL)
		return unsafe;
 	if (   p1->op == MACRO
	    || p2->op == MACRO)
		return unsafe;

	/*if one instruction is push/pop and the second one uses esp
	**then we lie and answer no, we want the dependency type 
	**in that case to be ANTI dependency. Sorry about that...
	*/
 	if (   ispp(p1)
	    && ispp(p2))
		return different;
 	if (ispp(p1)) {
   		if (p2->uses & ESP)
			if (!strcmp(p1->op1,p2->ops[m2]))
				return same;
			else
				return different;
   		else
			pidx1 &= ~ESP;
	}
 	if (ispp(p2)) {
   		if (p1->uses & ESP)
			if (!strcmp(p2->op1,p1->ops[m1]))
				return same;
			else
				return different;
   		else
			pidx2 &= ~ESP;
	}

 	/*The above were cases where an instruction may have two memory  */
 	/* references. Next we treat instructions with exactly one       */
 	/* memory reference.                                             */

	/* check whether both dont use registers. In that case, both are of */
	/* the form n+Label. If labels are different then they are different*/
	/* if Labels are same then if the numbers are different enaugh they */
	/* are different, otherwise same.                                   */
	if ((pidx1 | pidx2) == 0) {
  		if (n1 != n2)
			n2 = 1; /*strings are different*/
  		else
			n2 = strncmp(name1,name2,n1);
  		if (n2)
			return different; /*strings are different -> not same */
  		if ((absdiff(x1,x2)) < (int) (MAX((OpLength(p1)),(OpLength(p2)))))
			return same;
  		else
			return different;
	}
	if(use_trace)
	{
		get_base_indx(p1->ops[m1],&p1_base,&p1_indx);		
		get_base_indx(p2->ops[m2],&p2_base,&p2_indx);		
		i_p1_base = i_reg2ind(p1_base);
		i_p1_indx = i_reg2ind(p1_indx);
		i_p2_base = i_reg2ind(p2_base);
		i_p2_indx = i_reg2ind(p2_indx);
		if(   i_p1_base == iEXX
		   || i_p2_base == iEXX
		   || i_p1_indx == iEXX
		   || i_p2_indx == iEXX )
			use_trace=false;
	}  /* if */

	/* if one instruction indexs a register and the other one doesnt     */
	/* then if the register is either ESP or EBP then they are different */
	/*else if the labels are different then they are different, else same*/
	if (   (! pidx1)
	    && pidx2) {
		/*only p2 indexs a register*/
  		if (pidx2 & (ESP | frame_pointer))
			return different;
  		else {
			if (n1 != n2)
				n1 = 1;
			else
				n1 = strncmp(name1,name2,n1);
			if (n2 == 0)
				return unsafe;   /*if p2 has no fixup, they are same*/
			/*according to labels same or different*/
			if (n1)
				return different;
			else
				return unsafe;
  		}  /*end of this case*/
	}  /* if */
	if (   (! pidx2)
	    && pidx1) {
		/*only p1 indexs a register*/
  		if (pidx1 & (ESP | frame_pointer))
			return different;
  		else {
			if (n1 != n2)
				n2 = 1;
			else
				n2 = strncmp(name1,name2,n1);
			if (n1 == 0)
				return unsafe;   /*if p1 has no fixup, they are same*/
			/*according to labels same or different*/
			if (n2)
				return different;
			else
				return unsafe;
  		}  /*end of this case*/
	}  /* if */

	/* Now both use registers.
	** If both do not have labels then if the registers are different
	** the memory locations are (suspected) different. If the
	** registers are same, were not changed and the difference
	** between the numbers is big enaugh - different, else same.
	** If they both have labels and the labals are different then
	** the memory references are different.
	** If the labales are different and the registers are different then
	** the memory references are (suspected) same.
	** If the registers are same and were not changed between the 
	** two instructions and the difference between the numbers is big
	** enaugh then the memory references are different.
	** otherwise same.
	*/
	if (use_trace)
	{
		/* Recognizing content of operands */
		if(p1_base && p2_base)
		{
			if (   p1->trace[i_p1_base].tag == KNOWN
			    && p2->trace[i_p2_base].tag == KNOWN )
			{
			    bb = strcmp(p2->trace[i_p2_base].value,
					p1->trace[i_p1_base].value) ? different : same ;		
			}
			else bb = unsafe ;
		}
		ss = scale1 == scale2 ;
	}

	if (n1 + n2 == 0) 
	{ /*both dont have labels */
		if (use_trace)
		{
			if (   p1->trace[i_p1_base].kind == ADDRESS 
			    && p2->trace[i_p2_base].kind == ADDRESS)
			{
    		/* Case : base_1 and base_2 contain different labels */
				if( bb == different ) return bb;
				if( bb == same)
				{
					/* Do they have the same index & scale ? */
					if (   (!p1_indx && !p2_indx)
					    || (   p1_indx == p2_indx 
						&& !changed(p1,p2,p1_indx)
						&& ss ))
					{
		 				if (absdiff(x1,x2) < (int) MAX(OpLength(p1),OpLength(p2)))
							return same;
						else 
							return different;
					}
					return unsafe;
				}
			}
		}
	 	if (pidx1 != pidx2)
			return unsafe;
		if (scale1 != scale2 || changed(p2,p1,pidx1))
			return unsafe;
	 	if (absdiff(x1,x2) < (int) MAX(OpLength(p1),OpLength(p2)))
			return same;
	  	else
			return different;
   	}  /*endif both dont have labels*/

   	else if (n1 == 0) {   /*one has label, the other one has not */
	 	if (pidx1 & (frame_pointer | ESP))
			return different;
	 	else
		{
			if (use_trace)
			{ 
			/* case $.Label == [%reg] */ 
				if (   p1_base
				    && !p2_base 
				    && (   (!p1_indx && !p2_indx)
					|| (   p1_indx == p2_indx
					    && !changed(p1,p2,p1_indx)
					    && ss ) )
				    &&  p1->trace[i_p1_base].tag == KNOWN
				    &&  p1->trace[i_p1_base].kind== ADDRESS 
				    &&  !strcmp(p1->trace[i_p1_base].value,name2) ) 
				{
				
					if (x1 == x2 ) return same ;
					else return different;
				}	
			}
			return unsafe;
		}
	} else if (n2 == 0)  {  /*the second has label, the first has not */
	 	if ( pidx2 & (frame_pointer | ESP))
			return different;
	 	else
		{
			if(use_trace)
			{ 
			/* case $.Label == [%reg] */ 
				if (   p2_base
				    && !p1_base 
				    && (   (!p1_indx && !p2_indx)
					|| (   p1_indx == p2_indx
					    && !changed(p1,p2,p1_indx)
					    && ss ) )
				    &&  p2->trace[i_p2_base].tag == KNOWN
				    &&  p2->trace[i_p2_base].kind== ADDRESS 
				    &&  !strcmp(p2->trace[i_p2_base].value,name1) ) 
				{
					if (x1 == x2 ) return same ;
					else return different;
				}	
			}
			return unsafe;
		}
	}
   	/*Both have labels. */
   	if (n1 != n2)
		n2 = 1; /*labels are different*/
   	else
		n2 = strncmp(name1,name2,n1);
   	if (n2)   /*the labels are different*/
		return different;
   	if (   pidx1 != pidx2
	    || scale1 != scale2
	    || changed(p2,p1,pidx1))
		return unsafe;
   	if (absdiff(x1,x2) < (int) MAX(OpLength(p1),OpLength(p2)))
		return same;
   	else
		return different;
#undef iEXX 		
}  /*end seems_same*/


/*For each node find the length of the longenst chain to the leaves
**in the DAG.
*/
static void
calc_depth()
{ 
NODE * p;
	for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
  		if (p->nparents == 0)
			(void) depth(p); /*initiate from every root*/
  	}  /* for loop*/
}  /*end calc_depth*/

static int
depth(p) NODE *p;
{
 	if (   !p->dependent
	    && !p->anti_dep
	    && !p->agi_dep
	    && !p->may_dep) {
   		p->chain_length = 0;
   		return 0;
	} else if (p->chain_length != 0)
		return (p->chain_length);
 	else {
		int l = 1;
		if( target_cpu == P6 )  l = get_latency(p) ;
		p->chain_length = l + depth_4list(p);
		return (p->chain_length);
	}
}  /*end depth*/

static int
depth_4list(p)
NODE *p;
{
tdependent * tp;
int value =0;
int value1,i;
int N = target_cpu == P6 ? MAY : ANTI ;

  	for(i=N;i<=AGI;i++){
		for(tp= p->depend[i]; tp != NULL ; tp = tp->next) {
			value1 = depth(tp->depends);
			if (value1 > value)
    				value = value1;
 		}  /*end for loop*/
  	}  /*end for loop*/
  	return value;
}  /*end depth_4lists*/


/*For each node, how many direct sons it has.*/
static void
calc_width()
{
tdependent * tp;
NODE * p;
int i;
	for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
		for (i = ANTI ; i <= AGI ; i++)
			for (tp = p->depend[i]; tp ; tp = tp->next)
				p->dependents++; 
  	}  /*for all instructions in the block*/
}  /*end calc_width*/

static void
nail()
{
NODE *nailed;
NODE *p;
	for (nailed = b->firstn; nailed != b->lastn; nailed = nailed->forw) {
		if (Unexpected(nailed)) {
   			for (p=b->firstn; p != nailed; p=p->forw)
	 			install1(p,nailed,DEP);
   			for (p=nailed->forw; p != b->lastn->forw; p = p->forw)
	 			install1(nailed,p,DEP);
		}
	}
}  /*end nail*/

/*The second module of the scheduler: reorder the instruction of the basic
**block.
*/
static NODE *lastX86 = NULL, *lastV = NULL;
static int successive_memories;
static int scheded_inst_no = 0;

int come_from;

static void
prun()
{
NODE * last_scheded = NULL,*new_scheded;
int p6_before();
  	m0.next = &m1;
  	m1.back = &m0;
  	scheded_inst_no = 0;
	switch (target_cpu) {
		case P6:
			init_pick();
			static_analysis();
			break;
		case P5:
		case blend:
			fp_start();
    		lastX86 = lastV = NULL;
			break;
		case P4:
			fp_start();
    		successive_memories = 0;
			break;
	}
  	while (scheded_inst_no < b->length) { /*while not all were scheded*/
		if (target_cpu == P6) make_clist(p6_before);
		else make_clist(before);    /* make it already ordered */
		if (target_cpu & (P5 | blend)) {
			pick1or2();
			m0.next = fp0;
			dispatch1or2();
			remove1or2();
			scheded_inst_no += ( lastV == NULL ? 1:2);
		}
		else if (target_cpu == P4) 
		{ /*if i486 optimizations*/
			if ( last_scheded) {
				new_scheded = pick_one(last_scheded);
				last_scheded->usage = ((FP(new_scheded->op) && concurrent_value)
					|| agi_depends(last_scheded,new_scheded)
					|| successive_memories > 3) ? LOST_CYCLE : NONE;
				last_scheded = new_scheded; 
			} else
				last_scheded = pick_one(last_scheded);
			m0.next = fp0;
			dispatch(last_scheded);
			update_clist(last_scheded); /*remove the last scheded from clist*/
	  		if (   touchMEM(last_scheded)
			    && (get_exe_time(last_scheded) == 1))
				successive_memories++;
			else
				successive_memories = 0;
			scheded_inst_no++;
		}
		else if (target_cpu == P6)
		{
			p6_pick();
			p6_dispatch();
			p6_update_clist();
		}	
  	}  /*while loop*/
	if ( last_scheded) 
		last_scheded->usage = NONE;
}  /*end prun*/

/*collect all nodes with currently zero parents*/
static void
make_clist(compar) int	(*compar)();
{
NODE * p;
clistmem * tmp;
clistmem * scan;
static clistmem free_list[BLOCK_SIZE_LIMIT+2];
static	int next_free = 0;
	/*loop on all instructions and find the ones with no parents */
	/*insert the members into places according to the order      */
	/*induced by before().                                      */
	if (m0.next == &m1)
		next_free = 0;
	for (p = prev->forw ; p != b->lastn->forw; p = p->forw) {
		if (p->nparents == 0) {                             /*found one*/
			p->nparents= -1;
	  		/*find where to insert the new member. point there with scan*/
			scan = m0.next;
			while (   (scan != &m1)
			       && ((*compar)(scan->member,p) == 1))
				scan = scan->next;
			tmp = scan->back;
			tmp->next = &free_list[next_free++];
			tmp->next->next = scan;
			tmp->next->back = tmp;
			scan->back = tmp->next;
			tmp->next->member = p;
		}  /*endif nparents == 0 */
	}  /*for loop*/
  	if (m0.next == &m1)
		fatal(__FILE__,__LINE__,"Empty candidate list\n");
}  /*end make_clist*/


/*Who comes first in the candidate list*/
static int
before(p1,p2) NODE *p1, *p2;
{
	if (FP(p1->op)) {
		if (target_cpu == P5) {
			if (! FP(p2->op))
				return 1;
			if (FPPREFERENCE(p1->op) > FPPREFERENCE(p2->op))
				return 1;
			if (FPPREFERENCE(p1->op) < FPPREFERENCE(p2->op)) 
				return 2;
			return natural_order(p1,p2);
		} else return 1;
	}
	else if (FP(p2->op))
		return 2;
	if (p1->sets & p2->idxs)
		return 2; /* found AGI */
	if (p2->sets & p1->idxs)
		return 1; /* found AGI */
	if (target_cpu & (P5 | blend)) {
  		if (   (p1->anti_dep)
		    && (! p2->anti_dep))
			return 1;
  		if (   (p2->anti_dep)
		    && (! p1->anti_dep))
			return 2;
  		if (p1->exec_info.pairtype < p2->exec_info.pairtype)
			return 1;
  		if (p1->exec_info.pairtype > p2->exec_info.pairtype)
			return 2;
	} else {
  		if (   (has_agi_dep(p1))
		    && (! has_agi_dep(p2))) {
			return 1;
		}
  		if (   (has_agi_dep(p2))
		    && (! has_agi_dep(p1))) {
			return 2;
		}
	}
	if (p1->chain_length > p2->chain_length) {
		return 1;
	}
	if (p1->chain_length < p2->chain_length) {
		return 2;
	}
	if (p1->dependents   > p2->dependents) {
		return 1;
	}
	if (p2->dependents   > p1->dependents) {
		return 2;
	}
	return natural_order(p1,p2);
}  /*end before*/

/*who is first in the original code*/
static int
natural_order(p1,p2) NODE *p1, *p2;
{
 NODE *p;
 	for(p = b->firstn; p != b->lastn->forw; p=p->forw) { 
   		if (p == p1)
			return 1;
   		if (p == p2)
			return 2;
 	}  /*for loop*/
/* NOTREACHED */
}  /*end natural_order*/

/*Pick the following instruction. Used for i486 optimizations.*/
static NODE *
pick_one(last_scheded) NODE * last_scheded;
{
clistmem *clp;
int one_cycle;
static NODE *current_fp;

	fp0 = m0.next;
	if (   last_scheded
	    && isprefix(last_scheded))  {
		next_of_prefix->nparents = -1;
#ifdef DEBUG
	if (vflag)
		next_of_prefix->op4 = "\t\t  / prefixed ";
#endif
		return next_of_prefix;
	}
	if (   concurrent_value
	    && FP(m0.next->member->op)
	    && m0.next->next != &m1
	    && !current_fp) /* if fp busy and there is at list one more
		                        candidate skip the fp node in the list */
		m0.next = m0.next->next;
	else if (FP(m0.next->member->op)) {
		current_fp = NULL;
#ifdef DEBUG
		if (vflag)
			m0.next->member->op4 = concurrent_value ? 
				"\t\t / FP concurrent(+)" : "\t\t / FP concurrent(0)";
#endif
		return m0.next->member;
	}
	if (last_scheded == NULL)  {
#ifdef DEBUG
		if (vflag)
			m0.next->member->op4 = "\t\t / m0.next";
#endif
		return m0.next->member;
	}
	one_cycle = get_exe_time(last_scheded) == 1;
	switch (successive_memories) {
	case 0:
	case 1:
		for (clp = m0.next; clp != &m1; clp = clp->next)
			if (   ! agi_depends(last_scheded,clp->member)
			    && (!(   Hasprefix(clp->member)
				  && one_cycle))) {
#ifdef DEBUG
				if (vflag)
					clp->member->op4 = "\t\t / sm 0,1 ";
#endif
				return clp->member;
			}  /* if */
		break;
    	case 2:
		for (clp = m0.next; clp != &m1; clp = clp->next)
			if (   !agi_depends(last_scheded,clp->member)
			    && (!(   Hasprefix(clp->member)
				  && one_cycle))
			    && !touchMEM(clp->member)) {
#ifdef DEBUG
		 		if (vflag)
					clp->member->op4 = "\t\t / sm2, no mem";
#endif
				return clp->member;
			}  /* if */
		for (clp = m0.next; clp != &m1; clp = clp->next)
			if (   ! agi_depends(last_scheded,clp->member)
			    && (!(   Hasprefix(clp->member)
				  && one_cycle))) {
#ifdef DEBUG
				if (vflag)
					clp->member->op4 = "\t\t / sm2, ";
#endif
				return clp->member;
			}  /* if */
	  		break;
    	default:
		for (clp = m0.next; clp != &m1; clp = clp->next)
			if (   !agi_depends(last_scheded,clp->member)
			    && ( ! (   Hasprefix(clp->member)
				    && one_cycle))
			    && !touchMEM(clp->member)) {
#ifdef DEBUG
				if (vflag)
					clp->member->op4 = "\t\t / smd, no nothimg";
#endif
				return clp->member;
			}  /* if */
#if we_want_the_agis
		for (clp = m0.next; clp != &m1; clp = clp->next)
			if (! touchMEM(clp->member)) {
#ifdef DEBUG
				if (vflag)
					clp->member->op4 = "\t\t / smd, no mem";
#endif
				return clp->member;
			}  /* if */
#endif
		for (clp = m0.next; clp != &m1; clp = clp->next)
			if (   !agi_depends(last_scheded,clp->member)
			    && (!(   Hasprefix(clp->member)
				  && one_cycle))) {
#ifdef DEBUG
				if (vflag)
					clp->member->op4 = "\t\t / smd, no agi";
#endif
				return clp->member;
			}  /* if */
		break;
  	}  /*end switch*/
	for (clp = m0.next; clp != &m1; clp = clp->next) {
		if ( !agi_depends(last_scheded,clp->member) )  {
#ifdef DEBUG
			if (vflag)
				clp->member->op4 = "\t\t /last option, no agi";
#endif
			return clp->member;
	 	}  /* if */
	}  /* for */
#ifdef DEBUG
	if (vflag)
		m0.next->member->op4 = "\t\t / last option";
#endif
	return m0.next->member;
}  /*end pick_one*/

/*Does p1 agi - depend on p */
static int
agi_depends(p,p1) NODE *p, *p1;
{
	
	if (   !p
	    || !p1
	    || ! (p->sets & ~(MEM | CONCODES_AND_CARRY)))
		return false;  /* No register setting by p */

	if (   ispp(p)
	    && ! (p->sets & p1->idxs & ~ESP))
		return false;  /* No AGI with ESP setting by push/pop */

	if (p->sets & p1->idxs)
		return true; /* found AGI */

	if (target_cpu & (P5 | blend))
		return false;

	if (   (p->sets & p1->uses & ~(MEM | CONCODES_AND_CARRY))
					/* sets and uses same reg */
	    && ! (p->sets & R16MSB) /* less then full reg set */
	    && (p1->uses & R16MSB)) /* full reg uses */
		return true;

	return false;
}  /*end agi_depends*/

/* for each instruction which is not yet scheduled:
** mark the usage field of the node if moving if forward relative to it's
** current location may intruduce an AGI.
*/
static void
mark_new_agis()
{
NODE *first = lastV ? lastV->forw : lastX86 ? lastX86->forw : b->firstn;
NODE *q;
unsigned int indexes = 0;
	for (q = first; q != b->lastn->forw; q = q->forw)
		q->usage = NONE;
	for (q = first; q != b->lastn->forw; q = q->forw) {
		if (q->sets & indexes) q->usage = AGI;
		indexes |= q->idxs;
	}
}  /*end mark_new_agi*/

/*given an instruction p and a second instruction q, is it possible that if p
**is picked than it will generate an AGI with an instruction other than q?
**useful in the case that p is marked as makes_new_agi, is a candidate for
**being the second one in the pair, and makes_new_agi only with the first one
**in the pair - than it does not really makes_new_agi.
*/
static boolean
uniq_agi(p,q) NODE *p,*q;
{
NODE *first = lastV ? lastV->forw : lastX86 ? lastX86->forw : b->firstn;
NODE *l;
unsigned int indexes = 0;
	for (l = first; l != p; l = l->forw) {
		if (l == q) continue;
		indexes |= l->idxs;
	}  /* for */
	if (p->sets & indexes) return false;
	else return true;
}  /*end uniq_agi*/

/*An instruction will defenately make a new agi if it makes a new agi with
**one of the first two instructions after NODE *prev, and that one has 
**chain_length equal to the rest of the basic block.
*/
static boolean
defenate_new_agi(left,right) NODE *left,*right;
{
NODE *first = prev->forw;
NODE *second = first->forw;
clistmem *clp;
int i;
tdependent *tp;
unsigned int sets = left->sets | right->sets;
int needed = 0;
	if (   !(sets & first->idxs)
	    && !(sets & second->idxs)) return false;
	if (sets & first->idxs) {
		needed++;
	}
	if (sets & second->idxs) {
		needed++;
	}
	for (clp = m0.next; clp != &m1; clp = clp->next) {
		if (   clp->member == left
		    || clp->member == right
		    || clp->member == first
		    || clp->member == second) continue;
		if (   !(clp->member->sets & first->idxs) 
		    && !(clp->member->sets & second->idxs)) {
			needed--;
			if (needed == 0) return false;
		}  /* if */
		for (i = ANTI; i <= AGI; i++) {
			for (tp = first->depend[i]; tp; tp = tp->next) {
			/* Here we had (on earfilters) a non null tp in the chain
			** with tp->depends == 0.
			*/
				if (   tp->depends->nparents == 1
				    && !(tp->depends->sets & first->idxs)
				    && !(tp->depends->sets & second->idxs)) {
					needed--;
					if (needed == 0) return false;
				}  /* if */
			}  /* for */
			for (tp = second->depend[i]; tp; tp = tp->next) {
				if (   tp->depends->nparents == 1
				    && !(tp->depends->sets & first->idxs)
				    && !(tp->depends->sets & second->idxs)) {
					needed--;
					if (needed == 0) return false;
				}  /* if */
			}  /* for */
		}  /* for */
	}  /* for */
	return true;
}  /*end defenate_new_agi*/

/*Remove the issued instruction from the candidate list.
**Dont assume that last_scheded is really in the clist
*/
static void
update_clist (last_scheded) NODE * last_scheded;
{
clistmem * clp;
	for (clp=m0.next;clp!=&m1 && clp->member!=last_scheded;clp=clp->next)
		;
	/*clp now points to the clist member with the scheded instruction*/
	/*delete that member from the clist                          */
	if (clp != &m1) {
		clp->back->next = clp->next;
		clp->next->back = clp->back;
	}  /*endif clp*/
}  /*end update_clist*/


/*Make some book keeping for the issued instruction.*/
static void
dispatch(p) NODE * p;
{
tdependent * tp;
int reg,i;
int amount;
	if (p->agi_dep == &dummy_to_show_agi)
		p->agi_dep = NULL;
 	/*decrement the # of parents of the dependents of p if any.     */
	for (i = ANTI ; i <= AGI ; i++)
  		for (tp=p->depend[i]; tp ; tp= tp->next)
    			tp->depends->nparents--;
	
	/* record the amount of constant changes for the register(s) set by p*/
	for (reg = 0; reg < NREGS; reg++) {
		if (p->sets & regmasx[reg]) {
			if (change_by_const(p,regmasx[reg],&amount)) {
				depths[reg] += amount;
			} else {
				depths[reg] = 0;
			}  /* if */
		}  /* if */
	}  /* for */

  	if (target_cpu & (P4|blend))
		if (FP(p->op))
			concurrent_value = CONCURRENT(p->op);
		else {
			if (   prev != b->firstn->back
			    && agi_depends(prev,p))
				concurrent_value--;
			concurrent_value -= get_exe_time(p);
			if (concurrent_value < 0)
			 	concurrent_value = 0;
		}  /* if */
  	reorder(p); /* change pointers of NODEs to move p to new place */
	fix_offsets(p); /*account for changes of index regs by consts  */
	p->uses |= p->idus; /* no more uses but not indexing */
  	if ( FP(p->op)) {
  		if (target_cpu == P5)
  			reset_fp_reg(p);
  		else if (target_cpu & blend)
  			 fp_set_resource(p);
  	}   /* if */
}  /*end dispatch*/

static void
remove1or2()
{
  	update_clist(lastX86);
  	if (lastV)
		update_clist(lastV);
}  /*end remove1or2*/

static void
dispatch1or2()
{	/* first count the AGI cycle and use this cycle for fxch */
	if (   agi_depends(prev,lastX86)
	    || (   lastV
		&& agi_depends(prev,lastV))
	    || (   (prev->usage == ON_J )
		&& agi_depends(prev->back,lastX86) ||
			(lastV && agi_depends(prev->back,lastV)))) {

/* NOTE jlw ....   This compound conditional looks poorly formed
**                   and I suspect it should be something like:

	if (   agi_depends(prev,lastX86)
	    || (   lastV
		&& agi_depends(prev,lastV))
	    || (   (prev->usage == ON_J )
		&& (   agi_depends(prev->back,lastX86)
		    || (lastV && agi_depends(prev->back,lastV))))) {
**
**  Check it OUT !!!
*/
		fp_ticks(1);
		last_float = prev;
	} 
	if (! lastV) {
		dispatch(lastX86);
		if (! FP(lastX86->op))
			fp_ticks(get_exe_time(lastX86));
	}  /*endif*/
  	else {
		dispatch(lastX86);
		dispatch(lastV);
		fp_ticks(get_exe_time(lastX86) + get_exe_time(lastV) -1);
	}
}  /*end dispatch1or2*/

NODE *
insert_nop(p) NODE *p;
{
NODE *tmp;
	tmp = insert(p);
	chgop(tmp,NOP,"nop");
	tmp->op1 = tmp->op2 = tmp->op3 = NULL;
	tmp->dependent = tmp->anti_dep = tmp->agi_dep = tmp->may_dep = NULL;
	tmp->sets = tmp->uses = 0;
	return tmp;
}  /*end insert_nop*/


/*In case of pentium optimizations, find the best choice of the next one
**or two instructions to be issued.
*/
/*definitions of weights of quolities of pairs */
#define NOT_MEM	1
#define UNSAFE	2
#define RIGHT	4
#define DEPENDENT	8
#define ANTI_D	16
#define HAS_AGI_DEP	32
#define REVERSE_AGI	128
#define BOTH_AGI_UP	256
#define NONE_AGI_UP	512

static NODE *best_left,*best_right;
static int best_weight;

static void
pick1or2()
{
clistmem * clp;
NODE * pickedL;
	fp0 = m0.next;
	best_left = best_right = NULL;
	best_weight = 0;
	/*  Special case                           */
	if (  lastX86
	    && isprefix(lastX86)) {
		lastX86 = next_of_prefix;
		lastX86->nparents = -1;
		lastV = NULL;
		lastX86->usage = ALONE;
#ifdef DEBUG
		if (vflag)
			lastX86->op4 = "\t\t / prefixed";
#endif
		return;
	}  /* if */
	/*       FIRST PREFERENCE:                */
	/*  float instruction.                    */
	if ((pickedL = fp()) != NULL) {
		lastX86 = pickedL;
		lastV = NULL;
		return;
	}  /* if */
 
 	/*       SECOND PREFERENCE:                */
 	/*X86 no agi up, having agi dependents    */
 	if ((pickedL = EX86(1)) != NULL) {
		lastX86 = pickedL;
		lastX86->usage = ALONE;
#ifdef DEBUG
		if (vflag)
			lastX86->op4 = "\t\t/ X8601";
#endif
		lastV = NULL;
		return;
	}  /* if */
 
	/* find best pair and it's weight */
	pair();

	/* THIRD PREFERENCE                       */
	/*a pair, no agi up, both have agi dependents*/
	if (best_weight >= (NONE_AGI_UP + HAS_AGI_DEP + HAS_AGI_DEP )) {
		lastX86 = best_left;
		lastV = best_right;
		lastV->nparents = -1;
		lastX86->usage = ON_X;
		lastV->usage = ON_J;
#ifdef DEBUG
		if (vflag) {
			lastX86->op4 = " \t\t/ left 1 ";
			lastV->op4 = getspace(32);
			sprintf(lastV->op4,"\t/ right 1-%d:%d",come_from,best_weight);
		}  /* if */
#endif
		return;
	}  /* if */

	/*     FOURTH  PREFERENCE                   */
	/*X86 no agi up, no agi dependents        */
	if ((pickedL = EX86(0)) != NULL) {
		lastX86 = pickedL;
		lastX86->usage = ALONE;
#ifdef DEBUG
		if (vflag)
			lastX86->op4 = "\t\t/ X8600";
#endif
		lastV = NULL;;
		return;
 	}  /* if */

	/* FIFTH PREFERENCE                         */
	/* a pair with no agi up, no agi dependents. */
	/* It has to be stricktly bigger that BOTH because we want to exlude
	** the possibility of agis with the previous pair, yet it can be less
	** than NONE because of subtractions due to possible new agis down.
	*/
	if (best_weight > BOTH_AGI_UP) {
		lastX86 = best_left;
		lastV = best_right;
		lastV->nparents = -1;
		lastX86->usage = ON_X;
		lastV->usage = ON_J;
#ifdef DEBUG
		if (vflag) {
			lastX86->op4 = " \t\t/ left 2 ";
			lastV->op4 = getspace(32);
			sprintf(lastV->op4,"\t/ right 2-%d:%d",come_from,best_weight);
		}  /* if */
#endif
		return;
	}  /* if */

	/* SIXTH  PREFERENCE                      */
	/* a pairable with no pair and no agi up     */
	for (clp = m0.next; clp != &m1; clp = clp->next) {
		if (! (   agi_depends(lastX86,clp->member)
		       || (agi_depends(lastV,clp->member)))) {
			lastX86 = clp->member;
			lastX86->usage = ALONE;
#ifdef DEBUG
			if (vflag) 
				lastX86->op4 = "\t\t/ single";
#endif
			if (   target_cpu == P5
			    && best_weight
			    && lastV) {
				/* not for blended */
				lastV = insert_nop(lastX86);
				lastX86->usage = ON_X;
				lastV->usage = ON_J;
				b->length++;
			} else lastV = NULL;
			return;
		}  /* if */
	}  /* for */

 	/* SEVENTH  PREFERENCE                        */
	/* anything that is decided best by pair() will do */
	if (best_weight) {
		lastX86 = best_left;
		lastV = best_right;
		lastV->nparents = -1;
		lastX86->usage = ON_X;
		lastV->usage = ON_J;
#ifdef DEBUG
		if (vflag) {
			lastX86->op4 = " \t\t/ left 3 ";
			lastV->op4 = getspace(32);
			sprintf(lastV->op4,"\t/ right 3-%d:%d",come_from,best_weight);
		}  /* if */
#endif
		return;
	}  /* if */


	/* TWELEVETH AND LAST                          */
	lastX86 = m0.next->member;
	lastX86->usage = ALONE;
#ifdef DEBUG
	if (vflag) 
		lastX86->op4 = "\t\t/ last option";
#endif
	lastV = NULL;
	return;
}  /*end pick1or2*/


/*scan the c-list for an X86 type instruction*/
static NODE *
EX86(agidown)
int agidown;
{
clistmem *clp;
	for (clp = m0.next; clp != &m1; clp = clp->next){
		if (   clp->member->exec_info.pairtype == X86
		    && (! agi_depends(lastX86,clp->member))
		    && (! agi_depends(lastV,clp->member))
		    && ((! agidown) || has_agi_dep(clp->member)))
			return clp->member;
	}  /*end for loop*/

	return NULL;
}  /*end EX86*/

static NODE *
fp()
{
int time;
clistmem *clp;
int min_time,min_add_fxch,add_fxch;
NODE *min_time_p;
	min_time = 1000;
	min_add_fxch = true;
	min_time_p = NULL;
	for (clp = m0.next; clp != &m1; clp = clp->next) {
		if (FP(clp->member->op)) {
			time = (   agi_depends(lastX86,clp->member)
				|| agi_depends(lastV,clp->member));
			add_fxch = check_for_fxch(clp->member);
			if (! (time += fp_time(clp->member))) {
				if (!add_fxch ) {
#ifdef DEBUG
					if (vflag) 
						clp->member->op4 = "\t\t/ fp (T0)";
#endif
					clp->member->usage = ALONE;
	 				return clp->member;
				}  /* if */
				min_time_p = clp->member;
				min_time = 0;
				min_add_fxch = true;
			} else if (   time < min_time
				   || (   time == min_time
				       && min_add_fxch
				       && ! add_fxch)) {

					min_time_p = clp->member;
					min_time = time;
					min_add_fxch = add_fxch;
			}  /* if */
		} else {
			if (! min_time) { /* fxch added but no extra time */ 
#ifdef DEBUG
				if (vflag) 
					min_time_p->op4 = "\t\t/ fp (T0)";
#endif
				min_time_p->usage = ALONE;
				return min_time_p;
			}  else if (   lastX86
				    && lastX86->op == FXCH
				    && min_time_p != NULL) {
#ifdef DEBUG
				if (vflag)
					min_time_p->op4 = "\t\t/ fp (T+)";
#endif
				min_time_p->usage = ALONE;
				return min_time_p;
			}  /* if */
			fp0 = m0.next; /* non fp was found don't look for fp any more */
			m0.next = clp;
			return NULL;
		}  /* if */
 	}  /*end for loop*/
	if (min_time_p != NULL ) {
#ifdef DEBUG
		if (vflag)
			if ( min_time)
				min_time_p->op4 = "\t\t/ fp (T+)";
			else
				min_time_p->op4 = "\t\t/ fp (T0)";
#endif
		min_time_p->usage = ALONE;
	}  /* if */
	return min_time_p; /* only FP in the list */
}  /*end fp() */

static void
mark_cur_agis()
{
NODE *first = lastV ? lastV->forw : lastX86 ? lastX86->forw : b->firstn;
NODE *q;
unsigned int indexes = 0;
	/* unmark previous marks */
	for (q = first; q != b->lastn->forw; q = q->forw)
		if (q->agi_dep == &dummy_to_show_agi) q->agi_dep = NULL;

	/* Go on the block backwards. Collect all indexed registers.
	** If an instruction sets any of the above registers - mark it.
	** Do the test before the collect in order not to mark
	** movl (%eax),%eax
	*/
	for (q = b->lastn; q != first->back; q = q->back) {
		if (   !(q->agi_dep)
		    && q->sets & indexes)
			q->agi_dep = &dummy_to_show_agi;
		indexes |= q->idus;
	}  /* for */
}  /*end mark_cur_agis*/

/*Scan the c-list for a pair of instructions.*/
static void
pair()
{
NODE  *pickedL=NULL,*pickedR=NULL;
clistmem *clpl=NULL,*clpr=NULL;
int cur_weight;

	mark_cur_agis();
	mark_new_agis();
	for (clpl = m0.next; clpl != &m1; clpl = clpl->next) {
		if ((pickedL = islefthy(clpl->member)) != NULL) {
			/*is there an anti dependent     */
			if ((pickedR = Eanti_dependent(pickedL)) != NULL) {
				cur_weight = find_weight(pickedL,pickedR) + ANTI_D;
				if (cur_weight > best_weight) {
					best_weight = cur_weight;
					best_left = pickedL;
					best_right = pickedR;
					come_from = 1;
				}  /* if */
       			}  /* if */

        /* a dependent, like PUSH PUSH */

			if (   ispp(pickedL)
			    && isior(pickedL->op1)) {

				if ((pickedR = Edependent(pickedL)) != NULL) {
					cur_weight = find_weight(pickedL,pickedR) + DEPENDENT;
					if (cur_weight > best_weight) {
						best_weight = cur_weight;
						best_left = pickedL;
						best_right = pickedR;
						come_from = 2;
					}  /* if */
       				}  /* if */
			}  /* if */

          /* a clist member */

			for (clpr = m0.next; clpr != &m1; clpr = clpr->next) {
				if (   clpl != clpr
				    && !agi_depends(clpl->member,clpr->member)
				    && (pickedR = isright(clpr->member)) != NULL) {
					cur_weight = find_weight(pickedL,pickedR) + RIGHT;
					if (cur_weight > best_weight) {
						best_weight = cur_weight;
						best_left = pickedL;
						best_right = pickedR;
						come_from = 3;
					}  /* if */
				}  /*endif found right*/
			}  /*for all clist members*/


 	/* an unsafe pair.                          */

			if ((pickedR = Eunsafe_right(pickedL)) != NULL) {
				cur_weight = find_weight(pickedL,pickedR) + UNSAFE;
				if (cur_weight > best_weight) {
					best_weight = cur_weight;
					best_left = pickedL;
					best_right = pickedR;
					come_from = 4;
				}  /* if */
			}  /* if */

		}  /*endif Islefthy*/
  	}   /*for all clist members*/
}  /*end pair*/

static NODE *
Eunsafe_right(left) NODE *left;
{
tdependent *tp;

	if (left->may_dep)
		for (tp = left->may_dep; tp; tp=tp->next) {
			if (   tp->depends->nparents == 1
			    && (   tp->depends->exec_info.pairtype == WDA
				|| tp->depends->exec_info.pairtype == WD2)) {

				return tp->depends;
			}  /* if */
		}  /* for */
	return NULL;
}  /*end Eunsafe_right*/

static int
find_weight(left,right) NODE *left,*right;
{
int weight = 0;
boolean agi_left,agi_right;
boolean left_has_agi_dep = has_agi_dep(left);
boolean right_has_agi_dep = has_agi_dep(right);

	agi_left = agi_depends(lastX86,left) || agi_depends(lastV,left);
	/*if right is multy cycle, it does not have AGI with last X86 */
	if (get_exe_time(right) > 1)
		agi_right = agi_depends(lastV,right);
	else
		agi_right = agi_depends(lastX86,right) || agi_depends(lastV,right);
	if (   !touchMEM(left)
	    || !touchMEM(right)) {
		weight += NOT_MEM;
	}
	if (   agi_depends(right,left)
	    && right_has_agi_dep) {
		weight += REVERSE_AGI;
	}
	if (left->agi_dep) {
		weight += HAS_AGI_DEP;
	}
	if (right->agi_dep) {
		weight += HAS_AGI_DEP;
	}
	if (   ! left_has_agi_dep
	    && makes_new_agi(left)) {
		weight -= HAS_AGI_DEP;
	}
	if (   ! right_has_agi_dep
	    && makes_new_agi(right)
	    && !uniq_agi(right,left)) {
		weight -= HAS_AGI_DEP;
	}
	if (  agi_left
	    && agi_right) {
		weight += BOTH_AGI_UP;
	}
#if 0
	if (defenate_new_agi(left,right)) {
		weight -= BOTH_AGI_UP;
	}
#endif
	if (   !agi_left
	    && !agi_right) {
		weight += NONE_AGI_UP;
	}
	return weight;
}  /*end find_weight*/

/*Is p suitable to run on the U pipe*/
static NODE *
islefthy(p) NODE * p;
{
	if (   (p->exec_info.pairtype == WD1)
	    || (p->exec_info.pairtype == WDA))
		return p;
	return NULL;
}  /*end islefthy*/

/*Is p suitable to run on the V pipe*/
static NODE *
isright(p) NODE *p;
{
	if (   (p->exec_info.pairtype == WD2)
	    || (p->exec_info.pairtype == WDA))
		return p;
	return NULL;
}  /*end isright*/

/*Does p have an anti dependent son which may be paired with it*/
static NODE *
Eanti_dependent(p) NODE *p;
{
tdependent * tp;
	for (tp = p->anti_dep; tp; tp = tp->next) {
		if (   (   (tp->depends->exec_info.pairtype == WD2)
			|| (tp->depends->exec_info.pairtype == WDA))
		    && (tp->depends->nparents == 1))
			return tp->depends;
	}  /* for */
	return NULL;
}  /*end Eanti_dependent*/

/*Does p have a dependent son which may be paired with it*/
static NODE *
Edependent(pickedL) NODE * pickedL;
{
tdependent * tp;
	if (pickedL->op == PUSHL)
		for (tp = pickedL->dependent; tp; tp=tp->next)
			if (   (tp->depends->op == CALL)
			    && (tp->depends->nparents == 1))
				return tp->depends;
	return NULL;
}  /*end Edependent*/


/*remove pointers to put p next after the previously scheduled instruction*/
static void
reorder(p) NODE *p;
{
 	if (   (   (target_cpu & (P5 | blend))
		&& scheded_inst_no == 0
		&& p == lastX86) 
	    || (   (   target_cpu == P4
		    || target_cpu == P6)
		&& scheded_inst_no == 0)) {

		b->firstn = p;
	}  /* if */
	if (isprefix(p))
		next_of_prefix = p->forw;
  	if (prev->forw != p) {
		if (p == b->lastn)
	  		b->lastn = b->lastn->back;
		p->back->forw = p->forw;
		p->forw->back = p->back;
		p->forw = prev->forw;
		p->back = prev;
		prev->forw->back = p;
		prev->forw = p;
  	}  /* if */
  	prev = p;
}  /*end reorder*/

static void
count_depths_before_reorder()
{
NODE *p;
unsigned int regnum;
int reg;
int amount;
int m;
char *t;
int offset;
NODE *q;
	for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
		p->esp_offset = depths[7];
		for (reg = 0; reg < 8; reg++) {
			if (p->sets & regmasx[reg]) {
				if (change_by_const(p,regmasx[reg],&amount)) {
					depths[reg] += amount;
				} else {
					depths[reg] = 0;
				}  /* if */
			}  /* if */
		}  /* for all reg*/
		if (p->sets & ESP) { /*disallow negative esp offset */
			for (q = p->back; q != b->firstn->back; q = q->back) {
				if (q->idus == ESP) {
					if (   q->op1
					    && strchr(q->op1,'(')) {
						 m = 1;
					} else if (   q->op2
						   && strchr(q->op2,'('))  {
						m = 2;
					} else if (   q->op3
						   && strchr(q->op3,'('))  {
						m = 3;
					}
					else m = 0;
					if (m) {
						offset = q->esp_offset + (int)strtoul(q->ops[m],NULL,10);
						if (offset < p->esp_offset + amount) {
							install1(q,p,ANTI);
						}
					}  /* if */
				}  /* if */
			}  /* for */
		}  /* if */
		if (p->idus) {
			if (p->idus == ESP) {  /*disallow negative esp offset */
				if (   p->op1
				    && strchr(p->ops[1],'(')) {
					 m = 1;
				} else if (   p->op2
					   && strchr(p->ops[2],'(')) {
					 m = 2;
				} else if (   p->op3
					   && strchr(p->ops[3],'(')) {
					 m = 3;
				}
				else m = 0;
				if (m) { /* indexing is explicite */
					offset = p->esp_offset + (int)strtoul(p->ops[m],NULL,10);
					for (q = p->back; q != b->firstn->back; q = q->back) {
						if (q->sets & ESP) {
							if (offset < q->esp_offset) {
								install1(q,p,AGI);
								break;
							}  /* if */
						}  /* if */
					}  /* for */
				}  /* if */
			}  /* if */
			for (m = 1; m <=3; m++) {
				if (   !p->ops[m]
				    || (t = strchr(p->ops[m],'(')) == 0)
					continue;
				regnum = setreg(t+1); /*base reg*/
				for (reg = 0; reg < NREGS; reg++)
					if (regmasx[reg] == regnum)
						break;

				p->extra = depths[reg];
				t = strchr(t,',');
				if (t) {
					regnum = setreg(t+1); /*index register*/
					for (reg = 0; reg < NREGS; reg++)
						if (regmasx[reg] == regnum)
							break;

					p->extra2 = depths[reg];
				}  /* if */
			}  /*for loop*/
		}  /*endif p->idus*/
	}  /*end for loop*/
	for (reg = 0; reg < NREGS; reg++) /*prepare for use at prun()*/
		depths[reg] = 0;
}  /*end count_depths_before_reorder*/

static void
fix_offsets(p) NODE *p;
{
int reg;
int change =0;
char name_buf[MAX_LABEL_LENGTH];
char *name;
int m,x,scale;
unsigned int base,index;
char *t,*rand,*fptr;
char sign;

	if (p->idus == 0) {
		return; /* no need to fix anything */
	}  /* if */

	if (   !isreg(p->op1)
	    && ! ISFREG(p->op1)
	    && !isconst(p->op1)) {
		m = 1;

	} else if (   !isreg(p->op2)
	    && ! ISFREG(p->op2)) {
		m = 2;
	} else {
		m = 3;
	}

	if (!p->ops[m]) return; /* no real index, ops[2] in null. */
	t = strchr(p->ops[m],'(');
	if (!t) return;	/*indexing only implicitely*/
	base = setreg(1+t); /*base register*/
	t = strchr(t,',');
	if (t)
		index = setreg(t+1); /*index register*/
	else
		index = 0;
	fptr = (*p->ops[m] == '*') ? "*" : ""; /*  function pointer call */
	name = (strlen(p->ops[m]) < ((unsigned ) MAX_LABEL_LENGTH)) ? 
			                       name_buf : getspace(strlen(p->ops[m])); 
	(void) decompose(p->ops[m],&x,name,&scale); /*rest components*/
	for (reg = 0; reg < NREGS; reg++) {
		if (base == regmasx[reg]) {
			change += p->extra - depths[reg]; /*zero if they are equal*/
		}  /* if */
		if (index == regmasx[reg]) {
			change += (p->extra2 - depths[reg]) * scale;
		}  /* if */
	}  /*for all regs*/
	if (change) {
		rand = getspace(strlen(p->ops[m]) + 12);
		change +=x;
		t = strchr(p->ops[m],'(');
		if (name[0]) {
			if (!fixed_stack) {
				if (change > 0) {
					sign = '+';
				} else { 
					sign = '-';
					change = -change;
				}  /* if */
			} else 
				sign = '+' ;
			if (   fixed_stack
			    && strstr(name,".FSZ")) 
				sprintf(rand,"%s%d%c%s%s",fptr,change,sign,name,t);
			else
				sprintf(rand,"%s%s%c%d%s",fptr,name,sign,change,t);
		} else {
			sprintf(rand,"%s%d%s",fptr,change,t);
			if (   (change < 0)
			    && (p->idus == ESP)) {
				p->ops[m] = rand;
				fatal(__FILE__,__LINE__,"got a negative esp offset\n");
			}  /* if */
		}  /* if */
		p->ops[m] = rand;
	}  /*endif change*/
}  /*end fix_offsets*/

/* manage space allocation for tdependent structures.                */

static void
init_alloc()
{
	if (! a0.a)
		if ((a0.a = (tdependent *) calloc(ASIZE,sizeof(tdependent))) ==NULL)
			fatal(__FILE__,__LINE__,"sched: cant malloc\n");
}  /*end init_alloc*/


static tdependent *
next_dep_str()
{
	if (alloc_index < ASIZE) {
		alloc_index++;
		return &(acur->a[alloc_index-1]);

	} else {
		if (acur->next == NULL) {
			acur->next =  tmalloc(alist);
			if ((acur->next->a = (tdependent *)malloc(ASIZE*sizeof(tdependent))) == NULL)
				fatal(__FILE__,__LINE__,"sched: cant malloc\n");
			acur->next->next = NULL;
		}  /* if */
		acur = acur->next;
		alloc_index = 1;
		return &(acur->a[0]);
	}  /* if */
}  /*end next_dep_str*/

/* get cycle count of integer instruction */
int
get_exe_time(p) NODE *p;
{
int time,flags;
	flags = CYCLES(p->op);
	time =  flags & 0xff;
	if (target_cpu == P5) {
		if (ismem(p->op1))
			time += (flags & M1) >> 8;
		else if (ismem(p->op2))
			time += (flags & M2) >> 12;
	} else {
		if (ismem(p->op1)) {
			time += (flags & M1) >> 8;
			if ( strchr(p->op1,',') )
				time++; /* Extra cycle for index in i486 */
			if (   p->op2
			    && *p->op2 == '$'
			    && *p->op1 != '(' ) {
				time++;	 /* Displacement and immediate add one cycle */  
			}  /* if */
		}
		else if (ismem(p->op2)) {
			time += (flags & M2) >> 12;
			if ( strchr(p->op1,',') )
				time++; /* Extra cycle for index in i486 */
			if (   *p->op1 == '$'
			    && *p->op2 != '(') {
				time++;	 /* Displacement and immediate add one cycle */  
			}  /* if */
		}  /* if */
	}  /* if */
	return time;
}  /*end get_exe_time*/

/* set register use bits */
unsigned int
muses(p) NODE *p;
{

unsigned using;
unsigned int op;

	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;
	if (target_cpu == P5) {
		if (   float_used
		    && FP(op)) {
			return fmuses(p);
		}  /* if */
	}  /* if */
	using = uses_but_not_indexing(p);
	switch (op) {

	case MOVSBL:
	case MOVSBW:
	case MOVSWL:
	case MOVZBL:
	case MOVZBW:
	case MOVZWL:
	case MOVL:
	case MOVW:
	case MOVB:
		using |= ismem(p->op1);
		break;

	/* special case for sched: lea has () but doesnt use memory */
	case LEAL:
	case LEAW:
	case FSTCW:
	case FST:
	case FSTP:
	case FSTS:
	case FSTL:
	case FSTPS:
	case FSTPL:
	case FSTPT:
	case FIST:
	case FISTL:
	case FISTP:
	case FISTPL:
	case FISTPLL:
	case FBSTP:
	case FSAVE:
	case FNSAVE:
	case FSTENV:
	case FNSTSW:
		break;
	case POPL:
	case POPW:
	case POPA:
	case LCALL:
	case CALL:
	case SCMPL:
	case SCMPW:
	case SCMPB:
	case SMOVL:
	case SMOVW:
	case SMOVB:
	case SCAB:
	case SCAW:
	case SCAL:
	case SLODB:
	case SLODW:
	case SLODL:
	case SCASB:
	case SCASL:
	case SCASW:
	case SSCAB:
	case SSCAL:
	case SSCAW:
		using |= MEM;
		break;
	default:
		using |= (ismem(p->op1) | ismem(p->op2));
	}
	return(using);
}  /*end muses*/

/* set register destination bits */
unsigned int
msets(p) NODE *p;
{

int setting;
char * dst();
char *cp;
unsigned int op;
	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;
  	if (target_cpu == P5) {
		if (   float_used
		    && FP(op)) {
			return fmsets(p);
		}  /* if */
	}  /* if */

	setting = p->sets;

	switch (op) {
	case XCHGL:
	case XCHGW:
	case XCHGB:
		return( setting | ismem(p->op1) | ismem(p->op2) );

	case SMOVL:
	case SMOVW:
	case SMOVB:
	case SSTOB:
	case SSTOW:
	case SSTOL:
	case STOSB:
	case STOSL:
	case STOSW:
		return(setting | MEM);

	case CALL:
	case LCALL:
		return(setting |  MEM ) ;

	case PUSHW:
	case PUSHL:
	case PUSHA:
		return(setting |  MEM );

	default:
		cp = dst(p);
		return( setting | ismem(cp) );
	}  /* switch */
}  /*end msets*/


	unsigned
indexing(p) NODE * p;
{
unsigned index = 0;         /* to be the result */
char *cp;
	if (   p->op1 != NULL
	    && (cp = strchr(p->op1,'(')) != NULL) {
		index = scanreg(cp,false);
	} else if (   p->op2 != NULL
		   && (cp = strchr(p->op2,'(')) != NULL) {
		index = scanreg(cp,false);
	} else if (   p->op3 != NULL
		   && (cp = strchr(p->op3,'(')) != NULL) {
		index = scanreg(cp,false);
	}  /* if */
	index &= ~MEM; 
	/* opcodes that use special registers as pointers to memory */
	switch (p->op) {
	case PUSHL:
	case PUSHW:
	case POPL:
	case POPW:
	case PUSHA:
		index |= ESP;
		break;

/* This is not an index for AGI
	case SCMPL:
	case SCMPW:
	case SCMPB:
	case SMOVL:
	case SMOVW:
	case SMOVB:
		index |= (ESI | EDI);
		break;
*/

	default: break;

	}  /*end switch*/
	return index;
}  /*end indexing*/

/* Find base register in p. If op has the form 
	(reg1,reg2) where reg1 != reg2,
	reg1 will not be a base because postpeep can exchange
	reg1 with reg 2 if reg1 will couse AGI. 
*/
static	unsigned
base(p) NODE * p;
{
unsigned base = 0;         /* to be the result */
char *cp,*cp1;
int i;
char reg_buff[12];

	if (   (   p->op1 != NULL
		&& (cp = strchr(p->op1,'(')) != NULL)
	    || (   p->op2 != NULL
		&& (cp = strchr(p->op2,'(')) != NULL)) {
		for(i = 0, cp1=cp;*cp1;cp1++) {
		    /*check if may exchange base with index */
			if (*cp1 == ',')
				i++;
		}  /* for */
		if (i != 1) { /* if not 2 regs without scale */
			for(i =0 ; cp[i] && (cp[i] != ',')  ; i++)
				reg_buff[i] = cp[i];
			reg_buff[i] = '\0'; /* copy base only */
    			base = scanreg(reg_buff,false);
		}  /* if */
	}  /* if */

	/* opcodes that use special registers as pointers to memory */
  	switch (p->op) {
    	case PUSHL:
	case PUSHW:
	case POPL:
	case POPW:
	case PUSHA:
		base |= ESP;
		break;

/* This is not an index for AGI
	case SCMPL:
	case SCMPW:
	case SCMPB:
	case SMOVL:
	case SMOVW:
	case SMOVB:
		base |= (ESI | EDI);
		break;
*/
	default: break;

	}  /*end switch*/
	return base;
}  /*end base*/

static int
no_gain_in_schedule()
{
NODE *p;
unsigned int setsp;
	for(p = b->firstn; p != b->lastn->forw; p = p->forw) {
		p->usage = NONE;
		setsp = p->sets;
		if (   FP(p->op)
		    && CONCURRENT(p->op)) {
			return false;

		} else if (   ! ispp(p)
			   && (setsp & base(p->forw)))
			return false;
		if (   (p->sets & p->forw->uses & ~(MEM | CONCODES_AND_CARRY))
						/* sets and uses same reg */
		    && ! (p->sets & R16MSB) /* less then full reg set */
		    && (p->forw->uses & R16MSB)) /* full reg uses */ {
			return false;
		}  /* if */
	}  /*for loop*/
	return true;
}  /*end no_gain_in_schedule*/

/* Some Floating point scheduling code */

/* find_big_block_end(b) Return b if it was not bracked because of size or 
**	the block with last part of the original basic block before breaking it.
*/ 
static BLOCK *
find_big_block_end(b) BLOCK *b; {
	for (;b != NULL ; b = b->next) {	
		if (   isbr(b->lastn)
		    || islabel(b->lastn)
		    || b->lastn->op == CALL
		    || isbr(b->lastn->forw)
		    || islabel(b->lastn->forw)
		    || b->lastn->forw->op == CALL
		    || b->lastn->op == ASMS
		    || is_safe_asm(b->lastn)
		    || b->next == NULL) {
			return b;
		}  /* if */
	}  /* for */
	/* NOTREACHED */
}  /* find_big_block_end */


/*
 b->marked bits are used for : 
	3  - 0  : entry. Num. of regs in entry point used by this basic block
	7  - 4  : use.   Max number of registers used in the basic block 
	11 - 8  : exit.  Num of regs used in the basic block and not popped.
	15 - 12 : status clear or processed
*/

#define PROCESSED 0x7000
#define is_clear(b)	((b->marked & 0xf000) == 0)

/*  Check on the FP stack usage within a "big" block.  Depending
    on how the flow grapg was build CALL's may appear within a block.
    Since such a call may return a FP value on the stack we must
    allow for the additional FP stack operand. */
static void
check_fp_stack(b,last) BLOCK *b; NODE *last;
{
NODE * p;
int entry, use, cur_stack, max_use;
boolean open_call = 0;
		
	max_use = entry = cur_stack = ENTRY(b); 
	for (p=b->firstn; p != last->forw ; p = p->forw) {

		if (p->op == CALL) {
			open_call = true;
		} else
		if (   FP(p->op)   /* if FP instruction */	
		    && ! FNOST(p->op)  /* if uses FP stack */ ) {

			use = 0;
			/* Check of op1 or op2 is of the form %st(i).  If
			   so, record the register number. */
			if (   isreg(p->op1)
			    && p->op1[1] == 's'
			    && p->op1[3] == '(') {
				use = p->op1[4] - '0'; /* Find the deepest reg used by op1 */
			} else
			if (   isreg(p->op2)
			    && p->op2[1] == 's'
			    && p->op2[3] == '(') {
				use = p->op2[4] - '0'; /* Find the deepest reg used by op2 */
			}  /* if */

			/* Check if this instruction makes implicit use
			   of %st(1) and no deeper register usage - OR -
			   Is this on FP instruction that makes implicit
			   use of %st(0) and %st(1). */
			if (   (   FST1_SRC(p->op)
					&& (use < 1))
			    || (   (p->op1 == NULL)
					&& (FNOARG(p->op)))) {
				use = 1;
			}  /* if */

			/* if push or FST[P] where the use is >= the current
			   stack location, add one to current stack location */
			if (   FPUSH(p->op)
			    || (   (   p->op == FSTP
				    || p->op == FST)
				&& use >= cur_stack)) {
				cur_stack++;
				if ( cur_stack > max_use)
					max_use = cur_stack;
			}  /* if */

			/* Was it the deepest ever in this block? */
			if (   cur_stack <= use
			    && !open_call) {
			    /* There must have been some entries on the
			       FP stack already when we entered this block
			       that we where not aware of. */
				entry += 1 + use - cur_stack;
				cur_stack = use + 1;
			}  /* if */
			if ( use > max_use) /* set max use */
				max_use = use;

			/* Now handle POP semantics. */
			if (   FPOP(p)
			    && !(p->op == FSTP && use > cur_stack)) {
				if (cur_stack)
					cur_stack--;
				else if (open_call)					open_call = false;
				else {
					/* Must have been more on the stack
					   than originally thought. */
					entry++;
				}  /* if */
			}  /* if */
			/* Check for POP twice instructions. */
			if (   (p->op == FCOMPP)
			    || (p->op == FUCOMPP)) {
				if (cur_stack >= 2)
					cur_stack -= 2;
				else if (open_call && cur_stack) {
					cur_stack = 0;
					open_call = false;
				} else {
					cur_stack = 0;
					open_call = false;
					entry++;
				}  /* if */
			}  /* if */
		}  /*endif FP*/
	}  /* end for the big block */
	/* Check that values are within the number of FP stack registers. */
	if (   entry < 0     || entry > 8   
	    || max_use < 0   || max_use > 8
	    || cur_stack < 0 || cur_stack > 8) {
		fprinst(b->firstn);
		fatal(__FILE__,__LINE__,
			  "calc out of bound, entry %d max %d exit %d\n",
			  entry,max_use,cur_stack);
	}  /* if */

	/* Marked as processed amd save the current register usage values. */
	b->marked = 0xf000 | entry | (max_use << 4) | (cur_stack << 8);
}  /* check_fp_stack */

void
check_fp()
{
BLOCK *b,*b1;

	set_refs_to_blocks(); /* connect switch tables nanes to blocks */
	for (b = b0.next; b ; b = b->next)
		b->marked = 0;
	for (b = b0.next; b ; ) {
		if (is_clear(b)) {
			b1 = find_big_block_end(b);
			check_fp_block(b,b1->lastn);
			b = b1->next;
		} else b = b->next;
	}  /* for */
}  /* check_fp */

static void
check_fp_block(b,last) BLOCK *b; NODE *last;
{
BLOCK *b1,*b2;
REF *r;
SWITCH_TBL *sw,*get_base_label();
	b->marked |= PROCESSED;
	for (b1 = b; b1->lastn != last; b1 = b1->next)
		b1->marked |= PROCESSED; /* mark all block in the big block */
	check_fp_stack(b,last);
	if (b->nextl) {
		if (   (is_clear(b->nextl))
		    || (EXIT(b) != ENTRY(b->nextl))) {
			set_entry(b->nextl,EXIT(b));
			b1 = find_big_block_end(b->nextl);
			check_fp_block(b->nextl,b1->lastn);
		}  /* if */
	}
	if (b->nextr) {
		if (   (is_clear(b->nextr))
		    || (EXIT(b) != ENTRY(b->nextr))) {
			set_entry(b->nextr,EXIT(b));
			b1 = find_big_block_end(b->nextr);
			check_fp_block(b->nextr,b1->lastn);
		}  /* if */
	}  /* if */
	if (is_jmp_ind(b->lastn)) {
		sw = get_base_label(b->lastn->op1);
		for (r = sw->first_ref; r; r = r->nextref) {
			if (sw->switch_table_name == r->switch_table_name) {
				b1 = r->switch_entry;
				if (   (is_clear(b1))
				    || (EXIT(b) != ENTRY(b1))) {
					set_entry(b1,EXIT(b));
					b2 = find_big_block_end(b1);
					check_fp_block(b1,b2->lastn);
				}  /* if */
			}  /* if */
			if ( r == sw->last_ref )
				break;
		}  /* for */
	}  /* if */
}  /* check_fp_base */

static BLOCK *last_block = NULL; /* The last block that got virtual FP regs */

static int fp_stack[8] = { 0,0,0,0,0,0,0,0}; /* Stack used by set_fp_regs */
static int tos = -1;  /* Pointer to the input top of stack */
              
static int fp_free[8] = { 0,1,2,3,4,5,6,7};  /* free registers pool */
static int free_regs = 8;      /* index in fp_free */
static int input_stack[8] = { 0,0,0,0,0,0,0,0}; /* block entry point regs */
static int input_stack_index = 0; /* index in input_stack */
static int output_stack[2] = { 0, 0};       /* regs in the block exit point */
static int code_tos = -1;  /* top of stack in the output code stack */ 
static int code_fp_stack[8];  /* output code stack */
static char *fp_str[8] = {"FF#0","FF#1","FF#2","FF#3",
                   "FF#4","FF#5","FF#6","FF#7"}; /* virtual registers names */
static char *fkill_str[8] = {"FK#0","FK#1","FK#2","FK#3",
                   "FK#4","FK#5","FK#6","FK#7"}; /*Marker for register kill */
static char *fnew_str[8] = {"FN#0","FN#1","FN#2","FN#3", /*Marker for new register */
                   "FN#4","FN#5","FN#6","FN#7"};




/* initialize data for FP register transformation */ 
static void
init_fregs(b) BLOCK *b;
{
	int i;
	tos = -1;

	input_stack_index = 0;
	/*free_regs = 8 - ((b->marked >>4) & 0xf);*/
	free_regs = 8 - ((b->marked ) & 0xf);
	if (   ((b->marked & 0xf0) != 0x80)
	    && (b->lastn->op != CALL))
		free_regs--; /* We can have up to one register error in case block
** after call. So use only 7 regs if in the original block less the 8 were used
   and there was no call in this block */ 
	for ( i = 7; i >=0 ; i--)
	{	fp_free[i] = i; /* Get different free reg every time */
		fp_stack[i] = 0;
	}  /* for */
}  /* init_fregs */
	
/* get one free reg */
static char *
getfreg(p) NODE * p;
{
	int reg,i;

	if (tos > 7) /* can't have more then 8 FP regs */
		fatal(__FILE__,__LINE__,"no reg to get\n");
	reg = fp_free[0]; /* get the free reg */
	p->op3 = fnew_str[reg];
	free_regs--;
	for ( i = 0; i < free_regs ; i++) /* rotate the free regs by one */
		fp_free[i] = fp_free[i+1];
	fp_stack[++tos] = reg; /* add this reg to the FP stack */
	return fp_str[reg]; /* return the new reg */
}  /* getfreg */

/* If reg to the bottom of the stack if used without push */
static void 
addfreg()
{
	int reg,i;

	if (tos  > 7) /* can't have more then 8 FP regs */
		fatal(__FILE__,__LINE__,"no reg to get\n");
	reg = fp_free[0];
	free_regs--;
	for ( i = 0; i < free_regs ; i++)
		fp_free[i] = fp_free[i+1];
	for (i = tos +1 ; i ; i--) /* Move all the stack by one up */
		fp_stack[i] = fp_stack[i-1];
	fp_stack[0] = reg;
	tos++;
}  /* addfreg */

/* pop fp reg and free the stack */
static void 
freefreg(p) NODE * p;
{

	if (tos < 0) /* can't free reg if not on the stack */
		fatal(__FILE__,__LINE__,"no reg to free\n");
	p->op3 = fkill_str[fp_stack[tos]];
	fp_free[free_regs++] = fp_stack[tos--]; /* Pop it and add as last free */
}  /* freefreg */

static int last_st_num; /* mark the number of the last st_num found by st_num */

/* convert %st(?) reg to FF#?  */   
static char *
st_num(cp) char *cp;
{
int offset;
	last_st_num = -1;
	if (cp == NULL)         
		return cp;  
	if (   cp[0] == '%'
	    && cp[1] == 's'
	    && cp[2] == 't') { /* if %st[(?)] */   
		last_st_num = offset =  (cp[3] == '(') ? (cp[4] -'0') : 0;
		if ( offset <= tos) /* the reg is on the stack */
			return fp_str[fp_stack[tos - offset]];
		while ( offset > tos) { /*if not pushed it yet, add it to the bottom */
			addfreg();
			input_stack[input_stack_index++] = fp_stack[0];
		}  /* while */
		return fp_str[fp_stack[0]];
	}  /* if */
	return cp;
}  /* st_num */

static void
break_fi(b,p) BLOCK *b; NODE*p;  /* split FI?? to FILD and F?? */
{
int newop;
char *newopcode;
NODE *pn;
	switch (p->op) {
		case	FIADD:
			newop = FADD; newopcode = "fadd";
			break;
		case	FIADDL:
			newop = FADD; newopcode = "fadd";
			break;
		case	FICOM:
			newop = FCOM; newopcode = "fcom";
			break;
		case	FICOML:
			newop = FCOM; newopcode = "fcom";
			break;
		case	FIDIV:
			newop = FDIVR; newopcode = "fdivr";
			break;
		case	FIDIVL:
			newop = FDIVR; newopcode = "fdivr";
			break;
		case	FIDIVR:
			newop = FDIV; newopcode = "fdiv";
			break;
		case	FIDIVRL:
			newop = FDIV; newopcode = "fdiv";
			break;
		case	FIMUL:
			newop = FMUL; newopcode = "fmul";
			break;
		case	FIMULL:
			newop = FMUL; newopcode = "fmul";
			break;
		case	FISUB:
			newop = FSUBR; newopcode = "fsubr";
			break;
		case	FISUBL:
			newop = FSUBR; newopcode = "fsubr";
			break;
		case	FISUBR:
			newop = FSUB; newopcode = "fsub";
			break;
		case	FISUBRL:
			newop = FSUB; newopcode = "fsub";
			break;
		default:
			return; /*dont change other opcodes*/
	}  /*end switch*/
	pn = insert(p);
	if (OpLength(p) == WoRD)
		chgop(p,FILD,"fild");
	else if (OpLength(p) == LoNG)
		chgop(p,FILDL,"fildl");
	else {
		fatal(__FILE__,__LINE__,"Unexpected op length\n");
	}  /* if */
	chgop(pn,newop,newopcode);
	pn->op1 = pn->op2 = NULL;
	pn->sets = sets(pn);
	pn->uses = uses_but_not_indexing(pn);
	pn->idxs = indexing(pn);
	b->length++;
	if (p == b->lastn)
		b->lastn = pn;
	if (newop == FCOM) {
		pn = insert(p);
		chgop(p,FXCH,"fxch");
		p->op1 = p->op2 = NULL;
		new_sets_uses(pn);
		b->length++;
	}  /*endif FCOM*/
}  /*end break_fi*/

/* This is the main entry point for register converting from stack to virtual
** it will check if the we do not have more then 2 registers in the exit point
** of the block. Then if we are not using unexpected floating point 
** instructions.
** Then it will emulate the fp stack and convert the operands from stack to 
** vertual registers block. For example:
**  "flds a(%eax)" will be "flds a(%eax),FF#0"
*/
/* replace fp stack with registers */
static void
set_fp_regs(b) BLOCK * b;
{
	NODE * p, * last_p;
	BLOCK *last_b;
	int reg_num1,tmp,i;
	int use;

	last_b = find_big_block_end(b);
	if (last_b == last_block) /* If we did this block already */
		return;
	last_block = last_b;
	last_p = last_block->lastn->forw;
	if ((b->marked & 0xf00) > 0x200) /* Can't do FP with more then 2 regs */
		return;                      /* in the end of basic block.        */

	/*for all instructions in the block from the top to the bottom */
	for (p=b->firstn; p !=last_p; p = p->forw) {	
		if (   FP(p->op)
		    && FUNEXPECTED(p->op)) {
			return; /* using unexpected instructions */
		}  /* if */
	}  /* for */
	init_fregs(b);
	for (p=b->firstn; p !=last_p; p = p->forw) {
		if (p == b->lastn->forw)
    			b = b->next; /* Must know the current block to decrement length */
		if (FP(p->op)) { /* if it is floating point instruction */
			float_used = 1;
			if (   isreg(p->op1)
			    && p->op1[3] == '(') {
				use = p->op1[4] - '0';

			} else if (   isreg(p->op2)
				   && p->op2[3] == '(') {
				use = p->op2[4] - '0';
			} else
				use = 0;
			if (   free_regs
			    && p->opcode[1] == 'i') {
				break_fi(b,p);  /* split FI?? to FILD and F?? */
			}  /* if */
			if (   p->op == FXCH
			    && (p->op1 == NULL))  {
				p->op1 = "%st(1)";
			}  /* if */
			if (   FNOARG(p->op)
			    && (p->op1 == NULL)) {
				if (   p->op == FCOM
				    || p->op == FCOMP
				    || p->op == FUCOM
				    || p->op == FUCOMP) {
				    p->op1 =  st_num("%st(1)");
				 } else {
					switch(p->op) { /* "F{OP}" == "F{OP}P %st,%st(1)"   */
					case FADD: 
						p->op = FADDP;
						p->opcode = "faddp";
						break;
					case FDIV:
						p->op = FDIVP;
						p->opcode = "fdivp";
						break;
					case FDIVR:
						p->op = FDIVRP;
						p->opcode = "fdivrp";
						break;
					case FMUL:
						p->op = FMULP;
						p->opcode = "fmulp";
						break;
					case FSUB:
						p->op = FSUBP;
						p->opcode = "fsubp";
						break;
					case FSUBR:
						p->op = FSUBRP;
						p->opcode = "fsubrp";
						break;
					default:
						break;
					}  /* switch */
					p->op1 =  st_num("%st");
					p->op2 =  st_num("%st(1)");
				}  /* if */
			}  /* if */
			p->op1 = st_num(p->op1); /* if %st(?) used convert to FF#x */
			reg_num1 = tos - last_st_num;
			p->op2 = st_num(p->op2);
			if (p->op == FXCH) { /*Change regs on the stack and delete fxch */
				tmp = fp_stack[reg_num1];
				fp_stack[reg_num1] = fp_stack[tos];
				fp_stack[tos] = tmp;
				b->length--;
				if (b->firstn == p)
					b->firstn = p->forw;
				if (b->lastn == p)
					b->lastn = p->back;
				DELNODE(p);
				continue;
			}  /* if */
			if (   p->op == FCOMPP     /* bug fix: p->op1 is " " and not NULL */
				|| p->op == FUCOMPP)
				p->op1 = NULL;
			if (   FST1_SRC(p->op)
			    && (p->op1 == NULL)) { /* only FCOMPP  and FUCOMPP */
				p->op1 =  st_num("%st(1)");
			}  /* if */
			if (FST_SRC(p->op)) { /* if source is %st(0) */ 
				p->op2 = p->op1;
				p->op1 =  st_num("%st");
			} else if (FST_DEST(p->op)) /* if destination is %st(0) */
				p->op2 = st_num("%st"); 
			if (   FPUSH(p->op)
			    || (   p->op == FSTP
				&& use > tos)) {
				if (p->op1 == NULL)
					p->op1 = getfreg(p);
				else
					p->op2 = getfreg(p);

			} else if (   FPOP(p)
				 && !(   p->op == FSTP
				      && use > tos)) {
				freefreg(p);

			} else if (   p->op == FCOMPP
					   || p->op == FUCOMPP) { /* FCOMPP, special case: pop twice */
				freefreg(p);
				freefreg(p);
			}  /* if */
		}  /* if */
	}  /* for */
	if (tos == 1) { /* Mark the output stack reset regs may add fxch */	
		output_stack[0] = fp_stack[0];
		output_stack[1] = fp_stack[1];
	} else if (tos > 1) /* no set_fp_regs with more then 2 exit regs */
		fatal(__FILE__,__LINE__,"set fp stack internal error \n");
	for (i = 0; i < input_stack_index; i++)  /* copy the input to code stacks */
		code_fp_stack[++code_tos] = input_stack[input_stack_index - (1 +i)];
	input_stack_index = 0;
    
}  /* set_fp_regs */

/* service functions for virtual to stack conversion */ 

/* convert FF#x to x */ 
static int
str2reg(reg_string) char *reg_string;
{

	if (   reg_string[0] == 'F'
	    && reg_string[1] == 'F'
	    && reg_string[2] == '#')
		return (reg_string[3] - '0');

	fatal(__FILE__,__LINE__,"sched: get_reg with %s and not reg\n",reg_string);
	return 0;
}  /* str2reg */

static char *st_str[8] = {"%st","%st(1)","%st(2)","%st(3)",
                          "%st(4)","%st(5)","%st(6)","%st(7)"};
/* convert FF#x to %st(?) */   
static char *
replace_freg(reg_string) char *reg_string;
{
int reg,i;
	if (   reg_string[0] == 'F'
	    && reg_string[1] == 'F'
	    && reg_string[2] == '#') {
		reg = reg_string[3] - '0';
		for (i = code_tos  ; 0 <= i ; i--) {   /* find the reg in the stack */
			if ( code_fp_stack[i] == reg)
				return st_str[code_tos -i]; /* return it's string */
		}  /* for */

		fatal(__FILE__,__LINE__,"reg %d not found in stack\n",reg);
	}  /* if */
	return reg_string; /* return the string if FF reg was not found */
}  /* replace_freg */

/* Add fxch if needed */
static void 
fxch(reg,last_float) int reg; NODE *last_float;
{
int i;
NODE *new;
	if (reg == code_fp_stack[code_tos]) /* check if reg is O.K. */
		return; /* fxch is not needed */
	for (i = code_tos - 1 ; 0 <= i ; i--) { /* find the reg in the stack */
		if ( code_fp_stack[i] == reg)  { /* if reg found */ 
			code_fp_stack[i] = code_fp_stack[code_tos];
			code_fp_stack[code_tos] = reg;  /* replace reg on stack */
			new = insert(last_float); /* add fxch after last scheduled FP */
			new->op = FXCH;
			new->opcode = "fxch\0";
			new->op1 = st_str[code_tos - i];
			new->op2 = new->op3 = NULL;
#ifdef DEBUG
			if (vflag) {
				new->op4 = "\t/ added\0";
			}  /* if */
#endif
			new->dependent=new->anti_dep=new->agi_dep =new->may_dep = NULL;
			return;
		}  /* for */
	}  /* for */
}  /* fxch */

/* Some cleanup in the end of big basic block */
static void
clean_fp_regs(b) BLOCK * b;
{
	if (b != last_block) /* If not the end of BIG basic block */
		return; 
	if (   code_tos == 1
	    && output_stack[1] != code_fp_stack[1]) 
		fxch(output_stack[1],last_float); 
	code_tos = -1;
	float_used= 0;
	last_block = NULL;
}  /* clean_fp_regs */

/* check if instruction p will add fxch */    
static int
check_for_fxch(p) NODE *p;
{
	if (! float_used)
		return false;
	if (   p->op3 != NULL
	    && p->op3[1] == 'N') /* new reg */ {
		return false; /* push will never add fxch */
	}  /* if */
	if (   p->op == FCOMPP
		|| p->op == FUCOMPP) {
		if (code_fp_stack[code_tos] != str2reg(p->op1))
			return true;
		return code_fp_stack[code_tos -1] != str2reg(p->op2);

	} else if (   p->op3 != NULL
		   && p->op3[1] == 'K') /* kill reg */ {
		return code_fp_stack[code_tos] != p->op3[3] - '0'; /* pop from tos */
	} else if  (FST_SRC(p->op)) 
	 	return code_fp_stack[code_tos] != str2reg(p->op1); /* src is tos */
	else if (FST_DEST(p->op)) /* dest is tos */ {
		if (p->op2 == NULL) /* if dest is op2 */
        		return code_fp_stack[code_tos] != str2reg(p->op1);
        	else
        		return code_fp_stack[code_tos] != str2reg(p->op2);
	}  /* if */
    return false;
}  /* check_for_fxch */

/* Convert back the virtual registers to stack */
static void
reset_fp_reg(p) NODE * p;
{
int reg;
NODE *new = NULL;

  	fp_set_resource(p); /* Mark resources for the scheduler */
	if (! float_used)
    		return;
	if (   p->op3 != NULL
	    && (p->op3[1] == 'N')) { /* new reg Do push */
		if (p->op2 == NULL) {    /* FLDZ,FLD1 etc. */
			code_fp_stack[++code_tos] = str2reg(p->op1);
			p->op1 = NULL;
		} else {
			p->op1 = replace_freg(p->op1);
			code_fp_stack[++code_tos] = str2reg(p->op2);
		}  /* if */
		p->op3 = p->op2 = NULL;
	}  /* if */
	if (   p->op == FCOMPP
		|| p->op == FUCOMPP) { /* Do pop twice */
		reg = str2reg(p->op1);
		fxch(reg,last_float);
		reg =str2reg(p->op2);
		if (reg != code_fp_stack[code_tos - 1]) { /* if PO2 != %st(1) */
			/* do: f[u]comp %st(?); fxch %st(?) ; fstp %st(0) */
			if (p->op == FCOMPP ) {
				p->op = FCOMP;
				p->opcode = "fcomp";
			} else {
				p->op = FUCOMP;
				p->opcode = "fucomp";
			}  /* if */
			p->op1 = replace_freg(p->op2);
			p->op2 = NULL;
			new = insert(p);
			new->op = FSTP;
			new->opcode = "fstp";
			new->op1 = "%st(0)";
			new->op2 = new->op3 = NULL;
			new->op4 = "\t/ added";
			new->dependent=new->anti_dep=new->agi_dep = NULL;
			reorder(new);
			last_float = p;
		} else
			p->op1 = p->op2 = NULL;
		code_tos -= 1;
	} else if  (FST_SRC(p->op)) { /* set %st as op1 */
		reg =  str2reg(p->op1);
		fxch(reg,last_float);
		p->op1 = p->op2;
		p->op2 = NULL;
		if (   p->op == FSTP
		    && ISFREG(p->op1)
		    && reg == str2reg(p->op1)) {
			p->op1 = "%st(0)"; /* fstp must have %st(0) and not %st */
		}  /* if */
	} else if (FST_DEST(p->op)) { /* set destination as %st */
		if (p->op2 == NULL) {
			reg =  str2reg(p->op1);
			p->op1 = NULL;
		} else {
			reg =  str2reg(p->op2);
			p->op2 = NULL;
		}  /* if */
		
		fxch(reg,last_float);
	}  /* if */
	if (   p->op3 != NULL
	    && p->op3[1] == 'K')  { /* kill reg from top of stack */
		reg = p->op3[3] - '0';
		fxch(reg,last_float);
	} else 
	if (    ISFREG(p->op2) 
	    && ((p->op2[3] - '0') != code_fp_stack[code_tos]
	    || (p->op1[3] - '0') != code_fp_stack[code_tos])) {
		 /* for "F* %st(i),%st(j)"  i or j must be 0 */
		fxch(p->op2[3] - '0',last_float);
	}  /* if */
	if (p->op1 != NULL)
		p->op1 = replace_freg(p->op1);
	if (p->op2 != NULL)
		p->op2 = replace_freg(p->op2);
	if (   p->op3 != NULL
	    && p->op3[1] == 'K') { /* pop reg */
		code_tos--;
	}  /* if */
	last_float = (new != NULL) ? new : p;
	p->op3 = NULL;
	if (   (   FNOARG(p->op)
		|| p->op == FLD)
	    && strcmp(p->op1,"%st") == 0) {
		p->op1 = "%st(0)";	
	}  /* if */
	if (   FPOP(p)
	    && p->op2
	    && !strcmp(p->op2,"%st")) {
		p->op2 = "%st(0)";
	}  /* if */
}  /* reset_fp_reg */


/* set new FP register destination bits */
static unsigned int
fmsets(p) NODE *p;
{
char *cp;
unsigned int op;

	op = p->op > SAFE_ASM ? p->op - SAFE_ASM : p->op;
	if (   op == FSTSW
	    || op == FNSTSW) {
		return EAX;
	}  /* if */
	if (FDEST1(op))
		cp = p->op1;
	else if FDEST2(op)
		cp = p->op2;
	else 
		return 0;
	if (   cp != NULL
	    && cp[2]  != '#' ) {
		return  MEM;
	}  /* if */
	return 0;
}  /* fmsets */


/* set new FP register  use bits */
static unsigned int
fmuses(p) NODE *p;
{
	if (   p->op1
	    && FSRC1(p->op)
	    && p->op1[2] != '#') 
		return  MEM;
	if (   p->op2
	    && FSRC2(p->op)
	    && p->op2[2] != '#')  
		return  MEM;
	return 0;
}  /* fmuses */

static NODE *fp_regs[8];

/* add dependency caused by fp regs */
static void 
add_fp_dependency(b) BLOCK * b;
{
NODE *p,*p1 = NULL;
NODE *p_comp = NULL;
int i,reg,reg1;

	if (! float_used) {/*no virtual regs, every fp depend on it's predecessor*/
		for (p=b->firstn; p !=b->lastn->forw; p = p->forw) {
			if (   FP(p->op)
			    || (p->op == CALL)) {   
				if (p1 != NULL)
					install1(p1,p,DEP);
				p1 = p;
			}  /* if */
		}  /* for */
		return;
	}  /* if */
	for (i = 0 ; i< 8 ; i++)
		fp_regs[i] = NULL;   

    /* Forward dependency are from the type
    ** (1)	Fop1 FF#1,FF#2
    ** (2)  Fop2 FF#2,FF#3
    ** (2) is forward depend on (1) */
 
	for (p=b->firstn; p !=b->lastn->forw; p = p->forw) {/* forward dependency */
		if (FP(p->op)) {   
			if (FSETCC(p->op))
				p_comp = p;
			if (   (   p->op == FSTSW 
				|| p->op == FNSTSW)
			    && p_comp != NULL) { /* FSTSW depends on fcom */
				install1(p_comp,p,DEP);
			}  /* if */
			if (p->op == FLDCW) { /* Every fp depends on the control word */
				for (i = 0 ; i< 8 ; i++)
					fp_regs[i] = p;
			}  /* if */
			for (reg = -1,i = 1 ; i < 3 ; i++) { /* add register dependency */
				if (   (p->ops[i] != NULL)
				    && (p->ops[i][2] == '#')) {   

					reg1 = p->ops[i][3] - '0';
					if (reg == reg1) /* fadd F1,F1 will be linked only once */
						break;
					reg = reg1;
					if ((p1 = fp_regs[reg]) != NULL)
						install1(p1,p,DEP);
				}  /* if */
			}  /* for */
			if (   p->op3
			    && (   (p->op3[1] == 'N')
				|| (p->op3[1] == 'K'))) {
				fp_regs[p->op3[3] - '0'] = p; /* Add kill/new reg dependency */
			}  /* if */
			if (   p->op == FCOMPP		/* Kill without FK setting */
				|| p->op == FUCOMPP)
				fp_regs[p->op1[3] - '0'] = p;
			if (   FDEST1(p->op)
			    && (p->op1[2] == '#')) {
				fp_regs[p->op1[3] - '0'] = p;
			}  /* if */
			if (   FDEST2(p->op)
			    && (p->op2[2] == '#')) {
				fp_regs[p->op2[3] - '0'] = p;
			}  /* if */
		}
		else if (p->op == CALL) /* Don't schedule FP over calls */
 			for (i = 0 ; i< 8 ; i++)
				fp_regs[i] = p;
	}  /* for */
	for (i = 0 ; i< 8 ; i++)
		fp_regs[i] = NULL;
	p_comp = NULL;   
    /* Backward dependency can be from the type 
    ** (1)	Fop1 FF#1,FF#2
    ** (2)  Fop2 FF#3,FF#1
    ** (2) is backward  depend on (1) */
 
	for (p=b->lastn; p !=b->firstn->back; p = p->back) { 
		if (FP(p->op)) {   
			if (FSETCC(p->op))
				p_comp = p;
			if (   (   p->op == FSTSW
				|| p->op == FNSTSW)
			    && (p_comp != NULL)) { /*fcom depends on FSTSW */
				install1(p,p_comp,DEP);
			}  /* if */
			if (p->op == FLDCW) /* Every fp depends on the control word */
				for (i = 0 ; i< 8 ; i++)
					fp_regs[i] = p;
			for (reg = -1,i = 1 ; i < 3 ; i++) { /* add register dependency */
				if (   (p->ops[i] != NULL)
				    && (p->ops[i][2] == '#')) {   
					reg1 = p->ops[i][3] - '0';
					if (reg == reg1) /* fadd F1,F1 will be linked only once */
						break;
					reg = reg1;
					if ((p1 = fp_regs[reg]) != NULL)
						install1(p,p1,DEP);
				}  /* if */
			}  /* for */
			if (   p->op == FCOMPP		/* Kill without FK setting */
				|| p->op == FUCOMPP)
				fp_regs[p->op1[3] - '0'] = p;
			if (   p->op3
			    && (   (p->op3[1] == 'N')
				|| (p->op3[1] == 'K'))) {
				fp_regs[p->op3[3] - '0'] = p; /* Add kill/new reg dependency */
			}  /* if */
			if (   FDEST1(p->op)
			    && (p->op1[2] == '#')) {
				fp_regs[p->op1[3] - '0'] = p;
			}  /* if */
			if (   FDEST2(p->op)
			    && (p->op2[2] == '#')) {
				fp_regs[p->op2[3] - '0'] = p;
			}  /* if */
		}  /* if */
		else if (p->op == CALL)
			for (i = 0 ; i< 8 ; i++)
				fp_regs[i] = p;
	}  /* for */
}  /* add_fp_dependency */

static int resource_regs[11];
#define resource_fp resource_regs[8] 
#define resource_mul resource_regs[9] 
#define resource_com resource_regs[10] 
/* The clock ticks time cycles, set the resources */
static void 
fp_ticks(time) int time;
{
int i;
	if ( ! time) /* The clock did not tick */
		return;
	for (i = 10 ; i >= 0  ; i--) { /* decrement all resources */
		if (resource_regs[i] > time)
			resource_regs[i] -= time;
		else
			resource_regs[i] = 0;
	}  /* for */
}  /* fp_ticks */

/* How much time will we wait for this instruction ? */
int 
fp_time(p) NODE *p;
{
int time = 0;
	if (float_used) {
		if (   FSRC1(p->op)
		    && (p->op1[2] == '#')) /* time for reg to have it's data */
			time = resource_regs[p->op1[3] - '0'];
		if (   FSRC2(p->op)
		    && (p->op2[2] == '#'))
			time = MAX(resource_regs[p->op2[3] - '0'],time);
  
	} /* else Check for TOS (2 source registers one of them must be TOS */ 
	else if (target_cpu & (P5|blend)) {
		if (p->op == FXCH) return 0;
		else return resource_fp;
	} else if (   FST_SRC(p->op)
		   || (   FSRC1(p->op)
		       && FSRC2(p->op)))
    		time = resource_regs[0];
	if (   time
	    && (   p->op > FSTPT
		|| p->op < FST))
		time--; 

	if (resource_mul > time)
		time = resource_mul;

	if (resource_fp > time)
		time = resource_fp;

	if (   p->op == FSTSW
	    || p->op == FNSTSW) 
		time = MAX(resource_com,time);

	return time;
}  /* fp_time */


/* set resources used by this instruction */
static void 
fp_set_resource(p) NODE *p;
{
	if (target_cpu != P5) {
		resource_fp =  CONCURRENT(p->op);
		return;
	}  /* if */
   	if (   p->op >= FST
	    && p->op <= FSTPT)
	   fp_ticks(2+fp_time(p));

	else	
	   fp_ticks(1+fp_time(p));

   	if (   F_FLD(p->op)
	    || p->op == FABS)
		return;
	if (F_DIV(p->op))
		resource_fp = 11;
	else if (FSETCC(p->op))
		resource_com = 3;
	else if (F_MUL(p->op))
		resource_mul = 1; /* no back to back mul */
	if (float_used) {
		if (   FDEST1(p->op)
		    && (p->op1[2] == '#'))
			resource_regs[p->op1[3] - '0'] = 3;

		if (   FDEST2(p->op)
		    && (p->op2[2] == '#'))
			resource_regs[p->op2[3] - '0'] = 3;

	} else { /* Chech only for TOS setting.   */
		if (   FPOP(p)
		    || F_FLD(p->op)
		    || p->op == FXCH
		    || FSETCC(p->op)
		    || (   (p->op2)
			&& !strcmp(p->op2,"%st"))) 
			   resource_regs[0] = 0;
		else
			   resource_regs[0] = 3;
	}  /* if */
}  /* fp_set_resource */

/* reset all resources */
static void 
fp_start()
{
	int i;
	for (i = 10 ; i >= 0  ; i--)
		resource_regs[i] = 0;
}  /* fp_start */

/*************************************
 *      P6 section  
 * 
 *************************************/
#define N_resources_reg 18

enum { iEAX=0, iEBX, iECX, iEDX, iEDI, iESI, iEBP, iESP, 
	   iFP0, iFP1, iFP2, iFP3, iFP4, iFP5, iFP6, iFP7, iCARRY_FLAG,
       iCONCODES };
	   
unsigned regs[N_resources_reg]= 
	{ EAX, EBX, ECX, EDX, EDI, ESI, EBP, ESP, 
	  FP0, FP1, FP2, FP3, FP4, FP5, FP6, FP7, CARRY_FLAG,
      CONCODES };

#define ALL_PRT (PRT01|PRT0|PRT1|PRT2|PRT3|PRT4)

enum { iPRT01=0, iPRT0=1, iPRT1=2, iPRT2=3, iPRT3=4, iPRT4=5 }; 


const int      port_shift[6]= { 0, 4, 8, 12, 16, 20 };

#define uops_of(p) (p)->exec_info.rs_info.uops
#define port_of(p) (p)->exec_info.rs_info.ports
/*#define latency_of(p) (p)->exec_info.rs_info.latency*/
#define s_weight_of(p) (p)->exec_info.rs_info.weight

#define NOT_USED_IN_CLC 0
#define CLC_ADDR 0x01000000
#define CLC_JCC  0x02000000


static int wait_reg[N_resources_reg];
static int port_que[5];
static NODE *picked, *picked2;

/* used to sum number of 1-uop, 2-uops, etc instructions */
 
/* Typing weight info */
#ifdef DEBUG
#define comment4(p,n,st,dn)  if (vflag) sprintf((p)->ops[4]+(n-1)*6+3,st"%-3d",dn)
#else
#define comment4(p,n,st,dn) 
#endif 

void convertPRT01(), reset_static_data(), calc_static_weights();

void
mark_all_parents(p,rgs,n) NODE*p; unsigned rgs,n;/*mark value*/
{
NODE *q;
unsigned regs = rgs ? rgs : p->uses & ~MEM ;

	for (q = p->back; q != b->firstn->back; q = q->back) {
		if (q->sets & regs) {
			if (q->userdata & n) continue;
			q->userdata |= n;
			mark_all_parents(q,q->idus|q->idxs,n);
		}  /* if */
	}  /* for */
}  /* mark_all_parents */


/*Fills each NODE with static information ( i.e. information may be obtained
  before scheduling) :
  - No. of uops this instruction decomposes to.
  - Ports on which it can execute on.
  - Latency of instruction.
  - How many instructions in basic block are 1-uop, 2-uop, etc.
  - How many uops are executed on each port.
  - find and mark instructions which result is used in address or jcc 
    calculation
  Invocates calc_static_weights() to calculate static weight of each 
  instruction.
  Is invocated by bldag(); ROBERT it's really from prun!
  Sets port_sum[], uop_sum[], s_weight, userdata;
*/
/* This could be a good place to detemine if the current basic block is more
**decoder limited or more RS limited.
*/
void
static_analysis()
{
NODE *pi;
unsigned uia;

	/*memset(port_sum,0,sizeof(port_sum));*/
	/*memset(uops_sum,0,sizeof(uops_sum));*/
		
#if WE_WERE_DOING_DYNAMIC_JUST_FOR_FUN
	for(pi=b->lastn; pi != b->firstn->back; pi=pi->back) {
		/*for (uia=iPRT01; uia<=iPRT4; uia++)*/
			/*port_sum[uia] += (pi->exec_info.ports;*/
			
		/*uia = get_uops(pi);*/
		/*uops_sum[uia]++;*/
	}  /* for */
#endif
	/*ROBERT this has to do with register dependencies of mem access, but it 
	**does the same thing for loads and stores!!!
	*/
	for(pi=b->lastn; pi != b->firstn; pi=pi->back) {
		if (   pi->idus
		    && pi->op != LEAL) {
			uia = pi->idus | pi->idxs; 
			mark_all_parents(pi, uia, CLC_ADDR);
		}  /* if */
	}  /* for */
	/*this marks the insts that are on the path to the branch. once we have
	**Ball & Larus type heuristics for flacky branches, we will tune this.
	**for now, do not mark loop branches and unconditionals 
	**add a macro w/ a meaningful name for that.
	**This code finds the last instruction that sets the concodes and marks
	**it; next it marks all the parents of this instructions.
	**This also means that a previous instruction that sets the condition
	**code but otherwise is independent of the pivot will not be marked.
	*/
	if (   last_is_br
	    && iscbr(last_is_br)
	    && last_is_br->ebp_offset == 0) {
		for(pi=b->lastn;
			!(pi->sets & CONCODES_AND_CARRY) && pi!=b->firstn->back;
			pi=pi->back) {
			/* EMPTY */ ;
		}  /* for */
		pi->userdata |= CLC_JCC;
		mark_all_parents(pi, pi->idus|pi->idxs, CLC_JCC);
	}  /* if */

	calc_static_weights();
	reset_static_data();
}  /* static_analysis */

/* static weights */

#define MRW 0x040 /* load */
#define CAW 0x020 /* on the path of a store */
#define CJW 0x010 /* on the path of a jump */
#define NDW(n)  ( ((n>>4)/b->length) << 1 )  /* number of dependents. max=32 */ 
#define CLW(n)  ( ((n>>4)/b->length) << 2 )  /* chain length. max=64 */
/* dynamic weights */
#define DCD(l)  ((l)<<3)  /*uops*/
#define RGR(l)  ((l)<<2)  /*waiting reg*/
#define PRB(l)  ((l)<<1)  /*waiting port*/

/*calculates static weight and sets s_weight member of each node
**uses userdata. Weight is calculated according to goals #2.  
**Is invocated by static_analysis()
*/
void
calc_static_weights()
{
NODE *pi;
int i;
	for(pi=b->lastn; pi != b->firstn->back; pi=pi->back) {
#ifdef DEBUG
		if (vflag) {
			pi->ops[4]=getspace(160); /* 10 per one parametr */
			sprintf(pi->ops[4],"\t\t/");
		}  /* if */
#endif
		i=0;
		pi->exec_info.rs_info.weight = 0;
		if (pi->uses & MEM)
			pi->exec_info.rs_info.weight=i=MRW;  /* loads */
		comment4(pi,1,"MR:",i);
		i=0;
		if( pi->userdata & CLC_JCC ) {
			pi->exec_info.rs_info.weight +=CJW;  
			i=CJW;
		}  /* if */
		comment4(pi,2,"CJ:",i );
		i=0;
		if (pi->userdata & CLC_ADDR) {
			pi->exec_info.rs_info.weight +=CAW;  
			i=CAW;
		}  /* if */
		comment4(pi,3,"CA:",i);
		pi->exec_info.rs_info.weight += ( pi->chain_length << 8);  
		comment4(pi,4,"CL:",pi->chain_length);
		pi->exec_info.rs_info.weight += pi->dependents;
		comment4(pi,5,"ND:",pi->dependents);
	}  /* for */
}  /* calc_static_weights */

/*before start of prun(), it sets initial values of wait_reg[] and port_que[]
  to 0 for now, later perhaps will use the processor state
  at the end of the most probable executed previously basic block  
  Is invocated by  static_analysis()
*/
void
reset_static_data()
{
    memset(wait_reg,0,sizeof(wait_reg));
    memset(port_que,0,sizeof(port_que));
}  /* reset_static_data */

void 
init_pick() 
{
	picked=NULL;
}  /* init_pick */

/*returns which of two instructions must be issued in the first turn
  according to static weight and natural order.
  Is invocated by make_clist()
  calling the functions twice is a little silly.
*/
int
p6_before(p1,p2) NODE *p1, *p2;
{
  	if (s_weight_of(p1) > s_weight_of(p2))  return 1;
  	if (s_weight_of(p2) > s_weight_of(p1))  return 2;
  	return natural_order(p1,p2);
}  /* p6_before */

static int
p6_411_before(p1,p2) NODE *p1, *p2;
{
	if (   uops_of(p1) > 4
	    && uops_of(p2) <= 4) return 1;
	if (   uops_of(p2) > 4
	    && uops_of(p1) <= 4) return 2;
	return natural_order(p1,p2);
}  /* p6_411_before */

#if WE_WERE_DOING_DYNAMIC_JUST_FOR_FUN
/* counts dynamic weight */
calc_dynamic_weight()
{
clistmem *cl;
int w,n;

	for(cl=m0.next; cl!=&m1; cl=cl->next)	{
		w =  RGR( is_waiting_reg(cl->member) );
		comment4(cl->member,6,"RG:",w);
		n = PRB( is_waiting_port(cl->member) );
		comment4(cl->member,7,"PB:",n);
		w += n;
		w = (w << d_weight_shift);
		w += s_weight_of(cl->member);
		cl->member->userdata2 &= ~d_weight_mask ;
		cl->member->userdata2 |= w;
	}  /* for */
}  /* calc_dynamic_weight */
#endif

/* does it as in the tetrix */
/* The candidate list is sorted according to data which is a characteristic of
** the instruction and from the basic block.
** Here we add a heuristic that is a characteristic of the scheduling process.
** The only piece of new info: balance execution ports. Try not to issue
** back to back stores and back to back loads.
ROBERT: change it to NODE * and assign it to picked.
*/
static void
p6_pick()
{
clistmem *clp;
	if (picked == NULL) {
		picked =  m0.next->member;
		return;
	} else if (picked->exec_info.rs_info.ports & PRT3) {
		for (clp=m0.next; clp!=&m1; clp=clp->next) {
			if (clp->member->exec_info.rs_info.ports & PRT3 == 0) {
				picked = clp->member;
				return;
			}  /* if */
		}  /* for */
		picked =  m0.next->member;
		return;
	} else if (picked->exec_info.rs_info.ports & PRT2) {
		for (clp=m0.next; clp!=&m1; clp=clp->next) {
			if (clp->member->exec_info.rs_info.ports & PRT2 == 0) {
				picked = clp->member;
				return;
			}  /* if */
		}  /* for */
		picked =  m0.next->member;
		return;
	} else {
		picked =  m0.next->member;
		return;
	}  /* if */
	/* NOTREACHED */
}  /* p6_pick */

/*returns number characterizing how long an instruction has to wait port for 
  execution , uses port_que[]
  is invocated by calc_dynamic_weight()
  We can use several algorithms to count this number
  *1. Sum for all ports used by instruction
  2. Max wait time
  3. Multi_uops instr usually starts with uop in port 2,
     so return #cycles waiting of port 2.
  etc.                        
ROBERT: re-evaluate the logic of this thing.
*/
#if 0
int
is_waiting_port( p ) NODE*p;
{
	int i;
	unsigned wp=0;
	unsigned prt= port_of(p);
	
	for(i=PRT0 ; i<= iPRT4; i++ )
	{
		if( prt & port_mask[i] )
			wp+=port_que[i];
	}  /* for */
	return wp;
}  /* is_waiting_port */

/*returns number characterizing time that instr p must wait result in some
  reg as a resource to be dispatched by RS to execution 
  uses wait_reg[] array
  is invocated by find_weight 
*/
int
is_waiting_reg ( p ) NODE *p;
{
int i,m=0;

	for(i=iEAX; i<=iCONCODES; i++)
	{
		if( p->uses & wait_reg[i] ) m = MAX(m, wait_reg[i]);
	}  /* for */
	return m;
}  /* is_waiting_reg */
#endif

/*The main job of this function is to invoke for each picked[] dispatch() 
  which does all work related with moving of instruction and bookkeeping.
  Also it calls respective functions to updates port_que[] and wait_reg[].
  As mentioned above we consider one iteration as a "processor cycle"
  so decrement of wait time is accomplished here. 
  Is invocated by prun() 
  External dependence: it uses the value of the global variable 
  NODE *picked.
ROBERT: The idea of relating one selection with one processor cycle needs
revisit.
*/
void
p6_dispatch()
{
	static int cnt;
	int n=uops_of(picked);
	int i;
	picked2=NULL;

	if (isprefix(picked))
		picked2=picked->forw;
	dispatch(picked);
	scheded_inst_no++;
	
	cnt += n;
	
	if( picked2)
	{	
		dispatch(picked2);
		scheded_inst_no++;	
		cnt = 4;
	}  /* if */
	/*This is a little different commenting-out then the rest of the stuff
	**under if combined-scheduling. Part of this is doing decoder considerations
	**but also this models three dispatches for a machine cycle.
	**I think it make more sense not to model a cycle at all.
	**But then how do you use latency?
	**1. you model cycles.
	**2. you don't model latency.
	**O.k, let's bring this static cnt back and model latencies.
	**ROBERT revisit al least this long and confusing comment.
	*/
	if( cnt > 3 )
	{
		for(i=iPRT0;i<=iPRT4; i++)
			if (port_que[i])
				port_que[i]--;
		for(i=iEAX; i<=iCONCODES; i++)
			if (wait_reg[i])
				wait_reg[i]--;
		cnt = 0;
	}  /* if */
}  /* p6_dispatch */

/* updates clist by invocating old update_clist() function */
/* externally depends on picked wihch is set by the whole process of picking,
**and by picked2 which is set by p6_dispatch.
*/
void
p6_update_clist()
{
	update_clist(picked);
	if(picked2) update_clist(picked2);
}


/* returns p6 ports used by p uops yet wrong do each instr separately*/
/* The hack for move is there because the database info on moves if for
** register copies, which are done in alu ports.
*/
unsigned int
get_ports(p) NODE* p;
{
unsigned prt;
	
	if (is_move(p)) {
		if (p->uses & MEM) return PRT2;
		if (p->sets & MEM) return PRT3;
	}
	prt = opcodata[p->op-FIRST_opc].port ;
	if (p->uses & MEM) prt |= PRT2;
	if (p->sets & MEM) prt |= PRT3;

	return prt ;
}  /* get_ports */
  
/* returns latency of p , MOB bypasses are not taken into account */
int
get_latency (p) NODE *p;
{
int lt;
	
	lt = opcodata[p->op-FIRST_opc].latency;
	if (   is_move(p)
	    && ((p->uses | p->sets) & MEM))
		lt = 0;
	if (p->uses & MEM) lt+=3;
	if (p->sets & MEM) lt+=1;

	if (lt>15) lt=15; /*to fit in 4 bit representation...*/
	return lt;
}  /* get_latency */

/*How many uops are in the X86 instruction. It is only their number,
**Not their internal dependencies, so it is useful for decoder considerations,
**not for critical path scheduling.
*/
int
get_uops (p) NODE *p;
{
int n;
	
	if (   is_move(p)
	    && ((p->uses | p->sets) & MEM))
		n = 0;
	else
		n = opcodata[p->op-FIRST_opc].uops;
	if (p->uses & MEM)
		n += 1;
	if (   p->op != PUSHL
	    && p->sets & MEM)
		n += 2;

	return n;
}  /* get_uops */

static void
p6_pick_411()
{
static int ternary = 0;
clistmem *clp;

	switch (ternary) {
		case 0: /* need a multi uop instruction. */
			for (clp=m0.next; clp!=&m1; clp=clp->next) {
				if (uops_of(clp->member) > 4) {
					picked = clp->member;
					ternary = 0; /*next instruction also goes to 1'st decoder*/
					comment4(picked,6,"U1:",uops_of(picked));
					return;
				}  /* if */
			}  /* for */
			for (clp=m0.next; clp!=&m1; clp=clp->next) {
				if (uops_of(clp->member) > 1) {
					picked = clp->member;
					ternary = 1;
					comment4(picked,6,"U2:",uops_of(picked));
					return;
				}  /* if */
			}  /* for */
			picked =  m0.next->member;
			ternary = 1;
			comment4(picked,6,"U3:",uops_of(picked));
			return;

		case 1: /* need the 1'st of two single uop instructions */
			for (clp=m0.next; clp!=&m1; clp=clp->next) {
				if (uops_of(clp->member) == 1) {
					picked = clp->member;
					ternary = 2;
					comment4(picked,6,"U4:",uops_of(picked));
					return;
				}  /* if */
			}  /* for */
			/* not found a single uop inst, so the first in the clist is
			** a multiple uop inst. Issue it, and set the request for
			**the next according to whether it is more than 4 uops.
			*/
			picked = m0.next->member;
			if (uops_of(picked) > 4)
				ternary = 0;
			else
				ternary = 1;
			comment4(picked,6,"U5:",uops_of(picked));
			return;

		case 2: /* need the 2'nd of two single uop instructions */
			for (clp=m0.next; clp!=&m1; clp=clp->next) {
				if (uops_of(clp->member) == 1) {
					picked = clp->member;
					ternary = 0;
					comment4(picked,6,"U6:",uops_of(picked));
					return;
				}  /* if */
			}  /* for */
			picked = m0.next->member;
			if (uops_of(picked) > 4)
				ternary = 0;
			else
				ternary = 1;
			comment4(picked,6,"U7:",uops_of(picked));
			return;
	}  /* switch */
	/* NOTREACHED */
}  /* p6_pick_411 */

/********************
  P6 prescheduler
********************/

/*If the cbranch at the end of the basic block is flacky, and if "most
**of the instructions" in the basic block set the condition codes, then
**try to facilitate scheduling for the cbranch on the critical path by
**converting ADDL to LEAL and enabling the CMPL to move upwards.
*/
/*Meanwhile disable this trasformation because doing it for every basic
**block is a loser.
**ROBERT revisit this once flacky branch heuristics are implemented.
*/

static void
p6_prescheduler()
{
unsigned CCregs;
NODE *p=b->lastn;

	if ( !iscbr(p) )
		return;
	CCregs = p->uses & CONCODES_AND_CARRY;
	while (   !( p->sets & CCregs )
	       && p != b->firstn )
		p=p->back;
	if ( p == b->firstn )
		return;
	/* p now points to the instruction setting CONCODES and/or CARRY_FLAG
	   that the conditional branch is dependent on. */
	for (p=p->back; p != b->firstn; p=p->back) 
	{
		if ( !(p->sets & CCregs) ) continue; 
		if ( ( p->sets | p->uses) & ESP ) break;
		if (   p->op == SUBL
		    && isimmed(p->op1) && isreg(p->op2) )
		{
			char *s1=p->op1;
			s1++;
			if ( *s1 == '-' )  s1++;
			else { s1--; *s1 = '-'; }
			chgop(p,LEAL,"leal");
			p->op1=getspace(NEWSIZE);	
			sprintf(p->op1,"%s(%s)",s1,p->op2 );
			p->sets &= ~CONCODES_AND_CARRY;
			continue;
		}  /* if */
		if (   p->op == ADDL
		    && isreg(p->op2) )
		{
			if ( isimmed(p->op1) )
			{
				char *s1=p->op1;
				s1++;
				chgop(p,LEAL,"leal");
				p->op1=getspace(NEWSIZE);
				sprintf(p->op1,"%s(%s)",s1,p->op2);
				p->sets &= ~CONCODES_AND_CARRY;
				continue;
			}  /* if */
			if ( isreg(p->op1) )
			{
				char *s1=p->op1;
				chgop(p,LEAL,"leal");
				p->op1=getspace(NEWSIZE);
				sprintf(p->op1,"(%s,%s)",s1,p->op2);
				p->sets &= ~CONCODES_AND_CARRY;
				continue;
			}  /* if */
		}  /* if */
		break;
		/* mark xor if its used to avoid  partial stall	*/
	}  /* for */
}  /*end p6_prescheduler*/

static void
rebuildag()
{
	init_block(false);
	while (islabel(b->firstn)) {
		initnode(b->firstn);
        	b->firstn = b->firstn->forw; /*advance the block's beginning */ 
     	}  /*end while loop*/

	prev = b->firstn->back; /*last node before present block*/
	bldag(); /*construct dependency graph*/
	count_depths_before_reorder(); /* mark values of index regs at nodes */
	/*initnode fakes SUBL X,esp and ADDL X,esp to use MEM: undo this so that
	**we get the uops right.
	*/
	if (b->firstn ==  n0.forw->forw) {
		NODE *p;
		for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (   p->op == SUBL
			    && p->sets == (ESP | MEM |  CONCODES_AND_CARRY)) {
				p->sets = (unsigned) ESP | CONCODES_AND_CARRY;
				p->exec_info.rs_info.uops = get_uops(p);
				break;
			}  /* if */
		}  /* for */
	}  /* if */
	if (   last_is_br
	    && last_is_br->op == RET) {
		NODE *p;
		for (p = b->firstn; p != b->lastn->forw; p = p->forw) {
			if (   p->op == ADDL
			    && p->sets == (ESP | MEM | CONCODES_AND_CARRY)) {
				p->sets = (unsigned) ESP | CONCODES_AND_CARRY;
				p->exec_info.rs_info.uops = get_uops(p);
				break;
			}  /* if */
		}  /* for */
	}  /* if */
}  /* rebuildtag */

static void
reprun()
{
	scheded_inst_no = 0;
	m0.next = &m1;
	m1.back = &m0;
	init_pick();
	while (scheded_inst_no < b->length) { /*while not all were scheded*/
		make_clist(p6_411_before);
		p6_pick_411();
		p6_dispatch();
		p6_update_clist();
	}  /* while */
}  /* reprun */


/**********************
 *  ### END p6 section 
 **********************/

#ifdef DEBUG
/**************** 
 DEBUG utilities 
 ****************/
void show_clist()
{
	clistmem *clp;

	for( clp=m0.next; clp !=&m1; clp=clp->next )
		fprinst(clp->member); 
}  /* show_clist */

void show_bb(bi) BLOCK *bi;
{
NODE *p;
	if(!bi) bi=b;
	for(p=bi->firstn; p!=bi->lastn->forw; p=p->forw)
		fprinst(p);
	if( last_is_br )
		fprinst(last_is_br);
}  /* show_bb */
	
void show_depends()
{
NODE *p;
tdependent  *dp;
int i;
	for(p=b->firstn; p!=b->lastn->forw; p=p->forw) 
	{
		fprintf(stderr, "##:");fprinst(p);
		for(i=0;i<4;i++)
		{
			fprintf(stderr,"depend[%1d]\n",i);
			for(dp=p->depend[i]; dp ; dp=dp->next ) 
			{
				fprintf(stderr,"\t");
				fprinst(dp->depends);
			}  /* for */
		}  /* for */
		fprintf(stderr,"\n");
	}  /* for */
}  /* show_depend */
				
#endif

#ifdef USE_MACRO
/*************************
	### Macro operations
*************************/
static NODE *mo_mem;

is_our_case(b) BLOCK *b;
{
  NODE *p;
  char * c = b->lastn->forw->ops[0];
  int i;

	mo_mem=NULL; /* memory referencies in the b2 */
	 
	/* check memory operands in b. further do tis in seems_same ?? */

	for(p=b->firstn; p!=b->lastn->forw; p=p->forw) {
		if( p->op == CALL ) {
			return false;
		}
		if ( p->op >= SCASB ) {
			/* string, FP and exotic instructions */
			return false;
		}
		for(i=1; i<=3; i++) {
			if( !ismem( p->ops[i] ) ) {
				continue;
			}
			if( mo_mem ) {
				return false;
			}
			mo_mem = p;
		}  /* for */
	}  /* for */
	/* check that only one jmp exists to b3->first(label) */
	for( ALLN(p) ) {
		if (   (   iscbr(p)
			|| p->op == JMP )
		    && !strcmp(p->ops[1],c)
		    &&  p != b->firstn->back ) {

			return false;
		}  /* if */
	}  /* for */
	return true;
}  /* is_our_case */

#define M_LIMIT  10
/* add returned value to save time if it hasen't done transformations */
void
convert_block_to_macroop()
{
  NODE *p,*mo;
  BLOCK *b1,*b2,*b3;

	for( b1=b0.next; b1 != NULL; b1 = b1->next ) {
		b1->ltest=0;
	}
	
#ifdef DEBUG
	COND_RETURN("convert_block_to_macroop");
	if( getenv( "no_macro" ) ) {
		return;
	}
#endif

	b1=b0.next;
	b2=b1->next;
	if( !b2 ) {
		return;
	}
	b3=b2->next;
	if( !b3 ) {
		return;
	}
	
	while ( b3 ) {
		if (   b1->nextr == b3
		    && b1->nextl == b2
		    && b2->nextr == NULL
		    && b2->nextl == b3
		    && b2->length <= M_LIMIT 
		    && ( b1->length + b2->length + b3->length ) < BLOCK_SIZE_LIMIT
		    && !is_any_label(b2->firstn) 
		    && !is_any_label(b3->firstn->back)
		    && !is_any_label(b3->firstn->forw)
		    &&b1->length >1
		/* add memory change analysis for b2 (call, mov mem ) */
		    && is_our_case(b2) )
		{
			/* save info for inverse convertion */
			save_macro *save_ip = GETSTR(save_macro);
			save_ip->b2 = b2;
			save_ip->b3 = b3;
			save_ip->b1_length = b1->length;
			save_ip->b1_lastn = b1->lastn;
			save_ip->b3_firstn = b3->firstn;

#ifdef WD_macro_DEBUG
			fprintf(stderr,"### macro %s : %s\n", get_label(),
			        b3->firstn->ops[0]);
#endif
			if(b1->ltest) {
				/* b1 already contains an artificial mo */ 
				save_ip->next = (save_macro*)b1->ltest;
			} else {
				save_ip->next = NULL;
			}

			b1->ltest = (void *) save_ip;

			/* creating artificial block by merging b1, b2, and
			   b3 to one.
			*/
			b1->next = b3->next;
			b1->nextl = b3->nextl;
			b1->nextr = b3->nextr;
			if( b3->next ) {
				b3->next->prev = b1;
			}
			b1->length = b1->length-1/*jcc*/ + 1/*b2*/+ b3->length ;
			b1->lastn = b3->lastn;
			/* block info of b2,b3 is unchanged */
			
			/* create mo instead of b2 and b3->firstn ( label )
			   and make new relations between nodes of new b1 mo 
			*/
			/* compress b2, jcc, b3_firstn.label to macro */
			mo = insert(save_ip->b1_lastn->back);
			mo->forw = b3->firstn->forw; /* first after label */
			b3->firstn->forw->back = mo;
			save_ip->mo = mo;
			
			/* set fields of mo NODE */
			chgop(mo,  MACRO, "macro" );
			memset( mo->trace,0,sizeof(trace_type)*8 );
			if (mo_mem)
			{
				mo->op1 = mo_mem->op1; /*keep memory operand */
				mo->op2 = mo_mem->op2;
				mo->op3 = mo_mem->op3;
				mo->sasm = (int) mo_mem;
				mo->ebp_offset = mo_mem->ebp_offset;
				mo->esp_offset = mo_mem->esp_offset;
			}
			else
				 mo->sasm = 0;
				 
			/** sets_uses;	*/
			mo->uses = mo->sets = 0;
			for(p=b2->firstn; p!=b2->lastn->forw; p=p->forw) {
				mo->sets |= p->sets | msets(p);
				mo->uses |= p->uses | muses(p);
			}  /* for */
			mo->uses &= save_ip->b1_lastn->nlive;
			mo->uses |= save_ip->b1_lastn->uses & CONCODES_AND_CARRY; /* jcc!!! */
			 
			/* go on with next triplet of blocks */
			b2 = b3->next;
			if( b2) {
				b3 = b2->next;
			} else {
				b3=NULL;
			}
		} else {
			b1=b2;
			b2=b3;
			b3=b3->next;
		}  /* if */
	} /* while */
}  /* convert_block_to_macroop */


#endif	


