#ident	"@(#)optim:i386/postpeep.c	1.36"
#include "sched.h"
#include "optutil.h"
#include "regal.h"
#include <values.h>
#include <ctype.h>
static int postw1opt();
static int postw1_5opt();
static int postw2opt();
static int postw2agr();
static int postw3opt();
static int postw3agr();
static boolean num_name();
static NODE *pair_initw();
void sw_pipeline();
void backalign();
#ifdef DEBUG
extern int vflag;
#endif

extern char *itohalfreg();
extern char *itoreg();
extern int isbase();
extern void remove_base();
extern void window();

/* pair window is like window, but looks for pairs of instructions rather than
** single instructions. A non paired instruction is also taken as a pair.
** This is only relevant after pentium oriented scheduling!
*/

static NODE *gpf;	/* pointer to first window node */
static NODE *opf;	/* pointer to predecessor of first window node */
static int wsize;	/* window size for peephole trace */

void
pair_window(size, func) register int size; boolean (*func)();
{
register NODE *pl;
register int i;

	/* find first window */

	wsize = size;
	if ((pl = pair_initw(n0.forw)) == NULL)
		return;

	/* move window through code */

	for (opf = gpf->back; ; opf = gpf->back) {
		COND_SKIP(goto advance,"p%d %d",size,second_idx,0);
		if ((*func)(gpf, pl) == true) {
#ifdef DEBUG
		if (start && last_func() && last_one()) {
			fprintf(stderr,"+ ");
		}
#endif
			if (size > 1) {
			/* move window back in case there is an overlapping improvement */
			/* opf should go back to where it was before */
				for (i = 2; i <= size; ) {
					if ((opf = opf->back) == &n0) {
						opf = n0.forw;
						break;
					}
					if (opf->forw->usage != ON_J) i++;
				}
				/* now opf is there, set fp and pl */
				if ((pl = pair_initw(opf)) == NULL)
					return;
				/* we have gpf and pl, activate *func */
				continue;
			}
		}
#ifdef DEBUG
		else if (start && last_func() && last_one()) {
			fprintf(stderr,"- ");
		}
#endif

		/* move window ahead */
#ifdef DEBUG
	advance:
#endif
		if ((pl = pl->forw) == &ntail)
			return;
		if (pl->usage == ON_X && pl->forw->usage == ON_J) pl = pl->forw;
		if (pl == &ntail) 
			return;
		gpf = gpf->forw;
		if (gpf->usage == ON_J) gpf = gpf->forw;
		if (islabel(pl) && (pl = pair_initw(pl->forw)) == NULL)
			return;
	}
}

/* find first available window */
	static NODE *
pair_initw(p) register NODE *p;
{
	register int i;
	/* set the first inst to be the parameter, and go $size forward.
	** verify that there exist $size insts that are not labels.
	*/
	if ((gpf = p) == NULL)
		return (NULL);
	/* move p down until window is large enough */
	for (i = p->usage == ON_X ? 0 : 1; i <= wsize; ) {
		if (p == &ntail) /* no more windows */
			return (NULL);
		if (islabel(p)) { /* restart scan */
			/* Now the gpf will be the next one after the label */
			gpf = p->forw;
			i = 0;
		}
		/* so far so good, this will be in the window, examine the next p */
		p = p->forw;
		if (p->usage != ON_X) {
			i++;
		}
	}
	/* did one too many forw's in the loop, the real pl should be one less. */
	p = p->back;
	if (p->usage == ON_X) {
		p = p->back;
	}
	return p;
}

static boolean
pair_w2opt(pf,pl) NODE *pf,*pl;
{
NODE *pfv,*plu;
NODE *last;
unsigned int sets_first_pair, uses_first_pair;
char *pt;
unsigned int reg;
	/* set the local NODE *'s */
	pfv = pf->usage == ON_X ?  pf->forw : NULL;
	if (pl->usage == ON_J) {
		plu = pl->back;
	} else {
		plu = pl;
		pl = NULL;
	}
	if (pl) last = pl; else last = plu;
	/* all set */
	if (pfv && pfv->forw != plu) return false;
	if (!pfv && pf->forw != plu) return false;

	/* The last instruction can not be a branch */
	if (last->op == CALL || isbr(last)) return false;

	/* If the second inst in the first pair is a call then the transformation
	** is a loser.
	*/
	if (pfv && pfv->op == CALL) return false;

	sets_first_pair = sets(pf);
	if (pfv) sets_first_pair |= sets(pfv);
 	uses_first_pair = uses(pf);
	if (pfv) uses_first_pair |= uses(pfv);

/*
**  set		R1                    set		R1
**  leal	d(R1,R2,s),R3	      leal	d(,R2,s),R3
**                                addl	R1,R3
**  if R1 is the same as R2 there is no point in doing so.
**  if R1 is the same as R3 (can be!) it is no longer correct.
**/

	if ((plu->op == LEAL)
	 && (pt = strchr(plu->op1,'(')) != 0
	 && *(pt+1) == '%' /* a base register is there */
	 && ((reg = setreg(pt+1)) & sets_first_pair)
	 && strchr(plu->op1,',')
	 && strncmp(plu->op2,pt+1,4) != 0 /* r1 != r3 */
	 && strncmp(pt+1,pt+6,4) != 0 /* r1 != r2 */
	 && isdeadcc(plu)
	 && (!pl || !(indexing(pl) & sets_first_pair)) /*only one agi*/
	) {
			NODE *pnew;
			/* if(num_name(plu->op1)) goto out1a; osnat what is this ? */
			if (pl != NULL && (!(sets(pl) & reg)) && !isbr(pl)) {
				pnew=insert(pl);
				pnew->nlive = pl->nlive;
			} else {
				pnew=insert(plu);
				pnew->nlive = plu->nlive;
				if (pl) pl->usage = ALONE;
			}
			chgop(pnew,ADDL,"addl");
			pnew->op1 = itoreg(reg);
			pnew->op2 = plu->op2;
			pnew->usage = ALONE;
#ifdef DEBUG
            if (vflag) pnew->op4 = "\t\t/\tpostpeep";
#endif
			remove_base(&(plu->op1));
			plu->nlive |= setreg(pnew->op1) | setreg(pnew->op2);
			return true;	
	}
/*out1a:*/
 
/*
**
**	set		R1	        set		R1
**	leal	d(R1),R2	movl	$d,R2
**	                    addl	R1,R2
**
**	if R1 is the same as R2 it is no longer correct.
*/
	if (   (plu->op == LEAL)
	 	&& (isdigit(plu->op1[0]) || isalpha(plu->op1[0]))
	 	&&	(pt=strchr(plu->op1,'('))
	 	&&	((reg=setreg(pt+1)) & sets_first_pair) /* an agi */
	 	&&	strncmp(plu->op2,pt+1,4) != 0 /* r1 != r2 */
	 	&&	!strchr(plu->op1,',') /* only base register, no index */
	 	&&	isdeadcc(plu)
	 	&& (   !pl
			|| !(indexing(pl) & sets_first_pair)) /*only one agi*/
	) {
			char *r1;
			NODE *pnew;


			if (   pl
				&& (!(sets(pl) & reg))
				&& !isbr(pl)
				&& !(pl->sets & CONCODES_AND_CARRY)) {
				pnew = insert(pl);
				pnew->nlive = pl->nlive;
			} else {
				pnew = insert(plu);
				pnew->nlive = plu->nlive;
				if (pl) pl->usage = ALONE;
			}
			chgop(pnew,ADDL,"addl");
			pnew->op1 = itoreg(reg);
			pnew->op2 = plu->op2;
#ifdef DEBUG
            if (vflag) pnew->op4 = "\t\t/\tpostpeep";
#endif
			pnew->usage = ALONE;
			chgop(plu,MOVL,"movl");
			remove_base(&(plu->op1));
			r1=getspace(strlen(plu->op1)+2);
			strcpy(r1,"$");
			r1=strcat(r1,plu->op1);
			if ((strtoul(plu->op1,(char **)NULL,0) == 0)&& (*plu->op1 == '0')) {
				DELNODE(plu);
				chgop(pnew,MOVL,"movl");
			} else{
				plu->op1=r1;
			}
			plu->op1=r1;
			plu->nlive |= setreg(pnew->op1) | setreg(pnew->op2);

			return true;	
	}

	return false;

}/*end pair_w2opt*/

static boolean
pair_w3opt(pf,pl) NODE *pf,*pl;
{
NODE *pfv, *pmu,*pmv,*plu;
unsigned int sets_first_pair;
char *tmp_opr;
	
	/* set the local NODE *'s */
	pfv = pf->usage == ON_X ?  pf->forw : NULL;
	pmu = pfv ? pfv->forw : pf->forw;
	pmv = pmu->usage == ON_X ? pmu->forw : NULL;
	if (pl->usage == ON_J) {
		plu = pl->back;
	} else {
		plu = pl;
		pl = NULL;
	}
	/* all set */
	
	if (pmv && pmv->forw != plu) return false;
	if (!pmv && pmu->forw != plu) return false;

	/* If the second inst in the first or second pair is a call then
	** the transformation is a loser.
	*/
	if (pfv && pfv->op == CALL) return false;
	if (pmv && pmv->op == CALL) return false;


	sets_first_pair = sets(pf);
	if (pfv) sets_first_pair |= sets(pfv);

/* opportunity from addarr2.c:
**  (gain one cycle)
**  addl    %edx,%ebp            stays
**  leal    -3(%eax),%edx        stays
**  movl    4(%ebp),%esi         movl %ebx,%esi
**  addl    %ebx,%esi            nop
**  some pair                    addl 4(%ebp),%esi
**                               the same pair
*/

	if (pmv == NULL
	 && (sets_first_pair & indexing(pmu)) /*agi from 1'st pair to 2'nd*/
	 && (!pmv || !(sets_first_pair & indexing(pmv))) /*only one agi */
	 && pmu->op == MOVL
	 && plu->op == ADDL
	 && isreg(pmu->op2)
	 && isreg(plu->op2)
	 && !strcmp(pmu->op2,plu->op2)
	 && ! (sets_first_pair & indexing(plu)) /* no agi */
	 && ! (setreg(pmu->op2) & scanreg(pmu->op1,false))
	 && ! (setreg(pmu->op2) & scanreg(plu->op1,false))
	) {
		wchange();
		tmp_opr = pmu->op1;
		pmu->op1 = plu->op1;
		plu->op1 = tmp_opr;
		return true;
	}
	return false;
}/*end pair_w3opt*/

void
postpeep()
{
	COND_RETURN("postpeep");
	sets_and_uses(); /* schedule() add memory info, confuses ldanal(). */
	ldanal();    /*livedaed analisys */
    window(2,postw1_5opt);
    window(1,postw1opt);
	window(2,postw2opt);
	if (target_cpu & (P5 | blend)) {
		pair_window(2,pair_w2opt);
		pair_window(3,pair_w3opt);
		window(3,postw3opt);
		window(3,postw3agr);
		window(2,postw2agr);
	}
}/*end postpeep*/

static char *brnames[] = { "%al", "%dl", "%cl", "%bl", 0 };
static char *srnames[] = { "%ax", "%dx", "%cx", "%bx", 0 };
static char *rnames[] =  { "%eax", "%edx", "%ecx", "%ebx", 0 };

static boolean
num_name(cp)  char *cp;
{
	char ind= 1;
	while((*(cp+ind-1)) != '('){
		if(	(	(*(cp+ind)) == '+'	) || (	(*(cp+ind)) == '-'	))
			return true;
		ind++;
	}	
	return false;
}


static boolean					/* true if changes made */
postw2opt(pf,pl)
register NODE * pf;			/* first instruction node of window */
register NODE * pl;			/* second instruction node */
{

	int temp;
    int cop1 = pf->op;			/* op code number of first inst. */
    int cop2 = pl->op;			/* op code number of second inst. */
    int m;				/* general integer temporary */
    unsigned int m1;
    char *pt;
    char *resreg;
    int src1=0, dst1=0;			/* sizes of movX source, destination */
	char *tmpc;


  if (cop1 == INCL
   && cop2 == INCL
   && !isvolatile(pf,1)
   && !strcmp(pf->op1,pl->op1)
   ) {
	wchange();
	pf->op2 = pf->op1;
	pf->op1 = "$2";
	chgop(pf,ADDL,"addl");
	DELNODE(pl);
	return true;
   }

   if (cop1 == ADDL
	&& cop2 == ADDL
	&& isimmed(pf->op1)
	&& isimmed(pl->op1)
	&& !isvolatile(pf,2)
	&& !strcmp(pf->op2,pl->op2)
	) { int m1 = (int)strtoul(pf->op1+1,(char **)NULL,0);
	    int m2 = (int)strtoul(pl->op1+1,(char **)NULL,0);
		wchange();
		pf->op1 = getspace(ADDLSIZE);
		sprintf(pf->op1,"$%d",m1+m2);
		chgop(pf,ADDL,"addl");
		DELNODE(pl);
		return true;
	}

/* Convert dependency to anti dependency  
**  mov R1,R2  ==>  mov R1,R2 
**  mov R2,R3  ==>  mov R1,R3 
*/
   if (((cop1 ==MOVL) || (cop1 == MOVW) || (cop1 == MOVB))
   && (cop2 == cop1)
   && isreg(pf->op1)
   && isreg(pf->op2)
   && isreg(pl->op1)
   && isreg(pl->op2)
   && !strcmp(pf->op2,pl->op1)
   && !samereg(pf->op1,pf->op2)
   )
   {
     wchange();
     pl->op1 = pf->op1;
     pf->nlive |= setreg(pf->op1);
     return true;
   }
/* Eliminate agi contentions for i486.
** If there is a contention with the base register and there is no
** scale then swap the base and index registers.
*/
	if (target_cpu & (P4|blend))
	 if (((m = 1, pl->ops[1] && *pl->ops[1] != '$' && *pl->ops[1] != '%')
	  || ( m = 2, pl->ops[2] && *pl->ops[2] != '%')
	  || ( m = 3, pl->ops[3] && *pl->ops[3] != '%'))
	 && strchr(pl->ops[m],'(')
	 && isbase(sets(pf) & ~ESP,pl->ops[m]) /* ESP can't be index */
	 && strchr(pl->ops[m],',')
	 && ! has_scale(pl->ops[m])
	 ) { char ch,*pt1;
		pt1 = getspace(strlen(pl->ops[m]) +1);
		strcpy(pt1,pl->ops[m]);
		pt = strchr(pt1,'(');
		if ( pt[3] != pt[8] || pt[4] != pt[9]) {/* (%eax,%ebx) => (%ebx,%eax) */
			ch = pt[3];
			pt[3] = pt[8];
			pt[8] = ch;
			ch = pt[4];
			pt[4] = pt[9];
			pt[9] = ch;
	 	} else { /* save one cycle: XX(%eax,%eax) =>  XX(,%eax,2) */
	 		pt[1] = ',';
	 		pt[2] = '%';
	 		pt[3] = 'e';
	 		pt[4] = pt[8];
	 		pt[5] = pt[9];
	 		pt[6] = ',';
	 		pt[7] = '2';
	 		pt[8] = ')';
	 		pt[9] = '\0';
	 	}
		pl->ops[m] = pt1;
		return(true);			/* announce success */
	 }
	 /* copy elimination: scheduling made the opportunity.
	 ** was originally intended for backprop. Now the backprop opportunity
	 ** is done at w6opt.c, but it seems there are more opportunities for
	 ** this one.
	 ** move reg1,reg2
	 ** use reg2         -> use reg1
	 ** if reg2 is dead after the sequence.
	 */
	 if (pf->op == MOVL
	  && isreg(pf->op1)
	  && isreg(pf->op2)
	  && isdead(pf->op2,pl)
	  && OpLength(pl) >= LoNG
	  && uses(pl) & setreg(pf->op2)
	  && !(sets(pl) & setreg(pf->op2))
	  && ! (isshift(pl) && (setreg(pf->op2) == ECX) && (setreg(pl->op1) == CL))
	  && ((scanreg(pl->op1,false) | scanreg(pl->op2,false)) & setreg(pf->op2))
	  ) {
		replace_registers(pl,setreg(pf->op2),setreg(pf->op1),3);
		DELNODE(pf);
		new_sets_uses(pl);
		return true;
	  }
/* Below this point, only pentium oriented peepholes */
if ((target_cpu & (P5|blend)) == 0) return false;
if ((pf->usage != ALONE || pl->usage != ALONE)) return false;

/* Recombine pairs of movb instructions */

	if (cop1 == MOVB
	 && cop2 == MOVB
	 && isreg(pf->op2)
	 && isreg(pl->op2)
	 && !isreg(pf->op1)
	 && !isreg(pl->op1)
	 && pf->op2[1] == pl->op2[1]
	 && pf->op2[2] != pl->op2[2]
	 ) {
		if (   isconst(pf->op1)
			&& isconst(pl->op1)) {
		int n,n1,n2;
			n1 = (int)strtoul(pf->op1+1,(char **)NULL,0);
			n2 = (int)strtoul(pl->op1+1,(char **)NULL,0);
			if (sets(pf) & (AL | BL | CL | DL)) {
				n = n2 << 8 | n1;
				pf->op1 = getspace(ADDLSIZE);
				sprintf(pf->op1,"$%d",n);
				tmpc = getspace(4);
				tmpc = strdup(pf->op2);
				tmpc[2] = 'x';
				pf->op2 = tmpc;
				chgop(pf,MOVW,"movw");
				DELNODE(pl);
			} else {
				n = n1 << 8 | n2;
				pl->op1 = getspace(ADDLSIZE);
				sprintf(pl->op1,"$%d",n);
				tmpc = getspace(4);
				tmpc = strdup(pl->op2);
				tmpc[2] = 'x';
				pl->op2 = tmpc;
				chgop(pl,MOVW,"movw");
				DELNODE(pf);
			}
			return true;
		} else
		if (   ismem(pf->op1)
			&& ismem(pl->op1)) {
			int x1,x2;
			char *t1,*t2;
			x1 = (int)strtoul(pf->op1,&t1,0);
			x2 = (int)strtoul(pl->op1,&t2,0);
			/* If displacements are followed by a '+' or '-', both operands
			   must be identical. */
			if (   ((1 == x1 - x2) || (1 == x2 - x1))
				&& !strcmp(t1,t2)) {

				/* Further check that the register(s) modified by the first
				   instruction are not used to address the source operand
				   of the second instruction. */
				if (! (uses(pl) & full_reg(sets(pf)))) {
					NODE *del_node;
					int rotate;
					int reg;
					/* Load from the lowest memory address. */
					if (x1 < x2) {
						/* pf contains the low memory address. */
						del_node = pl;
					} else {
						/* pl contains the lowest memory address. */
						del_node = pf;
						pf = pl;
					}  /* if */
					rotate = ! ((reg = sets(pf)) & (AL | BL | CL | DL));
					chgop(pf,MOVW,"movw");
					pf->op2 = itohalfreg(reg);
					DELNODE(del_node);
					if (rotate) {
						pl = insert(pf);
						chgop(pl,RORW,"rorw");
						pl->op1 = "$8";
						pl->op2 = pf->op2;
					}  /* if */
					return true;
				}  /* if */
			}
			/* fall thru to next peephole */
		}
	 }
/* Eliminate extraneous moves when the next instruction is a
** compare, and the new temporary register is dead afterwards.
**
**	Example:
**
**	movX O1,R1		->	cmpX O1,R2
**	cmpX R1,R2
**
**/

    if (ismove(pf,&src1,&dst1)
	&&  (cop2 == CMPL || cop2 == CMPW || cop2 == CMPB)
	&&  src1 == dst1
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  strcmp(pl->op1,pl->op2) != 0
	&&  !isvolatile(pf, 1)
	) {
		wchange();
		lmrgin3(pl,pf,pl);		/* preserve line number info */ 
		pl->op1 = pf->op1;
		DELNODE(pf);			/* delete the movX */
		return(true);			/* announce success */
    }

/* Eliminate extraneous moves when the next instruction is a
** compare, and the new temporary register is dead afterwards.
**
**	Example:
**
**	movX R1,R2		->	cmpX R1,O1
**	cmpX R2,O1
**
**  or
**	movX $imm,R2		->	cmpX $imm,O1
**	cmpX R2,O1
**
**/

    if (ismove(pf,&src1,&dst1)
	&&  (cop2 == CMPL || cop2 == CMPW || cop2 == CMPB)
	&&  src1 <= dst1
	&&  ( isreg(pf->op1) || isnumlit(pf->op1))
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  ! usesreg(pl->op2,pf->op2)
	) {
	if (pf->op1[3] == 'i' && src1 < dst1)
	  goto cmp2_out;		/* do nothing, these regs may not be promotable */
	/* POSSIBLY UNNECESSARY */
	if (isreg(pl->op2) && pl->op2[3] == 'i' && src1 < dst1)
	  goto cmp2_out;		/* do nothing, these regs may not be promotable */
	switch ( src1 ) {
	case 1:
	  if ( cop2 != CMPB )
	    goto cmp2_out;
	  wchange();			/* changing window */
	  if ( isreg(pl->op2) ) {
	    for (temp = 0; rnames[temp] != 0; temp++ )
	      if ( strcmp(pl->op2,rnames[temp]) == 0 )
			pl->op2 = brnames[temp];
	    for (temp = 0; srnames[temp] != 0; temp++ )
	      if ( strcmp(pl->op2,srnames[temp]) == 0 )
			pl->op2 = brnames[temp];
	  }
	  break;
	case 2:
	  if ( cop2 != CMPW )
	    goto cmp2_out;
	  wchange();			/* changing window */
	  if ( isreg(pl->op2) ) {
	    for (temp = 0; rnames[temp] != 0; temp++ )
	      if ( strcmp(pl->op2,rnames[temp]) == 0 )
		pl->op2 = srnames[temp];
	  }
	  break;
	case 4:
	  if ( cop2 != CMPL )
	    goto cmp2_out;
	  wchange();
	  break;
	}
	lmrgin3(pl,pf,pl);		/* preserve line number info */ 
	pl->op1 = pf->op1;
	DELNODE(pf);			/* delete the movX */
	return(true);			/* announce success */
    }
cmp2_out:

/* case:  redundant movl
**
**	movl	   01,R1
**	op2	   R1,R2	->	op2   01,R2
**
**	if R1 is dead and is not R2.
*/
    if ((cop1 == MOVL || cop1 == MOVW || cop1 == MOVB)
	&&  is2dyadic(pl)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pf->op2,pl)
	&&  setreg(pf->op2) != setreg(pl->op2)
    ) {
		wchange();		/* change window */
		lmrgin3(pf,pl,pf);	/* preserve line number info */ 
		mvlivecc(pf);		/* preserve conditions codes live info */
		pl->op1 = pf->op1;
		DELNODE(pf);		/* delete first inst. */
		return(true);
    }
/* case:  clean up xorl left by one-instr peephole
**
**	xorl	R1,R1
**	op2	R1, ...	->	op2   $0, ...
**
**	if R1 is dead
*/
    if (cop1 == XORL
	&&  (is2dyadic(pl) || cop2==PUSHW || cop2==PUSHL)
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op1,pf->op2) == 0
	&&  strcmp(pf->op2,pl->op1) == 0
	&&  isdead(pl->op1,pl)
    )
	if (strcmp(pf->op2,pl->op2) != 0) /* the sensible case */
	{
		wchange();		/* change window */
		lmrgin3(pf,pl,pf);	/* preserve line number info */ 
		mvlivecc(pf);		/* preserve conditions codes live info */
		pl->op1 = "$0";		/* first operand is immed zero */
		DELNODE(pf);		/* delete first inst. */
		return(true);
    } else if (pl->nlive & CONCODES_AND_CARRY) { /*pl has no effect, remove it*/
		wchange();
		mvlivecc(pl);
		DELNODE(pl);
		return true;
	} else { /*both have only dead effects, remove them*/
		wchange();
		mvlivecc(pf);
		DELNODE(pf);
		DELNODE(pl);
		return true;
	}

/* case:  clean up cdtl left by one-instr peephole
**
**	movl	%eax,%edx  -> cltd
**	sarl	$31,%edx
**
*/
    if (    cop1 == MOVL 
		&&  cop2==SARL
		&&  strcmp(pl->op1,"$31") == 0
		&&  strcmp(pl->op2,"%edx") == 0
		&&  strcmp(pf->op2,"%edx") == 0
		&&  strcmp(pf->op1,"%eax") == 0
		&&  isdeadcc(pl)
    ) {
		wchange();		/* change window */
		lmrgin3(pf,pl,pf);	/* preserve line number info */ 
		mvlivecc(pf);		/* preserve conditions codes live info */
		pl->op1 = NULL;		/* no first operand  */
		pl->op2 = NULL;		/* no second operand */
		chgop(pl,CLTD,"cltd");
		DELNODE(pf);		/* delete first inst. */
		return(true);
    }

/*case:
**  mov O1,R2
**  test R2,R2  ->  cmp $0,O1
**
**  if R2 is dead after the sequence
*/
    if (cop1 == MOVL
	 && cop2 == TESTL
	 && ! isconst(pf->op1)
	 && isreg(pf->op2)
	 && isreg(pl->op1)
	 && isreg(pl->op2)
	 && isdead(pl->op2,pl)
	 && strcmp(pf->op2,pl->op2) == 0
	 && strcmp(pl->op1,pl->op2) == 0
	)  {
	 wchange();		/* change window */
	 lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	 mvlivecc(pf);		/* preserve conditions codes live info */
	 cp_vol_info(pf,pl);		/* preserve volatile info */
	 chgop(pl,CMPL,"cmpl");
	 pl->op1 = "$0";
	 pl->op2 = pf->op1;
	 DELNODE(pf);		/* delete first inst. */
	 return(true);
    }


/*case:
**  
**  set		R1              set		R1
**  leal	d1(R1,R2,d),R3	leal	(,R2,d),R3					
**	                        addl	R1,R3
**
**  if R1 is the same as R2 there is no point in doing so.
**  if R1 is the same as R3 (can be!) it is no longer correct.
**/

	if(	(pl->op == LEAL)
	&&	isreg(pl->op2)
	&&	(pt=strchr(pl->op1,'('))
	&&	strchr(pl->op1,',')
	&&	(m1=setreg(pt+1))
	&&	(resreg=itoreg(m1))
	&&	(m1 & sets(pf))
	&&	!samereg(pl->op2,resreg)
	&&	isdeadcc(pl)
	)
		{
			int r1;
			NODE *pnew;
			r1 = setreg(pt=strchr(pl->op1,',')+1);
			if(	(r1 != NULL) && (isreg(pt)) && samereg(pt,resreg))
			{
				goto out1;
			}
			if(num_name(pl->op1))
				goto out1;
			pnew=insert(pl);
			chgop(pnew,ADDL,"addl");
			pnew->op1 = resreg;
			pnew->op2 = pl->op2;
			pnew->nlive = setreg(pnew->op2) | CONCODES_AND_CARRY;
#ifdef DEBUG
            if (vflag) pnew->op4 = "\t\t/\tpostpeep";
#endif
			remove_base(&(pl->op1));
			return true;	
		}
out1:

/*case:
**
**	set		R1	        set		R1
**	leal	d(R1),R2	movl	$d,R2
**	                    addl	R1,R2
**
**	if R1 is the same as R2 it is no longer correct.
*/
	if(	(pl->op == LEAL)
	&&	isreg(pl->op2)
	&&  isdigit(pl->op1[0])
	&&	(pt=strchr(pl->op1,'('))
	&&	(m1=setreg(pt+1))  != 0
	&&	(resreg=itoreg(m1))  
	&&	(m1 & sets(pf))
	&&	!samereg(pl->op2,resreg)
	&&	!strchr(pl->op1,',')
	&&	isdeadcc(pl)
	&&  ! (num_name(pl->op1))
	)
		{
			char *r1;
			NODE *pnew;
			pnew=insert(pl);
			chgop(pnew,ADDL,"addl");
			pnew->op1 = resreg;
			pnew->op2 = pl->op2;
			pnew->nlive = setreg(pnew->op2) | CONCODES_AND_CARRY;
#ifdef DEBUG
            if (vflag) pnew->op4 = "\t\t/\tpostpeep";
#endif
			chgop(pl,MOVL,"movl");
			remove_base(&(pl->op1));
			r1=getspace(strlen(pl->op1)+2);
			strcpy(r1,"$");
			r1=strcat(r1,pl->op1);
			if((strtoul(pl->op1,(char **)NULL,0) == 0)&& (*pl->op1 == '0')){
				DELNODE(pl);
				chgop(pnew,MOVL,"movl");
			}
			else{
				pl->op1=r1;
			}
			pl->op1=r1;
			return true;	
		}
/*case:
**  mov O1,R2
**  cmp R3,R2  ->  cmp $0,O1
**
**  if R2 is dead after the sequence
**  if it is known that R3 holds zero (from zero value trace).
*/
    if (cop1 == MOVL
	 && cop2 == CMPL
	 && isreg(pf->op2)
	 && isreg(pl->op1)
	 && isreg(pl->op2)
	 && isdead(pl->op2,pl)
	 && pl->zero_op1
	 && strcmp(pf->op2,pl->op2) == 0
	)  {
	 wchange();		/* change window */
	 lmrgin3(pf,pl,pf);	/* preserve line number info */ 
	 mvlivecc(pf);		/* preserve conditions codes live info */
	 cp_vol_info(pf,pl);		/* preserve volatile info */
	 chgop(pl,CMPL,"cmpl");
	 pl->op1 = "$0";
	 pl->op2 = pf->op1;
	 DELNODE(pf);		/* delete first inst. */
	 return(true);
    }
	/*Un RISC the FP ops back to FI?? */
	if (pf->op == FILD || pf->op == FILDL) {
		if (pl->op1 && !strcmp(pl->op1,"%st(1)")) {
			switch (pl->op) {
				case FADDP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FIADD,"fiadd");
							else
								chgop(pl,FIADDL,"fiaddl");
							break;
				case FSUBP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FISUBR,"fisubr");
							else
								chgop(pl,FISUBRL,"fisubrl");
							break;
				case FMULP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FIMUL,"fimul");
							else
								chgop(pl,FIMULL,"fimull");
							break;
				case FDIVP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FIDIVR,"fidivr");
							else
								chgop(pl,FIDIVRL,"fidivrl");
							break;
				case FDIVRP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FIDIV,"fidiv");
							else
								chgop(pl,FIDIVL,"fidivl");
							break;
				case FSUBRP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FISUB,"fisub");
							else
								chgop(pl,FISUBL,"fisubl");
							break;
				case FCOMP:
							if (OpLength(pf) == WoRD)
								chgop(pl,FICOM,"ficom");
							else
								chgop(pl,FICOML,"ficoml");
							break;
				default:
					return false; /* not a bug: it's the last peep */
			}/*end switch*/
			pl->op1 = pf->op1;
			pl->op2 = NULL;
			DELNODE(pf);
			return true;
		}/*second operand*/
	}/*end un risk fp*/
	return false; /* made no opts. */

}
 

static boolean					/* true if changes made */
postw3opt(pf,pl)
register NODE * pf;			/* pointer to first inst. in window */
register NODE * pl;			/* pointer to last inst. in window */
{
    register NODE * pm = pf->forw;	/* point at middle node */
    int cop1 = pf->op;			/* op code number of first */
    int cop2 = pm->op;			/* op code number of second */
    int cop3 = pl->op;			/* op code number of third */
    int src1, src2;			/* size (bytes) of source of move */
    int dst1, dst2;			/* size of destination of move */
    int m1,m2;

/*Verify that all three instructions are doomed to rum alone
** in the U pipe. */
	if ((!(target_cpu & (P5|blend)))
	|| (pf->usage != ALONE
	|| pm->usage != ALONE || pl->usage != ALONE))
		return (false);
/*Unrisc FP instructions - the case for FCOM is with three instructions: 
**FILD
**FXCH -> FICOM
**FCOM
*/
	if ((cop1 == FILD || cop1 == FILDL)
	 && cop2 == FXCH
	 && cop3 == FCOM
	 ) {
		if (OpLength(pf) == WoRD)
			chgop(pf,FICOM,"ficom");
		else
			chgop(pf,FICOML,"ficoml");
		DELNODE(pm);
		DELNODE(pl);
		return true;
	 }
/* op= improvement
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op
** instruction sequence.  An example:
**
**	movX	R1,R2
**	addX	O3,R2		->	addX O3,R1
**	movX	R2,R1
**
** This is useful if R2 is then dead afterwards.
*/

    if (
	    ismove(pf,&src1,&dst1)
	&&  (  cop2 == ADDB || cop2 == ADDW || cop2 == ADDL
	    || cop2 == ANDB || cop2 == ANDW || cop2 == ANDL
	    || cop2 == XORB || cop2 == XORW || cop2 == XORL
	    || cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL
	    || cop2 == MULB || cop2 == MULW || cop2 == MULL
	    || cop2 == IMULB || cop2 == IMULW || cop2 == IMULL
	    || cop2 == SUBB || cop2 == SUBW || cop2 == SUBL
	    )
	&&  ismove(pl,&src2,&dst2)
	&&  isreg(pf->op1)
	&&  isreg(pf->op2)
	&&  isreg(pm->op2)
	&&  isreg(pl->op1)
	&&  isreg(pl->op2)
	&&  strcmp(pf->op2,pm->op2) == 0
	&&  strcmp(pm->op2,pl->op1) == 0
	&&  strcmp(pf->op1,pl->op2) == 0
	&&  isdead(pm->op2,pl)
	&&  pm->op3 == NULL
	)
    {
	wchange();		/* change the window */
	lmrgin1(pf,pm,pm);	/* preserve line number info */
	pm->op2 = pf->op1;	/* Move fields around */
	makelive(pm->op2,pm);
	DELNODE(pf);
	DELNODE(pl);
	return(true);
    }
/* op= improvement
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	movX	O1,R2
**	addX	R1,R2		->	addX O1,R1
**	movX	R2,O2		->	movX R1,O2
**
**	if O1, and O2 are not registers.
**
** Note that this transformation is is only correct for
** commutative operators.
**
** O2 does not use R1 as base / index.
*/
    if (
	    ismove(pf,&src1,&dst1)
	&&  (  cop2 == ADDB || cop2 == ADDW || cop2 == ADDL
	    || cop2 == ANDB || cop2 == ANDW || cop2 == ANDL
	    || cop2 == XORB || cop2 == XORW || cop2 == XORL
	    || cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL
	    || cop2 == MULB || cop2 == MULW || cop2 == MULL
	    || cop2 == IMULB || cop2 == IMULW || cop2 == IMULL
	    )
	&&  ismove(pl,&src2,&dst2)
	&&  isreg(pf->op2)
	&&  isreg(pm->op1)
	&&  isreg(pm->op2)
	&&  isreg(pl->op1)
	&&  src1 == src2
	&&  dst1 == dst2
	&&  src1 == dst1
	&&  strcmp(pf->op2,pm->op2) == 0
	&&  strcmp(pm->op2,pl->op1) == 0
	&&  strcmp(pm->op1,pm->op2) != 0
	&&  isdead(pm->op2,pl)
	&&  pm->op3 == NULL
	&&	!(scanreg(pl->op2,0) & setreg(pm->op1))
/*	&&  ( scanreg(pm->op1, false) & (EAX|EDX|ECX) ) */
	)
    if  (isdead(pm->op1,pl) || strcmp(pm->op1,pl->op2) == 0)
    {
	wchange();		/* change the window */
	lmrgin1(pf,pm,pm);	/* preserve line number info */
	pl->op1 = pm->op1;
	pm->op2 = pm->op1;	/* Move fields around */
	pm->op1 = pf->op1;
	makelive(pm->op2,pm);
	DELNODE(pf);
	return(true);
    }

/* Convert dependency to anti dependency 
**
** set R1      ==>   set R2 
** mov R1,R2   ==>   mov R2,R1 
** use R2      ==>   use R2 
**
** The use is with or without set 
*/ 
  if ( ((m1=1,(isreg(pf->op1) && (setreg(pf->op1) & sets(pf)))) || 
        (m1=2,(isreg(pf->op2) && (setreg(pf->op2) & sets(pf)))) ) 
   && !(setreg(pf->ops[m1]) & uses(pf))
   && ((cop2 ==MOVL) || (cop2 == MOVW) || (cop2 == MOVB))
   && isreg(pm->op1) 
   && isreg(pm->op2) 
   && !strcmp(pm->op1,pf->ops[m1]) 
   && !samereg(pm->op1,pm->op2)
   && ((m2=1,(isreg(pl->op1) && (setreg(pl->op1) & uses(pl)))) || 
       (m2=2,(isreg(pl->op2) && (setreg(pl->op2) & uses(pl)))) ) 
   && !strcmp(pm->op2,pl->ops[m2]) 
   ) 
   { 
     if ((cop1 == XORL) || (cop1 == XORW) || (cop1 == XORB))
     	pf->op1 = pf->op2 = pm->op2;
     else   
     	pf->ops[m1] = pm->op2;
     pm->op2 = pm->op1;
     pm->op1 = pf->ops[m1];
     pf->nlive |= setreg(pf->ops[m1]);
     return true;
   } 

/* op= improvement (For inc/dec only
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	movX	O1,R1
**	incX	R1		->	incX O1
**	movX	R1,O1
**
*/

	if (    ismove(pf,&src1,&dst1)
		&& (   cop2 == INCB || cop2 == INCW || cop2 == INCL
			|| cop2 == DECB || cop2 == DECW || cop2 == DECL
			|| cop2 == NEGB || cop2 == NEGW || cop2 == NEGL
			|| cop2 == NOTB || cop2 == NOTW || cop2 == NOTL)
		&&  ismove(pl,&src2,&dst2)
		&&  isreg(pf->op2)
		&&  isreg(pm->op1)
		&&  isreg(pl->op1)
		&&  strcmp(pf->op2,pm->op1) == 0
		&&  strcmp(pm->op1,pl->op1) == 0
		&&  strcmp(pf->op1,pl->op2) == 0
		&&  isdead(pm->op1,pl)
		&&  !isvolatile(pl,2)		/* non-volatile */
	   )
    {
		wchange();		/* change the window */
		lmrgin1(pf,pm,pm);	/* preserve line number info */
		pm->op1 = pf->op1;	/* Move fields around */
		makelive(pm->op1,pm);
		DELNODE(pf);
		DELNODE(pl);
		return(true);
    }

	return (false);
}


static int
postw3agr(pf,pl) NODE *pf,*pl;
{
NODE *pm = pf->forw;
int cop2 = pm->op;
int src1,src2,dst1,dst2;

	if (cop2 == NOP 
	 && (sets(pf) & uses(pl))
	) {
		wchange();
		DELNODE(pm);
		return true;
	}
/* op= improvement (For inc/dec only
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	movX	O1,R1
**	incX	R1		->	incX O1
**	movX	R1,O1
**
*/
    if (
		pf->usage == ALONE
	&&  pm->usage == ALONE
	&&  ismove(pf,&src1,&dst1)
	&&  (  cop2 == INCB || cop2 == INCW || cop2 == INCL
	    || cop2 == DECB || cop2 == DECW || cop2 == DECL
	    || cop2 == NEGB || cop2 == NEGW || cop2 == NEGL
	    || cop2 == NOTB || cop2 == NOTW || cop2 == NOTL
	    )
	&&  ismove(pl,&src2,&dst2)
	&&  isreg(pf->op2)
	&&  isreg(pm->op1)
	&&  isreg(pl->op1)
	&&  strcmp(pf->op2,pm->op1) == 0
	&&  strcmp(pm->op1,pl->op1) == 0
	&&  strcmp(pf->op1,pl->op2) == 0
	&&  isdead(pm->op1,pl)
	&&  !isvolatile(pl,2)		/* non-volatile */
	)
    {
		wchange();		/* change the window */
		lmrgin1(pf,pm,pm);	/* preserve line number info */
		pm->op1 = pf->op1;	/* Move fields around */
		makelive(pm->op1,pm);
		DELNODE(pf);
		DELNODE(pl);
		return(true);
    }
	return false;
}

static int
postw2agr(pf,pl) NODE *pf,*pl;
{
int cop1 = pf->op;
	if (cop1 == NOP && pl->usage == ALONE) 
	{
		wchange();
		DELNODE(pf);
		return true;
	}
	return false;
}/*end postw2agr*/


static boolean					/* true if changes made */
postw1_5opt(pf,pl)
NODE * pf;			/* first instruction node of window */
NODE * pl;
{

/* subl $8, %esp
 * subl $8, %esp  ==> sub $16,%esp
 * 
 * This coge appears after scheduler has moved instructions.
 */

	if ( pf->op == SUBL && pl->op == SUBL 
	&& isimmed(pf->op1) && isimmed(pl->op1)
	&& (!strcmp(pf->op2, pl->op2))
	){
		int n=(int)strtoul(pf->op1+1,(char **)NULL,0);
		int m=(int)strtoul(pl->op1+1,(char **)NULL,0);
		wchange();
		pf->op1=getspace(ADDLSIZE);
		sprintf(pf->op1,"$%d",n+m);
		DELNODE(pl);
		return true;
	}
	return false;
}


static boolean					/* true if changes made */
postw1opt(pf,pl)
register NODE * pf;			/* first instruction node of window */
{
    int m;				/* general integer temporary */
    unsigned free_reg, free_reg_=0;
    unsigned ocode;
    char *omnemo,*rmnemo,*rmnemo_=NULL;
	NODE *p;
	    



 /* eliminate agi  
**   
**	addl	$d,%esp
**			
** to:
**
**	popl	%ecx
**[	popl	%ecx ]
**	
** 
*/
#ifdef DEBUG
if (getenv("no_postw1opt")) return false;
#endif

	if( (pf->op == ADDL || pf->op == SUBL) && isimmed(pf->op1) 
	&& samereg(pf->op2,"%esp") 
	&& ( !(pf->nlive & ECX) || !(pf->nlive & EDX) )
	&& ((m=(int)strtoul(pf->op1+1,(char **)NULL,0)) == 8  || m==4) ){
		int cnt;
		if (pf->usage != ON_X ) cnt=1; else cnt=0;
		for (p=pf->forw; cnt < 4 ;cnt++){
			if ( (p->usage != ON_X) && (p->usage != ON_J) ) cnt++;
			if (indexing(p) & ESP || p->op == RET || p->op==CALL ) break;
			if (isbr(p))  p=find_label(p) ; else p=p->forw;
		}
		/* if cnt < 4 there is AGI here */ 
		if (cnt >= 4) return false ; /* nothing to do */	
		if (!(pf->nlive & EDX)){ 
			free_reg=EDX;
			rmnemo="%edx";
			if ( !(pf->nlive & ECX ) ){
				free_reg_=ECX; 
				rmnemo_="%ecx";
			}	
		}else{
			free_reg=ECX; 
			rmnemo="%ecx";
		}	
		
		if(pf->op == ADDL) {
			ocode = POPL;
			omnemo="popl";
		}else{
			ocode = PUSHL ; 
			omnemo="pushl";
			free_reg_=free_reg;
			rmnemo_=rmnemo;
		}
				 
	    if ((target_cpu & (P5 | blend)) 
	    || (target_cpu == P4 &&  m==4)) {    	  
	    	if ( ocode == POPL && free_reg_ == 0 && m == 8 ) return false ;
	    	/* Double pop into the same reg */
	 		chgop(pf,ocode,omnemo);
			pf->op1 = rmnemo;
			pf->op2 = NULL;
			pf->sets=ESP|free_reg;
			pf->usage = ON_X;
			if(m==8) {	
				NODE *pnew;
				wchange();
				pnew = insert(pf);
				chgop(pnew,ocode,omnemo);
				pnew->op1 = rmnemo_;
				pnew->op2 = NULL;
				pnew->nlive = pf->nlive;
				pnew->sets = ESP|free_reg_;
				pnew->uses = ESP;
				pnew->usage = ON_J;
			}
	    	return true;		
	  	}	
	}
	return false;
}

void backalign()
{
 BLOCK *b;
 NODE *p,*new;
	COND_RETURN("backalign");
	for (b = b0.next ; b ; b = b->next) {
		if (lookup_regals(b->firstn->opcode, first_label)) {
			for (p = b->firstn->back; p != b0.next->firstn ; p = p->back) {
				if (   isbr(p)
					|| islabel(p)
					|| (uses(p) & CONCODES_AND_CARRY))
					break;
				if (   p->usage == LOST_CYCLE
					&& ! isprefix(p)) {
					COND_SKIP(break,"%d %s ",second_idx,b->firstn->opcode,0);
					new = Saveop(0,"",0,GHOST);/* create isolated node, init. */
					APPNODE(new, p);		/* append new node after current */
					new->opcode = newlab();
					b->firstn->op1 = new->opcode;
					new->op = LABEL;
					break;
				}
			}
		}
	}/*for all blocks*/
}/*end backalign*/

/* In loops we should let loop's control (mainly the jump)
execute concurrently with floating point operations of the loop.
L:                              jmp   L:
.                           S1:
.                               fstp   O2
.                           L:
.                              ...
	fXXXl   O1                  fXXXl   O1                
	fstp    O2         =>       jne		S1:	
	jne		L:	                fstpl   O2

*/
void
sw_pipeline()
{
 BLOCK *b;
 NODE *p,*q,*new;
 unsigned count;

#ifdef FLIRT
 static int numexec=0;
#endif

COND_RETURN("sw_pipeline");

#ifdef FLIRT
	++numexec;
#endif

	bldgr(false);

	for (b = b0.next ; b ; b = b->next) {
		p = b->lastn->back;
		if (FP(p->op) 
		  && ((! CONCURRENT(p->op)) || (target_cpu == P5 || p->usage == LOST_CYCLE ))
		  && ( b->nextr  && b->nextr->firstn->op == LABEL  )
		  && ( strcmp(p->forw->op1,b->nextr->firstn->opcode) == 0 )
		  && lookup_regals(b->nextr->firstn->opcode, first_label)
		  ) {	
			if (target_cpu != P5) { /* Check if concurrency gain */
				count = 0;
				for (q = p->back ; q != b->firstn && !FP(q->op) ; q = q->back)
					count += get_exe_time(q);
				if ((! FP(q->op)) || (CONCURRENT(q->op) < (count + 3)))
					continue;
			}
			COND_SKIP(break,"%d %s ",second_idx,b->firstn->opcode,0);
			q = b->nextr->firstn;
			while (islabel(q))
				q = q->back;
			new = Saveop(0,"",0,GHOST);/* create isolated node, init. */
			APPNODE(new, q);		/* append new node after current */
			chgop(new,JMP,"jmp");
			new->op1 = p->forw->op1;
			kill_label(p->forw->op1);
			q = new;
			new = Saveop(0,"",0,GHOST);/* create isolated node, init. */
			APPNODE(new, q);		/* append new node after current */
			new->opcode = newlab();
			new->op = LABEL;
			q = new;
			new = Saveop(0,"",0,GHOST);/* create isolated node, init. */
			APPNODE(new, q);		/* append new node after current */
			new->opcode = p->opcode;
			new->op = p->op;
			new->op1 = p->op1; 
			new->op2 = p->op2; 
			p->forw->op1 = q->opcode;
			q = p->forw;
			DELNODE(p);
			APPNODE(p,q);		/* append new node after current */
			FLiRT(O_SW_PIPELINE,numexec);
		}
	}/*for loop*/
}/*end sw_pipeline*/
