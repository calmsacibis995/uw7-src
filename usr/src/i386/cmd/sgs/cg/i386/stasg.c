#ident	"@(#)cg:i386/stasg.c	1.7.1.14"
/*	stasg.c - machine dependent grunge for back end
 *	i386 CG
 *              Intel iAPX386
 */

# include <unistd.h>
# include "mfile1.h"
# include "mfile2.h"

static void smovregstate();
static void bringtoreg();
static void adrstasg();
static void incby();

extern int zflag;
extern RST regstused;   	/* List of registers used for structure moves */
extern int vol_opnd, special_opnd;
extern int cur_opnd;
#define CLEAN()     {vol_opnd = 0; special_opnd = 0; cur_opnd = 1;}

#define istnode(p)	((p)->in.op==REG && istreg((p)->tn.sid))


    /* Threshold value for unrolling string move into word moves.  The
     * value is computed as
     *              .-                                                   -.
     *              | smov + mov*(sibusy+dibusy) + load*(2-(siset+diset)) |
     * Threshold =  | --------------------------------------------------- |
     *              |                 mov - rep                           |
     * smov is the number of cycles to start a smov == 7
     * mov is the cost to load/store                == 6
     * load is the cost to load an address          == 4 if must use mov,
     *                                              == 2 if can use lea.
     * rep is the cost to "rep smov" a word - smov  == 4
     * [sd]ibusy is 1/0 if/ifnot [sd]i has an active value
     * [sd]iset is 1/0 if/ifnot [sd]i has correct address value as temp
     */
# define THRESHOLD(lop,rop,sibusy,dibusy,siset,diset)                        \
	( ( 1 + 7 + 6*( (sibusy)+(dibusy) ) +                                \
	    (( (lop)!=STAR && (lop)!=VPARAM && (lop)!=TEMP &&                \
		    (lop)!=VAUTO && (lop)!=NAME )             ? 2 : 4 ) *    \
		( 1 - (diset) ) +                                            \
	    (( (rop)!=STAR && (rop)!=VPARAM && (rop)!=TEMP &&                \
		    (rop)!=VAUTO && (rop)!=NAME )             ? 2 : 4 ) *    \
		( 1 - (siset) )                                           )  \
	  / 2 )

static OFFSZ edi_tmp, esi_tmp;

	/*
	** Following does not reclaim temp used in structure copy
	** We could prolly call set_next_temp() to set it back
	** to the value at the beginning of the structure copy.
	*/
static void
restore_index_reg(reg)
{
#ifdef FIXED_FRAME
    if(fixed_frame()) {
	OFFSZ tmp;
	if(reg == REG_ESI) tmp = esi_tmp;
	else if(reg == REG_EDI) tmp = edi_tmp;
	else cerror(gettxt(":0", "bad register restore attempt" ));
	fprintf(outfile,"\tmovl\t");
	printout_stack_addr(tmp);
	fprintf(outfile,",%s\n",rnames[reg]);
    }
    else
#endif
    fprintf(outfile,"\tpopl\t%s\n", rnames[reg]);
}

static void
save_index_reg(reg)
int reg;
{
#ifdef FIXED_FRAME
    if(fixed_frame()) {
	OFFSZ tmp;
	tmp = freetemp(1) / SZCHAR;

	if(reg == REG_ESI) esi_tmp = tmp;
	else if(reg == REG_EDI) edi_tmp = tmp;
	else cerror(gettxt(":0", "bad register save attempt" ));

	fprintf(outfile,"\tmovl\t%s,", rnames[reg]);
	printout_stack_addr(tmp);
	putc('\n',outfile);
    }
    else
#endif
    fprintf(outfile,"\tpushl\t%s\n", rnames[reg]);
}

void
stasg( l, r, stsize, q )
register NODE *l, *r;
register BITOFF stsize;
OPTAB *q;
{
    int sibusy, siset, dibusy, diset;
    int stwords, stchars;

    int vol_flg = 0;
    if ( (l->in.strat & VOLATILE) || (r->in.strat & VOLATILE))
        vol_flg = (VOL_OPND1 | VOL_OPND2);

    smovregstate( &sibusy, &siset, REG_ESI, r );
    smovregstate( &dibusy, &diset, REG_EDI, l );

    stwords = stsize/SZLONG;
    stchars = ( stsize%SZLONG )/SZCHAR;
    if( stwords >=
	    THRESHOLD( l->in.op, r->in.op, sibusy, dibusy, siset, diset ) ) {
	/* Use rep smov for stasg */

	int llreg = optype(l->in.op) == LTYPE ? -1 : regno(l->in.left);
	int rlreg = optype(r->in.op) == LTYPE ? -1 : regno(r->in.left);
	int lreg  = regno(l);
	int rreg  = regno(r);
#ifdef FIXED_FRAME
	OFFSET save_temp_offset;

	if(fixed_frame())
		save_temp_offset = get_temp_offset();
	else
#endif
	    /* special code for stargs so we can still address off %esp */
	{
	if( l->in.op == PLUS && llreg == REG_ESP && sibusy+dibusy ) {
	    INTCON sz;
	    (void)num_fromslong(&sz, (long)(sibusy + dibusy) * (SZINT / SZCHAR));
	    (void)num_sadd(&l->in.right->tn.c.ival, &sz);
	  }
	if( r->in.op == PLUS && rlreg == REG_ESP && sibusy+dibusy ) {
	    INTCON sz;
	    (void)num_fromslong(&sz, (long)(sibusy + dibusy) * (SZINT / SZCHAR));
	    (void)num_sadd(&r->in.right->tn.c.ival, &sz);
	  }
	}

	if ( lreg == REG_ESI ) {
		if ( rreg == REG_EDI ) {
			save_index_reg(REG_ESI);
			save_index_reg(REG_EDI);
			emit_str("\txchgl\t%esi,%edi\n");
		} else {
			bringtoreg( l, REG_EDI, q, diset, dibusy );
			bringtoreg( r, REG_ESI, q, siset, sibusy );
		}
	} else {
		bringtoreg( r, REG_ESI, q, siset, sibusy );
		bringtoreg( l, REG_EDI, q, diset, dibusy );
	}

	fprintf( outfile,"\tmovl\t$%d,%%ecx\n\trep\n\tsmovl", stwords );
	if( zflag ) {
	    emit_str( "\t\t/ STASG\n");
	}
	putc( '\n',outfile );

	/* Special case for the rep smovl. */
	if (vol_flg) PUTS("/VOL_OPND 0\n");

	if (stchars & 2) {
	    stchars -= 2;
	    fprintf( outfile,"\tmovw\t%d(%%esi),%%cx\n", stchars);
	    if (vol_flg) PUTS("/VOL_OPND 1\n");
	    fprintf( outfile,"\tmovw\t%%cx,%d(%%edi)\n", stchars );
	    if (vol_flg) PUTS("/VOL_OPND 2\n");
	}
	if ( stchars-- ) {
	    fprintf( outfile,"\tmovb\t%d(%%esi),%%cl\n", stchars );
            if (vol_flg) PUTS("/VOL_OPND 1\n");
	    fprintf( outfile,"\tmovb\t%%cl,%d(%%edi)\n", stchars );
            if (vol_flg) PUTS("/VOL_OPND 2\n");
	}

	if ( lreg == REG_ESI ) {
		if ( rreg == REG_EDI ) {
			    restore_index_reg(REG_EDI);
			    restore_index_reg(REG_ESI);
		} else {
			if( sibusy )
			    restore_index_reg(REG_ESI);
			if( dibusy )
			    restore_index_reg(REG_EDI);
		}
	} else {
		if( dibusy )
		    restore_index_reg(REG_EDI);
		if( sibusy )
		    restore_index_reg(REG_ESI);
	}

#ifdef FIXED_FRAME
		/* Allow any temp space to be reused at this
		** point.
		*/
	if(fixed_frame())
		restore_temp_offset(save_temp_offset);
#endif

    } else {
	/* Unroll stasg into multiple movl x,%ecx / movl %ecx,y */
	NODE lt, rt;
	char *comment;

	comment = zflag ? "\t\t/ STASG\n" : "\n" ;

	adrstasg( &rt, r, q, 2 );
	adrstasg( &lt, l, q, 3 );

	while( stwords-- ) {
	    expand( &rt, FOREFF, "\tmovlZB4\tA.,ZA1Zb", q );
	    emit_str( comment);
            CLEAN();
            if (vol_flg) PUTS("/VOL_OPND 1\n");
	    expand( &lt, FOREFF, "\tmovlZB4\tZA1Zb,A.", q );
	    emit_str( comment);
            CLEAN();
            if (vol_flg) PUTS("/VOL_OPND 2\n");
	    incby( &rt, SZLONG/SZCHAR );
	    incby( &lt, SZLONG/SZCHAR );
	}

	if ( stchars & 2 ) {
	    stchars -= 2;
	    expand( &rt, FOREFF, "\tmovwZB2\tA.,ZA1Zb", q );
	    emit_str( comment);
            CLEAN(); 
            if (vol_flg) PUTS("/VOL_OPND 1\n");
	    expand( &lt, FOREFF, "\tmovwZB2\tZA1Zb,A.", q );
	    emit_str( comment);
            CLEAN();
            if (vol_flg) PUTS("/VOL_OPND 2\n");
	    incby( &rt, SZSHORT/SZCHAR );
	    incby( &lt, SZSHORT/SZCHAR );
	}

	if ( stchars-- ) {
	    expand( &rt, FOREFF, "\tmovbZB1\tA.,ZA1Zb", q );
	    emit_str( comment);
            CLEAN(); 
            if (vol_flg) PUTS("/VOL_OPND 1\n");
	    expand( &lt, FOREFF, "\tmovbZB1\tZA1Zb,A.", q );
	    emit_str( comment);
            CLEAN();
            if (vol_flg) PUTS("/VOL_OPND 2\n");
	    incby( &rt, 1 );    /* SZCHAR/SZCHAR */
	    incby( &lt, 1 );
	}
	switch(optype(lt.in.op)) {
	case BITYPE:
		tfree(lt.in.right);
		/* FALLTHRU */
	case UTYPE:
		tfree(lt.in.left);
	}

	switch(optype(rt.in.op)) {
	case BITYPE:
		tfree(rt.in.right);
		/* FALLTHRU */
	case UTYPE:
		tfree(rt.in.left);
	}
    }
}

    /* Determine state of an index register for use in an smov.  Return
     * flags indicating if register reg is set properly for tree p, or
     * failing that, if reg has a live value in it.
     */
static void
smovregstate( used, set, reg, p )
int *used, *set, reg;
register NODE *p;
{
    extern char request[];

    if( istnode( p ) && p->in.op == REG && p->tn.sid == reg ) {
	*set = 1;
	*used = 0;
    } else {
	*set = 0;
	*used = request[reg];
    }
}

    /* Bring address represented by tree p into register reg, anticipatory
     * to doing an smov.  Register reg's state is indicated by whether it
     * is already set, or has a live value in it and therefore is busy.
     */
static void
bringtoreg( p, reg, q, set, busy )
register NODE *p;
int reg, set, busy;
OPTAB *q;
{
NODE t;

    if( !set ) {
	if( busy ) {
		save_index_reg(reg);
	} else
	    regstused |= RS_BIT(reg);
	switch( p->in.op ) {
	case VAUTO:
	case TEMP:
	case VPARAM:
	case NAME:
	case STAR:
	case REG:
	case ICON:
	    if ( p->tn.sid != reg ) {
		expand( p, FOREFF, "\tmovl\tA.,", q );
		fprintf( outfile,"%s\n", rnames[reg] );
	    }
	    break;
	case CSE:	
	    {
		int preg = regno(p);
		if ( preg != reg ) {
			expand( p, FOREFF, "\tmovl\tA.,", q );
			fprintf( outfile,"%s\n", rnames[reg] );
		}
		break;
	    }
	default:
	    t.in.op = STAR;
	    t.in.left = p;
	    expand( &t, FOREFF, "\tleal\tA.,", q );
	    fprintf( outfile,"%s\n", rnames[reg] );
	    break;
	}
    }
}

    /* Return a new tree, starting with the existing NODE at newp, that
     * represents an addressing mode suitable for movl.  This is used
     * for generating unrolled stasgs.
     */
static void
adrstasg( newp, p, q, tempno )
register NODE *newp, *p;
OPTAB *q;
int tempno;
{
    int regnum;

    newp->tn.type = p->in.type;
    switch( p->in.op ) {
    case NAME:
    case STAR:
    case VAUTO:
    case TEMP:
    case VPARAM:    /* pick up the address into a scratch reg */
	if( tempno == 2 )
	    expand( p, FOREFF, "\tmovl\tA.,A2\n", q );
	else if( tempno == 3 )
	    expand( p, FOREFF, "\tmovl\tA.,A3\n", q );
	else
	    expand( p, FOREFF, "\tmovl\tA.,A1\n", q );
	regnum = resc[tempno-1].tn.sid;
	goto starreg;
    case REG:       /* make it into an OREG */
    case CSE:
	regnum = regno(p);
starreg:
	if( regnum == REG_EBP 
#ifdef FIXED_FRAME
		/* in fixed frame %ebp is just another user register */
		&& ! fixed_frame()
#endif
	) {
	    newp->tn.op = VAUTO;
	} else {
	    newp->tn.op = STAR;
	    ( newp = ( newp->in.left = talloc() ) )->in.op = PLUS;
	    ( newp->in.left = talloc() )->in.op = REG;
	    ( newp->in.right = talloc() )->in.op = ICON;
	    newp->in.type = newp->in.left->tn.type = p->in.type;
	    newp->in.left->tn.sid = regnum;
	    newp->in.right->tn.name = 0;
	    newp->in.right->tn.c.ival = num_0;
	}
	break;
    case ICON:
	*newp = *p;
	newp->tn.op = NAME;
	(void)num_toslong(&p->tn.c.ival, &newp->tn.c.off);
	break;
    case UNARY AND:
	*newp = *p;
	newp->in.left = tcopy( p->in.left );
	break;
    default:
	newp->in.op = STAR;
	newp->in.left = tcopy( p );
	break;
    }
}

    /* Increment an address for use in each iteration of the unrolled stasg */
static void
incby( p, inc )
register NODE *p;
register inc;
{

recurse:
    switch( p->in.op ) {
    case VPARAM:
    case VAUTO:
    case NAME:
	p->tn.c.off += inc;
	break;
    case TEMP:
	p->tn.c.off += inc * SZCHAR;
	break;
    case UNARY AND:
    case STAR:
	p = p->in.left;
	goto recurse;
    case PLUS:
    case MINUS:
	if (p->in.right->in.op == ICON)
	    p = p->in.right;
	else
	    p = p->in.left;
	/*FALLTHRU*/
    case ICON:
    {
	INTCON tmp;

	(void)num_fromslong(&tmp, (long)inc);
	(void)num_sadd(&p->tn.c.ival, &tmp);
	break;
    }
    default:
	cerror(gettxt(":727", "incby: bad node op" ));
	/* NOTREACHED */
    }
}

