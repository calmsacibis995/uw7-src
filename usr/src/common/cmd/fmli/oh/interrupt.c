/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1988 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/interrupt.c	1.3.3.3"

#include "wish.h"

/*  routines defined in this file: */

void intr_handler();



/* declare data structures for interrupt  feature
 * the following is a copy of interrupt.h without the `extern'
 */


/* the current definition of the oninterrupt descriptor */
struct {
    bool  interrupt;
    char *oninterrupt;
    bool  skip_eval;
} Cur_intr;


/* intr_handler
          This routine will execute  on receipt of a SIGINT while
	  executing a fmli builtin  or external executable (if
	  interrupts are enabled)
	  
	  Sets flag  to tell eval() to throw away the rest of the
	  descriptor it is parsing.
*/
void
intr_handler()
{
    Cur_intr.skip_eval = TRUE;
}

