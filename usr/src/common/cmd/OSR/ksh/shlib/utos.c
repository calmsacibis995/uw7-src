#ident	"@(#)OSRcmds:ksh/shlib/utos.c	1.1"
#pragma comment(exestr, "@(#) utos.c 25.3 93/01/20 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 *  Modification History
 *
 *	L000	scol!markhe	25 Nov 92
 *	- corrected type of e_hdigits.
 */
/*
 *   UTOS.C
 *
 *   Programmer:  D. G. Korn
 *
 *         Date:  May 3, 1982
 *
 *
 *
 *   LTOS (SINT, BASE)
 *
 *        Return a pointer to a string denoting the value of
 *        the signed long integer SINT in base BASE.
 *
 *   UTOS (USINT, BASE)
 *
 *        Return a pointer to a string denoting the value of
 *        the unsigned long integer USINT in base BASE.
 *
 *
 *
 *   See Also:  arith(III)
 */

#include	"defs.h"					/* L000 */

#define BASEMAX	 (4+16*sizeof(int))
static char hexstr[BASEMAX];
extern MSG e_hdigits;						/* L000 */
char *utos();
char *ltos();
#ifdef FLOAT
    char *ftos();
    char *etos();
#endif /* FLOAT */

/*
 *   LTOS (SINT, BASE)
 *
 *        long USINT;
 *
 *        int BASE;
 *
 *   Return a pointer to a string denoting the value of SINT 
 *   in base BASE.  The string will be stored within HEXSTR.
 *   It will begin with the base followed by a single '#'.
 *   A minus sign will be prepended for negative numbers
 *
 */


char *ltos(sint,base)
long sint;
int base;
{
	register char *sp;
	register long l = (sint>=0?sint:-sint);
#ifdef pdp11
	sp = utos(l,base);
#else
	sp = utos((unsigned long)l,base);
#endif /* pdp11 */
	if(sint<0)
		*--sp = '-';
	return(sp);
}

/*
 *   UTOS (USINT, BASE)
 *
 *        unsigned USINT;
 *
 *        int BASE;
 *
 *   Return a pointer to a string denoting the value of USINT 
 *   in base BASE.  The string will be stored within HEXSTR.
 *   It will begin with the base followed by a single '#'.
 *
 */



char *utos(usint,base)
register int base;
#ifdef pdp11
 /* unsigned longs are not supported on pdp11 */
long usint;
{
	long l = usint;
#else
unsigned long usint;
{
	register unsigned long l = usint;
#endif	/* pdp11 */
	register char *cp = hexstr+(BASEMAX-1);
	if(base < 2 || base > BASEMAX)
		return(cp);
	for(*cp = 0;cp > hexstr && l;l /= base)
		*--cp = e_hdigits[(l%base)<<1];
	if(usint==0)
		*--cp = '0';
	if(base==10)
		return(cp);
	*--cp = '#';
	*--cp = e_hdigits[(base%10)<<1];
	if(base /= 10)
		*--cp = e_hdigits[(base%10)<<1];
	return(cp);	
}

#ifdef FLOAT
/*
 *   FTOS (VAL, PLACES)
 *
 *        double VAL;
 *
 *        int PLACES;
 *
 *   Return a pointer to a string denoting the value of VAL 
 *   with PLACES places after the decimal point.  The string
 *   will be stored within HEXSTR.
 *
 */

char *ftos(val,places)
double val;
int places;
{
	register char *sp=hexstr;
	register char *cp;
	extern char *fcvt();
	int decpt,sign;
	cp = fcvt(val,places,&decpt,&sign);
	if(sign)
		*sp++ = '-';
	while(decpt-- > 0)
		*sp++ = *cp++;
	*sp++ = '.';
	while(++decpt < 0)
		*sp++  = '0';
	while(*sp++ = *cp++);
	return(hexstr);
}

char *etos(val,places)
double val;
int places;
{
	extern char *gcvt();
	return(gcvt(val,places,hexstr));
}
#endif /* FLOAT */
