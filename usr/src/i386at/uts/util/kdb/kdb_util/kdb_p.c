#ident	"@(#)kern-i386at:util/kdb/kdb_util/kdb_p.c	1.1"

/*
 * Platform-specific routines for KDB.
 */

#include <util/kdb/kdb/debugger.h>

/*
 * void kdb_pdep_enter(void)
 *	Hook called on interactive entry into the kernel debugger,
 *	which will hold all processors and interrupts disabled for an
 *	arbitarily long time.  This can be used to disable watchdog timers
 *	and the like.
 *
 * Calling/Exit State:
 *	We are single-threaded at this point.  The caller has already
 *	suspended the other processors.
 */
void
kdb_pdep_enter(void)
{
}

/*
 * void kdb_pdep_exit(void)
 *	Hook called on exit from the kernel debugger interactive mode.
 *
 * Calling/Exit State:
 *	We are still single-threaded at this point.
 */

void
kdb_pdep_exit(void)
{
}
