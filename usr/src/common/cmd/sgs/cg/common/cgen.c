#ident	"@(#)cg:common/cgen.c	1.18.2.30"

# include "mfile2.h"
# include <malloc.h>
# include <memory.h>
# include <unistd.h>

static void rewlhs(), cfix(); 
static NODE *ind2type();

# define istnode(p) (p->in.op==REG && istreg(p->tn.sid))

/*
** For God's workes are, like him, all infinite
** And curious search but craftie sin's delight.
*/
void  
rewcom( p, goal )
NODE *p; 

{
	/* find all , ops, move as high as is legal */
	/* rewrite p in place; this takes some doing! */
	/* while we are at it, take care of setting the goal field */
	int o, ty, g1, g2;
	NODE *l, *r, *ql, *qr;
	NODE *last;
	int paren_flag;
	/* int changed = 0;	flag: 1 = we did a rewrite*/

	o = p->tn.op;
	g1 = g2 = NRGS;
	p->tn.goal = goal;

	/* special cases for subtrees:
	** GENBR has left as Condition codes
	** COMOP has left as Effects
	** COLON, GENUBR, CM, GENLAB have descendents = node
	** CALL, STCALL has right as Effects
	** All others use registers
	*/

	switch( o )
	{

	case FREE:
#ifndef NODBG
		cerror( "rewcom(%d) is FREE", node_no(p) );
#else
		cerror(gettxt(":594", "rewcom is FREE"));
#endif
		/*FALLTHRU*/
	case GENBR:
		g1 = CCC;
		break;
	case LET:
		g2 = goal;		/* right-side goal */
		break;
	case SEMI:
			/*Special case: a rightfirst semi,
			  done for effects, needs to get a value,
			  do the left side, then test the value*/	
			/*Cfix will do the test of the value*/
		if ( firstl(p) == p->in.right && goal == CCC)
		{
			p->tn.goal = goal = NRGS;
		}
		/*FALLTHRU*/
	case COMOP:
		g1 = CEFF;
		g2 = goal;
		break;

	case COLON:
	case GENUBR:
	case CM:
	case GENLAB:
		g1 = g2 = goal;
		break;

	case CALL:
#ifdef IN_LINE
	case INCALL:		/* handle asm calls like regular calls */
#endif
	case STCALL:
		g2 = CEFF;
		break;
	}


	switch( ty = optype(o) )
	{

	case BITYPE:
		rewcom( r = p->in.right, g2 );
		/*FALLTHRU*/
	case UTYPE:
		rewcom( l = p->in.left, g1 );
		break;
	case LTYPE:
		return;
	}

	if( o==COMOP || o==COLON || o==GENLAB ) return;

	/*If a comma is trying to go past the last side of an ordered node,
	  rewrite it into a semi*/
	if ( (last = lastl(p)) != NULL && last->in.op == COMOP)
	{
		uncomma(last);
	}
	/*If a comma is trying to go past a parened node, it gets just past,
	  but use a LTOR, Parened semi instead of a comma. This prevents it
	  from going any further.*/

	paren_flag = (p->in.strat & PAREN);
	
	/* look for (A,B) op C and A op (B,C), and rewrite */
	/* A,B if A headed by GENBR can't be rewritten */
	/* the assumption is that B is executed immediately after A, */
	/* and this won't necessarily be true if op is commutative */

	if( l->tn.op == COMOP && l->in.left->tn.op != GENBR ) 
	{
#ifndef NODBG
		if( odebug>1 ) {
			printf("before COMOP rewrite\n");
			e2print( p );
		}
#endif
		/* rewrite it... */
		/* (A,B) op C => A,(B op C) */
		/* also, for unary ops,  op (A,B) => A , (op B)  */
		ql = l->in.left;
		qr = l->in.right;
		
		/* Do not slip past an intermediate conversion */
		if (qr->tn.type != l->tn.type && p->tn.type != l->tn.type)
			qr->tn.type = l->tn.type;

		*l = *p;  /* copies op, and other stuff if op is unary */
		l->in.left = qr;
		p->in.right = l;
		p->in.left = ql;
		p->tn.op = COMOP;
#ifndef NODBG
		if( odebug>1 ) {
			printf("after COMOP rewrite\n");
			e2print( p );
		}
#endif
		if ( paren_flag)
			uncomma(p);
		rewcom( p, p->tn.goal );
	}
			/*The above rewrite might have put
			  a COMOP in p; must check again*/
	/* Can't rewrite right-side past the current node if the current
	** node is a COMOP, COLON, or GENLAB, or if there's no right node.
	*/
	if (   ty == UTYPE
	    || (o = p->in.op) == COMOP || o == COLON || o == GENLAB
	   ) return;

	/* Cheat a bit:  don't move COMOP on right side (ARG side)
	** of CALL OPs; this permits side effects within ARGs to
	** be done around the time the ARG is evaluated, since the
	** ARG is always done for effect.
	*/
	if( r->tn.op == COMOP && r->in.right->tn.op != GENBR && 
		(!callop(o) || o == INCALL))
	{
		/* rewrite, again */
		/* A op (B,C) => B,(A op C) */
		/* op is not unary now */
#ifndef NODBG
		if( odebug>1 ) {
			printf("before COMOP rewrite\n");
			e2print( p );
		}
#endif
		ql = r->in.left;
		qr = r->in.right;
		/* Do not slip past an intermediate conversion */
		if (qr->tn.type != r->tn.type && p->tn.type != r->tn.type)
			qr->tn.type = r->tn.type;
		*r = *p;
		p->tn.op = COMOP;
		if ( paren_flag)
			uncomma(p);
		p->in.left = ql;
		r->in.right = qr;
#ifndef NODBG
		if( odebug>1 ) {
			printf("after COMOP rewrite\n");
			e2print( p );
		}
#endif
		rewcom( p, p->tn.goal );
	}
}

static void 
rewlhs(p)
NODE *p; 

{
	/* rewrite x op= y as (x op= y),x */
	/* it would be really nice to optimize after doing this . . . */
	NODE *q, *t;
	q = talloc();
	*q = *p;
	t = tcopy( p->in.left );
	p->in.left = q;
	p->in.right = t;
	p->tn.op = COMOP;
	p->in.strat = 0;
	return;
}
int
rewsto(p)
NODE *p; 
{
	/* a temp, t, is generated, and p is rewritten as ((t=p),t) */
	/* if p has the form x op= A, and x is of the right form, rewrite
	/* as ((x op= A), x) */

	int o, ao;
	NODE *t, *q;
	/* probably not perfect for structs: CHECK UP.. */

	while( (o=p->tn.op) == COMOP ) p = p->in.right;
	if( o == TEMP ) return(0);  /* nothing to do */
			/*If this is a list of ;lp's, with a temp at the
			  bottom, don't do an assignment, just use the
			  generated temp.*/
	if (semilp(p) && rewsemi(p))
		return 1;

        /* rewriting a parened node removes the parens restriction*/
        p->in.strat &= ~PAREN;
	if( o == STARG ) 
	{
		 /* store a structure argument */

		t = talloc();
		*t = *p->in.left;  /* copy contents, mainly for type, etc. */
		t->in.strat = 0;	/*clear p's strategy bits */
		q = talloc();
		*q = *t;
		t->tn.op = TEMP;
		t->tn.c.off = BITOOR(freetemp(argsize(p)/SZINT));
		t->tn.name = (char *) 0;
		t->tn.type = TSTRUCT;
		q->tn.op = UNARY AND;
		q->in.left = t;
		/* now, q has & TEMP */
		t = talloc();
		*t = *p;
		t->in.left = q;
		t->in.right = p->in.left;
		t->tn.op = STASG;
		/* now, t has (&TEMP) = struct */
		p->in.left = talloc();
		p->in.left->tn.op = COMOP;
		p->in.left->in.left = t;
		p->in.left->in.right = t = talloc(); /* copy q here */
		*t = *q;
		t->in.left = talloc();
		*t->in.left = *q->in.left;
		/* finally, have (&TEMP = struct),(&TEMP) */
		/* this should do it: whew! */
#ifndef NODBG
		if( odebug>1 ) e2print( p );
#endif
		return( 1 );
	}

#ifndef NODBG
	if( odebug>1 ) 
	{
		e2print( p );
		printf( "\nrewritten by rewsto as:\n" );
	}
#endif
	if( asgop(o) && o!=INCR && o!=DECR && lhsok( p->in.left ) ) {
		/* x op= y turns into (x op= y), x */
		/*Exception: if OCOPY is set, must actually copy the subtree*/
		if ((!(p->in.strat & OCOPY)) || p->in.left->in.op == TEMP
		|| istnode(p->in.left) )
		{
		rewlhs( p );
#ifndef NODBG
		if( odebug>1 ) e2print( p );
#endif
		return( 1 );
		}
	}
	ao = ASG o;
	if( asgbinop(ao) ) 
	{
		/* If the left side is a TEMP that was not 
		** a CSE, we can redefine it.  If it was a CSE,
		** it will be re-used.
		*/
		if( p->in.left->tn.op == TEMP && 
		  !(p->in.left->tn.strat & WASCSE)) 
		{
			p->tn.op = ao;
			unorder(p);
			rewlhs( p );
#ifndef NODBG
			if( odebug>1 ) e2print( p );
#endif
			return( 1 );
		}
	}
	/* to rewrite in place, p becomes a COMOP; rhs is the temp, lsh
	** is t = p, where p has been converted to the intermediate type 
	*/
	/* after some debate, the type of the temp will be the type of p */

	t = talloc();
	*t = *p;  /* copy contents, mainly for type, etc. */
	q = talloc();
	*q = *p;
	t->tn.op = TEMP;
	t->tn.c.off = BITOOR(freetemp(argsize(p)/SZINT));
	t->tn.name = (char *) 0;
	q->tn.op = ASSIGN;
	q->in.left = t;
	q->in.right = talloc();
	q->in.strat = t->in.strat = 0;
	*(q->in.right) = *p;
		/* now, q has (t=p) */
	p->in.right = talloc();
	*(p->in.right) = *t;
	p->tn.op = COMOP;
	p->in.left = q;
	/* this should do it: whew! */
#ifndef NODBG
	if( odebug>1 ) e2print( p );
#endif
	return( 1 );
}

iseff( p )
NODE *p; 

{
	/* return 1 if p has some side effects, 0 otherwise */
	/* If DOEXACT is set, assume that anything might have side effects*/
	int o;
	o = p->tn.op;
	if( (p->in.strat & DOEXACT) || callop(o) || asgop(o))
		return( 1 );
	switch(o)
	{ 
	case COPY:
	case COPYASM:
	case BMOVEO:
	case BMOVE:
	case RETURN:
	case JUMP:
	case GOTO:
	case EH_LABEL:
			return( 1 );
	}
	switch( optype( o ) )
	{
	case BITYPE:
		if( iseff( p->in.right ) ) return( 1 );
		/*FALLTHRU*/
	case UTYPE:
		return( iseff( p->in.left ) );
	}
	return( 0 );
}

NODE *
lhsto( p )
NODE *p; 

{
	/* find a piece of the LHS to be stored */
	/* if found, rewrite tree */
	NODE *q;
	int o;

	for( q = p->in.left; (o=q->tn.op)!=STAR; q=q->in.left )
	{
		if( optype(o) == LTYPE ) return( (NODE *)0);
	}
	/* q is now the * node, if there one */
	q = q->in.left;
	o = q->tn.op;
			/*Must still rewrite if this is the first side*/
        if( optype(o) == LTYPE && !(p->in.strat & LTOR) ) return( (NODE *)0 );
	else return( q );
}

static int
c2bigger( p ) NODE *p; {
	/* p is a conversion op; does it make things bigger */
	register TWORD t, tl;

	t = p->tn.type;
	tl = p->in.left->tn.type;

	if( (t|tl)&TPOINT ) return( 0 );  /* pointers are funny */
	if( (t|tl)&TFPTR ) return( 0 );	/* so are frame pointers */
	if( t&TLDOUBLE ) return( 1 );
	if( tl&TLDOUBLE ) return( 0 );
	if( t&TDOUBLE ) return( 1 );
	if( tl&TDOUBLE ) return( 0 );
	if( t&TFLOAT ) return( 1 );
	if( tl&TFLOAT ) return( 0 );
	if( t&(TLLONG|TULLONG) ) return( 1 );
	if( tl&(TLLONG|TULLONG) ) return( 0 );
	if( t&(TLONG|TULONG) ) return( 1 );
	if( tl&(TLONG|TULONG) ) return( 0 );
	if( t&(TINT|TUNSIGNED) ) return( 1 );
	if( tl&(TINT|TUNSIGNED) ) return( 0 );
	if( t &(TSHORT|TUSHORT) ) return( 1 );
	return( 0 );
	}

static NODE *
ind2type( p )
register NODE *p; 

{
	/* make the type of p be the appropriate type for an argument */
	register TWORD t;
	NODE *q;

	t = p->tn.type;
	if( t == TCHAR || t == TSHORT ) t = TINT;
	else if( t == TUCHAR || t == TUSHORT ) t = TUNSIGNED;
	else return( p );

	if( p->tn.op == CONV && c2bigger(p) ) 
	{
		p->tn.type = t;
		return( p );
	}
	q = talloc();
	q->tn.op = CONV;
	q->in.left = p;
	q->in.right = 0;
	q->tn.name = (char *) 0;
	q->tn.type = t;
	q->tn.goal = NRGS;
	return( q );
}

void
reweop( p )
register NODE *p; 

{
	/* Rewrite A OP= B as A = A OP B.
	** Also, rewrite (CONV A) OP= B as A = (CONV ( (CONV A) OP B ) )
	** Rewritten in place.
	** On input, the type of the OP= equals the type of A.
	** The type of the OP node on output is the type of B,
	** the type of the = node on output is the type of A.
	** EXCEPT:
	**	>>= and <<= always have a right side of INT.
	**	Therefore, the type of OP (<< or >>) is the type
	**	of the left!
	*/

	register NODE *q, *t;
	register TWORD ty;

#ifndef NODBG
	if( odebug>1 ) 
	{
		e2print( p );
		printf( "\nrewritten by reweop as:\n" );
	}
#endif
	/* rewrite tree with duplicate left subtree in new right subtree */
	/* there is an implicit q->in.right = p->in.right in  *q = *p */
	q = talloc();
	*q = *p;	
	q->in.left = tcopy( p->in.left );
	if( p->in.left->tn.op == CONV ) 
	{
		/* ( CONV (A) ) op= B becomes A = CONV ( CONV(A) op B ) */
		/* the op is done to the type of B */
		/* the assignment is done to the type of A */
		t = p->in.left;
		ty = t->in.left->tn.type;
		p->in.left = t->in.left;
		p->in.right = t;
		t->in.left = q;
		/* now, have the tree built; fix the types */
		t->tn.type = ty;
	}
	else 
	{
		p->in.right = q;
	}
	/* NOTE: no =ops for structures... */
	p->tn.op = ASSIGN;
	q->tn.op = NOASG q->tn.op;
	p->tn.type = p->in.left->tn.type;
	if (q->tn.op != LS && q->tn.op != RS)
	    q->tn.type = q->in.right->tn.type;
#ifndef NODBG
	if( odebug>1 ) e2print( p );
#endif
}

static int
rewasgop(NODE *p)
{
	NODE *l, *r, *q, *t;
	TWORD ptype;

#ifndef NODBG
	if (odebug > 1) {
		printf("rewasgop called with:\n");
		e2print(p);
	}
#endif
	l = p->in.left;
	if (l->in.op == TEMP && !(l->in.strat & WASCSE))
		return 0; /* prevents looping with reweop()! */
	r = p->in.right;
	if (l->in.type != r->in.type) {
		if (p->in.op != LS && p->in.op != RS)
			return 0; /* ?maybe later bring to common type? */
	}
	/*
	* Turn (a OP b) into (T = a, T OP= b) when p has same type,
	* or into ((T = a, T OP= b), (CONV T)) otherwise.  Both must
	* leave "p" as the top of the subtree.
	*/
	ptype = p->in.type;
	p->in.type = l->in.type;
	r = talloc();
	*r = *p;
	p->in.op = COMOP;
	p->in.right = r;
	r->in.op = ASG r->in.op;
	t = talloc();
	t->in.type = l->in.type;
	r->in.left = t;
	t->tn.op = TEMP;
	t->in.c.off = BITOOR(freetemp(argsize(l) / SZINT));
	t->in.name = (char *)0;
	p->in.left = q = talloc();
	*q = *p;
	q->in.op = ASSIGN;
	q->in.right = l;
	q->in.left = tcopy(t);
	if (ptype != p->in.type) {
		l = talloc();
		*l = *p;
		p->in.type = ptype;
		p->in.left = l;
		p->in.right = q = talloc();
		q->in.op = CONV;
		q->in.type = ptype;
		q->in.left = tcopy(t);
	}
#ifndef NODBG
	if (odebug > 1) {
		printf("rewritten into:\n");
		e2print(p);
	}
#endif
	return 1;
}

rewass( p )
NODE *p; 

{
	NODE *q;
	NODE * rewrite;
	int o;
	/* look for =ops to be rewritten */

#ifndef NODBG
	if( odebug ) 
	{
		printf( "rewass called with:\n" );
		e2print( p );
	}
#endif
	o = p->tn.op;
	if( o == UNARY AND ) 
	{
		if( p->in.left->tn.op == RNODE ) 
		{
			/* this should happen only with structure returns */
			q = p->in.left;
			q->tn.op = ICON;
			*p = *q;
			nfree(q);
			return(0);  /* keep going in costs */
		}
		/* this case should happen only with short structures */
		(void) rewsto( p->in.left );
		/* & f() has turned into & ( t=f(),t) */
#ifndef NODBG
		if( odebug ) 
		{
			printf( "\nrewritten by rewass as:\n" );
			e2print( p );
		}
#endif
		return(1);
	}
	if( !asgop(o) || o==ASSIGN ) 
	{
		if( o==ASSIGN ) 
		{
			/* look for funny nodes on lhs */
			o = p->in.left->tn.op;
			if( o==RNODE || o==QNODE || o==SNODE ) 
			{
				/* force into r0 */
				p->in.left->tn.op = REG;
				p->in.left->tn.sid = callreg( p->in.right );
#ifndef NODBG
				if( odebug ) 
				{
					printf( "funny node redone\n" );
					e2print(p);
				}
#endif
				return(0);
			}
		}
		else if (optype(o) != LTYPE)
		{
			int changed;
			TWORD olt = p->in.left->tn.type;
			/* this case is, for example, 
				unsigned char a, b;
				...   a*b
			/* we convert both to a reasonable type */
			/* the result is assumed to be automatically
			/* converted downwards if it should be... */

			p->in.left = ind2type( p->in.left );
			changed = p->in.left->in.type != olt;
			if (optype(o) == BITYPE) {
			    TWORD ort = p->in.right->in.type;
			    p->in.right = ind2type( p->in.right );
			    changed |= (p->in.right->in.type != ort);
			}
#ifndef NODBG
			if( odebug ) {
				printf( "conversions inserted" );
				e2print(p);
			}
#endif
			/* if this didn't work, we are in trouble */
			if( changed ) {
				/* we have changed something */
				return( 0 );
			}
			/* Try turning (a OP b) into (T = a, T OP= b) */
			if (optype(o) == BITYPE && asgop(ASG o) && rewasgop(p))
				return REWROTE; /* pretty major */
		}
			/*Cannot match this node at all.
			  CG has some last ditch stuff it can do:*/

		if ( (rewrite = firstl(p)) != NULL)
		{
			/*otherwise, if this is an ordered node, rewrite
			  the first child to temp*/
			unorder(p);
#ifndef NODBG
			if ( odebug)
			{
				fprintf(outfile, "Rewriting first side:\n");
				e2print(p);
			}
#endif
			if (rewsto(rewrite))
				return REWROTE;	/*major rewrite*/
		}
			/*Give up!*/
#ifndef NODBG
		e2print(p);
#endif
		cerror(gettxt(":595", "can't deal with op %s"), opst[o] );
	}
	if( o == INCR || o == DECR ) 
	{
		/* very crude: a++ becomes (a+=1)-1 */
#ifndef NODBG
		if( odebug>1 ) 
		{
			e2print( p );
			printf( "\nrewritten by rewass as:\n" );
		}
#endif
		if( p->in.goal == CEFF )
		{
			p->in.op = ((o==INCR)?ASG PLUS:ASG MINUS);
		}
		else
		{
			/* rewrite tree with duplicate left subree in new
			/* right subtree...
			/* there is an implict q->in.left = p->in.left in *q = * p
			*/
			q = talloc();
			*q = *p;	
			q->in.right = tcopy( p->in.right );
			p->in.left = q;
			q->tn.op = ((o==INCR)?ASG PLUS:ASG MINUS);
			p->tn.op = ((o==INCR)?MINUS:PLUS);
			if (q->in.left->in.op == FLD)
			{
			    int size;
                            int fldsz = UPKFSZ(q->in.left->tn.c.off);
			    int t = q->in.left->in.type;

			    if (t & (TINT | TUNSIGNED))
				size = SZINT;
			    else if (t & (TSHORT | TUSHORT))
				size = SZSHORT;
			    else if (t & (TCHAR | TUCHAR))
				size = SZCHAR;
			    else if (t & (TLONG | TULONG))
				size = SZLONG;
			    else if (t & (TLLONG | TULLONG))
				size = SZLLONG;
			    else
				cerror(gettxt(":596","invalid type for bitfield"));

			    if (fldsz != size)
			    {

				NODE* r;

				r = talloc();
				*r = *p;
				r->in.left = p->in.left;
				r->in.right = p->in.right;
				p->in.left = r;
				p->in.op = AND;

				r = talloc();
			    	r->in.op = ICON;
			    	r->tn.type = t;
				(void)num_fromulong(&r->tn.c.ival,
					~(~0ul << fldsz)); /* mask fldsz */
				r->tn.sid = 0;
			    	p->in.right = r;
			    }
			}
		}
#ifndef NODBG
		if( odebug ) 
		{
			printf( "\nrewritten by rewass as:\n" );
			e2print( p );
		}
#endif
		return(1);
	}
	/* find out if some subtree has to be stored into a temp... */
	if( (q = lhsto(p)) != 0 )  
	{
		if( !rewsto( q ) ) cerror(gettxt(":597", "rewass0" ));  /* q => t=q,t */
		rewcom( p, p->tn.goal );  /* move COMOP to the top */
		if( p->tn.op != COMOP ) cerror(gettxt(":598", "rewass1" ));
		if( !asgop( p->in.right->tn.op ) ) cerror(gettxt(":599", "rewass2" ));
		reweop( p->in.right );
	}
	else 
		reweop( p );  /* rewrite p as an =OP */
	return(1);
}

/* subcall returns TRUE (1) if the node pointed to by "p" (presumably
** a FUNARG node) contains a nested call.  If no nested call is found,
** it returns FALSE (0).  For non-fixed frame we are only concerned
** with asm's.
*/
static int
subcall( p )
register NODE *p; 

{
	int o = p->tn.op;
	int t = optype(o);

#ifdef FIXED_FRAME
	if (fixed_frame() && (o == CALL || o == UNARY CALL 
			|| o == STCALL || o == UNARY STCALL)
		|| o == INCALL || o == UNARY INCALL )  
#else
	if (o == INCALL || o == UNARY INCALL )  
#endif
		return 1 /* TRUE */;
	switch (t)
	{
	case BITYPE:
		if (subcall(p->in.right)) 
			return( 1 /* TRUE */ );
		/* FALLTHRU */
	case UTYPE:
		return( subcall( p->in.left ) );
	}
	return( 0 /* FALSE */ );
}

static int
contains_call(p)
NODE *p;
{
	int o = p->tn.op;

	if (o == INCALL || o == CALL || o == STCALL )
		return 1;

	switch (optype(o))
	{
	case BITYPE:
		return (contains_call(p->in.left) || contains_call(p->in.right));
	case UTYPE:
		return (contains_call(p->in.left));
	default:
		return( 0 );
	}
}

	/* p is a CALL with left child containing a CALL:
	** Do the following rewrite.
	**
	**            CALL                 ,
	**            / \                 / \
	**           /   \    ===>       /   \
	**          L     R             /     \
	**                             =      CALL
	**                            / \     / \
	**                           T   L   T   R
	*/

static void
rewrite_call(p)
NODE *p;
{
	NODE *t = talloc(), *q = talloc();

#ifndef NODBG
	if( odebug ) 
	{
		printf( "Before rewrite_call:\n" );
		e2print( p );
	}
#endif
        *t = *p;  /* copy contents, mainly for type, etc. */
	p->in.right = t;
	p->tn.op = COMOP;
	p->in.left = talloc();
	*(p->in.left) = *(t->in.left);
	p->in.left->tn.op = ASSIGN;
	p->in.left->in.left = talloc();
        *(p->in.left->in.left) = *(t->in.left); /* For type, etc... */
        p->in.left->in.left->tn.op = TEMP;
        p->in.left->in.left->tn.c.off = BITOOR(freetemp(SZPOINT/SZINT));
        p->in.left->in.left->tn.name = (char *)0;
        p->in.left->in.left->in.strat = 0;
        p->in.left->in.right = t->in.left;
	t->in.left = talloc();
	*(t->in.left) = *(p->in.left->in.left);
#ifndef NODBG
	if( odebug ) 
	{
		printf( "After rewrite_call:\n" );
		e2print( p );
	}
#endif
}

#ifdef FIXED_FRAME

static NODE *lastcomma = 0;

static NODE *
pushlist(NODE *p, NODE *list)
{
	NODE *t;
	if(p->tn.op != REGARG)
		return list;
	t = tcopy(p);
	p->tn.op = DUMMYARG;
	tfree(p->in.left);


	if(!list) { /* first item */
		list = tcopy(t); /* To get type, etc., right */
		list->tn.op = CM;
		list->in.strat = 0;
		list->in.right = t;
		lastcomma = list;
	}
	else {
		lastcomma = lastcomma->in.left = tcopy(t);
		lastcomma->tn.op = CM;
		lastcomma->in.strat = 0;
		lastcomma->in.right = t;
	}

	return list;
}

	/*
	** Transform REGARG nodes to DUMMYARG nodes and move them to
	** the end of the arglist.
	*/	
static void
paintargs(p)
NODE *p;
{
	int o = p->tn.op;
	int t = optype(o);
	NODE *list;

	if( t == BITYPE ) {
		paintargs( p->in.right );
		if(o == CALL || o == STCALL || o == INCALL) {
			/* Walk down the arglist */
			NODE *callnode = p;
			int moveargs = 0;

			paintargs( p->in.left );

			p = p->in.right;
			while(p->tn.op == CM) {
				if(p->in.right->tn.op == REGARG)
					moveargs = 1;
				p = p->in.left;
			}
			if(p->tn.op  == REGARG)
				moveargs = 1;

			if(fixed_frame())
				moveargs = 1;
			
			list = 0;
			p = callnode->in.right;
			while(p->tn.op == CM) {
				NODE *r = p->in.right;
				if(moveargs)
					r->in.strat |= MOVEARGS;
				list = pushlist(p->in.right, list);
				p = p->in.left;
			}
			if(moveargs)
				p->in.strat |= MOVEARGS;
			list = pushlist(p, list);
			if(list) {
				lastcomma->in.left = callnode->in.right;
				callnode->in.right = list;
			}
			return;
		}
	}
	if( t != LTYPE ) paintargs( p->in.left );
}
#endif

/* Transform code from f(g(x)) -> temp = g(x), f(temp)
** for all arguments to INCALLs that have nested INCALLs.
** Trees must be unnested from the bottom-up.
** More precisely, a CALL node may not have a descendant
** which is a CALL node.
*/
nonest( p )
NODE *p; 

{
	int o = p->tn.op;
	int t = optype(o);

	if( t == BITYPE ) nonest( p->in.right );
	if( t != LTYPE ) nonest( p->in.left );

#ifdef FIXED_FRAME
	if( o==INCALL || fixed_frame() && (o==CALL || o==STCALL) ) {
#else
	if( o==INCALL ) {
#endif
		NODE *nested;

		if(contains_call(p->in.left)) {
			rewrite_call(p);
			p = p->in.right;
		}

		rewrite_args_for_nested_calls(&p, p->in.right);
	}
}

/* For each argument of the call node pointed to through "p", check for 
** nested calls.  If found, assign the argument to a temp as the left
** operand of a COMOP and the original call as the right operand.
**
** Arg will be either some FUNARGOP() or a CM.
*/
rewrite_args_for_nested_calls(NODE **call,
                              NODE *arg)
{
	register NODE *q;

	while (arg->tn.op == CM) {
		rewrite_args_for_nested_calls(call, arg->in.right);
		arg = arg->in.left;
	}  /* while */
	if (subcall( arg )) {
		store_arg((q = *call), arg->in.left);

		/* The call node has been changed to a COMOP node
		** with the call on the right.  The arg is replaced
		** by a temp and the (sub)call evaluating the temp
		** is the left node of the COMOP.
		*/
		*call = q->in.right;	/* point back to the call */
	}  /* if */
}


/* Store arg to a temp.  Change p to a comma, and change arg to be the
** temp.  The temp will be store before p.  The change looks like this:
**
**	 p(arg) => temp = arg, p(temp)
*/

store_arg(p,arg)
NODE *p, *arg;
{
	NODE *t = talloc(), *q = talloc(), *oldp = talloc();

#ifndef NODBG
	if( odebug ) 
	{
		printf( "Before store_arg:\n" );
		e2print( p );
	}
#endif
        *t = *arg;  /* copy contents, mainly for type, etc. */
        *q = *arg;
	*oldp = *p;

        t->tn.op = TEMP;
        t->tn.c.off = BITOOR(freetemp(argsize(arg)/SZINT));
        t->tn.name = (char *) 0;
	t->in.strat = 0;

        q->tn.op = ASSIGN;
        q->in.left = t;
	q->in.right = talloc();
	*(q->in.right) = *arg;
        q->in.strat = 0;

        p->in.right = oldp;
        p->tn.op = COMOP;
        p->in.left = q;

	*arg = *t;
	
#ifndef NODBG
	if( odebug ) 
	{
		printf( "After store_arg:\n" );
		e2print( p );
	}
#endif
}


#ifndef NODBG
outshp( pp )
SHAPE **pp; 

{
	SHAPE *p;

	if (pp == 0)
		return;

	for( ; p = *pp; ++pp )
	{
		printf("\t\t");
		shpr(p);
		putchar('\n');
	}
}

tabpr()
{
	register	OPTAB	*p;
	for (p = (OPTAB *) table; ;p++)
	{
		printf("Dump of table[%d] (stinline %d)\n", p-table, p->stinline );
		printf("\top = %s\n", opst[p->op]);
		printf("\tnextop = %d\n", p->nextop?p->nextop-table:-1 );
		printf("\tlshape = %d\n", p->lshape-pshape);
		printf("\tltype = 0%o\n", p->ltype);
		printf("\trshape = %d\n", p->rshape-pshape);
		printf("\trtype = 0%o\n", p->rtype);
		printf("\tneeds = %d\n", p->needs);
		printf("\trewrite = %d\n", p->rewrite);
		printf("\tcstring = %s", p->cstring);
		printf("\tLeft:\n");
		outshp(p->lshape);
		printf("\tRight:\n");
		outshp(p->rshape);
		printf("\n");
	}
}
#endif

void
codgen( p )
NODE *p; 

{

	/* generate the code for p; */
	int i, flag;

#ifndef NODBG
	if (odebug > 5)
	{
		tabpr();
		/* NOTREACHED */
	}
#endif

	nonest(p);
#ifdef FIXED_FRAME
	paintargs(p);
#ifndef NODBG
	if(rdebug) {
		fprintf(outfile,"After paintargs:\n*****\n");
		e2print(p);
		fprintf(outfile,"=====");
	}
#endif
#endif


	/* if we make drastic changes to the tree (e.g., introduce temps)
	** /* we will go back and do the whole thing again 
	*/
	/* statistics indicate that this happens about 10% of the time */
	/* if the percentage rises, there are many things that can be done to
	** /* improve matters 
	*/
	/* for example, RNODE, etc., could be removed by reader, and some of
	** /* the op rewriting could be discovered by reader as well 
	*/

	init_thrash();
again:

			/* various typechecking ops; also, set the
			   OCOPY flags*/
	(void)tnumbers(p);
	typecheck(p,0);

	/* move the comma ops as high as practical */

	rewcom( p, CEFF );
			/*Must do it again - we rewrote the tree*/
	if (tnumbers(p))
		goto again;

	cse_ptr = cse_list;

#ifndef NODBG
	if( odebug ) 
	{
		printf( "After goals are computed:" );
		e2print( p );
	}
#endif


			/*Parens at highest node are meaningless*/
	p->in.strat &= ~PAREN;
	/* do a trial code generation */
	nins = 0;
	switch (INSOUT( p, CEFF ))
	{
	case REWROTE:
		goto again;
	case OUTOFREG:
#ifndef NODBG
		e2print( p );
#endif
		cerror("Runs out of registers");
	/*default: fall thru*/
	}


	/* rewrite stored subtrees as assignments to temps, with COMOP's */
	flag = 0;
	for( i=0; i<nins; ++i ) 
	{
		if( inst[i].goal == CTEMP ) 
		{
#ifndef NODBG
			if( odebug ) 
			{
				printf( "subtree is stored in temp:\n" );
				e2print( inst[i].p );
			}
#endif
			if( rewsto( inst[i].p ) ) {
				if( !fast ) goto again;
				/* otherwise, rewrite all temps now */
				flag = 1;
			}
		}
	}
	if( flag ) goto again;

#ifndef NODBG
	if( odebug ) e2print(p);
	debug_thrash();
#endif
	/* if not just costing, output the actual instructions */
	if (!costing)
	    insprt();
}

static INST inst_init[INI_NINS];

TD_INIT(td_inst, INI_NINS, sizeof(INST), 0, inst_init, "instruction table");

#define CFIX(a,b) cfix(a, b, pi->rs_want, pi->rs_avail)

void
insprt()
{

	int i, ninsav;
	register INST *pi;
	register NODE *p;
	register OPTAB *q;
	register c, goal;
	int any_exact;			/* Flag:  on if any DOEXACTs in tree */
	extern int top_is_br;

	/* don't use nins for loop control in case IN_LINE is defined,
	/* giving the possibilty that nins gets changed, and insprt()
	/* gets called recursively.
	*/
	ninsav = nins;

	for( pi=inst,i=0; i<ninsav; ++i,++pi )
	{
		p = pi->p;
		q = pi->q;
		c = pi->goal;
		if( c == CCC && (q->rewrite&RESCC) ) goal = FORCC;
		else if( c == CEFF ) goal = FOREFF;
		else goal = INREG;
	if (top_is_br && (p->in.branch_stuff & PREDICTABLE_BRANCH)
		&& p->in.branch_stuff > 0) {
		 put_br_hint(p->in.branch_stuff);
	}
#ifndef NODBG
		if(odebug)
		{
			printf("INSPRT: %d c=",i);
			preff(c);
			printf(" goal=");
			prgoal(goal);
			printf("\n");
			e2print(p);
		}
#endif

#ifdef IN_LINE
	        if (p->in.op == INCALL || p->in.op == UNARY INCALL)
		{
		    /* handle asm inline calls here, don't use expand() */
		    int n;
		    char * savinst;

		    /* have to preserve inst[] since 
		    /* it may be overwritten when insout() is called
		    /* to generate code for argument evaluation or
		    /* a real function call
		    */
		    n = sizeof(INST) * (ninsav-i);
		    if (!(savinst = malloc((unsigned) n)))
			cerror("can't save instructions for INCALL");
		    (void) memcpy( savinst, (char*)(pi), (unsigned)n);

		    as_gencode((ND2 *) p, outfile);

		    /* At this point the asm call has either been expanded
		    /* or converted to a regular CALL or UNARY CALL
		    */

		    if (p->in.op == UNARY CALL || p->in.op == CALL)
		    {
		    	/* have to produce code for regular function call */
		    	nins = 0;
			if (INSOUT(p,CEFF))	/* expect zero return */
				cerror("Can't generate in-line CALL");
		    	insprt();  
		    }

		    /* put everything back the way it was */
		    /* reset "pi":  instruction generation may have resulted
		    ** in reallocation of the instruction array
		    */
		    pi = &inst[i];
		    (void) memcpy( (char*)(pi), savinst, (unsigned)n);
		    free( savinst );
		    nins = ninsav;  /* to be good and safe */

		    allo( p, q );		/* safe to allocate now */

		 }
		else	
#endif
		{ /* non-asm template expansion */


			/*If there are any DOEXACTs, protect them
			  from optimizers*/
			/*Also, if there are any ";lp" that have a
			  null left child, remove them from the tree*/

		if ( (any_exact = pre_ex(p)) != 0)
			protect(p);

# ifdef TMPSRET
		/* not the best place in the world, but... */
		if (p->in.op == STCALL || p->in.op == UNARY STCALL)
			expand(p, goal, TMPSRET, q);
#endif
		/* Look for the case where we wanted to use an ASGOP template
		** for a non-ASGOP tree.  If the left side of the tree is a
		** register variable, get it into a scratch register.
		*/

		allo( p, q );
		expand( p, goal, q->cstring, q );

		if ( any_exact)
			unprot(p);
		} /* end non-asm template expansion */


		reclaim( p, q->rewrite, goal );

		/* now, if condition codes needed, test */
		if( c == CCC && p->tn.op != CCODES ) 
		{
		        CFIX( p, CCC );
			if( p->tn.op != CCODES ) cerror( "ctest fails" );
		}
		else if( c>=0 && c<=NRGS) {
		    if (p->tn.op != REG )
			CFIX( p, NRGS );
		    /* make sure we're in a suitable register */
		    if ((pi->rs_want & RS_BIT(p->tn.sid)) == 0) {
			RST choices = pi->rs_want & pi->rs_avail;
			int regno;

			if (choices == 0)
			    cerror("no choices for RS_MOVE");

			regno = rs_rnum( RS_CHOOSE(choices) );
			RS_MOVE(p, regno);
			reallo(p, regno);	/* adjust reg. allo. info. */
		    }
		}
	}
}

int odebug = 0;

static void 
cfix( p, goal, want, avail )
NODE * p;
int goal;
RST want;
RST avail;
{
	/* p is to be fixed according to goal (CCC or NRGS) */
	OPTAB *q;
	NODE *pp;
	int r;
	int expgoal;

#ifndef NODBG
	if(odebug)
	{
		printf("CFIX: goal=");
		preff(goal);
		printf("\n");
		e2print(p);
	}
#endif
	if( goal == CCC ) 
	{
		r = RESCC;
		p->tn.goal = CCC;
	}
	else 
	{
		r = (RESC1|RESC2|RESC3);
		pp = getl( p );
		if( istnode( pp ) ) r |= RLEFT;
		pp = getr( p );
		if( istnode( pp ) ) r |= RRIGHT;
	}

	if( goal == CCC ) expgoal = FORCC;
	else expgoal = INREG;

	for( q=0; (q = match( p, q )) != 0; )
	{
		/* takes the first match (may not be cheapest) */
		/* template writers, take note! */
		if( q->rewrite & r ) 
		{
		/* rs_reclaim will set up the node to contain a bitmap 
		** for scratch registers needed by the template.
		*/
			/* these are set my match() */
			extern SHAPE *leftshape, *rightshape;
			static RST cfix_scratch();
			RST rregs = cfix_scratch(p);

			/* "rregs" are not also available. */
			if (RS_FAIL & rs_reclaim(p, q, goal,
						leftshape, RS_NONE,
						rightshape, rregs,
						avail & ~rregs, want))
			    cerror("cfix rs_reclaim fails");
			/* generate the code on the spot */
			allo( p, q );
# ifdef TMPSRET
			/* likewise */
			if (p->in.op == STCALL || p->in.op == UNARY STCALL)
				expand(p, expgoal, TMPSRET, q);
#endif
			expand( p, expgoal, q->cstring, q );
			reclaim( p, q->rewrite, expgoal );
			return;
		}
	}
#ifndef NODBG
	e2print(p);
#endif
        cerror( "cfix trouble" );
}

/* Determine the register bit vector that corresponds to the
** tree p.  We're only interested in scratch registers here:
** they're the ones that are no longer available.  We must make
** a recursive tree walk and OR together the bits of all scratch
** register found.  Also take account of register pairs.
*/
static RST
cfix_scratch(p)
NODE * p;
{
    RST curbits = 0;

    switch( optype(p->in.op) ){
    case BITYPE:
	curbits |= cfix_scratch(p->in.right);
	/*FALLTHRU*/
    case UTYPE:
	curbits |= cfix_scratch(p->in.left);
	break;
    case LTYPE:
	if (istnode( p )) {
	    curbits = RS_BIT(p->tn.sid);
	    /* account for register pairs */
	    /*CONSTANTCONDITION*/
	    if (szty(p->tn.type) > 1)
		curbits |= RS_PAIR(curbits);
	}
	break;
    }
    return(curbits);
}

void
cfix_regsused(p, rsp) /* same as the above, but for all registers */
NODE *p;
RST *rsp;
{
    switch (optype(p->in.op)) {
    case BITYPE:
	cfix_regsused(p->in.right, rsp);
	/*FALLTHRU*/
    case UTYPE:
	cfix_regsused(p->in.left, rsp);
	break;
    case LTYPE:
	if (p->in.op == REG) {
	    *rsp |= RS_BIT(p->tn.sid);
	    /*CONSTANTCONDITION*/
	    if (szty(p->tn.type) > 1)
		*rsp |= RS_PAIR(RS_BIT(p->tn.sid));
	}
	break;
    }
}

#ifndef NODBG
preff(c)
{
	char buf[20];
	register char *p;

	switch (c)
	{
	default:    p = buf; sprintf(buf, "%#o", c); break;
	case CCC:   p = "CCC";   break;
	case CTEMP: p = "CTEMP"; break;
	case CEFF:  p = "CEFF";  break;
	case NRGS:  p = "NRGS";  break;
	}
	printf("%s",p);
}
#endif
