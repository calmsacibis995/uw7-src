#ident	"@(#)optim:i386/w1opt.c	1.1.3.39"
/* w1opt.c
**
**	Intel 386 optimizer:  for one-instruction window
**
*/

#include "sched.h"
#include "optutil.h"

extern int jflag;
extern int fp_removed;	/* declared in local.c */
extern int double_aligned;	/* declared in local.c */

extern char *itohalfreg();
extern char *itoreg();
static boolean isaligned(char *op);

/*	D A N G E R
**
** This definition selects the highest numbered register that we
** can arbitrarily choose as a temporary in the multiply strength
** reduction that is performed below.  It should equal the highest
** numbered temporary register used by the compiler (1 less than
** lowest numbered register used for register variables.
*/

/* w1opt -- one-instruction optimizer
**
** This routine handles the single-instruction optimization window.
** See individual comments below about what's going on.
** In some cases (which are noted), the optimizations are ordered.
*/

extern NODE n0;
extern struct RI *reginfo;
#ifdef STATISTICS
extern int first_run;
int try_peep = 0, done_peep = 0;
#endif
extern int suppress_enter_leave;
extern unsigned int full_reg();
char name1_buf[MAX_LABEL_LENGTH];


boolean					/* true if we make any changes */
/* ARGSUSED */
w1opt(register NODE * pf,		/* pointer to first instruction in
					** window (and last)
					*/
      NODE * pl)			/* pointer to last instruction in
					** window (= pf)
					*/

{

    register int cop = pf->op;		/* single instruction's op code # */
    boolean retval = false;     	/* return value:  in some cases
                                	** we fall through after completing
                                	** an optimization because it could
                                	** lead into others.  This variable
                                	** contains the return state for the
                                	** end.
                                	*/
    register char *dp, *tmp;
    register int len, num;
    register unsigned int regs_set;
    unsigned int reg;

/* eliminate dead code:
**
**	op ...
**   where the destination operand and all registers set are dead
*/

	if (   !((regs_set=sets(pf)) & pf->nlive)  /* all regs set dead? */
	    && regs_set != 0		/* leave instructions that don't
					** set any regs alone */
	    && !(regs_set & (FP0 | FP1 | FP2 | FP3 | FP4 | FP5 | FP6 |FP7))
					/* don't mess with fp instr */
	    && ! isbr(pf)		/* some branches set variables
					** and jump:  keep them */
	    && (   isdead(dp=dst(pf),pf)  /* are the destination and ... */
	        || (   !*dp
	            && (   pf->op == TESTL
	                || pf->op == CMPL
	                || pf->op == TESTW
	                || pf->op == TESTB
	                || pf->op == CMPW
	                || pf->op == CMPB
	                || pf->op == SAHF))) /* maybe SETcc when implemented? */
	    && (   pf->op1 == NULL
	        || !isvolatile( pf,1 )) /* are operands non- */
	    && (   pf->op2 == NULL
	        || !isvolatile( pf,2 )) /* volatile? */
	    && (   pf->op3 == NULL
	        || !isvolatile( pf,3 ))) {

		wchange();		/* Note we're changing the window */
		ldelin2(pf);		/* preserve line number info */
		mvlivecc(pf);		/* preserve condition codes line info */
		DELNODE(pf);		/* discard instruction */
		return(true);		/* announce success */
    }

/*
**	leal num(reg),reg	->	addl	$num,reg
*/


	if (   cop == LEAL
	    && (*pf->op1 != '(' )  
	    && scanreg(pf->op1, false) == scanreg(pf->op2, false)
	    && (tmp = strchr(pf->op1, '(')) != NULL
	    && !strncmp(tmp+1, pf->op2, len = strlen(pf->op2))
	    && *(tmp + 1 + len) == ')'
	    && isdeadcc(pf)) {

		len = tmp - pf->op1;
		wchange();			/* note change */
		chgop(pf, ADDL, "addl");
		tmp = pf->op1;
		pf->op1 = getspace((unsigned)len+2); /* 1 for '$' and
		                                        1 for '\0' */
		sprintf(pf->op1, "$%.*s", len, tmp);
		return(true);			/* made a change */
	}

/*
**	cltd	->	movl	%eax,%edx
**	          	sarl	$31,%edx
*/
	if (   (cop == CLTD) 
	    && (target_cpu & (P5 | blend))
	    &&	isdeadcc(pf)) {

		NODE * pnew, *prepend();

		wchange();			/* note change */
		pnew = prepend(pf, "%edx");
		chgop(pnew, MOVL, "movl");
		chgop(pf, SARL , "sarl");
		pnew->op1 = "%eax";
		pnew->op2 = "%edx";
		pnew->sets=setreg(pnew->op2);
		pnew->uses=setreg(pnew->op1);
		pnew->nlive=pf->nlive;
		pf->op1 = "$31";
		pf->op2 = "%edx";
		return(true);			/* made a change */
	}

/*
**	addw %reg1,%reg2 -> addl %reg1,%reg2
**
**	If high part of reg2 is dead and if condition codes are dead.
*/
	if (   !eiger
	    && cop == ADDW
	    && isreg(pf->op2)
	    && (   isreg(pf->op1)
	        || isconst(pf->op1))
	    && !(pf->nlive & CONCODES_AND_CARRY)) {

		unsigned int fullreg = full_reg(setreg(pf->op2));
		unsigned int reg = setreg(pf->op2);
		unsigned int upper_16 = fullreg & ~ reg;

		if (pf->nlive & upper_16) {
			goto addw_out;
		}
		wchange();
		chgop(pf,ADDL,"addl");
		if (isreg(pf->op1)) {
			pf->op1 = itoreg(full_reg(setreg(pf->op1)));
		}
		pf->op2 = itoreg(full_reg(setreg(pf->op2)));
		return true;
	}
addw_out:

/*
**	mov[bw]	mem,reg		->	movl	mem,reg'
**
**	(if mem is 4-byte aligned)
**	If the high part of reg is dead.
*/


	if (   target_cpu != P3
	    && isreg(pf->op2)
	    && (   (   cop == MOVB
	            && pf->op2[2] == 'l'
	            && !isreg(pf->op1)
	            && *pf->op1 != '$'
	            && isaligned(pf->op1))
	        || (   cop == MOVW
	            && (   isreg(pf->op1)
	                || *pf->op1 == '$'
	                || isaligned(pf->op1))))) {

		reg = setreg(pf->op2);
		reg = full_reg(reg) & ~reg;
		if (reg & pf->nlive) {
			goto cont;
		}
		if (isreg(pf->op1)) {
			pf->op1 = itoreg(full_reg(setreg(pf->op1)));
		}
		pf->op2 =  itoreg(full_reg(setreg(pf->op2)));
		wchange();			/* note change */
		chgop(pf, MOVL, "movl");
		return true;			/* made a change */
	}
cont:

/*
**	movw	mem,reg			movb	mem,reg'
**
**	(if mem is 4-byte aligned)
**	If the high part of reg is dead.
*/


	if (   target_cpu != P3
	    && isreg(pf->op2)
	    && (   cop == MOVW
	        || cop == ADDW)
	    && ((reg = setreg(pf->op2)) & (AL | BL | CL | DL))
	    && ! (reg & pf->nlive & ~(AL | BL | CL| DL))
	    && (   !isreg(pf->op1)
	        || (setreg(pf->op1) & (AL | BL | CL | DL)))) {

		if (isreg(pf->op1)) {
			pf->op1 = itoreg(L2B(setreg(pf->op1)));
		}
		pf->op2 =  itoreg(L2B(reg));
		if (isimmed(pf->op1)) {
			int n;

			n = (int)strtoul(&pf->op1[1],(char **)NULL,0);
			if ( n != (char) n) {
				/* max size is '$-xxx'\0' */
				pf->op1 = getspace(5);
				sprintf(pf->op1, "$%d", (char) n );
			}
		}
		wchange();			/* note change */
		if (cop == MOVW ) {
			chgop(pf, MOVB, "movb");
		} else {
			chgop(pf, ADDB, "addb");
		}
		return true;			/* made a change */
	}

/*
**	testw	$imm,O2			testl	$imm,O2'
**
**	(If O2 is 4-byte aligned or register) and ($imm & 0xffff0000 ) == 0 
*/


	if (   target_cpu != P3
	    && (cop == TESTW )
	    && (   isimmed(pf->op1)
	        && isdigit(pf->op1[1]))
	    && (   isreg(pf->op2)
	        || isaligned(pf->op2))
	    && !(strtoul(&pf->op1[1],(char **)NULL,0) & 0xffff0000)) {

		if (isreg(pf->op2)) {
			pf->op2 = itoreg(full_reg(setreg(pf->op2)));
		}
		chgop(pf, TESTL, "testl");
		return true;			/* made a change */
	}

/*
**	testw	$imm,O2			testb	$imm,O2'
**
**	If the high part of reg is dead, and ($imm & 0xffffff00 ) == 0 
*/


	if (   target_cpu != P3
	    && (cop == TESTW )
	    && (isimmed(pf->op1) 
	    && isdigit(pf->op1[1]))
	    && (! isreg(pf->op2)) 
	    && !(strtoul(&pf->op1[1],(char **)NULL,0) & 0xffffff00)) {

		chgop(pf, TESTB, "testb");
		return true;			/* made a change */
	}

/* Split one movw into two movb's. It's only for P5, to gain pairing.
** It is a good idea to place this peephole AFTER the previous one, so that
** A movw has a better chance to turn into a movl than into two movb's.
**
**	movw X,ax    -> movb X,%al
**	             -> movb X+1,%ah
*/

	if (   target_cpu == P5
	    && cop == MOVW
	    && (   ! isreg(pf->op1)
	        || !(setreg(pf->op1) & (EDI|ESI|EBI|EBP)))
	    && (   ! isreg(pf->op2)
	        || !(setreg(pf->op2) & (EDI|ESI|EBI|EBP)))
	    && !isvolatile(pf,1)
	    && !isvolatile(pf,2)
	    && !(setreg(pf->op2) & scanreg(pf->op1,false))
	    && (   ! (ismem(pf->op1)
	        || ismem(pf->op2)))) {

		NODE *pnew,*insert();
		int i,n,n1,n2,m,l,x,scale;
		char *t,*name1;

		pnew = insert(pf);
		pnew->nlive = pf->nlive;
		wchange();
		chgop(pf,MOVB,"movb");
		chgop(pnew,MOVB,"movb");
		for (m = 1; m <= 2; m++) {
			for (i = 0; i < MAX_LABEL_LENGTH; i++) {
				name1_buf[i] = (char) 0;
			}
			if (isimmed(pf->ops[m])) {
				n = (int)strtoul(pf->ops[m]+1,(char **)NULL,0);
				n1 = n & 0xff;
				n2 = (n & 0xff00) >> 8;
				pf->ops[m] = getspace(ADDLSIZE);
				sprintf(pf->ops[m],"$%d",n1);
				pnew->ops[m] = getspace(ADDLSIZE);
				sprintf(pnew->ops[m],"$%d",n2);

			} else if (isreg(pf->ops[m])) {
				pnew->ops[m] = w2h(pf->ops[m]);
				pf->ops[m] = w2l(pf->ops[m]);
			} else {
				t = strchr(pf->ops[m],'(');
				if (t == NULL) {
					t = "";
				}
				name1 = ((l=strlen(pf->ops[m])) < ((unsigned) MAX_LABEL_LENGTH))
					? name1_buf : getspace(l = l + 1 + ADDLSIZE);
				(void) decompose(pf->ops[m],&x,name1,&scale);
				x++;
				l = MAX(l , MAX_LABEL_LENGTH);
				pnew->ops[m] = getspace(l);
				if (name1[0]) {
					if (x > 0) {
						sprintf(pnew->ops[m],"%s+%d%s",name1,x,t);

					} else if (x < 0) {
						sprintf(pnew->ops[m],"%s-%d%s",name1,x,t);
					} else {
						sprintf(pnew->ops[m],"%s%s",name1,t);
					}
				} else {
					sprintf(pnew->ops[m],"%d%s",x,t);
				}
			}
		}
		new_sets_uses(pnew);
		pf->nlive |= pnew->uses;
		return true;
	}

/*
**	leal (,reg,num),reg	->	shll	$num',reg
*/


	if (   cop == LEAL
	    && *(tmp = pf->op1) == '('
	    && tmp[1] == ','
	    && isreg(pf->op2)
	    && !strncmp(tmp+2, pf->op2, strlen(pf->op2))
	    && tmp[6] == ','
	    && tmp[8] == ')'
	    && (   (num = tmp[7] - '0') == 2
	        || num == 4
	        || num == 8)
	    && isdeadcc(pf)) {

		num = num == 4 ? 2 : num == 8 ? 3 : 1;

		wchange();			/* note change */
		chgop(pf, SHLL, "shll");
		pf->op1 = getspace(3);	/* 1 for '$', 1 for [248],
		                           and 1 for '\0' */
		sprintf(pf->op1, "$%d", num);
		return true;			/* made a change */
	}

/*
**	movzbl op,reg	->	xorl	reg,reg
**	                  	movb	op,reg'
**			OR
**
**	                ->	movb	op,reg
**                  		andl	$255,reg
**			OR   (from non aligned memory to non byte register  
**
**	                ->	movb	op,tmp_reg
**	                  	andl	$255,tmp_reg
**	                  	movl	tmp_reg,reg  may be removed by mov reg,reg
*/


	if (   target_cpu != P3
	    && (   target_cpu != blend
	        && target_cpu != P6)
    	    && cop == MOVZBL
	    && isdeadcc(pf)) {

		NODE * pnew, *prepend();

		num = setreg(pf->op2);
	
		if (   (num & (EAX | EBX | ECX | EDX))
		    && (! usesreg(pf->op1, pf->op2))
		    && (   isreg(pf->op1)
		        || !isaligned(pf->op1))
		    && (   pf->forw->op != ANDL
		        || pf->forw->op != TESTL)) {

			/* If new next instruction is AND or TEST w2opt
			    will remove one ANDL */
			wchange();			/* note change */

			pnew = prepend(pf, pf->op2);
			chgop(pnew, XORL, "xorl");
			pnew->op1 = pnew->op2 = pf->op2;
			pnew->nlive = pf->nlive;
			chgop(pf, MOVB, "movb");
			pf->op2 = getspace(4);		/* "%[abcd]l\0" */
			sprintf(pf->op2, "%%%cl", pnew->op1[2]);

			lexchin(pf,pnew);		/* preserve line number info */ 
			return(true);			/* made a change */

		} else {
			if (! (   (num & (EAX | EBX | ECX | EDX))
			       || (   isreg(pf->op1)
			           || isaligned(pf->op1)))) {

				if (! (pf->nlive & ECX )) {
					tmp = "%ecx";
				} else if (! (pf->nlive & EDX )) {
					tmp = "%edx";
				} else if (! (pf->nlive & EBX )) {
					tmp = "%ebx";
				} else if (! (pf->nlive & EAX )) {
					tmp = "%eax";
				} else {
					return false;
				}
				pnew = insert(pf);
				chgop(pnew, MOVL, "movl");
				pnew->op1 = tmp;
				pnew->op2 = pf->op2;
				pf->op2 = tmp;
				pnew->nlive = pf->nlive;
				pf->nlive &= ~setreg(pnew->op2); 
				pf->nlive |= setreg(tmp);
				lexchin(pf,pnew);		/* preserve line number info */ 
			}

			if (   (setreg(pf->op1) & (AH|BH|CH|DH))
			    && (setreg(pf->op2) & (ESI|EDI|EBI))) {

				return false;
			}

			wchange();			/* note change */
			pnew = prepend(pf, pf->op2);
			pnew->op2 = pf->op2;
			pnew->nlive = pf->nlive;
			if (   isreg(pf->op1)
			    || isaligned(pf->op1)) {
				chgop(pnew, MOVL, "movl");
				if (isreg(pf->op1)) {
					tmp = pf->op1;

					/* "%e[abcd]x\0" */
					pf->op1 = getspace(5);
					sprintf(pf->op1, "%%e%cx", tmp[1]);
				}
			} else {
				chgop(pnew, MOVB, "movb");

				/* "%[abcd]l\0" */
				pnew->op2 = getspace(4);
				sprintf(pnew->op2, "%%%cl", pf->op2[2]);
			}
			pnew->op1 = pf->op1;
			cp_vol_info(pf,pl);
			mark_not_vol(pf,1);
			chgop(pf, ANDL, "andl");
			pf->op1 = "$255";
			lexchin(pf,pnew);	/* preserve line number info */ 
			return(true);		/* made a change */
		}
	}

/*
**      For p6 and blend option
**
**  movzbl op,reg   ->  xorl    reg,reg         case1
**  ...                 movb    op,reg
**  use reg
**
**  movzbl op,reg   ->  xorl    r1,r1           case 2
**                      movb    op,r1
**                      movl    r1,reg
*/

	if (   (pf->op == MOVZBL /*|| pf->op == MOVZWL */)
	    &&  (   target_cpu != blend
	         && target_cpu != P6)
	    &&  isdeadcc( pf ) ) {

		NODE *pnew, *prepend();

		pf->uses = uses(pf);
		pf->sets = sets(pf);
   
		if (   usesreg(pf->op1, pf->op2)
		    ||  pf->sets & ~(EAX|EBX|ECX|EDX) ) {

			/* case 2 */
			char* c=pf->op2;
			int rn = get_free_reg(pf->sets|pf->uses|pf->nlive, 1, pf);
			if( rn == -1 ) {
				return false;
			}

#ifdef WD_Print_PS_Peepholes
	fprintf(stderr,"#w1opt movzbl_2_blend+newreg in %s",get_label());
	fprinst(pf);
#endif
			/* xorl enr,enr */
			pnew = prepend(pf, reginfo[rn].regname[0]);
			chgop(pnew, XORL, "xorl");
			pnew->op1 = pnew->op2 = reginfo[rn].regname[0];

			/* movb op,nr */
			chgop(pf, MOVB, "movb");
			pf->op2 = getspace(4);      /* "%[abcd]l\0" */
			sprintf(pf->op2, "%%%cl", pnew->op1[2]);
			pf->nlive |= reginfo[rn].reg_number;

			/* movl enr,reg */
			pnew = insert( pf );
			chgop(pnew, MOVL, "movl");
			pnew->op2 = c;
			pnew->op1 = reginfo[rn].regname[0];
			pnew->nlive = pf->nlive &~reginfo[rn].reg_number;

			lexchin(pf,pnew);       /* preserve line number info */
			return(true);		/* made a change */

		} else {

			/* case 1 */
			wchange();		  /* note change */

#ifdef WD_Print_PS_Peepholes
	fprintf(stderr,"#w1opt movzbl_2_blend in %s",get_label());
	fprinst(pf);
#endif
			pnew = prepend(pf, pf->op2);
			chgop(pnew, XORL, "xorl");
			pnew->op1 = pnew->op2 = pf->op2;
			pnew->nlive = pf->nlive;
			chgop(pf, MOVB, "movb");
			pf->op2 = getspace(4);      /* "%[abcd]l\0" */
			sprintf(pf->op2, "%%%cl", pnew->op1[2]);

			lexchin(pf,pnew);       /* preserve line number info */
			return(true);		   /* made a change */
		}
	}


/*
**	pushl mem	->	movl	mem,%eax
**	         	  	pushl	%eax
**
**	Note that for this improvement, we must be optimizing for a 486,
**	as this sequence is slower on a 386;  also, at least one scratch
**	register must not be live after this instruction.
*/


	if (   target_cpu != P3
	    && cop == PUSHL
	    && *pf->op1 != '%'
	    && *pf->op1 != '$') {

		NODE * pnew, *prepend();
		int i;

		i = get_free_reg(pf->nlive,0,pf);
		if (i == -1 ) {
			return false;
		}
		tmp = reginfo[i].regname[0];

		wchange();			/* note change */
		pnew = prepend(pf, tmp);
		chgop(pnew, MOVL, "movl");
		pnew->op1 = pf->op1;
		pf->op1 = pnew->op2 = tmp;
		cp_vol_info(pf,pnew);	/* preserve volatile info */
		mark_not_vol(pf,1);	/* get rid of old volatile info */	
	
		lexchin(pf,pnew);		/* preserve line number info */ 
		return(true);			/* made a change */
	}

/*
**	movzwl op,reg				( if op aligned or register)
**	             	->	movl	op,reg
**	             	  	andl	$65535,reg
**
**			OR			if op is not register and not
**			  			aligned and reg not used in op 
**
**                 	->	xorl	reg,reg  
**	             	  	movw	op,reg'
**
**			OR			if op is not register and not
**			  			aligned and reg is used in op 
**
**	              	->	movw	op,reg
**	             	  	andl	$65535,reg
*/    
	if (   (   target_cpu != P3
	        && target_cpu != blend
	        && target_cpu != P6)
	    &&  cop == MOVZWL && isdeadcc(pf)) {

		NODE * pnew, *prepend();

		if (   (isreg(pf->op1)
		    || isaligned(pf->op1))) {
			wchange();			/* note change */
			pnew = prepend(pf, pf->op2);
			chgop(pnew, MOVL, "movl");
			pnew->op2 = pf->op2;
			pnew->nlive = pf->nlive;
			if (isreg(pf->op1)) {
				tmp = pf->op1;

				/* "%??" -> "%e??\0" */
				pf->op1 = getspace(5);
				sprintf(pf->op1, "%%e%c%c", tmp[1],tmp[2]);
			}
			pnew->op1 = pf->op1;
			chgop(pf, ANDL, "andl");
			pf->op1 = "$65535";
			lexchin(pf,pnew);	/* preserve line number info */ 
			cp_vol_info(pf,pnew);	/* preserve volatile info */
			mark_not_vol(pf,1);
			return(true);		/* made a change */

		} else if (usesreg(pf->op1, pf->op2)) {
			wchange();			/* note change */
			pnew = prepend(pf, pf->op2);
			chgop(pnew, MOVW, "movw");
			pnew->op2 = getspace(4);	/* "%eXX" -> "%XX" */
			sprintf(pnew->op2, "%%%c%c", pf->op2[2],pf->op2[3]);
			pnew->nlive = pf->nlive;
			pnew->op1 = pf->op1;
			chgop(pf, ANDL, "andl");
			pf->op1 = "$65535";
			lexchin(pf,pnew);	/* preserve line number info */ 
			cp_vol_info(pf,pnew);	/* preserve volatile info */
			mark_not_vol(pf,1);
			return(true);			/* made a change */
		} else {
			wchange();			/* note change */
			pnew = prepend(pf, pf->op2);
			chgop(pnew, XORL, "xorl");
			pnew->op1 = pnew->op2 = pf->op2;
			pnew->nlive = pf->nlive;
			chgop(pf, MOVW, "movw");
			pf->op2 = getspace(4);		/* "%eXX" -> "%XX" */
			sprintf(pf->op2, "%%%c%c", pnew->op1[2],pnew->op1[3]);
			lexchin(pf,pnew);	/* preserve line number info */ 
			return(true);			/* made a change */
		}
	}

/*
**	cmpl $imm,mem	->	movl	mem,%eax
**	             	  	cmpl	$imm,%eax
**
**	This will only be done if a displacement is in the memory operand.
**	Note that for this improvement, we must be optimizing for a 486,
**	as this sequence is slower on a 386;  also, at least one scratch
**	register must not be live after this instruction.
*/

	if (   target_cpu != P3
	    && (   cop == CMPL
	        || cop == TESTL
	        || cop == CMPB
	        || cop == TESTB
	        || cop == CMPW
	        || cop == TESTW)
	    && *pf->op1 == '$'
	    && *pf->op2 != '%'
	    && *pf->op2 != '$'
	    && *pf->op2 != '(' ) {

		NODE * pnew, *prepend();
		int i;

		if (! ((pf->nlive | scanreg(pf->op1,false)) & EAX)) {

			/* %eax is shorter by one byte, pairable in
			   test for P5 */
			tmp = "%eax";
			i = 0;	/* %eax is the first register in reginfo */
		} else {
			if ((i = get_free_reg(pf->nlive | scanreg(pf->op1,false),
		                      OpLength(pf) == ByTE,pf)) == -1) {
				goto after_cmpl_immed_mem;
			}
			tmp = reginfo[i].regname[0];
		}
	

		wchange();			/* note change */

		switch(OpLength(pf)) {

			case ByTE:
				tmp = reginfo[i].regname[2];
				pnew = prepend(pf,tmp);
				chgop(pnew,MOVB,"movb");
				pnew->op1 = pf->op2;
				pf->op2 = pnew->op2;
				break;

			case WoRD:
				if (!isaligned(pf->op2)) {
					tmp = reginfo[i].regname[1];
					pnew = prepend(pf,tmp);
					chgop(pnew,MOVW,"movw");
					pnew->op1 = pf->op2;
					pnew->op2 = pf->op2 = tmp;
					break;

				}
				/*FALLTHRU*/
			default:
				pnew = prepend(pf,tmp);
				chgop(pnew,MOVL,"movl");
				pnew->op1 = pf->op2;
				pnew->op2 = pf->op2 = tmp;
				if (OpLength(pf) == WoRD) {
					pf->op2 = reginfo[i].regname[1]; 
				}
		} /* end switch */

		pnew->nlive &= ~CONCODES_AND_CARRY;
		xfer_vol_opnd(pf,2,pnew,1);
		mark_not_vol(pf,2);

		lexchin(pf,pnew);		/* preserve line number info */ 
		return(true);			/* made a change */
	}
after_cmpl_immed_mem:

/*
**	P5  only CISC to RISC code
**
*/

	if (   (target_cpu & (P5 | blend))
	    && ! jflag) {

/* Change any dyadic op on memory to move memory to register and
** then op. example:
**
**	addl X,R1 -> movl X,R2
**	             addl R2,R1
*/
		if (   (   ISRISCY(pf)
		        || Istest(pf))
		    && *pf->op1 != '%'
		    && *pf->op1 != '$') {

			NODE * pnew, *prepend();
			int i;
#ifdef STATISTICS
			if (first_run) {
	  			++try_peep;
			}
#endif
			if ((i = get_free_reg(pf->nlive | scanreg(pf->op1,0) | scanreg(pf->op2,0),
	                      OpLength(pf) == ByTE,pf)) == -1) {
				return false;
			}
			tmp = reginfo[i].regname[0];

#ifdef DEBUGW1
			fprintf(stderr,"was: "); fprinst(pf);
#endif

			switch(OpLength(pf)) {
				case ByTE:
					tmp = reginfo[i].regname[2];
					pnew = prepend(pf, tmp);
					chgop(pnew, MOVB, "movb");
				 	break;

				case LoNG:
					pnew = prepend(pf, tmp);
					chgop(pnew, MOVL, "movl");
				 	break;

				default  : 
					return false;
			} /* end switch */
	
#ifdef STATISTICS
			if (first_run) {
				++done_peep;
			}
#endif
			wchange();			/* note change */
			pnew->op1 = pf->op1;
			pf->op1 = tmp;
			pnew->nlive |= scanreg(pnew->op1,false);
			makelive(pf->op2,pnew);
#ifdef DEBUGW1
			fprintf(stderr,"became: "); fprinst(pnew); fprinst(pf);
#endif
			lexchin(pf,pnew);	/* preserve line number info */ 
			cp_vol_info(pf,pnew);	/* preserve volatile info */
			mark_not_vol(pf,1);
			return(true);			/* made a change */
		}

/*
** The other direction:
**
** op reg1,X	->	mov X,reg2
**          	  	op  reg1,reg2
**          	  	mov reg2,X
*/

		if (   ISRISCY(pf)
		    && *pf->op2 != '%'
		    && ! isvolatile(pf,2)) {

			NODE *pnew, *prepend();
        		int i;
			int top;
			char *topc;
#ifdef STATISTICS
			if (first_run) {
				++try_peep;
			}
#endif
			if ((i = get_free_reg(pf->nlive |scanreg(pf->op2,0) |scanreg(pf->op1,0),
		                     OpLength(pf) == ByTE,pf)) == -1) {
				return false;
			}
			tmp = reginfo[i].regname[0];
			switch(OpLength(pf)) {
				case ByTE:
					tmp = reginfo[i].regname[2];
					top = MOVB;
					topc = "movb";
					break;
  
				case LoNG:
					  top = MOVL;
					  topc = "movl";
					  break;

				default:
					  return false;
			} /* end switch */
#ifdef DEBUGW1
			fprintf(stderr,"was "); fprinst(pf);
#endif
			wchange();
			pnew = prepend(pf,tmp);
			chgop(pnew,top,topc);
			pnew->op1 = pf->op2;
			pnew->nlive |= scanreg(pnew->op1,false);
			makelive(pf->op1,pnew);
			pf->op2 = tmp;
			makelive(pf->op2,pf);
			pf->nlive |= scanreg(pnew->op1,false);
			lexchin(pf,pnew);
			pnew = insert(pf);
			chgop(pnew,top,topc);
			pnew->op1 = tmp;
			pnew->op2 = getspace(strlen(pf->back->op1));
			(void) strcpy(pnew->op2,pf->back->op1);
			pnew->nlive = pf->nlive;
#ifdef STATISTICS
			if (first_run) {
				++done_peep;
			}
#endif
#ifdef DEBUGW1
			fprintf(stderr,"became ");
			fprinst(pf->back); fprinst(pf); fprinst(pf->forw);
#endif
        		return(true);
		}


/* The other direction for compare, test:
**
**	cmp reg1,X	->	mov X,reg2
**	          	  	cmp reg1,reg2
*/
		if (   Istest(pf)
		    && *pf->op2 != '%'
		    && *pf->op2 != '$' ) {

			NODE *pnew, *prepend();
			int i;
			int top;
			char *topc;
#ifdef STATISTICS
			if (first_run) {
				++try_peep;
			}
#endif
#ifdef DEBUGW1
			fprintf(stderr,"was: "); fprinst(pf);
#endif
			if ((i = get_free_reg(pf->nlive |scanreg(pf->op2,0) |scanreg(pf->op1,0),
		                      OpLength(pf) == ByTE,pf)) == -1) {
				return false;
			}
			tmp = reginfo[i].regname[0];

#ifdef DEBUGW1
	   		fprintf(stderr,"cmp: %s",tmp);
			plive(pf->nlive,0);
			fprinst(pf);
#endif
			switch(OpLength(pf)) {
				case ByTE:
					tmp = reginfo[i].regname[2];
					top = MOVB;
					topc = "movb";
					break;

				case LoNG:
					top = MOVL;
					topc = "movl";
					break;

				default:
					return false;
			} /* end switch */
#ifdef STATISTICS
			if (first_run) {
				++done_peep;
			}
#endif
			wchange();
			pnew = prepend(pf,tmp);
			chgop(pnew,top,topc);
			pnew->op1 = pf->op2;
			makelive(pf->op1,pnew);
			pf->op2 = tmp;
			lexchin(pf,pnew);
			if (isvolatile(pf,2)) {
				mark_vol_opnd(pnew,1);
			}
			mark_not_vol(pf,1);
#ifdef DEBUGW1
			fprintf(stderr,"became: ");
			fprinst(pnew);
			fprinst(pf);
#endif
			return(true);
		}


/*
** The same as above for INC DEC NOT and NEG
*/
		if (   Isreflexive(pf)
		    &&  *pf->op1 != '%'
		    && ! isvolatile(pf,1)) {

			NODE * pnew, *prepend();
			int i;
			int top;
			char *topc;
#ifdef DEBUGW1
			fprintf(stderr,"was "); fprinst(pf);
#endif
#ifdef STATISTICS
			if (first_run) {
				++try_peep;
			}
#endif
			if ((i = get_free_reg(pf->nlive | scanreg(pf->op1,false),
	         		OpLength(pf) == ByTE,pf)) == -1) {

				return false;
			}
			tmp = reginfo[i].regname[0];

			switch (OpLength(pf)) {
				case ByTE: 
					tmp = reginfo[i].regname[2];
					top = MOVB;
					topc = "movb";
					break;

				case LoNG:
					top = MOVL;
					topc = "movl";
					break;

				default:
					return false;
			} /* end switch */

#ifdef STATISTICS
			if (first_run) {
				++done_peep;
			}
#endif
			wchange();			/* note change */
			pf->nlive |= scanreg(pf->op1,false);
			pnew = prepend(pf, tmp);
			pnew->nlive |= scanreg(pf->op1,false);
			chgop(pnew, top, topc);
			pnew->op1 = pf->op1;
			lexchin(pf,pnew);	/* preserve line number info */ 

			pf->op1 = tmp;
			makelive(pf->op1,pf);
	
			pnew = insert(pf);
			chgop(pnew, top, topc);
			pnew->op1 = tmp;
			pnew->op2 = getspace(strlen(pf->back->op1));
			(void) strcpy(pnew->op2,pf->back->op1);
			pnew->nlive = pf->nlive;
#ifdef DEBUGW1
			fprintf(stderr,"became ");
			fprinst(pf->back);
			fprinst(pf); fprinst(pf->forw);
#endif
			return(true);			/* made a change */
		}


/*
**	op $imm,disp(r1) -> mov $imm,r2
**	                    op  r2,disp(r1)
*/

		if (   (   ISRISCY(pf)
		        || Istest(pf) 
		        || (   (target_cpu == P6)
		             && Ismov(pf)))
		    && *pf->op1 == '$'
		    && hasdisplacement(pf->op2)) {
	
			NODE * pnew, *prepend();
			int i;
			int top;
			char *topc;
#ifdef DEBUGW1
			fprintf(stderr,"was 11");
			fprinst(pf);
#endif
#ifdef STATISTICS
			if (first_run) {
				++try_peep;
			}
#endif
			if ((i = get_free_reg(pf->nlive | scanreg(pf->op2,false),
	         		OpLength(pf) == ByTE,pf)) == -1) {

				return false;
			}
			tmp = reginfo[i].regname[0];
#ifdef STATISTICS
			if (first_run) {
				++done_peep;
			}
#endif
			if (OpLength(pf) == ByTE) {
				tmp = reginfo[i].regname[2];
				top = MOVB;
				topc = "movb";
			} else {
				top = MOVL;
				topc = "movl";
			}
			wchange();
			pnew = prepend(pf,tmp);
			chgop(pnew,top,topc);
			pnew->op1 = pf->op1;
			pf->op1 = tmp;
			lexchin(pf,pnew);
#ifdef DEBUGW1
			fprintf(stderr,"became: ");
			fprinst(pnew);
			fprinst(pf);
#endif
			return(true);
		}
	}

/*
**	cmp[bwl] $0,R	->	test[bwl] R,R
*/


	if (   (   cop == CMPL
	        || cop == CMPW
	        || cop == CMPB)
	    &&  strcmp(pf->op1,"$0") == 0
	    &&  isreg(pf->op2)) {

		wchange();			/* note change */
		switch ( cop ) {		/* change the op code */
			case CMPL:
				chgop(pf,TESTL,"testl");
				break;

			case CMPW:
				chgop(pf,TESTW,"testw");
				break;

			case CMPB:
				chgop(pf,TESTB,"testb");
				break;

		}
		pf->op1 = pf->op2;	/* both operands point at R */
		makelive(pf->op2,pf);	/* make register appear to be live
		pf->nlive |= CONCODES;	** hereafter so "compare" isn't thrown
					** away.  (Otherwise R might be dead
					** and we would eliminate the inst.)
					*/
		return(true);		/* made a change */
	}

/*
**
**	addl $1,O1	->	incl O1
**
**	Also for dec[bwl] and inc[bw]
*/


	if (   (   cop == ADDL
	        || cop == ADDW
	        || cop == ADDB
	        || cop == SUBL
	        || cop == SUBW
	        || cop == SUBB)
	    && strcmp(pf->op1,"$1") == 0
	    && isdeadcarry(pf)) {

		wchange();			/* note change */
		switch ( cop ) {		/* change the op code */
			case ADDL:
				chgop(pf,INCL,"incl");
				break;

			case ADDW:
				chgop(pf,INCW,"incw");
				break;

			case ADDB:
				chgop(pf,INCB,"incb");
				break;

			case SUBL:
				chgop(pf,DECL,"decl");
				break;

			case SUBW:
				chgop(pf,DECW,"decw");
				break;

			case SUBB:
				chgop(pf,DECB,"decb");
				break;

		}
		pf->op1 = pf->op2;	/* both operands point at R */
		pf->op2 = NULL;
		makelive(pf->op1,pf);	/* make register appear to be live
					** hereafter so "compare" isn't thrown
					** away.  (Otherwise R might be dead
					** and we would eliminate the inst.)
					*/
		return(true);		/* made a change */
	}

/*
**
**	addl $-1,O1	->	decl O1
**
**	Also for dec[bwl] and inc[bw]
*/
	if (   (   cop == ADDL
	        || cop == ADDW
	        || cop == ADDB
	        || cop == SUBL
	        || cop == SUBW
	        || cop == SUBB)
	    &&  strcmp(pf->op1,"$-1") == 0
	    && isdeadcarry(pf)) {

		wchange();			/* note change */
		switch ( cop ) {		/* change the op code */
			case ADDL:
				chgop(pf,DECL,"decl");
				break;

			case ADDW:
				chgop(pf,DECW,"decw");
				break;

			case ADDB:
				chgop(pf,DECB,"decb");
				break;

			case SUBL:
				chgop(pf,INCL,"incl");
				break;

			case SUBW:
				chgop(pf,INCW,"incw");
				break;

			case SUBB:
				chgop(pf,INCB,"incb");
				break;

		}
		pf->op1 = pf->op2;	/* both operands point at R */
		pf->op2 = NULL;
		makelive(pf->op1,pf);	/* make register appear to be live
					** hereafter so "compare" isn't thrown
					** away.  (Otherwise R might be dead
					** and we would eliminate the inst.)
					*/
		return(true);			/* made a change */
	}

/*
**	mov[bwl] $0,R1	->	xor[bwl] R1,R1
**
**	This is the same speed, but more compact.
*/


	if (   (   cop == MOVL
	        || cop == MOVW
	        || cop == MOVB)
	    &&  strcmp(pf->op1,"$0") == 0
	    &&  isreg(pf->op2)
	    &&  isdeadcc(pf)) {

		wchange();			/* note change */
		switch ( cop ) {		/* change the op code */
			case MOVL:
				chgop(pf,XORL,"xorl");
				break;

			case MOVW:
				chgop(pf,XORW,"xorw");
				break;

			case MOVB:
				chgop(pf,XORB,"xorb");
				break;

		}
		pf->op1 = pf->op2;	/* both operands point at R */
		makelive(pf->op1,pf);	/* make register appear to be live
					** hereafter so "compare" isn't thrown
					** away.  (Otherwise R might be dead
					** and we would eliminate the inst.)
					*/
		return(true);			/* made a change */
	}
/*
**   Check if the signed or zero extensions are still needed.
**
**     movswl   op1,Lreg       ->   movw   op1,Wreg
**
*/
	if (   (   cop == MOVSWL
	        || cop == MOVZWL)
	    && ((reg = sets(pf) & (Eax|Ebx|Ecx|Edx|Esi|Edi)) != 0)
	    && !(reg & pf->nlive)) {
		wchange();
		pf->op2 = itohalfreg((unsigned)sets(pf));
		chgop(pf,MOVW,"movw");
	}
	if (   (   cop == MOVSBL
	        || cop == MOVZBL
	        || cop == MOVSBW
		|| cop == MOVZBW)
	    && ((reg = sets(pf) & (Eax|Ebx|Ecx|Edx|Ax|Bx|Cx|Dx|AH|BH|CH|DH))
	                                  != 0)
	    && !(reg & pf->nlive)) {
		wchange();
		pf->op2 = itoreg((unsigned)L2B(sets(pf)));
		chgop(pf,MOVB,"movb");
	}


/* get rid of useless arithmetic
**
**	addl	$0,O		->	deleted  or  cmpl O,$0
**	subl	$0,O		->	deleted  or  cmpl O,$0
**	orl 	$0,O		->	deleted  or  cmpl O,$0
**	xorl	$0,O		->	deleted  or  cmpl O,$0
**	sall	$0,O		->	deleted  or  cmpl O,$0
**	sarl	$0,O		->	deleted  or  cmpl O,$0
**	shll	$0,O		->	deleted  or  cmpl O,$0
**	shrl	$0,O		->	deleted  or  cmpl O,$0
**	mull	$1,O		->	deleted  or  cmpl O,$0
**	imull	$1,O		->	deleted  or  cmpl O,$0
**	divl	$1,O		->	deleted  or  cmpl O,$0
**	idivl	$1,O		->	deleted  or  cmpl O,$0
**	andl	$-1,O		->	deleted  or  cmpl O,$0

**	mull	$0,O		->	movl $0,O
**	imull	$0,O		->	movl $0,O
**	andl	$0,O		->	movl $0,O
**
** Note that since we've already gotten rid of dead code, we won't
** check whether O (O2) is live.  However, we must be careful to
** preserve the sense of result indicators if a conditional branch
** follows some of these changes.
*/

/* Define types of changes we will make.... */

#define	UA_NOP		1		/* no change */
#define UA_DELL		2		/* delete instruction */
#define UA_DELW		3		/* delete instruction */
#define UA_DELB		4		/* delete instruction */
#define UA_MOVZL	5		/* change to move zero to ... */
#define UA_MOVZW	6		/* change to move zero to ... */
#define UA_MOVZB	7		/* change to move zero to ... */

/*
** We must have a literal as the first operand.
*/
	if (isnumlit(pf->op1)) {

		int ultype = UA_NOP;	/* initial type of change = none */

		/* branch on literal */
		switch((long)strtoul(pf->op1+1,(char **)NULL,0)) {
			case 0:		/* handle all instructions with &0
					** as first operand
					*/
				switch (cop) {
					case ADDL:
					case SUBL:
					case ORL:
					case XORL:
					case SALL:
					case SHLL:
					case SARL:
					case SHRL:
					case SHRDL:
					case SHLDL:
						ultype = UA_DELL;
						break;
	    
					case ADDW:
					case SUBW:
					case ORW:
					case XORW:
					case SALW:
					case SHLW:
					case SARW:
					case SHRW:
					case SHRDW:
					case SHLDW:
						ultype = UA_DELW;
						break;
	    
					case ADDB:
					case SUBB:
					case ORB:
					case XORB:
					case SALB:
					case SHLB:
					case SARB:
					case SHRB:
						ultype = UA_DELB;
						break;
	    
					case MULL:
					case IMULL:
					case ANDL:
						/* convert to move zero */
						ultype = UA_MOVZL;
						break;

					case MULW:
					case IMULW:
					case ANDW:
						/* convert to move zero */
						ultype = UA_MOVZW;
						break;
	    
					case MULB:
					case IMULB:
					case ANDB:
						/* convert to move zero */
						ultype = UA_MOVZB;
						break;
				}
				break;		/* done $0 case */

			case 1:				/* &1 case */
				/* branch on op code */
				switch( cop ) {
					case DIVL:
					case IDIVL:
					case MULL:
					case IMULL:
						ultype = UA_DELL;	
						break;
	    
					case DIVW:
					case IDIVW:
					case MULW:
					case IMULW:
						ultype = UA_DELW;	
						break;
	    
					case DIVB:
					case IDIVB:
					case MULB:
					case IMULB:
						ultype = UA_DELB;	
						break;
				}
				break;		/* done $1 case */
	
			case -1:			/* $-1 case */
				/* branch on op code */
				switch ( cop ) {
					case ANDL:
						ultype = UA_DELL;	
						break;
	    
					case ANDW:
						ultype = UA_DELW;	
						break;
	    
					case ANDB:
						ultype = UA_DELB;	
						break;
				}
				break;		/* end $-1 case */
		} /* end switch on immediate value */

/*
** Now do something, based on selections made above
*/

		switch ( ultype ) {
			case UA_MOVZL:	/* change to move zero to operand */
			case UA_MOVZW:	/* change to move zero to operand */
			case UA_MOVZB:	/* change to move zero to operand */

				/* if dest is volatile, don't touch this. */
				if (isvolatile(pf,2)) {
	 				break;
				}

				wchange();
				if (isdeadcc(pf)) {
					/* first operand is zero */
					/* second is ultimate destination */
					pf->op1 = "$0";
					pf->op2 = dst(pf);
					/* clean out op3 if there was one */
					pf->op3 = NULL;

					/* change op code */
					switch ( ultype ) {
						case UA_MOVZL:
							chgop(pf,MOVL,"movl");
							break;
						case UA_MOVZW:
							chgop(pf,MOVW,"movw");
							break;
						case UA_MOVZB:
							chgop(pf,MOVB,"movb");
							break;
					}

				} else if (   cop != ANDL
				           && cop != ANDW
				           && cop != ANDB) {

					/* change op code */
					switch ( ultype ) {
						case UA_MOVZL:
							chgop(pf,ANDL,"andl");
							break;
						case UA_MOVZW:
							chgop(pf,ANDW,"andw");
							break;
						case UA_MOVZB:
							chgop(pf,ANDB,"andb");
							break;
					}
				}
				retval = true;		/* made a change */
				break;
	
/* For this case we must be careful:  if a following instruction is a
** conditional branch, it is clearly depending on the result of the
** arithmetic, so we must put in a compare against zero instead of deleting
** the instruction.
*/

			case UA_DELL:	/* delete instruction */
			case UA_DELW:	/* delete instruction */
			case UA_DELB:	/* delete instruction */

				/* if dest is volatile, don't touch this. */
				if (   isvolatile(pf,2)
				    || isvolatile(pf,3) /* SH[LR]D */) {
					break;
				}

				wchange();	/* we will make a change */

				/* shifts by 0 do not affect the CC. */
				if (   ! isdeadcc(pf)
				    && !(   (cop >= SALB)
				         && (cop <= SHRDL))) {

					if (isreg(pf->op2)) {
						switch ( ultype ) {
							case UA_DELL:
								chgop(pf,TESTL,"testl");
								break;
							case UA_DELW:
								chgop(pf,TESTW,"testw");
								break;
		 					case UA_DELB:
								chgop(pf,TESTB,"testb");
								break;
						}
						pf->op1 = pf->op2;	/* always test second operand */
						pf->op3 = NULL;		/* for completeness */
						retval = true;	/* made a change */
					} else {
						switch ( ultype ) {
							case UA_DELL:
								chgop(pf,CMPL,"cmpl");
								break;
							case UA_DELW:
								chgop(pf,CMPW,"cmpw");
								break;
							case UA_DELB:
								chgop(pf,CMPB,"cmpb");
								break;
						}
						retval = true;
						pf->op1 = "$0";
					}

				} else {
					ldelin2(pf);	/* preserve line number info */
					mvlivecc(pf);	/* preserve cond. codes line info */
					DELNODE(pf);	/* not conditional; delete node */
					return(true);	/* say we changed something */
				}
				break;

		} /* end case that decides what to do */
	
		cop = pf->op;	/* reset current op for changed inst. */

	} /* end useless arithmetic removal */

/*
** Discard useless mov's
**
**	movw	O,O		->	deleted
**
** Don't worry about condition codes, because mov's don't change
** them anyways.
*/

	if (   (   pf->op == MOVB
	        || pf->op == MOVW
	        || pf->op == MOVL)
	    &&  strcmp(pf->op1,pf->op2) == 0
	    &&  !isvolatile(pf,1)  /* non-volatile */ ) {

		wchange();	/* changing the window */
		ldelin2(pf);	/* preserve line number info */
		mvlivecc(pf);	/* preserve condition codes line info */
		DELNODE(pf);	/* delete the movw */
		return(true);
	}


/* For Intel 386, a shift by one bit is more efficiently
** done as an add.
**
**	shll $1,R		->	addl R,R
**
**	shll R			->	addl R,R
**
**    Question whether this peephole is of any benefit for P5 and beyond ??
*/

	if(   (   pf->op == SHLL
	       || pf->op == SHLW
	       || pf->op == SHLB )
	   && (   (   strcmp( pf->op1, "$1" ) == 0 
	           && isreg(tmp = pf->op2)	/* safe from mmio */ )
	       || (   (pf->op2 == NULL) 	/* single operand inst. */
	           && isreg(tmp = pf->op1)	/* safe from mmio */ )) ) {

		if( pf->op == SHLL ) {
			chgop( pf, ADDL, "addl" );
			pf->op1 = pf->op2 = tmp;
			return( true );
		}
		if( pf->op == SHLW ) {
			chgop( pf, ADDW, "addw" );
			pf->op1 = pf->op2 = tmp;
			return( true );
		}
		if( pf->op == SHLB ) {
			chgop( pf, ADDB, "addb" );
			pf->op1 = pf->op2 = tmp;
			return( true );
		}
	}



	return(retval);			/* indicate whether anything changed */
}

extern int auto_elim;
extern unsigned int frame_reg;

static boolean
isaligned(char *op)
{
	int regs = scanreg(op, false);

	if (strtoul(op,(char **)NULL,0) % 4) {
		return false;
	}
	if ((regs & frame_reg) == frame_reg) {
		return true;
	}
	if (   (regs & frame_reg)
	    && (strstr(op,",4") || strstr(op,",8"))) {
		return true;
	}
	return false;
}
