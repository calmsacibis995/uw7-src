#ident	"@(#)libc-i386:gen/cvtl.c	1.10"

#ifndef DSHLIB
#ifdef __STDC__
	#pragma weak ecvtl = _ecvtl
	#pragma weak fcvtl = _fcvtl
	#pragma weak gcvtl = _gcvtl
#endif
#endif

#include "synonyms.h"
#include "fpemu.h"
#include <values.h>
#include "fpparts.h"
#include <float.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <locale.h>
#include <math.h>
#include <string.h>
#include "_locale.h"

#define	BITS_PER_MAG	16		/* Each internal magnitude word holds
					** 16 bits
					*/
/* Description of double-extended layout. */

#define	X_EXPBIAS	((1 << (_LDEXPLEN-1))-1)
/* Some machines (80387) bias denormalized numbers an extra number of
** bits so the high-order bit(s) of the significand are zero.
*/
#ifndef	X_DENORM_BIAS
#define	X_DENORM_BIAS	0
#endif

static	char *	cvtl();
extern long double frexpl();

/* Permit const declarations by nulling out for non-ANSI C. */
#if !defined(const) && !defined(__STDC__)
#define const
#endif


/* Routines, data to support internal double-extended to ASCII.  The
** conversion process uses "decimal floating point" (dfp) to do the
** calculations.  The representation consists a fixed number of chunks
** for the magnitude, and an exponent.  A DFP number then consists of
** a bunch of digits, right-justified, 4 (a parameter) per chunk,
** with an implicit decimal point to the right of the low-order digit.
** Because it's DECIMAL floating point, each chunk has a value 0-9999.
** An exponent gives the actual position of the decimal point.  Thus
** 117.3 would be represented by the lower-order chunk containing
** 1173, and an exponent of -1.
*/

#define ODIGITS 20

/* Assume we can fit 4 digits in an unsigned short. */
#define DFP_DIGITS 4			/* digits per chunk */
#define	DFP_MAXVAL 10000		/* 10^DFP_DIGITS */

/* Number of fraction words needed. */
#define DFP_NFRAC (((ODIGITS+1+DFP_DIGITS-1)/DFP_DIGITS)+1)

typedef struct {
    int exp;				/* decimal exponent */
    unsigned short frac[DFP_NFRAC];	/* DFP_DIGITS chunks:  frac[0] is
					** low order part
					*/
} dfp_t;


static void
dfp_init(pfp)
dfp_t * pfp;
/* Initialize decimal FP:  zero fraction, set exponent assuming
** fraction digits are added via dfp_adddigs().
*/
{
    int i;

    for (i = DFP_NFRAC-1; i >= 0; --i)
	pfp->frac[i] = 0;
    
    pfp->exp = -(DFP_NFRAC * DFP_DIGITS);
    return;
}


static void
dfp_adddigs(pfp, n)
dfp_t * pfp;
unsigned short n;
/* Add high-order digits word to decimal FP number.  Round if we're
** about to throw away a low-order word > DFP_MAXVAL/2.
*/
{
    int i;
    int carry = (int) pfp->frac[0] >= DFP_MAXVAL/2;

    /* Move fraction words down one, increment exponent. */
    for (i = 0; i < DFP_NFRAC-1; ++i) {
	pfp->frac[i] = pfp->frac[i+1] + carry;
	carry = 0;
	if (pfp->frac[i] >= DFP_MAXVAL) {
	    carry = 1;
	    pfp->frac[i] = 0;
	}
    }
    pfp->frac[DFP_NFRAC-1] = n;			/* set new high part */
    pfp->exp += DFP_DIGITS;
    return;
}


static void
dfp_mul3(px, py, pr)
dfp_t * px;
dfp_t * py;
dfp_t * pr;
/* Multiply decimal FP numbers *px and *py, giving result *pr.
** The result may share space with the operands.
*/
{
    int resexp = px->exp + py->exp;	/* anticipated result exponent */
    unsigned long partials[2*DFP_NFRAC];
    int i, j;

    /* Get partial products for result.  For 4-digit by 4-digit
    ** multiplies, there is room to hold the sum of 42 such
    ** products in a 32-bit unsigned long.  Therefore, collect
    ** partial products, then propagate carries.
    ** Collect the complete set of partial products because
    ** some leading words of the decimal FP values can be zero.
    ** We'll locate the significant part of the product later.
    */

    /* Unroll first iteration so the initialization of the result
    ** array gets folded into it.
    */
    /* for (i = 0; ...) */
    for (j = 0; j < DFP_NFRAC; ++j)
	partials[0+j] = px->frac[0] * py->frac[j];

    for (i = 1; i < DFP_NFRAC; ++i) {
	partials[i+DFP_NFRAC-1] = 0;	/* initialize highest product */
	for (j = 0; j < DFP_NFRAC; ++j)
	    partials[i+j] += px->frac[i] * py->frac[j];
    }
    partials[DFP_NFRAC-1 + DFP_NFRAC-1 + 1] = 0;

    /* Distribute carries. */
    for (i = 0; i <= DFP_NFRAC*2-2; ++i) {
	if (partials[i] >= DFP_MAXVAL) {
	    unsigned long quo = partials[i] / DFP_MAXVAL;
	    partials[i+1] += quo;
	    partials[i] -= quo * DFP_MAXVAL;
	}
    }

    /* Find first non-zero word, but leave DFP_NFRAC words. */
    for (i = DFP_NFRAC*2-1; i >= DFP_NFRAC; --i)
	if (partials[i] != 0) break;
    
    /* Write result to destination.  "i" is index of first word,
    ** and is >= DFP_NFRAC-1.
    */
    for (j = DFP_NFRAC-1; j >= 0; --j, --i)
	pr->frac[j] = partials[i];

    /* Adjust exponent for words not copied. */
    resexp += (i+1) * DFP_DIGITS;
    pr->exp = resexp;

    return;
}



/* Tables of 2^(2^n) and 5^(2^n) for quick exponentiation.  The ones that
** aren't filled in explicitly get filled in by code when they're needed.
*/

static dfp_t two2n[_LDEXPLEN+1] = {
    { 0, { 2 } },			/* 2^(2^0) */
    { 0, { 4 } },			/* 2^(2^1) */
    { 0, { 16 } },			/* 2^(2^2) */
    { 0, { 256 } },			/* 2^(2^3) */
    { 0, { 5536, 6 } },			/* 2^(2^4) */
    { 0, { 7296, 9496, 42, } },		/* 2^(2^5) */
};

static dfp_t five2n[_LDEXPLEN+1] = {
    { 0, { 5 } },			/* 5^(2^0) */
    { 0, { 25 } },			/* 5^(2^1) */
    { 0, { 625 } },			/* 5^(2^2) */
    { 0, { 625, 39 } },			/* 5^(2^3) */
};


static void
dfp_exp(pfp, n, e)
dfp_t * pfp;
int n;
unsigned int e;
/* Multiply decimal FP by n ^ e for positive e. */
{
    /* The algorithm does decimal FP multiplies by n ^ (2^i)
    ** for each "i" bit in "e".  We start with low exponents and
    ** work upward, building the exponent table if necessary.
    */
    dfp_t *ep;

    switch( n ){
    case 2:	ep = two2n; break;
    case 5:	ep = five2n; break;
    default:	_cerror("dfp_exp(): unknown factor %d", n);
    }

    for ( ; e != 0; e >>= 1) {
	if (ep->frac[0] == 0)		/* must compute exponential first */
	    dfp_mul3(&ep[-1], &ep[-1], ep);
	if (e & 1)
	    dfp_mul3(ep, pfp, pfp);
	++ep;
    }
}


static void
dfp_round(pfp, ndig)
dfp_t * pfp;
int ndig;
/* Round decimal floating point into ndig-th digit.  Digits are
** counted beginning with 1.
*/
{
    static unsigned short digs[DFP_DIGITS+1] = { 1, 10, 100, 1000, 10000 };
    int dig;
    int wd;

    /* Have to locate where the ndig+1-th digit is, check
    ** it for >= 5, increment ndig-th digit, propagate
    ** carry.  Find "dig" between 1 and DFP_DIGITS.
    */
    for (dig = 1; pfp->frac[DFP_NFRAC-1] >= digs[dig]; ++dig)
	;

    /* High-order word has "dig" digits.  Determine in which word we'll
    ** find the ndig+1-th digit.
    */
    wd = DFP_NFRAC-1 - (ndig + DFP_DIGITS-dig)/DFP_DIGITS;
    dig = DFP_DIGITS-1 - (ndig + DFP_DIGITS-dig) % DFP_DIGITS;
    /* 0 <= wd < DFP_NFRAC; 0 <= dig < DFP_DIGITS */

    /* See if we need to round. */
    if (((unsigned int) (pfp->frac[wd] / digs[dig])) % 10 >= 5) {
	/* Need to round.  Find place to round, propagate carries. */
	if (++dig >= 4) {
	    dig = 0;
	    ++wd;
	}
	if ((pfp->frac[wd] += digs[dig]) >= DFP_MAXVAL) {
	    /* Rounding produced carry. */
	    do {
		pfp->frac[wd] -= DFP_MAXVAL;
		if (++wd >= DFP_NFRAC) break;
		++pfp->frac[wd];
	    } while (pfp->frac[wd] >= DFP_MAXVAL);
	    if (wd >= DFP_NFRAC)	/* carried out of high-order */
		dfp_adddigs(pfp, 1);
	}
    }
    return;
}


#define ECVTL	1
#define FCVTL	2
#define GCVTL	3

char *
ecvtl(x, ndigit, decpt, sign)
long double x;
int ndigit;
int *decpt, *sign;
{
	return(cvtl(x, ndigit, NULL, decpt, sign, ECVTL));
}

char *
fcvtl(x, ndigit, decpt, sign)
long double x;
int ndigit;
int *decpt, *sign;
{
	return(cvtl(x, ndigit, NULL, decpt, sign, FCVTL));
}

char *
gcvtl(x, ndigit, buffer)
long double x;
int ndigit;
char *buffer;
{
	return(cvtl(x, ndigit, buffer, NULL, NULL, GCVTL));
}

static char *
cvtl(x, ndigit, buffer, decpt, sign, flag)
long double x;
int ndigit;
char *buffer;
int *decpt, *sign;
int flag;
/* Convert double-extended value to string in memory,
** null terminated.
*/
{
    char * retval;
    /* Buffer:  DFP_NFRAC*DFP_DIGITS digits, plus:
    **			1 for sign, 1 for decimal point, 1 for 'e', 5 for 
    **			exp (-eeee), 1 for null
    */
    static char lbuf[(DFP_NFRAC*DFP_DIGITS)+1+1+5+1];
    char *lbufp;
    int lbsize = 0;
    int moveback = 0;
    int fformat = 0;
    _ldval internal;

    internal.ld = x;

    if(flag != GCVTL) *sign = 0;

    if((internal.fparts.exp == 0) && 
	!(internal.fparts.lo | internal.fparts.hi)) retval = "0";
    else if(internal.nparts.exp == LDMAXEXP) {
	if(isnanl(x)) {
		retval = SIGNBITLD(x) ? "-NaN" : "NaN";
		if(flag == GCVTL) (void) strcpy(buffer, retval);
	}
	else {
		retval = SIGNBITLD(x) ? "-inf" : "inf";
		if(flag == GCVTL) (void) strcpy(buffer, retval);
	}
    }
    else {
	/* Must convert number to digits. */
	int i_flag = 0;
	int i;
	int bexp;			/* binary exponnent */
	int dexp = 0;			/* decimal exponent */
	static dfp_t signif;		/* significand decimal FP */
	unsigned long	extra = 0;

	if((internal.fparts.exp == 0) && 
		(internal.fwords.w0 | internal.fwords.w1 | 
		internal.fwords.w2 | internal.fwords.w3)) 
			i_flag = 1; /* denormalized exponent */

	dfp_init(&signif);

	/* Convert to raw digits, worry about exponent later. */
	for (i = (ODIGITS+1+DFP_DIGITS-1)/DFP_DIGITS; i >= 0; --i) {
		unsigned long rem = 0;
		unsigned long val = 0;

		val = (rem << BITS_PER_MAG) + internal.fwords.w3;
		internal.fwords.w3 = val / DFP_MAXVAL;
		rem = val % DFP_MAXVAL;
		val = (rem << BITS_PER_MAG) + internal.fwords.w2;
		internal.fwords.w2 = val / DFP_MAXVAL;
		rem = val % DFP_MAXVAL;
		val = (rem << BITS_PER_MAG) + internal.fwords.w1;
		internal.fwords.w1 = val / DFP_MAXVAL;
		rem = val % DFP_MAXVAL;
		val = (rem << BITS_PER_MAG) + internal.fwords.w0;
		internal.fwords.w0 = val / DFP_MAXVAL;
		rem = val % DFP_MAXVAL;
		val = (rem << BITS_PER_MAG) + extra;
		extra = val / DFP_MAXVAL;
		rem = val % DFP_MAXVAL;

	    dfp_adddigs(&signif, (unsigned short) rem);
	}
	/* Find correct exponents. */
	bexp = internal.fparts.exp - X_EXPBIAS - (LDBL_MANT_DIG+BITS_PER_MAG-1);
#if X_DENORM_BIAS != 0
	if (i_flag)	/* Adjust for denormalized range. */
	    bexp += X_DENORM_BIAS;
#endif

	/* Multiply significand by suitable exponential to scale the result. */
	if (bexp != 0) {
	    if (bexp > 0)
		dfp_exp(&signif, 2, (unsigned int) bexp);
	    else if (bexp < 0) {
		/* x * 2^(-n) = (x * 5^n)/10^n:  multiply by powers of 5,
		** reduce exponent.
		*/
		dexp += bexp;		/* (bexp < 0) */
		dfp_exp(&signif, 5, (unsigned int) -bexp);
	    }
	}
	if(flag != FCVTL) dfp_round(&signif, ndigit);
	else  {
		int high = signif.frac[DFP_NFRAC-1];
		int pt = signif.exp + dexp + ndigit;

		if (pt >= 0)
			pt = ODIGITS;
		else if ((pt += DFP_DIGITS * DFP_NFRAC) <= 0)
			pt = ndigit;

		else if (high < 10) pt += 3;
		else if (high < 100) pt += 2;
		else if (high < 1000) pt += 1;

		dfp_round(&signif, pt);
	}



	/* Convert "signif" to digits in buffer. */
	if(flag == GCVTL) lbufp = &buffer[3];
	else lbufp = &lbuf[3];		/* Leave room for sign, decimal point,
					** carry
					*/
	lbsize = 0;

	for (i = DFP_NFRAC-1; i > 0; --i) /* skim off zero high-order chunks */
	    if (signif.frac[i] != 0) break;

	for ( ; i >= 0; --i)
	    lbsize +=
		sprintf(&lbufp[lbsize], "%.*d", DFP_DIGITS, signif.frac[i]);
	dexp += signif.exp;
	/* "dexp" now gives exponent relative to the number in the buffer
	** with a decimal point to the right of the right-most digit.
	*/

	/* Remove leading zeroes in buffer. */
	while (lbufp[0] == '0') {
	    ++lbufp;
	    --lbsize;
	}
	/* Rounding has already been done.  Truncate to ndigit digits. */
	if (lbsize > ndigit) {
	    dexp += lbsize - ndigit;
	    lbsize = ndigit;
	}
	/* figure out where decimal point belongs for ecvt, fcvt	*/
	if(flag != GCVTL) {
		if(lbsize > 1) dexp += lbsize - 1;
		*decpt = dexp + 1;
	}
	/* Truncate for fcvt. */
	if(flag == FCVTL) lbsize += *decpt;

	/* Remove trailing zeroes. not for ecvt or fcvt */
	if(flag == GCVTL) {
		while (lbufp[lbsize-1] == '0') {
		    --lbsize;
		    ++dexp;
		}
	}

	/* At this point, "lbufp" points to a buffer of "lbsize" digits,
	** '0'-'9', and "dexp" is the exponent of the number.  Place a
	** sign, a decimal point, and an exponent, if necessary.  Also
	** stick in null termination.
	*/

	/* put in the decimal point for gcvt	*/
	if(flag == GCVTL) {
		if (lbsize > 1) {
		    --lbufp;
		    lbufp[0] = lbufp[1];
		    lbufp[1] = _numeric[0];
		    dexp += lbsize - 1;
		    ++lbsize;
		}
		/* if((dexp + 1 > ndigit) || (dexp <= -5)) */
			/* use E-style	*/;
		if((dexp + 1 <= ndigit) && (dexp > -5)) {
			if(dexp + 1 >= 0) {
				/* move decimal point to the right	*/
				int j;
	
				for(j = 1; j <= dexp; j++) 
					lbufp[j] = lbufp[1 + j];
				lbufp[j] = _numeric[0];
				++fformat;
				++lbsize;
			}
			else {	/* move decimal point to the left	*/
				int j;
				char *tmpbuf = (char *)malloc(ndigit + 
					dexp + 4);
	
				(void) strcpy(tmpbuf, "0.");
				for(j = 0; j < -dexp - 1; j++) 
					tmpbuf[2 + j] = '0';
				tmpbuf[2 + j] = lbufp[0];
				(void) strcpy(&tmpbuf[j + 3], &lbufp[2]);
				(void) strcpy(&lbufp[0], tmpbuf);
				free(tmpbuf);
				++fformat;
				lbsize += -dexp;
			}
		}
	}

	/* for ecvt, fcvt	*/
	if(flag != GCVTL) {
		if (SIGNBITLD(x)) *sign = 1;
	}
	else {
		if (SIGNBITLD(x)) {
		    --lbufp;
		    lbufp[0] = '-';
		    ++lbsize;
		}
		++moveback;
	}

	if ((dexp != 0) && (flag == GCVTL) && !fformat) {
	    char *efmt;

	    efmt = (dexp >= 0) ? "e+%d" : "e%d";
	    (void) sprintf(&lbufp[lbsize], efmt, dexp);
	}
	else
	    lbufp[lbsize] = '\0';

	if((flag == GCVTL) && moveback) {
		int i;
		for(i = 0; lbufp[i] != '\0'; i++) buffer[i] = lbufp[i];
		buffer[i] = '\0';
		lbufp = buffer;
	}

	retval = lbufp;
    }
    return retval;
}
