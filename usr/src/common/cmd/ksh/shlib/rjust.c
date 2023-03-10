/*		copyright	"%c%" 	*/

#ident	"@(#)ksh:shlib/rjust.c	1.2.6.2"
#ident "$Header$"

/*
 *   NAM_RJUST.C
 *
 *   Programmer:  D. G. Korn
 *
 *        Owner:  D. A. Lambeth
 *
 *         Date:  April 17, 1980
 *
 *
 *
 *   NAM_RJUST (STR, SIZE, FILL)
 *
 *      Right-justify STR so that it contains no more than 
 *      SIZE non-blank characters.  If necessary, pad with
 *      the character FILL.
 *
 *
 *
 *   See Also:  
 */

#ifdef KSHELL
#include	"shtype.h"
#else
#include	<ctype.h>
#endif	/* KSHELL */

/*
 *   NAM_RJUST (STR, SIZE, FILL)
 *
 *        char *STR;
 *
 *        int SIZE;
 *
 *        char FILL;
 *
 *   Right-justify STR so that it contains no more than
 *   SIZE characters.  If STR contains fewer than SIZE
 *   characters, left-pad with FILL.  Trailing blanks
 *   in STR will be ignored.
 *
 *   If the leftmost digit in STR is not a digit, FILL
 *   will default to a blank.
 */

void	nam_rjust(str,size,fill)
char *str,fill;
int size;
{
	register int n;
	register char *cp,*sp;
	n = strlen(str);

	/* ignore trailing blanks */

	for(cp=str+n;n && *--cp == ' ';n--);
	if (n == size) return;
	if(n > size)
        {
        	*(str+n) = 0;
        	for (sp = str, cp = str+n-size; sp <= str+size; *sp++ = *cp++);
        	return;
        }
	else *(sp = str+size) = 0;
	if (n == 0)  
        {
        	while (sp > str)
               		*--sp = ' ';
        	return;
        }
	while(n--)
	{
		sp--;
		*sp = *cp--;
	}
	if(!isdigit(*str))
		fill = ' ';
	while(sp>str)
		*--sp = fill;
	return;
}

