#ident	"@(#)optim:i386/w4opt.c	1.1.2.25"
/* w4opt.c
**
**	Intel 386 four-instruction window improver
**
**
**
** This module contains improvements for four instruction windows,
** of which there aren't many.
**
*/

/* #include "defs" -- optim.h takes care of this */
#include "optim.h"
#include "optutil.h"
extern int jflag;
extern struct RI *reginfo;

/* w4opt -- 4-instruction peephole window */

boolean					/* true if changes made */
w4opt(register NODE * pf, register NODE * pl)
/*
**  pf  -  pointer to first inst. in window
**  pl  -  pointer to last inst. in window
*/
{
    register NODE * p2 = pf->forw;	/* point at middle node */
    register NODE * p3 = p2->forw;	/* point at middle node */

    int cop1 = pf->op;			/* op code number of first inst. */
    int cop2 = p2->op;			/* op code number of second */
    int cop3 = p3->op;			/* ... of third */
    int cop4 = pl->op;			/* ... of fourth */

/*
** op= improvement
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	movl	O1,R1
**	op1X	R2,R1		->	op1X R2,O1
**	movl	R1,O1
**	op2X	R1,R3		->	op2X O1,R3
**
**	if R1 is dead after the sequence
**
*/
	if (   (   target_cpu == P4
	        || jflag)
	    && cop1 == MOVL
	    && cop3 == MOVL
	    && (   is2dyadic(pl)
	        || cop4 == PUSHW
	        || cop4 == PUSHL)
	    && isreg(pf->op2)
	    && isreg(p2->op1)
	    && isreg(p2->op2)
	    && isreg(p3->op1)
	    && isreg(p2->op2)
	    && isreg(pl->op1)
	    && (   pl->op2 == NULL
	        || isreg(pl->op2))
	    && strcmp(pf->op2,p2->op2) == 0
	    && strcmp(p2->op2,p3->op1) == 0
	    && strcmp(p3->op1,pl->op1) == 0
	    && strcmp(pf->op1,p3->op2) == 0
	    && ! strcmp(p2->op1,pf->op2) == 0
	    && (   (pl->op2 == NULL)
	        || (! strcmp(pl->op2,pf->op2) == 0))
	    && isdead(pl->op1,pl)
	    && ! usesreg(pf->op1, pf->op2)
	    && ! isvolatile(pf,1)		/* non-volatile */
					) {
		if (   cop2 == ADDB || cop2 == ADDW || cop2 == ADDL
		    || cop2 == ANDB || cop2 == ANDW || cop2 == ANDL
		    || cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL
		    || cop2 == SARB || cop2 == SARW || cop2 == SARL
		    || cop2 == SHLB || cop2 == SHLW || cop2 == SHLL
		    || cop2 == SHRB || cop2 == SHRW || cop2 == SHRL
		    || cop2 == SUBB || cop2 == SUBW || cop2 == SUBL
		    || cop2 == XORB || cop2 == XORW || cop2 == XORL) {

			wchange();		/* change the window */
			lmrgin1(p3,pl,p2);	/* preserve line number info */
			p2->op2 = pf->op1;
			pl->op1 = pf->op1;	/* Move fields around */
			makedead(pf->op2,p2);
			makelive(pf->op1,p2);
			DELNODE(pf);
			DELNODE(p3);
			return(true);
		}
	}

/* cmp  jcc cmp jcc improvement
**
** cmp reg1,X
** jcc
** cmp reg1,X   - delete
** jmp
*/
	if (   pf->op == CMPL
	    && p3->op == CMPL
	    && iscbr(p2)
	    && isbr(pl)
	    && !strcmp(pf->op1,p3->op1)
	    && !strcmp(pf->op2,p3->op2)
	    && !isvolatile(p3,2)) {

		wchange();
		DELNODE(p3);
		lexchin(p3,pl);
		if (iscbr(pl)) {
			p2->nlive |= CONCODES_AND_CARRY;
		}
		return true;
	 }

/* made by loop unroll:  but also possible from other "dumb" code
**
** addl r1,r2 -> leal (,r1,4),r2
** addl r1,r2
** addl r1,r2
** addl r1,r2
*/
	if (   cop1 == ADDL
	    && cop2 == ADDL
	    && cop3 == ADDL
	    && cop4 == ADDL
	    && isreg(pf->op1)
	    && isreg(p2->op1)
	    && isreg(p3->op1)
	    && isreg(pl->op1)
	    && isreg(pf->op2)
	    && isreg(p2->op2)
	    && isreg(p3->op2)
	    && isreg(pl->op2)
	    && !strcmp(pf->op1,p2->op1)
	    && !strcmp(pf->op1,p3->op1)
	    && !strcmp(pf->op1,pl->op1)
	    && !strcmp(pf->op2,p2->op2)
	    && !strcmp(pf->op2,p3->op2)
	    && !strcmp(pf->op2,pl->op2)
	    && !samereg(pf->op1,pf->op2)
	    && isdeadcc(pl)) {

		wchange();
		chgop(pf,LEAL,"leal");
		pf->op1 = getspace(NEWSIZE);
		sprintf(pf->op1,"(%s,%s,4)",pf->op2,p2->op1);
		DELNODE(p2);
		DELNODE(p3);
		DELNODE(pl);
		return true;
	  }

/*
** short - divide improvement:
** movswl %r16,%eax  -> movw %r16,%ax
** movswl %r16,%ecx  -> if (ecx dead after divide) delnode else movw %r16,%cx
** cltd              -> cwtd
** idivl  %ecx       -> idivw p2->op1 or %cx.
**
*/
	if (   !eiger
	    && cop1 == MOVSWL
	    && cop2 == MOVSWL
	    && cop3 == CLTD
	    && cop4 == IDIVL
	    && !strcmp(pl->op1, "%ecx")) {

		NODE *t1, *t2;

		/*t1 points to mov *, %ecx and t2 points to mov *, %eax */
		if (   !strcmp(pf->op2,"%eax")
		    && !strcmp(p2->op2,"%ecx")) {
			t1 = p2;
			t2 = pf;

		} else if (   !strcmp(p2->op2,"%eax")
		           && !strcmp(pf->op2,"%ecx")) {
			t1 = pf;
			t2 = p2;
		} else {
			goto div_out;
		}
		wchange();
		/* The idiv[lw] destroys the value in %eax.  Whether
		   the result of the divide is 32 or 16 bit has no bearing
		   on whether the word loaded into %eax needs to be sign
		   extended.  A 16 bit divide is sufficient for a 16 bit
		   dividend and divisor.

		   What may be an issue, though unlikely, is if the movswl
		   into %ecx depends on %eax for the source operand
		   reference. */
		if ( ! (   t1 == p2
		        && (uses(t1) & (EAX & ~ AX)))) {

			chgop(t2,MOVW,"movw");
			t2->op2 = "%ax";
		}
		chgop(p3,CWTD,"cwtd");
		chgop(pl,IDIVW,"idivw");

		/* If %ecx (or part of it) is used after the divide or t1
		   uses EAX or EDX in the source operand, %ecx
		   or %cx must still be loaded.  Otherwise, use t1->op1
		   as the operand in the divide. */
		if (   (pl->nlive & ECX)
		    || uses(t1) & (EDX|EAX)) {

			/* If the 32 bits of %ECX lives beyond the
			   divide, then the sign extension must still be done.
			*/
			if (!(pl->nlive & (ECX & ~CX))) {
				chgop(t1,MOVW,"movw");
				t1->op2 = "%cx";
			}
			pl->op1 = "%cx";
		} else {
			pl->op1 = t1->op1;
			DELNODE(t1);
		}

		/* One last check!  If 32 bits of the quotient/remainder
		   are needed after the divide instruction, then the 16 bit
		   results must be sign extended. */
		if (pl->nlive & (EDX & ~DX)) {
                        NODE *pnew;

			pnew = insert(pl);
			chgop(pnew, MOVSWL, "movswl");
			pnew->op1 = "%dx";
			pnew->op2 = "%edx";
			pnew->nlive = pl->nlive;
		}
		if (pl->nlive & (EAX & ~AX)) {
                        NODE *pnew;

			pnew = insert(pl);
			chgop(pnew, MOVSWL, "movswl");
			pnew->op1 = "%ax";
			pnew->op2 = "%eax";
			pnew->nlive = pl->nlive;
		}
		return true;
	}
div_out:

/* prevent register re-use to acheive data independence and better 
** scheduling opportinities. This happens in calling sequences under
** fixed frame style:
**
**    movl M1,reg1
**    movl reg1,M2
**    movl M3,reg1  movl M3,reg2
**    movl reg1,M4  movl reg2,M4
*/
	if (   (   target_cpu == P5
	        || target_cpu == blend)
	    && cop1 == MOVL
	    && ismem(pf->op1)
	    && isreg(pf->op2)
	    && cop3 == MOVL
	    && ismem(p3->op1)
	    && isreg(p3->op2)
	    && strcmp(pf->op2,p3->op2) == 0
	    && strcmp(pf->op1,p3->op1) != 0
	    && isdyadic(p2)
	    && isreg(p2->op1)
	    && !strcmp(pf->op2,p2->op1)
	    && isdyadic(pl)
	    && isreg(pl->op1)
	    && !strcmp(pf->op2,pl->op1)
	    && !(pl->nlive & setreg(pf->op2))) {

		int rn = get_free_reg( pf->sets|pf->uses|pf->nlive, 1, pf);

		if( rn == -1 ) {
			goto mov_out;
		}
		wchange();
		p3->op2 = pl->op1 = reginfo[rn].regname[0];
		p3->nlive |= reginfo[rn].reg_number;
		return true;
	 }
mov_out:

    return(false);
}
