#ident	"@(#)cg:common/xdefs.c	1.11"

# include "mfile1.h"
# include "mfile2.h"
# include <memory.h>		/* for memcpy() */
# include <malloc.h>
# include <unistd.h>

/*	communication between lexical routines	*/

int	lineno;		/* line number of the input file */

FILE * outfile = stdout;	/* place to write all output */
FILE * textfile = stdout;	/* user-requested output file */

/*	symbol table maintainence */

int	curftn;  /* "current" function */
int	strftn;  /* 1 if current function returns struct or union */
int	ftnno;  /* "current" function number */
int	curloc;		  /* current location counter value */

RST	regvar;		/* currently busy register variable bitmap */
int	nextrvar;	/* the next allocated reg (set by cisreg) */
OFFSZ	inoff;		/* offset of external element being initialized */

/* debugging flag */
int xdebug = 0;
int idebug = 0;

int strflg;  /* if on, strings are to be treated as lists */

#ifdef IMPSWREG
	int swregno;
#endif
int retlab = NOLAB;

#ifdef IMPREGAL
	/* for register allocation optimizations */

	int fordepth;	/* nest depth of 'for' loops */
	int whdepth;	/* nest depth of 'while' and 'do-while' loops */
	int brdepth;	/* nest depth of 'if', 'if-else', and 'switch' */
#endif

char costing = 0;	/* 1 if we are costing an expression */
int str_spot = -1;		/* place for structure return */

/* function to enlarge a table described by a table descriptor */

int
td_enlarge(tp,minsize)
register struct td * tp;
int minsize;				/* minimum size needed:  0 means 1 more
					** than current
					*/
{
    int oldsize = tp->td_allo;		/* old size (for return) */
    unsigned int ocharsize = tp->td_allo * tp->td_size; /* old size in bytes */
    int newsize;			/* new size in storage units */
    unsigned int ncharsize;		/* size of new array in bytes */

    /* Realloc() previously malloc'ed tables, malloc() new one.
    ** If "end" were part of the C library, a check would have been
    ** done on the current value of the pointer, instead of having a
    ** bit in the td flags.
    */

/*    printf("%s changes from	%#lx - %#lx\n", tp->td_name,
			tp->td_start, tp->td_start+ocharsize); */

    /* determine new size:  must be "large enough" */
    newsize = tp->td_allo;		/* start at old size */
    do {
	newsize *= 2;
    } while (newsize < minsize);	/* note:  always false for minsize==0 */
    ncharsize = newsize * tp->td_size;	/* size of new array in bytes */

    if (tp->td_flags & TD_MALLOC)
	tp->td_start = realloc(tp->td_start, ncharsize);
    else {
	myVOID * oldptr = tp->td_start;

	/* copy old static array */
	if ((tp->td_start = malloc(ncharsize)) != 0)  /*lint*/
	    (void)memcpy(tp->td_start, oldptr, ocharsize);
    }
    tp->td_flags |= TD_MALLOC;		/* array now unconditionally malloc'ed */

    if (!tp->td_start)
	cerror(gettxt(":691","can't get more room for %s"), tp->td_name);
    
/*    printf("		to	%#lx - %#lx\n",
			tp->td_start, tp->td_start+ncharsize); */
    /* zero out new part of array:  node and symbol tables expect this */
    if (tp->td_flags & TD_ZERO)
	(void)memset((char *) tp->td_start + ocharsize,0,(ncharsize-ocharsize));

    tp->td_allo = newsize;
    return oldsize;
}
