/*		copyright	"%c%" 	*/

/* defines and data structures for interrupt  feature */
/*
 * Copyright  (c) 1988 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:inc/interrupt.h	1.1.4.3"


#define DEF_ONINTR "`message Operation interrupted!`NOP"

#ifndef TYPE_BOOL
/* curses.h also  does a typedef bool */
#ifndef CURSES_H
#define TYPE_BOOL
typedef char bool;
#endif
#endif

extern struct {
    bool  interrupt;
    char *oninterrupt;
    bool  skip_eval;
} Cur_intr;
