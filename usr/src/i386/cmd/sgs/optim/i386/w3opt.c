#ident	"@(#)optim:i386/w3opt.c	1.1.3.47"
/* w3opt.c
**
**	Intel 386 three-instruction window improver
**
**
**
** This module contains improvements for three instruction windows,
** of which there aren't many.
*/

#include "sched.h" /*includes optim.h includes defs.h */
#include "optutil.h"
#include <stdio.h>
#include <limits.h>
extern int jflag;
extern int first_run;
extern int ieee_flag;
extern struct RI *reginfo;

extern char *itohalfreg();
extern char *itoqreg();
boolean ispwrof2();

/* w3opt -- 3-instruction peephole window */

boolean					/* true if changes made */
w3opt(register NODE * pf, register NODE * pl) {
/*
** pf -  pointer to first inst. in window
** pl -  pointer to last inst. in window
*/
    register NODE * pm = pf->forw;	/* point at middle node */
    char *ptr;
    int numbytes, numwords;
    int constant = 0, ilog;
    int srcsize = 0;
    int targsize = 0;
    char *temp = 0;
    unsigned int tmpi;

    int cop1 = pf->op;			/* op code number of first inst. */
    int cop2 = pm->op;			/* op code number of second */
    int cop3 = pl->op;			/* ... of third */

    int src1, src2;			/* size (bytes) of source of move */
    int dst1, dst2;			/* size of destination of move */
    int m;

/*
** Taken from w2opt, to limit it to jcc's which dont use CF and OF.
** Eliminate compares against zero if preceded by instruction that
** sets result indicators.  Because some Intel 386 instructions do not
** set all result indicators, we can only discard compares after
** those instructions that set all of the relevant ones.  w3opt
** takes care of those which set a limited range of result indicators,
** but which are followed by a suitable conditional jump.
**
** None of the arithmetic operations set the "unsigned" result indicators,
** so we retain compares before any unsigned-conditional jump.
**
**	op O1,O2                  	->	op O1,O2
**	cmpX $0,O2 (or TESTx  R,R)
**	jcc                       	  	jcc
**
**/
	if (   (   (   (cop2 == CMPL || cop2 == CMPW || cop2 == CMPB)
	            &&  strcmp(pm->op1,"$0") == 0)
	        || (   (cop2 == TESTL || cop2 == TESTW || cop2 == TESTB)
	            &&  samereg(pm->op1,pm->op2)))
	    && ! undefinedCC(pf)
	    && strcmp(dst(pf),pm->op2) == 0
  	    && (   ((tmpi = (uses(pl) & CONCODES_AND_CARRY)) != 0)	/* uses CC's */
	    	&& ((sets(pf) & tmpi) == tmpi))	/* pf sets all CC's needed */
	    && ! TEST_OF_or_CF(cop3)
	    && OpLength(pf) == OpLength(pm)
	    && !isvolatile(pm,2)		/* non-volatile */
	    && ! (pl->nlive & CONCODES_AND_CARRY)	/* CC's not needed beyond */
							) {

		wchange();		/* changing window */
		lmrgin3(pf,pm,pf);	/* preserve line number info */ 
		mvlivecc(pm);		/* preserve condition code info */
		DELNODE(pm);		/* delete the compare */
		return(true);		/* announce success */
	}

/*
** movswl r16,%eax -> movw r16,%ax
** cltd            -> cwtd
** idivl %ecx      -> idivw %cx
*/
	if (   !eiger
	    && cop1 == MOVSWL
	    && cop2 == CLTD
	    && cop3 == IDIVL
	    && !strcmp(pf->op2,"%eax")) {

		extern char *itoreg();

		wchange();
		if (!(pl->nlive & (EAX & ~ AX))) {
			chgop(pf,MOVW,"movw");
			pf->op2 = "%ax";
		}
		chgop(pm,CWTD,"cwtd");
		chgop(pl,IDIVW,"idivw");

		/* if it is mem, the operand looks the same, just make a 16
		   bit access*/
		if (isreg(pl->op1)) {
			pl->op1 = itoreg(L2W(setreg(pl->op1)));
		}
		return true;
	}

/*
**	fstsw %ax      ->  fnstsw %ax
**	sahf           ->  testl $c,%eax
**	jcc            ->  je / jne
**
**  The constant c depends on the type of the jcc.
*/
	if (   !eiger
	    && cop1 == FSTSW
	    && cop2 == SAHF
	    && target_cpu != P6
	    && iscbr(pl)) {

		char *bit_pos;
		unsigned int jcc;
		char *jcc_opc;

		switch (pl->op) {
			default:
				goto peep_out1;

			case JA:
			case JNBE:
				bit_pos = "$16640";
				jcc = JE;
				jcc_opc = "je";
				break;

			case JAE:
			case JNB:
				bit_pos = "$256";
				jcc = JE;
				jcc_opc = "je";
				break;

			case JB:
			case JNAE:
				bit_pos = "$256";
				jcc = JNE;
				jcc_opc = "jne";
				break;

			case JBE:
			case JNA:
				bit_pos = "$16640";
				jcc = JNE;
				jcc_opc = "jne";
				break;

			case JE:
			case JZ:
				bit_pos = "$16384";
				jcc = JNE;
				jcc_opc = "jne";
				break;

			case JNZ:
			case JNE:
				bit_pos = "$16384";
				jcc = JE;
				jcc_opc = "je";
				break;

		}
		wchange();
		chgop(pf,FNSTSW,"fnstsw");
		chgop(pm,TESTL,"testl");
		pm->op1 = bit_pos;
		pm->op2 = "%eax";
		chgop(pl,jcc,jcc_opc);
		return true;
	}

peep_out1:

/*
** case: redundant movzwl
**
**  mov[sz]wl  O1,R
**  cmpl       $imm,R  -> cmpw $imm, O1
*/
	if (   !eiger
	    && (cop1 == MOVZWL)
	    && (cop2 == CMPL)
	    && isreg(pf->op2)
	    && isreg(pm->op2)
	    && isdead(pm->op2,pm)
	    && strcmp(pf->op2,pm->op2) == 0
	    && *pm->op1 == '$'
	    && isdigit(pm->op1[1])
	    && isbr(pl)
	    && !TEST_SF(pl->op))  {

		int imm;

		imm = (int)strtoul(pm->op1 +1,NULL,10);
		if (imm < 0x8000) {
			wchange();		/* change window */
			lmrgin3(pf,pm,pf);	/* preserve line number info */
			mvlivecc(pf);		/* preserve CCs live info */
			chgop(pm,CMPW,"cmpw");
			pm->op2 = pf->op1;
			if (isvolatile(pf,1)) {
				/* propagate volatile info */
				mark_vol_opnd(pm,2);
			}
			DELNODE(pf);
			return true;
		}
	}

/*
** case: redundant movzwl
**
**  mov[sz]wl  O1,R
**  cmpl       $imm,R  -> cmpw $imm, O1
**  jcc                -> juscc          ( a signed jcc becomes an unsigned jcc)
*/
	if (   (cop1 == MOVZWL /* || cop1 == MOVSWL */ )
	    && (cop2 == CMPL || cop2 == TESTL)
	    && isreg(pf->op2)
	    && isreg(pm->op2)
	    && isdead(pm->op2,pm)
	    && strcmp(pf->op2,pm->op2) == 0
	    && *pm->op1 == '$'
	    && is_unsable_br(pl))  {

		int imm;

		imm = (int)strtoul(pm->op1 +1,NULL,10);
		if (imm >= 0 && imm < 0x8000) {
			wchange();		/* change window */
			lmrgin3(pf,pm,pf);	/* preserve line number info */ 
			mvlivecc(pf);		/* preserve CC's live info */
			if (cop2 == CMPL) {
				chgop(pm,CMPW,"cmpw");
			} else {
				chgop(pm,TESTW,"testw");
			}
			pm->op2 = pf->op1;
	   		if (isvolatile(pf,1)) {
				/* propagete volatile info */
				mark_vol_opnd(pm,2);
			}
			DELNODE(pf);
			sign2unsign(pl);
			return true;
		}
	}

	if (   (cop1 == MOVZBL /* || cop1 == MOVSBL */ )
	    && (cop2 == CMPL || cop2 == TESTL)
	    && isreg(pf->op2)
	    && isreg(pm->op2)
	    && isdead(pm->op2,pm)
	    && strcmp(pf->op2,pm->op2) == 0
	    && *pm->op1 == '$'
	    && is_unsable_br(pl))  {

		int imm;

		imm = (int)strtoul(pm->op1 +1,NULL,10);
		if (imm >= 0 && imm < 0x80) {
			wchange();		/* change window */
			lmrgin3(pf,pm,pf);	/* preserve line number info */ 
			mvlivecc(pf);		/* preserve CC's live info */
			if (cop2 == CMPL) {
				chgop(pm,CMPB,"cmpb");
			} else {
				chgop(pm,TESTB,"testb");
			}
			pm->op2 = pf->op1;
			if (isvolatile(pf,1)) {
				/* propagete volatile info */
				mark_vol_opnd(pm,2);
			}
			DELNODE(pf);
			sign2unsign(pl);
			return true;
		}
	}

/*
** block move improvement
**
** This improvement looks for a block move done a byte at a time, and
** attempts to do this with as many 4-byte blocks as possible, followed
** by 0 or 1 half-word moves and 0 or 1 byte moves.
*/

	if (   cop1 == MOVL
	    && isnumlit(pf->op1)
	    && scanreg(pf->op2, false) == ECX
	    && cop2 == REP
	    && cop3 == SMOVB) {

		numbytes = (int)strtoul(pf->op1+1,(char **)NULL,0);
		if ((numwords = numbytes / 4) != 0) {
			wchange();		/* change the window */
			pf->op1 = getspace(sizeof("$2147483647"));
			sprintf(pf->op1, "$%d", numwords);
			if (numwords * 4 == numbytes) {
				chgop(pl, SMOVL, "smovl");
			} else {
				NODE *psmov = Saveop(0,"",0,GHOST);
				INSNODE(psmov, pl);
				psmov->nlive = pl->nlive;
				psmov->ndead = pl->ndead;
				makelive("%esi", psmov);
				makelive("%edi", psmov);
				chgop(psmov, SMOVL, "smovl");
				if (numwords * 4 + 2 >= numbytes) {
					if (numbytes % 2) {
						chgop(pl, SMOVB, "smovb");
					} else {
						chgop(pl, SMOVW, "smovw");
					}
				} else {
					NODE *psmov1 = Saveop(0,"",0,GHOST);
					INSNODE(psmov1, pl);
					psmov1->nlive = pl->nlive;
					psmov1->ndead = pl->ndead;
					makelive("%esi", psmov1);
					makelive("%edi", psmov1);
					chgop(psmov1, SMOVW, "smovw");
					chgop(pl, SMOVB, "smovb");
				}
			}
			return true;
		}
	}
/*
**
** 
** 
** 
*/
	if (   cop1 == MOVL
	    && cop2 == CMPL
	    && isbr(pl)
	    && isnumlit(pf->op1)
	    && isnumlit(pm->op1)) { 

		char *tmp;
		if (   !strcmp(pf->op2,pm->op2)
		    && !isvolatile(pf,2) 
		    && !isvolatile(pm,2) ) {
			tmp = pm->op2;
			pm->op2 = pf->op1;
			if (cmp_imm_imm(pm,pl)) {
				wchange();
				return true;
			} else {
				/* didn't work, retrieve the inst. */
				pm->op2 = tmp;
			}
		} else if (!strcmp(pf->op2,pm->op1)) {
			tmp = pm->op1;
			pm->op1 = pf->op1;
			if (cmp_imm_imm(pm,pl)) {
				wchange();
				return true;
			} else {
				pm->op1 = tmp;
			}
		}
	 }

/*
**  dec / sub / inc / add
**  test         ->  delete
**  jge          ->  jns
**
**  jge is taken if sf == of. test sets of to 0. hence it's like a 
**  constant propagation on the flag.
*/
	if (   cop3 == JGE
	    && cop2 == TESTL
	    && isreg(pm->op2)
	    && isreg(pm->op1)
	    && samereg(pm->op1,pm->op2)
	    && (   (m = 1, (cop1 == DECL || cop1 == INCL))
	        || (m = 2, (cop1 == SUBL || cop1 == ADDL)))
	    && isreg(pf->ops[m])
	    && samereg(pf->ops[m],pm->op2)) {

		wchange();
		DELNODE(pm);
		chgop(pl,JNS,"jns");
		return true;
	  }

/*
** Get move of a power of 2 into %ecx next to cltd instruction in order
** to set up peephole for signed division.
*/

	if (   cop1 == MOVL
	    && cop3 == CLTD
	    && ismove(pm,&src1,&dst1)
	    && isnumlit(pf->op1)
	    && ispwrof2((int)strtoul(pf->op1+1,(char **)NULL,0), &ilog)
	    && !strcmp(pf->op2, "%ecx")
	    && !(   usesreg(pm->op1, pf->op2)
	         || usesreg(pm->op2, pf->op2))) {

		wchange();
		lexchin(pf,pm);		/* preserve line number info */ 
		exchange(pf);		/* interchange instructions */
		swplivecc(pf,pm);	/* switch the live/dead info on
					   condition codes */
		makedead(pf->op2, pm);
		if (isreg(pm->op2)) {
			makelive(pm->op2, pf);
		}
		return true;
   }

/*
** integer division improvement
**
** This improvement looks for integer division done by either 2, 4, 8 ....
**   
** For div with N = 2:
**
**    movl    $2,R1        ->    cltd
**    cltd                       subl    %edx,%eax
**    idiv    R1                 sarl    $1,%eax
**
**For div with N > 2:
**
**    movl    $N,R1   / (N == 2**N1) ->   movl    %eax,%edx
**    cltd                                sarl    $31,%edx
**    idiv    R1                          andl    $N-1,%edx
**                                        addl    %edx,%eax
**                                        sarl    $N1,%eax
**
** For mod with N = 2:
**
**    movl    $2,R1                   ->  movl    %eax,%edx
**    cltd                                andl    $1,%edx
**    idiv    R1                          testl   %eax,%eax
**                                        jge     ..0
**                                        neg     %edx
**                                    ..0:
**
**For mod with N > 2:
**
**    movl    $N,R1 / (N == 2**N1)  ->    movl    %eax,%edx
**    cltd                                andl    $(N-1),%edx
**    idiv    R1                          testl   %eax,%eax
**                                        jge     ..0
**                                        testl   %edx,%edx
**                                        je      ..0
**                                        subl    $N,%edx
**                                    ..0:
**
*/

	if (   cop1 == MOVL
	    && cop2 == CLTD
	    && cop3 == IDIVL
	    && isnumlit(pf->op1)
	    && ispwrof2(constant = (int)strtoul(pf->op1+1,(char **)NULL,0), &ilog)
	    && !strcmp(pf->op2, pl->op1)) {

		if (   (! isreg(pf->op2))
		    || (! isdead(pf->op2, pl))) {
			NODE * pnew, *prepend();
			pnew = prepend(pf,pf->op2);
			chgop(pnew, MOVL, "movl");
			pnew->op1 = pf->op1;
			pnew->op2 = pf->op2;
			lexchin(pf,pnew);	/* preserve line number info */ 
		}
		if (isdead("%edx", pl)) {
			if (constant == 2) {
				chgop(pf, CLTD, "cltd");
				pf->op1 = pf->op2 = NULL;
				makelive(pm->op1 = "%edx", pf);
				chgop(pm, SUBL, "subl");
				pm->op2 = pl->op2 = "%eax";
				chgop(pl, SARL, "sarl");
				pl->op1 = "$1";
			} else {
				NODE *pn;

				pf->op1 = "%eax";
				makelive(pf->op2 = "%edx", pf);

				chgop(pm, SARL, "sarl");
				pm->op1 = "$31";
				pm->op2 = "%edx";
				chgop(pl, ANDL, "andl");
				pl->op1 = getspace(sizeof("$-2147483647"));
				sprintf(pl->op1, "$%d", constant -1);
				makelive(pl->op2 = "%edx", pl);

				pn = insert(pl);
				chgop(pn, ADDL, "addl");
				pn->op1 = "%edx";
				makelive(pn->op2 = "%eax", pn);

				pn = insert(pn);
				chgop(pn, SARL, "sarl");
				pn->op1 = getspace(sizeof("$32"));
				sprintf(pn->op1, "$%d", ilog);
				makelive(pn->op2 = "%eax", pn);
			}
			return true;

	    	} else if (isdead("%eax", pl)) {

			char *nlab = newlab();
    			NODE *pn, *new = Saveop(0,nlab,0,GHOST);
		
			pf->op1 = "%eax";
			makelive(pf->op2 = "%edx", pf);

			pm->op1 = getspace(sizeof("$2147483647"));
			sprintf(pm->op1, "$%d", constant - 1);
			chgop(pm, ANDL, "andl");
			pm->op2 = pf->op2;

			chgop(pl, TESTL, "testl");
			pl->op1 = pl->op2 = pf->op1;
			pl->nlive |= CONCODES_AND_CARRY;

			pn = insert(pl);
			chgop(pn, JGE, "jge"); pn->op1 = nlab;
			makelive(pf->op2, pn);

			if ( constant != 2 ) {

				pn = insert(pn);
				chgop(pn, TESTL, "testl");
				pn->op1 = pn->op2 = pf->op2;
				makelive(pf->op2, pn);
				pn->nlive |= CONCODES_AND_CARRY;

				pn = insert(pn);
				chgop(pn, JE, "je");
				pn->op1 = nlab;
				makelive(pf->op2, pn);

				pn = insert(pn);
				pn->op1 = getspace(sizeof("$2147483647"));
				sprintf(pn->op1, "$%d", constant);
				chgop(pn, SUBL, "subl");
				pn->op2 = pf->op2;
				makelive(pf->op2, pn);
			} else {
				pn = insert(pn);
				chgop(pn, NEGL, "negl");
				pn->op1 = pf->op2;
				makelive(pf->op2, pn);
			}
    			APPNODE(new, pn);
			setlab(new);
			makelive(pf->op2, new);

			return true;
		}
	}
		
/*
** This transformation is for the following code, which
** can be a common sequence from the rcc compiler.
** The instruction sequence that is sought after is:
**
** 	fstpl	X(ebp)	\
** 	            		| fstl	X(ebp)
** 	fldl	Y(ebp)		| faddl	Y(ebp)
** 	faddl	X(ebp)	/
**
** All of the operations may be converted, because there
** are divrl, and subrl instructions non-communitive operators.
*/

	if (   !ieee_flag
	    && cop1 == FSTPL
	    && cop2 == FLDL
	    && !isvolatile(pf,1)
	    && (   cop3 == FADDL || cop3 == FSUBL || cop3 == FSUBRL
	        || cop3 == FMULL || cop3 == FDIVL || cop3 == FDIVRL )
	    && pl->op1
	    && strcmp(pf->op1,pl->op1) == 0) {

		wchange();		/* change the window */
		lmrgin1(pl,pf,pf);	/* preserve line number info */
		pl->op1 = pm->op1;
		DELNODE(pm);		/* delete the extraneous moves */
		chgop (pf, FSTL, "fstl");
		if ( cop3 == FSUBL ) {
			chgop (pl, FSUBRL, "fsubrl");
		}
		if ( cop3 == FDIVL ) {
			chgop (pl, FDIVRL, "fdivrl");
		}
		if ( cop3 == FSUBRL ) {
			chgop (pl, FSUBL, "fsubl");
		}
		if ( cop3 == FDIVRL ) {
			chgop (pl, FDIVL, "fdivl");
		}
		return(true);
	}

/*
** op= improvement
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

	if (   (   target_cpu == P4
	        || jflag
	        || OpLength(pm) == WoRD)
	    && ismove(pf,&src1,&dst1)
	    && (  cop2 == ADDB || cop2 == ADDW || cop2 == ADDL
	        || cop2 == ANDB || cop2 == ANDW || cop2 == ANDL
	        || cop2 == XORB || cop2 == XORW || cop2 == XORL
	        || cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL
	        || cop2 == MULB || cop2 == MULW || cop2 == MULL
	        || cop2 == IMULB || cop2 == IMULW || cop2 == IMULL
	        || cop2 == SUBB || cop2 == SUBW || cop2 == SUBL)
	    && ismove(pl,&src2,&dst2)
	    && isreg(pf->op1)
	    && isreg(pf->op2)
	    && isreg(pm->op2)
	    && isreg(pl->op1)
	    && isreg(pl->op2)
	    && strcmp(pf->op2,pm->op2) == 0
	    && strcmp(pm->op2,pl->op1) == 0
	    && strcmp(pf->op1,pl->op2) == 0
	    && isdead(pm->op2,pl)
	    && pm->op3 == NULL) {

		wchange();		/* change the window */
		lmrgin1(pf,pm,pm);	/* preserve line number info */
		if (samereg(pm->op1,pm->op2)) {
			pm->op1 = pf->op1;
		}
		pm->op2 = pf->op1;	/* Move fields around */
		makelive(pm->op2,pm);
		DELNODE(pf);
		DELNODE(pl);
		return(true);
	}

/*
** split in order to gain 2 cycles
**
**     movl      mem,R2            movl     mem,R1
**     movzbl    (R2),R2           xorl     R2,R2
**     incl      mem               movb     (R1),R2_low
**                                 incl     R1
**                                 movl     R1,mem
*/
	if (   (cop1 == MOVL )
	    && (cop2==MOVZBL)
	    && (cop3==INCL)
	    && isreg(pf->op2)
	    && isreg(pm->op2)
	    && (setreg(pf->op2)|EAX|EBX|ECX|EDX)
	    && (strcmp(pf->op2,pm->op2)==0)
	    && ismem(pf->op1)
	    && ismem(pm->op1)
	    && ismem(pl->op1)
	    && isdeadcarry(pl)
	    && !isvolatile(pf,1)
	    && (strcmp(pf->op1,pl->op1) == 0 )
	    && (scanreg(pm->op1,false) & setreg(pf->op2))
	    && ((tmpi = get_free_reg(pf->nlive | setreg(pf->op2),0,pf)) != -1)
								) {
		NODE * pnew1, *pnew2, *prepend();
		register char *reg1, *reg2;
		int regno;

		wchange();     /* print change in window */
		reg1 = reginfo[tmpi].regname[0];
		regno = reginfo[tmpi].reg_number;
		reg2 = pf->op2;
		pf->op2 = reg1;
		pf->nlive |= regno;
		pf->nlive &= ~(setreg(reg2));
		pnew1 = prepend(pm, reg2);
		pnew1->op1 = pnew1->op2 = reg2;
		chgop(pnew1, XORL, "xorl");
		chgop(pm,MOVB,"movb");
		sprintf(pm->op1,"(%s)",reg1);
		sprintf(pm->op2,"%%%cl",reg2[2]);
		pm->nlive |= regno;
		lexchin(pm,pnew1);		/* preserve line number info */ 
		pnew2 = prepend(pl,reg1);
		chgop(pnew2,INCL,"incl");
		pnew2->op1 = reg1;
		pnew2->op2 = NULL;
		/* live is the new register and any CONCODES (no CF)
		   used following the INCL */
		pnew2->nlive |= (regno | (pm->nlive & CONCODES));
		chgop(pl,MOVL,"movl");
		pl->op2 = pl->op1;
		pl->op1 = reg1;
		lexchin(pl,pnew2);		/* preserve line number info */ 
		return true;
	}   

/*
** op= improvement
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
*/
	if (   (   target_cpu == P4
	        || jflag
	        || OpLength(pm) == WoRD)
	    && ismove(pf,&src1,&dst1)
	    && (   cop2 == ADDB || cop2 == ADDW || cop2 == ADDL
	        || cop2 == ANDB || cop2 == ANDW || cop2 == ANDL
	        || cop2 == XORB || cop2 == XORW || cop2 == XORL
	        || cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL
	        || cop2 == MULB || cop2 == MULW || cop2 == MULL
	        || cop2 == IMULB || cop2 == IMULW || cop2 == IMULL)
	    && ismove(pl,&src2,&dst2)
	    && isreg(pf->op2)
	    && isreg(pm->op1)
	    && isreg(pm->op2)
	    && isreg(pl->op1)
	    && strcmp(pf->op2,pm->op2) == 0
	    && strcmp(pm->op2,pl->op1) == 0
	    && strcmp(pm->op1,pm->op2) != 0
	    && isdead(pm->op2,pl)
	    && pm->op3 == NULL
	    && ( scanreg(pm->op1, false) & (EAX|EDX|ECX) )
	    && src1 == src2
	    && dst1 == dst2
	    && src1 == dst1
	    && (   isdead(pm->op1,pl)
	        || strcmp(pm->op1,pl->op2) == 0 )
	    && ! (scanreg(pl->op2,true) & setreg(pm->op1))) {

		wchange();		/* change the window */
		lmrgin1(pf,pm,pm);	/* preserve line number info */
		pl->op1 = pm->op1;
		pm->op2 = pm->op1;	/* Move fields around */
		pm->op1 = pf->op1;
		makelive(pm->op2,pm);
		DELNODE(pf);
		return(true);
	}

/*
** op= improvement
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	movX	O1,R1		->	movX O1,R2
**	addX	O2,R1		->	addX O2,R2
**	movl	R1,R2
**
**	if R1 is dead after the sequence
**
*/

	if (   ismove(pf,&src1,&dst1)
	    && (   cop2 == ADDB || cop2 == ADDW || cop2 == ADDL
	        || cop2 == ANDB || cop2 == ANDW || cop2 == ANDL
	        || cop2 == XORB || cop2 == XORW || cop2 == XORL
	        || cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL
	        || cop2 == MULB || cop2 == MULW || cop2 == MULL
	        || cop2 == IMULB || cop2 == IMULW || cop2 == IMULL
	        || cop2 == SUBB || cop2 == SUBW || cop2 == SUBL)
	    &&  cop3 == MOVL
	    &&  isreg(pf->op2)
	    &&  isreg(pm->op2)
	    &&  isreg(pl->op1)
	    &&  isreg(pl->op2)
	    &&  strcmp(pf->op2,pm->op2) == 0
	    &&  strcmp(pm->op2,pl->op1) == 0
	    &&  isdead(pm->op2,pl)
	    &&  pm->op3 == NULL
	    &&  ! usesreg(pm->op1, pm->op2)
	    &&  ! usesreg(pm->op1, pl->op2)) {

		wchange();		/* change the window */
		lmrgin1(pm,pl,pm);	/* preserve line number info */
		pf->op2 = pl->op2;
		pm->op2 = pl->op2;	/* Move fields around */
		makelive(pf->op2,pf);
		makelive(pf->op2,pm);
		makedead(pl->op1,pf);
		makedead(pl->op1,pm);
		DELNODE(pl);
		return(true);
	}

/*
** op= improvement (For inc/dec only
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
	if (   (   target_cpu == P4
	        || jflag
	        || OpLength(pm) == WoRD)

	    && ismove(pf,&src1,&dst1)
	    && (   cop2 == INCB || cop2 == INCW || cop2 == INCL
	        || cop2 == DECB || cop2 == DECW || cop2 == DECL
	        || cop2 == NEGB || cop2 == NEGW || cop2 == NEGL
	        || cop2 == NOTB || cop2 == NOTW || cop2 == NOTL )
	    && ismove(pl,&src2,&dst2)
	    && isreg(pf->op2)
	    && isreg(pm->op1)
	    && isreg(pl->op1)
	    && strcmp(pf->op2,pm->op1) == 0
	    && strcmp(pm->op1,pl->op1) == 0
	    && strcmp(pf->op1,pl->op2) == 0
	    && isdead(pm->op1,pl)
	    && !isvolatile(pl,2)) {	/* non-volatile */

		wchange();		/* change the window */
		lmrgin1(pf,pm,pm);	/* preserve line number info */
		pm->op1 = pf->op1;	/* Move fields around */
		makelive(pm->op1,pm);
		DELNODE(pf);
		DELNODE(pl);
		return(true);
	}

/*
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	        O1=  	R  M	  	            R  M
**
**	movX	R1,R2	2  2		            0  0
**	addX	R3,R2	2  2	->	movX R1,O1  2  2
**	movX	R2,O1	2  2	->	addX R3,O1  2  7*
**
**	if O1 is not a register don't do it.
**
** Note that this transformation is is only correct for
** commutative operators.
*/

	if (   ismove(pf,&src1,&dst1)
	    && (   cop2 == ADDB || cop2 == ADDW || cop2 == ADDL
	        || cop2 == ANDB || cop2 == ANDW || cop2 == ANDL
	        || cop2 == XORB || cop2 == XORW || cop2 == XORL
	        || cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL
	        || cop2 == MULB || cop2 == MULW || cop2 == MULL
	        || cop2 == IMULB || cop2 == IMULW || cop2 == IMULL)
	    && ismove(pl,&src2,&dst2)
	    && isreg(pf->op1)
	    && isreg(pf->op2)
	    && isreg(pm->op1)
	    && isreg(pm->op2)
	    && isreg(pl->op1)
	    && isreg(pl->op2)
	    && strcmp(pf->op2,pm->op2) == 0
	    && strcmp(pm->op2,pl->op1) == 0
	    && isdead(pm->op2,pl)
	    && src1 == src2
	    && dst1 == dst2
	    && src1 == dst1
	    && !samereg(pm->op1,pm->op2) /* Must not have R2 == R3 */) {

		if (   pm->op3 == NULL
		    && setreg(pm->op1) != setreg(pl->op2)) {

			wchange();		/* change the window */
			pf->op2 = pl->op2;	/* Move fields around */
			pm->op2 = pl->op2;
			makelive(pl->op2,pf);
			makelive(pl->op2,pm);
			DELNODE(pl);
			return(true);
		}
	}

/*
** *p++ improvement
**
**	movl	R1,R2
**	incl	R1		->	opX  O1,(R1)
**	opX 	O1,(R2)		->	incl R1
**
** or
**	movl	R1,R2
**	addl	$imm,R1		->	opX  O1,(R1)
**	opX 	O1,(R2)		->	addl $imm,R1
**
**	if R2 is dead after sequence
*/

	if (   cop1 == MOVL
	    && isdyadic(pl)
	    && isreg(pf->op1)
	    && (   (   cop2 == INCL
	            && isreg(pm->op1)
	            && strcmp(pf->op1,pm->op1)==0 )

	       	|| (   cop2 == ADDL
	            && isnumlit(pm->op1)
	            && isreg(pm->op2)
	            && strcmp(pf->op1,pm->op2)==0 ) )
	    && isreg(pf->op2)
	    && iszoffset(pl->op2,pf->op2)
	    && ! strcmp(pf->op1,pf->op2) == 0
	    && isdead(pf->op2,pl)
	    && isdeadcc( pl )
	    && ! usesreg(pl->op1,pf->op1)
	    && ! usesreg(pl->op1,pf->op2)) {

		wchange();			/* change the window */
		lexchin(pm,pl);		/* preserve line number info */
		ldelin(pf);
		pl->op2 = getspace(NEWSIZE);
		pl->op2[0] = '(';
		pl->op2[1] = '\0';
		strcat(pl->op2,pf->op1);
		strcat(pl->op2,")");
		makelive(pf->op1,pl);	/* "R1" is now live in "op" node */
		makedead(pf->op2,pm);	/* "R2" is now dead in "incl" node */
		exchange(pm);		/* exchange the last two nodes */
		swplivecc(pm,pl);       /* swap live/dead info on CC's codes */
		DELNODE(pf);		/* delete first node */
		return(true);
	}

/*
** *p++ improvement
**
**	movl	R1,R2
**	incl	R1		->	opX  (R1),O1
**	opX	(R2),O1		->	incl R1
**
** or
**	movl	R1,R2
**	addl	$imm,R1		->	opX  (R1),O1
**	opX	(R1),O1		->	addl $imm,R1
**
**	if R2 is dead after sequence
**	opX can also be a PUSHW or PUSHL, with no 2nd operand
*/

	if (   cop1 == MOVL
	    && isreg(pf->op1)
	    && (   (   cop2 == INCL
	            && isreg(pm->op1)
	            && strcmp(pf->op1,pm->op1)==0 )

	        || (   cop2 == ADDL
	            && isnumlit(pm->op1)
	            && isreg(pm->op2)
	            && strcmp(pf->op1,pm->op2)==0 ))
	    && isreg(pf->op2)
	    && (   (   isdyadic(pl)
	            && ! usesreg(pl->op2,pf->op1)
	            && ! usesreg(pl->op2,pf->op2) )
	        || (   cop3 == PUSHW
	            || cop3 == PUSHL ))
	    && iszoffset(pl->op1,pf->op2)
	    && ! strcmp(pf->op1,pf->op2) == 0
	    && isdead(pf->op2,pl)
	    && isdeadcc( pl )) {

		wchange();			/* change the window */
		lexchin(pm,pl);		/* preserve line number info */
		ldelin(pf);
		pl->op1 = getspace(NEWSIZE);
		pl->op1[0] = '(';
		pl->op1[1] = '\0';
		strcat(pl->op1,pf->op1);
		strcat(pl->op1,")");
		makelive(pf->op1,pl);	/* "R1" is now live in "op" node */
		makedead(pf->op2,pm);	/* "R2" is now dead in "incl" node */
		exchange(pm);		/* exchange the last two nodes */
		swplivecc(pm,pl);       /* swap live/dead info on CC's */
		DELNODE(pf);		/* delete first node */
		return(true);
	}

/*
** case: rewrite addition or subtraction of constant from a register
**
**	
**   movs[bw]l	R1,R2		->	mov[bw]	R1,R3
**   incl   	R2   		->	inc[bw]	R3
**   mov[bwl] 	R2,R3
**
**   movs[bw]l	R1,R2		->	mov[bw]	R1,R3
**   leal     	$n(R2),R2 	->	add[bw]	$n,R3
**   mov[bwl] 	R2,R3
** 
**
*/
	if (   (cop1==MOVSBL ? srcsize = 1 : cop1 == MOVSWL ? srcsize = 2 : 0)
	    && (cop3==MOVB ? targsize = 1 : cop3 == MOVW ?
				targsize = 2 : cop3 == MOVL ? targsize= 3 : 0)
	    && isreg(pf->op1)
	    && isreg(pf->op2)
	    && isreg(pl->op1)
	    && isreg(pl->op2)
	    && isdead(pf->op2,pl)
	    && (   ((cop2 == INCL) ? (constant = 1, temp = pm->op1)  : 0)
	        || ((cop2 == DECL ) ? (constant = -1, temp = pm->op1) : 0)
	        || (   cop2 == LEAL  
	            && isdeadcc(pm)
 	            && ((constant=strtoul(pm->op1,&ptr,0)) != 0)
	            && ptr[0] == '(' 
	            && ptr[5] == ')' 
	            && ( (strncmp(ptr+1,pm->op2,4) == 0) ? temp = pm->op2 : 0))
	        || (   cop2 == SUBL 
	            && pm->op1[0] == '$' 
	            && ((constant= (int)strtoul(pm->op1+1,(char **)NULL,0)) ?
						temp = pm->op2 : 0 ) != 0
	            && (constant == 1 ? constant = -1
					: (constant == -1 ? constant = 1: 1))
	            && isdeadcc(pm) ))
            && isreg(temp)
            && scanreg(temp,0)&(EAX|ECX|EDX)
	    && strcmp(pf->op2,temp) == 0
			/* check register match between second and third
			   instructions */
	    && (targsize == 3 ? strcmp(temp+2,pl->op1+2) 
			: targsize == 2 ? strcmp(temp+2,pl->op1+1) == 0 
				: temp[2]==pl->op1[1] && pl->op1[2] == 'l')) {

		/* Decided we would like to do the change, but let's make
		** one final check for a show stopper - 16 bit to 8 bit that
		** does not originate in EAX ECX EDX or EBX.
		*/

		int do_8_bit_move = false;
		unsigned int srcreg, targreg;

		if (srcsize == 2 && targsize == 1) {
			if ((srcreg = (scanreg(pf->op1, 0) & (AX|DX|CX|BX))) != 0) {
				/* Can use low-order byte register as the
				** source register to get the 8 bits needed
				**and use MOVB.
				*/
				do_8_bit_move = true;

			} else {
				/* Coming from other 16 bit register such as
				** %si.  If dest. (R3) is low-order byte of
				** EAX, ECX, EDX or EBX (%al, %cl, %dl, or
				** %bl) and corresponding high-order byte
				** register is not live, than can use MOVW.
				*/
				if (   ((targreg = (scanreg(pl->op2,0) & (AL|DL|CL|BL))) != 0)
				    && ! pf->nlive & (full_reg(targreg) & (AH|DH|CL|BL))) {
					/* OK to do this as 16 bit move */
					/* EMPTY */
					;
				} else {
					/* Cannot easily move 16 bit into 8
					** bit register; do not attempt
					** peep-hole.
					*/
					goto cannot_do_rewrite_add_sub_opt;

				}  /* if */
			}  /* if */
		}  /* if */
		wchange();
		makedead(pf->op2,pf);
		/* save target reg in case need to modify pf->op2 */
		pf->op2 = temp = pl->op2;
		makelive(pl->op2,pf);
		pm->nlive = pl->nlive;
		DELNODE(pl);
	
		/* source size is either 1 or 2 */
		if (targsize == 3) {
			/* The opcode should be kept as it is --> movs[bw]l */
			/* EMPTY */
			;
		} else if (srcsize == 2 && targsize == 2) {
			chgop(pf,MOVW,"movw");
		} else if (srcsize == 2 && targsize == 1) {
			/* Check whether 1st inst should move 16 bits
			** or 8 bits.
			*/
			if (do_8_bit_move) {
				chgop(pf,MOVB,"movb");
				pf->op1 = itoqreg(srcreg);
			} else {
				chgop(pf,MOVW,"movw");
				pf->op2 = itohalfreg(targreg);
			}  /* if */

		} else if (srcsize == 1 && targsize == 2) {
			chgop(pf,MOVSBW,"movsbw");
		} else if (srcsize == 1 && targsize == 1) {
			chgop(pf,MOVB,"movb");
		}

		if (constant == 1) {
			if (targsize == 1) {
				chgop(pm,INCB,"incb");
			} else if (targsize == 2)  {
				chgop(pm,INCW,"incw");
			} else {
				chgop(pm,INCL,"incl");
			}
			pm->op1 = temp;
			/* should be no second operand */
			pm->op2 = NULL;

		} else if (constant == -1) {
			if (targsize == 1) {
				chgop(pm,DECB,"decb");
			} else if (targsize == 2) {
				chgop(pm,DECW,"decw");
			} else {
				chgop(pm,DECL,"decl");
			}
			pm->op1 = temp;
			/* should be no second operand */
			pm->op2 = NULL;

		} else {
			if (targsize == 1) {
				constant &= UCHAR_MAX;		/* avoid assembler warnings */
				if (cop2 == LEAL) {
					chgop(pm,ADDB,"addb");
				} else {
					chgop(pm,SUBB,"subb");
				}
			} else if (targsize == 2) {
				constant &= USHRT_MAX;		/* avoid assembler warnings */
				if (cop2 == LEAL) {
					chgop(pm,ADDW,"addw");
				} else {
					chgop(pm,SUBW,"subw");
				}
			} else {
				if (cop2 == LEAL) {
					chgop(pm,ADDL,"addl");
				} else {
					chgop(pm,SUBL,"subl");
				}
			}
			pm->op2 = temp;
			pm->op1 = getspace(NEWSIZE);
			sprintf(pm->op1,"$%d", constant);
		}
		makelive(pl->op2,pl);
		pm->nlive = pl->nlive;
		return (true);
	}

cannot_do_rewrite_add_sub_opt:

/*
** *p++ improvement
**
** This improvement rearranges things to facilitate a later 2-instruction
** improvement.  We're looking for the kind of code the compiler generates
** (naively) for *p++.  We want to make an indirect reference possible:
**
**	movl	O1,R 		->	movl O1,R
**	incl	O1    		->	movX (R),O2
**	movX	(R),O2		->	incl O1
**
** or
**	movl	O1,R  		->	movl O1,R
**	incl	O1    		->	movX O2,(R)
**	movX	O2,(R)		->	incl O1
**
**	if O1 is not a register used by O2
**	if instruction following movX is not conditional branch, since
**	we're setting different condition codes than before
**	in the first case, O2 cannot be a register used by O1.
**
** Note that this transformation is always correct because:
**
**	1.  O1 could not use R, or the first 2 instructions wouldn't work.
**	2.  O2 can use or set R without problems.
*/

	if (   cop1 == MOVL
	    && cop2 == INCL
	    && ismove(pl,&src1,&dst1)
	    && isreg(pf->op2)
	    && strcmp(pf->op1,pm->op1) == 0
	    && isdeadcc( pl )
	    && ! usesvar(pm->op1,pl->op1)	/* O1 can't use O2 */
	    && ! usesvar(pm->op1,pl->op2)	/* O1 can't use O2 */
	    && ! usesvar(pl->op2,pm->op1)	/* O2 can't use O1 */
	    && ! usesvar(pl->op1,pm->op1)	/* O2 can't use O1 */
	   						) {

		char * R = pf->op2;		/* point at register string */
		char * O1 = pf->op1;		/* point at first operand */
		char * O2;			/* second operand */

		if (   (   ( O2 = pl->op2, iszoffset(pl->op1,R) ) /* test (R) */
		        || ( O2 = pl->op1, iszoffset(pl->op2,R) ) )
		    && ! usesvar(O2,O1)	/* O2 can't use O1 */
		    && ! (   isvolatile(pm,1) 
		          && (   isvolatile(pl,1)
                              || isvolatile(pl,2)))
				/* O1 and O2 must not both be volatile, */ 
				/* and O1 and (R) must not both be volatile. */
						) {

			wchange();	/* change the window */
			lexchin(pm,pl);	/* preserve line number info */
			exchange(pm);	/* exchange the last two nodes */
			swplivecc(pm,pl);     /* swap live/dead info on CC's */
			makelive(R,pf);
			/* the target of the setting is now live earlier*/
			makelive(pl->op2,pm);
			return(true);
		}
	}

/*
** Scheduling  improvement
**
** This improvement rearranges things to facilitate a later scheduler
** improvement. 
**
**	movl	O1,R		->	fldl O1
**	movl	R,O2		->	movl O1,R
**	flds	O2  		->	movl R,O2
*/
	if (   cop1 == MOVL
	    && cop2 == MOVL
	    && cop3 == FLDS
	    && (! isreg(pf->op1))
	    && (!isconst(pf->op1))
	    && isreg(pf->op2)
	    && strcmp(pf->op2,pm->op1) == 0
	    && strcmp(pm->op2,pl->op1) == 0) {

		exchange(pm);
    		exchange(pf);
    		pl->op1 = pf->op1;
    		pl->nlive |= pf->nlive | scanreg(pf->op1,true);
    		return true;
	}


#if 0
/* bit field improvement:
**
**  This improvement will optimize expressions involving bit fields checked
**  for zero or non-zero.
**
**	movl	OP,R1
**	shrl	$C1,R1
**	andl	$C2,R1	->	testl	$(C2<<C1),OP
**
**  R1 must be dead afterwards
*/
	if (   cop1 == MOVL
	    && cop2 == SHRL
	    && cop3 == ANDL
	    && isreg(pf->op2)
	    && isreg(pm->op2)
	    && isreg(pl->op2)
	    && !strcmp(pf->op2, pm->op2)
	    && !strcmp(pf->op2, pl->op2)
	    && isdead(pf->op2, pl)
	    && isnumlit(pm->op1)
	    && isnumlit(pl->op1)
	    && *pf->op1 != '$') {

		wchange();
		lmrgin1(pf,pl,pl);	/* preserve line number info */
		constant = (int)strtoul(pl->op1+1,(char **)NULL,0)
				<< (int)strtoul(pm->op1+1,(char **)NULL,0);
		chgop(pl, TESTL, "testl");
		pl->op1 = getspace(sizeof("$-2147483647"));
		sprintf(pl->op1, "$%d", constant);
		pl->op2 = pf->op1;
		if (isvolatile(pf,1)) {
			/* propagate volatile info */
			mark_vol_opnd(pl,2);
		}
		DELNODE(pf);
		DELNODE(pm);
		return true;
	}
#endif


/* Get rid of no op loops.
** Label:
**		inc reg
**		cmp op,reg
**		jcc Label
** if reg is dead after the sequence.
** if label is not referenced anywhere else.
** label is never a part of the input to w?opt functions.
*/
	if (   cop1 == INCL
	    && cop2 == CMPL
	    && iscbr(pl)
	    && islabel(pf->back)
	    && (   samereg(pf->op1,pm->op1)
	        || samereg(pf->op1,pm->op2))
	    && ! isvolatile(pm,1)
	    && ! isvolatile(pm,2)
	    && isdead(pf->op1,pl)
	    && !strcmp(pl->op1,pf->back->opcode)) {

		DELNODE(pf);
		DELNODE(pm);
		DELNODE(pl);
		return true;
	  }

/************************************************************
**  The next section provides peepholes improving P6 targeted code
*************************************************************/
	if (target_cpu == P6) {

/*
**  This improvement will optimize expressions involving bit fields with
**  size of bite or word. ( rtx struct in 085.gcc )
**
**  movl    MEM,R1
**  shrl    $n,R1
**  andl    $m,R1   ->  movzbl x+MEM,R1
**
**  (n,m) = ( {8,16,24}, 255 )   bytes
**  (n,m) = ( 16,65535 )         words
*/
		if (   cop1 == MOVL
		    && cop2 == SHRL
		    && cop3 == ANDL
		    && isreg(pf->op2)
		    && isreg(pm->op2)
		    && isreg(pl->op2)
		    && ismem(pf->op1)
		    && !strcmp(pf->op2, pm->op2)
		    && !strcmp(pf->op2, pl->op2)
		    && isimmed(pm->op1)
		    && isimmed(pl->op1)  ) {

			char *c,*s,s1[2]="";
			int n=(int)strtoul(pm->op1+1,(char **)NULL,0);
			int m=(int)strtoul(pl->op1+1,(char **)NULL,0);
			int i,k=0;

			c = getspace(strlen(pf->op1) + 3);
			s = pf->op1;
			while ( (i=(int)strtoul(s,&s,0)) ) {
				k+=i;
			}
			if (   *s != '+'
			    && *s != '-'
			    && *s != '(' ) {

				*s1='+';
			}
			if ((m == 65535) & (n == 16)) {
				wchange();
				chgop(pf,MOVZWL,"movzwl");
				sprintf(c,"%d%s%s",k+2,s1,s);
				pf->op1 = c;
				DELNODE(pl);
				DELNODE(pm);
#ifdef WD_Print_PS_Peepholes
				fprintf(stderr,"#w3opt bit_field_peep in %s",
				        get_label());
				fprinst(pf);
#endif
				return true;
			}

			if( (m == 255) & ((n==8)|(n==16)|(n==24)) ) {

				i=n/8;
				wchange();
				chgop(pf,MOVZBL,"movzbl");
				sprintf(c,"%d%s%s",k+i,s1,s);
				pf->op1 = c;
				DELNODE(pl);
				DELNODE(pm);
#ifdef WD_Print_PS_Peepholes
				fprintf(stderr,"#w3opt bit_field_peep in %s",
				        get_label());
				fprinst(pf);
#endif
				return true;
			}
		}
/*  END of P6 section ***************************/
	} /* end if P6 */

	return(false);
}

boolean
ispwrof2(int num, int *ilog)
{
	if (num < 2) {
		return false;
	} else {
		int tmp;

		*ilog = 0;
		while ((tmp = num / 2) != 0) {
			++*ilog;
			if (num % 2) {
				return false;
			}
			num = tmp;
		}
		return true;
	}
}

