#ident	"@(#)ksh93:src/cmd/ksh93/sh/array.c	1.2"
#pragma prototyped
/*
 * Array processing routines
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 2B-102
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *   research!dgk
 *
 */

#include	"defs.h"
#include	<stak.h>
#include	"name.h"

#define NUMSIZE	(4+(ARRAY_MAX>999)+(ARRAY_MAX>9999)+(ARRAY_MAX>99999))
#define is_associative(ap)	array_assoc((Namarr_t*)(ap))

struct index_array
{
        Namarr_t        header;
        short           cur;    /* index of current element */
        short           maxi;   /* maximum index for array */
        union Value     val[1]; /* array of value holders */
};

/*
 *   Calculate the amount of space to be allocated to hold an
 *   indexed array into which <maxi> is a legal index.  The number of
 *   elements that will actually fit into the array (> <maxi>
 *   but <= ARRAY_MAX) is returned.
 *
 */
static int	arsize(register int maxi)
{
	register int i = roundof(maxi,ARRAY_INCR);
	return (i>ARRAY_MAX?ARRAY_MAX:i);
}

/*
 *        Increase the size of the indexed array of elements in <arp>
 *        so that <maxi> is a legal index.  If <arp> is 0, an array
 *        of the required size is allocated.  A pointer to the 
 *        allocated Namarr_t structure is returned.
 *        <maxi> becomes the current index of the array.
 */
static struct index_array *array_grow(register struct index_array *arp,int maxi)
{
	register struct index_array *ap;
	register int i=0;
	register int newsize = arsize(maxi+1);
	if (maxi >= ARRAY_MAX)
		error(ERROR_exit(1),gettxt(e_subscript_id,e_subscript), fmtbase((long)maxi,10,0));
	ap = new_of(struct index_array,(newsize-1)*sizeof(union Value*));
	ap->maxi = newsize;
	ap->cur = maxi;
	if(arp)
	{
		ap->header = arp->header;
		for(;i < arp->maxi;i++)
			ap->val[i].cp = arp->val[i].cp;
		free((void*)arp);
	}
	else
	{
		ap->header.nelem = 0;
		ap->header.fun = 0;
	}
	for(;i < newsize;i++)
		ap->val[i].cp = 0;
	return(ap);
}

/*
 * Change ARRAY_UNDEF as appropriate
 * Allocate the space if necessary, if flag is ARRAY_ASSIGN
 * Check for bounds violation for indexed array
 */
void array_check(Namval_t *np,int flag)
{
	register struct index_array *ap = (struct index_array*)nv_arrayptr(np);
	if(ap->header.nelem&ARRAY_UNDEF)
	{
		ap->header.nelem &= ~ARRAY_UNDEF;
		/* delete array is the same as delete array[@] */
		if(flag&ARRAY_DELETE)
		{
			nv_putsub(np, NIL(char*), ARRAY_SCAN);
			ap->header.nelem |= ARRAY_SCAN;
		}
		else /* same as array[0] */
		{
			if(is_associative(ap))
				(*ap->header.fun)(np,"0",0);
			else
				ap->cur = 0;
		}
	}
	if(!is_associative(ap))
	{
		if(!(ap->header.nelem&ARRAY_SCAN) && ap->cur >= ap->maxi)
		{
			ap = array_grow(ap, (int)ap->cur);
			np->nvalue.array = (Namarr_t*)ap;
		}
		if(ap->cur>=ap->maxi)
			error(ERROR_exit(1),gettxt(e_subscript_id,e_subscript), nv_name(np));
	}
}

/*
 * Get the Value pointer for an array.
 * Delete space as necessary if flag is ARRAY_DELETE
 * After the lookup is done the last @ or * subscript is incremented
 */
union Value *array_find(Namval_t *np,int flag)
{
	register struct index_array *ap = (struct index_array*)nv_arrayptr(np);
	register union Value *up;
	register unsigned dot=0;
	if(is_associative(ap))
		up = (union Value*)((*ap->header.fun)(np,NIL(char*),0));
	else
	{
		if((dot=ap->cur) >= ap->maxi)
			error(ERROR_exit(1),gettxt(e_subscript_id,e_subscript), nv_name(np));
		up = &(ap->val[dot]);
	}
	if(!up->cp)
	{
		if(flag==ARRAY_LOOKUP)
			return(0);
		ap->header.nelem++;
	}
	if(flag==ARRAY_DELETE)
	{
		ap->header.nelem--;
		if(is_associative(ap))
		{
			(*ap->header.fun)(np, NIL(char*), NV_ADELETE);
			if(!(ap->header.nelem&ARRAY_SCAN))
				return(up);
		}
		if(!(ap->header.nelem&ARRAY_MASK))
		{
			const char *cp = up->cp;
			up->cp = 0;
			if(is_associative(ap))
				(*ap->header.fun)(np, NIL(char*), NV_AFREE);
			nv_offattr(np,NV_ARRAY);
			np->nvalue.cp = cp;
			free((char*)ap);
			up = &np->nvalue;
		}
	}
	return(up);
}


/*
 * Verify that argument is an indexed array and convert to associative,
 * freeing relevant storage
 */
static Namarr_t *nv_changearray(Namval_t *np, void *(*fun)(Namval_t*,const char*,int))
{
	register Namarr_t *ap;
	char numbuff[NUMSIZE];
	unsigned dot, digit, n;
	union Value *up;
	struct index_array *save_ap;
	register char *string_index=&numbuff[NUMSIZE];
	numbuff[NUMSIZE]='\0';

	if(!fun || !(ap = nv_arrayptr(np)) || is_associative(ap))
		return(NIL(Namarr_t*));

	save_ap = (struct index_array *)ap;
	ap = (Namarr_t*)((*fun)(np, NIL(char*), NV_AINIT));
	ap->nelem = 0;
	ap->fun = fun;
	np->nvalue.array = ap;
	nv_onattr(np,NV_ARRAY);

	for(dot = 0; dot < (unsigned)save_ap->maxi; dot++)
	{
		if(save_ap->val[dot].cp)
		{
			if ((digit = dot)== 0)
				*--string_index = '0';
			else while( n = digit )
			{
				digit /= 10;
				*--string_index = '0' + (n-10*digit);
			}
			nv_putsub(np, string_index, ARRAY_ADD);
			up=array_find(np,0);
			up->cp = save_ap->val[dot].cp;
		}
		string_index = &numbuff[NUMSIZE];
	}
	free(save_ap);
	return(ap);
}
/*
 * set the associative array processing method for node <np> to <fun>
 * The array pointer is returned if sucessful.
 */
Namarr_t *nv_setarray(Namval_t *np, void *(*fun)(Namval_t*,const char*,int))
{
	register Namarr_t *ap;

	if(fun && (ap = nv_arrayptr(np)) && !is_associative(ap))
	{

	/* if it's already an indexed array, convert to associative structure */

		ap = nv_changearray(np, fun);
		return(ap);
	}

	if(fun && !nv_arrayptr(np) && (ap = (Namarr_t*)((*fun)(np, NIL(char*), NV_AINIT))))
	{

	/* check for preexisting initialization and save */

		char *value = nv_getval(np);
		ap->nelem = 0;
		ap->fun = fun;
		np->nvalue.array = ap;
		nv_onattr(np,NV_ARRAY);
		if(value)
		{
			nv_putsub(np, "0", ARRAY_ADD);
			nv_putval(np, value, 0);
		}
		return(ap);
	}
	return(NIL(Namarr_t*));
}

/*
 * This routine sets subscript of <np> to the next element, if any.
 * The return value is zero, if there are no more elements
 * Otherwise, 1 is returned.
 */
int nv_nextsub(Namval_t *np)
{
	register struct index_array *ap = (struct index_array*)nv_arrayptr(np);
	register unsigned dot;
	if(!ap || !(ap->header.nelem&ARRAY_SCAN))
		return(0);
	if(is_associative(ap))
	{
		if((*ap->header.fun)(np,NIL(char*),NV_ANEXT))
			return(1);
		ap->header.nelem &= ~ ARRAY_SCAN;
		return(0);
	}
	for(dot=ap->cur+1; dot <  (unsigned)ap->maxi; dot++)
	{
		if(ap->val[dot].cp)
		{
			ap->cur = dot;
			return(1);
		}
	}
	ap->header.nelem &= ~ ARRAY_SCAN;
	ap->cur = 0;
	return(0);
}

/*
 * Set an array subscript for node <np> given the subscript <sp>
 * An array is created if necessary.
 * <mode> can be a number, plus or more of symbolic constants
 *    ARRAY_SCAN, ARRAY_UNDEF, ARRAY_ADD
 * The node pointer is returned which can be NULL if <np> is
 *    not already array and the ARRAY_ADD bit of <mode> is not set.
 */
Namval_t *nv_putsub(Namval_t *np,register char *sp,register long mode)
{
	register struct index_array *ap = (struct index_array*)nv_arrayptr(np);
	register int size = (mode&ARRAY_MASK);
	if(!ap || !ap->header.fun)
	{
		if(sp)
			size = (int)sh_arith((char*)sp);
		if(size >= ARRAY_MAX || (size < 0))
		{
			error(ERROR_exit(1),gettxt(e_subscript_id,e_subscript), nv_name(np));
			return(NIL(Namval_t*));
		}
		if(!ap || size>=ap->maxi)
		{
			register struct index_array *apold;
			if(size==0)
				return(NIL(Namval_t*));
			ap = array_grow(apold=ap,size);
			if(!apold && (ap->val[0].cp=np->nvalue.cp))
				ap->header.nelem++;
			np->nvalue.array = (Namarr_t*)ap;
			nv_onattr(np,NV_ARRAY);
		}
		ap->header.nelem &= ~(ARRAY_SCAN|ARRAY_UNDEF);
		ap->header.nelem |= (mode&(ARRAY_SCAN|ARRAY_UNDEF));
		ap->cur = size;
		if((mode&ARRAY_SCAN) && !ap->val[size].cp && nv_nextsub(np))
			np = 0;
		return((Namval_t*)np);
	}
	ap->header.nelem &= ~(ARRAY_SCAN|ARRAY_UNDEF);
	ap->header.nelem |= (mode&(ARRAY_SCAN|ARRAY_UNDEF));
	if(sp)
		ap = (struct index_array*)(*ap->header.fun)(np, sp, (mode&ARRAY_ADD)?NV_AADD:0);
	else if(mode&ARRAY_UNDEF)
		(*ap->header.fun)(np, "",0);
	if((mode&ARRAY_SCAN) && !nv_nextsub(np))
		np = 0;
	return(np);
}

/*
 * process an array subscript for node <np> given the subscript <cp>
 * returns pointer to character after the subscript
 */
char *nv_endsubscript(Namval_t *np, register char *cp, int mode)
{
	register int count=1, quoted=0, c;
	register char *sp = cp+1;
	/* first find matching ']' */
	while(count>0 && (c= *++cp))
	{
		if(c=='\\')
		{
			quoted=1;
			cp++;
		}
		else if(c=='[')
			count++;
		else if(c==']')
			count--;
	}
	*cp = 0;
	if(quoted)
	{
		/* strip escape characters */
		count = staktell();
		stakwrite(sp,1+cp-sp);
		sh_trim(sp=stakptr(count));
	}
	if(mode)
		nv_putsub(np, sp, ARRAY_ADD);
	if(quoted)
		stakseek(count);
	*cp = c;
	if(*++cp == '[')
		error(ERROR_exit(1),gettxt(e_subscript_id,e_subscript), nv_name(np));
	return(cp);
}

Namval_t *nv_opensub(Namval_t* np)
{
	register struct index_array *ap = (struct index_array*)nv_arrayptr(np);
	if(ap && is_associative(ap))
		return((Namval_t*)((*ap->header.fun)(np,NIL(char*),NV_ACURRENT)));
	return(NIL(Namval_t*));
}

char	*nv_getsub(Namval_t* np)
{
	static char numbuff[NUMSIZE];
	register struct index_array *ap = (struct index_array*)nv_arrayptr(np);
	register unsigned dot, n;
	register char *cp = &numbuff[NUMSIZE];
	if(!ap)
		return(NIL(char*));
	if(is_associative(ap))
		return((char*)((*ap->header.fun)(np,NIL(char*),NV_ANAME)));
	if((dot = ap->cur)==0)
		*--cp = '0';
	else while(n=dot)
	{
		dot /= 10;
		*--cp = '0' + (n-10*dot);
	}
	return(cp);
}

/*
 * If <np> is an indexed array node, the current subscript index
 * retuned, otherwise returns -1
 */
int nv_aindex(register Namval_t* np)
{
	Namarr_t *ap = nv_arrayptr(np);
	if(!ap || is_associative(ap))
		return(-1);
	return(((struct index_array*)(ap))->cur&ARRAY_MASK);
}

struct assoc_array
{
	Namarr_t	header;
	Hashtab_t	*table;
	Hashpos_t	*pos;
	Namval_t	*cur;
};


/*
 *  This is the default implementation for associate arrays
 */
void *nv_associative(register Namval_t *np,const char *sp,int mode)
{
	register struct assoc_array *ap = (struct assoc_array*)nv_arrayptr(np);
	register int type;
	switch(mode)
	{
	    case NV_AINIT:
		if(ap = (struct assoc_array*)malloc(sizeof(struct assoc_array)))
		{
			ap->table = hashalloc(sh.var_tree,0);
			ap->cur = 0;
			ap->pos = 0;
		}
		return((void*)ap);
	    case NV_ADELETE:
		if(ap->cur)
			hashlook(ap->table,(char*)ap->cur,HASH_DELETE|HASH_BUCKET,0);
		ap->cur = 0;
		return((void*)ap);
	    case NV_AFREE:
		if(ap->pos)
			hashdone(ap->pos);
		hashfree(ap->table);
		return((void*)ap);
	    case NV_ANEXT:
		if(!ap->pos)
			ap->pos = hashscan(ap->table,0);
		while(ap->cur=(Namval_t*)hashnext(ap->pos))
		{
			if(ap->cur->nvalue.cp)
				return((void*)ap);
		}
		hashdone(ap->pos);
		ap->pos = 0;
		return(NIL(void*));
	    case NV_ACURRENT:
			return((void*)ap->cur);
	    case NV_ANAME:
		if(ap->cur)
			return((void*)nv_name(ap->cur));
		return(NIL(void*));
	    default:
		if(sp)
		{
			if(ap->pos)
			{
				hashdone(ap->pos);
				ap->pos = 0;
			}
			type = nv_isattr(np,NV_PUBLIC&~NV_ARRAY);
			if((np=nv_search(sp,ap->table,mode?NV_ADD:0)) && nv_isnull(np))
				nv_onattr(np,type);
			ap->cur = np;
		}
		if(ap->cur)
			return((void*)(&ap->cur->nvalue));
		else
			return((void*)(&ap->cur));
	}
}

/*
 * Assign values to an array
 */
void nv_setvec(register Namval_t *np,register int argc,register char *argv[])
{
	while(--argc >= 0)
	{
		if(argc>0  || nv_isattr(np,NV_ARRAY))
			nv_putsub(np,NIL(char*),(long)argc);
		nv_putval(np,argv[argc],0);
	}
}

