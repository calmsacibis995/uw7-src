#ident	"@(#)optim:i386/w5opt.c	1.15"
/* w5opt.c
**
**	Intel 386 five-instruction window improver
**
**
**
** This module contains improvements for five instruction windows,
** of which there aren't many.
**
*/

/* #include "defs" -- optim.h takes care of this */
#include "optim.h"
#include "optutil.h"
extern int jflag;
extern int ieee_flag;
extern struct RI *reginfo;


/* w5opt -- 5-instruction peephole window */

boolean					/* true if changes made */
w5opt(register NODE * pf, register NODE * pl)
/*
**  pf   -  pointer to first inst. in window
**  pl   -  pointer to last inst. in window
*/
{
    boolean is_pos_ro();
    unsigned long val;
    NODE *new1,*new2,*new3;
    register NODE * p2 = pf->forw;	/* point at middle node */
    register NODE * p3 = p2->forw;	/* point at middle node */
    register NODE * p4 = p3->forw;	/* point at middle node */

    int cop1 = pf->op;			/* op code number of first inst. */
    int cop2 = p2->op;			/* op code number of second */
    int cop3 = p3->op;			/* ... of third */
    int cop4 = p4->op;			/* ... of fourth */

/*
**	fld something    
**	fcomps .LX.RO   
**	fstsw %ax          ->   cmpl $val of LX.RO ,soemthing
**	sahf					
**	jbe                ->   jle  /etc...
**
** if .LX.OR == 0 and 	the jump is JGE, JE, JZ or JNL
**
**	fld something    
**	fcomps .LX.RO   
**	fstsw %ax           ->   cmpl $val of LX.RO ,soemthing
**	sahf
**	jbe   L             ->   jle  L /etc...
**                           cmpl $-2147483648 ,soemthing
**							 JNE   L
**
** if .LX.OR == 0 and 	the jump is JGE, JE, JZ or JNL
**
**	fld something    
**	fcomps .LX.RO   
**	fstsw %ax           ->   cmpl $0,soemthing
**	sahf
**	jae   L             ->   jge  L /etc...
**                           cmpl $-2147483648 ,soemthing
**							 JE   L
**
** if .LX.OR == 0 and 	the jump is  JL,JNGE,JNZ or JNE
**
**	fld something    
**	fcomps .LX.RO   
**	stsw %ax           ->   cmpl $0,soemthing
**	sahf
**	jb    L             ->   jnl  Z /etc...
**                           cmpl $-2147483648 ,soemthing
**							 JNE   L
*/
	if (   !ieee_flag
	    && cop1 == FLDS
	    && cop2 == FCOMPS
	    && (   cop3 == FSTSW
	        || cop3 == FNSTSW)
	    && cop4 == SAHF
	    && is_pos_ro(p2->op1,&val)
	    && is_unsable_br(pl)) {

		wchange();
		DELNODE(pf);
		DELNODE(p2);
		chgop(p3,CMPL,"cmpl");
		p3->op1 = getspace(ADDLSIZE);
		sprintf(p3->op1,"$%d",val); 
		p3->op2 = pf->op1;
		DELNODE(p4);
		unsign2sign(pl);
		pf->nlive |= setreg(pf->op2);
		p3->nlive |= CONCODES_AND_CARRY;

		if (   val
		    || pl->op == JG || pl->op == JNG
		    || pl->op == JLE || pl->op == JNLE) {
			return true;
		}
		/* fcomp with zero extra work */
		new1 = Saveop(0,"",0,GHOST);   /* create isolated node, init. */
		APPNODE(new1, pl);	/* append new node after current */
		chgop(new1,CMPL,"cmpl");
		new1->op1 = "$-2147483648"; /* -0 or 0x80000000 */
		new1->op2 = p3->op2; 
		new1->nlive = p3->nlive;
		new2 = Saveop(0,"",0,GHOST);/* create isolated node, init. */
		APPNODE(new2, new1);	/* append new node after current */
		new2->nlive = pl->nlive;
		new2->op1 = pl->op1;
		p3->nlive |= scanreg(p3->op2,false);   
		pl->nlive |= p3->nlive;
		switch (pl->op) {
			case JGE:
			case JE:
			case JZ:
			case JNL:
				chgop(new2,JE,"je");
				break;
			case JL:
			case JNGE:
			case JNZ:
			case JNE:
				/* create isolated node, init. */
				new3 = Saveop(0,"",0,GHOST);
				/* append new node after current */
				APPNODE(new3, new2);
				chgop(new2,JNE,"jne");
				new3->opcode = newlab();;
				new3->op = LABEL;
				pl->op1 = new3->opcode;
				revbr(pl);
				break;
			default:
				fatal(__FILE__,__LINE__,
				     "non valid op %s for fcomp \n",pl->opcode);
		}
		return true;
	}

/*
**	fxch %st(1)
**	fcompp     		fcompp
**	fnstsw	%ax		fnstsw	%ax
**	sahf       		sahf
**	jcc    	   		jRcc
*/
	if (   cop1 == FXCH
	    && cop2 == FCOMPP
	    && (cop3 == FNSTSW || cop3 == FSTSW)
	    && cop4 == SAHF
	    && iscbr(pl)
	    && pf->uses & FP1
	    && !strcmp(p3->op1,"%ax")
	    && is_fp_commutative(pl)) {

		DELNODE(pf);
		invbr(pl);
		return true;
	}

/*
**	fld     a
**	fcompp     		fcomp	a
**	fnstsw	%ax		fnstsw	%ax
**	sahf       		sahf
**	jcc        		jRcc
*/
	if (   (   cop1 == FLDL
	        || cop1 == FLDS)
	    && cop2 == FCOMPP
	    && (cop3 == FNSTSW || cop3 == FSTSW)
	    && cop4 == SAHF
	    && iscbr(pl)
	    && !strcmp(p3->op1,"%ax")
	    && is_fp_commutative(pl)) {

		p2->op1 = pf->op1;
		if (pf->op == FLDL) {
			chgop(p2,FCOMPL,"fcompl");
		} else {
			chgop(p2,FCOMPS,"fcomps");
		}
		DELNODE(pf);
		invbr(pl);
		return true;
	}

	return(false);
}
