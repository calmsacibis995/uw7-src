#ident	"@(#)optim:i386/w6opt.c	1.1.1.7"
/* w6opt.c
**
**	Intel 386 six-instruction window improver
**
**
**
** This module contains improvements for six instruction windows,
** of which there aren't many.
**
** Specifically *p++ = *q++ type optimizations.
*/

/* #include "defs" -- optim.h takes care of this */
#include "optim.h"
#include "optutil.h"

/* w6opt -- 6-instruction peephole window */

boolean					/* true if changes made */
w6opt(register NODE * pf, register NODE * pl)
/*
**   pf    -  pointer to first inst. in window
**   pl    -  pointer to last inst. in window
*/
{
    register NODE * p2 = pf->forw;	/* point at middle node */
    register NODE * p3 = p2->forw;	/* point at middle node */
    register NODE * p4 = p3->forw;	/* point at middle node */
    register NODE * p5 = p4->forw;	/* point at middle node */

    int cop1 = pf->op;			/* op code number of first inst. */
    int cop2 = p2->op;			/* op code number of second */
    int cop3 = p3->op;			/* ... of third */
    int cop4 = p4->op;			/* ... of fourth */
/*  int cop5 = p5->op;*/		/* ... of fifth */
/*  int cop6 = pl->op;*/		/* ... of sixth */

    int src1, src2;			/* size (bytes) of source of move */
    int dst1, dst2;			/* size of destination of move */

/*
** *p++ improvement
**
**	This transformation is for the following code, which
**	is known to be heavily used:
**		*p++ = *q++;
**
**	The instruction sequence that is sought after is:
**
**		movl	reg1,reg2	\
**		incl	reg1		| movX	(reg3),reg5
**		movl	reg3,reg4	| movX	reg5,(reg1)
**		incl	reg3		| incl	reg1
**		movX	(reg4),reg5	| incl	reg3
**		movX	reg5,(reg2)	/
*/

	if (   cop1 == MOVL
	    && cop2 == INCL
	    && cop3 == MOVL
	    && cop4 == INCL
	    && ismove(p5,&src1,&dst1)
	    && ismove(pl,&src2,&dst2)
	    && isreg(pf->op1)
	    && isreg(pf->op2)
	    && isreg(p2->op1)
	    && strcmp(pf->op1,p2->op1) == 0
	    && isreg(p3->op1)
	    && isreg(p3->op2)
	    && isreg(p4->op1)
	    && strcmp(p3->op1,p4->op1) == 0
	    && isreg(p5->op2)
	    && isreg(pl->op1)
	    && strcmp(p5->op2,pl->op1) == 0
	    && iszoffset(p5->op1,p3->op2)
	    && iszoffset(pl->op2,pf->op2)
	    && dst1 == src2
	    && src2 >= dst2
	    && isdead(pf->op2,pl)
	    && isdead(p3->op2,pl)) {

		wchange();			/* change the window */
		lmrgin1(pl,pf,pf);		/* preserve line number info */
		p5->op1 = getspace(NEWSIZE);	/* movX (reg3),reg5 */
		p5->op1[0] = '(';
		p5->op1[1] = '\0';
		strcat(p5->op1,p3->op1);
		strcat(p5->op1,")");
		pl->op2 = getspace(NEWSIZE);	/* movX reg5,(reg1) */
		pl->op2[0] = '(';
		pl->op2[1] = '\0';
		strcat(pl->op2,pf->op1);
		strcat(pl->op2,")");
		DELNODE(pf);		/* delete the extraneous moves */
		DELNODE(p3);
		exchange(p4);		/* bubble the incls down the line */
		exchange(p4);
		exchange(p2);
		exchange(p2);
		return(true);
	}

/*
** *p++ improvement
**
**	This transformation is for the following code, which
**	is known to be heavily used:
**		*p++ = *q++;
**
**	The instruction sequence that is sought after is:
**
**		movl	reg1,reg2	\
**		addl	$X,reg1		| movX	(reg3),reg5
**		movl	reg3,reg4	| movX	reg5,(reg1)
**		addl	$X,reg3		| addl	$X,reg1
**		movX	(reg4),reg5	| addl	$X,reg3
**		movX	reg5,(reg2)	/
*/

	if (   cop1 == MOVL
	    && cop2 == ADDL
	    && cop3 == MOVL
	    && cop4 == ADDL
	    && ismove(p5,&src1,&dst1)
	    && ismove(pl,&src2,&dst2)
	    && isreg(pf->op1)
	    && isreg(pf->op2)
	    && isnumlit(p2->op1)
	    && isreg(p2->op2)
	    && strcmp(pf->op1,p2->op2) == 0
	    && isreg(p3->op1)
	    && isreg(p3->op2)
	    && isnumlit(p4->op1)
	    && isreg(p4->op2)
	    && strcmp(p3->op1,p4->op2) == 0
	    && strcmp(p2->op1,p4->op1) == 0
	    && isreg(p5->op2)
	    && isreg(pl->op1)
	    && strcmp(p5->op2,pl->op1) == 0
	    && iszoffset(p5->op1,p3->op2)
	    && iszoffset(pl->op2,pf->op2)
	    && dst1 == src2
	    && src2 >= dst2
	    && isdead(pf->op2,pl)
	    && isdead(p3->op2,pl)) {

		wchange();			/* change the window */
		lmrgin1(pl,pf,pf);		/* preserve line number info */
		p5->op1 = getspace(NEWSIZE);	/* movX (reg1),reg5 */
		p5->op1[0] = '(';
		p5->op1[1] = '\0';
		strcat(p5->op1,p3->op1);
		strcat(p5->op1,")");
		pl->op2 = getspace(NEWSIZE);	/* movX reg5,(reg3) */
		pl->op2[0] = '(';
		pl->op2[1] = '\0';
		strcat(pl->op2,pf->op1);
		strcat(pl->op2,")");
		DELNODE(pf);		/* delete the extraneous moves */
		DELNODE(p3);
		exchange(p4);		/* bubble the incls down the line */
		exchange(p4);
		exchange(p2);
		exchange(p2);
		return(true);
	}

/*
** *p++ improvement
**
**	This transformation is for the following code, which
**	is known to be heavily used:
**		*p++ op *q++;
**
**	The instruction sequence that is sought after is:
**
**		movl	reg1,reg2	\
**		addl	$X,reg1		| useX	(reg1)
**		movl	reg3,reg4	| useX	(reg3)
**		addl	$X,reg3		| addl	$X,reg1
**		useX	(reg2) 		| addl	$X,reg3
**		useX	(reg4)		/
**
**	EXAMPLE from spec/052.alvinn/backprop.c:
**
**	movl	%edi,%eax	flds	(%edi)
**	addl	$4,%edi 	fmuls	(%ebi)
**	movl	%ebi,%edx 	addl	$4,%edi
**	addl	$4,%ebi  	addl	$4,%ebi
**	flds	(%eax)  
**	fmuls	(%edx) 
*/
	if (   cop1 == MOVL
	    && cop2 == ADDL
	    && cop3 == MOVL
	    && cop4 == ADDL
	    && isreg(pf->op1)
	    && isreg(pf->op2)
	    && isnumlit(p2->op1)
	    && isreg(p2->op2)
	    && strcmp(pf->op1,p2->op2) == 0
	    && isreg(p3->op1)
	    && isreg(p3->op2)
	    && isnumlit(p4->op1)
	    && isreg(p4->op2)
	    && strcmp(p3->op1,p4->op2) == 0
	    && iszoffset(p5->op1,pf->op2) /* p5->op1 = "(pf->op2)" */
	    && iszoffset(pl->op1,p3->op2)
	    && isdead(pf->op2,pl)
	    && isdead(p3->op2,pl)
	    && ! (p5->sets & setreg(pf->op2))
	    && ! (pl->sets & setreg(p3->op2))) {

		wchange();			/* change the window */
		lmrgin1(pl,pf,pf);		/* preserve line number info */
		p5->op1 = getspace(NEWSIZE);	/* useX (reg1) */
		p5->op1[0] = '(';
		p5->op1[1] = '\0';
		strcat(p5->op1,pf->op1);
		strcat(p5->op1,")");
		pl->op1 = getspace(NEWSIZE);	/* useX (reg3) */
		pl->op1[0] = '(';
		pl->op1[1] = '\0';
		strcat(pl->op1,p3->op1);
		strcat(pl->op1,")");
		DELNODE(pf);		/* delete the extraneous moves */
		DELNODE(p3);
		exchange(p4);		/* bubble the incls down the line */
		exchange(p4);
		exchange(p2);
		exchange(p2);
		new_sets_uses(p2);
		new_sets_uses(p4);
		new_sets_uses(p5);
		new_sets_uses(pl);
		return(true);
	}

	return(false);
}
