#ident	"@(#)cg:common/reader.c	1.25.2.40"

# include <unistd.h>
# include "mfile2.h"
#ifndef	NODBG
/*========= DEBUGGING */
FILE * debugout;
#include <fcntl.h>
#define DEBUGOUT 9		/* debug output file number */
/* ======= END DEBUGGING */
#endif

FILE * debugfile = stdout;	/* For CG debugging */
extern int tmp_start;		/* For CG:  start of temp offsets */

/*	some storage declarations */

int lflag;
int e2debug;
int udebug;
int fast;
#ifdef IN_LINE
int asmdebug;
#endif

/* maxtemp is the maximum size (in bits) needed for temps so far */
int maxtemp;
NODE * condit(), *leteff();
static NODE * seq();
static int cond();

static NODE *
force(p, strat)
register NODE *p; 
int strat;
{
	register NODE *q, *r;
	if( !p ) cerror( "force" );
	q = talloc();
	*q = *p;
	r = talloc();
	*r = *p;
	q->tn.op = ASSIGN;
	q->in.right = p;
	q->in.left = r;
	r->tn.op = QNODE;
	r->tn.sid = callreg(p); /* the reg where the value will be forced */
	q->in.strat = r->in.strat = strat;
	return( q );
}

static NODE *
dlabel( p, l )
register NODE *p; 
{
	/* define a label after p is executed */
	register NODE *q;
	if( !p ) cerror( "dlabel" );
	q = talloc();
	q->tn.type = p->tn.type;
	q->in.left = p;
	q->tn.op = GENLAB;
	q->bn.c.label = l;
	return( q );
}


static NODE *
genbr( o, l, p )
register NODE *p; 
register o,l;
{
	/* after evaluating p, generate a branch to l */
	/* if o is 0, unconditional */
	register NODE *q;
	int pop;

	if( !p ) cerror( "genbr" );
	if( l < 0 ) cerror( "genbr1" );
	q = talloc();
	q->tn.op = o?GENBR:GENUBR;
	q->tn.type = p->tn.type;
	q->in.left = p;
	q->bn.c.label = l;
	q->bn.lop = o;
#ifdef IEEE
/* For IEEE standard, have to differentiate between floating point comparisions:
/* CMP for no exception raised on non-trapping NaN's, used for == and != ;
/* CMPE to raise exceptions for all NaN's, used for all other relations.
*/
/* CG:  don't do this if exceptions are to be ignored */
	if (o && logop(p->tn.op)) {
		/* for multiword compares, let the CMP[E] know these, too */
		p->tn.lop = o;
		p->tn.c.label = l;
		/* Believe explicit EX... bits */
		if (p->in.strat & EXIGNORE)
			p->tn.op = CMP;
		else if (p->in.strat & EXHONOR)
			p->tn.op = CMPE;
		else switch (p->tn.op) {
		default:
			p->tn.op = CMP;
			if (p->in.left->tn.type
				& (TFLOAT|TDOUBLE|TLDOUBLE)
			    || p->in.right->tn.type
				& (TFLOAT|TDOUBLE|TLDOUBLE))
			{
				p->tn.op = CMPE;
			}
			break;
		case BCMP:
		case ANDAND:
		case OROR:
			break;
		case EQ:
		case NE:
		case NLE: case UNLE:
		case NLT: case UNLT:
		case NGE: case UNGE:
		case NGT: case UNGT:
		case NLG: case ULGE:
		case NLGE: case UNLGE:
			/* These should not raise exceptions. */
			p->tn.op = CMP;
			break;
		}
	}
#else
	if( o && logop( pop=p->tn.op )
		&& (pop != BCMP)
		&& (pop != ANDAND) 
	        && (pop != OROR)
	) {
		p->tn.lop = o;
		p->tn.c.label = l;
		p->tn.op = CMP;
	}
#endif
	return( q );
}


static NODE *
oreff(p)
register NODE *p;
{
	register NODE *r, *l;
	NODE *condit();
	int lab;
	/* oreff is called if an || op is evaluated with goal=CEFF
	   The rhs of || ops should be executed only if the
	   lhs is false.  Since our goal is CEFF, we don't need
	   a result of the ||, but we need to
	   preserve that dependancy with this special case */

	/* We must catch this case before its children are
	   condit() and change the goal on it left child to CCC */
	   
	switch( cond(p->in.left) ){
	case 1:				/* always true */
		tfree(p->in.right);
		nfree(p);
		p = condit( p->in.left, CEFF, -1, -1);
		break;
	case 0:				/* always false */
		p->in.op = COMOP;
		p = condit( p, CEFF, -1, -1);
		break;
	default:			/* don't know */
		lab = getlab();
		nfree(p);
		l = p->in.left;
		r = condit( p->in.right, CEFF, -1, -1);
		/* what to do with left depends on whether right does anything:
		** if right side is null, do left purely for side effects
		*/
		if (r)
		    l = condit( l, CCC, lab, -1);
		else
		    l = condit( l, CEFF, -1, -1);
		p = seq(l, r);	/* put r after l */
		/* generate a label if anything done */
		if (r)
		    p = dlabel(p, lab);
	}
	return p;
}
static NODE *
andeff(p)
register NODE *p;
{
	register NODE *r, *l;
	NODE *condit(); 
	int lab;
	/* andeff is called if an && op is evaluated with goal=CEFF
	   The rhs of && ops should be executed only if the
	   lhs is true.  Since our goal is CEFF, we don't need
	   a result of the &&, but we need to
	   preserve that dependancy with this special case */

	/* We must catch this case before its children are
	   condit() and change the goal on it left child to CCC */
	   
	switch( cond(p->in.left) ){
	case 0:				/* always false */
		tfree(p->in.right);
		nfree(p);
		p = condit( p->in.left, CEFF, -1, -1);
		break;
	case 1:				/* always true */
		p->in.op = COMOP;
		p = condit( p, CEFF, -1, -1);
		break;
	default:
		lab = getlab();
		nfree(p);
		l = p->in.left;
		r = condit( p->in.right, CEFF, -1, -1);
		/* what to do with left depends on whether right does anything:
		** if right side is null, do left purely for side effects
		*/
		if (r)
		    l = condit( l, CCC, -1, lab);
		else
		    l = condit( l, CEFF, -1, -1);
		p = seq(l, r);	/* put r after l */
		/* generate a label if anything done */
		if (r)
		    p = dlabel(p, lab);
	}
	return p;
}
/* In order, the equality and relational ops now are:
**   EQ,   NE,
**   LE,  NLE,  LT,  NLT,  GE,  NGE,  GT,  NGT,  LG,  NLG,  LGE,  NLGE,
**  ULE, UNLE, ULT, UNLT, UGE, UNGE, UGT, UNGT, ULG, UNLG, ULGE, UNLGE
*/
static const int negrel[] = /* negatives of relationals */
{
     NE,  EQ,
    NLE,  LE,  NLT,  LT,  NGE,  GE,  NGT,  GT,  NLG,  LG,  NLGE,  LGE,
   UNLE, ULE, UNLT, ULT, UNGE, UGE, UNGT, UGT, UNLG, ULG, UNLGE, ULGE
};

static const int commrel[] = /* commutatives of relationals */
{
     EQ,   NE,
     GE,  NGE,  GT,  NGT,  LE,  NLE,  LT,  NLT,  LG,  NLG,  LGE,  NLGE,
    UGE, UNGE, UGT, UNGT, ULE, UNLE, ULT, UNLT, ULG, UNLG, ULGE, UNLGE
};

static const int flatrel[] = /* "flattened" relationals for integers */
{
     EQ,  NE,
     LE,  GT,  LT,  GE,  GE,  LT,  GT,  LE, NE, EQ, -1, -2,
    ULE, UGT, ULT, UGE, UGE, ULT, UGT, ULE, NE, EQ, -1, -2
};

#define T_UNSIGNED(t)	\
	((t) & (TUCHAR|TUSHORT|TUNSIGNED|TULONG|TULLONG|TPOINT|TPOINT2))
#define T_SIGNED(t)	((t) & (TCHAR|TSHORT|TINT|TLONG|TLLONG))
#define T_INTEGER(t)	\
	((t) & (TCHAR|TUCHAR|TSHORT|TUSHORT|TINT|TUNSIGNED|TLONG|TULONG|TLLONG|TULLONG))


/* Check a tree.  Return 1 if it always evaluates to non-zero,
** 0 if it always evaluates to 0, -1 if we can't tell.
*/
static
cond( p )
register NODE *p; 
{
	register o = p->tn.op;
	register NODE *q;
	int newcond;

#ifndef NODBG
	if (odebug > 2) printf("cond(%d)\n", node_no(p));
#endif
	switch( o ) 
	{

	case ICON:
		/* Name constant may be non-zero due to weak symbols */
		if (p->tn.name != (char *) 0)
		    return -1;
		return num_ucompare(&p->tn.c.ival, &num_0) != 0;

	case COMOP:
	case SEMI:
		return( cond( p->in.right ) );

	case LGE:
	case ULGE:
		if (T_INTEGER(p->in.left->tn.type))
			return 1;
		return -1;

	case NLGE:
	case UNLGE:
		if (T_INTEGER(p->in.left->tn.type))
			return 0;
		return -1;

	case ANDAND:
		switch( cond(p->in.left) ){
		case 0:		return( 0 );
		case 1:		return( cond(p->in.right) );
		default:
		    if  (cond(p->in.right) == 0)
			return( 0 );
		    return( -1 );	/* don't know */
		}

	case OROR:
		switch( cond(p->in.left) ){
		case 1:		return( 1 );
		case 0:		return( cond(p->in.right) );
		default:
		    if (cond(p->in.right) > 0)
			return( 1 );
		    return( -1 );	/* don't know */
		}

	case NOT:
		switch( cond(p->in.left) ){
		case 1:		return( 0 );
		case 0:		return( 1 );
		default:	return( -1 );	/* don't know */
		}

	case QUEST:
		q = p->in.right;
		switch( cond(p->in.left) ){
		case 1:		return( cond(q->in.left) );
		case 0:		return( cond(q->in.right) );
		default:
		    /* Check if both sides return same value. */
		    newcond = cond(q->in.left);
		    if (newcond >= 0) {
			if (newcond == cond(q->in.right))
			    return( newcond );
		    }
		    return( -1 );	/* don't know */
		}

	default:
		return( -1 );
	}
}

static NODE *
rcomma( p )
register NODE *p; 
{
	/* p is a COMOP (or SEMI); return the shrunken version thereof */

	NODE *alive;
	if (!p) return( p );		/* NILs remain so */
	if( p->tn.op != COMOP && p->tn.op != SEMI ) cerror( "rcomma" );

	if( p->in.left && p->in.right ) return( p );
	alive = (p->in.left) ? p->in.left : p->in.right;

		/*For CG: if we delete a paren'ed node, we must parenthesize
		  the node under it (if there is a node under it)*/
	if (alive && (p->in.strat & PAREN))
		alive->in.strat |= PAREN;
	nfree(p);
	return( alive );
}
static NODE *
seq( p1, p2 )
register NODE *p1, *p2;
{
	/* execute p then q */
	register NODE *q;

	if (!p1) return p2;
	if (!p2) return p1;
	q = talloc();
	q->in.op = COMOP;
	q->in.type = p2->in.type;
	q->in.left = p1;
	q->in.right = p2;
	return q;
}
static NODE *
gtb( p, l )
register NODE *p; 
register l;
{
	register NODE *q;
	/* replace p by a trivial branch to l */
	/* if l is -1, return NULL */
	q = condit( p, CEFF, -1, -1 );
	if( l<0 ) return( q );
	if( !q ) 
	{
		q = talloc();
		q->tn.op = NOP;
		q->tn.c.off = 0;
		q->tn.name = (char *) 0;
		q->tn.type = TINT;
	}
	return( genbr( 0, l, q ) );
}

NODE *
condit( p, goal, t, f )
register NODE *p; 
register goal,t,f;
{
	/* generate code for conditionals in terms of GENLAB and GENBR nodes */
	/* goal is either CEFF, NRGS, or CCC */
	/* also, delete stuff that never needs get done */
	/* if goal==CEFF, return of null means nothing to be done */

	register o, lt, lf, l;
	register NODE *q, *q1, *q2;

	o = p->tn.op;

#ifndef NODBG
	if( odebug ) 
	{
		fprintf(outfile, "condit( %d (%s), %s, %d, %d )\n", (int)node_no(p),
		opst[o], goal==CCC?"CCC":(goal==NRGS?"NRGS":"CEFF"),
		t, f );
	}
#endif
	if( o == CBRANCH ) 
	{
		nfree(p);
		l = p->in.c.label;
		p = p->in.left;
		switch( cond(p) ){
		case 1:		return( gtb(p,-1) );	/* always true */
		case 0:		return( gtb(p,l) );	/* always false */
		default:				/* don't know */
		    return( condit( p, CCC, -1, l ) );
		}
	}

	/* a convenient place to diddle a few special ops */
	if( callop(o) )
	{
		if ( optype(o) == BITYPE )
		    p->stn.argsize = argsize(p->in.right);
		else
		    p->stn.argsize = 0;
		if( goal==CEFF ) goal = NRGS;
		/* flow on, so that we can handle if( f(...) )... */
	}
	else if (   goal == CEFF
		 && (
			/*If DOEXACT bit is on, don't edit this out*/
			(p->in.strat & DOEXACT)
		     || asgop(o)
		     || o == STASG
		     || o == INIT
		     )
		)
		goal=NRGS;

	/* do a bit of optimization */

	if( goal == NRGS ) 
	{
		if( logop(o) )
		{
			/* must make p into ( p ? 1 : 0 ), then recompile */
			q1 = talloc();
			q1->tn.op = ICON;
			q1->tn.name = (char *) 0;
			q1->tn.c.ival = num_1;
			q1->tn.type = p->tn.type;
			q2 = talloc();
			*q2 = *q1;
			q2->tn.c.ival = num_0;
			q = talloc();
			q->tn.op = COLON;
			q->tn.type = p->tn.type;
			q->in.left = q1;
			q->in.right = q2;
			q1 = talloc();
			q1->tn.op = o = QUEST;
			q1->tn.type = p->tn.type;
			q1->in.left = p;
			q1->in.right = q;
			p = q1;  /* flow on, and compile */
		}
	}

	if( goal != CCC ) 
	{
		if( o == QUEST ) 
		{
			/* rewrite ? : when goal not CCC */
			lf = getlab();
			l = getlab();
			p->tn.op = COMOP;
			q = p->in.right;
			q1 = condit( q->in.left, goal, -1, -1 );
			q->in.right = condit( q->in.right, goal, -1, -1 );
			switch( cond(p->in.left) ){
			case 1:				/* always true */
				nfree(q);
				tfree( q->in.right );
				p->in.right = q1;
				p->in.left=condit( p->in.left, CEFF, -1, -1 );
				return( rcomma( p ) );
			case 0:				/* always false */
				nfree(q);
				tfree( q1 );
				p->in.right = q->in.right;
				p->in.left=condit( p->in.left, CEFF, -1, -1 );
				return( rcomma( p ) );
			}
			/* Don't know disposition of condition. */
			if( !q1 ) 
			{
				if( !q->in.right ) 
				{
 					/* may still have work to do
 					** if left side of ? has effect
 					*/
 					q1 = condit(p->in.left, goal,
						-1, -1);
 					if (!q1)
 					{
 						tfree( p->in.left );
 					}
					nfree(p);
					nfree(q);
					return( q1 );
				}
				/* rhs done if condition is false */
				p->in.left = condit( p->in.left, CCC, l, -1 );
				p->in.right = dlabel( q->in.right, l );
				nfree(q);
				return( rcomma( p ) );
			}
			else if( !q->in.right ) 
			{
				/* lhs done if condition is true */
				p->in.left=condit( p->in.left, CCC, -1, lf );
				p->in.right = dlabel( q1, lf );
				nfree(q);
				return( rcomma( p ) );
			}

			/* both sides exist and the condition is nontrivial */
			/* ( actually, condition may be trivial, but this 
			/*   doesn't happen often enough to risk being clever)
			*/
			p->in.left = condit( p->in.left, CCC, -1, lf );
			/* if there's a value resulting, create QNODEs */
			if (goal <= NRGS) {
			    q1 = force(q1, 0);
			    q->in.right = force(q->in.right, RIGHT_QNODE);
			}
			q1 = genbr( 0, l, q1 );
			q->in.left = dlabel( q1, lf );
			q->tn.op = COMOP;
			return( dlabel( rcomma(p) , l ) );
		}

		if( goal == CEFF ) 
		{
			/* some things may disappear */
			switch( o ) 
			{

			default:
			    if ((p->in.strat & VOLATILE) == 0)
				break;
			    /* force evaluation for value to touch object */
			    /*FALLTHRU*/
			case CBRANCH:
			case GENBR:
			case GENUBR:
			case GENLAB:
			case CALL:
			case UNARY CALL:
#ifdef IN_LINE
			case INCALL:		/* handle asm calls as if */
			case UNARY INCALL:	/* they were regular calls */
#endif
			case STCALL:
			case UNARY STCALL:
			case STASG:
			case INIT:
			case MOD:   /* do these for the side effects */
			case DIV:
			case EH_CATCH_VALS:
			case EH_COMMA:
			case EH_LABEL:
			case UNINIT:
			case SINIT:
			case DEFNAM:
			case COPY:
                        case COPYASM:
                        case JUMP:
                        case GOTO:
                        case ALIGN:
                        case BMOVE:
                        case BMOVEO:
                        case RETURN:
			case LABELOP:
				goal = NRGS;
			}
		}

		/* The rhs of && and || ops are executed only if the
		   result is not clear from the lhs.  If our goal is
		   CEFF, we don't need a result, but we need to
		   preserve that dependancy. So special case it. */
		/* For CG: the lhs is always for value; the rhs is
		  whatever the goal of the LET is*/
		if (goal==CEFF)  {
			if (o == ANDAND) return andeff(p);
			if (o == OROR) return oreff(p);
			if (o == LET) return leteff(p);
		}
		/* This next batch of code wanders over the tree getting
		   rid of code which is for effect only and has no
		   effect */
		switch( optype(o) ) 
		{
		case LTYPE:
			/*If the DOEXACT bit is set, don't remove it*/
                        if( goal == CEFF && (!(p->in.strat & DOEXACT)))
			{
				nfree(p);
				return( NIL );
			}
			break;

		case BITYPE:
			p->in.right = condit( p->in.right, goal, -1, -1 );
			/*FALLTHRU*/
		case UTYPE:
			/*This duplicates goal-setting code in rewcom.
			  The lhs of COMOP or SEMI is always for effects*/
			switch(o)
			{
			case COMOP:
			case SEMI:
				p->in.left = condit( p->in.left, CEFF, -1, -1 );
				break;
			default:
				p->in.left = condit( p->in.left, goal, -1, -1 );
				break;
			}
		}

		/* If we are only interested in effects, we quit here */
		if(   goal == CEFF
		   || o == COMOP
		   || o == SEMI
		) 
		{
			if (p->in.strat & DOEXACT)
				return( p );
			/* lhs or rhs may have disappeared */
			/* op need not get done */

			switch( optype(o) )
			{

			case BITYPE:
				if (p->tn.op != SEMI)
				    p->tn.op = COMOP;
				p = rcomma(p);
				return ( p );

			case UTYPE:
				nfree(p);
				return( p->in.left );

			case LTYPE:
				nfree(p);
				return( NIL );
			}
		}
		return( p );
	}

	/* goal must = CCC from here on */

	switch (o)
	{
	case EQ: case NE:
	case LE: case NLE:
	case LT: case NLT:
	case GE: case NGE:
	case GT: case NGT:
	case LG: case NLG:
	case LGE: case NLGE:
	case ULE: case UNLE:
	case ULT: case UNLT:
	case UGE: case UNGE:
	case UGT: case UNGT:
	case ULG: case UNLG:
	case ULGE: case UNLGE:
		if (p->in.left->tn.op == ICON) 
		{
			NODE *temp = p->tn.left;
			p->tn.left = p->tn.right;
			p->tn.right = temp;
			o = p->tn.op = commrel[o - EQ];
		}
		if (t < 0) 
		{
			o = p->tn.op = negrel[o - EQ];
			t = f;
			f = -1;
		}
		if( p->in.right->in.op == ICON &&
		    p->in.right->in.name == (char *)0 &&
		    num_ucompare(&p->in.right->tn.c.ival, &num_0) == 0
		) {
			o = p->tn.op = flatrel[o - EQ];
			q1 = p->in.left;
			q2 = q1->in.left;
			if ((q1->in.op == FLD && T_UNSIGNED(q2->in.type))
				|| (q1->in.op == CONV && T_SIGNED(q1->in.type)
				&& T_UNSIGNED(q2->in.type)
				&& gtsize(q1->in.type) > gtsize(q2->in.type))
			) {
				switch (o) {
				case ULE:
				case LE:
					o = p->in.op = EQ;
					break;
				case ULT:
				case LT:
				case -2: /* always false */
					return gtb(p, f);
				case UGE:
				case GE:
				case -1: /* always true */
					return gtb(p, t);
				case UGT:
				case GT:
					o = p->in.op = NE;
					break;
				}
			}

#ifndef NOOPT
			/* the question here is whether we can assume that */
			/* unconditional branches preserve condition codes */
			/* if this turned out to be no, we would have to */
			/* explicitly handle this case here */

			switch (o)
			{
			case ULE:
				o = p->in.op = EQ;
				goto islogop;
			case UGT:
				o = p->in.op = NE;
				/*FALLTHRU*/
			case EQ:
			case NE:
			case LE:
			case LT:
			case GE:
			case GT:
			islogop:
				if (logop(q1->tn.op) || q1->in.op == QUEST)
				{
					/* situation like (a==0)==0
					** or ((i ? 0 : 1) == 0
					** ignore optimization
					*/
					goto noopt;
				}
				break;

			case -2: /* always false */
			case ULT:  /* never succeeds */
				return( gtb( p, f ) );

			case -1: /* always true */
			case UGE:
				/* always succeeds */
				return( gtb( p, t ) );
			}
			nfree(p->in.right);
			nfree(p);
			p = condit( q1, NRGS, -1, -1 );
			p = genbr( o, t, p );
			if( f<0 ) return( p );
			else return( genbr( 0, f, p ) );
noopt: ;
# endif
		}

		p->in.left = condit( p->in.left, NRGS, -1, -1 );
		p->in.right = condit( p->in.right, NRGS, -1, -1 );
		p = genbr( o, t, p );
		if( f>=0 ) p = genbr( 0, f, p );
		return( p );

	case BCMP:
			/*Don't call condit on the CM's that
			  hold these 'ternary' nodes together.*/
		p->in.left = condit(p->in.left, NRGS, -1, -1);
		p->in.right->in.left = condit(p->in.right->in.left, NRGS, -1, -1);
		p->in.right->in.right = condit(p->in.right->in.right, NRGS, -1, -1);
		if( t>=0 ) p = genbr( NE, t, p );
		if( f>=0 ) p = genbr( (t>=0)?0:EQ, f, p );
		return( p );

	case SEMI:
			/*RTOL semi for condition codes .
			  we must save a value, do the lhs, then
			  test the value*/
		if ( p->in.strat & RTOL)
		{
			p->in.right = condit(p->in.right, NRGS, -1, -1);
			p->in.left = condit(p->in.left, CEFF, -1, -1);

			/* The lhs may have vanished*/
			/* But, the rhs must still exist (it was for NRGS) */
			p = rcomma(p);

			/*Do the branches*/
			if( t>=0 ) p = genbr( NE, t, p );
			if( f>=0 ) p = genbr( (t>=0)?0:EQ, f, p );

			return p;
			
		}
		/*Normal (non-rtol) semicolons:*/
		/*FALLTHRU*/

	case COMOP:
		p->in.left = condit( p->in.left, CEFF, -1, -1 );
		p->in.right = condit( p->in.right, CCC, t, f );
		return( rcomma( p ) );

	case NOT:
		nfree(p);
		return( condit( p->in.left, CCC, f, t ) );

	case ANDAND:
		lf = f<0 ? getlab() : f;
		p->tn.op = COMOP;
		if (cond(p->in.left) > 0)
		{
			/* left is always true */
			p->in.left = condit( p->in.left, CEFF, -1, -1 );
			p->in.right = condit( p->in.right, CCC, t, f );
		}
		else  {
			/* lhs not always true */
			if (cond(p->in.right) > 0)
			{
				/* rhs is always true */
				p->in.right =
				   condit( p->in.right, CEFF, -1, -1 );
				if (p->in.right)  {
				    /* const with sideeffect */
				    p->in.left = 
					condit( p->in.left, CCC, -1,lf);
				    p->in.right = condit( p->in.right,
					CCC, t, t );
				} else
				    p->in.left =
				     condit( p->in.left, CCC, t, f );
			} else  {
				p->in.left =
				     condit( p->in.left, CCC, -1, lf );
				p->in.right =
				   condit( p->in.right, CCC, t, f );
			}
		}
		if( (q = rcomma( p )) != 0 ) 
		{
		    if( f<0 ) q = dlabel( q, lf );
		}
		return( q );

	case OROR:
		lt = t<0 ? getlab() : t;
		p->tn.op = COMOP;
		if (cond(p->in.left) == 0)
		{
			/* left is always false */
			p->in.left = condit( p->in.left, CEFF, -1, -1 );
			p->in.right = condit( p->in.right, CCC, t, f );
		}
		else  {
			/* left is not always false */
			if (cond(p->in.right) == 0)
			{
				/* right always false */
				p->in.right =
				   condit( p->in.right, CEFF, -1, -1 );
				if (p->in.right) { /* right side has side effect */
				    p->in.left = 
					condit( p->in.left, CCC, lt,-1);
				    if (f >= 0)		/* if fall-thru, no branch */
					p->in.right = genbr( 0, f, p->in.right );
				}
				else
				    p->in.left =
				        condit( p->in.left, CCC, t, f );
			} else  {
				p->in.left =
				     condit( p->in.left, CCC, lt, -1 );
				p->in.right =
				   condit( p->in.right, CCC, t, f );
			}
		}
		if( (p = rcomma( p )) != 0 ) 
		{
		    if( t<0 ) p = dlabel( p, lt );
		}
		return( p );

	case QUEST:
		lf = f<0 ? getlab() : f;
		lt = t<0 ? getlab() : t;
		p->in.left = condit( p->in.left, CCC, -1, l=getlab() );
		q = p->in.right;
		q1 = condit( q->in.left, goal, lt, lf );
		q->in.left = dlabel( q1, l );
		q->in.right = condit( q->in.right, goal, t, f );
		p->tn.op = COMOP;
		q->tn.op = COMOP;
		if( t<0 ) p = dlabel( p, lt );
		if( f<0 ) p = dlabel( p, lf );
		return( p );

	default:
		/* get the condition codes, generate the branch */
		switch( optype(o) )
		{
		case BITYPE:
			p->in.right = condit( p->in.right, NRGS, -1, -1 );
			/*FALLTHRU*/
		case UTYPE:
			p->in.left = condit( p->in.left, NRGS, -1, -1 );
		}
			/* If t == f, the genbr is unneeded. Moreover
			** we get a tree that cfix can't fix in
			** certain rare instances:
			** if(a && (func(),1)||b); where func returns
			** void. */
		if( t>=0 && t!=f ) p = genbr( NE, t, p );
		if( f>=0 ) p = genbr( (t>=0)?0:EQ, f, p );
		return( p );
	}
}

/* Try this to see if we can track the top of tree we are
** generating code for at all times.
*/

NODE *top;
static int killed;
static int branches;

/* Array ref_trees[] maintains list of trees where the 
** node was located.  Each entry holds the parent of the tree
** where the node was found, ecept where the parent would be
** a CONV, in which case we hold the parent of the CONV.
*/

NODE **ref_trees;
int ref_entries = 0;
#define REFTREE_INCR	10

static void
add_reftree(p)
NODE *p;
{
	static int size = 0;

	if (size == 0) {
		ref_trees = (NODE **)malloc(REFTREE_INCR * sizeof(NODE *));
		size = REFTREE_INCR;
	}
	else if (size == ref_entries) {
		ref_trees = (NODE **)realloc(ref_trees, 
			(size+REFTREE_INCR) * sizeof(NODE *));
		size += REFTREE_INCR;
	}
	if (ref_trees == NULL)
		cerror(gettxt(":689:","out of memory"));
	ref_trees[ref_entries] = p;
	ref_entries++;
}
		

int
is_live(p, ref)
NODE *p;
int ref;
/* Returns 1 if p will definitely not be killed before it is used again.
** A function call or an indirect assigment is assumed to kill p, unless
** p is a register.  If p is a REG node, an indirect assigment will not
** kill it.
*/
{
	int retval;
#ifndef NODBG
	/* Output debugging based on fpdebug because currently
	** only floating point code generation calls these
	** routines.
	*/
	extern int fpdebug;
	if (fpdebug) {
		printf("*** Test to see if\n");
		e2print(p);
		printf("*** is live after %d references in\n", ref);
		e2print(top);
	}
#endif
	retval = ref_entries = branches = killed = 0;
	if (optype(p->in.op) != LTYPE)
		return 0;	/* cannot handle */
	if (count_refs(p, top, 0, 1) > ref)
		retval = p->tn.strat & WASCSE || !branches;
#ifndef NODBG
	if (fpdebug) printf("It is %s live\n", retval ? "" : "not");
#endif
	return retval;
}

int 
references(p)
NODE *p;
/* Return the number of references to p in the top tree.  This
** is intended to be used to determine if a value will be re-used.
** p is assumed to be a leaf node of type NAME, TEMP, etc.  
*/
{
	ref_entries = branches = killed = 0;
	if (optype(p->in.op) != LTYPE)
		return 0;	/* cannot handle */
	return count_refs(p, top, 0, 0);
}

static int 
matches(p, t)
NODE *p, *t;
{
	if (p->tn.op != t->tn.op || p->tn.type != t->tn.type) 
		return 0;
	switch(p->tn.op) 
	{
	case NAME: 
		return (p->tn.name == t->tn.name && p->tn.c.off == t->tn.c.off);
	case CSE:
		return (p->csn.id == t->csn.id); 
	case REG:
		return (p->tn.sid == t->tn.sid);
	case VAUTO:
	case VPARAM:
	case TEMP:
		return p->tn.c.off == t->tn.c.off;
	case ICON:
		return num_ucompare(&p->tn.c.ival, &t->tn.c.ival) == 0;
	case FCON:
		return (!FP_CMPX(p->tn.c.fval, t->tn.c.fval));
	default:
		cerror(gettxt(":690","count_refs: unexpected LTYPE %o"), p->tn.op);
		/* NOTREACHED */
	}
	/* NOTREACHED */
}
	
static int 
count_refs(p, t, pt, kills)
NODE *p;	/* Node we are looking for */
NODE *t;	/* tree in which we are looking for p */
NODE *pt;	/* Parent of t, or parent of parent if parent is CONV */
int kills;	/* 1 if we want to account for possible kills */
/* Service function for references().  All the work is done here */
{
	int refs = 0;
	if (kills && killed) return 0;	/* not interested in this tree */

	switch (optype(t->tn.op))
	{
	case LTYPE:
		refs = matches(p, t);
		if (refs)
			add_reftree(pt);
		return refs;
	case UTYPE:
		switch (t->in.op)
		{
		case GENBR:
		case GENUBR:
			refs = count_refs(p, t->in.left, t, kills);
			branches = 1;
			return refs;
		case UNARY CALL:
		case UNARY STCALL:
		case UNARY INCALL:
			killed = 1;
			break;
		case CONV:
			/* do not pass this node as a parent */
			return count_refs(p, t->in.left, pt, kills);
		}
		return count_refs(p, t->in.left, t, kills);
	case BITYPE:
		if (kills) {
			switch (t->in.op)
			{
			case ASG PLUS: 
			case ASG MINUS: 
			case ASG MUL: 
			case ASG DIV: 
			case ASG MOD: 
			case ASG OR: 
			case ASG AND: 
			case ASG ER: 
			case ASG LS: 
			case ASG RS: 
			case INCR:
			case DECR:
				refs = count_refs(p, t->in.left, t, kills);
				/* FALLTHRU */
			case ASSIGN:
				refs += count_refs(p, t->in.right, t, kills);
				if (p->in.op != REG && t->in.left->in.op == STAR)
					killed = 1;
				else {
					while (t->in.left->in.op == CONV)
						t = t->in.left;
					if (matches(p, t->in.left))
						killed = 1;
				}
				return refs;
			case CALL:
			case STCALL:
				refs = count_refs(p, t->in.right, t, kills);
				killed = 1;
				return refs;
			case INCALL:
				/* Make no assumption how code uses p */
				killed = 1;
				refs = count_refs(p, t->in.right, t, kills);
				return refs;
			}
		}
		switch (t->in.strat & (LTOR|RTOL)) 
		{
		case RTOL:
			refs = count_refs(p, t->in.right, t, kills);
			return refs + count_refs(p, t->in.left, t, kills);
		default:
			/* If we can evaluate in either order, and p
			** can be killed, the user has written invalid
			** code.
			*/
			refs = count_refs(p, t->in.left, t, kills);
			return refs + count_refs(p, t->in.right, t, kills);
		}
	}
	/* NOTREACHED */
}
		

int top_is_br;

void
p2compile( p )
register NODE *p; 
{

	NODE *dolocal();

	top = p;
	top_is_br = p->in.op == CBRANCH;

#ifndef NODBG
        if( p && odebug>2 ) {
                fprintf(outfile,"Call p2compile:\n");
                e2print(p);

	}
#endif

	if( lflag ) lineid( lineno, ftitle );
	nins = 0;	/*Must do this here for costing */
			/*Do special local rewrites*/
	top = p = dolocal(p);
			/*Check for special nail nodes (standalones) */
	if ( p2nail(p) ) 
		return;
			/*Remember where temps start*/
	tmpoff = tmp_start;

	/* generate code for the tree p */

#ifdef MYREADER
	MYREADER(p);
#endif

	/* eliminate the conditionals */
# ifndef NODBG
	if( p && odebug )
		e2print(p);
# endif
	if (p->tn.op != EH_A_SAVE_REGS && p->tn.op != EH_A_RESTORE_REGS &&
	    p->tn.op != EH_B_SAVE_REGS && p->tn.op != EH_B_RESTORE_REGS)
		top = p = condit( p, CEFF, -1, -1 );
	else
		top = p;

	if( p ) 
	{
#ifdef	MYP2OPTIM
    	        NODE * MYP2OPTIM();
		top = p = MYP2OPTIM(p);	/* perform local magic on trees */
#endif
		/* expression does something */
		/* generate the code */
# ifndef NODBG
		if( odebug>2 ) {
                	fprintf(outfile, "Heading into codgen():\n");
			e2print(p);
		}
# endif
		codgen( p );
	}
# ifndef NODBG
	else if( odebug>1 ) PUTS( "null effect\n" );
# endif
	allchk();
	/* tcheck will be done by the first pass at the end of a ftn. */
	/* first pass will do it... */
#ifdef MYP2CLEANUP
	MYP2CLEANUP();
#endif
}

#ifndef NODBG
static char *cnames[] = 
{
	"CEFF",
	"NRGS",
	"CCC",
	0,
};
void
prgoal( goal ) 
register goal;
{
	/* print a nice-looking description of goal */

	register i, flag;

	flag = 0;
	for( i=0; cnames[i]; ++i )
	{
		if( goal & (1<<i) )
		{
			if( flag ) PUTCHAR( '|' );
			++flag;
			PUTS( cnames[i] );
		}
	}
	if( !flag ) fprintf(outfile, "?%o", goal );

}

void
e2print( p )
register NODE *p; 
{
	cgprint(p,0);
	return;
}
void
e22print( p ,s)
register NODE *p; 
char *s;
{
	static down=0;
	register ty;

	if (!p) return;
	ty = optype( p->tn.op );
	if( ty == BITYPE )
	{
		++down;
		e22print( p->in.right ,"R");
		--down;
	}
	e222print( down, p, s );

	if( ty != LTYPE )
	{
		++down;
		e22print( p->in.left, "L" );
		--down;
	}
}
void
e222print( down, p, s )
NODE *p; 
char *s;
{
	/* print one node */
	int d;

	for( d=down; d>1; d -= 2 ) PUTCHAR( '\t' );
	if( d ) PUTS( "    " );

	fprintf(outfile, "%s.%d) op= '%s'",s, (int)node_no(p), opst[p->in.op] );
	prstrat(p->in.strat);	/*print the strategy field*/
	switch( p->in.op ) 
	{
		 /* special cases */
	case REG:
		fprintf(outfile, " %s", rnames[p->tn.sid] );
		break;

	case ICON:
	case NAME:
	case VAUTO:
	case VPARAM:
	case TEMP:
		PUTCHAR(' ');
		adrput( p );
		break;

	case STCALL:
	case UNARY STCALL:
		fprintf(outfile, " args=%d", p->stn.argsize );
	case STARG:
	case STASG:
		fprintf(outfile, " size=%d", p->stn.stsize );
		fprintf(outfile, " align=%d", p->stn.stalign );
		break;

	case GENBR:
		fprintf(outfile, " %d (%s)", p->bn.c.label, opst[p->bn.lop] );
		break;

	case CALL:
	case UNARY CALL:
#ifdef IN_LINE
	case INCALL:
	case UNARY INCALL:
#endif
		fprintf(outfile, " args=%d", p->stn.argsize );
		break;

	case CBRANCH:
	case GENUBR:
	case GENLAB:
	case GOTO:
	case LABELOP:
	case JUMP:
		fprintf(outfile, " %d", p->bn.c.label );
		break;

	case FUNARG:
	case DUMMYARG:
	case REGARG:
		fprintf(outfile, " offset=%d", p->tn.c.off);
		break;

	case FCON:
#ifdef  FP_XTOA
		fprintf(outfile,"%s", FP_XTOA(p->fpn.c.fval));
#else
		fprintf(outfile,"%Lg", p->fpn.c.fval);
#endif
		break;

        case FLD:
                fprintf(outfile, " c.off = 0%o", p->tn.c.off);
                break;

			/*Print out CG nodes:*/
        case COPYASM:
        case COPY:

                if ( p->in.name && p->in.name[0] )
                        fprintf(outfile, " '%s'", p->in.name);
                else
                        PUTS( " (null)" );
                break;

        case BEGF:
                {
                int i;
                        for (i=0; i<TOTREGS; ++i)
                        {
                                if (p->in.name && ((char *)(p->in.name))[i])
                                        fprintf(outfile, "%d,",i);
				else
					PUTS(" (null)");
                        }
                }
                break;

        case ENDF:
                fprintf(outfile, " autos=%ld, regs=", p->tn.c.off);
                break;

        case LOCCTR:
                fprintf(outfile, " locctr=%d",p->tn.c.label);
                break;

	case SWEND:
		fprintf(outfile, " nswcase=%lu, default %d",
			p->in.c.size, p->in.sid);
		break;

	case SWCASE:
		fprintf(outfile, " %s, label %d", num_tohex(&p->tn.c.ival), p->tn.sid);
		break;

	case DEFNAM:
		fprintf(outfile, "%s, flags=0%o, size=%lu", p->tn.name, p->tn.sid, p->tn.c.size);
		break;

	case INIT:
	case UNINIT:
		fprintf(outfile, "%lu bytes", p->tn.c.size);
		break;

	case SINIT:
		fprintf(outfile, " '%s' (length %lu)",p->tn.name,p->tn.c.size);
		break;

	case LET:
	case CSE:
		fprintf(outfile, " id=%d", p->csn.id);
		break;

	}

	PUTS( ", " );
	if (p->in.strat & OCOPY)
	    PUTS( "<Ocopy>,");
	t2print( p->in.type );
	switch( p->tn.goal ) {
	case CEFF:	PUTS( " (EFF)" ); break;
	case CCC:	PUTS( " (CC)" ); break;
	case 0:		PUTS( " (NULL)" ); break;
	default:	if (p->tn.goal != NRGS)
			    fprintf(outfile, "(BAD GOAL: %d)\n", p->tn.goal );
			break;
	}
	PUTCHAR( '\n' );
}

t2print( t )
TWORD t;
{
	int i;
	static struct {
		TWORD mask;
		char * string;
		} t2tab[] = {
			TANY, "ANY",
			TINT, "INT",
			TUNSIGNED, "UNSIGNED",
			TCHAR, "CHAR",
			TUCHAR, "UCHAR",
			TSHORT, "SHORT",
			TUSHORT, "USHORT",
			TLONG, "LONG",
			TULONG, "ULONG",
			TLLONG, "LLONG",
			TULLONG, "ULLONG",
			TFLOAT, "FLOAT",
			TDOUBLE, "DOUBLE",
			TLDOUBLE, "LDOUBLE",
			TPOINT, "POINTER",
			TPOINT2, "POINTER2",
			TSTRUCT, "STRUCT",
			TFPTR, "TFPTR",
			TVOID, "VOID",
			0, 0
			};

	for( i=0; t && t2tab[i].mask; ++i ) {
		if( (t&t2tab[i].mask) == t2tab[i].mask ) {
			fprintf(outfile, " %s", t2tab[i].string );
			t ^= t2tab[i].mask;
			}
		}
	}
# endif
